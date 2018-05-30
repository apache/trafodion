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
 *****************************************************************************
 *
 * File:         QCache.cpp
 * Description:  Methods for class QueryCache.
 *
 *
 * Created:      07/31/2000
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "QCache.h"
#include "CacheWA.h"
#include "CmpCommon.h"
#include "CmpMain.h"
#include "IntervalType.h"
#include "ComTdbRoot.h"
#include "ItemFunc.h"
#include "ValueDesc.h"
#include "SchemaDB.h"
#include "ControlDB.h"
#include "CmpErrLog.h"
#include "CompilerTracking.h"
#include "exp_clause_derived.h"
#include "NumericType.h"
#include "ComDistribution.h"

#ifdef DBG_QCACHE
#include <fstream>

fstream* filestream= NULL;

void logmsg(char* msg)
{
  if ( filestream )
     *filestream << msg << endl;
}

void logmsg(char* msg, ULong u)
{
  if ( filestream )
     *filestream << msg << ": " << u << endl;
}
#endif

ULng32 getDefaultInK(const Int32& key)
{
  return (ULng32)1024 * ActiveSchemaDB()->getDefaults().getAsLong(key);
}

// convert DefaultToken optimization level into OptLevel
CompilerEnv::OptLevel CompilerEnv::defToken2OptLevel(DefaultToken tok)
{
  switch (tok) {
  case DF_MINIMUM: return OPT_MINIMUM;
  case DF_MEDIUM_LOW: return OPT_MEDIUM_LOW;
  case DF_MEDIUM: return OPT_MEDIUM;
  case DF_MAXIMUM: return OPT_MAXIMUM;
  default:
    CMPASSERT(tok == DF_MINIMUM || tok == DF_MEDIUM_LOW ||
              tok == DF_MEDIUM || tok == DF_MAXIMUM);
    return (OptLevel)0;
  }
}

// Set current compiler environment's optimization level
void CompilerEnv::setOptimizationLevel(DefaultToken optLvl)
  // modifies: optLvl_
  // effects:  optLvl_ = translation of optLvl into an equivalent OptLevel
  // note: optLvl is not suitable for optimization level comparisons because
  //       it's ordinals are sorted alphabetically by token string
{
  optLvl_ = CompilerEnv::defToken2OptLevel(optLvl);
}

// constructor for control query default settings
CQDefaultSet::CQDefaultSet(Int32 n, NAHeap *h)
  : nEntries(0), heap(h), CQDarray(0), arrSiz(n)
{
  if (arrSiz > 0) {
    CQDarray = new (h) CQDefPtr[arrSiz];
    for (Int32 x = 0; x < arrSiz; x++) CQDarray[x] = NULL;
  }
}

// constructor for control query default settings
CQDefaultSet::CQDefaultSet(const CQDefaultSet& s, NAHeap* h)
  : nEntries(s.nEntries), heap(h), CQDarray(0), arrSiz(s.nEntries)
{
  if (nEntries > 0) {
    CQDarray = new (h) CQDefPtr[nEntries];
    for (Int32 x = 0; x < nEntries; x++) {
      CQDarray[x] = new (heap) CQDefault(*s.CQDarray[x], heap);
    }
  }
}

// destructor frees all control query default settings
CQDefaultSet::~CQDefaultSet()
{
  if (CQDarray) {
    for (Int32 x = 0; x < nEntries; x++) {
      NADELETE(CQDarray[x], CQDefault, heap);
    }
    NADELETEBASIC(CQDarray, heap);
  }
  CQDarray = NULL;
}

// add a control query default to CQDarray
void CQDefaultSet::addCQD(CQDefPtr cqd)
{
  CMPASSERT(nEntries < arrSiz && CQDarray != NULL);
  CQDarray[nEntries++] = cqd;
}

// return byte size of this CQDefaultSet
ULng32 CQDefaultSet::getByteSize() const
{
  ULng32 result = sizeof(*this) + arrSiz * sizeof(CQDefPtr);
  for (Int32 x = 0; x < nEntries; x++) {
    if (CQDarray[x]) {
      result += CQDarray[x]->getByteSize();
    }
  }
  return result;
}

// comparison method for sorting & searching CQDarray
Int32 CQDefaultSet::Compare(const void *d1, const void *d2)
{
  // cast (void *) pointers to (CQDefPtr*) which they must be
  CQDefPtr* def1 = (CQDefPtr*)d1;
  CQDefPtr* def2 = (CQDefPtr*)d2;

  CMPASSERT(def1 != NULL && def2 != NULL && *def1 != NULL && *def2 != NULL);

  // return -1 if def1 < def2, 0 if def1 == def2, +1 if def1 > def2
  return (*def1)->attr.compareTo((*def2)->attr);
}

// constructor for control table settings
CtrlTblSet::CtrlTblSet(Int32 n, NAHeap *h)
  : nEntries(0), heap(h), CTarray(0), arrSiz(n)
{
  if (arrSiz > 0) {
    CTarray = new (h) CtrlTblPtr[arrSiz];
    for (Int32 x = 0; x < arrSiz; x++) CTarray[x] = NULL;
  }
}

// copy constructor for control table settings
CtrlTblSet::CtrlTblSet(const CtrlTblSet& s, NAHeap *h)
  : nEntries(s.nEntries), heap(h), CTarray(0), arrSiz(s.nEntries)
{
  if (nEntries > 0) {
    CTarray = new (h) CtrlTblPtr[nEntries];
    for (Int32 x = 0; x < nEntries; x++) {
      CTarray[x] = new (heap) CtrlTblOpt(*s.CTarray[x], heap);
    }
  }
}

// destructor frees all control table settings
CtrlTblSet::~CtrlTblSet()
{
  if (CTarray) {
    for (Int32 x = 0; x < nEntries; x++) {
      NADELETE(CTarray[x], CtrlTblOpt, heap);
    }
    heap->deallocateMemory(CTarray);
  }
  CTarray = NULL;
}

// add a control table setting to CTarray
void CtrlTblSet::addCT(CtrlTblPtr ct)
{
  CMPASSERT(nEntries < arrSiz && CTarray != NULL);
  CTarray[nEntries++] = ct;
}

// return byte size of this CtrlTblSet
ULng32 CtrlTblSet::getByteSize() const
{
  ULng32 result = sizeof(*this) + arrSiz * sizeof(CtrlTblPtr);
  for (Int32 x = 0; x < nEntries; x++) {
    result += CTarray[x]->getByteSize();
  }
  return result;
}

// comparison method for sorting & searching CTarray
Int32 CtrlTblSet::Compare(const void *t1, const void *t2)
{
  // cast (void *) pointers to (CtrlTblPtr*) which they must be
  CtrlTblPtr* opt1 = (CtrlTblPtr*)t1;
  CtrlTblPtr* opt2 = (CtrlTblPtr*)t2;

  CMPASSERT(opt1 != NULL && opt2 != NULL && *opt1 != NULL && *opt2 != NULL);

  // return -1 if opt1 < opt2, 0 if opt1 == opt2, +1 if opt1 > opt2
  Int32 result = (*opt1)->tblNam.compareTo((*opt2)->tblNam);
  return result ? result : (*opt1)->attr.compareTo((*opt2)->attr);
}

const NASize_T catalogInitStrLen = 8;
const NASize_T schemaInitStrLen = 8;

// constructor captures query compiler environment for current query
CompilerEnv::CompilerEnv(NAHeap *h, CmpPhase phase, 
                         const QryStmtAttributeSet& atts)
  : heap_(h), cat_(catalogInitStrLen,h), schema_(schemaInitStrLen,h),
    CQDset_(0), CTset_(0), attrs_(atts)
{
  // entry must include context-wide TransMode in its CompilerEnv
  // because CmpMain.cpp's post-generator QueryCache::addEntry call
  // unconditionally uses the same key (and therefore same CompilerEnv)
  // to add both preparser and postparser entries into the cache.
  tmode_ = new (heap_) TransMode();
  tmode_->updateTransMode(CmpCommon::transMode());

  // record current optimization level
  setOptimizationLevel(CmpCommon::getDefault(OPTIMIZATION_LEVEL, 0));

  // record catalog and schema settings
  CmpCommon::getDefault(CATALOG, cat_, 0);
  CmpCommon::getDefault(SCHEMA, schema_, 0);

  // record any explicit control query default settings
  CollIndex x, cnt;
  ControlDB *cdb = ActiveControlDB();
  if (cdb) {
    cnt = cdb->getCQDList().entries();
    if (cnt > 0) {
      CQDset_ = new (heap_) CQDefaultSet((Int32)cnt, heap_);
      for (x = 0; x < cnt; x++) {
        ControlQueryDefault *cqd = cdb->getCQDList()[x];
        switch (cqd->getAttrEnum()) {
          // skip these CQD settings -- they don't affect plan quality
        case COMPILE_TIME_MONITOR:
        case COMPILE_TIME_MONITOR_OUTPUT_FILE:        
        case COMPILER_TRACKING_INTERVAL:
        case COMPILER_TRACKING_LOGFILE:
        case NSK_DBG:
        case NSK_DBG_LOG_FILE:
        case NSK_DBG_PRINT_CHAR_INPUT:
        case NSK_DBG_PRINT_CHAR_OUTPUT:
        case NSK_DBG_PRINT_CONSTRAINT:
        case NSK_DBG_PRINT_CONTEXT:
        case NSK_DBG_PRINT_COST:
        case NSK_DBG_PRINT_ITEM_EXPR:
        case NSK_DBG_PRINT_LOG_PROP:
        case NSK_DBG_PRINT_PHYS_PROP:
        case NSK_DBG_PRINT_TASK:
        case NSK_DBG_SHOW_PASS1_PLAN:
        case NSK_DBG_SHOW_PASS2_PLAN:
        case NSK_DBG_SHOW_TREE_AFTER_BINDING:
        case NSK_DBG_SHOW_TREE_AFTER_CODEGEN:
        case NSK_DBG_SHOW_TREE_AFTER_NORMALIZATION:
        case NSK_DBG_SHOW_TREE_AFTER_SEMANTIC_QUERY_OPTIMIZATION:
        case NSK_DBG_QUERY_LOGGING_ONLY:
        case NSK_DBG_SHOW_TREE_AFTER_PARSING:
        case NSK_DBG_SHOW_TREE_AFTER_PRE_CODEGEN:
        case NSK_DBG_SHOW_TREE_AFTER_TRANSFORMATION:
        case QUERY_CACHE:
        case QUERY_CACHE_AVERAGE_PLAN_SIZE:
        case QUERY_CACHE_SELECTIVITY_TOLERANCE:
        case QUERY_CACHE_MAX_VICTIMS:
        case QUERY_CACHE_REQUIRED_PREFIX_KEYS:
        case QUERY_CACHE_STATEMENT_PINNING:
        case QUERY_CACHE_STATISTICS:
        case QUERY_CACHE_STATISTICS_FILE:
        case QUERY_TEMPLATE_CACHE:
        case QUERY_TEXT_CACHE:
        case SHARE_TEMPLATE_CACHED_PLANS: 
          break;
          // skip these CQD settings -- they are represented in CacheKey
        case CATALOG:
        case OPTIMIZATION_LEVEL:
        case SCHEMA:
          break;
          // record all other CQD settings
        case ISOLATION_LEVEL:
          if (phase != CmpMain::PREPARSE) break;
          // else PREPARSE doubly represent this CQD setting in 
          // CacheKey & CompilerEnv to guard against false
          // preparser hits that differ only in this CQD setting
        default:
          CQDset_->addCQD
            (new (heap_) CQDefault(cqd->getToken(), cqd->getValue(), heap_));
          break;
        }
      }
    }

    // record any control table settings
    cnt = cdb->getCTList().entries();
    if (cnt > 0) {
      // determine total number of control table settings
      Int32 totalEntries=0;
      for (x = 0; x < cnt; x++) {
        totalEntries += cdb->getCTList()[x]->numEntries();
      }
      CTset_ = new (heap_) CtrlTblSet(totalEntries, heap_);
      for (x = 0; x < cnt; x++) {
        ControlTableOptions *cto = cdb->getCTList()[x];
        CollIndex j, nEntries = cto->numEntries();
        for (j = 0; j < nEntries; j++) {
          CTset_->addCT
            (new (heap_) CtrlTblOpt
             (cto->tableName(), cto->getToken(j), cto->getValue(j), heap_));
        }
      }
    }
  }
}

// copy constructor is used when a CacheKey is copied into the query cache
CompilerEnv::CompilerEnv(const CompilerEnv &s, NAHeap *h)
  : optLvl_(s.optLvl_), cat_(s.cat_.data(),h), schema_(s.schema_.data(),h),
    heap_(h), CQDset_(0), CTset_(0), tmode_(0), attrs_(s.attrs_)
{
  if (s.tmode_) { 
    // deep copy s' transmode setting
    tmode_ = new (heap_) TransMode();
    tmode_->updateTransMode(s.tmode_);
  }

  if (s.CQDcnt() > 0) {
    // deep copy s' control query default settings
    CQDset_ = new (heap_) CQDefaultSet(*s.CQDset_, heap_);

    // sort control query default settings
    qsort(CQDset_->CQDarray, CQDset_->nEntries, sizeof(CQDefPtr),
          CQDefaultSet::Compare);
  }

  if (s.CTcnt() > 0) {
    // deep copy s' control table settings
    CTset_ = new (heap_) CtrlTblSet(*s.CTset_, heap_);

    // sort control table settings
    qsort(CTset_->CTarray, CTset_->nEntries, sizeof(CtrlTblPtr),
          CtrlTblSet::Compare);
  }
}

// free all allocated memory
CompilerEnv::~CompilerEnv()
{
  if (tmode_) {
    NADELETE(tmode_, TransMode, heap_);
    tmode_ = NULL;
  }
  if (CQDset_) {
    NADELETE(CQDset_, CQDefaultSet, heap_);
    CQDset_ = NULL;
  }
  if (CTset_) {
    NADELETE(CTset_, CtrlTblSet, heap_);
    CTset_ = NULL;
  }
}

// return byte size of this CompilerEnv
ULng32 CompilerEnv::getSize() const
{
  ULng32 result = sizeof(*this) + cat_.getAllocatedSize() + schema_.getAllocatedSize();

  // add size of s' TransMode setting
  if (tmode_) {
    result += tmode_->getByteSize();
  }

  // add size of s' control query default settings
  if (CQDset_) {
    result += CQDset_->getByteSize();
  }

  // add size of s' control table settings
  if (CTset_) {
    result += CTset_->getByteSize();
  }
  return result;
}

// returns TRUE if cached plan's environment is better or as good as other's
NABoolean CompilerEnv::isEqual(const CompilerEnv &other, CmpPhase phase)
const
  // requires: "this" is cached plan's environment
  //           "other" is the query that CmpMain::sqlcomp is compiling
  //           this order of comparison is determined by the order of
  //           comparison used in NAHashBucket<K,V>::getFirstValue()
  // effects:  return TRUE if cached plan's environment will produce a plan
  //           that is better or as good as the current compiler environment
{
  NABoolean strict = phase==CmpMain::PREPARSE || phase==CmpMain::PARSE;
  NABoolean result = optLvl_ >= other.optLvl_ &&
    (strict ? (cat_ == other.cat_ && schema_ == other.schema_) : TRUE);
  if (!result) return FALSE;

  // both must have the same TransMode for preparser cache entries
  if (phase == CmpMain::PREPARSE||phase == CmpMain::PARSE) {
    if (tmode_ && other.tmode_) {
      if (!(*tmode_ == *other.tmode_)) {
        return FALSE; // env mismatch
      }
      // else env match so fall thru & compare rest
    }
    else {
      CMPASSERT(tmode_!=NULL && other.tmode_!=NULL); // should never be
      return FALSE; // env mismatch
    }
  }

  // both must have same number of statement attributes & parserFlags
  Int32 nAttrs, x, nCQD, nCT;
  if ((nAttrs=attrs_.nEntries) != other.attrs_.nEntries) {
    return FALSE;
  }
  if (nAttrs) {
    for (x = 0; x < nAttrs; x++) {
      if (!(attrs_.has(other.attrs_.at(x)))) {
        return FALSE; // a statement attribute or parserFlags mismatch
      }
    }
  }

  // both must have the same number of control settings
  if ((nCQD=CQDcnt()) != other.CQDcnt() || (nCT=CTcnt()) != other.CTcnt()) {
    return FALSE;
  }

  if (nCQD) {
    // cached entry's control query default settings are in ascending order.
    // use binary search to look up other's settings in cached entry's CQDset_.
    for (x = 0; x < nCQD; x++) {
      CQDefPtr *cdef, *odef = &(other.CQDset_->CQDarray[x]);
      cdef = (CQDefPtr*)bsearch(odef, CQDset_->CQDarray, nCQD,
                                sizeof(CQDefPtr), CQDefaultSet::Compare);
      if (!cdef || *cdef==NULL || (*cdef)->value != (*odef)->value) {
        return FALSE; // a control query default mismatch
      }
    }
  }

  if (nCT) {
    // cached entry's control table settings are in ascending order.
    // use binary search to look up other's settings in cached entry's CTset_.
    for (x = 0; x < nCT; x++) {
      CtrlTblPtr *ctopt, *otopt = &(other.CTset_->CTarray[x]);
      ctopt = (CtrlTblPtr*)bsearch(otopt, CTset_->CTarray, nCT,
                                   sizeof(CtrlTblPtr), CtrlTblSet::Compare);
      if (!ctopt || *ctopt==NULL || (*ctopt)->value != (*otopt)->value) {
        return FALSE; // a control table mismatch
      }
    }
  }

  return result; // entries are equal
}

// compute hash address of this CompilerEnv
ULng32 CompilerEnv::hashKey() const
{
  // ignore optLvl_ (and controls) because we want 2 cache entries that
  // differ only in optLvl_ (and controls) to hash to the same address.
  // controls don't contribute to the hash for simplicity (and also because
  // controls for cached entries are sorted whereas controls for incoming
  // entry are not sorted and an old hash for controls was order-sensitive).
  return (cat_.hash() << 1) + schema_.hash();
}

// return true iff it is safe to call CompileEnv::hashKey on me
NABoolean CompilerEnv::amSafeToHash() const
{
  return cat_.data() != NULL && schema_.data() != NULL;
}

// constructor used by CacheWA::getKey() to construct a query's CacheKey
ParameterTypeList::ParameterTypeList(ConstantParameters *p, NAHeap *h)
  : LIST(ParamType)(h, p->entries())
  , heap_(NULL) // we don't own the elements
{
  // insert each ConstantParameter's type into this list
  CollIndex x, limit = (*p).entries();
  for (x=0; x < limit; x++) {
    NAType *typ = CONST_CAST(NAType*,(*p)[x]->getType());
    ParamType pTyp(typ, (*p)[x]->getPositionSet()); 
    insert(pTyp);
  }
}

// constructor used by QCache::addEntry(KeyDataPair& p) to transfer ownership
// of a new cache entry's memory from statementHeap to QCache::heap_
ParameterTypeList::ParameterTypeList(const ParameterTypeList& s, NAHeap *h)
  : LIST(ParamType)(h, s.entries())
  , heap_(h) // we do own all elements to be copied below
{
  CMPASSERT(heap_ != NULL);
  // insert a copy of each element into this list
  CollIndex x, limit = s.entries();
  for (x=0; x < limit; x++) {
    PositionSet *posns = new(h) PositionSet(*s.at(x).posns_, h);
    ParamType pTyp(s.at(x).type_->newCopy(h), posns);
    insert(pTyp);
  }
}

// free memory used by this ParameterTypeList if it's owned by QCache::heap_
ParameterTypeList::~ParameterTypeList()
{
  if (heap_) {
    // we do own the elements, so we must free them now
    CollIndex x, limit = entries();
    for (x=0; x < limit; x++) {
      NADELETE(at(x).type_,NAType,heap_);
      NADELETE(at(x).posns_,PositionSet,heap_);
    }
  }
  clear();
}

// return this ParameterTypeList's contribution to a CacheKey's hash value
ULng32 ParameterTypeList::hashKey() const
{
  ULng32 result = 0;
  CollIndex x, limit = entries();
  for (x=0; x < limit; x++) {
    result = (result << 1) + (*this)[x].type_->hashKey();
  }
  return result;
}

