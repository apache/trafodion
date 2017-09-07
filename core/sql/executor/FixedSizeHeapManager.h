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
#ifndef FIXEDSIZEHEAPMANAGER_H
#define FIXEDSIZEHEAPMANAGER_H
/* -*-C++-*-
********************************************************************************
*
* File:         FixedSizeHeapManager.h
* Description:  Fixed size heap manager
*
*
* Created:      12/19/96
* Language:     C++
*
*
********************************************************************************
*/

// -----------------------------------------------------------------------------

#include "NABoolean.h"
#include "FixedSizeHeapElement.h"
#include "NAHeap.h"   //
#include "ex_error.h"  // for ExeErrorCode
#ifdef NA_MDAM_EXECUTOR_DEBUG
#include <iostream>
#endif /* NA_MDAM_EXECUTOR_DEBUG */
// *****************************************************************************
// FixedSizeHeapManager is a simple manager for a heap containing fixed-sized
// elements.
// *****************************************************************************

// Forward declarations.
class FixedSizeHeapElement;
class FixedSizeHeapManagerBlock;
// End of forward declarations.

class FixedSizeHeapManager
{

friend class FixedSizeHeapManagerBlock;

public:

  // Constructor.
  FixedSizeHeapManager(const size_t elementSize,
				  const Lng32 numberOfElements);

  // Destructor.
   ~FixedSizeHeapManager();

  // Obtains the memory for the heap.  Returns 0 if successful and an
  // ExeErrorCode if not successful.  Builds the free list.
  ExeErrorCode acquireHeapMemory(NAMemory * defaultHeapPtr);

  // Allocate an element on the heap.
  void * allocateElement(size_t size);

  // Returns true if the heap memory has been acquired.  Otherwise, returns
  // false.
  NABoolean heapMemoryAcquired() const;

  // Print the status of the heap.
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  void print(const char * header = "") const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG */

  // Release an element from the heap and return the space to the free list.
  inline void releaseElement(void * elementPtr);

  // Release heap memory.
  void releaseHeapMemory();

  // Returns true if there is space available for an element on the heap.
  // Retruns false if the heap is full or memory not acquired.
  inline NABoolean elementSpaceAvailable() const;


private:

  // The size of an element in the heap.
  size_t adjustedElementSize_;

  // Maximum number of elements in the heap.
  const Lng32 numberOfElements_;

  // Pointer to the head of the free list.  Zero if the heap is full or
  // if the heap memory has not been acquired.
  FixedSizeHeapElement * freeListHeadPtr_;

  // First block pointer.  Zero if the heap memory is not acquired.  If
  // non-zero, the heap memory was sucessfully acquired and the free list
  // was built.
  FixedSizeHeapManagerBlock * firstBlockPtr_;

  // Pointer to the default heap.  This is needed to deallocate the space.
  NAMemory * defaultHeapPtr_;

  // Size of a FixedSizeHeapManagerBlock rounded up for allignment.
  Lng32 sizeBlockRounded_;

  // Copy constructor disallowed.
  inline FixedSizeHeapManager(const FixedSizeHeapManager &);

}; // class FixedSizeHeapManager


// *****************************************************************************
// Inline member functions for class FixedSizeHeapManager
// *****************************************************************************

// Copy constructor disallowed.  Use of this constructor is disallowed
// so that passing a class object by value rather than by reference will
// raise a compilation error.
inline FixedSizeHeapManager::FixedSizeHeapManager(const FixedSizeHeapManager &)
     : adjustedElementSize_(0),
       numberOfElements_(0),
       freeListHeadPtr_(0),
       firstBlockPtr_(0),
       sizeBlockRounded_(0)
{
  ex_assert(0,
    "FixedSizeHeapManager::FixedSizeHeapManager: copy constructor called.");
}


// Release an element returning the space to the free list.
inline void FixedSizeHeapManager::releaseElement(void * elementPtr)
{
  // Insert the element onto the head of the free list.
  ((FixedSizeHeapElement *)elementPtr)->setNextElementPtr(freeListHeadPtr_);
  freeListHeadPtr_ = (FixedSizeHeapElement *)elementPtr;
}


// Returns true if there is space available for an element on the heap.
// Returns false if the heap is full or memory not acquired.
inline NABoolean FixedSizeHeapManager::elementSpaceAvailable() const
{
  return freeListHeadPtr_ != 0;
}


// *****************************************************************************
// FixedSizeHeapManagerBlock models a block of memory for the heap.
// *****************************************************************************

class FixedSizeHeapManagerBlock
{

public:

  // Constructor.
  FixedSizeHeapManagerBlock(FixedSizeHeapManager & fixedSizeHeapManagerRef,
                                       const Lng32 numberOfElements,
                                       const size_t rawMemorySize);

  // Destructor.
  inline ~FixedSizeHeapManagerBlock();

  // Operator new.
  inline void * operator new(size_t, char * ptr);


  // Get function for nextBlockPtr_.
  inline FixedSizeHeapManagerBlock * getNextBlockPtr();


private:

  // Pointer to form a linked-list of the blocks.  This is a singlely-linked
  // list.  The last pointer is zero.
  FixedSizeHeapManagerBlock * nextBlockPtr_;

  // Raw memory size as allocated via call to allocateMemory().
  // Includes space for the FixedSizeHeapManagerBlock object
  // and the elements.
  const size_t rawMemorySize_;

  // Maximum number of elements in the block.
  const Lng32 numberOfElements_;

  // Pointer to first element in the block.
  FixedSizeHeapElement * firstElementPtr_;

  // Print function.
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  void print(const char * header = "") const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG */

}; // class FixedSizeHeapManagerBlock

// *****************************************************************************
// Inline member functions for class FixedSizeHeapManagerBlock
// *****************************************************************************

// Operator new.
inline void * FixedSizeHeapManagerBlock::operator new(size_t, char * ptr)
{
  return (void *)ptr;
}

// Get function for nextBlockPtr_.
inline FixedSizeHeapManagerBlock * FixedSizeHeapManagerBlock::getNextBlockPtr()
{
  return nextBlockPtr_;
}

#endif /* FIXEDSIZEHEAPMANAGER_H */
