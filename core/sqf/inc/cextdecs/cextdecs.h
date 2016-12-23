// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef _INC_TDM_CEXTDECS_H /* #include only once */
#define _INC_TDM_CEXTDECS_H

/* Define OMIT for C and C++ callers.
   When calling functions that on Tandem NSK systems allow missing
   parameters, on ANSI compilers you must supply the missing argument.
   To maintain the semantics of missing arguments, you must supply a
   special value to indicate that the argument is intended to be
   missing.  For reference or pointer parameters, this can be
   accomplished by specifying NULL.  However, for value parameters you
   must specify one of the following "special" values.  If this
   special value is a legitimate value for a given parameter, then
   you will need to use the "fat" argument types.
   The size of the container is not of concern to the C++
   programmer; you can simply supply an omitted parameter with the
   "OMIT" value.  However, in other languages, you must supply the
   appropriately-sized "OMIT" value; e.g., OMITSHORT for a missing short
   parameter, OMITFAT_16 for a fat_16 parameter, etc.

 */

#ifdef __linux__
  #ifndef _M_DG
    #define _M_DG
  #endif
  #ifndef _declspec
    #define _declspec(x)
  #endif
  #ifndef __int64
    #define __int64 long long int
  #endif
  #ifndef _int64
    #define _int64 long long int
  #endif
#endif

// Symbols used for "oversized" value arguments
//
typedef int		fat_16;
typedef __int64 fat_32;

// Define baddr
typedef void * baddr;

// Values used to indicate omitted arguments 
//
static short const          OMITSHORT         = -291;
static unsigned short const OMITUNSIGNEDSHORT = 0xFEDD;
static int const            OMITINT           = -19070975;
static fat_16 const         OMITFAT_16        = -19070975;
static unsigned int const   OMITUNSIGNEDINT   = 0xFEDD0001;
static long const           OMITLONG          = -19070975;
#ifdef __linux__
static fat_32 const         OMITFAT_32        = 0xfedd000000000001LL;
#else
static fat_32 const         OMITFAT_32        = -81909218222800895;
#endif
static unsigned long const  OMITUNSIGNEDLONG  = 0xFEDD0001;
#ifdef __linux__
__int64 const        OMIT__INT64       = 0xfedd000000000001LL;
#else
static __int64 const        OMIT__INT64       = -81909218222800895;
#endif


/* Define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef OMITREF
#define OMITREF    NULL
#endif

/* Make life easier for C++ callers.  They can just use "OMIT" to
   indicate a missing parameter. */
#ifdef __cplusplus
/*
Class ArgumentOmitted serves two purposes:

1) When the argument is an instance of ArgumentOmitted template class,
   causes the "omitted" constructor to be invoked during the call and the
   != overload operator to be invoked by the _arg_present function.

2) When the argument is a pre-define or a user-defined type, generates
   the specified value to indicate that the argument is omitted during the
   call or when tested by the _arg_present function.
 */
class ArgumentOmitted {
public:
	operator short() {return OMITSHORT;} 
	operator short*() {return OMITREF;} 
	operator int() {return OMITINT;} 
	operator int*() {return OMITREF;}
#ifndef NA_64BIT
	operator long() {return OMITLONG;}
#endif
	operator __int64() {return OMIT__INT64;}
	operator __int64*() {return OMITREF;}
#ifndef NA_64BIT
	operator long*() {return OMITREF;}
#endif
	operator char*() {return OMITREF;} 
	operator unsigned short() {return OMITUNSIGNEDSHORT;} 
	operator unsigned short*() {return OMITREF;} 
	operator unsigned int() {return OMITUNSIGNEDINT;} 
	operator unsigned int*() {return OMITREF;}
	operator unsigned long() {return OMITUNSIGNEDLONG;}
	operator unsigned long*() {return OMITREF;}
	operator unsigned char*() {return OMITREF;} 
	operator void*() {return OMITREF;}
	operator void**() {return OMITREF;}
};

static ArgumentOmitted OMIT;
#endif /* __cplusplus */
/*

The following defines are useful when calling functions that returned condition codes
on TNS CISC processors.

*/

#ifndef _cc_status
#define _cc_status int
#define _status_lt(x) ((x) < 0)
#define _status_gt(x) ((x) > 0)
#define _status_eq(x) ((x) == 0)
#define _status_le(x) ((x) <= 0)
#define _status_ge(x) ((x) >= 0)
#define _status_ne(x) ((x) != 0)
#endif

/*

Programs that use MFC have the symbol POSITION defined as a structure type. These programs
must call TDM_POSITION_(). Other programs can continue to use the symbol POSITION to refer to 
the file system procedure used to position within non-keysequenced files.

*/

#ifndef __AFX_H__
#define POSITION TDM_POSITION_
#endif

#ifdef __cplusplus
extern "C" {
_declspec(dllimport) short   ABORTTRANSACTION();

_declspec(dllimport) int ACTIVATERECEIVETRANSID
  (short    msgnum) //IN
;

_declspec(dllimport) int AWAITIOX
  (short    *filenum_,   //IN/OUT
   void  * *bufptr=NULL,     //OUT OPTIONAL
   unsigned short    *xfercount_=NULL, //OUT OPTIONAL
   int      *tag=NULL,        //OUT OPTIONAL
   int       timeout=OMITINT,    //IN OPTIONAL
   short    *segid=NULL)      //OUT OPTIONAL
;

_declspec(dllimport) short   BEGINTRANSACTION
  (int     *tag=NULL) //OUT OPTIONAL
                //TAG FOR RESUMETRANSACTION RETURNED HERE
;

_declspec(dllimport) int CANCEL (short    filenum) //IN
                                                         //FILE NUMBER
;

_declspec(dllimport) int CANCELREQ
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag=OMITINT)     //IN OPTIONAL
                    //TAG OF REQUEST TO CANCEL
;

_declspec(dllimport) int CANCELTIMEOUT
  (short    tleaddr) //IN
;

_declspec(dllimport) int     COMPUTEJULIANDAYNO
  (short     year,  //IN
   short     month, //IN
   short     day,   //IN
   short    *error=NULL) //OUT OPTIONAL
;

_declspec(dllimport) _int64   COMPUTETIMESTAMP (short   *date_n_time, //IN
                                                 short   *error=NULL)       //OUT OPTIONAL
;

_declspec(dllimport) short   COMPUTETRANSID (_int64   *transid,  //OUT
                                              int       System,   //IN
                                              short     cpu,      //IN
                                              int       sequence, //IN
                                              short     crashcount=OMITSHORT) // IN OPTIONAL
;

_declspec(dllimport) int CONTROL
  (short    filenum, //IN
                    //FILE NUMBER
   short    ctrlnum, //IN
                    //CONTROL NUMBER
   short    parm,    //IN
                    //PARAMETER
   int      tag=OMITINT)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) short   CONTROLMESSAGESYSTEM
  (short    actioncode, //IN
   fat_16   value)      //IN
;

_declspec(dllimport) _int64   CONVERTTIMESTAMP
  (_int64     fromtimestamp, //IN
   short      direction=OMITSHORT,     //IN OPTIONAL
   short      node=OMITSHORT,          //IN OPTIONAL
   short     *error=NULL)         //OUT OPTIONAL
;

_declspec(dllimport) short   DAYOFWEEK (int      jdn) //IN
                                          //Julian Day Number
;

_declspec(dllimport) void DELAY (int      time) //IN
;

_declspec(dllimport) short   DISK_REFRESH_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,     //IN
#else
  (char           *name,     //IN
#endif
   short           length_a) //IN
;

_declspec(dllimport) void  * DNUMIN
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *str,    //IN
#else
  (char           *str,    //IN
#endif
   int            *number, //OUT
   short           base,   //IN
   short          *result=NULL, //OUT OPTIONAL
   short           flags=OMITSHORT)  //IN OPTIONAL
;

_declspec(dllimport) short   DNUMOUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *buffer,  //OUT
#else
  (char           *buffer,  //OUT
#endif
   int             dnumber, //IN
   short           base,    //IN
   short           width=OMITSHORT,   //IN OPTIONAL
   short           flags=OMITSHORT)   //IN OPTIONAL
;

_declspec(dllimport) short   ENDTRANSACTION();

//ENDTRANSACTION_ERR() is same as ENDTRANSACTION(). However,
//errStr is allocated if errlen is not zero. Caller must deallocate errStr
//by calling DELLAOCATE_ERR.
//Rest of functionality same as ENDTRANSACTION.
_declspec(dllimport) short   ENDTRANSACTION_ERR(char *&errStr, int &errlen);
_declspec(dllimport) void    DEALLOCATE_ERR(char *&errStr);

_declspec(dllimport) short   FILENAME_COMPARE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name1,    //IN
#else
  (char           *name1,    //IN
#endif
   short           length_a, //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name2,    //IN
#else
   char           *name2,    //IN
#endif
   short           length_b) //IN
;

_declspec(dllimport) short   FILENAME_DECOMPOSE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,      //IN
#else
  (char           *name,      //IN
#endif
   short           length_a,  //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *piece,     //OUT
#else
   char           *piece,     //OUT
#endif
   short           length_b,  //IN
   short          *piece_len, //OUT
   short           level,     //IN
   short           options=OMITSHORT,   //IN OPTIONAL
   short           subpart=OMITSHORT)   //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_EDIT_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,     //IN/OUT
#else
  (char           *name,     //IN/OUT
#endif
   short           length_a, //IN
   short          *name_len, //IN/OUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *piece,    //IN
#else
   char           *piece,    //IN
#endif
   short           length_b, //IN
   short           level,    //IN
   short           options=OMITSHORT,  //IN OPTIONAL
   short           subpart=OMITSHORT)  //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_FINDFINISH_
  (short    search_id) //IN
;

