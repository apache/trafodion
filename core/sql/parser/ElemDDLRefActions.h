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
#ifndef ELEMDDLREFACTIONS_H
#define ELEMDDLREFACTIONS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLRefActions.h
 * Description:  classes for referential actions specified in
 *               References clause in DDL statements
 *
 *               
 * Created:      10/26/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "NAString.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLRefAct;
class ElemDDLRefActCascade;
class ElemDDLRefActNoAction;
class ElemDDLRefActRestrict;
class ElemDDLRefActSetDefault;
class ElemDDLRefActSetNull;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of base class ElemDDLRefAct
// -----------------------------------------------------------------------
class ElemDDLRefAct : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLRefAct(OperatorTypeEnum operatorType = ELM_ANY_REF_ACT_ELEM)
  : ElemDDLNode(operatorType)
  { }

  // virtual destructor
  virtual ~ElemDDLRefAct();

  // cast
  virtual ElemDDLRefAct * castToElemDDLRefAct();

  // methods for tracing
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLRefAct

// -----------------------------------------------------------------------
// definition of class ElemDDLRefActCascade
// -----------------------------------------------------------------------
class ElemDDLRefActCascade : public ElemDDLRefAct
{

public:

  // constructor
  ElemDDLRefActCascade()
  : ElemDDLRefAct(ELM_REF_ACT_CASCADE_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLRefActCascade();

  // cast
  virtual ElemDDLRefActCascade * castToElemDDLRefActCascade();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLRefActCascade

// -----------------------------------------------------------------------
// definition of class ElemDDLRefActNoAction
// -----------------------------------------------------------------------
class ElemDDLRefActNoAction : public ElemDDLRefAct
{

public:

  // constructor
  ElemDDLRefActNoAction()
  : ElemDDLRefAct(ELM_REF_ACT_NO_ACTION_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLRefActNoAction();

  // cast
  virtual ElemDDLRefActNoAction * castToElemDDLRefActNoAction();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLRefActNoAction


// -----------------------------------------------------------------------
// definition of class ElemDDLRefActRestrict
// -----------------------------------------------------------------------
class ElemDDLRefActRestrict : public ElemDDLRefAct
{

public:

  // constructor
  ElemDDLRefActRestrict()
  : ElemDDLRefAct(ELM_REF_ACT_RESTRICT_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLRefActRestrict();

  // cast
  virtual ElemDDLRefActRestrict * castToElemDDLRefActRestrict();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLRefActRestrict


// -----------------------------------------------------------------------
// definition of class ElemDDLRefActSetDefault
// -----------------------------------------------------------------------
class ElemDDLRefActSetDefault : public ElemDDLRefAct
{

public:

  // constructor
  ElemDDLRefActSetDefault()
  : ElemDDLRefAct(ELM_REF_ACT_SET_DEFAULT_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLRefActSetDefault();

  // cast
  virtual ElemDDLRefActSetDefault * castToElemDDLRefActSetDefault();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLRefActSetDefault

// -----------------------------------------------------------------------
// definition of class ElemDDLRefActSetNull
// -----------------------------------------------------------------------
class ElemDDLRefActSetNull : public ElemDDLRefAct
{

public:

  // constructor
  ElemDDLRefActSetNull()
  : ElemDDLRefAct(ELM_REF_ACT_SET_NULL_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLRefActSetNull();

  // cast
  virtual ElemDDLRefActSetNull * castToElemDDLRefActSetNull();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLRefActSetNull


#endif // ELEMDDLREFACTIONS_H
