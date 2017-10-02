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
 *****************************************************************************
 *
 * File:         GenPartFunc.C
 * Description:  Generate code for a Partitioning Function
 * Created:      2/23/96
 * Modified:     $ $Date: 2006/11/01 01:27:56 $ (GMT)
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   PartitioningFunction::codeGen()
//   HashPartitioningFunction::codeGen()
//   HashDistPartitioningFunction::codeGen()
//   Hash2PartitioningFunction::codeGen()
//   RangePartitionBoundaries::completePartitionBoundaries()
//   RangePartitioningFunction::codeGen()
//
//////////////////////////////////////////////////////////////////////


#include "ComOptIncludes.h"
#include "GroupAttr.h"
#include "PartFunc.h"
#include "Generator.h"
#include "GenExpGenerator.h"
//#include "ex_stdh.h"
#include "ExpCriDesc.h"
#include "PartInputDataDesc.h"


/////////////////////////////////////////////////////////
//
// PartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short PartitioningFunction::codeGen(Generator *, Lng32)
{
  ABORT("Need to override PartitioningFunction::codeGen()");
  return 0;
}

/////////////////////////////////////////////////////////
//
// PartitioningFunction::generatePivLayout()
//
/////////////////////////////////////////////////////////

void PartitioningFunction::generatePivLayout(
     Generator *generator,
     Lng32 &partitionInputDataLength,
     Lng32 atp,
     Lng32 atpIndex,
     Attributes ***pivAttrs)
{
  // assign offsets to the PIVs in a standard way

  ExpGenerator *expGen = generator->getExpGenerator();
  
  expGen->processValIdList(
       getPartitionInputValuesLayout(),
       ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
       (ULng32 &) partitionInputDataLength,
       atp,
       atpIndex,
       NULL,
       ExpTupleDesc::SHORT_FORMAT,
       0,
       pivAttrs);
}

/////////////////////////////////////////////////////////
//
// SinglePartitionPartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short SinglePartitionPartitioningFunction::codeGen(Generator *generator,
						   Lng32 partInputDataLength)
{
  GenAssert(partInputDataLength == 0,"Part input values for single part.");

  // there is no object generated for a single part. function
  generator->setGenObj(NULL, NULL);
  return 0;
}

/////////////////////////////////////////////////////////
//
// ReplicateViaBroadcastPartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short ReplicateViaBroadcastPartitioningFunction::codeGen
        (Generator *generator, Lng32 partInputDataLength)
{
  GenAssert(partInputDataLength == 0,"Part input values for replication");

  // there is no object generated for a replication part. function
  generator->setGenObj(NULL, NULL);
  return 0;
}

/////////////////////////////////////////////////////////
//
// ReplicateNoBroadcastPartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short ReplicateNoBroadcastPartitioningFunction::codeGen
        (Generator *generator, Lng32 partInputDataLength)
{
  GenAssert(partInputDataLength == 0,"Part input values for replication");

  // there is no object generated for a replication part. function
  generator->setGenObj(NULL, NULL);
  return 0;
}

/////////////////////////////////////////////////////////
//
// HashPartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short HashPartitioningFunction::codeGen(Generator *generator,
				        Lng32 /*partInputDataLength*/)
{
  // a hash partitioning scheme always produces a partition number as output
  ex_cri_desc *partInputCriDesc =
    new(generator->getSpace()) ex_cri_desc(1,generator->getSpace());
  // GenAssert(partInputDataLength == 4,"NOT partInputDataLength == 4");
  ExHashPartInputData *generatedObject =
    new(generator->getSpace()) ExHashPartInputData(partInputCriDesc,
						   numberOfHashPartitions_);
  generator->setGenObj(NULL,  (ComTdb*)generatedObject);
  return 0;
}

/////////////////////////////////////////////////////////
//
// HashDistPartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short HashDistPartitioningFunction::codeGen(Generator *generator,
                                            Lng32 /*partInputDataLength*/)
{
  // a HashDist partitioning scheme always produces a partition number
  // as output
  //
  ex_cri_desc *partInputCriDesc =
    new(generator->getSpace()) ex_cri_desc(1,generator->getSpace());

  // GenAssert(partInputDataLength == 4,"NOT partInputDataLength == 4");

  ExHashDistPartInputData *generatedObject =
    new(generator->getSpace()) 
    ExHashDistPartInputData(partInputCriDesc,
                            getCountOfPartitions(),
                            getCountOfOrigHashPartitions());

  generator->setGenObj(NULL,  (ComTdb*)generatedObject);
  return 0;
}

