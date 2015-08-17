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

/* 
 *	[ Replayable trace format ]
 *
 *      Thread ID : 
 *      	This value is only for checking purpose to identify thread ID produced the request.
 *      	No meaning in "replay mode" since it runs in single thread.
 *      Start time :
 *      	The time request is sent.
 *      RWBS :
 *      	This value should have a letter either of "R" or "W" in the string.
 *      Action :
 *      	Refer to TRACE ACTION of "blkparse"'s man page.
 *      	This value should be "D", otherwise replayer skip the line.
 *      Start sector :
 *      	Start LBA.
 *      Size :
 *      	Request size.
 *
 *
 *	[ How to make/get trace ]
 *
 *	Option#1. Manually make own trace that comply with "replayable format".
 *      Option#2. Get log from I/O generator
 *      Option#3. Get log from blktrace
 *			$ sudo blktrace -d DEV_PATH -a complete -a issue -o LOG_NAME_1
 *			$ sudo blkparse -i LOG_NAME_1.blktrace.0 -f "%2c,%T.%t,%d,%a,%S,%n\n" -o LOG_NAME_2
 *		    OR just run script file
 *			$ sudo ./blktrace.sh
 */

/* static varialbes */
static wg_env *desc;

/* Static Functions */
static unsigned long select_start_addr(unsigned long org_addr);
static unsigned long select_size(unsigned long org_size);


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
    if( (fd = open(desc->file_path, O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1 ){
    //if( (fd = open(desc->file_path, O_CREAT|O_RDWR, 0666)) == -1){
	PRINT("Error on opening the init_file of workload generator, file:%s, line:%d, fd=%d\n", __func__, __LINE__, fd);
	exit(1);
    }

#if !defined(BLOCKING_IO)
    aio_initialize(desc->max_queue_depth);
#endif

    mem_allocation(desc, &buf, REPLAYER_MAX_FILE_SIZE);
    if (NULL == buf) {
	PRINT("Error on memory allocation, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }

    while(1){
	//Dequeueing
	req = de_queue();
	//If a request is successfully dequeued.
	if(strcmp(req.rwbs, "EMPTY") != 0){
	    mSize = select_size(req.size);
	    mAddr = select_start_addr(req.sSector);
	    fill_data(desc, buf, mSize);

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
	    op = strstr(req.rwbs,"R")!=NULL? WG_READ : WG_WRITE;
#if defined(BLOCKING_IO)
	    if(op == WG_READ){
		//PRINT("R\n");
		ret = pread(fd, buf , (size_t)mSize, mAddr);
	    }else{
		//PRINT("W\n");
		ret = pwrite(fd, buf , (size_t)mSize, mAddr);
		fsync(fd);
	    }
	    if (ret != mSize) {
		PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
		break;
	    }
#else
    	    ret = aio_enqueue(fd, buf, mSize, mAddr, op);
	    if (ret != 1) {
		PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
		break;
	    }
#endif //BLOCKING_IO
#if 0
	    PRINT("DEQUEUED REQ tid:%u sTime:%lf rwbs:%s Addr:%12li \t Size:%12lu\n\n", 
		    tid,
		    req.sTime,
		    req.rwbs,
		    mAddr,
		    mSize);	
#endif
		   
	}

	if( get_queue_status() == 1 ){
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


	



