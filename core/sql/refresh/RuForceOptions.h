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
#ifndef _RU_FORCE_OPTIONS_H_
#define _RU_FORCE_OPTIONS_H_

/* -*-C++-*-
******************************************************************************
*
* File:         CRUMV.h
* Description:  Definition of class CRUForceOptions
*
*
* Created:      01/24/2002
* Language:     C++
*
* 
******************************************************************************
*/ 
#include "refresh.h"
#include "dsptrlist.h"

class CDSString;
class CRUTableForceOptions;
class CRUTableForceOptionsList;
class CRUMVForceOptions;
class CRUMVForceOptionsList;
   

//----------------------------------------------------------------------------
// CRUMVForceOptionsList
//----------------------------------------------------------------------------

// used by Range logging
DECLARE_PTRLIST(REFRESH_LIB_CLASS, CRUMVForceOptions);
   
//----------------------------------------------------------------------------
// CRUForceOptions
// Store data about the force options for a group of MVs
//----------------------------------------------------------------------------


class REFRESH_LIB_CLASS CRUForceOptions {
public:
	
	CRUForceOptions() : mvsList_(eItemsArentOwned) {};
	~CRUForceOptions();
	enum GroupByOptions
	{
		GB_NO_FORCE,
		GB_HASH,
		GB_SORT
	};
	enum JoinOptions
	{
		JOIN_NO_FORCE,
		JOIN_NESTED,
		JOIN_MERGE,
		JOIN_HASH
	};
	enum MdamOptions 
	{
		MDAM_NO_FORCE,
		MDAM_ENABLE,
		MDAM_ON,
		MDAM_OFF
	};
	
	enum ExplainOptions
	{
		EXPLAIN_ON,
		EXPLAIN_OFF
	};

	enum CQSStmtOption
	{
		CQS_DISABLE,
		CQS_ENABLE
	};

	void AddMV(CRUMVForceOptions*);

	const CRUMVForceOptionsList& GetMVForceOptionsList() const
	{
		return mvsList_;
	}
	
	CRUMVForceOptionsList& GetMVForceOptionsList()
	{
		return mvsList_;
	}

	CDSString& GetForceFileName() 
	{
		return fileName_;
	}

	BOOL IsMVExist(const CDSString&) const;

private:
	//-- Prevent copying
	CRUForceOptions(const CRUForceOptions&);
	CRUForceOptions &operator = (const CRUForceOptions &other);

private:
	CRUMVForceOptionsList mvsList_;
	CDSString fileName_;
};

//----------------------------------------------------------------------------
// CRUTableForceOptions
// Stores a table's name and its MDAM force option
//----------------------------------------------------------------------------

class REFRESH_LIB_CLASS CRUTableForceOptions {
public:
	CRUTableForceOptions(const CDSString&);
	CRUTableForceOptions(const CRUTableForceOptions&);
	
	//--------------------------------------------//
	// Accessors
	//--------------------------------------------//
	const CDSString& GetFullName() const
	{
		return tableName_;
	}
	
	CRUForceOptions::MdamOptions GetMdamOptions() const
	{
		return mdam_;
	}

	//--------------------------------------------//
	// Mutators
	//--------------------------------------------//
	void SetTableName(const CDSString& );
	
	void SetMdam(CRUForceOptions::MdamOptions mdam)
	{
			mdam_=mdam;
	}

private:
	CDSString tableName_;
	CRUForceOptions::MdamOptions mdam_;
};

//----------------------------------------------------------------------------
// CRUTableForceOptionsList
//----------------------------------------------------------------------------

DECLARE_PTRLIST(REFRESH_LIB_CLASS, CRUTableForceOptions);

//----------------------------------------------------------------------------
// CRUMVForceOptions
// Stores the name and the force options for a single MV
//----------------------------------------------------------------------------

class REFRESH_LIB_CLASS CRUMVForceOptions {
public:
	CRUMVForceOptions();
	CRUMVForceOptions(const CDSString&);
	~CRUMVForceOptions();

	//--------------------------------------------//
	// Accessors
	//--------------------------------------------//
	const CDSString& GetMVName() const
	{
		return mvName_;
	}
	
	CRUForceOptions::MdamOptions GetMDAMoption() const
	{
		return mdam_;
	}
	
	CRUForceOptions::GroupByOptions GetGroupByoption() const
	{
		return groupBy_;
	}

	CRUForceOptions::JoinOptions GetJoinoption() const
	{
		return join_;
	}

	const CRUTableForceOptionsList& GetTableForceList() const
	{
		return *pTablesList_;
	}
	
	const CRUTableForceOptionsList& GetTableForceList()
	{
		return *pTablesList_;
	}

	const Int32 GetNumOfTables() const
	{
		return pTablesList_->GetCount();
	}

	BOOL IsTableStarUsed() const
	{
		return usedTableStarOption_ != CRUForceOptions::MDAM_NO_FORCE;
	}

	CRUForceOptions::MdamOptions GetTableStarOption() const
	{
		return usedTableStarOption_;
	}

	// gets a table's name and returns in force options value
	// returns MDAM_NO_FORCE if the table does not exist
	CRUForceOptions::MdamOptions 
		GetForceMdamOptionForTable(const CDSString&) const;


	BOOL IsMVNameExist(const CDSString&) const;


	CRUForceOptions::ExplainOptions	GetExplainOption() const
	{
		return explain_;
	}

	const CDSString &GetCQSStatment() const
	{
		return cqsStmt_;
	}
	
	//--------------------------------------------//
	// Mutators
	//--------------------------------------------//
	void SetMVName(CDSString mvName)
	{
		mvName_ = mvName;
	}

	void SetGroupBy(CRUForceOptions::GroupByOptions gb)
	{
		groupBy_ = gb;
	}

	void SetJoin(CRUForceOptions::JoinOptions join)
	{
		join_ = join;
	}

	void SetMdam(CRUForceOptions::MdamOptions mdam)
	{
		mdam_ = mdam;
	}

	void SetUsedTableStarOption(CRUForceOptions::MdamOptions usedTableStarOption)
	{
		usedTableStarOption_ = usedTableStarOption;
	}

	void AddTable(CRUTableForceOptions*);


	void SetExplainOption(CRUForceOptions::ExplainOptions explain)
	{
		explain_ = explain;
	}

	void SetCQSStatment(const CDSString &str)
	{
		cqsStmt_ = str;
	}

private:
	//-- Prevent copying
	CRUMVForceOptions &operator = (const CRUMVForceOptions &other);
	CRUMVForceOptions(const CRUMVForceOptions& srcMv);

//--------------------------------------------//
// Private data and function members
//--------------------------------------------//
private:
	CDSString mvName_;
	CRUForceOptions::GroupByOptions groupBy_; //the groupby force option of the mv
	CRUForceOptions::JoinOptions join_;	  //the join force option of the mv
	CRUForceOptions::MdamOptions mdam_;	  //the mdam force option of the mv
	CRUForceOptions::MdamOptions usedTableStarOption_; 
	CRUTableForceOptionsList* pTablesList_;	  //a list of all the tables under the mv
	CRUForceOptions::ExplainOptions explain_;
	CDSString cqsStmt_;

};

// Complete the destructor's definition
inline CRUMVForceOptionsList::~CRUMVForceOptionsList() {}
inline CRUTableForceOptionsList::~CRUTableForceOptionsList() {}


#endif
