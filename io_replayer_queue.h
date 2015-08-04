#ifndef __IO_REPLAYER_QUEUE_H__
#define __IO_REPLAYER_QUEUE_H__

#include "common.h"

typedef struct _req_node{
    readLine  		req;
    struct _req_node	*next;
} req_node;

typedef struct _req_queue{
   req_node *head;
   req_node *tail;
   unsigned int num_node;
} req_queue;


void init_queue(unsigned int total_thread_num);
void terminate_queue(void);
void en_queue(int thread_id, readLine r);
readLine de_queue(int thread_id);
void set_queue_status(int value);
int get_queue_status(int thread_id);
void print_queue(int thread_id);

#endif //__IO_REPLAYER_QUEUE_H__
