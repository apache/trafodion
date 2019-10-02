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
// MODULE: SQLMXCommonFunctions.cpp
//

#include <string>
#include <platform_ndcs.h>
#include <stdlib.h>
#include <wchar.h>
#include <errno.h>
#include <time.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include "sqlcli.h"
#include "JdbcDriverGlobal.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXConnection.h"
#include "org_apache_trafodion_jdbc_t2_DataWrapper.h"
#include "SQLMXCommonFunctions.h"
#include "CoreCommon.h"
#include "SrvrCommon.h"
#include "SrvrOthers.h"
#include "Debug.h"
#include "GlobalInformation.h"
#include "org_apache_trafodion_jdbc_t2_T2Driver.h"  //spjrs

static const int INVALID_TRANSACTION_STATE = -1;

static Charset_def CHARSET_INFORMATION[] = {
	{ SQLCHARSETCODE_ISO88591,	"ISO8859_1",	NULL,	NULL},
	{ SQLCHARSETCODE_KANJI,		"Shift_JIS",	NULL,	NULL},
	{ SQLCHARSETCODE_KSC5601,	"EUC-KR",		NULL,	NULL},
	{ SQLCHARSETCODE_SJIS,		"SJIS",			NULL,	NULL},
//	{ SQLCHARSETCODE_UCS2,		"UTF-16BE",		NULL,	NULL},
	{ SQLCHARSETCODE_UCS2,		"UTF-16LE",		NULL,	NULL},
    { SQLCHARSETCODE_EUCJP,     "EUCJP",        NULL,   NULL},
    { SQLCHARSETCODE_GB18030,   "GB18030",      NULL,   NULL},
    { SQLCHARSETCODE_UTF8,      "UTF-8",        NULL,   NULL},
    { SQLCHARSETCODE_GB2312,    "GB2312",       NULL,   NULL},
	{ 0,						NULL,			NULL,	NULL}};

	static bool createWrapper(struct WrapperInfoStruct *wrapperInfo, JNIEnv *jenv, jsize totalColumns, bool allocateWrapper)
	{
		FUNCTION_ENTRY("createWrapper",
			("size=%ld, allocateWrapper=%s",
			totalColumns,
			DebugBoolStr(allocateWrapper)));
		memset(wrapperInfo,0,sizeof(*wrapperInfo));
		wrapperInfo->jenv = jenv;
		wrapperInfo->totalColumns = totalColumns;
		if (allocateWrapper)
		{
			wrapperInfo->wrapper = wrapperInfo->jenv->NewObject(gJNICache.wrapperClass,gJNICache.wrapperConstructorId, totalColumns);

			if (wrapperInfo->wrapper == NULL)
			{
				// Could not allocate JVM memory.
				FUNCTION_RETURN_NUMERIC(FALSE,("%s",DebugBoolStr(FALSE)));
			}
		}
		FUNCTION_RETURN_NUMERIC(TRUE,("%s",DebugBoolStr(TRUE)));
	}

	static void updateWrapperElements(struct WrapperInfoStruct *wrapperInfo)
	{
		FUNCTION_ENTRY("updateWrapperElements",
			("wrapper=0x%08x, wrapperInfo->totalColumns=%ld",
			wrapperInfo->wrapper,
			wrapperInfo->totalColumns));
		if (wrapperInfo->dataTypeObject)
			JNI_SetByteArrayRegion(wrapperInfo->jenv,wrapperInfo->dataTypeObject,0,wrapperInfo->totalColumns,wrapperInfo->dataType);
		if (wrapperInfo->setNeededObject)
			JNI_SetBooleanArrayRegion(wrapperInfo->jenv,wrapperInfo->setNeededObject,0,wrapperInfo->totalColumns,wrapperInfo->setNeeded);
		if (wrapperInfo->longValueObject)
			JNI_SetLongArrayRegion(wrapperInfo->jenv,wrapperInfo->longValueObject,0,wrapperInfo->totalColumns,wrapperInfo->longValue);
		if (wrapperInfo->floatValueObject)
			JNI_SetFloatArrayRegion(wrapperInfo->jenv,wrapperInfo->floatValueObject,0,wrapperInfo->totalColumns,wrapperInfo->floatValue);
		if (wrapperInfo->doubleValueObject)
			JNI_SetDoubleArrayRegion(wrapperInfo->jenv,wrapperInfo->doubleValueObject,0,wrapperInfo->totalColumns,wrapperInfo->doubleValue);
		if (wrapperInfo->SQLbytesArrayObject)
            JNI_SetByteArrayRegion(wrapperInfo->jenv,wrapperInfo->SQLbytesArrayObject,0,wrapperInfo->totalColumns,wrapperInfo->SQLbytesArray);
		FUNCTION_RETURN_VOID((NULL));
	}

	static jbyte getWrapperDataType(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperDataType",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->dataTypeObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->dataTypeObject = (jbyteArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperDataTypeFieldId);
			DEBUG_ASSERT(wrapperInfo->dataTypeObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->dataType = JNI_GetByteArrayElements(wrapperInfo->jenv,wrapperInfo->dataTypeObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->dataType!=NULL,("GetByteArrayElements() failed"));
		}
		FUNCTION_RETURN_NUMERIC(wrapperInfo->dataType[paramNumber],
			("dataType=%s",WrapperDataTypeStr(wrapperInfo->dataType[paramNumber])));
	}

	static void setWrapperDataType(struct WrapperInfoStruct *wrapperInfo, jint paramNumber, jbyte value)
	{
		FUNCTION_ENTRY("setWrapperDataType",
			("wrapper=0x%08x, paramNumber=%ld, value=%s",
			wrapperInfo->wrapper,
			paramNumber,
			WrapperDataTypeStr(value)));

		// If the dataType array object is not allocated, just call getWrapperDataType()
		//    to allocate it.
		if (wrapperInfo->dataTypeObject==NULL) getWrapperDataType(wrapperInfo, paramNumber);
		wrapperInfo->dataType[paramNumber] = value;
		if (wrapperInfo->setNeededObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->setNeededObject = (jbooleanArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperSetNeededFieldId);
			DEBUG_ASSERT(wrapperInfo->setNeededObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->setNeeded = JNI_GetBooleanArrayElements(wrapperInfo->jenv,wrapperInfo->setNeededObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->setNeeded!=NULL,("GetBooleanArrayElements() failed"));
		}
		wrapperInfo->setNeeded[paramNumber] = true;
		FUNCTION_RETURN_VOID((NULL));
	}

	static jboolean getWrapperIsNull(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperIsNull",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->isNullObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->isNullObject = (jbooleanArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperIsNullFieldId);
			DEBUG_ASSERT(wrapperInfo->isNullObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->isNull = JNI_GetBooleanArrayElements(wrapperInfo->jenv,wrapperInfo->isNullObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->isNull!=NULL,("GetBooleanArrayElements() failed"));
		}
		FUNCTION_RETURN_NUMERIC(wrapperInfo->isNull[paramNumber],(NULL));

	}

	static jbyte *getWrapperBytes(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperBytes",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		jboolean isCopy;
		if (wrapperInfo->bytesValueObject==NULL)
		{
			wrapperInfo->bytesValueObject = (jobjectArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperBytesFieldId);
			DEBUG_ASSERT(wrapperInfo->bytesValueObject!=NULL,("GetObjectField() failed"));
		}
		if (wrapperInfo->bytesArrayObject)
			JNI_ReleaseByteArrayElements(wrapperInfo->jenv,wrapperInfo->bytesArrayObject,wrapperInfo->bytesArray,JNI_ABORT);
		wrapperInfo->bytesArrayObject = (jbyteArray) JNI_GetObjectArrayElement(wrapperInfo->jenv,wrapperInfo->bytesValueObject,paramNumber);
		DEBUG_ASSERT(wrapperInfo->bytesArrayObject!=NULL,("GetObjectArrayElement() failed"));
		wrapperInfo->bytesArray = JNI_GetByteArrayElements(wrapperInfo->jenv,wrapperInfo->bytesArrayObject, &isCopy);
		DEBUG_ASSERT(wrapperInfo->bytesArray!=NULL,("GetByteArrayElements() failed"));
		FUNCTION_RETURN_PTR(wrapperInfo->bytesArray,(NULL));
	}

	static void setupWrapperBytes(struct WrapperInfoStruct *wrapperInfo)
	{
		// This method is implemented in DataWrapper.setupBytes() also.  Any changes need to be made in both places
		FUNCTION_ENTRY("setupWrapperBytes",
			("wrapper=0x%08x",
			wrapperInfo->wrapper));

		DEBUG_ASSERT(wrapperInfo->bytesValueObject==NULL,("Bytes array object is not NULL"));
		wrapperInfo->bytesValueObject = JNI_NewObjectArray(wrapperInfo->jenv,wrapperInfo->totalColumns,gJNICache.byteArrayClass,NULL);
		DEBUG_ASSERT(wrapperInfo->bytesValueObject!=NULL,("NewObjectArray() failed"));
		JNI_SetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperBytesFieldId,wrapperInfo->bytesValueObject);
		FUNCTION_RETURN_VOID((NULL));
	}

	static void setWrapperBytes(struct WrapperInfoStruct *wrapperInfo, jint paramNumber, jbyteArray value)
	{
		FUNCTION_ENTRY("setWrapperBytes",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->bytesValueObject==NULL)
		{
			wrapperInfo->bytesValueObject = (jobjectArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperBytesFieldId);
			if (wrapperInfo->bytesValueObject==NULL)
			{
				// Need to setup bytes array
				setupWrapperBytes(wrapperInfo);
			}
		}
		JNI_SetObjectArrayElement(wrapperInfo->jenv,wrapperInfo->bytesValueObject,paramNumber,value);
		setWrapperDataType(wrapperInfo,paramNumber,org_apache_trafodion_jdbc_t2_DataWrapper_BYTES);
		FUNCTION_RETURN_VOID((NULL));
	}

    static void setupWrapperSQLBytes(struct WrapperInfoStruct *wrapperInfo)
    {
        // This method is implemented in DataWrapper.setupSQLBytes() also.  Any changes need to be made in both places
        FUNCTION_ENTRY("setupWrapperSQLBytes",
            ("wrapper=0x%08x",
            wrapperInfo->wrapper));

        DEBUG_ASSERT(wrapperInfo->SQLbytesArrayObject==NULL,("SQLBytes array object is not NULL"));
        wrapperInfo->SQLbytesValueObject = JNI_NewObjectArray(wrapperInfo->jenv,wrapperInfo->totalColumns,gJNICache.SQLbyteArrayClass,NULL);
        DEBUG_ASSERT(wrapperInfo->SQLbytesValueObject!=NULL,("NewObjectArray() failed"));
        JNI_SetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperSQLBytesFieldId,wrapperInfo->SQLbytesValueObject);
        FUNCTION_RETURN_VOID((NULL));
    }

    static void setWrapperSQLBytes(struct WrapperInfoStruct *wrapperInfo, jint paramNumber, jbyteArray value)
    {
        FUNCTION_ENTRY("setWrapperSQLBytes",
            ("wrapper=0x%08x, paramNumber=%ld",
            wrapperInfo->wrapper,
            paramNumber));
        if (wrapperInfo->SQLbytesValueObject==NULL)
        {
            wrapperInfo->SQLbytesValueObject = (jobjectArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperSQLBytesFieldId);
            if (wrapperInfo->SQLbytesValueObject==NULL)
            {
                // Need to setup bytes array
                setupWrapperSQLBytes(wrapperInfo);
            }
        }
        JNI_SetObjectArrayElement(wrapperInfo->jenv,wrapperInfo->SQLbytesValueObject,paramNumber,value);
        setWrapperDataType(wrapperInfo,paramNumber,org_apache_trafodion_jdbc_t2_DataWrapper_BYTES);
        FUNCTION_RETURN_VOID((NULL));
    }
