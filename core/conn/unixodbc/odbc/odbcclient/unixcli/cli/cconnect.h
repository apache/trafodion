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

#include <sql.h>

#include "chandle.h"
#include "cenv.h"
#include "cdatasource.h"
#include "odbcMxSecurity.h"
#include "drvrnet.h"
#include "transport.h"
#include "bucket.h"
#include "secpwd.h"
#include "charsetconv.h"
#include <hpsqlext.h>
#define NON_CHARSET_SYSTEM 9001

#ifndef unixcli
#include "FileSystemDrvr.h"
#else
#include "TCPIPUnixDrvr.h"
#endif
#include <asyncIO.h>
#include "odbcas_cl.h"

class CStmt;
class CDesc;

extern "C"  {
	void* odbc_SQLDrvr_PreFetch_pst_(void *arg);
}

typedef enum PREFETCH_STATE
{
	PREFETCH_STATE_WRK_UNASSIGNED = 0,
	PREFETCH_STATE_WRK_ASSIGNED,
	PREFETCH_STATE_WRK_STARTED,
	PREFETCH_STATE_WRK_DONE,
    PREFETCH_STATE_WRK_CANCELLED
} PREFETCH_STATE;

class preFetchThread {
public:
   preFetchThread(CConnect *pConnection) {
	  int status;     
      m_pConnection = pConnection;
	  m_State = PREFETCH_STATE_WRK_UNASSIGNED;
	  m_pStatement = NULL;
	  m_rbuffer_length = 0;
	  m_rbuffer = NULL;
	  initializeResults();

      status = pthread_attr_init(&m_thread_attr);
	  if(status != 0)
	     gDrvrGlobal.gDisablePreFetch = true;
	  m_InitializationComplete = false;
   }

   void createThread() {
	  pthread_create( &m_thread,             // prefetch thread
	  		          &m_thread_attr,        // Thread attributes
					  odbc_SQLDrvr_PreFetch_pst_,   // Entry point for the prefetch thread
					  (void*)m_pConnection);          // Thread argument is the connection handle
   }

   ~preFetchThread() {
      if(m_rbuffer != NULL)
	     delete[] m_rbuffer;
	  pthread_cancel(m_thread);
	  pthread_join(m_thread,NULL);

   }
   void WaitCondition() {
      m_conditionVar.Lock();
      m_conditionVar.Wait();
      m_conditionVar.UnLock();
   }

   void SignalCondition() {
      m_conditionVar.Lock();
      m_conditionVar.Signal();
      m_conditionVar.UnLock();
   }

   void setSrvrCallContext(SRVR_CALL_CONTEXT *srvrCallContext);

   SRVR_CALL_CONTEXT* getSrvrCallContext() { 
      return &m_SrvrCallContext;
   }

   void initTask() {
      m_State = PREFETCH_STATE_WRK_UNASSIGNED;
   }

   void assignTask() {
      m_State = PREFETCH_STATE_WRK_ASSIGNED;
   }

   void beginTask() {
      m_State = PREFETCH_STATE_WRK_STARTED;
   }

   void endTask() {
      m_State = PREFETCH_STATE_WRK_DONE;
   }

   void cancelTask() {
      m_State = PREFETCH_STATE_WRK_CANCELLED;
   }

   PREFETCH_STATE getState() {
      return m_State;
   }

   void setStmtHandle(CStmt *stmt) {
      m_pStatement = stmt;
   }

   CStmt* getStmtHandle(void) {
      return m_pStatement;
   }

   void r_allocate( int size) {
      if(size > m_rbuffer_length)
	  {
	     if(m_rbuffer != NULL)
	        delete[] m_rbuffer;
		 m_rbuffer = new unsigned char[size];
		 m_rbuffer_length = size;
	  }
   }

   void copyOutBuffer(SQL_DataValue_def &output) {
      r_allocate(output._length);
	  memcpy(m_rbuffer,output._buffer,output._length);
	  outputDataValue._length = output._length;
	  outputDataValue._buffer = m_rbuffer;
   }

   void initializeResults(void) {
	  returnCode = 0;
	  rowsReturned = 0;
	  sqlWarningOrError = NULL;
	  outValuesFormat = 0;
	  outputDataValue._length = 0;
	  outputDataValue._buffer = NULL;
   }

// Save results of the prefetch
   CEE_status sts;
   BYTE *sqlWarningOrError;
   IDL_long returnCode;
   IDL_long rowsReturned;
   IDL_long outValuesFormat;	
   SQL_DataValue_def outputDataValue;
   int m_rbuffer_length;
   unsigned char* m_rbuffer;
   BOOL m_InitializationComplete;

private:
   Async_Mutex       m_mutex;
   Async_Condition   m_conditionVar;
   SRVR_CALL_CONTEXT m_SrvrCallContext;
   pthread_t		 m_thread;  
   pthread_attr_t	 m_thread_attr; 
   CConnect         *m_pConnection;
   CStmt            *m_pStatement;
   PREFETCH_STATE    m_State;
};

