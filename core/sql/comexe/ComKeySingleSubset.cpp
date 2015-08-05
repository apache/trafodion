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
* File:         ComKeySingleSubset.cpp
* Description:  
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

#include "exp_expr.h"
#include "ComPackDefs.h"
#include "ComKeyRange.h"
#include "ComKeySingleSubset.h"

keySingleSubsetGen::keySingleSubsetGen(ULng32 keyLen,
				       ex_cri_desc * workCriDesc,
				       unsigned short keyValuesAtpIndex,
				       unsigned short excludeFlagAtpIndex,
				       unsigned short dataConvErrorFlagIndex,
				       ex_expr * bk_pred,
				       ex_expr * ek_pred,
				       ex_expr * bkey_excluded_expr,
				       ex_expr * ekey_excluded_expr,
				       short bkey_excluded,
				       short ekey_excluded) :
     keyRangeGen(KEYSINGLESUBSET,
	    keyLen,
	    workCriDesc,
	    keyValuesAtpIndex,
	    excludeFlagAtpIndex,
	    dataConvErrorFlagIndex),
     bkPred_(bk_pred),ekPred_(ek_pred),
     lowKeyExcludedExpr_(bkey_excluded_expr),
     highKeyExcludedExpr_(ekey_excluded_expr),
     lowKeyExcluded_(bkey_excluded),
     highKeyExcluded_(ekey_excluded)
{ };

keySingleSubsetGen::~keySingleSubsetGen()
{ };

Long keySingleSubsetGen::pack(void *space)
{  
  bkPred_.pack(space);
  ekPred_.pack(space);
  lowKeyExcludedExpr_.pack(space);
  highKeyExcludedExpr_.pack(space);
  return keyRangeGen::pack(space);
};

Lng32 keySingleSubsetGen::unpack(void * base, void * reallocator)
{  
  if(bkPred_.unpack(base, reallocator)) return -1;
  if(ekPred_.unpack(base, reallocator)) return -1;
  if(lowKeyExcludedExpr_.unpack(base, reallocator)) return -1;
  if(highKeyExcludedExpr_.unpack(base, reallocator)) return -1;
  return keyRangeGen::unpack(base, reallocator);
};


ex_expr* keySingleSubsetGen::getExpressionNode(Int32 pos)
{
if (pos == 0)
  return bkPred_;
else if (pos == 1)
  return ekPred_;
else
  return NULL;
}

  

