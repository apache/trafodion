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
#ifndef _NSKmem_H
#define _NSKmem_H
#include "seaquest/sqtypes.h"
#define DllImport __declspec( dllimport )
#define DllExport __declspec( dllexport )




#ifdef NA_64BIT
// dg64 - these are equivalent, but this one's easier to see
#pragma pack(push, enter_NSKmem, 4)
#else
#pragma pack(push, enter_NSKmem)
#pragma pack(4)
#endif

// forward reference
typedef class  _NSK_PCB *PNSK_PCB;

// table header structure.  This struct is negatively indexed
// from a table pointer.

typedef struct _TABLE_HEADER {

   DWORD    eyecatcher;
   LIST_ENTRY  freelist;
   DWORD     spin;
   DWORD    type;
   DWORD    index;
   DWORD    pid;            // added by SH - do not change order of these four fields           
   DWORD    tid;            //   "     
   DWORD    line;           //   "
   char     file[4];        //   "
   DWORD    headertype;
   DWORD    freecount;
   DWORD    fails;
   DWORD    entrysize;
   DWORD    curentries;
   DWORD    maxentries;

  } TABLE_HEADER, *PTABLE_HEADER;

// to use the standard table routines, each table entry
// must have the following fields defined .
// Please note that the inline function PCBPIN( ) requires
// the following aspects of the TABLE_ENTRY to remain constant,
// or we will break the FS...
//    1) The offsets of the LIST_ENTRY and index cannot change.
//    2) The format of LIST_ENTRY cannot change.
//    3) The sizeof( TABLE_ENTRY ) cannot change.

typedef struct _TABLE_ENTRY {

   LIST_ENTRY     list;
   DWORD           spin;        // spin and type have to be consecutive and type
   DWORD          type;        // should follow spin
   DWORD    index;
   DWORD    pid;            // added by SH - do not change order of these four fields           
   DWORD    tid;            //   "     
   DWORD    line;           //   "
   char     file[4];        //   "
   long           unused[ 3 ];  // For future use...
} TABLE_ENTRY, *PTABLE_ENTRY;

// the base class for all NSK control blocks or control structures
// including but not limited to PCB, QCB and MQCs.  This must
// agree with the above struct.

typedef class _NSK_CB{
public:
   LIST_ENTRY  list;
   DWORD        spin;            // same comments as above...
   DWORD       type;
   DWORD    index;
   union {
       DWORD    pid;        // added by SH - do not change order of these four fields           
       DWORD    num_lockget; // number of times we called NSKLockGet
   };
   union {
       DWORD    tid;        //   "     
       DWORD    spin_in_first_try; // count of times we get the spin lock on the first try
   };
   union {
       DWORD    line;       //   "
       DWORD    num_spin_tries;  // number of times we tried to get the spin lock
   };
   union {
       char     file[4];    //   "
       DWORD    max_spins;  // high water mark for number of tries of the spin lock before success
   };
   long        unused[ 3 ];
} NSK_CB, *PNSK_CB;


//NRL - named resource table header - for msg ports

#define NRL_HASH_HEADS 32
#define NRL_HASH_HEADS_MASK ( NRL_HASH_HEADS - 1 )


typedef struct _NSK_NRL_TABLE_HEADER {

   // hash chain fields

   NSK_CB      hash_spin[ NRL_HASH_HEADS ]; //added by SH
   LIST_ENTRY  hash_heads[ NRL_HASH_HEADS ];

   LONG        spin;
   DWORD       type;
   DWORD       index;
   DWORD       pid;            // added by SH - do not change order of these four fields           
   DWORD       tid;            //   "     
   DWORD       line;           //   "
   char        file[4];        //   "
   DWORD        headertype;
   LONG        portnamecounter;      // for $X, $Y and $Znnnn names.

   // common table header

   TABLE_HEADER x;

} NSK_NRL_TABLE_HEADER, *PNSK_NRL_TABLE_HEADER;




// pool header structure.  This struct is allocated at the
// beginning of a pool area.

