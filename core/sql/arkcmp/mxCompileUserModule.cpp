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
 *****************************************************************************
 * File:         mxCompileUserModule.cpp
 * Description:  This is the main class for SQL compiling a C/C++/Cobol
 *               executable/library or SQLJ ser/jar file that has embedded 
 *               module definitions.
 * Created:      03/03/2003
 * Language:     C++
 *****************************************************************************
 */

#include <iostream>
#include "Platform.h"
#include "ComDiags.h"
#include "DgBaseType.h"
#include "ErrorMessage.h"
#include "mxCompileUserModule.h"
#include "NAMemory.h"

mxCompileUserModule::mxCompileUserModule() 
  : heap_(NULL), diags_(NULL), returnCode_(SUCCEED)
{
  heap_ = new NAHeap("mxCompileUserModule Heap", 
                     NAMemory::DERIVED_FROM_SYS_HEAP,
                     (Lng32)524288);

  diags_ = ComDiagsArea::allocate(heap_);
}

mxCompileUserModule::~mxCompileUserModule()
{
  if (diags_) {
    diags_->decrRefCount();
  }
  if (heap_) {
    delete heap_;
    heap_ = NULL;
  }
}

ComDiagsArea& mxCompileUserModule::operator <<(const DgBase& dgObj)
{
  if (!diags_) {
    cerr << "Error: ComDiagsArea is not yet created." << endl;
    exit(1);
  }
  return *diags_ << dgObj;
}

void mxCompileUserModule::dumpDiags()
{
  if (diagsCount()) {
    NADumpDiags(cerr, diags_, TRUE);
    diags_->clear();
  }
}

Int32 mxCompileUserModule::diagsCount()
{
  return !diags_ ? 0 : 
    (diags_->getNumber(DgSqlCode::ERROR_)+
     diags_->getNumber(DgSqlCode::WARNING_));
}

void mxCompileUserModule::internalError(const char *file, Int32 line, 
                                        const char *msg)
{
  *this << DgSqlCode(-2214) << DgString0(file) << DgInt0(line) 
        << DgString1(msg);
}

void mxCompileUserModule::setReturnCode(mxcmpExitCode rc)
{
  switch (returnCode_) {
  case FAIL: 
    break; // always report FAIL
  case ERROR:
    if (rc == FAIL)
      returnCode_ = rc; // report FAIL over ERROR
    break;
  case WARNING:
    if (rc == FAIL || rc == ERROR)
      returnCode_ = rc; // report FAIL, ERROR over WARNING
    break;
  default:
    returnCode_ = rc; // report FAIL, ERROR, WARNING over SUCCEED
    break;
  }
}
