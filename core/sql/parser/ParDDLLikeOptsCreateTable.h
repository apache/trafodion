#ifndef PARDDLLIKEOPTSCREATETABLE_H
#define PARDDLLIKEOPTSCREATETABLE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParDDLLikeOptsCreateTable.h
 * Description:  class to contain all legal options associating with the
 *               LIKE clause in DDL Create Table statements -- The
 *               parser constructs a parse node for each option
 *               specified in the LIKE clause in DDL statements.  Collecting
 *               all Like options to a single object (node) helps the
 *               user to access the Like option information easier.  The
 *               user does not need to traverse the parse sub-tree to
 *               look for each Like option parse node associating with
 *               the DDL statement.  Default values will be assigned to
 *               Like options that are not specified in the DDL statement.
 *
 *               Class ParDDLLikeOptsCreateTable does not represent a
 *               parse node.  Classes StmtDDLCreateTable (representing
 *               Create Table parse nodes) and ElemDDLLikeCreateTable
 *               contain the class ParDDLLikeOptsCreateTable.
 *
 *               
 * Created:      5/25/95
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */


#include "ParDDLLikeOpts.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParDDLLikeOptsCreateTable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class ParDDLLikeOptsCreateTable
// -----------------------------------------------------------------------
class ParDDLLikeOptsCreateTable : public ParDDLLikeOpts
{

public:

  // constructor
  ParDDLLikeOptsCreateTable();

  // virtual destructor
  virtual ~ParDDLLikeOptsCreateTable();

  // assignment
  ParDDLLikeOptsCreateTable & operator=(
       const ParDDLLikeOptsCreateTable & likeOptions);

  // accessors

  const NABoolean
  getIsWithComments() const
  {
    return isLikeOptWithComments_;
  }

  const NABoolean
  getIsWithoutConstraints() const
  {
    return isLikeOptWithoutConstraints_;
  }

  const NABoolean
  getIsWithHeadings() const
  {
    return isLikeOptWithHeadings_;
  }

  const NABoolean
  getIsWithHelpText() const
  {
    return isLikeOptWithHelpText_;
  }

  const NABoolean
  getIsWithHorizontalPartitions() const
  {
    return isLikeOptWithHorizontalPartitions_;
  }

  const NABoolean
  getIsWithoutSalt() const
  {
    return isLikeOptWithoutSalt_;
  }

  const NAString *
  getSaltClause() const
  {
    return isLikeOptSaltClause_;
  }

  const NABoolean
  getIsWithoutDivision() const
  {
    return isLikeOptWithoutDivision_;
  }

  const UInt32
  getIsLikeOptColumnLengthLimit() const
  {
    return isLikeOptColumnLengthLimit_;
  }

  const NABoolean
  getIsWithoutRowFormat() const
  {
    return isLikeOptWithoutRowFormat_;
  }

  const NABoolean
  getIsWithoutLobColumns() const
  {
    return isLikeOptWithoutLobColumns_;
  }

  const NAString &
  getLikeOptHiveOptions() const
  {
    return likeOptHiveOptions_;
  }

  // mutators

  void setLikeOption(ElemDDLLikeOpt * pLikeOptParseNode);

  void
  setIsWithComments(const NABoolean setting)
  {
    isLikeOptWithComments_ = setting;
  }

  void
  setIsWithoutConstraints(const NABoolean setting)
  {
    isLikeOptWithoutConstraints_ = setting;
  }

  void
  setIsWithHeadings(const NABoolean setting)
  {
    isLikeOptWithHeadings_ = setting;
  }

  void
  setIsWithHelpText(const NABoolean setting)
  {
    isLikeOptWithHelpText_ = setting;
  }

  void
  setIsWithHorizontalPartitions(const NABoolean setting)
  {
    isLikeOptWithHorizontalPartitions_ = setting;
  }

  void
  setIsWithoutSalt(const NABoolean setting)
  {
    isLikeOptWithoutSalt_ = setting;
  }

  void
  setIsWithoutDivision(const NABoolean setting)
  {
    isLikeOptWithoutDivision_ = setting;
  }

  void
  setIsLikeOptColumnLengthLimit(const UInt32 setting)
  {
    isLikeOptColumnLengthLimit_ = setting;
  }

  void
  setIsWithoutRowFormat(const NABoolean setting)
  {
    isLikeOptWithoutRowFormat_ = setting;
  }

  void
  setIsWithoutLobColumns(const NABoolean setting)
  {
    isLikeOptWithoutLobColumns_ = setting;
  }

  void
  setLikeHiveOptions(const NAString &opts)
  {
    likeOptHiveOptions_ = opts;
  }

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  void initializeDataMembers();

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // The flags is...Spec_ shows whether the corresponding Like
  // options were specified in the DDL statement or not.  They
  // are used by the parser to look for duplicate options.

  NABoolean isLikeOptWithCommentsSpec_;
  NABoolean isLikeOptWithoutConstraintsSpec_;
  NABoolean isLikeOptWithHeadingsSpec_;
  NABoolean isLikeOptWithHelpTextSpec_;
  NABoolean isLikeOptWithHorizontalPartitionsSpec_;
  NABoolean isLikeOptWithoutSaltSpec_;
  NABoolean isLikeOptSaltClauseSpec_;
  NABoolean isLikeOptWithoutDivisionSpec_;
  NABoolean isLikeOptLimitColumnLengthSpec_;
  NABoolean isLikeOptWithoutRowFormatSpec_;
  NABoolean isLikeOptWithoutLobColumnsSpec_;

  // legal Like options in DDL Create Table statements

  NABoolean isLikeOptWithComments_;
  NABoolean isLikeOptWithoutConstraints_;
  NABoolean isLikeOptWithHeadings_;
  NABoolean isLikeOptWithHelpText_;
  NABoolean isLikeOptWithHorizontalPartitions_;
  NABoolean isLikeOptWithoutSalt_;
  NAString * isLikeOptSaltClause_;
  NABoolean isLikeOptWithoutDivision_;
  UInt32 isLikeOptColumnLengthLimit_;  // in bytes; max UInt32 if no limit specified
  NABoolean isLikeOptWithoutRowFormat_;
  NABoolean isLikeOptWithoutLobColumns_;
  NAString  likeOptHiveOptions_;

}; // class ParDDLLikeOptsCreateTable

#endif /* PARDDLLIKEOPTSCREATETABLE_H */
