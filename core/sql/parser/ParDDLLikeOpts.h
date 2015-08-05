#ifndef PARDDLLIKEOPTS_H
#define PARDDLLIKEOPTS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParDDLLikeOpts.h
 * Description:  base class for derived classes to contain all the
 *               legal options in the LIKE clause of a DDL statement.
 *
 *               Note that the derived classes do not represent parse
 *               nodes.  Several kinds of parse nodes contain these
 *               derived classes.
 *
 *               
 * Created:      6/6/95
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


#include "NABasicObject.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParDDLLikeOpts;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of class ParDDLLikeOpts
// -----------------------------------------------------------------------
class ParDDLLikeOpts : public NABasicObject
{

public:

  // types of nodes containing all legal Like options
  // associating with a DDL statement
  enum likeOptsNodeTypeEnum { LIKE_OPTS_ANY_DDL_STMT,
                              LIKE_OPTS_CREATE_COLLATION,
                              LIKE_OPTS_CREATE_TABLE };

  // default constructor
  ParDDLLikeOpts(likeOptsNodeTypeEnum likeOptsNodeType
                 = LIKE_OPTS_ANY_DDL_STMT)
    : likeOptsNodeType_(likeOptsNodeType)
  {
  }

  // virtual destructor
  virtual ~ParDDLLikeOpts();

  // assignment
  ParDDLLikeOpts & operator=(const ParDDLLikeOpts & likeOptions);

  // accessors

  likeOptsNodeTypeEnum
  getLikeOptsNodeType() const
  {
    return likeOptsNodeType_;
  }


private:

  likeOptsNodeTypeEnum likeOptsNodeType_;

}; // class ParDDLLikeOpts

#endif /* PARDDLLIKEOPTS_H */
