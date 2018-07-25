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
#ifndef CSTMT_H
#define CSTMT_H

#include <vector>
using std::vector;
using namespace std;
#include "CHandle.h"
#include "CDesc.h"
#include "CConnect.h"
#include "DrvrSrvr.h"
#include "DrvrNet.h"
#include "sqlcli.h"

typedef vector<SQLULEN> DRVR_SQL_ROWID_MAP;  //the sqlRowId is implicitely the index of the vector
typedef struct SPJ_RS_DATA
{
	int		spjStmtHandle;								// unused for now
	char	spjStmtLabelName[MAX_STMT_LABEL_LEN+1];
	SQLItemDescList_def spjOutputItemDesc;				// Output Descriptor
} SPJ_RS_DATA;

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

   preFetchThread() {
      initializeResults();
	  m_rbuffer_length = 0;
	  m_rbuffer = NULL;
	  m_Thread = NULL;
	  m_State = PREFETCH_STATE_WRK_UNASSIGNED;
   }

   ~preFetchThread() {
      if(m_rbuffer != NULL)
	     delete[] m_rbuffer;
   }

   void initializeResults(void) {
	  returnCode = 0;
	  rowsReturned = 0;
	  sqlWarningOrError = NULL;
	  outValuesFormat = 0;
	  outputDataValue._length = 0;
	  outputDataValue._buffer = NULL;
   }

   HANDLE m_Thread;  // preFetch thread handle
   SRVR_CALL_CONTEXT m_SrvrCallContext; // server call context for the preFetch thread
   unsigned char* m_rbuffer; // after a fetch, we'll need a buffer to copy over everything locally
   int m_rbuffer_length; 

   // preFetched results
   BYTE *sqlWarningOrError;
   IDL_long returnCode;
   IDL_long rowsReturned;
   IDL_long outValuesFormat;	
   SQL_DataValue_def outputDataValue;
   PREFETCH_STATE m_State;

   void r_allocate( int size) {
      if(size > m_rbuffer_length)
	  {
	     if(m_rbuffer != NULL)
	        delete[] m_rbuffer;
		 m_rbuffer = new unsigned char[size];
		 m_rbuffer_length = size;
	  }
   }

   void setSrvrCallContext(SRVR_CALL_CONTEXT *srvrCallContext) {
      memcpy(&m_SrvrCallContext,srvrCallContext,sizeof(SRVR_CALL_CONTEXT));
   }

   void copyOutBuffer(SQL_DataValue_def &output) {
      r_allocate(output._length);
	  memcpy(m_rbuffer,output._buffer,output._length);
	  outputDataValue._length = output._length;
	  outputDataValue._buffer = m_rbuffer;
   }
};


class CStmt : public CHandle {

public:
	CStmt(SQLHANDLE InputHandle);
	~CStmt();
	preFetchThread m_preFetchThread;
	SQLRETURN initialize();
	
