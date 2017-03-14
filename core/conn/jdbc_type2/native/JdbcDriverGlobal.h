/**************************************************************************
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
**************************************************************************/

#ifndef JDBCDRIVERGLOBAL_H
#define JDBCDRIVERGLOBAL_H

#include "jni.h"
#include "Debug.h"
// Error Codes, May be we need to internationalize these error messages later
typedef enum JNILAYER_ERROR_CODES
{
    UNKNOWN_ERROR = 0,						// sqlcode base is 29250
    PROGRAMMING_ERROR,                      // 29251
    QUERY_CANCELLED_ERROR,                  // 29252
    NUMERIC_VALUE_OUT_OF_RANGE_ERROR,       // 29253
    STRING_DATA_TRUNCATED_ERROR,            // 29254
    TMF_ERROR,                              // 29255
    SYSTEM_CATALOG_ERROR,                   // 29256
    PARAMETER_NOT_SET_ERROR,                // 29257
    INCONSISTENT_TRANSACTION_ERROR,         // 29258
    MODULE_ERROR,                           // 29259
    INVALID_HANDLE_ERROR,                   // 29260
    NO_ERROR_MESSAGE_ERROR,                 // 29261
    INVALID_SQL_STRING_ERROR,               // 29262
    INVALID_STMT_LABEL_ERROR,               // 29263
    INVALID_MODULE_NAME_ERROR,              // 29264
    UNSUPPORTED_ENCODING_ERROR,             // 29265
	DATATYPE_NOT_SUPPPORTED_ERROR,			// 29266
	JVM_MEM_ALLOC_ERROR,				// 29267
	RESTRICTED_DATATYPE_ERROR,              // 29268
    INVALID_DATA_BUFFER_ERROR,               // 29269
    CUSTOM_ERROR = -1						// Error defined by caller
} JNILAYER_ERROR_CODES;

typedef struct Charset_def
{
	jint		charset;			// SQL/MX supported char sets (from sqlcli.h)
	const char	*encodingName;		// Normal C encoding name
	jstring		encodingNameJava;	// Normal Java encoding name
	jboolean	useDefaultEncoding;	// Allows a "default" value for the encoding
} Charset_def;

typedef struct JNICache_def
{
	jmethodID		sqlExceptionConstructorId;
	jmethodID		setNextExceptionMethodId;
	jmethodID		sqlWarningConstructorId;
	jmethodID		setNextWarningMethodId;
	jmethodID		sqlWarningMethodId;
	jmethodID		SQLMXDescConstructorId;
	jmethodID		prepareOutputsMethodId;
	jmethodID		execDirectOutputsMethodId;
	jmethodID		execRSOutputsMethodId;
	jmethodID		execDirectBatchOutputsMethodId;
	jmethodID		executeOutputsMethodId;
	jmethodID		fetchOutputsMethodId;
	jmethodID		SQLMXResultSetConstructorId;
	jfieldID		wasNullFieldId;
	jmethodID		setDataTrunctionMethodId;
	jmethodID		getSQLOctetLengthMethodId;
	jmethodID		getSQLDataTypeMethodId;
	jmethodID		getPrecisionMethodId;
	jmethodID		getScaleMethodId;
	jmethodID		getSqlDatetimeCodeMethodId;
	jmethodID		setCurrentTxidStmtMethodId;
      jmethodID		setCurrentStmtIdMethodId; //perf
	jmethodID		setCurrentTxidRSMethodId;
	jmethodID		setCurrentTxidDBMMethodId;
	jclass			sqlExceptionClass;
	jclass			sqlWarningClass;
	jclass			SQLMXDescClass;
	jclass			SQLMXResultSetClass;
	jclass			objectClass;
	jclass			stringClass;
	jclass			wrapperClass;
	jclass			byteArrayClass;
    jclass          SQLbyteArrayClass;
	jmethodID		RSgetSQLDataTypeMethodId;
	jmethodID		RSgetPrecisionMethodId;
	jmethodID		RSgetScaleMethodId;
	jmethodID		RSgetSignedMethodId;
	jmethodID		RSgetSqlDatetimeCodeMethodId;
	jmethodID		getBytesMethodId;
	jmethodID		getBytesEncodedMethodId;
	jmethodID		stringConstructorId;
	jmethodID		executeCallOutputsMethodId;
	jmethodID		RSgetFSDataTypeMethodId;
	jmethodID		getOutFSDataTypeMethodId;
	jmethodID		getOutScaleMethodId;
	jmethodID		getFSDataTypeMethodId;
	jmethodID		wrapperConstructorId;
	jfieldID		wrapperDataTypeFieldId;
	jfieldID		wrapperIsNullFieldId;
	jfieldID		wrapperObjectFieldId;
	jfieldID		wrapperBytesFieldId;
    jfieldID        wrapperSQLBytesFieldId;
	jfieldID		wrapperByteFieldId;
	jfieldID		wrapperShortFieldId;
	jfieldID		wrapperIntFieldId;
	jfieldID		wrapperLongFieldId;
	jfieldID		wrapperFloatFieldId;
	jfieldID		wrapperDoubleFieldId;
	jfieldID		wrapperBooleanFieldId;
	jfieldID		wrapperSetNeededFieldId;
	jmethodID		wrapperSetupObjectsMethodId;
	jmethodID       getSqlStmtTypeMethodId;         // Soln No.10-091103-5969
	jmethodID       wrapperconvertBigDecimalToSQLBigNum; // Big Num Changes
	jmethodID       wrapperconvertSQLBigNumToBigDecimal2; // Big Num Changes

	jclass			ResultSetInfoClass;				// SPJ ResultSet
	jfieldID		SPJRS_ctxHandleFieldID;			// SPJ ResultSet
	jfieldID		SPJRS_stmtIDFieldID;			// SPJ ResultSet
	jfieldID		SPJRS_stmtClosedFieldID;		//10-060831-8723
	jclass          SQLMXConnectionClass;           // Soln No.10-091103-5969

	Charset_def		*charsetInfo;
	int				totalCharsets;
	jint			defaultCharset;

	~JNICache_def()
	{
        MEMORY_DELETE_ARRAY(charsetInfo);
#ifdef TRACING_MEM_LEAK
        LogMemLeak();
#endif
	}
} JNICache_def;

extern char *gJNILayerErrorMsgs[];
extern const char *defaultEncodingOption;

extern JNICache_def gJNICache;
#ifdef TRACING_MEM_LEAK
extern CMemInfoMap gMemInfoMap;
#endif
extern JNIEnv *gJEnv;

#ifdef NSK_PLATFORM
//typedef long long Int64;
#else
typedef __int64 Int64;
#endif

#endif /* JDBCDRIVERGLOBAL_H */
