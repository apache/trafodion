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
#ifndef _RU_DELTA_DEF_H_
#define _RU_DELTA_DEF_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDeltaDef.h
* Description:  Definition of classes 
*				CRUUpdateBitmap, CRUDeltaDef and CRUDeltaStatistics
*				
* Created:      04/04/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "dsptrlist.h"
#include "dsmap.h"
#include "dsstring.h"

class CRUTbl;
class CRUUpdateBitmap;
struct CRUDeltaStatistics;

class CUOFsIpcMessageTranslator;

//--------------------------------------------------------------------------//
//	CRUDeltaDef
//	
//	 This class contains all the data about a single T-delta that is observed 
//	 by a specific MV. This data is collected by the Emptiness Check and
//	 Duplicate Elimination tasks.
//
//	 This data will be converted into the syntax of the DELTA-DEF clause 
//	 of the INTERNAL REFRESH command when the utility will generate one.
//
//--------------------------------------------------------------------------//

struct REFRESH_LIB_CLASS CRUDeltaDef {

public:
	CRUDeltaDef(CRUTbl *pTbl);
	~CRUDeltaDef();

public:
	TInt64 tblUid_;	  // Table UID
	CDSString tblName_;

	TInt32 fromEpoch_; // Apply T-log from epoch 
	TInt32 toEpoch_;   // ... to epoch 

	// Emptiness Check results
	BOOL isRangeLogNonEmpty_;
	BOOL isIUDLogNonEmpty_;
	
	BOOL isIUDLogInsertOnly_;	// INSERTLOG attribute

	// The Duplicate Elimination level.
	// Every following level includes all the semantics 
	// of the previoius ones.
	enum DELevel {

		NO_DE			= 0,
		RANGE_2_RANGE	= 1,
		RANGE_2_SINGLE	= 2,
		SINGLE_2_SINGLE	= 3
	};
	Int32 deLevel_;

	// Statistics collected by the DE task
	CRUDeltaStatistics *pStat_;

private:
	//-- Prevent copying
	CRUDeltaDef(const CRUDeltaDef &other);
	CRUDeltaDef &operator = (const CRUDeltaDef &other);
};

//--------------------------------------------------------------------------//
//	CRUDeltaDefList
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDeltaDefList : public CDSPtrList<CRUDeltaDef>
{
private:
	typedef CDSPtrList<CRUDeltaDef> inherited;

public:
	CRUDeltaDefList(EOwnership ownership = eItemsAreOwned)
		: inherited(ownership) {} 
	~CRUDeltaDefList() {}

public:
	CRUDeltaDef *FindByUID(TInt64 tblUid) const;
	void RemoveByUID(TInt64 tblUid);
};

//--------------------------------------------------------------------------//
//	CRUDeltaStatistics
//	
//	Log statistics collected by the Duplicate Elimination task.
//	Used for enhancing the INTERNAL REFRESH performance.
//
//--------------------------------------------------------------------------//

struct REFRESH_LIB_CLASS CRUDeltaStatistics {

public:
	enum { RANGE_SIZE_UNKNOWN = -1 };
	enum { MAX_STATISTIC = (1L<<30) };

	//-- Range log statistics
	TInt32	nRanges_;			// How many rows are in the range log?
	TInt32	nRangeCoveredRows_;	// How many rows are covered by ranges?

	// These numbers are applicable only if the DE made a full scan
	// (including the single-row records of the log).
	TInt32	nInsertedRows_;	
	TInt32	nDeletedRows_;	
	TInt32	nUpdatedRows_;
	CRUUpdateBitmap *pUpdateBitmap_;

public: 
	CRUDeltaStatistics();
	CRUDeltaStatistics(const CRUDeltaStatistics &other);

	~CRUDeltaStatistics();

	CRUDeltaStatistics& operator = (const CRUDeltaStatistics &other);

	TInt32 GetDeltaSize();	// Size estimate
	
	// Increment a counter (protect against overflow)
	void IncrementCounter(TInt32 &counterRef, Int32 delta=1)
	{
		TInt32 val = counterRef+delta;
		if (val < MAX_STATISTIC)
		{ 
			counterRef = val; 
		}
	}

public:
	//-- Pack/unpack for IPC
	void LoadData(CUOFsIpcMessageTranslator &translator);
	void StoreData(CUOFsIpcMessageTranslator &translator);

	static TInt32 GetPackedBufferSize(Int32 updateBitmapSize); 
};

//--------------------------------------------------------------------------//
//	CRUDeltaStatisticsMap
//
//	A hash table indexed by epoch value(s). Every element 
//	is an instance of CRUDeltaStatistics.
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDeltaStatisticsMap :	
	public CDSLongMap<CRUDeltaStatistics>
{
public:
	CRUDeltaStatisticsMap &operator = (const CRUDeltaStatisticsMap& other);

public:
	//-- Pack/unpack for IPC
	void LoadData(CUOFsIpcMessageTranslator &translator);
	void StoreData(CUOFsIpcMessageTranslator &translator);
};

//--------------------------------------------------------------------------//
//	CRUUpdateBitmap
//
//	The class for managing the values of the @UPDATE_BITMAP column 
//	in the IUD log. It can serialize and de-serialize itself.
//
//	The data buffer contains one byte more than the data, because the
//	DMOL representation of CHAR is null-terminated.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUUpdateBitmap {

public:
	CRUUpdateBitmap(Int32 size, const char* buffer = NULL);
	CRUUpdateBitmap(const CRUUpdateBitmap &other);

	virtual ~CRUUpdateBitmap();

public:
	char *GetBuffer() const
	{
		return buffer_;
	}

	Int32 GetSize() const 
	{
		return size_;
	}

	// Is the whole bitmap zero-only?
	BOOL IsNull() const;

	// Did some |= operator applied to the bitmap change its content?
	BOOL WasChanged() const
	{
		return wasChanged_;
	}

	BOOL IsColumnUpdated(Int32 i) const
	{
		Int32 index  = i / 8;
		Int32 bitNum = i % 8;
		
		return (0 != (buffer_[index] & (1L << bitNum)));
	}

public:
	// Set the whole bitmap to zeroes
	void Reset();

	CRUUpdateBitmap& operator  = (const CRUUpdateBitmap &other);
	CRUUpdateBitmap& operator |= (const CRUUpdateBitmap &other);

public:
	//-- Pack/unpack for IPC
	static CRUUpdateBitmap *CreateInstance(
		CUOFsIpcMessageTranslator &translator
	);

	void StoreData(CUOFsIpcMessageTranslator &translator);

private:
	Int32 size_;
	char *buffer_;

	BOOL wasChanged_;
};

#endif
