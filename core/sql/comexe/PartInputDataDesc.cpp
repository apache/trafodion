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
* File:         PartInputDataDesc.cpp
* Description:  Data structures related to generating partition input
*               tuples for ESPs and partitioned access nodes.
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

#include "PartInputDataDesc.h"
#include "str.h"
#include "exp_expr.h"
#include "ExpSqlTupp.h"
#include "ExpAtp.h"
#include "ComPackDefs.h"

#include "BaseTypes.h"

// -----------------------------------------------------------------------
// Methods for class ExPartInputDataDesc
// -----------------------------------------------------------------------

ExPartInputDataDesc::ExPartInputDataDesc(ExPartitioningType partType,
					 ex_cri_desc *partInputCriDesc,
					 Lng32 partInputDataLength,
					 Lng32 numPartitions)
  : NAVersionedObject(partType)
{
  partType_            = partType;
  partInputCriDesc_    = partInputCriDesc;
  partInputDataLength_ = partInputDataLength;
  numPartitions_       = numPartitions;
}

// -----------------------------------------------------------------------
// Methods for class ExHashPartInputData
// -----------------------------------------------------------------------
ExHashPartInputData::ExHashPartInputData(
     ex_cri_desc *partInputCriDesc,
     Lng32 numPartitions) : ExPartInputDataDesc(HASH_PARTITIONED,
					       partInputCriDesc,
					       2*sizeof(Lng32),
					       numPartitions)
{
}

void ExHashPartInputData::copyPartInputValue(Lng32 fromPartNum,
					     Lng32 toPartNum,
					     char *buffer,
					     Lng32 bufferLength)
{
  //ex_assert(bufferLength == 2 * sizeof(long),
  //	    "Hash part intput values are always two 4 byte integers");
  str_cpy_all(buffer,(char *) &fromPartNum,sizeof(Lng32));
  str_cpy_all(&buffer[sizeof(Lng32)],(char *) &toPartNum,sizeof(Lng32));
}

// -----------------------------------------------------------------------
// Methods for class ExRoundRobinPartInputData
// -----------------------------------------------------------------------
ExRoundRobinPartInputData::
ExRoundRobinPartInputData(ex_cri_desc *partInputCriDesc,
                          Lng32 numPartitions,
                          Lng32 numOrigRRPartitions)
      : ExPartInputDataDesc(ROUNDROBIN_PARTITIONED,
			    partInputCriDesc,
			    2*sizeof(Lng32),
			    numPartitions),
        numOrigRRPartitions_(numOrigRRPartitions)
{
}

void ExRoundRobinPartInputData::copyPartInputValue(Lng32 fromPartNum,
						   Lng32 toPartNum,
						   char *buffer,
						   Lng32 bufferLength)
{
  Lng32 scaleFactor = numOrigRRPartitions_/ getNumPartitions();
  Lng32 transPoint = numOrigRRPartitions_ % getNumPartitions();

  Lng32 loPart;
  Lng32 hiPart;

  if(fromPartNum < transPoint) {
    loPart = fromPartNum * (scaleFactor + 1);
  } else {
    loPart = (fromPartNum * scaleFactor) + transPoint;
  }

  if(toPartNum < transPoint) {
    hiPart = (toPartNum * (scaleFactor + 1)) + scaleFactor;
  } else {
    hiPart = (toPartNum * scaleFactor) + scaleFactor + transPoint - 1;
  }

  str_cpy_all(buffer,(char *) &loPart,sizeof(Lng32));
  str_cpy_all(&buffer[sizeof(Lng32)],(char *) &hiPart,sizeof(Lng32));
}

// -----------------------------------------------------------------------
// Methods for class ExRangePartInputData
// -----------------------------------------------------------------------