/////////////////////////////////////////////////////////
//
// Hash2PartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short Hash2PartitioningFunction::codeGen(Generator *generator,
                                            Lng32 /*partInputDataLength*/)
{
  ex_cri_desc *partInputCriDesc =
    new(generator->getSpace()) ex_cri_desc(1,generator->getSpace());

  // GenAssert(partInputDataLength == 4,"NOT partInputDataLength == 4");

  ExHash2PartInputData *generatedObject = new(generator->getSpace()) 
      ExHash2PartInputData(partInputCriDesc,
                           getCountOfPartitions());

  generator->setGenObj(NULL, (ComTdb*)generatedObject);
  return 0;
}

short SkewedDataPartitioningFunction::codeGen(Generator *generator,
                                            Lng32 partInputDataLength)
{
  return partialPartFunc_ -> codeGen(generator, partInputDataLength);
}


/////////////////////////////////////////////////////////
//
// LogPhysPartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short LogPhysPartitioningFunction::codeGen(Generator * /*generator*/,
					   Lng32 /*partInputDataLength*/)
{
  GenAssert(0,"Should not generate code for logphys part func.");
  return 0;
}

/////////////////////////////////////////////////////////
//
// RoundRobinPartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short RoundRobinPartitioningFunction::codeGen(Generator *generator,
					      Lng32 /*partInputDataLength*/)
{
  // A round-robin partitioning function produces a partition
  // number as output, for now at least.  
  //
  Space *space = generator->getSpace();

  ex_cri_desc *partInputCriDesc = new (space) ex_cri_desc(1, space);

  ExRoundRobinPartInputData *generatedObject =
    new (space) ExRoundRobinPartInputData(partInputCriDesc,
                                          getCountOfPartitions(),
                                          getCountOfOrigRRPartitions());

  generator->setGenObj(NULL, (ComTdb*)generatedObject);

  return 0;
}

/////////////////////////////////////////////////////////
//
// RangePartitioningFunction::codeGen()
//
/////////////////////////////////////////////////////////

