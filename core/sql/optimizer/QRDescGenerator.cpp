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
 **************************************************************************
 *
 * File:         QRDescGenerator.cpp
 * Description:  Construction of query descriptor from QueryAnalysis
 * Created:      01/24/2008
 * Language:     C++
 *
 **************************************************************************
 */

#include "Analyzer.h"
#include "QRDescGenerator.h"
#include "QRSharedPtr.h"
#include "RelGrby.h"
#include "NumericType.h"
#include "ItemLog.h"
#include "QRLogger.h"
#include "QRExprElement.h"
#include "RelUpdate.h"

const UInt32 QRDescGenerator::GENERATED_JBBID_START = 10000;

ULng32 hashString(const NAString& str) { return str.hash(); }
ULng32 hashValueId(const QRValueId& vid) { return vid; }

Visitor::VisitResult SetRefVisitor::visit(QRElementPtr caller)
{
  NAString ref = caller->getRef();
  if (ref == "")
    return VR_Continue;

  // Skip if not column, table, or joinpred
  if (!strchr("CTJ", *ref.data()))
    return VR_Continue;
  
  QRElementPtr elem = idHash_.getFirstValue(&ref);
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL, 
                     elem != NULL, QRLogicException, 
		     "Referenced element %s does not exist.", ref.toCharStar());
  caller->setReferencedElement(elem);

  return VR_Continue;
}

//
// XmlValidatorVisitor
//

Visitor::VisitResult XmlValidatorVisitor::visit(QRElementPtr caller)
{
  if (caller->getElementType() == ET_StringVal)
  {
    QRStringValPtr stringVal = static_cast<QRStringValPtr>(caller);
    const NAString& value = stringVal->getValue();
    if (value.contains('&') ||
        value.contains('%') )
    {
      foundProblem_ = TRUE;
      return VR_Stop;
    }
  }
  else if (caller->getElementType() == ET_WStringVal)
  {
    QRWStringValPtr stringVal = static_cast<QRWStringValPtr>(caller);
    const NAWString& value = stringVal->getWideValue();
    if (na_wcschr(value, NAWCHR('&')) ||
        na_wcschr(value, NAWCHR('%')) )
    {
      foundProblem_ = TRUE;
      return VR_Stop;
    }
  }

  return VR_Continue;
}
 
void EqualitySet::determineType()
{
  QRTRACER("EqualitySet::determineType()");
  if (isEmpty())
    {
      // I don't think this should happen.
      QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
        "determineType() called for empty EqualitySet.");
      return;
    }

  NABoolean isExact = FALSE, wasExact, isSigned = FALSE, isNullable = FALSE;
  Lng32 magnitude = 0, otherMagnitude, scale = 0;

  // Go through each member of the equality set, getting the parameters for the
  // narrowest type common to them all (excluding a constant, if any).
  NABoolean typeInitialized = FALSE;
  for (CollIndex i=0; i<entries(); i++)
    {
      // Don't let the constant's type influence the eqset type.
      if (at(i)->getOperatorType() == ITM_CONSTANT)
        continue;

      const NAType& t = at(i)->getValueId().getType();
      if (t.getTypeQualifier() != NA_NUMERIC_TYPE)
        {
          // Eventually we should synthesize non-numeric types as well, but for
          // now, just make sure the type is not null.
          type_ = t.newCopy(heap_);
          return;
        }

      const NumericType& nt = static_cast<const NumericType&>(t);
      if (!typeInitialized)
        {
          // Initalize the type parameters to those of the first member.
          // Regarding the computation of magnitude for float types, see the
          // sad tale of precision for those types in the comment for
          // NumericType::getTruePrecision(), and also note that getMagnitude()
          // uses getPrecision() (which apparently can return 0 for floating
          // point types) instead of getTruePrecision(). We add 1 to the
          // calculated magnitude so it can be easily distinguished from
          // decimal-precision exact numerics when we decide what type to
          // create at the end of this function.
          typeInitialized = TRUE;
          isExact = nt.isExact();
          isSigned = nt.isSigned();
          isNullable = nt.supportsSQLnull();
          magnitude = isExact ? nt.getMagnitude()
                              : nt.getTruePrecision() * 10 + 1;
          scale = nt.getScale();
        }
      else
        {
          // Modify any type parameters that are more restrictive than those
          // of this member.
          wasExact = isExact;
          if (nt.isExact())
            isExact = TRUE;
          if (!nt.isSigned())
            isSigned = FALSE;
          if (!nt.supportsSQLnull())
            isNullable = FALSE;
          otherMagnitude = nt.isExact() ? nt.getMagnitude()
                                        : nt.getTruePrecision() * 10 + 1;
          if (magnitude > otherMagnitude)
            magnitude = otherMagnitude;
          if (scale > nt.getScale() || !wasExact)
            scale = nt.getScale();
        }
    }

  // Create the type that supports a range of values common to all members of
  // the equality set. Magnitude is divisible by 10 for decimal precision exact
  // numerics.
  if (magnitude % 10 > 0)
    {
      // Binary precision smallint, int, largeint, real, double)
      if (magnitude < 50)
        type_ = new(heap_) SQLSmall(heap_,isSigned, isNullable);
      else if (magnitude < 100)
        type_ = new(heap_) SQLInt(heap_, isSigned, isNullable);
      else if (magnitude < 200)
        type_ = new(heap_) SQLLargeInt(heap_, isSigned, isNullable);
      else if (magnitude < 500)
        type_ = new(heap_) SQLReal(heap_, isNullable);
      else
        type_ = new(heap_) SQLDoublePrecision(heap_, isNullable);
    }
  else
    {
      // @ZX need to amend this (and elsewhere) for SQLBigNum.
      // Numeric or Decimal -- type will be generated as Numeric
      const Int16 DisAmbiguate = 0;
      type_ = new(heap_) SQLNumeric(heap_, isSigned, (magnitude / 10) + scale, scale,
                                    DisAmbiguate,  // added for 64bit proj.
                                    isNullable);
    }
} // determineType()

QRDescGenerator::~QRDescGenerator()
{
  QRTRACER("QRDescGenerator::~QRDescGenerator()");
  for (CollIndex i=0; i<allEqualitySets_.entries(); i++)
    delete allEqualitySets_[i];
}

NABoolean QRDescGenerator::typeSupported(const NAType* type)
{
  QRTRACER("QRDescGenerator::typeSupported()");
  switch (type->getTypeQualifier())
    {
      case NA_NUMERIC_TYPE:
        {
          const NumericType* numType = static_cast<const NumericType*>(type);
          
          // All approx numerics are ok.
          if (!numType->isExact())
            return TRUE;

          // Don't handle software-supported types yet. This is of type BigNum only 
          if (numType->isBigNum())
            return FALSE;
          else
            return TRUE;
        }

      case NA_CHARACTER_TYPE:
        {
          if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON)
            return FALSE;
          return ((CharType*)type)->getCollation() == CharInfo::DefaultCollation;
	      }

      case NA_INTERVAL_TYPE:
        {
          // For interval types, we have to take into account that we represent
          // values in terms of the lowest possible field for the interval
          // category (year-month or day-time). Thus, we can't support the max
          // leading field precision for specific interval types even when they
          // don't include those lower fields.
          const IntervalType* intvType = static_cast<const IntervalType*>(type);
          switch (intvType->getStartField())
            {
              case REC_DATE_YEAR:
                return (SQLInterval::MAX_LEADING_PRECISION >=
                        IntervalType::getPrecision(intvType->getStartField(),
                                                   intvType->getLeadingPrecision(),
                                                   REC_DATE_MONTH,
                                                   0));
              case REC_DATE_MONTH:
                return TRUE;

              default:
                // All day-time interval values are expressed in terms of microseconds.

                // If fractional precision is greater than
                // microsecs, disable rangespec transformation.
                if (type->getScale() > SQLInterval::MAX_FRACTION_PRECISION_USEC)
                  return FALSE;

                return (SQLInterval::MAX_LEADING_PRECISION >=
                        IntervalType::getPrecision(intvType->getStartField(),
                                                   intvType->getLeadingPrecision(),
                                                   REC_DATE_SECOND,
                                                   SQLInterval::MAX_FRACTION_PRECISION_USEC));
            }
        }

      case NA_DATETIME_TYPE:
      // Remaining types are not yet supported.
      //case NA_BOOLEAN_TYPE
      //case NA_UNKNOWN_TYPE:
      //case NA_USER_SUPPLIED_TYPE:
      //case NA_RECORD_TYPE:
      //case NA_ROWSET_TYPE:

        // datetime values are currently converted to Int64 microseconds value
        // for rangespec constants. If fractional precision is greater than
        // microsecs, disable rangespec transformation.
        if (type->getScale() > DatetimeType::MAX_FRACTION_PRECISION_USEC)
          return FALSE;

        return TRUE;

      default:
        return FALSE;
    }
}

// -----------------------------------------------------------------------
NABoolean
QRDescGenerator::getTableId(ValueId    vid, 
                            CANodeId&  nodeID,      // OUT
                            ValueId&   cvid,        // OUT
                            ValueId&   vegrefVid,   // OUT
                            NAString&  baseColName, // OUT
                            NABoolean& isExtraHub,  // OUT
                            Int32&       colIndex)    // OUT
{
  QRTRACER("QRDescGenerator::getTableId()");
  ItemExpr *pExpr = vid.getItemExpr();
  BaseColumn *pBC = 0;

  cvid = vid;
  vegrefVid = NULL_VALUE_ID;

  if (pExpr->getOperatorType() == ITM_VEG_REFERENCE) 
    {
      vegrefVid = vid;
      const ValueIdSet &vegMembers = static_cast<VEGReference*>(pExpr)
                                                    ->getVEG()->getAllValues();
      // Search the veg members for a base column, so we can get its table id.
      for (ValueId someMemberId=vegMembers.init();
           !pBC && vegMembers.next(someMemberId);
           vegMembers.advance(someMemberId))
        {
          if (someMemberId.getItemExpr()->getOperatorType() == ITM_BASECOLUMN) 
          {
            cvid = someMemberId;
            pBC = static_cast<BaseColumn*>(someMemberId.getItemExpr());
          }
        }
    }
  else if (pExpr->getOperatorType() == ITM_BASECOLUMN)
    pBC = static_cast<BaseColumn*>(pExpr);
  else if (pExpr->getOperatorType() == ITM_INDEXCOLUMN)
    {
      cvid = static_cast<IndexColumn*>(pExpr)->getDefinition(); 
      pBC = static_cast<BaseColumn*>(cvid.getItemExpr());
    }

  if (pBC) 
  {
    baseColName = pBC->getNAColumn()->getTableName()->getQualifiedNameAsAnsiString()
                    + "." + pBC->getColName();
    colIndex = pBC->getColNumber();
    TableDesc *pTD = pBC->getTableDesc();
    if (pTD) 
    {
      if (vegrefVid == NULL_VALUE_ID)
        {
          ValueIdList baseCols;
          ValueIdList vegCols;
          baseCols.insert(pBC->getValueId());
          pTD->getEquivVEGCols(baseCols, vegCols);
          ItemExpr* ie = vegCols[0].getItemExpr();
          if (ie->getOperatorType() == ITM_VEG_REFERENCE)
            vegrefVid = ie->getValueId();
        }
      const TableAnalysis *pTA = pTD->getTableAnalysis();
      if (pTA) 
      {
	NodeAnalysis *pNA = pTA->getNodeAnalysis();
	if (pNA) 
	{
	  nodeID = pNA->getId();
	  isExtraHub = pNA->isExtraHub();
	  return TRUE;
	}
      }
    }
  }

  nodeID = NULL_CA_ID;
  return FALSE;

}  // getTableId()

CANodeId QRDescGenerator::getNodeId(ValueId vid)
{
  QRTRACER("QRDescGenerator::getNodeId()");
  // Required arguments to getTableId(). All we want here is the node id.
  CANodeId nodeId;
  ValueId dummyVid;
  ValueId dummyVid2;
  NAString dummyString;
  NABoolean dummyBool;
  Int32 colIndex;

  getTableId(vid, nodeId, dummyVid, dummyVid2, dummyString, dummyBool, colIndex);
  return nodeId;
}

// -----------------------------------------------------------------------
QRColumnPtr QRDescGenerator::genQRColumn(ValueId vid, 
                                          UInt32 joinPredId /*= 0*/, 
                                          NABoolean markAsUsed /*= TRUE*/)
{
  QRTRACER("QRDescGenerator::genQRColumn()");

  NABoolean isExtraHub;
  CANodeId nodeID;
  ValueId col_vid;
  ValueId vegref_vid;
  NAString baseColName;
  Int32 colIndex;
  NABoolean gotNodeID = getTableId(vid, nodeID, col_vid, vegref_vid,
                                   baseColName, isExtraHub, colIndex);

  if (!gotNodeID)
    {
      // This should be a vegref to an instantiate_null for a LOJ.
      ItemExpr* itemExpr = vid.getItemExpr();
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        isInstNull(itemExpr), QRDescriptorException, 
			"genQRColumn called for ValueID that is neither column nor "
			"instantiate_null function");
      
      return skipInstNull(itemExpr);
    }

  QRColumnPtr columnElement = new(mvqrHeap_) QRColumn(ADD_MEMCHECK_ARGS(mvqrHeap_));

  if (bGenColumnRefs_) 
    {
      if (mColumnsUsed_.contains(col_vid)) 
        {
          // Note the early return if we reference an existing column,
          // which avoids setting other column properties below.
          columnElement->setRefFromInt(col_vid, FALSE);
          columnElement->setReferencedElement(getElementForValueID('C', col_vid));
          if (mExtraHubColumnsUsed_.contains(col_vid))
            columnElement->setExtraHub(TRUE);
          return columnElement;
        }
      else if (markAsUsed)  // not when called by setPredBitmap()
      {
        mColumnsUsed_ += col_vid;
        if (isExtraHub)
          mExtraHubColumnsUsed_ += col_vid;
      }
    }
  columnElement->setFullyQualifiedColumnName(baseColName);
  columnElement->setAndRegisterID(col_vid, colTblIdHash_);
  columnElement->setTableID((Int32)nodeID);
  columnElement->setExtraHub(isExtraHub);
  columnElement->setVegrefId(vegref_vid);
  columnElement->setColIndex(colIndex);
  columnElement->setNullable(vid.getType().supportsSQLnull());

  // If col_vid is the value id of a column used in a join pred, it should 
  // reference the id of the join pred.
  ValueId mappedVid, vegVid;
  colToJoinPredMap_.mapValueIdUp(mappedVid, col_vid);
  if (mappedVid != col_vid)  // returns same value if not mapped
    {
      ItemExpr* cvie = col_vid.getItemExpr();
      if (cvie->getOperatorType() == ITM_BASECOLUMN)
        {
          BaseColumn* bc = static_cast<BaseColumn*>(cvie);
          vegVid = bc->getTableDesc()->getColumnVEGList()[bc->getColNumber()];
        }
      else
        vegVid = mappedVid;
        
      if (mappedVid == vegVid)
        {
          columnElement->setVegrefId(vegref_vid);
        }
    }
        
  return columnElement;
}  // genQRColumn()

