#ifndef TCL_WEB_H
#define TCL_WEB_H 1

#include <tcl.h>

typedef struct TclWebRequest {
    Tcl_HashTable *headers;
    int headers_printed;
    int headers_set;
    int content_sent;
    int status;
} TclWebRequest;

typedef struct _interp_globals {
    char *scriptfile;
    TclWebRequest *req;
    int script_exit;
    int page_aborting;
    Tcl_Obj *abort_code;
} interp_globals;

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_SendHeaders --
 * Sends HTTP headers.
 *
 * Results:
 * HTTP headers output.  Things like cookies may no longer be manipulated.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_SendHeaders(TclWebRequest *req);

int TclWeb_SetHeaderType(char *header, TclWebRequest *req);

int TclWeb_PrintHeaders(TclWebRequest *req);

void TclWeb_OutputHeaderSet(char *header, char *val, TclWebRequest *req);
const char* TclWeb_OutputHeaderGet(char *header, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_HeaderAdd --
 * Adds an HTTP headers.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_HeaderAdd(char *header, char *val, TclWebRequest *req);

/*
 *-----------------------------------------------------------------------------
 *
 * TclWeb_SetStatus --
 * Sets status number for reply.
 *
 *-----------------------------------------------------------------------------
 */

int TclWeb_SetStatus(int status, TclWebRequest *req);

#endif

