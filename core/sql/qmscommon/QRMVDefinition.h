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

#ifndef _QRMVDEFINITION_H_
#define _QRMVDEFINITION_H_

#include "NAString.h"
#include "NABoolean.h"

class QRMVDefinition
{
public:
  QRMVDefinition(CollHeap* heap)
    : redefTimeString_(heap),
      refreshedTimeString_(heap)
  {
  }

  QRMVDefinition(NABoolean  hasIgnoreChanges,
                 Int64	    redefTime,
                 Int64	    refreshedTime,
                 Int64	    objectUID,
                 CollHeap*  heap)
   :  hasIgnoreChanges_(hasIgnoreChanges),
      redefTime_(redefTime),
      refreshedTime_(refreshedTime),
      objectUID_(objectUID),
      redefTimeString_(heap),
      refreshedTimeString_(heap)
  {
    refreshedTimeString_ = Int64ToNAString(refreshedTime);
    redefTimeString_     = Int64ToNAString(redefTime);
  }

  NABoolean hasIgnoreChanges_;
  NAString  redefTimeString_;
  Int64	    redefTime_;
  NAString  refreshedTimeString_;
  Int64	    refreshedTime_;
  Int64	    objectUID_;
};

#endif  // _QRMVDEFINITION_H_
