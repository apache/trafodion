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
 * File:         GenExpGenerator.cpp
 * Description:  Methods for class ExpGenerator
 *
 * Created:      6/28/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#define   SQLPARSERGLOBALS_FLAGS
#define   SQLPARSERGLOBALS_NADEFAULTS
#include "SqlParserGlobalsCmn.h"
#include "SqlParserGlobals.h"	// Parser Flags

#include <ctype.h>
#include <math.h>
#include "CmpContext.h"
#include "CmpStatement.h"
#include "CmpMain.h"
#include "CmpSeabaseDDL.h"

#include "GenExpGenerator.h"
#include "str.h"
#include "Sqlcomp.h"
#include "exp_clause_derived.h"
#include "exp_attrs.h"
#include "exp_function.h"
#include "exp_datetime.h"
#include "exp_bignum.h"
#include "DatetimeType.h"
#include "NumericType.h"
#include "IntervalType.h"
#include "CharType.h"
#include "charinfo.h"
#include "SQLCLIdev.h"
#include "BindWA.h"

#include "ExpCriDesc.h"
#include "ExpAtp.h"
#include "exp_attrs.h"
#include "exp_tuple_desc.h"
#include "exp_dp2_expr.h"
#include "ComQueue.h"

#include "ControlDB.h"

#include "ComSmallDefs.h"
#include "sqlcli.h"
#include "OptimizerSimulator.h"
#include "ItmFlowControlFunction.h"
#include "ExpPCodeOptimizations.h"
#include "ItemFunc.h"

#define   NEW_LEAN_EXPR

static short mapMode(ComColumnDirection direction);

static short mapMode(ComColumnDirection direction)
{
  switch(direction)
  {
  case COM_UNKNOWN_DIRECTION:

	return PARAMETER_MODE_UNDEFINED;

  case COM_INPUT_COLUMN:

	return PARAMETER_MODE_IN;

  case COM_OUTPUT_COLUMN:

	return PARAMETER_MODE_OUT;

  case COM_INOUT_COLUMN:

	return PARAMETER_MODE_INOUT;

  default:

	BriefAssertion(0, "Unrecognized ComColumnDirection");

	return 0; // not reached
  }
}

// End


ExpGenerator::ExpGenerator(Generator * generator_)
{
  generator = generator_;

  // $$$ is this the correct heap???
  cache_ = new(generator->wHeap()) ExpGeneratorCache(generator_);

  temps_length = 0;  // Initialize various member variables
  mapTable_ = 0;
  clause_list = 0;
  constant_list = 0;
  persistentList_ = 0;
  flags_ = 0;
  rowsSinceCounter_ = NULL;

  setHandleIndirectVC( TRUE );

  // default value for Pcode mode
  DefaultToken pcodeOptLevel = CmpCommon::getDefault(PCODE_OPT_LEVEL);

  switch (pcodeOptLevel)
    {
    case DF_OFF:
      // don't use any PCODE features at all
      pCodeMode_ = ex_expr::PCODE_NONE;
      break;

    case DF_MINIMUM:
      // use PCODE to call the regular expression evaluator
      pCodeMode_ =
	ex_expr::PCODE_ON | ex_expr::PCODE_EVAL;
      break;

    case DF_MEDIUM:
      // use PCODE for those clauses that can be translated to PCODE
      pCodeMode_ = ex_expr::PCODE_ON;
      break;

    case DF_HIGH:
      // use PCODE where possible and optimize it
      pCodeMode_ = ex_expr::PCODE_ON | ex_expr::PCODE_OPTIMIZE;
      break;

    case DF_MAXIMUM:
    case DF_ON:
    case DF_SYSTEM:
    default:
      // use PCODE where possible and run LLO (low-level) advanced optimizations
      pCodeMode_ = ex_expr::PCODE_ON | ex_expr::PCODE_OPTIMIZE |
                   ex_expr::PCODE_LLO;
      break;
    }
  static Int32 pcodeEnvSet = 0;
  static Int32 pcodeEnv = 1;  // if non-zero, pcode is enabled as before.
  if (pcodeEnvSet == 0)
    {
      char * pcodeEnvStr = getenv("64BIT_PCODE");
      if (pcodeEnvStr != NULL)
        pcodeEnv = atoi(pcodeEnvStr);

      pcodeEnvSet = 1;
    }
  if (pcodeEnv == 0)
    pCodeMode_ = ex_expr::PCODE_NONE;

  addPCodeMode( ex_expr::PCODE_SPECIAL_FIELDS );

  // May be used during key encoding expression generation if there are
  // added fields present.
  savedPCodeMode_ = -1;
};

ExpGenerator::~ExpGenerator()
  {
  delete cache_;
  }


///////////////////////////////////////////////////////////////////////////
// The following is a static function used by aggregate node when generating
// aggregate expression of type ITM_ONE_ROW
///////////////////////////////////////////////////////////////////////////
void createCastNodesatLeaves(ItemExpr *exp, Generator *generator)
{
  // expecting input of the kind: a,b,c,d....
  for (short i=0; i<exp->getArity(); i++)
  {
    if (exp->child(i)->getOperatorType() != ITM_ITEM_LIST)
    {
      ValueId val_id;
      ItemExpr *newExpr;
      val_id = exp->child(i)->castToItemExpr()->getValueId();
      newExpr = new(generator->wHeap()) Cast(exp->child(i), &(val_id.getType()));
      newExpr = newExpr->bindNode(generator->getBindWA());
      exp->child(i) = newExpr;
    }
    else
      createCastNodesatLeaves(exp->child(i), generator);
  }

} // createCastNodesatLeaves()

///////////////////////////////////////////////////////////
// returns a ConstValue node with min or max value
// The max value could either be the null max value or
// the non-null max value, based on the input min_max.
//////////////////////////////////////////////////////////
ItemExpr * ExpGenerator::getMinMaxValue(const NAType &type,
					short min_max /*0=min, -1=max, -2=max-without-null*/)
{
  NABoolean includeNulls = (min_max > -2);
  NABoolean getMin = (min_max == 0);

  return new(CmpCommon::statementHeap())
    ConstValue(&type,getMin,includeNulls);
}

/////////////////////////////////////////////////////////////////
// Make an equivalent Attributes object (or derived class)
// from an NAType.
/////////////////////////////////////////////////////////////////
Attributes * ExpGenerator::convertNATypeToAttributes
(const NAType &naType_x, CollHeap* wHeap)
{
  // ----------------------------------------------------------------
  // Each NAType corresponds to an object derived from class
  // "Attributes" used by expression generator to evaluate
  // operations on it.
  // All 'simple' types return SimpleType.
  // All 'complex' types return the class specific to them. These
  // classes contain methods used to perform operations on this type.
  // ----------------------------------------------------------------

  Attributes   *result = NULL;
  const NAType *naType = &naType_x;

  Int32 rsSize = 0;
  if (naType->getTypeQualifier() == NA_ROWSET_TYPE)
    {
      rsSize = ((SQLRowset *)naType)->getNumElements();
      naType = ((const SQLRowset *)naType)->getElementType();
    }

  if (naType->isSimpleType())
    {
      SimpleType * attr;

      switch (naType->getTypeQualifier()) {
      case NA_DATETIME_TYPE:
        attr = new(wHeap) ExpDatetime;
        break;
      default:
        attr = new(wHeap) SimpleType;
        break;
      }

      attr->setRowsetSize(rsSize);
      result = attr;
      attr->setNullFlag(naType->supportsSQLnull());
      attr->setNullIndicatorLength((short) naType->getSQLnullHdrSize());
      attr->setVCIndicatorLength((short) naType->getVarLenHdrSize());
      attr->setDatatype(naType->getFSDatatype());
      attr->setLength(naType->getNominalSize());
      attr->setDataAlignmentSize(naType->getDataAlignment());
      if (naType_x.getTypeQualifier() == NA_ROWSET_TYPE) {
	if (((SQLRowset *)(&naType_x))->useTotalSize()) {
	  // This indicates us to use the whole array size
 	  attr->setLength(sizeof(Lng32) + (attr->getRowsetSize() * naType->getTotalSize()));
	  result->setUseTotalRowsetSize();
	  ((SQLRowset *)(&naType_x))->useTotalSize() = FALSE;
	}
      }

      attr->setIsoMapping((CharInfo::CharSet)SqlParser_ISO_MAPPING);

      if (naType->getTypeQualifier() == NA_CHARACTER_TYPE) 
	{
	  const CharType *charType = (CharType *)naType;

	  if (charType->isUpshifted())
	    attr->setUpshift(1);

	  if (charType->isCaseinsensitive())
	    attr->setCaseinsensitive(1);

	  CharInfo::CharSet cs = charType->getCharSet();
	  if ( cs == CharInfo::UCS2       ||
               cs == CharInfo::KANJI_MP   ||
               cs == CharInfo::KSC5601_MP )
	    attr->setWidechar(1);

          attr->setPrecision(charType->getPrecisionOrMaxNumChars());
	  attr->setCharSet(charType->getCharSet());
	  attr->setCollation(charType->getCollation());
	}
      else if (naType->getTypeQualifier() == NA_NUMERIC_TYPE)
        {
          const NumericType *numericType = (const NumericType *) naType;

          if (numericType->binaryPrecision() && numericType->isExact() &&
              (numericType->getFSDatatype() != REC_BPINT_UNSIGNED))
            attr->setPrecision(0);
          else
            attr->setPrecision(numericType->getPrecision());

          attr->setScale((short) numericType->getScale());
        }
      else if (naType->getTypeQualifier() == NA_DATETIME_TYPE)
        {
          const DatetimeType *datetimeType = (const DatetimeType *) naType;

          // Map the datetime type (start and end fields) to the
          // corresponding REC_DATETIME_CODE.  Note that in order to
          // support the non-standard SQL/MP datetime types, the
          // REC_DATETIME_CODE enumeration has been extended to
          // contain a value for each legal (including non-standard)
          // combination of start and end fields.  This means that the
          // correspondence between the values of the SQLDATETIME_CODE
          // and REC_DATETIME_CODE enumerations holds only for the
          // standard datetime types.  The method getRecDateTimeCode()
          // maps the datetime types based on the start and end
          // fields.
          //
          // This method has been modified as part of the MP Datetime
          // Compatibility project.
          //
          attr->setPrecision(datetimeType->getRecDateTimeCode());
          if (attr->getPrecision() == REC_DTCODE_FRACTION)
          {
            attr->setPrecision(REC_DTCODE_SECOND);
          }
          attr->setScale(datetimeType->getFractionPrecision());
        }
      else if (naType->getTypeQualifier() == NA_INTERVAL_TYPE)
        {
          const IntervalType *intervalType = (const IntervalType *) naType;
          attr->setPrecision(intervalType->getLeadingPrecision());
          attr->setScale(intervalType->getFractionPrecision());
          if (attr->getDatatype() == REC_INT_FRACTION)
          {
           attr->setDatatype(REC_INT_SECOND);
          }
        }
      
      else if (naType->getTypeQualifier() == NA_LOB_TYPE)
        {
          
	  SQLlob *lobType = (SQLlob *) naType;
          attr->setCharSet(lobType->getCharSet());
	}
      
      else
        {
          attr->setPrecision(0);
          attr->setScale(0);
        }
    }
  else
    {
      if (naType->getSimpleTypeName() == "BIG NUM")
        {
          const NumericType *numericType = (const NumericType *) naType;

          BigNum * attr = new(wHeap)
            BigNum(numericType->getNominalSize(),
                   numericType->getPrecision(),
                   (short)numericType->getScale(),
                   numericType->isUnsigned());

	  attr->setRowsetSize(rsSize);
          result = attr;

          attr->setNullFlag(naType->supportsSQLnull());
          attr->setNullIndicatorLength((short) naType->getSQLnullHdrSize());
          attr->setVCIndicatorLength((short) naType->getVarLenHdrSize());
          attr->setDatatype(numericType->getFSDatatype());
          attr->setComplexDatatype(numericType->getFSDatatype());
          attr->setDataAlignmentSize(naType->getDataAlignment());
	  if (naType_x.getTypeQualifier() == NA_ROWSET_TYPE) {
	    if (((SQLRowset *)(&naType_x))->useTotalSize()) {
	      // This indicates us to use the whole array size
	      attr->setLength(sizeof(Lng32) + (attr->getRowsetSize() * naType->getTotalSize()));
	      result->setUseTotalRowsetSize();
	      ((SQLRowset *)(&naType_x))->useTotalSize() = FALSE;
	    }
	  }
        }

      else
        {
          GenAssert(0,"Don't know how to convert complex type to Attributes");
        }
    }

  return result;
}

Attributes::DefaultClass ExpGenerator::getDefaultClass(const NAColumn * col)
{
  Attributes::DefaultClass dc;

  switch (col->getDefaultClass())
  {
    case COM_CURRENT_DEFAULT:
      dc = Attributes::DEFAULT_CURRENT; break;
    case COM_CURRENT_UT_DEFAULT:
      dc = Attributes::DEFAULT_CURRENT_UT; break;
    case COM_UUID_DEFAULT:
      dc = Attributes::DEFAULT_UUID; break;
    case COM_FUNCTION_DEFINED_DEFAULT:
      dc = Attributes::DEFAULT_FUNCTION; break;
    case COM_NO_DEFAULT:
    case COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT:
    case COM_ALWAYS_DEFAULT_COMPUTED_COLUMN_DEFAULT:
      dc = Attributes::NO_DEFAULT; break;
    case COM_NULL_DEFAULT:
      dc = Attributes::DEFAULT_NULL; break;
    case COM_USER_DEFINED_DEFAULT:
      dc = Attributes::DEFAULT_USER; break;
    case COM_USER_FUNCTION_DEFAULT:
      dc = Attributes::DEFAULT_USER_FUNCTION; break;
    case COM_IDENTITY_GENERATED_BY_DEFAULT:
    case COM_IDENTITY_GENERATED_ALWAYS:
      dc = Attributes::DEFAULT_IDENTITY; break;
    default:
      dc = Attributes::INVALID_DEFAULT; break;
  }

  return dc;
}

short ExpGenerator::addDefaultValue(NAColumn * col, Attributes * attr,
				    ComDiagsArea  * diagsArea,
				    COM_VERSION   objectSchemaVersion)
{
  Attributes::DefaultClass dc;
  char		*defaultValue = NULL;
  ComDiagsArea	* da;
  short		fsDataType;
  short		extraFltLen = 0;
  NAString      terminalName;

  dc = getDefaultClass(col);
  if (dc == Attributes::INVALID_DEFAULT)
    {
      // invalid default value
      da = (diagsArea ? diagsArea : CmpCommon::diags());
      *da << DgSqlCode(-7001)
	  << DgString0(col->getDefaultValue() ? col->getDefaultValue() : " ")
	  << DgString1(col->getFullColRefNameAsAnsiString());
      return -1;
    }

  NAString *castStr;

  if ((col) && 
      (col->isAddedColumn()))
    {
      attr->setAddedCol();
    }

  fsDataType = attr->getDatatype();
  NABoolean isUpshifted = FALSE;
  switch (dc)
  {
    case Attributes::DEFAULT_CURRENT:
      // This value is for the old rows before the alter table with add column
      // For new rows compiler will produce a node with CURRENT exp
      castStr = new(wHeap()) NAString("CAST(TIMESTAMP '0001-01-01:12:00:00.000000' AS ", wHeap());
      break;
    case Attributes::DEFAULT_CURRENT_UT:
      // This value is for the old rows before the alter table with add column
      // For new rows compiler will produce a node with CURRENT exp
      castStr = new(wHeap()) NAString("CAST( 0 AS ", wHeap());
      break;
    case Attributes::DEFAULT_NULL:
      if (attr->getNullFlag())
      {
	  defaultValue =
	    new(generator->getSpace())char[attr->getDefaultValueStorageLength()];
	  // move null value
	  str_pad(defaultValue, ExpTupleDesc::NULL_INDICATOR_LENGTH, '\377');
      }
      // else treat them as if it is no default
      attr->setDefaultValue(dc, defaultValue);
      return 0;
    case Attributes::DEFAULT_USER_FUNCTION:
     {
      const NAType* colType = col->getType();
         castStr = new(wHeap()) NAString("CAST('\' \'' AS ", wHeap());
     }
      break;
    case Attributes::DEFAULT_USER:
      if (col->getDefaultValue() != NULL)
      {
        if (col->getType()->getTypeQualifier() == NA_CHARACTER_TYPE)
          {
            const CharType *colCharType = (const CharType *)col->getType();
 
            if (colCharType->isUpshifted())
              isUpshifted = TRUE;
          }

	castStr = new(wHeap()) NAString("CAST(", wHeap());
	castStr->append(NAString(col->getDefaultValue(), wHeap()));
	castStr->append(NAString(" AS ", wHeap()));
      }
      else
      {
	da = (diagsArea ? diagsArea : CmpCommon::diags());
	*da << DgSqlCode(-7001)
	  << DgString0("NULL POINTER")
	  << DgString1(col->getFullColRefNameAsAnsiString());
	return -1;
      }
      break;
    case Attributes::DEFAULT_IDENTITY:
      // since this is used only for AddColumn and that we don't allow to add
      // IDENTITY column, it is Ok. to cast it to zero.
      castStr = new(wHeap()) NAString("CAST(0 AS ", wHeap());
      break;
    case Attributes::DEFAULT_FUNCTION:
      castStr = new(wHeap()) NAString("CAST( \' \' AS ", wHeap());
      break;

    case Attributes::NO_DEFAULT:
    default:
      attr->setDefaultValue(dc, NULL);
      return 0;
  }
  // Append Type Name
  castStr->append(NAString(col->getType()->getTypeSQLname(TRUE), wHeap()));
  if (isUpshifted)
    castStr->append(NAString(" UPSHIFT ", wHeap()));
  if (! attr->getNullFlag())
      castStr->append(NAString(" NOT NULL)", wHeap()));
  else
      castStr->append(NAString(")", wHeap()));

  BindWA *bindWA = generator->getBindWA();

  // When creating an SQL/MX table, (Called from
  // addDefaultValuesToRCB) the NATable is NULL.  
  Parser parser(bindWA->currentCmpContext());

  ItemExpr *defaultValueExpr = parser.getItemExprTree(*castStr);

  // if this column is serialized, then encode the default value.
  if (CmpSeabaseDDL::isEncodingNeededForSerialization(col))
    {
      defaultValueExpr = new(wHeap()) CompEncode
	(defaultValueExpr, FALSE, -1, CollationInfo::Sort, TRUE);
    }

  if (!defaultValueExpr || bindWA->errStatus())
  {
    // invalid default value
    da = (diagsArea ? diagsArea : CmpCommon::diags());
    *da << DgSqlCode(-7001)
        << DgString0(col->getDefaultValue() ? col->getDefaultValue() : " ")
        << DgString1(col->getFullColRefNameAsAnsiString());
    
    return -1;
  }

  defaultValueExpr->bindNode(bindWA);

  ValueIdList defaultValueIdList;
  defaultValueIdList.insert(defaultValueExpr->getValueId());
  defaultValue = new(generator->getSpace()) char[attr->getDefaultValueStorageLength()];
  Lng32 length;
  Lng32 offset;
  Lng32 daMark;

  da = (diagsArea ? diagsArea : CmpCommon::diags());
  daMark = da->mark();
  ex_expr::exp_return_type evalReturnCode = defaultValueIdList.evalAtCompileTime
    (0,
     ExpTupleDesc::PACKED_FORMAT,
     defaultValue, attr->getDefaultValueStorageLength() + extraFltLen,
     &length, &offset, da);

  if ((evalReturnCode != ex_expr::EXPR_OK) || (da->getNumber() != daMark))
  {
    // remove all the errors inserted in expression evaluation
    da->rewind(daMark, TRUE);
    // invalid default value
    NAString *cTemp;
    cTemp = unicodeToChar((const NAWchar *) (col->getDefaultValue())
                        , (Int32)NAWstrlen((const NAWchar *) (col->getDefaultValue()))
                        , ComGetErrMsgInterfaceCharSet()
                        , CmpCommon::statementHeap());

    if ( cTemp == NULL ) /* Prevent null ptr reference */
      cTemp = new (CmpCommon::statementHeap()) NAString(CmpCommon::statementHeap());

    *da << DgSqlCode(-7001)
        << DgString0(convertNAString(*cTemp,CmpCommon::statementHeap()))
        << DgString1(col->getFullColRefNameAsAnsiString());
    delete cTemp;
    return -1;
  }

  // Adjusting the resultBuffer's variable Character indicator length to
  // attr VCIndicatorLength
  if (attr->getVCIndicatorLength() > 0)
  {
    char *defaultValueAdj =
           new(generator->getSpace()) char[attr->getDefaultValueStorageLength()];
    short len;
    char *lenPtr;
    char *valuePtr = defaultValueAdj;

    if (attr->getNullFlag())
    {
      str_pad(valuePtr, ExpTupleDesc::NULL_INDICATOR_LENGTH, 0);
      valuePtr += ExpTupleDesc::NULL_INDICATOR_LENGTH;
    }

    if (attr->getVCIndicatorLength() == sizeof(short))
    {
      len = (short)length;
      lenPtr = (char *)&len;
    }
    else
      lenPtr = (char *)&length;

    str_cpy_all(valuePtr, lenPtr, attr->getVCIndicatorLength());
    valuePtr += attr->getVCIndicatorLength();

    str_cpy_all(valuePtr, defaultValue + offset, length);
    attr->setDefaultValue(dc, defaultValueAdj);
  }
  else
    attr->setDefaultValue(dc, defaultValue);

  return 0;
}

void ExpGenerator::addDefaultValues(const ValueIdList & val_id_list,
				    const NAColumnArray &naColArr,
				    ExpTupleDesc * tupleDesc,
				    NABoolean inputIsColumnList)
{
  NABoolean setAddedColumn = FALSE;
  NAString defVal;

  for (unsigned short i = 0; i < val_id_list.entries(); i++)
    {
      NAColumn * col = NULL;
      
      if (inputIsColumnList)
	{
	  ItemExpr * ie = val_id_list[i].getItemExpr();
	  GenAssert(((ie->getOperatorType() == ITM_BASECOLUMN) ||
		     (ie->getOperatorType() == ITM_INDEXCOLUMN)), 
		    "Must be a base or index column");

	  if (ie->getOperatorType() == ITM_BASECOLUMN)
	    {
	      BaseColumn * bc = (BaseColumn *)ie;
	      col = bc->getNAColumn();
	    }
	  else
	    {
	      IndexColumn * ic = (IndexColumn *)ie;
	      col = ic->getNAColumn();
	    }
	}
      else
	{
	  col = naColArr.getColumn(i);
	}

      Attributes * attr = (tupleDesc ? tupleDesc->getAttr(i) : NULL);

      if (attr)
	{
	  short rc = addDefaultValue(col, attr);
	  if (rc)
	    {
	      GenExit();
	      return;
	    }
	}

      if (col->isAddedColumn())
	{
	  setAddedColumn = TRUE;

          // the i-th attribute
          Attributes * ithAttr =
            generator->getMapInfo(val_id_list[i])->getAttr();
          ithAttr->setAddedCol();
          ithAttr->setDefaultFieldNum(i);

	  if (attr)
            {
              attr->setAddedCol();
              attr->setDefaultFieldNum(i);
            }
	}
    } // for

  if ((setAddedColumn) && (tupleDesc))
    tupleDesc->setAddedField();

  // Set EID Olt optimization off for tables with added columns.
  if ( setAddedColumn
       && !( CmpCommon::getDefault( EXPAND_DP2_SHORT_ROWS ) == DF_ON ) )
    //    generator->doOltQueryOptimization() = FALSE;
    generator->oltOptInfo()->setOltEidOpt(FALSE);
}

