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
* File:         ItemCache.cpp
* Description:  All the ItemExpr methods introduced by query caching
* Created:      2/23/2001
* Language:     C++
*
*
******************************************************************************
*/
#include "AllItemExpr.h"
#include "CacheWA.h"
#include "CmpMain.h"
#include "NumericType.h"
#include "SchemaDB.h"
#include "ItemFuncUDF.h"

void computeAndAddSelParamIfPossible(
             CacheWA& cwa, BindWA& bindWA, ExprValueId& child,
             BaseColumn *base, ConstValue *val)
{
    ColStatsSharedPtr cStatsPtr = (base->getTableDesc()->tableColStats()).
                             getColStatsPtrForColumn(base->getValueId());

    if (cStatsPtr == NULL )
        return;

    HistogramSharedPtr hist = cStatsPtr->getHistogram();

    if ( hist == NULL )
       return;

    CostScalar sel;
    NABoolean canComputeSelectivity = hist -> computeSelectivityForEquality(
            val, cStatsPtr->getRowcount(), cStatsPtr->getTotalUec(),
            sel);

    if ( canComputeSelectivity == TRUE ) {
      const NAType * newType = base->getNAColumn()->getType();

      // for char datatypes, assign the caseinsensitive attribute from
      // the const node.
      if (newType->getTypeQualifier() == NA_CHARACTER_TYPE)
	{
	  newType = 
	    base->getNAColumn()->getType()->newCopy(cwa.wHeap());
	  ((CharType*)newType)->setCaseinsensitive(((CharType*)val->getType())->isCaseinsensitive());
	}
      
      cwa.replaceWithNewOrOldSelParam(val, newType, Selectivity(sel), 
                                      child, bindWA);

    }
}

ConstValue * SelParameter::castToConstValue(NABoolean & negate_it)
{
  negate_it = FALSE;
  return getConstVal();
}

// append an ascii-version of aggregate into cachewa.qryText_
void Aggregate::generateCacheKey(CacheWA& cwa) const
{
  ItemExpr::generateCacheKey(cwa); 
  cwa += " all:"; 
  cwa += isDistinct_ ? "a0" : "a1";
}

// is any literal in this expr safely coercible to its target type?
NABoolean Assign::isSafelyCoercible(CacheWA &cwa) const
{
  if (cwa.getPhase() >= CmpMain::BIND) {
    // we have to disallow caching of the following types of updates:
    //   update t set col = overlylongliteral
    ItemExpr *src = getSource().getItemExpr();
    if (src->getOperatorType() == ITM_CONSTANT) {
      // source is a literal; make sure this update is cacheable only if
      // literal can be safely coerced into its target type.
      return ((ConstValue*)src)->canBeSafelyCoercedTo(getTarget().getType());
    }      
    else { // source is not a literal
      // we need to descend into this expr to verify that no 
      // errors can occur during backpatching. For example, 
      //   update t set i = i + 123456789012345678901234567890
      // should not be cacheable if i is a smallint.
      // reject "update t set c=(subquery)" as noncacheable
      // as part of a fix to CR 10-020108-8401.
      return src->isSafelyCoercible(cwa) && src->isCacheableExpr(cwa);
    }
  }
  return FALSE;
}

// change literals of a cacheable query into ConstantParameters 
ItemExpr* Assign::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  if (cwa.getPhase() == CmpMain::PARSE) {
    child(1) = child(1)->normalizeForCache(cwa, bindWA);
  }
  else if (cwa.getPhase() >= CmpMain::BIND) {
    ItemExpr *leftC=child(0), *rightC=child(1);
    OperatorTypeEnum leftO = leftC->getOperatorType();
    OperatorTypeEnum rightO = rightC->getOperatorType();
    ConstantParameter *cParam;
    if (leftO == ITM_BASECOLUMN && rightO == ITM_CONSTANT
        // normalizeForCache only constants that can be safely backpatched.
        // part of fix to CR 10-010726-4109.
        && isSafelyCoercible(cwa)  
        // normalizeForCache only constants that are contained in the original
        // SQL query so that each parameter can be backpatched by the
        // corresponding constant from the query. This is to fix regression 
        // failure MP core/test055 where a SystemLiteral was inserted when the 
        // Keytag for a MP table column is not zero. see LeafInsert::bindNode().
        // The offending query in MP core/test055 is
        //   update t055t9v set b = b + 1 where current of c10;
        && NOT ((ConstValue*)rightC)->isSystemProvided() ) {

      ConstValue *val = (ConstValue*)rightC;
      if (!val->isNull()) {
        cParam = new (cwa.wHeap()) ConstantParameter
          ((ConstValue*)rightC, cwa.wHeap(),
          ((BaseColumn*)leftC)->getNAColumn()->getType());
        cwa.addConstParam(cParam, bindWA);
        child(1) = cParam;
      }
         // else val is null; keep null as is.
         // this is part of a fix to genesis case: 10-010618-3484.
    }
    else {
      child(1) = child(1)->normalizeForCache(cwa, bindWA);
    }
  }
  markAsNormalizedForCache();
  return this;
}

// is val a constant that can be safely coerced to BaseColumn's type?
NABoolean BaseColumn::canSafelyCoerce(ItemExpr& val) const
{
  NAColumn *baseCol = getNAColumn();
  OperatorTypeEnum valTyp = val.getOperatorType();
  return
    (baseCol != NULL &&
    ((valTyp == ITM_CONSTANT &&
      ((ConstValue*)&val)->canBeSafelyCoercedTo(*baseCol->getType()))
     // we don't need to check the types of param & hostvar here because we
     // are called after ItemExpr::bindNode has successfully type-checked
     // the current search predicate that is being tested for cacheability
     || valTyp == ITM_HOSTVAR
     || valTyp == ITM_DYN_PARAM 
     || valTyp == ITM_CACHE_PARAM
     ));
}

// append an ascii-version of ItemExpr into cachewa.qryText_
void BaseColumn::generateCacheKey(CacheWA& cwa) const
{
  if (getTableDesc()->getCorrNameObj().getPrototype() == NULL) {
    // return the table in the format "table.col"
    ColRefName name
      (getTableDesc()->getNATable()->getNAColumnArray()[colNumber_]->
       getColName(),
       getTableDesc()->getCorrNameObj(), CmpCommon::statementHeap());
    cwa += name.getColRefAsAnsiString();
  }
  else {
    ColRefName name
      (getTableDesc()->getNATable()->getNAColumnArray()[colNumber_]->
       getColName(), CmpCommon::statementHeap());
    cwa += name.getColRefAsAnsiString();
  }
}

// is this BaseColumn a primary or partitioning key and is val a single 
// value or a constant that can be safely coerced to BaseColumn's type?
NABoolean BaseColumn::isKeyColumnValue(ItemExpr& val) const
{
  NAColumn *baseCol = getNAColumn();
  return
    (baseCol != NULL &&
     (baseCol->isPrimaryKey() || baseCol->isPartitioningKey()) &&
     canSafelyCoerce(val));
}

