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
* File:         UdrFormalParamInfo.cpp
* Description:  Metadata for UDR parameters
* Created:      10/10/2000
* Language:     C++
*
****************************************************************************
*/

#include "UdrFormalParamInfo.h"

UdrFormalParamInfo::UdrFormalParamInfo(Int16 type,
                                       Int16 precision,
                                       Int16 scale,
                                       Int16 flags,
                                       Int16 encodingCharSet,
                                       Int16 collation,
                                       char *paramName)
  : NAVersionedObject(-1),
    flags_(flags),
    type_(type),
    precision_(precision),
    scale_(scale),
    encodingCharSet_(encodingCharSet),
    collation_(collation),
    paramName_(paramName)
{
}

Long UdrFormalParamInfo::pack(void *space)
{
  paramName_.pack(space);
  return NAVersionedObject::pack(space);
}

Lng32 UdrFormalParamInfo::unpack(void *base, void *reallocator)
{
  if (paramName_.unpack(base)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}
