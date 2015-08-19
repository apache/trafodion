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
//
**********************************************************************/

#include "MessageDigest.h"
#include "Key.h"

#include "openssl/pem.h"
#include "secdefsCommon.h"
#include "StaticLocking.h"

MessageDigest::MessageDigest() {}

MessageDigest::~MessageDigest() {}

void MessageDigest::digest(const char *key, const int key_len,
                           const char *data, const int data_len,
                           unsigned char *md, unsigned int *mdLen)
               throw (SecurityException) 
{
   if (!key)
      throw SecurityException(INPUT_PARAMETER_IS_NULL, " - key.");
   if (!data)
      throw SecurityException(INPUT_PARAMETER_IS_NULL, " - data.");
   if (!md)
      throw SecurityException(INPUT_PARAMETER_IS_NULL, " - md.");


   getMutex();
   const EVP_MD *evp_md = EVP_sha256();
   HMAC(evp_md, (const void *) key, key_len,
                          (const unsigned char *) data, data_len,
                          (unsigned char *) &md[0], mdLen);
   releaseMutex();
}
