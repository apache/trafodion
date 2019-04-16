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
* File:         RelCache.cpp
* Description:  All the RelExpr methods introduced by query caching
* Created:      2/23/2001
* Language:     C++
*
*
******************************************************************************
*/
#define   SQLPARSERGLOBALS_FLAGS   // must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "AllRelExpr.h"
#include "CacheWA.h"
#include "CmpMain.h"
#include "GroupAttr.h"
#include "OptHints.h"
#include "QRDescGenerator.h"
#include "HDFSHook.h"
#include "SqlParserGlobals.h"      // must be last #include
// append an ascii-version of GenericUpdate into cachewa.qryText_
void GenericUpdate::generateCacheKey(CacheWA& cwa) const
  // NB: This comment applies to all generateCacheKey methods. 
  // generateCacheKey is used to generate a string representation s of the
  // "parameterized" query. Since this string s is used by QCache::lookUp
  // to determine if a query is in the cache, it is essential that:
  //   (1) two different queries have different string representations
  //   (2) two queries that differ only in their query literals should
  //       have the same string representations
  // One possible implementation of generateCacheKey is to use the query's
  // original query text. But, original query text does not satisfy (2). 
  // To get (2), we call generateCacheKey() from RelRoot::normalizeForCache
  // which, by definition, replaced query literals with constant parameters.
  // However, generateCacheKey must also satisfy (1). generateCacheKey must
  // generate two different strings for two logically different queries.
  //
  // To satisfy requirements (1) and (2), generateCacheKey and
  // normalizeForCache must be in sync -- every user-specified expr that 
  // generateCacheKey emits into cwa.qryText_ must be examined by 
  // normalizeForCache for possible replacement of any literal there into 
  // a constant parameter. 
  //
  // In order for the literal-into-constantparameter replacement to be safe,
  // isCacheableExpr must visit all user-specified exprs to make sure that
  // only constants that can be safely cast into the query's target types
  // are considered cacheable. For example, given this update query
  //   update t set a = 'xyz' where pk = 1;
  // isCacheableeExpr, normalizeForCache, and generateCacheKey must cooperate
  // so that:
  // 1) isCacheableExpr rejects the query as noncacheble if 'xyz' cannot be
  //    safely cast into a's target type, eg, 'xyz' may be too long if a's
  //    type is char(1).
  // 2) normalizeForCache must visit and replace both 'xyz' and 1 with
  //    appropriate constant parameters.
  // 3) generateCacheKey must emit some string representation of the
  //    parameterized query, eg, "update t set a = % where pk = %".
  //    generateCacheKey can emit more stuff, eg, internally specified
  //    begin/end-key predicates, but it must emit a string representation
  //    of all user-specified parts of the query.
{
  // append to cwa.qryText_ GenericUpdate's "essential" data members
  RelExpr::generateCacheKey(cwa);
  // An extension of the fix to 10-010618-3505, 10-010619-3515: 
  // for "after bind" Insert/Update/Delete queries, include table's 
  // RedefTime into cwa.qryText_ to make sure we get a cache hit only on 
  // query that reference table(s) that have not changed since the query's 
  // addition to the cache. The queries that reference altered table(s) 
  // will never be hit again and will eventually age out of the cache.
  // This is not strictly necessary, but it speeds up the processing
  // of insert/update/delete queries on altered tables.
  const NATable *tbl;
  if (cwa.getPhase() >= CmpMain::BIND && 
      getTableDesc() && (tbl=getTableDesc()->getNATable()) != NULL) {
    char redefTime[40];
    convertInt64ToAscii(tbl->getRedefTime(), redefTime);
    cwa += " redef:";
    cwa += redefTime;
  }
  ItemExpr *newExpr = newRecExprTree_ ? newRecExprTree_ :
    newRecExpr_.rebuildExprTree(ITM_ITEM_LIST);
  if (newExpr) { 
    cwa += " newRecExpr:"; 
    newExpr->generateCacheKey(cwa); 
  }
  // make sure cache key can distinguish these 2 queries:
  // prepare s from select * from (update t042qT8 set b=7 where a=2) as t;
  // prepare s from select * from (update t042qT8 set b=7 set on rollback c=2 
  //                               where a=2) as t;
  ItemExpr *setOnRollback;
  if (newRecBeforeExpr_.entries() > 0 &&
      (setOnRollback=newRecBeforeExpr_.rebuildExprTree(ITM_ITEM_LIST))) {
    cwa += " setOnRollback:"; 
    setOnRollback->generateCacheKey(cwa); 
  }
  ItemExpr *execPred = executorPredTree_ ? executorPredTree_ :
    executorPred_.rebuildExprTree();
  if (execPred) { 
    cwa += " execPred:"; 
    execPred->generateCacheKey(cwa); 
  }

  // MVs --
  // The NOLOG parameter is essential.
  if (isNoLogOperation()) { 
    cwa += " NOLOG"; 
  }

  // "current of cursor/hostvar" is essential
  if (currOfCursorName_) {
    currOfCursorName_->generateCacheKey(cwa); 
  }

  // not sure if the following are essential, but better to be safe & 
  // slightly inefficient than to deliver a false hit (ie, wrong plan)
  cwa += mtsStatement_ ? "m1" : "m0";
  cwa += noFlow_ ? "n1" : "n0";
  cwa += noRollback_ ? "o1" : "o0";
  cwa += noCheck_ ? "nc" : "dc";

  // not sure if the following are essential, but we don't know how
  // to quickly & cheaply include them into our cachekey:
  //   updatedTableName_, tabId_, updateToSelectMap_, indexDesc_,
  //   newRecExprArray_, usedColumns_, newRecBeforeExpr_,
  //   newRecBeforeExprArray_, usedBeforeColumns_, potentialOutputs_
  //   indexNumberArray_, scanIndexDesc_, rowsAffected_, stoi_,
  //   oldToNewMap_

  // The following data members are not "essential" to generateCacheKey
  // (at least "after bind") because they are either covered by other
  // data members (eg, beginKeyPred and endKeyPred_ are covered by the
  // selection pred in RelExpr) or they are not yet defined until later
  // (eg, after the optimize phase):
  //   indexNewRecExprArrays_, beginKeyPred_, endKeyPred_,
  //   pathKeys_, partKeys_, indexBeginKeyPredArray_,
  //   indexEndKeyPredArray_, checkConstraints_
}

