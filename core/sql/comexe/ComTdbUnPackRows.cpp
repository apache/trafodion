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
* File:         ComTdbUnPackRows.cpp
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

#include "ComTdbUnPackRows.h"
#include "ComTdbCommon.h"

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////

// Default Constructor
// Used when unpacking the UnPackRows TDB to get a pointer
// to the Virtual Method Table. Called by ComTdb::fixupVTblPtr().
//
ComTdbUnPackRows::ComTdbUnPackRows() 
  : ComTdb(ComTdb::ex_UNPACKROWS, eye_UNPACKROWS)
{
}

// Construct a copy of the given UnPackRows Tdb.
// (This constructor does not seem to be used)

// Exclude this code from coverage analysis.
ComTdbUnPackRows::ComTdbUnPackRows(const ComTdbUnPackRows *unPackRowsTdb)
  : ComTdb(ComTdb::ex_UNPACKROWS,
	   eye_UNPACKROWS,
           unPackRowsTdb->getEstRowsUsed(),
	   unPackRowsTdb->criDescDown_,
	   unPackRowsTdb->criDescUp_,
           unPackRowsTdb->queueSizeDown_,
	   unPackRowsTdb->queueSizeUp_,
           unPackRowsTdb->numBuffers_,
	   unPackRowsTdb->bufferSize_),
    childTdb_(unPackRowsTdb->childTdb_),
    packingFactor_(unPackRowsTdb->packingFactor_),
    unPackColsExpr_(unPackRowsTdb->unPackColsExpr_),
    unPackColsTupleLen_(unPackRowsTdb->unPackColsTupleLen_),
    unPackColsAtpIndex_(unPackRowsTdb->unPackColsAtpIndex_),
    indexValueAtpIndex_(unPackRowsTdb->indexValueAtpIndex_),
    workCriDesc_(unPackRowsTdb->workCriDesc_),
    flags_(unPackRowsTdb->flags_)
{
}

// Construct a new UnPackRows TDB.
// This constructor is call by the generator (PhysUnPackRows::codeGen() in
// GenRelPackedRows.cpp.) 
//
// Parameters
//
// ComTdb *childTdb
//  IN: The child of this UnPackRows TDB.
//
// ex_expr *unPackColsExpr
//  IN: A move expression which unpacks one value from each packed column.
//
// ex_expr *packingFactorExpr
//  IN: A move expression used to extract the packing factor from one of
//      the packed columns.
//
// long unPackColsTupleLen
//  IN: The length of the tuple which will hold the unpacked values.  This
//      tuple will be allocated by the UnPackRows node.
//
// unsigned short unPackColsAtpIndex
//  IN: The index of the UnPackRows tuple in the work and returned ATP.
//
// unsigned short indexValueAtpIndex
//  IN: The index of the index Value tuple in the work.
//
// ex_cri_desc *criDescDown
//  IN: The Cri Descriptor given to this node by its parent.
//
// ex_cri_desc *criDescUp
//  IN: The Cri Descriptor returned to the parent node.
//
// ex_cri_desc *workCriDesc
//  IN: The Cri Descriptor for the work Atp.
//
// queue_index queueSizeDown
//  IN: Recommended queue size for the down queue used to communicate 
//      with the parent.
//
// queue_index queueSizeUp
//  IN: Recommended queue size for the up queue used to communicate
//      with the parent.
//
// Cardinality estimatedRowCount
//  IN: compiler estimate on number of returned rows
//
ComTdbUnPackRows::ComTdbUnPackRows(ComTdb *childTdb,
				   ex_expr *packingFactor,
				   ex_expr *unPackColsExpr,
				   Lng32 unPackColsTupleLen,
				   unsigned short unPackColsAtpIndex,
				   unsigned short indexValueAtpIndex,
				   ex_cri_desc *criDescDown,
				   ex_cri_desc *criDescUp,
				   ex_cri_desc *workCriDesc,
				   queue_index queueSizeDown,
				   queue_index queueSizeUp,
				   Cardinality estimatedRowCount,
				   NABoolean rowsetIterator,
				   NABoolean tolerateNonFatalError) :
  ComTdb(ComTdb::ex_UNPACKROWS,
	 eye_UNPACKROWS,
         estimatedRowCount,
	 criDescDown,
	 criDescUp,
         queueSizeDown,
	 queueSizeUp,
         0,
	 0),
  childTdb_(childTdb),
  packingFactor_(packingFactor),
  unPackColsExpr_(unPackColsExpr),
  unPackColsTupleLen_(unPackColsTupleLen),
  unPackColsAtpIndex_(unPackColsAtpIndex),
  indexValueAtpIndex_(indexValueAtpIndex),
  workCriDesc_(workCriDesc),
  flags_(0)
{
    if (rowsetIterator == TRUE)
    flags_ |= ROWSET_ITERATOR;

    setTolerateNonFatalError(tolerateNonFatalError);
}

