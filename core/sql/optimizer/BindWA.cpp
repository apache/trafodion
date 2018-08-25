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
*************************************************************************
*
* File:         BindWA.Cpp
* Description:  The workarea used by the name binder
* Created:      4/27/94
* Language:     C++
*
*
*    A Norwegian named Johan Vaaler should be credited with the invention
*    of the paper clip in 1899.  However, as the story goes, Norway had no
*    patent law at the time, and though Vaaler's drawing was accepted by a
*    special government commission, he had to seek an actual patent in Germany.
*    Norwegians are said to have remembered proudly the humble item's origins
*    in their country when, during WWII, they "fastened paper clips to their
*    jacket lapels to show patriotism and irritate the Germans."  Wearing a
*    paper clip could result in arrest, but the function of the device,
*   "TO BIND TOGETHER," took on the fiercely symbolic meaning of
*    "people joining against the forces of occupation."
*		-- Henry Petroski, _The Evolution of Useful Things_
*
*************************************************************************
*/

#define   SQLPARSERGLOBALS_FLAGS        // should precede all other #include's

#include "Sqlcomp.h"
#include "BindWA.h"
#include "CmpContext.h"
#include "StmtDDLCreateView.h"
#include "RelMisc.h"
#include "ItemOther.h"
#include "RelJoin.h"
#include "ItemSubq.h"
#include "ItemFunc.h"

#include "RelMisc.h"
#include "ItemOther.h"
#include "RelJoin.h"
#include "RelUpdate.h"
#include "MvRefreshBuilder.h"


#define   SQLPARSERGLOBALS_NADEFAULTS
#include "SqlParserGlobalsCmn.h"
#include "SqlParserGlobals.h"           // should be last #include

// ***********************************************************************
// BindScope()
// ***********************************************************************
BindScope::BindScope(BindWA* bindWA) :
  bindWA_(bindWA),
  xtnm_(bindWA ? bindWA->wHeap() : NULL),
  RETDesc_(NULL),
  sequenceNode_(NULL),
  ncToOldMap_(NULL),
  OlapPartitionChange_ (NULL),
  HasOlapSeqFunctions_ ( BindScope::OLAPUNKNOWN_ ),
  isFirstOlapWindowSpec_( TRUE ),
  inViewExpansion_(FALSE)
{
  xtnmStack()->createXTNM();
} // BindScope::BindScope()

// ***********************************************************************
// ~BindScope()
// ***********************************************************************
BindScope::~BindScope()
{
} // BindScope::~BindScope()

// ***********************************************************************
// BindScope::mergeOuterRefs()
// ***********************************************************************
void BindScope::mergeOuterRefs(const ValueIdSet& other, NABoolean keepLocalRefs)
{
  outerRefs_ += other;

  if (!keepLocalRefs)
  {
    // Get the list of valueIds that this scope exposes and remove
    // them from the local references
    ValueIdList localRefList;
    if (RETDesc_)
      RETDesc_->getValueIdList(localRefList,USER_AND_SYSTEM_COLUMNS);
    
    ValueIdSet localRefSet = localRefList;
    outerRefs_ -= localRefSet;
    
    // subtract any other local references
    outerRefs_ -= localRefs_;
  }
} // BindScope::mergeOuterRefs()


// ***********************************************************************
// BindWA()
// ***********************************************************************
BindWA::BindWA(SchemaDB *schemaDB, CmpContext* cmpContext, NABoolean inDDL, NABoolean allowExtTables)
     : schemaDB_(schemaDB)
     , currentCmpContext_(cmpContext)
     , inputVars_(cmpContext ? cmpContext->statementHeap() : NULL)
     , scopes_(cmpContext ? cmpContext->statementHeap() : NULL)
     , stoiList_(cmpContext ? cmpContext->statementHeap() : NULL)
     , udrStoiList_(cmpContext ? cmpContext->statementHeap() : NULL)
     , coProcAggrList_(cmpContext ? cmpContext->statementHeap() : NULL)
     , seqValList_(cmpContext ? cmpContext->statementHeap() : NULL)
     , defaultSchema_(cmpContext ? cmpContext->statementHeap() : NULL)
     , RETDescList_(cmpContext ? cmpContext->statementHeap() : NULL)
     , tableViewUsageList_(cmpContext ? cmpContext->statementHeap() : NULL)
     , inDDL_(inDDL)
  //     , inIndexMaint_(FALSE)
  //     , inRIMaint_(FALSE)
     , inViewWithCheckOption_(NULL)
     , viewCount_(0)
     , errFlag_(FALSE)
     , uniqueNum_(0)
     , uniqueIudNum_(0) //++Triggers,
     , maxIudNum_(0) //++Triggers,
     , isBindingMvRefresh_(FALSE)
     , isPropagateOpAndSyskeyColumns_(FALSE)
     , isExpandMvTree_(FALSE)
     , isBindingOnStatementMv_(FALSE)
     , isBindingIUD_(FALSE)
     , triggersList_(NULL)
     , pNameLocList_(NULL)
     , pUsageParseNode_(NULL)
     , hostArraysArea_(NULL)
     , assignmentStArea_(NULL)
     , topRoot_(NULL)
     , inTrigger_ ( FALSE )
     , spInParams_ (ItemExprList (wHeap()))
     , spOutParams_ (ItemExprList (wHeap()))
     , currOrdinalPosition_ (0)
     , currParamMode_ (COM_UNKNOWN_DIRECTION)
     , bindingCall_ (FALSE)
     , maxResultSets_(0)
     , spHVDPs_ (wHeap ())
     , currSPName_ (NULL)
     , dupWarning_ (FALSE)
     , inGenericUpdate_ (FALSE)
     , renameToScanTable_ (FALSE)
     , inViewExpansion_ (FALSE)
     , inliningInfoFlagsToSetRecursivly_(0)
     , currCSE_(NULL)
     , inCTAS_(FALSE)
     , viewsUsed_("", wHeap())
     , hasDynamicRowsetsInQuery_(FALSE)
     , inReadOnlyQuery_(FALSE)
     , embeddedIUDStatement_(FALSE)
     , insertSelectStatement_(FALSE)
     , mergeStatement_(FALSE)
     , corrNameTokens_("", wHeap())
     , uninitializedMvList_ (NULL)
     , toOverrideSchema_ (TRUE)
     , overrideSchemaEnabled_ (FALSE)
     , routineInvocationNum_ (0)
     , isBindTrueRoot_(FALSE)
     , noNeedToLimitSchemaAccess_(FALSE)
     , holdableType_(SQLCLIDEV_NONHOLDABLE)
     , isFastExtract_(FALSE)
     , failedForPrivileges_(FALSE)
     , shouldLogAccessViolations_(FALSE)
     , queryCanUseSeaMonster_(-1)
     , volatileTableFound_(FALSE)
     , outerAggScope_(NULL)
     , hasCallStmts_(FALSE)
     , isTrafLoadPrep_(FALSE)
     , connectByHasPath_(FALSE)
     , connectByHasIsLeaf_(FALSE)
     , flags_(0)
     , udfList_(wHeap())
{
  // get current default schema, using NAMETYPE NSK or ANSI rules
  defaultSchema_ = schemaDB_->getDefaultSchema(SchemaDB::APPLY_NAMETYPE_RULES);

  // create sentinel scope
  initNewScope();

  hcui_ = new(cmpContext->statementHeap()) HbaseColUsageInfo(cmpContext->statementHeap());

  setAllowExternalTables(allowExtTables);
} // BindWA::BindWA()

// ***********************************************************************
// ~BindWA()
// ***********************************************************************
BindWA::~BindWA()
{
  inputVars_.clearAndDestroy();
  //cerr << "BindWA::~BindWA " << (void*)this << " " << RETDescList_.entries() << endl; // ##
} // BindWA::~BindWA()