ExRangePartInputData::ExRangePartInputData(
     ex_cri_desc *partInputCriDesc,
     Lng32 partInputDataLength,
     Lng32 partKeyLength,
     Lng32 exclusionIndicatorOffset,
     Lng32 numPartitions,
     Space *space,
     Lng32 useExpressions) : ExPartInputDataDesc(RANGE_PARTITIONED,
						partInputCriDesc,
						partInputDataLength,
						numPartitions)
{
  exclusionIndicatorOffset_      = exclusionIndicatorOffset;
  exclusionIndicatorLength_      = sizeof(Lng32); // fixed for now
  partKeyLength_                 = partKeyLength;
  alignedPartKeyLength_          = (partKeyLength+7)/8 * 8;
  useExpressions_                = useExpressions;
  partRangeExprAtp_              = -1; // may be set later
  partRangeExprAtpIndex_         = -1; // may be set later
  partRangeExprHasBeenEvaluated_ = NOT useExpressions;

  if (numPartitions > 0)
    {
      // allocate space for (numPartitions+1) keys, since n partitions have
      // n+1 boundaries, if one counts the ends
      Lng32 totalPartRangesLength = alignedPartKeyLength_ * (numPartitions+1);
      partRanges_ =
	space->allocateAlignedSpace((size_t) totalPartRangesLength);

      if (useExpressions_)
	{
	  // Expressions are used at run time to compute the boundaries,
	  // the creator of this object will later set those expressions
	  // and (hopefully) set atp and atpindex values. Allocate an
	  // array of expression pointers, one pointer for each boundary.
	  partRangeExpressions_ = new(space) ExExprPtr [numPartitions + 1];

	  // just to be safe, initialize the array with NULLs
	  for (Lng32 i = 0; i < (numPartitions + 1); i++)
	    partRangeExpressions_[i] = NULL;
	}
      else
	{
	  partRangeExpressions_ = (ExExprPtrPtr) NULL;
	}
    }
  else
    {
      // otherwise, this is a fake partitioning data descriptor
      partRanges_           = (NABasicPtr)  NULL;
      partRangeExpressions_ = (ExExprPtrPtr) NULL;
    }
}

ExRangePartInputData::~ExRangePartInputData()
{
}

void ExRangePartInputData::setPartitionStartExpr(Lng32 partNo, ex_expr *expr)
{
  //ex_assert(partNo >= 0 AND partNo <= getNumPartitions() AND useExpressions_,
  //	    "Partition expr. number out of range or desc doesn't use exprs");
  partRangeExpressions_[partNo] = expr;
}

void ExRangePartInputData::setPartitionStartValue(Lng32 partNo,
						  char *val)
{
  //ex_assert(partNo >= 0 AND partNo <= getNumPartitions(),
  //	    "Partition number out of range");
  str_cpy_all(&((char*)partRanges_)[partNo * alignedPartKeyLength_],
	      val,
	      partKeyLength_);
}

void ExRangePartInputData::copyPartInputValue(Lng32 fromPartNum,
					      Lng32 toPartNum,
					      char *buffer,
					      Lng32 bufferLength)
{
  //ex_assert(fromPartNum >= 0 AND
  //	    fromPartNum <= getNumPartitions() AND
  //	    toPartNum >= 0 AND
  //	    toPartNum <= getNumPartitions() AND
  //	    fromPartNum <= toPartNum AND
  //	    bufferLength >= getPartInputDataLength(),
  //	    "Partition number or buffer length out of range");
  //ex_assert(2*partKeyLength_ <= getPartInputDataLength(),
  //	    "Part. input data length must > 2 * key length");
  // copy begin key (entry fromPartNum)
  str_cpy_all(buffer,
	      &((char*)partRanges_)[fromPartNum * alignedPartKeyLength_],
	      partKeyLength_);
  // copy end key (entry toPartNum + 1)
  str_cpy_all(&buffer[partKeyLength_],
	      &((char*)partRanges_)[(toPartNum+1) * alignedPartKeyLength_],
	      partKeyLength_);
  // indicate whether the end key is inclusive or exclusive
  Lng32 exclusive = (toPartNum < getNumPartitions() - 1);
  //ex_assert(exclusionIndicatorLength_ == sizeof(exclusive),
  //	    "Exclusion indicator length must be 4");
  str_cpy_all(&buffer[exclusionIndicatorOffset_],
	      (char *) &exclusive,
              sizeof(exclusive));
}

Lng32 ExRangePartInputData::evalExpressions(Space * space,
					   CollHeap * exHeap,
					   ComDiagsArea **diags)
{
  Lng32 result = 0; // 0 == success

  // return if there is no work to do
  if (getNumPartitions() == 0 OR partRangeExprHasBeenEvaluated_)
    return result;

  // Actually need to evaluate all the expressions and store their
  // results in the partRanges_ byte array.

  // prepare a work atp
  atp_struct *workAtp = allocateAtp(getPartInputCriDesc(), space);
  tupp_descriptor td;

  workAtp->getTupp(partRangeExprAtpIndex_) = &td;

  if (workAtp->getDiagsArea() != *diags)
     workAtp->setDiagsArea(*diags);
  
  // loop over all expressions, fixing them up and evaluating them
  for (Lng32 i = 0; i <= getNumPartitions() AND result == 0; i++)
    {
      Lng32 offs = i * alignedPartKeyLength_;

      workAtp->getTupp(partRangeExprAtpIndex_).setDataPointer
        (&((char*)partRanges_)[offs]);

      partRangeExpressions_[i]->fixup(0,ex_expr::PCODE_NONE,0,space,exHeap,FALSE,NULL);
      if (partRangeExpressions_[i]->eval(workAtp,NULL) == ex_expr::EXPR_ERROR)
	result = -1;
    }

  partRangeExprHasBeenEvaluated_ = (result == 0);
  return result;
}

