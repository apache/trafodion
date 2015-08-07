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
* File:         RuCacheDDLLockHandler.cpp
* Description:  Implementation of class CRUCacheDDLLockHandler.
*				
*
* Created:      02/10/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuCacheDDLLockHandler.h"

#include "RuGlobals.h"
#include "RuOptions.h"
#include "RuJournal.h"

//--------------------------------------------------------------------------//
//	Constructor 
//--------------------------------------------------------------------------//

CRUCacheDDLLockHandler::CRUCacheDDLLockHandler() :
	doHandle_(TRUE),
	didDDLLockErrorsHappen_(FALSE),
	pObjSortedArray_(NULL),
	objMap_(HASH_SIZE)
{
	CRUGlobals *pGlobals = CRUGlobals::GetInstance(); 
	CRUOptions &options = pGlobals->GetOptions();

	if (CRUOptions::DO_ONLY_LC == options.GetLogCleanupType())
	{
		// When the utility is applied with DO ONLY LOG CLEANUP option,
		// DDL locks are not required.
		doHandle_ = FALSE;
	}

#ifdef _DEBUG
	CRUOptions::DebugOption *pDO = 
		options.FindDebugOption(CRUGlobals::DUMP_DS, "");

	if (NULL != pDO)
	{
		doHandle_ = FALSE;	// The invocation is for the DS dump only
	}
#endif
}

//--------------------------------------------------------------------------//
//	Destructor 
//--------------------------------------------------------------------------//

CRUCacheDDLLockHandler::~CRUCacheDDLLockHandler()
{
	// Free the sorted array of pointers
	if (NULL != pObjSortedArray_)
	{
		delete [] pObjSortedArray_;
	}

	// Free the data owned by the hash table
	CDSMapPosition<ObjectLink *> pos;

	ObjectLink *pLink;
	TInt64 *uid;

	objMap_.GetStartPosition(pos);
	
	while (TRUE == pos.IsValid())
	{
		objMap_.GetNextAssoc(pos, uid, pLink);
		delete pLink;
	}

	objMap_.RemoveAll();
}

//--------------------------------------------------------------------------//
//	CRUCacheDDLLockHandler::AddObject()
//
//	Store the pointer to the object in a temporary object (link),
//	which will be pointed from the hash table.
//
//	This is an ugly usage of a VERY ugly implementation of TInt64Map.
//
//--------------------------------------------------------------------------//

void CRUCacheDDLLockHandler::AddObject(CRUObject *pObj)
{
	if (FALSE == doHandle_)
	{
		return;
	}

	RUASSERT (NULL != pObj);

	ObjectLink *pLink = new ObjectLink(pObj);

	objMap_[&(pLink->uid_)] = pLink;
}

//--------------------------------------------------------------------------//
//	CRUCacheDDLLockHandler::HandleDDLLocks()
//
//	Handle (acquire/replace/cancel) the DDL lock associated with each
//	involved objects, in order to provide mutual exclusion from
//	the concurrent DDL and utility operations.
//
//	The traversal of involved objects is done in the UID order, for 
//	theoretical deadlock prevention (although the deadlock scenario 
//	is VERY unlikely).
//
//--------------------------------------------------------------------------//

void CRUCacheDDLLockHandler::HandleDDLLocks(BOOL isCancelOnly)
{
	if (FALSE == doHandle_)
	{
		return;
	}

	CRUJournal &journal = CRUGlobals::GetInstance()->GetJournal(); 

	SortObjectsByUid();

	for (Int32 i=0; i<objMap_.GetCount(); i++)
	{
		CRUObject *pObj = pObjSortedArray_[i]->pObj_;

		// Perform the CatApi request(s) ...
		pObj->HandleDDLLockInCache(isCancelOnly);

		if (0 != pObj->GetStatus())
		{
			// For some reason, the DDL lock could not be dropped.	
			if (TRUE == isCancelOnly)
			{
				// If this is the CANCEL mode, report to the outfile.
				journal.LogError(pObj->GetErrorDesc());

				didDDLLockErrorsHappen_ = TRUE;
			}
			else
			{
				// This is the "normal" mode of execution.
				// The error message will be copied from the object
				// to some Refresh task, and will be printed by the
				// flow controller.
			}
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUCacheDDLLockHandler::SortObjectsByUid()
//--------------------------------------------------------------------------//

void CRUCacheDDLLockHandler::SortObjectsByUid()
{
	RUASSERT(NULL == pObjSortedArray_);
	
	Lng32 size = objMap_.GetCount();

	// Copy the pointers to the links to the array first ...
	pObjSortedArray_ = new PObjectLink[size];

	CDSMapPosition<ObjectLink *> pos;

	ObjectLink *pLink;
	TInt64 *uid;

	objMap_.GetStartPosition(pos);

	for (Int32 i=0; TRUE == pos.IsValid(); i++)
	{
		objMap_.GetNextAssoc(pos, uid, pLink);
		
		pObjSortedArray_[i] = pLink;
	}

	// ... and sort the array
	qsort(pObjSortedArray_, size, sizeof(PObjectLink), CompareElem);
}

//--------------------------------------------------------------------------//
//	CRUCacheDDLLockHandler::CompareElem()
//
//	The function serves for the quicksort criteria.
//--------------------------------------------------------------------------//

Int32 CRUCacheDDLLockHandler::CompareElem(const void *pEl1, const void *pEl2)
{
	CRUCacheDDLLockHandler::ObjectLink *pLink1 = 
		*((CRUCacheDDLLockHandler::PObjectLink *)pEl1);

	CRUCacheDDLLockHandler::ObjectLink *pLink2 = 
		*((CRUCacheDDLLockHandler::PObjectLink *)pEl2);

	return (pLink1->uid_ < pLink2->uid_) ? -1 : 1;
}
