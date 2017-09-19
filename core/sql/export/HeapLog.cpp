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
****************************************************************************
*
* File:         HeapLog.cpp
*
* Description:  Track object allocations and deallocations.
*              
* Created:      3/1/99
* Language:     C++
*
*
*
****************************************************************************
*/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "HeapLog.h"
#include "HeapLogImpl.h"

using namespace std;

#define HEAP_NUM_BASE         19
#define HEAP_NUM_RESERVED      9
#define HEAP_NUM_NULL         -1
#define FETCH_EOF              1
// -----------------------------------------------------------------------
// Initialize static members.
// -----------------------------------------------------------------------
HeapLog *HeapLogRoot::log = NULL; 
Lng32 HeapLogRoot::track = FALSE;
Lng32 HeapLogRoot::trackDealloc = FALSE;
Lng32 HeapLogRoot::maxHeapNum = HEAP_NUM_RESERVED;

static void BreakNow()
{
  DebugBreak();
}

// -----------------------------------------------------------------------
// Break to show call stack when the break condition is satisfied.
// -----------------------------------------------------------------------
static void BreakOnLeak( const Lng32 heapNum
                       , const Lng32 indx
                       , const Lng32 size
                       )
{
  static Lng32 object_index = -1;  
  static Lng32 object_size = -1;  
  static Lng32 heap_id = -1; 
  // ------------------------------------------------------------------
  // To catch memory leaks one by one:
  // 1. Set or enable a breakpoint at the if-statement. 
  // 
  // 2. When the breakpoint is hit, modify object_index, object_size, 
  //    and/or heap_id for locating the first leak in a prior leak report.
  //
  // 3. Disable the breakpoint and resume the execution.
  //
  // 4. When the code invokes debugger again, check if
  //    heapNum matches HEAP_ID, indx approximates to OBJECT_INDEX, and
  //    size matches OBJECT_SIZE from that in the leak report.
  //    If so, the call stack shows the corresponding leak location. 
  //
  // 5. Modify object_index, object_size, and/or heap_id for 
  //    locating the next leak.
  // 
  // 6. Repeat 4 and 5.
  // ------------------------------------------------------------------
  if ((object_index >= 0) &&
      (indx >= object_index) &&
      (size == object_size || object_size < 0) &&
      (heapNum == heap_id || heap_id < 0))
    { 
      BreakNow();  // invokes debugger
    }
}

// -----------------------------------------------------------------------
// Add a log entry to track allocations.
// -----------------------------------------------------------------------
void HeapLogRoot::addEntry( void *  addr
	         	  , Lng32 size 
		          , Lng32 &heapNum
		          , const char *heapName
		          )
{
  if ((heapNum >= MAX_NUM_HEAPS) ||
      (log != NULL && 
       log->disableLevel_ > 0))
    return;
  
  if (log == NULL)
    initLog();

  if (heapNum < 0)
    { // assign a unique ID.
      heapNum = assignHeapNum();
    }

  assert(heapNum > 0);
  Lng32 indx = log->addEntry(addr, size, heapNum, heapName);
  BreakOnLeak(heapNum, indx, size);

  return;
}

