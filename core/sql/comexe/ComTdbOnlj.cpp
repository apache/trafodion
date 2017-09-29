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
* File:         ComTdbOnlj.cpp
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

#include "ComTdbOnlj.h"
#include "ComTdbCommon.h"

/////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
/////////////////////////////////////////////////////////////////////////

// Constructor
ComTdbOnlj::ComTdbOnlj() 
: ComTdb(ComTdb::ex_ONLJ, eye_ONLJ),
  instantiatedRowAtpIndex_(0) 
{
  
}


ComTdbOnlj::ComTdbOnlj(ComTdb * leftTdb,
		       ComTdb * rightTdb,
		       ex_cri_desc * given_cri_desc,
		       ex_cri_desc * returned_cri_desc,
		       queue_index down,
		       queue_index up,
		       Cardinality estimatedRowCount,
		       Lng32 num_buffers,
		       ULng32 buffer_size,
		       ex_expr    *before_pred,
		       ex_expr    *after_pred,
		       ex_expr    *lj_expr,
		       ex_expr    * /*ni_expr*/,
		       ex_cri_desc * work_cri_desc,
		       const unsigned short instantiated_row_atp_index,
		       Lng32 lj_reclen,
		       Int32 semi_join,
		       Int32 anti_semi_join,
		       Int32 left_join,
		       Int32 undo_join,
		       Int32 setNFError_join,
		       Int32 rowset_iterator,
		       Int32 index_join,
	    	       NABoolean vsbbInsert,
		       Int32 rowsetRowCountArraySize,
		       NABoolean tolerateNonFatalError,
                       NABoolean drivingMVLogging
		       )
  : ComTdb(ComTdb::ex_ONLJ,
	   eye_ONLJ,
	   estimatedRowCount,
	   given_cri_desc,
	   returned_cri_desc,
	   down,
	   up,
	   num_buffers,
	   buffer_size),
    tdbLeft_(leftTdb),
    tdbRight_(rightTdb),
    ljExpr_(lj_expr), 
    workCriDesc_(work_cri_desc),
    instantiatedRowAtpIndex_(instantiated_row_atp_index),
    ljRecLen_(lj_reclen),
    rowsetRowCountArraySize_(rowsetRowCountArraySize)
{
  flags_ = 0;
  if (semi_join)
    flags_ |= ComTdbOnlj::SEMI_JOIN;
    
  if (left_join)
    flags_ |= ComTdbOnlj::LEFT_JOIN;

 if (undo_join)
    flags_ |= ComTdbOnlj::UNDO_JOIN;

 if (setNFError_join)
    flags_ |= ComTdbOnlj::SET_NONFATAL_ERROR;

  if (anti_semi_join)
  {
    flags_ |= ComTdbOnlj::SEMI_JOIN;
    flags_ |= ComTdbOnlj::ANTI_JOIN;
  }
  if (rowset_iterator)
  {
    flags_ |= ComTdbOnlj::ROWSET_ITERATOR;
  }
    
  if (index_join)
    flags_ |= ComTdbOnlj::INDEX_JOIN;

  if (vsbbInsert)
	flags_ |= VSBB_INSERT;

  if (tolerateNonFatalError == TRUE)
    setTolerateNonFatalError(TRUE);
 
  if (drivingMVLogging == TRUE)
    flags_ |= DRIVING_MV_LOGGING;
 

  preJoinPred_ = before_pred;
  postJoinPred_ = after_pred;
}

ComTdbOnlj::~ComTdbOnlj() {}

void ComTdbOnlj::display() const {}

Long ComTdbOnlj::pack(void * space)
{
  tdbLeft_.pack(space);
  tdbRight_.pack(space);
  preJoinPred_.pack(space);
  postJoinPred_.pack(space);
  ljExpr_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbOnlj::unpack(void * base, void * reallocator)
{
  if(tdbLeft_.unpack(base, reallocator)) return -1;
  if(tdbRight_.unpack(base, reallocator)) return -1;
  if(preJoinPred_.unpack(base, reallocator)) return -1;
  if(postJoinPred_.unpack(base, reallocator)) return -1;
  if(ljExpr_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

const char * ComTdbOnlj::getNodeName() const
{
  if (isLeftJoin())
    return "EX_ONLJ_LEFT_JOIN";
  else if (isSemiJoin())
    if (isAntiJoin())
      return "EX_ONLJ_ANTI_SEMI_JOIN";
    else
      return "EX_ONLJ_SEMI_JOIN";
  else
    return "EX_ONLJ";
}

  ex_expr * ComTdbOnlj::getExpressionNode(Int32 pos)
{
  if (pos == 0)
    return postJoinPred_;
  else if (pos == 1)
    return preJoinPred_;
  else if (pos == 2)
    return ljExpr_;
  else if (pos == 3)
    return niExpr_;
  else
    return NULL;
}

const char * ComTdbOnlj::getExpressionName(Int32 pos) const
{
  if (pos == 0)
    return "postJoinPred_";
  else if (pos == 1)
    return "preJoinPred_";
  else if (pos == 2)
    return "ljExpr_";
  else if (pos == 3)
    return "niExpr_";
  else
    return NULL;
}

const ComTdb* ComTdbOnlj::getChild(Int32 pos) const
{
  if (pos == 0)
    return tdbLeft_;
  else if (pos == 1)
    return tdbRight_;
  else
    return NULL;
}

void ComTdbOnlj::displayContents(Space * space,ULng32 flag)
{
      ComTdb::displayContents(space,flag & 0xFFFFFFFE);

      if(flag & 0x00000008)
        {
		      char buf[100];
	   		str_sprintf(buf, "\nFor ComTdbOnlj :\nFlags = %x, ljRecLen = %d, instantiatedRowAtpIndex = %d ",
      						flags_, ljRecLen_, instantiatedRowAtpIndex_);
		      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

      if(flag & 0x00000001)
        {
        		displayExpression(space,flag);
            displayChildren(space,flag);
        }
}

