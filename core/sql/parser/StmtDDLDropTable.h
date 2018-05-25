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
#ifndef STMTDDLDROPTABLE_H
#define STMTDDLDROPTABLE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropTable.h
 * Description:  class for parse node representing Drop Table statements
 *
 *
 * Created:      11/14/95
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
class StmtDDLDropTable;
class StmtDDLDropHbaseTable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Drop Table statement
// -----------------------------------------------------------------------
class StmtDDLDropTable : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropTable(const QualifiedName & tableQualName,
                   ComDropBehavior dropBehavior);
  StmtDDLDropTable(const QualifiedName & tableQualName,
                   ComDropBehavior dropBehavior,
                   NABoolean cleanupSpec,
                   NABoolean validateSpec,
                   NAString *pLogFile);



  // virtual destructor		
  virtual ~StmtDDLDropTable();

  // cast
  virtual StmtDDLDropTable * castToStmtDDLDropTable();

  void synthesize();

  // accessors
  inline ComDropBehavior getDropBehavior() const;
  inline const NAString getTableName() const;
  inline const QualifiedName & getOrigTableNameAsQualifiedName() const;
  inline       QualifiedName & getOrigTableNameAsQualifiedName();
  inline const QualifiedName & getTableNameAsQualifiedName() const;
  inline       QualifiedName   getTableNameAsQualifiedName();
  inline ExtendedQualName::SpecialTableType getTableType() const; // ++ MV
  inline NABoolean isSpecialTypeSpecified() const; //++ MV
  inline const NABoolean isCleanupSpecified() const;
  inline const NABoolean isValidateSpecified() const;
  inline const NABoolean isLogFileSpecified() const;
  inline const NAString & getLogFile() const;

  const NABoolean dropIfExists() const { return dropIfExists_; }

  const NABoolean isSmallTable() const { return smallTable_; }

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;

  // mutators
  inline void setTableType(ExtendedQualName::SpecialTableType tableType); // ++ MV

  void setDropIfExists(NABoolean v) { dropIfExists_ = v; }

  void setSmallTable(NABoolean v) { smallTable_= v; }

private:
  // the tablename specified by user in the drop stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origTableQualName_;

  QualifiedName tableQualName_;
  ComDropBehavior dropBehavior_;
  // ++ MV
  // The type of the table - for dropping tables in special namespaces	 
  ExtendedQualName::SpecialTableType tableType_;
  NABoolean isSpecialTypeSpecified_;
  // -- MV
  NABoolean isCleanupSpec_;
  NABoolean isValidateSpec_;
  NAString  *pLogFile_;

  // drop only if table exists. Otherwise just return.
  NABoolean dropIfExists_;

  NABoolean smallTable_;
}; // class StmtDDLDropTable

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropTable
// -----------------------------------------------------------------------

//
// accessors
//
inline QualifiedName &
StmtDDLDropTable::getOrigTableNameAsQualifiedName()
{
  return origTableQualName_;
}

inline const QualifiedName &
StmtDDLDropTable::getOrigTableNameAsQualifiedName() const
{
  return origTableQualName_;
}

inline QualifiedName
StmtDDLDropTable::getTableNameAsQualifiedName()
{
  return tableQualName_;
}

inline const QualifiedName &
StmtDDLDropTable::getTableNameAsQualifiedName() const
{
  return tableQualName_;
}

inline ComDropBehavior
StmtDDLDropTable::getDropBehavior() const
{
  return dropBehavior_;
}

inline const NAString
StmtDDLDropTable::getTableName() const
{
  return tableQualName_.getQualifiedNameAsAnsiString();
}

// ++ MV
inline NABoolean StmtDDLDropTable::isSpecialTypeSpecified() const
{
  return isSpecialTypeSpecified_;
}

inline void 
StmtDDLDropTable::setTableType(ExtendedQualName::SpecialTableType tableType)
{
  tableType_ = tableType;
  isSpecialTypeSpecified_ = TRUE;
}

inline 
ExtendedQualName::SpecialTableType StmtDDLDropTable::getTableType() const
{
  return tableType_;
}

// -- MV

inline const NABoolean
StmtDDLDropTable::isCleanupSpecified()const
{
  return isCleanupSpec_;
}

inline const NABoolean
StmtDDLDropTable::isValidateSpecified()const
{
  return isValidateSpec_;
}


inline const NABoolean
StmtDDLDropTable::isLogFileSpecified() const
{
  if (pLogFile_ == NULL)
    return FALSE;
  else
    return TRUE;
}

inline const NAString &
StmtDDLDropTable::getLogFile() const
{
  ComASSERT(pLogFile_ NEQ NULL);
  return *pLogFile_;
}

// -----------------------------------------------------------------------
// Drop Hbase Table statement
// -----------------------------------------------------------------------
class StmtDDLDropHbaseTable : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropHbaseTable(const QualifiedName & tableQualName);

  // virtual destructor		
  virtual ~StmtDDLDropHbaseTable();

  // cast
  virtual StmtDDLDropHbaseTable * castToStmtDDLDropHbaseTable();

  // accessors
  inline const NAString getTableName() const;
  inline const QualifiedName & getOrigTableNameAsQualifiedName() const;
  inline       QualifiedName & getOrigTableNameAsQualifiedName();
  inline const QualifiedName & getTableNameAsQualifiedName() const;
  inline       QualifiedName   getTableNameAsQualifiedName();

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;

private:
  // the tablename specified by user in the drop stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origTableQualName_;

  QualifiedName tableQualName_;
}; // class StmtDDLDropHbaseTable

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropHbaseTable
// -----------------------------------------------------------------------

//
// accessors
//
inline QualifiedName &
StmtDDLDropHbaseTable::getOrigTableNameAsQualifiedName()
{
  return origTableQualName_;
}

inline const QualifiedName &
StmtDDLDropHbaseTable::getOrigTableNameAsQualifiedName() const
{
  return origTableQualName_;
}

inline QualifiedName
StmtDDLDropHbaseTable::getTableNameAsQualifiedName()
{
  return tableQualName_;
}

inline const QualifiedName &
StmtDDLDropHbaseTable::getTableNameAsQualifiedName() const
{
  return tableQualName_;
}

inline const NAString
StmtDDLDropHbaseTable::getTableName() const
{
  return tableQualName_.getQualifiedNameAsAnsiString();
}

#endif // STMTDDLDROPTABLE_H
