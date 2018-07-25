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
* File:         RuSQLStatementContainer.cpp
* Description:  Implementation of class CRUSQLStatementContainer
*				
*
* Created:      09/08/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuSQLStatementContainer.h"

#include "dmconnection.h"
#include "RuException.h"
#include "uofsIpcMessageTranslator.h"



//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::StoreData()
//--------------------------------------------------------------------------//
void CRUSQLStatementContainer::
StoreData(CUOFsIpcMessageTranslator &translator)
{
	short size = GetNumOfStmt();
	
	translator.WriteBlock(&size,sizeof(short));
	
	for (Int32 i=0;i<GetNumOfStmt();i++)
	{
		GetStmt(i).StoreData(translator);
	}
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::LoadData()
//--------------------------------------------------------------------------//
void CRUSQLStatementContainer::
	LoadData(CUOFsIpcMessageTranslator &translator)
{
	short size;
	
	translator.ReadBlock(&size,sizeof(short));
	
	RUASSERT(size <= GetNumOfStmt());

	for (Int32 i=0;i<GetNumOfStmt();i++)
	{
		GetStmt(i).LoadData(translator);
	}
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt::StoreData()
//--------------------------------------------------------------------------//
void CRUSQLStatementContainer::Stmt::
StoreData(CUOFsIpcMessageTranslator &translator)
{
	translator.WriteBlock(&executionCounter_,sizeof(Lng32));
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt::LoadData()
//--------------------------------------------------------------------------//
void CRUSQLStatementContainer::Stmt::
	LoadData(CUOFsIpcMessageTranslator &translator)
{
	translator.ReadBlock(&executionCounter_,sizeof(Lng32));
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt::ExecuteQuery()
//--------------------------------------------------------------------------//
Lng32 CRUSQLStatementContainer::Stmt::ExecuteUpdate()
{
	executionCounter_++;
	return GetPreparedStatement()->ExecuteUpdate();
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt::ExecuteQuery()
//--------------------------------------------------------------------------//
CDMResultSet *CRUSQLStatementContainer::Stmt::ExecuteQuery()
{
	executionCounter_++;
	return GetPreparedStatement()->ExecuteQuery();
}
