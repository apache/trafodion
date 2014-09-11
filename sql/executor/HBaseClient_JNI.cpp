// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
// **********************************************************************

#include "Context.h"
#include "Globals.h"
#include "HBaseClient_JNI.h"
#include "HdfsLogger.h"
#include <signal.h>
#include "pthread.h"
// ===========================================================================
// ===== Class StringArrayList
// ===========================================================================

JavaMethodInit* StringArrayList::JavaMethods_ = NULL;
jclass StringArrayList::javaClass_ = 0;
bool StringArrayList::javaMethodsInitialized_ = false;
pthread_mutex_t StringArrayList::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const salErrorEnumStr[] = 
{
  "JNI NewStringUTF() in add() for writing."  // SAL_ERROR_ADD_PARAM
 ,"Java exception in add() for writing."      // SAL_ERROR_ADD_EXCEPTION
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* StringArrayList::getErrorText(SAL_RetCode errEnum)
{
  if (errEnum < (SAL_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)salErrorEnumStr[errEnum-SAL_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
StringArrayList::~StringArrayList()
{
//  HdfsLogger::log(CAT_JNI_TOP, LL_DEBUG, "StringArrayList destructor called.");
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SAL_RetCode StringArrayList::init()
{
  static char className[]="org/trafodion/sql/HBaseAccess/StringArrayList";
  SAL_RetCode rc;
  
  if (isInitialized())
    return SAL_OK;

  if (javaMethodsInitialized_)
    return (SAL_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (SAL_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR      ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR      ].jm_signature = "()V";
    JavaMethods_[JM_ADD       ].jm_name      = "addElement";
    JavaMethods_[JM_ADD       ].jm_signature = "(Ljava/lang/String;)V";
    JavaMethods_[JM_GET       ].jm_name      = "getElement";
    JavaMethods_[JM_GET       ].jm_signature = "(I)Ljava/lang/String;";
    JavaMethods_[JM_GETSIZE   ].jm_name      = "getSize";
    JavaMethods_[JM_GETSIZE   ].jm_signature = "()I";
 
    rc = (SAL_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
 
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SAL_RetCode StringArrayList::add(const char* str)
{
//  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "StringArrayList::add(%s) called.", str);
  jstring js_str = jenv_->NewStringUTF(str);
  if (js_str == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(SAL_ERROR_ADD_PARAM));
    return SAL_ERROR_ADD_PARAM;
  }

  // void add(java.lang.String);
  jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_ADD].methodID, js_str);

  jenv_->DeleteLocalRef(js_str);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return SAL_ERROR_ADD_EXCEPTION;
  }

  return SAL_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SAL_RetCode StringArrayList::add(const TextVec& vec)
{
  for (std::vector<Text>::const_iterator it = vec.begin() ; it != vec.end(); ++it)
  {
    const char* str = (*it).data();
    add(str);
  }

  return SAL_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SAL_RetCode StringArrayList::add(const HBASE_NAMELIST& nameList)
{
  for (CollIndex i=0; i<nameList.entries(); i++)
  {
    const HbaseStr& str = nameList.at(i);
    add(str.val);
  }

  return SAL_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
SAL_RetCode StringArrayList::add(const NAText * createOptions)
{
  for (CollIndex i=0; i<HBASE_MAX_OPTIONS; i++)
  {
    add(createOptions[i].c_str());
  }

  return SAL_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Text* StringArrayList::get(Int32 i)
{
  // String get(i);
  jstring strVal = (jstring)(jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET].methodID, i));
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return NULL;
  }

  if (strVal == NULL)
    return NULL;

  int len = jenv_->GetStringLength(strVal);

  if (len == 0)
    return NULL;
    
  // Not using UFTchars and NAWString for now.
  const char* char_result = jenv_->GetStringUTFChars(strVal, 0);
  Text* val = new (heap_) Text(char_result, len);
  jenv_->ReleaseStringUTFChars(strVal, char_result);
  jenv_->DeleteLocalRef(strVal);  

  return val;
}

Int32 StringArrayList::getSize()
{
  Int32 len = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_GETSIZE].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return 0;
  }
  return len;
}

// ===========================================================================
// ===== Class ByteArrayList
// ===========================================================================

JavaMethodInit* ByteArrayList::JavaMethods_ = NULL;
jclass ByteArrayList::javaClass_ = 0;
bool ByteArrayList::javaMethodsInitialized_ = false;
pthread_mutex_t ByteArrayList::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const balErrorEnumStr[] = 
{
  "JNI NewStringUTF() in add() for writing."  // BAL_ERROR_ADD_PARAM
 ,"Java exception in add() for writing."      // BAL_ERROR_ADD_EXCEPTION
 ,"Java exception in get() for reading."      // BAL_ERROR_GET_EXCEPTION
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* ByteArrayList::getErrorText(BAL_RetCode errEnum)
{
  if (errEnum < (BAL_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)balErrorEnumStr[errEnum-BAL_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
ByteArrayList::~ByteArrayList()
{
//  HdfsLogger::log(CAT_JNI_TOP, LL_DEBUG, "ByteArrayList destructor called.");
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
BAL_RetCode ByteArrayList::init()
{
  static char className[]="org/trafodion/sql/HBaseAccess/ByteArrayList";
  BAL_RetCode rc;
  
  if (isInitialized())
    return BAL_OK;
    
  if (javaMethodsInitialized_)
    return (BAL_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (BAL_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR      ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR      ].jm_signature = "()V";
    JavaMethods_[JM_ADD       ].jm_name      = "addElement";
    JavaMethods_[JM_ADD       ].jm_signature = "([B)V";
    JavaMethods_[JM_GET       ].jm_name      = "getElement";
    JavaMethods_[JM_GET       ].jm_signature = "(I)[B";
    JavaMethods_[JM_GETSIZE   ].jm_name      = "getSize";
    JavaMethods_[JM_GETSIZE   ].jm_signature = "()I";
    JavaMethods_[JM_GETENTRYSIZE].jm_name      = "getEntrySize";
    JavaMethods_[JM_GETENTRYSIZE].jm_signature = "(I)I";
    JavaMethods_[JM_GETENTRY  ].jm_name      = "getEntry";
    JavaMethods_[JM_GETENTRY].jm_signature = "(I)[B";
    
    rc = (BAL_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
BAL_RetCode ByteArrayList::add(const Text& t)
{
//  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "ByteArrayList::add(%s) called.", t.data());

  int len = t.size();
  jbyteArray jba_t = jenv_->NewByteArray(len);
  if (jba_t == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(BAL_ERROR_ADD_PARAM));
    return BAL_ERROR_ADD_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_t, 0, len, (const jbyte*)t.data());

  // void add(byte[]);
  jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_ADD].methodID, jba_t);
  jenv_->DeleteLocalRef(jba_t);  
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return BAL_ERROR_ADD_EXCEPTION;
  }

  return BAL_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
BAL_RetCode ByteArrayList::add(const TextVec& vec)
{
  for (std::vector<Text>::const_iterator it = vec.begin() ; it != vec.end(); ++it)
  {
    Text str(*it);
    add(str);
  }

  return BAL_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
BAL_RetCode ByteArrayList::addElement(const char* data, int keyLength)
{
  Text str(data, keyLength);
  add(str);
  return BAL_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Text* ByteArrayList::get(Int32 i)
{
  // byte[] get(i);
  jbyteArray jba_val = static_cast<jbyteArray>(jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET].methodID, i));
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return NULL;
  }

  if (jba_val == NULL)
    return NULL;

  jbyte* p_val = jenv_->GetByteArrayElements(jba_val, 0);
  int len = jenv_->GetArrayLength(jba_val);
  Text* val = new (heap_) Text((char*)p_val, len); 
  jenv_->ReleaseByteArrayElements(jba_val, p_val, JNI_ABORT);
  jenv_->DeleteLocalRef(jba_val);  

  return val;
}

Int32 ByteArrayList::getSize()
{
  Int32 len = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_GETSIZE].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return 0;
  }
  return len;
}

Int32 ByteArrayList::getEntrySize(Int32 i)
{
  jint jidx = i;  

  Int32 len = 
    jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_GETENTRYSIZE].methodID, jidx);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return 0;
  }
  return len;
}


char* ByteArrayList::getEntry(Int32 i, char* buf, Int32 bufLen, Int32& datalen)
{
  datalen = 0;

  jint jidx = i;  

  jbyteArray jBuffer = static_cast<jbyteArray>
      (jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GETENTRY].methodID, jidx));

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return NULL;
  }

  if (jBuffer != NULL) {

    datalen = jenv_->GetArrayLength(jBuffer);

    if (datalen > bufLen)
      // call setJniErrorStr?
      return NULL;

    jenv_->GetByteArrayRegion(jBuffer, 0, datalen, (jbyte*)buf);

    jenv_->DeleteLocalRef(jBuffer);
  }

  return buf;
}

// ===========================================================================
// ===== Class KeyValue
// ===========================================================================

JavaMethodInit* KeyValue::JavaMethods_ = NULL;
jclass KeyValue::javaClass_ = 0;
bool KeyValue::javaMethodsInitialized_ = false;
pthread_mutex_t KeyValue::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const resErrorEnumStr[] = 
{
  "JNI NewStringUTF() in add() for writing."
 ,"Java exception in add() for writing."
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* KeyValue::getErrorText(KYV_RetCode errEnum)
{
  if (errEnum < (KYV_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)resErrorEnumStr[errEnum-KYV_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
KeyValue::~KeyValue()
{
//  HdfsLogger::log(CAT_JNI_TOP, LL_DEBUG, "KeyValue destructor called.");
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
KYV_RetCode KeyValue::init()
{
  static char className[]="org/apache/hadoop/hbase/KeyValue";
  KYV_RetCode rc;
  
  if (isInitialized())
    return KYV_OK;
    
  if (javaMethodsInitialized_)
    return (KYV_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (KYV_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR      ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR      ].jm_signature = "()V";
    JavaMethods_[JM_BUFFER    ].jm_name      = "getBuffer";
    JavaMethods_[JM_BUFFER    ].jm_signature = "()[B";
    JavaMethods_[JM_FAM_LEN   ].jm_name      = "getFamilyLength";
    JavaMethods_[JM_FAM_LEN   ].jm_signature = "()B";
    JavaMethods_[JM_FAM_OFF   ].jm_name      = "getFamilyOffset";
    JavaMethods_[JM_FAM_OFF   ].jm_signature = "()I";
    JavaMethods_[JM_KEY_LEN   ].jm_name      = "getKeyLength";
    JavaMethods_[JM_KEY_LEN   ].jm_signature = "()I";
    JavaMethods_[JM_KEY_OFF   ].jm_name      = "getKeyOffset";
    JavaMethods_[JM_KEY_OFF   ].jm_signature = "()I";
    JavaMethods_[JM_QUA_LEN   ].jm_name      = "getQualifierLength";
    JavaMethods_[JM_QUA_LEN   ].jm_signature = "()I";
    JavaMethods_[JM_QUA_OFF   ].jm_name      = "getQualifierOffset";
    JavaMethods_[JM_QUA_OFF   ].jm_signature = "()I";
    JavaMethods_[JM_ROW_LEN   ].jm_name      = "getRowLength";
    JavaMethods_[JM_ROW_LEN   ].jm_signature = "()S";
    JavaMethods_[JM_ROW_OFF   ].jm_name      = "getRowOffset";
    JavaMethods_[JM_ROW_OFF   ].jm_signature = "()I";
    JavaMethods_[JM_VAL_LEN   ].jm_name      = "getValueLength";
    JavaMethods_[JM_VAL_LEN   ].jm_signature = "()I";
    JavaMethods_[JM_VAL_OFF   ].jm_name      = "getValueOffset";
    JavaMethods_[JM_VAL_OFF   ].jm_signature = "()I";
    JavaMethods_[JM_TS        ].jm_name      = "getTimestamp";
    JavaMethods_[JM_TS        ].jm_signature = "()J";          
   
    rc = (KYV_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getBufferSize()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getBufferSize() called.");

  // byte[] getBuffer();
  jbyteArray jBuffer = static_cast<jbyteArray>(jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_BUFFER].methodID));
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return 0;
  }

  if (jBuffer == NULL)
    return 0;
    
  Int32 buffLen = jenv_->GetArrayLength(jBuffer);
  jenv_->DeleteLocalRef(jBuffer);  

  return buffLen;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* KeyValue::getBuffer(char* targetBuffer, Int32 buffSize)
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getBuffer() called.");

  jbyteArray jBuffer = static_cast<jbyteArray>(jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_BUFFER].methodID));
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return NULL;
  }

  Int32 buffLen = jenv_->GetArrayLength(jBuffer);

  bool buffTooSmall = false;
  if (buffLen > buffSize)
    buffTooSmall = true;
  else
    jenv_->GetByteArrayRegion(jBuffer, 0, buffLen, (jbyte*)targetBuffer);

  jenv_->DeleteLocalRef(jBuffer);  

  if (buffTooSmall)
     // Should we setJniErrorStr
    return NULL;
  else
    return targetBuffer;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getFamilyLength()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getFamilyLength() called.");

  // byte getFamilyLength();
  Int32 len = jenv_->CallByteMethod(javaObj_, JavaMethods_[JM_FAM_LEN].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return len;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getFamilyOffset()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getFamilyOffset() called.");

  // int getFamilyOffset();
  Int32 offset = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_FAM_OFF].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return offset;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getKeyLength()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getKeyLength() called.");

  // int getKeyLength();
  Int32 len = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_KEY_LEN].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return len;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getKeyOffset()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getKeyOffset() called.");

  // int getKeyOffset();
  Int32 offset = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_KEY_OFF].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return offset;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getQualifierLength()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getQualifierLength() called.");

  // int getQualifierLength();
  Int32 len = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_QUA_LEN].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return len;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getQualifierOffset()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getQualifierOffset() called.");

  // int getQualifierOffset();
  Int32 offset = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_QUA_OFF].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return offset;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getRowLength()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getRowLength() called.");

  // short getRowLength();
  Int32 len = jenv_->CallShortMethod(javaObj_, JavaMethods_[JM_ROW_LEN].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return len;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getRowOffset()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getRowOffset() called.");

  // int getRowOffset();
  Int32 offset =  jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_ROW_OFF].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return offset;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getValueLength()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getValueLength() called.");

  // int getValueLength();
  Int32 len = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_VAL_LEN].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return len;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int32 KeyValue::getValueOffset()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getValueOffset() called.");

  // int getValueOffset();
  Int32 offset = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_VAL_OFF].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return offset;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
