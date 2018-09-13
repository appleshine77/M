#include "defs.h"

void exit_err(char* err, Para* para)
{
	fprintf(stderr, "%s\n", err);
	reset(para);
	exit(1);
}

My402List* init_list(Para* para) {
	My402List* new_list = (My402List*)malloc(sizeof(My402List));

	if (new_list == NULL) {
		exit_err("init list malloc failure.", para);
	}

	new_list->num_members = 0;
	new_list->anchor.prev = &(new_list->anchor);
	new_list->anchor.next = &(new_list->anchor);
	new_list->anchor.obj = NULL;

	return new_list;
}

Packet* initialize_packet(Para* para) {
	Packet* packet = malloc(sizeof(Packet));

	if (!packet) { exit_err("initialize a packet malloc failure.", para); }

	packet->packet_id = 0;
	packet->inter_arrival_ms = 0; //ms
	packet->ms_inter_arrival_time = 0;
	packet->P = 0;
	packet->start_in_Q1.tv_sec = 0;
	packet->start_in_Q1.tv_usec = 0;
	packet->ms_in_Q1 = 0;
	packet->start_in_Q2.tv_sec = 0;
	packet->start_in_Q2.tv_usec = 0;
	packet->ms_in_Q2 = 0;
	packet->serve_ms = 0;//ms
	packet->start_by_S1.tv_sec = 0;
	packet->start_by_S2.tv_usec = 0;
	packet->start_by_S1.tv_sec = 0;
	packet->start_by_S2.tv_usec = 0;
	packet->ms_in_system = 0;
	packet->ms_in_S1 = 0;
	packet->ms_in_S2 = 0;

	return packet;
}
int isNumber(char* s) {
	int ret = TRUE, i = 0;
	if (s == NULL || strlen(s) == 0) {
		return FALSE;
	}
	for (i = 0; i < strlen(s); i++) {
		ret = (*(s+i) >= '0' && *(s+i) <= '9');
		if (!ret) {
			return FALSE;
		}
	}
	return ret;
}
void get_batch_packets(Para* para) {
	para->batch_id++;

	int y = 0;
	if (para->batch_id > 1) {
		statistics(para);
		reset(para);
	}
	if (para->batch_last_size && para->batch_id == para->batch_num) {
		for (y = 0; y < para->batch_last_size; y++) {
			Packet* packet = initialize_packet(para);
			packet->inter_arrival_ms = round(1000/para->lambda);
			packet->P = para->P;

			packet->serve_ms = round(1000/para->mu);
			packet->packet_id = MAX_PACKET_LIST_Q_SIZE * (para->batch_num-1) + y + 1;
			My402ListAppend(para->list, packet);
		}
	} else {
		for (y = 0; y < MAX_PACKET_LIST_Q_SIZE; y++) {
			Packet* packet = initialize_packet(para);
			packet->inter_arrival_ms = round(1000/para->lambda);
			packet->P = para->P;

			packet->serve_ms = round(1000/para->mu);
			packet->packet_id = (para->batch_id-1)*MAX_PACKET_LIST_Q_SIZE + y + 1;
			My402ListAppend(para->list, packet);
		}
	}
}
Para* parse_command(int argc, char* argv[], Para* para) {
	FILE* fp = NULL;
	int i = 0;
	double dval=(double)0;
	//update parameters based on command line input
	if (argc > 1) {
		for (i = 1; i < argc; i=i+2) {
			if (!strcmp(argv[i], "-lambda") && argv[i+1]) {
				if (sscanf(argv[i+1], "%lf", &dval) != 1) {
					fprintf(stderr, "cannot parse argv[%d] to get a double value\n", i+1);
					exit(1);
				}
				para->lambda = atof(argv[i+1]);
				para->lambda_user = para->lambda;
			} else if (!strcmp(argv[i], "-mu") && argv[i+1]) {
				if (sscanf(argv[i+1], "%lf", &dval) != 1) {
					fprintf(stderr, "cannot parse argv[%d] to get a double value\n", i+1);
					exit(1);
				}
				para->mu = atof(argv[i+1]);
				para->mu_user = para->mu;
			} else if (!strcmp(argv[i], "-r") && argv[i+1]) {
				if (sscanf(argv[i+1], "%lf", &dval) != 1) {
					fprintf(stderr, "cannot parse argv[%d] to get a double value\n", i+1);
					exit(1);
				}
				para->r = atof(argv[i+1]);
				para->r_user = para->r;
				if (1/para->r > 10) { para->r = 0.1; }
			} else if (!strcmp(argv[i], "-B") && argv[i+1]) {
				if (sscanf(argv[i+1], "%lf", &dval) != 1) {
					fprintf(stderr, "cannot parse argv[%d] to get a double value\n", i+1);
					exit(1);
				}
				para->B = atol(argv[i+1]);
			} else if (!strcmp(argv[i], "-P") && argv[i+1]) {
				if (sscanf(argv[i+1], "%lf", &dval) != 1) {
					fprintf(stderr, "cannot parse argv[%d] to get a double value\n", i+1);
					exit(1);
				}
				para->P = atol(argv[i+1]);
			} else if (!strcmp(argv[i], "-n") && argv[i+1]) {
				if (sscanf(argv[i+1], "%lf", &dval) != 1) {
					fprintf(stderr, "cannot parse argv[%d] to get a double value\n", i+1);
					exit(1);
				}
				para->num = atol(argv[i+1]);
			} else if (!strcmp(argv[i], "-t") && argv[i+1]) {
				/*if (sscanf(argv[i+1], "%lf", &dval) != 1) {
					fprintf(stderr, "cannot parse argv[%d] to get a double value", i+1);
					exit(1);
				}*/
				para->is_trace_driven = TRUE;
				strcpy(para->tsfile_name, argv[i+1]);
			} else {
				exit_err("input command line error: should be ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n", para);
			}
		}
	}
	//if in trace driven mode, read contexts from tsfile
	if (para->is_trace_driven) {
		if (!(fp = fopen(para->tsfile_name, "r"))) {
			if (errno == NO_SUCH_FILE_OR_DIRECTORY) {
				exit_err("no such file or directory.", para);
			} else if (errno == PERMISSION_DENIED) {
				exit_err("permission denied.", para);
			}
			exit_err("open tsfile failure.", para);
		}
		char str[MAX_TSFILE_LINE_SIZE];
		char* end = NULL;
		i = 0;
		while (fgets(str, sizeof(str), fp) != NULL) {
			if (i == 0) { //the first line is num
				end = strchr(str, '\n');
				if (end) { *end = '\0'; }
				if (isNumber(&str[0])) {
					para->num = atol(str);
				} else {
					exit_err("input - line 1 is not just a number", para);
				}
			} else {
				Packet* packet = initialize_packet(para);

				//parse inter arrival time
				if ((end = strchr(str, '\t')) || (end = strchr(str, ' '))) {
					*end = '\0';
				} else if (end == NULL) { break; } //has already read the end of the file, probably a only '\n' exist in the last line.
				packet->inter_arrival_ms = atof(str);
				//parse P
				while (++end != NULL && (*end == '\t' || *end == ' ')) { /* do nothing. */ }
				if (end) {
					char* start = end;
					if ((end = strchr(start, '\t')) || (end = strchr(start, ' '))) {
						*end = '\0';
					}
					packet->P = atol(start);
				} else { exit_err("parsing tsfile content is invalid.", para); }
				//parse service time
				while (++end != NULL && (*end == '\t' || *end == ' ')) { /* do nothing. */ }
				if (end) {
					char* start = end;
					if ((end = strchr(start,'\n')) != NULL) {
						*end = '\0';
					}
					packet->serve_ms = atol(start);
				} else { exit_err("parsing tsfile content is invalid.", para); }
				packet->packet_id = i;
				My402ListAppend(para->list, packet);
			}
			i++;
		}
		if (i == 0) {
			exit_err("fgets() tsfile return NULL.", para);
		} else if (para->num > (--i)) {
			exit_err("tsfile num is more than the real packet lines.", para);
		}
	} else { //in deterministic mode
		if (1/para->lambda > 10) { para->lambda = 0.1; }
		if (1/para->mu > 10) { para->mu = 0.1; }

		if (para->num > MAX_PACKET_LIST_Q_SIZE) {
			if (para->num % MAX_PACKET_LIST_Q_SIZE) {
				para->batch_last_size = para->num % MAX_PACKET_LIST_Q_SIZE;
				para->batch_num = para->num / MAX_PACKET_LIST_Q_SIZE + 1;
			} else {
				para->batch_num = para->num / MAX_PACKET_LIST_Q_SIZE;
			}
		} else if (para->num > 0) {
			para->batch_num = 1;
			para->batch_last_size = para->num;
		}
		get_batch_packets(para);
	}
	//print parameters
	fprintf(stdout, "Emulation Parameters:\n");
	if (para->is_trace_driven == FALSE) {
		if (para->batch_num > 1) {
			fprintf(stdout, "\tnumber to arrive = %ld\t(if -t is not specified)\n", para->num);
		} else {
			fprintf(stdout, "\tnumber to arrive = %ld\t\t(if -t is not specified)\n", para->num);
		}
		fprintf(stdout, "\tlambda = %.6g\t\t\t(if -t is not specified)\n", para->lambda_user);
		fprintf(stdout, "\tmu = %.6g\t\t\t(if -t is not specified)\n", para->mu_user);
	}
	fprintf(stdout, "\tr = %.6g\n", para->r_user);
	fprintf(stdout, "\tB = %ld\n", para->B);
	if (para->is_trace_driven == FALSE) {
		fprintf(stdout, "\tP = %ld\t\t\t\t(if -t is not specified)\n", para->P);
	} else {
		fprintf(stdout, "\ttsfile = %s\t\t\t\(if -t is specified)\n", para->tsfile_name);
	}
	return para; //the list has already stored packets.
}

