/*
* @@@ START COPYRIGHT @@@                                                     
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@                                                          */

#include "tmddlrequests.h"
#include "dtm/tm.h"
#include <string.h>
#include <iostream>
#include "../../inc/fs/feerrors.h" 

using namespace std;

/*
* Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
* Method:    createTableReq
* Signature: ([B)V
*/

JNIEXPORT jstring JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_createTableReq
  (JNIEnv *pp_env, jobject pv_object, jbyteArray pv_tableDescriptor, jobjectArray pv_keys, jint pv_numSplits, jint pv_keyLength, jlong pv_transid, jbyteArray pv_tblname)
{

   char *la_err_str = 0;
   int la_err_len = 0;
   char** la_keys;
   int lv_error = FEOK;

   int lv_tbldesc_length = pp_env->GetArrayLength(pv_tableDescriptor);
   if(lv_tbldesc_length > TM_MAX_DDLREQUEST_STRING)
   {
     jstring lv_err_str = pp_env->NewStringUTF("Table Desc length is larger than max allowed");
     return lv_err_str;
   }
   jbyte *lp_tbldesc = pp_env->GetByteArrayElements(pv_tableDescriptor, 0);
   jbyte *lp_tblname = pp_env->GetByteArrayElements(pv_tblname, 0);
  
   long lv_transid = (long) pv_transid;

   // Keys for Salted Tables
   int lv_numSplits = (int) pv_numSplits;
   int lv_keyLength = (int) pv_keyLength;
   la_keys = new char *[lv_numSplits];
   
   for(int i=0; i<lv_numSplits; i++)
   {
     jbyteArray jba_keyarray = (jbyteArray)(pp_env->GetObjectArrayElement((jobjectArray)pv_keys, i));
     int lv_key_len = pp_env->GetArrayLength(jba_keyarray);
     la_keys[i] = new char[lv_key_len];
     pp_env->GetByteArrayRegion(jba_keyarray, 0, lv_key_len, (jbyte*)la_keys[i]);
     pp_env->DeleteLocalRef(jba_keyarray);
   }

   lv_error  = CREATETABLE((char*) lp_tbldesc, lv_tbldesc_length,
                           (char *)lp_tblname, la_keys, lv_numSplits,
                           lv_keyLength, lv_transid, la_err_str, la_err_len);

   pp_env->ReleaseByteArrayElements(pv_tableDescriptor, lp_tbldesc, 0);
   pp_env->ReleaseByteArrayElements(pv_tblname, lp_tblname, 0);
   
   delete [] la_keys;
   if(lv_error)
   {
     jstring lv_err_str;
     if(la_err_len && la_err_str)
     {
       lv_err_str = pp_env->NewStringUTF(la_err_str);
       
       //la_err_str is allocated in lower layers.
       delete la_err_str;
     }
     else
     {
       lv_err_str = pp_env->NewStringUTF("Create Table failed. Error is Unknown");
     }
     return lv_err_str;
   }
   else
   {
     return 0;
   }
}


