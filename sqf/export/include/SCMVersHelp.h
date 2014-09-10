/******************************************************************************
*
* File:         SCMVersHelp.h
* Description:  Help build version proc
* Language:     C
*
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
*
*******************************************************************************/
#ifndef SCMVERSHELP_H
#define SCMVERSHELP_H 

#include <stdio.h>

#include "SCMBuildStr.h"

/*
 * ------------------------------------------------------------------------
 */

/*
 * internal helpers (i.e. used by SCMVersHelp.h)
 */
#define INTVERS_PROC_BUILD(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,br,scmbv,dt) \
VERS_ ## comp ## \
_CV ## cvmaj ## _ ## cvmin ## _ ## cvupd ## \
_PV ## pvmaj ## _ ## pvmin ## _ ## pvupd ## \
_BV ## bv ## \
_BR ## br ## \
_DT ## dt ## \
_SV ## scmbv
#define INTVERS_PROC_MAC(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,br,scmbv,dt) \
INTVERS_PROC_BUILD(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,br,scmbv,dt)

#define INTVERS_STR_BUILD(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,scmbv,dt) \
#comp " " \
"Version " #cvmaj "." #cvmin "." #cvupd " " \
"Release " #pvmaj "." #pvmin "." #pvupd " " \
"(Build " #bv " [" #scmbv "], date " #dt ")"
#define INTVERS_STR_MAC(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,scmbv,dt) \
INTVERS_STR_BUILD(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,scmbv,dt)
#define INTVERS_STR(comp) INTVERS_STR_MAC(comp,\
                                          VERS_CV_MAJ,VERS_CV_MIN,VERS_CV_UPD,\
                                          VERS_PV_MAJ,VERS_PV_MIN,VERS_PV_UPD,\
                                          VERS_BV,\
                                          VERS_SCMBV,\
                                          VERS_DT)
#define INTVERS2_STR_BUILD(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,br,scmbv,dt) \
#comp " " \
"Version " #cvmaj "." #cvmin "." #cvupd " " \
"Release " #pvmaj "." #pvmin "." #pvupd " " \
"(Build " #bv " [" #scmbv "], branch " #br ", date " #dt ")"
#define INTVERS2_STR_MAC(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,br,scmbv,dt) \
INTVERS2_STR_BUILD(comp,cvmaj,cvmin,cvupd,pvmaj,pvmin,pvupd,bv,br,scmbv,dt)
#define INTVERS2_STR(comp) INTVERS2_STR_MAC(comp,\
                                           VERS_CV_MAJ,VERS_CV_MIN,VERS_CV_UPD,\
                                           VERS_PV_MAJ,VERS_PV_MIN,VERS_PV_UPD,\
                                           VERS_BV,\
                                           VERS_BR,\
                                           VERS_SCMBV,\
                                           VERS_DT)

#define INTVERS_CAT(a,b) a ## b

/*
 * ------------------------------------------------------------------------
 */

/*
 * for external use (i.e. not by SCMVersHelp.h)
 */

/*
 * use this to define version proc
 * e.g. a function of the form:
 *   // if called, does nothing
 *   void VERS_<comp>_CV<maj>_<min>_<upd>PV<maj>_<min>_<upd>_BV<bv>_BR<br>_DT<dt>_SV<scmbv>()
 */
#define VERS_PROC(comp) INTVERS_PROC_MAC(comp,\
                                         VERS_CV_MAJ,VERS_CV_MIN,VERS_CV_UPD,\
                                         VERS_PV_MAJ,VERS_PV_MIN,VERS_PV_UPD,\
                                         VERS_BV,\
                                         VERS_BR2,\
                                         VERS_SCMBV2,\
                                         VERS_DT)

/*
 * use this to get copyright string
 */
#define VERS_COPY_STR \
"(C) Copyright 2011-2013 Hewlett-Packard Development Company, L.P."