// is this entire expression cacheable after this phase?
NABoolean GenericUpdate::isCacheableExpr(CacheWA& cwa)
{
  // descend to scans early to get cwa.numberOfScans_ 
  if (!RelExpr::isCacheableExpr(cwa)) {
    return FALSE;
  }

  // Make "{update|delete} ... where current of cursor" non-cacheable
  // so that stale cache will not lead to timestamp mismatch error at
  // runtime.  AQR attempts to handle this error, but only after the 
  // referenced cursor is closed due to transaction rollback.  This is
  // Solution 10-100425-9755.
  if (currOfCursorName()) { 
    return FALSE; 
  }

  if (cwa.getPhase() >= CmpMain::BIND) {
    // make sure any literals in the assignment clause can be safely
    // cast and assigned to their target types at plan-backpatch-time
    ItemExpr *newExpr = newRecExprTree_ ? newRecExprTree_ :
      newRecExpr_.rebuildExprTree(ITM_ITEM_LIST);
    if (newExpr && !newExpr->isSafelyCoercible(cwa)) { 
      return FALSE; 
    }
    // reject as non-cacheable queries such as
    //   prepare s from select * from (update t042qT8 set b=7 
    //   set on rollback c=12345678901234567890 where a=2) as t;
    ItemExpr *setOnRollback;
    if (newRecBeforeExpr_.entries() > 0 &&
        (setOnRollback=newRecBeforeExpr_.rebuildExprTree(ITM_ITEM_LIST)) 
        && !setOnRollback->isSafelyCoercible(cwa)) {
      return FALSE; 
    }
    // make sure any executor predicate is cacheable
    ItemExpr *execPred = executorPredTree_ ? executorPredTree_ :
      executorPred_.rebuildExprTree();
    if (execPred) {
      cwa.setHasPredicate();
      if (execPred->hasNoLiterals(cwa)) {
        // predicate with no literals is cacheable
      }
      else {
        cwa.setPredHasNoLit(FALSE);
        return execPred->isCacheableExpr(cwa);
      }
    }

    // at this time, not cacheable if subquery is specified in 
    // UPDATE SET clause.
    // This could be enabled later.
    if (subqInUpdateAssign()) {
      return FALSE;
    }
  }
  else {
    if ((getTableName().isPartitionNameSpecified()) ||
	(getTableName().isLocationNameSpecified()) ||
	(getTableName().isPartitionRangeSpecified()))
      return FALSE; // If PartnClause is used no cache hit before bind stage.
  }
  return TRUE; // may be cacheable
}

RelExpr* Scan::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }

  if (CmpCommon::getDefault(QUERY_CACHE_TABLENAME) == DF_OFF) {
    // replace descendants' literals into ConstantParameters
    return RelExpr::normalizeForCache(cwa, bindWA);
  }

  // replace tablename with a prototyped tablename.
  TableDesc * td = getTableDesc();
  CorrName &origName = td->getCorrNameObj();
  if (origName.getPrototype() == NULL)
    {
      Lng32 CACHED_MAX_ANSI_NAME_EXTERNAL_LEN = 128;
      NAString hvName("dummy_name");
      HostVar * hv = 
	new(bindWA.wHeap()) 
	HostVar(hvName, 
		new(bindWA.wHeap()) SQLChar(bindWA.wHeap(), CACHED_MAX_ANSI_NAME_EXTERNAL_LEN));
      hv->setPrototypeValue(origName.getQualifiedNameAsString());
      hv->synthTypeAndValueId();
      hv->setIsCachedParam(TRUE);

      CorrName cn("HostVar$", 
		  bindWA.wHeap(),
		  hv->getName(),   // debugging ease
		  "$bogus");

      cn.setPrototype(hv);
      NAString *tmpName =
	new (bindWA.wHeap()) 
	NAString(hv->getPrototypeValue(), bindWA.wHeap());
      cn.setUgivenName(*tmpName);
      cn.applyDefaults(&bindWA, bindWA.getDefaultSchema());

      td->setCorrName(cn);
      setTableName(cn);

      char * strval = 
	new(bindWA.wHeap()) char[CACHED_MAX_ANSI_NAME_EXTERNAL_LEN];
      strcpy(strval, origName.getQualifiedNameAsString().data());
      CharType * typ = 
	new(bindWA.wHeap()) SQLChar(bindWA.wHeap(), CACHED_MAX_ANSI_NAME_EXTERNAL_LEN, FALSE);
      ConstValue * cv = 
	new(bindWA.wHeap()) ConstValue(typ, strval, CACHED_MAX_ANSI_NAME_EXTERNAL_LEN);

      ConstantParameter* result = new(bindWA.wHeap()) ConstantParameter
	(*cv, bindWA.wHeap(), cwa.getPhase() == CmpMain::PARSE);
      result->synthTypeAndValueId();
      cwa.addConstParam(result, bindWA);
      hv->setPMOrdPosAndIndex(COM_UNKNOWN_DIRECTION,
			      -1,
			      (Int32)cwa.getConstParams().entries());
    }

  // replace descendants' literals into ConstantParameters
  return RelExpr::normalizeForCache(cwa, bindWA);
}

