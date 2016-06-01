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
********************************************************************/
// OdbcDS.h: interface for the COdbcDS class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBCDS_H__139BA19D_D8DE_4FE3_B024_85FF2A8FED7F__INCLUDED_)
#define AFX_ODBCDS_H__139BA19D_D8DE_4FE3_B024_85FF2A8FED7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DSList.h"

class COdbcDS  
{
public:
	COdbcDS();
	virtual ~COdbcDS();

	char* Convert(const char* oldDriverName, const char* newDriverName);
	bool  IsDrvrInstalled(const char* pDriverName, char* Message);
	bool  FindDSNByVersion(const char* pDriverName, char* Message);

private:
	const static int DS_TYPE_SYS;
	const static int DS_TYPE_USER;

	SQLHANDLE m_henv;

	void Connect();
	bool GetMxDSList(const char* pDriverName, int iType, CDSList& DSList);
	bool FindInstalledDrvr(const char* pDrvrName);
	void RetrieveMxDSInfo(int iType, CDSList& DSList);
	void Disconnect();
	void UpdateDS(const char* pNewDriverName, int iType, CDSList& DSList);
	char* GetLastError();
	char* GetLastInstallerError();
};

#endif // !defined(AFX_ODBCDS_H__139BA19D_D8DE_4FE3_B024_85FF2A8FED7F__INCLUDED_)
