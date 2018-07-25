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
#ifndef _UDRUTIL_H_
#define _UDRUTIL_H_

/* -*-C++-*-
*****************************************************************************
*
* File:         udrutil.h
* Description:  Utility functions for UDR server code
* Created:      01/01/2001
* Language:     C++
*
*
*****************************************************************************
*/

#include "sqlcli.h"
#include "udrdefs.h"
#include "ComSmallDefs.h"
#include "LmParameter.h"

//
// Forward declarations
//
class SqlBuffer;
class ComCondition;
class ComDiagsArea;
class UdrGlobals;
class LmLanguageManager;
class LmParameter;
enum LmResult;

void displaySqlBuffer(SqlBuffer *, Lng32, ostream &os = cout);
void displayStatement(const SQLSTMT_ID &);

void dumpLmParameter(LmParameter &, Lng32, const char *);
void dumpBuffer(unsigned char * );
void dumpBuffer(unsigned char * , size_t );
void dumpComCondition(ComCondition * , char * );
void dumpDiagnostics (ComDiagsArea *, Lng32 );

const char * getFSDataTypeName(const ComFSDataType &);
const char * getDirectionName(ComColumnDirection d);
const char * getLmResultSetMode(const LmResultSetMode &);

extern FILE *UdrTraceFile;
void ServerDebug(const char *, ...);

void doMessageBox(UdrGlobals *UdrGlob, Int32 trLevel,
                  NABoolean moduleType, const char *moduleName);


#ifdef _DEBUG
void sleepIfPropertySet(LmLanguageManager &lm,
                        const char *property,
                        ComDiagsArea *d);
NABoolean getLmProperty(LmLanguageManager &lm,
                        const char *property,
                        Lng32 &result,
                        ComDiagsArea *diags);
#endif // _DEBUG

void  getActiveRoutineInfo(UdrGlobals *UdrGlob, 
                           char *routineName,
                           char *routineType,
                           char *routineLanguage,
                           NABoolean &isRoutineActive);

#endif // _UDRUTIL_H_















