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
#include "./WLGenerator.h"




static uint64_t utime_calculator(struct timeval *s, struct timeval *e){
 	long sec, usec;
    	
	//print_log("start sec:%ld, usec:%ld\n", s->tv_sec, s->tv_usec);
 	//print_log("end sec:%ld, usec:%ld\n", e->tv_sec, e->tv_usec);
	sec = e->tv_sec - s->tv_sec;
	usec = e->tv_usec - s->tv_usec;
	if(sec < 0){
	    print_log("%s : unbelievable thing happended, time warp\n", __func__);
	    exit(1);
	}
	if(usec < 0){
	    sec--;
	    usec += 1000000;
	}
	return sec*1000000ULL + usec;
}

static void io_completion_handler(int sig, siginfo_t *si, void *ucontext){
	struct aiocb *aiocbp;
	int rtn;
	int reqID;
	int i=1;
	
	if(si->si_signo == IO_SIGNAL){
	    aiocbp = (struct aiocb *)si->si_value.sival_ptr;
	    reqID = (int)si->si_value.sival_int;
	    while(1){ 
		if(aio_error(aiocbp) == 0 ){
		    rtn = aio_return(aiocbp);
		    print_log("\n %s : IO succeeded : reqID=%d, rtn=%d\n", __func__, reqID, rtn);
		    break;
		}else{
		    print_log("%s : IO failed : reqID=%d, rtn=%d", __func__, reqID, rtn);
		}
	    }

	    print_log("%s : reqID=%d\n", __func__, reqID);
	}
}

static void io_initialize(struct aiocb * aiocbp, int fd, 
	unsigned int buflen, int fileOffset, int reqID){
 
	aiocbp->aio_fildes = fd;
	aiocbp->aio_nbytes = buflen;
	aiocbp->aio_offset = fileOffset;

	aiocbp->aio_reqprio = 0;
}

static int io_enqueue(IO_DIR direction, struct aiocb *aiocbp){
    	int rtn;

    	if(direction == DIR_READ)
	    rtn = aio_read(aiocbp);
	else
	    rtn = aio_write(aiocbp);

	if(rtn){
	    print_log("%s : aio Read/Write error, Direction:%d, rtn:%d\n",
		    __func__, direction, rtn);
	    exit(1);
	}else{
	    //print_log("%s : IO_DIR : %d\n", __func__, rtn);
	}
	return rtn;
}

static void fill_buffer(char *buf, int size){
    unsigned int perSize;
    int rNum;
    int i=0;
	
    while(size){
	rNum = rand();
	perSize = sizeof(rNum);
	if(perSize > size)
	    perSize = size;
	memcpy(buf, &rNum, perSize);
	size -= perSize;
	buf += perSize;
    }
}

static int mem_allocation(char **buf, int reqSize, bool align){
	int alignedReqSize;
	int i;
	
	if(align){
	    alignedReqSize = (reqSize+SECTOR_SIZE-1)/SECTOR_SIZE*SECTOR_SIZE;
	    print_log("%s : reqSize:%d, alignedReqSize:%d, align:%d\n",
		    __func__, reqSize, alignedReqSize, align);
	    if( posix_memalign((void **)buf, SECTOR_SIZE, alignedReqSize) != 0){
		print_log("%s : buffer allocation failed\n", __func__);
		exit(1);
	    }
	}else{
	    if(	(*buf = (char *)malloc(reqSize)) == NULL ){
		print_log("%s : buffer allocation failed\n", __func__);
		exit(1);
	    }
	}
	if(align)
	    return alignedReqSize;
	else
	    return reqSize;
}

static int get_rand_offset(int align, int maxOffset, int unit){
    	int randNum;
	if(align){
	    //randNum = (rand()%MAX_FILE_SIZE + 1)/SECTOR_SIZE*SECTOR_SIZE;
	    randNum = (rand()%maxOffset + 1)/unit*unit;
	}
	else
	    //randNum = rand()%MAX_FILE_SIZE;
	    randNum = rand()%maxOffset;
        
	//print_log("%s : randNum : %d\n",__func__, randNum);
	return randNum;
}

