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
 * File:         ElemDDLConstraint.C
 * Description:  methods for classes representing constraints.
 *
 * Created:      9/21/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "AllElemDDLConstraint.h"
#include "AllElemDDLConstraintAttr.h"
#include "BaseTypes.h"
#include "ComASSERT.h"
#include "ComDiags.h"
#include "ComOperators.h"
#include "ElemDDLColName.h"
#include "ElemDDLRefTrigActions.h"
#include "ElemDDLReferences.h"
#include "ItemExpr.h"
#include "NADefaults.h"
#include "SchemaDB.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraint
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraint::~ElemDDLConstraint()
{
  // delete all children
  for (Int32 index = 0; index < getArity(); index++)
  {
    delete getChild(index);
  }
}

// cast
ElemDDLConstraint *
ElemDDLConstraint::castToElemDDLConstraint()
{
  return this;
}

//
// accessors
//

Int32
ElemDDLConstraint::getArity() const
{
  return MAX_ELEM_DDL_CONSTRAINT_ARITY;
}

ExprNode *
ElemDDLConstraint::getChild(Lng32 index)
{
  ComASSERT(index EQU INDEX_CONSTRAINT_ATTRIBUTES);
  return pConstraintAttributes_;
}

//
// mutators
//

void
ElemDDLConstraint::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index EQU INDEX_CONSTRAINT_ATTRIBUTES);
  if (NOT pChildNode)
  {
    pConstraintAttributes_ = NULL;
  }
  else
  {
    ComASSERT(pChildNode->castToElemDDLNode());
    pConstraintAttributes_ = pChildNode->castToElemDDLNode();
  }
}

