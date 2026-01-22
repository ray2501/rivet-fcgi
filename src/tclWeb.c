#include "tclWeb.h"
#include "dataParser.h"
#include "helputils.h"
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_HEADER_TYPE "text/html; charset=utf-8"

/*
 * -----------------------------------------------------------------------------
 *
 * TclWeb_InitRequest --
 * Initializes the request structure.
 *
 *-----------------------------------------------------------------------------
 */
int TclWeb_InitRequest(TclWebRequest *req, Tcl_Interp *interp, void *arg) {
    char *request_method = NULL;
    char *query_string = NULL;
    char *content_length = NULL;
    char *content_type = NULL;

    req->interp = interp;

    req->info = (RequestInfo *)Tcl_Alloc(sizeof(RequestInfo));
    req->info->method = HTTP_GET;
    req->info->query_string = NULL;
    req->info->post = NULL;
    req->info->upload = NULL;
    req->info->raw_length = 0;
    req->info->raw_post = NULL;

    request_method = getenv("REQUEST_METHOD");
    if (request_method == NULL) {
        // Set the value to HTTP_GET (maybe debug in console)
        req->info->method = HTTP_GET;
    } else if (!strcmp(request_method, "GET")) {
        req->info->method = HTTP_GET;
    } else if (!strcmp(request_method, "HEAD")) {
        req->info->method = HTTP_HEAD;
    } else if (!strcmp(request_method, "PUT")) {
        req->info->method = HTTP_PUT;
    } else if (!strcmp(request_method, "POST")) {
        req->info->method = HTTP_POST;
    } else if (!strcmp(request_method, "DELETE")) {
        req->info->method = HTTP_DELETE;
    } else if (!strcmp(request_method, "PATCH")) {
        req->info->method = HTTP_PATCH;
    }

    query_string = getenv("QUERY_STRING");

    if (query_string == NULL || strlen(query_string) < 1) {
        req->info->query_string = NULL;
    } else {
        req->info->query_string =
            (Tcl_HashTable *)Tcl_Alloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(req->info->query_string, TCL_STRING_KEYS);

        ParseQueryString(interp, req->info->query_string, query_string);
    }

    /*
     * Need more chechk for this
     */
    if (req->info->method != HTTP_GET) {
        content_length = getenv("CONTENT_LENGTH");
        if (content_length) {
            req->info->raw_length = atol(content_length);
            req->info->raw_post = Tcl_Alloc(req->info->raw_length + 1);

            if (req->info->raw_post) {
                size_t bytes_read =
                    fread(req->info->raw_post, 1, req->info->raw_length, stdin);

                if (bytes_read == req->info->raw_length) {
                    req->info->raw_post[req->info->raw_length] = '\0';
                } else {
                    LogMessage("Read POST body failed.");
                    if (req->info->raw_post)
                        Tcl_Free(req->info->raw_post);
                    req->info->raw_post = NULL;
                    req->info->raw_length = 0;
                }
            }
        }

        if (req->info->raw_post) {
            content_type = getenv("CONTENT_TYPE");
            if (content_type == NULL || strlen(content_type) < 1) {
                req->info->post = NULL;
            } else {
                if (strncmp(content_type, DEFAULT_POST_CONTENT_TYPE,
                            strlen(DEFAULT_POST_CONTENT_TYPE)) == 0) {
                    req->info->post =
                        (Tcl_HashTable *)Tcl_Alloc(sizeof(Tcl_HashTable));
                    Tcl_InitHashTable(req->info->post, TCL_STRING_KEYS);

                    ParseQueryString(interp, req->info->post,
                                     req->info->raw_post);
                } else if (strncmp(content_type, MULTIPART_CONTENT_TYPE,
                                   strlen(MULTIPART_CONTENT_TYPE)) == 0) {
                    int len = 0;
                    char *boundary = GetBoundary(content_type, &len);

                    if (boundary) {
                        req->info->post =
                            (Tcl_HashTable *)Tcl_Alloc(sizeof(Tcl_HashTable));
                        Tcl_InitHashTable(req->info->post, TCL_STRING_KEYS);

                        req->info->upload =
                            (Tcl_HashTable *)Tcl_Alloc(sizeof(Tcl_HashTable));
                        Tcl_InitHashTable(req->info->upload, TCL_STRING_KEYS);

                        ParseMultiPart(interp, boundary, req->info->post,
                                       req->info->upload, req->info->raw_post,
                                       req->info->raw_length);
                    }
                } else {
                    req->info->post = NULL;
                }
            }
        }
    }

    req->headers = (Tcl_HashTable *)Tcl_Alloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(req->headers, TCL_STRING_KEYS);

    req->headers_printed = 0;
    req->headers_set = 0;

    if (req->info->method == HTTP_HEAD) {
        req->content_sent = 1;
    } else {
        req->content_sent = 0;
    }

    req->status = 0;
    return TCL_OK;
}

