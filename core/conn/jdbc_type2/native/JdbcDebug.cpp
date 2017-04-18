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
// MODULE: JdbcDebug.cpp
//
#include "org_apache_trafodion_jdbc_t2_JdbcDebug.h"
#include "Debug.h"

/*
 * Class:     org_apache_trafodion_jdbc_t2_JdbcDebug
 * Method:    getDebugHandle
 * Signature: (I)I
 */
JNIEXPORT jlong JNICALL Java_org_apache_trafodion_jdbc_t2_JdbcDebug_getDebugHandle(JNIEnv *jenv, jclass jcls,
																	  jlong method_name_handle)
{
	jlong handle = 0;
#if defined(_BENCHMARK)
	handle = (jlong) new Benchmark((const char *) method_name_handle);
#endif /* _BENCHMARK */
	return(handle);
}

/*
 * Class:     org_apache_trafodion_jdbc_t2_JdbcDebug
 * Method:    getMethodNameHandle
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jlong JNICALL Java_org_apache_trafodion_jdbc_t2_JdbcDebug_getMethodNameHandle(JNIEnv *jenv, jclass jcls,
																		   jstring method_name)
{
	jlong rc = 0;
#if defined(_DEBUG) || defined(_BENCHMARK)
	if (method_name) rc = (jlong) jenv->GetStringUTFChars(method_name,NULL);
	if (rc==0) rc = (jlong) "Unknown";
#endif /* _DEBUG || _BENCHMARK */
	return(rc);
}

/*
 * Class:     org_apache_trafodion_jdbc_t2_JdbcDebug
 * Method:    methodEntry
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_JdbcDebug_methodEntry(JNIEnv *jenv, jclass jcls,
																   jlong debug_handle,
																   jint debug_level,
																   jlong method_name_handle)
{
#if defined(_DEBUG)
	DebugFunctionEntry((const char *) method_name_handle, debug_level, NULL , NULL, 0);
#endif /* _DEBUG */
#if defined(_BENCHMARK)
	((Benchmark *) debug_handle)->Entry();
#endif /* _BENCHMARK */
}

/*
 * Class:     org_apache_trafodion_jdbc_t2_JdbcDebug
 * Method:    methodReturn
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_JdbcDebug_methodReturn(JNIEnv *jenv, jclass jcls,
																	jlong debug_handle,
																	jstring comment)
{
#if defined(_DEBUG)
	const char *commentStr;
	if (comment) commentStr = jenv->GetStringUTFChars(comment,NULL);
	else commentStr = NULL;
	DebugFunctionReturn("JAVA", commentStr, false, "RETURNING", NULL, 0);
	if (commentStr) jenv->ReleaseStringUTFChars(comment,commentStr);
#endif /* _DEBUG */
}

/*
 * Class:     org_apache_trafodion_jdbc_t2_JdbcDebug
 * Method:    methodExit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_JdbcDebug_methodExit(JNIEnv *jenv, jclass jcls,
																  jlong debug_handle)
{
#if defined(_DEBUG)
	DebugFunctionReturn("JAVA", NULL, true, "EXITING", NULL, 0);
#endif /* _DEBUG */

#if defined(_BENCHMARK)
	((Benchmark *) debug_handle)->Exit();
#endif /* _BENCHMARK */
}

/*
 * Class:     org_apache_trafodion_jdbc_t2_JdbcDebug
 * Method:    traceOut
 * Signature: (IILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_JdbcDebug_traceOut(JNIEnv *jenv, jclass jcls,
																	jlong debug_handle,
																	jint debug_level,
																	jstring comment)
{
#if defined(_DEBUG)
	const char *commentStr;
	if (comment) commentStr = jenv->GetStringUTFChars(comment,NULL);
	else commentStr = NULL;
	if (DebugActive(debug_level,NULL,0)) DebugOutput(commentStr , NULL, 0);
	if (commentStr) jenv->ReleaseStringUTFChars(comment,commentStr);
#endif /* _DEBUG */
}
