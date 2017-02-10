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




#include <cextdecs/cextdecs.h>
#undef DllImport
#define DllImport __declspec( dllimport )

// fix a problem with different versions of pdh.dll 
#ifdef PdhOpenQuery
#undef PdhOpenQuery   //       PdhOpenQueryA or PdhOpenQueryW
extern "C" Lng32 __stdcall 
PdhOpenQuery (LPCSTR  szD, DWORD  dw, HQUERY  *phQ );
#endif


//SQ_LINUX #ifdef NA_WINNT

NABoolean MemoryMonitor::threadIsCreated_ = 0;
HANDLE MemoryMonitor::updateThread_ = (HANDLE) 0;
ULng32 MemoryMonitor::threadId_ = 0;

DWORD WINAPI memMonitorUpdateThread(void * param) {
  // params points to the memory monitor object
  MemoryMonitor *memMonitor = (MemoryMonitor*) param;
  Lng32 sleepTime = memMonitor->getSampleInterval();
  float scale = 1.0;
  while (TRUE) {
    memMonitor->update(scale);
    Sleep((ULng32)(sleepTime * scale));
  };
  return 0;
};

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
    entryCount_(1),
    resetEntryCount_(TRUE),
    resetOnce_(FALSE),

    enable_(TRUE),
    pressure_(0),

    sampleInterval_(sampleInterval * 1000)
{
  // if the windowSize is 0, we do not need memory monitor.
  assert(windowSize);
  char buffer[2048];
  char *currPtr;
  size_t bytesRead;
  fd_meminfo_ = fopen("/proc/meminfo", "r");
  if (fd_meminfo_) {
    bytesRead = fread(buffer, 1, 2048, fd_meminfo_);
    if (ferror(fd_meminfo_))
       assert(false); 
    if (feof(fd_meminfo_))
       clearerr(fd_meminfo_); 
    else
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

  // log info to a file, if requested
  char *lv_logfile = getenv("SQL_MEMMONITOR_LOGFILE");
  if (lv_logfile) {
    fd_logfile_ = fopen(lv_logfile, "a");
  }
  else
    fd_logfile_ = NULL;

  ULng32 pageSize = 0;  

  fd_vmstat_ = fopen("/proc/vmstat", "r");

  if (!threadIsCreated_)
    {
      // and finally start the update thread
      updateThread_ = CreateNewThread(
                           &memMonitorUpdateThread, // Thread func
                           this );   // Argument for thread
      threadIsCreated_ = TRUE;
    }

};

MemoryMonitor::~MemoryMonitor() {

//SQ_LINUX #ifdef NA_WINNT
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

  Int64 currTime = JULIANTIMESTAMP(OMIT,OMIT,OMIT,OMIT);
  
  // Just in case
  if (currTime <= prevTime_)
    return;

  if (resetEntryCount_)
    { // Start a new cycle after the main thread sees pressure peek.
      resetEntryCount_ = FALSE;
      entryCount_ = 1;
      prevPageFault_ = pageFault;
      prevTime_ = currTime;
      return;
    }

  float ratio = (float)(1.0 / entryCount_);
#pragma warning (disable : 4244)   //warning elimination
#pragma nowarn(1506)   // warning elimination 
  pageFaultRate_ = (1 - ratio) * pageFaultRate_ + ratio * 
                   delta / ((float)(currTime - prevTime_) / 1E6);
#pragma warn(1506)  // warning elimination 
#pragma warning (default : 4244)   //warning elimination
  prevPageFault_ = pageFault;
  prevTime_ = currTime;
  if (entryCount_ < 3)
    entryCount_++;
}

void MemoryMonitor::update(float &scale) {
	
	if (fd_meminfo_ == NULL) 
		return; // error handling here.
        Int32 success = fseek(fd_meminfo_, 0, SEEK_SET);
        if (success != 0)
                return;
	char buffer[4096];
	Int64 memFree = -1, memCommitAS = 0;
	Int64 pgpgout = -1, pgpgin = -1;
	size_t bytesRead;
	char * currPtr;
        bytesRead = fread(buffer, 1, 2048, fd_meminfo_);
        // Make sure there wasn't an error (next fseek will clear eof)
        if (ferror(fd_meminfo_))
           assert(false); 
        if (feof(fd_meminfo_))
           clearerr(fd_meminfo_); 
        else
           buffer[bytesRead] = '\0';
        currPtr = strstr(buffer, "MemFree");
	if (currPtr) sscanf(currPtr, "%*s " PF64 " kB", &memFree);
        currPtr = strstr(buffer, "Committed_AS");
	if (currPtr) sscanf(currPtr, "%*s " PF64 " kB", &memCommitAS);

	Lng32 prevPressure = pressure_;
	if (memTotal_ > 0 && memCommitAS > 0 && memFree > -1)
		commitPhysRatio_ = (float)memCommitAS / (float) memTotal_;
	else
	{
	        scale = 6;
		pressure_ = 0;
		return;
	}
        if(memFree > 0)
        {
          memFree_ = memFree;
        }
        else
        {
           memFree_ = 0;
           return;
        }

	if (fd_vmstat_ == NULL) 
		return; // error handling here.
        success = fseek(fd_vmstat_, 0, SEEK_SET);
        if (success != 0)
	{
	        scale = 6;
		pressure_ = 0;
		return;
	}
	bytesRead = fread(buffer, 1, 2048, fd_vmstat_);
        if (ferror(fd_vmstat_))
           assert(false); 
        if (feof(fd_vmstat_))
           clearerr(fd_vmstat_); 
        else
           buffer[bytesRead] = '\0';
        currPtr = strstr(buffer, "pgpgin");
	if (currPtr) sscanf(currPtr, "%*s " PF64 " kB", &pgpgin);
        currPtr = strstr(buffer, "pgpgout");
	if (currPtr) sscanf(currPtr, "%*s " PF64 " kB", &pgpgout);
	if (pgpgin > -1 && pgpgout > -1)
	  updatePageFaultRate(pgpgin + pgpgout);
	else
	{
	        scale = 6;
		pressure_ = 0;
		return;
	}
        float percentFree = (float)memFree / (float)memTotal_;
        float normalizedPageFaultRate = pageFaultRate_ / physKBytesRatio_;
        pressure_ = MAXOF(MINOF(((1 - 4 * percentFree) * (normalizedPageFaultRate / 25) * commitPhysRatio_), 100), 0);
	scale = 1 + (100 - pressure_) * 0.05;

        if (fd_logfile_) {
          time_t now = time(0);
          char timebuf[32];
          char* dt = ctime_r(&now, timebuf);
          size_t dtLen = strlen(timebuf);

          // remove trailing \n from timebuf
          if (dtLen > 1)
            timebuf[dtLen-1] = '\0';
          fprintf(fd_logfile_,
                  "%s: pctFree=%f, pageFaultRate=%f, (free*normpagefault*commitratio) = (%f * %f * %f), pressure=%d\n",
                  timebuf,
                  percentFree,
                  pageFaultRate_,
                  (1 - 4 * percentFree),
                  (normalizedPageFaultRate / 25),
                  commitPhysRatio_,
                  pressure_);
          fflush(fd_logfile_);
        }
	return;
}




