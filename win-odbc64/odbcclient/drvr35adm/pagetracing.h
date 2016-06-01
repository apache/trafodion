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

#if !defined(AFX_PAGETRACING_H__0DC14BCB_5653_11D3_9E7E_00508B0B983B__INCLUDED_)
#define AFX_PAGETRACING_H__0DC14BCB_5653_11D3_9E7E_00508B0B983B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PageTracing.h : header file
//

#include "propsheet.h"

#define TR_ON					0x00000001
#define TR_ODBC_API				0x00000002
#define TR_DRVR_TRANSPORT_API	0x00000004
/*
#define TR_ON						0x00000001
#define TR_ODBC_API					0x00000002
#define TR_ODBC_API_EXIT			0x00000004
#define TR_DRVR_KRYPTON_API			0x00000008
#define TR_SRVR_KRYPTON_API			0x00000010
#define TR_SQL_API					0x00000020
#define TR_POST_ERROR				0x00000040
#define TR_DM_API					0x10000000
#define TR_DM_API_EXIT				0x20000000
*/
/////////////////////////////////////////////////////////////////////////////
// PageTracing dialog

class PageTracing : public CPropertyPage
{
	DECLARE_DYNCREATE(PageTracing)

// Construction
public:
	PageTracing();
	~PageTracing();

// Dialog Data
	CPropSheet* m_ppropsheet;

	BOOL UpdateDSN( void );

	//{{AFX_DATA(PageTracing)
	enum { IDD = IDD_PAGE_TRACING };
	CButton	m_dm_api_exit;
	CButton	m_dm_api;
	CStatic	m_static_tracing;
	CButton	m_post_error;
	CButton	m_odbc_api_exit;
	CButton	m_drvr_krypton;
	CButton	m_odbc_api;
	CEdit	m_file_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PageTracing)
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
	//{{AFX_MSG(PageTracing)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGETRACING_H__0DC14BCB_5653_11D3_9E7E_00508B0B983B__INCLUDED_)
