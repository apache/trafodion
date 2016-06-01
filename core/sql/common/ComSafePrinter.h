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
#ifndef COMSAFEPRINTER_H
#define COMSAFEPRINTER_H
/* -*-C++-*-
******************************************************************************
*
* File:         ComSafePrinter.h
* Description:  Safe versions of some sprintf-style functions. They are
*               "safe" in the sense that they prevent writing beyond the
*               end of the target buffer. Some platforms, but not OSS,
*               provide similar functions in the stdio library.
*
* Created:      June 2003
* Language:     C++
*
*
******************************************************************************
*/

#include "Platform.h"
#include <stdarg.h>
#include <stdio.h>

class ComSafePrinter
{
public:
  ComSafePrinter();
  ~ComSafePrinter();
  Int32 snPrintf(char *, size_t, const char *, ...);
  Int32 vsnPrintf(char *, size_t, const char *, va_list);

protected:
  // We maintain a global temporary FILE for the purpose of buffering
  // sprintf output. We can safely write to this FILE without worrying
  // about buffer overflow, then read a fixed number of bytes from the
  // FILE back into memory. Don't konw for sure, but chances are that
  // when our output strings are small, no I/O will actually be
  // performed because of the stdio library's buffering of FILE data.
  static THREAD_P FILE *outfile_;

private:
  // Do not implement default constructors or an assignment operator
  ComSafePrinter(const ComSafePrinter &);
  ComSafePrinter &operator=(const ComSafePrinter &);
  
}; // class ComSafePrinter

#endif // COMSAFEPRINTER_H
