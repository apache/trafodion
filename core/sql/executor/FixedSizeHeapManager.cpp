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
********************************************************************************
*
* File:         FixedSizeHeapManager.C
* Description:  Implementation for fixed size heap manager
*               
*               
* Created:      12/19/96
* Language:     C++
*
*
*
*
********************************************************************************
*/

// -----------------------------------------------------------------------------

#include "ex_stdh.h"
#ifdef NA_MDAM_EXECUTOR_DEBUG
#include <iostream>
#endif /* NA_MDAM_EXECUTOR_DEBUG */

#include "FixedSizeHeapManager.h"

// *****************************************************************************
// Member functions for class FixedSizeHeapManager
// *****************************************************************************

// Constructor.
FixedSizeHeapManager::FixedSizeHeapManager(const size_t elementSize,
					   const Lng32 numberOfElements)
     : adjustedElementSize_(elementSize),
       numberOfElements_(numberOfElements),
       freeListHeadPtr_(0),
       firstBlockPtr_(0),
       defaultHeapPtr_(0),
       sizeBlockRounded_(0)
{
  // Increase adjustedElementSize_ to accommodate a FixedSizeHeapElement,
  // if necessary.
  if (adjustedElementSize_ < sizeof(FixedSizeHeapElement))
    {
      adjustedElementSize_ = sizeof(FixedSizeHeapElement);
    };
  
  // Increase adjustedElementSize_ to a multiple of four for allignment,
  // if necessary.
  // $$$$ Consider adjustedElementSize_ = ((adjustedElementSize_ + 3) >> 2) << 2;
  if (adjustedElementSize_ % 4 > 0)
    {
      adjustedElementSize_ += 4 - adjustedElementSize_ % 4;
    };

  // Calculate the size of a FixedSizeHeapManagerBlock and increase it for
  // allignment.
  sizeBlockRounded_ = sizeof(FixedSizeHeapManagerBlock);
  if (sizeBlockRounded_ % 4 > 0)
    {
      sizeBlockRounded_ += 4 - sizeBlockRounded_ % 4;
    };
}

// Destructor.
FixedSizeHeapManager::~FixedSizeHeapManager() 
{
  releaseHeapMemory();
}


// Allocate an element on the heap.
void * FixedSizeHeapManager::allocateElement(size_t size)
{
  // Check that element space is available.
  ex_assert(elementSpaceAvailable(),
	    "FixedSizeHeapManager::allocateElement: heap is full.");

  // Check that the item to be allocated will fit.
  ex_assert(size <= adjustedElementSize_,
	    "FixedSizeHeapManager::allocateElement: item too large.");

  // Return the first element from the free chain.
  FixedSizeHeapElement * tempPtr = freeListHeadPtr_;
  freeListHeadPtr_ = freeListHeadPtr_->getNextElementPtr();
  return tempPtr;
}


// Obtains the memory for the heap.  Returns 0 if successful and an
// ExeErrorCode if not successful.  Builds the free list.
ExeErrorCode FixedSizeHeapManager::acquireHeapMemory(CollHeap * defaultHeapPtr)
{
  defaultHeapPtr_ = defaultHeapPtr;
  ExeErrorCode rc = (ExeErrorCode)0;  // assume success

  // If acquireHeapMemory() is called and the memory was already acquired, just return.
  if (heapMemoryAcquired()) return rc;

  // elementsStillNeeded is the number of elements for which we still need memory.
  // Initially, we need memory for all elements.
  Lng32 elementsStillNeeded = numberOfElements_; 
 
  // The number of elements for the request.  Initially ask for all that is needed.
  Lng32 elementsRequested = numberOfElements_;  
 
  // Size of the request in bytes.
  size_t requestSize = 0;

  char * tempMemoryPtr = 0;

  // Iterate requesting blocks of memory until enough has been obtained to
  // acommodate all the elements.
  while (elementsStillNeeded > 0 && elementsRequested > 0)
    {
      requestSize = (size_t)(sizeBlockRounded_ + (elementsRequested * adjustedElementSize_));
      tempMemoryPtr = (char *)(defaultHeapPtr_->allocateMemory(requestSize, FALSE));
      if (tempMemoryPtr) // Did we get the memory block?
        { // Yes, we got the memory block.  Add the new elements to the free list.
          new (tempMemoryPtr) FixedSizeHeapManagerBlock(*this, elementsRequested, requestSize);
          elementsStillNeeded -= elementsRequested;
          // Ensure that elementsRequested <= elementsStillNeeded.  Otherwise, the last
          // block might be too large.
          if (elementsRequested > elementsStillNeeded)
            {
              elementsRequested = elementsStillNeeded;
            }
        }
      else
        { // No, we didn't get the memory block.  Try again with half as many elements.
          elementsRequested /= 2;
        };
    };

  if (elementsStillNeeded > 0 )
    {
       // Failed to get the required memory.  This is fatal.
       rc = EXE_NO_MEM_TO_EXEC;
       releaseHeapMemory();
    };

  return rc;
}