// -----------------------------------------------------------------------
// Reset a log entry associated with this object addr.
// Called when an object is deleted.
// -----------------------------------------------------------------------
void HeapLogRoot::deleteEntry( void *  addr 
			     , Lng32 heapNum
                             )
{
  if ((heapNum < 0) ||
      (heapNum >= MAX_NUM_HEAPS) ||
      (log == NULL) ||
      (log->disableLevel_ > 0))
    return;

  // Locate the log segment for the heap.
  HeapLogSegment &seg = log->header_[heapNum];
  if (seg.object_ == NULL)
    return;
  
  // Find the entry with matching addr.
  Lng32 i;
  if (seg.deleted_ == -1)
    {
      for (i = seg.last_; i >= 0; i--)
	if ((seg.object_[i].size > 0) &&
	  (seg.object_[i].addr == addr))
	  goto cont;
    }
  else
    { // Search the neighborhood of last deleted entry 
      // indicated by seg.deleted_
      Lng32 left = seg.deleted_;
      Lng32 right = seg.deleted_ + 1;
      Lng32 limit;
      Lng32 RANGE = 3;
      
      while((left >= 0) || (right <= seg.last_))
	{
	  if (left >= 0)
	    { // search the left side.
	      limit = left - RANGE;
	      if (limit < 0)
		limit = 0;
	      for (i = left; i >= limit; i--)
		if ((seg.object_[i].size > 0) &&
		    (seg.object_[i].addr == addr))
		  goto cont;
	      left = limit - 1;
	    }
	  if (right <= seg.last_)
	    { // search the right side.
	      limit = right + RANGE;
	      if (limit > seg.last_)
		limit = seg.last_;
	      for (i = right; i <= limit; i++)
		if ((seg.object_[i].size > 0) &&
		    (seg.object_[i].addr == addr))
		  goto cont;
	      right = limit + 1;
	    }
	  RANGE = 50;
	} // while
    }
  // not found
  return;

cont:
  // Found.
  // Negate the size.
  // Remember the entry in seg.deleted_.
  seg.object_[i].size *= -1;
  assert(seg.object_[i].size < 0);

  seg.usageCount_--;
  seg.totalSize_ += seg.object_[i].size;
  seg.deleted_ = i;

  if (seg.usageCount_ > seg.last_ * 0.5)
    return;
     
  // Compress the log segment when usage rate drops below 0.5.
  Lng32 freePos = 0;
  for (i = 0; i <= seg.last_; i++)
    {
      if (seg.object_[i].size <= 0)
	continue;
      
      if (freePos < i)
	{ // Copy and free slot i.
	  seg.object_[freePos] = seg.object_[i];
	  seg.object_[i].size *= -1; 
	}
      freePos++;
    }
  if (freePos < seg.last_)
    seg.last_ = freePos;
  // Reposition the last deleted entry.
  seg.deleted_ = seg.last_ >> 2;
}

// -----------------------------------------------------------------------
// Delete a log segment associated with a heap.
// Called when a heap is deleted or reinitialized.
// -----------------------------------------------------------------------
void HeapLogRoot::deleteLogSegment(Lng32 heapNum, NABoolean setfree)
{  
  if ((heapNum < 0) ||
      (heapNum >= MAX_NUM_HEAPS) ||
      (log == NULL))
     return;

   HeapLogSegment &seg = log->header_[heapNum];
   if (seg.object_ != NULL)
     {
       HEAPLOG_OFF();
       delete [] seg.object_;
       HEAPLOG_ON();
       seg.object_ = NULL;
     }
   seg.slotCount_ = 0;
   seg.usageCount_ = 0;
   seg.last_ = -1;
   seg.deleted_ = -1;
   seg.totalSize_ = 0;
   // Check if the entry can be reassigned to other heaps.
   if (setfree)
     seg.free_ = TRUE;
}

// -----------------------------------------------------------------------
// Session control the heap log.
// -----------------------------------------------------------------------
void HeapLogRoot::control(HeapControlEnum option)
{
#ifndef NA_DEBUG_HEAPLOG
  track        =
  trackDealloc = FALSE; 
  return;
#endif
  switch(option) {  
  case LOG_START:
    {
      track        =
      trackDealloc = TRUE; 
      initLog();
      return;
    }
  case LOG_DELETE_ONLY:
    { 
      track        = FALSE;
      trackDealloc = TRUE;
      return;
    }
  case LOG_RESET_START:
    {
      reset();
      track        =
      trackDealloc = TRUE;
      initLog();
      return;
    }
  case LOG_DISABLE:
    {
      track        =
      trackDealloc = FALSE;
      return;
    }
  case LOG_RESET:
    { 
      reset();
      return;
    }
  case LOG_RESET_DISABLE:
    { 
      reset();
      track        =
      trackDealloc = FALSE;
      return;
    }
  default:
    break;
  }
}

// -----------------------------------------------------------------------
// Do session control based on syntax clauses.
// -----------------------------------------------------------------------
void HeapLogRoot::control2(ULng32 flags, ULng32 mask)
{
  if ((flags & mask) == 0)
    {
      control(LOG_RESET_DISABLE);
      return;
    }
  if (flags & LeakDescribe::FLAG_CONTINUE)
    {
      control(LOG_START);
      return;
    }
  if (flags & LeakDescribe::FLAG_OFF)
    {
      control(LOG_RESET_DISABLE);
      return;
    }
  control(LOG_RESET_START);
  return;
}

