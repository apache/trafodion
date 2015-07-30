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
#include "Platform.h"
#include <string.h>
#include <stdio.h>

#include "SortError.h"
#include "str.h"

//----------------------------------------------------------------------
// SortError Constructor.
//----------------------------------------------------------------------
SortError::SortError(){
  initSortError(); 
}

//----------------------------------------------------------------------
// SortError Destructor.
//----------------------------------------------------------------------
SortError::~SortError() { }

void SortError::initSortError()
{
  sortError_      = 0;
  sysError_       = 0;
  sysErrorDetail_ = 0; 
  sortErrorMsg_[0] = '\0';
}

void SortError::setErrorInfo(short sorterr, short syserr, short syserrdetail, const char *errorMsg)
{
  sortError_ = -sorterr;
  sysError_ = syserr;
  sysErrorDetail_ = syserrdetail; 
  if (errorMsg == NULL) 
    sortErrorMsg_[0] = '\0';   
  else
  {
    Int32 count = str_len(errorMsg);
    if( count >= sizeof(sortErrorMsg_)) count = sizeof(sortErrorMsg_) - 1;
    str_cpy(sortErrorMsg_, errorMsg, count);
    sortErrorMsg_[count] = '\0';    
  }
}

short SortError::getSortError() const { return sortError_; }
short SortError::getSysError() const { return sysError_; }
short SortError::getErrorDetail() const {return sysErrorDetail_; }
char* SortError::getSortErrorMsg() const { return (char*)sortErrorMsg_; }