void ExpGenerator::copyDefaultValues(
				     ExpTupleDesc * tgtTupleDesc,
				     ExpTupleDesc * srcTupleDesc)
{
  Lng32 numAttrs = MINOF(srcTupleDesc->numAttrs(), tgtTupleDesc->numAttrs());
  for (CollIndex i = 0; i < numAttrs; i++)
    {
      Attributes * srcAttr = srcTupleDesc->getAttr(i);
      Attributes * tgtAttr = tgtTupleDesc->getAttr(i);

      if (srcAttr->isAddedCol())
	tgtAttr->setAddedCol();
      
      tgtAttr->setDefaultClass(srcAttr->getDefaultClass());

      if (srcAttr->getDefaultValue())
	{
          Lng32 tgtDefLen = tgtAttr->getDefaultValueStorageLength();
          Lng32 srcDefLen = srcAttr->getDefaultValueStorageLength();
          char* srcDefVal = srcAttr->getDefaultValue();
	  char * tgtDefVal = new(generator->getSpace()) char[tgtDefLen];

          // if source and target def storage lengths dont match, then
          // need to move each part (null, vclen, data) separately.
          if ((tgtDefLen == srcDefLen) &&
              (tgtAttr->getNullFlag() == srcAttr->getNullFlag()) &&
              (tgtAttr->getVCIndicatorLength() == srcAttr->getVCIndicatorLength()) &&
              (tgtAttr->getLength() == srcAttr->getLength()))
            {
              str_cpy_all(tgtDefVal, srcDefVal, srcDefLen);
            }
          else
            {
              char * tgtDefValCurr = tgtDefVal;

              short nullVal = 0;
              if (srcAttr->getNullFlag())
                {
                  str_cpy_all((char*)&nullVal, srcDefVal, 
                              ExpTupleDesc::NULL_INDICATOR_LENGTH);
                  srcDefVal += ExpTupleDesc::NULL_INDICATOR_LENGTH;
                }
              
              if (tgtAttr->getNullFlag())
                {
                  str_cpy_all(tgtDefVal, (char*)&nullVal, 
                              ExpTupleDesc::NULL_INDICATOR_LENGTH);
                  tgtDefValCurr += ExpTupleDesc::NULL_INDICATOR_LENGTH;
                }
              
              Lng32 srcDefLen    = srcAttr->getLength(srcDefVal);
              tgtAttr->setVarLength(srcDefLen, tgtDefValCurr);
              tgtDefValCurr += tgtAttr->getVCIndicatorLength();
              srcDefVal += srcAttr->getVCIndicatorLength();

              str_cpy_all(tgtDefValCurr, srcDefVal, srcDefLen);
            }
	  
	  tgtAttr->setDefaultValue(srcAttr->getDefaultClass(), tgtDefVal);
	}
    }
  
}

short ExpGenerator::genColNameList(const NAColumnArray &naColArr,
				   Queue* &colNameList)
{
  colNameList = NULL;
  if (naColArr.entries() == 0)
    return 0;

  Space * space = generator->getSpace();

  colNameList = new(space) Queue(space);
  for (unsigned short i = 0; i < naColArr.entries(); i++)
    {
      NAColumn * col = naColArr.getColumn(i);

      char * colName =
	space->allocateAndCopyToAlignedSpace(col->getColName().data(),
					     col->getColName().length());

      colNameList->insert(colName);
    }

  return 0;
}

short ExpGenerator::handleUnsupportedCast(Cast * castNode)
{
  const NAType &srcNAType = castNode->child(0)->getValueId().getType();
  const NAType &tgtNAType = castNode->getValueId().getType();
  short srcFsType = srcNAType.getFSDatatype();
  short tgtFsType = tgtNAType.getFSDatatype();

  // check if conversion involves tinyint or largeint unsigned
  NABoolean tinyintCast = FALSE;
  NABoolean largeUnsignedCast = FALSE;
  if (((DFS2REC::isTinyint(srcFsType)) &&
       (NOT DFS2REC::isTinyint(tgtFsType))) ||
      ((NOT DFS2REC::isTinyint(srcFsType)) &&
       (DFS2REC::isTinyint(tgtFsType))))
    tinyintCast = TRUE;

  if ((srcFsType == REC_BIN64_UNSIGNED) ||
      (tgtFsType == REC_BIN64_UNSIGNED))
    largeUnsignedCast = TRUE;

  if ((NOT tinyintCast) && (NOT largeUnsignedCast))
    return 0;

  ex_conv_clause tempClause;
  if (tempClause.isConversionSupported(srcFsType, srcNAType.getNominalSize(),
                                       tgtFsType, tgtNAType.getNominalSize()))
    return 0;

  // if this cast involved a tinyint and is unsupported, convert to
  // smallint.
  if (tinyintCast)
    {
      // add a Cast node to convert from/to tinyint to/from small int.
      NumericType * newType = NULL;
      if (DFS2REC::isInterval(srcFsType)) // interval to tinyint
        {
          const IntervalType &srcInt = (IntervalType&)srcNAType; 
          newType = new (generator->wHeap())
            SQLNumeric(generator->wHeap(), sizeof(short), srcInt.getTotalPrecision(), 
                       srcInt.getFractionPrecision(),
                       TRUE, srcNAType.supportsSQLnull());
        }
      else if (DFS2REC::isInterval(tgtFsType)) // tinyint to interval
        {
          const NumericType &srcNum = (NumericType&)srcNAType; 
           newType = new (generator->wHeap())
             SQLNumeric(generator->wHeap(), sizeof(short), srcNum.getPrecision(), 
                        srcNum.getScale(),
                        NOT srcNum.isUnsigned(), srcNAType.supportsSQLnull());
        }
      else if (DFS2REC::isTinyint(srcFsType)) // tinyint to non-tinyint
        {
          const NumericType &srcNum = (NumericType&)srcNAType; 
          
          if ((srcNum.getScale() == 0) &&
              (srcNum.binaryPrecision()))
            newType = new (generator->wHeap())
              SQLSmall(generator->wHeap(), NOT srcNum.isUnsigned(),
                       tgtNAType.supportsSQLnull());
          else
            newType = new (generator->wHeap())
              SQLNumeric(generator->wHeap(), sizeof(short), srcNum.getPrecision(), 
                         srcNum.getScale(),
                         NOT srcNum.isUnsigned(), 
                         tgtNAType.supportsSQLnull());
        }
      else if (DFS2REC::isTinyint(tgtFsType)) // non-tinyint to tinyint
        {
          const NumericType &srcNum = (NumericType&)srcNAType; 
          const NumericType &tgtNum = (NumericType&)tgtNAType; 
          
          if ((tgtNum.getScale() == 0) &&
              (tgtNum.binaryPrecision()))
            newType = new (generator->wHeap())
              SQLSmall(generator->wHeap(), NOT tgtNum.isUnsigned(),
                       tgtNAType.supportsSQLnull());
          else
            newType = new (generator->wHeap())
              SQLNumeric(generator->wHeap(), sizeof(short), tgtNum.getPrecision(), 
                         tgtNum.getScale(),
                         NOT tgtNum.isUnsigned(), 
                         tgtNAType.supportsSQLnull());
        }

      ItemExpr * newChild =
        new (generator->wHeap())
        Cast(castNode->child(0), newType);
      ((Cast*)newChild)->setFlags(castNode->getFlags());
      castNode->setSrcIsVarcharPtr(FALSE);
      newChild = newChild->bindNode(generator->getBindWA());
      newChild = newChild->preCodeGen(generator);
      if (! newChild)
        return -1;
      
      castNode->setChild(0, newChild);
      srcFsType = castNode->child(0)->getValueId().getType().getFSDatatype();
    }

  if ((srcFsType == REC_BIN64_UNSIGNED) ||
      (tgtFsType == REC_BIN64_UNSIGNED))
    {
      const NumericType &numSrc = (NumericType&)srcNAType;
      // add a Cast node to convert to sqllargeint signed.
      ItemExpr * newChild =
        new (generator->wHeap())
        Cast(castNode->child(0),
             new (generator->wHeap())
             SQLLargeInt(generator->wHeap(), numSrc.getScale(), 1,
                         TRUE,
                         srcNAType.supportsSQLnull()));
      ((Cast*)newChild)->setFlags(castNode->getFlags());
      castNode->setSrcIsVarcharPtr(FALSE);
      newChild = newChild->bindNode(generator->getBindWA());
      newChild = newChild->preCodeGen(generator);
      if (! newChild)
        return -1;
      
      castNode->setChild(0, newChild);
      srcFsType = castNode->child(0)->getValueId().getType().getFSDatatype();
    }

  return 0;
}

/////////////////////////////////////////////////////////////////
// this function returns an expr tree that multiplies the source
// by 10 ** exponent.  If exponent is negative, the returned expr
// tree divides the source by 10 ** (- exponent).
/////////////////////////////////////////////////////////////////
ItemExpr * ExpGenerator::scaleBy10x(const ValueId & source, Lng32 exponent)
{
  ItemExpr * retTree = source.getItemExpr();
  OperatorTypeEnum srcOrigOpType = retTree->origOpType(); 

  //
  // If the exponent is 0, return the source tree unchanged.
  //
  if (exponent == 0)
    return retTree;

  NABoolean downscale = (exponent < 0 ? TRUE : FALSE);
  
  // Retain this flag and pass it on to next ITM_DIVIDE
  // operator if any. This flag indicates that no special
  // rounding be performed. This is set in the case of
  // date-time kind of operations.
  NABoolean ignoreSpecialRounding = 
    (retTree->getOperatorType() != ITM_DIVIDE)? FALSE :
    ((BiArith*)retTree)->ignoreSpecialRounding();
  //
  // If the exponent is positive, multiply the source.  Otherwise, divide the
  // source.
  //
  char *str = new(wHeap()) char[ 100 + abs( exponent ) ];
  if (exponent > 0)
    strcpy(str, "@A1 * 1");
  else {
    strcpy(str, "@A1 / 1");
    exponent = - exponent;
  }
  do {
    strcat(str, "0");
  } while (--exponent > 0);
  retTree = createExprTree(str, 0, 1, retTree);

  // inherit and propogate OrigOpType_
  retTree->setOrigOpType(srcOrigOpType);

  NADELETEBASIC( str, wHeap() );

  if ((retTree) && (downscale) &&
      (retTree->getOperatorType() == ITM_DIVIDE))
    {
      ((BiArith*)retTree)->setDivToDownscale(TRUE);

      if(ignoreSpecialRounding)
        ((BiArith*)retTree)->setIgnoreSpecialRounding();
    }

  //
  // Bind the tree.  If necessary, change the scale of the result to match the
  // scale of the source, so the generator won't upscale the numerator.
  //
  retTree->bindNode(generator->getBindWA());
  const NAType& resultType = retTree->getValueId().getType();
  if (resultType.getTypeQualifier() == NA_NUMERIC_TYPE) {
    Lng32 sourceScale = ((NumericType &) source.getType()).getScale();
    if (((NumericType &) resultType).getScale() != sourceScale)
      ((NumericType &) resultType).setScale(sourceScale);
  }
  return retTree;
}

/////////////////////////////////////////////////////////////////
// if the scales of the source and target types are not
// the same, this function returns an expr tree to upscale
// or downscale the source to the target scale by multiplying
// the source by 10 ** (target_scale - source_scale).
/////////////////////////////////////////////////////////////////
ItemExpr * ExpGenerator::matchScales(const ValueId & source,
				     const NAType & targetType)
{
  ItemExpr * sourceTree = source.getItemExpr();
  ItemExpr * retTree = matchScalesNoCast(source,targetType);

  if (sourceTree != retTree)
    {
      // we generated an expression to change the scale of the source;
      // now add a Cast to make the source datatype match the target
      retTree = new(wHeap()) Cast(retTree, &targetType);
      retTree->bindNode(generator->getBindWA());

      if (handleUnsupportedCast((Cast*)retTree))
        return NULL;

      // Mark this as preCodeGenned so we don't generate more
      // scaling code to handle possible scale differences in the
      // new Cast (note that matchScalesNoCast() in this case has
      // already preCodeGenned the child of the Cast)
      retTree->markAsPreCodeGenned();
    }

  // Note that if sourceTree == retTree above, the tree might not
  // have been preCodeGenned; we let the caller take care of that.

  return retTree;
}


//////////////////////////////////////////////////////////////////
// This function is the "guts" of matchScales(), but it does not
// cast the resulting expression into the target type.  Usually,
// we get to this function via matchScales().  The one exception
// is in key building, where we wish to detect truncation and overflow
// errors due to scaling in a special way.  So, key building calls
// this method directly to avoid generating a Cast.
//////////////////////////////////////////////////////////////////
ItemExpr * ExpGenerator::matchScalesNoCast(const ValueId & source,
					   const NAType & targetType)
{
  ItemExpr * retTree = source.getItemExpr();
  //
  // If the source or target is neither interval nor numeric, return the
  // source tree unchanged.
  //
  NABuiltInTypeEnum sourceTypeQual = source.getType().getTypeQualifier();
  NABuiltInTypeEnum targetTypeQual = targetType.getTypeQualifier();

  if (((sourceTypeQual != NA_NUMERIC_TYPE) &&
       (sourceTypeQual != NA_INTERVAL_TYPE)) ||
      ((targetTypeQual != NA_NUMERIC_TYPE) &&
       (targetTypeQual != NA_INTERVAL_TYPE)))
    return retTree;

  // If it is a null constant no need to scale it.
  if (retTree->getOperatorType() == ITM_CONSTANT)
    {
      ConstValue * constant = (ConstValue *) retTree;
      if (constant->isNull())
        return retTree;
    }

  //
  // Upscale or downscale the source to the target scale.  If the source scale
  // is the same as the target scale, return the source tree unchanged.
  //
  Lng32 sourceScale = (source.getType().getTypeQualifier() == NA_NUMERIC_TYPE)
    ? ((NumericType &) source.getType()).getScale()
    : ((IntervalType &) source.getType()).getFractionPrecision();
  Lng32 targetScale = (targetType.getTypeQualifier() == NA_NUMERIC_TYPE)
    ? ((NumericType &) targetType).getScale()
    : ((IntervalType &) targetType).getFractionPrecision();
  if (sourceScale == targetScale)
    return retTree;

  if ((sourceTypeQual == NA_INTERVAL_TYPE) &&
      (targetTypeQual == NA_INTERVAL_TYPE))
    {
    // the Cast operator can do interval to interval scaling itself
    retTree = new(wHeap()) Cast(retTree,&targetType);
    retTree->bindNode(generator->getBindWA());
    }
  else {
    if (((NumericType &) source.getType()).isExact() &&
        (NOT((NumericType &)targetType).isExact()) &&
	(targetScale != sourceScale)) {
    retTree = new(wHeap()) Cast(retTree,&targetType);
    retTree->bindNode(generator->getBindWA());

    if (handleUnsupportedCast((Cast*)retTree))
      return NULL;

    retTree->markAsPreCodeGenned();
    retTree = scaleBy10x(retTree->getValueId(), targetScale - sourceScale);
    }
    else
      retTree = scaleBy10x(source, targetScale - sourceScale);
  };

  retTree = retTree->preCodeGen(generator);

  return retTree;
}

/////////////////////////////////////////////////////////////////
// if the source is an interval, this function returns an expr
// tree to convert the source type to numeric.
/////////////////////////////////////////////////////////////////
ItemExpr * ExpGenerator::convertIntervalToNumeric(const ValueId & source)
{
  ItemExpr * retTree = source.getItemExpr();
  //
  // If the source is not an interval, return the source tree unchanged.
  //
  if (source.getType().getTypeQualifier() != NA_INTERVAL_TYPE)
    return retTree;
  IntervalType& sourceInterval = (IntervalType&) source.getType();
  //
  // If the source interval does not consist of only a single datetime field,
  // cast it to an interval consisting of only the end field.
  //
  if (sourceInterval.getStartField() != sourceInterval.getEndField())
    retTree = new(wHeap())
      Cast(retTree, new(wHeap())
	   SQLInterval(wHeap(), sourceInterval.supportsSQLnull(),
		       sourceInterval.getEndField(),
		       sourceInterval.getTotalPrecision() -
		       sourceInterval.getFractionPrecision(),
		       sourceInterval.getEndField(),
		       sourceInterval.getFractionPrecision()));
  //
  // Convert the source interval to numeric, bind the tree, and return it.
  //
  const Int16 DisAmbiguate = 0;
  retTree = new(wHeap())
    Cast(retTree, new(wHeap()) SQLNumeric(wHeap(), TRUE, /* signed */
				     sourceInterval.getTotalPrecision(),
				     sourceInterval.getFractionPrecision(),
				     DisAmbiguate, // added for 64bit proj.
				     sourceInterval.supportsSQLnull()));
  retTree->bindNode(generator->getBindWA());

  return retTree;
}

/////////////////////////////////////////////////////////////////
// if the source is a numeric, this function returns an expr
// tree to convert the source type to interval.
/////////////////////////////////////////////////////////////////
ItemExpr * ExpGenerator::convertNumericToInterval(const ValueId & source,
                                                  const NAType & targetType)
{
  ItemExpr * retTree = source.getItemExpr();
  //
  // If the source is not numeric or the target type is not an interval,
  // return the source tree unchanged.
  //
  if ((source.getType().getTypeQualifier() != NA_NUMERIC_TYPE) ||
      (targetType.getTypeQualifier() != NA_INTERVAL_TYPE))
    return retTree;
  NumericType& sourceNumeric = (NumericType&) source.getType();
  IntervalType& targetInterval = (IntervalType&) targetType;

  //
  // Factor the source so that its scale matches the fractional precision of
  // the target, and cast the result to an interval consisting of the target's
  // end field.
  //

  SQLInterval *interval =
    new(wHeap()) SQLInterval(wHeap(), targetInterval.supportsSQLnull(),
                    targetInterval.getEndField(),
                    targetInterval.getTotalPrecision() -
                      targetInterval.getFractionPrecision(),
                    targetInterval.getEndField(),
                    targetInterval.getFractionPrecision());

  retTree = ((UInt32) sourceNumeric.getScale() !=
             targetInterval.getFractionPrecision())
            ? matchScales(retTree->getValueId(), *interval)
            : new(wHeap()) Cast(retTree, interval);
  //
  // Bind the tree and return it.
  //
  retTree->bindNode(generator->getBindWA());
  return retTree;
}

/////////////////////////////////////////////////////////////////
// if the source and target are intervals with different end fields,
// this function returns an expr tree to convert the source type to
// match the target's end field.
/////////////////////////////////////////////////////////////////
ItemExpr * ExpGenerator::matchIntervalEndFields(const ValueId & source,
                                                const NAType & targetType)
{
  ItemExpr * retTree = source.getItemExpr();
  //
  // If either the source or target is not an interval, or if they both have
  // the same end fields, return the source tree unchanged.
  //
  if ((source.getType().getTypeQualifier() != NA_INTERVAL_TYPE) ||
      (targetType.getTypeQualifier() != NA_INTERVAL_TYPE))
    return retTree;
  IntervalType& sourceInterval = (IntervalType&) source.getType();
  IntervalType& targetInterval = (IntervalType&) targetType;
  if (sourceInterval.getEndField() == targetInterval.getEndField())
    return retTree;

  // Create a Cast node to convert to the target type.
  retTree = new(wHeap()) Cast(retTree,&targetType);
  retTree->bindNode(generator->getBindWA());
  return retTree;
}


