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
#ifndef _RU_GLOBALS_H_
#define _RU_GLOBALS_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuGlobals.h
* Description:  Definition of class CRUGlobals
*
*
* Created:      08/20/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "refresh.h"

#include "uofsTransaction.h"

// Statements for standard printing into the output file
extern const char *RefreshDiags[];

class CDSString;
class CRUOptions;
class CRUJournal;
class CUOFsIpcMessageTranslator;

//--------------------------------------------------------------------------//
//	CRUGlobals
//	
//	The CRUGlobals class implements a Singleton design pattern to provide 
//	the access to the utility's global data (options, output file etc.).
//	
//	The class also provides the *testpoint* mechanism which allows 
//	to simulate the runtime failures of the utility. The programmer can define 
//	locations in the utility's code where exceptions may be thrown to simulate
//	the failure. The exception will be triggered if the user passes through the 
///	utility's DEBUG option the testpoint's unique number, and optionally 
//	the object name to apply the testpoint to. The whole mechanism is available
//	 only in the debug version.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUGlobals {

	//-- The Singleton's implementation
public:
	static CRUGlobals *GetInstance();
	
	static void Init(CRUOptions &options, 
					 CRUJournal &journal,
					 CUOFsTransManager &transManger);

	static void Done()
	{
		delete pInstance_;
		pInstance_ = NULL;
	}

public:
	static TInt64 GetCurrentTimestamp();

	static CDSString GetCurrentUser();

	//-- Accessors
public:
	CRUOptions &GetOptions() const
	{
		return options_;
	}

	CRUJournal &GetJournal() const
	{
		return journal_;
	}

	CUOFsTransManager &GetTransactionManager() const
	{
		return transManager_;
	}

        char *getParentQid() 
        {
          return parentQid_;
        }


        static void setParentQidAtSession(char *parentQid);

        //-- Testpoint mechanism
public:
	enum TestpointId {

		// Dump the data structures (dependence graph, cache etc).
		DUMP_DS = 1,
		// Display the queris used by the DE task in range resoultion
		DISPLAY_DE_RR = 2,
		// Display the queris used by the DE task in single resoultion
		DISPLAY_DE_SR = 3,
		// Display the select query used by the DE task 
		DISPLAY_DE_SEL = 4,

		// Instead of delete records use Always ignore option
		DE_USE_IGNORE_ALWAYS = 40,
		// If this option is on, every compilation of 
		// dynamic sql is logged 	
		DUMP_COMPILED_DYNAMIC_SQL = 51,
		// If this option is on the utility reports
		// times of each step in its execution
		DUMP_EXECUTION_TIMESTAMPS = 52,
		// Pop-up the Display tool for Internal Refresh
		DISPLAY_REFRESH			  = 53,
		// Print the number and distribution of IUD statements 
		// performed by the Duplicate Elimination task.
		DUMP_DE_STATISTICS		  = 54,
		// Force the Duplicate Elimination task executor
		// to solve all the possible conflicts (even if 
		// there is no MJV customer).
		ENFORCE_FULL_DE			  = 55,
		// Report about every lock (RP open) taken and released
		DUMP_LOCKS				  = 56,
		// Force an artificially small initial IPC buffer
		SHRINK_IPC_BUFFER		  = 57,
		// Crash the remote process brutally in table sync executor
		REMOTE_CRASH_IN_TABLE_SYNC= 58,
		// Turn the statistics usage by the scheduler off
		IGNORE_STATISTICS		  = 59,
		// Crash the remote process brutally in the remote 
		// controller initialization
		REMOTE_CRASH_IN_REMOTE_CONTROLLER = 60,
		// Force the artificially small time limit
		// for a single DE transaction
		SHRINK_DE_TXN_TIMELIMIT	= 61,
		// Force a system error in the (general) Refresh task executor
		SEVERE_REFRESH_CRASH = 62,
		SEVERE_PREPARE_CRASH = 63,
		// UnAuditedExecutor prints
		DUMP_PURGEDATA = 65,
		DUMP_POPINDEX = 66,
                // Report the DDL lock names created and released
                DUMP_DDL_LOCKS = 67,
		// 
		// UnAuditedExecutor test points
		TESTPOINT100 = 100,
		TESTPOINT101 ,
		TESTPOINT102 ,
		TESTPOINT103 ,
		TESTPOINT104 ,
		TESTPOINT105 ,
		TESTPOINT106 ,
		TESTPOINT107 ,
		// End UnAuditedExecutor test points

		// DDL locks release failure
		TESTPOINT112 = 112,

		// MultiTxnExecutor test points
		TESTPOINT120 = 120,
		TESTPOINT121 ,
		TESTPOINT122 ,
		TESTPOINT123 ,
		TESTPOINT124 ,
		TESTPOINT125 ,
		TESTPOINT126 ,
		TESTPOINT127 ,
		// End MultiTxnExecutor test points

		// AuditExecutor test points
		TESTPOINT130 = 130,
		TESTPOINT131,
		// End AuditExecutor test points
		
		// TableSyncExecutor test points
		TESTPOINT140 = 140, // Before epoch increment
		TESTPOINT141,		// After  epoch increment
		// End TableSyncExecutor test points

		// DupElimExecutor testpoints
		TESTPOINT150 = 150,	// After the initial metadata update
		TESTPOINT151,		// After phase <n> completion (accepts parameter)
		TESTPOINT152,		// After final metadata update	
		// End DupElimExecutor testpoints

		// LogCleanupExecutor testpoints
		TESTPOINT160 = 160,
		// End LogCleanupExecutor testpoints

		// Cache builder testpoints
		TESTPOINT170 = 170,	// Before the commit
		TESTPOINT171 = 171,	// After the commit
		// End cache builder testpoints
		
		// LockEquivSetTaskExecutor testpoints
		TESTPOINT173 = 173, // Before the begin
		TESTPOINT174 = 174  // After the commit
		// End LockEquivSetTaskExecutor testpoints

	};

	// Planting the exceptions

	// A "legal" exception
	void Testpoint(Int32 testpointId, const CDSString &objName);
	// A system exception (not a CDSException object)
	void TestpointSevere(Int32 testpointId, const CDSString &objName);

	void LogMessageWithTime(const char* msg);

	void LogDebugMessage(Int32 testpointId, 
						 const CDSString &objName, 
						 const CDSString &msg,
						 BOOL printRowNum = FALSE);