ComTdbUnPackRows::ComTdbUnPackRows(ComTdb *childTdb,
				   ex_expr *inputSizeExpr,
				   ex_expr *maxInputRowlenExpr,
				   ex_expr *rwrsBufferAddrExpr,
				   unsigned short rwrsAtpIndex,
				   ex_cri_desc *criDescDown,
				   ex_cri_desc *criDescUp,
				   ex_cri_desc *workCriDesc,
				   queue_index queueSizeDown,
				   queue_index queueSizeUp,
				   Cardinality estimatedRowCount,
				   Lng32 num_buffers,
				   ULng32 buffer_size) :
     ComTdb(ComTdb::ex_UNPACKROWS,
	    eye_UNPACKROWS,
	    estimatedRowCount,
	    criDescDown,
	    criDescUp,
	    queueSizeDown,
	    queueSizeUp,
	    num_buffers,
	    buffer_size),
  childTdb_(childTdb),
  packingFactor_(inputSizeExpr),
  unPackColsExpr_(maxInputRowlenExpr),
  rwrsBufferAddrExpr_(rwrsBufferAddrExpr),
  unPackColsTupleLen_(0),
  unPackColsAtpIndex_(rwrsAtpIndex),
  indexValueAtpIndex_(0),
  workCriDesc_(workCriDesc),
  flags_(0)
{
  setRowwiseRowset(TRUE);
}

// ComTdbUnPackRows::Display() -----------------------------------------
// (Don't know why this is here.  It does not seem to be virtual and
// on class seems to do anything for this method.)
//
//
// Exclude this code from coverage analysis.
// This code could be deleted since it is not used.
void
ComTdbUnPackRows::display() const 
{
  // Do nothing for now.
  //
}

// ComTdbUnPackRows::pack() ---------------------------------------------
// Pack the unPackRows TDB for transmission from the compiler to the
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
ComTdbUnPackRows::pack(void * space)
{
  // Pack the child TDB, (this calls the pack() method on the child.
  //
  childTdb_.pack(space);

  // Pack all the pointers that can be reach from the data members of this
  // TDB.
  //
  workCriDesc_.pack(space);

  packingFactor_.pack(space);
  unPackColsExpr_.pack(space);
  
  rwrsBufferAddrExpr_.pack(space);

  // Return the packed pointer to 'this', so that my parent can store the
  // packed version of my address.
  //
  return ComTdb::pack(space);
}

// ComTdbUnPackRows::unpack() ---------------------------------------------
// Unpack the unPackRows TDB after transmission from the compiler to the
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
ComTdbUnPackRows::unpack(void * base, void * reallocator)
{
  if(childTdb_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  if(packingFactor_.unpack(base, reallocator)) return -1;
  if(unPackColsExpr_.unpack(base, reallocator)) return -1;
  if(rwrsBufferAddrExpr_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

void ComTdbUnPackRows::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbUnPackRows :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (rowwiseRowset())
	{
	  str_sprintf(buf,"rwrsAtpIndex_  = %d", 
		      unPackColsAtpIndex_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      else
	{
	  str_sprintf(buf,"unPackColsTupleLen_  = %d, unPackColsAtpIndex_  = %d, indexValueAtpIndex_ = %d", 
		      unPackColsTupleLen_, unPackColsAtpIndex_, indexValueAtpIndex_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      str_sprintf(buf,"flags_ = %b", flags_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}




