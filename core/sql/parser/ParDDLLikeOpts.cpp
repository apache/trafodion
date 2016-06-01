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
 * File:         ParDDLLikeOpts.C
 * Description:  methods for class ParDDLLikeOpts and classes derived
 *               from class ParDDLLikeOpts
 *
 * Created:      6/6/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "BaseTypes.h"
#include "ComASSERT.h"
#include "ComOperators.h"
#include "ElemDDLLikeOptions.h"
#include "OperTypeEnum.h"
#include "ParDDLLikeOpts.h"
#include "ParDDLLikeOptsCreateTable.h"

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "SqlParserGlobals.h"

// -----------------------------------------------------------------------
// methods for class ParDDLLikeOpts
// -----------------------------------------------------------------------

//
// virtual destructor
//
ParDDLLikeOpts::~ParDDLLikeOpts()
{
}

//
// assignment operator
//
ParDDLLikeOpts &
ParDDLLikeOpts::operator=(
     const ParDDLLikeOpts & likeOptions)
{
  if (this EQU &likeOptions)
    return *this;

  likeOptsNodeType_ = likeOptions.likeOptsNodeType_;
  return *this;
}

// -----------------------------------------------------------------------
// methods for class ParDDLLikeOptsCreateTable
// -----------------------------------------------------------------------

//
// constructor
//
ParDDLLikeOptsCreateTable::ParDDLLikeOptsCreateTable()
: ParDDLLikeOpts(ParDDLLikeOpts::LIKE_OPTS_CREATE_TABLE)
{
  initializeDataMembers();
}

//
// virtual destructor
//
ParDDLLikeOptsCreateTable::~ParDDLLikeOptsCreateTable()
{
}

//
// assignment operator
//
ParDDLLikeOptsCreateTable &
ParDDLLikeOptsCreateTable::operator=(
     const ParDDLLikeOptsCreateTable & likeOptions)
{
  if (this EQU &likeOptions)
    return *this;

  ParDDLLikeOpts::operator=(likeOptions);

  isLikeOptWithCommentsSpec_    = likeOptions.isLikeOptWithCommentsSpec_;
  isLikeOptWithoutConstraintsSpec_ = likeOptions.isLikeOptWithoutConstraintsSpec_;
  isLikeOptWithHeadingsSpec_    = likeOptions.isLikeOptWithHeadingsSpec_;
  isLikeOptWithHelpTextSpec_    = likeOptions.isLikeOptWithHelpTextSpec_;
  isLikeOptWithHorizontalPartitionsSpec_ = likeOptions.isLikeOptWithHorizontalPartitionsSpec_;
  isLikeOptWithoutSaltSpec_     = likeOptions.isLikeOptWithoutSaltSpec_;
  isLikeOptWithoutDivisionSpec_ = likeOptions.isLikeOptWithoutDivisionSpec_;

  isLikeOptWithComments_        = likeOptions.isLikeOptWithComments_;
  isLikeOptWithoutConstraints_  = likeOptions.isLikeOptWithoutConstraints_;
  isLikeOptWithHeadings_        = likeOptions.isLikeOptWithHeadings_;
  isLikeOptWithHelpText_        = likeOptions.isLikeOptWithHelpText_;
  isLikeOptWithHorizontalPartitions_  = likeOptions.isLikeOptWithHorizontalPartitions_;
  isLikeOptWithoutSalt_         = likeOptions.isLikeOptWithoutSalt_;
  isLikeOptWithoutDivision_     = likeOptions.isLikeOptWithoutDivision_;

  return *this;
}

//
// mutators
//

