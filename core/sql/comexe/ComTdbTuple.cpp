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
* File:         ComTdbTuple.cpp
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

#include "ComTdbTuple.h"
#include "ComTdbCommon.h"
#include "ComQueue.h"
#include "str.h"

///////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
////////////////////////////////////////////////////////////////////////

ComTdbTuple::ComTdbTuple()
:ComTdb(ComTdb::ex_TUPLE, eye_TUPLE), tuppIndex_(0),
tupleLen_(0)
{
}

ComTdbTuple::ComTdbTuple(TupleTdbType ttt,
                         Queue * tupleExprList,
                         const ULng32 tupleLen,
                         const unsigned short tuppIndex,
                         ex_cri_desc * givenCriDesc,
                         ex_cri_desc * returnedCriDesc,
                         queue_index down,
                         queue_index up,
			 Cardinality estimatedRowCount,
                         Lng32 numBuffers,
                         ULng32 bufferSize,
                         ex_expr *predExpr)
     : ComTdb(ComTdb::ex_TUPLE,
	      eye_TUPLE,
	      estimatedRowCount,
              givenCriDesc,
	      returnedCriDesc,
              down,
	      up,
	      numBuffers,
	      bufferSize), 
       ttt_(ttt),
       tupleExprList_(tupleExprList),
       tupleLen_(tupleLen),
       tuppIndex_(tuppIndex),
       flags_(0),
       predExpr_(predExpr)
{
  switch(ttt)
  {
    case LEAF_:
      setNodeType(ComTdb::ex_LEAF_TUPLE);
      break;
    case NON_LEAF_:
      setNodeType(ComTdb::ex_NON_LEAF_TUPLE);
      break;
  }
}

ComTdbTuple::~ComTdbTuple()
{
}

void ComTdbTuple::display() const {};

Long ComTdbTuple::pack(void * space)
{
  PackQueueOfNAVersionedObjects(tupleExprList_,space,ex_expr);
  predExpr_.pack(space);

  return ComTdb::pack(space);
}

Lng32 ComTdbTuple::unpack(void * base, void * reallocator)
{
  UnpackQueueOfNAVersionedObjects(tupleExprList_,base,ex_expr,reallocator);
  if (predExpr_.unpack(base, reallocator)) return -1;

  return ComTdb::unpack(base, reallocator);
}

Int32 ComTdbTuple::numExpressions() const
{
  return tupleExprList_->numEntries() + 1;
}

ex_expr* ComTdbTuple::getExpressionNode(Int32 pos)
{
  if (pos == 0)
    return predExpr_;
  else
    return (ex_expr *)(tupleExprList_->get(pos-1));
}

const char * ComTdbTuple::getExpressionName(Int32 pos) const
{
  switch (pos)
    {
    case 0: return "predExpr_";
    case 1: return "tupleExprList_[0]";
    case 2: return "tupleExprList_[1]";
    case 3: return "tupleExprList_[2]";
    default: return "tupleExprList_[n]";
    }
}


///////////////////////////////////////
// class ExTupleLeafTdb
///////////////////////////////////////
ComTdbTupleLeaf::ComTdbTupleLeaf(Queue * tupleExprList,
				 const ULng32 tupleLen,
				 const unsigned short tuppIndex,
                                 ex_expr *predExpr,
				 ex_cri_desc * givenCriDesc,
				 ex_cri_desc * returnedCriDesc,
				 queue_index down,
				 queue_index up,
				 Cardinality estimatedRowCount,
				 Lng32 numBuffers,
				 ULng32 bufferSize)
  : ComTdbTuple(LEAF_,
		tupleExprList,
		tupleLen,
		tuppIndex,
		givenCriDesc,
		returnedCriDesc,
		down,
		up,
		estimatedRowCount,
		numBuffers,
		bufferSize,
                predExpr)
{
}


// This non-leaf tuple operator was not used so far on SQ, see GenRelMisc.cpp

///////////////////////////////////////
// class ExTupleNonLeafTdb
///////////////////////////////////////
ComTdbTupleNonLeaf::ComTdbTupleNonLeaf(Queue * tupleExprList,
				       ComTdb * tdbChild,
				       const ULng32 tupleLen,
				       const unsigned short tuppIndex,
				       ex_cri_desc * givenCriDesc,
				       ex_cri_desc * returnedCriDesc,
				       queue_index down,
				       queue_index up,
				       Cardinality estimatedRowCount,
				       Lng32 numBuffers,
				       ULng32 bufferSize)
  : ComTdbTuple(NON_LEAF_,
		tupleExprList,
		tupleLen,
		tuppIndex,
		givenCriDesc,
		returnedCriDesc,
		down,
		up,
		estimatedRowCount,
		numBuffers,
		bufferSize),
  tdbChild_(tdbChild)
{
}

Long ComTdbTupleNonLeaf::pack (void * space)
{
  tdbChild_.pack(space);
  return ComTdbTuple::pack(space);
}

Lng32 ComTdbTupleNonLeaf::unpack(void * base, void * reallocator)
{
  if(tdbChild_.unpack(base, reallocator)) return -1;
  return ComTdbTuple::unpack(base, reallocator);
}

// end of excluding non-leaf operator from coverage checking