/*
 * use this to define version proc function for a library
 * e.g. two functions of the form:
 *   // if called, does nothing
 *   void VERS_<comp>_CV<maj>_<min>_<upd>PV<maj>_<min>_<upd>_BV<bv>_BR<br>_DT<dt>_SV<scmbv>()
 *   // if called, returns version-string
 *   const char *<comp>_vers_str();
 */
#ifdef __cplusplus
#define VERS_LIB(comp) \
extern "C" void VERS_PROC(comp)(); \
void VERS_PROC(comp)() { \
    const char *temp = SCMBuildStr; \
    temp = temp; \
} \
extern "C" const char *INTVERS_CAT(comp,_vers_str)(); \
const char *INTVERS_CAT(comp,_vers_str)() { \
    return INTVERS_STR(comp); \
} \
extern "C" const char *INTVERS_CAT(comp,_vers2_str)(); \
const char *INTVERS_CAT(comp,_vers2_str)() { \
    return INTVERS2_STR(comp); \
} \
extern "C" void INTVERS_CAT(comp,_get_comp_vers)(int *cvmaj, int *cvmin, int *cvupd); \
void INTVERS_CAT(comp,_get_comp_vers)(int *cvmaj, int *cvmin, int *cvupd) { \
    *cvmaj = VERS_CV_MAJ; *cvmin = VERS_CV_MIN; *cvupd = VERS_CV_UPD; \
} \
extern "C" void INTVERS_CAT(comp,_get_prod_vers)(int *pvmaj, int *pvmin, int *pvupd); \
void INTVERS_CAT(comp,_get_prod_vers)(int *pvmaj, int *pvmin, int *pvupd) { \
    *pvmaj = VERS_PV_MAJ; *pvmin = VERS_PV_MIN; *pvupd = VERS_PV_UPD; \
}
#else
#define VERS_LIB(comp) \
extern void VERS_PROC(comp)(); \
void VERS_PROC(comp)() { \
    const char *temp = SCMBuildStr; \
    temp = temp; \
} \
extern const char *INTVERS_CAT(comp,_vers_str)(); \
const char *INTVERS_CAT(comp,_vers_str)() { \
    return INTVERS_STR(comp); \
} \
extern const char *INTVERS_CAT(comp,_vers2_str)(); \
const char *INTVERS_CAT(comp,_vers2_str)() { \
    return INTVERS2_STR(comp); \
} \
extern void INTVERS_CAT(comp,_get_comp_vers)(int *cvmaj, int *cvmin, int *cvupd); \
void INTVERS_CAT(comp,_get_comp_vers)(int *cvmaj, int *cvmin, int *cvupd) { \
    *cvmaj = VERS_CV_MAJ; *cvmin = VERS_CV_MIN; *cvupd = VERS_CV_UPD; \
} \
extern void INTVERS_CAT(comp,_get_prod_vers)(int *pvmaj, int *pvmin, int *pvupd); \
void INTVERS_CAT(comp,_get_prod_vers)(int *pvmaj, int *pvmin, int *pvupd) { \
    *pvmaj = VERS_PV_MAJ; *pvmin = VERS_PV_MIN; *pvupd = VERS_PV_UPD; \
}
#endif

/*
 * use this to define version proc function for a binary
 * e.g. four functions of the form:
 *   // if called, does nothing
 *   void VERS_<comp>_CV<maj>_<min>_<upd>PV<maj>_<min>_<upd>_BV<bv>_BR<br>_DT<dt>_SV<scmbv>()
 *   // if called, returns version-string
 *   const char *<comp>_vers_str();
 *   // if called, returns copyright-string
 *   const char *<comp>_vers_copy_str();
 *   // if called, prints version and copyright
 *   void <comp>_vers_print();
 */
