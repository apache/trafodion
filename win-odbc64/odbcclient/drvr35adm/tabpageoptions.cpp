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

// TabPageOptions.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "TabPageOptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TabPageOptions property page

IMPLEMENT_DYNCREATE(TabPageOptions, CPropertyPage)

TabPageOptions::TabPageOptions() : CPropertyPage(TabPageOptions::IDD)
{
	//{{AFX_DATA_INIT(TabPageOptions)
	//}}AFX_DATA_INIT
}

TabPageOptions::~TabPageOptions()
{
	freeDSInfo();
}

void TabPageOptions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TabPageOptions)
	DDX_Control(pDX, IDC_COMBO_DSNAMES, m_sdsnames);
	DDX_Control(pDX, IDC_STATIC_OPTIONS, m_static_options);
	DDX_Control(pDX, IDC_SCHEMA, m_schema);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TabPageOptions, CPropertyPage)
	//{{AFX_MSG_MAP(TabPageOptions)
	ON_EN_CHANGE(IDC_SCHEMA, OnChangeSchema)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TabPageOptions message handlers

BOOL TabPageOptions::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
		
	m_schema.SetWindowText( aAttr[ KEY_SCHEMA].szAttr);
	m_schema.LimitText(MAXCATLEN-1);
	m_schema.SetFocus();

	getDSInfo(aAttr[ KEY_IPADDRESS].szAttr, aAttr[ KEY_PORTNUM].szAttr);
	char* sDSName = getFirstDSName();

	while (sDSName[0] != 0)
	{
		m_sdsnames.AddString(sDSName);
		sDSName = getNextDSName();
	}

	char szStaticText[]="The current configuration is displayed.\n\
Schema is used to qualify Trafodion object names.\
The default value is SEABASE.";

	m_static_options.SetWindowText(szStaticText);

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL TabPageOptions::OnSetActive() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnSetActive();
}

void TabPageOptions::OnOK() 
{
	CPropertyPage::OnOK();
}

void TabPageOptions::OnCancel() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CPropertyPage::OnCancel();
}

BOOL TabPageOptions::OnApply() 
{
	if(	m_ppropsheet->bUpdateDSN==true)
	{
		m_ppropsheet->UpdateDSN();
		m_ppropsheet->bUpdateDSN=false;
	}
	
	return CPropertyPage::OnApply();
}

BOOL TabPageOptions::OnKillActive() 
{
	CString cCatalog;
	CString cSchema;

	m_schema.GetWindowText(cSchema);

	cSchema.TrimLeft();
	cSchema.TrimRight();
	
	strncpy(aAttr[ KEY_CATALOG].szAttr,(LPCTSTR)cCatalog, sizeof(aAttr[ KEY_CATALOG].szAttr));
	strncpy(aAttr[ KEY_SCHEMA].szAttr,(LPCTSTR)cSchema, sizeof(aAttr[ KEY_SCHEMA].szAttr));

	return CPropertyPage::OnKillActive();
}

void TabPageOptions::OnChangeSchema() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}
