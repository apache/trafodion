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
// BufferReference.h
//

#ifndef BUFFERREFERENCE_H
#define BUFFERREFERENCE_H

#include <NABasicObject.h>
#include "ExOverflow.h"

namespace ExOverflow
{
  class BufferReference : public NABasicObject
  {
  public:
    BufferReference(ByteCount bufferSize);
    BufferReference(const BufferReference& br);
    ~BufferReference(void);

    BufferReference& operator=(const BufferReference& rhs);

    // Advance buffer position numBytes
    void advance(ByteCount numBytes);

    // Byte available in buffer
    ByteCount bytesAvailable(void) const;

    // Get pointer to beginning of buffer
    char* getBuffer(void) const;

    // Get buffer position
    char* getPosition(void) const;

    // Invalidate buffer reference
    void invalidate(void);

    // Is buffer reference valid?
    bool isValid(void) const;

    // Byte offset of current buffer position
    BufferOffset getOffset(void) const;

    // Do both references refer to same buffer?
    static bool sameBuffer(const BufferReference& left, 
                           const BufferReference& right);

    // Do reference and pointer refer to same buffer?
    static bool sameBuffer(const BufferReference& left, 
                           const char* right);

    // Set buffer reference to beginning of buffer
    void set(char* buffer);

    // Set buffer reference to buffer position
    void set(char* buffer, BufferOffset offset);

  private:
    char* buffer_;            // buffer base
    BufferOffset maxOffset_;  // valid offset range is (0..maxOffset_-1)
    BufferOffset offset_;     // position within buffer_
  };

  inline
  char*
  BufferReference::getBuffer(void) const
  {
    return buffer_;
  }

  inline
  char*
  BufferReference::getPosition(void) const
  {
    return (buffer_ + offset_);
  }

  inline
  bool
  BufferReference::isValid(void) const
  {
    return (buffer_ != NULL);
  }

  inline
  BufferOffset
  BufferReference::getOffset(void) const
  {
    return offset_;
  }

  inline
  void 
  BufferReference::set(char* buffer)
  {
    set(buffer, 0);
  }
}

#endif
