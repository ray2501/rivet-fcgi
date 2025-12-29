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
        } else if(status == 301) {
            printf("Status: 301 Moved Permanently\r\n");
        } else if(status == 302) {
            printf("Status: 302 Found\r\n");
        } else if(status == 404) {
            printf("Status: 404 Not Found\r\n");
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

        printf("\r\n");
    }

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
    } else {
        hashvalue = (char *)Tcl_GetHashValue(entry);
        if (hashvalue)
            Tcl_Free(hashvalue);
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

void TclWeb_OutputHeaderSet(char *header, char *val, TclWebRequest *req) {
    if (req == NULL)
        return;

    if (req->headers) {
        Tcl_HashEntry *entry = NULL;
        char *hashvalue = NULL;
        int isNew = 0;

        entry = Tcl_FindHashEntry(req->headers, header);
        if (entry == NULL) {
            entry = Tcl_CreateHashEntry(req->headers, header, &isNew);
        } else {
            hashvalue = (char *)Tcl_GetHashValue(entry);
            if (hashvalue)
                Tcl_Free(hashvalue);
        }

        hashvalue = (char *)Tcl_Alloc(strlen(val) + 1);
        strcpy(hashvalue, val);
        Tcl_SetHashValue(entry, (ClientData) hashvalue);
    }
}

const char* TclWeb_OutputHeaderGet(char *header, TclWebRequest *req) {
    if (req == NULL)
        return NULL;

    if (req->headers) {
        Tcl_HashEntry *entry = NULL;
        const char *hashvalue = NULL;

        entry = Tcl_FindHashEntry(req->headers, header);
        if (entry == NULL) {
            return NULL;
        }

        hashvalue = (const char *)Tcl_GetHashValue(entry);
        if (hashvalue)
            return hashvalue;
    }

    return NULL;
}

int TclWeb_HeaderAdd(char *header, char *val, TclWebRequest *req) {
    if (req == NULL)
        return TCL_ERROR;

    if (req->headers) {
        Tcl_HashEntry *entry = NULL;
        char *hashvalue = NULL;
        int isNew = 0;

        entry = Tcl_CreateHashEntry(req->headers, header, &isNew);
        hashvalue = (char *)Tcl_Alloc(strlen(val) + 1);
        strcpy(hashvalue, val);
        Tcl_SetHashValue(entry, (ClientData) hashvalue);
    } else {
        return TCL_ERROR;
    }

    return TCL_OK;
}

int TclWeb_SetStatus(int status, TclWebRequest *req) {
    if (req == NULL)
        return TCL_ERROR;

    req->status = status;
    return TCL_OK;
}
