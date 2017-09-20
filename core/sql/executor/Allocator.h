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

// Allocator.h
//
// ExOverflow::Allocator manages memory for the ExOverflow module.
// Each allocate() invocation allocates bufferSize_ bytes of memory.
//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <NABasicObject.h>
#include "ExOverflow.h"
#include "BufferList.h"

class ExExeStmtGlobals;

namespace ExOverflow
{

  class Allocator : public NABasicObject
  {
    public:
      Allocator(ByteCount bufferSize, UInt32 memoryQuotaMB,
                BufferCount nBuffers, BufferCount reserve, 
                NAMemory* heap = NULL, ExExeStmtGlobals* exeGlobals = NULL,
                bool yieldQuota = true);
      ~Allocator(void);

      // Adjust buffer reuse list based on forecasted demand
      void adjust(BufferCount bufferDemand);

      // Allocate a buffer if the allocation doesn't exceed the memory quota.
      char* allocate(bool failureIsFatal = false);
      
      // Deallocate a buffer
      void deallocate(char*& buffer);

      // Deallocate all buffers
      void deallocateAll(void);

      // Get a buffer from the reuse list or allocate one.  The number
      // of available buffers is not allowed to fall below the reserve
      // count unless useReserve is true.  Returns NULL if useReserve
      // is false and the number of available buffers is less than or
      // equal to the reserve count.
      char *getBuffer(bool useReserve = false);

      // Get default buffer size in bytes
      ByteCount getDefaultBufferSize(void) const;

      // Access the heap
      NAMemory* getHeap(void) const;

      // Return the maximum number of bytes ever allocated
      ByteCount64 getMaxAllocation(void) const;

      // Return the memory quota (in megabytes)
      UInt32 getQuotaMB(void) const;

      // Reinitialize undoes deallocateAll.  Failure is fatal.
      void reinitialize(void);

      // Make a buffer available for reuse
      void reuse(char* buffer);

      // Return the number of bytes currently allocated
      ByteCount64 size(void) const;

    private:
      Allocator(void);                               // unimplemented
      Allocator(const Allocator& src);               // unimplemented
      Allocator& operator=(const Allocator& rhs);    // unimplemented

      // Allocate nInitial_ memory buffers.  Failure to allocate
      // these buffers is fatal.
      void acquireMemory(void);

      // Allocate a buffer if the allocation doesn't exceed the memory quota.
      char* allocateBuffer(bool failureIsFatal);

      // Destroy count buffers
      void destroy(BufferCount count);

      ByteCount64 maxAllocated_;      // high water mark for allocated memory
      ByteCount64 memAllocated_;      // total memory currently allocated
      ByteCount64 quota_;             // current memory quota (no quota if zero)
      ByteCount64 quotaInitial_;      // initial memory quota (no quota if zero)
      ExExeStmtGlobals* exeGlobals_;  // globals containing BMO memory quota
      NAMemory*   heap_;              // NULL => use global new
      ByteCount   bufferSize_;        // buffer allocation size
      BufferCount nInitial_;          // number of initial buffers
      BufferCount nReserve_;          // number of buffers to keep in reserve
      BufferList  reuseList_;         // buffers available for reuse
      bool        yieldQuota_;        // true => yield entire quota;
                                      // false => yield excess quota
  };

  inline
  ByteCount
  Allocator::getDefaultBufferSize(void) const
  {
    return bufferSize_;
  }

  inline
  NAMemory*   
  Allocator::getHeap(void) const
  {
    return heap_;
  }
  inline
  ByteCount64
  Allocator::getMaxAllocation(void) const
  {
    return maxAllocated_;
  }


  inline
  UInt32
  Allocator::getQuotaMB(void) const
  {
#if defined(HAS_ANSI_CPP_CASTS)
    return static_cast<UInt32>(quota_ / ONE_MEGABYTE);
#else
    return (UInt32) (quota_ / ONE_MEGABYTE);
#endif
  }
  inline
  void
  Allocator::reuse(char* buffer)
  {
    if (buffer)
    {
      reuseList_.push_back(buffer);
    }
  }
  inline
  ByteCount64
  Allocator::size(void) const
  {
    return memAllocated_;
  }

}

#endif