// input is a ValueIdSet of aggregate nodes
short ExpGenerator::generateAggrExpr(const ValueIdSet &val_id_set,
				     ex_expr::exp_node_type /* node_type */,
				     ex_expr ** expr,
				     short gen_last_clause,
				     NABoolean groupByOperation,
                                     const ValueIdSet *addedInitSet/*optional*/
                                     )
{
  ///////////////////////////////////////////////////////////////////////
  // 3 expressions are generated to evaluate the aggregate.
  // They are used to initialize, evaluate the aggr for each row,
  // and finalize with no rows (move nulls)
  //
  // Different expressions are generated depending on whether operand
  // is nullable or non-nullabls.
  //
  // for MIN, MAX and SUM aggregates.
  //
  // Case 1: when the operand is non-nullable.
  //         For non-grouping operation,
  //         NULL value is moved to result if no rows are found.
  // Case 2: when operand is nullable.
  //         This case needs extra handling for the scenario where all
  //         values of operand that are part of the same group, contain
  //         null values. In this case, the result becomes null.
  //
  //
  // Initialize Expr:
  //
  //                  (converted to)
  // Case 1:
  //   Sum           --------------->   Aggr = 0
  //   Min           --------------->   Aggr = Max value for that datatype
  //   Max           --------------->   Aggr = Min value for that datatype
  //
  // Case 2:
  //   Min/Max/Sum   --------------->   Aggr = NULL
  //
  // Case 1 and 2:
  //   Count         --------------->   Aggr = 0
  //   One/Any True  --------------->   Aggr = FALSE.
  //
  //
  // Perrec Eval Expr:
  // =================
  //
  // MIN Aggregate:
  // Case 1:
  //   Min(A)   ------> AggrMinMax(A, "A < Min(A)");
  // Case 2:
  //   Min(A)   ------> Case
  //                       When Min(A) is NULL then Min(A) = A
  //                       When A is not null AND A < Min(A) Then Min(A) = A
  //                    End
  //
  // MAX Aggregate:
  // Case 1:
  //   Max(A)   ------> AggrMinMax(A, "A > Max(A)");
  // Case 2:
  //   Max(A)   ------> Case
  //                       When Max(A) is NULL then Max(A) = A
  //                       When A is not null AND A > Max(A) Then Max(A) = A
  //                    End
  //
  // SUM Aggregate:
  // Case 1:
  //   Sum(A)   ------> Sum(A) + A
  //
  // Case 2:
  //   Sum(A)   ------> Case
  //                       When Sum(A) is NULL then Sum(A) = A
  //                       When A is not null Then Sum(A) = Sum(A) + A
  //                    End
  //
  // COUNT(A) Aggregate:
  // Case 1:
  //   Count(A)     ------>  Count(A) + 1;
  //
  // Case 2:
  //   Count(A)    -------> Count(A) + Case
  //                                     When A is not null then 1 else 0
  //                                   End
  //
  // Other aggregates, all cases:
  //   Count(*) ------> Count(*) = Count(*) + 1
  //   OneTrue(A)  ------> Aggr = TRUE
  //   AnyTrue(A)  ------> Case
  //                         When A is True then TRUE  (caller short circuits)
  //                         When A is Unknown then NULL
  //                       End
  //
  //
  // Finalize (no rows found). Case 1(with no groupBy) and 2:
  //
  //   Non-count Aggr = NULL
  //
  ////////////////////////////////////////////////////////////////////////


  // generate code to evaluate the initialize expression
  ValueId val_id;
  ItemExpr * newExpr = NULL;
  ValueIdSet * newValIdSet = new(wHeap()) ValueIdSet();
  MapInfo * map_info;

  CollIndex num_aggr_entries = val_id_set.entries();

  short case_;
  for (val_id = val_id_set.init(); val_id_set.next(val_id); val_id_set.advance(val_id))
    {
      ItemExpr * item_expr = val_id.getValueDesc()->getItemExpr();
      if (NOT item_expr->child(0)->castToItemExpr()->getValueId().getType().supportsSQLnull())
	case_ = 1;
      else
	case_ = 2;

      // the original value id list contains the aggregate nodes
      // which are being converted to an equivalent tree in some cases
      // to compute their values. Mark the original aggr nodes
      // to indicate that code has been generated for them.
      // We don't want to generate aggregate clauses for the
      // original aggregate nodes.

      switch (item_expr->getOperatorType())
	{
	case ITM_MIN:
	case ITM_MAX:
	case ITM_SUM:
	  {
	    generator->getMapInfo(val_id)->codeGenerated();
	    ItemExpr * initVal;
	    if (case_ == 1)
		{
		  if (item_expr->getOperatorType() == ITM_MIN)
		    initVal = getMinMaxValue(val_id.getType(),
					     -2/*max without null*/
					     );
		  else if (item_expr->getOperatorType() == ITM_MAX)
		    initVal = getMinMaxValue(val_id.getType(),
					     0 /* min value */
					     );
		  else //SUM
		    {
		      if (val_id.getType().getTypeQualifier() == NA_INTERVAL_TYPE)
			{
			  Int64 zero = 0;
			  NAType *type = val_id.getType().newCopy(wHeap());
			  type->setNullable(FALSE);
			  NAString * nas = new (wHeap()) NAString("0", wHeap());
			  initVal =
			    new(wHeap()) ConstValue(type,
						    (void *)&zero,
						    type->getNominalSize(),
						    nas);
			}
		      else
	  		initVal = new(wHeap()) ConstValue(0);
		  }
		  newExpr = new(wHeap())
		    Cast(initVal, &(val_id.getType()));
		}
	    else // Case 2
	      {
		initVal = generateNullConst(val_id.getType());
		NAType *newType
		  = item_expr->getValueId().getType().newCopy(wHeap());
		newType->setSQLnullFlag();

		newExpr = new(wHeap())
		  Cast(initVal, newType );
	      }

	  }
	  break;

	case ITM_COUNT:
	case ITM_COUNT_NONULL:
	  {
	    generator->getMapInfo(val_id)->codeGenerated();
            newExpr = new(wHeap())
              Cast(new(wHeap()) ConstValue(0), &(val_id.getType()));
	  }
	  break;

	case ITM_ONE_TRUE:
	case ITM_ANY_TRUE:
	  {
	    GenAssert((num_aggr_entries == 1), "Cannot have more than one 'special' aggregates.");

	    generator->getMapInfo(val_id)->codeGenerated();

	    // move boolean FALSE to result
	    newExpr = new(wHeap()) BoolVal(ITM_RETURN_FALSE);
	  }
	  break;

	case ITM_ANY_TRUE_MAX:
	  {
	    GenAssert((num_aggr_entries == 1), "Cannot have more than one 'special' aggregates.");

	    // move boolean FALSE to result
	    newExpr = new(wHeap()) BoolVal(ITM_RETURN_FALSE);
	  }
	  break;

	case ITM_ONE_ROW:
	  {

	    // move null value to all outputs
            ValueId outvid;
            ValueIdSet init_val_id_set;
           
            init_val_id_set += *addedInitSet; // other values to be init'ed.

            for ( outvid = init_val_id_set.init();
                           init_val_id_set.next(outvid);
                           init_val_id_set.advance(outvid) )
            {
              NAType *outType = outvid.getType().newCopy(wHeap());

              // Should be nullable, since one row aggregates are inst-null'ed.
              // except if this is an output from the aggregate node, plus it's
              // boolean.
              if (!outType->supportsSQLnull())
              {
               // The following assertion has been commented out because of the case where
               // although subqueries are nullable the statement enclosing the subquery is
               // not but if the subquery does not contain reference to the outer tables then
               // the whole statement can be pushed down to this side of the tree.
               // GenAssert(outType->getTypeQualifier() == NA_BOOLEAN_TYPE),
               // "outputs from one_row_aggr node should be nullable or BOOLEAN"
               // );
                outType->setNullable(TRUE);
              }

              // Make a null const of the same type as the value to be init'ed.
	      ConstValue * nullv = generateNullConst(outvid.getType());

              // Make a copy of the value, used in the generation of initExpr.
              newExpr = new(wHeap()) Cast(nullv,outType);
	      newExpr = newExpr->bindNode(generator->getBindWA());
	      newValIdSet->insert(newExpr->getValueId());

              // Attach the destination location to the copy. Used later to
              // generate the initExpr.
              Attributes *attr = generator->getMapInfo(outvid)->getAttr();
	      map_info = generator->addMapInfo(newExpr->getValueId(),attr);

              // seems unnecessary, since attr is passed in the call above. ??
	      // (map_info->getAttr())->copyLocationAttrs(attr);

            } // for

            newExpr = NULL;
	  }
	  break;

	case ITM_PIVOT_GROUP:
	  {
	    generator->getMapInfo(val_id)->codeGenerated();
            newExpr = new(wHeap())
              Cast(new(wHeap()) ConstValue(""), &(val_id.getType()));
	  }
	  break;

	default:
	  newExpr = NULL;
	  break;

	}

      if (newExpr != NULL)
	{
	  newExpr->bindNode(generator->getBindWA());
	  newValIdSet->insert(newExpr->getValueId());

	  // assign the data attributes of the aggr node to the convert node.
	  // This will make the result of initialization be moved to the
	  // aggregate location.
          Attributes * attr = generator->getMapInfo(val_id)->getAttr();
	  map_info = generator->addMapInfo(newExpr->getValueId(), attr);
	  (map_info->getAttr())->copyLocationAttrs(attr);
          if(attr->isForceFixed())
            (map_info->getAttr())->setForceFixed();
	}

    }

  ex_expr * init_expr = 0;
  generateSetExpr(*newValIdSet, ex_expr::exp_ARITH_EXPR, &init_expr);

  delete newValIdSet;

  // generate expression to move in null value as the aggr result
  // on an empty set. Applies to all non-count aggr for non-grouping
  // operation.
  ex_expr * final_null_expr = 0;
  if (groupByOperation == FALSE)
    {
      newValIdSet = new(wHeap()) ValueIdSet();
      for (val_id = val_id_set.init(); val_id_set.next(val_id); val_id_set.advance(val_id))
	{
	  ItemExpr * item_expr = val_id.getValueDesc()->getItemExpr();
          if(!item_expr->isAnAggregate()) {
            newExpr = NULL;
          } else {
            // part of fix to genesis case 10-070628-2258, soln 10-070610-5412:
            // guard against boundary case queries against empty table t
            //   "select count(distinct i), count(*) from t"
            //   "select avg(distinct i), count(*) from t"
            //   "select sum(distinct i), count(*) from t"
            // which transforms 
            //   "count(*)" into "sum(count(*))"
            // and can emit false alarms
            //   "error 8421 null cannot be assigned to a not null column"
            // when t is an empty table. 
            // The "sum" in the rewritten aggregate transformed by 
            // Aggregate::rewriteForStageEvaluation() should be treated like the
            // "count(*)" case below so it returns 0 when t is an empty table.
            switch (((Aggregate*)item_expr)->getEffectiveOperatorType())
            {
            case ITM_SUM:
            case ITM_MIN:
            case ITM_MAX:
            case ITM_ONEROW:
            case ITM_PIVOT_GROUP:
	      {
		generator->getMapInfo(val_id)->codeGenerated();
		ConstValue * const_value =
		  generateNullConst(val_id.getType());

		NAType *newType
		  = item_expr->getValueId().getType().newCopy(wHeap());
		newType->setSQLnullFlag();

		newExpr = new(wHeap())
		  Cast(const_value, newType );

	      }
	      break;

	    case ITM_COUNT:
	    case ITM_COUNT_NONULL:
	    case ITM_ONE_TRUE:
	    case ITM_ANY_TRUE:
	    case ITM_ANY_TRUE_MAX:
	    case ITM_ONE_ROW:
	      {
		newExpr = NULL;
	      }
	      break;

	    default:
	      newExpr = NULL;
	      break;

	    }
          }
	  if (newExpr != NULL)
	    {
	      newExpr->bindNode(generator->getBindWA());
	      newValIdSet->insert(newExpr->getValueId());

	      // assign the data attributes of the aggr node to the convert node.
	      // This will make the final null value to be moved to the
	      // aggregate location.
              Attributes * attr = generator->getMapInfo(val_id)->getAttr();
	      map_info = generator->addMapInfo(newExpr->getValueId(), attr);
	      (map_info->getAttr())->copyLocationAttrs(attr);
              if(attr->isForceFixed())
                (map_info->getAttr())->setForceFixed();
	    }
          
	} // for loop

      generateSetExpr(*newValIdSet, ex_expr::exp_ARITH_EXPR,
                      &final_null_expr);

      delete newValIdSet;
    }

  newValIdSet = new(wHeap())  ValueIdSet();

  // generate expression tree to evaluate the perrec aggr expression
  newExpr = 0;
  NABoolean raiseErrorGenerated = FALSE;
  ValueIdList NullSwitcharooVidList;
  for (val_id = val_id_set.init(); val_id_set.next(val_id); val_id_set.advance(val_id))
    {
      NABoolean nullSwitcharoo = FALSE;
      ItemExpr * item_expr = val_id.getValueDesc()->getItemExpr();
      if (NOT item_expr->child(0)->castToItemExpr()->getValueId().getType().supportsSQLnull())
	case_ = 1;
      else
	case_ = 2;

      switch (item_expr->getOperatorType())
	{
	case ITM_MIN:
	  {
	    if (case_ == 1)
	      {
		newExpr =
		  createExprTree("@A2 < @A1", 0, 2, item_expr,
				 item_expr->child(0));
		newExpr =
		  new(wHeap()) AggrMinMax(item_expr->child(0), newExpr);
		if (item_expr->getValueId().getType().supportsSQLnull())
		  nullSwitcharoo = TRUE;
	      } // Case 1
	    else
	      { //Case 2
                newExpr =
                  createExprTree("@A1 IS NULL OR"
                                 "(@A2 IS NOT NULL AND @A2 < @A1)",
                                 0, 2, item_expr, item_expr->child(0));
		newExpr =
                  new(wHeap()) AggrMinMax(item_expr->child(0), newExpr);
	      } // Case 2
	  }
  	  break;

	case ITM_MAX:
	  {
	    if (case_ == 1)
	      {
		newExpr = createExprTree("@A2 > @A1",
                                         0, 2, item_expr, item_expr->child(0));
		newExpr =
                  new(wHeap()) AggrMinMax(item_expr->child(0), newExpr);
		if (item_expr->getValueId().getType().supportsSQLnull())
		  nullSwitcharoo = TRUE;
	      }
	    else
	      {
                newExpr =
                  createExprTree("@A1 IS NULL OR"
                                 "(@A2 IS NOT NULL AND @A2 > @A1)",
                                 0, 2, item_expr, item_expr->child(0));
		newExpr =
                  new(wHeap()) AggrMinMax(item_expr->child(0), newExpr);
	      }
	  }
	  break;

	  // SUM --
	  //
	case ITM_SUM:
	  {
	    // The sum expression is not nullable. Cast the expression
	    // to the same width as the SUM but retain it non-nullability.
	    // BiArithSum has the following semantics:
	    //      if(operandA IS NOT NULL)
	    //        if(operandB IS NOT NULL) SUM = operandA + operandB
	    //        else SUM = operandA
	    //
	    if (case_ == 1)
	      {
		NAType *sumType
		  = item_expr->getValueId().getType().newCopy(wHeap());
		sumType->setNullable(FALSE);
		if (item_expr->child(0)->getValueId().getType().supportsSQLnull())
		  newExpr = new(wHeap()) Cast(item_expr->child(0),
					      sumType, ITM_CAST);
		else
		  newExpr = item_expr->child(0);
		//newExpr = new(wHeap()) Cast(item_expr->child(0),
		//			    sumType,
		//			    ITM_CAST);
		newExpr = new(wHeap()) BiArithSum(ITM_PLUS,
						  newExpr,
						  item_expr);
	      }
	    // The SUM expression is nullable. Cast the expression to the
	    // same width as the SUM. Then apply BiArithSum as above.
	    //
	    else // case_ == 2
	      {
		const NAType &sumType = item_expr->getValueId().getType();
		newExpr = new(wHeap()) Cast(item_expr->child(0),
					    &sumType,
					    ITM_CAST);
		newExpr = new(wHeap()) BiArithSum(ITM_PLUS,
					       newExpr,
					       item_expr);
	      }
	  }
	  break;

	  // COUNT (everything)
	  // BiArithCount has the following semantics:
	  //      if(operandA IS NOT NULL)
	  //        if(operandB IS NOT NULL) SUM = operandB + 1
	  //        else SUM = 1;
	  //
	case ITM_COUNT:
	  {
	    NAType *sumType
	      = item_expr->getValueId().getType().newCopy(wHeap());
	    sumType->setNullable(FALSE);

	    newExpr = new(wHeap()) ConstValue(1);
	    newExpr = new(wHeap()) Cast(newExpr, sumType, ITM_CAST);
	    newExpr = new(wHeap()) BiArithSum(ITM_PLUS,
					      newExpr,
					      item_expr);
	  }
	  break;

	  // COUNT (non nulls)
	  //
	case ITM_COUNT_NONULL:
	  {
	    // The COUNT attribute is non nullable.
	    //
	    NAType *sumType
	      = item_expr->getValueId().getType().newCopy(wHeap());
	    sumType->setNullable(FALSE);
	    if (case_ == 1)
	      {
		newExpr = new(wHeap()) ConstValue(1);
		newExpr = new(wHeap()) Cast(newExpr, sumType, ITM_CAST);
		newExpr = new(wHeap()) BiArithSum(ITM_PLUS,
						  newExpr,
						  item_expr);
	      }
	    // The COUNT attribute is nullable.
	    //
	    else
	      {
		ItemExpr *constZero = new(wHeap()) ConstValue(0);
		ItemExpr *constOne = new(wHeap()) ConstValue(1);
		newExpr = new(wHeap()) ReplaceNull(item_expr->child(0),
						   constOne,
						   constZero);
		newExpr = new(wHeap()) Cast(newExpr, sumType, ITM_CAST);
		newExpr = new(wHeap()) BiArithSum(ITM_PLUS,
						  newExpr,
						  item_expr);
	      }
	  }
	  break;

	case ITM_ONE_TRUE:
	  {
	    // move boolean TRUE to result
	    newExpr = new(wHeap()) BoolVal(ITM_RETURN_TRUE);

	    // assign the data attributes of the original aggr node
	    // to the newExpr.
	    // This will make the result of newExpr be moved to the
	    // original aggregate location.
	    newExpr->bindNode(generator->getBindWA());
            Attributes * attr = generator->getMapInfo(val_id)->getAttr();
	    map_info = generator->addMapInfo(newExpr->getValueId(), attr);
	    (map_info->getAttr())->copyLocationAttrs(attr);

	    // generate the expression that would return TRUE back
	    // to the caller which would short circuit.
	    newExpr = new(wHeap())BoolResult(newExpr);
	    newExpr->bindNode(generator->getBindWA());
	    newValIdSet->insert(newExpr->getValueId());

	    // no need to assign data attrs to newExpr anymore.
	    newExpr = 0;

	    // no need to generate last clause
	    gen_last_clause = 0;
	  }
	  break;

	case ITM_ANY_TRUE:
	  {
	    newExpr = createExprTree("CASE WHEN @B1 IS TRUE THEN @A2 WHEN @B1 IS UNKNOWN THEN @A3 ELSE @A4 END", 0,
				     4,
				     item_expr->child(0),
				     new(wHeap()) BoolVal(ITM_RETURN_TRUE),
				     new(wHeap()) BoolVal(ITM_RETURN_NULL),
				     0);

	    // assign the data attributes of the original aggr node
	    // to the newExpr.
	    // This will make the result of newExpr be moved to the
	    // original aggregate location.
	    newExpr->bindNode(generator->getBindWA());
            Attributes * attr = generator->getMapInfo(val_id)->getAttr();
	    map_info = generator->addMapInfo(newExpr->getValueId(), attr);
	    (map_info->getAttr())->copyLocationAttrs(attr);

	    // generate the expression that would return TRUE back
	    // to the caller which would short circuit.
	    // BTW, BoolResult returns TRUE only if its operand
	    // has evaluated to TRUE.
	    newExpr = new(wHeap())  BoolResult(newExpr);
	    newExpr->bindNode(generator->getBindWA());
	    newValIdSet->insert(newExpr->getValueId());

	    // no need to assign data attrs to newExpr anymore.
	    newExpr = 0;

	    // no need to generate last clause
	    gen_last_clause = 0;
	  }
	  break;

	case ITM_ONE_ROW:
	  {
            // Just create a dummy ONE_ROW aggregate node. Nothing is going
            // to be done on the atp at runtime anyway. We just want a one_row
            // clause generated so that we can keep track of the number of rows
            // returned so far.
            //
            newExpr = new(wHeap()) Aggregate(ITM_ONE_ROW,
                                             new(wHeap()) ConstValue(0),
                                             FALSE,
                                             ITM_ONE_ROW,' ');
	    newExpr->bindNode(generator->getBindWA());

            // Just create an entry in the map table. It won't be used anyway.
	    map_info = generator->addMapInfo(newExpr->getValueId(),0);

            // Just use the same location attrs as the original aggr. It won't
            // be used anyway.
            Attributes * attr = generator->getMapInfo(val_id)->getAttr();
	    (map_info->getAttr())->copyLocationAttrs(attr);

	     // So that a one_row_aggr clause can be generated in a later call.
            newValIdSet->insert(newExpr->getValueId());
            newExpr = NULL;

	  }
	  break;

	case ITM_ANY_TRUE_MAX:
	  {
	    // no conversion is done for these cases
	    newExpr = item_expr;
	  }
	  break;

	case ITM_PIVOT_GROUP:
	  {
            PivotGroup * pg = (PivotGroup*)item_expr;
            newExpr = 
              new(wHeap()) PivotGroup(ITM_PIVOT_GROUP, item_expr->child(0), 
                                      pg->delim(), pg->orderBy(), pg->reqdOrder(),
                                      pg->maxLen(), pg->isDistinct());
	  }
	  break;

	case ITM_ONEROW:
	  {
	   generator->getMapInfo(val_id)->codeGenerated();

	    const NAType &oneRowType = item_expr->getValueId().getType();
	    newExpr = new(wHeap()) Cast(item_expr->child(0),
					&oneRowType,
					ITM_CAST);
	   if (NOT raiseErrorGenerated)
	   {
	     ItemExpr * raiseError = new(wHeap()) Aggregate(ITM_ONE_ROW,
					       new(wHeap()) ConstValue(0),
					       FALSE,
					       ITM_ONE_ROW,' ');
	      raiseError->bindNode(generator->getBindWA());
	      newExpr = new(wHeap()) ItmBlockFunction(raiseError,newExpr);
	      raiseErrorGenerated = TRUE ;
	   }
	  }
	  break ;


	default:
	  newExpr = NULL;
	  break;

	}

      // assign the data attributes of the aggr node to the newExpr.
      // This will make the result of newExpr be moved to the
      // aggregate location.
      if (newExpr != NULL)
	{
	  newExpr->bindNode(generator->getBindWA());
	  newValIdSet->insert(newExpr->getValueId());
	  newExpr->getValueId().changeType(&(val_id.getType()));

          Attributes * attr = generator->getMapInfo(val_id)->getAttr();
	  map_info = generator->addMapInfo(newExpr->getValueId(), attr);
	  (map_info->getAttr())->copyLocationAttrs(attr);
          if(attr->isForceFixed())
            (map_info->getAttr())->setForceFixed();

	  if (nullSwitcharoo)
	    {
	      NullSwitcharooVidList.insert(val_id);
	      attr->setNullFlag(0);
	    }
	}

    }

  // generate expressions to evaluate GROUPING clause
  ValueIdSet groupingValIdSet;
  for (val_id = val_id_set.init(); 
       val_id_set.next(val_id); 
       val_id_set.advance(val_id))
    {
      ItemExpr * item_expr = val_id.getValueDesc()->getItemExpr();
      if (item_expr->getOperatorType() == ITM_GROUPING)
        {
          generator->getMapInfo(val_id)->codeGenerated();
          Aggregate * ag = (Aggregate*)item_expr;
          ItemExpr * groupingExpr = 
            new(wHeap()) AggrGrouping(ag->getRollupGroupIndex());
	  groupingExpr->bindNode(generator->getBindWA());
	  groupingValIdSet.insert(groupingExpr->getValueId());
          
          Attributes * attr = generator->getMapInfo(val_id)->getAttr();
          map_info = generator->addMapInfo(groupingExpr->getValueId(), attr);
          (map_info->getAttr())->copyLocationAttrs(attr);
        }
    } // for

  ex_expr * grouping_expr = NULL;
  if (NOT groupingValIdSet.isEmpty())
    {
      unsigned short pcm = getPCodeMode();
      setPCodeMode(ex_expr::PCODE_NONE);

      generateSetExpr(groupingValIdSet, ex_expr::exp_ARITH_EXPR,
                      &grouping_expr);

      setPCodeMode(pcm);
    }

  ex_expr * perrec_expr = NULL;
  generateSetExpr(*newValIdSet, ex_expr::exp_ARITH_EXPR, &perrec_expr);

  *expr = new(getSpace()) AggrExpr(init_expr,
				   perrec_expr,
				   0, //final_expr,
				   final_null_expr,
                                   grouping_expr);

  for (CollIndex ns = 0; ns < NullSwitcharooVidList.entries(); ns++)
    {
      map_info = generator->getMapInfo(NullSwitcharooVidList[ns]);
      map_info->getAttr()->setNullFlag(-1);
    }

  return 0;
}

// input is a ValueId which points to an arithmetic expression.
short ExpGenerator::generateArithExpr(const ValueId & val_id,
				      ex_expr::exp_node_type node_type,
				      ex_expr ** expr)

{
  initExprGen();
  startExprGen(expr, node_type);

  // generate code for this node and the tree under it
  ItemExpr * temp =
    val_id.getValueDesc()->getItemExpr()->preCodeGen(generator);
  if (! temp)
    return -1;

  temp = temp->preCodeGen(generator);
  ItemExpr * newTemp = NULL;
  Lng32 rc = foldConstants(temp, &newTemp);
  if ((rc == 0) &&
      (newTemp))
    temp = newTemp;

  temp->codeGen(generator);

  endExprGen(expr, -1);

  return 0;
}

// Deterimine if bulk move for Aligned Format can be done or not.
// If so, then generate the new src / tgt bulk move attribute
short ExpGenerator::generateBulkMoveAligned(
                                ValueIdList inValIdList,
                                ValueIdList &outValIdList,
                                UInt32       tupleLength,
                                Int32       *bulkMoveSrcStartOffset //IN(O)
                                )
{
  // turn bulk move off for aligned format for now until more testing is done
  if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BULK_MOVE) == DF_OFF)
    return 0;

  NABoolean bulkMove = TRUE;

  MapInfo * tgtMapInfo = generator->getMapInfoAsIs(inValIdList[0]);
  MapInfo * srcMapInfo =
               generator->getMapInfoAsIs(inValIdList[0].getItemExpr()->
                                 getChild(0)->castToItemExpr()->getValueId());

  GenAssert( tgtMapInfo, "Null target map info");
  GenAssert( srcMapInfo, "Null source map info");

  Attributes * tgtAttr = tgtMapInfo->getAttr();
  Attributes * srcAttr = srcMapInfo->getAttr();

  UInt32 firstTgtAtp   = tgtAttr->getAtp();
  UInt32 firstSrcAtp   = srcAttr->getAtp();
  UInt32 firstTgtAtpIx = tgtAttr->getAtpIndex();
  UInt32 firstSrcAtpIx = srcAttr->getAtpIndex();

  // Determine if bulk move should be considered for nullable columns.
  // Variable length columns can't be done yet since each row will be
  // a different length (based on the values of the variable column values)
  // and the bulk move attribute can only have 1 length - max row size.??????????????????????
  NABoolean bulkMoveNullVarchar =
                    (CmpCommon::getDefault(BULK_MOVE_NULL_VARCHAR) == DF_ON);


  UInt32 voaOffset = 0;
  Int16 vcIndicatorLength=0;
  Int16 nullIndicatorLength=0;
  UInt32 alignment = 0;


  for (CollIndex i = 0; ((i < inValIdList.entries()) && (bulkMove)); i++)
  {
    tgtMapInfo = generator->getMapInfoAsIs(inValIdList[i]);
    if (tgtMapInfo)
      tgtAttr = tgtMapInfo->getAttr();
    else
      tgtAttr = 0;

    srcMapInfo =
      generator->getMapInfoAsIs(inValIdList[i].
                   getItemExpr()->getChild(0)->castToItemExpr()->getValueId());

    if (srcMapInfo)
      srcAttr = srcMapInfo->getAttr();
    else
      srcAttr = 0;

    if (srcAttr && srcAttr->getVCIndicatorLength() && srcAttr->getVoaOffset()> voaOffset)
    {
      voaOffset = srcAttr->getVoaOffset();
      vcIndicatorLength = srcAttr->getVCIndicatorLength();
      nullIndicatorLength = srcAttr->getNullIndicatorLength();

      alignment =  (srcAttr->isSQLMXAlignedFormat() &&
                          (srcAttr->getNextFieldIndex() == ExpOffsetMax))
                          ? ExpAlignedFormat::ALIGNMENT : 0;

    }
    // Must have both the source and target attributes in the same data format
    // to do a bulk move.  Target was checked before getting into this routine.
    if ((srcAttr && (NOT srcAttr->isSQLMXAlignedFormat())) ||
       (tgtAttr &&  (NOT tgtAttr->isSQLMXAlignedFormat())))
      return 0;

    if ((! tgtAttr)                                              ||
        (! srcAttr)                                              ||
        (tgtAttr->getTupleFormat()       != srcAttr->getTupleFormat()) ||
        (tgtAttr->getDatatype()          != srcAttr->getDatatype())    ||
        ((NOT bulkMoveNullVarchar) &&
         (tgtAttr->getNullFlag() || srcAttr->getNullFlag()))           ||
        (tgtAttr->getNullFlag()          != srcAttr->getNullFlag())    ||
        (tgtAttr->getScale()             != srcAttr->getScale())       ||
        (tgtAttr->getPrecision()         != srcAttr->getPrecision())   ||
        (tgtAttr->getLength()            != srcAttr->getLength())      ||
        (srcAttr->getAtpIndex() == 0)                                  ||
        (srcAttr->getAtpIndex() == 1)                                  ||
        ((UInt32)tgtAttr->getAtp()       != firstTgtAtp)          ||
        ((UInt32)srcAttr->getAtp()       != firstSrcAtp)          ||
        ((UInt32)tgtAttr->getAtpIndex()  != firstTgtAtpIx)        ||
        ((UInt32)srcAttr->getAtpIndex()  != firstSrcAtpIx)        ||
        (tgtAttr->getOffset()            != srcAttr->getOffset()) ||
        (tgtAttr->getNullIndOffset()     != srcAttr->getNullIndOffset())  ||
        (tgtAttr->getNullBitIndex()      != srcAttr->getNullBitIndex())   ||
        //(srcAttr->getVCIndicatorLength() > 0)                          ||
	(tgtAttr->getVCIndicatorLength() != srcAttr->getVCIndicatorLength()) ||
        (tgtAttr->getVCLenIndOffset()    != srcAttr->getVCLenIndOffset()) ||
        (tgtAttr->getNullIndOffset()     != srcAttr->getNullIndOffset())   ||
        (tgtAttr->getVoaOffset()         != srcAttr->getVoaOffset())  ||
        (srcAttr->isAddedCol()) ||
        (tgtAttr->isAddedCol()))
    bulkMove = FALSE;
  }

  if (bulkMove == TRUE)
  {
    for (CollIndex i = 0; (i < inValIdList.entries()); i++)
    {
      outValIdList.insert(inValIdList[i].
                    getItemExpr()->getChild(0)->castToItemExpr()->getValueId());
    }

    // 5/21/98: Since the tuple is to be moved as a byte array,
    // the current SQLChar constructor should be sufficient.

    NAType * type = new(generator->wHeap()) SQLChar(generator->wHeap(), (Int32)tupleLength, FALSE);
    ItemExpr * bulkMoveSrc = new(generator->wHeap()) NATypeToItem(type);
    //ItemExpr * bulkMoveTgt = new(generator->wHeap()) Convert (bulkMoveSrc);
    ItemExpr * bulkMoveTgt = new(generator->wHeap()) Convert (bulkMoveSrc,
                                                              voaOffset,
                                                              vcIndicatorLength,
                                                              nullIndicatorLength,
                                                              alignment);

    bulkMoveTgt->synthTypeAndValueId();

    Attributes * tgtAttr = generator->getMapInfo(inValIdList[0])->getAttr();
    Attributes * srcAttr =
      generator->getMapInfo(inValIdList[0].getItemExpr()->getChild(0)->castToItemExpr()->getValueId())->getAttr();

    Attributes * bulkMoveTgtAttr =
                  generator->addMapInfo(bulkMoveTgt->getValueId(), 0)->getAttr();
    Attributes * bulkMoveSrcAttr =
                  generator->addMapInfo(bulkMoveSrc->getValueId(), 0)->getAttr();

    bulkMoveTgtAttr->copyLocationAttrs(tgtAttr);

    bulkMoveSrcAttr->copyLocationAttrs(srcAttr);

    // Bulk move is only done when the complete data row is the same for
    // all source attributes and target attributes.  The starting offset must
    // encompass the Aligned Format header thus set it to 0.
    UInt32 bulkSrcStartOffset = 0;
    UInt32 bulkTgtStartOffset = 0;

    bulkMoveSrcAttr->setOffset( bulkSrcStartOffset );
    bulkMoveTgtAttr->setOffset( bulkTgtStartOffset );

    // If orignal source was a varchar, then must change the bulk src/tgt
    // attribute vc indicator length back to 0 since these are now fixed
    // char's.
    if (srcAttr->isVariableLength())
    {
      bulkMoveSrcAttr->setVCIndicatorLength(0);
      bulkMoveTgtAttr->setVCIndicatorLength(0);
    }

    // Both source and destinations are no longer rowsets
    bulkMoveTgtAttr->setRowsetSize(0);
    bulkMoveSrcAttr->setRowsetSize(0);

    outValIdList.insert(bulkMoveTgt->getValueId());

    if (bulkMoveSrcStartOffset) // param passed in.
    {
      *bulkMoveSrcStartOffset = bulkSrcStartOffset;
    }
  }
  return 0;
}