// change literals of a cacheable query into ConstantParameters 
RelExpr* GenericUpdate::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  if (cwa.getPhase() >= CmpMain::BIND) {
    if (currOfCursorName_) {
      // do NOT parameterize the assignment clause(s) of positioned updates
      // because "update t051t22 set b = -1 where current of c1" in esqlc
      // program such as core/etest051.sql generates an assertion clause:
      // "...if_then_else(b= :hv0),return_true,return_true(raiserror())"
      // (see BindItemExpr.cpp Scan::bindUpdateCurrentOf) as part of
      // GenericUpdate::bindNode. Otherwise, the result is an error 8106.
      // The root cause is incomplete parameterization -- the update 
      // becomes "update t051t22 set b = 0-? where current of c1" but 
      // "...if_then_else(b= :hv0),return_true,return_true(raiserror())"
      // is untouched causing an error 8106 at runtime.
    }
    else {
      if (newRecExprTree_) {
        newRecExprTree_->normalizeForCache(cwa, bindWA);
      }
      else {
        newRecExpr_.normalizeForCache(cwa, bindWA);
      }
      // parameterize "set on rollback" clause for queries such as
      //   prepare s from select * from (update t042qT8 set b=7 
      //   set on rollback c=2 where a=2) as t;
      newRecBeforeExpr_.normalizeForCache(cwa, bindWA); 
    }
    if (executorPredTree_) {
      executorPredTree_->normalizeForCache(cwa, bindWA);
    }
    else {
      executorPred_.normalizeForCache(cwa, bindWA);
    }
  }

  // Solution: 10-060327-5370 and 10-060418-5903
  // Record the context-wide isolation_level_for_updates value in 
  // CacheWA when procssing an IDU stmt. Use ISOLATION_LEVEL_FOR_UPDATES 
  // if it is specified. Otherwise, use ISOLATION_LEVEL. The initial value
  // in cwa is IL_NOT_SPECIFIED_.
  if ( cwa.getIsoLvlForUpdates() == TransMode::IL_NOT_SPECIFIED_ ) {
    TransMode::IsolationLevel il;

    ActiveSchemaDB()->getDefaults().getIsolationLevel (il,
         CmpCommon::getDefault(ISOLATION_LEVEL_FOR_UPDATES));

    if ( il == TransMode::IL_NOT_SPECIFIED_ ) {
       ActiveSchemaDB()->getDefaults().getIsolationLevel (il,
            CmpCommon::getDefault(ISOLATION_LEVEL));
    }
    cwa.setIsoLvlForUpdates(il);
  }

  // replace descendants' literals into ConstantParameters
  return RelExpr::normalizeForCache(cwa, bindWA);
}

// append an ascii-version of IsolatedScalarUDF into cachewa.qryText_
void IsolatedScalarUDF::generateCacheKey(CacheWA &cwa) const
{
  NARoutine *routine = NULL;
  NARoutine *action = NULL;

  RelExpr::generateCacheKey(cwa);

  cwa += " UDFname:";
  cwa += getRoutineName().getQualifiedNameAsAnsiString().data();
  if (cwa.getPhase() >= CmpMain::BIND &&
      getRoutineDesc() &&
      (routine=getRoutineDesc()->getNARoutine()) != NULL)
  {
    char redefTime[40];
    convertInt64ToAscii(routine->getRedefTime(), redefTime);
    cwa += " redef:";
    cwa += redefTime;
  }

  if (getRoutineDesc() != NULL && getRoutineDesc()->isUUDFRoutine())
  {
    cwa += " action:";
    cwa += getRoutineDesc()->getActionNameAsGiven();

    if (cwa.getPhase() >= CmpMain::BIND &&
       getRoutineDesc() &&
       (action=getRoutineDesc()->getActionNARoutine()) != NULL)
    {
       char redefTime[40];
       convertInt64ToAscii(action->getRedefTime(), redefTime);
       cwa += " actredef:";
       cwa += redefTime;
    }
  }

  const ItemExpr *paramExpr = (getProcAllParamsTree() == NULL) ? 
                      getProcInputParamsVids().rebuildExprTree(ITM_ITEM_LIST) :
                      getProcAllParamsTree(); 
  if (paramExpr)
  { 
    cwa += " arg:(";
    paramExpr->generateCacheKey(cwa); 
    cwa += ")";
  }
}

// append an ascii-version of CallSP into cachewa.qryText_
void CallSP::generateCacheKey(CacheWA &cwa) const
{
  RelExpr::generateCacheKey(cwa);

  cwa += " CallSPname:";
  cwa += getRoutineName().getQualifiedNameAsAnsiString().data();

  const ItemExpr *paramExpr = (getProcAllParamsTree() == NULL) ? 
                      getProcInputParamsVids().rebuildExprTree(ITM_ITEM_LIST) :
                      getProcAllParamsTree(); 
  if (paramExpr)
  { 
    cwa += " arg:";
    paramExpr->generateCacheKey(cwa); 
  }
}

// append an ascii-version of GroupByAgg into cachewa.qryText_
void GroupByAgg::generateCacheKey(CacheWA &cwa) const
{
  RelExpr::generateCacheKey(cwa);

  // group by col/expr is an important part of the key
  ItemExpr *grpExpr = groupExprTree_ ? groupExprTree_ :
    groupExpr_.rebuildExprTree(ITM_ITEM_LIST);
  if (grpExpr) { 
    cwa += " gBy:"; 

    if (isRollup())
      {
        cwa += " roll:";

        ItemExpr * ie = rollupGroupExprList().rebuildExprTree(ITM_ITEM_LIST);
        if (ie)
          {
            ie->generateCacheKey(cwa);
          }
      }
    if (NOT extraOrderExpr().isEmpty() )
    {
        cwa += " extraOrder:";

        ItemExpr * ie = extraOrderExpr().rebuildExprTree(ITM_ITEM_LIST);
        if (ie)
          {
            ie->generateCacheKey(cwa);
          }
    }

    grpExpr->generateCacheKey(cwa); 
  }
}


// is this entire expression cacheable after this phase?
NABoolean GroupByAgg::isCacheableExpr(CacheWA& cwa)
{
  // descend to scans early to get cwa.numberOfScans_ 
  if (!RelExpr::isCacheableExpr(cwa)) {
    return FALSE;
  }
  // is the group by col/expr cacheable?
  ItemExpr *grpExpr = groupExprTree_ ? groupExprTree_ :
    groupExpr_.rebuildExprTree(ITM_ITEM_LIST);
  if (grpExpr && !grpExpr->isCacheableExpr(cwa)) { 
    return FALSE; 
  }
  return TRUE; // may be cacheable
}

