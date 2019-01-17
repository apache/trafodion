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
#ifndef TRIGGERS_H
#define TRIGGERS_H
/* -*-C++-*-
********************************************************************************
*
* File:         Triggers.h
* Description:	Definition of trigger objects and repository of common 
*				definitions related to triggers.
* Created:		06/23/98
*
*
*
*******************************************************************************
*/

#include "NABasicObject.h"
#include "Collections.h"
#include "ItemConstr.h"
#include "ComSmallDefs.h"
#include "NAString.h"
#include "ExecuteIdTrig.h"
#include "charinfo.h"

//-----------------------------------------------------------------------------
// classes defined in this file:

class Trigger;
class MVImmediate;
class UpdateColumns;
class TriggerList;
class BeforeAndAfterTriggers;

//-----------------------------------------------------------------------------
// Forward references
class SqlTableOpenInfo;
class StmtDDLCreateTrigger;
class MvRefreshBuilder;

//-----------------------------------------------------------------------------
//
// -- Transition Variables OLD/NEW Common Constant Variables
//
#undef  INIT_
#undef  GLOB_
#ifdef INITIALIZE_OLD_AND_NEW_NAMES
  #define INIT_(val)      = val
#else
  #define INIT_(val)
#endif

// const Globals that use the FUNNY_INTERNAL_IDENT macro should be extern.

// The OLD and NEW transition values.
extern const char OLDCorr[] INIT_(FUNNY_INTERNAL_IDENT("OLD"));	//  OLD@
extern const char NEWCorr[] INIT_(FUNNY_INTERNAL_IDENT("NEW"));	//  NEW@
extern const char OLDAnsi[] INIT_(FUNNY_ANSI_IDENT("OLD"));	// "OLD@"
extern const char NEWAnsi[] INIT_(FUNNY_ANSI_IDENT("NEW"));	// "NEW@"

// -- Triggers-Temporary Table Common Constant Variables
extern const char OLD_COLUMN_PREFIX[]       INIT_(FUNNY_INTERNAL_IDENT("OLD"));
extern const char NEW_COLUMN_PREFIX[]       INIT_(FUNNY_INTERNAL_IDENT("NEW")); 

// TBD: to be eliminated, both function and variable
const char TRIG_TEMP_TABLE_SUFFIX[]="__TEMP"; 
// The following two functions are used to convert the name of the subject
// table to the name of its triggers-temporary table and vise versa.
// (This is an "abstraction barrier": The rest of the code should only use
//  these calls, and have no internal knowledge of implementation.  Our
//  eventual goal is to have the two names be identical; this will be done
//  solely by changing the implementation of the following functions.)
NAString subjectNameToTrigTemp( const NAString & subjectTableName );      
NAString trigTempToSubjectName( const NAString & trigTempTableName );

const char UNIQUEEXE_COLUMN []=  "@UNIQUE_EXECUTE_ID";
const char UNIQUEEXE_QCOLUMN[]="\"@UNIQUE_EXECUTE_ID\"";
const char UNIQUEIUD_COLUMN []=  "@UNIQUE_IUD_ID";
const char UNIQUEIUD_QCOLUMN[]="\"@UNIQUE_IUD_ID\"";

//-----------------------------------------------------------------------------
//
// -- class Trigger

class Trigger : public NABasicObject 
{
public:	

  // Explicit ctor
  Trigger(const QualifiedName    &name,
		 const QualifiedName	&subjectTable,
		 ComOperation		operation,
		 ComActivationTime	activation,
		 ComGranularity		granularity,
		 ComTimestamp		timeStamp,
		 NAString              *sqlText,
		 CharInfo::CharSet      sqlTextCharSet,
		 UpdateColumns	       *updateCols = NULL
        )
  :  name_(name),
     subjectTable_(subjectTable),
     operation_(operation),
     activation_(activation),
     granularity_(granularity),
     timeStamp_(timeStamp),
     sqlText_(sqlText),
     sqlTextCharSet_(sqlTextCharSet),
     updateCols_(updateCols),
     recursionCounter_(0),
     parsedTrigger_(NULL),
     isVirginCopy_(FALSE)
  {}

