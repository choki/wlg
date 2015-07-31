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
#include <pthread.h>
#include "gio.h"
#include "io_generator.h"
#include "io_replayer.h"

/* Local Variables */
static wg_env *setting;

/* Extern Functions */
extern void *workload_replayer(void *arg);
extern void *workload_generator(void *arg);

/* Functions for Initialization */
static void f_file_path(char *in);
static void f_test_mode(unsigned long in);
static void f_thread_num(unsigned long in);
static void f_test_interface_type(unsigned long in);
static void f_test_length_type(unsigned long in);
static void f_total_test_req(unsigned long in);
static void f_total_test_time(unsigned long in);
static void f_max_addr(unsigned long in);
static void f_min_addr(unsigned long in);
static void f_max_size(unsigned long in);
static void f_min_size(unsigned long in);
static void f_sequential_w(unsigned long in);
static void f_nonsequential_w(unsigned long in);
static void f_read_w(unsigned long in);
static void f_write_w(unsigned long in);
static void f_burstiness_number(unsigned long in);
static void f_pose_time(unsigned long in);
static void f_alignment(unsigned long in);
static void f_alignment_unit(unsigned long in);
static void f_random_deterministic(unsigned long in);

/* Local Arrays For Initialization */
static char wg_param_num[NUM_WG_PARAMETER_NUM][255] = { 
    "TEST_MODE",
    "THREAD_NUM",
    "TEST_INTERFACE",		
    "TEST_LENGTH",			
    "TOTAL_TEST_REQUESTS",		
    "TOTAL_TEST_TIME",			
    "MAX_ADDRESS",			
    "MIN_ADDRESS",			
    "MAX_SIZE",				
    "MIN_SIZE",				
    "SEQUENTIAL_W",			
    "NONSEQUENTIAL_W",			
    "READ_W",				
    "WRITE_W",				
    "BURSTINESS_NUMBER",		
    "POSE_TIME",			
    "ALIGNMENT",			
    "ALIGNMENT_UNIT",			
    "RANDOM_DETERMINISTIC"
};

static char wg_param_str[NUM_WG_PARAMETER_STR][255] = { 
    "FILE_PATH"
};

static void (*wg_param_num_cmd[NUM_WG_PARAMETER_NUM])(unsigned long) = { 
    f_test_mode,			
    f_thread_num,
    f_test_interface_type,		
    f_test_length_type,			
    f_total_test_req,			
    f_total_test_time,			
    f_max_addr,			
    f_min_addr,			   					
    f_max_size,			
    f_min_size,			
    f_sequential_w,		
    f_nonsequential_w,		
    f_read_w,			
    f_write_w,				
    f_burstiness_number,	
    f_pose_time,		
    f_alignment,		
    f_alignment_unit,		
    f_random_deterministic	
};

static void (*wg_param_str_cmd[NUM_WG_PARAMETER_STR])(char *) = { 
    f_file_path,
};


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
    thread_info *tinfo;
     int tid;
     void *status;

    /* CPU dedication for blktrace logging */ 
    /*int errno;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(2, &mask);
    if( errno = sched_setaffinity(0, sizeof(mask), &mask) != 0 ){
	PRINT("Error on CPU affinity function, file:%s, line:%d, err:%d\n", \
	       	__func__, __LINE__, errno);
	exit(1);
    }*/
    
    tmpChar = malloc(sizeof(char) * WG_STR_LENGTH);
    line = malloc(sizeof(char) * WG_STR_LENGTH);
    buf = malloc(sizeof(char) * WG_STR_LENGTH);
    tmp = malloc(sizeof(char) * WG_STR_LENGTH);
    setting = malloc(sizeof(wg_env));

    sprintf(buf, "%s%s", "./", "init_workload_generator");
    if( (filp = fopen(buf, "rw")) == NULL){
	PRINT("Error on opening the init_file of workload generator, file:%s, line:%d\n", \
	       	__func__, __LINE__);
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

    //thread related
    tinfo = malloc(setting->thread_num * sizeof(thread_info));
    for(i=0; i<setting->thread_num; i++){
	tinfo[i].thr_num;

	if(setting->test_mode == 0){
	    tid = pthread_create(&tinfo[i].thr, NULL, &workload_generator, (void *)setting);
	}else if(setting->test_mode == 1){
	    tid = pthread_create(&tinfo[i].thr, NULL, &workload_replayer, (void *)setting);
	}
	if(tid < 0){
	    PRINT("Error on thread creation, line:%d, errno:%d\n", __LINE__, tid);
	    exit(1);
	}
    }

    for(i=0; i<setting->thread_num; i++){
	tid = pthread_join(tinfo[i].thr, (void *)&status);
	if(tid < 0){
	    PRINT("Error on thread join, line:%d, errno:%d\n", __LINE__, tid);
	    exit(1);
	}
	PRINT("#%u Thread joined with status %s\n", tinfo[i].thr_num, (char *)status);
	free(status);
    }

    if(setting->file_path)
	free(setting->file_path);
    if(setting)
	free(setting);
    exit(0);
}


