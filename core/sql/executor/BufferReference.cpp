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
// BufferReference.cpp
//

#include "BufferReference.h"
#include "ex_ex.h"

namespace ExOverflow
{
  BufferReference::BufferReference(ByteCount bufferSize)
    : buffer_(NULL), maxOffset_(bufferSize), offset_(0)
  {}

  BufferReference::BufferReference(const BufferReference& br)
  {
    buffer_    = br.buffer_;
    maxOffset_ = br.maxOffset_;
    offset_    = br.offset_;
  }

  BufferReference::~BufferReference(void)
  {}

  BufferReference& 
  BufferReference::operator=(const BufferReference& rhs)
  {
    if (this != &rhs)
    {
      buffer_    = rhs.buffer_;
      maxOffset_ = rhs.maxOffset_;
      offset_    = rhs.offset_;
    }
    return *this;
  }

  void 
  BufferReference::advance(ByteCount numBytes)
  { 
    ex_assert((bytesAvailable() >= numBytes), "advance past end of buffer");
    offset_ += numBytes;
  }

  ByteCount
  BufferReference::bytesAvailable(void) const
  {
    ByteCount nBytes = 0;
    if (isValid())
    {
      ex_assert((maxOffset_ > 0), "maxOffset_ not initialized");
      ex_assert((offset_ <= maxOffset_), "invalid offset");
      nBytes = (maxOffset_ - offset_);
    }

    return nBytes;
  }

  void
  BufferReference::invalidate(void)
  {
    buffer_ = NULL;
    offset_ = 0;
  }

  bool
  BufferReference::sameBuffer(const BufferReference& left, 
                              const BufferReference& right)
  {
    return left.isValid() && (left.buffer_ == right.buffer_);
  }

  bool
  BufferReference::sameBuffer(const BufferReference& left, 
                              const char* right)
  {
    return left.isValid() && (left.buffer_ == right);
  }

  void 
  BufferReference::set(char* buffer, BufferOffset offset)
  {
    ex_assert((buffer != NULL), "set NULL buffer reference");

    buffer_ = buffer;
    offset_ = offset;
  }
}
