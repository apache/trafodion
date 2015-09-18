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

// ODBCSvcDlg.cpp : implementation file
// NonStop ODBC/MX Service Dialog Window - IP Address and Port

#include "stdafx.h"
#include "drvr35adm.h"
#include "ODBCSvcDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CODBCSvcDlg dialog


CODBCSvcDlg::CODBCSvcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CODBCSvcDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CODBCSvcDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CODBCSvcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CODBCSvcDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CODBCSvcDlg, CDialog)
	//{{AFX_MSG_MAP(CODBCSvcDlg)
	ON_EN_CHANGE(IDC_SRVCN, OnChangeSrvcn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODBCSvcDlg message handlers

void CODBCSvcDlg::OnChangeSrvcn() 
{

	CString szItem;
	CButton *pBtn = (CButton *)GetDlgItem(IDOK);
	CEdit *pEdit = (CEdit *)GetDlgItem(IDC_SRVCN);
	if (pEdit)
		pEdit->GetWindowText(szItem);

	if (pBtn)
		pBtn->EnableWindow(	strlen(szItem) > 0);	

	strcpy(aAttr[ KEY_IPADDRESS].szAttr, szItem);
	
}

void CODBCSvcDlg::OnOK() 
{
	CString szItem;
	CEdit *pEdit = (CEdit *)GetDlgItem(IDC_SRVCP);
	if (pEdit)
		pEdit->GetWindowText(szItem);

	strcpy(aAttr[ KEY_PORTNUM].szAttr, szItem);
	
	CDialog::OnOK();
}
