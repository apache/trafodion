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
#ifndef STMTDDLCREATEMVRGROUP_H
#define STMTDDLCREATEMVRGROUP_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateMvRGroup.h
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
class StmtDDLCreateMvRGroup;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create MV group statement
// -----------------------------------------------------------------------
class StmtDDLCreateMvRGroup : public StmtDDLNode
{

public:

  // initialize constructor
  StmtDDLCreateMvRGroup(const QualifiedName & mvGroupName,
                      CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateMvRGroup();

  // cast
  virtual StmtDDLCreateMvRGroup * castToStmtDDLCreateMvRGroup();

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


}; // class StmtDDLCreateMvGroup 

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateMvGroup 
// -----------------------------------------------------------------------


inline const NAString 
StmtDDLCreateMvRGroup::getMvRGroupName() const
{
  return mvRGroupQualName_.getQualifiedNameAsAnsiString();
}


inline QualifiedName &
StmtDDLCreateMvRGroup::getMvRGroupNameAsQualifiedName()
{
  return mvRGroupQualName_;
}

inline const QualifiedName & 
StmtDDLCreateMvRGroup::getMvRGroupNameAsQualifiedName() const
{
  return mvRGroupQualName_;
}




#endif // STMTDDLCREATEMRVGROUP_H
