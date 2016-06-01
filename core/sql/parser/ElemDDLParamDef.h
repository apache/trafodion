/* -*-C++-*- */
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
#ifndef ELEMDDLPARAMDEF_H
#define ELEMDDLPARAMDEF_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLParamDef.h
 * Description:  class for Routine parameters in DDL statements
 *               
 *               
 * Created:      10/01/1999
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

#include "ElemDDLNode.h"
#include "ElemDDLParamName.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLParamDef;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class NAType;
class ItemExpr;

// -----------------------------------------------------------------------
// Param Definition elements in DDL statements.
// -----------------------------------------------------------------------
class ElemDDLParamDef : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLParamDef(NAType * pParamDataType,
                  ElemDDLParamName * paramName,
                  ComParamDirection paramDirection,
                  CollHeap * heap = PARSERHEAP());

	
  // copy ctor
  ElemDDLParamDef (const ElemDDLParamDef & orig, CollHeap * h=0) ; // not written

  // virtual destructor
  virtual ~ElemDDLParamDef();

  // cast
  virtual ElemDDLParamDef * castToElemDDLParamDef();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline NAType * getParamDataType(void) const;
  inline const NAString & getParamName(void) const;
  inline const ComParamDirection getParamDirection(void) const;

  //
  // mutators
  //

  inline void setParamDirection(const ComParamDirection direction);

  // virtual void setChild(long index, ExprNode * pChildNode);

  //
  // methods for tracing
  //
  
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  void setParamAttribute(ElemDDLNode * pParamAttribute);

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  NAString paramName_;
  NAType * paramDataType_;
  ComParamDirection paramDirection_; 

  // no child parse nodes
  //
 
  enum { MAX_ELEM_DDL_PARAM_DEF_ARITY = 0 };

  // ElemDDLNode * children_[MAX_ELEM_DDL_COL_DEF_ARITY];
  
}; // class ElemDDLParamDef

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLParamDef
// -----------------------------------------------------------------------

inline NAType *
ElemDDLParamDef::getParamDataType() const
{
  return paramDataType_;
}

inline const NAString &
ElemDDLParamDef::getParamName() const
{
  return paramName_;
}

inline const ComParamDirection
ElemDDLParamDef::getParamDirection() const
{
  return paramDirection_;
}


//
// mutators
//

inline void 
ElemDDLParamDef::setParamDirection(const ComParamDirection direction)
{
  paramDirection_ = direction;
}


#endif // ELEMDDLPARAMDEF_H
