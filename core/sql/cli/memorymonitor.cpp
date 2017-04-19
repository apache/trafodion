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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         MemoryMonitor.cpp
 * Description:  Methods to detect memory pressure from the operating system.
 *               On NT, a separate thread is doing this work, on NSK the
 *               pressure is detected between subsequent calls.
 *
 * Created:      8/2/99
 * Modified:     
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "memorymonitor.h"
#include "ComRtUtils.h"
#include "NAAssert.h"
#include "PortProcessCalls.h"

#include <time.h>
#include "QRLogger.h"
#include <cextdecs/cextdecs.h>
#undef DllImport
#define DllImport __declspec( dllimport )
/*
// fix a problem with different versions of pdh.dll 
#ifdef PdhOpenQuery
#undef PdhOpenQuery   //       PdhOpenQueryA or PdhOpenQueryW
extern "C" Lng32 __stdcall 
PdhOpenQuery (LPCSTR  szD, DWORD  dw, HQUERY  *phQ );
#endif
*/
#define FREAD_BUFFER_SIZE 2048

NABoolean MemoryMonitor::threadIsCreated_ = 0;
HANDLE MemoryMonitor::updateThread_ = (HANDLE) 0;
ULng32 MemoryMonitor::threadId_ = 0;

DWORD WINAPI MemoryMonitor::memMonitorUpdateThread(void * param) {
  // params points to the memory monitor object
  MemoryMonitor *memMonitor = (MemoryMonitor*) param;
  Lng32 sleepTime = memMonitor->getSampleInterval();
  while (TRUE) {
    memMonitor->update();
    Sleep((ULng32)(sleepTime));
  };
  return 0;
};

#define LOG_INTERVAL 300000000L

MemoryMonitor::MemoryMonitor(Lng32 windowSize,
			     Lng32 sampleInterval,
			     CollHeap *heap)
  : physKBytes_(0),
    physKBytesRatio_(0),
    availBytesPercent_(1.0),
    commitBytesPercent_(0),
    commitPhysRatio_(0),

    pagingCounterAdded_(FALSE),
    pageFaultRate_(0),
    prevPageFault_(0),
    prevTime_(0),
    currTime_(0),
    logTime_(0),
    entryCount_(1),
    resetEntryCount_(TRUE),
    resetOnce_(FALSE),

    enable_(TRUE),
    pressure_(0),
    memFree_(-1),
    memActive_(-1),
    memInactive_(-1),
    loggerEnabled_(FALSE),
    sampleInterval_(sampleInterval * 1000)
{
  // if the windowSize is 0, we do not need memory monitor.
  assert(windowSize);
  char buffer[FREAD_BUFFER_SIZE+1];
  char *currPtr;
  size_t bytesRead = 0;
  fd_meminfo_ = fopen("/proc/meminfo", "r");
  if (fd_meminfo_) {
    bytesRead = fread(buffer, 1, FREAD_BUFFER_SIZE, fd_meminfo_);
    if (ferror(fd_meminfo_))
       assert(false); 
    if (feof(fd_meminfo_))
       clearerr(fd_meminfo_); 
    buffer[bytesRead] = '\0';
    currPtr = strstr(buffer, "MemTotal");
    if (currPtr) {
      sscanf(currPtr, "%*s " PF64 " kB", &memTotal_);
      physKBytes_ = memTotal_;
      physKBytesRatio_ = physKBytes_ / (8 * 1024 * 1024);
    }
    else {
      // something unexpected. let's just close the file
      fclose(fd_meminfo_);
      fd_meminfo_ = 0;
    }
  }

  // Disable monitoring if the envvar is set.
  // We do this here to get the initial meminfo 
  // even if one wants to disable continous memory 
  // monitoring.
  char *lv_envVar = getenv("SQL_DISABLE_MEMMONITOR");
  if (lv_envVar && (strcmp(lv_envVar, "1") == 0)) {
    return;
  }

  ULng32 pageSize = 0;  

  fd_vmstat_ = fopen("/proc/vmstat", "r");

  if (!threadIsCreated_)
    {
      // and finally start the update thread
      updateThread_ = CreateNewThread(
                           &MemoryMonitor::memMonitorUpdateThread, // Thread func
                           this );   // Argument for thread
      threadIsCreated_ = TRUE;
    }

};

MemoryMonitor::~MemoryMonitor() {

  if (fd_meminfo_)
  {
    fclose(fd_meminfo_);
    fclose(fd_vmstat_);
  }
};

// Called by main thread only.
Lng32 MemoryMonitor::memoryPressure() {

//SQ_LINUX #ifdef NA_WINNT

  return pressure_;
};

