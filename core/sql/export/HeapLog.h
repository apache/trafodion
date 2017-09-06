/* -*-C++-*-
 *****************************************************************************
 *
 * File:         HeapLog.h
 * Description:  This file contains the declaration of HeapLog class to 
 *               track heap allocations and deallocations.
 *               
 * Created:      3/1/99
 * Language:     C++
 *
 *
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
*
****************************************************************************
*/

#ifndef HEAPLOG__H
#define HEAPLOG__H


#include <stddef.h>
#include "HeapID.h"
#include "NABoolean.h"

// -----------------------------------------------------------------------
// Expand to empty if this is a release build or heaplog is not importable.
// -----------------------------------------------------------------------
#ifndef NA_DEBUG_HEAPLOG
#define HEAPLOG_CONTROL(option)
#define HEAPLOG_ADD_ENTRY(objAddr, objSize, heapNum, heapName) 
#define HEAPLOG_DELETE_ENTRY(objAddr, heapNum)
#define HEAPLOG_REINITIALIZE(heapNum)
#define HEAPLOG_DISPLAY(prompt)
#define HEAPLOG_ON()
#define HEAPLOG_OFF()


// -----------------------------------------------------------------------
#else // debug build and heaplog is importable.
// -----------------------------------------------------------------------
// COntrol the heaplog.
#define HEAPLOG_CONTROL(option) \
{ \
   HeapLogRoot::control(option); \
}
// Track allocations.
#define HEAPLOG_ADD_ENTRY(objAddr, objSize, heapNum, heapName) \
{ \
  if (HeapLogRoot::track) \
    HeapLogRoot::addEntry(objAddr, objSize, (Lng32&)heapNum, heapName); \
}
// Track deallocations.
#define HEAPLOG_DELETE_ENTRY(objAddr, heapNum) \
{ \
  if (HeapLogRoot::trackDealloc) \
    HeapLogRoot::deleteEntry(objAddr, heapNum); \
}
// Track heap re-initialization.
#define HEAPLOG_REINITIALIZE(heapNum) \
{ \
  if (HeapLogRoot::trackDealloc) \
    HeapLogRoot::deleteLogSegment(heapNum, FALSE); \
}
#define HEAPLOG_DISPLAY(prompt) \
{ \
  HeapLogRoot::display(prompt); \
}
#define HEAPLOG_ON() \
{ \
  if (HeapLogRoot::track) HeapLogRoot::disable(FALSE); \
}
#define HEAPLOG_OFF() \
{ \
  if (HeapLogRoot::track) HeapLogRoot::disable(TRUE); \
}

// Reserved heaps.
const Lng32 NA_HEAP_BASIC = 1;

#endif

// Flags set by parser based on syntax clauses.
class LeakDescribe {

public:

  enum 
  {
    FLAGS         = 0x2f,

    FLAG_SQLCI    = 0x1,
    FLAG_ARKCMP   = 0x2,
    FLAG_BOTH     = 0x3,
    FLAG_CONTINUE = 0x8,
    FLAG_OFF      = 0x10,
    FLAG_PROMPT   = 0x20
  };
};


#ifdef __NOIMPORT_HEAPL
#else
// -----------------------------------------------------------------------
// For both debug and release builds.
// -----------------------------------------------------------------------
typedef enum HeapControlEnum
{
  LOG_START           = 1, 
  LOG_DELETE_ONLY,
  LOG_RESET_START,
  LOG_DISABLE,
  LOG_RESET,       
  LOG_RESET_DISABLE
} HeapControlEnum;

// -----------------------------------------------------------------------
// One global log structure per process.
// -----------------------------------------------------------------------
class HeapLog;

class HeapLogRoot {

public:
  
  static HeapLog *log;
  // Maximum heap number assigned.
  static Lng32 maxHeapNum;
  static Lng32 track;
  static Lng32 trackDealloc;

  static void control(HeapControlEnum option);

  static void addEntry( void  * objAddr 
		      , Lng32 objSize  
		      , Lng32 &heapNum  
		      , const char *heapName = NULL
		      );
  static void deleteEntry(void  * objAddr, Lng32 heapNum);
 
  static void deleteLogSegment(Lng32 heapNum, NABoolean setfree);  

  static void disable(NABoolean b);

  static void display(NABoolean prompt, Lng32 sqlci = 1);

  // called by arkcmp. 
  static Lng32 getPackSize();
  static void pack(char *buf, ULng32 flags);

  // called by executor.
  static Lng32 fetchLine( char *buf 
                       , ULng32 flags
		       , char *packdata = NULL
		       , Lng32 datalen = 0
		       );

  // called by heap constructor.
  static Lng32 assignHeapNum();

  virtual void pureVirtual() = 0;

private:

  static void control2( ULng32 flags
		      , ULng32 mask
		      );
  // Init the log;
  static void initLog();
  // Reset the log.
  static void reset();
};

#endif

#endif
