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

// BufferList.h
//
// BufferList is a buffer list that combines buffer management with a
// Queue.  BufferList does not own the buffers in its list.
//

#ifndef BUFFERLIST_H
#define BUFFERLIST_H

#include <NABasicObject.h>
#include <NAType.h>
#include <ComQueue.h>

#include "ExOverflow.h"

namespace ExOverflow
{

  class BufferList : public NABasicObject
  {
    public:
      BufferList(NAMemory* heap);
      ~BufferList(void);

      // Return a pointer to the last buffer in the list
      char* back(void);

      // Return a pointer to the current buffer in the list
      char* current(void);

      // Is the buffer list empty?
      bool empty(void);

      // Return a pointer to the first buffer in the list
      char* front(void);

      // Return a pointer to the next buffer in the list
      char* next(void);

      // Unlink the first buffer from the list
      char* pop(void);

      // Reset current position to head of list
      void position(void);

      // Insert a buffer at the end of the list
      void push_back(char* buffer);

      // Number of buffers in the buffer list
      UInt32 size(void);

    private:
      Queue bufferList_;        // List of buffers
  };

}

#endif
