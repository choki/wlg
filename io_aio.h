#ifndef __IO_AIO_H__
#define __IO_AIO_H__

#define MAX_QUEUE_DEPTH (32)

typedef struct _my_iocb{
    struct iocb iocbp;
    int qid;
}my_iocb;

int aio_initialize(unsigned int max_queue_depth);
int aio_enqueue(
	int fd, 
	char *buf, 
	unsigned long size, 
	unsigned long start_addr, 
	OPERATION_TYPE op
	);
void aio_termination(void);
int get_aio_status(void);

#endif //__IO_AIO_H__
