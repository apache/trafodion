//******************************************************************************
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
//******************************************************************************

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "openssl/bio.h"
#include "openssl/evp.h"
#include "openssl/x509.h"
#include "openssl/pkcs7.h"
#include "openssl/pem.h"
#include "seabed/ms.h"
#include "seabed/int/types.h"
#include "seabed/pctl.h"
#include "seabed/fserr.h"

#include <unistd.h>

#include "secsrvr.h"
#include "secsrvrmxo.h"

EVP_PKEY *cipher_key = NULL;                     // Private Key
char privkey_file[MS_MON_MAX_PROCESS_PATH];      // Private Key File Name
char pubkey_file[MS_MON_MAX_PROCESS_PATH];       // Public Key File Name
bool setup_already_done;                         // Has the security setup finished ? Private key/Public Certificate

char cert_expdate[EXPDATESIZE_ALLOC];            // Expiry Date of the certificate
int cert_data_len;                               // Size of the certificate
char *cert_data;                                 // Certificate contents to pass to clients for download
time_t cert_timestamp;                           // last mod time of the certificate

ProcInfo proc_info;                              // Process Information used by Digest

// *****************************************************************************
// *                                                                           *
// * Function: safe_strcat                                                     *
// *                                                                           *
// * Similar to strncat, this concatenates src onto dest and guarentees not to *
// * overflow the dest buffer and promises buffer will be null terminated.     *
// * Third param is size of the src buffer to avoid having to do the math in   *
// * the calling function                                                      *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *                                                                           *
// *  <dest>                    char *                    In/Out               *
// *    target buffer for the append                                           *
// *                                                                           *
// *  <src>                     char *                    In                   *
// *    data to be appended to src                                             *
// *                                                                           *
// *  <len>                     int                       In                   *
// *    size of buffer pointed to by dest                                      *
// *                                                                           *
// * Returns:                                                                  *
// *                                                                           *
// *  not a darn thing                                                         *
// *                                                                           *
// *****************************************************************************
static void safe_strcat(char *dest, char *src, size_t bufSize)
{
   size_t srcIndex = 0;;
   size_t destIndex = strlen(dest);
   while ((src[srcIndex] != 0) && (destIndex < bufSize))
   {
     dest[destIndex++] = src[srcIndex++];
   }
   dest[bufSize-1] = 0;  // in case we couldn't copy src buffer, we still need a null
}

// *****************************************************************************
// *                                                                           *
// * Function: GenerateLoginDigest                                             *
// *                                                                           *
// * Generate a login digest to compare against the login request's digest     *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *                                                                           *
// *  <key>                     char *                    In                   *
// *    is key to use for generating the digest                                *
// *                                                                           *
// *  <data>                    char *                    In/Output            *
// *    data on which the digest is generated                                  *
// *                                                                           *
// *  <len>                     int                       In                   *
// *    length of the digest data                                              *
// *                                                                           *
// * Returns:                                                                  *
// *                                                                           *
// *  1 if failure, 0 Success                                                  *
// *                                                                           *
// *****************************************************************************
short GenerateLoginDigest(
        char *key,                      // The key to use for digest generation
        char *data,                     // The data on which the digest is generated
        int len)                        // Length of digest data
                                        // Returns the digest 20 characters in data
{
        unsigned int md_len;
        unsigned int msg_len = len;
        unsigned char fp[DIGEST_LENGTH];

        // EVP_sha256() will return the address of an SSL constant;
        // do not attempt to free it later.  See file
        // openssl-0.9.8k\crypto\evp\m_sha1.c for details.
        const EVP_MD *evp_md = EVP_sha256();

        unsigned char *auth_code;
        auth_code = HMAC(evp_md,
                         (const void *) key,
                         SESSION_KEYLEN,
                         (const unsigned char *) data,
                         (int) msg_len,
                         &fp[0],
                         &md_len);
        //Error handling, if auth_code is null it failed
        if (auth_code == NULL)
                return 1;  // failed
        memcpy(data, &fp[0], md_len);

    return 0;
}

// *****************************************************************************
// *                                                                           *
// * Function: ValidateLoginDigest                                             *
// *                                                                           *
// * Validate a login digest by comparing against the login request's digest   *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *                                                                           *
// *  <key>                     char *                    In                   *
// *    is the session key                                                     *
// *                                                                           *
// *  <proc_info>               ProcInfo *                In                   *
// *    data on which the digest is generated                                  *
// *                                                                           *
// *  <pwd_key>                 PwdKey  *                 In                   *
// *    password key sent                                                      *
// *                                                                           *
// * Returns:                                                                  *
// *                                                                           *
// *  True if success, else False                                              *
// *                                                                           *
// *****************************************************************************

