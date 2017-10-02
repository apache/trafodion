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
* File:         Triggers.cpp
* Description:  
* Created:      06/23/98
*
*
******************************************************************************
*/

#include "AllRelExpr.h"
#include "ItemFunc.h"
#include "ItemOther.h"  // for Convert
#include "Triggers.h"
#include "TriggerDB.h"
#include "parser.h"
#include "StmtNode.h"
#include "ComDiags.h"
#include "StmtDDLCreateTrigger.h"
#include "BindWA.h"
#include "ChangesTable.h"
#include "MvRefreshBuilder.h"

const static short MAX_RECURSION_DEPTH = 16;	// limit for recursive triggers

//-----------------------------------------------------------------------------
//
// Take the name of the subject table; return the name of its trig-temp table
//
NAString 
subjectNameToTrigTemp( const NAString & subjectTableName )
{
  NAString trigTempTableName( subjectTableName );
  ChangesTable::addSuffixToTableName(trigTempTableName, TRIG_TEMP_TABLE_SUFFIX);
  return trigTempTableName;
}

//
// Take the name of the trig-temp table; return the name of its subject table
// ( Name of the trig-temp _MUST_ be "well formatted" !!! (i.e., generated
//   by  subjectNameToTrigTemp() ) )
//
// no longer used -buggy
NAString 
trigTempToSubjectName( const NAString & trigTempTableName )
{
  NAString subjectTableName( trigTempTableName );
  size_t nameLen = subjectTableName.length();
  ComBoolean isQuoted = FALSE;
  // special case: string ends with a quote char
  if ( '"' == subjectTableName[ nameLen - 1 ] ) 
  { 
    subjectTableName = subjectTableName.strip(NAString::trailing,'"');
    isQuoted = TRUE;
  }
  if ( TRIG_TEMP_TABLE_SUFFIX NEQ NULL ) 
  {
    CMPASSERT(subjectTableName.length() >= sizeof(TRIG_TEMP_TABLE_SUFFIX));
    subjectTableName.remove(subjectTableName.length() - sizeof(TRIG_TEMP_TABLE_SUFFIX) + 1);
  }
  if ( isQuoted ) subjectTableName.append( "\"" );

  return subjectTableName;
}

//-----------------------------------------------------------------------------
//
// -- class UpdateColumns Methods

// Ctor from STOI column list
UpdateColumns::UpdateColumns(SqlTableOpenInfo *stoi)
  : allColumns_(FALSE)
{
  columns_ = new (Trigger::Heap()) SET(Lng32) (Trigger::Heap());
  if (stoi != NULL)
  {
      for (short i=0; i<stoi->getColumnListCount(); i++)
        addColumn(stoi->getUpdateColumn(i));
  }
}

//
// -- UpdateColumns::~UpdateColumns
//

UpdateColumns::~UpdateColumns()	
{
  if (columns_)
  {
    delete columns_;
  }
}


//
// -- UpdateColumns::match
//
// Implements match-any: It is enough that there is a common column in the 
// subject columns and in the updated columns for the relevant trigger to fire.
//
NABoolean 
UpdateColumns::match(const UpdateColumns &other) const
{
  // all columns are specified iff the columns hash is not allocated
  CMPASSERT(isAllColumns() == !columns_);

  // if either the trigger is defined on all columns or the update is on all 
  // columns then return true at once
  if (isAllColumns() || other.isAllColumns())
    return TRUE;

  CollIndex numEntries = other.columns_->entries();
  for (CollIndex i = 0; i < numEntries; i++)
    if (columns_->contains((*other.columns_)[i]))
      return TRUE;

  return FALSE;
}