// append an ascii-version of BETWEEN into cachewa.qryText_
void Between::generateCacheKey(CacheWA& cwa) const
{ 
  cwa += leftBoundryIncluded_ ? "L1" : "L0";
  cwa += rightBoundryIncluded_ ? "R1" : "R0";
  if (pDirectionVector_) {
    cwa += "dV";
    Int32 x, limit=pDirectionVector_->entries();
    for (x = 0; x<limit; x++) {
      cwa += ((pDirectionVector_->at(x) == -1) ? "d" : "a");
    }
  }
  BuiltinFunction::generateCacheKey(cwa);
}

// append an ascii-version of biarith 
void BiArith::generateCacheKey(CacheWA& cwa) const
{
  ItemExpr::generateCacheKey(cwa); 

  // append an indication of rounding mode for datetime arithmetic functions
  if ( isKeepLastDay() ) cwa += "r1";
  else if ( isStandardNormalization() ) cwa += "r0";

  if (getOperatorType() == ITM_DIVIDE)
    {
      cwa += " arm:"; // arith rounding mode
      char dFmt[20]; 
      str_itoa(roundingMode_, dFmt); 
      cwa += dFmt;
    }
}

// change literals of a cacheable query into ConstantParameters 
ItemExpr* BiArith::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) {
    return this;
  }
  if (!isUnaryNegate()) { // set in sqlparser.y which transforms "-i" into
    // "0 - i" because apparently the executor does not have a unary negate.
    // so, we must avoid replacing this system-introduced "0".
    child(0) = child(0)->normalizeForCache(cwa, bindWA);
  }
  child(1) = child(1)->normalizeForCache(cwa, bindWA);
  markAsNormalizedForCache();
  return this;
}

// we want BiLogic to be cacheable
NABoolean BiLogic::isCacheableExpr(CacheWA& cwa)
{
  if (cwa.getPhase() >= CmpMain::BIND && getArity() == 2) {
    if (getOperatorType() == ITM_AND) {
      ItemExpr *leftC=child(0), *rightC=child(1);
      // we want to descend to both left & right
      // so cwa.usedKeys & cwa.keyCols get updated
      NABoolean leftOK = leftC->isCacheableExpr(cwa);
      NABoolean rightOK = rightC->isCacheableExpr(cwa);
      // return FALSE if both left & right are not cacheable.
      if (!leftOK && !rightOK) 
        return FALSE;

      // allow "select * from t where k1=1 and k2=2" as potentially cacheable
      return TRUE;
      // the definitive check whether a query's predicate covers
      // all referenced tables' key columns can be (and is) done
      // only in RelRoot::isCacheableExpr() when cwa.usedKeys &
      // cwa.keyCols are fully defined. Otherwise, queries like
      //   "select * from t1 join t2 on a=x and a=7 where x=7"
      // may be incorrectly rejected as non-cacheable if t1 has
      // primary key(a) and t2 has primary key(x).
    }
    else if (getOperatorType() == ITM_OR) {
      return TRUE;
      // ORs can be cacheable but are not necessarily
      // parameterizable, see Bilogic::normalizeForCache below.
    }
  }
  return FALSE;
}

// selectively change literals of a cacheable query into input parameters 
ItemExpr* BiLogic::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  if (getOperatorType() == ITM_AND) {
    return ItemExpr::normalizeForCache(cwa, bindWA);
  }
  else { 
    // do not replace literals in OR predicates
    markAsNormalizedForCache();
    return this;
  }
}

// append an ascii-version of ItemExpr into cachewa.qryText_
void BiRelat::generateCacheKey(CacheWA& cwa) const
{
  cwa += specialNulls_ ? "s1" : "s0";
  cwa += specialMultiValuePredicateTransformation_ ? "m1" : "m0";
  cwa += isaPartKeyPred_ ? "p1" : "p0";
  if (directionVector_) {
    cwa += "Dv";
    Int32 x, limit=directionVector_->entries();
    for (x = 0; x<limit; x++) {
      cwa += ((directionVector_->at(x) == -1) ? "d" : "a");
    }
  }
  // we are deliberately excluding from the cachekey the following BiRelat
  // data members: likeSelectivity_, originalLikeExprId_
  // because we believe they are "internal" variables, ie, they are not
  // essential to distinguishing between two different queries.
  ItemExpr::generateCacheKey(cwa);
}

// we want BiRelat to be cacheable
NABoolean BiRelat::isCacheableExpr(CacheWA& cwa)
{
  if (cwa.getPhase() >= CmpMain::BIND && getArity() == 2) {
    if (getOperatorType() == ITM_EQUAL) {
      ItemExpr *leftC=child(0), *rightC=child(1);
      OperatorTypeEnum leftO = leftC->getOperatorType();
      OperatorTypeEnum rightO = rightC->getOperatorType();
      BaseColumn *base;
      if (leftO == ITM_BASECOLUMN) {
        base = (BaseColumn*)leftC;
        if (base->isKeyColumnValue(*rightC)) {
          cwa.addToUsedKeys(base);
        }
        return TRUE;
      }
      else if (rightO == ITM_BASECOLUMN) {
        base = (BaseColumn*)rightC;
        if (base->isKeyColumnValue(*leftC)) {
          cwa.addToUsedKeys(base);
        }
        return TRUE;
      }
      else if (leftO == ITM_ITEM_LIST && rightO == ITM_ITEM_LIST &&
               ((ItemList*)leftC)->isListOfCacheableSelPred
               (cwa, (ItemList*)rightC)) {
        return TRUE;
      }
      else {
        // we want all other equality comparisons to be cacheable, eg,
        // retail_div_cd||acct_type_cd||substring(acct_id,1,8)='20V43085193'
        return TRUE;
      }
    }
    else {
      return TRUE;
      // other binary comparison predicates can be cacheable, but are
      // not necessarily parameterizable, see BiRelat::normalizeForCache
    }
  }
  return FALSE;
}