// append an ascii-version of Insert into cachewa.qryText_
void Insert::generateCacheKey(CacheWA &cwa) const
{
  GenericUpdate::generateCacheKey(cwa);
  if (insertColTree_) { 
    cwa += " insCol:"; 
    insertColTree_->generateCacheKey(cwa); 
  }
  // order by clause is important
  ItemExpr *orderBy = orderByTree_ ? orderByTree_ :
    reqdOrder_.rebuildExprTree();
  if (orderBy) { 
    cwa += " order:"; 
    orderBy->generateCacheKey(cwa); 
  }

  const NATable *tbl;
  if (cwa.getPhase() >= CmpMain::BIND && 
      getTableDesc() && (tbl=getTableDesc()->getNATable()) != NULL) {
    // If PARTITION clause has been used we must reflect that in the key.
    if (tbl->isPartitionNameSpecified()) {
      cwa += " partition:";
      cwa += tbl->getClusteringIndex()->getFileSetName().getQualifiedNameAsString().data();
    }
    // If PARTITION range has been used we must reflect that in the key.
    else if (tbl->isPartitionRangeSpecified()) {
      cwa += " partition:";

      char str[100];
      sprintf(str, " from %d to %d", 
	      tbl->getExtendedQualName().getPartnClause().getBeginPartitionNumber() ,
	      tbl->getExtendedQualName().getPartnClause().getEndPartitionNumber());
      cwa += str;
    }
  }

  if (isUpsert())
    {
      cwa += " upsert:";
    }
}

// is this entire expression cacheable after this phase?
NABoolean Insert::isCacheableExpr(CacheWA& cwa)
{
  // non-single-row inserts are non-cacheable 
  if (insertType_ != SIMPLE_INSERT) { 
    return FALSE; 
  }
  // single-row insert may be cacheable
  return GenericUpdate::isCacheableExpr(cwa);
}

// change literals of a cacheable query into ConstantParameters 
RelExpr* Insert::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  // replace descendants' literals into ConstantParameters
  return GenericUpdate::normalizeForCache(cwa, bindWA);
}

// is this entire expression cacheable after this phase?
NABoolean Delete::isCacheableExpr(CacheWA& cwa)
{
  return GenericUpdate::isCacheableExpr(cwa);
}

// append an ascii-version of Merge into cachewa.qryText_
void MergeUpdate::generateCacheKey(CacheWA &cwa) const
{
  Update::generateCacheKey(cwa);

  if (insertCols_) { 
    cwa += " insertCols:"; 
    insertCols_->generateCacheKey(cwa); 
  }

  if (insertValues_) { 
    cwa += " insertValues:"; 
    insertValues_->generateCacheKey(cwa); 
  }
}

// is this entire expression cacheable after this phase?
NABoolean MergeUpdate::isCacheableExpr(CacheWA& cwa)
{
  if ((insertValues_) &&
      (insertValues_->isCacheableExpr(cwa))) {
    setNonCacheable();
    return FALSE;
  }

  return Update::isCacheableExpr(cwa);
}

// change literals of a cacheable query into ConstantParameters 
RelExpr* MergeUpdate::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }

  if (insertValues_) {
    insertValues_ = insertValues_->normalizeForCache(cwa, bindWA);
  }

  // replace descendants' literals into ConstantParameters
  return Update::normalizeForCache(cwa, bindWA);
}

// append an ascii-version of Merge into cachewa.qryText_
void MergeDelete::generateCacheKey(CacheWA &cwa) const
{
  Delete::generateCacheKey(cwa);

  if (insertCols_) { 
    cwa += " insertCols:"; 
    insertCols_->generateCacheKey(cwa); 
  }

  if (insertValues_) { 
    cwa += " insertValues:"; 
    insertValues_->generateCacheKey(cwa); 
  }
}

// is this entire expression cacheable after this phase?
NABoolean MergeDelete::isCacheableExpr(CacheWA& cwa)
{
  if ((insertValues_) &&
      (insertValues_->isCacheableExpr(cwa))) {
    setNonCacheable();
    return FALSE;
  }

  return Delete::isCacheableExpr(cwa);
}

// change literals of a cacheable query into ConstantParameters 
RelExpr* MergeDelete::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }

  if (insertValues_) {
    insertValues_ = insertValues_->normalizeForCache(cwa, bindWA);
  }

  // replace descendants' literals into ConstantParameters
  return Delete::normalizeForCache(cwa, bindWA);
}

// append an ascii-version of Join into cachewa.qryText_
void Join::generateCacheKey(CacheWA &cwa) const
{
  RelExpr::generateCacheKeyNode(cwa);
  if (isNaturalJoin_) {
    cwa += " natj "; 
  }
  ItemExpr *pred = joinPredTree_ ? joinPredTree_ :
    joinPred_.rebuildExprTree();
  if (pred) { 
    cwa += " joinPred:";
    pred->generateCacheKey(cwa); 
  }
  generateCacheKeyForKids(cwa);
}

// is this entire expression cacheable after this phase?
NABoolean Join::isCacheableExpr(CacheWA& cwa)
{
  if (cwa.getPhase() >= CmpMain::BIND) {
    // must first descend to scans to get cwa.numberOfScans_ 
    if (!RelExpr::isCacheableExpr(cwa)) {
      return FALSE;
    }
    if (isCacheableNode(cwa.getPhase())) { 
      cwa.setConditionallyCacheable();
    }
    // if we allow joins of views to be cached, query caching cannot 
    // distinguish between (see note at bottom of cachewa.h)
    //   select avg(f.a) from v f, v s group by f.b;
    //   select avg(s.a) from v f, v s group by f.b;
    //   select avg(t.a) from v f, t   group by f.b;
    // assuming v is "create view v from select * from t". We avoid
    // false cache hits by detecting the possible occurrence of such 
    // view joins here and later using cwa.isViewJoin_ to include
    // their query texts into their cache keys.
    //
    // A view is repsented by a renamed table with isView() returnning 
    // TRUE.

    RelExpr *c0 = child(0);
    RelExpr *c1 = child(1);
    if ((c0->getOperatorType() == REL_RENAME_TABLE &&
        ((RenameTable *)c0)->isView() == TRUE)
        ||
        (c1->getOperatorType() == REL_RENAME_TABLE &&
        ((RenameTable *)c1)->isView() == TRUE)) {
      cwa.foundViewJoin();
    }
    // check its join predicate
    ItemExpr *pred = joinPredTree_ ? joinPredTree_ :
      joinPred_.rebuildExprTree();
    if (pred) {
      cwa.setHasPredicate();
      // is join predicate cacheable?
      if (pred->hasNoLiterals(cwa)) {
        // predicate with no literals is cacheable
      }
      else {
        cwa.setPredHasNoLit(FALSE);
        if (!pred->isCacheableExpr(cwa)) {
          // a non-cacheable predicate renders Join non-cacheable.
          setNonCacheable();
          return FALSE;
        }
      }
    }
    return TRUE; // join may be cacheable
  }
  return FALSE;
}

