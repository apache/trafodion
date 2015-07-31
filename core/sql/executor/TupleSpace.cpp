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
//
// TupleSpace.cpp
//

// The TupleSpace class stores tuple data and can overflow to
// temporary storage when memory limits are reached.  A constructor
// argument configures whether overflow to temporary storage is
// enabled.  Tuple data is inserted into a TupleSpace by evaluating a
// SQL/MX contiguous move expression, so data in a TupleSpace always
// consists of a single tuple.  TupleSpace storage is implemented by a
// read cache and a SwapSpace object.  The SwapSpace object swaps
// tuple data out to temporary storage when insufficient memory is
// available.  As long as memory limits are not encountered, write
// buffers are added to the read cache as they fill with tuple data.
//
// Before a TupleSpace object has overflowed to temporary storage, the
// the readCache_ buffers combined with the write_ buffer store all of
// the TupleSpace's data in memory.  The write_ buffer is the only
// buffer that isn't guaranteed to be full.  Read cache buffers
// are always full (ignoring buffer fragmentation at the
// end of each buffer that is smaller than tupleLen_).
//
// When insufficient room is available in the write_ buffer for an
// insert() and a new buffer cannot be allocated or reused as the next
// write_ buffer, the full write_ buffer is swapped out to temporary
// storage and a "reserved" buffer is used for the write_ buffer.
// When the write operation completes, the I/O buffer is made avaiable
// for reuse, replenishing the supply of reserve buffers.  Full write_
// buffers continue to be swapped out to temporary storage until data
// is discarded.
//
// The write_ buffer is always logically the last buffer.  The logical
// buffer order is read cache buffers followed by SwapSpace buffers
// followed by the write_ buffer.  When buffers are newly allocated
// for a TupleSpace object, start_ and write_ refer to the same buffer
// and readCache_ is empty.  The write_ buffer should never be empty
// when other buffers exist.
//
// The buffer referenced by start_ is always either a readCache_
// buffer or the write_ buffer.  SwapSpace buffers are added to
// readCache_ before being referenced by start_.
//
// Any buffer, including the current SwapSpace buffer, can be
// referenced by read_.
//
#include <limits.h>
#include <BaseTypes.h>
#include <ExpAtp.h>
#include <exp_expr.h>

#include "ex_ex.h"
#include "TupleSpace.h"

namespace ExOverflow
{
  TupleSpace::TupleSpace(ByteCount tupleLen, BufferCount nBuffers, 
                         ByteCount bufferSize, UInt32 memoryQuotaMB,
                         ex_expr* copyExpr, ex_cri_desc* criDesc,
                         NAMemory* heap, bool enableOverflow,
                         ExExeStmtGlobals* stmtGlobals,
                         UInt16 scratchThresholdPct,
                         ScratchOverflowMode ovMode, bool yieldQuota)
    : currentTuple_(0), nTuples_(0), atp_(NULL), copyExpr_(copyExpr),
      memory_(bufferSize, memoryQuotaMB, nBuffers, 
              enableOverflow ? NUM_RESERVE_BUFFERS : 0, heap,
              stmtGlobals, yieldQuota),
      overflowEnabled_(enableOverflow), read_(bufferSize), readCache_(heap),
      start_(bufferSize), 
      swap_(memory_, stmtGlobals, scratchThresholdPct),
      tupleLen_(tupleLen), write_(bufferSize)
  {
    ex_assert((tupleLen_ <= bufferSize), "tuple length exceeds buffer size");
    if (overflowEnabled_)
    {
      ex_assert((nBuffers >= MIN_INITIAL_BUFFERS), "Too few initial buffers");
    }
    else
    {
      ex_assert((nBuffers >= 1), "Must have at least one initial buffer");
    }

    atp_ = allocateAtp(criDesc, heap);
    atp_->getTupp(SAVED_DUP_ATP_INDEX) = new(heap) tupp_descriptor();
    swap_.setScratchOverflowMode(ovMode);
  }

