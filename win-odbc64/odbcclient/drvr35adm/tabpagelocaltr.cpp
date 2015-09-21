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

// TabPageLocalTr.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "TabPageLocalTr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TabPageLocalTr property page

IMPLEMENT_DYNCREATE(TabPageLocalTr, CPropertyPage)

TabPageLocalTr::TabPageLocalTr() : CPropertyPage(TabPageLocalTr::IDD)
{
	//{{AFX_DATA_INIT(TabPageLocalTr)
	//}}AFX_DATA_INIT
}

TabPageLocalTr::~TabPageLocalTr()
{
}

void TabPageLocalTr::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TabPageLocalTr)
	DDX_Control(pDX, IDC_TRANSLATE_OPTION, m_translationoption);
	DDX_Control(pDX, IDC_STATIC_LOCAL_TR, m_static_tr);
	DDX_Control(pDX, IDC_DLL_NAME, m_dllname);
	DDX_Control(pDX, IDC_COMBO_ERRORLANG, m_errorlang);
	DDX_Control(pDX, IDC_COMBO_DATALANG, m_datalang);
	DDX_Control(pDX, IDC_REPLACEMENT_CHAR, m_replacementchar);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TabPageLocalTr, CPropertyPage)
	//{{AFX_MSG_MAP(TabPageLocalTr)
	ON_BN_CLICKED(IDC_DLL_BROWSE, OnDllBrowse)
	ON_CBN_EDITCHANGE(IDC_COMBO_DATALANG, OnEditchangeComboDatalang)
	ON_CBN_SELCHANGE(IDC_COMBO_DATALANG, OnSelchangeComboDatalang)
	ON_CBN_EDITCHANGE(IDC_COMBO_ERRORLANG, OnEditchangeComboErrorlang)
	ON_CBN_SELCHANGE(IDC_COMBO_ERRORLANG, OnSelchangeComboErrorlang)
	ON_EN_CHANGE(IDC_DLL_NAME, OnChangeDllName)
	ON_EN_CHANGE(IDC_TRANSLATE_OPTION, OnChangeTranslateOption)
	ON_EN_CHANGE(IDC_REPLACEMENT_CHAR, OnChangeReplacementChar)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TabPageLocalTr message handlers

void TabPageLocalTr::OnDllBrowse() 
{
	char BASED_CODE szFilter[] = "DLL Files (*.dll)|*.dll|All Files (*.*)|*.*||";
	CFileDialog Dlg (FALSE, "*.dll", NULL, OFN_HIDEREADONLY,szFilter);
	Dlg.m_ofn.lpstrTitle="Select TRAF ODBC Translation DLL";
	if (Dlg.DoModal () == IDOK)
	{
		CString szPathName=Dlg.GetPathName();
		m_dllname.SetWindowText(szPathName);
		m_dllname.SetFocus();

	}

}

void TabPageLocalTr::OnOK() 
{
	CPropertyPage::OnOK();
}

void TabPageLocalTr::OnCancel() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CPropertyPage::OnCancel();
}

BOOL TabPageLocalTr::OnApply() 
{
	if(	m_ppropsheet->bUpdateDSN==true)
	{
		m_ppropsheet->UpdateDSN();
		m_ppropsheet->bUpdateDSN=false;
	}
	
	return CPropertyPage::OnApply();
}

BOOL TabPageLocalTr::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	char szTmp[MAX_PATH];

	CPropertyPage::OnInitDialog();

	m_dllname.SetWindowText( aAttr[ KEY_TRANSLATION_DLL].szAttr);
	m_translationoption.SetWindowText( aAttr[ KEY_TRANSLATION_OPTION].szAttr);

// For errors we support "SYSTEM_DEFAULT", "1033", "1041"
	int nCurSel = 0;
#if 0
	int nCurSel = m_errorlang.AddString("SYSTEM_DEFAULT");
	m_errorlang.SetItemData(nCurSel, (DWORD)0 ); 
#endif
	TabEnumLanguages();

// For data we support "SYSTEM_DEFAULT", "UTF8", "UTF16", "UTF32", "ISO-88591", "SJIS", "EUC-JP", 
// "KSC", "BIG5", "GB2312", "GB18030"
	m_datalang.AddString( "SYSTEM_DEFAULT");
#if 0	//only allow SYSTEM_DEFAULT character set interaction
	m_datalang.AddString( "UTF8");
	m_datalang.AddString( "UTF16");
	m_datalang.AddString( "UTF32");
	m_datalang.AddString( "ISO-88591");
	m_datalang.AddString( "SJIS");
	m_datalang.AddString( "EUC-JP");
	m_datalang.AddString( "KSC");
	m_datalang.AddString( "BIG5");;
	m_datalang.AddString( "GB2312");
	m_datalang.AddString( "GB18030");
