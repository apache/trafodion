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

#include "security.h"
#include "Key.h"
#include "utils.h"
#include "Cipher.h"
#include "MessageDigest.h"
#include "securityException.h"
#include "openssl/rand.h"
#include "openssl/bio.h"
#include "StaticLocking.h"
#include "secdefsCommon.h"
#ifndef _WINDOWS
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#else // _WINDOWS
#include <Wincrypt.h>
#include <windows.h>
#endif

#define SEED_SIZE 256 
//Used when /dev/urandom file does not exist on the system
#define WEAK_SEED_SIZE 64

#ifdef MXHPUXPA
static unsigned int seedGenerator(char* seed);
static int shellCommand(char* command, char* result, long dataLen);
#endif


// Constructors
Security::Security() 
{
   m_pwdKey.data.password = NULL;
   m_keyObj = NULL;
   m_DataEncryptionBuffer = NULL;
}

Security::~Security() 
{
	if (m_keyObj)
	{
		delete m_keyObj;
		m_keyObj = NULL;
	}
	if(m_DataEncryptionBuffer != NULL)
		delete[] m_DataEncryptionBuffer;
}


Security::Security(const char *cert_file) throw (SecurityException)
{

   if (!cert_file) 
      throw SecurityException(INPUT_PARAMETER_IS_NULL, " - cert_file.");

   m_littleEndian = is_little_endian();
   m_pwdKey.data.password = NULL;
   
/*   if (m_keyObj)
	{
		delete m_keyObj;
		 m_keyObj = NULL;
	}
*/
   m_keyObj = new Key();
 
   m_pwdKey.id[0] = '\1';
   m_pwdKey.id[1] = '\2';
   m_pwdKey.id[2] = '\3';
   m_pwdKey.id[3] = '\4';

   // Initialize rolename with 128 ' '
   memset(&m_pwdKey.rolename[0],'\0', ROLENAME_SIZE);

   try {
      m_keyObj->getPubKeyFromFile(cert_file);
      generateSessionKey();
   }catch (SecurityException se) {
	   m_keyObj->Key::~Key();
	   throw se;
   }
   m_DataEncryptionBuffer = NULL;
}

void Security::generateSessionKey() throw (SecurityException)            
{
   int seedSize; 
   unsigned char seed[SEED_SIZE];

#ifndef _WINDOWS
   // Reads 256 bytes from  file /dev/urandom and adds them to 
   // PRNG to use as seed of the random number generator 
   int fd = open("/dev/urandom", O_RDONLY);
   if (fd < 0)
#ifdef MXHPUXPA  // HP-UX PA-RISC
   // /dev/urandom file does not exist on HP-UX PA-RISK.  If customers
   // do not choose to install the HP-UX Strong Random Number Generator 
   // package which can be obtained from this link, provide the following
   // weak Random Number Generator  
   {
      seedSize = WEAK_SEED_SIZE;
      if (seedGenerator((char*)seed) != seedSize)
         throw SecurityException(UNABLE_TO_SEED_RNG, NULL);
   }
#else  // other UNIX platforms
	   throw SecurityException(UNABLE_TO_SEED_RNG, NULL);
   seedSize = SEED_SIZE;
   int num = 0;

   // This can infinitely loops if /dev/urandom does not return 256 bytes 
   // However, it's very very unlikely to happen.
   while (num != seedSize)
   {
      int count = 0;
      count = read(fd, &seed[num], seedSize - num);
	  if (count > 0)
		  num += count;
      else if (count == 0)
	  {
		 close(fd);
		 throw SecurityException(FAILED_GENERATE_RANDOM_NUM, NULL);
	  }
	  else //count < 0
	  {
         switch (errno)
         {
#ifdef EINTR
            case EINTR: // Interrupted system call 
#endif
#ifdef EAGAIN
            case EAGAIN:  // Resource temporarily unavailable 
#endif
               /* No error, try again */
               break;
            default:
			   close(fd);
		       throw SecurityException(FAILED_GENERATE_RANDOM_NUM, NULL);
          }
       }
   }
	close(fd);
#endif //MXHPUXPA
#else // _WINDOWS
        seedSize=SEED_SIZE;
	// Get a handle to the DLL module.
	HINSTANCE hLib = LoadLibrary(TEXT("advapi32.dll")); 
	// Do the following to avoid the overhead of loading the entire CryptoAPI
	if (hLib) {
		BOOLEAN (APIENTRY *pfn)(void*, ULONG) = 
		(BOOLEAN (APIENTRY *)(void*,ULONG))GetProcAddress(hLib, "SystemFunction036");
		if (pfn) {
			if(!pfn(seed, seedSize)) {
		       throw SecurityException(UNABLE_TO_SEED_RNG, NULL); 
			}
		}
	}
	else {
		throw SecurityException(FAILED_LOADING_LIB, NULL);
	}
#endif  // _WINDOWS
   getMutex();
   RAND_seed((const void *)seed, seedSize);
   // Check if the PRNG has been seeded with enough randomness
   if (!RAND_status ())
   {
	   releaseMutex();
		throw SecurityException(UNABLE_TO_SEED_RNG, NULL);
   }
   // Generate 64 bytes of random number using the seed generated above
   int errcode = RAND_bytes((unsigned char*) &m_pwdKey.data.session_key[0], 
                                 SESSION_KEYLEN + NONCE_SIZE);
   releaseMutex();

   if (errcode != 1)
      throw SecurityException(SESSION_KEY_GENERATION_FAILED, NULL);

   // Get nonce sequence 
   memcpy((char*) &m_nonceSeq, &m_pwdKey.data.nonce[NONCE_RANDOM], NONCE_SEQNUM); 
   if (!m_littleEndian)
   {
       m_nonceSeq = swapByteOrderOfLL(m_nonceSeq);
	   // Store the swap bytes back in nonce
	   //intLLong.llVal = m_nonceSeq;
	   memcpy(&m_pwdKey.data.nonce[NONCE_RANDOM], (char*) &m_nonceSeq, NONCE_SEQNUM);
   }

   // Set time when session key is generated
   m_keyTime = get_msts();
}

