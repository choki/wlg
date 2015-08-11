#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>	//exit
#include <sys/time.h>	//gettimeofday
#include <stdint.h> 	//uint64_t
#include <errno.h>
#include <string.h> 	//memcpy
#include <stdbool.h>	//boolean
#include <time.h>	//time()
#include <signal.h>
#include <unistd.h>
#include <sched.h>	//sched_affinity()
#include <libaio.h>
#include "gio.h"
#include "io_replayer.h"
#include "io_replayer_queue.h"
#include "common.h"
#include "io_aio.h"

/* static varialbes */
static wg_env *desc;

/* Static Functions */
static unsigned long select_start_addr(unsigned long org_addr);
static unsigned long select_size(unsigned long org_size);
static void fill_data(char *buf, unsigned int size);
static int mem_allocation(char **buf, int reqSize);
void usec_sleep(long long usec);
long long usec_elapsed(struct timeval start);


void *workload_replayer(void *arg)
{
    unsigned int tid = 0;
    readLine req;
    long long trace_sTime = -1;
    long long trace_wTime = 0;
    long long gio_sTime   = 0;
    long long gio_wTime   = 0;
    struct timeval now;
    int fd;
    char *buf;
    size_t ret;
    unsigned long mSize;
    unsigned long mAddr;
    OPERATION_TYPE op;


    desc = (wg_env *)arg;

    pthread_mutex_lock(&thr_mutex);
    tid = shared_cnt++;
    pthread_mutex_unlock(&thr_mutex);

    //if( (fd = open(desc->file_path, O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
    if( (fd = open(desc->file_path, O_CREAT|O_RDWR, 0666)) == -1){
	PRINT("Error on opening the init_file of workload generator, file:%s, line:%d, fd=%d\n", __func__, __LINE__, fd);
	exit(1);
    }

#if !defined(BLOCKING_IO)
    aio_initialize(desc->max_queue_depth);
#endif

    mem_allocation( &buf, REPLAYER_MAX_FILE_SIZE );
    if (NULL == buf) {
	PRINT("Error on memory allocation, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }

    while(1){
	//Dequeueing
	req = de_queue(tid);
	
	//If a request is successfully dequeued.
	if(strcmp(req.rwbs, "EMPTY") != 0){

	    mSize = select_size(req.size);
	    mAddr = select_start_addr(req.sSector);
	    fill_data(buf, mSize);

	    //Calculate wait time based on trace log
	    if(trace_sTime == -1){
		get_start_time(&trace_sTime, &gio_sTime); 
	    }
	    trace_wTime = MICRO_SECOND(req.sTime) - trace_sTime;
	    if(trace_wTime < 0){
		PRINT("trace_wTime : %lli\n", trace_wTime);
		PRINT("Issue time must be ordered. Check the issue time in trace file");
		exit(1);
	    }

	    //Caculate how much time should wait
	    get_current_time(&now);
	    gio_wTime = TIME_VALUE(&now) - gio_sTime;
	    PRINT("TIMEDEBUG trace_wTime:%10lli  \tgio_wTime:%10lli\n",
		    trace_wTime, gio_wTime);

	    //If we need to wait
	    if(trace_wTime > gio_wTime){
		PRINT("WAITING ....%lli us\n", trace_wTime - gio_wTime);
		usec_sleep(trace_wTime - gio_wTime);
	    }
#if defined(BLOCKING_IO)
	    //Do request operation
	    if(strstr(req.rwbs, "R") != NULL){
		//PRINT("R\n");
		lseek(fd, mAddr, SEEK_SET);
		ret = read(fd, buf , (size_t)mSize);
	    }else{
		//PRINT("W\n");
		lseek(fd, mAddr, SEEK_SET);
		ret = write(fd, buf , (size_t)mSize);
		fsync(fd);
	    }
	    if (ret != mSize) {*/
		PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
		break;
	    }
#else
	    op = strstr(req.rwbs,"R")!=NULL? WG_READ : WG_WRITE;

    	    ret = aio_enqueue(fd, buf, mSize, mAddr, op);
	    if (ret != 1) {
		PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
		break;
	    }
#endif //BLOCKING_IO
	    PRINT("DEQUEUED REQ tid:%u sTime:%lf rwbs:%s Addr:%12li \t Size:%12lu\n\n", 
		    tid,
		    req.sTime,
		    req.rwbs,
		    mAddr,
		    mSize);	
    }

	if( get_queue_status(tid) == 1 ){
	    PRINT("END OF REPLAYER\n");
	    break;
	}
    }
    pthread_mutex_destroy(&thr_mutex);
}

static unsigned long select_start_addr(unsigned long org_addr)
{
    unsigned long modify_addr = org_addr;

    if(desc->alignment == 1 && org_addr%desc->alignment_unit!=0){
	modify_addr = GET_ALIGNED_VALUE(org_addr);
	PRINT("Start address changed: from:%lu to:%lu\n",org_addr, modify_addr);
    }
    return modify_addr;
}

static unsigned long select_size(unsigned long org_size)
{
    unsigned long modify_size = org_size;

    if(desc->alignment == 1 && org_size%desc->alignment_unit!=0){
	modify_size = GET_ALIGNED_VALUE(org_size);
	//the smallest size is 512Byte
	if(modify_size == 0)
	    modify_size = 512;
	PRINT("Size changed: from:%lu to:%lu\n",org_size, modify_size);
    }
    return modify_size;
}

static void fill_data(char *buf, unsigned int size)
{
    if( memset(buf ,rand() ,size * desc->interface_unit) == NULL){
	PRINT("Error on workload data setup, file:%s, line:%d\n", __func__, __LINE__);
    }
}

static int mem_allocation(char **buf, int reqSize)
{
	int alignedReqSize;
	int i;
	
	if(desc->alignment){
	    alignedReqSize = GET_ALIGNED_VALUE(reqSize);
	    PRINT("%s : reqSize:%d, alignedReqSize:%d, align:%d\n",
		    __func__, reqSize, alignedReqSize, desc->alignment);
	    if( posix_memalign((void **)buf, SIZE_OF_SECTOR, alignedReqSize) != 0){
		PRINT("%s : buffer allocation failed\n", __func__);
		exit(1);
	    }
	}else{
	    if(	(*buf = (char *)malloc(reqSize)) == NULL ){
		PRINT("%s : buffer allocation failed\n", __func__);
		exit(1);
	    }
	}
	if(desc->alignment)
	    return alignedReqSize;
	else
	    return reqSize;
}

void usec_sleep(long long usec)
{
    struct timeval now;

    get_current_time(&now);
    while(usec_elapsed(now) < usec){
	NOP;
    }
}

long long usec_elapsed(struct timeval start)
{
    struct timeval end;
    get_current_time(&end);
    return TIME_VALUE(&end) - TIME_VALUE(&start);
}
