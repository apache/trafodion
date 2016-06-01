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

#ifndef SECSRVR_H
#define SECSRVR_H

#pragma nowarn(1303)
#include <stdio.h>  
#include <string.h>  
#include <time.h>  

#include "auth.h"

// Including following for EVP_PKEY
#include <openssl/evp.h>
#include <openssl/hmac.h>

// Data definitions:


// Size of a certificate expiration date.  From client's common/secdefsCommon.h.
// Expiration date = 12 bytes long, in the form "YYMMDDHHMMSS".  Note that this
// does not include the trailing "Z" that we get from X509 ASN1 code, nor a
// null terminator byte.

#define EXPDATESIZE 12
#define EXPDATESIZE_ALLOC 16  // When allocating, use 16 bytes

#define PIN_UNDEFINED  -1
#define CPU_UNDEFINED  -1

#define NONCE_RANDOM 24
#define NONCE_SEQNUM  8
#define NONCE_SIZE (NONCE_RANDOM+NONCE_SEQNUM)
#define SESSION_KEYLEN 32
#define DIGEST_LENGTH  32
#define TIMESTAMP_SIZE 8
#define LOGIN_DATA_OVERHEAD 4

//-----------------------------------------------------------------------------
//  Structure used to describe layout of Encrypted data in login message

typedef struct {
        char session_key[SESSION_KEYLEN];         // 000 Session key
        char nonce[NONCE_SIZE];                   // 0x20 Nonce
        char password[192 + LOGIN_DATA_OVERHEAD]; // 0x40 User's password                                                         
} LoginData;                // 256 + 4 bytes overhead for null terminator

#define MAX_PRIV_KEY_LEN  (2048 + 1)

//-----------------------------------------------------------------------------
// Structure used to hold information relating to this process

typedef struct {
    int       pid;             // Process PIN
    int       nid;             // Processor
} ProcInfo;                    // 8 bytes total, client uses this with 8 byte timestamp



//-----------------------------------------------------------------------------
//  Structure used to describe layout of password key

typedef struct {
        char id[4];                             //000 Key identifier, binary values 1,2,3,4 or 1,2,2,4
        //int index;                            //004 Index into static keys, optional mode only
        char rolename[128];                     //008 Rolename
        char digest[DIGEST_LENGTH];             //136 Digest of server id and encrypted data
        char ts[8];                             //156 Timestamp
        LoginData data;                         //164 Encrypted data
} PwdKey;                                       //292


//-----------------------------------------------------------------------------
// Validate Policy Digest

bool ValidateLoginDigest(char *key,                 // The sesion key
                         ProcInfo *proc_info,       // Info about the process
                         PwdKey *pwd_key);          // Pointer to pwd_key


// Obtain information about current process

short GetProcInfo(ProcInfo *proc_info);             // Address of struct to return results 

#endif
