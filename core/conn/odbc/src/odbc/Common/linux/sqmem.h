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
#ifndef _SQmem_H
#define _SQmem_H
#define DllImport
#define DllExport
//#define DllImport _declspec(dllimport)
//#define DllExport _declspec(dllexport)

#define NO_ERROR 0

typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY * volatile Flink;
   struct _LIST_ENTRY * volatile Blink;
} LIST_ENTRY, *PLIST_ENTRY;



#pragma pack(push, enter_SQmem, 4)

// pool header structure.  This struct is allocated at the
// beginning of a pool area.

typedef struct _POOL_HEADER {
    int        eyecatcher;      // 0

    void *        poolstart;      // 4
    void *        poolend;         // 8
    void *        curhigh;         // 12

    LIST_ENTRY    freelist;  // 16
    void *        maxhigh;         // 24
    int        curalloc;      // 28
    int        maxalloc;      // 32
    int        failsize;      // 36
   int    fails;        // 40
    int        gets;             // 44
    int        puts;             // 48
    short        curfrag;         // 52
    short        maxfrag;         // 54
    long        poolsize;      // 56
                           // 60
} POOL_HEADER, *PPOOL_HEADER;

#define tag_overhead 4L
//#define min_block_size 32L
const long min_block_size = 32L;

typedef struct _BLOCK_HEAD {
    long        tag;            // allocated < 0, free > 0

   union {
        LIST_ENTRY    list;        // free: freelist ptrs
        short        data;           // allocated: data
    };

    long        endtag;            // allocated < 0, free > 0
} BLOCK_HEAD, *PBLOCK_HEAD;

#pragma pack(pop,enter_SQmem)

//=========================================================================
// procedure declarations
//=========================================================================

#ifndef SQMem_C

DllImport
long SQDelete(LIST_ENTRY *pitem);

DllImport
void * PoolDefine(void * PoolPtr, int PoolSize, short *error);

DllImport
short PoolPutSpace(PPOOL_HEADER pph, void * pelement);

DllImport
void * PoolGetSpace(PPOOL_HEADER pph, int size, short *error);

DllImport
short PoolCheck(PPOOL_HEADER pph);

DllImport
void TestPool(void * membase, int length);

#endif

#endif