// Generate a bulk move of the input value Ids if possible.  There are very
// specific conditions for this to be possible - listed below.
// This routine handles the Exploded Internal format and branches to
// the routine ExpGenerator::generateBulkMoveAligned if the target attribute
// has data format Compressed Internal format.
short ExpGenerator::generateBulkMove(ValueIdList inValIdList,
				     ValueIdList &outValIdList,
				     ULng32 tupleLength,
				     Lng32 *bulkMoveSrcStartOffset) //IN(O)
{
  if (bulkMoveSrcStartOffset) // param passed in.
    {
      *bulkMoveSrcStartOffset = -1;
    }

  // no need to check for bulk move if list is empty.
  if (inValIdList.entries() == 0)
    return 0;

  NABoolean bulkMove = TRUE;

  MapInfo * tgtMapInfo;
  MapInfo * srcMapInfo;

  Attributes * tgtAttr;
  Attributes * srcAttr;

  tgtMapInfo = generator->getMapInfoAsIs(inValIdList[0]);

  if (!tgtMapInfo)
    return 0; // must be in maptable to check for bulkmove attrs

  tgtAttr = tgtMapInfo->getAttr();

  srcMapInfo = generator->getMapInfoAsIs(inValIdList[0].getItemExpr()->getChild(0)->castToItemExpr()->getValueId());

  if (!srcMapInfo)
    return 0; // must be in maptable to check for bulkmove attrs

  srcAttr = srcMapInfo->getAttr();

  // If there is only 1 input value id and the associated attribute is
  // not nullable, then bulk move is not needed.
  if ((NOT bulkMoveSrcStartOffset)  // parameter not passed in
      && (inValIdList.entries() == 0)
      && (NOT srcAttr->getNullFlag()))
    return 0;

  // The Compressed Internal format (ie. the Aligned Row Format) has a
  // different criteria when bulk move can be applied.
  // See the method comments for details.
  if ( tgtAttr && tgtAttr->isSQLMXAlignedFormat() )
  {
    return generateBulkMoveAligned( inValIdList,
                                    outValIdList,
                                    tupleLength,
                                    bulkMoveSrcStartOffset );
  }

  // Now only the Exploded Internal format exists as a possibility.
  UInt32 firstTgtAtp = tgtAttr->getAtp();
  UInt32 firstSrcAtp = srcAttr->getAtp();
  UInt32 firstTgtAtpIx = tgtAttr->getAtpIndex();
  UInt32 firstSrcAtpIx = srcAttr->getAtpIndex();
  Int32 firstTgtOffset = tgtAttr->getOffset();
  Int32 firstSrcOffset = srcAttr->getOffset();
  Int32 firstTgtNullOffset = ExpOffsetMax;
  Int32 firstSrcNullOffset = ExpOffsetMax;
  Int32 firstTgtVCLenIndOffset = ExpOffsetMax;
  Int32 firstSrcVCLenIndOffset = ExpOffsetMax;

  // Determine if bulk move should be considered for nullable and/or variable
  // length column values.
  NABoolean bulkMoveNullVarchar =
                    (CmpCommon::getDefault(BULK_MOVE_NULL_VARCHAR) == DF_ON);

  for (CollIndex i = 0;
       ((i < inValIdList.entries()) && (bulkMove));
       i++)
    {
      tgtMapInfo = generator->getMapInfoAsIs(inValIdList[i]);
      if (tgtMapInfo)
	tgtAttr = tgtMapInfo->getAttr();
      else
	tgtAttr = 0;

      srcMapInfo =
	generator->getMapInfoAsIs(inValIdList[i].getItemExpr()->getChild(0)->castToItemExpr()->getValueId());
      if (srcMapInfo)
	srcAttr = srcMapInfo->getAttr();
      else
	srcAttr = 0;

      // initialize the first offset variables
      if (tgtAttr && tgtAttr->getNullFlag() && 
         (firstTgtNullOffset == ExpOffsetMax))
        firstTgtNullOffset = tgtAttr->getNullIndOffset();
      if (srcAttr && srcAttr->getNullFlag() && 
         (firstSrcNullOffset == ExpOffsetMax))
        firstSrcNullOffset = srcAttr->getNullIndOffset();
      if (tgtAttr && tgtAttr->isVariableLength() && 
         (firstTgtVCLenIndOffset == ExpOffsetMax))
        firstTgtVCLenIndOffset = tgtAttr->getVCLenIndOffset();
      if (srcAttr && srcAttr->isVariableLength() && 
         (firstSrcVCLenIndOffset == ExpOffsetMax))
        firstSrcVCLenIndOffset = srcAttr->getVCLenIndOffset();

      if ((! tgtAttr)                                                  ||
          (! srcAttr)                                                  ||
          ((NOT bulkMoveNullVarchar) &&
           (tgtAttr->getNullFlag() || srcAttr->getNullFlag()))         ||
          (tgtAttr->getNullFlag()    != srcAttr->getNullFlag())        ||
          (tgtAttr->getTupleFormat() != srcAttr->getTupleFormat())     ||
          (tgtAttr->getDatatype()    != srcAttr->getDatatype())        ||
          (tgtAttr->getScale()       != srcAttr->getScale())           ||
          (tgtAttr->getPrecision()   != srcAttr->getPrecision())       ||
          (tgtAttr->getLength()      != srcAttr->getLength())          ||
          ((UInt32)tgtAttr->getAtp() != firstTgtAtp)                   ||
          ((UInt32)srcAttr->getAtp() != firstSrcAtp)                   ||
          ((UInt32)tgtAttr->getAtpIndex() != firstTgtAtpIx)            ||
          ((UInt32)srcAttr->getAtpIndex() != firstSrcAtpIx)            ||
          ((tgtAttr->getOffset() - firstTgtOffset)
            != (srcAttr->getOffset() - firstSrcOffset))                ||
          (tgtAttr->getNullFlag() &&
            ((tgtAttr->getOffset() - tgtAttr->getNullIndOffset()) 
              != (srcAttr->getOffset() - srcAttr->getNullIndOffset())))  ||
          (tgtAttr->getNullFlag() &&
            ((tgtAttr->getNullIndOffset() - firstTgtNullOffset) 
              != (srcAttr->getNullIndOffset() - firstSrcNullOffset)))  ||    
          ((NOT bulkMoveNullVarchar) &&
           ((tgtAttr->getVCIndicatorLength() > 0) 
            || (srcAttr->getVCIndicatorLength() > 0)))                 ||
          (tgtAttr->isVariableLength() &&
           ((tgtAttr->getVCLenIndOffset() - firstTgtVCLenIndOffset)
             != (srcAttr->getVCLenIndOffset() - firstSrcVCLenIndOffset))) ||
          (srcAttr->getAtpIndex() == 0)     ||
          (srcAttr->getAtpIndex() == 1)     ||
          (srcAttr->isAddedCol())       ||
          (tgtAttr->isAddedCol()))
      bulkMove = FALSE;
  }

  if (bulkMove == TRUE)
    {
      for (CollIndex i = 0; (i < inValIdList.entries()); i++)
	{
	  outValIdList.insert(inValIdList[i].getItemExpr()->getChild(0)->castToItemExpr()->getValueId());
	}

// 5/21/98: Since the tuple is to be moved as a byte array,
// the current SQLChar constructor should be sufficient.

      NAType * type = new(generator->wHeap()) SQLChar(generator->wHeap(), tupleLength, FALSE);
      ItemExpr * bulkMoveSrc = new(generator->wHeap()) NATypeToItem(type);
      ItemExpr * bulkMoveTgt = new(generator->wHeap()) Convert (bulkMoveSrc);
      bulkMoveTgt->synthTypeAndValueId();

      Attributes * tgtAttr =
	generator->getMapInfo(inValIdList[0])->getAttr();
      Attributes * srcAttr =
	generator->getMapInfo(inValIdList[0].getItemExpr()->getChild(0)->castToItemExpr()->getValueId())->getAttr();

      Attributes * bulkMoveTgtAttr = generator->addMapInfo(bulkMoveTgt->getValueId(), 0)->getAttr();
      Attributes * bulkMoveSrcAttr = generator->addMapInfo(bulkMoveSrc->getValueId(), 0)->getAttr();

      bulkMoveTgtAttr->copyLocationAttrs(tgtAttr);

      bulkMoveSrcAttr->copyLocationAttrs(srcAttr);

      UInt32 bulkSrcStartOffset = srcAttr->getOffset();
      UInt32 bulkTgtStartOffset = tgtAttr->getOffset();

      // Since the src / tgt attributes may be nullable or variable
      // must get the correct beginning offset to use and set it in the
      // new corresponding bulk attribute.
      if ( srcAttr->getNullFlag() )
        bulkSrcStartOffset = srcAttr->getNullIndOffset();
      else if ( srcAttr->isVariableLength() )
        bulkSrcStartOffset = srcAttr->getVCLenIndOffset();
           
      if ( tgtAttr->getNullFlag() )
        bulkTgtStartOffset = tgtAttr->getNullIndOffset();
      else if ( tgtAttr->isVariableLength() )
        bulkTgtStartOffset = tgtAttr->getVCLenIndOffset();
            
      bulkMoveSrcAttr->setOffset( bulkSrcStartOffset );
      bulkMoveTgtAttr->setOffset( bulkTgtStartOffset );

      // If orignal source was a varchar, then must change the bulk src/tgt
      // attribute vc indicator length back to 0 since these are now fixed
      // char's.
      if (srcAttr->isVariableLength())
      {
        bulkMoveSrcAttr->setVCIndicatorLength(0);
        bulkMoveTgtAttr->setVCIndicatorLength(0);
      }

      // Both source and destinations are no longer rowsets
      bulkMoveTgtAttr->setRowsetSize(0);

      bulkMoveSrcAttr->setRowsetSize(0);

      outValIdList.insert(bulkMoveTgt->getValueId());

      if (bulkMoveSrcStartOffset) // param passed in.
	{
          *bulkMoveSrcStartOffset = (Int32)bulkMoveSrcAttr->getOffset();
          // If the field is nullable, then its null indicator offset
          // Else if the field is a varchar, then its var len indicator offset
          if (bulkMoveSrcAttr->getNullIndicatorLength() > 0)
            *bulkMoveSrcStartOffset = (Int32)bulkMoveSrcAttr->getNullIndOffset();
          else if (bulkMoveSrcAttr->isVariableLength())
            *bulkMoveSrcStartOffset = (Int32)bulkMoveSrcAttr->getVCLenIndOffset();
	}
    }

  return 0;
}


//////////////////////////////////////////////////////////////////////
// See GenExpGenerator.h for comments.
///////////////////////////////////////////////////////////////////////
short ExpGenerator::generateContiguousMoveExpr(
					       const ValueIdList & valIdList,
					       short addConvNodes,
					       Int32 atp,
					       Int32 atpIndex,
					       ExpTupleDesc::TupleDataFormat tdataF,
					       ULng32 &tupleLength,
					       ex_expr ** moveExpr,
					       ExpTupleDesc ** tupleDesc,
					       ExpTupleDesc::TupleDescFormat tdescF,
					       MapTable ** newMapTable,
					       ValueIdList *tgtValues,
					       ULng32 startOffset,
					       Lng32 * bulkMoveSrcStartOffset,
					       NABoolean disableConstFolding,
                                               NAColumnArray *colArray,
                                               NABoolean doBulkMoves)
{
  // ---------------------------------------------------------------------
  // Generate an expression to take scattered values (given in valIdList)
  // and copy them to a contiguous buffer tuple (atp,atpIndex). The values
  // are allocated in the tuple in the sequence given in valIdList.
  // On request, the method also returns a map table with new attributes
  // for the value ids in valIdList and a list of the target expressions
  // that were generated.
  // ---------------------------------------------------------------------
  // a list with convert nodes corresponding to valIdList
  ValueIdList convValIdList;

  // Allocate a new map table that describes the value ids in valIdList
  // and convValList. Append it to the end of the list of map tables.
  MapTable *contBufferMapTable = generator->appendAtEnd();

  // should we use a temporary map table for the convert nodes? $$$$

  // ---------------------------------------------------------------------
  // For each value in the list, put a convert node on its top which
  // will take care of the move. Assign attributes for all those convert
  // nodes such that they are stored adjacently in a single tuple
  // (the one described by (atp, atpIndex)).
  // ---------------------------------------------------------------------

  NABoolean alignedFormat = (tdataF == ExpTupleDesc::SQLMX_ALIGNED_FORMAT);
  Attributes ** attrs = new(wHeap()) Attributes * [valIdList.entries()];
  NAColumn *col;
  for (CollIndex i = 0; i < valIdList.entries(); i++)
    {
      ItemExpr * itemExpr = valIdList[i].getItemExpr();

      ItemExpr * convNode;
      if (addConvNodes)
	{
	  // add the convert node
	  convNode = new(wHeap()) Convert (itemExpr);
	  convNode->bindNode(generator->getBindWA());
	}
      else
	convNode = itemExpr;

      convNode->setConstFoldingDisabled(TRUE);
      if (NOT disableConstFolding)
	{
	  convNode->preCodeGen(generator);
	  foldConstants(convNode, NULL);
	}

      // bind/type propagate the new/old node
      convNode->bindNode(generator->getBindWA());
      ValueId convValueId = convNode->getValueId();

      // get a pointer to the map table attributes and set them the
      // way we want them to be
      if (addConvNodes)
	attrs[i] = (generator->addMapInfoToThis(contBufferMapTable, convValueId, 0))->getAttr();
      else
	attrs[i] = (generator->addMapInfo(convValueId, 0))->getAttr();

      if ( alignedFormat  &&
           (colArray == NULL) &&
           ((col = valIdList[i].getNAColumn( TRUE )) != NULL) &&
           col->isAddedColumn() )
        attrs[i]->setAddedCol();

      convValIdList.insert(convValueId);
    }

  // Ensure added columns are tagged before computing offset for the aligned
  // record format since added columns are arranged differently.
  //  if ( (colArray != NULL) && alignedFormat )
  if ( (colArray != NULL) )
  {
    for( Int32 i = 0; i < (Int32)colArray->entries(); i++ )
    {
      col = (*colArray)[i];
      addDefaultValue(col, attrs[i], CmpCommon::diags()); 
    }
  }

  // If generating a contiguous move expression where the target tuple data
  // format is a disk format - Packed or Aligned - then header information
  // must be gathered during processing the attributes. This allows a new 
  // header clause to be generated during endExprGen()
  ExpHdrInfo *hdrInfo = NULL;
  if ( isHeaderNeeded(tdataF) )
    hdrInfo = new( wHeap() )ExpHdrInfo();

  // compute offsets and create tuple descriptor.
  processAttributes((ULng32)valIdList.entries(), attrs, tdataF,
		    tupleLength, atp, atpIndex,  tupleDesc, tdescF,
                    startOffset, hdrInfo);

  // deallocate the attributes array. Don't deallocate the attributes
  // pointed to by the array entries since we didn't allocate them.
  NADELETEBASIC(attrs,wHeap());

  // if bulkmovesrcstartoffset passed in is -2, then always generate
  // move expression even if bulk move using bulkmovesrcstartoffset
  // could be done.
  NABoolean alwaysGenMoveExpr = FALSE;
  if ((moveExpr) &&
      (bulkMoveSrcStartOffset) &&
      (*bulkMoveSrcStartOffset == -2))
    alwaysGenMoveExpr = TRUE;

  ValueIdList bulkMoveValueIdList;

  // Since convert nodes are asked for, check if a bulk move of all source
  // values can be done to the target.
  // if asked not to do bulk move, don't do it.
  if (addConvNodes && doBulkMoves)
  {
    if (generateBulkMove(convValIdList, bulkMoveValueIdList,
                         tupleLength,
                         bulkMoveSrcStartOffset) == -1)
      return -1;

    // If bulk move will be used, then no header clause needs
    // to be generated.
    if (hdrInfo  &&  bulkMoveValueIdList.entries() > 0)
    {
      NADELETEBASIC( hdrInfo, wHeap() );
      hdrInfo = NULL;
    }
  }

  ValueIdList * newValIdList;

  if (bulkMoveValueIdList.entries() > 0)
    newValIdList = &bulkMoveValueIdList;
  else
    newValIdList = &convValIdList;

  // generate the copy expression, if moveExpr is passed in,
  // and bulk move info is not to be returned
  // or bulk move info is to be returned but bulk move is not being done.
  if ((moveExpr) &&
      ((! bulkMoveSrcStartOffset) ||
       (*bulkMoveSrcStartOffset == -1) ||
       (alwaysGenMoveExpr)))
  {
    generateListExpr(*newValIdList, ex_expr::exp_ARITH_EXPR, moveExpr,
                     atp, atpIndex, hdrInfo);

    if ( newValIdList != &convValIdList )
    {
      for (int j =0; (UInt32)j< convValIdList.entries(); j++)
      {
        generator->getMapInfo(convValIdList[j])->codeGenerated();
      }
    }

    if ( hdrInfo != NULL )
    {
      NADELETEBASIC( hdrInfo, wHeap() );
    }
  }

  // if the user requested a map table back, make a new map table and
  // add all the original value ids to it, but assign the attributes
  // of the new, contiguous buffers
  if (newMapTable)
    {
      generator->appendAtEnd();
      *newMapTable = generator->unlinkLast();
      for (CollIndex i = 0; i < valIdList.entries(); i++)
	{
	  // don't add constants. Constants have a fixed location
	  // (atp_index = 0) and are added whenever they are used in an
	  // expression.
	  if (convValIdList[i].getItemExpr()->getOperatorType() !=
	      ITM_CONSTANT)
	    {
	      // Get the attributes of the contiguous buffer...
	      Attributes *contAttrib =
		generator->getMapInfo((convValIdList)[i])->getAttr();

	      // ...add it to the new map table as if it belonged to
	      // the original value id...
              MapInfo * mapInfo =
                generator->addMapInfoToThis((*newMapTable), valIdList[i],contAttrib);

	      // ... and make sure no more code gets generated for it.
	      mapInfo->codeGenerated();
	    }
	}
    }

  // if the caller wants to see the generated expressions for the move
  // targets, insert them into the list supplied by the caller
  if (tgtValues != NULL)
    tgtValues->insert(convValIdList);

  return 0;
}

short
ExpGenerator::genGuardedContigMoveExpr(const ValueIdSet guard,
                                       const ValueIdList & valIdList,
                                       short addConvNodes,
                                       Int32 atp,
                                       Int32 atpIndex,
                                       ExpTupleDesc::TupleDataFormat tdataF,
                                       ULng32 &tupleLength,
                                       ex_expr ** moveExpr,
                                       ExpTupleDesc ** tupleDesc,
                                       ExpTupleDesc::TupleDescFormat tdescF,
                                       MapTable ** newMapTable,
                                       ValueIdList *tgtValues,
                                       ULng32 start_offset)
{
  // ---------------------------------------------------------------------
  // Generate an expression to take scattered values (given in valIdList)
  // and copy them to a contiguous buffer tuple (atp,atpIndex). The values
  // are allocated in the tuple in the sequence given in valIdList.
  // On request, the method also returns a map table with new attributes
  // for the value ids in valIdList and a list of the target expressions
  // that were generated.
  // ---------------------------------------------------------------------
  // a list with convert nodes corresponding to valIdList
  ValueIdList convValIdList;

  // Allocate a new map table that describes the value ids in valIdList
  // and convValList. Append it to the end of the list of map tables.
  MapTable *contBufferMapTable = generator->appendAtEnd();

  // should we use a temporary map table for the convert nodes? $$$$

  // ---------------------------------------------------------------------
  // For each value in the list, put a convert node on its top which
  // will take care of the move. Assign attributes for all those convert
  // nodes such that they are stored adjacently in a single tuple
  // (the one described by (atp, atpIndex)).
  // ---------------------------------------------------------------------

  Attributes ** attrs = new(wHeap()) Attributes * [valIdList.entries()];
  for (CollIndex i = 0; i < valIdList.entries(); i++)
    {
      ItemExpr * itemExpr = valIdList[i].getItemExpr();

      ItemExpr * convNode;
      if (addConvNodes)
	{
	  // add the convert node
	  convNode = new(wHeap()) Convert (itemExpr);
	}
      else
	convNode = itemExpr;

      // bind/type propagate the new/old node
      convNode->bindNode(generator->getBindWA());
      ValueId convValueId = convNode->getValueId();

      // get a pointer to the map table attributes and set them the
      // way we want them to be
      if (addConvNodes)
	attrs[i] = (generator->addMapInfoToThis(contBufferMapTable, convValueId, 0))->getAttr();
      else
	attrs[i] = (generator->addMapInfo(convValueId, 0))->getAttr();

      convValIdList.insert(convValueId);
    }

  // compute offsets and create tuple descriptor.
  processAttributes((ULng32)valIdList.entries(), attrs, tdataF,
		    tupleLength, atp, atpIndex,  tupleDesc, tdescF,
		    start_offset);

  // deallocate the attributes array. Don't deallocate the attributes
  // pointed to by the array entries since we didn't allocate them.
  NADELETEBASIC(attrs,wHeap());

  // generate the copy expression
  genGuardedListExpr(guard, convValIdList,ex_expr::exp_ARITH_EXPR,moveExpr);

  // if the user requested a map table back, make a new map table and
  // add all the original value ids to it, but assign the attributes
  // of the new, contiguous buffers
  if (newMapTable)
    {
      generator->appendAtEnd();
      *newMapTable = generator->unlinkLast();
      for (CollIndex i = 0; i < valIdList.entries(); i++)
	{
	  // don't add constants. Constants have a fixed location
	  // (atp_index = 0) and are added whenever they are used in an
	  // expression.
	  if (convValIdList[i].getItemExpr()->getOperatorType() !=
	      ITM_CONSTANT)
	    {
	      // Get the attributes of the contiguous buffer...
	      Attributes *contAttrib =
		generator->getMapInfo((convValIdList)[i])->getAttr();

	      // ...add it to the new map table as if it belonged to
	      // the original value id...
              MapInfo * mapInfo =
                generator->addMapInfoToThis((*newMapTable), valIdList[i],contAttrib);

	      // ... and make sure no more code gets generated for it.
	      mapInfo->codeGenerated();
	    }
	}
    }

  // if the caller wants to see the generated expressions for the move
  // targets, insert them into the list supplied by the caller
  if (tgtValues != NULL)
    tgtValues->insert(convValIdList);

  return 0;
}