  // copy ctor
  // should not use
  Trigger (const Trigger &other)
   :  name_(other.name_),
      subjectTable_(other.subjectTable_),
      operation_(other.operation_),
      activation_(other.activation_),
      granularity_(other.granularity_),
      timeStamp_(timeStamp_),
      updateCols_(other.updateCols_)
      // Should never be called. Supplied only for collections.
  { CMPASSERT(FALSE); }

  // dtor
  virtual ~Trigger();
  
  // assignment operator
  // should not use
  Trigger&  operator = (const Trigger& other)
    // Should never be called. Supplied only because of collections.
    { CMPASSERT(FALSE); return *this; }

  // equality operator
  NABoolean operator ==(const Trigger &other) const;

  // Accessors & mutators

  const inline NAString	getTriggerName()		const 
    { return name_.getQualifiedNameAsAnsiString(); }
  const inline NAString	getSubjectTableName()		const 
    { return subjectTable_.getQualifiedNameAsAnsiString(); }

  // used for debugging only - print methods
  inline ComOperation getOperation()		        const 
    { return operation_; }
  inline NABoolean  isBeforeTrigger() const 
    { return (activation_ == COM_BEFORE); }
  inline NABoolean  isStatementTrigger() const 
    { return (granularity_== COM_STATEMENT); }

  inline NABoolean  isAfterTrigger() const 
    { return (activation_ == COM_AFTER); }
  inline NABoolean  isRowTrigger() const 
    { return (granularity_== COM_ROW); }

  // MV
  virtual NABoolean isMVImmediate() const
    { return false; } // this is a regular trigger

  const inline UpdateColumns *getUpdateColumns()	const 
    { return updateCols_; }
  const inline NAString * getSqlText()			const 
    { return sqlText_; } 
  inline CharInfo::CharSet getSqlTextCharSet()		const 
    { return sqlTextCharSet_; } 
  inline ComTimestamp getTimeStamp()		        const 
    { return timeStamp_; }

  // return a COPY of the parsed trigger (RelExpr tree) allocated from the
  // statementHeap. Recursion limit checking is also done here: an error-node
  // is returned and a compile-time warning is generated (if the recursion 
  // limit was exceeded)
  virtual RelExpr *getParsedTrigger(BindWA *bindWA);
  StmtDDLCreateTrigger *getCreateTriggerNode() const { return parsedTrigger_; }

  // recursion counter used to enforce the limit on the depth of recursive
  // triggers. Inc/Dec are called from RelExpr::bindChildren.
  inline Int32  getRecursionCounter()       const { return recursionCounter_; }
  inline void incRecursionCounter()       { recursionCounter_++; }
  inline void decRecursionCounter()       { recursionCounter_--; }
  
  // no longer used
  inline void resetRecursionCounter()		{ recursionCounter_=0; }

  //-------------------------------------------------------------------------
  //				Memory Management and Trigger Persistence
  // A trigger object is always created by the Catman (readTriggerDef). 
  // Currently, it's memory is managed as part of the StatementHeap. When
  // Triggers become persistent across statements, it's meomery will be 
  // managed as part of the ContextHeap, and then there are cases where the 
  // Trigger dtor is called and then it deallocates objects pointed to by the 
  // Trigger object as well.
  //
  // Please refer to the "Reusing Trigger Objects" Section of the detailed 
  // design document for more information on persistence across statements of 
  // trigger objects.
  //
  // the Heap() definition is originally intended to control the 
  // heap from which all data stored in the TriggerDB (including all
  // classes and members declared in this file and the TriggerDB itself) 
  // will be allocated. In order to make TriggerDB persistant across 
  // statements, the definition should have been CmpCommon::contextHeap(). 
  // However, in this version, some RelExpr and ItemExpr nodes cannot be 
  // allocated or copied to/from the ContextHeap, since some ctor and 
  // copying code specifies CmpCommon::statementHeap() explicitly, regardless 
  // of the parameter given to the overloaded new operator.
  // Therefore, until this is fixed, TriggerDB and its content cannot be
  // allocated from the CmpCommon::contextHeap(), and is not persistent 
  // across statements. 
  //
  // The Heap() method is the ONLY thing that needs to be changed to activate
  // Trigger persistence.
  //-------------------------------------------------------------------------
  inline static NAMemory* Heap()
    { return CmpCommon::statementHeap(); }