NABoolean QRDescGenerator::isInstNull(ItemExpr* itemExpr)
{
  QRTRACER("isInstNull()");
  
  if (itemExpr->getOperatorType() == ITM_INSTANTIATE_NULL)
    return TRUE;
    
  if (itemExpr->getOperatorType() != ITM_VEG_REFERENCE)
    return FALSE;

  const ValueIdSet& vegMembers =
    ((VEGReference*)itemExpr)->getVEG()->getAllValues();

  ValueId firstMemberId = vegMembers.init();
  vegMembers.next(firstMemberId);   // 1st call to next positions it on 1st member
  ItemExpr* firstMemberExpr = firstMemberId.getItemExpr();
  return (firstMemberExpr->getOperatorType() == ITM_INSTANTIATE_NULL);
}

QRColumnPtr QRDescGenerator::skipInstNull(ItemExpr* itemExpr)
{
  QRTRACER("QRDescGenerator::skipInstNull()");
  QRColumnPtr result = NULL;

  OperatorTypeEnum opType = itemExpr->getOperatorType();
  if (opType == ITM_VEG_REFERENCE)
  {
    const ValueIdSet& vegMembers =
              ((VEGReference*)itemExpr)->getVEG()->getAllValues();
    ValueId firstMemberId = vegMembers.init();
    vegMembers.next(firstMemberId);   // 1st call to next positions it on 1st member
    itemExpr = firstMemberId.getItemExpr();
    opType = itemExpr->getOperatorType();
  }

  if (opType == ITM_INSTANTIATE_NULL)
  {
  	// The instantiated null column is part of a join pred.
  	// Do not reference that join pred, and create a full QRColumn element instead.
  	setGenColumnRefs(FALSE);
    result = genQRColumn(itemExpr->child(0));
  	setGenColumnRefs(TRUE);
    NAString* idstr = new(mvqrHeap_) NAString(result->getID());
    result->setAndRegisterID(itemExpr->getValueId(), colTblIdHash_);  // Set the ID of the InstantiateNull function.
    colTblIdHash_.insert(idstr, result);
  }
  
  return result;
}

// Static nonmember helper function, called by QRDescGenerator::getExprTree()
// to add extra parameters to certain functions.
static void addFunctionParameters(ItemExpr* ie,
                                  NAMemory* heap,
                                  QRFunctionPtr function)
{
  QRTRACER("addFunctionParameters()");
  QRParameterPtr param = new(heap) QRParameter(ADD_MEMCHECK_ARGS(heap));

  switch (ie->getOperatorType())
    {
      case ITM_EXTRACT:
      case ITM_EXTRACT_ODBC:
        {
          Extract* extractFn = static_cast<Extract*>(ie);
          param->setName("extractField");
          param->setValue((Int32)extractFn->getExtractField());
          function->addHiddenParam(param);
        }
        break;

      case ITM_TRIM:
        {
          Trim* trimFn = static_cast<Trim*>(ie);
          param->setName("mode");
          param->setValue((Int32)trimFn->getTrimMode());
          function->addHiddenParam(param);
        }
        break;

      case ITM_TRANSLATE:
        {
          Translate* translateFn = static_cast<Translate*>(ie);
          param->setName("mapTableId");
          param->setValue((Int32)translateFn->getTranslateMapTableId());
          function->addHiddenParam(param);
        }
        break;

      case ITM_DATEFORMAT:
        {
          DateFormat* dateFormatFn = static_cast<DateFormat*>(ie);
          Int32 dateFormat = dateFormatFn->getExpDatetimeFormat();
          param->setName("dateFormat");
          param->setValue(dateFormat);
          function->addHiddenParam(param);
        }
        break;

      case ITM_COMP_ENCODE:
      case ITM_COMP_DECODE:
        {
          CompEncode* encodeFn = static_cast<CompEncode*>(ie);
          param->setName("descFlag");
          param->setValue(encodeFn->getDescFlag());
          function->addHiddenParam(param);

          param = new(heap) QRParameter(ADD_MEMCHECK_ARGS(heap));
          param->setName("caseInsensitiveEncode");
          param->setValue(encodeFn->getCaseInsensitiveEncode());
          function->addHiddenParam(param);

          param = new(heap) QRParameter(ADD_MEMCHECK_ARGS(heap));
          param->setName("encodedCollation");
          param->setValue(encodeFn->getEncodedCollation());
          function->addHiddenParam(param);

          param = new(heap) QRParameter(ADD_MEMCHECK_ARGS(heap));
          param->setName("collationType");
          param->setValue(encodeFn->getCollationType());
          function->addHiddenParam(param);
        }
        break;

      case ITM_FORMAT:
        {
          Format* formatFn = static_cast<Format*>(ie);
          param->setName("formatStr");
          param->setValue(formatFn->getFormatStr());
          function->addHiddenParam(param);

          param = new(heap) QRParameter(ADD_MEMCHECK_ARGS(heap));
          param->setName("formatType");
          param->setValue(formatFn->getFormatType());
          function->addHiddenParam(param);

          param = new(heap) QRParameter(ADD_MEMCHECK_ARGS(heap));
          param->setName("formatCharToDate");
          param->setValue(formatFn->getFormatCharToDate());
          function->addHiddenParam(param);
        }
        break;

      // We will hit this for OLAP and sequence functions for the time being.
      // These are not handled by MVQR yet, and the hidden parameters for these
      // functions have not been addressed.
      default:
        deletePtr(param);
        assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                           FALSE, QRDescriptorException,
			   "Unhandled function type in addFunctionParameters(): %d",
			   ie->getOperatorType());
        break;
    }
}

QRExplicitExprPtr QRDescGenerator::getExprTree(ItemExpr* itemExpr)
{
  QRTRACER("QRDescGenerator::getExprTree()");
  NABoolean valueWasSet=FALSE;

  if (itemExpr->getOperatorType() == ITM_RANGE_SPEC_FUNC)
  {
    RangeSpecRef* range = static_cast<RangeSpecRef*>(itemExpr);
    itemExpr = range->getRangeObject()->getRangeItemExpr();
  }
  else if (itemExpr->getOperatorType() == ITM_INSTANTIATE_NULL)
  {
    return getExprTree(itemExpr->child(0));
  }
  
  switch (itemExpr->getQRExprElem())
    {
      case QR::QRFunctionElem:
      case QR::QRFunctionWithParameters:
        {
          QRFunctionPtr function = 
                  new(mvqrHeap_) QRFunction(ADD_MEMCHECK_ARGS(mvqrHeap_));
          function->setID(itemExpr->getValueId());
          function->setFunctionName(itemExpr->getText());
          if (itemExpr->isAnAggregate())
          {
            Aggregate* aggrFunc = static_cast<Aggregate*>(itemExpr);
            function->setAggregateFunc(itemExpr->getOperatorType(), aggrFunc->isDistinct());
          }
          for (Int32 argInx=0; argInx<itemExpr->getArity(); argInx++)
            function->addArgument(getExprTree(itemExpr->child(argInx)));

          // Extra work for functions like ExtractOdbc that take non-ItemExpr
          // parameters.
          if (itemExpr->getQRExprElem() == QR::QRFunctionWithParameters)
            addFunctionParameters(itemExpr, mvqrHeap_, function);

          return function;
        }
        break;

      case QR::QRBinaryOperElem:
        {
          QRBinaryOperPtr binaryOper =
                  new(mvqrHeap_) QRBinaryOper(ADD_MEMCHECK_ARGS(mvqrHeap_));
          binaryOper->setID(itemExpr->getValueId());
          binaryOper->setOperator(itemExpr->getText());
          binaryOper->setFirstOperand(getExprTree(itemExpr->child(0)));
          binaryOper->setSecondOperand(getExprTree(itemExpr->child(1)));
          return binaryOper;
        }
        break;

      case QR::QRUnaryOperElem:
        {
          QRUnaryOperPtr unaryOper =
                  new(mvqrHeap_) QRUnaryOper(ADD_MEMCHECK_ARGS(mvqrHeap_));
          unaryOper->setID(itemExpr->getValueId());
          unaryOper->setOperator(itemExpr->getText());
          unaryOper->setOperand(getExprTree(itemExpr->child(0)));
          return unaryOper;
        }
        break;

      case QR::QRColumnElem:
        {
          QRElementPtr elem = NULL;

          if (isInstNull(itemExpr))
            return skipInstNull(itemExpr);
            
          // VEGReference will return QRColumnElem. If this is a vegref, use a
          // constant member of the veg if there is one. An expr with more than
          // one column won't be accepted by qms as a range pred.
          if (itemExpr->getOperatorType() == ITM_VEG_REFERENCE)
            {
              ValueId constVid = 
                  static_cast<VEGReference*>(itemExpr)->getVEG()->getAConstant(TRUE);
              if (constVid != NULL_VALUE_ID)
                return getExprTree(constVid.getItemExpr());
            }

          // If a vegref, it may have an instantiate_null as the first veg
          // member if outer joins are involved.
          elem = genQRColumn(itemExpr->getValueId());
          ElementType elemType = elem->getElementType();
          if (elemType == ET_Column)
            return elem->downCastToQRColumn();
          else if (elemType == ET_Function)
            return elem->downCastToQRFunction();
          else
            assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                               FALSE, QRDescriptorException,
			       "Unhandled element type returned from genQRColumn() in getExprTree(): %d",
			       elemType);
        }
        break;

      case QR::QRScalarValueElem:
        {
          ConstValue* constVal;
          OperatorTypeEnum op = itemExpr->getOperatorType();
          if (op == ITM_CACHE_PARAM)
            constVal = (static_cast<ConstantParameter*>(itemExpr))->getConstVal();
          else
            {
              assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                                 op == ITM_CONSTANT, QRDescriptorException,
                                 "In getExprTree(), expected ITM_CONSTANT but got %d",
                                 op);
              constVal = static_cast<ConstValue*>(itemExpr);
            }
          if (constVal->isNull())
            return new (mvqrHeap_) QRNullVal(ADD_MEMCHECK_ARGS(mvqrHeap_));

          const NAType* type = constVal->getType();
          QRScalarValuePtr scalar = NULL;
          switch (type->getTypeQualifier())
            {
              case NA_NUMERIC_TYPE:
                if (((NumericType*)type)->isExact())
                  {
                    scalar = new (mvqrHeap_) 
                                   QRNumericVal(ADD_MEMCHECK_ARGS(mvqrHeap_));
                    ((QRNumericValPtr)scalar)->setScale(istring(type->getScale()));
                  }
                else
                  scalar = new (mvqrHeap_) QRFloatVal(ADD_MEMCHECK_ARGS(mvqrHeap_));
                break;
              case NA_DATETIME_TYPE:
              case NA_INTERVAL_TYPE:
                scalar = new (mvqrHeap_) 
                                QRNumericVal(ADD_MEMCHECK_ARGS(mvqrHeap_));
                ((QRNumericValPtr)scalar)->setScale(istring(type->getScale()));
                break;
              case NA_CHARACTER_TYPE:
                if (((CharType*)type)->getBytesPerChar() == 1)
                  scalar = new (mvqrHeap_) QRStringVal(ADD_MEMCHECK_ARGS(mvqrHeap_));
                else
                  {
                    assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                                       ((CharType*)type)->getBytesPerChar() == 2, 
                                       QRDescriptorException,
                                       "Unhandled bytes-per-char: %d",
                                       ((CharType*)type)->getBytesPerChar());
                    scalar->setValue(constVal->getConstStr(FALSE));
                    valueWasSet = TRUE;
                  }
                break;
              default:
                QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                  "Unhandled data type: %d (%s)",
                  type->getTypeQualifier(), type->getTypeName().toCharStar());
                scalar = new (mvqrHeap_) QRStringVal(ADD_MEMCHECK_ARGS(mvqrHeap_));
                break;
            }
          if (scalar)
          {
            if (!valueWasSet)
              scalar->setValue(constVal->getText());
            scalar->setID(itemExpr->getValueId());
            
            if (isDumpMvMode())
            {
              // Add the "official" unparsed text of the expression as a sub-element.
              NAString unparsedText;
              itemExpr->unparse(unparsedText, OPTIMIZER_PHASE, QUERY_FORMAT);
              scalar->setSql(unparsedText);
            }            
          }
          return scalar;
        }
        break;

      case QR::QRNoElem:
      default:
        assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                           FALSE, QRDescriptorException,
			   "Unhandled ExprElement enum value: %d",
			   itemExpr->getQRExprElem());
        return NULL;
    }
  // make the compiler happy
  return NULL;
}  // getExprTree()

// -----------------------------------------------------------------------
QRExprPtr
QRDescGenerator::genQRExpr(ItemExpr* pExpr,
	                   NABoolean isResidual,
                           UInt32 joinPredId)
{
  QRExprPtr exprElement;
  QRTRACER("QRDescGenerator::genQRExpr()");
  NABoolean isNotProvided = FALSE;

  // Was this expression already generated?
  QRElementPtr existingElem = getElementForValueID('X', pExpr->getValueId());
  if (existingElem)
  {
    // Yes, just reference the other element.
    exprElement = new(mvqrHeap_) QRExpr(isResidual, ADD_MEMCHECK_ARGS(mvqrHeap_));
    exprElement->setRef(existingElem->getID());
    exprElement->setReferencedElement(existingElem);
    return exprElement;
  }

  Lng32 treeDepth = 0;
  Lng32 exprSize = pExpr->getTreeSize(treeDepth, 0);
  if (exprSize > maxExprSize_ || treeDepth > maxExprDepth_)
  {
    if (isQueryMode() || isDumpMvMode())
      isNotProvided = TRUE;
    else
      throw QRDescriptorException("Expression too complex");
  }
  
  // If the expression is a member of an equality set, it consists of a ref
  // attribute to the join pred for that equality set.
  if (joinPredId)
  {
    exprElement = new(mvqrHeap_) QRExpr(isResidual, ADD_MEMCHECK_ARGS(mvqrHeap_));
    exprElement->setRefFromInt(joinPredId, TRUE);
    return exprElement;
  }

  exprElement = new(mvqrHeap_) QRExpr(isResidual, ADD_MEMCHECK_ARGS(mvqrHeap_));
  exprElement->setAndRegisterID(pExpr->getValueId(), colTblIdHash_);

  if (isDumpMvMode())
  {
    // Add the "official" unparsed text of the expression as a sub-element.
    NAString unparsedText;
    pExpr->unparse(unparsedText, OPTIMIZER_PHASE, QUERY_FORMAT);
    if (unparsedText.first('%') != NA_NPOS || 
        unparsedText.first('&') != NA_NPOS    )
    {
    	// Can't handle special characters that cause XML parsing problems
    	// or sprintf() problems.
      isNotProvided = TRUE;
    }
    
    if (!isNotProvided)        
    {
      QRInfoPtr info = new QRInfo(ADD_MEMCHECK_ARGS(mvqrHeap_));
      info->setText(unparsedText);
      exprElement->setInfo(info);
    }
  }
  
  if (isNotProvided)
  {
    // This expression is either too big or using the '%' or '&' characters, 
    // which can cause problems during XML parsing or SQL generation, so we 
    // skip it and mark it as NotProvided for QMS. We just need to provide 
    // the expression's input columns.
    exprElement->setResult(QRElement::NotProvided);
    
    // Use a fake function element to hold the list of input columns.
    QRFunctionPtr fakeFunction = new(mvqrHeap_) QRFunction(ADD_MEMCHECK_ARGS(mvqrHeap_));
    fakeFunction->setID(pExpr->getValueId());
    fakeFunction->setFunctionName("fake function");

    ValueIdSet vegrefsInExpr;
    pExpr->findAll(ITM_VEG_REFERENCE, vegrefsInExpr, FALSE, FALSE);
    for (ValueId vegrefVid=vegrefsInExpr.init(); 
         vegrefsInExpr.next(vegrefVid); 
         vegrefsInExpr.advance(vegrefVid)) 
    {
      ItemExpr* itemExpr = vegrefVid.getItemExpr();
      QRColumnPtr col = genQRColumn(itemExpr->getValueId());      
      fakeFunction->addArgument(col);
    }
    exprElement->setExprRoot(fakeFunction);
  }
  else
  {
    // Generate the tree-structured representation of the expression.
    QRExplicitExprPtr treeExpr = getExprTree(pExpr);
    exprElement->setExprRoot(treeExpr);
  }
  
  return exprElement;
}  // genQRExpr()

