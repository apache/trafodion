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
#ifndef CDESC_H
#define CDESC_H

#include <windows.h>
#include <sql.h>
#include <sqlext.h>


#include <vector> 
#include "drvrglobal.h"
#include "chandle.h"
#include "odbccommon.h"
#include "cconnect.h"

#define MAX_IDENTIFIER_LEN	128

class CDesc;
class CStmt;

class RWRS_columnOffsets {
public:
    int varLocation;
    int indLocation;
};

class RWRS_Descriptor {
public:
	RWRS_Descriptor(CDesc *ipd);
    RWRS_columnOffsets *m_inputColOffsets;
	int m_inputRowLength;
    int m_inputDescCount;
	void alignColumnOffsets(SQLSMALLINT SQLDataType,SQLINTEGER SQLMaxLength,SQLINTEGER SQLDatetimeCode, int* MemOffSet,int* varOffSet);
};


class CDescRec {

public:
	CDescRec(SQLSMALLINT DescMode,CDesc* DescDesc);
	~CDescRec();
	unsigned long setDescRec(short DescMode,
				SQLSMALLINT	Type,
				SQLSMALLINT	SubType,
				SQLLEN	Length,
				SQLSMALLINT	Precision,
				SQLSMALLINT	Scale,
				SQLPOINTER	DataPtr,
				SQLLEN	*StringLengthPtr,
				SQLLEN	*IndicatorPtr);
	unsigned long setDescRec(short DescMode,
					SQLSMALLINT TargetType,
					SQLPOINTER TargetValuePtr,
					SQLINTEGER BufferLength,
					SQLLEN *StrLen_or_IndPtr);
	unsigned long BindParameter(short DescMode,
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLPOINTER  ParameterValuePtr,
			SQLINTEGER	BufferLength,
			SQLLEN *StrLen_or_IndPtr);
	unsigned long BindParameter(short DescMode,
					SQLSMALLINT ParameterType, 
					SQLUINTEGER	ColumnSize,
					SQLSMALLINT DecimalDigits);
	unsigned long setDescRec(short DescMode, SQLItemDesc_def *SQLItemDesc);
	unsigned long checkConsistency(short DescMode);
	unsigned long CopyDesc(CDescRec *srcDescRec);
	unsigned long SetDescField(short DescMode,
					SQLSMALLINT FieldIdentifier,
					SQLPOINTER ValuePtr,
					SQLINTEGER BufferLength);
	inline  SQLSMALLINT	getODBCPrecision() { return m_ODBCPrecision; }
	inline void deleteTranslatedDataPtr() {
		if (m_TranslatedDataPtr != NULL)
		{
			delete[] m_TranslatedDataPtr;
			m_TranslatedDataPtr = NULL;
		}
	};

//private:
public:
	SQLSMALLINT	m_DescType;
	SQLSMALLINT	m_DescConciseType;
	SQLSMALLINT	m_DescDatetimeIntervalCode;
	SQLINTEGER	m_DescOctetLength;
	SQLUINTEGER	m_DescLength;
	SQLINTEGER	m_DescPrecision;
	SQLSMALLINT	m_DescScale;
	SQLINTEGER	m_DescDatetimeIntervalPrecision;
	SQLSMALLINT	m_DescFixedPrecScale;
	SQLSMALLINT	m_DescNullable;
	SQLPOINTER	m_DescDataPtr;
	SQLLEN	    *m_DescIndicatorPtr;
	SQLINTEGER 	m_DescDisplaySize;
	SQLLEN	    *m_DescOctetLengthPtr;   
	SQLSMALLINT	m_DescParameterType;
	SQLCHAR		m_DescBaseColumnName[MAX_IDENTIFIER_LEN+1];
	SQLCHAR		m_DescName[MAX_IDENTIFIER_LEN+1];
	