  // for debug
  void print(ostream& os, const char* indent, const char* title) const;

private:

  StmtDDLCreateTrigger *parseTriggerText() const;

  const QualifiedName   name_;
  const QualifiedName   subjectTable_;
  const UpdateColumns  *updateCols_;

  const ComOperation	operation_;		// Insert/Update/Delete (IUD)
  const ComActivationTime activation_;		// Before/After
  const ComGranularity	granularity_;		// Row/Statement
  const ComTimestamp	timeStamp_;		// creation time

  NAString	       *sqlText_;
  CharInfo::CharSet     sqlTextCharSet_;
  StmtDDLCreateTrigger *parsedTrigger_;	
  short			recursionCounter_;

  // Is this the result of the first parse, or a copy of the second?
  NABoolean		isVirginCopy_;	    

};

//-----------------------------------------------------------------------------
//
// -- class MVImmediate
//
// MVImmediate implements the refreshing of ON STATEMENT MVs using the Triggers
// backbone. Each such MV is considered as a special type of trigger. This
// trigger is an after-trigger with special timestamp to ensure its firing before
// any other trigger defined on the same subject table. The sqlText parameter is
// not in use. This trigger differs from a regular trigger only in the behaviour
// of the virtual method getParsedTrigger. Unlike regular triggers, this one is
// not originated from a "create trigger" statement. The appropriate refresh
// tree is determined by the builder (MvRefreshBuilder object) given to it by
// parameter.
//
// Important note on timestamps:
// ------------------------------
// Since indirect-update operations consists two such triggers, we must ensure
// these triggers have unique timestamps (so the sorting according timestamps
// will be deterministic). The "row trigger" part will have timestamp of zero,
// while the "statement trigger" part will have timestamp of one. Since
// timestamp of one is enough to be sure it will fire before any regular
// trigger, we use it for any statement trigger of type MVImmediate.

class MVImmediate : public Trigger {
public:

  MVImmediate(BindWA              *bindWA,
	      MvRefreshBuilder    *triggerBuilder,
	      const QualifiedName &mvName,
	      const QualifiedName &subjectTable,
	      ComOperation	  operation,
	      ComGranularity	  granularity,
	      UpdateColumns	  *updateCols)  // should set to NULL if not used
  : Trigger(mvName,
	    subjectTable,
	    operation,
	    COM_AFTER,
	    granularity,
	    (granularity == COM_STATEMENT ? 1L : 0L),
	    new NAString(""), CharInfo::UnknownCharSet,
	    updateCols),
    bindWA_(bindWA),
    triggerBuilder_(triggerBuilder)
  {};

  virtual RelExpr *getParsedTrigger(BindWA *bindWA);
  virtual NABoolean isMVImmediate() const 
    { return true; } // this is a special trigger (ON STATEMENT MV)

private:

  BindWA *bindWA_; // for use by the builders
  MvRefreshBuilder *triggerBuilder_; // the builder to build the refresh tree
};

//-----------------------------------------------------------------------------
//
// -- class UpdateColumns
//
// Used to implement the match-any semantics of Update triggers: If there is 
// an intersection between the set of subject columns and the set of updated 
// columns, then the trigger is fired. An internal small hash table of column 
// positions is used to implement the intersection. If there are no explicit 
// subject columns, then all columns are considered as subject column and the 
// case is represented especially by isAllColumns_.
// Notice that Trigger::Heap() is used for memory allocation.

static const short INIT_SUBJECT_COLUMNS_NUMBER = 10;
 
class UpdateColumns : public NABasicObject 
{
public:

