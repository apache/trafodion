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

#if !defined(AFX_TABPAGELOCALTR_H__A7DA58F3_801D_11D3_9E99_00508B0B983B__INCLUDED_)
#define AFX_TABPAGELOCALTR_H__A7DA58F3_801D_11D3_9E99_00508B0B983B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TabPageLocalTr.h : header file
//
#include "propsheet.h"

/////////////////////////////////////////////////////////////////////////////
// TabPageLocalTr dialog

class TabPageLocalTr : public CPropertyPage
{
	DECLARE_DYNCREATE(TabPageLocalTr)

// Construction
public:
	TabPageLocalTr();
	~TabPageLocalTr();

	void TabEnumLanguages( void );
// Dialog Data
	CPropSheet* m_ppropsheet;

	//{{AFX_DATA(TabPageLocalTr)
	enum { IDD = IDD_PAGE_LOCAL_TR1 };
	CEdit	m_translationoption;
	CStatic	m_static_tr;
	CEdit	m_dllname;
	CComboBox	m_errorlang;
	CComboBox	m_datalang;
	CEdit	m_replacementchar;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(TabPageLocalTr)
	public:
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
	//{{AFX_MSG(TabPageLocalTr)
	afx_msg void OnDllBrowse();
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeComboDatalang();
	afx_msg void OnSelchangeComboDatalang();
	afx_msg void OnEditchangeComboErrorlang();
	afx_msg void OnSelchangeComboErrorlang();
	afx_msg void OnChangeDllName();
	afx_msg void OnChangeTranslateOption();
	afx_msg void OnChangeReplacementChar();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABPAGELOCALTR_H__A7DA58F3_801D_11D3_9E99_00508B0B983B__INCLUDED_)
