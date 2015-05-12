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
  (JNIEnv *pp_env, jobject pv_object, jbyteArray pv_tableDescriptor, jobjectArray pv_keys, jint pv_numSplits, jint pv_keyLength, jlong pv_transid, jbyteArray pv_tblname){

   short lv_ret;
   char la_tbldesc[TM_MAX_DDLREQUEST_STRING];
   char la_tblname[TM_MAX_DDLREQUEST_STRING];
   char* str_key;
   str_key = new char[TM_MAX_DDLREQUEST_STRING];
   char** la_keys;
   la_keys = new char *[TM_MAX_DDLREQUEST_STRING];

   int lv_tblname_len = pp_env->GetArrayLength(pv_tblname);
   if(lv_tblname_len > TM_MAX_DDLREQUEST_STRING) {
      cout << "Table name length is larger than max allowed" << endl;
   }
   else {
      int lv_tbldesc_length = pp_env->GetArrayLength(pv_tableDescriptor);
      memset(la_tbldesc, 0, lv_tbldesc_length);
      jbyte *lp_tbldesc = pp_env->GetByteArrayElements(pv_tableDescriptor, 0);
      memcpy(la_tbldesc, lp_tbldesc, lv_tbldesc_length);

      memset(la_tblname, 0, lv_tblname_len < TM_MAX_DDLREQUEST_STRING ? lv_tblname_len : TM_MAX_DDLREQUEST_STRING);
      jbyte *lp_tblname = pp_env->GetByteArrayElements(pv_tblname, 0);
      memcpy(la_tblname, lp_tblname, lv_tblname_len < TM_MAX_DDLREQUEST_STRING ? lv_tblname_len : TM_MAX_DDLREQUEST_STRING -1 );

      long lv_transid = (long) pv_transid;

      // Keys for Salted Tables
      int lv_numSplits = (int) pv_numSplits;
      int lv_keyLength = (int) pv_keyLength;

      for(int i=0; i<lv_numSplits; i++)
      {
         jbyteArray jba_keyarray = (jbyteArray)(pp_env->GetObjectArrayElement((jobjectArray)pv_keys, i));
         int lv_key_len = pp_env->GetArrayLength(jba_keyarray);
         pp_env->GetByteArrayRegion(jba_keyarray, 0, lv_key_len, (jbyte*)str_key);

         la_keys[i] = new char[lv_key_len];
         memcpy(la_keys[i], str_key, lv_key_len);

         pp_env->DeleteLocalRef(jba_keyarray);
      }

      lv_ret = CREATETABLE(la_tbldesc, lv_tbldesc_length, la_tblname, la_keys, lv_numSplits, lv_keyLength, lv_transid);

      pp_env->ReleaseByteArrayElements(pv_tableDescriptor, lp_tbldesc, 0);
      pp_env->ReleaseByteArrayElements(pv_tblname, lp_tblname, 0);
   }   
}


/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    dropTableReq
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_dropTableReq
  (JNIEnv *pp_env, jobject pv_object, jbyteArray pv_tblname, jlong pv_transid) {

   short lv_ret; 
   char la_tblname[TM_MAX_DDLREQUEST_STRING];

   int lv_tblname_len = pp_env->GetArrayLength(pv_tblname);
   if(lv_tblname_len > TM_MAX_DDLREQUEST_STRING) {
      cout << "Table name length is larger than max allowed" << endl;
   }
   else {
      memset(la_tblname, 0, lv_tblname_len < TM_MAX_DDLREQUEST_STRING ? lv_tblname_len : TM_MAX_DDLREQUEST_STRING);
      jbyte *lp_tblname = pp_env->GetByteArrayElements(pv_tblname, 0);
      memcpy(la_tblname, lp_tblname, lv_tblname_len < TM_MAX_DDLREQUEST_STRING ? lv_tblname_len : TM_MAX_DDLREQUEST_STRING -1 );

      long lv_transid = (long) pv_transid;

      lv_ret = DROPTABLE(la_tblname, lv_tblname_len, lv_transid);
      pp_env->ReleaseByteArrayElements(pv_tblname, lp_tblname, 0);
   }
}

/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    truncateOnAbortReq
 * Signature: ([BJ)V
 */
JNIEXPORT void JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_truncateOnAbortReq
  (JNIEnv *pp_env, jobject pv_object, jbyteArray pv_tblname, jlong pv_transid) {

   short lv_ret;
   char la_tblname[TM_MAX_DDLREQUEST_STRING];

   int lv_tblname_len = pp_env->GetArrayLength(pv_tblname);
   if(lv_tblname_len > TM_MAX_DDLREQUEST_STRING) {
      cout << "Table name length is larger than max allowed" << endl;
   }
   else {
      memset(la_tblname, 0, lv_tblname_len < TM_MAX_DDLREQUEST_STRING ? lv_tblname_len : TM_MAX_DDLREQUEST_STRING);
      jbyte *lp_tblname = pp_env->GetByteArrayElements(pv_tblname, 0);
      memcpy(la_tblname, lp_tblname, lv_tblname_len < TM_MAX_DDLREQUEST_STRING ? lv_tblname_len : TM_MAX_DDLREQUEST_STRING -1 );

      long lv_transid = (long) pv_transid;

      lv_ret = REGTRUNCATEONABORT(la_tblname, lv_tblname_len, lv_transid);
      pp_env->ReleaseByteArrayElements(pv_tblname, lp_tblname, 0);
   }
}
