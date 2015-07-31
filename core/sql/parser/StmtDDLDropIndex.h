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
#ifndef STMTDDLDROPINDEX_H
#define STMTDDLDROPINDEX_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropIndex.h
 * Description:  class for parse node representing Drop Index statements
 *
 *
 * Created:      11/14/95
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
class StmtDDLDropIndex;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Catalog statement
// -----------------------------------------------------------------------
class StmtDDLDropIndex : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropIndex(const QualifiedName & tableQualName,
                   ComDropBehavior dropBehavior,
                   NABoolean cleanupSpec = FALSE,
                   NABoolean validateSpec = FALSE,
		   NAString * pLogFile = NULL);

  // virtual destructor
  virtual ~StmtDDLDropIndex();

  // cast
  virtual StmtDDLDropIndex * castToStmtDDLDropIndex();

  void synthesize();

  // accessor
  inline const QualifiedName & getOrigIndexNameAsQualifiedName() const;
  inline       QualifiedName & getOrigIndexNameAsQualifiedName();

  inline const NAString getIndexName() const;
  inline const QualifiedName & getIndexNameAsQualifiedName() const;
  inline       QualifiedName & getIndexNameAsQualifiedName() ;
  inline ComDropBehavior getDropBehavior() const;
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
  // the indexname specified by user in the drop stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origIndexQualName_;

  QualifiedName indexQualName_;
  ComDropBehavior dropBehavior_;
  NABoolean isCleanupSpec_;
  NABoolean isValidateSpec_;
  NAString *pLogFile_;

}; // class StmtDDLDropIndex

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropIndex
// -----------------------------------------------------------------------

//
// accessor
//

inline QualifiedName &
StmtDDLDropIndex::getOrigIndexNameAsQualifiedName()
{
  return origIndexQualName_;
}

inline const QualifiedName &
StmtDDLDropIndex::getOrigIndexNameAsQualifiedName() const
{
  return origIndexQualName_;
}

inline QualifiedName  &
StmtDDLDropIndex::getIndexNameAsQualifiedName()
{
  return indexQualName_;
}

inline const QualifiedName &
StmtDDLDropIndex::getIndexNameAsQualifiedName() const
{
  return indexQualName_;
}


inline const NAString 
StmtDDLDropIndex::getIndexName() const
{
  return indexQualName_.getQualifiedNameAsAnsiString();
}

inline ComDropBehavior
StmtDDLDropIndex::getDropBehavior() const
{
  return dropBehavior_;
}

inline const NABoolean
StmtDDLDropIndex::isCleanupSpecified()const
{
  return isCleanupSpec_;
}

inline const NABoolean
StmtDDLDropIndex::isValidateSpecified()const
{
  return isValidateSpec_;
}

inline const NABoolean
StmtDDLDropIndex::isLogFileSpecified() const
{
  if (pLogFile_ == NULL)
    return FALSE;
  else
    return TRUE;
}

inline const NAString &
StmtDDLDropIndex::getLogFile() const
{
  ComASSERT(pLogFile_ NEQ NULL);
  return *pLogFile_;
}

#endif // STMTDDLDROPINDEX_H






