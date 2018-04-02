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

#include "SequenceFileReader.h"
#include "QRLogger.h"
#include "Globals.h"

// ===========================================================================
// ===== Class SequenceFileReader
// ===========================================================================

JavaMethodInit* SequenceFileReader::JavaMethods_ = NULL;
jclass SequenceFileReader::javaClass_ = 0;
bool SequenceFileReader::javaMethodsInitialized_ = false;
pthread_mutex_t SequenceFileReader::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

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
char* SequenceFileReader::getErrorText(SFR_RetCode errEnum)
{
  if (errEnum < (SFR_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)sfrErrorEnumStr[errEnum-JOI_LAST];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SequenceFileReader::~SequenceFileReader()
{
  close();
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFR_RetCode SequenceFileReader::init()
{
  static char className[]="org/trafodion/sql/SequenceFileReader";
  SFR_RetCode rc; 

  if (javaMethodsInitialized_)
    return (SFR_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_); 
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (SFR_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR      ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR      ].jm_signature = "()V";
  //  JavaMethods_[JM_INITSERDE ].jm_name      = "initSerDe";
  //  JavaMethods_[JM_INITSERDE ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V";
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
  //  JavaMethods_[JM_FETCHCOLS ].jm_name      = "fetchArrayOfColumns";
  //  JavaMethods_[JM_FETCHCOLS ].jm_signature = "()[Ljava/lang/String;";
    JavaMethods_[JM_FETCHROW  ].jm_name      = "fetchNextRow";
    JavaMethods_[JM_FETCHROW  ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_FETCHROW2 ].jm_name      = "fetchNextRow";
    JavaMethods_[JM_FETCHROW2 ].jm_signature = "(J)Ljava/lang/String;";
    JavaMethods_[JM_FETCHBUFF1].jm_name      = "fetchArrayOfRows";
    JavaMethods_[JM_FETCHBUFF1].jm_signature = "(I)[Ljava/lang/String;";
    JavaMethods_[JM_FETCHBUFF2].jm_name      = "fetchArrayOfRows";
    JavaMethods_[JM_FETCHBUFF2].jm_signature = "(II)[Ljava/lang/String;";
    JavaMethods_[JM_CLOSE     ].jm_name      = "close";
    JavaMethods_[JM_CLOSE     ].jm_signature = "()Ljava/lang/String;";
   
    rc = (SFR_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    if (rc == SFR_OK)
       javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}
        
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
//SFR_RetCode SequenceFileReader::initSerDe(int numColumns, 
//                                         const char* fieldDelim,
//                                         const char* columns, 
//                                         const char* colTypes, 
//                                         const char* nullFormat)
//{
//  char numColumns_s[10];
//  sprintf(numColumns_s, "%d", numColumns);
//  jstring js_numColumns = jenv_->NewStringUTF(numColumns_s);
//  if (js_numColumns == NULL)
//    return SFR_ERROR_INITSERDE_PARAMS;
//  
//  jstring js_delim = jenv_->NewStringUTF(fieldDelim);
//  if (js_delim == NULL) 
//    return SFR_ERROR_INITSERDE_PARAMS;
//
//  jstring js_columns = jenv_->NewStringUTF(columns);
//  if (js_columns == NULL) 
//    return SFR_ERROR_INITSERDE_PARAMS;
//
//  jstring js_colTypes = jenv_->NewStringUTF(colTypes);
//  if (js_colTypes == NULL) 
//    return SFR_ERROR_INITSERDE_PARAMS;
//
//  jstring js_nullFormat = jenv_->NewStringUTF(nullFormat);
//  if (js_nullFormat == NULL) 
//    return SFR_ERROR_INITSERDE_PARAMS;
//
//  jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_INITSERDE].methodID, js_numColumns, js_delim, js_columns, js_colTypes, js_nullFormat);
//
//  jenv_->DeleteLocalRef(js_numColumns);  
//  jenv_->DeleteLocalRef(js_delim);  
//  jenv_->DeleteLocalRef(js_columns);  
//  jenv_->DeleteLocalRef(js_colTypes);  
//  jenv_->DeleteLocalRef(js_nullFormat);  
//  
//  if (jenv_->ExceptionCheck()) 
//  {
//    //jenv_->ExceptionDescribe();
//    jenv_->ExceptionClear();
//    return SFR_ERROR_INITSERDE_EXCEPTION;
//  }
//
//  return SFR_OK;
//}
	
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFR_RetCode SequenceFileReader::open(const char* path)
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_READER, LL_DEBUG, "SequenceFileReader::open(%s) called.", path);
  if (initJNIEnv() != JOI_OK)
     return SFR_ERROR_OPEN_PARAM;
  jstring js_path = jenv_->NewStringUTF(path);
  if (js_path == NULL) {
     jenv_->PopLocalFrame(NULL);
     return SFR_ERROR_OPEN_PARAM;
  }

  // String open(java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_OPEN].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_OPEN].methodID, js_path);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileReader::open()");
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_OPEN_EXCEPTION;
  }

  if (jresult != NULL)
  {
    logError(CAT_SQL_HDFS_SEQ_FILE_READER, "SequenceFileReader::open()", jresult);
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_OPEN_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return SFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFR_RetCode SequenceFileReader::getPosition(Int64& pos)
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_READER, LL_DEBUG, "SequenceFileReader::getPosition(%ld) called.", pos);

  if (initJNIEnv() != JOI_OK)
     return SFR_ERROR_GETPOS_EXCEPTION;

  // long getPosition();
  tsRecentJMFromJNI = JavaMethods_[JM_GETPOS].jm_full_name;
  Int64 result = jenv_->CallLongMethod(javaObj_, JavaMethods_[JM_GETPOS].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileReader::getPosition()");
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_GETPOS_EXCEPTION;
  }

  if (result == -1) 
  {
    logError(CAT_SQL_HDFS_SEQ_FILE_READER, "SequenceFileReader::getPosition()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_GETPOS_EXCEPTION;
  }

  pos = result;
  jenv_->PopLocalFrame(NULL);
  return SFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFR_RetCode SequenceFileReader::seeknSync(Int64 pos)
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_READER, LL_DEBUG, "SequenceFileReader::seeknSync(%ld) called.", pos);

  if (initJNIEnv() != JOI_OK)
     return SFR_ERROR_SYNC_EXCEPTION;

  // String seeknSync(long);
  tsRecentJMFromJNI = JavaMethods_[JM_SYNC].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_SYNC].methodID, pos);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileReader::seeknSync()");
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_SYNC_EXCEPTION;
  }

  if (jresult != NULL)
  {
    logError(CAT_SQL_HDFS_SEQ_FILE_READER, "SequenceFileReader::seeknSync()", jresult);
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_SYNC_EXCEPTION;
  }
  
  jenv_->PopLocalFrame(NULL);
  return SFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFR_RetCode SequenceFileReader::isEOF(bool& isEOF)
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_READER, LL_DEBUG, "SequenceFileReader::isEOF() called.");

  if (initJNIEnv() != JOI_OK)
     return SFR_ERROR_ISEOF_EXCEPTION;
  // boolean isEOF();
  tsRecentJMFromJNI = JavaMethods_[JM_ISEOF].jm_full_name;
  bool result = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_ISEOF].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileReader::seeknSync()");
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_ISEOF_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  isEOF = result;
  return SFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFR_RetCode SequenceFileReader::fetchNextRow(Int64 stopOffset, char* buffer)
{
  if (initJNIEnv() != JOI_OK)
     return SFR_ERROR_FETCHROW_EXCEPTION;

  // java.lang.String fetchNextRow(long stopOffset);
  tsRecentJMFromJNI = JavaMethods_[JM_FETCHROW2].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_FETCHROW2].methodID, stopOffset);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileReader::fetchNextRow()");
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_FETCHROW_EXCEPTION;
  }
  if (jresult==NULL && getLastError()) 
  {
    logError(CAT_SQL_HDFS_SEQ_FILE_READER, "SequenceFileReader::fetchNextRow()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_FETCHROW_EXCEPTION;
  }

  if (jresult == NULL)
  {
    jenv_->PopLocalFrame(NULL);
    return SFR_NOMORE;
  }
  
  const char* char_result = jenv_->GetStringUTFChars(jresult, 0);
  strcpy(buffer, char_result);
  jenv_->ReleaseStringUTFChars(jresult, char_result);
  jenv_->PopLocalFrame(NULL);
  return SFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFR_RetCode SequenceFileReader::close()
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_READER, LL_DEBUG, "SequenceFileReader::close() called.");

  if (initJNIEnv() != JOI_OK)
     return SFR_ERROR_CLOSE_EXCEPTION;
    
  // String close();
  tsRecentJMFromJNI = JavaMethods_[JM_CLOSE].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_CLOSE].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileReader::close()");
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_CLOSE_EXCEPTION;
  }

  if (jresult!=NULL) 
  {
    logError(CAT_SQL_HDFS_SEQ_FILE_READER, "SequenceFileReader::close()", jresult);
    jenv_->PopLocalFrame(NULL);
    return SFR_ERROR_CLOSE_EXCEPTION;
  }
  
  jenv_->PopLocalFrame(NULL);
  return SFR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
