#ifndef __IO_TRACER_H__
#define __IO_TRACER_H__

#define MAX_TRACE_NUM 	2500
#define TRACE_A_LINE_LENGTH 40 	//Estimate value

void *tracer_add(char *line);
void tracer_initialize(void);
void tracer_terminate(void);
void tracer_save_file(void);

#endif //__IO_TRACER_H__
