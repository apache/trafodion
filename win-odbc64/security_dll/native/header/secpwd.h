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
// secpwd.h 
// Interface between Security_DLL and MXOAS
// Class SecPwd provides functions to build password key, get nonce
// set nonce, get password key length, etc..

#ifndef secpwdh
#define secpwdh

#include "securityException.h"
 
class Security;

typedef struct {
    int       pin;             // Proces PIN
    int       cpu;             // Procesor
	long long   timestamp;       // process timestamp
} ProcInfo;       


#ifdef _WINDOWS
class __declspec( dllexport ) SecPwd
#else
class SecPwd
#endif
{  
   public:
      // cert_file - fully qualified certificate file name
     SecPwd(const char *dir, const char* certFile, const char* activeCertFile, const char* serverName, int serverNameLength)
         throw (SecurityException);
      ~SecPwd();

     /**
	  * Open and load the certificate the certificate.
	  * If we open the certificate in the constructor and it fails, the object itself is no longer valid.
      * Since all the logic requires a SecPwd object, open certificate has to be a separate call.
	  */
     void openCertificate() throw (SecurityException);

      // Encrypts session key, nonce and password.  Builds password key
      // to return to caller
      // pwd - password to be encrypted
      // pwdLen - Length of the password 
      // rolename - role name to build password key. optional 
	  //            pass NULL if not needed
      // rolenameLen - Length of the rolename or 0
      // procInfo - 20 bytes of Process information in this order pin (2 bytes), 
      //            cpu (2 bytes), sequence name (8 bytes), and TS (8 bytes) 
      // procInfoLen - Length of the procInfo 
      // pwdkey - returns pwdkey  
      // pwdkeyLen - returned password key length in pwedkeyLen
      void encryptPwd(const char *pwd, const int pwdLen,
                        const char *rolename, const int rolenameLen,
                        const char *procInfo, const int procInfoLen,
                        char *pwdkey, int *pwdkeyLen) throw (SecurityException);


      // Get length of buffer for password encryption 
	  // Returns pass word key length if successed and 0 if failed
      int getPwdEBufferLen()throw (SecurityException);

      // Get Certificate expiration date
      // Returns string in YYMMDDHHMMSS format such as 
      // 100304224356 for March 04 2010 22:43:56 PM
      unsigned char* getCertExpDate();

      // Get length of buffer for data encryption 
      // dataLen - the length of the data to be encrypted
     // int getDataEBuffer(int dataLen);

     /**
      * When autodownload is on, client will download the certificate from server
      * when there is no certificate or certificate is stale.
      */
     void switchCertificate(char* cert, int len) throw (SecurityException);

	 /**
      * When the active certificate becomes stale and autodownload is false, we
      * need to check if there is a new certificate we could use.
      *
      * return 0 if different certificate is found and we have switched to the new certificate.
      * return 1 if there is no certificate to switch to.
      */
     int switchCertificate() throw (SecurityException);

     // The following two functions are added for regression test purpose.
     char* getCertFile();

     char* getActiveCertFile();

   private:
      // Ctors
	   SecPwd() {};

     Security * m_sec;
     char *certFile;
     char* activeCertFile;
};

#endif
