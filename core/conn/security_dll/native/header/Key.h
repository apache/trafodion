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

#ifndef keyh
#define keyh

#include "openssl/evp.h"
#include "openssl/ossl_typ.h"
#include "openssl/rsa.h"
#include "openssl/hmac.h"
#include "openssl/x509.h"
#include "openssl/x509v3.h"
#include "openssl/pem.h"
#include "secdefsCommon.h"
#include "securityException.h"

#ifdef _WINDOWS
class __declspec( dllexport ) Key
#else 
class Key 
#endif
{
   public:
      //enum KeyType{PUBLIC, PRIVATE};

      // Ctor
      // ktype - PUBLIC or PRIVATE key
	  Key();

      ~Key();

      // gets the public/private key from the certificate file
	  // Sets m_keyLen and m_certExpDate
      // keyFile - Certificate file contains the key
      void getPubKeyFromFile(const char *keyFile) throw (SecurityException); 

      // gets the private key from the input file
      // inFile - input file contains the key
	  // priv_key - points to key storage addr
      // Returs pointer to the ENV_PKEY structure in which the key is stored 
      static EVP_PKEY* getPrivKeyFromFile(const char *inFile,
		                         EVP_PKEY *priv_key) throw (SecurityException); 

      // returns key
      EVP_PKEY *getKey();

	  // Set m_key
	  void setKey(EVP_PKEY* key);

      // returns the length of the public key 
      int getKeyLen();

      // returns type of key
	  //Key::KeyType getKType();

	  // get m_certExpDate
	  unsigned char* getCertExpDate();

	  // get expiration date fron the certificate (used by server code)
	  // certFile - fully qualified certificate file name
	  // Returns string in YYMMDDHHMMSSZ format 
      static unsigned char* getCertExpDate(const char* certFile) throw (SecurityException);

   private:
	  // open the certificate file
	  // cert_file - certificate file name
	  // returns the certificate
      static X509* openCertFile(const char* cert_file)  throw (SecurityException);

      EVP_PKEY *m_key;
	  //Key::KeyType m_kType;
	  int m_keyLen;
	  unsigned char m_certExpDate[EXPDATESIZE + 1]; 
};
#endif
