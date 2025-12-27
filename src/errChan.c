#include "tclWeb.h"
#include <errno.h>
#include <fcgi_stdio.h>
#include <tcl.h>

/*
 * Capturing stdout and stderr in C or C++ program
 * https://wiki.tcl-lang.org/page/Capturing+stdout+and+stderr+in+C+or+C%2B%2B+program
 */

// outputErrProc is callback used by channel to handle data to outpu
int outputErrProc(ClientData instanceData, const char *buf, int toWrite,
                  int *errorCodePtr) {
    FCGI_fwrite((char *)buf, toWrite, 1, FCGI_stderr);
    return toWrite;
}

// inputErrProc doesn't do anything in an output-only channel.
int inputErrProc(ClientData instancedata, char *buf, int toRead,
                 int *errorCodePtr) {
    return TCL_ERROR;
}

// nothing to do on close
int closeErrProc(ClientData instancedata, Tcl_Interp *interp) { return 0; }

int close2ErrProc(ClientData instanceData, Tcl_Interp *interp, int flags) {
    if ((flags & (TCL_CLOSE_READ | TCL_CLOSE_WRITE)) == 0) {
        return closeErrProc(instanceData, interp);
    }
    return EINVAL;
}

// no options for this channel
int setoptionErrProc(ClientData instancedata, Tcl_Interp *interp,
                     const char *optionname, const char *value) {
    return TCL_OK;
}

// for non-blocking I/O, callback when data is ready.
void watchErrProc(ClientData instancedata, int mask) {
    /* not much to do here */
    return;
}

// gethandleErrProc -- retrieves device-specific handle, not applicable here.
int gethandleErrProc(ClientData instancedata, int direction,
                     ClientData *handlePtr) {
    return TCL_ERROR;
}
