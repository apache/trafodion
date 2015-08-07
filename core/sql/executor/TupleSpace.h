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

#ifndef TUPLESPACE_H
#define TUPLESPACE_H

//
// TupleSpace.h
//
// A TupleSpace stores tuple data, overflowing to temporary storage
// that occurs when memory limits are encountered.  Data is written
// at the end of the TupleSpace and is read sequentially starting at
// the current read position.  Tuple data can be read more than once
// by rewinding the current position to the beginning of the data.
//

class ex_cri_desc;
class ExSubtask;

#include "NABasicObject.h"
#include "ExOverflow.h"
#include "Allocator.h"
#include "BufferList.h"
#include "BufferReference.h"
#include "SwapSpace.h"
#include "CommonStructs.h"

namespace ExOverflow
{
  class TupleSpace : public NABasicObject
  {
    public:
  
      // TupleSpace constructor.  If overflow to disk is enabled, the
      // nBuffers value must be at least MIN_INITIAL_BUFFERS; otherwise
      // nBuffers must be at least one.
      TupleSpace(ByteCount tupleLen, BufferCount nBuffers, 
                 ByteCount bufferSize, UInt32 memoryQuotaMB, 
                 ex_expr* copyExpr, ex_cri_desc* criDesc, 
                 NAMemory* heap, bool enableOverflow,
                 ExExeStmtGlobals* stmtGlobals,
                 UInt16 scratchThresholdPct,
                 ScratchOverflowMode ovMode,bool yieldQuota);
      ~TupleSpace(void);

      // Default TupleSpace configuration is to initially allocate
      // four 56 KB buffers, then allocate more buffers as needed
      // subject to memory constraints.  Two of these buffers are
      // reserved for use as I/O buffers; the third buffer is used as
      // the write_ buffer, and the fourth buffer is used as a
      // readCache_ buffer.  Some regression tests use a smaller
      // 2048-byte buffer to make it easier to test merge join
      // overflow to temporary storage.  (NSK unbuffered I/O must be a
      // multiple of 2 KB).
#if defined(NA_HAS_ANSI_CONST)
      static const ByteCount OVERFLOW_TEST_BUFFER_SIZE = 2048;
      static const ByteCount OVERFLOW_BUFFER_SIZE = 56 * 1024 * 4;
      static const BufferCount NUM_RESERVE_BUFFERS = 2;
      static const BufferCount MIN_INITIAL_BUFFERS = 4;
#else
      enum { OVERFLOW_TEST_BUFFER_SIZE = 2048 };
      enum { OVERFLOW_BUFFER_SIZE = 56 * 1024 * 4 };
      enum { NUM_RESERVE_BUFFERS = 2 };
      enum { MIN_INITIAL_BUFFERS = 4 };
#endif

      // Advance the current tuple position to the next tuple
      IoStatus advance(void);

      // Set currentAtp to the current tuple position
      IoStatus current(atp_struct*& currentAtp);

      // Discard all tuple data
      void discard(void);

      // Is TupleSpace empty?
      bool empty(void);

      // Return the maximum number of bytes allocated since
      // the last reacquireResources call.
      ByteCount64 getMaxMemory(void) const;

      // Return the number of bytes currently allocated
      ByteCount64 getMemory(void) const;

      // Return the last system (OS or FS) error
      Int16 getLastError(void);

      // Return the last SQLCODE (temporary storage I/O error)
      Int16 getLastSqlCode(void);

      // Return the memory manager quota in megabytes
      UInt32 getQuotaMB(void) const;

      // Insert tuple data into the write_ buffer
      IoStatus insert(atp_struct* row, atp_struct **rtApt);

      // Reset the current tuple position to the first tuple
      IoStatus rewind(void);

      // Reacquire TupleSpace resources (undo releaseResources).
      // Failure to reacquire resources is fatal.
      void reacquireResources(void);

      // Release resources for use by other SQL/MX operators
      void releaseResources(void);

      // Set NSK-only ScratchFileConnection I/O event handler task
      void setIoEventHandler(ExSubtask* ioEventHandler);

    private:

#if defined(NA_HAS_ANSI_CONST)
      // ATP index for contiguous saved duplicate right row tupp
      static const Int16 SAVED_DUP_ATP_INDEX = 2;
#else
      enum { SAVED_DUP_ATP_INDEX = 2 };
#endif

     // Type of buffer read_ references:
      enum ReadBufferType { BT_INVALID, BT_CACHE_BUFFER, BT_SWAP_BUFFER,
                            BT_WRITE_BUFFER };


      // Attempt to save a full write buffer in the read cache.
      bool cacheWriteBuffer(void);

      IoStatus completeFetch(bool* newReadBuffer = NULL);

      atp_struct* getAtp(const BufferReference& br);

      // Set up initial write_ buffer.  Failure is fatal.
      void getInitialBuffer(void);

      // Return the type of buffer read_ references
      ReadBufferType getReadBufferType(void);

      // Does read_ buffer have unread tuple data?
      bool canReadTuples(void);

      bool isReadBuffer(char* buffer);

      bool isReadBuffer(const BufferReference &br);

      bool isWriteBuffer(char* buffer);

      bool isWriteBuffer(const BufferReference &br);

      // Ensure the write_ buffer has space to store a tuple.
      IoStatus makeRoom(void);

      IoStatus nextReadBuffer(void);

      void promoteSwapBuffer(void);

      void reset(bool allDone);

      void resetAtp(void);

      void reuseCacheBuffers(void);

      void setInitialBuffer(char* buffer);

    private:
      UInt64 currentTuple_;          // current tuple number (0..N-1)
      UInt64 nTuples_;               // number of tuples in TupleSpace
      atp_struct* atp_;              // ATP for accessing tuple data
      ex_expr* copyExpr_;            // cached copyExpr()
      Allocator memory_;             // memory allocator
      bool overflowEnabled_;         // is overflow enabled?
      BufferReference read_;         // read position
      BufferList readCache_;         // read buffer cache
      BufferReference start_;        // start of data
      SwapSpace swap_;               // swapped out buffer manager
      ByteCount tupleLen_;           // bytes per tuple
      BufferReference write_;        // write position
  };

  inline
  bool
  TupleSpace::empty(void)
  {
    return (nTuples_ == 0);
  }

  inline
  bool
  TupleSpace::canReadTuples(void)
  {
    return (read_.bytesAvailable() >= tupleLen_);
  }

  inline
  bool
  TupleSpace::isReadBuffer(char* buffer)
  {
    return BufferReference::sameBuffer(read_, buffer);
  }

  inline
  bool
  TupleSpace::isReadBuffer(const BufferReference& br)
  {
    return BufferReference::sameBuffer(read_, br);
  }

  inline
  bool
  TupleSpace::isWriteBuffer(char* buffer)
  {
    return BufferReference::sameBuffer(write_, buffer);
  }

  inline
  bool
  TupleSpace::isWriteBuffer(const BufferReference& br)
  {
    return BufferReference::sameBuffer(write_, br);
  }

}
#endif
