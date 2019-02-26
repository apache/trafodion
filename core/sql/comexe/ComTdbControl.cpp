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
* File:         ComTdbControl.cpp
* Description:  This is just a template for Executor TDB classes derived
*               from subclasses of ComTdb.
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbCommon.h"
#include "ComTdbControl.h"


/////////////////////////////////////////////////////
// class ExControlTdb
/////////////////////////////////////////////////////
ComTdbControl::ComTdbControl(ControlQueryType cqt,
			     Int32 reset,
			     char * sqlText,
			     Int16  sqlTextCharSet,
			     char * value1,
			     char * value2,
			     char * value3,
			     ex_cri_desc * given_cri_desc,
			     ex_cri_desc * returned_cri_desc,
			     queue_index down,
			     queue_index up,
			     Lng32 num_buffers,
			     ULng32 buffer_size)
  : ComTdb(ComTdb::ex_CONTROL_QUERY,
	   eye_CONTROL_QUERY,
           (Cardinality) 0.0,
	   given_cri_desc,
	   returned_cri_desc,
	   down,
	   up,
	   num_buffers,
	   buffer_size),
    cqt_(cqt),
    reset_(reset),
    sqlText_(sqlText),
    sqlTextCharSet_(sqlTextCharSet),
    value1_(value1),
    value2_(value2), 
    value3_(value3),
    actionType_(NONE_),
    nonResettable_(FALSE),
    flags_(0)
{}

Long ComTdbControl::pack(void * space)
{
  sqlText_.pack(space);
  value1_.pack(space);
  value2_.pack(space);
  value3_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbControl::unpack(void * base, void * reallocator)
{
  if (sqlText_.unpack(base)) return -1;
  if (value1_.unpack(base)) return -1;
  if (value2_.unpack(base)) return -1;
  if (value3_.unpack(base)) return -1;
  return ComTdb::unpack(base, reallocator);
}
