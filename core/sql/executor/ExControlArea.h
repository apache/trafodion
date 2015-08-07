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
#ifndef EXCONTROLAREA_H
#define EXCONTROLAREA_H
/* -*-C++-*-
****************************************************************************
*
* File:         ExControlArea.h
* Description:  The executor maintains a table of the dynamic CONTROL
*               statements that were issued by the current context.
*               This is done for two reasons: if arkcmp dies, we need
*               to send the new arkcmp all of the control statements,
*               and if we need to recompile a dynamic statement then
*               we need to find all the applicable dynamic CONTROL
*               statements (NOTE: we may not yet do the latter).
*
* Created:      5/6/98
* Language:     C++
*
****************************************************************************
*/

#include "ComTdbControl.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

class ExControlEntry;
class ExControlArea;

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------
class CliGlobals;
class Queue;

// -----------------------------------------------------------------------
// An entry in the Control Area (represents one CONTROL statement)
// -----------------------------------------------------------------------
class ExControlEntry : public NABasicObject
{
public:
  enum ResendType { UPON_ALL /*default */ , UPON_CMP_CRASH, UPON_CTX_SWITCH };

public:

  ExControlEntry(CollHeap * heap,
  		 ControlQueryType cqt,
		 Int32 reset = 0,
		 char * sqlText = NULL, Int32 lenX = 0, Int16 sqlTextCharSet = (Int16)0/*SQLCHARSETCODE_UNKNOWN*/,
		 char * value1  = NULL, Int32 len1 = 0,
		 char * value2  = NULL, Int32 len2 = 0,
		 char * value3  = NULL, Int32 len3 = 0,
                 Int16 actionType = ComTdbControl::NONE_,
		 ResendType resendType = ExControlEntry::UPON_ALL,
                 NABoolean isNonResettable = FALSE);

  ~ExControlEntry();

  ControlQueryType type() const	{ return cqt_; }
  Int32  getNumValues() const	{ return numValues_; }
  Int32  getReset() const	{ return reset_; }
  void   setReset(Int32 r)	{ reset_ = r; }
  char * getSqlText()           { return sqlText_; }
  Int32  getSqlTextLen()        { return lenX_; }
  Int16  getSqlTextCharSet()    { return sqlTextCharSet_; }
  char * getValue(Int32 i);
  Int32    getLen(Int32 i);
  Int32    match(ControlQueryType cqt, const char * value1, const char * value2,
	       Int32 reset = 0
	      );

  ResendType getResendType();
  Int16 getActionType() { return actionType_; }
  NABoolean isNonResettable() { return nonResettable_; }

private:
  ResendType resendType_;
  CollHeap * heap_;
  ControlQueryType cqt_;
  Int32 reset_;

  Int32 numValues_;

  char * sqlText_;
  Int16  sqlTextCharSet_;
  Int16  actionType_;
  char * value1_;
  char * value2_;
  char * value3_;
  Int32 lenX_;
  Int32 len1_;
  Int32 len2_;
  Int32 len3_;
  NABoolean nonResettable_;
};

// -----------------------------------------------------------------------
// The area (list) of CONTROL statements issued so far
// -----------------------------------------------------------------------
class ExControlArea : public NABasicObject
{
public:
  ExControlArea(ContextCli *context, CollHeap *heap);

  ~ExControlArea();

  void addControl(ControlQueryType type,
		  Int32 reset = 0,
		  const char * sqlText = NULL, Int32 lenX = 0,
		  const char * value1  = NULL, Int32 len1 = 0,
		  const char * value2  = NULL, Int32 len2 = 0,
		  const char * value3  = NULL, Int32 len3 = 0,
                  Int16 actionType = ComTdbControl::NONE_,
		  ExControlEntry::ResendType resendType = ExControlEntry::UPON_ALL,
                  NABoolean isNonResettable = FALSE);
  Queue * getControlList() { return controlList_; }

  static const char *getText(ControlQueryType cqt);

private:
  ContextCli *context_;
  CollHeap *heap_;
  Queue *controlList_;
  void *resetAllQueueEntry_;
  void * sysDefResetQueueEntry_;
};

#endif /* EXCONTROLAREA_H */