//////////////////////////////////////////////////////////////////////
// Input is a ValueIdList to be exploded into a contiguous buffer.
// Every other parameter is similar to generateContiguousMoveExpr.
///////////////////////////////////////////////////////////////////////
short ExpGenerator::generateExplodeExpr(
     const ValueIdList & val_id_list,  // IN
     Int32 atp,                          // IN
     Int32 atpIndex,                     // IN
     ExpTupleDesc::TupleDataFormat tf, // IN
     ULng32 &tupleLength,       // OUT
     ex_expr ** explodeExpr,              // OUT
     ExpTupleDesc ** tupleDesc, // OUT(O)
     ExpTupleDesc::TupleDescFormat tdescF, // IN
     MapTable ** newMapTable,   // OUT(O)
     ValueIdList *tgtValues,    // OUT(O)
     ULng32 start_offset)  // IN(O)
{
  ValueIdList explodeVidList;
  for (CollIndex i = 0; i < val_id_list.entries(); i++)
    {
      ItemExpr * ie = val_id_list[i].getItemExpr();

      ItemExpr * cast_node;
      if (ie->getValueId().getType().getVarLenHdrSize() > 0)
	{
	  // Explode varchars.
	  cast_node = new(generator->wHeap())
	    ExplodeVarchar (ie, &(ie->getValueId().getType()), FALSE);
	}
      else
	{
	  cast_node = new(generator->wHeap())
	    Cast(ie, &(ie->getValueId().getType()));
      }

      // Bind the cast node and insert the value ID into the list.
      //
      cast_node->bindNode(generator->getBindWA());
      explodeVidList.insert(cast_node->getValueId());
    }

  return generateContiguousMoveExpr(explodeVidList, 0/*don't add conv nodes*/,
				    atp, atpIndex, tf, tupleLength,
				    explodeExpr, tupleDesc, tdescF,
				    newMapTable, tgtValues,
				    start_offset);
}

//
// Expression generated for inserts and non-optimized updates to clear
// any header data areas in the buffer being written to.  This expression
// is only generated when the target is one of the 2 SQL/MX disk formats
//    SQLMX_FORMAT or SQLMX_ALIGNED_FORMAT
// For SQLMX_FORMAT the first fixed field area will be cleared,
// for the aligned format, everything up to the first fixed field will
// be cleared (header + entire VOA + bitmap + any padding).
// For the aligned format, the bitmap offset will be set too.
short ExpGenerator::generateHeaderClause( Int32       atp,
                                          Int32       atpIndex,
                                          ExpHdrInfo *hdrInfo)
{
  ExHeaderClause *hdrClause = NULL;
  Int16 numOperands = 1;        // target row that must have its header cleared
  Attributes **attr;

  GenAssert( hdrInfo != NULL, "Null expression" );

  ExpTupleDesc::TupleDataFormat tdf = (hdrInfo->getBitmapEntryOffset() > 0
                                       ? ExpTupleDesc::SQLMX_ALIGNED_FORMAT
                                       : ExpTupleDesc::SQLMX_FORMAT);

  // This is a showplan statement.
  if (getShowplan())
    numOperands = (Int16)(numOperands * 2);

  attr = new(generator->wHeap()) Attributes * [numOperands];

  // Initialize an attribute for the full header.
  attr[0] = (Attributes *)new(generator->wHeap())
                          SimpleType(hdrInfo->getAdminSize(), 1, 0);
  attr[0]->setDatatype(REC_BYTE_F_ASCII);
  attr[0]->setTupleFormat(tdf);
  attr[0]->setOffset(hdrInfo->getStartOffset()); 
  attr[0]->setNullIndicatorLength(0);
  attr[0]->setNullFlag(0);
  attr[0]->setVCIndicatorLength(0);
  attr[0]->setAtpIndex((Int16)atpIndex);
  attr[0]->setAtp((Int16)atp);

  if (getShowplan())
  {
    attr[0]->setShowplan();
    NAString hdrStr = "Hdr";
    ValueId dummyVID(NULL_VALUE_ID);

    attr[1] = new(generator->wHeap())
              ShowplanAttributes(dummyVID,
                                 convertNAString(hdrStr, generator->wHeap()));
  }

  hdrClause = new(getSpace())ExHeaderClause(attr, getSpace(),
                                            hdrInfo->getAdminSize(),
                                            hdrInfo->getBitmapEntryOffset(),
                                            hdrInfo->getBitmapOffset(),
                                            hdrInfo->getFirstFixedOffset());

  // don't end the expression generation since we didn't start it
  // endExprGen (expr, -1);

  // Add clear header clause to the clause_list. This is attached to the
  // expression by the method endExprGen()
  linkClause(0, hdrClause);

  return 0;
}

// input is a ValueId which points to a boolean tree.
// Generate code for the ItemExprTree corresponding
// to this ValueId.
short ExpGenerator::generateExpr(const ValueId & val_id,
				 ex_expr::exp_node_type node_type,
				 ex_expr ** expr)
{
  initExprGen();
  startExprGen(expr, node_type);

  // generate code for this node and the tree under it
  ItemExpr * temp =
    val_id.getValueDesc()->getItemExpr()->preCodeGen(generator);
  if (! temp)
    return -1;
  temp->codeGen(generator);

  endExprGen(expr, 0);

  return 0;
}

///////////////////////////////////////////////////////////////////////////
// This method is used to figure out if keys are to be extracted from base
// table row and encoded at runtime. If encoding is not needed based on
// certain criteria (detailed below), then offset of the first key column
// inside the base table row is returned.
// At runtime, this offset is used to directly access the key values
// from the base row instead of going thru expression evaluator to
// encode each key. This check is done for performance improvement.
//
// Encoding is needed if:
//   -- the datatype of key column needs encoding (for ex, if it is a
//      signed datatype)
//   -- it is a nullable column. This restriction could be removed in future.
//   -- it is a descending key
//   -- the key columns are not 'next' to each other from left to right
//      in the base row
//   -- the difference in offsets of any two consecutive key columns is
//      not the same as the size of first column. This case can happen
//      if columns are aligned inside base row by introducing filler bytes.
//   -- if any column preceding the first key column in the base table
//      is a varchar
//
// RETURN: TRUE,  if key encoding is needed.
//         FALSE, if key encoding is not needed, in this case the
//                firstKeyColumnOffset contains the offset of the first
//                key column inside the base table row.
/////////////////////////////////////////////////////////////////////////////
NABoolean ExpGenerator::processKeyEncodingOptimization(
     const NAColumnArray &allColumns,             /* IN */
     const NAColumnArray &indexKeyColumns,        /* IN */
     const ValueIdList   &indexKey,               /* IN */
     const short keyTag,                          /* IN */
     ULng32 &keyLen,
     ULng32 &firstKeyColumnOffset)
{
  CollIndex prevColNumber;

  // if this is a primary index (base table), then get the position
  // of the first key in the base table.
  // If this is an index, then the index key columns are the stored
  // in the beginning of the index row. Do not use the getPosition()
  // method as that will give the position of the index column in
  // the base table row. The position of the first
  // index key column in the index is 0.
  // NOTE: if this 'assumtion' about index columns changes, then
  // need to change this logic.
  if (keyTag == 0) // primary index
    prevColNumber =
      (CollIndex)indexKeyColumns[0]->getPosition();
  else
    {
      // index
      prevColNumber = (CollIndex)0;
    }

  ValueId prevValId = indexKey[0];

  // make sure that none of the columns preceding the first key column
  // in the base table row is a varchar
  CollIndex i = 0;
  while ( i < prevColNumber )
    {
      if (allColumns.getColumn(i)->getType()->getVarLenHdrSize() > 0)
	return TRUE;
      else
	i++;
    }

  keyLen = 0;
  const CollIndex indexKeyEntries = indexKey.entries();
  Attributes * prevAttr = generator->getMapInfo(prevValId)->getAttr();
  for (i = 0; i < indexKeyEntries; i++)
    {
      ValueId valId = indexKey[i];

      NAColumn * naCol = indexKeyColumns.getColumn(i);

      Attributes * thisAttr = generator->getMapInfo(valId)->getAttr();

      if ((valId.getType().isEncodingNeeded() == TRUE)  ||
	  ((valId.getType().getTypeQualifier() == NA_CHARACTER_TYPE) &&  
	   ((((CharType&)valId.getType()).isCaseinsensitive())  ||
           (CollationInfo::isSystemCollation(((CharType&)valId.getType()).getCollation())))) ||
	  (valId.getType().supportsSQLnull()  == TRUE)  ||
	  (indexKeyColumns.isAscending(i) == FALSE)     ||
	  ((naCol->getPosition() - prevColNumber) < 0)   ||
	  ((naCol->getPosition() - prevColNumber) > 1)   ||
	  ((thisAttr->getOffset() - prevAttr->getOffset())
	   > prevAttr->getStorageLength())               ||
          (!naCol->isStoredOnDisk()))
	return TRUE;

      prevColNumber = naCol->getPosition();
      prevValId = valId;
      prevAttr = thisAttr;
      keyLen += prevAttr->getStorageLength();
    }

  firstKeyColumnOffset = generator->getMapInfo(indexKey[0])->getAttr()->getOffset();

  return FALSE;
}

NABoolean ExpGenerator::isKeyEncodingNeeded(const IndexDesc * indexDesc,
					    ULng32 &keyLen,
					    ULng32 &firstKeyColumnOffset)
{
  return processKeyEncodingOptimization(
       indexDesc->getNAFileSet()->getAllColumns(),
       indexDesc->getNAFileSet()->getIndexKeyColumns(),
       indexDesc->getIndexKey(),
       indexDesc->getNAFileSet()->getKeytag(),
       keyLen,
       firstKeyColumnOffset);
}

// input is the index descriptor. Generate an expression to
// extract and encode the key of the table and create a
// contiguous row of encoded keys.
// If optimizeKeyEncoding is TRUE, then check to see if encoding could
// be avoided. If so, return the offset to the first key column in
// the data row.
short ExpGenerator::generateKeyEncodeExpr(const IndexDesc * indexDesc,
					  Int32 atp, Int32 atp_index,
					  ExpTupleDesc::TupleDataFormat tf,
					  ULng32 &keyLen,
					  ex_expr ** encode_expr,
					  NABoolean optimizeKeyEncoding,
					  ULng32 &firstKeyColumnOffset,
					  const ValueIdList * inKeyList,
					  NABoolean handleSerialization)
{
  if ((optimizeKeyEncoding == TRUE) &&
      (isKeyEncodingNeeded(indexDesc, keyLen, firstKeyColumnOffset) == FALSE))
    return 0;

  MapTable * mapTable = generator->getMapTable();

  ValueIdList encode_val_id_list;
  CollIndex i = 0;

  Lng32 numEntries = indexDesc->getIndexKey().entries();
  if (inKeyList)
    numEntries = MINOF(inKeyList->entries(),  indexDesc->getIndexKey().entries());

  for (i = 0; i < numEntries; i++)
    {
      // Allocate convert node
      // to move the key value to the key buffer.
      ItemExpr * col_node = 
	(inKeyList ? (*inKeyList)[i].getItemExpr() : 
	 (indexDesc->getIndexKey())[i].getItemExpr());

      short desc_flag = TRUE;
      if ((indexDesc->getNAFileSet()->getIndexKeyColumns()).isAscending(i))
	desc_flag = FALSE;
      
      if ((tf == ExpTupleDesc::SQLMX_KEY_FORMAT) &&
	  (col_node->getValueId().getType().getVarLenHdrSize() > 0) &&
          (NOT ((indexDesc->getNAFileSet()->getIndexKeyColumns()[i])->isPrimaryKeyNotSerialized())))
	{
	  // Explode varchars by moving them to a fixed field
	  // whose length is equal to the max length of varchar.
	  
	  // handle different character set cases.
          const CharType& char_t =
	    (CharType&)(col_node->getValueId().getType());
	  if (!CollationInfo::isSystemCollation(char_t.getCollation()))
	    {
	      col_node = new(wHeap())
		Cast (col_node,
		      (new(wHeap())
		       SQLChar(wHeap(), CharLenInfo(char_t.getStrCharLimit(), char_t.getDataStorageSize()),
			       col_node->getValueId().getType().supportsSQLnull(),
			       FALSE, FALSE, FALSE,
			       char_t.getCharSet(),
			       char_t.getCollation(),
			       char_t.getCoercibility()
			       )
		       )
		      );
	    }
	}

      ItemExpr * enode = NULL;
      if (handleSerialization)
	{
	  NAColumn * nac = indexDesc->getNAFileSet()->getIndexKeyColumns()[i];
          NABoolean isAlignedRowFormat = indexDesc->getNAFileSet()->isSqlmxAlignedRowFormat();
	  if ((!isAlignedRowFormat) && CmpSeabaseDDL::isEncodingNeededForSerialization(nac))
	    {
	      if (desc_flag)
		{
		  // this value is in serializable encoded form. Decode it before generating the
		  // key expr. Do this only if the new key order is different than the original
		  // order. Original encoding is in ascending order.
		  col_node = new(generator->wHeap()) CompDecode
		    (col_node, &col_node->getValueId().getType(),
		     FALSE, TRUE);
		}
	      else
		// col_node is already in encoded format.
		enode = 
		  new(generator->wHeap()) Cast(col_node, &col_node->getValueId().getType());
	    } // encoding/decoding needed
	} // handleSerialization

      if (enode == NULL)
        {
          if (NOT ((indexDesc->getNAFileSet()->getIndexKeyColumns()[i])->isPrimaryKeyNotSerialized()))
            enode = new(wHeap()) CompEncode(col_node, desc_flag);
          else 
            enode = 
              new(generator->wHeap()) Cast(col_node, &col_node->getValueId().getType());
        }            
      
      enode->bindNode(generator->getBindWA());

      encode_val_id_list.insert(enode->getValueId());
    }

  generateContiguousMoveExpr(encode_val_id_list,
			     0, // don't add convert nodes,
			     atp,
			     atp_index,
			     tf,
			     keyLen,
			     encode_expr);

  return 0;
}

short ExpGenerator::generateDeserializedMoveExpr(
						 const ValueIdList & valIdList,
						 Int32 atp,
						 Int32 atpIndex,
						 ExpTupleDesc::TupleDataFormat tdataF,
						 ULng32 &tupleLength,
						 ex_expr ** moveExpr,
						 ExpTupleDesc ** tupleDesc,
						 ExpTupleDesc::TupleDescFormat tdescF,
						 ValueIdList &deserVIDlist,
						 ValueIdSet &alreadyDeserialized)
{
  for (Lng32 i = 0; i < valIdList.entries(); i++)
    {
      ValueId vid = valIdList[i];
      NAColumn * nac = vid.getNAColumn( TRUE );

      ItemExpr * ie = NULL;
      if (nac && CmpSeabaseDDL::isEncodingNeededForSerialization(nac) &&
	  !alreadyDeserialized.contains(vid))
	{
	  ie = new(generator->wHeap()) CompDecode(vid.getItemExpr(), &vid.getType(),
						  FALSE, TRUE);
	}
      else
	{
	  ie = new(generator->wHeap()) Cast(vid.getItemExpr(), &vid.getType());
	}

      ie->bindNode(generator->getBindWA());
      deserVIDlist.insert(ie->getValueId());
    }

  return generateContiguousMoveExpr(
				    deserVIDlist,
				    0, // no conv nodes
				    atp, atpIndex,
				    tdataF,
				    tupleLength,
				    moveExpr,
				    tupleDesc,
				    tdescF);
}

short ExpGenerator::generateExtractKeyColsExpr(
					       const ValueIdList &colVidList,//const IndexDesc * indexDesc,
					       Int32 atp, Int32 atp_index,
					       ULng32 &keyLen,
					       ex_expr ** key_cols_expr)
{
  ValueIdList val_id_list;
  CollIndex i = 0;
  for (i = 0; i < colVidList.entries(); i++)
    //  for (i = 0; i < indexDesc->getIndexColumns().entries(); i++)
    {
      ItemExpr * col_node = colVidList[i].getItemExpr();

      val_id_list.insert(col_node->getValueId());
    }

  generateContiguousMoveExpr(val_id_list,
			     1, // add convert nodes,
			     atp,
			     atp_index,
			     ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			     keyLen,
			     key_cols_expr);

  return 0;
}

short ExpGenerator::generateKeyColValueExpr(
					    const ValueId vid,
					    Int32 atp, Int32 atp_index,
					    ULng32 &len,
					    ex_expr ** colValExpr)
{

  ItemExpr * eq_node = (vid.getValueDesc())->getItemExpr();

  ItemExpr * keycol = ((*eq_node)[0])->castToItemExpr();
  ItemExpr * keyval = ((*eq_node)[1])->castToItemExpr();

  keyval = keyval->preCodeGen(generator);

  const NAType * sourceType = &(keyval->getValueId().getType());
  const NAType * targetType = &(keycol->getValueId().getType());

  NABoolean generateNarrow = sourceType->errorsCanOccur(*targetType);

  // if narrow is to be generated or varchar key col, then checkAndDelete opt
  // cannot be done (for now).
  if (NOT ((generateNarrow) ||(targetType->getVarLenHdrSize() > 0)))
    {
      ItemExpr * cv = NULL;
      cv = new(wHeap()) Cast(keyval, targetType);

      if (HbaseAccess::isEncodingNeededForSerialization(keycol))
	{
	  cv = new(generator->wHeap()) CompEncode
	    (cv, FALSE, -1, CollationInfo::Sort, TRUE);
	}

      cv = cv->bindNode(generator->getBindWA());
      cv = cv->preCodeGen(generator);

      ValueIdList val_id_list;
      val_id_list.insert(cv->getValueId());
      generateContiguousMoveExpr(val_id_list,
				 0, // no conv nodes
				 atp,
				 atp_index,
				 ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
				 len,
				 colValExpr);
    }

  return 0;
}


ItemExpr * ExpGenerator::generateKeyCast(const ValueId vid,
					 ItemExpr * dataConversionErrorFlag,
					 NABoolean desc_flag,
					 ExpTupleDesc::TupleDataFormat tf,
					 Lng32 &possibleErrorCount,
					 NABoolean allChosenPredsAreEqualPreds,
                                         NABoolean castVarcharToAnsiChar)
{

  ItemExpr * eq_node = (vid.getValueDesc())->getItemExpr();

  ItemExpr * keycol = ((*eq_node)[0])->castToItemExpr();
  ItemExpr * keyval = ((*eq_node)[1])->castToItemExpr();

  // do preCodeGen now, to avoid inserting scaling later
  keyval = keyval->preCodeGen(generator);

  const NAType * sourceType = &(keyval->getValueId().getType());
  const NAType * targetType = &(keycol->getValueId().getType());

  NABoolean caseinsensitiveEncode = FALSE;

  ItemExpr * knode = 0;  // the node to be returned

  NABoolean generateNarrow = sourceType->errorsCanOccur(*targetType);

  switch (tf)
    {
    case ExpTupleDesc::SQLMX_KEY_FORMAT:
      {
	ItemExpr * cnode;

	if (targetType->getTypeQualifier() == NA_CHARACTER_TYPE)
	  {
	    const CharType& char_t =
	      (CharType&)(keycol->getValueId().getType());

	    // Explode varchars by moving them to a fixed field
	    // whose length is equal to the max length of varchar.
	    // Remove the UPSHIFT attr from the target. We don't want
	    // to upshift data for key lookup.

	    if ((keycol->getValueId().getType().getVarLenHdrSize() > 0) ||
                (char_t.isUpshifted()))
	      {
		if (!CollationInfo::isSystemCollation(char_t.getCollation()))
		{
                  if ((castVarcharToAnsiChar) &&
                      (keycol->getValueId().getType().getVarLenHdrSize() > 0))
                    {
                      targetType = new(wHeap())
                        ANSIChar(wHeap(), char_t.getDataStorageSize(),
                                 keycol->getValueId().getType().supportsSQLnull(),
                                 FALSE,
                                 FALSE,
                                 char_t.getCharSet(),
                                 char_t.getCollation(),
                                 char_t.getCoercibility()
                                 );
                    }
                  else if (DFS2REC::isCharacterString(keycol->getValueId().getType().getFSDatatype()))
                    {
                      targetType = new(wHeap())
                        SQLChar(wHeap(), CharLenInfo(char_t.getStrCharLimit(), char_t.getDataStorageSize()),
                                keycol->getValueId().getType().supportsSQLnull(),
                                FALSE,
                                ((CharType*)targetType)->isCaseinsensitive(),
                                FALSE,
                                char_t.getCharSet(),
                                char_t.getCollation(),
                                char_t.getCoercibility()
                                );
                    }
                  else if (DFS2REC::isBinaryString(keycol->getValueId().getType().getFSDatatype()))                  
                    {
                      targetType = new(wHeap())
                        SQLBinaryString(wHeap(),
                                        keycol->getValueId().getType().getNominalSize(),
                                        keycol->getValueId().getType().supportsSQLnull(),
                                        FALSE
                                        );                
                    }
		}
	      }
	  }

	NABoolean compEncodeGenerated = FALSE;
	if (generateNarrow)
	  {
	    cnode = new(wHeap()) Narrow (keyval,dataConversionErrorFlag,targetType,
                                         ITM_NARROW, desc_flag);
	  }
	else
	  {
	    if (*sourceType == *targetType)
	      {
		// if source & target are exactly the same and normal scaling
		// is being done, then generate the encode node. This avoids
		// an extra conversion node.
		cnode = new(wHeap()) CompEncode(keyval, desc_flag);
		compEncodeGenerated = TRUE;
	      }
	    else
	      {
		cnode = new(wHeap()) Cast (keyval,targetType);
	      }
	  }

        cnode = cnode->bindNode(generator->getBindWA());
        cnode = cnode->preCodeGen(generator);

	// if both source and target are caseinsensitive, then
	// do caseinsensitive encode.
	if ((targetType->getTypeQualifier() == NA_CHARACTER_TYPE) &&
	    (keyval->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE) &&
	    (((CharType*)targetType)->isCaseinsensitive()) &&
	    (((CharType&)keyval->getValueId().getType()).isCaseinsensitive()))
	  {
	    caseinsensitiveEncode = TRUE;
	  }

	// encode the key for byte comparison.
        knode = cnode;
	if (compEncodeGenerated)
	  {
	    knode = cnode;
	    ((CompEncode*)knode)->setCaseinsensitiveEncode(caseinsensitiveEncode);
	  }
	else if (NOT castVarcharToAnsiChar)
	  {
	    knode = new(wHeap()) CompEncode(cnode, desc_flag);
            knode = knode->bindNode(generator->getBindWA());
	    ((CompEncode*)knode)->setCaseinsensitiveEncode(caseinsensitiveEncode);
	    knode = knode->preCodeGen(generator);
	  }
      }
      break;

    default:
      {
	GenAssert(0, "Unrecognized tuple format");
      }
      break;
    }

  // if data conversion errors could have occurred on a prior column,
  // generate a CASE statement to move in the min or max value for the
  // datatype instead (depending on the conversion error and on whether
  // this column is ascending or descending)

  if ((possibleErrorCount > 0) &&
      (NOT allChosenPredsAreEqualPreds))
    {
      ItemExpr * relativeMin = new(wHeap()) ConstValue(targetType->newCopy(wHeap()),
						       !desc_flag,
						       targetType->supportsSQLnull());
      relativeMin = new(wHeap()) Cast(relativeMin,targetType);

      // Add an encode node here.
      relativeMin = new(wHeap()) CompEncode(relativeMin, desc_flag);

      ItemExpr * relativeMax = new(wHeap()) ConstValue(targetType->newCopy(wHeap()),
						       desc_flag,
						       targetType->supportsSQLnull());
      relativeMax = new(wHeap()) Cast(relativeMax,targetType);

      // Add an encode node here.
      relativeMax = new(wHeap()) CompEncode(relativeMax, desc_flag);

      knode = createExprTree("CASE WHEN @A1 = 0 THEN @A2 WHEN @A1 < 0 THEN @A3 ELSE @A4 END",
                             0,
			     4,
			     dataConversionErrorFlag, // @A1
			     knode, // @A2
			     relativeMax, // @A3
			     relativeMin); // @A4
    }

  GenAssert(knode, "generateKeyCast: knode null pointer ");
  knode = knode->bindNode(generator->getBindWA());
  knode = knode->preCodeGen(generator);

  // knode->displayTree();

  // Check for an error binding the node.
  GenAssert(!generator->getBindWA()->errStatus(),"MDAM: Error binding node.");

  // update count of columns that can have conversion errors
  if (generateNarrow)
    possibleErrorCount++;

  return knode;
}