void process(Para *para) {
	void* result = NULL;

	//pthread_t thread_sig_int_id;

	gettimeofday(&(para->start_time), NULL);
	fprintf(stdout, "%012.3fms: emulation begins\n", (double)0);

	sigemptyset(&para->set);
	sigaddset(&para->set, SIGINT);
	sigprocmask(SIG_BLOCK, &para->set, NULL);
	//pthread_sigmask(SIG_BLOCK, &para->set, NULL);
	//sigwait(&(para->set));
	//fprintf(stdout, "SIGINT caught main\n");

	// create packet arrival thread
	pthread_create(&para->thread_packet_id, NULL, (void*)packet_arrival_handler, (void*)para);
	// create token depositing thread
	pthread_create(&para->thread_token_id, NULL, (void*)token_depositing_hander, (void*)para);
	// create server 1 thread
	pthread_create(&para->thread_server1_id, NULL, (void*)server_handler, (void*)para);
	// create server 2 thread
	pthread_create(&para->thread_server2_id, NULL, (void*)server_handler, (void*)para);
	// create signal interrupt handler

	pthread_create(&para->thread_user_id, NULL, (void*)sig_int_handler, (void*)para);


	// waiting for the child thread
	pthread_join(para->thread_packet_id, &result);
	pthread_join(para->thread_server1_id, &result);
	pthread_join(para->thread_server2_id, &result);
	pthread_join(para->thread_token_id, &result);
	pthread_join(para->thread_user_id, &result);
}
Para* initialize() {
	Para* para = malloc(sizeof(Para));

	if (!para) { exit_err("initialize malloc para failure.", para); }

	//initialize parameters to default values
	para->lambda = 1;
	para->lambda_user = 1;
	para->mu = 0.35;
	para->mu_user = 0.35;
	para->r = 1.5;
	para->r_user = 1.5;
	para->B = 10;
	para->P = 3;
	para->num = 20;
	para->is_trace_driven = FALSE;

	//pthread related part
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	para->mutex = mutex;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	para->Q2_no_empty_cond = cond;
	para->is_guard_Q2_empty = EMPTY;
	para->thread_server1_id = INVALID_THREAD_ID;
	para->thread_server2_id = INVALID_THREAD_ID;

	//list of packets
	para->list = init_list(para);
	para->list_Q1 = init_list(para);
	para->list_Q2 = init_list(para);
	para->num_completed_packets = 1;//for fear that ctrl+C occurs too early, and in statistics, the parameter is a zero denominator.
    para->is_Q1_having_packets = FALSE;
    para->is_all_packets_received = FALSE;
    para->batch_last_size = 0;
    para->batch_num = 0;
    para->batch_id = 0;

	//tokens
	para->token_remained = 0;
	para->token_id = 0;

	//statistic
	para->start_time.tv_sec = 0;
	para->start_time.tv_usec = 0;
	para->static_drop_packets = 0;
	para->static_drop_tokens = 0;
	para->server_state = BOTH_WORK; //0 and 1 both stand for it's over cos probably S1 and S2 separately process the last two packets

	return para;
}

