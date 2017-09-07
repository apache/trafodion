/**********************************************************************
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
**********************************************************************/
#ifndef MEMORYMONITOR_H
#define MEMORYMONITOR_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         MemoryMonitor.h
 * Description:  Class declarations to monitor memory pressure.
 *
 *
 * Created:      8/2/99
 * Modified:     
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include <fstream>
#include "NAMemory.h"
#include "Int64.h"
// MemoryMonitor implements a very basic memory pressure detection mechanism.
// Due to the nature of the used heuristics (the page fault rate is the
// main indicator for memory pressure) the core of MemoryMonitor is platform
// dependent. The current implementaion is done for Windows NT and uses the
// NT performance data helper DLL.

// The memory monitor itself. There is one instance of MemoryMonitor per
// node. The MXSSCP process on the node creates a thread to constantly update
// the MemoryMonitor object values
class MemoryMonitor : public NABasicObject {
public:
  // windowSize is the number of entries in the slinding window  we keep
  // for the page faults. sampleInterval is the time in seconds between
  // two window entries.
  MemoryMonitor(Lng32 windowSize, Lng32 sampleInterval, CollHeap *heap);
  ~MemoryMonitor();

  // calculate the memory pressure indicator. This indicator has a value between 0
  // (no pressure) and 99 (system is in deep, deep trouble). It is up to the
  // caller to decide on the action to take based on this value
  Lng32 memoryPressure();

  inline float getPageFaultRate() {  return pageFaultRate_; }

  inline Lng32 getSampleInterval() const { return sampleInterval_; };
  inline ULng32 getPhysMemInBytes() const 
  {
    float physKBytes = physKBytes_ <= 4 * 1024 * 1024 ? physKBytes_ : 4 * 1024 * 1024;
    return (ULng32)(physKBytes * 1000);
  }

  inline void setEnable(NABoolean b) { enable_ = b; }
  inline void enableLogger() {loggerEnabled_ = TRUE; }
  inline NABoolean isLoggerEnabled() { return loggerEnabled_; }
  Int64 availablePhyMemKb() { return memFree_; }
  static DWORD WINAPI memMonitorUpdateThread(void * param);
private:
  void update();
  void updatePageFaultRate(Int64 pageFaultValue);

private:

  // data members
  float physKBytes_;                  // size of main memory in KBytes
  float physKBytesRatio_;
  float availBytesPercent_;           // % available (unused) physical memory
  float commitBytesPercent_;	      // % of virtual memory committed.
  float commitPhysRatio_;             // ratio of committed to physical memory.

  NABoolean pagingCounterAdded_;					     
  float pageFaultRate_;
  Int64 prevPageFault_;
  Int64 prevTime_;
  Int64 currTime_;
  Int64 logTime_;
  Int64 memTotal_;
  Int64 memActive_;
  Int64 memInactive_;
  // Written by monitor thread, and read by main thread.
  Int64 memFree_;   //pages that are free in kb.
  float entryCount_;

  // Written by main thread, and read by monitor thread.
  // ------------------------------------------------------------
  NABoolean enable_;            // TRUE => collect counter values.    
  NABoolean resetEntryCount_;   // For measuring page fault rate.
  // Written by monitor thread, and read by main thread.
  Lng32 pressure_;

  // Only referenced by the main thread.
  NABoolean resetOnce_;

  Lng32 sampleInterval_;                // in milliseconds

  NABoolean loggerEnabled_;

// SQ_LINUX will use Win32 API for worker thread to update pressure.
  static NABoolean threadIsCreated_;
  static HANDLE updateThread_;
  static ULng32 threadId_;
  FILE* fd_meminfo_;
  FILE* fd_vmstat_;
};
#endif






