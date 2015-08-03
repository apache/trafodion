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
#ifndef ELEMDDLLIKE_H
#define ELEMDDLLIKE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLLike.h
 * Description:  base class for Like clause in DDL statements
 *
 *
 * Created:      3/29/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "ObjectNames.h"
#ifndef SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "SqlParserGlobals.h"
// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLLike;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLLike
// -----------------------------------------------------------------------
class ElemDDLLike : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLLike(OperatorTypeEnum operatorType = ELM_ANY_LIKE_ELEM,
                     const CorrName & sourceTableName = 
                           CorrName("", PARSERHEAP()) ,
                     ElemDDLNode * pLikeOptions = NULL,
                     CollHeap * h=0)
  : ElemDDLNode(operatorType),
  sourceTableName_(h),
  sourceTableCorrName_(sourceTableName, h),
  pLikeOptions_(pLikeOptions)
  {
   sourceTableName_ =  
     sourceTableCorrName_.getQualifiedNameObj().getQualifiedNameAsAnsiString();
  }

  // copy ctor
  ElemDDLLike (const ElemDDLLike & orig, CollHeap * h=0 ) ; // not written

  // virtual destructor
  virtual ~ElemDDLLike();

  // cast
  virtual ElemDDLLike * castToElemDDLLike();

  // accessors
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);
  inline const NAString & getSourceTableName() const;
  inline const CorrName & getDDLLikeNameAsCorrName() const;
  inline       CorrName & getDDLLikeNameAsCorrName();
  // mutators
  virtual void setChild(Lng32 index, ExprNode * pChildNode);
  inline void setSourceTableName(const NAString & sourceTableName);

  // methods for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;

  // methods for binding.

  ExprNode * bindNode(BindWA *bindWAPtr);



private:

  NAString sourceTableName_;
  CorrName sourceTableCorrName_; // ++ MV - see comment at top of file
  // pointer to child parse node

  enum { INDEX_LIKE_OPT_LIST = 0,
         MAX_ELEM_DDL_LIKE_ARITY };

  ElemDDLNode * pLikeOptions_;

}; // class ElemDDLLike

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLLike
// -----------------------------------------------------------------------
//
// accessor
//

inline CorrName &
ElemDDLLike::getDDLLikeNameAsCorrName()
{
  return sourceTableCorrName_;
}

inline const CorrName & 
ElemDDLLike::getDDLLikeNameAsCorrName() const 
{
  return sourceTableCorrName_;
}

inline const NAString &
ElemDDLLike::getSourceTableName() const
{
  return sourceTableName_;
}

//
// mutator
//

inline void
ElemDDLLike::setSourceTableName(const NAString & sourceTableName)
{
  sourceTableName_ = sourceTableName;
}

#endif // ELEMDDLLIKE_H
