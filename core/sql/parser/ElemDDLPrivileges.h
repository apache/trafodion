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
#ifndef ELEMDDLPRIVILEGES_H
#define ELEMDDLPRIVILEGES_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPrivileges.h
 * Description:  class for a parse node representing the privileges
 *               appearing in a Grant DDL statement.
 *               
 *
 * Created:      10/16/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComASSERT.h"
#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLPrivileges;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivileges
// -----------------------------------------------------------------------
class ElemDDLPrivileges : public ElemDDLNode
{

public:

  // constructors
  ElemDDLPrivileges(ElemDDLNode::WhichAll typeOfAll)
  : ElemDDLNode(ELM_PRIVILEGES_ELEM),
  isAllDMLPrivileges_(TRUE),
  isAllDDLPrivileges_(TRUE),
  isAllOtherPrivileges_(TRUE)
  {
    ComASSERT(typeOfAll == ElemDDLNode::ALL);
    setChild(INDEX_PRIVILEGE_ACTION_LIST, NULL);
  }

  ElemDDLPrivileges(ElemDDLNode * pPrivilegeActions)
  : ElemDDLNode(ELM_PRIVILEGES_ELEM),
  isAllDMLPrivileges_(FALSE),
  isAllDDLPrivileges_(FALSE),
  isAllOtherPrivileges_(FALSE)
  {
    ComASSERT(pPrivilegeActions NEQ NULL);
    setChild(INDEX_PRIVILEGE_ACTION_LIST, pPrivilegeActions);
    isAllDMLPrivileges_ = containsPriv(ELM_PRIV_ACT_ALL_DML_ELEM);
    isAllDDLPrivileges_ = containsPriv(ELM_PRIV_ACT_ALL_DDL_ELEM);
    isAllOtherPrivileges_ = containsPriv(ELM_PRIV_ACT_ALL_OTHER_ELEM);
  }

  // virtual destructor
  virtual ~ElemDDLPrivileges();

  // cast
  virtual ElemDDLPrivileges * castToElemDDLPrivileges();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline ElemDDLNode * getPrivilegeActionList() const;

        // returns NULL when the phrase ALL PRIVILEGES appears;
        // otherwise returns a pointer to a parse node representing
        // a privilege action or a list of privilege actions.

        // returns TRUE if the phrase ALL PRIVILEGES appears;
        // returns FALSE otherwise.

  inline NABoolean isAllPrivileges() const;

  inline NABoolean isAllDDLPrivileges() const;

  inline NABoolean isAllDMLPrivileges() const;

  inline NABoolean isAllOtherPrivileges() const;


  // mutator
  virtual void setChild(Lng32 index, ExprNode * pElemDDLNode);
 
  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

  NABoolean containsColumnPrivs() const ;

  NABoolean containsPriv(OperatorTypeEnum whichPriv) const;


private:
  NABoolean isAllDDLPrivileges_;
  NABoolean isAllDMLPrivileges_;
  NABoolean isAllOtherPrivileges_;

  // pointer to child parse node

  enum { INDEX_PRIVILEGE_ACTION_LIST = 0,
         MAX_ELEM_DDL_PRIVILEGES_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_PRIVILEGES_ARITY];

}; // class ElemDDLPrivileges

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLPrivileges
// -----------------------------------------------------------------------

//
// accessors
//

inline ElemDDLNode *
ElemDDLPrivileges::getPrivilegeActionList() const
{
  return children_[INDEX_PRIVILEGE_ACTION_LIST];
}

inline NABoolean
ElemDDLPrivileges::isAllPrivileges() const
{
  return (isAllDDLPrivileges_ && isAllDMLPrivileges_ && isAllOtherPrivileges_);
}

inline NABoolean
ElemDDLPrivileges::isAllDDLPrivileges() const
{
  return isAllDDLPrivileges_;
}

inline NABoolean
ElemDDLPrivileges::isAllDMLPrivileges() const
{
  return isAllDMLPrivileges_;
}

inline NABoolean
ElemDDLPrivileges::isAllOtherPrivileges() const
{
  return isAllOtherPrivileges_;
}

#endif // ELEMDDLPRIVILEGES_H
