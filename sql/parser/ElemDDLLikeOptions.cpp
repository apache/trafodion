/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ElemDDLLikeOptions.C
 * Description:  methods for class ElemDDLLikeOpt and any classes
 *               derived from class ElemDDLLikeOpt.
 *
 *               Please note that class ElemDDLLikeOpt is not derived
 *               from class ElemDDLLike.  Class ElemDDLLike and classes
 *               derived from class ElemDDLLike are defined in files
 *               ElemDDLLike.C, ElemDDLLike.h, ElemDDLLikeCreateTable,
 *               etc.
 *
 * Created:      6/5/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ElemDDLLikeOptions.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLLikeOpt
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOpt::~ElemDDLLikeOpt()
{
}

// casting
ElemDDLLikeOpt *
ElemDDLLikeOpt::castToElemDDLLikeOpt()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLikeOpt::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLLikeOpt";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLLikeOptWithConstraints
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithConstraints::~ElemDDLLikeOptWithConstraints()
{
}

// casting
ElemDDLLikeOptWithConstraints *
ElemDDLLikeOptWithConstraints::castToElemDDLLikeOptWithConstraints()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLikeOptWithConstraints::getText() const
{
  return "ElemDDLLikeOptWithConstraints";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLLikeOptWithHeadings
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithHeadings::~ElemDDLLikeOptWithHeadings()
{
}

// casting
ElemDDLLikeOptWithHeadings *
ElemDDLLikeOptWithHeadings::castToElemDDLLikeOptWithHeadings()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLikeOptWithHeadings::getText() const
{
  return "ElemDDLLikeOptWithHeadings";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLLikeOptWithHorizontalPartitions
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithHorizontalPartitions::~ElemDDLLikeOptWithHorizontalPartitions()
{
}

// casting
ElemDDLLikeOptWithHorizontalPartitions *
ElemDDLLikeOptWithHorizontalPartitions::castToElemDDLLikeOptWithHorizontalPartitions()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLikeOptWithHorizontalPartitions::getText() const
{
  return "ElemDDLLikeOptWithHorizontalPartitions";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLLikeOptWithDivision
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithDivision::~ElemDDLLikeOptWithDivision()
{
}

// casting
ElemDDLLikeOptWithDivision * ElemDDLLikeOptWithDivision::castToElemDDLLikeOptWithDivision()
{
  return this;
}

//
// methods for tracing
//

const NAString ElemDDLLikeOptWithDivision::getText() const
{
  return "ElemDDLLikeOptWithDivision";
}


//
// End of File
//
