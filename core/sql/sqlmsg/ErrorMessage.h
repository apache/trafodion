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
#ifndef ERRORMESSAGE_H
#define ERRORMESSAGE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ErrorMessage.h
* Description:  Error Message
* Created:      01/23/95
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "BaseTypes.h"
#include "ComDiags.h"
#include "NAWinNT.h"
#include "NAError.h"

#undef max
#undef min

class SqlciEnv;

enum { NO_DETAIL, DETAIL };


enum MsgParamType { NAMED_PARAM, POSITIONAL_PARAM };

const NAWchar ERRORPARAM_BEGINMARK = NAWchar('$');
const NAWchar ERRORPARAM_TYPESEP   = NAWchar('~');

class ErrorMessage
{
public:

  // -------------------------------------------------------------------------
  // Size of the error message buffer.
  // If modified, also change the value of ErrorMessage__MSG_BUF_SIZE
  // in smdio/CmTableDiagnostics.cpp.
  // -------------------------------------------------------------------------
  enum { MSG_BUF_SIZE = 2048 };		// copy this to CmTableDiagnostics.cpp
  
  // -------------------------------------------------------------------------
  // Constructor, destructor
  // -------------------------------------------------------------------------
  ErrorMessage(void *) {}
  ~ErrorMessage() {}

  // -------------------------------------------------------------------------
  // Method for printing a formatted error message.
  // -------------------------------------------------------------------------
  void printErrorMessage(NAError * errcb);
  
private:
  // -------------------------------------------------------------------------
  // Utility method for formatting an error message.
  // -------------------------------------------------------------------------
  void insertParams(NAError * errcb);
  
  // -------------------------------------------------------------------------
  // Message buffer used for retrieving and formatting error text.
  // -------------------------------------------------------------------------
  NAWchar msgBuf_[MSG_BUF_SIZE];
  
}; // class ErrorMessage


// The "commentIf" param to NADumpDiags and NAWriteConsole:
// if 0 or FALSE, does nothing special (output as normal);
// if -1 or TRUE, each line we output is preceded by SQL comment marker "-- ";
// if +1 (NADumpDiags only), only *warning* lines are comment-prefixed.
// if +2 (NADumpDiags only), no SQL code prefix, no comment marker "-- "


const Int32 NO_COMMENT = +2;

void NADumpDiags(ostream&, ComDiagsArea*,
		 NABoolean newline = FALSE,
		 Int32 commentIf = 0,
		 FILE* fp = NULL,
		 short verbose = 1, 
                 CharInfo::CharSet terminalCharSet = CharInfo::ISO88591);

Lng32 FixupMessageParam(NAWchar *paramName, MsgParamType paramType = NAMED_PARAM);

void FixCarriageReturn(char *str);

// NT_PORT 03/20/97 mhr:
// Global function for portability, in two overloaded flavors for WINNT.
//
void NAWriteConsole(const char *str,		// always defined
		    ostream& outStream = cerr, 
		    NABoolean newline = FALSE,
		    NABoolean commentIf = FALSE);

void NAWriteConsole(const NAWchar *str,
		    ostream& outStream = cerr, 
		    NABoolean newline = FALSE,
		    NABoolean commentIf = FALSE,
                    CharInfo::CharSet terminal_cs = CharInfo::ISO88591);


void FixCarriageReturn(NAWchar *str);

#endif /* ERRORMESSAGE_H */
