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

#include <limits>
#include <float.h>
#include "nawstring.h"
#include "QRDescGenerator.h"
#include "NumericType.h"
#include "DatetimeType.h"
#include "QRLogger.h"
#include "OptRange.h"
#include "ItemLog.h"
#include "ComCextdecs.h"

double getDoubleValue(ConstValue* val, logLevel level);

/**
 * Returns the Int64 value corresponding to the type of the ConstValue val.
 * val can be of any numeric, datetime, or interval type. The returned value
 * is used in the representation of a range of values implied by the predicates
 * of a query for an exact numeric, datetime, or interval type.
 *
 * @param val ConstValue that wraps the value to be represented as an Int64.
 * @param rangeColType The type of the column or expr the range is for.
 * @param [out] truncated TRUE returned if the value returned was the result of
 *                        truncating the input value. This can happen for floating
 *                        point input, or exact numeric input that has greater
 *                        scale than rangeColType.
 * @param [out] valWasNegative TRUE returned if the input value was negative.
 *                             Adjustment of truncated values is only done for
 *                             positive values (because the truncation of a negative
 *                             value adjusts it correctly). The caller can't just
 *                             look at the returned value, because if it is 0, it
 *                             may have been truncated from a small negative (-1 < n < 0)
 *                             or a small positive (0 < n < 1) value.
 * @param level Logging level to use in event of failure.
 * @return The rangespec internal representation of the input constant value.
 */
static Int64 getInt64Value(ConstValue* val, const NAType* rangeColType,
                           NABoolean& truncated, NABoolean& valWasNegative,
                           logLevel level);

OptRangeSpec::OptRangeSpec(QRDescGenerator* descGenerator, CollHeap* heap, logLevel ll)
  : RangeSpec(heap, ll),
    descGenerator_(descGenerator),
    rangeExpr_(NULL),
    vid_(NULL_VALUE_ID),
    isIntersection_(FALSE)
{
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    descGenerator, QRLogicException,
                    "OptRangeSpec constructed for null QRDescGenerator");

  if (descGenerator->isDumpMvMode())
    setDumpMvMode();
}

ValueId OptRangeSpec::getBaseCol(const ValueIdSet& vegMembers)
{
  for (ValueId id=vegMembers.init(); vegMembers.next(id); vegMembers.advance(id))
    {
      if (id.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
        return id;
    }
  
  return NULL_VALUE_ID;
}

ValueId OptRangeSpec::getBaseCol(const ValueId vegRefVid)
{
  ItemExpr* itemExpr = vegRefVid.getItemExpr(); 
  OperatorTypeEnum opType = itemExpr->getOperatorType();

  // See if the vid is for a basecol instead of a vegref.
  if (opType == ITM_BASECOLUMN)
    return vegRefVid;

  // Get the value id of the first member that is a base column.
  assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                     opType == ITM_VEG_REFERENCE, QRDescriptorException,
                     "OptRangeSpec::getBaseCol() expected value id of a "
                     "vegref, not of op type -- %d", opType);

  ValueId baseColVid = getBaseCol(static_cast<VEGReference*>(itemExpr)
                                                ->getVEG()->getAllValues());
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    baseColVid != NULL_VALUE_ID, QRDescriptorException,
                    "Vegref contains no base columns");
  return baseColVid;
}

OptRangeSpec* OptRangeSpec::createRangeSpec(QRDescGenerator* descGenerator,
                                            ItemExpr* predExpr,
                                            CollHeap* heap,
                                            NABoolean createForNormalizer)
{
  QRTRACER("createRangeSpec");
  OptRangeSpec* range = NULL;

  if (predExpr->getOperatorType() == ITM_RANGE_SPEC_FUNC)
    {
      assertLogAndThrow(CAT_SQL_COMP_RANGE, LL_ERROR,
                        !createForNormalizer, QRDescriptorException,
                        "RangeSpecRef should not be present if creating for Normalizer");
      RangeSpecRef* rangeIE = static_cast<RangeSpecRef*>(predExpr);
      range = new(heap) OptRangeSpec(*rangeIE->getRangeObject(), heap);

      // RangeSpecRefs are produced by the Normalizer. The rangespec they contain
      // may have a vegref vid as the rangecolvalueid instead of using the first
      // basecol vid as we do when a range is created in mvqr. Also, the vid may
      // be that of a joinpred, but will still be stored as the rangecolvalueid.
      // Below, we sort out these issues for the new rangespec we have created
      // using the copy ctor on the one in the RangeSpecRef.
      ValueId rcvid = range->getRangeColValueId();
      if (rcvid != NULL_VALUE_ID)
        {
          ItemExpr* ie = rcvid.getItemExpr();
          if (ie->getOperatorType() == ITM_VEG_REFERENCE)
            {
              if (descGenerator->isJoinPredId(rcvid))
                {
                  range->setRangeColValueId(NULL_VALUE_ID);
                  range->setRangeJoinPredId(rcvid);
                }
              else
                {
                  rcvid = range->getBaseCol(((VEGReference*)ie)->getVEG()->getAllValues());
                  if (rcvid != NULL_VALUE_ID)
                    range->setRangeColValueId(rcvid);
                }
            }
        }
    }
  else
    {
      if (createForNormalizer)
        {
          range = new (heap) OptNormRangeSpec(descGenerator, heap);
          (static_cast<OptNormRangeSpec*>(range))
                    ->setOriginalItemExpr(predExpr);
        }
      else
        range = new (heap) OptRangeSpec(descGenerator, heap);

      if (!range->buildRange(predExpr))
        {
          delete range;
          return NULL;
        }
    }

  range->setID(predExpr->getValueId());
  range->log();
  return range;
}

// Protected copy ctor, used by clone().
OptRangeSpec::OptRangeSpec(const OptRangeSpec& other, CollHeap* heap)
  : RangeSpec(other, heap),
    descGenerator_(other.descGenerator_),
    rangeExpr_(NULL),
    vid_(other.vid_),
    isIntersection_(other.isIntersection_)
{
  // At this point the inherited heap ptr mvqrHeap_ has been initialized
  // by the superclass ctor.
  if (other.rangeExpr_)
    rangeExpr_ = other.rangeExpr_->copyTree(mvqrHeap_);
}

QRRangePredPtr OptRangeSpec::createRangeElem()
{
  QRTRACER("createRangeElem");
  QRRangePredPtr rangePredElem = 
          new (mvqrHeap_) QRRangePred(ADD_MEMCHECK_ARGS(mvqrHeap_));
  rangePredElem->setRangeItem(genRangeItem());

  NABoolean rangeIsOnCol = (rangeJoinPredId_ != NULL_VALUE_ID ||
                            rangeColValueId_ != NULL_VALUE_ID);

  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    getID()>0, QRDescriptorException,
                    "No id for range element in OptRangeSpec::createRangeElem().");

  // The id of this rangespec is the value id of the original predicate on the
  // corresponding range column/expr. If other preds on the range col/expr were
  // found and had to be intersected, we need to use the value id of the itemexpr
  // that is the result of the intersection (so the right predicat will be used
  // for the rewrite).
  if (isIntersection_)
    rangePredElem->setID(getRangeItemExpr()->getValueId());
  else
    rangePredElem->setID(getID());

  const NAType* typePtr = getType();
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    typePtr, QRDescriptorException,
                    "Call to getType() returned NULL in OptRangeSpec::createRangeElem().");
  const NAType& type = *typePtr;
  rangePredElem->setSqlType(type.getTypeSQLname());
  QROpInequalityPtr ineqOp;
  QROpEQPtr eqOp = NULL;
  QROpBTPtr betweenOp;
  CollIndex numSubranges = subranges_.entries();
  for (CollIndex i=0; i<numSubranges; i++)
    {
      SubrangeBase& subrange = *subranges_[i];
      if (subrange.startIsMin() || 
          (i==0 && rangeIsOnCol && subrange.isMinForType(type)))
        {
          assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                             i==0, QRDescriptorException,
                             "Subrange other than 1st is unbounded on low side, "
                             "subrange index %d", i);
          if (subrange.endIsMax() ||
              (i==numSubranges-1 && rangeIsOnCol && subrange.isMaxForType(type)))
            {
              // Range spans all values of the type. If NULL is included as
              // well, return NULL to indicate no range restriction. If NULL is
              // not included in the range, an empty <RangePred> is used to
              // indicate IS NOT NULL.
              assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                                numSubranges==1, QRDescriptorException,
                                "Range of all values must have a single Subrange.");
              if (nullIncluded_)
                {
                  QRLogger::log(CAT_SQL_COMP_RANGE, LL_INFO,
                    "Range predicate ignored because it spans entire range + NULL.");
                  deletePtr(rangePredElem);
                  return NULL;
                }
              else
                {
                  // An IS NOT NULL predicate on a NOT NULL column is removed in
                  // the early compilation stages. If we generate the usual empty
                  // range element to represent a predicate that spans all values,
                  // it will not match this missing predicate. So we detect and
                  // remove it.
                  QRElementPtr rangeItemElem = rangePredElem->getRangeItem()
                                                            ->getReferencedElement();
                  if (rangeItemElem->getIDFirstChar() == 'C')
                    {
                      ValueId vid = rangeItemElem->getIDNum();
                      if ((static_cast<BaseColumn*>(vid.getItemExpr()))
                                              ->getNAColumn()->getNotNullNondroppable())
                        {
                          QRLogger::log(CAT_SQL_COMP_RANGE, LL_INFO,
                                        "Range predicate ignored because it spans entire "
                                        "range and has NOT NULL constraint.");
                          deletePtr(rangePredElem);
                          return NULL;
                        }
                    }
                }
                
              return rangePredElem;  // leave it empty
            }
          if (subrange.endInclusive())
            ineqOp = new (mvqrHeap_) QROpLE(ADD_MEMCHECK_ARGS(mvqrHeap_));
          else
            ineqOp = new (mvqrHeap_) QROpLS(ADD_MEMCHECK_ARGS(mvqrHeap_));
          ineqOp->setValue(subrange.getEndScalarValElem(mvqrHeap_, type));
          // If start is not min, we are here because lower bound for the type
          // was start of first subrange.
          ineqOp->setNormalized(!subrange.startIsMin());
          rangePredElem->addOperator(ineqOp);
        }
      else if (subrange.endIsMax() ||
               (i==numSubranges-1 && rangeIsOnCol
                                  && subrange.isMaxForType(type)))
        {
          assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                             i==numSubranges-1, QRDescriptorException,
			     "Subrange other than last is unbounded on high side, "
                                 "subrange index %d",
                             i);
          if (eqOp)                   // wrap this up if one in progress
            {
              rangePredElem->addOperator(eqOp);
              eqOp = NULL;
            }
          if (subrange.startInclusive())
            ineqOp = new (mvqrHeap_) QROpGE(ADD_MEMCHECK_ARGS(mvqrHeap_));
          else
            ineqOp = new (mvqrHeap_) QROpGT(ADD_MEMCHECK_ARGS(mvqrHeap_));
          ineqOp->setValue(subrange.getStartScalarValElem(mvqrHeap_, type));
          // If end is not max, we are here because upper bound for the type
          // was end of last subrange.
          ineqOp->setNormalized(!subrange.endIsMax());
          rangePredElem->addOperator(ineqOp);
        }
      else if (subrange.isSingleValue())
        {
          // Add the value to a new OpEQ or the one we are already working on,
          // but don't finish it until we hit something besides a single-value
          // subrange.
          if (!eqOp)
            eqOp = new (mvqrHeap_) QROpEQ(ADD_MEMCHECK_ARGS(mvqrHeap_));
          eqOp->addValue(subrange.getStartScalarValElem(mvqrHeap_, type));
        }
      else
        {
          // If values have been accumulated in an OpEQ, add it before doing
          // the between op.
          if (eqOp)
            {
              rangePredElem->addOperator(eqOp);
              eqOp = NULL;
            }
          betweenOp = new (mvqrHeap_) QROpBT(ADD_MEMCHECK_ARGS(mvqrHeap_));
          betweenOp->setStartValue(subrange.getStartScalarValElem(mvqrHeap_,
                                                                  type));
          betweenOp->setStartIncluded(subrange.startInclusive());
          betweenOp->setEndValue(subrange.getEndScalarValElem(mvqrHeap_,
                                                              type));
          betweenOp->setEndIncluded(subrange.endInclusive());
          rangePredElem->addOperator(betweenOp);
        }
    }

  // If IS NULL is part of the range spec, it should come last. If an OpEQ
  // element is in progress, add it there, else create one for it. In either
  // of these cases, add it to the range pred element.
  if (eqOp)
    {
      if (nullIncluded_)
        eqOp->setNullVal(new(mvqrHeap_) QRNullVal(ADD_MEMCHECK_ARGS(mvqrHeap_)));
      rangePredElem->addOperator(eqOp);
    }
  else if (nullIncluded_)
    {
      eqOp = new (mvqrHeap_) QROpEQ(ADD_MEMCHECK_ARGS(mvqrHeap_));
      eqOp->setNullVal(new(mvqrHeap_) QRNullVal(ADD_MEMCHECK_ARGS(mvqrHeap_)));
      rangePredElem->addOperator(eqOp);
    }

  return rangePredElem;
}  // createRangeElem()

