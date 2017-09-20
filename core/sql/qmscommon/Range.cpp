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

#include <sstream>
#include "Range.h"
#include "nawstring.h"
#include "NumericType.h"
#include "DatetimeType.h"
#include "QRLogger.h"

//
// RangeSpec
//

const Int64 SubrangeBase::MICROSECONDS_IN_DAY = 86400000000LL;
const Int64 SubrangeBase::SUBRANGE_DATE_MIN = 148731163200000000LL / MICROSECONDS_IN_DAY;
const Int64 SubrangeBase::SUBRANGE_DATE_MAX = 274958884800000000LL / MICROSECONDS_IN_DAY;
const Int64 SubrangeBase::SUBRANGE_TIMESTAMP_MIN = 148731163200000000LL;
const Int64 SubrangeBase::SUBRANGE_TIMESTAMP_MAX = 274958971199999999LL;
const Int64 SubrangeBase::SUBRANGE_TIME_MIN = 0;
const Int64 SubrangeBase::SUBRANGE_TIME_MAX = 86399999999LL;

RangeSpec::RangeSpec(QRRangePredPtr rangePred, CollHeap* heap, logLevel ll)
  : rangeColValueId_(QR_NULL_VALUE_ID),
    rangeJoinPredId_(QR_NULL_VALUE_ID),
    rangeExprText_(heap),
    mvqrHeap_(heap),
    nullIncluded_(FALSE),
    type_(NULL),
    isDumpMvMode_(FALSE),
    logLevel_(ll),
    subranges_(heap)
{
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    rangePred, QRLogicException,
                    "RangeSpec constructed for null QRRangePredPtr");

  NumericID idNum, refNum;
  char refChar;
  QRElementPtr rangeItem = rangePred->getRangeItem();
  ElementType elemType = rangeItem->getElementType();
  QRValueId colValueId = QR_NULL_VALUE_ID;

  switch (elemType)
    {
      case ET_Column:
      //case ET_MVColumn:
        idNum = rangeItem->getIDNum();
        if (idNum)
          rangeColValueId_ = colValueId = idNum;
        else
          {
            refNum  = rangeItem->getRefNum();
            refChar = rangeItem->getRefFirstChar();
            if (refChar == 'J')
              rangeJoinPredId_ = colValueId = refNum;
            else
              {
                assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                                  refChar=='C', QRLogicException,
                                  "ref attribute had unexpected element type -- %c");
                rangeColValueId_ = colValueId = refNum;
              }
            assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                               rangeItem->getReferencedElement() != rangeItem,
                               QRLogicException,
                               "ref attribute present, but references self: %s",
                               rangeItem->getID().toCharStar());
          }
        break;

      case ET_Expr:
        rangeExprText_ = ((QRExprPtr)rangeItem)->getExprText();
        break;

      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                           FALSE, QRLogicException,
                           "Invalid element type for range item -- %d",
                           elemType);
        break;
    };

  QRRangeOperatorPtr rangeOp;
  QROpEQPtr opEQ;
  QROpInequalityPtr opNEQ;
  QROpBTPtr opBT;
  QRScalarValuePtr value;
  ElementType rangeOpType;
  const NAPtrList<QRRangeOperatorPtr>& rangeOps = rangePred->getOperatorList();
  for (CollIndex i=0; i<rangeOps.entries(); i++)
    {
      rangeOp = rangeOps[i];
      rangeOpType = rangeOp->getElementType();
      switch (rangeOpType)
        {
          case ET_OpEQ:
            {
              opEQ = static_cast<QROpEQPtr>(rangeOp);
              if (opEQ->includesNull())
                nullIncluded_ = TRUE;
              const NAPtrList<QRScalarValuePtr>& eqValues = opEQ->getValueList();
              for (CollIndex eqInx=0; eqInx<eqValues.entries(); eqInx++)
                {
                  value = eqValues[eqInx];
                  addSubrange(value, value, TRUE, TRUE);
                }
            }
            break;

          case ET_OpLS:
          case ET_OpLE:
          case ET_OpGT:
          case ET_OpGE:
            opNEQ = static_cast<QROpInequalityPtr>(rangeOp);
            value = opNEQ->getValue();
            if (colValueId != QR_NULL_VALUE_ID && opNEQ->isNormalized())
              addDenormalizedSubrange(rangePred->getSqlType().data(),
                                      rangeOpType,
                                      value);
            else
              addSubrange((rangeOpType == ET_OpGT || rangeOpType == ET_OpGE)
                            ? value : NULL,           //start value
                          (rangeOpType == ET_OpLS || rangeOpType == ET_OpLE)
                            ? value : NULL,           // end value
                          rangeOpType == ET_OpGE,     // startInclusive
                          rangeOpType == ET_OpLE);    // endInclusive
            break;

          case ET_OpBT:
            opBT = static_cast<QROpBTPtr>(rangeOp);
            addSubrange(opBT->getStartValue(), opBT->getEndValue(),
                        opBT->startIsIncluded(), opBT->endIsIncluded());
            break;

          default:
            assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                               FALSE, QRLogicException,
                               "Invalid range operator, element type = %d",
                               rangeOpType);
            break;
        }
    }
}

// Protected copy ctor, used by clone().
RangeSpec::RangeSpec(const RangeSpec& other, CollHeap* heap)
  : rangeColValueId_(other.rangeColValueId_),
    rangeJoinPredId_(other.rangeJoinPredId_),
    rangeExprText_(other.rangeExprText_),
    subranges_(other.subranges_),
    mvqrHeap_(heap ? heap : other.mvqrHeap_),
    nullIncluded_(other.nullIncluded_),
    isDumpMvMode_(other.isDumpMvMode_),
    logLevel_(other.logLevel_)
{
  // We do this in the body instead of in initialization so there is no
  // dependency on the declaration order of type_ and mvqrHeap_ in the
  // class definition.
  type_ = other.type_->newCopy(mvqrHeap_);

  // The NACollection copy ctor uses pairwise assignment of list items from the
  // old to the newly-constructed collection. Since subranges_ is a list of
  // pointers, the pointers will be duplicated in the two lists, so we need to
  // clone the objects they point to for the new list.
  for (CollIndex i=0; i<subranges_.entries(); i++)
    subranges_[i] = subranges_[i]->clone(mvqrHeap_);
}

RangeSpec::~RangeSpec()
{
  for (CollIndex i=0; i<subranges_.entries(); i++)
    delete subranges_[i];

  delete type_;
}

NABoolean RangeSpec::operator==(const RangeSpec& other) const
{
  QRTRACER("RangeSpec::operator==");

  // Types must be the same. operator!= is not defined for NAType, so negate
  // result of ==.
  if (!typeCompatible(&other))
    return FALSE;

  // Subranges are normalized, so equivalent RangeSpecs will be identical.
  if (subranges_.entries() != other.subranges_.entries())
    return FALSE;

  // Null value must be included in both, or in neither.
  if (nullIncluded_ != other.nullIncluded_)
    return FALSE;

  for (CollIndex i=0; i<subranges_.entries(); i++)
    {
      if (*subranges_[i] != *other.subranges_[i])
        return FALSE;
    }

  return TRUE;  // everything matched
}

NABoolean RangeSpec::subsumes(const RangeSpec* other) const
{
  QRTRACER("subsumes");
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    typeCompatible(other), QRLogicException,
                    "RangeSpec::subsumes() called with RangeSpec of incompatible type");

  if (!nullIncluded_ && other->nullIncluded_)
    return FALSE;

  // We shouldn't see rangespecs that evaluate to TRUE or FALSE here; range not
  // generated for a tautology, and an absurdity is turned into a residual pred.
  // Only allowable case for a range with no subranges is an IS NOT NULL pred.
  if (subranges_.entries() == 0)
    {
      if (nullIncluded_)
        // IS NULL is only condition on subsumer; other range is only subsumed
        // (and is in fact equal) if null is included and there are no subranges.
        return (other->nullIncluded_ && other->subranges_.entries() == 0);
      else
        // IS NOT NULL is only condition on subsumer; other range is subsumed as
        // long as it isn't null.
        return !other->nullIncluded_;           
    }

  if (other->subranges_.entries() == 0)
    {
      // Other range has no subranges, it either has only an IS NULL, or is empty,
      // signifying a predicate of IS NOT NULL.
      if (other->nullIncluded_)
        return nullIncluded_;
      else
        // Other rangespec is empty, meaning IS NOT NULL. We know this is not true
        // of the subsumer, or it would have been handled and returned above.
        return FALSE;
    }

  CollIndex searchStartIndex = 0;
  NABoolean found;
  for (CollIndex otherInx=0; otherInx < other->subranges_.entries(); otherInx++)
    {
      found = FALSE;
      for (CollIndex thisInx=searchStartIndex;
           thisInx < subranges_.entries() && !found;
           thisInx++)
        {
          if (other->subranges_[otherInx]->startsWithin(subranges_[thisInx]))
            {
              if (other->subranges_[otherInx]->endsWithin(subranges_[thisInx]))
                {
                  found = TRUE;
                  searchStartIndex = thisInx; // search for next one starting here
                }
              else
                // Starts in one subrange and ends outside it; since the
                // subranges of a given range are guaranteed to be noncontiguous
                // by the normalization process, it must have values that this
                // range doesn't.
                return FALSE;
            }
          else if (otherInx == 0 &&  // this can only happen on the 1st one
                   other->subranges_[otherInx]->startsBefore(subranges_[thisInx]))
            return FALSE;
        }

      // If there is any subrange of the passed range that is not contained in
      // any subrange of this range, the subsumption test fails.
      if (!found)
        return FALSE;
    }

  return TRUE;  // all subranges of other are contained in this range
}