//
// -- UpdateColumns::markColumnsOnBitmap
//
// Create a bitmap of the updated columns.
//
void 
UpdateColumns::markColumnsOnBitmap(unsigned char *bitmap, CollIndex numBytes) const
{
  // For every updated column, mark the corresponding bit.
  if (isAllColumns())
  {
    // Mark all the bits as TRUE.
    for (Int32 i=0; i<numBytes; i++)
      bitmap[i] = 0xFF;
  }
  else
  {
    // zero out the bitmap first
    for (Int32 i=0; i<numBytes; i++)
      bitmap[i] = 0;

    CollIndex numEntries = columns_->entries();
    for (CollIndex i = 0; i < numEntries; i++)
      {
        Int32 byteIx = (*columns_)[i] / 8;
        Int32 bitIx = (*columns_)[i] % 8;

        if (byteIx < numBytes)
          bitmap[byteIx] |= 0x01 << bitIx;
      }
  }
}

//
// -- UpdateColumns::print
//
// Debugging aid
//
// used for debugging only
void 
UpdateColumns::print(ostream & os, const char* indent, const char* title) const
{
  os << indent << title << " ";

  if (isAllColumns()) 
  {
    os << "All Columns\n";
    return;
  }

  CollIndex numEntries = columns_->entries();
  for (CollIndex i = 0; i < numEntries; i++)
    os << (*columns_)[i] << ", ";

  os << endl;
}

//-----------------------------------------------------------------------------
//
// -- class Trigger Methods

//
// -- Trigger::~Trigger
//						
// only used when triggers are allocated from the cntext heap
// are trigger object's get desructed explicitly
// Currently triggers are allocated from the statement heap.
// See method Trigger::Heap() in file Triggers.h for more details
Trigger::~Trigger()
{
  // only when persistence is active, trigger object's get desructed
  // explicitly
  CMPASSERT(Heap() == CmpCommon::contextHeap());

  // delete the relExpr tree
  if (parsedTrigger_)
    delete (parsedTrigger_);

  //-----------------------------------------------------------------
  // NOTE: the following fields are pointers that are not 
  // allocated by this object, yet it is the allocation policy - 
  // Trigger object members are allocated by the catman, but destroyed 
  // by the Trigger dtor.
  //-----------------------------------------------------------------

  if (updateCols_)
    delete updateCols_;
  if (sqlText_)
    delete sqlText_;	
}

//
// -- Trigger:: equality
//
NABoolean 
Trigger::operator ==(const Trigger &other) const
{
  return name_ == other.name_;
}


//------------------------------------------------------------------------------
//
// -- Trigger::parseTriggerText
//
// Parse the SQL trigger text in sqlText_ .
// If the parsing fails NULL is returned (and the caller, i.e., binder) has to 
// handle the error. 
// The returned RelExpr tree  is allocated from the StatementHeap.
// A dummy select list (with arity 1) is forced for the RelRoot, since this 
// subtree returns nothing, and it is essential for (ordered-) union 
// compatability.
//
StmtDDLCreateTrigger * 
Trigger::parseTriggerText() const
{
  ExprNode *parsedNode;
  RelExpr  *triggerBody;

  CMPASSERT(sqlText_ != NULL);
  if (sqlTextCharSet_ == CharInfo::UnknownCharSet)
    const_cast<Trigger*>(this)->sqlTextCharSet_ = CharInfo::UTF8;

  // Parse the SQL text.
  Parser parser(CmpCommon::context());
  if (parser.parseDML(sqlText_->data(), sqlText_->length(), sqlTextCharSet_, &parsedNode))
    return NULL;

  // Extract the trigger body without the DDL nodes on top
  DDLExpr *parseTree = 
    (DDLExpr *)((StmtNode *)parsedNode->castToStatementExpr())->
      getQueryExpression()->getChild(0);
  CMPASSERT(parseTree->getOperatorType() == REL_DDL); 
  CMPASSERT(parseTree->getDDLNode()->getOperatorType()==DDL_CREATE_TRIGGER);

  StmtDDLCreateTrigger *createTriggerNode = 
    (StmtDDLCreateTrigger*)parseTree->getDDLNode();

  triggerBody = createTriggerNode->getActionExpression();
  
  if (isAfterTrigger())
  {
    CMPASSERT(triggerBody->getOperatorType() == REL_ROOT);
    ((RelRoot *)triggerBody)->setRootFlag(FALSE);
    ((RelRoot *)triggerBody)->setEmptySelectList();
  }

  return createTriggerNode;
}