void Security::encryptPwd( const char *pwd, const int pwdLen,
                           const char *rolename, const int rolenameLen,
                           const char *procInfo, const int procInfoLen,
                           char *pwdkey, int *pwdkeyLen) 
                           throw (SecurityException) 
{
   // Get public key length
   int pubKeyLen = m_keyObj->getKeyLen();
   int maxPlainTextLen = pubKeyLen - UNUSEDBYTES;

   // Password + nonce + session key can't be longer than the public
   // key's length
   if ((NONCE_SIZE + SESSION_KEYLEN + pwdLen) > maxPlainTextLen)
     throw SecurityException(PWD_LENGTH_TOO_LONG, NULL);

   // Copy template with id etc
   memcpy(pwdkey, (char *)&m_pwdKey, PWDKEY_SIZE_LESS_LOGINDATA);

   // Null terminater is not needed when copying rolename
   // If rolename is longer than 128 characters, it will be truncated 
   if (rolename)
   {
      if (rolenameLen > ROLENAME_SIZE)
         memcpy(&pwdkey[PWDID_SIZE], rolename, ROLENAME_SIZE);
	  else
         memcpy(&pwdkey[PWDID_SIZE], rolename, rolenameLen);
   }
   // Copy 12 bytes of procInfo and 8 bytes of timestamp to password key
   // store procInfo in the digest starting from digest[20] 
   memcpy(&pwdkey[PWDID_SIZE + ROLENAME_SIZE + DIGEST_LENGTH - PROCINFO_SIZE], 
                   &procInfo[0], (PROCINFO_SIZE + TIMESTAMP_SIZE));

   char * to_encrypt = new char[SESSION_KEYLEN + NONCE_SIZE + pwdLen];

   memcpy((char *)to_encrypt, (char *)&m_pwdKey.data, 
                                  SESSION_KEYLEN + NONCE_SIZE);
   // Copy password to encrypt
   memcpy((char *)&to_encrypt[SESSION_KEYLEN + NONCE_SIZE], pwd, pwdLen);

   int plainTextLen = SESSION_KEYLEN + NONCE_SIZE + pwdLen;
   int cipherTextLen = 0;

   try { 
	   cipherTextLen = Cipher::encrypt((const unsigned char*) to_encrypt, plainTextLen,
             (unsigned char *)&pwdkey[PWDKEY_SIZE_LESS_LOGINDATA], m_keyObj, NULL, 0);
      if (cipherTextLen != pubKeyLen)
          throw SecurityException(CIPHER_TEXT_LEN_NOT_EQUAL_KEY_LEN, NULL); 
   } catch (SecurityException se) {
	      delete to_encrypt;
          throw se;
   }

   delete to_encrypt;

   // Create digest
   unsigned int md_len;
   int dglen = PROCINFO_SIZE + TIMESTAMP_SIZE + pubKeyLen; 

   try {
	   MessageDigest::digest(&m_pwdKey.data.session_key[0], SESSION_KEYLEN, 
              (const char*) &pwdkey[PWDID_SIZE + ROLENAME_SIZE + DIGEST_LENGTH - PROCINFO_SIZE],
              dglen, (unsigned char *) &pwdkey[PWDID_SIZE + ROLENAME_SIZE], &md_len);

      if (md_len != DIGEST_LENGTH)
         throw SecurityException(BAD_MESSAGE_DIGEST_LEN, NULL); 
   } catch (SecurityException se) { 
      throw se;
   } 

   *pwdkeyLen = PWDKEY_SIZE_LESS_LOGINDATA + pubKeyLen;
} 



