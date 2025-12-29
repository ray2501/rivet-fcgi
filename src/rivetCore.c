/* rivetCore.c -- Core commands which are compiled into mod_rivet itself */

/*
    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.
*/


#include <tcl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rivet.h"
#include "tclWeb.h"

/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN
#endif /* EXTERN */
#include "rivetParser.h"
#include "helputils.h"

#define ABORTPAGE_CODE "ABORTPAGE"
#define SCRIPT_EXIT_CODE "SCRIPT_EXIT"

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Headers --
 *
 *      Command to manipulate HTTP headers from Tcl.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Headers )
{
    char *opt;
    interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (objc < 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    opt = Tcl_GetStringFromObj(objv[1], NULL);

    /* Basic introspection returning the value of the headers_printed flag */

    if (!strcmp("sent",opt))
    {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(globals->req->headers_printed));
        return TCL_OK;
    }

    if (globals->req->headers_printed != 0)
    {
        Tcl_AddObjErrorInfo(interp,
                            "Cannot manipulate headers - already sent", -1);
        return TCL_ERROR;
    }

    if (!strcmp("redirect", opt)) /* ### redirect ### */
    {
        Tcl_HashEntry *entry = NULL;
        int isNew = 0;
        char *hashvalue = NULL;
        char *value = NULL;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "new-url");
            return TCL_ERROR;
        }

        value = Tcl_GetStringFromObj (objv[2], (int *)NULL);
        entry = Tcl_FindHashEntry(globals->req->headers, "Location");
        if (entry == NULL) {
            entry = Tcl_CreateHashEntry(globals->req->headers, "Location", &isNew);
        } else {
            hashvalue = (char *)Tcl_GetHashValue(entry);
            if (hashvalue)
                Tcl_Free(hashvalue);
        }

        hashvalue = (char *)Tcl_Alloc(strlen(value) + 1);
        strcpy(hashvalue, value);
        Tcl_SetHashValue(entry, (ClientData) hashvalue);

        TclWeb_SetStatus(301, globals->req);
        return TCL_OK;
    }
    else if (!strcmp("get", opt)) /* ### get ### */
    {
        const char* header_value;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "headername");
            return TCL_ERROR;
        }
        header_value = TclWeb_OutputHeaderGet(Tcl_GetString(objv[2]),globals->req);

        Tcl_SetObjResult(interp,Tcl_NewStringObj(header_value ? header_value : "",-1));
    }
    else if (!strcmp("set", opt)) /* ### set ### */
    {
        if (objc != 4)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "headername value");
            return TCL_ERROR;
        }
        TclWeb_OutputHeaderSet(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), globals->req);
    }
    else if (!strcmp("add", opt)) /* ### set ### */
    {
        if (objc != 4)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "headername value");
            return TCL_ERROR;
        }
        TclWeb_HeaderAdd(Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), globals->req);
    }
    else if (!strcmp("type", opt)) /* ### set ### */
    {
        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "mime/type");
            return TCL_ERROR;
        }
        TclWeb_SetHeaderType(Tcl_GetString(objv[2]), globals->req);
    }
    else if (!strcmp("numeric", opt)) /* ### numeric ### */
    {
        int st = 200;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "response_code_number");
            return TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[2], &st) != TCL_ERROR) {
            TclWeb_SetStatus(st, globals->req);
        } else {
            return TCL_ERROR;
        }

    } else {

        Tcl_Obj* result = Tcl_NewStringObj("unrecognized subcommand: ",-1);
        Tcl_IncrRefCount(result);
        Tcl_AppendStringsToObj(result,opt,NULL);

        Tcl_SetObjResult(interp, result);
        Tcl_DecrRefCount(result);
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 * -----------------------------------------------------------------------------
 *
 * Rivet_AbortPageCmd --
 *
 *      Similar in purpose to PHP's "die" command, which halts all
 *      further output to the user.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      Stop the script.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_AbortPageCmd )
{
    static char *errorMessage = "Page generation terminated by abort_page directive";
    interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (objc > 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }

    if (objc == 2)
    {
        char* cmd_arg = Tcl_GetStringFromObj(objv[1],NULL);

        if (strcmp(cmd_arg,"-aborting") == 0)
        {
            Tcl_SetObjResult (interp,Tcl_NewBooleanObj(globals->page_aborting));
            return TCL_OK;
        }

        if (strcmp(cmd_arg,"-exiting") == 0)
        {
            Tcl_SetObjResult (interp,Tcl_NewBooleanObj(globals->script_exit));
            return TCL_OK;
        }

        /*
         * we assume abort_code to be null, as abort_page shouldn't run twice while
         * processing the same request
         */

        if (globals->abort_code == NULL)
        {
            globals->abort_code = objv[1];
            Tcl_IncrRefCount(globals->abort_code);
        }
    }

    /*
     * If page_aborting is true then this is the second call to abort_page
     * processing the same request: we ignore it and return a normal
     * completion code
     */

    if (globals->page_aborting)
    {
        return TCL_OK;
    }

    /* this is the first (and supposedly unique) abort_page call during this request */

    globals->page_aborting = 1;

    Tcl_AddErrorInfo (interp, errorMessage);
    Tcl_SetErrorCode (interp, "RIVET", ABORTPAGE_CODE, errorMessage, (char *)NULL);
    return TCL_ERROR;
}


