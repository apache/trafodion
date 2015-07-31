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
#ifndef _RU_SQL_SIMPLE_REFRESH_COMPOSER_H_
#define _RU_SQL_SIMPLE_REFRESH_COMPOSER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuSQLSimpleRefreshComposer.h
* Description:  Definition of class CRUSQLRefreshComposer
*				
*
* Created:      08/13/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/


#include "RuRefreshSQLComposer.h"

//--------------------------------------------------------------------------//
//
// CRUSimpleRefreshSQLComposer
//
// This class composes the INTERNAL REFRESH command (both incremenetal and 
// recompute ) that is used with an audited  single transaction MV
// and unaudited MV. The class also composes the LOCK/UNLOCK TABLE statements 
// that are used in the unaudited executor.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUSimpleRefreshSQLComposer : 
	  public CRURefreshSQLComposer {

public:
	CRUSimpleRefreshSQLComposer(CRURefreshTask *pTask);
	virtual ~CRUSimpleRefreshSQLComposer() {}

public:
	void ComposeRefresh();

	enum NoDeleteClause { REC_DELETE = FALSE , REC_NODELETE = TRUE };
	void ComposeRecompute(NoDeleteClause flag);
	
	void ComposeLock(const CDSString& name, BOOL exclusive);
	void ComposeUnLock(const CDSString& name);

private:
	//-- Prevent copying
	CRUSimpleRefreshSQLComposer(const CRUSimpleRefreshSQLComposer &other);
	CRUSimpleRefreshSQLComposer &operator = 
						(const CRUSimpleRefreshSQLComposer &other);

	void AddDeltaDefListClause();
};

#endif