// return true iff it is safe to call ParameterTypeList::hashKey on me
NABoolean ParameterTypeList::amSafeToHash() const
{
  CollIndex x, limit = entries();
  for (x=0; x < limit; x++) {
    if ( (*this)[x].type_->amSafeToHash() != TRUE )
      return FALSE; // any unsafe entry makes me unsafe
  }
  return TRUE; // I am safe, I have no unsafe entry
}

// return this ParameterTypeList's elements' total byte size
ULng32 ParameterTypeList::getSize() const
{
  CollIndex x, limit = entries();
  ULng32 result = limit * sizeof(NAType*); // amount of space allocated
                                          // for the array of type NAType*.
  for (x=0; x < limit; x++) {
    result += (*this)[x].type_->getSize();
  }
  return result;
}

// return TRUE iff "other" can be safely coerced to "this" at compile-time
NABoolean ParameterTypeList::operator==(const ParameterTypeList& other) const
  // requires: "other" is the type signature of the query that's being
  //             compiled by CmpMain::sqlcomp
  //           "this" is the type signature of a cached plan
  // effects:  return TRUE iff "other" can be safely backpatched into "this"
{
  CollIndex nParms = entries();
  if (nParms != other.entries()) {
    return FALSE;
  }
  const NABoolean STRICT_CHK=FALSE;
  NABoolean typeEqual;
  for (CollIndex i = 0; i < nParms; i++) {
    typeEqual = (at(i).type_ == other.at(i).type_);
    if (!typeEqual && // if types are equal don't check any further
	(!(at(i).type_->isCompatible(*other.at(i).type_)) ||
	 other.at(i).type_->errorsCanOccur(*at(i).type_, STRICT_CHK))) {
      return FALSE;
    }
    if (*at(i).posns_ != *other.at(i).posns_) 
    {
      return FALSE;
    }
  }
  return TRUE;
}

const NASize_T parmTypesInitStrLen = 256;

// deposit parameter types string form into parameterTypes
void ParameterTypeList::depositParameterTypes(NAString* parameterTypes) const
{
  CollIndex limit = entries();
  if (limit > 0) {
    *parameterTypes += (*this)[0].type_->getTypeSQLname().data();
    for (CollIndex x=1; x < limit; x++) {
      *parameterTypes += ",";
      *parameterTypes += (*this)[x].type_->getTypeSQLname().data();
    }
  }
}

// constructor used by CacheWA::getCacheKey() to construct a query's CacheKey
SelParamTypeList::SelParamTypeList(SelParameters *p, NAHeap *h)
  : LIST(SelParamType)(h, p->entries())
  , heap_(NULL) // we don't own the elements
{
  // insert each SelParameter's type & selectivity into this list
  CollIndex x, limit = (*p).entries();
  for (x=0; x < limit; x++) {
    NAType *typ = CONST_CAST(NAType*,(*p)[x]->getType());
    SelParamType sTyp(typ, (*p)[x]->getSelectivity(), (*p)[x]->getPositionSet());
    insert(sTyp);
  }
}

// constructor used by QCache::addEntry(KeyDataPair& p) to transfer ownership
// of a new cache entry's memory from statementHeap to QCache::heap_
SelParamTypeList::SelParamTypeList(const SelParamTypeList& s, NAHeap *h)
  : LIST(SelParamType)(h, s.entries())
  , heap_(h) // we do own all elements to be copied below
{
  CMPASSERT(heap_ != NULL);
  // insert a copy of each element into this list
  CollIndex x, limit = s.entries();
  for (x=0; x < limit; x++) {
    PositionSet *posns = new(h) PositionSet(*s[x].posns_, h);
    SelParamType sTyp(s[x].type_->newCopy(h), s[x].sel_, posns);
    insert(sTyp);
  }
}

// free memory used by this SelParamTypeList if it's owned by QCache::heap_
SelParamTypeList::~SelParamTypeList()
{
  if (heap_) {
    // we do own the elements, so we must free them now
    CollIndex x, limit = entries();
    for (x=0; x < limit; x++) {
      NADELETE(at(x).type_,NAType,heap_);
      NADELETE(at(x).posns_,PositionSet,heap_);
    }
  }
  clear();
}

// return this SelParamTypeList's contribution to a CacheKey's hash value
ULng32 SelParamTypeList::hashKey() const
{
  ULng32 result = 0;
  CollIndex x, limit = entries();
  for (x=0; x < limit; x++) {
    result = (result << 1) + (*this)[x].type_->hashKey();
  }
  return result;
}

// return true iff it is safe to call SelParamTypeList::hashKey on me
NABoolean SelParamTypeList::amSafeToHash() const
{
  CollIndex x, limit = entries();
  for (x=0; x < limit; x++) {
    if ( (*this)[x].type_->amSafeToHash() != TRUE )
      return FALSE; // any unsafe entry makes me unsafe
  }
  return TRUE; // I am safe, I have no unsafe entry
}

// return this SelParamTypeList's elements' total byte size
ULng32 SelParamTypeList::getSize() const
{
  CollIndex x, limit = entries();
  ULng32 result = limit * sizeof(SelParamType);
  for (x=0; x < limit; x++) {
    result += (*this)[x].type_->getSize();
  }
  return result;
}

// return TRUE iff "other" can be safely coerced to "this" at compile-time
NABoolean SelParamTypeList::compareParamTypes(const SelParamTypeList& other) const
  // requires: "other" is the type signature of the query that's being
  //             compiled by CmpMain::sqlcomp
  //           "this" is the type signature of a cached plan
  // effects:  return TRUE iff "other" can be safely backpatched into "this"
{
  CollIndex nParms = entries();
  if (nParms != other.entries()) {
    return FALSE;
  }
  for (CollIndex i = 0; i < nParms; i++) {
    const NABoolean STRICT_CHK=FALSE;
    if (NOT (at(i).type_->isCompatible(*other.at(i).type_)) ||
        other.at(i).type_->errorsCanOccur(*at(i).type_, STRICT_CHK) ) 
    {
      return FALSE;
    }
    if (*at(i).posns_ != *other.at(i).posns_)
    {
      return FALSE;
    }
  }
  return TRUE;
}

NABoolean SelParamTypeList::compareSelectivities(const SelParamTypeList& other) const
{
  CollIndex nParms = entries();
  if (nParms != other.entries()) {
    return FALSE;
  }
  for (CollIndex i = 0; i < nParms; i++) {
    if (NOT (at(i).sel_ == other.at(i).sel_)) {
      return FALSE;
    }
  }
  return TRUE;
}

NABoolean SelParamTypeList::operator==(const SelParamTypeList& other) const
{
  // requires: "other" is the type signature of the query that's being
  //             compiled by CmpMain::sqlcomp
  //           "this" is the type signature of a cached plan
  // effects:  return TRUE iff "other" can be safely backpatched into "this"
  return ( compareParamTypes(other) && compareSelectivities(other) );
}

// deposit parameter types string form into parameterTypes
void SelParamTypeList::depositParameterTypes(NAString* parameterTypes) const
{
  CollIndex limit = entries();
  if (limit > 0) {
    *parameterTypes += (*this)[0].type_->getTypeSQLname().data();
    for (CollIndex x=1; x < limit; x++) {
      *parameterTypes += ",";
      *parameterTypes += (*this)[x].type_->getTypeSQLname().data();
    }
  }
}

// constructor
Key::Key(CmpPhase phase, CompilerEnv* e, NAHeap *h)
  : phase_(phase), heap_(h), env_(e)
{
}

// copy constructor
Key::Key(const Key &s, NAHeap *h)
  : phase_(s.phase_), heap_(h), env_(0)
{
  if (s.env_) {
    env_ = new (heap_) CompilerEnv(*s.env_, heap_);
  }
}

// free memory allocated by this Key
Key::~Key()
{
  if (env_) {
    NADELETE(env_, CompilerEnv, heap_);
    env_ = NULL;
  }
  // all other data members except heap_ are in-line;
  // their destructors are called implicitly. we don't own heap_.
}

// return TRUE iff "this" Key matches "other" Key. This operator
// is used (and required) by the NAHashDictionary template class.
NABoolean Key::isEqual(const Key &other) const
{
  return phase_ == other.phase_ &&
    (env_ == other.env_ ||
     (env_ && other.env_ &&
      (*env_).isEqual(*other.env_, phase_)));
}

Int32 Key::getOptLvl() const
{
  CMPASSERT(env_ != NULL);
  return env_ ? env_->getOptLvl() : CompilerEnv::OPT_UNDEFINED;
}

const char *Key::getCatalog() const
{
  CMPASSERT(env_ != NULL);
  return env_ ? env_->getCatalog() : "";
}

const char *Key::getSchema() const
{
  CMPASSERT(env_ != NULL);
  return env_ ? env_->getSchema() : "";
}

// return this Key's total size in bytes
ULng32 Key::getSize() const
{
  return sizeof(*this) + (env_ ? env_->getSize() : 0);
}

// return hash value of this Key; used by hashKeyFunc() which is called
// by NAHashDictionary<K,V>::getHashCode() to compute a key's hash address.
ULng32 Key::hashKey() const
{
  ULng32 hval = (phase_ << 1);
  if (env_) {
    hval = (hval << 1) + env_->hashKey();
  }
  return hval;
}

// return true iff it is safe to call Key::hashKey on me
NABoolean Key::amSafeToHash() const
{
  return env_ ? env_->amSafeToHash() : TRUE;
}

// constructor used by CacheWA::getKey() to construct a query's CacheKey
CacheKey::CacheKey(NAString &stmt, CmpPhase phase, CompilerEnv* e,
                   const ParameterTypeList& p, const SelParamTypeList& s, 
                   NAHeap *h, NAString &cqs,
                   TransMode::IsolationLevel l, TransMode::AccessMode m,
                   TransMode::IsolationLevel lu, 
                   TransMode::AutoCommit a, Int16 f, 
                   TransMode::RollbackMode r,
                   LIST(NATable*) &tables, NABoolean useView)
  : Key(phase, e, h), stmt_(stmt,h), actuals_(p,h), sels_(s,h)
  , isoLvl_(l), accMode_(m), isoLvlIDU_(lu)
  , autoCmt_(a), flags_(f), rbackMode_(r)
  , reqdShape_(cqs,h), compareSelectivity_(TRUE)
  , updateStatsTime_(h,tables.entries())
  , useView_(useView)
  , planId_(-1)
{
  // get & save referenced tables' histograms' timestamps
  updateStatsTimes(tables);
}

// constructor used by QCache::addEntry(KeyDataPair& p) to transfer ownership
// of a new cache entry's memory from statementHeap to QCache::heap_
CacheKey::CacheKey(CacheKey &s, NAHeap *h)
  : Key(s, h), stmt_(s.stmt_.data(),h)
  , actuals_(s.actuals_,h), sels_(s.sels_,h), reqdShape_(s.reqdShape_,h)
  , isoLvl_(s.isoLvl_), accMode_(s.accMode_) , isoLvlIDU_(s.isoLvlIDU_)
  , autoCmt_(s.autoCmt_) , flags_(s.flags_)
  , rbackMode_(s.rbackMode_), compareSelectivity_(TRUE)
  , updateStatsTime_(s.updateStatsTime_,h)
  , useView_(s.useView_)
  , planId_(s.planId_)
{
}

// free memory allocated by this CacheKey
CacheKey::~CacheKey()
{
}

// update referenced tables' histograms' timestamps
void CacheKey::updateStatsTimes( LIST(NATable*) &tables )
{
  // get & save referenced tables' histograms' timestamps
  CollIndex x, limit = tables.entries();
  NABoolean isEmpty = updateStatsTime_.isEmpty();
  for (x = 0; x < limit; x++) {
    if (isEmpty)
      updateStatsTime_.insert(tables[x]->getStatsTime());
    else
      updateStatsTime_[x] = tables[x]->getStatsTime();
  }
}

// return TRUE iff "this" CacheKey matches "other" CacheKey. This operator
// is used (and required) by the NAHashDictionary template class.
NABoolean CacheKey::operator ==(const CacheKey &other) const
{
  // the 'other' is the key and 'this' is one of the items in 
  // the hash dictionary. 
  if ( Key::isEqual(other) && 
    isoLvl_ == other.isoLvl_ && accMode_ == other.accMode_ &&
    isoLvlIDU_ == other.isoLvlIDU_ && 
    autoCmt_ == other.autoCmt_ && flags_ == other.flags_  &&
    rbackMode_ == other.rbackMode_ && reqdShape_ == other.reqdShape_ &&
    stmt_ == other.stmt_ && actuals_ == other.actuals_ &&
    sels_.compareParamTypes(other.sels_) &&
    updateStatsTime_ == other.updateStatsTime_) 
  {
    if ( other.compareSelectivity_ ) 
      return sels_.compareSelectivities(other.sels_);
    else
      return TRUE;
  }
  return FALSE;
}

// return this CacheKey's total size in bytes
ULng32 CacheKey::getSize() const
{
  ULng32 x = sizeof(*this) - sizeof(Key) + Key::getSize() +
         stmt_.getAllocatedSize() + actuals_.getSize() + sels_.getSize() + 
         reqdShape_.getAllocatedSize();

  return x;
}

// return hash value of this CacheKey; used by hashKeyFunc() which is called
// by NAHashDictionary<K,V>::getHashCode() to compute a key's hash address.
ULng32 CacheKey::hashKey() const
{
  ULng32 hval = Key::hashKey() + stmt_.hash();
  hval = (hval << 1) + reqdShape_.hash();
  hval = (hval << 1) + actuals_.hashKey();
  hval = (hval << 1) + sels_.hashKey();
  hval = (hval << 1) + isoLvl_;
  hval = (hval << 1) + accMode_;
  hval = (hval << 1) + isoLvlIDU_;
  hval = (hval << 1) + autoCmt_;
  hval = (hval << 1) + flags_;
  hval = (hval << 1) + rbackMode_;
  return hval;
}

// return true iff it is safe to call CacheKey::hashKey on me
NABoolean CacheKey::amSafeToHash() const
{
  return 
    Key::amSafeToHash() && 
    stmt_.data() != NULL && 
    reqdShape_.data() != NULL && 
    actuals_.amSafeToHash() &&
    sels_.amSafeToHash();
}

// return cache entry's parameters as a string for querycache() virtual table
const char *CacheKey::getParameterTypes(NAString* parameterTypes) const
{
  *parameterTypes = "";
  actuals_.depositParameterTypes(parameterTypes);
  sels_.depositParameterTypes(parameterTypes);
  return parameterTypes->data();
}

// constructor used by CacheWA::getKey() to construct a query's TextKey
TextKey::TextKey(const char *sText, CompilerEnv* e, NAHeap *h, Lng32 cSet)
  : Key(CmpMain::PREPARSE, e, h), sText_(sText, h), charset_(cSet)
{
}

// constructor used by QCache::addEntry(KeyDataPair& p) to transfer ownership
// of a new cache entry's memory from statementHeap to QCache::heap_
TextKey::TextKey(TextKey &s, NAHeap *h)
  : Key(s, h), sText_(s.sText_.data(),h), charset_(s.charset_)
{
}

// free memory allocated by this TextKey
TextKey::~TextKey()
{
}

// return TRUE iff "this" TextKey matches "other" TextKey. This operator
// is used (and required) by the NAHashDictionary template class.
NABoolean TextKey::operator ==(const TextKey &other) const
{
  return Key::isEqual(other) && sText_ == other.sText_ && 
    charset_ == other.charset_;
}

// return this TextKey's total size in bytes
ULng32 TextKey::getSize() const
{
  return Key::getSize() + sText_.length();
}

// return hash value of this TextKey; used by hashKeyFunc() which is called
// by NAHashDictionary<K,V>::getHashCode() to compute a key's hash address.
ULng32 TextKey::hashKey() const
{
  return Key::hashKey() + sText_.hash() + charset_;
}

// return true iff it is safe to call TextKey::hashKey on me
NABoolean TextKey::amSafeToHash() const
{
  return Key::amSafeToHash() && sText_.data() != NULL;
}

// return hash value of a CacheKey; this is called (and required) by
// NAHashDictionary<K,V>::getHashCode() to compute a key's hash address.
static ULng32
hashHQCKeyFunc(const HQCCacheKey& key)
{
  return key.hashKey();
}
// return hash value of a CacheKey; this is called (and required) by
// NAHashDictionary<K,V>::getHashCode() to compute a key's hash address.
static ULng32
hashKeyFunc(const CacheKey& key)
{
  // this function must have the following properties:
  // 1) very fast
  // 2) hashKey(k1) == hashKey(k2) if k1 == k2
  // 3) hashKey(k1) <> hashKey(k2) if k1 <> k2 (for as many cases as possible)
  return key.hashKey();
}

// return hash value of a TextKey; this is called (and required) by
// NAHashDictionary<K,V>::getHashCode() to compute a key's hash address.
static ULng32
hashTextFunc(const TextKey& key)
{
  // this function must have the following properties:
  // 1) very fast
  // 2) hashKey(k1) == hashKey(k2) if k1 == k2
  // 3) hashKey(k1) <> hashKey(k2) if k1 <> k2 (for as many cases as possible)
  return key.hashKey();
}

Plan::Plan(Plan&s, NAHeap *h) : plan_(s.plan_), planL_(s.planL_), planId_(s.planId_), heap_(h), refCount_(s.refCount_), visits_(s.visits_)
{
   if (s.plan_) {
     CMPASSERT(h != NULL);

    // If this a contiguous pack plan, copy it...
    //
    if (planL_ > 0) {
      plan_ = new (h) char[planL_];
      str_cpy_all(plan_, s.plan_, (Lng32)planL_);
    } else {
      // Otherwise, this is a pointer to the generator that has access
      // to the plan.  Convert to a contiguous packed plan on copy.
      //
      planL_ = ((Generator *)plan_)->getFinalObjLength();
      char *packedPlan = new (h) char[planL_];

      // Zero out the entire buffer to guarantee all filler fields get
      // value zero.
      memset(packedPlan, 0, planL_);
      plan_ = ((Generator *)plan_)->getFinalObj(packedPlan, planL_);
    }
   }
}

// constructor 
CData::CData(NAHeap *h)
  : hits_(0), heap_(h), compTime_(0), cumHitTime_(0)
{
}

const Int32 initialTextPtrArrayLen=10;

// constructor used by CmpMain::sqlcomp on a cache miss to create a new
// cache entry of a compiled plan for possible addition into the cache.
CacheData::CacheData(Generator *plan, const ParameterTypeList& f,
                     const SelParamTypeList& s,
                     LIST(Int32) hqcParamPos, LIST(Int32) hqcSelPos, LIST(Int32) hqcConstPos,
                     Int64 planId, const char *text, Lng32 cs, NAHeap *h)
  : CData(h), formals_(f,h), fSels_(s,h), hqcListOfConstParamPos_(hqcParamPos, h), 
    hqcListOfSelParamPos_(hqcSelPos, h), hqcListOfConstPos_ (hqcConstPos, h),
    origStmt_((char*)text), textentries_(h, initialTextPtrArrayLen)
{
  plan_ = new (h) Plan(plan, planId, h);
}

// constructor 
TextData::TextData(NAHeap *h, char *p, ULng32 l, CacheEntry *e)
  : CData(h), entry_(e), actLen_(l), indexInTextEntries_(0), heap_(h)
{
  // make a copy of the actual parameters because these constants
  // will be destroyed when the cache miss query compilation completes.
  actuals_ = new(h) char[l];
  memcpy(actuals_, p, l);
}

// copy constructor used to transfer ownership of a cache entry's memory 
// from the statementHeap to QCache::heap_.
CData::CData(CData &s, NAHeap *h)
  : hits_(s.hits_), heap_(h)
  , compTime_(s.compTime_), cumHitTime_(s.cumHitTime_)
{
}

// constructor used by QCache::addPostParserEntry(KeyDataPair& p) to transfer 
// ownership of a cache entry's memory from the statementHeap to QCache::heap_.
CacheData::CacheData(CacheData &s, NAHeap *h, NABoolean sharePlan)
  : CData(s, h)
  , formals_(s.formals_, h), fSels_(s.fSels_, h)
  , hqcListOfConstParamPos_(s.hqcListOfConstParamPos_, h)
  , hqcListOfSelParamPos_(s.hqcListOfSelParamPos_, h)
  , hqcListOfConstPos_(s.hqcListOfConstPos_, h)
  , origStmt_(s.origStmt_)
  , textentries_(s.textentries_, h)
{
    if (sharePlan==FALSE) {
       CMPASSERT(s.plan_);
       plan_ = new (h) Plan(*s.plan_, h) ;
    } else
       plan_ = s.plan_;

    plan_ -> incRefCount();

    if ( s.plan_ && s.origStmt_) {
      origStmt_ = new (h) char[strlen(s.origStmt_)+1];
      strcpy(CONST_CAST(char*,origStmt_), s.origStmt_);
    }
}

