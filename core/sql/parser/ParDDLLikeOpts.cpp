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
  isLikeOptSaltClauseSpec_      = likeOptions.isLikeOptSaltClauseSpec_;
  isLikeOptWithoutDivisionSpec_ = likeOptions.isLikeOptWithoutDivisionSpec_;
  isLikeOptLimitColumnLengthSpec_ = likeOptions.isLikeOptLimitColumnLengthSpec_;
  isLikeOptWithoutRowFormatSpec_  = likeOptions.isLikeOptWithoutRowFormatSpec_;
  isLikeOptWithoutLobColumnsSpec_  = likeOptions.isLikeOptWithoutLobColumnsSpec_;

  isLikeOptWithComments_        = likeOptions.isLikeOptWithComments_;
  isLikeOptWithoutConstraints_  = likeOptions.isLikeOptWithoutConstraints_;
  isLikeOptWithHeadings_        = likeOptions.isLikeOptWithHeadings_;
  isLikeOptWithHelpText_        = likeOptions.isLikeOptWithHelpText_;
  isLikeOptWithHorizontalPartitions_  = likeOptions.isLikeOptWithHorizontalPartitions_;
  isLikeOptWithoutSalt_         = likeOptions.isLikeOptWithoutSalt_;
  isLikeOptWithoutDivision_     = likeOptions.isLikeOptWithoutDivision_;
  isLikeOptColumnLengthLimit_   = likeOptions.isLikeOptColumnLengthLimit_;
  isLikeOptWithoutRowFormat_    = likeOptions.isLikeOptWithoutRowFormat_;
  isLikeOptWithoutLobColumns_    = likeOptions.isLikeOptWithoutLobColumns_;
  likeOptHiveOptions_           = likeOptions.likeOptHiveOptions_;

  if (this != &likeOptions)  // make sure not assigning to self
    {
      if (likeOptions.isLikeOptSaltClause_)
        {
          delete isLikeOptSaltClause_;
          isLikeOptSaltClause_ = new (PARSERHEAP()) NAString(*likeOptions.isLikeOptSaltClause_);
        }
      else if (isLikeOptSaltClause_)
        {
          delete isLikeOptSaltClause_;
          isLikeOptSaltClause_ = NULL;
        }
      // else both are NULL; nothing to do
    }

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
  isLikeOptSaltClauseSpec_      = FALSE;
  isLikeOptWithoutDivisionSpec_ = FALSE;
  isLikeOptLimitColumnLengthSpec_ = FALSE;
  isLikeOptWithoutRowFormatSpec_  = FALSE;
  isLikeOptWithoutLobColumnsSpec_  = FALSE;

  isLikeOptWithComments_        = FALSE;
  isLikeOptWithoutConstraints_  = FALSE;
  isLikeOptWithHeadings_        = FALSE;
  isLikeOptWithHelpText_        = FALSE;
  isLikeOptWithHorizontalPartitions_        = FALSE;
  isLikeOptWithoutSalt_         = FALSE;
  isLikeOptSaltClause_          = NULL;
  isLikeOptWithoutDivision_     = FALSE;
  isLikeOptColumnLengthLimit_   = UINT_MAX;
  isLikeOptWithoutRowFormat_    = FALSE;
  isLikeOptWithoutLobColumns_    = FALSE;
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
    if (isLikeOptSaltClauseSpec_)
    {
      // ERROR[3154] The WITHOUT SALT clause is not allowed with the SALT clause.
      *SqlParser_Diags << DgSqlCode(-3154) << DgString0("WITHOUT SALT") << DgString1("SALT");
    }
    ComASSERT(pLikeOption->castToElemDDLLikeOptWithoutSalt() != NULL);
    isLikeOptWithoutSalt_ = TRUE;
    isLikeOptWithoutSaltSpec_ = TRUE;
    break;

  case ELM_LIKE_OPT_SALT_CLAUSE_ELEM:
    {  // braces needed since we declare some variables in this case
      if (isLikeOptSaltClauseSpec_)
      {
        // ERROR[3183] Duplicate SALT clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183) << DgString0("SALT");
      }
      if (isLikeOptWithoutSaltSpec_)
      {
        // ERROR[3154] The WITHOUT SALT clause is not allowed with the SALT clause.
        *SqlParser_Diags << DgSqlCode(-3154) << DgString0("WITHOUT SALT") << DgString1("SALT");
      }
      ComASSERT(pLikeOption->castToElemDDLLikeSaltClause() != NULL);
      isLikeOptSaltClauseSpec_ = TRUE;
      isLikeOptSaltClause_ = new (PARSERHEAP()) NAString();
      ElemDDLLikeSaltClause * saltClauseWrapper = pLikeOption->castToElemDDLLikeSaltClause();
      const ElemDDLSaltOptionsClause * saltOptions = saltClauseWrapper->getSaltClause();
      saltOptions->unparseIt(*isLikeOptSaltClause_ /* side-effected */);
      isLikeOptWithoutSalt_ = TRUE;  // suppresses any SALT clause from the source table
    }
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

  case ELM_LIKE_OPT_LIMIT_COLUMN_LENGTH:
    {
      if (isLikeOptLimitColumnLengthSpec_)
      {
        // ERROR[3152] Duplicate LIMIT COLUMN LENGTH phrases were specified
        //             in LIKE clause in CREATE TABLE statement.
        *SqlParser_Diags << DgSqlCode(-3152) << DgString0("LIMIT COLUMN LENGTH");
      }
      ComASSERT(pLikeOption->castToElemDDLLikeLimitColumnLength() != NULL);
      ElemDDLLikeLimitColumnLength * limitColumnLength = 
        pLikeOption->castToElemDDLLikeLimitColumnLength();
      isLikeOptColumnLengthLimit_ = limitColumnLength->getColumnLengthLimit();
      isLikeOptLimitColumnLengthSpec_ = TRUE;
    }
    break;

  case ELM_LIKE_OPT_WITHOUT_ROW_FORMAT_ELEM :
    if (isLikeOptWithoutRowFormatSpec_)
    {
      // ERROR[3152] Duplicate WITHOUT ROW FORMAT phrases were specified
      //             in LIKE clause in CREATE TABLE statement.
      *SqlParser_Diags << DgSqlCode(-3152) << DgString0("ROW FORMAT");
    }
    ComASSERT(pLikeOption->castToElemDDLLikeOptWithoutRowFormat() != NULL);
    isLikeOptWithoutRowFormat_ = TRUE;
    isLikeOptWithoutRowFormatSpec_ = TRUE;
    break;

  case ELM_LIKE_OPT_WITHOUT_LOB_COLUMNS :
    if (isLikeOptWithoutLobColumnsSpec_)
    {
      // ERROR[3152] Duplicate WITHOUT LOB COLUMNS phrases were specified
      //             in LIKE clause in CREATE TABLE statement.
      *SqlParser_Diags << DgSqlCode(-3152) << DgString0("LOB COLUMNS");
    }
    ComASSERT(pLikeOption->castToElemDDLLikeOptWithoutLobColumns() != NULL);
    isLikeOptWithoutLobColumns_ = TRUE;
    isLikeOptWithoutLobColumnsSpec_ = TRUE;
    break;

  default :
    NAAbort("ParDDLLikeOpts.C", __LINE__, "internal logic error");
    break;
  }
} // ParDDLLikeOptsCreateTable::setLikeOption()

//
// End of File
//
