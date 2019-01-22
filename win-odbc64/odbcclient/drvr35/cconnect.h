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
/**************************************************************************
**************************************************************************/
//
//
#ifndef CCONNECT_H
#define CCONNECT_H

#include "CHandle.h"
#include "CEnv.h"
#include "CDataSource.h"
#include "odbcMxSecurity.h"
#include "drvrnet.h"
#include "Transport.h"
#include "TCPIPSystemDrvr.h"
#include "secpwd.h"
#include "charsetconv.h"


#define NON_CHARSET_SYSTEM 9001

INT_PTR CALLBACK ChangePwdProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
);

class CStmt;

class CConnect : public CHandle {

public:
	CConnect(SQLHANDLE InputHandle);
	~CConnect();
//	SQLRETURN InitializeTranslation();
	SQLRETURN Connect(SQLCHAR *ServerName, 
		   SQLSMALLINT	NameLength1,
           SQLCHAR		*UserName, 
		   SQLSMALLINT	NameLength2,
           SQLCHAR		*Authentication, 
		   SQLSMALLINT	NameLength3,
		   BOOL			readDSN);
	SQLRETURN generateConnectionString(CONNECT_KEYWORD_TREE *KeywordTree, 
			CONNECT_FIELD_ITEMS	*ConnectFieldItems,
			short				DSNAttrNo, 
			SQLCHAR				*OutConnectionString,
			SQLSMALLINT			BufferLength,
			SQLSMALLINT 		*StringLength2Ptr);
	SQLRETURN prepareForConnect(CONNECT_KEYWORD_TREE *KeywordTree, 
			CONNECT_FIELD_ITEMS &connectFieldItems, 
			short				DSNAttrNo,
			short				DSNType,
			char				*ServerNameNTS);
	SQLRETURN BrowseConnect(SQLCHAR *InConnectionString,
			SQLSMALLINT        StringLength1,
			SQLCHAR 		  *OutConnectionString,
			SQLSMALLINT        BufferLength,
			SQLSMALLINT       *StringLength2Ptr);
	SQLRETURN DriverConnect(SQLHWND WindowHandle,
			SQLCHAR			*InConnectionString,
			SQLSMALLINT     StringLength1,
			SQLCHAR         *OutConnectionString,
			SQLSMALLINT     BufferLength,
			SQLSMALLINT 	*StringLength2Ptr,
			SQLUSMALLINT    DriverCompletion);
	