QRElementPtr OptRangeSpec::genRangeItem()
{
  QRElementPtr elem;
  if (rangeColValueId_ != NULL_VALUE_ID)
    elem = descGenerator_->genQRColumn(rangeColValueId_,
                                       rangeJoinPredId_);
  else if (rangeExpr_)
    elem = descGenerator_->genQRExpr(rangeExpr_, rangeJoinPredId_);
  else
    {
      assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                        rangeJoinPredId_ != NULL_VALUE_ID, QRDescriptorException,
                        "All range value ids are null");
      ValueId colVid = getBaseCol(rangeJoinPredId_);
      elem = descGenerator_->genQRColumn(colVid,
                                         rangeJoinPredId_);
    }
  return elem;
}

ItemExpr* OptRangeSpec::getRangeExpr() const
{
  if (rangeExpr_)
    return rangeExpr_;
  else if (rangeJoinPredId_ != NULL_VALUE_ID)
    return ((ValueId)rangeJoinPredId_).getItemExpr();
  else
    return ((ValueId)rangeColValueId_).getItemExpr();
}

// Exprs have been converted to a canonical form in which const is the 2nd operand.
// NULL is returned if there is not a constant operand, or if the other operand
// is not the one the range is being built for.
ConstValue* OptRangeSpec::getConstOperand(ItemExpr* predExpr, Lng32 constInx)
{
  QRTRACER("getConstOperand");
  ItemExpr* left = predExpr->child(0);
  ItemExpr* right = predExpr->child(constInx);

  // Bail out if we don't have a constant. If a vegref, see if the veg includes
  // a constant, and substitute that if so.
  if (right->getOperatorType() != ITM_CONSTANT)
    {
      if (right->getOperatorType() == ITM_VEG_REFERENCE)
        {
          ValueId constVid = (static_cast<VEGReference*>(right))->getVEG()->getAConstant(TRUE);
          if (constVid == NULL_VALUE_ID)
            return NULL;
          else
            right = constVid.getItemExpr();
        }
      else
        return NULL;
    }

  ValueId colVid;
  switch (left->getOperatorType())
    {
      case ITM_VEG_REFERENCE:
        if (forNormalizer())
          colVid = left->getValueId();
        else
          colVid = getBaseCol(((VEGReference*)left)->getVEG()->getAllValues());
        break;
      case ITM_BASECOLUMN:  // Should only happen for a check constraint
        colVid = left->getValueId();
        break;
      default:  // must be an expression
        colVid = NULL_VALUE_ID;
        break;
    }

  if (colVid != NULL_VALUE_ID)
    {
      // If this range is for an expression, the pred is not on it.
      if (rangeExpr_)
        return NULL;

      if (rangeColValueId_ == NULL_VALUE_ID)
        {
          rangeColValueId_ = colVid;
          EqualitySet* eqSet =
                    descGenerator_->getEqualitySet(&rangeColValueId_);
          if (eqSet)
            {
              rangeJoinPredId_ = (QRValueId)eqSet->getJoinPredId();
              setType(eqSet->getType());
            }
          else
            {
              rangeJoinPredId_ = (QRValueId)NULL_VALUE_ID;
              setType(&((ValueId)rangeColValueId_).getType());
            }
        }
      else if (rangeColValueId_ != colVid)
        return NULL;
    }
  else
    {
      // The left side of the pred is an expression. If this range is for a
      // simple column, it doesn't match.
      if (rangeColValueId_ != NULL_VALUE_ID)
        return NULL;

      if (!rangeExpr_)
        {
          // If more than one node is involved in an expression, it is a
          // residual pred instead of a range pred.
          if (descGenerator_->getExprNode(left) == NULL_CA_ID)
            return NULL;

          setRangeExpr(left);  // sets rangeExpr_, rangeExprText_
          EqualitySet* eqSet =
                    descGenerator_->getEqualitySet(&rangeExprText_);
          if (eqSet)
            {
              rangeJoinPredId_ = (QRValueId)eqSet->getJoinPredId();
              setType(eqSet->getType());
            }
          else
            {
              rangeJoinPredId_ = (QRValueId)NULL_VALUE_ID;
              setType(&rangeExpr_->getValueId().getType());
            }
        }
      else
        {
          // The ItemExpr ptrs will be different, so we compare the expression
          // text to see if they are the same. At some point this text will be
          // in a canonical form that ignores syntactic variances.
          NAString exprText;
          left->unparse(exprText, OPTIMIZER_PHASE, MVINFO_FORMAT);
          if (rangeExprText_ != exprText)
            return NULL;
        }
    }

  if (!(QRDescGenerator::typeSupported(getType())))
    return NULL;

  // If we reach this point, the predicate has been confirmed to apply to the
  // same col/expr of this range, and the right operand has been confirmed to
  // be a constant. Before returning the ConstValue, make sure it is a type we
  // currently support. Predicates involving types not yet supported will be
  // treated as residual predicates.
  if (QRDescGenerator::typeSupported(static_cast<ConstValue*>(right)->getType()))
    return static_cast<ConstValue*>(right);
  else
    return NULL;
} // getConstOperand()

void OptRangeSpec::addSubrange(ConstValue* start, ConstValue* end,
                               NABoolean startInclusive, NABoolean endInclusive)
{
  QRTRACER("addSubrange");
  const NAType* type = getType();
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    type, QRDescriptorException,
                    "Call to getType() returned NULL in OptRangeSpec::addSubrange().");
                    
  NAString unparsedStart(""), unparsedEnd("");
  if (isDumpMvMode())
  {
    // Add the "official" unparsed text of the expression as a sub-element.
    if (start)
      start->unparse(unparsedStart, OPTIMIZER_PHASE, QUERY_FORMAT);
    if (end)
      end->unparse(unparsedEnd, OPTIMIZER_PHASE, QUERY_FORMAT);
  }            

  NABuiltInTypeEnum typeQual = type->getTypeQualifier();
  switch (typeQual)
    {
      case NA_NUMERIC_TYPE:
      case NA_DATETIME_TYPE:
      case NA_INTERVAL_TYPE:
      case NA_BOOLEAN_TYPE:
        //if (((const NumericType*)type)->isExact())
        if (typeQual == NA_DATETIME_TYPE ||
            typeQual == NA_INTERVAL_TYPE ||
            (typeQual == NA_NUMERIC_TYPE && 
             static_cast<const NumericType*>(type)->isExact()) ||
            (typeQual == NA_BOOLEAN_TYPE))
          {
            // Fixed-point numeric subranges are normalized to be inclusive, to
            // simplify equivalence and subsumption checks.
            Subrange<Int64>* sub = new (mvqrHeap_) Subrange<Int64>(logLevel_);
            sub->setUnparsedStart(unparsedStart);
            sub->setUnparsedEnd(unparsedEnd);
            NABoolean valTruncated;
            NABoolean valNegative;
            NABoolean startOverflowed = FALSE;
            NABoolean endOverflowed = FALSE;
            if (start)
              {
                // If the constant is truncated because it has higher scale than
                // the type of the range col/expr, the truncated value is not the
                // start of the range. 1 is added to the truncated value to get
                // the next value that is possible for the type.
                sub->start = getInt64Value(start, type, valTruncated, valNegative, logLevel_);
                if ((!startInclusive || valTruncated) &&
                    (!valTruncated || !valNegative))
                  sub->makeStartInclusive(type, startOverflowed);
              }
            else
              sub->setStartIsMin(TRUE);
            if (end)
              {
                // If the constant is truncated because it has higher scale than
                // the type of the range col/expr, the truncated value must be
                // included in the range even if the end is not inclusive (<).
                sub->end = getInt64Value(end, type, valTruncated, valNegative, logLevel_);
                if ((!endInclusive && !valTruncated) ||
                    (valTruncated && valNegative))
                  sub->makeEndInclusive(type, endOverflowed);
              }
            else
              sub->setEndIsMax(TRUE);

            // If not originally inclusive, has been adjusted above.
            // Need this in case makeXXXInclusive was not called, but leave as
            // is if made noninclusive because of positive (for start) or 
            // negative (for end) overflow.
            if (!startOverflowed)
              sub->setStartInclusive(TRUE);
            if (!endOverflowed)
              sub->setEndInclusive(TRUE);

            // Handling a constant with scale that exceeds that of the range
            // column could result in an empty (i.e., start>end) range. For example,
            // if n is a numeric(4,2), the predicate n = 12.341 will result in
            // the range 12.35..12.34, which is appropriate since the predicate
            // is guaranteed to be false by the type constraint of the column.
            if (sub->startIsMin() || sub->endIsMax() || sub->start <= sub->end)
              placeSubrange(sub);
            else
              delete sub;
          }
        else
          {
            Subrange<double>* sub = new (mvqrHeap_) Subrange<double>(logLevel_);
            sub->setUnparsedStart(unparsedStart);
            sub->setUnparsedEnd(unparsedEnd);
            if (start)
              sub->start = getDoubleValue(start, logLevel_);
            else
              sub->setStartIsMin(TRUE);
            if (end)
              sub->end = getDoubleValue(end, logLevel_);
            else
              sub->setEndIsMax(TRUE);
            sub->setStartInclusive(startInclusive);
            sub->setEndInclusive(endInclusive);
            placeSubrange(sub);
          }
        break;

      // In some cases, constant folding of char expressions produces a varchar
      // constant, so we have to take a possible length field into account.
      case NA_CHARACTER_TYPE:
        {
          Lng32 headerBytes;
          const NAType* startType = (start ? start->getType() : NULL);
          const NAType* endType = (end ? end->getType() : NULL);

          // Alignment is 2 for UCS2, 1 for single-byte char set.
          if (type->getDataAlignment() == 2)
            {
              // Unicode string.
              Subrange<RangeWString>* sub = new (mvqrHeap_) Subrange<RangeWString>(logLevel_);
              sub->setUnparsedStart(unparsedStart);
              sub->setUnparsedEnd(unparsedEnd);
              if (start)
                {
                  headerBytes = startType->getVarLenHdrSize() +
                                startType->getSQLnullHdrSize();
                  sub->start.remove(0)
                            .append((const NAWchar*)start->getConstValue() 
                                        + (headerBytes / sizeof(NAWchar)),
                                    (start->getStorageSize() - headerBytes)
                                        / sizeof(NAWchar));
                }
              else
                sub->setStartIsMin(TRUE);
              if (end)
                {
                  headerBytes = endType->getVarLenHdrSize() +
                                endType->getSQLnullHdrSize();
                  sub->end.remove(0)
                          .append((const NAWchar*)end->getConstValue()
                                        + (headerBytes / sizeof(NAWchar)),
                                   (end->getStorageSize() - headerBytes)
                                        / sizeof(NAWchar));
                }
              else
                sub->setEndIsMax(TRUE);
              sub->setStartInclusive(startInclusive);
              sub->setEndInclusive(endInclusive);
              placeSubrange(sub);
            }
          else
            {
              // Latin1 string.
              Subrange<RangeString>* sub = new (mvqrHeap_) Subrange<RangeString>(logLevel_);
              sub->setUnparsedStart(unparsedStart);
              sub->setUnparsedEnd(unparsedEnd);
              if (start)
                {
                  headerBytes = startType->getVarLenHdrSize() +
                                startType->getSQLnullHdrSize();
                  sub->start.remove(0)
                            .append((const char*)start->getConstValue() + headerBytes,
                                    start->getStorageSize() - headerBytes);
                }
              else
                sub->setStartIsMin(TRUE);
              if (end)
                {
                  headerBytes = endType->getVarLenHdrSize() +
                                endType->getSQLnullHdrSize();
                  sub->end.remove(0)
                          .append((const char*)end->getConstValue() + headerBytes,
                                  end->getStorageSize() - headerBytes);
                }
              else
                sub->setEndIsMax(TRUE);
              sub->setStartInclusive(startInclusive);
              sub->setEndInclusive(endInclusive);
              placeSubrange(sub);
            }
        }
        break;

      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                           FALSE, QRDescriptorException, 
                           "Unhandled data type: %d", typeQual);
        break;
    }
}  // addSubrange(ConstValue*...


