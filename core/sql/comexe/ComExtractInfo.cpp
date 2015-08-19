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

/*
 * File:         ComExtractInfo.cpp
 * Description:  NAVersionedObject subclasses to store information in plans
 *               specific to parallel extract producer and consumer queries.
 * Created:      May 2007
 * Language:     C++
 *
 */

#include "ComExtractInfo.h"

Long ComExtractProducerInfo::pack(void *space)
{
  securityKey_.pack(space);
  return NAVersionedObject::pack(space);
}

Lng32 ComExtractProducerInfo::unpack(void *base, void *reallocator)
{
  if (securityKey_.unpack(base)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

Long ComExtractConsumerInfo::pack(void *space)
{
  espPhandle_.pack(space);
  securityKey_.pack(space);
  return NAVersionedObject::pack(space);
}

Lng32 ComExtractConsumerInfo::unpack(void *base, void *reallocator)
{
  if (espPhandle_.unpack(base)) return -1;
  if (securityKey_.unpack(base)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}