// free memory allocated by this CData entry
CData::~CData()
{
}

// free memory allocated by this CacheData entry
CacheData::~CacheData()
{
  if ( plan_->inGenerator() == FALSE ) {

    NADELETEBASIC(origStmt_,heap_);

  // delete plan_ only if cache_ owns it and ref. count is 1.
  // a null heap_ means cache_ does not own plan_.
  // (planL_ == 0 or inGenerator() == TRUE ) means that the plan_ is 
  // a pointer to the generator

    plan_->decRefCount();

    if ( plan_->getRefCount() == 0 ) 
      NADELETE(plan_,Plan,heap_);
   }
}

// free memory allocated by this TextData entry. Also reset the pointer
// in the textentries array in the postparser entry so that the deleted
// preparser entry can not be referenced again from that postparser entry. 
TextData::~TextData()
{
   if ( entry_ ) {
     KeyDataPair& pair = entry_->data_;
     CacheData* cData = ((CacheData*)(pair.second_));
     if ( cData ) {
        TextPtrArray& tpArray = cData->PreParserEntries();
        CollIndex n = getIndexInTextEntries();
        tpArray[n] = NULL;
     }
   }
   if ( actLen_ > 0 )
      NADELETEBASIC(actuals_,heap_);
}

// return this CacheData's entry size in bytes. Note the plan size
// is not included!!!
ULng32 CacheData::getSize() const
{
  ULng32 hqcTypesSize = (hqcListOfConstParamPos_.entries() * sizeof (Int32)) +
                        (hqcListOfSelParamPos_.entries() * sizeof (Int32)) +
                        (hqcListOfConstPos_.entries() * sizeof (Int32)) ;
  ULng32 x = sizeof(*this) + formals_.getSize() + fSels_.getSize()  + hqcTypesSize + (origStmt_ ? strlen(origStmt_) : 0);

  return x;
}

// return byte size of this CacheData's preparser entries
ULng32 CacheData::getSizeOfPreParserEntries() const
{
  ULng32 byteSize = 0;
  CollIndex x, count = textentries_.entries();
  for (x = 0; x < count; x++) {
    byteSize += sizeof(CacheEntry) + TextHashTbl::getBucketEntrySize();
    if ( textentries_[x] ) {
        byteSize += textentries_[x]->data_.first_->getSize() +
                    textentries_[x]->data_.second_->getSize();
    }
  }
  return byteSize;
}

// return this TextData's size in bytes
ULng32 TextData::getSize() const
{
  return sizeof(*this) + actLen_;
  // don't add postparser entry's memory usage because it
  // has already been accounted for by the postparser cache
}

// allocate and copy plan
void CacheData::allocNcopyPlan(NAHeap *h, char **plan, ULng32 *pLen)
{
  *pLen = plan_->getPlanLen();
  *plan = new (h) char[*pLen];
  str_cpy_all(*plan, plan_->getPlan(), (Lng32)*pLen);
}

// helper method to unpack parameter buffer part of plan_
NABoolean CacheData::unpackParms
(NABasicPtr &parameterBuffer, ULng32 &parmSz)
{
  // We partially unpack plan_ to get parameterBuffer
  ComTdbRoot *root = NULL;
  if(plan_->inGenerator() == FALSE) {
    root = (ComTdbRoot*)(plan_->getPlan());
  } else {
    root = ((Generator *)(plan_->getPlan()))->getTopRoot();
  }

  if (!root) {
    parmSz = 0;
    return FALSE;
  }

  if (root->qCacheInfoIsClass())
    {
      NABasicPtr qCacheInfoPtr = root->getQCInfoPtr();
      qCacheInfoPtr.unpack(root);
      QCacheInfo * qcInfo = (QCacheInfo *)qCacheInfoPtr.getPointer();
      parameterBuffer = qcInfo->getParameterBuffer();
    }
  else
    parameterBuffer = root->getParameterBuffer();
  parmSz = root->getCacheVarsSize();
  if (plan_->inGenerator() == FALSE) {
    // fix coverity cid 820 checked_return "return value of 
    // NABasicPtrTempl<char>::unpack(void*,short) is not checked"
    if (parameterBuffer.unpack(root)) {
      return FALSE;
    }
  } else {
    // part of fix to genesis case 10-020920-1608. This partial unpack
    // now works even if parameterBuffer.offset exceeds Space's first
    // n block's total allocated size.
    const Space *topSpace = ((Generator*)(plan_->getPlan()))->getTopSpace();
    if (!topSpace) return FALSE;
    if (parameterBuffer.unpack((void*)topSpace, 1) || !parameterBuffer) {
      return FALSE;
    }
  }
  return TRUE; // all OK
}

// backpatch for HQC queries
NABoolean CacheData::backpatchParams
    (LIST(hqcConstant *) &listOfConstantParameters,
     LIST(hqcDynParam *) &listOfDynamicParameters,
     BindWA &bindWA, char* &params, ULng32 &parameterBufferSize)
{
  // exit early if there's nothing to backpatch
  parameterBufferSize = 0;
  CollIndex countP = listOfConstantParameters.entries();
  CollIndex countD = listOfDynamicParameters.entries();
  if (countP+countD <= 0) {
    return TRUE;
  }

  // number of HQC constants should be the sum of the SQC formal and selective parameters
  CMPASSERT (countP == (formals_.entries() + fSels_.entries()))


  // collect all the constants types in the order the constants appear in the query
  CollIndex x=0;
  CollIndex y=0;
  Int32 countP2 = formals_.entries();
  Int32 countS2 = fSels_.entries();
  LIST(NAType*) hqcTypes(STMTHEAP);

  for (CollIndex j = 0; j < (countP2+countS2); j ++)
  {
     if ((x < countP2) && hqcListOfConstParamPos_[x] == j)
     {
        hqcTypes.insert(formals_[x].type_);
        x++;
     }
     else if ((y < countS2) && (hqcListOfSelParamPos_[y] == j))
     {
        hqcTypes.insert(fSels_[y].type_);
        y++;
     }
  }

  // temporary code until the issue of using convDotIt with numeric is resolved
  NABoolean anyNumeric = FALSE;
  CollIndex i;

  NABoolean useConvDoIt = (CmpCommon::getDefault(QUERY_CACHE_USE_CONVDOIT_FOR_BACKPATCH) == DF_ON);

  // disable numeric (x.y) for now, until issue is resolved
  if (CmpCommon::getDefault(HQC_CONVDOIT_DISABLE_NUMERIC_CHECK) == DF_OFF)
    for (CollIndex j = 0; j < (countP2+countS2); j ++)
    {
      NAType* targetType = hqcTypes[j];
      if ((targetType->getTypeQualifier() == NA_NUMERIC_TYPE) &&
        ((((NumericType*)targetType)->getSimpleTypeName() == "NUMERIC") || (((NumericType*)targetType)->getSimpleTypeName() == "BIG NUM")))
      {
         useConvDoIt = FALSE;
         break;
      }
    }

  // Generate a series of "assignment" expressions where the right hand sides
  // "cast" each of the list of ConstantParameters into their corresponding
  // formal types (taken from this CacheData's formals_) and the left hand
  // sides are the corresponding elements of parameterBuffer (taken from the
  // partially unpacked plan_ of this CacheData)

  ValueIdList castExprs;
  if (!useConvDoIt)
  {
     // Use an array to collect the ValueIds so that the literal positions
     // in the SQL statement match those of the substituting parameters 
     // in the plan.
     ValueIdArray castExprArray(countP);
     CollIndex i;
     for (i = 0; i < countP; i++) {
       ConstValue *constVal = listOfConstantParameters[hqcListOfConstPos_[i]]->getConstValue();
       NAType *type = CONST_CAST(NAType*, hqcTypes[i]);
       ItemExpr *castNode = new (CmpCommon::statementHeap()) Cast(constVal, type);
       castNode = castNode->bindNode(&bindWA);
       if (!castNode) {
         return FALSE;
       }
       castExprArray.insertAt(i, castNode->getValueId());
     }
   
     // transfer the list from array form to list form
     //ValueIdList castExprs;
     for (i=0; i<countP; i++) {
       castExprs.insert(castExprArray[i]);
     }
  } // QUERY_CACHE_USE_CONVDOIT_FOR_BACKPATCH is OFF

  // unpack parameter buffer part of plan_
  NABasicPtr parameterBuffer;
  if (!unpackParms(parameterBuffer, parameterBufferSize)) {
    return FALSE;
  }

  if (!useConvDoIt)
  {
     // evaluate the "assignments" to accomplish the backpatch
     ex_expr::exp_return_type evalReturnCode = castExprs.evalAtCompileTime
       (0, ExpTupleDesc::SQLARK_EXPLODED_FORMAT, parameterBuffer,
        parameterBufferSize);
     // evaluation success is guaranteed for the 2 cases that get here:
     // 1) on a cache hit, ParameterTypeList::operator == returns TRUE iff
     //    NAType::errorsCanOccur() is false for all pairs of actual to
     //    formal cast expression.
     // 2) on a cache miss, actuals and formals are identical.
     // But as CR 10-010618-3503 showed, there are still cases where
     // evaluation can fail; so, let's fail gracefully here.
     if (evalReturnCode != ex_expr::EXPR_OK) {
       return FALSE;
     }
  } // QUERY_CACHE_USE_CONVDOIT_FOR_BACKPATCH is OFF
  else
  {
     char* targetBugPtr = parameterBuffer.getPointer();
     Lng32 offset = 0;
     CollIndex x=0;
     CollIndex y=0;
     
     for (CollIndex j = 0; j < (countP); j ++)
     {
        ConstValue *constVal = NULL;
        const NAType *sourceType = NULL;
        const NAType *targetType = NULL;
  
        constVal = listOfConstantParameters[hqcListOfConstPos_[j]]->getConstValue();
        targetType = CONST_CAST(NAType*, hqcTypes[j]);

        sourceType = constVal->getType();
        Lng32 targetLen = targetType->getNominalSize();
        Lng32 sourceScale = constVal->getType()->getScale();
        Lng32 targetScale = targetType->getScale();
        Lng32 varCharLenSize = 0;
        char* varCharLen = NULL;
        
        if ((targetType->getFSDatatype() >= REC_MIN_NUMERIC) and (targetType->getFSDatatype() <= REC_MAX_FLOAT))
        {
            Lng32 extraBuffer = targetLen - (offset % targetLen);
            if (extraBuffer != targetLen)
               offset += extraBuffer;
        }

        if (DFS2REC::isAnyVarChar(targetType->getFSDatatype()))
        {
           varCharLenSize = targetType->getVarLenHdrSize();
           // align on a 2-byte since this is an integer
           offset += (offset % varCharLenSize);
           varCharLen = (char*)(targetBugPtr+offset);
           offset += varCharLenSize;
           
           // is this an empty string
           //if (constVal->isEmptyString())
           //   varCharLenSize = 0;
        }
  
        if (DFS2REC::isAnyCharacter(targetType->getFSDatatype()))
        {
           sourceScale= constVal->getType()->getCharSet();
           targetScale= targetType->getCharSet();
        }
  
        char* charVal = (char*)(constVal->getConstValue());
        Int32 val = 0;

        // NEED TO TEST FOR BIGNUM
        if ((targetType->getTypeQualifier() == NA_NUMERIC_TYPE) &&
          ((((NumericType*)targetType)->getSimpleTypeName() == "NUMERIC") || (((NumericType*)targetType)->getSimpleTypeName() == "BIG NUM")) &&
           (targetScale > sourceScale)
           )
        {
              val = *((Int32*) (constVal->getConstValue()));
              val = val * pow(10, targetScale-sourceScale);
              charVal = (char*) (&val);
        }

        Lng32 dataConversionErrorFlag = 0;
        short retCode = convDoIt((char*)charVal,
                                 constVal->getStorageSize(),
                                 (short)sourceType->getFSDatatype(),
                                 sourceType->getPrecisionOrMaxNumChars(),
                                 sourceScale,
                                 (char*)(targetBugPtr+offset),
                                 targetLen,
                                 (short)(targetType->getFSDatatype()),
                                 targetType->getPrecisionOrMaxNumChars(),
                                 targetScale,
                                 varCharLen, 
                                 varCharLenSize,
                                 NULL, NULL, CONV_UNKNOWN,
                                 &dataConversionErrorFlag);

        if ((retCode != ex_expr::EXPR_OK) ||
            (dataConversionErrorFlag == ex_conv_clause::CONV_RESULT_ROUNDED_DOWN))
           return FALSE;
  
        offset += targetLen;
        CMPASSERT ((j < (countP-1)) || (offset == parameterBufferSize));
     }
  }

  params = parameterBuffer.getPointer();
  return TRUE; // all OK
}



// copies listOfConstantParameters into this CacheData's plan_
NABoolean CacheData::backpatchParams
(const ConstantParameters &listOfConstantParameters, 
 const SelParameters &listOfSelParameters, 
 const LIST(Int32)& listOfConstParamPositionsInSql,
 const LIST(Int32)& listOfSelParamPositionsInSql,
 BindWA &bindWA, char* &params, ULng32 &parameterBufferSize)
{
  // exit early if there's nothing to backpatch
  parameterBufferSize = 0;
  CollIndex countP = listOfConstantParameters.entries();
  CollIndex countS = listOfSelParameters.entries();
  if (countP+countS <= 0) {
    return TRUE;
  }

  // temporary code until the issue of using convDotIt with numeric is resolved
  NABoolean anyNumeric = FALSE;
  CollIndex i;

  NABoolean useConvDoIt = (CmpCommon::getDefault(QUERY_CACHE_USE_CONVDOIT_FOR_BACKPATCH) == DF_ON);

  // disable numeric (x.y) for now, until issue is resolved
  if (CmpCommon::getDefault(HQC_CONVDOIT_DISABLE_NUMERIC_CHECK) == DF_OFF)
    for (CollIndex x = 0; x  < countP; x++)
    {
       NAType* targetType = formals_[x].type_;
       if ((targetType->getTypeQualifier() == NA_NUMERIC_TYPE) && 
            ((((NumericType*)targetType)->getSimpleTypeName() == "NUMERIC") || (((NumericType*)targetType)->getSimpleTypeName() == "BIG NUM")))
       {
          useConvDoIt = FALSE;
          break;
       }
    }
  
  for (CollIndex x = 0; (x  < countS) && useConvDoIt; x++)
  {
     NAType* targetType = fSels_[x].type_;
     if ((targetType->getTypeQualifier() == NA_NUMERIC_TYPE) && 
       ((((NumericType*)targetType)->getSimpleTypeName() == "NUMERIC") || (((NumericType*)targetType)->getSimpleTypeName() == "BIG NUM")))
     {
          useConvDoIt = FALSE;
          break;
     }
  }

  // Generate a series of "assignment" expressions where the right hand sides
  // "cast" each of the list of ConstantParameters into their corresponding
  // formal types (taken from this CacheData's formals_) and the left hand
  // sides are the corresponding elements of parameterBuffer (taken from the
  // partially unpacked plan_ of this CacheData)

  ValueIdList castExprs;
  if (!useConvDoIt)
  {
     // Use an array to collect the ValueIds so that the literal positions
     // in the SQL statement match those of the substituting parameters 
     // in the plan.
     ValueIdArray castExprArray(countP+countS);
     CollIndex i;
     for (i = 0; i < countP; i++) {
       ConstValue *constVal = listOfConstantParameters[i]->getConstVal();
       NAType *type = formals_[i].type_;
       ItemExpr *castNode = new (CmpCommon::statementHeap()) Cast(constVal, type);
       castNode = castNode->bindNode(&bindWA);
       if (!castNode) {
         return FALSE;
       }
       castExprArray.insertAt(
          listOfConstParamPositionsInSql[i],
          castNode->getValueId());
     }
     for (i = 0; i < countS; i++) {
       ConstValue *constVal = listOfSelParameters[i]->getConstVal();
       NAType *type = fSels_[i].type_;
       ItemExpr *castNode = new (CmpCommon::statementHeap()) Cast(constVal, type);
       castNode = castNode->bindNode(&bindWA);
       if (!castNode) {
         return FALSE;
       }
       castExprArray.insertAt(
           listOfSelParamPositionsInSql[i],
           castNode->getValueId());
     }
   
     // transfer the list from array form to list form
     //ValueIdList castExprs;
     for (i=0; i<countP+countS; i++) {
       castExprs.insert(castExprArray[i]);
     }
  } // QUERY_CACHE_USE_CONVDOIT_FOR_BACKPATCH is OFF

  // unpack parameter buffer part of plan_
  NABasicPtr parameterBuffer;
  if (!unpackParms(parameterBuffer, parameterBufferSize)) {
    return FALSE;
  }

  if (!useConvDoIt)
  {
     // evaluate the "assignments" to accomplish the backpatch
     ex_expr::exp_return_type evalReturnCode = castExprs.evalAtCompileTime
       (0, ExpTupleDesc::SQLARK_EXPLODED_FORMAT, parameterBuffer,
        parameterBufferSize);
     // evaluation success is guaranteed for the 2 cases that get here:
     // 1) on a cache hit, ParameterTypeList::operator == returns TRUE iff
     //    NAType::errorsCanOccur() is false for all pairs of actual to
     //    formal cast expression.
     // 2) on a cache miss, actuals and formals are identical.
     // But as CR 10-010618-3503 showed, there are still cases where
     // evaluation can fail; so, let's fail gracefully here.
     if (evalReturnCode != ex_expr::EXPR_OK) {
       return FALSE;
     }
  } // QUERY_CACHE_USE_CONVDOIT_FOR_BACKPATCH is OFF
  else
  {
     char* targetBugPtr = parameterBuffer.getPointer();
     Lng32 offset = 0;
     CollIndex x=0;
     CollIndex y=0;
     
     for (CollIndex j = 0; j < (countP+countS); j ++)
     {
        ConstValue *constVal = NULL;
        const NAType *sourceType = NULL;
        const NAType *targetType = NULL;
  
        if ((x < countP) && listOfConstParamPositionsInSql[x] == j)
        {
           constVal = listOfConstantParameters[x]->getConstVal();
           targetType = formals_[x].type_;
           x++; 
        }
        else if ((y < countS) && (listOfSelParamPositionsInSql[y] == j))
        {
           constVal = listOfSelParameters[y]->getConstVal();
           targetType = fSels_[y].type_;
           y++; 
        }

        sourceType = constVal->getType();
        Lng32 targetLen = targetType->getNominalSize();
        Lng32 sourceScale = constVal->getType()->getScale();
        Lng32 targetScale = targetType->getScale();
        Lng32 varCharLenSize = 0;
        char* varCharLen = NULL;
        
        if ((targetType->getFSDatatype() >= REC_MIN_NUMERIC) and (targetType->getFSDatatype() <= REC_MAX_FLOAT))
        {
            Lng32 extraBuffer = targetLen - (offset % targetLen);
            if (extraBuffer != targetLen)
               offset += extraBuffer;
        }

        if (DFS2REC::isAnyVarChar(targetType->getFSDatatype()))
        {
           varCharLenSize = targetType->getVarLenHdrSize();
           // align on a 2-byte since this is an integer
           offset += (offset % varCharLenSize);
           varCharLen = (char*)(targetBugPtr+offset);
           offset += varCharLenSize;
           
           // is this an empty string
           if (constVal->isEmptyString())
              varCharLenSize = 0;
        }
  
        if (DFS2REC::isAnyCharacter(targetType->getFSDatatype()))
        {
           sourceScale= constVal->getType()->getCharSet();
           targetScale= targetType->getCharSet();
        }
  
        char* charVal = (char*)(constVal->getConstValue());
        Int32 val = 0;

        // NEED TO TEST FOR BIGNUM
        if ((targetType->getTypeQualifier() == NA_NUMERIC_TYPE) &&
          ((((NumericType*)targetType)->getSimpleTypeName() == "NUMERIC") || (((NumericType*)targetType)->getSimpleTypeName() == "BIG NUM")) &&
           (targetScale > sourceScale)
           )
        {
              val = *((Int32*) (constVal->getConstValue()));
              val = val * pow(10, targetScale-sourceScale);
              charVal = (char*) (&val);
        }
  
        short retCode = convDoIt((char*)charVal,
           constVal->getStorageSize(),
           (short)sourceType->getFSDatatype(),
           sourceType->getPrecision(),
           sourceScale,
           (char*)(targetBugPtr+offset),
           targetLen,
           (short)(targetType->getFSDatatype()),
           targetType->getPrecision(),
           targetScale,
           varCharLen, 
           varCharLenSize);

        if (retCode != ex_expr::EXPR_OK)
           return FALSE;
  
        offset += targetLen;
        CMPASSERT ((j < (countP+countS-1)) || (offset == parameterBufferSize));
     }
  }

  params = parameterBuffer.getPointer();
  return TRUE; // all OK

}

