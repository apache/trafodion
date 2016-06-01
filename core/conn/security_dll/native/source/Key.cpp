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

#include "Key.h"
#include "StaticLocking.h"

Key::Key()
{
   m_key = NULL;
   //m_kType = PUBLIC;
   m_keyLen = 0;
}

Key::~Key() 
{
	if (m_key)
		EVP_PKEY_free(m_key);
}

void Key::getPubKeyFromFile(const char *cert_file) throw (SecurityException)
{
   X509* cert = openCertFile(cert_file);
   
   if (!cert)
       throw SecurityException(ERR_OPEN_INPUT_FILE, (char*)cert_file);

   //Get expiration date of the certificate
   strncpy((char*) m_certExpDate, (char*) (X509_get_notAfter(cert)->data), EXPDATESIZE);
   m_certExpDate[EXPDATESIZE] = '\0';

   m_key = X509_extract_key(cert);

   if (!m_key)
   {
     X509_free(cert);
	 throw SecurityException(ERR_RETRIEVE_KEY_FROM_FILE, (char* )cert_file);
   }

//   m_kType = PUBLIC;
   m_keyLen = EVP_PKEY_size(m_key);
   X509_free(cert);
}

X509* Key::openCertFile(const char* certFile) throw (SecurityException)
{
   if (!certFile)
   {
	   throw SecurityException(INPUT_PARAMETER_IS_NULL, " - certFile.");
	   return NULL;
   }

   FILE* inFile=fopen(certFile, "r");
   if (!inFile)
   {
	   throw SecurityException(FILE_NOTFOUND, (char* )certFile);
	   return NULL;
   }

   getMutex();
   X509 *certificate = PEM_read_X509(inFile, NULL, 0, NULL);
   releaseMutex();
   if (!certificate)
   {
	   fclose(inFile);
	   throw SecurityException(ERR_READ_CERT_FILE, (char* )certFile);
	   return NULL;
   }  

   fclose(inFile);

   return certificate;
}

EVP_PKEY* Key::getPrivKeyFromFile(const char* inFile,
								  EVP_PKEY *priv_key) throw (SecurityException)
{
   if (!inFile)
	 throw SecurityException(INPUT_PARAMETER_IS_NULL, " - inFile.");

   FILE* file=fopen(inFile, "r");
   if (!file)
      throw SecurityException(FILE_NOTFOUND, (char *)inFile);

   EVP_PKEY* key = PEM_read_PrivateKey(file, &priv_key, 0, NULL);

   fclose (file);
   if (!key)
      throw SecurityException(ERR_RETRIEVE_KEY_FROM_FILE, (char *)inFile);

   return key;
}

unsigned char* Key::getCertExpDate(const char* certFile) throw (SecurityException)
{
   X509* cert = openCertFile(certFile);
   
   if (!cert)
	   throw SecurityException(ERR_OPEN_INPUT_FILE, " - cert.");

   unsigned char* date = X509_get_notAfter(cert)->data;

   X509_free(cert);

   return date;
}


EVP_PKEY* Key::getKey()
{
   return m_key;
}

void Key::setKey(EVP_PKEY* key)
{
	m_key = key;
}

int Key::getKeyLen() 
{
   return m_keyLen;
}

unsigned char* Key::getCertExpDate() 
{
   return m_certExpDate;
}