// -----------------------------------------------------------------------
// Disable logging.
// -----------------------------------------------------------------------
void HeapLogRoot::disable(NABoolean b)
{
  if (log == NULL)
    return;
  if (b)
    log->disableLevel_++;
  else
    log->disableLevel_--;
  assert(log->disableLevel_ >= 0);
}

// -----------------------------------------------------------------------
// Display heap log.
// Open a console window if none.
// If prompt is TRUE, prompt the user to enter a character n, c, or o.
// -----------------------------------------------------------------------
void HeapLogRoot::display( NABoolean prompt 
			 , Lng32 sqlci
			 )
{
}

// ---------------------------------------------------------------------
// Called by ARKCMP to allocate buffer.
// ---------------------------------------------------------------------
Lng32 HeapLogRoot::getPackSize()
{
  Lng32 heapCount = 0;
  Lng32 objCount = 0;
  if (log == NULL)
    return 8;
  for (Int32 i = 0; i <= maxHeapNum ; i++)
    {
      if (log->header_[i].usageCount_ > 0)
	{
	  heapCount++;
	  objCount += log->header_[i].usageCount_;
	}
    }
  // must be in multiple of 8.
  Lng32 size = (heapCount * 3 + objCount + 25) * HeapLog::DISPLAY_LEN;  
  return size;
}

// ---------------------------------------------------------------------
// Pack log data into a flat buffer.
// ---------------------------------------------------------------------
void HeapLogRoot::pack(char *buf, ULng32 flags) 
{
  if (HeapLogRoot::log == NULL ||
      ((flags & LeakDescribe::FLAG_ARKCMP) == 0)
     )
    { // eof, pack null chars.
      buf[0] = buf[1] = '\0';
      control2(flags, LeakDescribe::FLAG_ARKCMP); 
      return;
    }

  if (flags & LeakDescribe::FLAG_PROMPT)
    { // Display log data and prompt the user.
      display(TRUE, 0 /* arkcmp */);
      // eof
      buf[0] = buf[1] = '\0';
      return;
    }

  // Pack all lines, each of which is terminated with '\0'.
  Lng32 len = 0;
  log->status_ = HeapLog::PHASE_1;
  while(log->fetchLine(&buf[len], 0/*arkcmp*/) != FETCH_EOF)
    {
      len += strlen(&buf[len]) + 1;
    }
  buf[len++] = '\0';
  buf[len]   = '\0';
  control2(flags, LeakDescribe::FLAG_ARKCMP); 
  return;
}

// ---------------------------------------------------------------------
// Fetch log line by line called by executor.
// Optionally prompt the user for input.
// return FETCH_EOF if eof else 0.
// ---------------------------------------------------------------------
Lng32 HeapLogRoot::fetchLine( char *buf
			   , ULng32 flags 
			   , char *packdata    /*=NULL*/
			   , Lng32 datalen      /*=0*/
			   )
{
  if ((flags & (LeakDescribe::FLAG_SQLCI | LeakDescribe::FLAG_ARKCMP)) == 0)
    { // neither sqlci nor arkcmp.  Done.
      control(LOG_RESET_DISABLE);
      return FETCH_EOF;
    }

  // Display report in ARKCMP window.
  if ((flags & LeakDescribe::FLAG_PROMPT) &&
      !(flags & LeakDescribe::FLAG_SQLCI))
    return FETCH_EOF;
  
  if (log == NULL)
    initLog();

  log->fetchInit(flags, packdata, datalen);
  if (flags & LeakDescribe::FLAG_PROMPT) 
    { // Display log data and prompt the user.
      display(TRUE, 1 /* sqlci */);
      // eof
      *(short*)buf = 0;
      return FETCH_EOF;
    }

  Lng32 error = log->fetchLine(&buf[2], 1 /*sqlci*/);
#pragma nowarn(1506)   // warning elimination 
  *((short*)buf) = strlen(&buf[2]);
#pragma warn(1506)  // warning elimination 
  if (error == FETCH_EOF)
    control2(flags, LeakDescribe::FLAG_SQLCI);
  return error;
}

