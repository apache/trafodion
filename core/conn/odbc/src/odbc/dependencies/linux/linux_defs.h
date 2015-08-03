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
#ifndef LINUX_H_
#define LINUX_H_

// return code when monitor shutdown
#define SYSTEM_SHUTDOWN			1000


#define SEVERITY_CRITICAL           0
#define SEVERITY_MAJOR              1
#define SEVERITY_MINOR              2
#define SEVERITY_WARNING            3
#define SEVERITY_INFORMATIVE        4
#define SEVERITY_CLEARED            4
#define SEVERITY_INDETERMINATE      5

#define CAPTURE_SELECTIVE      0x0001
#define CAPTURE_STACK_TRACE    0x0002
#define CAPTURE_ALL            0x0004
#define PROCESS_CONTINUE       0x0008

#define PROCESS_STOP           0x0010
#define PROCESS_ABEND          0x0020
#define CPU_HALT               0x0040
#define VD_BYPASS              0x0080


// For the Linux platform do not define EMS_SUPPORTED (this is a
// reminder that this define must exist for the NSK platform when
// "platform.h" is set up for NSK
//      #define EMS_SUPPORTED

// originally defined in "zsysc.h"
#define ZSYS_VAL_LEN_FILENAME 47

// originally defined in "winreg.h"
#define HKEY	int

// originally defined in "cextdecs.h"
#ifndef ___cc_status_DEFINED
#define ___cc_status_DEFINED
typedef int _cc_status;
#define _status_lt(x) ((x) < 0)
#define _status_gt(x) ((x) > 0)
#define _status_eq(x) ((x) == 0)
#define _status_le(x) ((x) <= 0)
#define _status_ge(x) ((x) >= 0)
#define _status_ne(x) ((x) != 0)
#endif

// Handle produced by dlopen (NOT a pointer)
typedef void *dlHandle; /* handle from dlopen (NOT a pointer) */

#define _far

typedef long long int64;

// [note Intel compiler has built-in definition for __int64]
typedef long long __int64;

// Map NSK function to equivalent linux function
#define fopen_guardian fopen

// Macro used to make closing a socket platform independent.
// (Guardian sockets require FILE_CLOSE_)
#define CLOSE_SOCKET close

// wrapper names
#include <stddef.h>
#include <seabed/pctl.h>
#include <seabed/timer.h>
#define FILE_OPEN_               XFILE_OPEN_
#define AWAITIOX                 XAWAITIOX
#define AWAITIOXTS               XAWAITIOXTS
#define FILE_CLOSE_              XFILE_CLOSE_
#define FILE_GETINFO_            XFILE_GETINFO_
#define FILE_GETRECEIVEINFO_     XFILE_GETRECEIVEINFO_
#define PROCESSHANDLE_COMPARE_   XPROCESSHANDLE_COMPARE_
#define PROCESSHANDLE_NULLIT_    XPROCESSHANDLE_NULLIT_
#define PROCESSHANDLE_GETMINE_   XPROCESSHANDLE_GETMINE_
#define PROCESSHANDLE_DECOMPOSE_ XPROCESSHANDLE_DECOMPOSE_
#define READUPDATEX              XREADUPDATEX
#define REPLYX                   XREPLYX
#define SETMODE                  XSETMODE
#define WRITEX                   XWRITEX
#define WRITEREADX               XWRITEREADX
#define PROCESSOR_GETINFOLIST_   XPROCESSOR_GETINFOLIST_
#define SIGNALTIMEOUT			 XSIGNALTIMEOUT
#define CANCELTIMEOUT			 XCANCELTIMEOUT
#define FILENAME_TO_PROCESSHANDLE_   XFILENAME_TO_PROCESSHANDLE_

#define FEOK 0
#define FILE_OPEN_WAIT_OPTION    0x4000

typedef unsigned short awaitiox_count_t;
// WMS uses Tag_Type, while NDCS uses tag_t, for now have an extra typedef
// Need to resolve
typedef SB_Tag_Type tag_t;
typedef SB_Tag_Type Tag_Type;


