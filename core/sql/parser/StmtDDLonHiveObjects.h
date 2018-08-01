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
#ifndef STMTDDLHIVEOBJECTS_H
#define STMTDDLHIVEOBJECTS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLonHiveObjects.h
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
class StmtDDLonHiveObjects;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// DDL on Hive Objects from Traf interface
// -----------------------------------------------------------------------
class StmtDDLonHiveObjects : public StmtDDLNode
{

public:
  enum Operation
    {
      CREATE_,
      DROP_,
      ALTER_,
      TRUNCATE_,
      MSCK_,  // MetaStore Check
      PASSTHRU_DDL_,
      UNKNOWN_OPER_
    };
  
  enum ObjectType
    {
      TABLE_,
      SCHEMA_,
      VIEW_,
      UNKNOWN_TYPE_
    };
      
  // initialize constructor
  StmtDDLonHiveObjects(Operation oper,
                       ObjectType type,
                       NABoolean ifExistsOrNotExists,
                       const NAString &name,
                       NAString &hiveDDL,
                       NAString &hiveDefaultDB,
                       CollHeap * heap)
       : StmtDDLNode(DDL_ON_HIVE_OBJECTS),
         oper_(oper),
         type_(type),
         ifExistsOrNotExists_(ifExistsOrNotExists),
         name_(name),
         hiveDDL_(hiveDDL),
         hiveDefaultDB_(hiveDefaultDB),
         childNode_(NULL)
  {}
  
  // virtual destructor
  virtual ~StmtDDLonHiveObjects()
  {}

  // cast
  virtual StmtDDLonHiveObjects * castToStmtDDLonHiveObjects()
  {return this;}

  //
  // accessors
  //

  // methods relating to parse tree
  virtual Int32 getArity() const {return 1;}
  virtual ExprNode * getChild(Lng32 index) {return childNode_;}
  virtual void setChild(Lng32 index, ExprNode * pChildNode)
  {
    ComASSERT(index >= 0 AND index < getArity());
    childNode_ = pChildNode->castToElemDDLNode();
  }

  const Operation getOper() const {return oper_;}
  const char* getOperStr() const
  {
    switch (oper_)
      {
      case CREATE_       : return "create";
      case DROP_         : return "drop";
      case ALTER_        : return "alter";
      case TRUNCATE_     : return "truncate";
      case MSCK_         : return "msck";
      case PASSTHRU_DDL_ : return "passthru";
      case UNKNOWN_OPER_ : return "unknown";
      default            : return "unknown";
      } // switch
  }

  const ObjectType getType() const {return type_;}
  const char* getTypeStr() const
  {
    switch (type_)
      {
      case TABLE_         : return "table";
      case SCHEMA_        : return "schema";
      case VIEW_          : return "view";
      case UNKNOWN_TYPE_  : return "unknown";
      default             : return "unknown";
      } // switch
  }

  const NABoolean getIfExistsOrNotExists()  const { return ifExistsOrNotExists_; }
  const NAString &getName() const { return name_; }

  const NAString &getHiveDDL() const {return hiveDDL_;}
  NAString &getHiveDDL() {return hiveDDL_;}
  void setHiveDDL(NAString &hiveDDL) {hiveDDL_ = hiveDDL;}

  const NAString &getHiveDefaultDB() const {return hiveDefaultDB_;}
  NAString &getHiveDefaultDB() {return hiveDefaultDB_;}

  // ExprNode * bindNode(BindWA * pBindWA);

  //
  // methods for tracing
  //

  //  virtual const NAString displayLabel1() const;
  //  virtual const NAString displayLabel2() const;
  // virtual const NAString getText() const;

private:

  Operation oper_;
  ObjectType type_;

  // TRUE: if 'if exists' is specified for drop or truncate,
  //       or if 'if not exists' is specified for create.
  // FALSE: otherwise
  NABoolean ifExistsOrNotExists_;
  
  NAString name_;
  NAString hiveDDL_;

  // default hive database/schema set for the current session
  NAString hiveDefaultDB_;

  //
  // please do not use the following methods
  //
  
  StmtDDLonHiveObjects();                                        // DO NOT USE
  StmtDDLonHiveObjects(const StmtDDLonHiveObjects &);              // DO NOT USE
  StmtDDLonHiveObjects & operator=(const StmtDDLonHiveObjects &);  // DO NOT USE

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------
  ElemDDLNode *childNode_;
}; // class StmtDDLonHiveObjects

#endif // STMTDDLHIVEOBJECTS_H
