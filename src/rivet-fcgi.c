#include "channel.h"
#include "config.h"
#include "errChan.h"
#include "helputils.h"
#include "rivetCore.h"
#include "rivetParser.h"
#include "tclWeb.h"
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>

#define TCL_MAX_CHANNEL_BUFFER_SIZE (1024 * 1024)

void FreeGlobalsData(ClientData clientData, Tcl_Interp *interp) {
    interp_globals *globals = clientData;

    if (globals) {
        if (globals->scriptfile)
            Tcl_Free(globals->scriptfile);

        if (globals->req) {
            if (globals->req->headers) {
                char *hashvalue = NULL;
                Tcl_HashEntry *entry = NULL;
                Tcl_HashSearch search;

                for (entry = Tcl_FirstHashEntry(globals->req->headers, &search);
                     entry != NULL; entry = Tcl_NextHashEntry(&search)) {
                    hashvalue = (char *)Tcl_GetHashValue(entry);
                    if (hashvalue)
                        Tcl_Free(hashvalue);

                    Tcl_DeleteHashEntry(entry);
                }

                Tcl_DeleteHashTable(globals->req->headers);
                Tcl_Free((char *)globals->req->headers);
            }

            Tcl_Free((char *)globals->req);
        }

        Tcl_Free((char *)globals);
    }
}

/*
 * Only declare, I do not really test on Windows platform
 */
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

