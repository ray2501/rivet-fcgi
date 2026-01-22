#include "dataParser.h"

#define MAXPATHLEN 256

/*
 * For DEFAULT_POST_CONTENT_TYPE
 */
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
    char *parseString = NULL;

    if (query_string == NULL)
        return;

    parseString = strdup(query_string);

    token = strtok_r(parseString, outer_delimiters, &outer_saveptr);

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

    if (parseString)
        free(parseString);
}

/*
 * For DEFAULT_POST_CONTENT_TYPE
 */
char *GetBoundary(char *content_type, int *length) {
    char *boundary = NULL, *boundary_end = NULL;
    int boundary_len = 0;

    *length = 0;

    boundary = strstr(content_type, "boundary");
    if (!boundary) {
        char *ct_lcase = strdup(content_type);

        for (int i = 0; ct_lcase[i] != '\0'; i++) {
            ct_lcase[i] = tolower((unsigned char)ct_lcase[i]);
        }

        boundary = strstr(ct_lcase, "boundary");
        if (boundary) {
            boundary = content_type + (boundary - ct_lcase);
        }

        free(ct_lcase);
    }

    if (!boundary || !(boundary = strchr(boundary, '='))) {
        LogMessage("Missing boundary in multipart/form-data data");
        return NULL;
    }

    boundary++;
    boundary_len = strlen(boundary);

    if (boundary[0] == '"') {
        boundary++;
        boundary_end = strchr(boundary, '"');
        if (!boundary_end) {
            LogMessage("Invalid boundary in multipart/form-data data");
            return NULL;
        }
    } else {
        boundary_end = strpbrk(boundary, ",;");
    }

    if (boundary_end) {
        boundary_end[0] = '\0';
        boundary_len = boundary_end - boundary;
    }

    if (boundary_len > 70) {
        LogMessage("Boundary too large in multipart/form-data data");
        return NULL;
    }

    *length = boundary_len;
    return boundary;
}

char *getword(char **line, char stop) {
    char *pos = *line;
    char *res;
    char quote;

    while (*pos && *pos != stop) {
        if ((quote = *pos) == '"' || quote == '\'') {
            ++pos;

            while (*pos && *pos != quote) {
                if (*pos == '\\' && pos[1] && pos[1] == quote) {
                    pos += 2;
                } else {
                    ++pos;
                }
            }

            if (*pos) {
                ++pos;
            }
        } else
            ++pos;
    }

    if (*pos == '\0') {
        res = strdup(*line);
        *line += strlen(*line);
        return res;
    }

    res = strndup(*line, pos - *line);

    while (*pos == stop) {
        ++pos;
    }

    *line = pos;
    return res;
}

char *substring_conf(char *start, int len, char quote) {
    char *result = malloc(len + 1);
    char *resp = result;
    int i;

    for (i = 0; i < len && start[i] != quote; ++i) {
        if (start[i] == '\\' &&
            (start[i + 1] == '\\' || (quote && start[i + 1] == quote))) {
            *resp++ = start[++i];
        } else {
            *resp++ = start[i];
        }
    }

    *resp = '\0';
    return result;
}

char *getword_conf(char *str) {
    while (*str && isspace(*str)) {
        ++str;
    }

    if (!*str) {
        return strdup("");
    }

    if (*str == '"' || *str == '\'') {
        char quote = *str;

        str++;
        return substring_conf(str, (int)strlen(str), quote);
    } else {
        char *strend = str;

        while (*strend && !isspace(*strend)) {
            ++strend;
        }

        return substring_conf(str, strend - str, 0);
    }
}

/*
 * Return an open file descriptor for the temp file
 */
int getTempFile(char **tempFileName) {
    char tmpName[MAXPATHLEN];
    int fd;

    char *tmpdir = getenv("TMP");
    if (!tmpdir) {
        tmpdir = getenv("TEMP");

        if (!tmpdir) {
            tmpdir = getenv("TMPDIR");

            if (!tmpdir) {
                tmpdir = "/tmp";
            }
        }
    }

    snprintf(tmpName, MAXPATHLEN, "%s/rivet_XXXXXX", tmpdir);

    fd = mkstemp(tmpName);
    *tempFileName = (char *)strdup(tmpName);

    return fd;
}

char *memstr(char *haystack, char *needle, int size) {
    char *p = NULL;
    char needlesize = 0;

    if (!haystack)
        return NULL;

    if (!needle)
        return NULL;

    if (size == 0)
        return NULL;

    needlesize = strlen(needle);
    for (p = haystack; p <= (haystack - needlesize + size); p++) {
        if (memcmp(p, needle, needlesize) == 0)
            return p;
    }

    return NULL;
}

