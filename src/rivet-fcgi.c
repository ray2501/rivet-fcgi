#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>
#include "channel.h"
#include "helputils.h"
#include "rivetParser.h"
#include "rivetCore.h"
#include "config.h"

/*
 * Only declare, I do not really test on Windows platform
 */
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif


Tcl_Interp *interp;

int main(int argc, char *argv[]) {
    int ret = EXIT_SUCCESS;

    Tcl_FindExecutable(argv[0]);

    interp = Tcl_CreateInterp();

    if (interp == NULL) {
        char *msg = "rivet-fcgi: Create Tcl interpreter failed.\n";
        #ifdef _WIN32
        _write(STDOUT_FILENO, msg, sizeof(msg)-1);
        #else
        write(STDOUT_FILENO, msg, sizeof(msg)-1);
        #endif

        return EXIT_FAILURE;
    }

    if (Tcl_Init(interp) != TCL_OK) {
        char *msg = "rivet-fcgi: Tcl_init failed.\n";
        #ifdef _WIN32
        _write(STDOUT_FILENO, msg, sizeof(msg)-1);
        #else
        write(STDOUT_FILENO, msg, sizeof(msg)-1);
        #endif

        return EXIT_FAILURE;
    }

    if (Rivet_InitCore(interp) != TCL_OK) {
        char *msg = "rivet-fcgi: Rivet_InitCore failed.\n";
        #ifdef _WIN32
        _write(STDOUT_FILENO, msg, sizeof(msg)-1);
        #else
        write(STDOUT_FILENO, msg, sizeof(msg)-1);
        #endif

        return EXIT_FAILURE;
    }

    if (Tcl_PkgRequire(interp, "Rivet", RIVET_INIT_VERSION, 1) == NULL)
    {
        char *msg = "rivet-fcgi: Tcl_PkgRequire Rivet failed.\n";
        #ifdef _WIN32
        _write(STDOUT_FILENO, msg, sizeof(msg)-1);
        #else
        write(STDOUT_FILENO, msg, sizeof(msg)-1);
        #endif

        return EXIT_FAILURE;
    }

    /*
     * We need redfine stdout and stderr to output result.
     * Create a channel to do this thing.
     */
    Tcl_Channel m_Out =
        Tcl_CreateChannel(&TclFCGIChan, "fcgiout",  (ClientData) FCGI_stdout, TCL_WRITABLE);

    Tcl_SetChannelOption(NULL, m_Out, "-translation", "lf");
    Tcl_SetChannelOption(NULL, m_Out, "-buffering", "none");
    Tcl_SetChannelOption(NULL, m_Out, "-encoding", "utf-8");

    Tcl_SetStdChannel(m_Out, TCL_STDOUT);
    Tcl_RegisterChannel(interp, m_Out);

    Tcl_Channel m_Err =
        Tcl_CreateChannel(&TclFCGIChan, "fcgierr",  (ClientData) FCGI_stderr, TCL_WRITABLE);

    Tcl_SetChannelOption(NULL, m_Err, "-translation", "lf");
    Tcl_SetChannelOption(NULL, m_Err, "-buffering", "none");
    Tcl_SetChannelOption(NULL, m_Err, "-encoding", "utf-8");

    Tcl_SetStdChannel(m_Err, TCL_STDERR);
    Tcl_RegisterChannel(interp, m_Err);

    /*
     * Response loop
     */
    while (FCGI_Accept() >= 0) {
        Tcl_Obj *script = NULL;
        Tcl_Obj *pathPtr = NULL;
        char *filename = NULL;
        const char *file_ext = NULL;
        int result;

        if (argc <= 1) {
            filename = getenv("SCRIPT_FILENAME");
            if (filename == NULL) {
                fprintf(stderr, "rivet-fcgi: No filename.\n");
                ret = EXIT_FAILURE;
                goto end;
            }
        } else if (argc==2) {
            if (strcmp(argv[1], "--version")==0) {
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
                *p =0;
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
                *p =0;
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
            FCGI_Finish();
            continue;
        }
        Tcl_DecrRefCount(pathPtr);

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
            FCGI_Finish();
            continue;
        }

        if (result == TCL_OK) {
            /* Send default header for .rvt file */
            if (strcmp(file_ext, ".rvt") == 0) {
                printf("Status: 200 OK\r\n");
                printf("Content-type: text/html; charset=utf-8\r\n\r\n");
            }
        } else {
            printf("Status: 500 Internal Server Error\r\n");

            fprintf(stderr, "rivet-fcgi: Could not read file %s.\n", filename);
            Tcl_DecrRefCount(script);
            FCGI_Finish();
            continue;
        }

        /*
         * Save the filename info to scriptfile
         */
        Tcl_SetVar(interp, "scriptfile", filename, TCL_GLOBAL_ONLY);

        result = Tcl_EvalObjEx(interp, script, 0);
        Tcl_DecrRefCount(script);

        /*
         * If result is not TCL_OK, print error message to stderr.
         */
        if (result != TCL_OK) {
            fprintf(stderr, "ERROR when eval: %s: %s\n", script,
                    Tcl_GetStringResult(interp));

            Tcl_Eval(interp,
                    "puts stderr {STACK TRACE:}; puts stderr $errorInfo; flush stderr;");
        }

        FCGI_Finish();
    }

end:
    Tcl_Finalize();
    return ret;
}
