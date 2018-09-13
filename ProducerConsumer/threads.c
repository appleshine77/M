#include "defs.h"

double time2ms_interval(struct timeval* start, struct timeval* end) {
	double ret = (double)((end->tv_sec - start->tv_sec)*S2US + (end->tv_usec - start->tv_usec))/1000;
//	fprintf(stdout,"%012.3f %012.3f\n", (double)(start->tv_sec), (double)(end->tv_sec));
	return ret;
}

/* return --- TRUE: move a packet; FALSE: not move a packet because of having no enough token. */
int move_packet_Q1toQ2(Para* para) {
	//if tokens in the filter are enough for cur packet, move the packet from Q1 to Q2
	My402ListElem* move_node = My402ListFirst(para->list_Q1); //move the first because of FIFO
	struct timeval cur_time;
	double ms_stamp;

	if (move_node) {
		Packet* move_packet = (Packet*)(move_node->obj);
		para->is_Q1_having_packets = TRUE;
		if (para->token_remained < move_packet->P) {
			//fprintf(stdout, "move packet from Q1 to Q2 : failure because of no enough tokens.\n");
			return FALSE;
		}
		//the node leaved Q1
		My402ListUnlink(para->list_Q1, move_node);

		gettimeofday(&cur_time, NULL);
		ms_stamp = time2ms_interval(&(para->start_time), &cur_time);

		move_packet->ms_in_Q1 = time2ms_interval(&(move_packet->start_in_Q1), &cur_time);
		para->token_remained = para->token_remained - move_packet->P;
		fprintf(stdout, "%012.3fms: p%ld leaves Q1, time in Q1 = %.3fms, token bucket now has %ld token\n",
			ms_stamp, move_packet->packet_id, move_packet->ms_in_Q1, para->token_remained);

		//the node enter into Q2
		if (!My402ListAppend(para->list_Q2, move_packet)) {
			exit_err("move packet from Q1 to Q2 : append a packet to Q2 failure.", para);
		}
        gettimeofday(&(move_packet->start_in_Q2), NULL);
        ms_stamp = time2ms_interval(&(para->start_time), &(move_packet->start_in_Q2));

		para->is_guard_Q2_empty++;
		fprintf(stdout, "%012.3fms: p%ld enters Q2\n", ms_stamp, move_packet->packet_id);

		//if Q2 was empty before, the thread need notify servers having a new packet coming after adding a new packet.
		if (My402ListLength(para->list_Q2) == 1) { //because append processes first, so here has already one packet is appended. therefore, checking equal one or not to judge empty or not before.
			pthread_cond_broadcast(&para->Q2_no_empty_cond);
		}
		return TRUE;
	}
	para->is_Q1_having_packets = FALSE;
	return FALSE;
}

void* packet_arrival_handler(Para* para) {
	void* ret = NULL;
	struct timeval last_start_time, cur_time;
	double ms_stamp;
	My402ListElem* cur_node = My402ListFirst(para->list);

	last_start_time.tv_sec = para->start_time.tv_sec;
	last_start_time.tv_usec = para->start_time.tv_usec;

	while (cur_node) {
		pthread_mutex_lock(&para->mutex);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);

		Packet* cur_packet = (Packet*)(cur_node->obj);
		//sleep inter-arrival time
		gettimeofday(&cur_time, NULL);
		double remain_sleep_ms = cur_packet->inter_arrival_ms - time2ms_interval(&last_start_time, &cur_time);
	    if (remain_sleep_ms > 0) {
	    	pthread_mutex_unlock(&para->mutex);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
			usleep(remain_sleep_ms * MS2US);
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
			//wake up
			//lock the mutex
			pthread_mutex_lock(&para->mutex);
			gettimeofday(&cur_packet->arrival_time, NULL);
	    } else {
	    	cur_packet->arrival_time = cur_time;
	    }
		ms_stamp = time2ms_interval(&(para->start_time), &cur_packet->arrival_time);
		cur_packet->ms_inter_arrival_time = time2ms_interval(&last_start_time, &cur_packet->arrival_time);

		if (cur_packet->P > para->B) {
			para->static_drop_packets++;
			fprintf(stdout, "%012.3fms: p%ld arrives, needs %ld tokens, inter-arrival time = %.3fms, dropped\n",
					ms_stamp, cur_packet->packet_id, cur_packet->P, cur_packet->ms_inter_arrival_time);
			//point to next packet
			last_start_time = cur_packet->arrival_time;
			cur_node = My402ListNext(para->list, cur_node);
			//unlock mutex
			pthread_mutex_unlock(&para->mutex);
			continue;
		} else {
			fprintf(stdout, "%012.3fms: p%ld arrives, needs %ld tokens, inter-arrival time = %.3fms\n",
					ms_stamp, cur_packet->packet_id, cur_packet->P, cur_packet->ms_inter_arrival_time);
		}
		//encode a new packet into Q1
		gettimeofday(&(cur_packet->start_in_Q1), NULL);
		ms_stamp = time2ms_interval(&(para->start_time), &(cur_packet->start_in_Q1));
		My402ListAppend(para->list_Q1, cur_packet); //adding to the Q1 tail
		fprintf(stdout, "%012.3fms: p%ld enters Q1\n", ms_stamp, cur_packet->packet_id);

		//move a packet from Q1 to Q2
		move_packet_Q1toQ2(para);

		//point to next packet
		last_start_time = cur_packet->arrival_time;
		cur_node = My402ListNext(para->list, cur_node);
		//unlock mutex
		pthread_mutex_unlock(&para->mutex);
		if (!cur_node) {
			break;
		}
	}
	if (para->batch_id >= para->batch_num) {
		para->is_all_packets_received = TRUE;
		pthread_cond_broadcast(&para->Q2_no_empty_cond);
	}
	//fprintf(stdout, "packet arrival handler : no any packet need to be processed.\n");
	return ret;
}

