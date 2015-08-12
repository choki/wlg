#ifndef __GIO_H__
#define __GIO_H__


/* extern variables */
extern unsigned long max_written_size;
extern unsigned int shared_cnt;
extern pthread_mutex_t thr_mutex;

/* Types */
typedef enum{
    WG_GENERATING_MODE,
    WG_REPLAY_MODE,
    NUM_TEST_MODE_TYPE
}TEST_MODE_TYPE;

typedef enum {
    WG_CHARDEV,			   
    WG_BLKDEV,			   
    NUM_INTERFACE_TYPE	    
}INTERFACE_TYPE;

typedef enum {
    WG_TIME,		    
    WG_NUMBER,		    
    NUM_TEST_LENGHTH_TYPE	   
}TEST_LENGTH_TYPE;

typedef enum { 
    TEST_MODE,
    THREAD_NUM,
    TEST_INTERFACE,    
    TEST_LENGTH,	    
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
    MAX_QUEUE_DEPTH,
    NUM_WG_PARAMETER_NUM,   
}WG_PARAMETER_NUM;

typedef enum { 
    FILE_PATH,
    NUM_WG_PARAMETER_STR,	    
}WG_PARAMETER_STR;

typedef enum {
    WG_READ,		
    WG_WRITE,		
    NUM_OPERATION_TYPE	
}OPERATION_TYPE;

typedef enum {
    WG_SEQ,			    
    WG_RND,			   
    NUM_SEQUENTIALITY_TYPE	
}SEQUENTIALITY_TYPE;

/* Structures */
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

    unsigned int max_queue_depth;
} wg_env;

typedef struct _thread_info {
    pthread_t thr;
} thread_info;


#endif 		//__GIO_H__
