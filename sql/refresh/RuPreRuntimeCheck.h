/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1999-2015 Hewlett-Packard Development Company, L.P.
//
// @@@ END COPYRIGHT @@@
//
**********************************************************************/
#ifndef _RU_PRE_RUNTIME_CHECK_H_
#define _RU_PRE_RUNTIME_CHECK_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuPreRuntimeCheck.h
* Description:  Definition of class CRUPreRuntimeCheck
*
*				Edward Bortnikov
* Created:      10/10/2000
* Language:     C++
*
*
******************************************************************************/

#include "refresh.h"
#include "dsstring.h"

class CRUMV;
class CRURefreshTask;
class CRUException;

//--------------------------------------------------------------------------//
//	CRUPreRuntimeCheck
//
//	This class encapsulates the algorithm of a pre-runtime
//	check of a single MV. It is performed at the time of
//	the dependence graph's construction. 
//
//	The checks performed are as follows:
//	1. DDL locks checks
//	   - There should be no problem (in cache construction time) about 
//		acquiring the DDL locks for the MV or the used table(s).
//	2. Privilege checks
//	   - The current user must have the INSERT privilege the MV.
//	   - If indexes must be populated, the user must have the SELECT privilege.
//	   - If purgedata is performed, the user must have SELECT+DELETE.
//  3. Initialization checks
//	   - No re-initialization of ON STATEMENT MV.
//	   - A non-involved MV used by involved MV(s) 
//	     must be initialized.
//	4. Consistency checks (only in a single-MV refresh):
//	   - ON REQUEST and RECOMPUTE MVs that are based on 
//	     more than one MV cannot be refreshed standalone.
//     - RECOMPUTED MV based on some other MV cannot have 
//       any other object beneath it.
//     - ON REQUEST MV based on some other MV must observe 
//       empty table-deltas (this check can be done only in 
//	     runtime, after the EmpCheck task).
//	5. NO LOCKONREFRESH checks
//	   - An ON REQUEST/ON STATEMENT MV on top of 
//	     NO LOCKONREFRESH table cannot be recomputed.
//	6. Auditing checks.
//	   - An ON REQUEST/ON STATEMENT MV on top of non-audited 
//	     regular table cannot be refreshed.
//	7. Unavailable recompute checks.
//	   - If a non-available non-audited MV is recomputed, 
//	     the user must specify the RECOMPUTE option explicitly.
//
//	If any of the tests fails, the algorithm will mark 
//	the appropriate error on the Refresh task's object,
//	which will prevent the refresh of this MV and the
//	dependent ones.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUPreRuntimeCheck {

public:
	CRUPreRuntimeCheck(CRURefreshTask &refreshTask);
	~CRUPreRuntimeCheck() {}

public:
	void PerformCheck();

private:
	CRURefreshTask &refreshTask_;
	CRUMV &mv_;
	CDSString mvName_;
	CRUException &errDesc_;

private:
	// PerformCheck() callees
	BOOL CheckDDLLockErrors();
	BOOL CheckPrivileges();
	BOOL CheckUsedObjectsInitialization();
	BOOL CheckOnStatementMVReInit();
	BOOL CheckConsistency();
	BOOL CheckUsedTablesNoLockOnRefresh();
	BOOL CheckUsedTablesAuditing();
	BOOL CheckNonAvailableMVRecompute();
	
	// CheckConsistency() callee
	BOOL CheckConsistencyIfSingleMVUsed();
};

#endif
