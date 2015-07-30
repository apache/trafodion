// ***************************************************************************
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
// -*-C++-*-
// ***************************************************************************
//
// File:         CmpErrLog.h
// Description:  This file contains the definition of the CmpErrLog class.
//               This class is used for logging information to a file when
//               certain types of errors occur. It is used primarily for
//               logging memory errors, but may be used for other types of
//               errors too.
//
//               This class is used by simply constructing a CmpErrLog
//               object.
//
// Created:      9/19/2008
// Language:     C++
//
// ***************************************************************************
#ifndef _CMP_ERR_LOG_H
#define _CMP_ERR_LOG_H

#include "Platform.h"
#include <sys/types.h>

// Forward declarations
class NAMemory;
class NAHeap;
typedef NAMemory CollHeap;


class CmpErrLog
{
public:
  // CmpErrLog constructor. The "failedHeap" field below is a pointer
  //     to the heap where a failure occurred.
  CmpErrLog(const char *failureTxt, CollHeap *failedHeap = 0, size_t size = 0);
  ~CmpErrLog();

  // This function may be passed to NAMemory::setErrorCallback() to
  // allow logging to occur when a memory allocation failure occurs
  // in the NAMemory code.
  static void CmpErrLogCallback(NAHeap *heap, size_t userSize);

private:
  void renameBigLogFile(const char *fileName);
  void openLogFile();
  void closeLogFile();

  void writeHeader(const char *failureTxt, CollHeap *failedHeap, size_t size);
  void writeMemoryStats();
  void writeHeapInfo(CollHeap *heap);
  void writeAllHeapInfo(CollHeap *failedHeap);
  void writeCQDInfo();
  void writeQueryInfo();
  void writeStackTrace();

  // Output file pointer 
  FILE *fp;

  // Pointer to memory buffer that can be freed to allow memory needed
  // during logging to be allocated.
  static void *memPtr;
};

#endif // _CMP_ERR_LOG_H
