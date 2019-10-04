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
// MODULE: SQLMXDriver.cpp
//
/**************************************************************************/
/*
  * Methods Changed: Java_org_apache_trafodion_jdbc_t2_T2Driver_SQLMXInitialize(JNIEnv *, jclass, jstring, jint, jstring, jstring)
  */
#include <platform_ndcs.h>
#include <sys/types.h> /* optional except for POSIX.1 */
#include <unistd.h>
#ifdef NSK_PLATFORM
#include <sqlWin.h>
#include <windows.h>
#include "cextdecs.h"
#include "NskUtil.h"
#include "pThreadsSync.h"
#else
#include <sql.h>
#endif
#include "Vproc.h"
#include <sqlext.h>
#include "CoreCommon.h"
#include "SrvrCommon.h"
#include "JdbcDriverGlobal.h"
#include "SQLMXCommonFunctions.h"
#include "org_apache_trafodion_jdbc_t2_T2Driver.h"
#include "Debug.h"
#include "GlobalInformation.h"
#include "CommonLogger.h"
#include "sqlcli.h"

static bool driverVersionChecked = false;
#ifdef NSK_PLATFORM	// Linux port - ToDo txn related
int client_initialization(void);
#endif

void setTM_enable_cleanup ()
{
  static bool sv_envvar_setup = false;
  if (sv_envvar_setup) {
    return;
  }
  putenv("TMLIB_ENABLE_CLEANUP=0");
  sv_envvar_setup = true;
}

JNIEXPORT jint JNICALL Java_org_apache_trafodion_jdbc_t2_T2Driver_getPid (JNIEnv *env, jclass cls)
{
	return (jint)getpid();
}
/*
* Class:     org_apache_trafodion_jdbc_t2_T2Driver
* Method:    SQLMXInitialize
* Signature: (Ljava/lang/String;I)V
*/
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_T2Driver_SQLMXInitialize(JNIEnv *jenv, jclass cls,
																		 jstring language)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_T2Driver_SQLMXInitialize",("language=%s,
		DebugJString(jenv,language)));
	const char 					*nLanguage;
	//	static GlobalInformation	*globalInfo = new GlobalInformation();
        
	if (!driverVersionChecked)
	{
		printf("JDBC Library Version Error - Jar: Unknown Jni: %s\n",
			driverVproc);
		abort();
	}

	GlobalInformation::setSQLMX_Version();

	if (srvrGlobal == NULL)
		MEMORY_ALLOC_PERM(srvrGlobal,SRVR_GLOBAL_Def)
	else
	{
		if (srvrGlobal->boolFlgforInitialization == 1)
			FUNCTION_RETURN_VOID(("Already Initialized"));
	}


	if (! cacheJNIObjects(jenv))
		FUNCTION_RETURN_VOID(("cacheJNIObjects() failed"));

	srvrGlobal->dialogueId = 0;			// DialogueId is set to zero now

	srvrGlobal->boolFlgforInitialization = 1;
	FUNCTION_RETURN_VOID((NULL));
}


/*
* Class:     org_apache_trafodion_jdbc_t2_T2Driver
* Method:    setDefaultEncoding
* Signature: (Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_T2Driver_setDefaultEncoding(JNIEnv *jenv, jclass jcls,
																			jstring encoding)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_T2Driver_setDefaultEncoding",("encoding=%s",
		DebugJString(jenv,encoding)));
	gJNICache.defaultCharset = SQLCHARSETCODE_UNKNOWN;
	if (encoding)
	{
		const char *encoding_str = JNI_GetStringUTFChars(jenv,encoding, NULL);
		if (encoding_str)
		{
			gJNICache.defaultCharset = getCharset(encoding_str);
			JNI_ReleaseStringUTFChars(jenv,encoding, encoding_str);
		}
	}
	FUNCTION_RETURN_VOID((NULL));
}


/*
* Class:     org_apache_trafodion_jdbc_t2_T2Driver
* Method:    setCharsetEncodingOverride
* Signature: (I)Ljava/lang/String;
* Note: This function is a generic implementation to allow setting the
*       char set override encoding for the given charset (currently only the ISO88591
*       column is allowed to be overriden)
*/
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_T2Driver_setCharsetEncodingOverride(JNIEnv *jenv, jclass jcls,
																					jint charset, jstring encodingOverride)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_T2Driver_setCharsetEncodingOverride",
		("charset=%ld, encodingOverride=%s",charset, DebugJString(jenv,encodingOverride)));

	//Added for connect/disconnect impro.
		 if (srvrGlobal == NULL)
		 MEMORY_ALLOC_PERM(srvrGlobal,SRVR_GLOBAL_Def)

	for (int idx=0; idx<gJNICache.totalCharsets; idx++)
		if (charset == gJNICache.charsetInfo[idx].charset)
		{
			const char *nEncodingOverride = JNI_GetStringUTFChars(jenv,encodingOverride, NULL);
			if (strcmp(nEncodingOverride,defaultEncodingOption) == 0)
			{
				gJNICache.charsetInfo[idx].useDefaultEncoding = TRUE;
			}
			else
			{
				gJNICache.charsetInfo[idx].useDefaultEncoding = FALSE;
			}
			JNI_ReleaseStringUTFChars(jenv,encodingOverride, nEncodingOverride);
		}
		FUNCTION_RETURN_VOID((NULL));
}