// -----------------------------------------------------------------------
NABoolean
QRDescGenerator::normalizeColumnInExpression(NAString& pExprText,
					     ValueId   colvid,
					     short     paramIndex)
{
  QRTRACER("QRDescGenerator::normalizeColumnInExpression()");
  NABoolean bColFound = FALSE;
  NABoolean bFirst    = TRUE;

  NAString sReplacement = "@A";

  NAString sFQColName;
  sFQColName = ((BaseColumn *) colvid.getItemExpr())->getText();

  size_t dPos = 0;
  size_t dIndex = 0;
  NABoolean bDone = FALSE;
  while (!bDone) 
  {
    dIndex = pExprText.index(sFQColName, dPos, NAString::exact);
    if (dIndex != NA_NPOS) 
    {
      if (bFirst) 
      {
	sReplacement += istring(paramIndex);
	bColFound = TRUE;
	bFirst = FALSE;
      }
      pExprText.replace(dIndex, sFQColName.length(), sReplacement);
      dPos += sReplacement.length();
    }
    else 
    {
      bDone = TRUE;
    }
  }

  return bColFound;

}  // normalizeColumnInExpression()

void QRDescGenerator::markColumnsAsResidual(ValueIdSet& vegrefsInExpr)
{
  QRTRACER("QRDescGenerator::markColumnsAsResidual()");
  ValueIdSet baseColsInExpr;

  // Iterate over vegrefs in the expression. For each of those, find all the base
  // columns in its veg. Mark them all as being used in a residual predicate.
  // Avoid including base columns that are equated to a constant.
  for (ValueId vegrefVid=vegrefsInExpr.init(); 
       vegrefsInExpr.next(vegrefVid); 
       vegrefsInExpr.advance(vegrefVid)) 
    {
      ItemExpr* itemExpr = vegrefVid.getItemExpr();
      if (itemExpr->getOperatorType() == ITM_VEG_REFERENCE &&
          static_cast<VEGReference*>(itemExpr)->getVEG()->getAConstant()
                                      == NULL_VALUE_ID)
        vegrefVid.getItemExpr()->findAll(ITM_BASECOLUMN, baseColsInExpr, TRUE, TRUE);
      else if (itemExpr->getOperatorType() == ITM_BASECOLUMN)
        baseColsInExpr.addElement(vegrefVid);
    }

  // Have all the columns used in the residual predicate, now set their bits in
  // the residual pred bitmap.
  for (ValueId colVid=baseColsInExpr.init(); 
       baseColsInExpr.next(colVid); 
       baseColsInExpr.advance(colVid)) 
    {
      setPredBitmap(colVid, ET_ResidualPred); // mark use of resid pred on col
    }
}

// -----------------------------------------------------------------------
void QRDescGenerator::processResidualPredicate(ValueId predVid,
                                               QRJBBPtr jbbElement)
{
  QRTRACER("QRDescGenerator::processResidualPredicate()");
  ItemExpr* predItemExpr = predVid.getItemExpr();

  QRExprPtr residualPredExpr = genQRExpr(predItemExpr, TRUE);

  // Set bits in residual predicate bitmap for all columns referenced in the
  // expression.
  ValueIdSet vegrefsInExpr;
  predItemExpr->findAll(ITM_VEG_REFERENCE, vegrefsInExpr, FALSE, FALSE);
  markColumnsAsResidual(vegrefsInExpr);

  // Residual predicates can only be on Hub tables.
  jbbElement->getHub()->getResidualPredList()->addItem(residualPredExpr);
}

// -----------------------------------------------------------------------
void QRDescGenerator::processGroupBy(RelExpr*    groupByNode, 
                                     CANodeId    gbID,
                                     QRJBBPtr    jbbElement)
{
  if (groupByNode->getOperator() != REL_GROUPBY)
    return;

  QRTRACER("QRDescGenerator::processGroupBy()");

  GroupByAgg* groupBy = (GroupByAgg *)groupByNode;
  QRGroupByPtr groupByElement = new(mvqrHeap_) QRGroupBy(ADD_MEMCHECK_ARGS(mvqrHeap_));
  jbbElement->setGroupBy(groupByElement);
  QRElementPtr groupItem = NULL;

  ValueIdSet gbvis = groupBy->groupExpr();

  // Both the hub and extra-hub GroupBy can have the same ID since they are both
  // based on the same RelExpr node with the same NodeID. In QMS, the ID from the
  // hub GroupBy element will be the only one actually used.
  groupByElement->setID(gbID);

  for (ValueId gbvid = gbvis.init(); 
        gbvis.next(gbvid); 
	gbvis.advance(gbvid)) 
  {
    ItemExpr *pExpr = gbvid.getItemExpr();

    if (isInstNull(pExpr))
      groupItem = skipInstNull(pExpr);
    else if (pExpr->getOperatorType() == ITM_VEG_REFERENCE) 
      groupItem = genQRColumn(gbvid);
    else
    {
      // This is an expression.
      groupItem = genQRExpr(pExpr, FALSE);
    }
    groupByElement->getPrimaryList()->addElement(groupItem);
  }
}  // processGroupBy()

// -----------------------------------------------------------------------
void
QRDescGenerator::processOutputList(const ValueIdSet& normOutputs,
    				   QRJBBPtr jbbElement)
{
  QRTRACER("QRDescGenerator::processOutputList()");
  QROutputListPtr outputListElement = jbbElement->getOutputList();

  const ColumnDescList* mvColList = (isMvMode()
                                       ? relExpr_->getRETDesc()->getColumnList()
                                       : NULL);
  CollIndex mvNumCols = (mvColList ? mvColList->entries() : 0);
  CollIndex outInx, cdInx;
  ValueId cvid;

  for (cvid = normOutputs.init(), outInx = 0; 
       normOutputs.next(cvid); 
       normOutputs.advance(cvid), outInx++) 
  {
    ItemExpr *cvExpr = cvid.getItemExpr();
    QROutputPtr qro_p = new(mvqrHeap_)  QROutput(ADD_MEMCHECK_ARGS(mvqrHeap_));
    if (mvColList)
      {
        NABoolean found = FALSE;
        const OperatorTypeEnum cvExprOp = cvExpr->getOperatorType();
        for (cdInx=0; cdInx<mvNumCols && !found; cdInx++)
          {
            ValueId mvColVid = mvColList->at(cdInx)->getValueId();
            if (cvid == mvColVid)
              found = TRUE;

            // Note that we can't just break out of the for loop if 'found'
            // becomes true, because code after the loop makes assumptions
            // about the loop index value.
            if (!found)
              {
                ItemExpr* mvExpr = mvColVid.getItemExpr();
                OperatorTypeEnum op = mvExpr->getOperatorType();
                while (op == ITM_CAST || op == ITM_INSTANTIATE_NULL)
                  {
                    mvExpr = mvExpr->child(0);
                    op = mvExpr->getOperatorType();
                  }

                mvColVid = mvExpr->getValueId();
                if (cvid == mvColVid)
                  found = TRUE;
              }

            if (!found && cvExprOp == ITM_VEG_REFERENCE)
              {
                const ValueIdSet& cvVegVids = 
                        ((VEGReference*)cvExpr)->getVEG()->getAllValues();
                found = cvVegVids.contains(mvColVid);
              }
          }

        // If we did not find the ordinal number of the column, throw an 
        // exception now, so that an MV descriptor will not be created.
        Int32 cvid_i = cvid;
        assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                           found, QRDescriptorException,
                           "No matching MV column found for output item with "
                           "ValueId %d", cvid_i);
        // Set the ordinal position of the output item within the MV's select
        // list, so we can match it to the right column name once the column
        // names are known for sure.
        qro_p->setColPos((Int32)cdInx-1);
      }
    if (isInstNull(cvExpr))
      {
        QRColumnPtr col = skipInstNull(cvExpr);    
        qro_p->setOutputItem(col);
      }
    else if (cvExpr->getOperatorType() != ITM_VEG_REFERENCE) 
      {
        QRExprPtr exprElement = genQRExpr(cvExpr, FALSE);
        qro_p->setOutputItem(exprElement);
      }
    else 
      {
        QRElementPtr element = genQRColumn(cvid);
        qro_p->setOutputItem(element);
      }
    qro_p->setID(cvid);
    outputListElement->addItem(qro_p);
  }
}  // processOutputList()

// Nonmember function to write the graphviz specification of the join order
// predecessor requirements to the log.
  static void logJBBCPredecessorGraph(CANodeIdSet& jbbcs)
{
  NAString graphString(STMTHEAP);
  graphString += "\n#STARTJOINORDER\n"
                  "digraph \"\"\n"
                  "{\n"
                  "  label=\"Predecessor Relationships\"\n";
  char buf[200];
  
  for (CANodeId nodeId = jbbcs.init(); jbbcs.next(nodeId); jbbcs.advance(nodeId))
    {
      NodeAnalysis* nodeAnalysis = nodeId.getNodeAnalysis();
      if (!nodeAnalysis)
        continue;

      JBBC* jbbc = nodeAnalysis->getJBBC();
      if (!jbbc)
        continue;
        
      TableAnalysis* tableAnalysis = nodeAnalysis->getTableAnalysis();
      if (!tableAnalysis)
        continue;

      CorrName& corrName = tableAnalysis->getTableDesc()->getCorrNameObj();
      NAString nodeLabel = corrName.getCorrNameAsString();
      if (nodeLabel.length() == 0)
        nodeLabel = corrName.getQualifiedNameAsString();
      sprintf(buf, "  n%d [label=\"%s\"];\n", (CollIndex)nodeId, nodeLabel.data());
      graphString += buf;
      
      const CANodeIdSet& predecessors = jbbc->getPredecessorJBBCs();
      for (CANodeId predNodeId = predecessors.init();  
           predecessors.next(predNodeId); 
           predecessors.advance(predNodeId))
        {
          sprintf(buf, "  n%d->n%d;\n", (CollIndex)predNodeId, (CollIndex)nodeId);
          graphString += buf;
        }
    }
    
  graphString += "}\n"
                 "#ENDJOINORDER\n";
  QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG, graphString.data());
}

// Nonmember helper function for processJBBCList; returns array of join order
// group numbers indexed by node number.
static Int32* getJoinOrderInfo(const CANodeIdSet* const const_jbbcs, size_t& arrSize)
{
  CANodeIdSet jbbcs(*const_jbbcs);  // Need a copy we can change
  CANodeIdSet reached;
  CANodeId nodeId, predNodeId;
  Int32 currGroupOrderNum = 1;
  NABoolean foundOne = FALSE;
  Int32 numSkipped;

  // Find needed size for array.
  CollIndex maxNodeId = NULL_CA_ID;
  for (nodeId = jbbcs.init(); jbbcs.next(nodeId); jbbcs.advance(nodeId))
    {
      if (nodeId > maxNodeId)
        maxNodeId = nodeId;
    }

  // Allocate array of join order group numbers we will return.
  arrSize = maxNodeId + 1;  // assign output param
  Int32* joinOrderGroupNumbers = new(STMTHEAP) Int32[arrSize];

  if (QRLogger::isCategoryInDebug(CAT_SQL_COMP_QR_DESC_GEN))
    logJBBCPredecessorGraph(jbbcs);

  while (jbbcs.entries() > 0)
    {
      numSkipped = 0;
      for (nodeId = jbbcs.init(); jbbcs.next(nodeId); jbbcs.advance(nodeId))
        {
          NodeAnalysis* nodeAnalysis = nodeId.getNodeAnalysis();
          if (!nodeAnalysis)
            {
              numSkipped++; 
              continue;
            }

          JBBC* jbbc = nodeAnalysis->getJBBC();
          if (!jbbc)
            {
              numSkipped++; 
              continue;
            }

          NABoolean reachable = TRUE;
          const CANodeIdSet& predecessors = jbbc->getPredecessorJBBCs();
          for (predNodeId = predecessors.init();  
               reachable && predecessors.next(predNodeId); 
               predecessors.advance(predNodeId))
            {
              if (reached.containsThisId(predNodeId))
                continue;
              reachable = FALSE;  // haven't reached all its predecessors yet
            } // each predecessor

          if (reachable)
            {
              // Set the node's join order group number and add it to set of
              // nodes we have processed. It also needs to be removed from the
              // list of jbbcs, but we can't do that while we're iterating
              // over it. After the current pass, the set of reached nodes will
              // be removed from the jbbcs set, prior to the next loop over the
              // jbbcs.
              joinOrderGroupNumbers[nodeId] = currGroupOrderNum;
              reached += nodeId;
              foundOne = TRUE;
            }
        } // each jbbc

      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        foundOne || numSkipped == jbbcs.entries(),
                        QRDescriptorException,
                        "One or more nodes not reachable via required predecessor nodes");
      foundOne = FALSE;
      currGroupOrderNum++;
      jbbcs -= reached;
    } // while

  return joinOrderGroupNumbers;
}