// ***********************************************************************
// BindWA::initNewScope()
// ***********************************************************************
void BindWA::initNewScope()
{
  CollIndex i = scopes_.entries();
  BindScope* newScope = new(wHeap()) BindScope(this);
  scopes_.insert(newScope);
  if (i) {
    // newScope->context()->inSubquery() = scopes_[i-1]->context()->subqSeen();
    newScope->context()->inUpdateOrInsert() = scopes_[i-1]->
	      context()->inUpdateOrInsert();
    // do not pass inViewExpansion to subquery, for override_schema, etc.
    if (//overrideSchemaEnabled_ &&  
       (!scopes_[i-1]->context()->inSubquery()))
      newScope->setInViewExpansion(scopes_[i-1]->getInViewExpansion());
  }
}

// ***********************************************************************
// BindWA::getCurrentScope()
// ***********************************************************************
BindScope* BindWA::getCurrentScope() const
{
  return scopes_[scopes_.entries() - 1];  // Return the last item of the list
} // BindWA::getCurrentScope()

// ***********************************************************************
// BindWA::getPreviousScope()
// ***********************************************************************
BindScope* BindWA::getPreviousScope(BindScope *currentScope) const
{
  //
  // Find the index of the current scope in the BindScope list.  Assert that
  // the scope is in the list.  If the scope is not the first, return a pointer
  // to the previous scope.  Otherwise, return a NULL.
  //
  CollIndex i = scopes_.index(currentScope);
  CMPASSERT(i != NULL_COLL_INDEX);
  if (i > 0)
    return scopes_[i - 1];
  return NULL;
} // BindWA::getPreviousScope()

// ***********************************************************************
// BindWA::findNextScopeWithTriggerInfo()
// Starting with the next scope, look for a scope that has trigger 
// information. If the parameter is NULL, start with the current scope.
// ***********************************************************************
BindScope *BindWA::findNextScopeWithTriggerInfo(BindScope *currentScope)
{
  if (currentScope == NULL)
    currentScope = getCurrentScope();
  else
    currentScope = getPreviousScope(currentScope);

  while ((currentScope!=NULL) && 
	 (currentScope->context()->triggerObj() == NULL))
    currentScope = getPreviousScope(currentScope);

  return currentScope;
}

// ***********************************************************************
// MV --
void BindWA::markScopeWithMvBindContext(MvBindContext *mvContext)
{
  getCurrentScope()->context()->getMvBindContext() = mvContext;
}

// ***********************************************************************
const MvBindContext * 
BindWA::getClosestMvBindContext(BindScope *currentScope) const
{
  if (!isBindingMvRefresh() && !isBindingOnStatementMv())
    return NULL;

  if (currentScope == NULL)
    currentScope = getCurrentScope();

  while ((NULL != currentScope) && 
	 (NULL == currentScope->context()->getMvBindContext()))
  {
    currentScope = getPreviousScope(currentScope);
  }
  
  if(NULL != currentScope)
  {
    return currentScope->context()->getMvBindContext();
  }

  return NULL;
} // getClosestMvBindContext
/******************************************************************************
    Method: BindWA::addUninitializedMv

    Description:
        Add uninitialized mvs to this list.  This list is kept for runtime
        to report errors on DML statments made on uninitialized mvs.

    Parameters: 
        const char *physicalName
            - the location of the mv
        
        const char *ansiName
            - the name of the mv
             
    Return: NONE
                         
******************************************************************************/
void 
BindWA::addUninitializedMv( const char *physicalName, const char *ansiName )
{    
    UninitializedMvName * pMvName = new(wHeap())UninitializedMvName;

    // first use?
    if( NULL == uninitializedMvList_ )
    {
        uninitializedMvList_ = new (wHeap()) 
                                UninitializedMvNameList( wHeap(), 1 );
    }    
    
    // add the names to the list
    pMvName->setPhysicalName( physicalName );
    pMvName->setAnsiName( ansiName );
    uninitializedMvList_->insert( pMvName );    
}





// ***********************************************************************
// BindWA::getPreviousScope()
// ***********************************************************************
BindScope* BindWA::getSubqueryScope (BindScope *currentScope) const
{
  // This method returns the BindScope* with which we determine whether
  // we're in a subquery (for the purposes to determining a certain syntax
  // error).  This involves checking the previousScope, and if that's a
  // GroupByAgg (or possibly some other node, in some, as yet unfound bug
  // ...?), then we look ahead one scope further.
  //
  // It is not clear whether we need to recurse (potentially) multiple
  // times.  For now we do, though this may later introduce problems, so
  // the 'while' stmt below can be replaced with an 'if' if necessary.
  //
  // Guaranteed semantics of this function: If getPreviousScope() returns
  // a non-NULL BindScope*, then this method will too.
  //
  // See the comments in bindRowValues(), BindRelExpr.cpp, for a more
  // complete discussion of how/why this method is used.
  //
  BindScope * prev   = NULL ; 
  BindScope * retVal = getPreviousScope (currentScope) ;
  while ( retVal && retVal->context()->lookAboveToDecideSubquery() )
    {
      prev   = retVal ;
      retVal = getPreviousScope (prev) ;
    }

  if   ( retVal ) return retVal ;
  else            return prev ;
} // BindWA::getSubqueryScope()


// ***********************************************************************
// BindWA::removeCurrentScope()
// ***********************************************************************
void BindWA::removeCurrentScope(NABoolean keepLocalRefs)
{
  //
  // Remove the current scope from the BindScope list.  If there is a parent
  // scope, merge the outer references of the old current scope into the new
  // current scope.
  // This is NOT the same as BindScopeList::removeScope(), called by
  // ~BindScopeList() as a safety net against memory leaks.
  //
  BindScope *currScope;
  scopes_.getLast(currScope);
  if (NOT scopes_.isEmpty())
    getCurrentScope()->mergeOuterRefs(currScope->getOuterRefs(),keepLocalRefs);

  delete currScope;
} // BindWA::removeCurrentScope()

// ***********************************************************************
// BindWA::findColumn()
//
// The first method searches for the given name in the BindScopes.
// It starts from the current BindScope.  If the given name is not
// found in the ColumnNameMap associated with a BindScope, the
// search progresses to the previous BindScope.  The search terminates
// either when a ColumnNameMap corresponding to the given name is
// found or the outermost BindScope has been searched unsucessfully.
//
// If the name is found, the method returns a pointer to the xcnmEntry
// and a pointer to the BindScope in which it was found.  Otherwise, it
// returns a NULL and a pointer to the outermost BindScope.
//
// The second findColumn() locates the xcnmEntry to a ColumnDesc
// known to exist in the current scope (no backward searching thru scopes!).
// ***********************************************************************
ColumnNameMap *BindWA::findColumn(const ColRefName &colRefName,
                                  BindScope *&bindScope)
{
  bindScope = getCurrentScope();
  while (bindScope) {
    RETDesc *resultTable = bindScope->getRETDesc();
    if (resultTable) {
      ColumnNameMap *result = resultTable->findColumn(colRefName);
      if (!result) // check if need to try public schema if not found in default schema
      {
        NAString publicSchema = "";
        CmpCommon::getDefault(PUBLIC_SCHEMA_NAME, publicSchema, FALSE);
        ComSchemaName pubSchema(publicSchema);
        ColRefName newColRef = colRefName;
        if ( !pubSchema.getSchemaNamePart().isEmpty() 
          && colRefName.getCorrNameObj().getQualifiedNameObj().getSchemaName().isNull() )
        {
          newColRef.getCorrNameObj().getQualifiedNameObj().setSchemaName(
            pubSchema.getSchemaNamePart().getInternalName());
          if ( !pubSchema.getCatalogNamePart().isEmpty() )
            newColRef.getCorrNameObj().getQualifiedNameObj().setCatalogName(
              pubSchema.getCatalogNamePart().getInternalName());
          result = resultTable->findColumn(newColRef);
        }
      }
      if (result)
        return result;
    }
    bindScope = getPreviousScope(bindScope);
  }
  return NULL;
} // BindWA::findColumn() the first

