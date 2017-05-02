//**********************************************************************
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

// SwapSpace.cpp
//
// SwapSpace has these invariants:
//   numSwap_ = (swapEnd_ - swapStart_)/swapBufferSize_
//   swapStart_ <= swapRead_ <= swapEnd_ (when swapRead_ is valid)
//
// This class defines "const-ness" in terms of whether a method
// modifies a member variable.  Data buffers pointed to by member
// variables are not considered part of a SwapSpace object's state.
//
// The ScratchSpace is referenced by a pointer to enable lazy initialization.
// Initialized objects have a non-null store_ pointer.
//

#include "BaseTypes.h"
#include "Const.h"
#include "ex_ex.h"
#include "ScratchSpace.h"
#include "SortError.h"
#include "SwapSpace.h"

namespace ExOverflow
{
  SwapSpace::SwapSpace(Allocator& memory, 
                       const ExExeStmtGlobals* stmtGlobals,
                       UInt16 scratchThresholdPct)
    : atEOF_(true), ioEventHandler_(NULL), ioPending_(NONE),
      memory_(memory), numSwap_(0), readBuffer_(NULL), 
      readingFirstBuffer_(false), scratchThresholdPct_(scratchThresholdPct),
      stmtGlobals_(stmtGlobals),
      store_(NULL), swapBufferSize_(0), swapOutBuffer_(NULL),
      swapEnd_(0), swapRead_(0), swapStart_(0), ovMode_(SCRATCH_DISK)
  {
    swapBufferSize_ = memory_.getDefaultBufferSize();
  }

  SwapSpace::~SwapSpace(void)
  {
    // Pending I/O operations are cancelled when the ScratchFileMap
    // destructor calls FILE_CLOSE_.
    delete store_;
    store_ = NULL;

    memory_.deallocate(readBuffer_);
    memory_.deallocate(swapOutBuffer_);
  }

  void
  SwapSpace::discard(bool allDone) 
  {
    if (numSwap_ > 0)
    {
      reset(allDone);
    }
  }

  IoStatus
  SwapSpace::fetchNextBuffer(void)
  {
    IoStatus status = (numSwap_ > 0) ? checkIO() : END_OF_DATA;
    if ((status == OK) && (swapStart_ != swapEnd_))
    {
      if (swapRead_ != swapEnd_)
      {
        swapRead_ += swapBufferSize_;
      }

      if (swapRead_ < swapEnd_)
      {
        status = fetch();    // status == OK if read initiated
      }
      else
      {
        atEOF_ = true;
        status = END_OF_DATA;
      }
    }

    return status;
  }

  Int16
  SwapSpace::getLastError(void)
  {
    Int16 error = 0;

    if (sortError_.getSortError())
    {
#if defined(NA_HAS_ANSI_CPP_CASTS)
      error = static_cast<Int16>(sortError_.getSysError());
#else
      error = (Int16) sortError_.getSysError();
#endif
    }

    return error;
  }

  Int16
  SwapSpace::getLastSqlCode(void)
  {
    // NSK compiler emits an erroneous type conversion warning
    // when this routine is coded as a return statement with 
    // a conditional expression.
    if (store_)
    {
      return store_->getLastSqlCode();
    }
    else
    {
      return 0;
    }   
  }

  char*
  SwapSpace::promoteBuffer(void)
  {
    char* adoptee = NULL;

    if ((numSwap_ > 0) && haveFirstBuffer())
    {
      adoptee = readBuffer_;
      if (--numSwap_ == 0)
      {
        readBuffer_ = NULL;    // caller owns promoted buffer
        reset();
      }
      else
      {
        readBuffer_ = memory_.getBuffer();
        if (!readBuffer_)
        {
          // Insufficient memory for buffer promotion, restore
          // original object state.
          ++numSwap_;
          readBuffer_ = adoptee;
          adoptee = NULL;
        }
        else
        {
          swapStart_ += swapBufferSize_;
          swapRead_ = swapStart_;
          readingFirstBuffer_ = false;
        }
      }
    }

    return adoptee;
  }

  IoStatus
  SwapSpace::rewind(void)
  {
    IoStatus status = OK;

    if ((numSwap_ > 0) || isIoPending())
    {
      status = checkIO();
      if (status == OK)
      {
        atEOF_ = false;
        if (!haveFirstBuffer())
        {
          swapRead_ = swapStart_;
          status = fetch();    // status == OK if read initiated
        }
      }
    }

    return status;
  }

  IoStatus
  SwapSpace::swapOut(char* buffer)
  {
    IoStatus status = OK;

    if (!store_)
    {
      init();    // failure is fatal
    }
    else
    {
      status = checkIO();
    }

    if (status == OK)
    {
      swapOutBuffer_ = buffer;
      // Initiate overflow of buffer to temporary storage
      ioPending_ = WRITE;
      DWORD ignoredBlockNum = 0;
      status = mapStatus(store_->writeThru(swapOutBuffer_, swapBufferSize_,
                                           ignoredBlockNum));
      // status is OK if write was successfully initiated
    }

    return status;
  }