	// These items are returned from SQL and are stored as separate entities.
	// These items should not be copied or overwritten by SQLCopyDesc or any other means
	// Overwrtting this items may cause unexpected behavior
	SQLINTEGER	m_SQLCharset;
	SQLINTEGER	m_ODBCCharset;
	SQLSMALLINT m_SQLDataType;
	SQLSMALLINT m_ODBCDataType;
	SQLINTEGER	m_SQLPrecision;
	SQLINTEGER	m_ODBCPrecision;
	SQLSMALLINT	m_ODBCScale;
	SQLSMALLINT m_SQLDatetimeCode;
	SQLINTEGER	m_SQLOctetLength;
	SQLSMALLINT	m_SQLUnsigned;
	SQLINTEGER	m_SQLMaxLength;
	
	SQLINTEGER	m_DescAutoUniqueValue;
	SQLINTEGER	m_DescCaseSensitive;
	SQLINTEGER	m_DescNumPrecRadix;
	SQLSMALLINT	m_DescSearchable;
	SQLSMALLINT	m_DescUnnamed;
	SQLSMALLINT	m_DescUnsigned;
	SQLSMALLINT	m_DescUpdatable;

	SQLCHAR		m_DescTypeName[MAX_IDENTIFIER_LEN+1];
	SQLCHAR		m_DescBaseTableName[MAX_IDENTIFIER_LEN+1];
	SQLCHAR		m_DescCatalogName[MAX_IDENTIFIER_LEN+1];
	SQLCHAR		m_DescLabel[MAX_IDENTIFIER_LEN+1];
	SQLCHAR		m_DescTableName[MAX_IDENTIFIER_LEN+1];
	
	SQLCHAR		m_DescLiteralPrefix[20];
	SQLCHAR		m_DescLiteralSuffix[20];
	SQLCHAR		m_DescLocalTypeName[MAX_IDENTIFIER_LEN+1];
	SQLCHAR		m_DescSchemaName[MAX_IDENTIFIER_LEN];

	SQLINTEGER	m_DescReturnedLength;	// Used by SQLGetData - Length of data returned already
									// SQL_NO_DATA means the data is alreay sent
	CDesc*		m_DescDesc;
 	CHAR		*m_TranslatedDataPtr; // Used by SQLGetData - 10-090119-8521

	friend class CDesc;	
};


typedef CDescRec *CDescRecPtr;


typedef std::vector<CDescRecPtr> CDESCRECLIST; 