ColumnNameMap *BindWA::findColumn(const ColumnDesc &columnDesc)
{
  ColumnNameMap *result = NULL;
  RETDesc *resultTable = getCurrentScope()->getRETDesc();
  if (resultTable)
    result = resultTable->findColumn(columnDesc.getColRefNameObj());
  CMPASSERT(result);
  return result;
} // BindWA::findColumn() the second

// ***********************************************************************
// BindWA::findCorrName()
//
// Same search strategy as the first findColumn().
// ***********************************************************************
ColumnDescList *BindWA::findCorrName(const CorrName &corrName,
                                     BindScope *&bindScope)
{
  bindScope = getCurrentScope();
  while (bindScope) {
    RETDesc *resultTable = bindScope->getRETDesc();
    if (resultTable) {
      ColumnDescList *result = resultTable->getQualColumnList(corrName);
      if (result)
        return result;	  // This list may have zero entries (SYSKEY the only
    }			  // column from that table -- see RETDesc::addColumn).
    bindScope = getPreviousScope(bindScope);
  }
  return NULL;
} // BindWA::findCorrName()

// Same search strategy as the first findColumn().
StmtLevelAccessOptions *BindWA::findUserSpecifiedAccessOption()
{
  BindScope *bindScope = getCurrentScope();
  while (bindScope) {
    BindContext *context = bindScope->context();
    if (context) {
      StmtLevelAccessOptions *axOpts = CONST_CAST
        (StmtLevelAccessOptions*,context->stmtLevelAccessOptions());
      if (axOpts && axOpts->userSpecified()) {
        return axOpts;
      }
    }
    bindScope = getPreviousScope(bindScope);
  }
  return NULL;
} // BindWA::findUserSpecifiedAccessOption()

// ***********************************************************************
// BindWA::getTablesInScope() and BindScope::getTablesInScope()
//
// Return a list of TableNameMaps of all tables that are in scope.
// Caller can get the exposed names of the tables and display them,
// or tell this function to do that via the optional NAString parameter.
//
// Same search strategy as the first findColumn().
// ***********************************************************************
void BindWA::getTablesInScope(LIST(TableNameMap*) &xtnmList,
			      NAString *formattedList) const
{
  xtnmList.clear();
  LIST(TableNameMap*) xtnmPerScope(wHeap());
  BindScope *bindScope = getCurrentScope();
  while (bindScope) {
    bindScope->getTablesInScope(xtnmPerScope, NULL);
    bindScope = getPreviousScope(bindScope);
    xtnmList.insert(xtnmPerScope);
  }
  RETDesc::formatTableList(xtnmList, formattedList);
} // BindWA::getTablesInScope()

void BindScope::getTablesInScope(LIST(TableNameMap*) &xtnmList,
			         NAString *formattedList) const
{
  xtnmList.clear();
  RETDesc *resultTable = getRETDesc();
  if (resultTable) resultTable->getTableList(xtnmList, formattedList);
} // BindWA::getTablesInScope()

// BindScope::addUnresolvedAggregate()

// Optionally add this newAggrId to the set of unresolved aggregates.
// It will be added if there is not already an equivalent aggregate in
// the unresolved aggregate set.  The return value is the value id
// representing this aggregate.  This is either the new ValueId or the
// equivalent ValueId found in the set.
//
ValueId BindScope::addUnresolvedAggregate(ValueId newAggrId)
{
  // Assume that there is no other equivalent aggregate.
  //
  ValueId equivId = newAggrId;

  ItemExpr *newItem = newAggrId.getItemExpr();
  Aggregate *newAggr = NULL;
  if(newItem->isAnAggregate()) 
    {
      newAggr = (Aggregate *)newItem;
    }

  // If this is not an aggregate, then skip the search for an equiv.
  //
  if(newAggr) {

    // Check all the existing aggregates for an equivalent
    //
    for(ValueId aggrId = unresolvedAggregates_.init();
        unresolvedAggregates_.next(aggrId);
        unresolvedAggregates_.advance(aggrId)
        )
      {

        ItemExpr *aggr = aggrId.getItemExpr();

        if(newAggr->isEquivalentForBinding(aggr)) {
        
          // Found an equivalent, use this one instead.
          //
          equivId = aggrId;
          if(newAggr->origOpType() != aggr->origOpType()) {
            aggr->setOrigOpType(aggr->getOperatorType());
          }
          break;
        }
      }
  }

  // Add the new (or maybe old) aggregate to the set. If this was an
  // existing aggregate that is equivalent to the new aggregatem this
  // operation is essentially a no-op.
  //
  unresolvedAggregates_ += equivId;

  // Return the equivalent ValueId to the caller so it can be used as
  // a replacement for the new aggrid.
  //
  return equivId;
}

ValueId BindScope::getEquivalentItmSequenceFunction(ValueId newSeqId)
{
  //
  ValueId equivId = newSeqId;

  ItemExpr *newItem = newSeqId.getItemExpr();
  ItmSequenceFunction *newSeq = NULL;
  if(newItem->isASequenceFunction()) 
    {
      newSeq = (ItmSequenceFunction *)newItem;
    }

  //
  if(newSeq) {

    //
    for(ValueId seqId = unresolvedSequenceFunctions_.init();
      unresolvedSequenceFunctions_.next(seqId);
       unresolvedSequenceFunctions_.advance(seqId) )
    {

	ItemExpr *seq = seqId.getItemExpr();

        if(newSeq->isEquivalentForBinding(seq)) {
        
          equivId = seqId;
          if(newSeq->origOpType() != seq->origOpType()) {
            seq->setOrigOpType(seq->getOperatorType());
          }
          break;
        }
    }
  }

  // Add the new (or maybe old) seq to the set. If this was an
  // existing seq that is equivalent to the new  this
  // operation is essentially a no-op.
  //
  unresolvedSequenceFunctions_ += equivId;

  // Return the equivalent ValueId to the caller so it can be used as
  // a replacement for the new aggrid.
  //
  return equivId;
}


// ***********************************************************************
// Get default schema/catalog.
//
// Set default schema/catalog --
// This should be called by the caller of the Binder, and should remain
// constant over the binding/compiling of the query.
// Caller needs to know what type of query (create schema/dynamic/static)
// and pass in the appropriate defaults; autorecompiles must pass in the
// *saved* defaults (saved by Generator).
// So, Compiler needs to keep track of static defaults and interpret
// our 'Declare Schema' stmt; default for 'Decl Sch :hostvar' will be
// the prototype value if given, and the previous static defaults otherwise.
// Executor needs to keep track of *both* the static defaults and the
// dynamic ones (Ansi 'Set Schema' stmts); in particular, 'Decl Sch :hostvar'
// is executed, which means the string in :hv is parsed and saved in the
// Executor context for static defaults.  Both sets of defaults are sent
// to Compiler when a compilation is needed.  Executor passes its current
// static defaults to do a similarity-check on each static stmt it is about
// to execute (and of course it opens/accesses the correct table based on
// the tablename with current default applied).
// ***********************************************************************
const SchemaName &BindWA::getDefaultSchema() const
{
  // Return current default schema, using NAMETYPE NSK or ANSI rules.
  //
  // *** BindWA::getDefaultSchema() differs from SchemaDB::getDefaultSchema().
  // *** The latter by default (not passing in any flags)
  // *** returns the ANSI schema only.
  //
  return defaultSchema_;
}

void BindWA::setDefaultSchema(const SchemaName& defaultSchema)
{
  defaultSchema_ = defaultSchema;
}

// ***********************************************************************
// BindWA::getCreateViewParseNode()
// ***********************************************************************
StmtDDLCreateView * BindWA::getCreateViewParseNode() const
{
  if (getUsageParseNodePtr())
  {
    return getUsageParseNodePtr()->castToElemDDLNode()->
                                   castToStmtDDLCreateView();
  }
  return NULL;
}

// ***********************************************************************
// BindWA::inViewDefinition()
// BindWA::inMVDefinition()
// BindWA::inCheckConstraintDefinition()
// ***********************************************************************

