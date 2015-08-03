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
#ifndef _RU_DUPELIM_TASKEX_UNIT_H_
#define _RU_DUPELIM_TASKEX_UNIT_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimTaskExUnit.h
* Description:  Definition of class CRUDupElimTaskExUnit
*				
*
* Created:      07/02/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "refresh.h"
#include "dsstring.h"

#include "RuSQLDynamicStatementContainer.h"

class CRUDupElimGlobals;

//--------------------------------------------------------------------------//
//	CRUDupElimTaskExUnit
//
//	The Duplicate Elimination task executor applies a number of *units*. 
//
//	Every unit holds references to two objects owned by the task executor:
//	(1) The DE globals object
//	(2) The control statements' (CONTROL TABLE/QUERY SHAPE) dynamic SQL 
//	    statement container.
//
//	Every unit owns:
//	(1) Its own dynamic SQL container, to handle the statements applied 
//	    by this unit.
//	(2) Statistics (log statistics, or statement application statistics).
//
//	The class defines the common behavior of the DE units:
//	(1) Every unit is *resettable*, i.e., its state variables can be
//		reset before the next duplicate elimination phase.
//	(2) Every unit can be configured by a number of SQL statements
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDupElimTaskExUnit {

public:
	CRUDupElimTaskExUnit(
		const CRUDupElimGlobals &dupElimGlobals,
		CRUSQLDynamicStatementContainer &ctrlStmtContainer,
		Int32 nStmts);
	
	virtual ~CRUDupElimTaskExUnit() {}

public:
	void SetStatementText(short index, const CDSString &sql)
	{
		stmtContainer_.SetStatementText(index, sql);
	}
	void PrepareStatement(short index)
	{
		stmtContainer_.PrepareStatement(index);
	}

	// Reset the state variables before the next phase.
	// To be implemented by the child classes
	virtual void Reset() = 0;

public:
	// These functions serialize/de-serialize the unit's context 
	// for the message communication with the remote server process
	
	// Used at the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator)
	{
		stmtContainer_.StoreData(translator);
	}
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator) = 0;
	
	// Used at the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator)
	{
		stmtContainer_.LoadData(translator);
	}
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator) = 0;

protected:
	const CRUDupElimGlobals &GetDupElimGlobals() const
	{
		return dupElimGlobals_;
	}

	CRUSQLDynamicStatementContainer &GetControlStmtContainer() const
	{
		return ctrlStmtContainer_;
	}

	CDMPreparedStatement *GetPreparedStatement(short index) 
	{
		return stmtContainer_.GetPreparedStatement(index);
	}

private:
	const CRUDupElimGlobals &dupElimGlobals_;

	// The unit's statement container
	CRUSQLDynamicStatementContainer stmtContainer_;

	// The control statement container: a reference
	CRUSQLDynamicStatementContainer &ctrlStmtContainer_;
};

// Forward reference
class CRUDupElimLogScanner;

//--------------------------------------------------------------------------//
//	CRUDupElimResolver
//
//	CRUDupElimResolver is a base abstract class for the two resolver
//	classes: the single-row resolver and the range resolver.
//	Each resolver class solves a different kind of duplicate conflicts,
//	and translates its decisions into the appropriate IUD statements.
//	
//	The class defines the common (virtual method) framework for both
//	resolvers (see the documentation for CRUDupElimTaskExecutor for
//	the general architecture description):	
//
//	(1) Resolve() - a single resolution step.
//	(2) CanCompletePhase() - does the resolver permit the task executor
//		to complete the current DE phase (and commit the transaction).
//	(3) PrepareSQL() - prepare (together) all the IUD statements in the SQL
//		container.
//	(4) DumpPerformanceStatistics() - print the data about all the 
//		IUD statements applied by the unit.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDupElimResolver : public CRUDupElimTaskExUnit {

private:
	typedef CRUDupElimTaskExUnit inherited;

public:
	CRUDupElimResolver(
		const CRUDupElimGlobals &dupElimGlobals,
		CRUSQLDynamicStatementContainer &ctrlStmtContainer,
		Int32 nStmts) :
	inherited(dupElimGlobals, ctrlStmtContainer, nStmts),
	canCompletePhase_(FALSE)
	{}

	virtual ~CRUDupElimResolver() {}

	//------------------------------------//
	//	Accessors
	//------------------------------------//
public:
	// Do I permit the outer mechanism to commit?
	BOOL CanCompletePhase() const
	{
		return canCompletePhase_;
	}

#ifdef _DEBUG
	virtual void DumpPerformanceStatistics(CDSString &to) const = 0;
#endif

	//------------------------------------//
	//	Mutators
	//------------------------------------//
public:
	void SetCanCompletePhase(BOOL flag)
	{
		canCompletePhase_ = flag;
	}

	//-- Resolve the next duplicate conflict (including the write to disk).
	virtual void Resolve(CRUDupElimLogScanner &scanner) = 0;

	// Prepare all the SQL statements in the container (together)
	virtual void PrepareSQL() = 0;

protected:
	enum LogType {

		IUD_LOG	= 1,
		RNG_LOG	= 2
	};

	// PrepareSQL() callees

	// Execute the Control Query Shape statement
	// that will force the optimizer to employ MDAM
	void ExecuteCQSForceMDAM(LogType logType);
	// ... and reset the settings
	void ExecuteCQSOff();

#ifdef _DEBUG
	// DumpPerformanceStatistics() callee
	void DumpStmtStatistics(CDSString &to, CDSString &stmt, Lng32 num) const;
#endif

private:
	BOOL canCompletePhase_;
};

#endif
