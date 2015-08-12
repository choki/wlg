#ifndef __IO_REPLAYER_H__
#define __IO_REPLAYER_H__

//Set the value at least bigger than 152KB, 
//	which is blktrace log trace's max request size.
#define REPLAYER_MAX_FILE_SIZE 5*1024*1024 	//5MB


void *workload_replayer(void *arg);

#endif //__IO_REPLAYER_H__
