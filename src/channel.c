#include "helputils.h"
#include "tclWeb.h"
#include <errno.h>
#include <fcgi_stdio.h>
#include <string.h>
#include <tcl.h>

/*
 * Capturing stdout and stderr in C or C++ program
 * https://wiki.tcl-lang.org/page/Capturing+stdout+and+stderr+in+C+or+C%2B%2B+program
 */

// outputproc is callback used by channel to handle data to outpu
int outputproc(ClientData instanceData, const char *buf, int toWrite,
               int *errorCodePtr) {
    const char *file_ext = NULL;
    Tcl_Interp *interp = (Tcl_Interp *)instanceData;

    interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    file_ext = FileSuffix(globals->scriptfile);
    if (globals->req->headers_set == 0 && strcmp(file_ext, ".tcl") == 0) {
        /*
         * Do nothing for Tcl script when headers_set == 0
         */
    } else {
        TclWeb_PrintHeaders(globals->req);
    }

    if (globals->req->content_sent == 0) {
        FCGI_fwrite((char *)buf, toWrite, 1, FCGI_stdout);
    }

    return toWrite;
}

// inputproc doesn't do anything in an output-only channel.
int inputproc(ClientData instancedata, char *buf, int toRead,
              int *errorCodePtr) {
    return TCL_ERROR;
}

// nothing to do on close
int closeproc(ClientData instancedata, Tcl_Interp *interp) { return 0; }

int close2proc(ClientData instanceData, Tcl_Interp *interp, int flags) {
    if ((flags & (TCL_CLOSE_READ | TCL_CLOSE_WRITE)) == 0) {
        return closeproc(instanceData, interp);
    }
    return EINVAL;
}

// no options for this channel
int setoptionproc(ClientData instancedata, Tcl_Interp *interp,
                  const char *optionname, const char *value) {
    return TCL_OK;
}

// for non-blocking I/O, callback when data is ready.
void watchproc(ClientData instancedata, int mask) {
    /* not much to do here */
    return;
}

// gethandleproc -- retrieves device-specific handle, not applicable here.
int gethandleproc(ClientData instancedata, int direction,
                  ClientData *handlePtr) {
    return TCL_ERROR;
}