_declspec(dllimport) short   FILENAME_FINDNEXT_
  (short           search_id,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name=NULL,        //OUT OPTIONAL
#else
   char           *name=NULL,        //OUT OPTIONAL
#endif
   short           length_a=OMITSHORT,    //IN OPTIONAL
   short          *name_len=NULL,    //OUT OPTIONAL
   short          *entity_info=NULL, //OUT OPTIONAL
                               //[0:4]
   int             tag=OMITINT)         //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_FINDSTART_
  (short          *search_id,     //OUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *srch_pat=NULL,      //IN OPTIONAL
#else
   char           *srch_pat=NULL,      //IN OPTIONAL
#endif
   short           length_a=OMITSHORT,      //IN OPTIONAL
   short           resolve_level=OMITSHORT, //IN OPTIONAL
   short           dev_type=OMITSHORT,      //IN OPTIONAL
   short           dev_subtype=OMITSHORT,   //IN OPTIONAL
   short           options=OMITSHORT,       //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *startname=NULL,     //IN OPTIONAL
#else
   char           *startname=NULL,     //IN OPTIONAL
#endif
   short           length_b=OMITSHORT)      //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_MATCH_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //IN
#else
  (char           *name,        //IN
#endif
   short           length_a,    //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *pattern,     //IN
#else
   char           *pattern,     //IN
#endif
   short           length_b,    //IN
   short          *generic_set=NULL) //OUT OPTIONAL
;

_declspec(dllimport) short   FILENAME_RESOLVE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *part_name,     //IN
#else
  (char           *part_name,     //IN
#endif
   short           length_a,      //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *full_name,     //OUT
#else
   char           *full_name,     //OUT
#endif
   short           length_b,      //IN
   short          *full_len,      //OUT
   short           options=OMITSHORT,       //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *override_name=NULL, //IN OPTIONAL
#else
   char           *override_name=NULL, //IN OPTIONAL
#endif
   short           length_c=OMITSHORT,      //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *search=NULL,        //IN OPTIONAL
#else
   char           *search=NULL,        //IN OPTIONAL
#endif
   short           length_d=OMITSHORT,      //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *defaults=NULL,      //IN OPTIONAL
#else
   char           *defaults=NULL,      //IN OPTIONAL
#endif
   short           length_e=OMITSHORT)      //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_SCAN_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,         //IN
#else
  (char           *name,         //IN
#endif
   short           length_a,     //IN
   short          *count=NULL,        //OUT OPTIONAL
   short          *kind=NULL,         //OUT OPTIONAL
   short          *entity_level=NULL, //OUT OPTIONAL
   short           options=OMITSHORT)    //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_TO_OLDFILENAME_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,     //IN
#else
  (char           *name,     //IN
#endif
   short           length_a, //IN
   short          *oldname)  //OUT
;

_declspec(dllimport) short   FILENAME_TO_PROCESSHANDLE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,           //IN
#else
  (char           *name,           //IN
#endif
   short           length_a,       //IN
   short          *process_handle) //OUT
                              //[0:9]
;

_declspec(dllimport) short   FILENAME_UNRESOLVE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *longname,  //IN
#else
  (char           *longname,  //IN
#endif
   short           length_a,  //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *shortname, //OUT
#else
   char           *shortname, //OUT
#endif
   short           length_b,  //IN
   short          *shortlen,  //OUT
   short           level=OMITSHORT,     //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *defaults=NULL,  //IN OPTIONAL
#else
   char           *defaults=NULL,  //IN OPTIONAL
#endif
   short           length_c=OMITSHORT)  //IN OPTIONAL
;

_declspec(dllimport) short   FILE_ALTERLIST_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //IN
#else
  (char           *name,        //IN
#endif
   short           length_a,    //IN
   short          *itemlist,    //IN
   short           numberitems, //IN
   short          *values,      //IN
   short           valueslen,   //IN
   short           partonly=OMITSHORT,    //IN OPTIONAL
   short          *erroritem=NULL)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_CLOSE_
  (short    filenum, //IN
   short    tape_disposition=OMITSHORT) //IN OPTIONAL
;

_declspec(dllimport) short   FILE_CREATELIST_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //IN/OUT
#else
  (char           *name,        //IN/OUT
#endif
   short           length_a,    //IN
   short          *namelen,     //IN/OUT
   short          *itemlist,    //IN
   short           numberitems, //IN
   short          *values,      //IN
   short           valueslen,   //IN
   short          *erroritem=NULL)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_CREATE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,            //IN/OUT
#else
  (char           *name,            //IN/OUT
#endif
   short           length_a,        //IN
   short          *namelen,         //IN/OUT
   short           filecode=OMITSHORT,        //IN OPTIONAL
   fat_16          pri_extent_size=OMITFAT_16, //IN OPTIONAL
   fat_16          sec_extent_size=OMITFAT_16, //IN OPTIONAL
   short           max_extents=OMITSHORT,     //IN OPTIONAL
   short           filetype=OMITSHORT,        //IN OPTIONAL
   fat_16          options=OMITFAT_16,         //IN OPTIONAL
   short           recordlen=OMITSHORT,       //IN OPTIONAL
   short           blocklen=OMITSHORT,        //IN OPTIONAL
   short           keylen=OMITSHORT,          //IN OPTIONAL
   short           keyoffset=OMITSHORT)       //IN OPTIONAL
;

_declspec(dllimport) short   FILE_GETINFOBYNAME_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,         //IN
#else
  (char           *name,         //IN
#endif
   short           length_a,     //IN
   short          *typeinfo=NULL,     //OUT OPTIONAL
                                //[0:4]
   short          *phys_rec_len=NULL, //OUT OPTIONAL
   short           options=OMITSHORT,      //IN OPTIONAL
   int             timeout=OMITINT,      //IN OPTIONAL
   short          *flags=NULL)        //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETINFOLISTBYNAME_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //IN
#else
  (char           *name,        //IN
#endif
   short           length_a,    //IN
   short          *itemlist,    //IN
   short           numberitems, //IN
   short          *result,      //OUT
   short           resultmax,   //IN
   short          *result_len=NULL,  //OUT OPTIONAL
   short          *erroritem=NULL)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETINFOLIST_
  (short     filenum,     //IN
   short    *itemlist,    //IN
   short     numberitems, //IN
   short    *result,      //OUT
   short     resultmax,   //IN
   short    *result_len=NULL,  //OUT OPTIONAL
   short    *erroritem=NULL)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETINFO_
  (short           filenum,      //IN
   short          *lasterror=NULL,    //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *filename=NULL,     //OUT OPTIONAL
#else
   char           *filename=NULL,     //OUT OPTIONAL
#endif
   short           length_a=OMITSHORT,     //IN OPTIONAL
   short          *filename_len=NULL, //OUT OPTIONAL
   short          *typeinfo=NULL,     //OUT OPTIONAL
                                //[0:4]
   short          *flags=NULL)        //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETLOCKINFO_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,            //IN
#else
  (char           *name,            //IN
#endif
   short           length_a,        //IN
   short          *phandle=NULL,         //IN OPTIONAL
                                   //[0:9]
   short          *transid=NULL,         //IN OPTIONAL
   short          *control=NULL,         //IN/OUT
                                   //[0:9]
   short          *lockdesc=NULL,        //OUT
   short           desclen=OMITSHORT,         //IN
   short          *participants=NULL,      //OUT
   short           maxparticipants=OMITSHORT, //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *lockedname=NULL,       //OUT OPTIONAL
#else
   char           *lockedname=NULL,       //OUT OPTIONAL
#endif
   short           length_b=OMITSHORT,        //IN OPTIONAL
   short          *lockednamelen=NULL)      //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETOPENINFO_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *searchname, //IN
#else
  (char           *searchname, //IN
#endif
   short           length_a,    //IN
   _int64         *prevtag,     //IN/OUT
   short          *pri_opener=NULL,  //OUT OPTIONAL
                               //[0:9]
   short          *back_opener=NULL, //OUT OPTIONAL
                               //[0:9]
   short          *accessmode=NULL,  //OUT OPTIONAL
   short          *exclusion=NULL,   //OUT OPTIONAL
   short          *syncdepth=NULL,   //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *filename=NULL,    //OUT OPTIONAL
#else
   char           *filename=NULL,    //OUT OPTIONAL
#endif
   short           length_b=OMITSHORT,    //IN OPTIONAL
   short          *flen=NULL,        //OUT OPTIONAL
   short          *accessid=NULL,    //OUT OPTIONAL
   short          *validmask=NULL)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETRECEIVEINFO_
  (short   *receiveinfo, //OUT
   short   *reserved=NULL)    //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_OPEN_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,              //IN
#else
  (char           *name,              //IN
#endif
   short           length_a,          //IN
   short          *filenum,           //IN/OUT
   short           access=OMITSHORT,            //IN OPTIONAL
   short           exclusion=OMITSHORT,         //IN OPTIONAL
   short           nowait=OMITSHORT,            //IN OPTIONAL
   short           sync_rec_depth=OMITSHORT,    //IN OPTIONAL
   fat_16          options=OMITFAT_16,           //IN OPTIONAL
   short           seq_block_buf_id=OMITSHORT,   //IN OPTIONAL
   short           seq_block_buf_len=OMITSHORT,   //IN OPTIONAL
   short          *primary_phandle=NULL)        //IN OPTIONAL
                                     //[0:9]
;

_declspec(dllimport) short   FILE_PURGE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,     //IN
#else
  (char           *name,     //IN
#endif
   short           length_a) //IN
;

_declspec(dllimport) short   FILE_RENAME_
  (short           filenum,  //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,     //IN
#else
   char           *name,     //IN
#endif
   short           length_a) //IN
;

_declspec(dllimport) int GETSYNCINFO
  (short     filenum,  //IN
                      //FILE NUMBER
   short    *syncinfo, //OUT
                      //ARRAY FOR SYNCHRONIZATION INFORMATION
   short    *infosize=NULL) //OUT OPTIONAL
                      //SIZE OF SYNCINFO ARRAY
;

_declspec(dllimport) short   GETSYSTEMNAME
  (short     sysid,   //IN
   short    *sysname) //OUT
;

_declspec(dllimport) short   GETTMPNAME (short   *devname) //OUT
                                                         //12-WORD ARRAY TO WHICH TMP NAME IS RETURNED
;