short RangePartitioningFunction::codeGen(Generator *generator,
					 Lng32 partInputDataLength)
{
  ExpGenerator         * exp_gen = generator->getExpGenerator();
  Lng32                 myOwnPartInputDataLength;

  const Int32            pivMoveAtp = 0; // only one atp is used for this expr
  const Int32            pivMoveAtpIndex = 2; // 0: consts, 1: temps, 2: result
  const ExpTupleDesc::TupleDataFormat pivFormat = // format of PIVs
                          ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  ex_cri_desc          *partInputCriDesc = new(generator->getSpace())
                           ex_cri_desc(pivMoveAtpIndex+1,
				       generator->getSpace());
  ExpTupleDesc         *partInputTupleDesc;
  ExRangePartInputData *generatedObject = NULL;

  // get the list of partition input variables
  ValueIdList          piv(getPartitionInputValuesLayout());

  CollIndex numPartInputs = piv.entries();
  CollIndex numPartKeyCols = (numPartInputs - 1) / 2;
  // the number of partition input variables must be odd
  GenAssert(2*numPartKeyCols+1 == numPartInputs,
	    "NOT 2*numPartKeyCols+1 == numPartInputs");

  Attributes **begEndAttrs;
  Int32 alignedPartKeyLen;

  // make a layout of the partition input data record
  generatePivLayout(
       generator,
       myOwnPartInputDataLength,
       pivMoveAtp,
       pivMoveAtpIndex,
       &begEndAttrs);
  
  // the aligned part key length is where the end key values start
  alignedPartKeyLen = (Int32) begEndAttrs[numPartKeyCols]->getOffset();

  if (begEndAttrs[numPartKeyCols]->getNullIndicatorLength() > 0)
    alignedPartKeyLen = MINOF(
	 alignedPartKeyLen,
	 (Int32)begEndAttrs[numPartKeyCols]->getNullIndOffset());
    
  if (begEndAttrs[numPartKeyCols]->getVCIndicatorLength() > 0)
    alignedPartKeyLen = MINOF(
	 alignedPartKeyLen,
	 begEndAttrs[numPartKeyCols]->getVCLenIndOffset());
    
  // generate a tuple desc for the whole PIV record and a cri desc
  partInputTupleDesc = new(generator->getSpace()) ExpTupleDesc(
       numPartInputs,
       begEndAttrs,
       myOwnPartInputDataLength,
       pivFormat,
       ExpTupleDesc::LONG_FORMAT,
       generator->getSpace());
  partInputCriDesc->setTupleDescriptor(pivMoveAtpIndex,partInputTupleDesc);

  // make sure we fulfill the assertions we made

  // optimizer and generator should agree on the part input data length
  GenAssert(partInputDataLength == (Lng32) myOwnPartInputDataLength,
	    "NOT partInputDataLength == myOwnPartInputDataLength");
  // the length of the begin key and the end key must be the same
  // (compare offsets of their last fields)
// Commented out because this check does not work. The check needs
// to compute the LENGTH of each key field, by subtracting the current
// offset from the next offset, taking into account varchar length
// and null indicator fields (which are not part of the length but
// increase the offset).
//GenAssert(begEndAttrs[numPartKeyCols-1]->getOffset() + alignedPartKeyLen ==
//  begEndAttrs[2*numPartKeyCols-1]->getOffset(),
//    "begin/end piv keys have different layouts");

  generatedObject = new(generator->getSpace()) ExRangePartInputData(
       partInputCriDesc,
       partInputDataLength,
       alignedPartKeyLen, //len of one part key + filler
       begEndAttrs[numPartInputs-1]->getOffset(),//offset of last field
       getCountOfPartitions(),
       generator->getSpace(),
       TRUE); // uses expressions to calculate ranges in the executor
  generatedObject->setPartitionExprAtp(pivMoveAtp);
  generatedObject->setPartitionExprAtpIndex(pivMoveAtpIndex);

  // now fill in the individual partition boundaries
  // (NOTE: there is one more than there are partitions)
  ULng32 boundaryDataLength = 0;
  for (Lng32 i = 0; i <= getCountOfPartitions(); i++)
    {
      const ItemExprList *iel = partitionBoundaries_->getBoundaryValues(i);
      ex_expr * generatedExpr = NULL;

      ValueIdList boundaryColValues;
      ULng32 checkedBoundaryLength;

      // convert the ItemExpressionList iel into a ValueIdList
      for (CollIndex kc = 0; kc < iel->entries(); kc++)
	{
	  ItemExpr *boundaryVal = (*iel)[kc];

	  // create a cast node to convert the boundary value to the
	  // data type of the column
	  ItemExpr *castBoundaryVal =
	    new(generator->wHeap()) Cast(boundaryVal,&piv[kc].getType());

	  castBoundaryVal->bindNode(generator->getBindWA());

	  boundaryColValues.insert(castBoundaryVal->getValueId());
	}

      // Now generate a contiguous move expression. Only for the first time
      // generate a tuple desc, since all tuples should be the same.
      exp_gen->generateContiguousMoveExpr(
	   boundaryColValues,
	   0, // cast nodes created above will do the move, no conv nodes
	   pivMoveAtp,
	   pivMoveAtpIndex,
	   pivFormat,
	   checkedBoundaryLength,
	   &generatedExpr);

      if (i == 0)
	{
	  // first time set the actual part key data length
	  boundaryDataLength = checkedBoundaryLength;
	}
      else
	{
	  // all boundary values (piv tuples) must have the same layout
	  // and therefore the same length
	  GenAssert(boundaryDataLength == checkedBoundaryLength,
		    "Partition boundary tuple layout mismatch");
	}

      generatedObject->setPartitionStartExpr(i,generatedExpr);
    }

  NADELETEBASIC(begEndAttrs, generator->wHeap());
  generator->setGenObj(NULL, (ComTdb*)generatedObject);
  return 0;
}