Int64 KeyValue::getTimestamp()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::getTimestamp() called.");

  // long getTimestamp();
  Int64 timestamp =  jenv_->CallLongMethod(javaObj_, JavaMethods_[JM_TS].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return -1;
  }
  return timestamp;
}

//////////////////////////////////////////////////////////////////////////////
// Translate to the corresponding Thrift class.
//////////////////////////////////////////////////////////////////////////////
TCell* KeyValue::toTCell()
{
  //HdfsLogger::log(CAT_HBASE, LL_DEBUG, "KeyValue::toTCell() called.");
  Int32 buffSize = getBufferSize();
  char* buffer = new (heap_) char[buffSize];
  if (getBuffer(buffer, buffSize) == NULL)
  {
    // Error.
    return NULL;
  }
  
  TCell* cell = new (heap_) TCell();
  const Bytes value(buffer+getValueOffset(), getValueLength());
  
  cell->__set_value(value);
  cell->__set_timestamp(getTimestamp());
  
  NADELETEBASIC(buffer, heap_);
  return cell;
}

// ===========================================================================
// ===== Class HBaseClient_JNI
// ===========================================================================

JavaMethodInit* HBaseClient_JNI::JavaMethods_ = NULL;
jclass HBaseClient_JNI::javaClass_ = 0;
bool HBaseClient_JNI::javaMethodsInitialized_ = false;
pthread_mutex_t HBaseClient_JNI::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const hbcErrorEnumStr[] = 
{
  "Preparing parameters for initConnection()."
 ,"Java exception in initConnection()."
 ,"Java exception in cleanup()."
 ,"Java exception in getHTableClient()."
 ,"Java exception in releaseHTableClient()."
 ,"Preparing parameters for create()."
 ,"Java exception in create()."
 ,"Preparing parameters for drop()."
 ,"Java exception in drop()."
 ,"Preparing parameters for dropAll()."
 ,"Java exception in dropAll()."
 ,"Preparing parameters for exists()."
 ,"Java exception in exists()."
 ,"Preparing parameters for flushAll()."
 ,"Java exception in flushAll()." 
 ,"Preparing parameters for grant()."
 ,"Java exception in grant()."
 ,"Preparing parameters for revoke()."
 ,"Java exception in revoke()."
 ,"Error in Thread Create"
 ,"Error in Thread Req Alloc"
 ,"Error in Thread SIGMAS"
 ,"Error in Attach JVM"
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
// private default constructor
HBaseClient_JNI::HBaseClient_JNI(NAHeap *heap, int debugPort, int debugTimeout)
                 :  JavaObjectInterface(heap, debugPort, debugTimeout)
                   ,isConnected_(FALSE)
{
  for (int i=0; i<NUM_HBASE_WORKER_THREADS; i++) {
    threadID_[i] = NULL;
  }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* HBaseClient_JNI::getErrorText(HBC_RetCode errEnum)
{
  if (errEnum < (HBC_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)hbcErrorEnumStr[errEnum-HBC_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBaseClient_JNI* HBaseClient_JNI::getInstance(int debugPort, int debugTimeout)
{
   ContextCli *currContext = GetCliGlobals()->currContext();
   HBaseClient_JNI *hbaseClient_JNI = currContext->getHBaseClient();
   if (hbaseClient_JNI == NULL)
   {
     NAHeap *heap = currContext->exHeap();
    
     hbaseClient_JNI  = new (heap) HBaseClient_JNI(heap,
                   debugPort, debugTimeout);
     currContext->setHbaseClient(hbaseClient_JNI);
   }
   return hbaseClient_JNI;
}

void HBaseClient_JNI::deleteInstance()
{
   ContextCli *currContext = GetCliGlobals()->currContext();
   HBaseClient_JNI *hbaseClient_JNI = currContext->getHBaseClient();
   if (hbaseClient_JNI != NULL)
   {
      NAHeap *heap = currContext->exHeap();
      NADELETE(hbaseClient_JNI, HBaseClient_JNI, heap);
      currContext->setHbaseClient(NULL);
   }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBaseClient_JNI::~HBaseClient_JNI()
{
  //HdfsLogger::log(CAT_JNI_TOP, LL_DEBUG, "HBaseClient_JNI destructor called.");
  
  // worker threads need to go away and be joined. 
  if (threadID_[0])
  {
    // tell the worker threads to go away
    for (int i=0; i<NUM_HBASE_WORKER_THREADS; i++) {
      enqueueShutdownRequest(); 
    }

    // wait for worker threads to exit and join
    for (int i=0; i<NUM_HBASE_WORKER_THREADS; i++) {
      pthread_join(threadID_[i], NULL);
    }

    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&workBell_);
  }
  // Clean the Java Side
  cleanup();
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::init()
{
  static char className[]="org/trafodion/sql/HBaseAccess/HBaseClient";
  HBC_RetCode rc;
  
  if (isInitialized())
    return HBC_OK;
  
  if (javaMethodsInitialized_)
    return (HBC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HBC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR       ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR       ].jm_signature = "()V";
    JavaMethods_[JM_GET_ERROR  ].jm_name      = "getLastError";
    JavaMethods_[JM_GET_ERROR  ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_INIT       ].jm_name      = "init";
    JavaMethods_[JM_INIT       ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_CLEANUP    ].jm_name      = "cleanup";
    JavaMethods_[JM_CLEANUP    ].jm_signature = "()Z";
    JavaMethods_[JM_GET_HTC    ].jm_name      = "getHTableClient";
    JavaMethods_[JM_GET_HTC    ].jm_signature = "(Ljava/lang/String;Z)Lorg/trafodion/sql/HBaseAccess/HTableClient;";
    JavaMethods_[JM_REL_HTC    ].jm_name      = "releaseHTableClient";
    JavaMethods_[JM_REL_HTC    ].jm_signature = "(Lorg/trafodion/sql/HBaseAccess/HTableClient;)V";
    JavaMethods_[JM_CREATE     ].jm_name      = "create";
    JavaMethods_[JM_CREATE     ].jm_signature = "(Ljava/lang/String;[Ljava/lang/Object;)Z";
    JavaMethods_[JM_CREATEK    ].jm_name      = "createk";
    JavaMethods_[JM_CREATEK    ].jm_signature = "(Ljava/lang/String;[Ljava/lang/Object;Lorg/trafodion/sql/HBaseAccess/ByteArrayList;)Z";
    JavaMethods_[JM_DROP       ].jm_name      = "drop";
    JavaMethods_[JM_DROP       ].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_DROP_ALL       ].jm_name      = "dropAll";
    JavaMethods_[JM_DROP_ALL       ].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_COPY       ].jm_name      = "copy";
    JavaMethods_[JM_COPY       ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_EXISTS     ].jm_name      = "exists";
    JavaMethods_[JM_EXISTS     ].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_GRANT      ].jm_name      = "grant";
    JavaMethods_[JM_GRANT      ].jm_signature = "([B[B[Ljava/lang/Object;)Z";
    JavaMethods_[JM_REVOKE     ].jm_name      = "revoke";
    JavaMethods_[JM_REVOKE     ].jm_signature = "([B[B[Ljava/lang/Object;)Z";
    JavaMethods_[JM_FLUSHALL   ].jm_name      = "flushAllTables";
    JavaMethods_[JM_FLUSHALL   ].jm_signature = "()Z";
    JavaMethods_[JM_GET_HBLC   ].jm_name      = "getHBulkLoadClient";
    JavaMethods_[JM_GET_HBLC   ].jm_signature = "()Lorg/trafodion/sql/HBaseAccess/HBulkLoadClient;";
   
   
   
   
    rc = (HBC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
jstring HBaseClient_JNI::getLastJavaError()
{
  if (javaObj_ == NULL)
    return NULL;

  // java.lang.String getLastError();
  return (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_ERROR].methodID);
}
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
ByteArrayList* HBaseClient_JNI::newByteArrayList()
{
  ByteArrayList* bal = new (heap_) ByteArrayList(heap_);
  if ( bal->init() != BAL_OK)
  {
    HdfsLogger::log(CAT_HBASE, LL_ERROR, "ByteArrayList::init() error.");
    return NULL;
  }
  
/*
  if (bal->add(nameList) != BAL_OK)
  {
    HdfsLogger::log(CAT_HBASE, LL_ERROR, "StringArrayList::add() error.");
    return NULL;
  } 
*/
  
  return bal;
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::initConnection(const char* zkServers, const char* zkPort)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::initConnection(%s, %s) called.", zkServers, zkPort);

  jstring js_zkServers = jenv_->NewStringUTF(zkServers);
  if (js_zkServers == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INIT_PARAM));
    return HBC_ERROR_INIT_PARAM;
  }
  jstring js_zkPort = jenv_->NewStringUTF(zkPort);
  if (js_zkPort == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INIT_PARAM));
    return HBC_ERROR_INIT_PARAM;
  }

  // boolean init(java.lang.String, java.lang.String); 
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_INIT].methodID, js_zkServers, js_zkPort);

  jenv_->DeleteLocalRef(js_zkServers);  
  jenv_->DeleteLocalRef(js_zkPort);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::initConnection()", getLastError());
    return HBC_ERROR_INIT_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::initConnection()", getLastError());
    return HBC_ERROR_INIT_EXCEPTION;
  }

  isConnected_ = TRUE;
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::cleanup()
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::cleanup() called.");
 
  if (! (isInitialized_ && isConnected_))
     return HBC_OK;

  // boolean cleanup();
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_CLEANUP].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::cleanup()", getLastError());
    return HBC_ERROR_CLEANUP_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::cleanup()", getLastError());
    return HBC_ERROR_CLEANUP_EXCEPTION;
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTableClient_JNI* HBaseClient_JNI::getHTableClient(NAHeap *heap, const char* tableName, bool useTRex)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::getHTableClient(%s) called.", tableName);

  if (javaObj_ == NULL || (!isInitialized()))
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HTC_EXCEPTION));
    return NULL;
  }

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HTC_EXCEPTION));
    return NULL;
  }

  jobject j_htc = jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_HTC].methodID, js_tblName, (jboolean)useTRex);

  jenv_->DeleteLocalRef(js_tblName); 

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::getHTableClient()", getLastError());
    return NULL;
  }

  if (j_htc == NULL) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::getHTableClient()", getLastError());
    return NULL;
  }
  HTableClient_JNI *htc = new (heap) HTableClient_JNI(heap, j_htc);
  if (htc->init() != HTC_OK)
     return NULL;
  htc->setTableName(tableName);
  return htc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::releaseHTableClient(HTableClient_JNI* htc)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::releaseHTableClient() called.");

  jobject j_htc = htc->getJavaObject();
    
  // public void releaseHTableClient(org.trafodion.sql.HBaseAccess.HTableClient);
  jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_REL_HTC].methodID, j_htc);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::releaseHTableClient()", getLastError());
    return HBC_ERROR_REL_HTC_EXCEPTION;
  }
  NADELETE(htc, HTableClient_JNI, htc->getHeap()); 

  return HBC_OK;
}

HBulkLoadClient_JNI* HBaseClient_JNI::getHBulkLoadClient(NAHeap *heap)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::getHBulkLoadClient() called.");
  if (javaObj_ == NULL || (!isInitialized()))
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HBLC_EXCEPTION));
    return NULL;
  }


  jobject j_hblc = jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_HBLC].methodID);


  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::getHTableClient()", getLastError());
    return NULL;
  }

  if (j_hblc == NULL)
  {
    logError(CAT_HBASE, "HBaseClient_JNI::getHTableClient()", getLastError());
    return NULL;
  }
  HBulkLoadClient_JNI *hblc = new HBulkLoadClient_JNI(heap, j_hblc);
  hblc->init();
  return hblc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::create(const char* fileName, HBASE_NAMELIST& colFamilies)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::create(%s) called.", fileName);
  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_PARAM));
    return HBC_ERROR_CREATE_PARAM;
  }
  jobjectArray j_fams = convertToStringObjectArray(colFamilies);
  if (j_fams == NULL)
  {
     getExceptionDetails();
     logError(CAT_HBASE, __FILE__, __LINE__);
     logError(CAT_HBASE, "HBaseClient_JNI::create()", getLastError());
     return HBC_ERROR_CREATE_PARAM;
  }
    
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
        JavaMethods_[JM_CREATE].methodID, js_fileName, j_fams);

  jenv_->DeleteLocalRef(js_fileName); 

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::create()", getLastError());
    return HBC_ERROR_CREATE_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::create()", getLastError());
    return HBC_ERROR_CREATE_EXCEPTION;
  }

  return HBC_OK;
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::create(const char* fileName, 
                                    NAText* createOptionsArray,
                                    int numSplits, int keyLength,
                                    const char ** splitValues)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::create(%s) called.", fileName);
  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_PARAM));
    return HBC_ERROR_CREATE_PARAM;
  }
  jobjectArray j_opts = convertToStringObjectArray(createOptionsArray, 
                   HBASE_MAX_OPTIONS);
  if (j_opts == NULL)
  {
     getExceptionDetails();
     logError(CAT_HBASE, __FILE__, __LINE__);
     logError(CAT_HBASE, "HBaseClient_JNI::create()", getLastError());
     return HBC_ERROR_CREATE_PARAM;
  }
      
  ByteArrayList* beginEndKeys = newByteArrayList();
  if (beginEndKeys == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_PARAM));
    return HBC_ERROR_CREATE_PARAM;
  }
  for (int i = 0; i < numSplits; i++)
         beginEndKeys->addElement(splitValues[i], keyLength);
    

  jobject j_keys = beginEndKeys->getJavaObject();
    
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
          JavaMethods_[JM_CREATEK].methodID, js_fileName, j_opts, j_keys);

  jenv_->DeleteLocalRef(js_fileName); 
  NADELETE(beginEndKeys, ByteArrayList, beginEndKeys->getHeap());

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::create()", getLastError());
    return HBC_ERROR_CREATE_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::create()", getLastError());
    return HBC_ERROR_CREATE_EXCEPTION;
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBaseClientRequest::HBaseClientRequest(NAHeap *heap, HBaseClientReqType reqType)
                    :  heap_(heap)
                      ,reqType_(reqType)
                      ,fileName_(NULL)
{
}