  TupleSpace::~TupleSpace(void)
  {
    resetAtp();
    atp_->release();
    atp_ = NULL;

    copyExpr_ = NULL;
  }

  IoStatus
  TupleSpace::advance(void)
  {
    IoStatus status = OK;
    bool newReadBuffer = false;

    if ((nTuples_ > 0) && (currentTuple_ < (nTuples_ - 1)))
    {
      // If a read is pending, complete the buffer fetch by setting
      // the current read position to the beginning of the swapped-in
      // buffer.
      if (swap_.isIoPending())
      {
        status = completeFetch(&newReadBuffer);
      }
    }
    else
    {
      read_.invalidate();
      status = END_OF_DATA;
    }

    if ((status == OK) && !newReadBuffer)
    {
      // Access the current read buffer.  If the current buffer is
      // exhausted, move to the next read buffer.  If the next buffer
      // is a SwapSpace buffer, initiate a fetch of this buffer
      // which will be completed by completeFetch().
      ex_assert((read_.isValid()), "advance() - invalid read_ position");
      read_.advance(tupleLen_);
      if (!canReadTuples())
      {
        status = nextReadBuffer();  // read_ invalid => fetch is pending
      }
      if ((status == OK) && read_.isValid())
      {
        ++currentTuple_;
        ex_assert((canReadTuples()),
                  "advance() failed to fetch next read buffer");
      }
    }

    return status;
  }

  IoStatus
  TupleSpace::current(atp_struct*& currentAtp)
  {
    IoStatus status = nTuples_ ? OK : END_OF_DATA;
    if ((status == OK) && swap_.isIoPending())
    {
      status = completeFetch();
    }
    if (status == OK)
    {
      if (read_.isValid())
      {
        currentAtp = getAtp(read_);
      }
      else 
      {
        currentAtp = NULL;
        status = END_OF_DATA;
      }
    }

    return status;
  }

  void
  TupleSpace::discard(void) 
  {
    reset(false);

    // TODO: Could track memory buffer demand and call memory_.adjust
    // TODO: to free up some memory if tuples did not overflow to
    // TODO: temporary storage.
  }

  Int16
  TupleSpace::getLastError(void)
  {
    return swap_.getLastError();
  }

  Int16
  TupleSpace::getLastSqlCode(void)
  {
    return swap_.getLastSqlCode();
  }

  ByteCount64
  TupleSpace::getMaxMemory(void) const
  {
    return memory_.getMaxAllocation();
  }

  ByteCount64
  TupleSpace::getMemory(void) const
  {
    return memory_.size();
  }

  UInt32
  TupleSpace::getQuotaMB(void) const
  {
    return memory_.getQuotaMB();
  }

  IoStatus 
  TupleSpace::insert(atp_struct* row, atp_struct **rtAtp)
  {
    IoStatus status = makeRoom();
    *rtAtp = NULL;
    if (status == OK) 
    {
      atp_struct* atp = getAtp(write_);
      ex_assert((atp != NULL), "Internal error - NULL atp");
      ex_expr::exp_return_type rc = copyExpr_->eval(row, atp);
      if (rc != ex_expr::EXPR_ERROR)
      {
        write_.advance(tupleLen_);
        ++nTuples_;
        *rtAtp = atp;
      }
      else
      {
        status = SQL_ERROR;
      }
    }
    return status;
  }

  void
  TupleSpace::reacquireResources(void)
  {
    memory_.reinitialize();
  }

  void
  TupleSpace::releaseResources(void)
  {
    reset(true);
  }

  IoStatus
  TupleSpace::rewind(void)
  {
    IoStatus status = OK;

    // Start of data is the beginning of either the first readCache_ 
    // buffer or the write_ buffer.
    if (nTuples_ > 0)
    {
      if (!readCache_.empty())
      {
        readCache_.position();
      }
      currentTuple_ = 0;
      read_ = start_;
      if (overflowEnabled_)
      {
        status = swap_.rewind();
      }
    }

    return status;
  }