/* Functions for Initialization */
static void f_file_path(char *in)
{
    setting->file_path = malloc(sizeof(char) * WG_STR_LENGTH);
    strncpy(setting->file_path, in, (int)strlen(in)+1);
    PRINT("file path : %s\n", (char *)setting->file_path);
}
static void f_test_mode(unsigned long in){
    setting->test_mode = (unsigned int)in;
    PRINT("test mode : %u\n", (unsigned int)in);
}
static void f_thread_num(unsigned long in){
    setting->thread_num = (unsigned int)in;
    PRINT("thread number : %u\n", (unsigned int)in);
}
static void f_test_interface_type(unsigned long in)
{
    setting->test_interface_type = (unsigned int)in;
    PRINT("test_interface_type : %u\n", (unsigned int)in);
    if (setting->test_interface_type == WG_CHARDEV)
	setting->interface_unit = 1;
    else
	setting->interface_unit = SIZE_OF_SECTOR;
}
static void f_test_length_type(unsigned long in)
{
    setting->test_length_type = (unsigned int)in;
    PRINT("test_length_type : %u\n", (unsigned int)in);
}
static void f_total_test_req(unsigned long in)
{
    setting->total_test_req = (unsigned long)in;
    PRINT("total_test_req : %u\n", (unsigned int)in);
}
static void f_total_test_time(unsigned long in)
{
    setting->total_test_time = (unsigned int)in;
    PRINT("total_test_time : %u\n", (unsigned int)in);
}
static void f_max_addr(unsigned long in)
{
    setting->max_addr = in;
    PRINT("max_addr : %lu\n", in);
}
static void f_min_addr(unsigned long in)
{
    setting->min_addr = in;
    PRINT("min_addr : %lu\n", in);
}
static void f_max_size(unsigned long in)
{
    setting->max_size = in;
    PRINT("max_size : %lu\n", in);
}
static void f_min_size(unsigned long in)
{
    setting->min_size = in;
    PRINT("min_size : %lu\n", in);
}
static void f_sequential_w(unsigned long in)
{
    setting->sequential_w = (unsigned int)in;
    PRINT("sequential_w : %u\n", (unsigned int)in);
}
static void f_nonsequential_w(unsigned long in)
{
    setting->nonsequential_w = (unsigned int)in;
    PRINT("nonsequential_w : %u\n", (unsigned int)in);
}
static void f_read_w(unsigned long in)
{
    setting->read_w = (unsigned int)in;
    PRINT("read_w : %u\n", (unsigned int)in);
}
static void f_write_w(unsigned long in)
{
    setting->write_w = (unsigned int)in;
    PRINT("write_w : %u\n", (unsigned int)in);
}
static void f_burstiness_number(unsigned long in){
    setting->burstiness_number = (unsigned int)in;
    PRINT("burstiness_number : %u\n", (unsigned int)in);
}
static void f_pose_time(unsigned long in)
{
    setting->pose_time = (unsigned int)in;
    PRINT("pose_time : %u\n", (unsigned int)in);
}
static void f_alignment(unsigned long in)
{
    setting->alignment = (unsigned int)in;
    PRINT("alignment : %u\n", (unsigned int)in);
}
static void f_alignment_unit(unsigned long in)
{
    setting->alignment_unit = (unsigned int)in;
    PRINT("alignment_unit : %u\n", (unsigned int)in);
}
static void f_random_deterministic(unsigned long in){
    setting->rand_deterministic = (unsigned int)in;
    PRINT("random deterministic : %u\n", (unsigned int)in);
}
