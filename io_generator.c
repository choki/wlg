
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

/********************************************
 	  Static Functions 
 ********************************************/
static int select_op(unsigned long cur_file_size);
static unsigned long select_start_addr(unsigned long prior_end_addr, int op, unsigned long cur_file_size);
static unsigned long select_size(unsigned long start_addr, int op, unsigned long cur_file_size);
static void fill_data(char *buf, unsigned int size);
static unsigned int get_rand_range(unsigned int min, unsigned int max);
static int mem_allocation(char **buf, int reqSize);


static wg_env *desc;

void *workload_generator(void *arg)
{
    int fd;
    int op;
    size_t ret;
    unsigned long start_addr;
    unsigned long size;
    unsigned long prior_end_addr = 0;
    unsigned int counter = 0;
    unsigned long i = 0;
    char *buf;
    struct timeval current_time, \
	posed_time, \
	start_time;
    unsigned long max_written_size = 0;
#ifdef ONLY_FOR_TEST
    int test_count=1;
#endif
    desc = (wg_env *)arg;

    if(desc->rand_deterministic == 0){
	srand(time(NULL));
    }

    if( (fd = open(desc->file_path, O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
    //if( (fd = open(desc->file_path, O_CREAT|O_RDWR, 0666)) == -1){
	PRINT("Error on opening the init_file of workload generator, file:%s, line:%d, fd=%d\n", __func__, __LINE__, fd);
	exit(1);
    }

    mem_allocation( &buf, (desc->max_size)*(desc->interface_unit) );
    if (NULL == buf) {
	PRINT("Error on memory allocation, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }

#ifdef ONLY_FOR_TEST
    lseek(fd, GET_ALIGNED_VALUE(desc->max_addr-512), SEEK_SET);
    fill_data(buf, 512);
    ret = write(fd, buf , 512);
    if (ret != 512) {
	PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
	exit(1);
    }
    max_written_size = desc->max_addr;
    //size = 2048;
#endif
    //usleep(50);
    gettimeofday(&start_time, NULL);

    while (1) {
	op = select_op(max_written_size);
	start_addr = select_start_addr(prior_end_addr, op, max_written_size);
	
#ifdef ONLY_FOR_TEST
	size = select_size(start_addr, op , max_written_size);

	//TODO for test
	/*if(test_count%4 == 1){
	    size *= 2;
	}
	test_count++;*/
#endif

	if (WG_READ == op)
	    PRINT("READ  ");
	else
	    PRINT("WRITE ");

	fill_data(buf, size);

	PRINT("\tstart_addr:%12lu \t size:%12lu\n", start_addr, size);
	switch (op){
	    case WG_READ:
		lseek(fd, start_addr, SEEK_SET);
		ret = read(fd, buf , size);
		break;
	    case WG_WRITE:
		lseek(fd, start_addr, SEEK_SET);
		ret = write(fd, buf , size);

		if(max_written_size < start_addr + size){
		    max_written_size = start_addr + size;
		    PRINT("New max_written_size:%lu\n", max_written_size);
		}
		fsync(fd);
		break;
	    default:
		PRINT("Error on file:%s, line:%d\n", __func__, __LINE__);
		exit(1);
	}

	// For sequentiality control
	prior_end_addr = start_addr + size;

	if (ret != size) {
	    PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
	    break;
	}

	i++;
	counter++;

	if (desc->test_length_type == WG_TIME) {
	    gettimeofday(&current_time, NULL);
	    if ( (TIME_VALUE(&current_time) - TIME_VALUE(&start_time)) >= \
		    (desc->total_test_time * 1000ULL) )
		break;
	} else if (desc->test_length_type == WG_NUMBER) {
	    if (i >= (unsigned int)desc->total_test_req) 
		break;
	} else {
	    PRINT("Error on file:%s, line:%d", __func__, __LINE__);
	}

	if (counter >= desc->burstiness_number) {
	    counter = 0;
	    gettimeofday(&posed_time, NULL);

	    while (1) {
		gettimeofday(&current_time, NULL);
		if ( (TIME_VALUE(&current_time) - TIME_VALUE(&posed_time)) >= \
			((long long)desc->pose_time * 1000ULL) )
		    break;
	    }
	}
    }
    PRINT("END OF WG\n");
    close(fd);
    free(buf);
    free(desc->file_path);
    free(desc);
}

/********************************************
 ***  Functions for Workload Generation  ****
 ********************************************/
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

static unsigned long select_start_addr(unsigned long prior_end_addr, int op, unsigned long cur_file_size)
{
    unsigned long selector;

    selector = get_rand_range(0, desc->sequential_w + desc->nonsequential_w - 1);

    //Sequential case
    if(selector < desc->sequential_w){
	selector = prior_end_addr;
	if(selector >= desc->max_addr){
	    selector = desc->min_addr;
	}
	PRINT("S ");
    }
    //Random case
    else{
	if(op == WG_READ){
	    selector = get_rand_range(desc->min_addr, cur_file_size - 1);
	    //PRINT("Read rand: cur_file_size=%lu, selector=%lu\n", cur_file_size, selector);
	}else{
	    selector = get_rand_range(desc->min_addr, desc->max_addr - 1);
	}
	PRINT("R ");
    }

    if( desc->alignment ){
	selector = GET_ALIGNED_VALUE(selector);
    }

    return selector;
}

static unsigned long select_size(unsigned long start_addr, int op, unsigned long cur_file_size)
{
    unsigned long selector;
    unsigned long aligned_selector;

    selector = get_rand_range(desc->min_size, desc->max_size - 1);

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

static void fill_data(char *buf, unsigned int size)
{
    if( memset(buf ,rand() ,size * desc->interface_unit) == NULL){
	PRINT("Error on workload data setup, file:%s, line:%d\n", __func__, __LINE__);
    }
}

static unsigned int get_rand_range(unsigned int min, unsigned int max)
{
    unsigned int range = max - min + 1;
    return (unsigned int)((rand() % range) + min);
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