HBaseClientRequest::~HBaseClientRequest()
{
  if (fileName_) {
    NADELETEBASIC(fileName_, heap_);
  }
}

void HBaseClientRequest::setFileName(const char *fileName)
{
  int len = strlen(fileName);
  fileName_ = new (heap_) char[len + 1];
  strcpy(fileName_, fileName);
}

HBC_RetCode HBaseClient_JNI::enqueueRequest(HBaseClientRequest *request)
{
  pthread_mutex_lock( &mutex_ );
  reqQueue_.push_back(request);
  pthread_cond_signal(&workBell_);
  pthread_mutex_unlock( &mutex_ );

  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::enqueueDropRequest(const char* fileName)
{
  HBaseClientRequest *request = new (heap_) HBaseClientRequest(heap_, HBC_Req_Drop);

  if (!request) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_THREAD_REQ_ALLOC));
    return HBC_ERROR_THREAD_REQ_ALLOC; 
  }

  request->setFileName(fileName);

  enqueueRequest(request);

  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::enqueueShutdownRequest()
{
  HBaseClientRequest *request = new (heap_) HBaseClientRequest(heap_, HBC_Req_Shutdown);

  if (!request) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_THREAD_REQ_ALLOC));
    return HBC_ERROR_THREAD_REQ_ALLOC;
  }

  enqueueRequest(request);

  return HBC_OK;
}

HBaseClientRequest* HBaseClient_JNI::getHBaseRequest() 
{
  HBaseClientRequest *request;
  reqList_t::iterator it;

  pthread_mutex_lock( &mutex_ );
  it = reqQueue_.begin();

  request = NULL;

  while (request == NULL)
  {
    if (it != reqQueue_.end())
    {
      request = *it;
      it = reqQueue_.erase(it);
    } else {
      pthread_cond_wait(&workBell_, &mutex_);
      it = reqQueue_.begin();
    }
  }

  pthread_mutex_unlock( &mutex_ );

  return request;
}

HBC_RetCode HBaseClient_JNI::performRequest(HBaseClientRequest *request, JNIEnv* jenv)
{
  switch (request->reqType_)
  {
    case HBC_Req_Drop :
	  drop(request->fileName_, jenv);
	  break;
	default :
	  break;
  }

  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::doWorkInThread() 
{
  int rc;

  HBaseClientRequest *request;

  // mask all signals
  sigset_t mask;
  sigfillset(&mask);
  rc = pthread_sigmask(SIG_BLOCK, &mask, NULL);
  if (rc)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_THREAD_SIGMASK));
    return HBC_ERROR_THREAD_SIGMASK;
  }

  JNIEnv* jenv; // thread local
  jint result = jvm_->GetEnv((void**) &jenv, JNI_VERSION_1_6);
  switch (result)
  {
    case JNI_OK:
	  break;

	case JNI_EDETACHED:
	  result = jvm_->AttachCurrentThread((void**) &jenv, NULL);
          if (result != JNI_OK)
          {
             GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_ATTACH_JVM));
	     return HBC_ERROR_ATTACH_JVM;
          }
      break;

	default: 
	  break;
  }

  // enter processing zone
  for (;;)
  {
    request = getHBaseRequest();

	if (request->isShutDown()) {
	  // wake up another thread as this wakeup could have consumed
	  // multiple workBell_ rings.
	  pthread_cond_signal(&workBell_);
	  break;
	} else {
      performRequest(request, jenv);
	  NADELETE(request, HBaseClientRequest, request->getHeap());
	}
  }

  jvm_->DetachCurrentThread();

  pthread_exit(0);

  return HBC_OK;
}

static void *workerThreadMain_JNI(void *arg)
{
  // parameter passed to the thread is an instance of the HBaseClient_JNI object
  HBaseClient_JNI *client = (HBaseClient_JNI *)arg;

  client->doWorkInThread();

  return NULL;
}

HBC_RetCode HBaseClient_JNI::startWorkerThreads()
{
  pthread_mutexattr_t mutexAttr;
  pthread_mutexattr_init( &mutexAttr );
  pthread_mutex_init( &mutex_, &mutexAttr );
  pthread_cond_init( &workBell_, NULL );

  int rc;
  for (int i=0; i<NUM_HBASE_WORKER_THREADS; i++) {
    rc = pthread_create(&threadID_[i], NULL, workerThreadMain_JNI, this);
    if (rc != 0)
    {
       GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_THREAD_CREATE));
       return HBC_ERROR_THREAD_CREATE;
    }
  }

  return HBC_OK;
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::drop(const char* fileName, bool async)
{
  if (async) {
    if (!threadID_[0]) {
	  startWorkerThreads();
	}
    enqueueDropRequest(fileName);
  } else {
    return drop(fileName, jenv_); // not in worker thread
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::flushAllTablesStatic()
{
  return GetCliGlobals()->currContext()->getHBaseClient()->flushAllTables();
}

HBC_RetCode HBaseClient_JNI::flushAllTables()
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::flushAllTablescalled.");

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_FLUSHALL].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::flushAllTables()", getLastError());
    return HBC_ERROR_FLUSHALL_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::flushAllTables()", getLastError());
    return HBC_ERROR_FLUSHALL_EXCEPTION;
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::drop(const char* fileName, JNIEnv* jenv)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::drop(%s) called.", fileName);
  jstring js_fileName = jenv->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    return HBC_ERROR_DROP_PARAM;
  }

  // boolean drop(java.lang.String);
  jboolean jresult = jenv->CallBooleanMethod(javaObj_, JavaMethods_[JM_DROP].methodID, js_fileName);

  jenv->DeleteLocalRef(js_fileName);  

  if (jenv->ExceptionCheck())
  {
    getExceptionDetails(jenv);
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::drop()", getLastError(jenv));
    return HBC_ERROR_DROP_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::drop()", getLastError(jenv));
    return HBC_ERROR_DROP_EXCEPTION;
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::dropAll(const char* pattern, bool async)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::dropAll(%s) called.", pattern);

  if (async) {
    // not supported yet.
    return HBC_ERROR_DROP_EXCEPTION;
  }

  jstring js_pattern = jenv_->NewStringUTF(pattern);
  if (js_pattern == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    return HBC_ERROR_DROP_PARAM;
  }

  // boolean drop(java.lang.String);
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_DROP_ALL].methodID, js_pattern);

  jenv_->DeleteLocalRef(js_pattern);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(jenv_);
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::dropAll()", getLastError(jenv_));
    return HBC_ERROR_DROP_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::dropAll()", getLastError(jenv_));
    return HBC_ERROR_DROP_EXCEPTION;
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::copy(const char* currTblName, const char* oldTblName)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::copy(%s,%s) called.", currTblName, oldTblName);

  jstring js_currTblName = jenv_->NewStringUTF(currTblName);
  if (js_currTblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    return HBC_ERROR_DROP_PARAM;
  }

  jstring js_oldTblName = jenv_->NewStringUTF(oldTblName);
  if (js_oldTblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    return HBC_ERROR_DROP_PARAM;
  }

  // boolean drop(java.lang.String);
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_COPY].methodID, js_currTblName, js_oldTblName);

  jenv_->DeleteLocalRef(js_currTblName);  

  jenv_->DeleteLocalRef(js_oldTblName);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(jenv_);
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::copy()", getLastError());
    return HBC_ERROR_DROP_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::copy()", getLastError());
    return HBC_ERROR_DROP_EXCEPTION;
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::exists(const char* fileName)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::exists(%s) called.", fileName);
  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_EXISTS_PARAM));
    return HBC_ERROR_EXISTS_PARAM;
  }

  // boolean exists(java.lang.String);
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_EXISTS].methodID, js_fileName);

  jenv_->DeleteLocalRef(js_fileName);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::exists()", getLastError());
    return HBC_ERROR_EXISTS_EXCEPTION;
  }

  if (jresult == false) 
     return HBC_DONE;  // Table does not exist

  return HBC_OK;  // Table exists.
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::grant(const Text& user, const Text& tblName, const TextVec& actions)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::grant(%s, %s, %s) called.", user.data(), tblName.data(), actions.data());

  int len = user.size();
  jbyteArray jba_user = jenv_->NewByteArray(len);
  if (jba_user == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GRANT_PARAM));
    return HBC_ERROR_GRANT_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_user, 0, len, (const jbyte*)user.data());

  len = tblName.size();
  jbyteArray jba_tblName = jenv_->NewByteArray(len);
  if (jba_tblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GRANT_PARAM));
    return HBC_ERROR_GRANT_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_tblName, 0, len, (const jbyte*)tblName.data());

  jobjectArray j_actionCodes = NULL;
  if (!actions.empty())
  {
    HdfsLogger::log(CAT_HBASE, LL_DEBUG, "  Adding %d actions.", actions.size());
    j_actionCodes = convertToStringObjectArray(actions);
    if (j_actionCodes == NULL)
    {
       getExceptionDetails();
       logError(CAT_HBASE, __FILE__, __LINE__);
       logError(CAT_HBASE, "HBaseClient_JNI::grant()", getLastError());
       return HBC_ERROR_GRANT_PARAM;
    }
  }
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_GRANT].methodID, jba_user, jba_tblName, j_actionCodes);

  jenv_->DeleteLocalRef(jba_user);  
  jenv_->DeleteLocalRef(jba_tblName);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::grant()", getLastError());
    return HBC_ERROR_GRANT_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::grant()", getLastError());
    return HBC_ERROR_GRANT_EXCEPTION;
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
JavaMethodInit* HBulkLoadClient_JNI::JavaMethods_ = NULL;
jclass HBulkLoadClient_JNI::javaClass_ = 0;
bool HBulkLoadClient_JNI::javaMethodsInitialized_ = false;
pthread_mutex_t HBulkLoadClient_JNI::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;


static const char* const hblcErrorEnumStr[] = ///need to update content
{
    "preparing parameters for init."
   ,"java exception in init."
   ,"java exception in cleanup."
   ,"java exception in close"
   ,"java exception in create_hfile()."
   ,"java exception in create_hfile()."
   ,"preparing parameters for add_to_hfile()."
   ,"java exception in add_to_hfile()."
   ,"preparing parameters for hblc_error_close_hfile()."
   ,"java exception in close_hfile()."
   ,"java exception in do_bulkload()."
   ,"java exception in do_bulkload()."
   ,"preparing parameters for bulkload_cleanup()."
   ,"java exception in bulkload_cleanup()."
   ,"preparing parameters for init_hblc()."
   ,"java exception in init_hblc()."
};
HBLC_RetCode HBulkLoadClient_JNI::init()
{
  static char className[]="org/trafodion/sql/HBaseAccess/HBulkLoadClient";
  HBLC_RetCode rc;

  if (isInitialized())
    return HBLC_OK;

  if (javaMethodsInitialized_)
    return (HBLC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HBLC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];

    JavaMethods_[JM_CTOR       ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR       ].jm_signature = "()V";
    JavaMethods_[JM_GET_ERROR  ].jm_name      = "getLastError";
    JavaMethods_[JM_GET_ERROR  ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_INIT_HFILE_PARAMS     ].jm_name      = "initHFileParams";
    JavaMethods_[JM_INIT_HFILE_PARAMS     ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;)Z";
    JavaMethods_[JM_CLOSE_HFILE      ].jm_name      = "closeHFile";
    JavaMethods_[JM_CLOSE_HFILE      ].jm_signature = "()Z";
    JavaMethods_[JM_DO_BULK_LOAD     ].jm_name      = "doBulkLoad";
    JavaMethods_[JM_DO_BULK_LOAD     ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;ZZ)Z";
    JavaMethods_[JM_BULK_LOAD_CLEANUP].jm_name      = "bulkLoadCleanup";
    JavaMethods_[JM_BULK_LOAD_CLEANUP].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_ADD_TO_HFILE_DB  ].jm_name      = "addToHFile";
    JavaMethods_[JM_ADD_TO_HFILE_DB  ].jm_signature = "(SLjava/lang/Object;Ljava/lang/Object;)Z";

    rc = (HBLC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

char* HBulkLoadClient_JNI::getErrorText(HBLC_RetCode errEnum)
{
  if (errEnum < (HBLC_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else
    return (char*)hblcErrorEnumStr[errEnum-HBLC_FIRST-1];
}

HBLC_RetCode HBulkLoadClient_JNI::initHFileParams(
                        const HbaseStr &tblName,
                        const Text& hFileLoc,
                        const Text& hfileName,
                        Int64 maxHFileSize)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::initHFileParams(%s, %s, %s, %ld) called.", hFileLoc.data(), hfileName.data(), tblName.val,maxHFileSize);

  jstring js_hFileLoc = jenv_->NewStringUTF(hFileLoc.c_str());
   if (js_hFileLoc == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_CREATE_HFILE_PARAM));
     return HBLC_ERROR_CREATE_HFILE_PARAM;
   }
  jstring js_hfileName = jenv_->NewStringUTF(hfileName.c_str());
   if (js_hfileName == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_CREATE_HFILE_PARAM));
     return HBLC_ERROR_CREATE_HFILE_PARAM;
   }
   jstring js_tabName = jenv_->NewStringUTF(tblName.val);
    if (js_tabName == NULL)
    {
      GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_CREATE_HFILE_PARAM));
      return HBLC_ERROR_CREATE_HFILE_PARAM;
    }
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBulkLoadClient_JNI::initHFileParams() => before calling Java.", getLastError());
    return HBLC_ERROR_CREATE_HFILE_EXCEPTION;
  }

  jlong j_maxSize = maxHFileSize;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_INIT_HFILE_PARAMS].methodID, js_hFileLoc, js_hfileName,j_maxSize,js_tabName);

  jenv_->DeleteLocalRef(js_hFileLoc);
  jenv_->DeleteLocalRef(js_hfileName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBulkLoadClient_JNI::initHFileParams()", getLastError());
    return HBLC_ERROR_CREATE_HFILE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_HBASE, "HBulkLoadClient_JNI::initHFileParams()", getLastError());
    return HBLC_ERROR_CREATE_HFILE_EXCEPTION;
  }

  return HBLC_OK;
}