ItemExpr* OptRangeSpec::getCheckConstraintPred(ItemExpr* checkConstraint)
{
  if (checkConstraint->getOperatorType() != ITM_CASE)
    {
      QRLogger::log(CAT_SQL_COMP_RANGE, logLevel_,
        "Expected ITM_CASE but found operator %d.",
        checkConstraint->getOperatorType());
      return NULL;
    }

  ItemExpr* itemExpr = checkConstraint->child(0);
  if (itemExpr->getOperatorType() != ITM_IF_THEN_ELSE)
    {
      QRLogger::log(CAT_SQL_COMP_RANGE, logLevel_,
        "Expected ITM_IF_THEN_ELSE but found operator %d.",
        itemExpr->getOperatorType());
      return NULL;
    }

  // Child of the if-then-else is either is_false, which is the parent of the
  // predicate (for most check constraints), or the predicate itself (for
  // isnotnull or the check option of a view).
  itemExpr = itemExpr->child(0);
  if (itemExpr->getOperatorType() == ITM_IS_FALSE)
    return itemExpr->child(0);
  else
    return itemExpr;
}

void OptRangeSpec::intersectCheckConstraints(QRDescGenerator* descGen,
                                             ValueId colValId)
{
  QRTRACER("intersectCheckConstraints");

  // Check and Not Null constraints.
  //
  ItemExpr* itemExpr = colValId.getItemExpr();
  if (itemExpr->getOperatorType() == ITM_VEG_REFERENCE)
    {
      // For a vegref, intersect all constraints applied to any member.
      const ValueIdSet& vidSet = static_cast<VEGReference*>(itemExpr)
                                                    ->getVEG()->getAllValues();
      for (ValueId vid=vidSet.init(); vidSet.next(vid); vidSet.advance(vid)) 
        {
          if (vid.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
            intersectCheckConstraints(descGen, vid);
        }
      return;
    }
  else if (itemExpr->getOperatorType() != ITM_BASECOLUMN)
    {
      QRLogger::log(CAT_SQL_COMP_RANGE, logLevel_,
        "Nonfatal unexpected result: range column operator type is "
        "%d instead of ITM_BASECOLUMN.", itemExpr->getOperatorType());
      return;
    }

#ifdef _DEBUG
    const NATable* tbl = colValId.getNAColumn()->getNATable();
    const CheckConstraintList& checks = tbl->getCheckConstraints();
    for (CollIndex c=0; c<checks.entries(); c++)
      {
        QRLogger::log(CAT_SQL_COMP_RANGE, LL_DEBUG,
          "Check constraint on table %s: %s",
                    tbl->getTableName().getObjectName().data(),
                    checks[c]->getConstraintText().data());
      }
#endif

  OptRangeSpec* checkRange = NULL;
  ItemExpr* checkPred;
  const ValueIdList& checkConstraints =
      (static_cast<BaseColumn*>(itemExpr))->getTableDesc()->getCheckConstraints();
  for (CollIndex i=0; i<checkConstraints.entries(); i++)
    {
      checkPred = getCheckConstraintPred(checkConstraints[i].getItemExpr());
      if (checkPred)
        {
          checkRange = new(mvqrHeap_) OptRangeSpec(descGen, mvqrHeap_);
          checkRange->setRangeColValueId(colValId);
          checkRange->setType(colValId.getType().newCopy(mvqrHeap_));
          if (checkRange->buildRange(checkPred))
            // Call the RangeSpec version of intersect; this avoids trying to
            // modify the original ItemExpr with the check constraint pred.
            RangeSpec::intersectRange(checkRange);
          delete checkRange;
        }
    }
}

// Add type-implied constraint for numeric, datetime, and interval types.
void OptRangeSpec::intersectTypeConstraint(QRDescGenerator* descGen,
                                           ValueId colValId)
{
  QRTRACER("intersectTypeConstraint");

  NABoolean isExact;
  const NAType& colType = colValId.getType();
  NABuiltInTypeEnum typeQual = colType.getTypeQualifier();

  switch (typeQual)
    {
      case NA_NUMERIC_TYPE:
        {
          const NumericType& numType = static_cast<const NumericType&>(colType);
          isExact = numType.isExact();
          if (isExact && numType.getFSDatatype() == REC_BIN64_SIGNED)
            return;  // No type restriction for 64-bit integers
        }
        break;

      case NA_DATETIME_TYPE:
      case NA_INTERVAL_TYPE:
        isExact = TRUE;
        break;

      default:
        return;      // No type constraint applied for other types
    }

  OptRangeSpec* typeRange = NULL;
  if (isExact)  // Exact numeric, datetime, interval
    {
      // Add the subrange implied by the type. If the type is largeint,
      // nothing is needed here and no type range will be created. We don't
      // need to set the type of the range specs created in this function,
      // because addSubrange (which looks at the type) is bypassed and we call
      // placeSubrange directly.
      typeRange = new(mvqrHeap_) OptRangeSpec(descGen, mvqrHeap_);
      Int64 start, end;
      SubrangeBase::getExactNumericMinMax(colType, start, end, logLevel_);
      Subrange<Int64>* numSubrange = new(mvqrHeap_) Subrange<Int64>(logLevel_);
      numSubrange->setUnparsedStart("");
      numSubrange->setUnparsedEnd("");
      numSubrange->start = start;
      numSubrange->end = end;
      numSubrange->setStartIsMin(FALSE);
      numSubrange->setEndIsMax(FALSE);
      numSubrange->setStartInclusive(TRUE);
      numSubrange->setEndInclusive(TRUE);
      typeRange->placeSubrange(numSubrange);
    }
  else  // approximate numeric
    {
      switch (colType.getFSDatatype())
        {
          case REC_IEEE_FLOAT32:
            {
              typeRange = new(mvqrHeap_) OptRangeSpec(descGen, mvqrHeap_);
              Subrange<double>* dblSubrange = new(mvqrHeap_) Subrange<double>(logLevel_);
              dblSubrange->end = static_cast<const NumericType&>(colType).getMaxValue();
              dblSubrange->setUnparsedStart("");
              dblSubrange->setUnparsedEnd("");
              dblSubrange->start = -(dblSubrange->end);
              dblSubrange->setStartIsMin(FALSE);
              dblSubrange->setEndIsMax(FALSE);
              dblSubrange->setStartInclusive(TRUE);
              dblSubrange->setEndInclusive(TRUE);
              typeRange->placeSubrange(dblSubrange);
            }
            break;

          case REC_IEEE_FLOAT64:
            // No range restriction needed.
            break;

          default:
            QRLogger::log(CAT_SQL_COMP_RANGE, logLevel_,
              "No case in intersectTypeConstraint() for "
                        "approximate numeric of type %d",
                        colType.getFSDatatype());
            break;
        }
    }

  if (typeRange)
    {
      typeRange->setNullIncluded(TRUE); // Null always part of a type range
      // Call the RangeSpec version of intersect; this avoids trying to
      // modify the original ItemExpr with the type constraint pred.
      RangeSpec::intersectRange(typeRange);
      delete typeRange;
    }
}  // intersectTypeConstraint()

void OptRangeSpec::addConstraints(QRDescGenerator* descGen)
{
  QRTRACER("addConstraints");

  ValueId colValId = getRangeColValueId();
  if (colValId == NULL_VALUE_ID)
    colValId = getRangeJoinPredId();
  if (colValId == NULL_VALUE_ID)
    return;

  // Add constraint implied by the column's type.
  intersectTypeConstraint(descGen, colValId);

  // Check constraints can be added and dropped at will, and so can be in
  // different states when the MV is created and when the query is matched.
  // Therefore we utilize check constraints only for query descriptors. This
  // may result in a NotProvided instead of a Provided match, but avoids the
  // need to invalidate all the MV descriptors using a table when one of the
  // table's check constraints is added/dropped.
  if (descGen->getDescriptorType() == ET_QueryDescriptor)
    intersectCheckConstraints(descGen, colValId);
//  else
//    assertLogAndThrow1(descGen->getDescriptorType() == ET_MVDescriptor,
//                       QRDescriptorException,
//                       "Invalid descriptor type -- %d",
//                       descGen->getDescriptorType());
}

void OptRangeSpec::addColumnsUsed(const QRDescGenerator* descGen)
{
  if (descGen != descGenerator_)
    descGenerator_->mergeDescGenerator(descGen);
}

#define AVR_STATE0 0
#define AVR_STATE1 1
#define AVR_STATE2 2

NABoolean OptRangeSpec::buildRange(ItemExpr* origPredExpr)
{
  QRTRACER("buildRange");
  ConstValue *startValue, *endValue;
  OperatorTypeEnum leftOp, rightOp;
  NABoolean isRange = TRUE;
  NABoolean reprocessAND = FALSE;

  //
  // buildRange() can be called recursively for all the items in an IN-list
  // at a point in time when we are already many levels deep in other
  // recursion (e.g. Scan::applyAssociativityAndCommutativity() ). Consequently,
  // we may not have much of our stack space available at the time, so
  // we must eliminate the recursive calls to buildRange() by keeping the
  // information needed by each "recursive" level in the heap and using
  // a "while" loop to look at each node in the tree in the same order as
  // the old recursive technique would have done.
  // The information needed by each "recursive" level is basically just
  // a pointer to what node (ItemExpr *) to look at next and a "state" value
  // that tells us where we are in the buildRange() code for the ItemExpr
  // node that we are currently working on.
  //
  ARRAY( ItemExpr * ) IEarray(mvqrHeap_, 10) ; //Initially 10 elements (no particular reason to choose 10)
  ARRAY( Int16 )      state(mvqrHeap_, 10)   ; //These ARRAYs will grow automatically as needed.)

  Int32 currIdx     = 0 ;
  IEarray.insertAt( currIdx, origPredExpr ) ; //Initialize 1st element in the ARRAYs
  state.insertAt(   currIdx, AVR_STATE0   ) ;

  while( currIdx >= 0 && isRange )
  {
    ItemExpr * predExpr = IEarray[currIdx] ;  //Get ptr to the current IE
    OperatorTypeEnum op = predExpr->getOperatorType();
    switch (op)
    {
      case ITM_AND:
        // Check for a bounded subrange, from BETWEEN predicate, etc. If not of
        // this form, the predicate was presumably too complex to convert to
        // conjunctive normal form without a combinatorial explosion of clauses,
        // and we handle it by creating separate range objects for each operand
        // of the AND, intersecting them, and then unioning the result with the
        // primary range.
        leftOp = predExpr->child(0)->getOperatorType();
        rightOp = predExpr->child(1)->getOperatorType();
        if (leftOp == ITM_LESS || leftOp == ITM_LESS_EQ)
          {
            if (rightOp == ITM_GREATER || rightOp == ITM_GREATER_EQ)
              {
                endValue = getConstOperand(predExpr->child(0));
                if (endValue)
                  {
                    startValue = getConstOperand(predExpr->child(1));
                    if (startValue)
                      addSubrange(startValue, endValue,
                                  rightOp == ITM_GREATER_EQ,
                                  leftOp == ITM_LESS_EQ);
                    else
                      isRange = FALSE;
                  }
                else
                  isRange = FALSE;
              }
            else
              reprocessAND = TRUE;
          }
        else if (leftOp == ITM_GREATER || leftOp == ITM_GREATER_EQ)
          {
            if (rightOp == ITM_LESS || rightOp == ITM_LESS_EQ)
              {
                startValue = getConstOperand(predExpr->child(0));
                if (startValue)
                  {
                    endValue = getConstOperand(predExpr->child(1));
                    if (endValue)
                      addSubrange(startValue, endValue,
                                  leftOp == ITM_GREATER_EQ,
                                  rightOp == ITM_LESS_EQ);
                    else
                      isRange = FALSE;
                  }
                else
                  isRange = FALSE;
              }
            else
              reprocessAND = TRUE;
          }
        else
          reprocessAND = TRUE;

        // AND was used in a sense other than that of a BETWEEN predicate, so
        // we must intersect the operand ranges before adding the result to the
        // overall range.
        if (reprocessAND)
          {
            OptRangeSpec *leftRange = NULL, *rightRange = NULL;
            leftRange = createRangeSpec(descGenerator_, predExpr->child(0),
                                        mvqrHeap_, forNormalizer());
            if (!leftRange)
              isRange = FALSE;
            else if (!rangeSubjectIsSet())
              setRangeSubject(leftRange);
            else if (!sameRangeSubject(leftRange))
              isRange = FALSE;

            if (isRange)
              {
                rightRange = createRangeSpec(descGenerator_, predExpr->child(1),
                                             mvqrHeap_, forNormalizer());
                if (rightRange && rightRange->sameRangeSubject(leftRange))
                  {
                    leftRange->intersectRange(rightRange);
                    // Call only the superclass part of unionRange(); we want
                    // to avoid the part that modifies the originalItemExpr_;
                    RangeSpec::unionRange(leftRange);
                  }
                else
                  isRange = FALSE;
              }
            delete leftRange;
            delete rightRange;
          }
        break;

      case ITM_OR:
        if ( state[currIdx] == AVR_STATE0 )
        {
           state.insertAt( currIdx, AVR_STATE1 ) ;
           currIdx++ ;                               //"Recurse" down to child 0
           state.insertAt(   currIdx, AVR_STATE0 ) ; // and start that child's state at 0
           IEarray.insertAt( currIdx, predExpr->child(0) ) ;
           continue ;
        }
        else if ( state[currIdx] == AVR_STATE1 )
        {
           state.insertAt( currIdx, AVR_STATE2 ) ;
           currIdx++ ;                               //"Recurse" down to child 1
           state.insertAt(   currIdx, AVR_STATE0 ) ; // and start that child's state at 0
           IEarray.insertAt( currIdx, predExpr->child(1) ) ;
           continue ;
        }
        else
           state.insertAt( currIdx, AVR_STATE0 ); // We are done processing predExpr
        break ;

      case ITM_EQUAL:
        startValue = endValue = getConstOperand(predExpr);
        if (startValue)
          addSubrange(startValue, endValue, TRUE, TRUE);
        else
          isRange = FALSE;
        break;

      case ITM_LESS:
        endValue = getConstOperand(predExpr);
        if (endValue)
          addSubrange(NULL, endValue, TRUE, FALSE);
        else
          isRange = FALSE;
        break;

      case ITM_LESS_EQ:
        endValue = getConstOperand(predExpr);
        if (endValue)
          addSubrange(NULL, endValue, TRUE, TRUE);
        else
          isRange = FALSE;
        break;

      case ITM_GREATER:
        startValue = getConstOperand(predExpr);
        if (startValue)
          addSubrange(startValue, NULL, FALSE, TRUE);
        else
          isRange = FALSE;
        break;

      case ITM_GREATER_EQ:
        startValue = getConstOperand(predExpr);
        if (startValue)
          addSubrange(startValue, NULL, TRUE, TRUE);
        else
          isRange = FALSE;
        break;

      case ITM_NOT_EQUAL:
        startValue = endValue = getConstOperand(predExpr);
        if (startValue)
          {
            addSubrange(NULL, endValue, TRUE, FALSE);
            addSubrange(startValue, NULL, FALSE, TRUE);
          }
        else
          isRange = FALSE;
        break;

      case ITM_BETWEEN:
        startValue = getConstOperand(predExpr);
        if (startValue)
          {
            endValue = getConstOperand(predExpr, 2);
            if (endValue)
              //@ZX The Between class has private member variables
              //    leftBoundryIncluded_ and rightBoundryIncluded_ that have
              //    no accessor functions. If it is in fact possible for the
              //    boundaries to be noninclusive, we will need access to
              //    these properties.
              addSubrange(startValue, endValue, TRUE, TRUE);
          }
        else
          isRange = FALSE;
        break;

      case ITM_IS_NULL:
      case ITM_IS_NOT_NULL:
        {
          ValueId colVid;
          ItemExpr* child = predExpr->child(0);
          if (child->getOperatorType() == ITM_VEG_REFERENCE)
            {
              // If this range is for an expression, the pred is not on it.
              if (rangeExpr_)
                isRange = FALSE;
              else
                {
                  VEG* veg = ((VEGReference*)child)->getVEG();
                  if (forNormalizer())
                    colVid = veg->getVEGReference()->getValueId();
                  else
                    colVid = getBaseCol(veg->getAllValues());
                  if (rangeColValueId_ == NULL_VALUE_ID)
                    {
                      rangeColValueId_ = colVid;
                      EqualitySet* eqSet =
                              descGenerator_->getEqualitySet(&rangeColValueId_);
                      if (eqSet)
                        {
                          rangeJoinPredId_ = (QRValueId)eqSet->getJoinPredId();
                          setType(eqSet->getType());
                        }
                      else
                        {
                          rangeJoinPredId_ = (QRValueId)NULL_VALUE_ID;
                          setType(&((ValueId)rangeColValueId_).getType());
                        }
                    }
                  else if (rangeColValueId_ != colVid)
                    isRange = FALSE;
                }
            }
          else
            {
              // The left side of the pred is an expression. If this range is 
              // for a simple column, it doesn't match.
              if (rangeColValueId_ != NULL_VALUE_ID)
                isRange = FALSE;
              else if (!rangeExpr_)
                {
                  setRangeExpr(child);  // sets rangeExpr_, rangeExprText_
                  EqualitySet* eqSet =
                            descGenerator_->getEqualitySet(&rangeExprText_);
                  if (eqSet)
                    {
                      rangeJoinPredId_ = (QRValueId)eqSet->getJoinPredId();
                      setType(eqSet->getType());
                    }
                  else
                    {
                      rangeJoinPredId_ = (QRValueId)NULL_VALUE_ID;
                      setType(&rangeExpr_->getValueId().getType());
                    }
                }
              else
                {
                  NAString exprText;
                  child->unparse(exprText, OPTIMIZER_PHASE, MVINFO_FORMAT);
                  if (rangeExprText_ != exprText)
                    isRange = FALSE;
                }
            }

          // Now that it has been validated as a range pred, take the appropriate
          // action depending on whether the op is IS NULL or IS NOT NULL.
          if (isRange)
            {
              if (op == ITM_IS_NULL)
                nullIncluded_ = TRUE;
              else
                {
                  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_, op == ITM_IS_NOT_NULL,
                                    QRLogicException,
                                    "op must be ITM_IS_NOT_NULL here");
                  addSubrange(NULL, NULL, TRUE, TRUE);
                }
            }
        }
        break;

      default:
        isRange = FALSE;
        break;
    }
    if ( state[currIdx] == AVR_STATE0 )
       currIdx-- ;  // Go back to the parent node & continue working on it.
  }

  return isRange;
} // buildRange()

