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
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         LmUtility.cpp
* Description:  Native C code used by LmUtility.java
*
* Created:      06/06/2002
* Language:     C++
*
*
******************************************************************************
*/

#include "LmUtility.h"
#include "ComCextdecs.h"
#include "NAString.h"
#include "Int64.h"
#include "str.h"
#include <stdlib.h>
#include "LmDebug.h"
#include "sqlcli.h"
#include "LmAssert.h"

  #include <sys/types.h>
  #include <unistd.h>
  #define GETPID getpid

  #include "dtm/tm.h"

// sqlAccessMode stores the SQL Access mode the SPJ was registered with.
// This value is populated before SPJ method is called and is used
// by LmSQLMXDriver class to decide whether to allow SPJ to get a datbase
// connection or not. This is only used for Type 4 connections.
// Type 2 connections work as they used to.
Int32 sqlAccessMode =0;

void lmUtilitySetSqlAccessMode(Int32 mode) { sqlAccessMode = mode; }
// sqlAccessMode stores the SQL Access mode the SPJ was registered with.
// This value is populated before SPJ method is called and is used
// by LmSQLMXDriver class to decide whether to allow SPJ to get a datbase
// connection or not. This is only used for Type 4 connections.
// Type 2 connections work as they used to.

// transactionAttrs stores the Transaction Required the SPJ was registered with.
// This value is populated before SPJ method is called and is used
// by LmSQLMXDriver class to decide whether to allow SPJ to join a datbase
// transaction or not. 
Int32 transactionAttrs =0;

void lmUtilitySetTransactionAttrs(Int32 transAttrs) { transactionAttrs = transAttrs; }


// 'lmUtilityConnList' below contains a list of java.sql.Connection 
// object references of default connections i.e.
// Java connection objects created using the url 
// "jdbc:default:connection".
//
// These connection objects come from the LmSQLMXDriver java
// class via the 'addConnection()' native method. Whenever a
// default connection is created within a SPJ method the 
// LmSQLMXDriver class will call the above native method passing
// it the new created default connection's object reference.
//
// The basic idea behind having this list is to provide a 
// mechanism for the LmRoutine class to retrieve all the 
// default connection objects that may get created as part
// of the SPJ invocation. The following steps should be followed:
//
// 1. Just before a SPJ method gets invoked the 'lmUtilityInitConnList()'
//    should be called to empty 'lmUtilityConnList'. If there are any 
//    connection objects present in the list then they are closed and 
//    their global reference are deleted.
//
// 2. Invoke the SPJ method. Now the 'lmUtilityConnList' will get populated 
//    with default connection objects that the SPJ method may have created.
//
// 3. Immediately after the SPJ method invocation completes 
//    call the 'lmUtilityGetConnList()' method to get the 
//    'lmUtilityConnList'.

// 4. Retrieve all the connection entries from 'lmUtilityConnList' and
//    then remove them from the above list.
//
// Note: 
// The caller of 'lmUtilityGetConnList()' will be responsible to delete
// the retrieved connection references when they are no longer needed.
// 

NAList<jobject> lmUtilityConnList(NULL);


// If there are any Java connection objects present in 
// 'lmUtilityConnList' then the following actions are taken:
// - Calls the 'close()' method on the connection
// - Deletes the object reference
// - Removes the connection entry from 'lmUtilityConnList'
// 
// The method expects the following as input:
// - Pointer to the JNIEnv interface
// - JNI method ID of java.sql.Connection.close() method
//
void lmUtilityInitConnList( JNIEnv *jni, jmethodID connCloseId )
{
  while( lmUtilityConnList.entries() )
  {
    jobject conn = lmUtilityConnList[0];

    if( conn )
    {
      jni->CallVoidMethod( conn,
                           connCloseId );
      jni->ExceptionClear();
    }

    jni->DeleteGlobalRef( conn );
    lmUtilityConnList.removeAt(0);
  }

  lmUtilityConnList.clear();
}

// Returns the 'connList' as a reference
NAList<jobject> &lmUtilityGetConnList() { return lmUtilityConnList; }

static void Throw(JNIEnv *env, const char *msg)
{
  jclass c = env->FindClass("java/lang/Exception");
  //
  // If c is NULL an exception has already been thrown
  //
  if (c != NULL)
  {
    env->ThrowNew(c, msg);
  }
  env->DeleteLocalRef(c);
}

