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

// ===========================================================================
// ===== The HiveClient_JNI class implements access to the Java 
// ===== HiveClient class.
// ===========================================================================
#ifndef HIVE_CLIENT_H
#define HIVE_CLIENT_H

#include <list>
#include "JavaObjectInterface.h"

typedef enum {
  HVC_OK     = JOI_OK
 ,HVC_FIRST  = JOI_LAST
 ,HVC_DONE   = HVC_FIRST
 ,HVC_ERROR_INIT_PARAM
 ,HVC_ERROR_CLOSE_EXCEPTION
 ,HVC_ERROR_EXISTS_PARAM
 ,HVC_ERROR_EXISTS_EXCEPTION
 ,HVC_ERROR_GET_HVT_PARAM
 ,HVC_ERROR_GET_HVT_EXCEPTION
 ,HVC_ERROR_GET_HVP_PARAM
 ,HVC_ERROR_GET_HVP_EXCEPTION
 ,HVC_ERROR_GET_REDEFTIME_PARAM
 ,HVC_ERROR_GET_REDEFTIME_EXCEPTION
 ,HVC_ERROR_GET_ALLSCH_EXCEPTION
 ,HVC_ERROR_GET_ALLTBL_PARAM
 ,HVC_ERROR_GET_ALLTBL_EXCEPTION
 ,HVC_ERROR_EXECUTE_HIVE_SQL_PARAM
 ,HVC_ERROR_EXECUTE_HIVE_SQL_EXCEPTION
 ,HVC_LAST
} HVC_RetCode;

class HiveClient_JNI : public JavaObjectInterface
{
public:
  static HiveClient_JNI* getInstance();
  static void deleteInstance();

  // Destructor
  virtual ~HiveClient_JNI();
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  HVC_RetCode init();

  HVC_RetCode close();
  static HVC_RetCode exists(const char* schName, const char* tabName);
  static HVC_RetCode getHiveTableStr(const char* schName, const char* tabName, 
                              Text& hiveTblStr);
  static HVC_RetCode getHiveTableParameters(const char *schName, const char *tabName, 
                              Text& hiveParamsStr);
  static HVC_RetCode getRedefTime(const char* schName, const char* tabName, 
                           Int64& redefTime);
  static HVC_RetCode getAllSchemas(NAHeap *heap, LIST(Text *)& schNames);
  static HVC_RetCode getAllTables(NAHeap *heap, const char* schName, LIST(Text *)& tblNames);

  static HVC_RetCode executeHiveSQL(const char* hiveSQL);
  // Get the error description.
  static char* getErrorText(HVC_RetCode errEnum);
  
  static void logIt(const char* str);

private:   
  // Private Default constructor		
  HiveClient_JNI(NAHeap *heap)
  :  JavaObjectInterface(heap)
  , isConnected_(FALSE)
  {}

private:  
  enum JAVA_METHODS {
    JM_CTOR = 0
   ,JM_CLOSE
   ,JM_EXISTS     
   ,JM_GET_HVT
   ,JM_GET_HVP
   ,JM_GET_RDT
   ,JM_GET_ASH
   ,JM_GET_ATL
   ,JM_EXEC_HIVE_SQL
   ,JM_LAST
  };
  static jclass          javaClass_; 
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
  bool isConnected_;
};
#endif