// Local helper function to cast int64, which, as the widest integral type is
// used for rangespec processing, to the actual type of the column the rangespec
// applies to. numBuf is declared as Int64* to ensure proper alignment for all
// possible integral types.
static void downcastRangespecInt(Int64 val, Lng32 scale, NAType*& type,
                                 Int64* numBuf, NAMemory* heap)
{
  Lng32 precision = 0;  // Only used for non-integral exact numeric

  // For non-integer values with leading 0's, have to adjust precision to
  // equal scale. For example, the raw value 23 with a scale of 4 (.0023)
  // will initially be calculated to have precision 2 instead of 4.
  if (scale) // non-integral value
    {
      precision = (Lng32)log10(::abs((double)val)) + 1;
      if (scale > precision)
        precision = scale;
    }

  if (val <= SHRT_MAX && val >= SHRT_MIN)
    {
      *((Int16*)numBuf) = static_cast<Int16>(val);
      if (scale == 0)
        type = new(heap) SQLSmall(heap, TRUE, FALSE);
      else
        type = new(heap) SQLNumeric(heap, sizeof(Int16), precision, scale,
                                    TRUE, FALSE);
    }
  else if (val <= INT_MAX && val >= INT_MIN)
    {
      *((Int32*)numBuf) = static_cast<Int32>(val);
      if (scale == 0)
        type = new(heap) SQLInt(heap, TRUE, FALSE);
      else
        type = new(heap) SQLNumeric(heap, sizeof(Int32), precision, scale,
                                    TRUE, FALSE);
    }
  else
    {
      *numBuf = val;
      if(scale == 0)
        type = new(heap) SQLLargeInt(heap, TRUE, FALSE);
      else
        type = new(heap) SQLNumeric(heap, sizeof(Int64), precision, scale,
                                    TRUE, FALSE);
    }
}

