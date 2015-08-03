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
 * File:         ElemDDLLoadOptions.C
 * Description:  methods for class ElemDDLLoadOpt and any classes
 *               derived from class ElemDDLLoadOpt.
 *
 * Created:      9/28/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComDiags.h"
#include "ElemDDLLoadOptions.h"
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#include "SqlParserGlobals.h"
#include "NAString.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLLoadOpt
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLLoadOpt::~ElemDDLLoadOpt()
{
}

// casting
ElemDDLLoadOpt *
ElemDDLLoadOpt::castToElemDDLLoadOpt()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLoadOpt::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLLoadOpt";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLLoadOptDSlack
// -----------------------------------------------------------------------

// constructor
ElemDDLLoadOptDSlack::ElemDDLLoadOptDSlack(Lng32 percentage)
: ElemDDLLoadOpt(ELM_LOAD_OPT_D_SLACK_ELEM)
{
  if (isLegalPercentageValue(percentage))
  {
    percentage_ = (unsigned short)percentage;
  }
  else
  {
    percentage_ = DEFAULT_PERCENTAGE;
    // illegal percentage value in DSLACK phrase.
    *SqlParser_Diags << DgSqlCode(-3060);
  }
} // ElemDDLLoadOptDSlack::ElemDDLLoadOptDSlack()

// virtual destructor
ElemDDLLoadOptDSlack::~ElemDDLLoadOptDSlack()
{
}

// casting
ElemDDLLoadOptDSlack *
ElemDDLLoadOptDSlack::castToElemDDLLoadOptDSlack()
{
  return this;
}

//
// accessors
//

// is the specified percentage a legal value?
NABoolean
ElemDDLLoadOptDSlack::isLegalPercentageValue(Lng32 percentage)
{
  if (percentage >= 0 AND percentage <= 100)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

//
// methods for tracing
//

const NAString
ElemDDLLoadOptDSlack::displayLabel1() const
{
  return (NAString("Percentage: ") +
          LongToNAString((Lng32)getPercentage()));
}

const NAString
ElemDDLLoadOptDSlack::getText() const
{
  return "ElemDDLLoadOptDSlack";
}


// method for building text
// virtual 
NAString ElemDDLLoadOptDSlack::getSyntax() const
{
  NAString syntax = "DSLACK ";

  syntax  += UnsignedToNAString(percentage_);

  return syntax; 
} // getSyntax


// -----------------------------------------------------------------------
// methods for class ElemDDLLoadOptISlack
// -----------------------------------------------------------------------

// constructor
ElemDDLLoadOptISlack::ElemDDLLoadOptISlack(Lng32 percentage)
: ElemDDLLoadOpt(ELM_LOAD_OPT_I_SLACK_ELEM)
{
  if (isLegalPercentageValue(percentage))
  {
    percentage_ = (unsigned short)percentage;
  }
  else
  {
    percentage_ = DEFAULT_PERCENTAGE;
    // illegal percentage value in ISLACK phrase.
    *SqlParser_Diags << DgSqlCode(-3060) ;
  }
} // ElemDDLLoadOptISlack::ElemDDLLoadOptISlack()

// virtual destructor
ElemDDLLoadOptISlack::~ElemDDLLoadOptISlack()
{
}

// casting
ElemDDLLoadOptISlack *
ElemDDLLoadOptISlack::castToElemDDLLoadOptISlack()
{
  return this;
}

//
// accessors
//

// is the specified percentage a legal value?
NABoolean
ElemDDLLoadOptISlack::isLegalPercentageValue(Lng32 percentage)
{
  if (percentage >= 0 AND percentage <= 100)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

//
// methods for tracing
//

const NAString
ElemDDLLoadOptISlack::displayLabel1() const
{
  return (NAString("Percentage: ") +
          LongToNAString((Lng32)getPercentage()));
}

const NAString
ElemDDLLoadOptISlack::getText() const
{
  return "ElemDDLLoadOptISlack";
}

// method for building text
// virtual 
NAString ElemDDLLoadOptISlack::getSyntax() const
{
  NAString syntax = "ISLACK ";

  syntax  += UnsignedToNAString(percentage_);
  return syntax; 
} // getSyntax



//
// End of File
//
