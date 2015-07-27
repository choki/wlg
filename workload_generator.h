#ifndef __WORKLOAD_GENERATOR_H__
#define __WORKLOAD_GENERATOR_H__

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
    TEST_INTERFACE_TYPE,	    // 0
    TEST_LENGTH_TYPE,	    // 1
    TOTAL_TEST_REQUESTS,	    // 2
    TOTAL_TEST_TIME,	    // 3
    MAX_ADDR,		    // 4
    MIN_ADDR,		    // 5
    MAX_SIZE,		    // 6
    MIN_SIZE,		    // 7
    SEQUENTIAL_W,		    // 8
    NONSEQUENTIAL_W,	    // 9
    READ_W,			    // 10
    WRITE_W,		    // 11
    BURSTINESS_NUMBER,	    // 12
    POSE_TIME,		    // 13
    ALIGNMENT,		    // 14
    ALIGNMENT_UNIT,		    // 15
    RANDOM_DETERMINISTIC,
    NUM_WG_PARAMETER_NUM,	    // 16
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
    unsigned int test_length_type;
    unsigned int total_test_time;
    unsigned int rand_deterministic;
} WG_ENV, *PWG_ENV;



/******************************************************************************************************
 *************                            External Variables                              *************
 ******************************************************************************************************/

extern int ****damaged_block;


#endif