typedef struct _POOL_HEADER {
    DWORD        eyecatcher;      // 0

    PVOID        poolstart;      // 4
    PVOID        poolend;         // 8
    PVOID        curhigh;         // 12

    LIST_ENTRY    freelist;  // 16
    PVOID        maxhigh;         // 24
    DWORD        curalloc;      // 28
    DWORD        maxalloc;      // 32
    DWORD        failsize;      // 36
   DWORD    fails;        // 40
    DWORD        gets;             // 44
    DWORD        puts;             // 48
    SHORT        curfrag;         // 52
    SHORT        maxfrag;         // 54
    DWORD        poolsize;      // 56
                           // 60
} POOL_HEADER, *PPOOL_HEADER;

#define tag_overhead 4L
//#define min_block_size 32L
const long min_block_size = 32L;

typedef struct _BLOCK_HEAD {
    LONG        tag;            // allocated < 0, free > 0

   union {
        LIST_ENTRY    list;        // free: freelist ptrs
        short        data;           // allocated: data
    };

    LONG        endtag;            // allocated < 0, free > 0
} BLOCK_HEAD, *PBLOCK_HEAD;

#pragma pack(pop,enter_NSKmem)


  // The free_spin macro is called from NSKmain.cpp in msgsys_cleanup_thread
  // and in NSKport.cpp from NSKQCBDelete while doing cleanup.
	// if the spin lock is held, then wait for 20 ms and clear it.  The
  // assumption is that if the lock is still held, then it must be an
  // orphan lock as the data structure is being deallocated.
	// (This seems quite risky, but as yet we don't have an alternative
	// way to release all the locks held by a process that died.)
#ifndef __TDM_NONSKLOCKINFO
#define free_spin( spin, pid )  do {                                                                \
                                if(spin != 0) {                                                 \
                                    Sleep(20); spin = 0;                                        \
                                    AddLockerInfo(&spin, __LINE__,__NSK_FILE_FOR_UNLOCK__,pid, GetCurrentThreadId());         \
                                } } while (0)
#else
#define free_spin( spin, pid )  do {                                                                \
                                if(spin != 0) {                                                 \
                                    Sleep(20); spin = 0;                                        \
                                } } while (0)

#endif


//=========================================================================
// procedure declarations
//=========================================================================

#ifndef NSKMem_C

DllImport
void * NSKGetFirstInQ ( LIST_ENTRY *phead) ;

DllImport
BOOL NSKIsQEmpty( LIST_ENTRY *phead);

DllImport
void NSKQueueInit( LIST_ENTRY *phead );

DllImport
LONG NSKInsert( LIST_ENTRY *pwhere, LIST_ENTRY *pitem );

DllImport
LONG NSKDelete( LIST_ENTRY *pitem );

DllImport
PVOID PoolDefine( PVOID PoolPtr, DWORD PoolSize, PLONG error );

DllImport
LONG PoolPutSpace( PPOOL_HEADER pph, PVOID pelement, short poolCheck = 0);

DllImport
PVOID PoolGetSpace( PPOOL_HEADER pph,
                 DWORD size,
                 DWORD *error, short poolCheck = 0 );

DllImport // For NSK-Lite use only
LONG PgPoolPutSpace( PVOID pelement,
                     BOOL halt_on_error = TRUE );
DllImport // For NSK-Lite use only
PVOID PgPoolGetSpace( DWORD size,
                      DWORD *error       = NULL,
                      BOOL halt_on_error = TRUE );

DllImport
void TestPool( PVOID membase, DWORD length, int maxalloc = 0 );

DllImport
PVOID NSKTableSetup( PVOID base,            // base address
                     DWORD headersize,      // bytes in header
                     DWORD entrysize,       // entry size in bytes
                     DWORD entries,         // entries to allocate
                     DWORD maxentries,       // max entries
                     PVOID *nextaddr,      // address after table
                     DWORD headertype,  // type of table header
                     DWORD    type );   // type of data structure
DllImport
void RealNSKTablePut( PVOID ptable, PVOID pentry, int line, PCHAR file );

DllImport
PVOID RealNSKTableGet( PVOID ptable, int line, PCHAR file );

DllImport
DWORD RealNSKLockGet( DWORD * plock, int line, PCHAR file );

DllImport
void RealNSKLockPut( DWORD * plock, int line, PCHAR file );

DllImport
inline void AddLockerInfo( DWORD *plock, int line, PCHAR file, DWORD pid, DWORD tid );

DllImport
BOOL NSKIsPcbLockedBySelf();


#endif

#endif
