/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2000-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
*************************************************************************
*
* File:         CacheWA.C
* Description:  The workarea used by {Rel|Item}Expr::normalizeForCache and
*               {Rel|Item}Expr::isCacheableExpr
* Created:      08/03/2000
* Language:     C++
*
*
*
*
*************************************************************************
*/

#include "CacheWA.h"
#include "CmpMain.h"
#include "SchemaDB.h"

// create an empty list of constant parameters
ConstantParameters::ConstantParameters(NAHeap *h)
  : LIST(ConstantParameter*)(h)
{
}

// free our allocated memory 
ConstantParameters::~ConstantParameters()
{
  CollIndex x, count = entries();
  for (x = 0; x < count; x++) {
    delete at(x);
  }
}

// return our elements' total byte size
Lng32 ConstantParameters::getSize() const
{
  Lng32 result = 0;
  CollIndex x, limit = entries();
  for (x=0; x < limit; x++) {
    result += (*this)[x]->getSize();
  }
  return result;
}

// create an empty list of SelParameters
SelParameters::SelParameters(NAHeap *h)
  : LIST(SelParameter*)(h)
{
}

// free our allocated memory 
SelParameters::~SelParameters()
{
  CollIndex x, count = entries();
  for (x = 0; x < count; x++) {
    delete at(x);
  }
}

// return our elements' total byte size
Lng32 SelParameters::getSize() const
{
  Lng32 result = 0;
  CollIndex x, limit = entries();
  for (x=0; x < limit; x++) {
    result += (*this)[x]->getSize();
  }
  return result;
}

// Create and initialize cache work area. Allocate cachewa.qryText_
// with a reasonable estimate of its eventual length to avoid the
// penalty of repeated lengthening. Assume current query is not
// cacheable until proven otherwise.
const NASize_T cacheKeyInitStrLen = 256;
CacheWA::CacheWA(NAHeap *h)
  : heap_(h), qryText_(cacheKeyInitStrLen, h), actuals_(h)
  , sels_(h), parmTypes_(0), selTypes_(0)
  , cacheable_(FALSE), conditionallyCacheable_(FALSE)
  , tkey_(NULL), ckey_(NULL), phase_(CmpMain::END), topRoot_(NULL)
  , numberOfScans_(0), hasPredicate_(FALSE)
  , tabDescPtr_(0), usedKyPtr_(0), tabArraySize_(0)
  , predHasNoLit_(TRUE), reqdShape_("",h)
  , isoLvl_(TransMode::IL_NOT_SPECIFIED_)
  , isoLvlIDU_(TransMode::IL_NOT_SPECIFIED_)
  , accMode_(TransMode::AM_NOT_SPECIFIED_)
  , autoCmt_(TransMode::AC_NOT_SPECIFIED_)
  , flags_(0), isViewJoin_(FALSE), useView_(FALSE)
  , rbackMode_(TransMode::ROLLBACK_MODE_NOT_SPECIFIED_)
  , isUpdate_(FALSE)
  , HQCKey_(NULL)
  , numberOfExprs_(0)
  , tables_(h,1)
  , hasRewriteEnabledMV_(FALSE)
  , hasParameterizedPred_(FALSE)
  , posCounter_(0)
{
  requiredPrefixKeys_ = getDefaultAsLong(QUERY_CACHE_REQUIRED_PREFIX_KEYS);
}

// Free our allocated memory
CacheWA::~CacheWA()
{
  if (tkey_) { 
     NADELETE(tkey_,TextKey,heap_); 
     tkey_ = 0; 
  }
  if (ckey_) { 
     NADELETE(ckey_,CacheKey,heap_); 
     ckey_ = 0; 
  }
  if (parmTypes_) { 
    NADELETE(parmTypes_,ParameterTypeList,heap_); 
    parmTypes_ = 0; 
  }
  if (selTypes_) { 
    NADELETE(selTypes_,SelParamTypeList,heap_); 
    selTypes_ = 0; 
  }

  if (numberOfScans_ > 0) {
    heap_->deallocateMemory(tabDescPtr_);
    tabDescPtr_ = 0;

    // Release the memory used by the ValueIdSets.
    for (Int32 i = 0; i < numberOfScans_; i++)
      delete usedKyPtr_[i];
    heap_->deallocateMemory(usedKyPtr_);
    usedKyPtr_ = 0;
  }
}