template <class T>
void RangeSpec::placeSubrange(Subrange<T>* sub)
{
  QRTRACER("placeSubrange");
  Lng32 specValCount1, specValCount2;
  CollIndex i, j, insertionPoint = UINT_MAX;
  CollIndex numEntries = subranges_.entries();

  sub->initSpecifiedValueCount();

  // Make i the index of the first subrange that does not start before the start
  // of the new one.
  for (i=0;
       i<numEntries && subranges_[i]->startsBefore(sub);
       i++)
    ;

  if (i==0 || sub->startsAfter(subranges_[i-1]))
    {
      // Start of sub is before start of first existing subrange, or between two
      // existing subranges.
      if (i==numEntries || sub->endsBefore(subranges_[i]))
        {
          subranges_.insertAt(i, sub);  // New subrange overlaps no existing one.
          insertionPoint = i;
        }
      else
        {
          // Sub overlaps one or more succeeding subranges.
          for (j = i; j<numEntries && sub->endsAfter(subranges_[j]); numEntries--)
            {
              delete subranges_[j];
              subranges_.removeAt(j);
            }
          if (j == numEntries || sub->endsBefore(subranges_[j]))
            {
              subranges_.insertAt(j, sub);
              insertionPoint = j;
            }
          else // sub ends within subranges_[j]
            {
	      specValCount1 = sub->getSpecifiedValueCount();
              specValCount2 = subranges_[j]->getSpecifiedValueCount();
              if (specValCount1 && specValCount2)
                sub->setSpecifiedValueCount(specValCount1 + specValCount2);
              else
                sub->setSpecifiedValueCount(0);
              sub->extendEnd(subranges_[j]);
              delete subranges_[j];
              subranges_.removeAt(j);
              subranges_.insertAt(j, sub);
              insertionPoint = j;
            }
        }
    }
  else
    {
      // The new sub starts within the preceding subrange. Extend it to cover all
      // overlapped subranges, and remove those subranges before inserting the
      // new subrange.
      specValCount1 = sub->getSpecifiedValueCount();
      specValCount2 = subranges_[i-1]->getSpecifiedValueCount();
      if (specValCount1 && specValCount2)
        sub->setSpecifiedValueCount(specValCount1 + specValCount2);
      else
        sub->setSpecifiedValueCount(0);
      sub->extendStart(subranges_[i-1]);
      for (j=i-1; j<numEntries && sub->endsAfter(subranges_[j]); numEntries--)
        {
          delete subranges_[j];
          subranges_.removeAt(j);
        }
      if (j<numEntries && sub->endsWithin(subranges_[j]))
        {
	  specValCount1 = sub->getSpecifiedValueCount();  // may have changed above
          specValCount2 = subranges_[j]->getSpecifiedValueCount();
          if (specValCount1 && specValCount2)
            sub->setSpecifiedValueCount(specValCount1 + specValCount2);
          else
            sub->setSpecifiedValueCount(0);
          sub->extendEnd(subranges_[j]);
          delete subranges_[j];
          subranges_.removeAt(j);
        }
      subranges_.insertAt(j, sub);
      insertionPoint = j;
    }

  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    insertionPoint < UINT_MAX, QRLogicException,
                    "Forgot to assign insertionPoint");

  // If the newly-inserted subrange abuts its successor, extend it to cover the
  // successor, which is then removed.
  if (insertionPoint < subranges_.entries() - 1 &&
      subranges_[insertionPoint]->adjacentTo(subranges_[insertionPoint+1]))
    {
      // If both subranges are derived from IN lists or disjunctions of
      // equality preds, mark the result as being so, and combine the count.
      specValCount1 = subranges_[insertionPoint]->getSpecifiedValueCount();
      specValCount2 = subranges_[insertionPoint+1]->getSpecifiedValueCount();
      if (specValCount1 && specValCount2) // both must be nonzero
        subranges_[insertionPoint]->setSpecifiedValueCount(specValCount1 +
                                                          specValCount2);
      else
        subranges_[insertionPoint]->setSpecifiedValueCount(0);

      subranges_[insertionPoint]->extendEnd(subranges_[insertionPoint+1]);
      delete subranges_[insertionPoint+1];
      subranges_.removeAt(insertionPoint+1);
    }

  // If the new subrange (which may have absorbed its successor) abuts its
  // predecessor, extend the predecessor to cover the new subrange, which is
  // then removed.
  if (insertionPoint > 0 &&
      subranges_[insertionPoint-1]->adjacentTo(subranges_[insertionPoint]))
    {
      // If both subranges are derived from IN lists or disjunctions of
      // equality preds, mark the result as being so, and combine the count.
      specValCount1 = subranges_[insertionPoint-1]->getSpecifiedValueCount();
      specValCount2 = subranges_[insertionPoint]->getSpecifiedValueCount();
      if (specValCount1 && specValCount2) // both must be nonzero
        subranges_[insertionPoint-1]->setSpecifiedValueCount(specValCount1 +
                                                            specValCount2);
      else
        subranges_[insertionPoint-1]->setSpecifiedValueCount(0);

      subranges_[insertionPoint-1]->extendEnd(subranges_[insertionPoint]);
      delete subranges_[insertionPoint];
      subranges_.removeAt(insertionPoint);
    }
}  // placeSubrange()

void RangeSpec::addSubrange(QRScalarValuePtr startVal, QRScalarValuePtr endVal,
                            NABoolean startInclusive,  NABoolean endInclusive)
{
  ElementType valueElemType = (startVal ? startVal->getElementType()
                                        : endVal->getElementType());
  switch (valueElemType)
    {
      case ET_NumericVal:
        {
          // Fixed-point numeric subranges are normalized to be inclusive, to
          // simplify equivalence and subsumption checks.
          // @ZX -- would fail for x>max or x<min -- should we bother to check?
          Subrange<Int64>* numSubrange = new(mvqrHeap_) Subrange<Int64>(logLevel_);
          if (startVal)
            {
              numSubrange->start = 
                 static_cast<QRNumericValPtr>(startVal)->getUnscaledNumericVal();
              if (!startInclusive)
                numSubrange->start++;  // make inclusive
            }
          else
            numSubrange->setStartIsMin(TRUE);
          if (endVal)
            {
              numSubrange->end = 
                  static_cast<QRNumericValPtr>(endVal)->getUnscaledNumericVal();
              if (!endInclusive)
                numSubrange->end--;    // make inclusive
            }
          else
            numSubrange->setEndIsMax(TRUE);

          // If not originally inclusive, has been adjusted above.
          numSubrange->setStartInclusive(TRUE);
          numSubrange->setEndInclusive(TRUE);
          placeSubrange(numSubrange);
        }
        break;

      case ET_StringVal:
        {
          Subrange<RangeString>* charSubrange
                  = new(mvqrHeap_) Subrange<RangeString>(logLevel_);
          if (startVal)
            charSubrange->start=static_cast<QRStringValPtr>(startVal)->getValue();
          else
            charSubrange->setStartIsMin(TRUE);
          if (endVal)
            charSubrange->end = static_cast<QRStringValPtr>(endVal)->getValue();
          else
            charSubrange->setEndIsMax(TRUE);
          charSubrange->setStartInclusive(startInclusive);
          charSubrange->setEndInclusive(endInclusive);
          placeSubrange(charSubrange);
        }
        break;

      case ET_WStringVal:
        {
          Subrange<RangeWString>* charSubrange
                  = new(mvqrHeap_) Subrange<RangeWString>(logLevel_);
          if (startVal)
            charSubrange->start=static_cast<QRWStringValPtr>(startVal)->getWideValue();
          else
            charSubrange->setStartIsMin(TRUE);
          if (endVal)
            charSubrange->end = static_cast<QRWStringValPtr>(endVal)->getWideValue();
          else
            charSubrange->setEndIsMax(TRUE);
          charSubrange->setStartInclusive(startInclusive);
          charSubrange->setEndInclusive(endInclusive);
          placeSubrange(charSubrange);
        }
        break;

      case ET_FloatVal:
        {
          Subrange<double>* floatSubrange = new(mvqrHeap_) Subrange<double>(logLevel_);
          if (startVal)
            floatSubrange->start = 
                static_cast<QRFloatValPtr>(startVal)->getFloatVal();
          else
            floatSubrange->setStartIsMin(TRUE);
          if (endVal)
            floatSubrange->end = 
                static_cast<QRFloatValPtr>(endVal)->getFloatVal();
          else
            floatSubrange->setEndIsMax(TRUE);
          floatSubrange->setStartInclusive(startInclusive);
          floatSubrange->setEndInclusive(endInclusive);
          placeSubrange(floatSubrange);
        }
        break;

      default:
        {
          // Need this to avoid link errors.
          Subrange<RangeWString>* charSubrange =
                        new(mvqrHeap_) Subrange<RangeWString>(logLevel_);
          assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                            FALSE, QRLogicException,
                            "Unhandled value type, element type = %d",
                            valueElemType);
        }
        break;
    }
}  // addSubrange(QRScalarValuePtr...

