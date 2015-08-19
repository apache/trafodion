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
#ifndef SECSRVRMXO_H
#define SECSRVRMXO_H


// This file (formerly security.h) is used as an interface between MXOSRVR
// code and the Password Security code.

// Error definitions:

// Return codes of 0 for no error, else a SQL error code.

// Error text for each of these SECMXO_* errors is defined in 
// MXOInterface.cpp PWD_GET_ERRORTEXT.  If any new errors are added here,
// corresponding text needs to be added there.

// no error
#define SECMXO_NO_ERROR                0

// Unexplained error
#define SECMXO_INTERNAL_ERROR          4400
#define SECMXO_INTERNAL_ERROR_FATAL    4419   // Make MXOSRVR die

#define SECMXO_PROGFILE_NOT_AUTHORIZED 4402

// Problems from Logon checks
#define SECMXO_DIGEST_MISMATCH         4410
#define SECMXO_DECRYPTION_ERROR        4411

// Problems from PrivKey File
#define SECMXO_PRIVKEY_FILE_NOT_FOUND  4412
#define SECMXO_PRIVKEY_FILE_ERR        4413

// Problems from KEYS (certificate) File
#define SECMXO_CERTIFICATE_EXPIRED     4414
#define SECMXO_NO_CERTIFICATE          4415
#define SECMXO_CERTIFICATE_UPDATED     4416

// Problems from rejected user
#define SECMXO_USER_NOT_AUTHORIZED     4418


#define SECSERV_SQLSTATE_ID "38001"

extern "C" void pwd_get_errortext (short err,
                                   char *errortext,
                                   short errortext_len);

extern "C" int decrypt_message(char *message,
                               char *role);
//------------------------------------------------------------------------------
// Check to see if certificate is needed
// Returns zero if not needed, else 1
extern "C" short IS_CERTIFICATE_NEEDED_(char *credentials) // IN Pointer to
                                                           // password key in message
;


//------------------------------------------------------------------------------
// Security setup
// Returns zero if successful

extern "C" short SECURITY_SETUP_();

extern "C" short VALIDATE_CERTIFICATE_TS(char* ts) ;


//-----------------------------------------------------------------------------
// Get Certificate (public key)
// Returns SECMXO_NO_ERROR if successful, SECMXO_NO_CERTIFICATE if error.

extern "C" short GET_CERTIFICATE(void* certificate,       // NUL if just want len
                                 short* certificate_len); //in bytes 


#endif
