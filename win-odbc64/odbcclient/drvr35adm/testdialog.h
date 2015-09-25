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

#if !defined(AFX_TESTDIALOG_H__AFC66638_614E_11D3_9E8A_00508B0B983B__INCLUDED_)
#define AFX_TESTDIALOG_H__AFC66638_614E_11D3_9E8A_00508B0B983B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TestDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTestDialog dialog

class CTestDialog : public CDialog
{
// Construction
public:
	CTestDialog(CWnd* pParent = NULL);   // standard constructor
	void ListBoxAddString(char* buf);
// Dialog Data
	//{{AFX_DATA(CTestDialog)
	enum { IDD = IDD_TESTSERVICE };
	CButton	m_ok;
	CListBox	m_output;
	CButton	m_testconn;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTestDialog)
	afx_msg void OnTestconn();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTDIALOG_H__AFC66638_614E_11D3_9E8A_00508B0B983B__INCLUDED_)