HBLC_RetCode HBulkLoadClient_JNI::addToHFile( short rowIDLen, HbaseStr &rowIDs,
            HbaseStr &rows)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::addToHFile called.");

  jobject jRowIDs = jenv_->NewDirectByteBuffer(rowIDs.val, rowIDs.len);
  if (jRowIDs == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_ADD_TO_HFILE_EXCEPTION));
    return HBLC_ERROR_ADD_TO_HFILE_EXCEPTION;
  }

  jobject jRows = jenv_->NewDirectByteBuffer(rows.val, rows.len);
  if (jRows == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_ADD_TO_HFILE_EXCEPTION));
    return HBLC_ERROR_ADD_TO_HFILE_EXCEPTION;
  }

  jshort j_rowIDLen = rowIDLen;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
            JavaMethods_[JM_ADD_TO_HFILE_DB].methodID, 
            j_rowIDLen, jRowIDs, jRows);
  jenv_->DeleteLocalRef(jRowIDs);
  jenv_->DeleteLocalRef(jRows);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBulkLoadClient_JNI::addToHFile()", getLastError());
    return HBLC_ERROR_ADD_TO_HFILE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_HBASE, "HBulkLoadClient_JNI::addToHFile()", getLastError());
    return HBLC_ERROR_ADD_TO_HFILE_EXCEPTION;
  }

  return HBLC_OK;
}

HBLC_RetCode HBulkLoadClient_JNI::closeHFile(
                        const HbaseStr &tblName)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::closeHFile(%s) called.", tblName.val);

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_CLOSE_HFILE].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBulkLoadClient_JNI::closeHFile()", getLastError());
    return HBLC_ERROR_CLOSE_HFILE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_HBASE, "HBulkLoadClient_JNI::closeHFile()", getLastError());
    return HBLC_ERROR_CLOSE_HFILE_EXCEPTION;
  }

  return HBLC_OK;
}


HBLC_RetCode HBulkLoadClient_JNI::doBulkLoad(
                             const HbaseStr &tblName,
                             const Text& prepLocation,
                             const Text& tableName,
                             NABoolean quasiSecure,
                             NABoolean snapshot)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::doBulkLoad(%s, %s, %s) called.", tblName.val, prepLocation.data(), tableName.data());

  jstring js_PrepLocation = jenv_->NewStringUTF(prepLocation.c_str());
   if (js_PrepLocation == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_DO_BULKLOAD_PARAM));
     return HBLC_ERROR_DO_BULKLOAD_PARAM;
   }
  jstring js_TableName = jenv_->NewStringUTF(tableName.c_str());
   if (js_TableName == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_DO_BULKLOAD_PARAM));
     return HBLC_ERROR_DO_BULKLOAD_PARAM;
   }

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBulkLoadClient_JNI::doBulkLoad() => before calling Java.", getLastError());
    return HBLC_ERROR_DO_BULKLOAD_EXCEPTION;
  }

  jboolean j_quasiSecure = quasiSecure;

  jboolean j_snapshot = snapshot;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_DO_BULK_LOAD].methodID, js_PrepLocation, js_TableName, j_quasiSecure, j_snapshot);

  jenv_->DeleteLocalRef(js_PrepLocation);
  jenv_->DeleteLocalRef(js_TableName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBulkLoadClient_JNI::doBulkLoad()", getLastError());
    return HBLC_ERROR_DO_BULKLOAD_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_HBASE, "HBaseClient_JNI::doBulkLoad()", getLastError());
    return HBLC_ERROR_DO_BULKLOAD_EXCEPTION;
  }

  return HBLC_OK;
}

HBLC_RetCode HBulkLoadClient_JNI::bulkLoadCleanup(
                             const HbaseStr &tblName,
                             const Text& location)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::bulkLoadCleanup(%s, %s) called.", tblName.val, location.data());

  jstring js_location = jenv_->NewStringUTF(location.c_str());
   if (js_location == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_BULKLOAD_CLEANUP_PARAM));
     return HBLC_ERROR_BULKLOAD_CLEANUP_PARAM;
   }


  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBulkLoadClient_JNI::bulkLoadCleanup() => before calling Java.", getLastError());
    return HBLC_ERROR_BULKLOAD_CLEANUP_PARAM;
  }
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_BULK_LOAD_CLEANUP].methodID, js_location);

  jenv_->DeleteLocalRef(js_location);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBulkLoadClient_JNI::bulkLoadCleanup()", getLastError());
    return HBLC_ERROR_BULKLOAD_CLEANUP_PARAM;
  }

  if (jresult == false)
  {
    logError(CAT_HBASE, "HBulkLoadClient_JNI::bulkLoadCleanup()", getLastError());
    return HBLC_ERROR_BULKLOAD_CLEANUP_PARAM;
  }

  return HBLC_OK;
}

HBulkLoadClient_JNI::~HBulkLoadClient_JNI()
{
  //HdfsLogger::log(CAT_JNI_TOP, LL_DEBUG, "HBulkLoadClient_JNI destructor called.");
}

jstring HBulkLoadClient_JNI::getLastJavaError()
{
  if (javaObj_ == NULL)
    return NULL;

  // java.lang.String getLastError();
  return (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_ERROR].methodID);
}




////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::revoke(const Text& user, const Text& tblName, const TextVec& actions)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::revoke(%s, %s, %s) called.", user.data(), tblName.data(), actions.data());

  int len = user.size();
  jbyteArray jba_user = jenv_->NewByteArray(len);
  if (jba_user == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_REVOKE_PARAM));
    return HBC_ERROR_REVOKE_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_user, 0, len, (const jbyte*)user.data());

  len = tblName.size();
  jbyteArray jba_tblName = jenv_->NewByteArray(len);
  if (jba_tblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_REVOKE_PARAM));
    return HBC_ERROR_REVOKE_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_tblName, 0, len, (const jbyte*)tblName.data());

  jobjectArray j_actionCodes = NULL;
  if (!actions.empty())
  {
    HdfsLogger::log(CAT_HBASE, LL_DEBUG, "  Adding %d actions.", actions.size());
    j_actionCodes = convertToStringObjectArray(actions);
    if (j_actionCodes == NULL)
    {
       getExceptionDetails();
       logError(CAT_HBASE, __FILE__, __LINE__);
       logError(CAT_HBASE, "HBaseClient_JNI::revoke()", getLastError());
       return HBC_ERROR_REVOKE_PARAM;
    }
  }
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_REVOKE].methodID, jba_user, jba_tblName, j_actionCodes);

  jenv_->DeleteLocalRef(jba_user);  
  jenv_->DeleteLocalRef(jba_tblName);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::revoke()", getLastError());
    return HBC_ERROR_REVOKE_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::revoke()", getLastError());
    return HBC_ERROR_REVOKE_EXCEPTION;
  }

  return HBC_OK;
}


////////////////////////////////////////////////////////////////////
void HBaseClient_JNI::logIt(const char* str)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, str);
}


// ===========================================================================
// ===== Class HTableClient
// ===========================================================================