// Get the precision and scale from the passed string.
void RangeSpec::getPrecScale(const char* openParen, Lng32& prec, Lng32& scale) const
{
  char* delim = (char *) strpbrk(openParen, ",)");
  if (*delim == ',')
    // both prec and scale present
    sscanf(openParen, "(%d,%d)", &prec, &scale);
  else
    {
      assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                        delim, QRLogicException,
                        "No ',' or ')' found in numeric/decimal type string -- %s",
                        openParen);
      // no scale specified
      sscanf(openParen, "(%d)", &prec);
    }
}

// Returns the enum value corresponding to the given inverval field name.
rec_datetime_field RangeSpec::getIntervalFieldFromName(const char* name, Int32 len) const
{
  switch(len)
    {
      case 4:
        if (!strncmp(name, "YEAR", len))
          return REC_DATE_YEAR;
        else if (!strncmp(name, "HOUR", len))
          return REC_DATE_HOUR;
        break;

      case 6:
        if (!strncmp(name, "MINUTE", len))
          return REC_DATE_MINUTE;
        else if (!strncmp(name, "SECOND", len))
          return REC_DATE_SECOND;
        break;

      case 3:
        if (!strncmp(name, "DAY", len))
          return REC_DATE_DAY;
        break;

      case 5:
        if (!strncmp(name, "MONTH", len))
          return REC_DATE_MONTH;
        break;

      default:
        break;
    }

  // If we found it, we would have returned already.
  assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                     FALSE, QRLogicException, "Unknown interval field -- %s", name);
}

// Parse an Interval type designation and return a pointer to an IntervalType object.
IntervalType* RangeSpec::parseIntervalTypeText(const char* text) const
{
  Lng32 lp = SQLInterval::DEFAULT_LEADING_PRECISION;  // Leading field precision
  Lng32 fp = SQLInterval::DEFAULT_FRACTION_PRECISION; // Fractional seconds precision

  rec_datetime_field leadingField, trailingField;

  // typeText points to blank after INTERVAL keyword; scan blanks up to
  // leading field.
  text += strspn(text, " ");  // skip blanks
  Int32 len = strcspn(text, " (");
  leadingField = getIntervalFieldFromName(text, len);
  text += len;
  if (*text == '(')
    {
      // If both lp and fp are given, the interval must consist of a single
      // field, which is SECOND.
      getPrecScale(text, lp, fp);
      text = strpbrk(text, ")") + 1;
    }

  // Done with leading field. If "TO" is next token, there is a trailing field.
  text += strspn(text, " ");  // skip blanks
  if (!strncmp(text, "TO ", 3))
    {
      text += 3;
      len = strcspn(text, " (");
      trailingField = getIntervalFieldFromName(text, len);
      text += len;
      if (*text == '(')
        // If a precision is given with the trailing field, it must be only
        // the fractional seconds precision for a SECOND field. Pass fp for
        // both precision args, only the first will be set.
        getPrecScale(text, fp, fp);
    }
  else
    trailingField = leadingField;  // Only one field specified.

  // Construct and return the interval type.
  return new(mvqrHeap_) SQLInterval(mvqrHeap_, TRUE, leadingField, lp, trailingField, fp);
}

void RangeSpec::addDenormalizedSubrange(const char* typeText,
                                        ElementType rangeOpType,
                                        QRScalarValuePtr value)
{
  QRTRACER("addDenormalizedSubrange");

  QRLogger::log(CAT_SQL_COMP_RANGE, LL_DEBUG,
    "Type description for normalized range item: %s", typeText);
  NABoolean isSigned = strstr(typeText, "UNSIGNED") == NULL;
  Lng32 prec, scale = 0;  // default scale for numeric types
  NumericType* numType = NULL;
  DatetimeType* dtType = NULL;
  IntervalType* intvlType = NULL;
  const Int16 DisAmbiguate = 0;

  Int32 typeNameLen = strcspn(typeText, " (");

  // Parse the type text and create the corresponding type object.
  if (!strncmp(typeText, "INTEGER", typeNameLen))
    numType = new(mvqrHeap_) SQLInt(mvqrHeap_, isSigned, TRUE/*don't care*/);
  else if (!strncmp(typeText, "SMALLINT", typeNameLen))
    numType = new(mvqrHeap_) SQLSmall(mvqrHeap_, isSigned, TRUE/*don't care*/);
  else if (!strncmp(typeText, "NUMERIC", typeNameLen))
    {
      getPrecScale(typeText + typeNameLen, prec, scale);
      numType = new(mvqrHeap_) SQLNumeric(mvqrHeap_, isSigned, prec, scale,
                                          DisAmbiguate); // added for 64bit proj.
    }
  else if (!strncmp(typeText, "DECIMAL", typeNameLen))
    {
      getPrecScale(typeText + typeNameLen, prec, scale);
      numType = new(mvqrHeap_) SQLNumeric(mvqrHeap_, isSigned, prec, scale,
                                          DisAmbiguate); // added for 64bit proj.
    }
  else if (!strncmp(typeText, "REAL", typeNameLen))
    numType = new(mvqrHeap_) SQLReal(mvqrHeap_, TRUE/*don't care*/);
  else if (!strncmp(typeText, "DATE", typeNameLen))
    dtType = new(mvqrHeap_) SQLDate(mvqrHeap_, TRUE/*don't care*/);
  else if (!strncmp(typeText, "TIME", typeNameLen))
    {
      // Only one value (fractional seconds precision) will be given for this.
      getPrecScale(typeText + typeNameLen, prec, scale);
      dtType = new(mvqrHeap_) SQLTime(mvqrHeap_, TRUE/*don't care*/, prec);
    }
  else if (!strncmp(typeText, "TIMESTAMP", typeNameLen))
    {
      // Only one value (fractional seconds precision) will be given for this.
      getPrecScale(typeText + typeNameLen, prec, scale);
      dtType = new(mvqrHeap_) SQLTimestamp(mvqrHeap_, TRUE/*don't care*/, prec);
    }
  else if (!strncmp(typeText, "INTERVAL", typeNameLen))
    intvlType = parseIntervalTypeText(typeText + typeNameLen);
  else
    assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                       FALSE, QRLogicException,
                       "Invalid data type for normalized range pred item -- %s",
                       typeText);

  // Fill in the missing value based on the type to turn the open interval into
  // a closed one. This restricts the range to the values allowable for the type.
  // If the range op is < or <=, the missing value is the minimum value for the
  // type, otherwise it is the maximum value.
  QRScalarValuePtr typeBoundaryValue;
  Int64 typeMin, typeMax;
  if (numType || intvlType)
    {
      if (intvlType || numType->isExact())  // order of OR operands important
        {
          NAType& type = (numType ? (NAType&)*numType : (NAType&)*intvlType);
          SubrangeBase::getExactNumericMinMax(type, typeMin, typeMax, logLevel_);
          if (rangeOpType == ET_OpLE || rangeOpType == ET_OpLS)
            typeBoundaryValue = SubrangeBase::createScalarValElem
                                                  (mvqrHeap_, typeMin, type, "");
          else
            typeBoundaryValue = SubrangeBase::createScalarValElem
                                                  (mvqrHeap_, typeMax, type, "");
        }
      else  // real or double
        {
          if (rangeOpType == ET_OpLE || rangeOpType == ET_OpLS)
            typeBoundaryValue = new(mvqrHeap_) QRFloatVal(-(numType->getMaxValue()),
                                                          mvqrHeap_);
          else
            typeBoundaryValue = new(mvqrHeap_) QRFloatVal(numType->getMaxValue(),
                                                          mvqrHeap_);
        }
    }
  else      // char
    {
      SubrangeBase::getExactNumericMinMax(*dtType, typeMin, typeMax, logLevel_);
      if (rangeOpType == ET_OpLE || rangeOpType == ET_OpLS)
        typeBoundaryValue = SubrangeBase::createScalarValElem
                                              (mvqrHeap_, typeMin, *dtType, "");
      else
        typeBoundaryValue = SubrangeBase::createScalarValElem
                                              (mvqrHeap_, typeMax, *dtType, "");
    }

  // Fill in the missing value to produce a closed interval.
  switch (rangeOpType)
    {
      case ET_OpLS:
        addSubrange(typeBoundaryValue, value, TRUE, FALSE);
        break;
      case ET_OpLE:
        addSubrange(typeBoundaryValue, value, TRUE, TRUE);
        break;
      case ET_OpGT:
        addSubrange(value, typeBoundaryValue, FALSE, TRUE);
        break;
      case ET_OpGE:
        addSubrange(value, typeBoundaryValue, TRUE, TRUE);
        break;
      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                           FALSE, QRLogicException,
                           "Invalid inequality op in addDenormalizedSubrange() -- %d",
                           rangeOpType);
        break;
    }

  // One of these was allocated, the others are null.
  delete numType;
  delete dtType;
  delete intvlType;
  
} // addDenormalizedSubrange()

