#ifndef __COMMON_H__
#define __COMMON_H__


#include "gio.h"
#define USER_SETTING_FILE_NAME "./settings"
#define TRACE_INPUT_FILE_NAME "./trace"
#define PARSER_INPUT_FILE_NAME "./trace.blkparse"
#define PARSER_OUTPUT_FILE_NAME "./trace_p"
#define TRACER_OUPUT_FILE_NAME "./trace"
#define FIRST_RUN_CHECK_FILE_NAME "./first_run"
#define MAX_STR_LEN (1024)
#define VERIFY_STR_LEN (20)   
#define DEBUG_MODE 
#define TIME_VALUE(t) ( ((t)->tv_sec)*1000000ULL+((t)->tv_usec) )
#define MICRO_SECOND(sec) 	(sec)*1000000ULL
#define MILLI_SECOND(sec) 	(sec)*1000ULL
#define SIZE_OF_SECTOR 	(512)
#define INITIAL_SEQ_WRITE_CHUNK_SIZE (1024*1024) //1024KB
#define NOP __asm__ __volatile__("rep;nop": : :"memory")

//#define GET_ALIGNED_VALUE(v) ((v)+SIZE_OF_SECTOR-1)/SIZE_OF_SECTOR*SIZE_OF_SECTOR
#define GET_ALIGNED_VALUE(v) ((v)-((v)%SIZE_OF_SECTOR))

#ifdef DEBUG_MODE
#define PRINT(...) \
    	do{ printf(__VA_ARGS__); }while(0)
#else
#define PRINT
#endif

typedef struct _readLine{
    int 	thr_id;
    double 	sTime;
    char 	rwbs[6];	//In replay mode, this value also can indicate queue empty status with value "EMPTY"
    char 	action[4];
    long sSector;
    int 	size;
    double 	eTime;
} readLine;

void parse_one_line(char *line, readLine *string);
void get_current_time(struct timeval *now);
void fill_data(wg_env *desc, char *buf, unsigned int size);
int mem_allocation(wg_env *desc, char **buf, int reqSize);
void get_current_time(struct timeval *now);
void usec_sleep(long long usec);
long long usec_elapsed(struct timeval start);
#endif //__COMMON_H__