void
ParDDLLikeOptsCreateTable::initializeDataMembers()
{
  isLikeOptWithCommentsSpec_    = FALSE;
  isLikeOptWithoutConstraintsSpec_ = FALSE;
  isLikeOptWithHeadingsSpec_    = FALSE;
  isLikeOptWithHelpTextSpec_    = FALSE;
  isLikeOptWithHorizontalPartitionsSpec_  = FALSE;
  isLikeOptWithoutSaltSpec_     = FALSE;
  isLikeOptWithoutDivisionSpec_ = FALSE;

  isLikeOptWithComments_        = FALSE;
  isLikeOptWithoutConstraints_  = FALSE;
  isLikeOptWithHeadings_        = FALSE;
  isLikeOptWithHelpText_        = FALSE;
  isLikeOptWithHorizontalPartitions_        = FALSE;
  isLikeOptWithoutSalt_         = FALSE;
  isLikeOptWithoutDivision_     = FALSE;
}

void
ParDDLLikeOptsCreateTable::setLikeOption(ElemDDLLikeOpt * pLikeOption)
{
  ComASSERT(pLikeOption != NULL);
  
  switch (pLikeOption->getOperatorType())
  {
  case ELM_LIKE_OPT_WITHOUT_CONSTRAINTS_ELEM :
    if (isLikeOptWithoutConstraintsSpec_)
    {
      *SqlParser_Diags << DgSqlCode(-3149);

      //  "*** Error *** Duplicate WITHOUT CONSTRAINTS phrases "
      //  << "in LIKE clause" << endl;
    }
    ComASSERT(pLikeOption->castToElemDDLLikeOptWithoutConstraints() != NULL);
    isLikeOptWithoutConstraints_ = TRUE;
    isLikeOptWithoutConstraintsSpec_ = TRUE;
    break;

  case ELM_LIKE_OPT_WITH_HEADINGS_ELEM :
    if (isLikeOptWithHeadingsSpec_)
    {
      *SqlParser_Diags << DgSqlCode(-3150);
      //      cout << "*** Error *** Duplicate WITH HEADING phrases "
      //  << "in LIKE clause" << endl;
    }
    ComASSERT(pLikeOption->castToElemDDLLikeOptWithHeadings() != NULL);
    isLikeOptWithHeadings_ = TRUE;
    isLikeOptWithHeadingsSpec_ = TRUE;
    break;

  case ELM_LIKE_OPT_WITH_HORIZONTAL_PARTITIONS_ELEM :
    if (isLikeOptWithHorizontalPartitionsSpec_)
    {
      *SqlParser_Diags << DgSqlCode(-3151);
      //cout << "*** Error *** Duplicate WITH HORIZONTAL PARTITIONS phrases "
      //  << "in LIKE clause" << endl;
    }
    ComASSERT(pLikeOption->castToElemDDLLikeOptWithHorizontalPartitions() != NULL);
    isLikeOptWithHorizontalPartitions_ = TRUE;
    isLikeOptWithHorizontalPartitionsSpec_ = TRUE;
    break;

  case ELM_LIKE_OPT_WITHOUT_SALT_ELEM :
    if (isLikeOptWithoutSaltSpec_)
    {
      // ERROR[3152] Duplicate WITHOUT SALT phrases were specified
      //             in LIKE clause in CREATE TABLE statement.
      *SqlParser_Diags << DgSqlCode(-3152) << DgString0("SALT");
    }
    ComASSERT(pLikeOption->castToElemDDLLikeOptWithoutSalt() != NULL);
    isLikeOptWithoutSalt_ = TRUE;
    isLikeOptWithoutSaltSpec_ = TRUE;
    break;

  case ELM_LIKE_OPT_WITHOUT_DIVISION_ELEM :
    if (isLikeOptWithoutDivisionSpec_)
    {
      // ERROR[3152] Duplicate WITHOUT DIVISION phrases were specified
      //             in LIKE clause in CREATE TABLE statement.
      *SqlParser_Diags << DgSqlCode(-3152) << DgString0("DIVISION");
    }
    ComASSERT(pLikeOption->castToElemDDLLikeOptWithoutDivision() != NULL);
    isLikeOptWithoutDivision_ = TRUE;
    isLikeOptWithoutDivisionSpec_ = TRUE;
    break;

  default :
    NAAbort("ParDDLLikeOpts.C", __LINE__, "internal logic error");
    break;
  }
} // ParDDLLikeOptsCreateTable::setLikeOption()

//
// End of File
//
