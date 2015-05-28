/**********************************************************************
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
********************************************************************/
// TabPageNetwork.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "TabPageNetwork.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TabPageNetwork property page

IMPLEMENT_DYNCREATE(TabPageNetwork, CPropertyPage)

TabPageNetwork::TabPageNetwork() : CPropertyPage(TabPageNetwork::IDD)
{
	//{{AFX_DATA_INIT(TabPageNetwork)
	//}}AFX_DATA_INIT
}

TabPageNetwork::~TabPageNetwork()
{
}

void TabPageNetwork::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TabPageNetwork)
	DDX_Control(pDX, IDC_STATIC_NETWORK, m_static_network);
	DDX_Control(pDX, IDC_QUERY, m_query);
	DDX_Control(pDX, IDC_PORTNUM, m_portnum);
	DDX_Control(pDX, IDC_LOGIN, m_login);
	DDX_Control(pDX, IDC_IPADDRESS, m_ipaddress);
	DDX_Control(pDX, IDC_FETCH_BUFFER_SIZE, m_fetch_buffer_size);
	DDX_Control(pDX, IDC_CONNECTION, m_connection);
	DDX_Control(pDX, IDC_COMPRESSION, m_Compression);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TabPageNetwork, CPropertyPage)
	//{{AFX_MSG_MAP(TabPageNetwork)
	ON_CBN_EDITCHANGE(IDC_CONNECTION, OnEditchangeConnection)
	ON_CBN_EDITCHANGE(IDC_FETCH_BUFFER_SIZE, OnEditchangeFetchBufferSize)
	ON_EN_CHANGE(IDC_IPADDRESS, OnChangeIpaddress)
	ON_CBN_EDITCHANGE(IDC_LOGIN, OnEditchangeLogin)
	ON_EN_CHANGE(IDC_PORTNUM, OnChangePortnum)
	ON_CBN_EDITCHANGE(IDC_QUERY, OnEditchangeQuery)
	ON_CBN_SELCHANGE(IDC_LOGIN, OnSelchangeLogin)
	ON_CBN_SELCHANGE(IDC_QUERY, OnSelchangeQuery)
	ON_CBN_SELCHANGE(IDC_CONNECTION, OnSelchangeConnection)
	ON_CBN_SELCHANGE(IDC_FETCH_BUFFER_SIZE, OnSelchangeFetchBufferSize)
	ON_CBN_SELCHANGE(IDC_COMPRESSION, OnSelchangeCompression)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TabPageNetwork message handlers

BOOL TabPageNetwork::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_ipaddress.SetWindowText(aAttr[ KEY_IPADDRESS].szAttr);				
	m_portnum.SetWindowText(aAttr[ KEY_PORTNUM].szAttr);

	m_ipaddress.LimitText(MAXIPLEN-1);
	m_portnum.LimitText(MAXPORTLEN-1);

	if( aAttr[ KEY_LOGIN].szAttr[0] != 0 )
		m_login.SetWindowText( aAttr[ KEY_LOGIN].szAttr );
	else
		m_login.SetCurSel(0);

	if( aAttr[ KEY_CONNECTION].szAttr[0] != 0 )
		m_connection.SetWindowText(aAttr[ KEY_CONNECTION].szAttr );
	else
		m_connection.SetCurSel(0);

	if( aAttr[ KEY_QUERY].szAttr[0] != 0 )
		m_query.SetWindowText(aAttr[ KEY_QUERY].szAttr );
	else
		m_query.SetCurSel(1);

	if( aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr[0] != 0 )
		m_fetch_buffer_size.SetWindowText(aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr );
	else
		m_fetch_buffer_size.SetCurSel(0);

	if( aAttr[ KEY_COMPRESSION].szAttr[0] != 0 )
		m_Compression.SetWindowText(aAttr[ KEY_COMPRESSION].szAttr );
	else
		m_Compression.SetCurSel(0);


	char szStaticText[]="The current configuration is displayed.\n\
DCS Master IP address is specified during the Trafodion installation.\n\
Port is the starting port number for the DCS Master specified during\n\
the Trafodion installation; default is 37800.\n\
Login Timeout default is 0 (no timeout). Connection Timeout default is 60 seconds.\n\
Query Timeout is NO_TIMEOUT. Fetch Buffer Size default is 8 kbytes.";

	m_static_network.SetWindowText(szStaticText);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL TabPageNetwork::OnSetActive() 
{
	return CPropertyPage::OnSetActive();
}