// ---------------------------------------------------------------------
// Called by heap constructor.
// ---------------------------------------------------------------------
Lng32 HeapLogRoot::assignHeapNum()
{
  if (log == NULL)
    { // Haven't started logging.
      if (maxHeapNum >= HEAP_NUM_BASE)
	return HEAP_NUM_NULL;
      else
	{ // up to HEAP_NUM_BASE
	  ++maxHeapNum;
	  return maxHeapNum;
	}
    }
  // Find a free entry.
  for(Lng32 i = log->currHeapNum_ + 1; i < MAX_NUM_HEAPS; i++)
    {
      if (log->header_[i].free_)
	{
	  log->currHeapNum_ = i;
	  log->header_[i].free_ = FALSE;
	  if (i > maxHeapNum)
	    maxHeapNum = i;
	  assert(i > HEAP_NUM_RESERVED);
	  return i;
	}
    }
  log->currHeapNum_ = maxHeapNum = MAX_NUM_HEAPS - 1;
  log->overflow_ = TRUE;
  return MAX_NUM_HEAPS;
}

// -----------------------------------------------------------------------
// Init log after logging starts.
// -----------------------------------------------------------------------
void HeapLogRoot::initLog()
{
  if (log != NULL)
    return;
  Lng32 old = track;
  track = FALSE;
  log = new HeapLog;
  track = old;
}

// -----------------------------------------------------------------------
// Reset log.
// -----------------------------------------------------------------------
void HeapLogRoot::reset()
{
  if (log == NULL)
    return;
  NABoolean setFree = log->overflow_;
  log->currHeapNum_ = HEAP_NUM_BASE;
  log->objIndex_ = -1;
  log->disableLevel_ = 0;
  log->overflow_ = FALSE;
  log->close();

  // Find the maximun heap number that is still in-use.
  for (Lng32 i = 0; i < MAX_NUM_HEAPS ; i++)
    {
      if (log->header_[i].free_)
	continue;
      deleteLogSegment(i, (i <= HEAP_NUM_BASE ? FALSE : setFree));
      maxHeapNum = i;
    }
  if (maxHeapNum < HEAP_NUM_RESERVED)
    maxHeapNum = HEAP_NUM_RESERVED;
  if (setFree)
    maxHeapNum = HEAP_NUM_BASE;
}

// ---------------------------------------------------------------------
// Constructor and destructor.
// Each log segment has a corresponding heap.
// ---------------------------------------------------------------------
HeapLogSegment::HeapLogSegment()
  : object_(NULL), 
    slotCount_(0), usageCount_(0), 
    last_(-1), deleted_(-1),
    totalSize_(0),
    free_(TRUE)
{
  name_[0] = '\0';
}

HeapLogSegment::~HeapLogSegment()
{
  HEAPLOG_OFF();
  delete [] object_;
  HEAPLOG_ON();
  
  object_ = NULL;
  slotCount_ = 0;
  usageCount_ = 0;
  last_ = -1;
  totalSize_ = 0;
  free_ = TRUE;
}

// -----------------------------------------------------------------------
// Constructor.
// -----------------------------------------------------------------------
HeapLog::HeapLog()
  : currHeapNum_(HEAP_NUM_BASE),
    objIndex_(-1),
    disableLevel_(0),
    overflow_(FALSE),
    
    heading_(NULL),
    h_(0),
    s_(0),
    objCount_(0),
    status_(PHASE_CLOSED),
    packdata_(NULL),
    datalen_(0),
    currlen_(0)
{
  // heading text for display.
  static const char *headings[11] = {
     
  "ARKCMP Process\n==============\n\n",
  "SQLCI Process\n=============\n\n",
  "Objects not Deallocated:\n------------------------\n\n",
  "  HEAP_ID       OBJECT_INDEX   OBJECT_SIZE (BYTES)",
  "  ------------  -------------  -------------------\n",
  "Summary of Leaks:\n-----------------\n\n",
  "  HEAP_ID       LEAK_COUNT     LEAK_SIZE   (BYTES)  HEAP_NAME",
  "  ------------  -------------  -------------------  -----------------------\n",
  "\n*** Error: The total number of heaps in ARKCMP exceeds 5000 limit.",
  "\n*** Error: The total number of heaps in SQLCI exceeds 5000 limit.",
  "\nPlease reduce the number statements within a tracking session and retry."
  };

  heading_ = headings;
}