// -----------------------------------------------------------------------
// Methods for class ExHashDistPartInputData
// -----------------------------------------------------------------------
ExHashDistPartInputData::ExHashDistPartInputData(ex_cri_desc *partInputCriDesc,
                                                 Lng32 numPartitions,
                                                 Lng32 numOrigHashPartitions)
  : ExPartInputDataDesc(HASH1_PARTITIONED,
                        partInputCriDesc,
                        2*sizeof(Lng32),
                        numPartitions),
    numOrigHashPartitions_(numOrigHashPartitions)
{
}

void ExHashDistPartInputData::copyPartInputValue(Lng32 fromPartNum,
                                                 Lng32 toPartNum,
                                                 char *buffer,
                                                 Lng32 bufferLength)
{
  // ex_assert(bufferLength == 2 * sizeof(long),
  //	    "Hash part intput values are always two 4 byte integers");

  Lng32 scaleFactor = numOrigHashPartitions_/ getNumPartitions();
  Lng32 transPoint = numOrigHashPartitions_ % getNumPartitions();

  Lng32 loPart;
  Lng32 hiPart;

  if(fromPartNum < transPoint) {
    loPart = fromPartNum * (scaleFactor + 1);
  } else {
    loPart = (fromPartNum * scaleFactor) + transPoint;
  }

  if(toPartNum < transPoint) {
    hiPart = (toPartNum * (scaleFactor + 1)) + scaleFactor;
  } else {
    hiPart = (toPartNum * scaleFactor) + scaleFactor + transPoint - 1;
  }

  // ex_assert(loPart >= 0 &&
  //           loPart <= hiPart &&
  //           hiPart < numOrigHashPartitions_,
  //           "Hash Dist: invalid input values");

  str_cpy_all(buffer,(char *) &loPart,sizeof(Lng32));
  str_cpy_all(&buffer[sizeof(Lng32)],(char *) &hiPart,sizeof(Lng32));
}

// -----------------------------------------------------------------------
// Methods for class ExHash2PartInputData
// -----------------------------------------------------------------------
ExHash2PartInputData::ExHash2PartInputData(ex_cri_desc *partInputCriDesc,
                                           Lng32 numPartitions)
  : ExPartInputDataDesc(HASH2_PARTITIONED,
                        partInputCriDesc,
                        2*sizeof(Lng32),
                        numPartitions)
{
}

void ExHash2PartInputData::copyPartInputValue(Lng32 fromPartNum,
                                              Lng32 toPartNum,
                                              char *buffer,
                                              Lng32 bufferLength)
{
  Int64 numPartitions = (Int64)getNumPartitions();

  // For hash2, partition numbers are not passed within the partition input
  // values.  Instead, the hash boundaries are passed.  This allows a single
  // ESP to handle tables with different numbers of partitions.
  //
  // Because the integer math of the hash2 split function causes a rounding
  // down, the integer math when determining a hash boundary must round up.
  // This explains why " + numPartitions - 1" is seen in the numerator of
  // the division below.
  ULng32 loHash = (ULng32)((((Int64)fromPartNum << 32)
      + numPartitions - 1) / numPartitions);

  // The hiHash value is one less than the hash boundary for the next
  // partition number.
  ULng32 hiHash = (ULng32)(((((Int64)(toPartNum + 1) << 32)
      + numPartitions - 1) / numPartitions) - 1);

  str_cpy_all(buffer,(char *) &loHash,sizeof(Lng32));
  str_cpy_all(&buffer[sizeof(Lng32)],(char *) &hiHash,sizeof(Lng32));
}

Long ExPartInputDataDesc::pack(void * space)
{
  partInputCriDesc_.pack(space);
  return NAVersionedObject::pack(space);
}

Long ExHashPartInputData::pack(void * space)
{
  return ExPartInputDataDesc::pack(space);
}

Long ExRoundRobinPartInputData::pack(void * space)
{
  return ExPartInputDataDesc::pack(space);
}