	SQLRETURN Close(SQLUSMALLINT Option);
	SQLRETURN SetStmtAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength);
	SQLRETURN GetStmtAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
					SQLINTEGER *StringLengthPtr);
	SQLRETURN BindCol(SQLUSMALLINT ColumnNumber,
			SQLSMALLINT TargetType,
			SQLPOINTER	TargetValuePtr,
			SQLLEN		BufferLength,
			SQLLEN		*StrLen_or_IndPtr);
	SQLRETURN BindParameter(SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN		ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN		BufferLength,
			SQLLEN		*StrLen_or_Ind);
	SQLRETURN SendSQLCommand(BOOL SkipProcess, SQLCHAR *StatementText,
			SQLINTEGER TextLength);
	SQLRETURN Prepare(SQLCHAR *StatementText,
			SQLINTEGER TextLength);
	SQLRETURN ExecDirect(SQLCHAR *StatementText,
			SQLINTEGER TextLength);
	SQLRETURN SendExecute(BOOL SkipProcess);
	SQLRETURN Execute();
	SQLRETURN getDescRec(short odbcAPI,
			SQLUSMALLINT ColumnNumber, 
			SQLWCHAR *ColumnName,
			SQLSMALLINT BufferLength, 
			SQLSMALLINT *NameLengthPtr,
		    SQLSMALLINT *DataTypePtr, 
			SQLULEN *ColumnSizePtr,
			SQLSMALLINT *DecimalDigitsPtr,
			SQLSMALLINT *NullablePtr);
	SQLRETURN getDescSize(short odbcAPI, SQLSMALLINT *ColumnCountPtr);
	SQLRETURN FreeStmt(short odbcAPI, SQLUSMALLINT Option);
	SQLRETURN SendGetSQLCatalogs(short odbcAPI,
			BOOL SkipProcess,
			SQLWCHAR *CatalogNameW, 
			SQLSMALLINT NameLength1,
			SQLWCHAR *SchemaNameW, 
			SQLSMALLINT NameLength2,
			SQLWCHAR *TableNameW, 
			SQLSMALLINT NameLength3,
			SQLWCHAR *ColumnNameW = NULL, 
			SQLSMALLINT NameLength4 = SQL_NTS,
			SQLWCHAR *TableTypeW = NULL, 
			SQLSMALLINT NameLength5 = SQL_NTS,
			SQLUSMALLINT IdentifierType = 0,
		   	SQLUSMALLINT Scope = 0,
			SQLUSMALLINT Nullable = 0,
			SQLSMALLINT SqlType = 0,
			SQLUSMALLINT Unique = 0,
			SQLUSMALLINT Reserved = 0,
            SQLWCHAR *FKCatalogNameW = NULL, 
		    SQLSMALLINT NameLength6 = SQL_NTS,
            SQLWCHAR *FKSchemaNameW = NULL, 
		    SQLSMALLINT NameLength7 = SQL_NTS,
            SQLWCHAR *FKTableNameW = NULL, 
		    SQLSMALLINT NameLength8 = SQL_NTS);
	SQLRETURN GetSQLCatalogs(short odbcAPI, 
			SQLWCHAR *CatalogNameW, 
			SQLSMALLINT NameLength1,
			SQLWCHAR *SchemaNameW, 
			SQLSMALLINT NameLength2,
			SQLWCHAR *TableNameW, 
			SQLSMALLINT NameLength3,
			SQLWCHAR *ColumnNameW = NULL, 
			SQLSMALLINT NameLength4 = SQL_NTS,
			SQLWCHAR *TableType = NULL, 
			SQLSMALLINT NameLength5 = SQL_NTS,
			SQLUSMALLINT IdentifierType = 0,
		   	SQLUSMALLINT Scope = 0,
			SQLUSMALLINT Nullable = 0,
			SQLSMALLINT SqlType = 0,
			SQLUSMALLINT Unique = 0,
			SQLUSMALLINT Reserved = 0,
            SQLWCHAR *FKCatalogNameW = NULL, 
		    SQLSMALLINT NameLength6 = SQL_NTS,
            SQLWCHAR *FKSchemaNameW = NULL, 
		    SQLSMALLINT NameLength7 = SQL_NTS,
            SQLWCHAR *FKTableNameW = NULL, 
		    SQLSMALLINT NameLength8 = SQL_NTS);
	SQLRETURN GetCursorName(SQLWCHAR *CursorNameW, 
			SQLSMALLINT BufferLength,
			SQLSMALLINT *NameLengthPtr);
	SQLRETURN SetCursorName(SQLWCHAR *CursorNameW, 
			SQLSMALLINT NameLength);
	SQLRETURN Cancel();	
	SQLRETURN ColAttribute(SQLUSMALLINT ColumnNumber, 
			SQLUSMALLINT FieldIdentifier,
			SQLPOINTER CharacterAttributePtr, 
			SQLSMALLINT BufferLength,
			SQLSMALLINT *StringLengthPtr, 
			SQLPOINTER NumericAttributePtr);
	SQLRETURN SendParamData(BOOL SkipProcess, SQLPOINTER *ValuePtrPtr);
	SQLRETURN ParamData(SQLPOINTER *ValuePtrPtr);
	SQLRETURN PutData(SQLPOINTER DataPtr, 
			SQLLEN StrLen_or_Ind);
	SQLRETURN SendFetch(BOOL SkipProcess);
	SQLRETURN Fetch();
	SQLRETURN SendExtendedFetch(BOOL SkipProcess,
			SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray);
	SQLRETURN ExtendedFetch(SQLUSMALLINT FetchOrientation,
			SQLLEN FetchOffset,
			SQLULEN* RowCountPtr,
			SQLUSMALLINT* RowStatusArray);
	SQLRETURN FetchScroll(SQLUSMALLINT FetchOrientation,
			SQLINTEGER FetchOffset);
	SQLRETURN GetData(SQLUSMALLINT ColumnNumber, 
			SQLSMALLINT TargetType,
			SQLPOINTER TargetValuePtr, 
			SQLLEN BufferLength,
			SQLLEN *StrLen_or_IndPtr);
	SQLRETURN SetPos(SQLUSMALLINT	RowNumber,
			SQLUSMALLINT    Operation,
			SQLUSMALLINT    LockType);
    SQLRETURN CStmt::SendExtractLob(IDL_short extractType,
		IDL_string lobHandle,
		IDL_long lobHandleLen,
		IDL_long &extractLen,
		BYTE *& extractData);

    SQLRETURN ExtractLob(IDL_short extractType,
        IDL_string lobHandle,
        IDL_long   lobHandleLen,
        IDL_long &extractLen,
        BYTE * &extractData);
    SQLRETURN SendUpdateLob(IDL_long updateType,
        IDL_string lobHandle,
        IDL_long   lobHandleLen,
        IDL_long_long totalLength,
        IDL_long_long offset,
        IDL_long_long pos,
        IDL_long_long length,
        BYTE *        data);
    SQLRETURN UpdateLob(IDL_long updateType,
        IDL_string lobHandle,
        IDL_long   lobHandleLen,
        IDL_long_long totalLength,
        IDL_long_long offset,
        IDL_long_long pos,
        IDL_long_long length,
        BYTE *        data);

	SQLRETURN setDescRec(const SQLItemDescList_def *IPDDescList, const SQLItemDescList_def *IRDDescList);
	inline BOOL getHeartBeatEnable() {return m_ConnectHandle->getHeartBeatEnable();};
	inline DWORD getErrorMsgLang() { return m_ConnectHandle->getErrorMsgLang(); };
	inline DWORD getDataLang() { return m_ConnectHandle->getDataLang(); };
	inline BOOL	getByteSwap() { return m_ConnectHandle->getByteSwap(); };
	inline BOOL isNskVersion() { return m_ConnectHandle->getByteSwap(); };
	inline char *getSrvrIdentity() { return m_ConnectHandle->getSrvrIdentity(); };
	inline long sendCDInfo(long exception_nr) { return m_ConnectHandle->sendCDInfo(exception_nr); };
	inline long sendStopServer() { return m_ConnectHandle->sendStopServer(); };
	inline char *getCurrentCatalog() { return m_ConnectHandle->getCurrentCatalog(); };
	inline char *getCurrentSchema() { return m_ConnectHandle->getCurrentSchema(); };
	inline SQLINTEGER getODBCAppVersion() { return m_ConnectHandle->getODBCAppVersion();};
	void setFetchOutput(long rowsFetched, const SQLValueList_def *outputValueList);
	BOOL setFetchOutputPerf(long rowsFetched, SQL_DataValue_def*& outputDataValue);
	BOOL setFetchOutputPerf(SQL_DataValue_def*& outputDataValue, long rowsFetched);
	inline short getStmtState() { return m_StmtState; };
	inline short getStmtType() { return m_StmtType; };
	inline BOOL getFetchCatalog(){return m_bFetchCatalog;}
	inline void setFetchCatalog(BOOL bFetchCatalog){m_bFetchCatalog = bFetchCatalog;}
	inline void deleteValueList()
	{
		clearFetchDataValue();
	}
	inline void revertStmtState()
	{
		if (! m_StmtPrepared)
			m_StmtState = STMT_ALLOCATED;
		else
		{
			if (m_StmtState == STMT_EXECUTED_NO_RESULT)
				m_StmtState = STMT_PREPARED_NO_RESULT;
			else
				m_StmtState = STMT_PREPARED;
		}
		setRowCount(-1);
		m_ThreadStatus = SQL_SUCCESS;
		m_RowsFetched = -1;
		m_CurrentRowFetched = 0;
		m_CurrentRowInRowset = 0;
		m_RowsetSize = 0;
		m_ResultsetRowsFetched = 0;
		m_RowNumber = 0;
		m_RowStatusArray = NULL;
		m_CurrentFetchType = 0;
		m_AsyncCanceled = FALSE;
	};
	inline void setStmtLabel(const char *stmtLabel) { strcpy(m_StmtLabel, stmtLabel); } ;
	inline CConnect* getConnectHandle() {return m_ConnectHandle;};
	inline void setRowCount(IDL_long_long RowCount) {m_RowCount = RowCount;};
	inline IDL_long_long getRowCount() {return m_RowCount;};
	long getImpDescCount();
	BOOL getImpSQLData(long colnumber,
		SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
		SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLOctetLength, 
		SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned);

	BOOL getImpBulkSQLData(long colnumber,
		SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
		SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLOctetLength, 
		SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned);

	long getImpRowDescCount();
	BOOL getImpRowData(long colnumber,
		SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
		SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLMaxLength, 
		SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned,
		SQLSMALLINT& SQLNullable);
	BOOL getImpRowData(long colnumber, SQLSMALLINT& SQLDataType, 
		SQLINTEGER& SQLMaxLength, SQLSMALLINT& SQLNullable);

	long getImpParamDescCount();
	BOOL getImpParamData(long colnumber,
		SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
		SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLMaxLength, 
		SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned,
		SQLSMALLINT& SQLNullable);

	BOOL checkInputParam(string m_SqlString);
	void trimSqlString(unsigned char* inpString, unsigned char* outString, unsigned long& sqlStringLen, BOOL padDelimitedIdentifiers = FALSE);
	inline SQLULEN getMaxLength(){ return m_MaxLength;}
	inline BOOL getSelectRowsets() { return m_SelectRowsets; };

	void setNumberOfElements(unsigned long count);
	unsigned long getNumberOfElements(void);
	void setNumberOfRows(unsigned long count);
	unsigned long getNumberOfRows(void);
	void setRowAddress( unsigned long row, BYTE* address);
	BYTE* getRowAddress( unsigned long row);
	void clearFetchDataValue(void);
	void clearInputValueList(void);
	inline void clearSqlDrvrRowIdMap(){m_RowIdMap.clear();}
	inline SRVR_CALL_CONTEXT* getSrvrCallContext(){ return &m_SrvrCallContext;}
	inline BOOL rowsetSupported(){return m_ConnectHandle->rowsetSupported();}
	inline void insertIntoDrvrSqlRowIdMap( IDL_long drvrRow){m_RowIdMap.push_back(drvrRow);}
	inline bool rowsetWasFiltered(){return (!(m_RowIdMap.empty()));}
	SQLRETURN mapSqlRowIdsToDrvrRowIds(BYTE *&WarningOrError); 

	SQLRETURN SendGetSQLCatalogsArgsHlpr(bool WCharArg, SQLCHAR* arg, int argLen, char * dest, int destLen, SQLUINTEGER metaDataId);
	inline DWORD getTranslateOption() { return m_ConnectHandle->getTranslateOption(); };
	
	// For Stored Proceudre Call support
	SQLRETURN setExecuteCallOutputs();

	//  SQL/MX returns query types and server returns handle
    //  Not to block statements of type SELECT
	inline void setStmtQueryType(long StmtQueryType) 
	{
		m_StmtQueryType = StmtQueryType;
        if ((StmtQueryType == SQL_SELECT_UNIQUE) ||
		    (StmtQueryType == SQL_SELECT_NON_UNIQUE) ||
		    (StmtQueryType == 10000)) // bulk fetch
		    m_StmtType = TYPE_SELECT;
	};

	inline void setStmtHandle(long StmtHandle) {m_StmtHandle = StmtHandle;};
	inline void setAPIDecision(BOOL APIDecision) {m_APIDecision = APIDecision;};
	SQLRETURN setStmtData(BYTE *&inputParams, BYTE *&outputColumns);
	inline IDL_long getStmtQueryType() { return m_StmtQueryType; };
	inline IDL_long getStmtHandle() { return m_StmtHandle; };
	inline BOOL getAPIDecision() {return m_APIDecision;};
	void getStmtData(BYTE *&inputParams, BYTE *&outputColumns);

	//TCPIP transport
	CTCPIPSystemDrvr* getSrvrTCPIPSystem();
	void setRowsetArrayStatus(const ERROR_DESC_LIST_def *sqlWarning, long rowsAffected);
	void setRowsetArrayStatus(BYTE *&WarningOrError, long rowsAffected);
	inline BOOL rowsetErrorRecovery(){return m_ConnectHandle->rowsetErrorRecovery();};
	void setColumnOffset(short columnNumber, long offset);
	long getColumnOffset(short columnNumber);
	bool isInfoStats(char *inpString);

	inline long *getColumnIndexes(){return m_ColumnIndexes;};
	inline char **getSwapInfo(){return m_SwapInfo;};