/*
* Class:     org_apache_trafodion_jdbc_t2_T2Driver
* Method:    checkLibraryVersion
* Signature: (Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL Java_org_apache_trafodion_jdbc_t2_T2Driver_checkLibraryVersion(JNIEnv *jenv, jclass jcls,
																			 jstring javaVproc)
{


#ifdef NSK_PLATFORM	// Linux port - ToDo txn related
		int cliret = client_initialization();//by venu for TSLX
		if (cliret != 1)//client init return 1 on success
		{

				DEBUG_OUT(DEBUG_LEVEL_TXN,("TSLX initialization failed %d", cliret));

		}
		DEBUG_OUT(DEBUG_LEVEL_TXN,("TSLX Initialization done"));
#endif

	//Added for R3.0 Transaction issue sol. 10-100430-9906


	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_T2Driver_checkLibraryVersion",("javaVproc=%s",
		DebugJString(jenv,javaVproc)));

	// Set the TMLIB cleanup 
	setTM_enable_cleanup();

#ifdef NSK_PLATFORM		// Linux port
  	short status = tmfInit();

  	DEBUG_OUT(DEBUG_LEVEL_TXN,("tmfInit() returned %d", status));

  	if (status != 0)
  	{
  		throwTransactionException(jenv, status);
  		FUNCTION_RETURN_VOID(("status(%d) is non-zero",status));
	}
#endif

//End





	if (javaVproc && !driverVersionChecked)
	{
		const char *java_vproc_str = JNI_GetStringUTFChars(jenv,javaVproc, NULL);
		if (java_vproc_str)
		{
			if (strcmp(java_vproc_str,driverVproc)!=0)
			{
				printf("JDBC Library Version Error - Jar: %s Jni: %s\n",
					java_vproc_str,driverVproc);
				abort();
			}
			JNI_ReleaseStringUTFChars(jenv,javaVproc, java_vproc_str);
			driverVersionChecked = true;
		}
	}
	FUNCTION_RETURN_VOID((NULL));
}

JNIEXPORT jint JNICALL Java_org_apache_trafodion_jdbc_t2_T2Driver_getDatabaseMajorVersionJNI(JNIEnv *jenv, jclass jcls)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_T2Driver_getDatabaseMajorVersionJNI",("..."));

	int databaseMajorVersion = TRAFODION_JDBCT2_VER_MAJOR;

	FUNCTION_RETURN_NUMERIC(databaseMajorVersion,
		("Database Major Version = %d", databaseMajorVersion));
}

JNIEXPORT jint JNICALL Java_org_apache_trafodion_jdbc_t2_T2Driver_getDatabaseMinorVersionJNI(JNIEnv *jenv, jclass jcls)
{
	FUNCTION_ENTRY("Java_org_apache_trafodion_jdbc_t2_T2Driver_getDatabaseMinorVersionJNI",("..."));

	int databaseMinorVersion=TRAFODION_JDBCT2_VER_MINOR;

	FUNCTION_RETURN_NUMERIC(databaseMinorVersion,
		("Database Minor Version = %d", databaseMinorVersion));

}


