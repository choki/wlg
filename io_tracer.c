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
#include <unistd.h>
#include <pthread.h>
#include "gio.h"
#include "common.h"
#include "io_tracer.h"

/* Static functions */


static char *ptracer;
static unsigned int pcurrent = 0;
static unsigned int line_cnt = 0;
static int overflow = 0;
static int length_per_line[MAX_TRACE_NUM];


void *tracer_add(char *line)
{
    if(overflow == 0){
	strcpy(ptracer+pcurrent, line);
	PRINT("pcurrent: %u line:%s\n", pcurrent, ptracer+pcurrent);
	pcurrent += strlen(line);
	length_per_line[line_cnt] = strlen(line);
	line_cnt++;

	if(line_cnt > MAX_TRACE_NUM){
	    overflow = 1;
	}
    }
}

void tracer_initialize(void)
{
    ptracer = (char *)calloc(MAX_TRACE_NUM, TRACE_A_LINE_LENGTH);
    if(ptracer == NULL){
	PRINT("%s : buffer allocation failed\n", __func__);
	exit(1);
   }
}

void tracer_save_file(void)
{
    FILE *fpW = NULL;
    int i;
    char *tmp = ptracer;

    if(line_cnt > MAX_TRACE_NUM){
	PRINT("\n"); 
	PRINT("\t*************** WARNING! ****************\n"); 
	PRINT("\tTrace buffer was overflowed while processing.\n");
	PRINT("\tOnly part of the traces are saved.\n");
	PRINT("\tRecommand to enlarge \"MAX_TRACE_NUM value.\"\n");
	PRINT("\tCurrent \"MAX_TRACE_NUM\" is %d and total traces are %d\n", 
		MAX_TRACE_NUM, line_cnt);
	PRINT("\t******************************************\n"); 
	PRINT("\n"); 
	overflow = 1;
    }
    if( (fpW=fopen(TRACER_OUPUT_FILE_NAME, "w+")) == NULL ){
	PRINT("Error on opening the \"trace\" file, file:%s, line:%d\n", \
	       	__func__, __LINE__);
	exit(1);
    }
    for(i=0; i<line_cnt; i++){
	fwrite(tmp, 1, length_per_line[i], fpW);
	fwrite("\n", 1, 1, fpW);
	tmp += length_per_line[i];
    }
    fflush(fpW);
    fclose(fpW);
    tracer_terminate();
}

void tracer_terminate(void)
{
    if(ptracer){
	free(ptracer);
    }
}
