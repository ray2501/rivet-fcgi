#include <tcl.h>

#ifndef __ERRCHAN_H__
#define __ERRCHAN_H__

extern int outputErrProc(ClientData instanceData, const char *buf, int toWrite,
                      int *errorCodePtr);

extern int inputErrProc(ClientData instancedata, char *buf, int toRead,
                     int *errorCodePtr);

extern int closeErrProc(ClientData instancedata, Tcl_Interp *interp);

extern int close2ErrProc(ClientData instancedata, Tcl_Interp *interp, int flags);

extern int setoptionErrProc(ClientData instancedata, Tcl_Interp *interp,
                         const char *optionname, const char *value);

extern void watchErrProc(ClientData instancedata, int mask);

extern int gethandleErrProc(ClientData instancedata, int direction,
                         ClientData *handlePtr);


/*
 * Tcl Channel descriptor type.
 */
Tcl_ChannelType TclFCGIErrChan = {
    "tclIOFCGIErrChan",       /* typeName */
    TCL_CHANNEL_VERSION_5, /* channel type version */
    closeErrProc,             /* close ErrProc */
    inputErrProc,             /* input ErrProc */
    outputErrProc,            /* output ErrProc */
    NULL,                  /* seek ErrProc - can be null */
    setoptionErrProc,         /* set option ErrProc - can be null */
    NULL,                  /* get option ErrProc - can be null */
    watchErrProc,             /* watch ErrProc */
    gethandleErrProc,         /* get handle ErrProc */
    close2ErrProc,            /* close 2 ErrProc */
    NULL,                  /* block mode ErrProc - can be null */
    NULL,                  /* flush ErrProc - can be null */
    NULL,                  /* handler ErrProc - can be null */
    NULL,                  /* wide seek ErrProc - can be null if seekErrProc is*/
    NULL                   /* thread action ErrProc - can be null */
};

#endif
