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
#ifndef STMTDDLCREATETRIGGER_H
#define STMTDDLCREATETRIGGER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateTrigger.h
 * Description:  class representing Create Trigger Statement parser nodes
 *
 *
 * Created:      1/21/98
 * Language:     C++
 * Status:       $State: Exp $
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: StmtDDLCreateTrigger.h,v $
// Revision 1.0  1998/01/21 14:57:46
// Initial revision
//
//
// -----------------------------------------------------------------------

#include "ElemDDLNode.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLLocation.h"
#include "ElemDDLPartitionArray.h"
#include "NAString.h"
#include "StmtDDLNode.h"
#include "ParNameLocList.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateTrigger;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Trigger statement
// -----------------------------------------------------------------------
class StmtDDLCreateTrigger : public StmtDDLNode
{

public:

  // initialize constructor
  StmtDDLCreateTrigger(const QualifiedName & aTriggerName,
		       ParNameLocList  * nameLocList,
		       NABoolean       isAfter,
		       NABoolean       hasRowOrTableInRefClause,
		       ComOperation    iud_event,
		       ElemDDLNode     * columnList,
		       const QualifiedName & aTableName,
		       NAString        * oldName,
		       NAString        * newName,
		       NABoolean       isStatement,
                       ElemDDLNode     * pOwner,
		       CollHeap        * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateTrigger();

  // cast
  virtual StmtDDLCreateTrigger * castToStmtDDLCreateTrigger();

  //
  // accessors
  //

  // methods relating to parse tree
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline const ElemDDLColRefArray & getColRefArray() const;
  inline ElemDDLColRefArray & getColRefArray();

        // returns a NAList of ElemDDLColRef parse nodes.

  inline const NAString getTriggerName() const;
  inline const QualifiedName & getTriggerNameAsQualifiedName() const;
  inline       QualifiedName & getTriggerNameAsQualifiedName() ;

  inline const ElemDDLColRefArray & getPartitionKeyColRefArray() const;
  inline       ElemDDLColRefArray & getPartitionKeyColRefArray();

        // returns column name list in partition by clause if
        // specified; otherwise, an empty array is returned.

  inline const NAString getTableName() const;
  inline const QualifiedName& getTableNameObject() const;


  inline NABoolean isAfter() const;

        // returns TRUE if and only if this is an AFTER trigger 

  inline NABoolean hasRowORTableInRefClause () const;

        // Set to TRUE if and only if either or both keywords ROW
        // or TABLE appear in the referencing clause

  inline NABoolean isStatement() const;

        // returns TRUE if and only if this is a STATEMENT (not ROW) trigger 

  inline NABoolean isOwnerSpecified() const;

        // returns TRUE if the BY <owner> phrase appears
        // in the Create statement; returns FALSE otherwise.

  inline ComOperation getIUDEvent() const;

        // Return the IUD event ( COM_INSERT, COM_UPDATE, COM_DELETE )

  inline NABoolean areColumnsImplicit() const;

        // Return TRUE if and only if Columns are defined implicitly

  inline const NAString * getOldName() const;
  inline const NAString * getNewName() const;

  // Is this the name of the OLD or NEW transition table name?
  NABoolean isOldTransitionName(const NAString& tableName);
  NABoolean isNewTransitionName(const NAString& tableName);
  NABoolean isTransitionName(const NAString& tableName);
       
       //  Return the OLD / NEW correlation name
  //
  // mutators
  //

  void setChild(Lng32 index, ExprNode * newNode);

  void setAction( RelExpr * actionExpression );

  //
  // method for binding
  //

  ExprNode * bindNode(BindWA * pBindWA);
  RelExpr  * bindRowTrigger(BindWA *pBindWA); // Fix the pipelined values for row triggers.

  //
  // method for collecting information
  //
  
  void synthesize();

        // collects information in the parse sub-tree and
        // copy/move them to the current parse node.

  //
  // methods for tracing
  //

  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


  inline const ParNameLocList & getNameLocList() const;
  inline       ParNameLocList & getNameLocList();

        // returns a list of locations of names appearing in
        // the statement input string.  The list helps with
        // the computing of the trigger text.

  inline void setEndPosition(const StringPos endPos);

        // sets the ending position (the position of the
        // last character) of the statement (within the
        // input string)
  
  inline void setStartPosition(const StringPos startPos);

        // sets the starting position (the position of the
        // first character) of the statement (within the
        // input string)

  inline const StringPos getEndPosition() const;

        // returns the ending position (the position of the last
        // character) of the statement (within the input string)
  
  inline const StringPos getStartPosition() const;

        // returns the starting position (the position of the
        // first character) of the statement (within the
        // input string)
  
  inline const RelExpr * getActionExpression() const;
  inline       RelExpr * getActionExpression();

        // returns the pointer pointing to the parse sub-tree
        // representing the action expression in the trigger
        // definition.


  void setHasCSInAction ();
        // does the trigger referencing clause include ROW or TABLE
        // keyword


  inline const ElemDDLGrantee *getOwner() const;
        // returns pointer to the optional "by owner"
        // (in the form of an ElemDDLGrantee).

  NABoolean actionHasCompoundStatement () const;
        // does the trigger action has a compound statement
  
  //
  // pointers to child parse nodes
  //

  enum { TRIGGER_OPTIONAL_UPDATE_COLUMN_LIST = 0,
	 TRIGGER_ACTION_EXPRESSION,
         TRIGGER_OPTIONAL_OWNER,
         MAX_STMT_DDL_CREATE_TRIGGER_ARITY };


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------


  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  NABoolean isAfter_;

        // Set to TRUE if and only if this is an AFTER trigger

  NABoolean hasCSInAction_;
        // Set to TRUE if an only if this trigger has CS in action

  NABoolean hasRowORTableInRefClause_;

       // Set to TRUE if and only if either or both keywords ROW
       // or TABLE appear in the referencing clause

  NABoolean isStatement_;

        // Set to TRUE if and only if this is a STATEMENT trigger

  ElemDDLGrantee * pOwner_;

  ComOperation iudEvent_;

        // Set to one of: 1 - Insert, 2 - Update, 3 - Delete .

  // trigger name can only be a simple name
  QualifiedName triggerQualName_;

  // The syntax of table name is
  // [ [ catalog-name . ] schema-name . ] table-name

  QualifiedName tableQualName_;

  // list of column names (in case of an update trigger.)
  // ( The method synthesize() fills this variable, based on the
  //   information in *pColumnList_ )
  ElemDDLColRefArray columnRefArray_;

  // From the REFERENCING clause: NEW and OLD names
  NAString * oldName_;
  NAString * newName_;

  //
  // information about the position of the name within the input
  // string (to help with computing the trigger text)
  //

  ParNameLocList * nameLocListPtr_;
  StringPos startPos_;
  StringPos endPos_;

  //
  //  The child-nodes
  //
  ElemDDLNode * pColumnList_ ;
  RelExpr     * pActionExpression_;

}; // class StmtDDLCreateTrigger

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateTrigger
// -----------------------------------------------------------------------

inline QualifiedName &
StmtDDLCreateTrigger::getTriggerNameAsQualifiedName()
{
  return triggerQualName_;
}

inline const QualifiedName & 
StmtDDLCreateTrigger::getTriggerNameAsQualifiedName() const
{
  return triggerQualName_;
}

inline const ElemDDLColRefArray &
StmtDDLCreateTrigger::getColRefArray() const
{
  return columnRefArray_;
}

inline ElemDDLColRefArray &
StmtDDLCreateTrigger::getColRefArray()
{
  return columnRefArray_;
}


inline const RelExpr *
StmtDDLCreateTrigger::getActionExpression() const
{
  return pActionExpression_;
}

inline RelExpr *
StmtDDLCreateTrigger::getActionExpression()
{
  return pActionExpression_;
}

// get trigger name
inline const NAString 
StmtDDLCreateTrigger::getTriggerName() const
{
  return triggerQualName_.getQualifiedNameAsAnsiString();
}

// is this an AFTER trigger ?
inline NABoolean
StmtDDLCreateTrigger::isAfter() const
{
  return isAfter_;
}

// does the trigger action has a compound statement
inline NABoolean
StmtDDLCreateTrigger::actionHasCompoundStatement() const
{
  return hasCSInAction_;
}

// does the trigger action has a compound statement
inline void
StmtDDLCreateTrigger::setHasCSInAction ()
{
  hasCSInAction_ = TRUE;
}

// is either or both keywords ROW
// or TABLE appear in the referencing clause?
inline NABoolean
StmtDDLCreateTrigger::hasRowORTableInRefClause() const
{
  return hasRowORTableInRefClause_;
}

// is this a STATEMENT trigger ?
inline NABoolean
StmtDDLCreateTrigger::isStatement() const
{
  return isStatement_;
}

// return type of event
inline ComOperation
StmtDDLCreateTrigger::getIUDEvent() const
{
  return iudEvent_;
}

// Are columns defined implicitly (in case of an UPDATE)
inline NABoolean
StmtDDLCreateTrigger::areColumnsImplicit() const
{
  return ( pColumnList_ == NULL ? TRUE : FALSE ) ;
}

inline const NAString *
StmtDDLCreateTrigger::getOldName() const 
{ 
  return oldName_;
}

inline const NAString * 
StmtDDLCreateTrigger::getNewName() const 
{ 
  return newName_;
}

inline const NAString
StmtDDLCreateTrigger::getTableName() const
{
  return tableQualName_.getQualifiedNameAsAnsiString();
}

inline const QualifiedName&
StmtDDLCreateTrigger::getTableNameObject() const
{
  return tableQualName_;
}


inline const ParNameLocList & StmtDDLCreateTrigger::getNameLocList() const
{
  return *nameLocListPtr_;
}

inline ParNameLocList & StmtDDLCreateTrigger::getNameLocList()
{
  return *nameLocListPtr_;
}


inline void StmtDDLCreateTrigger::setEndPosition(const StringPos endPos)
{
  endPos_ = endPos;
}

inline void StmtDDLCreateTrigger::setStartPosition(const StringPos startPos)
{
  startPos_ = startPos;
}

inline const StringPos StmtDDLCreateTrigger::getEndPosition() const
{
  return endPos_;
}

inline const StringPos StmtDDLCreateTrigger::getStartPosition() const
{
  return startPos_;
}

inline NABoolean
StmtDDLCreateTrigger::isOwnerSpecified() const
{
  return pOwner_ ? TRUE : FALSE;
}

// Not used by Triggers
inline const ElemDDLGrantee *
StmtDDLCreateTrigger::getOwner() const
{
  return pOwner_;
}

#endif // STMTDDLCREATETRIGGER_H




