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
#if _MSC_VER >= 1100
#pragma once
#endif
/*++

Module name:

NSKcommon.h

Abstract:

   This module defines the types and constants that are defined by the
     Tandem NSK services suite.

Revision History:

--*/

#ifndef _NSK_common_
#define _NSK_common_

#undef DllImport
#define DllImport __declspec( dllimport )
#define DllExport __declspec( dllexport )

#define _NSK_TRACE_

#include "seaquest/sqtypes.h"

#include "nsk/nskmsglo.h"
#include "nsk/nskmem.h"
#include "nsk/nskport.h"
#include "nsk/nsktime.h"

#define  dsecure_h_including_section
#define  dsecure_h_size_literals
#include "security/dsecure.h"
#undef dsecure_h_including_section

#include "nsk/nskltimsg.h"

                     // save original alignment in source
#pragma pack ( push, enter_NSK_common)

                     // new alignment for declarations 4-byte
#pragma pack ( 4 )

// maximum pes in cluster

#define MAX_PE_COUNT 16

// the port name for FS Helper Process

#define FSHELP_PORT_NAME    "$ZFS"

// prefix for port name for router process.
#define ROUTERPROCESS_PORT_NAME "$ZL"
//  Following are data structure type definitions to be used for the lock log.

#define NSK_MTG_TYPE    11
#define NSK_CACHE_CTL_TYPE 12
#define NSK_SG_TYPE    13
#define NSK_BSD_TYPE    14
#define NSK_MQC_TYPE    21
#define NSK_PCB_TYPE    22
#define NSK_QCB_TYPE    23
#define    NSK_TLE_TYPE    24
#define NSK_CACHE_TYPE    25
#define NSK_NRL_TYPE    26
#define NSK_PHOENIX_TYPE    27
#define NSK_TCB_TYPE    28
#define NSK_MQC_HEADER_TYPE    121
#define    NSK_PCB_HEADER_TYPE    122
#define NSK_QCB_HEADER_TYPE    123
#define NSK_TLE_HEADER_TYPE    124
#define NSK_CACHE_HEADER_TYPE    125
#define NSK_NRL_HEADER_TYPE    126
#define NSK_PHOENIX_HEADER_TYPE 127
#define    NSK_TCB_HEADER_TYPE    128
#define NSK_NRL_HASHHEAD_TYPE  129 //added by SH
#define NSK_PTABLE_HEADER_TYPE  130 //added by SH
#define NSK_NRL_TABLE_HEADER_TYPE 131 //added by SH   

// forward references

typedef class _NSK_PCB *PNSK_PCB;
typedef class _NSK_QCB *PNSK_QCB;
typedef class _NSK_MQC *PNSK_MQC;
typedef class _NSK_CACHE *PNSK_CACHE;
typedef class _NSK_TCB  *PNSK_TCB;


// ------------------------------------
//
// NSK message queue Control Block (QCB)
//
// ------------------------------------

#define  CACHED_EVENT_HANDLES 4
#define  NUM_EVENT_HANDLES 4
#define  CACHED_PROCESS_HANDLES 4

#define    MSG_LIMITS    255

// masks for the 'flags' field of the NSK_QCB structure

#define msgabandon    0x10
#define msgstatus     0x8
#define msgldone      0x4
#define msgtpop       0x2
#define msgireq       0x1

// following defines are for priority queueing.
#define MSG_INVALID_PRI 0
#define NUM_MSG_PRI	  0x8
#define QUEUING_MODE_FIFO 0
#define QUEUING_MODE_PRI  1
#define QUEUING_MODE_NOCHANGE 10
#define SYSTEM_DEFAULT_MSG_PRI 3
#define SYSTEM_DEFAULT_MSNGR_PRI 3





// -------------------------
//
// NSK process control block
//
// -------------------------


#define  CACHED_PROCESS_HANDLES 4

#define    ILDONE    0
#define    ILREQ        1
#define    ILCAN        2

