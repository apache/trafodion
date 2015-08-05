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

#ifndef clienterrcodeh
#define  clienterrcodeh

// clientErrCode.h
// This file inludes all defined error codes returns by the
// client Security DLL 

#define ERR_RETRIEVE_KEY_FROM_FILE 1
#define SESSION_KEY_GENERATION_FAILED 2
#define INPUT_PARAMETER_IS_NULL 3
#define PWD_LENGTH_TOO_LONG 4
#define ENCRYPTION_FAILED 5
#define HMAC_FAILED 6
#define PUBKEY_LENGTH_IS_ZERO 7
#define ERR_READ_CERT_FILE 8
#define FAILED_GENERATE_RANDOM_NUM 9
#define CIPHER_TEXT_LEN_NOT_EQUAL_KEY_LEN 10
#define BAD_MESSAGE_DIGEST_LEN 11
#define DECRYPTION_FAILED 12
#define ERR_WRITE_CERT_FILE 13
#define HOME_ENVIRONMENT_VAR_IS_NULL 14
#define FAILED_BUILDING_PWDKEY  15
#define DIR_NOTFOUND 16
#define UNABLE_TO_SEED_RNG 17
#define MEMORY_ALLOC_FAILED 18
#define MUTEX_NOT_EXIST 19
#define FAILED_LOADING_LIB 20
#define MUTEX_RELEASE_FAILED 21
#define FILE_NOTFOUND 22
#define ERR_OPEN_INPUT_FILE 23
#define BAD_PWDKEY_LENGTH 24


struct tableErr
{
    int        SQLErrNum;
    char      *errMsg;
};

// An array of tables error numbers and error messages

static tableErr TableErrArray []=
{
	// The index 0 is not used since ODBC treats 0 as success.
	{00000, " This is not an error code"},
	{29701, "*** ERROR[29701] failed to retrieve the public key from the certificate file %s."},
	{29702, "*** ERROR[29702] Internal error: Session key generation failed."},
	{29703, "*** ERROR[29703] Internal error: The input parameter %s is null."},
	{29704, "*** ERROR[29704] The password was too long."},
	{29705, "*** ERROR[29705] Internal error: Failed to encrypt the password."},
	{29706, "*** ERROR[29706] Internal error: Failed to create the keyed-Hash Message Authentication Code."},
	{29707, "*** ERROR[29707] Internal error: The public key length was zero."},
	{29709, "*** ERROR[29709] Error reading the certificate file %s."},
	{29710, "*** ERROR[29710] Internal error: Random number generation failed."},
	{29711, "*** ERROR[29711] Internal error: The length of the cipher text was not equal to the length of the public key."},
	{29712, "*** ERROR[29712] Internal error: The length of the HMAC message was less than the expected length."},
	{29715, "*** ERROR[29715] Internal error: Failed to decrypt the password."},
	{29716, "*** ERROR[29716] error writing to the certificate file %s."},
	{29720, "*** ERROR[29720] No HOME environment variables are set on the system."},
	{29722, "*** ERROR[29722] Internal error: Error building the password key."},
	{29723, "*** ERROR[29723] Directory %s is not found."},
	{29724, "*** ERROR[29724] Internal error: Failed to seed the random number generator."},
	{29725, "*** ERROR[29725] Internal error: Memory allocation failed."},
	{29726, "*** ERROR[29726] Internal error: Mutex does not exist."},
	{29727, "*** ERROR[29727] Internal error: Failed to load the library ADVAPI32.DLL."},
	{29728, "*** ERROR[29728] Internal error: Failed to release mutex."},
	{29713, "*** ERROR[29713] Certificate file %s is not found."},
	{29729, "*** ERROR[29729] Failed to open the certificate file %s."},
	{29730, "*** ERROR[29730] Internal error: The password key length is either longer or shorter than the expected length."}

};

#endif