// increment Expr counter and check if current query is still cacheable
NABoolean CacheWA::inc_N_check_still_cacheable()
{
  numberOfExprs_++;
  return numberOfExprs_ <= CmpCommon::getDefaultNumeric(QUERY_CACHE_MAX_EXPRS);
}

// increase the sizes of the scan arrays by one and insert a new TableDesc
// pointer into the table description array.
void CacheWA::incNofScans(TableDesc* td)
{
  // Increase the tabArraySize_ and usedKyPtr_ arrays if they are
  // empty or have used the entire buffer that was allocated during
  // a previous call to this function.
  if (numberOfScans_ == tabArraySize_)
  {
    // This increases the size that takes advantage of the internal
    // knowledge of NAMemory overhead, which is 4 bytes minimum.
    // The sizes of the arrays start at 3 elements, and increase
    // by 8 after that.
    if (tabArraySize_ == 0)
      tabArraySize_ = 3;
    else
      tabArraySize_ += 8;

    TableDescPtr *newT = (TableDescPtr*)heap_->allocateMemory(
             tabArraySize_ * sizeof(TableDescPtr)); 
    ValueIdSet **newK = (ValueIdSet**)heap_->allocateMemory(
             tabArraySize_ * sizeof(ValueIdSet*));

    // If previous arrays exist, then copy them to the new array and
    // free the old array
    if (numberOfScans_ != 0)
    {
      memcpy(newT, tabDescPtr_, numberOfScans_ * sizeof(TableDescPtr));
      memcpy(newK, usedKyPtr_, numberOfScans_ * sizeof(ValueIdSet*));
      heap_->deallocateMemory(tabDescPtr_);
      heap_->deallocateMemory(usedKyPtr_);
    }
    tabDescPtr_ = newT;
    usedKyPtr_ = newK;
  }

  tabDescPtr_[numberOfScans_] = td;
  usedKyPtr_[numberOfScans_] = new (heap_) ValueIdSet;
  numberOfScans_++;
}

void CacheWA::operator+=(const char* s)
{
   // If the current remaining space on qryText_ is less than
   // strlen(s)+1, we double the capacity for qryText_ in one call
   // to adjustMemory().

   size_t cap = qryText_.capacity();
   size_t len = strlen(s);
   if ( cap - qryText_.length() < len + 1) {
      qryText_.adjustMemory(MAXOF(len, 2 * cap));
   }

   qryText_.append(s, len);
}
  
// add referenced column's valueid to usedKeys
void CacheWA::addToUsedKeys(BaseColumn *base)
{
  for (Int32 x=0; x<numberOfScans_; x++) {
    if (tabDescPtr_[x] == base->getTableDesc()) {
      usedKyPtr_[x]->addElement(base->getValueId());
      return;
    }
  }
}

// is this column parameterizable?
NABoolean CacheWA::isParameterizable(BaseColumn *base)
{
  for (Int32 x=0; x<numberOfScans_; x++) {
    if (tabDescPtr_[x] == base->getTableDesc()) {
      // column is parameterizable if it's part of a predicate that 
      // specifies at least the requiredPrefixKeys_
      return usedKyPtr_[x]->coversFirstN
        (tabDescPtr_[x]->getClusteringIndex()->getClusteringKeyCols(), 
         requiredPrefixKeys_);
    }
  }
  return FALSE;
}

// add ConstantParameter to current query's actual parameters
void CacheWA::addConstParam(ConstantParameter* p, BindWA& bindwa)
{ 
  if (p && topRoot_ && phase_ >= CmpMain::BIND) {
    // we must bind "after-binder" ConstantParameters now and treat them as
    // "outer references" just like DynamicParams. If we're a cache miss,
    // our parent query will be fed to the normalizer, transformer, optimizer
    // which expect to see ConstantParameters as characteristic inputs.
    p = (ConstantParameter*)p->bindNode(&bindwa);
  }
  if (p) {
    sqlStmtConstParamPos_.insert(actuals_.entries() + sels_.entries());
    actuals_.insert(p);
    topRoot_->addInputVarTree(p);
  }
}

