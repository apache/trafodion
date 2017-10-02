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
* File:         ComTdbTupleFlow.cpp
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

#include "ComTdbTupleFlow.h"
#include "ComTdbCommon.h"

/////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
/////////////////////////////////////////////////////////////////////////
ComTdbTupleFlow::ComTdbTupleFlow(ComTdb * tdb_src,
				 ComTdb * tdb_tgt,
				 ex_cri_desc * given_cri_desc,
				 ex_cri_desc * returned_cri_desc,
				 ex_expr * tgt_expr,
				 ex_cri_desc * work_cri_desc,
				 queue_index down,
				 queue_index up,
				 Cardinality estimatedRowCount,
				 Lng32 num_buffers,
				 ULng32 buffer_size,
				 NABoolean vsbbInsert,
				 NABoolean rowsetIterator,
				 NABoolean tolerateNonFatalError)
  : ComTdb(ComTdb::ex_TUPLE_FLOW,
	   eye_TUPLE_FLOW,
	   estimatedRowCount,
           given_cri_desc,
	   returned_cri_desc,
	   down,
	   up,	
	   num_buffers,
	   buffer_size),
    tdbSrc_(tdb_src),
    tdbTgt_(tdb_tgt),
    tgtExpr_(tgt_expr),
    workCriDesc_(work_cri_desc),
    flags_(0)
{
  if (vsbbInsert == TRUE)
    flags_ |= VSBB_INSERT;
  if (rowsetIterator == TRUE)
    flags_ |= ROWSET_ITERATOR;
  if (tolerateNonFatalError == TRUE)
    setTolerateNonFatalError(TRUE);
}

// destructor
ComTdbTupleFlow::~ComTdbTupleFlow() {}

Long ComTdbTupleFlow::pack(void * space)
{
  tdbSrc_.pack(space);
  tdbTgt_.pack(space);
  workCriDesc_.pack(space);
  tgtExpr_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbTupleFlow::unpack(void * base, void * reallocator)
{
  if(tdbSrc_.unpack(base, reallocator)) return -1;
  if(tdbTgt_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  if(tgtExpr_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

const ComTdb* ComTdbTupleFlow::getChild(Int32 pos) const
{
  if (pos == 0)
    return tdbSrc_;
  else if (pos == 1)
    return tdbTgt_;
  else
    return NULL;
}

ex_expr* ComTdbTupleFlow::getExpressionNode(Int32 pos)
{
  if (pos == 0)
    return tgtExpr_;
  else
    return NULL;
}

const char * ComTdbTupleFlow::getExpressionName(Int32 pos) const
{
  if (pos == 0)
    return "tgtExpr_";
  else
    return NULL;
}

void ComTdbTupleFlow::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      
      Lng32 lFlags = flags_%65536;
      Lng32 hFlags = (flags_- lFlags)/65536;
      str_sprintf(buf, "\nFor ComTdbTupleFlow :\nFlags = %x%x ",hFlags,lFlags );
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

