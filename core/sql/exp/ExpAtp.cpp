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
#include "exp_attrs.h"
#include "str.h"

// constructor (Allocate and initialize) for an Atp
atp_struct * allocateAtp(ex_cri_desc * criDesc, CollHeap * space)
{
  // Allocate space for the atp_struct (which has room for one tupp in
  // the tupp array) plus room for the remainder of the tupps.
  //
  Int32 atp_size = sizeof(atp_struct) + (criDesc->noTuples()-1) * sizeof(tupp);

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

atp_struct * allocateAtp(Lng32 numTuples, CollHeap * space)
{
  // Allocate space for the atp_struct (which has room for one tupp in
  // the tupp array) plus room for the remainder of the tupps.
  //
  Int32 atp_size = sizeof(atp_struct) + (numTuples-1) * sizeof(tupp);

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

void deallocateAtp(atp_struct * atp, CollHeap * space)
{
  if (space)
    space->deallocateMemory((char *)atp);
}

atp_struct * allocateAtpArray(
     ex_cri_desc * criDesc, Lng32 cnt, Int32 *atpSize, CollHeap * space, NABoolean failureIsFatal)
{
  const Lng32 numTuples = criDesc->noTuples();

  // Alocate space for the atp_struct (which has room for one tupp in
  // the tupp array) plus room for the remainder of the tupps,
  // times the count requested.
  *atpSize = sizeof(atp_struct) + numTuples * sizeof(tupp);
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

void atp_struct::display(const char* title, ex_cri_desc* cri)
{
   cout << title << endl;
   if (!cri)
      cri = getCriDesc();

   char subtitle[100];
   unsigned short tuples = numTuples();
   for (Int32 i=0; i<tuples; i++) {
      ExpTupleDesc* tDesc = cri->getTupleDescriptor(i);

      if ( tDesc && tDesc->attrs() ) {

         tupp& tup = getTupp(i);

         // get the pointer to the composite row
         char* dataPtr = tup.getDataPointer();

         //cout << "dataPtr=" << dataPtr << endl;

         UInt32 attrs = tDesc->numAttrs();
         for (Int32 j=0; j<attrs; j++) {
             Attributes* attr = tDesc->getAttr(j);
             Int16 dt = attr->getDatatype();
             UInt32 len = attr->getLength();
             
             NABoolean isVarChar = attr->getVCIndicatorLength() > 0;

             char* realDataPtr = dataPtr;
             if ( isVarChar ) {  
                realDataPtr += attr->getVCIndicatorLength();
             } 

             sprintf(subtitle, "%dth field: ", j);
             print(subtitle, dt, realDataPtr, len);

             dataPtr = realDataPtr + len;
         }
      }
   }
   cout << endl;
}

void atp_struct::print(char* title, Int16 dt, char* ptr, UInt32 len)
{
   cout << title << "datatype=" << dt << ", len=" << len << ", data=\"";
   switch (dt) {
      case REC_DECIMAL_LSE: 
        cout << ptr ;
        //printBrief(ptr, len);
        break;

      case REC_BYTE_V_ASCII: 
        cout << ptr ;
        break;

      case REC_BYTE_F_ASCII: 
        {
           for (Int32 i=0; i<len; i++)
             cout << ptr[i];
        }
        break;

      default:
         cout << "unimplemented, skip for now";
        break;
   }
   cout << "\"" << endl;
}