// -----------------------------------------------------------------------
void QRDescGenerator::processJBBCList(CANodeIdSet* jbbcNodeIds,
                                      QRJBBPtr jbbElement,
                                      ValueIdSet &jbbOutputs,
                                      CANodeId& groupJbbcNodeId)
{
  QRTRACER("QRDescGenerator::processJBBCList()");
  QRTablePtr *qrtableArray = 0;
  qrtableArray = (QRTablePtr *)calloc(jbbcNodeIds->entries(),
                                      sizeof(QRTablePtr));
  short numTables = 0;

  // Get array of join order groups, indexed by node number. This array is
  // allocated in the function called, and we have to delete it.
  size_t joinOrderArrSize;
  Int32* joinOrders = getJoinOrderInfo(jbbcNodeIds, joinOrderArrSize);

  groupJbbcNodeId = NULL_CA_ID;  // Unless we find one
  
  // Create an array of JBBCs
  for (CANodeId nodeId = jbbcNodeIds->init();  
       jbbcNodeIds->next(nodeId); 
       jbbcNodeIds->advance(nodeId)) 
  {
    NodeAnalysis* nodeAnalysis = nodeId.getNodeAnalysis();
    if (!nodeAnalysis) // It is possible there is no node analysis, but I'm not
      continue;        //   sure why that is the case

    TableAnalysis* tableAnalysis = nodeAnalysis->getTableAnalysis();
    if (tableAnalysis)
      {
        // Table
        const NATable* tbl = tableAnalysis->getTableDesc()->getNATable();

        NAString tblName = tbl->getTableName().getQualifiedNameAsAnsiString();
        QRTablePtr tableElement = new(mvqrHeap_)
                                    QRTable(ADD_MEMCHECK_ARGS(mvqrHeap_));
        tableElement->setAndRegisterID(nodeId, colTblIdHash_);
        tableElement->setTableName(tblName);
        tableElement->setExtraHub(nodeAnalysis->isExtraHub());
        char sRedeftime[30];
        memset(sRedeftime, 0, sizeof(sRedeftime));
        convertInt64ToAscii(tbl->getRedefTime(), sRedeftime);
        tableElement->setTimestamp(NAString(sRedeftime));
        tableElement->setIsAnMV(tbl->isAnMV());
        tableElement->setNumCols(tbl->getNAColumnArray().entries());
        tableElement->setJoinOrder(joinOrders[nodeId]);
        if (nodeAnalysis->getJBBC()->parentIsLeftJoin())
          tableElement->setLOJParent(TRUE);
        if (isDumpMvMode())
        {
          const NAString& exposedName = tableAnalysis->getTableDesc()->getCorrNameObj().getExposedNameAsAnsiString();
          const NAString& corrName = strchr(exposedName, '.') ?
                                     tableAnalysis->getTableDesc()->getCorrNameObj().getQualifiedNameObj().getObjectName() :
                                     exposedName;
          tableElement->setCorrelationName(corrName);
        }

        processKeys(tableAnalysis, tableElement, jbbOutputs);

        qrtableArray[numTables++] = tableElement;
      }
    else
      {
        // Handle linked JBBs here.
        QRJBBPtr groupJbb = new(mvqrHeap_) QRJBB(genJBBid(), 
                                                 (CollIndex)NULL_CA_ID,
                                                 ADD_MEMCHECK_ARGS(mvqrHeap_));
        const JBBSubset* jbbSubset = nodeAnalysis->getOriginalExpr()
                                       ->getGroupAnalysis()->getLocalJBBView();
        if (jbbSubset)
          {
            groupJbb->setRefFromInt(jbbSubset->getJBB()->getJBBId());
            jbbElement->getHub()->getJbbcList()->addElement(groupJbb);
          }
        else if (nodeAnalysis->getOriginalExpr()->getOperatorType() == REL_GROUPBY)
          {
            groupJbbcNodeId = nodeId;
          }
        else
          {
            deletePtr(groupJbb);
            Int32 nodeIdVal = nodeId;
            assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                               FALSE, QRDescriptorException,
                               "Unsupported operator: %s",
                               nodeAnalysis->getOriginalExpr()->getText().data());
          }
      }
  }

  // Finished with join order array.
  NADELETEARRAY(joinOrders, joinOrderArrSize, Int32, STMTHEAP);

  // Sort the array by table name.
  if (numTables > 0) 
  {
    opt_qsort(qrtableArray, numTables, sizeof(QRTablePtr),
              //QRDescGenerator::cmpQRTableName);
              QRElement::cmpQRElement);
  }

  // Add the sorted tables to the descriptor.
  for (short iTableIndex = 0; iTableIndex < numTables; iTableIndex++) 
  {
    QRTablePtr tableElement = qrtableArray[iTableIndex];

    // Is this a hub or extra-hub table?
    if (tableElement->isExtraHub())
      jbbElement->getExtraHub()->addTable(tableElement);
    else
      jbbElement->getHub()->getJbbcList()->addElement(tableElement);
  }

  free(qrtableArray);

}  // processJBBCList()

// -----------------------------------------------------------------------
void QRDescGenerator::processKeys(TableAnalysis* tableAnalysis, 
                                  QRTablePtr     tableElement,
                                  ValueIdSet&    jbbOutputs)
{
  QRTRACER("QRDescGenerator::processKeys()");
  // Generate the content of the <Key> element using the clustering index.
  // The primary key serves this purpose if one is available, otherwise SYSKEY.
  ValueIdSet keyCols(tableAnalysis->getTableDesc()->getClusteringIndex()
                                                  ->getClusteringKeyCols());
  QRKeyPtr keyElement = NULL;

  if (isQueryMode())
    keyElement = new(mvqrHeap_) QRKey(ADD_MEMCHECK_ARGS(mvqrHeap_));

  //check if the key columns are covered by the jbb outputs
  NABoolean keyCovered = TRUE;

  for (ValueId vid=keyCols.init();
       keyCols.next(vid);
       keyCols.advance(vid))
  {
    if (isQueryMode())
    {
    QRElementPtr keyColumn = genQRColumn(vid);
    keyElement->addElement(keyColumn);
  }

    if (!jbbOutputs.containsTheGivenValue(vid))
      keyCovered = FALSE;
  }

  if (isQueryMode())
  tableElement->setKey(keyElement);

  if (keyCovered && (!isQueryMode()))
    tableElement->setIsKeyCovered(TRUE);
}

// -----------------------------------------------------------------------
void QRDescGenerator::getSingleTableJBBs(QRDescriptorPtr desc, RelExpr* expr)
{
  QRTRACER("QRDescGenerator::getSingleTableJBBs()");
  RelExpr* subtree;
  Scan* scanExpr;
  NodeAnalysis* nodeAnalysis;
  const NATable* naTable;

  // Traverse query tree, looking for table scans that are not part of a JBB.
  // We will treat these as single-JBBC JBBs for the purpose of query rewrite.
  char sRedeftime[30];
  Int32 arity = expr->getArity();
  for (Int32 i = 0; i < arity; i++)
  {
    subtree = expr->child(i).getPtr();
    OperatorType op = subtree->getOperator();

    // If the node is a GroupBy that is not part of a JBB (its GBAnalyis is
    // null), skip over it and look for a Scan node below. This can happen when
    // the GB is on a primary key and will eventually be discarded as unnecessary.
    // Use a loop in case of cascaded groupbys as in the case of "select distinct"
    // with a grouping query.
    while (op.match(REL_GROUPBY) &&
           static_cast<GroupByAgg*>(subtree)->getGBAnalysis() == NULL)
      {
        subtree = subtree->child(0).getPtr();
        op = subtree->getOperator();
      }

    if (op.match(REL_SCAN) &&
            (desc->getElementType() == ET_MVDescriptor ||  // always for MV desc
             qrNeededForTable(expr, subtree)))
    {
      scanExpr = static_cast<Scan*>(subtree);
      nodeAnalysis = scanExpr->getGroupAnalysis()->getNodeAnalysis();
      QRJBBPtr jbb = new(mvqrHeap_) QRJBB(genJBBid(),
                                          (CollIndex)(nodeAnalysis->getId()),
                                          ADD_MEMCHECK_ARGS(mvqrHeap_));
      desc->addJBB(jbb);
      QRTablePtr tableElement = new(mvqrHeap_) QRTable(ADD_MEMCHECK_ARGS(mvqrHeap_));
      naTable = scanExpr->getTableDesc()->getNATable();
      tableElement->setAndRegisterID(nodeAnalysis->getId(), colTblIdHash_);
      tableElement->setTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
      tableElement->setExtraHub(nodeAnalysis->isExtraHub());
      memset(sRedeftime, 0, sizeof(sRedeftime));
      convertInt64ToAscii(naTable->getRedefTime(), sRedeftime);
      tableElement->setTimestamp(NAString(sRedeftime));
      tableElement->setIsAnMV(naTable->isAnMV());
      tableElement->setNumCols(naTable->getNAColumnArray().entries());
      if (isDumpMvMode())
      {
        const NAString& exposedName = scanExpr->getTableDesc()->getCorrNameObj().getExposedNameAsAnsiString();
        const NAString& corrName = strchr(exposedName, '.') ?
                                   scanExpr->getTableDesc()->getCorrNameObj().getQualifiedNameObj().getObjectName() :
                                   exposedName;
        tableElement->setCorrelationName(corrName);
      }

      ValueIdSet jbbOutputs = nodeAnalysis->
                                getOriginalExpr()->
                                  getGroupAttr()->
                                    getCharacteristicOutputs();

      processKeys(nodeAnalysis->getTableAnalysis(), tableElement, jbbOutputs);

      if (tableElement->isExtraHub())
        jbb->getExtraHub()->addTable(tableElement);
      else
        jbb->getHub()->getJbbcList()->addElement(tableElement);

      if (expr->getOperator() == REL_GROUPBY)
        processGroupBy(expr, 0, jbb);

      // Use characteristic outputs of the parent if it is a groupby node.
      if (expr->getOperator().match(REL_GROUPBY))
        processOutputList(expr->getGroupAttr()->getCharacteristicOutputs(), jbb);
      else
        processOutputList(scanExpr->getGroupAttr()->getCharacteristicOutputs(), jbb);
    }
    else if (op.match(REL_UNARY_UPDATE))
    {
      // Need to create a qrtable for the update node and add its id to the hash
      // table to prevent an assertion failure when placing its preds in range
      // or residual bitmaps.
      QRTablePtr tableElement = new(mvqrHeap_) QRTable(ADD_MEMCHECK_ARGS(mvqrHeap_));
      Update* updExpr = static_cast<Update*>(subtree);
      CANodeId id = updExpr->getTableDesc()->getTableAnalysis()
                           ->getNodeAnalysis()->getId();
      tableElement->setAndRegisterID(id, colTblIdHash_);
    }
    // cut off recursion at root of JBB
    // assume outer joins are part of JBB
    //else if (!op.match(REL_ANY_JOIN) && !op.match(REL_MULTI_JOIN))
    else if (!op.match(REL_ANY_JOIN) && !op.match(REL_MULTI_JOIN) && !op.match(REL_GROUPBY))
    {
      getSingleTableJBBs(desc, subtree);
    }
  }
}  // getSingleTableJBBs

QRJBBPtr QRDescGenerator::createJbb(JBB* jbb)
{
  QRTRACER("QRDescGenerator::createJbb()");
  QRJBBPtr jbbElement = new(mvqrHeap_) QRJBB(jbb->getJBBId(), jbb,
                                             ADD_MEMCHECK_ARGS(mvqrHeap_));
  CANodeIdSet jbbcs = jbb->getJBBCs();

  ValueIdSet jbbOutputs;

  if (jbb->getGBAnalysis())
    jbbOutputs = jbb->getNormOutputs();
  else if (jbb->getJBBCs().entries() == 1)
    jbbOutputs = jbb->getJBBCs().getFirst().getNodeAnalysis()->
                   getOriginalExpr()->getGroupAttr()->
                     getCharacteristicOutputs();

  else
    jbbOutputs = jbb->getNormOutputs();   

  // JBBC List
  CANodeId groupJbbcNodeId = NULL_CA_ID;
  processJBBCList(&jbbcs, jbbElement, jbbOutputs, groupJbbcNodeId);

  // Predicates. GB node needed to find having pred on count(*), which is not
  // pushed down to a scan node.
  GBAnalysis* gbAnalysis = jbb->getGBAnalysis();

  // Group By
  // For some nested queries with GB that constitute a separate JBB, the JBB
  // does not have a GBAnalysis object. The GB original expr must be
  // obtained from the NodeAnalysis of the grouping node itself.
  // However the GroupByAgg* is derived, it must be saved in the QRJBB object
  // for the eventual call to processReferencingPreds() for the jbb.
  if (gbAnalysis != NULL)
    {
      processGroupBy(gbAnalysis->getOriginalGBExpr(),
                     gbAnalysis->getGroupingNodeId(),
                     jbbElement);
      jbbElement->setGbExpr(gbAnalysis->getOriginalGBExpr());
    }
  else if (groupJbbcNodeId != NULL_CA_ID)
    {
      RelExpr* relExpr = groupJbbcNodeId.getNodeAnalysis()->getOriginalExpr();
      assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_ERROR,
                          relExpr->getOperatorType() == REL_GROUPBY,
                          QRDescriptorException,
                          "Expected grouping JBBC, but op type was %d.",
                          relExpr->getOperatorType());
      processGroupBy(static_cast<GroupByAgg*>(relExpr),
                      groupJbbcNodeId, jbbElement);
      jbbElement->setGbExpr(static_cast<GroupByAgg*>(relExpr));
    }

  // Output List
  processOutputList(jbb->getNormOutputs(), jbbElement);

  return jbbElement;
}  // createJbb()

NABoolean QRDescGenerator::hasRewriteEnabledMVs(TableDesc* tableDesc)
{
  QRTRACER("QRDescGenerator::hasRewriteEnabledMVs()");
  const UsingMvInfoList& mvList = tableDesc->getNATable()->getMvsUsingMe();
  for (CollIndex i=0; i<mvList.entries(); i++)
    {
      if (mvList[i]->isRewriteEnabled())
        return TRUE;
    }

  return FALSE;
}

NABoolean QRDescGenerator::qrNeededForTable(RelExpr* parent, RelExpr* scanExpr)
{
  QRTRACER("QRDescGenerator::qrNeededForTable()");
  if (putAllJBBsInQD_)
    return TRUE;

  Scan* scan = static_cast<Scan*>(scanExpr);

  // If there are no MVs on the table, no rewrite is possible.
  if (!hasRewriteEnabledMVs(scan->getTableDesc()))
    {
      QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG,
        "Omitting single table JBB %s from query descriptor -- no "
                  "rewrite-enabled MVs are defined on it.",
        scan->getTableDesc()->getNATable()->getTableName().getObjectName().data());
      return FALSE;
    }

  return TRUE;
}

NABoolean QRDescGenerator::qrNeededForJBB(JBB* jbb)
{
  QRTRACER("QRDescGenerator::qrNeededForJBB()");
  if (putAllJBBsInQD_)
    return TRUE;

  NodeAnalysis* nodeAnalysis;
  TableAnalysis* tableAnalysis;
  CANodeIdSet jbbcs = jbb->getJBBCs();

  // Count the JBBCs that have MVs defined on them. We need at least two, or
  // one plus a Group By.
  for (CANodeId jbbc = jbbcs.init();
       jbbcs.next(jbbc);
       jbbcs.advance(jbbc))
    {
      nodeAnalysis = jbbc.getNodeAnalysis();
      if (!nodeAnalysis->isExtraHub())  // Only check hub tables.
        {
          tableAnalysis = nodeAnalysis->getTableAnalysis();
          if (tableAnalysis && hasRewriteEnabledMVs(tableAnalysis->getTableDesc()))
            return TRUE;  // found a base table that uses an mv
        }
    }

  return FALSE;
}

