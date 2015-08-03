#ifndef __COMMON_H__
#define __COMMON_H__

#define MAX_STR_LEN 1024
#define DEBUG_MODE

#ifdef DEBUG_MODE
#define PRINT(...) \
    	do{ printf(__VA_ARGS__); }while(0)
#else
#define PRINT
#endif

typedef struct _readLine{
    int 	cpu;
    double 	sTime;
    char 	rwbs[4];
    char 	action[4];
    long 	sSector;
    int 	size;
    double 	eTime;
} readLine;

void parse_one_line(char *line, readLine *string);

#endif //__COMMON_H__
