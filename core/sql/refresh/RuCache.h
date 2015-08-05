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
#ifndef _RU_CACHE_H_
#define _RU_CACHE_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuCache.h
* Description:  Definition of class CRUCache
*
* Created:      8/23/1999
* Language:     C++
*
*
*
******************************************************************************
*/

#include "refresh.h"

#include "dsplatform.h"
#include "ddsqlnode.h"

class CDDSchema;
class CDDMV;
class CDDTable;
class CDDTblUsedByMV;
class CDDMVGroup;
class CDDDefaults;

#include "RuCacheDDLLockHandler.h"
#include "RuMV.h"
#include "RuOptions.h"
#include "RuTblImpl.h"

//--------------------------------------------------------------------------//
// CRUCache
//   
//   The metadata storage class of the Refresh utility. 
// 
//   The only instance of this class will provide the utility's engine
//   with the interface for retrieval and manipulation of the: 
//     (1) The metadata wrapper objects (read/write)
//     (2) The system defaults (read only)
//
//   A metadata wrapper object (CRUMV/CRUTbl) contains a reference 
//	 to the Data Definition (DD) object (core) of an involved MV or table,
//	 as well as more data about this object related to a specific invocation 
//	 of REFRESH. The objects are constructed based on:
//		(1) Catalog data (SMD and UMD tables).
//		(2) The utility's command parameters.  
//	 Saving a wrapper object maps to saving one or more DD objects, which maps
//	 to one or more writes to SMD and UMD tables.
//
//	 In general, the internal layout of CRUCache is as follows.
//	 The class contains a CDDSqlNode data member that manages the cloud
//	 of DD objects. In addition, it contains two lists of wrapper objects
//	 (for tables and MVs - CRUMVList and CRUTblList, respectively); 
//	 Wrappers contain references into the CDDSqlNode cloud.
//	 These lists are exposed to the class user.
//
//	 CRUMV and CRUTbl objects stored in the cache can be *involved* and 
//	 *non-involved* into REFRESH. The MVs that are being refreshed and the
//	 objects used by them are considered involved, while the rest are not.
//	 The involved objects serve as a base for building the task set for 
//	 the dependence graph. The rest serve the other purposes (see the
//	 implementation comments).
//
//	 The involved objects maintain complete cross-connectivity, whereas
//	 the non-involved objects are only referenced by the involved ones 
//	 (but not vice-versa).
//
//	 Memory deallocation. Since the destructor CDDSqlNode handles the automatic 
//	 deallocation of all the DD objects in the cloud, CRUCache should only 
//	 take care to dispose the wrapper lists. 
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUCache {

public:
	CRUCache();
	virtual ~CRUCache();

	//--------------------------------------------//
	//  Accessors
	//--------------------------------------------//
public:
	CRUMVList &GetMVList() 
	{ 
		return mvList_; 
	}
	CRUMV *GetMV(TInt64 objUid) const;

	CRUTblList &GetTableList() 
	{ 
		return tableList_; 
	}
	CRUTbl *GetTable(TInt64 objUid) const;

	// Any problems about canceling the DDL locks during the build?
	BOOL DidDDLLockErrorsHappen() const
	{
		return ddlLockHandler_.DidDDLLockErrorsHappen();
	}

	CDDSqlNode * GetSqlNode()
	{
		return &sqlNode_;
	}

#ifdef _DEBUG
public:
	void Dump(CDSString& to, BOOL isExtended=FALSE) const;
#endif

public:
	//--  API to System Defaults -//

	// Maximum number of task processes
	TInt32 GetMaxParallelism() const 
	{ 
		return maxParallelism_; 
	}
	
	// Maximum number of MVs to be pipelined in one statement
	TInt32 GetMaxPipelining() const 
	{ 
		return maxPipelining_; 
	}

	//-- FetchDefaults() callee
        // Also called from LogCleanup task.
	static void FetchSingleDefault( CDSString &name,
				        TInt32 &to);

	//--------------------------------------------//
	// Mutators
	//--------------------------------------------//
public:
	// Populate the cache and aquire DDL locks
	void Build();

  // to search for public schema
  CDDSchema *GetPublicSchema();

	//--------------------------------------------//
	// Private data and function members
	//--------------------------------------------//
private:
	//-- Prevent copying
	CRUCache(const CRUCache&);
	CRUCache& operator = (const CRUCache&);

private:
	//-- Build() callees
	void InitBuild();

	void FetchInvolvedMVsMetadata();
	void FetchInvolvedUsedObjectsMetadata();

	void FetchNonInvolvedMVsMetadata();
	void FetchNonInvolvedUsedObjectsMetadata();

	void SetupRecomputeProperty();
	void FetchDefaults();
	void FetchForceData();
	void FixupMVTblInterfaces();

private:
	//-- FetchInvolvedMVsMetadata() callees

	//-- Fetch all the metadata if REFRESH is run on MVGROUP
	void FetchMVsInGroup(CDDSchema *pddSch, const CDSString& mvgName);
	
	//-- Fetch the metadata for all the MVs 
	//-- if REFRESH is run with CASCADE option
	void FetchCascadedMVs(CDDSchema *pddSch, const CDSString& mvName);
	void RecursiveFetchCascadedMVs(CDDMV *pddRootMV);

	//-- Least common denominator
	void FetchSingleInvolvedMV(CDDMV *pddMV);

	
	//-- FetchForceData() callee
	void UpdateForceOptionForMV(CRUForceOptions& forceOptions,
								CRUMV *pMV);
private:
	//-- FetchInvolvedUsedObjectsMetadata() callees
	void FetchUsedObjectsMetadataForSingleMV(CRUMV *pddMV);
	CRUTbl *FetchSingleUsedObject(CRUMV *pUsingMV, const CDDUIDTriple &uidt);

	void SetupUsedObject(CRUTbl *pTbl);
	void FixupLogTblReference(CRUTbl *pTbl);

private:
	//-- FetchNonInvolvedMVsMetadata() callee
	CRUMV *FetchSingleNonInvolvedMV(CDDMV *pddMV);

private:
	// Check whether there is at least one log to clean
	void CheckLogCleanupApplicability();
	BOOL IsOnRequestMVInvolved();

	// Various accessors to DD objects
private:
	CDDSchema *GetDDSchemaByName(const CDSString& catName, 
		 					     const CDSString& schName);
	CDDSchema *GetDDSchemaByUID(TInt64 catUid, TInt64 schUid);

	CDDMV *GetDDMVByUID(const CDDUIDTriple &uidt);
	CDDTable *GetDDTableByUID(const CDDUIDTriple &uidt);

	CDDMV *GetDDMVByName(CDDSchema *pddSch, const CDSString& mvName);
	
	CDDTable *GetDDIUDLogTableByName(
		CDDSchema *pddSch, 
		const CDSString& logName);

	CDDMVGroup *GetDDMVGroupByName(
		CDDSchema *pddSch, 
		const CDSString& mvgName);

private:
	//--------------------------------------------//
	//	Core data members
	//--------------------------------------------//
	
	// Command options
	CRUOptions::LogCleanupType lcType_;
	BOOL isCancelOnly_;
	BOOL isTotalRecompute_;

	// Will become obsolete when privileges are checked through file-open
	CDSString currentUser_;

	// The DDOL object hierarchy root.
	// The actual container of the DD objects 
	// (catalogs, schemas, tables, MVs).
	// Since this data member is an object (not a pointer),
	// the deallocation of DD objects will happen automatically.
	CDDSqlNode	 sqlNode_;

	//-- Lists of pointers to involved objects --//
	CRUMVList mvList_;
	CRUTblList tableList_;

	CRUCacheDDLLockHandler ddlLockHandler_;

	TInt32 maxPipelining_;
	TInt32 maxParallelism_;
};

#endif