  void TupleSpace::setIoEventHandler(ExSubtask* ioEventHandler)
  {
    if (overflowEnabled_)
    {
      swap_.setIoEventHandler(ioEventHandler);
    }
  }

//**********************************************************************
// Private methods
//**********************************************************************

  bool
  TupleSpace::cacheWriteBuffer(void)
  {

#if defined(_DEBUG)
    // Limiting the read cache to a single buffer makes it easier to
    // test merge join overflow to temporary storage.
    if (getenv("MJ_TEST_OVERFLOW") &&(readCache_.size() > 0))
    {
      return false;
    }
#endif

    bool bufferAvailable = false;

    // Write buffers are added to readCache_ only when buffers are not
    // being swapped out to temporary storage.
    if (!overflowEnabled_ || swap_.empty())
    {
      char* emptyBuffer = memory_.getBuffer();
      bufferAvailable = (emptyBuffer != NULL);
      if (bufferAvailable)
      {
        readCache_.push_back(write_.getBuffer());
        write_.set(emptyBuffer);
      }
    }

    return bufferAvailable;
  }

  IoStatus
  TupleSpace::completeFetch(bool* newReadBuffer)
  {
    bool readCompleted = false;
    IoStatus status = swap_.checkIO(&readCompleted);

    if ((status == OK) && readCompleted)
    {
      // A buffer fetch initiated by advance() or rewind() has
      // completed.  An invalid read_ reference indicates that the
      // just swapped-in buffer is the next buffer to read.
      if (!read_.isValid())
      {
        read_.set(swap_.getReadBuffer());
        ++currentTuple_;
        ex_assert((canReadTuples()),
                  "completeFetch() failed to fetch next read buffer");
      }

      // If the first active SwapSpace buffer has been fetched,
      // promote it into the read cache if there is sufficient memory
      // to do so.
      promoteSwapBuffer();
    }

    if (newReadBuffer)
    {
      *newReadBuffer = readCompleted;
    }

    return status;
  }

  atp_struct*
  TupleSpace::getAtp(const BufferReference& br)
  {
    atp_struct* atp = NULL;

    if (br.isValid())
    {
      atp_->getTupp(SAVED_DUP_ATP_INDEX).setDataPointer(br.getPosition());
      atp = atp_;
    }

    return atp;
  }

  void
  TupleSpace::getInitialBuffer(void)
  {
    setInitialBuffer(memory_.getBuffer(true));
  }

  TupleSpace::ReadBufferType
  TupleSpace::getReadBufferType(void)
  {
    ReadBufferType bt = BT_INVALID;

    if (read_.isValid())
    {
      if (isReadBuffer(readCache_.current()))
        bt = BT_CACHE_BUFFER;
      else if (isReadBuffer(swap_.getReadBuffer()))
        bt = BT_SWAP_BUFFER;
      else if (isReadBuffer(write_))
        bt = BT_WRITE_BUFFER;
    }

    return bt;
  }

  void
  TupleSpace::setInitialBuffer(char* buffer)
  {
    start_.set(buffer);
    write_.set(buffer);
    read_.set(buffer);
  }

  IoStatus
  TupleSpace::makeRoom(void)
  {
    // If insufficient space is available to insert a tuple into the
    // write_ buffer, check whether our quota allows a new buffer to
    // be allocated.  If a new buffer is permitted and a new write_
    // buffer is successfully allocated, save the old write buffer in
    // the read cache.  If a new write_ buffer can't be allocated, use
    // a reserved buffer as our new write_ buffer and overflow the old
    // write buffer to temporary storage.

    IoStatus status = OK;

    if (!write_.isValid())
    { 
      getInitialBuffer();
    }
    else if ((write_.bytesAvailable() < tupleLen_) && !cacheWriteBuffer())
    {
      if (!overflowEnabled_)
      {
        status = NO_MEMORY;
      }
      else
      {  // swap out write buffer
        status = swap_.swapOut(write_.getBuffer());
        if (status == OK)
        {
          // swap out was successfully initiated
          char* newBuffer = memory_.getBuffer(true);
          if (newBuffer)
          {
            write_.set(newBuffer);
          }
          else
          {
            write_.invalidate();
            status = NO_MEMORY;
          }
        }
        else if (status != IO_PENDING)
        {
          write_.invalidate();
        }
      }  // swap out write buffer
    }

    return status;
  }

