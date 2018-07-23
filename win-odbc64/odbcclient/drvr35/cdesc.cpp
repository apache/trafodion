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
#include "CDesc.h"
#include "DrvrGlobal.h"
#include "tdm_odbcDrvMsg.h"
#include "sqltocconv.h"
#include "ctosqlconv.h"
#include "DrvrSrvr.h"
#include "sqlCli.h"
#include "CStmt.h"
#include "DiagFunctions.h"
#include "nskieee.h"
#include "swap.h"

using namespace ODBC;

#ifdef unixcli
SQLINTEGER CDesc::returnedLength(long colnumber) {
        CDescRec                *descRecPtr;
        GETDESCRECPTR(descRecPtr, m_DescRecCollect, colnumber-1);
        return descRecPtr->m_DescReturnedLength;

}
#else
SQLINTEGER CDesc::returnedLength(long colnumber) {
	return m_DescRecCollect[colnumber-1]->m_DescReturnedLength;
};
#endif

// Implementation for the member functions of CDescRec
CDescRec::CDescRec(SQLSMALLINT DescMode, CDesc* Desc)
{
	m_DescDesc = Desc;
	m_DescAutoUniqueValue = SQL_FALSE;
	m_DescBaseColumnName[0] = '\0';
	m_DescBaseTableName[0] = '\0';
	m_DescCaseSensitive = SQL_FALSE;
	m_DescCatalogName[0] = '\0';
	m_DescDataPtr = NULL;
	m_DescDatetimeIntervalPrecision = 0;
	m_DescDisplaySize = 0;
	m_DescFixedPrecScale = 0;
	m_DescIndicatorPtr = NULL;
	m_DescLabel[0] = '\0';
	m_DescLiteralPrefix[0] = '\0';
	m_DescLiteralSuffix[0] = '\0';
	m_DescLocalTypeName[0] = '\0';
	m_DescName[0] = '\0';
	m_DescNullable = SQL_NULLABLE_UNKNOWN;
	m_DescNumPrecRadix = 0;
	m_DescOctetLength = 0;
	m_DescOctetLengthPtr = NULL;
	m_DescParameterType = SQL_PARAM_TYPE_UNKNOWN;
	m_DescPrecision = 0;
	m_DescScale = 0;
	m_DescSchemaName[0] = '\0';
	m_DescSearchable = SQL_PRED_NONE;
	m_DescTableName[0] = '\0';
	m_DescUnnamed = SQL_UNNAMED;
	m_DescUnsigned = SQL_FALSE;
	m_DescUpdatable = SQL_ATTR_READWRITE_UNKNOWN;
	m_DescTypeName[0] = '\0';
	m_DescLength = 0;
	m_translateOptionFromSQL = -1; 
	m_translateOptionToSQL = -1; 
	switch (DescMode)
	{
	case SQL_ATTR_IMP_ROW_DESC:
	case SQL_ATTR_IMP_PARAM_DESC:
		m_DescType = SQL_UNKNOWN_TYPE;
		break;
	case SQL_ATTR_APP_ROW_DESC:
	case SQL_ATTR_APP_PARAM_DESC:
	default:
		m_DescType = SQL_C_DEFAULT;
		break;
	}
	m_DescConciseType = m_DescType; // Appendix - D
	m_DescDatetimeIntervalCode = 0;

	m_SQLDataType = m_DescType;
	m_ODBCDataType = m_DescType;
	m_SQLPrecision = m_DescPrecision;
	m_ODBCPrecision = m_ODBCPrecision;
	m_SQLDatetimeCode = 0;
	m_SQLCharset = 0;
	m_ODBCCharset = 0;
	m_SQLOctetLength = 0;
	m_DescReturnedLength = 0;
	m_SQLUnsigned = SQL_FALSE;
	m_TranslatedDataPtr = NULL;
}

CDescRec::~CDescRec()
{
	deleteTranslatedDataPtr();
}

unsigned long CDescRec::setDescRec(short DescMode,
				SQLSMALLINT	Type,
				SQLSMALLINT	SubType,
				SQLLEN		Length,
				SQLSMALLINT	Precision,
				SQLSMALLINT	Scale,
				SQLPOINTER	DataPtr,
				SQLLEN		*StringLengthPtr,
				SQLLEN		*IndicatorPtr)
{
	unsigned long retCode=SQL_SUCCESS;

	m_DescType = Type;
	m_DescConciseType = Type;
	m_DescDatetimeIntervalCode = SubType;
	m_DescOctetLength = Length;
	m_DescPrecision = Precision;
	m_DescScale = Scale;
	m_DescOctetLengthPtr = StringLengthPtr;
	m_DescIndicatorPtr = IndicatorPtr;
	if(DataPtr!=NULL)
		retCode = checkConsistency(DescMode);
	if(DescMode!=SQL_ATTR_IMP_PARAM_DESC)
		m_DescDataPtr = DataPtr;
	return retCode;
}

unsigned long CDescRec::setDescRec(short DescMode,
					SQLSMALLINT TargetType,
					SQLPOINTER TargetValuePtr,
					SQLLEN BufferLength,
					SQLLEN *StrLen_or_IndPtr)
{
	BOOL	found;
	short	i;
	unsigned long retCode;

	// Checking if ConciseType in in gCDatatypeMap table
	if (TargetType != SQL_C_DEFAULT)
	{
		for (i = 0, found = FALSE; gCDatatypeMap[i].conciseType != SQL_C_DEFAULT ; i++)
		{
			if (gCDatatypeMap[i].conciseType == TargetType)
			{
				found = TRUE;
				m_DescConciseType = TargetType;
				m_DescDatetimeIntervalCode = gCDatatypeMap[i].datetimeIntervalCode;
				m_DescType = gCDatatypeMap[i].verboseType;
				strcpy((char*)m_DescTypeName, gCDatatypeMap[i].typeName);
				break;
			}
		}
		if (! found)
			return IDS_HY_003;
	}
	else
	{
		m_DescConciseType = SQL_C_DEFAULT;
		m_DescDatetimeIntervalCode = 0;
		m_DescType = SQL_C_DEFAULT;
		m_DescTypeName[0] = '\0';
	}
	m_DescOctetLength = BufferLength;
	m_DescDataPtr	= TargetValuePtr;
	m_DescOctetLengthPtr = StrLen_or_IndPtr;
	m_DescIndicatorPtr = StrLen_or_IndPtr;
	retCode = checkConsistency(DescMode);
	return retCode;
}

