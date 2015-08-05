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
* File:         ComTdbDDL.cpp
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

#include "ComTdbDDL.h"
#include "ComTdbCommon.h" 

ComTdbGenericUtil::ComTdbGenericUtil(char * query,
				     ULng32 querylen,
				     Int16 querycharset,
				     char * objectName,
				     ULng32 objectNameLen,
				     ex_expr * input_expr,
				     ULng32 input_rowlen,
				     ex_expr * output_expr,
				     ULng32 output_rowlen,
				     ex_cri_desc * work_cri_desc,
				     const unsigned short work_atp_index,
				     ex_cri_desc * given_cri_desc,
				     ex_cri_desc * returned_cri_desc,
				     queue_index down,
				     queue_index up,
				     Lng32 num_buffers,
				     ULng32 buffer_size)
     : ComTdb(ComTdb::ex_DDL,
	      eye_DDL,
	      (Cardinality) 0.0,
	      given_cri_desc,
	      returned_cri_desc,
	      down,
	      up,   
	      num_buffers,
	      buffer_size),
       query_(query),
       queryLen_(querylen),
       queryCharSet_(querycharset),
       objectName_(objectName),
       objectNameLen_(objectNameLen),
       inputExpr_(input_expr),
       inputRowlen_(input_rowlen),
       outputExpr_(output_expr),
       outputRowlen_(output_rowlen),
       workCriDesc_(work_cri_desc),
       workAtpIndex_(work_atp_index),
       flags_(0),
       tuppIndex_(returned_cri_desc->noTuples() - 1)
{
  memset(fillersComTdbGenericUtil_, sizeof(fillersComTdbGenericUtil_), 0);
}

ComTdbGenericUtil::~ComTdbGenericUtil()
{
}

