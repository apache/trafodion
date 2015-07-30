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
#ifndef _RU_SQL_STATEMENT_CONTAINER_H_
#define _RU_SQL_STATEMENT_CONTAINER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuSQLStatementContainer.h
* Description:  Definition of class CRUTaskExUnit
*				
*
* Created:      09/08/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/
#include "refresh.h"
#include "dmconnection.h"
#include "dmprepstatement.h"
#include "RuException.h"

class CUOFsIpcMessageTranslator;

//-------------------------------------------------------//
//	CRUSQLStatementContainer
//
//	A general use class that contain many sql statements
//	and allows to execute them by demand
//
//-------------------------------------------------------//

class REFRESH_LIB_CLASS CRUSQLStatementContainer {

public:
	CRUSQLStatementContainer(short nStmts)
		: nStmts_(nStmts) 
	{}
	
	virtual ~CRUSQLStatementContainer() 
	{}

public:
	class Stmt;

public:
	inline short GetNumOfStmt();

	// Returns an already compiled statement that is ready to run
	inline CDMPreparedStatement *GetPreparedStatement(short index, 
	                                                  BOOL DeleteUsedStmt = TRUE);

	inline Lng32 GetNumOfExecution(short index);
	
public:
	inline Lng32 ExecuteUpdate(short index);
	inline CDMResultSet *ExecuteQuery(short index);

public:
	// This functions used to (un)serialized the executor context 
	// for the message communication with the remote server process
	virtual void LoadData(CUOFsIpcMessageTranslator &translator);

	virtual void StoreData(CUOFsIpcMessageTranslator &translator);

protected:
	virtual Stmt& GetStmt(short index) = 0;

private:
	//-- Prevent copying
	CRUSQLStatementContainer(const CRUSQLStatementContainer &other);
	CRUSQLStatementContainer &operator = (const CRUSQLStatementContainer &other);

private:
	
	short nStmts_;
};


//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUSQLStatementContainer::Stmt {
public:
	
	Stmt() : pPrepStmt_(NULL),
			 executionCounter_(0)
	{
		pConnect_ = new CDMConnection();
	}

	virtual ~Stmt() 
	{
		delete pPrepStmt_;
		delete pConnect_;
	}

public:
	// Get the prepared statment and compile if necessary  
	inline virtual CDMPreparedStatement *GetPreparedStatement(BOOL DeleteUsedStmt = TRUE);

	Lng32 GetNumOfExecution() const
	{
		return executionCounter_;
	}

	inline CDMConnection *GetConnection();
public:
	Lng32 ExecuteUpdate();
	
	CDMResultSet *ExecuteQuery();

public:
	// This functions used to (un)serialized the executor context 
	// for the message communication with the remote server process
	virtual void LoadData(CUOFsIpcMessageTranslator &translator);

	virtual void StoreData(CUOFsIpcMessageTranslator &translator);

protected:
	inline void SetPreparedStatement(CDMPreparedStatement * pPrepStmt, 
	                                 BOOL DeleteUsedStmt = TRUE);

private:
	// DMOL sql statement object 
	CDMPreparedStatement *pPrepStmt_;
	
	Lng32 executionCounter_;
	CDMConnection *pConnect_;
};

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer inlines
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::GetPreparedStatement()
//--------------------------------------------------------------------------//
inline CDMPreparedStatement* REFRESH_LIB_CLASS CRUSQLStatementContainer::
	GetPreparedStatement(short index, BOOL DeleteUsedStmt)
{
	return GetStmt(index).GetPreparedStatement(DeleteUsedStmt);
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::GetNumOfStmt()
//--------------------------------------------------------------------------//
inline short REFRESH_LIB_CLASS CRUSQLStatementContainer::GetNumOfStmt() 
{
	return nStmts_;
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::GetNumOfStmt()
//--------------------------------------------------------------------------//
Lng32 REFRESH_LIB_CLASS CRUSQLStatementContainer::GetNumOfExecution(short index) 
{
	return GetStmt(index).GetNumOfExecution();
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::GetConnection()
//--------------------------------------------------------------------------//
inline CDMConnection * REFRESH_LIB_CLASS CRUSQLStatementContainer::Stmt::GetConnection() 
{
	return pConnect_;
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt::SetPreparedStatement()
//--------------------------------------------------------------------------//
inline Lng32 REFRESH_LIB_CLASS CRUSQLStatementContainer::
	ExecuteUpdate(short index)
{
	return GetStmt(index).ExecuteUpdate();
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt::SetPreparedStatement()
//--------------------------------------------------------------------------//
inline CDMResultSet * REFRESH_LIB_CLASS CRUSQLStatementContainer::
	ExecuteQuery(short index)
{
	return GetStmt(index).ExecuteQuery();
}


//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt inlines
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt::GetPreparedStatement()
//--------------------------------------------------------------------------//
inline CDMPreparedStatement * REFRESH_LIB_CLASS CRUSQLStatementContainer::Stmt::
	GetPreparedStatement(BOOL DeleteUsedStmt)
{
	return pPrepStmt_;
}

//--------------------------------------------------------------------------//
//	CRUSQLStatementContainer::Stmt::SetPreparedStatement()
//--------------------------------------------------------------------------//
inline void REFRESH_LIB_CLASS CRUSQLStatementContainer::Stmt::
	SetPreparedStatement(CDMPreparedStatement * pPrepStmt, 
	                     BOOL DeleteUsedStmt)
{
	executionCounter_ = 0;
	
	if (NULL != pPrepStmt_ && DeleteUsedStmt)
	{
	  delete pPrepStmt_;
	}
	
	pPrepStmt_ = pPrepStmt;
}



#endif
