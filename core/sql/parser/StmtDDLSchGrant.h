#ifndef STMTDDLSCHGRANT_H
#define STMTDDLSCHGRANT_H

/* -*-C++-*-
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
 *
 * File:         StmtDDLSchGrant.h
 * Description:  class for parse nodes representing Grant Schema privileges
		 DDL statements
 *
 *
 * Created:      03/07/07
 * Language:     C++
 *
 
**********************************************************************/


#include "ElemDDLGranteeArray.h"
#include "StmtDDLNode.h"
#include "ElemDDLSchemaName.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLSchGrant;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class StmtDDLSchGrant
// -----------------------------------------------------------------------
class StmtDDLSchGrant : public StmtDDLNode
{

public:

  // constructor
  StmtDDLSchGrant(ElemDDLNode * pPrivileges,
               const ElemDDLSchemaName & aSchemaNameParseNode,
               ElemDDLNode * pGranteeList,
               ElemDDLNode * pWithGrantOption,
	       ElemDDLNode * pByGrantorOption,
               CollHeap    * heap = PARSERHEAP());
  
  // virtual destructor
  virtual ~StmtDDLSchGrant();

  // cast
  virtual StmtDDLSchGrant * castToStmtDDLSchGrant();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline const ElemDDLGranteeArray & getGranteeArray() const;
  inline       ElemDDLGranteeArray & getGranteeArray();
  
  // return the schema name in the Grant statement
  inline NAString getSchemaName() const;

  // returns the array of pointer pointing to parse
  // nodes representing privilege actions.  If the All
  // Privileges phrase appears in the Grant statement,
  // the returned array is empty.  The method
  // isAllPrivileges() may be used to find out whether
  // the All Privileges phrase appears or not.
  inline const ElemDDLPrivActArray & getPrivilegeActionArray() const;
  inline       ElemDDLPrivActArray & getPrivilegeActionArray();            

  // returns TRUE if the All Privileges phrase appears in the Grant 
  // Schema privileges statement; returns FALSE otherwise.
  // Note that getPrivilegeActionArray() returns an
  // empty array when the the All Privileges phrase
  // appears in the Grant schema privileges statement.
  inline NABoolean isAllPrivilegesSpecified() const;

  inline NABoolean isAllDMLPrivilegesSpecified() const;

  inline NABoolean isAllDDLPrivilegesSpecified() const;
       
  inline NABoolean isAllOtherPrivilegesSpecified() const;

  // returns TRUE if the With Grant Option phrase appears
  // in the Grant statement; returns FALSE otherwise.
  inline NABoolean isWithGrantOptionSpecified() const;
   
  // returns TRUE if the BY <grantor> phrase appears
  // in the GRANT SCHEMA statement; returns FALSE otherwise.
  inline NABoolean isByGrantorOptionSpecified() const;

  // returns pointer to the optional "by grantor"
  // (in the form of an ElemDDLGrantee).
  inline const ElemDDLGrantee * getByGrantor() const;

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  // for processing
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:
  
  NAString schemaName_;
  SchemaName schemaQualName_;

  // privilege actions
  NABoolean isAllDMLPrivileges_;
  NABoolean isAllDDLPrivileges_;
  NABoolean isAllOtherPrivileges_;
  ElemDDLPrivActArray privActArray_;

  // grantees
  ElemDDLGranteeArray granteeArray_;

  // with grant option
  NABoolean isWithGrantOptionSpec_;
  
  // by grantor option specified?
  NABoolean isByGrantorOptionSpec_;

  // by grantor option value
  ElemDDLGrantee * byGrantor_;

  // pointers to child parse nodes

  enum { INDEX_PRIVILEGES = 0,
         INDEX_GRANTEE_LIST,
         INDEX_WITH_GRANT_OPTION,
	 INDEX_BY_GRANTOR_OPTION,
         MAX_STMT_DDL_GRANT_ARITY };

  ElemDDLNode * children_[MAX_STMT_DDL_GRANT_ARITY];

}; // class StmtDDLSchGrant


inline ElemDDLGranteeArray &
StmtDDLSchGrant::getGranteeArray()
{
  return granteeArray_;
}

inline const ElemDDLGranteeArray &
StmtDDLSchGrant::getGranteeArray() const
{
  return granteeArray_;
}

inline ElemDDLPrivActArray &
StmtDDLSchGrant::getPrivilegeActionArray()
{
  return privActArray_;
}

inline const ElemDDLPrivActArray &
StmtDDLSchGrant::getPrivilegeActionArray() const
{
  return privActArray_;
}

inline NAString 
StmtDDLSchGrant::getSchemaName() const

{
  return schemaName_;
}

inline NABoolean
StmtDDLSchGrant::isAllPrivilegesSpecified() const
{
  return (isAllDMLPrivileges_ && isAllDDLPrivileges_ && isAllOtherPrivileges_);
}
inline NABoolean
StmtDDLSchGrant::isAllDMLPrivilegesSpecified() const
{
  return isAllDMLPrivileges_;
}
inline NABoolean
StmtDDLSchGrant::isAllDDLPrivilegesSpecified() const
{
  return isAllDDLPrivileges_;
}
inline NABoolean
StmtDDLSchGrant::isAllOtherPrivilegesSpecified() const
{
  return isAllOtherPrivileges_;
}


inline NABoolean
StmtDDLSchGrant::isWithGrantOptionSpecified() const
{
  return isWithGrantOptionSpec_;
}

inline NABoolean
StmtDDLSchGrant::isByGrantorOptionSpecified() const
{
  return isByGrantorOptionSpec_;
}

inline const ElemDDLGrantee *
StmtDDLSchGrant::getByGrantor() const
{
  return byGrantor_;
}

#endif // STMTDDLSCHGRANT_H
