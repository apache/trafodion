/**********************************************************************
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
********************************************************************/
// TabPageTesting.cpp : implementation file
//

#include "stdafx.h"
#include "Drvr35adm.h"
#include "TabPageTesting.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TabPageTesting property page

IMPLEMENT_DYNCREATE(TabPageTesting, CPropertyPage)

TabPageTesting::TabPageTesting() : CPropertyPage(TabPageTesting::IDD)
{
	//{{AFX_DATA_INIT(TabPageTesting)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

TabPageTesting::~TabPageTesting()
{
}

void TabPageTesting::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TabPageTesting)
	DDX_Control(pDX, IDC_TESTCONN, m_testconn);
	DDX_Control(pDX, IDC_OUTPUT, m_output);
	DDX_Control(pDX, IDC_STATIC_TESTING, m_static_testing);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TabPageTesting, CPropertyPage)
	//{{AFX_MSG_MAP(TabPageTesting)
	ON_BN_CLICKED(IDC_TESTCONN, OnTestconn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TabPageTesting message handlers

BOOL TabPageTesting::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL TabPageTesting::OnSetActive() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnSetActive();
}

void TabPageTesting::OnOK() 
{
	CPropertyPage::OnOK();
}

void TabPageTesting::OnCancel() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CPropertyPage::OnCancel();
}

BOOL TabPageTesting::OnApply() 
{
	if(	m_ppropsheet->bUpdateDSN==true)
	{
		m_ppropsheet->UpdateDSN();
		m_ppropsheet->bUpdateDSN=false;
	}
	
	return CPropertyPage::OnApply();
}

BOOL TabPageTesting::OnKillActive() 
{
	return CPropertyPage::OnKillActive();
}

void TabPageTesting::OnTestconn() 
{
	RETCODE	returncode=SQL_SUCCESS;
	HENV henv=NULL;
	HDBC hdbc=NULL;
	char ConnectString[2048];
	UCHAR buf[3001];
	UCHAR State[6];

	m_output.ResetContent();

__try
{
	ListBoxAddString("Trying to Connect....");
	UpdateWindow();
	// Association Service (IP Address and Port Number)
	wsprintf((char*)buf, "%s%s/%s", TCP_STR, aAttr[ KEY_IPADDRESS].szAttr, aAttr[ KEY_PORTNUM].szAttr);
	sprintf(ConnectString,"DRIVER=%s;SERVER=%s;CATALOG=%s;SCHEMA=%s;ServerDSN=%s;",
			DRIVER_NAME, buf, aAttr[KEY_CATALOG].szAttr, aAttr[KEY_SCHEMA].szAttr, aAttr[ KEY_DSN].szAttr);

	if (SQL_SUCCESS!=(returncode = SQLAllocEnv(&henv)))
	{
		SQLError(henv,NULL,NULL,State,NULL,buf,3000,NULL);		
		__leave;
	}
	returncode = SQLAllocConnect(henv,&hdbc);
	if (returncode != SQL_SUCCESS)
	{
		SQLError(henv,hdbc,NULL,State,NULL,buf,3000,NULL);
		__leave;
	}

	returncode = SQLDriverConnect(hdbc,hwndParent,(UCHAR *)ConnectString,(SQLSMALLINT)strlen(ConnectString),SQL_NULL_HANDLE,0,SQL_NULL_HANDLE,SQL_DRIVER_COMPLETE);
	if (returncode == SQL_SUCCESS || returncode == SQL_SUCCESS_WITH_INFO)
	{
		strcpy((char *)buf,"Connected Successfully.");
		Sleep(5*1000);
		SQLDisconnect(hdbc);
	}	
	else
	{
		SQLError(henv,hdbc,NULL,State,NULL,buf,3000,NULL);
	}
}
__finally
{
	ListBoxAddString((char *)buf);

	if(hdbc!=NULL) SQLFreeConnect(hdbc);
	if(henv!=NULL) SQLFreeEnv(henv);
	if(returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
	{
		ListBoxAddString("Connection Test failed.");
	}
}
}

void TabPageTesting::ListBoxAddString(char* lpString)
{
	TEXTMETRIC tm;
	RECT rect;
 
	DWORD dwLBWidth;
	DWORD dwStringExt;
	static DWORD dwLongest=0;

	CDC* pDC = m_output.GetDC();
	HFONT hFont = (HFONT)m_output.SendMessage(WM_GETFONT);
	CFont *pFont = CFont::FromHandle(hFont);

	CFont* pPrevFont = pDC->SelectObject(pFont);

	pDC->GetTextMetrics(&tm);

	CSize cExtent = pDC->GetTextExtent(lpString, strlen(lpString)) + CSize(tm.tmAveCharWidth,0);
	dwStringExt= cExtent.cx;

	pDC->SelectObject(pPrevFont);
	m_output.ReleaseDC(pDC);

	m_output.GetClientRect( &rect );
	dwLBWidth = rect.right-rect.left;

	if( (dwStringExt > dwLongest)|| ( dwStringExt > dwLBWidth ) ) 
	{
		dwLongest = max( dwLongest, dwStringExt );
		m_output.SetHorizontalExtent( dwLongest );
	}
	m_output.AddString(lpString);
}

