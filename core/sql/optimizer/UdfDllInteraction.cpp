/**********************************************************************
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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
**************************************************************************
*
* File:         UdfDllInteraction.cpp
* Description:  Methods for a udf RelExpr to interact with a dll
* Created:      3/01/10
* Language:     C++
*
*************************************************************************
*/

#include "UdfDllInteraction.h"
#include "NumericType.h"
#include "CharType.h"
#include "DatetimeType.h"
#include "ItemOther.h"
#include "NARoutine.h"
#include "LmError.h"


// -----------------------------------------------------------------------
// methods for class TMUDFDllInteraction
// -----------------------------------------------------------------------

TMUDFDllInteraction::TMUDFDllInteraction() :
     dllPtr_(NULL),
     createCompilerInterfaceObjectPtr_(NULL)
{}

NABoolean TMUDFDllInteraction::describeParamsAndMaxOutputs(
                                TableMappingUDF * tmudfNode,
                                BindWA * bindWA)
{
  // convert the compiler structures into something we can pass
  // to the UDF writer, to describe input parameters and input tables
  tmudr::UDRInvocationInfo *invocationInfo =
    TMUDFInternalSetup::createInvocationInfoFromRelExpr(tmudfNode,
                                                        CmpCommon::diags());
  if (!invocationInfo)
    {
      bindWA->setErrStatus();
      return FALSE;
    }

  tmudfNode->setInvocationInfo(invocationInfo);

  // get the interface class that has the code for the compiler interaction,
  // this could be a C++ class provided by the UDF writer or the base class
  // with the default implementation. Note: This could already be cached.
  tmudr::TMUDRInterface *udrInterface = tmudfNode->getUDRInterface();

  if (!udrInterface)
    {
      // try to allocate a user-provided class for compiler interaction
      if (createCompilerInterfaceObjectPtr_)
        {
          tmudr::CreateCompilerInterfaceObjectFunc factoryFunc =
            (tmudr::CreateCompilerInterfaceObjectFunc)
            createCompilerInterfaceObjectPtr_;

          udrInterface = (*factoryFunc)(invocationInfo);
        }
      else
        // just use the default implementation
        udrInterface = new tmudr::TMUDRInterface();

      tmudfNode->setUDRInterface(udrInterface);
    }

  try
    {
      udrInterface->describeParamsAndColumns(*invocationInfo);
    }
  catch (tmudr::UDRException e)
    {
      processReturnStatus(e,
                          CmpCommon::diags(), 
                          tmudfNode->getUserTableName().getExposedNameAsAnsiString().data());
      bindWA->setErrStatus();
      return FALSE;
    }

  NAHeap *outHeap = CmpCommon::statementHeap();
  NAColumnArray * modifiedParameterArray =
    new(outHeap) NAColumnArray(outHeap);

  for (int p=0; p<invocationInfo->getNumFormalParameters(); p++)
    {
      NAColumn *newParam = 
        TMUDFInternalSetup::createNAColumnFromParameterInfo(
             invocationInfo->getFormalParameterInfo(p),
             p,
             outHeap,
             CmpCommon::diags());
      if (newParam == NULL)
        {
          bindWA->setErrStatus();
          return FALSE;
        }
      modifiedParameterArray->insert(newParam);
    }

  tmudfNode->setScalarInputParams(modifiedParameterArray);

  NAColumnArray * outColArray =
    TMUDFInternalSetup::createColumnArrayFromTableInfo(
         invocationInfo->getOutputTableInfo(),
         tmudfNode,
         outHeap,
         CmpCommon::diags());
  if (outColArray == NULL)
    {
      bindWA->setErrStatus();
      return FALSE;
    }

  tmudfNode->setOutputParams(outColArray);

  return TRUE;
}

NABoolean TMUDFDllInteraction::createOutputInputColumnMap(
     TableMappingUDF * tmudfNode,
     ValueIdMap &result)
{
  tmudr::UDRInvocationInfo * invocationInfo = tmudfNode->getInvocationInfo();
  tmudr::TableInfo & outputTableInfo = invocationInfo->getOutputTableInfo();
  int numOutputColumns = outputTableInfo.getNumColumns();

  for (int oc=0; oc<numOutputColumns; oc++)
    {
      const tmudr::ProvenanceInfo &p =
        outputTableInfo.getColumn(oc).getProvenance();

      if (p.isFromInputTable())
        {
          result.addMapEntry(
               tmudfNode->getProcOutputParamsVids()[oc],
               tmudfNode->getChildInfo(p.getInputTableNum())->
                             getOutputIds()[p.getInputColumnNum()]);
        }
    }

  return TRUE;
}

