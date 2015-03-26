//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

#include <jni.h>

#ifndef _IDTMJNI_H_
#define _IDTMJNI_H_

extern "C" {
/*
 * Class:     org_apache_hadoop_hbase_regionserver_transactional_IdTm
 * Method:    native_id
 * Signature: (ILorg/hp/traf/t284id;)I
 */
JNIEXPORT jint JNICALL Java_org_apache_hadoop_hbase_regionserver_transactional_IdTm_native_1id (JNIEnv *, jobject, jint, jobject);

/*
 * Class:     org_apache_hadoop_hbase_regionserver_transactional_IdTm
 * Method:    native_ping
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_apache_hadoop_hbase_regionserver_transactional_IdTm_native_1ping (JNIEnv *, jobject, jint);

/*
 * Class:     org_apache_hadoop_hbase_regionserver_transactional_IdTm
 * Method:    native_reg_hash_cb
 * Signature: (Lorg/hp/traf/t284cb;)I
 */
JNIEXPORT jint JNICALL Java_org_apache_hadoop_hbase_regionserver_transactional_IdTm_native_1reg_1hash_1cb (JNIEnv *, jobject, jobject);

}
#endif
