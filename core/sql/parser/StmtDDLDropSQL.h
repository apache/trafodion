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
#ifndef STMTDDLDROPSQL_H
#define STMTDDLDROPSQL_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropSQL.h
 * Description:  class for parse node representing Drop SQL statements
 *
 *
 * Created:      03/29/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLDropSQL;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Drop SQL  statement
// -----------------------------------------------------------------------
class StmtDDLDropSQL : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropSQL(ComDropBehavior dropBehavior);

  // virtual destructor
  virtual ~StmtDDLDropSQL();

  // cast
  virtual StmtDDLDropSQL * castToStmtDDLDropSQL();

  // accessor
  inline ComDropBehavior getDropBehavior() const;

  // for tracing
  virtual const NAString getText() const;

private:

  ComDropBehavior dropBehavior_;

}; // class StmtDDLDropSQL


// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropSQL
// -----------------------------------------------------------------------
inline ComDropBehavior
StmtDDLDropSQL::getDropBehavior() const
{
  return dropBehavior_;
}

#endif // STMTDDLDROPSQL_H








