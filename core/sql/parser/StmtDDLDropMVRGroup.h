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
#ifndef STMTDDLDROPMRVGROUP_H
#define STMTDDLDROPMRVGROUP_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropMvRGroup.h
 * Description:  class representing Create MV group Statement parser nodes
 *
 *
 * Created:      5/06/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ElemDDLNode.h"
#include "NAString.h"

#include "StmtDDLNode.h"
#include "ComSmallDefs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLDropMvRGroup;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create MV group statement
// -----------------------------------------------------------------------
class StmtDDLDropMvRGroup : public StmtDDLNode
{

public:

  // initialize constructor
  StmtDDLDropMvRGroup(const QualifiedName & mvGroupName);
                      //CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLDropMvRGroup();

  // cast
  virtual StmtDDLDropMvRGroup * castToStmtDDLDropMvRGroup();

  //
  // accessors
  //


  inline const NAString getMvRGroupName() const;
  inline const QualifiedName & getMvRGroupNameAsQualifiedName() const;
  inline       QualifiedName & getMvRGroupNameAsQualifiedName() ;

  
  //
  // mutators
  //

  //
  // method for binding
  //

  ExprNode * bindNode(BindWA * pBindWA);

  //
  // methods for tracing
  //

  virtual const NAString displayLabel1() const;
//  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------


//  NAString mvRGroupName_;

  // The syntax of table name is
  // [ [ catalog-name . ] schema-name . ] table-name
  QualifiedName mvRGroupQualName_;


}; // class StmtDDLDropMvRGroup 

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateMvGroup 
// -----------------------------------------------------------------------


// get index name
inline const NAString 
StmtDDLDropMvRGroup::getMvRGroupName() const
{
  // return mvRGroupName_;
  return mvRGroupQualName_.getQualifiedNameAsAnsiString();
}


inline QualifiedName &
StmtDDLDropMvRGroup::getMvRGroupNameAsQualifiedName()
{
  return mvRGroupQualName_;
}

inline const QualifiedName & 
StmtDDLDropMvRGroup::getMvRGroupNameAsQualifiedName() const
{
  return mvRGroupQualName_;
}




#endif // STMTDDLDROPMRVGROUP_H
