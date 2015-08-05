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
****************************************************************************
*
* File:         UdrTableDescInfo.h
* Description:  Metadata for child table descriptor info of a TMUDF tcb
* Created:      2/10/2010
* Language:     C++
*
****************************************************************************
*/



#include "udrtabledescinfo.h"


//
// Constructor
//

UdrTableDescInfo::UdrTableDescInfo(const char *corrName,
				   Int16 numColumns,
                                   UInt32 outputRowLength,
                                   UInt16 outputTuppIndex,
				   UdrColumnDescInfoPtrPtr colDescList)
  : NAVersionedObject(-1)
{
  corrName_ = corrName;
  numColumns_ = numColumns;
  outputRowLen_ = outputRowLength;
  outputTuppIndex_ = outputTuppIndex;
  colDescList_ = colDescList;
}

Long UdrTableDescInfo::pack(void *space)
{
  corrName_.pack(space);
  if (numColumns_ >0)
    colDescList_.pack(space,numColumns_);
  return NAVersionedObject::pack(space);
}

Lng32 UdrTableDescInfo::unpack(void *base, void *reallocator)
{
  if (corrName_.unpack(base)) return -1;
  if (colDescList_.unpack(base,numColumns_,reallocator)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}