void MultiPartHeaders(Tcl_Interp *interp, char **multipart, Tcl_Obj *headList) {
    char *lineEnd = NULL;

    while ((lineEnd = strchr(*multipart, '\n')) != NULL) {
        if (lineEnd) {
            int count = lineEnd - *multipart;

            if (count <= 2) { /* Found \n or \r\n in a line */
                *multipart = *multipart + count + 1;
                break;
            } else {
                char *line = NULL;
                char *key = NULL;
                char *value = NULL;
                int keycount = 0;

                line = (char *)calloc(1, count);
                memcpy(line, *multipart, count - 1);

                if (line[count - 2] == '\r') {
                    line[count - 2] = '\0';
                }

                value = strchr(line, ':');
                if (value) {
                    keycount = value - line + 1;
                    key = (char *)calloc(1, keycount);
                    memcpy(key, line, keycount - 1);

                    do {
                        value++;
                    } while (isspace(*value));

                    Tcl_ListObjAppendElement(interp, headList,
                                             Tcl_NewStringObj(key, -1));
                    Tcl_ListObjAppendElement(interp, headList,
                                             Tcl_NewStringObj(value, -1));

                    if (key)
                        free(key);
                } else {
                    Tcl_Size listLength = 0;
                    int len_res = TCL_OK;
                    len_res = Tcl_ListObjLength(interp, headList, &listLength);

                    if (len_res == TCL_OK) {
                        if (listLength == 0) {
                            continue;
                        } else {
                            Tcl_Obj *val_obj = NULL;

                            if (listLength % 2 == 0) {
                                Tcl_ListObjIndex(interp, headList,
                                                 listLength - 1, &val_obj);
                                Tcl_AppendToObj(val_obj, value, -1);
                            }
                        }
                    }
                }

                if (line)
                    free(line);

                *multipart = *multipart + count + 1;
            }
        }
    }
}