//
// -- Trigger::getParsedTrigger
//
// 1) check the recursion limit. if the recursion depth limit is exceeded then 
//    (a) the trigger action is replaced to generate a RUN_TIME error 
//	  return an 'error node' allocated from the StatementHeap
//	  (b) a COMPILE-TIME warning is generated indicating the potential for a 
//     run time error in case the execution path will indeed reach this point.
//    (else)
// 2) if this is the first call to this function, then parse the trigger's text
//	  and return it, without initializing parsedTrigger_ (this is an 
//	  optimization for the common case in which triggers are not recursive).
// 3) if this is the second call to this func, initialize parsedTriggers_
//    and return a COPY of it to the caller. All other calls to this func
//    will return only a COPY of parsedTrigger_ (optimization again - most 
//	  likely that the recursion will not end after 2 steps only, thus we save 
//	  parsing over and over again each time.
//    
// (modified by <aviv>)
//
RelExpr * 
Trigger::getParsedTrigger(BindWA *bindWA)
{
  RelExpr *result = NULL;
  // check the resursion limit -- only after triggers can be recursive
  if (isAfterTrigger() && recursionCounter_ > MAX_RECURSION_DEPTH) 
  { 
    // prepare error generating RelExpr for runtime
    RaiseError *error = new (CmpCommon::statementHeap()) 
	RaiseError(11003, getTriggerName(), getSubjectTableName());
    Tuple *tuple = new (CmpCommon::statementHeap()) 
	TupleList(new (CmpCommon::statementHeap()) Convert(error));

    // When execution path gets here, limit was already exceeded, and
    // therefore error will be emitted unconditionally.
    result =  new (CmpCommon::statementHeap()) RelRoot(tuple);
    ((RelRoot *)result)->setRootFlag(FALSE);
    ((RelRoot *)result)->setEmptySelectList();

    // emit a compile-time warning
    *CmpCommon::diags() << DgSqlCode(11002) << 
      DgString0(getTriggerName()) << 
      DgString1(getSubjectTableName());
  }
  else 
  { // recursion limit not exceeded
    if (parsedTrigger_ == NULL) 
    { 
      //first call to this func
      parsedTrigger_ = parseTriggerText();
      if (parsedTrigger_ == NULL)
      {
         bindWA->setErrStatus();
         return NULL;
      }
      // parsing must succeed since it passed DDL parsing
      CMPASSERT(parsedTrigger_ != NULL);

      result = parsedTrigger_->getActionExpression();
    }
    else 
    { 
      // not the first call to this func
      // wasParsed_ == TRUE;
      if (!isVirginCopy_) 
      { // second call
	// Call the parser again since parsedTrigger_ points to an
	// already bound RelExpr tree. Save the pointer for the future
	parsedTrigger_ = parseTriggerText();
	// parsing must succeed since it passed DDL parsing
	CMPASSERT(parsedTrigger_ != NULL); 
      }
      // Return a copy allocated from Trigger::Heap
      result = 
	parsedTrigger_->getActionExpression()->copyTree(CmpCommon::statementHeap());
      isVirginCopy_ = TRUE;
    }
  }

  return result;
}
	
 
//
// -- Trigger::print
//
// Debugging aid.
//
// used for debugging only
void 
Trigger::print(ostream &os, const char* indent, const char* title) const
{
  os << indent << title << " ";
  os << indent << getTriggerName() << " : " << endl;
  BUMP_INDENT(indent);
  os << indent << "Subject Table Name : " << getSubjectTableName() << endl;
  os << indent << "Granularity: " << 
	  (isStatementTrigger() ? "Statement " : "Row ") << endl;
  os << indent << "Activation: " << 
	  (isBeforeTrigger() ? "Before " : "After ") << endl;
  os << indent << "Operation: ";
  switch (getOperation()) 
  {
    case COM_UPDATE: os << "Update";
      break;
    case COM_DELETE: os << "Delete";
      break;
    case COM_INSERT: os << "Insert";
      break;
  }
  os << endl;

  if (operation_ == COM_UPDATE)
    updateCols_->print(os, indent, "Subject Columns");

  os << "TimeStamp: " << convertInt64ToDouble(timeStamp_) << endl;
  os << "Recursion Counter: " << recursionCounter_ << endl;
}