Long ComTdbGenericUtil::pack(void * space)
{
  if (query_) query_.pack(space);
  if (objectName_) objectName_.pack(space);
  inputExpr_.pack(space);
  outputExpr_.pack(space);
  workCriDesc_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbGenericUtil::unpack(void * base, void * reallocator)
{
  if(query_.unpack(base)) return -1;
  if(objectName_.unpack(base)) return -1;
  if(inputExpr_.unpack(base, reallocator)) return -1;
  if(outputExpr_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

Int32 ComTdbGenericUtil::orderedQueueProtocol() const {return -1;}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbDDL
//
///////////////////////////////////////////////////////////////////////////
ComTdbDDL::ComTdbDDL(char * ddl_query,
		     ULng32 ddl_querylen,
		     Int16 ddl_querycharset,
		     char * schemaName,
		     ULng32 schemaNameLen,
		     ex_expr * input_expr,
		     ULng32 input_rowlen,
		     ex_expr * output_expr,
		     ULng32 output_rowlen,
		     ex_cri_desc * work_cri_desc,
		     const unsigned short work_atp_index,
		     ex_cri_desc * given_cri_desc,
		     ex_cri_desc * returned_cri_desc,
		     queue_index down,
		     queue_index up,
		     Lng32 num_buffers,
		     ULng32 buffer_size)
     : ComTdbGenericUtil(ddl_query, ddl_querylen, ddl_querycharset, schemaName, schemaNameLen,
			 input_expr, input_rowlen,
			 output_expr, output_rowlen,
			 work_cri_desc, work_atp_index,
			 given_cri_desc, returned_cri_desc,
			 down, up, 
			 num_buffers, buffer_size),
       flags_(0)
{
  setNodeType(ComTdb::ex_DDL);

  memset(fillersComTdbDDL_, sizeof(fillersComTdbDDL_), 0);
}


///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbDDL
//
///////////////////////////////////////////////////////////////////////////
ComTdbDDLwithStatus::ComTdbDDLwithStatus(char * ddl_query,
                                         ULng32 ddl_querylen,
                                         Int16 ddl_querycharset,
                                         char * schemaName,
                                         ULng32 schemaNameLen,
                                         ex_expr * input_expr,
                                         ULng32 input_rowlen,
                                         ex_expr * output_expr,
                                         ULng32 output_rowlen,
                                         ex_cri_desc * work_cri_desc,
                                         const unsigned short work_atp_index,
                                         ex_cri_desc * given_cri_desc,
                                         ex_cri_desc * returned_cri_desc,
                                         queue_index down,
                                         queue_index up,
                                         Lng32 num_buffers,
                                         ULng32 buffer_size)
  : ComTdbDDL(ddl_query, ddl_querylen, ddl_querycharset, 
              schemaName, schemaNameLen,
              input_expr, input_rowlen,
              output_expr, output_rowlen,
              work_cri_desc, work_atp_index,
              given_cri_desc, returned_cri_desc,
              down, up, 
              num_buffers, buffer_size),
    flags2_(0)
{
  setNodeType(ComTdb::ex_DDL_WITH_STATUS);
}


///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbDescribe, ExDescribeTcb, ExDescribePrivateState
//
///////////////////////////////////////////////////////////////////////////
ComTdbDescribe::ComTdbDescribe(char * query,
			     ULng32 querylen,
			     Int16 ddl_querycharset,
			     ex_expr * input_expr,
			     ULng32 input_rowlen,
			     ex_expr * output_expr,
			     ULng32 output_rowlen,
			     ex_cri_desc * work_cri_desc,
			     const unsigned short work_atp_index,
			     DescribeType type,
			     ULng32 flags,
                             ex_cri_desc * given_cri_desc,
		             ex_cri_desc * returned_cri_desc,
		             queue_index down,
		             queue_index up,
		             Lng32 num_buffers,
		             Lng32 buffer_size)
  : ComTdbDDL(query, querylen, ddl_querycharset, NULL, 0, 
	     input_expr, input_rowlen,
	     output_expr, output_rowlen,
	     work_cri_desc, work_atp_index,
             given_cri_desc, returned_cri_desc,
             down, up, 
             num_buffers, buffer_size),
    type_(type),
    flags_((UInt32)flags)
{
  setNodeType(ComTdb::ex_DESCRIBE);
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbProcessVolatileTable
//
///////////////////////////////////////////////////////////////////////////
ComTdbProcessVolatileTable::ComTdbProcessVolatileTable
(char * query,
 ULng32 querylen,
 Int16 querycharset,
 char * volTabName,
 ULng32 volTabNameLen,
 NABoolean isCreate,
 NABoolean isTable,
 NABoolean isIndex,
 NABoolean isSchema,
 char * schemaName,
 ULng32 schemaNameLen,
 ex_expr * input_expr,
 ULng32 input_rowlen,
 ex_expr * output_expr,
 ULng32 output_rowlen,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbDDL(query, querylen, querycharset, schemaName, schemaNameLen,
		 input_expr, input_rowlen,
		 output_expr, output_rowlen,
		 work_cri_desc, work_atp_index,
		 given_cri_desc, returned_cri_desc,
		 down, up, 
		 num_buffers, buffer_size),
       volTabName_(volTabName),
       volTabNameLen_(volTabNameLen),
       flags_(0)
{
  setIsCreate(isCreate);
  setIsTable(isTable);
  setIsIndex(isIndex);
  setIsSchema(isSchema);

  setNodeType(ComTdb::ex_PROCESS_VOLATILE_TABLE);
}

Long ComTdbProcessVolatileTable::pack(void * space)
{
  if (volTabName_) volTabName_.pack(space);

  return ComTdbDDL::pack(space);
}

Lng32 ComTdbProcessVolatileTable::unpack(void * base, void * reallocator)
{
  if (volTabName_.unpack(base)) return -1;

  return ComTdbDDL::unpack(base, reallocator);
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbProcessInMemoryTable
//
///////////////////////////////////////////////////////////////////////////
ComTdbProcessInMemoryTable::ComTdbProcessInMemoryTable
(char * query,
 ULng32 querylen,
 Int16 querycharset,
 char * objName,
 ULng32 objNameLen,
 NABoolean isCreate,
 NABoolean isVolatile,
 NABoolean isTable,
 NABoolean isIndex,
 NABoolean isMV,
 char * schemaName,
 ULng32 schemaNameLen,
 ex_expr * input_expr,
 ULng32 input_rowlen,
 ex_expr * output_expr,
 ULng32 output_rowlen,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbDDL(query, querylen, querycharset, schemaName, schemaNameLen,
		 input_expr, input_rowlen,
		 output_expr, output_rowlen,
		 work_cri_desc, work_atp_index,
		 given_cri_desc, returned_cri_desc,
		 down, up, 
		 num_buffers, buffer_size),
       objName_(objName),
       objNameLen_(objNameLen),
       flags_(0)
{
  setIsCreate(isCreate);
  setIsVolatile(isVolatile);
  setIsTable(isTable);
  setIsIndex(isIndex);
  setIsMV(isMV);

  setNodeType(ComTdb::ex_PROCESS_INMEMORY_TABLE);
}

Long ComTdbProcessInMemoryTable::pack(void * space)
{
  if (objName_) objName_.pack(space);

  return ComTdbDDL::pack(space);
}

Lng32 ComTdbProcessInMemoryTable::unpack(void * base, void * reallocator)
{
  if (objName_.unpack(base)) return -1;

  return ComTdbDDL::unpack(base, reallocator);
}