// copies actuals_ into this CacheData's plan_
NABoolean CacheData::backpatchPreParserParams
(char *actuals, ULng32 actLen)
{
  // exit early if there's nothing to backpatch
  if (!actuals || actLen <= 0) {
    return TRUE;
  }

  // get parameter buffer part of plan_
  NABasicPtr parameterBuffer; ULng32 formals_len;
  if (!unpackParms(parameterBuffer, formals_len)) {
    return FALSE;
  }
  CMPASSERT(formals_len == actLen);

  // copy actuals into plan
  memcpy(parameterBuffer.getPointer(), actuals, actLen);
  return TRUE; // all OK
}

// copies actuals_ into this TextData's postparser's plan_
NABoolean TextData::backpatchParams()
{
  // exit early if there's nothing to backpatch
  if (!actuals_ || actLen_ <= 0) {
    return TRUE;
  }

  // backpatch actuals_ into our postparser entry's plan
  return PostParserEntry()->backpatchPreParserParams(actuals_, actLen_);
}

// add backpointer to given preparser cache entry
void CacheData::addTextEntry(CacheEntry *entry)
{
  CollIndex n = textentries_.entries();
  textentries_.insert(n, entry);
  KeyDataPair& pair = entry->data_;
  ((TextData*)(pair.second_))->setIndexInTextEntries(n);
}

// tally hit time
void CData::addHitTime(TimeVal& begTime)
{
  cumHitTime_ += timeSince(begTime);
}

// tally hit time
void TextData::addHitTime(TimeVal& begTime)
{
  CData::addHitTime(begTime); // for preparser entry
  PostParserEntry()->addHitTime(begTime); // for postparser entry
}

// compute this entry's compile time (in msec)
void CData::setCompTime(TimeVal& begTime)
{
  compTime_ = timeSince(begTime);
}

// return elapsed msec since begTime
Int64 CData::timeSince(TimeVal& begTime)
{
  TimeVal endTime;
  GETTIMEOFDAY(&endTime, 0);
  return
    ((endTime.tv_sec * (Int64) 1000000) + endTime.tv_usec) -
    ((begTime.tv_sec * (Int64) 1000000) + begTime.tv_usec);
}

// return the prime number closest to nEntries used by QCache::QCache() &
// QCache::resizeCache() to determine number of buckets for its hash tables
static ULng32 npBuckets(ULng32 nEntries)
{
  static const ULng32 primes[] = 
    {7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,
     107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,
     199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,
     311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,
     421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,
     541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,641,643,
     647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,
     769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,877,881,883,
     887,907,911,919,929,937,941,947,953,967,971,977,983,991,997,1009,1013,
     1019,1021,1031,1033,1039,1049,1051,1061,1063,1069,1087,1091,1093,1097,
     1103,1109,1117,1123,1129,1151,1153,1163,1171,1181,1187,1193,1201,1213,
     1217,1223,1229,1231,1237,1249,1259,1277,1279,1283,1289,1291,1297,1301,
     1303,1307,1319,1321,1327,1361,1367,1373,1381,1399,1409,1423,1427,1429,
     1433,1439,1447,1451,1453,1459,1471,1481,1483,1487,1489,1493,1499,1511,
     1523,1531,1543,1549,1553,1559,1567,1571,1579,1583,1597,1601,1607,1609,
     1613,1619,1621,1627,1637,1657,1663,1667,1669,1693,1697,1699,1709,1721,
     1723,1733,1741,1747,1753,1759,1777,1783,1787,1789,1801,1811,1823,1831,
     1847,1861,1867,1871,1873,1877,1879,1889,1901,1907,1913,1931,1933,1949,
     1951,1973,1979,1987,1993,1997,1999,2003,2011,2017,2027,2029,2039,2053,
     2063,2069,2081,2083,2087,2089,2099,2111,2113,2129,2131,2137,2141,2143,
     2153,2161,2179,2203,2207,2213,2221,2237,2239,2243,2251,2267,2269,2273,
     2281,2287,2293,2297,2309,2311,2333,2339,2341,2347,2351,2357,2371,2377,
     2381,2383,2389,2393,2399,2411,2417,2423,2437,2441,2447,2459,2467,2473,
     2477,2503,2521,2531,2539,2543,2549,2551,2557,2579,2591,2593,2609,2617,
     2621,2633,2647,2657,2659,2663,2671,2677,2683,2687,2689,2693,2699,2707,
     2711,2713,2719,2729,2731,2741,2749,2753,2767,2777,2789,2791,2797,2801,
     2803,2819,2833,2837,2843,2851,2857,2861,2879,2887,2897,2903,2909,2917,
     2927,2939,2953,2957,2963,2969,2971,2999,3001,3011,3019,3023,3037,3041,
     3049,3061,3067,3079,3083,3089,3109,3119,3121,3137,3163,3167,3169,3181,
     3187,3191,3203,3209,3217,3221,3229,3251,3253,3257,3259,3271,3299,3301,
     3307,3313,3319,3323,3329,3331,3343,3347,3359,3361,3371,3373,3389,3391,
     3407,3413,3433,3449,3457,3461,3463,3467,3469,3491,3499,3511,3517,3527,
     3529,3533,3539,3541,3547,3557,3559,3571,3581,3583,3593,3607,3613,3617,
     3623,3631,3637,3643,3659,3671,3673,3677,3691,3697,3701,3709,3719,3727,
     3733,3739,3761,3767,3769,3779,3793,3797,3803,3821,3823,3833,3847,3851,
     3853,3863,3877,3881,3889,3907,3911,3917,3919,3923,3929,3931,3943,3947,
     3967,3989,4001,4003,4007,4013,4019,4021,4027,4049,4051,4057,4073,4079,
     4091,4093,4099,4111,4127,4129,4133,4139,4153,4157,4159,4177,4201,4211,
     4217,4219,4229,4231,4241,4243,4253,4259,4261,4271,4273,4283,4289,4297,
     4327,4337,4339,4349,4357,4363,4373,4391,4397,4409,4421,4423,4441,4447,
     4451,4457,4463,4481,4483,4493,4507,4513,4517,4519,4523,4547,4549,4561,
     4567,4583,4591,4597,4603,4621,4637,4639,4643,4649,4651,4657,4663,4673,
     4679,4691,4703,4721,4723,4729,4733,4751,4759,4783,4787,4789,4793,4799,
     4801,4813,4817,4831,4861,4871,4877,4889,4903,4909,4919,4931,4933,4937,
     4943,4951,4957,4967,4969,4973,4987,4993,4999,5003,5009,5011,5021,5023,
     5039,5051,5059,5077,5081,5087,5099,5101,5107,5113,5119,5147,5153,5167,
     5171,5179,5189,5197,5209,5227,5231,5233,5237,5261,5273,5279,5281,5297,
     5303,5309,5323,5333,5347,5351,5381,5387,5393,5399,5407,5413,5417,5419,
     5431,5437,5441,5443,5449,5471,5477,5479,5483,5501,5503,5507,5519,5521,
     5527,5531,5557,5563,5569,5573,5581,5591,5623,5639,5641,5647,5651,5653,
     5657,5659,5669,5683,5689,5693,5701,5711,5717,5737,5741,5743,5749,5779,
     5783,5791,5801,5807,5813,5821,5827,5839,5843,5849,5851,5857,5861,5867,
     5869,5879,5881,5897,5903,5923,5927,5939,5953,5981,5987,6007,6011,6029,
     6037,6043,6047,6053,6067,6073,6079,6089,6091,6101,6113,6121,6131,6133,
     6143,6151,6163,6173,6197,6199,6203,6211,6217,6221,6229,6247,6257,6263,
     6269,6271,6277,6287,6299,6301,6311,6317,6323,6329,6337,6343,6353,6359,
     6361,6367,6373,6379,6389,6397,6421,6427,6449,6451,6469,6473,6481,6491,
     6521,6529,6547,6551,6553,6563,6569,6571,6577,6581,6599,6607,6619,6637,
     6653,6659,6661,6673,6679,6689,6691,6701,6703,6709,6719,6733,6737,6761,
     6763,6779,6781,6791,6793,6803,6823,6827,6829,6833,6841,6857,6863,6869,
     6871,6883,6899,6907,6911,6917,6947,6949,6959,6961,6967,6971,6977,6983,
     6991,6997,7001,7013,7019,7027,7039,7043,7057,7069,7079,7103,7109,7121,
     7127,7129,7151,7159,7177,7187,7193,7207,7211,7213,7219,7229,7237,7243,
     7247,7253,7283,7297,7307,7309,7321,7331,7333,7349,7351,7369,7393,7411,
     7417,7433,7451,7457,7459,7477,7481,7487,7489,7499,7507,7517,7523,7529,
     7537,7541,7547,7549,7559,7561,7573,7577,7583,7589,7591,7603,7607,7621,
     7639,7643,7649,7669,7673,7681,7687,7691,7699,7703,7717,7723,7727,7741,
     7753,7757,7759,7789,7793,7817,7823,7829,7841,7853,7867,7873,7877,7879,
     7883,7901,7907,7919 
    };
  ULng32 result = nEntries;
  if (result <= primes[1]) {
    return primes[1];
  }

  ULng32 limit = sizeof(primes) / sizeof(primes[0]), hi = limit-1;
  if (result >= primes[hi-1]) {
    return primes[hi-1];
  }

  ULng32 lo=0, mid;
  while (lo < hi) {
    mid = (lo + hi) / 2;
    if (primes[mid-1] <= result && result <= primes[mid+1]) {
      return primes[mid];
    }

    if (result < primes[mid]) {
      hi = mid - 1;
    }
    else {
      lo = mid + 1;
    }
  }
  return primes[limit-1];
}

const Int32 AVGTEXTENTRYSIZE=500;
const Int32 A_PREPARSE=CmpMain::PREPARSE;
const Int32 A_PARSE=CmpMain::PARSE;
const Int32 A_BIND=CmpMain::BIND;

// return the prime number closest to (maxHeapSz/avgPlanSz)*2.
// used by QCache::QCache() and QCache::resizeCache() to determine
// number of buckets for CacheHashTbl.
static ULng32 nBuckets(ULng32 maxHeapSz, ULng32 avgPlanSz)
{
  ULng32 nEntries = (ULng32)((maxHeapSz / avgPlanSz) * 1.2);
  return npBuckets(nEntries);
}

// reset counters
void QCache::clearStats()
{
  limit_ = 0;
  nOfCompiles_ = 0;
  nOfLookups_ = 0;
  nOfRecompiles_ = 0;  
  nOfRetries_ = 0;
  nOfCacheableButTooLarge_ = 0;
  nOfDisplacedEntries_ = 0;
  nOfDisplacedPreParserEntries_ = 0;
  nOfCacheableCompiles_[A_PREPARSE] =
  nOfCacheableCompiles_[A_PARSE] =
  nOfCacheableCompiles_[A_BIND] = 0;
  nOfCacheHits_[A_PREPARSE] =
  nOfCacheHits_[A_PARSE] =
  nOfCacheHits_[A_BIND] = 0;  
}

// constructor for query cache
QCache::QCache(QueryCache & qc, ULng32 maxSize, ULng32 maxVictims, ULng32 avgPlanSz)
  : querycache_(qc), maxSiz_(maxSize), limit_(maxVictims)
  , heap_(new CTXTHEAP NABoundedHeap
          ("mxcmp cache heap", (NAHeap *)CTXTHEAP, 0, 0))
  , clruQ_(heap_)
  , tlruQ_(heap_)
  , nOfCompiles_(0)
  , nOfLookups_(0)
  , nOfRecompiles_(0)  
  , nOfRetries_(0)
  , nOfCacheableButTooLarge_(0)
  , nOfDisplacedEntries_(0)
  , nOfDisplacedPreParserEntries_(0)
  , planSz_(avgPlanSz)
  , tEntSz_(AVGTEXTENTRYSIZE)
{
  nOfCacheableCompiles_[A_PREPARSE] =
  nOfCacheableCompiles_[A_PARSE] =
  nOfCacheableCompiles_[A_BIND] = 0;
  nOfCacheHits_[A_PREPARSE] =
  nOfCacheHits_[A_PARSE] =
  nOfCacheHits_[A_BIND] = 0;  
  heap_->setErrorCallback(&CmpErrLog::CmpErrLogCallback);

#ifdef DBG_QCACHE
  filestream = new fstream("qcache.log", ios::out);
  logmsg("begin QCache::QCache()");
#endif

  ULng32 x=heap_->getAllocSize();

  cache_ = new (heap_) CacheHashTbl
    (hashKeyFunc, // use hash function defined above
                  // compute nBuckets from maxSize & avgPlanSz
     nBuckets(maxSize,avgPlanSz),
     TRUE,        // enforce uniqueness of keys
     heap_);      // use this bounded heap


  tHash_ = new (heap_) TextHashTbl
    (hashTextFunc, // use hash function defined above
                  // compute nBuckets from maxSize & avgPlanSz
     npBuckets(maxPreParserEntries(maxSiz_,tEntSz_)),
     TRUE,        // enforce uniqueness of keys
     heap_);      // use this bounded heap

  totalHashTblSize_ = heap_->getAllocSize() - x;

#ifdef DBG_QCACHE
  ULng32 y=heap_->getAllocSize();
  logmsg("after allocate CacheHashTbl", y-x);
  logmsg("end QCache::QCache()");
#endif
}

// free all memory allocated by query cache
QCache::~QCache()
{
  // free all memory owned by cache heap
  NADELETE(heap_, NABoundedHeap, CTXTHEAP);

#ifdef DBG_QCACHE
  delete filestream;
#endif
}

// shrink query cache to zero entries
void QCache::makeEmpty()
{
  //
  // Need to delete text entries before deleting the post-parser ones 
  // since TextData::~TextData() will clear one element in the textentries[]
  // in the post-parser entry it is pointing at. If the post-parer entries 
  // are deleted first, the destructor will reference the deallocated memory. 
  // 
  // Without doing so, fullstack2/TEST042Q fails on debug NSK in R2.4SP2 
  // in 20100107 test label when the TRANSACTION LEVEL suddenly changes 
  // from READ COMMITTED to SERIALIZABLE. The failure happens near the 
  // SQL comment 'verify fix to a bug found by Ramses Sotelo's group' 
  // in the test.
  //
  tHash_->clear(); // removes entries from text hash table
  cache_->clear(); // removes entries from template hash table
  tlruQ_.clear();  // removes and destroys entries from text list
  clruQ_.clear();  // removes and destroys entries from template list
  querycache_.getHQC()->clear();   //empty HQC as well
}

// return bytes that can be freed by evicting this postparser cache entry
ULng32 QCache::getSizeOfPostParserEntry(KeyDataPair& entry) 
{
  Plan* p = ((CacheData*)entry.second_)->getPlan();
  p ->visitOnce();
  ULng32 planSize = ( p->getVisits() == p->getRefCount() ) ? p -> getSize() : 0;

  return entry.first_->getSize() + entry.second_->getSize() + planSize +
    + ((CacheData*)(entry.second_))->getSizeOfPreParserEntries() 
    + sizeof(CacheEntry) + CacheHashTbl::getBucketEntrySize();
}

// this routine is used for debugging qcache bugs only
void QCache::sanityCheck(Int32 mark)
{
  // compute total freeable bytes of prospective LRU entries
  LRUList::iterator lru = clruQ_.end();
  while (lru != clruQ_.begin() ) {
    KeyDataPair& entry = *(--lru);
    getSizeOfPostParserEntry(entry);
  }
}

// return TRUE iff cache can accommodate a new entry of this size
NABoolean QCache::canFit(ULng32 size)
{
  ULng32 x = 0, freeable = getFreeSize(), bytesNeeded = size;
  // compute total freeable bytes of prospective LRU entries
  LRUList::iterator lru = clruQ_.end();
  while (x < limit_ && lru != clruQ_.begin() && freeable < bytesNeeded) {
    KeyDataPair& entry = *(--lru);
    freeable += getSizeOfPostParserEntry(entry);
    x++;
  }

  // reset the visit counters for all plans visited
  LRUList::iterator i = clruQ_.end();
  while ( i != clruQ_.begin() ) {
    KeyDataPair& entry = *(--i);
    ((CacheData*)entry.second_)->getPlan()->resetVisits();
    if ( i == lru ) 
      break;
  }

  return freeable >= bytesNeeded;
}

// try to add this new postparser (and preparser) entry into the cache
CacheKey* QCache::addEntry(TextKey *tkey,
                      CacheKey*stmt, CacheData *plan, TimeVal& begTime,
                      char *params, ULng32 parmSz)
{
   CacheKey* ckeyInQCache = NULL;

   // stmt and plan and tkey are well formed
   CMPASSERT(stmt && plan && tkey);

   // remove any old cache entries with the same text key, because they may be stale
   deCacheAll(tkey, NULL);

   // include cache entry insert attempts (for all cache misses) in cache lookups 
   incNOfLookups();

   // prepare an iterator that will go over all items (plans) that potentially 
   // match 'stmt'. These item should be in the same bucket!

   NABoolean canReuseCachedPlan = FALSE;
   Plan* planToUse = plan->getPlan();

   NAString defVal;
   NABoolean doPlanSharing =
     (CmpCommon::getDefault(SHARE_TEMPLATE_CACHED_PLANS, defVal) == DF_ON);

  if ( doPlanSharing ) {

     stmt->setCompareSelectivity(FALSE); // set the flag to FALSE so that
                                         // selectivity is not compared. See
               // method void NAHashBucket<K,V>::getKeyValuePair() and 
               // NAHashDictionaryIterator<K,V>::NAHashDictionaryIterator().

               // CacheKey::opeartor==() is called to collect items in the
               // cItor. The env, the parameterized SQL text and parameter 
               // types for key or non-key columns are compared. 
    
     CacheHashTblIterator cItor(*cache_ /*template hash table*/, stmt, NULL);

     // restore the flag to TRUE (enable selectivity compare)
     stmt->setCompareSelectivity(TRUE); 

     CacheKey *key = NULL;
     CacheEntry *cEntry = NULL;
     CacheData* value = NULL;
   
   
     for ( CollIndex i = 0 ; i < cItor.entries() ; i++) {
       cItor.getNext(key, cEntry);
   
       value = (CacheData*)(cEntry->data_.second_);
   
       // Reuse the plan if the plan length is the same. Note plans are not 
       // compared as it is very unlikely two plans with equal length diff. 
       // Besides, even two plans are the same, certain bytes in the two 
       // still can diff!  
       if ( value->getPlan()->getPlanLen() == plan->getPlan()->getPlanLen() ) {
          planToUse = value->getPlan();
          canReuseCachedPlan = TRUE;
          break;
       }
     }
  }

  // does heap have sufficient free space to hold smt + plan?
  ULng32 bytesNeeded = 

    // size of template cache entry is:
    sizeof(CacheEntry) + 
    stmt->getSize()   // key part (CacheEntry.first_)
    + plan->getSize()   // data part (CacheEntry.second_ excluding plan size)
    // include plan size if we can not reuse the plan
    + ((canReuseCachedPlan==TRUE) ? 0 : (plan->getPlan()->getSize())) 
    + CacheHashTbl::getBucketEntrySize() +

    // size of text cache entry is:
    + tkey->getSize() + parmSz + TextHashTbl::getBucketEntrySize() ;

  // When there is no space to hold the new entry with plan sharing, do not attempt
  // the sharing because the entry-to-be-shared can be purged to create room for
  // the new entry.  Fix for CR2202 and CR4905.
  if (canReuseCachedPlan && getFreeSize() < bytesNeeded) { 
     canReuseCachedPlan = FALSE;
     bytesNeeded += plan->getPlan()->getSize();
  }
    
  Plan* newPlan = NULL; // the newly compiled plan
  if ( canReuseCachedPlan ) {
    newPlan = plan->getPlan(); // reuse the existing one
    plan->setPlan(planToUse);
  }

  KeyDataPair newEntry(stmt,plan);
  if (getFreeSize() >= bytesNeeded) { // yes, there's space.
    ckeyInQCache = 
        addEntry(tkey, newEntry, begTime, params, parmSz, canReuseCachedPlan);

    // we must null newEntry's contents here to prevent its destructor
    // from delete'ing its contents which are now owned by the cache.
    newEntry.first_ = NULL; newEntry.second_ = NULL;
  }
  else { // no, there's no space.
    if (!canFit(bytesNeeded)) {
      // not enough space to hold newEntry
      incNOfCacheableButTooLarge();
    }
    // yes, we can free enough space to make room for newEntry
    else if (freeLRUentries(bytesNeeded, limit_)) {
      ckeyInQCache = 
         addEntry(tkey, newEntry, begTime, params, parmSz, canReuseCachedPlan);

      // we must null newEntry's contents here to prevent its destructor
      // from delete'ing its contents which are now owned by the cache.
      newEntry.first_ = NULL; newEntry.second_ = NULL;
    }
    else { // still no room.
      incNOfCacheableButTooLarge();
    }
  }

  if ( canReuseCachedPlan ) 
    plan->setPlan(newPlan); // restore the plan

   // we must null newEntry's contents here to prevent its destructor
   // from delete'ing its contents which are now owned by the cache.
   newEntry.first_ = NULL; newEntry.second_ = NULL;

   return ckeyInQCache;
}