_declspec(dllimport) short   GETTRANSID (short   *transid) //OUT
                                                         //ARRAY TO WHICH TRANSID IS RETURNED
;

_declspec(dllimport)  int INTERPRETINTERVAL (_int64    time,               // IN
                                             short    *hours = NULL,       // OUT
                                             short    *minutes = NULL,     // OUT
                                             short    *seconds = NULL,     // OUT
                                             short    *milsecs = NULL,     // OUT
                                             short    *microsecs = NULL);  // OUT

_declspec(dllimport) void INTERPRETJULIANDAYNO (int       juliandayno, //IN
                                      short    *year,        //OUT
                                      short    *month,       //OUT
                                      short    *day)         //OUT
;

_declspec(dllimport) int     INTERPRETTIMESTAMP (_int64     juliantimestamp, //IN
                                      short     *date_n_time)     //OUT
;

_declspec(dllimport) short   INTERPRETTRANSID (_int64     transid,  //IN
                                                int       *System,   //OUT
                                                short     *cpu,      //OUT
                                                int       *sequence, //OUT
                                                short     *crashcount=NULL) // OUT OPTIONAL
;

_declspec(dllimport) _int64   JULIANTIMESTAMP
  (short     type=OMITSHORT,  //IN OPTIONAL
   short    *tuid=NULL,  //OUT OPTIONAL
   short    *error=NULL, //OUT OPTIONAL
   short     node=OMITSHORT)  //IN OPTIONAL
;

_declspec(dllimport) int KEYPOSITIONX
  (short           filenum, //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *key,     //IN
#else
   char           *key,     //IN
#endif
   short           keytag=OMITSHORT,  //IN OPTIONAL
   fat_16          keylen=OMITFAT_16,  //IN OPTIONAL
   fat_16          postype=OMITFAT_16) //IN OPTIONAL
;

_declspec(dllimport) int LOCKFILE
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag=OMITINT)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) int LOCKREC
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag=OMITINT)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) short   MESSAGESTATUS
  (short    msgnum=OMITSHORT) //IN OPTIONAL
;

_declspec(dllimport) short   MESSAGESYSTEMINFO
  (short     itemcode, //IN
   short    *value)    //OUT
;

_declspec(dllimport) void MONITORCPUS (short    mask) //IN
;

_declspec(dllimport) short   MYSYSTEMNUMBER();

_declspec(dllimport) short   NODENAME_TO_NODENUMBER_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *sysname=NULL,    //IN OPTIONAL
#else
  (char           *sysname=NULL,    //IN OPTIONAL
#endif
   short           sysnamelen=OMITSHORT, //IN OPTIONAL
   int            *nodenumber=NULL) //OUT
;

_declspec(dllimport) short   NODENUMBER_TO_NODENAME_
  (int             sysnum=OMITINT,  //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *sysname=NULL, //OUT
#else
   char           *sysname=NULL, //OUT
#endif
   short           maxlen=OMITSHORT,  //IN
   short          *syslen=NULL)  //OUT
;

#ifdef TDM_ROSETTA_COMPATIBILITY_
_declspec(dllimport) baddr NUMIN (unsigned char  *str,    //IN
#else
_declspec(dllimport) baddr NUMIN (char           *str,    //IN
#endif
                                                         //STRING WHERE NUMBER STARTS
                                  short          *number, //OUT
                                                         //NUMERIC VALUE RETURNED HERE
                                  short           base,   //IN
                                                         //DEFAULT CONVERSION BASE, A % FORCES 8
                                  short          *result) //OUT
                                                         //1: STR DOES NOT LOOK LIKE A NUMBER
                                                         //0: LEGAL NUMBER
                                                         //-1: ILLEGAL NUMBER
;

#ifdef TDM_ROSETTA_COMPATIBILITY_
_declspec(dllimport) void NUMOUT (unsigned char  *str,    //OUT
#else
_declspec(dllimport) void NUMOUT (char           *str,    //OUT
#endif
                                                         //BYTE STRING FOR ASCII NUMBER
                                  short           number, //IN
                                                         //LOGICAL 16 BIT NUMERIC VALUE
                                  short           base,   //IN
                                                         //CONVERSION BASE, 2-10 ALLOWED
                                  short           width)  //IN
                                                         //CONVERTED NUMBER WILL OCCUPY STR TO
                                                         //STR [ WIDTH - 1 ]
;

_declspec(dllimport) short   OLDFILENAME_TO_FILENAME_
  (short          *oldname,  //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,     //OUT
#else
   char           *name,     //OUT
#endif
   short           length_a, //IN
   short          *name_len) //OUT
;

_declspec(dllimport) int TDM_POSITION_ (short    filenum, //IN
                                                           //FILE NUMBER
                                          int      address) //IN
                                                           //NEW VALUE OF LOGICAL DATA PTR
                                                           //(-1D TO SET TO EOF)
;

_declspec(dllimport) short   PROCESSHANDLE_COMPARE_
  (short   *phandle1, //IN
                     //[0:9]
   short   *phandle2) //IN
                     //[0:9]
;

_declspec(dllimport) short   PROCESSHANDLE_DECOMPOSE_
  (short          *process_handle, //IN
                                  //[0:9]
   short          *cpu=NULL,            //OUT OPTIONAL
   short          *pin=NULL,            //OUT OPTIONAL
   int            *nodenumber=NULL,       //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *nodename=NULL,       //OUT OPTIONAL
#else
   char           *nodename=NULL,       //OUT OPTIONAL
#endif
   short           length_a=OMITSHORT,       //IN OPTIONAL
   short          *namelen=NULL,        //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *procname=NULL,       //OUT OPTIONAL
#else
   char           *procname=NULL,       //OUT OPTIONAL
#endif
   short           length_b=OMITSHORT,       //IN OPTIONAL
   short          *plen=NULL,           //OUT OPTIONAL
   _int64         *seqno=NULL)          //OUT OPTIONAL
;

_declspec(dllimport) short   PROCESSHANDLE_GETMINE_
  (short   *phandle) //OUT
;

_declspec(dllimport) short   PROCESSHANDLE_NULLIT_
  (short   *prochand) //OUT
                     // PROCESSHANDLE
;

_declspec(dllimport) short   PROCESSHANDLE_TO_FILENAME_
  (short          *process_handle, //IN
                                  //[0:9]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,           //OUT
#else
   char           *name,           //OUT
#endif
   short           length_a,       //IN
   short          *name_len,       //OUT
   short           options=OMITSHORT)        //IN OPTIONAL
;

_declspec(dllimport) short   PROCESSHANDLE_TO_STRING_
  (short          *phandle,    //IN
                              //[0:9]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *pstring,    //OUT
#else
   char           *pstring,    //OUT
#endif
   short           length_a,   //IN
   short          *pstringlen, //OUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *System=NULL,     //IN OPTIONAL
#else
   char           *System=NULL,     //IN OPTIONAL
#endif
   short           length_b=OMITSHORT,   //IN OPTIONAL
   short           namedform=OMITSHORT)  //IN OPTIONAL
;

_declspec(dllimport) short   PROCESSNAME_CREATE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //OUT
#else
  (char           *name,        //OUT
#endif
   short           maxnamelen,  //IN
   short          *namelen,     //OUT
   short           name_type=OMITSHORT,   //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *nodename=NULL,    //IN OPTIONAL
#else
   char           *nodename=NULL,    //IN OPTIONAL
#endif
   short           nodenamelen=OMITSHORT, //IN OPTIONAL
   short           options=OMITSHORT)     //IN OPTIONAL
;

_declspec(dllimport) int     PROCESSORSTATUS();

_declspec(dllimport) short   PROCESSSTRING_SCAN_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *process_string, //IN
#else
  (char           *process_string, //IN
#endif
   short           length_a, //IN
   short          *length_used=NULL, //OUT OPTIONAL
   short          *process_handle=NULL, //OUT OPTIONAL
                              //[0:9]
   short          *string_kind=NULL, //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name=NULL,           //OUT OPTIONAL
#else
   char           *name=NULL,           //OUT OPTIONAL
#endif
   short           length_b=OMITSHORT,       //IN OPTIONAL
   short          *name_len=NULL,       //OUT OPTIONAL
   short          *cpu=NULL,            //OUT OPTIONAL
   short          *pin=NULL,            //OUT OPTIONAL
   short           options=OMITSHORT)        //IN OPTIONAL
;

_declspec(dllimport) short   PROCESS_CREATE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *program_file=NULL, //IN OPTIONAL
#else
  (char           *program_file=NULL, //IN OPTIONAL
#endif
   short           length_a=OMITSHORT, //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *library_file=NULL, //IN OPTIONAL
#else
   char           *library_file=NULL, //IN OPTIONAL
#endif
   short           length_b=OMITSHORT,          //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *swap_file=NULL,         //IN OPTIONAL
#else
   char           *swap_file=NULL,         //IN OPTIONAL
#endif
   short           length_c=OMITSHORT,          //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *ext_swap_file=NULL,         //IN OPTIONAL
#else
   char           *ext_swap_file=NULL,         //IN OPTIONAL
#endif
   short           length_d=OMITSHORT,          //IN OPTIONAL
   short           priority=OMITSHORT,          //IN OPTIONAL
   short           processor=OMITSHORT,         //IN OPTIONAL
   short          *phandle=NULL,           //OUT OPTIONAL
                                     //[0:9]
   short          *error_detail=NULL,        //OUT OPTIONAL
   short           name_option=OMITSHORT,       //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name=NULL,              //IN OPTIONAL
#else
   char           *name=NULL,              //IN OPTIONAL
#endif
   short           length_e=OMITSHORT,          //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *process_descr=NULL,         //OUT OPTIONAL
#else
   char           *process_descr=NULL,         //OUT OPTIONAL
#endif
   short           length_f=OMITSHORT,          //IN OPTIONAL
   short          *process_descr_len=NULL, //OUT OPTIONAL
   int             nowait_tag=OMITINT,        //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *hometerm=NULL,          //IN OPTIONAL
#else
   char           *hometerm=NULL,          //IN OPTIONAL
#endif
   short           length_g=OMITSHORT,          //IN OPTIONAL
   short           memory_pages=OMITSHORT,      //IN OPTIONAL
   short           jobid=OMITSHORT,             //IN OPTIONAL
   short           create_options=OMITSHORT,    //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *defines=NULL,           //IN OPTIONAL
#else
   char           *defines=NULL,           //IN OPTIONAL
#endif
   short           length_h=OMITSHORT,          //IN OPTIONAL
   short           debug_options=OMITSHORT,     //IN OPTIONAL
   int             pfs_size=OMITINT,          //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *reserved1=NULL,         //IN OPTIONAL
#else
   char           *reserved1=NULL,         //IN OPTIONAL
#endif
                                     //RESERVED FOR INTERNAL USE
   short           reserved2=OMITSHORT)         //IN OPTIONAL
                                     //RESERVED FOR INTERNAL USE
;

_declspec(dllimport) short   PROCESS_GETINFO_
  (short          *phandle=NULL,             //IN/OUT OPTIONAL
                                       //[0:9]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *process_desc=NULL,           //OUT OPTIONAL
#else
   char           *process_desc=NULL,           //OUT OPTIONAL
#endif
   short           process_desc_maxlen=OMITSHORT,     //IN OPTIONAL
   short          *process_desc_len=NULL,          //OUT OPTIONAL
   short          *priority=NULL,            //OUT OPTIONAL
   short          *moms_phandle=NULL,          //OUT OPTIONAL
                                       //[0:9]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *hometerm=NULL,            //OUT OPTIONAL
#else
   char           *hometerm=NULL,            //OUT OPTIONAL
#endif
   short           hometerm_maxlen=OMITSHORT,     //IN OPTIONAL
   short          *hometerm_len=NULL,        //OUT OPTIONAL
   _int64         *process_time=NULL,        //OUT OPTIONAL
   short          *creator_access_id=NULL,   //OUT OPTIONAL
   short          *process_access_id=NULL,   //OUT OPTIONAL
   short          *gmoms_phandle=NULL,   //OUT OPTIONAL
                                       //[0:9]
   short          *jobid=NULL,               //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *program_file=NULL,           //OUT OPTIONAL
#else
   char           *program_file=NULL,           //OUT OPTIONAL
#endif
   short           program_file_maxlen=OMITSHORT,     //IN OPTIONAL
   short          *program_file_len=NULL,          //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *swap_file=NULL,           //OUT OPTIONAL
#else
   char           *swap_file=NULL,           //OUT OPTIONAL
#endif
   short           swap_file_maxlen=OMITSHORT,     //IN OPTIONAL
   short          *swap_file_len=NULL,          //OUT OPTIONAL
   short          *error_detail=NULL,          //OUT OPTIONAL
   short          *process_type=NULL,        //OUT OPTIONAL
   int            *oss_pid=NULL)           //OUT OPTIONAL
;

_declspec(dllimport) short   PROCESS_STOP_
  (short          *phandle=NULL,          //IN OPTIONAL
                                    //[0:9]
   short           specifier=OMITSHORT,        //IN OPTIONAL
   short           options=OMITSHORT,          //IN OPTIONAL
   short           compl_code=OMITSHORT,       //IN OPTIONAL
   short           termination_info=OMITSHORT, //IN OPTIONAL
   short          *spi_ssid=NULL,         //IN OPTIONAL
                                    //[0:5]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *ascii_text=NULL,        //IN OPTIONAL
#else
   char           *ascii_text=NULL,        //IN OPTIONAL
#endif
   short           length_a=OMITSHORT)         //IN OPTIONAL
;

_declspec(dllimport) int READLOCKX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //OUT
#else
   void           *buf,       //OUT
#endif
   unsigned short  count,     //IN
   unsigned short *countread=NULL, //OUT OPTIONAL
   int             tag=OMITINT)       //IN OPTIONAL
;

_declspec(dllimport) int READUPDATELOCKX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //OUT
#else
   void           *buf,       //OUT
#endif
   unsigned short  count,     //IN
   unsigned short *countread=NULL, //OUT OPTIONAL
   int             tag=OMITINT)       //IN OPTIONAL