// macros for File System requested fields in PCB
// these fields are set and munipulate by File System code

// **** NOTE ****
// If any fields are added to this define, please be sure that
// the fields are appropriately copied and handled in the routines
// transmute_fs_pcb and detransmute_* used in FS cleanup.
// **** NOTE ****


// FS requires the var fsPCB to point to a 64byte(16 DWORD)area in the PCB
#define FS_BUFFER_SIZE 16



// --------------------------------------------------------
//
// BSD - bus state descriptor (send / receive control)
//
// --------------------------------------------------------

#define MAX_IP_NAME_LENGTH 64
#define NSK_SOCKADDR 1649
#define NSK_SEND_SOCKADDR    22344

// The NSK-Lite Service will only look at the first
// MAX_IF_NAMES_SUPPORTED IP addresses found by gethostbyname( ).
// The rest will be ignored.
#define MAX_IP_NAMES_SUPPORTED  20

// Lti i/o data structure with two scatter gather sections to make it
//  work like sockets
//

enum operationType_t {
      operReceive       = 0
    , operSend          = 1
    , operDupHandle     = 2
    , operQueuedSend    = 3
    , operConnect       = 4
    , operCancelReceive = 5
    , operCancelSend    = 6
    , operVoid          = -1
};


// The following define the max supported (but not necessarily used sends and
//   receives.  The actual size is set in NSKmain.cpp

#define NSK_BSD_RECVMSG_MAX 8
#define NSK_BSD_SENDMSG_MAX 8
#define NSK_BSD_FREESEND_MAX 256 // must be 2^NSK_BSD_SENDMSG_MAX





// --------------------------------------------
//
// SAC - msgsys control area + glup state area
// -------------------------------------------

typedef struct _NSK_SAC{
      SHORT dummy;  // dummy for now
} NSK_SAC, *PNSK_SAC;

/********************
*****  THIS SECTION HAS THE DECLARATIONS AND DEFINES FOR SYSTEM STATUS
*****  MESSAGES******/

// Maybe we ought to get the rosetta-ed status.h files from NSK but
// meanwhile.....


#define  MBID_CPUDOWN         0
#define  MBID_CPUUP           1


// -----------------------------------------------------------
// SG equivalent
//
// This data structure is equivalent to the low SG area
// on an NSK cpu.  It will reside in the shared memory
// area accessible to all processes that utilize NSK services
//
// -----------------------------------------------------------

// use the following for table allocation.  Later we should make these
// registry entries or something...

                                        // address of common area
#define NSKCOMMONBASE 0x1D000000
                                         // size of common area
#define NSKCOMMONBYTES   0x2400000

                                    // is buffer in common area?  Note that we don't
                                    // bother to test the end of the buffer as it would
                                    // be a programming error if it was partially in.

#define NSKINCOMMON( a ) (  ( (DWORD) a - NSKCOMMONBASE ) < NSKCOMMONBYTES )

#define MAX_REGKEY_PATH_LENGTH    100
#define MAX_IMAGEPATH_LENGTH    100
#define MAX_TANDEMID_LENGTH        50
#define MAX_BOOT_CONFIG_LENGTH    100


#define MQC_ENTRIES_INITIAL  0X005
#define MQC_ENTRIES_MAX      0X800

#define DEFAULT_CACHE_ENTRIES 0x80
#define DEFAULT_CACHE_SIZE    0x1400

#define NRL_ENTRIES_INITIAL  0X10
#define NRL_ENTRIES_MAX      0XB00

#define QCB_ENTRIES_INITIAL  0X040
#define QCB_ENTRIES_MAX      0XB00

#define NSK_INVALID_INDEX    0XFFFFFFFF

// the maximum number of threads that can be allocated to
// clean up after dead processes.

#define PCB_SHADOWS          0X10

// the number of handles each thread can wait on

#define SHADOW_HANDLES       0X10  // MAXIMUM_WAIT_OBJECTS - set low for debugging