// make room for and add this new preparser entry into the cache
void QCache::addPreParserEntry(TextKey *tkey, 
                               char *params, ULng32 parmSz, 
                               CacheEntry *entry, TimeVal &begTime)
{
  // do nothing if preparser caching is off
  if (!isPreparserCachingOn(TRUE)) return;

  // arguments are well formed
  if (!tkey || !entry || !tkey->amSafeToHash())
    return; // do nothing

  // remove any old cache entries with the same text key, because they may be stale
  // Fix for Bugzilla 2135: Decache only preparser entries, to avoid
  // decaching the entry "entry" itself that we want to insert
  deCachePreParserEntry(tkey);

  // has cache reached its preparser limit?
  if (tlruQ_.size() >= maxPreParserEntries(maxSiz_,tEntSz_)) { 
    // yes, preparser cache is full. make room for new entry.
    freeLRUPreParserEntry();
  }
  // preparser cache has room. add new entry

  // transfer ownership of newEntry's memory to cache heap_
  TextKey *tk = new(heap_) TextKey(*tkey, heap_);
  TextData *td = new(heap_) TextData(heap_, params, parmSz, entry);
  KeyDataPair newEntry(tk, td);

  // add newEntry to front (MRU) end of preparser LRU queue
  LRUList::iterator e = tlruQ_.pushFront(newEntry);

  // add entry to preparser cache
  TextKey *insertResult = tHash_->insert((TextKey*)(newEntry.first_), e.node_);
  CMPASSERT(insertResult != NULL);

  // make postparser entry point back to new preparser entry
  ((CacheData*)(entry->data_.second_))->addTextEntry(e.node_);

  // compute and set this entry's compile time
  newEntry.second_->setCompTime(begTime);

  // we must null newEntry's contents here to prevent its destructor
  // from delete'ing its contents which are now owned by the cache.
  newEntry.first_ = NULL;
  newEntry.second_ = NULL;
}

// search cache for a query plan that matches this key and if found,
// return TRUE & pointers to its plan & entry. Otherwise, return FALSE.
NABoolean QCache::lookUp(CacheKey *stmt, CacheDataPtr &data,
                         CacheEntryPtr &entry, CmpPhase phase)
{
  // make sure arguments are well-formed
  if (!stmt || !stmt->amSafeToHash())
    return FALSE;

  CmpPhase stage = stmt->getCmpPhase();
  if (stage == phase) { // to avoid double counting post-parser cacheables. 
    // Otherwise, a tuple-insert miss will undergo post-binder lookup and
    // would be counted twice as cacheable-after-parser.
    incNOfCacheableCompiles(stage);
  }

  entry = cache_->getFirstValue((const CacheKey*)stmt);
  if (entry && entry->data_.second_) { // found a match
    incNOfCacheHits(stage);
    // include template cache hits in cache lookups 
    incNOfLookups();

    entry->data_.second_->incHits();

    // move entry to front (MRU) end of LRU queue
    clruQ_.splice(clruQ_.begin(), clruQ_, LRUList::iterator(entry));

    // return pointer to compiled plan
    data = (CacheDataPtr)(entry->data_.second_);
    return TRUE; // cache hit
  }
  return FALSE; // cache miss
}

// search cache for a query plan that matches this key and if found,
// return TRUE & set data to point to that plan. Otherwise, return FALSE.
NABoolean QCache::lookUp(TextKey *stmt, TextDataPtr &data)
{
  // make sure arguments are well-formed
  if (!stmt || !stmt->amSafeToHash())
    return FALSE; 

  CmpPhase stage = stmt->getCmpPhase();

  CacheEntry* entry = tHash_->getFirstValue((const TextKey*)stmt);
  if (entry && entry->data_.second_) { // found a match
    incNOfCacheHits(stage);
    // include text cache hits in lookups 
    incNOfLookups();

    entry->data_.second_->incHits();

    // move entry to front (MRU) end of LRU queue of preparser cache
    tlruQ_.splice(tlruQ_.begin(), tlruQ_, LRUList::iterator(entry));

    // move entry to front (MRU) end of LRU queue of postparser cache
    CacheEntry *ppentry = ((TextData*)entry->data_.second_)->PostParserNode();
    clruQ_.splice(clruQ_.begin(), clruQ_, LRUList::iterator(ppentry));

    // consider this a hit for this template cache entry also
    ppentry->data_.second_->incHits();

    // return pointer to compiled plan
    data = (TextDataPtr)(entry->data_.second_);
    return TRUE; // cache hit
  }
  return FALSE; // cache miss
}

// unconditionally add this postparser (and preparser) entry into the cache
CacheKey* QCache::addEntry(TextKey *tkey, 
                      KeyDataPair& newEntry, TimeVal& begTime,
                      char *params, ULng32 parmSz, NABoolean sharePlan)
{
  CacheKey* newCKeyInQCache = NULL;

  // if entry is malformed
  const CacheKey *entry = (const CacheKey*)(newEntry.first_);
  if (!entry || !entry->amSafeToHash())
    return NULL; // do nothing

  // if entry is already in cache, do nothing
  if (cache_->getFirstValue((const CacheKey*)(newEntry.first_))) 
     return NULL;

#ifdef DBG_QCACHE
   ULng32 x, y;
   x = heap_->getAllocSize();
   logmsg("Begin ::addEntry()");
#endif

  // transfer ownership of new postparser entry's memory to cache heap_
  newEntry.first_ = newCKeyInQCache = new(heap_) CacheKey(*(CacheKey*)(newEntry.first_), heap_);
  
#ifdef DBG_QCACHE
  y = heap_->getAllocSize();
  logmsg("after allocate first_", y-x);
#endif

  newEntry.second_ = new(heap_) CacheData
    (*(CacheData*)(newEntry.second_), heap_, sharePlan);

  //keep a copy of planId in CacheKey for HybridQueryCacheDetails convenience
  CacheKey* cacheKey = (CacheKey*)(newEntry.first_);
  CacheData* cacheData = (CacheData*)(newEntry.second_);
  cacheKey->setPlanId(cacheData->getPlan()->getId());

#ifdef DBG_QCACHE
  x = heap_->getAllocSize();
  logmsg("after allocate second_", x-y);
#endif

  // At this point any plans represented by a pointer to the Generator
  // have been converted to a contiguous pack plan.

  // add newEntry to front (MRU) end of postparser LRU queue
  LRUList::iterator e = clruQ_.pushFront(newEntry);

  // add entry to postparser cache
  CacheKey *insertResult = cache_->insert
    ((CacheKey*)(newEntry.first_), e.node_);
  CMPASSERT(insertResult != NULL);

#ifdef DBG_QCACHE
  y = heap_->getAllocSize();
  logmsg("after insert into postparser cache", y-x);
#endif

  // add text cache entry only if query has no view reference
  if (((CacheKey*)(newEntry.first_))->useView() == FALSE) {
    // add new preparser entry 
    addPreParserEntry(tkey, params, parmSz, e.node_, begTime);

#ifdef DBG_QCACHE
  x = heap_->getAllocSize();
  logmsg("after insert into preparser cache", x-y);
#endif
  }
  // compute and set this entry's compile time
  newEntry.second_->setCompTime(begTime);

  // return the cache key so that HQC can use the same pointer
  // to point at the same ckey.
  return newCKeyInQCache;

#ifdef DBG_QCACHE
logmsg("End in ::addEntry()");
#endif
}

// remove this entry from the cache
void QCache::deCache(CacheKey *stmt)
{
  if (stmt && stmt->amSafeToHash()) {
    CacheEntry* entry = cache_->getFirstValue
      (CONST_CAST(const CacheKey*, stmt));
    if (entry) {
      // decache postparser entry and its preparser instances
      deCachePostParserEntry(entry);
    }
  }
}

// decache a postparser cache entry
void QCache::deCachePostParserEntry(CacheEntry *entry)
{
  // decache its preparser entries
  deCachePreParserEntries
    (((CacheData*)(entry->data_.second_))->PreParserEntries());
  // decache postparser entry
  incNOfDisplacedEntries();

#ifdef DBG_QCACHE
  logmsg("begin QCache::deCachePostParserEntry");
  ULng32 x = heap_->getAllocSize();
#endif

  querycache_.getHQC()->delEntryWithCacheKey((CacheKey*)(entry->data_.first_));

  cache_->remove((CacheKey*)(entry->data_.first_));

#ifdef DBG_QCACHE
  ULng32 y = heap_->getAllocSize();
  logmsg("after call QCache::remove(CacheKey*)", x-y);
#endif

  clruQ_.erase(LRUList::iterator(entry));

#ifdef DBG_QCACHE
  x = heap_->getAllocSize();
  logmsg("after call clruQ_.erase()", y-x);
  logmsg("end QCache::deCachePostParserEntry");
#endif
}

// decache this preparser entry
void QCache::deCachePreParserEntry(CacheEntry *entry)
{
  incNOfDisplacedPreParserEntries();
  tHash_->remove((TextKey*)(entry->data_.first_));
  tlruQ_.erase(LRUList::iterator(entry));
}

// remove this preparser entry from the cache
void QCache::deCachePreParserEntry(TextKey *stmt)
{
  if (stmt && stmt->amSafeToHash()) {
    CacheEntry* entry = tHash_->getFirstValue
      (CONST_CAST(const TextKey*, stmt));
    if (entry) {
      deCachePreParserEntry(entry);
    }
  }
}

// decache an array of preparser cache entries
void QCache::deCachePreParserEntries(TextPtrArray &entries)
{
  CollIndex x, count = entries.entries();
  for (x=0; x<count; x++) {
    if ( entries[x] )
       deCachePreParserEntry((TextKey*)(entries[x]->data_.first_));
  }
}

// decache all entries that match this key
void QCache::deCacheAll(CacheKey *stmt, RelExpr *qry)
{
  // We want to decache all entries that reference a given table.
  Scan *scan;
  if (!qry || (scan = qry->getAnyScanNode()) == NULL) {
    // For inserts, it's sufficient to delete entries with matching keys.
    if (stmt && stmt->amSafeToHash()) {
      CacheEntry* entry = cache_->getFirstValue
        (CONST_CAST(const CacheKey*, stmt));
      while (entry) {
        deCachePostParserEntry(entry);
        entry = cache_->getFirstValue(CONST_CAST(const CacheKey*, stmt));
      }
    }
  }
  else {
    // For selects, we have to delete all entries that match our Scan node.
    LRUList::iterator mru = begin();
    while (mru != end()) {
      KeyDataPair& entry = *mru;
      ++mru; // advance iterator to next entry (before current can disappear)
      // does Scan text occur in entry's key?
      if (entry.first_ &&
          ((CacheKey*)(entry.first_))->contains(scan->getText())) { // yes.
        deCache((CacheKey*)(entry.first_)); // decache it.
      }
    }
  }
}

// decache all entries that match this preparser key
void QCache::deCacheAll(TextKey *stmt, RelExpr *qry)
{
  // NB: We're here because a preparser cache hit is being recompiled by the 
  // CLI executor because it failed similarity checks there. We need to 
  // decache not only this but also all preparser and postparser entries 
  // related to this stale stmt. We do this by getting our postparser 
  // representative entry and decaching it.
  if (stmt && stmt->amSafeToHash()) {
    CacheEntry* entry = tHash_->getFirstValue
      (CONST_CAST(const TextKey*, stmt));
    if (entry) { // decache only if it's in cache
      CacheEntry* repE = ((TextData*)(entry->data_.second_))->PostParserNode();

      // deCacheAll may delete repE->data_.first_ during normal operation
      // so a copy is contructed before calling deCacheAll().
      CacheKey stmtCopy(*(CacheKey*)repE->data_.first_, heap_);
      deCacheAll(&stmtCopy, qry);
    }
  }
  // else it's not in cache, so we're done
}

// increment number of compiles
void QCache::incNOfCompiles(IpcMessageObjType op)
{
  if ((op == CmpMessageObj::SQLTEXT_RECOMPILE)
      || (op == CmpMessageObj::SQLTEXT_STATIC_RECOMPILE)) {
    nOfRecompiles_++;
  } else {
    nOfCompiles_++;
  }
}

// increment number of cacheable compiles at this stage
void QCache::incNOfCacheableCompiles(CmpPhase stage)
{
  if (stage < N_STAGES) {
    nOfCacheableCompiles_[stage]++;
  }
}

// return number of cacheable compiles at this stage
ULng32 QCache::nOfCacheableCompiles(CmpPhase stage) const
{
  if (stage < N_STAGES) {
    return nOfCacheableCompiles_[stage];
  }
  return 0;
}

// increment number of cache hits at this stage
void QCache::incNOfCacheHits(CmpPhase stage)
{
  if (stage < N_STAGES) {
    nOfCacheHits_[stage]++;
  }
}

// return number of cache hits at this stage
ULng32 QCache::nOfCacheHits(CmpPhase stage) const
{
  if (stage < N_STAGES) {
    return nOfCacheHits_[stage];
  }
  // return the cummulative number of hits 
  else if( stage == N_STAGES )
  {
    ULng32 totalHits = 0;
    for( Int32 i = 0; i < N_STAGES; i++ )
    {
      totalHits += nOfCacheHits_[i];
    }
    return totalHits;
  }
  return 0;
}

// compute total number of plans in template cache
ULng32 QCache::nOfPlans()
{
  ULng32 plans = 0;
  LRUList::iterator lru = clruQ_.end();
  while (lru != clruQ_.begin()) {
    KeyDataPair& entry = *(--lru);
    Plan* p = ((CacheData*)entry.second_)->getPlan();
    p ->visitOnce();
    plans += ( p->getVisits() == p->getRefCount() ) ? 1 : 0;
  }

  // reset the visit counters for all plans visited
  lru = clruQ_.end();
  while ( lru != clruQ_.begin() ) {
    KeyDataPair& entry = *(--lru);
    ((CacheData*)entry.second_)->getPlan()->resetVisits();
  }

  return plans;
}

// increment number of cacheable entries that were too big for the cache
void QCache::incNOfCacheableButTooLarge()
{
  nOfCacheableButTooLarge_++;
}

// increment number of displaced entries
void QCache::incNOfDisplacedEntries(ULng32 howMany)
{
  nOfDisplacedEntries_ += howMany;
}

// increment number of displaced preparser entries
void QCache::incNOfDisplacedPreParserEntries(ULng32 howMany)
{
  nOfDisplacedPreParserEntries_ += howMany;
}

// Free all entries with specified QI Object Redefinition or Security Key
void QCache::free_entries_with_QI_keys( Int32 pNumKeys, SQL_QIKEY * pSiKeyEntry )
{
  LRUList::iterator lru = clruQ_.end();
  // loop thru query cache entries
  while ( (clruQ_.size() > 0) && (lru != clruQ_.begin()) )
  {
     KeyDataPair& entry = *(--lru);
     CacheData * cdata = (CacheData*)entry.second_ ;
     ComTdbRoot * rootTdb = (ComTdbRoot *)cdata->getPlan()->getPlan();

     char * base = (char *) rootTdb;
     const ComSecurityKey * planSet = rootTdb->getPtrToUnpackedSecurityInvKeys( base );
     CollIndex numPlanSecKeys = (CollIndex)(rootTdb->getNumberOfUnpackedSecKeys( base ));
     char SiKeyOpVal[4] ;
     SiKeyOpVal[2] = '\0'; //Put null terminator after first 2 chars

     NABoolean found = FALSE;
     // loop thru the keys passed as params
     for ( Int32 jj = 0; jj < pNumKeys  && !found; jj ++ )
     {
        SiKeyOpVal[0] = pSiKeyEntry[jj].operation[0];
        SiKeyOpVal[1] = pSiKeyEntry[jj].operation[1];
        ComQIActionType siKeyType =
          ComQIActionTypeLiteralToEnum( SiKeyOpVal );
        if (siKeyType == COM_QI_OBJECT_REDEF)
        {
          if (rootTdb->getNumObjectUIDs() > 0)
          {
            // this key passed in as a param is for object redefinition
            // (DDL) so look for matching ObjectUIDs.
            const Int64 *planObjectUIDs = rootTdb->
              getUnpackedPtrToObjectUIDs(base);
            for (Int32 ii = 0; ii < rootTdb->getNumObjectUIDs() &&
                               !found; ii++)  
            {
              if (planObjectUIDs[ii] ==  pSiKeyEntry[jj].ddlObjectUID)
                found = TRUE;
            }
          }
        }

       else if (siKeyType == COM_QI_USER_GRANT_ROLE)
        {
          for ( CollIndex ii = 0; ii < numPlanSecKeys && !found; ii ++ )
          {
            // If user ID's (subjects match)
            if ( ((pSiKeyEntry[jj]).revokeKey.subject == planSet[ii].getSubjectHashValue()) ||
                  qiSubjectMatchesRole(planSet[ii].getSubjectHashValue()) )
            {
               if ( ( pSiKeyEntry[jj]).revokeKey.object ==
                       planSet[ii].getObjectHashValue() &&
                    ( siKeyType == planSet[ii].getSecurityKeyType() ) )
                 found = TRUE;
            }
          }
        }

        else if (siKeyType != COM_QI_STATS_UPDATED)
        {
          // this key passed in as a param is for REVOKE so look
          // thru the plan's revoke keys.
          for ( CollIndex ii = 0; ii < numPlanSecKeys && !found; ii ++ )
          {
            // If user ID's (subjects match)
            if ( (pSiKeyEntry[jj]).revokeKey.subject ==
                    planSet[ii].getSubjectHashValue() ) 
            {
               // Remove all plans for user
               if ( ( pSiKeyEntry[jj]).revokeKey.object ==
                       planSet[ii].getObjectHashValue() &&
                    ( siKeyType == planSet[ii].getSecurityKeyType() ) )
              found = TRUE;
            }
          }
        }
     } // end loop thru the keys passed as params

     if ( found )
     {
        ++lru; // restart backward scan from entry's previous neighbor
        deCache((CacheKey*)(entry.first_)); // decache entry
     }
  } // end loop thru query cache entries
}

// free least recently used entries to make room for a new entry
NABoolean QCache::freeLRUentries(ULng32 newFreeBytes, ULng32 maxVictims)
{
  if (newFreeBytes <= getFreeSize()) {
    return TRUE;
  }

  // start freeing least recently used entries
  LRUList::iterator lru = clruQ_.end();
  ULng32 freedEntries = 0;
  // check list size to prevent infinite looping just in case, soln 10-090224-9516
  ULng32 size=clruQ_.size();
  while ( (getFreeSize() < newFreeBytes) && 
          (freedEntries < maxVictims) &&
          (clruQ_.size() > 0) && 
          (lru != clruQ_.begin()) ) {
    KeyDataPair& entry = *(--lru);
    ++lru; // restart backward scan from entry's previous neighbor
    deCache((CacheKey*)(entry.first_)); // decache entry
    freedEntries++;
    if (freedEntries>size) break;  // cannot free more than it has
  }
  return getFreeSize() >= newFreeBytes;
}


