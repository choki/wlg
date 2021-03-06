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
#include "gio.h"
#include "common.h"

//#define ONLY_FOR_TEST

/* Static Functions */
static int select_op(void);
static SEQUENTIALITY_TYPE select_start_addr(unsigned long *start_addr, long prior_end_addr, int op);
static unsigned long select_size(unsigned long start_addr, int op);
static unsigned int get_rand_range(unsigned int min, unsigned int max);
static void first_run_checker(int *first_run, int tid);
static void make_initial_file(int fd);

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
    unsigned int burst_cnt = 0;
    unsigned long req_cnt = 0;
    char *buf;
    struct timeval current_time, 
	posed_time, 
	start_time,
    	end_time;
    unsigned long long timediff = 0;
    unsigned long total_byte = 0;
    unsigned int tid = 0;
    int first_run = 0;
    char comp_1[VERIFY_STR_LEN] = {0};
    char comp_2[VERIFY_STR_LEN] = {0};
    //io tracer related
    unsigned long long trace_time;
    char trace_line[MAX_STR_LEN] = {0};

    desc = (wg_env *)arg;
    if(desc->rand_deterministic == 0){
	srand(time(NULL));
    }
    
    pthread_mutex_lock(&thr_mutex);
    tid = shared_cnt++;
    pthread_mutex_unlock(&thr_mutex);

    if( (fd = open(desc->file_path, O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
    //if( (fd = open(desc->file_path, O_CREAT|O_RDWR, 0666)) == -1){
	PRINT("Error on opening the init_file of workload generator, file:%s, line:%d, fd=%d\n", __func__, __LINE__, fd);
	exit(1);
    }

    //Check if we need to make initial file.
    pthread_mutex_lock(&thr_mutex);
    first_run_checker(&first_run, tid);
    if(first_run == 1){
    	//Initial file sequantial write over the "MIN_ADDRESS~MAX_ADDRESS" range
	make_initial_file(fd);
    }
    pthread_mutex_unlock(&thr_mutex);
    
    mem_allocation(desc, &buf, (desc->max_size)*(desc->interface_unit));
    if (NULL == buf) {
	PRINT("Error on memory allocation, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }
    usleep(1000);
#ifdef ONLY_FOR_TEST
    size = 512;
#endif
    get_current_time(&start_time);
    PRINT("\n");

    while (1) {
	op = select_op();
	// More likely fail to generate sequential requests as thread number are getting bigger,
	//  because of scheduling between the threads. 
	// Neverthless keep try to make sequential requests by using lock.
	pthread_mutex_lock(&thr_mutex);
	seq_rnd = select_start_addr(&start_addr, prior_end_addr, op);
#ifdef ONLY_FOR_TEST
	if(req_cnt%200 == 0){
	    size *= 2;
	}
#else
	size = select_size(start_addr, op);
#endif
	if(size == 0){
	    pthread_mutex_unlock(&thr_mutex);
	    continue;
	}
#if !defined(ONLY_FOR_TEST)
	if( (desc->max_size == desc->min_size) && (size != desc->min_size) ){
	    pthread_mutex_unlock(&thr_mutex);
	    continue;
	}
#endif
	// For sequentiality control
	prior_end_addr = start_addr + size;
	pthread_mutex_unlock(&thr_mutex);
	
	fill_data(desc, buf, size);

	PRINT("\nGENERATOR tid:%u %s %s Addr:%12lu \t Size:%12lu\n", 
		tid,
		(seq_rnd==WG_SEQ?"SEQ":"RND"),
		(op==WG_READ?"READ ":"WRITE"), 
		start_addr, 
		size);
	total_byte += size;

	if(desc->test_mode == WG_GENERATING_MODE){
	    pthread_mutex_lock(&thr_mutex);
	    get_current_time(&current_time);
	    sprintf(trace_line, "%u,%li.%06li,%s,%s,%lu,%lu", 
		    tid, current_time.tv_sec, current_time.tv_usec, (op==WG_READ?"R":"W"), "D", start_addr, size);
	    tracer_add(trace_line);
	    pthread_mutex_unlock(&thr_mutex);
	}

	switch (op){
	    case WG_READ:
		ret = pread(fd, buf , size, start_addr);
		
		if(desc->test_mode == WG_VERIFY_MODE){
		    pread(fd, buf , size, start_addr);
		    strncpy(comp_1, buf, sizeof(char)*VERIFY_STR_LEN);
		    comp_1[VERIFY_STR_LEN] = '\0';
		    PRINT("R COMP : %s\n", comp_1);
		}
		break;
	    case WG_WRITE:
		ret = pwrite(fd, buf , size, start_addr);
		//fsync(fd);
		
		if(desc->test_mode == WG_VERIFY_MODE){
		    strncpy(comp_1, buf, sizeof(char)*VERIFY_STR_LEN);
		    comp_1[VERIFY_STR_LEN] = '\0';
		    PRINT("W COMP_1 : %s", comp_1);
		    pread(fd, buf , size, start_addr);
		    strncpy(comp_2, buf, sizeof(char)*VERIFY_STR_LEN);
		    comp_2[VERIFY_STR_LEN] = '\0';
		    PRINT("\t W COMP_2 : %s\n", comp_2);
		}
		break;
	    default:
		PRINT("Error on file:%s, line:%d\n", __func__, __LINE__);
		exit(1);
	}
	if (ret != size) {
	    PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
	    break;
	}
	req_cnt++;
	burst_cnt++;

	if (desc->test_length_type == WG_TIME) {
	    get_current_time(&current_time);
	    if ( (TIME_VALUE(&current_time) - TIME_VALUE(&start_time)) >= \
		    MILLI_SECOND(desc->total_test_time) )
		//Go out!!
		break;
	} else if (desc->test_length_type == WG_NUMBER) {
	    if (req_cnt >= (unsigned int)desc->total_test_req/desc->thread_num) 
		//Go out!!
		break;
	} else {
	    PRINT("Error on file:%s, line:%d", __func__, __LINE__);
	}

	if (burst_cnt >= desc->burstiness_number) {
	    burst_cnt = 0;
	    get_current_time(&posed_time);

	    while (1) {
		get_current_time(&current_time);
		if ( (TIME_VALUE(&current_time) - TIME_VALUE(&posed_time)) >= \
			MILLI_SECOND((long long)desc->pose_time) )
		    break;
	    }
	}
    }
    get_current_time(&end_time);
    timediff = utime_calculator(&start_time, &end_time);
    PRINT("\n");
    PRINT("* [%u] THROUGHPUT:%lluKB/sec | Run Time:%llu.%llu | Total Bytes:%lu **\n", 
	    tid, total_byte/(timediff/1000ULL), timediff/1000000ULL, timediff%1000000ULL, total_byte); 
    close(fd);
    free(buf);
    PRINT("END OF GENERATOR\n");
}

/*  Functions for Workload Generation */
static int select_op(void)
{
    int selector;

    selector = get_rand_range(0, desc->read_w + desc->write_w - 1);
    if(selector < desc->read_w)
	return WG_READ;
    else
	return WG_WRITE;
}

static SEQUENTIALITY_TYPE select_start_addr(unsigned long *start_addr, long prior_end_addr, int op)
{
    unsigned long selector;
    SEQUENTIALITY_TYPE seq_rnd;

    if(desc->sequential_mode == WG_COUNT){
	if( (desc->sequential_c > cur_seq_cnt) && (prior_end_addr != -1) ){
	    seq_rnd = WG_SEQ;
	    cur_seq_cnt++;
	}else{
	    seq_rnd = WG_RND;
	    cur_seq_cnt = 1;
	}
    }else{
	selector = get_rand_range(0, desc->sequential_w + desc->nonsequential_w - 1);
	if( (selector < desc->sequential_w) && (prior_end_addr != -1) ){
	    seq_rnd = WG_SEQ;
	}else{
	    seq_rnd = WG_RND;
	}
    }

    if(seq_rnd == WG_SEQ){
	*start_addr = prior_end_addr;
	if(*start_addr >= desc->max_addr){
	    *start_addr = desc->min_addr;
	}
    }else{
	*start_addr = get_rand_range(desc->min_addr, desc->max_addr - 1);
    }
    if( desc->alignment ){
	*start_addr = GET_ALIGNED_VALUE(*start_addr);
    }
    return seq_rnd;
}

static unsigned long select_size(unsigned long start_addr, int op)
{
    unsigned long selector;
    unsigned long aligned_selector;

    if(desc->min_size == desc->max_size){
	selector = desc->min_size;
    }else{
	selector = get_rand_range(desc->min_size, desc->max_size - 1);
    }

    if(start_addr + selector > desc->max_addr){
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

static void first_run_checker(int *first_run, int tid)
{
    struct stat stat_buf;
    int fd_check;

    if( stat(FIRST_RUN_CHECK_FILE_NAME, &stat_buf) != 0){
	PRINT("%s file is not exist\n",FIRST_RUN_CHECK_FILE_NAME);
	PRINT("This is first RUN, tid=%d\n", tid);
	if( (fd_check = open(FIRST_RUN_CHECK_FILE_NAME, O_CREAT|O_RDONLY, 0666)) == -1){
	    PRINT("Error on opening the init_file of workload generator, file:%s, line:%d, fd_check=%d\n", __func__, __LINE__, fd_check);
	    exit(1);
	}
	*first_run = 1;
	close(fd_check);
    }else{
	*first_run = 0;
    }
}

static void make_initial_file(int fd)
{
    int init_chunk_size;
    int i = 0;
    char *init_buf;
    size_t ret;

    PRINT("File initialization START, total size = %lu\n", desc->max_addr);
    init_chunk_size = mem_allocation(desc, &init_buf, sizeof(char)*INITIAL_SEQ_WRITE_CHUNK_SIZE);
    if (init_buf == NULL) {
	PRINT("Error on memory allocation, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }

    do{
	fill_data(desc, init_buf, init_chunk_size);
	ret = pwrite(fd, init_buf, init_chunk_size, i);
	if (ret != init_chunk_size) {
	    PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
	    exit(1);
	}
	i += init_chunk_size;
    }while(i < desc->max_addr);

    free(init_buf);
    fsync(fd);
    PRINT("File initialization END\n");
}
