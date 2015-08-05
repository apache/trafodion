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

#ifdef _WINDOWS
#include "openssl/applink.c"
#endif
#include "Cipher.h"

#include "StaticLocking.h"

Cipher::Cipher(){} 

Cipher::~Cipher() {}

int Cipher::encrypt(const unsigned char* plainText, const int plainTextLen,
                unsigned char* cipherText,
                Key * key, const char * iv,
                const int ivLen) throw (SecurityException)
{
   int len = 0;

   if (iv == NULL) //Public key encryption
   {
      if(!cipherText)
         throw SecurityException(INPUT_PARAMETER_IS_NULL, " - cipherText");
      if(!plainText)
         throw SecurityException(INPUT_PARAMETER_IS_NULL, " - plainText");
      if(!key)
         throw SecurityException(INPUT_PARAMETER_IS_NULL, " - key");

      getMutex();
      len = RSA_public_encrypt(plainTextLen, plainText, cipherText, 
                                  (key->getKey())->pkey.rsa,
                                  RSA_PKCS1_PADDING); 
	  releaseMutex();
   }
   
   return len;
}

int Cipher::decrypt(const unsigned char* cipherText, const int cipherTextLen,
            unsigned char* plainText,
            Key * key, const char * iv,
            const int ivLen) throw (SecurityException) 
{
   int dlen;

   if (iv == NULL) //private key decryption
   {
      if(!cipherText)
         throw SecurityException(INPUT_PARAMETER_IS_NULL, " - cipherText");
      if(!plainText)
         throw SecurityException(INPUT_PARAMETER_IS_NULL, " - plainText");
      if(!key)
         throw SecurityException(INPUT_PARAMETER_IS_NULL, " - key");

      // Decrypt
	  getMutex();
      int len = RSA_size((key->getKey())->pkey.rsa);
      dlen = RSA_private_decrypt(len, cipherText, plainText, 
                               (key->getKey())->pkey.rsa, RSA_PKCS1_PADDING);
      releaseMutex();
   }

   return dlen;
}



