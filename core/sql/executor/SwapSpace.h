// **********************************************************************
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
// **********************************************************************

// SwapSpace.h
//
// SwapSpace swaps data buffers to/from temporary storage.  It shares
// buffer management with its clients and encapsulates
// ScratchSpace::writeThru/readThru details.
//
#ifndef SWAPSPACE_H
#define SWAPSPACE_H

#include <limits.h>

#include <NABasicObject.h>
#include <SortError.h>
#include "Const.h"
#include "ExOverflow.h"
#include "Allocator.h"
#include "CommonStructs.h"

class ScratchSpace;
class ExSubtask;
class ExExeStmtGlobals;

namespace ExOverflow
{
  class SwapSpace : public NABasicObject
  {
    public:
      SwapSpace(Allocator& memory, const ExExeStmtGlobals* stmtGlobals,
                UInt16 scratchThresholdPct);
      ~SwapSpace(void);

      // Poll pending I/O operation
      IoStatus checkIO(bool* readCompleted = NULL);

      // Discard all data
      void discard(bool allDone);

      // Is SwapSpace empty?
      bool empty(void) const;

      // Fetch the next buffer.
      IoStatus fetchNextBuffer(void);

      // Return the last OS error or file system error
      Int16 getLastError(void);

      // Return the SQLCODE associated with the last error
      Int16 getLastSqlCode(void);

      // Get a pointer to the read buffer
      char* getReadBuffer(void) const;

      // True => read buffer is the first active buffer.
      bool haveFirstBuffer(void) const;

      // Is a SwapSpace I/O operation pending?
      bool isIoPending(void);

      // Promote the first buffer out of SwapSpace
      char* promoteBuffer(void);

      // Reposition read buffer offset to first active buffer
      IoStatus rewind(void);

      // Set NSK-only ScratchFileConnection I/O event handler
      void setIoEventHandler(ExSubtask* ioEventHandler);

      // Swap buffer out to temporary storage.
      IoStatus swapOut(char* buffer);

      void setScratchOverflowMode(ScratchOverflowMode ovMode)
          { ovMode_ = ovMode;}
      ScratchOverflowMode getScratchOverflowMode(void)
          { return ovMode_;}

    private:

      SwapSpace(void);                             // unimplemented
      SwapSpace(const SwapSpace& src);             // unimplemented
      SwapSpace& operator=(const SwapSpace& rhs);  // unimplemented

      enum  IoPending { NONE, READ, WRITE };

      // Poll to check whether a pending I/O operation has completed
      IoStatus checkIOPending(bool* readCompleted = NULL);

      // Read data from temporary storage into read buffer
      IoStatus fetch(void);

      // Initialize SwapSpace storage.
      void init(void);

      // Map ScratchSpace status to IoStatus
      IoStatus mapStatus(RESULT scratchStatus) const;

      // Reset SwapSpace state.  Close store_ files if closeFiles is true;
      // otherwise truncate the store_ files.
      void reset(bool allDone = false);

      bool atEOF_;                // No more swap buffers to read?
      ExSubtask* ioEventHandler_; // NSK-only I/O event handler task
      IoPending ioPending_;       // pending I/O state
      Allocator& memory_;         // memory allocator (not owned by this class)
      UInt32 numSwap_;            // number of swapped-out buffers in store_
      char* readBuffer_;          // swapped in data buffer
      bool readingFirstBuffer_;   // readBuffer_ is first active buffer?
      UInt16 scratchThresholdPct_;    // store_ attribute
      SortError sortError_;       // last store_ error
      const ExExeStmtGlobals* stmtGlobals_;    // for store_ attributes
      ScratchSpace* store_;       // temporary storage for swapped-out buffers
      ByteCount swapBufferSize_;  // swap buffer size in bytes
      char* swapOutBuffer_;       // swap out (pending write) buffer
      SpaceOffset swapEnd_;       // store_ offset for swapOutBuffer_
      SpaceOffset swapRead_;      // store_ offset for readBuffer_
      SpaceOffset swapStart_;     // store_ offset of first swapped-out buffer
      ScratchOverflowMode ovMode_; //overflow mode, SSD, HDD or HYBRID
  };

  inline
  bool
  SwapSpace::isIoPending(void)
  {
    return (ioPending_ != NONE);
  }

  // Test ioPending_ in-line so that the ioPending_ == NONE case is efficient
  inline
  IoStatus 
  SwapSpace::checkIO(bool* readCompleted)
  {
    if (readCompleted)
    {
      *readCompleted = false;
    }
    return (ioPending_ == NONE) ? OK : checkIOPending(readCompleted);
  }

  inline
  bool
  SwapSpace::empty(void) const
  {
    return ((numSwap_ == 0) && (ioPending_ == NONE));
  }

  inline
  char* 
  SwapSpace::getReadBuffer(void) const
  {
    return atEOF_ ? NULL : readBuffer_;
  }

  inline
  bool
  SwapSpace::haveFirstBuffer(void) const
  {
    return readingFirstBuffer_;
#if 0
    return ((numSwap_ > 0) && (swapRead_ == swapStart_)
            && (swapStart_ != swapEnd_));
#endif
  }
}

#endif
