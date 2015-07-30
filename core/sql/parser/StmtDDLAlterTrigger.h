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
#ifndef STMTDDLALTERTRIGGER_H
#define STMTDDLALTERTRIGGER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTrigger.h
 * Description:  class for parse node representing Alter Trigger statements
 *
 *
 * Created:      7/29/98
 * Language:     C++
 * Status:       $State: Exp $
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// Revision 1.0  1998/29/07 03:21:44
// Initial revision
//
//
//
// -----------------------------------------------------------------------

#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterTrigger;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Alter Trigger statement
// -----------------------------------------------------------------------
class StmtDDLAlterTrigger : public StmtDDLNode
{

public:

  // constructor
  //  Parameters:
  //    "isEnable"   - TRUE  --> to enable , FALSE --> to disable
  //    "allOfTable" - TRUE  --> apply to all the triggers of the 
  //                             table whose name is "triggerOrTableQualName"
  //                   FALSE --> apply to a single trigger whose
  //                             name is "triggerOrTableQualName"
   StmtDDLAlterTrigger(NABoolean isEnable,
			     NABoolean allOfTable,
			     const QualifiedName & triggerOrTableQualName):
    StmtDDLNode(DDL_ALTER_TRIGGER),
    triggerOrTableQualName_(triggerOrTableQualName, PARSERHEAP()),
    isEnable_(isEnable),
    allOfTable_(allOfTable)
  {
  }
  
  // virtual destructor
  virtual ~StmtDDLAlterTrigger();

  // cast
  virtual StmtDDLAlterTrigger * castToStmtDDLAlterTrigger();

  // accessor
  inline       NABoolean isEnable(void)     { return isEnable_ ; }
  inline       NABoolean isAllOfTable(void) { return allOfTable_ ; }
  inline const NAString getTriggerOrTableName() const;
  inline const QualifiedName & getTriggerOrTableNameAsQualifiedName() const;
  inline       QualifiedName & getTriggerOrTableNameAsQualifiedName() ;

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;


private:
  
  NABoolean isEnable_;    //  TRUE if ENABLE, FALSE if DISABLE
  NABoolean allOfTable_;  //  TRUE for a TABLE, FALSE for a TRIGGER
  QualifiedName triggerOrTableQualName_;

}; // class StmtDDLAlterTrigger

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTrigger
// -----------------------------------------------------------------------

//
// accessor
//

inline QualifiedName  &
StmtDDLAlterTrigger::getTriggerOrTableNameAsQualifiedName()
{
  return triggerOrTableQualName_;
}

inline const QualifiedName &
StmtDDLAlterTrigger::getTriggerOrTableNameAsQualifiedName() const
{
  return triggerOrTableQualName_;
}


inline const NAString 
StmtDDLAlterTrigger::getTriggerOrTableName() const
{
  return triggerOrTableQualName_.getQualifiedNameAsAnsiString();
}

#endif // STMTDDLALTERTRIGGER_H