unsigned long CDescRec::BindParameter(short DescMode,
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN		BufferLength,
			SQLLEN		*StrLen_or_IndPtr)
{
	unsigned long retCode;

	if (InputOutputType != SQL_PARAM_INPUT && InputOutputType != SQL_PARAM_INPUT_OUTPUT && InputOutputType != SQL_PARAM_OUTPUT)
		return IDS_HY_105;
	
	m_DescParameterType = InputOutputType;
	retCode = setDescRec(DescMode, ValueType, ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
	return retCode;
}

unsigned long CDescRec::BindParameter(short DescMode,
					SQLSMALLINT ParameterType, 
					SQLULEN		ColumnSize,
					SQLSMALLINT DecimalDigits)
{
	unsigned long retCode = SQL_SUCCESS;
	BOOL	found;
	short	i;
	
	for (i = 0, found = FALSE; gSQLDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
	{
		if (gSQLDatatypeMap[i].conciseType == ParameterType)
		{
			found = TRUE;
			m_DescConciseType = ParameterType;
			m_DescDatetimeIntervalCode = gSQLDatatypeMap[i].datetimeIntervalCode;
			m_DescType = gSQLDatatypeMap[i].verboseType;
			strcpy((char*)m_DescTypeName, gSQLDatatypeMap[i].typeName);
			break;
		}
	}
	if (! found)
		return IDS_HY_021;
	switch (gSQLDatatypeMap[i].columnSizeAttr)
	{
	case SQL_DESC_LENGTH:
		m_DescLength = ColumnSize;
		break;
	case SQL_DESC_PRECISION:
		m_DescPrecision = ColumnSize;
		break;
	default:
		break;
	}
	switch (gSQLDatatypeMap[i].decimalDigitsAttr)
	{
	case SQL_DESC_PRECISION:
		m_DescPrecision = DecimalDigits;
		break;
	case SQL_DESC_SCALE:
		m_DescScale = DecimalDigits;
		break;
	default:
		break;
	}
	return retCode;
}

unsigned long CDescRec::setDescRec(short DescMode, SQLItemDesc_def *SQLItemDesc)
{
	unsigned long	retCode = SQL_SUCCESS;
	short			i;
	BOOL			found;
	short			DisplaySizeOffset = 0;
	SQLINTEGER		ODBCAppVersion = m_DescDesc->getDescConnect()->getODBCAppVersion();

	if (SQLItemDesc->colHeadingNm != NULL && SQLItemDesc->colHeadingNm[0] != '\0')
	{
		strcpy((char *)m_DescName, SQLItemDesc->colHeadingNm);
		m_DescUnnamed = SQL_NAMED;
	}
	else
	{
		m_DescName[0] = '\0';
		m_DescUnnamed = SQL_UNNAMED;
	}

	if (DescMode == SQL_ATTR_IMP_ROW_DESC)
	{
		strcpy((char *)m_DescBaseColumnName,(char *)m_DescName);
		if (SQLItemDesc->TableName != NULL)
		{
			strcpy((char *)m_DescBaseTableName, SQLItemDesc->TableName);
			strcpy((char *)m_DescTableName, SQLItemDesc->TableName);
		}
		if (SQLItemDesc->SchemaName != NULL)
			strcpy((char *)m_DescSchemaName, SQLItemDesc->SchemaName);
		if (SQLItemDesc->CatalogName != NULL)
			strcpy((char *)m_DescCatalogName, SQLItemDesc->CatalogName);
		if (SQLItemDesc->Heading != NULL)
			strcpy((char *)m_DescLabel, SQLItemDesc->Heading);
		if (m_DescLabel[0] == '\0')
			strcpy((char *)m_DescLabel,(char *)m_DescName);
	}

	m_DescLength = 0;
	m_DescPrecision = 0;
	m_DescScale = 0;
	m_DescFixedPrecScale = SQL_FALSE;
	m_DescNumPrecRadix = 0;
	if (SQLItemDesc->signType == TRUE)
		m_DescUnsigned = SQL_FALSE;
	else
		m_DescUnsigned = SQL_TRUE;
	m_DescNullable = SQLItemDesc->nullInfo;
	m_SQLDataType = SQLItemDesc->dataType;
	m_ODBCDataType = SQLItemDesc->ODBCDataType;
	m_SQLDatetimeCode = SQLItemDesc->datetimeCode;
	m_SQLCharset = SQLItemDesc->SQLCharset;
	m_ODBCCharset = SQLItemDesc->ODBCCharset;
	m_SQLUnsigned = m_DescUnsigned;
	m_DescDatetimeIntervalPrecision = SQLItemDesc->intLeadPrec;
	m_DescParameterType = SQLItemDesc->paramMode;

	if (m_ODBCCharset == SQLCHARSETCODE_UCS2)
	{
		switch(m_ODBCDataType)
		{
		case SQL_CHAR:
			m_ODBCDataType = SQL_WCHAR;
			break;
		case SQL_VARCHAR:
			m_ODBCDataType = SQL_WVARCHAR;
			break;
		case SQL_LONGVARCHAR:
			m_ODBCDataType = SQL_WVARCHAR;
			break;
		default:
			break;
		}
	}

	for (i = 0, found = FALSE; gSQLDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
	{
		if (gSQLDatatypeMap[i].conciseType == m_ODBCDataType)
		{
			found = TRUE;
			m_DescConciseType = m_ODBCDataType;
			m_DescDatetimeIntervalCode = gSQLDatatypeMap[i].datetimeIntervalCode;
			m_DescType = gSQLDatatypeMap[i].verboseType;
			break;
		}
	}
	if (! found)
		return IDS_HY_021;
	strcpy((char *)m_DescTypeName, gSQLDatatypeMap[i].typeName);
	switch (m_DescConciseType)
	{
	case SQL_NUMERIC:
	case SQL_DECIMAL:
		m_DescNumPrecRadix = 10;
		m_DescScale = SQLItemDesc->scale;
		m_DescPrecision =  SQLItemDesc->ODBCPrecision;
		if (m_DescScale != 0)
			m_DescFixedPrecScale = SQL_TRUE;
		break;
	case SQL_SMALLINT:
	case SQL_INTEGER:
	case SQL_TINYINT:
		m_DescNumPrecRadix = 10;
		m_DescPrecision =  SQLItemDesc->ODBCPrecision;
		if (m_SQLUnsigned)		 
			DisplaySizeOffset = 1;
		break;
	case SQL_BIGINT:
		m_DescPrecision =  SQLItemDesc->ODBCPrecision;
		m_DescNumPrecRadix = 10;
		break;
	case SQL_REAL:
	case SQL_DOUBLE:
	case SQL_FLOAT:
		m_DescPrecision =  SQLItemDesc->ODBCPrecision;
		m_DescNumPrecRadix = 2;
		break;
	case SQL_DATE:
	case SQL_TYPE_DATE:
#if (ODBCVER >= 0x0300)
		if (m_DescConciseType == SQL_DATE)
			m_DescConciseType = SQL_TYPE_DATE;
#else
		if(m_DescConciseType == SQL_TYPE_DATE)
			m_DescConciseType = SQL_DATE;
#endif
		strcpy((char*)m_DescLiteralPrefix,"{d'");
		strcpy((char*)m_DescLiteralSuffix,"'}");
		m_DescLength = 10;
		m_DescPrecision = 0;
		break;
	case SQL_TIME:
	case SQL_TYPE_TIME:
#if (ODBCVER >= 0x0300)
		if (m_DescConciseType == SQL_TIME)
			m_DescConciseType = SQL_TYPE_TIME;
#else
		if(m_DescConciseType == SQL_TYPE_TIME)
			m_DescConciseType = SQL_TIME;
#endif
		strcpy((char*)m_DescLiteralPrefix,"{t'");
		strcpy((char*)m_DescLiteralSuffix,"'}");
		m_DescLength = 8;
		if (SQLItemDesc->ODBCPrecision == 8) //Preserve the non rwws insert behavior
			m_DescPrecision = 0;
		else
			m_DescPrecision = SQLItemDesc->ODBCPrecision; //For rwrs fetch

		break;
	case SQL_TYPE_TIMESTAMP:
	case SQL_TIMESTAMP:
#if (ODBCVER >= 0x0300)
		if (m_DescConciseType == SQL_TIMESTAMP)
			m_DescConciseType = SQL_TYPE_TIMESTAMP;
#else
		if(m_DescConciseType == SQL_TYPE_TIMESTAMP)
			m_DescConciseType = SQL_TIMESTAMP;
#endif
		strcpy((char*)m_DescLiteralPrefix,"{ts'");
		strcpy((char*)m_DescLiteralSuffix,"'}");
		m_DescPrecision = SQLItemDesc->precision; // Should come for Server - Must be changed
		if (m_DescPrecision == 0)
			m_DescLength = 19;
		else
			m_DescLength = 19 + m_DescPrecision + 1; //for Dot;
		break;
	case SQL_CHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
		strcpy((char*)m_DescLiteralPrefix,"'");
		strcpy((char*)m_DescLiteralSuffix,"'");
		m_DescLength = SQLItemDesc->maxLen;
		m_DescPrecision = m_DescLength;
		switch (m_SQLCharset)
		{
		case SQLCHARSETCODE_ISO88591:
			sprintf((char *)m_DescLocalTypeName,"%s CHARACTER SET %s", gSQLDatatypeMap[i].typeName, SQLCHARSETSTRING_ISO88591);
			break;
		case SQLCHARSETCODE_KANJI:
			sprintf((char *)m_DescLocalTypeName,"%s CHARACTER SET %s", gSQLDatatypeMap[i].typeName, SQLCHARSETSTRING_KANJI);
			break;
		case SQLCHARSETCODE_KSC5601:
			sprintf((char *)m_DescLocalTypeName,"%s CHARACTER SET %s", gSQLDatatypeMap[i].typeName, SQLCHARSETSTRING_KSC5601);
			break;
		case SQLCHARSETCODE_SJIS:
			sprintf((char *)m_DescLocalTypeName,"%s CHARACTER SET SJIS", gSQLDatatypeMap[i].typeName);
			break;
		case SQLCHARSETCODE_UCS2:
			sprintf((char *)m_DescLocalTypeName,"%s CHARACTER SET %s", gSQLDatatypeMap[i].typeName, SQLCHARSETSTRING_UNICODE);
			break;
		default:
			sprintf((char *)m_DescLocalTypeName,"%s CHARACTER SET UNKNOWN(%d)", gSQLDatatypeMap[i].typeName, m_SQLCharset);
			break;
		}
		break;
	case SQL_WCHAR:
	case SQL_WVARCHAR:
		strcpy((char*)m_DescLiteralPrefix,"N'");
		strcpy((char*)m_DescLiteralSuffix,"'");
		m_DescLength = SQLItemDesc->maxLen/2;
		m_DescPrecision = m_DescLength;
		sprintf((char *)m_DescLocalTypeName,"%s CHARACTER SET %s", gSQLDatatypeMap[i].typeName, SQLCHARSETSTRING_UNICODE);
		break;
	case SQL_INTERVAL_SECOND:
		m_DescPrecision = SQLItemDesc->precision; // Should come for Server - Must be changed
		if (m_DescPrecision == 0)
			m_DescLength = m_DescDatetimeIntervalPrecision;
		else
			m_DescLength = m_DescDatetimeIntervalPrecision + m_DescPrecision + 1; 
		break;
	case SQL_INTERVAL_DAY_TO_SECOND:
		m_DescPrecision = SQLItemDesc->precision; // Should come for Server - Must be changed
		if (m_DescPrecision == 0)
			m_DescLength = 9 + m_DescDatetimeIntervalPrecision;
		else
			m_DescLength = 10 + m_DescDatetimeIntervalPrecision + m_DescPrecision; 
		break;
	case SQL_INTERVAL_HOUR_TO_SECOND:
		m_DescPrecision = SQLItemDesc->precision; // Should come for Server - Must be changed
		if (m_DescPrecision == 0)
			m_DescLength = 6 + m_DescDatetimeIntervalPrecision;
		else
			m_DescLength = 7 + m_DescDatetimeIntervalPrecision + m_DescPrecision; 
		break;
	case SQL_INTERVAL_MINUTE_TO_SECOND:
		m_DescPrecision = SQLItemDesc->precision; // Should come for Server - Must be changed
		if (m_DescPrecision == 0)
			m_DescLength = 3 + m_DescDatetimeIntervalPrecision;
		else
			m_DescLength = 4 + m_DescDatetimeIntervalPrecision + m_DescPrecision; 
		break;
	case SQL_INTERVAL_YEAR:
	case SQL_INTERVAL_MONTH:
	case SQL_INTERVAL_DAY:
	case SQL_INTERVAL_HOUR:
	case SQL_INTERVAL_MINUTE:
		m_DescLength = m_DescDatetimeIntervalPrecision;
		m_DescPrecision = 0;
		break;
	case SQL_INTERVAL_YEAR_TO_MONTH:
	case SQL_INTERVAL_DAY_TO_HOUR:
	case SQL_INTERVAL_HOUR_TO_MINUTE:
		m_DescLength = 3 + m_DescDatetimeIntervalPrecision;
		m_DescPrecision = 0;
		break;
	case SQL_INTERVAL_DAY_TO_MINUTE:
		m_DescLength = 6 + m_DescDatetimeIntervalPrecision;
		m_DescPrecision = 0;
		break;
    case TYPE_BLOB:
    case TYPE_CLOB:
        strcpy((char*)m_DescLiteralPrefix, "'");
        strcpy((char*)m_DescLiteralSuffix, "'");
        m_DescLength = SQLItemDesc->maxLen;
        m_DescPrecision = m_DescLength;
        break;
	default:
		return IDS_HY_021;
	}
	m_ODBCPrecision = m_DescPrecision;
	m_ODBCScale = m_DescScale;
	switch (gSQLDatatypeMap[i].displaySizeAttr)
	{
	case SQL_DESC_LENGTH:
		m_DescDisplaySize = m_DescLength;
		break;
	case SQL_DESC_PRECISION:
		m_DescDisplaySize = m_DescPrecision+2;
		break;
	case -1:
		m_DescDisplaySize = m_DescLength * 2;
		break;
	default:
		m_DescDisplaySize = gSQLDatatypeMap[i].displaySizeAttr;
		break;
	}
	m_DescDisplaySize -= DisplaySizeOffset;
	switch (gSQLDatatypeMap[i].octetLength)
	{
	case SQL_DESC_LENGTH:
		switch (m_DescConciseType)
		{
		case SQL_WCHAR:
		case SQL_WVARCHAR:
			m_DescOctetLength = m_DescLength; // but in descriptors it should be * 2;
			break;
		default:
			m_DescOctetLength = m_DescLength;
		}
		break;
	case SQL_DESC_PRECISION:
		m_DescOctetLength = m_DescPrecision+2;
		break;
	default:
		m_DescOctetLength = gSQLDatatypeMap[i].octetLength;
		break;
	}
	m_DescCaseSensitive = SQL_FALSE;
	switch (SQLItemDesc->dataType)
	{
	case SQLTYPECODE_CHAR:
	case SQLTYPECODE_VARCHAR:
		if (m_SQLCharset == SQLCHARSETCODE_UCS2)
			m_SQLOctetLength = SQLItemDesc->maxLen+2;
		else
			m_SQLOctetLength = SQLItemDesc->maxLen+1;
		m_DescCaseSensitive = SQL_TRUE;
		m_DescSearchable = SQL_PRED_SEARCHABLE;
		break;
	case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		if (m_SQLCharset == SQLCHARSETCODE_UCS2)
		{
			if (SQLItemDesc->maxLen > SHRT_MAX)
			{
				m_SQLOctetLength = SQLItemDesc->maxLen + 6;  //4 bytes for len and 2 bytes of null
			}
			else
			{
				m_SQLOctetLength = SQLItemDesc->maxLen + 4;  //2 bytes for len and 2 bytes of null
			}
		}
		else
		{ 
			if (SQLItemDesc->maxLen > SHRT_MAX)
			{
				m_SQLOctetLength = SQLItemDesc->maxLen + 5;  //4 bytes for len and 1 byte of null
			}
			else
			{
				m_SQLOctetLength = SQLItemDesc->maxLen + 3;  //2 bytes for len and 1 byte of null
			}
		}
		m_DescCaseSensitive = SQL_TRUE;
		m_DescSearchable = SQL_PRED_SEARCHABLE;
		break;
	case SQLTYPECODE_VARCHAR_LONG:
		if (m_SQLCharset == SQLCHARSETCODE_UCS2)
			m_SQLOctetLength = SQLItemDesc->maxLen+4;
		else
			m_SQLOctetLength = SQLItemDesc->maxLen+3;
		m_DescCaseSensitive = SQL_TRUE;
		m_DescSearchable = SQL_PRED_CHAR;
		break;
//	case SQLTYPECODE_CHAR_UP:
//		m_SQLOctetLength = SQLItemDesc->maxLen+1;
//		m_DescSearchable = SQL_PRED_SEARCHABLE;
//		break;
//	case SQLTYPECODE_VARCHAR_UP:
//		m_SQLOctetLength = SQLItemDesc->maxLen+3;
//		m_DescSearchable = SQL_PRED_SEARCHABLE;
//		break;
    case SQLTYPECODE_BLOB:
    case SQLTYPECODE_CLOB:
        m_SQLOctetLength = SQLItemDesc->maxLen + 4;
        break;
	default:
		m_SQLOctetLength = SQLItemDesc->maxLen;
		m_DescSearchable = SQL_PRED_BASIC;
		break;
	}
	m_SQLMaxLength = SQLItemDesc->maxLen;

	switch (m_DescConciseType)
	{
	case SQL_NUMERIC:
	case SQL_DECIMAL:
		m_DescLength = m_DescOctetLength;
		m_DescDatetimeIntervalPrecision = m_DescPrecision;
		break;
	case SQL_SMALLINT:
	case SQL_INTEGER:
	case SQL_TINYINT:
		m_DescLength = m_DescOctetLength;
		m_DescDatetimeIntervalPrecision = m_DescPrecision;
		break;
	case SQL_BIGINT:
		m_DescLength = m_DescOctetLength;
		m_DescDatetimeIntervalPrecision = m_DescPrecision;
		break;
	case SQL_REAL:
	case SQL_DOUBLE:
	case SQL_FLOAT:
		m_DescLength = m_DescOctetLength;
		m_DescDatetimeIntervalPrecision = m_DescPrecision;
		break;
	case SQL_DATE:
	case SQL_TYPE_DATE:
		m_DescDatetimeIntervalPrecision = m_DescPrecision;
		break;
	case SQL_TIME:
	case SQL_TYPE_TIME:
		m_DescDatetimeIntervalPrecision = m_DescPrecision;
		break;
	case SQL_TYPE_TIMESTAMP:
	case SQL_TIMESTAMP:
		m_DescDatetimeIntervalPrecision = m_DescPrecision;
		break;
	case SQL_CHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
	case SQL_WCHAR:
	case SQL_WVARCHAR:
    case TYPE_BLOB:
    case TYPE_CLOB:
		m_DescDatetimeIntervalPrecision = m_DescLength;
		break;
	case SQL_INTERVAL_SECOND:
	case SQL_INTERVAL_DAY_TO_SECOND:
	case SQL_INTERVAL_HOUR_TO_SECOND:
	case SQL_INTERVAL_MINUTE_TO_SECOND:
	case SQL_INTERVAL_YEAR:
	case SQL_INTERVAL_MONTH:
	case SQL_INTERVAL_DAY:
	case SQL_INTERVAL_HOUR:
	case SQL_INTERVAL_MINUTE:
	case SQL_INTERVAL_YEAR_TO_MONTH:
	case SQL_INTERVAL_DAY_TO_HOUR:
	case SQL_INTERVAL_HOUR_TO_MINUTE:
	case SQL_INTERVAL_DAY_TO_MINUTE:
		break;
	default:
		return IDS_HY_021;
	}
	return retCode;
}

unsigned long CDescRec::checkConsistency(short DescMode)
{
	unsigned long		retCode = SQL_SUCCESS;
	short		i;
	BOOL		found;

	switch (DescMode)
	{
	case SQL_ATTR_IMP_ROW_DESC:
	case SQL_ATTR_IMP_PARAM_DESC:
		// Checking if ConciseType in in DatetimeIntervalMap table
		for (i = 0, found = FALSE; gSQLDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
		{
			if (gSQLDatatypeMap[i].conciseType == m_DescConciseType)
			{
				found = TRUE;
				if (gSQLDatatypeMap[i].datetimeIntervalCode != m_DescDatetimeIntervalCode ||
						gSQLDatatypeMap[i].verboseType != m_DescType)
					retCode = IDS_HY_021;
				break;
			}
		}
		if (! found)
			retCode = IDS_HY_021;
		break;
	case SQL_ATTR_APP_ROW_DESC:
	case SQL_ATTR_APP_PARAM_DESC:
	default:
		// Checking if ConciseType in in DatetimeIntervalMap table
		if (m_DescConciseType != SQL_C_DEFAULT)
		{
			for (i = 0, found = FALSE; gCDatatypeMap[i].conciseType != SQL_C_DEFAULT ; i++)
			{
				if (gCDatatypeMap[i].conciseType == m_DescConciseType)
				{
					found = TRUE;
					if (gCDatatypeMap[i].datetimeIntervalCode != m_DescDatetimeIntervalCode ||
							gCDatatypeMap[i].verboseType != m_DescType)
						retCode = IDS_HY_021;
					break;
				}
			}
			if (! found)
				retCode = IDS_HY_021;
		}
		break;
	}
	return retCode;
}

unsigned long CDescRec::CopyDesc(CDescRec *srcDescRec)
{
	unsigned long retCode = SQL_SUCCESS;

	m_DescAutoUniqueValue = srcDescRec->m_DescAutoUniqueValue;
	strcpy((char *)m_DescBaseColumnName, (const char *)srcDescRec->m_DescBaseColumnName);
	strcpy((char *)m_DescBaseTableName, (const char *)srcDescRec->m_DescBaseTableName);
	m_DescCaseSensitive = srcDescRec->m_DescCaseSensitive;
	strcpy((char *)m_DescCatalogName, (const char *)srcDescRec->m_DescCatalogName);
	m_DescDataPtr = srcDescRec->m_DescDataPtr;
	m_DescDatetimeIntervalPrecision = srcDescRec->m_DescDatetimeIntervalPrecision;
	m_DescDisplaySize = srcDescRec->m_DescDisplaySize;
	m_DescFixedPrecScale = srcDescRec->m_DescFixedPrecScale;
	m_DescIndicatorPtr = srcDescRec->m_DescIndicatorPtr;
	strcpy((char *)m_DescLabel, (const char *)srcDescRec->m_DescLabel);
	strcpy((char *)m_DescLiteralPrefix, (const char *)srcDescRec->m_DescLiteralPrefix);
	strcpy((char *)m_DescLiteralSuffix, (const char *)srcDescRec->m_DescLiteralSuffix);
	strcpy((char *)m_DescLocalTypeName, (const char *)srcDescRec->m_DescLocalTypeName);
	strcpy((char *)m_DescName, (const char *)srcDescRec->m_DescName);
	m_DescNullable = srcDescRec->m_DescNullable;
	m_DescNumPrecRadix = srcDescRec->m_DescNumPrecRadix;
	m_DescOctetLength = srcDescRec->m_DescOctetLength;
	m_DescOctetLengthPtr = srcDescRec->m_DescOctetLengthPtr;
	m_DescParameterType = srcDescRec->m_DescParameterType;
	m_DescPrecision = srcDescRec->m_DescPrecision;
	m_DescScale = srcDescRec->m_DescScale;
	strcpy((char *)m_DescSchemaName, (const char *)srcDescRec->m_DescSchemaName);
	m_DescSearchable = srcDescRec->m_DescSearchable;
	strcpy((char *)m_DescTableName, (const char *)srcDescRec->m_DescTableName);
	m_DescUnnamed = srcDescRec->m_DescUnnamed;
	m_DescUnsigned = srcDescRec->m_DescUnsigned;
	m_DescUpdatable = srcDescRec->m_DescUpdatable;
	strcpy((char *)m_DescTypeName, (const char *)srcDescRec->m_DescTypeName);
	m_DescLength = srcDescRec->m_DescLength;
	m_DescType = srcDescRec->m_DescType;
	m_DescConciseType = srcDescRec->m_DescConciseType; 
	m_DescDatetimeIntervalCode = srcDescRec->m_DescDatetimeIntervalCode;
	return SQL_SUCCESS;
}

unsigned long CDescRec::SetDescField(short DescMode,
					SQLSMALLINT FieldIdentifier,
					SQLPOINTER ValuePtr,
					SQLINTEGER BufferLength)
{
	unsigned long	retCode = SQL_SUCCESS;
	BOOL			UnboundRecord = TRUE;
	short			strLen;
	BOOL			found = FALSE;
	int				i=0;
	SQLSMALLINT		value;

	switch (FieldIdentifier)
	{
	case SQL_DESC_CONCISE_TYPE:
		switch (DescMode)
		{
			case SQL_ATTR_IMP_ROW_DESC:
			case SQL_ATTR_IMP_PARAM_DESC:
				for (i = 0, found = FALSE; gSQLDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
				{
					if (gSQLDatatypeMap[i].conciseType == (SQLSMALLINT)ValuePtr)
					{
						found = TRUE;
						m_DescConciseType = gSQLDatatypeMap[i].conciseType;
						m_DescDatetimeIntervalCode = gSQLDatatypeMap[i].datetimeIntervalCode;
						m_DescType = gSQLDatatypeMap[i].verboseType;
						strcpy((char*)m_DescTypeName,gSQLDatatypeMap[i].typeName);
						break;
					}
				}
				if (! found)
					retCode = IDS_HY_021;
				break;

			case SQL_ATTR_APP_ROW_DESC:
			case SQL_ATTR_APP_PARAM_DESC:
			default:
				if ((SQLSMALLINT)ValuePtr != SQL_C_DEFAULT)
				{
					for (i = 0, found = FALSE; gCDatatypeMap[i].conciseType != SQL_C_DEFAULT ; i++)
					{
						if (gCDatatypeMap[i].conciseType == (SQLSMALLINT)ValuePtr)
						{
							found = TRUE;
							m_DescConciseType = gCDatatypeMap[i].conciseType;
							m_DescDatetimeIntervalCode = gCDatatypeMap[i].datetimeIntervalCode;
							m_DescType = gCDatatypeMap[i].verboseType;
							break;
						}
					}
					if (! found)
						retCode = IDS_HY_021;
				}
				else
				{
					m_DescConciseType = SQL_C_DEFAULT;
					m_DescDatetimeIntervalCode = 0;
					m_DescType = SQL_C_DEFAULT;
				}

			}
		break;

	case SQL_DESC_TYPE:
		switch (DescMode)
		{
			case SQL_ATTR_IMP_ROW_DESC:
			case SQL_ATTR_IMP_PARAM_DESC:
				for (i = 0, found = FALSE; gSQLDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
				{
					if (gSQLDatatypeMap[i].verboseType == (SQLSMALLINT)ValuePtr)
					{
						found = TRUE;
						if (gSQLDatatypeMap[i].verboseType != SQL_DATETIME && gSQLDatatypeMap[i].verboseType != SQL_INTERVAL)
							m_DescConciseType = gSQLDatatypeMap[i].conciseType;
						m_DescDatetimeIntervalCode = gSQLDatatypeMap[i].datetimeIntervalCode;
						m_DescType = gSQLDatatypeMap[i].verboseType;
						strcpy((char*)m_DescTypeName,gSQLDatatypeMap[i].typeName);
						break;
					}
				}
				if (! found)
					retCode = IDS_HY_021;
				break;

			case SQL_ATTR_APP_ROW_DESC:
			case SQL_ATTR_APP_PARAM_DESC:
			default:
				if ((SQLSMALLINT)ValuePtr != SQL_C_DEFAULT)
				{
					for (i = 0, found = FALSE; gCDatatypeMap[i].conciseType != SQL_C_DEFAULT ; i++)
					{
						if (gCDatatypeMap[i].verboseType == (SQLSMALLINT)ValuePtr)
						{
							found = TRUE;
							if (gCDatatypeMap[i].verboseType != SQL_DATETIME && gCDatatypeMap[i].verboseType != SQL_INTERVAL)
								m_DescConciseType = gCDatatypeMap[i].conciseType;
							m_DescDatetimeIntervalCode = gCDatatypeMap[i].datetimeIntervalCode;
							m_DescType = gCDatatypeMap[i].verboseType;
							strcpy((char*)m_DescTypeName,gCDatatypeMap[i].typeName);
							break;
						}
					}
					if (! found)
						retCode = IDS_HY_021;
				}
				else
				{
					m_DescConciseType = SQL_C_DEFAULT;
					m_DescDatetimeIntervalCode = 0;
					m_DescType = SQL_C_DEFAULT;
					m_DescTypeName[0] = '\0';
				}

			}
		break;

	case SQL_DESC_DATA_PTR:
		if (DescMode != SQL_ATTR_IMP_PARAM_DESC)
			m_DescDataPtr = (SQLPOINTER)ValuePtr;
		UnboundRecord = FALSE;
		break;
	case SQL_DESC_DATETIME_INTERVAL_CODE:
		m_DescDatetimeIntervalCode = (SQLSMALLINT)ValuePtr;
		if ( m_DescDatetimeIntervalCode == SQL_CODE_DATE)
		{
			m_DescType = SQL_DATETIME;
			m_DescConciseType = SQL_TYPE_DATE;
			m_DescDatetimeIntervalPrecision = 0;
			m_DescLength = 10;
			m_DescPrecision = 0;
			strcpy((char*)m_DescTypeName,"DATE");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_TIME)
		{
			m_DescType = SQL_DATETIME;
			m_DescConciseType = SQL_TYPE_TIME;
			m_DescDatetimeIntervalPrecision = 0;
			m_DescLength = 8;
			m_DescPrecision = 0;
			strcpy((char*)m_DescTypeName,"TIME");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_TIMESTAMP)
		{
			m_DescType = SQL_DATETIME;
			m_DescConciseType = SQL_TYPE_TIMESTAMP;
			strcpy((char*)m_DescTypeName,"TIMESTAMP");
		}else if ( m_DescDatetimeIntervalCode == SQL_CODE_MONTH)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_MONTH;
			strcpy((char*)m_DescTypeName,"INTERVAL MONTH");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_YEAR)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_YEAR;
			strcpy((char*)m_DescTypeName,"INTERVAL YEAR");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_YEAR_TO_MONTH)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_YEAR_TO_MONTH;
			strcpy((char*)m_DescTypeName,"INTERVAL YEAR TO MONTH");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_DAY)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_DAY;
			strcpy((char*)m_DescTypeName,"INTERVAL DAY");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_HOUR)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_HOUR;
			strcpy((char*)m_DescTypeName,"INTERVAL HOUR");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_MINUTE)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_MINUTE;
			strcpy((char*)m_DescTypeName,"INTERVAL MINUTE");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_SECOND)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_SECOND;
			strcpy((char*)m_DescTypeName,"INTERVAL SECOND");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_DAY_TO_HOUR)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_DAY_TO_HOUR;
			strcpy((char*)m_DescTypeName,"INTERVAL DAY TO HOUR");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_DAY_TO_MINUTE)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_DAY_TO_MINUTE;
			strcpy((char*)m_DescTypeName,"INTERVAL DAY TO MINUTE");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_DAY_TO_SECOND)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_DAY_TO_SECOND;
			strcpy((char*)m_DescTypeName,"INTERVAL DAY TO SECOND");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_HOUR_TO_MINUTE)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_HOUR_TO_MINUTE;
			strcpy((char*)m_DescTypeName,"INTERVAL HOUR TO MINUTE");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_HOUR_TO_SECOND)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_HOUR_TO_SECOND;
			strcpy((char*)m_DescTypeName,"INTERVAL HOUR TO SECOND");
		} else if ( m_DescDatetimeIntervalCode == SQL_CODE_MINUTE_TO_SECOND)
		{
			m_DescType = SQL_INTERVAL;
			m_DescConciseType = SQL_INTERVAL_MINUTE_TO_SECOND;
			strcpy((char*)m_DescTypeName,"INTERVAL MINUTE TO SECOND");
		}
		break;
	case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		m_DescDatetimeIntervalPrecision = (SQLINTEGER)ValuePtr;
		break;
	case SQL_DESC_INDICATOR_PTR:
		m_DescIndicatorPtr = (SQLLEN *)ValuePtr;
		UnboundRecord = FALSE;
		break;
	case SQL_DESC_LENGTH:
		m_DescLength = (SQLUINTEGER)ValuePtr;
		break;
	case SQL_DESC_NAME:
		if (BufferLength != SQL_NTS)
			strLen	= BufferLength;
		else
			strLen = strlen((const char *)ValuePtr);
		if (strLen >= sizeof(m_DescName))
			retCode = IDS_22_001;
		else
		{	
			// Note that we don't do any error checking here! All the in/out length checks should have happned before!
			// translate from WChar to UTF8 here
			char transError[MAX_TRANSLATE_ERROR_MSG_LEN];
			int	 transLen = 0;
			WCharToUTF8((wchar_t*)ValuePtr, strLen, (char*)m_DescName, sizeof(m_DescName), &transLen, transError);
		}
		if (m_DescName[0] == 0)
			m_DescUnnamed = SQL_UNNAMED;
		else
			m_DescUnnamed = SQL_NAMED;
		break;
	case SQL_DESC_NUM_PREC_RADIX:
		m_DescNumPrecRadix = (SQLINTEGER)ValuePtr;
		break;
	case SQL_DESC_OCTET_LENGTH:
		m_DescOctetLength = (SQLINTEGER)ValuePtr;
		break;
	case SQL_DESC_OCTET_LENGTH_PTR:
		m_DescOctetLengthPtr = (SQLLEN *)ValuePtr;
		UnboundRecord = FALSE;
		break;
	case SQL_DESC_PARAMETER_TYPE:
		m_DescParameterType = (SQLSMALLINT)ValuePtr;
		break;
	case SQL_DESC_PRECISION:
		m_DescPrecision = (SQLSMALLINT)ValuePtr;
		break;
	case SQL_DESC_SCALE:
		m_DescScale = (SQLSMALLINT)ValuePtr;
		break;
	case SQL_DESC_UNNAMED:
		value = (SQLSMALLINT)ValuePtr;
		if ((value != SQL_NAMED && value != SQL_UNNAMED) || (value == SQL_NAMED && m_DescName[0] == 0 ))
			retCode = IDS_HY_092;
		else {
			if ((m_DescUnnamed = value) == SQL_UNNAMED)
				m_DescName[0] = 0;
		}
		break;
	case SQL_DESC_AUTO_UNIQUE_VALUE:
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CASE_SENSITIVE:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_DISPLAY_SIZE:
	case SQL_DESC_FIXED_PREC_SCALE:
	case SQL_DESC_LABEL:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NULLABLE:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_SEARCHABLE:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_TYPE_NAME:
	case SQL_DESC_UNSIGNED:
	case SQL_DESC_UPDATABLE:
		UnboundRecord = FALSE;
		retCode = IDS_HY_091;
		break;
	}
	if (UnboundRecord)
		m_DescDataPtr = NULL;
	else if (retCode == SQL_SUCCESS)
		retCode = checkConsistency(DescMode);
	return retCode;
}