NABoolean BindWA::inCheckConstraintDefinition() const
{
  return getUsageParseNodePtr() &&
    getUsageParseNodePtr()->getOperatorType() == DDL_ALTER_TABLE_ADD_CONSTRAINT_CHECK;
}

NABoolean BindWA::inViewDefinition() const
{
  return getUsageParseNodePtr() &&
    getUsageParseNodePtr()->getOperatorType() == DDL_CREATE_VIEW;
}

NABoolean BindWA::inMVDefinition() const
{
  return getUsageParseNodePtr() &&
    getUsageParseNodePtr()->getOperatorType() == DDL_CREATE_MV;
}

// ***********************************************************************
// Display/print, for debugging.
// ***********************************************************************
void BindWA::display() const { print(); }

void BindWA::print(FILE* ofd, const char* indent, const char* title) const
{
} // BindWA::print()

//============================================================================
//======================  class MvBindContext  ===============================
//============================================================================

MvBindContext::~MvBindContext()
{
  delete builder_;
}

void MvBindContext::setReplacementFor(const QualifiedName *tableName, 
				      RelExpr             *replacementTree)
{ 
  //replacementTreeHash_[tableName] = replacementTree;
  replacementTreeHash_.remove(tableName);
  replacementTreeHash_.insert(tableName, replacementTree);
}

RelExpr *MvBindContext::getReplacementFor(const QualifiedName& tableName) const
{ 
  return replacementTreeHash_.getFirstValue(&tableName); 
}


//============================================================================
//======================  class HostArraysWA   ===============================
//============================================================================

ItemExpr *BindWA::getHVorDPFromSPDups (ItemExpr *h)
{
  CollIndex numEntries = spHVDPs_.entries();

  const NAString &name1 = (h->getOperatorType() == ITM_HOSTVAR) ?
    ((HostVar *)h)->getName() : ((DynamicParam *)h)->getName();

  // The param mode in the BindWA is correct one for the current ItemExpr
  ComColumnDirection mode1 = this->getCurrParamMode();

  for (CollIndex i = 0; i < numEntries; i++)
  {
    ItemExpr *expr = spHVDPs_[i];

    const NAString &name2 = (expr->getOperatorType() == ITM_HOSTVAR) ?
      ((HostVar *) expr)->getName() : ((DynamicParam *) expr)->getName();

    ComColumnDirection mode2 = (expr->getOperatorType() == ITM_HOSTVAR) ?
      ((HostVar *) expr)->getParamMode() :
      ((DynamicParam *) expr)->getParamMode();

    if (name1.compareTo(name2) == 0 && mode1 == mode2)
      return expr;
  }
  return NULL;
}

NABoolean BindWA::checkHVorDPinSPDups (ItemExpr *h)
{
  if ( NULL == getHVorDPFromSPDups (h))
    return FALSE;

  return TRUE;
}

NABoolean BindWA::checkMultiOutSPParams (ItemExpr *h)
{
  ComColumnDirection mode = (ITM_HOSTVAR == h->getOperatorType()) ?
    ((HostVar *)h)->getParamMode() : ((DynamicParam *)h)->getParamMode();
  
  if (COM_INPUT_COLUMN == mode)
    return FALSE;
  
  CollIndex numEntries = spOutParams_.entries();

  for (CollIndex i = 0; i < numEntries; i++)
  {
    const NAString &s1 = (ITM_HOSTVAR == h->getOperatorType()) ?
      ((HostVar *)h)->getName() : ((DynamicParam *)h)->getName();
    const NAString &s2 = (ITM_HOSTVAR == spOutParams_[i]->getOperatorType()) ?
      ((HostVar *)spOutParams_[i])->getName() :
      ((DynamicParam *)spOutParams_[i])->getName();

    if ((s1.compareTo(s2) == 0) && (s1.compareTo("") != 0))
      return TRUE;
  }

  return FALSE;
}

void BindWA::setColumnRefsInStoi(const char* fileName, Lng32 colPosition)
{

  if (getCurrentScope()->context()->inAnyConstraint()) return;

  if (
      (!isBindingMvRefresh()) &&
      ((!inViewExpansion()) ||
       (inViewExpansion() && getCurrentScope()->context()->inWhereClause())))
  {
    // mark column as referenced for select access in stoi.
    OptSqlTableOpenInfo* stoiInList = NULL;
    for (CollIndex ii=0; ii < getStoiList().entries(); ii++)
    {
      if (strcmp((getStoiList())[ii]->getStoi()->fileName(), 
                  fileName) == 0) 
      {
        stoiInList = getStoiList()[ii];
        break;
      }
    }
    if (stoiInList)
    {
      stoiInList->getStoi()->setSelectAccess();
      stoiInList->addSelectColumn(colPosition);
    }
  }
}

// Scans list of expressions in a VALUES node and replaces all host vars found with
// the names used in the RENAME node
void HostArraysWA::collectArrays(ItemExpr *parent)
{

  ItemExpr *ptr = parent;

  while (ptr) {

    ItemExpr *leftChild;

    if (ptr->getOperatorType() != ITM_ITEM_LIST) {
      leftChild = ptr;
    }
    else {
      leftChild = ptr->child(0);
    }

   // We create a temporary constant to serve as a dummy tree root
    ItemExpr *parent = new (CmpCommon::statementHeap()) SystemLiteral();
    parent->setChild(0,leftChild);

    // Traverse this list element. Find all rowset host arrays and replace them
    // with appropriate scalar variables
    collectHostVarsInPred(parent, 0);

    leftChild = parent->child(0);

    // Add list element (now that we have processed it) to the list
    // pointed by newItemsList_
    if (!lastItemList_) {   
       newItemsList_ = new (bindWA_->wHeap()) ItemList(leftChild, NULL);
       lastItemList_ = newItemsList_;
    }
    else {
      lastItemList_->child(1) = new (bindWA_->wHeap()) ItemList(leftChild, NULL);
      lastItemList_ = lastItemList_->child(1);
    }

    // Move to the next list element
    if (ptr->getOperatorType() == ITM_ITEM_LIST) {
      ptr = ptr->child(1);
    }
    else {
      break;
    }
  }
  if (hasHostArrays()) {
    setHasHostArraysInTuple(TRUE);
  }

}

// Scans list of expressions in a VALUES node and replaces all host vars found with
// the names used in the RENAME node. Replaces node with a ROOT, RENAME and ROWSET nodes.
RelExpr * HostArraysWA::modifyTupleNode(RelExpr *node)
{
  // listofHostArrays for insert has been collected prior to bind phase, by xformRowsetsinTree()
  if (hasHostArrays()) {

    if (getRowwiseRowset()) {
      // arrays cannot be specified with rowwise rowset.
      *CmpCommon::diags() << DgSqlCode(-30008);
      bindWA_->setErrStatus();
      return NULL;
    }
    
    // Create new Rowset node
    RelExpr *newRowSet = new (bindWA_->wHeap()) Rowset(listOfHostArrays_, indexExpr_, 
						       inputSizeExpr_);
    
    // Create list of scalar variables    
    createNewNames();
    
    RelExpr *newRename = new (bindWA_->wHeap()) RenameTable(newRowSet, "Rowset___", newNames_);
    
    RelExpr *newRoot = new (bindWA_->wHeap()) RelRoot(newRename, REL_ROOT, newItemsList_);
    
    if (hasHostArraysInTuple()) {
      ((RelRoot *) newRoot)->setDontOpenNewScope() ;
    }
    
    return newRoot->bindNode(bindWA_);
    
  }
  else if (getRowwiseRowset()) {
    if (node->getOperatorType() != REL_TUPLE)
      {
	bindWA_->setErrStatus();
	
	return NULL;
      }

    Tuple * tuple = (Tuple*)node;

    // Mark all params in the tuple to be part of row which will be
    // moved from the rowwise rowset buffer.
    ItemExprList tList(tuple->tupleExprTree(), bindWA_->wHeap());
    
    for (CollIndex i = 0; (i < (CollIndex) tList.entries());i++)
      {
	ItemExpr * tupleVal = 
	  ((ItemExpr *) tList[i])->castToItemExpr();
	if (tupleVal->getOperatorType() == ITM_DYN_PARAM)
	  {
	    // mark this param that it is part of the row in the rowset.
	    ((DynamicParam*)tupleVal)->setRowInRowwiseRowset();
	  }
      }

    // Create new Rowset node
    RelExpr *newRowSet = 
      new (bindWA_->wHeap()) RowsetRowwise(node);
    
    return newRowSet->bindNode(bindWA_);
  }

  return NULL;
}
  

