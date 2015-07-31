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
#ifndef ELEMDDLPRIVACTIONS_H
#define ELEMDDLPRIVACTIONS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPrivActions.h
 * Description:  classes representing privilege actions specified in
 *               Grant DDL statements; also classes representing
 *               arrays of privilege actions.
 *
 *               
 * Created:      10/16/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

#include "Collections.h"
#include "ElemDDLColNameArray.h"
#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLPrivAct;
class ElemDDLPrivActArray;
class ElemDDLPrivActDelete;
class ElemDDLPrivActExecute;
class ElemDDLPrivActInsert;
class ElemDDLPrivActReferences;
class ElemDDLPrivActSelect;
class ElemDDLPrivActUpdate;
class ElemDDLPrivActWithColumns;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of base class ElemDDLPrivAct
// -----------------------------------------------------------------------
class ElemDDLPrivAct : public ElemDDLNode
{

public:

   enum PrivClass {DDLPriv = 2, DMLPriv = 3, OtherPriv = 4};

  // default constructor
  ElemDDLPrivAct(OperatorTypeEnum operatorType = ELM_ANY_PRIV_ACT_ELEM,
                 PrivClass privClass = DMLPriv)
  : ElemDDLNode(operatorType),
    privClass_(privClass)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivAct();

  // cast
  virtual ElemDDLPrivAct * castToElemDDLPrivAct();

  // methods for tracing
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;
  inline NABoolean isDDLPriv() const;
  inline NABoolean isDMLPriv() const;

private:

PrivClass privClass_;

}; // class ElemDDLPrivAct

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActArray
//   representing an array of pointers pointing to parse nodes
//   representing privilege actions.
//
//   Note that class ElemDDLPrivActArray is not derived from
//   class ElemDDLPrivAct.
// -----------------------------------------------------------------------
class ElemDDLPrivActArray : public LIST(ElemDDLPrivAct *)
{

public:

  // default constructor
  ElemDDLPrivActArray(CollHeap * heap = PARSERHEAP())
  : LIST(ElemDDLPrivAct *)(heap),
    hasDDLPriv_(FALSE)
  { }

  // copy ctor
  ElemDDLPrivActArray (const ElemDDLPrivActArray & orig, 
                       CollHeap * h=PARSERHEAP()) ; // not written

  // virtual destructor
  virtual ~ElemDDLPrivActArray();
  inline NABoolean hasDDLPriv() const;
  inline void setHasDDLPriv(NABoolean setting);

private:

NABoolean hasDDLPriv_;

}; // class ElemDDLPrivActArray

// -----------------------------------------------------------------------
// definition of base class ElemDDLPrivActWithColumns
//   representing a privelege action involving a list of column names.
//
//   classes ElemDDLPrivActInsert, ElemDDLPrivActReferences, and
//   ElemDDLPrivActUpdate are derived from this class.
// -----------------------------------------------------------------------
class ElemDDLPrivActWithColumns : public ElemDDLPrivAct
{

public:

  // default constructor
  ElemDDLPrivActWithColumns(OperatorTypeEnum operatorType
                            = ELM_ANY_PRIV_ACT_WITH_COLUMNS_ELEM,
                            ElemDDLNode * pColumnNameList = NULL,
                            CollHeap * heap = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLPrivActWithColumns();

  virtual ElemDDLPrivActWithColumns * castToElemDDLPrivActWithColumns();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline const ElemDDLColNameArray & getColumnNameArray() const;
  inline       ElemDDLColNameArray & getColumnNameArray();

  // mutator
  virtual void setChild(Lng32 index, ExprNode * pElemDDLNode);

  // methods for tracing
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  //
  // methods
  //

  inline ElemDDLNode * getColumnNameList() const;

  //
  // data members
  //

  ElemDDLColNameArray columnNameArray_;

  // pointers to child parse node

  enum { INDEX_COLUMN_NAME_LIST = 0,
         MAX_ELEM_DDL_PRIV_ACT_WITH_COLUMNS_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_PRIV_ACT_WITH_COLUMNS_ARITY];

}; // class ElemDDLPrivActWithColumns

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlter
// -----------------------------------------------------------------------
class ElemDDLPrivActAlter : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlter()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlter();

