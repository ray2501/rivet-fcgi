#ifndef TCL_WEB_H
#define TCL_WEB_H 1

#include <tcl.h>

#define VAR_SRC_QUERYSTRING 1
#define VAR_SRC_POST 2
#define VAR_SRC_ALL 3

/* Request Methods */
enum http_method {
    HTTP_GET = 0,
    HTTP_HEAD,
    HTTP_PUT,
    HTTP_POST,
    HTTP_DELETE,
    HTTP_PATCH
};

typedef struct RequestInfo {
    enum http_method method;
    Tcl_HashTable *query_string;
    Tcl_HashTable *post;
    int raw_length;
    char *raw_post;
} RequestInfo;

typedef struct TclWebRequest {
    Tcl_Interp *interp;
    RequestInfo *info;
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


int TclWeb_InitRequest(TclWebRequest *req, Tcl_Interp *interp, void *arg);
void TclWeb_FreeRequest(TclWebRequest *req);

int TclWeb_SendHeaders(TclWebRequest *req);
int TclWeb_SetHeaderType(char *header, TclWebRequest *req);
int TclWeb_PrintHeaders(TclWebRequest *req);
void TclWeb_OutputHeaderSet(char *header, char *val, TclWebRequest *req);
const char *TclWeb_OutputHeaderGet(char *header, TclWebRequest *req);
int TclWeb_HeaderAdd(char *header, char *val, TclWebRequest *req);
int TclWeb_SetStatus(int status, TclWebRequest *req);

int TclWeb_GetVar(Tcl_Obj *result, char *varname, int source, TclWebRequest *req);
int TclWeb_GetVarAsList(Tcl_Obj *result, char *varname, int source, TclWebRequest *req);
int TclWeb_VarExists(Tcl_Obj *result, char *varname, int source, TclWebRequest *req);
int TclWeb_VarNumber(Tcl_Obj *result, int source, TclWebRequest *req);
int TclWeb_GetVarNames(Tcl_Obj *result, int source, TclWebRequest *req);
int TclWeb_GetAllVars(Tcl_Obj *result, int source, TclWebRequest *req);

char *TclWeb_GetRawPost(TclWebRequest *req, int *len);

#endif
