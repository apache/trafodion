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
* File:         ComTdbUnion.cpp
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

#include "ComTdbUnion.h"
#include "ComTdbCommon.h"

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////

// Constructor
ComTdbUnion::ComTdbUnion() :
     ComTdb(ComTdb::ex_UNION, eye_UNION),
     tuppIndex_(0)
{
}


// not called, tested by removing the code and testing
ComTdbUnion::ComTdbUnion(const ComTdbUnion *union_tdb)
  : ComTdb(ComTdb::ex_UNION,
	   eye_UNION,
	   union_tdb->getEstRowsUsed(),
	   union_tdb->criDescDown_,
	   union_tdb->criDescUp_,
	   union_tdb->queueSizeDown_, 
	   union_tdb->queueSizeUp_,
	   union_tdb->numBuffers_,
	   union_tdb->bufferSize_),
    tdbLeft_(union_tdb->tdbLeft_), 
    tdbRight_(union_tdb->tdbRight_),
    leftExpr_(union_tdb->leftExpr_),
    rightExpr_(union_tdb->rightExpr_), 
    mergeExpr_(union_tdb->mergeExpr_),
    condExpr_(union_tdb->condExpr_),
    trigExceptExpr_(union_tdb->trigExceptExpr_),
    unionReclen_(union_tdb->unionReclen_), 
    tuppIndex_(union_tdb->tuppIndex_),
    flags_(union_tdb->flags_),
    csErrFlags_(union_tdb->csErrFlags_)
{
}

ComTdbUnion::ComTdbUnion(ComTdb * left_tdb,
			 ComTdb * right_tdb,
			 ex_expr * left_expr,
			 ex_expr * right_expr,
			 ex_expr * merge_expr,
			 ex_expr * cond_expr,
			 ex_expr * trig_expr,
			 Lng32 union_reclen,
			 const unsigned short tupp_index,
			 ex_cri_desc * given_cri_desc,
			 ex_cri_desc * returned_cri_desc,
			 queue_index down,
			 queue_index up,
			 Cardinality estimatedRowCount,
			 Lng32 num_buffers,
			 ULng32 buffer_size,
			 NABoolean ordered_union,
                         Int32 blocked_union, //++ Triggers -
			 Int32 hasNoOutput,   //++ Triggers -
			 NABoolean rowsFromLeft,
                         NABoolean rowsFromRight,
                         NABoolean afterUpdate,
			 NABoolean inNotAtomicStmt)
  : ComTdb(ComTdb::ex_UNION,
	   eye_UNION,
	   estimatedRowCount,
	   given_cri_desc,
	   returned_cri_desc,
	   down,
	   up,
	   num_buffers,
	   buffer_size),
    tdbLeft_(left_tdb),
    tdbRight_(right_tdb),
    leftExpr_(left_expr),
    rightExpr_(right_expr),
    mergeExpr_(merge_expr),
    condExpr_(cond_expr),
    trigExceptExpr_(trig_expr),
    unionReclen_(union_reclen),
    tuppIndex_(tupp_index),
    flags_(0),
    csErrFlags_(NOT_CONDITIONAL_UNION)
{
  if (merge_expr)
    flags_ |= ComTdbUnion::MERGE_UNION;
  else if (cond_expr)
    flags_ |= ComTdbUnion::CONDITIONAL_UNION;
  else if (ordered_union)
    flags_ |= ComTdbUnion::ORDERED_UNION;
  else if (blocked_union) //++ Triggers -
    flags_ |= ComTdbUnion::BLOCKED_UNION;
  else if (cond_expr)
    flags_ |= ComTdbUnion::CONDITIONAL_UNION;
  else
    flags_ |= ComTdbUnion::UNION_ALL;

  //++ Triggers -
  if (hasNoOutput)
      flags_ |= ComTdbUnion::NO_OUTPUTS;

  if (cond_expr) {
    if (rowsFromLeft)
      csErrFlags_ |= ComTdbUnion::ROWS_FROM_LEFT;
    if (rowsFromRight)
      csErrFlags_ |= ComTdbUnion::ROWS_FROM_RIGHT;
    if (afterUpdate)
      csErrFlags_ |= ComTdbUnion::AFTER_UPDATE;
    if (inNotAtomicStmt)
      flags_ |= ComTdbUnion::IN_NOT_ATOMIC_STMT;
  }

  // If in NAR and this is a blocked or ordered
  // union, then set the IN_NOT_ATOMIC_STMT flag
  if (inNotAtomicStmt && (ordered_union || blocked_union))
      flags_ |= ComTdbUnion::IN_NOT_ATOMIC_STMT;
}

ComTdbUnion::~ComTdbUnion() {}

Int32 ComTdbUnion::numChildren() const
{
  return ( ( tdbRight_ == (ComTdbPtr) NULL ) ? 1 : 2 );
}

// exclude from code coverage since this is used only GUI
void ComTdbUnion::display() const {}; 

Long ComTdbUnion::pack(void * space)
{
  tdbLeft_.pack(space);
  tdbRight_.pack(space);
  leftExpr_.pack(space);
  rightExpr_.pack(space);
  mergeExpr_.pack(space);
  condExpr_.pack(space);
  trigExceptExpr_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbUnion::unpack(void * base, void * reallocator)
{
  if(tdbLeft_.unpack(base, reallocator)) return -1;
  if(tdbRight_.unpack(base, reallocator)) return -1;
  if(leftExpr_.unpack(base, reallocator)) return -1;
  if(rightExpr_.unpack(base, reallocator)) return -1;
  if(mergeExpr_.unpack(base, reallocator)) return -1;
  if(condExpr_.unpack(base, reallocator)) return -1;
  if(trigExceptExpr_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

ex_expr* ComTdbUnion::getExpressionNode(Int32 pos)
{
  if (pos == 0)
    return leftExpr_;
  else if (pos == 1) 
    return rightExpr_;
  else if (pos == 2)
    return mergeExpr_;
  else if (pos == 3)
    return condExpr_;
  else if (pos == 4)
    return trigExceptExpr_;
  else
    return NULL;
}

const char * ComTdbUnion::getExpressionName(Int32 pos) const
{
  if (pos == 0)
    return "leftExpr_";
  else if (pos == 1) 
    return "rightExpr_";
  else if (pos == 2)
    return "mergeExpr_";
  else if (pos == 3)
    return "condExpr_";
  else if (pos == 4)
    return "trigExceptExpr_";
  else
    return NULL;
}

void ComTdbUnion::displayContents(Space * space,ULng32 flag)
{
      ComTdb::displayContents(space,flag & 0xFFFFFFFE);

      if(flag & 0x00000008)
        {
		      char buf[100];
	   		str_sprintf(buf, "\nFor ComTdbUnion :\nFlags = %x, unionReclen = %d ",
      						flags_,unionReclen_);
		      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

      if(flag & 0x00000001)
        {
        		displayExpression(space,flag);
            displayChildren(space,flag);
        }
}

