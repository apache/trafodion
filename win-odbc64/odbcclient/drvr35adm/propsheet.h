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
#if !defined(AFX_PROPSHEET_H__F59DB842_5719_11D3_9E7E_00508B0B983B__INCLUDED_)
#define AFX_PROPSHEET_H__F59DB842_5719_11D3_9E7E_00508B0B983B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PropSheet.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CPropSheet

class CPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPropSheet)

// Construction
public:
	CPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:
	bool bUpdateDSN;
	bool UpdateDSN( void );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropSheet)
	public:
	virtual BOOL OnInitDialog();
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPropSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPropSheet)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	static const int m_NewDSNIndexOffset;
	BOOL DisplayHtmlHelp();
	CString GetHelpFilePath();

	typedef struct tagHH_LAST_ERROR
	{
		int      cbStruct;
		HRESULT  hr;
		BSTR     description;
	} HH_LAST_ERROR ;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPSHEET_H__F59DB842_5719_11D3_9E7E_00508B0B983B__INCLUDED_)
