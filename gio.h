#ifndef __GIO_H__
#define __GIO_H__

/******************************************************************************************************
 *************                                 Defines                                    *************
 ******************************************************************************************************/

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define PRINT(...) \
    	do{ printf(__VA_ARGS__); }while(0)
#else
#define PRINT
#endif

#define WG_STR_LENGTH   1024
#define SIZE_OF_SECTOR 	512

#define TIME_VALUE(t) ( ((t)->tv_sec)*1000000ULL+((t)->tv_usec) )
//#define GET_ALIGNED_VALUE(v) ((v)+SIZE_OF_SECTOR-1)/SIZE_OF_SECTOR*SIZE_OF_SECTOR
#define GET_ALIGNED_VALUE(v) ((v)-((v)%SIZE_OF_SECTOR))

enum TEST_INTERFACE_TYPE{
    WG_CHARDEV,			    // 0
    WG_BLKDEV,			    // 1
    NUM_TEST_LENGTH_TYPE	    // 2
};

enum TEST_LENGTH_TYPE {
	WG_TIME,		    // 0
	WG_NUMBER,		    // 1
	NUM_TEST_LENGHTH_TYPE	    // 2
};

enum WG_PARAMETER_NUM { 
    TEST_MODE,
    THREAD_NUM,
    TEST_INTERFACE_TYPE,    
    TEST_LENGTH_TYPE,	    
    TOTAL_TEST_REQUESTS,    
    TOTAL_TEST_TIME,	    
    MAX_ADDR,		    
    MIN_ADDR,		    
    MAX_SIZE,		    
    MIN_SIZE,		    
    SEQUENTIAL_W,	    
    NONSEQUENTIAL_W,	    
    READ_W,		    
    WRITE_W,		    
    BURSTINESS_NUMBER,	    
    POSE_TIME,		    
    ALIGNMENT,		    
    ALIGNMENT_UNIT,	    
    RANDOM_DETERMINISTIC,
    NUM_WG_PARAMETER_NUM,   
};

enum WG_PARAMETER_STR { 
    FILE_PATH,
    NUM_WG_PARAMETER_STR,	    
};

enum OPERATION_TYPE {
    WG_READ,			    // 0
    WG_WRITE,			    // 1
    NUM_OPERATION_TYPE		    // 2
};

/******************************************************************************************************
 *************                                 Structures                                 *************
 ******************************************************************************************************/

/********************************************
 *****   Environmental Structures  **********
 ********************************************/

typedef struct _wg_env {
    char *file_path;
    unsigned int test_mode;
    unsigned int thread_num;
    unsigned int read_w;
    unsigned int write_w;
    unsigned int sequential_w;
    unsigned int nonsequential_w;
    unsigned long max_addr;
    unsigned long min_addr;
    unsigned long max_size;
    unsigned long min_size;

    unsigned long total_test_req;

    unsigned int burstiness_number;
    unsigned int pose_time; //ms

    unsigned int alignment;
    unsigned int alignment_unit;

    unsigned int test_interface_type;
    unsigned int interface_unit;
    unsigned int test_length_type;
    unsigned int total_test_time;
    unsigned int rand_deterministic;
} wg_env;

typedef struct _thread_info {
    pthread_t thr;
    unsigned int thr_num;
} thread_info;


#endif 		//__GIO_H__