// See if a jbbsubset uses any semijoins or TSJs. This supports a temporary
// restriction that is checked in processJBBs() below. Since the need for this
// check is temporary, we make it through this local nonmember function instead
// of further complicating the JBBSubsetAnalysis class with a member function.
static NABoolean noSemiOrTSJ(JBBSubsetAnalysis* jbbSubsetAnalysis)
{
  CANodeId node;
  const CANodeIdSet& jbbcs = jbbSubsetAnalysis->getJBBCs();
  
  for (node = jbbcs.init();  jbbcs.next(node); jbbcs.advance(node))
    {
      Join* parentJoin = node.getNodeAnalysis()->getJBBC()->getOriginalParentJoin();
      if (parentJoin)
        {
          OperatorType op = parentJoin->getOperator();
          if (op.match(REL_ANY_SEMIJOIN) || op.match(REL_ANY_TSJ))
            return FALSE;
        }
    }
    
  return TRUE;  // no semijoins or TSJs
}

NABoolean QRDescGenerator::processJBBs(QRDescriptorPtr descPtr,
                                       QueryAnalysis* qa)
{
  QRTRACER("QRDescGenerator::processJBBs()");

  // This is a reference to the descriptor's list of JBBs, to which will
  // first be added the pseudo-JBBs we create for table scans that are not
  // part of any JBB, and later in this function, the actual Analyzer JBBs.
  const NAPtrList<QRJBBPtr>& qrJbbList = descPtr->getJbbList();

  if (isDumpMvMode())
  {
    // Don't bother with single table queries in WA mode.
    const CANodeIdSet& allTables = qa->getTables();
    assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                      allTables.entries() > 1,
                      QRDescriptorException,
                      "This query has a single table.");
  }

  // Look for table scans that are not part of a JBB, and create pseudo-JBBs
  // for them. Note that these will be reflected in the contents of qrJbbList
  // upon return.
  getSingleTableJBBs(descPtr, relExpr_);

  // Initialize return value to false if previous step created no JBBs.
  NABoolean jbbElemsWereCreated = !qrJbbList.isEmpty();
  
  // Check if any single-table (pseudo) JBBs use an MV. If so, we can skip this
  // step for any actual JBBs later in this function.
  // If the cqd MVQR_ALL_JBBS_IN_QD is ON (putAllJBBsInQD_), we don't need to
  // check this, the descriptor is created regardless of MV usage.
  NABoolean usesEnabledMVs = FALSE;
  CollIndex i;
  if (jbbElemsWereCreated && !putAllJBBsInQD_)
    for (i = 0; !usesEnabledMVs && i < qrJbbList.entries(); i++)
      {
        // getNodeId() returns a CollIndex, so we have to cast it to CANodeId.
        NodeAnalysis* nodeAnalysis =
              static_cast<CANodeId>(qrJbbList[i]->getNodeId()).getNodeAnalysis();
        if (!nodeAnalysis->isExtraHub() &&
            hasRewriteEnabledMVs(nodeAnalysis->getTableAnalysis()->getTableDesc()))
          {
            usesEnabledMVs = TRUE;  // table is used by an MV
          }
      }

  const ARRAY(JBB*)& jbbArray = qa->getJBBs();
  CollIndex remainingJBBs = jbbArray.entries();

  if (isDumpMvMode())
  {
    Int32 totalJBBs = remainingJBBs + descPtr->getJbbList().entries();
    assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                       totalJBBs == 1,
                       QRDescriptorException,
                       "This query has %d JBBs.", totalJBBs);
  }

  // For a query descriptor, make sure some table involved is used by an MV
  // before going further. We used to leave out individual JBBs on this basis,
  // but there are typically inter-JBB column references that can result in an
  // exception being thrown if some JBBs are not processed (see bug 2502).
  // If the cqd MVQR_ALL_JBBS_IN_QD is ON (putAllJBBsInQD_), we don't need to
  // check this, the descriptor is created regardless of MV usage.
  if (!usesEnabledMVs && !putAllJBBsInQD_ && isQueryMode())
    {
      for (i = 0; !usesEnabledMVs && remainingJBBs > 0; i++)
        {
          if (jbbArray.used(i))
            { 
              remainingJBBs--;
              currentJBB_ = jbbArray[i];
              usesEnabledMVs = qrNeededForJBB(currentJBB_);
            }
        }

      if (!usesEnabledMVs)
        {
          QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG,
              "Query descriptor not generated: no JBBC of any JBB is used in "
              "a rewrite-enabled MV.");
          return FALSE;
        }
    }

  // Now revisit each JBB and create a descriptor element for it.
  remainingJBBs = jbbArray.entries();
  for (i = 0; remainingJBBs > 0; i++)
    {
      if (jbbArray.used(i))
        { 
          remainingJBBs--;
          currentJBB_ = jbbArray[i];
          // Semijoins and TSJs not handled yet.
          JBBSubsetAnalysis* jbbSubsetAnalysis =
                  currentJBB_->getMainJBBSubset().getJBBSubsetAnalysis();
          if (noSemiOrTSJ(jbbSubsetAnalysis))
            {
              QRJBBPtr jbb = createJbb(currentJBB_);
              jbbElemsWereCreated = TRUE;
              descPtr->addJBB(jbb);
            }
          else
            {
              QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG,
                "Omitting JBB #%d from descriptor because it contains "
                "semijoins, anti-semijoins, or TSJs", currentJBB_->getJBBId());

              if (isDumpMvMode())
                {
                  assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                                   FALSE,
                                   QRDescriptorException,
                                   "SemiJoins are not supported yet.");
                }
            }
        }
    }

  // The processing of predicates is deferred until after the descriptor
  // elements for all JBBs have been created. This avoids the problem where
  // a predicate refers to a column for which the table hasn't been processed,
  // which leads to a failure in getting the table's range or residual pred
  // bitmap because it hasn't been set up yet.
  QRJBBPtr qrJbbPtr = NULL;
  for (CollIndex qrJbbInx=0; qrJbbInx<qrJbbList.entries(); qrJbbInx++)
    {
      qrJbbPtr = qrJbbList[qrJbbInx];
      currentJBB_ = qrJbbPtr->getJBB();
      if (currentJBB_)
        {
          CANodeIdSet jbbcs = currentJBB_->getJBBCs();
          processReferencingPreds(&jbbcs,
                                  qrJbbPtr->getGbExpr(),
                                  qrJbbPtr);
        }
      else
        {
          CANodeId nodeId = (CANodeId)(qrJbbPtr->getNodeId());
          assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                             nodeId != NULL_CA_ID,
                             QRDescriptorException,
                             "QRJBB at position %d in list had neither a JBB* nor a node id",
                             qrJbbInx);
          CANodeIdSet nodes;
          nodes.addElement(nodeId);
          RelExpr* expr = nodeId.getNodeAnalysis()->getOriginalExpr();
          processReferencingPreds(&nodes,
                                  expr->getOperator() == REL_GROUPBY ? expr : NULL,
                                  qrJbbPtr);
        }
    }

  currentJBB_ = NULL;  // No longer processing a specific JBB

  // Now that all predicates have been processed, the range predicates we have
  // kept in a hash table are guaranteed to be complete, and can be added to
  // the appropriate range predicate list.
  if (jbbElemsWereCreated)
    addRangePredicates();

  // Return true if one or more JBB elements were found. Returning false
  // prevents a descriptor from being generated.
  return jbbElemsWereCreated;
}

void QRDescGenerator::logColumnBitmap(QRTablePtr table,
                                      const XMLBitmap& bitmap,
                                      ElementType predType)
{
  // Exit immediately if logging is not in DEBUG level.
  if (!QRLogger::isCategoryInDebug(CAT_SQL_COMP_QR_DESC_GEN))
    return;
    
  QRTRACER("QRDescGenerator::logColumnBitmap()");
  // Log which bitmap this is.
  if (predType == ET_RangePred)
  {
    QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG,
      "For table %s, the following columns have range predicates:",
      table->getTableName().data());
  }
  else if (predType == ET_ResidualPred)
  {
    QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG,
      "For table %s, the following columns have residual predicates:",
      table->getTableName().data());
  }
  else
  {
    QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG,
      "Table %s has these columns set in bitmap of element type %d:",
               table->getTableName().data(), predType);
  }
                       
  // If nothing set, log a message noting that and return.
  if (bitmap.isEmpty())
    {
      QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG, "    <none>");
      return;
    }    
                       
  // At least one column is marked in the bitmap. List the ones that are.
  CANodeId tblNodeId = table->getIDNum();
  const NAColumnArray& colsInTable =
        tblNodeId.getNodeAnalysis()->getTableAnalysis()->getTableDesc()
                                   ->getNATable()->getNAColumnArray();
  assertLogAndThrow2(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                     table->getNumCols() == colsInTable.entries(),
                     QRDescriptorException,
                     "logColumnBitmap() expected %d columns, but found %d in list",
                     table->getNumCols(), colsInTable.entries());
                
  for (CollIndex i=0; i<colsInTable.entries(); i++)
    {
      if (bitmap.testBit(i))
      {
        QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_DEBUG,
          "    %s", colsInTable[i]->getColName().data());
      }
      
    }
}  // logColumnBitmap()

QRQueryDescriptorPtr QRDescGenerator::createQueryDescriptor(QueryAnalysis* qa,
                                                            RelExpr* expr)
{
  QRTRACER("QRDescGenerator::createQueryDescriptor()");
  QRQueryDescriptorPtr queryDesc;
  
  queryDesc = new (mvqrHeap_) QRQueryDescriptor(ADD_MEMCHECK_ARGS(mvqrHeap_));
  descriptorType_ = ET_QueryDescriptor;
  relExpr_ = expr;
	
  // Initialize the return value to null, and leave it null if no JBBs are
  // present.
  if (!processJBBs(queryDesc, qa))
  {
    deletePtr(queryDesc);
    queryDesc = NULL;
    return NULL;
  }
 
  queryDesc->setVersion(createVersionElement());
    
  NAString mvAgeValue;
  CmpCommon::getDefault(MV_AGE, mvAgeValue, 0);
  queryDesc->getMisc()->setMVAge(mvAgeValue);
    
  Int32 level = CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL);
  queryDesc->getMisc()->setRewriteLevel((MvqrRewriteLevel)level);
  
  return queryDesc;
}  // createQueryDescriptor()

QRMVDescriptorPtr QRDescGenerator::createMvDescriptor(QueryAnalysis* qa,
                                                      RelExpr* expr)
{
  QRTRACER("QRDescGenerator::createMvDescriptor()");
  QRMVDescriptorPtr mvDesc = new(mvqrHeap_) QRMVDescriptor(ADD_MEMCHECK_ARGS(mvqrHeap_));
  descriptorType_ = ET_MVDescriptor;
  relExpr_ = expr;

  if (processJBBs(mvDesc, qa))
    {
      mvDesc->setVersion(createVersionElement());
      return mvDesc;
    }
  else
    {
      deletePtr(mvDesc);
      return NULL;
    }
}  // createMvDescriptor() 

QRVersionPtr QRDescGenerator::createVersionElement()
{
  QRTRACER("QRDescGenerator::createVersionElement()");
  NAString versionString(QR::CURRENT_VERSION);
  QRVersionPtr descVersion = new (mvqrHeap_) QRVersion(ADD_MEMCHECK_ARGS(mvqrHeap_));
  descVersion->setVersionString(versionString);

  return descVersion;
}

XMLString* QRDescGenerator::createXmlText(QRElementPtr desc)
{
  QRTRACER("QRDescGenerator::createXmlText()");
  XMLString* xmlText = NULL;
  
//  Formatted XML takes much more space, especially when very large 
//  expressions are used.
//  if (isDumpMvMode())
//    bFormatted_ = FALSE;
  if (bFormatted_) 
    xmlText = (XMLFormattedString *) new(mvqrHeap_) XMLFormattedString(mvqrHeap_);
  else 
    xmlText = new (mvqrHeap_) XMLString(mvqrHeap_);

  desc->toXML(*xmlText);
            
  return xmlText;
}

// Sort the join columns, distinguish between hub vs. extra-hub, and add to
// result.
void QRDescGenerator::addJoinPred(QRJBBPtr jbbElem,
                                  QRElementPtr* qrElemArray,
                                  Int32* idArray,
                                  Int32 eqCount,
                                  UInt32& hubJoinPredId)
{
  QRTRACER("QRDescGenerator::addJoinPred()");
  QRJoinPredPtr hubJoinPred = NULL;
  QRJoinPredPtr extraHubJoinPred = NULL;

  // Sort the list of columns and expressions.
  if (bSortJoinPredicateCols_) 
    opt_qsort(qrElemArray, eqCount, sizeof(QRElementPtr),
              QRElement::cmpQRElement);

  // Assign each element in the sorted array to either a hub or extrahub join
  // pred. The id of the join pred is derived from that of the first element
  // added to it.
  for (Int32 i=0; i<eqCount; i++)
    {
      if (qrElemArray[i]->isExtraHub())
        {
          if (!extraHubJoinPred)
            {
              extraHubJoinPred =
                    new (mvqrHeap_) QRJoinPred(ADD_MEMCHECK_ARGS(mvqrHeap_));
              QRValueId id(idArray[i]);
              EqualitySet* eqSet = vegsUsedHash_.getFirstValue(&id);
              if (eqSet && eqSet->getJoinPredId())
                extraHubJoinPred->setAndRegisterID(eqSet->getJoinPredId(),
                                                   colTblIdHash_);
              else
                extraHubJoinPred->setAndRegisterID(idArray[i], colTblIdHash_);
            }
          extraHubJoinPred->addElement(qrElemArray[i]);
          colToJoinPredMap_.addMapEntry(idArray[i],
                                        qrElemArray[i]->getReferencedElement()->getIDNum());
        }
      else
        {
          if (!hubJoinPred)
            {
              // hubJoinPredId will have been set to the value id of a veg pred
              // if the equality set was derived from one.
              if (hubJoinPredId == NULL_VALUE_ID)
                hubJoinPredId = idArray[i];  // returned to caller
              hubJoinPred =
                    new (mvqrHeap_) QRJoinPred(ADD_MEMCHECK_ARGS(mvqrHeap_));
              hubJoinPred->setAndRegisterID(hubJoinPredId, colTblIdHash_);
            }
          hubJoinPred->addElement(qrElemArray[i]);
          colToJoinPredMap_.addMapEntry(hubJoinPredId,
                                        qrElemArray[i]->getReferencedElement()->getIDNum());
        }
    }

  // Now lets look at the result and figure out how to insert it.
  QRJoinPredListPtr hubJoinPredList = jbbElem->getHub()->getJoinPredList();
  QRJoinPredListPtr extraHubJoinPredList = jbbElem->getExtraHub()->getJoinPredList();

  if (hubJoinPred == NULL)
    {
      // Insert the extra-hub join pred to the JBB.
      if (!extraHubJoinPred->isRedundant())
        extraHubJoinPredList->addItem(extraHubJoinPred);
      else
        deletePtr(extraHubJoinPred);
    }
  else if (hubJoinPred->entries() < 2)
    {
      // Since we have only a single hub member and we know there are two or more
      // members, there must be at least one extra-hub member.
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        extraHubJoinPred, QRDescriptorException,
                        "In addJoinPred(): single hub member and no extra-hub");

      // When there is only one entry in the hub list, we don't add the list to
      // hubJoinPredList; instead we add the single hub member to extraHubJoinPred.
      const QRElementPtr hubItem = hubJoinPred->getEqualityListElement(0);
      extraHubJoinPred->addElement(hubItem); // @ZX -- needs to be in order
      hubJoinPred->removeItem(hubItem);
      deletePtr(hubJoinPred);

      // Insert the extra-hub join pred to the JBB.
      if (!extraHubJoinPred->isRedundant())
        extraHubJoinPredList->addItem(extraHubJoinPred);
      else
        deletePtr(extraHubJoinPred);
    }
  else
    {
      // The hub list has at least 2 columns - insert it.
      if (!hubJoinPred->isRedundant())
      hubJoinPredList->addItem(hubJoinPred);
      else
        deletePtr(hubJoinPred);

      // extraHubJoinPred is only NULL if there are no extra-hub tables among the
      // tables represented in the join pred.
      if (extraHubJoinPred)
        {
	  // The extra-hub list has at least 1 column in it. 
	  // Insert it too, after adding a ref to the hub join pred.
          QRJoinPredPtr hubJoinPredRef = 
                    new(mvqrHeap_) QRJoinPred(ADD_MEMCHECK_ARGS(mvqrHeap_));
          hubJoinPredRef->setRefFromInt(hubJoinPred->getIDNum());
          extraHubJoinPred->addElement(hubJoinPredRef);
          if (!extraHubJoinPred->isRedundant())
            extraHubJoinPredList->addItem(extraHubJoinPred);
          else
            deletePtr(extraHubJoinPred);
        }
    }
} // addJoinPred()