void main(int argc, char *argv[]){
	int fd;
	int fd2;
	int rtn;
	struct timeval stime, etime;
	uint64_t ttime=0;
	char resultBuf[100]={""};
	int i,j,k;
	int loop;
	char **nBuf;
	int tmpReqSize;
	int reqSize;
	int fileOffset;

	int pageOffset;

	typedef struct ioRequest_t{
	    int id;
	    int status;
	    struct aiocb *aiocbp;
	    struct timeval sTime;
	    struct timeval eTime;
	}ioRequest;
	
	ioRequest *ioList;
	struct aiocb *aiocbList;
	struct sigaction sa;
	int reqNum = 2;
	int remainReqNum;
	int chunkSize;
	char * wBuf;
	
	//init random seed
	srand(time(NULL));

	if( (fd = open("/dev/sda1", O_CREAT|O_RDWR, 0666)) == -1){
	//if( (fd = open("./test.txt", O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
	    print_log("%s : File open error : %d\n", __func__, fd);
	    exit(1);
	}
	if( (fd2 = open("./result.txt", O_CREAT|O_RDWR, 0666)) == -1){
	    print_log("%s : File open error : %d\n", __func__, fd);
	    exit(1);
	}
	
	/* aio test */
	/*
	ioList = calloc(reqNum, sizeof(ioRequest));
	if(ioList == NULL){
	    print_log("%s : ioList calloc error\n", __func__);
	    exit(1);
	}
	aiocbList = calloc(reqNum, sizeof(struct aiocb));
	if(aiocbList == NULL){
	    print_log("%s : aiocbList calloc error\n", __func__);
	    exit(1);
	}

	reqSize = mem_allocation(&wBuf, 128*1024, true);
	fill_buffer(wBuf, reqSize);
	lseek(fd, 0, SEEK_SET);
	for(i=0; i<1000; i++)
	    write(fd, wBuf, reqSize);

	chunkSize = 4*1024;
	for(i=0; i<reqNum; i++){
	    ioList[i].id = i;
	    ioList[i].status = EINPROGRESS;
	    ioList[i].aiocbp = &aiocbList[i];
	    
	    nBuf = (char **)&(ioList[i].aiocbp->aio_buf);

	    reqSize = mem_allocation(&(*nBuf), chunkSize, true);
	    //fill_buffer(*nBuf, reqSize);

	}
	sleep(5);
	loop = 10000;
	for(k=chunkSize ; k<50*chunkSize; k+=chunkSize){
	    gettimeofday(&ioList[0].sTime, NULL);
	    for(j=0 ; j<loop; j++){
		//fileOffset = get_rand_offset(true, 255*1024*1000, 4*1024);
		fileOffset = 0;

		for(i=0; i<reqNum; i++){
		    fileOffset = i*k;
		    //printf("%d ", fileOffset);
		    io_initialize(ioList[i].aiocbp, fd, reqSize, fileOffset, i);
		    //gettimeofday(&ioList[i].sTime, NULL);
		    io_enqueue(DIR_READ, ioList[i].aiocbp);
		}
		//printf("\n");
		remainReqNum = reqNum;

		while(remainReqNum){

		    for(i=0; i<reqNum; i++){
			if(ioList[i].status == EINPROGRESS){
			    ioList[i].status = aio_error(ioList[i].aiocbp);

			    switch (ioList[i].status) {
				case 0:
				    //printf("I/O succeeded\n");
				    break;
				case EINPROGRESS:
				    //printf(".");
				    break;
				case ECANCELED:
				    printf("Canceled\n");
				    break;
				default:
				    printf("aio_error\n");
				    break;
			    }
			    if(ioList[i].status != EINPROGRESS){
				//gettimeofday(&ioList[i].eTime, NULL);
				remainReqNum--;
			    }
			}
		    }
		}
		for(i=0; i<reqNum; i++){
		    ioList[i].status = EINPROGRESS;
		    //print_log("elapse time : %lld\n", utime_calculator(&ioList[i].sTime, &ioList[i].eTime));
		    //ttime += utime_calculator(&ioList[i].sTime, &ioList[i].eTime);	    
		}
	    }
	    gettimeofday(&ioList[0].eTime, NULL);
	    ttime = utime_calculator(&ioList[0].sTime, &ioList[0].eTime);	    


	    print_log("%s : total elapse time : %lld, loop : %d\n", __func__, ttime, loop);
	    sprintf(resultBuf, "%10d, %10d, %10d\n", k, ttime, loop*chunkSize/ttime );
	    printf("%10d, %10d, %10d\n", k, ttime, loop*4*chunkSize/ttime );
	    lseek(fd, 0, SEEK_SET);
	    write(fd2, resultBuf, sizeof(resultBuf));
	    ttime = 0;
	}
	*/
	/*******************************/
	/* Mapping granurarity checker */
	
	for(tmpReqSize=1*1024; tmpReqSize<4*1024; tmpReqSize+=1*1024){
	    
	    //reqSize = mem_allocation(&nBuf, tmpReqSize, true);
	    reqSize = mem_allocation(&nBuf, 4096, true);
	    
	    for(loop=0; loop<2; loop++){
		fill_buffer(nBuf, reqSize);

		fileOffset = get_rand_offset(true, MAX_FILE_SIZE, SECTOR_SIZE);
		//lseek(fd, 0, SEEK_SET); 
		lseek(fd, fileOffset, SEEK_SET); 

		gettimeofday(&stime, NULL);
		printf("WRITE : %d, offset : %d\n", write(fd, nBuf, reqSize), fileOffset);
		//fsync(fd);
		gettimeofday(&etime, NULL);
		ttime = utime_calculator(&stime, &etime);	    
		print_log("%s : total elapse time : %lld\n", __func__, ttime);

		sprintf(resultBuf, "%10d, %10d, %20lld\n", reqSize, loop, ttime);
		lseek(fd, sizeof(resultBuf), SEEK_SET);
		write(fd2, resultBuf, sizeof(resultBuf));
		//usleep(7000);
	    }
	    free(nBuf);
	}
	
	
	/*******************************/
	/* align test */
	    /*
	for(tmpReqSize=1*1024; tmpReqSize<=28*1024; tmpReqSize+=1*1024){
	    reqSize = mem_allocation(&nBuf, tmpReqSize, true);
	    for(pageOffset=0; pageOffset<40*1024; pageOffset+=1*1024){
		for(loop=0; loop<1; loop++){
		    fill_buffer(nBuf, reqSize);
		    lseek(fd, pageOffset, SEEK_SET); 
		    gettimeofday(&stime, NULL);
		    printf("WRITE : %d\n", write(fd, nBuf, reqSize));
		    //fsync(fd);
		    gettimeofday(&etime, NULL);
		    ttime = utime_calculator(&stime, &etime);	    
		    print_log("%s : total elapse time : %lld\n", __func__, ttime);

		    sprintf(resultBuf, "%10d, %10d, %10d, %10lld\n",
			    reqSize, pageOffset, loop, ttime);
		    lseek(fd, sizeof(resultBuf), SEEK_SET);
		    write(fd2, resultBuf, sizeof(resultBuf));
		    usleep(5000);
		}
	    }
	    free(nBuf);
	}*/
	close(fd);
	close(fd2);
}
