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
* File:         ComKeyRange.cpp
* Description:  Implements methods for the base classes of key range objects
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

// -----------------------------------------------------------------------

#include "ExpCriDesc.h"
#include "ComPackDefs.h"
#include "ComKeyRange.h"
#include "ComKeyMDAM.h"
#include "ComKeySingleSubset.h"


keyRangeGen::keyRangeGen(key_type keyType,
	       ULng32 keyLen,
	       ex_cri_desc *workCriDesc,
	       unsigned short keyValuesAtpIndex,
	       unsigned short excludeFlagAtpIndex,
	       unsigned short dataConvErrorFlagAtpIndex) :
     keyType_(keyType),
     keyLength_(keyLen),
     workCriDesc_(workCriDesc),
     keyValuesAtpIndex_(keyValuesAtpIndex),
     excludeFlagAtpIndex_(excludeFlagAtpIndex),
     dataConvErrorFlagAtpIndex_(dataConvErrorFlagAtpIndex),
     NAVersionedObject(keyType),
     keytag_(0),
     flags_(0)
{
  for (Int32 i = 0; i < FILLER_LENGTH; i++)
    fillersKeyRangeGen_[i] = '\0';
}

// -----------------------------------------------------------------------
// This method returns the virtual function table pointer for an object
// with the given class ID; used by NAVersionedObject::driveUnpack().
// -----------------------------------------------------------------------
char *keyRangeGen::findVTblPtr(short classID)
{
  char *vtblPtr;
  switch (classID)
  {
    case KEYSINGLESUBSET:
      GetVTblPtr(vtblPtr, keySingleSubsetGen);
      break;
    case KEYMDAM:
      GetVTblPtr(vtblPtr, keyMdamGen);
      break;
    default:
      GetVTblPtr(vtblPtr, keyRangeGen);
      break;
  }
  return vtblPtr;
}

void keyRangeGen::fixupVTblPtr()
{
  switch (getType())
    {
    case KEYSINGLESUBSET:
      {
        keySingleSubsetGen bek;

        COPY_KEY_VTBL_PTR((char *)&bek, (char *)this);
      }
      break;
    case KEYMDAM:
      {
        keyMdamGen mdam;

        COPY_KEY_VTBL_PTR((char *)&mdam, (char *)this);
      }
      break;
    default:
      break;
    }
}

Long keyRangeGen::pack(void * space)
{
  workCriDesc_.pack(space);
  return NAVersionedObject::pack(space);
}


Lng32 keyRangeGen::unpack(void * base, void * reallocator)
{  
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