void CDescRec::setTranslateOption(SQLSMALLINT CDataType)
{
	CConnect *connectHandle = m_DescDesc->getDescConnect();
	if (connectHandle != NULL) 
	{
		DWORD drvrCharset = (CDataType == SQL_C_WCHAR) ? 0x00000002L /*cnv_UTF16*/ : connectHandle->getDrvrCharSet();
		DWORD sqlCharset  = connectHandle->getSqlCharSet(m_SQLCharset);

		if (drvrCharset == sqlCharset)
		{
			m_translateOptionFromSQL = 0;
			m_translateOptionToSQL = 0;
		}
		else
		{
			m_translateOptionFromSQL = (sqlCharset << 16) + drvrCharset;
			m_translateOptionToSQL   = (drvrCharset << 16) + sqlCharset;
		}
	}
}

// Implementation for the member functions of CDesc

CDesc::CDesc(SQLHANDLE InputHandle, short DescMode, SQLSMALLINT DescAllocType,  CStmt *pStmt) : 
	CHandle(SQL_HANDLE_DESC, InputHandle)
{

	m_DescMode	= DescMode;
	m_DescAllocType = DescAllocType;
	m_DescArraySize = 1;	
	m_DescArrayStatusPtr = NULL;
	m_DescBindOffsetPtr = NULL;
	m_DescBindType = SQL_BIND_BY_COLUMN;
	m_DescCount = 0;
	m_DescRowsProcessedPtr = NULL;
	m_DescRecLength = 0;
	m_AssociatedStmt = pStmt;
	m_ConnectHandle = (CConnect *)InputHandle;
	m_replacementChar = m_ConnectHandle->getReplacementChar(); 
	m_ODBCAppVersion = m_ConnectHandle->getODBCAppVersion(); 
	m_drvrCharSet = getDrvrCharSet(); 
	m_DescRecCollect.reserve(100);
}

CDesc::~CDesc()
{
	CHANDLECOLLECT::iterator i;

	// Remove this from DescCollection in Connection
	EnterCriticalSection(&m_ConnectHandle->m_CSObject);
	for (i = m_ConnectHandle->m_DescCollect.begin() ; i !=  m_ConnectHandle->m_DescCollect.end() ; ++i)
	{
		if ((*i) == this)
		{
			m_ConnectHandle->m_DescCollect.erase(i);
			break;
		}
	}
	LeaveCriticalSection(&m_ConnectHandle->m_CSObject);
	clear();
}

void CDesc::clear()
{

	CDESCRECLIST::iterator i;
//	Need to comment since SetStmtAttributes where getting reset.
//	m_DescArraySize = 1;	
//	m_DescArrayStatusPtr = NULL;
//	m_DescBindOffsetPtr = NULL;
//	m_DescBindType = SQL_BIND_BY_COLUMN;
//	m_DescRowsProcessedPtr = NULL;
	m_DescCount = 0;
	
	for (i = m_DescRecCollect.begin() ; i != m_DescRecCollect.end() ; ++i)
		delete (*i);
	m_DescRecCollect.clear();
}

void CDesc::deleteRecords()
{

	CDESCRECLIST::iterator i;

	m_DescCount = 0;
	
	for (i = m_DescRecCollect.begin() ; i != m_DescRecCollect.end() ; ++i)
		delete (*i);
	m_DescRecCollect.clear();
}

SQLRETURN CDesc::SetDescRec(SQLSMALLINT	RecNumber,
					SQLSMALLINT	Type,
					SQLSMALLINT	SubType,
					SQLLEN		Length,
					SQLSMALLINT	Precision,
					SQLSMALLINT	Scale,
					SQLPOINTER	DataPtr,
					SQLLEN		*StringLengthPtr,
					SQLLEN		*IndicatorPtr)
{
	CDescRec		*descRecPtr;
	unsigned long	retCode;
	SQLRETURN		rc = SQL_SUCCESS;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLSETDESCREC;
	if (m_DescMode == SQL_ATTR_IMP_ROW_DESC)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_016);
		return SQL_ERROR;
	}
	if (RecNumber <= 0)		// 0 - Bookmark Record not supported
	{
		setDiagRec(DRIVER_ERROR, IDS_07_009);
		return SQL_ERROR;
	}
	retCode = SetDescField(RecNumber);
	if (retCode != SQL_SUCCESS)
	{
		setDiagRec(DRIVER_ERROR, retCode);
		return SQL_ERROR;
	}
	descRecPtr = m_DescRecCollect[RecNumber-1];
	if ((retCode = descRecPtr->setDescRec(m_DescMode, Type, SubType, Length, Precision, Scale, DataPtr, 
			StringLengthPtr, IndicatorPtr)) != SQL_SUCCESS)
	{
		setDiagRec(DRIVER_ERROR, retCode);
		rc = SQL_ERROR;
	}
	return rc;
}

unsigned long CDesc::BindCol(SQLSMALLINT ColumnNumber,
					SQLSMALLINT TargetType,
					SQLPOINTER TargetValuePtr,
					SQLLEN BufferLength,
					SQLLEN *StrLen_or_IndPtr)
{
	CDescRec		*descRecPtr;
	unsigned long	retCode;

	if (ColumnNumber <= 0)		// 0 - Bookmark Record not supported
	{
		return IDS_07_009;
	}
	retCode = SetDescField(ColumnNumber);
	if (retCode != SQL_SUCCESS)
		return retCode;
	descRecPtr = m_DescRecCollect[ColumnNumber-1];
	retCode = descRecPtr->setDescRec(m_DescMode, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
	return retCode;
}

unsigned long CDesc::BindParameter(SQLSMALLINT ParameterNumber, 
			SQLSMALLINT InputOutputType,
			SQLSMALLINT ValueType,
			SQLPOINTER  ParameterValuePtr,
			SQLLEN	BufferLength,
			SQLLEN *StrLen_or_IndPtr)
{
	CDescRec		*descRecPtr;
	unsigned long	retCode;

	retCode = SetDescField(ParameterNumber);
	if (retCode != SQL_SUCCESS)
		return retCode;
	descRecPtr = m_DescRecCollect[ParameterNumber-1];
	return (descRecPtr->BindParameter(m_DescMode, InputOutputType, ValueType, ParameterValuePtr, 
					BufferLength, StrLen_or_IndPtr));
}

unsigned long CDesc::BindParameter(SQLSMALLINT ParameterNumber, 
					SQLSMALLINT ParameterType, 
					SQLULEN		ColumnSize,
					SQLSMALLINT DecimalDigits)
{
	CDescRec		*descRecPtr;
	unsigned long	retCode;

	retCode = SetDescField(ParameterNumber);
	if (retCode != SQL_SUCCESS)
		return retCode;
	descRecPtr = m_DescRecCollect[ParameterNumber-1];
	return (descRecPtr->BindParameter(m_DescMode, ParameterType, ColumnSize, DecimalDigits));
}

unsigned long CDesc::setDescRec(const SQLItemDescList_def *SQLDesc)
{
	
	unsigned long	retCode = SQL_SUCCESS;
	unsigned long	retCode1;
	short			curDescRec;
	SQLItemDesc_def	*SQLItemDesc;
	CDescRec		*descRecPtr;
	SQLSMALLINT		DescCount;
	
	// Clear the previous Descriptor Information
	deleteRecords();
	// Increase the Descriptor Rec
	DescCount = SQLDesc->_length;
	retCode = SetDescField(DescCount);
	if (retCode != SQL_SUCCESS)
		return retCode;
	for (curDescRec = 0, m_DescRecLength = 0 ; curDescRec < SQLDesc->_length ; curDescRec++)
	{
		SQLItemDesc = (SQLItemDesc_def *)SQLDesc->_buffer + curDescRec;
	
		m_DescRecLength += SQLItemDesc->maxLen;
		descRecPtr = m_DescRecCollect[curDescRec];
		if ((retCode1 = descRecPtr->setDescRec(m_DescMode, SQLItemDesc)) != SQL_SUCCESS)
			retCode = retCode1;
	}
	return retCode;
}
	
SQLRETURN CDesc::GetDescRec(SQLSMALLINT	RecNumber, 
					SQLWCHAR *NameW,
					SQLSMALLINT	BufferLength,
					SQLSMALLINT	*StringLengthPtr,
					SQLSMALLINT	*TypePtr,
					SQLSMALLINT	*SubTypePtr,
					SQLLEN		*LengthPtr,
					SQLSMALLINT	*PrecisionPtr,
					SQLSMALLINT	*ScalePtr,
					SQLSMALLINT	*NullablePtr)
{
	CDescRec		*descRecPtr;
	SQLSMALLINT		strLen;
	SQLRETURN		rc=SQL_SUCCESS;
	short			stmtState;
	short           stmtQueryType;
	unsigned long	retCode;
	int				translen = 0;
	int				translateLengthMax = 0;
	char			transError[MAX_TRANSLATE_ERROR_MSG_LEN];

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLGETDESCREC;
	if (m_DescMode == SQL_ATTR_IMP_ROW_DESC)
	{
		if (m_AssociatedStmt != NULL)
		{
			stmtState = m_AssociatedStmt->getStmtState();
			stmtQueryType = m_AssociatedStmt->getStmtQueryType();
		    if ((stmtState== STMT_EXECUTED_NO_RESULT) && 
                (stmtQueryType != SQL_SELECT_UNIQUE))
			{
				setDiagRec(DRIVER_ERROR, IDS_24_000);
				return SQL_ERROR;
			}
			if (stmtState == STMT_PREPARED_NO_RESULT)
				return SQL_NO_DATA;
		}
	}
	if (RecNumber <= 0) // BookMark Record
	{
		setDiagRec(DRIVER_ERROR, IDS_07_009);
		return SQL_ERROR;
	}
	if (RecNumber > m_DescRecCollect.size())
		return SQL_NO_DATA;
	descRecPtr = m_DescRecCollect[RecNumber-1];
	strLen = strlen((char *)descRecPtr->m_DescName);
	if (NameW != NULL)
	{
		if (BufferLength <= 0 && BufferLength != SQL_NTS)
		{
			setDiagRec(DRIVER_ERROR, IDS_HY_090);
			return SQL_ERROR;
		}
		else
		{
			if(strLen > 0) // translate from UTF8 to WChar
			{
				translateLengthMax = (BufferLength == SQL_NTS) ? strLen : BufferLength;
				if((rc = UTF8ToWChar((char*)descRecPtr->m_DescName, strLen, (wchar_t *)NameW, translateLengthMax,
									 &translen, transError)) == SQL_ERROR)
				{
					return IDS_190_DSTODRV_ERROR;
				}
				else
					strLen = translen;
			}
			else
				*((wchar_t *)NameW) = L'\0';
		}
	}
	if (*StringLengthPtr != NULL)
		*StringLengthPtr = strLen;
	if (TypePtr != NULL)
		*TypePtr = descRecPtr->m_DescType;
	if (SubTypePtr != NULL)
		*SubTypePtr = descRecPtr->m_DescDatetimeIntervalCode;
	if (LengthPtr != NULL)
		*LengthPtr = descRecPtr->m_DescOctetLength;
	if (PrecisionPtr != NULL)
		*PrecisionPtr = descRecPtr->m_DescPrecision;
	if (ScalePtr != NULL)
		*ScalePtr = descRecPtr->m_DescScale;
	if (NullablePtr != NULL)
		*NullablePtr = descRecPtr->m_DescNullable;
	return rc;
}

SQLRETURN CDesc::SetDescField(SQLSMALLINT RecNumber,
					SQLSMALLINT FieldIdentifier,
					SQLPOINTER ValuePtr,
					SQLINTEGER BufferLength)
{
	SQLRETURN		rc = SQL_SUCCESS;
	SQLSMALLINT		descValue;
	short			i;
	CDescRec		*descRecPtr;
	unsigned long	retCode;
	CDESCRECLIST::reverse_iterator		rit;

	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLSETDESCFIELD;
	
	for (i=0; gDescSetTypes[i].FieldIdentifier != 9999 && 
			gDescSetTypes[i].FieldIdentifier != FieldIdentifier ; i++);

	if (gDescSetTypes[i].FieldIdentifier != 9999)
	{
		switch (m_DescMode)
		{
		case SQL_ATTR_IMP_ROW_DESC:
			if (gDescSetTypes[i].IRDDescType != RW_DESC_TYPE)
				rc = SQL_ERROR;
			break;
		case SQL_ATTR_IMP_PARAM_DESC:
			if (FieldIdentifier != SQL_DESC_DATA_PTR && gDescSetTypes[i].IPDDescType != RW_DESC_TYPE)
				rc = SQL_ERROR;
			break;
		case SQL_ATTR_APP_ROW_DESC:
			if (gDescSetTypes[i].ARDDescType != RW_DESC_TYPE)
				rc = SQL_ERROR;
			break;
		case SQL_ATTR_APP_PARAM_DESC:
			if (gDescSetTypes[i].APDDescType != RW_DESC_TYPE)
				rc = SQL_ERROR;
			break;
		case SQL_DESC_MODE_UNKNOWN:
			if (gDescSetTypes[i].ARDDescType != RW_DESC_TYPE &&  
					gDescSetTypes[i].APDDescType != RW_DESC_TYPE) 
				rc = SQL_ERROR;
			break;
		default:
			setDiagRec(DRIVER_ERROR, IDS_HY_C00);
			return SQL_ERROR;
			break;
		}
	}
	else
		rc = SQL_ERROR;
	if (rc == SQL_ERROR)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_091);
		return SQL_ERROR;
	}
	switch (FieldIdentifier)
	{
	case SQL_DESC_ALLOC_TYPE:
		break;
	case SQL_DESC_ARRAY_SIZE:
		m_DescArraySize = (SQLULEN)ValuePtr;
		break;
	case SQL_DESC_ARRAY_STATUS_PTR:
		m_DescArrayStatusPtr = (SQLUSMALLINT *)ValuePtr;
		break;
	case SQL_DESC_BIND_OFFSET_PTR:
		m_DescBindOffsetPtr = (SQLLEN *)ValuePtr;
		break;
	case SQL_DESC_BIND_TYPE:
		m_DescBindType = (SQLINTEGER)ValuePtr;
		break;
	case SQL_DESC_COUNT:
		descValue = (SQLSMALLINT)ValuePtr;
		if (descValue > m_DescCount)
		{
			retCode = SetDescField(descValue);
			if (retCode != SQL_SUCCESS)
			{
				setDiagRec(DRIVER_ERROR, retCode);
				rc = SQL_ERROR;
			}
		}
		else
		{
			switch (m_DescMode)
			{
			case SQL_ATTR_APP_ROW_DESC:
			case SQL_ATTR_APP_PARAM_DESC:
			case SQL_DESC_MODE_UNKNOWN:
				while(descValue < m_DescCount)
				{
					delete m_DescRecCollect.back();
					m_DescRecCollect.pop_back();
					m_DescCount--;
				}
			case SQL_ATTR_IMP_ROW_DESC:
			case SQL_ATTR_IMP_PARAM_DESC:
			default:
				break;
			}
		}
		break;
	case SQL_DESC_ROWS_PROCESSED_PTR:
		m_DescRowsProcessedPtr = (SQLULEN *)ValuePtr;
		break;
	default:
		// Setting the Record Fields
		if (RecNumber <= 0)
		{
			setDiagRec(DRIVER_ERROR, IDS_07_009);
			return SQL_ERROR;
		}
		// Increase the descriptor records to RecNumber
		retCode = SetDescField(RecNumber);
		if (retCode != SQL_SUCCESS)
		{
			setDiagRec(DRIVER_ERROR, retCode);
			rc = SQL_ERROR;
		}
		else
		{
			descRecPtr = m_DescRecCollect[RecNumber-1];
			retCode = descRecPtr->SetDescField(m_DescMode, FieldIdentifier, ValuePtr, BufferLength);
			if (retCode != SQL_SUCCESS)
			{
				setDiagRec(DRIVER_ERROR, retCode);
				rc = SQL_ERROR;
			}
		}
		break;
	}
	return rc;
}