	SQLRETURN Disconnect();
	SQLRETURN SetConnectAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength);
	SQLRETURN GetConnectAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
								   SQLINTEGER *StringLengthPtr);
	SQLRETURN EndTran(SQLSMALLINT CompletionType);
	SQLRETURN GetInfo(SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr);

	SQLRETURN AllocHandle(SQLSMALLINT HandleType,
			SQLHANDLE InputHandle, 
			SQLHANDLE *OutputHandle);

	SQLRETURN NativeSql(
			SQLWCHAR *InStatementTextW,
			SQLINTEGER TextLength1,
			SQLWCHAR *OutStatementTextW,
			SQLINTEGER BufferLength,
			SQLINTEGER *TextLength2Ptr);
		
	void setVersion(const VERSION_LIST_def *versionList,int componentId);
	void getVersion(VERSION_def *versionList, int componentId);
	void setUserSid(const USER_SID_def *userSid);
	void setOutContext(const OUT_CONNECTION_CONTEXT_def *outContext);
	void setGetObjRefHdlOutput(const IDL_char *srvrObjRef,
		DIALOGUE_ID_def dialogueId,
		const IDL_char *dataSource, 
		const USER_SID_def *userSid,
		const VERSION_LIST_def *versionList,
		const IDL_long srvrNodeId,
		const IDL_long srvrProcessId,
		const IDL_long_long timestamp);
	inline void setRetSrvrObjRef(const IDL_OBJECT_def objref) { strcpy(m_RetSQLSvc_ObjRef, objref); };
	long sendCDInfo(long exception_nr);
	long sendStopServer();
	SQLRETURN initialize();
	void resetGetObjRefHdlOutput();
		
	inline DWORD getErrorMsgLang() { return m_DSValue.m_DSErrorMsgLang; };
	inline DWORD getDataLang() { return m_DSValue.m_DSDataLang; };
	inline DWORD getCharSet() { return m_DSValue.m_DSCharSet; };
	inline char *getReplacementChar() { return m_DSValue.m_DSReplacementChar; };
	inline IDL_Object getRetSrvrObjRef() { return m_RetSQLSvc_ObjRef; };
	inline IDL_Object getSrvrObjRef() { return m_SQLSvc_ObjRef; };
	inline IDL_Object getAsObjRef() { return m_ASSvc_ObjRef; };
	inline char *getSrvrDSName() { return m_SrvrDSName; };
	inline char *getSrvrIdentity() { return m_SrvrIdentity; };
	inline DIALOGUE_ID_def	getDialogueId() { return m_DialogueId; } ;
	inline SQLUINTEGER getConnectionTimeout() { return m_ConnectionTimeout;};
	inline BOOL getConnectionStatus() { return m_ConnectionDead; };
	inline char *getCurrentCatalog() { return m_CurrentCatalog; };
	inline char *getCurrentSchema() { return m_DSValue.m_DSSchema; };
	inline SQLINTEGER getODBCAppVersion() { return m_EnvHandle->getODBCAppVersion();};
	inline long getFetchBufferSize() {return m_FetchBufferSize;};
	inline BOOL getChangePasswordStatus(){return m_ChangePassword;}
	inline DWORD getTranslateOption() { return m_TranslateOption; };
	DWORD getSqlCharSet(long inSqlCharSet);
	DWORD getDrvrCharSet();
	inline BOOL	getByteSwap() { 
		if(m_srvrTCPIPSystem->swap() == SWAP_YES)
			return TRUE;
		else
			return FALSE;
	};
	inline BOOL getSelectRowsets() { return m_SelectRowsets; };
	void deleteValueList();
	inline BOOL rowsetSupported(){return m_SQLRowsetSupported;}
	inline BOOL getFlushFetchData() { return m_FlushFetchData; };

	inline CEnv* getEnvHandle(){ return m_EnvHandle; }
	inline BOOL rowsetErrorRecovery(){return m_RowsetErrorRecovery;};
	BOOL getUserDesc(char *userNameNTS, char *AuthenticationNT, USER_DESC_def *userDesc);
	int getUTF8CharLength(const char *inputChar, const int inputLength, const int maxLength);

	void setSecurityError(int error, char* sqlState, char* errorMsg);
	inline void setRetryEncryption(){ m_RetryEncryption = true; };

private:
	SQLRETURN SetConnectAttr(SQLINTEGER Attribute, SQLUINTEGER valueNum, SQLPOINTER ValueStr, bool bClearErrors = true);
	short getTransactionNumber(void);
	SQLRETURN DoEncryption(SecPwd* &m_SecPwd, ProcInfo SecInfo,USER_DESC_def &userDesc,char *AuthenticationNTS,CONNECTION_CONTEXT_def &inContext);

public:
	SRVR_CALL_CONTEXT			m_srvrCallContext;
	// TCPIPSystems
	CTCPIPSystemDrvr*			m_asTCPIPSystem;
	CTCPIPSystemDrvr*			m_srvrTCPIPSystem;
	CRITICAL_SECTION			m_CSTransmision;
	BOOL						m_IgnoreCancel; // If SQL_ATTR_IGNORE_CANCEL is set at the server DSN, then SQLCancel will issue a sendStopSrvr request

	SecPwd*			m_SecPwd;
	ProcInfo		m_SecInfo;
    char            m_ClusterName[MAX_SQL_IDENTIFIER_LEN+1]; // seaquest cluster name
    int             m_ClusterNameLength;
