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
/* -*-C++-*-
******************************************************************************
*
* File:         TriggerDB.cpp
* Description:  
* Created:      06/23/98
*
*
******************************************************************************
*/

#include "AllRelExpr.h"
#include "SchemaDB.h"
#include "Triggers.h"
#include "TriggerDB.h"
#include "BindWA.h"

//-----------------------------------------------------------------------------
//
// -- class TableOp Methods

//
// -- TableOp::print
// 
// Debugging aid.
//
// For debugging only
void 
TableOp::print(ostream& os) const 
{
  os << "Triggers on ";
  switch (operation_) 
  {
    case COM_UPDATE: os << "Update";
      break;
    case COM_DELETE: os << "Delete";
      break;
    case COM_INSERT: os << "Insert";
      break;
  }
  os << "of Table " << subjectTable_.getQualifiedNameAsAnsiString() << " : " 
     << endl;
}

//-----------------------------------------------------------------------------
//
// -- class TriggerDB Methods

//
// -- TriggerDB::HashFunction 
//
// Make sure that triggers on different operations on the same table are hashed
// to different hash value.
//
ULng32 
TriggerDB::HashFunction(const TableOp & key) 
{
  ULng32 hval = key.subjectTable_.hash();
  if      (key.operation_ == COM_INSERT)
    return hval;
  else if (key.operation_ == COM_DELETE)
    return hval+1;
  else if (key.operation_ == COM_UPDATE)
    return hval+2;
  else 
  {
    CMPASSERT(FALSE);
    return 0;
  }
}

/*
ULng32 
TableOp::hash() const 
{ 
  return TriggerDB::HashFunction(*this);
}
*/
    
//
// --TriggerDB::getValidEntry
//
// return the BeforeAndAfterTriggers (ptr) given the key. 
// NULL is returned if no entry was found. In case of working across statements
// (Trigger::heap() == CmpCommon::contextHeap()) then this method
// VALIDATES (timestamp check) the return value before returning. 
// Note: a non-valid entry is first removed from the DB and a NULL is returned.
//
BeforeAndAfterTriggers* 
TriggerDB::getValidEntry(const TableOp * key, BindWA * bindWA)
{
  BeforeAndAfterTriggers* result = 
    NAHashDictionary<TableOp, BeforeAndAfterTriggers>::getFirstValue(key);

  if ((result != NULL) && (Trigger::Heap() == CmpCommon::contextHeap())) 
  { 
    // only used when triggers are allocated from the cntext heap. 
    // Currently triggers are allocated from the statement heap.
    // See method Trigger::Heap() in file Triggers.h for more details

    // entry exist in TriggerDB and we work ACROSS statements =>
    //   validate the entry: compare the entry's subject table timestamp
    //   against the actual subject table's timestamp 

    // convert to ExtendedQualName 
    ExtendedQualName exSubjectTable = 
      ExtendedQualName(key->subjectTable_, CmpCommon::statementHeap());
    // fetch the subject table from the NATableDB
    NATable *subjectTable = 
      bindWA->getSchemaDB()->getNATableDB()->get(&exSubjectTable);

    // the subject table MUST appear in the TableTB because it must have 
    // been fetched earlier by the binder (also in the case of cascaded 
    // triggers)
    CMPASSERT(subjectTable != NULL);
    ComTimestamp trigsTS = result->getSubjectTableTimeStamp();
    CMPASSERT(trigsTS != 0);	// must have been initialized by the catman

    if (subjectTable->getRedefTime() != trigsTS) 
    { 
      // no match => invalidate
      this->remove((TableOp*) key);
      result->clearAndDestroy();
      delete result; // destroy the entry
      return NULL;
    }
  } // end of validation

  // at this point, if result != NULL, then it is valid. Otherwise it is 
  // not in the TriggerDB
  return result;
}

// -- TriggerDB::getTriggers
//
// This method is the driver of getting triggers to the binder:
// It performs caching of triggers in triggerDB
// It accesses the Catman to get triggers (readTriggersDef())
//
BeforeAndAfterTriggers * 
TriggerDB::getTriggers(QualifiedName &subjectTable, 
		       ComOperation   operation, 
		       BindWA        *bindWA)
{
  BeforeAndAfterTriggers *triggers = NULL;

  return triggers;
}