// Instantiate a ConstValue using the actual type of the passed Int64 value.
// The actual type is indicated by the type parameter, and could be any type
// represented as an integer in a rangespec.
ConstValue* OptRangeSpec::reconstituteInt64Value(NAType* type, Int64 val) const
{
  // Use these for the textual representation of a constant value, which is
  // passed to the ConstValue ctor.
  char constValTextBuffer[50];
  NAString constValTextStr;

  NABuiltInTypeEnum typeQual = type->getTypeQualifier();
  switch (typeQual)
    {
      case NA_NUMERIC_TYPE:
        {
          sprintf(constValTextBuffer, PF64, val);
          constValTextStr = constValTextBuffer;
          NumericType* numType = static_cast<NumericType*>(type);
          assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_, 
                            numType->isExact(),
                            QRLogicException,
                            "Expecting exact numeric type in "
                                "reconstituteInt64Value");
          Int64 numBuf;
          NAType* constType = NULL;
          downcastRangespecInt(val, numType->getScale(), constType, &numBuf, mvqrHeap_);
          return new(mvqrHeap_) ConstValue(constType, &numBuf,
                                           constType->getNominalSize(),
                                           &constValTextStr, mvqrHeap_);
        }
        break;

      case NA_DATETIME_TYPE:
        {
          DatetimeType* dtType = static_cast<DatetimeType*>(type);
          ULng32 tsFieldValues[DatetimeValue::N_DATETIME_FIELDS];
          DatetimeValue dtv("", 0);
          switch (dtType->getSubtype())
            {
              case DatetimeType::SUBTYPE_SQLDate:
                // Since Julian timestamp is calculated from noon on the base day,
                // the timestamp corresponding to a given date (without time) is
                // always x.5 days, where x+1 is the actual number of days passed.
                // We truncate the fractional part when we store it, so it has to
                // be added back here. .5 could be added, but to be safe we add a
                // full day minus a microsecond, which is then truncated to the
                // proper value.
                DatetimeValue::decodeTimestamp
                                 ((val+1) * SubrangeBase::MICROSECONDS_IN_DAY - 1,
                                  dtType->getFractionPrecision(), tsFieldValues);
                dtv.setValue(REC_DATE_YEAR, REC_DATE_DAY,
                             dtType->getFractionPrecision(), tsFieldValues);
                assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                                  dtv.isValid(), QRLogicException,
                                  "Invalid date value reconstructed from Julian timestamp");
                constValTextStr = dtv.getValueAsString(*dtType);
                return new(mvqrHeap_) ConstValue(new(mvqrHeap_)SQLDate(mvqrHeap_, FALSE),
                                                 (void*)dtv.getValue(), dtv.getValueLen(),
                                                 &constValTextStr, mvqrHeap_);
                break;

              case DatetimeType::SUBTYPE_SQLTime:
                // The fractional seconds part is represented not by a number
                // of microseconds but by the numerator of the fraction having
                // denominator equal to 10^fractionPrecision.
                tsFieldValues[DatetimeValue::FRACTION] = 
                  (ULng32)((val % 1000000) 
                          / (Int64)pow(10, 6 - dtType->getFractionPrecision()));
                val /= 1000000;
                tsFieldValues[DatetimeValue::SECOND] = (ULng32)(val % 60);
                val /= 60;
                tsFieldValues[DatetimeValue::MINUTE] = (ULng32)(val % 60);
                val /= 60;
                tsFieldValues[DatetimeValue::HOUR] = (ULng32)val;
                dtv.setValue(REC_DATE_HOUR, REC_DATE_SECOND,
                              dtType->getFractionPrecision(), tsFieldValues);
                assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                                  dtv.isValid(), QRLogicException,
                                  "Invalid time value reconstructed from Int64 value");
                constValTextStr = dtv.getValueAsString(*dtType);
                return new(mvqrHeap_)
                        ConstValue(new(mvqrHeap_)SQLTime(mvqrHeap_, FALSE,
                                                         dtType->getFractionPrecision()),
                                  (void*)dtv.getValue(), dtv.getValueLen(),
                                  &constValTextStr, mvqrHeap_);
                break;

              case DatetimeType::SUBTYPE_SQLTimestamp:
                // We represent these as a number of microseconds, so fractional
                // precision does not have to be taken into account.
                DatetimeValue::decodeTimestamp(val,
                                               dtType->getFractionPrecision(),
                                               tsFieldValues);
                dtv.setValue(REC_DATE_YEAR, REC_DATE_SECOND,
                             dtType->getFractionPrecision(), tsFieldValues);
                assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                                  dtv.isValid(), QRLogicException,
                                  "Invalid timestamp value reconstructed from Julian timestamp");
                constValTextStr = dtv.getValueAsString(*dtType);
                return new(mvqrHeap_) ConstValue(new(mvqrHeap_)
                                                   SQLTimestamp(mvqrHeap_, FALSE, 
                                                                dtType->getFractionPrecision()),
                                                 (void*)dtv.getValue(),
                                                 dtv.getValueLen(),
		                            &constValTextStr, mvqrHeap_);
                break;

              default:
                assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                                   FALSE, QRLogicException,
                                   "Unknown datetime subtype -- %d",
                                   dtType->getSubtype());
                return NULL;
                break;
            }
        }
        break;

      case NA_INTERVAL_TYPE:
        {
          Int64 origVal = val;  // Use this later to check for precision loss.
          Int64 scaledVal;      // Compare to origVal to check precision loss.
          Int64 scaleFactor;
          IntervalType* intvlType = static_cast<IntervalType*>(type);

          // For rangespec analysis, all values are converted to months or
          // microseconds. Change it back to its normal internal representation
          // in terms of units of the end field.
          switch (intvlType->getEndField())
            {
              case REC_DATE_YEAR:
                val /= 12;
                scaledVal = val * 12;
                break;

              case REC_DATE_MONTH:
                // No conversion necessary.
                scaledVal = val;
                break;

              case REC_DATE_DAY:
                val /= (24LL * 60 * 60000000);
                scaledVal = val * (24LL * 60 * 60000000);
                break;

              case REC_DATE_HOUR:
                val /= (60LL * 60000000);
                scaledVal = val * (60LL * 60000000);
                break;

              case REC_DATE_MINUTE:
                val /= 60000000LL;
                scaledVal = val * 60000000;
                break;

              case REC_DATE_SECOND:
                scaleFactor = (Int64)pow(10, 6 - intvlType->getFractionPrecision());
                val /= scaleFactor;
                scaledVal = val * scaleFactor;
                break;

              default:
                assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                                  FALSE, QRLogicException,
                                  "Invalid end field for interval type -- %d",
                                  intvlType->getEndField());
                break;
            }

          // Calculate the leading field precision of the interval constant, when
          // converted to units of its end field.
          UInt32 leadingPrec = 0;
          Int64 tempVal = (val >= 0 ? val : -val); // abs has no overload for Int64
          while (tempVal > 0)
            {
              leadingPrec++;
              tempVal /= 10;
            }

          // If the field is seconds, some of the digits we just counted may be
          // fractional seconds, so we subtract them from what we counted.
          if (intvlType->getEndField() == REC_DATE_SECOND)
            {
              UInt32 fracSecPrec = intvlType->getFractionPrecision();
              if (leadingPrec <= fracSecPrec) // could be < for a fraction of a
                leadingPrec = 2;              //   sec with leading zeroes
              else
                leadingPrec -= fracSecPrec;
            }

          // There are cases where we can come up with leading precision of 0,
          // and this causes a problem. Make it min 2, which is the default.
          if (leadingPrec < 2)
            leadingPrec = 2;

          // Any discrepancy in precision between the range column and the
          // constant value should have been dealt with when the value was
          // converted to months or microseconds in getInt64ValueFromInterval().
          assertLogAndThrow2(CAT_SQL_COMP_RANGE, logLevel_,
                             origVal == scaledVal, QRLogicException,
                             "Precision lost in conversion to interval type: "
                               "value in microseconds (or months) = %Ld, "
                               "interval end field = %d",
                             origVal, intvlType->getEndField());

          // IntervalValue::setValue() will cast the Int64 value to the appropriate
          // type if you pass the length of that type, but negative values seem
          // to be stored incorrectly unless they are 8 bytes (see bug 2773), so
          // we just use 8 bytes always.
          IntervalValue intvlVal(NULL, 0);
          intvlVal.setValue(val, SQL_LARGE_SIZE);
          constValTextStr = intvlVal.getValueAsString(*intvlType);

          // The interval type for the constant is derived from that of the column
          // the predicate is on, but is always a single field (the end field of
          // the column type), and has the maximum possible leading field precision
          // to prevent overflow in case the value is outside the range for the
          // column's declared type (can happen when the rangespec is created for
          // the Normalizer rather than MVQR, because type constraints are not
          // incorporated in that case). See bug 2974.
          return new(mvqrHeap_) ConstValue(new(mvqrHeap_)SQLInterval
                                                 (mvqrHeap_, FALSE,
                                                  intvlType->getEndField(),
                                                  leadingPrec,
                                                  intvlType->getEndField(),
                                                  intvlType->getFractionPrecision()),
                                      (void*)intvlVal.getValue(),
                                      intvlVal.getValueLen(),
                                      &constValTextStr, mvqrHeap_);
        }
        break;

      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_, FALSE, QRLogicException,
                           "Type not handled by reconstituteInt64Value() -- %d",
                           typeQual);
        return NULL;
        break;
    }
  // make the compiler happy
  return NULL;
} // reconstituteInt64Value()

// Instantiate a ConstValue using the passed double value. 
ConstValue* OptRangeSpec::reconstituteDoubleValue(NAType* type, Float64 val) const
{
  NABuiltInTypeEnum typeQual = type->getTypeQualifier();
  if (typeQual != NA_NUMERIC_TYPE)
    {
      assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_, FALSE, QRLogicException,
                         "Type not handled by reconstituteDoubleValue() -- %d",
                         typeQual);
      return NULL;
    }

  // Use these for the textual representation of a constant value, which is
  // passed to the ConstValue ctor.
  char constValTextBuffer[50];
  NAString constValTextStr;

  sprintf(constValTextBuffer, "%g", val);
  constValTextStr = constValTextBuffer;
  NumericType* numType = static_cast<NumericType*>(type);
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_, 
                    !numType->isExact(),
                    QRLogicException,
                    "Expecting approximate numeric type in "
                        "reconstituteDoubleValue");

  NAType* constType = new(mvqrHeap_) SQLDoublePrecision(mvqrHeap_, FALSE);
  return new(mvqrHeap_) ConstValue(constType, &val, constType->getNominalSize(),
                                   &constValTextStr, mvqrHeap_);
} // reconstituteDoubleValue()

// Generate a left-linear OR backbone of equality predicates corresponding
// to the values within the subrange.
ItemExpr* OptRangeSpec::makeSubrangeORBackbone(SubrangeBase* subrange,
                                               ItemExpr* subrangeItem) const
{
  QRTRACER("makeSubrangeOrBackbone");

  NAType* type = getType()->newCopy(mvqrHeap_);
  NAType* int64Type = new (mvqrHeap_) SQLLargeInt(mvqrHeap_, TRUE, FALSE );
  type->resetSQLnullFlag();
  type->resetSQLnullHdrSize();
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    type, QRDescriptorException,
                    "Call to getType() returned NULL in "
                    "OptRangeSpec::makeSubrangeOrBackbone().");
  NABuiltInTypeEnum typeQual = type->getTypeQualifier();
  assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                     typeQual == NA_DATETIME_TYPE ||
                       typeQual == NA_INTERVAL_TYPE ||
                       (typeQual == NA_NUMERIC_TYPE && 
                         static_cast<const NumericType*>(type)->isExact()),
                     QRDescriptorException,
                     "Invalid type for makeSubrangeOrBackbone() -- %d", typeQual);
  
  Subrange<Int64>* intSubrange = (Subrange<Int64>*)subrange;
  Int64 startVal = intSubrange->start;
  Int64 endVal = intSubrange->end;
  ItemExpr* top = NULL;  // current top of backbone; eventual return value
  ItemExpr* eqExpr;      // construct each rangeitem=value pred here...
  ConstValue* cv;        // ...using equality to this constant

  for (Int64 val=startVal; val<=endVal; val++)
    {
      cv = reconstituteInt64Value(type, val);
      eqExpr = new(mvqrHeap_) BiRelat(ITM_EQUAL, subrangeItem, cv);
      eqExpr->synthTypeAndValueId();

      if (top)
        top = new(mvqrHeap_) BiLogic(ITM_OR, top, eqExpr);
      else
        top = eqExpr;
    }

  return top;
} // makeSubrangeORBackbone