// This function is part of the temporary restriction on a range predicate that
// disallows referring to the same column more than once in an expression that
// is the subject of a range predicate.
static void getLeafValueIds(ItemExpr* itemExpr, ValueIdList &lv)
{
  QRTRACER("getLeafValueIds()");
  Int32 nc = itemExpr->getArity();

  // if this is a leaf node, add its value id
  if (nc == 0)
    {
      lv.insertSet(itemExpr->getValueId());
    }
  else
    {
      // else add the leaf value ids of all the children
      for (Lng32 i = 0; i < (Lng32)nc; i++)
	{
	  getLeafValueIds(itemExpr->child(i), lv);
	}
    }
}

// This function is temporary, and will be used as long as the restriction
// disallowing >1 reference to the same column in a range predicate persists.
// When this restriction is lifted, remove this function and uncomment the one
// with the same name immediately below it. The present solution uses a list
// instead of a set so that use of the same column more than once can be detected.
// The static function getLeafValueIds() defined immediately above here can be
// removed at the same time this function is.
CANodeId QRDescGenerator::getExprNode(ItemExpr* itemExpr)
{
  QRTRACER("QRDescGenerator::getExprNode()");

  // Find and return the expression's containing node, or NULL_CA_ID if it is
  // a multi-node expression.
  //
  // There is currently a restriction that keeps a single node expression from
  // being used with a range predicate if it references more than one column,
  // or even the same column more than once. So for now, NULL_CA_ID is returned
  // in this case even if there is only a single node.
  ValueIdList vids;
  NABoolean isMultiNode = FALSE;
  CANodeId exprNodeId = NULL_CA_ID;
  CANodeId itemNodeId;
  ItemExpr* leafItemExpr;
  ValueId vid;
  Int32 inputCount = 0;

  getLeafValueIds(itemExpr, vids);

  for (CollIndex i=0; i<vids.entries(); i++)
    {
      vid = vids[i];
      leafItemExpr = vid.getItemExpr();
      switch (leafItemExpr->getOperatorType())
        {
          case ITM_VEG_REFERENCE:
            // If a veg contains a constant, it will be used in the range expr,
            // so only count it if it contains no constants.
            if (static_cast<VEGReference*>(leafItemExpr)->getVEG()->getAConstant(TRUE)
                                      == NULL_VALUE_ID)
              inputCount++;
            break;

          //case ITM_CONSTANT:
          case ITM_REFERENCE:
          case ITM_BASECOLUMN:
          case ITM_INDEXCOLUMN:
          //case ITM_HOSTVAR:
          //case ITM_DYN_PARAM:
          case ITM_SEL_INDEX:
          case ITM_VALUEIDREF:
          case ITM_VALUEIDUNION:
          case ITM_VEG:
          case ITM_VEG_PREDICATE:
          //case ITM_DEFAULT_SPECIFICATION:
          //case ITM_SAMPLE_VALUE:
          //case ITM_CACHE_PARAM:
            inputCount++;
            break;
          default:
            break;
        }

      itemNodeId = getNodeId(vid);
      if (itemNodeId != NULL_CA_ID)
        {
          if (exprNodeId == NULL_CA_ID)
            exprNodeId = itemNodeId;
          else if (itemNodeId != exprNodeId)
            isMultiNode = TRUE;
        }
    }

  if (exprNodeId)
    return (isMultiNode || inputCount != 1)
              ? NULL_CA_ID
              : exprNodeId;
  else if (itemExpr->containsAnAggregate())
    {
      // exprNodeId will be NULL_CA_ID if only count(*) is used.
      if (!currentJBB_)
        // Called from Normalizer. Analysis not done yet, have to settle for
        // residual pred.
        return NULL_CA_ID;
      else
        {
          assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                            currentJBB_->getGBAnalysis(), QRDescriptorException,
                            "No GBAnalysis for JBB %d, although an aggfn was used",
                            currentJBB_->getJBBId());
          return currentJBB_->getGBAnalysis()->getGroupingNodeId();
        }
    }
  else
    {
      NAString predText;
      itemExpr->unparse(predText);
      QRLogger::log(CAT_SQL_COMP_QR_DESC_GEN, LL_WARN,
          "Predicate encountered with no columns and no count(*), will treat "
          "as residual predicate -- %s", predText.data());
      return NULL_CA_ID;
    }
}

// DON'T REMOVE THIS CODE -- it will be restored when the temporary restriction
//                           described at the top of the previous function is
//                           eliminated.
//CANodeId QRDescGenerator::getExprNode(ItemExpr* itemExpr)
//{
//  QRTRACER("getExprNode");
//
//  // Find and return the expression's containing node, or NULL_CA_ID if it is
//  // a multi-node expression.
//  //
//  // There is currently a restriction that keeps a single node expression from
//  // being used with a range predicate if it references more than one column,
//  // So for now, NULL_CA_ID is returned in this case even if there is only a
//  // single node.
//  ValueIdSet vids;
//  NABoolean isMultiNode = FALSE;
//  CANodeId exprNodeId = NULL_CA_ID;
//  CANodeId itemNodeId;
//  ItemExpr* leafItemExpr;
//  int inputCount = 0;
//
//  itemExpr->getLeafValueIds(vids);
//
//  for (ValueId vid=vids.init();
//       !isMultiNode && inputCount < 2 && vids.next(vid);
//       vids.advance(vid))
//    {
//      leafItemExpr = vid.getItemExpr();
//      switch (leafItemExpr->getOperatorType())
//        {
//          //case ITM_CONSTANT:
//          case ITM_REFERENCE:
//          case ITM_BASECOLUMN:
//          case ITM_INDEXCOLUMN:
//          //case ITM_HOSTVAR:
//          //case ITM_DYN_PARAM:
//          case ITM_SEL_INDEX:
//          case ITM_VALUEIDREF:
//          case ITM_VALUEIDUNION:
//          case ITM_VEG:
//          case ITM_VEG_PREDICATE:
//          case ITM_VEG_REFERENCE:
//          //case ITM_DEFAULT_SPECIFICATION:
//          //case ITM_SAMPLE_VALUE:
//          //case ITM_CACHE_PARAM:
//            inputCount++;
//            break;
//          default:
//            break;
//        }
//      itemNodeId = getNodeId(vid);
//      if (itemNodeId != NULL_CA_ID)
//        {
//          if (exprNodeId == NULL_CA_ID)
//            exprNodeId = itemNodeId;
//          else if (itemNodeId != exprNodeId)
//            isMultiNode = TRUE;
//        }
//    }
//    
//  return (isMultiNode || inputCount != 1)
//            ? NULL_CA_ID
//            : exprNodeId;
//}
 
void QRDescGenerator::processEqualitySet(QRJBBPtr jbbElem,
                                         EqualitySet& eqSet)
{
  QRTRACER("QRDescGenerator::processEqualitySet()");
  CollIndex i;
  ItemExpr* itemExpr;
  OperatorTypeEnum op;
  CANodeId itemNodeId;

  // Keep track of the CA nodes represented by the join pred members. Only one
  // col/expr per node is placed in the join pred.
  CANodeIdSet joinPredNodes;
  
  // Retain a ptr to the first item expression added to the list of join pred
  // members. If it turns out to be the only one, there is no join and we add
  // it to the list of residual pred operands.
  ItemExpr* firstJoinPredItem = NULL;

  // List to store members of the equality set that are not part of a JoinPred.
  // This includes expressions involving more than a single node, cols/exprs
  // other than the first from each represented node, dynamic params, etc.
  // When a join pred is derived from a subset of the members of the equality
  // set, each remaining equality set member will appear in the query descriptor
  // either as
  //   1) a residual predicate equated to a reference to the join pred created
  //      from the equality set. This is the case when
  //        a) there is no constant in the equality set
  //        b) there is a constant, but the given eq set member is a multinode
  //           expression
  //   2) a range predicate equated to the constant member of the eq set, except
  //      for a multinode expression
  // If no join pred comes out of the equality set and there is no constant, we
  // will form a chain of equality residual predicates involving all members of
  // the eq set.
  NAList<ItemExpr*> nonJoinPredOperands(mvqrHeap_);

  // This keeps track of which members can NOT be used in a range predicate
  // (multi-node expressions).
  NABitVector rangeIneligible(mvqrHeap_);

  // Stores the constant value, if any (max of 1 is possible), from the equality
  // set. Used to create a range pred on the join columns.
  ConstValue* constItem = NULL;

  CollIndex joinPredElemCount = 0;
  CollIndex eqCount = eqSet.entries();
  NAString exprText;

  // Array to put elements in so they can be sorted, and array of corresponding
  // ids that can be used for the created join preds (for hub and extrahub).
  QRElementPtr* qrElemArray = new(mvqrHeap_) QRElementPtr[eqCount];
  Int32* idArray = new(mvqrHeap_) Int32[eqCount];

  for (i=0; i<eqCount; i++)
    {
      itemExpr = eqSet[i];
      op = itemExpr->getOperatorType();
      if (op == ITM_BASECOLUMN)
        {
          const TableAnalysis* tableAnalysis =
                   ((BaseColumn*)itemExpr)->getTableDesc()->getTableAnalysis();
          assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                            tableAnalysis, QRDescriptorException,
                            "TableDesc has null table analysis");
          itemNodeId = tableAnalysis->getNodeAnalysis()->getId();
          //itemNodeId = ((BaseColumn*)itemExpr)->getTableDesc()->getTableAnalysis()
          //                                    ->getNodeAnalysis()->getId();
          if (!joinPredNodes.containsThisId(itemNodeId))
            {
              joinPredNodes.insert(itemNodeId);
              if (joinPredElemCount == 0)
                firstJoinPredItem = itemExpr;
              else
                {
                  // Avoid call to genQRColumn until we're sure there is more than
                  // one join pred element. Otherwise we'll have to retract it,
                  // and the created col elem will not be used, but will be
                  // referenced by subsequent occurrences of the column.
                  if (joinPredElemCount == 1)
                    {
                      if (isInstNull(firstJoinPredItem))
                        qrElemArray[0] = skipInstNull(firstJoinPredItem);
                      else if (firstJoinPredItem->getOperatorType() == ITM_BASECOLUMN)
                        qrElemArray[0] = 
                            genQRColumn(firstJoinPredItem->getValueId());
                      else
                        qrElemArray[0] = genQRExpr(firstJoinPredItem, FALSE);
                      idArray[0] = firstJoinPredItem->getValueId();
                    }
                  qrElemArray[joinPredElemCount] = 
                          genQRColumn(itemExpr->getValueId());
                  idArray[joinPredElemCount] = itemExpr->getValueId();
                }
              joinPredElemCount++;
            }
          else
            nonJoinPredOperands.insert(itemExpr);
        }
      else if (op == ITM_CONSTANT)
        constItem = static_cast<ConstValue*>(itemExpr);
      else if (op == ITM_CACHE_PARAM)
        {
          // Substitute an itemexpr representing the constant value underlying
          // the parameter.
          assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                            !constItem, QRDescriptorException, 
                            "Equality set contains more than one constant");
          constItem = (static_cast<ConstantParameter*>(itemExpr))->getConstVal();
        }
      else if (itemExpr->doesExprEvaluateToConstant(FALSE, FALSE))
        nonJoinPredOperands.insert(itemExpr);
      else if (op != ITM_INDEXCOLUMN)
        {
          // If more than one node is involved in an expression, it is treated
          // as a residual pred of the form ref(J1)=expr.
          // First, see if the expression is multi-node, and find the single
          // node if it is not.
          CANodeId exprNodeId = getExprNode(itemExpr);

          // The expression will NOT be part of a join pred if any of the
          // following hold true:
          //   1) the expression references more than one node
          //   2) the single referenced node is already in the join pred list
          //   3) the expression contains an aggregate function
          // Upon failing the join pred test, the expression can be part of a
          // range pred if it only references a single node. Otherwise, it can
          // only be part of a residual pred.
          if (exprNodeId == NULL_CA_ID
                  || joinPredNodes.containsThisId(exprNodeId)
                  || itemExpr->containsAnAggregate())
            {
              if (exprNodeId == NULL_CA_ID)
                rangeIneligible += nonJoinPredOperands.entries();
              nonJoinPredOperands.insert(itemExpr);
            }
          else
            {
              joinPredNodes.insert(exprNodeId);
              if (joinPredElemCount == 0)
                firstJoinPredItem = itemExpr;
              else
                {
                  // Avoid call to genQRExpr until we're sure there is more than
                  // one join pred element. Otherwise we'll have to retract it,
                  // and the created col elems will not be used, but will be
                  // referenced by subsequent occurrences of those columns.
                  if (joinPredElemCount == 1)
                    {
                      if (firstJoinPredItem->getOperatorType() == ITM_BASECOLUMN)
                        qrElemArray[0] = 
                            genQRColumn(firstJoinPredItem->getValueId());
                      else
                        qrElemArray[0] = genQRExpr(firstJoinPredItem, FALSE);
                      idArray[0] = firstJoinPredItem->getValueId();
                    }
                  qrElemArray[joinPredElemCount] = genQRExpr(itemExpr, FALSE);
                  idArray[joinPredElemCount] = itemExpr->getValueId();
                }
              joinPredElemCount++;
            }
        }
    }  // for each member of equality set

  // Create a JoinPred object if there is more than 1 table represented. If only
  // one, add it to the residual preds operand list.
  UInt32 hubJoinPredId = 0;
  if (joinPredElemCount > 1)
    {
      hubJoinPredId = eqSet.getJoinPredId();
      addJoinPred(jbbElem, qrElemArray, idArray, joinPredElemCount,
                  hubJoinPredId); // will be set only if it has no value
    }
  else if (joinPredElemCount == 1)
    nonJoinPredOperands.insert(firstJoinPredItem);

  // If a hub join pred was created from the equality set, save its id. Range
  // and residual preds that use a column of the equality set will reference the
  // equality set using this id.
  eqSet.setJoinPredId(hubJoinPredId);

  NABoolean useConstItem = FALSE;
  if (constItem)
    {
      if (typeSupported(constItem->getType()))
        useConstItem = TRUE;
      else
        nonJoinPredOperands.insert(constItem);
    }

  // If a constant was part of the equality set, create a range predicate for
  // each eligible (i.e., single-node) non-joinpred member of the set, and a
  // residual pred for the ineligible ones. If there is a join pred, create a
  // range predicate for it.
  if (useConstItem)
    {
      for (CollIndex i=0; i<nonJoinPredOperands.entries(); i++)
        {
          if (rangeIneligible.testBit(i))
            addEqualityResidPred(jbbElem, nonJoinPredOperands[i], constItem);
          else
            addEqualityRangePred(nonJoinPredOperands[i], eqSet.getType(),
                                 jbbElem, constItem);
        }

  if (hubJoinPredId)
        addEqualityRangePred(hubJoinPredId, eqSet.getType(), jbbElem, constItem);
    }
  else if (hubJoinPredId)
    {
      // Create residual preds that link each nonjoin operand to the hub join pred.
      for (CollIndex i=0; i<nonJoinPredOperands.entries(); i++)
        addEqualityResidPred(jbbElem, nonJoinPredOperands[i], NULL,
                             hubJoinPredId);
    }
  else
    {
      // No hub join pred to reference. Just produce a series of equality
      // residual predicates relating the equality set members.
      for (CollIndex i=0; i<nonJoinPredOperands.entries()-1; i++)
        addEqualityResidPred(jbbElem, nonJoinPredOperands[i],
                             nonJoinPredOperands[i+1]);
    }

  // Don't delete the element array, because the elements are used in JoinPreds
  // as well as in the array. However, we can delete the parallel id array.
  NADELETEBASIC(idArray, mvqrHeap_);
}  // processEqualitySet()

