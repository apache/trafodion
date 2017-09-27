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

/*
 *****************************************************************************
 *
 * File:         ComViewColUsage.h
 * Description:  Defines the referenced object (table or view) column 
 *               relationship to the corresponding view column.
 *               
 * Created:      7/15/16
 * Language:     C++
 *
 *****************************************************************************
 */

#ifndef COMVIEWCOLUSAGE_H
#define COMVIEWCOLUSAGE_H

#include "ComSmallDefs.h"
#include "str.h"

class ComViewColUsage
{
  Int64 viewUID_;
  Int32 viewColNumber_;
  Int64 refdUID_;
  Int32 refdColNumber_;
  ComObjectType refdObjectType_;

public:
  ComViewColUsage ()
  : viewUID_ (0),
    viewColNumber_ (-1),
    refdUID_(0),
    refdColNumber_(-1),
    refdObjectType_(COM_UNKNOWN_OBJECT)
  {}

  ComViewColUsage (
   Int64 viewUID,
   Int32 viewColNumber,
   Int64 refdUID,
   Int32 refdColNumber,
   ComObjectType refdObjectType)
 : viewUID_ (viewUID),
   viewColNumber_ (viewColNumber),
   refdUID_ (refdUID),
   refdColNumber_ (refdColNumber),
   refdObjectType_ (refdObjectType)
{}

  virtual ~ComViewColUsage()
  {};

  Int64 getViewUID () { return viewUID_; }
  void setViewUID(Int64 viewUID) { viewUID_ = viewUID; }

  Int32 getViewColNumber () { return viewColNumber_; }
  void setViewColNumber(Int32 viewColNumber) { viewColNumber_ = viewColNumber; }

  Int64 getRefdUID () { return refdUID_; }
  void setRefdUID(Int64 refdUID) { refdUID_ = refdUID; }

  Int32 getRefdColNumber () { return refdColNumber_; }
  void setRefdColNumber(Int32 refdColNumber) { refdColNumber_ = refdColNumber; }

  Int32 getRefdObjectType () { return refdObjectType_; }
  void setRefdObjectType(ComObjectType refdObjectType) { refdObjectType_ = refdObjectType; }

  void packUsage (NAString &viewColUsageStr)
  {
      // usage contains 2 int64 and 3 int32, 200 chars is big enough to hold 
      // the string representation
      char buf[200];
      str_sprintf(buf, "viewUID: %ld viewCol: %d refUID: %ld refCol: %d refType: %d;",
                  viewUID_, viewColNumber_,
                  refdUID_, refdColNumber_, refdObjectType_);
      viewColUsageStr = buf;
  }

  void unpackUsage (const char *viewColUsageStr)
  {
      Int32 theRefdObjectType;
      Int32 retcode = sscanf(viewColUsageStr, "viewUID: %Ld viewCol: %d refUID: %Ld refCol: %d refType: %d%*s" ,
             (long long int *)&viewUID_, &viewColNumber_, 
             (long long int *)&refdUID_, &refdColNumber_, &theRefdObjectType);
      assert (retcode == 5);
      refdObjectType_ = (ComObjectType) theRefdObjectType;
  }
};

#endif  // COMVIEWCOLUSAGE_H