// change literals of a cacheable query into ConstantParameters 
RelExpr* Join::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  return RelExpr::normalizeForCache(cwa, bindWA);
}

// are RelExpr's kids cacheable after this phase?
NABoolean RelExpr::cacheableKids(CacheWA& cwa)
{
  switch (cwa.getPhase()) {
  case CmpMain::PARSE:
  case CmpMain::BIND: {
    Int32 arity = getArity();
    if (arity <= 0) { // we have no kids
      if (cwa.isConditionallyCacheable()) {
        // we're conditionally cacheable and have no kids
        setCacheableNode(cwa.getPhase()); 
        return TRUE; // so, we're cachable
      }
      else {
        return FALSE; // MAYBECACHEABLE is not cacheable at this phase
        // don't mark this node non-cacheable because this
        // RelExpr may be cacheable after the next phase.
      }
    }
    // cacheability of child(ren) determine our cacheability
    for (Int32 x = 0; x < arity; x++) {
      if (!child(x) || // cases like "insert into t default values"
          // return 1 from getArity() even if child(0) is NULL; so
          // guard against this potential mxcmp crash and consider
          // these cases non-cacheable during the PARSE stage.
          child(x)->isNonCacheable()) {
        // the 1st noncacheable child makes us noncacheable
        setNonCacheable();
        return FALSE;
      }
      else if (!child(x)->isCacheableExpr(cwa)) {
        // noncacheable child
        return FALSE;
        // don't mark this node non-cacheable because this
        // RelExpr may be cacheable after the next phase.
      }
      else { // cacheable child
        continue; // look at next child
      }
    }
    // all children are cacheable, so we're cacheable too
    setCacheableNode(cwa.getPhase());
    return TRUE;
  }
  default:
    return FALSE;
  }
}

// append an ascii-version of RelExpr into cachewa.qryText_
void RelExpr::generateCacheKey(CacheWA &cwa) const
{
  generateCacheKeyNode(cwa);
  generateCacheKeyForKids(cwa);
}

// append an ascii-version of RelExpr node into cachewa.qryText_
void RelExpr::generateCacheKeyNode(CacheWA &cwa) const
{
  // emit any "[firstn_sorted]" modifier
  if (firstNRows_ != -1) {
    char firstN[40]; 
    convertInt64ToAscii(((RelExpr*)this)->getFirstNRows(), firstN); 
    cwa += firstN; 
    cwa += " ";
  }
  // emit other "significant" parts of RelExpr
  cwa += getText();
  ItemExpr *pred = selPredTree() ? selPredTree() :
    getSelectionPred().rebuildExprTree();
  if (pred) { 
    cwa += " selPred:"; 
    pred->generateCacheKey(cwa); 
  }
  // make any optimizer hints part of the postbinder cache key so that
  // 2 cacheable queries with different optimizer hints do not match
  if (hint_) {
    CollIndex x, cnt=hint_->indexCnt();
    if (cnt > 0) {
      cwa += " xhint:";
      for (x = 0; x < cnt; x++) {
        cwa += (*hint_)[x].data();
        cwa += ",";
      }
    }
    char str[100];
    if (hint_->hasCardinality()) {
      sprintf(str, "card:%g", hint_->getCardinality());
      cwa += str;
    }
    if (hint_->hasSelectivity()) {
      sprintf(str, ",sel:%g", hint_->getSelectivity());
      cwa += str;
    }
  }
}

// append an ascii-version of RelExpr's kids into cachewa.qryText_
void RelExpr::generateCacheKeyForKids(CacheWA& cwa) const
{
  Int32 maxi = getArity();
  if (maxi) {
    cwa += " kids(";
    for (Lng32 i = 0; i < maxi; i++) {
      if (i > 0) { 
        cwa += ","; 
      }
      if ( child(i).getPtr() == NULL ) { 
        continue; 
      }
      child(i)->generateCacheKey(cwa);
    }
    cwa += ")";
  }
}

// return any Scan node from this RelExpr
Scan *RelExpr::getAnyScanNode() const
{
  if (getOperatorType() == REL_SCAN) { 
    return (Scan*)this; 
  }
  Scan *result = NULL;
  Int32 arity = getArity();
  for (Int32 x = 0; x < arity && !result; x++) {
    if (child(x)) { 
      result = child(x)->getAnyScanNode(); 
    }
  }
  return result;
}

// is this entire expression cacheable after this phase?
NABoolean RelExpr::isCacheableExpr(CacheWA& cwa)
{
  switch (cwa.getPhase()) {
  case CmpMain::PARSE:
  case CmpMain::BIND: {
    // does query have too many ExprNodes?
    if (cwa.inc_N_check_still_cacheable() == FALSE) {
      // yes. query with too many ExprNodes is not cacheable.
      return FALSE;
    }
    if (isNonCacheable()) { // this node is not cacheable
      return FALSE; // so the entire expression is not cacheable
      // don't mark this node non-cacheable because this
      // RelExpr may be cacheable after the next phase.
    }
    if (isCacheableNode(cwa.getPhase())) { 
      // must be an INSERT, UPDATE, DELETE, or SELECT node;
      // so, mark this expression as conditionally cacheable.
      cwa.setConditionallyCacheable();
    }
    // must descend to scans to get cwa.numberOfScans_ 
    if (!cacheableKids(cwa)) {
      return FALSE;
    }
    // this node is either cacheable or maybecacheable
    // check its selection predicate
    ItemExpr *pred = selPredTree() ? selPredTree() :
      getSelectionPred().rebuildExprTree();
    if (pred) {
      cwa.setHasPredicate();
      // is selection predicate cacheable?
      if (pred->hasNoLiterals(cwa)) {
        // predicate with no literals is cacheable
      }
      else {
        cwa.setPredHasNoLit(FALSE);
        if (!pred->isCacheableExpr(cwa)) {
          // a non-cacheable selection predicate 
          // renders entire RelExpr non-cacheable.
          setNonCacheable();
          return FALSE;
        }
      }
    }
    return TRUE; // RelExpr may be cacheable
  }
  default: { const NABoolean notYetImplemented = FALSE; 
  CMPASSERT(notYetImplemented);
  return FALSE;
    }
  }
}