short ExpGenerator::generateKeyExpr(const NAColumnArray & indexKeyColumns,
				    const ValueIdList & val_id_list,
				    Int32 atp, Int32 atp_index,
				    ItemExpr * dataConversionErrorFlag,
				    ExpTupleDesc::TupleDataFormat tf,
				    ULng32 &keyLen,
				    ex_expr ** key_expr,
				    NABoolean allChosenPredsAreEqualPreds)
{
  // generate key expression.
  // Key value id list has entries of the form:
  //    col1 = value1, col2 = value2, ... colN = valueN
  // All key values are present (missing keys are added in preCodeGen).
  if (val_id_list.entries() > 0)
    {
      ValueIdList key_val_id_list;

      // generate expression to create the key buffer. All key values
      // are part of a contiguous key buffer whose length is same
      // as the key length of the table. Allocate convert nodes
      // to move the key values to the key buffer.

      Lng32 possibleErrorCount = 0;

      short prevAtp = -1, prevAtpIndex = -1;
      UInt32 prevOffset = 0;
      UInt32 firstOffset = 0;
      UInt32 prevStorageLength = 0;
      for (ULng32 i = 0; i < val_id_list.entries(); i++)
	{
	  ItemExpr * knode = NULL;

	  if ((indexKeyColumns[i]->getNATable()->isHbaseCellTable()) ||
	      (indexKeyColumns[i]->getNATable()->isHbaseRowTable()) ||
              (indexKeyColumns[i]->isPrimaryKeyNotSerialized()) ||
              ((indexKeyColumns[i]->getNATable()->isHbaseMapTable()) &&
               (indexKeyColumns[i]->getNATable()->getClusteringIndex()->hasSingleColVarcharKey())))
	    {
	      ItemExpr * eq_node = (val_id_list[i].getValueDesc())->getItemExpr();
	      
	      ItemExpr * keycol = ((*eq_node)[0])->castToItemExpr();
	      ItemExpr * keyval = ((*eq_node)[1])->castToItemExpr();
	      
	      // do preCodeGen now, to avoid inserting scaling later
	      keyval = keyval->preCodeGen(generator);
	      
	      const NAType * sourceType = &(keyval->getValueId().getType());
	      const NAType * targetType = &(keycol->getValueId().getType());
	      
	      knode = new(wHeap()) Cast (keyval,targetType);
	      knode = knode->bindNode(generator->getBindWA());
	      knode = knode->preCodeGen(generator);
	    }
	  else
	    {
	      knode =
		generateKeyCast(val_id_list[i],
				dataConversionErrorFlag,
				!indexKeyColumns.isAscending(i),
				tf,
				possibleErrorCount /* in/out */,
				allChosenPredsAreEqualPreds,
                                FALSE);
	    }

	  key_val_id_list.insert(knode->getValueId());

	}

      generateContiguousMoveExpr(key_val_id_list,
                                 0, // don't add convert nodes,
                                 atp, atp_index, tf, keyLen,
                                 key_expr);
    }

  return 0;
}


short ExpGenerator::generateExclusionExpr(ItemExpr *expr,
					  Int32 atp,
					  Int32 atpindex,
					  ex_expr ** excl_expr)
{
  // if the begin/end key exclusion flag is dynamically calculated, generate
  // a move expression to get the value into a predetermined place
  if (expr)
    {
      // generate move expression
      ValueIdList exclusionResult;
      ULng32 resultLen;

      exclusionResult.insert(expr->getValueId());
      generateContiguousMoveExpr(
	   exclusionResult,
	   -1, // add convert node
	   atp,
	   atpindex,
	   ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
	   resultLen,
	   excl_expr);

      // executor has hard-coded assumption that the result is a long
      GenAssert(resultLen == sizeof(Lng32),
		"Exclusion flag expression result must have length 4");
    }
  return 0;
}

// generate code to do Describe Input and move input values
// from user area to executor area.
short ExpGenerator::generateInputExpr(const ValueIdList &val_id_list,
				      ex_expr::exp_node_type /* node_type */,
				      ex_expr ** expr)
{
  Space * space;
  Lng32 num_input_entries = 0;
  InputOutputExpr * ioExpr;
  Attributes ** attr;
  short retCode;

  initExprGen();

  space = getSpace();
  num_input_entries = 0;
  ioExpr = new(getSpace()) InputOutputExpr();
  *expr = ioExpr;

  (*expr)->setLength(getSpace()->getAllocatedSpaceSize());
  startExprGen(expr, ioExpr->getType());

  ValueId val_id;
  ex_inout_clause * input_clause = 0;

  const NABoolean isJdbc =
    (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON ? TRUE : FALSE);
  const NABoolean isOdbc =
    (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON ? TRUE : FALSE);

  for (CollIndex i = 0; i < val_id_list.entries(); i++)
    {
      val_id = val_id_list[i];

      ValueDesc * val_desc = val_id.getValueDesc();
      ItemExpr * item_expr = val_desc->getItemExpr();
      if ((item_expr->getOperatorType() == ITM_HOSTVAR) ||
	  (item_expr->getOperatorType() == ITM_DYN_PARAM))
	  {

	  item_expr->preCodeGen(generator);
	  item_expr->codeGen(generator);

	  const NAString *hostvarOrParamName = NULL;

	  if (item_expr->getOperatorType() == ITM_HOSTVAR)
	    {
	      hostvarOrParamName = &((HostVar *)item_expr)->getName();
	    }
	  else
	    {
	      hostvarOrParamName = &((DynamicParam *)item_expr)->getName();
	    }

	  Lng32 numAttrs = 1;
	  if (getShowplan())
	    numAttrs *= 2;

	  attr    = new(wHeap()) Attributes * [numAttrs];
	  //	  attr[0] = generator->getMapInfo(val_id)->getAttr();
	  attr[0] = generator->getAttr(item_expr);

	  if (getShowplan())
	    attr[1] =
	      new(wHeap()) ShowplanAttributes(val_id,
					      convertNAString(item_expr->getText(),
							      generator->wHeap()));

	  num_input_entries++;

	  // just give it ITM_CONVERT operator type since there is
	  // no ITM_INOUT type available.
	  input_clause = new(space) ex_inout_clause(ITM_CONVERT, attr, getSpace());

          // The name we assign to the clause will typically be the
          // host variable or dynamic parameter name or an empty name
          // if this is an unnamed dynamic paramter. There is one
          // exception though. When JDBC or ODBC compiles a CALL
          // statement and an IN or INOUT argument is a single UNNAMED
          // dynamic parameter, we assign the FORMAL parameter name to
          // the clause instead of the empty dynamic parameter name.
          const NAString *nameForClause = hostvarOrParamName;
          NABoolean isDynamicParam =
            (item_expr->getOperatorType() == ITM_DYN_PARAM ? TRUE : FALSE);

          if ((isOdbc || isJdbc) && isDynamicParam)
          {
            DynamicParam *param = (DynamicParam *) item_expr;
            const NAString &formalParamName = param->getUdrFormalParamName();
            if (!formalParamName.isNull())
            {
              // Note that we only come here for CALL statements
              const NAString &paramName = param->getName();
              if (paramName.isNull())
              {
                nameForClause = &formalParamName;
              }
            }
          }

          char *nameBuffer = getSpace()->
            AllocateAndCopyToAlignedSpace(*nameForClause, sizeof(Lng32));
          input_clause->setName(nameBuffer);

          if (isJdbc)
	    {
	      if (isDynamicParam) {
		  DynamicParam * param = (DynamicParam *)item_expr;
		  if (NOT param->getParamHeading().isNull())
		  {
		    char * heading =
		      getSpace()->AllocateAndCopyToAlignedSpace(param->getParamHeading(),
								sizeof(Lng32));
		    input_clause->setHeading(heading);
		  }

		if (NOT param->getParamTablename().isNull())
		  {
		    char * tablename =
		      getSpace()->AllocateAndCopyToAlignedSpace(param->getParamTablename(),
								sizeof(Lng32));
		    input_clause->setTableName(tablename);
		  }
		}
	      else { // HostVar
		  HostVar * hv = (HostVar *)item_expr;
		  if (NOT hv->getParamHeading().isNull())
		  {
		    char * heading =
		      getSpace()->AllocateAndCopyToAlignedSpace(hv->getParamHeading(),
								sizeof(Lng32));
		    input_clause->setHeading(heading);
		  }

		if (NOT hv->getParamTablename().isNull())
		  {
		    char * tablename =
		      getSpace()->AllocateAndCopyToAlignedSpace(hv->getParamTablename(),
								sizeof(Lng32));
		    input_clause->setTableName(tablename);
		  }
	      }
	    }

	  // Put CALL statement specific info into input clauses
	  short paramMode = mapMode(item_expr->getParamMode());
	  short paramIdx = (short) item_expr->getHVorDPIndex();

	  if(paramIdx) {
	    ioExpr->setCall(TRUE);
	  }
	  else{
	    ioExpr->setCall(FALSE);
	  }

	  short ordPos = (short) item_expr->getOrdinalPosition();

	  if(ioExpr->isCall()){
	    input_clause->setParamMode(paramMode);
	    input_clause->setParamIdx(paramIdx);
	    input_clause->setOrdPos(ordPos);
	  }
	  else{
	    BriefAssertion(paramMode==0,"Invalid value for param mode");
	    BriefAssertion(paramIdx==0,"Invalid value for param index");
	    BriefAssertion(ordPos==0,"Invalid value for ordinal position");
	    paramMode = PARAMETER_MODE_IN;
	    paramIdx = (short)num_input_entries;
	    ordPos = (short)num_input_entries;
	    input_clause->setParamMode(paramMode);
	    input_clause->setParamIdx(paramIdx);
	    input_clause->setOrdPos(ordPos);
	  }
	  // End
	  linkClause(0, input_clause);
	}
      else
	if ((item_expr->isAUserSuppliedInput()) || //evaluate once functions
		(item_expr->getOperatorType() == ITM_CURRENT_TIMESTAMP) ||
		(item_expr->getOperatorType() == ITM_UNIX_TIMESTAMP) ||
		(item_expr->getOperatorType() == ITM_SLEEP) ||
	    (item_expr->getOperatorType() == ITM_CURRENT_USER) ||
	    (item_expr->getOperatorType() == ITM_SESSION_USER) ||
            (item_expr->getOperatorType() == ITM_EXEC_COUNT) ||
	    (item_expr->getOperatorType() == ITM_CURR_TRANSID))
	{
	  item_expr->preCodeGen(generator);
	  item_expr->codeGen(generator);
	}
    }

  ioExpr->setNumEntries(num_input_entries);

  retCode = endExprGen(expr, -1);

  // Reset ioExpr since after endExprGen() it's possible that the space used
  // for the expression was copied elsewhere and deallocated.
  ioExpr = (InputOutputExpr*)(*expr);

  // if all clauses are inout clauses, then do not evaluate
  // input expr at runtime. This is an optimization.
  // Don't do this optimization if we no longer have a pointer to all clauses
  ex_clause * clause = ioExpr->getClauses();
  NABoolean allInoutClauses = (retCode) ? FALSE : TRUE;
  while ((clause) && (allInoutClauses))
    {
      if (clause->getType() != ex_clause::INOUT_TYPE)
	allInoutClauses = FALSE;

      clause = clause->getNextClause();
    }

  if (allInoutClauses)
    ioExpr->setNoEval(TRUE);

  if (isOdbc || isJdbc)
    {
      // skip type compatibility check at runtime between the user hvar
      // and the input value. Skipping the check will make all conversions
      // (if they are possible) go through. So a user hvar(char,
      // for example) could be converted to an integer input value.
      ioExpr->setSkipTypeCheck(TRUE);
    }

  if (CmpCommon::getDefault(IMPLICIT_HOSTVAR_CONVERSION) == DF_ON)
    {
      ioExpr->setSkipTypeCheck(TRUE);

      ioExpr->setRestrictedSkipTypeCheck(TRUE);
    }

  if (CmpCommon::getDefault(SUPPRESS_CHAR_LIMIT_CHECK) == DF_ON)
    // suppress the fix for TRAFODION-2719, use this only
    // if the new behavior causes a regression in an existing
    // application that cannot be fixed easily
    ioExpr->setSuppressCharLimitCheck(TRUE);

  if ( CmpCommon::getDefault(MARIAQUEST_PROCESS) == DF_ON ) 
    {
      ioExpr->setNoDatetimeValidation(TRUE);
    }

  return 0;
}


// input is a ValueIdList
short ExpGenerator::generateListExpr(const ValueIdList      &val_id_list,
				     ex_expr::exp_node_type  node_type,
				     ex_expr               **expr,
                                     Int32                   atp,
                                     Int32                   atpIndex,
                                     ExpHdrInfo             *hdrInfo)
{
  if (val_id_list.entries() == 0)
    {
      return 0;
    }

  ValueIdList * vidList = (ValueIdList *)&val_id_list;

  /*
  // Do constant folding and simplify expressions.
  ValueIdList tempList;
  tempList = vidList->constantFolding();
  if (!tempList.isEmpty())
    {
      vidList = &tempList;
    }
    */

  initExprGen();
  startExprGen(expr, node_type);

  if ( hdrInfo != NULL )
  {
    if (atp < 0)
    {
      Attributes *attr = generator->getMapInfo( (*vidList)[0] )->getAttr();
      atp = attr->getAtp();
      atpIndex = attr->getAtpIndex();
    }    
    generateHeaderClause(atp, atpIndex, hdrInfo);
  }

  ValueId val_id;
  for (CollIndex i = 0; i < vidList->entries(); i++)
    {
      val_id = (*vidList)[i];

      // generate code for this node and the tree under it
      ItemExpr *itmExpr = val_id.getValueDesc()->getItemExpr();
#ifdef GEN_SF_SC_BUG_TESTING
      if(getenv("GEN_ENABLE_SF_PROTECT_ALL"))
	itmExpr->protectiveSequenceFunctionTransformation(generator);
#endif
      itmExpr = itmExpr->preCodeGen(generator);
      itmExpr->codeGen(generator);
    }

  endExprGen(expr, -1);

  return 0;
}

short ExpGenerator::genGuardedListExpr(const ValueIdSet guard,
                                       const ValueIdList &val_id_list,
                                       ex_expr::exp_node_type node_type,
                                       ex_expr ** expr)
{
  if (val_id_list.entries() == 0 && guard.entries() == 0) {
    return 0;
  }

  ItemExpr *guardedTree = NULL;

  if(val_id_list.entries() == 0 && guard.entries() != 0) {
    guardedTree = new(wHeap()) BoolResult(guard.rebuildExprTree(ITM_AND));
  } else if(guard.entries() == 0) {
    guardedTree = new(wHeap())
      BoolResult(new(wHeap())
                 ItemList(val_id_list.rebuildExprTree(ITM_ITEM_LIST),
                          new(wHeap()) BoolVal(ITM_RETURN_TRUE)));
  } else {

    ItemExpr * trueListTree = new(wHeap())
      ItemList(val_id_list.rebuildExprTree(ITM_ITEM_LIST),
               new(wHeap()) BoolVal(ITM_RETURN_TRUE));

    guardedTree = new(wHeap())
      BoolResult(new (wHeap())
                 BiLogic(ITM_AND,
                         guard.rebuildExprTree(ITM_AND),
                         trueListTree));
  }

  guardedTree->bindNode(generator->getBindWA());

  generateExpr(guardedTree->getValueId(),
               ex_expr::exp_SCAN_PRED,
               expr);

  return 0;
}

// generate code to do Describe Output and move output values
// from executor area to user area.
short ExpGenerator::generateOutputExpr(const ValueIdList &val_id_list,
				       ex_expr::exp_node_type /* node_type */,
				       ex_expr ** expr,
				       RETDesc * ret_desc,
                                       const ItemExprList *spOutParams,
                                       ConstNAStringPtr *colNamesForExpr,
                                       ConstQualifiedNamePtr *tblNamesForExpr)
{
  Lng32 rc;
  Space* space;
  Lng32 num_output_entries;
  InputOutputExpr * ioExpr;

  initExprGen();

  space = getSpace();

  num_output_entries = 0;

  ioExpr = new(getSpace()) InputOutputExpr();
  *expr = ioExpr;

  (*expr)->setLength(getSpace()->getAllocatedSpaceSize());
  startExprGen(expr, ioExpr->getType());

  ValueId val_id;
  ex_inout_clause * output_clause = 0;

  const NABoolean isJdbc =
    (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON ? TRUE : FALSE);
  const NABoolean isOdbc =
    (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON ? TRUE : FALSE);

  const NABoolean isCallStatement = (spOutParams ? TRUE : FALSE);

  // Output expression must produce result in exploded format.
  generator->setExplodedInternalFormat();

  // Normally the name we assign to an unnamed column is "(EXPR)".
  // However for CALL statements, ODBC and JDBC requested we use an
  // empty string.
  NAString empty_colname("(EXPR)", CmpCommon::statementHeap());
  if (isCallStatement && (isOdbc || isJdbc))
    empty_colname = "";

  for (CollIndex i = 0; i < val_id_list.entries(); i++)
  {
    val_id = val_id_list[i];
    ItemExpr * item_expr = val_id.getItemExpr();

    char *nameForClause = 0;
    char *heading = 0;
    char *tableName = 0;
    char *schemaName = 0;
    char *catalogName = 0;

    ItemExpr *spOutExpr = NULL;
    if (isCallStatement)
    {
      spOutExpr = (*spOutParams)[i];
      BriefAssertion(spOutExpr, "Unmatched spOutParams");
    }

    // We allow the caller of this function to inject special names
    // into the generated object. Currently this is only being done
    // for stored procedure result set proxy plans. If a special
    // column name was passed in, colNameFromCaller will point to it.
    const NAString *colNameFromCaller =
      (colNamesForExpr ? colNamesForExpr[i] : NULL);

    // colNameFromRETDesc is the column name from the RETDesc
    const NAString colNameFromRETDesc
      (ret_desc->getColRefNameObj(i).getColNameAsAnsiString());

    // The next IF block will set colname_ptr to point to the name we
    // want in the generated object
    const NAString *colname_ptr = NULL;

    if (colNameFromCaller)
    {
      // The caller wants a special name
      colname_ptr = colNameFromCaller;
    }
    else
    {
      // Use the name from the RETDesc or if that name is not
      // specified, use our manufactured name for unnamed columns
      if (colNameFromRETDesc.length() > 0)
        colname_ptr = &colNameFromRETDesc;
      else
        colname_ptr = &empty_colname;

      // For CALL statements, ODBC and JDBC requested that non-empty
      // parameter names in the SQL text (e.g. "call foo(?x)") take
      // precedence over formal parameter names.
      NAString spOutParamName("", CmpCommon::statementHeap());
      if (isCallStatement && (isOdbc || isJdbc))
      {
        // Even though ODBC and JDBC always do dynamic compiles, we
        // also check for a named static host variable here, to cover
        // the case of SQLJ static compiles. SQLJ static plans are
        // generated with the ODBC_PROCESS default turned on and
        // involve the JDBC runtime stack during execution.
        if (spOutExpr->getOperatorType() == ITM_DYN_PARAM)
          spOutParamName = ((DynamicParam *) spOutExpr)->getName();
        else if (spOutExpr->getOperatorType() == ITM_HOSTVAR)
          spOutParamName = ((HostVar *) spOutExpr)->getName();

        if (spOutParamName.length() > 0)
        {
          colname_ptr = &spOutParamName;
        }
      }

    } // if (colNameFromCaller) else ...

    nameForClause =
      space->AllocateAndCopyToAlignedSpace(*colname_ptr, sizeof(Lng32));

    if (ret_desc->getHeading(i) != NULL)
    {
      heading =
        space->allocateAndCopyToAlignedSpace(ret_desc->getHeading(i),
                                             strlen(ret_desc->getHeading(i)),
                                             sizeof(Lng32));
    }

    // if the output expression (item_expr) and the select list
    // item (from ret_desc) do not have the same type, generate
    // conversion clause.
    item_expr = item_expr->preCodeGen(generator);
    val_id = item_expr->getValueId();

    // If the value is in compressed format, then conversion
    // nodes must be added to convert back to exploded format.
    // Sometimes this is done already - i.e., in the split bottom of top level
    // ESPs.
    NABoolean genConvertNodes = false;
    MapInfo *mapInfo = generator->getMapInfoAsIs(val_id);
    if(mapInfo) {
      genConvertNodes = mapInfo->getAttr()->isSQLMXAlignedFormat();
    }
    else
    {
      if(item_expr->getOperatorType() == ITM_BASECOLUMN) {
        item_expr->codeGen(generator);
        MapInfo *mapInfo = generator->getMapInfoAsIs(val_id);
        if(mapInfo) {
          genConvertNodes = mapInfo->getAttr()->isSQLMXAlignedFormat();
        }
      }
    }

    if ( (NOT(val_id.getType() == ret_desc->getType(i))) || genConvertNodes )
    {
      {
      
         item_expr = new(wHeap()) Cast(item_expr, &(ret_desc->getType(i)));
      }

      item_expr->bindNode(generator->getBindWA());
    }

    ItemExpr * temp = item_expr->preCodeGen(generator);
    ItemExpr *outExpr = NULL;
    rc = foldConstants(temp, &outExpr);
    if ((rc == 0) && (outExpr))
      temp = outExpr;
    temp->codeGen(generator);

    {
      MapInfo *mapInfo = generator->getMapInfoAsIs(temp->getValueId());
      GenAssert(mapInfo, "no mapinf");
      GenAssert(!mapInfo->getAttr()->isSQLMXAlignedFormat(), "Aligned Output");
    }

    //attr = new(wHeap()) Attributes * [1];
    ItemExpr * outNode
      = new(wHeap()) NATypeToItem((NAType *)&temp->getValueId().getType());
    outNode->synthTypeAndValueId();

    Attributes ** attr;
    if (genItemExpr(outNode, &attr, 1, -1) == 1)
      return 0;

    attr[0]->copyLocationAttrs(
      generator->getMapInfo(((ItemExpr *)temp)->getValueId())->getAttr());

    if (genConvertNodes)
    {// bulk move may cause an issue where the values are overriden
      attr[0]->setBulkMoveable(FALSE);
    }
    //attr[0] = getMapTable()->getMapInfo(((ItemExpr *)temp)->getValueId())->getAttr();

    // Get the table/catalog and schema names
    // We get this from the ret_desc because we want the original column
    // list - NOT the VEG transformed column list because some of the
    // columns may have been replaced by constants.

    // We also allow the caller of this function to inject special
    // names into the generated object. Currently this is only being
    // done for stored procedure result set proxy plans.
    const QualifiedName *tblNameFromCaller =
      (tblNamesForExpr ? tblNamesForExpr[i] : NULL);

    // Another case to consider is that NO catalog/schema/table names
    // should be set for this column. In that case, the following
    // boolean should remain FALSE.
    NABoolean tableNameShouldBeSet = FALSE;

    if (tblNamesForExpr)
    {
      if (tblNameFromCaller)
        tableNameShouldBeSet = TRUE;
    }
    else if (((ret_desc->getValueId(i)).getItemExpr()->getOperatorType()
              == ITM_BASECOLUMN) ||
             ((ret_desc->getValueId(i)).getItemExpr()->getOperatorType()
              == ITM_INDEXCOLUMN))
    {
      tableNameShouldBeSet = TRUE;
    }

    if (tableNameShouldBeSet)
    {
      // Initially we set qaname to the caller's special name. If that
      // name was not specified then qaname is changed to point to the
      // name from the RETDesc.
      const QualifiedName *qaname = tblNameFromCaller;

      if (qaname == NULL)
      {
        NAColumn *column = (ret_desc->getValueId(i)).getNAColumn();
        qaname = (const QualifiedName *) column->getTableName(TRUE);
      }

      if (NOT qaname->getObjectName().isNull())
      {
        tableName = space->AllocateAndCopyToAlignedSpace
          (qaname->getObjectName(), sizeof(Lng32));
      }

      // If nametype is Shortansi, we need to extract the schema name
      // from bindWA because at this point the shortansi name has been
      // converted to NSK format. We do not consider SHORTANSI cases
      // when the caller of this function is passing in special table
      // names.
      if (tblNameFromCaller == NULL && SqlParser_NAMETYPE == DF_SHORTANSI)
      {
        SchemaName shortAnsiSchema = ActiveSchemaDB()->getDefaultSchema();

        if (NOT shortAnsiSchema.getSchemaName().isNull())
          schemaName = space->AllocateAndCopyToAlignedSpace
            (shortAnsiSchema.getSchemaName(),sizeof(Lng32));

        if (NOT shortAnsiSchema.getCatalogName().isNull())
          catalogName = space->AllocateAndCopyToAlignedSpace
            (shortAnsiSchema.getCatalogName(),sizeof(Lng32));
      }
      else
      {
        if (NOT qaname->getSchemaName().isNull())
        {
          schemaName = space->AllocateAndCopyToAlignedSpace
            (qaname->getSchemaName(),sizeof(Lng32));
        }
        if (NOT qaname->getCatalogName().isNull())
        {
          catalogName = space->AllocateAndCopyToAlignedSpace
            (qaname->getCatalogName(), sizeof(Lng32));
        }
      }

    } // if (tableNameShouldBeSet)

    output_clause = new(space) ex_inout_clause(ITM_NATYPE, attr, space);

    num_output_entries++;

    output_clause->setName(nameForClause);
    output_clause->setHeading(heading);

    // Put CALL statement specific info into output clauses
    if (spOutExpr)
    {
      short paramMode = mapMode(spOutExpr->getParamMode());
      short paramIdx = (short) spOutExpr->getHVorDPIndex();

      if (paramIdx != 0)
        ioExpr->setCall(TRUE);
      else
        BriefAssertion(0, "Invalid parameter index");

      short ordPos = (short) spOutExpr->getOrdinalPosition();
      output_clause->setParamMode(paramMode);
      output_clause->setParamIdx(paramIdx);
      output_clause->setOrdPos(ordPos);

    }
    else
    {
      short paramMode = PARAMETER_MODE_OUT;
      short paramIdx = 0;
      short ordPos = 0;
      output_clause->setParamMode(paramMode);
      output_clause->setParamIdx(paramIdx);
      output_clause->setOrdPos(ordPos);
    }

    output_clause->setTableName(tableName);
    output_clause->setSchemaName(schemaName);
    output_clause->setCatalogName(catalogName);

    linkClause(0, output_clause);

  } // for each entry in the ValueId list

  if (output_clause)
  {
    short retCode;

    ioExpr->setNumEntries(num_output_entries);

    output_clause->setLastClause();

    retCode = endExprGen(expr, 0);

    // Reset ioExpr since after endExprGen() it's possible that the space used
    // for the expression was copied elsewhere and deallocated.
    ioExpr = (InputOutputExpr*)(*expr);

    // if all clauses are inout clauses, then do not evaluate
    // output expr at runtime. This is an optimization.
    // Don't do this optimization if we no longer have a pointer to all clauses
    ex_clause * clause = ioExpr->getClauses();
    NABoolean allInoutClauses = (retCode) ? FALSE : TRUE;
    while ((clause) && (allInoutClauses))
    {
      if (clause->getType() != ex_clause::INOUT_TYPE)
        allInoutClauses = FALSE;

      clause = clause->getNextClause();
    }

    if (allInoutClauses)
      ioExpr->setNoEval(TRUE);

    if (isOdbc || isJdbc)
    {
      // skip type compatibility check at runtime between the user hvar
      // and the input value. Skipping the check will make all conversions
      // (if they are possible) go through. So a user hvar(char,
      // for example) could be converted to an integer input value.
      ioExpr->setSkipTypeCheck(TRUE);
    }

    if ((CmpCommon::getDefault(IMPLICIT_HOSTVAR_CONVERSION) == DF_ON) ||
        (CmpCommon::getDefault(IMPLICIT_DATETIME_INTERVAL_HOSTVAR_CONVERSION) == DF_ON))
    {
      ioExpr->setSkipTypeCheck(TRUE);

      ioExpr->setRestrictedSkipTypeCheck(TRUE);
    }
  } // output_clause

  return 0;
}

