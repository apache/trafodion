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
#ifndef cipherh
#define cipherh

#include "stdio.h"
#include "stdlib.h"
#include "securityException.h"
#include "Key.h"

class Key;

#ifdef _WINDOWS
class __declspec( dllexport ) Cipher
#else 
class  Cipher
#endif
{
   public:
   // Ctor

   Cipher();
   ~Cipher();

   // Encrypts plainTextLen bytes of plain text and stores the 
   // cipher text in cipherText using public key or symmetric key
   // plainText - plain text to be encrypted
   // plainTextLen - length of plain text
   // cipherText - encrypted plain text is returned
   // key - password encryption: public or private key
   //       message encryption: session key 
   // iv - 8 sequence nonce is used as the initial vector for symmetric 
   //      key encryption.  Null if is public key encryption 
   // ivLen - Length of iv
   // Returns the length of the cipher text
   static int encrypt(const unsigned char* plainText, const int plainTextLen,
               unsigned char* cipherText,
               Key * key, const char * iv, 
               const int ivLen) throw (SecurityException); 

   // Decrypts cipherTextLen bytes of plain text and stores the 
   // cipher text in plainText using private key or symmetric key
   // cipherText - cipher text to be decrypted
   // cipherTextLen - length of cipher text
   // plainText - decrypted cipher text is returned
   // key - password encryption: public or private key
   //       message encryption: session key 
   // iv - 8 sequence nonce is used as the initial vector for symmetric 
   //      key encryption.  Null if is private key encryption 
   // ivLen - Length of iv
   // Returns the length of the plain text or -1 in case of error
   static int decrypt(const unsigned char* cipherText, const int cipherTextLen,
               unsigned char* plainText, 
               Key* key, const char * iv, 
               const int ivLen)throw (SecurityException);

};

#endif