// is this ExprNode cacheable after this phase?
NABoolean RelExpr::isCacheableNode(CmpPhase phase) const
{ 
  switch (phase) {
  case CmpMain::PARSE: 
    return cacheable_ == ExprNode::CACHEABLE_PARSE;
  case CmpMain::BIND:
    return cacheable_ == ExprNode::CACHEABLE_BIND ||
      cacheable_ == ExprNode::CACHEABLE_PARSE;
  default:
    break;
  }
  return FALSE;
}

// change literals of a cacheable query into ConstantParameters 
RelExpr* RelExpr::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  // replace descendants' literals into ConstantParameters
  normalizeKidsForCache(cwa, bindWA);
  if (cwa.getPhase() >= CmpMain::BIND) {
    if (selection_) {
      selection_ = selection_->normalizeForCache(cwa, bindWA);
    }
    else {
      selectionPred().normalizeForCache(cwa, bindWA);
    }
    // RelExpr::bindSelf has done this line during binding; but, we
    // must redo it to recognize any new constantparameters created 
    // by the above normalizeForCache call(s) as RelExpr inputs.
    getGroupAttr()->addCharacteristicInputs
      (bindWA.getCurrentScope()->getOuterRefs());
  }

  markAsNormalizedForCache();
  return this;
}

// change literals in cacheable query's kids into ConstantParameters
void RelExpr::normalizeKidsForCache(CacheWA& cachewa, BindWA& bindWA)
{
  Int32 arity = getArity();
  for (Int32 x = 0; x < arity; x++) {
    child(x) = child(x)->normalizeForCache(cachewa, bindWA);
  }
}

// mark this ExprNode as cacheable after this phase
void RelExpr::setCacheableNode(CmpPhase phase)
{ 
  switch (phase) {
  case CmpMain::PARSE: 
    cacheable_ = ExprNode::CACHEABLE_PARSE;
    break;
  case CmpMain::BIND:
    cacheable_ = ExprNode::CACHEABLE_BIND;
    break;
  default:
    break;
  }
}

// append an ascii-version of RelRoot into cachewa.qryText_
void RelRoot::generateCacheKey(CacheWA &cwa) const
{
  RelExpr::generateCacheKey(cwa);
  ItemExpr *cExpr = compExprTree_ ? compExprTree_ : 
    compExpr_.rebuildExprTree();
  if (cExpr) { 
    // append any select list into cache key
    cwa += " cExpr:"; 
    cExpr->generateCacheKey(cwa);
    // reflect any "[first n]"
    cwa += ((RelRoot*)this)->needFirstSortedRows() ? " 1stN" : " ";
    // Should the select_list aliases be a part of the cache key?
    // Their not affecting the compiled plan argues for their exclusion.
    // Their affecting sqlci's expected output argues for their inclusion.
    RETDesc *rDesc = getRETDesc(); 
    CollIndex degree, x;

    if (rDesc && (degree=rDesc->getDegree()) > 0) {
      cwa += " sla:";
      for (x = 0; x < degree; x++){
        cwa += rDesc->getColRefNameObj(x).getColName().data(); 
        cwa += " ";
      }

        // fix 0-061115-0532 (query cache didn't handle select with embedded 
        // update correctly). New/Old corr. names are recorded for embedded
        // updates here for exact match. This is important because otherwise 
        // a reuse of a query returning the old/new version of values for 
        // a query requesting new/old version is totally possible and 
        // unacceptable.
        //
        // Sample embedded update queries
        // select * from (update tab1 set x = x + 1 where x > 1 return new.*) y;
        // select * from (update tab1 set x = x + 1 where x > 1 return new.x, old.y) y;
        //
      if ( cwa.isUpdate() && isTrueRoot() == FALSE ) {
         cwa += " corrNamTok:";
         cwa += rDesc->getBindWA()->getCorrNameTokens();
      }
    }
  }
  // order by clause is important
  ItemExpr *orderBy = orderByTree_ ? orderByTree_ :
    reqdOrder_.rebuildExprTree();
  if (orderBy) { 
    cwa += " order:"; 
    orderBy->generateCacheKey(cwa); 
  }
  // statement-level access type & lock mode are important for multiuser 
  // applications. both are reflected in the stmt-level and/or context-wide
  // TransMode. So, we mimic RelRoot::codeGen logic here: "copy the current 
  //   context-wide TransMode, then overlay with this stmt's 'FOR xxx ACCESS' 
  //   setting, if any".
  TransMode tmode;
  tmode.updateTransMode(CmpCommon::transMode());

  StmtLevelAccessOptions &opts = ((RelRoot*)this)->accessOptions();
  if (opts.accessType() != TransMode::ACCESS_TYPE_NOT_SPECIFIED_) {
    tmode.updateAccessModeFromIsolationLevel
      (TransMode::ATtoIL(opts.accessType()));
    tmode.setStmtLevelAccessOptions();
  }
  if (isTrueRoot()) {
    // these are needed by Javier's qc stats virtual tbl interface
    cwa.setIsoLvl(tmode.getIsolationLevel());
    cwa.setAccessMode(tmode.getAccessMode());
    cwa.setAutoCommit(tmode.getAutoCommit());
    cwa.setFlags(tmode.getFlags());
    cwa.setRollbackMode(tmode.getRollbackMode());
    cwa.setAutoabortInterval(tmode.getAutoAbortIntervalInSeconds());
    cwa.setMultiCommit(tmode.getMultiCommit());
  }

  // needed to distinguish these queries and avoid a false hit
  // select * from (delete from t where a=2) as t;
  // select * from (delete from t where a=2 for SKIP CONFLICT ACCESS) as t;
  char mode[40]; 
  str_itoa(tmode.getIsolationLevel(), mode); cwa += " il:"; cwa += mode;
  str_itoa(tmode.getAccessMode(), mode); cwa += " am:"; cwa += mode;

  // Solution: 10-060418-5903
  str_itoa(cwa.getIsoLvlForUpdates(), mode); cwa += " ilu:"; cwa += mode;

  str_itoa(tmode.getAutoCommit(), mode); cwa += " ac:"; cwa += mode;
  str_itoa(tmode.getFlags(), mode); cwa += " fl:"; cwa += mode;
  str_itoa(tmode.getRollbackMode(), mode); cwa += " rm:"; cwa += mode;
  str_itoa(tmode.getAutoAbortIntervalInSeconds(), mode); cwa += " ai:"; cwa += mode;
  str_itoa(tmode.getMultiCommit(), mode); cwa += " mc:"; cwa += mode;

  if (opts.lockMode() != LOCK_MODE_NOT_SPECIFIED_) {
    // need to distinguish these queries and avoid a false hit
    //   select * from t in share mode;
    //   select * from t in exclusive mode;
    str_itoa(opts.lockMode(), mode); cwa += " lm:"; cwa += mode;
  }

  // updatableSelect_ is essential. Otherwise, queries like
  // "select * from t" and "select * from t for update" can confuse
  // query caching into a false hit, causing fullstack/test051 to fail.
  if (updatableSelect_) { 
    cwa += " 4updt "; 
  }
  // for update of col [,col]... clause is important
  ItemExpr *updCol = updateColTree_ ? updateColTree_ :
    updateCol_.rebuildExprTree();
  if (updCol) { 
    cwa += " updCol:"; 
    updCol->generateCacheKey(cwa); 
  }
  // making the CQS part of the key is more efficient than calling
  // CompilerEnv::changeEnv() in ControlDB::setRequiredShape()
  if (reqdShape_) { 
    reqdShape_->unparse(cwa.reqdShape_); 
  }
}

