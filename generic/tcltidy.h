/*
 * tcltidy.h --
 *
 *	This header file contains the function declarations needed for
 *	all of the source files in this package.
 *
 */

#ifndef _TIDY
#define _TIDY

#define NS "tidy"

/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <tidy/tidy.h>
#include <tidy/tidybuffio.h>
#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
   
typedef struct ThreadSpecificData
{
  int initialized; /* initialization flag */
  Tcl_HashTable *tidy_hashtblPtr; /* per thread hash table. */
  int doc_count;
} ThreadSpecificData;

typedef struct TclTidyDoc {
  int initialized; /* initialization flag */
  TidyDoc doc;
  TidyBuffer errbuf;
} TclTidyDoc;    
    
void TIDY_Thread_Exit(ClientData clientdata);
static int TIDY_DOC(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv);
static int TIDY_MAIN(void *cd, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);

 /*
 * Only the _Init function is exported.
 */

 extern DLLEXPORT int Tidy_Init(Tcl_Interp *interp);

 /*
 * end block for C++
 */

#ifdef __cplusplus
}
#endif

#endif /* _TIDY */