void RangePartitioningFunction::generatePivLayout(
     Generator *generator,
     Lng32 &partitionInputDataLength,
     Lng32 atp,
     Lng32 atpIndex,
     Attributes ***pivAttrs)
{
  // Make a layout of the partition input data record such that
  // begin and end key are aligned in the same way.
  // (layout = ((beg. key) (filler1) (end key) (filler2) (exclusion flag)))

  ExpGenerator *expGen = generator->getExpGenerator();
  CollIndex numPartInputs = getPartitionInputValuesLayout().entries();
  CollIndex numPartKeyCols = (numPartInputs - 1) / 2;
  // the number of partition input variables must be odd
  GenAssert(2*numPartKeyCols+1 == numPartInputs,
	    "NOT 2*numPartKeyCols+1 == numPartInputs");


  // ---------------------------------------------------------------------
  // Start by processing the begin key PIVs
  // ---------------------------------------------------------------------
  ValueIdList partialPivs;
  Attributes **returnedAttrs = NULL;
  Attributes **localPartialAttrs;
  Lng32 maxAlignment = 1;
  Lng32 alignedPartKeyLen;

  if (pivAttrs)
    {
      returnedAttrs = new(generator->wHeap()) Attributes *[numPartInputs];
    }

  CollIndex i = 0;
  for (i = 0; i < numPartKeyCols; i++)
    partialPivs.insert(getPartitionInputValuesLayout()[i]);
  
  expGen->processValIdList(
       partialPivs,
       ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
       (ULng32 &) partitionInputDataLength,
       atp,
       atpIndex,
       NULL,
       ExpTupleDesc::SHORT_FORMAT,
       0,
       &localPartialAttrs);

  if (returnedAttrs)
    for (i = 0; i < numPartKeyCols; i++)
      returnedAttrs[i] = localPartialAttrs[i];

  // ---------------------------------------------------------------------
  // Now find out the max. alignment that is needed in the begin key,
  // make sure that the end key starts on an offset that is a
  // multiple of the max. alignment in the partition input values
  // ---------------------------------------------------------------------
  for (i = 0; i < numPartKeyCols; i++)
    {
      if (localPartialAttrs[i]->getDataAlignmentSize() > maxAlignment)
	maxAlignment = localPartialAttrs[i]->getDataAlignmentSize();
      if (localPartialAttrs[i]->getVCIndicatorLength() > maxAlignment)
	maxAlignment = localPartialAttrs[i]->getVCIndicatorLength();
      if (localPartialAttrs[i]->getNullIndicatorLength() > maxAlignment)
	maxAlignment = localPartialAttrs[i]->getNullIndicatorLength();
    }

  alignedPartKeyLen = partitionInputDataLength;
  while (alignedPartKeyLen % maxAlignment != 0)
    alignedPartKeyLen++;

  // ---------------------------------------------------------------------
  // Now that we are starting on a good offset, process the end key
  // ---------------------------------------------------------------------
  partialPivs.clear();
  for (i = numPartKeyCols; i < numPartInputs-1; i++)
    partialPivs.insert(getPartitionInputValuesLayout()[i]);
  
  expGen->processValIdList(
       partialPivs,
       ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
       (ULng32 &) partitionInputDataLength,
       atp,
       atpIndex,
       NULL,
       ExpTupleDesc::SHORT_FORMAT,
       alignedPartKeyLen,
       &localPartialAttrs);

  if (returnedAttrs)
    for (i = numPartKeyCols; i < numPartInputs-1; i++)
      returnedAttrs[i] = localPartialAttrs[i-numPartKeyCols];

  // ---------------------------------------------------------------------
  // Process the exclusion flag at offset 2*alignedPartKeyLen
  // ---------------------------------------------------------------------
  partialPivs.clear();
  partialPivs.insert(getPartitionInputValuesLayout()[numPartInputs-1]);
  
  expGen->processValIdList(
       partialPivs,
       ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
       (ULng32 &) partitionInputDataLength,
       atp,
       atpIndex,
       NULL,
       ExpTupleDesc::SHORT_FORMAT,
       2*alignedPartKeyLen,
       &localPartialAttrs);

  // set up return values

  if (returnedAttrs)
    returnedAttrs[numPartInputs-1] = localPartialAttrs[0];

  partitionInputDataLength += 2*alignedPartKeyLen;

  if (pivAttrs)
    {
      *pivAttrs = returnedAttrs;
    }
  else
    {
      NADELETEBASIC(returnedAttrs, generator->wHeap());
    }
}
