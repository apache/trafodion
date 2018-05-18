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
#ifndef _UDRDEFS_H_
#define _UDRDEFS_H_

/* -*-C++-*-
*****************************************************************************
*
* File:         udrdefs.h
* Description:  Symbolic constants for UDR server code
* Created:      01/01/2001
* Language:     C++
*
*
*****************************************************************************
*/

#define EYE_SP "SP"
#define EYE_SPLIST "SL"

#define MAXERRTEXT  255

enum UdrErrorEnum {
        UDR_ERR_UNKNOWN_MSG_TYPE        = 11101, // udrserv
        UDR_ERR_MISSING_UDRHANDLE       = 11102, // udrunload, udrcontext, udrinvoke
        UDR_ERR_MISSING_LMROUTINE       = 11103, // udrunload, udrinvoke
        UDR_ERR_CLI_ERROR               = 11104, // udrcancel, udrcommit, udrload
                                                 // spinfo, udrinvoke
        UDR_ERR_INVALID_LM_PARAMMODE    = 11105, // udrinvoke
        UDR_ERR_UNABLE_TO_ALLOCATE_MEMORY=11108, // udrload
        UDR_ERR_TOO_MANY_OPENERS        = 11109, // udrserv
        UDR_ERR_MESSAGE_PROCESSING      = 11110, // udrcancel, udrcommint,
                                                 // udrimok, rsload, rsfetch
                                                 // rscontinue, rsunload
        UDR_ERR_INTERNAL_ERROR          = 11111, // spinfo
        UDR_ERR_DUPLICATE_LOADS         = 11112,
        UDR_ERR_UNEXPECTED_UNLOAD       = 11113,

// RS related messages
        UDR_ERR_MISSING_RSHANDLE	= 11114, // RS handle missing
        UDR_ERR_INTERNAL_CLI_ERROR   	= 11115, // RS Internal CLI error
        UDR_ERR_INVALID_RS_INDEX     	= 11116, // RS Invalid index in RS_LOAD
        UDR_ERR_INVALID_RS_STATE    	= 11117, // RS is in invalid state for
                                                 //  current operation
        UDR_ERR_INVALID_RS_COLUMN_COUNT = 11118  // RS column count is <= 0
};

#define INVOKE_ERR_NO_REQUEST_BUFFER          1
#define INVOKE_ERR_NO_INPUT_ROW               2
#define INVOKE_ERR_NO_REPLY_DATA_BUFFER       3
#define INVOKE_ERR_NO_REPLY_ROW               4
#define INVOKE_ERR_NO_ERROR_ROW               5
#define INVOKE_ERR_NO_REPLY_BUFFER            6

#define RS_ERR_NO_REPLY_MSG                  11
#define RS_ERR_NO_REPLY_DATA_BUFFER          12

#define TRACE_ALWAYS 0
#define TRACE_IPMS 1
#define TRACE_DATA_AREAS 2
#define TRACE_DETAILS 3
#define TRACE_SHOW_DIALOGS 4

#include "Platform.h"

extern void udrAssert(const char *, Int32, const char *);
extern void udrAbort(const char *, Int32, const char *);
#define UDR_ASSERT(p, msg) \
  if (!(p)) { udrAssert( __FILE__ , __LINE__ , msg); }
#define UDR_ABORT(msg) \
  { udrAbort( __FILE__ , __LINE__ , msg); }

// FFDC/TDFS Support
#include "EHCommonDefs.h"
#include "EHException.h"

//
// Define the printf format specifier for 64-bit integers
//
#define INT64_SPEC "%lld"

const short NULL_TX_HANDLE[10] = {0,0,0,0,0,0,0,0,0,0};

enum RequestRowProcessingStatus
{
  A_ROW_IS_PROCESSED   = 1,
  ALL_ROWS_PROCESSED   = 2,
  REPLY_BUFFER_FULL    = 3
};
#endif  //  _UDRDEFS_H_

