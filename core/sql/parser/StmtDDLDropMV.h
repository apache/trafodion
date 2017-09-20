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
#ifndef STMTDDLDROPMV_H
#define STMTDDLDROPMV_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropMV.h
 * Description:  class for parse node representing Drop Mat. View statements
 *
 *
 * Created:      06/11/99
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
class StmtDDLDropMV;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Catalog statement
// -----------------------------------------------------------------------
class StmtDDLDropMV : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropMV(const QualifiedName & tableQualName,
                ComDropBehavior dropBehavior,
                NABoolean cleanupSpec,
                NABoolean validateSpec,
		NAString * pLogFile);

  // virtual destructor
  virtual ~StmtDDLDropMV();

  // cast
  virtual StmtDDLDropMV * castToStmtDDLDropMV();

  // accessors
  inline ComDropBehavior getDropBehavior() const;
  inline const NAString getMVName() const;
  inline const QualifiedName & getMVNameAsQualifiedName() const;
  inline       QualifiedName & getMVNameAsQualifiedName();
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

  QualifiedName MVQualName_;
  ComDropBehavior dropBehavior_;
  NABoolean isCleanupSpec_;
  NABoolean isValidateSpec_;
  NAString  *pLogFile_;

}; // class StmtDDLDropMV

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropMV
// -----------------------------------------------------------------------

//
// accessors
//

inline QualifiedName &
StmtDDLDropMV::getMVNameAsQualifiedName()
{
  return MVQualName_;
}

inline const QualifiedName & 
StmtDDLDropMV::getMVNameAsQualifiedName() const 
{
  return MVQualName_;
}

inline ComDropBehavior
StmtDDLDropMV::getDropBehavior() const
{
  return dropBehavior_;
}

inline const NAString
StmtDDLDropMV::getMVName() const
{
  return MVQualName_.getQualifiedNameAsAnsiString();
}

inline const NABoolean
StmtDDLDropMV::isCleanupSpecified()const
{
  return isCleanupSpec_;
}

inline const NABoolean
StmtDDLDropMV::isValidateSpecified()const
{
  return isValidateSpec_;
}

inline const NABoolean
StmtDDLDropMV::isLogFileSpecified() const
{
  if (pLogFile_)
    return TRUE;
  return FALSE;
}

inline const NAString &
StmtDDLDropMV::getLogFile() const
{
  ComASSERT(pLogFile_ NEQ NULL);
  return *pLogFile_;
}

#endif // STMTDDLDROPMV_H