// input is a ValueIdSet
short ExpGenerator::generateSetExpr(const ValueIdSet &val_id_set,
				    ex_expr::exp_node_type node_type,
				    ex_expr ** expr,
				    short gen_last_clause,
                                    ExpHdrInfo * hdrInfo)
{
  ValueId val_id;
  val_id = val_id_set.init();
  if (!val_id_set.next(val_id))
    return 0; // empty set

  initExprGen();
  startExprGen(expr, node_type);

  if (hdrInfo != NULL)
  {
    Attributes *attr = generator->getMapInfo(val_id)->getAttr();
    generateHeaderClause(attr->getAtp(), attr->getAtpIndex(), hdrInfo);
  }

  for (val_id = val_id_set.init(); val_id_set.next(val_id); val_id_set.advance(val_id))
    {
      // generate code for this node and the tree under it
      ItemExpr * temp =
	val_id.getValueDesc()->getItemExpr()->preCodeGen(generator);
      temp->codeGen(generator);
    }

  endExprGen(expr, gen_last_clause);

  return 0;
}

// generateSequenceExpression
//
short ExpGenerator::generateSequenceExpression(const ValueIdSet &sequenceItems, 
                                               ex_expr* &expr)
{
  // If there are no items, just return
  //
  if (sequenceItems.isEmpty()) return 0;

  initExprGen();
  setInSequenceFuncExpr(TRUE);
  startExprGen(&expr, ex_expr::exp_ARITH_EXPR);

  // Loop over the sequence items calling the protective short-circuit
  // transformation, preCodeGen and codeGen on each.
  //
  ValueId valId;
  for (valId = sequenceItems.init();
       sequenceItems.next(valId);
       sequenceItems.advance(valId))
    {
      ItemExpr *itmExpr = valId.getValueDesc()->getItemExpr();
#ifdef GEN_SF_SC_BUG_TESTING
      if(!getenv("GEN_DISABLE_SF_PROTECT"))
#endif
	itmExpr->protectiveSequenceFunctionTransformation(generator);
      itmExpr = valId.getValueDesc()->getItemExpr();
      itmExpr = itmExpr->preCodeGen(generator);
      itmExpr->codeGen(generator);
    }

  setInSequenceFuncExpr(FALSE);
  // Finalize the expression generation machinery.
  //
  endExprGen(&expr, 1);

  return 0;
}

// input is a list of ValueIdUnion.
short ExpGenerator::generateUnionExpr(const ValueIdList &val_id_list,
				      ex_expr::exp_node_type node_type,
				      ex_expr ** left_expr,
				      ex_expr ** right_expr)
{
  // generate code to move the left child row to result row

  initExprGen();
  startExprGen(left_expr, node_type);

  Space * space = getSpace();

  CollIndex i = 0;
  for (i = 0; i < val_id_list.entries(); i++)
    {
      ValueIdUnion * vidu_node = (ValueIdUnion *)(((val_id_list[i]).getValueDesc())->getItemExpr());
      ItemExpr * left_node = ((vidu_node->getLeftSource()).getValueDesc())->getItemExpr();

      Attributes ** attr = new(wHeap()) Attributes * [2];
      ex_conv_clause * conv_clause;

      /* assign result attributes*/
      attr[0] = generator->getMapInfo(vidu_node->getResult())->getAttr();

      ItemExpr * temp = left_node->preCodeGen(generator);
      temp->codeGen(generator);

      attr[1] = generator->getMapInfo(((ItemExpr *)temp)->getValueId())->getAttr();
      conv_clause = new(space) ex_conv_clause(ITM_CONVERT, attr, space);
      linkClause(0, conv_clause);
    }

  endExprGen(left_expr, -1);

  // generate code to move the right child row to result row
  initExprGen();
  startExprGen(right_expr, node_type);

  // Reset space since temporary space object may be used
  space = getSpace();

  for (i = 0; i < val_id_list.entries(); i++)
    {
      ValueIdUnion * vidu_node = (ValueIdUnion *)(((val_id_list[i]).getValueDesc())->getItemExpr());
      ItemExpr * right_node = ((vidu_node->getRightSource()).getValueDesc())->getItemExpr();

      Attributes ** attr = new(wHeap()) Attributes * [2];
      ex_conv_clause * conv_clause;

      /* assign result attributes*/
      attr[0] = generator->getMapInfo(vidu_node->getResult())->getAttr();

      ItemExpr * temp = right_node->preCodeGen(generator);
      temp->codeGen(generator);

      attr[1] = generator->getMapInfo(((ItemExpr *)temp)->getValueId())->getAttr();
      conv_clause = new(space) ex_conv_clause(ITM_CONVERT, attr, space);
      linkClause(0, conv_clause);
    }

  endExprGen(right_expr, -1);

  return 0;
}


short ExpGenerator::processAttributes(ULng32 numAttrs,
				      Attributes ** attrs,
				      ExpTupleDesc::TupleDataFormat tdataF,
				      ULng32 &tupleLength,
				      Int32 atp,
				      Int32 atpIndex,
				      ExpTupleDesc ** tupleDesc,
				      ExpTupleDesc::TupleDescFormat tdescF,
				      ULng32 startOffset,
                                      ExpHdrInfo  * hdrInfo)
{
  ULng32 tuppDescFlags = 0x0;

  ExpTupleDesc::computeOffsets(numAttrs,
			       attrs,
			       tdataF,
			       tupleLength,
			       startOffset,
			       &tuppDescFlags,
			       &tdataF,
                               hdrInfo);

  for (Int32 i = 0; i < (Int32) numAttrs; i++)
    {
      attrs[i]->setAtp(atp);
      attrs[i]->setAtpIndex(atpIndex);
      attrs[i]->setDefaultFieldNum(i);		// this is needed for handling
						// SQL/MP added cols correctly.
    }

  // if tuple descriptor is to be returned, allocate it in 'space'
  if (tupleDesc)
    {
      *tupleDesc
	= new(getSpace()) ExpTupleDesc(numAttrs, attrs, tupleLength,
				       tdataF, tdescF, getSpace());
      (*tupleDesc)->setFlags(tuppDescFlags);
    }

  return 0;
}

short ExpGenerator::processValIdList(ValueIdList valIdList,
				     ExpTupleDesc::TupleDataFormat tdataF,
				     ULng32 &tupleLength,
				     Int32 atp,
				     Int32 atpIndex,
				     ExpTupleDesc ** tupleDesc,
				     ExpTupleDesc::TupleDescFormat tdescF,
				     Lng32 startOffset,
				     Attributes ***returnedAttrs,
                                     NAColumnArray *colArray,
                                     NABoolean isIndex,
                                     NABoolean placeGuOutputFunctionsAtEnd,
                                     ExpHdrInfo * hdrInfo)
{
  MapTable * myMapTable = generator->appendAtEnd();
  Attributes ** attrs = new(wHeap()) Attributes * [valIdList.entries()];

  NABoolean alignedFormat = (tdataF == ExpTupleDesc::SQLMX_ALIGNED_FORMAT);
  NAColumn *col;

  for (CollIndex i = 0; i < valIdList.entries(); i++)
    {
      // get a pointer to the map table attributes and set them the
      // way we want them to be
      attrs[i] = (generator->addMapInfoToThis(myMapTable, valIdList[i], 0))->getAttr();

      // If this is a GenericUpdateOutputFunction
      // and we want these handled specially - mark the attr.
      if (valIdList[i].getItemExpr()->isAGenericUpdateOutputFunction()
          && placeGuOutputFunctionsAtEnd)
	attrs[i]->setGuOutput();

      // There are times we don't have a column array and must use the value id
      // list to get to the attribute and mark it special (DP2Scan is one place).
      // The attribute must be marked 'special' before computing offsets since
      // for the aligned format we may re-arrange fixed size columns, but
      // can NOT re-arrange added columns.
      // Note that indexes (non-clustering indexes) do not have added columns,
      // because we cannot do an alter index add column operation.
      if ( alignedFormat &&
           !isIndex &&
           ( colArray == NULL ) &&
           ((col = valIdList[i].getNAColumn( TRUE )) != NULL) &&
           col->isAddedColumn() )
        attrs[i]->setAddedCol();
    }

  // Ensure added columns are tagged before computing offset for the aligned
  // record format since added columns are arranged differently.
  if ( (colArray != NULL) )
  {
    for( Int32 i = 0; i < (Int32)colArray->entries(); i++ )
    {
      col = (*colArray)[i];
      if (( col->isAddedColumn() ) && (!isIndex))
        attrs[i]->setAddedCol();
    }
  }

  ExpHdrInfo *retHdrInfo = NULL;
  if ( isHeaderNeeded(tdataF) )
    retHdrInfo = hdrInfo;

  // compute offsets and create tuple descriptor.
  processAttributes((ULng32)valIdList.entries(), attrs, tdataF,
		    tupleLength, atp, atpIndex,  tupleDesc, tdescF,
		    startOffset, retHdrInfo);

  if (returnedAttrs)
    {
      // caller wants array of Attributes pointers returned
      *returnedAttrs = attrs;
    }
  else
    {
      // deallocate the attributes array. Don't deallocate the attributes
      // pointed to by the array entries since we didn't allocate them.
      NADELETEBASIC( attrs, wHeap());
    }

// Below is commented out:  it doesn't work because we're adding the wrong
// attributes.  Can't get the correct ones any more because they will have
// been removed by ExpGenerator::endExprGen (removeLast).
// (This code was added, found wanting, and commented out trying to fix
// Genesis 10-980112-5942.)
//
//for (i = 0; i < valIdList.entries(); i++)
//  {
//    ValueId vid = valIdList[i];
//    ValueId vidRepl = vid.getItemExpr()->getReplacementExpr()->getValueId();
//    if (vidRepl != vid)
//	myMapTable->addMapInfo(vidRepl, myMapTable->getMapInfo(vid)->getAttr());
//  }

  return 0;
}


short ExpGenerator::computeTupleSize(const ValueIdList & valIdList,
                                     ExpTupleDesc::TupleDataFormat tdataF,
                                     ULng32 &tupleLength,
                                     Lng32 startOffset,
                                     UInt32 * varCharSize,
                                     UInt32 * headerSize)
{
  MapTable * myMapTable = generator->appendAtEnd();
  UInt32 vcSize = 0;
  UInt32 numAttrs = valIdList.entries();

  Attributes ** attrs = new(wHeap()) Attributes * [numAttrs];
  
  for (CollIndex i = 0; i < valIdList.entries(); i++)
  {
    attrs[i] = (generator->addMapInfoToThis(myMapTable, valIdList[i], 0))->getAttr();
  }

  UInt32 rtnFlags = 0x0;
  if (varCharSize)
    *varCharSize = 0;

  ExpTupleDesc::computeOffsets((ULng32)numAttrs,
                               attrs,
                               tdataF,
                               tupleLength,
                               startOffset,
                               &rtnFlags,
                               NULL,
                               NULL,
                               headerSize,
                               varCharSize);
 
   // deallocate the attributes array. 
   NADELETEBASIC( attrs, wHeap());
  
   if (myMapTable != NULL)
    generator->unlinkMe(myMapTable);


  return 0;
}

short ExpGenerator::assignAtpAndAtpIndex(ValueIdList valIdList,
					 Int32 atp,
					 Int32 atpIndex)
{
  for (CollIndex i = 0; i < valIdList.entries(); i++)
    {
      Attributes * attr = generator->getMapInfo(valIdList[i])->getAttr();
      if (atp >= 0)
	attr->setAtp(atp);
      if (atpIndex >= 0)
	attr->setAtpIndex(atpIndex);
    }

  return 0;
}

Int32 shafoo() {
  return 10;
}

// Called just before the start of expr generation to determine what space
// object to store the expression in.
void ExpGenerator::initExprGen()
{
#ifdef NEW_LEAN_EXPR
  // Attempt to create tempSpace only if we are compiling for leaner
  // expressions.  Also, only do this if PCODE generation is on *and* we're not
  // just generating PCODE for clause evaluation purposes.  Also, the internal
  // PCODE_NO_LEANER_EXPR mode may be set if there are certain expressions we
  // just don't want to make leaner for whatever reasons.  And also :), don't
  // do this if we're compiling for a showplan.

  if ((CmpCommon::getDefault(GEN_LEANER_EXPRESSIONS) == DF_ON) &&
      !(pCodeMode_ & ex_expr::PCODE_NO_LEANER_EXPR) &&
      (pCodeMode_ & ex_expr::PCODE_ON) &&
      !(pCodeMode_ & ex_expr::PCODE_EVAL))
  {
    // Only create tempSpace if it hasn't been created already.  This is done
    // specifically because AggrExpr attempts to create the expression first by
    // calling new() (as opposed to letting startExprGen do it), and then calls
    // generateSetExpr() to create all the clauses.  Since both need to be
    // in tempSpace, initExprGen() gets called twice, so we don't want to
    // create/delete tempSpace twice.

    if (generator->getTempSpace() == NULL) {
      ComSpace * space = new(wHeap()) ComSpace(ComSpace::GENERATOR_SPACE);
      space->setParent(wHeap());
      generator->setTempSpace(space, TRUE);
    }
  }
#endif
}

// called at the start of expr generation
short ExpGenerator::startExprGen(ex_expr ** expr,
				 ex_expr::exp_node_type node_type)
{
  if (*expr == 0)
    *expr = new(getSpace()) ex_expr(node_type);
  (*expr)->setLength(getSpace()->getAllocatedSpaceSize());

  // copy handleIndirectVC flag from expGenerator to expr
  (*expr)->setHandleIndirectVC( handleIndirectVC() );

  // copy forInsertUpdate flag from expGenerator to expr
  (*expr)->setForInsertUpdate( forInsertUpdate() );

  // set up pointer to NExDbgInfo object
  (*expr)->setNExDbgInfo( generator->getNExDbgInfoAddr() ) ;

  mapTableAllocated = -1;
  mapTable_ = generator->appendAtEnd();

  initClauseList();
  initConstantList();
  initPersistentList();

  savePCodeMode();

  return 0;
}

// Input: gen_last_clause, if not passed in or -1, generate a last NO-OP clause
short ExpGenerator::endExprGen(ex_expr ** expr, short gen_last_clause)
{
  UInt32 optFlags;
  short retCode = 0;

  gen_last_clause = 0;

  // Certain expressions may request no PCode to be generated, otherwise
  // get the PCode mode from the ctor ExpGenerator and set it in the
  // expression itself.  This allows us to regenerate PCode at runtime
  // (for versioning) with the same mode specified at compile time.
  (*expr)->setPCodeMode( getPCodeMode() );

  // Get default low-level opt flags and prevent predicate reordering from
  // happening if this expression represents a case statement.
  optFlags = (UInt32)CmpCommon::getDefaultLong(PCODE_OPT_FLAGS);

  UInt32 Enbld = (UInt32)CmpCommon::getDefaultLong(PCODE_NE_ENABLED);
  if ( Enbld == 0 )
     optFlags &= ~( PCodeCfg::NATIVE_EXPR ); // Disable Native Exprs in optFlags

  Enbld = (UInt32)CmpCommon::getDefaultLong(PCODE_EXPR_CACHE_ENABLED);
  if ( Enbld == 0 )
     optFlags |= PCodeCfg::OPT_PCODE_CACHE_DISABLED ; // Disable feature in optFlags

  Enbld = (UInt32)CmpCommon::getDefaultLong(PCODE_EXPR_CACHE_CMP_ONLY);
  if ( Enbld == 1 )
     optFlags |= ( PCodeCfg::EXPR_CACHE_CMP_ONLY ); // Enable Compare-Only mode -- FOR TESTING!

  if (caseStmtGenerated()) {
    optFlags &= ~(PCodeCfg::REORDER_PREDICATES);
    setCaseStmtGenerated(FALSE);
  }

  // Set pcode low-level optimization flags for this expression
  (*expr)->setPCodeOptFlags(optFlags);

  // Set pcode Max Branch Cnt and Max Instr Cnt for PCODE optimization
  UInt32 maxBrnchCnt = (UInt32)CmpCommon::getDefaultLong(PCODE_MAX_OPT_BRANCH_CNT);
  UInt32 maxInstCnt  = (UInt32)CmpCommon::getDefaultLong(PCODE_MAX_OPT_INST_CNT);
  (*expr)->setPCodeMaxOptBrCnt(maxBrnchCnt);
  (*expr)->setPCodeMaxOptInCnt(maxInstCnt);

  if (gen_last_clause)
    {
      /* generate a no-op clause and set it to be the last clause */
      ex_noop_clause *clause = new(getSpace()) ex_noop_clause;
      clause->setLastClause();
      linkClause(0, clause);
    }

  // fixup the next clause pointers
  fixupNextClause();

  // Set the constant expression storage length and area.
  //
  char * constants_area = placeConstants(getConstantList(), getConstLength());
  (*expr)->setConstantsArea(constants_area);
  (*expr)->setConstsLength(getConstLength());

  // Set the persistent expression storage length and initialization area.
  //
  char *persistentInitializationArea
    = placeConstants(getPersistentList(), persistentLength());
  (*expr)->persistentInitializationArea() = persistentInitializationArea;
  (*expr)->persistentLength() = persistentLength();
  (*expr)->persistentArea()
    = getSpace()->allocateAlignedSpace(persistentLength());


  // Set the temporary expression storage length.
  //
  (*expr)->setTempsLength(getTempsLength());

  // set the pointer to the first clause in the ex_expr
  (*expr)->setClauses((ex_clause *)(getClauseList()->getElement()));

  // Fixup the pointers to the constant, temporary, and persistent
  // expression areas.
  //
  //  (*expr)->setFixupConstsAndTemps((short)getFixupConstsAndTemps());

  if (mapTableAllocated)
    {
      mapTable_ = 0;
      generator->removeLast();
    }

  Lng32 len1 = getSpace()->getAllocatedSpaceSize();

  // generate pcode at compile time
  UInt32 f = 0;
  ex_expr_base::setForShowplan(f, FALSE);
  if (generator->genNoFloatValidatePCode())
    ex_expr_base::setNotValidateFloat64(f, TRUE);

  (*expr)->pCodeGenerate(getSpace(), wHeap(), f);
  (*expr)->setPCodeGenCompile(1);
  if ((*expr)->getPCodeNative() &&
      CmpCommon::getDefault(PCODE_NE_IN_SHOWPLAN) == DF_ON)
    (*expr)->setNEInShowplan(TRUE);

  Lng32 pcodeExprLen = getSpace()->getAllocatedSpaceSize() - len1;
  Lng32 totalExprLen =
    getSpace()->getAllocatedSpaceSize() - (*expr)->getLength();

  (*expr)->setLength(totalExprLen);
#ifdef NEW_LEAN_EXPR
  // If tempSpace exists, then the entire expression was written to it, so
  // we need to now make a "lean" copy of it and copy it back into the
  // fragment space.

  // If we are in the process or generating container expressions, then
  // no need to copy to fragment space yet, let's wait till we complete
  // generating the main expression.
  // Note: The term Container Expression is used to denote an expression
  // - ex_expr within an expression - ex_expr.
  if(inContainerExpr())
    return 0;

  ComSpace * tempSpace = generator->getTempSpace();

  NABoolean saveClauses = (getShowplan() || saveClausesInExpr());

  if (tempSpace)
  {
    ex_expr_base * newExpr = NULL;
    NABoolean branchFound = FALSE;

    // Reset tempSpace back to NULL.  Calls to getSpace() will now return the
    // fragment space.
    generator->setTempSpace(NULL, FALSE);

    // If no pCode was generated for this expression, or if the generated
    // pCode contains a clause eval instruction, we need to selectively copy
    // in the expression from tempSpace into the fragment space.  Otherwise,
    // the expression can completely rid itself of it's clauses and we can
    // make a very lean copy of it.

    // Copying some expressions is too hard because they contain clauses with
    // too many embedded pointers which need copying.  Be selective.
    NABoolean tooHard = FALSE;
    for (ex_clause* clause = (*expr)->getClauses();
         clause;
         clause = clause->getNextClause())
      {
        if ((clause->getClassID() == ex_clause::FUNC_HBASE_COLUMN_CREATE) ||
	    (clause->getClassID() == ex_clause::FUNC_HBASE_COLUMNS_DISPLAY))
	  tooHard = TRUE;
        break;
      }

    if (!tooHard && ((*expr)->getPCodeSegment() != NULL))
    {
      // Needed to set the length of the expression
      len1 = getSpace()->getAllocatedSpaceSize();

      // Previous clause ptr
      ex_clause* prevClause = NULL;

      // Determine size of expression.
      totalExprLen = (*expr)->getClassSize();

      // Allocate space in fragment space for just the expression class and
      // copy it over from the expr (which is in tempSpace).
      newExpr = (ex_expr_base*)
          new(generator->getSpace()) char[totalExprLen];
      str_cpy_all((char*)newExpr, (char*)(*expr), totalExprLen);

      // Call makeLeanCopyAll to just copy over the pCode and other stuff (but
      // not the clauses).  Note, this routine is different than makeLeanCopy
      // since it makes the existing expression lean, as opposed to storing
      // the expression in an "expr_lean" expression.  The advantages are that
      // all expressions are supported (which is exactly what we need).  The
      // disadvantages are that the expression isn't as lean as it could be.
      newExpr->makeLeanCopyAll(generator->getSpace());

      // Initially assume that there are no clauses needed to copy over
      newExpr->setClauses(NULL);

      // Walk through clauses list if expression contains EVAL instruction.
      // Otherwise we're pretty much done since PCODE was already copied over.
      if((*expr)->getPCodeSegment()->containsClauseEval() || saveClauses) {
        for (ex_clause* clause = (*expr)->getClauses();
            clause;
            clause = clause->getNextClause())
        {
          short clauseSize;

          // If this has pCode, then we don't need to copy the clause.  Change
          // retCode, however, to reflect the fact that we got rid of a clause.
          // If we're generating a showplan, all clauses are to be emitted.
          if (!clause->noPCodeAvailable() && !saveClauses)
          {
            retCode = 1;
            continue;
          }

          // TBD:
          // Special case given to ExFunctionRandomSelection since this class
          // does not define the method getClassSize().  Adding it would require
          // a DP2 rebuild.  Maybe later - but soon!

          clauseSize =
            (clause->getClassID() == ex_clause::FUNC_RAND_SELECTION_ID)
              ? sizeof(ExFunctionRandomSelection)
              : clause->getClassSize();

          // Record size of space before allocation of clause and other stuff.
          Lng32 beforeSize = getSpace()->getAllocatedSpaceSize();

          // Allocate space for new clause
          ex_clause* newClause = (ex_clause*) new(getSpace()) char[clauseSize];

          // Copy Clause
          str_cpy_all((char*)newClause, (char*)clause, clauseSize);

          // Null-terminate next clause pointer
          newClause->setNextClause(NULL);

          newClause->copyOperands(clause, getSpace());
          newExpr->getPCodeSegment()->replaceClauseEvalPtr(clause, newClause);

          if (newExpr->getClauses() == NULL) {
            newExpr->setClauses(newClause);
          }
          else {
            prevClause->setNextClause(newClause);
          }

          // If a branch clause was previously seen, then we need to fix up it's
          // branch_clause and saved_next pointers (since they were initialized
          // to 0 in the newly copied clause.  The clause currently being
          // processed needs to be examined to see if it's one of these two
          // pointers for the branch clause.  The algorithm used below can be
          // improved if we allocate a list of pointers to branch clauses, but
          // I think simplicity outweights performance in this case.

          if (branchFound)
          {
            for (ex_clause* tempClause = newExpr->getClauses(); tempClause;
                 tempClause = tempClause->getNextClause())
            {
              if (tempClause->getType() == ex_clause::BRANCH_TYPE) {
                ex_branch_clause* branchClause = (ex_branch_clause*)tempClause;
  
                if (branchClause->get_branch_clause() == clause) {
                  branchClause->set_branch_clause(newClause);
                  break;
                }
                else if (branchClause->get_saved_next() == clause) {
                  branchClause->set_saved_next(newClause);
                  break;
                }
              }
            }
          }

          // If we're keeping a branch clause, we have to keep that clauses
          // associated clauses as well (i.e. branch_clause and saved_next
          // clauses.  This is because at runtime we do a check to see if the
          // two are equal to each other.  Also, when we pack the branch clause,
          // we also try and pack the associated clauses - which would lead to
          // an error if that didn't exist.  There may be a workaround here.
          // Also set the branchFound flag here.
  
          if (newClause->getType() == ex_clause::BRANCH_TYPE) {
            ex_branch_clause* branchClause = (ex_branch_clause*)newClause;

            // Only set no-pcode-available flag if we're not generating a
            // showplan.  If we are generating a showplan, then the clauses
            // will already get emitted.  This way we don't falsely report that
            // no pcode is available.

            if (!saveClauses) {
              branchClause->get_branch_clause()->setNoPCodeAvailable(TRUE);
              branchClause->get_saved_next()->setNoPCodeAvailable(TRUE);
            }

            branchFound = TRUE;
          }

          // Record size of space after clause and other stuff allocated.
          Lng32 afterSize = getSpace()->getAllocatedSpaceSize();

          // If showplan is being generated, make sure to discount the size of
          // those clauses which would be deleted if this wasn't a showplan.
          if (saveClauses)
            len1 += (afterSize - beforeSize);

          prevClause = newClause;
        }
      }

      // Search for embedded addresses in pcode segment and fixup if needed
      newExpr->getPCodeSegment()->replaceAttributesPtr(newExpr->getClauses(),
                                                       getSpace());

      newExpr->setLength(getSpace()->getAllocatedSpaceSize() - len1);

      // Point expr to new expression stored in fragment space
      *expr = (ex_expr*)newExpr;

    }
    else
    {
      // Allocate space in fragment for entire expression.
      newExpr = (ex_expr_base*)
        new(getSpace()) char[tempSpace->getAllocatedSpaceSize()];

      // Call the "pack" routine for the base expr class (via "packExpr").
      // This is done so that we can rebase it starting at the expression's
      // new location in the fragment space once it's copied over.  Also note,
      // only the base expr needs to be packed since any other expression
      // pointed to in the expression was already packed and rebased in the
      // fragment space.
      (*expr)->packExpr(tempSpace);

      // Make a full copy of expr from tempSpace to fragment space
      tempSpace->makeContiguous((char*)newExpr,
				 tempSpace->getAllocatedSpaceSize());

      // Point expr to new expression stored in fragment space
      *expr = (ex_expr*)newExpr;

      // Call the "unpack" routine for the base expr class for rebasing.
      (*expr)->unpackExpr((void*)(*expr), 0);
    }

    delete tempSpace;
  }

#else

  if (genLeanExpr())
    {
      // restore the original fragment space.
      ComSpace * tempSpace = generator->getTempSpace();
      generator->setTempSpace(NULL, FALSE);

      ex_expr_base * newExpr = NULL;
      if (((*expr)->getPCodeSegment() == NULL) ||
	  ((*expr)->getPCodeSegment()->containsClauseEval()))
	{
	  // make full copy of expr in fragment space
	  newExpr = (ex_expr_base*)
	    new(generator->getSpace()) char[tempSpace->getAllocatedSpaceSize()];
	  (*expr)->pack(tempSpace);
	  tempSpace->makeContiguous((char*)newExpr,
				    tempSpace->getAllocatedSpaceSize());
	  *expr = (ex_expr*)newExpr;
	  (*expr)->unpack((void*)(*expr), 0);

	  generator->setGenLeanExpr(FALSE);
	}
      else
	{
	  len1 = getSpace()->getAllocatedSpaceSize();

	  // make lean copy of expr in fragment space
	  newExpr =
	    new(generator->getSpace()) ex_expr_lean();
	  (*expr)->makeLeanCopy((ex_expr_lean*)newExpr, generator->getSpace());
	  *expr = (ex_expr*)newExpr;

	  totalExprLen =
	    getSpace()->getAllocatedSpaceSize() - len1;
	  pcodeExprLen = ((ex_expr_lean*)newExpr)->getPCodeSize();

	  ((ex_expr_lean*)newExpr)->setLength(totalExprLen);
	}

      delete tempSpace;
    }

#endif /* NEW_LEAN_EXPR */

  // clear cache of generated subexpressions
  cache_->clear();

  restorePCodeMode();

  return retCode;
}

