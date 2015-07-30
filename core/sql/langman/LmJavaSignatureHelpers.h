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
**********************************************************************/
#ifndef LMJAVASIGNATUREHELPERS_H
#define LMJAVASIGNATUREHELPERS_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmJavaSignatureHelpers.h
* Description:  Java Signature
*
* Created:      10/07/2003
* Language:     C++
*
*
******************************************************************************
*/

#include "LmJavaType.h"

// Note: The methods in this file are called directly (without using the 
// LmJavaSignature class) by code in 'sqlutils' directory. The reason for sqlutils
// to take this approach was because it does not include code from the 'common' directory, 
// which is required in LmJavaSignature class. Hence the helper files were created to 
// implement the below methods without having to include 'common' code. 
// So, any change to the signatures in this file will impact the 'sqlutils' code.


// A helper function to unpack (decode) an encoded Java signature.
// Input: encodedSignature (the encoded signature),  unpackedSignature (pointing at the pre-allocated space receiving the unpacked signature)
// Output: unpackedSignature (unpacked signature)
Int32 unpackSignature(const char* encodedSignature, char *unpackedSignature);

// A helper function to return the size if an encoded string is unpacked.
// Input: encodedSignature (the encoded signature)
// Return: - the size if the encoded signature is unpacked
//		   - the total number of parameters present in the 
//           packed method signature
Int32 getUnpackedSignatureSize(const char* encodedSignature, Int32 *numParams=NULL);

#endif