void ParseMultiPart(Tcl_Interp *interp, char *boundary, Tcl_HashTable *qs,
                    Tcl_HashTable *upload, char *multipart, int partSize) {
    char *curBoundary = NULL;
    char *nextBoundary = NULL;
    int curLength = 0;
    int nextLength = 0;
    char *org_multipart = multipart;

    curLength = strlen("--") + strlen(boundary) + 1;
    curBoundary = (char *)calloc(1, curLength);
    snprintf(curBoundary, curLength, "--%s", boundary);

    nextLength = strlen("\r\n--") + strlen(boundary) + 1;
    nextBoundary = (char *)calloc(1, nextLength);
    snprintf(nextBoundary, nextLength, "\r\n--%s", boundary);

    char *loc = NULL;
    int size = partSize;
    while ((loc = memstr(multipart, curBoundary, size)) != NULL) {
        char *param = NULL;
        char *filename = NULL;
        char *filetype = NULL;
        char *tmpfile = NULL;
        int filesize = 0;

        multipart = multipart + (loc - multipart) + strlen(curBoundary);
        if (multipart[0] == '\r' && multipart[1] == '\n') {
            multipart = multipart + 2;
        } else if (multipart[0] == '-' && multipart[1] == '-') {
            break;
        }

        /*
         * Parse header
         */
        Tcl_Obj *headList = Tcl_NewObj();
        Tcl_IncrRefCount(headList);

        MultiPartHeaders(interp, &multipart, headList);

        /*
         * Get data from header
         */

        Tcl_Size list_len = 0;
        int result = TCL_OK;
        result = Tcl_ListObjLength(interp, headList, &list_len);
        if (result == TCL_OK) {
            if (list_len > 0 && list_len % 2 == 0) {
                for (int index = 0; index < list_len; index += 2) {
                    Tcl_Obj *key_obj = NULL;
                    Tcl_Size key_len = 0;
                    char *keyString = NULL;

                    Tcl_ListObjIndex(interp, headList, index, &key_obj);
                    keyString = Tcl_GetStringFromObj(key_obj, &key_len);

                    if (strcmp("Content-Disposition", keyString) == 0) {
                        Tcl_Obj *value_obj = NULL;
                        Tcl_Size value_len = 0;
                        char *cd = NULL;
                        char *pair = NULL;

                        Tcl_ListObjIndex(interp, headList, index + 1,
                                         &value_obj);
                        cd = Tcl_GetStringFromObj(value_obj, &value_len);
                        while (isspace(*cd)) {
                            ++cd;
                        }

                        while (*cd && (pair = getword(&cd, ';'))) {
                            char *key = NULL, *word = pair;

                            while (isspace(*cd)) {
                                ++cd;
                            }

                            if (strchr(pair, '=')) {
                                key = getword(&pair, '=');

                                if (!strcasecmp(key, "name")) {
                                    param = getword_conf(pair);
                                } else if (!strcasecmp(key, "filename")) {
                                    filename = getword_conf(pair);
                                }
                            }

                            if (key)
                                free(key);
                            if (word)
                                free(word);
                        }
                    } else if (strcmp("Content-Type", keyString) == 0) {
                        Tcl_Obj *value_obj = NULL;
                        Tcl_Size value_len = 0;
                        char *cd = NULL;

                        Tcl_ListObjIndex(interp, headList, index + 1,
                                         &value_obj);
                        cd = Tcl_GetStringFromObj(value_obj, &value_len);

                        while (isspace(*cd)) {
                            ++cd;
                        }

                        filetype = strdup(cd);
                    }
                }
            }
        }

        /*
         * Handle adata and update Tcl_HashTable
         */
        char *nextBoundaryLoc = NULL;
        char *nextPtr = NULL;
        int nextBoundaryCount = 0;

        if (param && filename) {
            int curPartLength = partSize - (multipart - org_multipart);
            int fd = -1;

            if (filename[0] == '\0') {
                /*
                 * User does not send file!
                 */
                goto clean;
            }

            if ((nextBoundaryLoc =
                     memstr(multipart, nextBoundary, curPartLength)) != NULL) {
                nextBoundaryCount = nextBoundaryLoc - multipart;
                nextPtr = (char *)malloc(nextBoundaryCount * sizeof(char));

                if (nextPtr) {
                    Tcl_HashEntry *entry = NULL;
                    Tcl_Obj *hashvalue = NULL;
                    int isNew = 0;

                    filesize = nextBoundaryCount;
                    memcpy(nextPtr, multipart, nextBoundaryCount);

                    fd = getTempFile(&tmpfile);
                    if (fd < 0) {
                        if (nextPtr)
                            free(nextPtr);

                        LogMessage("Creation of temp file failed.");
                        goto clean;
                    }

                    write(fd, nextPtr, nextBoundaryCount);
                    close(fd);

                    entry = Tcl_CreateHashEntry(upload, param, &isNew);
                    hashvalue = Tcl_NewDictObj();
                    Tcl_IncrRefCount(hashvalue);

                    Tcl_DictObjPut(interp, hashvalue,
                                   Tcl_NewStringObj("name", -1),
                                   Tcl_NewStringObj(filename, -1));

                    if (filetype) {
                        Tcl_DictObjPut(interp, hashvalue,
                                       Tcl_NewStringObj("type", -1),
                                       Tcl_NewStringObj(filetype, -1));
                    } else {
                        Tcl_DictObjPut(interp, hashvalue,
                                       Tcl_NewStringObj("type", -1),
                                       Tcl_NewStringObj("", -1));
                    }

                    Tcl_DictObjPut(interp, hashvalue,
                                   Tcl_NewStringObj("tmp_name", -1),
                                   Tcl_NewStringObj(tmpfile, -1));
                    Tcl_DictObjPut(interp, hashvalue,
                                   Tcl_NewStringObj("size", -1),
                                   Tcl_NewIntObj(filesize));

                    Tcl_SetHashValue(entry, (ClientData)hashvalue);

                    if (nextPtr)
                        free(nextPtr);
                }
            } else {
                LogMessage("I cannot find next boundary!");
            }
        } else if (param && !filename) {
            Tcl_HashEntry *entry = NULL;
            Tcl_Obj *hashvalue = NULL;
            int isNew = 0;
            char *empty = "";
            int curPartLength = partSize - (multipart - org_multipart);

            if ((nextBoundaryLoc =
                     memstr(multipart, nextBoundary, curPartLength)) != NULL) {
                nextBoundaryCount = nextBoundaryLoc - multipart + 1;
                nextPtr = (char *)calloc(1, nextBoundaryCount);

                if (nextPtr) {
                    memcpy(nextPtr, multipart, nextBoundaryCount - 1);

                    entry = Tcl_FindHashEntry(qs, param);
                    if (entry == NULL) {
                        entry = Tcl_CreateHashEntry(qs, param, &isNew);
                        hashvalue = Tcl_NewListObj(1, NULL);
                        Tcl_IncrRefCount(hashvalue);
                    } else {
                        hashvalue = (Tcl_Obj *)Tcl_GetHashValue(entry);
                    }

                    if (nextPtr && strlen(nextPtr) > 0) {
                        Tcl_ListObjAppendElement(interp, hashvalue,
                                                 Tcl_NewStringObj(nextPtr, -1));
                        Tcl_SetHashValue(entry, (ClientData)hashvalue);
                    } else {
                        Tcl_ListObjAppendElement(interp, hashvalue,
                                                 Tcl_NewStringObj(empty, -1));
                        Tcl_SetHashValue(entry, (ClientData)hashvalue);
                    }

                    if (nextPtr)
                        free(nextPtr);
                }
            } else {
                LogMessage("No filename and I cannot find next boundary!");
            }
        }

    clean:
        if (param)
            free(param);
        if (filename)
            free(filename);
        if (filetype)
            free(filetype);
        if (tmpfile)
            free(tmpfile);

        if (headList) {
            Tcl_DecrRefCount(headList);
            headList = NULL;
        }

        size = partSize - (multipart - org_multipart);
    }

    if (curBoundary)
        free(curBoundary);
    if (nextBoundary)
        free(nextBoundary);
}
