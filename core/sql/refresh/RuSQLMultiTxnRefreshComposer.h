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
#ifndef _RU_SQL_MULTI_TXN_REFRESH_COMPOSER_H_
#define _RU_SQL_MULTI_TXN_REFRESH_COMPOSER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuSQLMultiTxnRefreshComposer.h
* Description:  Definition of class CRUSQLRefreshComposer
*				
*
* Created:      08/17/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/
#include "RuSQLRefreshComposer.h"

class REFRESH_LIB_CLASS CRUSQLMultiTxnRefreshComposer : public CRUSQLRefreshComposer {

public:

	CRUSQLMultiTxnRefreshComposer(CRURefreshTask *pTask);
	virtual ~CRUSQLMultiTxnRefreshComposer() {}

public:

	void ComposeRefresh(Int32 phase, BOOL catchup);

private:

	void AddNRowsClause(Int32 phase, BOOL catchup);
};

#endif
