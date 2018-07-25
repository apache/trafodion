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
//
// BufferList.cpp
//
#include "BufferList.h"

namespace ExOverflow
{

  BufferList::BufferList(NAMemory* heap) : bufferList_(heap)
  {
  }

  BufferList::~BufferList(void)
  {
  }

  char*
  BufferList::back(void)
  {
    return static_cast<char*>(bufferList_.getTail());
  }

  char*
  BufferList::current(void)
  {
    return static_cast<char*>(bufferList_.getCurr());
  }

  bool
  BufferList::empty(void)
  {
    return (bufferList_.isEmpty() != 0);
  }

  char*
  BufferList::front(void)
  {
    return static_cast<char*>(bufferList_.getHead());
  }

  char*
  BufferList::next(void)
  {
    if (bufferList_.atEnd())
      return NULL;
    else
    {
      bufferList_.advance();
      return this->current();
    }
  }


  char*
  BufferList::pop(void)
  {
    char* bufptr = NULL;

    if (!empty())
    {
      bufptr = front();
      bufferList_.remove();
    }

    return bufptr;
  }

  void
  BufferList::position(void)
  {
    bufferList_.position();
  }

  void
  BufferList::push_back(char* buffer)
  {
    if (buffer != NULL)
    {
      bufferList_.insert(buffer);
    }
  }

  UInt32
  BufferList::size(void)
  {
    return static_cast<UInt32>(bufferList_.numEntries());
  }

}