#define PCB_ENTRIES_INITIAL  0X10
#define PCB_ENTRIES_MAX      ( PCB_SHADOWS * ( SHADOW_HANDLES - 1 ) )

#define PHOENIX_ENTRIES_INITIAL 0x4
#define PHOENIX_ENTRIES_MAX     0x40  // Allocate extra for fscleanup processes

#define THREAD_ENTRIES_INITIAL  0x30
#define THREAD_ENTRIES_MAX    (( PCB_SHADOWS * ( SHADOW_HANDLES - 1) ) * 3 )

#define MBE_ENTRIES_INITIAL   0x10
#define MBE_ENTRIES_MAX    0x10

#define MB_ENTRIES           0x4
#define TB_ENTRIES           0x80

#define NSK_ACL_LENGTH  1024
#define NSK_SID_LENGTH  1024

#define  mbid_cpudown   0
#define  mbid_cpuup     1

#define LOCK_LOG_SIZE    256    // It should be a power of 2 so that the wraparound is correct
#define NSKLockGet(plock) RealNSKLockGet(plock,__LINE__,__NSK_FILE_FOR_GET__) //added by SH
#define NSKLockPut(plock) RealNSKLockPut(plock,__LINE__,__NSK_FILE_FOR_PUT__) //added by SH
#define NSKTablePut(ptable,pentry) RealNSKTablePut(ptable, pentry, __LINE__, __NSK_FILE_FOR_PUT__) //added by SH
#define NSKTableGet(ptable) RealNSKTableGet(ptable, __LINE__, __NSK_FILE_FOR_GET__) //added by SH

#define GlupLockGet() RealNSKLockGet(&pnsk->gluppy.spin, __LINE__,__NSK_FILE_FOR_GET__)
#define GlupLockPut() RealNSKLockPut(&pnsk->gluppy.spin, __LINE__,__NSK_FILE_FOR_PUT__) // added for glup lock synchronization

#define NSKForeignPCBLockGet(pcb,success) ForeignPCBLockGet(pcb,__LINE__,__NSK_FILE_FOR_GET__,success)

typedef struct mbe_general_struct {
         USHORT mbetype;
         USHORT cpunum;
         USHORT cpumask;
         USHORT cpucount;
} mbe_general_template, mbe_cpustat_template, mbe;

typedef struct trace_buf_struct {
         char  eyecatch[24];
         char  fname[24];
         char  line[6];
         DWORD info;
         DWORD moreinfo;
         DWORD reserved1;
         DWORD reserved2;
} tbe;


// Global variables used only the the Messenger Thread

typedef class _NSK_MTG  {  // Messenger Thread Globals

public:

    // This spin lock will synchronize accesses to the below
    // linked lists. The following three fields must be consecutive
    // and in the same order.
    DWORD spin;
    DWORD type;
    DWORD filler; // corresponds to index (DON't USE)
    DWORD       pid;     // added by SH - do not change order of these four fields          
    DWORD       tid;     //   "     
    DWORD       line;    //   "
    char        file[4]; //   "
    // The following three lists are queues of MRQs.  The order of the
    // items on these chains is not significant.

    LIST_ENTRY MRQinit;     // MRQs the Messenger needs to initiate (send)
    LIST_ENTRY MRQtime;  // MRQs the Mgr. has initiated with a time limit
    LIST_ENTRY MRQperm;     // MRQs the Mgr. has initiated without a time limit

    // A port for receiving messenger requests, and its index
    NSK_PORT_HANDLE  MessengerPort;
    DWORD            MessengerPortIdx;

} NSK_MTG, *PNSK_MTG;


