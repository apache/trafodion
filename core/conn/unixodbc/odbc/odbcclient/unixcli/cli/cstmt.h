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

#include <string>
#include <vector>
#include <list>
using std::string;
using std::list;
using namespace std;

#include <list>
#include <vector>
#include "chandle.h"
#include "cdesc.h"
#include "cconnect.h"
#include "DrvrSrvr.h"
#include "drvrnet.h"
#include "sqlcli.h"

#include <asyncIO.h>
#include "odbcas_cl.h"

typedef list<char*> VALUES;
typedef vector<SQLULEN> DRVR_SQL_ROWID_MAP;  //the sqlRowId is implicitely the index of the vector

//#ifdef ASYNCIO

typedef enum _Stmt_Execution_State
{
	STMT_EXECUTION_NONE = 0, 
	STMT_EXECUTION_EXECUTING, 
	STMT_EXECUTION_DONE

} StmtExecState;

//#endif /* ASYNCIO */


typedef enum _WMTOKEN
{

	WM_INIT = 0,
	WM_BT,
	WM_ET,
	WM_INBT,
	WM_RETURN_SUCCESS,
	WM_OTHER
} WMTOKEN;

WMTOKEN ParseStmt(CStmt* pStmt, SQLCHAR *StatementText); 

typedef struct SPJ_RS_DATA
{
	int		spjStmtHandle;						   
	char	spjStmtLabelName[MAX_STMT_LABEL_LEN+1];
	SQLItemDescList_def spjOutputItemDesc;				// Output Descriptor
} SPJ_RS_DATA;


class CStmt : public CHandle {

public:
	CStmt(SQLHANDLE InputHandle);
	~CStmt();
	SQLRETURN initialize();
	
