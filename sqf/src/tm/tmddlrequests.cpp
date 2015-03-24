// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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

#include "tmddlrequests.h"
#include "dtm/tm.h"
#include <string.h>
#include <iostream>
using namespace std;

/*
* Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
* Method:    createTableReq
* Signature: ([B)V
*/

JNIEXPORT void JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_createTableReq
  (JNIEnv *pp_env, jobject pv_object, jbyteArray pv_tableDescriptor){

   short lv_ret;
   char la_tbldesc[TM_MAX_DDLREQUEST_STRING];

   int lv_tbldesc_length = pp_env->GetArrayLength(pv_tableDescriptor);
   memset(la_tbldesc, 0, lv_tbldesc_length);

   jbyte *lp_tbldesc = pp_env->GetByteArrayElements(pv_tableDescriptor, 0);

   memcpy(la_tbldesc, lp_tbldesc, lv_tbldesc_length);
   //cout << "JAVATOCPP CREATETABLEREQ"<< endl;
    
   lv_ret = CREATETABLE(la_tbldesc, lv_tbldesc_length);
   pp_env->ReleaseByteArrayElements(pv_tableDescriptor, lp_tbldesc, 0);

}
