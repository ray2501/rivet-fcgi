#include "tclWeb.h"
#include <string.h>
#include <fcgi_stdio.h>

#define DEFAULT_HEADER_TYPE "text/html; charset=utf-8"

int TclWeb_SendHeaders(TclWebRequest *req) {
    int status = 0;

    if (req == NULL)
        return TCL_ERROR;

    status = req->status;
    if (status != 0) {
        if (status == 200) {
            printf("Status: 200 OK\r\n");
        } else if(status == 500) {
            printf("Status: 500 Internal Server Error\r\n");
        } else {
            printf("Status: %d\r\n", status);
        }
    }

    if (req->headers) {
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;
        char *hashkey = NULL;
        char *hashvalue = NULL;

        for (entry = Tcl_FirstHashEntry(req->headers, &search);
            entry != NULL; entry = Tcl_NextHashEntry(&search)) {
            hashkey = (char *) Tcl_GetHashKey(req->headers, entry);
            hashvalue = (char *)Tcl_GetHashValue(entry);
            if (hashkey != NULL && hashvalue != NULL) {
                printf("%s: %s\r\n", hashkey, hashvalue);
            }
        }
    }

    printf("\r\n");

    return TCL_OK;
}

int TclWeb_SetHeaderType(char *header, TclWebRequest *req) {
    Tcl_HashEntry *entry = NULL;
    int isNew = 0;
    char *hashvalue = NULL;

    if (req == NULL)
        return TCL_ERROR;
 
    if (req->headers_set)
        return TCL_ERROR;

    entry = Tcl_FindHashEntry(req->headers, "Content-type");
    if (entry == NULL) {
        entry = Tcl_CreateHashEntry(req->headers, "Content-type", &isNew);
    }

    hashvalue = (char *)Tcl_Alloc(strlen(header) + 1);
    strcpy(hashvalue, header);
    Tcl_SetHashValue(entry, (ClientData) hashvalue);

    req->headers_set = 1;
    return TCL_OK;
}

int TclWeb_PrintHeaders(TclWebRequest *req) {
    if (req == NULL)
        return TCL_ERROR;

    if (req->headers_printed == 1)
        return TCL_ERROR;

    if (req->headers_set == 0)
    {
        TclWeb_SetHeaderType(DEFAULT_HEADER_TYPE, req);
    }

    TclWeb_SendHeaders(req);

    req->headers_printed = 1;
    return TCL_OK;
}

int TclWeb_SetStatus(int status, TclWebRequest *req) {
    if (req == NULL)
        return TCL_ERROR;

    req->status = status;
    return TCL_OK;
}