// is this entire expression cacheable after this phase?
NABoolean RelRoot::isCacheableExpr(CacheWA& cwa)
{
  //queries prefixed by display are not cacheable e.g. display select * from ...
  if(getDisplayTree())
    return FALSE;
  
  // Parallel extract producer queries are not cacheable
  if (numExtractStreams_ > 0)
    return FALSE;

  // descend to scans early to get cwa.numberOfScans_ 
  if (!RelExpr::isCacheableExpr(cwa)) {
    return FALSE;
  }
  if (cwa.getPhase() == CmpMain::PARSE) {
    // it is unclear why select...insert is not being cached.
    // For now, cache it only for internal queries. This is needed to
    // improve compile time of internal lob queries.

    if (! Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL) &&(compExprTree_ || compExpr_.entries() > 0)) {
      // insert-returning is not cacheable after parse
      return FALSE; 
    }
  }
  else if (cwa.getPhase() >= CmpMain::BIND) {
    // make sure select list is cacheable
    if (compExprTree_) {
      if  (!compExprTree_->isCacheableExpr(cwa)) { 
        return FALSE; 
      }
    }
    else if (!compExpr_.isCacheableExpr(cwa)) {
      return FALSE;
    }
  }

  if (isAnalyzeOnly())
    return FALSE;

  return TRUE;
}

// change literals of a cacheable query into ConstantParameters and save
// true root into cachewa so we can "bind" ConstantParameters as "inputvars"
RelExpr* RelRoot::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  if (isTrueRoot()) { 
    cwa.setTopRoot(this); 
  }
  if (cwa.getPhase() >= CmpMain::BIND) {
    // replace any select list literals into constant parameters
    if (compExprTree_) {
      compExprTree_ = compExprTree_->normalizeForCache(cwa, bindWA);
    }
    else {
      compExpr_.normalizeForCache(cwa, bindWA);
    }
  }
  // replace descendants' literals into ConstantParameters
  RelExpr *result = RelExpr::normalizeForCache(cwa, bindWA);
  if (cwa.getPhase() >= CmpMain::BIND) {
    // query tree has undergone BINDing, but RelExpr::normalizeForCache
    // may have introduced new ConstantParameters in place of ConstValues;
    // we want to BIND these new ConstantParameters but a RelRoot::bindNode()
    // call here would be overkill; we just want these new ConstantParameters
    // to be "bound" as "inputvars"; so, we selectively cut and paste code
    // from BindRelExpr.cpp RelRoot::bindNode into here to "bind" any new 
    // ConstantParameters as "inputvars".
    ItemExpr *inputVarTree = removeInputVarTree();
    if (inputVarTree) {
      inputVarTree->convertToValueIdList(inputVars(), &bindWA, ITM_ITEM_LIST);
      if (bindWA.errStatus()) {
        return NULL;
      }
    }
  }
  return result;
}

