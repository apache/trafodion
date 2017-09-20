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
#ifndef _RU_RC_RELEASE_TASKEX_H_
#define _RU_RC_RELEASE_TASKEX_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuRcReleaseTaskExecutor.cpp
* Description:  Implementation of class	CRURcReleaseTaskExecutor
*
* Created:      10/18/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"
#include "RuTaskExecutor.h"

class REFRESH_LIB_CLASS CRURcReleaseTaskExecutor : public CRUTaskExecutor {

private:
	typedef CRUTaskExecutor inherited;

public:
	CRURcReleaseTaskExecutor(CRUTask *pParentTask);
	~CRURcReleaseTaskExecutor() {}

public:
	//-- Implementation of pure virtual functions
	virtual void Work();

	virtual void Init();

public:
	// These functions serialize/de-serialize the executor's context 
	// for the message communication with the remote server process

	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator) {};
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator) {};
	
	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator) {};
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator) {};

protected:
	//-- Implementation of pure virtual

	virtual Lng32 GetIpcBufferSize() const
	{
		return 0;	// The task is always performed locally
	}

private:
	//-- Prevent copying
	CRURcReleaseTaskExecutor(const CRURcReleaseTaskExecutor &other);
	CRURcReleaseTaskExecutor &operator = (const CRURcReleaseTaskExecutor &other);
};

#endif
