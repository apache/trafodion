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
******************************************************************************
*
* File:         ExSimpleSQLBuffer.cpp
* RCS:          $Id
* Description:  ExSimpleSQLBuffer class implementation
* Created:      6/1/97
* Modified:     $Author
* Language:     C++
* Status:       $State
*
*
*
*
******************************************************************************
*/

// Implementation of the ExSimpleSQLBuffer class and the private helper class
// ExSimpleSQLBufferEntry.  The ExSimpleSQLBuffer class is used to manage
// free and referenced tuples. It is most commonly used as a tuple allocator
// in TCB work procedures.
//

// Includes
//
#include "ExSimpleSqlBuffer.h"

// ExSimpleSQLBuffer::ExSimpleSQLBuffer - constructor
//
// Initializes a simple SQL buffer for managing tuples.
//
// IN     : numTuples - the number of tuples to allocate in the buffer
// IN     : tuppSize - the size of each tuple in bytes
// IN     : heap - memory allocator
// OUT    :
// RETURN : 
// EFFECTS: allocates space for <numTuples> tuples of <tuppSize> bytes
//          and initializes the free list to point to every tuple.
//
// The memory layout of the buffer looks something like this after
// initialization. Note that an ExSimpleSQLBufferEntry has a char[1] field 
// as the last member to hold the TuppData. Obviously, the tupp data will 
// usually be much bigger than 1 byte, so when an ExSimpleSQLBufferEntry 
// is allocated, <tuppSize> less one additional bytes are allocated to make 
// room for the tupp data. This is a common C _hack_ to save a pointer 
// and pointer dereference.
//
//         13 bytes              tuppSize-1 bytes  padding 
//    +-------------------------+----------------+---------+
//    | ExSimpleSQLBufferEntry ...   TupleData   |         |
//    | ExSimpleSQLBufferEntry ...   TupleData   |         |
//    | ExSimpleSQLBufferEntry ...   TupleData   |         |
//    +-------------------------+----------------+---------+
//
ExSimpleSQLBuffer::ExSimpleSQLBuffer(Int32 numberTuples, Int32 tupleSize, 
				     CollHeap *heap) :
  numberTuples_(numberTuples), tupleSize_(tupleSize), 
  allocationSize_(0), freeList_(0), usedList_(0), data_(0) 
{
  init(heap);
};

// ExSimpleSQLBuffer::ExSimpleSQLBuffer - constructor
//
// Initializes a simple SQL buffer for managing tuples. This constructor is
// specialized for mapping sql_buffer constructor call to an equivalent
// ExSimpleSQLBuffer call.
//
// IN     : numBuffers - the number of sql_buffer buffers
// IN     : bufferSize - the size of each buffer in bytes
// IN     : tupleSize - the size of a tuple in bytes
// IN     : heap - memory allocator
// OUT    :
// RETURN : 
// EFFECTS: calls main ExSimpleSQLBuffer constructor
//
ExSimpleSQLBuffer::ExSimpleSQLBuffer(Int32 numBuffers, Int32 bufferSize, 
				     Int32 tupleSize, CollHeap *heap)
  : numberTuples_(0), tupleSize_(tupleSize), 
    allocationSize_(0), freeList_(0), usedList_(0), data_(0) 
{
  // Compute the number of tuples that would fit in an eqivalent
  // sql buffer pool.
  //
  UInt32 numTuples
    = (numBuffers * bufferSize)/(sizeof(tupp_descriptor) + tupleSize);

  // If space was asked for try to allocate at least one tuple.
  //
  if ((numBuffers > 0) && (bufferSize > 0) && (numTuples == 0))
   {
      numTuples = 1;
   }
  numberTuples_ = (Int32) numTuples;

  init(heap);
}

// ExSimpleSQLBuffer::~ExSimpleSQLBuffer - destructor
//
// Cleans up a simple SQL buffer. Nothing to do because memory cleanup
// is handle by the heap.
//
// IN     :
// OUT    :
// RETURN : 
// EFFECTS: none, at the moment.
//
ExSimpleSQLBuffer::~ExSimpleSQLBuffer()
{
};