void RangeSpec::splitSubrangeEnd(CollIndex subrangeInx, 
                                 SubrangeBase* otherSubrange)
{
  SubrangeBase* newSubrange = 
          subranges_[subrangeInx]->splitAtEndOf(otherSubrange, mvqrHeap_);
  if (newSubrange)
    subranges_.insertAt(subrangeInx+1, newSubrange);
}

void RangeSpec::log()
{
  // For now, avoid routine logging of range predicate details. This will be
  // replaced with a leveling feature for QRLogger that can distinguish between
  // normal debug logging, and logging to indicate an exceptional event.
  if (QRLogger::isCategoryInDebug(CAT_SQL_COMP_RANGE))
  {
    ostringstream os;
    os << *this << '\0';
    QRLogger::log(CAT_SQL_COMP_RANGE, LL_DEBUG, "%s", os.str().c_str());
  }
}

void RangeSpec::display()
{
  ostringstream os;
  os << *this << '\0';
  cout << os.str().c_str() << endl;
}

// The list of subranges of this RangeSpec is modified by intersecting the range
// with that specified by the passed RangeSpec. Internal comments in this function
// refer to "this" and "other" for the range (or a subrange of it) being modified
// and the parameter, respectively.
void RangeSpec::intersectRange(RangeSpec* other)
{
  QRTRACER("intersectRange");
  // Start and end of other subrange relative to current subrange of this RangeSpec.
  RelativeLocation startLocation, endLocation;
  CollIndex otherSubrangeCount = other->subranges_.entries();
  CollIndex thisInx = 0, otherInx = 0;
  SubrangeBase *thisSubrange, *otherSubrange;
  
  QRLogger::log(CAT_SQL_COMP_RANGE, LL_DEBUG,
    "=== Entering intersectRange() ===\nRange to be modified:");
  log();  // write current state of this range
  QRLogger::log(CAT_SQL_COMP_RANGE, LL_DEBUG,
    "Range that will be ANDed to the above range:");
  other->log();

  // Handle the nullIncluded_ indicator.
  if (!other->nullIncluded_)
    nullIncluded_ = FALSE;

  // entries() must be called on each iteration for subranges_; the number of
  // entries can change within the loop, so don't use a variable assigned to it
  // before loop entry.
  while (thisInx < subranges_.entries())
    {
      // If there are no more subranges in other, the intersection with the
      // current subrange of this is empty, so remove it.
      if (otherInx >= otherSubrangeCount)
        {
          // Don't increment thisInx -- it will become the index of the next
          // entry after removal of the current one at that index.
          subranges_.removeAt(thisInx);
          continue;
        }

      // Assign the current subranges of "this" and "other", and compare them to
      // find the relative location of other to this.
      thisSubrange = subranges_[thisInx];
      otherSubrange = other->subranges_[otherInx];
      otherSubrange->compareTo(thisSubrange, startLocation, endLocation);

      switch (startLocation)
        {
          case rel_loc_after:  // other starts after end of this; remove this
            subranges_.removeAt(thisInx);
            break;
            
          case rel_loc_before: // other starts before this
            switch (endLocation)
              {
                case rel_loc_before:  // all of other precedes this;
                  otherInx++;         //   ignore other, keep the current this
                  break;
                case rel_loc_after:   // this entirely contained in other;
                  thisInx++;          //   keep this intact, move to next this
                  break;
                case rel_loc_within:  // other overlaps first part of this
                  splitSubrangeEnd(thisInx, otherSubrange);
                  thisInx++;          // skip over (retain) the part of this that
                  otherInx++;         //   intersects other, move to next other
                  break;
                default:
                  assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                                     FALSE, QRLogicException,
                                     "Unhandled RelativeLocation enum value -- %d",
				     startLocation);
                  break;
              }
            break;
            
          case rel_loc_within:
            switch (endLocation)
              {
                case rel_loc_within:
                  // Change current subrange to have start of other, split
                  // rest at end point of other. Move to the new subrange
                  // (2nd half of the split), keep current subrange of other.
                  thisSubrange->restrictStart(otherSubrange);
                  splitSubrangeEnd(thisInx, otherSubrange);
                  thisInx++;
                  break;
                case rel_loc_after:
                  // Drop part of current subrange before the start of the
                  // other; move to next subrange of this, stay on current
                  // subrange of other.
                  thisSubrange->restrictStart(otherSubrange);
                  thisInx++;
                  break;
                case rel_loc_before:
                  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                                    FALSE, QRLogicException,
				    "compareTo returned inconsistent results -- "
				    "start is within, but end is before");
                  break;
                default:
                  assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                                     FALSE, QRLogicException,
				     "Unhandled RelativeLocation enum value -- %d",
				     startLocation);
                  break;
              }
            break;
            
          default:
            assertLogAndThrow1(CAT_SQL_COMP_RANGE, logLevel_,
                               FALSE, QRLogicException,
			       "Unhandled RelativeLocation enum value -- %d",
			       startLocation);
            break;
        }
    }

  QRLogger::log(CAT_SQL_COMP_RANGE, LL_DEBUG,
    "Original range after intersection:");
  log();
}  // RangeSpec::intersectRange()

void RangeSpec::unionRange(RangeSpec* other)
{
  QRTRACER("unionRange");
  //@ZX -- should the ranges be required to be on the same column?
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    typeCompatible(other), QRLogicException,
                    "Attempt to union RangeSpecs of incompatible types");

  // Handle nullIncluded_ indicator.
  if (other->nullIncluded_)
    nullIncluded_ = TRUE;

  NAList< SubrangeBase* >& otherSubranges = other->subranges_;
  for (CollIndex i=0; i<otherSubranges.entries(); i++)
    {
      otherSubranges[i]->copyToRangeSpec(this);
    }
}

Int64 RangeSpec::getTotalDistinctValues(CollHeap* heap)
{
   Int64 total = 0;

   for (CollIndex i=0; i<subranges_.entries(); i++ ) {
      SubrangeBase* subr = subranges_[i];
      
      Int64 tval = subr->getTotalDistinctValues(heap, getType());

      if ( tval == -1 )
         return -1;

      total += tval;
   }
   return total;
}

Int64 SubrangeBase::getTotalDistinctValues(CollHeap* heap, const NAType* type)
{
   if ( isSingleValue() ) return 1;

   QRScalarValuePtr startVal = getStartScalarValElem(heap, *type);
   QRScalarValuePtr endVal = getEndScalarValElem(heap, *type);

   if ( startVal == NULL || endVal == NULL )
      return -1;

   ElementType valueElemType = startVal->getElementType();

   switch (valueElemType)
   {
      case ET_NumericVal:
      {

       if ( ! ((NumericType*)type)->isExact() )
          return -1;

       //Int32 scale = 
       //    static_cast<QRNumericValPtr>(startVal)-> getNumericScale();

       Int64 start = 
           static_cast<QRNumericValPtr>(startVal)-> getUnscaledNumericVal();

       if (!startInclusive())
          start++;  // make inclusive
            
       Int64 end =
           static_cast<QRNumericValPtr>(endVal)->getUnscaledNumericVal();

       if (!endInclusive())
         end--;    // make inclusive


       // Examples.
       // 1.  [1, 4] of SQLInt:  4-1+1=4
       // 2.  [1.0, 4.3] of SQLNumeric:  43-10+1=34
       return (end - start + 1);
     }

     // For all other types, if the subrange is not a single value,
     // return -1
     default:
       break;
   }

   return -1;
}


//
// SubrangeBase
//

