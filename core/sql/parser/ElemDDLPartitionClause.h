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
#ifndef ELEMDDLPARTITIONCLAUSE_H
#define ELEMDDLPARTITIONCLAUSE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPartitionClause.h
 * Description:  class for parse nodes representing partition clauses
 *               in DDL statements.  Note that this class is derived
 *               from class ElemDDLNode instead of class ElemDDLPartition.
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
#include "ComSmallDefs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLPartitionClause;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLPartitionClause
// -----------------------------------------------------------------------
class ElemDDLPartitionClause : public ElemDDLNode
{

public:

  // constructor
  ElemDDLPartitionClause(ElemDDLNode * pPartitionDefBody
                                , ElemDDLNode * pPartitionByOption  
                                , ComPartitioningScheme partitionType)
  : ElemDDLNode(ELM_PARTITION_CLAUSE_ELEM)
  {
    setChild(INDEX_PARTITION_DEFINITION_BODY, pPartitionDefBody);
    setChild(INDEX_PARTITION_BY_OPTION, pPartitionByOption);
    partitionType_ = partitionType;
  }

  // virtual destructor
  virtual ~ElemDDLPartitionClause();

  // cast
  virtual ElemDDLPartitionClause * castToElemDDLPartitionClause();

  // accessors
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);
  inline ElemDDLNode * getPartitionDefBody() const;
  inline ElemDDLNode * getPartitionByOption() const;
  inline ComPartitioningScheme getPartitionType() const;

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pElemDDLNode);
 
  // method for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;



private:

  // pointers to child parse nodes

  enum { INDEX_PARTITION_DEFINITION_BODY = 0,
         INDEX_PARTITION_BY_OPTION,
         MAX_ELEM_DDL_PARTITION_CLAUSE_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_PARTITION_CLAUSE_ARITY];

  ComPartitioningScheme partitionType_;                         

}; // class ElemDDLPartitionClause

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLPartitionClause
// -----------------------------------------------------------------------


inline ElemDDLNode *
ElemDDLPartitionClause::getPartitionDefBody() const
{
  return children_[INDEX_PARTITION_DEFINITION_BODY];
}

inline ElemDDLNode *
ElemDDLPartitionClause::getPartitionByOption() const
{
  return children_[INDEX_PARTITION_BY_OPTION];
}

inline ComPartitioningScheme 
ElemDDLPartitionClause::getPartitionType() const 
{ 
  return partitionType_; 
}   

#endif // ELEMDDLPARTITIONCLAUSE_H