// is it safe to parameterize this selection predicate term?
// change literals of a cacheable query into input parameters 
ItemExpr* BiRelat::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (cwa.getPhase() >= CmpMain::BIND) {
    // NB: we assume here that when a query is cacheable because it has a key
    //     equi-predicate, then its key equi-predicates can be parameterized
    if (getArity() == 2) {
      if (getOperatorType() == ITM_EQUAL) {
        // normalizeForCache only constants that can be safely backpatched.
        // part of fix to CR 10-010726-4109.
        ItemExpr *leftC=child(0), *rightC=child(1);
        OperatorTypeEnum leftO = leftC->getOperatorType();

        // fix case 10-061027-0129: discover the potential base column 
        // below the InstantiateNull node
        if ( leftO == ITM_INSTANTIATE_NULL ) {
           leftC = leftC->child(0);
           leftO = leftC->getOperatorType();
        }

        OperatorTypeEnum rightO = rightC->getOperatorType();

        // fix case 10-061027-0129.
        if ( rightO == ITM_INSTANTIATE_NULL ) {
           rightC = rightC->child(0);
           rightO = rightC->getOperatorType();
        }

        if (leftO == ITM_BASECOLUMN && rightO == ITM_CONSTANT) {
          parameterizeMe(cwa, bindWA, child(1),
                         (BaseColumn*)leftC, (ConstValue*)rightC);
		  //HQC collect histogram for column
		 ItemExpr * cParameter = child(1);
		 cwa.bindConstant2SQC((BaseColumn*)leftC, (ConstantParameter*)cParameter);
        }
        else if (rightO == ITM_BASECOLUMN && leftO == ITM_CONSTANT) {
          parameterizeMe(cwa, bindWA, child(0),
                         (BaseColumn*)rightC, (ConstValue*)leftC);
             //HQC collect histogram for column
          ItemExpr * cParameter = child(0);
          cwa.bindConstant2SQC((BaseColumn*)rightC, (ConstantParameter*)cParameter);
        }
        else if (leftO == ITM_ITEM_LIST && rightO == ITM_ITEM_LIST) {
          child(0) = ((ItemList*)leftC)->normalizeListForCache
            (cwa, bindWA, (ItemList*)rightC);
        }
      }
      // FIXME: ie, parameterize other binary comparison predicates
      // if we can guarantee the correctness of such parameterizations
    }
  }
  markAsNormalizedForCache();
  return this;
}

// helper to change literals of a cacheable query into input parameters
void BiRelat::parameterizeMe(CacheWA& cwa, BindWA& bindWA, ExprValueId& child,
                             BaseColumn *base, ConstValue *val)
{
  NABoolean parameterizableMVQRplan = cwa.hasRewriteEnabledMV() && 
    CmpCommon::getDefault(MVQR_PARAMETERIZE_EQ_PRED) == DF_ON;
  NABoolean parameterizablePlan = parameterizableMVQRplan || 
    !cwa.hasRewriteEnabledMV();

  if (val->isNull()) {
    // val is null; keep null as is.
    // this is part of a fix to genesis case: 10-010618-3484.
  }
  else if (base->isKeyColumnValue(*val) // keyCol = constant
           && cwa.isParameterizable(base) && parameterizablePlan) {
    cwa.replaceWithNewOrOldConstParam(val, base->getNAColumn()->getType(),
                                      child, bindWA);
    if (parameterizableMVQRplan)
      cwa.setParameterizedPred(TRUE);
  }
  else if (!base->canSafelyCoerce(*val)) {
    // term is not parameterizable, keep it as is.
  }
  else { // nonKeyCol = constant
    if (parameterizablePlan) {
      computeAndAddSelParamIfPossible(cwa, bindWA, child, base, val);
      if (parameterizableMVQRplan)
        cwa.setParameterizedPred(TRUE);
    }
  }
}

// append an ascii-version of ItemExpr into cachewa.qryText_
void Case::generateCacheKey(CacheWA& cwa) const
{
  // print any caseOperand before the case itself. That's out-of-order,
  // but, we must differentiate 2 case exprs that differ only in their
  // caseOperands. Otherwise, we will fail on a test, ie,
  // we'll cache this insert
  //   insert into t values(case 3 when 4 then 0 when 3 then 1 end)
  // and then later incorrectly consider this insert as a hit
  //   insert into t values(case 4 when 4 then 0 when 3 then 1 end)
  // because their cache keys are identical
  if (caseOperand_) {
    cwa += " caseOpd:"; 
    caseOperand_->generateCacheKey(cwa);
    cwa += caseOperandWasNullable_ ? " 1" : " 0";
  }
  cwa += " builtinFunc:"; 
  BuiltinFunction::generateCacheKey(cwa);
}

// does this entire ItemExpr qualify query to be cacheable after this phase?
NABoolean Case::isCacheableExpr(CacheWA& cwa)
{
  if (caseOperand_ && !caseOperand_->isCacheableExpr(cwa)) {
    return FALSE;
  }
  return ItemExpr::isCacheableExpr(cwa);
}

// change literals of a cacheable query into input parameters 
ItemExpr* Case::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (caseOperand_) {
    caseOperand_ = caseOperand_->normalizeForCache(cwa, bindWA);
  }
  return BuiltinFunction::normalizeForCache(cwa, bindWA);
}

// append an ascii-version of Cast into cachewa.qryText_
void Cast::generateCacheKey(CacheWA& cwa) const
{
  BuiltinFunction::generateCacheKey(cwa);
  if (type_) { 
    cwa += " "; 
    cwa += type_->getTypeSQLname().data(); 
  }
}

// is any literal in this expr safely coercible to its target type?
NABoolean Cast::isSafelyCoercible(CacheWA &cwa) const
{
  if (cwa.getPhase() >= CmpMain::BIND) {
    ItemExpr *opd = child(0);
    if (!opd->isSafelyCoercible(cwa)) { 
      return FALSE; 
    }
    if (opd->getOperatorType() == ITM_CONSTANT) {
      return ((ConstValue*)opd)->canBeSafelyCoercedTo(*type_);
    }      
    return TRUE; 
  }
  return FALSE;
}

// append an ascii-version of ConstantParameter into cachewa.qryText_
void ConstantParameter::generateCacheKey(CacheWA& cwa) const
{ 
  cwa += "%"; 
}

// return our size in bytes
Lng32 ConstantParameter::getSize() const
{ 
  return (Lng32)(sizeof(*this) + type_->getSize() + val_->getSize());
}