// Given a node in an ItemExpr whose root is parent, this function traverses
// the expression and stores in listOfHostArrays_ all host arrays found;
// it also replaces the array with a new variable that will appear 
// in a Rename node.
void HostArraysWA::collectHostVarsInPred(ItemExpr *parent, Int32 childNumber)
{
  if (!parent || !parent->child(childNumber)) {
    return;
  }

  Subquery *tempSubquery;
  ItemExpr *op = parent->child(childNumber);
  ItemExpr *tempExpr;

  switch (op->getOperatorType()) {

    case REL_ROOT:
      {
	collectHostVarsInRelExprTree((RelRoot *) (ItemExpr *) op, RelExpr::UNSPECIFIED_);
	if (bindWA_->errStatus())
	  return;
      }
      break;

    // At this point we store the host variable in listOfHostArrays_ if
    // it happens to be a rowset variable. If it is a dynamic parameter, we
    // will treat it as a host variable
    case ITM_DYN_PARAM:
    case ITM_HOSTVAR:
      processArrayHostVar(parent, childNumber);
      break;

    case ITM_CASE:
      // Case statement. It has an operand which is not one of its children,
      // so we have to process it explicitly...

      // so we have to process it by hand...
      // to process it explicitly...
      tempExpr = new (CmpCommon::statementHeap()) SystemLiteral();
      tempExpr->setChild(0,((Case *) op)->getCaseOperand());

      // Process the operand
      collectHostVarsInPred(tempExpr, 0);

      // Reinstall case operand, now that we have replaced any rowset host var
      // that could have been there
      ((Case *) op)->setCaseOperand(tempExpr->child(0));

      break;

    default:
      if (op->isASubquery()) {
	HostArraysWA *tempWA = bindWA_->getHostArraysArea();
	HostArraysWA *subQueryHostArraysWA = new (bindWA_->wHeap()) HostArraysWA(bindWA_);
	subQueryHostArraysWA->inputSizeExpr_ = inputSizeExpr_ ; //subquery inherits outer query's 
	                                                        // ROWSET FOR INPUT SIZE clause.

        // We create a new environment to process a subquery
        bindWA_->setHostArraysArea(subQueryHostArraysWA);
        tempSubquery = (Subquery *) (ItemExpr *) op;
        tempSubquery->getSubquery()->xformRowsetsInTree(*bindWA_);

	// Restore previous environment
        bindWA_->setHostArraysArea(tempWA);
      }
  }

  Lng32 nc = op->getArity();

  for (Lng32 j = 0; j < nc; j++) {
    collectHostVarsInPred(op, j);
  } 

}
// Searches in list for a host variable whose name is in *inputVar. 
// If it finds it, replaces *inputVar with the host variable found
NABoolean HostArraysWA::findHostVar(ItemExpr **inputVar, ItemExpr *list)
{     
  ItemExpr *ptr = list;
  ItemExpr *child;
  HostVar *hostVar = (HostVar *) *inputVar;

  while (ptr) {    
		 
   if (ptr->getOperatorType() != ITM_ITEM_LIST) {
      child = ptr;
   }
   else {
     child = ptr->child(0);
   }
 
   HostVar * host = (HostVar *) child;

   // Found it
   if (host->getName() == hostVar->getName()) {
	 *inputVar = child;
	 return TRUE;
   }
   
   if (ptr->getOperatorType() == ITM_ITEM_LIST) {
     ptr = ptr->child(1);
   }
   else
     break;
 }

 *inputVar = NULL;
 return FALSE;
}

// We have found an array host variable in the parse tree.
// We store it and replace it with a name that will be used in the rename node.
void HostArraysWA::processArrayHostVar(ItemExpr *parent, Int32 childNumber)
{
  ItemExpr *child = parent->child(childNumber);
  HostVar *node;

  if (child->getOperatorType() == ITM_DYN_PARAM) {
    ULng32 inputArrayMaxSize;
    if (((inputArrayMaxSize = getInputArrayMaxSize()) > 0) 
	|| (((DynamicParam *)child)->getRowsetSize() > 0)) {   
     setHasDynamicRowsets(TRUE) ; // dynamic rowsets from either ODBC (first cond.)
                                  // (second cond.) or embedded SQL
     bindWA_->setHasDynamicRowsetsInQuery(TRUE) ;  // setting a flag that is global in scope

     DynamicParam *param = (DynamicParam *) child;
     ULng32 size ;
       if (inputArrayMaxSize > 0) {
	 size = inputArrayMaxSize;
	 param->setRowsetSize(size); // for ODBC queries set rowsetsize_ attribute. 
	                             // Used in Relroot bindnode.
       }
       else {
	 size = param->getRowsetSize() ;
       }
      // This is an dynamic query.
      // We do not know yet what the type of the individual rowset elements will be. This
      // will be determined on a later binding stage. In the case in which we are doing 
      // an INSERT, for instance, the types will be determined in the Insert node
      SQLRowset *rowsetType = 
        new (bindWA_->wHeap()) SQLRowset(bindWA_->wHeap(), (new (bindWA_->wHeap()) SQLUnknown(bindWA_->wHeap())),
                                          size, size);
      NAString name = param->getName();
      if (name.isNull()) {
        name = "__array" + bindWA_->fabricateUniqueName();
      }
  
      if (!(param->getIndicatorName().isNull())) {
        node = new (bindWA_->wHeap()) HostVar(name, param->getIndicatorName(),
  	                                      rowsetType);
      }
      else {
        node = new (bindWA_->wHeap()) HostVar(name, rowsetType);
      }
    }
    else {
      return;
    }
  }
  else {        
    // Make sure it is a Rowset host var
    node = (HostVar *) (ItemExpr *) parent->child(childNumber);
    if (node->getType()->getTypeQualifier() != NA_ROWSET_TYPE) {
      return;
    }
  }

  ItemExpr *ptrList = listOfHostArrays_;

  Int32 found = FALSE;
  Int32 i = 0;
  numHostArrays_++;  

  // Traverse the list of name we already have stored to see if our
  // new HostVar is already there
  while (!found && ptrList) {
    HostVar *tmp = (HostVar *) (ItemExpr *) (ptrList->child(0));
    found = (tmp->getName() == node->getName());
    if (!found) {
      ptrList = ptrList->child(1);
      i++;
    }
  }

  // The rowset host variable is not in listOfHostArrays_. We append it.
  if (!found) {

    if (!lastHostArray_) {
      listOfHostArrays_ = new (bindWA_->wHeap()) ItemList(node, NULL);
      lastHostArray_ = listOfHostArrays_;
    }
    else {
      lastHostArray_->child(1) = new (bindWA_->wHeap()) ItemList(node, NULL);
      lastHostArray_ = lastHostArray_->child(1);
    }
  }
    
  // We create the new scalar variable and replace the rowset host var with it.
  // The name of the scalar variable is an "x" followed by its position number
  // listHostArrays_ 
  // We create the new scalar variable and replace the rowset host var with it.
  // The name of the scalar variable is an "x" followed by its position number
  // listHostArrays_ 
  char tmp1[100], tmp2[100];
#ifdef NA_ITOA_NOT_SUPPORTED
  sprintf(tmp1, "%d",i);
#else
  itoa(i, tmp1, 10);
#endif // NA_ITOA_NOT_SUPPORTED
  strcpy(tmp2,"x");

  ItemExpr *ptr = new (bindWA_->wHeap()) ColReference(new (bindWA_->wHeap()) 
                             ColRefName(strcat(tmp2,tmp1), bindWA_->wHeap()));
  parent->setChild(childNumber,ptr);
}


