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
* File:         ExBitMapTable.h
* RCS:          $Id
* Description:  ExBitMapTable class declaration
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
#ifndef __ExBitMapTable_h__
#define __ExBitMapTable_h__

// Includes
//
#include "ex_god.h"
#include "Int64.h"
#include "ComDefs.h"

// External forward declarations
//
class NAMemory;

class ExBitMapTable : public ExGod {
public:
  // constructor
  //
  ExBitMapTable(Int32 keySize, Int32 dataSize, Int32 countOffset,
		Int32 memSize, NAMemory *heap);

  // destructor
  //
  ~ExBitMapTable();

  // Returns the maximum number of groups that can fit in the table. This
  // reflects the best case scenario.
  //
  Int32 getMaximumNumberGroups() const { return maximumNumberGroups_; };

  // Returns the minimum number of groups that can fit in the table. This
  // reflects the worst case scenario.
  //
  Int32 getMinimumNumberGroups() const { return maximumNumberGroups_; };

  // Returns a pointer to the current group's data.
  //
  char *getData() const { return data_; };
  
  // Returns a pointer to the Nth group's data.
  //
  char *getGroup(Int32 n) { return groups_ + n * rowSize_; }

  // Returns a pointer to the group's key.
  //
  char *getKey(char *group) { return group + dataSize_; }

  // Returns a pointer to the group's next group pointer.
  //
  char**getNextPtr(char *group) 
  { return (char**)(group + dataSize_ + ROUND4(keySize_) ); }

  // Return the number of groups in the table.
  //
  Int32 getNumberGroups() const { return numberGroups_; };

  // Advances the current return group.
  //
  void advanceReturnGroup() { returnGroup_++; };

  // Resets the current return group.
  //
  void resetReturnGroup() { returnGroup_ = 0; };

  // Gets the current returng group.
  //
  char *getReturnGroup() { 
    if(returnGroup_ < numberGroups_) return getGroup(returnGroup_);
    return NULL;
  }

  // Find or adds the group pointed to be key to the table.
  //
  Int32 findOrAdd(char *key);

  // Initialize any table aggregates.
  //
  inline void initAggregates() { *(Int64*)(data_ + countOffset_) = 0; };

  // Increment any table aggregates.
  //
  inline void applyAggregates() { (*(Int64*)(data_ + countOffset_))++; };

  // Reset the table.
  //
  void reset();

private:
  Int32 keySize_;
  Int32 dataSize_;
  Int32 countOffset_;
  Int32 memSize_;
  Int32 rowSize_;
  NAMemory *heap_;

  Int32 maximumNumberGroups_;
  Int32 numberHashBuckets_;
  char *memory_;
  char *data_;
  char *groups_;
  char **buckets_;

  Int32 numberGroups_;
  Int32 returnGroup_;
};

#endif