// can this ConstValue be safely coerced into this target type?
NABoolean ConstValue::canBeSafelyCoercedTo(const NAType& target)
{
  // if errorsCanOccur says it's safe then it is
  if (!type_->errorsCanOccur(target)) {
    if ((target.getTypeQualifier() == NA_CHARACTER_TYPE) &&
        (type_->getTypeQualifier() == NA_CHARACTER_TYPE)) {
      if (((CharType&)target).isCaseinsensitive() !=
          (((CharType*)type_)->isCaseinsensitive()))
        return FALSE;
      else {
        // fix bugzilla 2764 by skipping parameterization of long
        // char literals because backpatching those does not work.
        // not yet.
        return target.getNominalSize() < 
          CmpCommon::getDefaultNumeric(QUERY_CACHE_MAX_CHAR_LEN);
      }
    }
    return TRUE;
  }

  // a NULL can be safely coerced to any nullable type
  if (isNull()) {
    return target.supportsSQLnullLogical();
  }

  // If the ConstValue type and the target type are both numeric
  // and if the ConstValue can fit in the target type,
  // it should be ok.
  if ( target.getTypeQualifier() == NA_NUMERIC_TYPE && 
       type_->getTypeQualifier() == NA_NUMERIC_TYPE ) {

    NumericType *srcType = (NumericType*)type_;
    NumericType *tgtType = (NumericType*)(&target);
    if (tgtType->shouldCheckValueFitInType()) {
      double val = srcType->getNormalizedValue(value_);
      if ((val >= 0) && (val <= (tgtType->getMaxValue())) ) {
        if ( srcType->isExact() && tgtType->isExact() &&
             (srcType->getScale() > tgtType->getScale() ) )
           return FALSE;
        else
           return TRUE;
      }
      if ((val < 0) && (val >= (tgtType->getMinValue())) ) {
        if ( srcType->isExact() && tgtType->isExact() &&
             (srcType->getScale() > tgtType->getScale() ) )
           return FALSE;
        else
           return TRUE;
      }
    }
    // NUMERIC and DECIMAL should be (but are not always) covered in 
    // errorsCanOccur.  
    if (srcType->isExact() && tgtType->isExact()) {
      // Source and target are exact.
      if (srcType->isUnsigned() || tgtType->isSigned()) {
        // If the magnitude and scale of the target are greater
        // than or equal to the source, then no conversion
        // error can occur
        if ((srcType->getMagnitude() <= tgtType->getMagnitude()) &&
            (srcType->getScale() <= tgtType->getScale())
            )
          return TRUE;
      }
      else { // source literal is signed and target is unsigned
        // errors can NOT occur if source literal >= 0 and the
        // magnitude and scale of the target are >= those of source
        if (type_->encode(value_) >= 0.0 &&
            (srcType->getMagnitude() <= tgtType->getMagnitude()) &&
            (srcType->getScale() <= tgtType->getScale()))
          return TRUE;
      }
    }
  }

  // except if target is unsigned exact numeric and
  // our type is signed exact numeric and our value is non-negative
  // in which case we morph our type into signed
  // and then retry the errorsCanOccur test
  if (target.getTypeQualifier() == NA_NUMERIC_TYPE && 
      ((const NumericType&)target).isExact() &&
      ((const NumericType&)target).isUnsigned() && 
      isExactNumeric() && ((NumericType*)type_)->isSigned() &&
      type_->encode(value_) >= 0.0) {
    ((NumericType*)type_)->makeUnsigned();
    NABoolean result = !type_->errorsCanOccur(target);
    ((NumericType*)type_)->makeSigned();
    return result;
  }
  // except if source & target types are INTERVALs and 
  // our constant's SQLnullFlag_ == NOT_NULL_NOT_DROPPABLE
  // and target's SQLnullFlag_ == ALLOWS_NULLS
  if (target.getTypeQualifier() == NA_INTERVAL_TYPE && 
      type_->getTypeQualifier() == NA_INTERVAL_TYPE &&
      target.supportsSQLnullLogical() && !type_->supportsSQLnullPhysical()) {
    // in which case, we morph our constant's SQLnullFlag_ to ALLOWS_NULLS
    // and then retry the errorsCanOccur test
    ((NAType*)type_)->setSQLnullFlag();
    NABoolean result = !type_->errorsCanOccur(target);
    ((NAType*)type_)->resetSQLnullFlag();
    return result;
  }

  return FALSE; // we can't guarantee a safe coercion into target
}

// return our size in bytes
Lng32 ConstValue::getSize() const
{
  return (Lng32)(sizeof(*this) + getStorageSize() + 
    (text_ ? text_->length() : 0) +
    (locale_strval ? locale_strval->length() : 0) +
    (locale_wstrval ? locale_wstrval->length() : 0));
}

// return true iff I am a string literal with unknown character set
NABoolean ConstValue::hasUnknownCharSet()
{
  return type_->getTypeQualifier() == NA_CHARACTER_TYPE &&
    ((CharType*)type_)->getCharSet() == CharInfo::UnknownCharSet;
}

// does this ItemExpr (dis)qualify query to be cacheable after this phase?
NABoolean ConstValue::isCacheableExpr(CacheWA& cwa)
{
  if (cwa.getPhase() <= CmpMain::PARSE && hasUnknownCharSet()) {
    // string literal with unknown character set cannot be considered
    // cacheable by pre-binder stages because the information needed
    // to determine their character set is not known until bind-time.
    // This fixes genesis case 10-041215-6141, soln 10-041215-2826.
    return FALSE;
  }

  // string of length zero causes a problem during backpatching in method 
  // CacheData::backpatchParams.
  // For now, do not cache it.
  if ((NOT isNull()) &&
      (isEmptyString()))
    return FALSE;

  return ItemExpr::isCacheableExpr(cwa);
}

// change literal of a cacheable query into ConstantParameter
ItemExpr* ConstValue::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  // skip system supplied or empty string literals
  if (isSystemSupplied_ || isEmptyString()) { 
    return this; 
  }
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  // for now, do not parameterize NULLs because they can cause false 
  // constraint violations, eg, fullstack/TEST005 
  //   create table t005t2(a int, b int not null, c char(10) not null,
  //                       d int, primary key (c) );
  //   insert into t005t2 values (NULL,7,'?-7-?',NULL);
  // gets "ERROR[8421] NULL cannot be assigned to a NOT NULL column."
  if (isNull()) { 
    return this; 
  }

  NAHeap* heap = cwa.wHeap();
  // quantize length of strings only in after parse case
  ConstantParameter* result = new(heap) ConstantParameter
    (*this, heap, cwa.getPhase() == CmpMain::PARSE);
  if (result) {
    // "after-parser" ConstantParameters will undergo complete binding, so
    // addConstParam does not have to bind "after-parser" ConstantParameters
    cwa.addConstParam(result, bindWA);
    cwa.bindConstant2SQC((BaseColumn*)NULL, result);
    result->markAsNormalizedForCache();
  }
  // do not mark this ConstValue as normalizedForCache because it may
  // be "shared" and may need to be replaced into a ConstantParameter
  // again in another referencing expression context. For example, the
  // binder visits the case operand "3" more than once in
  //   insert into t values(case 3 when 4 then 0 when 3 then 1 end)

  return result;
}

// append an ascii-version of DateFormat into cachewa.qryText_
void DateFormat::generateCacheKey(CacheWA& cwa) const
{
  BuiltinFunction::generateCacheKey(cwa); 
  cwa += " fmt:";
  char dFmt[20]; 
  str_itoa(dateFormat_, dFmt); 
  cwa += dFmt;
  str_itoa(frmt_, dFmt);
  cwa += dFmt;
}

// does this entire ItemExpr qualify query to be cacheable after this phase?
NABoolean DynamicParam::isCacheableExpr(CacheWA& cwa)
{
  // a dynamic parameter is not cacheable after parse
  return (cwa.getPhase() <= CmpMain::PARSE) ? FALSE : TRUE;
}

// append an ascii-version of Cast into cachewa.qryText_
void Extract::generateCacheKey(CacheWA& cwa) const
{
  BuiltinFunction::generateCacheKey(cwa); 
  cwa += " xFld:"; 
  char xFld[20]; str_itoa(extractField_, xFld); 
  cwa += xFld;
  cwa += " fldFun:"; 
  cwa += fieldFunction_ ? "1" : "0";
}

// append an ascii-version of HbaseTimestamp into cachewa.qryText_
void HbaseTimestamp::generateCacheKey(CacheWA& cwa) const
{
  BuiltinFunction::generateCacheKey(cwa); 

  col()->generateCacheKey(cwa);
}

// append an ascii-version of HbaseVersion into cachewa.qryText_
void HbaseVersion::generateCacheKey(CacheWA& cwa) const
{
  BuiltinFunction::generateCacheKey(cwa); 

  col()->generateCacheKey(cwa);
}