unsigned long CDesc::SetDescField(SQLSMALLINT RecNumber)
{
	CDescRec	*descRecPtr;

	if (RecNumber > m_DescRecCollect.size())
	{
		for ( ; m_DescRecCollect.size() < RecNumber ;)
		{
			descRecPtr = new CDescRec(m_DescMode, this);
			if (descRecPtr == NULL)
				return IDS_HY_001;
			m_DescRecCollect.push_back(descRecPtr);
			m_DescCount++;
		}
	}
	return SQL_SUCCESS;
}		

SQLRETURN CDesc::GetDescField(SQLSMALLINT RecNumber,
					SQLSMALLINT FieldIdentifier,
					SQLPOINTER ValuePtr,
					SQLINTEGER BufferLength,
					SQLINTEGER *StringLengthPtr)
{
	CDescRec			*descRecPtr;
	SQLRETURN			rc = SQL_SUCCESS;
	RETURN_VALUE_STRUCT	retValue;
	short				stmtState;
	short               stmtQueryType;
	retValue.dataType = DRVR_PENDING;
	retValue.u.strPtr = NULL;
		
	clearError();
	m_CurrentOdbcAPI = SQL_API_SQLGETDESCFIELD;
	if (m_DescMode == SQL_ATTR_IMP_ROW_DESC)
	{
		if (m_AssociatedStmt != NULL)
		{
			stmtState = m_AssociatedStmt->getStmtState();
			stmtQueryType = m_AssociatedStmt->getStmtQueryType();
		    if ((stmtState == STMT_EXECUTED_NO_RESULT) && 
                (stmtQueryType != SQL_SELECT_UNIQUE))
			{
				setDiagRec(DRIVER_ERROR, IDS_24_000);
				return SQL_ERROR;
			}
			if (stmtState == STMT_PREPARED_NO_RESULT)
				return SQL_NO_DATA;
		}
	}
	switch (FieldIdentifier)
	{
	case SQL_DESC_ALLOC_TYPE:
		retValue.u.s16Value = m_DescAllocType;
		retValue.dataType = SQL_IS_SMALLINT;
		break;
	case SQL_DESC_ARRAY_SIZE:
		#ifdef _WIN64
			retValue.u.s64Value = m_DescArraySize;
			retValue.dataType = SQL_C_SBIGINT;
		#else
			retValue.u.u32Value = m_DescArraySize;
			retValue.dataType = SQL_IS_UINTEGER;
		#endif
		break;
	case SQL_DESC_ARRAY_STATUS_PTR:
		retValue.u.pValue = m_DescArrayStatusPtr;
		retValue.dataType = SQL_IS_POINTER;
		break;
	case SQL_DESC_BIND_OFFSET_PTR:
		retValue.u.pValue = m_DescBindOffsetPtr;
		retValue.dataType = SQL_IS_POINTER;
		break;
	case SQL_DESC_BIND_TYPE:
		retValue.u.s32Value = m_DescBindType;
		retValue.dataType = SQL_IS_INTEGER;
		break;
	case SQL_DESC_COUNT:
		retValue.u.s16Value = m_DescCount;	
		retValue.dataType = SQL_IS_SMALLINT;
		break;
	case SQL_DESC_ROWS_PROCESSED_PTR:
		retValue.u.pValue = m_DescRowsProcessedPtr;
		retValue.dataType = SQL_IS_POINTER;
		break;
	default:
		if (RecNumber <= 0)		// BOOKMARK fields are not supported
		{
			setDiagRec(DRIVER_ERROR, IDS_07_009);
			rc = SQL_ERROR;
			break;
		}
		if (RecNumber > m_DescRecCollect.size())
		{
			rc = SQL_NO_DATA;
			break;
		}
		descRecPtr = m_DescRecCollect[RecNumber-1];
		switch (FieldIdentifier)
		{
		case SQL_DESC_AUTO_UNIQUE_VALUE:
			retValue.u.s32Value = descRecPtr->m_DescAutoUniqueValue;
			retValue.dataType = SQL_IS_INTEGER;
			break;
		case SQL_DESC_CASE_SENSITIVE:
			retValue.u.s32Value = descRecPtr->m_DescCaseSensitive;
			retValue.dataType = SQL_IS_INTEGER;
			break;
		case SQL_DESC_CONCISE_TYPE:
			retValue.u.s16Value = descRecPtr->m_DescConciseType;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_BASE_COLUMN_NAME:
			retValue.u.strPtr = (char *)descRecPtr->m_DescBaseColumnName;
			break;
		case SQL_DESC_BASE_TABLE_NAME:
			retValue.u.strPtr = (char *)descRecPtr->m_DescBaseTableName;
			break;
		case SQL_DESC_CATALOG_NAME:
			retValue.u.strPtr = (char *)descRecPtr->m_DescCatalogName;
			break;
		case SQL_DESC_DATA_PTR:
			retValue.u.pValue = descRecPtr->m_DescDataPtr;
			retValue.dataType = SQL_IS_POINTER;
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			retValue.u.s16Value = descRecPtr->m_DescDatetimeIntervalCode;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
			retValue.u.s32Value = descRecPtr->m_DescDatetimeIntervalPrecision;
			retValue.dataType = SQL_IS_INTEGER;
			break;
		case SQL_DESC_DISPLAY_SIZE:
			retValue.u.s32Value = descRecPtr->m_DescDisplaySize;
			retValue.dataType = SQL_IS_INTEGER;
			break;
		case SQL_DESC_FIXED_PREC_SCALE:
			retValue.u.s16Value = descRecPtr->m_DescFixedPrecScale;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_INDICATOR_PTR:
			retValue.u.pValue = descRecPtr->m_DescIndicatorPtr;
			retValue.dataType = SQL_IS_POINTER;
			break;
		case SQL_DESC_LABEL:
			if (descRecPtr->m_DescLabel[0] != '\0')
				retValue.u.strPtr = (char *)descRecPtr->m_DescLabel;
			else
				retValue.u.strPtr = (char *)descRecPtr->m_DescName;
			break;
		case SQL_DESC_LENGTH:
			retValue.u.u32Value = descRecPtr->m_DescLength;
			retValue.dataType = SQL_IS_UINTEGER;
			break;
		case SQL_DESC_LITERAL_PREFIX:
			retValue.u.strPtr = (char *)descRecPtr->m_DescLiteralPrefix;
			break;
		case SQL_DESC_LITERAL_SUFFIX:
			retValue.u.strPtr = (char *)descRecPtr->m_DescLiteralSuffix;
			break;
		case SQL_DESC_LOCAL_TYPE_NAME:
			retValue.u.strPtr = (char *)descRecPtr->m_DescLocalTypeName;
			break;
		case SQL_DESC_NAME:
			retValue.u.strPtr = (char *)descRecPtr->m_DescName;
			break;
		case SQL_DESC_NULLABLE:
			retValue.u.u16Value = descRecPtr->m_DescNullable;
			retValue.dataType = SQL_IS_USMALLINT;
			break;
		case SQL_DESC_NUM_PREC_RADIX:
			retValue.u.s32Value = descRecPtr->m_DescNumPrecRadix;
			retValue.dataType = SQL_IS_INTEGER;
			break;
		case SQL_DESC_OCTET_LENGTH:
			switch (descRecPtr->m_DescConciseType)
			{
				case SQL_WCHAR:
				case SQL_WVARCHAR:
					retValue.u.u32Value = descRecPtr->m_DescOctetLength * 2;
					break;
				default :
					retValue.u.s32Value = descRecPtr->m_DescOctetLength;
					break;
			}
			retValue.dataType = SQL_IS_INTEGER;
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			retValue.u.pValue = descRecPtr->m_DescOctetLengthPtr;
			retValue.dataType = SQL_IS_POINTER;
			break;
		case SQL_DESC_PARAMETER_TYPE:
			retValue.u.s16Value = descRecPtr->m_DescParameterType;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_PRECISION:
			retValue.u.s16Value = descRecPtr->m_DescPrecision;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_SCALE:
			retValue.u.s16Value = descRecPtr->m_DescScale;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_SCHEMA_NAME:
			retValue.u.strPtr = (char *)descRecPtr->m_DescSchemaName;
			break;
		case SQL_DESC_SEARCHABLE:
			retValue.u.s16Value = descRecPtr->m_DescSearchable;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_TABLE_NAME:
			retValue.u.strPtr = (char *)descRecPtr->m_DescTableName;
			break;
		case SQL_DESC_TYPE:
			retValue.u.s16Value = descRecPtr->m_DescType;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_TYPE_NAME:
			retValue.u.strPtr = (char *)descRecPtr->m_DescTypeName;
			break;
		case SQL_DESC_UNNAMED:
			retValue.u.s16Value = descRecPtr->m_DescUnnamed;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_UNSIGNED:
			retValue.u.s16Value = descRecPtr->m_DescUnsigned;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_DESC_UPDATABLE:
			retValue.u.s16Value = descRecPtr->m_DescUpdatable;
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		case SQL_COLUMN_LENGTH:
			switch (descRecPtr->m_DescConciseType)
			{
			case SQL_NUMERIC:
			case SQL_DECIMAL:
				retValue.u.u32Value = descRecPtr->m_DescPrecision+2;
				break;
			case SQL_WCHAR:
			case SQL_WVARCHAR:
				retValue.u.u32Value = descRecPtr->m_DescOctetLength * 2;
				break;
			default:
				retValue.u.u32Value = descRecPtr->m_DescOctetLength;
				break;
			}
			retValue.dataType = SQL_IS_UINTEGER;
			break;
		case SQL_COLUMN_PRECISION:
			switch (descRecPtr->m_DescConciseType)
			{
			case SQL_CHAR:
			case SQL_VARCHAR:
			case SQL_LONGVARCHAR:
			case SQL_WCHAR:
			case SQL_WVARCHAR:
			case SQL_DATE:
			case SQL_TYPE_DATE:
			case SQL_TIME:
			case SQL_TYPE_TIME:
			case SQL_TIMESTAMP:
			case SQL_TYPE_TIMESTAMP:			// We may do for Interval DataTypes also
			case SQL_INTERVAL_MONTH:
			case SQL_INTERVAL_YEAR:
			case SQL_INTERVAL_YEAR_TO_MONTH:
			case SQL_INTERVAL_DAY:
			case SQL_INTERVAL_HOUR:
			case SQL_INTERVAL_MINUTE:
			case SQL_INTERVAL_SECOND:
			case SQL_INTERVAL_DAY_TO_HOUR:
			case SQL_INTERVAL_DAY_TO_MINUTE:
			case SQL_INTERVAL_DAY_TO_SECOND:
			case SQL_INTERVAL_HOUR_TO_MINUTE:
			case SQL_INTERVAL_HOUR_TO_SECOND:
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				retValue.u.u32Value = descRecPtr->m_DescLength;
				break;
			default:
				retValue.u.u32Value = descRecPtr->m_DescPrecision;
				break;
			}
			retValue.dataType = SQL_IS_UINTEGER;
			break;
		case SQL_COLUMN_SCALE:
			switch (descRecPtr->m_DescConciseType)
			{
			case SQL_TIMESTAMP:
			case SQL_TYPE_TIMESTAMP:			// We may do for Interval DataTypes also
			case SQL_INTERVAL_SECOND:
			case SQL_INTERVAL_DAY_TO_SECOND:
			case SQL_INTERVAL_HOUR_TO_SECOND:
			case SQL_INTERVAL_MINUTE_TO_SECOND:
				retValue.u.s16Value = descRecPtr->m_DescPrecision;
				break;
			default:
				retValue.u.s16Value = descRecPtr->m_DescScale;
				break;
			}
			retValue.dataType = SQL_IS_SMALLINT;
			break;
		default:
			setDiagRec(DRIVER_ERROR, IDS_HY_091);
			break;
		}
	}
	if (rc == SQL_SUCCESS) 
	{
		rc = returnAttrValue(TRUE, this, &retValue, ValuePtr, BufferLength, StringLengthPtr);
	}
	return rc;
}


