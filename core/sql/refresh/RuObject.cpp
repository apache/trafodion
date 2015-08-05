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
* File:         RUObject.cpp
* Description:  Implementation of class CRUObject 
*
* Created:      03/13/2000
* Language:     C++
*
* 
******************************************************************************
*/
#include "ddlock.h"
#include "RuObject.h"
#include "RuSQLComposer.h"
#include "CatSQLShare.h"

//--------------------------------------------------------------------------//
//	Globals
//--------------------------------------------------------------------------//

CDSString const CRUObject::DDLLockSuffix = "__REFRESH__DDLLOCK";

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUObject::CRUObject() : 
	isInvolved_(FALSE),
	isDDLLockPending_(FALSE),
	canReleaseDDLLock_(TRUE),
	ex_()
{}

//--------------------------------------------------------------------------//
//	CRUObject::HandleDDLLockInCache()
//
//	CRUCache::HandleDDLLock() callee.
//
//	Handle (acquire/replace/cancel) the DDL lock associated with an
//	involved MV or table, in order to provide mutual exclusion from
//	the concurrent DDL and utility operations.
//	
//	If the utility is applied with the CANCEL option, it will only try
//	to cancel the DDL lock associated with the object. If there is none,
//	the operation is simply skipped.
//
//	Otherwise, it will try to cancel the DDL lock (if exists), and to
//	create its own one. If both operations are non-empty and succeed,
//	this is actually a replacement of a stale DDL lock.

//--------------------------------------------------------------------------//

void CRUObject::HandleDDLLockInCache(BOOL isCancelOnly)
{
	// Nothing is supposed to go wrong so far...
	RUASSERT(0 == GetStatus());

	// Drop the previously existing DDL lock (if there was one)
	CancelDDLLock(isCancelOnly);

	if (TRUE == isCancelOnly	// No more work
		|| 
		0 != GetStatus())		// Something went wrong, do not continue
	{
		return;
	}

	RUASSERT(TRUE == GetDDLLockList()->IsEmpty());

	// Create a new DDL lock
	CreateDDLLock();
}

//--------------------------------------------------------------------------//
//	CRUObject::ReleaseDDLLock()
//
//	Called (indirectly) by CRURcReleaseTaskExecutor::Work().
//
//	Drop the DDL lock that was created at the cache building stage.
//
//--------------------------------------------------------------------------//

void CRUObject::ReleaseDDLLock()
{
	// The DDL lock's drop is done through the DDOL Save() API.
	// If some DDL operation on this object failed at an earlier stage,
	// the DDOL object is contaminated, and Save() will fail.
	// Reset the modify flag on the object.
	CancelChanges();

	// Verify that there is a single DDL lock ...
	CDDLockList *pLockList = GetDDLLockList();
	RUASSERT(1 == pLockList->GetCount());

	// And we have put it
	CDDLock *pLock = pLockList->GetAt(0);
	RUASSERT(CDDObject::eREFRESH == pLock->GetOperation());

#ifdef _DEBUG        
	CDSString msg(
          "\nDDL lock released: " + pLock->GetName() + "\n" ); 

	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_DDL_LOCKS,"",msg);
#endif

        // something very bad went wrong, just raise an error and quit
        if (!pLock)
        {
          SetDDLLockError(IDS_RU_DDLOCK_RELEASE_IMPOSSIBLE);
          return;
        }

	// Go !
	DropDDLLock(pLock->GetName());
	SaveMetadata();

	isDDLLockPending_ = FALSE;
}

//--------------------------------------------------------------------------//
//	CRUObject::CancelDDLLock()
//
//	CRUCache::HandleDDLLock() callee
//
//	Drop the DDL lock associated with the object (if one exists).
//	The lock can be dropped under the following conditions:
//	(1) It is a SINGLE DDL lock on this object.  
//	(2) It was cast by some earlier invocation of REFRESH.
//	(3) The Catman process that has aquired it is not running.
//	(4) If the utility does not mean to re-acquire this lock
//		(i.e., it is invoked with the CANCEL option), this
//		operation must be legal (for example, a DDL lock cannot
//		be dropped from an MV in the non-available state).
//
//--------------------------------------------------------------------------//