// does this entire ItemExpr qualify query to be cacheable after this phase?
NABoolean HostVar::isCacheableExpr(CacheWA& cwa)
{
  // a HostVar is always cacheable
  return TRUE;
}

// append an ascii-version of PivotGroup into cachewa.qryText_
void PivotGroup::generateCacheKey(CacheWA& cwa) const
{
  Aggregate::generateCacheKey(cwa); 
  cwa += delim_;

  char xFld[20]; str_itoa(maxLen_, xFld); 
  cwa += xFld;

  cwa += orderBy_ ? "1" : "0";
}

// append an ascii-version of ItemExpr into cachewa.qryText_
void ItemExpr::generateCacheKey(CacheWA& cwa) const
{
  Int32 arity = getArity();
  switch (arity) {
  case 0:
    // simply print the text out for a leaf operator
    cwa += getText4CacheKey();
    break;
  case 2:
    if (getOperatorType() == ITM_ITEM_LIST || getOperatorType() == ITM_AND) {
      if (child(0)) {
        child(0)->generateCacheKey(cwa);
      }
      else {
        cwa += "NULL";
      }
      if (getOperatorType() != ITM_ITEM_LIST) {
        cwa += " ";
      }
      cwa += getText4CacheKey();
      cwa += " ";

      if (child(1)) {
        child(1)->generateCacheKey(cwa);
      }
      else {
        cwa += "NULL";
      }
    }
    else {
      // assume this is an infix operator (<child0>) op (<child1>)
      cwa += "(";

      NABoolean list0 = FALSE, list1 = FALSE;
      if (getOperatorType() != ITM_ITEM_LIST) {
        list0 = child(0)->getOperatorType() == ITM_ITEM_LIST;
        list1 = child(1)->getOperatorType() == ITM_ITEM_LIST;
      }
      if (list0) {
        cwa += "(";
      }
      child(0)->generateCacheKey(cwa);
      if (list0) {
        cwa += ")";
      }
      cwa += " ";
      cwa += getText4CacheKey();
      cwa += " ";
      if (list1) {
        cwa += "(";
      }
      child(1)->generateCacheKey(cwa);
      if (list1) {
        cwa += ")";
      }
      cwa += ")";
    }
    break;

  default:
    if (arity == 1 && isAPredicate() && getOperatorType() != ITM_NOT) {
      child(0)->generateCacheKey(cwa);
      cwa += " ";
      cwa += getText4CacheKey();
    }
    else {
      // any other arity: assume a function op(<child>,<child>,...)
      cwa += getText4CacheKey();
      cwa += "(";
      for (Lng32 i = 0; i < (Lng32)arity; i++) {
        if (i > 0) {
          cwa += ", ";
        }
        child(i)->generateCacheKey(cwa);
      }
      cwa += ")";
      break;
    }
  }
  addSelectivityFactor( cwa ) ;
}

//
// addSelectivityFactor() - a helper routine for ItemExpr::generateCacheKey()
//
// NOTE: The code in this routine came from the previous version of
//       ItemExpr::generateCacheKey().   It has been pulled out
//       into a separate routine so that the C++ compiler will produce
//       code that needs signficantly less stack space for the
//       recursive ItemExpr::generateCacheKey() routine.
//
void ItemExpr::addSelectivityFactor( CacheWA& cwa ) const
{
  if(((ItemExpr *)this)->isSelectivitySetUsingHint()) 
  {
    char str[100];
    sprintf(str, " sel:%g", getSelectivityFactor());
    cwa += str;
  }
}

// return true if ItemExpr & its descendants have no constants & noncacheables
NABoolean ItemExpr::hasNoLiterals(CacheWA& cwa)
{
  if (isNonCacheable()) {
    return FALSE;
  }
  OperatorTypeEnum opTyp = getOperatorType();
  switch (opTyp) {
  case ITM_CONSTANT:
  case ITM_HOSTVAR:
  case ITM_DYN_PARAM:
  case ITM_CACHE_PARAM: 
    return FALSE;
  default: // fall thru
    break;
  }
  NABoolean result = TRUE;
  Int32 arity = getArity();
  for (Int32 x = 0; x < arity && result; x++) {
    if (child(x)) { 
      result = child(x)->hasNoLiterals(cwa); 
    }
  }
  return result;
}

// does this entire ItemExpr qualify query to be cacheable after this phase?
NABoolean ItemExpr::isCacheableExpr(CacheWA& cwa)
{
  switch (cwa.getPhase()) {
  default: { const NABoolean notYetImplemented = FALSE; 
    CMPASSERT(notYetImplemented);
    break;
  }
  case CmpMain::PARSE:
    // ItemExpr::isCacheableExpr is used by Tuple::isCacheable to
    // determine whether a tuple-insert or a tuplelist-insert is
    // cacheable after parse. It traverses a tuple's values list
    // looking for non-cacheable ItemExprs such as HostVar, 
    // DynamicParam, Subquery, etc.  
    // determine cacheability of tuple INSERT's itemexprs here
    if (isNonCacheable() || !cwa.isConditionallyCacheable()) {
      return FALSE;
    }
    else { // we're either cacheable or maybecacheable
      Int32 arity = getArity();
      if (arity <= 0) { // we have no kids & we're conditionally cacheable
        return TRUE; // we're cacheable
      }
      // cacheability of child(ren) determine our cacheability
      for (Int32 x = 0; x < arity; x++) {
        if (!child(x) || child(x)->isNonCacheable()) {
          // the 1st noncacheable child makes us noncacheable
          setNonCacheable();
          return FALSE;
        }
        else if (!child(x)->isCacheableExpr(cwa)) {
          // noncacheable child
          return FALSE;
        }
        else { // cacheable child
          continue; // look at next child
        }
      }
      // all children are cacheable, so we're cacheable too
      setCacheableNode(cwa.getPhase());
      return TRUE;
    }
    break;
  case CmpMain::BIND:
    // does query have too many ExprNodes?
    if (cwa.inc_N_check_still_cacheable() == FALSE) {
      // yes. query with too many ExprNodes is not cacheable.
      return FALSE;
    }
    // ItemExpr::isCacheableExpr is used in "after BIND" cases to
    // visit all operands of an ItemExpr looking for noncacheable
    // expressions. For example, it discovers that
    //   SELECT (SELECT a FROM t2 WHERE MAX(o.c)>1) FROM t1 o;
    // is noncacheable only after it visits the outer query's select list.
    if (isNonCacheable()) {
      return FALSE;
    }
    else if (isCacheableNode(cwa.getPhase())) {
      return TRUE;
    }
    else { // we're either cacheable or maybecacheable
      // Assume this ItemExpr is an operand of a function or a list. In
      // this case, a single noncacheable child renders us noncacheable.
      Int32 arity = getArity();
      for (Int32 x = 0; x < arity; x++) {
        if (!child(x) || !child(x)->isCacheableExpr(cwa)) {
          // noncacheable child
          return FALSE;
        }
        else { // cacheable child
          continue; // look at next child
        }
      }
      // all children are cacheable, so we're cacheable too
      setCacheableNode(cwa.getPhase());
      return TRUE;
    }
    break;
  }
  return FALSE;
}

