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
#ifndef STMTDDLADDCONSTRAINTRI_H
#define STMTDDLADDCONSTRAINTRI_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAddConstraintRI.h
 * Description:  class for Alter Table <table-name> Add Constraint
 *               statements containing Referential Integrity clause
 *
 *               The proper name for this class should be
 *               StmtDDLAlterTableAddConstraintRI because this class
 *               is derived from the base class StmtDDLAlterTable.  The
 *               name StmtDDLAlterTableAddConstraintRI is too long
 *               however.  So the short name StmtDDLAddConstraintRI
 *               is used even though it is less clear.
 *
 *               The methods in this class are defined either in this
 *               header file or the source file StmtDDLAlter.C.
 *
 *               
 * Created:      6/15/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "ElemDDLConstraintRI.h"
#include "StmtDDLAlterTable.h"
#include "StmtDDLAddConstraint.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAddConstraintRI;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class StmtDDLAddConstraintRI
// -----------------------------------------------------------------------

class StmtDDLAddConstraintRI : public StmtDDLAddConstraint
{

public:

  // constructors
  StmtDDLAddConstraintRI(ElemDDLNode * pElemDDLConstraintRI)
  : StmtDDLAddConstraint(
     DDL_ALTER_TABLE_ADD_CONSTRAINT_REFERENTIAL_INTEGRITY,
     pElemDDLConstraintRI)
  { }

  StmtDDLAddConstraintRI(const QualifiedName & tableQualName,
                                ElemDDLNode * pElemDDLConstraintRI)
  : StmtDDLAddConstraint(
     DDL_ALTER_TABLE_ADD_CONSTRAINT_REFERENTIAL_INTEGRITY,
     tableQualName,
     pElemDDLConstraintRI)
  { }

  // virtual destructor
  virtual ~StmtDDLAddConstraintRI();

  // cast
  virtual StmtDDLAddConstraintRI * castToStmtDDLAddConstraintRI();

  //
  // accessors
  //
  
  ComRCDeleteRule getDeleteRule() const;

        // returns COM_UNKNOWN_DELETE_RULE when Delete
        // rule does not appear.
  
  ComRCMatchOption getMatchType() const;
  ElemDDLColNameArray & getReferencedColumns();
  const ElemDDLColNameArray & getReferencedColumns() const;
  
  NAString getReferencedTableName() const;

        // returns the externally-formatted name of the
        // referenced table.  If this routine is invoked
        // after the parse node is bound, the returned
        // name is guaranteed to be fully-expanded.

  ElemDDLColNameArray & getReferencingColumns();
  const ElemDDLColNameArray & getReferencingColumns() const;

  ComRCUpdateRule getUpdateRule() const;

        // returns COM_UNKNOWN_UPDATE_RULE when Update
        // rule does not appear.

  NABoolean isDeleteRuleSpecified() const;
  NABoolean isUpdateRuleSpecified() const;

  // method for tracing
  virtual const NAString getText() const;
 
  // definition of method bindNode for the class StmtDDLAddConstraintRI
  virtual ExprNode * bindNode(BindWA * pBindWA);

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------
  
  StmtDDLAddConstraintRI();                             // DO NOT USE
  StmtDDLAddConstraintRI(const NAString & tableName,
                         ElemDDLNode *);                // DO NOT USE
  StmtDDLAddConstraintRI
        (const StmtDDLAddConstraintRI &);               // DO NOT USE
  StmtDDLAddConstraintRI & operator=
        (const StmtDDLAddConstraintRI &);               // DO NOT USE
  
}; // class StmtDDLAddConstraintRI


#endif // STMTDDLADDCONSTRAINTRI_H