  void
  SwapSpace::setIoEventHandler(ExSubtask* ioEventHandler)
  {
    ioEventHandler_ = ioEventHandler;
  }

  // **********************************************************************
  // * Private methods
  // **********************************************************************

  IoStatus
  SwapSpace::checkIOPending(bool* readCompleted)
  {
    IoStatus status = OK;

    ex_assert((ioPending_ != NONE),
              "checkIOPending not called via checkIO wrapper");
    ex_assert((store_), "temporary storage uninitialized");

    status = mapStatus(store_->checkIO());
    if (status == OK)
    {
      // A read or a write operation has completed.
      if (ioPending_ == READ) 
      {
        readingFirstBuffer_ = ((swapRead_ == swapStart_)
                                && (swapRead_ != swapEnd_));
        if (readCompleted)
        {
          *readCompleted = true;
        }
      }
      else
      {
        // A SwapSpace buffer has been written to temporary storage.
        // The swapOutBuffer_ is available for reuse.
        memory_.reuse(swapOutBuffer_);
        swapOutBuffer_ = NULL;
        ++numSwap_;
        swapEnd_ += swapBufferSize_;
      }
      ioPending_ = NONE;
    }

    return status;
  }

  // Read swapped-out tuple data into readBuffer_
  IoStatus
  SwapSpace::fetch(void)
  {
    IoStatus status = OK;

    ex_assert((ioPending_ == NONE), "fetch() invoked while I/O pending");
    ex_assert((store_), "fetch() - temporary storage uninitialized");
    ex_assert((numSwap_ > 0), "fetch() - no SwapSpace buffers to fetch");
    ex_assert((swapRead_ < swapEnd_), "fetch() - can't fetch past end of file");

    if (!readBuffer_)
    {
      readBuffer_ = memory_.getBuffer(true);
      ex_assert((readBuffer_), "fetch() - failed to get read buffer");
    }
    ioPending_ = READ;
    readingFirstBuffer_ = false;

    // ScratchSapce blockNums are 1..N
#if defined(NA_HAS_ANSI_CPP_CASTS)
    Int32 blockNum = static_cast<Int32>(swapRead_ / swapBufferSize_) + 1;
#else
    Int32 blockNum = ((Int32) (swapRead_ / swapBufferSize_)) + 1;
#endif
    status = mapStatus(store_->readThru(readBuffer_, blockNum, 
                                        swapBufferSize_));
    // status is OK if read was successfully initiated

    return status;
  }

  void
  SwapSpace::init(void)
  {
    ex_assert((!store_), "temporary storage already initialized");

    readBuffer_ = memory_.getBuffer(true);
#if defined(NA_HAS_ANSI_CPP_CASTS)
    Lng32 bufSize = static_cast<Lng32>(swapBufferSize_);
#else
    Lng32 bufSize = (Lng32) swapBufferSize_;
#endif
    NAMemory* heap = memory_.getHeap();
    store_ = new(heap) ScratchSpace(heap, &sortError_,bufSize, 1, -1, //explain node
                                    false);   // logInfoEvent = false
    store_->configure(stmtGlobals_, ioEventHandler_, scratchThresholdPct_);
    store_->setScratchOverflowMode(ovMode_);
  }

  IoStatus
  SwapSpace::mapStatus(RESULT scratchStatus) const
  {
    IoStatus status = OK;

    switch (scratchStatus)
    {
      case SCRATCH_SUCCESS:
      case IO_COMPLETE:
        // status = OK;
        break;

      case SCRATCH_FAILURE:
        status = IO_ERROR;
        break;

      case IO_NOT_COMPLETE:
        status = IO_PENDING;
        break;

      case WRITE_EOF:
      case READ_EOF:
        status = END_OF_DATA;
        break;

      default:
        status = INTERNAL_ERROR;
        break;
    }

    return status;
  }
  void
  SwapSpace::reset(bool allDone) 
  {
    if (allDone)
    {
      store_->close();
    }
    else
    {
      store_->truncate();
    }

    if (readBuffer_)
    {
      memory_.reuse(readBuffer_);
      readBuffer_ = NULL;
    }
    if (swapOutBuffer_)
    {
      memory_.reuse(swapOutBuffer_);
      swapOutBuffer_ = NULL;
    }
    ioPending_ = NONE;
    atEOF_ = true;
    readingFirstBuffer_ = false;
    numSwap_ = 0;
    swapEnd_ = 0;
    swapRead_ = 0;
    swapStart_ = 0;
  }

}
