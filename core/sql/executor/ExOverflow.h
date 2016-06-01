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
// ExOverflow.h - ExOverflow module common declarations

#ifndef EXOVERFLOW_H
#define EXOVERFLOW_H

#include "NAVersionedObject.h"

namespace ExOverflow
{
  enum IoStatus { OK, IO_PENDING, SQL_ERROR, END_OF_DATA, IO_ERROR, NO_MEMORY,
                  INTERNAL_ERROR } ;

  const UInt32 ONE_MEGABYTE = 1024 * 1024;

  // ExOverflow module common types
  typedef UInt32 BufferCount;
  typedef UInt32 BufferOffset;
  typedef UInt32 ByteCount;
  typedef UInt64 ByteCount64;
  typedef UInt64 SpaceOffset;
}

#endif
