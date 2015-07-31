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
#ifndef NSK_H_
#define NSK_H_


typedef long tag_t;

#define TCPU_DECL(name)      short name
#define TPIN_DECL(name)      short name
#define TPT_DECL(name)       short name[10]
#define TPT_PTR(name)        short *name
#define TPT_REF(name)        (name)
#define TPT_RECEIVE	 		 short*

/*-----------------------------------------------------------------------
// For the Linux platform do not define TFDS_SUPPORTED (this is a
// reminder that this define must exist for the NSK platform when
// "platform.h" is set up for NSK
//      #define TFDS_SUPPORTED

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
 typedef void *dlHandle; //* handle from dlopen (NOT a pointer) 
 
 #define _min  min
 #define _far
 
 // not sure where this was defined for the build
 typedef long long int64;
 
 // [note Intel compiler has built-in definition for __int64]
 #if !defined(__INTEL_COMPILER)
 typedef long long __int64;
 #endif
 
 // Map NSK function to equivalent linux function
 #define fopen_guardian fopen
 
 // Macro used to make closing a socket platform independent.  (NSK uses
 // Guardian sockets which requires FILE_CLOSE_)
 #define CLOSE_SOCKET close
 
 // Mapping for Seabed Guardian wrapper names
 #include <stddef.h>
 #include <seabed/pctl.h>
 #define FILE_OPEN_               XFILE_OPEN_
 #define AWAITIOX                 XAWAITIOX
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
 
 #define FEOK 0
 #define FILE_OPEN_WAIT_OPTION    0x4000
 
 typedef unsigned short awaitiox_count_t;
 
 #define ZSYS_VAL_SMSG_CPUDOWN    XZSYS_VAL_SMSG_CPUDOWN  
 #define ZSYS_VAL_SMSG_CPUUP      XZSYS_VAL_SMSG_CPUUP
 #define ZSYS_VAL_SMSG_PROCDEATH  XZSYS_VAL_SMSG_PROCDEATH
 #define ZSYS_VAL_SMSG_OPEN       XZSYS_VAL_SMSG_OPEN
 #define ZSYS_VAL_SMSG_CLOSE      XZSYS_VAL_SMSG_CLOSE
 
 #define zsys_ddl_smsg_procdeath_def xzsys_ddl_smsg_procdeath_def
 
 // function prototypes for time functions implemented in nsktime.cpp
 // (formerly used cextdecs.h function prototypes)
 extern "C" {
     void TIME( short * a );
     long long JULIANTIMESTAMP (short   type  = 0,
                             short * tuid  = 0,
                             short * error = 0,
                             short   node  = -1
         );
 }
 
 // Folllowing in process.h in ndcs NSK build
 #define _getpid getpid
 
 // SrvrConnect.cpp uses this function but it does not seem to be
 // available on Linux
 #define dlresultcode() -1
 
 
 // "OMIT" macros used to fill in "default" arguments in Guardian calls
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
 // spent in various functions.  For Seaquest this facility is disabled
 // by having the macros expand to nothing.
 #define SRVRTRACE_ENTER(name)
 #define SRVRTRACE_EXIT(name)
-----------------------------------------------------------------------*/

#endif
