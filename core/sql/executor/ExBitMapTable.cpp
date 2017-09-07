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
* File:         ExBitMapTable.cpp
* RCS:          $Id
* Description:  ExBitMapTable class Implementation
* Created:      7/1/97
* Modified:     $Author
* Language:     C++
* Status:       $State
*
*
*
*
******************************************************************************
*/

// ExBitMapTable.cpp - Implementation for the ExBitMapTable class.
//

// Includes
//
#include "ex_ex.h"
#include "ExBitMapTable.h"

// ExBitMapTable::ExBitMapTable
//
// Constructs an ExBitMapTable.
//
// IN     : keySize  - The size of the key in bytes.
// IN     : dataSize - The size of a data row in bytes.
// IN     : memSize  - The amount of memory requested for the table.
// IN     : heap     - The memory allocator.
// EFFECTS: Constructs an ExBitMapTable object.
//
ExBitMapTable::ExBitMapTable(Int32 keySize, Int32 dataSize, Int32 countOffset,
			     Int32 memSize, CollHeap *heap) 
  : keySize_(keySize), dataSize_(dataSize), countOffset_(countOffset),
    memSize_(memSize), 

    // row layout:
    //  data: 
    //  key:  
    //  filler: so nextPtr could be aligned at 4
    //  next ptr: 
    // total length a multiple of 8 so row could be aligned.
    rowSize_(ROUND8(dataSize_ + ROUND4(keySize) + sizeof(char *))),
    heap_(heap), memory_(NULL), maximumNumberGroups_(0), 
    numberHashBuckets_(0), numberGroups_(0), returnGroup_(0),
    buckets_(0), groups_(0)
{

  
  // Attempt to allocate the requested memory. If that fails, try to at
  // least allocate enough memory for a few tuples. Otherwise, simply
  // return and the caller will check getMaximumNumberTuples and realize
  // that the table cannot store any tuples.
  //
  // Memory layout
  //
  // Hash Bucket Pointer Array - NumberHashBuckets
  //   [sizeof(void*)] - pointer to first group in bucket 0
  //   [sizeof(void*)] - pointer to first group in bucket 1
  //       . . .
  //   [sizeof(void*)] - pointer to first group in bucket NumberHashBuckets-1
  // Group Memory - NumberGroups
  //   [rowSize] - data, key, and next for group 0
  //   [rowSize] - data, key, and next for group 1
  //   [rowSize] - data, key, and next for group NumberGroups-1
  //
#pragma nowarn(1506)   // warning elimination 
  const Int32 minimumMemorySize = 32 * (sizeof(char*) + rowSize_);
#pragma warn(1506)  // warning elimination 
  memSize_ *= 2;
  while(!memory_ && (memSize_ >= 2 * minimumMemorySize)) {
    memSize_ /= 2;
    memory_ = (char *)heap_->allocateMemory((size_t) memSize_, FALSE);
  }
  if(!memory_) 
    {
      return;
    }

  // Save room for as many hash buckets as there are groups.
  //
  numberHashBuckets_ = ROUND2(memSize_ / (sizeof(char*) + rowSize_));


  // Compute the maximum number of groups that can be stored in the table. 
  //
#pragma nowarn(1506)   // warning elimination 
  maximumNumberGroups_ = (memSize_ - sizeof(char*) * numberHashBuckets_)
    / rowSize_;
#pragma warn(1506)  // warning elimination 

  // The buckets 
  // BitMap at memory_. To start there is only one BitMap.
  //
  buckets_ = (char**)memory_;
  groups_ = memory_ + sizeof(char*) * numberHashBuckets_;

  // Initialize the hash buckets.
  //
  for(Int32 i=0; i<numberHashBuckets_; i++)
    buckets_[i] = 0;
}

// ExBitMapTable::~ExBitMapTable
//
// Destructs an ExBitMapTable.
//
// EFFECTS: Destructs an ExBitMapTable object. Releases the memory
//          allocated in the constructor and sets the referencing
//          pointer to NULL.
//
ExBitMapTable::~ExBitMapTable() {
  if(memory_) heap_->deallocateMemory(memory_);
  memory_ = NULL;
}

// ExBitMapTable::reset
//
// Resets an ExBitMapTable
//
// EFFECTS: Resets all of the bitmaps and number of groups in the table.
//
void ExBitMapTable::reset() {
  // Initialize the hash buckets_.
  //
  for(Int32 i=0; i<numberHashBuckets_; i++)
    buckets_[i] = 0;

  // Reset the number of groups.
  //
  numberGroups_ = 0;

  // Also reset the return group.
  //
  returnGroup_ = 0;
}

// ExBitMapTable::findOrAdd
//
// Attemps to find an existing group in the table with the same key 
// as _key_. If a group cannot be found, attempts to add a new group
// to the table given sufficient memory resources. Sets the member data_
// to reference the found or added group, or NULL if no match.
//
// IN     : key   - Pointer to the key for incoming group.
// EFFECTS: A group may be added to the table. The member data_ points 
//          to the matching group. 
//
Int32 ExBitMapTable::findOrAdd(char *key) {
  unsigned char *ukey = (unsigned char*)key;

  // Compute the hash 
  //
  UInt32 hash = 0;
  Int32 i=0;
  for(; i<keySize_; i++)
    {
      hash ^= ukey[i];
      hash <<= 3;
    }
  hash = hash % numberHashBuckets_;

  // If the bucket is empty, try to add a new group.
  //
  char *row = buckets_[hash];
  if(!row)
    {
      // If no more groups can be added, return 0.
      //
      if(numberGroups_ >= maximumNumberGroups_) return 0;

      // Set data_ and the bucket to point to the new group and
      // increment the number of groups.
      //
      data_ = buckets_[hash] = getGroup(numberGroups_++);

      // Initialize the key
      //
      row = getKey(data_);
      for(i=0; i<keySize_; i++)
	row[i] = key[i];

      // Initialize the next pointer
      //
      *getNextPtr(data_) = 0;

      // Initialize any table aggregates.
      //
      initAggregates();
      applyAggregates();

      // Return -1 to indicate that a new group was added. The 
      // initialization is done externally.
      //
      return -1;
    }

  char *lastRow;
  while(row)
    {
      lastRow = row;

      // Test for matching keys.
      //
      Int32 match = 1;
      row = getKey(lastRow);
      for(i=0; match && i<keySize_; i++)
	if(row[i] != key[i]) match = 0;

      // If a matching row is found, set data_ to point to it and 
      // return 0 to indicate a match.
      //
      if(match)
	{
	  data_ = lastRow;

	  // Apply any table aggregates.
	  //
	  applyAggregates();

	  // Return 1 to indicate the group was matched/added.
	  //
	  return 1;
	}

      // Get the pointer to the next row in this hash chain.
      //
      row = *getNextPtr(lastRow);
    }

  // No matching groups were found so try to add another group.
  //
  // If no more groups can be added, return 0.
  //
  if(numberGroups_ >= maximumNumberGroups_) return 0;

  // Set data_ and the next row pointer to point to the new group and
  // increment the number of groups.
  //
  *getNextPtr(lastRow) = data_ = getGroup(numberGroups_++);

  // Initialize the key
  //
  row = getKey(data_);
  for(i=0; i<keySize_; i++)
    row[i] = key[i];

  // Initialize the next pointer
  //
  *getNextPtr(data_) = 0;

  // Initialize any table aggregates.
  //
  initAggregates();
  applyAggregates();

  // Return -1 to indicate that a new group was added. The 
  // initialization is done externally.
  //
  return -1;
}
