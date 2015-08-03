#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef DEBUG_MODE
#define PRINT(...) \
    	do{ printf(__VA_ARGS__); }while(0)
#else
#define PRINT
#endif

#endif //__COMMON_H__
