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
//
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         RuTableSyncTask.cpp
* Description:  Implementation of class CRUTableSyncTask
*				
*
* Created:      12/06/1999
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuTableSyncTask.h"
#include "RuTableSyncTaskExecutor.h"
#include "RuTbl.h"

//---------------------------------------------------------------//
//	Constructor and destructor of CRUTableSyncTask
//---------------------------------------------------------------//

CRUTableSyncTask::CRUTableSyncTask(Lng32 id, CRUTbl &table) :
	CRULogProcessingTask(id,table)
{}

CRUTableSyncTask::~CRUTableSyncTask() 
{}

//---------------------------------------------------------//
//	CRUTableSyncTask::GetTaskName() 
//---------------------------------------------------------//

CDSString CRUTableSyncTask::GetTaskName() const
{
	CDSString name("TS(");
	name += GetTable().GetFullName() + ") ";
	
	return name;
}

//---------------------------------------------------------//
//	CRUTableSyncTask::CreateExecutorInstance()
//
//	Task executor's creation
//---------------------------------------------------------//

CRUTaskExecutor *CRUTableSyncTask::CreateExecutorInstance()
{
	GetTable().CheckIfLongLockNeeded();

	CRUTaskExecutor *pTaskEx = new CRUTableSyncTaskExecutor(this);

	return pTaskEx;
}