class CConnect : public CHandle {

public:
	CEE_handle_def			m_MonitorCallProxy;
	
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
	int initStream(short streams);
	int destroyStream();
	IDL_long initHashValues(BYTE *&WarningOrError, long wLength);
	void setVersion(const VERSION_LIST_def *versionList, int componentId);
	void getVersion(VERSION_def *versionList, int componentId);
	void setUserSid(const USER_SID_def *userSid);
	void setOutContext(const OUT_CONNECTION_CONTEXT_def *outContext);
	void setGetObjRefHdlOutput(const IDL_char *srvrObjRef,
		DIALOGUE_ID_def dialogueId,
		const IDL_char *dataSource, 
		const USER_SID_def *userSid,
		const VERSION_LIST_def *versionList,
		const IDL_unsigned_long isoMapping,
		const IDL_long srvrNodeId,
		const IDL_long srvrProcessId,
		const IDL_long_long timestamp);
	inline void setRetSrvrObjRef(const IDL_OBJECT_def objref) { strcpy(m_RetSQLSvc_ObjRef, objref); };
	long sendCDInfo(long exception_nr);
	long sendStopServer(odbcas_ASSvc_StopSrvr_exc_ *stopSrvrException);
	SQLRETURN initialize();
	void resetGetObjRefHdlOutput();
		
	inline DWORD getErrorMsgLang() { return m_DSValue.m_DSErrorMsgLang; };
	inline DWORD getDataLang() { return m_DSValue.m_DSDataLang; };
	inline char *getCharSet() { return m_DSValue.m_DSCharSet; };
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
	inline DWORD getTranslateOption() { return m_TranslateOption; };
	inline DWORD getEnforceISO88591() { return m_EnforceISO88591; };
	DWORD getSqlCharSet(long inSqlCharSet);
	inline int getCaseSensitivity() { return (m_MetadataId? CASE_YES:CASE_NO); };
	DWORD defaultLocale();
	inline ICUConverter* getICUConverter() { return m_ICUConv;};
	inline BOOL	getByteSwap() { 
		if(m_srvrTCPIPSystem->swap() == SWAP_YES)
			return TRUE;
		else
			return FALSE;
	};
	void deleteValueList();
	inline BOOL getSelectRowsets() { return m_SelectRowsets; };
	inline BOOL rowsetSupported(){return m_SQLRowsetSupported;}
	inline BOOL getFlushFetchData() { return m_FlushFetchData; };
	inline CEnv *getEnvHandle(){return m_EnvHandle;}
	inline BOOL rowsetErrorRecovery(){return m_RowsetErrorRecovery;};

	inline char* getTcpProcessName(){ return m_DSValue.m_DSTcpProcessName; }
        inline char* getASProcessName(){ char* st=strchr(m_ASSvc_ObjRef,':');
	         while (*(++st) == ' ' || *st == '\t'); return st;};
	inline SQLUINTEGER getModeLoader() { return m_ModeLoader; }
	inline SQLUINTEGER getStreamDelayedError() { return m_StreamDelayedError; };
	inline void        setStreamDelayedError(SQLUINTEGER delay) { m_StreamDelayedError=delay; };
	inline SQLUINTEGER getStartNode() { return m_StartNode; }
	inline SQLUINTEGER getEndNode() { return m_EndNode; }
	inline SQLUINTEGER getStreamsPerSeg() { return m_StreamsPerSeg; }
	void restoreDefaultDescriptor(CDesc* desc);

public:
	// Collection of Stmt Handles in this Connection
	CHANDLECOLLECT				m_StmtCollect;
	// Collection of Desc Handles in this Connection
	CHANDLECOLLECT				m_DescCollect;

	CTCPIPUnixDrvr*		        m_asTCPIPSystem;
	CTCPIPUnixDrvr*             m_srvrTCPIPSystem;

	preFetchThread              *m_preFetchThread;  // prefetch thread for double buffering SQLFetch

#ifdef ASYNCIO
	AsyncIO_Thread				*m_async_IOthread; // Async IO thread object
#endif /* ASYNCIO */

	int                         userStreams;
	int                     	maxUserStreams;
	master_t					masterStream;
	bucket_t*					m_srvrTCPIPSystemStream;
	hashinfo_t					m_hashInfo;
	int						    m_SugRowsetSize;

	bool                        initHashValuesComplete;

