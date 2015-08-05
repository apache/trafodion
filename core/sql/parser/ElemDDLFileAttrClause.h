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
#ifndef ELEMDDLFILEATTRCLAUSE_H
#define ELEMDDLFILEATTRCLAUSE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttrClause.h
 * Description:  class for parse nodes representing file Attribute(S)
 *               clauses in DDL statements.  Note that this class is
 *               derived from class ElemDDLNode instead of class
 *               ElemDDLFileAttr.
 *               
 *
 * Created:      10/4/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrClause;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// A fileAttrClause of elements in DDL statement.
// -----------------------------------------------------------------------
class ElemDDLFileAttrClause : public ElemDDLNode
{

public:

  // constructor
  ElemDDLFileAttrClause(ElemDDLNode * pFileAttrDefBody)
  : ElemDDLNode(ELM_FILE_ATTR_CLAUSE_ELEM)
{
  setChild(INDEX_FILE_ATTR_DEFINITION_BODY, pFileAttrDefBody);
}

  // virtual destructor
  virtual ~ElemDDLFileAttrClause();

  // cast
  virtual ElemDDLFileAttrClause * castToElemDDLFileAttrClause();

  // accessors
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);
  inline ElemDDLNode * getFileAttrDefBody() const;

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pElemDDLNode);
 
  // methods for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;



private:

  // pointers to child parse nodes

  enum { INDEX_FILE_ATTR_DEFINITION_BODY = 0,
         MAX_ELEM_DDL_FILE_ATTR_CLAUSE_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_FILE_ATTR_CLAUSE_ARITY];

}; // class ElemDDLFileAttrClause

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLFileAttrClause
// -----------------------------------------------------------------------

inline ElemDDLNode *
ElemDDLFileAttrClause::getFileAttrDefBody() const
{
  return (((ElemDDLFileAttrClause *)this)->
          getChild(INDEX_FILE_ATTR_DEFINITION_BODY)->castToElemDDLNode());
}

#endif // ELEMDDLFILEATTRCLAUSE_H
