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
//
//**********************************************************************
//
// ExDupSqlBuffer.cpp

#include "ExDupSqlBuffer.h"

ExDupSqlBuffer::ExDupSqlBuffer(UInt32 nTuples, UInt32 tupleSize,
                               UInt32 nReserve, NAMemory* heap)
  : ExSimpleSQLBuffer(static_cast<Int32>(nTuples),
                      static_cast<Int32>(tupleSize), heap),
    dupCurrent_(NULL), dupHead_(NULL), dupTail_(NULL),
    maxDups_(0), nDups_(0)
{
  if (nTuples > nReserve)
  {
    maxDups_ = nTuples - nReserve;
  }
};

ExDupSqlBuffer::ExDupSqlBuffer(UInt32 nBuffers, UInt32 bufferSize, 
                               UInt32 nReserve, UInt32 tupleSize,
                               NAMemory* heap)
  : ExSimpleSQLBuffer(static_cast<Int32>(nBuffers),
                      static_cast<Int32>(bufferSize),
                      static_cast<Int32>(tupleSize), heap),
    dupCurrent_(NULL), dupHead_(NULL), dupTail_(NULL),
    maxDups_(0), nDups_(0)
{
  UInt32 nTuples = static_cast<UInt32>(getNumTuples());
  if (nTuples > nReserve)
  {
    maxDups_ = nTuples - nReserve;
  }
}

ExDupSqlBuffer::~ExDupSqlBuffer() 
{
};

void 
ExDupSqlBuffer::finishDups(void)
{
  if (dupHead_)
  {
    ExSimpleSQLBufferEntry * usedList = getUsedList();
    if (usedList)
    {
      dupTail_->setNext(usedList);
    }
    setUsedList(dupHead_);
    dupHead_ = NULL;
    dupTail_ = NULL;
    dupCurrent_ = NULL;
    nDups_ = 0;
  }
}

bool 
ExDupSqlBuffer::getDupTuple(tupp& tupp) 
{
  // Preserve tuple order by adding entries to the end of the duplicate list.

  bool haveEntry = false;

  if (nDups_ < maxDups_)
  {
    ExSimpleSQLBufferEntry* entry = getFreeEntry();
    haveEntry = (entry != NULL);
    if (haveEntry)
    {
      ++nDups_;
      if (!dupHead_)
      {
        dupHead_ = entry;
      }
      if (dupTail_)
      {
        dupTail_->setNext(entry);
      }
      dupTail_ = entry;

      tupp = &entry->tuppDesc_;
    }
  }

  return haveEntry;
};