void HostArraysWA::processKeyVar(ItemExpr *parent, Int32 childNumber)
{
} 

// Traverses listOfHostArrays_ and creates a list of mapping variable names 
// that will be used in the Rename node. The list of names is pointed by newNames_
void HostArraysWA::createNewNames()
{
  ItemExpr *tmp = listOfHostArrays_;  

  char tmp1[100], tmp2[100];

  CollHeap *heap = bindWA_->wHeap();

  for (Int32 i = 0; tmp; i++, tmp = tmp->child(1)) {
#ifdef NA_ITOA_NOT_SUPPORTED
    sprintf(tmp1, "%d",i);
#else
    itoa(i, tmp1, 10);   
#endif // NA_ITOA_NOT_SUPPORTED
    strcpy(tmp2,"x");    
    
    ItemExpr *tempExpr = new (heap)
                         ItemList(new (heap) RenameCol(NULL, new (heap) 
                                  ColRefName(strcat(tmp2,tmp1),heap)), NULL);

    if (!lastName_) {   
       newNames_ = tempExpr;
       lastName_ = newNames_;
    }
    else {
      lastName_->child(1) = tempExpr;
      lastName_ = lastName_->child(1);
    }

  }

  // There is a ROWSET FOR KEY BY <var> statement. We add <var> to the list of
  // variables of the RenameTable node.
  if (indexExpr_) {

    ColReference *ref = (ColReference *) indexExpr_;
    ColRefName name1  = ref->getColRefNameObj();
    ColRefName *name2 = new (heap) ColRefName(name1,heap);
    ItemExpr *tempExpr = new (heap)
                         ItemList(new (heap) RenameCol(NULL, name2), NULL); 

    newTable_ = "Rowset___";
    CMPASSERT(lastName_);
    lastName_->child(1) = tempExpr;
    lastName_ = lastName_->child(1);
  }
}

// Collects all rowset host variables found in the tree rooted
// by root.
void HostArraysWA::collectHostVarsInRelExprTree(RelExpr *root, RelExpr::AtomicityType atomicity)
{
  if (!root) {
    return;
  }

  NABoolean doneWithChildren = FALSE;

  ItemExpr *selectList = root->selPredTree();
  ItemExpr *parent = new (CmpCommon::statementHeap()) SystemLiteral();


  if (selectList) {    
    parent->setChild(0,selectList);
    Lng32 savedNumHostArrays = numHostArrays_ ;
    collectHostVarsInPred(parent, 0);
    if ( numHostArrays_ > savedNumHostArrays) {
      setHasHostArraysInWhereClause(TRUE);
      // have where clause and we are not in delete or upadate for direct rowsets.
      // therefore we are in a select.
      if (hasInputRowsetsInSelectPredicate() == HostArraysWA::UNKNOWN_)  
	setHasInputRowsetsInSelectPredicate(HostArraysWA::YES_);
      }
  }

  // Case when we have a subquery
  if (root->getOperatorType() == REL_ROOT) {
    if (((RelRoot *) root)->getCompExprTree()) {
      selectList = ((RelRoot *) root)->getCompExprTree();

      // We create a temporary constant to serve as a dummy tree root
      parent->setChild(0,selectList);

      collectHostVarsInPred(parent, 0); 
      ((RelRoot *) root)->removeCompExprTree();
      ((RelRoot *) root)->addCompExprTree(parent->child(0));

    }
  }

  if (root->getOperatorType() == REL_UNARY_DELETE) {
    setHasInputRowsetsInSelectPredicate(HostArraysWA::NO_);
  }


  // When we have an UPDATE node, we may have rowset arrays in
  // the SET clause.
  if (root->getOperatorType() == REL_UNARY_UPDATE) {

    Update *node = (Update *) root;
    
    // For regular update, the SET clause comes first in the query before
    // the WHERE predicate.
    // For merge/upsert stmt, the ON clause is specified before the SET
    // or INSERT clauses. 
    // For merge/upsert stmt, process child first. This will
    // get the params that were specified in the ON clause of merge stmt
    // before processing SET or INSERT clauses.
    // This ON clause is part of the scan node which is update's child.
    if (node->isMerge()) {
      // Process the children of this node
      for (Int32 i = 0; i < root->getArity(); i++) {
	collectHostVarsInRelExprTree(root->child(i), atomicity);
	if (bindWA_->errStatus())
	  return;
      }
      
      doneWithChildren = TRUE;
    }
    
    ItemExpr *setClause = node->recExprTree();
    setHasInputRowsetsInSelectPredicate(HostArraysWA::NO_);

    if (setClause) {

      // We create a temporary constant to serve as a dummy tree root
      parent->setChild(0,setClause);

      Lng32 savedNumHostArrays = numHostArrays_ ;
      // Traverse the SET clause. Find and replace any rowset host 
      // variables found there
      collectHostVarsInPred(parent, 0);
      if ( numHostArrays_ > savedNumHostArrays ) {
	setHasHostArraysInSetClause(TRUE);
      }
    }

    if (node->isMerge()) {
      // process INSERT VALUES clause
      ItemExpr * insertClause = 
	(node->isMergeUpdate() ? ((MergeUpdate*)node)->insertValues()
	 : ((MergeDelete*)node)->insertValues());
      
      if (insertClause) {
	
	// We create a temporary constant to serve as a dummy tree root
	parent->setChild(0,insertClause);
	
 Lng32 savedNumHostArrays = numHostArrays_ ;
	// Traverse the INSERT clause. Find and replace any rowset host 
	// variables found there
	collectHostVarsInPred(parent, 0);
      }
    }
    
  }

  // indicate the presence of derived rowsets in the query
  if ((root->getOperatorType() == REL_ROWSET) ||
      (root->getOperatorType() == REL_ROWSET_INTO)) 
  {
     setHasDerivedRowsets(TRUE);
  }

  // If we have a UNION node then we treat its children as subqueries, in 
  // the sense that any arrays found in them act independently of the rest 
  // of the relational tree
  if ((root->getOperatorType() == REL_UNION) && 
      !((Union *) root)->getUnionForIF()) {

    HostArraysWA *tempWA = bindWA_->getHostArraysArea();

    // We create a new environment to process each child
    bindWA_->setHostArraysArea(new (bindWA_->wHeap()) HostArraysWA(bindWA_));
    root->child(0)->xformRowsetsInTree(*bindWA_);

    bindWA_->setHostArraysArea(new (bindWA_->wHeap()) HostArraysWA(bindWA_));
    root->child(1)->xformRowsetsInTree(*bindWA_);

    // Restore previous environment
    bindWA_->setHostArraysArea(tempWA);
    return;
  }

  if (root->getOperatorType() == REL_UNARY_INSERT) 
  {
    if ((CmpCommon::getDefault(ODBC_PROCESS) == DF_ON) ||
	(CmpCommon::getDefault(JDBC_PROCESS) == DF_ON)) 
    {
      // disabling this check
      /*if (root->getTolerateNonFatalError() != RelExpr::UNSPECIFIED_) 
       {
	  // ATOMIC/NOT ATOMIC clause not supported from ODBC and JDBC
	  *CmpCommon::diags() << DgSqlCode(-30030);
	  bindWA_->setErrStatus();
	  return;
       }*/

      //transfer atomicity setting from statement attribute to Insert RelExpr
      //if the Insert node doesn't already have atomicity set.
      if (root->getTolerateNonFatalError() == RelExpr::UNSPECIFIED_) 
	root->setTolerateNonFatalError(atomicity);
    }
  }

  if (root->getOperatorType() == REL_TUPLE) {
    // We replace all arrays found with the scalar variable names in the Rename node
    collectArrays(((Tuple *) root)->tupleExprTree());
  }

  if (NOT doneWithChildren) {
    // Process the children of this node
    for (Int32 i = 0; i < root->getArity(); i++) {
      collectHostVarsInRelExprTree(root->child(i), atomicity);
      if (bindWA_->errStatus())
	return;
    }
  }
  }
  