// free least recently used preparser entry to make room for a new entry
void QCache::freeLRUPreParserEntry()
{
  // free least recently used preparser entry
  LRUList::iterator lru = tlruQ_.end();
  if (lru != tlruQ_.begin()) {
    KeyDataPair& entry = *(--lru);
    deCachePreParserEntry((TextKey*)(entry.first_)); // decache entry
  }
}

// reconfigure cache to have new maxSize and new maxVictims
QCache* QCache::resizeCache(ULng32 maxSize, ULng32 maxVictims)
{
  // empty cache if maxSize <= 0
  if (maxSize <= 0) {
    incNOfDisplacedEntries(clruQ_.size());
    incNOfDisplacedPreParserEntries(tlruQ_.size());
    makeEmpty();
    maxSiz_ = 0;
  }
  else if (maxSize == maxSiz_) { // same size as before
    // so do nothing.
  }
  else { // desired size is nonzero and not same as before
    if (maxSize < maxSiz_) { // desired size is smaller
      // shrink cache to fit within maxSize
      ULng32 currentSize=heap_->getAllocSize();
      if (currentSize > maxSize) {
        // free LRU entries until cache is small enough
        //
        // Fix solution 10-060901-8773 (QCD6:qcache:decaching shows 
        // unexpected behavior).
        //
        // The number of bytes to be freed is mapped to the existing 
        // space of allocated heap size and maxSiz_ :  
        //
        // 
        //   getFreeSize(),  which is maxSiz_ - getAllocaSize() 
        //
        //                   plus 
        //
        //   currentSize - maxSize, which is the size of space occupied 
        //                          by entries to be freed
        //
        //                   minus
        //
        //   (totalHashTblSize_ * (1-maxSize/currentSize), 
        //                          which is a portion of the hash table 
        //                          itself that can be freed, assuming 
        //                          hash table size grows propotionally to
        //                          the number of entries in the hash table
        //                          which grows propotionally to the 
        //                          total amount of allocated heap space. 
        //                          This amount is substracted because 
        //                          we want to keep entries that can use this 
        //                          amount of space. 
        //
        freeLRUentries(getFreeSize() + currentSize - maxSize -
                 ULng32(totalHashTblSize_ * 
                         (1 - float(maxSize) / currentSize)), 
                 INT_MAX);
      }
    }
    else { // desired size is bigger
      // all entries will fit into a bigger cache.
    }
    // reconfigure cache_ hashtable to have less or more buckets
    NADELETE(cache_,CacheHashTbl,heap_);

    ULng32 x = heap_->getAllocSize();
    cache_ = new(heap_) CacheHashTbl
      (hashKeyFunc, nBuckets(maxSize, avgPlanSize()), TRUE, heap_);

    totalHashTblSize_ = heap_->getAllocSize() - x;
    // insert surviving entries into new cache_
    LRUList::iterator mru = clruQ_.begin();
    // check list size to prevent infinite looping just in case, soln 10-090224-9516
    ULng32 size=clruQ_.size();
    ULng32 cnt=0;
    for (; mru != clruQ_.end(); ) {
      CacheKey *reinsert = cache_->insert
        ((CacheKey*)((*mru).first_), mru.node_);

      // It is possible that some plans in the previous hashtable
      // cannot be inserted into the new hashtable.  This is due to
      // the order in which the plans are originally inserted into the
      // cache.  As an example, consider two statements that differ
      // only in the size of a constant.  One has a char(5) constant
      // and the other has a char(10) constant.  If the char(5)
      // statement is prepared first, it is placed in the cache.  Then
      // later, the char(10) statement is prepared and is also placed
      // into the cache, since it will not get a hit from the char(5).
      // Now, due to the way the cache lookup is performed, the
      // char(5) statement in the cache is effectively masked and will
      // never get a hit.  The more general char(10) statement will
      // always be found first.  Now if the cache size is changed (and
      // that is why we are here), these statements will be reinserted
      // into the hashtable in MRU order.  Since the char(5) statement
      // is masked by the char(10) statement, the char(10) statement
      // is more recently used and will therefore be inserted first.
      // Then, when the char(5) statement is inserted into the
      // hashtable, it gets a hit on the char(10) statement and is not
      // inserted.  So, in effect, when changing the size of the
      // cache, the masked plans are removed.
      // On the other hand, if the char(10) statement was prepared first,
      // the char(5) statement would get a hit on the char(10) statement.
      //
      // Remember that the CacheKey == operator is not commutative:
      //    char(5) == char(10)
      //    char(10) != char(5)
      //
      if(reinsert == NULL) {
        // this entry is redundant, it cannot be inserted.
        // delete its preparser entries before erasing it.
        deCachePreParserEntries
          (((CacheData*)((*mru).second_))->PreParserEntries());
        incNOfDisplacedEntries();
        
        // Erase returns the next element in list
        // no need to advance iterator.
        mru = clruQ_.erase(mru);
      } else {
        mru++;
      }
      if (++cnt >= size) break;
    }

    // Can not delete tHash_ earlier because deCachePreParserEntries()
    // call potentially made above can access tHash_.
    NADELETE(tHash_,TextHashTbl,heap_);
    ULng32 maxEntries = maxPreParserEntries(maxSize, avgTextEntrySize());

    ULng32 numBuckets = npBuckets(maxEntries);
    tHash_ = new(heap_) TextHashTbl(hashTextFunc, numBuckets, TRUE, heap_);
    // check list size to prevent infinite looping just in case, soln 10-090224-9516
    size=tlruQ_.size();
    cnt=0;
    for (mru = tlruQ_.begin(); mru != tlruQ_.end(); ) {
      TextKey *reinsert = tHash_->insert
        ((TextKey*)((*mru).first_), mru.node_);
      if(reinsert == NULL) {
        // Erase returns the next element in list
        // no need to advance iterator.
        mru = tlruQ_.erase(mru);
      } else {
        mru++;
      }
      if (++cnt >= size) break;
    }
    maxSiz_ = maxSize;
  }
  // adjust limit_
  limit_ = maxVictims;
  return this;
}

// return average plan size of existing entries if any or
// return previous average plan size if cache has no entries
ULng32 QCache::avgPlanSize()
{
  // if there are no cache entries then return previous average
  if (clruQ_.size() <= 0) {
    return planSz_;
  }

  // otherwise recompute and return the actual average plan size
  ULng32 totalSz = 0;
  for (LRUList::iterator mru = clruQ_.begin(); mru != clruQ_.end(); ++mru) {
    KeyDataPair& entry = *mru;
    int cnt = ((CacheData*)entry.second_)->getPlan() -> getRefCount();
    if (cnt <= 0) cnt = 1;
    totalSz += (entry.first_->getSize() + entry.second_->getSize()) +
      ((CacheData*)entry.second_)->getPlan() -> getSize() / cnt;
  }

  if (totalSz > 0 AND clruQ_.size() > 0)
    planSz_ = totalSz / clruQ_.size();

  return planSz_;
}

// return average text entry size
ULng32 QCache::avgTextEntrySize()
{
  // if there are no cache entries then return previous average
  if (tlruQ_.size() <= 0) {
    return tEntSz_;
  }

  // otherwise recompute and return the actual average text entry size
  ULng32 totalSz = 0, newSz;
  for (LRUList::iterator mru = tlruQ_.begin(); mru != tlruQ_.end(); ++mru) {
    KeyDataPair& entry = *mru;
    totalSz += (entry.first_->getSize() + entry.second_->getSize());
  }
  if ((newSz=totalSz / tlruQ_.size()) > 0) {
    tEntSz_ = newSz;
  }
  return tEntSz_;
}

NABoolean QCache::isCachingOn() const 
{ 
  return maxSiz_ > 0 && CmpCommon::getDefault(QUERY_TEMPLATE_CACHE) == DF_ON;
}

NABoolean QCache::isPreparserCachingOn(NABoolean duringAddEntry) const 
{ 
  // if QTC is OFF, no text caching is to be done.
  if (CmpCommon::getDefault(QUERY_TEXT_CACHE) == DF_OFF)
    return FALSE;

  // if QTC is SKIP, then qtc should not be done during compile phase,
  // but should be done during the add phase after a query has been compiled.
  // This ensures that a query that was compiled without cache, is added
  // to the text cache.
  if ((CmpCommon::getDefault(QUERY_TEXT_CACHE) == DF_SKIP) &&
      (NOT duringAddEntry))
    return FALSE;

  if (CmpCommon::getDefault(AUTO_QUERY_RETRY) == DF_OFF)
    return FALSE;

  // A volatile tab and a regular table may have the same name.
  // That would get incorrect query from the qcache during text match. This
  // would not get detected by timestamp mismatch at runtime since the 2
  // tables are physically different.
  // Turn off text cache, if volatile tables are in use.
  if (CmpCommon::getDefault(VOLATILE_SCHEMA_IN_USE) == DF_ON)
    return FALSE;

  // turn off text cache, if there is a CQS in use
  if (ActiveControlDB() && ActiveControlDB()->getRequiredShape() &&
      ActiveControlDB()->getRequiredShape()->getShape())
    return FALSE;

  // text caching could be used.
  // It could still be turned off if the previously generated code did
  // not use auto-query-retry. At runtime, use of AQR is a requirement for
  // text cached generated plans to be executed.
  return TRUE;
}

// return maximum number of preparser entries
ULng32 QCache::maxPreParserEntries(ULng32 maxByteSz, ULng32 avgEntrySz)
{
  // assume text cache size <= 20% of text+template cache size
  ULng32 limit = (maxByteSz / 5) / avgEntrySz;
  return limit;
}

// load-time initialization
//THREAD_P QCache* QueryCache::cache_ = NULL;

QueryCache::QueryCache(ULng32 maxSize, ULng32 maxVictims, ULng32 avgPlanSz)
{
  cache_ = NULL;
  hqc_ = NULL;
  resizeCache(maxSize, maxVictims, avgPlanSz);
  parameterTypes_ = new (CTXTHEAP) NAString(parmTypesInitStrLen, CTXTHEAP);
}


// return an iterator positioned at beginning of query cache's LRU list
LRUList::iterator QueryCache::begin()
{
  CMPASSERT(cache_ != NULL);
  return cache_->begin();
}

// return an iterator positioned at beginning of preparser cache's LRU list
LRUList::iterator QueryCache::beginPre()
{
  CMPASSERT(cache_ != NULL);
  return cache_->beginPre();
}

// return an iterator positioned at end of query cache's LRU list
LRUList::iterator QueryCache::end()
{
  CMPASSERT(cache_ != NULL);
  return cache_->end();
}

// return an iterator positioned at end of preparser cache's LRU list
LRUList::iterator QueryCache::endPre()
{
  CMPASSERT(cache_ != NULL);
  return cache_->endPre();
}

void QueryCache::shutdownCache()
{
  // Only free the cache when the context heap is not null. During some
  // assertion processing, the CmpContext may be destroyed along with the
  // query cache. Deleting the cache in that case will cause runtime
  // failures and abends. 
  if (CTXTHEAP != NULL)
  {
    delete cache_;
    cache_ = NULL;
  }
}

// free all memory allocated to cache
void QueryCache::finalize(const char *staticOrDynamic)
{
  if (cache_) {
    QueryCache::shutdownCache();
  }
  // else nothing to do if no cache

  // free parameterTypes
  if (parameterTypes_) {
    NADELETE(parameterTypes_, NAString, CTXTHEAP);
    parameterTypes_ = NULL;
  }
}

// get query cache statistics used by CompilationStats
// return FALSE if cache size is 0
NABoolean QueryCache::getCompilationCacheStats(QCacheStats &stats)
{
  if (!cache_) {
    memset(&stats, 0, sizeof(stats));
  }
  else {
    stats.currentSize = cache_->byteSize();
    stats.nRecompiles = cache_->nOfRecompiles();
    stats.nCacheableP = cache_->nOfCacheableCompiles(A_PARSE);
    stats.nCacheableB = cache_->nOfCacheableCompiles(A_BIND);
    stats.nCacheHitsPP = cache_->nOfCacheHits(A_PREPARSE);
    stats.nCacheHitsP = cache_->nOfCacheHits(A_PARSE);
    stats.nCacheHitsB = cache_->nOfCacheHits(A_BIND);
    stats.nCacheHitsT = cache_->nOfCacheHits(N_STAGES);
    stats.nLookups = cache_->nOfLookups();
  }

  if (cache_->maxSize() <= 0)
     return FALSE;

  return TRUE;
}

void QueryCache::getHQCStats(HybridQueryCacheStats & stats)
{
    if(!hqc_)
    {
        memset(&stats, 0, sizeof(stats));
    }
    else{
        stats.nHKeys = hqc_->getEntries();
        stats.nSKeys = hqc_->getNumSQCKeys();
        stats.nMaxValuesPerKey = hqc_->getMaxEntriesPerKey();
        stats.nHashTableBuckets = hqc_->getNumBuckets();
    }
}

void QueryCache::getHQCEntryDetails(HQCCacheKey* hkey, HQCCacheEntry* entry, HybridQueryCacheDetails & details)
{
    if(!hqc_)
    {
        memset(&details, 0, sizeof(details));
    }
    else{
        details.planId = entry->getSQCKey()->getPlanId();
        details.hkeyTxt = hkey->getKey();
        details.skeyTxt = entry->getSQCKey()->getText();
        details.nHits = entry->getNumHits();
        details.nOfPConst = entry->getParams()->getConstantList().entries();

        for(int i = 0; i < entry->getParams()->getConstantList().entries(); i++){
            details.PConst += entry->getParams()->getConstantList()[i]->getConstValue()->getText();
            details.PConst += "\n";
        }
        
        details.nOfNPConst = 0;
        for(int i = 0; i < entry->getParams()->getNPLiterals().entries(); i++)
            if(!entry->getParams()->getNPLiterals()[i].isNull())
                details.nOfNPConst++; 
                     
        for(int i = 0; i < entry->getParams()->getNPLiterals().entries(); i++)
            if(!entry->getParams()->getNPLiterals()[i].isNull()){
                details.NPConst += entry->getParams()->getNPLiterals()[i];
                details.NPConst += "\n";
            }
    }
}
// get query cache statistics
void QueryCache::getCacheStats(QueryCacheStats &stats)
{
  if (!cache_) {
    memset(&stats, 0, sizeof(stats));
  }
  else {
    stats.avgPlanSize = cache_->avgPlanSize();
    stats.s.currentSize = cache_->byteSize();
    stats.highWaterMark = cache_->highWaterMark();
    stats.maxSize = cache_->maxSize();
    stats.maxVictims = cache_->maxVictims();
    stats.nEntries = cache_->nOfPostParserEntries();
    stats.nPlans = cache_->nOfPlans();
    stats.nCompiles = cache_->nOfCompiles();
    stats.s.nLookups = cache_->nOfLookups();
    stats.s.nRecompiles = cache_->nOfRecompiles();
    stats.nRetries = cache_->nOfRetries();
    stats.s.nCacheableP = cache_->nOfCacheableCompiles(A_PARSE);
    stats.s.nCacheableB = cache_->nOfCacheableCompiles(A_BIND);
    stats.s.nCacheHitsPP = cache_->nOfCacheHits(A_PREPARSE);
    stats.s.nCacheHitsP = cache_->nOfCacheHits(A_PARSE);
    stats.s.nCacheHitsB = cache_->nOfCacheHits(A_BIND);
    stats.s.nCacheHitsT = cache_->nOfCacheHits(N_STAGES);
    stats.nTooLarge = cache_->nOfCacheableButTooLarge();
    stats.nDisplaced = cache_->nOfDisplacedEntries();
    stats.avgTEntSize = cache_->avgTextEntrySize();
    stats.nTextEntries = cache_->nOfPreParserEntries();
    stats.nDispTEnts = cache_->nOfDisplacedTextEntries();    
    stats.intervalWaterMark = cache_->intervalWaterMark();
  }
  stats.optimLvl = CompilerEnv::defToken2OptLevel
    (CmpCommon::getDefault(OPTIMIZATION_LEVEL, 0));
  stats.envID = 0;
}

// get details of this query cache entry
void QueryCache::getEntryDetails
(LRUList::iterator i,        // (IN) : query cache iterator entry
 QueryCacheDetails &details) // (OUT): cache entry's detailed statistics
{
  if (!cache_) {
    memset(&details, 0, sizeof(details));
  }
  else {
    Key *ckey = (*i).first_;
    CData *cdat = (*i).second_;
    details.planId = cdat->getPlan()->getId();
    details.qryTxt = (ckey->am() == Key::TEXTKEY) ? ((TextKey*)ckey)->getText()
                      : cdat->getOrigStmt();
    details.entrySize = ckey->getSize() + cdat->getSize() + 
                        sizeof(CacheEntry) + 
                        CacheHashTbl::getBucketEntrySize();

    details.planLength = ((ckey->am() == Key::TEXTKEY)) ?
     ((TextData*)cdat)->PostParserEntry()->getPlan()->getSize() :
     ((CacheData*)(cdat))->getPlan()->getSize() ;

    details.nOfHits = cdat->getHits();
    details.phase = ckey->getCmpPhase();
    details.optLvl = ckey->getOptLvl();
    details.envID = 0;
    details.catalog = ckey->getCatalog();
    details.schema = ckey->getSchema();
    details.nParams = ckey->getNofParameters();
    details.paramTypes = ckey->getParameterTypes(parameterTypes_);
    details.compTime = cdat->getCompTime();
    details.avgHitTime = cdat->getAvgHitTime();
    details.reqdShape = ckey->getReqdShape();
    details.isoLvl = ckey->getIsoLvl();
    details.isoLvlForUpdates = ckey->getIsoLvlForUpdates();
    details.accMode = ckey->getAccessMode();
    details.autoCmt = ckey->getAutoCommit();
    details.flags = ckey->getFlags();
    details.rbackMode = ckey->getRollbackMode();
  }
}

// resize query cache
void QueryCache::resizeCache(ULng32 maxSize, ULng32 maxVictims, ULng32 avgPlanSz)
{
  if (cache_ != NULL) {
    cache_ = cache_->resizeCache(maxSize, maxVictims);
  }
  else {
    cache_ = new CTXTHEAP QCache(*this, maxSize, maxVictims, avgPlanSz);
  }

  //do the same for Hybrid Query Cache
  // set HQC HashTable to have same bucket number as CacheKey HashTable

  ULng32 numBuckets = cache_->getNumBuckets();
  if (hqc_ != NULL) {
      hqc_ = hqc_->resizeCache(numBuckets);
  }
  else {
      hqc_ = new (CTXTHEAP) HybridQCache(*this, numBuckets);
  }
}

void QueryCache::setQCache(QCache *qCache)
{
   cache_ = qCache;
}

// add a new postparser entry into the cache
CacheKey* QueryCache::addEntry
(TextKey            *tkey,   // (IN) : preparser key
 CacheKey           *stmt,   // (IN) : postparser key
 CacheData          *plan,   // (IN) : sql statement's compiled plan
 TimeVal&            begT,   // (IN) : time at start of this compile
 char               *params, // (IN) : parameters for preparser entry
 ULng32       parmSz) // (IN) : len of params for preparser entry
{ 
  return (cache_) ? 
    cache_->addEntry(tkey, stmt, plan, begT, params, parmSz) : NULL;
}

// add a new preparser entry into the cache
void QueryCache::addPreParserEntry
(TextKey            *tkey,   // (IN) : a cachable sql statement
 char               *actuals,// (IN) : actual parameters
 ULng32       actLen, // (IN) : len of actuals
 CacheEntry         *entry,  // (IN) : postparser cache entry
 TimeVal            &begT)   // (IN) : time at start of this compile
{ 
  if (cache_) {
    cache_->addPreParserEntry(tkey, actuals, actLen, entry, begT); 
  }
}

void QueryCache::free_entries_with_QI_keys( Int32 NumSiKeys , SQL_QIKEY * pSiKeyEntry )
{
  if (cache_) {
    cache_->free_entries_with_QI_keys( NumSiKeys, pSiKeyEntry );
  }
}

KeyDataPair::~KeyDataPair()
{

#ifdef DBG_QCACHE
   NAHeap *heap = NULL;
   ULng32 x, y;

   if ( first_ ) {
     heap = first_->heap();
     x = heap -> getAllocSize();
     logmsg("begin ~KeyDataPair()");
   }
#endif

    if ( first_ ) {
      NADELETE(first_,Key,first_->heap()); first_=NULL;
#ifdef DBG_QCACHE
   y = heap -> getAllocSize();
   logmsg("after delete first_", x-y);
#endif
    }

    if ( second_ ) {
      NADELETE(second_,CData,second_->heap()); second_=NULL;
#ifdef DBG_QCACHE
   x = heap -> getAllocSize();
   logmsg("after delete second_", y-x);
#endif
    }

#ifdef DBG_QCACHE
   if ( first_ )
     logmsg("end ~KeyDataPair()\n");
#endif
}


