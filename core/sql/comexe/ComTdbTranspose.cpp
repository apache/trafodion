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
* File:         ComTdbTranspose.cpp
* Description:  Methods for the tdb of a TRANSPOSE operation
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbTranspose.h"
#include "ComTdbCommon.h"

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////

// Default Constructor
// Used when unpacking the Transpose TDB to get a pointer
// to the Virtual Method Table. Called by ComTdb::fixupVTblPtr().
//
ComTdbTranspose::ComTdbTranspose() :
  ComTdb(ComTdb::ex_TRANSPOSE, eye_TRANSPOSE),
  transTuppIndex_(0)
{
}

// Construct a copy of the given Transpose Tdb.
// (This constructor does not seem to be used)
//
ComTdbTranspose::ComTdbTranspose(const ComTdbTranspose *transTdb)
  : ComTdb(ComTdb::ex_TRANSPOSE,
	   eye_TRANSPOSE,
	   transTdb->getEstRowsUsed(),
           transTdb->criDescDown_,
	   transTdb->criDescUp_,
           transTdb->queueSizeDown_,
	   transTdb->queueSizeUp_,
           transTdb->numBuffers_,
	   transTdb->bufferSize_),
    childTdb_(transTdb->childTdb_),
    transColExprs_(transTdb->transColExprs_),
    numTransExprs_(transTdb->numTransExprs_),
    afterTransPred_(transTdb->afterTransPred_),
    transRowLen_(transTdb->transRowLen_),
    transTuppIndex_(transTdb->transTuppIndex_)
{
}

// Construct a new Transpose TDB.
// This constructor is call by the generator (PhysTranspose::codeGen() in
// GenRelMisc.cpp.) 
//
// Parameters
//
// ComTdb *childTdb
//  IN: The child of this Transpose TDB.
//
// ex_expr **transColExprs
//  IN: A vector of pointers to ex_expr.  There are 'numTransExprs'
//      in the vector.  Each expression represents a transpose expression.
//      Each expression is a move expression which will generate a key
//      value, and multiple values.  One value will be from a transpose
//      expression, the others will generate the NULL value.
//
// int numTransExprs
//  IN: The number of expressions in transColsExprs.
//
// ex_expr *afterTransPred
//  IN: The selection Predicate for the Transpose operator. This expression
//      is applied after the transpose columns have been generated (so this
//      predicate will likely involve these generated values, other wise it
//      could have been pushed to the child of this node.
//
// long transRowLen
//  IN: The length of the tuple which will hold the generated values.  This
//      tuple will be allocated by the transpose node.
//
// const unsigned short transTuppIndex
//  IN: The index of the transpose tuple in the ATP.
//
// ex_cri_desc *criDescDown
//  IN: the Cri Descriptor given to this node by its parent.
//
// ex_cri_desc *criDescUp
//  IN: the Cri Descriptor return to the parent node.
//
// queue_index fromParent
//  IN: Recommended queue size for the down queue used to communicate
//      with the parent.
//
// queue_index toParent
//  IN: Recommended queue size for the up queue used to communicate
//      with the parent.
//
// Cardinality estimatedRowCount
//  IN: compiler estimate on number of returned rows
//
// long numBuffers
//  IN: Recommended number of buffers to allocate.
//
// unsigned long bufferSize
//  IN: Recommended size for pool buffers.
//
ComTdbTranspose::ComTdbTranspose(ComTdb *childTdb,
				 ex_expr **transColExprs,
				 Int32 numTransExprs,
				 ex_expr *afterTransPred,
				 Lng32 transRowLen,
				 const unsigned short transTuppIndex,
				 ex_cri_desc *criDescDown,
				 ex_cri_desc *criDescUp,
				 queue_index fromParent,
				 queue_index toParent,
				 Cardinality estimatedRowCount,
				 Lng32 numBuffers,
				 ULng32 bufferSize,
			         Space *space) :
  ComTdb(ComTdb::ex_TRANSPOSE,
	 eye_TRANSPOSE,
	 estimatedRowCount,
         criDescDown,
	 criDescUp,
         fromParent,
	 toParent,
         numBuffers,
	 bufferSize),
  childTdb_(childTdb),
  numTransExprs_(numTransExprs),
  afterTransPred_(afterTransPred),
  transRowLen_(transRowLen),
  transTuppIndex_(transTuppIndex),
  transColExprs_(space,(void **)transColExprs,numTransExprs)
{
  // Reallocate the array of pointers (which are 64-bit), and assign the
  // pointer values.
  //

  /*
  if (numTransExprs > 0)
  {
    transColExprs_ = (ExExprPtr *) 
      space->allocateAlignedSpace(numTransExprs * sizeof(ExExprPtr));
    for(int i=0; i < numTransExprs; i++) transColExprs_[i] = transColExprs[i];
  }
  */
}

// ComTdbTranspose::Display() -----------------------------------------
// (Don't know why this is here.  It does not seem to be virtual and
// on class seems to do anything for this method.)
//
void
ComTdbTranspose::display() const 
{
  // Do nothing for now.
  //
}

// ComTdbTranspose::pack() ---------------------------------------------
// Pack the transpose TDB for transmission from the compiler to the
// executor, or from an ESP to DP2, etc.  This method needs to convert
// all pointers to offsets, so that the memory containing this TDB fragment,
// can be relocated and 'unpacked' at a new base address.
//
// Parameters
//
// void *space
//  IN - The space object which was used to allocate this TDB. Used to
//       compute offsets all pointers.  It is an error if any pointer
//       that can be reached from this TDB points to memory outside 
//       this space object.
//
Long
ComTdbTranspose::pack(void * space)
{
  // Pack the child TDB, (this calls the pack() method on the child.
  //
  childTdb_.pack(space);
  transColExprs_.pack(space,numTransExprs_);
  afterTransPred_.pack(space);

  // Return the packed pointer to 'this', so that my parent can store the
  // packed version of my address.
  //
  return ComTdb::pack(space);
}

// ComTdbTranspose::unpack() ---------------------------------------------
// Unpack the transpose TDB after transmission from the compiler to the
// executor, or from an ESP to DP2, etc.  This method needs to convert
// all offsets to pointers, so that all pointers now reflected the new
// location of the TDB fragment.
//
// Parameters
//
// long base
//  IN - The base address of the TDB fragment.  Pointers are calculated
//       by adding the offset to the base address (more or less).
//
Lng32
ComTdbTranspose::unpack(void * base, void * reallocator)
{
  if(childTdb_.unpack(base, reallocator)) return -1;
  if(afterTransPred_.unpack(base, reallocator)) return -1;
  if(transColExprs_.unpack(base,numTransExprs_,reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

