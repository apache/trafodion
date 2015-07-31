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
#ifndef _RU_DUPELIM_CONST_H_
#define _RU_DUPELIM_CONST_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimConst.h
* Description:  Definition of class CRUDupElimConst
*
* Created:      06/12/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

//--------------------------------------------------------------------------//
//	CRUDupElimConst
//	
//	This class is rather a namespace. 
//
//	It contains the definitions of the constants common to
//	the classes that implement the duplicate elimination 
//	algorithm.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDupElimConst {

public:
	// @OPERATION_TYPE bitmaps
	enum
	{
		IS_DELETE			= 0x1,
		IS_PART_OF_UPDATE	= 0x2,
		IS_RANGE_RECORD		= 0x4,
		IS_BEGIN_RANGE		= 0x8
	};

public:
	// Types of the queries to compute the delta
	enum {

		// Query for the first txn (no lower bound)
		PHASE_0_QUERY	= 0,
		// Query for the following txns (with lower bound)
		PHASE_1_QUERY	= 1,

		NUM_QUERY_STMTS
	};

	// The maximum number of epochs for optimizable query
	enum { MAX_SELECT_CLAUSES = 10 };

public:
	// Types of the Insert/Update/Delete statements
	// to be applied to the range log and the IUD log
	// by the range resolver
	enum {

		// Insert a new record to the range-log
		RANGE_LOG_INSERT			= 0,	
		// Delete a subset of records from the range-log
		RANGE_LOG_DELETE			= 1,	
		// Delete a subset of records from the IUD-log
		IUD_LOG_SUBSET_DELETE		= 2, 
		// Update the @IGNORE column 
		// in a subset of records in the IUD-log
		IUD_LOG_SUBSET_UPDATE_IGNORE = 3, 
		// Update the @IGNORE column to the max-int number
		// These rows will always be ignored
		IUD_LOG_SUBSET_UPDATE_ALWAYS_IGNORE = 4,
		NUM_RNG_RESOLV_STMTS
	};

	// Control statements that force/reset the MDAM
	// optimization in the access to the IUD log
	// CQS stands for "control query shape", CT for "control table"
	enum {

		IUD_LOG_FORCE_MDAM_CQS	= 0,
		RNG_LOG_FORCE_MDAM_CQS	= 1,
		RESET_MDAM_CQS	= 2,
		
		FORCE_MDAM_CT	= 3,
		RESET_MDAM_CT	= 4,

		NUM_CONTROL_STMTS
	};

public:
	// Types of the Insert/Update/Delete statements
	// to be applied to the IUD log by the single-row resolver
	enum {
		// Delete a single record from the IUD-log
		IUD_LOG_SINGLE_DELETE			= 0, 
		// Update the @IGNORE column of a single record
		IUD_LOG_SINGLE_UPDATE_IGNORE	= 1, 
		// Update the @UPDATE_BITMAP column of all the 
		// "update" records in the chain
		IUD_LOG_UPDATE_BITMAP			= 2, 
		// Reset the Update bit in the @OPERATION_TYPE column 
		// of all the "update" records in the chain
		IUD_LOG_UPDATE_OPTYPE		= 3, 

		NUM_SINGLE_RESOLV_STMTS
	};

public:
	// Order between the control (@) columns in the query select list
	enum { 

		OFS_EPOCH	= 0,
		OFS_OPTYPE	= 1,
		OFS_RNGSIZE	= 2,
		// Optional columns, required only for DE level = 3
		OFS_IGNORE	= 3,
		OFS_UPD_BMP	= 4,

		NUM_IUD_LOG_CONTROL_COLS_BASIC  = 3,	// DE level=1/2
		NUM_IUD_LOG_CONTROL_COLS_EXTEND = 5		// DE level=3
	};

	// The number of control (@...) columns in the range log
	enum { NUM_RNG_LOG_CONTROL_COLS = 3 };
};

#endif
