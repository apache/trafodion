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
// security.h
// The actual implemantation of functions in the wrapper class 
// SecPwd.
#ifndef securityh
#define securityh


#ifdef _WINDOWS
#define _CRT_RAND_S
#endif

#include "secdefsCommon.h"
#include "securityException.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class Key;

#ifdef _WINDOWS
class __declspec( dllexport ) Security
#else 
class Security
#endif
{  
   public:
      // Ctor
      Security();

      // cert_file - fully qualified name of certificate file
      Security(const char *cert_file) throw (SecurityException); 

      ~Security();

      // Encrypts session key, nonce and password.  Builds password key
      // to return to caller
      // pwd - password to be encrypted
      // pwdLen - Length of the password
      // rolename - role name to build password key
      // rolenameLen - Length or the rolename
      // procInfo - process information (PIN, CPU, segment name and time stamp)
      // procInfoLen - Length of the process info (20 bytes)
      // pwdkey - returns pwdkey
      // pwdkeyLen - returned password key length in pwedkeyLen
      void encryptPwd(const char *pwd, const int pwdLen,
                        const char *rolename, const int rolenameLen,
                        const char *proInfo, const int procInfoLen,
                        char *pwdkey, int *pwdkeyLen) 
               throw (SecurityException); 


      // Get length of buffer for password encryption (public)
	  // Returns pass word key length if successed and 0 if failed
      int getPwdEBufferLen() throw (SecurityException); 

	  // Gets certificate's expiration date
	  // Returns pointer to string represents the certificate's expiration day
	  // in the format YYMMDDHHMMSSZ
      unsigned char* getCertExpDate();

      // Get length of buffer for data encryption (symmetric)
      // dataLen - the length of the data to be encrypted
      //int getDataEBuffer(int dataLen);

	  int getLittleEndian();

   private:
      // encryption is required or not for replied message 0-yes, 1-no
      //int m_encrypted;
	
      // Generates session key and nonce
      void generateSessionKey() throw (SecurityException); 

      // security option - mandatory - 1 or undocumented option - 0
      bool m_security;
      // Time when session key is generated
      unsigned long long m_keyTime;
      // Neo system name
      unsigned long long m_nonceSeq;  // Need to use 64 bit number type here
      // Processor architecture of the client system 1 - little Endian 2 - big Endian
      int m_littleEndian;
      // key
      Key *m_keyObj;
      // password key
      PwdKey m_pwdKey;
	  unsigned char *m_DataEncryptionBuffer;
};

#endif
