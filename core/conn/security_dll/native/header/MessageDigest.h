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

#ifndef messagedigesth
#define messagedigesth

#include "securityException.h"

#ifdef _WINDOWS
class __declspec( dllexport ) MessageDigest
#else 
class MessageDigest
#endif
{
   public:
      //Ctor
      MessageDigest();
      ~MessageDigest();

      // Computes the message authentication code of the md_len bytes 
      // at md using the hash function evp_md and the key which 
      // is key_len bytes long.
      // key - The key to use for HMAC digest 
      // key_len - Session key length 20 bytes
      // data - The data used to create the digest 
      // data_len - Length of the data to be digested
      // md - returns the pointer to the digested message
      // mdLen - returns the pointer to the digested message's length
      static void digest(const char *key, const int key_len,
                          const char *data, const int data_len,
                          unsigned char *md, unsigned int *mdLen)
						  throw (SecurityException); 

   private:
};

#endif
