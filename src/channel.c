#include <fcgi_stdio.h>
#include <tcl.h>

/*
 * Capturing stdout and stderr in C or C++ program
 * https://wiki.tcl-lang.org/page/Capturing+stdout+and+stderr+in+C+or+C%2B%2B+program
 */

// outputproc is callback used by channel to handle data to outpu
int outputproc(ClientData instanceData, const char *buf, int toWrite,
                      int *errorCodePtr) {
    FCGI_fwrite((char *) buf, toWrite, 1,  instanceData);
    return toWrite;
}
// inputproc doesn't do anything in an output-only channel.
int inputproc(ClientData instancedata, char *buf, int toRead,
                     int *errorCodePtr) {
    return TCL_ERROR;
}
// nothing to do on close
int closeproc(ClientData instancedata, Tcl_Interp *interp) { return 0; }
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