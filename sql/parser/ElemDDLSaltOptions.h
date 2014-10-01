#ifndef ELEMDDLSALTOPTIONS_H
#define ELEMDDLSALTOPTIONS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLSaltOptions.h
 * Description:  Classes representing SALT BY clause specified in
 *               create DDL for HBase objects
 *
 *               
 * Created:      8/27/2013
 * Language:     C++
 *
 *
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
 *
 *
 *****************************************************************************
 */

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"
#include "ParNameLocList.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLSaltOptionsClause;

// -----------------------------------------------------------------------
// class ElemDDLSaltOptionsClause 
// -----------------------------------------------------------------------

class ElemDDLSaltOptionsClause : public ElemDDLNode
{

public:

  //
  // constructors
  //

  ElemDDLSaltOptionsClause(ElemDDLNode * pSaltExprTree,
                           Int32 numPartitions);

  ElemDDLSaltOptionsClause( NABoolean likeTable);

  // virtual destructor
  virtual ~ElemDDLSaltOptionsClause();

  // casting
  virtual ElemDDLSaltOptionsClause * castToElemDDLSaltOptionsClause();

  //
  // accessors
  //

  inline Int32 getNumPartitions() const
  {
    return numPartitions_;
  }

  inline ElemDDLColRefArray & getSaltColRefArray()
  {
    return saltColumnArray_;
  }

  // get the degree of this node
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);
  // name we choose for the system column that contains the salting value
  static const char * getSaltSysColName() { return "_SALT_"; }

  //
  // mutators
  //

  void setChild(Lng32 index, ExprNode * pChildNode);

  void setNumPartns(Int32 numPartns) {numPartitions_ = numPartns;}

  //
  // methods for tracing and/or building text
  //

  virtual const NAString getText() const;

  NABoolean getLikeTable() const;

private:

  //
  // Private methods
  //

  ElemDDLSaltOptionsClause(const ElemDDLSaltOptionsClause & rhs) ; // not defined - DO NOT USE
  ElemDDLSaltOptionsClause & operator=(const ElemDDLSaltOptionsClause & rhs) ; // not defined - DO NOT USE

  //
  // data members
  //

  Int32 numPartitions_;
  ElemDDLColRefArray saltColumnArray_;
  NABoolean likeTable_; // salt an index like its base table

  // pointers to child parse node

  enum { INDEX_SALT_COLUMN_LIST = 0,
         MAX_ELEM_DDL_SALT_OPT_KEY_COLUMN_LIST_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_SALT_OPT_KEY_COLUMN_LIST_ARITY];

}; // class ElemDDLSaltOptionsClause

#endif // ELEMDDLSALTOPTIONSCLAUSE_H
