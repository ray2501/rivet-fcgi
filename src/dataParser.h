#ifndef DATA_PARSER_H
#define DATA_PARSER_H 1

#include "helputils.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>

#define DEFAULT_POST_CONTENT_TYPE "application/x-www-form-urlencoded"
#define MULTIPART_CONTENT_TYPE "multipart/form-data"

void ParseQueryString(Tcl_Interp *interp, Tcl_HashTable *qs,
                      char *query_string);

char *GetBoundary(char *content_type, int *length);

void ParseMultiPart(Tcl_Interp *interp, char *boundary, Tcl_HashTable *qs,
                    Tcl_HashTable *upload, char *multipart,
                    int multipartlength);

#endif
