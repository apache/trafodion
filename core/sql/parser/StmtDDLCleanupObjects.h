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
#ifndef STMTDDLCLEANUPOBJECTS_H
#define STMTDDLCLEANUPOBJECTS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCleanupObjects.h
 * Description:  
 *
 *
 * Created:     
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "NAString.h"
#include "StmtDDLNode.h"
#include "ComSmallDefs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCleanupObjects;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Cleanup Objects statement
// -----------------------------------------------------------------------
class StmtDDLCleanupObjects : public StmtDDLNode
{

public:
  enum ObjectType
  {
    TABLE_,
    OBJECT_UID_,
    INDEX_,
    VIEW_,
    SEQUENCE_,
    SCHEMA_PRIVATE_,
    SCHEMA_SHARED_,
    //    HIVE_TABLE_,
    //    HIVE_VIEW_,
    HBASE_TABLE_,
    UNKNOWN_,
    OBSOLETE_
  };
    
  // initialize constructor
  StmtDDLCleanupObjects(ObjectType type,
                        const NAString & param1,
                        const NAString * param2,
                        CollHeap    * heap);
  
  // virtual destructor
  virtual ~StmtDDLCleanupObjects();

  // cast
  virtual StmtDDLCleanupObjects * castToStmtDDLCleanupObjects();

  //
  // accessors
  //

  // methods relating to parse tree
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline const QualifiedName & getOrigTableNameAsQualifiedName() const;
  inline       QualifiedName & getOrigTableNameAsQualifiedName();
  inline const QualifiedName * getTableNameAsQualifiedName() const;
  inline       QualifiedName * getTableNameAsQualifiedName() ;

  // returns table name, in external format.
  const NAString getTableName() const;

  const Int64 getObjectUID() const { return objectUID_; }

  const ObjectType getType() const {return type_;}

  ExprNode * bindNode(BindWA * pBindWA);

  //
  // methods for tracing
  //

  //  virtual const NAString displayLabel1() const;
  //  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;

  NABoolean stopOnError() { return stopOnError_; }
  void setStopOnError(NABoolean v) { stopOnError_ = v; }

  NABoolean getStatus() { return getStatus_; }
  void setGetStatus(NABoolean v) { getStatus_ = v; }

  NABoolean checkOnly() { return checkOnly_; }
  void setCheckOnly(NABoolean v) { checkOnly_ = v; }

  NABoolean returnDetails() { return returnDetails_; }
  void setReturnDetails(NABoolean v) { returnDetails_ = v; }

private:

  ObjectType type_;
  NAString param1_;
  NAString param2_;

  //
  // please do not use the following methods
  //
  
  StmtDDLCleanupObjects();                                        // DO NOT USE
  StmtDDLCleanupObjects(const StmtDDLCleanupObjects &);              // DO NOT USE
  StmtDDLCleanupObjects & operator=(const StmtDDLCleanupObjects &);  // DO NOT USE

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // the tablename specified by user in the drop stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origTableQualName_;

  // The syntax of table name is
  // [ [ catalog-name . ] schema-name . ] table-name
  QualifiedName * tableQualName_;

  Int64 objectUID_;

  NABoolean stopOnError_;

  NABoolean getStatus_;
  NABoolean checkOnly_;
  NABoolean returnDetails_;
}; // class StmtDDLCleanupObjects

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCleanupObjects
// -----------------------------------------------------------------------
inline QualifiedName &
StmtDDLCleanupObjects::getOrigTableNameAsQualifiedName()
{
  return origTableQualName_;
}

inline const QualifiedName &
StmtDDLCleanupObjects::getOrigTableNameAsQualifiedName() const
{
  return origTableQualName_;
}

inline QualifiedName * 
StmtDDLCleanupObjects::getTableNameAsQualifiedName()
{
  return tableQualName_;
}

inline const QualifiedName *
StmtDDLCleanupObjects::getTableNameAsQualifiedName() const
{
  return tableQualName_;
}

#endif // STMTDDLCLEANUPOBJECTS_H