JavaMethodInit* HTableClient_JNI::JavaMethods_ = NULL;
jclass HTableClient_JNI::javaClass_ = 0;
bool HTableClient_JNI::javaMethodsInitialized_ = false;
pthread_mutex_t HTableClient_JNI::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const htcErrorEnumStr[] = 
{
  "Preparing parameters for initConnection()."
 ,"Java exception in initConnection()."
 ,"Java exception in setTransactionID()."
 ,"Java exception in cleanup()."
 ,"Java exception in close()."
 ,"Preparing parameters for scanOpen()."
 ,"Java exception in scanOpen()."
 ,"Java exception in scanFetch()."
 ,"Java exception in fetchNextRow()."
 ,"Java exception in scanClose()."
 ,"Preparing parameters for getRowOpen()."
 ,"Java exception in getRowOpen()."
 ,"Preparing parameters for getRowsOpen()."
 ,"Java exception in getRowsOpen()."
 ,"Java exception in getFetch()."
 ,"Java exception in getClose()."
 ,"Preparing parameters for deleteRow()."
 ,"Java exception in deleteRow()."
 ,"Preparing parameters for deleteRows()."
 ,"Java exception in deleteRows()."
 ,"Preparing parameters for checkAndDeleteRow()."
 ,"Java exception in checkAndDeleteRow()."
 ,"Row deleted in checkAndDeleteRow()."
 ,"Preparing parameters for insertRow()."
 ,"Java exception in insertRow()."
 ,"Preparing parameters for insertRows()."
 ,"Java exception in insertRows()."
 ,"Preparing parameters for checkAndInsertRow()."
 ,"Java exception in checkAndInsertRow()."
 ,"Dup RowId in checkAndInsertRow()."
 ,"Transactions not supported yet in checkAndInsertRow()"
 ,"Preparing parameters for checkAndUpdateRow()."
 ,"Java exception in checkAndUpdateRow()."
 ,"Dup RowId in checkAndUpdateRow()."
 ,"Transactions not supported yet in checkAndUpdateRow()"
 ,"Preparing parameters for create()."
 ,"Java exception in create()."
 ,"Preparing parameters for drop()."
 ,"Java exception in drop()."
 ,"Preparing parameters for exists()."
 ,"Java exception in exists()."
 ,"Preparing parameters for coProcAggr()."
 ,"Java exception in coProcAggr()."
 ,"Preparing parameters for grant()."
 ,"Java exception in grant()."
 ,"Preparing parameters for revoke()."
 ,"Java exception in revoke()."
 ,"Java exception in getendkeys()."
 ,"Java exception in getHTableName()."
 ,"Java exception in flush()."
 ,"Java exception in getColName()."
 ,"Java exception in getColValue()."
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* HTableClient_JNI::getErrorText(HTC_RetCode errEnum)
{
  if (errEnum < (HTC_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)htcErrorEnumStr[errEnum-HTC_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTableClient_JNI::~HTableClient_JNI()
{
  //HdfsLogger::log(CAT_JNI_TOP, LL_DEBUG, "HTableClient destructor called.");
  if (tableName_ != NULL)
  {
     NADELETEBASIC(tableName_, heap_);
  }
  if (colNameAllocLen_ != 0)
     NADELETEBASIC(colName_, heap_);
  if (numCellsAllocated_ > 0)
  {
      NADELETEBASIC(p_kvValLen_, heap_);
      NADELETEBASIC(p_kvValOffset_, heap_);
      NADELETEBASIC(p_kvFamLen_, heap_);
      NADELETEBASIC(p_kvFamOffset_, heap_);
      NADELETEBASIC(p_kvQualLen_, heap_);
      NADELETEBASIC(p_kvQualOffset_, heap_);
      NADELETEBASIC(p_timestamp_, heap_);
      numCellsAllocated_ = 0;
  }
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::init()
{
  static char className[]="org/trafodion/sql/HBaseAccess/HTableClient";
  HTC_RetCode rc;
  
  if (isInitialized())
    return HTC_OK;
  
  if (javaMethodsInitialized_)
    return (HTC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HTC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR       ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR       ].jm_signature = "()V";
    JavaMethods_[JM_GET_ERROR  ].jm_name      = "getLastError";
    JavaMethods_[JM_GET_ERROR  ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_SCAN_OPEN  ].jm_name      = "startScan";
    JavaMethods_[JM_SCAN_OPEN  ].jm_signature = "(J[B[B[Ljava/lang/Object;JZI[Ljava/lang/Object;[Ljava/lang/Object;[Ljava/lang/Object;FZ)Z";
    JavaMethods_[JM_GET_OPEN   ].jm_name      = "startGet";
    JavaMethods_[JM_GET_OPEN   ].jm_signature = "(J[B[Ljava/lang/Object;JZ)Z";
    JavaMethods_[JM_GETS_OPEN  ].jm_name      = "startGet";
    JavaMethods_[JM_GETS_OPEN  ].jm_signature = "(J[Ljava/lang/Object;[Ljava/lang/Object;JZ)Z";
    JavaMethods_[JM_SCAN_FETCH ].jm_name      = "scanFetch";
    JavaMethods_[JM_SCAN_FETCH ].jm_signature = "()Z";
    JavaMethods_[JM_GET_FETCH  ].jm_name      = "getFetch";
    JavaMethods_[JM_GET_FETCH  ].jm_signature = "()Z";
    JavaMethods_[JM_FETCH_ROW  ].jm_name      = "fetchNextRow";
    JavaMethods_[JM_FETCH_ROW  ].jm_signature = "()Z";
    JavaMethods_[JM_GET_CELL   ].jm_name      = "getLastFetchedCell";
    JavaMethods_[JM_GET_CELL   ].jm_signature = "()Lorg/apache/hadoop/hbase/KeyValue;";
    JavaMethods_[JM_DELETE     ].jm_name      = "deleteRow";
    JavaMethods_[JM_DELETE     ].jm_signature = "(J[B[Ljava/lang/Object;J)Z";
    JavaMethods_[JM_CHECKANDDELETE     ].jm_name      = "checkAndDeleteRow";
    JavaMethods_[JM_CHECKANDDELETE     ].jm_signature = "(J[B[B[BJ)Z";
    JavaMethods_[JM_CHECKANDUPDATE     ].jm_name      = "checkAndUpdateRow";
    JavaMethods_[JM_CHECKANDUPDATE     ].jm_signature = "(J[BLjava/lang/Object;[B[BJ)Z";
    JavaMethods_[JM_COPROC_AGGR     ].jm_name      = "coProcAggr";
    JavaMethods_[JM_COPROC_AGGR     ].jm_signature = "(JI[B[B[B[BZI)Z";
    JavaMethods_[JM_COPROC_AGGR_GET_RESULT     ].jm_name      = "coProcAggrGetResult";
    JavaMethods_[JM_COPROC_AGGR_GET_RESULT     ].jm_signature = "()Lorg/trafodion/sql/HBaseAccess/ByteArrayList;";
    JavaMethods_[JM_GET_NAME   ].jm_name      = "getTableName";
    JavaMethods_[JM_GET_NAME   ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_GET_HTNAME ].jm_name      = "getTableName";
    JavaMethods_[JM_GET_HTNAME ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_GETENDKEYS ].jm_name      = "getEndKeys";
    JavaMethods_[JM_GETENDKEYS ].jm_signature = "()Lorg/trafodion/sql/HBaseAccess/ByteArrayList;";
    JavaMethods_[JM_FLUSHT     ].jm_name      = "flush";
    JavaMethods_[JM_FLUSHT     ].jm_signature = "()Z";
    JavaMethods_[JM_SET_WB_SIZE ].jm_name      = "setWriteBufferSize";
    JavaMethods_[JM_SET_WB_SIZE ].jm_signature = "(J)Z";
    JavaMethods_[JM_SET_WRITE_TO_WAL ].jm_name      = "setWriteToWAL";
    JavaMethods_[JM_SET_WRITE_TO_WAL ].jm_signature = "(Z)Z";
    JavaMethods_[JM_DIRECT_INSERT ].jm_name      = "insertRow";
    JavaMethods_[JM_DIRECT_INSERT ].jm_signature = "(J[BLjava/lang/Object;J)Z";
    JavaMethods_[JM_DIRECT_CHECKANDINSERT     ].jm_name      = "checkAndInsertRow";
    JavaMethods_[JM_DIRECT_CHECKANDINSERT     ].jm_signature = "(J[BLjava/lang/Object;J)Z";
    JavaMethods_[JM_DIRECT_INSERT_ROWS ].jm_name      = "putRows";
    JavaMethods_[JM_DIRECT_INSERT_ROWS ].jm_signature = "(JSLjava/lang/Object;Ljava/lang/Object;JZ)Z";
    JavaMethods_[JM_DIRECT_DELETE_ROWS ].jm_name      = "deleteRows";
    JavaMethods_[JM_DIRECT_DELETE_ROWS ].jm_signature = "(JSLjava/lang/Object;J)Z";
    JavaMethods_[JM_FETCH_ROWS ].jm_name      = "fetchRows";
    JavaMethods_[JM_FETCH_ROWS ].jm_signature = "(J)I";
   
    rc = (HTC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
jstring HTableClient_JNI::getLastJavaError()
{
  if (javaObj_ == NULL)
    return NULL;

  // java.lang.String getLastError();
  return (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_ERROR].methodID);
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
ByteArrayList* HTableClient_JNI::newByteArrayList(jobject jObj)
{
  ByteArrayList* bal = new (heap_) ByteArrayList(heap_, jObj);
  if ( bal->init() != BAL_OK)
  {
    HdfsLogger::log(CAT_HBASE, LL_ERROR, "ByteArrayList::init() error.");
    return NULL;
  }
  
  return bal;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
ByteArrayList* HTableClient_JNI::newByteArrayList(const TextVec& vec)
{
  ByteArrayList* bal = new (heap_) ByteArrayList(heap_);
  if ( bal->init() != BAL_OK)
  {
    HdfsLogger::log(CAT_HBASE, LL_ERROR, "ByteArrayList::init() error.");
    return NULL;
  }
  
  if (bal->add(vec) != BAL_OK)
  {
    HdfsLogger::log(CAT_HBASE, LL_ERROR, "ByteArrayList::add() error.");
    return NULL;
  }
  
  return bal;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::startScan(Int64 transID, const Text& startRowID, 
   const Text& stopRowID, const TextVec& cols, Int64 timestamp, 
   bool cacheBlocks, Lng32 numCacheRows, NABoolean preFetch,
					const TextVec *inColNamesToFilter, 
					const TextVec *inCompareOpList,
					const TextVec *inColValuesToCompare,
					Float32 samplePercent)

{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::startScan() called.");
  int len = startRowID.size();
  jbyteArray jba_startRowID = jenv_->NewByteArray(len);
  if (jba_startRowID == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_SCANOPEN_PARAM));
    return HTC_ERROR_SCANOPEN_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_startRowID, 0, len, (const jbyte*)startRowID.data());

  len = stopRowID.size();
  jbyteArray jba_stopRowID = jenv_->NewByteArray(len);
  if (jba_stopRowID == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_SCANOPEN_PARAM));
    return HTC_ERROR_SCANOPEN_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_stopRowID, 0, len, (const jbyte*)stopRowID.data());

  jobjectArray j_cols = NULL;
  if (!cols.empty())
  {
    j_cols = convertToByteArrayObjectArray(cols);
    if (j_cols == NULL)
    {
       getExceptionDetails();
       logError(CAT_HBASE, __FILE__, __LINE__);
       logError(CAT_HBASE, "HTableClient_JNI::startScan()", getLastError());
       return HTC_ERROR_SCANOPEN_PARAM;
    }
    numColsInScan_ = cols.size();
  }
  else
     numColsInScan_ = 0;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;

  jboolean j_cb = cacheBlocks;
  jboolean j_preFetch = preFetch;
  jint j_ncr = numCacheRows;
  numReqRows_ = numCacheRows;
  currentRowNum_ = -1;
  
  jobjectArray j_colnamestofilter = NULL;
  if ((inColNamesToFilter) && (!inColNamesToFilter->empty()))
  {
    j_colnamestofilter = convertToByteArrayObjectArray(*inColNamesToFilter);
    if (j_colnamestofilter == NULL)
    {
       getExceptionDetails();
       logError(CAT_HBASE, __FILE__, __LINE__);
       logError(CAT_HBASE, "HTableClient_JNI::startScan()", getLastError());
       return HTC_ERROR_SCANOPEN_PARAM;
    }
  }

  jobjectArray j_compareoplist = NULL;
  if ((inCompareOpList) && (! inCompareOpList->empty()))
  {
     j_compareoplist = convertToByteArrayObjectArray(*inCompareOpList);
     if (j_compareoplist == NULL)
     {
        getExceptionDetails();
        logError(CAT_HBASE, __FILE__, __LINE__);
        logError(CAT_HBASE, "HTableClient_JNI::startScan()", getLastError());
        return HTC_ERROR_SCANOPEN_PARAM;
     }
  }

  jobjectArray j_colvaluestocompare = NULL;
  if ((inColValuesToCompare) && (!inColValuesToCompare->empty()))
  {
     j_colvaluestocompare = convertToByteArrayObjectArray(*inColValuesToCompare);
     if (j_colvaluestocompare == NULL)
     {
        getExceptionDetails();
        logError(CAT_HBASE, __FILE__, __LINE__);
        logError(CAT_HBASE, "HTableClient_JNI::startScan()", getLastError());
        return HTC_ERROR_SCANOPEN_PARAM;
     }
  }

  jfloat j_smplPct = samplePercent;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
            JavaMethods_[JM_SCAN_OPEN].methodID, 
            j_tid, jba_startRowID, jba_stopRowID, j_cols, j_ts, j_cb, j_ncr,
            j_colnamestofilter, j_compareoplist, j_colvaluestocompare, 
            j_smplPct, j_preFetch);

  jenv_->DeleteLocalRef(jba_startRowID);  
  jenv_->DeleteLocalRef(jba_stopRowID);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::scanOpen()", getLastError());
    return HTC_ERROR_SCANOPEN_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HTableClient_JNI::scanOpen()", getLastError());
    return HTC_ERROR_SCANOPEN_EXCEPTION;
  }
  fetchMode_ = SCAN_FETCH;
  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::startGet(Int64 transID, const Text& rowID, const TextVec& cols, 
	Int64 timestamp, NABoolean directRow)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::startGet(%s) called.", rowID.data());
  int len = rowID.size();
  jbyteArray jba_rowID = jenv_->NewByteArray(len);
  if (jba_rowID == NULL) 
  {
     GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_GETROWOPEN_PARAM));
     return HTC_ERROR_GETROWOPEN_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, len, (const jbyte*)rowID.data());
  jobjectArray j_cols = NULL;
  if (!cols.empty())
  {
     j_cols = convertToByteArrayObjectArray(cols);
     if (j_cols == NULL)
     {
        getExceptionDetails();
        logError(CAT_HBASE, __FILE__, __LINE__);
        logError(CAT_HBASE, "HTableClient_JNI::startGet()", getLastError());
        return HTC_ERROR_GETROWOPEN_PARAM;
     }  
     numColsInScan_ = cols.size();
  }
  else
     numColsInScan_ = 0;
  numReqRows_ = 1;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jboolean j_directRow = directRow;
  
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_GET_OPEN].methodID, j_tid, jba_rowID, j_cols, j_ts, j_directRow);

  jenv_->DeleteLocalRef(jba_rowID);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::getRowOpen()", getLastError());
    return HTC_ERROR_GETROWOPEN_EXCEPTION;
  }

  fetchMode_ = GET_ROW;
  if (jresult == false) 
     numRowsReturned_ = -1;
  else
     numRowsReturned_ = 0;
  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::startGets(Int64 transID, const TextVec& rowIDs, 
	const TextVec& cols, Int64 timestamp, NABoolean directRow)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::startGet(multi-row) called.");
  jobjectArray j_cols = NULL;
  if (!cols.empty())
  {
     j_cols = convertToByteArrayObjectArray(cols);
     if (j_cols == NULL)
     {
        getExceptionDetails();
        logError(CAT_HBASE, __FILE__, __LINE__);
        logError(CAT_HBASE, "HTableClient_JNI::startGets()", getLastError());
        return HTC_ERROR_GETROWSOPEN_PARAM;
     }
     numColsInScan_ = cols.size();
  }  
  else
     numColsInScan_ = 0;
  jobjectArray j_rows = convertToByteArrayObjectArray(rowIDs);
  if (j_rows == NULL)
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::startGets()", getLastError());
    return HTC_ERROR_GETROWSOPEN_PARAM;
  }  
  numReqRows_ = rowIDs.size();
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jboolean j_directRow = directRow;
  
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_GETS_OPEN].methodID, j_tid, j_rows, j_cols, j_ts, j_directRow);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::getRowsOpen()", getLastError());
    return HTC_ERROR_GETROWSOPEN_EXCEPTION;
  }

  if (jresult == false) 
  {

    logError(CAT_HBASE, "HTableClient_JNI::getRowsOpen()", getLastError());
    return HTC_ERROR_GETROWSOPEN_EXCEPTION;
  }
  fetchMode_ = BATCH_GET;
  return HTC_OK;
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::scanFetch()
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::scanFetch() called.");

  // public boolean scanFetch();
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_SCAN_FETCH].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::scanFetch()", getLastError());
    return HTC_ERROR_SCANFETCH_EXCEPTION;
  }

  if (jresult == false) 
  {
    jstring jLastError = getLastJavaError();
    if (jLastError != NULL)
    {
      logError(CAT_HBASE, "HTableClient_JNI::scanFetch()", jLastError);
      const char *errStr = jenv_->GetStringUTFChars(jLastError, 0);
      GetCliGlobals()->setJniErrorStr(errStr);
      jenv_->ReleaseStringUTFChars(jLastError, errStr);
      jenv_->DeleteLocalRef(jLastError);
      return HTC_ERROR_SCANFETCH_EXCEPTION;
    }
    else
      // Done reading all the data.
      return HTC_DONE;
  }
  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::getFetch()
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::getFetch() called.");

  // public boolean getFetch();
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_GET_FETCH].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::getFetch()", getLastError());
    return HTC_ERROR_GETFETCH_EXCEPTION;
  }

  if (jresult == false) 
  {
    jstring jLastError = getLastJavaError();
    if (jLastError != NULL)
    {
      logError(CAT_HBASE, "HTableClient_JNI::getFetch()", jLastError);
      const char *errStr = jenv_->GetStringUTFChars(jLastError, 0);
      GetCliGlobals()->setJniErrorStr(errStr);
      jenv_->ReleaseStringUTFChars(jLastError, errStr);
      jenv_->DeleteLocalRef(jLastError);
      return HTC_ERROR_GETFETCH_EXCEPTION;
    }
    else
     // Done reading all the data.
       return HTC_DONE;
  }

  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::fetchNextRow()
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::fetchNextRow() called.");

  // public boolean fetchNextRow();
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_FETCH_ROW].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::fetchNextRow()", getLastError());
    return HTC_ERROR_FETCHNEXTROW_EXCEPTION;
  }

  if (jresult == false) 
  {
    jstring jLastError = getLastJavaError();
    if (jLastError != NULL)
    {
      logError(CAT_HBASE, "HTableClient_JNI::fetchNextRow()", jLastError);
      const char *errStr = jenv_->GetStringUTFChars(jLastError, 0);
      GetCliGlobals()->setJniErrorStr(errStr);
      jenv_->ReleaseStringUTFChars(jLastError, errStr);
      jenv_->DeleteLocalRef(jLastError);
      return HTC_ERROR_FETCHNEXTROW_EXCEPTION;
    }
    else
      return HTC_DONE;
  }
  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
KeyValue* HTableClient_JNI::getLastFetchedCell()
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::getLastFetchedCell() called.");

  // public org.apache.hadoop.hbase.KeyValue getLastFetchedCell();
  jobject jKeyValue = jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_CELL].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::getLastFetchedCell()", getLastError());
    return NULL;
  }

  if (jKeyValue == NULL)
    return NULL;
    
  KeyValue* kv = new (heap_) KeyValue(heap_, jKeyValue);
  if (kv->init() != KYV_OK)
     return NULL;
;
  return kv;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::deleteRow(Int64 transID, HbaseStr &rowID, const TextVec& cols, Int64 timestamp)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::deleteRow(%ld, %s) called.", transID, rowID.val);
  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) 
  {
     GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_DELETEROW_PARAM));
     return HTC_ERROR_DELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);
  jobjectArray j_cols = NULL;
  if (!cols.empty())
  {
     j_cols = convertToByteArrayObjectArray(cols);
     if (j_cols == NULL)
     {
        getExceptionDetails();
        logError(CAT_HBASE, __FILE__, __LINE__);
        logError(CAT_HBASE, "HTableClient_JNI::deleteRow()", getLastError());
        return HTC_ERROR_DELETEROW_PARAM;
     }
  }  
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
          JavaMethods_[JM_DELETE].methodID, j_tid, jba_rowID, j_cols, j_ts);

  jenv_->DeleteLocalRef(jba_rowID);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::deleteRow()", getLastError());
    return HTC_ERROR_DELETEROW_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HTableClient_JNI::deleteRow()", getLastError());
    return HTC_ERROR_DELETEROW_EXCEPTION;
  }

  return HTC_OK;
}
//
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::deleteRows(Int64 transID, short rowIDLen, HbaseStr &rowIDs, Int64 timestamp)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::deleteRows() called.");

  jobject jRowIDs = jenv_->NewDirectByteBuffer(rowIDs.val, rowIDs.len);
  if (jRowIDs == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_DELETEROWS_PARAM));
    return HTC_ERROR_DELETEROWS_PARAM;
  }
  jshort j_rowIDLen = rowIDLen;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_DIRECT_DELETE_ROWS].methodID, j_tid, j_rowIDLen, jRowIDs, j_ts);

  jenv_->DeleteLocalRef(jRowIDs);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::deleteRows()", getLastError());
    return HTC_ERROR_DELETEROWS_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HTableClient_JNI::deleteRows()", getLastError());
    return HTC_ERROR_DELETEROWS_EXCEPTION;
  }

  return HTC_OK;
}