	TRANSPORT_TYPE				m_TransportType;
	void setObjRefHdlStream(const IDL_char *srvrObjRef, 
							short streamNumber,
							const IDL_short srvrNodeId,
							const IDL_short srvrProcessId,
							const IDL_long_long timestamp);
	void copyObjRefHdl(IDL_char *srvrObjRef);
	void copyObjRefHdlStream(IDL_char *srvrObjRef, short streamNumber);
	inline SRVR_CALL_CONTEXT* getSrvrCallContext(){ return &m_SrvrCallContext;}
	inline void setSrvrCallContext(SRVR_CALL_CONTEXT *SrvrCallContext){ m_SrvrCallContext = *SrvrCallContext;}

#ifdef ASYNCIO
	Async_Mutex					*m_CSTransmision; // mutex to prevent multiple threads from using the connection at the same time
#endif
	bool 						m_IgnoreCancel; // If SQL_ATTR_IGNORE_CANCEL is set at the server DSN, then SQLCancel will issue a sendStopSrvr request

	SecPwd*	  m_SecPwd;
	ProcInfo  m_SecInfo;
        char      m_ClusterName[MAX_SQL_IDENTIFIER_LEN+1]; // seaquest cluster name
	
	
	inline void setRetryEncryption(){ m_RetryEncryption = true; };
	void setSecurityError(int error, char* sqlState, char* errorMsg);

private:
	BOOL getUserDesc(char *userNameNTS, char *AuthenticationNT, USER_DESC_def *userDesc);
	SQLRETURN SetConnectAttr(SQLINTEGER Attribute, SQLUINTEGER valueNum, SQLPOINTER ValueStr, bool bClearError = true);
	SQLRETURN DoEncryption(SecPwd* &SecPwd, ProcInfo SecInfo,USER_DESC_def &userDesc,char *AuthenticationNTS,CONNECTION_CONTEXT_def &inContext);
	short getTransactionNumber(void);
	//10-080313-1379	
	int getUTF8CharLength(const char *inputChar, const int inputLength, const int maxLength);

	SRVR_CALL_CONTEXT			m_SrvrCallContext;
	//pthread_mutex_t				m_CSTransmision; // mutex to prevent multiple threads from using the connection at the same time
	HANDLE						m_ConnectEvent;
	IDL_OBJECT_def				m_RetSQLSvc_ObjRef;
	IDL_OBJECT_def				m_ASSvc_ObjRef;
	IDL_OBJECT_def				m_SQLSvc_ObjRef;
	IDL_OBJECT_def*				m_SQLSvc_ObjRefStream;
	DIALOGUE_ID_def				m_DialogueId;
	DIALOGUE_ID_def*				m_DialogueIdStream;
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
	inline void					clearUserName() { memset(m_UserName,0,sizeof(m_UserName));memset(m_UserSid,0,sizeof(m_UserSid));}
	
	CDataSource					m_DSValue;
	CEnv						*m_EnvHandle;
	
	// If rowsets supported
	BOOL						m_SQLRowsetSupported;
	BOOL						m_RowsetErrorRecovery;

	//  To support Dynamically change Fetch Buffer Size to ZERO for PUB/SUB and update/delete where current of CURSOR
	long						m_FetchBufferSize;		// in Bytes

	// Translate DLL 
	HMODULE						m_TranslateLibHandle;
//	FPSQLDriverToDataSource		m_FPSQLDriverToDataSource;
//	FPSQLDataSourceToDriver		m_FPSQLDataSourceToDriver;

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
	DWORD			m_TxnIsolation;
	BOOL			m_ConnectionDead;
	BOOL			m_SelectRowsets;
	BOOL			m_FlushFetchData; //  For query driver which cannot handle huge data being returned.
	SQLUINTEGER		m_ModeLoader;
	SQLUINTEGER		m_StreamDelayedError; // if this bit is set in the version buildId, we will get the status array for the current
	                                      // rowset after the next Execute2 
	SQLUINTEGER		m_StartNode;	// starting node number
        SQLUINTEGER		m_EndNode;	// Ending node number
	SQLUINTEGER		m_StreamsPerSeg; //how many streams per segment

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

	//wms_mapping
	char			m_QueryID_SessionName[SQL_MAX_SESSIONNAME_LEN]; 
	char			m_applName[SQL_MAX_APPLNAME_LEN + 1]; 

	// Charset
	BOOL			m_EnforceISO88591;
	BOOL			m_StandardConfig;
	
	SQLUINTEGER		m_FetchAhead;

	// Security
	char			m_CertificateDir[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_CertificateFile[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_CertificateFileActive[MAX_SQL_IDENTIFIER_LEN+1];
	SQLUINTEGER		m_SecurityMode;
	bool			m_RetryEncryption;
	SQLUINTEGER	    m_IOCompression;
       int             m_IOCompressionlimits;
	void reset(bool clearE=true);
	friend class	CStmt;
	friend class	CDesc;
};
extern "C"
BOOL GetTranslationData( char* ID, void*& SQLDriverToDataSource, void*&  SQLDataSourceToDriver);


#endif