bool ValidateLoginDigest(
       char *key,            // The session key
       ProcInfo *proc_info,  // Info about the process
       PwdKey *pwd_key)      // Pointer to pwd_key
{ 
    // setup the buffer to generate the message digest
    char data[sizeof(LoginData) + TIMESTAMP_SIZE + sizeof(ProcInfo)]; 
    // total message length for the digest

    int len = sizeof(ProcInfo) + TIMESTAMP_SIZE + sizeof(LoginData) - LOGIN_DATA_OVERHEAD;
    
    // The len was calculated for a 2048 bit (256 byte) key; adjust if 1024.
    int key_byte_len = RSA_size(cipher_key->pkey.rsa);
    if (key_byte_len == (1024 / 8)) {
        len -= ((2048 - 1024) / 8);
    }

    // First copy the ProcInfo
        memcpy(&data[0],  (char *) proc_info, sizeof(ProcInfo));
        //timestamp plus encrypted data
        memcpy(&data[sizeof(ProcInfo)], (char *) &pwd_key->ts[0], len - sizeof(ProcInfo));

    short ret = GenerateLoginDigest(key, &data[0], len);
    if (ret)
        return false;

        for(int i = 0; i< DIGEST_LENGTH; i++)
                if(pwd_key->digest[i] != data[i]) return false;

    return true;
}

// *****************************************************************************
// *                                                                           *
// * Function: SetupCertFilenames                                              *
// *                                                                           *
// * Based on the environment variables and default filenames, generate        *
// * the correspnding Private and Public Key filenames                         *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *  None                                                                     *
// *                                                                           *
// * Returns:                                                                  *
// *                                                                           *
// *  None                                                                     *
// *                                                                           *
// * Filenames stored in Global variables privkey_file and pubkey_file         *
// *****************************************************************************

static void SetupCertFilename()
{

   char default_priv_filename[] = "server.key";
   char default_pub_filename[] = "server.crt";
   char *priv_filename = getenv("SQCERT_PRIVKEY");
   char *pub_filename = getenv("SQCERT_PUBKEY");
   char *clustername  = getenv("CLUSTERNAME");
   char *homedir  = getenv("HOME");
   char *sqvar  = getenv("TRAF_VAR");
   char *certdir = getenv("SQCERT_DIR");

   // Get the Certificate directory and the filename
   // If the env variable SQCERT_DIR
   // For a Cluster - look

   if (certdir == NULL)
   {
      if (clustername != NULL)
      {
         strncpy (privkey_file, homedir, MS_MON_MAX_PROCESS_PATH);
         privkey_file[MS_MON_MAX_PROCESS_PATH-1] = 0;
      }

      else
      { // it's a workstation

      if (sqvar != NULL)
      {
         strncpy (privkey_file, sqvar, MS_MON_MAX_PROCESS_PATH);
         privkey_file[MS_MON_MAX_PROCESS_PATH-1] = 0;
      }
      else
         strcpy (privkey_file, "/tmp");
      }

      safe_strcat (privkey_file, (char *) "/sqcert/", MS_MON_MAX_PROCESS_PATH);
   }
   else
   {
      strncpy(privkey_file, certdir, MS_MON_MAX_PROCESS_PATH);
      privkey_file[MS_MON_MAX_PROCESS_PATH-1] == 0;
      strcat (privkey_file, "/");
   }

   // Done forming the directory location - copy the same to public file
   strcpy(pubkey_file, privkey_file);

   if (priv_filename != NULL)
      safe_strcat (privkey_file, priv_filename, MS_MON_MAX_PROCESS_PATH);  
   else
      safe_strcat (privkey_file, default_priv_filename, MS_MON_MAX_PROCESS_PATH);

   if (pub_filename != NULL)
      safe_strcat (pubkey_file, pub_filename, MS_MON_MAX_PROCESS_PATH);
   else
      safe_strcat (pubkey_file, default_pub_filename, MS_MON_MAX_PROCESS_PATH);

}