unsigned long CDesc::getDescRec(SQLUSMALLINT ColumnNumber, 
			SQLWCHAR *ColumnName,
			SQLSMALLINT BufferLength, 
			SQLSMALLINT *NameLengthPtr,
		    SQLSMALLINT *DataTypePtr, 
			SQLULEN *ColumnSizePtr,
			SQLSMALLINT *DecimalDigitsPtr,
			SQLSMALLINT *NullablePtr,
			UCHAR		*errorMsg,
			SWORD		errorMsgMax)
{

	CDescRec		*descRecPtr;
	SQLSMALLINT		strLen;
	BOOL			found;
	short			i;
	SQLSMALLINT		dataType;
	unsigned long	retCode;
	int				translen = 0;
	int				translateLengthMax = 0;
	char			transError[MAX_TRANSLATE_ERROR_MSG_LEN];

	if (ColumnNumber > m_DescRecCollect.size())
		return IDS_07_009;
	descRecPtr = m_DescRecCollect[ColumnNumber-1];
	strLen = strlen((char *)descRecPtr->m_DescName);
	if (ColumnName != NULL)
	{
		if (BufferLength <= 0 && BufferLength != SQL_NTS)
			return IDS_HY_090;
		else									
		{
			// translate from UTF8 to WChar
			if(strLen > 0) // translate from UTF8 to WChar
			{
				translateLengthMax = (BufferLength == SQL_NTS) ? strLen : BufferLength;
				if(UTF8ToWChar((char*)descRecPtr->m_DescName, strLen, (wchar_t *)ColumnName, translateLengthMax,
								&translen, transError) == SQL_ERROR)
				{
					return IDS_190_DSTODRV_ERROR;
				}
				else
					strLen = translen;
			}
			else
				*((wchar_t *)ColumnName) = L'\0';
		}
	}
	if (NameLengthPtr != NULL)
		*NameLengthPtr = strLen;

	for (i = 0, found = FALSE; gSQLDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
	{
		if (gSQLDatatypeMap[i].conciseType == descRecPtr->m_DescConciseType)
		{
			found = TRUE;
			break;
		}
	}
	if (! found)
		dataType = SQL_UNKNOWN_TYPE;
	else
		dataType = descRecPtr->m_DescConciseType;
		
	if (DataTypePtr != NULL)
		*DataTypePtr = descRecPtr->m_DescConciseType;
	
	if (ColumnSizePtr != NULL)
	{
		switch (gSQLDatatypeMap[i].columnSizeAttr)
		{
		case SQL_DESC_LENGTH:
			*ColumnSizePtr = descRecPtr->m_DescLength;
			break;
		case SQL_DESC_PRECISION:
			*ColumnSizePtr = descRecPtr->m_DescPrecision;
			break;
		default:
			*ColumnSizePtr = 0;
			break;
		}
	}
	
	if (DecimalDigitsPtr != NULL)
	{
		switch (gSQLDatatypeMap[i].decimalDigitsAttr)
		{
		case SQL_DESC_PRECISION:
			*DecimalDigitsPtr = descRecPtr->m_DescPrecision;
			break;
		case SQL_DESC_SCALE:
			*DecimalDigitsPtr = descRecPtr->m_DescScale;
			break;
		default:
			*DecimalDigitsPtr = 0;
			break;
		}
	}
	if (NullablePtr != NULL)
		*NullablePtr = descRecPtr->m_DescNullable;
	return SQL_SUCCESS;
}

//	Function : BuildValueList
//	Parameters:	ParamBuffer	-	This is a pointer to ParamBuffers for all rows and Params
//				InputValueList - ValueList which is shipped to the server
//	Called from Execute, ParamData of CStmt member functions
//  Processing Sequence:
//		Assumptions:	SQLExecute deletes the InputValueList when needed
//						InputValueList->_buffer is NULL, when called from Execute
//						InputValueList->_buffer is already initialized when called from ParamData
//	When the InputValueList->_buffer is NULL
//		Calculate the Param Buffer Size needed for all rows and params adjusting it to 8 BYTE boundary
//		Allocate the ParamBuffer in chunk.
//		Assign from this Param Buffer to individual Params in InputValueList. This should never be nulled
//		out in any case. The length can be used to ship the value buffer if needed by Krypton
// For the rest of procesing refer SQLBindParameter in ODBC 3.0 Manual

SQLRETURN CDesc::BuildValueList(CStmt *pHandle,  //AMR - changed this pointer type to CStmt*, (from CHandle *) in order to access m_RowId member of CStmt object
			SQLSMALLINT				ImpParamCount,
			CDesc					*IPD,
			SQLINTEGER				ODBCAppVersion,
			SQLSMALLINT				&ParamNumber, 
			SQLULEN					&RowNumber,
			SQLValueList_def		*InputValueList,
			BYTE					*&ParamBuffer)
{
	SQLRETURN		rc = SQL_SUCCESS;
	SQLRETURN		RowStatus;
	unsigned long	numValues;
	unsigned long	ParamCount;
	SQLULEN			RowCount;
	unsigned long	retCode;
	SQLValue_def	*SQLValue;
	CDescRec		*descRecPtr;
	CDescRec		*IPDDescRecPtr;
	SQLLEN			*StrLenPtr;
	SQLINTEGER		StrLen;
	SQLUSMALLINT	*ArrayStatusPtr;
	SQLUSMALLINT	*IPDArrayStatusPtr;
	SQLPOINTER		DataPtr;
	SQLSMALLINT		DataType;
	SQLLEN			*IndPtr;
	SQLINTEGER		AllocLength;
	SQLINTEGER		ParamBufferSize;
	BYTE			*memPtr;
	long			memOffSet = 0;
	unsigned long	i;
	UCHAR			errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	BOOL			byteSwap;
	SQLSMALLINT		startParamNumber = ParamNumber; 
	SQLULEN			startRowNumber = RowNumber;

	byteSwap = getByteSwap();
	bool useNAR = true;
	if(InputValueList->_buffer != NULL || m_DescArraySize == 1 || !pHandle->getConnectHandle()->rowsetErrorRecovery() ) 
		useNAR = false;
					
	
	if (InputValueList->_buffer == NULL  && InputValueList->_length == 0)	
	{
		numValues = m_DescArraySize * ImpParamCount;
		InputValueList->_buffer = new SQLValue_def[numValues];
		if (InputValueList->_buffer == NULL)
		{
			pHandle->setDiagRec(DRIVER_ERROR, IDS_HY_001);
			return SQL_ERROR;
		}
		InputValueList->_length = 0;
		// Calculate the Buffer Size for all rows and all params
		for (ParamCount = 0, ParamBufferSize = 0; ParamCount  < ImpParamCount ; ParamCount++)
		{
			IPDDescRecPtr = IPD->m_DescRecCollect[ParamCount];
//			AllocLength =  ((IPDDescRecPtr->m_SQLOctetLength + 8 - 1) >> 3) << 3;	// Adjust it 8 Byte Boundary Length
			AllocLength =  IPDDescRecPtr->m_SQLOctetLength + 1;
			ParamBufferSize += (AllocLength * m_DescArraySize);
		}

		ParamBuffer = new BYTE[ParamBufferSize];
		if (ParamBuffer == NULL)
		{
			pHandle->setDiagRec(DRIVER_ERROR, IDS_HY_001);
			return SQL_ERROR;
		}
		// Assign Buffer Pointers in the SQLValue->dataValue
		for (RowCount = 0, memPtr = ParamBuffer, i = 0; RowCount < m_DescArraySize ; RowCount++)
		{
			for (ParamCount = 0; ParamCount < ImpParamCount && i < numValues; ParamCount++, i++)
			{
				SQLValue = (SQLValue_def *)InputValueList->_buffer + i;
				IPDDescRecPtr = IPD->m_DescRecCollect[ParamCount];
//				memOffSet = ((memOffSet + 8 - 1) >> 3) << 3; 
				SQLValue->dataValue._buffer = memPtr + memOffSet + 1;
				SQLValue->dataValue._length = 0;
				memOffSet += IPDDescRecPtr->m_SQLOctetLength + 1;
			}
		}
		if ( i != numValues)
			return SQL_ERROR;
	}
	unsigned long tempInputValueListLength = InputValueList->_length;
	SQLValue_def *tempSQLValue = SQLValue = (SQLValue_def *)InputValueList->_buffer;
	bool filteredPrevRow = false;
	SQLULEN SqlRowNumber = RowNumber;  //sql row ids start counting from 1, not 0
	pHandle->clearSqlDrvrRowIdMap();
	for (RowCount = RowNumber-1, RowStatus = SQL_PARAM_UNUSED; RowCount < m_DescArraySize ; RowCount++)
	{
		tempInputValueListLength = InputValueList->_length;
		tempSQLValue = SQLValue;
		filteredPrevRow = false;

		if (IPD->m_DescRowsProcessedPtr != NULL)
			*(IPD->m_DescRowsProcessedPtr) = RowCount+1;
	
		if (IPD->m_DescArrayStatusPtr != NULL)
		{
			IPDArrayStatusPtr = IPD->m_DescArrayStatusPtr + RowCount;
			*IPDArrayStatusPtr = RowStatus;
		}
		else
			IPDArrayStatusPtr = NULL;
		// Refer Ignoring a set of Parameters in SQLBindParameter
		if (m_DescArrayStatusPtr != NULL)
		{
			ArrayStatusPtr = m_DescArrayStatusPtr + RowCount;
			switch (*ArrayStatusPtr)
			{
			case SQL_PARAM_PROCEED:
			//case SQL_ROW_PROCEED:
			case SQL_ROW_ADDED:
			//case SQL_ROW_SUCCESS:
			case SQL_ROW_SUCCESS_WITH_INFO:
				break;
			case SQL_ROW_IGNORE:
			default:
				continue;
			}
		}

		for (ParamCount = startParamNumber, RowStatus = SQL_PARAM_SUCCESS;  
					ParamCount < ImpParamCount ; ParamCount++)
		{
				// Update the APD SQL_DESC_ROWS_PROCESSED with the current row count
			if (m_DescRowsProcessedPtr != NULL)
				*m_DescRowsProcessedPtr = ParamCount+1;
	
			SQLValue = (SQLValue_def *)InputValueList->_buffer + InputValueList->_length;
			descRecPtr = m_DescRecCollect[ParamCount];
			IPDDescRecPtr = IPD->m_DescRecCollect[ParamCount];
			DataType = descRecPtr->m_DescConciseType;
			if (DataType == SQL_C_DEFAULT)
			{
				if ((retCode = getCDefault(IPDDescRecPtr->m_DescConciseType, ODBCAppVersion, DataType)) != SQL_SUCCESS)
				{
					ParamNumber = ParamCount+1;
					RowNumber = RowCount+1;
					RowStatus = SQL_PARAM_ERROR;
					pHandle->setDiagRec(DRIVER_ERROR, retCode, 0, NULL, NULL, RowNumber, ParamNumber);
					if(!useNAR)
					{
						rc = SQL_ERROR;
						break; //out of param loop
					}
					else
					{
						filteredPrevRow = true;
						rc = SQL_SUCCESS_WITH_INFO;
					}
					continue;
				}
			}
			if (descRecPtr->m_DescDataPtr != NULL)
			{
				int bufferLength = 0;

				if (m_DescBindType == SQL_BIND_BY_COLUMN)
				{
					bufferLength = descRecPtr->m_DescOctetLength;
					if (bufferLength == 0)
					{
						for (int i = 0; gCDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
						{
							if (gCDatatypeMap[i].conciseType == DataType)
							{
								bufferLength = gCDatatypeMap[i].octetLength;
								break;
							}
						}
					}

					if (bufferLength == 0)
					{
						pHandle->setDiagRec(DRIVER_ERROR, IDS_S1_000_06);
						return SQL_ERROR;
					}
					DataPtr = (SQLPOINTER)((SQLULEN)descRecPtr->m_DescDataPtr + (RowCount * bufferLength));
				}
				else
					DataPtr = (SQLPOINTER)((SQLULEN)descRecPtr->m_DescDataPtr + (RowCount * m_DescBindType));
			}
			else
				DataPtr = NULL;
			if (descRecPtr->m_DescOctetLengthPtr != NULL)
			{
				if (m_DescBindType == SQL_BIND_BY_COLUMN)
					StrLenPtr = descRecPtr->m_DescOctetLengthPtr + RowCount;
				else
					StrLenPtr = (SQLLEN*)((SQLULEN)descRecPtr->m_DescOctetLengthPtr + 
							(RowCount * m_DescBindType));

				if (*StrLenPtr == SQL_DEFAULT_PARAM)
				{
					ParamNumber = ParamCount+1;
					RowNumber = RowCount+1;
					pHandle->setDiagRec(DRIVER_ERROR, IDS_07_S01, 0, NULL, NULL, RowNumber, ParamNumber);  
					RowStatus = SQL_PARAM_ERROR;
					if(!useNAR)
					{
						rc = SQL_ERROR;
						break; //out of param loop
					}
					else
					{
						filteredPrevRow = true;
						rc = SQL_SUCCESS_WITH_INFO;
					}
					continue;
				}
				if (*StrLenPtr <=  SQL_LEN_DATA_AT_EXEC_OFFSET 
							|| *StrLenPtr == SQL_DATA_AT_EXEC)
				{
					ParamNumber = ParamCount+1;
					RowNumber = RowCount+1;
					return SQL_NEED_DATA;
				}
			}
			else
				StrLenPtr = NULL;
			if (descRecPtr->m_DescIndicatorPtr != NULL)
			{
				if (m_DescBindType == SQL_BIND_BY_COLUMN)
					IndPtr = descRecPtr->m_DescIndicatorPtr + RowCount;
				else
					IndPtr = (SQLLEN*)((SQLULEN)descRecPtr->m_DescIndicatorPtr + 
							(RowCount * m_DescBindType));
			}
			else
				IndPtr = NULL;
			if (m_DescBindOffsetPtr != NULL)
			{
				DataPtr = (SQLPOINTER)((SQLULEN)DataPtr + *m_DescBindOffsetPtr);
				StrLenPtr = (SQLLEN*)((SQLULEN)StrLenPtr + *m_DescBindOffsetPtr);
				IndPtr = (SQLLEN*)((SQLULEN)IndPtr + *m_DescBindOffsetPtr);
			}
			// ValuePtr cannot be NULL at SQLBindParameter
			if (DataPtr == NULL					// ParameterValuePtr is NULL and
				&& (IndPtr == NULL		// Strlen_or_indPtr is NULL or
				|| (IndPtr != NULL && *IndPtr != SQL_NULL_DATA)) 
				&& descRecPtr->m_DescParameterType != SQL_PARAM_OUTPUT)
			{
				ParamNumber = ParamCount+1;
				RowNumber = RowCount+1;
				pHandle->setDiagRec(DRIVER_ERROR, IDS_07_002, 0, NULL, NULL, RowNumber, ParamNumber);
				RowStatus = SQL_PARAM_ERROR;
				if(!useNAR)
				{
					rc = SQL_ERROR;
					break; //out of param loop
				}
				else
				{
					filteredPrevRow = true;
					rc = SQL_SUCCESS_WITH_INFO;
				}
				continue;
			}

			SQLValue->dataType = IPDDescRecPtr->m_SQLDataType;
			SQLValue->dataCharset = 0;	// To be filled up

			if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
			{
				char sTmp[50];
				long Size;

				if (IndPtr == NULL)
				{
					TraceOut(TR_ODBC_API, "CToSQL->inpData: <NULL> %s", "IndPtr");
				}
				else 
				{
					TraceOut(TR_ODBC_API, "CToSQL->inpData: IndPtr <%d>", *IndPtr);
				}

				if (DataPtr == NULL)
				{
					TraceOut(TR_ODBC_API, "CToSQL->inpData: <NULL> %s", "DataPtr");
				}
				else
				{
					if (StrLenPtr != NULL)
						Size = *StrLenPtr;
					else
						Size = SQL_NTS;
					sprintf(sTmp,"(%d) ",Size);
					if (Size == SQL_NTS) Size = strlen((char*)DataPtr);
					if (Size <= 0 ) Size = 16;
					HexOut(TR_ODBC_API, (SQLLEN*)&Size, DataPtr, strcat(sTmp,"CToSQL->inpData"));
				}
			}

			if (descRecPtr->m_DescParameterType == SQL_PARAM_OUTPUT 
			    || (IndPtr != NULL && *IndPtr == SQL_NULL_DATA))
			{
				if(!IPDDescRecPtr->m_DescNullable)
				{
					ParamNumber = ParamCount+1;
					RowNumber = RowCount+1;
					pHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, " Null Value in a non nullable column.", "23000",RowNumber,ParamNumber);
					RowStatus = SQL_PARAM_ERROR;
					if(!useNAR)
					{
						rc = SQL_ERROR;
						break; //out of param loop
					}
					else
					{
						filteredPrevRow = true;
						rc = SQL_SUCCESS_WITH_INFO;
					}
					continue;
				}
				SQLValue->dataInd = -1;
				SQLValue->dataValue._length = 0;
			}
			else
			{
				if (StrLenPtr != NULL)
					StrLen = *StrLenPtr;
				else
					StrLen = SQL_NTS;
				SQLValue->dataInd = 0;
	
				IPDDescRecPtr->setTranslateOption(DataType);
				retCode = ConvertCToSQL(ODBCAppVersion,
								DataType,
								DataPtr,
								StrLen,
								IPDDescRecPtr->m_ODBCDataType,
								IPDDescRecPtr->m_SQLDataType,
								IPDDescRecPtr->m_SQLDatetimeCode,
								SQLValue->dataValue._buffer,
								IPDDescRecPtr->m_SQLOctetLength,
								IPDDescRecPtr->m_ODBCPrecision,
								IPDDescRecPtr->m_ODBCScale,
								IPDDescRecPtr->m_SQLUnsigned,
								IPDDescRecPtr->m_SQLCharset,
								byteSwap,
								IPDDescRecPtr->m_translateOptionToSQL,
								errorMsg,
								sizeof(errorMsg),
								m_ConnectHandle->m_SrvrVersion.buildId);

				if(IPDDescRecPtr->m_ODBCDataType == SQL_VARCHAR ||
				   IPDDescRecPtr->m_ODBCDataType == SQL_LONGVARCHAR ||
				   IPDDescRecPtr->m_ODBCDataType == SQL_CHAR &&
				   IPDDescRecPtr->m_SQLDataType != SQLTYPECODE_BOOLEAN)
					SQLValue->dataValue._length = IPDDescRecPtr->m_SQLOctetLength-1;
				else if (IPDDescRecPtr->m_ODBCDataType == SQL_WVARCHAR ||
						 IPDDescRecPtr->m_ODBCDataType == SQL_WCHAR) 
					SQLValue->dataValue._length = IPDDescRecPtr->m_SQLOctetLength-2;
				else
					SQLValue->dataValue._length = IPDDescRecPtr->m_SQLOctetLength;

				if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
				{
					HexOut(TR_ODBC_API, (SQLLEN*)&SQLValue->dataValue._length, SQLValue->dataValue._buffer, "CToSQL->outData");
				}

				if (retCode != SQL_SUCCESS)
				{
					RowNumber = RowCount+1;
					ParamNumber = ParamCount+1;
					if  (errorMsg[0] != '\0')
						pHandle->setDiagRec(DRIVER_ERROR, retCode, 0, (char *)errorMsg, NULL, RowNumber, ParamNumber);
					else
					{
						sprintf((char *)errorMsg," Incorrect Format or Data [RowNumber: %d, ParamNumber:%d].",
                                                        RowNumber, ParamNumber);
						pHandle->setDiagRec(DRIVER_ERROR, retCode, 0, (char *)errorMsg, NULL,
								RowNumber, ParamNumber);
					}
					switch (retCode)
					{
					case IDS_01_004: //DATA TRUNCATED
					case IDS_01_S07: //FRACTIONAL TRUNCATION
					case IDS_188_DRVTODS_TRUNC:
						if (rc == SQL_SUCCESS)
							rc = SQL_SUCCESS_WITH_INFO;
						if(RowStatus != SQL_PARAM_ERROR)	
							RowStatus = SQL_PARAM_SUCCESS_WITH_INFO;
						break;
					//handle non-fatal errors if performing non atomic rowset insert
					case IDS_07_006: //RESTRICTED DATA TYPE ATTRIBUTE VIOLATION
					case IDS_193_DRVTODS_ERROR: //TRANSLATIONDLL ERROR: DRIVERTODATASOURCE
					case IDS_22_001: //STRING DATA RIGHT TRUNCATION
					case IDS_22_003: //NUMERIC VALUE OUT OF RANGE
					case IDS_22_005: //ERROR IN ASSIGNMENT
					case IDS_22_008: //DATETIME FIELD OVERFLOW
					case IDS_22_003_02://negValue in unsigned col error
						RowStatus = SQL_PARAM_ERROR;
						if(!useNAR)
							rc = SQL_ERROR;
						else
						{
							filteredPrevRow = true;
							rc = SQL_SUCCESS_WITH_INFO;
						}
						break;
					//the following are warning cases accoridng to unixmsg.h
					case IDS_22_015: //INTERVAL FIELD OVERFLOW
					case IDS_22_018: //INVALID CHARACTER VALUE FOR CAST SPECIFICATION
					case IDS_HY_000: //GENERAL ERROR where severity == WARNING
						if (rc == SQL_SUCCESS)
							rc = SQL_SUCCESS_WITH_INFO;
						if(RowStatus != SQL_PARAM_ERROR)	
							RowStatus = SQL_PARAM_SUCCESS_WITH_INFO;
						break;
					default:
						RowStatus = SQL_PARAM_ERROR;
						if(!useNAR)
							rc = SQL_ERROR;
						else
						{
							filteredPrevRow = true;
							rc = SQL_SUCCESS_WITH_INFO;
						}
						break;
					}
					if(rc == SQL_ERROR)
						break; //out of param-loop
				}
			}
			if (RowStatus != SQL_PARAM_ERROR)
				InputValueList->_length++;
		}  //end parameter/column for-loop
		if (IPDArrayStatusPtr != NULL)
			*IPDArrayStatusPtr = RowStatus;
		if(rc == SQL_ERROR)
			break; //out of row for-loop
		if(!filteredPrevRow)
		{
			pHandle->insertIntoDrvrSqlRowIdMap(RowCount+1);
			SqlRowNumber++;
		}
		else
		{
			InputValueList->_length = tempInputValueListLength;
			SQLValue = tempSQLValue;
		}
	}  //end row for-loop
	RowNumber = RowCount;
	if(InputValueList->_length==0 || rc == SQL_ERROR)//all rows contained ERRORS or the entire rowset was rejected
		pHandle->clearSqlDrvrRowIdMap();//don't need the row id map since we are not going to send anything to the server

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
	{
		long Size = sizeof(SQLUSMALLINT)* m_DescArraySize;
		if (IPD->m_DescArrayStatusPtr != NULL)
			HexOut(TR_ODBC_API, (SQLLEN*)&Size, IPD->m_DescArrayStatusPtr, "IPD->m_DescArrayStatusPtr");
	}
	return rc;
}

unsigned long CDesc::ParamData(SQLSMALLINT RowNumber, SQLSMALLINT ParamNumber, SQLPOINTER *ValuePtrPtr)
{

	CDescRec		*descRecPtr;
	SQLPOINTER		DataPtr;

	if (RowNumber > m_DescArraySize || ParamNumber > m_DescRecCollect.size())
		return IDS_07_002;

	descRecPtr = m_DescRecCollect[ParamNumber-1];
	if (descRecPtr->m_DescDataPtr != NULL)
	{
		int bufferLength = 0;

		if (m_DescBindType == SQL_BIND_BY_COLUMN)
		{
			bufferLength = descRecPtr->m_DescOctetLength;
			if (bufferLength == 0)
			{
				for (int i = 0; gCDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
				{
					if (gCDatatypeMap[i].conciseType == descRecPtr->m_DescConciseType)
					{
						bufferLength = gCDatatypeMap[i].octetLength;
						break;
					}
				}
			}

			if (bufferLength == 0)
			{
				setDiagRec(DRIVER_ERROR, IDS_S1_000_06);
				return SQL_ERROR;
			}
			DataPtr = (SQLPOINTER)((SQLULEN)descRecPtr->m_DescDataPtr + ((RowNumber -1) * bufferLength));
		}
		else
			DataPtr = (SQLPOINTER)((SQLULEN)descRecPtr->m_DescDataPtr + ((RowNumber -1) * m_DescBindType));
	}
	else
		DataPtr = NULL;
	if (m_DescBindOffsetPtr != NULL)		
		DataPtr = (SQLPOINTER)((SQLULEN)DataPtr + *m_DescBindOffsetPtr);
	*ValuePtrPtr = DataPtr;
	return SQL_SUCCESS;
}
	

unsigned long CDesc::GetParamType(SQLSMALLINT RowNumber, 
		SQLSMALLINT ParamNumber,
		SQLSMALLINT &DataType,
		SQLLEN	*&StrLenPtr)
{
	CDescRec		*descRecPtr;
	
	if (RowNumber > m_DescArraySize || ParamNumber > m_DescRecCollect.size())
		return IDS_07_002;

	descRecPtr = m_DescRecCollect[ParamNumber-1];
	DataType = descRecPtr->m_DescConciseType;
	
	if (descRecPtr->m_DescOctetLengthPtr != NULL)
		StrLenPtr = descRecPtr->m_DescOctetLengthPtr + RowNumber -1;
	else
		StrLenPtr = NULL;
	return SQL_SUCCESS;
}

unsigned long CDesc::GetParamSQLInfo(SQLSMALLINT RowNumber,
		SQLSMALLINT	ParamNumber,
		SQLSMALLINT	&ParameterType,
		SQLSMALLINT	&ODBCDataType)
{
	CDescRec		*descRecPtr;
	
	if (RowNumber > m_DescArraySize || ParamNumber > m_DescRecCollect.size())
		return IDS_07_002;

	descRecPtr = m_DescRecCollect[ParamNumber-1];
	ParameterType = descRecPtr->m_DescConciseType;
	ODBCDataType = descRecPtr->m_ODBCDataType;
	return SQL_SUCCESS;
}


SQLRETURN CDesc::CopyData(CHandle	*pHandle,
					SQLSMALLINT		ImpColumnCount,
					CDesc			*IRD,
					SQLULEN			RowsFetched,
					SQLULEN			&CurrentRowFetched,
					SQLULEN			&RowNumber,
					SQLULEN			&RowsetSize)
{
	SQLRETURN		rc = SQL_SUCCESS;
	SQLRETURN		RowStatus;
	short			ColumnCount;
	SQLULEN			AppRowCount;
	SQLULEN			RowsetRowCount;
	unsigned long	retCode;
//	SQLValue_def	*SQLValue;
	CDescRec		*descRecPtr;
	CDescRec		*IRDDescRecPtr;
	SQLLEN			*StrLenPtr;
	SQLPOINTER		DataPtr;
	SQLLEN			*IndPtr;
	SQLSMALLINT		AppColumnCount;
	SQLUSMALLINT	*ArrayStatusPtr;
	unsigned long	Index;
	BOOL			byteSwap;
	FPSQLDataSourceToDriver fpSQLDataSourceToDriver;
	UCHAR			errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	DWORD			DataLang;

	CStmt*			pStatement = (CStmt*)pHandle;
	SQLINTEGER		SQLCharset=0;
	SQLSMALLINT		SQLDataType=0; 
	SQLSMALLINT		SQLDatetimeCode=0;
	SQLINTEGER		SQLOctetLength=0; 
	SQLSMALLINT		SQLPrecision=0;
	SQLSMALLINT		SQLUnsigned=0;
	SQLSMALLINT		CDataType;

	short			SQLDataInd=0;
	int			SQLDataLength=0;
	BYTE*			SQLDataRow;
	BYTE*			SQLDataValue;
	unsigned long	IndexOffset; // Added this since Buffer pointers are not getting reset back in the driver when fetching locally.
	unsigned long	offset;
	
	unsigned long columnIndexCounter=0;
	unsigned long indIndex=0;
	unsigned long dataIndex=0;
	BYTE          *IndicatorPtr;
	long          *columnIndexes;
	char		  **swapArray = NULL;
	SQLUINTEGER	maxLength = 0;

	long stmtQueryType = pStatement->getStmtQueryType();

	DataLang = getDataLang();
	byteSwap = getByteSwap();
	AppColumnCount = this->m_DescCount;
	
	if (!pStatement->getFetchCatalog())
		swapArray = pStatement->getSwapInfo();

	if (IRD->m_DescArrayStatusPtr != NULL)
		for (AppRowCount = 0; AppRowCount < m_DescArraySize; AppRowCount++)
			*(IRD->m_DescArrayStatusPtr + AppRowCount) = SQL_ROW_NOROW;


	//Perf code START - part 1
	CacheDescStruct* CacheDescArray = NULL;
	if ((m_DescArraySize > 1) && (AppColumnCount > 0)) //create cache only incase of rowset
		CacheDescArray = new CacheDescStruct[ImpColumnCount];
	//Perf code END - part 1
	for (IndexOffset = 0, AppRowCount = 0, RowsetRowCount = CurrentRowFetched;
			AppRowCount < m_DescArraySize && RowsetRowCount < RowsFetched ; 
				IndexOffset++, RowsetRowCount++, AppRowCount++)
	{
		SQLDataRow = pStatement->getRowAddress(RowsetRowCount);

		// Obtain the RowStatusPtr from IRD
		if (IRD->m_DescArrayStatusPtr != NULL)
			ArrayStatusPtr = IRD->m_DescArrayStatusPtr + AppRowCount;
		else
			ArrayStatusPtr = NULL;

		columnIndexes = pStatement->getColumnIndexes();
		
		offset = 0; 
		int bufferLength = 0;
		for (ColumnCount = 0, RowStatus = SQL_ROW_SUCCESS ;
				ColumnCount < ImpColumnCount ;
					ColumnCount++)
		{
			if (AppColumnCount == ColumnCount) // Less Columns are bound
				break;
			descRecPtr = m_DescRecCollect[ColumnCount];
			bufferLength = 0;
			//Perf code START - part 2
			if(AppRowCount == 0)
			{
				pStatement->getImpBulkSQLData(ColumnCount+1,
												SQLCharset, 
												SQLDataType, 
												SQLDatetimeCode,
												SQLOctetLength,
												SQLPrecision,
												SQLUnsigned);
				if (m_DescArraySize > 1) //cache it only incase of rowsets
				{
					CacheDescArray[ColumnCount].SQLCharset = SQLCharset;
					CacheDescArray[ColumnCount].SQLDataType = SQLDataType;
					CacheDescArray[ColumnCount].SQLDatetimeCode = SQLDatetimeCode;
					CacheDescArray[ColumnCount].SQLOctetLength = SQLOctetLength;
					CacheDescArray[ColumnCount].SQLPrecision = SQLPrecision;
					CacheDescArray[ColumnCount].SQLUnsigned = SQLUnsigned;
					CacheDescArray[ColumnCount].bufferLength = 0 ; //initial value
					if (m_DescBindType == SQL_BIND_BY_COLUMN)
					{
						for	(int i = 0; gCDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
						{
							if (gCDatatypeMap[i].conciseType == descRecPtr->m_DescConciseType)
							{
								bufferLength = gCDatatypeMap[i].octetLength;
								break;
							}
						}
						if(bufferLength == SQL_DESC_LENGTH || bufferLength == 0)
							bufferLength = descRecPtr->m_DescOctetLength;
						CacheDescArray[ColumnCount].bufferLength = bufferLength;
					}
				}
			}
			if ((m_DescArraySize > 1) && (AppRowCount > 0)) //get desc info from cache
			{
				SQLCharset = CacheDescArray[ColumnCount].SQLCharset;
				SQLDataType = CacheDescArray[ColumnCount].SQLDataType;
				SQLDatetimeCode = CacheDescArray[ColumnCount].SQLDatetimeCode;
				SQLOctetLength = CacheDescArray[ColumnCount].SQLOctetLength;
				SQLPrecision = CacheDescArray[ColumnCount].SQLPrecision;
				SQLUnsigned = CacheDescArray[ColumnCount].SQLUnsigned;
				bufferLength = CacheDescArray[ColumnCount].bufferLength ; 
			}
			//Perf code END - part 2
			if (!pStatement->getFetchCatalog()) //set the swap flag per row
				swapArray[RowsetRowCount][ColumnCount] = 'Y';

			Index = RowsetRowCount *ImpColumnCount + ColumnCount;
			if (Index >= pStatement->getNumberOfElements())
			{
				pHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, NULL, NULL, 
							AppRowCount+1, ColumnCount+1);
				RowStatus = SQL_ROW_ERROR;
				rc = SQL_ERROR;
				break;
			}
			
			if (!pStatement->getFetchCatalog()) // (stmtQueryType == 10000)
			{
				SQLDataInd = 0;
				columnIndexCounter = ColumnCount*2;
				if ((indIndex=columnIndexes[columnIndexCounter]) != -1)
				{	
					IndicatorPtr = SQLDataRow + indIndex;
				}
				else
					IndicatorPtr = NULL;
				dataIndex = columnIndexes[++columnIndexCounter];
				SQLDataValue = SQLDataRow + dataIndex;
				if ((IndicatorPtr == NULL) || (IndicatorPtr != NULL && *((short *)IndicatorPtr) != -1))
				{
					if (byteSwap)
						SQLDatatype_Dependent_Swap(SQLDataValue, SQLDataType,
																	SQLCharset,
																	SQLOctetLength,
																	SQLDatetimeCode);
					SQLDataLength = SQLOctetLength;
				}
				else 
				{
					SQLDataValue = 0;
					SQLDataLength = 0;
					SQLDataInd = -1;
				}
				if (SQLDataInd != -1 && SQLCharset != SQLCHARSETCODE_UCS2 && 
						((SQLDataType == SQLTYPECODE_CHAR) || 
							(SQLDataType == SQLTYPECODE_BIT) ||
							(SQLDataType == SQLTYPECODE_VARCHAR)))
					SQLDataLength++;  

				if ((maxLength = pStatement->getMaxLength()) > 0 )
				{
					switch (SQLDataType)
					{
						case SQLTYPECODE_CHAR:
						case SQLTYPECODE_BIT:
						case SQLTYPECODE_VARCHAR:
						{
							if (SQLDataLength - 1 > maxLength) 
							{
								SQLDataLength = maxLength + 1;
							}
						}
						break;
						case SQLTYPECODE_VARCHAR_WITH_LENGTH:
						case SQLTYPECODE_VARCHAR_LONG:
						case SQLTYPECODE_BITVAR:
						{	
							if (maxLength > 0)
							{
								//SQLDataLength-3 to account for length(2 bytes) and null terminator(1 byte)
								if(IRD->m_DescRecCollect[ColumnCount]->m_SQLMaxLength<=32767){
									SQLDataLength = ((SQLDataLength-3)>maxLength)?maxLength + 3:SQLDataLength;
									*(short *)SQLDataValue = SQLDataLength - 3;
								}
								else{
									SQLDataLength = ((SQLDataLength-5)>maxLength)?maxLength + 5:SQLDataLength;
									*(int *)SQLDataValue = SQLDataLength - 5;
								}
								break;
							}
						}
						break;
					}
				} // if maxLength
			}
			else
			{
				SQLDataInd = (short)*(unsigned char*)(SQLDataRow + offset);
				if (SQLDataInd == 0)
				{
					if (SQLDataType == SQL_CHAR)
					{
						SQLOctetLength++; //The difference between getImpSQLData and getImpBulkSQLData
						//reset the cache
						if (m_DescArraySize > 1 && AppRowCount == 0)
							CacheDescArray[ColumnCount].SQLOctetLength = SQLOctetLength ;
					}
					offset = offset + 1;
					SQLDataValue = SQLDataRow + offset;
					SQLDataLength = dataLengthFetchPerf(SQLDataType,
															SQLOctetLength,
															pStatement->getMaxLength(),
															SQLCharset,
															SQLDataValue);
					offset = offset + SQLDataLength;
				}
				else 
				{
					SQLDataValue = 0;
					SQLDataLength = 0;
					SQLDataInd = -1;
					offset = offset + 1;
				}
			}
			if (pStatement->getAPIDecision() && !pStatement->getFetchCatalog())
				switch (SQLDataType)
				{
					case SQLTYPECODE_CHAR:
					case SQLTYPECODE_BIT:
					case SQLTYPECODE_VARCHAR:
					if (SQLDataInd != -1) SQLDataLength++;
						break;
				}
//			descRecPtr = m_DescRecCollect[ColumnCount];
			
			IRDDescRecPtr = IRD->m_DescRecCollect[ColumnCount];

			if ((descRecPtr->m_DescDataPtr != NULL) && (descRecPtr->m_DescParameterType != SQL_PARAM_INPUT))
			{
				if (m_DescArraySize > 1)	// Array Binding is done
				{
					if (m_DescBindType == SQL_BIND_BY_COLUMN)
					{
						if (bufferLength == 0)
						{
							pHandle->setDiagRec(DRIVER_ERROR, IDS_S1_000_06);
							if(CacheDescArray != NULL)
							{
								delete[] CacheDescArray;
								CacheDescArray = NULL;
							}
							return SQL_ERROR;
						}
						DataPtr = (SQLPOINTER)((SQLULEN)descRecPtr->m_DescDataPtr + IndexOffset * bufferLength);
					}
					else
						DataPtr = (SQLPOINTER)((SQLULEN)descRecPtr->m_DescDataPtr + (IndexOffset * m_DescBindType));
				}
				else
					DataPtr = descRecPtr->m_DescDataPtr;
			}
			else
				DataPtr = NULL;
			if (descRecPtr->m_DescOctetLengthPtr != NULL)
			{
				if (m_DescArraySize > 1)
				{
					if (m_DescBindType == SQL_BIND_BY_COLUMN)
						StrLenPtr = descRecPtr->m_DescOctetLengthPtr + IndexOffset;
					else
						StrLenPtr = (SQLLEN*)((SQLULEN)descRecPtr->m_DescOctetLengthPtr + 
							(IndexOffset * m_DescBindType));
				}
				else
					StrLenPtr = descRecPtr->m_DescOctetLengthPtr;
			}
			else
				StrLenPtr = NULL;

			if (descRecPtr->m_DescIndicatorPtr != NULL)
			{
				if (m_DescArraySize > 1)
				{
					if (m_DescBindType == SQL_BIND_BY_COLUMN)
						IndPtr = descRecPtr->m_DescIndicatorPtr + IndexOffset;
					else
						IndPtr = (SQLLEN*)((SQLULEN)descRecPtr->m_DescIndicatorPtr + 
							(IndexOffset * m_DescBindType));
				}
				else
					IndPtr = descRecPtr->m_DescIndicatorPtr;
			}
			else
				IndPtr = NULL;

			if (m_DescBindOffsetPtr != NULL)
			{
				DataPtr = (SQLPOINTER)((SQLULEN)DataPtr + *m_DescBindOffsetPtr);
				StrLenPtr = (SQLLEN*)((SQLULEN)StrLenPtr + *m_DescBindOffsetPtr);
				IndPtr = (SQLLEN*)((SQLULEN)IndPtr + *m_DescBindOffsetPtr);
			}

			if (DataPtr != NULL)
			{
				if (SQLDataInd == -1)
				{
					if (IndPtr == NULL)
					{
						pHandle->setDiagRec(DRIVER_ERROR, IDS_22_002, 0, NULL, NULL, 
								AppRowCount+1, ColumnCount+1);
						RowStatus = SQL_ROW_ERROR;
						rc = SQL_ERROR;
					}
					else
						*IndPtr = SQL_NULL_DATA;
				}
				else
				{
					descRecPtr->m_SQLCharset=IRDDescRecPtr->m_SQLCharset;
					descRecPtr->setTranslateOption(descRecPtr->m_DescConciseType);
                    retCode = ConvertSQLToC(m_ConnectHandle,
                                            m_InputHandle,
                                            m_ODBCAppVersion,
											DataLang,
											SQLDataType,
											IRDDescRecPtr->m_ODBCDataType,
											IRDDescRecPtr->m_SQLDatetimeCode,
											SQLDataValue,
											SQLDataLength,
											IRDDescRecPtr->m_ODBCPrecision,
											IRDDescRecPtr->m_ODBCScale,
											IRDDescRecPtr->m_DescUnsigned,
											IRDDescRecPtr->m_SQLCharset,
											IRDDescRecPtr->m_SQLMaxLength,
											descRecPtr->m_DescConciseType, 
											DataPtr, 
											descRecPtr->m_DescOctetLength, 
											StrLenPtr,
											byteSwap,
											descRecPtr->m_TranslatedDataPtr,
											NULL,
											descRecPtr->m_translateOptionFromSQL,
											errorMsg,
											sizeof(errorMsg),
											m_ConnectHandle->m_SrvrVersion.buildId,
											pStatement->getFetchCatalog(),
											m_replacementChar);

					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
					{
						HexOut(TR_ODBC_API, (SQLLEN*)&SQLDataLength, SQLDataValue, "SQLToC->inpData");
						HexOut(TR_ODBC_API, StrLenPtr, DataPtr, "SQLToC->outData");
					}

					if (retCode != SQL_SUCCESS && retCode != SQL_NO_DATA)
					{
						if (errorMsg[0] != '\0')
							pHandle->setDiagRec(DRIVER_ERROR, retCode, 0, (char *)errorMsg, NULL,
								AppRowCount+1, ColumnCount+1);
						else
							pHandle->setDiagRec(DRIVER_ERROR, retCode, 0, NULL, NULL,
								AppRowCount+1, ColumnCount+1);
					
						switch (retCode)
						{
						case IDS_01_004:
						case IDS_01_S07:
						case IDS_22_003:
						case IDS_186_DSTODRV_TRUNC:
							rc = SQL_SUCCESS_WITH_INFO;
							RowStatus = SQL_ROW_SUCCESS_WITH_INFO;
							break;
						default:
							RowStatus = SQL_ROW_ERROR;
							rc = SQL_ERROR;
							break;
						}
					}
					descRecPtr->deleteTranslatedDataPtr();
				}
			}
			else
			{
				if (StrLenPtr != NULL)
					*StrLenPtr = IRD->m_DescRecCollect[ColumnCount]->m_DescOctetLength;
			}
		}
		if (ArrayStatusPtr != NULL)
			*ArrayStatusPtr = RowStatus;

		RowNumber++;
		CurrentRowFetched++;
	}
	RowsetSize = AppRowCount;
	// Initialize the Returned Length to Zero in IRD
	// ARD is not used since all the columns may not be bound

	for (ColumnCount = 0 ; ColumnCount < ImpColumnCount ; ColumnCount++)
	{
		IRD->m_DescRecCollect[ColumnCount]->m_DescReturnedLength = 0;
		IRD->m_DescRecCollect[ColumnCount]->deleteTranslatedDataPtr();
	}

	if (rc == SQL_ERROR)
	{
		if (m_DescArraySize > 1 && CurrentRowFetched > 1)
			rc = SQL_SUCCESS_WITH_INFO; 
	}
	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
	{
		long Size = sizeof(SQLUSMALLINT)* m_DescArraySize;
		if (IRD->m_DescArrayStatusPtr != NULL)
			HexOut(TR_ODBC_API, (SQLLEN*)&Size, IRD->m_DescArrayStatusPtr, "IRD->m_DescArrayStatusPtr");
		Size = sizeof(SQLUINTEGER);
		HexOut(TR_ODBC_API, (SQLLEN*)&Size, &RowsFetched, "RowsFetched");
	}
	if(CacheDescArray != NULL)
		delete[] CacheDescArray;
	return rc;
}

SQLRETURN CDesc::ExtendedCopyData(CHandle *pHandle,
					SQLSMALLINT		ImpColumnCount,
					CDesc			*IRD,
					SQLULEN			RowsFetched,
					SQLULEN			&CurrentRowFetched,
					SQLULEN			&RowNumber,
					SQLULEN			&RowsetSize,
					SQLUSMALLINT*	RowStatusArray)
{
	SQLRETURN		rc = SQL_SUCCESS;
	SQLRETURN		RowStatus;
	short			ColumnCount;
	SQLULEN			AppRowCount;
	SQLULEN			RowsetRowCount;
	unsigned long	retCode;
	CDescRec		*descRecPtr;
	CDescRec		*IRDDescRecPtr;
	SQLLEN			*StrLenPtr;
	SQLPOINTER		DataPtr;
	SQLLEN			*IndPtr;
	SQLSMALLINT		AppColumnCount;
	SQLUSMALLINT	*ArrayStatusPtr;
	unsigned long	Index;
	BOOL			byteSwap;
	UCHAR			errorMsg[MAX_TRANSLATE_ERROR_MSG_LEN];
	DWORD			DataLang;

	CStmt*			pStatement = (CStmt*)pHandle;
	SQLINTEGER		SQLCharset=0;
	SQLSMALLINT		SQLDataType=0; 
	SQLSMALLINT		SQLDatetimeCode=0;
	SQLINTEGER		SQLOctetLength=0; 
	SQLSMALLINT		SQLPrecision=0;
	SQLSMALLINT		SQLUnsigned=0;

	short			SQLDataInd=0;
	int			SQLDataLength=0;
	BYTE*			SQLDataRow;
	BYTE*			SQLDataValue;
	unsigned long	IndexOffset; // Added this since Buffer pointers are not getting reset back in the driver when fetching locally. 
	long stmtQueryType = pStatement->getStmtQueryType();
	unsigned long	offset;

	unsigned long columnIndexCounter=0;
	unsigned long indIndex=0;
	unsigned long dataIndex=0;
	BYTE	*IndicatorPtr;
	long          *columnIndexes;
	char		  **swapArray = NULL;

	DataLang = getDataLang();
	byteSwap = getByteSwap();
//	fpSQLDataSourceToDriver = getSQLDataSourceToDriverFP();
	GetDescField(0, SQL_DESC_COUNT, &AppColumnCount, SQL_IS_SMALLINT, NULL);
	
	if (!pStatement->getFetchCatalog())
		swapArray = pStatement->getSwapInfo();

	if (RowStatusArray != NULL)
		for (AppRowCount = 0; AppRowCount < m_DescArraySize; AppRowCount++)
			*(RowStatusArray + AppRowCount) = SQL_ROW_NOROW;


	for (IndexOffset = 0, AppRowCount = 0, RowsetRowCount = CurrentRowFetched;
			AppRowCount < m_DescArraySize && RowsetRowCount < RowsFetched ; 
				IndexOffset++, RowsetRowCount++, AppRowCount++)
	{
		SQLDataRow = pStatement->getRowAddress(RowsetRowCount);

		// Obtain the RowStatusPtr from IRD
		if (RowStatusArray != NULL)
			ArrayStatusPtr = RowStatusArray + AppRowCount;
		else
			ArrayStatusPtr = NULL;
		
		columnIndexes = pStatement->getColumnIndexes();
		offset = 0;
		for (ColumnCount = 0, RowStatus = SQL_ROW_SUCCESS ;
				ColumnCount < ImpColumnCount ;
					ColumnCount++)
		{
			if (AppColumnCount == ColumnCount) // Less Columns are bound
				break;

			if (!pStatement->getFetchCatalog()) //set the swap flag per row
				swapArray[RowsetRowCount][ColumnCount] = 'Y';

			Index = RowsetRowCount *ImpColumnCount + ColumnCount;
			if (Index >= pStatement->getNumberOfElements())
			{
				pHandle->setDiagRec(DRIVER_ERROR, IDS_HY_000, 0, NULL, NULL, 
							AppRowCount+1, ColumnCount+1);
				RowStatus = SQL_ROW_ERROR;
				rc = SQL_ERROR;
				break;
			}

			if(!pStatement->getFetchCatalog()) //if (stmtQueryType == 10000)
			{
				SQLDataInd = 0;
				columnIndexCounter = ColumnCount*2;
				pStatement->getImpBulkSQLData(ColumnCount+1, SQLCharset, SQLDataType, SQLDatetimeCode, SQLOctetLength, SQLPrecision,SQLUnsigned);
				//SQLOctetLength = SQLMaxLength;
				if ((indIndex=columnIndexes[columnIndexCounter]) != -1)
				{	
					IndicatorPtr = SQLDataRow + indIndex;
				}
				else
					IndicatorPtr = NULL;
				
				dataIndex = columnIndexes[++columnIndexCounter];
				SQLDataValue = SQLDataRow + dataIndex;
	
				if ((IndicatorPtr == NULL) || (IndicatorPtr != NULL && *((short *)IndicatorPtr) != -1))
				{
					if (byteSwap)
						SQLDatatype_Dependent_Swap(SQLDataValue, SQLDataType, SQLCharset, SQLOctetLength, SQLDatetimeCode);

					SQLDataLength = SQLOctetLength;
				
				}
				else 
				{
					SQLDataValue = 0;
					SQLDataLength = 0;
					SQLDataInd = -1;
				
				}
				SQLUINTEGER	maxLength = pStatement->getMaxLength();
				switch (SQLDataType)
				{
					case SQLTYPECODE_CHAR:
					case SQLTYPECODE_BIT:
					case SQLTYPECODE_VARCHAR:
					{
						//SQLDataLength-1, to take care of null terminator
						SQLUINTEGER	maxLength2 = maxLength;
						if (SQLCharset == SQLCHARSETCODE_UCS2)
							maxLength2 = maxLength * 2;
						if (maxLength2 > 0)
								SQLDataLength = ((SQLDataLength-1)>maxLength2)?maxLength2 + 1:SQLDataLength;
						if (SQLDataInd != -1) SQLDataLength++;
					}
					break;
					case SQLTYPECODE_VARCHAR_WITH_LENGTH:
					case SQLTYPECODE_VARCHAR_LONG:
					case SQLTYPECODE_BITVAR:
					{	
						SQLUINTEGER	maxLength2 = maxLength;
						if (SQLCharset == SQLCHARSETCODE_UCS2)
							maxLength2 = maxLength * 2;
						if (maxLength2 > 0)
						{
							//SQLDataLength-3 to account for length(2 bytes) and null terminator(1 byte)
							if(IRD->m_DescRecCollect[ColumnCount]->m_SQLMaxLength<=32767){
								SQLDataLength = ((SQLDataLength-3)>maxLength2)?maxLength2 + 3:SQLDataLength;
								*(short *)SQLDataValue = SQLDataLength - 3;
							}
							else{
								SQLDataLength = ((SQLDataLength-5)>maxLength2)?maxLength2 + 5:SQLDataLength;
								*(int *)SQLDataValue = SQLDataLength - 5;
							}
							break;
						}
					}
					break;
				}
			}
			else
			{
				SQLDataInd = (short)*(unsigned char*)(SQLDataRow + offset);
				if (SQLDataInd == 0)
				{
					pStatement->getImpSQLData(ColumnCount+1, SQLCharset, SQLDataType, SQLDatetimeCode, SQLOctetLength, SQLPrecision,SQLUnsigned);
					offset = offset + 1;
					SQLDataValue = SQLDataRow + offset;
					SQLDataLength = dataLengthFetchPerf(SQLDataType, SQLOctetLength, pStatement->getMaxLength(), SQLCharset, SQLDataValue);
					offset = offset + SQLDataLength;
				}
				else
				{
					SQLDataValue = 0;
					SQLDataLength = 0;
					SQLDataInd = -1;
					offset = offset + 1;
				}
			}
			if (pStatement->getAPIDecision() && !pStatement->getFetchCatalog())
				switch (SQLDataType)
				{
					case SQLTYPECODE_CHAR:
					case SQLTYPECODE_BIT:
					case SQLTYPECODE_VARCHAR:
					if (SQLDataInd != -1) SQLDataLength++;
						break;
				}

			descRecPtr = m_DescRecCollect[ColumnCount];
			IRDDescRecPtr = IRD->m_DescRecCollect[ColumnCount];
			if (descRecPtr->m_DescDataPtr != NULL)
			{
				if (m_DescArraySize > 1)	// Array Binding is done
				{
					if (m_DescBindType == SQL_BIND_BY_COLUMN)
					{
						int bufferLength = 0;

						for (int i = 0; gCDatatypeMap[i].conciseType != SQL_DEFAULT ; i++)
						{
							if (gCDatatypeMap[i].conciseType == descRecPtr->m_DescConciseType)
							{
								bufferLength = gCDatatypeMap[i].octetLength;
								break;
							}
						}
						
						if(bufferLength == SQL_DESC_LENGTH || bufferLength == 0)
							bufferLength = descRecPtr->m_DescOctetLength;

						if (bufferLength == 0)
						{
							pHandle->setDiagRec(DRIVER_ERROR, IDS_S1_000_06);
							return SQL_ERROR;
						}
						DataPtr = (SQLPOINTER)((SQLULEN)descRecPtr->m_DescDataPtr + IndexOffset * bufferLength);
					}
					else
						DataPtr = (SQLPOINTER)((SQLULEN)descRecPtr->m_DescDataPtr + (IndexOffset * m_DescBindType));
				}
				else
					DataPtr = descRecPtr->m_DescDataPtr;
			}
			else
				DataPtr = NULL;

			if (descRecPtr->m_DescOctetLengthPtr != NULL)
			{
				if (m_DescArraySize > 1)
				{
					if (m_DescBindType == SQL_BIND_BY_COLUMN)
						StrLenPtr = descRecPtr->m_DescOctetLengthPtr + IndexOffset;
					else
						StrLenPtr = (SQLLEN*)((SQLULEN)descRecPtr->m_DescOctetLengthPtr + 
							(IndexOffset * m_DescBindType));
				}
				else
					StrLenPtr = descRecPtr->m_DescOctetLengthPtr;
			}
			else
				StrLenPtr = NULL;

			if (descRecPtr->m_DescIndicatorPtr != NULL)
			{
				if (m_DescArraySize > 1)
				{
					if (m_DescBindType == SQL_BIND_BY_COLUMN)
						IndPtr = descRecPtr->m_DescIndicatorPtr + IndexOffset;
					else
						IndPtr = (SQLLEN*)((SQLULEN)descRecPtr->m_DescIndicatorPtr + 
							(IndexOffset * m_DescBindType));
				}
				else
					IndPtr = descRecPtr->m_DescIndicatorPtr;
			}
			else
				IndPtr = NULL;

			if (m_DescBindOffsetPtr != NULL)
			{
				DataPtr = (SQLPOINTER)((SQLULEN)DataPtr + *m_DescBindOffsetPtr);
				StrLenPtr = (SQLLEN*)((SQLULEN)StrLenPtr + *m_DescBindOffsetPtr);
				IndPtr = (SQLLEN*)((SQLULEN)IndPtr + *m_DescBindOffsetPtr);
			}

			if (DataPtr != NULL)
			{
				if (SQLDataInd == -1)
				{
					if (IndPtr == NULL)
					{
						pHandle->setDiagRec(DRIVER_ERROR, IDS_22_002, 0, NULL, NULL, 
								AppRowCount+1, ColumnCount+1);
						RowStatus = SQL_ROW_ERROR;
						rc = SQL_ERROR;
					}
					else
						*IndPtr = SQL_NULL_DATA;
				}
				else
				{
					descRecPtr->m_SQLCharset=IRDDescRecPtr->m_SQLCharset;
					descRecPtr->setTranslateOption(descRecPtr->m_DescConciseType);
                    retCode = ConvertSQLToC(m_ConnectHandle,
                                            m_InputHandle,
											m_ODBCAppVersion,
											DataLang,
											SQLDataType,
											IRDDescRecPtr->m_ODBCDataType,
											IRDDescRecPtr->m_SQLDatetimeCode,
											SQLDataValue,
											SQLDataLength,
											IRDDescRecPtr->m_ODBCPrecision,
											IRDDescRecPtr->m_ODBCScale,
											IRDDescRecPtr->m_DescUnsigned,
											IRDDescRecPtr->m_SQLCharset,
											IRDDescRecPtr->m_SQLMaxLength,
											descRecPtr->m_DescConciseType, 
											DataPtr, 
											descRecPtr->m_DescOctetLength, 
											StrLenPtr,
											byteSwap,
											descRecPtr->m_TranslatedDataPtr,
											NULL,
											descRecPtr->m_translateOptionFromSQL,
											errorMsg,
											sizeof(errorMsg),
											m_ConnectHandle->m_SrvrVersion.buildId,
											pStatement->getFetchCatalog(),
											m_replacementChar);

					if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
					{
						HexOut(TR_ODBC_API, (SQLLEN*)&SQLDataLength, SQLDataValue, "SQLToC->inpData");
						HexOut(TR_ODBC_API, StrLenPtr, DataPtr, "SQLToC->outData");
					}

					if (retCode != SQL_SUCCESS && retCode != SQL_NO_DATA)
					{
						if (errorMsg[0] != '\0')
							pHandle->setDiagRec(DRIVER_ERROR, retCode, 0, (char *)errorMsg, NULL,
								AppRowCount+1, ColumnCount+1);
						else
							pHandle->setDiagRec(DRIVER_ERROR, retCode, 0, NULL, NULL,
								AppRowCount+1, ColumnCount+1);
					
						switch (retCode)
						{
						case IDS_01_004:
						case IDS_01_S07:
						case IDS_22_003:
						case IDS_186_DSTODRV_TRUNC:
							rc = SQL_SUCCESS_WITH_INFO;
							RowStatus = SQL_ROW_SUCCESS_WITH_INFO;
							break;
						default:
							RowStatus = SQL_ROW_ERROR;
							rc = SQL_ERROR;
							break;
						}
					}
					descRecPtr->deleteTranslatedDataPtr();
				}
			}
			else
			{
				if (StrLenPtr != NULL)
					*StrLenPtr = IRD->m_DescRecCollect[ColumnCount]->m_DescOctetLength;
			}
		}
		if (ArrayStatusPtr != NULL)
			*ArrayStatusPtr = RowStatus;

		RowNumber++;
		CurrentRowFetched++;
	}
	RowsetSize = AppRowCount;
	// Initialize the Returned Length to Zero in IRD
	// ARD is not used since all the columns may not be bound

	for (ColumnCount = 0 ; ColumnCount < ImpColumnCount ; ColumnCount++)
	{
		IRD->m_DescRecCollect[ColumnCount]->m_DescReturnedLength = 0;
		IRD->m_DescRecCollect[ColumnCount]->deleteTranslatedDataPtr();
	}

	if (rc == SQL_ERROR)
	{
		if (m_DescArraySize > 1 && CurrentRowFetched > 1)
			rc = SQL_SUCCESS_WITH_INFO; 
	}
	if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
	{
		long Size = sizeof(SQLUSMALLINT)* m_DescArraySize;
		if (RowStatusArray != NULL)
			HexOut(TR_ODBC_API, (SQLLEN*)&Size, RowStatusArray, "RowStatusArray");
		Size = sizeof(SQLUINTEGER);
		HexOut(TR_ODBC_API, (SQLLEN*)&Size, &RowsFetched, "RowsFetched");
	}
	return rc;
}

unsigned long CDesc::GetData(SQLValue_def *SQLValue,
			SQLUSMALLINT ColumnNumber,
			SQLSMALLINT TargetType,
			SQLPOINTER	TargetValuePtr, 
			SQLLEN		BufferLength,
			SQLLEN		*StrLen_or_IndPtr,
			UCHAR		*errorMsg,
			SWORD		errorMsgMax,
			BOOL		ColumnwiseData)
{
	unsigned long retCode = SQL_SUCCESS;
	CDescRec		*descRecPtr;
	BOOL		byteSwap;
	DWORD		DataLang;

	DataLang = getDataLang();
	byteSwap = getByteSwap();
	
	descRecPtr = m_DescRecCollect[ColumnNumber-1];
	// If the data is already returned, return SQL_NO_DATA
	if (descRecPtr->m_DescReturnedLength == -1)
		return SQL_NO_DATA;

	if (SQLValue->dataInd == -1)
	{
		descRecPtr->m_DescReturnedLength = -1;
		if (StrLen_or_IndPtr == NULL)
			return IDS_22_002;
		else
			*StrLen_or_IndPtr = SQL_NULL_DATA;
	}
	else
	{
		descRecPtr->setTranslateOption(TargetType);
        retCode = ConvertSQLToC(m_ConnectHandle,
                                m_InputHandle,
                                m_ODBCAppVersion,
								DataLang,
								SQLValue->dataType,
								descRecPtr->m_ODBCDataType,
								descRecPtr->m_SQLDatetimeCode,
								SQLValue->dataValue._buffer,
								SQLValue->dataValue._length,
								descRecPtr->m_ODBCPrecision,
								descRecPtr->m_ODBCScale,
								descRecPtr->m_DescUnsigned,
								descRecPtr->m_SQLCharset,
								descRecPtr->m_SQLMaxLength,
								TargetType,
								TargetValuePtr, 
								BufferLength,
								StrLen_or_IndPtr,
								byteSwap,
								descRecPtr->m_TranslatedDataPtr,
								&descRecPtr->m_DescReturnedLength,
								descRecPtr->m_translateOptionFromSQL,
								errorMsg,
								errorMsgMax,
								m_ConnectHandle->m_SrvrVersion.buildId,
								ColumnwiseData,
								m_replacementChar);

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
		{
			HexOut(TR_ODBC_API, (SQLLEN*)&SQLValue->dataValue._length, SQLValue->dataValue._buffer, "SQLToC->inpData");
			HexOut(TR_ODBC_API, StrLen_or_IndPtr, TargetValuePtr, "SQLToC->outData");
		}
	}
	if (retCode == SQL_NO_DATA) descRecPtr->deleteTranslatedDataPtr();
	return retCode;
}
	

unsigned long CDesc::AssignDataAtExecValue(SQLValue_def *DataAtExecData,
							CDesc		*APD,
							SQLINTEGER	ODBCAppVersion,
							SQLSMALLINT ParamNumber, 
							SQLSMALLINT RowNumber,
							SQLValueList_def *OutputValueList,
							UCHAR		*errorMsg,
							SWORD		errorMsgMax)
{
	SQLValue_def	*SQLValue;
	SQLSMALLINT		DataType;
	CDescRec		*descRecPtr;
	CDescRec		*APDDescRecPtr;
	SQLRETURN		RowStatus = SQL_PARAM_SUCCESS;
	unsigned long	retCode = SQL_SUCCESS;
	SQLUSMALLINT	*ArrayStatusPtr;
	BOOL			byteSwap;				
	byteSwap = getByteSwap();
	
	SQLValue = (SQLValue_def *)OutputValueList->_buffer + OutputValueList->_length;
	APDDescRecPtr = APD->m_DescRecCollect[ParamNumber-1];
	descRecPtr = m_DescRecCollect[ParamNumber-1];

	DataType = DataAtExecData->dataType;
	// Fill up the SQLValue	
	if (DataType == SQL_C_DEFAULT)
	{
		if ((retCode = getCDefault(descRecPtr->m_DescConciseType,
								ODBCAppVersion, DataType)) != SQL_SUCCESS)
			return IDS_07_006;
	}
	SQLValue->dataType = descRecPtr->m_SQLDataType;
	SQLValue->dataCharset = 0;	// To be filled up
	if (DataAtExecData->dataInd == -1)
	{
		SQLValue->dataInd = -1;
		SQLValue->dataValue._length = 0;
	}
	else
	{
		SQLValue->dataInd = 0;
		SQLValue->dataValue._length = descRecPtr->m_SQLOctetLength;

		descRecPtr->setTranslateOption(DataType);
		retCode = ConvertCToSQL(ODBCAppVersion,
					DataType,
					DataAtExecData->dataValue._buffer,
					DataAtExecData->dataValue._length,
					descRecPtr->m_ODBCDataType,
					descRecPtr->m_SQLDataType,
					descRecPtr->m_SQLDatetimeCode,
					SQLValue->dataValue._buffer,
					descRecPtr->m_SQLOctetLength,
					descRecPtr->m_ODBCPrecision,
					descRecPtr->m_ODBCScale,
					descRecPtr->m_SQLUnsigned,
					descRecPtr->m_SQLCharset,
					byteSwap,
					descRecPtr->m_translateOptionToSQL,
					errorMsg,
					errorMsgMax,
					m_ConnectHandle->m_SrvrVersion.buildId);

		if(pdwGlobalTraceVariable && *pdwGlobalTraceVariable)
		{
			char sTmp[50];
			long Size = DataAtExecData->dataValue._length;
			sprintf(sTmp,"(%d) ",Size);
			if (Size == SQL_NTS)
				Size = strlen((char*)DataAtExecData->dataValue._buffer);
			if (Size <= 0) Size = 16;
			HexOut(TR_ODBC_API, (SQLLEN*)&Size, DataAtExecData->dataValue._buffer, strcat(sTmp,"CToSQL->inpData"));
			HexOut(TR_ODBC_API, (SQLLEN*)&descRecPtr->m_SQLOctetLength, SQLValue->dataValue._buffer, "CToSQL->outData");
		}

		if (retCode != SQL_SUCCESS)
		{
			switch (retCode)
			{
			case IDS_01_004:
			case IDS_01_S07:
			case IDS_188_DRVTODS_TRUNC:
				RowStatus = SQL_PARAM_SUCCESS_WITH_INFO;
				break;
			case IDS_22_001:
			default:
				RowStatus = SQL_PARAM_ERROR;
				break;
			}
		}
	}
	if (m_DescArrayStatusPtr != NULL)
	{
		ArrayStatusPtr = m_DescArrayStatusPtr + RowNumber-1;
		if (RowStatus == SQL_PARAM_ERROR)
			*ArrayStatusPtr = RowStatus;
	}
	OutputValueList->_length++;
	return retCode;
}

SQLRETURN CDesc::CopyDesc(SQLHDESC SourceDescHandle)
{
	SQLRETURN	rc = SQL_SUCCESS;
	CDesc		*pSrcDesc;
	short		stmtState;
	IDL_long    stmtQueryType;
	CDescRec	*tgtDescRecPtr;
	CDescRec	*srcDescRecPtr;
	size_t		i;
	unsigned long retCode;

	clearError();
	if (m_DescMode == SQL_ATTR_IMP_ROW_DESC)
	{
		setDiagRec(DRIVER_ERROR, IDS_HY_016);
		return SQL_ERROR;
	}
	pSrcDesc = (CDesc *)SourceDescHandle;
	if (pSrcDesc->m_DescMode == SQL_ATTR_IMP_ROW_DESC)
	{
		if (pSrcDesc->m_AssociatedStmt != NULL)
		{
			stmtState = pSrcDesc->m_AssociatedStmt->getStmtState();
			stmtQueryType = m_AssociatedStmt->getStmtQueryType();
			switch (stmtState)
			{
			case STMT_EXECUTED_NO_RESULT:
			case STMT_PREPARED_NO_RESULT:
				if(stmtQueryType != SQL_SELECT_UNIQUE)
				{
				   setDiagRec(DRIVER_ERROR, IDS_24_000);
				   return SQL_ERROR;
				}
			case STMT_ALLOCATED:
			case STMT_STILL_EXECUTING:
				setDiagRec(DRIVER_ERROR, IDS_HY_007);
				return SQL_ERROR;
			default:
				break;	
			}
		}
	}
	// Copy all the fields except m_DescAllocType and m_DescMode
	m_DescArraySize = pSrcDesc->m_DescArraySize;
	m_DescArrayStatusPtr = pSrcDesc->m_DescArrayStatusPtr;
	m_DescBindType = pSrcDesc->m_DescBindType;
	m_DescBindOffsetPtr = pSrcDesc->m_DescBindOffsetPtr;
	m_DescCount = pSrcDesc->m_DescCount;
	m_DescRowsProcessedPtr = pSrcDesc->m_DescRowsProcessedPtr;
	
	for (i = 0; i < pSrcDesc->m_DescRecCollect.size() ; i++)
	{
		if (m_DescRecCollect.size() <= i)
		{
			retCode = SetDescField(i+1);
			if (retCode != SQL_SUCCESS)
			{
				setDiagRec(DRIVER_ERROR, retCode);
				rc = SQL_ERROR;
				break;
			}
		}
		tgtDescRecPtr = m_DescRecCollect[i];
		srcDescRecPtr = pSrcDesc->m_DescRecCollect[i];
		retCode = tgtDescRecPtr->CopyDesc(srcDescRecPtr);
		if (retCode != SQL_SUCCESS)
		{
			setDiagRec(DRIVER_ERROR, retCode);
			rc = SQL_ERROR;
			break;
		}
	}
	return rc;
}

SQLRETURN CDesc::SetPos(SQLSMALLINT NumResultCols)
{
	SQLRETURN	rc = SQL_SUCCESS;
	for (int ColumnCount = 0 ; ColumnCount < NumResultCols ; ColumnCount++)
		m_DescRecCollect[ColumnCount]->m_DescReturnedLength = 0;
	return rc;
}

BOOL CDesc::getDescRecSQLData(long colnumber,
	SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
	SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLOctetLength, 
	SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned)
{
	CDescRec *descRecPtr;

	if (m_DescCount > colnumber - 1)
	{
		descRecPtr = m_DescRecCollect[colnumber-1];
		SQLCharset = descRecPtr->m_SQLCharset;
		SQLDataType = descRecPtr->m_SQLDataType;
		SQLDatetimeCode = descRecPtr->m_SQLDatetimeCode;
		SQLOctetLength = descRecPtr->m_SQLOctetLength;
		SQLPrecision = descRecPtr->m_SQLPrecision;
		SQLUnsigned = descRecPtr->m_SQLUnsigned;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CDesc::getDescRecBulkSQLData(long colnumber,
	SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
	SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLOctetLength, 
	SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned)
{
	CDescRec *descRecPtr;

	if (m_DescCount > colnumber - 1)
	{
		descRecPtr = m_DescRecCollect[colnumber-1];
		SQLCharset = descRecPtr->m_SQLCharset;
		SQLDataType = descRecPtr->m_SQLDataType;
		SQLDatetimeCode = descRecPtr->m_SQLDatetimeCode;
		SQLOctetLength = descRecPtr->m_SQLOctetLength;
		SQLPrecision = descRecPtr->m_SQLPrecision;
		SQLUnsigned = descRecPtr->m_SQLUnsigned;
		if (SQLDataType == SQL_CHAR)
			SQLOctetLength--;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CDesc::getDescParamRecData(long colnumber,
	SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
	SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLMaxLength, 
	SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned,
	SQLSMALLINT& SQLNullable)
{
	CDescRec *descRecPtr;

	if (m_DescCount > colnumber - 1)
	{
		descRecPtr = m_DescRecCollect[colnumber-1];
		SQLCharset = descRecPtr->m_SQLCharset;
		SQLDataType = descRecPtr->m_SQLDataType;
		SQLDatetimeCode = descRecPtr->m_SQLDatetimeCode;
		SQLMaxLength = descRecPtr->m_SQLMaxLength;
		SQLPrecision = descRecPtr->m_SQLPrecision;
		SQLUnsigned = descRecPtr->m_SQLUnsigned;
		SQLNullable = descRecPtr->m_DescNullable;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CDesc::getDescRowRecData(long colnumber,
	SQLINTEGER& SQLCharset, SQLSMALLINT& SQLDataType, 
	SQLSMALLINT& SQLDatetimeCode, SQLINTEGER& SQLMaxLength, 
	SQLSMALLINT& SQLPrecision, SQLSMALLINT& SQLUnsigned,
	SQLSMALLINT& SQLNullable)
{
	CDescRec *descRecPtr;

	if (m_DescCount > colnumber - 1)
	{
		descRecPtr = m_DescRecCollect[colnumber-1];
		SQLCharset = descRecPtr->m_SQLCharset;
		SQLDataType = descRecPtr->m_SQLDataType;
		SQLDatetimeCode = descRecPtr->m_SQLDatetimeCode;
		SQLMaxLength = descRecPtr->m_SQLMaxLength;
		SQLPrecision = descRecPtr->m_SQLPrecision;
		SQLUnsigned = descRecPtr->m_SQLUnsigned;
		SQLNullable = descRecPtr->m_DescNullable;
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CDesc::getDescRowRecData(long colnumber, SQLSMALLINT& SQLDataType, 
					SQLINTEGER& SQLMaxLength, SQLSMALLINT& SQLNullable)
{
	CDescRec *descRecPtr;

	if (m_DescCount > colnumber - 1)
	{
		descRecPtr = m_DescRecCollect[colnumber-1];
		SQLDataType = descRecPtr->m_SQLDataType;
		SQLMaxLength = descRecPtr->m_SQLMaxLength;
		SQLNullable = descRecPtr->m_DescNullable;
		return TRUE;
	}
	else
		return FALSE;
}

