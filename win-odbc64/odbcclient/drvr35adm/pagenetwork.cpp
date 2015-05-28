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
// PageNetwork.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "PageNetwork.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PageNetwork property page

IMPLEMENT_DYNCREATE(PageNetwork, CPropertyPage)

PageNetwork::PageNetwork() : CPropertyPage(PageNetwork::IDD)
{
	//{{AFX_DATA_INIT(PageNetwork)
	//}}AFX_DATA_INIT
}

PageNetwork::~PageNetwork()
{
}

void PageNetwork::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PageNetwork)
	DDX_Control(pDX, IDC_FETCH_BUFFER_SIZE, m_fetch_buffer_size);
	DDX_Control(pDX, IDC_STATIC_NETWORK, m_static_network);
	DDX_Control(pDX, IDC_QUERY, m_query);
	DDX_Control(pDX, IDC_PORTNUM, m_portnum);
	DDX_Control(pDX, IDC_LOGIN, m_login);
	DDX_Control(pDX, IDC_IPADDRESS, m_ipaddress);
	DDX_Control(pDX, IDC_CONNECTION, m_connection);
	DDX_Control(pDX, IDC_COMPRESSION, m_Compression);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PageNetwork, CPropertyPage)
	//{{AFX_MSG_MAP(PageNetwork)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PageNetwork message handlers


BOOL PageNetwork::OnInitDialog() 
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


	char szStaticText[]="Enter the DCS Master IP address specified during the Trafodion installation.\n\
Enter the starting port number for the range of ports numbers specified for\n\
DCS Master during the Trafodion installation; default is 37800.\n\n\
Login Timeout default is 0 (no timeout). Connection Timeout default is 60 seconds.\n\
Query Timeout default is NO_TIMEOUT. Fetch Buffer Size default is 8 kbytes."; 

	m_static_network.SetWindowText(szStaticText);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL PageNetwork::OnSetActive() 
{
	m_ppropsheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);	
	return CPropertyPage::OnSetActive();
}

LRESULT PageNetwork::OnWizardBack() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnWizardBack();
}

LRESULT PageNetwork::OnWizardNext() 
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

	strncpy(aAttr[ KEY_LOGIN].szAttr,(LPCTSTR)cLoginTimeout, sizeof(aAttr[ KEY_LOGIN].szAttr));
	strncpy(aAttr[ KEY_CONNECTION].szAttr,(LPCTSTR)cConnectionTimeout, sizeof(aAttr[ KEY_CONNECTION].szAttr));
	strncpy(aAttr[ KEY_QUERY].szAttr,(LPCTSTR)cQueryTimeout, sizeof(aAttr[ KEY_QUERY].szAttr));
	strncpy(aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr,(LPCTSTR)cFetchBufferSize, sizeof(aAttr[ KEY_FETCH_BUFFER_SIZE].szAttr));
	strncpy(aAttr[ KEY_COMPRESSION].szAttr,(LPCTSTR)cCompression, sizeof(aAttr[ KEY_COMPRESSION].szAttr));

	return CPropertyPage::OnWizardNext();
}

