/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1996-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef _UDREXTRN_H_
#define _UDREXTRN_H_

/* -*-C++-*-
*****************************************************************************
*
* File:         udrextrn.h
* Description:
* Created:      01/01/2001
* Language:     C++
*
*
*****************************************************************************
*/

#include "LmError.h"
#include "LmLangManagerJava.h"
#include "spinfo.h"
#include "UdrExeIpc.h"
#include "ComDiags.h"
#include "sqlcli.h"
#include "SQLCLIdev.h"
#include "udrglobals.h"

class UdrServerReplyStream;
class UdrServerDataStream;
class UdrGlobals;

extern void sendControlReply(UdrGlobals *UdrGlob,
                             UdrServerReplyStream &msgStream,
                             SPInfo *sp);
extern void sendDataReply(UdrGlobals *UdrGlob,
                          UdrServerDataStream &msgStream,
                          SPInfo *sp);

extern void controlErrorReply(UdrGlobals *UdrGlob,
                              UdrServerReplyStream &msgStream,
                              Lng32 errortype, Lng32 error,
                              const char *charErrorInfo);
extern void dataErrorReply(UdrGlobals *UdrGlob,
                           UdrServerDataStream &msgStream,
                           Lng32 errortype, Lng32 error,
                           const char *charErrorInfo = NULL,
                           ComDiagsArea *diags = NULL);

#endif // _UDREXTRN_H_