// *****************************************************************************
// *                                                                           *
// * Function: ReadPublicCertificate                                           *
// *                                                                           *
// * Reads and stores the contents of the Public certificate and extracts      *
// * the expiry date                                                           *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *  None                                                                     *
// *                                                                           *
// * Returns:                                                                  *
// *                                                                           *
// *  True if Success, else false                                              *
// *                                                                           *
// * Updates Global variables                                                  *
// *****************************************************************************
static bool ReadPublicCertificate()
{
    int ret = 0;
    struct stat file_stats;

    // Get the Certificate directory and the filename
    if (!setup_already_done)
      SetupCertFilename();

    if (0 != stat(pubkey_file, &file_stats))
      return false;   // failed to read file info

    FILE *pFile = fopen(pubkey_file, "r");

    if (pFile == NULL)
      return false;

    cert_data_len = file_stats.st_size;
    
    if (cert_data)
      delete [] cert_data;

    cert_data = new char[cert_data_len + 1];
    cert_data_len = (long) fread(cert_data, 1, cert_data_len, pFile);
    cert_data[cert_data_len] = 0;

    if ( cert_data_len != file_stats.st_size )
    {
      fclose(pFile);
      return false;
    }

    cert_timestamp = file_stats.st_mtime;

    // Extract the Expiry Date

    fclose(pFile);
    pFile = fopen(pubkey_file, "r");

    X509 *certificate = PEM_read_X509(pFile, NULL, 0, NULL);
    strncpy((char*) cert_expdate, (char*) (X509_get_notAfter(certificate)->data), EXPDATESIZE);
    cert_expdate[EXPDATESIZE] = 0;


    X509_free(certificate);
    fclose(pFile);

    return true;

}

// *****************************************************************************
// *                                                                           *
// * Function: ReadPrivateKey                                                  *
// *                                                                           *
// * Reads the Private Key file and generates the cipher key  in EVP_PKEY form *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *  None                                                                     *
// *                                                                           *
// * Returns:                                                                  *
// *                                                                           *
// *  True if Success                                                          *
// *                                                                           *
// *****************************************************************************
static bool ReadPrivateKey()
{
    int ret = 0;
    EVP_PKEY *local_priv_key = NULL;
    FILE *pFile = NULL;

    if (cipher_key) {
        EVP_PKEY_free(cipher_key);    // don't leak
        cipher_key = 0;    // don't leak
    }

    // Get the Certificate directory and the filename
    if (!setup_already_done)
	SetupCertFilename();

    pFile = fopen(privkey_file, "r");

    if (pFile == NULL)
       return false;

    cipher_key = PEM_read_PrivateKey(pFile, &local_priv_key, 0, NULL);

    fclose(pFile);

    if ( ! cipher_key)
        return false;

    setup_already_done = true;
    return true;

}

// *****************************************************************************
// *                                                                           *
// * Function: MyProcInfo                                                      *
// *                                                                           *
// * Get the process info from monitor                                         *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *  <proc_info>                     ProcInfo *          In                   *
// *                                                                           *
// * Returns:                                                                  *
// *                                                                           *
// *  None                                                                     *
// *                                                                           *
// *****************************************************************************
void MyProcInfo(ProcInfo *proc_info) // Address of struct to return results
{
        short res;
        int nid, pid;
        MS_Mon_Node_Info_Type node_info;
//       char *nodename;  

	// Get process info
        res = msg_mon_get_process_info( NULL, &nid, &pid );
        proc_info->nid = nid;
        proc_info->pid = pid;

//        res = msg_mon_get_node_info_detail(nid, &node_info);
//        nodename = strtok(node_info.node[0].node_name, ".");

}

// *****************************************************************************
// *                                                                           *
// * Function: decrypt_message                                                 *
// *                                                                           *
// * Decrypt the message sent by the client                                    *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *  <message>                 char *                    In/Out               *
// *                                                                           *
// * Returns:                                                                  *
// *                                                                           *
// *  0 if Success                                                             *
// *  SECMXO_* errors otherwise                                                *
// *                                                                           *
// *  Decrypted Message is copied back into <message>                          *
// *****************************************************************************
extern "C" int decrypt_message(char *message, char *role)
{
    short ret = 0;
    int encryption = 1;

    if (role)
       *role = 0;

    PwdKey *pwd_key = (PwdKey *) message;
    char *rolename = &pwd_key->rolename[0];
    unsigned char *pwd_key_text = (unsigned char *) &pwd_key->data;

    LoginData decrypted_data ;
    

    // check if password is a user token
    if ((message[0] == USERTOKEN_ID_1) &&
        (message[1] == USERTOKEN_ID_2))
       
       return 0;

    if (encryption)

    {
       if ( ! cipher_key )
    	    ReadPrivateKey();

       if ( ! cipher_key )
       {
          // Failure from PEM_read_PrivateKey().
          return SECMXO_PRIVKEY_FILE_ERR;
       }

       // Decrypt (session key, nonce, password).
       int key_byte_len = RSA_size(cipher_key->pkey.rsa);

       int plain_text_len = RSA_private_decrypt(key_byte_len, 
                                                pwd_key_text,
                                                (unsigned char *)&decrypted_data, 
                                                cipher_key->pkey.rsa,
                                                RSA_PKCS1_PADDING);
//     Above returns -1 if there is an error
       if (plain_text_len < 0)
       {
          return SECMXO_DECRYPTION_ERROR;
       }


       int offset = plain_text_len - SESSION_KEYLEN - NONCE_SIZE;
       decrypted_data.password[offset] = 0;            // Add two 0 as null terminator
       decrypted_data.password[++offset] = 0;

       // Build a digest and compare it with the user's.  Reject if they mismatch.
 
       if ( ! ValidateLoginDigest(&decrypted_data.session_key[0], &proc_info, pwd_key))
       {
          // Log error in EMS.
          return SECMXO_DIGEST_MISMATCH;
       }

       if (role && rolename)
       {
          while ((*rolename) != '\0')
          {
             *(role++) = toupper(*(rolename++));
          }
       }
       strcpy(message, (const char *)&decrypted_data.password);
    
    }

return 0;
}

