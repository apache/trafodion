// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2002-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

// PageOptions.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "PageOptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PageOptions property page

IMPLEMENT_DYNCREATE(PageOptions, CPropertyPage)

PageOptions::PageOptions() : CPropertyPage(PageOptions::IDD)
{
	//{{AFX_DATA_INIT(PageOptions)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

PageOptions::~PageOptions()
{
	freeDSInfo();
}

void PageOptions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PageOptions)
	DDX_Control(pDX, IDC_COMBO_DSNAMES, m_sdsnames);
	DDX_Control(pDX, IDC_STATIC_OPTIONS, m_static_options);
	DDX_Control(pDX, IDC_SCHEMA, m_schema);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PageOptions, CPropertyPage)
	//{{AFX_MSG_MAP(PageOptions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PageOptions message handlers

BOOL PageOptions::OnInitDialog() 
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
	
	char szStaticText[]="Schema is used to qualify Trafodion object names.\
The default value is SEABASE.";

	m_static_options.SetWindowText(szStaticText);

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL PageOptions::OnSetActive() 
{
	m_ppropsheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);	
	return CPropertyPage::OnSetActive();
}

LRESULT PageOptions::OnWizardBack() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnWizardBack();
}

LRESULT PageOptions::OnWizardNext() 
{
	CString cCatalog;
	CString cSchema;

	m_schema.GetWindowText(cSchema);

	cSchema.TrimLeft();
	cSchema.TrimRight();

	strncpy(aAttr[ KEY_CATALOG].szAttr,(LPCTSTR)cCatalog, sizeof(aAttr[ KEY_CATALOG].szAttr));
	strncpy(aAttr[ KEY_SCHEMA].szAttr,(LPCTSTR)cSchema, sizeof(aAttr[ KEY_SCHEMA].szAttr));

	return CPropertyPage::OnWizardNext();
}

