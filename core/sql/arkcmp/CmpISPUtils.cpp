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
 * File:         CmpSPUtils.C
 * Description:  This file contains the utility functions provided by arkcmp
 *               for internal stored procedures.
 *               
 *               
 * Created:      03/16/97
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "CmpStoredProc.h"
#include <memory.h>

// contents of this file includes the procedures to extract/format the
// fields of data, these functions will be passed into the user developed
// stored procedures to manipulate data.
//
// CmpSPExtractFunc 
// CmpSPFormatFunc
// CmpSPKeyValueFunc

SP_HELPER_STATUS CmpSPExtractFunc_ (
                                    Lng32 fieldNo, 
                                    SP_ROW_DATA rowData, 
                                    Lng32 fieldLen, 
                                    void* fieldData,
                                    Lng32 casting )
{
  CmpSPExecDataItemInput* inPtr = (CmpSPExecDataItemInput*)rowData;
  ULng32 tempNum = (ULng32)fieldNo;
  ULng32 tempLen = (ULng32)fieldLen;
  ComDiagsArea* diags = inPtr->SPFuncsDiags();
  if ( inPtr->extract(tempNum,(char*)fieldData,tempLen, 
    ((casting==1) ? TRUE : FALSE), diags) < 0 ||
    diags->getNumber() )
  {
    // TODO, convert the errors in diags into error code.
    diags->clear(); // to be used for next CmpSPExtractFunc_
    return SP_ERROR_EXTRACT_DATA;
  }
  // test code for assert, check for arkcmp/SPUtil.cpp SP_ERROR_*
  // for detail description.
  //#define ASTRING "TestCMPASSERTEXE"
  //assert( strncmp((char*)fieldData, ASTRING, strlen(ASTRING) != 0 ) ) ;

  return SP_NO_ERROR;
}

SP_HELPER_STATUS CmpSPFormatFunc_ (Lng32 fieldNo,
                                   SP_ROW_DATA rowData, 
                                   Lng32 fieldLen, 
                                   void* fieldData,
                                   Lng32 casting)
{
  CmpSPExecDataItemReply* replyPtr = (CmpSPExecDataItemReply*)rowData;
  ULng32 tempNum = (ULng32)fieldNo;
  ULng32 tempLen = (ULng32)fieldLen;
  ComDiagsArea* diags = replyPtr->SPFuncsDiags();
  if ( replyPtr->moveOutput(fieldNo,(char*)fieldData,tempLen, 
    ((casting==1) ? TRUE : FALSE), diags) < 0 ||
    diags->getNumber() )
  {
    diags->clear(); // to be used for next CmpSPFormatFunc_
    return SP_ERROR_FORMAT_DATA;
  }
  else
    return SP_NO_ERROR;
}

SP_HELPER_STATUS CmpSPKeyValueFunc_ (
                                     Lng32 keyIndex,
                                     SP_KEY_VALUE key,
                                     Lng32 keyLength,
                                     void* keyValue,
                                     Lng32 casting)
{
  return SP_NOT_SUPPORTED_YET;
}

extern "C" { 
	SP_EXTRACT_FUNCPTR CmpSPExtractFunc = &CmpSPExtractFunc_;
    SP_FORMAT_FUNCPTR CmpSPFormatFunc = &CmpSPFormatFunc_;
    SP_KEYVALUE_FUNCPTR CmpSPKeyValueFunc = &CmpSPKeyValueFunc_;
}