// append an ascii-version of Scan into cachewa.qryText_
void Scan::generateCacheKey(CacheWA &cwa) const
{
  RelExpr::generateCacheKey(cwa);
  // Fix to 10-010618-3505, 10-010619-3515: include this Scan table's 
  // RedefTime into cwa.qryText_ to make sure we get a cache hit only on 
  // query that reference table(s) that have not changed since the query's 
  // addition to the cache. The queries that reference altered table(s) 
  // will never be hit again and will eventually age out of the cache.
  const NATable *tbl;
  if (cwa.getPhase() >= CmpMain::BIND && 
      getTableDesc() && (tbl=getTableDesc()->getNATable()) != NULL) {
    char redefTime[40];
    convertInt64ToAscii(tbl->getRedefTime(), redefTime);
    cwa += " redef:";
    cwa += redefTime;

    if (tbl->isHiveTable()) {
      char lastModTime[40];
      Int64 mTime = tbl->getClusteringIndex()->getHHDFSTableStats()->getModificationTS();
      convertInt64ToAscii(mTime, lastModTime);
      cwa += " lastMod:";
      cwa += lastModTime;

      cwa += " numFiles:";
      char numFiles[20];
      Int64 numberOfFiles = tbl->getClusteringIndex()->getHHDFSTableStats()->getNumFiles();
      sprintf(numFiles, " %ld", numberOfFiles); 
      cwa += numFiles ;
    }
    // save pointer to this table. later, QueryCache::addEntry will use
    // this pointer to get to this table's histograms's timestamp
    cwa.addTable( (NATable*)tbl );
    // If PARTITION clause has been used we must reflect that in the key.
    if (tbl->isPartitionNameSpecified()) {
      cwa += " partition:";
      cwa += tbl->getClusteringIndex()->getFileSetName().getQualifiedNameAsString().data();
    }
    // If PARTITION range has been used we must reflect that in the key.
    else if (tbl->isPartitionRangeSpecified()) {
      cwa += " partition:";

      char str[100];
      sprintf(str, " from %d to %d", 
	      tbl->getExtendedQualName().getPartnClause().getBeginPartitionNumber() ,
	      tbl->getExtendedQualName().getPartnClause().getEndPartitionNumber());
      cwa += str;
    }
  }
  // We must reflect userTableName_.location into cache key.
  // Otherwise, two queries which differ only in location such as
  //   table table (table T058a, location $system.zsd12345.x1234500);
  //   table table (table T058a, location $data  .zsd12345.x1234500);
  // can confuse our query caching code to return a false hit and 
  // cause fullstack/test058 to fail.
  cwa += userTableName_.getLocationName().data();

  // Same with stream_ because queries like
  // "select * from t" and "select * from stream(t)" can
  // confuse query caching into a false hit causing test079 to fail.
  if (stream_) { 
    cwa += " stream "; 
  }

  if (getHbaseAccessOptions())
    {
      cwa += " hbaseVersions: ";
      char numVersions[20];
      sprintf(numVersions, " %d", getHbaseAccessOptions()->getHbaseVersions());
      cwa += numVersions ;
    }
}

// is this entire expression cacheable after this phase?
NABoolean Scan::isCacheableExpr(CacheWA& cwa)
{
  if (cwa.getPhase() >= CmpMain::BIND) {
    // save scan's TableDesc
    cwa.incNofScans(tabId_);

    // native hbase access is not cacheable for now.
    if ((getTableDesc()->getNATable()->isHbaseRowTable()) ||
	(getTableDesc()->getNATable()->isHbaseCellTable()))
      return FALSE;

    if (stream_) { // pub-sub streams are not cacheable
      return FALSE;
    }

    cwa.setConditionallyCacheable(); 
    if (CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL) >= 1 &&
        QRDescGenerator::hasRewriteEnabledMVs(getTableDesc())) {
      cwa.setRewriteEnabledMV();
    }
    return RelExpr::isCacheableExpr(cwa);
  }
  return FALSE;
}

// append an ascii-version of Tuple into cachewa.qryText_
void Tuple::generateCacheKey(CacheWA &cwa) const
{
  // Do not call RelExpr::generateCacheKey(cwa) here because it's redundant.
  // It does the same things as the code below. RelExpr::generateCacheKey() 
  // calls Tuple::getText() which has logic similar to the following code.
  ItemExpr *tExpr = tupleExprTree() ? tupleExprTree() :
    tupleExpr_.rebuildExprTree();
  if (tExpr) { 
    cwa += " tupExpr:"; 
    tExpr->generateCacheKey(cwa); 
  }
  else {
    RelExpr::generateCacheKey(cwa);
  }
}

// is this entire expression cacheable after this phase?
NABoolean Tuple::isCacheableExpr(CacheWA& cwa)
{
  // we do not call RelExpr::isCacheableExpr here because it's redundant
  // -- Tuple is a leaf node and has no predicates.

  ItemExpr *tExpr = tupleExprTree() ? tupleExprTree() :
    tupleExpr_.rebuildExprTree();
  return tExpr->isCacheableExpr(cwa);
}

// change literals of a cacheable query into ConstantParameters 
RelExpr* Tuple::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  if (tupleExprTree_) {
    tupleExprTree_ = tupleExprTree_->normalizeForCache(cwa, bindWA);
  }
  else {
    tupleExpr_.normalizeForCache(cwa, bindWA);
  }
  // replace descendants' literals into ConstantParameters
  return RelExpr::normalizeForCache(cwa, bindWA);
}

// append an ascii-version of Union into cachewa.qryText_
void Union::generateCacheKey(CacheWA &cwa) const
{
  RelExpr::generateCacheKeyNode(cwa);

  char buf[40];

  cwa += " flgs_: ";
  convertInt64ToAscii(flags_, buf);
  cwa += buf;

  cwa += " ctrFlgs_: ";
  convertInt64ToAscii(controlFlags_, buf);
  cwa += buf;

  cwa += " sysGen_: ";
  cwa += (isSystemGenerated_) ? "1" : "0";

  // turn on the following when condExprTree_ and trigExceptExprTree_
  // are considered part of the key
  //
  //if (condExprTree_) {
  //  cwa += " condExprTree_: ";
  //  condExprTree_->generateCacheKey(cwa);
  //}

  //if (trigExceptExprTree_) {
  //  cwa += " trigExceptExprTree_: ";
  //  trigExceptExprTree_->generateCacheKey(cwa);
  //}
  generateCacheKeyForKids(cwa);
}

NABoolean Update::isCacheableExpr(CacheWA& cwa)
{
  cwa.setIsUpdate(TRUE);
  return GenericUpdate::isCacheableExpr(cwa);
}

// append an ascii-version of FastExtract into cachewa.qryText_
void FastExtract::generateCacheKey(CacheWA &cwa) const
{
  RelExpr::generateCacheKeyNode(cwa);

  char buf[40];
  cwa += " targType_ ";
  str_itoa(getTargetType(), buf);
  cwa += buf;

  cwa += " targName_ ";
  cwa += getTargetName();

  cwa += " delim_ ";
  cwa += getDelimiter();

  cwa += " isAppend_ ";
  cwa += isAppend() ? "1" : "0";

  cwa += " includeHeader_ ";
  cwa += includeHeader() ? "1" : "0";

  cwa += " cType_ ";
  str_itoa(getCompressionType(), buf);
  cwa += buf;

  cwa += " nullString_ ";
  cwa += getNullString();

  cwa += " recSep_ ";
  cwa += getRecordSeparator();

  generateCacheKeyForKids(cwa);
}