//////////////////////////////////////////////////////////////////////////////
// 


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::checkAndDeleteRow(Int64 transID, HbaseStr &rowID,
	    const Text &columnToCheck, const Text &colValToCheck,
	    Int64 timestamp)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::checkAndDeleteRow(%s, %s, %s) called.", rowID.val, columnToCheck.data(), colValToCheck.data());
  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDDELETEROW_PARAM));
    return HTC_ERROR_CHECKANDDELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);

  int len = columnToCheck.size();
  jbyteArray jba_columntocheck = jenv_->NewByteArray(len);
  if (jba_columntocheck == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDDELETEROW_PARAM));
    return HTC_ERROR_CHECKANDDELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_columntocheck, 0, len, (const jbyte*)columnToCheck.data());

  len = colValToCheck.size();
  jbyteArray jba_colvaltocheck = jenv_->NewByteArray(len);
  if (jba_colvaltocheck == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDDELETEROW_PARAM));
    return HTC_ERROR_CHECKANDDELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_colvaltocheck, 0, len, 
			    (const jbyte*)colValToCheck.data());
 
  jlong j_tid = transID;  
  jlong j_ts = timestamp;

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::checkAndDeleteRow() => before calling Java.", getLastError());
    return HTC_ERROR_CHECKANDDELETEROW_EXCEPTION;
  }

  // public boolean checkAndDeleteRow(long, byte[], byte[], long);
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_CHECKANDDELETE].methodID, j_tid, jba_rowID, jba_columntocheck, jba_colvaltocheck, j_ts);

  jenv_->DeleteLocalRef(jba_rowID);  
  jenv_->DeleteLocalRef(jba_columntocheck);  
  jenv_->DeleteLocalRef(jba_colvaltocheck);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::checkAndDeleteRow()", getLastError());
    return HTC_ERROR_CHECKANDDELETEROW_EXCEPTION;
  }

  if (jresult == false) 
  {
     GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDDELETE_ROW_NOTFOUND));
     return HTC_ERROR_CHECKANDDELETE_ROW_NOTFOUND;
  }
    
  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::insertRow(Int64 transID, HbaseStr &rowID, 
     HbaseStr &row, Int64 timestamp)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::insertRow(%s) direct called.", rowID.val);

  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_INSERTROW_PARAM));
    return HTC_ERROR_INSERTROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);

  jobject jDirectBuffer = jenv_->NewDirectByteBuffer(row.val, row.len);
  if (jDirectBuffer == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_INSERTROW_PARAM));
    return HTC_ERROR_INSERTROW_PARAM;
  }

  jlong j_tid = transID;  
  jlong j_ts = timestamp;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_DIRECT_INSERT].methodID, j_tid, jba_rowID, jDirectBuffer, j_ts);

  jenv_->DeleteLocalRef(jba_rowID);  
  jenv_->DeleteLocalRef(jDirectBuffer);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::insertRow()", getLastError());
    return HTC_ERROR_INSERTROW_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HTableClient_JNI::insertRow()", getLastError());
    return HTC_ERROR_INSERTROW_EXCEPTION;
  }

  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::insertRows(Int64 transID, short rowIDLen, HbaseStr &rowIDs, 
      HbaseStr &rows, Int64 timestamp, bool autoFlush)
{
  jobject jRowIDs = jenv_->NewDirectByteBuffer(rowIDs.val, rowIDs.len);
  if (jRowIDs == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_INSERTROWS_PARAM));
    return HTC_ERROR_INSERTROWS_PARAM;
  }
  
  jobject jRows = jenv_->NewDirectByteBuffer(rows.val, rows.len);
  if (jRows == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_INSERTROWS_PARAM));
    return HTC_ERROR_INSERTROWS_PARAM;
  }
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jshort j_rowIDLen = rowIDLen;
  jboolean j_af = autoFlush;
 
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_DIRECT_INSERT_ROWS].methodID, j_tid, j_rowIDLen, jRowIDs, jRows, j_ts, j_af);

  jenv_->DeleteLocalRef(jRowIDs);  
  jenv_->DeleteLocalRef(jRows);  

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::insertRows()", getLastError());
    return HTC_ERROR_INSERTROWS_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HTableClient_JNI::insertRows()", getLastError());
    return HTC_ERROR_INSERTROWS_EXCEPTION;
  }

  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::setWriteBufferSize(Int64 size)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::setWriteBufferSize() called.");



  jlong j_size = size;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_SET_WB_SIZE].methodID, j_size);


  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::setWriteBufferSize()", getLastError());
    return HTC_ERROR_INSERTROWS_EXCEPTION;// need to change exception
  }

  if (jresult == false)
  {
    logError(CAT_HBASE, "HTableClient_JNI::setWriteBufferSize()", getLastError());
    return HTC_ERROR_INSERTROWS_EXCEPTION;// need to change exception
  }

  return HTC_OK;
}

HTC_RetCode HTableClient_JNI::setWriteToWAL(bool WAL)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::setWriteToWAL() called.");

  jboolean j_WAL = WAL;

    jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_SET_WRITE_TO_WAL].methodID, j_WAL);


  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::setWriteToWAL()", getLastError());
    return HTC_ERROR_INSERTROWS_EXCEPTION;// need to change exception
  }

  if (jresult == false)
  {
    logError(CAT_HBASE, "HTableClient_JNI::setWriteToWAL()", getLastError());
    return HTC_ERROR_INSERTROWS_EXCEPTION;// need to change exception??????????????????
  }

  return HTC_OK;
}
//
//////////////////////////////////////////////////////////////////////////////
//   3-way return value!!!
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::checkAndInsertRow(Int64 transID, HbaseStr &rowID,
HbaseStr &row, Int64 timestamp)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::checkAndInsertRow(%s) called.", rowID.val);
  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDINSERTROW_PARAM));
    return HTC_ERROR_CHECKANDINSERTROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);

  jobject jDirectBuffer = jenv_->NewDirectByteBuffer(row.val, row.len);
  if (jDirectBuffer == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDINSERTROW_PARAM));
    return HTC_ERROR_CHECKANDINSERTROW_PARAM;
  }

  jlong j_tid = transID;  
  jlong j_ts = timestamp;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_DIRECT_CHECKANDINSERT].methodID, j_tid, jba_rowID, jDirectBuffer, j_ts);

  jenv_->DeleteLocalRef(jba_rowID);  
  jenv_->DeleteLocalRef(jDirectBuffer);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::checkAndInsertRow()", getLastError());
    return HTC_ERROR_CHECKANDINSERTROW_EXCEPTION;
  }

  if (jresult == false) 
     return HTC_ERROR_CHECKANDINSERT_DUP_ROWID;
  return HTC_OK;
}

HTC_RetCode HTableClient_JNI::checkAndUpdateRow(Int64 transID, HbaseStr &rowID,
            HbaseStr &row,
	    const Text &columnToCheck, const Text &colValToCheck, 
            Int64 timestamp)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::checkAndUpdateRow(%s) called.", rowID.val);
  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDUPDATEROW_PARAM));
    return HTC_ERROR_CHECKANDUPDATEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);

  jobject jDirectBuffer = jenv_->NewDirectByteBuffer(row.val, row.len);
  if (jDirectBuffer == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDUPDATEROW_PARAM));
    return HTC_ERROR_CHECKANDUPDATEROW_PARAM;
  }
  
  int len = columnToCheck.size();
  jbyteArray jba_columntocheck = jenv_->NewByteArray(len);
  if (jba_columntocheck == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDUPDATEROW_PARAM));
    return HTC_ERROR_CHECKANDUPDATEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_columntocheck, 0, len, 
			    (const jbyte*)columnToCheck.data());
 
  len = colValToCheck.size();
  jbyteArray jba_colvaltocheck = jenv_->NewByteArray(len);
  if (jba_colvaltocheck == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDUPDATEROW_PARAM));
    return HTC_ERROR_CHECKANDUPDATEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_colvaltocheck, 0, len, 
			    (const jbyte*)colValToCheck.data());
 
  jlong j_tid = transID;  
  jlong j_ts = timestamp;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
                JavaMethods_[JM_CHECKANDUPDATE].methodID, 
                j_tid, jba_rowID, jDirectBuffer, 
	        jba_columntocheck, jba_colvaltocheck, j_ts);

  jenv_->DeleteLocalRef(jba_rowID);  
  jenv_->DeleteLocalRef(jba_columntocheck);  
  jenv_->DeleteLocalRef(jba_colvaltocheck);
  jenv_->DeleteLocalRef(jDirectBuffer);
  
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::checkAndUpdateRow()", getLastError());
    return HTC_ERROR_CHECKANDUPDATEROW_EXCEPTION;
  }

  if (jresult == false) 
  {
     GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_CHECKANDUPDATE_ROW_NOTFOUND));
     return HTC_ERROR_CHECKANDUPDATE_ROW_NOTFOUND;
  }
  return HTC_OK;
}
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
const char *HTableClient_JNI::getTableName()
{
  return tableName_;
}
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
std::string* HTableClient_JNI::getHTableName()
{
  jstring js_name = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_HTNAME].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::getHTableName()", getLastError());
    return NULL;
  }
 
  if (js_name == NULL)
    return NULL;
    
  const char* char_result = jenv_->GetStringUTFChars(js_name, 0);
  std::string* tableName = new (heap_) std::string(char_result);
  jenv_->ReleaseStringUTFChars(js_name, char_result);
  jenv_->DeleteLocalRef(js_name);  
  
  return tableName;
}

HTC_RetCode HTableClient_JNI::coProcAggr(Int64 transID, 
					 int aggrType, // 0:count, 1:min, 2:max, 3:sum, 4:avg
					 const Text& startRowID, 
					 const Text& stopRowID, 
					 const Text &colFamily,
					 const Text &colName,
					 const NABoolean cacheBlocks,
					 const Lng32 numCacheRows,
					 Text &aggrVal) // returned value
{

  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::coProcAggr called.");

  int len = 0;

  len = startRowID.size();
  jbyteArray jba_startrowid = jenv_->NewByteArray(len);
  if (jba_startrowid == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_startrowid, 0, len, 
			    (const jbyte*)startRowID.data());

  len = stopRowID.size();
  jbyteArray jba_stoprowid = jenv_->NewByteArray(len);
  if (jba_stoprowid == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_stoprowid, 0, len, 
			    (const jbyte*)stopRowID.data());
 
  len = colFamily.size();
  jbyteArray jba_colfamily = jenv_->NewByteArray(len);
  if (jba_colfamily == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_colfamily, 0, len, 
			    (const jbyte*)colFamily.data());
 
  len = colName.size();
  jbyteArray jba_colname = jenv_->NewByteArray(len);
  if (jba_colname == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_colname, 0, len, 
			    (const jbyte*)colName.data());

  jlong j_tid = transID;  
  jint j_aggrtype = aggrType;

  jboolean j_cb = cacheBlocks;
  jint j_ncr = numCacheRows;

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_COPROC_AGGR].methodID, j_tid, j_aggrtype, jba_startrowid, jba_stoprowid, jba_colfamily, jba_colname, j_cb, j_ncr);

  jenv_->DeleteLocalRef(jba_startrowid);  
  jenv_->DeleteLocalRef(jba_stoprowid);  
  jenv_->DeleteLocalRef(jba_colfamily);
  jenv_->DeleteLocalRef(jba_colname);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::coProcAggr()", getLastError());
    return HTC_ERROR_COPROC_AGGR_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HTableClient_JNI::coProcAggr()", getLastError());
    return HTC_ERROR_COPROC_AGGR_EXCEPTION;
  }

  jobject jbal = jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_COPROC_AGGR_GET_RESULT].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HTableClient_JNI::coprocAggr()", getLastError());
    return HTC_ERROR_COPROC_AGGR_EXCEPTION;
  }

  if (jbal == NULL)
  {
    return HTC_ERROR_COPROC_AGGR_EXCEPTION;
  } 

  ByteArrayList * aggrValList = newByteArrayList(jbal);
  if (aggrValList == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }  
  Text * val = aggrValList->get(0);
  if (val == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }  
  aggrVal = *val;

  return HTC_OK;
}

ByteArrayList* HTableClient_JNI::getEndKeys()
{
  jobject jByteArrayList = 
     jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GETENDKEYS].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    return NULL;
  }

  if (jByteArrayList == NULL)
    return NULL;

  ByteArrayList* endKeys = new (heap_) ByteArrayList(heap_, jByteArrayList);
  if (endKeys->init() != BAL_OK)
     return NULL;
  return endKeys;

}

HTC_RetCode HTableClient_JNI::flushTable()
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HBaseClient_JNI::flushTable() called.");

  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_FLUSHT].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HBaseClient_JNI::flushTable()", getLastError());
    return HTC_ERROR_FLUSHTABLE_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HBaseClient_JNI::flushTable()", getLastError());
    return HTC_ERROR_FLUSHTABLE_EXCEPTION;
  }

  return HTC_OK;
}

// ===========================================================================
// ===== Class HiveClient_JNI
// ===========================================================================

JavaMethodInit* HiveClient_JNI::JavaMethods_ = NULL;
jclass HiveClient_JNI::javaClass_ = 0;
bool HiveClient_JNI::javaMethodsInitialized_ = false;
pthread_mutex_t HiveClient_JNI::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const hvcErrorEnumStr[] = 
{
 "Preparing parameters for initConnection()."
 ,"Java exception in initConnection()."
 ,"Java exception in close()."
 ,"Preparing parameters for exists()."
 ,"Java exception in exists()."
 ,"Preparing parameters for getHiveTableStr()."
 ,"Java exception in getHiveTableStr()."
 ,"Preparing parameters for getRedefTime()."
 ,"Java exception in getRedefTime()."
 ,"Java exception in getAllSchemas()."
 ,"Preparing parameters for getAllTables()."
 ,"Java exception in getAllTables()."
};



