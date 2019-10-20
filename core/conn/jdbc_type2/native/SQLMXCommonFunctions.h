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
//
//

#ifndef SQLMXCOMMONFUNCTIONS_H
#define SQLMXCOMMONFUNCTIONS_H

#include <jni.h>
#include <CSrvrStmt.h>
#include <SqlInterface.h>

#define _SQLDT_NUM_BIG_U  155   /* BigNum unsigned NUMERIC datatype */
                                /* NUMERIC(N) where N > 9           */
#define _SQLDT_NUM_BIG_S  156   /* BigNum signed NUMERIC datatype   */
struct WrapperInfoStruct
{
	JNIEnv *jenv;
	jobject wrapper;
	jsize totalColumns;
	jbyteArray dataTypeObject;
	jbyte *dataType;
	jbooleanArray setNeededObject;
	jboolean *setNeeded;
	jbooleanArray isNullObject;
	jboolean *isNull;
	jobjectArray bytesValueObject;
	jbyteArray bytesArrayObject;
	jbyte *bytesArray;
	jbyteArray byteValueObject;
	jbyte *byteValue;
	jshortArray shortValueObject;
	jshort *shortValue;
	jintArray intValueObject;
	jint *intValue;
	jlongArray longValueObject;
	jlong *longValue;
	jfloatArray floatValueObject;
	jfloat *floatValue;
	jdoubleArray doubleValueObject;
	jdouble *doubleValue;
	jbooleanArray booleanValueObject;
	jboolean *booleanValue;
	jobjectArray objectValueObject;
	jobject objectValue;
    jobjectArray SQLbytesValueObject;
    jbyteArray SQLbytesArrayObject;
    jbyte *SQLbytesArray;

};

extern void throwSetConnectionException(JNIEnv *jenv, ExceptionStruct *exception_);

extern void throwSQLException(JNIEnv *jenv, const odbc_SQLSvc_SQLError *SQLError);

extern void throwSQLException(JNIEnv *jenv, short errorNumber, const char *errorMessage, const char *errorSqlState, 
						int nativeCode = 0);

extern void throwSQLException(JNIEnv *jenv, const ERROR_DESC_LIST_def *SQLError);

extern void setSQLWarning(JNIEnv *jenv, jobject jobj, ERROR_DESC_LIST_def *sqlWarning);

extern void setSQLWarning(JNIEnv *jenv, jobject jobj, short errorNumber, const char *errorMessage, const char *errorSqlState, 
						int nativeCode = 0);

extern jobjectArray NewDescArray(JNIEnv *jenv, jobject jobj, SQLItemDescList_def *desc, jint offset);

extern void setExecuteCallOutputs(JNIEnv *jenv, jobject jobj, 
						 SRVR_STMT_HDL  *pSrvrStmt, 
						short returnResultSet, jint txid);

extern void setExecuteDirectOutputs(JNIEnv *jenv, jobject jobj, SQLItemDescList_def *outputDesc, 
								 long rowsAffected, jint txid, jlong stmtId);

extern void setExecuteRSOutputs(JNIEnv *jenv, jobject jobj, SQLItemDescList_def *outputDesc, 
								jint txid, jlong RSstmtId, jint RSindex);

extern void setExecuteOutputs(JNIEnv *jenv, jobject jobj, SRVR_STMT_HDL *pSrvrStmt,
							  ROWS_COUNT_LIST_def *rowCount,
							  jint totalRowCount, jint txid);

extern jobject setFetchOutputs(JNIEnv *jenv, jobject jobj, SRVR_STMT_HDL *pSrvrStmt,
							   SQLValueList_def  *outputValueList,
							   long rowsAffected, BOOL endOfData, BOOL doCallBack, jint txid);

jobject setGetSQLCatalogOutputs(JNIEnv *jenv, jobject jobj, 
									SRVR_STMT_HDL *pSrvrStmt,
									SQLItemDescList_def *outputDesc, 
									char *catStmtLabel, jint txid,
									long rowsAffected, BOOL endOfData,
									SQLValueList_def *outputValueList,
									jlong stmtId);

extern void setPrepareOutputs(JNIEnv *jenv, jobject jobj, SQLItemDescList_def *inputDesc,
                              SQLItemDescList_def *outputDesc, jint txid, jlong stmtId,
							  jint inputParamOffset);

short ConvertDecimalToChar(jint sqlDataType, void *srcDataPtr, jint srcLength, 
								jshort srcScale, char *cTmpBuf);

char *convertInt64toAscii(Int64 n, short scale, char *buff);

BOOL convertJavaToSQL(jobject jobj, jint paramNumber, struct WrapperInfoStruct *wrapperInfo, jint sqlDataType, 
					jint sqlDatetimeCode,
					BYTE *targetDataPtr,
					jint targetLength, jint precision, jint scale, jboolean isSigned,
					jint FSDataType, jint charSet, jstring iso88591Encoding, jint vcIndLength);

short ConvertSoftDecimalToDouble(jint sqlDataType, void *srcDataPtr, jint srcLength, 
								jint srcScale, char *cTmpBuf);

char *rTrim(char *string);

extern jint fillInSQLValues(JNIEnv *jenv, jobject jobj, SRVR_STMT_HDL  *pSrvrStmt,
                            jint paramRowNumber, jint paramRowCount, jint paramCount,
                            jobject paramValues, jstring iso88591Encoding);

BOOL setWrapper(struct WrapperInfoStruct *wrapperInfo, jobject jobj, SRVR_STMT_HDL *pSrvrStmt, jint columnIndex, SQLValue_def *SQLValue,
				   BOOL isIndexParam = FALSE);

extern jobject getSQLCatalogsInfo(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId, 
			jint txid, jboolean autoCommit, jint txnMode,
			short	catalogAPI,
			jstring catalogNm,
			jstring schemaNm, jstring tableNm, jstring tableTypeList,
			jstring columnNm, jint columnType, jint rowIdScope, long nullable,
			long uniqueness, jint accuracy, jshort sqlType, jint metadataId,
			jstring fkcatalogNm, jstring fkschemaNm, jstring fktableNm);


extern void throwTransactionException(JNIEnv *jenv, jint error_code);

extern BOOL cacheJNIObjects(JNIEnv *jenv);

BOOL useDefaultCharsetEncoding(JNIEnv *jenv, jobject jobj, jint charset, jstring iso88591Encoding);

jstring getCharsetEncodingJava(JNIEnv *jenv, long charset, jstring iso88591Encoding);

const char *getCharsetEncoding(jint charset);

jint getCharset(const char *encoding);

//Start Soln No:10-091103-5969
jboolean getSqlStmtType(unsigned char* sql);
//End Soln No. :10-091103-5969


extern long SQLMXStatement_EXECUTE_FAILED(void);
extern long SQLMXStatement_SUCCESS_NO_INFO(void);
#endif