public:
	SRVR_CALL_CONTEXT	m_SrvrCallContext;
	bool				m_isClosed;
	SQL_DataValue_def	m_outputDataValue;
	DWORD				m_intStmtType;

	// SPJ result set support
    IDL_long			m_spjNumResultSets; // Number of result sets
	SPJ_RS_DATA			*m_spjResultSets;   // SPJ result sets
	SQLSMALLINT			m_spjCurrentResultSetIndex; // Index of the current active result set
	// For SPJs we need to save the Input and Output param descriptors, in case somebody does a prepare once and Execute multiple times
	SQLItemDescList_def m_spjInputParamDesc;	// SPJ input param descriptors
	SQLItemDescList_def m_spjOutputParamDesc;	// SPJ output param descriptors
	bool m_CancelCalled;
	SQLUINTEGER		m_Concurrency;				// 2.0
	CDesc			*m_AppRowDesc;				// 3.0
	SQLUINTEGER		m_AsyncEnable;				// 1.0

private:
	void InitInputValueList();
	void InitParamColumnList();

	HANDLE				m_AsyncThread;
	DWORD				m_ThreadStatus;
	BOOL				m_WarningSetBeforeAsync; // Was a warning set before trying to asynchronously execute a statement ?
	HANDLE				m_StmtEvent;
	unsigned int		m_ThreadAddr;
	HANDLE				m_SyncThread;

	DRVR_SQL_ROWID_MAP	m_RowIdMap; 
	SQLValueList_def	m_InputValueList;
	// The following variables are used SQL_DATA_AT_EXEC Processing
	SQLSMALLINT			m_CurrentParam;
	SQLUINTEGER			m_CurrentRow;
	short				m_CurrentParamStatus;

	SQLValue_def		m_DataAtExecData;
	unsigned long		m_DataAtExecDataBufferSize;
	BYTE				*m_ParamBuffer;			// Total Param Buffer 

	SQLULEN				m_RowsFetched;			// Number of Rows fetched in the Current RowSet
	SQLULEN				m_CurrentRowFetched;	// The application current row in the RowSet
	SQLULEN				m_ResultsetRowsFetched; // Number of Rows fetched in the ResultSet
	SQLULEN				m_CurrentRowInRowset;
	SQLULEN				m_RowsetSize;
	BOOL				m_bFetchCatalog;
	SQLSMALLINT			m_NumResultCols;
	SQLSMALLINT			m_NumParams;
	
	SQLFetchDataValue	m_FetchDataValue;

	//	Check DrvrGlobal.h for possible values in m_StmtState
	//	May not maintain all possible states
	short				m_StmtState;			 
	BOOL				m_StmtPrepared;			// If SQLPrepare is issued on this statement
	BOOL				m_AsyncCanceled;
	BOOL				m_SelectRowsets;
	BOOL				m_DoCompression;

	char			m_StmtLabel[MAX_STMT_LABEL_LEN+1];
	char			m_StmtLabelOrg[MAX_STMT_LABEL_LEN+1];	//  this is introduced since catalog API replaces the statement
															// label returned by server. we should revert it back to original label.
	char			m_StmtName[MAX_STMT_NAME_LEN+1];
	char			m_CursorName[MAX_CURSOR_NAME_LEN+1];
	string			m_SqlString;
	short			m_StmtType;					// Type of Stmt - TYPE_SELECT,TYPE_EXPLAIN,TYPE_UNKNOWN

	// returns query types and server returns handle
	IDL_long		m_StmtQueryType;
	IDL_long		m_StmtHandle;
	BOOL			m_APIDecision;
	BYTE			*m_InputParams;
	BYTE			*m_OutputColumns;

	CConnect		*m_ConnectHandle;
	CDesc			*m_ARDDesc;
	CDesc			*m_IRDDesc;
	CDesc			*m_APDDesc;
	CDesc			*m_IPDDesc;

	//	Statement Attributes
	CDesc			*m_AppParamDesc;			// 3.0
