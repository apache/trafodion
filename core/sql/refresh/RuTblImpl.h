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
#ifndef _RU_TBL_IMPL_H_
#define _RU_TBL_IMPL_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuTblImpl.h
* Description:  Definition of classes CRURegularTbl and CRUMVTbl
*				(derived from CRUTbl)
*
* Created:      03/21/2000
* Language:     C++
*
* 
******************************************************************************
*/

#include "ddtable.h"
#include "ddMV.h"

#include "RuTbl.h"

//--------------------------------------------------------------------------//
//	CRURegularTbl (inherits from CRUTbl)
//
//	Implementation of CRUTbl when the used object is a regular table.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRURegularTbl : public CRUTbl {

private:
	typedef CRUTbl inherited;

public:
	CRURegularTbl(CDDTable *pddTable);
	virtual ~CRURegularTbl() {}

	//--------------------------------//
	//  Accessors
	//--------------------------------//
public:
	virtual const CDSString& GetFullName()
	{
		return pddTable_->GetFullName();
	}
	virtual const CDSString &GetCatName()
	{
		return pddTable_->GetCatName();
	}
	virtual const CDSString &GetSchName()
	{
		return pddTable_->GetSchName();
	}
	virtual const CDSString &GetName()
	{
		return pddTable_->GetName();
	}
	virtual BOOL IsAudited()
	{
		return pddTable_->GetIsAudited();
	}
	virtual BOOL IsRegularTable() const
	{
		return TRUE;
	}
	virtual BOOL IsFullySynchronized()
	{
		return TRUE;
	}
	virtual BOOL IsInitialized() 
	{ 
		return TRUE; 
	}

	//--------------------------------//
	//  Mutators
	//--------------------------------//
public:
	// Flush the metadata changes to the SMD and UMD tables
	virtual void SaveMetadata();

	//-- Read-protected opens 
	virtual void ExecuteReadProtectedOpen();
	virtual void ReleaseReadProtectedOpen();

protected:
	// Get the store by key column list 
	virtual CDDKeyColumnList *GetDDKeyColumnList()
	{
		return pddTable_->GetKeyColumnList();
	}

	// Get the list of detailed info about all the columns
	virtual CDDColumnList *GetDDColumnList()
	{
		return pddTable_->GetColumnList();
	}

	virtual CUOFsFileList *GetPartitionFileList()
	{
		return pddTable_->GetPartitionFileList();
	}

protected:
	virtual CDDLockList *GetDDLLockList()
	{
		return pddTable_->GetDDLLockList();
	}

	virtual CDDLock* AddDDLLock(CDSString lockName, CDDObject::EOperationType op,
 					 const CDDDetailTextList &details, CDSLocationList *locationList,
 					 Int32 status)
        {
		return pddTable_->AddDDLLock(lockName, op, details, locationList, status);
	}

	virtual void DropDDLLock(const CDSString &name)
	{
		pddTable_->DropDDLLock(name);
	}

private:
	//-- Prevent copying
	CRURegularTbl(const CRURegularTbl& other);
	CRURegularTbl& operator = (const CRURegularTbl& other);

private:
	CDDTable *pddTable_;
};

//--------------------------------------------------------------------------//
//	CRUMVTbl (inherits from CRUTbl)
//
//	Implementation of CRUTbl when the used object is an MV itself.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUMVTbl : public CRUTbl {

private:
	typedef CRUTbl inherited;

public:
	CRUMVTbl(CDDMV *pddMV);
	virtual ~CRUMVTbl() {}

	//--------------------------------//
	//  Accessors
	//--------------------------------//
public:
	virtual const CDSString& GetFullName()
	{
		return pddMV_->GetFullName();
	}
	virtual const CDSString &GetCatName()
	{
		return pddMV_->GetCatName();
	}
	virtual const CDSString &GetSchName()
	{
		return pddMV_->GetSchName();
	}
	virtual const CDSString &GetName()
	{
		return pddMV_->GetName();
	}
	virtual BOOL IsRegularTable() const
	{
		return FALSE;
	}
	virtual BOOL IsFullySynchronized()
	{
		return (CDDObject::eON_STATEMENT == pddMV_->GetRefreshType());
	}
	virtual BOOL IsAudited()
	{
		return (CDDObject::eAUDIT == pddMV_->GetAuditType());
	}
	
	virtual BOOL IsInitialized()
	{
		return (CDDObject::eINITIALIZED == pddMV_->GetMVStatus()); 
	}

	//--------------------------------//
	//  Mutators
	//--------------------------------//
public:
	// Flush the metadata changes to the SMD and UMD tables
	virtual void SaveMetadata();

	virtual void ExecuteReadProtectedOpen();
	virtual void ReleaseReadProtectedOpen();

protected:
	// Get the store by key column list 
	virtual CDDKeyColumnList *GetDDKeyColumnList()
	{
		return pddMV_->GetKeyColumnList();
	}

	// Get the list of detailed info about all the columns
	virtual CDDColumnList *GetDDColumnList()
	{
		return pddMV_->GetColumnList();
	}

	// Get the list of all partitions
	virtual CUOFsFileList *GetPartitionFileList()
	{
		return pddMV_->GetPartitionFileList();
	}

protected:
	virtual CDDLockList *GetDDLLockList()
	{
		return pddMV_->GetDDLLockList();
	}

	virtual CDDLock* AddDDLLock(CDSString lockName, CDDObject::EOperationType op,
 					 const CDDDetailTextList &details, CDSLocationList *locationList,
 					 Int32 status)
	{
		return pddMV_->AddDDLLock(lockName, op, details, locationList, status);
	}

	virtual void DropDDLLock(const CDSString &name)
	{
		pddMV_->DropDDLLock(name);
	}

private:
	//-- Prevent copying
	CRUMVTbl(const CRUMVTbl& other);
	CRUMVTbl& operator = (const CRUMVTbl& other);

private:
	CDDMV *pddMV_;
};

#endif
