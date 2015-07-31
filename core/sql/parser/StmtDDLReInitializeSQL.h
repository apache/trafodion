#ifndef STMTDDLREINITIALIZESQL_H
#define STMTDDLREINITIALIZESQL_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLReInitializeSQL.h
 * RCS:          $Id: StmtDDLInitializeSQL.h
 * Description:  class for parse node representing ReInitialize SQL
 *               statements
 *
 *
 * Created:      07/10/96
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

#include "ElemDDLLocation.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLReInitializeSQL;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLReInitializeSQL
// -----------------------------------------------------------------------
class StmtDDLReInitializeSQL : public StmtDDLNode
{

public:

  //
  // constructor
  //
  
  StmtDDLReInitializeSQL();

  //
  // virtual destructor
  //
  
  virtual ~StmtDDLReInitializeSQL();

  //
  // cast
  //
  
  virtual StmtDDLReInitializeSQL * castToStmtDDLReInitializeSQL();

  //
  // methods for tracing
  //

  virtual const NAString getText() const;


private:
  
  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  // copy constructor not supported
  


}; // class StmtDDLReInitializeSQL



#endif // STMTDDLREINITIALIZESQL_H


