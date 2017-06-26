// **********************************************************************
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
// **********************************************************************

#include "OrcFileReader.h"
#include "QRLogger.h"

// ===========================================================================
// ===== Class OrcFileReader
// ===========================================================================

JavaMethodInit* OrcFileReader::JavaMethods_ = NULL;
jclass OrcFileReader::javaClass_ = 0;

static const char* const sfrErrorEnumStr[] = 
{
  "No more data."
 ,"JNI NewStringUTF() in initSerDe()"
 ,"Java exception in initSerDe()"
 ,"JNI NewStringUTF() in open()"
 ,"Java exception in open()"
 ,"Java exception in getPos()"
 ,"Java exception in seeknSync()"
 ,"Java exception in isEOF()"
 ,"Java exception in fetchNextRow()"
 ,"Java exception in close()"
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* OrcFileReader::getErrorText(OFR_RetCode errEnum)
{
  if (errEnum < (OFR_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)sfrErrorEnumStr[errEnum-JOI_LAST];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OrcFileReader::~OrcFileReader()
{
  close();
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OFR_RetCode OrcFileReader::init()
{
  static char className[]="org/trafodion/sql/OrcFileReader";
  
  if (JavaMethods_)
    return (OFR_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, TRUE);       
  else
  {
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR      ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR      ].jm_signature = "()V";
    JavaMethods_[JM_GETERROR  ].jm_name      = "getLastError";
    JavaMethods_[JM_GETERROR  ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_OPEN      ].jm_name      = "open";
    JavaMethods_[JM_OPEN      ].jm_signature = "(Ljava/lang/String;)Ljava/lang/String;";
    JavaMethods_[JM_GETPOS    ].jm_name      = "getPosition";
    JavaMethods_[JM_GETPOS    ].jm_signature = "()J";
    JavaMethods_[JM_SYNC      ].jm_name      = "seeknSync";
    JavaMethods_[JM_SYNC      ].jm_signature = "(J)Ljava/lang/String;";
    JavaMethods_[JM_ISEOF     ].jm_name      = "isEOF";
    JavaMethods_[JM_ISEOF     ].jm_signature = "()Z";
//    JavaMethods_[JM_FETCHROW  ].jm_name      = "fetchNextRow";
//    JavaMethods_[JM_FETCHROW  ].jm_signature = "()Ljava/lang/String;";
//    JavaMethods_[JM_FETCHROW2 ].jm_name      = "fetchNextRow";
//    JavaMethods_[JM_FETCHROW2 ].jm_signature = "(J)Ljava/lang/String;";
//    JavaMethods_[JM_FETCHROW  ].jm_name      = "fetchNextRow";
//    JavaMethods_[JM_FETCHROW  ].jm_signature = "(C)Ljava/lang/String;";
//    JavaMethods_[JM_FETCHROW2 ].jm_name      = "fetchNextRow";
//    JavaMethods_[JM_FETCHROW2 ].jm_signature = "(C)Ljava/lang/String;";
    JavaMethods_[JM_FETCHROW  ].jm_name      = "fetchNextRow";
    JavaMethods_[JM_FETCHROW  ].jm_signature = "()[B";
//    JavaMethods_[JM_FETCHROW2 ].jm_name      = "fetchNextRow";
//    JavaMethods_[JM_FETCHROW2 ].jm_signature = "()[B";
    JavaMethods_[JM_FETCHROW2 ].jm_name      = "fetchNextRowObj";
    JavaMethods_[JM_FETCHROW2 ].jm_signature = "()Lorg/trafodion/sql/OrcFileReader$OrcRowReturnSQL;";
    JavaMethods_[JM_GETNUMROWS ].jm_name      = "getNumberOfRows";
    JavaMethods_[JM_GETNUMROWS ].jm_signature = "()J";
//    JavaMethods_[JM_FETCHBUFF1].jm_name      = "fetchArrayOfRows";
//    JavaMethods_[JM_FETCHBUFF1].jm_signature = "(I)[Ljava/lang/String;";
//    JavaMethods_[JM_FETCHBUFF2].jm_name      = "fetchArrayOfRows";
//    JavaMethods_[JM_FETCHBUFF2].jm_signature = "(II)[Ljava/lang/String;";
    JavaMethods_[JM_CLOSE     ].jm_name      = "close";
    JavaMethods_[JM_CLOSE     ].jm_signature = "()Ljava/lang/String;";
   
    return (OFR_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, FALSE);
  }
}
	
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OFR_RetCode OrcFileReader::open(const char* path)
{
  QRLogger::log(CAT_SQL_HDFS_ORC_FILE_READER, LL_DEBUG, "OrcFileReader::open(%s) called.", path);
  jstring js_path = jenv_->NewStringUTF(path);
  if (js_path == NULL) 
    return OFR_ERROR_OPEN_PARAM;

  // String open(java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_OPEN].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_OPEN].methodID, js_path);

  jenv_->DeleteLocalRef(js_path);  

  if (jresult != NULL)
  {
  	const char *my_string = jenv_->GetStringUTFChars(jresult, JNI_FALSE);
  	printf("open error: %s\n", my_string);
    logError(CAT_SQL_HDFS_ORC_FILE_READER, "OrcFileReader::open()", jresult);
    return OFR_ERROR_OPEN_EXCEPTION;
  }
  
  return OFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OFR_RetCode OrcFileReader::getPosition(Int64& pos)
{
  QRLogger::log(CAT_SQL_HDFS_ORC_FILE_READER, LL_DEBUG, "OrcFileReader::getPosition(%ld) called.", pos);

  // long getPosition();
  tsRecentJMFromJNI = JavaMethods_[JM_GETPOS].jm_full_name;
  Int64 result = jenv_->CallLongMethod(javaObj_, JavaMethods_[JM_GETPOS].methodID);

  if (result == -1) 
  {
    logError(CAT_SQL_HDFS_ORC_FILE_READER, "OrcFileReader::getPosition()", getLastError());
    return OFR_ERROR_GETPOS_EXCEPTION;
  }

  pos = result;
  return OFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OFR_RetCode OrcFileReader::seeknSync(Int64 pos)
{
	Int64 orcPos;
	
	orcPos = pos -1;	//When you position in ORC, reading the NEXT row will be one greater than what you wanted.
  QRLogger::log(CAT_SQL_HDFS_ORC_FILE_READER, LL_DEBUG, "OrcFileReader::seeknSync(%ld) called.", pos);

  // String seeknSync(long);
  tsRecentJMFromJNI = JavaMethods_[JM_SYNC].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_SYNC].methodID, orcPos);

  if (jresult != NULL)
  {
  	const char *my_string = jenv_->GetStringUTFChars(jresult, JNI_FALSE);
  	printf("seeknSync error: %s\n", my_string);
    logError(CAT_SQL_HDFS_ORC_FILE_READER, "OrcFileReader::seeknSync()", jresult);
    return OFR_ERROR_SYNC_EXCEPTION;
  }
  
  return OFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OFR_RetCode OrcFileReader::isEOF(bool& isEOF)
{
  QRLogger::log(CAT_SQL_HDFS_ORC_FILE_READER, LL_DEBUG, "OrcFileReader::isEOF() called.");

  // boolean isEOF();
  tsRecentJMFromJNI = JavaMethods_[JM_ISEOF].jm_full_name;
  bool result = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_ISEOF].methodID);

  isEOF = result;
  return OFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
//OFR_RetCode OrcFileReader::fetchNextRow(Int64 stopOffset, char* buffer)
OFR_RetCode OrcFileReader::fetchNextRow(char * buffer, long& array_length, long& rowNumber, int& num_columns)
{
/*
  // java.lang.String fetchNextRow(long stopOffset);
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_FETCHROW2].methodID, stopOffset);
  if (jresult==NULL && getLastError()) 
  {
    logError(CAT_SQL_HDFS_ORC_FILE_READER, "OrcFileReader::fetchNextRow()", getLastError());
    return OFR_ERROR_FETCHROW_EXCEPTION;
  }

  if (jresult == NULL)
  {
    return OFR_NOMORE;
  }
  
  const char* char_result = jenv_->GetStringUTFChars(jresult, 0);
  strcpy(buffer, char_result);
  jenv_->ReleaseStringUTFChars(jresult, char_result);
  jenv_->DeleteLocalRef(jresult);  
  return OFR_OK;
*/


	jfieldID fid;

        tsRecentJMFromJNI = JavaMethods_[JM_FETCHROW2].jm_full_name;
	jobject jresult = (jobject)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_FETCHROW2].methodID);
	if (jresult==NULL && getLastError()) 
  	{
		  logError(CAT_SQL_HDFS_ORC_FILE_READER, "OrcFileReader::fetchNextRow()", getLastError());
		  return OFR_ERROR_FETCHROW_EXCEPTION;
  	}

	if (jresult == NULL)
		return (OFR_NOMORE);		//No more rows

//Retrieve row and associated data		
	jclass cls = jenv_->GetObjectClass(jresult);
	
	fid = jenv_->GetFieldID(cls,"m_row_length","I");
	if (fid ==NULL)
		{
			return (OFR_ERROR_FETCHROW_EXCEPTION);
		}		
	jint row_length = (jint)jenv_->GetIntField(jresult, fid);
	array_length = (long)row_length;

	
	fid = jenv_->GetFieldID(cls,"m_column_count","I");
	if (fid ==NULL)
		{
			return(OFR_ERROR_FETCHROW_EXCEPTION);
		}
	jint column_count = (jint)jenv_->GetIntField(jresult, fid);
	num_columns = column_count;

	fid = jenv_->GetFieldID(cls,"m_row_number","J");
	if (fid ==NULL)
		{
			return(OFR_ERROR_FETCHROW_EXCEPTION);
		}
	jlong rowNum = (jlong)jenv_->GetIntField(jresult, fid);
	rowNumber = rowNum;

	
// Get the actual row (it is a byte array). Use the row_length above to specify how much to copy	
	fid = jenv_->GetFieldID(cls,"m_row_ba","[B");
	if (fid ==NULL)
		{
			return (OFR_ERROR_FETCHROW_EXCEPTION);
		}
	jbyteArray jrow = (jbyteArray)jenv_->GetObjectField(jresult, fid);

  if (jrow == NULL)
  		return (OFR_ERROR_FETCHROW_EXCEPTION);

		jenv_->GetByteArrayRegion(jrow, 0, row_length, (jbyte*)buffer);
 		jenv_->DeleteLocalRef(jrow);  

  return (OFR_OK);
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OFR_RetCode OrcFileReader::close()
{
  QRLogger::log(CAT_SQL_HDFS_ORC_FILE_READER, LL_DEBUG, "OrcFileReader::close() called.");
  if (javaObj_ == NULL)
  {
    // Maybe there was an initialization error.
    return OFR_OK;
  }
    
  // String close();
  tsRecentJMFromJNI = JavaMethods_[JM_CLOSE].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_CLOSE].methodID);

  if (jresult!=NULL) 
  {
    logError(CAT_SQL_HDFS_ORC_FILE_READER, "OrcFileReader::close()", jresult);
    return OFR_ERROR_CLOSE_EXCEPTION;
  }
  
  return OFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OFR_RetCode OrcFileReader::getRowCount(Int64& count)
{
  QRLogger::log(CAT_SQL_HDFS_ORC_FILE_READER, LL_DEBUG, "OrcFileReader::getRowCount() called.");
  if (javaObj_ == NULL)
  {
    // Maybe there was an initialization error.
    return OFR_OK;
  }
    
  tsRecentJMFromJNI = JavaMethods_[JM_GETNUMROWS].jm_full_name;
  jlong jresult = (jlong)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GETNUMROWS].methodID);
  count = jresult;
  
  return OFR_OK;
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
jstring OrcFileReader::getLastError()
{
  // String getLastError();
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GETERROR].methodID);

  return jresult;
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
OFR_RetCode OrcFileReader::fetchRowsIntoBuffer(Int64   stopOffset, 
                                                    char*   buffer, 
                                                    Int64   buffSize, 
                                                    Int64&  bytesRead, 
                                                    char    rowDelimiter)
{
/*
Removed until implemented
  QRLogger::log(CAT_SQL_HDFS_ORC_FILE_READER, LL_DEBUG, "OrcFileReader::fetchRowsIntoBuffer(stopOffset: %ld, buffSize: %ld) called.", stopOffset, buffSize);
  Int32 maxRowLength = 0;
  char* pos = buffer;
  Int64 limit = buffSize;
  OFR_RetCode retCode;
  long rowsRead=0;
  bytesRead = 0;
  do
  {
    retCode = fetchNextRow(stopOffset, pos);
    if (retCode == OFR_OK)
    {
      rowsRead++;
      Int32 rowLength = strlen(pos);
      pos += rowLength;
      *pos = rowDelimiter;
      pos++;
      *pos = 0;
      
      bytesRead += rowLength+1;
      if (maxRowLength < rowLength)
        maxRowLength = rowLength;
      limit = buffSize - maxRowLength*2;
    }
  } while (retCode == OFR_OK && bytesRead < limit);
  
  QRLogger::log(CAT_SQL_HDFS_ORC_FILE_READER, LL_DEBUG, "  =>Returning %d, read %ld bytes in %d rows.", retCode, bytesRead, rowsRead);
  return retCode;
*/
	return (OFR_NOMORE);
}