ItemExpr* OptRangeSpec::makeSubrangeItemExpr(SubrangeBase* subrange,
                                             ItemExpr* subrangeItem) const
{
  QRTRACER("makeSubrangeItemExpr");

  // ItemExpr that we will build and return, representing the subrange.
  ItemExpr* itemExpr;

  // Nodes that the start and end values of the subrange will be attached
  // to (as the 2nd child) as necessary.
  ItemExpr *parentOfStart = NULL, *parentOfEnd = NULL;

  // If the subrange is derived from an IN list or a disjunction of equality
  // predicates, return an OR backbone of ITM_EQUALs.
  if ((subrange->getSpecifiedValueCount() > 0))
    return makeSubrangeORBackbone(subrange, subrangeItem);

  // Build the tree, except for the ConstValue subtrees that will be attached
  // later, after the underlying type of the subrange is determined.
  // parentOfStart and parentOfEnd mark the nodes to attach them to.
  if (subrange->isSingleValue())
    {
      // A single-point subrange may have been the result of intersection of
      // two ranges that left it with adjustment flags adopted from the another
      // subranges. They aren't relevant when the subrange represents a single
      // value, and will distort the value used if left in place.
      subrange->setStartAdjustment(0);
      subrange->setEndAdjustment(0);
      itemExpr = parentOfStart
               = new(mvqrHeap_) BiRelat(ITM_EQUAL, subrangeItem);
    }
  else if (subrange->endIsMax())
    if (subrange->startInclusive() && subrange->getStartAdjustment() == 0)
      itemExpr = parentOfStart
               = new(mvqrHeap_) BiRelat(ITM_GREATER_EQ, subrangeItem);
    else
      itemExpr = parentOfStart
               = new(mvqrHeap_) BiRelat(ITM_GREATER, subrangeItem);
  else if (subrange->startIsMin())
    if (subrange->endInclusive() && subrange->getEndAdjustment() == 0)
      itemExpr = parentOfEnd
               = new(mvqrHeap_) BiRelat(ITM_LESS_EQ, subrangeItem);
    else
      itemExpr = parentOfEnd
               = new(mvqrHeap_) BiRelat(ITM_LESS, subrangeItem);
  else
    {
      itemExpr = new(mvqrHeap_) BiLogic(ITM_AND);
      if (subrange->startInclusive() && subrange->getStartAdjustment() == 0)
        itemExpr->child(0) = parentOfStart
                           = new(mvqrHeap_) BiRelat(ITM_GREATER_EQ, subrangeItem);
      else
        itemExpr->child(0) = parentOfStart
                           = new(mvqrHeap_) BiRelat(ITM_GREATER, subrangeItem);
      if (subrange->endInclusive() && subrange->getEndAdjustment() == 0)
        itemExpr->child(1) = parentOfEnd
                           = new(mvqrHeap_) BiRelat(ITM_LESS_EQ, subrangeItem);
      else
        itemExpr->child(1) = parentOfEnd
                           = new(mvqrHeap_) BiRelat(ITM_LESS, subrangeItem);
    }

  // Now the item expression is complete except for filling in the constants
  // at the appropriate locations, denoted by the parentOfStart and parentOfEnd
  // ItemExpr ptrs, each of which is null if not applicable (e.g., an unbounded
  // range will only use either the start or end value, not both).
  NAType* type = getType()->newCopy(mvqrHeap_);
  type->resetSQLnullFlag();
  type->resetSQLnullHdrSize();
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    type, QRDescriptorException,
                    "Call to getType() returned NULL in "
                    "OptRangeSpec::makeSubrangeItemExpr().");
  NABuiltInTypeEnum typeQual = type->getTypeQualifier();
  switch (typeQual)
    {
      case NA_NUMERIC_TYPE:
      case NA_DATETIME_TYPE:
      case NA_INTERVAL_TYPE:
        if (typeQual == NA_DATETIME_TYPE ||
            typeQual == NA_INTERVAL_TYPE ||
            (typeQual == NA_NUMERIC_TYPE && 
               static_cast<const NumericType*>(type)->isExact()))
          {
            Subrange<Int64>* intSubrange = (Subrange<Int64>*)subrange;
            Int64 startVal = intSubrange->start - intSubrange->getStartAdjustment();
            Int64 endVal = intSubrange->end + intSubrange->getEndAdjustment();
            if (parentOfStart)
              parentOfStart->child(1) = reconstituteInt64Value(type, startVal);
            if (parentOfEnd)
              parentOfEnd->child(1) = reconstituteInt64Value(type, endVal);
          }
        else
          {
            Subrange<double>* dblSubrange = (Subrange<double>*)subrange;
            double startVal = dblSubrange->start;
            double endVal = dblSubrange->end;
            if (parentOfStart)
              parentOfStart->child(1) = reconstituteDoubleValue(type, startVal);
            if (parentOfEnd)
              parentOfEnd->child(1) = reconstituteDoubleValue(type, endVal);
          }
        break;

      case NA_CHARACTER_TYPE:
        {
          // Alignment is 2 for UCS2, 1 for single-byte char set.
          if (type->getDataAlignment() == 2)
            {
              // Unicode string.
              Subrange<RangeWString>* wcharSubrange =
                             (Subrange<RangeWString>*)subrange;
              if (parentOfStart)
                parentOfStart->child(1) =
                            new(mvqrHeap_) ConstValue(wcharSubrange->start);
              if (parentOfEnd)
                parentOfEnd->child(1) =
                            new(mvqrHeap_) ConstValue(wcharSubrange->end);
            }
          else
            {
              // Latin1 string.
              Subrange<RangeString>* charSubrange 
                      = (Subrange<RangeString>*)subrange;
              if (parentOfStart)
                parentOfStart->child(1) =
                            new(mvqrHeap_) ConstValue(charSubrange->start, ((CharType *)type)->getCharSet() );
              if (parentOfEnd)
                parentOfEnd->child(1) =
                            new(mvqrHeap_) ConstValue(charSubrange->end, ((CharType *)type)->getCharSet() );
            }
        }
        break;

      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_, 
                           FALSE, QRDescriptorException, 
                           "Unhandled data type in "
                           "OptRangeSpec::makeSubrangeItemExpr: %d",
                           typeQual);
        break;
    }

  // This NAType object is never used in the creation of a ConstValue.
  delete type;

  return itemExpr;
}  // makeSubrangeItemExpr()

void OptRangeSpec::intersectRange(OptRangeSpec* other)
{
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    sameRangeSubject(other) || !other->rangeSubjectIsSet(),
                    QRDescriptorException,
                    "Intersecting with range on a different col/expr, or range "
                    "subject is not set");

  // Call the superclass version to intersect the ranges, and mark this range
  // spec as being an intersection. The isIntersection_ flag will cause the
  // itemexpr representing the overall range to be created, so its value id
  // can be used as the id attribute of the RangePred element, instead of that
  // of the initial predicate on the range column/expr. This is critical,
  // because the predicate corresponding to that id is the one used to carry
  // out a NotProvided rewrite instruction for a range pred.
  RangeSpec::intersectRange(other);
  isIntersection_ = TRUE;
}

void OptRangeSpec::unionRange(OptRangeSpec* other)
{
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    sameRangeSubject(other) || !other->rangeSubjectIsSet(),
                    QRDescriptorException,
                    "Unioning with range on a different col/expr, or range "
                    "subject is not set");

  // Call the superclass version to union the ranges.
  RangeSpec::unionRange(other);
}

// normWA is not used here. It is a parameter because the OptNormRangeSpec
// redefinition of this virtual function needs it.
ItemExpr* OptRangeSpec::getRangeItemExpr(NormWA* normWA) const
{
  QRTRACER("OptRangeSpec::getRangeItemExpr");

  // ItemExpr representing the application of the range condition to the
  // subject column or expression.
  ItemExpr* rangeItemExpr = NULL;

  // If there are no values in the range, the predicate can't be satisfied;
  // return a FALSE itemexpr.
  if (!subranges_.entries() && !nullIncluded_)
    rangeItemExpr = new(mvqrHeap_) BoolVal(ITM_RETURN_FALSE);

  // If all values are covered by the range, the underlying predicate is
  // necessarily true; return a TRUE itemexpr.
  if (coversFullRange())
    rangeItemExpr = new(mvqrHeap_) BoolVal(ITM_RETURN_TRUE);

  // coversFullrange is false but we may have a full range [-inf..+inf] except
  // for NULL values. In other words the predicate 'a > -1 OR a < 1' is converted
  // to 'a IS NOT NULL'
  if ((rangeItemExpr == NULL) && ((subranges_.entries() == 1) && 
	subranges_[0]->coversFullRange()))
    rangeItemExpr = new(mvqrHeap_) UnLogic(ITM_IS_NOT_NULL, getRangeExpr());

  if (rangeItemExpr == NULL)
  {

  // ItemExpr representing the column or expression the range applies to.
    ItemExpr* subjectItemExpr = getRangeExpr();

  // Create a left-deep OR backbone joining the conditions representing the
  // subranges of this range.
  SubrangeBase* subrange = NULL;
  for (CollIndex i=0; i < subranges_.entries(); i++)
    {
      subrange = subranges_[i];
      if (!rangeItemExpr)
        rangeItemExpr = makeSubrangeItemExpr(subrange, subjectItemExpr);
      else
        rangeItemExpr = 
            new(mvqrHeap_) BiLogic(ITM_OR, rangeItemExpr,
                                   makeSubrangeItemExpr(subrange,
                                                        subjectItemExpr));
    }

  // If the range permits the null value, OR in an isNull predicate.
  if (nullIncluded_)
    {
      ItemExpr* isNullItemExpr = new(mvqrHeap_) UnLogic(ITM_IS_NULL,
                                                        subjectItemExpr);
      if (!rangeItemExpr)
        rangeItemExpr = isNullItemExpr;
      else
        rangeItemExpr = new(mvqrHeap_) BiLogic(ITM_OR,
                                               rangeItemExpr,
                                               isNullItemExpr);
    }
  }

  rangeItemExpr->synthTypeAndValueId();
  rangeItemExpr->setRangespecItemExpr(TRUE);
  return rangeItemExpr;
} // OptRangeSpec::getRangeItemExpr()