static Int64 intervalMaxIntegerValue(const NAType& naType, logLevel level)
{
  assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                     naType.getTypeQualifier() == NA_INTERVAL_TYPE,
                     QRLogicException,
                     "intervalMaxIntegerValue() called for non-interval type -- %d",
                     naType.getTypeQualifier());

  const IntervalType& type = static_cast<const IntervalType&>(naType);
  Int64 maxVal = 0;
  rec_datetime_field startField = type.getStartField();
  rec_datetime_field endField = type.getEndField();
  UInt32 lp = type.getLeadingPrecision();
  UInt32 fp = type.getFractionPrecision();
  // Force correct overload of pow function to avoid overflow in result.
  Int64 maxStartFieldUnits = (Int64)(pow((long double)10, (long double)lp) - 1);
  const Int64 MILLION = 1000000;  // For clarity, and to avoid possibility of
                                  //    miscounting zeroes in repeated use

  // Requiring a break statement to exit a switch is an essentially useless
  // feature of C++ that leads to many difficult-to-find bugs, but here we
  // have a use for it (for once). The first case executed is for the end
  // field of the interval, but the cases "fall through" to the next case
  // until the start field is encountered (note that this requires ordering
  // the cases from least to greatest significance). As each case is executed,
  // the maximum integral value (in months for year-month intervals, microseconds
  // for day-time intervals) is accumulated. In addition to the start and end
  // fields, the maximal value is affected by the leading field precision and
  // (for day-time) the fractional seconds precision.
  switch (type.getEndField())
    {
      case REC_DATE_SECOND:
        if (startField == REC_DATE_SECOND)
          {
            maxVal += maxStartFieldUnits * MILLION +  // whole seconds as microseconds
                      ((Int64)pow(10, fp) - 1)        // plus fractional part...
                        * (Int64)pow(10, 6-fp);       // ...scaled to microseconds
            break;
          }
        else
          maxVal += 59 * MILLION +                  // whole seconds as microseconds
                    ((Int64)pow(10, fp) - 1)        // plus fractional part...
                      * (Int64)pow(10, 6-fp);       // ...scaled to microseconds
        // INTENTIONAL OMISSION OF BREAK; FALL THROUGH UNTIL START FIELD HANDLED

      case REC_DATE_MINUTE:
        if (startField == REC_DATE_MINUTE)
          {
            maxVal += maxStartFieldUnits * 60 * MILLION;
            break;
          }
        else
          maxVal += 59 * 60 * MILLION;
        // INTENTIONAL OMISSION OF BREAK; FALL THROUGH UNTIL START FIELD HANDLED

      case REC_DATE_HOUR:
        if (startField == REC_DATE_HOUR)
          {
            maxVal += maxStartFieldUnits * 60 * 60 * MILLION;
            break;
          }
        else
          maxVal += 23 * 60 * 60 * MILLION;
        // INTENTIONAL OMISSION OF BREAK; FALL THROUGH UNTIL START FIELD HANDLED

      case REC_DATE_DAY:
        // DAY has to be the start field, all less significant fields for a
        // day-time interval have been tried.
        maxVal += maxStartFieldUnits * 24 * 60 * 60 * MILLION;
        break;

      case REC_DATE_MONTH:
        if (startField == REC_DATE_MONTH)
          {
            maxVal = maxStartFieldUnits;
            break;
          }
        else
          maxVal = 11;
        // INTENTIONAL OMISSION OF BREAK; FALL THROUGH UNTIL START FIELD HANDLED

      case REC_DATE_YEAR:
        // This must be the start field for a year-month interval, since we have
        // already tried month.
        maxVal += maxStartFieldUnits * 12;
        break;

      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                          FALSE, QRLogicException,
                          "Invalid end field for interval type -- %d",
                          type.getEndField());
        break;
    }

  return maxVal;
} // intervalMaxIntegerValue()

// Get the minimum and maximum values for the passed type.
void SubrangeBase::getExactNumericMinMax(const NAType& type,
                                         Int64& typeMin,
                                         Int64& typeMax,
                                         logLevel level)
{
  if (type.getTypeQualifier() == NA_NUMERIC_TYPE)
    {
      const NumericType& numType = static_cast<const NumericType&>(type);
      if (numType.binaryPrecision())
        {
          switch (numType.getFSDatatype())
            {
              case REC_BIN64_SIGNED:  // Won't likely be called for this
              case REC_BIN16_SIGNED:
              case REC_BIN16_UNSIGNED:
              case REC_BIN32_SIGNED:
              case REC_BIN32_UNSIGNED:
              case REC_BPINT_UNSIGNED:
                typeMax = (Int64)numType.getMaxValue();
                typeMin = (numType.isSigned() ? (-typeMax - 1) : 0);
                break;

              default:
                assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                                  FALSE, QRLogicException,
                                  "No case in getExactNumericMinMax() for "
                                  "binary exact numeric of type %d",
                                  numType.getFSDatatype());
                break;
            }
        }
      else  // decimal precision exact numeric
        {
          // Force correct overload of pow function to avoid overflow in result.
          typeMax = (Int64)pow((long double)10, numType.getPrecision()) - 1;
          typeMin = (numType.isSigned() ? -typeMax : 0);
        }
    }
  else if (type.getTypeQualifier() == NA_DATETIME_TYPE)
    {
      const DatetimeType& dtType = (const DatetimeType&)type;
      switch (dtType.getSubtype())
        {
          case DatetimeType::SUBTYPE_SQLDate:
            typeMax = SUBRANGE_DATE_MAX;
            typeMin = SUBRANGE_DATE_MIN;
            break;
          case DatetimeType::SUBTYPE_SQLTime:
            // We represent these as a number of microseconds, so fractional
            // precision does not have to be taken into account.
            typeMax = SUBRANGE_TIME_MAX;
            typeMin = SUBRANGE_TIME_MIN; 
            break;
          case DatetimeType::SUBTYPE_SQLTimestamp:
            // We represent these as a number of microseconds, so fractional
            // precision does not have to be taken into account.
            typeMax = SUBRANGE_TIMESTAMP_MAX;
            typeMin = SUBRANGE_TIMESTAMP_MIN; 
            break;
          default:
            assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                               FALSE, QRLogicException,
                               "Invalid datetime subtype -- %d", dtType.getSubtype());
        }
    }
  else if (type.getTypeQualifier() == NA_INTERVAL_TYPE)
    {
      // Range of a specific interval type depends on the fields involved, the
      // leading field precision, and (for an interval involving seconds) the
      // fractional seconds precision.
      typeMax = intervalMaxIntegerValue(type, level);
      typeMin = -typeMax; 
    }
  else
    assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                       FALSE, QRLogicException,
                       "Type not handled by getExactNumericMinMax() -- %d", type.getTypeQualifier());
} // getExactNumericMinMax()

QRScalarValuePtr SubrangeBase::createScalarValElem(CollHeap* heap,
                                                   Int64 value,
                                                   const NAType& type,
                                                   const NAString& unparsedText)
{
  QRNumericValPtr numericValElem = new (heap) QRNumericVal(ADD_MEMCHECK_ARGS(heap));
  numericValElem->setNumericVal(value, type.getScale());
  numericValElem->setSql(unparsedText);
  return numericValElem;
}

QRScalarValuePtr SubrangeBase::createScalarValElem(CollHeap* heap,
                                                   double value,
                                                   const NAType& type,
                                                   const NAString& unparsedText)
{
  QRFloatValPtr floatValElem = new (heap) QRFloatVal(ADD_MEMCHECK_ARGS(heap));
  floatValElem->setFloatVal(value);
  floatValElem->setSql(unparsedText);
  return floatValElem;
}

QRScalarValuePtr SubrangeBase::createScalarValElem(CollHeap* heap,
                                                   const NAString& value,
                                                   const NAType& type,
                                                   const NAString& unparsedText)
{
  // When a LIKE predicate of the form "charCol like 'abc%'" is translated to a
  // >/<= pair, the length of the const operands is set to the declared length
  // of the column LIKE is applied to. The extra characters are nulls, which
  // messes up the XML parser, so we set the length to the actual length of the
  // constant.
  NAString correctValue(value);
  correctValue.resize(strlen(value.data()));
  QRStringValPtr stringValElem = new (heap) QRStringVal(ADD_MEMCHECK_ARGS(heap));
  stringValElem->setValue(correctValue);
  stringValElem->setSql(unparsedText);
  return stringValElem;
}

QRScalarValuePtr SubrangeBase::createScalarValElem(CollHeap* heap,
                                                   const RangeWString& value,
                                                   const NAType& type,
                                                   const NAString& unparsedText)
{
  // Trim trailing NULLs from possible operand of LIKE pred, as in similar
  // function above for non-Unicode strings. wcslen() returns wrong value on
  // Linux (half the correct size -- maybe a wide char is 4 bytes there?), so
  // we have to scan for terminating null.
  RangeWString correctValue(value);
  size_t len = 0;
  const NAWchar* wch = value.data();
  while (*wch++)
    len++;
  correctValue.remove(len);
  QRWStringValPtr stringValElem = new (heap) QRWStringVal(ADD_MEMCHECK_ARGS(heap));
  stringValElem->setWideValue(correctValue);
  stringValElem->setSql(unparsedText);
  return stringValElem;
}