// is this ExprNode cacheable after this phase?
NABoolean ItemExpr::isCacheableNode(CmpPhase phase) const
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

// is any literal in this expr safely coercible to its target type?
NABoolean ItemExpr::isSafelyCoercible(CacheWA &cwa) const
{
  if (cwa.getPhase() >= CmpMain::BIND) {
    Int32 arity = getArity();
    for (Int32 x = 0; x < arity; x++) {
      if (!child(x)->isSafelyCoercible(cwa)) { 
        return FALSE; 
      }
    }
    if (arity == 2) {
      // we have to disallow caching of the following types of exprs:
      //   expr + 123456789012345678901234567890
      //   expr || 'overlylongstringthatwouldoverflow'
      ItemExpr *left = child(0), *right = child(1);
      if (left->getOperatorType() == ITM_CONSTANT) {
        if (right->getOperatorType() == ITM_CONSTANT) {
          // "10 + 1" should be safely coercible
          return TRUE;
        } else {
          return ((ConstValue*)left)->canBeSafelyCoercedTo
            (right->getValueId().getType());
        }
      }      
      else if (right->getOperatorType() == ITM_CONSTANT) {
        return ((ConstValue*)right)->canBeSafelyCoercedTo
          (left->getValueId().getType());
      }      
      // else both are nonliterals; fall thru
    }
    // else nondyadic expr; fall thru
    return TRUE; 
  }
  return FALSE;
}

// change literals of a cacheable query into ConstantParameters 
ItemExpr* ItemExpr::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  Int32 arity = getArity();
  for (Int32 x = 0; x < arity; x++) {
    if (cwa.getPhase() == CmpMain::PARSE) {
      // we want to parameterize tuple inserts
      child(x) = child(x)->normalizeForCache(cwa, bindWA);
    }
    else if (cwa.getPhase() == CmpMain::BIND) {
      if (child(x)->isSafelyCoercible(cwa)) {
        // fix CR 10-010726-4109: make sure queries with constants that 
        // cannot be safely backpatched such as 
        //   select case smallintcol when 4294967393 then 'max' end from t
        // are not parameterized
        child(x) = child(x)->normalizeForCache(cwa, bindWA);
      }
    }
  }
  markAsNormalizedForCache();
  return this;
}

// mark this ExprNode as cacheable after this phase
void ItemExpr::setCacheableNode(CmpPhase phase)
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

// does this query's selection predicate list qualify query 
// to be cacheable after this phase?
NABoolean ItemList::isListOfCacheableSelPred
(CacheWA& cwa, ItemList *other) const
{
  Int32 arity = getArity();
  NABoolean result = FALSE;
  if (cwa.getPhase() >= CmpMain::BIND && 
      other && arity == other->getArity()) {
    // assume this is an AND list, so, we need only one
    // cacheable conjunct to consider the list cacheable.
    for (Int32 x = 0; x < arity; x++) {
      ItemExpr *leftC = child(x), *rightC = other->child(x);
      OperatorTypeEnum leftO = leftC->getOperatorType();
      OperatorTypeEnum rightO = rightC->getOperatorType();
      BaseColumn *base;
      if (leftO == ITM_BASECOLUMN) {
        base = (BaseColumn*)leftC;
        if (base->isKeyColumnValue(*rightC)) {
          cwa.addToUsedKeys(base);
        }
        result = TRUE;
        continue;
      }
      else if (rightO == ITM_BASECOLUMN) {
        base = (BaseColumn*)rightC;
        if (base->isKeyColumnValue(*leftC)) {
          cwa.addToUsedKeys(base);
        }
        result = TRUE;
        continue;
      }
      else if (leftO == ITM_ITEM_LIST && rightO == ITM_ITEM_LIST &&
               ((ItemList*)leftC)->isListOfCacheableSelPred
               (cwa, (ItemList*)rightC)) {
        result = TRUE;
        continue;
      }
    }
  }
  return result;
}

// is any literal in this expr safely coercible to its target type?
NABoolean ItemList::isSafelyCoercible(CacheWA& cwa) const
{
  if (cwa.getPhase() >= CmpMain::BIND) {
    Int32 arity = getArity();
    for (Int32 x = 0; x < arity; x++) {
      if (!child(x)->isSafelyCoercible(cwa)) { 
        return FALSE; 
      }
    }
    return TRUE;
  }
  return FALSE;
}

// change literals of a cacheable query into input parameters 
ItemExpr* ItemList::normalizeListForCache
(CacheWA& cwa, BindWA& bindWA, ItemList *other)
{
  Int32 arity = getArity();
  if (cwa.getPhase() >= CmpMain::BIND && 
      other && arity == other->getArity()) {
    for (Int32 x = 0; x < arity; x++) {
      ItemExpr *leftC = child(x), *rightC = other->child(x);
      OperatorTypeEnum leftO = leftC->getOperatorType();
      OperatorTypeEnum rightO = rightC->getOperatorType();
      if (leftO == ITM_BASECOLUMN && rightO == ITM_CONSTANT) {
        parameterizeMe(cwa, bindWA, other->child(x), (BaseColumn*)leftC,
                       (ConstValue*)rightC);
		 //HQC collect histogram for column
		 ItemExpr * cParameter = other->child(x);
        cwa.bindConstant2SQC((BaseColumn*)leftC, (ConstantParameter*)cParameter);
      }
      else if (rightO == ITM_BASECOLUMN && leftO == ITM_CONSTANT) {
        parameterizeMe(cwa, bindWA, child(x), (BaseColumn*)rightC,
                       (ConstValue*)leftC);
		 //HQC collect histogram for column
		 ItemExpr * cParameter = child(x);
        cwa.bindConstant2SQC((BaseColumn*)rightC, (ConstantParameter*)cParameter);
      }
      else if (leftO == ITM_ITEM_LIST && rightO == ITM_ITEM_LIST) {
        child(x) = ((ItemList*)leftC)->normalizeListForCache
          (cwa, bindWA, (ItemList*)rightC);
      }
    }
  }
  return this;
}

// helper to change literals of a cacheable query into input parameters
void ItemList::parameterizeMe(CacheWA& cwa, BindWA& bindWA, ExprValueId& child,
                              BaseColumn *base, ConstValue *val)
{
  NABoolean parameterizableMVQRplan = cwa.hasRewriteEnabledMV() && 
    CmpCommon::getDefault(MVQR_PARAMETERIZE_EQ_PRED) == DF_ON;
  NABoolean parameterizablePlan = parameterizableMVQRplan || 
    !cwa.hasRewriteEnabledMV();

  if (val->isNull()) {
    // val is null; keep null as is.
    // this is part of a fix to genesis case: 10-010618-3484.
  }
  else if (base->isKeyColumnValue(*val) // keyCol = constant
           && cwa.isParameterizable(base) && parameterizablePlan) {
    cwa.replaceWithNewOrOldConstParam(val, base->getNAColumn()->getType(),
                                      child, bindWA);
    if (parameterizableMVQRplan)
      cwa.setParameterizedPred(TRUE);
  }
  else if (!base->canSafelyCoerce(*val)) {
    // term is not parameterizable, keep it as is.
  }
  else { // nonKeyCol = constant
    if (parameterizablePlan) {
      computeAndAddSelParamIfPossible(cwa, bindWA, child, base, val);
      if (parameterizableMVQRplan)
        cwa.setParameterizedPred(TRUE);
    }
  }
}

