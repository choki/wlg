#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> //getline()
#include <fcntl.h>
#include <string.h> //strtok()
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
    string->cpu = atoi(ptr);
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
