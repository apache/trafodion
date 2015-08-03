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
#ifndef ELEMDDLCOLHEADING_H
#define ELEMDDLCOLHEADING_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLColHeading.h
 * Description:  class for Column Heading elements in DDL statements
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


#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLColHeading;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Column Heading elements in DDL statements
// -----------------------------------------------------------------------
class ElemDDLColHeading : public ElemDDLNode
{

public:

  enum colHeadingTypeEnum { COL_NO_HEADING,
                            COL_HEADING };

  enum { maxHeadingLength = 128 };

  // default constructor
  ElemDDLColHeading(colHeadingTypeEnum headingType = COL_NO_HEADING,
                           const NAString & heading = NAString())
  : ElemDDLNode(ELM_COL_HEADING_ELEM),
  headingType_(headingType),
  heading_(heading, PARSERHEAP())
  { }


  // virtual destructor
  virtual ~ElemDDLColHeading();

  // cast
  virtual ElemDDLColHeading * castToElemDDLColHeading();

  // accessors
  inline const NAString & getColumnHeading() const;
  inline colHeadingTypeEnum getColumnHeadingType() const;

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;


private:

  colHeadingTypeEnum headingType_;
  NAString heading_;

}; // class ElemDDLColHeading

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLColHeading
// -----------------------------------------------------------------------
//
// accessors
//

inline const NAString &
ElemDDLColHeading::getColumnHeading() const
{
  return heading_;
}

inline ElemDDLColHeading::colHeadingTypeEnum
ElemDDLColHeading::getColumnHeadingType() const
{
  return headingType_;
}

#endif // ELEMDDLCOLHEADING_H