// only used when triggers are allocated from the cntext heap. 
// Currently triggers are allocated from the statement heap.
// See method Trigger::Heap() in file Triggers.h for more details
void
TriggerDB::clearAndDestroy()
{
  NAHashDictionaryIterator<TableOp,BeforeAndAfterTriggers> iter(*this);
  
  TableOp * key = NULL;
  BeforeAndAfterTriggers* value = NULL;
  // iterate over all entries and destroy them
#pragma warning (disable : 4018)   //warning elimination
  for (Int32 i=0; i < iter.entries(); i++) 
#pragma warning (default : 4018)   //warning elimination
  {
    iter.getNext(key, value);
    CMPASSERT(key != NULL);
    CMPASSERT(value != NULL);
    value->clearAndDestroy();
    delete value;
    value = NULL;
    delete key;
    key = NULL;
  }
  this->clear(FALSE);

  // now, TriggerDB should be empty
  CMPASSERT(this->entries() == 0);
}

//
// -- ResetRecursionCounter()
//
// When triggerDB is allocated from the contextHeap (see func heap()),
// then we must make sure that after each statement the recursion counter 
// of every Trigger object is 0. Called from TriggerDB::cleanupPerStatement().
//
// only used when triggers are allocated from the cntext heap. 
// Currently triggers are allocated from the statement heap.
// See method Trigger::Heap() in file Triggers.h for more details
static void 
ResetRecursionCounter(TriggerList* triggerList)
{
  if (triggerList == NULL)
    return;
  Trigger * trg;
  for (CollIndex i=0; i<triggerList->entries(); i++) 
  {
    trg=(*triggerList)[i];
    trg->resetRecursionCounter();
  }
}

//
// -- TriggerDB::cleanupPerStatement()
//
// Called only in case that trigger-persistence is active. Returns a boolean 
// indicating whether to deallocate triggerDB or not.
// If the triggerDB_ is too big (THRASHOLD exceeded) then clear triggerDB and
// dealocate it. Else, reset the recursion counters, since triggers persist 
// across statements.
//
// only used when triggers are allocated from the cntext heap. 
// Currently triggers are allocated from the statement heap.
// See method Trigger::Heap() in file Triggers.h for more details
NABoolean
TriggerDB::cleanupPerStatement()
{
  CMPASSERT(Trigger::Heap() == CmpCommon::contextHeap());

  if (this != NULL && (entries() >= TRIGGERDB_THRASHOLD)) 
  {
    clearAndDestroy();
    // caller will dealocate triggerDB itself
    return TRUE;
  } 
  else 
  {
    NAHashDictionaryIterator<TableOp,BeforeAndAfterTriggers> iter(*this);

    TableOp * key = NULL;
    BeforeAndAfterTriggers* curr = NULL;

    // iterate over all entries 
#pragma warning (disable : 4018)   //warning elimination
    for (Int32 i=0; i < iter.entries(); i++) 
#pragma warning (default : 4018)   //warning elimination
    {
      iter.getNext(key, curr);
      CMPASSERT(curr != NULL);

      //reset recursion counter for all lists
      ResetRecursionCounter(curr->getAfterRowTriggers());
      ResetRecursionCounter(curr->getAfterStatementTriggers());
      ResetRecursionCounter(curr->getBeforeTriggers());

      curr = NULL;
    }
    // Do not deallocate
    return FALSE;
  }
}

//
// -- TriggerDB::print
//
// Debugging aid.
//
// For debugging only
void TriggerDB::print(ostream& os) const 
{
  NAHashDictionaryIterator<TableOp, BeforeAndAfterTriggers> iter (*this) ; 
  TableOp * top;
  BeforeAndAfterTriggers * triggers;

  iter.getNext(top,triggers);
  while (top) 
  {
    os << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " 
       << endl;
    top->print(os);
    os << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " 
       << endl;
    triggers->print(os, "", "");
    os << endl 
       << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " 
       << endl;
    iter.getNext(top, triggers);
  }
}

NABoolean TriggerDB::isHiveTable(QualifiedName& name)
{
   return strcmp(name.getSchemaName(), "HIVE") == 0;
}



//-----------------------------------------------------------------------------
//
// -- class SchemaDB methods

//
// -- SchemaDB::getRIs
//
// TBD
//
// not implemented
RefConstraintList * 
SchemaDB::getRIs(QualifiedName &subjectTable, 
		 ComOperation   operation)
{
  return NULL;
}

