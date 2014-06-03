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
	ON_BN_CLICKED(IDC_GETSVRDSN, OnGetsvrdsn)
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
NOTE: If the specified data source cannot be found on the server, the client connects to the default data source.\n\
NOTE: Although data source names on the server may be greater than 32 characters, TRAF ODBC only allows a maximum of 32 characters.\n\
The Description field is optional; maximum of 255 characters.";
	m_static_general.SetWindowText(szStaticText);
	
	return FALSE; // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void PageGeneral::OnGetsvrdsn() 
{
/*
	CODBCSvcDlg dlg;
	BOOL fRemove;
	char *InternalDSN = "Default_DSN_For_GetSRVRDSN";
	RETCODE	returncode;
	HENV henv;
	HDBC hdbc;
	HSTMT hstmt;
	char ConnectString[300];
	UCHAR buf[3000];
	UCHAR State[6];	
	TCHAR ServerDS[120];
	SQLINTEGER	ServerDSLen;


		
	dlg.aAttr = aAttr;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{		
		fRemove = SQLRemoveDSNFromIni(InternalDSN);
		// delete File DSN
		DeleteFileDSN(InternalDSN);
		lstrcpy(aAttr[ KEY_DSN].szAttr,InternalDSN);
		lstrcpy(aAttr[ KEY_DESC].szAttr,"Unique Default Datasource for Internal Use only.");
		
		// update ODBC.INI
		SetDSNAttributes( hwndParent, lpszDrvr, bNewDSN);
		
		henv = (HENV) NULL;
		hdbc = (HDBC) NULL;
		hstmt = (HSTMT) NULL;
		lstrcpy(ConnectString,"DSN=");
		lstrcat(ConnectString,aAttr[ KEY_DSN].szAttr);
		lstrcat(ConnectString,";");
		returncode = SQLAllocEnv(&henv);
		if (returncode != SQL_SUCCESS)
		{
			SQLError(henv,NULL,NULL,State,NULL,buf,3000,NULL);
			AfxMessageBox((char *)buf);
			fRemove = SQLRemoveDSNFromIni(InternalDSN);
			
			// delete File DSN
			DeleteFileDSN(InternalDSN);
			// return TRUE;
			return;
		}
		
		returncode = SQLAllocConnect(henv,&hdbc);
		if (returncode != SQL_SUCCESS)
		{
			SQLError(henv,hdbc,NULL,State,NULL,buf,3000,NULL);
			AfxMessageBox((char *)buf);
			fRemove = SQLRemoveDSNFromIni(InternalDSN);
			
			// delete File DSN
			DeleteFileDSN(InternalDSN);
			SQLFreeEnv(henv);
			// return TRUE;
			return;
		}
		
		returncode = SQLDriverConnect(hdbc,hwndParent,(UCHAR *)ConnectString,(SQLSMALLINT)strlen(ConnectString),SQL_NULL_HANDLE,0,SQL_NULL_HANDLE,SQL_DRIVER_COMPLETE);
		if ((returncode != SQL_SUCCESS) && (returncode != SQL_SUCCESS_WITH_INFO))
		{
			SQLError(henv,hdbc,NULL,State,NULL,buf,3000,NULL);
			AfxMessageBox((char *)buf);
			fRemove = SQLRemoveDSNFromIni(InternalDSN);
			// delete File DSN
			DeleteFileDSN(InternalDSN);
			SQLFreeConnect(hdbc);
			SQLFreeEnv(henv);
			// return TRUE;
			return;
		}
							
		returncode = SQLAllocStmt(hdbc, &hstmt);
		if (returncode != SQL_SUCCESS)
		{
			SQLError(henv,hdbc,hstmt,State,NULL,buf,3000,NULL);
			AfxMessageBox((char *)buf);
			fRemove = SQLRemoveDSNFromIni(InternalDSN);
			// delete File DSN
			DeleteFileDSN(InternalDSN);
			SQLDisconnect(hdbc);
			SQLFreeConnect(hdbc);
			SQLFreeEnv(henv);
			// return TRUE;
			return;
		}
		returncode = SQLExecDirect(hstmt,(unsigned char *)"SELECT OBJ_NAME from tandem_system_nsk.odbc_schema.NAME2ID where OBJ_TYPE = 2",SQL_NTS);
		if (returncode != SQL_SUCCESS)
		{
			SQLError(henv,hdbc,hstmt,State,NULL,buf,3000,NULL);
			AfxMessageBox((char *)buf);
			fRemove = SQLRemoveDSNFromIni(InternalDSN);
			// delete File DSN
			DeleteFileDSN(InternalDSN);
			SQLFreeStmt(hstmt,SQL_CLOSE);		
			SQLFreeStmt(hstmt,SQL_DROP);		
			SQLDisconnect(hdbc);
			SQLFreeConnect(hdbc);
			SQLFreeEnv(henv);
			// return TRUE;
			return;
		}
		lstrcpy(ServerDS,"");	
		returncode = SQLBindCol(hstmt,(SWORD)(1),SQL_C_CHAR,ServerDS,300,&ServerDSLen);
		if (returncode != SQL_SUCCESS)
		{		
			SQLError(henv,hdbc,hstmt,State,NULL,buf,3000,NULL);
			AfxMessageBox((char *)buf);
			fRemove = SQLRemoveDSNFromIni(InternalDSN);
			// delete File DSN
			
			DeleteFileDSN(InternalDSN);			
			SQLFreeStmt(hstmt,SQL_CLOSE);		
			SQLFreeStmt(hstmt,SQL_DROP);		
			SQLDisconnect(hdbc);
			SQLFreeConnect(hdbc);
			SQLFreeEnv(henv);
			// return TRUE;
			return;
		}
		if (returncode == SQL_SUCCESS)
		{
			m_dsname.ResetContent();
			while (returncode != SQL_NO_DATA_FOUND)
			{
				returncode = SQLFetch(hstmt);
				if (returncode == SQL_SUCCESS)
				{	
					m_dsname.AddString(ServerDS);	
					
					lstrcpy(ServerDS,"");					
				}
				
				else if (returncode != SQL_NO_DATA_FOUND)				
				{
				
					SQLError(henv,hdbc,hstmt,State,NULL,buf,3000,NULL);					
					AfxMessageBox((char *)buf);
					fRemove = SQLRemoveDSNFromIni(InternalDSN);
					
					// delete File DSN					
					DeleteFileDSN(InternalDSN);
					SQLFreeStmt(hstmt,SQL_CLOSE);		
					SQLFreeStmt(hstmt,SQL_DROP);		
					SQLDisconnect(hdbc);					
					SQLFreeConnect(hdbc);					
					SQLFreeEnv(henv);
					
					// return TRUE;
					return;
					
				}
				
			}
			
		}
		
		m_dsname.SetCurSel( 0 );

		SQLFreeStmt(hstmt,SQL_CLOSE);		
		SQLFreeStmt(hstmt,SQL_DROP);		
		SQLDisconnect(hdbc);		
		SQLFreeConnect(hdbc);		
		SQLFreeEnv(henv);		
		fRemove = SQLRemoveDSNFromIni(InternalDSN);
		
		// delete File DSN		
		DeleteFileDSN(InternalDSN);	
		
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
*/
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