  IoStatus
  TupleSpace::nextReadBuffer(void)
  {
    // Move the read_ buffer reference to the next buffer.  The
    // logical buffer order is: read cache buffers followed by swap
    // buffers followed by the write_ buffer.

    // Implementation Assumptions:
    // TupleSpace::rewind will be called before attempts to advance
    // the current read position in a TupleSpace object.  Pending I/O
    // operations will be completed by TupleSpace::advance.  There
    // should always be at least one read cache buffer, so the first
    // getReadBufferType call should return BT_CACHE_BUFFER.  Rewind
    // will fetch the first active swap buffer, but this doesn't guarantee
    // that swap_ will have an active buffer, since this buffer may
    // have been promoted into the read cache.

    IoStatus status = OK;

    switch(getReadBufferType())
    {
      case BT_CACHE_BUFFER:
      {
        // Get the next read cache buffer.
        char* buffer = readCache_.next();
        if (buffer)
        {
          read_.set(buffer);
        }
        else if (overflowEnabled_ && !swap_.empty())
        {
          // Get the first active swap buffer
          if (!swap_.haveFirstBuffer())
          {
            read_.invalidate();
            status = swap_.rewind();
          }
          else
          {
            read_.set(swap_.getReadBuffer());
          }
        }
        else
        {
          // Read the write_ buffer last
          read_.set(write_.getBuffer());
        }
        break;
      }

      case BT_SWAP_BUFFER:
        // Get the next SwapSpace buffer
        read_.invalidate();
        status = swap_.fetchNextBuffer();  // OK => fetching next buffer
        if (status == END_OF_DATA)
        {
          // Read the write_ buffer last
          read_.set(write_.getBuffer());
          status = OK;
        }
        break;

      case BT_WRITE_BUFFER:

        // The write_ buffer is the last buffer; there is no next buffer.
        read_.invalidate();
        status = END_OF_DATA;
        break;

      default:
        // The read_ buffer reference must be valid, since the "next"
        // read_ buffer is defined relative to the current read_ buffer.
        ex_assert(false, "nextReadBuffer - read_ is invalid");
        break;

    } // switch

    return status;
  }

  void
  TupleSpace::promoteSwapBuffer(void)
  {
    char* buffer = swap_.promoteBuffer();
    if (buffer)
    {
      readCache_.push_back(buffer);
    }
  }

  void
  TupleSpace::reset(bool allDone) 
  {
    if (nTuples_ > 0)
    {
      resetAtp();
      reuseCacheBuffers();
      nTuples_ = 0;
      currentTuple_ = 0;
    }

    if (overflowEnabled_)
    {
      swap_.discard(allDone);
    }

    if (allDone)
    {
      memory_.reuse(write_.getBuffer());
      write_.invalidate();
      read_.invalidate();
      start_.invalidate();
      memory_.deallocateAll();
    }
    else if (write_.isValid())
    {
      setInitialBuffer(write_.getBuffer());
    }
  }

  void
  TupleSpace::resetAtp(void)
  {
    atp_->getTupp(SAVED_DUP_ATP_INDEX).setDataPointer(NULL);
  }

  void
  TupleSpace::reuseCacheBuffers(void)
  {
    for (BufferCount i = readCache_.size(); i > 0; --i)
    {
      memory_.reuse(readCache_.pop());
    }
  }

}