//============================
	// Big Num Changes
	static jbyteArray convertBigDecimalToSQLBigNum(struct WrapperInfoStruct *wrapperInfo, jint paramNumber, jint scale, jint targetLength)
	{
		FUNCTION_ENTRY("convertBigDecimalToSQLBigNum",
			("wrapper=0x%08x, paramNumber=%ld, scale=%ld, targetLength=%ld",
			wrapperInfo->wrapper,
			paramNumber, scale, targetLength));
		// We are incrementing the param number because its zero based in Native side
		jbyteArray returnValue = (jbyteArray)wrapperInfo->jenv->CallObjectMethod(wrapperInfo->wrapper,
					gJNICache.wrapperconvertBigDecimalToSQLBigNum, paramNumber+1, scale, targetLength);
		FUNCTION_RETURN_PTR((jbyteArray)returnValue,(NULL));
	}
	static jstring convertSQLBigNumToBigDecimal2(struct WrapperInfoStruct *wrapperInfo, jbyteArray columnValue, jint scale)
	{
		FUNCTION_ENTRY("convertSQLBigNumToBigDecimal2",
			("wrapper=0x%08x, columnValue=0x%08x, scale=%ld",
			wrapperInfo->wrapper, columnValue,
			scale));
		// Call the method.
		jstring returnValue = (jstring)wrapperInfo->jenv->CallObjectMethod(wrapperInfo->wrapper,
			gJNICache.wrapperconvertSQLBigNumToBigDecimal2, columnValue, scale);
		FUNCTION_RETURN_PTR((jstring)returnValue,(NULL));
	}
	// Big Num Changes
	static jobject getWrapperObject(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperObject",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->objectValueObject==NULL)
		{
			wrapperInfo->objectValueObject = (jobjectArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperObjectFieldId);
			DEBUG_ASSERT(wrapperInfo->objectValueObject!=NULL,("GetObjectField() failed"));
		}
		wrapperInfo->objectValue = JNI_GetObjectArrayElement(wrapperInfo->jenv,wrapperInfo->objectValueObject,paramNumber);
		DEBUG_ASSERT(wrapperInfo->objectValue!=NULL,("GetObjectArrayElement() failed"));
		FUNCTION_RETURN_PTR(wrapperInfo->objectValue,(NULL));
	}

	static bool setWrapperObject(struct WrapperInfoStruct *wrapperInfo, jint paramNumber, jobject value, jbyte data_type)
	{
		FUNCTION_ENTRY("setWrapperObject",
			("wrapper=0x%08x, paramNumber=%ld, data_type=%s",
			wrapperInfo->wrapper,
			paramNumber,
			WrapperDataTypeStr(data_type)));

		if (data_type==org_apache_trafodion_jdbc_t2_DataWrapper_STRING)
			DEBUG_OUT(DEBUG_LEVEL_ENTRY,("String Value=%s",DebugJString(wrapperInfo->jenv,value)));

		if (wrapperInfo->objectValueObject==NULL)
		{
			wrapperInfo->objectValueObject = (jobjectArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperObjectFieldId);
			if (wrapperInfo->objectValueObject==NULL)
			{
				wrapperInfo->jenv->CallVoidMethod(wrapperInfo->wrapper, gJNICache.wrapperSetupObjectsMethodId);
				wrapperInfo->objectValueObject = (jobjectArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperObjectFieldId);
				DEBUG_ASSERT(wrapperInfo->objectValueObject!=NULL,("GetObjectField() failed"));
				if (wrapperInfo->objectValueObject==NULL) // Could not allocate JVM memory.
					FUNCTION_RETURN_NUMERIC(FALSE,("setWrapperObject - JNI_GetObjectField() failed"));
			}
		}
		JNI_SetObjectArrayElement(wrapperInfo->jenv,wrapperInfo->objectValueObject,paramNumber,value);
		setWrapperDataType(wrapperInfo,paramNumber,data_type);
		FUNCTION_RETURN_NUMERIC(TRUE,("%s",DebugBoolStr(TRUE)));
	}

	static jbyte getWrapperByte(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperByte",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->byteValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->byteValueObject = (jbyteArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperByteFieldId);
			DEBUG_ASSERT(wrapperInfo->byteValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->byteValue = JNI_GetByteArrayElements(wrapperInfo->jenv,wrapperInfo->byteValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->byteValue!=NULL,("GetByteArrayElements() failed"));
		}
		FUNCTION_RETURN_NUMERIC(wrapperInfo->byteValue[paramNumber],(NULL));
	}

	static jshort getWrapperShort(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperShort",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->shortValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->shortValueObject = (jshortArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperShortFieldId);
			DEBUG_ASSERT(wrapperInfo->shortValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->shortValue = JNI_GetShortArrayElements(wrapperInfo->jenv,wrapperInfo->shortValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->shortValue!=NULL,("GetShortArrayElements() failed"));
		}
		FUNCTION_RETURN_NUMERIC(wrapperInfo->shortValue[paramNumber],(NULL));
	}

	static jint getWrapperInt(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperInt",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->intValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->intValueObject = (jintArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperIntFieldId);
			DEBUG_ASSERT(wrapperInfo->intValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->intValue = JNI_GetIntArrayElements(wrapperInfo->jenv,wrapperInfo->intValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->intValue!=NULL,("GetIntArrayElements() failed"));
		}
		FUNCTION_RETURN_NUMERIC(wrapperInfo->intValue[paramNumber],(NULL));
	}

	static jlong getWrapperLong(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperLong",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->longValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->longValueObject = (jlongArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperLongFieldId);
			DEBUG_ASSERT(wrapperInfo->longValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->longValue = JNI_GetLongArrayElements(wrapperInfo->jenv,wrapperInfo->longValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->longValue!=NULL,("GetLongArrayElements() failed"));
		}
		FUNCTION_RETURN_INT64(wrapperInfo->longValue[paramNumber],(NULL));
	}

	static void setWrapperLong(struct WrapperInfoStruct *wrapperInfo, jint paramNumber, jlong value)
	{
		FUNCTION_ENTRY("setWrapperLong",
			("wrapper=0x%08x, paramNumber=%ld, value=%Ld",
			wrapperInfo->wrapper,
			paramNumber,
			value));
		if (wrapperInfo->longValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->longValueObject = (jlongArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperLongFieldId);
			DEBUG_ASSERT(wrapperInfo->longValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->longValue = JNI_GetLongArrayElements(wrapperInfo->jenv,wrapperInfo->longValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->longValue!=NULL,("GetLongArrayElements() failed"));
		}
		wrapperInfo->longValue[paramNumber] = value;
		setWrapperDataType(wrapperInfo,paramNumber,org_apache_trafodion_jdbc_t2_DataWrapper_LONG);
		FUNCTION_RETURN_VOID((NULL));
	}

	static jfloat getWrapperFloat(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperFloat",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->floatValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->floatValueObject = (jfloatArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperFloatFieldId);
			DEBUG_ASSERT(wrapperInfo->floatValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->floatValue = JNI_GetFloatArrayElements(wrapperInfo->jenv,wrapperInfo->floatValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->floatValue!=NULL,("GetFloatArrayElements() failed"));
		}
		FUNCTION_RETURN_FLOAT(wrapperInfo->floatValue[paramNumber],(NULL));
	}

	static void setWrapperFloat(struct WrapperInfoStruct *wrapperInfo, jint paramNumber, jfloat value)
	{
		FUNCTION_ENTRY("setWrapperFloat",
			("wrapper=0x%08x, paramNumber=%ld, value=%f",
			wrapperInfo->wrapper,
			paramNumber,
			value));
		if (wrapperInfo->floatValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->floatValueObject = (jfloatArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperFloatFieldId);
			DEBUG_ASSERT(wrapperInfo->floatValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->floatValue = JNI_GetFloatArrayElements(wrapperInfo->jenv,wrapperInfo->floatValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->floatValue!=NULL,("GetFloatArrayElements() failed"));
		}
		wrapperInfo->floatValue[paramNumber] = value;
		setWrapperDataType(wrapperInfo,paramNumber,org_apache_trafodion_jdbc_t2_DataWrapper_FLOAT);
		FUNCTION_RETURN_VOID((NULL));
	}

	static jdouble getWrapperDouble(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperDouble",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->doubleValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->doubleValueObject = (jdoubleArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperDoubleFieldId);
			DEBUG_ASSERT(wrapperInfo->doubleValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->doubleValue = JNI_GetDoubleArrayElements(wrapperInfo->jenv,wrapperInfo->doubleValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->doubleValue!=NULL,("GetDoubleArrayElements() failed"));
		}
		FUNCTION_RETURN_DOUBLE(wrapperInfo->doubleValue[paramNumber],(NULL));
	}

	static void setWrapperDouble(struct WrapperInfoStruct *wrapperInfo, jint paramNumber, jdouble value)
	{
		FUNCTION_ENTRY("setWrapperDouble",
			("wrapper=0x%08x, paramNumber=%ld, value=%g",
			wrapperInfo->wrapper,
			paramNumber,
			value));
		if (wrapperInfo->doubleValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->doubleValueObject = (jdoubleArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperDoubleFieldId);
			DEBUG_ASSERT(wrapperInfo->doubleValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->doubleValue = JNI_GetDoubleArrayElements(wrapperInfo->jenv,wrapperInfo->doubleValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->doubleValue!=NULL,("GetDoubleArrayElements() failed"));
		}
		wrapperInfo->doubleValue[paramNumber] = value;
		setWrapperDataType(wrapperInfo,paramNumber,org_apache_trafodion_jdbc_t2_DataWrapper_DOUBLE);
		FUNCTION_RETURN_VOID((NULL));
	}

	static jboolean getWrapperBoolean(struct WrapperInfoStruct *wrapperInfo, jint paramNumber)
	{
		FUNCTION_ENTRY("getWrapperBoolean",
			("wrapper=0x%08x, paramNumber=%ld",
			wrapperInfo->wrapper,
			paramNumber));
		if (wrapperInfo->booleanValueObject==NULL)
		{
			jboolean isCopy;
			wrapperInfo->booleanValueObject = (jbooleanArray) JNI_GetObjectField(wrapperInfo->jenv,wrapperInfo->wrapper,gJNICache.wrapperBooleanFieldId);
			DEBUG_ASSERT(wrapperInfo->booleanValueObject!=NULL,("GetObjectField() failed"));
			wrapperInfo->booleanValue = JNI_GetBooleanArrayElements(wrapperInfo->jenv,wrapperInfo->booleanValueObject, &isCopy);
			DEBUG_ASSERT(wrapperInfo->booleanValue!=NULL,("GetBooleanArrayElements() failed"));
		}
		FUNCTION_RETURN_NUMERIC(wrapperInfo->booleanValue[paramNumber],(NULL));
	}

	static void cleanupWrapperBytesArray(struct WrapperInfoStruct *wrapperInfo)
	{
		FUNCTION_ENTRY("cleanupWrapperBytesArray",
			("wrapper=0x%08x",
			wrapperInfo->wrapper));
		if (wrapperInfo->bytesArrayObject)
		{
			JNI_ReleaseByteArrayElements(wrapperInfo->jenv,wrapperInfo->bytesArrayObject,wrapperInfo->bytesArray,JNI_ABORT);
			wrapperInfo->bytesArrayObject = NULL;
			wrapperInfo->bytesArray = NULL;
		}
		FUNCTION_RETURN_VOID((NULL));
	}

    static void cleanupWrapperSQLBytesArray(struct WrapperInfoStruct *wrapperInfo)
    {
        FUNCTION_ENTRY("cleanupWrapperSQLBytesArray",
            ("wrapper=0x%08x",
            wrapperInfo->wrapper));
        if (wrapperInfo->SQLbytesArrayObject)
        {
            JNI_ReleaseByteArrayElements(wrapperInfo->jenv,wrapperInfo->SQLbytesArrayObject,wrapperInfo->SQLbytesArray,JNI_ABORT);
            wrapperInfo->SQLbytesArrayObject = NULL;
            wrapperInfo->SQLbytesArray = NULL;
        }
        FUNCTION_RETURN_VOID((NULL));
    }
//==============
	static void cleanupWrapperInfo(struct WrapperInfoStruct *wrapperInfo)
	{
		FUNCTION_ENTRY("cleanupWrapperInfo",
			("wrapper=0x%08x",
			wrapperInfo->wrapper));
		if (wrapperInfo->dataTypeObject)
		{
			JNI_ReleaseByteArrayElements(wrapperInfo->jenv,wrapperInfo->dataTypeObject,wrapperInfo->dataType,JNI_ABORT);
			wrapperInfo->dataTypeObject = NULL;
			wrapperInfo->dataType = NULL;
		}
		if (wrapperInfo->setNeededObject)
		{
			JNI_ReleaseBooleanArrayElements(wrapperInfo->jenv,wrapperInfo->setNeededObject,wrapperInfo->setNeeded,JNI_ABORT);
			wrapperInfo->setNeededObject = NULL;
			wrapperInfo->setNeeded = NULL;
		}
		if (wrapperInfo->isNullObject)
		{
			JNI_ReleaseBooleanArrayElements(wrapperInfo->jenv,wrapperInfo->isNullObject,wrapperInfo->isNull,JNI_ABORT);
			wrapperInfo->isNullObject = NULL;
			wrapperInfo->isNull = NULL;
		}
		if (wrapperInfo->byteValueObject)
		{
			JNI_ReleaseByteArrayElements(wrapperInfo->jenv,wrapperInfo->byteValueObject,wrapperInfo->byteValue,JNI_ABORT);
			wrapperInfo->byteValueObject = NULL;
			wrapperInfo->byteValue = NULL;
		}
		if (wrapperInfo->shortValueObject)
		{
			JNI_ReleaseShortArrayElements(wrapperInfo->jenv,wrapperInfo->shortValueObject,wrapperInfo->shortValue,JNI_ABORT);
			wrapperInfo->shortValueObject = NULL;
			wrapperInfo->shortValue = NULL;
		}
		if (wrapperInfo->intValueObject)
		{
			JNI_ReleaseIntArrayElements(wrapperInfo->jenv,wrapperInfo->intValueObject,wrapperInfo->intValue,JNI_ABORT);
			wrapperInfo->intValueObject = NULL;
			wrapperInfo->intValue = NULL;
		}
		if (wrapperInfo->longValueObject)
		{
			JNI_ReleaseLongArrayElements(wrapperInfo->jenv,wrapperInfo->longValueObject,wrapperInfo->longValue,JNI_ABORT);
			wrapperInfo->longValueObject = NULL;
			wrapperInfo->longValue = NULL;
		}
		if (wrapperInfo->floatValueObject)
		{
			JNI_ReleaseFloatArrayElements(wrapperInfo->jenv,wrapperInfo->floatValueObject,wrapperInfo->floatValue,JNI_ABORT);
			wrapperInfo->floatValueObject = NULL;
			wrapperInfo->floatValue = NULL;
		}
		if (wrapperInfo->doubleValueObject)
		{
			JNI_ReleaseDoubleArrayElements(wrapperInfo->jenv,wrapperInfo->doubleValueObject,wrapperInfo->doubleValue,JNI_ABORT);
			wrapperInfo->doubleValueObject = NULL;
			wrapperInfo->doubleValue = NULL;
		}
		if (wrapperInfo->booleanValueObject)
		{
			JNI_ReleaseBooleanArrayElements(wrapperInfo->jenv,wrapperInfo->booleanValueObject,wrapperInfo->booleanValue,JNI_ABORT);
			wrapperInfo->booleanValueObject = NULL;
			wrapperInfo->booleanValue = NULL;
		}
		cleanupWrapperBytesArray(wrapperInfo);
		wrapperInfo->bytesValueObject = NULL;

        cleanupWrapperSQLBytesArray(wrapperInfo);
        wrapperInfo->SQLbytesValueObject = NULL;

		FUNCTION_RETURN_VOID((NULL));
	}

	BOOL dataLengthNotExceeded(jint charSet, jint dataLen, jint allocLen)
	{
		FUNCTION_ENTRY("dataLengthNotExceeded",
			("charset=%s, dataLen=%ld, allocLen=%ld",
			getCharsetEncoding(charSet),
			dataLen,
			allocLen));

		bool lengthOK = TRUE;
		switch (charSet)
		{
		case SQLCHARSETCODE_ISO88591:
			if (dataLen >= allocLen)
				lengthOK = FALSE;
			break;
		case SQLCHARSETCODE_KANJI:
		case SQLCHARSETCODE_KSC5601:
		case SQLCHARSETCODE_UCS2:
			if (dataLen > allocLen)
				lengthOK = FALSE;
			break;
		}
		FUNCTION_RETURN_NUMERIC(lengthOK,(NULL));
	}

	void throwSQLException(JNIEnv *jenv, const odbc_SQLSvc_SQLError *SQLError)
	{
		FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ERROR,"throwSQLException",("jenv=0x%08x, SQLError=0x%08x",
			jenv,
			SQLError));
		jthrowable			sqlException;
		jthrowable			sqlExceptionHead;
		jstring				sqlMessage;
		jstring				sqlState;
		unsigned long		curErrorNo;
		ERROR_DESC_def		*error_desc_def;

		// Clear any pending exceptions
		// Otherwise, unexpected behaviour
		JNI_ExceptionClear(jenv);

		if (SQLError->errorList._length == 0)
		{
			throwSQLException(jenv, NO_ERROR_MESSAGE_ERROR, NULL, "HY000");
			FUNCTION_RETURN_VOID(("NO_ERROR_MESSAGE_ERROR"));
		}

		for (curErrorNo = 0, error_desc_def = SQLError->errorList._buffer;
			curErrorNo < SQLError->errorList._length ; curErrorNo++, error_desc_def++)

		{
			DEBUG_OUT(DEBUG_LEVEL_ERROR,("sqlstate=%s, errorText=%s",
				DebugString(error_desc_def->sqlstate),
				DebugString(error_desc_def->errorText)));

			sqlMessage = jenv->NewStringUTF(error_desc_def->errorText);
			sqlState = jenv->NewStringUTF(error_desc_def->sqlstate);
			sqlException = (jthrowable)jenv->NewObject(gJNICache.sqlExceptionClass,
				gJNICache.sqlExceptionConstructorId, sqlMessage,sqlState,
				error_desc_def->sqlcode);
			if(curErrorNo == 0)
				sqlExceptionHead = sqlException;
			else
				jenv->CallVoidMethod(sqlExceptionHead, gJNICache.setNextExceptionMethodId,
				sqlException);
		}
		JNI_Throw(jenv,sqlExceptionHead);
		FUNCTION_RETURN_VOID((NULL));
	}

	void throwSQLException(JNIEnv *jenv, short errorNumber, const char *errorMessage,
		const char *errorSqlState, int nativeCode)
	{
		FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ERROR,"throwSQLException",("jenv=0x%08x, errorNumber=%d, errorMessage=%s, errorSqlState=%s, nativeCode=%d",
			jenv,
			errorNumber,
			DebugString(errorMessage),
			DebugString(errorSqlState),
			nativeCode));
		jthrowable		sqlException;
		jstring			sqlMessage;
		jstring			sqlState;
		jint			sqlcode;
		char			lcErrorMessage[2048];

		// Clear any pending exceptions
		// Otherwise, unexpected behaviour
		JNI_ExceptionClear(jenv);
		if (errorNumber>=0)
		{
			strcpy(lcErrorMessage, gJNILayerErrorMsgs[errorNumber]);
			DEBUG_OUT(DEBUG_LEVEL_ERROR,("gJNILayerErrorMsgs[%d]=%s",
				errorNumber,
				gJNILayerErrorMsgs[errorNumber]));
		} else lcErrorMessage[0] = 0;
		if (errorMessage != NULL)
			strcat(lcErrorMessage, errorMessage);
		sqlMessage = jenv->NewStringUTF(lcErrorMessage);
		sqlState = jenv->NewStringUTF(errorSqlState);
		if (nativeCode == 0)
			sqlcode = -(29250+errorNumber);
		else
			sqlcode = nativeCode;
		sqlException = (jthrowable)jenv->NewObject(gJNICache.sqlExceptionClass,
			gJNICache.sqlExceptionConstructorId, sqlMessage, sqlState,
			sqlcode);
		JNI_Throw(jenv,sqlException);
		FUNCTION_RETURN_VOID(("lcErrorMessage=%s",lcErrorMessage));
	}

	void throwSQLException(JNIEnv *jenv, const ERROR_DESC_LIST_def *SQLError)
	{
		FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_ERROR,"throwSQLException",("jenv=0x%08x, SQLError[]=0x%08x",
			jenv,
			SQLError));
		jthrowable			sqlException;
		jthrowable			sqlExceptionHead;
		jstring				sqlMessage;
		jstring				sqlState;
		unsigned long		curErrorNo;
		ERROR_DESC_def		*error_desc_def;

		// Clear any pending exceptions
		// Otherwise, unexpected behaviour
		JNI_ExceptionClear(jenv);

		if (SQLError->_length == 0)
		{
			throwSQLException(jenv, NO_ERROR_MESSAGE_ERROR, NULL, "HY000");
			FUNCTION_RETURN_VOID(("NO_ERROR_MESSAGE_ERROR"));
		}
		for (curErrorNo = 0, error_desc_def = SQLError->_buffer;
			curErrorNo < SQLError->_length ; curErrorNo++, error_desc_def++)
		{
			DEBUG_OUT(DEBUG_LEVEL_ERROR,("sqlstate=%s, errorText=%s",
				DebugString(error_desc_def->sqlstate),
				DebugString(error_desc_def->errorText)));
			sqlMessage = jenv->NewStringUTF(error_desc_def->errorText);
			sqlState = jenv->NewStringUTF(error_desc_def->sqlstate);
			sqlException = (jthrowable)jenv->NewObject(gJNICache.sqlExceptionClass,
				gJNICache.sqlExceptionConstructorId, sqlMessage,sqlState,
				error_desc_def->sqlcode);
			if(curErrorNo == 0)
				sqlExceptionHead = sqlException;
			else
				jenv->CallVoidMethod(sqlExceptionHead, gJNICache.setNextExceptionMethodId,
				sqlException);
		}
		JNI_Throw(jenv,sqlExceptionHead);
		FUNCTION_RETURN_VOID((NULL));
	}

	void setSQLWarning(JNIEnv *jenv, jobject jobj, ERROR_DESC_LIST_def *sqlWarning)
	{
		FUNCTION_ENTRY("setSQLWarning",("jenv=0x%08x, jobj=0x%08x",
			jenv,
			jobj));

		unsigned long		curErrorNo;
		ERROR_DESC_def		*error_desc_def;
		jobject				sqlWarningLeaf;
		jobject				sqlWarningHead;
		jstring				sqlMessage;
		jstring				sqlState;

		if (sqlWarning->_length == 0)
		{
			setSQLWarning(jenv, jobj, NO_ERROR_MESSAGE_ERROR, NULL, "01000");
			FUNCTION_RETURN_VOID(("NO_ERROR_MESSAGE_ERROR"));
		}

		for (curErrorNo = 0, error_desc_def = sqlWarning->_buffer;
			curErrorNo < sqlWarning->_length ; curErrorNo++, error_desc_def++)
		{
			sqlMessage = jenv->NewStringUTF(error_desc_def->errorText);
			sqlState = jenv->NewStringUTF(error_desc_def->sqlstate);
			sqlWarningLeaf = jenv->NewObject(gJNICache.sqlWarningClass,
				gJNICache.sqlWarningConstructorId, sqlMessage, sqlState,
				error_desc_def->sqlcode);
			if(curErrorNo == 0)
				sqlWarningHead = sqlWarningLeaf;
			else
				jenv->CallVoidMethod(sqlWarningHead, gJNICache.setNextWarningMethodId,
				sqlWarningLeaf);
		}
		jenv->CallVoidMethod(jobj, gJNICache.sqlWarningMethodId, sqlWarningHead);
		FUNCTION_RETURN_VOID((NULL));
	}

	void setSQLWarning(JNIEnv *jenv, jobject jobj, short errorNumber, const char *errorMessage,
		const char *errorSqlState, int nativeCode)
	{
		FUNCTION_ENTRY("setSQLWarning",("jenv=0x%08x, jobj=0x%08x, errorNumber=%d, errorMessage=%s, errorSqlState=%s, nativeCode=%d",
			jenv,
			jobj,
			errorNumber,
			DebugString(errorMessage),
			DebugString(errorSqlState),
			nativeCode));
		jstring			 sqlMessage;
		jstring			 sqlState;
		char				lcErrorMessage[2048];
		jobject			 sqlWarning;
		jint				sqlcode;

		strcpy(lcErrorMessage, gJNILayerErrorMsgs[errorNumber]);
		if (errorMessage != NULL)
			strcat(lcErrorMessage, errorMessage);
		sqlMessage = jenv->NewStringUTF(lcErrorMessage);
		sqlState = jenv->NewStringUTF(errorSqlState);
		if (nativeCode == 0)
			sqlcode = 29250+errorNumber;
		else
			sqlcode = nativeCode;
		sqlWarning = jenv->NewObject(gJNICache.sqlWarningClass,
			gJNICache.sqlWarningConstructorId, sqlMessage, sqlState,
			sqlcode);
		jenv->CallVoidMethod(jobj, gJNICache.sqlWarningMethodId, sqlWarning);
		FUNCTION_RETURN_VOID((NULL));
	}

	jobjectArray NewDescArray(JNIEnv *jenv, jobject jobj, SQLItemDescList_def *desc, jint offset)
	{
		FUNCTION_ENTRY("NewDescArray",("jenv=0x%08x, jobj=0x%08x, desc=0x%08x, offset=%ld",
			jenv,
			jobj,
			desc,
			offset));
		unsigned long		index;
		SQLItemDesc_def		*SQLDesc;
		jstring				colName;
		jstring				colLabel;
		jstring				catalogName;
		jstring				schemaName;
		jstring				tableName;
		jobjectArray		SQLMXDescArray = NULL;
		jobject				SQLMXDesc = NULL;

		if ((desc->_length-offset )<= 0)
			FUNCTION_RETURN_PTR(NULL,("(desc->_length(%ld) - offset(%ld))<= 0",
			desc->_length, offset));

		SQLMXDescArray = JNI_NewObjectArray(jenv,desc->_length-offset, gJNICache.SQLMXDescClass, NULL);

		for (index = offset; index < desc->_length ; index++)
		{
			SQLDesc = (SQLItemDesc_def *)desc->_buffer + index;
			DEBUG_OUT(DEBUG_LEVEL_DATA,("Adding descriptor.  Column Name='%s' Column Heading='%s'",
				DebugString(SQLDesc->colNm), DebugString(SQLDesc->colLabel)));
			colName = jenv->NewStringUTF(SQLDesc->colNm);
			catalogName = jenv->NewStringUTF(SQLDesc->catalogNm);
			schemaName = jenv->NewStringUTF(SQLDesc->schemaNm);
			tableName = jenv->NewStringUTF(SQLDesc->tableNm);
			colLabel = jenv->NewStringUTF(SQLDesc->colLabel);
			SQLMXDesc = jenv->NewObject(gJNICache.SQLMXDescClass, gJNICache.SQLMXDescConstructorId,
				SQLDesc->dataType, SQLDesc->datetimeCode, SQLDesc->maxLen,
				SQLDesc->precision, SQLDesc->scale, SQLDesc->nullInfo, colName, SQLDesc->signType,
				SQLDesc->ODBCDataType, SQLDesc->ODBCPrecision, SQLDesc->SQLCharset,
				SQLDesc->ODBCCharset, catalogName, schemaName, tableName, SQLDesc->fsDataType,
				SQLDesc->intLeadPrec,
				SQLDesc->paramMode,
				colLabel);
			JNI_SetObjectArrayElement(jenv,SQLMXDescArray, index-offset, SQLMXDesc);
		}
		FUNCTION_RETURN_PTR(SQLMXDescArray,("desc->_length = %ld",desc->_length));
	}

	void setPrepareOutputs(JNIEnv *jenv, jobject jobj, SQLItemDescList_def *inputDesc,
		SQLItemDescList_def *outputDesc, jint txid, jlong stmtId,
		jint inputParamOffset)
	{
		FUNCTION_ENTRY("setPrepareOutputs",("inputDesc=0x%08x, outputDesc=0x%08x, txid=0x%08x, stmtId=0x%08x, inputParamOffset=%ld",
			inputDesc,
			outputDesc,
			txid,
			stmtId,
			inputParamOffset));

		jobjectArray SQLMXOutputDescArray = NULL;
		jobjectArray SQLMXInputDescArray = NULL;
		SQLMXOutputDescArray = NewDescArray(jenv, jobj, outputDesc, 0);
		SQLMXInputDescArray = NewDescArray(jenv, jobj, inputDesc, inputParamOffset);
		jenv->CallVoidMethod(jobj, gJNICache.prepareOutputsMethodId, SQLMXInputDescArray,
			SQLMXOutputDescArray, txid, inputDesc->_length-inputParamOffset, 0, stmtId);
		FUNCTION_RETURN_VOID((NULL));
	}

	void setExecuteDirectOutputs(JNIEnv *jenv, jobject jobj, SQLItemDescList_def *outputDesc,
		long rowsAffected, jint txid, jlong stmtId)
	{
		FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"setExecuteDirectOutputs",("jenv=0x%08x, jobj==0x%08x, outputDesc=0x%08x, rowsAffected=%ld, txid=0x%08x, stmtId=0x%08x",
			jenv,
			jobj,
			outputDesc,
			rowsAffected,
			txid,
			stmtId));
		jobjectArray		SQLMXOuputDescArray = NULL;

		SQLMXOuputDescArray = NewDescArray(jenv, jobj, outputDesc, 0);
		jobject		fetchedRows = NULL;
		jint		fetchedRowCount = 0;

		jint		stmtType = INVALID_SQL_QUERY_STMT_TYPE; // Initialize to invalid type

		SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *) stmtId;

		if (pSrvrStmt != NULL)
		{
			{
				stmtType = pSrvrStmt->getSqlQueryStatementType();
				DEBUG_OUT(DEBUG_LEVEL_CLI|DEBUG_LEVEL_STMT,("SQL Query Statement Type=%s",
					CliDebugSqlQueryStatementType(stmtType)));

				if(stmtType == SQL_SELECT_UNIQUE)
				{
					// For unique selects, the endOfData flag indicates if
					//   the select returned a row or not
					if(pSrvrStmt->endOfData) fetchedRowCount = 0;
					else fetchedRowCount = 1;

					// Go get the Row[] object to return to the Java layer
					fetchedRows = setFetchOutputs(jenv, pSrvrStmt->resultSetObject, pSrvrStmt,
						&pSrvrStmt->outputValueList, fetchedRowCount,
						TRUE, FALSE, txid);
				}
			}
		}
		// Return the execute results and any result set
		//   information that may have been generated
		jenv->CallVoidMethod(jobj, gJNICache.execDirectOutputsMethodId, SQLMXOuputDescArray, rowsAffected,
			fetchedRows, fetchedRowCount, txid, stmtId, stmtType);

		FUNCTION_RETURN_VOID((NULL));
	}

	void setExecuteRSOutputs(JNIEnv *jenv, jobject jobj, SQLItemDescList_def *outputDesc,
		jint txid, jlong rsStmtId, jint rsIndex)
	{
		FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"setExecuteRSOutputs",
			("jenv=0x%08x, jobj=0x%08x, outputDesc=0x%08x, txid=0x%08x, rsStmtId=0x%08x, rsIndex=%ld",
			jenv,
			jobj,
			outputDesc,
			txid,
			rsStmtId,
			rsIndex));

		jobjectArray		SQLMXOuputDescArray = NULL;

		SQLMXOuputDescArray = NewDescArray(jenv, jobj, outputDesc, 0);

		jint	stmtType = INVALID_SQL_QUERY_STMT_TYPE; // Initialize to invalid type

		SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *) rsStmtId;

		stmtType = pSrvrStmt->getSqlQueryStatementType();
		DEBUG_OUT(DEBUG_LEVEL_CLI,("SQL Query Statement Type=%s",
			CliDebugSqlQueryStatementType(stmtType)));

		// Return the execute open spjrs results and any result set
		//  information that may have been generated
		jenv->CallVoidMethod(jobj, gJNICache.execRSOutputsMethodId, SQLMXOuputDescArray,
			txid, rsStmtId, rsIndex);

		FUNCTION_RETURN_VOID((NULL));
	}


	void setExecuteOutputs(JNIEnv *jenv, jobject jobj, SRVR_STMT_HDL *pSrvrStmt,
		ROWS_COUNT_LIST_def *rowCount, jint totalRowCount, jint txid)
	{
		FUNCTION_ENTRY("setExecuteOutputs",("... pSrvrStmt=0x%08x, rowCount=0x%08x, totalRowCount=%ld, txid=0x%08x",
			pSrvrStmt,
			rowCount,
			totalRowCount,
			txid));

		jintArray	rowCountArray = NULL;
		jint		i;
		jint		fetchedRowCount = 0;
		jobject		fetchedRows=NULL;

		if (rowCount)
		{
			DEBUG_OUT(DEBUG_LEVEL_DATA,("rowCount->_length = %ld", rowCount->_length));
			rowCountArray = JNI_NewIntArray(jenv,rowCount->_length);
			if (rowCount->_length)
				JNI_SetIntArrayRegion(jenv,rowCountArray, 0, rowCount->_length, (jint *)rowCount->_buffer);
		}
		{
			if(pSrvrStmt->getSqlQueryStatementType() == SQL_SELECT_UNIQUE)
			{
				if(pSrvrStmt->endOfData) fetchedRowCount = 0;
				else fetchedRowCount = 1;
				fetchedRows = setFetchOutputs(jenv, pSrvrStmt->resultSetObject, pSrvrStmt,
					&pSrvrStmt->outputValueList, fetchedRowCount,
					TRUE, FALSE, txid);
			}
		}
		jenv->CallVoidMethod(jobj, gJNICache.executeOutputsMethodId, rowCountArray, totalRowCount,
			fetchedRows, fetchedRowCount, txid);
		FUNCTION_RETURN_VOID((NULL));
	}

	jobject setFetchOutputs(JNIEnv *jenv, jobject jobj, SRVR_STMT_HDL *pSrvrStmt,
		SQLValueList_def  *outputValueList,
		long rowsAffected, BOOL endOfData,
		BOOL doCallBack, jint txid)
	{
		FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"setFetchOutputs",("pSrvrStmt=0x%08x, outputValueList=0x%08x, rowsAffected=%ld, endOfData=%d, doCallBack=%d, txid=0x%08x",
			pSrvrStmt,
			outputValueList,
			rowsAffected,
			endOfData,
			doCallBack,
			txid));

		jobjectArray	rowArray;

		long			columnCount;
		long			rowIndex;
		long			columnIndex;
		SQLValue_def	*SQLValue;

		rowArray = JNI_NewObjectArray(jenv,rowsAffected, gJNICache.wrapperClass, NULL);
		if (rowsAffected != 0) columnCount = outputValueList->_length / rowsAffected;
		else columnCount = 0;

		DEBUG_OUT(DEBUG_LEVEL_STMT,("columnCount=%ld, rowsAffected=%ld",columnCount,rowsAffected));

		for (rowIndex = 0; rowIndex < rowsAffected ; rowIndex++)
		{
			struct WrapperInfoStruct wrapperInfo;

			if (!createWrapper(&wrapperInfo,jenv,columnCount,true))
			{
				throwSQLException(jenv, JVM_MEM_ALLOC_ERROR, NULL, "HY000");
				cleanupWrapperInfo(&wrapperInfo);
				FUNCTION_RETURN_PTR(NULL,("setFetchOutputs - JVM_MEM_ALLOC_ERROR"));
			}

			for (columnIndex = 0 ; columnIndex < columnCount ; columnIndex++)
			{
				SQLValue = (SQLValue_def *)outputValueList->_buffer +
					(rowIndex * columnCount) + columnIndex;

				if (SQLValue->dataInd != -1)
				{
					if (!setWrapper(&wrapperInfo, jobj, pSrvrStmt, columnIndex, SQLValue))
					{
						cleanupWrapperInfo(&wrapperInfo);
						FUNCTION_RETURN_PTR(NULL,("setWrapper returned error"));
					}
					DEBUG_OUT(DEBUG_LEVEL_DATA,("Column index %ld wrapper is 0x%08x",
						columnIndex, wrapperInfo.wrapper));
				}
			}
			updateWrapperElements(&wrapperInfo);

			JNI_SetObjectArrayElement(jenv,rowArray, rowIndex, wrapperInfo.wrapper);

			cleanupWrapperInfo(&wrapperInfo);
		}

		// If requested, do the callback to Java.
		// If not, they just wanted the Row[] created.
		if(doCallBack)
		{
			jenv->CallVoidMethod(jobj, gJNICache.fetchOutputsMethodId, rowArray,
				rowsAffected, endOfData, txid);
		}
		FUNCTION_RETURN_PTR(rowArray, (NULL));
	}


	jobject setGetSQLCatalogOutputs(JNIEnv *jenv, jobject jobj, SRVR_STMT_HDL *pSrvrStmt,
		SQLItemDescList_def *outputDesc,
		char *catStmtLabel, jint txid,
		long rowsAffected, BOOL endOfData, SQLValueList_def *outputValueList,
		jlong stmtId)

	{
		FUNCTION_ENTRY("setGetSQLCatalogOutputs",("jenv=0x%08x, jobj=0x%08x, pSrvrStmt=0x%08x, outputDesc=0x%08x, catStmtLabel=%s, txid=0x%08x, rowsAffected=%ld, endOfData=%d, outputValueList=0x%08x, stmtId=0x%08x",
			jenv,
			jobj,
			pSrvrStmt,
			outputDesc,
			DebugString(catStmtLabel),
			txid,
			rowsAffected,
			endOfData,
			outputValueList,
			stmtId));
		jobjectArray		SQLMXOutputDescArray = NULL;
		jstring				stmtLabel;
		jobject				SQLMXResultSet = NULL;
		SQLValueList_def	CopyOutputValueList;
		SQLValue_def		*SQLValue1;
		SQLValue_def		*SQLValue2;
		unsigned long		ValueBufferLen;
		BYTE				*ValueBuffer;
		BYTE				*CopyValueBuffer;
		unsigned long		index;

		SQLMXOutputDescArray = NewDescArray(jenv, jobj, outputDesc, 0);
		stmtLabel = jenv->NewStringUTF(catStmtLabel);
		SQLMXResultSet = jenv->NewObject(gJNICache.SQLMXResultSetClass, gJNICache.SQLMXResultSetConstructorId,
			jobj, SQLMXOutputDescArray, txid, stmtId);
		setFetchOutputs(jenv, SQLMXResultSet, pSrvrStmt, outputValueList, rowsAffected, endOfData, TRUE, txid);

		FUNCTION_RETURN_PTR(SQLMXResultSet,("SQLMXResultSet=0x%08x",SQLMXResultSet));
	}


	void setExecuteCallOutputs(JNIEnv *jenv, jobject jobj,
		SRVR_STMT_HDL  *pSrvrStmt,
		short returnResultSet, jint txid)
	{
		FUNCTION_ENTRY("setExecuteCallOutputs",("... pSrvrStmt=0x%08x, returnResultSet=%d, txid=0x%08x",
			pSrvrStmt,
			returnResultSet,
			txid));
		unsigned long	i;
		SQLValue_def	sqlValue;
		SRVR_DESC_HDL	*IRD;
		BYTE			*dataPtr;
		short			*indPtr;
		struct WrapperInfoStruct wrapperInfo;

		IRD = pSrvrStmt->IRD;

		DEBUG_OUT(DEBUG_LEVEL_STMT,("pSrvrStmt->IRD=0x%08x, pSrvrStmt->columnCount=%ld",
			pSrvrStmt->IRD, pSrvrStmt->columnCount));


		if(!createWrapper(&wrapperInfo,jenv,pSrvrStmt->columnCount,true))
		{
			throwSQLException(jenv, JVM_MEM_ALLOC_ERROR, NULL, "HY000");
			cleanupWrapperInfo(&wrapperInfo);
			FUNCTION_RETURN_VOID(("setExecuteCallOutputs - JVM_MEM_ALLOC_ERROR"));
		}

		if (pSrvrStmt->columnCount > 0)
		{
			for (i = 0 ; i < pSrvrStmt->columnCount ; i++)
			{
				if (IRD[i].paramMode == PARAMETER_MODE_INOUT ||
					IRD[i].paramMode == PARAMETER_MODE_OUT)
				{
					indPtr = (short *)IRD[i].indPtr;
					dataPtr = IRD[i].varPtr;
					sqlValue.dataType = IRD[i].dataType;
					if (indPtr != NULL)
						sqlValue.dataInd = *indPtr;
					else
						sqlValue.dataInd = 0;
					sqlValue.dataValue._buffer = dataPtr;
					getMemoryAllocInfo(sqlValue.dataType, sqlValue.dataCharset, IRD[i].length, IRD[i].vc_ind_length, 0,
						NULL, (int *)&sqlValue.dataValue._length, NULL);
					if (sqlValue.dataInd != -1)
					{
						if (!setWrapper(&wrapperInfo, jobj, pSrvrStmt, i, &sqlValue, TRUE))
						{
							cleanupWrapperInfo(&wrapperInfo);
							FUNCTION_RETURN_VOID(("setWrapper returned error"));
						}
					}
					DEBUG_OUT(DEBUG_LEVEL_DATA,("IRD[%ld] - sqlValue: dataType=%ld, dataInd=0x%08x, dataValue._buffer=0x%08x, dataValue._length=%ld wrapper=0x%08x",
						i,
						sqlValue.dataType,
						sqlValue.dataInd,
						sqlValue.dataValue._buffer,
						sqlValue.dataValue._length,
						wrapperInfo.wrapper));
				}
				else
				{
					DEBUG_OUT(DEBUG_LEVEL_DATA,("IRD[%ld].paramMode(%ld) is not PARAMETER_MODE_INOUT or PARAMETER_MODE_OUT",
						i,
						IRD[i].paramMode));
				}
			}
		}
		DEBUG_OUT(DEBUG_LEVEL_DATA,("Calling Java SQLMXCallableStatement.setExecuteCallOutputs() method"));
		updateWrapperElements(&wrapperInfo);
		jenv->CallVoidMethod(jobj, gJNICache.executeCallOutputsMethodId, wrapperInfo.wrapper, returnResultSet, txid,
			pSrvrStmt->RSMax, pSrvrStmt->RSIndex, pSrvrStmt->isSPJRS);
		cleanupWrapperInfo(&wrapperInfo);
		FUNCTION_RETURN_VOID((NULL));
	}

	char *convertInt64toAscii(Int64 n, short scale, char *buff)
	{
		FUNCTION_ENTRY("convertInt64toAscii",("n=%ld, scale=%d, buff=0x%08x",
			n,
			scale,
			buff));
		char t[100], *c=t, *f=buff;
		int d;
		int s = 0;
		BOOL fixRightMost = FALSE;

		if (n < 0)
		{
			if (n == LLONG_MIN)
			{
				n = n+1;
				fixRightMost = TRUE;
			}
			*(f++) = '-';
			n = -n;
		}

		if (scale > 0)
		{
			while (s < scale)
			{
				d = (int)(n % 10);
				*(c++) = d + '0';
				n = n / 10;
				s++;
			}
			*(c++) = '.';
		}

		do
		{
			d = (int)(n % 10);
			*(c++) = d + '0';
			n = n / 10;
		} while (n > 0) ;

		c--;


		while (c >= t) *(f++) = *(c--);
		if (fixRightMost)
		{
			*(--f) = '8';
			f++;
		}
		*f = '\0';
		FUNCTION_RETURN_PTR(buff,("buff=%s",DebugString(buff)));
	}


	short ConvertDecimalToChar(jint sqlDataType, void *srcDataPtr, jint srcLength,
		jshort srcScale, char *cTmpBuf)
	{
		FUNCTION_ENTRY("ConvertDecimalToChar",("sqlDataType=%s, srcDataPtr=0x%08x, srcLength=%ld, srcScale=%d, cTmpBuf=0x%08x",
			CliDebugSqlTypeCode(sqlDataType),
			srcDataPtr,
			srcLength,
			srcScale,
			cTmpBuf));

		char	*destTempPtr;
		short	i;
		BOOL	leadZero;
		BOOL	valByteCopied;
		BYTE	valByte;	// Sign Bit + first digit


		destTempPtr = cTmpBuf;

		switch(sqlDataType)
		{
		case 151:
			leadZero = TRUE;
			valByteCopied = TRUE;
			*destTempPtr == ((char*)srcDataPtr)[0];
			i = 1;
			break;
		case SQLTYPECODE_DECIMAL_UNSIGNED:
			leadZero = TRUE;
			valByteCopied = TRUE;
			i = 0;
			break;
		case SQLTYPECODE_DECIMAL:
			valByte = (BYTE)(*(BYTE *)srcDataPtr & (BYTE)0x80);
			if (valByte)
				*destTempPtr++ = '-';
			valByte = (BYTE)(*(BYTE *)srcDataPtr & (BYTE) 0x7F);
			if (valByte != '0')
			{
				valByteCopied = FALSE;
				leadZero = FALSE;
			}
			else
			{
				leadZero = TRUE;
				valByteCopied = TRUE;
			}
			i = 1;
			break;
		default:
			FUNCTION_RETURN_NUMERIC(1,("Data Type Default"));
		}
		for ( ; i < (srcLength-srcScale) ; i++)
		{
			if (!(leadZero && ((char *)srcDataPtr)[i] == '0'))
			{
				if (! valByteCopied)
				{
					*destTempPtr++ = valByte;
					valByteCopied = TRUE;
				}
				*destTempPtr++ = ((char *)srcDataPtr)[i];
				leadZero = FALSE;
			}
		}
		if (leadZero)
			*destTempPtr++ = '0';
		if (srcScale > 0)
		{
			if (valByteCopied)
				*destTempPtr++ = '.';
			for (i = (short)(srcLength-srcScale) ; i < srcLength ; i++)
			{
				if (! valByteCopied)
				{
					valByteCopied = TRUE;
					if (srcLength == srcScale)
					{
						*destTempPtr++ = '.';
						*destTempPtr++ = valByte;
						if (++i >= srcLength)
							break;
					}
					else
					{
						*destTempPtr++ = valByte;
						*destTempPtr++ = '.';
					}
				}
				*destTempPtr++ = ((char *)srcDataPtr)[i];
			}
		}
		else
			if (! valByteCopied)
			{
				*destTempPtr++ = valByte;
				valByteCopied = TRUE;
			}
			*destTempPtr = '\0';
			FUNCTION_RETURN_NUMERIC(0,(NULL));
	}

	short ConvertSoftDecimalToDouble(jint sqlDataType, void *srcDataPtr, jint srcLength,
		jint srcScale, char *cTmpBuf)
	{
		// SQLTYPECODE_DECIMAL_LARGE and SQLTYPECODE_DECIMAL_LARGE_UNSIGNED are actually not
		//   supported by SQL, so the datatypes processing is not really required.  Since they
		//   are not supported, I have no way of verifying this routines functionality.  I have corrected
		//   an unititalized variable in this routine, but in looking at the code I think there is
		//   an error in the logic.  If someone does support these types later, you should look at the
		//   handling of the signed values.  For standard signed numeric types, the first byte is a
		//   valid digit in the value with the high bit being the sign.  In this routines handling, the
		//   first byte is skipped in the processing for signed values.  I think this is incorrect.
		//   R Harlow 1/27/2004
		FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_DATA,"ConvertSoftDecimalToDouble",("sqlDataType=%s, srcDataPtr/srcLength=Below, srcScale=%ld, cTmpBuf=0x%08x)",
			CliDebugSqlTypeCode(sqlDataType),
			srcScale,
			cTmpBuf));
		MEMORY_DUMP(DEBUG_LEVEL_DATA,srcDataPtr,srcLength);
		char	*destTempPtr;
		short	i=0;
		BOOL	leadZero = TRUE;

		destTempPtr = cTmpBuf;

		switch(sqlDataType)
		{
		case SQLTYPECODE_DECIMAL_LARGE:
			i = 1;
			break;
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
			break;
		default:
			FUNCTION_RETURN_NUMERIC(1,("Not SQLTYPECODE_DECIMAL_LARGE or SQLTYPECODE_DECIMAL_LARGE_UNSIGNED"));
		}
		for (; i < (srcLength-srcScale) ; i++)
		{
			if (!(leadZero && ((char *)srcDataPtr)[i] == 0))
			{
				*destTempPtr++ = ((char *)srcDataPtr)[i]+'0';
				leadZero = FALSE;
			}
		}
		if (srcScale > 0)
		{
			*destTempPtr++ = '.';
			for (i = (short)(srcLength-srcScale) ; i < srcLength ; i++)
				*destTempPtr++ = ((char *)srcDataPtr)[i]+'0';
		}
		DEBUG_OUT(DEBUG_LEVEL_DATA,("Returning string %s",cTmpBuf));
		FUNCTION_RETURN_NUMERIC(0,(NULL));
	}

	char *rTrim(char *string)
	{
		FUNCTION_ENTRY("rTrim",("string=0x%08x",
			string));
		char *strPtr;

		for (strPtr = string + strlen(string) - 1;
			strPtr >= string && (*strPtr == ' ' || *strPtr == '\t') ;
			*(strPtr--) = '\0');
		FUNCTION_RETURN_PTR(string,("string=%s",DebugString(string)));
	}


	jint fillInSQLValues(JNIEnv *jenv, jobject jobj, SRVR_STMT_HDL  *pSrvrStmt,
		jint paramRowNumber, jint paramRowCount, jint paramCount,
		jobject paramValues, jstring iso88591Encoding)
	{
		jint dataType;
		jint octetLength;
		jint charSet;
		jint dataLen;
		BYTE  *dataPtr;
		BYTE *indPtr;
		jint allocLength;

		FUNCTION_ENTRY("fillInSQLValues",
			("... pSrvrStmt=0x%08x, paramRowNumber=%ld, paramRowCount=%ld, paramCount=%ld, iso88591Encoding=%s ...",
			pSrvrStmt,
			paramRowNumber,
			paramRowCount,
			paramCount,
			DebugJString(jenv,iso88591Encoding)));

		jint			row;
		jint			paramOffset = pSrvrStmt->inputDescParamOffset;

		SRVR_DESC_HDL *IPD = pSrvrStmt->IPD;
		if (paramCount && paramOffset)
		{
			// First desc is rowset size
			DEBUG_ASSERT(IPD[0].dataType==SQLTYPECODE_INTEGER,
				("IPD[0].dataType(%s)!=SQLTYPECODE_INTEGER",
				CliDebugSqlTypeCode(IPD[0].dataType)));
			*((long *)IPD[0].varPtr) = pSrvrStmt->batchRowsetSize;
			IPD[0].indPtr = NULL;
			DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_ROWSET,("First descriptor (varPtr=0x%08x) set to batch rowset size (%ld)",
				IPD[0].varPtr,
				pSrvrStmt->batchRowsetSize));
		}

		for (row=0; row<paramRowCount; row++)
		{
			jint paramNumber;
			struct WrapperInfoStruct wrapperInfo;
			createWrapper(&wrapperInfo,jenv,paramCount,false);

			// Get the data wrapper object
			if (JNI_IsInstanceOf(wrapperInfo.jenv,paramValues,gJNICache.wrapperClass))
			{
				wrapperInfo.wrapper = paramValues;
			}
			else
			{
				// If more than one, it is an array of data wrappers
				wrapperInfo.wrapper = (jobject) JNI_GetObjectArrayElement(jenv,(jobjectArray)paramValues, paramRowNumber+row);
			}

			for (paramNumber = 0; paramNumber < paramCount; paramNumber++)
			{
				jint paramMode = IPD[paramNumber+paramOffset].paramMode;
				jboolean isNull = getWrapperIsNull(&wrapperInfo,paramNumber);

				DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("Param %ld ParamMode=%ld DataType=%s IPD[%ld]",
					paramNumber,
					paramMode,
					CliDebugSqlTypeCode(IPD[paramNumber+paramOffset].dataType),
					paramNumber+paramOffset));

				if (paramMode == PARAMETER_MODE_OUT)
					continue;
				if (IPD[paramNumber+paramOffset].indPtr)
				{
					indPtr = IPD[paramNumber+paramOffset].indPtr + row * sizeof(short);
					if (!isNull)
						*((short *)indPtr) = 0;
					else
						*((short *)indPtr) = -1;
					DEBUG_OUT(DEBUG_LEVEL_DATA,("  indPtr=0x%08x *indPtr=%d",
						indPtr,
						*((short *)indPtr)));
				} else {
					indPtr = NULL;
					DEBUG_OUT(DEBUG_LEVEL_DATA,("  indPtr=NULL"));
				}

				if (isNull && (indPtr==NULL))
				{
					// Value is null in a NOT NULL column
					throwSQLException(jenv, CUSTOM_ERROR,
						"*** ERROR[8421] NULL cannot be assigned to a NOT NULL column.",
						"23000",
						-8421);
					cleanupWrapperInfo(&wrapperInfo);
					FUNCTION_RETURN_NUMERIC((row+1),("Value in row %ld is null in a NOT NULL column",row+1));
				}

				if (!isNull)
				{
					dataType = IPD[paramNumber+paramOffset].dataType;
					octetLength = IPD[paramNumber+paramOffset].length;
					charSet = IPD[paramNumber+paramOffset].charSet;
					getMemoryAllocInfo(dataType, charSet, octetLength, IPD[paramNumber + paramOffset].vc_ind_length, 0,
						NULL, &allocLength, NULL);
					dataPtr = IPD[paramNumber+paramOffset].varPtr + row*allocLength;

					DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("  dataPtr=0x%08x", dataPtr));
					DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("  dataType=%s",CliDebugSqlTypeCode(dataType)));
					DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("  octetLength=%ld", octetLength));

					if (getWrapperDataType(&wrapperInfo,paramNumber)!=org_apache_trafodion_jdbc_t2_DataWrapper_BYTES)
					{
						// Non byte array value
						DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,
							("Non byte array wrapper (%s). IPD[paramNumber(%ld)+paramOffset(%ld)]",
							DebugJavaObjectInfo(jenv,wrapperInfo.wrapper),
							paramNumber,
							paramOffset));
						jint precision			= IPD[paramNumber+paramOffset].precision;
						DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("precision=%ld",precision));

						jint scale				= IPD[paramNumber+paramOffset].scale;
						DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("scale=%ld",scale));

						jint sqlDatetimeCode	= IPD[paramNumber+paramOffset].sqlDatetimeCode;
						DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("sqlDatetimeCode=%ld",sqlDatetimeCode));

						jint FSDataType			= IPD[paramNumber+paramOffset].FSDataType;
						DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("FSDataType=%ld",FSDataType));

						jint charSet			= IPD[paramNumber+paramOffset].charSet;
						DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("charSet=%ld",charSet));

						jint vcIndLength		= IPD[paramNumber+paramOffset].vc_ind_length;
						DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("charSet=%ld",charSet));

						// do the data Conversion
						if (!convertJavaToSQL(jobj, paramNumber, &wrapperInfo, dataType, sqlDatetimeCode,
							dataPtr, allocLength, precision, scale, FALSE,
							FSDataType, charSet, iso88591Encoding, vcIndLength))
						{
							cleanupWrapperInfo(&wrapperInfo);
							FUNCTION_RETURN_NUMERIC(row+1,("convertJavaToSQL() failed in row %ld",row+1));
						}
					}
					else
					{
						// Process byte array
						DEBUG_OUT(DEBUG_LEVEL_DATA,("Byte array wrapper (%s) paramNumber=%ld",
							DebugJavaObjectInfo(jenv,wrapperInfo.wrapper),
							paramNumber));
						jbyte *byteValue = getWrapperBytes(&wrapperInfo,paramNumber);

						dataLen = JNI_GetArrayLength(wrapperInfo.jenv,wrapperInfo.bytesArrayObject);
						DEBUG_OUT(DEBUG_LEVEL_JAVA|DEBUG_LEVEL_DATA,("Byte array length = %ld",dataLen));
						MEMORY_DUMP(DEBUG_LEVEL_JAVA|DEBUG_LEVEL_DATA,byteValue,dataLen);

						switch (dataType)
						{
						case SQLTYPECODE_CHAR:
							// if (allocLength > dataLen)
							if (dataLengthNotExceeded(charSet, dataLen, allocLength))
							{
								memcpy(dataPtr, (const void *)byteValue, dataLen);
                                memset(dataPtr + (dataLen), ' ', allocLength - (dataLen));
                                if (charSet == SQLCHARSETCODE_UCS2)
                                {
                                    // Back fill target buffer with double byte 'space' characters (i.e. 0x00 0x20)
                                    for (int i = (dataLen+1); i < allocLength-dataLen; i+=2)
                                    {
                                        *(dataPtr+i) = 0;
                                    }
                                }
							}
							else
							{
								throwSQLException(jenv, STRING_DATA_TRUNCATED_ERROR, NULL, "22001");
								cleanupWrapperInfo(&wrapperInfo);
								FUNCTION_RETURN_NUMERIC(row+1,("fillInSQLValues() failed in row %ld",row+1));
							}
							break;
						case SQLTYPECODE_VARCHAR:
							if (allocLength > dataLen)
							{
								memcpy(dataPtr, (const void *)byteValue, dataLen);
								*(dataPtr+dataLen) = '\0';
							}
							else
							{
								throwSQLException(jenv, STRING_DATA_TRUNCATED_ERROR, NULL, "22001");
								cleanupWrapperInfo(&wrapperInfo);
								FUNCTION_RETURN_NUMERIC(row+1,("fillInSQLValues() failed in row %ld",row+1));
							}
							break;
						case SQLTYPECODE_VARCHAR_WITH_LENGTH:
						case SQLTYPECODE_VARCHAR_LONG:
						case SQLTYPECODE_DATETIME:
						case SQLTYPECODE_INTERVAL:
						case SQLTYPECODE_CLOB:
						case SQLTYPECODE_BLOB:
							if ((IPD[paramNumber + paramOffset].vc_ind_length == 4) && (allocLength > dataLen + sizeof(int)))
							{
							    *(unsigned int *)dataPtr = (int)dataLen;
							    memcpy(dataPtr + sizeof(int), (const void *)byteValue, dataLen);
							}
							else if (allocLength > dataLen + sizeof(short))
							{
								*(unsigned short *)dataPtr = (short)dataLen;
								memcpy(dataPtr+sizeof(short), (const void *)byteValue, dataLen);
							}
							else
							{
								throwSQLException(jenv, STRING_DATA_TRUNCATED_ERROR, NULL, "22001");
								cleanupWrapperInfo(&wrapperInfo);
								FUNCTION_RETURN_NUMERIC(row+1,("fillInSQLValues() failed in row %ld",row+1));
							}
							break;
						default:
							throwSQLException(jenv, PROGRAMMING_ERROR, gJNILayerErrorMsgs[PROGRAMMING_ERROR],
								"HY000");
							cleanupWrapperInfo(&wrapperInfo);
							FUNCTION_RETURN_NUMERIC(row+1,("fillInSQLValues() failed in row %ld",row+1));
						}
						cleanupWrapperBytesArray(&wrapperInfo);
					}