VEGPredicate* QRDescGenerator::getVegPredicate(UInt32 hubJoinPredId)
{
  QRTRACER("QRDescGenerator::getVegPredicate()");
  ValueId vegRefVid = (ValueId)hubJoinPredId;
  ItemExpr* ie = vegRefVid.getItemExpr();
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                     ie->getOperatorType() == ITM_VEG_REFERENCE,
                     QRDescriptorException,
                     "Hub join pred id is not ValueId of vegref -- %d",
                     hubJoinPredId);
  return (static_cast<VEGReference*>(ie))->getVEG()->getVEGPredicate();
}

void QRDescGenerator::addEqualityRangePred(ItemExpr* rangeItemExpr,
                                           const NAType* type,
                                           QRJBBPtr jbbElem,
                                           ConstValue* constItem)
{
  QRTRACER("QRDescGenerator::addEqualityRangePred()");
  OptRangeSpec* range = new (mvqrHeap_) OptRangeSpec(this, mvqrHeap_);
  if (rangeItemExpr->getOperatorType() == ITM_BASECOLUMN)
    range->setRangeColValueId(rangeItemExpr->getValueId());
  else
    range->setRangeExpr(rangeItemExpr);
  range->setType(&rangeItemExpr->getValueId().getType());
  range->addSubrange(constItem, constItem, TRUE, TRUE);
  ItemExpr* eqItemExpr = new(mvqrHeap_) BiRelat(ITM_EQUAL, rangeItemExpr, constItem);
  eqItemExpr->synthTypeAndValueId(TRUE);
  range->setID(eqItemExpr->getValueId());
  range->log();
  storeRangeInfo(range, jbbElem);
}

void QRDescGenerator::addEqualityRangePred(UInt32 hubJoinPredId,
                                           const NAType* type,
                                           QRJBBPtr jbbElem,
                                           ConstValue* constItem)
{
  QRTRACER("QRDescGenerator::addEqualityRangePred()");
  OptRangeSpec* range = new (mvqrHeap_) OptRangeSpec(this, mvqrHeap_);
  range->setRangeJoinPredId(hubJoinPredId);
  range->setID(getVegPredicate(hubJoinPredId)->getValueId());
  range->setType(type);
  range->addSubrange(constItem, constItem, TRUE, TRUE);
  range->log();
  storeRangeInfo(range, jbbElem);
}

void QRDescGenerator::addEqualityResidPred(QRJBBPtr jbbElem,
                                           ItemExpr* op1,
                                           ItemExpr* op2,
                                           ValueId hubJoinPredId)
{
  QRTRACER("QRDescGenerator::addEqualityResidPred()");
  ItemExpr* residExpr;
  assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                    op1, QRDescriptorException,
                    "op1 of equality resid pred is null");
  if (op2)
    residExpr = new(mvqrHeap_) BiRelat(ITM_EQUAL, op1, op2);
  else
    {
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        hubJoinPredId, QRDescriptorException,
                        "Neither op2 nor hubJoinPredId is given");
      residExpr = new(mvqrHeap_) BiRelat(ITM_EQUAL,
                                          op1, hubJoinPredId.getItemExpr());
    }

  
  assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                    relExpr_,
		    QRDescriptorException, 
		    "No RelExpr* stored for QRDescGenerator");
  residExpr->bindNode(relExpr_->getRETDesc()->getBindWA());
  processResidualPredicate(residExpr->getValueId(), jbbElem);
}

void QRDescGenerator::processReferencingPreds(CANodeIdSet* nodeSet,
                                              RelExpr* gbNode,
                                              QRJBBPtr jbbElem)
{
  QRTRACER("QRDescGenerator::processReferencingPreds()");
  ValueIdSet preds;
  NodeAnalysis* nodeAnalysis;
  TableAnalysis* tableAnalysis;

  // Gather all the predicates referencing a member of the nodeset. 
  for (CANodeId node = nodeSet->init();
       nodeSet->next(node);
       nodeSet->advance(node))
    {
      nodeAnalysis = node.getNodeAnalysis();
      assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                         nodeAnalysis, QRDescriptorException,
                         "No NodeAnalysis found for CA node id %ud", node.toUInt32());
      tableAnalysis = nodeAnalysis->getTableAnalysis();
      if (tableAnalysis)
        {
          preds.addSet(tableAnalysis->getReferencingPreds());

          // Constant preds, like return_false (for a combination of preds that
          // can't be satisfied), or a pred on current_date, etc., appear only
          // in localPreds, so add those.
          preds.addSet(tableAnalysis->getLocalPreds());
        }
    }
  
  // Add any predicates attached to the Group By node. Count(*) will not have
  // been pushed down to one of the scan nodes.
  if (gbNode)
    preds.addSet(gbNode->getSelectionPredicates());

  // Identify equality sets implied by vegpreds and equality conditions, and
  // translate these to joinpred/range/residual predicates in the descriptor.
  NAList<EqualitySet*> equalitySets(mvqrHeap_);
  formEqualitySets(preds, equalitySets);
  for (CollIndex i=0; i<equalitySets.entries(); i++)
    processEqualitySet(jbbElem, *equalitySets[i]);

  // Iterate over the remaining preds and generate range or residual predicates
  // as needed. The preds involved in equality sets have been removed by
  // formEqualitySets().
  for (ValueId predVid=preds.init(); preds.next(predVid); preds.advance(predVid))
    {
      OptRangeSpec* range = OptRangeSpec::createRangeSpec(this,
                                                          predVid.getItemExpr(), 
                                                          mvqrHeap_);
      if (range)
        storeRangeInfo(range, jbbElem);
      else  // residual predicate
        processResidualPredicate(predVid, jbbElem);
    }

  // Delete the equality sets and clean up the hash tables that reference them.
  discardEqualitySets(equalitySets);

  // Move eqset ptrs to QRDescGenerator member var before exiting scope. They
  // may be used later to rewrite a vegpred.
  for (CollIndex i=0; i<equalitySets.entries(); i++)
    allEqualitySets_.insert(equalitySets[i]);
}

void QRDescGenerator::discardEqualitySets(NAList<EqualitySet*>& equalitySets)
{
  QRTRACER("QRDescGenerator::discardEqualitySets()");
  // Iterate over all keys in the vid->eqset hash table and delete them.
  NAHashDictionaryIterator<QRValueId, EqualitySet> vegsHashIter(vegsUsedHash_);
  QRValueId* vidKey;
  EqualitySet* value;
  for (CollIndex i=0; i<vegsHashIter.entries(); i++) 
    {
      vegsHashIter.getNext(vidKey, value);
      delete vidKey;
    }

  // Now delete the keys in the exprtext->eqset hash table.
  NAHashDictionaryIterator<const NAString, EqualitySet>
            exprsHashIter(exprsUsedHash_);
  const NAString* exprKey;
  for (CollIndex i=0; i<exprsHashIter.entries(); i++) 
    {
      exprsHashIter.getNext(exprKey, value); 
      delete exprKey;
    } 

  // Empty the hash tables so they will be clean for the next JBB processed.
  // Can't let them delete their contents, because the key to value relationship
  // is many to one, so some of the values (EqualitySets) would be deleted more
  // than once. You can't request that only keys be deleted, so we have to
  // delete both the keys (above) and values (below, by iterating over the list
  // of equality sets) ourselves.
  vegsUsedHash_.clear(FALSE);
  exprsUsedHash_.clear(FALSE);

  // Delete all the equality sets, which were dynamically allocated so a
  // persistent pointer could be used as the value referenced in the hash
  // tables.
  //@ZXeqset
  //for (CollIndex i=0; i<equalitySets.entries(); i++)
  //  delete equalitySets[i];
}

void QRDescGenerator::combineEqSets(EqualitySet* source,
                                    EqualitySet* destination,
                                    NAList<EqualitySet*>& equalitySets)
{
  QRTRACER("QRDescGenerator::combineEqSets()");

  // Combine the two lists.
  destination->insert(*source);
  delete source;
  equalitySets.remove(source);

  // Change ptrs in hash tables from source to destination list.
  NAHashDictionaryIterator<QRValueId, EqualitySet>
        vegsHashIter(vegsUsedHash_);
  NAHashDictionaryIterator<const NAString, EqualitySet>
        exprsHashIter(exprsUsedHash_);

  // Iterate over all key/value pairs in the hash table (iterator doesn't allow
  // iterating over a selected value alone), selecting those keys that have the
  // source equality set as a value. I'm not sure how manipulating the hash
  // table in mid-iteration would affect the iteration, so we keep the keys in
  // a list, and delete/reinsert them with the new equality set after completing
  // the iteration.
  QRValueId* vidKey;
  EqualitySet* ptrListValue;
  NAList<QRValueId*> vidsToRemove(mvqrHeap_);
  CollIndex i;
  for (i=0; i<vegsHashIter.entries(); i++) 
    {
      vegsHashIter.getNext(vidKey, ptrListValue);  // get key and value
      if (ptrListValue == source)
        vidsToRemove.insert(vidKey);
    }

  // Remove and reinsert keys referencing the old (source) equality set.
  for (i=0; i<vidsToRemove.entries(); i++)
    {
      vegsUsedHash_.remove(vidsToRemove[i]);
      vegsUsedHash_.insert(vidsToRemove[i], destination);
    }

  // Now iterate over the hash table that uses expression text as a key, the
  // same as with the ValueId keys above.
  const NAString* exprKey;
  NAList<const NAString*> exprsToRemove(mvqrHeap_);
  for (i=0; i<exprsHashIter.entries(); i++) 
    {
      exprsHashIter.getNext(exprKey, ptrListValue); 
      if (ptrListValue == source)
        exprsToRemove.insert(exprKey);
    } 

  // Remove and reinsert expression text keys referencing the old (source)
  // equality set.
  for (i=0; i<exprsToRemove.entries(); i++)
    {
      exprsUsedHash_.remove(exprKey);
      exprsUsedHash_.insert(exprKey, destination);
    }
}  // combineEqSets()

// If no members were found in any other list, create a new equality list,
// populate it with the members of the vegpred, and add it to the list of lists.
// Also add the ValueId or expression text of each member to the appropriate
// hash table, with the address of the containing list as the value. The ValueId
// of the veg itself is also added as a key, so an equality operator that uses
// a vegref will find it. We have to create heap-allocated copies of the hash
// keys, because they need to persist beyond this function.
//
// Takes ValueIdSet and veg vid instead of VEG because we may remove one or more
// members from a local copy of a veg member list before making this call.
void QRDescGenerator::putVegMembersInEqualitySet(
              ItemExpr* vegPred,    // NULL if called for equality pred
              const ValueId& vegVid,
              const ValueIdSet& vegVals,
              EqualitySet*& eqSet,  // will be set if NULL
              NAList<EqualitySet*>& equalitySets)
{
  QRTRACER("QRDescGenerator::putVegMembersInEqualitySet()");

  // Make sure they weren't all removed because they were already members of
  // other separate-but-equal lists.
  if (vegVals.isEmpty())
    return;

  // Create list if one was not passed in, which is the case when an existing
  // compatible equality set is not found.
  if (!eqSet)
    {
      eqSet = new(mvqrHeap_) EqualitySet(mvqrHeap_);
      equalitySets.insert(eqSet);
    }

  // If this fn was called for a vegpred instead of a simple equality pred, use
  // the vegpred's value id as the joinpred id. If no vegpreds contribute to this
  // equality set, the value id of one of the members of the equality set will
  // be used.
  if (vegPred)
    eqSet->setJoinPredId(((VEGPredicate*)vegPred)->getVEG()->getVEGReference()
                                                           ->getValueId());

  // Put the value id of this veg in the hash table. The ValueIds of the
  // individual members will be added as well. NAHashDictionary is pointer-based,
  // so we have to allocate a copy of the ValueId on the heap, so it will
  // persist beyond this function. Note that ValueId is not an NABasicObject,
  // so we use the system heap.
  // 
  QRValueId* vegVidPtr = new (mvqrHeap_) QRValueId(vegVid);
  vegsUsedHash_.insert(vegVidPtr, eqSet);

  // Put the veg members in the list and in the hash tables.
  ItemExpr* itemExpr;
  OperatorTypeEnum op;
  QRValueId* vidPtr;
  NAString* exprTextPtr;
  for (ValueId vid=vegVals.init(); vegVals.next(vid); vegVals.advance(vid)) 
    {
      itemExpr = vid.getItemExpr();
      op = itemExpr->getOperatorType();
      if (op == ITM_BASECOLUMN)
        {
          eqSet->insert(itemExpr);
          vidPtr = new (mvqrHeap_) QRValueId(vid);
          vegsUsedHash_.insert(vidPtr, eqSet);
        }
      else if (op != ITM_INDEXCOLUMN)
        {
          // Constants are lumped in with general expressions, because when
          // query caching is enabled they are converted to params with
          // distinct ValueIds for each occurrence.
          eqSet->insert(itemExpr);
          exprTextPtr = new(mvqrHeap_) NAString(mvqrHeap_);
          itemExpr->unparse(*exprTextPtr, OPTIMIZER_PHASE, MVINFO_FORMAT);
          exprsUsedHash_.insert(exprTextPtr, eqSet);
        }
    }
}  // putVegMembersInEqualitySet()

