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
* File:         LmUtility.h
* Description:  Native C code used by LmUtility.java
*
* Created:      06/06/2002
* Language:     C++
*
*
******************************************************************************
*/

#ifndef _LMUTILITY_H_
#define _LMUTILITY_H_

#include "lmjni.h"

#include "Collections.h"
#include "LmLangManagerJava.h"

//
// To generate signatures for the LmUtility native methods:
//
// 1. Build the Language Manager. This will create
//    w:/lib/nt/debug/mxlangman.jar.
//
// 2. Run the following commands:
//
//    cd w:/langman
//    javah -jni -classpath w:/lib/nt/debug/mxlangman.jar \
//          org.trafodion.sql.udr.LmUtility
//
// 3. Copy signatures from the newly generated file
//    org_trafodion_sql_udr_LmUtility.h into this file.
//

// NOTE: The native methods declared in this file have to be
// ----  registered with the appropriate Java class by calling 
//       the 'RegisterNatives()' JNI method. Currently the 
//       following methods are available to register the native
//       methods.
//       
//       - registerLmUtilityMethods() 
//         Registers native methods of LmUtility Java class
//
//       - registerLmSQLMXDriverMethods()
//         Registers native methods of LmSQLMXDriver Java class
//

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_trafodion_sql_udr_LmUtility
 * Method:    nativeUtils
 * Signature: (Ljava/lang/String;[Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_trafodion_sql_udr_LmUtility_nativeUtils
  (JNIEnv *, jclass, jstring, jobjectArray);

/*
 * Class:     org_trafodion_sql_udr_LmUtility
 * Method:    getTransactionId
 * Signature: ()[S
 */
JNIEXPORT jshortArray JNICALL Java_org_trafodion_sql_udr_LmUtility_getTransactionId
  (JNIEnv *, jclass);

/*
 * Class:     com_tandem_sqlmx_LmT2Driver
 * Method:    getTransactionAttrs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_tandem_sqlmx_LmT2Driver_getTransactionAttrs
  (JNIEnv *, jclass);
  
/*
 * Class:     com_tandem_sqlmx_LmT2Driver
 * Method:    getSqlAccessMode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_tandem_sqlmx_LmT2Driver_getSqlAccessMode
  (JNIEnv *, jclass);

/*
 * Class:     com_tandem_sqlmx_LmT2Driver
 * Method:    getSqlAccessMode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_tandem_sqlmx_LmT2Driver_getSqlAccessMode
  (JNIEnv *, jclass);

/*
 * Class:     com_tandem_sqlmx_LmT2Driver
 * Method:    addConnection
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_com_tandem_sqlmx_LmT2Driver_addConnection
  (JNIEnv *, jclass, jobject);

/*
 * Class:     com_tandem_sqlmx_LmT2Driver
 * Method:    getTransId
 * Signature:()J 
 */
JNIEXPORT jlong JNICALL Java_com_tandem_sqlmx_LmT2Driver_getTransId
(JNIEnv *, jclass);
#ifdef __cplusplus
}
#endif

// Calls the RegisterNatives() JNI method to register 
// the native methods defined in this file to the 
// appropriate LM Java class.
Int32 registerLmUtilityMethods(JNIEnv *env, jclass lmCls);
Int32 registerLmT2DriverMethods(JNIEnv *env, jclass lmCls);

// See the LmUtility.cpp file for details on the below
// methods
void lmUtilityInitConnList( JNIEnv *env, jmethodID connCloseId );
NAList<jobject> &lmUtilityGetConnList();
void lmUtilitySetSqlAccessMode(Int32 mode);
void lmUtilitySetTransactionAttrs(Int32 mode);


#endif // _LMUTILITY_H_