// add SelParameter to current query's selection parameters
void CacheWA::addSelParam(SelParameter* p, BindWA& bindwa)
{ 
  if (p && topRoot_ && phase_ >= CmpMain::BIND) {
    // we must bind "after-binder" ConstantParameters now and treat them as
    // "outer references" just like DynamicParams. If we're a cache miss,
    // our parent query will be fed to the normalizer, transformer, optimizer
    // which expect to see ConstantParameters as characteristic inputs.
    p = (SelParameter*)p->bindNode(&bindwa);
  }
  if (p) {
    sqlStmtSelParamPos_.insert(actuals_.entries() + sels_.entries());
    sels_.insert(p);
    topRoot_->addInputVarTree(p);
  }
}

// return current query's formal parameter types
const ParameterTypeList *CacheWA::getFormalParamTypes() 
{ 
  // parmTypes_ is always derived from actuals_; so, free any previous
  // parmTypes_ before deriving it again from actuals_
  if (parmTypes_) { NADELETE(parmTypes_,ParameterTypeList,wHeap()); }
  // save pointer in parmTypes_ so ~CacheWA() can free it
  parmTypes_ = new (wHeap()) ParameterTypeList(&actuals_, wHeap());
  return parmTypes_;
}

// return current query's formal SelParamTypes
const SelParamTypeList *CacheWA::getFormalSelParamTypes() 
{ 
  // selTypes_ is always derived from sels_; so, free any previous
  // selTypes_ before deriving it again from sels_
  if (selTypes_) { NADELETE(selTypes_,SelParamTypeList,wHeap()); }
  // save pointer in selTypes_ so ~CacheWA() can free it
  selTypes_ = new (wHeap()) SelParamTypeList(&sels_, wHeap());
  return selTypes_;
}

// compose and return TextKey of current query
TextKey* CacheWA::getTextKey(const char *sText, Lng32 charset,
                             const QryStmtAttributeSet& attrs)
{ 
  if (!tkey_) {
    // capture compiler environment for this query
    CompilerEnv *env = new (wHeap()) CompilerEnv
      (wHeap(), CmpMain::PREPARSE, attrs);

    // use query's formal parameter types, etc in creating its candidate Key
    // because the Key must hash to the same address as its cache entry.
    tkey_ = new (wHeap()) TextKey(sText, env, wHeap(), charset);
  }
  return tkey_; 
}

// compose and return cache key of current query
CacheKey* CacheWA::getCacheKey(const QryStmtAttributeSet& attrs)
{ 
  if (!ckey_) {
    // capture compiler environment for this query
    CompilerEnv *env = new (wHeap()) CompilerEnv(wHeap(), phase_, attrs);

    // use query's formal parameter types, etc in creating its candidate Key
    // because the Key must hash to the same address as its cache entry.
    ckey_ = new (wHeap()) CacheKey
      (qryText_, phase_, env, *getFormalParamTypes(), 
       *getFormalSelParamTypes(), wHeap(), reqdShape_, 
       isoLvl_, accMode_, isoLvlIDU_, autoCmt_, flags_, rbackMode_,
       tables_, useView_);
  }
  return ckey_; 
}

// caller wants us to remember an occurrence of a view join
void CacheWA::foundViewJoin() 
{
  if (!isViewJoin_) {
    isViewJoin_ = TRUE;
  }
}

// traverse queryExpr and put together its cacheKey
void CacheWA::generateCacheKey(RelExpr *queryExpr, const char *sText,
                               Lng32 charset, const NAString& viewsUsed)
{
  if (viewsUsed.length() > 0) {
    qryText_ += "v:";
    qryText_ += viewsUsed.data();
    useView_ = TRUE;
  }
  if (isViewJoin_) {
    // prepend view join's text into its cache key
    qryText_ += sText;
    char cs[20]; sprintf(cs, "%d", charset);
    qryText_ += cs;
  }
  // save parameterized statement text into cwa.qryText_
  queryExpr->generateCacheKey(*this);
}