  // cast
  virtual ElemDDLPrivActAlter * castToElemDDLPrivActAlter();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlter

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterLibrary
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterLibrary : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterLibrary()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_LIBRARY_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterLibrary();

  // cast
  virtual ElemDDLPrivActAlterLibrary * castToElemDDLPrivActAlterLibrary();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterLibrary

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterMV
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterMV : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterMV()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_MV_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterMV();

  // cast
  virtual ElemDDLPrivActAlterMV * castToElemDDLPrivActAlterMV();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterMV

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterMVGroup
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterMVGroup : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterMVGroup()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_MVGROUP_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterMVGroup();

  // cast
  virtual ElemDDLPrivActAlterMVGroup * castToElemDDLPrivActAlterMVGroup();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterMVGroup

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterRoutine
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterRoutine : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterRoutine()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_ROUTINE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterRoutine();

  // cast
  virtual ElemDDLPrivActAlterRoutine * castToElemDDLPrivActAlterRoutine();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterRoutine

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterRoutineAction
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterRoutineAction : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterRoutineAction()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_ROUTINE_ACTION_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterRoutineAction();

  // cast
  virtual ElemDDLPrivActAlterRoutineAction * castToElemDDLPrivActAlterRoutineAction();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterRoutineAction

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterSynonym
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterSynonym : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterSynonym()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_SYNONYM_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterSynonym();

  // cast
  virtual ElemDDLPrivActAlterSynonym * castToElemDDLPrivActAlterSynonym();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterSynonym

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterTable
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterTable : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterTable()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_TABLE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterTable();

  // cast
  virtual ElemDDLPrivActAlterTable * castToElemDDLPrivActAlterTable();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterTable

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterTrigger
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterTrigger : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterTrigger()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_TRIGGER_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterTrigger();

  // cast
  virtual ElemDDLPrivActAlterTrigger * castToElemDDLPrivActAlterTrigger();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterTrigger

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAlterView
// -----------------------------------------------------------------------
class ElemDDLPrivActAlterView : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAlterView()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALTER_VIEW_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAlterView();

  // cast
  virtual ElemDDLPrivActAlterView * castToElemDDLPrivActAlterView();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAlterView

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAllOther
// -----------------------------------------------------------------------
class ElemDDLPrivActAllOther : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAllOther()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALL_OTHER_ELEM,ElemDDLPrivAct::OtherPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAllOther();

  // cast
  virtual ElemDDLPrivActAllOther * castToElemDDLPrivActAllOther();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAllOther

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreate
// -----------------------------------------------------------------------
class ElemDDLPrivActCreate : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreate()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreate();

  // cast
  virtual ElemDDLPrivActCreate * castToElemDDLPrivActCreate();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreate

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateLibrary
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateLibrary : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateLibrary()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_LIBRARY_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateLibrary();

  // cast
  virtual ElemDDLPrivActCreateLibrary * castToElemDDLPrivActCreateLibrary();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateLibrary

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateMV
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateMV : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateMV()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_MV_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateMV();

  // cast
  virtual ElemDDLPrivActCreateMV * castToElemDDLPrivActCreateMV();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateMV

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateMVGroup
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateMVGroup : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateMVGroup()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_MVGROUP_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateMVGroup();

  // cast
  virtual ElemDDLPrivActCreateMVGroup * castToElemDDLPrivActCreateMVGroup();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateMVGroup

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateProcedure
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateProcedure : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateProcedure()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_PROCEDURE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateProcedure();

  // cast
  virtual ElemDDLPrivActCreateProcedure * castToElemDDLPrivActCreateProcedure();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateProcedure

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateRoutine
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateRoutine : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateRoutine()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_ROUTINE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateRoutine();

  // cast
  virtual ElemDDLPrivActCreateRoutine * castToElemDDLPrivActCreateRoutine();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateRoutine

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateRoutineAction
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateRoutineAction : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateRoutineAction()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_ROUTINE_ACTION_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateRoutineAction();