// append an ascii-version of LIKE into cachewa.qryText_
void PatternMatchingFunction::generateCacheKey(CacheWA& cwa) const
{
  // we are deliberately excluding from cachekey the data members: 
  // numberOfNonWildcardChars_, patternAStringLiteral_, 
  // oldDefaultSelForLikeWildCardUsed_
  // because we believe they are "internal" variables, ie, they are
  // not essential to distinguishing two different LIKEs.
  BuiltinFunction::generateCacheKey(cwa);
}

// change literals of a cacheable query into ConstantParameters 
ItemExpr* PatternMatchingFunction::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  // replace only the matchValue
  child(0) = child(0)->normalizeForCache(cwa, bindWA);
  // but, don't touch the pattern or the escapeChar.

  markAsNormalizedForCache();
  return this;
}

// Note: When this method is used during query cache lookup (actually the 
// verification step), this belongs to the item in the bucket that matches
// the key, and other is the selectivity computed for an equal-predicate 
// constant in the query to be looked up. 
//
// This method is called to verify that this and other are not different
// very much.
//
// We return TRUE iff 
//  this.val_ / (1+delta)  <= other.val_ <= this.val_ * (1 + delta)
//
// where delta is 
//    the value stored in QUERY_CACHE_SELECTIVITY_TOLERANCE
//
NABoolean Selectivity::operator==(const Selectivity& other) const
{
  CostScalar delta = 
    CostScalar((ActiveSchemaDB()->getDefaults()).getAsDouble(QUERY_CACHE_SELECTIVITY_TOLERANCE)) + 1;

  return (val_ / delta <= other.val_) && (other.val_ <= val_ * delta);
}

// append an ascii-version of SelParameter into cachewa.qryText_
void SelParameter::generateCacheKey(CacheWA& cwa) const
{ 
  cwa += "#"; 
}

// append an ascii-version of SequenceValue into cachewa.qryText_
void SequenceValue::generateCacheKey(CacheWA& cwa) const
{
  BuiltinFunction::generateCacheKey(cwa); 
  cwa += seqCorrName_.getQualifiedNameAsString();

  if (currVal_)
    cwa += "currVal";
  else if (nextVal_)
    cwa += "nextVal";
}

// append an ascii-version of Subquery into cachewa.qryText_
void Subquery::generateCacheKey(CacheWA& cwa) const
{
  ItemExpr::generateCacheKey(cwa); 
  cwa += " tExp:";
  getSubquery()->generateCacheKey(cwa);
}

// return true if ItemExpr & its descendants have no constants 
// and no noncacheable nodes
NABoolean Subquery::hasNoLiterals(CacheWA& cwa)
{
  return ItemExpr::hasNoLiterals(cwa) && getSubquery()->isCacheableExpr(cwa);
}

// we want Subquery to be cacheable
NABoolean Subquery::isCacheableExpr(CacheWA& cwa)
{
  if (!getSubquery()->isCacheableExpr(cwa)) {
    return FALSE;
  }
  return ItemExpr::isCacheableExpr(cwa);
}

// change literals of a cacheable query into ConstantParameters 
ItemExpr* Substring::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  // parameterize S in "substring(S from B for L)" but skip B & L
  child(0) = child(0)->normalizeForCache(cwa, bindWA);

  markAsNormalizedForCache();
  return this;
}

// append an ascii-version of Translate into cachewa.qryText_
void Translate::generateCacheKey(CacheWA& cwa) const
{
  BuiltinFunction::generateCacheKey(cwa); 
  cwa += " map:";
  char map[20]; str_itoa(map_table_id_, map); 
  cwa += map;
}

// append an ascii-version of Trim into cachewa.qryText_
void Trim::generateCacheKey(CacheWA& cwa) const
{
  BuiltinFunction::generateCacheKey(cwa); 
  cwa += " tlb:";
  char mode[20]; str_itoa(mode_, mode); 
  cwa += mode;
}

// Only alow UDFunctions to be cached after BIND time.
NABoolean UDFunction::isCacheableExpr(CacheWA& cwa)
{ 
  if (cwa.getPhase() >= CmpMain::BIND) 
    return TRUE;
  else 
    return FALSE; 
}

// change literals of a cacheable query into ConstantParameters 
ItemExpr* UDFunction::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  // Since the UDFunction::transformNode refers to both the 
  // inputVars_ ValueIdSet and the children array we need to make sure
  // they are consistent.
  // The children array may contain cast() of the expressions in inputVars_


  // If we have a UUDF function the inputs that were given 
  // at parse time, really do not reflect reality anymore.
  // So here we will simply reinitialize them with what we find in the
  // inputVars_.

  CMPASSERT(udfDesc_); // we better have one after binding.
  NABoolean isUUDF(udfDesc_->isUUDFRoutine());

  // Save off a copy of the original inputVars_ set.
  ValueIdSet origInputVars(inputVars_);


  // Loop through the given inputs
  for (Int32 x = 0; x < getArity(); x++)
  {
    NABoolean origInputIsChildOfCast(FALSE);
    ItemExpr * origIe = child(x);
    ItemExpr * newIe = NULL;
    ValueId vid = origIe->getValueId();
    if (cwa.getPhase() == CmpMain::BIND) 
    {
      if (origIe->isSafelyCoercible(cwa)) 
      {
   
        // fix CR 10-010726-4109: make sure queries with constants that 
        // cannot be safely backpatched such as 
        //   select case smallintcol when 4294967393 then 'max' end from t
        // are not parameterized
   
   
        newIe = origIe->normalizeForCache(cwa, bindWA);
   
        if (newIe != origIe )
        {
          // normalizeForCache returned a new ItemExpr. We have to update
          // the UDFunction inputVars_ set, as this is used to determine
          // characteristic inputs for IsolatedScalarUDF at transform time.

          child(x) = newIe;

          // Is it a input that UDFunction::bindNode put a cast around?
          if ((origIe->getOperatorType() == ITM_CAST) &&
              (origInputVars.contains(origIe->child(0)->getValueId())))
          {

            // Since the original child was a CAST that UDFunction::bindNode
            // created, check to see if that CAST's child is part of the new
            // expression, if it is, we don't have to do anything, since the
            // CASTED value is part of the inputVars set, otherwise, we have
            // to update the inputVars set to keep the characteristic inputs
            // consistent.
            if (! newIe->referencesTheGivenValue(
                    origIe->child(0)->getValueId()), TRUE)
            {
              // remove the original input from inputVars. It is the child
              // of origIe because origIe is a cast that UDFunction::bindNode
              // introduced.
              inputVars_ -= origIe->child(0)->getValueId();

              if (newIe->getOperatorType() == ITM_CAST)
              {
                // If the new expression is a CAST, we assume the child is the
                // real input. We don't expect CAST(CAST(CAST(expr))) type
                // expressions only simple CAST(expr) ones.
                inputVars_ += newIe->child(0)->getValueId();
              }
              else
              {
                // add the newIe itself if it was not a CAST.
                inputVars_ += newIe->getValueId();
              }
            }
          }
          else
          {
            // If the newIe doesn't contain the original one, we need to update
            // the inputVars set.
            if (! newIe->referencesTheGivenValue(
                    origIe->getValueId()), TRUE)
            {
              // the origIe was not a CAST introduced by UDFunction::bindNode()
              if (newIe->getOperatorType() == ITM_CAST) 
              {
                if (!origInputVars.contains(newIe->child(0)->getValueId()))
                {
                  inputVars_ -= origIe->getValueId();
                  // If the new expression is a CAST, we assume the child is the
                  // real input. We don't expect CAST(CAST(CAST(expr))) type
                  // expressions only simple CAST(expr) ones.
                  inputVars_ += newIe->child(0)->getValueId();
                }
                // we don't need to update inputVars_ if the origInputVars 
                // contains the valueId of the newIe child already.
              } 
              else
              {
                // This is an entirely new input. Remove the old one, and 
                // add in the new.
                inputVars_ -= origIe->getValueId();
                inputVars_ += newIe->getValueId();
              }
            }
          }
        }
      }
    }
  }

  markAsNormalizedForCache();
  return this;
}


