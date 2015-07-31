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
//
// Allocator.cpp
//
//

#include "ex_ex.h"
#include "ex_god.h"
#include "ex_exe_stmt_globals.h"
#include "Allocator.h"

namespace ExOverflow
{
  Allocator::Allocator(ByteCount bufferSize, UInt32 memoryQuotaMB,
                       BufferCount nBuffers, BufferCount reserve, 
                       NAMemory* heap, ExExeStmtGlobals* exeGlobals,
                       bool yieldQuota)
    : 
      maxAllocated_(0), memAllocated_(0), quota_(0), exeGlobals_(exeGlobals),
      heap_(heap), bufferSize_(bufferSize), nInitial_(nBuffers),
      nReserve_(reserve), reuseList_(heap), yieldQuota_(yieldQuota)
  {
    quotaInitial_ = memoryQuotaMB * ONE_MEGABYTE;

    if (quotaInitial_)
    {
      ex_assert(((bufferSize_ * nInitial_) <= quotaInitial_),
                "inconsistent constructor arguments");
    }
    ex_assert((nInitial_ >= nReserve_), "reserve > initial buffer count");

    acquireMemory();
  }

  Allocator::~Allocator(void)
  {
    deallocateAll();
  }

  void
  Allocator::adjust(BufferCount bufferDemand)
  {
    bufferDemand += nReserve_;
    BufferCount count = reuseList_.size();

    if (count > bufferDemand)
    {
      destroy(count - bufferDemand);
    }
  }

  char*
  Allocator::allocate(bool failureIsFatal)
  {
    // Return NULL if Allocator memory has been released
    if (!memAllocated_ && (reuseList_.size() == 0))
    {
      return NULL;
    }
    else
    {
     return allocateBuffer(failureIsFatal);
    }
  }

  void
  Allocator::deallocate(char*& buffer)
  {
    if (buffer)
    {
      NADELETEBASIC(buffer, heap_);
      buffer = NULL;
      memAllocated_ -= bufferSize_;
    }
  }

  void
  Allocator::deallocateAll(void)
  {
    if (exeGlobals_)
    {
      // Yield entire quota if yieldQuota_ is true; otherwise yield just the
      // excess quota acquired via grabMemoryQuotaIfAvailable().
      ByteCount64 yieldBytes = quota_;
      if (!yieldQuota_)
      {
        yieldBytes = (quota_ > quotaInitial_) ? (quota_ - quotaInitial_) : 0;
        quota_ -= yieldBytes;
      }
#if defined(HAS_ANSI_CPP_CASTS)
      ByteCount yieldMB
        = static_cast<ByteCount>(yieldBytes / ONE_MEGABYTE);
#else
      ByteCount yieldMB
        = (ByteCount) (yieldBytes / ONE_MEGABYTE);
#endif
      if (yieldMB)
      {
        exeGlobals_->yieldMemoryQuota(yieldMB);
      }
    }

    destroy(reuseList_.size());
    ex_assert((reuseList_.size() == 0), "failed to empty reuseList_");
  }

  char*
  Allocator::getBuffer(bool useReserve)
  {
    char* buffer = NULL;

    if (useReserve || (reuseList_.size() > nReserve_))
    {
      buffer = reuseList_.pop();
      ex_assert((buffer), "reuse list should never be empty");
    }
    else
    {
      buffer = allocate();
    }

    return buffer;
  }

  void
  Allocator::reinitialize(void)
  {
    if (!memAllocated_)
    {
      maxAllocated_ = 0;
      acquireMemory();
    }
  }

  // **********************************************************************
  // * Private methods
  // **********************************************************************

  void
  Allocator::acquireMemory(void)
  {
    quota_ = quotaInitial_;
    for (BufferCount i = 0; i < nInitial_; ++i)
    {
      reuseList_.push_back(allocateBuffer(true));
    }
  }

  char*
  Allocator::allocateBuffer(bool failureIsFatal)
  {
    char* buffer = NULL;
    ByteCount64 totalBytes = memAllocated_ + bufferSize_;

    // If the maximum quota has been reached, check whether the quota
    // can be increased.  Round the quota increase up to an even number
    // of megabytes, since the BMO quota is tracked in megabytes.
    if (exeGlobals_ && quota_ && (totalBytes > quota_))
    {
#if defined(HAS_ANSI_CPP_CASTS)
      ByteCount quotaIncrease = static_cast<ByteCount>(totalBytes - quota_);
#else
      ByteCount quotaIncrease = (ByteCount) (totalBytes - quota_);
#endif
      UInt32 quotaIncreaseMB = (quotaIncrease + ONE_MEGABYTE - 1)/ ONE_MEGABYTE;
      if (exeGlobals_->grabMemoryQuotaIfAvailable(quotaIncreaseMB))
      {
        quota_ += (quotaIncreaseMB * ONE_MEGABYTE);
      }
    }

    if (!quota_ || (totalBytes <= quota_))
    {
      buffer = (char *) ((NAHeap *)heap_)->allocateAlignedHeapMemory(bufferSize_, 512,  failureIsFatal);
      if (buffer)
      {
        memAllocated_ = totalBytes;
        if (maxAllocated_ < memAllocated_)
        {
          maxAllocated_ = memAllocated_;
        }
      }
    }

    return buffer;
  }

  void Allocator::destroy(BufferCount count)
  {
    ex_assert((count <= reuseList_.size()), "invalid argument value");
    char* buffer = NULL;
    for (BufferCount i = 0; i < count; ++i)
    {
      buffer = reuseList_.pop();
      ex_assert((buffer), "failed to unlink reuseList_ buffer");
      deallocate(buffer);
    }
  }

}