// Returns true if the heap memory has been acquired.  Otherwise, returns
// false.  Invarient relation: we have all or none of the memory.
NABoolean FixedSizeHeapManager::heapMemoryAcquired() const
{
  NABoolean rc = FALSE;
  if (firstBlockPtr_)
    {
      rc = TRUE;
    };
  return rc;
}


// Print the status of the heap.
#ifdef NA_MDAM_EXECUTOR_DEBUG
void FixedSizeHeapManager::print(const char * header) const
{
  cout << endl << header << endl;
  cout << "  adjustedElementSize_ = " << adjustedElementSize_ << endl;
  cout << "  numberOfElements_ = " << numberOfElements_ << endl;
  cout << "  freeListHeadPtr_ = " << (void *)freeListHeadPtr_ << endl;
  cout << "  firstBlockPtr_ =   " << (void *)firstBlockPtr_ << endl;
  cout << "  defaultHeapPtr_ =   " << (void *)defaultHeapPtr_ << endl;
  // Print the free list.
  cout << "  Free list..." << endl;
  FixedSizeHeapElement * tempElementPtr = 0;
  for (tempElementPtr = freeListHeadPtr_; 
       tempElementPtr != 0; 
       tempElementPtr = tempElementPtr->getNextElementPtr())
    {
      tempElementPtr->print();
    };
}
#endif /* NA_MDAM_EXECUTOR_DEBUG */


// Release heap memory.
void FixedSizeHeapManager::releaseHeapMemory()
{
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  // Check that all element memory has been returned to the free list.
  if (heapMemoryAcquired())
    {
      Lng32 freeElementCount = 0;
      FixedSizeHeapElement * tempElementPtr = 0;
      for (tempElementPtr = freeListHeadPtr_; 
	   tempElementPtr != 0; 
	   tempElementPtr = tempElementPtr->getNextElementPtr())
	{
	  ++freeElementCount;
	};
      ex_assert(freeElementCount == numberOfElements_,
      "FixedSizeHeapManager::releaseHeapMemory(): elements missing from free list.");
    }
  #endif /* NA_MDAM_EXECUTOR_DEBUG */


  // Iterate over the blocks releasing memory.
  FixedSizeHeapManagerBlock * tempBlockPtr = 0;
  FixedSizeHeapManagerBlock * nextTempBlockPtr = 0;
  for (tempBlockPtr = firstBlockPtr_;
       tempBlockPtr != 0;
       tempBlockPtr = nextTempBlockPtr)
    {
       nextTempBlockPtr = tempBlockPtr->getNextBlockPtr();
       defaultHeapPtr_->deallocateMemory(tempBlockPtr);
    };

  freeListHeadPtr_ = 0;
  firstBlockPtr_ = 0;
  defaultHeapPtr_ = 0;
}


// *****************************************************************************
// Member functions for class FixedSizeHeapManagerBlock
// *****************************************************************************

// Constructor.
FixedSizeHeapManagerBlock::FixedSizeHeapManagerBlock
  (FixedSizeHeapManager & fixedSizeHeapManagerRef,
   const Lng32 numberOfElements, 
   const size_t rawMemorySize)
     : nextBlockPtr_(fixedSizeHeapManagerRef.firstBlockPtr_),
       rawMemorySize_(rawMemorySize),
       numberOfElements_(numberOfElements),
       firstElementPtr_(0)
{
  // Set the first element pointer.
  firstElementPtr_ = (FixedSizeHeapElement *)(((char *)this) 
     + fixedSizeHeapManagerRef.sizeBlockRounded_);

  ex_assert(numberOfElements_ > 0,
      "FixedSizeHeapManagerBlock::FixedSizeHeapManagerBlock(): numberOfElements_ must be positive.");

  // Build the free list.
  char * tempPtr = 0;
  char * lastPtr = (char *)firstElementPtr_
	             + ((numberOfElements_ - 1) 
                        * fixedSizeHeapManagerRef.adjustedElementSize_);
  for (tempPtr = (char *)firstElementPtr_;
       tempPtr < lastPtr;
       tempPtr += fixedSizeHeapManagerRef.adjustedElementSize_)
    {
      new (tempPtr) FixedSizeHeapElement
	    ((FixedSizeHeapElement *) (tempPtr + fixedSizeHeapManagerRef.adjustedElementSize_));
    };

    // Set the pointer in the last element if this block to point to the
    // first element in the next block.
    new (lastPtr) FixedSizeHeapElement(fixedSizeHeapManagerRef.freeListHeadPtr_);

    // Set freeListHeadPtr_ to point to the first element in this new block.
    fixedSizeHeapManagerRef.freeListHeadPtr_ = firstElementPtr_;

    // Set firstBlockPtr_ to point to this new block.
    fixedSizeHeapManagerRef.firstBlockPtr_ = this;
}