// ExSimpleSQLBuffer::getFreeTuple
//
// Attempts to get and return a free tuple from the buffer.
//
// IN/OUT : tupp - a reference to the tupp that will reference the new tuple.
// RETURN : 0 - success, nonzero - failed
// EFFECTS: A free tuple is moved from the free list to the used list
//          and <tupp> is set to reference the new tuple.
//
Int32 ExSimpleSQLBuffer::getFreeTuple(tupp &tupp) 
{
  Int32 rc = 1;

  ExSimpleSQLBufferEntry *entry = getFreeEntry();
  if (entry != 0)
  {
    entry->setNext(usedList_);
    usedList_ = entry;
    tupp = &entry->tuppDesc_;
    rc = 0;
  }

  return rc;
};

// Protected methods

ExSimpleSQLBufferEntry*
ExSimpleSQLBuffer::getFreeEntry(void)
{
  if (!freeList_) 
    {
      reclaimTuples();
    }

  ExSimpleSQLBufferEntry *entry = freeList_;
  if (entry)
  {
    freeList_ = freeList_->getNext();
    entry->setNext(NULL);    // unlink from free list
  }

  return entry;
}

// Private methods

void 
ExSimpleSQLBuffer::init(CollHeap *heap) 
{
  // DP2Oper ends up calling this function for zero tupps. In this case
  // no calls are every made to allocate a tuple. Every such call would
  // fail in this case.
  // 
  if (numberTuples_ == 0) return;

  // Each entry in the buffer has a header (ExSimpleSQLBufferEntry) + data.
  // The first byte of data is stored in ExSimpleSQLBufferEntry::data_[0],
  // so subtract one from tupleSize_ when calculating the allocation size.
  // We need to round this size up to nearest 8 bytes to ensure alignment.
  //
  allocationSize_ = (Int32) sizeof(ExSimpleSQLBufferEntry) + tupleSize_ - 1;
  allocationSize_ += 7;
  allocationSize_ &= ~0x07;

  // Sometimes we might not be able to get all the memory requested. Try
  // to get at least enough memory for one tupp.
  //
  Int32 tuplesRequested = numberTuples_;
  Int32 nBytes = 0;
  numberTuples_ *= 2;
  while (!data_ && numberTuples_ > 0) 
    {
      numberTuples_ /= 2;
      nBytes = numberTuples_ * allocationSize_;
#if defined(HAS_ANSI_CPP_CASTS)
      data_ = static_cast<char *>(heap->allocateMemory(nBytes, false));
#else
      data_ = (char *) heap->allocateMemory(nBytes, false);
#endif
    }

  if (!data_)
    {
      nBytes = tuplesRequested * allocationSize_;
#if defined(HAS_ANSI_CPP_CASTS)
      data_ = static_cast<char *>(heap->allocateMemory(nBytes, true));
#else
      data_ = (char *) heap->allocateMemory(nBytes, true);
#endif
    }

  // Initially, all of the tuples are free so put all of them into the
  // free list.
  //
  freeList_ = (ExSimpleSQLBufferEntry*)data_;
  freeList_->init(tupleSize_);

  for(Int32 i=1; i < numberTuples_; i++) 
    {
      ExSimpleSQLBufferEntry *entry = 
	(ExSimpleSQLBufferEntry*)(data_ + i * allocationSize_);
      entry->init(tupleSize_);
      entry->setNext(freeList_);
      freeList_ = entry;
    }
};

void 
ExSimpleSQLBuffer::reclaimTuples(void) 
{
  ExSimpleSQLBufferEntry *prevUsedEntry = 0;
  ExSimpleSQLBufferEntry *usedEntry = usedList_;
  while(usedEntry) 
    {
      ExSimpleSQLBufferEntry *nextUsedEntry = usedEntry->getNext();

      // If a tupp is no longer referenced, reclaim it by moving it from
      // the used list to the free list.
      //
      if (usedEntry->tuppDesc_.getReferenceCount() == 0)
        {
          if (prevUsedEntry)
            {
              prevUsedEntry->setNext(nextUsedEntry);
            }
          else
            {
              usedList_ = nextUsedEntry;
            }

          usedEntry->setNext(freeList_);
          freeList_ = usedEntry;
        }
      else
        {
          prevUsedEntry = usedEntry;
        }

      usedEntry = nextUsedEntry;
    }
}