void
ElemDDLConstraint::setConstraintAttributes(
     ElemDDLNode * pConstraintAttributes)
{
  setChild(INDEX_CONSTRAINT_ATTRIBUTES, pConstraintAttributes);

  NABoolean hasDroppableConstrSpecified = FALSE;
  if (pConstraintAttributes)
  {
    CollIndex num = pConstraintAttributes->entries();
    for (CollIndex index = 0; ((index < num) && NOT hasDroppableConstrSpecified); index++)
    {
      ElemDDLNode *pCnstrntAttr = (*pConstraintAttributes)[index];
      if (pCnstrntAttr->castToElemDDLConstraintAttrDroppable())
	hasDroppableConstrSpecified = TRUE;
    }
  }
  if (NOT hasDroppableConstrSpecified )  // no constraint attributes specified
  {
    //
    // Currently, there are only two kinds of constraint attributes
    // available: 
    // droppable (or not droppable), and only not null
    // and primary key constraints can have the not-droppable
    // attribute.
    // enforced ( or not enforced), and only RI constraints 
    // can have the not enforced attribute
    //
    // [not] droppable attribute was not specified, use the
    // default settings.
    //
    // Note that the primary key constraint created by the ALTER
    // TABLE <table-name> ADD CONSTRAINT statement specified by
    // the user will always have the DROPPABLE attribute, but
    // there is not enough information available at this point
    // so we still use the default settings if the user does not
    // specify the [ not ] droppable attribute.  This information
    // will be overwritten later when we process the root node
    // of the ALTER TABLE <table-name> ADD CONSTRAINT statement.
    //

    if (isConstraintNotNull())
    {
      setDroppableFlag(
        CmpCommon::getDefault(NOT_NULL_CONSTRAINT_DROPPABLE_OPTION)
	== DF_ON);
    }
    else if (castToElemDDLConstraintPK())
    {
      setDroppableFlag(
        CmpCommon::getDefault(PRIMARY_KEY_CONSTRAINT_DROPPABLE_OPTION)
	== DF_ON);
    }
    else
    {
      // Does nothing.  isDroppable_ was already set to TRUE.
    }
    if (NOT pConstraintAttributes)   
      return;
  }

  CollIndex entries = pConstraintAttributes->entries();
  NABoolean droppableClauseSpec = FALSE;
  NABoolean enforcedClauseSpec = FALSE;

  for (CollIndex index = 0; index < entries; index++)
  {
    //
    // Currently, there is only twos kind of constraint attributes
    // available: (a) droppable (or not droppable), and only not null
    // and primary key constraints can have the not-droppable
    // attribute.
    //(b) enforced (or not enforced), and only RI constraints can have
    // the not enforced attribute
    ElemDDLNode *pCnstrntAttr = (*pConstraintAttributes)[index];
    if (pCnstrntAttr->castToElemDDLConstraintAttrDroppable())
    {
      //
      // The user specified the [ NOT ] DROPPABLE clause explicitly.
      //

      if (droppableClauseSpec)
      {
        // Duplicate [ NOT ] DROPPABLE clauses.
        *SqlParser_Diags << DgSqlCode(-3167);
        return;
      }
      droppableClauseSpec = TRUE;

      if (NOT isConstraintNotNull() AND
          NOT castToElemDDLConstraintPK() AND
	  NOT pCnstrntAttr->
	      castToElemDDLConstraintAttrDroppable()->isDroppable())
      {
	// NOT DROPPABLE clause can only appear in NOT NULL and PRIMARY KEY
	// constraint definitions.
	*SqlParser_Diags << DgSqlCode(-3054); 
	return;
      }

      setDroppableFlag(pCnstrntAttr->
                       castToElemDDLConstraintAttrDroppable()->isDroppable());

      if (isDroppable())  // DROPPABLE clause specified explicitly
      {
        droppableClauseInfo_ = DROPPABLE_SPECIFIED_EXPLICITLY;
      }
      else
      {
        droppableClauseInfo_ = NOT_DROPPABLE_SPECIFIED_EXPLICITLY;
      }
    }
    else if (pCnstrntAttr->castToElemDDLConstraintAttrEnforced())
    {
      //
      // The user specified the [ NOT ] ENFORCED clause explicitly.
      //

      if (enforcedClauseSpec)
      {
        // Duplicate [ NOT ] ENFORCED clauses.
        *SqlParser_Diags << DgSqlCode(-3243);
        return;
      }
      enforcedClauseSpec = TRUE;

      if (NOT castToElemDDLConstraintRI() AND
	  NOT pCnstrntAttr->
	      castToElemDDLConstraintAttrEnforced()->isEnforced())
      {
	// NOT ENFORCED clause can only appear in RI constraints
	// constraint definitions.
	*SqlParser_Diags << DgSqlCode(-3244); 
	return;
      }

      setEnforcedFlag(pCnstrntAttr->
                       castToElemDDLConstraintAttrEnforced()->isEnforced());
    }
    else
    {
      ABORT("internal logic error");
    }
  } // for

} // ElemDDLConstraint::setConstraintAttributes()

void
ElemDDLConstraint::setDroppableFlag(const NABoolean setting)
{
  isDroppable_ = setting;
}

void
ElemDDLConstraint::setEnforcedFlag(const NABoolean setting)
{
  isEnforced_ = setting;
}

//
// methods for tracing
//

const NAString
ElemDDLConstraint::displayLabel1() const
{
  if (getConstraintName().length() NEQ 0)
    return NAString("Constraint name: ") + getConstraintName();
  else
    return NAString("Constraint name not specified.");
}

NATraceList
ElemDDLConstraint::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());   // constraint name
  detailTextList.append(displayLabel2());   // constraint type (e.g., Not Null)

  detailText = "Constraint kind: ";
  if (getConstraintKind() EQU ElemDDLConstraint::COLUMN_CONSTRAINT_DEF)
  {
    detailText += "Column";
  }
  else
  {
    detailText += "Table";
  }
  detailTextList.append(detailText);

  detailText = "is deferrable? ";
  detailText += YesNo(isDeferrable());
  detailTextList.append(detailText);

  detailText = "is droppable?  ";
  detailText += YesNo(isDroppable());
  detailTextList.append(detailText);

  detailText = "is enforced?  ";
  detailText += YesNo(isEnforced());
  detailTextList.append(detailText);

  return detailTextList;

} // ElemDDLConstraint::getDetailInfo()