void* token_depositing_hander(Para* para) {
	void* ret = NULL;
	struct timeval last_start_time, cur_time, new_start_time;
	double ms_stamp;

	last_start_time.tv_sec = para->start_time.tv_sec;
	last_start_time.tv_usec = para->start_time.tv_usec;

	while (para->server_state == BOTH_WORK) {
	    //lock the mutex
	    pthread_mutex_lock(&para->mutex);

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
		//sleep inter-arrival time
		gettimeofday(&cur_time, NULL);
		double remain_sleep_ms = (1/para->r)*S2MS - time2ms_interval(&last_start_time, &cur_time);
	    pthread_mutex_unlock(&para->mutex);
      	if (remain_sleep_ms > 0) {
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
			usleep(remain_sleep_ms * MS2US);
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
		}
        //lock the mutex
        pthread_mutex_lock(&para->mutex);
     
		//wake up
		gettimeofday(&new_start_time, NULL);
		ms_stamp = time2ms_interval(&(para->start_time), &new_start_time);

		if (para->server_state == BOTH_STOP) {
			pthread_mutex_unlock(&para->mutex);
			pthread_exit(0);
			break;
		}
		//increment a token count
		if (para->token_remained == 0) {
			fprintf(stdout, "%012.3fms: token t%ld arrives, token bucket now has %ld token\n",
				ms_stamp, ++para->token_id, ++para->token_remained);
		} else if (para->token_remained >= para->B) {
			//token is full, drop a new one
			fprintf(stdout, "%012.3fms: token t%ld arrives, dropped\n",
							ms_stamp, ++para->token_id);
			para->static_drop_tokens++;

			//continue to sleep
			last_start_time = new_start_time;

			//unlock mutex
			pthread_mutex_unlock(&para->mutex);
			continue;
		} else {
			fprintf(stdout, "%012.3fms: token t%ld arrives, token bucket now has %ld tokens\n",
				ms_stamp, ++para->token_id, ++para->token_remained);
		}

		//move a packet from Q1 to Q2
		move_packet_Q1toQ2(para);

		//ready to create the next token
		last_start_time = new_start_time;

		if (para->server_state != BOTH_WORK) {
			//unlock mutex
			pthread_mutex_unlock(&para->mutex);
			pthread_exit(0);
		}
		//unlock mutex
		pthread_mutex_unlock(&para->mutex);
	}
	return ret;
}

