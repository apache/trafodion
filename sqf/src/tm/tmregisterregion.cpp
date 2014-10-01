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

#include "tmregisterregion.h"
#include "dtm/tm.h"
#include <string.h>
#include <iostream>
using namespace std;

/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    registerRegion2
 * Signature: (I[B[B)V   
 */
JNIEXPORT void JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_registerRegion
(JNIEnv *pp_env, jobject pv_object, jint pv_port, jbyteArray pv_hostname, jlong pv_startcode, jbyteArray pv_dos)
{
   short lv_ret;
   char la_hostname[TM_MAX_REGIONSERVER_STRING];
   char la_dos[TM_MAX_REGIONSERVER_STRING];
   memset(la_hostname, 0, TM_MAX_REGIONSERVER_STRING);
   memset(la_dos, 0, TM_MAX_REGIONSERVER_STRING);

   //cout << "ENTRY registerRegion JNI - passing ByteArray\n";
   int lv_hostname_length = pp_env->GetArrayLength(pv_hostname);
   jbyte *lp_hostname = pp_env->GetByteArrayElements(pv_hostname, 0);

   int lv_dos_length = pp_env->GetArrayLength(pv_dos);
   jbyte *lp_dos = pp_env->GetByteArrayElements(pv_dos, 0);

   //cout << "registerRegion 1 lp_hostname length: " << lv_hostname_length << " data: " << lp_hostname << endl;
   //cout << "registerRegion 2 lp_dos length: " << lv_dos_length << " data: " << lp_dos << endl;
   memcpy(la_hostname,
	  lp_hostname,
	  lv_hostname_length);
   memcpy(la_dos,
	  lp_dos,
	  lv_dos_length);

   lv_ret = REGISTERREGION(pv_port, la_hostname, lv_hostname_length, pv_startcode, la_dos, lv_dos_length);
   //cout << "REGISTERREGION Error: " << lv_ret << endl;
   pp_env->ReleaseByteArrayElements(pv_hostname, lp_hostname, 0);
   pp_env->ReleaseByteArrayElements(pv_dos, lp_dos, 0);

}
