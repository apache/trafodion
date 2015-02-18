/**********************************************************************
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
#include "SchemaDB.h"
#include "exp_attrs.h"
#include "LmError.h"


// -----------------------------------------------------------------------
// methods for class TMUDFDllInteraction
// -----------------------------------------------------------------------

TMUDFDllInteraction::TMUDFDllInteraction() :
     dllPtr_(NULL),
     createInterfaceObjectPtr_(NULL)
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
  tmudr::UDRInterface *udrInterface = tmudfNode->getUDRInterface();

#ifndef NDEBUG
  if (invocationInfo->getDebugFlags() & tmudr::UDRInvocationInfo::PRINT_INVOCATION_INFO_INITIAL)
    invocationInfo->print();
#endif

  if (!udrInterface)
    {
      // try to allocate a user-provided class for compiler interaction
      if (createInterfaceObjectPtr_)
        {
          tmudr::CreateInterfaceObjectFunc factoryFunc =
            (tmudr::CreateInterfaceObjectFunc)
            createInterfaceObjectPtr_;

          udrInterface = (*factoryFunc)(invocationInfo);
        }
      else
        // just use the default implementation
        udrInterface = new tmudr::UDRInterface();

      tmudfNode->setUDRInterface(udrInterface);
    }

  try
    {
#ifndef NDEBUG
      if (invocationInfo->getDebugFlags() & tmudr::UDRInvocationInfo::DEBUG_INITIAL_COMPILE_TIME_LOOP)
        udrInterface->debugLoop();
#endif

      TMUDFInternalSetup::setCallPhase(
           invocationInfo,
           tmudr::COMPILER_INITIAL_CALL);
      udrInterface->describeParamsAndColumns(*invocationInfo);
      TMUDFInternalSetup::resetCallPhase(invocationInfo);
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
        TMUDFInternalSetup::createNAColumnFromColumnInfo(
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
  tmudr::UDRInterface *udrInterface = tmudfNode->getUDRInterface();
  tmudr::UDRInvocationInfo *invocationInfo = tmudfNode->getInvocationInfo();
  tmudr::UDRPlanInfo *udrPlanInfo = pws->getUDRPlanInfo();

  if (udrPlanInfo == NULL)
    {
      // make a UDRPlanInfo for this PWS
      udrPlanInfo = TMUDFInternalSetup::createUDRPlanInfo(invocationInfo);
      CmpCommon::statement()->addUDRPlanInfoToDelete(udrPlanInfo);
      pws->setUDRPlanInfo(udrPlanInfo);
    }

  try
    {
      TMUDFInternalSetup::setCallPhase(
           invocationInfo,
           tmudr::COMPILER_DOP_CALL);
      udrInterface->describeDesiredDegreeOfParallelism(*invocationInfo,
                                                       *udrPlanInfo);
      TMUDFInternalSetup::resetCallPhase(invocationInfo);
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


NABoolean TMUDFDllInteraction::finalizePlan(
     TableMappingUDF * tmudfNode,
     tmudr::UDRPlanInfo *planInfo)
{ 
  tmudr::UDRInterface *udrInterface = tmudfNode->getUDRInterface();
  tmudr::UDRInvocationInfo *invocationInfo = tmudfNode->getInvocationInfo();

  try
    {
      TMUDFInternalSetup::setCallPhase(
           invocationInfo,
           tmudr::COMPILER_COMPLETION_CALL);
      udrInterface->completeDescription(*invocationInfo, *planInfo);
      TMUDFInternalSetup::resetCallPhase(invocationInfo);
    }
  catch (tmudr::UDRException e)
    {
      processReturnStatus(e,
                          CmpCommon::diags(), 
                          tmudfNode->getUserTableName().getExposedNameAsAnsiString().data());
      return FALSE;
    }

#ifndef NDEBUG
  if (invocationInfo->getDebugFlags() & tmudr::UDRInvocationInfo::PRINT_INVOCATION_INFO_END_COMPILE)
    {
      invocationInfo->print();
      planInfo->print();
    }
#endif

  return TRUE;
}

void TMUDFDllInteraction::setFunctionPtrs(
     const NAString& entryName)
{
  if (dllPtr_ == NULL)
    return ;

  createInterfaceObjectPtr_ = getRoutinePtr(dllPtr_, entryName.data());
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

  result->name_ = tmudfNode->getRoutineName().getQualifiedNameAsAnsiString().data();
  result->numTableInputs_ = tmudfNode->getArity();

  result->debugFlags_ = static_cast<int>(ActiveSchemaDB()->getDefaults().getAsLong(UDR_DEBUG_FLAGS));
  // initialize the function type with the most general
  // type there is, SETFUNC
  result->funcType_ = tmudr::UDRInvocationInfo::SETFUNC;

  // set info for the formal scalar input parameters
  const NAColumnArray &formalParams = tmudfNode->getNARoutine()->getInParams();
  for (CollIndex c=0; c<formalParams.entries(); c++)
    {
      tmudr::ColumnInfo *pi =
        TMUDFInternalSetup::createColumnInfoFromNAColumn(
             formalParams[c],
             diags);
      if (!pi)
        return NULL;

      result->addFormalParameter(*pi);
    }

  // set info for the actual scalar input parameters
  const ValueIdList &actualParamVids = tmudfNode->getProcInputParamsVids();
  int constBufferLength = 0;
  char *constBuffer = NULL;
  int nextOffset = 0;

  for (CollIndex j=0; j < actualParamVids.entries(); j++)
    {
      NABoolean negate;
      ConstValue *constVal = actualParamVids[j].getItemExpr()->castToConstValue(negate);

      if (constVal)
        {
          constBufferLength += constVal->getType()->getTotalSize();
          // round up to the next multiple of 8
          // do this the same way as with nextOffset below
          constBufferLength = ((constBufferLength + 7) / 8) * 8;
        }
    }

  // allocate a buffer to hold the constant values in binary form
  // add pass this buffer to the tmudr::ParameterInfoList object
  // that holds the actual parameters (we are a friend and can
  // therefore access the data members directly)
  if (constBufferLength > 0)
    constBuffer = new char[constBufferLength];
  result->nonConstActualParameters().setConstBuffer(constBufferLength,
                                                    constBuffer);

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
        paramName = result->getFormalParameterInfo(i).getColName();

      tmudr::TypeInfo ti;
      success = TMUDFInternalSetup::setTypeInfoFromNAType(
           ti,
           &paramType,
           diags);
      if (!success)
        return NULL;

      tmudr::ColumnInfo pi = tmudr::ColumnInfo(
           paramName.data(),
           ti);

      result->nonConstActualParameters().addColumn(pi);
      
      // if the actual parameter is a constant value, pass its data
      // to the UDF
      if (constVal)
        {
          int totalSize     = constVal->getType()->getTotalSize();
          int nullIndOffset = -1;
          int vcLenOffset   = -1;
          int dataOffset    = -1;

          memcpy(constBuffer + nextOffset, constVal->getConstValue(), totalSize);

          constVal->getOffsetsInBuffer(nullIndOffset, vcLenOffset, dataOffset);
          result->nonConstActualParameters().getColumn(i).getType().setOffsets(
               (nullIndOffset > 0 ? nextOffset + nullIndOffset : -1),
               (vcLenOffset   > 0 ? nextOffset + vcLenOffset   : -1),
               nextOffset + dataOffset);

          nextOffset += totalSize;
          // round up to the next multiple of 8
          // do this the same way as with constBufferLength above
          nextOffset = ((nextOffset + 7) / 8) * 8;

        }
    }
  CMPASSERT(nextOffset == constBufferLength);

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

  tgt.d_.scale_        = 0;
  tgt.d_.precision_    = 0;
  tgt.d_.collation_    = tmudr::TypeInfo::SYSTEM_COLLATION;
  tgt.d_.nullable_     = ((src->supportsSQLnullLogical() != FALSE) ? 1 : 0);
  tgt.d_.charset_      = tmudr::TypeInfo::UNDEFINED_CHARSET;
  tgt.d_.intervalCode_ = tmudr::TypeInfo::UNDEFINED_INTERVAL_CODE;
  tgt.d_.length_       = src->getNominalSize();
  
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

        tgt.d_.scale_ = src->getScale();

        if (isDecimalPrecision)
          {
            // only decimal precision is stored for SQL type NUMERIC
            tgt.d_.sqlType_ = tmudr::TypeInfo::NUMERIC;
            // C type will be set below, same as non-decimal exact numeric
            tgt.d_.precision_ = src->getPrecision();
          }

        if (isDecimal)
          {
            // decimals are represented as strings in the UDF (Todo: ???)
            // NOTE: For signed LSE decimals, the most significant
            //       bit in the first digit is the sign
            if (isUnsigned)
              tgt.d_.sqlType_ = tmudr::TypeInfo::DECIMAL_UNSIGNED;
            else
              tgt.d_.sqlType_ = tmudr::TypeInfo::DECIMAL_LSE;
            tgt.d_.cType_ = tmudr::TypeInfo::STRING;
          }
        else if (isExact)
          switch (tgt.d_.length_)
            {
              // SMALLINT, INT, LARGEINT, NUMERIC, signed and unsigned
            case 2:
              if (isUnsigned)
                {
                  if (!isDecimalPrecision)
                    tgt.d_.sqlType_ = tmudr::TypeInfo::SMALLINT_UNSIGNED;
                  tgt.d_.cType_   = tmudr::TypeInfo::UINT16;
                }
              else
                {
                  if (!isDecimalPrecision)
                    tgt.d_.sqlType_ = tmudr::TypeInfo::SMALLINT;
                  tgt.d_.cType_   = tmudr::TypeInfo::INT16;
                }
              break;
              
            case 4:
              if (isUnsigned)
                {
                  if (!isDecimalPrecision)
                    tgt.d_.sqlType_ = tmudr::TypeInfo::INT_UNSIGNED;
                  tgt.d_.cType_   = tmudr::TypeInfo::UINT32;
                }
              else
                {
                  if (!isDecimalPrecision)
                    tgt.d_.sqlType_ = tmudr::TypeInfo::INT;
                  tgt.d_.cType_   = tmudr::TypeInfo::INT32;
                }
              break;

            case 8:
              CMPASSERT(!isUnsigned);
              if (!isDecimalPrecision)
                tgt.d_.sqlType_ = tmudr::TypeInfo::LARGEINT;
              tgt.d_.cType_   = tmudr::TypeInfo::INT64;
              break;

            default:
              *diags << DgSqlCode(11151)
                     << DgString0("type")
                     << DgString1(src->getTypeSQLname())
                     << DgString2("unsupported length");
              result = FALSE;
            }
        else // inexact numeric
          if (tgt.d_.length_ == 4)
            {
              tgt.d_.sqlType_ = tmudr::TypeInfo::REAL;
              tgt.d_.cType_   = tmudr::TypeInfo::FLOAT;
            }
          else
            {
              // note that there is no SQL FLOAT in UDFs, SQL FLOAT
              // gets mapped to REAL or DOUBLE PRECISION
              CMPASSERT(tgt.d_.length_ == 8);
              tgt.d_.sqlType_ = tmudr::TypeInfo::DOUBLE_PRECISION;
              tgt.d_.cType_   = tmudr::TypeInfo::DOUBLE;
            }
      }
      break;

    case NA_CHARACTER_TYPE:
      {
        CharInfo::CharSet cs = (CharInfo::CharSet) src->getScaleOrCharset();

        if (src->isVaryingLen())
          {
            tgt.d_.sqlType_ = tmudr::TypeInfo::VARCHAR;
            if (src->getVarLenHdrSize() == 4)
              tgt.d_.flags_ |= tmudr::TypeInfo::TYPE_FLAG_4_BYTE_VC_LEN;
          }
        else
          tgt.d_.sqlType_ = tmudr::TypeInfo::CHAR;

        // character set
        switch (cs)
          {
          case CharInfo::ISO88591:
            tgt.d_.charset_ = tmudr::TypeInfo::CHARSET_ISO88591;
            tgt.d_.cType_   = tmudr::TypeInfo::STRING;
            break;

          case CharInfo::UTF8:
            tgt.d_.charset_ = tmudr::TypeInfo::CHARSET_UTF8;
            tgt.d_.cType_   = tmudr::TypeInfo::STRING;
            break;

          case CharInfo::UCS2:
            tgt.d_.charset_ = tmudr::TypeInfo::CHARSET_UCS2;
            tgt.d_.cType_ = tmudr::TypeInfo::U16STRING;
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
        tgt.d_.precision_ = dType->getFractionPrecision();

        switch (dType->getSubtype())
          {
          case DatetimeType::SUBTYPE_SQLDate:
            tgt.d_.sqlType_ = tmudr::TypeInfo::DATE;
            break;
          case DatetimeType::SUBTYPE_SQLTime:
            tgt.d_.sqlType_ = tmudr::TypeInfo::TIME;
            break;
          case DatetimeType::SUBTYPE_SQLTimestamp:
            tgt.d_.sqlType_ = tmudr::TypeInfo::TIMESTAMP;
            break;
          default:
            *diags << DgSqlCode(11151)
                   << DgString0("type")
                   << DgString1(src->getTypeSQLname())
                   << DgString2("unsupported datetime subtype");
            result = FALSE;
          }

        // Todo, translate to time_t C type or to string??
        tgt.d_.cType_ = tmudr::TypeInfo::STRING;
      }
      break;

    case NA_INTERVAL_TYPE:
      {
        const IntervalType *iType = static_cast<const IntervalType *>(src);

        tgt.d_.sqlType_ = tmudr::TypeInfo::INTERVAL;
        tgt.d_.precision_ = iType->getLeadingPrecision();
        tgt.d_.scale_ = iType->getFractionPrecision();

        switch (src->getFSDatatype())
          {
          case REC_INT_YEAR:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_YEAR;
            break;
          case REC_INT_MONTH:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_MONTH;
            break;
          case REC_INT_YEAR_MONTH:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_YEAR_MONTH;
            break;
          case REC_INT_DAY:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_DAY;
            break;
          case REC_INT_HOUR:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_HOUR;
            break;
          case REC_INT_DAY_HOUR:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_DAY_HOUR;
            break;
          case REC_INT_MINUTE:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_MINUTE;
            break;
          case REC_INT_HOUR_MINUTE:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_HOUR_MINUTE;
            break;
          case REC_INT_DAY_MINUTE:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_DAY_MINUTE;
            break;
          case REC_INT_SECOND:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_SECOND;
            break;
          case REC_INT_MINUTE_SECOND:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_MINUTE_SECOND;
            break;
          case REC_INT_HOUR_SECOND:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_HOUR_SECOND;
            break;
          case REC_INT_DAY_SECOND:
            tgt.d_.intervalCode_ = tmudr::TypeInfo::INTERVAL_DAY_SECOND;
            break;
          default:
            *diags << DgSqlCode(11151)
                   << DgString0("type")
                   << DgString1("interval")
                   << DgString2("unsupported interval subtype");
            result = FALSE;
          }

        // Todo, convert to special data structure or just an integer?
        tgt.d_.cType_ = tmudr::TypeInfo::INT64;

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

NAColumn *TMUDFInternalSetup::createNAColumnFromColumnInfo(
     const tmudr::ColumnInfo &src,
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
       src.getColName().data(),
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

tmudr::UDRPlanInfo *TMUDFInternalSetup::createUDRPlanInfo(
     tmudr::UDRInvocationInfo *invocationInfo)
{
  return new tmudr::UDRPlanInfo(invocationInfo);
}

void TMUDFInternalSetup::setCallPhase(
     tmudr::UDRInvocationInfo *invocationInfo,
     tmudr::CallPhase cp)
{
  invocationInfo->callPhase_ = cp;
}

void TMUDFInternalSetup::resetCallPhase(
     tmudr::UDRInvocationInfo *invocationInfo)
{
  invocationInfo->callPhase_ = 
    tmudr::UNKNOWN_CALL_PHASE;
}

void TMUDFInternalSetup::setOffsets(tmudr::UDRInvocationInfo *invocationInfo,
                                    ExpTupleDesc *paramTupleDesc,
                                    ExpTupleDesc *outputTupleDesc,
                                    ExpTupleDesc **inputTupleDescs)
{
  if (paramTupleDesc)
    {
      CMPASSERT(invocationInfo->getNumFormalParameters() ==
                paramTupleDesc->numAttrs());
      invocationInfo->nonConstFormalParameters().setRecordLength(
           paramTupleDesc->tupleDataLength());
      for (int p=0; p<invocationInfo->getNumFormalParameters(); p++)
        {
          Attributes *attr = paramTupleDesc->getAttr(p);

          invocationInfo->nonConstFormalParameters().getColumn(p).getType().setOffsets(
               attr->getNullIndOffset(),
               attr->getVCLenIndOffset(),
               attr->getOffset());
        }
    }
  else
    {
      CMPASSERT(invocationInfo->getNumFormalParameters() == 0);
      invocationInfo->nonConstFormalParameters().setRecordLength(0);
    }

  // As we move from compile time to runtime, replace the actual
  // parameter list with the formal parameter list, since we
  // can expect the actual parameters at runtime to have the
  // types of the formal parameters. We should not access
  // formal parameters at runtime.
  // Also erase the compile time constant parameters.
  invocationInfo->nonConstActualParameters().setConstBuffer(0, NULL);

  while (invocationInfo->getNumActualParameters() > 0)
    invocationInfo->nonConstActualParameters().deleteColumn(0);

  while (invocationInfo->getNumFormalParameters() > 0)
    {
      invocationInfo->nonConstActualParameters().addColumn(
           invocationInfo->nonConstFormalParameters().getColumn(0));
      invocationInfo->nonConstFormalParameters().deleteColumn(0);
    }

  tmudr::TableInfo &ot = invocationInfo->getOutputTableInfo();

  if (outputTupleDesc)
    {
      CMPASSERT((outputTupleDesc == NULL &&
                 ot.getNumColumns() == 0) ||
                (ot.getNumColumns() == outputTupleDesc->numAttrs()));
      ot.setRecordLength(outputTupleDesc->tupleDataLength());
      for (int oc=0; oc<ot.getNumColumns(); oc++)
        {
          Attributes *attr = outputTupleDesc->getAttr(oc);

          ot.getColumn(oc).getType().setOffsets(attr->getNullIndOffset(),
                                                attr->getVCLenIndOffset(),
                                                attr->getOffset());
        }
    }
  else
    {
      CMPASSERT(ot.getNumColumns() == 0);
      ot.setRecordLength(0);
    }

  for (int i=0; i<invocationInfo->getNumTableInputs(); i++)
    {
      tmudr::TableInfo &it = invocationInfo->inputTableInfo_[i];
      ExpTupleDesc *inputTupleDesc = inputTupleDescs[i];

      if (inputTupleDesc)
        {
          CMPASSERT(it.getNumColumns() == inputTupleDesc->numAttrs());
          it.setRecordLength(inputTupleDesc->tupleDataLength());
          for (int ic=0; ic<it.getNumColumns(); ic++)
            {
              Attributes *attr = inputTupleDesc->getAttr(ic);
              
              it.getColumn(ic).getType().setOffsets(attr->getNullIndOffset(),
                                                    attr->getVCLenIndOffset(),
                                                    attr->getOffset());
            }
        }
      else
        {
          CMPASSERT(it.getNumColumns() == 0);
          it.setRecordLength(0);
        }
    }
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
