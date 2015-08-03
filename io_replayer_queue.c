#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>	//exit
#include <stdint.h> 	//uint64_t
#include "io_replayer_queue.h"

req_queue *r_queue;

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
    req_queue *tmp_queue = &r_queue[thread_id];
    req_node *new_node;
    //TODO free
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
}

readLine de_queue(int thread_id)
{
    //TODO check free properly
    req_queue *tmp_queue = &r_queue[thread_id];
    readLine del_req;

    if(tmp_queue->tail == NULL){
	PRINT("Queue is empty\n");
    }else{
	del_req = tmp_queue->tail->req;
	tmp_queue->tail = tmp_queue->tail->next;
	free(tmp_queue->tail);
	tmp_queue->num_node--;
    }
    return del_req;
}

void print_queue(int thread_id)
{
    unsigned int cnt = 0;
    req_node *node = r_queue[thread_id].tail;
    
    printf("Print queue#%d, total node#:%u\n", thread_id, r_queue[thread_id].num_node);
    while(node != NULL){
	printf("node#%d  cpu:%d, sTime:%lf, rwbs:%s, action:%s, sSector:%ld, size:%d\n",
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