ItemExpr* OptNormRangeSpec::getRangeItemExpr(NormWA* normWA) const
{
  QRTRACER("OptNormRangeSpec::getRangeItemExpr");

  // Call the superclass version to do the primary work of producing an
  // ItemExpr from the range specification.
  ItemExpr* rangeItemExpr = OptRangeSpec::getRangeItemExpr();

  // Copy LIKE pred info from original expr.
  OperatorTypeEnum op = rangeItemExpr->getOperatorType();
  if ((op == ITM_AND)||(op == ITM_OR))
  {
    op = rangeItemExpr->child(0)->getOperatorType();
    if ( (op == ITM_GREATER_EQ) ||(op == ITM_GREATER) ||
         (op == ITM_LESS) ||(op == ITM_LESS_EQ))
    {
      BiRelat *br = (BiRelat *) rangeItemExpr->child(0).getPtr();
      OperatorTypeEnum op1 = getOriginalItemExpr()->child(0)->getOperatorType();
      if( (op1 == ITM_GREATER_EQ) || (op1 == ITM_GREATER) ||
          (op1 == ITM_LESS) ||(op1 == ITM_LESS_EQ))
      {
        BiRelat *br1 = (BiRelat *) getOriginalItemExpr()->child(0).getPtr();
	br->setAddedForLikePred(br1->addedForLikePred());
        br->setOriginalLikeExprId(br1->originalLikeExprId());
        br->setLikeSelectivity(br1->getLikeSelectivity());  
        if(br1->isSelectivitySetUsingHint())
        {
          br->setSelectivitySetUsingHint();
          br->setSelectivityFactor(br1->getSelectivityFactor());
        }
      }
    }
    op = rangeItemExpr->child(1)->getOperatorType();
    if ( (op == ITM_GREATER_EQ) ||(op == ITM_GREATER) ||
         (op == ITM_LESS) ||(op == ITM_LESS_EQ))
    {
      BiRelat *br = (BiRelat *) rangeItemExpr->child(1).getPtr();
      OperatorTypeEnum op1 = getOriginalItemExpr()->child(1)->getOperatorType();
      if( (op1 == ITM_GREATER_EQ) || (op1 == ITM_GREATER) ||
          (op1 == ITM_LESS) ||(op1 == ITM_LESS_EQ))
      {
        BiRelat *br1 = (BiRelat *) getOriginalItemExpr()->child(1).getPtr();
	br->setAddedForLikePred(br1->addedForLikePred());
        br->setOriginalLikeExprId(br1->originalLikeExprId());
        br->setLikeSelectivity(br1->getLikeSelectivity()); 
        if(br1->isSelectivitySetUsingHint())
        {
          br->setSelectivitySetUsingHint();
          br->setSelectivityFactor(br1->getSelectivityFactor());
        }
      }
    }
  }
  else if ( (op == ITM_GREATER_EQ) ||(op == ITM_GREATER) ||
	(op == ITM_LESS) ||(op == ITM_LESS_EQ))
    {
      BiRelat *br = (BiRelat *) rangeItemExpr;
      OperatorTypeEnum op1 = getOriginalItemExpr()->getOperatorType();
      if( (op1 == ITM_GREATER_EQ) || (op1 == ITM_GREATER) ||
          (op1 == ITM_LESS) ||(op1 == ITM_LESS_EQ))
      {
        BiRelat *br1 = (BiRelat *) getOriginalItemExpr();
	br->setAddedForLikePred(br1->addedForLikePred());
        br->setOriginalLikeExprId(br1->originalLikeExprId());
        br->setLikeSelectivity(br1->getLikeSelectivity());  
        if(br1->isSelectivitySetUsingHint())
        {
          br->setSelectivitySetUsingHint();
          br->setSelectivityFactor(br1->getSelectivityFactor());
        }
      }
    }

  if(((ItemExpr *)getOriginalItemExpr())->isSelectivitySetUsingHint())
  {
    rangeItemExpr->setSelectivitySetUsingHint();
    rangeItemExpr->setSelectivityFactor(getOriginalItemExpr()->getSelectivityFactor());
  }

  // Now associate the range spec's itemexpr with the value id of the
  // original expression.
  ValueId vid = getOriginalItemExpr()->getValueId();
  vid.replaceItemExpr(rangeItemExpr);
  if (normWA)
    rangeItemExpr->normalizeNode(*normWA);
  return rangeItemExpr;
}

void OptNormRangeSpec::intersectRange(OptNormRangeSpec* other)
{
  // Build the item expression tree corresponding to the combined range.
  ItemExpr* otherItemExpr = const_cast<ItemExpr*>(other->getOriginalItemExpr());
  if (originalItemExpr_ && otherItemExpr)
    {
      originalItemExpr_ = new(mvqrHeap_) BiLogic(ITM_AND,
                                                 originalItemExpr_,
                                                 otherItemExpr);
      originalItemExpr_->synthTypeAndValueId(TRUE);
    }
  else
    {
      if (!originalItemExpr_)
      {
        QRLogger::log(CAT_SQL_COMP_RANGE, logLevel_,
          "OptNormRangeSpec::intersectRange -- originalItemExpr_ is NULL.");
      }
      if (!otherItemExpr)
      {
        QRLogger::log(CAT_SQL_COMP_RANGE, logLevel_,
          "OptNormRangeSpec::intersectRange -- other op has NULL ItemExpr.");
      }
    }

  // Now call the superclass version to intersect the ranges.
  OptRangeSpec::intersectRange(other);
}

void OptNormRangeSpec::unionRange(OptNormRangeSpec* other)
{
  // Build the item expression tree corresponding to the combined range.
  ItemExpr* otherItemExpr = const_cast<ItemExpr*>(other->getOriginalItemExpr());
  if (originalItemExpr_ && otherItemExpr)
    {
      originalItemExpr_ = new(mvqrHeap_) BiLogic(ITM_OR,
                                                 originalItemExpr_,
                                                 otherItemExpr);
      originalItemExpr_->synthTypeAndValueId(TRUE);
    }
  else
    {
      if (!originalItemExpr_)
      {
        QRLogger::log(CAT_SQL_COMP_RANGE, logLevel_,
          "OptNormRangeSpec::unionRange -- originalItemExpr_ is NULL.");
      }
      if (!otherItemExpr)
      {
        QRLogger::log(CAT_SQL_COMP_RANGE, logLevel_,
          "OptNormRangeSpec::unionRange -- other op has NULL ItemExpr.");
      }
    }

  // Now call the superclass version to union the ranges.
  OptRangeSpec::unionRange(other);
}

// Convert a date, time, or timestamp value to an integral form for use with
// range specifications.
static Int64 getInt64ValueFromDateTime(ConstValue* val,
                                       const DatetimeType* rangeColType,
                                       const DatetimeType* constType,
                                       NABoolean& truncated,
                                       logLevel level)
{
  char dateRep[11];
  Int64 i64val;
  char* valPtr = (char*)val->getConstValue() +
                        val->getType()->getSQLnullHdrSize();
  Lng32 rangeColFracSecPrec = rangeColType->getFractionPrecision();
  Lng32 constValueFracSecPrec = constType->getFractionPrecision();
  Lng32 fracSec;

  truncated = FALSE;

  switch (constType->getSubtype())
    {
      case DatetimeType::SUBTYPE_SQLDate:
        memcpy(dateRep, valPtr, 4);
        memset(dateRep+4, 0, 7);
        i64val = DatetimeType::julianTimestampValue(dateRep, 11, 0)
                      / SubrangeBase::MICROSECONDS_IN_DAY;
        break;

      case DatetimeType::SUBTYPE_SQLTime:
        i64val = *valPtr * 3600 + *(valPtr+1) * 60 + *(valPtr+2);
        i64val *= 1000000;  // in microseconds
        if (constValueFracSecPrec)
          {
            memcpy(&fracSec, valPtr+3, 4);
            i64val += fracSec * (Int64)pow(10, 6-constValueFracSecPrec);
          }
        break;

      case DatetimeType::SUBTYPE_SQLTimestamp:
        i64val = DatetimeType::julianTimestampValue(valPtr,
                                                    constValueFracSecPrec ? 11 : 7,
                                                    constValueFracSecPrec);
        break;

      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                           FALSE, QRDescriptorException,
                           "Invalid datetime subtype -- %d", constType->getSubtype());
    }

  // For time or timestamp (if date, both compared values will be 0), truncate
  // any extra (wrt the col type) fractional precision from the value and mark
  // it as truncated. The caller will use this to determine if the comparison
  // operator with the truncated value needs to be changed (e.g., t<time'12:00:00.4'
  // <--> t<=time'12:00:00', if t is time(0).
  if (constValueFracSecPrec > rangeColFracSecPrec)
    {
      Int64 scaleFactor = (Int64)pow(10, 6-rangeColFracSecPrec);
      Int64 unscaledI64Val = i64val;
      i64val = i64val / scaleFactor * scaleFactor;
      truncated = (i64val != unscaledI64Val);
    }

  return i64val;
}

// Return an interval value as an Int64 for use with range specifications.
// An interval value of either type (year-month or day-time) will be calculated
// in terms of the least significant field of that type, regardless of the
// actual fields used in the declaration. For example, an interval specified as
// HOUR TO MINUTE will return a value which is a number of microseconds. This
// facilitates comparing intervals of different granularity.
//
// A day-time interval that ends in seconds may specify a fractional precision
// less than the default (e.g., Interval '5.1' Second(2,2)), in which case its
// raw value is in units of a lower precision than microseconds (hundredths of
// a second in the example). We scale such values to microseconds so that
// everything is comparable. In the example above, the value used would be
// 5100000 instead of the original internal integral value of 510.
static Int64 getInt64ValueFromInterval(ConstValue* constVal,
                                       const IntervalType* colIntvlType,
                                       NABoolean& truncated,
                                       NABoolean& valWasNegative,
                                       logLevel level)
{
  const IntervalType* constIntvlType = 
          static_cast<const IntervalType*>(constVal->getType());
  Int64 i64val;
  char* val = (char*)constVal->getConstValue();
  Lng32 storageSize = constVal->getStorageSize();
  Lng32 nullHdrSize = constVal->getType()->getSQLnullHdrSize();
  val += nullHdrSize;
  storageSize -= nullHdrSize;
  switch (storageSize)
    {
      // No need to check for signed/unsigned before checking for negative;
      // interval type is always signed.
      case 2:
        {
          Int16 valx;
          memcpy(&valx, val, sizeof(Int16));
          valWasNegative = (valx < 0);
          i64val = valx;
        }
        break;
      case 4:
        {
          Lng32 valx;
          memcpy(&valx, val, sizeof(Lng32));
          valWasNegative = (valx < 0);
          i64val = valx;
        }
        break;
      case 8:
        memcpy(&i64val, val, sizeof(i64val));
        valWasNegative = (i64val < 0);
        break;
      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                           FALSE, QRDescriptorException,
                           "Invalid interval storage length -- %d",
                           storageSize);
        break;
    }

  // Now cast the value in terms of the least significant field for the constant's
  // interval type (months for year-month, microseconds for day-time), which may
  // have different fields than those of the column's type.
  switch (constIntvlType->getEndField())
    {
      case REC_DATE_YEAR:
        // Multiply by number of months in a year
        i64val *= 12;
        break;
      case REC_DATE_MONTH:
        // Value is already in correct units (months).
        break;
      case REC_DATE_DAY:
        // Multiply by number of microseconds in a day.
        i64val *= (24LL * 60 * 60000000);
        break;
      case REC_DATE_HOUR:
        // Multiply by number of microseconds in an hour.
        i64val *= (60LL * 60000000);
        break;
      case REC_DATE_MINUTE:
        // Multiply by number of microseconds in a minute.
        i64val *= 60000000;
        break;
      case REC_DATE_SECOND:
        // We want all intervals with seconds as the end field to be represented
        // as a number of microseconds to ensure comparability, so if the
        // fractional precision is less than the default (6), scale it to
        // microseconds.
        i64val *= (Int64)pow(10, 6 - constIntvlType->getFractionPrecision());
        break;
      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                           FALSE, QRDescriptorException,
                           "Invalid end field for interval -- %d",
                           constIntvlType->getEndField());
        break;
    }

  // Now discard any part of the constant value owing to extra precision in the
  // constant's type. For instance, if the column type is INTERVAL YEAR and the
  // constant is "interval'2-7'year to month", the constant is truncated to 2
  // years. We remember that this truncation occurred, as it may affect the
  // comparison operator used in the rangespec.
  Int64 scaleFactor;
  Int64 origI64Val = i64val;
  switch (colIntvlType->getEndField())
    {
      // Do integer division to scrape off any excess precision in the
      // constant value, then multipy by same to restore it as a number of
      // months (year-month interval) or microseconds (day-time).
      case REC_DATE_YEAR:
        // Divide/multiply by number of months in a year
        i64val = i64val / 12 * 12;
        break;
      case REC_DATE_MONTH:
        // No possibility of extra precision for the constant value.
        break;
      case REC_DATE_DAY:
        // Divide/multiply by number of microseconds in a day.
        i64val = i64val / (24LL * 60 * 60000000) * (24LL * 60 * 60000000);
        break;
      case REC_DATE_HOUR:
        // Divide/multiply by number of microseconds in an hour.
        i64val = i64val / (60LL * 60000000) * (60LL * 60000000);
        break;
      case REC_DATE_MINUTE:
        // Divide/multiply by number of microseconds in a minute.
        i64val = i64val / 60000000 * 60000000;
        break;
      case REC_DATE_SECOND:
        // Divide/multiply by 10 ** <unused frac secs>.
        scaleFactor = (Int64)pow(10, 6 - colIntvlType->getFractionPrecision());
        i64val = i64val / scaleFactor * scaleFactor;
        break;
      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                          FALSE, QRDescriptorException,
                          "Invalid end field for interval -- %d",
                          colIntvlType->getEndField());
        break;
    }

  truncated = (origI64Val != i64val);
  return i64val;
}