//------------------------------------------------------------------------------
//
// -- MVImmediate::getParsedTrigger
//
// Returns the refresh tree for this MVImmediate object, i.e. ON STATEMENT MV.

RelExpr *MVImmediate::getParsedTrigger(BindWA *bindWA)
{
  return triggerBuilder_->buildRefreshTree();
}

//------------------------------------------------------------------------------
//
// -- TriggerList::calcColumnMatchingTriggers
//
// Used by BeforeAndAfterTriggers::calcColumnMatchingTriggers
//

TriggerList* 
TriggerList::getColumnMatchingTriggers(const UpdateColumns *updateCols)
{
  CollIndex i;
  TriggerList* relevantTrgs = NULL;
  Trigger* trg;

  if (updateCols == NULL)
    return this;

  for (i=0; i<entries(); i++) 
  {
    trg=(*this)[i];
    if (trg->getUpdateColumns()->match(*updateCols)) 
    {
      if (!relevantTrgs) 
	relevantTrgs = new (CmpCommon::statementHeap()) 
	  TriggerList(CmpCommon::statementHeap());
      relevantTrgs->insert(trg);
    }
  }

  return relevantTrgs;
}

//
// -- TriggerList::sortByTimeStamp
//
// internally sorts the list by time stamp, so that (*this)[0] is the oldest Trigger
//
typedef Trigger* TriggerPtr;

// used in qsort
//
// Return Value
// -1 elem1 less than    elem2
//  0 elem1 equivalent   elem2
//  1 elem1 greater than elem2
//-----------------------------------------------------------------------------------


static Int32 triggerTimeCompare(const void *elem1, const void *elem2)
{
  ComTimestamp ts1 =  (*((TriggerPtr *)elem1))->getTimeStamp();
  ComTimestamp ts2 =  (*((TriggerPtr *)elem2))->getTimeStamp();
  if (ts1 > ts2) return 1;
  else if(ts1 < ts2 ) return -1;
  else return 0;
}


void 
TriggerList::sortByTimeStamp()
{
  CollIndex mySize = this->entries();
  // prepare an array of Trigger* for qsort
  TriggerPtr* trigArray = new TriggerPtr[mySize];

  // copy the list's entries into trigArray
  for (CollIndex idx = 0; idx < mySize; idx++)
    trigArray[idx] = (*this)[idx];

  // sort trigArray by timestamp
  opt_qsort(trigArray,
	mySize, 
	sizeof(Trigger*), 
	triggerTimeCompare);

  // set triggerList_ based on the reordered array optTriggerPtrArray
  this->clear();
  for (CollIndex k = 0 ; k < mySize; k++)
    this->insert(trigArray[k]);

  // cleanup
  delete[] trigArray;
}


//
// -- TriggerList::print
//
// Debugging aid.
//
// used for debugging only
void 
TriggerList::print(ostream & os, const char* indent, const char* title) const
{
  os << title << endl;
  BUMP_INDENT(indent);
  for (CollIndex i=0; i<entries(); i++)  
  {
    (*this)[i]->print(os, indent, "Trigger ");
    os << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . ";
  }
}

//
// -- TriggerList::clearAndDestroy()
//
// free the memory of all the Trigger* entries by calling their destructors
// (operator delete). This should deallocate them from the CollHeap from which 
// they were allocated. The list itself is also 'cleared'

// only used when triggers are allocated from the cntext heap. 
// Currently triggers are allocated from the statement heap.
// See method Trigger::Heap() in file Triggers.h
void
TriggerList::clearAndDestroy()
{
  Trigger *currEntry;
  for (CollIndex i=0; i < entries(); i++) 
  {
    currEntry = this->at(i);
    if (currEntry != NULL)
      delete currEntry;
  }
  this->clear();
}