/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    dropTableReq
 * Signature: ([BJ)V
 */
JNIEXPORT jstring JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_dropTableReq
  (JNIEnv *pp_env, jobject pv_object, jbyteArray pv_tblname, jlong pv_transid) {

   char *la_err_str = 0;
   int la_err_len = 0;
   int lv_error = FEOK;

   int lv_tblname_len = pp_env->GetArrayLength(pv_tblname);
   if(lv_tblname_len > TM_MAX_DDLREQUEST_STRING)
   {
     jstring lv_err_str = pp_env->NewStringUTF("Table name length is larger than max allowed");
     return lv_err_str;
   }
   jbyte *lp_tblname = pp_env->GetByteArrayElements(pv_tblname, 0);
   long lv_transid = (long) pv_transid;
   lv_error = DROPTABLE((char*)lp_tblname, lv_tblname_len, lv_transid, la_err_str,
                         la_err_len);
   pp_env->ReleaseByteArrayElements(pv_tblname, lp_tblname, 0);
   
   if(lv_error)
   {
     jstring lv_err_str;
     if(la_err_len && la_err_str)
     {
       lv_err_str = pp_env->NewStringUTF(la_err_str);
       
       //la_err_str is allocated in lower layers.
       delete la_err_str;
     }
     else
     {
       lv_err_str = pp_env->NewStringUTF("Drop Table failed. Error is Unknown");
     }
     return lv_err_str;	   
   }
   else
   {
	 return 0;
   }
}

/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    truncateOnAbortReq
 * Signature: ([BJ)V
 */
JNIEXPORT jstring JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_truncateOnAbortReq
  (JNIEnv *pp_env, jobject pv_object, jbyteArray pv_tblname, jlong pv_transid) {

   char *la_err_str = 0;
   int la_err_len = 0;
   int lv_error = FEOK;

   int lv_tblname_len = pp_env->GetArrayLength(pv_tblname);
   if(lv_tblname_len > TM_MAX_DDLREQUEST_STRING)
   {
     jstring lv_err_str = pp_env->NewStringUTF("Table name length is larger than max allowed");
     return lv_err_str;
   }
   
   jbyte *lp_tblname = pp_env->GetByteArrayElements(pv_tblname, 0);
   long lv_transid = (long) pv_transid;

   lv_error = REGTRUNCATEONABORT((char*)lp_tblname, lv_tblname_len, lv_transid, la_err_str,
                                 la_err_len);
   pp_env->ReleaseByteArrayElements(pv_tblname, lp_tblname, 0);
   
   if(lv_error)
   {
     jstring lv_err_str;
     if(la_err_len && la_err_str)
     {
       lv_err_str = pp_env->NewStringUTF(la_err_str);
       
       //la_err_str is allocated in lower layers.
       delete la_err_str;
     }
     else
     {
       lv_err_str = pp_env->NewStringUTF("RegTruncateonAbort failed. Error is Unknown");
     }
     return lv_err_str;    
   }
   else
   {
     return 0;
   }
}

/*
 * Class:     org_apache_hadoop_hbase_client_transactional_RMInterface
 * Method:    alterTableReq
 * Signature: ([B[Ljava/lang/Object;J)V
 */
JNIEXPORT jstring JNICALL Java_org_apache_hadoop_hbase_client_transactional_RMInterface_alterTableReq
  (JNIEnv *pp_env, jobject pv_object, jbyteArray pv_tblName, jobjectArray pv_tableOptions, jlong pv_transID) {

   int lv_error = FEOK;
   int tblopts_len =0;
   char *la_err_str = 0;
   int la_err_len = 0;

   char** tbl_options = 0;
   
   int lv_tblname_len = pp_env->GetArrayLength(pv_tblName);
   if(lv_tblname_len > TM_MAX_DDLREQUEST_STRING)
   {
     jstring lv_err_str = pp_env->NewStringUTF("Table name length is larger than max allowed");
     return lv_err_str;
   }
   jbyte *lp_tblname = pp_env->GetByteArrayElements(pv_tblName, 0);
   
   int tbloptions_cnt = pp_env->GetArrayLength(pv_tableOptions);
   if(tbloptions_cnt)
     tbl_options = new char *[tbloptions_cnt];

   for (int i=0; i<tbloptions_cnt; i++) {

       //cout << " TableOptions loop " << i << endl;
       jstring jstr_options = (jstring) pp_env->GetObjectArrayElement(pv_tableOptions, i);
       const char *str_options = pp_env->GetStringUTFChars(jstr_options, 0);
       // Don't forget to call `ReleaseStringUTFChars` when you're done.
       
       //int str_opts_len = length(jstr_options);
       //int str_opts_len = pp_env->GetStringUTFLength(jstr_options);
       int str_opts_len = sizeof(str_options)/sizeof(*str_options);
       //cout << "str_opts_len: " << str_opts_len <<  " or " << sizeof(str_options)/sizeof(*str_options) << endl;

       tbl_options[i] = new char[tbloptions_cnt];
       memcpy(tbl_options[i], str_options, str_opts_len);

       pp_env->ReleaseStringUTFChars(jstr_options, str_options);

       if(tblopts_len == 0)
          tblopts_len = str_opts_len;
   }

   long lv_transid = (long) pv_transID;
   lv_error = ALTERTABLE((char*)lp_tblname, lv_tblname_len, tbl_options, 
                          tblopts_len, tbloptions_cnt, lv_transid,
                          la_err_str, la_err_len);
   pp_env->ReleaseByteArrayElements(pv_tblName, lp_tblname, 0);
   if(tbl_options)
     delete [] tbl_options;
   if(lv_error)
   {
     jstring lv_err_str;
     if(la_err_len && la_err_str)
     {
       lv_err_str = pp_env->NewStringUTF(la_err_str);
       
       //la_err_str is allocated in lower layers.
       delete la_err_str;
     }
     else
     {
       lv_err_str = pp_env->NewStringUTF("Alter Table failed. Error is Unknown");
     }
     return lv_err_str;    
   }
   else
   {
     return 0;
   }
}
 
