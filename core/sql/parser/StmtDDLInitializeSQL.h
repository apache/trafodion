#ifndef STMTDDLINITIALIZESQL_H
#define STMTDDLINITIALIZESQL_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLInitializeSQL.h
 * Description:  class for parse node representing Initialize SQL
 *               statements
 *
 *
 * Created:      3/9/95
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

#include "ComLocationNames.h"
#include "ElemDDLLocation.h"
#include "StmtDDLNode.h"
#include "StmtDDLRegisterUser.h"
#include "StmtDDLCreateRole.h"


// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class StmtDDLInitializeSQL;
class StmtDDLRegisterUser;
class StmtDDLRegisterUserArray;
class StmtDDLCreateRole;
class StmtDDLCreateRoleArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLInitializeSQL
// -----------------------------------------------------------------------
class StmtDDLInitializeSQL : public StmtDDLNode
{

public:

  friend void StmtDDLInitializeSQL_visitRegisterUserElement
      (ElemDDLNode * pInitSQLNode,
       CollIndex /* index */,
       ElemDDLNode * pElement);

  friend void StmtDDLInitializeSQL_visitCreateRoleElement
      (ElemDDLNode * pInitSQLNode,
       CollIndex /* index */,
       ElemDDLNode * pElement);

  //
  // constructor
  //
  
  StmtDDLInitializeSQL( ElemDDLNode * pCreateRoleList,
                        ElemDDLNode * pRegisterUserList,
                        CollHeap * heap);

  //
  // virtual destructor
  //
  
  virtual ~StmtDDLInitializeSQL();

  //
  // cast
  //
  
  virtual StmtDDLInitializeSQL *castToStmtDDLInitializeSQL();


  //
  // accessors
  //

  inline const  StmtDDLRegisterUserArray * getRegisterUserArray() const
    {   return &registerUserArray_; };

  inline const StmtDDLCreateRoleArray * getCreateRoleArray() const
    {   return &createRoleArray_; };

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  virtual void setChild(Lng32 index, ExprNode * newNode);

  //
  // for binding
  //

  ExprNode * bindNode(BindWA *bindWAPtr);
  void synthesize();

  //
  // methods for tracing
  //

  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:
  
  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  StmtDDLInitializeSQL(const StmtDDLInitializeSQL &);

        // copy constructor not supported
  
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------
  
  StmtDDLRegisterUserArray registerUserArray_;
  StmtDDLCreateRoleArray   createRoleArray_;

  // pointer to child parse node

  enum { INDEX_REGISTER_USER_LIST = 0,
         INDEX_CREATE_ROLE_LIST,
         MAX_STMT_DDL_INITIALIZE_SQL_ARITY };

  ElemDDLNode * children_[MAX_STMT_DDL_INITIALIZE_SQL_ARITY];

}; // class StmtDDLInitializeSQL

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLInitializeSQL
// -----------------------------------------------------------------------

//
// accessors
//

#endif // STMTDDLINITIALIZESQL_H


