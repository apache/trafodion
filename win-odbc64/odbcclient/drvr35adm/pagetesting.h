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

#if !defined(AFX_PAGETESTING_H__0DC14BCC_5653_11D3_9E7E_00508B0B983B__INCLUDED_)
#define AFX_PAGETESTING_H__0DC14BCC_5653_11D3_9E7E_00508B0B983B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PageTesting.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PageTesting dialog

class PageTesting : public CPropertyPage
{
	DECLARE_DYNCREATE(PageTesting)

// Construction
public:
	PageTesting();
	~PageTesting();

// Dialog Data
	CPropertySheet* m_ppropsheet;
	BOOL m_bDSNUpdated;

	//{{AFX_DATA(PageTesting)
	enum { IDD = IDD_PAGE_TESTING };
	CStatic	m_static_testing;
	//}}AFX_DATA

	BOOL UpdateDSN( void );

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PageTesting)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(PageTesting)
	afx_msg void OnTestconn();
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGETESTING_H__0DC14BCC_5653_11D3_9E7E_00508B0B983B__INCLUDED_)