Int64 SubrangeBase::getStepSize(const NAType* type, logLevel level)
{
  NABuiltInTypeEnum typeQual = type->getTypeQualifier();
  const DatetimeIntervalCommonType* dtiType = NULL;
  Int64 stepSize;
  const Int64 MILLION = 1000000;

  switch (typeQual)
    {
      case NA_NUMERIC_TYPE:
        stepSize = 1;
        break;

      case NA_DATETIME_TYPE:
        dtiType = static_cast<const DatetimeIntervalCommonType*>(type);
        switch (dtiType->getEndField())
          {
            case REC_DATE_DAY:
              stepSize = 1;
              break;
            case REC_DATE_SECOND:
              stepSize = (Int64)pow(10, 6 - dtiType->getFractionPrecision());
              break;
            default:
              assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                                 FALSE, QRDescriptorException,
                                 "unexpected end field for datetime type -- %d",
                                 dtiType->getEndField());
          }
        break;

      case NA_INTERVAL_TYPE:
        dtiType = static_cast<const DatetimeIntervalCommonType*>(type);
        switch (dtiType->getEndField())
          {
            case REC_DATE_YEAR:
              stepSize = 12;
              break;
            case REC_DATE_MONTH:
              stepSize = 1;
              break;
            case REC_DATE_DAY:
              stepSize = 24 * 60 * 60 * MILLION;  // # microseconds in a day
              break;
            case REC_DATE_HOUR:
              stepSize = 60 * 60 * MILLION;       // # microseconds in an hour
              break;
            case REC_DATE_MINUTE:
              stepSize = 60 * MILLION;            // # microseconds in a minute
              break;
            case REC_DATE_SECOND:
              stepSize = (Int64)pow(10, 6 - dtiType->getFractionPrecision());
              break;
            default:
              assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                                 FALSE, QRDescriptorException,
                                 "unexpected end field for datetime type -- %d",
                                 dtiType->getEndField());
          }
        break;

      default:
        assertLogAndThrow1(CAT_SQL_COMP_RANGE, level,
                           FALSE, QRDescriptorException,
                           "getStepSize() called for incorrect type -- %d",
                           typeQual);
    }

  return stepSize;
}

NABoolean SubrangeBase::isMinForType(const NAType& type)
{
  // Exclude non-inclusive start and single-point subranges.
  if (startIsMin_ || !startInclusive_ || isSingleValue())
    return FALSE;

  NABuiltInTypeEnum typeQual = type.getTypeQualifier();
  if (typeQual == NA_NUMERIC_TYPE)
    {
      const NumericType& numType = static_cast<const NumericType&>(type);
      if (numType.isExact())
        {
          Subrange<Int64>& int64Sub = static_cast<Subrange<Int64>&>(*this);
          Int64 typeMin, typeMax;
          getExactNumericMinMax(type, typeMin, typeMax, getLogLevel());
          return int64Sub.start == typeMin;
        }
      else
        {
          Subrange<double>& doubleSub = static_cast<Subrange<double>&>(*this);
          return doubleSub.start == -(numType.getMaxValue());
        }
    }
  else if (typeQual == NA_DATETIME_TYPE ||
           typeQual == NA_INTERVAL_TYPE)
    {
      Subrange<Int64>& int64Sub = static_cast<Subrange<Int64>&>(*this);
      Int64 typeMin, typeMax;
      getExactNumericMinMax(type, typeMin, typeMax, getLogLevel());
      return int64Sub.start == typeMin;
    }
  else
    return FALSE;
}

NABoolean SubrangeBase::isMaxForType(const NAType& type)
{
  // Exclude non-inclusive end and single-point subranges.
  if (endIsMax_ || !endInclusive_ || isSingleValue())
    return FALSE;

  NABuiltInTypeEnum typeQual = type.getTypeQualifier();
  if (typeQual == NA_NUMERIC_TYPE)
    {
      const NumericType& numType = static_cast<const NumericType&>(type);
      if (numType.isExact())
        {
          Subrange<Int64>& int64Sub = static_cast<Subrange<Int64>&>(*this);
          Int64 typeMin, typeMax;
          getExactNumericMinMax(numType, typeMin, typeMax, getLogLevel());
          return int64Sub.end == typeMax;
        }
      else
        {
          Subrange<double>& doubleSub = static_cast<Subrange<double>&>(*this);
          return doubleSub.end == numType.getMaxValue();
        }
    }
  else if (typeQual == NA_DATETIME_TYPE ||
           typeQual == NA_INTERVAL_TYPE)
    {
      Subrange<Int64>& int64Sub = static_cast<Subrange<Int64>&>(*this);
      Int64 typeMin, typeMax;
      getExactNumericMinMax(type, typeMin, typeMax, getLogLevel());
      return int64Sub.end == typeMax;
    }
  else
    return FALSE;
}

void SubrangeBase::display() const
{
  ostringstream os;
  write(os);
  cout << os.str().c_str() << endl;
}


//
// Subrange<T>
//

template <class T>
NABoolean Subrange<T>::operator==(const SubrangeBase& other) const
{
  QRTRACER("Subrange<T>::operator==");
  const Subrange<T>& otherSubrange = static_cast<const Subrange<T>&>(other);
  if (startIsMin_ != otherSubrange.startIsMin_ ||
      endIsMax_   != otherSubrange.endIsMax_)
    return FALSE;

  if (!startIsMin_ && start != otherSubrange.start)
    return FALSE;

  if (!endIsMax_ && end != otherSubrange.end)
    return FALSE;

  if (!startIsMin_ && startInclusive_ != otherSubrange.startInclusive_)
    return FALSE;

  if (!endIsMax_ && endInclusive_ != otherSubrange.endInclusive_)
    return FALSE;

  return TRUE;
}

template <class T>
void Subrange<T>::write(ostream& os) const
{
  os << ((startIsMin_ || startInclusive_) ? '[' : '(');
  if (startIsMin_)
    os << "<MIN>";
  else
    os << start;
  os << "..";
  if (endIsMax_)
    os << "<MAX>";
  else
    os << end;
  os << ((endIsMax_ || endInclusive_) ? ']' : ')');
  os << '\n';
}

// This template specialization for Unicode strings makes the assumption that
// the string is a UCS2 encoding of a Latin1 string.
template <>
void Subrange<RangeWString>::write(ostream& os) const
{
  const NAWchar* wstart = (const NAWchar*)start;
  const NAWchar* wend   = (const NAWchar*)end;
  UInt32 bufSize = start.length() + 1;
  if (end.length() >= bufSize)
    bufSize = end.length() + 1;
  char* buf = new char[bufSize];

  os << ((startIsMin_ || startInclusive_) ? '[' : '(');
  if (startIsMin_)
    os << "<MIN>";
  else
    {
      na_wcstombs(buf, wstart, bufSize);
      os << buf;
    }
  os << "..";
  if (endIsMax_)
    os << "<MAX>";
  else
    {
      na_wcstombs(buf, wend, bufSize);
      os << buf;
    }
  os << ((endIsMax_ || endInclusive_) ? ']' : ')');
  os << '\n';

  delete buf;
}

template <class T>
void Subrange<T>::copyToRangeSpec(RangeSpec *range,
                   CollHeap *heap) const
{
  // Use default copy ctor.
  range->placeSubrange(new(heap) Subrange<T>(*this));
}

template <class T>
NABoolean Subrange<T>::startsBefore(SubrangeBase* other) const
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (otherSubrange->startIsMin_)
    return FALSE;
  else if (startIsMin_)
    return TRUE;
  else if (start < otherSubrange->start)
    return TRUE;
  else 
    return start == otherSubrange->start &&
           startInclusive_ &&
           !otherSubrange->startInclusive_;
}

template <class T>
NABoolean Subrange<T>::startsAfter(SubrangeBase* other) const
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (otherSubrange->endIsMax_)
    return FALSE;
  else if (startIsMin_)
    return FALSE;
  else if (start > otherSubrange->end)
    return TRUE;
  else
    return start == otherSubrange->end &&
           (!startInclusive_ || !otherSubrange->endInclusive_);
}

template <class T>
NABoolean Subrange<T>::startsWithin(SubrangeBase* other) const
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (startIsMin_)
    return otherSubrange->startIsMin_;
  else if (otherSubrange->startIsMin_)
    if (otherSubrange->endIsMax_)
      return TRUE;
    else if (start < otherSubrange->end)
      return TRUE;
    else
      return start == otherSubrange->end && startInclusive_
                                         && otherSubrange->endInclusive_;
  else if (otherSubrange->endIsMax_)
    if (start > otherSubrange->start)
      return TRUE;
    else 
      return start == otherSubrange->start &&
                        (!startInclusive_ || otherSubrange->startInclusive_);
  else if (start > otherSubrange->start && start < otherSubrange->end)
    return TRUE;
  else if (start == otherSubrange->start &&
           (!startInclusive_ || otherSubrange->startInclusive_))
    return TRUE;
  else
    return start == otherSubrange->end &&
           startInclusive_ &&
           otherSubrange->endInclusive_;
}

