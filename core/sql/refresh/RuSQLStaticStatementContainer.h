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
#ifndef _RU_SQL_STATIC_STATEMENT_CONTAINER_H_
#define _RU_SQL_STATIC_STATEMENT_CONTAINER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuSQLStaticStatementContainer.h
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
#include "RuSQLStatementContainer.h"
#include "ComVersionDefs.h"

//------------------------------------------------------------------//
//	CRUSQLStaticStatementContainer
//
//	A general use class that contain many static sql statements
//  from various modules files and allows to execute them by demand
//
//------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUSQLStaticStatementContainer : 
				public CRUSQLStatementContainer {
private:
	typedef CRUSQLStatementContainer inherited;

public:
	
	CRUSQLStaticStatementContainer(short nStmts);
	virtual ~CRUSQLStaticStatementContainer();

protected:
	REFRESH_LIB_CLASS class StaticStmt;

public:
	inline void SetStatement(short index, const CDMQueryDescriptor &pInfo, const COM_VERSION schemaVersion);

protected:
	inline virtual StaticStmt& GetStaticStmt(short index);
	
	// Implementation of pure virtual function
	inline virtual CRUSQLStatementContainer::Stmt& GetStmt(short index);

private:
	
	//-- Prevent copying
	CRUSQLStaticStatementContainer(const CRUSQLStaticStatementContainer &other);
	CRUSQLStaticStatementContainer &operator = (const CRUSQLStaticStatementContainer &other);

private:
	
	StaticStmt *pStaticStmtVec_;


};

//------------------------------------------------------------------//
//	CRUSQLStaticStatementContainer::StaticStmt
//
//
//------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUSQLStaticStatementContainer::StaticStmt: 
				public  CRUSQLStaticStatementContainer::Stmt {
private:
  typedef CRUSQLStaticStatementContainer::Stmt inherited;

public:
  StaticStmt() :pInfo_(NULL), schemaVersion_(COM_VERS_UNKNOWN) {}

public:

  void SetStatement(const CDMQueryDescriptor &pInfo, const COM_VERSION schemaVersion)
  {
    pInfo_ = &pInfo;
    schemaVersion_ = schemaVersion;
  }

  // Returns an already compiled statement that is ready to run
  CDMPreparedStatement *GetPreparedStatement();

private:

  //-- Prevent copying
	CDMPreparedStatement *GetPreparedStatement(BOOL DeleteUsedStmt = TRUE);
#if defined(NA_WINNT)
  CRUSQLStaticStatementContainer::StaticStmt
    (const CRUSQLStaticStatementContainer::StaticStmt &other);
  CRUSQLStaticStatementContainer::StaticStmt &operator = 
    (const CRUSQLStaticStatementContainer::StaticStmt &other);
#endif

private:

  const CDMQueryDescriptor *pInfo_;
  COM_VERSION       schemaVersion_;
};

//--------------------------------------------------------------------------//
//	CRUSQLStaticStatementContainer inlines
//--------------------------------------------------------------------------//
	
//--------------------------------------------------------------------------//
//	CRUSQLStaticStatementContainer::SetStatement()
//--------------------------------------------------------------------------//
inline void REFRESH_LIB_CLASS CRUSQLStaticStatementContainer::
SetStatement(short index, const CDMQueryDescriptor &pInfo, const COM_VERSION schemaVersion)
{
	GetStaticStmt(index).SetStatement(pInfo, schemaVersion);
}

//--------------------------------------------------------------------------//
//	CRUSQLStaticStatementContainer::GetStaticStmt()
//--------------------------------------------------------------------------//
inline CRUSQLStaticStatementContainer::StaticStmt& REFRESH_LIB_CLASS 
	CRUSQLStaticStatementContainer::GetStaticStmt(short index)
{
	RUASSERT(0 <= index && index < GetNumOfStmt());

	return  pStaticStmtVec_[index];
}

//--------------------------------------------------------------------------//
// CRUSQLStaticStatementContainer::GetStmt()
//
// Implementation of pure virtual function
//--------------------------------------------------------------------------//
inline CRUSQLStatementContainer::Stmt& 
REFRESH_LIB_CLASS CRUSQLStaticStatementContainer::GetStmt(short index)
{
	return GetStaticStmt(index);
}

#endif