//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* HiveClient_JNI::getErrorText(HVC_RetCode errEnum)
{
  if (errEnum < (HVC_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)hvcErrorEnumStr[errEnum-HVC_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HiveClient_JNI* HiveClient_JNI::getInstance()
{
    ContextCli *currContext = GetCliGlobals()->currContext();
    HiveClient_JNI *hiveClient_JNI = currContext->getHiveClient();
    if (hiveClient_JNI == NULL)
    { 
       NAHeap *heap = currContext->exHeap();
       hiveClient_JNI = new (heap) HiveClient_JNI(heap);
       currContext->setHiveClient(hiveClient_JNI);
  }
  return hiveClient_JNI;
}

void HiveClient_JNI::deleteInstance()
{
  ContextCli *currContext = GetCliGlobals()->currContext();
  HiveClient_JNI *hiveClient_JNI = currContext->getHiveClient();
  if (hiveClient_JNI != NULL)
  {
     NAHeap *heap = currContext->exHeap();
     NADELETE(hiveClient_JNI, HiveClient_JNI, heap);
     currContext->setHiveClient(NULL);
  }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HiveClient_JNI::~HiveClient_JNI()
{
  if (isConnected_)
    close(); // error handling?
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::init()
{
  static char className[]="org/trafodion/sql/HBaseAccess/HiveClient";
  HVC_RetCode rc;
  
  if (isInitialized())
    return HVC_OK;
  
  if (javaMethodsInitialized_)
    return (HVC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HVC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR       ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR       ].jm_signature = "()V";
    JavaMethods_[JM_GET_ERROR  ].jm_name      = "getLastError";
    JavaMethods_[JM_GET_ERROR  ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_INIT       ].jm_name      = "init";
    JavaMethods_[JM_INIT       ].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_CLOSE      ].jm_name      = "close";
    JavaMethods_[JM_CLOSE      ].jm_signature = "()Z";
    JavaMethods_[JM_EXISTS     ].jm_name      = "exists";
    JavaMethods_[JM_EXISTS     ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_GET_HVT    ].jm_name      = "getHiveTableString";
    JavaMethods_[JM_GET_HVT    ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";
    JavaMethods_[JM_GET_RDT    ].jm_name      = "getRedefTime";
    JavaMethods_[JM_GET_RDT    ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)J";
    JavaMethods_[JM_GET_ASH     ].jm_name      = "getAllSchemas";
    JavaMethods_[JM_GET_ASH     ].jm_signature = "()Lorg/trafodion/sql/HBaseAccess/StringArrayList;";
    JavaMethods_[JM_GET_ATL    ].jm_name      = "getAllTables";
    JavaMethods_[JM_GET_ATL    ].jm_signature = "(Ljava/lang/String;)Lorg/trafodion/sql/HBaseAccess/StringArrayList;";
   
    rc = (HVC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
jstring HiveClient_JNI::getLastJavaError()
{
  if (javaObj_ == NULL)
    return NULL;

  // java.lang.String getLastError();
  return (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_ERROR].methodID);
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::initConnection(const char* metastoreURI)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HiveClient_JNI::initConnection(%s) called.", metastoreURI);

  jstring js_metastoreURI = jenv_->NewStringUTF(metastoreURI);
  if (js_metastoreURI == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_INIT_PARAM));
    return HVC_ERROR_INIT_PARAM;
  }
  

  // boolean init(java.lang.String); 
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_INIT].methodID, js_metastoreURI);

  jenv_->DeleteLocalRef(js_metastoreURI);   

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HiveClient_JNI::initConnection()", getLastError());
    return HVC_ERROR_INIT_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_HBASE, "HiveClient_JNI::initConnection()", getLastError());
    return HVC_ERROR_INIT_EXCEPTION;
  }

  isConnected_ = TRUE;
  return HVC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::close()
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HiveClient_JNI::close() called.");

  // boolean close();
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_CLOSE].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HiveClient_JNI::close()", getLastError());
    return HVC_ERROR_CLOSE_EXCEPTION;
  }
  
  if (jresult == false) 
  {
    logError(CAT_HBASE, "HiveClient_JNI::close()", getLastError());
    return HVC_ERROR_CLOSE_EXCEPTION;
  }

  return HVC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::exists(const char* schName, const char* tabName)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HiveClient_JNI::exists(%s, %s) called.", schName, tabName);
  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_EXISTS_PARAM));
    return HVC_ERROR_EXISTS_PARAM;
  }
  jstring js_tabName = jenv_->NewStringUTF(tabName);
  if (js_tabName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_EXISTS_PARAM));
    return HVC_ERROR_EXISTS_PARAM;
  }

  // boolean exists(java.lang.String, java.lang.String);
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_EXISTS].methodID, js_schName, js_tabName);

  jenv_->DeleteLocalRef(js_schName);
  jenv_->DeleteLocalRef(js_tabName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HiveClient_JNI::exists()", getLastError());
    return HVC_ERROR_EXISTS_EXCEPTION;
  }

  if (jresult == false) 
     return HVC_DONE;  // Table does not exist

  return HVC_OK;  // Table exists.
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::getHiveTableStr(const char* schName, 
                                            const char* tabName, 
                                            Text& hiveTblStr)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getHiveTableStr(%s, %s, %s).", schName, tabName, hiveTblStr.data());
  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_HVT_PARAM));
    return HVC_ERROR_GET_HVT_PARAM;
  }
  jstring js_tabName = jenv_->NewStringUTF(tabName);
  if (js_tabName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_HVT_PARAM));
    return HVC_ERROR_GET_HVT_PARAM;
  }

  // java.lang.String getHiveTableString(java.lang.String, java.lang.String);
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, 
                                            JavaMethods_[JM_GET_HVT].methodID, 
                                            js_schName, js_tabName);

  jenv_->DeleteLocalRef(js_schName);
  jenv_->DeleteLocalRef(js_tabName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HiveClient_JNI::getHiveTableStr()", getLastError());
    return HVC_ERROR_GET_HVT_EXCEPTION;
  }

  if (jenv_->GetStringLength(jresult) <= 0)
    return HVC_DONE; // Table does not exist
    
  // Not using UFTchars and NAWString for now.
  const char* char_result = jenv_->GetStringUTFChars(jresult, 0);
  hiveTblStr += char_result ; // deep copy. hiveTblStr is assumed to be empty.
  jenv_->ReleaseStringUTFChars(jresult, char_result);
  jenv_->DeleteLocalRef(jresult);  

  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "Exit HiveClient_JNI::getHiveTableStr(%s, %s, %s).", schName, tabName, hiveTblStr.data());
  return HVC_OK;  // Table exists.
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////  
HVC_RetCode HiveClient_JNI::getRedefTime(const char* schName, 
                                         const char* tabName, 
                                         Int64& redefTime)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getRedefTime(%s, %s, %lld).", schName, tabName, redefTime);
  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_REDEFTIME_PARAM));
    return HVC_ERROR_GET_REDEFTIME_PARAM;
  }
  jstring js_tabName = jenv_->NewStringUTF(tabName);
  if (js_tabName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_REDEFTIME_PARAM));
    return HVC_ERROR_GET_REDEFTIME_PARAM;
  }

  //  jlong getRedefTime(java.lang.String, java.lang.String);
  jlong jresult = jenv_->CallLongMethod(javaObj_, 
                                        JavaMethods_[JM_GET_RDT].methodID, 
                                        js_schName, js_tabName);

  jenv_->DeleteLocalRef(js_schName);
  jenv_->DeleteLocalRef(js_tabName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HiveClient_JNI::getRedefTime()", getLastError());
    return HVC_ERROR_GET_REDEFTIME_EXCEPTION;
  }

  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "Exit HiveClient_JNI::getRedefTime(%s, %s, %lld).", schName, tabName, redefTime);

  if (jresult < 0)
    return HVC_DONE; // Table does not exist

  redefTime = jresult ;
  return HVC_OK;  // Table exists.
  
}

//////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////// 
HVC_RetCode HiveClient_JNI::getAllSchemas(LIST(Text *)& schNames)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getAllSchemas(%p) called.", (void *) &schNames);

  jobject jStringArrayList = 
     jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_ASH].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HiveClient_JNI::getAllSchemas()", getLastError());
    return HVC_ERROR_GET_ALLSCH_EXCEPTION;
  }

   StringArrayList* schNames_SAL = new (heap_) StringArrayList(heap_, 
                     jStringArrayList);
   if (schNames_SAL == NULL)
     return HVC_DONE;

   if (schNames_SAL->init() != SAL_OK)
      return HVC_ERROR_GET_ALLSCH_EXCEPTION;

   int numSchemas = schNames_SAL->getSize();
   if (numSchemas == 0)
     return HVC_DONE;

   Text * schName;
   for(int i=0; i<numSchemas; i++) {
     schName = schNames_SAL->get(i); // Allocation is done in the get method.
     schNames.insert(schName);
   }

  NADELETE(schNames_SAL, StringArrayList, schNames_SAL->getHeap());
  jenv_->DeleteLocalRef(jStringArrayList); 
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "Exit HiveClient_JNI::getAllSchemas(%p) called.", (void *) &schNames);

  return HVC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::getAllTables(const char* schName, 
                                         LIST(Text *)& tblNames)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getAllTables(%s, %p) called.", schName, (void *) &tblNames);

  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_ALLTBL_PARAM));
    return HVC_ERROR_GET_ALLTBL_PARAM;
  }

  jobject jStringArrayList = 
    jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_ATL].methodID, 
                            js_schName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_HBASE, __FILE__, __LINE__);
    logError(CAT_HBASE, "HiveClient_JNI::getAllTables()", getLastError());
    return HVC_ERROR_GET_ALLTBL_EXCEPTION;
  }

   StringArrayList* tabNames_SAL = new (heap_) StringArrayList(heap_, 
                                                       jStringArrayList);
   if (tabNames_SAL == NULL)
     return HVC_DONE;
   if (tabNames_SAL->init() != SAL_OK)
      return HVC_ERROR_GET_ALLTBL_EXCEPTION;
   int numTables = tabNames_SAL->getSize();
   if (numTables == 0)
     return HVC_DONE;

   Text * tblName;
   for(int i=0; i<numTables; i++) {
     tblName = tabNames_SAL->get(i); // allocation done in the get method, 
                                     //what heap should we use?
     tblNames.insert(tblName);
   }

  NADELETE(tabNames_SAL, StringArrayList, tabNames_SAL->getHeap());
  jenv_->DeleteLocalRef(jStringArrayList);  
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, "Exit HiveClient_JNI::getAllTables(%s, %p) called.", schName, (void *) &tblNames);

  return HVC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////  
void HiveClient_JNI::logIt(const char* str)
{
  HdfsLogger::log(CAT_HBASE, LL_DEBUG, str);
}

#ifdef __cplusplus
extern "C" {
#endif
/*
 *   Class:     org_trafodion_sql_HBaseAccess_HTableClient
 *   Method:    setResultInfo
 *   Signature: (J[I[I[I[I[I[I[J[[B[B)I
 *  
 */

JNIEXPORT jint JNICALL Java_org_trafodion_sql_HBaseAccess_HTableClient_setResultInfo
  (JNIEnv *jenv, jobject jobj, jlong jniObject, 
	jintArray jKvValLen, jintArray jKvValOffset, 
        jintArray jKvQualLen, jintArray jKvQualOffset,
        jintArray jKvFamLen, jintArray jKvFamOffset, 
        jlongArray jTimestamp, 
        jobjectArray jKvBuffer, jobjectArray jRowIDs,
        jintArray jKvsPerRow, jint numCellsReturned)
{
   HTableClient_JNI *htc = (HTableClient_JNI *)jniObject;
   htc->setResultInfo(jKvValLen, jKvValOffset,
                jKvQualLen, jKvQualOffset, jKvFamLen, jKvFamOffset,
                jTimestamp, jKvBuffer, jRowIDs, jKvsPerRow, numCellsReturned);  
   return 0;
}
#ifdef __cplusplus
}
#endif

