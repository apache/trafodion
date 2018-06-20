/* -*-C++-*-
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
// methods for class ElemDDLLikeOptWithoutConstraints
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithoutConstraints::~ElemDDLLikeOptWithoutConstraints()
{
}

// casting
ElemDDLLikeOptWithoutConstraints *
ElemDDLLikeOptWithoutConstraints::castToElemDDLLikeOptWithoutConstraints()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLikeOptWithoutConstraints::getText() const
{
  return "ElemDDLLikeOptWithoutConstraints";
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
// methods for class ElemDDLLikeOptWithoutSalt
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithoutSalt::~ElemDDLLikeOptWithoutSalt()
{
}

// casting
ElemDDLLikeOptWithoutSalt * ElemDDLLikeOptWithoutSalt::castToElemDDLLikeOptWithoutSalt()
{
  return this;
}

//
// methods for tracing
//

const NAString ElemDDLLikeOptWithoutSalt::getText() const
{
  return "ElemDDLLikeOptWithoutSalt";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLLikeSaltClause
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeSaltClause::~ElemDDLLikeSaltClause()
{
  delete saltClause_;
}

// casting
ElemDDLLikeSaltClause * ElemDDLLikeSaltClause::castToElemDDLLikeSaltClause()
{
  return this;
}

//
// methods for tracing
//

const NAString ElemDDLLikeSaltClause::getText() const
{
  return "ElemDDLLikeSaltClause";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLLikeOptWithoutDivision
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithoutDivision::~ElemDDLLikeOptWithoutDivision()
{
}

// casting
ElemDDLLikeOptWithoutDivision * ElemDDLLikeOptWithoutDivision::castToElemDDLLikeOptWithoutDivision()
{
  return this;
}

//
// methods for tracing
//

const NAString ElemDDLLikeOptWithoutDivision::getText() const
{
  return "ElemDDLLikeOptWithoutDivision";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLLikeLimitColumnLength
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeLimitColumnLength::~ElemDDLLikeLimitColumnLength()
{
}

// casting
ElemDDLLikeLimitColumnLength * ElemDDLLikeLimitColumnLength::castToElemDDLLikeLimitColumnLength()
{
  return this;
}

//
// methods for tracing
//

const NAString ElemDDLLikeLimitColumnLength::getText() const
{
  return "ElemDDLLikeLimitColumnLength";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLLikeOptWithoutRowFormat
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithoutRowFormat::~ElemDDLLikeOptWithoutRowFormat()
{
}

// casting
ElemDDLLikeOptWithoutRowFormat * ElemDDLLikeOptWithoutRowFormat::castToElemDDLLikeOptWithoutRowFormat()
{
  return this;
}

//
// methods for tracing
//

const NAString ElemDDLLikeOptWithoutRowFormat::getText() const
{
  return "ElemDDLLikeOptWithoutRowFormat";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLLikeOptWithoutLobColumns
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLikeOptWithoutLobColumns::~ElemDDLLikeOptWithoutLobColumns()
{
}

// casting
ElemDDLLikeOptWithoutLobColumns * ElemDDLLikeOptWithoutLobColumns::castToElemDDLLikeOptWithoutLobColumns()
{
  return this;
}

//
// methods for tracing
//

const NAString ElemDDLLikeOptWithoutLobColumns::getText() const
{
  return "ElemDDLLikeOptWithoutLobColumns";
}

//
// End of File
//