int main(int argc, char *argv[]) {
    int ret = EXIT_SUCCESS;

    Tcl_FindExecutable(argv[0]);

    /*
     * Response loop
     */
    while (FCGI_Accept() >= 0) {
        Tcl_Interp *interp = NULL;
        interp_globals *globals = NULL;
        Tcl_Obj *script = NULL;
        Tcl_Obj *pathPtr = NULL;
        Tcl_Obj *request_init = NULL;
        char *filename = NULL;
        const char *file_ext = NULL;
        int result;

        interp = Tcl_CreateInterp();

        if (interp == NULL) {
            char *msg = "rivet-fcgi: Create Tcl interpreter failed.\n";
#ifdef _WIN32
            _write(STDOUT_FILENO, msg, sizeof(msg) - 1);
#else
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
#endif

            ret = EXIT_FAILURE;
            goto end;
        }

        if (Tcl_Init(interp) != TCL_OK) {
            char *msg = "rivet-fcgi: Tcl_init failed.\n";
#ifdef _WIN32
            _write(STDOUT_FILENO, msg, sizeof(msg) - 1);
#else
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
#endif

            ret = EXIT_FAILURE;
            goto end;
        }

        if (Rivet_InitCore(interp) != TCL_OK) {
            char *msg = "rivet-fcgi: Rivet_InitCore failed.\n";
#ifdef _WIN32
            _write(STDOUT_FILENO, msg, sizeof(msg) - 1);
#else
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
#endif

            ret = EXIT_FAILURE;
            goto end;
        }

        if (Tcl_PkgRequire(interp, "Rivet", RIVET_INIT_VERSION, 1) == NULL) {
            char *msg = "rivet-fcgi: Tcl_PkgRequire Rivet failed.\n";
#ifdef _WIN32
            _write(STDOUT_FILENO, msg, sizeof(msg) - 1);
#else
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
#endif

            ret = EXIT_FAILURE;
            goto end;
        }

        globals = (interp_globals *)Tcl_Alloc(sizeof(interp_globals));
        globals->scriptfile = NULL;
        globals->req = (TclWebRequest *)Tcl_Alloc(sizeof(TclWebRequest));
        globals->req->headers =
            (Tcl_HashTable *)Tcl_Alloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(globals->req->headers, TCL_STRING_KEYS);

        globals->req->headers_printed = 0;
        globals->req->headers_set = 0;
        globals->req->content_sent = 0;
        globals->req->status = 0;
        Tcl_SetAssocData(interp, "rivet", FreeGlobalsData, globals);

        Tcl_Preserve((ClientData)interp);

        /*
         * We need redfine stdout and stderr to output result.
         * Create a channel to do this thing.
         */
        Tcl_Channel m_Out = Tcl_CreateChannel(&TclFCGIChan, "fcgiout",
                                              (ClientData)interp, TCL_WRITABLE);

        Tcl_SetChannelOption(NULL, m_Out, "-encoding", "utf-8");

        Tcl_SetStdChannel(m_Out, TCL_STDOUT);
        Tcl_SetChannelBufferSize(m_Out, TCL_MAX_CHANNEL_BUFFER_SIZE);
        Tcl_RegisterChannel(interp, m_Out);

        Tcl_Channel m_Err = Tcl_CreateChannel(&TclFCGIErrChan, "fcgierr",
                                              (ClientData)interp, TCL_WRITABLE);

        Tcl_SetChannelOption(NULL, m_Err, "-translation", "lf");
        Tcl_SetChannelOption(NULL, m_Err, "-buffering", "none");
        Tcl_SetChannelOption(NULL, m_Err, "-encoding", "utf-8");

        Tcl_SetStdChannel(m_Err, TCL_STDERR);
        Tcl_RegisterChannel(interp, m_Err);

        if (argc <= 1) {
            filename = getenv("SCRIPT_FILENAME");
            if (filename == NULL) {
                fprintf(stderr, "rivet-fcgi: No filename.\n");
                ret = EXIT_FAILURE;
                goto end;
            }
        } else if (argc == 2) {
            if (strcmp(argv[1], "--version") == 0) {
                fprintf(stdout, "%s\n", VERSION);
                ret = EXIT_SUCCESS;
                goto end;
            } else {
                filename = argv[1];
            }
        } else {
            fprintf(stderr, "rivet-fcgi: Too many arguments.\n");
            ret = EXIT_FAILURE;
            goto end;
        }

        /*
         * It is necessary to fix proxy URLs from Apache mod_proxy_fcgi and
         * mod_proxy_balancer.
         */
        if (filename &&
            strncasecmp(filename, APACHE_PROXY_FCGI_PREFIX,
                        sizeof(APACHE_PROXY_FCGI_PREFIX) - 1) == 0) {
            char *p = filename + (sizeof(APACHE_PROXY_FCGI_PREFIX) - 1);
            while (*p != '\0' && *p != '/') {
                p++;
            }
            if (*p != '\0') {
                memmove(filename, p, strlen(p) + 1);
            }

            /* for query string, skip it. */
            p = strchr(filename, '?');
            if (p) {
                *p = 0;
            }
        }

        if (filename &&
            strncasecmp(filename, APACHE_PROXY_BALANCER_PREFIX,
                        sizeof(APACHE_PROXY_BALANCER_PREFIX) - 1) == 0) {
            char *p = filename + (sizeof(APACHE_PROXY_BALANCER_PREFIX) - 1);
            while (*p != '\0' && *p != '/') {
                p++;
            }
            if (*p != '\0') {
                memmove(filename, p, strlen(p) + 1);
            }

            /* for query string, skip it. */
            p = strchr(filename, '?');
            if (p) {
                *p = 0;
            }
        }

        /*
         * Check file can read.
         */
        pathPtr = Tcl_NewStringObj(filename, -1);
        if (!pathPtr) {
            printf("Status: 500 Internal Server Error\r\n");

            fprintf(stderr, "rivet-fcgi: Something is wrong.\n");
            ret = EXIT_FAILURE;
            goto end;
        }

        Tcl_IncrRefCount(pathPtr);
        if (Tcl_FSAccess(pathPtr, R_OK)) {
            printf("Status: 500 Internal Server Error\r\n");

            fprintf(stderr, "rivet-fcgi: File cannot read.\n");
            fprintf(stderr, "rivet-fcgi: %s.\n", filename);
            Tcl_DecrRefCount(pathPtr);
            goto myclean;
        }
        Tcl_DecrRefCount(pathPtr);

        /*
         * Eval ::Rivet::initialize_request
         */
        request_init = Tcl_NewStringObj("::Rivet::initialize_request\n", -1);
        Tcl_IncrRefCount(request_init);

        if (Tcl_EvalObjEx(interp, request_init, 0) == TCL_ERROR)
        {
            printf("Status: 500 Internal Server Error\r\n");

            fprintf(stderr, "rivet-fcgi: execute initialize_request failed.\n");
            Tcl_DecrRefCount(request_init);
            goto myclean;
        }

        file_ext = FileSuffix(filename);

        script = Tcl_NewObj();
        Tcl_IncrRefCount(script);

        if (strcmp(file_ext, ".rvt") == 0) {
            result = Rivet_GetRivetFile(filename, script, interp);
        } else if (strcmp(file_ext, ".tcl") == 0) {
            result = Rivet_GetTclFile(filename, script, interp);
        } else {
            /*
             * Skip other types.
             */
            printf("Status: 500 Internal Server Error\r\n");

            fprintf(stderr, "rivet-fcgi: Wrong file type.\n");
            Tcl_DecrRefCount(script);
            goto myclean;
        }

        if (result != TCL_OK) {
            printf("Status: 500 Internal Server Error\r\n");

            fprintf(stderr, "rivet-fcgi: Could not read file %s.\n", filename);
            Tcl_DecrRefCount(script);
            goto myclean;
        }

        /*
         * Save the filename info to scriptfile
         */
        // Tcl_SetVar(interp, "scriptfile", filename, TCL_GLOBAL_ONLY);
        globals->scriptfile = (char *)Tcl_Alloc(strlen(filename) + 1);
        strcpy(globals->scriptfile, filename);

        result = Tcl_EvalObjEx(interp, script, 0);
        Tcl_DecrRefCount(script);

        /*
         * If result is not TCL_OK, print error message to stderr.
         */
        if (result != TCL_OK) {
            fprintf(stderr, "ERROR when eval: %s: %s\n", script,
                    Tcl_GetStringResult(interp));

            Tcl_Eval(interp, "puts stderr {STACK TRACE:}; puts stderr "
                             "$errorInfo; flush stderr;");
        }

        Tcl_Flush(m_Out); // Flushes the standard output channel
        Tcl_DecrRefCount(request_init);

    myclean:
        FCGI_Finish();
        Tcl_DeleteInterp(interp);
        Tcl_Release((ClientData)interp);
    }

end:
    Tcl_Finalize();
    return ret;
}