// -----------------------------------------------------------------------
// Add a log entry. 
// Fill in heapNum, object index, object size, object addr. 
// Called when a new object is allocated in a heap.
// -----------------------------------------------------------------------
Lng32 HeapLog::addEntry( void *  addr
		      , Lng32 size 
		      , Lng32 heapNum
		      , const char *heapName
		      )
{
  HeapLogSegment &seg = header_[heapNum];
  if (seg.object_ == NULL)
    {
      HEAPLOG_OFF();
      seg.object_ = new HeapLogEntry[INITIAL_OBJECT_SLOTS];
      HEAPLOG_ON();
      
      if (heapName != NULL)
	{
	  strncpy(seg.name_, heapName, 24);
	  seg.name_[24] = 0;
	}
      else
	seg.name_[0] = '\0';
      seg.slotCount_ = INITIAL_OBJECT_SLOTS;
      seg.usageCount_ = 0;
      seg.last_ = -1;
    }
  seg.free_ = FALSE;
  objIndex_++;
  seg.usageCount_++;
  seg.last_++;
  if (seg.last_ >= seg.slotCount_)
    { // resize the log segment.
      HEAPLOG_OFF();
      HeapLogEntry *pNewLog = seg.object_;
      seg.object_ = new HeapLogEntry[seg.slotCount_ * 2];
      for (Int32 i = 0; i < seg.slotCount_; i++)
	seg.object_[i] = pNewLog[i];
      delete [] pNewLog;
      HEAPLOG_ON();
      seg.slotCount_ *= 2;
    }

  seg.object_[seg.last_].indx = objIndex_;
  seg.object_[seg.last_].size = size;
  seg.object_[seg.last_].addr = addr;
  seg.totalSize_ += size;

  return objIndex_;
} 

// -----------------------------------------------------------------------
// Prepare to fetch packdata returned from arkcmp.
// -----------------------------------------------------------------------
Lng32 HeapLog::fetchInit( ULng32 flags
                       , char *packdata
		       , Lng32 datalen
		       )
{
  if (status_ != PHASE_CLOSED)
    // fetchInit has been performed.
    return 0;

  if ((flags & LeakDescribe::FLAG_SQLCI) == 0)
    HeapLogRoot::control(LOG_RESET_DISABLE);
  
  if (datalen && strlen(packdata) > 0)
    {
      packdata_ = packdata;
      datalen_ = datalen;
    }

  status_ = PHASE_1;
  return 0;
}

// -----------------------------------------------------------------------
// Reset some data upon FETCH_EOF.
// -----------------------------------------------------------------------
void HeapLog::close()
{
  packdata_ = NULL;
  datalen_ = 0;
  currlen_ = 0;

  s_ = h_ = 0;
  status_ = PHASE_CLOSED;
  objCount_ = 0;
}

