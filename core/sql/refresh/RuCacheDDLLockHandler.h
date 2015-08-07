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
* File:         RuCacheDDLLockHandler.h
* Description:  Definition of class CRUCacheDDLLockHandler.
*				
*
* Created:      02/10/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#ifndef _RU_CACHE_DDLLOCK_H_
#define _RU_CACHE_DDLLOCK_H_

#include "refresh.h"
#include "dsmap.h"

#include "RuObject.h"

//--------------------------------------------------------------------------//
//	CRUCacheDDLLockHandler
//
//	This class performs the handling (cancellation and aquirement)
//	of DDL locks in the cache. The new DDL locks must be created 
//	in the growing order of UIDs, in order to prevent a deadlock
//	with the other (concurrent) invocation of Refresh.
//		
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUCacheDDLLockHandler {

public:
	CRUCacheDDLLockHandler();
	virtual ~CRUCacheDDLLockHandler();

public:
	BOOL DidDDLLockErrorsHappen() const
	{
		return didDDLLockErrorsHappen_;
	}

public:
	void AddObject(CRUObject *pObj);

	// The main method
	void HandleDDLLocks(BOOL isCancelOnly);

private:
	//-- Prevent copying --//
	CRUCacheDDLLockHandler(const CRUCacheDDLLockHandler &other);
	CRUCacheDDLLockHandler & operator= (const CRUCacheDDLLockHandler &other);

private:
	void SortObjectsByUid();
	
	// Sorting criteria
	static Int32 CompareElem(const void *pEl1, const void *pEl2);

private:
	// Do we need to handle the DDL locks at all?
	BOOL doHandle_;

	// Any problems about canceling the DDL locks?
	BOOL didDDLLockErrorsHappen_;

	struct ObjectLink {

		ObjectLink(CRUObject *pObj) : 
			pObj_(pObj), uid_(pObj->GetUID()) 
		{}

		TInt64 uid_;
		CRUObject *pObj_;
	};
	
	typedef ObjectLink * PObjectLink;
	enum { HASH_SIZE = 101 };

	CDSTInt64Map<ObjectLink *> objMap_;

	// The array of pointers to links (will be sorted by the object UID)
	PObjectLink *pObjSortedArray_;
};

#endif
