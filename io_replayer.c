
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
#include "gio.h"
#include "io_replayer.h"
#include "io_replayer_queue.h"
#include "common.h"

void *workload_replayer(void *arg)
{
    unsigned int tid = 0;
    readLine req;

    pthread_mutex_lock(&thr_mutex);
    tid = shared_cnt++;
    pthread_mutex_unlock(&thr_mutex);

    while(1){
	req = de_queue(tid);
	if(strcmp(req.rwbs, "EMPTY") != 0){
	    PRINT("REPLAYER tid:%u %s Addr:%12li \t Size:%12d\n", 
		    tid,
		    req.rwbs,
		    req.sSector,
		    req.size);	
	}
	if( get_queue_status(tid) == 1 ){
	    break;
	}
    }
    pthread_mutex_destroy(&thr_mutex);
}