// -----------------------------------------------------------------------
// Fetch log data line by line.  Each line is terminated by '\0'.
// return FETCH_EOF if eof, else 0.
// -----------------------------------------------------------------------
Lng32 HeapLog::fetchLine( char *buf
		       , Lng32 sqlci
		       )
{
  assert(status_ != PHASE_CLOSED);

  if (datalen_ > 0)
    { // Fetch log data from ARKCMP.
#pragma nowarn(1506)   // warning elimination 
      Lng32 s = strlen(packdata_) + 1;
#pragma warn(1506)  // warning elimination 
      if (currlen_ >= datalen_ ||
	  s == 1)
	{ // eof
	  *buf = '\0';
	  close();
	  status_ = PHASE_1;
	}
      else
	{
	  strncpy(buf, packdata_, s);
	  packdata_ += s;
	  currlen_ += s;
	  return 0;
	}
    }
  
  ostringstream oss;
  if (objCount_ == 0 && status_ != PHASE_EOF)
    { // No data to report.
      for (Int32 i = 0; i <= HeapLogRoot::maxHeapNum; i++)
	objCount_ += header_[i].usageCount_;
      if (objCount_ == 0)
	if (overflow_)
	  {
	    oss.str().clear();
	    oss << heading_[8+sqlci] << heading_[10] << ends;
	    strncpy(buf, oss.str().c_str(), DISPLAY_LEN*2-1);
	    buf[DISPLAY_LEN*2-1] = 0;
	    status_ = PHASE_EOF;
	    return 0;
	  }
	else
	  {
	    *buf = '\0';
	    close();
	    return FETCH_EOF;
	  }
    }

  HeapLogSegment *seg;
  NABoolean first;

  // Return log data for executor.
  switch(status_)
    {
    case PHASE_1: // Heading
      oss.str().clear();
      oss << heading_[sqlci]
	  << heading_[2]
	  << heading_[3]
	  << ends;
      strncpy(buf, oss.str().c_str(), DISPLAY_LEN*3-1);
      buf[DISPLAY_LEN*3-1] = 0;
      status_ = PHASE_2;
      return 0;
      break;

    case PHASE_2: // Data for individual heaps.
      while (h_ <= HeapLogRoot::maxHeapNum)
	{
	  seg = &header_[h_];
	 
	  if (seg->usageCount_ == 0)
	    {
	      h_++;
	      continue;
	    }

	  first = (s_ == 0);
	  while (s_ <= seg->last_)
	    {
	      if (seg->object_[s_].size > 0)
		{
	  	  oss.str().clear();
		  oss << (first ? heading_[4] : "")
		      << setw(14) << h_ 
		      << setw(15) << seg->object_[s_].indx
		      << setw(21) << seg->object_[s_].size
		      << ends;
	          strncpy(buf, oss.str().c_str(), DISPLAY_LEN*2-1);
		  buf[DISPLAY_LEN*2-1] = 0;
		  if (s_ < seg->last_)
		    s_++;
		  else
		    {
		      s_ = 0;
		      h_++;
		    }
		  return 0;
		}
	      s_++;
	    }
     
	  s_ = 0;
	  h_++;
	}
      status_ = PHASE_3;
      oss.str().clear();
      oss << heading_[4]
	  << ends;
      strncpy(buf, oss.str().c_str(), DISPLAY_LEN-1);
      buf[DISPLAY_LEN-1] = 0;
      return 0;

    case PHASE_3: // Summary of Leaks.
      oss.str().clear();
      oss << heading_[5]
	  << heading_[6]
	  << ends;
      strncpy(buf, oss.str().c_str(), DISPLAY_LEN*2-1);
      buf[DISPLAY_LEN*2-1] = 0;
      status_ = PHASE_4;
      h_ = 0;
      return 0;
    
    case PHASE_4:
      while(h_ <= HeapLogRoot::maxHeapNum)
	{ // Summary of each heap.
	  seg = &header_[h_];
	  if (seg->usageCount_ == 0)
	    {
	      h_++;
	      continue;
	    }
	  oss.str().clear();
	  oss << heading_[7]
	      << setw(14) << h_
	      << setw(15) << seg->usageCount_
	      << setw(21) << seg->totalSize_
	      << "  " 
	      << seg->name_
	      << ends;
          strncpy(buf, oss.str().c_str(), DISPLAY_LEN*2-1);
	  buf[DISPLAY_LEN*2-1] = 0;
	  h_++;
	  return 0;
	}
    case PHASE_5: 
      { // Total 
	Lng32 heapCount = 0;
	Lng32 totalSize = 0; 
	for (Lng32 i = 0; i <= HeapLogRoot::maxHeapNum; i++)
	  {
	    seg = &header_[i];
	    if (seg->usageCount_ == 0)
	      continue;
	    heapCount++;
	    totalSize += seg->totalSize_;
	  }
	oss.str().clear();
	oss << heading_[7]
	    << "  Total: "
	    << setw(5) << heapCount
	    << setw(15) << objCount_
	    << setw(21) << totalSize << "\n"
	    << (overflow_ ? heading_[8+sqlci] : "")
	    << (overflow_ ? heading_[10] : "")
	    << ends;
        strncpy(buf, oss.str().c_str(), DISPLAY_LEN*4-1);
	buf[DISPLAY_LEN*4-1] = 0;
	status_ = PHASE_EOF;
	return 0;
      }
    default: 
      close();
      return FETCH_EOF; 
    }
#pragma nowarn(203)   // warning elimination 
  return 0;
#pragma warn(203)  // warning elimination 
}  