//---------------------------------------------------dataPtr--------

				} else DEBUG_OUT(DEBUG_LEVEL_DATA,("  Parameter is NULL"));
			}
			cleanupWrapperInfo(&wrapperInfo);
		}
		FUNCTION_RETURN_NUMERIC(0,(NULL));
	}

	long jGetDigitCount(Int64 value)
	{
		FUNCTION_ENTRY("jGetDigitCount",("value=%ld",
			value));
		static Int64 decValue[] = {0,
			9,
			99,
			999,
			9999,
			99999,
			999999,
			9999999,
			99999999,
			999999999,
			9999999999,
			99999999999,
			999999999999,
			9999999999999,
			99999999999999,
			999999999999999,
			9999999999999999,
			99999999999999999,
			999999999999999999};
		Int64 value1;

		if (value < 0)
			value1 = -value;
		else
			value1 = value;

		for (int i = 4; i <= 16; i += 4)
		{
			if (value1 <= decValue[i])
			{
				if (value1 <= decValue[i-3])
					FUNCTION_RETURN_NUMERIC((i-3),("i-3"));
				if (value1 <= decValue[i-2])
					FUNCTION_RETURN_NUMERIC((i-2),("i-2"));
				if (value1 <= decValue[i-1])
					FUNCTION_RETURN_NUMERIC((i-1),("i-1"));
				FUNCTION_RETURN_NUMERIC(i,("i"));
			}
		}
		if (value1 <= decValue[17])
			FUNCTION_RETURN_NUMERIC(17,("value1 <= decValue[17]"));
		if (value1 <= decValue[18])
			FUNCTION_RETURN_NUMERIC(18,("value1 <= decValue[18]"));
		FUNCTION_RETURN_NUMERIC(19,("value1 > decValue[18]"));
	}

	BOOL convertJavaToSQL(jobject jobj, jint paramNumber, struct WrapperInfoStruct *wrapperInfo, jint sqlDataType,
		jint sqlDatetimeCode,
		BYTE *targetDataPtr, jint targetLength, jint precision, jint scale,
		jboolean isSigned, jint FSDataType, jint charSet, jstring iso88591Encoding, jint vcIndLength)
	{
		FUNCTION_ENTRY("convertJavaToSQL",
			("... paramNumber=%ld, wrapper=0x%08x, sqlDataType=%s, charSet=%s, targetLength=%ld, precision=%ld, iso88591Encoding=%s ...",
			paramNumber,
			wrapperInfo->wrapper,
			CliDebugSqlTypeCode(sqlDataType),
			getCharsetEncoding(charSet),
			targetLength,
			precision,
			DebugJString(wrapperInfo->jenv,iso88591Encoding)));

		long			dataLen = 0;
		Int64			integralPart;
		Int64			decimalPart;
		Int64			tempVal64;
		long			i;
		BOOL			negative;
		double			dTmp;		// Used to store the converted floating point string to double
		char			*errorCharPtr;
		long			decimalDigits;
		long			integralDigits;
		long			scaleOffset;
		Int64			tempScaleVal64;
		short			decLeadingZeros;
		BOOL			retcode = FALSE;
		char            *charptr = NULL;  // Big Num Changes
		jbyteArray		stringByteArray;
		jboolean		isCopy = FALSE;
		char			tmpBuf[100];
		char			fmtBuf[50];
		jstring			encoding;
		const char		*nParamValue = NULL;

		if (getWrapperIsNull(wrapperInfo,paramNumber))
		{
			throwSQLException(wrapperInfo->jenv, PARAMETER_NOT_SET_ERROR, NULL, "07002", 0);
			FUNCTION_RETURN_NUMERIC(FALSE,("parameter is NULL"));
		}

		// Linux port- moving here from case block because of compile errors
		jbyteArray bigNumAsBytes;
		jint bytesLength;
		BYTE *pB;
		
		switch(sqlDataType)
		{
		case SQLTYPECODE_CHAR:
		case SQLTYPECODE_VARCHAR:
		case SQLTYPECODE_VARCHAR_LONG:
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		case SQLTYPECODE_CLOB:
		case SQLTYPECODE_BLOB:		
			if ( useDefaultCharsetEncoding(wrapperInfo->jenv, jobj, charSet, iso88591Encoding) )
			{
				encoding = NULL;
				DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("Using default encoding instead of '%s'",
					getCharsetEncoding(charSet)));
			}
			else
			{
				encoding = getCharsetEncodingJava(wrapperInfo->jenv, charSet, iso88591Encoding);
				DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,
					("encoding='%s' (from getCharsetEncodingJava)",DebugJString(wrapperInfo->jenv, encoding)));
			}

            if (encoding)
                stringByteArray = (jbyteArray)wrapperInfo->jenv->CallObjectMethod(getWrapperObject(wrapperInfo,paramNumber),
                        gJNICache.getBytesEncodedMethodId, encoding);
            else
                stringByteArray = (jbyteArray)wrapperInfo->jenv->CallObjectMethod(getWrapperObject(wrapperInfo,paramNumber),
                        gJNICache.getBytesMethodId);
            if (stringByteArray==NULL)
            {
                throwSQLException(wrapperInfo->jenv, PARAMETER_NOT_SET_ERROR, NULL, "07002", 0);
                FUNCTION_RETURN_NUMERIC(FALSE,("getBytesEncodedMethod failed"));
            }
            if ((nParamValue = (const char *)JNI_GetByteArrayElements(wrapperInfo->jenv,stringByteArray, &isCopy)) == NULL)
            {
                throwSQLException(wrapperInfo->jenv, PARAMETER_NOT_SET_ERROR, NULL, "07002", 0);
                FUNCTION_RETURN_NUMERIC(FALSE,("GetByteArrayElements() failed"));
            }
            dataLen = JNI_GetArrayLength(wrapperInfo->jenv,stringByteArray);

            DEBUG_OUT(DEBUG_LEVEL_UNICODE,("Shift_JIS byte array dataLen ='%ld'", dataLen));
            MEMORY_DUMP(DEBUG_LEVEL_UNICODE,nParamValue,dataLen);
            break;


		case SQLTYPECODE_IEEE_REAL:
			*(float *)targetDataPtr = getWrapperFloat(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_FLOAT: targetData = '%.8g'",*(float *)targetDataPtr));
			retcode				 = TRUE;
			goto func_exit;
			break;
		case SQLTYPECODE_IEEE_FLOAT:
		case SQLTYPECODE_IEEE_DOUBLE:
			*(double *)targetDataPtr = getWrapperDouble(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_DOUBLE: targetData = '%.17g'",*(double *)targetDataPtr));
			retcode				  = TRUE;
			goto func_exit;
			break;
		case SQLTYPECODE_INTEGER:
			*(int *)targetDataPtr = getWrapperInt(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_INTEGER: targetData = '%d'",*(int *)targetDataPtr));
			retcode               = TRUE;
			goto func_exit;
			break;
		case SQLTYPECODE_INTEGER_UNSIGNED:
			*(unsigned int *)targetDataPtr = getWrapperInt(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_INTEGER: targetData = '%d'",*(int *)targetDataPtr));
			retcode               = TRUE;
			goto func_exit;
			break;
		case SQLTYPECODE_LARGEINT:
			*(long long *)targetDataPtr = getWrapperLong(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_LARGEINT: targetData = '%Ld'",*(long long *)targetDataPtr));
			retcode                = TRUE;
			goto func_exit;
			break;
		case SQLTYPECODE_SMALLINT:
			*(short *)targetDataPtr = getWrapperShort(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_SMALLINT: targetData = '%hd'",*(short *)targetDataPtr));
			retcode                 = TRUE;
			goto func_exit;
			break;
		case SQLTYPECODE_SMALLINT_UNSIGNED:
			*(unsigned short *)targetDataPtr = getWrapperShort(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_SMALLINT: targetData = '%hd'",*(short *)targetDataPtr));
			retcode                 = TRUE;
			goto func_exit;
			break;
			/* ****************************************************************************************************************
			* NOTE: SQL/MX uses three different storage sizes to store the SQLTYPECODE_NUMERIC and SQLTYPECODE_NUMERIC_UNSIGNED
			*       column data types. Depending upon the precision used to create the column. The store size can be either a
			*       16 bit, 32 bit or 64 bit number. Also remember that inorder to write the full range of number into a
			*       unsigned column type, the next larger setter method has to be used. For example, if I want to write data
			*       to a unsigned 16 bit column, I would have to use the setInteger method and not the setShort. Java does
			*       not have any unsigned data type and the max value for short is 32,767 were as the max value for an unsigned
			*       short would be 65,535.
			*
			*       The Java code has be updated to allow using next larger setter method to write to an unsigned column type.
			* ****************************************************************************************************************/
		case SQLTYPECODE_NUMERIC:
		case SQLTYPECODE_NUMERIC_UNSIGNED:
			switch (FSDataType) {                //TYPE_FS descriptor fields
		case _SQLDT_16BIT_S:
			*(short *)targetDataPtr = (short) getWrapperLong(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_16BIT_S: targetData = '%hd', scale = '%ld'",*(short *)targetDataPtr, scale));
			retcode                 = TRUE;
			goto func_exit;
			break;
		case _SQLDT_16BIT_U:
			*(unsigned short *)targetDataPtr = (unsigned short) getWrapperLong(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_16BIT_U: targetData = '%hu', scale = '%ld'",*(unsigned short *)targetDataPtr, scale));
			retcode                 = TRUE;
			goto func_exit;
			break;
		case _SQLDT_32BIT_S:
			*(int *)targetDataPtr = (int) getWrapperLong(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_32BIT_S: targetData = '%d', scale = '%ld'",*(int *)targetDataPtr, scale));
			retcode                 = TRUE;
			goto func_exit;
			break;
		case _SQLDT_32BIT_U:
			*(unsigned int *)targetDataPtr = (unsigned int) getWrapperLong(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_32BIT_U: targetData = '%u', scale = '%ld'",*(unsigned int *)targetDataPtr, scale));
			retcode                 = TRUE;
			goto func_exit;
			break;
		case _SQLDT_64BIT_S:
			*(long long *)targetDataPtr = getWrapperLong(wrapperInfo,paramNumber);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_64BIT_S: targetData = '%Ld', scale = '%ld'",*(long long *)targetDataPtr, scale));
			retcode                 = TRUE;
			goto func_exit;
			break;
			// Big Num Changes
		case _SQLDT_NUM_BIG_U:
		case _SQLDT_NUM_BIG_S:
			bigNumAsBytes = convertBigDecimalToSQLBigNum(wrapperInfo,paramNumber,scale, targetLength);
			bytesLength = wrapperInfo->jenv->GetArrayLength(bigNumAsBytes);
			if (bytesLength <= 0)
			{
				throwSQLException(wrapperInfo->jenv, PARAMETER_NOT_SET_ERROR, NULL, "07002", 0);
				FUNCTION_RETURN_NUMERIC(FALSE,("convertBigDecimalToSQLBigNum() failed"));
				goto func_exit;
			}
			pB = new BYTE[bytesLength];
			wrapperInfo->jenv->GetByteArrayRegion(bigNumAsBytes, 0, bytesLength, (jbyte *)pB);
			if(_SQLDT_NUM_BIG_S == FSDataType)
			{
				DEBUG_OUT(DEBUG_LEVEL_DATA,("_SQLDT_NUM_BIG_S: precision(%ld)>targetLength(%ld)",precision,targetLength));
			}
			else
			{
				DEBUG_OUT(DEBUG_LEVEL_DATA,("_SQLDT_NUM_BIG_U: precision(%ld)>targetLength(%ld)",precision,targetLength));
			}
			memcpy(targetDataPtr, pB, bytesLength);
			delete [] pB;
			pB = NULL;
			if(_SQLDT_NUM_BIG_U == FSDataType)
			{
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_NUM_BIG_U: targetData = '%Ld', scale = '%ld'",*(char *)targetDataPtr, scale));
			}
			else if(_SQLDT_NUM_BIG_S == FSDataType)
			{
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_NUM_BIG_S: targetData = '%Ld', scale = '%ld'",*(char *)targetDataPtr, scale));
			}
			retcode                 = TRUE;
			goto func_exit;
			break;
			// Big Num Changes
		default:
			break;
			}
			/* ****************************************************************************************************************
			* NOTE: SQL/MX stores the SQLTYPECODE_DECIMAL and SQLTYPECODE_DECIMAL_UNSIGNED as numeric ASCII strings. The ASCII
			*       string must only contain numbers and it must be padded with ASCII zero (0x30). A negative number is denoted
			*       by orring a hex 0x80 with the first charater in the string.
			*
			*       SQL/MX told us that we should never see SQLTYPECODE_DECIMAL_LARGE or SQLTYPECODE_DECIMAL_LARGE_UNSIGNED.
			*       These are software datatypes and are not externalized.
			*
			*       The only TYPE_FS descriptor field that we will see is the _SQLDT_DEC_U which is used to denote that the
			*       column is unsigned. All of the other TYPE_FS descriptor fields for decimal, _SQLDT_DEC_LSS, _SQLDT_DEC_LSE
			*       _SQLDT_DEC_TSS, and _SQLDT_DEC_TSE, are for host variable declaration only. We should never see any of these
			*       in the JDBC driver.
			*
			* ****************************************************************************************************************/
		case SQLTYPECODE_DECIMAL:
		case SQLTYPECODE_DECIMAL_UNSIGNED:
			nParamValue = JNI_GetStringUTFChars(wrapperInfo->jenv,(jstring)getWrapperObject(wrapperInfo,paramNumber), NULL);
			charptr = (char *) nParamValue;                       //Make a copy of the pointer we may need to update it
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_DECIMAL: nParamValue = '%s', precision = %ld",nParamValue, precision));

			if (charptr == NULL)
			{
				throwSQLException(wrapperInfo->jenv, PARAMETER_NOT_SET_ERROR, NULL, "07002", 0);
				FUNCTION_RETURN_NUMERIC(FALSE,("GetStringUTFChars() failed"));
				goto func_exit;
			}
			if (*charptr == '-')              //If the 1st char is a "-" then the number is negative
			{                                 //and we must remove the "-" before sending value to SQL/MX
				charptr++;                    //Point to the next charater in the string.
				*charptr |= 0x80;             //SQL/MX uses high order bit of the 1st char to indicated negative number
			}
			dataLen = strlen(charptr);

			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_DECIMAL: precision(%ld)>targetLength(%ld)",precision,targetLength));

			if (dataLen > targetLength)       //Make sure that we do not over flow the strorage area in the descriptor
			{
				throwSQLException(wrapperInfo->jenv, NUMERIC_VALUE_OUT_OF_RANGE_ERROR, NULL, "22003");
				goto func_exit;
			}
			memcpy(targetDataPtr, charptr, dataLen);   //Copy the data value to the SQL/MX descriptor
			retcode = TRUE;
			goto func_exit;
			break;

		case SQLTYPECODE_DATETIME:
		case SQLTYPECODE_INTERVAL:
			nParamValue = JNI_GetStringUTFChars(wrapperInfo->jenv,(jstring)getWrapperObject(wrapperInfo,paramNumber), NULL);
			if (nParamValue == NULL)
			{
				throwSQLException(wrapperInfo->jenv, PARAMETER_NOT_SET_ERROR, NULL, "07002", 0);
				FUNCTION_RETURN_NUMERIC(FALSE,("GetStringUTFChars() failed"));
			}
			dataLen = strlen(nParamValue);
			break;
		default:
			nParamValue = NULL;
			dataLen     = 0;
			break;
		}

		switch (sqlDataType)
		{
		case SQLTYPECODE_CHAR:
			// targetLength includes extra byte for null (ISO88591 only)
			if (dataLengthNotExceeded(charSet, dataLen, targetLength))
			{
				memcpy(targetDataPtr, nParamValue, dataLen);
				memset(targetDataPtr+dataLen, ' ', targetLength-dataLen);

				if (charSet == SQLCHARSETCODE_UCS2)
				{
					// Back fill target buffer with double byte 'space' characters (i.e. 0x00 0x20)
					for (i = dataLen; i < targetLength; i+=2)
					{
						*(targetDataPtr+i) = 0;
					}
				}
			}
			else
			{
				throwSQLException(wrapperInfo->jenv, STRING_DATA_TRUNCATED_ERROR, NULL, "22001");
				DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,
					("dataLen(%ld) exceeds targetLength(%ld)", dataLen, targetLength));
				goto func_exit;
			}
			MEMORY_DUMP(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,targetDataPtr,targetLength);
			break;
		case SQLTYPECODE_VARCHAR:
			if (targetLength > dataLen)
			{
				memcpy(targetDataPtr, nParamValue, dataLen);
				// null terminate ascii strings
				if (charSet == SQLCHARSETCODE_ISO88591)
				{
					*(targetDataPtr+dataLen) = 0;
				}
			}
			else
			{
				throwSQLException(wrapperInfo->jenv, STRING_DATA_TRUNCATED_ERROR, NULL, "22001");
				DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,
					("targetLength(%ld)<=dataLen(%ld)",
					targetLength, dataLen));
				goto func_exit;
			}
			MEMORY_DUMP(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,targetDataPtr,targetLength);
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		case SQLTYPECODE_VARCHAR_LONG:
		case SQLTYPECODE_CLOB:
		case SQLTYPECODE_BLOB:
		case SQLTYPECODE_DATETIME:
		case SQLTYPECODE_INTERVAL:
			if (vcIndLength == 4 && targetLength > dataLen+sizeof(int))
			{
                            *(unsigned int *)targetDataPtr = (unsigned int)dataLen;
                            memcpy(targetDataPtr+sizeof(int), nParamValue, dataLen);

                            switch(charSet)
                            {
                                case SQLCHARSETCODE_KANJI:
                                case SQLCHARSETCODE_KSC5601:
                                    // Back fill target buffer with "special" double byte
                                    // 'space' characters (i.e. 0x20 0x20)
                                    // Note: This is a backward compatibility issue with SQL/MP Tables
                                    memset(targetDataPtr+sizeof(int)+dataLen, ' ', targetLength-dataLen-sizeof(int));
                                    break;

                                case SQLCHARSETCODE_UCS2:
                                    // Back fill target buffer with double byte 'space' characters (i.e. 0x00 0x20)
                                    // by first setting entire buffer to spaces (0x20), then backfilling with 0x00
                                    memset(targetDataPtr+sizeof(int)+dataLen, ' ', targetLength-dataLen-sizeof(int));
                                    for (i = dataLen+sizeof(int); i < targetLength; i+=2)
                                    {
                                        *(targetDataPtr+i) = 0;
                                    }
                                    break;
                            }
                        }
			else if ( targetLength >= dataLen + sizeof(short))
			{
                            *(unsigned short *)targetDataPtr = (unsigned short) dataLen;
                            memcpy(targetDataPtr + sizeof(short), nParamValue, dataLen);

                            switch(charSet)
                            {
                                case SQLCHARSETCODE_KANJI:
                                case SQLCHARSETCODE_KSC5601:
                                    // Back fill target buffer with "special" double byte
                                    // 'space' characters (i.e. 0x20 0x20)
                                    // Note: This is a backward compatibility issue with SQL/MP Tables
                                    memset(targetDataPtr + sizeof(short) + dataLen, ' ', targetLength - dataLen - sizeof(short));
                                    break;

                                case SQLCHARSETCODE_UCS2:
                                    // Back fill target buffer with double byte 'space' characters (i.e. 0x00 0x20)
                                    // by first setting entire buffer to spaces (0x20), then backfilling with 0x00
                                    memset(targetDataPtr + sizeof(short) + dataLen, ' ', targetLength -dataLen - sizeof(short));
                                    for (i = dataLen + sizeof(short); i < targetLength; i += 2)
                                    {
                                        *(targetDataPtr + i) = 0;
                                    }
                                    break;
                            }
			}
			else
			{
				throwSQLException(wrapperInfo->jenv, STRING_DATA_TRUNCATED_ERROR, NULL, "22001");
				DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,
					("targetLength(%ld)<=dataLen(%ld)+sizeof(short)(%ld)",
					targetLength, dataLen, sizeof(short)));
				goto func_exit;
			}
			MEMORY_DUMP(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,targetDataPtr,targetLength);
			break;

		default:
			throwSQLException(wrapperInfo->jenv, PROGRAMMING_ERROR, gJNILayerErrorMsgs[RESTRICTED_DATATYPE_ERROR],
				"HY000");
			DEBUG_OUT(DEBUG_LEVEL_DATA,("Data Type not supported. RESTRICTED_DATATYPE_ERROR thrown."));
			goto func_exit;
		}
		retcode = TRUE;
func_exit:
		switch (sqlDataType)
		{
		case SQLTYPECODE_CHAR:
		case SQLTYPECODE_VARCHAR:
		case SQLTYPECODE_VARCHAR_LONG:
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		case SQLTYPECODE_CLOB:
		case SQLTYPECODE_BLOB:
			if (nParamValue != NULL)
			{
				JNI_ReleaseByteArrayElements(wrapperInfo->jenv,stringByteArray, (jbyte *)nParamValue, JNI_ABORT);
			}
			break;
		default:
			if (nParamValue != NULL)
			{
				JNI_ReleaseStringUTFChars(wrapperInfo->jenv,(jstring)getWrapperObject(wrapperInfo,paramNumber), nParamValue);
			}
			break;
		}
		FUNCTION_RETURN_NUMERIC(retcode,(NULL));
	}


	jobject getSQLCatalogsInfo(JNIEnv *jenv, jobject jobj, jstring server, jlong dialogueId,
		jint txid, jboolean autoCommit, jint txnMode,
		short catalogAPI,
		jstring catalogNm,
		jstring schemaNm, jstring tableNm, jstring tableTypeList,
		jstring columnNm, jint columnType, jint rowIdScope, long nullable,
		long uniqueness, jint accuracy, jshort sqlType, jint metadataId,
		jstring fkcatalogNm, jstring fkschemaNm, jstring fktableNm)
	{
		FUNCTION_ENTRY("getSQLCatalogsInfo",("server=%s, dialogueId=0x%08x, txid=0x%08x, autoCommit=%s, txnMode=%ld, catalogAPI=%d ...",
			DebugJString(jenv,server),
			dialogueId,
			txid,
			DebugBoolStr(autoCommit),
			txnMode,
			catalogAPI));


		ExceptionStruct					exception_;
		SQLItemDescList_def				outputDesc;
		ERROR_DESC_LIST_def				sqlWarning;
		SQLValueList_def				outputValueList;
		long							rowsAffected;
		char							catStmtLabel[50];
		jobject							SQLMXResultSet = NULL;
		jint							currentTxid = txid;
		jint							externalTxid;
		SRVR_STMT_HDL					*pSrvrStmt;
		long							sqlcode;
		short							txn_status;

		//intialize exception_ structure, so that if catalog api run successfully, there is no exception
		memset((void*)&exception_,0,sizeof(ExceptionStruct));

		const char	*nCatalogNm;
		const char	*nSchemaNm;
		const char	*nTableNm;
		const char	*nTableTypeList;
		const char	*nColumnNm;
		const char	*nfkCatalogNm;
		const char	*nfkSchemaNm;
		const char	*nfkTableNm;
		long		stmtId;

		if (catalogNm)
			nCatalogNm = JNI_GetStringUTFChars(jenv,catalogNm, NULL);
		else
			nCatalogNm = "";

		if (schemaNm)
			nSchemaNm = JNI_GetStringUTFChars(jenv,schemaNm, NULL);
		else
			nSchemaNm = "";

		if (tableNm)
			nTableNm = JNI_GetStringUTFChars(jenv,tableNm, NULL);
		else
			nTableNm = "";

		if (columnNm)
			nColumnNm = JNI_GetStringUTFChars(jenv,columnNm, NULL);
		else
			nColumnNm = "";

		if (tableTypeList)
			nTableTypeList = JNI_GetStringUTFChars(jenv,tableTypeList, NULL);
		else
			nTableTypeList = "";

		if (fkcatalogNm)
			nfkCatalogNm = JNI_GetStringUTFChars(jenv,fkcatalogNm, NULL);
		else
			nfkCatalogNm = "";

		if (fkschemaNm)
			nfkSchemaNm = JNI_GetStringUTFChars(jenv,fkschemaNm, NULL);
		else
			nfkSchemaNm = "";

		if (fktableNm)
			nfkTableNm = JNI_GetStringUTFChars(jenv,fktableNm, NULL);
		else
			nfkTableNm = "";

		if ((txn_status = beginTxnControl(jenv, currentTxid, externalTxid, txnMode, -1)) != 0)
		{
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidDBMMethodId, currentTxid);
			throwTransactionException(jenv, txn_status);
			FUNCTION_RETURN_PTR(NULL,("beginTxnControl() failed"));
		}

		// Null out outputValueList before we pass it down
		CLEAR_LIST(outputValueList);

		odbc_SQLSvc_GetSQLCatalogs_sme_(NULL, NULL,
			&exception_,
			dialogueId,
			catalogAPI,
			nCatalogNm,
			nSchemaNm,
			nTableNm,
			nTableTypeList,
			nColumnNm,
			columnType,
			rowIdScope,
			nullable,
			uniqueness,
			accuracy,
			sqlType,
			metadataId,
			catStmtLabel,
			&outputDesc,
			&sqlWarning,
			&rowsAffected,
			&outputValueList,
			&stmtId,
			nfkCatalogNm,
			nfkSchemaNm,
			nfkTableNm);

		if (catalogNm)
			JNI_ReleaseStringUTFChars(jenv,catalogNm, nCatalogNm);

		if (schemaNm)
			JNI_ReleaseStringUTFChars(jenv,schemaNm, nSchemaNm);

		if (tableNm)
			JNI_ReleaseStringUTFChars(jenv,tableNm, nTableNm);

		if (columnNm)
			JNI_ReleaseStringUTFChars(jenv,columnNm, nColumnNm);

		if (tableTypeList)
			JNI_ReleaseStringUTFChars(jenv,tableTypeList, nTableTypeList);

		if (fkcatalogNm)
			JNI_ReleaseStringUTFChars(jenv,fkcatalogNm, nfkCatalogNm);

		if (fkschemaNm)
			JNI_ReleaseStringUTFChars(jenv,fkschemaNm, nfkSchemaNm);

		if (fktableNm)
			JNI_ReleaseStringUTFChars(jenv,fktableNm, nfkTableNm);
		if ((txn_status = endTxnControl(jenv, currentTxid, txid, autoCommit,
			exception_.exception_nr, TRUE, txnMode, externalTxid)) != 0)
		{
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidDBMMethodId, currentTxid);
			throwTransactionException(jenv, txn_status);
			FUNCTION_RETURN_PTR(NULL,("endTxnControl() Failed"));
		}

		switch (exception_.exception_nr)
		{
		case CEE_SUCCESS:

			if ((pSrvrStmt = getSrvrStmt(dialogueId, stmtId, &sqlcode)) == NULL)
			{
				throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", sqlcode);
				FUNCTION_RETURN_PTR(NULL,("getSrvrStmt() did not return a server statement"));
			}
			SQLMXResultSet = setGetSQLCatalogOutputs(jenv, jobj, pSrvrStmt, &outputDesc, catStmtLabel, currentTxid,
				rowsAffected, TRUE, &outputValueList, stmtId);

			if (sqlWarning._length > 0) setSQLWarning(jenv, jobj, &sqlWarning);
			break;
		case odbc_SQLSvc_GetSQLCatalogs_ParamError_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidDBMMethodId, currentTxid);
			throwSQLException(jenv, MODULE_ERROR, exception_.u.ParamError.ParamDesc, "HY000");
			break;
		case odbc_SQLSvc_GetSQLCatalogs_SQLError_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidDBMMethodId, currentTxid);
			throwSQLException(jenv, &exception_.u.SQLError);
			break;
		case odbc_SQLSvc_GetSQLCatalogs_SQLInvalidHandle_exn_:
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidDBMMethodId, currentTxid);
			throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_.u.SQLInvalidHandle.sqlcode);
			break;
		case odbc_SQLSvc_GetSQLCatalogs_InvalidConnection_exn_:
		default:
			// TFDS - These exceptions should not happen
			jenv->CallVoidMethod(jobj, gJNICache.setCurrentTxidDBMMethodId, currentTxid);
			throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_.exception_nr);
			break;
		}
		FUNCTION_RETURN_PTR(SQLMXResultSet,(NULL));
	}

	void throwSetConnectionException(JNIEnv *jenv, ExceptionStruct *exception_)
	{
		switch (exception_->exception_nr)
		{
		case odbc_SQLSvc_SetConnectionOption_ParamError_exn_:
			throwSQLException(jenv, PROGRAMMING_ERROR, exception_->u.ParamError.ParamDesc, "HY000");
			break;
		case odbc_SQLSvc_SetConnectionOption_SQLError_exn_:
			throwSQLException(jenv, &exception_->u.SQLError);
			break;
		case odbc_SQLSvc_SetConnectionOption_SQLInvalidHandle_exn_:
			throwSQLException(jenv, INVALID_HANDLE_ERROR, NULL, "HY000", exception_->u.SQLInvalidHandle.sqlcode);
			break;
		case odbc_SQLSvc_SetConnectionOption_InvalidConnection_exn_:
		default:
			// TFDS - These exceptions should not happen
			throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception_->exception_nr);
			break;
		}
	}

	short suspendExtTxn(jint &externalTxid)
	{
/*
		FUNCTION_ENTRY("suspendExtTxn",
			("externalTxid=0x%08x",
			externalTxid));
			
		short	exttxnhandle[10];
		short	status = 0;

#ifndef NSK_PLATFORM	// Linux port - ToDo txn related
		return 0;
#else		
		// Get txn handle within current thread
		status = TMF_GETTXHANDLE_((short *)&exttxnhandle);
		DEBUG_OUT(DEBUG_LEVEL_TXN,("TMF_GETTXHANDLE_() returned %d", status));
		if (status != 0)
			FUNCTION_RETURN_NUMERIC(status,("TMF_GETTXHANDLE_ returned error"));

		// Get "txn-begin-tag" from txn handle
		#ifdef _LP64
		status = TMF_BEGINTAG_FROM_TXHANDLE_(exttxnhandle, ( __int32_t _ptr32 *) &externalTxid); //venu type casted this pointer
		#else
			status = TMF_BEGINTAG_FROM_TXHANDLE_(exttxnhandle, &externalTxid);
		#endif
		DEBUG_OUT(DEBUG_LEVEL_TXN,("TMF_BEGINTAG_FROM_TXHANDLE_() returned %d; externalTxid = 0x%08x", status, (long *)externalTxid));
		if (status != 0)
			FUNCTION_RETURN_NUMERIC(status,("TMF_BEGINTAG_FROM_TXHANDLE_ returned error"));
#endif		
		status = resumeTransaction(0);
		DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(0x%08x) returned %d", externalTxid, status));

		FUNCTION_RETURN_NUMERIC(status,(NULL));
*/
               return 0;
	}

	short resumeOrBeginTxn(JNIEnv *jenv, jint &currentTxid, jint &externalTxid, jint txnMode)
	{
/*
		FUNCTION_ENTRY("resumeOrBeginTxn",("jenv=0x%08x, currentTxid=0x%08x, externalTxid=0x%08x, txnMode=%ld,",
			jenv,
			currentTxid,
			externalTxid,
			txnMode));

		short status;
		
#ifndef NSK_PLATFORM	// Linux port - ToDo txn related
		return 0;
#else				
		short txid[4];
		long *txid_ptr;;
		txid_ptr = (long *)&txid[0];

		externalTxid = 0;

		status = GETTRANSID((short *)&txid);
		txid[3]=0;

		DEBUG_OUT(DEBUG_LEVEL_TXN,("GETTRANSID() returned %d ; trans-begin-tag = 0x%08x", status, *txid_ptr));

		switch (status)
		{
			// Zero indicates that there is a TMF transaction on the current thread
		case 0:
			// Display internal and external form of the transaction tag for debug purposes
			DEBUG_OUT(DEBUG_LEVEL_TXN, ("currentTxid = 0x%08x", currentTxid));
			DEBUG_TRANSTAG();

			if (currentTxid != 0)
			{
				status = INVALID_TRANSACTION_STATE;
				FUNCTION_RETURN_NUMERIC(status,("currentTxid != 0"));
			}
			// suspend external txn when in "internal" txn mode
			if (txnMode == org_apache_trafodion_jdbc_t2_SQLMXConnection_TXN_MODE_INTERNAL)
			{
				if ((status = suspendExtTxn(externalTxid)) != 0)
					FUNCTION_RETURN_NUMERIC(status,
					("Failed to suspend external txn -- ext txnId = 0x%08x", externalTxid));
				// continue processing to the next case to either begin a new txn or resume the current txn
			}
			else
			{
				// Set external txid to a non-zero value to indicate an external txn is in progress
				// and break from switch
				externalTxid = -1;
				break;
			}

		case 75: // Nil State Transaction
        case 78: //Added for R3.0,incase of a invalid transaction also begin a transaction -senthil
			if (currentTxid == 0)
			{
				status = beginTransaction((long *)&currentTxid);
				DEBUG_OUT(DEBUG_LEVEL_TXN,("beginTransaction(0x%08x) returned %d",
					currentTxid,
					status));
			}
			else
			{
				status = resumeTransaction(currentTxid);
				DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(0x%08x) returned %d",
					currentTxid,
					status));
				switch (status)
				{
				case 0:
					break;
					// Note: Per TMF Application Programmer's Guide - Do not need call abortTransaction() or endTransaction()
					//       for the following three error codes.
				case 31: // Unable to obtain file-system buffer space
				case 36: // Unable to lock physical memory; not enough memory available
				case 78: // Invalid txn id
					currentTxid = 0;
					break;
					/*For 10-091005-5100, Just pass back the error 97*/
/*
				case 97: // CONTROL QUERY DEFAULT DOOM_USERTRANSACTION 'ON'
					break;
				default:
					short rc = abortTransaction();
					DEBUG_OUT(DEBUG_LEVEL_TXN,("abortTransaction() returned %d (Not handled)",
						rc));
					currentTxid = 0;
				}
			}
			break;
		default:
			DEBUG_OUT(DEBUG_LEVEL_TXN,("GETTRANSID() returned %d (default case)", status));
		}
#endif		
		FUNCTION_RETURN_NUMERIC(status,(NULL));
*/
               return 0;
	}

	short resumeTxn(JNIEnv *jenv, jint &currentTxid, jint &externalTxid, jint txnMode)
	{
/*
		FUNCTION_ENTRY("resumeTxn",("jenv=0x%08x, currentTxid=0x%08x, externalTxid=0x%08x",
			jenv,
			currentTxid,
			externalTxid));

		short status;
		
#ifndef NSK_PLATFORM	// Linux port - ToDo txn related
		return 0;
#else				
		short txid[4];
		long *txid_ptr;;
		txid_ptr = (long *)&txid[0];

		externalTxid = 0;

		status = GETTRANSID((short *)&txid);
		txid[3]=0;

		DEBUG_OUT(DEBUG_LEVEL_TXN,("GETTRANSID() returned %d ; trans-begin-tag = 0x%08x", status, *txid_ptr));

		switch (status)
		{
		case 0:
			// Display internal and external form of the transaction tag for debug purposes
			DEBUG_OUT(DEBUG_LEVEL_TXN, ("currentTxid = 0x%08x", currentTxid));
			DEBUG_TRANSTAG();

			// If there is any txid in the Connection Context, throw error (mixed mode only)
			if (currentTxid != 0)
			{
				status = INVALID_TRANSACTION_STATE;
				FUNCTION_RETURN_NUMERIC(status,("Invalid transaction state -- currentTxid != 0 "));
			}
			// suspend external txn when in "internal" txn mode
			if (txnMode == org_apache_trafodion_jdbc_t2_SQLMXConnection_TXN_MODE_INTERNAL)
			{
				if ((status = suspendExtTxn(externalTxid)) != 0)
				{
					DEBUG_OUT(DEBUG_LEVEL_TXN,("suspendExtTxn returned ext txn = 0x%08x", externalTxid));
					FUNCTION_RETURN_NUMERIC(status,("Failed to suspend external txn or get external txid."));
				}
			}
			else
			{
				// Set external txid to a non-zero value to indicate an external txn is in progress
				// and break from switch
				externalTxid = -1;
				break;
			}
		case 75: // Nil State Transaction
			case 78: //Added for R3.0,incase of a invalid transaction also begin a transaction -senthil
			status = resumeTransaction(currentTxid);
			DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(0x%08x) returned %d",
				currentTxid, status));
			switch (status)
			{
			case 0:
				break;
				// Note: Per TMF Application Programmer's Guide - Do not need call abortTransaction() or endTransaction()
				//       for the following three error codes.
			case 31:  // Unable to obtain file-system buffer space
			case 36:  // Unable to lock physical memory; not enough memory available
			case 78:  // Invalid txn id
				currentTxid = 0;
				break;
			default:
				short rc = abortTransaction();
				DEBUG_OUT(DEBUG_LEVEL_TXN,("abortTransaction() returned %d (Not handled)",
					rc));
				currentTxid = 0;
			}
			break;
		default:
			DEBUG_OUT(DEBUG_LEVEL_TXN,("GETTRANSID() returned %d (default case)", status));
		}
#endif		
		FUNCTION_RETURN_NUMERIC(status,(NULL));
*/
		return 0;
	}

	short suspendOrEndTxn(JNIEnv *jenv, jint &currentTxid,
		jboolean autoCommit, unsigned long exception_nr,
		jboolean isSelect)
	{
/*
		FUNCTION_ENTRY("suspendOrEndTxn",("jenv=0x%08x, currentTxid=0x%08x, autoCommit=%s, exception_nr=%ld, isSelect=%s",
			jenv,
			currentTxid,
			DebugBoolStr(autoCommit),
			exception_nr,
			DebugBoolStr(isSelect)));

		short status = 0;

		// Abort the transaction independent of autoCommit mode
		// to ensure the database is consistent
		// - Ask SQL folks to verify
		if ((exception_nr != CEE_SUCCESS) && (! isSelect))
		{
			short rc = abortTransaction();
			// Ignore any error from ABORT, since SQL might have also aborted
			DEBUG_OUT(DEBUG_LEVEL_TXN,("abortTransaction() returned %d (Ignored)",
				rc));
			currentTxid = 0;
			FUNCTION_RETURN_NUMERIC(status,(NULL));
		}
		if (autoCommit)
		{
			if (! isSelect)
			{
				status = endTransaction();
				DEBUG_OUT(DEBUG_LEVEL_TXN,("endTransaction() returned %d",
					status));
				currentTxid = 0;
			}
			else
			{
				status = resumeTransaction(0);
				DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(0) returned %d",
					status));
			}
		}
		else
		{
			status = resumeTransaction(0);
			DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(0) returned %d",
				status));
		}
		FUNCTION_RETURN_NUMERIC(status,(NULL));
*/
                return 0;
	}



	short suspendOrEndTxn(JNIEnv *jenv, jint &currentTxid,
		jint previousTxid, jboolean autoCommit,
		unsigned long exception_nr, jboolean isSelect)
	{
/*
		FUNCTION_ENTRY("suspendOrEndTxn",
			("jenv=0x%08x, currentTxid=0x%08x, previousTxid=0x%08x, autoCommit=%s, exception_nr=%ld, isSelect=%s",
			jenv,
			currentTxid,
			previousTxid,
			DebugBoolStr(autoCommit),
			exception_nr,
			DebugBoolStr(isSelect)));

		short status = 0;

		/* 
		* Description: Type 2 driver now supports atomicity at statement level
		*/
/*
		if (previousTxid) // Transaction was already started by the previous SQL execution
		{
			DEBUG_OUT(DEBUG_LEVEL_TXN,("previous txn (0x%08x) in progres",previousTxid));
			if (!autoCommit )
			{
				if((STMT_ATOMICITY == true) || (exception_nr == CEE_SUCCESS) || (isSelect))
				{
					//Always suspend the txn if STMT_ATOMICITY is true
					status = resumeTransaction(0);
					DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(0) returned %d", status));
				}
				else
				{
					/*10-091005-5100: Whenever the user has started the txn
					* dont abort it just resume it as above.
					*/
/*
					status = resumeTransaction(0);
					DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(0) returned %d", status));
					/*Commented for 10-091005-5100*/
					//short rc = abortTransaction();
					// Ignore any error from ABORT, since SQL might have also aborted
					//DEBUG_OUT(DEBUG_LEVEL_TXN,("abortTransaction() returned %d (Ignored)", rc));
					//currentTxid = 0;
					/*Commented for 10-091005-5100*/
/*
				}
			}
			// Resume txn if cmd passed OR it failed and it is a select
			else if ( (exception_nr == CEE_SUCCESS) || ((exception_nr != CEE_SUCCESS) && (isSelect)) )
			{
				status = resumeTransaction(0);
				DEBUG_OUT(DEBUG_LEVEL_TXN,("resumeTransaction(0) returned %d", status));
			}
			// abort the txn if cmd failed and it is not a select
			else // the only other possible option is ==> ((exception_nr != CEE_SUCCESS) && (!isSelect))
			{
				// Abort the transaction (independent of autoCommit mode)
				// just to make sure the database is consistent
				//   Note: Ask SQL folks to verify
				short rc = abortTransaction();
				// Ignore any error from ABORT, since SQL might have also aborted
				DEBUG_OUT(DEBUG_LEVEL_TXN,("abortTransaction() returned %d (Ignored)", rc));
				currentTxid = 0;
			}
		}
		else
		{
			status = suspendOrEndTxn(jenv, currentTxid, autoCommit, exception_nr, isSelect);
			DEBUG_OUT(DEBUG_LEVEL_TXN,("suspendOrEndTxn returned %d : currentTxid = 0x%08x, autoCommit = %s ",
				status, currentTxid, DebugBoolStr(autoCommit)));
		}
		FUNCTION_RETURN_NUMERIC(status,(NULL));
*/
		return 0;
	}

	short beginTxnControl(JNIEnv *jenv, jint &currentTxid, jint &externalTxid, jint txnMode, jint holdability)
	{
/*
		FUNCTION_ENTRY("beginTxnControl",("jenv=0x%08x, currentTxid=0x%08x, externalTxid=0x%08x, txnMode=%ld, holdability=%ld",
			jenv,
			currentTxid,
			externalTxid,
			txnMode,
			holdability));

		short status = 0;

		switch (txnMode)
		{
		case org_apache_trafodion_jdbc_t2_SQLMXConnection_TXN_MODE_INTERNAL:
		case org_apache_trafodion_jdbc_t2_SQLMXConnection_TXN_MODE_MIXED:
			if (holdability == CLOSE_CURSORS_AT_COMMIT)
			{
				status = resumeTxn(jenv, currentTxid, externalTxid, txnMode);
				DEBUG_OUT(DEBUG_LEVEL_TXN,(
					"beginTxnControl -- resumeTxn returned %d currentTxid = 0x%08x",
					status, currentTxid));
			}
			else
			{
				status = resumeOrBeginTxn(jenv, currentTxid, externalTxid, txnMode);
				DEBUG_OUT(DEBUG_LEVEL_TXN,(
					"beginTxnControl -- resumeOrBeginTxn returned %d currentTxid = 0x%08x",
					status, currentTxid));
			}
			break;
		case org_apache_trafodion_jdbc_t2_SQLMXConnection_TXN_MODE_EXTERNAL:
			break;
		default:
			// This is a programming error if it ever gets to hers
			status = -2;
			DEBUG_OUT(DEBUG_LEVEL_TXN,("Invalid txn mode"));
		}
		FUNCTION_RETURN_NUMERIC(status,(NULL));
*/
		return 0;
	}

	void throwTransactionException(JNIEnv *jenv, jint err_code)
	{
		FUNCTION_ENTRY("throwTransactionException",("jenv=0x%08x, err_code=%ld",
			jenv, err_code));

		char msg[100];

		if (err_code > 0)
		{
			sprintf(msg,"error %d",err_code);
			throwSQLException(jenv, TMF_ERROR, msg, "HY000", 0);
		}
		else
		{
			switch (err_code)
			{
			case INVALID_TRANSACTION_STATE:
				throwSQLException(jenv, INCONSISTENT_TRANSACTION_ERROR, NULL, "25000", 0);
				break;
				// This exception should never be thrown
			default:
				throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", 0);
				break;
			}
		}
		FUNCTION_RETURN_VOID((NULL));
	}

	short endTxnControl(JNIEnv *jenv, jint &currentTxid, jint txid,
		jboolean autoCommit, unsigned long exception_nr, jboolean isSelect,
		jint txnMode, jint &externalTxid)
	{
/*
		FUNCTION_ENTRY("endTxnControl",("jenv=0x%08x, currentTxid=0x%08x, txid=0x%08x, autoCommit=%s, exception_nr=%ld, isSelect=%s, txnMode=%ld, externalTxid=0x%08x",
			jenv,
			currentTxid,
			txid,
			DebugBoolStr(autoCommit),
			exception_nr,
			DebugBoolStr(isSelect),
			txnMode,
			externalTxid));

		short status = 0;
		switch (txnMode)
		{
		case org_apache_trafodion_jdbc_t2_SQLMXConnection_TXN_MODE_INTERNAL:
			status = suspendOrEndTxn(jenv, currentTxid, txid, autoCommit, exception_nr, isSelect);
			DEBUG_OUT(DEBUG_LEVEL_TXN,("suspendOrEndTxn returned %d for currentTxid 0x%08x , txid = 0x%08x",
				status, currentTxid, txid));
			if (status != 0)
				FUNCTION_RETURN_NUMERIC(status,("suspendOrEndTxn() failed"));

			// resume external txn if one existed
			if (externalTxid != 0)
			{
				status = resumeTransaction(externalTxid);
				DEBUG_OUT(DEBUG_LEVEL_TXN,("Resuming ext txn -- resumeTransaction(0x%08x) returned %d",
					externalTxid, status));
				switch (status)
				{
				case 0:
					break;
					// Note: Per TMF Application Programmer's Guide - Do not need call abortTransaction() or endTransaction()
					//       for the following three error codes.
				case 31:  // Unable to obtain file-system buffer space
				case 36:  // Unable to lock physical memory; not enough memory available
				case 78:  // Invalid txn id
					externalTxid = 0;
					break;
				default:
					short rc = abortTransaction();
					DEBUG_OUT(DEBUG_LEVEL_TXN,("abortTransaction() returned %d (Not handled)",
						rc));
					externalTxid = 0;
				}
			}
			break;
		case org_apache_trafodion_jdbc_t2_SQLMXConnection_TXN_MODE_MIXED:
			if (externalTxid == 0)
			{
				DEBUG_OUT(DEBUG_LEVEL_TXN,("Not an external txn: currentTxid = 0x%08x, txid = 0x%08x", currentTxid, txid));
				status = suspendOrEndTxn(jenv, currentTxid, txid, autoCommit, exception_nr, isSelect);
				DEBUG_OUT(DEBUG_LEVEL_TXN,("suspendOrEndTxn returned %d for currentTxid (0x%08x)", status, currentTxid));
			}
			break;
		case org_apache_trafodion_jdbc_t2_SQLMXConnection_TXN_MODE_EXTERNAL:
			break;
		default:
			// This is a programming error if it ever gets to hers
			status = -2;
			DEBUG_OUT(DEBUG_LEVEL_TXN,("Invalid txn mode"));
		}
		FUNCTION_RETURN_NUMERIC(status,(NULL));
*/
		return 0;
	}


	BOOL cacheJNIObjects(JNIEnv *jenv)
	{
		FUNCTION_ENTRY("cacheJNIObjects",("jenv=0x%08x",
			jenv));

		if (jenv == gJEnv)
			FUNCTION_RETURN_NUMERIC(TRUE,("%s",DebugBoolStr(TRUE)));

		//SQLException
		jclass sqlExceptionClass = JNI_FindClass(jenv,"java/sql/SQLException");
		if (sqlExceptionClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - sqlExceptionClass == NULL"));
		gJNICache.sqlExceptionClass = (jclass)JNI_NewGlobalRef(jenv,sqlExceptionClass);

		gJNICache.sqlExceptionConstructorId = JNI_GetMethodID(jenv,sqlExceptionClass, "<init>",
			"(Ljava/lang/String;Ljava/lang/String;I)V");
		if (gJNICache.sqlExceptionConstructorId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.sqlExceptionConstructorId == NULL"));
		gJNICache.setNextExceptionMethodId=JNI_GetMethodID(jenv,sqlExceptionClass,"setNextException",
			"(Ljava/sql/SQLException;)V");
		if (gJNICache.setNextExceptionMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.setNextExceptionMethodId == NULL"));

		//SQLWarning
		jclass sqlWarningClass = JNI_FindClass(jenv,"java/sql/SQLWarning");
		if (sqlWarningClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - sqlWarningClass == NULL"));
		gJNICache.sqlWarningClass = (jclass)JNI_NewGlobalRef(jenv,sqlWarningClass);
		gJNICache.sqlWarningConstructorId = JNI_GetMethodID(jenv,sqlWarningClass, "<init>",
			"(Ljava/lang/String;Ljava/lang/String;I)V");
		if (gJNICache.sqlWarningConstructorId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.sqlWarningConstructorId"));
		gJNICache.setNextWarningMethodId=JNI_GetMethodID(jenv,sqlWarningClass,"setNextWarning",
			"(Ljava/sql/SQLWarning;)V");
		if (gJNICache.setNextWarningMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.setNextWarningMethodId == NULL"));

		//SQLMXHandle
		jclass  handleClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/SQLMXHandle");
		gJNICache.sqlWarningMethodId = JNI_GetMethodID(jenv,handleClass, "setSqlWarning",
			"(Ljava/sql/SQLWarning;)V");
		if (gJNICache.sqlWarningMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.sqlWarningMethodId == NULL"));

		//SQLMXDesc
		jclass SQLMXDescClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/SQLMXDesc");
		if (SQLMXDescClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - SQLMXDescClass == NULL"));
		gJNICache.SQLMXDescClass = (jclass)JNI_NewGlobalRef(jenv,SQLMXDescClass);
		gJNICache.SQLMXDescConstructorId = JNI_GetMethodID(jenv,SQLMXDescClass, "<init>",
			"(ISISSZLjava/lang/String;ZISIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;IIILjava/lang/String;)V");
		if (gJNICache.SQLMXDescConstructorId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.SQLMXDescConstructorId == NULL"));

		// SQLMXPreparedStatement
		jclass  SQLMXPreparedStatementClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/SQLMXPreparedStatement");
		if (SQLMXPreparedStatementClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - SQLMXPreparedStatementClass == NULL"));
		gJNICache.prepareOutputsMethodId = JNI_GetMethodID(jenv,SQLMXPreparedStatementClass, "setPrepareOutputs",
			"([Lorg/apache/trafodion/jdbc/t2/SQLMXDesc;[Lorg/apache/trafodion/jdbc/t2/SQLMXDesc;IIIJ)V");
		if (gJNICache.prepareOutputsMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.prepareOutputsMethodId == NULL"));
		gJNICache.executeOutputsMethodId = JNI_GetMethodID(jenv,SQLMXPreparedStatementClass, "setExecuteOutputs",
			"([II[Lorg/apache/trafodion/jdbc/t2/DataWrapper;II)V");
		if (gJNICache.executeOutputsMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.executeOutputsMethodId == NULL"));

		// SQLMXStatement
		jclass  SQLMXStatementClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/SQLMXStatement");
		if (SQLMXStatementClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - SQLMXStatementClass == NULL"));
		gJNICache.execDirectOutputsMethodId = JNI_GetMethodID(jenv,SQLMXStatementClass, "setExecDirectOutputs",
			"([Lorg/apache/trafodion/jdbc/t2/SQLMXDesc;I[Lorg/apache/trafodion/jdbc/t2/DataWrapper;IIJI)V");
		if (gJNICache.execDirectOutputsMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.execDirectOutputsMethodId == NULL"));

		gJNICache.execRSOutputsMethodId = JNI_GetMethodID(jenv,SQLMXStatementClass, "setExecRSOutputs",
			"([Lorg/apache/trafodion/jdbc/t2/SQLMXDesc;IJI)V");
		if (gJNICache.execRSOutputsMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.execRSOutputsMethodId == NULL"));

		gJNICache.execDirectBatchOutputsMethodId = JNI_GetMethodID(jenv,SQLMXStatementClass, "setExecDirectBatchOutputs",
			"(III)V");
		if (gJNICache.execDirectBatchOutputsMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.execDirectBatchOutputsMethodId == NULL"));
		gJNICache.setCurrentTxidStmtMethodId = JNI_GetMethodID(jenv,SQLMXStatementClass, "setCurrentTxid",
			"(I)V");
		if (gJNICache.setCurrentTxidStmtMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.setCurrentTxidStmtMethodId == NULL"));
		gJNICache.setCurrentStmtIdMethodId = JNI_GetMethodID(jenv,SQLMXStatementClass, "setCurrentStmtId",
			"(J)V");
		if (gJNICache.setCurrentStmtIdMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.setCurrentStmtIdMethodId == NULL"));

		// SQLMXResultSet
		jclass  SQLMXResultSetClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/SQLMXResultSet");
		if (SQLMXResultSetClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - SQLMXResultSetClass == NULL"));
		gJNICache.SQLMXResultSetClass = (jclass) JNI_NewGlobalRef(jenv,SQLMXResultSetClass);
		gJNICache.fetchOutputsMethodId = JNI_GetMethodID(jenv,SQLMXResultSetClass, "setFetchOutputs",
			"([Lorg/apache/trafodion/jdbc/t2/DataWrapper;IZI)V");
		if (gJNICache.fetchOutputsMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.fetchOutputsMethodId == NULL"));
		gJNICache.SQLMXResultSetConstructorId = JNI_GetMethodID(jenv,SQLMXResultSetClass, "<init>",
			"(Lorg/apache/trafodion/jdbc/t2/SQLMXDatabaseMetaData;[Lorg/apache/trafodion/jdbc/t2/SQLMXDesc;IJ)V");
		if (gJNICache.SQLMXResultSetConstructorId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.SQLMXResultSetConstructorId == NULL"));
		gJNICache.setCurrentTxidRSMethodId = JNI_GetMethodID(jenv,SQLMXResultSetClass, "setCurrentTxid",
			"(I)V");
		if (gJNICache.setCurrentTxidRSMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.setCurrentTxidRSMethodId == NULL"));

		//SQLMXDatabaseMetaData
		jclass  SQLMXDatabaseMetaDataClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/SQLMXDatabaseMetaData");
		if (SQLMXDatabaseMetaDataClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - SQLMXDatabaseMetaDataClass == NULL"));
		gJNICache.setCurrentTxidDBMMethodId = JNI_GetMethodID(jenv,SQLMXDatabaseMetaDataClass, "setCurrentTxid",
			"(I)V");
		if (gJNICache.setCurrentTxidDBMMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.setCurrentTxidDBMMethodId == NULL"));

		//SQLMXCallableStatement
		jclass  SQLMXCallableStatementClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/SQLMXCallableStatement");
		if (SQLMXCallableStatementClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - SQLMXCallableStatementClass == NULL"));
		gJNICache.executeCallOutputsMethodId = JNI_GetMethodID(jenv,SQLMXCallableStatementClass, "setExecuteCallOutputs",
			"(Lorg/apache/trafodion/jdbc/t2/DataWrapper;SIIIZ)V");


		if (gJNICache.executeCallOutputsMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.executeCallOutputsMethodId == NULL"));

		gJNICache.SQLMXConnectionClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/SQLMXConnection");
		if (gJNICache.SQLMXConnectionClass == NULL)
		{
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.SQLMXConnectionClass == NULL"));
		}
		gJNICache.getSqlStmtTypeMethodId = jenv->GetStaticMethodID(gJNICache.SQLMXConnectionClass, "getSqlStmtType2", "(Ljava/lang/String;)Z");
		if (gJNICache.getSqlStmtTypeMethodId == NULL)
		{
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.getSqlStmtTypeMethodId == NULL"));
		}

		//String
		jclass stringClass = JNI_FindClass(jenv,"java/lang/String");
		if (stringClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - stringClass == NULL"));
		gJNICache.stringClass = (jclass)JNI_NewGlobalRef(jenv,stringClass);
		gJNICache.getBytesMethodId = JNI_GetMethodID(jenv,stringClass, "getBytes",
			"()[B");
		if (gJNICache.getBytesMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.getBytesMethodId == NULL"));
		gJNICache.stringConstructorId = JNI_GetMethodID(jenv,stringClass, "<init>",
			"([B)V");
		if (gJNICache.stringConstructorId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.stringConstructorId == NULL"));
		gJNICache.getBytesEncodedMethodId = JNI_GetMethodID(jenv,stringClass, "getBytes",
			"(Ljava/lang/String;)[B");
		if (gJNICache.getBytesEncodedMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.getBytesEncodedMethodId == NULL"));

		// Create reference to the DataWrapper class
		jclass wrapperClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/DataWrapper");
		if (wrapperClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - wrapperClass == NULL"));
		gJNICache.wrapperClass = (jclass)JNI_NewGlobalRef(jenv,wrapperClass);
		// Big Num Changes
		gJNICache.wrapperconvertBigDecimalToSQLBigNum = JNI_GetMethodID(jenv, wrapperClass,
			"convertBigDecimalToSQLBigNum" , "(III)[B");
		gJNICache.wrapperconvertSQLBigNumToBigDecimal2 = JNI_GetMethodID(jenv, wrapperClass,
			"convertSQLBigNumToBigDecimal2", "([BI)Ljava/lang/String;");
		// Big Num Changes

		jclass byteArrayClass = JNI_FindClass(jenv,"[B");
		if (byteArrayClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - byteArrayClass == NULL"));
		gJNICache.byteArrayClass = (jclass)JNI_NewGlobalRef(jenv,byteArrayClass);

        jclass SQLbyteArrayClass = JNI_FindClass(jenv,"[B");
        if (SQLbyteArrayClass == NULL)
            FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - SQLbyteArrayClass == NULL"));
        gJNICache.SQLbyteArrayClass = (jclass)JNI_NewGlobalRef(jenv,SQLbyteArrayClass);
//------------
		// SPJRS
		// Create reference to the ResultSetInfo class
		jclass ResultSetInfoClass = JNI_FindClass(jenv,"org/apache/trafodion/jdbc/t2/ResultSetInfo");
		if (ResultSetInfoClass == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - ResultSetInfoClass == NULL"));
		gJNICache.ResultSetInfoClass = (jclass)JNI_NewGlobalRef(jenv,ResultSetInfoClass);

		// ** ctxHandle - JNI method used to fetch the int field
		gJNICache.SPJRS_ctxHandleFieldID  = JNI_GetFieldID(jenv, ResultSetInfoClass, "ctxHandle", "I");
		if (gJNICache.SPJRS_ctxHandleFieldID == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.SPJRS_ctxHandleFieldID == NULL"));

		// ** stmtID - JNI method used to fetch the int field
		gJNICache.SPJRS_stmtIDFieldID  = JNI_GetFieldID(jenv, ResultSetInfoClass, "stmtID", "I");
		if (gJNICache.SPJRS_stmtIDFieldID == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.SPJRS_stmtIDFieldID == NULL"));

		//10-060831-8723 - stmtClosed - JNI method to fetch the boolean field
		gJNICache.SPJRS_stmtClosedFieldID = JNI_GetFieldID(jenv, ResultSetInfoClass, "stmtClosed", "Z");
		if (gJNICache.SPJRS_stmtClosedFieldID == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.SPJRS_stmtClosedFieldID == NULL"));

		// SPJRS

		// Constructor
		gJNICache.wrapperConstructorId = JNI_GetMethodID(jenv,wrapperClass, "<init>", "(I)V");
		if (gJNICache.wrapperConstructorId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperConstructorId == NULL"));

		// ** DataType - These are the JNI medthods used to fetch the data type field
		gJNICache.wrapperDataTypeFieldId  = JNI_GetFieldID(jenv,wrapperClass, "dataType", "[B");
		if (gJNICache.wrapperDataTypeFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperDataTypeFieldId == NULL"));

		// ** setNeeded - These are the JNI medthods used to fetch the setNeeded field
		gJNICache.wrapperSetNeededFieldId  = JNI_GetFieldID(jenv,wrapperClass, "setNeeded", "[Z");
		if (gJNICache.wrapperSetNeededFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperSetNeededFieldId == NULL"));

		// ** isNull - These are the JNI medthods used to fetch the isNull field
		gJNICache.wrapperIsNullFieldId  = JNI_GetFieldID(jenv,wrapperClass, "isNullValue", "[Z");
		if (gJNICache.wrapperIsNullFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperIsNullFieldId == NULL"));

		gJNICache.wrapperSetupObjectsMethodId = JNI_GetMethodID(jenv,wrapperClass, "setupObjects","()V");
		if (gJNICache.wrapperSetupObjectsMethodId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperSetupObjectsMethodId == NULL"));

		// ** Object - These are the JNI medthods used to fetch the object field
		gJNICache.wrapperObjectFieldId  = JNI_GetFieldID(jenv,wrapperClass, "objectValue", "[Ljava/lang/Object;");
		if (gJNICache.wrapperObjectFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperObjectFieldId == NULL"));

		// ** Bytes - These are the JNI medthods used to fetch the byte array field
		gJNICache.wrapperBytesFieldId  = JNI_GetFieldID(jenv,wrapperClass, "bytesValue", "[[B");
		if (gJNICache.wrapperBytesFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperBytesFieldId == NULL"));

        // ** SQLBytes - These are the JNI medthods used to fetch the sql byte array field
        gJNICache.wrapperSQLBytesFieldId  = JNI_GetFieldID(jenv,wrapperClass, "SQLbytesValue", "[[B");
        if (gJNICache.wrapperSQLBytesFieldId == NULL)
            FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperSQLBytesFieldId == NULL"));
//------------
		// ** Byte - These are the JNI medthods used to fetch the byte field
		gJNICache.wrapperByteFieldId  = JNI_GetFieldID(jenv,wrapperClass, "byteValue", "[B");
		if (gJNICache.wrapperByteFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperByteFieldId == NULL"));

		// ** Short - These are the JNI medthods used to fetch the short field
		gJNICache.wrapperShortFieldId  = JNI_GetFieldID(jenv,wrapperClass, "shortValue", "[S");
		if (gJNICache.wrapperShortFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperShortFieldId == NULL"));

		// ** Int - These are the JNI medthods used to fetch the int field
		gJNICache.wrapperIntFieldId  = JNI_GetFieldID(jenv,wrapperClass, "intValue", "[I");
		if (gJNICache.wrapperIntFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperIntFieldId == NULL"));

		// ** Long - These are the JNI medthods used to fetch the long field
		gJNICache.wrapperLongFieldId  = JNI_GetFieldID(jenv,wrapperClass, "longValue", "[J");
		if (gJNICache.wrapperLongFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperLongFieldId == NULL"));

		// ** Float - These are the JNI medthods used to fetch the float field
		gJNICache.wrapperFloatFieldId  = JNI_GetFieldID(jenv,wrapperClass, "floatValue", "[F");
		if (gJNICache.wrapperFloatFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperFloatFieldId == NULL"));

		// ** Double - These are the JNI medthods used to fetch the double field
		gJNICache.wrapperDoubleFieldId  = JNI_GetFieldID(jenv,wrapperClass, "doubleValue", "[D");
		if (gJNICache.wrapperDoubleFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperDoubleFieldId == NULL"));

		// ** Boolean - These are the JNI medthods used to fetch the boolean field
		gJNICache.wrapperBooleanFieldId  = JNI_GetFieldID(jenv,wrapperClass, "booleanValue", "[Z");
		if (gJNICache.wrapperBooleanFieldId == NULL)
			FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - gJNICache.wrapperBooleanFieldId == NULL"));

		// Allocate Charset structure
		gJNICache.defaultCharset = SQLCHARSETCODE_UNKNOWN;
		gJNICache.totalCharsets = 0;
		while (CHARSET_INFORMATION[gJNICache.totalCharsets].encodingName) gJNICache.totalCharsets++;
		MEMORY_ALLOC_PERM_ARRAY(gJNICache.charsetInfo,Charset_def,gJNICache.totalCharsets);
		for (int idx=0; idx<gJNICache.totalCharsets; idx++)
		{
			gJNICache.charsetInfo[idx].charset = CHARSET_INFORMATION[idx].charset;
			gJNICache.charsetInfo[idx].encodingName = CHARSET_INFORMATION[idx].encodingName;
			jstring encoding = jenv->NewStringUTF(gJNICache.charsetInfo[idx].encodingName);
			if (encoding==NULL)
				FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - Encoding NewStringUTF() == NULL"));
			gJNICache.charsetInfo[idx].encodingNameJava = (jstring) JNI_NewGlobalRef(jenv,encoding);
			if (gJNICache.charsetInfo[idx].encodingNameJava==NULL) {
				jenv->DeleteLocalRef(encoding);
				FUNCTION_RETURN_NUMERIC(FALSE,("FALSE - Encoding NewGlobalRef() == NULL"));
			}
			gJNICache.charsetInfo[idx].useDefaultEncoding = FALSE;
			jenv->DeleteLocalRef(encoding);
		}
                if (setenv("MASTER_FAST_COMPLETION","0",1) != 0)
		   abort();
		gJEnv = jenv;
		FUNCTION_RETURN_NUMERIC(TRUE,(NULL));
	}

	void convertInt64ToWrapper(struct WrapperInfoStruct *wrapperInfo, jint columnIndex, Int64 value, short scale)
	{
		FUNCTION_ENTRY("convertInt64ToWrapper",("columnIndex=%ld, value=%Ld, scale=%d",
			columnIndex,
			value,
			scale));

		if (scale>0) setWrapperDouble(wrapperInfo,columnIndex,value/pow((double)10,scale));
	else
		setWrapperLong(wrapperInfo, columnIndex, value);
		FUNCTION_RETURN_VOID((NULL));
	}

	BOOL setWrapper(struct WrapperInfoStruct *wrapperInfo, jobject jobj, SRVR_STMT_HDL *pSrvrStmt, jint columnIndex, SQLValue_def *SQLValue,
		BOOL isIndexParam)
	{
		FUNCTION_ENTRY("setWrapper",("pSrvrStmt=0x%08x, columnIndex=%ld, SQLValue=0x%08x, isIndexParam=%d",
			pSrvrStmt,
			columnIndex,
			SQLValue,
			isIndexParam));

		char					*strDataPtr = NULL;
		long					DataLen;
		Int64					source;
		// Big Num Changes
		char					cTmpBuf[1024];
		memset(cTmpBuf, '\0', 1024);
		// Big Num Changes
		short					currPos = 0;
		DATE_TYPES				*SQLDate;
		TIME_TYPES				*SQLTime;
		unsigned long			ulFraction;
		short					retCode;
		BOOL					usejchar = FALSE;
		jchar					*uStr;
		size_t					retLength;
		char					tmpString[32];
		jint					precision;
		jint					scale;
		jboolean				isSigned;
		jint					sqlDatetimeCode;
		jint					FSDataType;

        char                    *strSQLDataPtr = (char*)SQLValue->dataValue._buffer;
        long                    SQLDataLen = SQLValue->dataValue._length;

        if (strSQLDataPtr){
            jbyteArray stringSQLByteArray = JNI_NewByteArray(wrapperInfo->jenv,SQLDataLen);
            if (stringSQLByteArray == NULL)
            {
                DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE, ("stringSQLByteArray is NULL, returning NULL"));
            }
            else
            {
                JNI_SetByteArrayRegion(wrapperInfo->jenv,stringSQLByteArray,0, SQLDataLen, (jbyte *)strSQLDataPtr);
                MEMORY_DUMP(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,strSQLDataPtr,SQLDataLen);
            }
            setWrapperSQLBytes(wrapperInfo,columnIndex,stringSQLByteArray);
        }
//---------------------------------------
		DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLValue: dataType=%s dataValue._buffer=0x%08x, dataValue._length=%ld",
			CliDebugSqlTypeCode(SQLValue->dataType),
			SQLValue->dataValue._buffer,
			SQLValue->dataValue._length));

		SRVR_DESC_HDL *IRD;
		IRD = pSrvrStmt->IRD;
		scale = IRD[columnIndex].scale;

		// Linux port- moving here from case block because of compile errors
		const char *bigNum;
		jstring bigNumAsString;
		jbyteArray columnValue;
		
		switch (SQLValue->dataType)
		{
		case SQLTYPECODE_CHAR:
			strDataPtr = (char *)SQLValue->dataValue._buffer;

			// Returns true if dataCharset is ISO88591
			if(nullRequired(SQLValue->dataCharset))
			{
				DataLen = SQLValue->dataValue._length - 1;
				*(strDataPtr + DataLen) = '\0';
			}
			else
			{
				DataLen = SQLValue->dataValue._length;
			}
			usejchar = TRUE;
			DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("char length = %ld", DataLen));
			break;
		case SQLTYPECODE_VARCHAR:
			strDataPtr = (char *)SQLValue->dataValue._buffer;
			DataLen = strlen((const char *)SQLValue->dataValue._buffer);
			DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("varchar length = %ld", DataLen));

			// Null terminate only if ISO88591
			if(nullRequired(SQLValue->dataCharset))
				*(strDataPtr + DataLen) = '\0';
			usejchar = TRUE;
			break;
		case SQLTYPECODE_CLOB:
		case SQLTYPECODE_BLOB:
			strDataPtr = (char *)SQLValue->dataValue._buffer + sizeof(short);
			DataLen = *(short *)SQLValue->dataValue._buffer;
			// Null terminate only if ISO88591
			if(nullRequired(SQLValue->dataCharset))
				*(strDataPtr + DataLen) = '\0';
			usejchar = TRUE;
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		case SQLTYPECODE_VARCHAR_LONG:
		case SQLTYPECODE_INTERVAL:
		case SQLTYPECODE_DATETIME:

			FSDataType = IRD[columnIndex].FSDataType;

			if (FSDataType != 151)
			{
				if ( pSrvrStmt->IRD[columnIndex].vc_ind_length == 4)
				{
					strDataPtr = (char *)SQLValue->dataValue._buffer + sizeof(int);
					DataLen = *(int *)SQLValue->dataValue._buffer;
				}
				else
				{
					strDataPtr = (char *)SQLValue->dataValue._buffer + sizeof(short);
					DataLen = *(short *)SQLValue->dataValue._buffer;
				}
				DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,("varchar with length = %ld", DataLen));
				usejchar = TRUE;
			}
			else
			{
				retCode = ConvertDecimalToChar(151, // REC_DECIMAL_LS
					SQLValue->dataValue._buffer, SQLValue->dataValue._length, scale, cTmpBuf);
				switch (retCode)
				{
				case 0:
					break;
				default:
					throwSQLException(wrapperInfo->jenv, PROGRAMMING_ERROR, gJNILayerErrorMsgs[RESTRICTED_DATATYPE_ERROR],
						"HY000");
					FUNCTION_RETURN_NUMERIC(FALSE,("ConvertDecimalToChar() returned %d. RESTRICTED_DATATYPE_ERROR thrown.",retCode));
				}
				strDataPtr = cTmpBuf;
				break;
			}
			break;

		case SQLTYPECODE_SMALLINT:
			source = *(short *)SQLValue->dataValue._buffer;
			convertInt64ToWrapper(wrapperInfo,columnIndex,source,scale);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_SMALLINT: SQL datavalue = %Ld, Scale = %d",
				source,
				scale));
			break;
		case SQLTYPECODE_SMALLINT_UNSIGNED:
			source = *(unsigned short *)SQLValue->dataValue._buffer;
			convertInt64ToWrapper(wrapperInfo,columnIndex,source,scale);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_SMALLINT_UNSIGNED: SQL datavalue = %Ld, Scale = %d",
				source,
				scale));
			break;
		case SQLTYPECODE_INTEGER:
			source = *(int *)SQLValue->dataValue._buffer;
			convertInt64ToWrapper(wrapperInfo,columnIndex,source,scale);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_INTEGER: SQL datavalue = %Ld, Scale = %d",
				source,
				scale));
			break;
		case SQLTYPECODE_INTEGER_UNSIGNED:
			source = *(unsigned int *)SQLValue->dataValue._buffer;
			convertInt64ToWrapper(wrapperInfo,columnIndex,source,scale);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_INTEGER_UNSIGNED: SQL datavalue = %Ld, Scale = %d",
				source,
				scale));
			break;
		case SQLTYPECODE_LARGEINT:
			source = *(Int64 *)SQLValue->dataValue._buffer;
			convertInt64ToWrapper(wrapperInfo,columnIndex,source,scale);
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_LARGEINT: SQL datavalue = %Ld, Scale = %d",
				source,
				scale));
			break;
		case SQLTYPECODE_NUMERIC:
		case SQLTYPECODE_NUMERIC_UNSIGNED:
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: isIndexParm = %ld",
				isIndexParam));
			/*if (isIndexParam)
			FSDataType = wrapperInfo->jenv->CallIntMethod(jobj, gJNICache.getOutFSDataTypeMethodId, columnIndex);
			else
			FSDataType = wrapperInfo->jenv->CallIntMethod(jobj, gJNICache.RSgetFSDataTypeMethodId, columnIndex); //Anitha --10-060329-5459 */
			/* 
			* Description: JDBC/MX no longer throws Restricted Data Type Exception
			*/

			FSDataType = IRD[columnIndex].FSDataType;

			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: FSDataType = %ld",
				FSDataType));
			switch (FSDataType)
			{
			case _SQLDT_16BIT_S:
				source = *(short *)SQLValue->dataValue._buffer;
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_16BIT_S: SQL datavalue = %Ld, Scale = %d",
					source,
					scale));
				break;
			case _SQLDT_16BIT_U:
				source = *(unsigned short *)SQLValue->dataValue._buffer;
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_16BIT_U: SQL datavalue = %Ld, Scale = %d",
					source,
					scale));
				break;
			case _SQLDT_32BIT_S:
				source = *(int *)SQLValue->dataValue._buffer;
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_32BIT_S: SQL datavalue = %Ld, Scale = %d",
					source,
					scale));
				break;
			case _SQLDT_32BIT_U:
				source = *(unsigned int *)SQLValue->dataValue._buffer;
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_32BIT_U: SQL datavalue = %Ld, Scale = %d",
					source,
					scale));
				break;
			case _SQLDT_64BIT_S:
				source = *(Int64 *)SQLValue->dataValue._buffer;
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_NUMERIC: _SQLDT_64BIT_S: SQL datavalue = %Ld, Scale = %d",
					source,
					scale));
				break;
				// Big Num Changes
			case _SQLDT_NUM_BIG_U:
			case _SQLDT_NUM_BIG_S:
				columnValue = wrapperInfo->jenv->NewByteArray(SQLValue->dataValue._length);
				wrapperInfo->jenv->SetByteArrayRegion(columnValue, 0, SQLValue->dataValue._length, (jbyte *)SQLValue->dataValue._buffer);
				bigNumAsString = convertSQLBigNumToBigDecimal2(wrapperInfo, columnValue, scale);
				bigNum = wrapperInfo->jenv->GetStringUTFChars(bigNumAsString, NULL);
				strcpy(cTmpBuf, bigNum);
				wrapperInfo->jenv->ReleaseStringUTFChars(bigNumAsString, bigNum);
				bigNum = NULL;
				wrapperInfo->jenv->DeleteLocalRef(columnValue);
				strDataPtr = cTmpBuf;
				break;
			// Big Num Changes
			default:
				throwSQLException(wrapperInfo->jenv, PROGRAMMING_ERROR, gJNILayerErrorMsgs[RESTRICTED_DATATYPE_ERROR],
					"HY000");
				FUNCTION_RETURN_NUMERIC(FALSE,("wrapperInfo->jenv->CallIntMethod() returned %ld. RESTRICTED_DATATYPE_ERROR thrown.",FSDataType));
			}
			if(_SQLDT_NUM_BIG_U != FSDataType &&
				_SQLDT_NUM_BIG_S != FSDataType)
			{
			strDataPtr = convertInt64toAscii(source, scale, cTmpBuf);
			}
			// Big Num Changes
			break;
		case SQLTYPECODE_DECIMAL:
		case SQLTYPECODE_DECIMAL_UNSIGNED:
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_DECIMAL: SQL datavalue = %*s, scale = %ld",
				SQLValue->dataValue._length,
				SQLValue->dataValue._buffer,
				scale));
			retCode = ConvertDecimalToChar(SQLValue->dataType,
				SQLValue->dataValue._buffer, SQLValue->dataValue._length, scale, cTmpBuf);
			if (retCode)
			{
				throwSQLException(wrapperInfo->jenv, PROGRAMMING_ERROR, gJNILayerErrorMsgs[RESTRICTED_DATATYPE_ERROR],
					"HY000");
				FUNCTION_RETURN_NUMERIC(FALSE,("ConvertDecimalToChar() returned %d. RESTRICTED_DATATYPE_ERROR thrown.",retCode));
			}
			strDataPtr = cTmpBuf;
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_DECIMAL: JDBC datavalue = %s",
				strDataPtr));
			break;
		case SQLTYPECODE_DECIMAL_LARGE:
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
			DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_DECIMAL_LARGE: SQL datavalue = %*s, scale = %ld",
				SQLValue->dataValue._length,
				SQLValue->dataValue._buffer,
				scale));
			if ((retCode = ConvertSoftDecimalToDouble(SQLValue->dataType, SQLValue->dataValue._buffer,
				SQLValue->dataValue._length, scale, cTmpBuf)) != 0)
			{
				throwSQLException(wrapperInfo->jenv, PROGRAMMING_ERROR, gJNILayerErrorMsgs[RESTRICTED_DATATYPE_ERROR],
					"HY000");
				FUNCTION_RETURN_NUMERIC(FALSE,("ConvertSoftDecimalToDouble() returned %d. RESTRICTED_DATATYPE_ERROR thrown.",retCode));
			}
			strDataPtr = cTmpBuf;
			break;
		case SQLTYPECODE_IEEE_REAL:
			{
				float fsource = *(float *)SQLValue->dataValue._buffer;
				setWrapperFloat(wrapperInfo,columnIndex,fsource);
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_IEEE_REAL: JDBC datavalue = %g",
					fsource));
			}
			break;
		case SQLTYPECODE_IEEE_DOUBLE:
			{
				double dsource = *(double *)SQLValue->dataValue._buffer;
				setWrapperDouble(wrapperInfo,columnIndex,dsource);
				DEBUG_OUT(DEBUG_LEVEL_DATA,("SQLTYPECODE_IEEE_DOUBLE: JDBC datavalue = %g",
					dsource));
			}
			break;
			// The following datatypes should not be returned by SQL/MX.
		case SQLTYPECODE_BIT:
		case SQLTYPECODE_BITVAR:
		case SQLTYPECODE_BPINT_UNSIGNED:
		case SQLTYPECODE_FLOAT:
		case SQLTYPECODE_REAL:
		case SQLTYPECODE_DOUBLE:
		case SQLTYPECODE_IEEE_FLOAT:
			char msg[10];
			sprintf(msg,"%d",SQLValue->dataType);
			throwSQLException(wrapperInfo->jenv, DATATYPE_NOT_SUPPPORTED_ERROR, msg, "HY000", 0);
			FUNCTION_RETURN_NUMERIC(FALSE,("SQLValue->dataType=%s (unsupported). DATATYPE_NOT_SUPPPORTED_ERROR thrown.",
				CliDebugSqlTypeCode(SQLValue->dataType)));
		default:
			throwSQLException(wrapperInfo->jenv, PROGRAMMING_ERROR, gJNILayerErrorMsgs[PROGRAMMING_ERROR],
				"HY000");
			FUNCTION_RETURN_NUMERIC(FALSE,("SQLValue->dataType=%s (programming error). PROGRAMMING_ERROR thrown.",
				CliDebugSqlTypeCode(SQLValue->dataType)));
		}
		if (strDataPtr)
		{
			if (usejchar)
			{
				jbyteArray stringByteArray = JNI_NewByteArray(wrapperInfo->jenv,DataLen);
				if (stringByteArray == NULL)
				{
					DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE, ("stringByteArray is NULL, returning NULL"));
				}
				else
				{
					JNI_SetByteArrayRegion(wrapperInfo->jenv,stringByteArray,0, DataLen, (jbyte *)strDataPtr);
					MEMORY_DUMP(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE,strDataPtr,DataLen);
				}
				setWrapperBytes(wrapperInfo,columnIndex,stringByteArray);
			}
			else
			{
				jstring jstr = wrapperInfo->jenv->NewStringUTF(strDataPtr);
				if (!setWrapperObject(wrapperInfo,columnIndex,jstr,org_apache_trafodion_jdbc_t2_DataWrapper_STRING))
				{
					throwSQLException(wrapperInfo->jenv, JVM_MEM_ALLOC_ERROR, NULL, "HY000");
					FUNCTION_RETURN_NUMERIC(FALSE,("setWrapper - JVM_MEM_ALLOC_ERROR"));
				}
				DEBUG_OUT(DEBUG_LEVEL_DATA|DEBUG_LEVEL_UNICODE, ("String=%s",DebugJString(wrapperInfo->jenv, jstr)));
			}
		}
		FUNCTION_RETURN_NUMERIC(TRUE,(NULL));
	}

	jstring getCharsetEncodingJava(JNIEnv *jenv, long charset, jstring iso88591Encoding)
	{
		FUNCTION_ENTRY("getCharsetEncodingJava",("jenv=0x%08x, charset=%ld, iso88591Encoding=%s",
			jenv,
			charset,
			DebugJString(jenv,iso88591Encoding)));

		jstring rc = NULL;

		for (int idx=0; idx<gJNICache.totalCharsets; idx++)
		{
			if (charset == gJNICache.charsetInfo[idx].charset)
			{
				if (iso88591Encoding)
				{
					const char *nIso88591Encoding = JNI_GetStringUTFChars(jenv,iso88591Encoding, NULL);
					if ( strcmp(nIso88591Encoding,defaultEncodingOption) == 0)
					{
						rc = NULL;
						DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_UNICODE,
							("iso88591Encoding is set to DEFAULT, returning null"));
					}
					else
					{
						rc = iso88591Encoding;
						DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_UNICODE,
							("Using iso88591Encoding (override)"));
					}
					JNI_ReleaseStringUTFChars(jenv,iso88591Encoding, nIso88591Encoding);
				}
				else if (charset==gJNICache.defaultCharset)
				{
					rc = NULL;
					DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_UNICODE,
						("charset is equal to the default charset encoding, returning NULL"));
				}
				else // Use normal encoding (not overriden and not default)
				{
					rc = gJNICache.charsetInfo[idx].encodingNameJava;
					DEBUG_OUT(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_UNICODE,
						("Using normal charset encoding = %s", DebugJString(jenv,rc)));
				}
			}
		}
		FUNCTION_RETURN_PTR(rc,("%s",DebugJString(jenv,rc)));
	}

	const char *getCharsetEncoding(jint charset)
	{
		FUNCTION_ENTRY("getCharsetEncoding",("charset=%ld",charset));
		static __thread const char *encoding_name = NULL;
		static __thread jint last_charset = -9999;
		if (charset!=last_charset)
		{
			int idx = 0;
			while ((idx<gJNICache.totalCharsets) &&
				(charset!=gJNICache.charsetInfo[idx].charset)) idx++;
			if (idx!=gJNICache.totalCharsets) encoding_name = gJNICache.charsetInfo[idx].encodingName;
			else encoding_name = NULL;
			last_charset = charset;
		}
		if (encoding_name)
			FUNCTION_RETURN_PTR(encoding_name,("%s",encoding_name));
		FUNCTION_RETURN_PTR(NULL,("No match"));
	}


	jint getCharset(const char *encoding)
	{
		FUNCTION_ENTRY("getCharset",("encoding=%s",encoding));
		for (int idx=0; idx<gJNICache.totalCharsets; idx++)
			if (strcmp(encoding,gJNICache.charsetInfo[idx].encodingName)==0)
				FUNCTION_RETURN_NUMERIC(gJNICache.charsetInfo[idx].charset,("%s",gJNICache.charsetInfo[idx].encodingName));
		FUNCTION_RETURN_NUMERIC(SQLCHARSETCODE_UNKNOWN,("Unknown"));
	}

	BOOL useDefaultCharsetEncoding(JNIEnv *jenv, jobject jobj, jint charset, jstring iso88591Encoding)
	{
		FUNCTION_ENTRY("useDefaultCharsetEncoding",
			("jenv=0x%08x, jobj=0x%08x, charset=%s",
			jenv,
			jobj,
			getCharsetEncoding(charset),
			DebugJString(jenv,iso88591Encoding)));
		bool useDefaultEnc = FALSE;
		//  Potential change:
		//  Add logic to use default encoding only if iso88591Encoding
		//  is set to the same value as the default charset or NOT ?
		//
		if (iso88591Encoding)
			useDefaultEnc = FALSE;
		else if (charset==gJNICache.defaultCharset)
			useDefaultEnc = TRUE;
		else
		{
			for (int idx=0; idx<gJNICache.totalCharsets; idx++)
				if (charset == gJNICache.charsetInfo[idx].charset)
					useDefaultEnc = gJNICache.charsetInfo[idx].useDefaultEncoding;
		}
		FUNCTION_RETURN_NUMERIC(useDefaultEnc,(NULL));
	}


	jboolean getSqlStmtType(unsigned char* sql)
	{
		char *p = new char[strlen((const char *)sql)+1];
		memset(p, '\0', strlen((const char *)sql)+1);
		strncpy(p, (const char *)sql, strlen((const char *)sql)+1);
		/*
		jstring jstrSQL = gJEnv->NewStringUTF(p);
		jboolean result = gJEnv->CallStaticBooleanMethod(gJNICache.SQLMXConnectionClass, gJNICache.getSqlStmtTypeMethodId, jstrSQL);
		*/
		jboolean result;
		std::string s = p;
		int x = (s.find_first_of("INSERT") || s.find_first_of("UPDATE") || s.find_first_of("DELETE"));
		int y = (s.find_first_of("insert") || s.find_first_of("update") || s.find_first_of("delete"));
		if(x == 0 || y == 0)
		{
			result = JNI_TRUE;
		}
		else
		{
			result = JNI_FALSE;
		}
		delete [] p;
		p = NULL;
		return result;

	}