void TabPageNetwork::OnOK() 
{
	CPropertyPage::OnOK();
}

void TabPageNetwork::OnCancel() 
{
	CPropertyPage::OnCancel();
}

BOOL TabPageNetwork::OnApply() 
{
	if(	m_ppropsheet->bUpdateDSN==true)
	{
		m_ppropsheet->UpdateDSN();
		m_ppropsheet->bUpdateDSN=false;
	}
	
	return CPropertyPage::OnApply();
}

BOOL TabPageNetwork::OnKillActive() 
{
	CString cIPAddress;
	CString cPortNum;
	CString cLoginTimeout;
	CString cConnectionTimeout;
	CString cQueryTimeout;
	CString cFetchBufferSize;
	CString cCompression;

	m_ipaddress.GetWindowText(cIPAddress);
	m_portnum.GetWindowText(cPortNum);
	m_login.GetWindowText(cLoginTimeout);
 	m_connection.GetWindowText(cConnectionTimeout);
 	m_query.GetWindowText(cQueryTimeout);
 	m_fetch_buffer_size.GetWindowText(cFetchBufferSize);
 	m_Compression.GetWindowText(cCompression);

	cIPAddress.TrimLeft();
	cIPAddress.TrimRight();

	cPortNum.TrimLeft();
	cPortNum.TrimRight();

	cLoginTimeout.TrimLeft();
	cLoginTimeout.TrimRight();

	cConnectionTimeout.TrimLeft();
	cConnectionTimeout.TrimRight();

	cQueryTimeout.TrimLeft();
	cQueryTimeout.TrimRight();

	cFetchBufferSize.TrimLeft();
	cFetchBufferSize.TrimRight();

	cCompression.TrimLeft();
	cCompression.TrimRight();

 	if(cIPAddress.IsEmpty() || cPortNum.IsEmpty())
	{
		AfxMessageBox("IP Address and/or Port Number name can not be empty");
		m_ipaddress.SetFocus();
		return -1;
	}

	strncpy(aAttr[ KEY_IPADDRESS].szAttr,(LPCTSTR)cIPAddress, sizeof(aAttr[ KEY_IPADDRESS].szAttr));
	strncpy(aAttr[ KEY_PORTNUM].szAttr,(LPCTSTR)cPortNum, sizeof(aAttr[ KEY_PORTNUM].szAttr));

	if(cLoginTimeout.IsEmpty())
		cLoginTimeout = "NO_TIMEOUT";

	if(cConnectionTimeout.IsEmpty())
		cConnectionTimeout = "NO_TIMEOUT";

	if(cQueryTimeout.IsEmpty())
		cQueryTimeout = "NO_TIMEOUT";

	if(cFetchBufferSize.IsEmpty())
		cFetchBufferSize = "0";

	if(cCompression.IsEmpty())
		cCompression = "SYSTEM_DEFAULT";

	strncpy(aAttr[ KEY_LOGIN].szAttr,(LPCTSTR)cLoginTimeout, sizeof(aAttr[ KEY_LOGIN].szAttr));
	strncpy(aAttr[ KEY_CONNECTION].szAttr,(LPCTSTR)cConnectionTimeout, sizeof(aAttr[ KEY_CONNECTION].szAttr));
	strncpy(aAttr[ KEY_QUERY].szAttr,(LPCTSTR)cQueryTimeout, sizeof(aAttr[ KEY_QUERY].szAttr));
	strncpy(aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr,(LPCTSTR)cFetchBufferSize, sizeof(aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr));
	strncpy(aAttr[ KEY_COMPRESSION].szAttr,(LPCTSTR)cCompression, sizeof(aAttr[ KEY_COMPRESSION].szAttr));

	return CPropertyPage::OnKillActive();
}

void TabPageNetwork::OnEditchangeConnection() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnEditchangeFetchBufferSize() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnChangeIpaddress() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnEditchangeLogin() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnChangePortnum() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnEditchangeQuery() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnSelchangeLogin() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnSelchangeQuery() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnSelchangeConnection() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnSelchangeFetchBufferSize() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageNetwork::OnSelchangeCompression() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