void HTableClient_JNI::setResultInfo( jintArray jKvValLen, jintArray jKvValOffset,
        jintArray jKvQualLen, jintArray jKvQualOffset,
        jintArray jKvFamLen, jintArray jKvFamOffset,
        jlongArray jTimestamp, 
        jobjectArray jKvBuffer, jobjectArray jRowIDs,
        jintArray jKvsPerRow, jint numCellsReturned)
{
  if (numRowsReturned_ > 0)
     cleanupResultInfo();
   NABoolean exceptionFound = FALSE;
   jKvValLen_ = (jintArray)jenv_->NewGlobalRef(jKvValLen);
   if (jenv_->ExceptionCheck())
       exceptionFound = TRUE;
   if (! exceptionFound)
   {
      jKvValOffset_ = (jintArray)jenv_->NewGlobalRef(jKvValOffset);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound)
   {
      jKvQualLen_ = (jintArray)jenv_->NewGlobalRef(jKvQualLen);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound)
   {
      jKvQualOffset_ = (jintArray)jenv_->NewGlobalRef(jKvQualOffset);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound)
   {
      jKvFamLen_ = (jintArray)jenv_->NewGlobalRef(jKvFamLen);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound)
   {
      jKvFamOffset_ = (jintArray)jenv_->NewGlobalRef(jKvFamOffset);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound)
   {
      jTimestamp_ = (jlongArray)jenv_->NewGlobalRef(jTimestamp);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound)
   {
      jKvBuffer_ = (jobjectArray)jenv_->NewGlobalRef(jKvBuffer);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound)
   {
      jRowIDs_ = (jobjectArray)jenv_->NewGlobalRef(jRowIDs);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound)
   {
      jKvsPerRow_ = (jintArray)jenv_->NewGlobalRef(jKvsPerRow);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   numCellsReturned_ = numCellsReturned;
   currentRowNum_ = 0;
   prevRowCellNum_ = 0;
   cleanupDone_ = FALSE;
   ex_assert(! exceptionFound, "Exception in setResultInfo");
   return;
} 

void HTableClient_JNI::cleanupResultInfo()
{
   if (cleanupDone_)
      return;
   jenv_->DeleteGlobalRef(jKvValLen_);
   jenv_->DeleteGlobalRef(jKvValOffset_);
   jenv_->DeleteGlobalRef(jKvQualLen_);
   jenv_->DeleteGlobalRef(jKvQualOffset_);
   jenv_->DeleteGlobalRef(jKvFamLen_);
   jenv_->DeleteGlobalRef(jKvFamOffset_);
   jenv_->DeleteGlobalRef(jTimestamp_);
   jenv_->DeleteGlobalRef(jKvBuffer_);
   jenv_->DeleteGlobalRef(jRowIDs_);
   if (jba_kvBuffer_ != NULL)
   {
      jenv_->DeleteGlobalRef(jba_kvBuffer_);
      jba_kvBuffer_ = NULL;
   }
   if (p_rowID_ != NULL)
   {
      jenv_->ReleaseByteArrayElements(jba_rowID_, p_rowID_, JNI_ABORT);
      p_rowID_ = NULL;
      jenv_->DeleteGlobalRef(jba_rowID_);
   }
   if (p_kvsPerRow_ != NULL)
   {
      jenv_->ReleaseIntArrayElements(jKvsPerRow_, p_kvsPerRow_, JNI_ABORT);
       p_kvsPerRow_ = NULL;
   }
   jenv_->DeleteGlobalRef(jKvsPerRow_);
   cleanupDone_ = TRUE;
   return;
}

HTC_RetCode HTableClient_JNI::nextRow()
{
    HTC_RetCode retCode;

    ex_assert(fetchMode_ != UNKNOWN, "invalid fetchMode_");
    if (fetchMode_ == GET_ROW && numRowsReturned_ == -1)
       return HTC_DONE;
    if (currentRowNum_ == -1 || ((currentRowNum_+1) >= numRowsReturned_))
    {
        if (currentRowNum_ != -1)
        {
            switch (fetchMode_)
            {
              case BATCH_GET:
                cleanupResultInfo();
                return HTC_DONE_RESULT;
              case GET_ROW:
                cleanupResultInfo();
                return HTC_DONE;
              case SCAN_FETCH:
                if (numRowsReturned_ < numReqRows_)
                {
                   cleanupResultInfo();
                   return HTC_DONE;
                }   
                break;
            }
        }
        retCode = fetchRows();
        if (retCode != HTC_OK)
        {
           cleanupResultInfo();
           return retCode;
        }
        // Allocate Buffer and copy the cell info
        int numCellsNeeded;
        if (numCellsAllocated_ == 0 || 
			numCellsAllocated_ < numCellsReturned_)
        {
           NAHeap *heap = getHeap();
           if (numCellsAllocated_ > 0)
           {
              NADELETEBASIC(p_kvValLen_, heap);
              NADELETEBASIC(p_kvValOffset_, heap);
              NADELETEBASIC(p_kvFamLen_, heap);
              NADELETEBASIC(p_kvFamOffset_, heap);
              NADELETEBASIC(p_kvQualLen_, heap);
              NADELETEBASIC(p_kvQualOffset_, heap);
              NADELETEBASIC(p_timestamp_, heap);
              numCellsNeeded = numCellsReturned_;
           }
           else
              numCellsNeeded = 2 * numReqRows_ * numColsInScan_;
           p_kvValLen_ = new (heap) jint[numCellsNeeded];
           p_kvValOffset_ = new (heap) jint[numCellsNeeded];
           p_kvFamLen_ = new (heap) jint[numCellsNeeded];
           p_kvFamOffset_ = new (heap) jint[numCellsNeeded];
           p_kvQualLen_ = new (heap) jint[numCellsNeeded];
           p_kvQualOffset_ = new (heap) jint[numCellsNeeded];
           p_timestamp_ = new (heap) jlong[numCellsNeeded];
           numCellsAllocated_ = numCellsNeeded;
        }
        jenv_->GetIntArrayRegion(jKvValLen_, 0, numCellsReturned_, p_kvValLen_);
        jenv_->GetIntArrayRegion(jKvValOffset_, 0, numCellsReturned_, p_kvValOffset_);
        jenv_->GetIntArrayRegion(jKvQualLen_, 0, numCellsReturned_, p_kvQualLen_);
        jenv_->GetIntArrayRegion(jKvQualOffset_, 0, numCellsReturned_, p_kvQualOffset_);
        jenv_->GetIntArrayRegion(jKvFamLen_, 0, numCellsReturned_, p_kvFamLen_);
        jenv_->GetIntArrayRegion(jKvFamOffset_, 0, numCellsReturned_, p_kvFamOffset_);
        jenv_->GetLongArrayRegion(jTimestamp_, 0, numCellsReturned_, p_timestamp_);
        p_kvsPerRow_ = jenv_->GetIntArrayElements(jKvsPerRow_, NULL);
        currentRowNum_ = 0;
        prevRowCellNum_ = 0;
    }
    else
    {
        // Add the number of previous cells returned
        jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
        prevRowCellNum_ += kvsPerRow;  
        currentRowNum_++;
    }
    // clean the rowID of the previous row
    if (p_rowID_ != NULL)
    {
       jenv_->ReleaseByteArrayElements(jba_rowID_, p_rowID_, JNI_ABORT);
       p_rowID_ = NULL;
       jenv_->DeleteGlobalRef(jba_rowID_);
    }
    return HTC_OK;
}

HTC_RetCode HTableClient_JNI::getColName(int colNo,
              char **outColName, 
              short &colNameLen,
              Int64 &timestamp)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    if (kvsPerRow == 0 || colNo >= kvsPerRow)
    {
       *outColName == NULL;
       timestamp = 0;
       return HTC_OK;
    }
    int idx = prevRowCellNum_ + colNo;
    ex_assert((idx < numCellsReturned_), "Buffer overflow");
    jint kvQualLen = p_kvQualLen_[idx];
    jint kvQualOffset = p_kvQualOffset_[idx];
    jint kvFamLen = p_kvFamLen_[idx];
    jint kvFamOffset = p_kvFamOffset_[idx];

    // clean the kvBuffer of the previous column
    // And get the kvBuffer for the current column
    if (jba_kvBuffer_ != NULL)
    {
       jenv_->DeleteGlobalRef(jba_kvBuffer_);
       jba_kvBuffer_ = NULL;
    }
    jobject kvBufferObj;
    kvBufferObj = jenv_->GetObjectArrayElement(jKvBuffer_, idx);
    if (jenv_->ExceptionCheck())
    {
      getExceptionDetails();
      logError(CAT_HBASE, __FILE__, __LINE__);
      logError(CAT_HBASE, "HTableClient_JNI::getColName()", getLastError());
      return HTC_GET_COLNAME_EXCEPTION;
    }

    jba_kvBuffer_ = (jbyteArray)jenv_->NewGlobalRef(kvBufferObj);
    jenv_->DeleteLocalRef(kvBufferObj);

    colNameLen = kvQualLen + kvFamLen + 1; // 1 for ':'
    char * colName;
    if (colNameAllocLen_ == 0  && colNameLen <= INLINE_COLNAME_LEN)
    	colName = inlineColName_;
    else
    {
        if (colNameLen > colNameAllocLen_)
        {
	   if (colNameAllocLen_ != 0)
              NADELETEBASIC(colName_, heap_);
           colName_ = new (heap_) char[colNameLen+1];
           colNameAllocLen_ = colNameLen;
        }
        colName = colName_; 
    }
    jenv_->GetByteArrayRegion(jba_kvBuffer_, kvFamOffset, kvFamLen, 
            (jbyte *)colName);
    colName[kvFamLen] = ':';
    char *temp = colName+ kvFamLen+1;
    jenv_->GetByteArrayRegion(jba_kvBuffer_, kvQualOffset, kvQualLen, 
            (jbyte *)temp);
    timestamp = p_timestamp_[idx];
    *outColName = colName;
    return HTC_OK; 
}

HTC_RetCode HTableClient_JNI::getColVal(int colNo, BYTE *colVal, 
          Lng32 &colValLen, NABoolean nullable, BYTE &nullVal)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    if (kvsPerRow == 0 || colNo >= kvsPerRow)
       return HTC_GET_COLVAL_EXCEPTION;
    int idx = prevRowCellNum_ + colNo;
    ex_assert((idx < numCellsReturned_), "Buffer overflow");
    jint kvValLen = p_kvValLen_[idx];
    jint kvValOffset = p_kvValOffset_[idx];
    Lng32 copyLen;
    jbyte nullByte;
    // If the column is nullable, get the first byte
    // The first byte determines if the column is null(0xff) or not (0)
    if (nullable)
    {
       copyLen = MINOF(kvValLen-1, colValLen);
       jenv_->GetByteArrayRegion(jba_kvBuffer_, kvValOffset, 1, &nullByte); 
       jenv_->GetByteArrayRegion(jba_kvBuffer_, kvValOffset+1, copyLen, 
               (jbyte *)colVal); 
    }
    else 
    {
        copyLen = MINOF(kvValLen, colValLen);
        nullByte = 0;
    	jenv_->GetByteArrayRegion(jba_kvBuffer_, kvValOffset, copyLen,
             (jbyte *)colVal); 
    }
    nullVal = nullByte;
    colValLen = copyLen;
    return HTC_OK;
}

HTC_RetCode HTableClient_JNI::getColVal(NAHeap *heap, int colNo, BYTE **colVal, 
          Lng32 &colValLen)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    if (kvsPerRow == 0 || colNo >= kvsPerRow)
       return HTC_GET_COLVAL_EXCEPTION;
    int idx = prevRowCellNum_ + colNo;
    ex_assert((idx < numCellsReturned_), "Buffer overflow");
    jint kvValLen = p_kvValLen_[idx];
    jint kvValOffset = p_kvValOffset_[idx];
    BYTE *colValTmp = new (heap) BYTE[kvValLen];
    jenv_->GetByteArrayRegion(jba_kvBuffer_, kvValOffset, kvValLen,
             (jbyte *)colValTmp); 
    *colVal = colValTmp;
    colValLen = kvValLen;
    return HTC_OK;
}

HTC_RetCode HTableClient_JNI::getNumCols(int &numCols)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    numCols = kvsPerRow;
    if (numCols == 0)
       return HTC_DONE_DATA;
    else
       return HTC_OK;
}  


HTC_RetCode HTableClient_JNI::getRowID(HbaseStr &rowID)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    if (p_rowID_ != NULL)
    {
      jenv_->ReleaseByteArrayElements(jba_rowID_, p_rowID_, JNI_ABORT);
      p_rowID_ = NULL;
      jenv_->DeleteGlobalRef(jba_rowID_);
    }

    if (kvsPerRow == 0) 
    {
       rowID.len = 0;
       rowID.val = NULL;
    }
    else
    {
       jobject rowIDObj;
       rowIDObj = jenv_->GetObjectArrayElement(jRowIDs_, currentRowNum_);
       jba_rowID_ = (jbyteArray)jenv_->NewGlobalRef(rowIDObj);
       jenv_->DeleteLocalRef(rowIDObj);
       p_rowID_ = jenv_->GetByteArrayElements(jba_rowID_, NULL);
       rowID.len = jenv_->GetArrayLength(jba_rowID_); 
       rowID.val = (char *)p_rowID_;
    }
    return HTC_OK;
}

HTC_RetCode HTableClient_JNI::fetchRows()
{
   HdfsLogger::log(CAT_HBASE, LL_DEBUG, "HTableClient_JNI::fetchRows() called.");
   jlong jniObject = (jlong)this;
   jlong jRowsReturned = jenv_->CallLongMethod(javaObj_, 
             JavaMethods_[JM_FETCH_ROWS].methodID,
             jniObject);

   if (jenv_->ExceptionCheck())
   {
      getExceptionDetails();
      logError(CAT_HBASE, __FILE__, __LINE__);
      logError(CAT_HBASE, "HTableClient_JNI::fetchRows()", getLastError());
      return HTC_ERROR_FETCHNEXTROW_EXCEPTION;
   }

   numRowsReturned_ = jRowsReturned;
   if (numRowsReturned_ == -1)
   {
      GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_FETCHNEXTROW_EXCEPTION));
      logError(CAT_HBASE, "HTableClient_JNI::fetchRows()", getLastJavaError());
      return HTC_ERROR_FETCHNEXTROW_EXCEPTION;
   }
   else
   if (numRowsReturned_ == 0)
      return HTC_DONE;
   return HTC_OK; 
}

jobjectArray convertToByteArrayObjectArray(const TextVec &vec)
{
   int vecLen = vec.size();
   int i = 0;
   jobjectArray j_objArray = NULL;
   if (jenv_->ExceptionCheck())
      return NULL;
   for (std::vector<Text>::const_iterator it = vec.begin(); 
           it != vec.end(); ++it, i++)
   {
       jbyteArray j_obj = jenv_->NewByteArray((*it).size());
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       jenv_->SetByteArrayRegion(j_obj, 0, (*it).size(), (const jbyte *)(*it).data());
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(vecLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
   }
   return j_objArray;
}

jobjectArray convertToStringObjectArray(const TextVec &vec)
{
   int vecLen = vec.size();
   int i = 0;
   jobjectArray j_objArray = NULL;
   for (std::vector<Text>::const_iterator it = vec.begin(); 
           it != vec.end(); ++it, i++)
   {
       jstring j_obj = jenv_->NewStringUTF((*it).data());
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(vecLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
   }
   return j_objArray;
}

jobjectArray convertToStringObjectArray(const HBASE_NAMELIST& nameList)
{
   int listLen = nameList.entries();
   int i = 0;
   jobjectArray j_objArray = NULL;
   for ( i = 0; i < listLen ; i++)
   {
       jstring j_obj = jenv_->NewStringUTF(nameList.at(i).val);
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(listLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
   }
   return j_objArray;
}

jobjectArray convertToStringObjectArray(const NAText *textArray, int arrayLen)
{
   int i = 0;
   jobjectArray j_objArray = NULL;
   for ( i = 0; i < arrayLen ; i++)
   {
       jstring j_obj = jenv_->NewStringUTF(textArray[i].c_str());
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(arrayLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
   }
   return j_objArray;
}

