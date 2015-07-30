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
#ifndef _RU_OBJECT_H_
#define _RU_OBJECT_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RUObject.h
* Description:  Definition of class CRUObject 
*
* Created:      03/13/2000
* Language:     C++
*
* 
******************************************************************************
*/

#include "refresh.h"
#include "dsplatform.h"
#include "dslocation.h"
#include "ddobject.h"
#include "RuException.h"

class CDDLockList;
class CDDLock;

//--------------------------------------------------------------------------//
//	CRUObject
//
//	  Abstract base class for the REFRESH utility's wrapper classes 
//	  for MV, table, and index .The class provides a skeleton  
//	  for the common interface of the wrapper objects.
//
//	  An object can be *involved* and *non-involved* into the execution
//	  of the utility (see RUCache.h for exact definitions).
//
//	  The class provides the functionality for storing error messages 
//	  if an error occurs to the object (in an internal CRUException object), 
//	  and for managing DDL locks to provide mutual exclusion between 
//	  concurrent invocations of the utility.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUObject {

public:
	CRUObject();
	virtual ~CRUObject() {}

	//--------------------------------//
	//  Accessors
	//--------------------------------//
public:
	virtual TInt64 GetUID() const = 0;
	virtual const CDSString& GetFullName() = 0;

	virtual const CDSString &GetCatName() = 0;
	virtual const CDSString &GetSchName() = 0;
	virtual const CDSString &GetName() = 0;

	virtual TInt64 GetTimestamp() const = 0;

public:
	// Is this object involved into the utility's execution?
	// (either a refreshed MV, or a table used by a refreshed MV) 
	BOOL IsInvolved() const 
	{ 
		return isInvolved_; 
	}

	CRUException &GetErrorDesc() 
	{ 
		return ex_; 
	}
	Lng32 GetStatus() 
	{ 
		return ex_.GetStatus(); 
	}

	//-- DDL locks management
public:
	BOOL IsDDLLockPending() const
	{
		return isDDLLockPending_;
	}

	// To be implemented by the child classes
	virtual BOOL HoldsResources() const = 0;

	// Is there any problem that the utility 
	// will release the DDL lock on the object?
	BOOL CanReleaseDDLLock() const
	{
		return canReleaseDDLLock_;
	}

#ifdef _DEBUG
public:
	virtual void Dump(CDSString &to, BOOL isExtended=FALSE) = 0;
#endif

	//--------------------------------//
	//  Mutators
	//--------------------------------//
public:
	void SetInvolved()	
	{ 
		isInvolved_ = TRUE; 
	}

	virtual void SetTimestamp(TInt64 ts) = 0;

public:
	// Fetch the REFRESH-specific metadata
	// from the SMD and UMD tables
	virtual void FetchMetadata() = 0;

	// Save the metadata updates to the catalog
	virtual void SaveMetadata() = 0;

	// Prevent DDOL from accidentally trying to save
	// a dirtied object that the utility did not save voluntarily
	virtual void CancelChanges() = 0;

public:
	// CACHE BUILDER CALLEES

	// Set/unset the permission to release the DDL lock
	void SetReleaseDDLLock(BOOL flag)
	{
		canReleaseDDLLock_ = flag;
	}

	// Release the old DDL lock (if exists) + (optionally) acquire a new one.
	void HandleDDLLockInCache(BOOL isStandaloneCancel);

	// RESOURCE RELEASE TASK EXECUTOR'S CALLEES

	// Release the DDL  lock(and, optionally, read-protected open(s))
	// associated with this object.
	virtual void ReleaseResources() = 0;

public:
	// To be used by ReleaseResources() in the child classes
	// also used in the Epilogue of RuUnAuditRefreshTask
	void ReleaseDDLLock();

	void CreateDDLLock();
	void CreateDDLLock(const CDDDetailTextList &details,
                CDSLocationList *locationList = NULL,
 					 Int32 status = 0);

protected:
	virtual CDDLockList *GetDDLLockList() = 0;

	virtual CDDLock *AddDDLLock(CDSString lockName, 
                                    CDDObject::EOperationType op,
 				    const CDDDetailTextList &details,
                                    CDSLocationList *locationList,
 				    Int32 status) =0;

	virtual void DropDDLLock(const CDSString &name) = 0;

private:
	// Drop the DDL lock associated with the object,
	// if one exists and conditions permit.
	void CancelDDLLock(BOOL isCancelOnly);

	// Make the DDL lock change persistent
	void SaveDDLLockOperation();

	void SetDDLLockError(Lng32 errnum)
	{
		ex_.SetError(errnum);
		ex_.AddArgument(GetFullName());
	}

private:
	BOOL isInvolved_;

	BOOL isDDLLockPending_;
	BOOL canReleaseDDLLock_;

	// Error description.
	// Used to convey the information about the DDL lock handling
	CRUException ex_;

	// The unique name suffix for all the DDL locks created by the utility
	static const CDSString DDLLockSuffix;
};

#endif
