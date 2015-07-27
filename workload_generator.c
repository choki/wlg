/******************************************************************************************************
 *************                                 Headers                                    *************
 ******************************************************************************************************/

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
#include "workload_generator.h"


/******************************************************************************************************
 *************                           Global Variables                                 *************
 ******************************************************************************************************/



/******************************************************************************************************
 *************                           Local Variables                                  *************
 ******************************************************************************************************/

static PWG_ENV wg_env;
static unsigned long interface_unit = 1;



/******************************************************************************************************
 *************                   Prototypes of Public Functions                           *************
 ******************************************************************************************************/

void workload_generator(void);


/******************************************************************************************************
 *************                   Prototypes of Private Functions                           ************
 ******************************************************************************************************/

/********************************************
 ***  Functions for Workload Generation  ****
 ********************************************/

int select_op(unsigned long cur_file_size);
unsigned long select_start_addr(unsigned long prior_end_addr, int op, unsigned long cur_file_size);
unsigned long select_size(unsigned long start_addr, int op, unsigned long cur_file_size);
void fill_data(char *buf, unsigned int size);
unsigned int get_rand_range(unsigned int min, unsigned int max);
int mem_allocation(char **buf, int reqSize);


/********************************************
 ****   Functions for Initialization   ******
 ********************************************/

void f_file_path(char *in);
void f_test_interface_type(unsigned long in);
void f_test_length_type(unsigned long in);
void f_total_test_req(unsigned long in);
void f_total_test_time(unsigned long in);
void f_max_addr(unsigned long in);
void f_min_addr(unsigned long in);
void f_max_size(unsigned long in);
void f_min_size(unsigned long in);
void f_sequential_w(unsigned long in);
void f_nonsequential_w(unsigned long in);
void f_read_w(unsigned long in);
void f_write_w(unsigned long in);
void f_burstiness_number(unsigned long in);
void f_pose_time(unsigned long in);
void f_alignment(unsigned long in);
void f_alignment_unit(unsigned long in);
void f_random_deterministic(unsigned long in);


/******************************************************************************************************
 *************                  Local Arrays For Initialization                           *************
 ******************************************************************************************************/

static char wg_param_num[NUM_WG_PARAMETER_NUM][255] = { 
    "TEST_INTERFACE_TYPE",		// 0
    "TEST_LENGTH_TYPE",			// 1
    "TOTAL_TEST_REQUESTS",		// 2
    "TOTAL_TEST_TIME",			// 3
    "MAX_ADDRESS",			// 4
    "MIN_ADDRESS",			// 5
    "MAX_SIZE",				// 6
    "MIN_SIZE",				// 7
    "SEQUENTIAL_W",			// 8
    "NONSEQUENTIAL_W",			// 9
    "READ_W",				// 10
    "WRITE_W",				// 11
    "BURSTINESS_NUMBER",		// 12
    "POSE_TIME",			// 13
    "ALIGNMENT",			// 14
    "ALIGNMENT_UNIT",			// 15
    "RANDOM_DETERMINISTIC",			// 16
};

static char wg_param_str[NUM_WG_PARAMETER_STR][255] = { 
    "FILE_PATH"
};

static void (*wg_param_num_cmd[NUM_WG_PARAMETER_NUM])(unsigned long) = { 
    f_test_interface_type,		// 0
    f_test_length_type,			// 1
    f_total_test_req,			// 2
    f_total_test_time,			// 3
    f_max_addr,				// 4
    f_min_addr,				// 5						
    f_max_size,				// 6
    f_min_size,				// 7
    f_sequential_w,			// 8
    f_nonsequential_w,			// 9
    f_read_w,				// 10
    f_write_w,				// 11							
    f_burstiness_number,		// 12
    f_pose_time,			// 13
    f_alignment,			// 14
    f_alignment_unit,			// 15
    f_random_deterministic,		// 16
};

static void (*wg_param_str_cmd[NUM_WG_PARAMETER_STR])(char *) = { 
    f_file_path,
};

/******************************************************************************************************
 *************                            Public Functions                                 ************
 ******************************************************************************************************/