;

_declspec(dllimport) int READUPDATEX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //OUT
#else
   void           *buf,       //OUT
#endif
   unsigned short  count,     //IN
   unsigned short *countread=NULL, //OUT OPTIONAL
   int             tag=OMITINT)       //IN OPTIONAL
;

_declspec(dllimport) int READX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //OUT
#else
   void           *buf,       //OUT
#endif
   unsigned short  count,     //IN
   unsigned short *countread=NULL, //OUT OPTIONAL
   int             tag=OMITINT)       //IN OPTIONAL
;

_declspec(dllimport) int REPLYX
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *buf=NULL,          //IN OPTIONAL
#else
  (void           *buf=NULL,          //IN OPTIONAL
#endif
   unsigned short  count=OMITUNSIGNEDSHORT,        //IN OPTIONAL
   unsigned short *countwritten=NULL, //OUT OPTIONAL
   short           replynum=OMITSHORT,     //IN OPTIONAL
   short           errret=OMITSHORT)       //IN OPTIONAL
;

_declspec(dllimport) int REPOSITION (short     filenum,  //IN
                                                               //FILE NUMBER
                                            short    *position) //IN
                                                               //ARRAY FOR POSITION INFORMATION
;

_declspec(dllimport) short   RESUMETRANSACTION (int      tag) //IN
                                                            //TAG RETURNED FROM BEGINTRANSACTION
;

_declspec(dllimport) int SAVEPOSITION
  (short     filenum,  //IN
                      //FILE NUMBER
   short    *position, //OUT
                      //ARRAY FOR POSITION INFORMATION
   short    *infosize=NULL) //OUT OPTIONAL
                      //SIZE OF SYNCINFO ARRAY
;

_declspec(dllimport) short   SERVERCLASS_DIALOG_ABORT_
  (int      dialogid) //IN
;

_declspec(dllimport) short   SERVERCLASS_DIALOG_BEGIN_
  (int            *dialogid,           //OUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *pmname,             //IN
#else
   char           *pmname,             //IN
#endif
   short           pmnamelen,          //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *scname,             //IN
#else
   char           *scname,             //IN
#endif
   short           scnamelen,          //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *messagebuffer,          //IN/OUT
#else
   void           *messagebuffer,          //IN/OUT
#endif
   unsigned short  requestbytes,       //IN
   short           maxreplybytes,      //IN
   unsigned short *actualreplybytes=NULL,         //OUT OPTIONAL
   int             timeout=OMITINT,            //IN OPTIONAL
   fat_16          flags=OMITFAT_16,              //IN OPTIONAL
   short          *scsoperationnumber=NULL, //OUT OPTIONAL
   int             tag=OMITINT)        //IN OPTIONAL
;

_declspec(dllimport) short   SERVERCLASS_DIALOG_END_
  (int      dialogid) //IN
;

_declspec(dllimport) short   SERVERCLASS_DIALOG_SEND_
  (int             dialogid, //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *messagebuffer, //IN/OUT
#else
   void           *messagebuffer, //IN/OUT
#endif
   unsigned short  requestbytes,       //IN
   short           maxreplybytes,      //IN
   unsigned short *actualreplybytes=NULL,         //OUT OPTIONAL
   int             timeout=OMITINT,            //IN OPTIONAL
   fat_16          flags=OMITFAT_16,              //IN OPTIONAL
   short          *scsoperationnumber=NULL, //OUT OPTIONAL
   int             tag=OMITINT)        //IN OPTIONAL
;

_declspec(dllimport) short   SERVERCLASS_SEND_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *pathmon,            //IN
#else
  (char           *pathmon,            //IN
#endif
   short           pathmonbytes,       //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *serverclass,          //IN
#else
   char           *serverclass,          //IN
#endif
   short           serverclassbytes,    //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *messagebuffer,          //IN/OUT
#else
   void           *messagebuffer,          //IN/OUT
#endif
   unsigned short  requestbytes,       //IN
   unsigned short  maximumreplybytes,    //IN
   unsigned short *actualreplybytes=NULL,         //OUT OPTIONAL
   int             timeout=OMITINT,            //IN OPTIONAL
   fat_16          flags=OMITFAT_16,              //IN OPTIONAL
   short          *scsoperationnumber=NULL, //OUT OPTIONAL
   int             tag=OMITINT)        //IN OPTIONAL
;

_declspec(dllimport) short   SERVERCLASS_SEND_INFO_
  (short   *serverclasserror, //OUT
   short   *filesystemerror) //OUT
;

_declspec(dllimport) int SETMODE
  (short     filenum, //IN
                     //FILE NUMBER
   short     modenum, //IN
                     //NUMBER OF CHARACTERISTIC TO BE CHANGED
   fat_16    parm1=OMITFAT_16,   //IN OPTIONAL
                     //FIRST PARAMETER
   fat_16    parm2=OMITFAT_16,   //IN OPTIONAL
                     //SECOND PARAMETER
   short    *oldval=NULL)  //OUT OPTIONAL
                     //ARRAY FOR RETURNING OLD VALUE
;

_declspec(dllimport) int SETMODENOWAIT
  (short     filenum, //IN
                     //FILE NUMBER
   short     modenum, //IN
                     //NUMBER OF CHARACTERISTIC TO BE CHANGED
   fat_16    parm1=OMITFAT_16,   //IN OPTIONAL
                     //FIRST PARAMETER
   fat_16    parm2=OMITFAT_16,   //IN OPTIONAL
                     //SECOND PARAMETER
   short    *oldval=NULL,  //OUT OPTIONAL
                     //ARRAY FOR RETURNING OLD VALUE
   int       tag=OMITINT)     //IN OPTIONAL
                     //SPECIFIED TAG FOR NOWAIT REQUESTS
;

_declspec(dllimport) int SETPARAM
  (short     filenum,      //IN
   short     function,     //IN
   short    *param=NULL,        //IN OPTIONAL
   short     paramsize=OMITSHORT,    //IN
   short    *oldparam=NULL,     //OUT OPTIONAL
   short    *oldparamsize=NULL, //OUT OPTIONAL
   short     oldparammax=OMITSHORT,  //IN OPTIONAL
   int       tag=OMITINT)          //IN OPTIONAL
