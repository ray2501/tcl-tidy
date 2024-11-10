#include "tcltidy.h"

static Tcl_ThreadDataKey dataKey;

TCL_DECLARE_MUTEX(myMutex);

void TIDY_Thread_Exit(ClientData clientdata)
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
        Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->tidy_hashtblPtr)
    {
        Tcl_DeleteHashTable(tsdPtr->tidy_hashtblPtr);
        ckfree(tsdPtr->tidy_hashtblPtr);
    }
}

static int TIDY_DOC(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int choice;
    TclTidyDoc *doc;
    Tcl_HashEntry *hashEntryPtr;
    char *handle;

    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
        Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->initialized == 0)
    {
        tsdPtr->initialized = 1;
        tsdPtr->tidy_hashtblPtr = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(tsdPtr->tidy_hashtblPtr, TCL_STRING_KEYS);
    }

    static const char *TIDYDOC_strs[] = {
        "quick_repair string",
        "parse_string string",
        "clean_repair",
        "diagnose",
        "get_output",
        "get_status",
        "get_config",
        "set_config",
        "get_opt",
        "set_opt",
        "error_count",
        "warning_count",
        "access_count",
        "close",
        0
    };

    enum TIDYDOC_enum
    {
        TIDYDOC_QUICK_REPAIR,
        TIDYDOC_PARSE_STRING,
        TIDYDOC_CLEAN_REPAIR,
        TIDYDOC_DIAGNOSE,
        TIDYDOC_GET_OUTPUT,
        TIDYDOC_GET_STATUS,
        TIDYDOC_GET_CONFIG,
        TIDYDOC_SET_CONFIG,
        TIDYDOC_GET_OPT,
        TIDYDOC_SET_OPT,
        TIDYDOC_ERROR_COUNT,
        TIDYDOC_WARNING_COUNT,
        TIDYDOC_ACCESS_COUNT,
        TIDYDOC_CLOSE,
    };

    if (objc < 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], TIDYDOC_strs, "option", 0, &choice))
    {
        return TCL_ERROR;
    }

    /*
     * Get the TclTidyDoc * point
     */
    handle = Tcl_GetStringFromObj(objv[0], 0);
    hashEntryPtr = Tcl_FindHashEntry(tsdPtr->tidy_hashtblPtr, handle);
    if (!hashEntryPtr)
    {
        if (interp)
        {
            Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
            Tcl_AppendStringsToObj(resultObj, "invalid tidy handle ", handle, (char *)NULL);
        }

        return TCL_ERROR;
    }

    doc = Tcl_GetHashValue(hashEntryPtr);

    switch ((enum TIDYDOC_enum)choice)
    {

    case TIDYDOC_QUICK_REPAIR:
    {
        Tcl_Obj *pResultStr = NULL;
        char *input = NULL;
        char *encname = "utf8";
        Tcl_Size str_len = 0;
        TidyBuffer buf;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "string");
            return TCL_ERROR;
        }

        input = Tcl_GetStringFromObj(objv[2], &str_len);
        if (!input || str_len < 1)
        {
            return TCL_ERROR;
        }
        
        /*
         * Values include: ascii, latin1, raw, utf8, iso2022, mac,
         * win1252, utf16le, utf16be, utf16, big5 and shiftjis.
         * Case in-sensitive.
         */
        if (tidySetCharEncoding(doc->doc, encname) < 0)
        {   
            if (interp)
            {   
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Could not set encoding", (char *)NULL);
            }
            
            return TCL_ERROR;
        }

        tidyBufInit(&buf);
        tidyBufAttach(&buf, (void *)input, str_len);

        if (tidyParseBuffer(doc->doc, &buf) < 0)
        {
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, doc->errbuf.bp, (char *)NULL);
            }

            return TCL_ERROR;
        }

        if (tidyCleanAndRepair(doc->doc) >= 0)
        {
            TidyBuffer output;
            tidyBufInit(&output);

            tidyOptSetBool(doc->doc, TidyForceOutput, yes);
            tidyOptSetBool(doc->doc, TidyMark, no);

            tidySaveBuffer(doc->doc, &output);

            if (output.bp)
            {
                pResultStr = Tcl_NewStringObj((char *)output.bp, output.size);
            }
            else
            {
                pResultStr = Tcl_NewStringObj("", 0);
            }
            Tcl_SetObjResult(interp, pResultStr);

            tidyBufFree(&output);
        }

        doc->initialized = 1;

        break;
    }

    case TIDYDOC_PARSE_STRING: {
        char *input = NULL;
        char *encname = "utf8";
        Tcl_Size str_len = 0;
        TidyBuffer buf;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "string");
            return TCL_ERROR;
        }

        input = Tcl_GetStringFromObj(objv[2], &str_len);
        if (!input || str_len < 1)
        {
            return TCL_ERROR;
        }

        /*
         * Values include: ascii, latin1, raw, utf8, iso2022, mac,
         * win1252, utf16le, utf16be, utf16, big5 and shiftjis.
         * Case in-sensitive.
         */
        if (tidySetCharEncoding(doc->doc, encname) < 0)
        {
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Could not set encoding", (char *)NULL);
            }

            return TCL_ERROR;
        }

        tidyBufInit(&buf);
        tidyBufAttach(&buf, (void *)input, str_len);

        if (tidyParseBuffer(doc->doc, &buf) < 0)
        {
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, doc->errbuf.bp, (char *)NULL);
            }

            return TCL_ERROR;
        }

        doc->initialized = 1;

        break;
    }
                              
    case TIDYDOC_CLEAN_REPAIR:
    {
        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        if (doc->initialized == 0) {
            return TCL_ERROR;
        }

        if (tidyCleanAndRepair(doc->doc) < 0)
        {
            return TCL_ERROR;
        }

        break;
    }

    case TIDYDOC_DIAGNOSE:
    {
        Tcl_Obj *pResultStr = NULL;

        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        if (doc->initialized == 0) {
            return TCL_ERROR;
        }

        if (tidyRunDiagnostics(doc->doc) < 0)
        {
            return TCL_ERROR;
        }

        if (doc->errbuf.bp)
        {
            pResultStr = Tcl_NewStringObj((char *)doc->errbuf.bp, doc->errbuf.size);
        }
        else
        {
            pResultStr = Tcl_NewStringObj("", 0);
        }

        Tcl_SetObjResult(interp, pResultStr);

        break;
    }

    case TIDYDOC_GET_OUTPUT:
    {
        Tcl_Obj *pResultStr = NULL;
        TidyBuffer output;

        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        if (doc->initialized == 0) {
            return TCL_ERROR;
        }

        tidyBufInit(&output);
        tidySaveBuffer(doc->doc, &output);

        if (output.bp)
        {
            pResultStr = Tcl_NewStringObj((char *)output.bp, output.size);
        }
        else
        {
            pResultStr = Tcl_NewStringObj("", 0);
        }
        Tcl_SetObjResult(interp, pResultStr);

        tidyBufFree(&output);
        break;
    }

    case TIDYDOC_GET_STATUS:
    {
        Tcl_Obj *pResultStr = NULL;

        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        pResultStr = Tcl_NewLongObj(tidyStatus(doc->doc));
        Tcl_SetObjResult(interp, pResultStr);

        break;
    }

    case TIDYDOC_GET_CONFIG:
    {
        TidyIterator itOpt;
        char *opt_name;
        char *string_value;
        int int_value;
        int bool_value;
        TidyOptionType type;
        Tcl_Obj *pResultStr = NULL;

        itOpt = tidyGetOptionList(doc->doc);

        pResultStr = Tcl_NewListObj(0, NULL);
        while (itOpt)
        {
            TidyOption opt = tidyGetNextOption(doc->doc, &itOpt);

            opt_name = (char *)tidyOptGetName(opt);
            type = tidyOptGetType(opt);
            switch (type)
            {
            case TidyString:
            {
                string_value = (char *)tidyOptGetValue(doc->doc, tidyOptGetId(opt));
                if (string_value == NULL)
                {
                    string_value = "";
                }

                Tcl_ListObjAppendElement(interp, pResultStr,
                                         Tcl_NewStringObj(opt_name, strlen(opt_name)));
                Tcl_ListObjAppendElement(interp, pResultStr,
                                         Tcl_NewStringObj(string_value, strlen(string_value)));

                break;
            }

            case TidyInteger:
                int_value = tidyOptGetInt(doc->doc, tidyOptGetId(opt));
                Tcl_ListObjAppendElement(interp, pResultStr,
                                         Tcl_NewStringObj(opt_name, strlen(opt_name)));
                Tcl_ListObjAppendElement(interp, pResultStr,
                                         Tcl_NewIntObj(int_value));
                break;

            case TidyBoolean:
                bool_value = tidyOptGetBool(doc->doc, tidyOptGetId(opt));
                Tcl_ListObjAppendElement(interp, pResultStr,
                                         Tcl_NewStringObj(opt_name, strlen(opt_name)));
                Tcl_ListObjAppendElement(interp, pResultStr,
                                         Tcl_NewBooleanObj(bool_value));
                break;
            }
        }
        Tcl_SetObjResult(interp, pResultStr);

        break;
    }

    case TIDYDOC_SET_CONFIG:
    {
        TidyOption opt;
        Tcl_Size list_objc = 0;
        Tcl_Obj **list_objv = NULL;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "options_list");
            return TCL_ERROR;
        }

        if (Tcl_ListObjGetElements(interp, objv[2], &list_objc, 
                    (Tcl_Obj ***)&list_objv) == TCL_ERROR)
        {
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Get list elements failed", (char *)NULL);
            }

            return TCL_ERROR;
        }

        if (list_objc % 2 != 0)
        {
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Check list failed", (char *)NULL);
            }

            return TCL_ERROR;
        }

        for (int i = 0; i < list_objc; i = i + 2)
        {
            const char *optname = Tcl_GetString(list_objv[i]);
            const char *optvalue = Tcl_GetString(list_objv[i + 1]);
            int int_value = 0;

            opt = tidyGetOptionByName(doc->doc, optname);
            if (!opt)
            {
                return TCL_ERROR;
            }

            if (tidyOptIsReadOnly(opt))
            {
                if (interp)
                {
                    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                    Tcl_AppendStringsToObj(resultObj, "Try to set ReadOnly option", (char *)NULL);
                }

                return TCL_ERROR;
            }

            switch (tidyOptGetType(opt))
            {
            case TidyString:
                if (tidyOptSetValue(doc->doc, tidyOptGetId(opt), optvalue) <= 0)
                {
                    if (interp)
                    {
                        Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                        Tcl_AppendStringsToObj(resultObj, "Failed (string)", (char *)NULL);
                    }

                    return TCL_ERROR;
                }
                break;

            case TidyInteger:
                int_value = atoi(optvalue);
                if (tidyOptSetInt(doc->doc, tidyOptGetId(opt), int_value) <= 0)
                {
                    if (interp)
                    {
                        Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                        Tcl_AppendStringsToObj(resultObj, "Failed (int)", (char *)NULL);
                    }

                    return TCL_ERROR;
                }
                break;

            case TidyBoolean:
                int_value = atoi(optvalue);
                if (tidyOptSetBool(doc->doc, tidyOptGetId(opt), int_value) <= 0)
                {
                    if (interp)
                    {
                        Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                        Tcl_AppendStringsToObj(resultObj, "Failed (boolean)", (char *)NULL);
                    }

                    return TCL_ERROR;
                }
                break;

            default:
                if (interp)
                {
                    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                    Tcl_AppendStringsToObj(resultObj, "Unknown type", (char *)NULL);
                }

                return TCL_ERROR;
            }
        }

        break;
    }

    case TIDYDOC_GET_OPT:
    {
        char *optname = NULL;
        const char *optvalue = NULL;
        Tcl_Size str_len;
        TidyOption opt;
        int int_value = 0;
        int bool_value = 0;
        Tcl_Obj *pResultStr = NULL;

        if (objc != 3)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "option_name");
            return TCL_ERROR;
        }

        optname = Tcl_GetStringFromObj(objv[2], &str_len);
        if (!optname || str_len < 1)
        {
            return TCL_ERROR;
        }

        opt = tidyGetOptionByName(doc->doc, optname);
        if (!opt)
        {
            return TCL_ERROR;
        }

        switch (tidyOptGetType(opt))
        {
        case TidyString:
            optvalue = tidyOptGetValue(doc->doc, tidyOptGetId(opt));
            if (!optvalue)
            {
                optvalue = "";
            }

            pResultStr = Tcl_NewStringObj(optvalue, -1);
            break;

        case TidyInteger:
            int_value = tidyOptGetInt(doc->doc, tidyOptGetId(opt));
            pResultStr = Tcl_NewIntObj(int_value);
            break;

        case TidyBoolean:
            bool_value = tidyOptGetBool(doc->doc, tidyOptGetId(opt));
            pResultStr = Tcl_NewBooleanObj(bool_value);
            break;

        default:
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Unknown type", (char *)NULL);
            }

            return TCL_ERROR;
        }
        Tcl_SetObjResult(interp, pResultStr);

        break;
    }

    case TIDYDOC_SET_OPT:
    {
        char *optname = NULL;
        char *optvalue = NULL;
        Tcl_Size str_len;
        TidyOption opt;
        int int_value = 0;

        if (objc != 4)
        {
            Tcl_WrongNumArgs(interp, 2, objv, "option_name option_value");
            return TCL_ERROR;
        }

        optname = Tcl_GetStringFromObj(objv[2], &str_len);
        if (!optname || str_len < 1)
        {
            return TCL_ERROR;
        }

        optvalue = Tcl_GetStringFromObj(objv[3], &str_len);
        if (!optvalue || str_len < 1)
        {
            return TCL_ERROR;
        }

        opt = tidyGetOptionByName(doc->doc, optname);
        if (!opt)
        {
            return TCL_ERROR;
        }

        if (tidyOptIsReadOnly(opt))
        {
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Try to set ReadOnly option", (char *)NULL);
            }

            return TCL_ERROR;
        }

        switch (tidyOptGetType(opt))
        {
        case TidyString:
            if (tidyOptSetValue(doc->doc, tidyOptGetId(opt), optvalue) <= 0)
            {
                if (interp)
                {
                    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                    Tcl_AppendStringsToObj(resultObj, "Failed (string)", (char *)NULL);
                }

                return TCL_ERROR;
            }
            break;

        case TidyInteger:
            int_value = atoi(optvalue);
            if (tidyOptSetInt(doc->doc, tidyOptGetId(opt), int_value) <= 0)
            {
                if (interp)
                {
                    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                    Tcl_AppendStringsToObj(resultObj, "Failed (int)", (char *)NULL);
                }

                return TCL_ERROR;
            }
            break;

        case TidyBoolean:
            int_value = atoi(optvalue);
            if (tidyOptSetBool(doc->doc, tidyOptGetId(opt), int_value) <= 0)
            {
                if (interp)
                {
                    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                    Tcl_AppendStringsToObj(resultObj, "Failed (boolean)", (char *)NULL);
                }

                return TCL_ERROR;
            }
            break;

        default:
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Unknown type", (char *)NULL);
            }

            return TCL_ERROR;
        }

        break;
    }

    /*
     * Returns the Number of Tidy errors
     */
    case TIDYDOC_ERROR_COUNT:
    {
        Tcl_Obj *pResultStr = NULL;

        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        pResultStr = Tcl_NewLongObj(tidyErrorCount(doc->doc));
        Tcl_SetObjResult(interp, pResultStr);

        break;
    }

    /*
     * Returns the Number of Tidy warnings
     */
    case TIDYDOC_WARNING_COUNT:
    {
        Tcl_Obj *pResultStr = NULL;

        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        pResultStr = Tcl_NewLongObj(tidyWarningCount(doc->doc));
        Tcl_SetObjResult(interp, pResultStr);

        break;
    }

    /*
     * Returns the Number of Tidy accessibility warnings
     */
    case TIDYDOC_ACCESS_COUNT:
    {
        Tcl_Obj *pResultStr = NULL;

        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        pResultStr = Tcl_NewLongObj(tidyAccessWarningCount(doc->doc));
        Tcl_SetObjResult(interp, pResultStr);

        break;
    }

    case TIDYDOC_CLOSE:
    {
        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        if (doc)
        {
            tidyBufFree(&doc->errbuf);
            tidyRelease(doc->doc);

            free(doc);
            doc = NULL;
        }
        Tcl_MutexLock(&myMutex);
        if (hashEntryPtr)
            Tcl_DeleteHashEntry(hashEntryPtr);
        Tcl_MutexUnlock(&myMutex);
        Tcl_DeleteCommand(interp, handle);

        break;
    }
    }

    return TCL_OK;
}