void TclWeb_FreeRequest(TclWebRequest *req, Tcl_Interp *interp) {
    if (req) {
        if (req->info) {
            if (req->info->query_string) {
                Tcl_Obj *hashvalue = NULL;
                Tcl_HashEntry *entry = NULL;
                Tcl_HashSearch search;

                for (entry =
                         Tcl_FirstHashEntry(req->info->query_string, &search);
                     entry != NULL; entry = Tcl_NextHashEntry(&search)) {
                    hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                    if (hashvalue) {
                        Tcl_DecrRefCount(hashvalue);
                        hashvalue = NULL;
                    }

                    Tcl_DeleteHashEntry(entry);
                }

                Tcl_DeleteHashTable(req->info->query_string);
                Tcl_Free((char *)req->info->query_string);
            }

            if (req->info->post) {
                Tcl_Obj *hashvalue = NULL;
                Tcl_HashEntry *entry = NULL;
                Tcl_HashSearch search;

                for (entry = Tcl_FirstHashEntry(req->info->post, &search);
                     entry != NULL; entry = Tcl_NextHashEntry(&search)) {
                    hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                    if (hashvalue) {
                        Tcl_DecrRefCount(hashvalue);
                        hashvalue = NULL;
                    }

                    Tcl_DeleteHashEntry(entry);
                }

                Tcl_DeleteHashTable(req->info->post);
                Tcl_Free((char *)req->info->post);
            }

            if (req->info->upload) {
                Tcl_Obj *hashvalue = NULL;
                Tcl_HashEntry *entry = NULL;
                Tcl_HashSearch search;

                for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
                     entry != NULL; entry = Tcl_NextHashEntry(&search)) {
                    hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                    if (hashvalue) {
                        /*
                         * Try to remove temp file
                         */
                        Tcl_Obj *tmpfile_obj = NULL;

                        Tcl_DictObjGet(interp, hashvalue,
                                       Tcl_NewStringObj("tmp_name", -1),
                                       &tmpfile_obj);
                        Tcl_FSDeleteFile(tmpfile_obj);

                        Tcl_DecrRefCount(hashvalue);
                        hashvalue = NULL;
                    }

                    Tcl_DeleteHashEntry(entry);
                }

                Tcl_DeleteHashTable(req->info->upload);
                Tcl_Free((char *)req->info->upload);
            }

            if (req->info->raw_post)
                Tcl_Free(req->info->raw_post);

            Tcl_Free((char *)req->info);
        }

        if (req->headers) {
            char *hashvalue = NULL;
            Tcl_HashEntry *entry = NULL;
            Tcl_HashSearch search;

            for (entry = Tcl_FirstHashEntry(req->headers, &search);
                 entry != NULL; entry = Tcl_NextHashEntry(&search)) {
                hashvalue = (char *)Tcl_GetHashValue(entry);
                if (hashvalue)
                    Tcl_Free(hashvalue);

                Tcl_DeleteHashEntry(entry);
            }

            Tcl_DeleteHashTable(req->headers);
            Tcl_Free((char *)req->headers);
        }

        Tcl_Free((char *)req);
    }
}

/*
 * -----------------------------------------------------------------------------
 *
 * TclWeb_SendHeaders --
 * Sends HTTP headers.
 *
 * Results:
 * HTTP headers output.  Things like cookies may no longer be manipulated.
 *
 *-----------------------------------------------------------------------------
 */
