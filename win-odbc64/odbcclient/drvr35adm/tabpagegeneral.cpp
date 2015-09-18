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

// TabPageGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "TabPageGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TabPageGeneral property page

IMPLEMENT_DYNCREATE(TabPageGeneral, CPropertyPage)

TabPageGeneral::TabPageGeneral() : CPropertyPage(TabPageGeneral::IDD)
{
	//{{AFX_DATA_INIT(TabPageGeneral)
	//}}AFX_DATA_INIT
}

TabPageGeneral::~TabPageGeneral()
{
}

void TabPageGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TabPageGeneral)
	DDX_Control(pDX, IDC_SERVICE_NAME, m_service_name);
	DDX_Control(pDX, IDC_DSNAME, m_dsname);
	DDX_Control(pDX, IDC_STATIC_GENERAL, m_static_general);
	DDX_Control(pDX, IDC_DESC, m_desc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TabPageGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(TabPageGeneral)
	ON_EN_CHANGE(IDC_DESC, OnChangeDesc)
	ON_EN_CHANGE(IDC_DSNAME, OnChangeDsname)
	ON_EN_CHANGE(IDC_SERVICE_NAME, OnChangeServiceName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TabPageGeneral message handlers

BOOL TabPageGeneral::OnInitDialog() 
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
	
	m_desc.LimitText(MAXDESC-1);
	m_desc.SetWindowText( aAttr[ KEY_DESC].szAttr);

	m_service_name.LimitText(MAXSN-1);
	m_service_name.SetWindowText( aAttr[ KEY_SERVICE_NAME].szAttr);
	char szStaticText[]="Enter the name of the new data source; maximum of 32 characters.\n\
The Description field is optional; maximum of 255 characters.";

	m_static_general.SetWindowText(szStaticText);
	
	return FALSE; // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
	
}

BOOL TabPageGeneral::OnSetActive() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnSetActive();
}

void TabPageGeneral::OnOK() 
{
	CPropertyPage::OnOK();
}

BOOL TabPageGeneral::OnApply() 
{
	if(	m_ppropsheet->bUpdateDSN==true)
	{
		m_ppropsheet->UpdateDSN();
		m_ppropsheet->bUpdateDSN=false;
	}
	
	return CPropertyPage::OnApply();
}

void TabPageGeneral::OnCancel() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CPropertyPage::OnCancel();
}

BOOL TabPageGeneral::OnKillActive() 
{
	CString cDSName;
	 
	m_dsname.GetWindowText(cDSName);
	cDSName.TrimLeft();
	cDSName.TrimRight();

// validate data source name argument
	if(cDSName.IsEmpty())
	{
		AfxMessageBox("Data Source Name can not be empty");
		return FALSE;
	}

	strncpy(aAttr[ KEY_DSN].szAttr,(LPCTSTR)cDSName, sizeof( aAttr[ KEY_DSN].szAttr));

	m_desc.GetWindowText( aAttr[ KEY_DESC].szAttr,sizeof( aAttr[ KEY_DESC].szAttr));
	m_service_name.GetWindowText( aAttr[ KEY_SERVICE_NAME].szAttr,sizeof( aAttr[ KEY_SERVICE_NAME].szAttr));
	
	return CPropertyPage::OnKillActive();
}

void TabPageGeneral::OnChangeDesc() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageGeneral::OnChangeDsname() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageGeneral::OnChangeServiceName() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}
