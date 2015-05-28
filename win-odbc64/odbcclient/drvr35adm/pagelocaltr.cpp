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
// PageLocalTr.cpp : implementation file
//

#include "stdafx.h"
#include "drvr35adm.h"
#include "PageLocalTr.h"
#include "TestDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PageLocalTr property page

IMPLEMENT_DYNCREATE(PageLocalTr, CPropertyPage)

PageLocalTr::PageLocalTr() : CPropertyPage(PageLocalTr::IDD)
{
	//{{AFX_DATA_INIT(PageLocalTr)
	//}}AFX_DATA_INIT
}

PageLocalTr::~PageLocalTr()
{
}

void PageLocalTr::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PageLocalTr)
	DDX_Control(pDX, IDC_STATIC_LOCAL_TR, m_static_tr);
	DDX_Control(pDX, IDC_TRANSLATE_OPTION, m_translationoption);
	DDX_Control(pDX, IDC_DLL_NAME, m_dllname);
	DDX_Control(pDX, IDC_COMBO_ERRORLANG, m_errorlang);
	DDX_Control(pDX, IDC_COMBO_DATALANG, m_datalang);
	DDX_Control(pDX, IDC_REPLACEMENT_CHAR, m_replacementchar);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PageLocalTr, CPropertyPage)
	//{{AFX_MSG_MAP(PageLocalTr)
	ON_BN_CLICKED(IDC_DLL_BROWSE, OnDllBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PageLocalTr message handlers

BOOL PageLocalTr::OnInitDialog() 
{
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
	EnumLanguages();

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

	char szStaticText[]="The Translation DLL translates data from one character set to another.\n\n\
Client Error Message Language default is English.\n\
Client/Server Character Set Interaction default is the client local character set.";

	m_static_tr.SetWindowText(szStaticText);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL PageLocalTr::OnSetActive() 
{
	m_ppropsheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	return CPropertyPage::OnSetActive();
}
BOOL PageLocalTr::OnWizardFinish()  
{
	CString cTmp;
	int nCurSel; 
	CTestDialog dlgTest;
	
	m_dllname.GetWindowText( aAttr[ KEY_TRANSLATION_DLL].szAttr,sizeof( aAttr[ KEY_TRANSLATION_DLL].szAttr));
	m_translationoption.GetWindowText( aAttr[ KEY_TRANSLATION_OPTION].szAttr,sizeof( aAttr[ KEY_TRANSLATION_OPTION].szAttr));

	m_errorlang.GetWindowText(cTmp);
	if(CB_ERR==(nCurSel=m_errorlang.FindString(-1,cTmp))) nCurSel=0;
	sprintf(aAttr[ KEY_ERRORLANG].szAttr,"%d",m_errorlang.GetItemData(nCurSel));

	m_datalang.GetWindowText(cTmp);
	if(CB_ERR==(nCurSel=m_datalang.FindString(-1,cTmp))) nCurSel=0;
	sprintf(aAttr[ KEY_DATALANG].szAttr,"%d",nCurSel);

	m_replacementchar.GetWindowText( aAttr[ KEY_REPLACEMENT_CHAR].szAttr,sizeof( aAttr[ KEY_REPLACEMENT_CHAR].szAttr));

	if(IDCANCEL==dlgTest.DoModal()) return FALSE;
	if(FALSE==m_ppropsheet->UpdateDSN()) return FALSE;

	return CPropertyPage::OnWizardFinish();
}
void PageLocalTr::OnDllBrowse() 
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

BOOL CALLBACK EnumResLangProc(
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
	
	PageLocalTr* pPageLocalTr = (PageLocalTr*)lParam;
	LCID lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(wIDLanguage), SUBLANG_DEFAULT), SORT_DEFAULT); 
	GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, szTmp, sizeof(szTmp)); 
	int nCurSel = pPageLocalTr->m_errorlang.AddString(_strupr(szTmp));
	if(nCurSel==CB_ERR) return FALSE;
	pPageLocalTr->m_errorlang.SetItemData(nCurSel, (DWORD)wIDLanguage ); 
	return TRUE;

}


void PageLocalTr::EnumLanguages( void )
{

	HINSTANCE hModule = LoadLibraryEx(DRVR_MSG_DLL, NULL, LOAD_LIBRARY_AS_DATAFILE);
	if(hModule!=NULL)
	{
		EnumResourceLanguages(hModule,
			RT_MESSAGETABLE,
			(char*)1,
			(ENUMRESLANGPROC)EnumResLangProc,
			(LPARAM) this);
	
		FreeLibrary(hModule);
	}
}

 

