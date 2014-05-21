/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
****************************************************************************
*
* File:         ComTdbSequenceGenerator.cpp
* Description:
*
* Created:      03/04/2008
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbSequenceGenerator.h"

// Default Constructor.  This is used by the ComTdb::fixupVTblPtr()
// routine which fixes up the virtual function table pointer after
// a node has been unpacked.
ComTdbSequenceGenerator::ComTdbSequenceGenerator() 
: ComTdb(ComTdb::ex_SEQGEN, eye_SEQGEN),
  dstTuppIndex_(0),
  sgAttributes_(NULL)
{
  
}

// Constructor used by the generator SequenceGenerator::codegen to
// generate the sequence generator TDB.
//
// Arguements:
//
//   criDescParentDown - pointer to the criDesc for the parent down queue.
//   A descriptor of the request recieved from the parent.
//
//   criDescParentUp - pointer to the criDesc for the parent up queue.
//   A descriptor of the replies given to the parent.
//
//   queueSizeDown - Size of the down queue. Must be a power of two.
//
//   queueSizeUp - Size of the up queue. Must be a power of two.
//
//   tuppIndex - Index into the ATP of the parent queue which will
//   contain the Sequence Value.
//

ComTdbSequenceGenerator::ComTdbSequenceGenerator(ComTdb *childTdb,
                                                 ex_cri_desc *criDescParentDown,
                                                 ex_cri_desc *criDescParentUp,
                                                 queue_index queueSizeDown,
                                                 queue_index queueSizeUp,
                                                 const unsigned short dstTuppIndex,
 						 ComSequenceGeneratorAttributes *sgAttributes,
                                                 const UInt32 sgCache,
				                 const UInt32 sgCacheInitial,
				                 const UInt32 sgCacheIncrement,
				                 const UInt32 sgCacheMaximum,
                                                 const UInt32 sgCacheRetry,
                                                 Lng32 numBuffers,
                                                 ULng32 bufferSize)
  : ComTdb(ComTdb::ex_SEQGEN,
	   eye_SEQGEN,
	   (Cardinality) 1.0,
	   criDescParentDown,
	   criDescParentUp,
	   queueSizeDown,
	   queueSizeUp,
	   numBuffers,
	   bufferSize),
    childTdb_(childTdb),
    dstTuppIndex_(dstTuppIndex),
    sgAttributes_(sgAttributes),
    sgCache_(sgCache),
    sgCacheInitial_(sgCacheInitial),
    sgCacheIncrement_(sgCacheIncrement),
    sgCacheMaximum_(sgCacheMaximum),
    sgCacheRetry_(sgCacheRetry)
{
}

