#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>	//exit
#include <stdint.h> 	//uint64_t
#include <string.h>
#include "io_replayer_queue.h"
#include "gio.h"

req_queue *r_queue;
static int file_read_finished = 0;
static long long trace_start_time = -1;
static long long  gio_start_time;


void init_queue(unsigned int total_thread_num)
{
    int i;

    r_queue = (req_queue *)malloc(sizeof(req_queue) * total_thread_num);
    if(r_queue == NULL){
	PRINT("Error on malloc, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }
    for(i=0; i<total_thread_num; i++){
	r_queue->head = NULL;
	r_queue->tail = NULL;
	r_queue->num_node = 0;
    }
}

void terminate_queue(void){
    free(r_queue);
}

void en_queue(int thread_id, readLine r)
{
    pthread_mutex_lock(&thr_mutex);
    req_queue *tmp_queue = &r_queue[thread_id];
    req_node *new_node;
    
    new_node = (req_node *)malloc(sizeof(req_node));


    if(new_node == NULL){
	PRINT("Error on malloc, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }
    new_node->req = r;
    new_node->next = NULL;

    //TODO lock??
    if(tmp_queue->num_node == 0){
	tmp_queue->head = new_node;
	tmp_queue->tail = new_node;
    }else{
	tmp_queue->head->next = new_node;
	tmp_queue->head = new_node;
    }
    tmp_queue->num_node++;
    pthread_mutex_unlock(&thr_mutex);
}

readLine de_queue(int thread_id)
{
    pthread_mutex_lock(&thr_mutex);
    req_queue *tmp_queue = &r_queue[thread_id];
    req_node  *tmp_node;
    readLine ret_req;

    if(tmp_queue->tail == NULL){
	//PRINT("EmptyQ ");
	strcpy(ret_req.rwbs, "EMPTY");
    }else{
	ret_req = tmp_queue->tail->req;
	tmp_node = tmp_queue->tail;
	tmp_queue->tail = tmp_queue->tail->next;
	free(tmp_node);
	tmp_queue->num_node--;
    }
    pthread_mutex_unlock(&thr_mutex);
    return ret_req;
}

void set_queue_status(int value)
{
    file_read_finished = value;
}

int get_queue_status(int thread_id)
{
    //"Feeder is done" && "Queue is empty" == No more job
    if(file_read_finished && (r_queue[thread_id].num_node == 0))
	return 1;
    else
	return 0;
}

void set_start_time(double sTime)
{
    struct timeval now;
    //Trace time
    trace_start_time = MILLI_SECOND(sTime);
    //Replayer time
    get_current_time(&now);
    gio_start_time = TIME_VALUE(&now);

    PRINT("trace start time:%lli ms  replayer start time:%lli ms\n",
	    trace_start_time, gio_start_time);
}

void get_start_time(long long *trace_sTime, long long *gio_sTime)
{
    *trace_sTime = trace_start_time;
    *gio_sTime   = gio_start_time;
}

void print_queue(int thread_id)
{
    unsigned int cnt = 0;
    req_node *node = r_queue[thread_id].tail;
    
    PRINT("Print queue#%d, total node#:%u\n", thread_id, r_queue[thread_id].num_node);
    while(node != NULL){
	PRINT("node#%d  cpu:%d, sTime:%lf, rwbs:%s, action:%s, sSector:%ld, size:%d\n",
		cnt,
		node->req.cpu,
		node->req.sTime,
		node->req.rwbs,
		node->req.action,
		node->req.sSector,
		node->req.size);
	node = node->next;
	cnt++;
    }
}