// Readlink cache globals
typedef class _NSK_CACHE_CTL {
public:

         DWORD       spin;           // For syncronization in cache
         DWORD        type;            // For putting the type of data structure in the lock log
         DWORD        filler;            // corresponds to index (DON't USE)
         DWORD       pid;     // added by SH - do not change order of these four fields          
         DWORD       tid;     //   "     
         DWORD       line;    //   "
         char        file[4]; //   "
         LIST_ENTRY pre_datalist;  // Cache entry here if MSG_LISTEN_ called
         LIST_ENTRY post_datalist; // Here if MSG_READDATA_ called
         PNSK_CACHE pcache;        // List of free cacge entries
         LONG       cache_size;    // Set at service startup
         INT        cache_entries; // Set at service startup

} NSK_CACHE_CTL, *PNSK_CACHE_CTL;

typedef struct _NSK_SYSTEM_THREAD {
    char    name[44];   // The name of procedure that runs the thread
    DWORD   threadId;
} NSK_SYSTEM_THREAD, *PNSK_SYSTEM_THREAD;
typedef struct _NSK_LOCKLOG {

    char    getput;                // `G' or `P' depending on LockGet or LockPut
    DWORD    type;                // type of the data structure
    DWORD    index;                // index of the data structure in the linked list
    DWORD    pid;                // process ID of the process calling LockGet/LockPut
    DWORD    tid;                // thread ID of the thread calling the routines
    DWORD    line;                // line number where the call is made
    char    file[4];            // full path name of the file in which the call is made
}    NSK_LOCKLOG, *PNSK_LOCKLOG;


 // Definition for the Global buffers allocated by
 //  NSK_GLOBALBUFFER_ALLOCATE_
 //
struct NskGlobalBuffer
{
    NskGlobalBuffer *next;
    NskGlobalBuffer *prev;    // unused for now
};



//=========================================================================
// procedure declarations
//=========================================================================

#ifdef NSKCommon_C

DllExport
BOOL NSKConsoleHandler( DWORD type );

DllExport
PVOID NSKMapCommon ( BOOL createmap, DWORD * size, HANDLE *common_memory_handle );

#else

// The following three routines are for the private use of NSK
// in fscleanup handling...



// The following two routines are for thread handling inside NSK

DllImport
PNSK_TCB NSKGetTCBPtr(void);

DllImport
void NSKSetTCBPtr( PNSK_TCB pthread );

DllImport
void NSKNodeHalt( DWORD cause );

DllImport
void NSKSystemFreeze( DWORD cause );

DllImport
void NSKExitService( DWORD status_code );

DllImport
void NSKExitServiceNoBlueScreen( DWORD status_code );

DllImport
void NSKTerminateService( DWORD status_code );

DllImport
void NSKTerminateServiceNoBlueScreen( DWORD status_code );

DllImport
void NSKStartingExceptionFilter( );

DllImport
DWORD NSKWaitForSingleObject (PNSK_PCB lppcb, HANDLE hEvent, DWORD timeout);

#endif // NSKCommon_C


#ifndef NSKmsghi_C

// For QA use only....
DllExport void
MSG_GETREQINFO_PRIVATE_SHELL_( unsigned_16   itemcode,
                               int_32        oldid,
                               void         *item );

#endif

#ifndef NSKcpu_C

// This routine behaves identically to PROCESSORSTATUS (declared in cextdecs
// and pcpuctlz.h) except that it will also reflect the latest NSK-Lite status 
// of a reloadee node coming up.  A reloadee is considered up from the point of 
// view of this routine just prior to starting the first process that runs on 
// top of the tdm_service on the reloadee; currently this first process is 
// tdm_tmfinit.exe.
//
// This routine is not meant for general use.  It is most likely only useful to
// those subsystems invoked while a node is in the process of being integrated 
// into the cluster.  Currently, TMF is the only subsystem involved.  If you 
// think you need this routine, please talk to the NSK-Lite group, because you 
// probably don't.

extern "C"
DllImport
int_32 PROCESSORSTATUS_INCLUDING_RELOADEE();

#endif

// restore original alignment in source
#pragma pack ( pop, enter_NSK_common)


DllImport void NSKQCBCacheInit(PNSK_QCB pqcb, BOOL closeFlag);

#endif //_NSK_common_
