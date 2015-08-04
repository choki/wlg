#ifndef __COMMON_H__
#define __COMMON_H__

#define MAX_STR_LEN 1024
#define DEBUG_MODE
#define TIME_VALUE(t) ( ((t)->tv_sec)*1000000ULL+((t)->tv_usec) )
#define MILLI_SECOND(sec) (sec)*1000000ULL
#define SIZE_OF_SECTOR 	512

//#define GET_ALIGNED_VALUE(v) ((v)+SIZE_OF_SECTOR-1)/SIZE_OF_SECTOR*SIZE_OF_SECTOR
#define GET_ALIGNED_VALUE(v) ((v)-((v)%SIZE_OF_SECTOR))

#ifdef DEBUG_MODE
#define PRINT(...) \
    	do{ printf(__VA_ARGS__); }while(0)
#else
#define PRINT
#endif

typedef struct _readLine{
    int 	cpu;
    double 	sTime;
    char 	rwbs[6];	//In replay mode, this value also can indicate queue empty status with value "EMPTY"
    char 	action[4];
    long sSector;
    int 	size;
    double 	eTime;
} readLine;

void parse_one_line(char *line, readLine *string);
void get_current_time(struct timeval *now);

#endif //__COMMON_H__