template <class T>
NABoolean Subrange<T>::endsBefore(SubrangeBase* other) const
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (endIsMax_)
    return FALSE;
  else if (otherSubrange->startIsMin_)
    return FALSE;
  else if (end < otherSubrange->start)
    return TRUE;
  else
    return end == otherSubrange->start &&
           (!endInclusive_ || !otherSubrange->startInclusive_);
}

template <class T>
NABoolean Subrange<T>::endsAfter(SubrangeBase* other) const
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (endIsMax_)
    return !otherSubrange->endIsMax_;
  else if (otherSubrange->endIsMax_)
    return FALSE;
  else if (end > otherSubrange->end)
    return TRUE;
  else
    return end == otherSubrange->end &&
           endInclusive_ &&
           !otherSubrange->endInclusive_;
}

template <class T>
NABoolean Subrange<T>::endsWithin(SubrangeBase* other) const
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (endIsMax_)
    return otherSubrange->endIsMax_;
  else if (otherSubrange->endIsMax_)
    if (otherSubrange->startIsMin_)
      return TRUE;
    else if (end > otherSubrange->start)
      return TRUE;
    else
      return end == otherSubrange->start && endInclusive_
                                         && otherSubrange->startInclusive_;
  else if (otherSubrange->startIsMin_)
    if (end < otherSubrange->end)
      return TRUE;
    else 
      return end == otherSubrange->end &&
             (!endInclusive_ || otherSubrange->endInclusive_);
  else if (end > otherSubrange->start && end < otherSubrange->end)
    return TRUE;
  else if (end == otherSubrange->start &&
           endInclusive_ &&
           otherSubrange->startInclusive_)
    return TRUE;
  else
    return end == otherSubrange->end &&
           (otherSubrange->endInclusive_ || !endInclusive_);
}

// This template returns false for all types except Int64, which is used for
// all integral types and fixed numerics. A specialization of this template
// for Int64 handles those types.
template <class T> inline
NABoolean Subrange<T>::adjacentTo(SubrangeBase* other) const
{
  return FALSE;
}

// This is the specialization of the above template that determines adjacency
// for types represented in subranges of Int64.
template <>
NABoolean Subrange<Int64>::adjacentTo(SubrangeBase* other) const
{
  Subrange<Int64>* otherSubrange = (Subrange<Int64>*)other;
  return (end + 1 == otherSubrange->start);
}

template <class T>
void Subrange<T>::compareTo(SubrangeBase* other,
                            RelativeLocation& startLoc,
                            RelativeLocation& endLoc) const
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;

  if (startsBefore(otherSubrange))
    startLoc = rel_loc_before;
  else if (startsAfter(otherSubrange))
    startLoc = rel_loc_after;
  else
    startLoc = rel_loc_within;

  if (endsBefore(otherSubrange))
    endLoc = rel_loc_before;
  else if (endsAfter(otherSubrange))
    endLoc = rel_loc_after;
  else
    endLoc = rel_loc_within;
}

template <class T>
void Subrange<T>::extendStart(SubrangeBase* other)
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (startIsMin_)
    return;

  if (otherSubrange->startIsMin_)
    startIsMin_ = TRUE;
  else
    {
      if (otherSubrange->start < start)
        {
          start = otherSubrange->start;
          unparsedStart_= otherSubrange->unparsedStart_;
          startInclusive_ = otherSubrange->startInclusive_;
          startAdjustment_ = otherSubrange->startAdjustment_;
        }
      else if (otherSubrange->start == start && otherSubrange->startInclusive_)
        {
          startInclusive_ = TRUE;
          startAdjustment_ = otherSubrange->startAdjustment_;
        }
    }
}

template <class T>
void Subrange<T>::extendEnd(SubrangeBase* other)
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (endIsMax_)
    return;

  if (otherSubrange->endIsMax_)
    endIsMax_ = TRUE;
  else 
    {
      if (otherSubrange->end > end)
        {
          end = otherSubrange->end;
          unparsedEnd_= otherSubrange->unparsedEnd_;
          endInclusive_ = otherSubrange->endInclusive_;
          endAdjustment_ = otherSubrange->endAdjustment_;
        }
      else if (otherSubrange->end == end && otherSubrange->endInclusive_)
        {
          endInclusive_ = TRUE;
          endAdjustment_ = otherSubrange->endAdjustment_;
        }
    }
}

template <class T>
void Subrange<T>::restrictStart(SubrangeBase* other)
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  if (otherSubrange->startIsMin_)
    return;

  if (startIsMin_ || otherSubrange->start > start)
    {
      start = otherSubrange->start;
      unparsedStart_= otherSubrange->unparsedStart_;
      startIsMin_ = FALSE;
      startInclusive_ = otherSubrange->startInclusive_;
      startAdjustment_ = otherSubrange->startAdjustment_;
    }
  else if (otherSubrange->start == start && !otherSubrange->startInclusive_)
    {
      startInclusive_ = FALSE;
      startAdjustment_ = 0;
    }
}

template <class T>
SubrangeBase* Subrange<T>::splitAtEndOf(const SubrangeBase* other, CollHeap* heap)
{
  Subrange<T>* otherSubrange = (Subrange<T>*)other;
  Subrange<T>* newSubrange;
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                    otherSubrange->endsWithin(this),
		    QRLogicException,
		    "Can't split subrange on end of passed subrange; it does not "
		    "end within the target subrange");

  if (otherSubrange->endIsMax_)
    // this must be endIsMax_ as well, so no new subrange created by split.
    newSubrange = NULL;
  else if (!endIsMax_ &&(end == otherSubrange->end))
    {
      // End point of both is same, check inclusion to see if new subrange
      // results from split.
      if (endInclusive_)
        {
          if (otherSubrange->endInclusive_)
            newSubrange = NULL;
          else
            {
              // Split subrange loses last value, which becomes the sole value
              // of the new subrange.
              endInclusive_ = FALSE;
              endAdjustment_ = 0;
              newSubrange = new (heap) Subrange<T>(logLevel_);
              newSubrange->start = newSubrange->end = end;
              newSubrange->unparsedStart_= newSubrange->unparsedEnd_= unparsedEnd_;
              newSubrange->startInclusive_ = newSubrange->endInclusive_ = TRUE;
            }
        }
      else
        newSubrange = NULL;
    }
  else
    {
      newSubrange = new (heap) Subrange<T>(logLevel_);
      newSubrange->start = otherSubrange->end;
      newSubrange->unparsedStart_= otherSubrange->unparsedEnd_;
      newSubrange->startInclusive_ = !otherSubrange->endInclusive_;
      newSubrange->startAdjustment_ = 0;
      newSubrange->end = end;
      newSubrange->unparsedEnd_= unparsedEnd_;
      newSubrange->endInclusive_ = endInclusive_;
      newSubrange->endAdjustment_ = endAdjustment_;
      newSubrange->endIsMax_ = endIsMax_;
      end = otherSubrange->end;
      unparsedEnd_= otherSubrange->unparsedEnd_;
      endInclusive_ = otherSubrange->endInclusive_;
      endAdjustment_ = otherSubrange->endAdjustment_;
      endIsMax_ = FALSE; // we know other endIsMax_ is false from test above
    }

  // If the manipulations above have produced a single-point subrange (equivalent
  // to an equality predicate), remove any adjustment previously used to change
  // </> to <=/>=. Otherwise, makeSubrangeItemExpr() will modify the value to
  // restore the predicate to what it thinks is its original form.
  if (isSingleValue())
    startAdjustment_ = endAdjustment_ = 0;
  if (newSubrange && newSubrange->isSingleValue())
    newSubrange->startAdjustment_ = newSubrange->endAdjustment_ = 0;

  return newSubrange;
}

// This template is a no-op for all types except Int64, which is used for
// all integral types and fixed numerics. A specialization of this template
// for Int64 handles those types.
template <class T> inline
void Subrange<T>::initSpecifiedValueCount()
{}

// This is the specialization for Int64, which sets the specified value count
// to 1 (if not already set) for a single-point subrange.
template <>
void Subrange<Int64>::initSpecifiedValueCount()
{
  if (isSingleValue() && specifiedValueCount_ == 0)
    specifiedValueCount_ = 1;
}

// This template throws an exception if the function is called for any type other
// than Int64. The work for Int64 is done by a specialization of the template.
template <class T> inline
void Subrange<T>::makeStartInclusive(const NAType* type, NABoolean& overflowed)
{
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_, FALSE, QRLogicException,
                    "makeStartInclusive() called for non-Int64-based type");
}

