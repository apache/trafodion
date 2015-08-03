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

#ifndef EXDUPSQLBUFFER_H
#define EXDUPSQLBUFFER_H

//
// ExDupSqlBuffer.h
//
// The ExDupSqlBuffer class adds duplicate tuple support to the
// ExSimpleSQLBuffer class.  Callers should not alter the duplicate
// list while iterating over its entries.
//

#include "ExSimpleSqlBuffer.h"

class ExDupSqlBuffer : public ExSimpleSQLBuffer
{
public:

  ExDupSqlBuffer(UInt32 numTuples, UInt32 tupleSize, UInt32 nReserve,
                 NAMemory* heap);

  // Constructor that is similar to the sql_buffer_pool ctor
  ExDupSqlBuffer(UInt32 numBuffers, UInt32 bufferSize, UInt32 nReserve,
                 UInt32 tupleSize, NAMemory* heap);

  ~ExDupSqlBuffer();

  // Advance the current position to the next duplicate tuple.
  // True => current position changed.
  bool advance(void);

  // Get the current tuple in the duplicate list.
  bool current(tupp& tp) const;

  // Allocate a tuple in the duplicate list.  True => success.
  bool getDupTuple(tupp &tp);

  // Allocate a tuple from the free list.  True => success.
  bool getTuple(tupp &tp);

  // Does the duplicate list have any entries?
  bool hasDups(void);

  // Set current position to the head of the duplicate list.
  void rewind(void);

  // Merge duplicate tuples into the ExSimpleSQLBuffer::usedList_ used
  // list. Unreferenced tuples will be moved to the free list when
  // tuples are reclaimed from the used list.
  void finishDups(void);

private:
  // Linked list of duplicate tuples.  Tuples in the duplicate list
  // start with a zero reference count.  Unreferenced tuples aren't
  // reclaimed until the duplicate list is combined with the 
  // ExSimpleSQLBuffer::usedList_ used list.  The current position
  // is used to iterate through the duplicate list.
  ExSimpleSQLBufferEntry * dupCurrent_;
  ExSimpleSQLBufferEntry * dupHead_;
  ExSimpleSQLBufferEntry * dupTail_;
  UInt32 maxDups_;    // Max duplicate list size
  UInt32 nDups_;      // Duplicate list size
};

inline
bool
ExDupSqlBuffer::advance(void)
{
  bool haveCurrent = (dupCurrent_ != NULL);

  if (haveCurrent)
  {
    dupCurrent_ = dupCurrent_->getNext();
  }

  return haveCurrent;
}

inline
bool
ExDupSqlBuffer::current(tupp& tp) const
{
  bool haveDup = (dupCurrent_ != NULL);

  if (haveDup)
  {
    tp = dupCurrent_->getTupDesc();
  }
  else
  {
    tp = NULL;
  }

  return haveDup;
}

inline
bool
ExDupSqlBuffer::getTuple(tupp &tp)
{
  return (getFreeTuple(tp) == 0);
}

inline
bool
ExDupSqlBuffer::hasDups(void)
{
  return (dupHead_ != NULL);
}

inline
void
ExDupSqlBuffer::rewind(void)
{
  dupCurrent_ = dupHead_;
}

#endif
