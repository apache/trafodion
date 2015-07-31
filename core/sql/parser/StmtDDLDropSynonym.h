#ifndef STMTDDLDROPSYNONYM_H
#define STMTDDLDROPSYNONYM_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropSynonym.h
 * Description:  class for parse node representing Drop Synonym statements
 *
 *
 * Created:      01/27/06
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLDropSynonym;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Drop Synonym statement
// -----------------------------------------------------------------------
class StmtDDLDropSynonym : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropSynonym();
  StmtDDLDropSynonym(const QualifiedName & synonymName);

  // virtual destructor
  virtual ~StmtDDLDropSynonym();

  // cast
  virtual StmtDDLDropSynonym * castToStmtDDLDropSynonym();

  // accessors

  inline const NAString getSynonymName() const;

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

 // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

private:
  QualifiedName synonymName_;

}; // class StmtDDLDropSynonym

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropSynonym
// -----------------------------------------------------------------------

//
// accessors
//

inline const NAString
StmtDDLDropSynonym::getSynonymName() const
{
  NAString synonymName = synonymName_.getQualifiedNameAsAnsiString();
  return synonymName;
}

#endif // STMTDDLDROPSYNONYM_H