#endif	

// assume default values for Localization (internationalization) keys
	if(aAttr [KEY_ERRORLANG].szAttr[0]==0)
		strcpy (aAttr [KEY_ERRORLANG].szAttr, "0");
	if(aAttr [KEY_DATALANG].szAttr[0]==0)
		strcpy (aAttr [KEY_DATALANG].szAttr, "0");

	int nTmp = atol(aAttr [KEY_ERRORLANG].szAttr);
	if(nTmp!=0)
	{
		LCID lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(nTmp), SUBLANG_DEFAULT), SORT_DEFAULT); 
		if(!GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, szTmp, sizeof(szTmp))) 
			nTmp=0;
		else
			if(CB_ERR==(nTmp=m_errorlang.FindString(-1,_strupr(szTmp)))) nTmp=0;
	}

	m_errorlang.SetCurSel(nTmp);
	
	nCurSel = atol(aAttr [KEY_DATALANG].szAttr);
	if(CB_ERR == m_datalang.SetCurSel(nCurSel))
		m_datalang.SetCurSel(0);

	m_replacementchar.SetWindowText( aAttr[ KEY_REPLACEMENT_CHAR].szAttr);

	char szStaticText[]="The current configuration is displayed.\n\
The translation DLL translates data from one character set to another.\n\
Client Error Message Language default is English.\n\
Client/Server Character Set Interaction default is the client local character set.";

	m_static_tr.SetWindowText(szStaticText);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CALLBACK TabEnumResLangProc(
	HMODULE hModule, 
	LPCTSTR lpszType,
	LPCTSTR lpszName,
	LANGID wIDLanguage,
	LPARAM lParam)
{
	char szTmp[MAX_PATH]; 
	hModule=hModule;
	lpszType=lpszType;
	lpszName=lpszName;
	
	TabPageLocalTr* pPageLocalTr = (TabPageLocalTr*)lParam;
	LCID lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(wIDLanguage), SUBLANG_DEFAULT), SORT_DEFAULT); 
	GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, szTmp, sizeof(szTmp)); 
	int nCurSel = pPageLocalTr->m_errorlang.AddString(_strupr(szTmp));
	if(nCurSel==CB_ERR) return FALSE;
	pPageLocalTr->m_errorlang.SetItemData(nCurSel, (DWORD)wIDLanguage ); 
	return TRUE;

}


void TabPageLocalTr::TabEnumLanguages( void )
{

	HINSTANCE hModule = LoadLibraryEx(DRVR_MSG_DLL, NULL, LOAD_LIBRARY_AS_DATAFILE);
	if(hModule!=NULL)
	{
		EnumResourceLanguages(hModule,
			RT_MESSAGETABLE,
			(char*)1,
			(ENUMRESLANGPROC)TabEnumResLangProc,
			(LPARAM) this);
	
		FreeLibrary(hModule);
	}
}


BOOL TabPageLocalTr::OnKillActive() 
{
	CString cTmp;
	int nCurSel; 
	
	m_dllname.GetWindowText( aAttr[ KEY_TRANSLATION_DLL].szAttr,sizeof( aAttr[ KEY_TRANSLATION_DLL].szAttr));
	m_translationoption.GetWindowText( aAttr[ KEY_TRANSLATION_OPTION].szAttr,sizeof( aAttr[ KEY_TRANSLATION_OPTION].szAttr));

	m_errorlang.GetWindowText(cTmp);
	if(CB_ERR==(nCurSel=m_errorlang.FindString(-1,cTmp))) nCurSel=0;
	sprintf(aAttr[ KEY_ERRORLANG].szAttr,"%d",m_errorlang.GetItemData(nCurSel));

	m_datalang.GetWindowText(cTmp);
	if(CB_ERR==(nCurSel=m_datalang.FindString(-1,cTmp))) nCurSel=0;
	sprintf(aAttr[ KEY_DATALANG].szAttr,"%d",nCurSel);
	
	m_replacementchar.GetWindowText( aAttr[ KEY_REPLACEMENT_CHAR].szAttr,sizeof( aAttr[ KEY_REPLACEMENT_CHAR].szAttr));

	return CPropertyPage::OnKillActive();
}

void TabPageLocalTr::OnEditchangeComboDatalang() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageLocalTr::OnSelchangeComboDatalang() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageLocalTr::OnEditchangeComboErrorlang() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageLocalTr::OnSelchangeComboErrorlang() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageLocalTr::OnChangeDllName() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageLocalTr::OnChangeTranslateOption() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

void TabPageLocalTr::OnChangeReplacementChar() 
{
	SetModified(TRUE);
	m_ppropsheet->bUpdateDSN=true;
}