//-----------------------------------------------------------------------------
// Return error text associated with a SECMXO_ error as defined in SecSrvrMXO.h.
// Whenever a new error is added there, text must be added here.
// 
// LCOV_EXCL_START 
extern "C" void pwd_get_errortext     (short err, /*IN: error to retrieve text for */
                                       char* errortext, /*OUT: error text buffer */
                                       short errortext_len /* IN: length of errortext */)
{
  char buf[256];

  buf[0] = 0;

  switch (err)
  {
  case SECMXO_NO_ERROR:
    sprintf (buf, "No error");
    break;
  case SECMXO_INTERNAL_ERROR:
    sprintf (buf, "Internal error");
    break;
  case SECMXO_INTERNAL_ERROR_FATAL:
    sprintf (buf, "Internal error (fatal)");
    break;
  case SECMXO_PROGFILE_NOT_AUTHORIZED:
    sprintf (buf, "Program file not authorized");
    break;
  case SECMXO_DIGEST_MISMATCH:
    sprintf (buf, "Digest mismatch");
    break;
  case SECMXO_DECRYPTION_ERROR:
    sprintf (buf, "Decryption error");
    break;
  case SECMXO_USER_NOT_AUTHORIZED:
    sprintf (buf, "Unauthorized user");
    break;
  case SECMXO_PRIVKEY_FILE_NOT_FOUND:
    sprintf (buf, "Certificate file not found");
    break;
  case SECMXO_PRIVKEY_FILE_ERR:
    sprintf (buf, "Certificate file error");
    break;
  case SECMXO_CERTIFICATE_EXPIRED:
    sprintf (buf, "Certificate expired");
    break;
  case SECMXO_NO_CERTIFICATE:
    sprintf (buf, "Certificate not found");
    break;
  case SECMXO_CERTIFICATE_UPDATED:
    sprintf (buf, "Certificate has been updated");
    break;

  default:
    sprintf (buf, "Unknown error");
    break;
  }

  strncpy(errortext, buf, errortext_len);

}
// LCOV_EXCL_STOP 

//-----------------------------------------------------------------------------
// SECURITY_SETUP_
//
// Security Initialization - call once during process startup

extern "C" short SECURITY_SETUP_()
{
    short ret = 0;

    if (setup_already_done)
    {
        return SECMXO_NO_ERROR;
    }

    if (!ReadPrivateKey())
// LCOV_EXCL_START 
      // Failure from PEM_read_PrivateKey().
      return SECMXO_PRIVKEY_FILE_ERR;
// LCOV_EXCL_STOP 

    if(!ReadPublicCertificate())
// LCOV_EXCL_START 
      return SECMXO_NO_CERTIFICATE;
// LCOV_EXCL_STOP 

    MyProcInfo(&proc_info);

    setup_already_done = 1;
  
    return SECMXO_NO_ERROR;
}

//------------------------------------------------------------------------------
// IS_CERTIFICATE_NEEDED_
//
// Check to see if certificate is needed
//
// Returns zero (false) if not needed, else 1 (true)

extern "C" short IS_CERTIFICATE_NEEDED_(char *credentials) // IN Pointer to
                                             // password key in message
{
    // If no credentials, then a certificate is needed.
    if (credentials == NULL) {
// LCOV_EXCL_START 
        return 1;   // return true
// LCOV_EXCL_STOP 
    }

    // If this is a user token, then the certificate was already checked.
    if (credentials[0] == USERTOKEN_ID_1 && credentials[1] == USERTOKEN_ID_2) {
// LCOV_EXCL_START 
        return 0;   // return false
// LCOV_EXCL_STOP 
    }

    return 1;   // return true
}