const NAString
ElemDDLConstraint::getText() const
{
  return "ElemDDLConstraint";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintArray
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintArray::~ElemDDLConstraintArray()
{}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintCheck
// -----------------------------------------------------------------------

// initialize constructor
ElemDDLConstraintCheck::ElemDDLConstraintCheck(ItemExpr *pSearchCondition,
                                               const ParNameLocList &nameLocs,
                                               CollHeap *heap)
: ElemDDLConstraint(heap, ELM_CONSTRAINT_CHECK_ELEM),
  searchCondition_(pSearchCondition),
  nameLocList_(nameLocs, heap),
  startPos_(nameLocs.getTextStartPosition()),
  endPos_(0)  // to be set later
{
}

// virtual destructor
ElemDDLConstraintCheck::~ElemDDLConstraintCheck()
{
}

// cast
ElemDDLConstraintCheck *
ElemDDLConstraintCheck::castToElemDDLConstraintCheck()
{
  return this;
}

// This method should be:				####
//   ElemDDLConstraintNotNull * 
//   ElemDDLConstraintCheck::castToElemDDLConstraintNotNull()
// and
//   class ElemDDLConstraintNotNull shd be derived from ElemDDLConstraintCheck
//
NABoolean
ElemDDLConstraintCheck::isConstraintNotNull() const
{
  return getSearchCondition()->isISNOTNULL();
}
NABoolean
ElemDDLConstraintCheck::getColumnsNotNull(ItemExprList &il)
{
  return getSearchCondition()->getColumnsIfThisIsISNOTNULL(il);
}

//
// accessors
//

Int32
ElemDDLConstraintCheck::getArity() const
{
  return MAX_ELEM_DDL_CONSTRAINT_CHECK_ARITY;
}

ExprNode *
ElemDDLConstraintCheck::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (index < ElemDDLConstraint::getArity())
  {
    return ElemDDLConstraint::getChild(index);
  }
  else
  {
    ComASSERT(index EQU INDEX_SEARCH_CONDITION);
    return searchCondition_;
  }
}

//
// mutator
//

void
ElemDDLConstraintCheck::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (index < ElemDDLConstraint::getArity())
  {
    ElemDDLConstraint::setChild(index, pChildNode);
  }
  else
  {
    ComASSERT(index EQU INDEX_SEARCH_CONDITION);
    if (NOT pChildNode)
    {
      searchCondition_ = NULL;
    }
    else
    {
      ComASSERT(pChildNode->castToItemExpr());
      searchCondition_ = pChildNode->castToItemExpr();
    }
  }
}

//
// methods for tracing
//

const NAString
ElemDDLConstraintCheck::displayLabel2() const
{
  return "Constraint type: Check constraint";
}

