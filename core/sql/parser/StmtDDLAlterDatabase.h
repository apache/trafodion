#ifndef STMTDDLALTERDB_H
#define STMTDDLALTERDB_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterDatabase.h
 * Description:  base class for Alter Database statements
 *
 * Alter statements supported:
 *
 *    ALTER DATABASE {enable | disable} AUTHORIZATION CHANGES
 *
 * Created:      10/31/2012
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


#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAlterDatabase;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Alter database statement
// -----------------------------------------------------------------------
class StmtDDLAlterDatabase : public StmtDDLNode
{

public:

  // enums
  enum DbCmdType { DBCMDTYPE_UNKNOWN = 0
                 , DBCMDTYPE_AUTHNAME
                 };

  // constructors  
  StmtDDLAlterDatabase( DbCmdType cmdType,
                        NABoolean enableStatus                    
                      )
  : StmtDDLNode(DDL_ALTER_DATABASE),
    cmdType_ (cmdType),
    enableStatus_ (enableStatus)
  {}

  // virtual destructor
  virtual ~StmtDDLAlterDatabase(){};

  // cast
  virtual StmtDDLAlterDatabase * castToStmtDDLAlterDatabase() { return this; };

  //
  // accessors
  //

  inline const DbCmdType getCmdType() const { return cmdType_; };
  inline const NABoolean isEnableAuth() const { return enableStatus_; };
  inline const NABoolean isDisableAuth() const { return !enableStatus_; };
      
  // methods for tracing
  virtual const NAString getText() const { return "StmtDDLAlterDatabase"; };
  virtual const NAString displayLabel1() const 
     { return "Enable status: " + enableStatus_; };

  // method for binding 
  ExprNode * bindNode(BindWA *bindWAPtr) { markAsBound(); return this; };

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  StmtDDLAlterDatabase(const StmtDDLAlterDatabase &);             // DO NOT USE
  StmtDDLAlterDatabase & operator=(const StmtDDLAlterDatabase &); // DO NOT USE
  
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------
  
  DbCmdType cmdType_;
  NABoolean enableStatus_;

}; // class StmtDDLAlterDatabase

#endif // STMTDDLALTERDB_H