Int64 getInt64Value(ConstValue* val, const NAType* rangeColType, 
                    NABoolean& truncated, NABoolean& valWasNegative,
                    logLevel level)
{
  truncated = FALSE;
  valWasNegative = FALSE;

  // If the constant value is a date, time, timestamp, or interval, pass it on
  // to the correct function for handling that type.
  const NAType* constValType = val->getType();
  if (constValType->getTypeQualifier() == NA_DATETIME_TYPE)
    return getInt64ValueFromDateTime(val,
                                     static_cast<const DatetimeType*>(rangeColType),
                                     static_cast<const DatetimeType*>(constValType),
                                     truncated, level);
  else if (constValType->getTypeQualifier() == NA_INTERVAL_TYPE)
    return getInt64ValueFromInterval(val,
                                     static_cast<const IntervalType*>(rangeColType),
                                     truncated, valWasNegative, level);

  assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                     constValType->getTypeQualifier() == NA_NUMERIC_TYPE,
                     QRDescriptorException,
                     "Unexpected date type -- %d",
                     constValType->getTypeQualifier());
  const NumericType* constValNumType = static_cast<const NumericType*>(constValType);
  Lng32 rangeColumnScale = rangeColType->getScale();
  NABoolean isExactNumeric = val->isExactNumeric();
  Lng32 constValueScale = (isExactNumeric ? constValNumType->getScale() : 0);
  char* valuePtr = (char*)val->getConstValue();
  Lng32 storageSize = val->getStorageSize();
  Lng32 nullHdrSize = constValNumType->getSQLnullHdrSize();
  valuePtr += nullHdrSize;
  storageSize -= nullHdrSize;
  
  // Scale factor for exact numeric constant values.
  Int64 scaleFactor = 1;
  if (constValueScale < rangeColumnScale)
    scaleFactor = (Int64)pow(10, rangeColumnScale - constValueScale);

  // Scale factor for approximate (floating-point) constant values.
  double dblScaleFactor = pow(10, rangeColumnScale);
  Int64 i64val = 0;
  Float64 flt64val;

  if (constValNumType->isDecimal())
    {
      // Decode the decimal value from the string of digits. The digit values
      // are the lower nibble of each byte. As we go from left to right,
      // multiple the accumulated value by 10, and add the value of the next
      // digit. The value is negative if the upper nibble of the first byte is
      // 11 (0xB0 after masking off lower nibble).
      char* p = valuePtr;
      for (Lng32 i=0; i<storageSize; i++)
        (i64val *= 10) += (*p++ & 0x0F);
      if ((*valuePtr & 0xF0) == 0xB0)
        {
          i64val *= -1;
          valWasNegative = TRUE;
        }
    }
  else
    switch (storageSize)
      {
        case 1:
          {
            assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                            isExactNumeric, QRDescriptorException,
                            "Constant value of size 1 not exact numeric, type is: %d",
                            constValNumType->getTypeQualifier());
            Int8 i8val;
            memcpy(&i8val, valuePtr, storageSize);
            if (constValNumType->isSigned())
              {
                valWasNegative = (i8val < 0);
                i64val = (Int64)(i8val * scaleFactor);
              }
            else
              i64val = (Int64)((UInt8)i8val * scaleFactor);
          }
          break;

       case 2:
         {
            assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                            isExactNumeric, QRDescriptorException,
                            "Constant value of size 2 not exact numeric, type is: %d",
                            constValNumType->getTypeQualifier());
            Int16 i16val;
            memcpy(&i16val, valuePtr, storageSize);
            if (constValNumType->isSigned())
              {
                valWasNegative = (i16val < 0);
                i64val = (Int64)(i16val * scaleFactor);
              }
            else
              i64val = (Int64)((UInt16)i16val * scaleFactor);
          }
          break;

        case 4:
          if (isExactNumeric)
            {
              Lng32 i32val;
              memcpy(&i32val, valuePtr, storageSize);
              if (constValNumType->isSigned())
                {
                  valWasNegative = (i32val < 0);
                  i64val = (Int64)(i32val * scaleFactor);
                }
              else
                i64val = (Int64)((UInt32)i32val * scaleFactor);
            }
          else
            {
              Float32 flt32val;
              memcpy(&flt32val, valuePtr, storageSize);
              valWasNegative = (flt32val < 0);
              flt64val = flt32val * dblScaleFactor;
              if (flt64val < LLONG_MIN)
                i64val = LLONG_MIN;
              else if (flt64val > LLONG_MAX)
                i64val = LLONG_MAX;
              else
                i64val = (Int64)flt64val;
            }
          break;
        
        case 8:
          if (isExactNumeric)
            {
              memcpy(&i64val, valuePtr, storageSize);
              valWasNegative = (i64val < 0);
              i64val *= scaleFactor;
            }
          else
            {
              memcpy(&flt64val, valuePtr, storageSize);
              valWasNegative = (flt64val < 0);
              flt64val *= dblScaleFactor;
              if (flt64val < LLONG_MIN)
                i64val = LLONG_MIN;
              else if (flt64val > LLONG_MAX)
                i64val = LLONG_MAX;
              else
                i64val = (Int64)flt64val;
            }
          break;
        
        default:
          assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                            FALSE, QRDescriptorException,
                            "Numeric constant of unexpected size: %d bytes",
                            storageSize);
          return 0;      // avoid warning
      }

  // If the constant has more digits to the right of the decimal point than the
  // range column, note it as truncated if any of the excess digits is nonzero.
  // The caller will use this to determine if the comparison operator with the
  // truncated value needs to be changed (e.g., i>4.3 <--> i>=4, if scale of i
  // is 0).
  if (isExactNumeric)
    {
      if (constValueScale > rangeColumnScale)
        {
          scaleFactor = (Int64)pow(10, constValueScale - rangeColumnScale);
          truncated = (i64val / scaleFactor * scaleFactor != i64val);
          return i64val / scaleFactor;
        }
      else
        return i64val;
    }
  else
    {
      truncated = (i64val != flt64val);
      return i64val;
    }
}

double getDoubleValue(ConstValue* val, logLevel level)
{
  // If the constant is a SystemLiteral (typically produced by the cast of
  // constant value), the value pointer and storage size will include a null
  // indicator header field that we have to take into account when accessing
  // the value. It also means the actual value field may not be aligned
  // properly, so we need to memcpy the value representation into a variable
  // of the appropriate type.
  const NAType* constValType = val->getType();
  Lng32 nullHdrSize = constValType->getSQLnullHdrSize();
  Lng32 valueStorageSize = val->getStorageSize() - nullHdrSize;
  void* valuePtr = (char*)val->getConstValue() + nullHdrSize;

  NABoolean isExactNumeric = val->isExactNumeric();
  double scaleDivisor = pow(10, isExactNumeric ? constValType->getScale() : 0);

  if (static_cast<const NumericType*>(constValType)->isDecimal())
    {
      // Decode the decimal value from the string of digits. The digit values
      // are the lower nibble of each byte. As we go from left to right,
      // multiple the accumulated value by 10, and add the value of the next
      // digit. The value is negative if the upper nibble of the first byte is
      // 11 (0xB0 after masking off lower nibble).
      char* p = (char*)valuePtr;
      Int64 i64val = 0;
      for (Lng32 i=0; i<valueStorageSize; i++)
        (i64val *= 10) += (*p++ & 0x0F);
      if ((*(char*)valuePtr & 0xF0) == 0xB0)
        i64val *= -1;
      return i64val / scaleDivisor;
    }

  switch (valueStorageSize)
    {
      case 1: // tinyint
        {
          assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                           isExactNumeric, QRDescriptorException,
                           "const value of size 1 not exact numeric: %d",
                           constValType->getTypeQualifier());
          Int8 i8val;
          memcpy(&i8val, valuePtr, valueStorageSize);
          return i8val / scaleDivisor;
        }

      case 2: // smallint
        {
          assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                           isExactNumeric, QRDescriptorException,
                           "const value of size 2 not exact numeric: %d",
                           constValType->getTypeQualifier());
          Int16 i16val;
          memcpy(&i16val, valuePtr, valueStorageSize);
          return i16val / scaleDivisor;
        }

      case 4: //  int
        if (isExactNumeric)
          {
            Lng32 i32val;
            memcpy(&i32val, valuePtr, valueStorageSize);
            return i32val / scaleDivisor;
          }
        else
          {
            Float32 fltval;
            memcpy(&fltval, valuePtr, valueStorageSize);
            return fltval;
          }
      
      case 8: // largeint
        if (isExactNumeric)
          {
            // possible loss of data
            Int64 i64val;
            memcpy(&i64val, valuePtr, valueStorageSize);
            return (double)(i64val / scaleDivisor);
          }
        else
          {
            Float64 dblval;
            memcpy(&dblval, valuePtr, valueStorageSize);
            return dblval;
          }
      
      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                           FALSE, QRDescriptorException,
                           "Numeric constant of unexpected size: %d bytes",
                           val->getStorageSize());
        return 0;      // avoid warning
    }
}

NABoolean RangeInfo::operator==(const RangeInfo& other)
{
  if (rangeSpec_->getRangeJoinPredId() != NULL_VALUE_ID)
    return rangeSpec_->getRangeJoinPredId() ==
              other.rangeSpec_->getRangeJoinPredId();
  else if (rangeSpec_->getRangeColValueId() != NULL_VALUE_ID)
    return rangeSpec_->getRangeColValueId() ==
              other.rangeSpec_->getRangeColValueId();
  else
    return rangeSpec_->getRangeExprText()==other.rangeSpec_->getRangeExprText();
}
