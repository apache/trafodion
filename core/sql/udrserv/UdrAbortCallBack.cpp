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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         UdrAbortCallBack.cpp
 * Description:  abort call back functions from the UDR server
 *               
 *               
 * Created:      7/08/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "UdrAbortCallBack.h"
#include "UdrFFDC.h"
#include "string.h"

void UdrAbortCallBack::doCallBack(const char *msg, const char *file, UInt32 line){
#define TEXT_SIZE 1024
  char extMsg[TEXT_SIZE];
  strcpy(extMsg, "MXUDR: ");
  strncat(extMsg, msg, sizeof(extMsg)-strlen(extMsg));
  makeTFDSCall(extMsg,(char *)file,line);
}