	SQLRETURN Close(SQLUSMALLINT Option);
	SQLRETURN SetStmtAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength);
	SQLRETURN GetStmtAttr(SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER BufferLength,
					SQLINTEGER *StringLengthPtr);
	SQLRETURN BindCol(SQLUSMALLINT ColumnNumber,
			SQLSMALLINT TargetType,
			SQLPOINTER	TargetValuePtr,
			SQLINTEGER	BufferLength,
			SQLLEN *StrLen_or_IndPtr);
	SQLRETURN BindParameter(SQLUSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, 
			SQLULEN 	ColumnSize,
			SQLSMALLINT DecimalDigits,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN   	BufferLength,
			SQLLEN      *StrLen_or_Ind);
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
			SQLCHAR *ColumnName,
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
			SQLCHAR *CatalogName, 
			SQLSMALLINT NameLength1,
			SQLCHAR *SchemaName, 
			SQLSMALLINT NameLength2,
			SQLCHAR *TableName, 
			SQLSMALLINT NameLength3,
			SQLCHAR *ColumnName = NULL, 
			SQLSMALLINT NameLength4 = SQL_NTS,
			SQLCHAR *TableType = NULL, 
			SQLSMALLINT NameLength5 = SQL_NTS,
			SQLUSMALLINT IdentifierType = 0,
		   	SQLUSMALLINT Scope = 0,
			SQLUSMALLINT Nullable = 0,
			SQLSMALLINT SqlType = 0,
			SQLUSMALLINT Unique = 0,
			SQLUSMALLINT Reserved = 0,
            SQLCHAR *FKCatalogName = NULL, 
		    SQLSMALLINT NameLength6 = SQL_NTS,
            SQLCHAR *FKSchemaName = NULL, 
		    SQLSMALLINT NameLength7 = SQL_NTS,
            SQLCHAR *FKTableName = NULL, 
		    SQLSMALLINT NameLength8 = SQL_NTS);
	SQLRETURN GetSQLCatalogs(short odbcAPI, 
			SQLCHAR *CatalogName, 
			SQLSMALLINT NameLength1,
			SQLCHAR *SchemaName, 
			SQLSMALLINT NameLength2,
			SQLCHAR *TableName, 
			SQLSMALLINT NameLength3,
			SQLCHAR *ColumnName = NULL, 
			SQLSMALLINT NameLength4 = SQL_NTS,
			SQLCHAR *TableType = NULL, 
			SQLSMALLINT NameLength5 = SQL_NTS,
			SQLUSMALLINT IdentifierType = 0,
		   	SQLUSMALLINT Scope = 0,
			SQLUSMALLINT Nullable = 0,
			SQLSMALLINT SqlType = 0,
			SQLUSMALLINT Unique = 0,
			SQLUSMALLINT Reserved = 0,
            SQLCHAR *FKCatalogName = NULL, 
		    SQLSMALLINT NameLength6 = SQL_NTS,
            SQLCHAR *FKSchemaName = NULL, 
		    SQLSMALLINT NameLength7 = SQL_NTS,
            SQLCHAR *FKTableName = NULL, 
		    SQLSMALLINT NameLength8 = SQL_NTS);
	SQLRETURN GetCursorName(SQLCHAR *CursorName, 
			SQLSMALLINT BufferLength,
			SQLSMALLINT *NameLengthPtr);
	SQLRETURN SetCursorName(SQLCHAR *CursorName, 
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
	void clearError(); // override method in the base class CHandle

	SQLRETURN setDescRec(const SQLItemDescList_def *IPDDescList, const SQLItemDescList_def *IRDDescList);
	inline BOOL getHeartBeatEnable() {return m_ConnectHandle->getHeartBeatEnable();};
	inline DWORD getErrorMsgLang() { return m_ConnectHandle->getErrorMsgLang(); };
	inline DWORD getDataLang() { return m_ConnectHandle->getDataLang(); };
	inline BOOL	getByteSwap() { return m_ConnectHandle->getByteSwap(); };
	inline BOOL isNskVersion() { return m_ConnectHandle->getByteSwap(); };
	inline char *getSrvrIdentity() { return m_ConnectHandle->getSrvrIdentity(); };
	inline long sendCDInfo(long exception_nr) { return m_ConnectHandle->sendCDInfo(exception_nr); };
	inline long sendStopServer(odbcas_ASSvc_StopSrvr_exc_ *stopSrvrException) { return m_ConnectHandle->sendStopServer(stopSrvrException); };
	inline char *getCurrentCatalog() { return m_ConnectHandle->getCurrentCatalog(); };
	inline char *getCurrentSchema() { return m_ConnectHandle->getCurrentSchema(); };
	inline SQLINTEGER getODBCAppVersion() { return m_ConnectHandle->getODBCAppVersion();};
	void setFetchOutput(long rowsFetched, const SQLValueList_def *outputValueList, CEE_handle_def srvrProxy);
	BOOL setFetchOutputPerf(long rowsFetched, SQL_DataValue_def*& outputDataValue, CEE_handle_def srvrProxy);
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

	inline SQLUINTEGER getAsyncEnable() { return m_AsyncEnable; }

	inline void deleteOutputDataValue()
	{
		if (m_OutputDataValue != NULL)
			delete m_OutputDataValue;
		m_OutputDataValue = NULL;
	}
#ifdef VERSION3
	BOOL checkInputParam(std::string SqlString);       
#else
	BOOL checkInputParam(RWCString SqlString);
#endif
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
	void restoreDefaultDescriptor(CDesc* desc);
	inline BOOL rowsetSupported(){return m_ConnectHandle->rowsetSupported();}
	inline void insertIntoDrvrSqlRowIdMap( IDL_long drvrRow){m_RowIdMap.push_back(drvrRow);}
	inline bool rowsetWasFiltered(){return (!(m_RowIdMap.empty()));}
	SQLRETURN mapSqlRowIdsToDrvrRowIds(BYTE *&WarningOrError); 
	
	// For Stored Proceudre Call support
	SQLRETURN setExecuteCallOutputs();

	// for Caffeine SQL/MX returns query types and server returns handle
    // Vijay - Not to block statements of type SELECT
	inline void setStmtQueryType(long StmtQueryType) 
	{
	   m_StmtQueryType = StmtQueryType;
       if ((StmtQueryType == SQL_SELECT_UNIQUE) ||
		   (StmtQueryType == SQL_SELECT_NON_UNIQUE) ||
		   (StmtQueryType == 10000)) // bulk fetch
		    m_StmtType = TYPE_SELECT;
	};


	inline void setStmtHandle(IDL_long StmtHandle) {m_StmtHandle = StmtHandle;};
	inline void setAPIDecision(BOOL APIDecision) {m_APIDecision = APIDecision;};
	SQLRETURN setStmtData(BYTE *&inputParams, BYTE *&outputColumns);
	inline long getStmtQueryType() { return m_StmtQueryType; };
	inline IDL_long getStmtHandle() { return m_StmtHandle; };
	inline IDL_long getStmtHandleStream(short index) {return m_StmtHandleStream[index];};
	inline BOOL getAPIDecision() {return m_APIDecision;};
	void getStmtData(BYTE *&inputParams, BYTE *&outputColumns);

	void setRowsetArrayStatus(const ERROR_DESC_LIST_def *sqlWarning, long rowsAffected);
	void setRowsetArrayStatus(BYTE *&WarningOrError, long rowsAffected);

//TCPIP transport
#ifndef unixcli
	CTCPIPSystemDrvr* getSrvrTCPIPSystem();
#else
	CTCPIPUnixDrvr*   getSrvrTCPIPSystem();
#endif
	inline BOOL rowsetErrorRecovery(){return m_ConnectHandle->rowsetErrorRecovery();};
	inline void setDescArrayStatus(SQLUINTEGER value = SQL_ROW_IGNORE ) {
		SQLUINTEGER MaxRowNumber;
		m_AppParamDesc->GetDescField(0, SQL_DESC_ARRAY_SIZE, &MaxRowNumber, SQL_IS_UINTEGER, NULL);
		m_ImpParamDesc->setDescArrayStatus(MaxRowNumber, value);
	};

	void setColumnOffset(short columnNumber, long offset);
	long getColumnOffset(short columnNumber);
	bool isInfoStats(char *inpString);
	
	inline long *getColumnIndexes(){return m_ColumnIndexes;};
	inline char **getSwapInfo(){return m_SwapInfo;};
	inline CDesc *getImpParamDesc() { return m_ImpParamDesc; }
	inline CDesc *getAppParamDesc() { return m_AppParamDesc; }

	bool				m_isClosed;
	SQL_DataValue_def	m_outputDataValue;
	DWORD				m_intStmtType;
		
	WMTOKEN		m_token;
	bool		m_WMStmtPrepared;
	bool		m_BT;
	string		m_sqlTable;
	string		m_sqlStmt;
	int			m_valcnt;
	VALUES		m_values;
	vector<int>	m_vlen;

	// SPJ result set support
    SQLSMALLINT			m_spjNumResultSets; // Number of result sets
	SPJ_RS_DATA			*m_spjResultSets;   // SPJ result sets
	SQLSMALLINT			m_spjCurrentResultSetIndex; // Index of the current active result set
	// For SPJs we need to save the Input and Output param descriptors, in case somebody does a prepare once and Execute multiple times
	SQLItemDescList_def m_spjInputParamDesc;	// SPJ input param descriptors
	SQLItemDescList_def m_spjOutputParamDesc;	// SPJ output param descriptors
	SQLUINTEGER	m_CursorHoldable;
        RWRS_Descriptor	*m_RWRS_Descriptor;

#ifdef ASYNCIO
	// get/set the returnCode from the last Async Opertion on this statement handle
	inline int  getAsyncOperationReturnCode() { return m_AsyncOperationReturnCode; }
	inline void setAsyncOperationReturnCode(int returnCode) { m_AsyncOperationReturnCode = returnCode; }
#endif

	inline StmtExecState  getStmtExecState() { return m_StmtExecState; }
	inline void setStmtExecState(StmtExecState state) { m_StmtExecState = state; }
	inline void setAsyncCancelled(bool state) { m_AsyncCanceled = state; }
	inline bool getAsyncCancelled() { return m_AsyncCanceled; }
	SQLUINTEGER		m_Concurrency;				// 2.0
private:
	void InitInputValueList();

	void InitParamColumnList();

#ifdef ASYNCIO
	DWORD				m_AsyncOperationReturnCode;    // Return Code from the last Async Operation on this Statement Handle
	BOOL				m_WarningSetBeforeAsync;       // Was a warning set before trying to asynchronously execute a statement ?
#endif
	StmtExecState	    m_StmtExecState;               // One of STMT_EXECUTION_NONE, STMT_EXECUTION_EXECUTING, STMT_EXECUTION_DONE
	DWORD				m_ThreadStatus;
	BOOL				m_AsyncCanceled;
	HANDLE				m_StmtEvent;
	unsigned int		m_ThreadAddr;
	SRVR_CALL_CONTEXT	m_SrvrCallContext;

	DRVR_SQL_ROWID_MAP	m_RowIdMap; 
	SQLValueList_def	m_InputValueList;
	// The following variables are used SQL_DATA_AT_EXEC Processing
	SQLSMALLINT			m_CurrentParam;
	SQLULEN				m_CurrentRow;
	short				m_CurrentParamStatus;

	SQLValue_def		m_DataAtExecData;
	unsigned long		m_DataAtExecDataBufferSize;
	BYTE				*m_ParamBuffer;			// Total Param Buffer 

	SQLULEN				m_RowsFetched;			// Number of Rows fetched in the Current RowSet
	SQLULEN				m_CurrentRowFetched;	// The application current row in the RowSet
	SQLULEN				m_ResultsetRowsFetched; // Number of Rows fetched in the ResultSet
	SQLULEN				m_CurrentRowInRowset;
	SQLULEN 			m_RowsetSize;
	BOOL				m_bFetchCatalog;
//	SQLValueList_def	*m_OutputValueList;
//	SQL_DataValue_def	*m_OutputDataValue;
	SQLSMALLINT			m_NumResultCols;
	SQLSMALLINT			m_NumParams;

	SQLFetchDataValue	m_FetchDataValue;
	unsigned char*		m_OutputDataValue;
	
	//	Check DrvrGlobal.h for possible values in m_StmtState
	//	May not maintain all possible states
	short				m_StmtState;			 
	BOOL				m_StmtPrepared;			// If SQLPrepare is issued on this statement
	BOOL				m_SelectRowsets;

	char			m_StmtLabel[MAX_STMT_LABEL_LEN+1];
	char			m_StmtLabelOrg[MAX_STMT_LABEL_LEN+1];	// this is introduced since catalog API replaces the statement
															// label returned by server. we should revert it back to original label.
	char			m_StmtName[MAX_STMT_NAME_LEN+1];
	char			m_CursorName[MAX_CURSOR_NAME_LEN+1];
	std::string	 	m_SqlString;
	DWORD			m_StmtType;					// Type of Stmt - TYPE_SELECT,TYPE_EXPLAIN,TYPE_UNKNOWN

	//  for Caffeine SQL/MX returns query types and server returns handle
	IDL_long  		m_StmtQueryType;
	IDL_long 		m_StmtHandle;
	IDL_long  		m_StmtHandleStream[MAX_STREAMS];
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
	CDesc			*m_AppRowDesc;			// 3.0
	SQLUINTEGER		m_AsyncEnable;				// 1.0
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
	char			m_CatalogName[(MAX_SQL_IDENTIFIER_LEN+1)*4];
	char			m_SchemaName[(MAX_SQL_IDENTIFIER_LEN+1)*4];
	char			m_TableName[(MAX_SQL_IDENTIFIER_LEN+1)*4];
	char			m_ColumnName[(MAX_SQL_IDENTIFIER_LEN+1)*4];
	char			m_TableType[(MAX_SQL_IDENTIFIER_LEN+1)*4];
	char			m_FKCatalogName[(MAX_SQL_IDENTIFIER_LEN+1)*4];
	char			m_FKSchemaName[(MAX_SQL_IDENTIFIER_LEN+1)*4];
	char			m_FKTableName[(MAX_SQL_IDENTIFIER_LEN+1)*4];

	int				m_CatalogNameRTLen;
	int				m_SchemaNameRTLen;
	int				m_TableNameRTLen;
	int				m_ColumnNameRTLen;
	int				m_TableTypeRTLen;
	int				m_FKCatalogNameRTLen;
	int				m_FKSchemaNameRTLen;
	int				m_FKTableNameRTLen;

	// For SQLExtendedFetch
	SQLUSMALLINT*	m_RowStatusArray;
	SQLSMALLINT		m_CurrentFetchType;

	// For Row Count
	IDL_long_long	m_RowCount;

	// To support Dynamically change Fetch Buffer Size to ZERO for PUB/SUB and update/delete where current of CURSOR
	long			m_FetchBufferSize;		// in Bytes

	long*     m_ColumnIndexes;
	
	// While fetching data, a SQLBindCol(), SQLFetch(), SQLGetData() sequence can 
	// cause a double swap. This array prevents it.
	char**	  m_SwapInfo;
	long	  m_SwapInfo_NumRows;

	friend class	CConnect;
};

void ResetRowsetValues(CStmt *pStmt);
void GetRowsetValues(CStmt *pStmt, unsigned char* sql_i, unsigned char* sql_ie);
SQLRETURN ProcessRowset(CStmt *pStmt);

#endif
