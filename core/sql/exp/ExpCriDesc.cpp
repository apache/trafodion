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
* File:         ExpCriDesc.cpp (previously /executor/ex_cri_desc.cpp)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#include "Platform.h"


#include "ComPackDefs.h"
#include "ExpCriDesc.h"
#include "ComSpace.h"
#include "exp_tuple_desc.h"
#include <iostream>


ex_cri_desc::ex_cri_desc(const unsigned short numTuples, void * space_) :
  numTuples_(numTuples), NAVersionedObject(-1)
{
  tupleDesc_ = (ExpTupleDescPtr *)
    (((Space *)space_)->allocateAlignedSpace
     (numTuples_ * sizeof(ExpTupleDescPtr)));

  unsigned short i;
  for (i=0; i< numTuples; i++)
    {
      tupleDesc_[i] = (ExpTupleDescPtrPtr)NULL;
    };

  flags_ = 0;
};

Long ex_cri_desc::pack(void * space)
{
  if ( ! (flags_ & PACKED)) // REVISIT
    {
      tupleDesc_.pack(space, numTuples_);
      flags_ |= PACKED;
    }
  return NAVersionedObject::pack(space);
}

Lng32 ex_cri_desc::unpack(void * base, void * reallocator)
{
  if (flags_ & PACKED) // REVISIT
    {
      if (tupleDesc_.unpack(base, numTuples_, reallocator)) return -1;
      flags_ &= ~PACKED;
    }
  return NAVersionedObject::unpack(base, reallocator);
}

void ex_cri_desc::display(const char* title)
{
   cout << title << endl;

   unsigned short tuples = noTuples();
   char buf[100];
   for (Int32 i=0; i<tuples; i++) {
      ExpTupleDesc* tDesc = getTupleDescriptor(i);
      if ( tDesc ) {
         sprintf(buf, "%dth tuple:", i);
         tDesc->display(buf);
      } else
         cout << i << "th tuple's TupleDesc is NULL" << endl;
   }
}

