/*************************************************************************
*
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
**************************************************************************/

#if !defined(AFX_ODBCSVCDLG_H__61E64254_7E53_11D2_A539_0060B01A6EC3__INCLUDED_)
#define AFX_ODBCSVCDLG_H__61E64254_7E53_11D2_A539_0060B01A6EC3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ODBCSvcDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CODBCSvcDlg dialog

class CODBCSvcDlg : public CDialog
{
// Construction
public:
	CODBCSvcDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CODBCSvcDlg)
	enum { IDD = IDD_SERVICENAME };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CODBCSvcDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CODBCSvcDlg)
	afx_msg void OnChangeSrvcn();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	LPAttr  aAttr;				// Attribute array pointer
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ODBCSVCDLG_H__61E64254_7E53_11D2_A539_0060B01A6EC3__INCLUDED_)