int TclWeb_SendHeaders(TclWebRequest *req) {
    int status = 0;

    if (req == NULL)
        return TCL_ERROR;

    status = req->status;
    if (status != 0) {
        if (status == 200) {
            printf("Status: 200 OK\r\n");
        } else if (status == 301) {
            printf("Status: 301 Moved Permanently\r\n");
        } else if (status == 302) {
            printf("Status: 302 Found\r\n");
        } else if (status == 404) {
            printf("Status: 404 Not Found\r\n");
        } else if (status == 500) {
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

        for (entry = Tcl_FirstHashEntry(req->headers, &search); entry != NULL;
             entry = Tcl_NextHashEntry(&search)) {
            hashkey = (char *)Tcl_GetHashKey(req->headers, entry);
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
    Tcl_SetHashValue(entry, (ClientData)hashvalue);

    req->headers_set = 1;
    return TCL_OK;
}

int TclWeb_PrintHeaders(TclWebRequest *req) {
    if (req == NULL)
        return TCL_ERROR;

    if (req->headers_printed == 1)
        return TCL_ERROR;

    if (req->headers_set == 0) {
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
        Tcl_SetHashValue(entry, (ClientData)hashvalue);
    }
}

const char *TclWeb_OutputHeaderGet(char *header, TclWebRequest *req) {
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

/*
 * -----------------------------------------------------------------------------
 *
 * TclWeb_HeaderAdd --
 * Adds an HTTP headers.
 *
 *-----------------------------------------------------------------------------
 */
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
        Tcl_SetHashValue(entry, (ClientData)hashvalue);
    } else {
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 * -----------------------------------------------------------------------------
 *
 * TclWeb_SetStatus --
 * Sets status number for reply.
 *
 *-----------------------------------------------------------------------------
 */
int TclWeb_SetStatus(int status, TclWebRequest *req) {
    if (req == NULL)
        return TCL_ERROR;

    req->status = status;
    return TCL_OK;
}

int TclWeb_GetVar(Tcl_Obj *result, char *varname, int source,
                  TclWebRequest *req) {
    int flag = 0;
    if (source == VAR_SRC_QUERYSTRING || source == VAR_SRC_ALL) {
        if (req->info->query_string) {
            Tcl_HashEntry *entry = NULL;
            entry = Tcl_FindHashEntry(req->info->query_string, varname);
            if (entry) {
                Tcl_Obj *hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                Tcl_Size listObjc;
                Tcl_Obj **listObjv;
                Tcl_Size i;

                if (Tcl_ListObjGetElements(req->interp, hashvalue, &listObjc,
                                           &listObjv) != TCL_OK) {
                    return TCL_ERROR;
                }

                for (i = 0; i < listObjc; i++) {
                    char *element = Tcl_GetString(listObjv[i]);

                    if (flag == 0) {
                        Tcl_SetStringObj(result, element, -1);
                        flag = 1;
                    } else {
                        Tcl_Obj *tmpobj;
                        Tcl_Obj *tmpobjv[2];
                        tmpobjv[0] = result;
                        tmpobjv[1] = Tcl_NewStringObj(element, -1);
                        tmpobj = Tcl_ConcatObj(2, tmpobjv);
                        Tcl_SetStringObj(result, Tcl_GetString(tmpobj), -1);
                    }
                }
            }
        }
    }

    if (source == VAR_SRC_POST || source == VAR_SRC_ALL) {
        if (req->info->post) {
            Tcl_HashEntry *entry = NULL;
            entry = Tcl_FindHashEntry(req->info->post, varname);
            if (entry) {
                Tcl_Obj *hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                Tcl_Size listObjc;
                Tcl_Obj **listObjv;
                Tcl_Size i;

                if (Tcl_ListObjGetElements(req->interp, hashvalue, &listObjc,
                                           &listObjv) != TCL_OK) {
                    return TCL_ERROR;
                }

                for (i = 0; i < listObjc; i++) {
                    char *element = Tcl_GetString(listObjv[i]);

                    if (flag == 0) {
                        Tcl_SetStringObj(result, element, -1);
                        flag = 1;
                    } else {
                        Tcl_Obj *tmpobj;
                        Tcl_Obj *tmpobjv[2];
                        tmpobjv[0] = result;
                        tmpobjv[1] = Tcl_NewStringObj(element, -1);
                        tmpobj = Tcl_ConcatObj(2, tmpobjv);
                        Tcl_SetStringObj(result, Tcl_GetString(tmpobj), -1);
                    }
                }
            }
        }
    }

    if (result->length == 0) {
        return TCL_ERROR;
    }

    return TCL_OK;
}

int TclWeb_GetVarAsList(Tcl_Obj *result, char *varname, int source,
                        TclWebRequest *req) {
    Tcl_Size length = 0;

    if (source == VAR_SRC_QUERYSTRING || source == VAR_SRC_ALL) {
        if (req->info->query_string) {
            Tcl_HashEntry *entry = NULL;
            entry = Tcl_FindHashEntry(req->info->query_string, varname);
            if (entry) {
                Tcl_Obj *hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                Tcl_Size listObjc;
                Tcl_Obj **listObjv;
                Tcl_Size i;

                if (Tcl_ListObjGetElements(req->interp, hashvalue, &listObjc,
                                           &listObjv) != TCL_OK) {
                    return TCL_ERROR;
                }

                for (i = 0; i < listObjc; i++) {
                    char *element = Tcl_GetString(listObjv[i]);

                    Tcl_ListObjAppendElement(req->interp, result,
                                             Tcl_NewStringObj(element, -1));
                }
            }
        }
    }

    if (source == VAR_SRC_POST || source == VAR_SRC_ALL) {
        if (req->info->post) {
            Tcl_HashEntry *entry = NULL;
            entry = Tcl_FindHashEntry(req->info->post, varname);
            if (entry) {
                Tcl_Obj *hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                Tcl_Size listObjc;
                Tcl_Obj **listObjv;
                Tcl_Size i;

                if (Tcl_ListObjGetElements(req->interp, hashvalue, &listObjc,
                                           &listObjv) != TCL_OK) {
                    return TCL_ERROR;
                }

                for (i = 0; i < listObjc; i++) {
                    char *element = Tcl_GetString(listObjv[i]);

                    Tcl_ListObjAppendElement(req->interp, result,
                                             Tcl_NewStringObj(element, -1));
                }
            }
        }
    }

    Tcl_ListObjLength(req->interp, result, &length);
    if (length == 0) {
        return TCL_ERROR;
    }

    return TCL_OK;
}

int TclWeb_VarExists(Tcl_Obj *result, char *varname, int source,
                     TclWebRequest *req) {

    if (source == VAR_SRC_QUERYSTRING || source == VAR_SRC_ALL) {
        if (req->info->query_string) {
            char *hashkey = NULL;
            Tcl_HashEntry *entry = NULL;
            Tcl_HashSearch search;

            for (entry = Tcl_FirstHashEntry(req->info->query_string, &search);
                 entry != NULL; entry = Tcl_NextHashEntry(&search)) {

                hashkey = Tcl_GetHashKey(req->info->query_string, entry);
                if (strcmp(hashkey, varname) == 0) {
                    Tcl_SetIntObj(result, 1);
                    return TCL_OK;
                }
            }
        }
    }

    if (source == VAR_SRC_POST || source == VAR_SRC_ALL) {
        if (req->info->post) {
            char *hashkey = NULL;
            Tcl_HashEntry *entry = NULL;
            Tcl_HashSearch search;

            for (entry = Tcl_FirstHashEntry(req->info->post, &search);
                 entry != NULL; entry = Tcl_NextHashEntry(&search)) {

                hashkey = Tcl_GetHashKey(req->info->post, entry);
                if (strcmp(hashkey, varname) == 0) {
                    Tcl_SetIntObj(result, 1);
                    return TCL_OK;
                }
            }
        }
    }

    Tcl_SetIntObj(result, 0);
    return TCL_OK;
}

int TclWeb_VarNumber(Tcl_Obj *result, int source, TclWebRequest *req) {
    int count = 0;

    if (source == VAR_SRC_QUERYSTRING) {
        if (req->info->query_string) {
            Tcl_SetIntObj(result, req->info->query_string->numEntries);
        } else {
            Tcl_SetIntObj(result, 0);
        }
    } else if (source == VAR_SRC_POST) {
        if (req->info->post) {
            Tcl_SetIntObj(result, req->info->post->numEntries);
        } else {
            Tcl_SetIntObj(result, 0);
        }
    } else {
        count = 0;

        if (req->info->query_string) {
            count = count + req->info->query_string->numEntries;
        }

        if (req->info->post) {
            count = count + req->info->post->numEntries;
        }

        Tcl_SetIntObj(result, count);
    }

    return TCL_OK;
}

int TclWeb_GetVarNames(Tcl_Obj *result, int source, TclWebRequest *req) {
    Tcl_Size length = 0;

    if (source == VAR_SRC_QUERYSTRING || source == VAR_SRC_ALL) {
        if (req->info->query_string) {
            Tcl_HashEntry *entry = NULL;
            Tcl_HashSearch search;
            char *hashkey = NULL;

            for (entry = Tcl_FirstHashEntry(req->info->query_string, &search);
                 entry != NULL; entry = Tcl_NextHashEntry(&search)) {

                hashkey = Tcl_GetHashKey(req->info->query_string, entry);
                Tcl_ListObjAppendElement(req->interp, result,
                                         Tcl_NewStringObj(hashkey, -1));
            }
        }
    }

    if (source == VAR_SRC_POST || source == VAR_SRC_ALL) {
        if (req->info->post) {
            Tcl_HashEntry *entry = NULL;
            Tcl_HashSearch search;
            char *hashkey = NULL;

            for (entry = Tcl_FirstHashEntry(req->info->post, &search);
                 entry != NULL; entry = Tcl_NextHashEntry(&search)) {

                hashkey = Tcl_GetHashKey(req->info->post, entry);
                Tcl_ListObjAppendElement(req->interp, result,
                                         Tcl_NewStringObj(hashkey, -1));
            }
        }
    }

    Tcl_ListObjLength(req->interp, result, &length);
    if (length == 0) {
        return TCL_ERROR;
    }

    return TCL_OK;
}

int TclWeb_GetAllVars(Tcl_Obj *result, int source, TclWebRequest *req) {
    Tcl_Size length = 0;

    if (source == VAR_SRC_QUERYSTRING || source == VAR_SRC_ALL) {
        if (req->info->query_string) {
            Tcl_Obj *hashvalue = NULL;
            Tcl_HashEntry *entry = NULL;
            Tcl_HashSearch search;

            for (entry = Tcl_FirstHashEntry(req->info->query_string, &search);
                 entry != NULL; entry = Tcl_NextHashEntry(&search)) {

                Tcl_ListObjAppendElement(
                    req->interp, result,
                    Tcl_NewStringObj(
                        Tcl_GetHashKey(req->info->query_string, entry), -1));

                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                Tcl_ListObjAppendElement(req->interp, result, hashvalue);
            }
        }
    }

    if (source == VAR_SRC_POST || source == VAR_SRC_ALL) {
        if (req->info->post) {
            Tcl_Obj *hashvalue = NULL;
            Tcl_HashEntry *entry = NULL;
            Tcl_HashSearch search;

            for (entry = Tcl_FirstHashEntry(req->info->post, &search);
                 entry != NULL; entry = Tcl_NextHashEntry(&search)) {

                Tcl_ListObjAppendElement(
                    req->interp, result,
                    Tcl_NewStringObj(Tcl_GetHashKey(req->info->post, entry),
                                     -1));

                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                Tcl_ListObjAppendElement(req->interp, result, hashvalue);
            }
        }
    }

    Tcl_ListObjLength(req->interp, result, &length);
    if (length == 0) {
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 * -----------------------------------------------------------------------------
 *
 * TclWeb_GetRawPost --
 *
 *  Fetch the raw POST data from the request.
 *
 * Results:
 *  The data, or NULL if it's not a POST or there is no data.
 *
 * Side Effects:
 *  None.
 *
 *-----------------------------------------------------------------------------
 */
char *TclWeb_GetRawPost(TclWebRequest *req, int *len) {
    *len = req->info->raw_length;

    return req->info->raw_post;
}

int TclWeb_PrepareUpload(char *varname, TclWebRequest *req) {
    if (req->info->upload) {
        char *hashkey = NULL;
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {

            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            if (strcmp(hashkey, varname) == 0) {
                return TCL_OK;
            }
        }
    }

    return TCL_ERROR;
}

int TclWeb_UploadChannel(char *varname, TclWebRequest *req) {
    if (req->info->upload) {
        char *hashkey = NULL;
        Tcl_Obj *hashvalue = NULL;
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;
        Tcl_Obj *result;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {

            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            if (strcmp(hashkey, varname) == 0) {
                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                if (hashvalue) {
                    Tcl_Obj *name_obj = NULL;
                    char *name_str;
                    Tcl_Size namelength;

                    Tcl_DictObjGet(req->interp, hashvalue,
                                   Tcl_NewStringObj("tmp_name", -1), &name_obj);
                    name_str = Tcl_GetStringFromObj(name_obj, &namelength);

                    if (name_str && namelength > 0) {
                        Tcl_Channel chan;

                        chan =
                            Tcl_OpenFileChannel(req->interp, name_str, "r", 0);
                        if (chan == NULL) {
                            Tcl_AddErrorInfo(
                                req->interp,
                                "Error opening channel to uploaded data");
                            return TCL_ERROR;
                        }

                        if (Tcl_SetChannelOption(req->interp, chan,
                                                 "-translation",
                                                 "binary") == TCL_ERROR) {
                            Tcl_AddErrorInfo(
                                req->interp,
                                "Error setting channel option -translation");
                            return TCL_ERROR;
                        }

                        if (Tcl_SetChannelOption(req->interp, chan, "-encoding",
                                                 "iso8859-1") == TCL_ERROR) {
                            Tcl_AddErrorInfo(
                                req->interp,
                                "Error setting channel option -encoding");
                            return TCL_ERROR;
                        }

                        Tcl_RegisterChannel(req->interp, chan);

                        result = Tcl_NewObj();
                        Tcl_SetStringObj(result, Tcl_GetChannelName(chan), -1);
                        Tcl_SetObjResult(req->interp, result);
                        return TCL_OK;
                    }
                }
            }
        }
    }

    return TCL_ERROR;
}

int TclWeb_UploadTempname(char *varname, TclWebRequest *req) {
    if (req->info->upload) {
        char *hashkey = NULL;
        Tcl_Obj *hashvalue = NULL;
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {

            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            if (strcmp(hashkey, varname) == 0) {
                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                if (hashvalue) {
                    Tcl_Obj *tmp_obj = NULL;

                    Tcl_DictObjGet(req->interp, hashvalue,
                                   Tcl_NewStringObj("tmp_name", -1), &tmp_obj);

                    Tcl_SetObjResult(req->interp, tmp_obj);
                    return TCL_OK;
                }
            }
        }
    }

    return TCL_ERROR;
}

int TclWeb_UploadSave(char *varname, Tcl_Obj *filename, TclWebRequest *req) {
    if (req->info->upload) {
        char *hashkey = NULL;
        Tcl_Obj *hashvalue = NULL;
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {

            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            if (strcmp(hashkey, varname) == 0) {
                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                if (hashvalue) {
                    Tcl_Obj *file_obj = NULL;
                    int result;

                    Tcl_DictObjGet(req->interp, hashvalue,
                                   Tcl_NewStringObj("tmp_name", -1), &file_obj);
                    result = Tcl_FSCopyFile(file_obj, filename);

                    if (result != TCL_OK) {
                        Tcl_Obj *err = Tcl_ObjPrintf("Save failed");
                        Tcl_SetObjResult(req->interp, err);
                        return TCL_ERROR;
                    }

                    return TCL_OK;
                }
            }
        }
    }

    return TCL_ERROR;
}

int TclWeb_UploadData(char *varname, TclWebRequest *req) {
    if (req->info->upload) {
        char *hashkey = NULL;
        Tcl_Obj *hashvalue = NULL;
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;
        Tcl_Obj *result;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {

            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            if (strcmp(hashkey, varname) == 0) {
                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                if (hashvalue) {
                    Tcl_Obj *name_obj = NULL;
                    Tcl_Obj *size_obj = NULL;
                    char *name_str;
                    Tcl_Size namelength;
                    int filesize;

                    Tcl_DictObjGet(req->interp, hashvalue,
                                   Tcl_NewStringObj("tmp_name", -1), &name_obj);
                    name_str = Tcl_GetStringFromObj(name_obj, &namelength);

                    Tcl_DictObjGet(req->interp, hashvalue,
                                   Tcl_NewStringObj("size", -1), &size_obj);
                    Tcl_GetIntFromObj(req->interp, size_obj, &filesize);

                    if (name_str && namelength > 0) {
                        Tcl_Channel chan;

                        chan =
                            Tcl_OpenFileChannel(req->interp, name_str, "r", 0);
                        if (chan == NULL) {
                            Tcl_AddErrorInfo(
                                req->interp,
                                "Error opening channel to uploaded data");
                            return TCL_ERROR;
                        }

                        if (Tcl_SetChannelOption(req->interp, chan,
                                                 "-translation",
                                                 "binary") == TCL_ERROR) {
                            Tcl_AddErrorInfo(
                                req->interp,
                                "Error setting channel option -translation");
                            return TCL_ERROR;
                        }

                        if (Tcl_SetChannelOption(req->interp, chan, "-encoding",
                                                 "iso8859-1") == TCL_ERROR) {
                            Tcl_AddErrorInfo(
                                req->interp,
                                "Error setting channel option -encoding");
                            return TCL_ERROR;
                        }

                        result = Tcl_NewObj();
                        Tcl_ReadChars(chan, result, filesize, 0);
                        if (Tcl_Close(req->interp, chan) == TCL_ERROR) {
                            return TCL_ERROR;
                        }

                        Tcl_SetObjResult(req->interp, result);
                        return TCL_OK;
                    }
                }
            }
        }
    }

    return TCL_ERROR;
}

int TclWeb_UploadSize(char *varname, TclWebRequest *req) {
    if (req->info->upload) {
        char *hashkey = NULL;
        Tcl_Obj *hashvalue = NULL;
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {

            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            if (strcmp(hashkey, varname) == 0) {
                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                if (hashvalue) {
                    Tcl_Obj *size_obj = NULL;

                    Tcl_DictObjGet(req->interp, hashvalue,
                                   Tcl_NewStringObj("size", -1), &size_obj);

                    Tcl_SetObjResult(req->interp, size_obj);
                    return TCL_OK;
                }
            }
        }
    }

    return TCL_ERROR;
}

int TclWeb_UploadType(char *varname, TclWebRequest *req) {
    if (req->info->upload) {
        char *hashkey = NULL;
        Tcl_Obj *hashvalue = NULL;
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {

            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            if (strcmp(hashkey, varname) == 0) {
                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                if (hashvalue) {
                    Tcl_Obj *type_obj = NULL;

                    Tcl_DictObjGet(req->interp, hashvalue,
                                   Tcl_NewStringObj("type", -1), &type_obj);

                    Tcl_SetObjResult(req->interp, type_obj);
                    return TCL_OK;
                }
            }
        }
    }

    return TCL_ERROR;
}

int TclWeb_UploadFilename(char *varname, TclWebRequest *req) {
    if (req->info->upload) {
        char *hashkey = NULL;
        Tcl_Obj *hashvalue = NULL;
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {

            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            if (strcmp(hashkey, varname) == 0) {
                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                if (hashvalue) {
                    Tcl_Obj *file_obj = NULL;

                    Tcl_DictObjGet(req->interp, hashvalue,
                                   Tcl_NewStringObj("name", -1), &file_obj);

                    Tcl_SetObjResult(req->interp, file_obj);
                    return TCL_OK;
                }
            }
        }
    }

    return TCL_ERROR;
}

int TclWeb_UploadNames(TclWebRequest *req) {
    if (req->info->upload) {
        Tcl_Obj *result = Tcl_NewObj();
        Tcl_HashEntry *entry = NULL;
        Tcl_HashSearch search;
        char *hashkey = NULL;

        for (entry = Tcl_FirstHashEntry(req->info->upload, &search);
             entry != NULL; entry = Tcl_NextHashEntry(&search)) {
            hashkey = Tcl_GetHashKey(req->info->upload, entry);
            Tcl_ListObjAppendElement(req->interp, result,
                                     Tcl_NewStringObj(hashkey, -1));
        }

        Tcl_SetObjResult(req->interp, result);
        return TCL_OK;
    }

    return TCL_ERROR;
}