private:
	HANDLE						m_ConnectEvent;
	IDL_OBJECT_def				m_RetSQLSvc_ObjRef;
	IDL_OBJECT_def				m_ASSvc_ObjRef;
	IDL_OBJECT_def				m_SQLSvc_ObjRef;
	DIALOGUE_ID_def				m_DialogueId;
	SQL_IDENTIFIER_def			m_SrvrDSName;
	short						m_SrvrDSReadType;		// 0 - Read Only  1- Read Write
   	VERSION_def					m_ASVersion;
	VERSION_def					m_SrvrVersion;
	VERSION_def					m_SqlVersion;
	BOOL						m_CDInfoSent;
	char						m_SrvrIdentity[256];

	char						m_UserName[UNLEN+1];
	char						m_UserRole[SQL_MAX_ROLENAME_LEN+1];
	char						m_UserDomain[DNLEN+1];
	char						m_UserSid[MAX_TEXT_SID_LEN];
	
	CDataSource					m_DSValue;
	CEnv						*m_EnvHandle;
	
	// If rowsets supported
	BOOL						m_SQLRowsetSupported;
	BOOL						m_RowsetErrorRecovery;

	// To support Dynamically change Fetch Buffer Size to ZERO for PUB/SUB and update/delete where current of CURSOR
	long						m_FetchBufferSize;		// in Bytes

	// Translate DLL 
	HMODULE						m_TranslateLibHandle;
	FPSQLDriverToDataSource		m_FPSQLDriverToDataSource;
	FPSQLDataSourceToDriver		m_FPSQLDataSourceToDriver;


	// Attributes that can be overwritten by the server
	SQLUINTEGER		m_AccessMode;
	
	// Attributes that can be obtained from the server
	char			m_CurrentCatalog[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_CurrentSchema[MAX_SQL_IDENTIFIER_LEN+1]; 
	
	// Attributes that are local to the connection
	SQLUINTEGER		m_AutoIPD;
	SQLUINTEGER		m_AutoCommit;
	SQLUINTEGER		m_ConnectionTimeout;
	SQLUINTEGER		m_LoginTimeout;
	SQLUINTEGER		m_ODBCCursors;
	SQLUINTEGER		m_PacketSize;
	HWND			m_QuietMode;
	SQLUINTEGER		m_Trace;
	char			m_TraceFile[MAX_SQL_IDENTIFIER_LEN+1];;
	char			m_TranslateLib[MAX_SQL_IDENTIFIER_LEN+1];;
	DWORD			m_TranslateOption;
	SQLUINTEGER		m_TxnIsolation;
	BOOL			m_ConnectionDead;
	BOOL			m_SelectRowsets;
	SQLUINTEGER		m_IOCompression;
	int                     m_IOCompressionThreshold;
	BOOL			m_ChangePassword; // for password expiry
	BOOL			m_FlushFetchData; // For query driver which cannot handle huge data being returned.
	SQLINTEGER      m_StartNode; // Node(cpu) to start the mxosrvr on - defaults to -1 (any)

	// Attributes that are to be propgated to the future Statement Handles as per 3.0 Spec
	SQLUINTEGER		m_AsyncEnable;
	SQLUINTEGER		m_MetadataId;

	// Attributes that are supported in Connection for ODBC 2.x applications
	// Future statements will inherit these values
	SQLUINTEGER		m_Concurrency;				// 2.0
	SQLUINTEGER		m_CursorType;				// 2.0
	SQLULEN 		m_MaxLength;				// 1.0  -expanded for 64 bit
    SQLULEN 		m_MaxRows;					// 1.0  -expanded for 64 bit
	SQLUINTEGER		m_Noscan;					// 1.0	
	SQLUINTEGER		m_QueryTimeout;				// 1.0
	SQLUINTEGER		m_SimulateCursor;			// 2.0
	SQLUINTEGER		m_UseBookmarks;				// 2.0
	// Collection of Stmt Handles in this Connection
	CHANDLECOLLECT	m_StmtCollect;
	// Collection of Descriptors allocated by a user
	CHANDLECOLLECT	m_DescCollect;

	//wms_mapping
	// for query service name
	char			m_QSServiceName[SQL_MAX_SERVICENAME_LEN+1];
	char			m_QueryID_SessionName[SQL_MAX_SESSIONNAME_LEN*4+1]; 
	char			m_applName[SQL_MAX_APPLNAME_LEN*4 + 1];

	SQLUINTEGER		m_FetchAhead;

	// Security
	char			m_CertificateDir[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_CertificateFile[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_CertificateFileActive[MAX_SQL_IDENTIFIER_LEN+1];
	SQLUINTEGER		m_SecurityMode;
	bool			m_RetryEncryption;

    //hold the lob handle from last select for insert >16m
    IDL_string      lobHandleSave;
    IDL_long        lobHandleLenSave;

	void reset(bool clearE=true);
	friend class	CStmt;
	friend class	CDesc;

};

#endif