class CDesc : public CHandle {

public:
	CDesc(SQLHANDLE InputHandle, short DescMode, SQLSMALLINT DescAllocType, CStmt *pStmt = NULL);
	~CDesc();
	void clear();
	void deleteRecords();
	SQLRETURN SetDescRec(SQLSMALLINT	RecNumber,
					SQLSMALLINT	Type,
					SQLSMALLINT	SubType,
					SQLLEN	Length,
					SQLSMALLINT	Precision,
					SQLSMALLINT	Scale,
					SQLPOINTER	DataPtr,
					SQLLEN	*StringLengthPtr,
					SQLLEN	*IndicatorPtr);
	SQLRETURN GetDescRec(SQLSMALLINT	RecNumber, 
					SQLCHAR *Name,
					SQLSMALLINT	BufferLength,
					SQLSMALLINT	*StringLengthPtr,
					SQLSMALLINT	*TypePtr,
					SQLSMALLINT	*SubTypePtr,
					SQLLEN   	*LengthPtr,
					SQLSMALLINT	*PrecisionPtr,
					SQLSMALLINT	*ScalePtr,
					SQLSMALLINT	*NullablePtr);
	SQLRETURN SetDescField(SQLSMALLINT RecNumber,
					SQLSMALLINT FieldIdentifier,
					SQLPOINTER ValuePtr,
					SQLINTEGER BufferLength);
	unsigned long SetDescField(SQLSMALLINT RecNumber);
	SQLRETURN GetDescField(SQLSMALLINT RecNumber,
					SQLSMALLINT FieldIdentifier,
					SQLPOINTER ValuePtr,
					SQLINTEGER BufferLength,
					SQLINTEGER *StringLengthPtr);
	unsigned long BindCol(SQLSMALLINT ColumnNumber,
					SQLSMALLINT TargetType,
					SQLPOINTER TargetValuePtr,
					SQLLEN BufferLength,
					SQLLEN *StrLen_or_IndPtr);
	unsigned long BindParameter(SQLSMALLINT ParameterNumber, 
					SQLSMALLINT InputOutputType,
					SQLSMALLINT ValueType,
					SQLPOINTER  ParameterValuePtr,
					SQLINTEGER	BufferLength,
					SQLLEN *StrLen_or_Ind);
	unsigned long BindParameter(SQLSMALLINT ParameterNumber, 
					SQLSMALLINT ParameterType, 
					SQLUINTEGER	ColumnSize,
					SQLSMALLINT DecimalDigits);
	unsigned long setDescRec(const SQLItemDescList_def *SQLDesc);
	unsigned long getDescRec(SQLUSMALLINT ColumnNumber, 
			SQLCHAR *ColumnName,
			SQLSMALLINT BufferLength, 
			SQLSMALLINT *NameLengthPtr,
		    SQLSMALLINT *DataTypePtr, 
			SQLULEN *ColumnSizePtr,
			SQLSMALLINT *DecimalDigitsPtr,
			SQLSMALLINT *NullablePtr,
			UCHAR		*errorMsg,
			SWORD		errorMsgMax);
	SQLRETURN BuildValueList(CStmt *pHandle,	//AMR - changed this pointer type to CStmt*, (from CHandle *) in order to access m_RowId member of CStmt object
			SQLSMALLINT	ImpParamCount,
			CDesc		*IPD,
			SQLINTEGER	ODBCAppVersion,
			SQLSMALLINT &ParamNumber, 
			SQLULEN     &RowNumber,
			SQLValueList_def *InputValueList,
			BYTE		*&ParamBuffer);
	unsigned long GetParamType(SQLSMALLINT RowNumber, 
			SQLSMALLINT ParamNumber,
			SQLSMALLINT &DataType,
			SQLLEN  	*&StrLenPtr);
	unsigned long GetParamSQLInfo(SQLSMALLINT RowNumber,
			SQLSMALLINT	ParamNumber,
			SQLSMALLINT	&ParameterType,
			SQLSMALLINT	&ODBCDataType,
			SQLINTEGER &ParamCharSet);	//Add for getting the character set information.
	SQLRETURN CopyData(CHandle	*pHandle,
			SQLSMALLINT		ImpColumnCount,
			CDesc			*IRD,
			SQLULEN			RowsFetched,
			SQLULEN			&CurrentRowFetched,
			SQLULEN 		&RowNumber,
			SQLULEN 		&RowsetSize);
	SQLRETURN ExtendedCopyData(CHandle	*pHandle,
			SQLSMALLINT	ImpColumnCount,
			CDesc		*IRD,
			SQLULEN		RowsFetched,
			SQLULEN		&CurrentRowFetched,
			SQLULEN 	&RowNumber,
			SQLULEN 	&RowsetSize,
			SQLUSMALLINT* RowStatusArray);
	unsigned long GetData(SQLValue_def *SQLValue,
			SQLUSMALLINT ColumnNumber,
			SQLSMALLINT TargetType,
			SQLPOINTER	TargetValuePtr, 
			SQLLEN	BufferLength,
			SQLLEN *StrLen_or_IndPtr,
			UCHAR		*errorMsg,
			SWORD		errorMsgMax,
			BOOL		ColumnwiseData);
	unsigned long AssignDataAtExecValue(SQLValue_def *DataAtExecData,
			CDesc		*APD,
			SQLINTEGER	ODBCAppVersion,
			SQLSMALLINT ParamNumber, 
			SQLSMALLINT RowNumber,
			SQLValueList_def *OutputValueList,
			UCHAR		*errorMsg,
			SWORD		errorMsgMax);
	unsigned long ParamData(SQLSMALLINT RowNumber, SQLSMALLINT ParamNumber, SQLPOINTER *ValuePtrPtr);
	SQLRETURN CopyDesc(SQLHDESC SourceDescHandle);
	SQLRETURN SetPos(SQLSMALLINT NumResultCols);

