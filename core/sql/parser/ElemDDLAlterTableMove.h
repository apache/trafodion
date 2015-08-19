#ifndef ELEMDDLALTERTABLEMOVE_H
#define ELEMDDLALTERTABLEMOVE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLAlterTableMove.h
 * Description:  class for Move clause in Alter Table <table-name>
 *               DDL statements
 *
 *               
 * Created:      9/20/95
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


#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLAlterTableMove;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Key Value elements in DDL statements.
// -----------------------------------------------------------------------
class ElemDDLAlterTableMove : public ElemDDLNode
{

public:

  // constructor
  ElemDDLAlterTableMove(ElemDDLNode * sourceLocationList,
                        ElemDDLNode * destLocationList);

  // virtual destructor
  virtual ~ElemDDLAlterTableMove();

  // cast
  virtual ElemDDLAlterTableMove * castToElemDDLAlterTableMove();  

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline ElemDDLNode * getSourceLocationList() const;
  inline ElemDDLNode * getDestLocationList() const;

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  // method for tracing
  virtual const NAString getText() const;


private:

  // pointers to child parse nodes

  enum { INDEX_SOURCE_LOCATION_LIST = 0,
         INDEX_DEST_LOCATION_LIST,
         MAX_ELEM_DDL_ALTER_TABLE_MOVE_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_ALTER_TABLE_MOVE_ARITY];
  
}; // class ElemDDLAlterTableMove

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLAlterTableMove
// -----------------------------------------------------------------------

inline
ElemDDLNode *
ElemDDLAlterTableMove::getSourceLocationList() const
{
  return children_[INDEX_SOURCE_LOCATION_LIST];
}

inline
ElemDDLNode *
ElemDDLAlterTableMove::getDestLocationList() const
{
  return children_[INDEX_DEST_LOCATION_LIST];
}

#endif // ELEMDDLALTERTABLEMOVE_H
