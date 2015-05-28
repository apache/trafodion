// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2002-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#if !defined(AFX_PAGELOCALTR_H__55EE6C7A_6218_11D3_9E8A_00508B0B983B__INCLUDED_)
#define AFX_PAGELOCALTR_H__55EE6C7A_6218_11D3_9E8A_00508B0B983B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PageLocalTr.h : header file
//
#include "propsheet.h"

/////////////////////////////////////////////////////////////////////////////
// PageLocalTr dialog

class PageLocalTr : public CPropertyPage
{
	DECLARE_DYNCREATE(PageLocalTr)

// Construction
public:
	PageLocalTr();
	~PageLocalTr();

	void EnumLanguages( void );
// Dialog Data
	CPropSheet* m_ppropsheet;

	//{{AFX_DATA(PageLocalTr)
	enum { IDD = IDD_PAGE_LOCAL_TR };
	CStatic	m_static_tr;
	CEdit	m_translationoption;
	CEdit	m_dllname;
	CComboBox	m_errorlang;
	CComboBox	m_datalang;
	CEdit	m_replacementchar;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PageLocalTr)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(PageLocalTr)
	virtual BOOL OnInitDialog();
	afx_msg void OnDllBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGELOCALTR_H__55EE6C7A_6218_11D3_9E8A_00508B0B983B__INCLUDED_)