;

_declspec(dllimport) short   SETSTOP (short    stopmode) //IN
                                                                 //NEW STOP MODE
;

#ifdef TDM_ROSETTA_COMPATIBILITY_
_declspec(dllimport) void SHIFTSTRING (unsigned char  *bytestring, //IN/OUT
#else
_declspec(dllimport) void SHIFTSTRING (char           *bytestring, //IN/OUT
#endif
                                                                  //String to be shifted
                                       short           bytecount,  //IN
                                                                  //Number of BYTES in String
                                       short           casebit)    //IN
                                                                  //.<15> = 0 -> UPSHIFT,
;

_declspec(dllimport) int SIGNALTIMEOUT
  (int       toval,   //IN
   short     parm1=OMITSHORT,   //IN OPTIONAL
   int       parm2=OMITINT,   //IN OPTIONAL
   short    *tleaddr=NULL) //OUT OPTIONAL
;


_declspec(dllimport) short   STRING_UPSHIFT_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *in_string,  //IN
#else
  (char           *in_string,  //IN
#endif
   short           length_a,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *out_string, //OUT
#else
   char           *out_string, //OUT
#endif
   short           length_b)  //IN
;

_declspec(dllimport) short   TEXTTOTRANSID
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *text,        //IN
#else
  (char           *text,        //IN
#endif
   short           textbytelen, //IN
   _int64         *transid)     //OUT
;

_declspec(dllimport) void TMF_VERSION_ (int       sysnum,  //IN
                                                          // System Number of the system to interrogate
                                        short    *version, //OUT
                                                          // Version number of the TMP process
                                        short    *error)   //OUT
                                                          // File System Error Code
;

_declspec(dllimport) short   TOSVERSION();

_declspec(dllimport) short   TRANSIDTOTEXT (_int64     transid,     //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
                                             unsigned char 
#else
                                             char          
#endif
                                                      *text,        //OUT
                                             short      textbytelen, //IN
                                             short     *bytesused)   //OUT
;

_declspec(dllimport) int UNLOCKFILE
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag=OMITINT)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) int UNLOCKREC
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag=OMITINT)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) int WRITEREADX
  (short           filenum,    //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,        //IN/OUT
#else
   void           *buf,        //IN/OUT
#endif
   unsigned short  writecount, //IN
   unsigned short  readcount,  //IN
   unsigned short *countread=NULL,  //OUT OPTIONAL
   int             tag=OMITINT)        //IN OPTIONAL
;

_declspec(dllimport) int WRITEUPDATEUNLOCKX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //IN
#else
   void           *buf,       //IN
#endif
   unsigned short  count,     //IN
   unsigned short *countread=NULL, //OUT OPTIONAL
   int             tag=OMITINT)       //IN OPTIONAL
;

_declspec(dllimport) int WRITEUPDATEX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //IN
#else
   void           *buf,       //IN
#endif
   unsigned short  count,     //IN
   unsigned short *countread=NULL, //OUT OPTIONAL
   int             tag=OMITINT)       //IN OPTIONAL
;

_declspec(dllimport) int WRITEX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //IN
#else
   void           *buf,       //IN
#endif
   unsigned short  count,     //IN
   unsigned short *countread=NULL, //OUT OPTIONAL
   int             tag=OMITINT)       //IN OPTIONAL
;


_declspec(dllimport) short TDM_NTAWAITIOX_( 
 short 	*filenum ,	// IN/OUT
 void 	**bufptr =NULL,	// OUT OPTIONAL
 unsigned short *xfercount=NULL, 	// OUT OPTIONAL
 int   *tag	 =NULL,	// OUT OPTIONAL
 int	timeout	 =OMITINT,	// IN OPTIONAL
 int	eventCount=OMITINT,	//IN OPTIONAL
 void** eventList=NULL,	//IN OPTIONAL
 int	*eventIndex=NULL,	//OUT OPTIONAL
 int	dwWaitMask=OMITINT,	//IN OPTIONAL
 int	alertable=OMITINT,	//IN OPTIONAL
 int	*NtError=NULL)	//OUT OPTIONAL
;
_declspec(dllimport) short SETTMFAFFINITY
  (const short TmfSegmentNumber //IN
  )
;
}

#else /* __cplusplus */

_declspec(dllimport) short   ABORTTRANSACTION();

_declspec(dllimport) int ACTIVATERECEIVETRANSID
  (short    msgnum) //IN
;

_declspec(dllimport) int AWAITIOX
  (short    *filenum_,   //IN/OUT
   void  * *bufptr,     //OUT OPTIONAL
   unsigned short    *xfercount_, //OUT OPTIONAL
   int      *tag,        //OUT OPTIONAL
   int       timeout,    //IN OPTIONAL
   short    *segid)      //OUT OPTIONAL
;

_declspec(dllimport) short   BEGINTRANSACTION
  (int     *tag) //OUT OPTIONAL
                //TAG FOR RESUMETRANSACTION RETURNED HERE
;

_declspec(dllimport) int CANCEL (short    filenum) //IN
                                                         //FILE NUMBER
;

_declspec(dllimport) int CANCELREQ
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag)     //IN OPTIONAL
                    //TAG OF REQUEST TO CANCEL
;

_declspec(dllimport) int CANCELTIMEOUT
  (short    tleaddr) //IN
;

_declspec(dllimport) int     COMPUTEJULIANDAYNO
  (short     year,  //IN
   short     month, //IN
   short     day,   //IN
   short    *error) //OUT OPTIONAL
;

_declspec(dllimport) _int64   COMPUTETIMESTAMP (short   *date_n_time, //IN
                                                 short   *error)       //OUT OPTIONAL
;

_declspec(dllimport) short   COMPUTETRANSID (_int64   *transid,  //OUT
                                              int       System,   //IN
                                              short     cpu,      //IN
                                              int       sequence, //IN
                                              short     crashcount) // IN OPTIONAL
;

_declspec(dllimport) int CONTROL
  (short    filenum, //IN
                    //FILE NUMBER
   short    ctrlnum, //IN
                    //CONTROL NUMBER
   short    parm,    //IN
                    //PARAMETER
   int      tag)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) short   CONTROLMESSAGESYSTEM
  (short    actioncode, //IN
   fat_16   value)      //IN
;

_declspec(dllimport) _int64   CONVERTTIMESTAMP
  (_int64     fromtimestamp, //IN
   short      direction,     //IN OPTIONAL
   short      node,          //IN OPTIONAL
   short     *error)         //OUT OPTIONAL
;

_declspec(dllimport) short   DAYOFWEEK (int      jdn) //IN
                                          //Julian Day Number
;

_declspec(dllimport) void DELAY (int      time) //IN
;

_declspec(dllimport) short   DISK_REFRESH_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,     //IN
#else
  (char           *name,     //IN
#endif
   short           length_a) //IN
;

_declspec(dllimport) void  * DNUMIN
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *str,    //IN
#else
  (char           *str,    //IN
#endif
   int            *number, //OUT
   short           base,   //IN
   short          *result, //OUT OPTIONAL
   short           flags)  //IN OPTIONAL
;

_declspec(dllimport) short   DNUMOUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *buffer,  //OUT
#else
  (char           *buffer,  //OUT
#endif
   int             dnumber, //IN
   short           base,    //IN
   short           width,   //IN OPTIONAL
   short           flags)   //IN OPTIONAL
;

_declspec(dllimport) short   ENDTRANSACTION();

//errStr is allocated if errlen is not zero. Caller must deallocate errStr.
//Rest of functionality same as ENDTRANSACTION();
_declspec(dllimport) short   ENDTRANSACTION_ERR(char *&errStr, int &errlen);


_declspec(dllimport) short   FILENAME_COMPARE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name1,    //IN
#else
  (char           *name1,    //IN
#endif
   short           length_a, //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name2,    //IN
#else
   char           *name2,    //IN
#endif
   short           length_b) //IN
;

_declspec(dllimport) short   FILENAME_DECOMPOSE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,      //IN
#else
  (char           *name,      //IN
#endif
   short           length_a,  //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *piece,     //OUT
#else
   char           *piece,     //OUT
#endif
   short           length_b,  //IN
   short          *piece_len, //OUT
   short           level,     //IN
   short           options,   //IN OPTIONAL
   short           subpart)   //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_EDIT_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,     //IN/OUT
#else
  (char           *name,     //IN/OUT
#endif
   short           length_a, //IN
   short          *name_len, //IN/OUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *piece,    //IN
#else
   char           *piece,    //IN
#endif
   short           length_b, //IN
   short           level,    //IN
   short           options,  //IN OPTIONAL
   short           subpart)  //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_FINDFINISH_
  (short    search_id) //IN
;

_declspec(dllimport) short   FILENAME_FINDNEXT_
  (short           search_id,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,        //OUT OPTIONAL
#else
   char           *name,        //OUT OPTIONAL
#endif
   short           length_a,    //IN OPTIONAL
   short          *name_len,    //OUT OPTIONAL
   short          *entity_info, //OUT OPTIONAL
                               //[0:4]
   int             tag)         //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_FINDSTART_
  (short          *search_id,     //OUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *srch_pat,      //IN OPTIONAL
#else
   char           *srch_pat,      //IN OPTIONAL
#endif
   short           length_a,      //IN OPTIONAL
   short           resolve_level, //IN OPTIONAL
   short           dev_type,      //IN OPTIONAL
   short           dev_subtype,   //IN OPTIONAL
   short           options,       //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *startname,     //IN OPTIONAL
#else
   char           *startname,     //IN OPTIONAL
#endif
   short           length_b)      //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_MATCH_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //IN
#else
  (char           *name,        //IN
#endif
   short           length_a,    //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *pattern,     //IN
#else
   char           *pattern,     //IN
#endif
   short           length_b,    //IN
   short          *generic_set) //OUT OPTIONAL
;