  // cast
  virtual ElemDDLPrivActCreateRoutineAction * castToElemDDLPrivActCreateRoutineAction();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateRoutineAction

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateSynonym
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateSynonym : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateSynonym()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_SYNONYM_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateSynonym();

  // cast
  virtual ElemDDLPrivActCreateSynonym * castToElemDDLPrivActCreateSynonym();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateSynonym

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateTable
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateTable : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateTable()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_TABLE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateTable();

  // cast
  virtual ElemDDLPrivActCreateTable * castToElemDDLPrivActCreateTable();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateTable

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateTrigger
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateTrigger : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateTrigger()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_TRIGGER_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateTrigger();

  // cast
  virtual ElemDDLPrivActCreateTrigger * castToElemDDLPrivActCreateTrigger();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateTrigger

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActCreateView
// -----------------------------------------------------------------------
class ElemDDLPrivActCreateView : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActCreateView()
  : ElemDDLPrivAct(ELM_PRIV_ACT_CREATE_VIEW_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActCreateView();

  // cast
  virtual ElemDDLPrivActCreateView * castToElemDDLPrivActCreateView();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActCreateView

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDBA
// -----------------------------------------------------------------------
class ElemDDLPrivActDBA : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDBA()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DBA_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDBA();

  // cast
  virtual ElemDDLPrivActDBA * castToElemDDLPrivActDBA();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDBA

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDelete
// -----------------------------------------------------------------------
class ElemDDLPrivActDelete : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDelete()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DELETE_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDelete();

  // cast
  virtual ElemDDLPrivActDelete * castToElemDDLPrivActDelete();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDelete

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDrop
// -----------------------------------------------------------------------
class ElemDDLPrivActDrop : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDrop()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDrop();

  // cast
  virtual ElemDDLPrivActDrop * castToElemDDLPrivActDrop();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDrop

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropLibrary
// -----------------------------------------------------------------------
class ElemDDLPrivActDropLibrary : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropLibrary()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_LIBRARY_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropLibrary();

  // cast
  virtual ElemDDLPrivActDropLibrary * castToElemDDLPrivActDropLibrary();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropLibrary

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropMV
// -----------------------------------------------------------------------
class ElemDDLPrivActDropMV : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropMV()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_MV_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropMV();

  // cast
  virtual ElemDDLPrivActDropMV * castToElemDDLPrivActDropMV();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropMV

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropMVGroup
// -----------------------------------------------------------------------
class ElemDDLPrivActDropMVGroup : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropMVGroup()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_MVGROUP_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropMVGroup();

  // cast
  virtual ElemDDLPrivActDropMVGroup * castToElemDDLPrivActDropMVGroup();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropMVGroup

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropProcedure
// -----------------------------------------------------------------------
class ElemDDLPrivActDropProcedure : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropProcedure()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_PROCEDURE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropProcedure();

  // cast
  virtual ElemDDLPrivActDropProcedure * castToElemDDLPrivActDropProcedure();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropProcedure

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropRoutine
// -----------------------------------------------------------------------
class ElemDDLPrivActDropRoutine : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropRoutine()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_ROUTINE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropRoutine();

  // cast
  virtual ElemDDLPrivActDropRoutine * castToElemDDLPrivActDropRoutine();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropRoutine

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropRoutineAction
// -----------------------------------------------------------------------
class ElemDDLPrivActDropRoutineAction : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropRoutineAction()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_ROUTINE_ACTION_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropRoutineAction();

  // cast
  virtual ElemDDLPrivActDropRoutineAction * castToElemDDLPrivActDropRoutineAction();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropRoutineAction

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropSynonym
// -----------------------------------------------------------------------
class ElemDDLPrivActDropSynonym : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropSynonym()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_SYNONYM_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropSynonym();

  // cast
  virtual ElemDDLPrivActDropSynonym * castToElemDDLPrivActDropSynonym();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropSynonym

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropTable
// -----------------------------------------------------------------------
class ElemDDLPrivActDropTable : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropTable()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_TABLE_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropTable();

  // cast
  virtual ElemDDLPrivActDropTable * castToElemDDLPrivActDropTable();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropTable

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropTrigger
// -----------------------------------------------------------------------
class ElemDDLPrivActDropTrigger : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropTrigger()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_TRIGGER_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropTrigger();

  // cast
  virtual ElemDDLPrivActDropTrigger * castToElemDDLPrivActDropTrigger();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropTrigger

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActDropView
// -----------------------------------------------------------------------
class ElemDDLPrivActDropView : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActDropView()
  : ElemDDLPrivAct(ELM_PRIV_ACT_DROP_VIEW_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActDropView();

  // cast
  virtual ElemDDLPrivActDropView * castToElemDDLPrivActDropView();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActDropView

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActExecute
// -----------------------------------------------------------------------
class ElemDDLPrivActExecute : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActExecute()
  : ElemDDLPrivAct(ELM_PRIV_ACT_EXECUTE_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActExecute();

  // cast
  virtual ElemDDLPrivActExecute * castToElemDDLPrivActExecute();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActExecute

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActInsert
// -----------------------------------------------------------------------
class ElemDDLPrivActInsert : public ElemDDLPrivActWithColumns
{

public:

  // constructor
  ElemDDLPrivActInsert(ElemDDLNode * pColumnNameList,
                       CollHeap * heap = PARSERHEAP())
  : ElemDDLPrivActWithColumns(ELM_PRIV_ACT_INSERT_ELEM,
                            pColumnNameList,
                            heap)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActInsert();

  // cast
  virtual ElemDDLPrivActInsert * castToElemDDLPrivActInsert();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActInsert

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActMaintain
// -----------------------------------------------------------------------
class ElemDDLPrivActMaintain : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActMaintain()
  : ElemDDLPrivAct(ELM_PRIV_ACT_MAINTAIN_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActMaintain();

  // cast
  virtual ElemDDLPrivActMaintain * castToElemDDLPrivActMaintain();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActMaintain

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActReferences
// -----------------------------------------------------------------------
class ElemDDLPrivActReferences : public ElemDDLPrivActWithColumns
{

public:

  // constructor
  ElemDDLPrivActReferences(ElemDDLNode * pColumnNameList,
                                  CollHeap * heap = PARSERHEAP())
  : ElemDDLPrivActWithColumns(ELM_PRIV_ACT_REFERENCES_ELEM,
                            pColumnNameList,
                            heap)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActReferences();

  // cast
  virtual ElemDDLPrivActReferences * castToElemDDLPrivActReferences();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActReferences

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActRefresh
// -----------------------------------------------------------------------
class ElemDDLPrivActRefresh : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActRefresh()
  : ElemDDLPrivAct(ELM_PRIV_ACT_REFRESH_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActRefresh();

  // cast
  virtual ElemDDLPrivActRefresh * castToElemDDLPrivActRefresh();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActRefresh

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActReorg
// -----------------------------------------------------------------------
class ElemDDLPrivActReorg : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActReorg()
  : ElemDDLPrivAct(ELM_PRIV_ACT_REORG_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActReorg();

  // cast
  virtual ElemDDLPrivActReorg * castToElemDDLPrivActReorg();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActReorg

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActReplicate
// -----------------------------------------------------------------------
class ElemDDLPrivActReplicate : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActReplicate()
  : ElemDDLPrivAct(ELM_PRIV_ACT_REPLICATE_ELEM,ElemDDLPrivAct::OtherPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActReplicate();

  // cast
  virtual ElemDDLPrivActReplicate * castToElemDDLPrivActReplicate();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActReplicate

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAllDML
// -----------------------------------------------------------------------
class ElemDDLPrivActAllDML : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActAllDML()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALL_DML_ELEM,ElemDDLPrivAct::DMLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAllDML();

  // cast
  virtual ElemDDLPrivActAllDML * castToElemDDLPrivActAllDML();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAllDML

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActSelect
// -----------------------------------------------------------------------
class ElemDDLPrivActSelect : public ElemDDLPrivActWithColumns
{

public:

  // constructor
  ElemDDLPrivActSelect(ElemDDLNode * pColumnNameList,
                       CollHeap * heap = PARSERHEAP())
  : ElemDDLPrivActWithColumns(ELM_PRIV_ACT_SELECT_ELEM,
                            pColumnNameList,
                            heap)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActSelect();

  // cast
  virtual ElemDDLPrivActSelect * castToElemDDLPrivActSelect();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActSelect

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActTransform
// -----------------------------------------------------------------------
class ElemDDLPrivActTransform : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActTransform()
  : ElemDDLPrivAct(ELM_PRIV_ACT_TRANSFORM_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActTransform();

  // cast
  virtual ElemDDLPrivActTransform * castToElemDDLPrivActTransform();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActTransform

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActUpdate
// -----------------------------------------------------------------------
class ElemDDLPrivActUpdate : public ElemDDLPrivActWithColumns
{

public:

  // constructor
  ElemDDLPrivActUpdate(ElemDDLNode * pColumnNameList,
                              CollHeap * heap = PARSERHEAP())
  : ElemDDLPrivActWithColumns(ELM_PRIV_ACT_UPDATE_ELEM,
                            pColumnNameList,
                            heap)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActUpdate();

  // cast
  virtual ElemDDLPrivActUpdate * castToElemDDLPrivActUpdate();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActUpdate

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActUpdateStats
// -----------------------------------------------------------------------
class ElemDDLPrivActUpdateStats : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActUpdateStats()
  : ElemDDLPrivAct(ELM_PRIV_ACT_UPDATE_STATS_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActUpdateStats();

  // cast
  virtual ElemDDLPrivActUpdateStats * castToElemDDLPrivActUpdateStats();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActUpdateStats

// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActAllDDL
// -----------------------------------------------------------------------
class ElemDDLPrivActAllDDL : public ElemDDLPrivAct
{
public:

  // constructor
  ElemDDLPrivActAllDDL()
  : ElemDDLPrivAct(ELM_PRIV_ACT_ALL_DDL_ELEM,ElemDDLPrivAct::DDLPriv)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActAllDDL();

  // cast
  virtual ElemDDLPrivActAllDDL * castToElemDDLPrivActAllDDL();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActAllDDL


// -----------------------------------------------------------------------
// definition of class ElemDDLPrivActUsage
// -----------------------------------------------------------------------
class ElemDDLPrivActUsage : public ElemDDLPrivAct
{

public:

  // constructor
  ElemDDLPrivActUsage()
  : ElemDDLPrivAct(ELM_PRIV_ACT_USAGE_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLPrivActUsage();

  // cast
  virtual ElemDDLPrivActUsage * castToElemDDLPrivActUsage();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;

private:

}; // class ElemDDLPrivActUsage


// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLPrivAct
// -----------------------------------------------------------------------


inline NABoolean ElemDDLPrivAct::isDDLPriv() const 
{
   return privClass_ == DDLPriv;
   
}

inline NABoolean ElemDDLPrivAct::isDMLPriv() const 
{
   return privClass_ == DMLPriv;
   
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLPrivActArray
// -----------------------------------------------------------------------


inline NABoolean ElemDDLPrivActArray::hasDDLPriv() const 
{
   return hasDDLPriv_;  
}
  
  
inline void ElemDDLPrivActArray::setHasDDLPriv(NABoolean setting) 
{
   hasDDLPriv_ = setting;
}

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLPrivActWithColumns
// -----------------------------------------------------------------------

//
// accessors
//

inline ElemDDLColNameArray &
ElemDDLPrivActWithColumns::getColumnNameArray()
{
  return columnNameArray_;
}

inline const ElemDDLColNameArray &
ElemDDLPrivActWithColumns::getColumnNameArray() const
{
  return columnNameArray_;
}


inline ElemDDLNode *
ElemDDLPrivActWithColumns::getColumnNameList() const
{
  return children_[INDEX_COLUMN_NAME_LIST];
}

#endif // ELEMDDLPRIVACTIONS_H
