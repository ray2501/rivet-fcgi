#include "helputils.h"
#include <ctype.h>
#include <fcgi_stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * From https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
 * This function is to get filename extension.
 */
const char *FileSuffix(const char path[]) {
    const char *result;
    int i, n;

    if (path == NULL) {
        return NULL;
    }

    n = strlen(path);
    i = n - 1;
    while ((i > 0) && (path[i] != '.') && (path[i] != '/') &&
           (path[i] != '\\')) {
        i--;
    }

    if ((i > 0) && (i < n - 1) && (path[i] == '.') && (path[i - 1] != '/') &&
        (path[i - 1] != '\\')) {
        result = path + i;
    } else {
        result = path + n;
    }

    return result;
}

void get_timestamp(char *buffer, size_t buffer_size) {
    time_t timer;
    struct tm *tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void LogMessage(const char *format, ...) {
    char timestamp[26];
    get_timestamp(timestamp, sizeof(timestamp));

    fprintf(stderr, "[%s] ", timestamp);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

char HexToInt(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

char *DecodeUrlstring(const char *src) {
    char *dst = malloc(strlen(src) + 1);
    char *d = dst;

    while (*src) {
        if (*src == '%') {
            if (isxdigit(*(src + 1)) && isxdigit(*(src + 2))) {
                *d++ = HexToInt(*(src + 1)) * 16 + HexToInt(*(src + 2));
                src += 3;
            } else {
                // Invalid encoding, copy the '%' literal
                *d++ = *src++;
            }
        } else if (*src == '+') {
            *d++ = ' ';
            src++;
        } else {
            *d++ = *src++;
        }
    }

    *d = '\0'; // Null-terminate the destination string
    return dst;
}

void ParseQueryString(Tcl_Interp *interp, Tcl_HashTable *qs,
                      char *query_string) {
    const char outer_delimiters[] = "&";
    const char inner_delimiters[] = "=";

    char *token;
    char *outer_saveptr = NULL;
    char *inner_saveptr = NULL;

    token = strtok_r(query_string, outer_delimiters, &outer_saveptr);

    while (token != NULL) {
        char *inner_token = strtok_r(token, inner_delimiters, &inner_saveptr);
        Tcl_HashEntry *entry = NULL;
        int isNew = 0;
        Tcl_Obj *hashvalue = NULL;
        char *key = NULL;
        char *value = NULL;
        char *empty = "";

        if (inner_token) {
            key = DecodeUrlstring(inner_token);
            entry = Tcl_FindHashEntry(qs, key);
            if (entry == NULL) {
                entry = Tcl_CreateHashEntry(qs, key, &isNew);
                hashvalue = Tcl_NewListObj(1, NULL);
                Tcl_IncrRefCount(hashvalue);
            } else {
                hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
            }

            inner_token = strtok_r(NULL, inner_delimiters, &inner_saveptr);
            if (inner_token) {
                value = DecodeUrlstring(inner_token);

                Tcl_ListObjAppendElement(interp, hashvalue,
                                         Tcl_NewStringObj(value, -1));
                Tcl_SetHashValue(entry, (ClientData)hashvalue);

                free(value);
            } else {
                Tcl_ListObjAppendElement(interp, hashvalue,
                                         Tcl_NewStringObj(empty, -1));
                Tcl_SetHashValue(entry, (ClientData)hashvalue);
            }

            free(key);
        }

        token = strtok_r(NULL, outer_delimiters, &outer_saveptr);
    }
}
