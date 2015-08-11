#ifndef __IO_TRACER_H__
#define __IO_TRACER_H__

#define MAX_TRACE_NUM 	1000
#define TRACE_A_LINE_LENGTH 40 	//Estimate value

void *tracer_add(char *line);
void tracer_initialize(void);
void tracer_terminate(void);

#endif //__IO_TRACER_H__
