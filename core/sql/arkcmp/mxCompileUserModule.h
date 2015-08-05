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
#ifndef MXCOMPILEUSERMODULE__H
#define MXCOMPILEUSERMODULE__H

/* -*-C++-*-
 *****************************************************************************
 * File:         mxCompileUserModule.h
 * Description:  This class holds the heap, diags & other global variables used
 *               by the mxCompileUserModule tool.
 * Created:      05/19/2003
 * Language:     C++
 *****************************************************************************
 */

#include "mxcmpReturnCodes.h"

class ComDiagsArea;
class DgBase;
class NAHeap;

class mxCompileUserModule {
 public:
  mxCompileUserModule();
  virtual ~mxCompileUserModule();

  ComDiagsArea& operator<<(const DgBase&);
  void internalError(const char *file, Int32 line, const char *msg);

  void dumpDiags();
  Int32 diagsCount();

  ComDiagsArea& operator<<(mxcmpExitCode rc) 
    { setReturnCode(rc); return *diags_; }
  void setReturnCode(mxcmpExitCode rc);
  mxcmpExitCode returnCode() { return returnCode_; }

  NAHeap *heap();

 private:
  NAHeap       *heap_;
  ComDiagsArea *diags_;
  mxcmpExitCode returnCode_;
};

inline NAHeap* mxCompileUserModule::heap() { return heap_; }

extern mxCompileUserModule *mxCUMptr;

#define mxCUMinternalError(msg) mxCUMptr->internalError(__FILE__,__LINE__,msg)

#endif // MXCOMPILEUSERMODULE__H
