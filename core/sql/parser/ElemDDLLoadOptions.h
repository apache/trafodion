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
#ifndef ELEMDDLLOADOPTIONS_H
#define ELEMDDLLOADOPTIONS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLLoadOptions.h
 * Description:  classes for load options specified in DDL statements
 *
 *               
 * Created:      9/28/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "BaseTypes.h"
#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLLoadOpt;
class ElemDDLLoadOptDSlack;
class ElemDDLLoadOptISlack;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of base class ElemDDLLoadOpt
// -----------------------------------------------------------------------
class ElemDDLLoadOpt : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLLoadOpt(OperatorTypeEnum operatorType = ELM_ANY_LOAD_OPT_ELEM)
  : ElemDDLNode(operatorType)
  { }

  // virtual destructor
  virtual ~ElemDDLLoadOpt();

  // cast
  virtual ElemDDLLoadOpt * castToElemDDLLoadOpt();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLoadOpt

// -----------------------------------------------------------------------
// definition of class ElemDDLLoadOptDSlack
// -----------------------------------------------------------------------
class ElemDDLLoadOptDSlack : public ElemDDLLoadOpt
{

public:

  enum { DEFAULT_PERCENTAGE = 15 };

  // constructor
  ElemDDLLoadOptDSlack(Lng32 percentage);

  // virtual destructor
  virtual ~ElemDDLLoadOptDSlack();

  // cast
  virtual ElemDDLLoadOptDSlack * castToElemDDLLoadOptDSlack();

  // accessors
  inline unsigned short getPercentage() const;

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;


  // method for building text
  virtual NAString getSyntax() const;


private:

  //
  // method(s)
  //

  NABoolean isLegalPercentageValue(Lng32 percentage);

  //
  // data member(s)
  //

  unsigned short percentage_;

}; // class ElemDDLLoadOptDSlack

// -----------------------------------------------------------------------
// definition of class ElemDDLLoadOptISlack
// -----------------------------------------------------------------------
class ElemDDLLoadOptISlack : public ElemDDLLoadOpt
{

public:

  enum { DEFAULT_PERCENTAGE = 15 };

  // constructor
  ElemDDLLoadOptISlack(Lng32 percentage);

  // virtual destructor
  virtual ~ElemDDLLoadOptISlack();

  // cast
  virtual ElemDDLLoadOptISlack * castToElemDDLLoadOptISlack();

  // accessors
  inline unsigned short getPercentage() const;

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;

private:

  //
  // method(s)
  //

  NABoolean isLegalPercentageValue(Lng32 percentage);

  //
  // data member(s)
  //

  unsigned short percentage_;

}; // class ElemDDLLoadOptISlack


// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLLoadOptDSlack
// -----------------------------------------------------------------------

inline unsigned short
ElemDDLLoadOptDSlack::getPercentage() const
{
  return percentage_;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLLoadOptISlack
// -----------------------------------------------------------------------

inline unsigned short
ElemDDLLoadOptISlack::getPercentage() const
{
  return percentage_;
}

#endif // ELEMDDLLOADOPTIONS_H