// Used by GUI, does nothing now.
void
ComTdbSequenceGenerator::displayContents(Space *space, ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

// The sequence generator TDB has no children
Int32
ComTdbSequenceGenerator::numChildren() const
{
  return(1);
}
 
// Return the number of expressions held by the sequence generator TDB (0)
Int32
ComTdbSequenceGenerator::numExpressions() const
{
  return(0); //
}

// Exclude this code from coverage analysis since there are no expressions.
// This code could be deleted since it is basically the same as the base implementation.
// LCOV_EXCL_START
// Return the expression names of the sequence generator TDB.
const char *
ComTdbSequenceGenerator::getExpressionName(Int32 expNum) const
{
  switch(expNum)
    {
    case 0:
      assert(1); // We should not get here
      return NULL;
    default:
      return NULL;  // no expressions in SequenceGenerator
    }
}

// Return the expressions of the sequence generator TDB
ex_expr *
ComTdbSequenceGenerator::getExpressionNode(Int32 expNum)
{
  switch(expNum)
    {
    case 0:
      assert(1); // We should not get here
      return NULL;
    default:
      return NULL;
    }

}
// LCOV_EXCL_STOP

// Pack the sequenceGeneratorTdb: Convert all pointers to offsets relative
// to the space object.
Long ComTdbSequenceGenerator::pack(void * space)
{
  // Pack the child TDB, (this calls the pack() method on the child.
  //
  childTdb_.pack(space);
  sgAttributes_.pack(space);

  return ComTdb::pack(space);
}

// Unpack the sequenceGeneratorTdb.: Convert all offsets relative to base
// to pointers
Lng32 ComTdbSequenceGenerator::unpack(void * base, void * reallocator)
{
  if(childTdb_.unpack(base, reallocator)) 
    return -1;
  if(sgAttributes_.unpack(base, reallocator)) 
    return -1;
  return ComTdb::unpack(base, reallocator);
}


// Methods for ComTdbNextValueFor

// Default Constructor.  This is used by the ComTdb::fixupVTblPtr()
// routine which fixes up the virtual function table pointer after
// a node has been unpacked.
ComTdbNextValueFor::ComTdbNextValueFor() 
: ComTdb(ComTdb::ex_NEXTVALUEFOR, eye_NEXTVALUEFOR)
{
}

// Constructor used by the generator NextValueFor::codegen to
// generate the NextValueFor TDB.
//
// Arguements:
//
//   criDescParentDown - pointer to the criDesc for the parent down queue.
//   A descriptor of the request recieved from the parent.
//
//   criDescParentUp - pointer to the criDesc for the parent up queue.
//   A descriptor of the replies given to the parent.
//
//   queueSizeDown - Size of the down queue. Must be a power of two.
//
//   queueSizeUp - Size of the up queue. Must be a power of two.
//
//   nextValueReturnTuppIndex - atpindex where the next value is 
//                              returned to the parent.
ComTdbNextValueFor::ComTdbNextValueFor(ComTdb *leftChildTdb,
				       ComTdb *rightChildTdb,
				       ex_cri_desc *criDescParentDown,
				       ex_cri_desc *criDescParentUp,
				       ex_cri_desc *workCriDesc,
				       queue_index queueSizeDown,
				       queue_index queueSizeUp,
				       const unsigned short nextValueReturnTuppIndex,
				       ComSequenceGeneratorAttributes *nvSGAttributes,
				       ex_expr *moveSGOutputExpr,
				       Lng32 numBuffers,
				       ULng32 bufferSize)
  : ComTdb(ComTdb::ex_NEXTVALUEFOR,
	   eye_NEXTVALUEFOR,
	   (Cardinality) 1.0,
	   criDescParentDown,
	   criDescParentUp,
	   queueSizeDown,
	   queueSizeUp,
	   numBuffers,
	   bufferSize),
    leftChildTdb_(leftChildTdb),
    rightChildTdb_(rightChildTdb),
    nextValueReturnTuppIndex_(nextValueReturnTuppIndex),
    workCriDesc_(workCriDesc),
    moveSGOutputExpr_(moveSGOutputExpr),
    nvSGAttributes_(nvSGAttributes)
{
}

// Used by GUI, does nothing now.
void
ComTdbNextValueFor::displayContents(Space *space, ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

// The sequence generator TDB has no children
Int32
ComTdbNextValueFor::numChildren() const
{
  return(2);
}

// Return the number of expressions held by the Next Value For TDB (0)
Int32
ComTdbNextValueFor::numExpressions() const
{
  return(1);
}

// Return the expression names of the Next Value For  TDB.
const char *
ComTdbNextValueFor::getExpressionName(Int32 expNum) const
{
  switch(expNum)
    {
    case 0:
      return "moveSGOutputExpr_";
    default:
      return NULL;
    }
}

// Return the expressions of the Next Value For TDB
ex_expr *
ComTdbNextValueFor::getExpressionNode(Int32 expNum)
{
  switch(expNum)
    {
    case 0:
      return moveSGOutputExpr_;
    default:
      return NULL;
    }
}

// Pack the NextValueForTDB: Convert all pointers to offsets relative
// to the space object.
Long ComTdbNextValueFor::pack(void * space)
{
  // Pack the child TDB, (this calls the pack() method on the child.
  //
  leftChildTdb_.pack(space);
  rightChildTdb_.pack(space);
  moveSGOutputExpr_.pack(space);
  workCriDesc_.pack(space);
  nvSGAttributes_.pack(space);

  return ComTdb::pack(space);
  }

// Unpack the NextValueForTdb.: Convert all offsets relative to base
// to pointers
Lng32 ComTdbNextValueFor::unpack(void * base, void * reallocator)
{
  if(leftChildTdb_.unpack(base, reallocator)) 
    return -1;
  if(rightChildTdb_.unpack(base, reallocator)) 
    return -1;
  if (moveSGOutputExpr_.unpack(base, reallocator)) 
    return -1;
  if (workCriDesc_.unpack(base, reallocator)) 
    return -1;
  if (nvSGAttributes_.unpack(base, reallocator)) 
    return -1;
  
  return ComTdb::unpack(base, reallocator);
}