// append an ascii-version of UDFunction into cachewa.qryText_
void UDFunction::generateCacheKey(CacheWA& cwa) const
{
  NARoutine *routine = NULL;
  NARoutine *action = NULL;

  cwa += " nam:"; 
  cwa += functionName_.getExternalName().data();
  if (cwa.getPhase() >= CmpMain::BIND && 
      getRoutineDesc() && 
      (routine=getRoutineDesc()->getNARoutine()) != NULL) 
  {
    char redefTime[40];
    convertInt64ToAscii(routine->getRedefTime(), redefTime);
    cwa += " redef:";
    cwa += redefTime;
  }

  if (getRoutineDesc() && 
      getRoutineDesc()->getActionNameAsGiven().length() != 0)
  {
    cwa += " actnam:"; 
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

  cwa += "(";
  Lng32 arity =  (Lng32) getArity();
  for (Lng32 i = 0; i < arity; i++) {
    if (i > 0) {
      cwa += ", ";
    }
    child(i)->generateCacheKey(cwa);
  }
 
  if (getRoutineDesc() && getRoutineDesc()->getLocale() != 0 )
  {
    cwa += ", LOCALE: ";
    char dFmt[20]; 
    str_itoa(getRoutineDesc()->getLocale(), dFmt); 
    cwa += dFmt;
  }
  cwa += ")";
}

// is any literal in this expr safely coercible to its target type?
NABoolean UDFunction::isSafelyCoercible(CacheWA& cwa) const
{
  if (cwa.getPhase() >= CmpMain::BIND) {
    Int32 arity = getArity();
    for (Int32 x = 0; x < arity; x++) {
      if (!child(x)->isSafelyCoercible(cwa)) { 
        return FALSE; 
      }
    }
    return TRUE;
  }
  return FALSE;
}

// append an ascii-version of Subquery into cachewa.qryText_
void ValueIdProxy::generateCacheKey(CacheWA& cwa) const
{
  ItemExpr::generateCacheKey(cwa); 
  cwa += " tExp:";
  derivedFrom_.getItemExpr()->generateCacheKey(cwa);
}

// return true if ItemExpr & its descendants have no constants 
// and no noncacheable nodes
NABoolean ValueIdProxy::hasNoLiterals(CacheWA& cwa)
{
  return derivedFrom_.getItemExpr()->hasNoLiterals(cwa) && 
         derivedFrom_.getItemExpr()->isCacheableExpr(cwa);
}

// is any literal in this expr safely coercible to its target type?
NABoolean ValueIdProxy::isSafelyCoercible(CacheWA& cwa) const
{
  if (cwa.getPhase() >= CmpMain::BIND) {
      if (!derivedFrom_.getItemExpr()->isSafelyCoercible(cwa))
        return FALSE; 
      else
        return TRUE;
  }
  return FALSE;
}

// change literals of a cacheable query into ConstantParameters 
ItemExpr* ValueIdProxy::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  if (nodeIsNormalizedForCache()) { 
    return this; 
  }
  if (cwa.getPhase() == CmpMain::PARSE) {
    // we want to parameterize tuple inserts
    derivedFrom_ = derivedFrom_.getItemExpr()->normalizeForCache(cwa, bindWA)->getValueId();
  }
  else if (cwa.getPhase() == CmpMain::BIND) {
      if (derivedFrom_.getItemExpr()->isSafelyCoercible(cwa)) {
        // fix CR 10-010726-4109: make sure queries with constants that 
        // cannot be safely backpatched such as 
        //   select case smallintcol when 4294967393 then 'max' end from t
        // are not parameterized
        derivedFrom_ = derivedFrom_.getItemExpr()->normalizeForCache(cwa, bindWA)->getValueId();
      }
  }
  markAsNormalizedForCache();
  return this;
}





// --------------------------------------------------------------
// member functions for LOBoper operator
// --------------------------------------------------------------
NABoolean LOBoper::isCacheableExpr(CacheWA& cwa)
{
  if (NOT ItemExpr::isCacheableExpr(cwa))
    return FALSE;

  return TRUE;
}

void LOBoper::generateCacheKey(CacheWA& cwa) const
{
  ItemExpr::generateCacheKey(cwa);

  char oper[20];
  cwa += " operType: ";
  cwa += str_itoa((int)getOperatorType(), oper);
  cwa += " ";
}


void LOBinsert::generateCacheKey(CacheWA & cwa) const
{
  LOBoper::generateCacheKey(cwa);
   char oper[40];
  cwa += " fromObj: ";
  cwa += str_itoa((int)getObj(), oper);
  cwa += " isAppend: ";
  cwa += append_? "1":"0";
  cwa += " ";
  
}

void LOBconvert::generateCacheKey(CacheWA & cwa) const
{
  LOBoper::generateCacheKey(cwa);
   char oper[40];
  cwa += " fromObj: ";
  cwa += str_itoa((int)getObj(), oper);
  cwa += " tgtSize: ";
  cwa += str_itoa((int)getTgtSize(), oper);
  cwa += " ";
  
}


void LOBupdate::generateCacheKey(CacheWA & cwa) const
{
  LOBoper::generateCacheKey(cwa);
   char oper[40];
  cwa += " fromObj: ";
  cwa += str_itoa((int)getObj(), oper);
  cwa += " isAppend: ";
  cwa += append_? "1":"0"; 
  cwa += " ";
  
}