// This is the specialization of the above template that makes the start of a
// subrange inclusive, by adjusting it up to the next allowable value (in its
// rangespec internal representation) of the type if necessary, so that > can
// be changed to >= (the canonical form for subranges uses closed rather than
// open intervals).
template <>
void Subrange<Int64>::makeStartInclusive(const NAType* type,
                                         NABoolean& overflowed)
{
  startAdjustment_ = getStepSize(type, logLevel_);

  // The adjustment is always a positive integer, so if adding it produces a
  // smaller value, we know we've overflowed to a negative number.
  if (start + startAdjustment_ > start)
    {
      start += startAdjustment_;
      setStartInclusive(TRUE);
      overflowed = FALSE;
    }
  else
    {
      // Overflow; set the adjustment to 0 so start will not be changed in
      // makeSubrangeItemExpr(), which is used to get the value to use in the
      // modified predicate produced by the Normalizer.
      startAdjustment_ = 0;
      start = LLONG_MAX;
      setStartInclusive(FALSE);
      overflowed = TRUE;
    }
}

// This template throws an exception if the function is called for any type other
// than Int64. The work for Int64 is done by a specialization of the template.
template <class T> inline
void Subrange<T>::makeEndInclusive(const NAType* type, NABoolean& overflowed)
{
  assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_, FALSE, QRLogicException,
                    "makeEndInclusive() called for non-Int64-based type");
}

// This is the specialization of the above template that makes the end of a
// subrange inclusive, by adjusting it down to the previous allowable value (in
// its rangespec internal representation) of the type if necessary, so that <
// can be changed to <= (the canonical form for subranges uses closed rather
// than open intervals).
template <>
void Subrange<Int64>::makeEndInclusive(const NAType* type,
                                       NABoolean& overflowed)
{
  endAdjustment_ = getStepSize(type, logLevel_);

  // The adjustment is always a positive integer, so if subtracting it produces
  // a larger value, we know we've wrapped around to a positive number.
  if (end - endAdjustment_ < end)
    {
	    end -= endAdjustment_;
      setEndInclusive(TRUE);
      overflowed = FALSE;
    }
  else
    {
      // Overflow; set the adjustment to 0 so end will not be changed in
      // makeSubrangeItemExpr(), which is used to get the value to use in the
      // modified predicate produced by the Normalizer.
      endAdjustment_ = 0;
      end = LLONG_MIN;
      setEndInclusive(FALSE);
      overflowed = TRUE;
    }
}

Int32 RangeStringComparison::rngStrncmp(const char* s1, const char* s2, size_t len)
{
  return strncmp(s1, s2, len);
}

Int32 RangeStringComparison::rngStrncmp(const NAWchar* s1, const NAWchar* s2, size_t len)
{
  return na_wcsncmp(s1, s2, len);
}

template <class STRTYPE, class CHARTYPE>
Int32 RangeStringComparison::cmp(const STRTYPE& rngStr1, const STRTYPE& rngStr2,
                               size_t len1, size_t len2, const CHARTYPE padChar)
{
  const CHARTYPE* str1 = rngStr1.data();
  const CHARTYPE* str2 = rngStr2.data();
  const CHARTYPE* longerStr;
  size_t shorterLen;           // length of shorter string
  size_t diffLen;              // number of extra chars in longer string
  
  // The comparison of the longer string's extra characters is performed as if
  // done by a strcmp with the longer string as the first argument, regardless
  // of whether it was the first argument to this function. resultIfGtr will be
  // set to 1 if it actually was the first argument, or -1 if it was the 2nd.
  // The negation of this value is returned if the longer string is less than
  // the shorter, which reverses the sign of the result if the longer string was
  // the 2nd argument.
  Int32 resultIfGtr;
  
  if (len1 > len2)
    {
      shorterLen = len2;
      diffLen = len1 - len2;
      longerStr = str1;
      resultIfGtr = 1;
    }
  else
    {
      shorterLen = len1;
      diffLen = len2 - len1;
      longerStr = str2;
      resultIfGtr = -1;
    }

  Int32 stemResult = rngStrncmp(str1, str2, shorterLen);
  if (stemResult)
    return stemResult;  // strings differ before end of shorter one

  // Compare each extra character from the longer string to a space, and
  // return the result of the comparison if we find one that isn't a space.
  const CHARTYPE* currentCharPtr = longerStr + shorterLen;
  for (size_t i = 0; i<diffLen; i++)
    {
      // Note that we return 1 or -1 if different, which may not be the same
      // value that strcmp would have returned (although the sign will be the
      // same).
      if (*currentCharPtr > padChar)
        return resultIfGtr;
      else if (*currentCharPtr < padChar)
        return -resultIfGtr;

      currentCharPtr++;
    }

  // All extra characters were spaces, strings are equal.
  return 0;
}

Int32 RangeStringComparison::cmp(const RangeWString& rngStr1, const RangeWString& rngStr2,
                   size_t len1, size_t len2, const wchar_t padChar)
{
  const wchar_t* str1 = rngStr1.data();
  const wchar_t* str2 = rngStr2.data();
  const wchar_t* longerStr;
  size_t shorterLen;           // length of shorter string
  size_t diffLen;              // number of extra chars in longer string

  // The comparison of the longer string's extra characters is performed as if
  // done by a strcmp with the longer string as the first argument, regardless
  // of whether it was the first argument to this function. resultIfGtr will be
  // set to 1 if it actually was the first argument, or -1 if it was the 2nd.
  // The negation of this value is returned if the longer string is less than
  // the shorter, which reverses the sign of the result if the longer string was
  // the 2nd argument.
  Int32 resultIfGtr;

  if (len1 > len2)
    {
      shorterLen = len2;
      diffLen = len1 - len2;
      longerStr = str1;
      resultIfGtr = 1;
    }
  else
    {
      shorterLen = len1;
      diffLen = len2 - len1;
      longerStr = str2;
      resultIfGtr = -1;
    }

  Int32 stemResult = rngStrncmp(str1, str2, shorterLen);
  if (stemResult)
    return stemResult;  // strings differ before end of shorter one

  // Compare each extra character from the longer string to a space, and
  // return the result of the comparison if we find one that isn't a space.
  const wchar_t* currentCharPtr = longerStr + shorterLen;
  for (size_t i = 0; i<diffLen; i++)
    {
      // Note that we return 1 or -1 if different, which may not be the same
      // value that strcmp would have returned (although the sign will be the
      // same).
      if (*currentCharPtr > padChar)
        return resultIfGtr;
      else if (*currentCharPtr < padChar)
        return -resultIfGtr;

      currentCharPtr++;
    }

  // All extra characters were spaces, strings are equal.
  return 0;
}

Int32 RangeStringComparison::cmp(const RangeString& rngStr1, const RangeString& rngStr2,
                   size_t len1, size_t len2, const char padChar)
{
  const char* str1 = rngStr1.data();
  const char* str2 = rngStr2.data();
  const char* longerStr;
  size_t shorterLen;           // length of shorter string
  size_t diffLen;              // number of extra chars in longer string

  // The comparison of the longer string's extra characters is performed as if
  // done by a strcmp with the longer string as the first argument, regardless
  // of whether it was the first argument to this function. resultIfGtr will be
  // set to 1 if it actually was the first argument, or -1 if it was the 2nd.
  // The negation of this value is returned if the longer string is less than
  // the shorter, which reverses the sign of the result if the longer string was
  // the 2nd argument.
  Int32 resultIfGtr;

  if (len1 > len2)
    {
      shorterLen = len2;
      diffLen = len1 - len2;
      longerStr = str1;
      resultIfGtr = 1;
    }
  else
    {
      shorterLen = len1;
      diffLen = len2 - len1;
      longerStr = str2;
      resultIfGtr = -1;
    }

  Int32 stemResult = rngStrncmp(str1, str2, shorterLen);
  if (stemResult)
    return stemResult;  // strings differ before end of shorter one

  // Compare each extra character from the longer string to a space, and
  // return the result of the comparison if we find one that isn't a space.
  const char* currentCharPtr = longerStr + shorterLen;
  for (size_t i = 0; i<diffLen; i++)
    {
      // Note that we return 1 or -1 if different, which may not be the same
      // value that strcmp would have returned (although the sign will be the
      // same).
      if (*currentCharPtr > padChar)
        return resultIfGtr;
      else if (*currentCharPtr < padChar)
        return -resultIfGtr;

      currentCharPtr++;
    }

  // All extra characters were spaces, strings are equal.
  return 0;
}

RangeWString& RangeWString::operator=(const RangeWString& other)
{
  // Not sure if this is the most efficient way, but it seems safe. In either
  // case (rhs it an empty string or not), we rely on the existing NAWString
  // operator=(const NAWchar* wstr) function.
  if (other.length() == 0)
    {
      NAWchar nullChar = 0;
      *((NAWString*)this) = &nullChar;
      return *this;
    }
  else
    {
      replace(0, length(), other.data(), other.length());
      return *this;
    }
}