Long ExRangePartInputData::pack(void * space)
{
  if(useExpressions_)
  {
    partRangeExpressions_.pack(space,getNumPartitions() + 1);
  }

  partRanges_.pack(space);
  return ExPartInputDataDesc::pack(space);
}

Long ExHashDistPartInputData::pack(void * space)
{
  return ExPartInputDataDesc::pack(space);
}

Long ExHash2PartInputData::pack(void * space)
{
  return ExPartInputDataDesc::pack(space);
}

Lng32 ExPartInputDataDesc::unpack(void * base, void * reallocator)
{
  if(partInputCriDesc_.unpack(base, reallocator)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

Lng32 ExHashPartInputData::unpack(void * base, void * reallocator)
{
  return ExPartInputDataDesc::unpack(base, reallocator);
}

Lng32 ExRoundRobinPartInputData::unpack(void * base, void * reallocator)
{
  return ExPartInputDataDesc::unpack(base, reallocator);
}

Lng32 ExRangePartInputData::unpack(void * base, void * reallocator)
{
  if(useExpressions_)
  {
    if(partRangeExpressions_.unpack(base,getNumPartitions() + 1,reallocator)) return -1;
  }

  if(partRanges_.unpack(base)) return -1;
  return ExPartInputDataDesc::unpack(base, reallocator);
}

Lng32 ExHashDistPartInputData::unpack(void * base, void * reallocator)
{
  return ExPartInputDataDesc::unpack(base, reallocator);
}

Lng32 ExHash2PartInputData::unpack(void * base, void * reallocator)
{
  return ExPartInputDataDesc::unpack(base, reallocator);
}

// -----------------------------------------------------------------------
// This method returns the virtual function table pointer for an object
// with the given class ID; used by NAVersionedObject::driveUnpack().
// -----------------------------------------------------------------------
char *ExPartInputDataDesc::findVTblPtr(short classID)
{
  char *vtblPtr;
  switch (classID)
    {
    case HASH_PARTITIONED:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, ExHashPartInputData);
#pragma warn(1506)  // warning elimination 
      break;
    case RANGE_PARTITIONED:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, ExRangePartInputData);
#pragma warn(1506)  // warning elimination 
      break;
    case ROUNDROBIN_PARTITIONED:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, ExRoundRobinPartInputData);
#pragma warn(1506)  // warning elimination 
      break;
    case HASH1_PARTITIONED:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, ExHashDistPartInputData);
#pragma warn(1506)  // warning elimination 
      break;
    case HASH2_PARTITIONED:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, ExHash2PartInputData);
#pragma warn(1506)  // warning elimination 
      break;
    default:
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblPtr, ExPartInputDataDesc);
#pragma warn(1506)  // warning elimination 
      break;
    }
  return vtblPtr;
}

void ExPartInputDataDesc::fixupVTblPtr()
{
  char * to_vtbl_ptr =  (char *) this;
  char * from_vtbl_ptr;

  switch (partType_)
  {
  case HASH_PARTITIONED:
    {
      ExHashPartInputData partInputDataDesc (NULL,1);
      from_vtbl_ptr = (char *)&partInputDataDesc;
      str_cpy_all(to_vtbl_ptr, from_vtbl_ptr, sizeof(char *));
    }
    break;
  case RANGE_PARTITIONED:
    {
      ExRangePartInputData partInputDataDesc (NULL,0,0,0,0,NULL,0);
      from_vtbl_ptr = (char *)&partInputDataDesc;
      str_cpy_all(to_vtbl_ptr, from_vtbl_ptr, sizeof(char *));
    }
    break;
  case ROUNDROBIN_PARTITIONED:
    {
      ExRoundRobinPartInputData partInputDataDesc (NULL,1,1);
      from_vtbl_ptr = (char *)&partInputDataDesc;
      str_cpy_all(to_vtbl_ptr, from_vtbl_ptr, sizeof(char *));
    }
    break;
  case HASH1_PARTITIONED:
    {
      ExHashDistPartInputData partInputDataDesc (NULL,1,1);
      from_vtbl_ptr = (char *)&partInputDataDesc;
      str_cpy_all(to_vtbl_ptr, from_vtbl_ptr, sizeof(char *));
    }
    break;
  case HASH2_PARTITIONED:
    {
      ExHash2PartInputData partInputDataDesc (NULL,1);
      from_vtbl_ptr = (char *)&partInputDataDesc;
      str_cpy_all(to_vtbl_ptr, from_vtbl_ptr, sizeof(char *));
    }
    break;
  default:
    {
      // ex_assert(0,"Invalid partitioning type");
    }
  }
}