NABoolean TMUDFDllInteraction::describeInputsAndOutputs(
     TableMappingUDF * tmudfNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::describeInputPartitionAndOrder(
     TableMappingUDF * tmudfNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::predicatePushDown(
     TableMappingUDF * tmudfNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::cardinality(
     TableMappingUDF * tmudfNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::constraints(
     TableMappingUDF * tmudfNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::cost(
     TableMappingUDF * tmudfNode,
     TMUDFPlanWorkSpace * pws)
{ return FALSE; }

NABoolean TMUDFDllInteraction::degreeOfParallelism(
     TableMappingUDF * tmudfNode,
     TMUDFPlanWorkSpace * pws,
     int &dop)
{ 
  tmudr::TMUDRInterface *udrInterface = tmudfNode->getUDRInterface();
  tmudr::UDRInvocationInfo *invocationInfo = tmudfNode->getInvocationInfo();
  tmudr::UDRPlanInfo *udrPlanInfo = pws->getUDRPlanInfo();

  if (udrPlanInfo == NULL)
    {
      // make a UDRPlanInfo for this PWS
      udrPlanInfo = TMUDFInternalSetup::createUDRPlanInfo();
      CmpCommon::statement()->addUDRPlanInfoToDelete(udrPlanInfo);
      pws->setUDRPlanInfo(udrPlanInfo);
    }

  try
    {
      udrInterface->describeDesiredDegreeOfParallelism(*invocationInfo,
                                                       *udrPlanInfo);
    }
  catch (tmudr::UDRException e)
    {
      processReturnStatus(e,
                          CmpCommon::diags(), 
                          tmudfNode->getUserTableName().getExposedNameAsAnsiString().data());
      return FALSE;
    }

  dop = udrPlanInfo->getDesiredDegreeOfParallelism();
  
  return TRUE;
}

NABoolean TMUDFDllInteraction::generateInputPartitionAndOrder(
     TableMappingUDF * tmudfNode,
     TMUDFPlanWorkSpace * pws)
{ return FALSE; }

NABoolean TMUDFDllInteraction::describeOutputOrder(
     TableMappingUDF * tmudfNode,
     TMUDFPlanWorkSpace * pws)
{ return FALSE; }


void TMUDFDllInteraction::setFunctionPtrs(
     const NAString& entryName)
{
  if (dllPtr_ == NULL)
    return ;

  // create the mangled C++ entry point name
  // Todo: should this call some ABI interface instead???
  NAString f;
  char nameLenString[10];
  f = entryName + "_CreateCompilerInterfaceObject";
  snprintf(nameLenString, 10,"Z%d",(int) f.length());
  // This did not work for me to get the mangled C++ name, not sure why
  // f.prepend(nameLenString);
  // f += "PKN5tmudr17UDRInvocationInfoE";
  createCompilerInterfaceObjectPtr_ = getRoutinePtr(dllPtr_, f.data());
}

void TMUDFDllInteraction::processReturnStatus(const tmudr::UDRException &e, 
                                              ComDiagsArea *diags,
                                              const char* routineName)
{
  int iSQLState = e.getSQLState();
  char sqlState[6];

  snprintf(sqlState,6,"%5d", iSQLState);

  if (iSQLState > 38000 && iSQLState < 39000)
    {
      *diags << DgSqlCode(-LME_CUSTOM_ERROR)
             << DgString0(e.getText().data())
             << DgString1(sqlState);
      *diags << DgCustomSQLState(sqlState);
    }
  else
    {
      *diags << DgSqlCode(-LME_UDF_ERROR)
             << DgString0(routineName)
             << DgString1(sqlState)
             << DgString2(e.getText().data());
    }
}

tmudr::UDRInvocationInfo *TMUDFInternalSetup::createInvocationInfoFromRelExpr(
     TableMappingUDF * tmudfNode,
     ComDiagsArea *diags)
{
  tmudr::UDRInvocationInfo *result = new tmudr::UDRInvocationInfo();
  NABoolean success = TRUE;

  result->version_ = result->getCurrentVersion();
  result->name_ = tmudfNode->getRoutineName().getQualifiedNameAsAnsiString().data();
  result->numTableInputs_ = tmudfNode->getArity();
  // initialize the function type with the most general
  // type there is, SETFUNC
  result->funcType_ = tmudr::UDRInvocationInfo::SETFUNC;

  // set info for the formal scalar input parameters
  const NAColumnArray &formalParams = tmudfNode->getNARoutine()->getInParams();
  for (CollIndex c=0; c<formalParams.entries(); c++)
    {
      tmudr::ParameterInfo *pi =
        TMUDFInternalSetup::createParameterInfoFromNAColumn(
             formalParams[c],
             diags);
      if (!pi)
        return NULL;

      result->formalParameterInfo_.push_back(pi);
    }

  // set info for the actual scalar input parameters
  const ValueIdList &actualParamVids = tmudfNode->getProcInputParamsVids();
  for (CollIndex i=0; i < actualParamVids.entries(); i++)
    {
      NABoolean success = TRUE;
      NABoolean negate;
      const NAType &paramType = actualParamVids[i].getType();
      NABuiltInTypeEnum typeClass = paramType.getTypeQualifier();
      ConstValue *constVal = actualParamVids[i].getItemExpr()->castToConstValue(negate);
      std::string paramName;
      char paramNum[10];

      if (i < result->getNumFormalParameters())
        paramName = result->getFormalParameterInfo(i).getParameterName();

      tmudr::TypeInfo ti;
      success = TMUDFInternalSetup::setTypeInfoFromNAType(
           ti,
           &paramType,
           diags);
      if (!success)
        return NULL;

      tmudr::ParameterInfo *pi = new tmudr::ParameterInfo(
           paramName.data(),
           ti);
      
      // if the actual parameter is a constant value, pass its data
      // to the UDF
      if (constVal)
        {
          if (typeClass == NA_NUMERIC_TYPE)
            {
              if (constVal->canGetExactNumericValue())
                {
                  Lng32 scale;
                  pi->isAvailable_ = tmudr::ParameterInfo::EXACT_NUMERIC_VALUE;
                  pi->exactNumericValue_ = constVal->getExactNumericValue(scale);
                  // leave it up to the UDF writer to evaluate the scale
                }
              else if (constVal->canGetApproximateNumericValue())
                {
                  pi->isAvailable_ = tmudr::ParameterInfo::APPROX_NUMERIC_VALUE;
                  pi->approxNumericValue_ = constVal->getApproximateNumericValue();
                }
            }
          else if (typeClass == NA_CHARACTER_TYPE)
            {
              // return the actual string value, no quoting,
              // no character set conversion (for now)
              pi->isAvailable_ = tmudr::ParameterInfo::STRING_VALUE;
              pi->stringValue_ =
                std::string(reinterpret_cast<const char *>(constVal->getConstValue()),
                            paramType.getNominalSize());
            }
          else if (typeClass == NA_DATETIME_TYPE ||
                   typeClass == NA_INTERVAL_TYPE)
            {
              // convert the value into a string
              pi->isAvailable_ = tmudr::ParameterInfo::STRING_VALUE;
              NAString dateIntVal = constVal->getConstStr();
              pi->stringValue_ = std::string(dateIntVal.data(),
                                             dateIntVal.length());
            }
        }
      result->actualParameterInfo_.push_back(pi);
    }

  // set up info for the input (child) tables
  for (int c=0; c<result->numTableInputs_; c++)
    {
      TableMappingUDFChildInfo *childInfo = tmudfNode->getChildInfo(c);
      const NAColumnArray &childColArray = childInfo->getInputTabCols();

      success = TMUDFInternalSetup::setTableInfoFromNAColumnArray(
           result->inputTableInfo_[c],
           &childColArray,
           diags);
      if (!success)
        return NULL;

      // Todo: Set query partitioning and ordering for this child table
      // (numRows and constraints will be set later)
    }

  // initialize output columns with the columns declared in the metadata
  // UDF compiler interface can change this
  success = TMUDFInternalSetup::setTableInfoFromNAColumnArray(
       result->outputTableInfo_,
       &(tmudfNode->getNARoutine()->getOutParams()),
       diags);
  if (!success)
    return NULL;

  // predicates_ is initially empty, nothing to do

  // Since we allocated all our structures from the system heap,
  // make sure that someone deletes it after compilation of the
  // current statement completes
  CmpCommon::statement()->addUDRInvocationInfoToDelete(result);

  return result;
}

NABoolean TMUDFInternalSetup::setTypeInfoFromNAType(
     tmudr::TypeInfo &tgt,
     const NAType *src,
     ComDiagsArea *diags)
{
  // follows code in TMUDFDllInteraction::setParamInfo() - approximately
  NABoolean result = TRUE;

  tgt.scale_        =
  tgt.precision_    =
  tgt.collation_    = tmudr::TypeInfo::SYSTEM_COLLATION;
  tgt.nullable_     = (src->supportsSQLnullLogical() != FALSE);
  tgt.charset_      = tmudr::TypeInfo::UNDEFINED_CHARSET;
  tgt.intervalCode_ = tmudr::TypeInfo::UNDEFINED_INTERVAL_CODE;
  tgt.length_       = src->getNominalSize();
  
  // sqlType_, cType_, scale_, charset_, intervalCode_, precision_, collation_
  // are somewhat dependent on each other and are set in the following
  switch (src->getTypeQualifier())
    {
    case NA_NUMERIC_TYPE:
      {
        const NumericType *numType = static_cast<const NumericType *>(src);
        NABoolean isUnsigned = numType->isUnsigned();
        NABoolean isDecimal = numType->isDecimal();
        NABoolean isDecimalPrecision = numType->decimalPrecision();
        NABoolean isExact = numType->isExact();

        tgt.scale_ = src->getScale();

        if (isDecimalPrecision)
          {
            // only decimal precision is stored for SQL type NUMERIC
            tgt.sqlType_ = tmudr::TypeInfo::NUMERIC;
            // C type will be set below, same as non-decimal exact numeric
            tgt.precision_ = src->getPrecision();
          }

        if (isDecimal)
          {
            // decimals are represented as strings in the UDF (Todo: ???)
            // NOTE: For signed LSE decimals, the most significant
            //       bit in the first digit is the sign
            if (isUnsigned)
              tgt.sqlType_ = tmudr::TypeInfo::DECIMAL_UNSIGNED;
            else
              tgt.sqlType_ = tmudr::TypeInfo::DECIMAL_LSE;
            tgt.cType_ = tmudr::TypeInfo::STRING;
          }
        else if (isExact)
          switch (tgt.length_)
            {
              // SMALLINT, INT, LARGEINT, NUMERIC, signed and unsigned
            case 2:
              if (isUnsigned)
                {
                  if (!isDecimalPrecision)
                    tgt.sqlType_ = tmudr::TypeInfo::SMALLINT_UNSIGNED;
                  tgt.cType_   = tmudr::TypeInfo::UINT16;
                }
              else
                {
                  if (!isDecimalPrecision)
                    tgt.sqlType_ = tmudr::TypeInfo::SMALLINT;
                  tgt.cType_   = tmudr::TypeInfo::INT16;
                }
              break;
              
            case 4:
              if (isUnsigned)
                {
                  if (!isDecimalPrecision)
                    tgt.sqlType_ = tmudr::TypeInfo::INT_UNSIGNED;
                  tgt.cType_   = tmudr::TypeInfo::UINT32;
                }
              else
                {
                  if (!isDecimalPrecision)
                    tgt.sqlType_ = tmudr::TypeInfo::INT;
                  tgt.cType_   = tmudr::TypeInfo::INT32;
                }
              break;

            case 8:
              CMPASSERT(!isUnsigned);
              if (!isDecimalPrecision)
                tgt.sqlType_ = tmudr::TypeInfo::LARGEINT;
              tgt.cType_   = tmudr::TypeInfo::INT64;
              break;

            default:
              *diags << DgSqlCode(11151)
                     << DgString0("type")
                     << DgString1(src->getTypeSQLname())
                     << DgString2("unsupported length");
              result = FALSE;
            }
        else // inexact numeric
          if (tgt.length_ == 4)
            {
              tgt.sqlType_ = tmudr::TypeInfo::REAL;
              tgt.cType_   = tmudr::TypeInfo::FLOAT;
            }
          else
            {
              // note that there is no SQL FLOAT in UDFs, SQL FLOAT
              // gets mapped to REAL or DOUBLE PRECISION
              CMPASSERT(tgt.length_ == 8);
              tgt.sqlType_ = tmudr::TypeInfo::DOUBLE_PRECISION;
              tgt.cType_   = tmudr::TypeInfo::DOUBLE;
            }
      }
      break;

    case NA_CHARACTER_TYPE:
      {
        CharInfo::CharSet cs = (CharInfo::CharSet) src->getScaleOrCharset();

        if (src->isVaryingLen())
          tgt.sqlType_ = tmudr::TypeInfo::VARCHAR;
        else
          tgt.sqlType_ = tmudr::TypeInfo::CHAR;

        // character set
        switch (cs)
          {
          case CharInfo::ISO88591:
            tgt.charset_ = tmudr::TypeInfo::CHARSET_ISO88591;
            tgt.cType_   = tmudr::TypeInfo::STRING;
            break;

          case CharInfo::UTF8:
            tgt.charset_ = tmudr::TypeInfo::CHARSET_UTF8;
            tgt.cType_   = tmudr::TypeInfo::STRING;
            break;

          case CharInfo::UCS2:
            tgt.charset_ = tmudr::TypeInfo::CHARSET_UCS2;
            tgt.cType_ = tmudr::TypeInfo::U16STRING;
            break;

          default:
            *diags << DgSqlCode(11151)
                   << DgString0("character set")
                   << DgString1(CharInfo::getCharSetName(
                                     (CharInfo::CharSet) src->getScaleOrCharset()))
                   << DgString2("unsupported character set");
            result = FALSE;
          }

        // collation stays at 0 for now
      }
      break;

    case NA_DATETIME_TYPE:
      {
        const DatetimeType *dType = static_cast<const DatetimeType *>(src);

        // fraction precision for time/timestamp
        tgt.precision_ = dType->getFractionPrecision();

        switch (dType->getSubtype())
          {
          case DatetimeType::SUBTYPE_SQLDate:
            tgt.sqlType_ = tmudr::TypeInfo::DATE;
            break;
          case DatetimeType::SUBTYPE_SQLTime:
            tgt.sqlType_ = tmudr::TypeInfo::TIME;
            break;
          case DatetimeType::SUBTYPE_SQLTimestamp:
            tgt.sqlType_ = tmudr::TypeInfo::TIMESTAMP;
            break;
          default:
            *diags << DgSqlCode(11151)
                   << DgString0("type")
                   << DgString1(src->getTypeSQLname())
                   << DgString2("unsupported datetime subtype");
            result = FALSE;
          }

        // Todo, translate to time_t C type or to string??
        tgt.cType_ = tmudr::TypeInfo::STRING;
      }
      break;

    case NA_INTERVAL_TYPE:
      {
        const IntervalType *iType = static_cast<const IntervalType *>(src);

        tgt.sqlType_ = tmudr::TypeInfo::INTERVAL;
        tgt.precision_ = iType->getLeadingPrecision();
        tgt.scale_ = iType->getFractionPrecision();

        switch (src->getFSDatatype())
          {
          case REC_INT_YEAR:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_YEAR;
            break;
          case REC_INT_MONTH:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_MONTH;
            break;
          case REC_INT_YEAR_MONTH:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_YEAR_MONTH;
            break;
          case REC_INT_DAY:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_DAY;
            break;
          case REC_INT_HOUR:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_HOUR;
            break;
          case REC_INT_DAY_HOUR:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_DAY_HOUR;
            break;
          case REC_INT_MINUTE:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_MINUTE;
            break;
          case REC_INT_HOUR_MINUTE:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_HOUR_MINUTE;
            break;
          case REC_INT_DAY_MINUTE:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_DAY_MINUTE;
            break;
          case REC_INT_SECOND:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_SECOND;
            break;
          case REC_INT_MINUTE_SECOND:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_MINUTE_SECOND;
            break;
          case REC_INT_HOUR_SECOND:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_HOUR_SECOND;
            break;
          case REC_INT_DAY_SECOND:
            tgt.intervalCode_ = tmudr::TypeInfo::INTERVAL_DAY_SECOND;
            break;
          default:
            *diags << DgSqlCode(11151)
                   << DgString0("type")
                   << DgString1("interval")
                   << DgString2("unsupported interval subtype");
            result = FALSE;
          }

        // Todo, convert to special data structure or just an integer?
        tgt.cType_ = tmudr::TypeInfo::INT64;

      }
      break;

    default:
      *diags << DgSqlCode(11151)
             << DgString0("type")
             << DgString1(src->getTypeSQLname())
             << DgString2("unsupported type class");
      result = FALSE;
    }

  return result;
}

tmudr::ParameterInfo *TMUDFInternalSetup::createParameterInfoFromNAColumn(
     NAColumn *src,
     ComDiagsArea *diags)
{
  tmudr::TypeInfo ti;

  if (!TMUDFInternalSetup::setTypeInfoFromNAType(
       ti,
       src->getType(),
       diags))
    return NULL;

  tmudr::ParameterInfo *result = new tmudr::ParameterInfo(
       src->getColName().data(),
       ti);

  return result;
}

tmudr::ColumnInfo * TMUDFInternalSetup::createColumnInfoFromNAColumn(
     const NAColumn *src,
     ComDiagsArea *diags)
{
  tmudr::TypeInfo ti;

  if (!TMUDFInternalSetup::setTypeInfoFromNAType(
       ti,
       src->getType(),
       diags))
    return NULL;

  return new tmudr::ColumnInfo(
       src->getColName(),
       ti);
}

NABoolean TMUDFInternalSetup::setTableInfoFromNAColumnArray(
     tmudr::TableInfo &tgt,
     const NAColumnArray *src,
     ComDiagsArea *diags)
{
  for (CollIndex c=0; c<src->entries(); c++)
    {
      const NAColumn *nac = (*src)[c];
      tmudr::ColumnInfo *ci =
        TMUDFInternalSetup::createColumnInfoFromNAColumn(
             nac,
             diags);
      if (ci == NULL)
        return FALSE;

      tgt.columns_.push_back(ci);
    }

  return TRUE;
}

NAType *TMUDFInternalSetup::createNATypeFromTypeInfo(
     const tmudr::TypeInfo &src,
     int colNumForDiags,
     NAHeap *heap,
     ComDiagsArea *diags)
{
  NAType *result = NULL;
  tmudr::TypeInfo::SQLTYPE_CODE typeCode = src.getSQLType();

  switch (typeCode)
    {
    case tmudr::TypeInfo::SMALLINT:
    case tmudr::TypeInfo::SMALLINT_UNSIGNED:
      result = new(heap)
        SQLSmall((typeCode == tmudr::TypeInfo::SMALLINT),
                 src.getIsNullable(),
                 heap);
      break;

    case tmudr::TypeInfo::INT:
    case tmudr::TypeInfo::INT_UNSIGNED:
      result = new(heap)
        SQLInt((typeCode == tmudr::TypeInfo::INT),
               src.getIsNullable(),
               heap);
      break;

    case tmudr::TypeInfo::LARGEINT:
      result = new(heap) SQLLargeInt(TRUE,
                                     src.getIsNullable(),
                                     heap);
      break;

    case tmudr::TypeInfo::NUMERIC:
    case tmudr::TypeInfo::NUMERIC_UNSIGNED:
      {
        int storageSize = getBinaryStorageSize(src.getPrecision());
        // if the storage size is specified, it must match
        if (src.getLength() > 0 &&
            src.getLength() != storageSize)
          {
            *diags << DgSqlCode(-11152)
                   << DgInt0(typeCode)
                   << DgInt1(colNumForDiags)
                   << DgString0("Incorrect storage size");
          }
        else
          result = new(heap) 
            SQLNumeric(storageSize,
                       src.getPrecision(),
                       src.getScale(),
                       (typeCode == tmudr::TypeInfo::NUMERIC),
                       src.getIsNullable(),
                       heap);
      }
      break;

    case tmudr::TypeInfo::DECIMAL_LSE:
    case tmudr::TypeInfo::DECIMAL_UNSIGNED:
      result = new(heap)
        SQLDecimal(src.getPrecision(),
                   src.getScale(),
                   (typeCode == tmudr::TypeInfo::DECIMAL_LSE),
                   src.getIsNullable(),
                   heap);
      break;

    case tmudr::TypeInfo::REAL:
      result = new(heap) SQLReal(src.getIsNullable(),
                                 heap);
      break;

    case tmudr::TypeInfo::DOUBLE_PRECISION:
      result = new(heap) SQLDoublePrecision(src.getIsNullable(),
                                            heap);
      break;

    case tmudr::TypeInfo::CHAR:
    case tmudr::TypeInfo::VARCHAR:
      {
        CharInfo::CharSet cs = CharInfo::UnknownCharSet;

        switch (src.getCharset())
          {
          case tmudr::TypeInfo::CHARSET_ISO88591:
            cs = CharInfo::ISO88591;
            break;
          case tmudr::TypeInfo::CHARSET_UTF8:
            cs = CharInfo::UTF8;
            break;
          case tmudr::TypeInfo::CHARSET_UCS2:
            cs = CharInfo::UCS2;
            break;
          default:
            *diags << DgSqlCode(-11152)
                   << DgInt0(src.getSQLType())
                   << DgInt1(colNumForDiags)
                   << DgString0("Invalid charset");
          }

        if (cs != CharInfo::UnknownCharSet)
          {
            // assume that any UTF8 strings are
            // limited by their byte length, not
            // the number of UTF8 characters
            CharLenInfo lenInfo(0,src.getLength());

            if (typeCode == tmudr::TypeInfo::CHAR)
              result = new(heap)
                SQLChar(lenInfo,
                        src.getIsNullable(),
                        FALSE,
                        FALSE,
                        FALSE,
                        cs);
            else
              result = new(heap)
                SQLVarChar(lenInfo,
                           src.getIsNullable(),
                           FALSE,
                           FALSE,
                           cs);
          }
      }
      break;

    case tmudr::TypeInfo::DATE:
      result = new(heap) SQLDate(src.getIsNullable(),
                                 heap);
      break;

    case tmudr::TypeInfo::TIME:
      result = new(heap) SQLTime(src.getIsNullable(),
                                 src.getScale(),
                                 heap);
      break;

    case tmudr::TypeInfo::TIMESTAMP:
      result = new(heap) SQLTimestamp(src.getIsNullable(),
                                      src.getScale(),
                                      heap);
      break;

    case tmudr::TypeInfo::INTERVAL:
      {
        rec_datetime_field startField = REC_DATE_UNKNOWN;
        rec_datetime_field endField = REC_DATE_UNKNOWN;

        switch (src.getIntervalCode())
          {
          case tmudr::TypeInfo::INTERVAL_YEAR:
            startField =
              endField = REC_DATE_YEAR;
            break;

          case tmudr::TypeInfo::INTERVAL_MONTH:
            startField =
              endField = REC_DATE_MONTH;
            break;

          case tmudr::TypeInfo::INTERVAL_DAY:
            startField =
              endField = REC_DATE_DAY;
            break;

          case tmudr::TypeInfo::INTERVAL_HOUR:
            startField =
              endField = REC_DATE_HOUR;
            break;

          case tmudr::TypeInfo::INTERVAL_MINUTE:
            startField =
              endField = REC_DATE_MINUTE;
            break;

          case tmudr::TypeInfo::INTERVAL_SECOND:
            startField =
              endField = REC_DATE_SECOND;
            break;

          case tmudr::TypeInfo::INTERVAL_YEAR_MONTH:
            startField = REC_DATE_YEAR;
            endField = REC_DATE_MONTH;
            break;

          case tmudr::TypeInfo::INTERVAL_DAY_HOUR:
            startField = REC_DATE_DAY;
            endField = REC_DATE_HOUR;
            break;

          case tmudr::TypeInfo::INTERVAL_DAY_MINUTE:
            startField = REC_DATE_DAY;
            endField = REC_DATE_MINUTE;
            break;

          case tmudr::TypeInfo::INTERVAL_DAY_SECOND:
            startField = REC_DATE_DAY;
            endField = REC_DATE_SECOND;
            break;

          case tmudr::TypeInfo::INTERVAL_HOUR_MINUTE:
            startField = REC_DATE_HOUR;
            endField = REC_DATE_MINUTE;
            break;

          case tmudr::TypeInfo::INTERVAL_HOUR_SECOND:
            startField = REC_DATE_HOUR;
            endField = REC_DATE_SECOND;
            break;

          case tmudr::TypeInfo::INTERVAL_MINUTE_SECOND:
            startField = REC_DATE_MINUTE;
            endField = REC_DATE_SECOND;
            break;

          default:
            *diags << DgSqlCode(-11152)
                   << DgInt0(src.getSQLType())
                   << DgInt1(colNumForDiags)
                   << DgString0("Invalid interval start/end fields");
          }

        if (startField != REC_DATE_UNKNOWN)
          {
            result = new(heap) SQLInterval(src.getIsNullable(),
                                           startField,
                                           src.getPrecision(),
                                           endField,
                                           src.getScale(),
                                           heap);
            if (!static_cast<SQLInterval *>(result)->checkValid(diags))
              {
                *diags << DgSqlCode(-11152)
                       << DgInt0(src.getSQLType())
                       << DgInt1(colNumForDiags)
                       << DgString0("See previous error");
                result = NULL;
              }
          }
      }
      break;

    default:
      *diags << DgSqlCode(-11152)
             << DgInt0(src.getSQLType())
             << DgInt1(colNumForDiags)
             << DgString0("Invalid SQL Type code");
      break;
    }

  return result;
}

NAColumn *TMUDFInternalSetup::createNAColumnFromParameterInfo(
     const tmudr::ParameterInfo &src,
     int position,
     NAHeap *heap,
     ComDiagsArea *diags)
{
  NAType *newColType = createNATypeFromTypeInfo(src.getType(),
                                                position,
                                                heap,
                                                diags);
  if (newColType == NULL)
    return NULL;

  return new(heap) NAColumn(
       src.getParameterName().data(),
       position,
       newColType,
       heap);
}

NAColumnArray * TMUDFInternalSetup::createColumnArrayFromTableInfo(
     const tmudr::TableInfo &tableInfo,
     TableMappingUDF * tmudfNode,
     NAHeap *heap,
     ComDiagsArea *diags)
{
  NAColumnArray *result = new(heap) NAColumnArray(heap);
  int numColumns = tableInfo.getNumColumns();

  for (int i=0; i<numColumns; i++)
    {
      const tmudr::ColumnInfo &colInfo = tableInfo.getColumn(i);
      NAColumn *newCol = NULL;
      const tmudr::ProvenanceInfo &provenance = colInfo.getProvenance();

      if (provenance.isFromInputTable())
        {
          // the output column is passed through from an input
          // column

          const NAColumn *ic =
            tmudfNode->getChildInfo(
                 provenance.getInputTableNum())->getInputTabCols().getColumn(
                      provenance.getInputColumnNum());
          const char *newColName = colInfo.getColName().data();

          // unless specified, use the input column name
          if (!newColName || strlen(newColName) == 0)
            newColName = ic->getColName();

          // use type and heading from the input column when
          // creating the descriptor of the output column
          newCol = new(heap) NAColumn(
               newColName,
               i,
               ic->getType()->newCopy(heap),
               heap,
               0,
               NULL,
               USER_COLUMN,
               COM_NO_DEFAULT,
               NULL,
               const_cast<char *>(ic->getHeading()));
        }
      else
        {
          char defaultName[30];
          const char *usedColName;
          NAType *newColType = createNATypeFromTypeInfo(colInfo.getType(),
                                                        i,
                                                        heap,
                                                        diags);

          if (newColType == NULL)
            return NULL;

          usedColName = colInfo.getColName().data();

          if (usedColName[0] == 0)
            {
              // no name specified by UDF writer, make one up
              snprintf(defaultName, 30, "output_%d", i);
              usedColName = defaultName;
            }

          newCol = new(heap) NAColumn(
               usedColName,
               i,
               newColType,
               heap);
        }

      result->insert(newCol);
    }

  return result;
}

tmudr::UDRPlanInfo *TMUDFInternalSetup::createUDRPlanInfo()
{
  return new tmudr::UDRPlanInfo();
}

void TMUDFInternalSetup::deleteUDRInvocationInfo(tmudr::UDRInvocationInfo *toDelete)
{
  // sorry, I'm your friend, but I'll have to terminate you now
  delete toDelete;
}

void TMUDFInternalSetup::deleteUDRPlanInfo(tmudr::UDRPlanInfo *toDelete)
{
  // sorry, I'm your friend, but I'll have to terminate you now
  delete toDelete;
}

// also source in the methods defined in sqludr.cpp
#include "sqludr.cpp"
