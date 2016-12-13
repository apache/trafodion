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
// definition of class ElemDDLLikeOptWithoutConstraints
// -----------------------------------------------------------------------
class ElemDDLLikeOptWithoutConstraints : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeOptWithoutConstraints()
  : ElemDDLLikeOpt(ELM_LIKE_OPT_WITHOUT_CONSTRAINTS_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOptWithoutConstraints();

  // cast
  virtual ElemDDLLikeOptWithoutConstraints *
    castToElemDDLLikeOptWithoutConstraints();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLikeOptWithoutConstraints

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
// definition of class ElemDDLLikeOptWithoutSalt
// -----------------------------------------------------------------------
class ElemDDLLikeOptWithoutSalt : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeOptWithoutSalt()
    : ElemDDLLikeOpt(ELM_LIKE_OPT_WITHOUT_SALT_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOptWithoutSalt();

  // cast
  virtual ElemDDLLikeOptWithoutSalt *
    castToElemDDLLikeOptWithoutSalt();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLikeOptWithoutSalt


// -----------------------------------------------------------------------
// definition of class ElemDDLLikeSaltClause
// -----------------------------------------------------------------------
class ElemDDLLikeSaltClause : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeSaltClause(ElemDDLSaltOptionsClause * saltClause)
    : ElemDDLLikeOpt(ELM_LIKE_OPT_SALT_CLAUSE_ELEM),
      saltClause_(saltClause)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeSaltClause();

  // cast
  virtual ElemDDLLikeSaltClause *
    castToElemDDLLikeSaltClause();

  // method for tracing
  virtual const NAString getText() const;

  ElemDDLSaltOptionsClause * getSaltClause()
  { return saltClause_; };


private:

  ElemDDLSaltOptionsClause * saltClause_;

}; // class ElemDDLLikeOptWithoutSalt

// -----------------------------------------------------------------------
// definition of class ElemDDLLikeOptWithoutDivision
// -----------------------------------------------------------------------
class ElemDDLLikeOptWithoutDivision : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeOptWithoutDivision()
    : ElemDDLLikeOpt(ELM_LIKE_OPT_WITHOUT_DIVISION_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOptWithoutDivision();

  // cast
  virtual ElemDDLLikeOptWithoutDivision *
    castToElemDDLLikeOptWithoutDivision();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLikeOptWithoutDivision

// -----------------------------------------------------------------------
// definition of class ElemDDLLikeLimitColumnLength
// -----------------------------------------------------------------------
class ElemDDLLikeLimitColumnLength : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeLimitColumnLength(UInt32 limit)
    : ElemDDLLikeOpt(ELM_LIKE_OPT_LIMIT_COLUMN_LENGTH),
      columnLengthLimit_(limit)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeLimitColumnLength();

  // cast
  virtual ElemDDLLikeLimitColumnLength *
    castToElemDDLLikeLimitColumnLength();

  // method for tracing
  virtual const NAString getText() const;

  UInt32 getColumnLengthLimit()
  { return columnLengthLimit_; };


private:

  UInt32 columnLengthLimit_; // in bytes

}; // class ElemDDLLikeLimitColumnLength

// -----------------------------------------------------------------------
// definition of class ElemDDLLikeOptWithoutRowFormat
// -----------------------------------------------------------------------
class ElemDDLLikeOptWithoutRowFormat : public ElemDDLLikeOpt
{

public:

  // constructor
  ElemDDLLikeOptWithoutRowFormat()
    : ElemDDLLikeOpt(ELM_LIKE_OPT_WITHOUT_ROW_FORMAT_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLLikeOptWithoutRowFormat();

  // cast
  virtual ElemDDLLikeOptWithoutRowFormat *
    castToElemDDLLikeOptWithoutRowFormat();

  // method for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLLikeOptWithoutRowFormat

#endif // ELEMDDLLIKEOPTIONS_H