_declspec(dllimport) short   FILENAME_RESOLVE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *part_name,     //IN
#else
  (char           *part_name,     //IN
#endif
   short           length_a,      //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *full_name,     //OUT
#else
   char           *full_name,     //OUT
#endif
   short           length_b,      //IN
   short          *full_len,      //OUT
   short           options,       //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *override_name, //IN OPTIONAL
#else
   char           *override_name, //IN OPTIONAL
#endif
   short           length_c,      //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *search,        //IN OPTIONAL
#else
   char           *search,        //IN OPTIONAL
#endif
   short           length_d,      //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *defaults,      //IN OPTIONAL
#else
   char           *defaults,      //IN OPTIONAL
#endif
   short           length_e)      //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_SCAN_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,         //IN
#else
  (char           *name,         //IN
#endif
   short           length_a,     //IN
   short          *count,        //OUT OPTIONAL
   short          *kind,         //OUT OPTIONAL
   short          *entity_level, //OUT OPTIONAL
   short           options)    //IN OPTIONAL
;

_declspec(dllimport) short   FILENAME_TO_OLDFILENAME_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,     //IN
#else
  (char           *name,     //IN
#endif
   short           length_a, //IN
   short          *oldname)  //OUT
;

_declspec(dllimport) short   FILENAME_TO_PROCESSHANDLE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,           //IN
#else
  (char           *name,           //IN
#endif
   short           length_a,       //IN
   short          *process_handle) //OUT
                              //[0:9]
;

_declspec(dllimport) short   FILENAME_UNRESOLVE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *longname,  //IN
#else
  (char           *longname,  //IN
#endif
   short           length_a,  //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *shortname, //OUT
#else
   char           *shortname, //OUT
#endif
   short           length_b,  //IN
   short          *shortlen,  //OUT
   short           level,     //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *defaults,  //IN OPTIONAL
#else
   char           *defaults,  //IN OPTIONAL
#endif
   short           length_c)  //IN OPTIONAL
;

_declspec(dllimport) short   FILE_ALTERLIST_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //IN
#else
  (char           *name,        //IN
#endif
   short           length_a,    //IN
   short          *itemlist,    //IN
   short           numberitems, //IN
   short          *values,      //IN
   short           valueslen,   //IN
   short           partonly,    //IN OPTIONAL
   short          *erroritem)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_CLOSE_
  (short    filenum, //IN
   short    tape_disposition) //IN OPTIONAL
;

_declspec(dllimport) short   FILE_CREATELIST_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //IN/OUT
#else
  (char           *name,        //IN/OUT
#endif
   short           length_a,    //IN
   short          *namelen,     //IN/OUT
   short          *itemlist,    //IN
   short           numberitems, //IN
   short          *values,      //IN
   short           valueslen,   //IN
   short          *erroritem)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_CREATE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,            //IN/OUT
#else
  (char           *name,            //IN/OUT
#endif
   short           length_a,        //IN
   short          *namelen,         //IN/OUT
   short           filecode,        //IN OPTIONAL
   fat_16          pri_extent_size, //IN OPTIONAL
   fat_16          sec_extent_size, //IN OPTIONAL
   short           max_extents,     //IN OPTIONAL
   short           filetype,        //IN OPTIONAL
   fat_16          options,         //IN OPTIONAL
   short           recordlen,       //IN OPTIONAL
   short           blocklen,        //IN OPTIONAL
   short           keylen,          //IN OPTIONAL
   short           keyoffset)       //IN OPTIONAL
;

_declspec(dllimport) short   FILE_GETINFOBYNAME_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,         //IN
#else
  (char           *name,         //IN
#endif
   short           length_a,     //IN
   short          *typeinfo,     //OUT OPTIONAL
                                //[0:4]
   short          *phys_rec_len, //OUT OPTIONAL
   short           options,      //IN OPTIONAL
   int             timeout,      //IN OPTIONAL
   short          *flags)        //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETINFOLISTBYNAME_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //IN
#else
  (char           *name,        //IN
#endif
   short           length_a,    //IN
   short          *itemlist,    //IN
   short           numberitems, //IN
   short          *result,      //OUT
   short           resultmax,   //IN
   short          *result_len,  //OUT OPTIONAL
   short          *erroritem)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETINFOLIST_
  (short     filenum,     //IN
   short    *itemlist,    //IN
   short     numberitems, //IN
   short    *result,      //OUT
   short     resultmax,   //IN
   short    *result_len,  //OUT OPTIONAL
   short    *erroritem)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETINFO_
  (short           filenum,      //IN
   short          *lasterror,    //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *filename,     //OUT OPTIONAL
#else
   char           *filename,     //OUT OPTIONAL
#endif
   short           length_a,     //IN OPTIONAL
   short          *filename_len, //OUT OPTIONAL
   short          *typeinfo,     //OUT OPTIONAL
                                //[0:4]
   short          *flags)        //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETLOCKINFO_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,            //IN
#else
  (char           *name,            //IN
#endif
   short           length_a,        //IN
   short          *phandle,         //IN OPTIONAL
                                   //[0:9]
   short          *transid,         //IN OPTIONAL
   short          *control,         //IN/OUT
                                   //[0:9]
   short          *lockdesc,        //OUT
   short           desclen,         //IN
   short          *participants,      //OUT
   short           maxparticipants, //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *lockedname,       //OUT OPTIONAL
#else
   char           *lockedname,       //OUT OPTIONAL
#endif
   short           length_b,        //IN OPTIONAL
   short          *lockednamelen)      //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETOPENINFO_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *searchname, //IN
#else
  (char           *searchname, //IN
#endif
   short           length_a,    //IN
   _int64         *prevtag,     //IN/OUT
   short          *pri_opener,  //OUT OPTIONAL
                               //[0:9]
   short          *back_opener, //OUT OPTIONAL
                               //[0:9]
   short          *accessmode,  //OUT OPTIONAL
   short          *exclusion,   //OUT OPTIONAL
   short          *syncdepth,   //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *filename,    //OUT OPTIONAL
#else
   char           *filename,    //OUT OPTIONAL
#endif
   short           length_b,    //IN OPTIONAL
   short          *flen,        //OUT OPTIONAL
   short          *accessid,    //OUT OPTIONAL
   short          *validmask)   //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_GETRECEIVEINFO_
  (short   *receiveinfo, //OUT
   short   *reserved)    //OUT OPTIONAL
;

_declspec(dllimport) short   FILE_OPEN_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,              //IN
#else
  (char           *name,              //IN
#endif
   short           length_a,          //IN
   short          *filenum,           //IN/OUT
   short           access,            //IN OPTIONAL
   short           exclusion,         //IN OPTIONAL
   short           nowait,            //IN OPTIONAL
   short           sync_rec_depth,    //IN OPTIONAL
   fat_16          options,           //IN OPTIONAL
   short           seq_block_buf_id,   //IN OPTIONAL
   short           seq_block_buf_len,   //IN OPTIONAL
   short          *primary_phandle)        //IN OPTIONAL
                                     //[0:9]
;

_declspec(dllimport) short   FILE_PURGE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,     //IN
#else
  (char           *name,     //IN
#endif
   short           length_a) //IN
;

_declspec(dllimport) short   FILE_RENAME_
  (short           filenum,  //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,     //IN
#else
   char           *name,     //IN
#endif
   short           length_a) //IN
;

_declspec(dllimport) int GETSYNCINFO
  (short     filenum,  //IN
                      //FILE NUMBER
   short    *syncinfo, //OUT
                      //ARRAY FOR SYNCHRONIZATION INFORMATION
   short    *infosize) //OUT OPTIONAL
                      //SIZE OF SYNCINFO ARRAY
;

_declspec(dllimport) short   GETSYSTEMNAME
  (short     sysid,   //IN
   short    *sysname) //OUT
;

_declspec(dllimport) short   GETTMPNAME (short   *devname) //OUT
                                                         //12-WORD ARRAY TO WHICH TMP NAME IS RETURNED
;

_declspec(dllimport) short   GETTRANSID (short   *transid) //OUT
                                                         //ARRAY TO WHICH TRANSID IS RETURNED
;

_declspec(dllimport) void INTERPRETJULIANDAYNO (int       juliandayno, //IN
                                      short    *year,        //OUT
                                      short    *month,       //OUT
                                      short    *day)         //OUT
;

_declspec(dllimport) int     INTERPRETTIMESTAMP (_int64     juliantimestamp, //IN
                                      short     *date_n_time)     //OUT
;

_declspec(dllimport) short   INTERPRETTRANSID (_int64     transid,  //IN
                                                int       *System,   //OUT
                                                short     *cpu,      //OUT
                                                int       *sequence, //OUT
                                                short     *crashcount) // OUT OPTIONAL
;

_declspec(dllimport) _int64   JULIANTIMESTAMP
  (short     type,  //IN OPTIONAL
   short    *tuid,  //OUT OPTIONAL
   short    *error, //OUT OPTIONAL
   short     node)  //IN OPTIONAL
;

_declspec(dllimport) int KEYPOSITIONX
  (short           filenum, //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *key,     //IN
#else
   char           *key,     //IN
#endif
   short           keytag,  //IN OPTIONAL
   fat_16          keylen,  //IN OPTIONAL
   fat_16          postype) //IN OPTIONAL
;

_declspec(dllimport) int LOCKFILE
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) int LOCKREC
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) short   MESSAGESTATUS
  (short    msgnum) //IN OPTIONAL
;

_declspec(dllimport) short   MESSAGESYSTEMINFO
  (short     itemcode, //IN
   short    *value)    //OUT
;

_declspec(dllimport) void MONITORCPUS (short    mask) //IN
;

_declspec(dllimport) short   MYSYSTEMNUMBER();

_declspec(dllimport) short   NODENAME_TO_NODENUMBER_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *sysname,    //IN OPTIONAL
#else
  (char           *sysname,    //IN OPTIONAL
#endif
   short           sysnamelen, //IN OPTIONAL
   int            *nodenumber) //OUT
;

