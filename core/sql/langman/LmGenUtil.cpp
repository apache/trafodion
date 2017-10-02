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
******************************************************************************
*
* File:         LmGenUtil.cpp
* Description:  LM Utility functions for Generator
*
* Created:      06/18/2001
* Language:     C++
*
*
******************************************************************************
*/
#include "LmLangManager.h"
#include "LmGenUtil.h"
#include "str.h"

/* setLMObjectMapping() : Processes Java signature and sets the object mapping
 * in Boolean Array. Clients need to allocate the array of Boolean type for
 * number of params + return type. This function sets object mapping for return
 * type too.
 */
LmResult setLMObjectMapping(
  const char *routineSig,
  ComBoolean objMapArray[])
{
  ComUInt32 sigLen = strlen(routineSig);
  char *sig = new char[sigLen+1];
  char *origSigPtr = sig;
  LmResult retCode = LM_OK;

  str_cpy_all(sig, routineSig, sigLen);
  sig[sigLen] = '\0';
 
  ComUInt32 pos = 0;

  while (*sig && retCode != LM_ERR)
  {
    switch (*sig)
    {
      case '(' :
      case '[' :
      case ')' :
        sig++;
        break;
      case 'V' :
      case 'S' :
      case 'I' :
      case 'J' :
      case 'F' :
      case 'D' :
        objMapArray[pos++] = FALSE;
        sig++;
        break;

      case 'L' :
        objMapArray[pos++] = TRUE;
        if ((sig = strchr(sig, ';')) != NULL)
          sig += 1;
        else
          retCode = LM_ERR;
        break;

      default :
        retCode = LM_ERR;

    } // switch (*sig)
  } // while (*sig)

  if (origSigPtr)
    delete origSigPtr;

  return retCode;
}
