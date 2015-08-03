#ifndef __IO_REPLAYER_QUEUE_H__
#define __IO_REPLAYER_QUEUE_H__


typedef struct _request{
    double 	sTime;
    char 	rwbs[4];
    long 	sSector;
    int 	size;
} request;

typedef struct _req_node{
    request  		req;
    struct _req_node	*next;
} req_node;

typedef struct _req_queue{
   req_node *head;
   req_node *tail;
   unsigned int num_node;
} req_queue;


void init_queue(unsigned int thread_num);
void terminate_queue(unsigned int thread_num);
void en_queue(int thread_id, request r);
request de_queue(int thread_id);

#endif //__IO_REPLAYER_QUEUE_H__
