/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ElemDDLPrivActions.C
 * Description:  methods for class ElemDDLPrivAct and any classes
 *               derived from class ElemDDLPrivAct; also methods
 *               for class ElemDDLPrivActArray
 *
 * Created:      10/16/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComASSERT.h"
#include "ComOperators.h"
#include "ElemDDLPrivActions.h"
#include "NAString.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivAct
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivAct::~ElemDDLPrivAct()
{
}

// casting
ElemDDLPrivAct *
ElemDDLPrivAct::castToElemDDLPrivAct()
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLPrivAct::getDetailInfo() const
{
  NATraceList detailTextList;

  //
  // Note that the invoked displayLabel1() is a method of
  // class ElemDDLPrivActDelete or ElemDDLPrivActSelect
  //
  detailTextList.append(displayLabel1());

  return detailTextList;
}

const NAString
ElemDDLPrivAct::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLPrivAct";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActArray
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActArray::~ElemDDLPrivActArray()
{
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlter
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlter::~ElemDDLPrivActAlter()
{
}

// casting
ElemDDLPrivActAlter *
ElemDDLPrivActAlter::castToElemDDLPrivActAlter()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlter::displayLabel1() const
{
  return NAString("Alter privilege action");
}

const NAString
ElemDDLPrivActAlter::getText() const
{
  return "ElemDDLPrivActAlter";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterLibrary
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterLibrary::~ElemDDLPrivActAlterLibrary()
{
}

// casting
ElemDDLPrivActAlterLibrary *
ElemDDLPrivActAlterLibrary::castToElemDDLPrivActAlterLibrary()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterLibrary::displayLabel1() const
{
  return NAString("Alter Library privilege action");
}

const NAString
ElemDDLPrivActAlterLibrary::getText() const
{
  return "ElemDDLPrivActAlterLibrary";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterMV
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterMV::~ElemDDLPrivActAlterMV()
{
}

// casting
ElemDDLPrivActAlterMV *
ElemDDLPrivActAlterMV::castToElemDDLPrivActAlterMV()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterMV::displayLabel1() const
{
  return NAString("Alter MV privilege action");
}

const NAString
ElemDDLPrivActAlterMV::getText() const
{
  return "ElemDDLPrivActAlterMV";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterMVGroup
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterMVGroup::~ElemDDLPrivActAlterMVGroup()
{
}

// casting
ElemDDLPrivActAlterMVGroup *
ElemDDLPrivActAlterMVGroup::castToElemDDLPrivActAlterMVGroup()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterMVGroup::displayLabel1() const
{
  return NAString("Alter MVGroup privilege action");
}

const NAString
ElemDDLPrivActAlterMVGroup::getText() const
{
  return "ElemDDLPrivActAlterMVGroup";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterRoutine
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterRoutine::~ElemDDLPrivActAlterRoutine()
{
}

// casting
ElemDDLPrivActAlterRoutine *
ElemDDLPrivActAlterRoutine::castToElemDDLPrivActAlterRoutine()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterRoutine::displayLabel1() const
{
  return NAString("Alter Routine privilege action");
}

const NAString
ElemDDLPrivActAlterRoutine::getText() const
{
  return "ElemDDLPrivActAlterRoutine";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterRoutineAction
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterRoutineAction::~ElemDDLPrivActAlterRoutineAction()
{
}

// casting
ElemDDLPrivActAlterRoutineAction *
ElemDDLPrivActAlterRoutineAction::castToElemDDLPrivActAlterRoutineAction()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterRoutineAction::displayLabel1() const
{
  return NAString("Alter RoutineAction privilege action");
}

const NAString
ElemDDLPrivActAlterRoutineAction::getText() const
{
  return "ElemDDLPrivActAlterRoutineAction";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterSynonym
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterSynonym::~ElemDDLPrivActAlterSynonym()
{
}

// casting
ElemDDLPrivActAlterSynonym *
ElemDDLPrivActAlterSynonym::castToElemDDLPrivActAlterSynonym()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterSynonym::displayLabel1() const
{
  return NAString("Alter Synonym privilege action");
}

const NAString
ElemDDLPrivActAlterSynonym::getText() const
{
  return "ElemDDLPrivActAlterSynonym";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterTable
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterTable::~ElemDDLPrivActAlterTable()
{
}

// casting
ElemDDLPrivActAlterTable *
ElemDDLPrivActAlterTable::castToElemDDLPrivActAlterTable()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterTable::displayLabel1() const
{
  return NAString("Alter table privilege action");
}

const NAString
ElemDDLPrivActAlterTable::getText() const
{
  return "ElemDDLPrivActAlterTable";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterTrigger
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterTrigger::~ElemDDLPrivActAlterTrigger()
{
}

// casting
ElemDDLPrivActAlterTrigger *
ElemDDLPrivActAlterTrigger::castToElemDDLPrivActAlterTrigger()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterTrigger::displayLabel1() const
{
  return NAString("Alter Trigger privilege action");
}

const NAString
ElemDDLPrivActAlterTrigger::getText() const
{
  return "ElemDDLPrivActAlterTrigger";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAlterView
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAlterView::~ElemDDLPrivActAlterView()
{
}

// casting
ElemDDLPrivActAlterView *
ElemDDLPrivActAlterView::castToElemDDLPrivActAlterView()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAlterView::displayLabel1() const
{
  return NAString("Alter View privilege action");
}

const NAString
ElemDDLPrivActAlterView::getText() const
{
  return "ElemDDLPrivActAlterView";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAllOther
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAllOther::~ElemDDLPrivActAllOther()
{
}

// casting
ElemDDLPrivActAllOther *
ElemDDLPrivActAllOther::castToElemDDLPrivActAllOther()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAllOther::displayLabel1() const
{
  return NAString("Backup privilege action");
}

const NAString
ElemDDLPrivActAllOther::getText() const
{
  return "ElemDDLPrivActAllOther";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreate
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreate::~ElemDDLPrivActCreate()
{
}

// casting
ElemDDLPrivActCreate *
ElemDDLPrivActCreate::castToElemDDLPrivActCreate()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreate::displayLabel1() const
{
  return NAString("Create privilege action");
}

const NAString
ElemDDLPrivActCreate::getText() const
{
  return "ElemDDLPrivActCreate";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateLibrary
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateLibrary::~ElemDDLPrivActCreateLibrary()
{
}

// casting
ElemDDLPrivActCreateLibrary *
ElemDDLPrivActCreateLibrary::castToElemDDLPrivActCreateLibrary()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateLibrary::displayLabel1() const
{
  return NAString("Create Library privilege action");
}

const NAString
ElemDDLPrivActCreateLibrary::getText() const
{
  return "ElemDDLPrivActCreateLibrary";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateMV
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateMV::~ElemDDLPrivActCreateMV()
{
}

// casting
ElemDDLPrivActCreateMV *
ElemDDLPrivActCreateMV::castToElemDDLPrivActCreateMV()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateMV::displayLabel1() const
{
  return NAString("Create MV privilege action");
}

const NAString
ElemDDLPrivActCreateMV::getText() const
{
  return "ElemDDLPrivActCreateMV";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateMVGroup
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateMVGroup::~ElemDDLPrivActCreateMVGroup()
{
}

// casting
ElemDDLPrivActCreateMVGroup *
ElemDDLPrivActCreateMVGroup::castToElemDDLPrivActCreateMVGroup()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateMVGroup::displayLabel1() const
{
  return NAString("Create MVGroup privilege action");
}

const NAString
ElemDDLPrivActCreateMVGroup::getText() const
{
  return "ElemDDLPrivActCreateMVGroup";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateProcedure
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateProcedure::~ElemDDLPrivActCreateProcedure()
{
}

// casting
ElemDDLPrivActCreateProcedure *
ElemDDLPrivActCreateProcedure::castToElemDDLPrivActCreateProcedure()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateProcedure::displayLabel1() const
{
  return NAString("Create Procedure privilege action");
}

const NAString
ElemDDLPrivActCreateProcedure::getText() const
{
  return "ElemDDLPrivActCreateProcedure";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateRoutine
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateRoutine::~ElemDDLPrivActCreateRoutine()
{
}

// casting
ElemDDLPrivActCreateRoutine *
ElemDDLPrivActCreateRoutine::castToElemDDLPrivActCreateRoutine()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateRoutine::displayLabel1() const
{
  return NAString("Create Routine privilege action");
}

const NAString
ElemDDLPrivActCreateRoutine::getText() const
{
  return "ElemDDLPrivActCreateRoutine";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateRoutineAction
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateRoutineAction::~ElemDDLPrivActCreateRoutineAction()
{
}

// casting
ElemDDLPrivActCreateRoutineAction *
ElemDDLPrivActCreateRoutineAction::castToElemDDLPrivActCreateRoutineAction()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateRoutineAction::displayLabel1() const
{
  return NAString("Create RoutineAction privilege action");
}

const NAString
ElemDDLPrivActCreateRoutineAction::getText() const
{
  return "ElemDDLPrivActCreateRoutineAction";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateSynonym
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateSynonym::~ElemDDLPrivActCreateSynonym()
{
}

// casting
ElemDDLPrivActCreateSynonym *
ElemDDLPrivActCreateSynonym::castToElemDDLPrivActCreateSynonym()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateSynonym::displayLabel1() const
{
  return NAString("Create Synonym privilege action");
}

const NAString
ElemDDLPrivActCreateSynonym::getText() const
{
  return "ElemDDLPrivActCreateSynonym";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateTable
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateTable::~ElemDDLPrivActCreateTable()
{
}

// casting
ElemDDLPrivActCreateTable *
ElemDDLPrivActCreateTable::castToElemDDLPrivActCreateTable()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateTable::displayLabel1() const
{
  return NAString("Create table privilege action");
}

const NAString
ElemDDLPrivActCreateTable::getText() const
{
  return "ElemDDLPrivActCreateTable";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateTrigger
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateTrigger::~ElemDDLPrivActCreateTrigger()
{
}

// casting
ElemDDLPrivActCreateTrigger *
ElemDDLPrivActCreateTrigger::castToElemDDLPrivActCreateTrigger()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateTrigger::displayLabel1() const
{
  return NAString("Create Trigger privilege action");
}

const NAString
ElemDDLPrivActCreateTrigger::getText() const
{
  return "ElemDDLPrivActCreateTrigger";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActCreateView
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActCreateView::~ElemDDLPrivActCreateView()
{
}

// casting
ElemDDLPrivActCreateView *
ElemDDLPrivActCreateView::castToElemDDLPrivActCreateView()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActCreateView::displayLabel1() const
{
  return NAString("Create view privilege action");
}

const NAString
ElemDDLPrivActCreateView::getText() const
{
  return "ElemDDLPrivActCreateView";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDelete
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDelete::~ElemDDLPrivActDelete()
{
}

// casting
ElemDDLPrivActDelete *
ElemDDLPrivActDelete::castToElemDDLPrivActDelete()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDelete::displayLabel1() const
{
  return NAString("Delete privilege action");
}

const NAString
ElemDDLPrivActDelete::getText() const
{
  return "ElemDDLPrivActDelete";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDBA
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDBA::~ElemDDLPrivActDBA()
{
}

// casting
ElemDDLPrivActDBA *
ElemDDLPrivActDBA::castToElemDDLPrivActDBA()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDBA::displayLabel1() const
{
  return NAString("DBA privilege action");
}

const NAString
ElemDDLPrivActDBA::getText() const
{
  return "ElemDDLPrivActDBA";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDrop
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDrop::~ElemDDLPrivActDrop()
{
}

// casting
ElemDDLPrivActDrop *
ElemDDLPrivActDrop::castToElemDDLPrivActDrop()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDrop::displayLabel1() const
{
  return NAString("Drop privilege action");
}

const NAString
ElemDDLPrivActDrop::getText() const
{
  return "ElemDDLPrivActDrop";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropLibrary
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropLibrary::~ElemDDLPrivActDropLibrary()
{
}

// casting
ElemDDLPrivActDropLibrary *
ElemDDLPrivActDropLibrary::castToElemDDLPrivActDropLibrary()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropLibrary::displayLabel1() const
{
  return NAString("Drop Library privilege action");
}

const NAString
ElemDDLPrivActDropLibrary::getText() const
{
  return "ElemDDLPrivActDropLibrary";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropMV
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropMV::~ElemDDLPrivActDropMV()
{
}

// casting
ElemDDLPrivActDropMV *
ElemDDLPrivActDropMV::castToElemDDLPrivActDropMV()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropMV::displayLabel1() const
{
  return NAString("Drop MV privilege action");
}

const NAString
ElemDDLPrivActDropMV::getText() const
{
  return "ElemDDLPrivActDropMV";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropMVGroup
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropMVGroup::~ElemDDLPrivActDropMVGroup()
{
}

// casting
ElemDDLPrivActDropMVGroup *
ElemDDLPrivActDropMVGroup::castToElemDDLPrivActDropMVGroup()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropMVGroup::displayLabel1() const
{
  return NAString("Drop MVGroup privilege action");
}

const NAString
ElemDDLPrivActDropMVGroup::getText() const
{
  return "ElemDDLPrivActDropMVGroup";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropProcedure
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropProcedure::~ElemDDLPrivActDropProcedure()
{
}

// casting
ElemDDLPrivActDropProcedure *
ElemDDLPrivActDropProcedure::castToElemDDLPrivActDropProcedure()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropProcedure::displayLabel1() const
{
  return NAString("Drop Procedure privilege action");
}

const NAString
ElemDDLPrivActDropProcedure::getText() const
{
  return "ElemDDLPrivActDropProcedure";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropRoutine
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropRoutine::~ElemDDLPrivActDropRoutine()
{
}

// casting
ElemDDLPrivActDropRoutine *
ElemDDLPrivActDropRoutine::castToElemDDLPrivActDropRoutine()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropRoutine::displayLabel1() const
{
  return NAString("Drop Routine privilege action");
}

const NAString
ElemDDLPrivActDropRoutine::getText() const
{
  return "ElemDDLPrivActDropRoutine";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropRoutineAction
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropRoutineAction::~ElemDDLPrivActDropRoutineAction()
{
}

// casting
ElemDDLPrivActDropRoutineAction *
ElemDDLPrivActDropRoutineAction::castToElemDDLPrivActDropRoutineAction()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropRoutineAction::displayLabel1() const
{
  return NAString("Drop RoutineAction privilege action");
}

const NAString
ElemDDLPrivActDropRoutineAction::getText() const
{
  return "ElemDDLPrivActDropRoutineAction";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropSynonym
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropSynonym::~ElemDDLPrivActDropSynonym()
{
}

// casting
ElemDDLPrivActDropSynonym *
ElemDDLPrivActDropSynonym::castToElemDDLPrivActDropSynonym()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropSynonym::displayLabel1() const
{
  return NAString("Drop Synonym privilege action");
}

const NAString
ElemDDLPrivActDropSynonym::getText() const
{
  return "ElemDDLPrivActDropSynonym";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropTable
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropTable::~ElemDDLPrivActDropTable()
{
}

// casting
ElemDDLPrivActDropTable *
ElemDDLPrivActDropTable::castToElemDDLPrivActDropTable()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropTable::displayLabel1() const
{
  return NAString("Drop table privilege action");
}

const NAString
ElemDDLPrivActDropTable::getText() const
{
  return "ElemDDLPrivActDropTable";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropTrigger
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropTrigger::~ElemDDLPrivActDropTrigger()
{
}

// casting
ElemDDLPrivActDropTrigger *
ElemDDLPrivActDropTrigger::castToElemDDLPrivActDropTrigger()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropTrigger::displayLabel1() const
{
  return NAString("Drop Trigger privilege action");
}

const NAString
ElemDDLPrivActDropTrigger::getText() const
{
  return "ElemDDLPrivActDropTrigger";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActDropView
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActDropView::~ElemDDLPrivActDropView()
{
}

// casting
ElemDDLPrivActDropView *
ElemDDLPrivActDropView::castToElemDDLPrivActDropView()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActDropView::displayLabel1() const
{
  return NAString("Drop view privilege action");
}

const NAString
ElemDDLPrivActDropView::getText() const
{
  return "ElemDDLPrivActDropView";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActExecute
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActExecute::~ElemDDLPrivActExecute()
{
}

// casting
ElemDDLPrivActExecute *
ElemDDLPrivActExecute::castToElemDDLPrivActExecute()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActExecute::displayLabel1() const
{
  return NAString("Execute privilege action");
}

const NAString
ElemDDLPrivActExecute::getText() const
{
  return "ElemDDLPrivActExecute";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActInsert
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActInsert::~ElemDDLPrivActInsert()
{
}

// casting
ElemDDLPrivActInsert *
ElemDDLPrivActInsert::castToElemDDLPrivActInsert()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActInsert::displayLabel1() const
{
  return NAString("Insert privilege action");
}

const NAString
ElemDDLPrivActInsert::getText() const
{
  return "ElemDDLPrivActInsert";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActMaintain
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActMaintain::~ElemDDLPrivActMaintain()
{
}

// casting
ElemDDLPrivActMaintain *
ElemDDLPrivActMaintain::castToElemDDLPrivActMaintain()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActMaintain::displayLabel1() const
{
  return NAString("Maintain privilege action");
}

const NAString
ElemDDLPrivActMaintain::getText() const
{
  return "ElemDDLPrivActMaintain";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActReferences
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActReferences::~ElemDDLPrivActReferences()
{
}

// casting
ElemDDLPrivActReferences *
ElemDDLPrivActReferences::castToElemDDLPrivActReferences()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActReferences::displayLabel1() const
{
  return NAString("References privilege action");
}

const NAString
ElemDDLPrivActReferences::getText() const
{
  return "ElemDDLPrivActReferences";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActReorg
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActReorg::~ElemDDLPrivActReorg()
{
}

// casting
ElemDDLPrivActReorg *
ElemDDLPrivActReorg::castToElemDDLPrivActReorg()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActReorg::displayLabel1() const
{
  return NAString("Reorg privilege action");
}

const NAString
ElemDDLPrivActReorg::getText() const
{
  return "ElemDDLPrivActReorg";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActRefresh
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActRefresh::~ElemDDLPrivActRefresh()
{
}

// casting
ElemDDLPrivActRefresh *
ElemDDLPrivActRefresh::castToElemDDLPrivActRefresh()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActRefresh::displayLabel1() const
{
  return NAString("Refresh privilege action");
}

const NAString
ElemDDLPrivActRefresh::getText() const
{
  return "ElemDDLPrivActRefresh";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActReplicate
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActReplicate::~ElemDDLPrivActReplicate()
{
}

// casting
ElemDDLPrivActReplicate *
ElemDDLPrivActReplicate::castToElemDDLPrivActReplicate()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActReplicate::displayLabel1() const
{
  return NAString("Replicate privilege action");
}

const NAString
ElemDDLPrivActReplicate::getText() const
{
  return "ElemDDLPrivActReplicate";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAllDML
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAllDML::~ElemDDLPrivActAllDML()
{
}

// casting
ElemDDLPrivActAllDML *
ElemDDLPrivActAllDML::castToElemDDLPrivActAllDML()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAllDML::displayLabel1() const
{
  return NAString("All DML privilege action");
}

const NAString
ElemDDLPrivActAllDML::getText() const
{
  return "ElemDDLPrivActAllDML";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActSelect
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActSelect::~ElemDDLPrivActSelect()
{
}

// casting
ElemDDLPrivActSelect *
ElemDDLPrivActSelect::castToElemDDLPrivActSelect()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActSelect::displayLabel1() const
{
  return NAString("Select privilege action");
}

const NAString
ElemDDLPrivActSelect::getText() const
{
  return "ElemDDLPrivActSelect";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActTransform
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActTransform::~ElemDDLPrivActTransform()
{
}

// casting
ElemDDLPrivActTransform *
ElemDDLPrivActTransform::castToElemDDLPrivActTransform()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActTransform::displayLabel1() const
{
  return NAString("Transform privilege action");
}

const NAString
ElemDDLPrivActTransform::getText() const
{
  return "ElemDDLPrivActTransform";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActUpdate
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActUpdate::~ElemDDLPrivActUpdate()
{
}

// casting
ElemDDLPrivActUpdate *
ElemDDLPrivActUpdate::castToElemDDLPrivActUpdate()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActUpdate::displayLabel1() const
{
  return NAString("Update privilege action");
}

const NAString
ElemDDLPrivActUpdate::getText() const
{
  return "ElemDDLPrivActUpdate";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActUpdateStats
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActUpdateStats::~ElemDDLPrivActUpdateStats()
{
}

// casting
ElemDDLPrivActUpdateStats *
ElemDDLPrivActUpdateStats::castToElemDDLPrivActUpdateStats()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActUpdateStats::displayLabel1() const
{
  return NAString("UpdateStats privilege action");
}

const NAString
ElemDDLPrivActUpdateStats::getText() const
{
  return "ElemDDLPrivActUpdateStats";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActAllDDL
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActAllDDL::~ElemDDLPrivActAllDDL()
{
}

// casting
ElemDDLPrivActAllDDL *
ElemDDLPrivActAllDDL::castToElemDDLPrivActAllDDL()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActAllDDL::displayLabel1() const
{
  return NAString("AllDDL privilege action");
} 

const NAString
ElemDDLPrivActAllDDL::getText() const
{
  return "ElemDDLPrivActAllDDL";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActUsage
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivActUsage::~ElemDDLPrivActUsage()
{
}

// casting
ElemDDLPrivActUsage *
ElemDDLPrivActUsage::castToElemDDLPrivActUsage()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivActUsage::displayLabel1() const
{
  return NAString("Usage privilege action");
}

const NAString
ElemDDLPrivActUsage::getText() const
{
  return "ElemDDLPrivActUsage";
}




// -----------------------------------------------------------------------
// methods for class ElemDDLPrivActWithColumns
// -----------------------------------------------------------------------

//
// constructor
//

ElemDDLPrivActWithColumns::ElemDDLPrivActWithColumns(
     OperatorTypeEnum operatorType,
     ElemDDLNode * pColumnNameList,
     CollHeap    * heap)
: ElemDDLPrivAct(operatorType),
  columnNameArray_(heap)
{
  setChild(INDEX_COLUMN_NAME_LIST, pColumnNameList);

  //
  // copies pointers to parse nodes representing column
  // names (appearing in a privilege action) to
  // columnNameArray_ so the user can access this
  // information easier.
  //

  if (pColumnNameList NEQ NULL)
  {
    for (CollIndex i = 0; i < pColumnNameList->entries(); i++)
    {
      columnNameArray_.insert((*pColumnNameList)[i]->castToElemDDLColName());
    }
  }
}

// virtual destructor
ElemDDLPrivActWithColumns::~ElemDDLPrivActWithColumns()
{
}

ElemDDLPrivActWithColumns *
ElemDDLPrivActWithColumns::castToElemDDLPrivActWithColumns()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLPrivActWithColumns::getArity() const
{
  return MAX_ELEM_DDL_PRIV_ACT_WITH_COLUMNS_ARITY;
}

ExprNode *
ElemDDLPrivActWithColumns::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutator
//

void
ElemDDLPrivActWithColumns::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
  {
    children_[index] = NULL;
  }
}

//
// methods for tracing
//

NATraceList
ElemDDLPrivActWithColumns::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;
  ElemDDLNode   * pColumnNameList = getColumnNameList();

  //
  // kind of privilege action
  //
  //   Note that the invoked displayLabel1() is a method of
  //   class ElemDDLPrivActInsert, ElemDDLPrivActReferences,
  //   or ElemDDLPrivActUpdate.
  //

  detailTextList.append(displayLabel1());

  //
  // column name list
  //

  if (pColumnNameList EQU NULL)
  {
    detailTextList.append("No column name list.");
    return detailTextList;
  }

  detailText = "Column Name List [";
  detailText += LongToNAString((Lng32)pColumnNameList->entries());
  detailText += " element(s)]:";
  detailTextList.append(detailText);

  for (CollIndex i = 0; i < pColumnNameList->entries(); i++)
  {
    detailText = "[column ";
    detailText += LongToNAString((Lng32)i);
    detailText += "]";
    detailTextList.append(detailText);

    detailTextList.append("    ", (*pColumnNameList)[i]->getDetailInfo());
  }
  return detailTextList;
}

const NAString
ElemDDLPrivActWithColumns::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLPrivActWithColumns";
}

//
// End of File
//
