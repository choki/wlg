#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>	//exit
#include <stdint.h> 	//uint64_t
#include <errno.h>
#include <string.h> 	//memcpy
#include <stdbool.h>	//boolean
#include <time.h>	//time()
#include <unistd.h>
#include <sched.h>	//sched_affinity()
#include <pthread.h>
#include <libaio.h>
#include "gio.h"
#include "common.h"
#include "io_aio.h"

static 			pthread_t thr;
static unsigned long 	status_mask = 0;		//Status mask indicating which iocb is used
static int 		next_id = 0;		//Queue id that will be used in next turn
static my_iocb 		my_iocbp[MAX_QUEUE_DEPTH]={0};
static unsigned int 	max_depth;			//User setting max queue depth
static io_context_t 	context;
static 			req_num = 0;			//To check request processing is done or not

/* Static funstions */
static void *	aio_dequeue(void *arg);
static int 	find_and_set_id(void);
static void 	clear_id(int id);

pthread_mutex_t aio_status_mask_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t aio_req_num_mutex = PTHREAD_MUTEX_INITIALIZER;

int aio_initialize(unsigned int max_queue_depth)
{
    int rtn;
    max_depth = max_queue_depth;
    int i;

    rtn = io_setup(125, &context);
    if(rtn<0){
	PRINT("Error on setup I/O, file:%s, line:%d, errno=%d\n", __func__, __LINE__, rtn);
	exit(1);
    }
    rtn = pthread_create(&thr, NULL, &aio_dequeue, &context);
    if(rtn<0){
	PRINT("Error on thread creation, line:%d, errno:%d\n", __LINE__, rtn);
	exit(1);
    }
}

int aio_enqueue(
	int fd, 
	char *buf, 
	unsigned long size, 
	unsigned long start_addr, 
	OPERATION_TYPE op
	)
{
    int rtn;
    int queue_id;
    struct iocb *cb;
    int i;


retry:
    queue_id = find_and_set_id();
    if(queue_id == -1){
	PRINT("Qbusy ");
	usleep(10);
	goto retry;
    }
    my_iocbp[queue_id].qid = queue_id;
    cb = &my_iocbp[queue_id].iocbp;

    switch (op){
	case WG_READ:
	    io_prep_pread(cb, fd, buf, size, start_addr);
	    break;
	case WG_WRITE:
	    io_prep_pwrite(cb, fd, buf, size, start_addr);
	    break;
	default:
	    PRINT("Error not supported I/O operation file:%s, line:%d\n", __func__, __LINE__);
	    exit(1);
    }
    rtn = io_submit(context, 1, &cb);
    if(rtn<0){
	PRINT("Error on I/O submission, line:%d, errno:%d\n", __LINE__, rtn);
	exit(1);
    }
    pthread_mutex_lock(&aio_req_num_mutex);
    req_num++;
    pthread_mutex_unlock(&aio_req_num_mutex);
    return rtn;
}

void aio_termination(void)
{
    int rtn;

    rtn = io_destroy(context);
    if(rtn<0){
	PRINT("Error on io_destory:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }
}

int get_aio_status(void)
{
    return req_num;
}

/* Static functions */
static void *aio_dequeue(void *arg)
{
    struct io_event event;
    int rtn;
    int resp;
    my_iocb *cbp;

    while(1){
	resp = io_getevents(context, 1, 1, &event, NULL);
	//PRINT("io_getevent resp:%d\n", resp);
	if(resp <= 0){
	    if(resp == 0){
		continue;
	    }else if(resp == EINTR){
		PRINT("EINTR recieved \n");
		continue;
	    }
	    PRINT("Error I/O getevent file:%s, line:%d errno:%d\n", __func__, __LINE__, resp);
	    //exit(1);
	}
	else{
	    cbp = (my_iocb *)event.obj;
	    PRINT("\tFinished qid:%d\n", cbp->qid);
	    clear_id(cbp->qid);
	    pthread_mutex_lock(&aio_req_num_mutex);
	    req_num--;
	    pthread_mutex_unlock(&aio_req_num_mutex);
	}
    }
}

static int find_and_set_id(void)
{
    int i;
    char bit;
    int chk_bit;

    pthread_mutex_lock(&aio_status_mask_mutex);
    for(i=0; i<max_depth; i++){
	chk_bit = next_id +i;
	if(chk_bit >= max_depth){
	    chk_bit %= max_depth;
	}
	//PRINT("i:%d, chk_bit:%d\n", i, chk_bit);
	bit = (status_mask&(1<<chk_bit))?1:0;
	if(bit == 1){
	    continue;
	}
	else{
	    status_mask|=(1<<chk_bit);
	    //PRINT("SET status_mask:%08lx, id:%d\n", status_mask, chk_bit);
	    next_id = chk_bit +1;
	    if(next_id >= max_depth){
		next_id %= max_depth;
	    }
	    //PRINT("Next id=%d\n", last_used_id);
	    pthread_mutex_unlock(&aio_status_mask_mutex);
	    return chk_bit;
	}
    }
    //All queues are in busy.
    //PRINT("Fail to find empty id\n");
    pthread_mutex_unlock(&aio_status_mask_mutex);
    return -1;
}

static void clear_id(int id)
{
    pthread_mutex_lock(&aio_status_mask_mutex);
    status_mask &= ~(1<<id);
    //PRINT("CLEAR status_mask:%08lx, id:%d\n", status_mask, id);
    pthread_mutex_unlock(&aio_status_mask_mutex);
}