#define VERS_BIN(comp) \
VERS_LIB(comp) \
const char *INTVERS_CAT(comp,_vers_copy_str)() { \
    return VERS_COPY_STR; \
} \
void INTVERS_CAT(comp,_vers_print)() { \
    printf("%s\n%s\n", \
           INTVERS_CAT(comp,_vers_str)(),  \
           INTVERS_CAT(comp,_vers_copy_str)()); \
} \
void INTVERS_CAT(comp,_vers2_print)() { \
    printf("%s\n%s\n", \
           INTVERS_CAT(comp,_vers2_str)(),  \
           INTVERS_CAT(comp,_vers_copy_str)()); \
}

/*
 * use this to define function 'dovers' to display version info
 * e.g. a function of the form:
 *   // if called, checks argc/argv for '--version' and if '--version',
 *   // then print version and copyright and exit(0)
 *   void dovers(int argc, char **argv)
 */
#define DEFINE_DOVERS(comp) \
\
extern void comp ## _vers2_print(); \
\
void dovers(int argc, char **argv) \
{ \
  int arg; \
  for (arg=1; arg<argc; arg++) { \
    if (strcmp(argv[arg], "--version") == 0) { \
       comp ## _vers2_print(); \
       exit(0); \
    } \
  } \
} 

/*
 * use this to define function '<comp>_dovers' to display version info
 * e.g. a function of the form:
 *   // if called, checks argc/argv for '--version' and if '--version',
 *   // then print version and copyright and exit(0)
 *   void <comp>_dovers(int argc, char **argv)
 *
 */
#define DEFINE_COMP_DOVERS(comp) \
\
extern void comp ## _vers2_print(); \
\
void comp ## _dovers(int argc, char **argv) \
{ \
  int arg; \
  for (arg=1; arg<argc; arg++) { \
    if (strcmp(argv[arg], "--version") == 0) { \
       comp ## _vers2_print(); \
       exit(0); \
    } \
  } \
} 

/*
 * use this to define 'extern void <comp>_dovers(int, char **)'
 */
#define DEFINE_EXTERN_COMP_DOVERS(comp) \
extern void comp ## _dovers(int argc, char **argv);

/*
 * use this to define 'extern void <comp>_vers_str()'
 */
#ifdef __cplusplus
#define DEFINE_EXTERN_COMP_GETVERS(comp) \
extern "C" const char *comp ## _vers_str();
#define DEFINE_EXTERN_COMP_GETVERS2(comp) \
extern "C" const char *comp ## _vers2_str();
#else
#define DEFINE_EXTERN_COMP_GETVERS(comp) \
extern const char *comp ## _vers_str();
#define DEFINE_EXTERN_COMP_GETVERS2(comp) \
extern const char *comp ## _vers2_str();
#endif

/*
 * use this to define 'extern void <comp>_vers_print()'
 */
#define DEFINE_EXTERN_COMP_PRINTVERS(comp) \
extern void comp ## _vers_print();

/*
 * use this to call '<comp>_dovers()'
 */
#define CALL_COMP_DOVERS(comp, argc, argv) \
comp ## _dovers(argc, argv); \
{ \
    const char *temp = SCMBuildStr; \
    temp = temp; \
}

/*
 * use this to call '<comp>_vers_str()'
 */
#define CALL_COMP_GETVERS(comp) \
comp ## _vers_str()
#define CALL_COMP_GETVERS2(comp) \
comp ## _vers2_str()

/*
 * use this to call '<comp>_get_comp_vers()'
 */
#define CALL_COMP_GET_COMP_VERS(comp,cmaj,cmin,cupd) \
comp ## _get_comp_vers(cmaj,cmin,cupd)

/*
 * use this to call '<comp>_get_prod_vers()'
 */
#define CALL_COMP_GET_PROD_VERS(comp,pmaj,pmin,pupd) \
comp ## _get_prod_vers(pmaj,pmin,pupd)

/*
 * use this to call '<comp>_vers_print()'
 */
#define CALL_COMP_PRINTVERS(comp) \
comp ## _vers_print();

#endif
