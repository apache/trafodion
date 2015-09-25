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

// TabPageTracing.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "TabPageTracing.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TabPageTracing property page

IMPLEMENT_DYNCREATE(TabPageTracing, CPropertyPage)

TabPageTracing::TabPageTracing() : CPropertyPage(TabPageTracing::IDD)
{
	//{{AFX_DATA_INIT(TabPageTracing)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

TabPageTracing::~TabPageTracing()
{
}

void TabPageTracing::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TabPageTracing)
	DDX_Control(pDX, IDC_STATIC_TRACING, m_static_tracing);
	DDX_Control(pDX, IDC_POST_ERROR, m_post_error);
	DDX_Control(pDX, IDC_ODBC_API_EXIT, m_odbc_api_exit);
	DDX_Control(pDX, IDC_ODBC_API, m_odbc_api);
	DDX_Control(pDX, IDC_FILE_NAME, m_file_name);
	DDX_Control(pDX, IDC_DRVR_KRYPTON, m_drvr_krypton);
	DDX_Control(pDX, IDC_DM_API_EXIT, m_dm_api_exit);
	DDX_Control(pDX, IDC_DM_API, m_dm_api);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TabPageTracing, CPropertyPage)
	//{{AFX_MSG_MAP(TabPageTracing)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_DM_API, OnDmApi)
	ON_BN_CLICKED(IDC_DM_API_EXIT, OnDmApiExit)
	ON_BN_CLICKED(IDC_DRVR_KRYPTON, OnDrvrKrypton)
	ON_EN_CHANGE(IDC_FILE_NAME, OnChangeFileName)
	ON_BN_CLICKED(IDC_ODBC_API, OnOdbcApi)
	ON_BN_CLICKED(IDC_ODBC_API_EXIT, OnOdbcApiExit)
	ON_BN_CLICKED(IDC_POST_ERROR, OnPostError)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TabPageTracing message handlers

BOOL TabPageTracing::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	long diagnostic_flags=0;
	
	m_file_name.LimitText( _MAX_PATH );
	m_file_name.SetWindowText(aAttr[ KEY_TRACE_FILE].szAttr);
	m_file_name.SetFocus();

	diagnostic_flags = atol(aAttr[ KEY_TRACE_FLAGS].szAttr);
//	diagnostic_flags & TR_DM_API ? m_dm_api.SetCheck(1) :m_dm_api.SetCheck(0); 
//	diagnostic_flags & TR_DM_API_EXIT ? m_dm_api_exit.SetCheck(1) :m_dm_api_exit.SetCheck(0); 
	diagnostic_flags & TR_ODBC_API ? m_odbc_api.SetCheck(1) :m_odbc_api.SetCheck(0); 
//	diagnostic_flags & TR_ODBC_API_EXIT ? m_odbc_api_exit.SetCheck(1) :m_odbc_api_exit.SetCheck(0); 
	diagnostic_flags & TR_DRVR_TRANSPORT_API ? m_drvr_krypton.SetCheck(1) :m_drvr_krypton.SetCheck(0); 
//	diagnostic_flags & TR_POST_ERROR ? m_post_error.SetCheck(1) :m_post_error.SetCheck(0);
	char szStaticText[]="The current configuration is displayed.\n\
Trace Options specify what is traced when a trace is next started.\n\n\
Log File path specifies the name of the file in which all TRAF ODBC trace data is stored.";
	m_static_tracing.SetWindowText(szStaticText);
	
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL TabPageTracing::OnSetActive() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnSetActive();
}

void TabPageTracing::OnOK() 
{
	CPropertyPage::OnOK();
}

void TabPageTracing::OnCancel() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CPropertyPage::OnCancel();
}

BOOL TabPageTracing::OnApply() 
{
	if(	m_ppropsheet->bUpdateDSN==true)
	{
		m_ppropsheet->UpdateDSN();
		m_ppropsheet->bUpdateDSN=false;
	}
	
	return CPropertyPage::OnApply();
}

void TabPageTracing::OnBrowse() 
{
	char BASED_CODE szFilter[] = "Log Files (*.log)|*.log|All Files (*.*)|*.*||";
	CFileDialog Dlg (FALSE, "*.log", NULL, OFN_HIDEREADONLY | OFN_CREATEPROMPT,szFilter);
	Dlg.m_ofn.lpstrTitle="Select TRAF ODBC Trace File";

	if (Dlg.DoModal () == IDOK)
	{
		CString szPathName=Dlg.GetPathName();
		m_file_name.SetWindowText(szPathName);
		m_file_name.SetFocus();

	}
}

BOOL TabPageTracing::OnKillActive() 
{
	long diagnostic_flags=0;
	CString cTmp;

//	if(m_dm_api.GetCheck()) diagnostic_flags |= TR_DM_API; 
//	if(m_dm_api_exit.GetCheck()) diagnostic_flags |= TR_DM_API_EXIT; 
	if(m_odbc_api.GetCheck()) diagnostic_flags |= TR_ODBC_API; 
//	if(m_odbc_api_exit.GetCheck()) diagnostic_flags |= TR_ODBC_API_EXIT; 
	if(m_drvr_krypton.GetCheck()) diagnostic_flags |= TR_DRVR_TRANSPORT_API; 
//	if(m_post_error.GetCheck()) diagnostic_flags |= TR_POST_ERROR;

	m_file_name.GetWindowText(cTmp);
	cTmp.TrimLeft();
	cTmp.TrimRight();

	strncpy(aAttr[ KEY_TRACE_FILE].szAttr,(LPCTSTR)cTmp, sizeof(aAttr[ KEY_TRACE_FILE].szAttr));
	sprintf(aAttr[ KEY_TRACE_FLAGS].szAttr,"%ld",diagnostic_flags);

	return CPropertyPage::OnKillActive();
}

void TabPageTracing::OnDmApi() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageTracing::OnDmApiExit() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageTracing::OnDrvrKrypton() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageTracing::OnChangeFileName() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageTracing::OnOdbcApi() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageTracing::OnOdbcApiExit() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageTracing::OnPostError() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}
