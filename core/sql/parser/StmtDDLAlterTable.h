#ifndef STMTDDLALTERTABLE_H
#define STMTDDLALTERTABLE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterTable.h
 * Description:  base class for Alter Table statements
 *
 *
 * Created:      6/15/95
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
class StmtDDLAlterTable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Catalog statement
// -----------------------------------------------------------------------
class StmtDDLAlterTable : public StmtDDLNode
{

public:

  // constructors
  StmtDDLAlterTable();
  StmtDDLAlterTable(OperatorTypeEnum operatorType);
  StmtDDLAlterTable(OperatorTypeEnum operatorType,
                    ElemDDLNode * pAlterTableAction);
  StmtDDLAlterTable(OperatorTypeEnum operatorType,
                    const QualifiedName & tableQualName,
                    ElemDDLNode * pAlterTableAction);

  // virtual destructor
  virtual ~StmtDDLAlterTable();

  // cast
  virtual StmtDDLAlterTable * castToStmtDDLAlterTable();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline ElemDDLNode * getAlterTableAction() const;
  inline const NAString getTableName() const;
  inline const QualifiedName & getOrigTableNameAsQualifiedName() const;
  inline       QualifiedName & getOrigTableNameAsQualifiedName();
  inline const QualifiedName & getTableNameAsQualifiedName() const;
  inline       QualifiedName & getTableNameAsQualifiedName();
  
  inline const NABoolean isFakeNode() const;
  inline const NABoolean isDroppable() const;
  inline const NABoolean isInsertOnly() const;
  inline const NABoolean isOnline() const;
  inline const NABoolean forPurgedata() const;

        // returns TRUE if this node is fake node created by the
        // parser to represent a check constraint definition
        // appearring in a Create Table statement; returns FALSE
        // otherwise.  For more information, please read the
        // comments preceding the definition of data member
        // isParseSubTreeDestroyedByDestructor_
  
  //
  // mutators
  //

  virtual void setChild(Lng32 index, ExprNode * pChildNode);
  inline void setIsParseSubTreeDestroyedByDestructor(NABoolean setting);
  void setTableName(const QualifiedName & tableName);
  void setIsDroppable(NABoolean d);
  void setInsertOnly(NABoolean d);
  void setIsOnline(NABoolean d);
  void setForPurgedata(NABoolean d);

  // methods for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;

  // method for binding 
  ExprNode * bindNode(BindWA *bindWAPtr);

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  StmtDDLAlterTable(OperatorTypeEnum operatorType,
                    const NAString & tableName,
                    ElemDDLNode * pAlterTableAction);         // DO NOT USE
  StmtDDLAlterTable(const StmtDDLAlterTable &);               // DO NOT USE
  StmtDDLAlterTable & operator=(const StmtDDLAlterTable &);   // DO NOT USE
  
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------
  
  // the tablename specified by user in the alter stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origTableQualName_;

  QualifiedName tableQualName_;

  //
  // The following flag is a kludge.  Usually when the destructor
  // is invoked to destroy a parse node, the destructor will also
  // destroy the parse sub-tree linked to the parse node.  There
  // are exceptions to this rule.  For each column or table
  // constraint definition in a Create Table statement, we create
  // a corresponding parse node (derived and instantiated from
  // class ElemDDLConstraint) which is a node in a parse sub-tree
  // of the Create Table parse node.  To make the processing of
  // the Create Table parse node easier, we also create a kludge
  // parse node derived from class StmtDDLAddConstraint to
  // correspond to each column or table constraint definition
  // in the Create Table statement.  The pointer to the child
  // parse node in this kludge parse node is also set to point
  // to the parse node derived from class ElemDDLConstraint.
  //
  // When the Create Table parse node is destroyed, its parse
  // sub-trees, which include the parse nodes derived from class
  // ElemDDLConstraint, are also destroyed.  Since the kludge
  // parse nodes do not belong to the Create Table parse tree,
  // the destructor of the Create Table parse node will need to
  // destroy them separately.  Before these kludge parse nodes
  // can be destroyed, the flag isParseSubTreeDestroyedByDestructor_
  // must be reset.  We don't want the destructor to destroy the
  // sub-trees of these kludge parse nodes.  These sub-tree are
  // deleted when the destructor of the Create Table parse node
  // is invoked.
  //
  NABoolean isParseSubTreeDestroyedByDestructor_;

  // pointer to child parse node

  enum { INDEX_ALTER_TABLE_ACTION = 0,
         MAX_STMT_DDL_ALTER_TABLE_ARITY };

  ElemDDLNode * alterTableAction_;

  NABoolean isDroppable_;
  NABoolean insertOnly_;

  // used with DDL_ALTER_TABLE_TOGGLE_ONLINE alter action,
  // This indicates if the table is to be made available online
  // or offline.
  NABoolean isOnline_;

  // This one goes with the isOnline_ field. If set, it indicates
  // that the redeftime of the partition need not be modified.
  // Used for internal operation(parallel purgedata) only.
  NABoolean forPurgedata_;

}; // class StmtDDLAlterTable

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterTable
// -----------------------------------------------------------------------


//
// accessors
//

inline ElemDDLNode *
StmtDDLAlterTable::getAlterTableAction() const
{
  return alterTableAction_;
}

inline const NAString
StmtDDLAlterTable::getTableName() const
{
  return tableQualName_.getQualifiedNameAsAnsiString();
}

inline QualifiedName &
StmtDDLAlterTable::getOrigTableNameAsQualifiedName()
{
  return origTableQualName_;
}

inline const QualifiedName &
StmtDDLAlterTable::getOrigTableNameAsQualifiedName() const
{
  return origTableQualName_;
}

inline const QualifiedName &
StmtDDLAlterTable::getTableNameAsQualifiedName() const
{
  return tableQualName_;
}

inline QualifiedName &
StmtDDLAlterTable::getTableNameAsQualifiedName()
{
  return tableQualName_;
}

inline const NABoolean
StmtDDLAlterTable::isFakeNode() const
{
  return (NOT isParseSubTreeDestroyedByDestructor_);
}

inline const NABoolean
StmtDDLAlterTable::isDroppable() const
{
  return isDroppable_;
}

inline const NABoolean 
StmtDDLAlterTable::isInsertOnly() const
{
  return insertOnly_;
}

inline const NABoolean 
StmtDDLAlterTable::isOnline() const
{
  return isOnline_;
}

inline const NABoolean 
StmtDDLAlterTable::forPurgedata() const
{
  return forPurgedata_;
}

//
// mutator
//

inline void
StmtDDLAlterTable::setIsParseSubTreeDestroyedByDestructor(NABoolean setting)
{
  isParseSubTreeDestroyedByDestructor_ = setting;
}

inline void 
StmtDDLAlterTable::setIsDroppable(NABoolean d)
{
  isDroppable_ = d;
}

inline void
StmtDDLAlterTable::setInsertOnly(NABoolean d)
{
  insertOnly_ = d;
}

inline void 
StmtDDLAlterTable::setIsOnline(NABoolean d)
{
  isOnline_ = d;
}

inline void 
StmtDDLAlterTable::setForPurgedata(NABoolean d)
{
  forPurgedata_ = d;
}

#endif // STMTDDLALTERTABLE_H