Packet* move_packet_Q2toS(Para* para, int server_id) {
	My402ListElem* move_node = My402ListFirst(para->list_Q2); //move the first because of FIFO
	Packet* move_packet = (Packet*)(move_node->obj);
	struct timeval start_time;
	double ms_stamp;

	if (!move_node) {
		exit_err("move packet Q2 to S : failure because of Q2 empty.", para);
	}

	//the node leaved Q2 to some one server
	gettimeofday(&start_time, NULL);

	if (server_id == 1) {
		move_packet->start_by_S1 = start_time;
	} else {
		move_packet->start_by_S2 = start_time;
	}

	ms_stamp = time2ms_interval(&(para->start_time), &start_time);
	move_packet->ms_in_Q2 = time2ms_interval(&(move_packet->start_in_Q2), &start_time);

	fprintf(stdout, "%012.3fms: p%ld leaves Q2, time in Q2 = %.3fms\n",
			ms_stamp, move_packet->packet_id, move_packet->ms_in_Q2);
	My402ListUnlink(para->list_Q2, move_node);

	para->is_guard_Q2_empty--;

	return move_packet;
}
void two_servers_exit(Para* para) {
	struct timeval cur_time;

	if (para->is_guard_Q2_empty == EMPTY &&
	     para->is_all_packets_received &&
		 para->is_Q1_having_packets == FALSE) {

		if (para->batch_id < para->batch_num) {
			para->server_state = BOTH_WORK;
			get_batch_packets(para);
			return;
		}
	}
	//ctrl + C exit
	if (para->is_guard_Q2_empty == CTRL_C_OCCUR
	 ||(para->is_guard_Q2_empty == EMPTY && //packet all received and some packets dropped before
	    para->is_all_packets_received &&
		para->is_Q1_having_packets == FALSE)) {
		para->is_guard_Q2_empty = ONLY_ONE_SRV_WORK;
		para->server_state = ONLY_ONE_WORK;
		pthread_cond_broadcast(&para->Q2_no_empty_cond);
		pthread_mutex_unlock(&para->mutex);
		pthread_exit(0);
	} else if (para->is_guard_Q2_empty == ONLY_ONE_SRV_WORK) {
		para->server_state = BOTH_STOP;
		pthread_cancel(para->thread_user_id);
		gettimeofday(&cur_time, NULL);
		para->ms_total_emulation = time2ms_interval(&(para->start_time), &cur_time);
		fprintf(stdout, "emulation ends\n");
		pthread_mutex_unlock(&para->mutex);
		pthread_exit(0);
	}
    //normally exit often all packets are processed
	if (para->server_state == ONLY_ONE_WORK) {//one server is serving the last packet, so here exit one server
		para->server_state = BOTH_STOP;
		pthread_mutex_unlock(&para->mutex);
		pthread_exit(0);
	} else if (para->server_state == BOTH_STOP) {//here exit another server
		pthread_cancel(para->thread_user_id); 
		gettimeofday(&cur_time, NULL);
		para->ms_total_emulation = time2ms_interval(&(para->start_time), &cur_time);
		fprintf(stdout, "emulation ends\n");
        pthread_mutex_unlock(&para->mutex);
		pthread_exit(0);
	}
}
void* server_handler(Para* para) {
	void* ret = NULL;

	struct timeval cur_time;
	int server_id;
	pthread_t self;
	double ms_stamp;

	while (para->server_state == BOTH_WORK && para->is_guard_Q2_empty >= EMPTY) {
		//lock the mutex
		pthread_mutex_lock(&para->mutex);
		//get current thread being server1 or server2
		if ((self = pthread_self()) == para->thread_server1_id) {
			server_id = 1;
		} else if ((self = pthread_self()) == para->thread_server2_id) {
			server_id = 2;
		} else {
			exit_err("server handler : unexpected pthread id.", para);
		}
		while (para->is_guard_Q2_empty == EMPTY && para->server_state == BOTH_WORK && !(para->is_all_packets_received && !para->is_Q1_having_packets)) {
			pthread_cond_wait(&para->Q2_no_empty_cond, &para->mutex);
		}
		two_servers_exit(para);
		//Q2 is not empty, dequeue the packet
		Packet* move_packet = move_packet_Q2toS(para, server_id);
		if (move_packet->packet_id >= para->num) {
			para->server_state = ONLY_ONE_WORK;
		}
		//work/sleep during service time
		gettimeofday(&cur_time, NULL);
		ms_stamp = time2ms_interval(&(para->start_time), &cur_time);
		//the node begin service at server
		fprintf(stdout, "%012.3fms: p%ld begins service at S%d, requesting %.3fms of service\n",
				ms_stamp, move_packet->packet_id, server_id, move_packet->serve_ms);

		double remain_sleep_ms;
		if (para->is_trace_driven) {
			if (server_id == 1) {
				remain_sleep_ms = move_packet->serve_ms - time2ms_interval(&move_packet->start_by_S1, &cur_time);
			} else {
				remain_sleep_ms = move_packet->serve_ms - time2ms_interval(&move_packet->start_by_S2, &cur_time);
			}
		} else {
			if (server_id == 1) {
				remain_sleep_ms = S2MS*1/para->mu - time2ms_interval(&move_packet->start_by_S1, &cur_time);
			} else {
				remain_sleep_ms = S2MS*1/para->mu - time2ms_interval(&move_packet->start_by_S2, &cur_time);
			}
		}
        //unlock the mutex
        pthread_mutex_unlock(&para->mutex);

		if (remain_sleep_ms > 0) {
			usleep(remain_sleep_ms * MS2US);
		}
		//lock the mutex
        pthread_mutex_lock(&para->mutex);

		gettimeofday(&cur_time, NULL);
		ms_stamp = time2ms_interval(&(para->start_time), &cur_time);
		move_packet->ms_in_system = time2ms_interval(&move_packet->arrival_time, &cur_time);
		if (server_id == 1) {
			move_packet->ms_in_S1 = time2ms_interval(&(move_packet->start_by_S1), &cur_time);
			fprintf(stdout, "%012.3fms: p%ld departs from S%d, service time = %.3fms, time in system = %.3fms\n",
				ms_stamp, move_packet->packet_id, server_id, move_packet->ms_in_S1, move_packet->ms_in_system);
		} else {
			move_packet->ms_in_S2 = time2ms_interval(&(move_packet->start_by_S2), &cur_time);
			fprintf(stdout, "%012.3fms: p%ld departs from S%d, service time = %.3fms, time in system = %.3fms\n",
				ms_stamp, move_packet->packet_id, server_id, move_packet->ms_in_S2, move_packet->ms_in_system);
		}
		//here exit normally and finished all during the time no ctrl+C happened.
		para->num_completed_packets = move_packet->packet_id;
		two_servers_exit(para);
		//unlock the mutex
        pthread_mutex_unlock(&para->mutex);
	}
	return ret;
}
void remove_packets_in_all_QQ1Q2(Para* para) {
	My402ListElem* move_node = NULL;
	Packet* move_packet = NULL;
	double ms_stamp;
	struct timeval cur_time;

	if (para->list_Q1) {
		while (!My402ListEmpty(para->list_Q1)) {
			move_node = My402ListFirst(para->list_Q1); //move the first because of FIFO
			move_packet = (Packet*)(move_node->obj);

			gettimeofday(&cur_time, NULL);
			ms_stamp = time2ms_interval(&(para->start_time), &cur_time);
			fprintf(stdout, "%012.3fms: p%ld removed from Q%d\n", ms_stamp, move_packet->packet_id, 1);
			My402ListUnlink(para->list_Q1, move_node);
		}
	}
	if (para->list_Q2) {
		while (!My402ListEmpty(para->list_Q2)) {
			move_node = My402ListFirst(para->list_Q2); //move the first because of FIFO
			move_packet = (Packet*)(move_node->obj);

			gettimeofday(&cur_time, NULL);
			ms_stamp = time2ms_interval(&(para->start_time), &cur_time);
			fprintf(stdout, "%012.3fms: p%ld removed from Q%d", ms_stamp, move_packet->packet_id, 2);
			My402ListUnlink(para->list_Q2, move_node);
		}
	}
	para->is_guard_Q2_empty = CTRL_C_OCCUR;//ctrl+C happen when is -2
	pthread_cond_broadcast(&para->Q2_no_empty_cond);

	return;
}
void* sig_int_handler(Para* para) {
	void* ret = NULL;
	int sig;

	while (1) {
		sigwait(&(para->set), &sig);

		fprintf(stdout, "SIGINT caught, no new packets or tokens will be allowed\n");
		pthread_cancel(para->thread_packet_id);
		pthread_cancel(para->thread_token_id);
		pthread_mutex_lock(&para->mutex);
		remove_packets_in_all_QQ1Q2(para);
		pthread_mutex_unlock(&para->mutex);
		pthread_exit(0);
	}
	return ret;
} 