//	CDesc			*m_AppRowDesc;				// 3.0
//	SQLUINTEGER		m_Concurrency;				// 2.0
	SQLUINTEGER		m_CursorHoldable;			// for SAP support
	SQLUINTEGER		m_CursorScrollable;			// 3.0
	SQLUINTEGER		m_CursorSensitivity;		// 3.0
	SQLUINTEGER		m_CursorType;				// 2.0
	SQLUINTEGER		m_EnableAutoIPD;			// 3.0
	SQLPOINTER		m_FetchBookmarkPtr;			// 3.0
	CDesc			*m_ImpParamDesc;			// 3.0
	CDesc			*m_ImpRowDesc;				// 3.0
	SQLULEN 		m_KeysetSize;				// 2.0 // expanded for 64 bit
	SQLULEN 		m_MaxLength;				// 1.0 // expanded for 64 bit
	SQLULEN 		m_MaxRows;					// 1.0 // expanded for 64 bit
	SQLUINTEGER		m_MetadataId;				// 3.0
	SQLUINTEGER		m_Noscan;					// 1.0	
	SQLUINTEGER		m_QueryTimeout;				// 1.0
	SQLUINTEGER		m_RetrieveData;				// 2.0
	SQLULEN 		m_RowNumber;				// 2.0		//1 based Current Row in the Result Set // expanded for 64 bit
	SQLUINTEGER		m_SimulateCursor;			// 2.0
	SQLUINTEGER		m_UseBookmarks;				// 2.0
	char			m_CatalogName[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_SchemaName[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_TableName[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_ColumnName[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_TableType[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_FKCatalogName[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_FKSchemaName[MAX_SQL_IDENTIFIER_LEN+1];
	char			m_FKTableName[MAX_SQL_IDENTIFIER_LEN+1];

	// For SQLExtendedFetch
	SQLUSMALLINT*	m_RowStatusArray;
	SQLSMALLINT		m_CurrentFetchType;

	// For Row Count
	IDL_long_long	m_RowCount;

	//  To support Dynamically change Fetch Buffer Size to ZERO for PUB/SUB and update/delete where current of CURSOR
	long			m_FetchBufferSize;		// in Bytes
	
	long*     m_ColumnIndexes;
	
	// While fetching data, a SQLBindCol(), SQLFetch(), SQLGetData() sequence can 
	// cause a double swap. This array prevents it.
	char**	  m_SwapInfo;
	long	  m_SwapInfo_NumRows;
	
	friend class	CConnect;
};

#endif