jstring SequenceFileReader::getLastError()
{
  // String getLastError();
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GETERROR].methodID);

  return jresult;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFR_RetCode SequenceFileReader::fetchRowsIntoBuffer(Int64   stopOffset, 
                                                    char*   buffer, 
                                                    Int64   buffSize, 
                                                    Int64&  bytesRead, 
                                                    char    rowDelimiter)
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_READER, LL_DEBUG, "SequenceFileReader::fetchRowsIntoBuffer(stopOffset: %ld, buffSize: %ld) called.", stopOffset, buffSize);
  Int32 maxRowLength = 0;
  char* pos = buffer;
  Int64 limit = buffSize;
  SFR_RetCode retCode;
  long rowsRead=0;
  bytesRead = 0;
  do
  {
    retCode = fetchNextRow(stopOffset, pos);
    if (retCode == SFR_OK)
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
  } while (retCode == SFR_OK && bytesRead < limit);
  
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_READER, LL_DEBUG, "  =>Returning %d, read %ld bytes in %d rows.", retCode, bytesRead, rowsRead);
  return retCode;
}

// ===========================================================================
// ===== Class SequenceFileWriter
// ===========================================================================

JavaMethodInit* SequenceFileWriter::JavaMethods_ = NULL;
jclass SequenceFileWriter::javaClass_ = 0;
bool SequenceFileWriter::javaMethodsInitialized_ = false;
pthread_mutex_t SequenceFileWriter::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const sfwErrorEnumStr[] = 
{
  "JNI NewStringUTF() in open() for writing."
 ,"Java exception in open() for writing."
 ,"JNI NewStringUTF() in write()"
 ,"Java exception in write()"
 ,"Java exception in close() after writing."
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* SequenceFileWriter::getErrorText(SFW_RetCode errEnum)
{
  if (errEnum < (SFW_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)sfwErrorEnumStr[errEnum-SFW_FIRST];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SequenceFileWriter::~SequenceFileWriter()
{
  close();
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFW_RetCode SequenceFileWriter::init()
{
  static char className[]="org/trafodion/sql/SequenceFileWriter";
  SFW_RetCode rc;
  
  if (javaMethodsInitialized_)
    return (SFW_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (SFW_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR      ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR      ].jm_signature = "()V";
    JavaMethods_[JM_OPEN      ].jm_name      = "open";
    JavaMethods_[JM_OPEN      ].jm_signature = "(Ljava/lang/String;I)Ljava/lang/String;";
    JavaMethods_[JM_WRITE     ].jm_name      = "write";
    JavaMethods_[JM_WRITE     ].jm_signature = "(Ljava/lang/String;)Ljava/lang/String;";
    JavaMethods_[JM_CLOSE     ].jm_name      = "close";
    JavaMethods_[JM_CLOSE     ].jm_signature = "()Ljava/lang/String;";

    rc = (SFW_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    if (rc == SFW_OK)
       javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFW_RetCode SequenceFileWriter::open(const char* path, SFW_CompType compression)
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_WRITER, LL_DEBUG, "SequenceFileWriter::open(%s) called.", path);
  if (initJNIEnv() != JOI_OK)
     return SFW_ERROR_OPEN_PARAM;
  jstring js_path = jenv_->NewStringUTF(path);
  if (js_path == NULL) {
     jenv_->PopLocalFrame(NULL);
     return SFW_ERROR_OPEN_PARAM;
  }

  // String open(java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_OPEN].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_OPEN].methodID, js_path, compression);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileWriter::open()");
    jenv_->PopLocalFrame(NULL);
    return SFW_ERROR_OPEN_EXCEPTION;
  }

  if (jresult != NULL)
  {
    logError(CAT_SQL_HDFS_SEQ_FILE_WRITER, "SequenceFileWriter::open()", jresult);
    jenv_->PopLocalFrame(NULL);
    return SFW_ERROR_OPEN_EXCEPTION;
  }
  
  jenv_->PopLocalFrame(NULL);
  return SFW_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFW_RetCode SequenceFileWriter::write(const char* data)
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_WRITER, LL_DEBUG, "SequenceFileWriter::write(%s) called.", data);
  if (initJNIEnv() != JOI_OK)
     return SFW_ERROR_WRITE_PARAM;

  jstring js_data = jenv_->NewStringUTF(data);
  if (js_data == NULL) {
    jenv_->PopLocalFrame(NULL);
    return SFW_ERROR_WRITE_PARAM;
  }

  // String write(java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_WRITE].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_WRITE].methodID, js_data);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileWriter::write()");
    jenv_->PopLocalFrame(NULL);
    return SFW_ERROR_WRITE_EXCEPTION;
  }

  if (jresult != NULL)
  {
    logError(CAT_SQL_HDFS_SEQ_FILE_WRITER, "SequenceFileWriter::write()", jresult);
    jenv_->PopLocalFrame(NULL);
    return SFW_ERROR_WRITE_EXCEPTION;
  }
  
  jenv_->PopLocalFrame(NULL);
  return SFW_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFW_RetCode SequenceFileWriter::writeBuffer(char* data, Int64 buffSize, const char* rowDelimiter)
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_WRITER, LL_DEBUG, "SequenceFileWriter::writeBuffer() called.");

  // Point to the first row.
  char* nextRow = data;
  char* buffEnd = data + buffSize;
  char* rowEnd = NULL;
  do
  {
    // Find the end of the row
    rowEnd = strchr(nextRow, *rowDelimiter);
    if (rowEnd && rowEnd < buffEnd)
    {
      // We need the row to become a null-terminated string.
      *rowEnd = '\0';
      SFW_RetCode result = write(nextRow);
      if (result != SFW_OK)
        return result;
      
      // Skip to the next row.
      nextRow = rowEnd+1;
    } 
  } while (rowEnd && nextRow < buffEnd);
  
  return SFW_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SFW_RetCode SequenceFileWriter::close()
{
  QRLogger::log(CAT_SQL_HDFS_SEQ_FILE_WRITER, LL_DEBUG, "SequenceFileWriter::close() called.");
    
  if (initJNIEnv() != JOI_OK)
     return SFW_ERROR_CLOSE_EXCEPTION;

  // String close();
  tsRecentJMFromJNI = JavaMethods_[JM_CLOSE].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_CLOSE].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "SequenceFileWriter::close()");
    jenv_->PopLocalFrame(NULL);
    return SFW_ERROR_CLOSE_EXCEPTION;
  }

  if (jresult != NULL)
  {
    logError(CAT_SQL_HDFS_SEQ_FILE_WRITER, "SequenceFileWriter::close()", jresult);
    jenv_->PopLocalFrame(NULL);
    return SFW_ERROR_CLOSE_EXCEPTION;
  }
  
  jenv_->PopLocalFrame(NULL);
  return SFW_OK;
}