// - Compute weighted running average for pageFaultRate
//   (# of disk writes per second).
// - Start a new cycle when the main thread sees the pressure.
// [Eric]
void MemoryMonitor::updatePageFaultRate(Int64 pageFault) {
  
  float delta = (float)(pageFault - prevPageFault_); 
  if (delta < 0)
    return;

  currTime_ = JULIANTIMESTAMP(OMIT,OMIT,OMIT,OMIT);
  
  // Just in case
  if (currTime_ <= prevTime_)
    return;

  if (resetEntryCount_)
    { // Start a new cycle after the main thread sees pressure peek.
      resetEntryCount_ = FALSE;
      entryCount_ = 1;
      prevTime_ = currTime_;
      prevPageFault_ = pageFault;
      return;
    }

  float ratio = (float)(1.0 / entryCount_);
  pageFaultRate_ = (1 - ratio) * pageFaultRate_ + ratio * 
                   delta / ((float)(currTime_ - prevTime_) / 1E6);
  prevPageFault_ = pageFault;
  prevTime_ = currTime_;
  if (entryCount_ < 3)
    entryCount_++;
}

void MemoryMonitor::update() {
        static char logBuffer[300];	
	Int64 memFree = -1, memCommitAS = 0;
	Int64 pgpgout = -1, pgpgin = -1;
        Int64 memActive = -1, memInactive = -1;
	size_t bytesRead = 0;
	char * currPtr;
	char buffer[FREAD_BUFFER_SIZE+1];
        Int32 success;

	if (fd_meminfo_ != NULL)  {
           success = fseek(fd_meminfo_, 0, SEEK_SET);
           if (success == 0)
              bytesRead = fread(buffer, 1, FREAD_BUFFER_SIZE, fd_meminfo_);
           if (ferror(fd_meminfo_))
               assert(false); 
           if (feof(fd_meminfo_))
              clearerr(fd_meminfo_); 
           buffer[bytesRead] = '\0';
           currPtr = strstr(buffer, "MemFree");
	   if (currPtr) 
              sscanf(currPtr, "%*s " PF64 " kB", &memFree);
           currPtr = strstr(buffer, "Committed_AS");
	   if (currPtr) 
              sscanf(currPtr, "%*s " PF64 " kB", &memCommitAS);
           currPtr = strstr(buffer, "Active:");
	   if (currPtr) 
              sscanf(currPtr, "%*s " PF64 " kB", &memActive);
           currPtr = strstr(buffer, "Inactive:");
	   if (currPtr) 
              sscanf(currPtr, "%*s " PF64 " kB", &memInactive);
	   if (memTotal_ > 0 && memCommitAS > 0 && memFree > -1)
               commitPhysRatio_ = (float)memCommitAS / (float) memTotal_;
           memFree_ = MAXOF(memFree, -1);
           memActive_ = MAXOF(memActive, -1);
           memInactive_ = MAXOF(memInactive, -1); 
        }
	if (fd_vmstat_ != NULL) {
           success = fseek(fd_vmstat_, 0, SEEK_SET);
           if (success == 0) 
	      bytesRead = fread(buffer, 1, FREAD_BUFFER_SIZE, fd_vmstat_);
           if (ferror(fd_vmstat_))
              assert(false); 
           if (feof(fd_vmstat_))
              clearerr(fd_vmstat_); 
           buffer[bytesRead] = '\0';
           currPtr = strstr(buffer, "pgpgin");
	   if (currPtr)  
              sscanf(currPtr, "%*s " PF64 " kB", &pgpgin);
           currPtr = strstr(buffer, "pgpgout");
	   if (currPtr) 
              sscanf(currPtr, "%*s " PF64 " kB", &pgpgout);
	   if (pgpgin > -1 && pgpgout > -1)
	      updatePageFaultRate(pgpgin + pgpgout);
        }
        float percentFree = (float)memFree_ / (float)memTotal_;
        float percentActive = (float)memActive_ / (float)memTotal_;
        float percentInactive = (float)memInactive_ / (float)memTotal_; 
        float normalizedPageFaultRate = pageFaultRate_ / physKBytesRatio_;
	Lng32 prevPressure = pressure_;
        pressure_ = MAXOF(((1 - 4 * percentFree) * (normalizedPageFaultRate / 25) * commitPhysRatio_), 0);
        if (isLoggerEnabled() && ((pressure_ != prevPressure) || (currTime_ - logTime_ > LOG_INTERVAL))) {
           QRLogger::log(CAT_SQL_SSCP, LL_INFO,  
                 "pctFree=%.3f, pctActive=%.3f, pctInactive=%.3f pageFaultRate=%.3f, (free*normpagefault*commitratio) = (%.3f * %.3f * %.3f), pressure=%d\n",
                 percentFree,
                 percentActive,
                 percentInactive,
                 pageFaultRate_,
                 (1 - 4 * percentFree),
                 (normalizedPageFaultRate / 25),
                 commitPhysRatio_,
                 pressure_);
           logTime_ = currTime_;
        } 
	return;
}