// Used by Rowsets. Traverses the relational expression pointed to by
// queryExpr. It finds all rowsets that ocurr in selection or select
// lists and replaces them with scalar variables. It also introduces
// a join operator so that one of its operands will be an unPack node
// that will extract all elements in the rowsets, and the other operand 
// is the subtree pointed by the child of queryexpr. The net result is
// that the original query gets executed as many times as there are 
// elements in the array.
RelExpr *HostArraysWA::modifyTree(RelExpr *queryExpr, RelExpr::AtomicityType atomicity)
{
  RelExpr *tempTree = queryExpr;

  if (queryExpr->child(0)->getOperatorType() == REL_ROWSETFOR) {
    RowsetFor *tempVar = (RowsetFor *) (RelExpr *) queryExpr->child(0);
    inputSizeExpr_   = tempVar->getInputSize();
    outputSizeExpr_  = tempVar->getOutputSize();
    indexExpr_       = tempVar->getIndexExpr();
    rwrsMaxSize_     = tempVar->getMaxSizeExpr();
    rwrsMaxInputRowlen_  = tempVar->getMaxInputRowlen();
    rwrsBuffer_      = tempVar->getRwrsBuffer();
    partnNum_        = tempVar->partnNum();
    setRowwiseRowset(tempVar->rowwiseRowset());
    tempVar->getBufferAttributes(packedFormat_,
				 compressed_, dcompressInMaster_,
				 compressInMaster_, partnNumInBuffer_);
    queryExpr->child(0) = queryExpr->child(0)->child(0);
  }

  // Put into listOfHostArrays_ all rowset host variables found
  collectHostVarsInRelExprTree(queryExpr, atomicity);
  if (bindWA_->errStatus())
      return queryExpr;

  // For insert tree transformation is done in modifyTupleNode(), not here.
  if ((listOfHostArrays_) && (!hasHostArraysInTuple())) {

    if ((queryExpr->child(0)->getOperatorType() == REL_UNARY_INSERT) &&
        (queryExpr->child(0)->child(0)->getOperatorType() == REL_ROOT)) {
      tempTree = queryExpr->child(0)->child(0);
    }

    if ((queryExpr->child(0)->getOperatorType() == REL_ROWSET_INTO) &&
        (queryExpr->child(0)->child(0)->getOperatorType() == REL_ROOT)) {
      tempTree = queryExpr->child(0)->child(0);
    }

    // Create new Rowset node with information collected so far
    RelExpr *newRowSet = new (bindWA_->wHeap()) Rowset
      (listOfHostArrays_, indexExpr_, inputSizeExpr_);

    // Create the new names for the arrays so they can be referenced as scalars
    createNewNames();

    // The rename node will map the arrays to their new names
    RelExpr *newRename = new (bindWA_->wHeap()) RenameTable
      (newRowSet, "Rowset___", newNames_);

    // Create a root node below the Join node. It will have the same access type
    // and lock mode as the root of the tree, so that the scans, deletes and inserts
    // used in rowsets inherit that
    RelExpr *newRoot1 = new (bindWA_->wHeap()) RelRoot
      (tempTree->child(0), 
      ((RelRoot *) queryExpr)->accessOptions().accessType(), 
      ((RelRoot *) queryExpr)->accessOptions().lockMode(), 
      REL_ROOT, NULL, NULL, NULL);

    // The new subroot will have the aggregates of the original one, as well as the
    // specification for ordering. In this way, a query that contains an array
    // in the WHERE clause and some aggregates in the select list, will compute
    // those aggregates the number of times that corresponds to the number of 
    // elements in the array, i.e.
    // select sum(a) into :array from t where t1 = :array will return many sums,
    // one for each array element

    ((RelRoot *) newRoot1)->removeCompExprTree();
    ((RelRoot *) newRoot1)->addCompExprTree(((RelRoot *) tempTree)->removeCompExprTree());

    // lac:
    // Fix for defect 10-010522-2978
    // Move the order by tree to the new RelRoot (i.e. the new Subroot).
    // Save a pointer to the upper level RelRoot in the new RelRoot
    // Leave order by tree processing until binding is done
    // in RelRoot::bindNode(); in that code the new RelRoot handles
    // moving the properly bound reqdOrder_ to the upper level RelRoot.

    ((RelRoot *) newRoot1)->addOrderByTree(
                             ((RelRoot *) tempTree)->removeOrderByTree());
    ((RelRoot *) newRoot1)->setParentForRowsetReqdOrder((RelRoot *)tempTree);

    // End Fix for defect 10-010522-2978

    // This join node will send tuples from the rowset node to the original 
    // query (which now contains the arrays names changed to scalars)
    RelExpr *newJoin = new (bindWA_->wHeap()) Join
      (newRename, newRoot1, REL_TSJ, NULL);
    
    if (!getHasDerivedRowsets()) {
      // this flag is being set here so that it can be read in OptPhysRelExpr.cpp
      // and in BindRelExpr.cpp, to indicate that this is a flow node for rowset unpack. 
      // This setting does not affect the rownumber functionality and is not used in the generator.
      newJoin->setRowsetIterator(TRUE);
    }
   
    tempTree->child(0) = newJoin;
    ((RelRoot *) tempTree)->removeCompExprTree();

  }


  if ((atomicity != RelExpr::UNSPECIFIED_) && (!hasHostArraysInTuple()))
   {
      // ATOMIC/NOT ATOMIC statement attribute set for statement that is not an insert
      *CmpCommon::diags() << DgSqlCode(-30025);
      bindWA_->setErrStatus();
   }

  if ((getRowwiseRowset()) &&
      (atomicity == RelExpr::NOT_ATOMIC_))
    {
      // NAR not yet supported for RWRS inserts
      *CmpCommon::diags() << DgSqlCode(-30025);
      bindWA_->setErrStatus();
    }
  
  return queryExpr;
}


// ***********************************************************************
// Add trigger Id's to the triggersList_, only if that trigger was not added
// alreday. Used for the enable/disable mechanism.
// ***********************************************************************
CollIndex 
BindWA::addTrigger(const ComTimestamp& triggerId)
{
	// first use?
	if (triggersList_ == NULL)
		triggersList_ = new (wHeap()) LIST(ComTimestamp)(wHeap(),1);

	CollIndex triggerIndex = triggersList_->index(triggerId);

	// exists already?
	if (triggerIndex == NULL_COLL_INDEX) 
	{
		triggersList_->insert(triggerId);
		triggerIndex = triggersList_->entries() - 1; 
		// index of 1st trigger is 0
	}

	return triggerIndex; 
}

// apply xformRowsetsInTree to our descendants & return transformed tree
RelExpr *RelExpr::xformRowsetsInTree(BindWA& wa, 
				     const ULng32 arrayMaxsize,
				     const AtomicityType atomicity)
{
  for (Int32 i = 0; i < getArity(); i++) {
    if (child(i)) {
      child(i) = child(i)->xformRowsetsInTree(wa, arrayMaxsize);
    }
  }
  
  return this;
}

// apply xformRowsetsInTree to RelRoot & return transformed tree
RelExpr *RelRoot::xformRowsetsInTree(BindWA& wa,
				     const ULng32 arrayMaxsize,
				     const AtomicityType atomicity)
{
  if (hostArraysArea_) return this; // we've been here, done that

  if (wa.getHostArraysArea()) {
    // start out with array area pointed to by bindWA.
    // For subqueries and children of union node we allocate a new HostArrayWA,
    // set bindWA to point to it, and call xformRowsetsInTree. For subqueries 
    // the newly created array WA has a copy of the outer query's inputSizeExpr_.
    hostArraysArea_ = wa.getHostArraysArea();
  }
  else {
  // start out with empty host arrays
  hostArraysArea_ = new (wa.wHeap()) HostArraysWA(&wa);
  }

  hostArraysArea_->setInputArrayMaxSize(arrayMaxsize) ;  // used to determine input array max. size 
                                                         // for ODBC queries.

  if (child(0)) {
    switch (child(0)->getOperatorType()) {
    case REL_COMPOUND_STMT:
      // Drill down until it's rowset-safe.
      child(0) = child(0)->xformRowsetsInTree(wa);
      break;
    case REL_UNION:
      if (((Union *)(child(0).getPtr()))->getUnionForIF()) {
        child(0) = child(0)->xformRowsetsInTree(wa);
      }
      else {
        return hostArraysArea_->modifyTree(this, atomicity);
      }
      break;

    default:
      {
	// REL_ROOT above non-compound-stmt
	RelExpr * newExpr =
	  hostArraysArea_->modifyTree(this, atomicity); // open sesame!
	
	return newExpr;
      }
    }
  }

  return this;
}

