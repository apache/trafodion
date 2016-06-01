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
* File:         ComTdbTimeout.cpp
* Description:  
*
* Created:      12/27/1999
* Language:     C++
*
*
*
****************************************************************************
*/

// -----------------------------------------------------------------------

#include "ComTdbTimeout.h"
#include "ComTdbCommon.h"

/////////////////////////////////////////////////////////////////
// class ComTdbTimeout
/////////////////////////////////////////////////////////////////
ComTdbTimeout::ComTdbTimeout( ex_expr * timeout_value_expr,
			      ex_cri_desc * work_cri_desc,
			      ex_cri_desc * given_cri_desc,
			      ex_cri_desc * returned_cri_desc,
			      queue_index down,
			      queue_index up,
			      Lng32 num_buffers,
			      ULng32 buffer_size)
  : ComTdb(ComTdb::ex_SET_TIMEOUT,
	   eye_SET_TIMEOUT,
	   (Cardinality) 0.0,
	   given_cri_desc,
	   returned_cri_desc,
	   down,
	   up, 
	   num_buffers,
	   buffer_size),
     timeoutValueExpr_(timeout_value_expr),
     workCriDesc_(work_cri_desc),
     flags_(0)
{
}

Long ComTdbTimeout::pack(void * space)
{
  timeoutValueExpr_.pack(space);
  workCriDesc_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbTimeout::unpack(void * base, void * reallocator)
{
  if(timeoutValueExpr_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}





