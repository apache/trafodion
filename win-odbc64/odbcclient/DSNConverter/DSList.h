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
// List.h: interface for the List class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DSLIST_H__3C442044_651E_490B_88E7_004928060078__INCLUDED_)
#define AFX_DSLIST_H__3C442044_651E_490B_88E7_004928060078__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDSList  
{
public:
	CDSList();
	virtual ~CDSList();

	const static int NAME;
	const static int VALUE;

	// List related functions.
	bool  append(const char* name, const char* value = NULL);
	int   findName(const char* name, bool Search = true);
	char* getAt(int index, int type = CDSList::NAME);
	int   getCount();

	// Attribute related functions.
	bool  addAttrib(int index, const char* attrib,
				const char* attrib_value = NULL);
	bool  addAttrib(const char* name, const char* attrib,
				const char* attrib_value = NULL);
	int   getAttribCount(int index);
	int   getAttribCount(const char* name);
	char* getAttribValue(int index, const char* name);

private:
	// Private constructor for list element construction.
	CDSList(CDSList* pParent);

	// List related members.
	CDSList* m_pNext;
	CDSList* m_pPrev;
	const int m_iIndex;

	// Information about a list element.
	char* m_pName;
	char* m_pValue;
	CDSList* m_pAttribs;

	int getIndex();
};

#endif // !defined(AFX_DSLIST_H__3C442044_651E_490B_88E7_004928060078__INCLUDED_)