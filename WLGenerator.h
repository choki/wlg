#define BUF_SIZE 125
#define SECTOR_SIZE 512

#define DEBUG_MODE

#define MAX_FILE_SIZE (40*1024*1024)
#define IO_SIGNAL SIGUSR1

#ifdef DEBUG_MODE
#define print_log(...) \
    	do{ printf(__VA_ARGS__); }while(0)
#else
#define print_log
#endif

typedef enum{
    DIR_READ=0,
    DIR_WRITE
}IO_DIR;


/*typedef struct ioRequest_t{
	int 	
}ioRequest;*/