_declspec(dllimport) short   NODENUMBER_TO_NODENAME_
  (int             sysnum,  //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *sysname, //OUT
#else
   char           *sysname, //OUT
#endif
   short           maxlen,  //IN
   short          *syslen)  //OUT
;

#ifdef TDM_ROSETTA_COMPATIBILITY_
_declspec(dllimport) baddr NUMIN (unsigned char  *str,    //IN
#else
_declspec(dllimport) baddr NUMIN (char           *str,    //IN
#endif
                                                         //STRING WHERE NUMBER STARTS
                                  short          *number, //OUT
                                                         //NUMERIC VALUE RETURNED HERE
                                  short           base,   //IN
                                                         //DEFAULT CONVERSION BASE, A % FORCES 8
                                  short          *result) //OUT
                                                         //1: STR DOES NOT LOOK LIKE A NUMBER
                                                         //0: LEGAL NUMBER
                                                         //-1: ILLEGAL NUMBER
;

#ifdef TDM_ROSETTA_COMPATIBILITY_
_declspec(dllimport) void NUMOUT (unsigned char  *str,    //OUT
#else
_declspec(dllimport) void NUMOUT (char           *str,    //OUT
#endif
                                                         //BYTE STRING FOR ASCII NUMBER
                                  short           number, //IN
                                                         //LOGICAL 16 BIT NUMERIC VALUE
                                  short           base,   //IN
                                                         //CONVERSION BASE, 2-10 ALLOWED
                                  short           width)  //IN
                                                         //CONVERTED NUMBER WILL OCCUPY STR TO
                                                         //STR [ WIDTH - 1 ]
;

_declspec(dllimport) short   OLDFILENAME_TO_FILENAME_
  (short          *oldname,  //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,     //OUT
#else
   char           *name,     //OUT
#endif
   short           length_a, //IN
   short          *name_len) //OUT
;

_declspec(dllimport) int TDM_POSITION_ (short    filenum, //IN
                                                           //FILE NUMBER
                                          int      address) //IN
                                                           //NEW VALUE OF LOGICAL DATA PTR
                                                           //(-1D TO SET TO EOF)
;

_declspec(dllimport) short   PROCESSHANDLE_COMPARE_
  (short   *phandle1, //IN
                     //[0:9]
   short   *phandle2) //IN
                     //[0:9]
;

_declspec(dllimport) short   PROCESSHANDLE_DECOMPOSE_
  (short          *process_handle, //IN
                                  //[0:9]
   short          *cpu,            //OUT OPTIONAL
   short          *pin,            //OUT OPTIONAL
   int            *nodenumber,       //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *nodename,       //OUT OPTIONAL
#else
   char           *nodename,       //OUT OPTIONAL
#endif
   short           length_a,       //IN OPTIONAL
   short          *namelen,        //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *procname,       //OUT OPTIONAL
#else
   char           *procname,       //OUT OPTIONAL
#endif
   short           length_b,       //IN OPTIONAL
   short          *plen,           //OUT OPTIONAL
   _int64         *seqno)          //OUT OPTIONAL
;

_declspec(dllimport) short   PROCESSHANDLE_GETMINE_
  (short   *phandle) //OUT
;

_declspec(dllimport) short   PROCESSHANDLE_NULLIT_
  (short   *prochand) //OUT
                     // PROCESSHANDLE
;

_declspec(dllimport) short   PROCESSHANDLE_TO_FILENAME_
  (short          *process_handle, //IN
                                  //[0:9]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,           //OUT
#else
   char           *name,           //OUT
#endif
   short           length_a,       //IN
   short          *name_len,       //OUT
   short           options)        //IN OPTIONAL
;

_declspec(dllimport) short   PROCESSHANDLE_TO_STRING_
  (short          *phandle,    //IN
                              //[0:9]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *pstring,    //OUT
#else
   char           *pstring,    //OUT
#endif
   short           length_a,   //IN
   short          *pstringlen, //OUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *System,     //IN OPTIONAL
#else
   char           *System,     //IN OPTIONAL
#endif
   short           length_b,   //IN OPTIONAL
   short           namedform)  //IN OPTIONAL
;

_declspec(dllimport) short   PROCESSNAME_CREATE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *name,        //OUT
#else
  (char           *name,        //OUT
#endif
   short           maxnamelen,  //IN
   short          *namelen,     //OUT
   short           name_type,   //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *nodename,    //IN OPTIONAL
#else
   char           *nodename,    //IN OPTIONAL
#endif
   short           nodenamelen, //IN OPTIONAL
   short           options)     //IN OPTIONAL
;

_declspec(dllimport) int     PROCESSORSTATUS();

_declspec(dllimport) short   PROCESSSTRING_SCAN_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *process_string, //IN
#else
  (char           *process_string, //IN
#endif
   short           length_a, //IN
   short          *length_used, //OUT OPTIONAL
   short          *process_handle, //OUT OPTIONAL
                              //[0:9]
   short          *string_kind, //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,           //OUT OPTIONAL
#else
   char           *name,           //OUT OPTIONAL
#endif
   short           length_b,       //IN OPTIONAL
   short          *name_len,       //OUT OPTIONAL
   short          *cpu,            //OUT OPTIONAL
   short          *pin,            //OUT OPTIONAL
   short           options)        //IN OPTIONAL
;

_declspec(dllimport) short   PROCESS_CREATE_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *program_file, //IN OPTIONAL
#else
  (char           *program_file, //IN OPTIONAL
#endif
   short           length_a, //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *library_file, //IN OPTIONAL
#else
   char           *library_file, //IN OPTIONAL
#endif
   short           length_b,          //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *swap_file,         //IN OPTIONAL
#else
   char           *swap_file,         //IN OPTIONAL
#endif
   short           length_c,          //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *ext_swap_file,         //IN OPTIONAL
#else
   char           *ext_swap_file,         //IN OPTIONAL
#endif
   short           length_d,          //IN OPTIONAL
   short           priority,          //IN OPTIONAL
   short           processor,         //IN OPTIONAL
   short          *phandle,           //OUT OPTIONAL
                                     //[0:9]
   short          *error_detail,        //OUT OPTIONAL
   short           name_option,       //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *name,              //IN OPTIONAL
#else
   char           *name,              //IN OPTIONAL
#endif
   short           length_e,          //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *process_descr,         //OUT OPTIONAL
#else
   char           *process_descr,         //OUT OPTIONAL
#endif
   short           length_f,          //IN OPTIONAL
   short          *process_descr_len, //OUT OPTIONAL
   int             nowait_tag,        //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *hometerm,          //IN OPTIONAL
#else
   char           *hometerm,          //IN OPTIONAL
#endif
   short           length_g,          //IN OPTIONAL
   short           memory_pages,      //IN OPTIONAL
   short           jobid,             //IN OPTIONAL
   short           create_options,    //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *defines,           //IN OPTIONAL
#else
   char           *defines,           //IN OPTIONAL
#endif
   short           length_h,          //IN OPTIONAL
   short           debug_options,     //IN OPTIONAL
   int             pfs_size,          //IN OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *reserved1,         //IN OPTIONAL
#else
   char           *reserved1,         //IN OPTIONAL
#endif
                                     //RESERVED FOR INTERNAL USE
   short           reserved2)         //IN OPTIONAL
                                     //RESERVED FOR INTERNAL USE
;

_declspec(dllimport) short   PROCESS_GETINFO_
  (short          *phandle,             //IN/OUT OPTIONAL
                                       //[0:9]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *process_desc,           //OUT OPTIONAL
#else
   char           *process_desc,           //OUT OPTIONAL
#endif
   short           process_desc_maxlen,     //IN OPTIONAL
   short          *process_desc_len,          //OUT OPTIONAL
   short          *priority,            //OUT OPTIONAL
   short          *moms_phandle,          //OUT OPTIONAL
                                       //[0:9]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *hometerm,            //OUT OPTIONAL
#else
   char           *hometerm,            //OUT OPTIONAL
#endif
   short           hometerm_maxlen,     //IN OPTIONAL
   short          *hometerm_len,        //OUT OPTIONAL
   _int64         *process_time,        //OUT OPTIONAL
   short          *creator_access_id,   //OUT OPTIONAL
   short          *process_access_id,   //OUT OPTIONAL
   short          *gmoms_phandle,   //OUT OPTIONAL
                                       //[0:9]
   short          *jobid,               //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *program_file,           //OUT OPTIONAL
#else
   char           *program_file,           //OUT OPTIONAL
#endif
   short           program_file_maxlen,     //IN OPTIONAL
   short          *program_file_len,          //OUT OPTIONAL
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *swap_file,           //OUT OPTIONAL
#else
   char           *swap_file,           //OUT OPTIONAL
#endif
   short           swap_file_maxlen,     //IN OPTIONAL
   short          *swap_file_len,          //OUT OPTIONAL
   short          *error_detail,          //OUT OPTIONAL
   short          *process_type,        //OUT OPTIONAL
   int            *oss_pid)           //OUT OPTIONAL
;

_declspec(dllimport) short   PROCESS_STOP_
  (short          *phandle,          //IN OPTIONAL
                                    //[0:9]
   short           specifier,        //IN OPTIONAL
   short           options,          //IN OPTIONAL
   short           compl_code,       //IN OPTIONAL
   short           termination_info, //IN OPTIONAL
   short          *spi_ssid,         //IN OPTIONAL
                                    //[0:5]
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *ascii_text,        //IN OPTIONAL
#else
   char           *ascii_text,        //IN OPTIONAL
#endif
   short           length_a)         //IN OPTIONAL
;

_declspec(dllimport) int READLOCKX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //OUT
#else
   void           *buf,       //OUT
#endif
   unsigned short  count,     //IN
   unsigned short *countread, //OUT OPTIONAL
   int             tag)       //IN OPTIONAL
;

_declspec(dllimport) int READUPDATELOCKX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //OUT
#else
   void           *buf,       //OUT
#endif
   unsigned short  count,     //IN
   unsigned short *countread, //OUT OPTIONAL
   int             tag)       //IN OPTIONAL
;

_declspec(dllimport) int READUPDATEX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //OUT
#else
   void           *buf,       //OUT