void QRDescGenerator::addVegPredToEqualitySets(
              VEGPredicate* vegPred,
              NAList<EqualitySet*>& equalitySets)
{
  QRTRACER("QRDescGenerator::addVegPredToEqualitySets()");
  ValueIdSet vegVals;
  vegPred->getVEG()->getAndExpandAllValues(vegVals);
  EqualitySet* eqSet;
  EqualitySet* targetEqSet = NULL;
  ItemExpr* itemExpr;
  NAString exprText;
  OperatorTypeEnum op;
  QRValueId vidNum;

  for (ValueId vid=vegVals.init(); vegVals.next(vid); vegVals.advance(vid)) 
    {
      itemExpr = vid.getItemExpr();
      op = itemExpr->getOperatorType();
      if (op == ITM_BASECOLUMN)
        {
          // Probe with base col vid. These are in hash table as well as veg
          // vids, to handle all cases.
          vidNum = vid;
          eqSet = vegsUsedHash_.getFirstValue(&vidNum);
        }
      else if (op != ITM_INDEXCOLUMN)
        {
          // Constants are lumped in with general expressions, because when
          // query caching is enabled they are converted to params with
          // distinct ValueIds for each occurrence.
          // @ZX -- maybe add case for caching turned off
          exprText = "";
          itemExpr->unparse(exprText, OPTIMIZER_PHASE, MVINFO_FORMAT);
          eqSet = exprsUsedHash_.getFirstValue(&exprText);
        }
      else
        eqSet = NULL;

      if (eqSet)
        {
          // Remove the item that was found in the hash table, because it is
          // already in an existing list that the members of this vegpred will
          // be merged with.
          vegVals.subtractElement(vid);

          // If this is not the first list we've found, merge it into the target
          // list (the one the members of this vegpred will be added to).
          if (targetEqSet)
            {
              if (eqSet != targetEqSet)
                combineEqSets(eqSet, targetEqSet, equalitySets);
            }
          else
            targetEqSet = eqSet;
        }
    }

  // Add the members to targetEqSet, or if it is null, create a new list.
  putVegMembersInEqualitySet(vegPred, vegPred->getVEG()->getValueId(), vegVals,
                             targetEqSet, equalitySets);
}  // addVegPredToEqualitySets()

void QRDescGenerator::addEqPredToEqualitySets(
              ItemExpr* pred,
              NAList<EqualitySet*>& equalitySets)
{
  QRTRACER("QRDescGenerator::addEqPredToEqualitySets()");
  EqualitySet* eqSet;
  EqualitySet* targetEqSet = NULL;
  ItemExpr* eqOperand;
  OperatorTypeEnum op;
  NAString exprText;
  QRValueId vid;
  NABoolean moveToTarget[] = {TRUE, TRUE};

  assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                    pred->getOperatorType() == ITM_EQUAL,
		    QRDescriptorException, 
		    "addEqPredToEqualitySets() called for non-equality predicate");

  for (Int32 i=0; i<2; i++)
    {
      eqOperand = pred->child(i);
      op = eqOperand->getOperatorType();
      if (op == ITM_VEG_REFERENCE)
        {
          vid = ((VEGReference*)eqOperand)->getVEG()->getValueId();
          eqSet = vegsUsedHash_.getFirstValue(&vid);
        }
      else
        {
          exprText = "";
          eqOperand->unparse(exprText, OPTIMIZER_PHASE, MVINFO_FORMAT);
          eqSet = exprsUsedHash_.getFirstValue(&exprText);
        }

      if (eqSet)
        {
          moveToTarget[i] = FALSE;  // already there

          // If this is not the first list we've found (the one we will add the
          // eq operands to), merge it into the target list.
          if (targetEqSet)
            {
              if (eqSet != targetEqSet)
                combineEqSets(eqSet, targetEqSet, equalitySets);
            }
          else
            targetEqSet = eqSet;
        }
    }

    // Put the equality operands in a list (either new or created) and hash
    // tables, unless they were already members of a separate-but-equal list.
    for (Int32 i=0; i<2; i++) 
      {
        if (!moveToTarget[i])
          continue;  // already in target list

        eqOperand = pred->child(i);
        op = eqOperand->getOperatorType();
        if (op == ITM_VEG_REFERENCE)
          {
            VEG* veg = ((VEGReference*)eqOperand)->getVEG();
            // This will create targetEqSet if it is NULL.
            putVegMembersInEqualitySet(NULL,
                                       veg->getValueId(), veg->getAllValues(),
                                       targetEqSet, equalitySets);
          }
        else
          {
            if (!targetEqSet)
              {
                targetEqSet = new(mvqrHeap_) EqualitySet(mvqrHeap_);
                equalitySets.insert(targetEqSet);
              }
            targetEqSet->insert(eqOperand);
            NAString* exprTextPtr = new(mvqrHeap_) NAString(mvqrHeap_);
            eqOperand->unparse(*exprTextPtr, OPTIMIZER_PHASE, MVINFO_FORMAT);
            exprsUsedHash_.insert(exprTextPtr, targetEqSet);
          }
      }
}  // addEqPredToEqualitySets()

void QRDescGenerator::formEqualitySets(ValueIdSet& preds,  // processed preds removed
                                       NAList<EqualitySet*>& equalitySets)
{
  QRTRACER("QRDescGenerator::formEqualitySets()");

  ValueId predVid;
  ItemExpr* predExpr;
  OperatorTypeEnum op;

  // To form equality groups, we look at vegpreds and equality operators (and
  // remove them from the passed set of pred ValueIds after processing them).
  // The operands of each will either constitute a new equality set, or be
  // merged with an existing one. In some cases, 2 or more operands may
  // correspond to members of different equality sets, in which case they
  // will all be merged. 
  for (predVid=preds.init(); preds.next(predVid); preds.advance(predVid))
    {
      predExpr = predVid.getItemExpr();
      op = predExpr->getOperatorType();
      
      // If it's a range spec operator, sub in the actual predicate from the
      // right subtree.
      if (op == ITM_RANGE_SPEC_FUNC)
        {
          predExpr = predExpr->child(1);
          op = predExpr->getOperatorType();
        }
        
      if (op == ITM_VEG_PREDICATE)
        {
          addVegPredToEqualitySets((VEGPredicate*)predExpr, equalitySets);
          preds.subtractElement(predVid);
        }
      else if (op == ITM_EQUAL)
        {
          addEqPredToEqualitySets(predExpr, equalitySets);
          preds.subtractElement(predVid);
        }
    }
} // formEqualitySets()

void QRDescGenerator::setPredBitmap(QRValueId colVid, ElementType elemType)
{
  QRTRACER("QRDescGenerator::setPredBitmap()");
  QRElementPtr elem = getElementForValueID('C', colVid);
  if (!elem)
    {
      ValueId vid = colVid;
      OperatorTypeEnum opType = vid.getItemExpr()->getOperatorType();
      if (opType == ITM_BASECOLUMN)
        // Pass FALSE to prevent adding the col to the list of used cols. This
        // col element is created to find its table's id and position in the
        // table; it is not added to the descriptor.
        elem = genQRColumn(vid, 0, FALSE);
      else if (opType == ITM_VEG_REFERENCE)
        {
          // For a range pred on a member of a vegref, we need to set the bitmap
          // entry for each column in the veg. Make recursive calls to this fn
          // for each column in the veg, then exit.
          ValueIdSet vegMembers =
              static_cast<VEGReference*>(vid.getItemExpr())->getVEG()->getAllValues();
          for (ValueId id=vegMembers.init();
               vegMembers.next(id);
               vegMembers.advance(id))
            {
              if (id.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
                setPredBitmap(id, elemType);
            }
          return;
        }
      else
        {
          Int32 vidInt = vid;
          assertLogAndThrow2(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                            FALSE, QRDescriptorException,
                            "ValueId %d is not a base col or veg ref -- op type = %d",
                            vidInt, opType);
        }
    }
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                     elem, QRDescriptorException,
                     "Column id not found in hash table -- %d", (UInt32)colVid);
  QRColumnPtr col = elem->getReferencedElement()->downCastToQRColumn();
  const NAString& tblID = col->getTableID();
  elem = colTblIdHash_.getFirstValue(&tblID);
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                     elem, QRDescriptorException,
                     "Table id not found in hash table -- %s", tblID.data());
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                     elem->getElementType() == ET_Table, QRDescriptorException,
                     "Expected a Table element type, not %d",
                     elem->getElementType());

  // Set the column's bit in the appropriate bitmap, using its ordinal position
  // in the table.
  if (elemType == ET_RangePred)
    elem->downCastToQRTable()->setRangeBit(col->getColIndex());
  else if (elemType == ET_ResidualPred)
    elem->downCastToQRTable()->setResidualBit(col->getColIndex());
  else
    assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                       FALSE, QRDescriptorException,
                       "Wrong element type sent to setPredBitmap() -- %d",
                       elemType);
}

void QRDescGenerator::storeRangeInfo(OptRangeSpec* range, QRJBBPtr jbbElem)
{
  QRTRACER("QRDescGenerator::storeRangeInfo()");
  RangeInfo* rangeInfo = NULL;
  NAString* exprText = NULL;
  QRValueId* key = new (mvqrHeap_) QRValueId(range->getRangeJoinPredId());

  if (*key != NULL_VALUE_ID)
    rangeInfo = rangeColHash_.getFirstValue(key);
  else if (range->getRangeColValueId() != NULL_VALUE_ID)
    {
      *key = range->getRangeColValueId();
      rangeInfo = rangeColHash_.getFirstValue(key);
    }
  else
    {
      exprText = new (mvqrHeap_) NAString(range->getRangeExprText(), mvqrHeap_);
      rangeInfo = rangeExprHash_.getFirstValue(exprText);
    }

  // If a range was already created for another predicate on this column or
  // expression, intersect it with this range.
  if (rangeInfo)
    {
      rangeInfo->getRangeSpec()->intersectRange(range);
      // Don't need these since new hash entry was not created.
      delete key;
      delete exprText;
      delete range;
      range = NULL;  // makes it easier to debug use after deletion
    }
  else if (*key != NULL_VALUE_ID)
    rangeColHash_.insert(key, new(mvqrHeap_) RangeInfo(range, jbbElem));
  else
    {
      rangeExprHash_.insert(exprText, new(mvqrHeap_) RangeInfo(range, jbbElem));
      delete key;
    }
}

NABoolean QRDescGenerator::isRangeSupported(QRRangePredPtr rangePred)
{
  Int32 rangeExprSize = rangePred->getSize();
  if (rangeExprSize > maxExprSize_)
    return FALSE;
  	
  return TRUE;
}

void QRDescGenerator::addRangePredicates()
{
  QRTRACER("QRDescGenerator::addRangePredicates()");
  QRValueId* vid;
  const NAString* exprText;
  RangeInfo* rangeInfo; 
  OptRangeSpec* rangeSpec;
  QRRangePredPtr rangePred;

  NAHashDictionaryIterator<QRValueId, RangeInfo> vidIter(rangeColHash_);
  for (CollIndex i=0; i<vidIter.entries(); i++) 
    {
      vidIter.getNext(vid, rangeInfo);
      rangeSpec = rangeInfo->getRangeSpec();
      rangeSpec->addColumnsUsed(this);
      rangeSpec->addConstraints(this);
      if (rangeSpec->isFalse())
        {
          // Unsatisfiable condition; generate FALSE as a residual predicate.
          BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());
          ItemExpr* expr = new(mvqrHeap_) BoolVal(ITM_RETURN_FALSE);
          expr->bindNode(&bindWA);
          processResidualPredicate(expr->getValueId(), rangeInfo->getJbbElem());
        }
      else
        {
          rangePred = rangeSpec->createRangeElem();
          // The OptRangeSpec may involve a tautology (e.g., a>20 OR a<25 OR
          // a is null), which is equivalent to no range restriction at all.
          if (rangePred)
            {
              if ((isQueryMode() || isDumpMvMode()) && 
                  !isRangeSupported(rangePred))
                {
                  // This range predicate is too big, or unsuitable for a descriptor
                  // for some other reason. Replace it with an empty range pred
                  // marked as NotProvided.
                  QRRangePredPtr notProvidedRangePred = new(mvqrHeap_)
                  QRRangePred(ADD_MEMCHECK_ARGS(mvqrHeap_));
                  QRElementPtr rangeItem = rangePred->getRangeItem();
                  rangePred->setRangeItem(NULL);
                  notProvidedRangePred->setRangeItem(rangeItem);
                  notProvidedRangePred->setID(rangePred->getIDNum());
                  notProvidedRangePred->setResult(QRElement::NotProvided);
                
                  // Now delete the full range pred and use the empty one instead.
                  deletePtr(rangePred);
                  rangePred = notProvidedRangePred;
                }
              
              // Add the RangePred to the appropriate RangePredList, and set the
              // column's bit in the range predicate bitmap.
              rangeInfo->getOwningList()->addItemOrdered(rangePred);
              setPredBitmap(*vid, ET_RangePred);
            }
        }
    }
  rangeColHash_.clear(TRUE);

  NAHashDictionaryIterator<const NAString, RangeInfo> exprIter(rangeExprHash_);
  for (CollIndex i=0; i<exprIter.entries(); i++) 
    {
      exprIter.getNext(exprText, rangeInfo);
      rangePred = rangeInfo->getRangeSpec()->createRangeElem();
      // The OptRangeSpec may involve a tautology (e.g., a>20 or a<25), which is
      // equivalent to no range restriction at all.
      if (rangePred) 
        rangeInfo->getOwningList()->addItemOrdered(rangePred);
    }
  rangeExprHash_.clear(TRUE);
}

QRElementPtr QRDescGenerator::getElementForValueID(char firstChar, UInt32 id) const
{
  QRTRACER("QRDescGenerator::getElementForValueID()");
  char buff[12];
  QRValueId vid(id);

  buff[0] = firstChar;
  str_itoa(vid, buff+1);
  NAString idStr(buff);
  return colTblIdHash_.getFirstValue(&idStr);
}

void QRDescGenerator::mergeDescGenerator(const QRDescGenerator* other)
{
  mColumnsUsed_ += other->mColumnsUsed_;
  mExtraHubColumnsUsed_ += other->mExtraHubColumnsUsed_;

  const NAString* key;
  QRElementPtr value; 
  QRElementHashIterator otherIter(other->colTblIdHash_);

  for (CollIndex i=0; i<otherIter.entries(); i++) 
    {
      otherIter.getNext(key, value); 
      colTblIdHash_.insert(key, value);
    } 
}