// retrive the source/target schema names from OVERRIDE_SCHEMA
void BindWA::initializeOverrideSchema()
{
  NAString osSettings = schemaDB_->getDefaults().getValue(OVERRIDE_SCHEMA);
  Int32 len = osSettings.length();

  osFromSchema_ = "";
  osToSchema_ = "";

  if (len == 0)                             // empty 
    return;  

  extractOverrideSchemas(osSettings.data(), osFromSchema_, osToSchema_);

  // remove "" and leading/trailing spaces
  if ( osFromSchema_(0) == '\"' )
  {
    osFromSchema_ = osFromSchema_(1,osFromSchema_.length()-2);
    osFromSchema_ = osFromSchema_.strip(NAString::both);
  }
  if ( osToSchema_(0) == '\"' )
  {
    osToSchema_ = osToSchema_(1,osToSchema_.length()-2);
    osToSchema_ = osToSchema_.strip(NAString::both);
  }

  if ( (!osFromSchema_.isNull()) && (!osToSchema_.isNull()) 
    && (osFromSchema_ != osToSchema_) )
    overrideSchemaEnabled_ = TRUE;
  else
    overrideSchemaEnabled_ = FALSE;

}

// to extract override_schema settings from value (fromSchema:toSchema)
// and return in fromSchema and toSchema
void extractOverrideSchemas(const char *value, NAString& fromSchema, NAString& toSchema)
{
  fromSchema = "";
  toSchema = "";
  Int32 len = strlen(value);

  if ( (len > ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES*2+2) 
       || (len < 3) )  // at least x:y
    return;

  char in[ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES*2+2];
  char fromSch[ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES*2+2];
  char toSch[ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES*2+2];
  strcpy(in, value);
  char *p = in;

  //find from schema
  len = extractDelimitedName(fromSch, p, ':');
   
  if ( (len > 0) && (len <= ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES) 
    && (strlen(value) > (size_t)len+1) ) // value long enough for at least one more chr for toSch
  {
    // find to schema
    len = extractDelimitedName(toSch, p + len + 1, '\0');

    if ( (len > 0) && (len <= ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES) )
    {
      fromSchema = NAString(fromSch);
      toSchema = NAString(toSch);
      // remove leading and trailing spaces
      fromSchema = fromSchema.strip(NAString::both);
      toSchema = toSchema.strip(NAString::both);
      // UPPER if not delimited
      if (fromSchema(0)!='\"')
        fromSchema.toUpper();
      if (toSchema(0)!='\"')
        toSchema.toUpper();
    }
  }

  return;
}

// OVERRIDE_SCHEMA: replace the source schema with the target schema
void BindWA::doOverrideSchema(CorrName& corrName)
{
  if (   
         (toOverrideSchema_ )
         // the database object must have been named
      && ( ! corrName.getQualifiedNameObj().getObjectName().isNull() )
         // do not override if no schema was specified (table alias)
      && ( ! corrName.getQualifiedNameObj().getSchemaName().isNull() )
         // not to override for other tables like TRIGTEMP_TABLE or IUD_LOG_TABLE
         // NORMAL_TABLE will also cover synonym, view and MV
      && ( corrName.getSpecialType() == ExtendedQualName::NORMAL_TABLE )
         // not in DDL
      && ( ! inDDL() )
         // not in view expansion
      && ( ! getCurrentScope()->getInViewExpansion() )
         // not in CTAS
      && ( ! inCTAS() )
     )
  {
    // specified schema is the same as the from_schema
    if ( corrName.getQualifiedNameObj().getSchemaName() == osFromSchema_ )
    {
      corrName.getQualifiedNameObj().setSchemaName(osToSchema_);
      return;
    }
    // wildcard override for any schema 
    if ( osFromSchema_ == "*" ) 
    {
      corrName.getQualifiedNameObj().setSchemaName(osToSchema_);
      return;
    }
  }
}

// if DEFAULT_SCHEMA_ACCESS_ONLY, can only access default and public schemas
NABoolean BindWA::violateAccessDefaultSchemaOnly(const QualifiedName& objName)
{
  NAString cat=objName.getCatalogName();
  NAString sch=objName.getSchemaName();

  // when this is called, catalog and schema should have been assigned
  if (!cat.isNull() && !sch.isNull()) 
  {
    
    // let the following system catalogs and schemas pass
    // they are used in tools like HP-DM, etc.
    if ( (cat == "MANAGEABILITY") ||
         (cat == "HP_SYSTEM_CATALOG") ||
         (sch == "HP_METRICS") ||
         (sch == "HP_DEFINITION_SCHEMA") )
      return FALSE;

    // let volatile objects pass
    if (!strncmp(sch.data(), "VOLATILE_SCHEMA_MX", 18)) 
      return FALSE;

    SchemaName objSchName(sch,cat);
    if (objSchName.matchDefaultPublicSchema())
      return FALSE;
  }

  return raiseAccessDefaultSchemaOnlyError();
}

NABoolean BindWA::raiseAccessDefaultSchemaOnlyError()
{
  // no error for the following situations
  if ( (CmpCommon::getDefault(DEFAULT_SCHEMA_ACCESS_ONLY)!=DF_ON)
       || noNeedToLimitSchemaAccess()               // if the temporary flag is set
       || CmpCommon::context()->isSecondaryMxcmp()  // second mxcmp (update stats, etc.)
       || getCurrentScope()->getInViewExpansion()   // in view expansion
       || Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL) 
                                                    // internal query from executor
       || (isInTrigger() && !inDDL())               // in trigger actions       
     )
    return FALSE;

  *CmpCommon::diags() << DgSqlCode(-30044);
  setErrStatus();
  return TRUE;
}

NABoolean BindWA::queryCanUseSeaMonster()
{
  // Values defined for the queryCanUseSeaMonster_ variable
  // -1 : the variable is not yet initialized
  //  0 : the query cannot use SM
  //  1 : the query can use SM

  // If the variable is not initialized, we compute the value based on
  // two things: an environment variable and the SEAMONSTER default.
  if (queryCanUseSeaMonster_ == -1)
  {
    // Initialize to OFF
    queryCanUseSeaMonster_ = 0;

    // Allow SM if CQD is ON, or CQD is SYSTEM and env var is 1
    if (CmpCommon::getDefault(SEAMONSTER) == DF_ON)
    {
      queryCanUseSeaMonster_ = 1;
    }
    else if (CmpCommon::getDefault(SEAMONSTER) == DF_SYSTEM)
    {
      const char *e = getenv("SQ_SEAMONSTER");
      if (e && e[0] == '1')
        queryCanUseSeaMonster_ = 1;
    }
  }

  return (queryCanUseSeaMonster_ == 0 ? FALSE : TRUE);
}

//----------------------------------------------------------------------
// qualNameHashFunc()
// calculates a hash value given a QualifiedName.Hash value is mod by
// the hashTable size in HashDictionary.
//----------------------------------------------------------------------
ULng32 BindWA::qualNameHashFunc(const QualifiedName& qualName)
{
  ULng32 index = 0;
  const NAString& name = qualName.getObjectName();
  
  for(UInt32 i=0;i<name.length();i++)
    {
      index += (ULng32) (name[i]);
    }
  return index;
}

// ***********************************************************************
// Additional BindWA methods are found in BindRelExpr.C ...
// ***********************************************************************
