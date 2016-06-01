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

// PageTracing.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "PageTracing.h"
#include "TestDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int DropTraceTable( const char* szTraceTable);
/////////////////////////////////////////////////////////////////////////////
// PageTracing property page

IMPLEMENT_DYNCREATE(PageTracing, CPropertyPage)

PageTracing::PageTracing() : CPropertyPage(PageTracing::IDD)
{
	//{{AFX_DATA_INIT(PageTracing)
	//}}AFX_DATA_INIT
}

PageTracing::~PageTracing()
{
}

void PageTracing::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PageTracing)
	DDX_Control(pDX, IDC_DM_API_EXIT, m_dm_api_exit);
	DDX_Control(pDX, IDC_DM_API, m_dm_api);
	DDX_Control(pDX, IDC_STATIC_TRACING, m_static_tracing);
	DDX_Control(pDX, IDC_POST_ERROR, m_post_error);
	DDX_Control(pDX, IDC_ODBC_API_EXIT, m_odbc_api_exit);
	DDX_Control(pDX, IDC_DRVR_KRYPTON, m_drvr_krypton);
	DDX_Control(pDX, IDC_ODBC_API, m_odbc_api);
	DDX_Control(pDX, IDC_FILE_NAME, m_file_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PageTracing, CPropertyPage)
	//{{AFX_MSG_MAP(PageTracing)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PageTracing message handlers

BOOL PageTracing::OnInitDialog() 
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
	char szStaticText[]="Trace Options specify what is traced when a trace is next started.\n\n\
Log File Path specifies the name of the file in which all TRAF ODBC trace data is stored.\n\n\
Click Finish to create the data source.";

	m_static_tracing.SetWindowText(szStaticText);
	
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PageTracing::OnBrowse() 
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

BOOL PageTracing::OnSetActive() 
{
	m_ppropsheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);	
	return CPropertyPage::OnSetActive();
}

LRESULT PageTracing::OnWizardBack() 
{
	return CPropertyPage::OnWizardBack();
}

BOOL PageTracing::OnWizardFinish() 
{
	CTestDialog dlgTest;
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

	if(IDCANCEL==dlgTest.DoModal()) return FALSE;
	if(FALSE==m_ppropsheet->UpdateDSN()) return FALSE;

	return CPropertyPage::OnWizardFinish();
}