extern "C" short CHECK_CERTIFICATE_TS()
{
    short ret;

    ret = SECURITY_SETUP_();
    if (ret != SECMXO_NO_ERROR)
    {
// LCOV_EXCL_START 
        return ret;
// LCOV_EXCL_STOP 
    }

    time_t saved_file_timestamp = cert_timestamp;

    struct stat file_stats;
    if (0 != stat(pubkey_file, &file_stats))
// LCOV_EXCL_START 
       return SECMXO_NO_CERTIFICATE;
// LCOV_EXCL_STOP 

    bool certificate_was_updated = cert_timestamp != file_stats.st_mtime;

    if (certificate_was_updated)
    {
// LCOV_EXCL_START 
       if (!ReadPrivateKey())
           // Failure from PEM_read_PrivateKey().
          return SECMXO_PRIVKEY_FILE_ERR;
       if (!ReadPublicCertificate())
          return SECMXO_NO_CERTIFICATE;
// LCOV_EXCL_STOP 
    }

    return SECMXO_NO_ERROR;

}

//------------------------------------------------------------------------------
// Validate client certificate timestamp.
// Returns zero if successful

// When MXOSRVR starts up, this is what it downloads as a dummy certificate.
#define NULL_CERTIFICATE "000000000000"

extern "C" short VALIDATE_CERTIFICATE_TS(char * users_expiration_time)
{
    short ret;
 
//    if (my_pin == PIN_UNDEFINED)
//        FindMyProcessInfo();

//    if (cert_expdate[0] == 0)
//       return SECMXO_PRIVKEY_FILE_NOT_FOUND;

    // Check to see if we have the latest server certificates

    ret=CHECK_CERTIFICATE_TS();
    if (ret != SECMXO_NO_ERROR)
// LCOV_EXCL_START 
       return ret;
// LCOV_EXCL_STOP 

    // When MXOSRVR starts up, it downloads a dummy certificate.
    if ((users_expiration_time == NULL) ||
        (users_expiration_time[0] == 0) ||
        (0 == strncmp(users_expiration_time,
                      NULL_CERTIFICATE,
                      strlen(NULL_CERTIFICATE)))) {
// LCOV_EXCL_START 
             return SECMXO_CERTIFICATE_UPDATED;
// LCOV_EXCL_STOP 
    }

    // If client has an old certificate, request download.
    if ((0 != strncmp(cert_expdate,
                      users_expiration_time,
                      EXPDATESIZE))) {
// LCOV_EXCL_START 
        return SECMXO_CERTIFICATE_UPDATED;
// LCOV_EXCL_STOP 
    }


    // Get "now" in ASN1 format; put in current_time.
    X509* x509_cert = X509_new();
    if ( ! x509_cert) {
// LCOV_EXCL_START 
         return SECMXO_INTERNAL_ERROR;
// LCOV_EXCL_STOP 
    }

    X509_gmtime_adj(X509_get_notBefore(x509_cert), 0);
    ASN1_TIME* asn1_notBefore = X509_get_notBefore(x509_cert);
    char* asn1_current_time = (char*) asn1_notBefore->data;
    char current_time[EXPDATESIZE_ALLOC];
    strncpy(current_time, asn1_current_time, EXPDATESIZE+3); // 2 for the ZZ & 1 for the NULL
    X509_free(x509_cert);


    // Check to see if the certificate expired.
    if (-1 == strncmp(users_expiration_time,
                      current_time,
                      EXPDATESIZE)) {
        // Client had the latest timestamp, but it expired.
// LCOV_EXCL_START 
        return SECMXO_CERTIFICATE_EXPIRED;
// LCOV_EXCL_STOP 
    }

    return SECMXO_NO_ERROR;
}

//-----------------------------------------------------------------------------
// GET_CERTIFICATE
//
// Get Certificate (public key)
//
// Returns zero if successful

extern "C" short GET_CERTIFICATE(void* certificate, /* 0 if just want len*/
                                     short* certificate_len /*in bytes*/ )
{
    short ret;

    ret = SECURITY_SETUP_();
    if (ret != SECMXO_NO_ERROR)
    {
// LCOV_EXCL_START 
        return ret;
// LCOV_EXCL_STOP 
    }

    if (certificate_len)
    {
        *certificate_len = (short) cert_data_len;
    }

    if (certificate)
    {
        memcpy(certificate, cert_data, (short) cert_data_len);
        ((char*)certificate) [cert_data_len] = 0;
    }
    return SECMXO_NO_ERROR;
}