	inline SQLINTEGER getRecLength() { return m_DescRecLength; };
	inline void setRowsProcessed(SQLULEN RowsProcessed) { 
		if (m_DescRowsProcessedPtr != NULL) 
			*m_DescRowsProcessedPtr = RowsProcessed;
	};
	inline void setDescArrayStatus(SQLUINTEGER MaxRowNumber, SQLUINTEGER value = SQL_ROW_IGNORE ) {
		if (m_DescArrayStatusPtr != NULL)
			for( SQLUINTEGER i=0; i < MaxRowNumber; i++)
				*(m_DescArrayStatusPtr + i) = value;
	};
	inline void setDescArrayStatusAt(SQLUINTEGER RowNumber, SQLUINTEGER value) {
		if (m_DescArrayStatusPtr != NULL)
			*(m_DescArrayStatusPtr + RowNumber) = value;
	};
	inline SQLUINTEGER getDescArrayStatusFrom(SQLUINTEGER RowNumber) {
		if (m_DescArrayStatusPtr != NULL)
			return *(m_DescArrayStatusPtr + RowNumber);
		return SQL_ERROR;
	};
	inline BOOL	getByteSwap() { return m_ConnectHandle->getByteSwap(); };
	inline DWORD getDataLang() { return m_ConnectHandle->getDataLang(); };
	inline DWORD getErrorMsgLang() { return m_ConnectHandle->getErrorMsgLang(); };
	inline char *getReplacementChar() { return m_ConnectHandle->getReplacementChar(); };
	inline long getDescCount(){	return m_DescCount;}
	inline CStmt* getDescStmt(){ return m_AssociatedStmt; };
	BOOL getDescRecSQLData(long colnumber,
		SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
		SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLOctetLength, 
		SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned);
	BOOL getDescRecBulkSQLData(long colnumber,
		SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
		SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLOctetLength, 
		SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned);
	BOOL getDescParamRecData(long colnumber,
		SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
		SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLMaxLength, 
		SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned,
		SQLSMALLINT& SQLNullable);
	BOOL getDescRowRecData(long colnumber,
		SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
		SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLMaxLength, 
		SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned,
		SQLSMALLINT& SQLNullable);
	BOOL getDescRowRecData(long colnumber, SQLSMALLINT& SQLDataType, 
		SQLINTEGER& SQLMaxLength, SQLSMALLINT& SQLNullable) ;

	inline CConnect* getDescConnect(){return m_ConnectHandle;}

	// For Stored Proceudre Call support
	SQLINTEGER returnedLength(long colnumber);					
	inline 	CDescRec *getDescRec(int index) 
	{ 
	    return m_DescRecCollect[index];
	}

private:
	short			m_DescMode;
	SQLSMALLINT		m_DescAllocType;
	SQLUINTEGER		m_DescArraySize;
	SQLUSMALLINT	*m_DescArrayStatusPtr;
	SQLINTEGER		m_DescBindType;
	SQLLEN  		*m_DescBindOffsetPtr;
	SQLSMALLINT		m_DescCount;
	SQLULEN 		*m_DescRowsProcessedPtr;
	CDESCRECLIST	m_DescRecCollect;

	SQLINTEGER		m_DescRecLength;		// This field accumaltes the OctetLength of all
											// DescRec entries, which is equivalent to RecLength
	CStmt			*m_AssociatedStmt;		// Stmt Associated for IRD and IPD
	CConnect		*m_ConnectHandle;
        CHAR                    *m_replacementChar;             // 10-080310-1259
        SQLINTEGER              m_ODBCAppVersion;               // 10-080310-1259
        DWORD                   m_drvrCharSet;                  // 10-080310-1259
	friend class    RWRS_Descriptor;
};

struct CacheDescInfo
{
	SQLINTEGER SQLCharset;
	SQLSMALLINT SQLDataType;
	SQLSMALLINT SQLDatetimeCode;
	SQLINTEGER SQLOctetLength;
	SQLSMALLINT SQLPrecision;
	SQLSMALLINT SQLUnsigned;
	SQLINTEGER bufferLength;
};
typedef CacheDescInfo CacheDescStruct;

#endif
