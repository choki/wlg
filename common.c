#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> //getline()
#include <fcntl.h>
#include <string.h> //strtok()
#include <sys/time.h>	//gettimeofday
#include "trace_parser.h"
#include "common.h"  

void parse_one_line(char *line, readLine *string)
{
    char *tmp = NULL;
    char *ptr;
    int cnt = 0;


    //printf("%s", line);
    //line changes during strtok.....;;;;
    tmp = (char *)malloc(sizeof(char)*MAX_STR_LEN);
    strncpy(tmp, line, strlen(line)+1);

    ptr = strtok(tmp, ",");
    string->thr_id = atoi(ptr);
    cnt++;
    while(ptr != NULL){
	//printf("%s ",ptr);
	ptr = strtok(NULL, ",");

	switch(cnt){
	    case 1:
		string->sTime = atof(ptr);
		break;
	    case 2:
		strncpy(string->rwbs, ptr, strlen(ptr)+1);
		break;
	    case 3:
		strncpy(string->action, ptr, strlen(ptr)+1);
		break;
	    case 4:
		string->sSector = atol(ptr);
		break;
	    case 5:
		string->size = atoi(ptr);
		break;
	    default:
		break;
	}
	cnt++;
    }
    cnt = 0;

    /*printf("STRING  cpu:%d, sTime:%lf, rwbs:%s, action:%s, sSector:%ld, size:%d\n",
      string->cpu,
      string->sTime,
      string->rwbs,
      string->action,
      string->sSector,
      string->size);*/
    free(tmp);
}

void fill_data(wg_env *desc, char *buf, unsigned int size)
{
    if( memset(buf ,rand() ,size * desc->interface_unit) == NULL){
	PRINT("Error on workload data setup, file:%s, line:%d\n", __func__, __LINE__);
    }
}

int mem_allocation(wg_env *desc, char **buf, int reqSize)
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

void get_current_time(struct timeval *now)
{
    struct timespec ts;

    /* Temporary commented due to compile error in old kernel version
    if(clock_gettime(1, &ts) < 0){
	now->tv_sec = ts.tv_sec;
	now->tv_usec = ts.tv_nsec/1000;
    }else{
    */
    	gettimeofday(now, NULL);
    //}
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

unsigned long long utime_calculator(struct timeval *s, struct timeval *e)
{
 	long sec, usec;
    	
	//PRINT("start sec:%ld, usec:%ld\n", s->tv_sec, s->tv_usec);
 	//PRINT("end sec:%ld, usec:%ld\n", e->tv_sec, e->tv_usec);
	sec = e->tv_sec - s->tv_sec;
	usec = e->tv_usec - s->tv_usec;
	if(sec < 0){
	    PRINT("%s : unbelievable thing happended, time warp\n", __func__);
	    exit(1);
	}
	if(usec < 0){
	    sec--;
	    usec += 1000000;
	}
	return sec*1000000ULL + usec;
}