#endif
   unsigned short  count,     //IN
   unsigned short *countread, //OUT OPTIONAL
   int             tag)       //IN OPTIONAL
;

_declspec(dllimport) int READX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //OUT
#else
   void           *buf,       //OUT
#endif
   unsigned short  count,     //IN
   unsigned short *countread, //OUT OPTIONAL
   int             tag)       //IN OPTIONAL
;

_declspec(dllimport) int REPLYX
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *buf,          //IN OPTIONAL
#else
  (void           *buf,          //IN OPTIONAL
#endif
   unsigned short  count,        //IN OPTIONAL
   unsigned short *countwritten, //OUT OPTIONAL
   short           replynum,     //IN OPTIONAL
   short           errret)       //IN OPTIONAL
;

_declspec(dllimport) int REPOSITION (short     filenum,  //IN
                                                               //FILE NUMBER
                                            short    *position) //IN
                                                               //ARRAY FOR POSITION INFORMATION
;

_declspec(dllimport) short   RESUMETRANSACTION (int      tag) //IN
                                                            //TAG RETURNED FROM BEGINTRANSACTION
;

_declspec(dllimport) int SAVEPOSITION
  (short     filenum,  //IN
                      //FILE NUMBER
   short    *position, //OUT
                      //ARRAY FOR POSITION INFORMATION
   short    *infosize) //OUT OPTIONAL
                      //SIZE OF SYNCINFO ARRAY
;

_declspec(dllimport) short   SERVERCLASS_DIALOG_ABORT_
  (int      dialogid) //IN
;

_declspec(dllimport) short   SERVERCLASS_DIALOG_BEGIN_
  (int            *dialogid,           //OUT
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *pmname,             //IN
#else
   char           *pmname,             //IN
#endif
   short           pmnamelen,          //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *scname,             //IN
#else
   char           *scname,             //IN
#endif
   short           scnamelen,          //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *messagebuffer,          //IN/OUT
#else
   void           *messagebuffer,          //IN/OUT
#endif
   unsigned short  requestbytes,       //IN
   short           maxreplybytes,      //IN
   unsigned short *actualreplybytes,         //OUT OPTIONAL
   int             timeout,            //IN OPTIONAL
   fat_16          flags,              //IN OPTIONAL
   short          *scsoperationnumber, //OUT OPTIONAL
   int             tag)        //IN OPTIONAL
;

_declspec(dllimport) short   SERVERCLASS_DIALOG_END_
  (int      dialogid) //IN
;

_declspec(dllimport) short   SERVERCLASS_DIALOG_SEND_
  (int             dialogid, //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *messagebuffer, //IN/OUT
#else
   void           *messagebuffer, //IN/OUT
#endif
   unsigned short  requestbytes,       //IN
   short           maxreplybytes,      //IN
   unsigned short *actualreplybytes,         //OUT OPTIONAL
   int             timeout,            //IN OPTIONAL
   fat_16          flags,              //IN OPTIONAL
   short          *scsoperationnumber, //OUT OPTIONAL
   int             tag)        //IN OPTIONAL
;

_declspec(dllimport) short   SERVERCLASS_SEND_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *pathmon,            //IN
#else
  (char           *pathmon,            //IN
#endif
   short           pathmonbytes,       //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *serverclass,          //IN
#else
   char           *serverclass,          //IN
#endif
   short           serverclassbytes,    //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *messagebuffer,          //IN/OUT
#else
   void           *messagebuffer,          //IN/OUT
#endif
   unsigned short  requestbytes,       //IN
   unsigned short  maximumreplybytes,    //IN
   unsigned short *actualreplybytes,         //OUT OPTIONAL
   int             timeout,            //IN OPTIONAL
   fat_16          flags,              //IN OPTIONAL
   short          *scsoperationnumber, //OUT OPTIONAL
   int             tag)        //IN OPTIONAL
;

_declspec(dllimport) short   SERVERCLASS_SEND_INFO_
  (short   *serverclasserror, //OUT
   short   *filesystemerror) //OUT
;

_declspec(dllimport) int SETMODE
  (short     filenum, //IN
                     //FILE NUMBER
   short     modenum, //IN
                     //NUMBER OF CHARACTERISTIC TO BE CHANGED
   fat_16    parm1,   //IN OPTIONAL
                     //FIRST PARAMETER
   fat_16    parm2,   //IN OPTIONAL
                     //SECOND PARAMETER
   short    *oldval)  //OUT OPTIONAL
                     //ARRAY FOR RETURNING OLD VALUE
;

_declspec(dllimport) int SETMODENOWAIT
  (short     filenum, //IN
                     //FILE NUMBER
   short     modenum, //IN
                     //NUMBER OF CHARACTERISTIC TO BE CHANGED
   fat_16    parm1,   //IN OPTIONAL
                     //FIRST PARAMETER
   fat_16    parm2,   //IN OPTIONAL
                     //SECOND PARAMETER
   short    *oldval,  //OUT OPTIONAL
                     //ARRAY FOR RETURNING OLD VALUE
   int       tag)     //IN OPTIONAL
                     //SPECIFIED TAG FOR NOWAIT REQUESTS
;

_declspec(dllimport) int SETPARAM
  (short     filenum,      //IN
   short     function,     //IN
   short    *param,        //IN OPTIONAL
   short     paramsize,    //IN
   short    *oldparam,     //OUT OPTIONAL
   short    *oldparamsize, //OUT OPTIONAL
   short     oldparammax,  //IN OPTIONAL
   int       tag)          //IN OPTIONAL
;

_declspec(dllimport) short   SETSTOP (short    stopmode) //IN
                                                                 //NEW STOP MODE
;

#ifdef TDM_ROSETTA_COMPATIBILITY_
_declspec(dllimport) void SHIFTSTRING (unsigned char  *bytestring, //IN/OUT
#else
_declspec(dllimport) void SHIFTSTRING (char           *bytestring, //IN/OUT
#endif
                                                                  //String to be shifted
                                       short           bytecount,  //IN
                                                                  //Number of BYTES in String
                                       short           casebit)    //IN
                                                                  //.<15> = 0 -> UPSHIFT,
;

_declspec(dllimport) int SIGNALTIMEOUT
  (int       toval,   //IN
   short     parm1,   //IN OPTIONAL
   int       parm2,   //IN OPTIONAL
   short    *tleaddr) //OUT OPTIONAL
;

_declspec(dllimport) short   STATUSTRANSACTION
  (short     *status,  //OUT
                      //status of transaction (if FEOK returned)
   _int64     transid) //IN OPTIONAL
                      //status requested for this transid (optional)
;

_declspec(dllimport) short   STRING_UPSHIFT_
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *in_string,  //IN
#else
  (char           *in_string,  //IN
#endif
   short           length_a,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *out_string, //OUT
#else
   char           *out_string, //OUT
#endif
   short           length_b)  //IN
;

_declspec(dllimport) short   TEXTTOTRANSID
#ifdef TDM_ROSETTA_COMPATIBILITY_
  (unsigned char  *text,        //IN
#else
  (char           *text,        //IN
#endif
   short           textbytelen, //IN
   _int64         *transid)     //OUT
;

_declspec(dllimport) void TMF_VERSION_ (int       sysnum,  //IN
                                                          // System Number of the system to interrogate
                                        short    *version, //OUT
                                                          // Version number of the TMP process
                                        short    *error)   //OUT
                                                          // File System Error Code
;

_declspec(dllimport) short   TOSVERSION();

_declspec(dllimport) short   TRANSIDTOTEXT (_int64     transid,     //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
                                             unsigned char 
#else
                                             char          
#endif
                                                      *text,        //OUT
                                             short      textbytelen, //IN
                                             short     *bytesused)   //OUT
;

_declspec(dllimport) int UNLOCKFILE
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) int UNLOCKREC
  (short    filenum, //IN
                    //FILE NUMBER
   int      tag)     //IN OPTIONAL
                    //NO-WAIT REQUEST TAG
;

_declspec(dllimport) int WRITEREADX
  (short           filenum,    //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,        //IN/OUT
#else
   void           *buf,        //IN/OUT
#endif
   unsigned short  writecount, //IN
   unsigned short  readcount,  //IN
   unsigned short *countread,  //OUT OPTIONAL
   int             tag)        //IN OPTIONAL
;

_declspec(dllimport) int WRITEUPDATEUNLOCKX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //IN
#else
   void           *buf,       //IN
#endif
   unsigned short  count,     //IN
   unsigned short *countread, //OUT OPTIONAL
   int             tag)       //IN OPTIONAL
;

_declspec(dllimport) int WRITEUPDATEX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //IN
#else
   void           *buf,       //IN
#endif
   unsigned short  count,     //IN
   unsigned short *countread, //OUT OPTIONAL
   int             tag)       //IN OPTIONAL
;

_declspec(dllimport) int WRITEX
  (short           filenum,   //IN
#ifdef TDM_ROSETTA_COMPATIBILITY_
   unsigned char  *buf,       //IN
#else
   void           *buf,       //IN
#endif
   unsigned short  count,     //IN
   unsigned short *countread, //OUT OPTIONAL
   int             tag)       //IN OPTIONAL
;


_declspec(dllimport) short TDM_NTAWAITIOX_( 
 short 	*filenum ,	// IN/OUT
 void 	**bufptr ,	// OUT OPTIONAL
 unsigned short *xfercount, 	// OUT OPTIONAL
 int   *tag	 ,	// OUT OPTIONAL
 int	timeout	 ,	// IN OPTIONAL
 int	eventCount,	//IN OPTIONAL
 void** eventList,	//IN OPTIONAL
 int	*eventIndex,	//OUT OPTIONAL
 int	dwWaitMask,	//IN OPTIONAL
 int	alertable,	//IN OPTIONAL
 int	*NtError)	//OUT OPTIONAL
;
_declspec(dllimport) short SETTMFAFFINITY
  (const short TmfSegmentNumber //IN
  )
;
#endif /* ! _cplusplus */

#endif /* _INC_TDM_CEXTDECS_H */

