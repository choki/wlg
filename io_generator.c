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
#include <signal.h>
#include <unistd.h>
#include <sched.h>	//sched_affinity()
#include <pthread.h>
//#include <libaio.h>
#include "gio.h"
#include "common.h"


/* Static Functions */
static int select_op(unsigned long cur_file_size);
static SEQUENTIALITY_TYPE select_start_addr(unsigned long *start_addr, unsigned long prior_end_addr, int op, unsigned long cur_file_size);
static unsigned long select_size(unsigned long start_addr, int op, unsigned long cur_file_size);
static unsigned int get_rand_range(unsigned int min, unsigned int max);

/* static varialbes */
static wg_env *desc;


void *workload_generator(void *arg)
{
    int fd;
    int op;
    SEQUENTIALITY_TYPE seq_rnd;
    size_t ret;
    unsigned long start_addr;
    unsigned long size;
    unsigned long prior_end_addr = 0;
    unsigned int counter = 0;
    unsigned long i = 0;
    char *buf;
    struct timeval current_time, 
	posed_time, 
	start_time;
    //pthread_t tid;
    unsigned int tid = 0;
#ifdef ONLY_FOR_TEST
    int test_count=1;
#endif
    //io tracer related
    unsigned long long trace_time;
    char trace_line[MAX_STR_LEN] = {0};

    desc = (wg_env *)arg;
    if(desc->rand_deterministic == 0){
	srand(time(NULL));
    }
    
    //tid = pthread_self();
    pthread_mutex_lock(&thr_mutex);
    tid = shared_cnt++;
    pthread_mutex_unlock(&thr_mutex);

    if( (fd = open(desc->file_path, O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
    //if( (fd = open(desc->file_path, O_CREAT|O_RDWR, 0666)) == -1){
	PRINT("Error on opening the init_file of workload generator, file:%s, line:%d, fd=%d\n", __func__, __LINE__, fd);
	exit(1);
    }
    //aio_initialize(desc->max_queue_depth);
    mem_allocation(desc, &buf, (desc->max_size)*(desc->interface_unit));
    if (NULL == buf) {
	PRINT("Error on memory allocation, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }
    //Preparing for 100% READ_W, otherwise read operation will fail.
    if(desc->write_w == 0){
	lseek(fd, GET_ALIGNED_VALUE(desc->max_addr-512), SEEK_SET);
	fill_data(desc, buf, 512);
	ret = write(fd, buf , 512);
	if (ret != 512) {
	    PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
	    exit(1);
	}
	pthread_mutex_lock(&thr_mutex);
	if(desc->max_addr > max_written_size){
	    max_written_size = desc->max_addr;
	}
	pthread_mutex_unlock(&thr_mutex);
    }
    //usleep(50);
    get_current_time(&start_time);

    PRINT("\n");
    while (1) {
	op = select_op(max_written_size);
	seq_rnd = select_start_addr(&start_addr, prior_end_addr, op, max_written_size);
	
#ifdef ONLY_FOR_TEST
	//TODO for test
	/*if(test_count%4 == 1){
	    size *= 2;
	}
	test_count++;*/
#else
	size = select_size(start_addr, op , max_written_size);
#endif
	if(size == 0){
	    continue;
	}
	if( (desc->max_size == desc->min_size) && (size != desc->min_size) ){
	    continue;
	}
	fill_data(desc, buf, size);

	PRINT("\nGENERATOR tid:%u %s %s Addr:%12lu \t Size:%12lu\n", 
		tid,
		(seq_rnd==WG_SEQ?"SEQ":"RND"),
		(op==WG_READ?"READ ":"WRITE"), 
		start_addr, 
		size);

    	pthread_mutex_lock(&thr_mutex);
	get_current_time(&current_time);
	sprintf(trace_line, "%u,%li.%06li,%s,%s,%lu,%lu", 
		tid, current_time.tv_sec, current_time.tv_usec, (op==WG_READ?"R":"W"), "D", start_addr, size);
	tracer_add(trace_line);
	pthread_mutex_unlock(&thr_mutex);

	//ret = aio_enqueue(fd, buf, size, start_addr, op);

	switch (op){
	    case WG_READ:
		ret = pread(fd, buf , size, start_addr);
		break;
	    case WG_WRITE:
		ret = pwrite(fd, buf , size, start_addr);

		if(max_written_size < start_addr + size){
		    pthread_mutex_lock(&thr_mutex);
		    if(start_addr+size > max_written_size){
			max_written_size = start_addr + size;
		    }
		    PRINT("New max_written_size:%lu\n", max_written_size);
		    pthread_mutex_unlock(&thr_mutex);
		}
		//fsync(fd);
		break;
	    default:
		PRINT("Error on file:%s, line:%d\n", __func__, __LINE__);
		exit(1);
	}
	if(op == WG_WRITE){
	    if(max_written_size < start_addr + size){
		max_written_size = start_addr + size;
		PRINT("New max_written_size:%lu\n", max_written_size);
	    }
	}

	// For sequentiality control
	prior_end_addr = start_addr + size;

	if (ret != size) {
	//if (ret != 1) {
	    PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
	    break;
	}

	i++;
	counter++;

	if (desc->test_length_type == WG_TIME) {
	    get_current_time(&current_time);
	    if ( (TIME_VALUE(&current_time) - TIME_VALUE(&start_time)) >= \
		    MILLI_SECOND(desc->total_test_time) )
		//Go out!!
		break;
	} else if (desc->test_length_type == WG_NUMBER) {
	    if (i >= (unsigned int)desc->total_test_req) 
		//Go out!!
		break;
	} else {
	    PRINT("Error on file:%s, line:%d", __func__, __LINE__);
	}

	if (counter >= desc->burstiness_number) {
	    counter = 0;
	    get_current_time(&posed_time);

	    while (1) {
		get_current_time(&current_time);
		if ( (TIME_VALUE(&current_time) - TIME_VALUE(&posed_time)) >= \
			MILLI_SECOND((long long)desc->pose_time) )
		    break;
	    }
	}
    }
    pthread_mutex_destroy(&thr_mutex);
    close(fd);
    free(buf);
    PRINT("END OF GENERATOR\n");
}

/*  Functions for Workload Generation */
static int select_op(unsigned long cur_file_size)
{
    int selector;

    if(cur_file_size == 0){
	PRINT("First req must be write\n");
	return WG_WRITE;
    }

    selector = get_rand_range(0, desc->read_w + desc->write_w - 1);
    if(selector < desc->read_w)
	return WG_READ;
    else
	return WG_WRITE;
}

static SEQUENTIALITY_TYPE select_start_addr(unsigned long *start_addr, unsigned long prior_end_addr, int op, unsigned long cur_file_size)
{
    unsigned long selector;
    SEQUENTIALITY_TYPE seq_rnd;

    selector = get_rand_range(0, desc->sequential_w + desc->nonsequential_w - 1);

    //Sequential case
    if( (selector < desc->sequential_w) && (cur_file_size != 0) ){
	*start_addr = prior_end_addr;
	if(selector >= desc->max_addr){
	    *start_addr = desc->min_addr;
	}
	seq_rnd = WG_SEQ;
    }
    //Random case
    else{
	if(op == WG_READ){
	    *start_addr = get_rand_range(desc->min_addr, cur_file_size - 1);
	    //PRINT("Read rand: cur_file_size=%lu, start_addr=%lu\n", cur_file_size, *start_addr);
	}else{
	    *start_addr = get_rand_range(desc->min_addr, desc->max_addr - 1);
	}
	seq_rnd = WG_RND;
    }

    if( desc->alignment ){
	*start_addr = GET_ALIGNED_VALUE(*start_addr);
    }

    return seq_rnd;
}

static unsigned long select_size(unsigned long start_addr, int op, unsigned long cur_file_size)
{
    unsigned long selector;
    unsigned long aligned_selector;

    if(desc->min_size == desc->max_size){
	selector = desc->min_size;
    }else{
	selector = get_rand_range(desc->min_size, desc->max_size - 1);
    }

    if(op == WG_READ && start_addr+selector > cur_file_size){
	selector = cur_file_size - start_addr;
	//PRINT("Read re-size: cur_file_size=%lu, modified selector=%lu\n", cur_file_size, selector);
    }else if(start_addr + selector > desc->max_addr){
	selector = desc->max_addr - start_addr;
    }
    if(desc->alignment == 1){
	selector = GET_ALIGNED_VALUE(selector);
    }
    return selector;
}


static unsigned int get_rand_range(unsigned int min, unsigned int max)
{
    unsigned int range = max - min + 1;
    return (unsigned int)((rand() % range) + min);
}

	