static int TIDY_MAIN(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int choice;

    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
        Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->initialized == 0)
    {
        tsdPtr->initialized = 1;

        tsdPtr->tidy_hashtblPtr = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(tsdPtr->tidy_hashtblPtr, TCL_STRING_KEYS);

        tsdPtr->doc_count = 0;
    }

    static const char *TIDY_strs[] = {
        "create",
        "libversion",
        0
    };

    enum TIDY_enum
    {
        TIDY_create,
        TIDY_libversion,
    };

    if (objc < 2)
    {
        Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], TIDY_strs, "option", 0, &choice))
    {
        return TCL_ERROR;
    }

    switch ((enum TIDY_enum)choice)
    {
    case TIDY_create:
    {
        TclTidyDoc *doc;
        Tcl_HashEntry *hashEntryPtr;
        char handleName[16 + TCL_INTEGER_SPACE];
        Tcl_Obj *pResultStr = NULL;
        int newvalue;

        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        doc = (TclTidyDoc *)malloc(sizeof(TclTidyDoc));
        if (!doc)
        {
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Malloc memory failed", (char *)NULL);
            }

            return TCL_ERROR;
        }

        doc->initialized = 0;
        doc->doc = tidyCreate();
        tidyBufInit(&doc->errbuf);

        if (tidySetErrorBuffer(doc->doc, &doc->errbuf) < 0)
        {
            if (interp)
            {
                Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
                Tcl_AppendStringsToObj(resultObj, "Could not set ErrorBuffer", (char *)NULL);
            }

            if(doc) {
                tidyBufFree(&doc->errbuf);
                tidyRelease(doc->doc);
                free(doc);
            }
            return TCL_ERROR;
        }

        Tcl_MutexLock(&myMutex);
        sprintf(handleName, "tidy%d", tsdPtr->doc_count++);

        pResultStr = Tcl_NewStringObj(handleName, -1);

        hashEntryPtr = Tcl_CreateHashEntry(tsdPtr->tidy_hashtblPtr, handleName, &newvalue);
        Tcl_SetHashValue(hashEntryPtr, doc);
        Tcl_MutexUnlock(&myMutex);

        Tcl_CreateObjCommand(interp, handleName, (Tcl_ObjCmdProc *)TIDY_DOC,
                             (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

        Tcl_SetObjResult(interp, pResultStr);

        break;
    }

    case TIDY_libversion:
    {
        const char *verString = NULL;
        Tcl_Obj *pResultStr = NULL;

        if (objc != 2)
        {
            Tcl_WrongNumArgs(interp, 2, objv, 0);
            return TCL_ERROR;
        }

        verString = tidyLibraryVersion();
        pResultStr = Tcl_NewStringObj(verString, -1);
        Tcl_SetObjResult(interp, pResultStr);

        break;
    }
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tidy_Init --
 *
 *  Initialize the new package.  The string "Tidy" in the
 *  function name must match the PACKAGE declaration at the top of
 *  configure.ac.
 *
 * Results:
 *  A standard Tcl result
 *
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    DLLEXPORT int
    Tidy_Init(Tcl_Interp *interp)
    {
        Tcl_Namespace *nsPtr; /* pointer to hold our own new namespace */

        if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL)
        {
            return TCL_ERROR;
        }

        if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK)
        {
            return TCL_ERROR;
        }

        /*
     * We need a namespace.
     */
        nsPtr = Tcl_CreateNamespace(interp, "::" NS, NULL, NULL);
        if (nsPtr == NULL)
        {
            return TCL_ERROR;
        }

        /*
     *   Tcl_GetThreadData handles the auto-initialization of all data in
     *  the ThreadSpecificData to NULL at first time.
     */
        Tcl_MutexLock(&myMutex);
        ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

        if (tsdPtr->initialized == 0)
        {
            tsdPtr->initialized = 1;

            tsdPtr->tidy_hashtblPtr = (Tcl_HashTable *)ckalloc(sizeof(Tcl_HashTable));
            Tcl_InitHashTable(tsdPtr->tidy_hashtblPtr, TCL_STRING_KEYS);

            tsdPtr->doc_count = 0;
        }
        Tcl_MutexUnlock(&myMutex);

        /* Add a thread exit handler to delete data */
        Tcl_CreateThreadExitHandler(TIDY_Thread_Exit, (ClientData)NULL);

        Tcl_CreateObjCommand(interp, "::tidy::tidy", (Tcl_ObjCmdProc *)TIDY_MAIN,
                             (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

        return TCL_OK;
    }
#ifdef __cplusplus
}
#endif /* __cplusplus */