void ExpGenerator::initClauseList()
{
  clause_list = new(wHeap()) AListNode(wHeap());
};


void ExpGenerator::linkClause(ItemExpr * node, ex_clause * clause)
{
  if (node)
    {
      setClauseLinked(TRUE);

      node->setClause(clause);
    }

  clause_list->linkElement(clause);

  // Do some specialized processing.

  // If this is a function clause, set the origFunctionOperType.
  // This is used to return correct error info at runtime.
  if ((clause->getType() == ex_clause::FUNCTION_TYPE) &&	node &&
      (((ex_function_clause *)clause)->getOperType() != node->origOpType()))
    {
      ((ex_function_clause *)clause)->setDerivedFunction(TRUE);
      ((ex_function_clause *)clause)->setOrigFunctionOperType(
                                                        node->origOpType());
    }
};

void ExpGenerator::fixupNextClause()
{
  AListNode0 *curr_node = clause_list->getHead();

  unsigned short currClauseNum = 0;
  while (curr_node->getNext())
    {
      ((ex_clause *)(curr_node->getElement()))->setNextClause((ex_clause *)(curr_node->getNext()->getElement()));
      if (((ex_clause *)(curr_node->getElement()))->getType() == ex_clause::BRANCH_TYPE)
	{
	  ((ex_clause *)(curr_node->getElement()))->setNextClause((ex_clause *)(curr_node->getNext()->getElement()));
	  ((ex_branch_clause *)curr_node->getElement())->set_saved_next((ex_clause *)(curr_node->getNext()->getElement()));
	}

      if (!curr_node->getNext())
	{
	  // this is the last clause. Mark it so.
	  ((ex_clause *)(curr_node->getElement()))->setLastClause();
	}

      ((ex_clause *)(curr_node->getElement()))->clauseNum() = ++currClauseNum;
      curr_node = curr_node->getNext();
    };
}

void ExpGenerator::initConstantList()
{
  constant_list = new(wHeap()) AListNode(wHeap());
  constants_length = 0;
  temps_length = 0;
};

void ExpGenerator::initPersistentList()
{
  persistentList_ = new(wHeap()) AListNode(wHeap());
  persistentLength_ = 0;
};

AListNode * ExpGenerator::getConstantList()
{
  return constant_list;
};

AListNode * ExpGenerator::getPersistentList()
{
  return persistentList_;
};

void ExpGenerator::linkConstant(void * expr_tree)
{
  constant_list->linkElement(expr_tree);
};

void ExpGenerator::linkPersistent(void *expTree)
{
  persistentList_->linkElement(expTree);
};

char * ExpGenerator::placeConstants(AListNode *list, Int32 length)
{
  // If there are no constants/persistents, bug out...
  //
  if(length == 0) return 0;

  // Allocate space for storing the image of the constants
  //
  char *area = getSpace()->allocateAlignedSpace(length);

  // All uninitialized constant/peristent variables must be zero.
  //
  // All memory from Space is zeroed out by default.
  // memset(area, 0, length);

  // Loop over each constant in the list and construct it in the image area.
  // The data format for the constants area is assumed to be in
  // SQLARK_EXPLODED_FORMAT.
  // See also method ConstValue::getOffsetsinBuffer()
  ConstValue * item;
  AListNode0 *current = list->getHead();
  while (current->getNext())
    {
      item = (ConstValue *)
	(((ExprNode *)(current->getElement()))->castToItemExpr());

      Attributes * attr
	= generator->getMapInfo(item->getValueId())->getAttr();

      if ((attr->getNullFlag()) && (item->isNull())) // null constant
	{
          ExpTupleDesc::setNullValue( area + attr->getNullIndOffset(),
                                      attr->getNullBitIndex(),
                                      attr->getTupleFormat() );
	}
      else
	{
	  if (attr->getNullFlag())
	    {
              ExpTupleDesc::clearNullValue( area + attr->getNullIndOffset(),
                                            attr->getNullBitIndex(),
                                            attr->getTupleFormat() );
	    }

	  if (attr->getVCIndicatorLength() > 0)
	    {
	      str_cpy_all
		(&area[attr->getVCLenIndOffset()],
		 (char *)item->getConstValue()
		 + item->getValueId().getType().getSQLnullHdrSize(),
		 attr->getVCIndicatorLength());
	    }

	  str_cpy_all
	    (&area[attr->getOffset()],
	     (char *)item->getConstValue()
	     + item->getValueId().getType().getSQLnullHdrSize()
	     + attr->getVCIndicatorLength(),
	     attr->getLength());
	}
      current = current->getNext();
    }

  return area;
}

NABoolean GenEvalPredicate(ItemExpr * rootPtr)
{
  // set up binder/generator stuff so expressions could be generated.
  InitSchemaDB();
  CmpStatement cmpStatement(CmpCommon::context());
  ActiveSchemaDB()->createStmtTables();
  BindWA       bindWA(ActiveSchemaDB(), CmpCommon::context());
  Generator    generator(CmpCommon::context());
  ExpGenerator expGen(&generator);
  generator.appendAtEnd();

  generator.setBindWA(&bindWA);
  generator.setExpGenerator(&expGen);
  FragmentDir * compFragDir = generator.getFragmentDir();

  // create the fragment (independent code space) for this expression
  CollIndex myFragmentId = compFragDir->pushFragment(FragmentDir::MASTER);

  // space where RCB will be generated
  Space * space = generator.getSpace();

  // allocate a work cri desc to encode keys. It has
  // 2 entries: 0, for consts. 1, for temps.
  ex_cri_desc * workCriDesc = new(space) ex_cri_desc(2, space);

  ex_expr * expr = 0;

  ItemExpr * boolResult = new(generator.wHeap()) BoolResult(rootPtr);

  boolResult->bindNode(generator.getBindWA());

  expGen.generateExpr(boolResult->getValueId(),
		      ex_expr::exp_SCAN_PRED,
		      &expr);

  // create a DP2 expression and initialize it with the key encode expr.
  ExpDP2Expr * dp2Expr = new(space) ExpDP2Expr(expr,
					       workCriDesc,
					       space);

  dp2Expr->setPCodeMode( expGen.getPCodeMode() );

  dp2Expr->getExpr()->fixup(0,0,0,NULL,NULL,FALSE,NULL);

  atp_struct * workAtp = dp2Expr->getWorkAtp();

  generator.removeAll();

  if (dp2Expr->getExpr()->eval(workAtp, 0) == ex_expr::EXPR_TRUE)
    return TRUE;
  else
    return FALSE;
}

short ExpGenerator::generateSamplingExpr(const ValueId &valId, ex_expr **expr,
					 Int32 &returnFactorOffset)
{
  // Prime the expression generator.
  //
  initExprGen();
  startExprGen(expr, ex_expr::exp_SCAN_PRED);

  // PreCodeGen the expression.
  //
  ItemExpr *temp = valId.getValueDesc()->getItemExpr()->preCodeGen(generator);
  if (!temp) return -1;

  // Set the result of the sampling expression to be a persistent variable.
  // This must be done after preCodeGen because the original item expression
  // is changed. Also, reset the codeGenerated flag for the item so
  // that the actual code will be generated. This is not the usual way
  // to use a persistent variable.
  //
  MapInfo *mapInfo=addPersistent(temp->getValueId(), generator->getMapTable());
  mapInfo->resetCodeGenerated();

  // If there were no errors, codeGen the expression.
  //
  temp->codeGen(generator);

  // Finialize the expression generator.
  //
  endExprGen(expr, 0);

  // Assert that the result of the sampling expression is stored in
  // persisent space. And, compute the offset for the result.
  //
  mapInfo = generator->getMapInfoAsIs(temp->getValueId());
  GenAssert(mapInfo, "Sampling result not in map table!");

  Attributes *attr = mapInfo->getAttr();
  GenAssert(attr->getAtpIndex() == 1,
	    "Sampling result must be a persistent expression variable.");

  returnFactorOffset = attr->getOffset();

  return 0;
}

MapInfo *ExpGenerator::addTemporary(ValueId val, MapTable *mapTable)
{
  // Add the value ID to the last map table.
  //
  MapInfo *mapInfo =
    generator->addMapInfoToThis(generator->getLastMapTable(), val, NULL);

  // Set the attributes to indicate a temporary ... ATP 0, Index 1
  //
  Attributes * mapAttr = mapInfo->getAttr();
  mapAttr->setAtp(0);
  mapAttr->setAtpIndex(1);

  // Compute length of this temporary and assign offsets to the attributes.
  // All temps are in sqlark_exploded format.
  //
  ULng32 len;
  ExpTupleDesc::TupleDataFormat tdataF = ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  ExpTupleDesc::computeOffsets(mapAttr,
			       tdataF,
			       len,
			       getTempsLength());
  addTempsLength(len);
  return mapInfo;
}

MapInfo *ExpGenerator::addPersistent(ValueId val, MapTable *mapTable)
{
  // Add the value ID to the map table.
  //
  MapInfo *mapInfo = generator->addMapInfoToThis(mapTable, val, NULL);

  // Set the attributes to indicate a persistent: Atp 1 and AtpIndex 1.
  //
  Attributes * mapAttr = mapInfo->getAttr();
  mapAttr->setAtp(1);
  mapAttr->setAtpIndex(1);

  // Compute length of this persistent and assign offsets to the attributes.
  // All persistents are in sqlark_exploded format.
  //
  ULng32 len;
  ExpTupleDesc::TupleDataFormat tdataF = ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  ExpTupleDesc::computeOffsets(mapAttr,
			       tdataF,
			       len,
			       persistentLength());
  persistentLength() = persistentLength() + len;

  // Mark the item as codeGenned.
  //
  mapInfo->codeGenerated();

  return mapInfo;
}

short ExpGenerator::genItemExpr(ItemExpr * item_expr, Attributes *** out_attr,
				Lng32 num_attrs, short gen_child)
/////////////////////////////////////////////////////////////////////////////
// Description of parameters:
// item_expr (IN) : the tree for which code is to be generated
// out_attr  (OUT): the generated attributes array is returned here. This
//                  is a pointer to an array of pointers, hence 3 *** s.
// num_attrs (IN) : number of attributes in the attribute array.
// gen_child (IN) : if -1, generate code for children.
//
// RETURN VALUE: 0, all ok. 1, code already been generated. -1, error.
/////////////////////////////////////////////////////////////////////////////
{
  MapInfo * map_info = generator->getMapInfoAsIs(item_expr->getValueId());
  if ((map_info) &&
      (map_info->isCodeGenerated()))
    return 1;

  if (!map_info)
    {
      // attempt to perform common subexpression elimination on this
      // expression
      if (cache_->expressionCanBeEliminated(item_expr))
        {
        // an equivalent expression has already been generated, and
        // this one has been replaced by it
        return 1;   // tell caller expression already generated
        }

      // no equivalent expression has been generated... continue...

      // this value has not been added to the map table. Add it
      // to the last map table as a temp. Temps are allocated
      // at atp_index = 1.
      map_info = addTemporary(item_expr->getValueId(), NULL);
    }

  Lng32 numAttrs = num_attrs;
  if (getShowplan())
    numAttrs *= 2;

  Attributes ** attr = new(wHeap()) Attributes * [numAttrs];
  for (Int32 i = 0; i < numAttrs; i++)
    {
      attr[i] = NULL;
    }

  /* assign result attributes*/
  attr[0] = map_info->getAttr();

  if (gen_child)
  {
    item_expr->codegen_and_set_attributes( generator, attr, num_attrs );
  }

  if (getShowplan())
    {
      attr[0+num_attrs] =
	new(wHeap()) ShowplanAttributes(item_expr->getValueId(),
					convertNAString(item_expr->getText(),
							generator->wHeap()));

      attr[0]->setShowplan();

      for (short i = 0; i < num_attrs - 1; i++)
	{
	  attr[i+1+num_attrs] =
	    new(wHeap())
	    ShowplanAttributes(item_expr->child(i)->getValueId(),
			       convertNAString(item_expr->child(i)->getText(),
					       generator->wHeap()));
	}
    }

  map_info->codeGenerated();

  *out_attr = attr;

  // add expression to generated cache (so if we later encounter an
  // equivalent expression, we can just use this one's generated code)
  cache_->add(item_expr);

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// methods for class ExpGeneratorCache
/////////////////////////////////////////////////////////////////////////////
ExpGeneratorCache::ExpGeneratorCache(Generator * generator)
: generator_(generator), cachedCasts_(generator->wHeap()),
  cachedBiAriths_(generator->wHeap()),
  substitutions_(generator->wHeap()),
  level_(0)
  {
  enableCommonSubexpressionElimination_ = TRUE;
  // getenv() calls are costly. avoid especially in release code.
#ifdef _DEBUG
  if (getenv("NO_COMMON_SUBEX_ELIM"))
    enableCommonSubexpressionElimination_ = FALSE;
#endif
  }

ExpGeneratorCache::~ExpGeneratorCache(void)
  {
  clear();  // to cause clean-up (e.g. avoid memory leak in substitutions_)
  }

// add an expression (which has had code generated) to the cache
void ExpGeneratorCache::add(ItemExpr * ie)
  {
  if (enableCommonSubexpressionElimination_)
    {
    if (level_ == 0) // don't cache anything that can be short-circuited
      {
      switch (ie->getOperatorType())
        {
        case ITM_CAST:
          {
          Cast * cie = (Cast *)ie;
          cachedCasts_.insert(cie);
          break;
          }
        case ITM_PLUS:
        case ITM_MINUS:
        case ITM_TIMES:
        case ITM_DIVIDE:
          {
          BiArith * bie = (BiArith *)ie;
          cachedBiAriths_.insert(bie);
          break;
          }
        default:
          break;
        }
      }
    }
  }


// find an expression which has already been generated equivalent
// to the input parameter; if none exists, returns NULL
ItemExpr * ExpGeneratorCache::findEquivalentGeneratedExpr(ItemExpr * ie)
  {
  ItemExpr * rc = 0;        // assume none exists

  if ((ie) &&
      (enableCommonSubexpressionElimination_))
    {
    switch (ie->getOperatorType())
      {
      case ITM_CAST:
        {
        for (CollIndex i = 0; (i < cachedCasts_.entries()) && (rc == 0); i++)
          {
          Cast * c = cachedCasts_[i];
          if (c->isEquivalentForCodeGeneration(ie))
            rc = c;
          }
        break;
        }
      case ITM_PLUS:
      case ITM_MINUS:
      case ITM_TIMES:
      case ITM_DIVIDE:
        {
        for (CollIndex i = 0; (i < cachedBiAriths_.entries()) && (rc == 0); i++)
          {
          BiArith * b = cachedBiAriths_[i];
          if (b->isEquivalentForCodeGeneration(ie))
            rc = b;
          }
        break;
        }
      default:
        break;
      }
    }

  return rc;
  }

NABoolean ExpGeneratorCache::expressionCanBeEliminated(ItemExpr * ie)
  {
  NABoolean rc = FALSE;  // assume expression cannot be eliminated

  // look to see if code to compute an equivalent expression has
  // already been generated
  ItemExpr * equiv_expr = findEquivalentGeneratedExpr(ie);
  if (equiv_expr)
    {
    // an equivalent expression has already been generated; replace
    // this one with that (common subexpression elimination)

      ValueId  temp1 = equiv_expr->getValueId();
      ValueId &temp2 = temp1;

      substitutions_.insert(new (generator_->wHeap())
        ExpGeneratorCacheSubstitution(ie,temp2));

      rc = TRUE;   // expression can be (and has been!) eliminated
    }
    return rc;
  }

void ExpGeneratorCache::incrementLevel(void)
  {
  level_++;
  }

void ExpGeneratorCache::decrementLevel(void)
  {
  GenAssert(level_ > 0,"Bad level in generated expression cache");
  level_--;
  }

// clear cache
void ExpGeneratorCache::clear(void)
  {
  cachedCasts_.clear();
  cachedBiAriths_.clear();

  // undo all substitutions and delete the substitution objects
  ExpGeneratorCacheSubstitution * subs = 0;
  while (substitutions_.getFirst(subs /* out */))
    {
    subs->undo();
    delete subs;
    }
  GenAssert(substitutions_.isEmpty(),"Bad undeleted substitutions");

  level_ = 0;
  }

Lng32 ExpGenerator::foldConstants(ItemExpr * inExpr, ItemExpr ** outExpr)
{
  Lng32 rc = 0;

  if (outExpr)
    *outExpr = inExpr;

  if (inExpr == NULL)
    return 0;

  if (CmpCommon::getDefault(CONSTANT_FOLDING) == DF_OFF)
    return 0;

  ValueId dummy;
  Lng32 daMark = CmpCommon::diags()->mark();
  rc = ValueIdList::evaluateExpr(dummy,
				 inExpr->getValueId(),
				 -1,
				 FALSE, // don't simplify expr
				 TRUE,  // do eval all consts
				 outExpr,
				 CmpCommon::diags());
  if ((rc < 0) ||
      (CmpCommon::diags()->getNumber() > 0))
    {
      if (outExpr)
         *outExpr = NULL;
      CmpCommon::diags()->rewind(daMark, TRUE);
      return -1;
    }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// methods for class ExpGeneratorCacheSubstitution
/////////////////////////////////////////////////////////////////////////////
ExpGeneratorCacheSubstitution::ExpGeneratorCacheSubstitution(ItemExpr * ie,
                                                             ValueId &newValueId)
: ie_(ie), oldValueId_(ie->getValueId()), newValueId_(newValueId)
  {
  // now replace the old valueId in the ItemExpr with the new
  // (the act of creating a substitution causes substitution to be performed!)
  ie->setValueId(newValueId);
  }

// undo a substitution
void ExpGeneratorCacheSubstitution::undo(void)
  {
  // the 'if' below makes this method idempotent
  if (ie_->getValueId() == newValueId_)
    ie_->setValueId(oldValueId_);
  }

// generate a null constant and type it to input type
ConstValue * ExpGenerator::generateNullConst(const NAType &type)
{
  ConstValue * nullConst = new(wHeap()) ConstValue();
  nullConst->bindNode(generator->getBindWA());
  NAType * newType = type.newCopy(wHeap());
  newType->setNullable(TRUE);
  (nullConst->getValueId()).changeType(newType);
  return nullConst;
}