void SQL_ERROR_HANDLER(Lng32 sqlCode)
{
  // This is a no-op for now. Might be useful in the future for logic
  // that needs to be executed after any CLI error.
}

// The MXStatement class provides a simple interface into the SQL/MX
// CLI and can be used for SQL operations in a C++ program that is not
// a preprocessed embedded program. The LmUtility::utils() native
// method supports an "ExecSql" operation where an arbitrary SQL
// string is compiled and executed through the MXStatement interface.
class MXStatement
{
public:

  // This class creates input and output descriptors with a fixed
  // maximum on the number of columns. Removing this limitation is
  // future work.
  enum Constants
  {
    MAX_COLUMNS_IN_DESC = 500
  };

  MXStatement()
  {
    initialized_ = false;

    moduleId_.module_name = NULL;
    
    stmtId_.version = SQLCLI_CURRENT_VERSION;
    stmtId_.name_mode = stmt_handle;
    stmtId_.module = &moduleId_;
    stmtId_.identifier = 0;
    stmtId_.handle = 0;
    
    stmtTextDesc_.version = SQLCLI_CURRENT_VERSION;
    stmtTextDesc_.name_mode = desc_handle;
    stmtTextDesc_.module = &moduleId_;
    stmtTextDesc_.identifier = 0;
    stmtTextDesc_.handle = 0;
    
    inDesc_.version = SQLCLI_CURRENT_VERSION;
    inDesc_.name_mode = desc_handle;
    inDesc_.module = &moduleId_;
    inDesc_.identifier = 0;
    inDesc_.handle = 0;

    outDesc_.version = SQLCLI_CURRENT_VERSION;
    outDesc_.name_mode = desc_handle;
    outDesc_.module = &moduleId_;
    outDesc_.identifier = 0;
    outDesc_.handle = 0;

    numInColumns_ = 0;
    numOutColumns_ = 0;
  }
  
  Lng32 init(const char *&status)
  {
    Lng32 result = 0;
    status = "OK";
    
    if (initialized_)
      return result;
    
    stmtText_ = NULL;
    
    if (result == 0)
    {
      result = SQL_EXEC_ClearDiagnostics(NULL);
      if (result != 0)
      {
        status = "SQL_EXEC_ClearDiagnostics failed";
      }
    }
    
    if (result == 0)
    {
      result = SQL_EXEC_AllocStmt(&stmtId_, 0);
      if (result != 0)
      {
        status = "SQL_EXEC_AllocStmt failed";
      }
    }

    if (result == 0)
    {
      result = SQL_EXEC_AllocDesc(&stmtTextDesc_, 1);
      if (result != 0)
      {
        status = "SQL_EXEC_AllocDesc failed for statement text";
        SQL_EXEC_DeallocStmt(&stmtId_);
      }
    }
    
    if (result == 0)
    {
      result = SQL_EXEC_AllocDesc(&inDesc_, MAX_COLUMNS_IN_DESC);
      SQL_ERROR_HANDLER(result);
      if (result != 0)
      {
        status = "SQL_EXEC_AllocDesc failed for input descriptor";
        SQL_EXEC_DeallocDesc(&stmtTextDesc_);
        SQL_EXEC_DeallocStmt(&stmtId_);
      }
    }

    if (result == 0)
    {
      result = SQL_EXEC_AllocDesc(&outDesc_, MAX_COLUMNS_IN_DESC);
      SQL_ERROR_HANDLER(result);
      if (result != 0)
      {
        status = "SQL_EXEC_AllocDesc failed for output descriptor";
        SQL_EXEC_DeallocDesc(&inDesc_);
        SQL_EXEC_DeallocDesc(&stmtTextDesc_);
        SQL_EXEC_DeallocStmt(&stmtId_);
      }
    }

    if (result == 0)
      initialized_ = true;

    SQL_ERROR_HANDLER(result);
    return result;
  }

  ~MXStatement()
  {
    if (initialized_)
    {
      SQL_EXEC_ClearDiagnostics(NULL);
      SQL_EXEC_DeallocDesc(&outDesc_);
      SQL_EXEC_DeallocDesc(&inDesc_);
      SQL_EXEC_DeallocDesc(&stmtTextDesc_);
      SQL_EXEC_DeallocStmt(&stmtId_);
    }
    delete [] stmtText_;
  }

