#ifndef _WARMUP2_H_
#define _WARMUP2_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include "my402list.h"

#define MAX_TSFILE_NAME_SIZE 1024
#define MAX_TSFILE_LINE_SIZE 1025 //1024 content including \n + 1byte \0
#define S2MS 1000 //sec to millisecond
#define MS2US 1000
#define S2US 1000000
#define INVALID_THREAD_ID 0
#define MAX_PACKET_LIST_Q_SIZE 1024 //more than it, will batch.

typedef struct tagObj {
	long int packet_id;
	double inter_arrival_ms; //requested time
	long int P;
	double serve_ms; //requested time
	struct timeval arrival_time;
	double ms_inter_arrival_time; //real time
	struct timeval start_in_Q1;
	double ms_in_Q1;
	struct timeval start_in_Q2;
	double ms_in_Q2;
	struct timeval start_by_S1;
	double ms_in_S1;//ms
	struct timeval start_by_S2;
	double ms_in_S2;//ms
	double ms_in_system;
} Packet;

enum ServerState {
	BOTH_STOP = 0,
	ONLY_ONE_WORK = 1,
	BOTH_WORK = 2
};

enum GuardQ2State {
	EMPTY = 0,
	CTRL_C_OCCUR = -2,
	ONLY_ONE_SRV_WORK = -1,
	//any positive number stands for the number of packets queued in Q2
};

enum FileOperateErr {
	NO_SUCH_FILE_OR_DIRECTORY = 2,
	PERMISSION_DENIED = 13
};
typedef struct tagParameters {
	double lambda; //packets per second --- rate of packets into Q1
	double lambda_user; //if 1/lambda > 10, change 1/lambda = 10 and save user input here.
	double mu; //packets per second --- server rate
	double mu_user; //if 1/mu > 10, change 1/mu = 10 and save user input here.
	double r; //tokens per second --- filter rate
	double r_user; //if 1/r > 10, change to 10s/token
	long int B; //#tokens of filter depth --- use long int cos the maximum is declared 32bit in 32bit system
	long int P; //#tokens that each packet is required
	long int num; //#packets of totally processed by the system
	int is_trace_driven; //drive the emulation using a trace specification file
	char tsfile_name[MAX_TSFILE_NAME_SIZE];
	My402List *list;
	My402List *list_Q1;
	My402List *list_Q2;
	struct timeval start_time; //s the time which the whole process start to run
	long int token_remained;
	long int token_id;
	long int static_drop_packets;
	long int static_drop_tokens;
	pthread_t thread_server1_id;
	pthread_t thread_server2_id;
	pthread_t thread_packet_id;
	pthread_t thread_token_id;
	pthread_t thread_user_id; //to catch Ctrl+C
	sigset_t set;
	pthread_mutex_t mutex;
	pthread_cond_t Q2_no_empty_cond;
	enum GuardQ2State is_guard_Q2_empty;
	int is_Q1_having_packets;
	int is_all_packets_received;
	enum ServerState server_state;
	long int num_completed_packets;//differ to num when ctrl+C happens
	double ms_total_emulation;
	long int batch_num;
	long int batch_last_size;
	long int batch_id;
} Para;

extern void reset(void* trans_para);
extern void exit_err(char* err, Para* para);
extern My402List* init_list(Para* para);
extern void* packet_arrival_handler(Para* para);
extern void* token_depositing_hander();
extern void* server_handler();
extern void* sigint_handler();
extern void* sig_int_handler(Para* para);
extern void get_batch_packets(Para* para);
extern void statistics(Para* para);

#endif //_WARMUP2_H_
