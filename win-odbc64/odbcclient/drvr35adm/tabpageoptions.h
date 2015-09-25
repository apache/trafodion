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

#if !defined(AFX_TABPAGEOPTIONS_H__CEE85AE1_8015_11D3_9E99_00508B0B983B__INCLUDED_)
#define AFX_TABPAGEOPTIONS_H__CEE85AE1_8015_11D3_9E99_00508B0B983B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TabPageOptions.h : header file
//
#include "propsheet.h"

/////////////////////////////////////////////////////////////////////////////
// TabPageOptions dialog

class TabPageOptions : public CPropertyPage
{
	DECLARE_DYNCREATE(TabPageOptions)

// Construction
public:
	TabPageOptions();
	~TabPageOptions();

// Dialog Data
	CPropSheet* m_ppropsheet;

	//{{AFX_DATA(TabPageOptions)
	enum { IDD = IDD_PAGE_OPTIONS1 };
	CComboBox	m_sdsnames;
	CStatic	m_static_options;
	CEdit	m_schema;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(TabPageOptions)
	public:
	virtual BOOL OnSetActive();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(TabPageOptions)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeSchema();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABPAGEOPTIONS_H__CEE85AE1_8015_11D3_9E99_00508B0B983B__INCLUDED_)
