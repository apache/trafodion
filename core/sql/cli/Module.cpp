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
 *
 * File:         Module.cpp
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "cli_stdh.h"
#include "ExMeas.h"

Module::Module(const char * module_name, Lng32 len, char * pathName, 
               Lng32 pathNameLen, NAHeap *heap)
     : module_name_len_(len), path_name_len_(pathNameLen), 
       heap_(heap), statementCount_(0),
       stmtCntrsSpace_(NULL),
       vproc_(NULL)
{
  module_name_ = (char *)
   (heap->allocateMemory((size_t)(len+1)));

  str_cpy_all(module_name_, module_name, len);
  module_name_[len] = 0;

  path_name_ = (char *)
   (heap->allocateMemory((size_t)(pathNameLen+1)));

  str_cpy_all(path_name_, pathName, pathNameLen);
  path_name_[pathNameLen] = 0;

}
// ss-cc_change . The desctructore for module is never called.
//LCOV_EXCL_START
Module::~Module()
{
  if (module_name_)
    {
      heap_->deallocateMemory(module_name_);
    }
  module_name_ = 0;

  if (stmtCntrsSpace_)
    {
      heap_->deallocateMemory(stmtCntrsSpace_);
    }
  stmtCntrsSpace_ = 0;
}
//LCOV_EXCL_STOP

void Module::allocStmtCntrsSpace(NABoolean measEnabled)
{
  if (statementCount_ == 0)
    return;

  // allocate a block of contiguous memery for measure statement counters
  // of all statements in one module.

  stmtCntrsSpace_ = (char *)heap_->allocateMemory((size_t) (statementCount_ * sizeof (ExMeasStmtCntrs)));

  // initialize each statement counters space.

  // mid = PXFS_PATHNAME_RESOLVE_(moduleName)  
  //char * mid = this->getPathName();

  ExMeasStmtCntrs * stmtCntrs;
  for (Int32 i = 0; i < statementCount_; i++)
    {
      stmtCntrs = (ExMeasStmtCntrs *)getStmtCntrsSpace(i);
      stmtCntrs->init(i);
    };
  if (measEnabled)
    {
      // call Measure to setup statment counters.
      stmtCntrs = (ExMeasStmtCntrs *)stmtCntrsSpace_;
      stmtCntrs->ExMeasStmtCntrsBump(statementCount_, module_name_,
				     (Int32)module_name_len_);
    }
};

char * Module::getStmtCntrsSpace (Int32 statementIndex)
{
  // get individule counter space
  assert (statementIndex < statementCount_);
  if (stmtCntrsSpace_)
    return stmtCntrsSpace_ + (statementIndex * sizeof (ExMeasStmtCntrs));
  else return 0;
};
  