void reset(void* trans_para) {
	My402ListElem* i_node = NULL;
	Para* para = trans_para;

	if (para != NULL) {
		if (para->list) {
			for (i_node = My402ListFirst(para->list); i_node != NULL; i_node = My402ListNext(para->list, i_node)) {
				free(i_node->obj);
				i_node->obj = NULL;
			}
			My402ListUnlinkAll(para->list);
			free(para->list);
			para->list = NULL;
		}
		if (para->list_Q1) {
			My402ListUnlinkAll(para->list_Q1);
			free(para->list_Q1);
			para->list_Q1 = NULL;
		}
		if (para->list_Q2) {
			My402ListUnlinkAll(para->list_Q2);
			free(para->list_Q2);
			para->list_Q2 = NULL;
		}
		if (para->batch_id > para->batch_num) {
			free(para);
			para = NULL;
		}
	}
	return;
}

void statistics(Para* para) {
	My402ListElem* i_node = NULL;
	Packet* cur_packet;
	double total_inter_arrival_ms = 0;
	double total_serve_ms = 0;
	double total_ms_in_Q1 = 0;
	double total_ms_in_Q2 = 0;
	double total_ms_in_S1 = 0;
	double total_ms_in_S2 = 0;
	double total_ms_in_system = 0;
	double total_square = 0;

	for (i_node = My402ListFirst(para->list); i_node != NULL; i_node = My402ListNext(para->list, i_node)) {
		cur_packet = (Packet*)(i_node->obj);
		total_inter_arrival_ms = total_inter_arrival_ms + cur_packet->ms_inter_arrival_time;
		total_serve_ms = total_serve_ms + cur_packet->ms_in_S1 + cur_packet->ms_in_S2;
		total_ms_in_Q1 = total_ms_in_Q1 + cur_packet->ms_in_Q1;
		total_ms_in_Q2 = total_ms_in_Q2 + cur_packet->ms_in_Q2;
		total_ms_in_S1 = total_ms_in_S1 + cur_packet->ms_in_S1;
		total_ms_in_S2 = total_ms_in_S2 + cur_packet->ms_in_S2;
		total_ms_in_system = total_ms_in_system + cur_packet->ms_in_system;

		total_square = total_square + cur_packet->ms_in_system * cur_packet->ms_in_system;
	}
	double deviation = total_square/para->num - (total_ms_in_system/para->num) * (total_ms_in_system/para->num);
	fprintf(stdout, "\nStatistics:\n");

	fprintf(stdout, "\n\taverage packet inter-arrival time = %.6gms\n", total_inter_arrival_ms/para->num);
	fprintf(stdout, "\n\taverage packet service time = %.6gms\n", total_serve_ms/para->num_completed_packets);

	fprintf(stdout, "\n\taverage number of packets in Q1 = %.6g\n", total_ms_in_Q1 / para->ms_total_emulation);
	fprintf(stdout, "\taverage number of packets in Q2 = %.6g\n", total_ms_in_Q2 / para->ms_total_emulation);
	fprintf(stdout, "\taverage number of packets in S1 = %.6g\n", total_ms_in_S1 / para->ms_total_emulation);
	fprintf(stdout, "\taverage number of packets in S2 = %.6g\n", total_ms_in_S2 / para->ms_total_emulation);

	fprintf(stdout, "\n\taverage time a packet spent in system = %.6gms\n", total_ms_in_system/para->num_completed_packets);
	fprintf(stdout, "\tstandard deviation for time spent in system = %.6g\n", sqrt(deviation));
    if (para->static_drop_tokens == 0) {
    	fprintf(stdout, "\n\ttoken drop probability = %.6g\n", (double)(0));
    } else {
    	fprintf(stdout, "\n\ttoken drop probability = %.6g\n", (double)para->static_drop_tokens / para->token_id);
    }
	fprintf(stdout, "\tpacket drop probability = %.6g\n", (double)para->static_drop_packets / para->num);
}

int main(int argc, char* argv[]) {
	Para* para = initialize();
	parse_command(argc, argv, para);
	process(para);
	statistics(para);
	reset(para);
	return TRUE;
}
