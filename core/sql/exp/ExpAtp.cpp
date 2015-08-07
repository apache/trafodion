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
* File:         ExpAtp.cpp (previously /executor/ex_atp.cpp)
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


#include "ExpAtp.h"
#include "ComPackDefs.h"

// constructor (Allocate and initialize) for an Atp
NA_EIDPROC SQLEXP_LIB_FUNC
atp_struct * allocateAtp(ex_cri_desc * criDesc, CollHeap * space)
{
  // Allocate space for the atp_struct (which has room for one tupp in
  // the tupp array) plus room for the remainder of the tupps.
  //
#pragma nowarn(1506)  // warning elimination
  Int32 atp_size = sizeof(atp_struct) + (criDesc->noTuples()-1) * sizeof(tupp);
#pragma warn(1506)  // warning elimination

  atp_struct * atp;
  atp = (atp_struct *) space->allocateMemory(atp_size);
  
  // set the pointer to the descriptor
  atp->setCriDesc(criDesc);

  // initialize tag data member.
  atp->set_tag(0 /* FALSE*/);  

  // no errors yet.
  atp->initDiagsArea(0);
  
  // Initialize each tupp.
  //
  Lng32 j;
  for(j=0; j<criDesc->noTuples(); j++)
  {
    atp->getTupp(j).init();
  }
  
  return atp;
}

// Create an atp inside a pre-allocated buffer instead of allocating it from
// a space object. Used by ExpDP2Expr.
NA_EIDPROC SQLEXP_LIB_FUNC
atp_struct * createAtpInBuffer(ex_cri_desc * criDesc, char * & buf)
{
  atp_struct * atp = (atp_struct *)buf;
  // set the pointer to the descriptor
  atp->setCriDesc(criDesc);

  // initialize tag data member.
  atp->set_tag(0 /* FALSE*/);

  // no errors yet.
  atp->initDiagsArea(0);

  // Initialize each tupp.
  //
  Lng32 j;
  for(j=0; j<criDesc->noTuples(); j++)
  {
    atp->getTupp(j).init();
  }

  buf += sizeof(atp_struct) + (criDesc->noTuples() -1 ) * sizeof(tupp);
  return atp;
}

NA_EIDPROC SQLEXP_LIB_FUNC
atp_struct * allocateAtp(Lng32 numTuples, CollHeap * space)
{
  // Allocate space for the atp_struct (which has room for one tupp in
  // the tupp array) plus room for the remainder of the tupps.
  //
#pragma nowarn(1506)  // warning elimination
  Int32 atp_size = sizeof(atp_struct) + (numTuples-1) * sizeof(tupp);
#pragma warn(1506)  // warning elimination

  atp_struct * atp;
  atp = (atp_struct *) space->allocateMemory(atp_size);
  
  // set the pointer to the descriptor
  atp->setCriDesc(NULL);

  // initialize tag data member.
  atp->set_tag(0 /* FALSE*/);  

  // no errors yet.
  atp->initDiagsArea(0);
  
  // Initialize each tupp.
  //
  Lng32 j;
  for(j=0; j<numTuples; j++)
  {
    atp->getTupp(j).init();
  }
  
  return atp;
}

NA_EIDPROC SQLEXP_LIB_FUNC
void deallocateAtp(atp_struct * atp, CollHeap * space)
{
  if (space)
    space->deallocateMemory((char *)atp);
}

NA_EIDPROC SQLEXP_LIB_FUNC
atp_struct * allocateAtpArray(
     ex_cri_desc * criDesc, Lng32 cnt, Int32 *atpSize, CollHeap * space, NABoolean failureIsFatal)
{
  const Lng32 numTuples = criDesc->noTuples();

  // Alocate space for the atp_struct (which has room for one tupp in
  // the tupp array) plus room for the remainder of the tupps,
  // times the count requested.
#pragma nowarn(1506)   // warning elimination 
  *atpSize = sizeof(atp_struct) + numTuples * sizeof(tupp);
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
#pragma warn(1506)  // warning elimination 
  char *atpSpace = (char *) space->allocateMemory(*atpSize * cnt, failureIsFatal);

  if (atpSpace == NULL)
    return NULL;

  atp_struct * atpStart = (atp_struct *) atpSpace;
  atp_struct * atp;

  for (Lng32 i = 0; i < cnt; i++)
    {
      atp = (atp_struct *) atpSpace;
      atpSpace += *atpSize;

      // set the pointer to the descriptor
      atp->setCriDesc(criDesc);

      // initialize tag data member.
      atp->set_tag(0 /* FALSE*/);

      // no errors yet.
      atp->initDiagsArea(0);

      // Initialize each tupp.
      //
      for(Lng32 j=0; j < numTuples; j++)
        {
          atp->getTupp(j).init();
        }
    }
  return atpStart;
}

NA_EIDPROC SQLEXP_LIB_FUNC
void deallocateAtpArray(atp_struct ** atp, CollHeap * space)
{
  if (space)
    space->deallocateMemory((char *)*atp);
}

Long atp_struct::pack(void * space)
{
  for (Int32 i = 0; i < criDesc_->noTuples(); i++)
  {
    tuppArray_[i].pack(space);
  }

  criDesc_ = (ex_cri_desc *)(criDesc_->drivePack(space));
  return ((Space *)space)->convertToOffset(this);
}

Lng32 atp_struct::unpack(Lng32 base)
{
  ex_cri_desc obj;

  if (criDesc_)
  {
    criDesc_ = (ex_cri_desc *)CONVERT_TO_PTR(criDesc_, base);
    if (criDesc_->driveUnpack((void *)base,&obj,NULL) == NULL) return -1;
    for (Int32 i = 0; i < criDesc_->noTuples(); i++)
    {
      tuppArray_[i].unpack(base);
    }
  }
  return 0;
}