// begin part of fix to join plan quality bug. Given a join 
//   select ... from v1, h2, a where ... and h2.id_level = a.id_level
//   and h2.id_level = '3' and a.id_level = '3' and ...
// query caching would normally replace the literals with parameters
//   ... h2.id_level = ?selParam1 and a.id_level = ?selParam2
// since the same plan can be shared by matching joins whose predicates
// have matching selectivities. But, in the above join, this
// parameterization of equality selection predicates has the unintended
// consequence of preventing a single VEG
//   ('3' = h2.id_level = a.id_level = '3')
// from forming. The result is an inferior hash join that reads 2M rows
// from h2 instead of a nested join that reads 29 rows from h2.
// When cqd MATCH_CONSTANTS_OF_EQUALITY_PREDICATES is ON, parameterization
// of equality selection predicates preserves matches
//   ... h2.id_level = ?selParam1 and a.id_level = ?selParam1

// helper functions:
// return true & matching element if val is in this list
NABoolean ConstantParameters::find
(ConstValue *val, ConstantParameter **match)
{
  CollIndex x, limit = entries();
  for (x = 0; x < limit; x++) {
    if (at(x)->matches(val)) {
      *match = at(x);
      return TRUE;
    }
  }
  return FALSE;
}

// return true & matching element if val is in this list
NABoolean SelParameters::find
(ConstValue *val, SelParameter **match)
{
  CollIndex x, limit = entries();
  for (x = 0; x < limit; x++) {
    if (at(x)->matches(val)) {
      *match = at(x);
      return TRUE;
    }
  }
  return FALSE;
}

// replace constant with new or existing ConstantParameter
void CacheWA::replaceWithNewOrOldConstParam
(ConstValue *val, const NAType *typ, ExprValueId& tgt, BindWA &bindWA)
{
  // match ConstantParameters only in complex queries
  Lng32 complexity = (Lng32)CmpCommon::getDefaultNumeric
    (MATCH_CONSTANTS_OF_EQUALITY_PREDICATES);
  if (complexity >= 0 && numberOfScans_ >= complexity) {
    ConstantParameter *cParm;
    if (actuals_.find(val, &cParm)) {
      cParm->addPosition(++posCounter_);
      tgt = cParm; // replace with existing ConstantParameter
      return;
    }
    if (sels_.find(val, (SelParameter**)&cParm)) {
      cParm->addPosition(++posCounter_);
      tgt = cParm; // replace with existing ConstantParameter
      return;
    }
  }
  // query is simple or there is no matching constant.
  // introduce new ConstantParameter.
  ConstantParameter *nParam = new (wHeap()) ConstantParameter
    (val, wHeap(), typ, ++posCounter_);
  addConstParam(nParam, bindWA);
  tgt = nParam;
}

// replace constant with new or existing SelParameter
void CacheWA::replaceWithNewOrOldSelParam
(ConstValue *val, const NAType *typ, const Selectivity sel, ExprValueId& tgt,
 BindWA &bindWA)
{
  // match SelParameters only in complex queries
  Lng32 complexity = (Lng32)CmpCommon::getDefaultNumeric
    (MATCH_CONSTANTS_OF_EQUALITY_PREDICATES);
  if (complexity >= 0 && numberOfScans_ >= complexity) {
    SelParameter *sParm;
    if (actuals_.find(val, (ConstantParameter**)&sParm)) {
      sParm->addPosition(++posCounter_);
      tgt = sParm; // replace with existing SelParameter
      return;
    }
    if (sels_.find(val, &sParm)) {
      sParm->addPosition(++posCounter_);
      tgt = sParm; // replace with existing SelParameter
      return;
    }
  }
  // query is simple or there is no matching constant.
  // introduce new SelParameter.
  SelParameter *nParam = new (wHeap()) SelParameter
    (val, wHeap(), typ, sel, ++posCounter_);
  addSelParam(nParam, bindWA);
  tgt = nParam;
}

