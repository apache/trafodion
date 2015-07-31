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
#ifndef _RU_SQL_DYNAMIC_STATEMENT_CONTAINER_H_
#define _RU_SQL_DYNAMIC_STATEMENT_CONTAINER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuSQLDynamicStatementContainer.h
* Description:  Definition of class CRUSQLDynamicStatementContainer
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

class CUOFsIpcMessageTranslator;

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer
//
//	A general use class that contain many dynamic sql statements
//	and allows to execute them by demand
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer : 
				public CRUSQLStatementContainer {
private:
	typedef CRUSQLStatementContainer inherited;

public:
	
	enum { 
		MAX_COMPILED_PARAMS = 10,
		MAX_SQL_TEXT_SIZE = 10000 
	};

	static const char* const COMPILED_PARAM_TOKEN ;

public:
	CRUSQLDynamicStatementContainer(short nStmts);
	virtual ~CRUSQLDynamicStatementContainer();

public:
	// Is the statment already prepared ?
	inline BOOL IsStatementPrepared(short index);
	
	// Returns the last sql statement text that was prepared
	inline char const* GetLastSQL(short index);

	// Load the container with an sql text 
	inline void SetStatementText(short index, const CDSString &sql);

public:
	// Prepares all statements
	void PrepareSQL();
	
	// Prepare a single statement
	inline void PrepareStatement(short index);
	
	// Replace the compiled time param (represented by COMPILED_PARAM_TOKEN
	// in the statment text) by value
	inline void SetCompiledTimeParam(short index, // statment index
						  short paramIndex,
						  const CDSString &value);

	// Direct execution of the sql statement - generally used for ddl statments
	void DirectExecStatement(short index);

protected:
	REFRESH_LIB_CLASS class DynamicStmt;

protected:
	
	virtual DynamicStmt& GetDynamicStmt(short index);
	
protected:
	// Implementation of pure virtual function
	inline virtual CRUSQLStatementContainer::Stmt& GetStmt(short index);

private:
	//-- Prevent copying
	CRUSQLDynamicStatementContainer
							(const CRUSQLDynamicStatementContainer &other);
	CRUSQLDynamicStatementContainer &operator = 
							(const CRUSQLDynamicStatementContainer &other);

private:
	// An array of dynamic statments 
	DynamicStmt *pDynamicStmtVec_;
};


//--------------------------------------------------------------------------//
//	CRUSQLStaticStatementContainer::DynamicStmt
//
//
//--------------------------------------------------------------------------//

class CRUSQLDynamicStatementContainer::DynamicStmt: 
				public  CRUSQLDynamicStatementContainer::Stmt {

private:
	typedef CRUSQLDynamicStatementContainer::Stmt inherited;

public:
	
	DynamicStmt();
	virtual ~DynamicStmt();

public:
	// Is the statment already prepared ?
	inline BOOL IsStatementPrepared() const
	{
		return prepared_;
	}

	char const* GetLastSQL();

	// The function compiles the statment if the statement needs
	// recompilation .
	// The statement needs recompilation at the first time or when 
	// one or more of its compiled parameters is changed
	CDMPreparedStatement *GetPreparedStatement(BOOL DeleteUsedStmt = TRUE);

	// Direct execution of the sql statement - generally used for ddl statments
	void DirectExecStatement();


public:
	void SetStatementText(const CDSString &sql);
	void SetCompiledTimeParam(short paramIndex,const CDSString &value);
	void PrepareStatement(BOOL PrepareStatement = TRUE);

public:
	// This functions used to (un)serialized the executor context 
	// for the message communication with the remote server process
	virtual void LoadData(CUOFsIpcMessageTranslator &translator);

	virtual void StoreData(CUOFsIpcMessageTranslator &translator);

private:
	// Find outs how many compile parameters are in the statement and their 
	// position
	void AnalyzeSql();

	// Prepare the sql text for compilation by inserting all compiledtime 
	// params into the sql text
	void PrepareSqlText(char *buffer);

private:

	//-- Prevent copying
#if defined(NA_WINNT)
	CRUSQLDynamicStatementContainer::DynamicStmt
		(const CRUSQLDynamicStatementContainer::DynamicStmt &other);
	CRUSQLDynamicStatementContainer::DynamicStmt &operator = 
		(const CRUSQLDynamicStatementContainer::DynamicStmt &other);
#endif

private:
	// These fields are the only one who travels between processes
	char*				sql_;
	Lng32				paramNum_;

private:
	// Is the statment already prepared ?
	BOOL				prepared_; 
	CDSString			params_[MAX_COMPILED_PARAMS];
	Int32					paramsPos_[MAX_COMPILED_PARAMS];
	Int32					paramSumStringSize_;
};


//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer inlines
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DirectExecStatement()
//
// Direct execution of the sql statement - generally used for ddl statments
//--------------------------------------------------------------------------//

inline void REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::
	DirectExecStatement(short index)
{
	RUASSERT(0 <= index && index < GetNumOfStmt());

	GetDynamicStmt(index).DirectExecStatement();
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::GetDynamicStmt()
//--------------------------------------------------------------------------//
inline REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::DynamicStmt& 
REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::GetDynamicStmt(short index)
{
	RUASSERT(0 <= index && index < GetNumOfStmt());

	return  pDynamicStmtVec_[index];
}

//--------------------------------------------------------------------------//
// CRUSQLDynamicStatementContainer::GetStmt()
//
// Implementation of pure virtual function
//--------------------------------------------------------------------------//
inline REFRESH_LIB_CLASS CRUSQLStatementContainer::Stmt& 
REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::GetStmt(short index)
{
	return GetDynamicStmt(index);
}


//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::IsStatementPrepared()
//--------------------------------------------------------------------------//
inline BOOL REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::
IsStatementPrepared(short index) 
{
	return GetDynamicStmt(index).IsStatementPrepared();
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::GetLastSQL()
//
// Returns the last sql statement text that was prepared
//--------------------------------------------------------------------------//
inline char const* REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::
GetLastSQL(short index)
{
	return GetDynamicStmt(index).GetLastSQL();
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::SetStatementText()
//
// Load the container with an sql text 
//--------------------------------------------------------------------------//
inline void REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::
SetStatementText(short index, const CDSString &sql)
{
	GetDynamicStmt(index).SetStatementText(sql);
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::SetStatementText()
//
// Prepare a single statement
//--------------------------------------------------------------------------//
inline void REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::
	PrepareStatement(short index)
{
	GetDynamicStmt(index).PrepareStatement();
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::SetCompiledTimeParam()
//--------------------------------------------------------------------------//
void REFRESH_LIB_CLASS CRUSQLDynamicStatementContainer::
	SetCompiledTimeParam(	short index ,
							short paramIndex,
							const CDSString &value)
{
	GetDynamicStmt(index).SetCompiledTimeParam(paramIndex,value);
}



#endif