const NAString
ElemDDLConstraintCheck::getText() const
{
  return "ElemDDLConstraintCheck";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintName
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintName::~ElemDDLConstraintName()
{
}

// cast virtual function
ElemDDLConstraintName *
ElemDDLConstraintName::castToElemDDLConstraintName()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLConstraintName::getText() const
{
  return "ElemDDLConstraintName";
}

const NAString
ElemDDLConstraintName::displayLabel1() const
{
  return NAString("Constraint name: ") + getConstraintName();
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintNotNull
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintNotNull::~ElemDDLConstraintNotNull()
{
}

// cast
ElemDDLConstraintNotNull *
ElemDDLConstraintNotNull::castToElemDDLConstraintNotNull()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLConstraintNotNull::displayLabel2() const
{
  return "Constraint type: Not Null constraint";
}

const NAString
ElemDDLConstraintNotNull::getText() const
{
  return "ElemDDLConstraintNotNull";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintPK
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintPK::~ElemDDLConstraintPK()
{
}

// cast
ElemDDLConstraintPK *
ElemDDLConstraintPK::castToElemDDLConstraintPK()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLConstraintPK::displayLabel2() const
{
  return "Constraint type: Primary Key constraint";
}

const NAString
ElemDDLConstraintPK::getText() const
{
  return "ElemDDLConstraintPK";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintPKColumn
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintPKColumn::~ElemDDLConstraintPKColumn()
{
}

// cast
ElemDDLConstraintPKColumn *
ElemDDLConstraintPKColumn::castToElemDDLConstraintPKColumn()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLConstraintPKColumn::getText() const
{
  return "ElemDDLConstraintPKColumn";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintRI
// -----------------------------------------------------------------------

// constructor
ElemDDLConstraintRI::ElemDDLConstraintRI(
     ElemDDLNode * pReferencedTableAndColumns,
     ComRCMatchOption matchType,
     ElemDDLNode * pReferentialTriggeredActions,
     CollHeap    * heap)

: ElemDDLConstraint(heap, ELM_CONSTRAINT_REFERENTIAL_INTEGRITY_ELEM),
  matchType_(matchType),
  isDeleteRuleSpec_(FALSE),
  deleteRule_(COM_NO_ACTION_DELETE_RULE),
  isUpdateRuleSpec_(FALSE),
  updateRule_(COM_NO_ACTION_UPDATE_RULE),
  referencingColumnNameArray_(heap),
  referencedColumnNameArray_(heap)
{
  setChild(INDEX_REFERENCING_COLUMN_NAME_LIST, NULL);
  setChild(INDEX_REFERENCED_TABLE_AND_COLUMNS, pReferencedTableAndColumns);
  setChild(INDEX_REFERENTIAL_TRIGGERED_ACTIONS, pReferentialTriggeredActions);

  //
  // Note that at the point this parse node is contructed, the information
  // regarding the referencing column name list (in the FOREIGN KEY phrase)
  // has not been collected yet.
  //

  //
  // referenced table name
  //

  ComASSERT(pReferencedTableAndColumns);
  ElemDDLReferences * pReferences = pReferencedTableAndColumns
        ->castToElemDDLReferences();
  ComASSERT(pReferences);

  //
  // referenced column name list (if any)
  //

  ElemDDLNode * pReferencedColumns = pReferences->getReferencedColumns();
  if (pReferencedColumns)
  {
    for (CollIndex index = 0; index < pReferencedColumns->entries(); index++)
    {
      referencedColumnNameArray_.insert((*pReferencedColumns)[index]
                                        ->castToElemDDLColName());
    }
  }

  //
  // referential triggered actions
  //

  if (pReferentialTriggeredActions)
  {
    for (CollIndex i = 0; i < pReferentialTriggeredActions->entries(); i++)
    {
      ComASSERT((*pReferentialTriggeredActions)[i]);
      setReferentialTriggeredAction((*pReferentialTriggeredActions)[i]
                                    ->castToElemDDLRefTrigAct());
    }
  }

} // ElemDDLConstraintRI::ElemDDLConstraintRI()

// virtual destructor
ElemDDLConstraintRI::~ElemDDLConstraintRI()
{
  //
  // delete all child parse nodes added to this class
  //
  // Note that class ElemDDLConstraintRI is derived from class
  // ElemDDLConstraint.  ~ElemDDLConstraint() deletes all child parse
  // nodes belong to class ElemDDLConstraint.  So the destructor
  // ~ElemDDLConstraintRI() only needs to delete the additional child
  // parse nodes that only belong to class ElemDDLConstraintRI.
  //
  for (Int32 index = ElemDDLConstraint::getArity(); index < getArity(); index++)
  {
    delete getChild(index);
  }
}

// cast
ElemDDLConstraintRI *
ElemDDLConstraintRI::castToElemDDLConstraintRI()
{
  return this;
}

//
// accessors
//

Int32
ElemDDLConstraintRI::getArity() const
{
  return MAX_ELEM_DDL_CONSTRAINT_RI_ARITY;
}

ExprNode *
ElemDDLConstraintRI::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  if (index < ElemDDLConstraint::getArity())
  {
    return ElemDDLConstraint::getChild(index);
  }
  else
  {
    return children_[index];
  }
}

NAString
ElemDDLConstraintRI::getMatchTypeAsNAString() const
{
  switch (getMatchType())
  {
  case COM_NONE_MATCH_OPTION :
    return NAString("Match phrase not specified");
  case COM_FULL_MATCH_OPTION :
    return NAString("MATCH FULL");
  case COM_PARTIAL_MATCH_OPTION :
    return NAString("MATCH PARTIAL");
  default :
    ABORT("internal logic error");
    return NAString();
  }
}

NAString
ElemDDLConstraintRI::getReferencedTableName() const
{
  return getReferencesNode()->
    getReferencedNameAsQualifiedName().getQualifiedNameAsAnsiString();
}

ElemDDLReferences *
ElemDDLConstraintRI::getReferencesNode() const
{
  return ((ElemDDLReferences *)this)->
    getChild(INDEX_REFERENCED_TABLE_AND_COLUMNS)->
    castToElemDDLNode()->castToElemDDLReferences();
}

//
// mutators
//

void
ElemDDLConstraintRI::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (index < ElemDDLConstraint::getArity())
  {
    ElemDDLConstraint::setChild(index, pChildNode);
  }
  else
  {
    if (NOT pChildNode)
    {
      children_[index] = NULL;
    }
    else
    {
      ComASSERT(pChildNode->castToElemDDLNode());
      children_[index] = pChildNode->castToElemDDLNode();
    }
  }
}

void
ElemDDLConstraintRI::setReferencingColumnNameList(
     ElemDDLNode * pReferencingColumnNameList)
{
  setChild(INDEX_REFERENCING_COLUMN_NAME_LIST,
           pReferencingColumnNameList);

  //
  // copies the information to referencingColumnNameArray_
  // so the user can access the information easier.  This
  // method should not be called more than once.
  //

  if (pReferencingColumnNameList)
  {
    for (CollIndex i = 0; i < pReferencingColumnNameList->entries(); i++)
    {
      ComASSERT((*pReferencingColumnNameList)[i] AND
                (*pReferencingColumnNameList)[i]
                ->castToElemDDLColName());
      referencingColumnNameArray_.insert((*pReferencingColumnNameList)[i]
                                         ->castToElemDDLColName());
    }
  }
} // ElemDDLConstraintRI::setReferencingColumnNameList()

void
ElemDDLConstraintRI::setReferentialTriggeredAction(
     ElemDDLRefTrigAct * pRefTrigAct)
{
  ComASSERT(pRefTrigAct);
  if (pRefTrigAct->castToElemDDLRefTrigActDeleteRule())
  {
    if (isDeleteRuleSpec_)
    {
      // Duplicate DELETE rules specified.
      *SqlParser_Diags << DgSqlCode(-3055);
    }
    isDeleteRuleSpec_ = TRUE;
    deleteRule_ = pRefTrigAct->castToElemDDLRefTrigActDeleteRule()
        ->getDeleteRule();
  }
  else if (pRefTrigAct->castToElemDDLRefTrigActUpdateRule())
  {
    if (isUpdateRuleSpec_)
    {
      // Duplicate UPDATE rules specified.
      *SqlParser_Diags << DgSqlCode(-3056);
    }
    isUpdateRuleSpec_ = TRUE;
    updateRule_ = pRefTrigAct->castToElemDDLRefTrigActUpdateRule()
        ->getUpdateRule();
  }
  else 
  {
    ABORT("internal logic error");
  }
}

//
// methods for tracing
//

const NAString
ElemDDLConstraintRI::displayLabel2() const
{
  return "Constraint type: Referential Integrity constraint";
}

NATraceList
ElemDDLConstraintRI::getDetailInfo() const
{
  //
  // constraint name and related information
  //

  NAString        detailText;
  NATraceList detailTextList = ElemDDLConstraint::getDetailInfo();

  //
  // match type
  //

  if (getMatchType() EQU COM_NONE_MATCH_OPTION)
  {
    detailTextList.append("Match type not specified.");
  }
  else
  {
    detailText = "Match type: ";
    detailText += getMatchTypeAsNAString();
    detailTextList.append(detailText);
  }

  //
  // delete rule
  //

  switch (getDeleteRule())
  {
  case COM_UNKNOWN_DELETE_RULE :
    if (isDeleteRuleSpecified())
    {
      ABORT("internal logic error");
    }
    detailTextList.append("Delete Rule not specified.");
    break;

  case COM_CASCADE_DELETE_RULE :
    detailTextList.append("Delete rule: Cascade");
    break;

  case COM_NO_ACTION_DELETE_RULE :
    detailTextList.append("Delete rule: No Action");
    break;

  case COM_SET_DEFAULT_DELETE_RULE :
    detailTextList.append("Delete rule: Set Default");
    break;

  case COM_SET_NULL_DELETE_RULE :
    detailTextList.append("Delete rule: Set Null");
    break;

  default :
    ABORT("internal logic error");
    break;
  }

  //
  // update rule
  //

  switch (getUpdateRule())
  {
  case COM_UNKNOWN_UPDATE_RULE :
    if (isUpdateRuleSpecified())
    {
      ABORT("internal logic error");
    }
    detailTextList.append("Update Rule not specified.");
    break;

  case COM_CASCADE_UPDATE_RULE :
    detailTextList.append("Update rule: Cascade");
    break;

  case COM_NO_ACTION_UPDATE_RULE :
    detailTextList.append("Update rule: No Action");
    break;

  case COM_SET_DEFAULT_UPDATE_RULE :
    detailTextList.append("Update rule: Set Default");
    break;

  case COM_SET_NULL_UPDATE_RULE :
    detailTextList.append("Update rule: Set Null");
    break;

  default :
    ABORT("internal logic error");
    break;
  }

  //
  // referencing information
  //

  CollIndex nbrRefCols;
  const ElemDDLColNameArray & cols = getReferencingColumns();

  nbrRefCols = cols.entries();

  if (nbrRefCols EQU 0)
  {
    detailTextList.append("Referencing column list is empty.");
  }
  else
  {
    detailText = "Referencing column list [";
    detailText += LongToNAString((Lng32)nbrRefCols);
    detailText += " element(s)]:";
    detailTextList.append(detailText);
  
    for (CollIndex i = 0; i < nbrRefCols; i++)
    {
      detailText = "[referencing column ";
      detailText +=  LongToNAString((Lng32)i);
      detailText += "]";
      detailTextList.append(detailText);
  
      detailTextList.append("    ", cols[i]->getDetailInfo());
    }
  }

  //
  // referenced information
  //

  ElemDDLReferences * pRefd = children_[INDEX_REFERENCED_TABLE_AND_COLUMNS]
        ->castToElemDDLReferences();
  
  detailTextList.append(pRefd->getDetailInfo());

  return detailTextList;
}

const NAString
ElemDDLConstraintRI::getText() const
{
  return "ElemDDLConstraintRI";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintUnique
// -----------------------------------------------------------------------

//
// constructors
//

ElemDDLConstraintUnique::ElemDDLConstraintUnique(ElemDDLNode * pColRefList,
                                                 CollHeap * heap)
: ElemDDLConstraint(heap, ELM_CONSTRAINT_UNIQUE_ELEM),
  columnRefList_(pColRefList),
  keyColumnArray_(heap)
{
}

ElemDDLConstraintUnique::ElemDDLConstraintUnique(OperatorTypeEnum operType,
                                                 ElemDDLNode * pColRefList,
                                                 CollHeap * heap)
: ElemDDLConstraint(heap, operType),
  columnRefList_(pColRefList),
  keyColumnArray_(heap)
{
}

//
// virtual destructor
//
ElemDDLConstraintUnique::~ElemDDLConstraintUnique()
{
  //
  // delete all child parse nodes added to this class
  //
  // Note that class ElemDDLConstraintUnique is derived from class
  // ElemDDLConstraint.  ~ElemDDLConstraint() deletes all child parse
  // nodes belong to class ElemDDLConstraint.  So the destructor
  // ~ElemDDLConstraintUnique() only needs to delete the additional
  // child parse nodes that only belong to class ElemDDLConstraintUnique.
  //
  for (Int32 index = ElemDDLConstraint::getArity(); index < getArity(); index++)
  {
    delete getChild(index);
  }
}

// cast
ElemDDLConstraintUnique *
ElemDDLConstraintUnique::castToElemDDLConstraintUnique()
{
  return this;
}

//
// accessors
//

Int32
ElemDDLConstraintUnique::getArity() const
{
  return MAX_ELEM_DDL_CONSTRAINT_UNIQUE_ARITY;
}

ExprNode *
ElemDDLConstraintUnique::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  if (index < ElemDDLConstraint::getArity())
  {
    return ElemDDLConstraint::getChild(index);
  }
  else
  {
    ComASSERT(index EQU INDEX_COLUMN_NAME_LIST);
    return columnRefList_;
  }
}

//
// mutators
//

void
ElemDDLConstraintUnique::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (index < ElemDDLConstraint::getArity())
  {
    ElemDDLConstraint::setChild(index, pChildNode);
  }
  else
  {
    ComASSERT(index EQU INDEX_COLUMN_NAME_LIST);
    if (NOT pChildNode)
    {
      columnRefList_ = NULL;
    }
    else
    {
      ComASSERT(pChildNode->castToElemDDLNode());
      columnRefList_ = pChildNode->castToElemDDLNode();
    }
  }
}

void
ElemDDLConstraintUnique::setColumnRefList(ElemDDLNode * pColumnRefList)
{
  setChild(INDEX_COLUMN_NAME_LIST, pColumnRefList);

  if (getColumnRefList())
  {
    //
    // Copies information about the key column list to keyColumnArray_
    // so the user can access the information easier.  (Assumes that
    // keyColumnArray_ is still empty.)
    //
    for (CollIndex i = 0; i < getColumnRefList()->entries(); i++)
    {
      ComASSERT((*getColumnRefList())[i] AND
                (*getColumnRefList())[i]->castToElemDDLColRef());
      keyColumnArray_.insert((*getColumnRefList())[i]->castToElemDDLColRef());
    }
  }

} // ElemDDLConstraintUnique::setColumnRefList()

//
// methods for tracing
//

const NAString
ElemDDLConstraintUnique::displayLabel2() const
{
  return "Constraint type: Unique constraint";
}

NATraceList
ElemDDLConstraintUnique::getDetailInfo() const
{
  NATraceList detailTextList = ElemDDLConstraint::getDetailInfo();

  if (getKeyColumnArray().entries() EQU 0)
  {
    detailTextList.append("No key column list.");
  }
  else
  {
    NAString detailText;

    detailText = "Key Column List [";
    detailText += LongToNAString((Lng32)getKeyColumnArray().entries());
    detailText += " element(s)]:";
    detailTextList.append(detailText);
    
    for (CollIndex i = 0; i < getKeyColumnArray().entries(); i++)
    {
      detailText = "[column ";
      detailText += LongToNAString((Lng32)i);
      detailText += "]";
      detailTextList.append(detailText);
      
      detailTextList.append("    ", getKeyColumnArray()[i]->getDetailInfo());
    }
  }

  return detailTextList;

} // ElemDDLConstraintUnique::getDetailInfo()

const NAString
ElemDDLConstraintUnique::getText() const
{
  return "ElemDDLConstraintUnique";
}

//
// End of File
//