/*
 * -----------------------------------------------------------------------------
 * Rivet_AbortCodeCmd --
 *
 * Returns the abort code stored internally by passing a user defined parameter
 * to the command 'abort_page'.
 *
 *-----------------------------------------------------------------------------
 */
TCL_CMD_HEADER( Rivet_AbortCodeCmd )
{
    interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (globals->abort_code != NULL)
    {
        Tcl_SetObjResult(interp,globals->abort_code);
    }

    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Parse --
 *
 *      Include and parse a Rivet file.
 *
 * Results:
 *      Standard Tcl result.
 *
 * Side Effects:
 *      Whatever occurs in the Rivet page parsed.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Parse )
{
    const char*     scriptfile = NULL;
    char*           filename = 0;
    Tcl_Obj*        script = NULL;
    Tcl_Obj*        pathPtr = NULL;
    int             result;
    Tcl_DString     fullpath;
    interp_globals* globals = Tcl_GetAssocData(interp, "rivet", NULL);


    if( objc != 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
    }

    if( objc == 2 ) {
        filename = Tcl_GetStringFromObj( objv[1], (Tcl_Size *)NULL );
    }

    // scriptfile = Tcl_GetVar(interp, "scriptfile", TCL_GLOBAL_ONLY);
    scriptfile = globals->scriptfile;

    /*
     * Try to get real path.
     */
    Tcl_DStringInit(&fullpath);

    /* If it is not absolute path */
    if (filename[0] != '/') {
        char *mypath = NULL;
        int i, n;

        n = strlen(scriptfile);
        i = n - 1;
        while ((i > 0) && (scriptfile[i] != '/') && (scriptfile[i] != '\\')) {
            i--;
        }

        if ((i > 0) && (i < n - 1) && ((scriptfile[i] == '/') || (scriptfile[i] == '\\'))) {
            mypath = strndup(scriptfile, i + 1);
            Tcl_DStringAppend (&fullpath, mypath, strlen(mypath));
            if (mypath) free(mypath);
        }
    }

    Tcl_DStringAppend (&fullpath, filename, strlen(filename));

    if (!strcmp(Tcl_DStringValue(&fullpath), scriptfile))
    {
        fprintf(stderr, "Rivet_Parse: Cannot recursively call the same file!");
        Tcl_DStringFree(&fullpath);
        return TCL_ERROR;
    }

    /*
     * Check file can access.
     */
    pathPtr = Tcl_NewStringObj(Tcl_DStringValue(&fullpath), -1);
    if (!pathPtr) {
        fprintf(stderr, "Rivet_Parse: Something is wrong.");
        Tcl_DStringFree(&fullpath);
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(pathPtr);
    if (Tcl_FSAccess(pathPtr, R_OK)) {
        fprintf(stderr, "Rivet_Parse: File cannot read.\n");
        Tcl_DStringFree(&fullpath);
        Tcl_DecrRefCount(pathPtr);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(pathPtr);

    script = Tcl_NewObj();
    Tcl_IncrRefCount(script);

    result = Rivet_GetRivetFile(Tcl_DStringValue(&fullpath),script,interp);
    Tcl_DStringFree(&fullpath);
    if (result != TCL_OK)
    {
        fprintf(stderr, "Rivet_Parse: Could not read file ");
        Tcl_DecrRefCount(script);
        return result;
    }

    result = Tcl_EvalObjEx(interp,script,0);
    Tcl_DecrRefCount(script);
    return result;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_Include --
 *
 *      Includes a file literally in the output stream.  Useful for
 *      images, plain HTML and the like.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      Adds to the output stream.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_Include )
{
    int                     sz;
    Tcl_Channel             fd;
    Tcl_Channel             tclstdout;
    Tcl_Obj*                outobj;
    char*                   filename;
    Tcl_DString             transoptions;
    Tcl_DString             encoptions;
    Tcl_DString             fullpath;
    interp_globals*         globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if( objc != 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "filename");
        return TCL_ERROR;
    }

    if( objc == 2 ) {
        filename = Tcl_GetStringFromObj( objv[1], (Tcl_Size *)NULL );
    }

    /*
     * Try to get real path.
     */
    Tcl_DStringInit(&fullpath);

    /* If it is not absolute path */
    if (filename[0] != '/') {
        char *result = NULL;
        const char *scriptfile = NULL;
        int i, n;

        //scriptfile = Tcl_GetVar(interp, "scriptfile", TCL_GLOBAL_ONLY);
        scriptfile = globals->scriptfile;
        n = strlen(scriptfile);
        i = n - 1;
        while ((i > 0) && (scriptfile[i] != '/') && (scriptfile[i] != '\\')) {
            i--;
        }

        if ((i > 0) && (i < n - 1) && ((scriptfile[i] == '/') || (scriptfile[i] == '\\'))) {
            result = strndup(scriptfile, i + 1);
            Tcl_DStringAppend (&fullpath, result, strlen(result));
            if (result) free(result);
        }
    }

    Tcl_DStringAppend (&fullpath, filename, strlen(filename));

    /*
     * Open file channel and get data.
     */
    fd = Tcl_OpenFileChannel(interp, Tcl_DStringValue(&fullpath), "r", 0664);
    Tcl_DStringFree(&fullpath);

    if (fd == NULL)
    {
        return TCL_ERROR;
    }
    Tcl_SetChannelOption(interp, fd, "-translation", "binary");

    outobj = Tcl_NewObj();
    Tcl_IncrRefCount(outobj);
    sz = Tcl_ReadChars(fd, outobj, -1, 0);
    if (sz == -1)
    {
        Tcl_AddErrorInfo(interp, Tcl_PosixError(interp));
        Tcl_DecrRefCount(outobj);
        return TCL_ERROR;
    }

    /* What we are doing is saving the translation and encoding
     * options, setting them both to binary, and the restoring the
     * previous settings. */
    Tcl_DStringInit(&transoptions);
    Tcl_DStringInit(&encoptions);
    tclstdout = Tcl_GetChannel(interp, "stdout", NULL);
    Tcl_GetChannelOption(interp, tclstdout, "-translation", &transoptions);
    Tcl_GetChannelOption(interp, tclstdout, "-encoding", &encoptions);
    Tcl_SetChannelOption(interp, tclstdout, "-translation", "binary");
    Tcl_WriteObj(tclstdout, outobj);
    Tcl_SetChannelOption(interp, tclstdout, "-translation", Tcl_DStringValue(&transoptions));
    Tcl_SetChannelOption(interp, tclstdout, "-encoding", Tcl_DStringValue(&encoptions));
    Tcl_DStringFree(&transoptions);
    Tcl_DStringFree(&encoptions);
    Tcl_DecrRefCount(outobj);
    return Tcl_Close(interp, fd);
}


/*
 * -----------------------------------------------------------------------------
 *
 * Rivet_NoBody --
 *
 *      Tcl command to erase body, so that only header is returned.
 *      Necessary for 304 responses.
 *
 * Results:
 *      A standard Tcl return value.
 *
 * Side Effects:
 *      Eliminates any body returned in the HTTP response.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_NoBody )
{
    interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);

    if (globals->req->content_sent == 1) {
        Tcl_AddErrorInfo(interp, "Content already sent");
        return TCL_ERROR;
    }

    globals->req->content_sent = 1;
    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_EnvCmd --
 *
 *      Loads a single environmental variable, to avoid the overhead
 *      of storing all of them when only one is needed.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_EnvCmd )
{
    char*                   key;
    char*                   val;

    if( objc != 2 ) {
        Tcl_WrongNumArgs( interp, 1, objv, "variable" );
        return TCL_ERROR;
    }

    key = Tcl_GetStringFromObj (objv[1],NULL);

    val = getenv (key);

    Tcl_SetObjResult(interp, Tcl_NewStringObj( val, -1 ) );
    return TCL_OK;
}


/*
 * -----------------------------------------------------------------------------
 *
 * Rivet_ExitCmd --
 *
 *      Stop the script.
 *
 * Result:
 *
 *      TCL_ERROR
 *
 * Side Effects:
 *      Stop the script.
 *
 *-----------------------------------------------------------------------------
 */

TCL_CMD_HEADER( Rivet_ExitCmd )
{
    int value;
    interp_globals *globals = Tcl_GetAssocData(interp, "rivet", NULL);
    char* errorMessage = "page generation interrupted by exit command";

    if ((objc != 1) && (objc != 2)) {
        Tcl_WrongNumArgs(interp, 1, objv, "?returnCode?");
        return TCL_ERROR;
    }

    if (objc == 1) {
        value = 0;
    } else if (Tcl_GetIntFromObj(interp, objv[1], &value) != TCL_OK) {
        return TCL_ERROR;
    }

    if (globals->script_exit)
    {
        return TCL_OK;
    }

    /*
     * Need to check the behavior is correct or not.
     */
    // globals->page_aborting = 1;
    globals->script_exit = 1;
    globals->abort_code = Tcl_NewDictObj();

    /* The private->abort_code ref count is decremented before
     * request processing terminates */

    Tcl_IncrRefCount(globals->abort_code);

    Tcl_DictObjPut(interp, globals->abort_code,
                   Tcl_NewStringObj("error_code",-1),
                   Tcl_NewStringObj("exit",-1));

    Tcl_DictObjPut(interp, globals->abort_code,
                   Tcl_NewStringObj("return_code",-1),
                   Tcl_NewIntObj(value));

    // globals->script_exit = 1;

    Tcl_AddErrorInfo (interp, errorMessage);
    Tcl_SetErrorCode (interp, "RIVET", SCRIPT_EXIT_CODE, errorMessage, (char *)NULL);

    return TCL_ERROR;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_InitCore --
 *
 *      Creates the core rivet commands.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side Effects:
 *      Creates new commands.
 *
 *-----------------------------------------------------------------------------
 */

DLLEXPORT int
Rivet_InitCore(Tcl_Interp *interp)
{

    RIVET_OBJ_CMD ("headers",Rivet_Headers,NULL);
    RIVET_OBJ_CMD ("abort_page",Rivet_AbortPageCmd,NULL);
    RIVET_OBJ_CMD ("abort_code", Rivet_AbortCodeCmd,NULL);
    RIVET_OBJ_CMD ("include",Rivet_Include,NULL);
    RIVET_OBJ_CMD ("parse",Rivet_Parse,NULL);
    RIVET_OBJ_CMD ("no_body",Rivet_NoBody,NULL);
    RIVET_OBJ_CMD ("env",Rivet_EnvCmd,NULL);
    RIVET_OBJ_CMD ("exit",Rivet_ExitCmd,NULL);

    {
        Tcl_Namespace *rivet_ns;

        rivet_ns = Tcl_FindNamespace(interp,RIVET_NS,NULL,TCL_GLOBAL_ONLY);
        if (rivet_ns == NULL)
        {
            rivet_ns = Tcl_CreateNamespace (interp,RIVET_NS,NULL,
                                        (Tcl_NamespaceDeleteProc *)NULL);
        }

        RIVET_EXPORT_CMD(interp,rivet_ns,"headers");
        RIVET_EXPORT_CMD(interp,rivet_ns,"abort_page");
        RIVET_EXPORT_CMD(interp,rivet_ns,"abort_code");
        RIVET_EXPORT_CMD(interp,rivet_ns,"include");
        RIVET_EXPORT_CMD(interp,rivet_ns,"parse");
        RIVET_EXPORT_CMD(interp,rivet_ns,"no_body");
        RIVET_EXPORT_CMD(interp,rivet_ns,"env");
        // ::rivet::exit is not exported
    }

    return TCL_OK;
}
