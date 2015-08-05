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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         CatSQLShare.h
 * Description:  This file code to be shared between Catman and Utilities
 *               
 *               
 * Created:      6/2/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef _CATSQLSHARE_H_
#define _CATSQLSHARE_H_

#include "Platform.h"
   typedef __int64 SQLShareInt64;
   #ifdef CLI_DLL
      #define CATSQLSHARE_LIB_FUNC __declspec( dllexport )
   #else
      #ifdef SHARE_DLL
         #define CATSQLSHARE_LIB_FUNC __declspec( dllexport )
      #else
         #ifdef CAT_DEBUG
            #define CATSQLSHARE_LIB_FUNC __declspec( dllexport )
         #else
	    #define CATSQLSHARE_LIB_FUNC __declspec( dllimport )
         #endif
      #endif
   #endif


// Generate a unique value. This is really the meat and bone of UIDs,
// packaged in a context where it can be used by all SQL components including utilities.
SQLShareInt64 CATSQLSHARE_LIB_FUNC
generateUniqueValue (void);

// Generate a funny name, of a specific format:
enum FunnyNameFormat 
{ 
  UID_SMD_SUBVOL_NAME = 0,
  UID_USER_SUBVOL_NAME,
  UID_FILE_NAME,
  UID_GENERATED_ANSI_NAME 
};

// The next two defines include the terminating null character of a string
#define MAX_FUNNY_NAME_LENGTH 16     // max length of a generated name part
#define MAX_GENERATED_NAME_LENGTH 36 // max length of an entire generated name

void CATSQLSHARE_LIB_FUNC
generateFunnyName (const FunnyNameFormat nameFormat, char * generatedName);

//  Generate a name from input simple object name by truncating to 20 chars,
//  and appending 9-byte timestamp.
//  Caller must allocate and pass a 30 char array for output generatedName.
//  This array will be overwritten with a null-terminated string.

void  CATSQLSHARE_LIB_FUNC 
  CatDeriveRandomName ( const char* inputName // input simple name, any size
                      , char* generatedName   // output name, max 30 bytes.
                      );

//  Return value indicating whether process is currently running:
enum CatIsRunningResult { notRunning, isRunning, errorOcurred };
//  Input value is null-terminated ASCII string form of process pHandle + proc Name.


Int32 CATSQLSHARE_LIB_FUNC
SqlShareLnxGetMyProcessIdString (char   *processIdStrOutBuf,        // out
                                 size_t  processIdStrOutBufMaxLen,  // in
                                 size_t *processIdStrLen);          // out

#endif  // _CATSQLSHARE_H_