#define ZSYS_VAL_SMSG_CPUDOWN    XZSYS_VAL_SMSG_CPUDOWN
#define ZSYS_VAL_SMSG_CPUUP      XZSYS_VAL_SMSG_CPUUP
#define ZSYS_VAL_SMSG_PROCDEATH  XZSYS_VAL_SMSG_PROCDEATH
#define ZSYS_VAL_SMSG_OPEN       XZSYS_VAL_SMSG_OPEN
#define ZSYS_VAL_SMSG_CLOSE      XZSYS_VAL_SMSG_CLOSE
#define ZSYS_VAL_SMSG_TIMESIGNAL XZSYS_VAL_SMSG_TIMESIGNAL
#define ZSYS_VAL_SMSG_SHUTDOWN   XZSYS_VAL_SMSG_SHUTDOWN

#define zsys_ddl_smsg_cpudown_def   xzsys_ddl_smsg_cpudown_def
#define zsys_ddl_smsg_cpuup_def     xzsys_ddl_smsg_cpuup_def
#define zsys_ddl_smsg_procdeath_def xzsys_ddl_smsg_procdeath_def
#define zsys_ddl_smsg_open_def		xzsys_ddl_smsg_open_def

//New phandle changes
#ifdef USE_NEW_PHANDLE
#define TCPU_DECL(name)      int name
#define TPIN_DECL(name)      int name
#define TPT_DECL(name)       SB_Phandle_Type name
#define TPT_PTR(name)        SB_Phandle_Type *name
#define TPT_REF(name)        (&name)
#define TPT_RECEIVE			 FS_Receiveinfo_Type*
#else
#define TCPU_DECL(name)      short name
#define TPIN_DECL(name)      short name
#define TPT_DECL(name)       short name[10]
#define TPT_PTR(name)        short *name
#define TPT_REF(name)        (name)
#define TPT_RECEIVE	 		 short*
#endif


// function prototypes for time functions implemented in nsktime.cpp
// (formerly used cextdecs.h function prototypes)
extern "C" {
   void TIME( short * a );
   long long JULIANTIMESTAMP (short   type  = 0,
                           short * tuid  = 0,
                           short * error = 0,
                           short   node  = -1
          );

   long INTERPRETTIMESTAMP (long long juliantimestamp,
							short * date_n_time);

   long long CONVERTTIMESTAMP (long long  timestamp,
	                           short   direction,
	                           short   node,
	                           short * error
	   );

   long INTERPRETINTERVAL (
	   long long time,
	   short  *hours,
	   short  *minutes,
	   short  *seconds,
	   short  *milsecs,
	   short  *microsecs
	   );

   int64 COMPUTETIMESTAMP (
	   const short * date_n_time,
	   short * error = 0
	   );


   void DELAY (int      time);     //libtdm_nsklib.so

   short PROCESS_GETINFO_ (  short *processhandle,  /*i,o 1 */
      char *proc_fname   /* o 2 */
      ,short maxlen1   /* o 2 */
      ,short *proc_fname_len   /* o 3 */
      ,short *priority   /* i 4 */
      ,short *moms_processhandle   /*i 5 */
      ,char *hometerm   /* i 6 */
      ,short maxlen2   /* i 6 */
      ,short *hometerm_len   /* i 7 */
      ,long long *process_time  /* i 8 */
      ,short *creator_access_id  /*i 9 */
      ,short *process_access_id  /*i 10 */
      ,short *gmoms_processhandle  /* i11 */
      ,short *jobid   /* i 12 */
      ,char *program_file   /* i 13 */
      ,short maxlen3   /* i 13 */
      ,short *program_len   /* i 14 */
      ,char *swap_file   /* i 15 */
      ,short maxlen4   /* i 15 */
      ,short *swap_len   /* i 16 */
      ,short *error_detail   /* i 17 */
      ,short *proc_type   /* i 18 */
      ,__int32_t *oss_pid   ); /* i 19*/;

  }


// Folllowing in process.h in ndcs NSK build
#define _getpid getpid

// SrvrConnect.cpp uses this function but it does not seem to be
// available on Linux
#define dlresultcode() -1


// "OMIT" macros used to fill in "default" arguments
#ifndef OMITREF
#define OMITREF     NULL
#endif

#ifndef OMITSHORT
#define OMITSHORT   0
#endif

#define PRINTSRVRTRC
#define INITSRVRTRC

// NDCS code contains a number of uses of these macros.  This was for
// a now obsolete facility that was used to measure the amount of time
// spent in various functions.  This facility is disabled
// by having the macros expand to nothing.
#define SRVRTRACE_ENTER(name)
#define SRVRTRACE_EXIT(name)

#endif