private:
        void getCurrentParentQid();

        //-- Prevent the direct object creation
	CRUGlobals();
        ~CRUGlobals();
	CRUGlobals(CRUOptions &options, 
			   CRUJournal &journal,
			   CUOFsTransManager &transManger);

	// The singleton object
	static CRUGlobals *pInstance_;

	CRUOptions &options_;
	CRUJournal &journal_;
	CUOFsTransManager &transManager_;
        char *parentQid_;
};

//------------------------------------------------------//
//	TESTPOINT macro definitions
//------------------------------------------------------//

#ifdef _DEBUG

#define TESTPOINT(testpointId) \
{ \
	CRUGlobals *pGlobals = CRUGlobals::GetInstance();\
	CDSString objName("");\
	pGlobals->Testpoint((testpointId), objName);\
}

#define TESTPOINT2(testpointId, objName) \
{ \
	CRUGlobals *pGlobals = CRUGlobals::GetInstance();\
	pGlobals->Testpoint((testpointId), (objName));\
}

#define TESTPOINT_SEVERE(testpointId) \
{ \
	CRUGlobals *pGlobals = CRUGlobals::GetInstance();\
	CDSString objName("");\
	pGlobals->TestpointSevere((testpointId), (objName));\
}

#define TESTPOINT_SEVERE2(testpointId, objName) \
{ \
	CRUGlobals *pGlobals = CRUGlobals::GetInstance();\
	pGlobals->TestpointSevere((testpointId), (objName));\
}

#else

#define TESTPOINT(testpointId)
#define TESTPOINT2(testpointId, objName) 
#define TESTPOINT_SEVERE(testpointId)
#define TESTPOINT_SEVERE2(testpointId, objName)

#endif // _DEBUG

#define LOGTIME(msg) \
{ \
	CRUGlobals *pGlobals = CRUGlobals::GetInstance();\
	pGlobals->LogMessageWithTime(msg);\
}

#endif
