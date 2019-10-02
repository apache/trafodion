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
// MODULE: SQLMXClobReader.cpp
//
#include <platform_ndcs.h>
#include <sql.h>
#include <sqlext.h>
#include "JdbcDriverGlobal.h"
#include "org_apache_trafodion_jdbc_t2_SQLMXClobReader.h"
#include "SQLMXCommonFunctions.h"
#include "CoreCommon.h"
#include "SrvrCommon.h"
#include "SrvrOthers.h"
#include "CSrvrConnect.h"
#include "Debug.h"
#include "GlobalInformation.h"
#include "sqlcli.h"

JNIEXPORT jint JNICALL Java_org_apache_trafodion_jdbc_t2_SQLMXClobReader_readChunk
  (JNIEnv *jenv, jobject jobj, jstring jServer, jlong jDialogueId, jint jExtractMode, jstring jLobLocator, jobject jCharBuffer)
{
   odbc_SQLsrvr_ExtractLob_exc_ exception = {0,0,0};
   BYTE *chunkBuf = (BYTE *)jenv->GetDirectBufferAddress(jCharBuffer);
   jlong jLength = jenv->GetDirectBufferCapacity(jCharBuffer);
   const char *lobLocator = jenv->GetStringUTFChars(jLobLocator, NULL);
   IDL_long_long  lobLength = 0; // Used when extractMode is 0 
   IDL_long_long  extractLen = jLength;

   odbc_SQLSrvr_ExtractLob_sme_(NULL, NULL, &exception, jDialogueId, jExtractMode, 
                (IDL_char *)lobLocator, lobLength, extractLen, chunkBuf);
   jenv->ReleaseStringUTFChars(jLobLocator, lobLocator);
   switch (exception.exception_nr) {
      case CEE_SUCCESS:
         return extractLen;
      case odbc_SQLSvc_ExtractLob_SQLError_exn_:
         throwSQLException(jenv, &exception.u.SQLError);
         break;
      case odbc_SQLSvc_ExtractLob_SQLInvalidhandle_exn_:
         throwSQLException(jenv, MODULE_ERROR, exception.u.ParamError.ParamDesc, "HY000");
         break;
      case odbc_SQLSvc_ExtractLob_InvalidConnect_exn_:
      default:
         throwSQLException(jenv, PROGRAMMING_ERROR, NULL, "HY000", exception.exception_nr);
         break;
   }
   return -1;
}