int Security::getPwdEBufferLen() throw (SecurityException) 
{
   int pubKLen = m_keyObj->getKeyLen(); 

   if (pubKLen != 0)
       return (pubKLen + PWDKEY_SIZE_LESS_LOGINDATA);
   else
       throw SecurityException(PUBKEY_LENGTH_IS_ZERO, NULL);
}

unsigned char* Security::getCertExpDate()
{
   return m_keyObj->getCertExpDate();
}

int Security::getLittleEndian()
{
	return m_littleEndian;
}

#ifdef MXHPUXPA
#define DATA_MAX 1024

static unsigned int seedGenerator(char* seed)
{
   char buffer[DATA_MAX + DATA_MAX/2];
   unsigned long long ts = 0;
   unsigned int mdLen = 0;
   char md[WEAK_SEED_SIZE+1];
   char key[WEAK_SEED_SIZE];

   // Get process ID
   pid_t myPID = getpid();
   if (myPID == -1)
      return mdLen;
  
   // Get current timestamp
   ts = swapByteOrderOfLL(get_msts());

   // Get 512 bytes output from the finger command 
   if (shellCommand("finger |sort -r -k5,5", &buffer[0], DATA_MAX/2) == 0)
      return mdLen;

   // reverse the ps command output based on timestamp hoping the first
   // 1024 bytes would be as fresh as possible
   if (shellCommand("ps -ef |sort -r  -k5,5|cut -c 10-100",
                   &buffer[DATA_MAX/2], DATA_MAX) == 0)
      return mdLen;

   // HMAC buffer using pid + the last 52 bytes of buffer + ts as key
   memcpy(&key[0], (char*)&myPID, 4);
   memcpy(&key[4], &buffer[DATA_MAX + DATA_MAX/2 - 52], 52);
   memcpy(&key[56], (char*)&ts,8);
   const EVP_MD *evp_md = EVP_sha256();
   HMAC(evp_md, (const void *) key, WEAK_SEED_SIZE,
                       (const unsigned char *) buffer,
                       DATA_MAX + DATA_MAX/2,
                       (unsigned char *) &md[0], &mdLen);

   if (mdLen != 32)
      return 0;
   mdLen = 0; //refresh
 
   // HMAC again to get the other 32 bytes using the prvious 
   // HMAC result as key and the previous 64 bytes key as data 
   HMAC(evp_md, (const void *) &md[0], 32,
                 (const unsigned char *) key, WEAK_SEED_SIZE,
                 (unsigned char *) &md[32], &mdLen);
   if (mdLen != 32)
      return 0;

   return (2 * mdLen);
}

static int shellCommand(char* command, char* result, long dataLen)
{
   FILE *fp;
   int status;
   int len=0;
   char buffer[DATA_MAX + 1];

   if ((fp = popen(command, "r")) != NULL)
   {
      // Want dataLen bytes but fgets dataLen - 1 bytes so add one to avoid
      // a second get 
      while ( fgets( buffer, dataLen + 1, fp))
      {
         // You might not get that dataLen bytes from the command output 
         // but it is alright since junk in the buffer can be used as 
         // entropies also.
         int tmpLen = strlen(buffer);

         if (len >= dataLen)
            break;
         // output is less than expected bytes
         else if (tmpLen > dataLen - len)
         {
            memcpy(&result[len], buffer, dataLen - len);
            len += dataLen - len;
            break; //enough bytes
         }
         else
         {
            memcpy(&result[len], buffer,tmpLen);
            len += tmpLen;
         }
      }

   }
   else
      return 0;

   status = pclose(fp);
   // -1 if stream is not associated with a popen()ed command
   // 127 sh could not be executed for some reason
   // This stops the program, should we? No, just ignore.
   if (status == -1 || status == 127) 
      return 1; 

   return 1;
}
#endif //HPUXPA

