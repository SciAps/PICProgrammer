
#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

#define LOG(format, ...) printf(format, ##__VA_ARGS__)

#define DEBUG_TRACE 1

#ifdef DEBUG_TRACE
#  define TRACE(format, ...) LOG("[%s " __FILE__ ":%d] " format, __func__, __LINE__, ##__VA_ARGS__)
#else
#  define TRACE(format, ...)
#endif


#endif /* _LOG_H_ */
