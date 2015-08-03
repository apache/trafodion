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
#ifndef STMTDDLDROPROUTINE_H
#define STMTDDLDROPROUTINE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropRoutine.h
 * Description:  class for parse node representing Drop Routine statements
 *
 *
 * Created:      9/30/1999
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLDropRoutine;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Catalog statement
// -----------------------------------------------------------------------
class StmtDDLDropRoutine : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropRoutine(ComRoutineType routineType,
                     const QualifiedName & routineQualName,
                     const QualifiedName & routineActionQualName,
                     ComDropBehavior dropBehavior,
                     NABoolean cleanupSpec,
                     NABoolean validateSpec,
                     NAString * pLogFile,
                     CollHeap * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLDropRoutine();

  // cast
  virtual StmtDDLDropRoutine * castToStmtDDLDropRoutine();

  // accessors
  inline ComRoutineType getRoutineType() const;
  inline ComDropBehavior getDropBehavior() const;
  inline const NAString getRoutineName() const;
  inline const QualifiedName & getRoutineNameAsQualifiedName() const;
  inline       QualifiedName & getRoutineNameAsQualifiedName() ;
  inline const NAString getRoutineActionName() const;
  inline const QualifiedName & getRoutineActionNameAsQualifiedName() const;
  inline       QualifiedName & getRoutineActionNameAsQualifiedName() ;
  inline const NABoolean isCleanupSpecified() const;
  inline const NABoolean isValidateSpecified() const;
  inline const NABoolean isLogFileSpecified() const;
  inline const NAString & getLogFile() const;

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;


private:

  ComRoutineType routineType_;
  QualifiedName  routineQualName_;
  QualifiedName  routineActionQualName_;
  ComDropBehavior dropBehavior_;
  NABoolean isCleanupSpec_;
  NABoolean isValidateSpec_;
  NAString  *pLogFile_;


}; // class StmtDDLDropRoutine

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropRoutine
// -----------------------------------------------------------------------

//
// accessors
//

inline ComRoutineType
StmtDDLDropRoutine::getRoutineType() const
{
  return routineType_;
}

inline QualifiedName  &
StmtDDLDropRoutine::getRoutineNameAsQualifiedName()
{
  return routineQualName_;
}

inline const QualifiedName &
StmtDDLDropRoutine::getRoutineNameAsQualifiedName() const
{
  return routineQualName_;
}

inline QualifiedName &
StmtDDLDropRoutine::getRoutineActionNameAsQualifiedName()
{
  return routineActionQualName_;
}

inline const QualifiedName &
StmtDDLDropRoutine::getRoutineActionNameAsQualifiedName() const
{
  return routineActionQualName_;
}

inline ComDropBehavior
StmtDDLDropRoutine::getDropBehavior() const
{
  return dropBehavior_;
}

inline const NAString 
StmtDDLDropRoutine::getRoutineName() const
{
  return routineQualName_.getQualifiedNameAsAnsiString();
}

inline const NAString
StmtDDLDropRoutine::getRoutineActionName() const
{
  return routineActionQualName_.getQualifiedNameAsAnsiString();
}

inline const NABoolean
StmtDDLDropRoutine::isCleanupSpecified()const
{
  return isCleanupSpec_;
}

inline const NABoolean
StmtDDLDropRoutine::isValidateSpecified()const
{
  return isValidateSpec_;
}

inline const NABoolean 
StmtDDLDropRoutine::isLogFileSpecified() const
{
  if (pLogFile_)
    return TRUE;
  return FALSE;
}

inline const NAString &
StmtDDLDropRoutine::getLogFile() const
{
  ComASSERT(pLogFile_ NEQ NULL);
  return *pLogFile_;
}

#endif // STMTDDLDROPROUTINE_H