//------------------------------------------------------------------------------
//
// -- class BeforeAndAfterTriggers Methods
 


//
// -- BeforeAndAfterTriggers::print()
//
// Debugging aid.
//
// used for debugging only
void 
BeforeAndAfterTriggers::print(ostream&    os, 
			      const char *indent, 
			      const char *title) const
{
  if (getBeforeTriggers())
    getBeforeTriggers()->print(os, indent, "Before Triggers: ");
  if (getAfterStatementTriggers())
    getAfterStatementTriggers()->print(os, indent, "After Statement Triggers: ");
  if (getAfterRowTriggers())
    getAfterRowTriggers()->print(os, indent, "After Row Triggers: ");
}


//
// -- BeforeAndAfterTriggers::clearAndDestroyAllEntries()
//
// free the memory of all the TriggerLists referenced by this object
// 
// only used when triggers are allocated from the cntext heap
// are trigger object's get desructed explicitly
// Currently triggers are allocated from the statement heap.
// See method Trigger::Heap() in file Triggers.h for more details
void
BeforeAndAfterTriggers::clearAndDestroy()
{
  if (NULL != afterRowTriggers_) 
  {
    afterRowTriggers_->clearAndDestroy();
    delete afterRowTriggers_;
  }

  if (NULL != afterStatementTriggers_) 
  {
    afterStatementTriggers_->clearAndDestroy();
    delete afterStatementTriggers_;
  }

  if (NULL != beforeTriggers_) 
  {
    beforeTriggers_->clearAndDestroy();
    delete beforeTriggers_;
  }
}


// 
// -- BeforeAndAfterTriggers::~BeforeAndAfterTriggers
//

// only used when triggers are allocated from the cntext heap
// are trigger object's get desructed explicitly
// Currently triggers are allocated from the statement heap.
// See method Trigger::Heap() in file Triggers.h for more details
BeforeAndAfterTriggers::~BeforeAndAfterTriggers() 
{
  // NOTE: if deep-destruction is required then
  // call this->clearAndDestroyAllEntries() here
}

// 
// -- BeforeAndAfterTriggers::entries
//
// returns the total number of triggers in all the lists together.

Lng32 BeforeAndAfterTriggers::entries() const
{
  Lng32 triggerCount = 0;

  // add before triggers to the total count
  if (beforeTriggers_)
  {
    triggerCount += beforeTriggers_->entries();
  }

  // add after statement triggers to the total count
  if (afterStatementTriggers_)
  {
    triggerCount += afterStatementTriggers_->entries();
  }

  // add after row triggers to the total count
  if (afterRowTriggers_)
  {
    triggerCount += afterRowTriggers_->entries();
  }

  return triggerCount;
}

// 
// -- BeforeAndAfterTriggers::addNewAfterStatementTrigger
//
// Add a new trigger into the list of after statement triggers. If the list was
// not allocated yet, it is allocated just before the insertion
// (aka lazy evaluation).

void
BeforeAndAfterTriggers::addNewAfterStatementTrigger(Trigger *newTrigger)
{
  if (!afterStatementTriggers_)
  {
    afterStatementTriggers_ = new(CmpCommon::statementHeap())
      TriggerList(CmpCommon::statementHeap());
  }

  afterStatementTriggers_->insert(newTrigger);
}

// 
// -- BeforeAndAfterTriggers::addNewAfterRowTrigger
//
// Add a new trigger into the list of after row triggers. If the list was
// not allocated yet, it is allocated just before the insertion
// (aka lazy evaluation).

void
BeforeAndAfterTriggers::addNewAfterRowTrigger(Trigger *newTrigger)
{
  if (!afterRowTriggers_)
  {
    afterRowTriggers_ = new(CmpCommon::statementHeap())
      TriggerList(CmpCommon::statementHeap());
  }

  afterRowTriggers_->insert(newTrigger);
}
