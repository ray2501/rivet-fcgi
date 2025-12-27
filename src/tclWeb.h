#ifndef TCL_WEB_H
#define TCL_WEB_H 1

#include <tcl.h>

typedef struct TclWebRequest {
    Tcl_HashTable *headers;
    int headers_printed;
    int headers_set;
    int status;
} TclWebRequest;

typedef struct _interp_globals {
    char *scriptfile;
    TclWebRequest *req;
} interp_globals;


int TclWeb_SendHeaders(TclWebRequest *req);

int TclWeb_SetHeaderType(char *header, TclWebRequest *req);

int TclWeb_PrintHeaders(TclWebRequest *req);

int TclWeb_SetStatus(int status, TclWebRequest *req);

#endif

