#ifndef __HELPUTILS_H__
#define __HELPUTILS_H__

#include <tcl.h>

/*
 * Only declare, I do not really test on Windows platform
 */
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef F_OK
#    define F_OK 00
#endif
#ifndef X_OK
#    define X_OK 01
#endif
#ifndef W_OK
#    define W_OK 02
#endif
#ifndef R_OK
#    define R_OK 04
#endif

#define APACHE_PROXY_FCGI_PREFIX "proxy:fcgi://"
#define APACHE_PROXY_BALANCER_PREFIX "proxy:balancer://"

extern const char *FileSuffix(const char path[]);

extern void LogMessage(const char* format, ...);

#endif
