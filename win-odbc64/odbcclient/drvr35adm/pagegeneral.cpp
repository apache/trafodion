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

// PageGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "ODBCSvcDlg.h"
#include "PageGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PageGeneral property page

IMPLEMENT_DYNCREATE(PageGeneral, CPropertyPage)

PageGeneral::PageGeneral() : CPropertyPage(PageGeneral::IDD)
{
	//{{AFX_DATA_INIT(PageGeneral)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

PageGeneral::~PageGeneral()
{
}

void PageGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PageGeneral)
	DDX_Control(pDX, IDC_SERVICE_NAME, m_service_name);
	DDX_Control(pDX, IDC_STATIC_GENERAL, m_static_general);
	DDX_Control(pDX, IDC_DSNAME, m_dsname);
	DDX_Control(pDX, IDC_DESC, m_desc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PageGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(PageGeneral)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PageGeneral message handlers

BOOL PageGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	if(!lstrcmpi( aAttr[ KEY_DSN].szAttr, INI_SDEFAULT))
	{	// default data source, so cannot modify data source name
		m_dsname.EnableWindow(FALSE);					
	}
	else
	{	// otherwise, limit data source name length
		m_dsname.LimitText(MAXDSNAME-1);
	}
	m_dsname.SetWindowText( aAttr[ KEY_DSN].szAttr);
	
	m_desc.SetLimitText(MAXDESC-1);
	m_desc.SetWindowText( aAttr[ KEY_DESC].szAttr);

	m_service_name.SetLimitText(MAXSN-1);
	m_service_name.SetWindowText( aAttr[ KEY_SERVICE_NAME].szAttr);

	char szStaticText[]="Enter the name of the new data source; maximum of 32 characters.\n\
The Description field is optional; maximum of 255 characters.";
	m_static_general.SetWindowText(szStaticText);
	
	return FALSE; // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL PageGeneral::OnSetActive() 
{
	m_ppropsheet->SetWizardButtons(PSWIZB_NEXT);	
	return CPropertyPage::OnSetActive();
}

LRESULT PageGeneral::OnWizardNext() 
{
	CString cDSName;
	 
	m_dsname.GetWindowText(cDSName);
	cDSName.TrimLeft();
	cDSName.TrimRight();

// validate data source name argument
	if(cDSName.IsEmpty())
	{
		AfxMessageBox("Data Source Name can not be empty");
		return -1;
	}

	strncpy(aAttr[ KEY_DSN].szAttr,(LPCTSTR)cDSName, sizeof( aAttr[ KEY_DSN].szAttr));

	m_desc.GetWindowText( aAttr[ KEY_DESC].szAttr,sizeof( aAttr[ KEY_DESC].szAttr));
	
	m_service_name.GetWindowText( aAttr[ KEY_SERVICE_NAME].szAttr,sizeof( aAttr[ KEY_SERVICE_NAME].szAttr));
	
	return CPropertyPage::OnWizardNext();
}


