/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
#ifndef ELEMDDLLIKEOPTIONS_H
#define ELEMDDLLIKEOPTIONS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLLikeOptions.h
 * Description:  classes for options in Like clause in DDL statements
 *
 *               
 * Created:      6/5/95
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
class ElemDDLLikeOpt;
class ElemDDLLikeOptWithConstraints;
class ElemDDLLikeOptWithHeadings;
class ElemDDLLikeOptWithHorizontalPartitions;
class ElemDDLLikeOptWithDivision;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of base class ElemDDLLikeOpt
// -----------------------------------------------------------------------
class ElemDDLLikeOpt : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLLikeOpt(OperatorTypeEnum operatorType = ELM_ANY_LIKE_OPT_ELEM)
  : ElemDDLNode(operatorType)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOpt();

  // cast
  virtual ElemDDLLikeOpt * castToElemDDLLikeOpt();

  // method for tracing
  virtual const NAString getText() const;

private:

}; // class ElemDDLLikeOpt

// -----------------------------------------------------------------------
// definition of class ElemDDLLikeOptWithConstraints
// -----------------------------------------------------------------------
class ElemDDLLikeOptWithConstraints : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeOptWithConstraints()
  : ElemDDLLikeOpt(ELM_LIKE_OPT_WITH_CONSTRAINTS_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOptWithConstraints();

  // cast
  virtual ElemDDLLikeOptWithConstraints *
    castToElemDDLLikeOptWithConstraints();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLikeOptWithConstraints

// -----------------------------------------------------------------------
// definition of class ElemDDLLikeOptWithHeadings
// -----------------------------------------------------------------------
class ElemDDLLikeOptWithHeadings : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeOptWithHeadings()
  : ElemDDLLikeOpt(ELM_LIKE_OPT_WITH_HEADINGS_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOptWithHeadings();

  // cast
  virtual ElemDDLLikeOptWithHeadings * castToElemDDLLikeOptWithHeadings();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLikeOptWithHeadings


// -----------------------------------------------------------------------
// definition of class ElemDDLLikeOptWithHorizontalPartitions
// -----------------------------------------------------------------------
class ElemDDLLikeOptWithHorizontalPartitions : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeOptWithHorizontalPartitions()
    : ElemDDLLikeOpt(ELM_LIKE_OPT_WITH_HORIZONTAL_PARTITIONS_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOptWithHorizontalPartitions();

  // cast
  virtual ElemDDLLikeOptWithHorizontalPartitions *
    castToElemDDLLikeOptWithHorizontalPartitions();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLikeOptWithHorizontalPartitions



// -----------------------------------------------------------------------
// definition of class ElemDDLLikeOptWithDivision
// -----------------------------------------------------------------------
class ElemDDLLikeOptWithDivision : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeOptWithDivision()
    : ElemDDLLikeOpt(ELM_LIKE_OPT_WITH_DIVISION_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOptWithDivision();

  // cast
  virtual ElemDDLLikeOptWithDivision *
    castToElemDDLLikeOptWithDivision();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLikeOptWithDivision

#endif // ELEMDDLLIKEOPTIONS_H