void HQCParseKey::bindConstant2SQC(BaseColumn* base, ConstantParameter* cParameter, LIST(Int32) &hqcConstPos)
{
   if ( !cParameter->getConstVal() || cParameter->getConstVal()->getOperatorType() != ITM_CONSTANT ||!isCacheable())
        return;
      
    //loop over ConstValues collected in parser, 
    //and find one corresponding to constVal,
    //the matched one will be marked parameterized, and move to ConstantList_
    //its correspondant in NPLiterals will be set empty.
    hqcConstant* matchedItem = NULL;
    LIST (hqcConstant*) & tmpL = *(getParams().getTmpList());
    for(Int32 i = paramStart_; i < tmpL.entries(); i++)
    {
      if( (tmpL[i]->getConstValue() == cParameter->getConstVal() 
               || tmpL[i]->getBinderRetConstVal() == cParameter->getConstVal())
           && !tmpL[i]->isProcessed()
        )
      {
          matchedItem = tmpL[i];
          matchedItem->setIsParameterized(TRUE);
          matchedItem->setProcessed();
          matchedItem->setSQCType(cParameter->getType()->newCopy(heap()));
          //the pos is index of new list
          hqcConstPos.insert(getParams().getConstantList().entries());
          //set corresponding literal empty as a sign of parameterized
          getParams().getNPLiterals()[matchedItem->getIndex()] = "";
          //also add to new list for furture compare/backpatch
          getParams().getConstantList().insert(matchedItem);
          numConsts_++;
          //next time, the loop will start from paramStart_
          //for performence good.
          paramStart_ = i + 1;
          break;
      }
    }
    // If the constant cannot be found in HQC key, due to multiple 
    // identical constant values bound into a single ConstValue 
    // object, mark the key not cacheable and return. 
    if(!matchedItem) {
      setIsCacheable(FALSE);
      return;
   }

   //if this constant doesn't need histogram info, the work is done.
   if (!base)  
     return;
   CMPASSERT(base->getOperatorType()==ITM_BASECOLUMN);
   //then get histogram info for it.
       
   // check if there exists a histogram for the column
   ColStatsSharedPtr cStatsPtr = 
        (base->getTableDesc()->tableColStats()).
               getColStatsPtrForColumn(base->getValueId());

    if (cStatsPtr == 0)
      return;

    // if the stats for this column is fake or the number of intervals is 1, there
    // is no need to mark the boundary as all values will have the same selectivity
    // just mark this item as no stats required
    if (cStatsPtr->isFakeHistogram() ||  cStatsPtr->isSingleIntHist())
    {
        return;
    }

    const EncodedValue encodedConstVal(cParameter->getConstVal(), FALSE);

    // check frquent value list first.
    const FrequentValueList & fvList = cStatsPtr->getFrequentValues();
    const FrequentValue fv(0, 0, 1, encodedConstVal);

    CollIndex fvIndex;
    if ( fvList.getfrequentValueIndex(fv, fvIndex) ) {
       matchedItem->addRange(encodedConstVal, encodedConstVal, TRUE, TRUE);
       matchedItem->setIsLookupHistRequired(TRUE);
       return;
    }

    // next check each interval in the histogram
    HistogramSharedPtr hist = cStatsPtr->getHistogram();

    // was this histogram compressed to reduce the number of intervals
    NABoolean histogramReduced = cStatsPtr->afterFetchIntReductionAttempted();

    Interval last = hist->getLastInterval();
    for ( Interval iter = hist->getFirstInterval(); ; iter.next())
    {
        if ( !iter.isValid() || iter.isNull() ) {
             if ( iter == last ) {//cannot be found in any intervals.
                 setIsCacheable(FALSE);
                 break;
             }
             else
                 continue;
        }
        // if the value contains in the current interval, save
        // the range info for future HybridQCache key comparison.
        if ( iter.containsValue(encodedConstVal) ) 
        {
            NABoolean loBoundInclusive = iter.isLoBoundInclusive();
            NABoolean hiBoundInclusive = iter.isHiBoundInclusive();
            EncodedValue loBound = iter.loBound();
            EncodedValue hiBound = iter.hiBound();
         
            if (!histogramReduced)
            {
               //Column whose histogram is referred to by this ColStats object
               const NAColumn * column = cStatsPtr->getStatColumns()[0];
               Criterion reductionCriterion = cStatsPtr->decideReductionCriterion (AFTER_FETCH,CRITERION1,column,TRUE);
               hist->computeExtendedIntRange(iter, reductionCriterion, hiBound, loBound, hiBoundInclusive, loBoundInclusive);
            }
            matchedItem->addRange(hiBound, loBound, hiBoundInclusive, loBoundInclusive);
            matchedItem->setIsLookupHistRequired(TRUE);
            break;
        }
        if ( iter == last ) 
            break;
    }//for
}

void HQCParseKey::FixupForUnaryNegate(BiArith* itm)
{
  if(!this->isCacheable()
    ||getParams().getNPLiterals().entries() <= 0 
    ||getParams().getTmpList()->entries() <=0
    ||!itm->isUnaryNegate())
    return;

  ConstValue* zero = (ConstValue*)itm->getChild(0);
  if(zero->getText().compareTo("0")!=0)
    return;
  ConstValue* unarycv = (ConstValue*)itm->getChild(1);
  CollIndex lastNPIndex = getParams().getNPLiterals().entries() - 1;
  //insert 0 to NPLiterals at penultimate position
  getParams().getNPLiterals().insertAt(lastNPIndex, NAString("0"));
  //create hqcConstant for 0
  hqcConstant * hqcConst = 
   new (heap_) hqcConstant( (ConstValue*)zero, 
                               lastNPIndex,
                               heap_ );
  //insert hqcConstant of 0 to tmpList at penultimate position
  CollIndex lastTmpIndex = getParams().getTmpList()->entries() - 1;                             
  getParams().getTmpList()->insertAt(lastTmpIndex, hqcConst);
  //update index info
  hqcConstant* unaryhqc = (*getParams().getTmpList())[lastTmpIndex+1];
  unaryhqc->getIndex() = lastNPIndex + 1;
}

void HQCParseKey::collectItem4HQC(ItemExpr* itm)
{
  CMPASSERT(itm);
  if(isCacheable())
  {
    if(itm->getOperatorType() == ITM_CONSTANT)
    {
        // The new hpcConstant will contain the same pointer to 
        // itm. Later on, when call HQCParseKey::collectItem4HQC(),
        // we will assure that that the histogram is collected for
        // the same constant.  The constructor hqcConstant::hqcConstant()
        // used has to guarantee that the same pointer is stored in
        // the object. 
        CMPASSERT(getParams().getNPLiterals().entries()>0);
        hqcConstant * constPtr = 
           new (heap_) hqcConstant( (ConstValue*)itm, 
                                    getParams().getNPLiterals().entries() -1,
                                    heap_ );
        getParams().getTmpList()->insert(constPtr);
    }
    else if(itm->getOperatorType() == ITM_DYN_PARAM)
    {
        for(Int32 i = 0; i < HQCDynParamMap_.entries(); i ++)
        {
            if(HQCDynParamMap_[i].original_ == ((DynamicParam*)itm)->getText()){

                hqcDynParam * dynPtr = 
                    new (heap_) hqcDynParam((DynamicParam*)itm,
                                            params_.getDynParamList().entries(),
                                            HQCDynParamMap_[i].normalized_,
                                            HQCDynParamMap_[i].original_);
                params_.getDynParamList().insert(dynPtr);
                return;
            }
        }
    }
  }
}

void HQCParseKey::collectBinderRetConstVal4HQC(ConstValue * origin, ConstValue* after)
{
    if( origin == after) //do nothing if bindNode didn't replace origin ConstValue
        return;
    //if ConstValue is replaced during bindNode,
    //save pointer of new ConstValue, which will be used in bindConstant2SQC,
    //to identify corresponding hqcConstant
    LIST (hqcConstant*) & tmpL = *(getParams().getTmpList());
    for(Int32 i = 0; i < tmpL.entries(); i++)
        if(tmpL[i]->getConstValue() == origin)
            tmpL[i]->setBinderRetConstVal(after);
}

// This operator is used to lookup the HQC hash table, only requiring a match
// on the parent class, the HQC key text and the constant and param lists. 
NABoolean HQCCacheKey::operator==(const HQCCacheKey &other) const
{
    return ( Key::isEqual(other) && this->keyText_ == other.keyText_ &&
             numDynParams_ == other.numDynParams_  &&
             reqdShape_ == other.reqdShape_
           );
}


NABoolean HQCCacheEntry::operator==(const HQCCacheEntry &other) const
{
  return TRUE;
}

NABoolean HistIntRangeForHQC::constains(const EncodedValue & value) const
{
   if ( value < loBound_ || hiBound_ < value )
      return FALSE;

   if ( loBound_ < value && value < hiBound_ )
       return TRUE ;

   if ( loBound_ == value )
      return loInclusive_;

   if ( hiBound_ == value )
      return hiInclusive_;

   return FALSE;
}

NABoolean hqcConstant::isApproximatelyEqualTo(hqcConstant & other) 
{
  //for literals do not need to lookup histogram
  // only make sure type is compatible, when coverting from other to *this,
  // also fall into this categoray, if all histogram stats are uniform 
  if( getIndex() == other.getIndex()
      && isParameterized()
      && !isLookupHistRequired()) //Constants of SQL functions don't need to lookup histograms.               
  {
    CMPASSERT(this->getSQCType());
    // check type compatability for SQL function constant parameters

#if 1    
    //Check if an error can occur when coverting coming ConstValue()(other) to target( this->getConstValue() )
    if(!other.getConstValue()->getType()->isCompatible(*(this->getSQCType())) 
       || !other.getConstValue()->canBeSafelyCoercedTo(*(this->getSQCType())) )
       return FALSE;
#else     
    if(other.getConstValue()->getType()->errorsCanOccur(*(this->getSQCType()), FALSE))
        return FALSE;
     //string literal ConstValue of ISO88591 can be convert to UTF8 encoded column     
    if(!other.getConstValue()->getType()->isCompatible(*(this->getSQCType())) )
    {
         if(this->getSQCType()->getTypeQualifier() == NA_CHARACTER_TYPE 
            && other.getConstValue()->getType()->getTypeQualifier() == NA_CHARACTER_TYPE
            && this->getSQCType()->getCharSet() == CharInfo::UTF8 
            && other.getConstValue()->getType()->getCharSet() == CharInfo::ISO88591 )
         {
            //for string ConstValue, if source charset is CharInfo::ISO88591 target is CharInfo::UTF8
            //consider them compatible
         }
         else
            return FALSE;
    }
#endif

    other.setIsParameterized(TRUE);
    return TRUE;
  }
  
  //for literals need to lookup histogram
  //cached hqcConstant should be parameterized, lookupHistRequired
  if( getIndex() == other.getIndex()
      && isParameterized()
      && isLookupHistRequired()
    )
  {
      CMPASSERT(this->getSQCType());
      const EncodedValue encodedConstVal(other.getConstValue(), FALSE);
      //histogram range should be collected
      CMPASSERT(this->getRange());
      if(this->getRange()->constains(encodedConstVal)){
            other.setIsParameterized(TRUE);
            return TRUE;
      }
  }
  return FALSE;
}

hqcConstant::~hqcConstant()
{
    if(histRange_)
       NADELETE(histRange_, HistIntRangeForHQC, heap_);
}

void hqcConstant::addRange(const EncodedValue & hiBound, const EncodedValue & loBound, NABoolean hiinc, NABoolean loinc)
{
  CMPASSERT(histRange_ == NULL);
  histRange_ = new (heap_) HistIntRangeForHQC(hiBound, loBound, hiinc, loinc, heap_);
}
 

HybridQCache::HybridQCache(QueryCache & qc, ULng32 nOfBuckets) 
 : querycache_(qc), heap_(new CTXTHEAP NABoundedHeap("hybrid query cache heap", (NAHeap *)CTXTHEAP, 0, 0))
 , hashTbl_(new (heap_) HQCHashTbl(hashHQCKeyFunc, nOfBuckets, TRUE, heap_))// enforce uniqueness of keys
 , maxValuesPerKey_(CmpCommon::getDefaultLong(HQC_MAX_VALUES_PER_KEY))
 , currentKey_(NULL)
 , HQCLogFile_(NULL)
 , planNoAQROrHiveAccess_(FALSE)
{}

HybridQCache::~HybridQCache()
{
   // free all memory owned by hybrid query cache heap
   NADELETE(heap_, NABoundedHeap, CTXTHEAP);
   delete HQCLogFile_;
}

void HybridQCache::invalidateLogging()
{
    delete HQCLogFile_;
    HQCLogFile_ = NULL;
}

void HybridQCache::initLogging()
{
   NAString logFile = ActiveSchemaDB()->getDefaults().getValue(HQC_LOG_FILE);
   DefaultToken DF = CmpCommon::getDefault(HQC_LOG);

   if(HQCLogFile_ == NULL &&  DF == DF_ON) {
       HQCLogFile_ = new ofstream(logFile, std::ofstream::app);
   }
   else if(HQCLogFile_ && DF == DF_ON) 
   {//re-open everytime for logging
       HQCLogFile_->close();
       HQCLogFile_->open(logFile, std::ofstream::app);
   }
}

NABoolean HybridQCache::addEntry(HQCParseKey* hkey, CacheKey* ckey)
{
    //transfer ownership to context heap;
    HQCCacheKey* _hkey = new (heap_) HQCCacheKey (*hkey, heap_);

    // check if we have seen this key before
    HQCHashTblItor hqcItor(*hashTbl_, _hkey);

    HQCCacheKey* hqcKeyPtr = NULL;
    HQCCacheData* valueList = NULL;
    CacheKey* cKeyPtr = NULL;

    hqcItor.getNext(hqcKeyPtr, valueList);

    HQCCacheEntry* storedValue = new (heap_) HQCCacheEntry (hkey->getParams(), ckey, heap_);

    // we have seen this before, we need to make sure we have enough space in the list
    // if yes add it, otherwise we need to eject the LRU item
    if (valueList)
    {
       if (valueList->addOrReplace())
          valueList->insert(storedValue);
       return TRUE;
       
    }
    else // we have not seen this key before, just add it
    {
        HQCCacheData* valueList = new (heap_) HQCCacheData (heap_, maxValuesPerKey_);
        valueList->insert(storedValue);
        return (hashTbl_->insert(_hkey, valueList) != NULL);
    }

    return NULL;
}

NABoolean HybridQCache::lookUp(HQCParseKey * hkey, CacheKey* & ckey)
{
    HQCCacheKey* hkey2 = new (heap_) HQCCacheKey (*hkey, heap_);
    HQCHashTblItor hqcItor(*hashTbl_, hkey2);

    HQCCacheKey* hqcKeyPtr = NULL;
    HQCCacheData* valueList = NULL;
    CacheKey* cKeyPtr = NULL;

    hqcItor.getNext(hqcKeyPtr, valueList);

    // The passed-in hkey contains only the text key and constants. The
    // key stored in hqc hash table (hqcKeyPtr) contains more information.
    // Here we want to verfiy that each constant in hkey is within the 
    // corresponding histogram interval boundaries the histogram specified
    // in hqcKeyPtr.
    if(hqcKeyPtr && valueList)
    {
       if (cKeyPtr = valueList->findMatchingSQCKey(hkey->getParams()))
       {
           ckey = cKeyPtr;
           return TRUE;
       }
    }
    return FALSE;
}

NABoolean HybridQCache::delEntryWithCacheKey(CacheKey* ckey)
{
    //iterate to remove entries with value ckey

    HQCHashTblItor hqcHashItor(*hashTbl_);

    HQCCacheKey* hkeyPtr = NULL;
    HQCCacheData* valueList = NULL;
    CacheKey* ckeyPtr = NULL;

    hqcHashItor.getNext(hkeyPtr, valueList) ; 

    while ( hkeyPtr && valueList )
    {
       if (valueList->removeEntry(ckey))
       {
          if( valueList->entries() == 0)
          {
             deCache(hkeyPtr);
             delete valueList;
          }
          return TRUE;
       }

       hqcHashItor.getNext(hkeyPtr, valueList) ; 
    }

    return FALSE;
} 

ULng32 HybridQCache::getNumSQCKeys() const 
{
    //count number of SQCKeys in HQC
	//one HQCkey entry may have multiple SQCKeys
    ULng32 counter = 0;
    
    HQCHashTblItor hqcHashItor(*hashTbl_);

    HQCCacheKey* hkeyPtr = NULL;
    HQCCacheData* valueList = NULL;
    CacheKey* ckeyPtr = NULL;

    hqcHashItor.getNext(hkeyPtr, valueList) ; 

    while ( hkeyPtr && valueList )
    {
       counter += valueList->entries();
       hqcHashItor.getNext(hkeyPtr, valueList) ; 
    }
    return counter;
}

HybridQCache* HybridQCache::resizeCache(ULng32 numBuckets)
{
  // empty cache if numBuckets <= 0
  if (numBuckets <= 0) {
     clear();
  }
  else if (numBuckets == getNumBuckets()) { // same size as before
    // do nothing.
  } else { 
     // this method has not been implemented fully.
     //hashTbl_->resize(numBuckets);
     if(getNumBuckets() < numBuckets)
     {
        //create a bigger hash table dictionary
        HQCHashTbl* newHashTable = new (heap_) HQCHashTbl (hashHQCKeyFunc, 
                                      numBuckets,
                                      TRUE,   // enforce uniqueness of keys
                                      heap_); // use this bounded heap

         //move data from old hash dictionary to new
        HQCHashTblItor itor(*hashTbl_);
        HQCCacheKey* keyPtr = NULL;
        HQCCacheData* valueList = NULL;
        
        itor.getNext(keyPtr, valueList);
        while(keyPtr&&valueList)
        {
            newHashTable->insert(keyPtr, valueList);
            itor.getNext(keyPtr, valueList);
        }
        //delete old one, and swith to new one
        delete hashTbl_;
        hashTbl_ =newHashTable;
        
     }
  }
  return this;
}

void HybridQCache::clear()
{
  NAHashDictionaryIterator<HQCCacheKey, HQCCacheData> Iter (*hashTbl_);
  HQCCacheKey* hk = NULL;
  HQCCacheData* hkvl = NULL;

  Iter.getNext (hk,hkvl) ; 
  while ( hk && hkvl )
  {
      delete hk;
      delete hkvl;
      Iter.getNext (hk, hkvl) ; 
  }

  hashTbl_->clear();
}

//Set HQCParseKey::isCacheable=FALSE if non-cacheable case is detected.
void HQCParseKey::verifyCacheability(CacheKey* ckey)
{
   if(!this->isCacheable())
     return;
   NAList <hqcConstant*> & paramConstList = getParams().getConstantList();
   for(Int32 i = 0; i < paramConstList.entries(); i++)
   {
      if( paramConstList[i]->getConstValue() 
          && paramConstList[i]->isParameterized()
          && !paramConstList[i]->isProcessed()
          )
      {
         setIsCacheable(FALSE);
         return;
      }
      //or
      if (paramConstList[i]->isLookupHistRequired()
          && NULL == paramConstList[i]->getRange()
          )
      {
         setIsCacheable(FALSE);
         return;
      }
      //or
      Int32 literalIndex = paramConstList[i]->getIndex();
      if(!getParams().getNPLiterals()[literalIndex].isNull())
      {
         setIsCacheable(FALSE);
         return;
      }
      
   }

   if ( ckey == NULL ||
        numConsts_ != ckey->getNofParameters() ) 
   {
     setIsCacheable(FALSE);
     return;
   }
}

// This constructor is used add a hqc constant into a HQC key. It is 
// very important to keep the pointer to cv. Never construct a new 
// ConstValue object here.
hqcConstant::hqcConstant(ConstValue* cv,
                Int32 index,
                NAHeap* h)
      : constValue_(cv)
      , binderRetConstVal_(cv)
      , SQCType_(NULL)
      , histRange_(NULL)
      , index_(index)
      , heap_(h)
      , flags_(0)
{}

