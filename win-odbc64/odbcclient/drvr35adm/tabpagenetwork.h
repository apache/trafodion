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

#if !defined(AFX_TABPAGENETWORK_H__AAF68730_8013_11D3_9E99_00508B0B983B__INCLUDED_)
#define AFX_TABPAGENETWORK_H__AAF68730_8013_11D3_9E99_00508B0B983B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TabPageNetwork.h : header file
//
#include "propsheet.h"

/////////////////////////////////////////////////////////////////////////////
// TabPageNetwork dialog

class TabPageNetwork : public CPropertyPage
{
	DECLARE_DYNCREATE(TabPageNetwork)

// Construction
public:
	TabPageNetwork();
	~TabPageNetwork();

// Dialog Data
	CPropSheet* m_ppropsheet;

	//{{AFX_DATA(TabPageNetwork)
	enum { IDD = IDD_PAGE_NETWORK1 };
	CStatic	m_static_network;
	CComboBox	m_query;
	CEdit	m_portnum;
	CComboBox	m_login;
	CEdit	m_ipaddress;
	CComboBox	m_fetch_buffer_size;
	CComboBox	m_connection;
	CComboBox	m_Compression;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(TabPageNetwork)
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
	//{{AFX_MSG(TabPageNetwork)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeConnection();
	afx_msg void OnEditchangeFetchBufferSize();
	afx_msg void OnChangeIpaddress();
	afx_msg void OnEditchangeLogin();
	afx_msg void OnChangePortnum();
	afx_msg void OnEditchangeQuery();
	afx_msg void OnSelchangeLogin();
	afx_msg void OnSelchangeQuery();
	afx_msg void OnSelchangeConnection();
	afx_msg void OnSelchangeFetchBufferSize();
	afx_msg void OnSelchangeCompression();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABPAGENETWORK_H__AAF68730_8013_11D3_9E99_00508B0B983B__INCLUDED_)
