#include <tcl.h>

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

extern int outputproc(ClientData instanceData, const char *buf, int toWrite,
                      int *errorCodePtr);

extern int inputproc(ClientData instancedata, char *buf, int toRead,
                     int *errorCodePtr);

extern int closeproc(ClientData instancedata, Tcl_Interp *interp);

extern int setoptionproc(ClientData instancedata, Tcl_Interp *interp,
                         const char *optionname, const char *value);

extern void watchproc(ClientData instancedata, int mask);

extern int gethandleproc(ClientData instancedata, int direction,
                         ClientData *handlePtr);


/*
 * Tcl Channel descriptor type.
 */
Tcl_ChannelType TclFCGIChan = {
    "tclIOFCGIChan",       /* typeName */
    TCL_CHANNEL_VERSION_4, /* channel type version */
    closeproc,             /* close proc */
    inputproc,             /* input proc */
    outputproc,            /* output proc */
    NULL,                  /* seek proc - can be null */
    setoptionproc,         /* set option proc - can be null */
    NULL,                  /* get option proc - can be null */
    watchproc,             /* watch proc */
    gethandleproc,         /* get handle proc */
    NULL,                  /* close 2 proc - can be null */
    NULL,                  /* block mode proc - can be null */
    NULL,                  /* flush proc - can be null */
    NULL,                  /* handler proc - can be null */
    NULL,                  /* wide seek proc - can be null if seekproc is*/
    NULL                   /* thread action proc - can be null */
};

#endif