void main(void)
{
    FILE *filp = NULL;
    size_t len = 0;
    char *buf;
    char *tmp;
    char *line;
    int i;
    int ret;
    unsigned long tmpNum;
    char *tmpChar;
    bool found;

    tmpChar = malloc(sizeof(char) * WG_STR_LENGTH);
    line = malloc(sizeof(char) * WG_STR_LENGTH);
    buf = malloc(sizeof(char) * WG_STR_LENGTH);
    tmp = malloc(sizeof(char) * WG_STR_LENGTH);
    wg_env = malloc(sizeof(WG_ENV));

    sprintf(buf, "%s%s", "./", "init_workload_generator");
    if( (filp = fopen(buf, "rw")) == NULL){
	PRINT("Error on opening the init_file of workload generator, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }

    while (0 <= getline(&line, &len, filp)) {
	found = false;

	if ('#' == line[0]) 
	    continue;
	if ('\n' == line[0]) 
	    continue;

	sscanf(line, "%s", tmp);

	for (i = 0; i < NUM_WG_PARAMETER_NUM; i++) {
	    if (0 == strcmp(wg_param_num[i], tmp)) {
		ret = sscanf(line, "%s %lu", tmp, &tmpNum);
		wg_param_num_cmd[i](tmpNum);
		found = true;
	    }
	}
	if(found == false){
	    for (i = 0; i < NUM_WG_PARAMETER_STR; i++) {
		if(0 == strcmp(wg_param_str[i], tmp)) {
		    ret = sscanf(line, "%s %s", tmp, tmpChar);
		    wg_param_str_cmd[i](tmpChar);
		    found = true;
		}
	    }
	    if (found == false){
		PRINT("There is no matching parameter. Plz check setting parameter name.\n");
		PRINT("Value : %s\n", line);
	    }
	}
    }
    free(tmpChar);
    free(line);
    free(buf);
    free(tmp);
    fclose(filp);

    workload_generator();
}

void workload_generator(void)
{
    int fd = NULL;
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

    if(wg_env->rand_deterministic == 0){
	srand(time(NULL));
    }

    if( (fd = open(wg_env->file_path, O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
    //if( (fd = open(wg_env->file_path, O_CREAT|O_RDWR, 0666)) == -1){
	PRINT("Error on opening the init_file of workload generator, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }

    mem_allocation(&buf, (wg_env->max_size)*interface_unit);
    if (NULL == buf) {
	PRINT("Error on memory allocation, file:%s, line:%d\n", __func__, __LINE__);
	exit(1);
    }


    //TODO for test
    lseek(fd, GET_ALIGNED_VALUE(wg_env->max_addr-512), SEEK_SET);
    fill_data(buf, 512);
    ret = write(fd, buf , 512);
    if (ret != 512) {
	PRINT("Error on file I/O (error# : %zu), file:%s, line:%d\n", ret, __func__, __LINE__);
	exit(1);
    }
    max_written_size = wg_env->max_addr;
    //end of TODO

    //sleep(1);
    gettimeofday(&start_time, NULL);

    while (1) {
	op = select_op(max_written_size);
	start_addr = select_start_addr(prior_end_addr, op, max_written_size);
	size = select_size(start_addr, op , max_written_size);

	//TODO for test
	size = 4096;

	if (WG_READ == op)
	    PRINT("READ  ");
	else
	    PRINT("WRITE ");

	fill_data(buf, size);

	PRINT("start_addr : %20lu, size %20lu\n", start_addr, size);
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

	if (wg_env->test_length_type == WG_TIME) {
	    gettimeofday(&current_time, NULL);
	    if ( (TIME_VALUE(&current_time) - TIME_VALUE(&start_time)) >= \
		    (wg_env->total_test_time * 1000ULL) )
		break;
	} else if (wg_env->test_length_type == WG_NUMBER) {
	    if (i >= (unsigned int)wg_env->total_test_req) 
		break;
	} else {
	    PRINT("Error on file:%s, line:%d", __func__, __LINE__);
	}

	if (counter >= wg_env->burstiness_number) {
	    counter = 0;
	    gettimeofday(&posed_time, NULL);

	    while (1) {
		gettimeofday(&current_time, NULL);
		if ( (TIME_VALUE(&current_time) - TIME_VALUE(&posed_time)) >= \
			((long long)wg_env->pose_time * 1000ULL) )
		    break;
	    }
	}
    }
    PRINT("END OF WG\n");
    close(fd);
    free(buf);
    free(wg_env->file_path);
    free(wg_env);
}

/******************************************************************************************************
 *************                        Private Functions                                    ************
 ******************************************************************************************************/

/********************************************
 ***  Functions for Workload Generation  ****
 ********************************************/
int select_op(unsigned long cur_file_size)
{
    int selector;

    if(cur_file_size == 0){
	PRINT("First req must be write\n");
	return WG_WRITE;
    }

    selector = get_rand_range(0, wg_env->read_w + wg_env->write_w - 1);
    if(selector < wg_env->read_w)
	return WG_READ;
    else
	return WG_WRITE;
}

unsigned long select_start_addr(unsigned long prior_end_addr, int op, unsigned long cur_file_size)
{
    unsigned long selector;

    selector = get_rand_range(0, wg_env->sequential_w + wg_env->nonsequential_w - 1);

    //Sequential case
    if(selector < wg_env->sequential_w){
	selector = prior_end_addr;
	PRINT("S ");
    }
    //Random case
    else{
	if(op == WG_READ){
	    selector = get_rand_range(wg_env->min_addr, cur_file_size - 1);
	    //PRINT("Read rand: cur_file_size=%lu, selector=%lu\n", cur_file_size, selector);
	}else{
	    selector = get_rand_range(wg_env->min_addr, wg_env->max_addr - 1);
	}
	PRINT("R ");
    }

    if( wg_env->alignment ){
	selector = GET_ALIGNED_VALUE(selector);
    }

    return selector;
}

unsigned long select_size(unsigned long start_addr, int op, unsigned long cur_file_size)
{
    unsigned long selector;
    unsigned long aligned_selector;

    selector = get_rand_range(wg_env->min_size, wg_env->max_size - 1);

    if(op == WG_READ && start_addr+selector > cur_file_size){
	selector = cur_file_size - start_addr;
	//PRINT("Read re-size: cur_file_size=%lu, modified selector=%lu\n", cur_file_size, selector);
    }else if(start_addr + selector > wg_env->max_addr){
	selector = wg_env->max_addr - start_addr;
    }
    if(wg_env->alignment == 1){
	selector = GET_ALIGNED_VALUE(selector);
    }
    return selector;
}

void fill_data(char *buf, unsigned int size)
{
    if( memset(buf ,rand() ,size * interface_unit) == NULL){
	PRINT("Error on workload data setup, file:%s, line:%d\n", __func__, __LINE__);
    }
}

unsigned int get_rand_range(unsigned int min, unsigned int max)
{
    unsigned int range = max - min + 1;
    return (unsigned int)((rand() % range) + min);
}

int mem_allocation(char **buf, int reqSize)
{
	int alignedReqSize;
	int i;
	
	if(wg_env->alignment){
	    alignedReqSize = GET_ALIGNED_VALUE(reqSize);
	    PRINT("%s : reqSize:%d, alignedReqSize:%d, align:%d\n",
		    __func__, reqSize, alignedReqSize, wg_env->alignment);
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
	if(wg_env->alignment)
	    return alignedReqSize;
	else
	    return reqSize;
}

/********************************************
 ****   Functions for Initialization   ******
 ********************************************/

void f_file_path(char *in)
{
    wg_env->file_path = malloc(sizeof(char) * WG_STR_LENGTH);
    strncpy(wg_env->file_path, in, (int)strlen(in)+1);
    PRINT("test_length_type : %s\n", (char *)wg_env->file_path);
}
void f_test_interface_type(unsigned long in)
{
    wg_env->test_interface_type = (unsigned int)in;
    PRINT("test_interface_type : %u\n", (unsigned int)in);
    if (wg_env->test_interface_type == WG_CHARDEV)
	interface_unit = 1;
    else
	interface_unit = SIZE_OF_SECTOR;
}
void f_test_length_type(unsigned long in)
{
    wg_env->test_length_type = (unsigned int)in;
    PRINT("test_length_type : %u\n", (unsigned int)in);
}
void f_total_test_req(unsigned long in)
{
    wg_env->total_test_req = (unsigned long)in;
    PRINT("total_test_req : %u\n", (unsigned int)in);
}
void f_total_test_time(unsigned long in)
{
    wg_env->total_test_time = (unsigned int)in;
    PRINT("total_test_time : %u\n", (unsigned int)in);
}
void f_max_addr(unsigned long in)
{
    wg_env->max_addr = in;
    PRINT("max_addr : %lu\n", in);
}
void f_min_addr(unsigned long in)
{
    wg_env->min_addr = in;
    PRINT("min_addr : %lu\n", in);
}
void f_max_size(unsigned long in)
{
    wg_env->max_size = in;
    PRINT("max_size : %lu\n", in);
}
void f_min_size(unsigned long in)
{
    wg_env->min_size = in;
    PRINT("min_size : %lu\n", in);
}
void f_sequential_w(unsigned long in)
{
    wg_env->sequential_w = (unsigned int)in;
    PRINT("sequential_w : %u\n", (unsigned int)in);
}
void f_nonsequential_w(unsigned long in)
{
    wg_env->nonsequential_w = (unsigned int)in;
    PRINT("nonsequential_w : %u\n", (unsigned int)in);
}
void f_read_w(unsigned long in)
{
    wg_env->read_w = (unsigned int)in;
    PRINT("read_w : %u\n", (unsigned int)in);
}
void f_write_w(unsigned long in)
{
    wg_env->write_w = (unsigned int)in;
    PRINT("write_w : %u\n", (unsigned int)in);
}
void f_burstiness_number(unsigned long in){
    wg_env->burstiness_number = (unsigned int)in;
    PRINT("burstiness_number : %u\n", (unsigned int)in);
}
void f_pose_time(unsigned long in)
{
    wg_env->pose_time = (unsigned int)in;
    PRINT("pose_time : %u\n", (unsigned int)in);
}
void f_alignment(unsigned long in)
{
    wg_env->alignment = (unsigned int)in;
    PRINT("alignment : %u\n", (unsigned int)in);
}
void f_alignment_unit(unsigned long in)
{
    wg_env->alignment_unit = (unsigned int)in;
    PRINT("alignment_unit : %u\n", (unsigned int)in);
}

void f_random_deterministic(unsigned long in){
    wg_env->rand_deterministic = (unsigned int)in;
    PRINT("random deterministic : %u\n", (unsigned int)in);
}
