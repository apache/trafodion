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

#ifndef secdefscommonh
#define secdefscommonh

//*****************************************************************************
// Definitions common to client and server

#define NONCE_RANDOM 24
#define NONCE_SEQNUM  8
#define NONCE_SIZE (NONCE_RANDOM+NONCE_SEQNUM)
#define SESSION_KEYLEN 32
#define DIGEST_LENGTH 32 
#define PWDKEY_SIZE_LESS_LOGINDATA (PWDID_SIZE + ROLENAME_SIZE + DIGEST_LENGTH + TIMESTAMP_SIZE)
#define AES_BLOCKSIZE      16  // AES block size used in data encryption
#define KEY_REFRESH   30
#define TIMESTAMP_SIZE 8
#define ROLENAME_SIZE 128 
//#define PROCINFO_SIZE 12
#define PROCINFO_SIZE 8
#define PWDID_SIZE 4
#define EXPDATESIZE 12 
#define UNUSEDBYTES 11 // For public key encryption, the number of bytes
                       // to be encrypted is 11 bytes less than the public 
                       // key length

//  Structure used to describe layout of Encrypted data in login message

typedef struct {
        char session_key[SESSION_KEYLEN]; // 000 Session key
        char nonce[NONCE_SIZE];           // 032 Nonce
        char *password;                // 064 User's password
} LoginData;            // 128 for 1024 or 256 for 2048


//-----------------------------------------------------------------------------
//  Structure used to describe layout of password key

typedef struct {
    char id[4];             //000 Key identifier, binary values 1,2,3,4
                            //or 1,2,2,4 keys, optional mode only
   char rolename[ROLENAME_SIZE];      //004 Rolename
   char digest[DIGEST_LENGTH]; //132 Digest of server id and encrypted data
   char ts[TIMESTAMP_SIZE];    // 164 time stamp
   LoginData data;             //172 Encrypted data
} PwdKey;                      // 300 for 1024 or 428 for 2048

#endif