  UpdateColumns(NABoolean allColumns)
    :  allColumns_(allColumns)
  {
    if (!allColumns)
      columns_ = new (Trigger::Heap()) SET(Lng32) (Trigger::Heap());
    else
      columns_ = NULL;
  }

  // Ctor from STOI column list
  UpdateColumns(SqlTableOpenInfo *stoi);

  virtual ~UpdateColumns();
  
  inline void addColumn(const Lng32 column) { columns_->insert(column); }  

  // Implements match-any: It is enough that there is a common column in the 
  // subject columns and in the updated columns for the relevant trigger to 
  // fire.
  NABoolean match(const UpdateColumns &other) const;  

  inline NABoolean contains(const Lng32 col) const
	  { return columns_->contains(col); }

  inline NABoolean isAllColumns()	      const { return allColumns_; }

  void markColumnsOnBitmap(unsigned char *bitmap, CollIndex numBytes) const;

  void print(ostream& os, const char* indent, const char* title) const;
  
private:

  NABoolean		 allColumns_;	// no explicit subject columns
  SET(Lng32)*             columns_;
  
}; 


//-----------------------------------------------------------------------------
//
// -- class TriggerList

class TriggerList : public LIST(Trigger *) 
{
public:

  TriggerList(NAMemory * h): LIST(Trigger *)(h) {};

  // returns the triggers whose subject columns intersect the given updateCols
  TriggerList* getColumnMatchingTriggers(const UpdateColumns *updateCols);

  // internally sorts the list by time stamp, so that (*this)[0] is the oldest Trigger
  void sortByTimeStamp();

  // free the memory of all the Trigger objects of the list
  void clearAndDestroy();

  void print(ostream& os, const char* indent, const char* title) const;

};

//-----------------------------------------------------------------------------
//
//
// -- class BeforeAndAfterTriggers
//
// Used only to group together existing triggers according to their activation
// time. Serves as the Value of the triggerDB hash.
//
class BeforeAndAfterTriggers : public NABasicObject
{
public:

  // explicit ctor, with timestamp, just copies pointers
  BeforeAndAfterTriggers(TriggerList *beforeTriggers, 
			 TriggerList *afterStatementTriggers,
			 TriggerList *afterRowTriggers,
			 ComTimestamp subjectTableTimestamp,
                         ComPrivilegeChecks doPrivCheck = COM_PRIV_NO_CHECK) 
  : beforeTriggers_(beforeTriggers),
    afterStatementTriggers_(afterStatementTriggers),
    afterRowTriggers_(afterRowTriggers),
    subjectTableTimestamp_(subjectTableTimestamp),
    doPrivCheck_(doPrivCheck)
  {}

  virtual ~BeforeAndAfterTriggers();

  // free the memory of all the lists referenced by this object
  void clearAndDestroy();

  // Accessors
  inline TriggerList *getBeforeTriggers()	  const 
    { return beforeTriggers_; }
  inline TriggerList *getAfterStatementTriggers() const 
    { return afterStatementTriggers_;  }
  inline TriggerList *getAfterRowTriggers()	  const 
    { return afterRowTriggers_;  }
  inline ComTimestamp getSubjectTableTimeStamp()  const 
    { return subjectTableTimestamp_; }
  
  Lng32 entries() const;

  // initialize lists for insertions
  void addNewAfterStatementTrigger(Trigger *newTrigger);
  void addNewAfterRowTrigger(Trigger *newTrigger);

  // Should never be called. Needed for compilation. We rely on 
  // NAHashBucket::contains() being called with a default NULL argument for
  // Values in the triggerDB hash where BeforeAndAfterTriggers are Values.
  NABoolean operator == (const BeforeAndAfterTriggers &other) const 
    { CMPASSERT(FALSE); return FALSE; }

  void print(ostream& os, const char* indent, const char* title) const;

private:
  TriggerList *beforeTriggers_;
  TriggerList *afterStatementTriggers_;
  TriggerList *afterRowTriggers_;
  ComTimestamp subjectTableTimestamp_; // used for validation purposes
  ComPrivilegeChecks doPrivCheck_;
};

#endif /* TRIGGERS_H */