  Lng32 prepare(const char *stmtText)
  {
    if (!initialized_)
    {
      return -9999;
    }

    delete [] stmtText_;
    stmtText_ = new char[strlen(stmtText) + 1];
    strcpy(stmtText_, stmtText);
    
    Lng32 result = 0;

    if (!result)
      result = SQL_EXEC_ClearDiagnostics(NULL);
    if (!result)
      result = SQL_EXEC_SetDescItem(&stmtTextDesc_, 1, SQLDESC_TYPE,
                                    SQLTYPECODE_CHAR, 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&stmtTextDesc_, 1, SQLDESC_LENGTH,
                                    strlen(stmtText_), 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&stmtTextDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long) stmtText_, 0);
    if (!result)
      result = SQL_EXEC_Prepare(&stmtId_, &stmtTextDesc_);

    if (!result)
      result = SQL_EXEC_DescribeStmt(&stmtId_, &inDesc_, &outDesc_);

    if (!result)
      result = SQL_EXEC_GetDescEntryCountBasic(&outDesc_, &numOutColumns_);

    if (!result)
      result = SQL_EXEC_GetDescEntryCountBasic(&inDesc_, &numInColumns_);

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 execute()
  {
    if (!initialized_)
    {
      return -9999;
    }

    Lng32 result = 0;

    if (!result)
      result = SQL_EXEC_ClearDiagnostics(NULL);
    if (!result)
      result = SQL_EXEC_ExecClose(&stmtId_, NULL, 0);

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 executeUsingLong(Lng32 i)
  {
    if (!initialized_)
    {
      return -9999;
    }

    Lng32 result = 0;

    if (!result)
      result = SQL_EXEC_ClearDiagnostics(NULL);
    if (!result)
      result = SQL_EXEC_SetDescItem(&inDesc_, 1, SQLDESC_TYPE,
                                    SQLTYPECODE_INTEGER, 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&inDesc_, 1, SQLDESC_LENGTH,
                                    sizeof(i), 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&inDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long) &i, 0);
    if (!result)
      result = SQL_EXEC_ExecClose(&stmtId_, &inDesc_, 0);

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 executeUsingString(const char *s, Lng32 len)
  {
    if (!initialized_)
    {
      return -9999;
    }

    Lng32 result = 0;

    if (!result)
      result = SQL_EXEC_ClearDiagnostics(NULL);
    if (!result)
      result = SQL_EXEC_SetDescItem(&inDesc_, 1, SQLDESC_TYPE,
                                    (Lng32) SQLTYPECODE_CHAR, 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&inDesc_, 1, SQLDESC_LENGTH,
                                    (Lng32) len, 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&inDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long) s, 0);
    if (!result)
      result = SQL_EXEC_ExecClose(&stmtId_, &inDesc_, 0);

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 fetchEOD()
  {
    if (!initialized_)
    {
      return -9999;
    }

    Lng32 result = SQL_EXEC_Fetch(&stmtId_, NULL, 0);
    if (result == 100)
    {
      result = 0;
    }

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 fetchLong(Lng32 &i)
  {
    if (!initialized_)
    {
      return -9999;
    }

    Lng32 result = 0;

    if (!result)
      result = SQL_EXEC_SetDescItem(&outDesc_, 1, SQLDESC_TYPE,
                                    SQLTYPECODE_INTEGER, 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&outDesc_, 1, SQLDESC_LENGTH,
                                    sizeof(i), 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&outDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long) &i, 0);
    if (!result)
      result = SQL_EXEC_Fetch(&stmtId_, &outDesc_, 0);

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 fetchString(char *buf, Lng32 bufLen)
  {
    if (!initialized_)
    {
      return -9999;
    }

    Lng32 result = 0;

    if (!result)
      result = SQL_EXEC_SetDescItem(&outDesc_, 1, SQLDESC_TYPE,
                                    SQLTYPECODE_VARCHAR, 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&outDesc_, 1, SQLDESC_LENGTH,
                                    bufLen, 0);
    if (!result)
      result = SQL_EXEC_SetDescItem(&outDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long) buf, 0);
    if (!result)
      result = SQL_EXEC_Fetch(&stmtId_, &outDesc_, 0);

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 fetchStrings(char **strings, Lng32 bufLen)
  {
    if (!initialized_)
      return -9999;

    Lng32 result = 0;
    for (Lng32 i = 0; i < numOutColumns_ && !result; i++)
    {
      if (!result)
        result = SQL_EXEC_SetDescItem(&outDesc_, i + 1, SQLDESC_TYPE,
                                      SQLTYPECODE_VARCHAR, 0);
      if (!result)
        result = SQL_EXEC_SetDescItem(&outDesc_, i + 1, SQLDESC_LENGTH,
                                      bufLen, 0);
      if (!result)
        result = SQL_EXEC_SetDescItem(&outDesc_, i + 1, SQLDESC_VAR_PTR,
                                      (Long) (strings[i]), 0);
    }

    if (!result)
      result = SQL_EXEC_Fetch(&stmtId_, &outDesc_, 0);

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 close()
  {
    if (!initialized_)
    {
      return -9999;
    }

    Lng32 result = 0;

    if (!result)
      result = SQL_EXEC_CloseStmt(&stmtId_);

    SQL_ERROR_HANDLER(result);
    return result;
  }

  Lng32 getNumInColumns() { return numInColumns_; }
  Lng32 getNumOutColumns() { return numOutColumns_; }

protected:
  SQLSTMT_ID stmtId_;
  SQLMODULE_ID moduleId_;
  SQLDESC_ID stmtTextDesc_;
  SQLDESC_ID inDesc_;
  SQLDESC_ID outDesc_;
  Lng32 numInColumns_;
  Lng32 numOutColumns_;
  char *stmtText_;
  bool initialized_;

}; // class MXStatement

//
// This is the LmUtility.nativeUtils method. It takes one string as
// input and produces one string as output. The output string gets
// written to the first element of the String[] array object passed in
// as the jobjectArray parameter.
//
// Although we do not document this method for customers, there is
// nothing preventing customer code from calling this method. So don't
// put anything in the method that you wouldn't want customers
// doing. Currently this function just serves as an entry point to
// various systems calls such as TMF operations and getting/setting
// environment variables. There is nothing here that customers could
// not do on their own if they wanted to.
//
JNIEXPORT void JNICALL Java_org_trafodion_sql_udr_LmUtility_nativeUtils
(JNIEnv * env, jclass jc, jstring js, jobjectArray joa)
{
  const char *input = env->GetStringUTFChars(js, NULL);
  if (input == NULL)
  {
    // OutOfMemory error already thrown
    return;
  }

  NAString action(input);
  TrimNAStringSpace(action);

  short error;
  NAString result("OK");

  static MXStatement staticStmt;

  if (action.compareTo("GetTxName", NAString::ignoreCase) == 0)
  {
    Int64 transid;
    error = GETTRANSID((short *) &transid);
    if (error)
    {
      if (error == 75)
      {
        result = "No active transaction";
      }
      else
      {
        result = "GETTRANSID returned ";
        result += LongToNAString((Lng32) error);
      }
      Throw(env, result.data());
    }
    else
    {
      short actualLen;
      char text[256];
      error = TRANSIDTOTEXT(transid, text, 255, &actualLen);
      if (error)
      {
        result = "TRANSIDTOTEXT returned ";
        result += LongToNAString((Lng32) error);
        Throw(env, result);
      }
      else
      {
        text[actualLen] = 0;
        result = text;
      }
    }
  } // GetTxName

  else if (action.compareTo("BeginTx", NAString::ignoreCase) == 0)
  {
    Int32 tag;
    error = BEGINTRANSACTION(&tag);
    if (error)
    {
      result = "BEGINTRANSACTION returned ";
      result += LongToNAString((Lng32) error);
      Throw(env, result);
    }
  } // BeginTx

  else if (action.compareTo("CommitTx", NAString::ignoreCase) == 0)
  {
    error = ENDTRANSACTION();
    if (error)
    {
      if (error == 75)
      {
        result = "No active transaction";
      }
      else
      {
        result = "ENDTRANSACTION returned ";
        result += LongToNAString((Lng32) error);
      }
      Throw(env, result);
    }
  } // CommitTx

  else if (action.compareTo("RollbackTx", NAString::ignoreCase) == 0)
  {
    error = ABORTTRANSACTION();
    if (error)
    {
      if (error == 75)
      {
        result = "No active transaction";
      }
      else
      {
        result = "ABORTTRANSACTION returned ";
        result += LongToNAString((Lng32) error);
      }
      Throw(env, result);
    }
  } // RollbackTx

  else if (action.compareTo("GetProcessId", NAString::ignoreCase) == 0)
  {
    Lng32 pid = GETPID();
    result = LongToNAString(pid);
  } // GetProcessId

  else if (action.index("GetEnv ", 0, NAString::ignoreCase) == 0)
  {
    NAString name = action;
    name.remove(0, str_len("GetEnv "));
    TrimNAStringSpace(name);
    char *value = getenv(name.data());
    if (value != NULL)
    {
      result = value;
    }
    else
    {
      result = "";
    }
  } // GetEnv

  else if (action.index("PutEnv ", 0, NAString::ignoreCase) == 0)
  {
    NAString nameAndValue = action;
    nameAndValue.remove(0, str_len("PutEnv "));
    TrimNAStringSpace(nameAndValue);
    Int32 retcode = putenv((char *) nameAndValue.data());
    if (retcode != 0)
    {
      result = "putenv returned ";
      result += LongToNAString((Lng32) retcode);
      Throw(env, result);
    }
  } // PutEnv

  else if (action.index("LmDebug ", 0, NAString::ignoreCase) == 0)
  {
    NAString name = action;
    name.remove(0, str_len("LmDebug "));
    LM_DEBUG0(name.data());
  } // LmDebug

  else if (action.index("ExecSql ", 0, NAString::ignoreCase) == 0)
  {
    NAString stmtText = action.remove(0, str_len("ExecSql "));

    MXStatement s;
    const char *status = "OK";
    Lng32 retcode = 0;

    retcode = s.init(status);
  
    if (retcode == 0)
    {
      retcode = s.prepare(stmtText.data());
      if (retcode != 0)
      {
        status = "PREPARE failed";
      }
    }
  
    if (retcode == 0)
    {
      retcode = s.execute();
      if (retcode != 0)
      {
        status = "EXECUTE failed";
      }
    }
  
    if (retcode == 0)
    {
      retcode = s.fetchEOD();
      if (retcode != 0)
      {
        status = "FETCH failed";
      }
    }
  
    if (retcode == 0)
    {
      retcode = s.close();
      if (retcode != 0)
      {
        status = "CLOSE failed";
      }
    }

    if (retcode != 0)
    {
      char msg[256];
      sprintf(msg, "[UdrSqlException %d] %s", retcode, status);
      Throw(env, msg);
    }
  
  } // ExecSql

  else if (action.index("FetchSql ", 0, NAString::ignoreCase) == 0)
  {
    // The incoming string is SQL statement text. The code below will
    // prepare and execute the statement then fetch only the first
    // row. It will build one long multi-line string containing all
    // column values, one on each line. The multi-line string can be
    // split by the Java caller into an array of Strings with the
    // split("\n") method.

    Lng32 i;
    NAString stmtText = action.remove(0, str_len("FetchSql "));

    MXStatement s;
    const char *status = "OK";
    Lng32 retcode = 0;

    retcode = s.init(status);
  
    if (!retcode)
    {
      retcode = s.prepare(stmtText.data());
      if (retcode)
        status = "PREPARE failed";
    }
  
    if (!retcode)
    {
      retcode = s.execute();
      if (retcode)
        status = "EXECUTE failed";
    }

    Lng32 numOutColumns = s.getNumOutColumns();
    NABoolean stringsAllocated = FALSE;
    char **argv = NULL;

    if (!retcode && numOutColumns > 0)
    {
      argv = new char *[numOutColumns];
      Lng32 bufLen = 1000;
      for (i = 0; i < numOutColumns; i++)
        argv[i] = new char[bufLen + 1];

      stringsAllocated = TRUE;

      retcode = s.fetchStrings(argv, bufLen);
      if (retcode)
        status = "FETCH STRINGS failed";

      if (!retcode)
      {
        result = argv[0];
        for (i = 1; i < numOutColumns; i++)
        {
          result += "\n";
          result += argv[i];
        }
      }
    }
  
    if (!retcode)
    {
      retcode = s.fetchEOD();
      if (retcode)
        status = "FETCH EOD failed";
    }
  
    if (!retcode)
    {
      retcode = s.close();
      if (retcode)
        status = "CLOSE failed";
    }

    if (stringsAllocated)
    {
      for (i = 0; i < numOutColumns; i++)
        delete [] argv[i];
      delete [] argv;
    }

    if (retcode)
    {
      char msg[256];
      sprintf(msg, "[UdrSqlException %d] %s", retcode, status);
      Throw(env, msg);
    }
  
  } // FetchSql

  else if (action.index("Prepare ", 0, NAString::ignoreCase) == 0)
  {
    NAString stmtText = action.remove(0, str_len("Prepare "));

    const char *status = "OK";
    Lng32 retcode = 0;

    retcode = staticStmt.init(status);
  
    if (retcode == 0)
    {
      retcode = staticStmt.prepare(stmtText.data());
      if (retcode != 0)
      {
        status = "PREPARE failed";
      }
    }
  
    if (retcode)
    {
      char msg[256];
      sprintf(msg, "[UdrSqlException %d] %s", retcode, status);
      Throw(env, msg);
    }

  } // Prepare
  
  else if (action.index("ExecUsingString ", 0, NAString::ignoreCase) == 0)
  {
    NAString data = action.remove(0, str_len("ExecUsingString "));

    const char *status = "OK";
    Lng32 retcode = 0;

    if (retcode == 0)
    {
      retcode = staticStmt.executeUsingString(data.data(),
                                              (Lng32) data.length());
      if (retcode != 0)
      {
        status = "EXECUTE failed";
      }
    }
  
    if (retcode == 0)
    {
      retcode = staticStmt.fetchEOD();
      if (retcode != 0)
      {
        status = "FETCH failed";
      }
    }
  
    if (retcode == 0)
    {
      retcode = staticStmt.close();
      if (retcode != 0)
      {
        status = "CLOSE failed";
      }
    }

    if (retcode != 0)
    {
      char msg[256];
      sprintf(msg, "[UdrSqlException %d] %s", retcode, status);
      Throw(env, msg);
    }
  
  } // ExecUsingString

  else if (action.index("FetchUsingString ", 0, NAString::ignoreCase) == 0)
  {
    NAString data = action.remove(0, str_len("FetchUsingString "));
    const char *status = "OK";
    Lng32 retcode = 0;
    Int32 i = 0;

    if (!retcode)
    {
      retcode = staticStmt.executeUsingString(data.data(),
                                              (Lng32) data.length());
      if (retcode)
        status = "EXECUTE failed";
    }

    Lng32 numOutColumns = staticStmt.getNumOutColumns();
    NABoolean stringsAllocated = FALSE;
    char **argv = NULL;

    if (!retcode && numOutColumns > 0)
    {
      argv = new char *[numOutColumns];
      Lng32 bufLen = 1000;
      for (i = 0; i < numOutColumns; i++)
        argv[i] = new char[bufLen + 1];

      stringsAllocated = TRUE;

      retcode = staticStmt.fetchStrings(argv, bufLen);
      if (retcode)
        status = "FETCH STRINGS failed";

      if (!retcode)
      {
        result = argv[0];
        for (i = 1; i < numOutColumns; i++)
        {
          result += "\n";
          result += argv[i];
        }
      }
    }
  
    if (!retcode)
    {
      retcode = staticStmt.fetchEOD();
      if (retcode)
        status = "FETCH EOD failed";
    }
  
    if (!retcode)
    {
      retcode = staticStmt.close();
      if (retcode)
        status = "CLOSE failed";
    }

    if (stringsAllocated)
    {
      for (i = 0; i < numOutColumns; i++)
        delete [] argv[i];
      delete [] argv;
    }

    if (retcode)
    {
      char msg[256];
      sprintf(msg, "[UdrSqlException %d] %s", retcode, status);
      Throw(env, msg);
    }
  
  } // FetchUsingString

  else
  {
    //
    // Over time other operations can be supported
    //
    result = "Invalid action: ";
    result += action;
    Throw(env, result);
  }

  //
  // Create the Java output string
  //
  if (env->ExceptionCheck() == JNI_FALSE)
  {
    jobject j = env->NewStringUTF(result.data());
    env->SetObjectArrayElement(joa, 0, j);
  }

}

// Calls the RegisterNatives() JNI method to register 
// the native methods defined in this file to the 
// appropriate LM Java class. 
// RegisterNatives() removes the need for creating a 
// DLL for the native methods on Yosemite systems.
//
// Input parameters:
// env : Pointer to JNI environment
// lmCls : Pointer to LmUtility class object
//
// Return Value:
// 0  : The native methods were registered successfully 
// <0 : An exception was throw by the JVM. The caller 
// is responsible to report any exceptions.
//
Int32 registerLmUtilityMethods(JNIEnv *env, jclass lmCls)
{
  // To add new native methods to the 'jnm' array just
  // increment the array size and set the new entry's fields 
  // as needed.
  const Int32 numMethods = 2;
  JNINativeMethod jnm[numMethods];

  jnm[0].name = (char *)"nativeUtils";
  jnm[0].signature = (char *)"(Ljava/lang/String;[Ljava/lang/String;)V";
  jnm[0].fnPtr = (void *)Java_org_trafodion_sql_udr_LmUtility_nativeUtils;

  jnm[1].name = (char *)"getTransactionId";
  jnm[1].signature = (char *)"()[S";
  jnm[1].fnPtr = (void *)Java_org_trafodion_sql_udr_LmUtility_getTransactionId;

  // RegisterNatives() returns zero on success or a negative
  // value on a failure.
  return env->RegisterNatives(lmCls, &jnm[0], numMethods);
}

Int32 registerLmT2DriverMethods(JNIEnv *env, jclass lmCls)
{
  // To add new native methods to the 'jnm' array just
  // increment the array size and set the new entry's fields 
  // as needed.
  const Int32 numMethods = 4;
  JNINativeMethod jnm[numMethods];

  jnm[0].name = (char *)"addConnection";
  jnm[0].signature = (char *)"(Ljava/lang/Object;)V";
  jnm[0].fnPtr = (void *)Java_com_tandem_sqlmx_LmT2Driver_addConnection;

  jnm[1].name = (char *)"getSqlAccessMode";
  jnm[1].signature = (char *)"()I";
  jnm[1].fnPtr = (void *)Java_com_tandem_sqlmx_LmT2Driver_getSqlAccessMode;

  jnm[2].name = (char *)"getTransId";
  jnm[2].signature = (char *)"()J";
  jnm[2].fnPtr = (void *)Java_com_tandem_sqlmx_LmT2Driver_getTransId;
  
  jnm[3].name = (char *)"getTransactionAttrs";
  jnm[3].signature = (char *)"()I";
  jnm[3].fnPtr = (void *)Java_com_tandem_sqlmx_LmT2Driver_getTransactionAttrs;

  // RegisterNatives() returns zero on success or a negative
  // value on a failure.
  return env->RegisterNatives(lmCls, &jnm[0], numMethods);
}


// This native method is called by LmT2Driver to get the SQL Access
// mode of the SPJ
JNIEXPORT jint JNICALL Java_com_tandem_sqlmx_LmT2Driver_getSqlAccessMode
(JNIEnv *env, jclass jc)
{
  return (jint) sqlAccessMode;
}

// This native method is called by LmT2Driver to get the Transaction 
// Attributes of the SPJ
JNIEXPORT jint JNICALL Java_com_tandem_sqlmx_LmT2Driver_getTransactionAttrs
(JNIEnv *env, jclass jc)
{
  return (jint) transactionAttrs;
}

// This native method is called by the LmT2Driver Java class
// whenever a java.sql.Connection object of type default connection 
// is created. The default connection's object reference is passed 
// as input to this method.
//
// This method creates a global reference of the provided connection
// object and adds it to 'lmUtilityConnList'.
//
JNIEXPORT void JNICALL Java_com_tandem_sqlmx_LmT2Driver_addConnection
(JNIEnv * env, jclass jc, jobject conn)
{
  LM_ASSERT( conn != NULL );
  jobject newConn = env->NewGlobalRef( conn );
  lmUtilityConnList.insert(newConn);
}
// This method returns the transaction id that can be passed
// down to Type 4 JDBC driver
JNIEXPORT jshortArray JNICALL Java_org_trafodion_sql_udr_LmUtility_getTransactionId
(JNIEnv * env, jclass jc)
{
    // On Seaquest, SQL/NDCS/TSE all use 64-bit transactionid
    Int64 transid;
    short *stransid = (short *)&transid;
    short error = GETTRANSID(stransid);
    jshortArray returnArray = env->NewShortArray(4);
    env->SetShortArrayRegion(returnArray, 0, 4, stransid);
    return returnArray;
}

// This method is used by LmSQLMXDriver to retrieve the transaction id 
// to be passed to MXOSRVR via JDBC T4 driver joinUDRTransaction method
JNIEXPORT jlong JNICALL Java_com_tandem_sqlmx_LmT2Driver_getTransId
(JNIEnv * env, jclass jc)
{
  Int64 transId = 0;
  short *stransId = (short *)&transId;
  short error = GETTRANSID(stransId);
  if (error)
	transId = -1;
  return (jlong) transId;
}

