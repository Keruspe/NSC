#ifndef __UTIL_H__
#define __UTIL_H__

#define BUFFER_SIZE 1024

#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

void error (bool fatal, const char *format, ...) __attribute__((format(printf, 2, 3)));

#endif /*__UTIL_H__*/