void CRUObject::CancelDDLLock(BOOL isCancelOnly)
{
	RUASSERT(FALSE == IsDDLLockPending());

	CDDLockList *pLockList = GetDDLLockList();

	switch (pLockList->GetCount())
	{
	case 0:
		{
			return;	// Nothing to cancel
		}
	case 1:	
		{	
			CDDLock *pLock = pLockList->GetAt(0);
			if (CDDObject::eREFRESH != pLock->GetOperation())
			{
				// Condition #2
				SetDDLLockError(IDS_RU_FOREIGN_DDLLOCK);
				break;
			}

			if (isRunning ==	// CatSQLShare enum
				CatProcessIsRunning(pLock->GetProcessId()))
			{
				// Condition #3
				SetDDLLockError(IDS_RU_DDLLOCK_PROC_IS_RUNNING);
				break;
			}

			if (TRUE == isCancelOnly
				&&
				FALSE == CanReleaseDDLLock())
			{
				// Condition #4
				SetDDLLockError(IDS_RU_DDLOCK_CANCEL_IMPOSSIBLE);
				break;
			}

			// Go!
			DropDDLLock(pLock->GetName());
			SaveDDLLockOperation();

			break;
		}
	default:
		{
			// Condition #1.
			// Cannot handle multiple DDL locks. 
			
			// Maybe this should never happen? Assertion?
			SetDDLLockError(IDS_RU_MULTI_DDLLOCKS);
			break;
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUObject::CreateDDLLock()
//
//	Create a new DDL lock, using the following naming convention:
//	<object-name>__REFRESH__DDLLOCK.
//
//	WARNING! If shared DDL locks will be supported once, the naming 
//	convention must be more sophisticated.
//
//--------------------------------------------------------------------------//

void CRUObject::CreateDDLLock()
{
   // Set up dummy (default) parameter values
   CDDDetailTextList details;
	CDSLocationList *pLocationList = NULL;
   Int32 status = 0;
   CreateDDLLock(details, pLocationList, status);
}

void CRUObject::CreateDDLLock(const CDDDetailTextList &details,
									   CDSLocationList *pLocationList,
 					 					Int32 status)
{	
	// Write through the DDOL
	CDDLock *pLock = AddDDLLock("", CDDObject::eREFRESH, details, pLocationList, status);
  	SaveDDLLockOperation();

#ifdef _DEBUG        
	CDSString msg(
          "\nDDL lock created: " + pLock->GetName() + "\n" ); 

	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_DDL_LOCKS,"",msg);
#endif

	if (0 == GetStatus())
	{
		isDDLLockPending_ = TRUE;
	}
}

//--------------------------------------------------------------------------//
//	CRUObject::SaveDDLLockOperation()
//
//	Apply the CatApi request (through the DDOL) to save the 
//	creation/drop of DDL lock to the catalog.
//
//	The call to SaveMetadata() is enclosed in a try ... catch
//	clause, which is meant to idenitify the deadlock of the 
//	following type during the concurrent build of cache in two
//	invocations of the utility:
//
//	(1) Invocation A of Refresh performs a serializable read 
//		of the row that corresponds to the object from the Objects
//		SMD table (basic DDOL mechanism).
//	(2) Invocation B performs the same read.
//	(3) Both invocations eventually try to put a DDL lock 
//		on the object - and enter a deadlock.
//
//	The deadlock will finally result in a timeout. While the utility 
//	does not prevent this conflict - it prevents that the whole cache 
//	construction will crash because of it.
//
//--------------------------------------------------------------------------//

void CRUObject::SaveDDLLockOperation()
{
	RUASSERT(0 == GetStatus());

	try 
	{
		SaveMetadata();
	}
	catch (CDSException &ex)
	{
		ex_ = ex;	// Store the exception
	}
}
