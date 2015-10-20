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
// SetCertDirDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SetCertificateDirReg.h"
#include "SetCertDirDlg.h"
#include <odbcinst.h>
#include <ShlObj.h>


#define ODBC_SET_CERTIFICATE_DIR ODBC_CONFIG_DRIVER_MAX+1

CString ErrorString(DWORD err)
{
     CString Error;
     LPTSTR s;
     if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            err,
            0,
            (LPTSTR)&s,
            0,
            NULL) == 0)
    {
		Error = "Unknown error code";
    }
    else
    {
		LPTSTR p = _tcschr(s, _T('\r'));
		if(p != NULL)
		{ /* lose CRLF */
			*p = _T('\0');
		} /* lose CRLF */
		Error = s;
		::LocalFree(s);
    }
    return Error;

} // ErrorString()

// SetCertDirDlg dialog

IMPLEMENT_DYNAMIC(SetCertDirDlg, CDialog)

SetCertDirDlg::SetCertDirDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SetCertDirDlg::IDD, pParent)
{

}

SetCertDirDlg::~SetCertDirDlg()
{
}

void SetCertDirDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_CertificatePath);
	m_CertificatePath.SetWindowText("SYSTEM_DEFAULT");
}


BEGIN_MESSAGE_MAP(SetCertDirDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &SetCertDirDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDOK, &SetCertDirDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT1, &SetCertDirDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()


// SetCertDirDlg message handlers

void SetCertDirDlg::OnBnClickedButton1()
{
	LPMALLOC pMalloc;
    
    if( SUCCEEDED( SHGetMalloc( &pMalloc ) ) ) 
    {
        TCHAR szTitle[] = _T("Choose Directory for storing certificates.");
        BROWSEINFO bi;
        ZeroMemory( &bi, sizeof( bi ) );
        bi.hwndOwner = NULL;
        bi.pszDisplayName = NULL;
        bi.lpszTitle = szTitle;
        bi.pidlRoot = NULL;
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
        
        LPITEMIDLIST pidl = SHBrowseForFolder( &bi );
        if( pidl ) 
        {
            TCHAR szDir[MAX_PATH];
            if( SHGetPathFromIDList( pidl, szDir ) ) 
            {
				m_CertificatePath.SetWindowText(szDir);
				m_CertificatePath.SetFocus();
            }
            pMalloc->Free(pidl); 
            pMalloc->Release();
        }
    }
}

void SetCertDirDlg::OnBnClickedOk()
{
	const char DriverName[]= "TRAF ODBC 1.0";
	CString szCertificateDir;
	TCHAR szCertificateDirRegEntry[_MAX_PATH];
	CHAR  szConfigMsg[_MAX_PATH] = "";
	WORD  cbPathOut;

	m_CertificatePath.GetWindowText(szCertificateDir);

	sprintf_s(szCertificateDirRegEntry,"CertificateDir=%s\0",szCertificateDir);
	SQLConfigDriver( NULL, ODBC_CONFIG_DRIVER, DriverName,
						 szCertificateDirRegEntry,szConfigMsg,
						 sizeof(szConfigMsg), &cbPathOut );
	/*
	if(!SQLConfigDriver( NULL, ODBC_CONFIG_DRIVER, DriverName,
						 szCertificateDirRegEntry,szConfigMsg,
						 sizeof(szConfigMsg), &cbPathOut ))
	{
		CString szErrorMessage;
		DWORD dwErrorCode;
		WORD wRetSize;
		LPSTR pszMessage=szErrorMessage.GetBuffer(SQL_MAX_MESSAGE_LENGTH-4);
		SQLInstallerError(1,&dwErrorCode,pszMessage,SQL_MAX_MESSAGE_LENGTH-8,&wRetSize);
		szErrorMessage.ReleaseBuffer();
		szErrorMessage+="\nDriver Diags:";
		szErrorMessage+=szConfigMsg;
		MessageBox(szErrorMessage,"SQLConfigDriver Failed",MB_ICONERROR|MB_OK);
	}
	*/
	OnOK();
}


void SetCertDirDlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}