// This constructor is called to transfer HQC key to CmpContext.
hqcConstant::hqcConstant(const hqcConstant & other, NAHeap* h)
      : constValue_(new (h) ConstValue(*other.constValue_, h))
      , binderRetConstVal_(constValue_)
      , SQCType_(NULL)
      , histRange_(NULL)
      , index_(other.index_)
      , heap_(h)
      , flags_(other.flags_)
{
    // we need a new copy of the type_ member of ConstValue as
    // the ConstValue construcotr does not do a deep copy of this attribute
    constValue_->changeType (other.constValue_->getType()->newCopy(h));

    //create copy of SQC NAType from parameterized source 
    if(other.getSQCType() && other.isParameterized()) 
      SQCType_ = other.getSQCType()->newCopy(h);

    if(other.histRange_)
      histRange_ = new (heap_) HistIntRangeForHQC(*(other.histRange_), heap_);
}

HQCCacheKey::HQCCacheKey(CompilerEnv* e, NAHeap* h)
                : Key(CmpMain::PARSE, e, h)
                , keyText_(h)
                , numConsts_(0)
                , numDynParams_(0)
{
    RelExpr* reqdShapeExp = NULL;
    NAString reqdShape = "";
    if (ActiveControlDB() && ActiveControlDB()->getRequiredShape() && ActiveControlDB()->getRequiredShape()->getShape())
    {
        ActiveControlDB()->getRequiredShape()->getShape()->unparse(reqdShape);
    }
    reqdShape_ = NAString(reqdShape, h);
}

HQCCacheKey::HQCCacheKey(const HQCParseKey& hkey, NAHeap* h)
          : Key(hkey, h)
          , keyText_(hkey.keyText_.data(), h)
          , reqdShape_(hkey.getReqdShape(), h)
          , numConsts_(hkey.getParams().getConstantList().entries())
          , numDynParams_(hkey.getParams().getDynParamList().entries())
{}

HQCParseKey::HQCParseKey(CompilerEnv* e, NAHeap* h)
                : HQCCacheKey(e, h)
                , params_(h)
		, HQCDynParamMap_(h)
                , isCacheable_(FALSE)
                , isStringNormalized_(FALSE)
                , nOfTokens_(0)
                , paramStart_(0)
{}

HQCCacheEntry::~HQCCacheEntry()
{
    if (params_)
       delete params_;

    sqcCacheKey_ = NULL;
}

HQCCacheEntry::HQCCacheEntry(NAHeap* h)
                : params_(NULL)
                , sqcCacheKey_ (NULL)
                , numHits_(0)
                , heap_(h)
{}

HQCCacheEntry::HQCCacheEntry(const HQCParams& params, CacheKey* sqcCacheKey, NAHeap* h)
                : heap_(h)
                , params_(new (heap_) HQCParams (params, heap_))
                , sqcCacheKey_(sqcCacheKey)
                , numHits_(0)
{}

HQCParams::HQCParams(NAHeap* h)
  : heap_(h)
  , ConstantList_(h)
  , DynParamList_(h)
  , NPLiterals_(h)
  , tmpList_(new (STMTHEAP) NAList<hqcConstant*>(STMTHEAP))
{}

   // copy constructor
HQCParams::HQCParams (const HQCParams & other, NAHeap* h)
  : heap_(h)
  , ConstantList_(h)
  , DynParamList_(h)
  , NPLiterals_(other.NPLiterals_, h)
{
  for(Int32 i = 0; i < other.ConstantList_.entries(); i++)
  {
     hqcConstant* src = other.ConstantList_[i];
     hqcConstant* des = new (h) hqcConstant(*src, h);
     ConstantList_.insert(des);
  }

  for(Int32 i = 0; i < other.DynParamList_.entries(); i++)
  {
      hqcDynParam* src = other.DynParamList_[i];
      hqcDynParam* des = new (h) hqcDynParam(*src);
      DynParamList_.insert(des);
  }
  //tmpList_ shall be discarded, after a sucessful hit or addEntry  
}
   
HQCParams::~HQCParams()
{
   for(Int32 i = 0; i < ConstantList_.entries(); i++)
      delete ConstantList_[i];
      
   for(Int32 i = 0; i < DynParamList_.entries(); i++)
      delete DynParamList_[i];
}

NABoolean HQCParams::isApproximatelyEqualTo (HQCParams& otherParam) 
{
    //compare non-parameteriezed & parameterized literals in one run
    NAList <hqcConstant* > & otherTmpL = *(otherParam.getTmpList());
    for(CollIndex i = 0, h = 0, x = 0; i < this->getNPLiterals().entries(); i++)
    {   
        
        if(!this->getNPLiterals()[i].isNull())
        {   //return FALSE if non-parameterized literal is not identical
            if(otherParam.getNPLiterals()[i].compareTo(this->getNPLiterals()[i], NAString::exact))
                return FALSE;
                
            //ConstValue might also be created for non-parameterized literal.
            //we need to advance to skip these non-parameterized ConstValue.
            if(otherTmpL.entries() > h && i > otherTmpL[h]->getIndex())
                h++;
        }
        else//this means we hit a parameterized literal
        {
            //find the parameterized constant in otherTmpL that corresponds to the one in this->getConstantList(), then compare them.
            
            //if h is pointing to preceding parameterized literal, advance to current one(which is also a parameterized literal)
            if(otherTmpL.entries() > h && i > otherTmpL[h]->getIndex())
                h++;
             //if otherParam.getNPLiterals()[i] is a parameterized literal, otherTmpL[i]->getIndex() == i MUST be true.
             CMPASSERT(otherTmpL[h]->getIndex() == i);

             hqcConstant* parameterizedLiteral = otherTmpL[h];

             CMPASSERT(parameterizedLiteral);
             //ConstValue empty string(like '') is non-parameterized literal.
             //Launchpad bug 1409863
             if(parameterizedLiteral->getConstValue()->isEmptyString())
                 return FALSE;       
      
             //add it to otherParam.getConstantList(), if all items are match, we will use it for backpatch, 
             //otherwise, empty otherParam.getConstantList() after comparing all literals.
             otherParam.getConstantList().insert(parameterizedLiteral);
             
             //do parameterized comparison
             if(!this->getConstantList()[x]->isApproximatelyEqualTo(*parameterizedLiteral))
                    return FALSE;

             x++;
         }
         // The algorithm above is base on following truth: 
         // it is possible a literal in this->getNPLiterals()(or otherParam.getNPLiterals()) doesn't have counterpart in otherTmpL, but the reverse is not true.
         // it is also possible that hqcConstant in otherTmpL will not be a parameterized constant,
         // if this->getNPLiterals()[i].isNull() is true, it MUST have a counterpart in otherTmpL whose getIndex()==i.
         // so indexes
         //               i  -- index of all literals, no matter with ConstValues or not, paremeterized or not, 
         //               h -- index of literals with ConstValues, 
         //               x -- index of parameterized constants
         // The indexes of the three lists aren't corresponding,  
         // but we can figure out items that corresponding in the three lists, by calculating these indexes.
         
    }

    // we don't support dynamic parameters for now, so they are ignored
    
    return TRUE;
}

// search the list of values for the value associated with the given params
// if found, increment the number of cache hits
CacheKey* HQCCacheData::findMatchingSQCKey (HQCParams& param)
{
   for (CollIndex x = 0; x < entries(); x++)
   {
      HQCParams* entryParams = (*this)[x]->getParams();
      if (entryParams && entryParams->isApproximatelyEqualTo(param))
      {
         // increment the number if hits for this entry
         (*this)[x]->incrementNumHits();
         return (*this)[x]->getSQCKey();
      }
      else
       //hqcConstants might be added to param.getConstantList() 
       //in HQCParams::isApproximatelyEqualTo() for possible backpatch, 
       //if comparing fails, param.getConstantList() must be cleared.
          param.getConstantList().clear();
   }

   return NULL;
}

// logic to decide whether we need to replace an exisiting
// value for a given key. This is needed if the maximum
// allowed number of values is exhausted
NABoolean HQCCacheData::addOrReplace()
{

  // there is space, just add the new value
  if (this->entries() < this->maxEntries_)
     return TRUE;

  Int32 minHits = (*this)[0]->getNumHits();
  CollIndex minHitsLoc = 0;

  // list is full, we need to eject the LRU entry
  // find it first
  for (CollIndex x = 1; x < entries(); x++)
   {
      Int32 numHits = (*this)[x]->getNumHits();
      if (numHits < minHits)
      {
         minHits = numHits;
         minHitsLoc = x;
      }
   }

   // now remove it
   HQCCacheEntry* entryToRemove = (*this)[minHitsLoc];
   ostream* hqc_ostream=CURRENTQCACHE->getHQCLogFile();
   if (hqc_ostream)
   {
      *hqc_ostream << "  -- HQC cache entry replaced. Entry has <" << entryToRemove->getNumHits() << "> hits \n";
   }
   this->removeAt(minHitsLoc);
   delete entryToRemove;
   return TRUE;

}

// remove a given value from the list of values that can be associated with an HQC key
NABoolean HQCCacheData::removeEntry (CacheKey* sqcCacheKey)
{
  NABoolean entryRemoved = FALSE;
  for (CollIndex j = 0; j < entries(); j++)
  {
    if ((*this)[j]->getSQCKey() == sqcCacheKey)
    {
        HQCCacheEntry* currEntry = (*this)[j];
        this->removeAt(j);
        delete currEntry;
        entryRemoved = TRUE;
        j--;
    }
  }

  return entryRemoved;
}

QueryCacheStatsISPIterator::QueryCacheStatsISPIterator(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                           SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h)
: ISPIterator(ctxs, h), currQCache_(NULL)
{
    initializeISPCaches(inputData, eFunc, error, ctxs, contextName_, currCacheIndex_);
    //initialize currQCache_ here
    //set currQCache_ to CURRENTQCACHE, and set to NULL after fetch one row.
    if(currCacheIndex_ == -1);
      currQCache_ = CURRENTQCACHE;
}

QueryCacheEntriesISPIterator::QueryCacheEntriesISPIterator(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                             SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h)
: ISPIterator(ctxs, h), counter_(0), currQCache_(NULL)
{
     initializeISPCaches( inputData, eFunc, error, ctxs, contextName_, currCacheIndex_);
     //initialize currQCache_ here
     if(currCacheIndex_ == -1);
         currQCache_ = CURRENTQCACHE;
     //iterator start with preparser cache entries of first query cache, if any
     if(currCacheIndex_ == -1 && currQCache_)
         SQCIterator_ = currQCache_->beginPre();
     else if(currCacheIndex_ == 0)
         SQCIterator_ = ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache()->beginPre();
}

HybridQueryCacheStatsISPIterator::HybridQueryCacheStatsISPIterator(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                                      SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h)
 : ISPIterator(ctxs, h), currQCache_(NULL)
{
    initializeISPCaches( inputData, eFunc, error, ctxs, contextName_, currCacheIndex_);
    //initialize currQCache_ here
    //set currQCache_ to CURRENTQCACHE, and set to NULL after fetch one row.
    if(currCacheIndex_ == -1);
       currQCache_ = CURRENTQCACHE;
}

HybridQueryCacheEntriesISPIterator::HybridQueryCacheEntriesISPIterator(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                                        SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h)
: ISPIterator(ctxs, h)
, currEntryIndex_(-1)
, currEntriesPerKey_(-1)
, currHKeyPtr_(NULL)
, currValueList_(NULL)
, HQCIterator_(NULL)
, currQCache_(NULL)
{
    initializeISPCaches( inputData, eFunc, error, ctxs, contextName_, currCacheIndex_);
    //initialize currQCache_ here
    if(currCacheIndex_ == -1);
       currQCache_ = CURRENTQCACHE;
    if(currCacheIndex_ == -1 && currQCache_)
       HQCIterator_ = new (heap_) HQCHashTblItor (currQCache_->getHQC()->begin());
    else if(currCacheIndex_ == 0)
       HQCIterator_ = new (heap_) HQCHashTblItor (ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache()->getHQC()->begin());
}

HybridQueryCacheEntriesISPIterator::~HybridQueryCacheEntriesISPIterator()
{
    if(HQCIterator_)
        delete HQCIterator_;
}

//if currCacheIndex_ is set 0, currQCache_ is not used and should always be NULL
NABoolean QueryCacheStatsISPIterator::getNext(QueryCacheStats & stats)
{
   //Only for remote tdm_arkcmp with 0 context
   if(currCacheIndex_ == -1 && currQCache_)
   {
      currQCache_->getCacheStats(stats);
      currQCache_ = NULL;
      return TRUE;
   }
   
   //fetch QueryCaches of all CmpContexts with name equal to contextName_
   if(currCacheIndex_ > -1 && currCacheIndex_ < ctxInfos_.entries())
   {
      if( !ctxInfos_[currCacheIndex_]->isSameClass(contextName_.data()) //current context name is not equal to contextName_
       && contextName_.compareTo("ALL", NAString::exact)!=0) //and contextName_ is not "ALL"
      {// go to next context in ctxInfos_
          currCacheIndex_++;
          return getNext(stats);
      }
      ctxInfos_[currCacheIndex_++]->getCmpContext()->getQueryCache()->getCacheStats(stats);
      return TRUE;
   }
   //all entries of all caches are fetched, we are done!
   return FALSE;
}

NABoolean QueryCacheEntriesISPIterator::getNext(QueryCacheDetails & details)
{
   //Only for remote tdm_arkcmp with 0 context
   if( currCacheIndex_ == -1 && currQCache_ )
   {
       if (SQCIterator_ == currQCache_->endPre()) {
           // end of preparser cache, continue with postparser entries
           SQCIterator_ = currQCache_->begin();
       }
       if (SQCIterator_ != currQCache_->end())
           currQCache_->getEntryDetails(SQCIterator_++, details);
       else
           return FALSE; //all entries are fetched, we are done!
             
       return TRUE;
   }

   //fetch QueryCaches of all CmpContexts with name equal to contextName_
   if(currCacheIndex_ > -1 && currCacheIndex_ < ctxInfos_.entries())
   {
        if( !ctxInfos_[currCacheIndex_]->isSameClass(contextName_.data()) //current context name is not equal to contextName_
         && contextName_.compareTo("ALL", NAString::exact)!=0) //and contextName_ is not "ALL"
        {// go to next context in ctxInfos_
          currCacheIndex_++;
          if(currCacheIndex_ < ctxInfos_.entries()){
              //initialize iterator to first cache entry of next query cache, if any
              SQCIterator_ = ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache()->beginPre();
          }
          return getNext(details);
        }
        
        QueryCache * qcache = ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache();
        if(SQCIterator_ == qcache->endPre()) {
              // end of preparser cache, continue with postparser entries
             SQCIterator_ = qcache->begin();
        }
        
        if(SQCIterator_ != qcache->end()){
             qcache->getEntryDetails( SQCIterator_++, details);
             return TRUE;
        }
        else
        {
            //end of this query cache, try to get from next in ctxInfos_
            currCacheIndex_++;
            if(currCacheIndex_ < ctxInfos_.entries()){
                //initialize iterator to first cache entry of next query cache, if any
                SQCIterator_ = ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache()->beginPre();
            }
            //let deeper getNext() decide.
            return getNext(details);
        }
   }
   //no more query caches, we are done.
   return FALSE;
}

NABoolean HybridQueryCacheStatsISPIterator::getNext(HybridQueryCacheStats & stats)
{
   //Only for remote tdm_arkcmp with 0 context
   if(currCacheIndex_ == -1 && currQCache_)
   {
      currQCache_->getHQCStats(stats);
      currQCache_ = NULL;
      return TRUE;
   }
   //fetch QueryCaches of all CmpContexts with name equal to contextName_
   if(currCacheIndex_ > -1 && currCacheIndex_ < ctxInfos_.entries())
   {
      if( !ctxInfos_[currCacheIndex_]->isSameClass(contextName_.data()) //current context name is not equal to contextName_
       && contextName_.compareTo("ALL", NAString::exact)!=0) //and contextName_ is not "ALL"
      {// go to next context in ctxInfos_
          currCacheIndex_++;
          return getNext(stats);
      }
      ctxInfos_[currCacheIndex_++]->getCmpContext()->getQueryCache()->getHQCStats(stats);
      return TRUE;
   }
   //all entries of all caches are fetched, we are done!
   return FALSE;
}

NABoolean HybridQueryCacheEntriesISPIterator::getNext(HybridQueryCacheDetails & details)
{
   //Only for remote tdm_arkcmp with 0 context
   if( currCacheIndex_ == -1 && currQCache_ )
   {
      if( currEntryIndex_ >= currEntriesPerKey_ )
      {
          //get next key-value pair
          HQCIterator_->getNext(currHKeyPtr_, currValueList_);
          if(currHKeyPtr_ && currValueList_ && currQCache_->isCachingOn())
          {
             currEntryIndex_ = 0;
             currEntriesPerKey_ = currValueList_->entries();
          }
          else//interator == end, all entries fetched, we are done!
              return FALSE;
      }
      //just get next entry in previous valueList
      currQCache_->getHQCEntryDetails(currHKeyPtr_, (*currValueList_)[currEntryIndex_++], details);
      return TRUE;
   }
 
   //fetch QueryCaches of all CmpContexts with name equal to contextName_
   if(currCacheIndex_ > -1 && currCacheIndex_ < ctxInfos_.entries())
   {
      if( !ctxInfos_[currCacheIndex_]->isSameClass(contextName_.data()) //current context name is not equal to contextName_
       && contextName_.compareTo("ALL", NAString::exact)!=0) //and contextName_ is not "ALL"
      {// go to next context in ctxInfos_
          currCacheIndex_++;
          if(currCacheIndex_ < ctxInfos_.entries())
          {  //initialize HQCIterator to begin of next query cache, if any
             if(HQCIterator_)
               delete HQCIterator_;
             HQCIterator_ = NULL;
             HQCIterator_ = new (heap_) HQCHashTblItor (ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache()->getHQC()->begin());
          }
          return getNext(details);
      }
      
      if( currEntryIndex_ >= currEntriesPerKey_ )
      {
         //get next key-value pair
         HQCIterator_->getNext(currHKeyPtr_, currValueList_);
         QueryCache * tmpCache = ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache();
         if(currHKeyPtr_ && currValueList_ && tmpCache->isCachingOn())
         {
            currEntryIndex_ = 0;
            currEntriesPerKey_ = currValueList_->entries();
         }
         else//interator == end
         {  
            currCacheIndex_++;
            if(currCacheIndex_ < ctxInfos_.entries())
            {  //initialize HQCIterator to begin of next query cache, if any
               if(HQCIterator_)
                 delete HQCIterator_;
               HQCIterator_ = NULL;
               HQCIterator_ = new (heap_) HQCHashTblItor (ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache()->getHQC()->begin());
            }
            //let deeper getNext() decide.
            return getNext(details);
         }
      }
      //just get next entry in previous valueList
      QueryCache * qcache = ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache();
      qcache->getHQCEntryDetails(currHKeyPtr_, (*currValueList_)[currEntryIndex_++], details);
      return TRUE;
   }
   //no more query caches, we are done.
   return FALSE;
}

QueryCacheDeleter::QueryCacheDeleter(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                             SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h)
:ISPIterator(ctxs, h), currQCache_(NULL)
{
  initializeISPCaches( inputData, eFunc, error, ctxs, contextName_, currCacheIndex_);
  //initialize currQCache_ here
  if(currCacheIndex_ == -1);
     currQCache_ = CURRENTQCACHE;
}

void QueryCacheDeleter::doDelete()
{
   //if currCacheIndex_ is set 0, currQCache_ is not used and should always be NULL
   
   //Only for remote tdm_arkcmp with 0 context
   if(currCacheIndex_ == -1 && currQCache_)
   {
      currQCache_->makeEmpty();
   }
   
   //clear QueryCaches of all CmpContexts with name equal to contextName_
   if(currCacheIndex_ > -1 && currCacheIndex_ < ctxInfos_.entries())
   {
      for(;currCacheIndex_ < ctxInfos_.entries(); currCacheIndex_++)
      {
          if(contextName_.compareTo("ALL", NAString::exact)==0)
              ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache()->makeEmpty();
          else if(ctxInfos_[currCacheIndex_]->isSameClass(contextName_.data()))
              ctxInfos_[currCacheIndex_]->getCmpContext()->getQueryCache()->makeEmpty();
      }
   }
}
