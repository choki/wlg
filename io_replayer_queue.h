#ifndef __IO_REPLAYER_QUEUE_H__
#define __IO_REPLAYER_QUEUE_H__

#include "common.h"

typedef struct _req_node{
    readLine  		req;
    struct _req_node	*next;
} req_node;

typedef struct _req_queue{
   req_node 		*head;
   req_node 		*tail;
   unsigned int 	num_node;
} req_queue;


void 	init_queue(void);
void 	terminate_queue(void);
void 	en_queue(readLine r);
readLine de_queue(void);
void 	set_queue_status(int value);
int 	get_queue_status(void);
void 	set_start_time(double sTime);
void 	get_start_time(long long *trace_sTime, long long *gio_sTime);
void 	print_queue(void);

#endif //__IO_REPLAYER_QUEUE_H__
