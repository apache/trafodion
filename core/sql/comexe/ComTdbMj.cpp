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
* File:         ComTdbMj.cpp
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

// --------------------------------------------------------------------

#include "ComTdbMj.h"
#include "ComTdbCommon.h"

/////////////////////////////////////////////////////////////
// Methods for class ComTdbMj
/////////////////////////////////////////////////////////////

// Constructor
ComTdbMj::ComTdbMj() 
: ComTdb(ComTdb::ex_MJ, eye_MJ)
{
};

ComTdbMj::ComTdbMj(ComTdb * left_tdb,
		   ComTdb * right_tdb,
		   ex_cri_desc * given_cri_desc,
		   ex_cri_desc * returned_cri_desc,
		   ex_expr * merge_expr,
		   ex_expr * comp_expr,
		   ex_expr * left_check_dup_expr,
		   ex_expr * right_check_dup_expr,
		   ex_expr * lj_expr,
		   ex_expr * ni_expr,
                   ex_expr * right_copy_dup_expr,
		   Lng32 right_dup_reclen,
		   Lng32 lj_reclen,
		   ex_cri_desc * work_cri_desc, 
		   short instantiated_row_atp_index,
		   ULng32 encoded_key_len,
		   short encoded_key_work_atp_index,
		   ex_expr * pre_join_expr,
		   ex_expr * post_join_expr,
		   queue_index down,
		   queue_index up,
		   Cardinality estimatedRowCount,
		   Lng32 num_buffers,
		   ULng32 buffer_size,
		   Int32 semi_join,
		   Int32 left_join,
		   Int32 anti_semi_join,
		   NABoolean left_is_unique,
		   NABoolean right_is_unique,
		   bool isOverflowEnabled,
		   UInt16 scratchThresholdPct,
		   UInt16 quotaMB,
		   UInt16 quotaPct,
		   bool yieldQuota
		   )
  : ComTdb(ComTdb::ex_MJ,
	   eye_MJ,
	   estimatedRowCount,
	   given_cri_desc,
	   returned_cri_desc,
	   down,
	   up,
	   num_buffers,
	   buffer_size),
    tdbLeft_(left_tdb),
    tdbRight_(right_tdb),
    leftCheckDupExpr_(left_check_dup_expr), 
    rightCheckDupExpr_(right_check_dup_expr),
    mergeExpr_(merge_expr),
    compExpr_(comp_expr),
    ljExpr_(lj_expr),
    niExpr_(ni_expr),
    rightCopyDupExpr_(right_copy_dup_expr),
    rightDupRecLen_(right_dup_reclen),
    ljRecLen_(lj_reclen),
    workCriDesc_(work_cri_desc),
    instantiatedRowAtpIndex_(instantiated_row_atp_index),
    encodedKeyLen_(encoded_key_len),
    encodedKeyWorkAtpIndex_(encoded_key_work_atp_index),
    preJoinExpr_(pre_join_expr),
    postJoinExpr_(post_join_expr),
    scratchThresholdPct_(scratchThresholdPct),
    quotaMB_(quotaMB),
    quotaPct_(quotaPct)
{
  flags_ = 0;

  if (semi_join)
    flags_ |= ComTdbMj::SEMI_JOIN;
  
  if (left_join)
    flags_ |= ComTdbMj::LEFT_JOIN;
  
  if (anti_semi_join)
    {
      flags_ |= ComTdbMj::SEMI_JOIN;
      flags_ |= ComTdbMj::ANTI_JOIN;
    }
  
  if (left_is_unique)
    flags_ |= ComTdbMj::LEFT_UNIQUE;

  if (right_is_unique)
    flags_ |= ComTdbMj::RIGHT_UNIQUE;

  if (encoded_key_len > 0)
    flags_ |= ComTdbMj::ENCODED_KEY_COMP_OPT;

  if (isOverflowEnabled)
    flags_ |= ComTdbMj::OVERFLOW_ENABLED;

  if (yieldQuota)
    flags_ |= ComTdbMj::YIELD_QUOTA;

};

void ComTdbMj::display() const {}; // LCOV_EXCL_LINE - ignore the next line, only used by GUI

Long ComTdbMj::pack(void * space)
{
  workCriDesc_.pack(space);
  tdbLeft_.pack(space);
  tdbRight_.pack(space);
  mergeExpr_.pack(space);
  compExpr_.pack(space);
  postJoinExpr_.pack(space);
  preJoinExpr_.pack(space);
  leftCheckDupExpr_.pack(space);
  rightCheckDupExpr_.pack(space);
  ljExpr_.pack(space);
  niExpr_.pack(space);
  rightCopyDupExpr_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbMj::unpack(void * base, void * reallocator)
{
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  if(tdbLeft_.unpack(base, reallocator)) return -1;
  if(tdbRight_.unpack(base, reallocator)) return -1;
  if(mergeExpr_.unpack(base, reallocator)) return -1;
  if(compExpr_.unpack(base, reallocator)) return -1;
  if(postJoinExpr_.unpack(base, reallocator)) return -1;
  if(preJoinExpr_.unpack(base, reallocator)) return -1;
  if(leftCheckDupExpr_.unpack(base, reallocator)) return -1;
  if(rightCheckDupExpr_.unpack(base, reallocator)) return -1;
  if(ljExpr_.unpack(base, reallocator)) return -1;
  if(niExpr_.unpack(base, reallocator)) return -1;
  if(rightCopyDupExpr_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

void ComTdbMj::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if (flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbMj :\nFlags = %x, ljRecLen = %d, rightDupRecLen = %d ",
		  flags_, ljRecLen_, rightDupRecLen_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

// LCOV_EXCL_START
Int32 ComTdbMj::orderedQueueProtocol() const
{
      return -1; // TRUE
} // there 4 lines will not be covered, obsolete but not in the list yet
// LCOV_EXCL_STOP
