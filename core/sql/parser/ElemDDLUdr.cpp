/* -*-C++-*- */
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
 * File:         ElemDDLUdr.C
 * Description:  methods for classes relating to UDRs.
 *               
 *               
 * Created:      10/13/1999
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "AllElemDDLUdr.h"
#include "ComASSERT.h"
#include "ComDiags.h"
#include "ComOperators.h"
#include "ComMisc.h"


// -----------------------------------------------------------------------
// methods for class ElemDDLUdrDeterministic
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrDeterministic::ElemDDLUdrDeterministic(NABoolean theDeterministic)
         : ElemDDLNode(ELM_UDR_DETERMINISTIC), 
         deterministic_(theDeterministic)
{
}

// virtual destructor
ElemDDLUdrDeterministic::~ElemDDLUdrDeterministic(void)
{
}

// cast
ElemDDLUdrDeterministic * 
  ElemDDLUdrDeterministic::castToElemDDLUdrDeterministic(void)
{
  return this;
}


//
// methods for tracing
//

NATraceList
ElemDDLUdrDeterministic::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "deterministic? ";
  detailText += YesNo(getDeterministic());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdrDeterministic::getText() const
{
  return "ElemDDLUdrDeterministic";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdrExternalName
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrExternalName::ElemDDLUdrExternalName(NAString &theName) 
  : ElemDDLNode(ELM_UDR_EXTERNAL_NAME),
    externalName_(theName)
{
}

// virtual destructor
ElemDDLUdrExternalName::~ElemDDLUdrExternalName()
{
}

// cast
ElemDDLUdrExternalName * 
  ElemDDLUdrExternalName::castToElemDDLUdrExternalName(void)
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLUdrExternalName::displayLabel1() const
{
  return NAString("External name: ") + externalName_;
}

NATraceList
ElemDDLUdrExternalName::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());

  return detailTextList;

}

const NAString
ElemDDLUdrExternalName::getText() const
{
  return "ElemDDLUdrExternalName";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLUdrExternalPath
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrExternalPath::ElemDDLUdrExternalPath(NAString &thePath) 
  : ElemDDLNode(ELM_UDR_EXTERNAL_PATH),
    externalPath_(thePath)
{
  TrimNAStringSpace(externalPath_);
}

// virtual destructor
ElemDDLUdrExternalPath::~ElemDDLUdrExternalPath()
{
}

// cast
ElemDDLUdrExternalPath * 
  ElemDDLUdrExternalPath::castToElemDDLUdrExternalPath(void)
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLUdrExternalPath::displayLabel1() const
{
  return NAString("External path: ") + externalPath_;
}

NATraceList
ElemDDLUdrExternalPath::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());

  return detailTextList;

}

const NAString
ElemDDLUdrExternalPath::getText() const
{
  return "ElemDDLUdrExternalPath";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLUdrIsolate
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrIsolate::ElemDDLUdrIsolate(NABoolean theIsolate)
  :  ElemDDLNode(ELM_UDR_ISOLATE), 
     isolate_(theIsolate)
{
}

// virtual destructor
ElemDDLUdrIsolate::~ElemDDLUdrIsolate(void)
{
}

// cast
ElemDDLUdrIsolate * 
  ElemDDLUdrIsolate::castToElemDDLUdrIsolate(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdrIsolate::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "isolate? ";
  detailText += YesNo(getIsolate());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdrIsolate::getText() const
{
  return "ElemDDLUdrIsolate";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLUdrLanguage
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrLanguage::ElemDDLUdrLanguage(ComRoutineLanguage theLanguage)
    : ElemDDLNode(ELM_UDR_LANGUAGE),
      language_(theLanguage)
{
}

// virtual destructor
ElemDDLUdrLanguage::~ElemDDLUdrLanguage(void)
{
}

// cast
ElemDDLUdrLanguage * 
  ElemDDLUdrLanguage::castToElemDDLUdrLanguage(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdrLanguage::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "Language: ";
  detailText += UnsignedToNAString((UInt32)getLanguage());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdrLanguage::getText() const
{
  return "ElemDDLUdrLanguage";
}




// -----------------------------------------------------------------------
// methods for class ElemDDLUdrLibrary
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrLibrary::ElemDDLUdrLibrary(QualifiedName &theLibrary) 
  : ElemDDLNode(ELM_UDR_LIBRARY),
    libraryName_(theLibrary)
{
}

// virtual destructor
ElemDDLUdrLibrary::~ElemDDLUdrLibrary()
{
}

// cast
ElemDDLUdrLibrary * 
  ElemDDLUdrLibrary::castToElemDDLUdrLibrary(void)
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLUdrLibrary::displayLabel1() const
{
  return NAString("Library: ") + libraryName_.getQualifiedNameAsAnsiString();
}

NATraceList
ElemDDLUdrLibrary::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());

  return detailTextList;

}

const NAString
ElemDDLUdrLibrary::getText() const
{
  return "ElemDDLUdrLibrary";
}







// -----------------------------------------------------------------------
// methods for class ElemDDLUdrMaxResults
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrMaxResults::ElemDDLUdrMaxResults(ComUInt32 theMaxResults)
  : ElemDDLNode(ELM_UDR_MAX_RESULTS),
    maxResults_(theMaxResults)
{
}

// virtual destructor
ElemDDLUdrMaxResults::~ElemDDLUdrMaxResults(void)
{
}

// cast
ElemDDLUdrMaxResults * 
  ElemDDLUdrMaxResults::castToElemDDLUdrMaxResults(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdrMaxResults::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "MaxResults: ";
  detailText += LongToNAString(getMaxResults());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdrMaxResults::getText() const
{
  return "ElemDDLUdrMaxResults";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLUdrParamStyle
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrParamStyle::ElemDDLUdrParamStyle(ComRoutineParamStyle theParamStyle)
  : ElemDDLNode(ELM_UDR_PARAM_STYLE),
    paramStyle_(theParamStyle) 
{
}

// virtual destructor
ElemDDLUdrParamStyle::~ElemDDLUdrParamStyle(void)
{
}

// cast
ElemDDLUdrParamStyle * 
  ElemDDLUdrParamStyle::castToElemDDLUdrParamStyle(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdrParamStyle::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "ParamStyle: ";
  detailText += UnsignedToNAString((UInt32)getParamStyle());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdrParamStyle::getText() const
{
  return "ElemDDLUdrParamStyle";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLUdrSqlAccess
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrSqlAccess::ElemDDLUdrSqlAccess(ComRoutineSQLAccess theSqlAccess)
  : ElemDDLNode(ELM_UDR_SQL_ACCESS),
    sqlAccess_(theSqlAccess)
{
}

// virtual destructor
ElemDDLUdrSqlAccess::~ElemDDLUdrSqlAccess(void)
{
}

// cast
ElemDDLUdrSqlAccess * 
  ElemDDLUdrSqlAccess::castToElemDDLUdrSqlAccess(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdrSqlAccess::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "SqlAccess: ";
  detailText += UnsignedToNAString((UInt32)getSqlAccess());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdrSqlAccess::getText() const
{
  return "ElemDDLUdrSqlAccess";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdrTransaction
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrTransaction::ElemDDLUdrTransaction(ComRoutineTransactionAttributes theTransactionAttributes)
  : ElemDDLNode(ELM_UDR_TRANSACTION_ATTRIBUTES),
    transactionAttributes_(theTransactionAttributes)
{
}

// virtual destructor
ElemDDLUdrTransaction::~ElemDDLUdrTransaction(void)
{
}

// cast
ElemDDLUdrTransaction *
ElemDDLUdrTransaction::castToElemDDLUdrTransaction(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdrTransaction::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "Transaction: ";
  detailText += UnsignedToNAString((UInt32)getTransactionAttributes());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdrTransaction::getText() const
{
  return "ElemDDLUdrTransaction";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdrExternalSecurity
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdrExternalSecurity::ElemDDLUdrExternalSecurity(ComRoutineExternalSecurity theExternalSecurity)
  : ElemDDLNode(ELM_UDR_EXTERNAL_SECURITY),
    externalSecurity_(theExternalSecurity) 
{
}

// virtual destructor
ElemDDLUdrExternalSecurity::~ElemDDLUdrExternalSecurity(void)
{
}

// cast
ElemDDLUdrExternalSecurity * 
  ElemDDLUdrExternalSecurity::castToElemDDLUdrExternalSecurity(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdrExternalSecurity::getDetailInfo() const
{
  NAString    detailText;
  NATraceList detailTextList;

  detailText = "ExternalSecurity: ";
  detailText += UnsignedToNAString((UInt32)getExternalSecurity());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdrExternalSecurity::getText() const
{
  return "ElemDDLUdrExternalSecurity";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLUdfExecutionMode
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdfExecutionMode::ElemDDLUdfExecutionMode(ComRoutineExecutionMode theExecutionMode)
         : ElemDDLNode(ELM_UDF_EXECUTION_MODE),
           executionMode_(theExecutionMode)
{
}

// virtual destructor
ElemDDLUdfExecutionMode::~ElemDDLUdfExecutionMode(void)
{
}

// cast
ElemDDLUdfExecutionMode *
ElemDDLUdfExecutionMode::castToElemDDLUdfExecutionMode(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdfExecutionMode::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "execution mode = ";
  if (getExecutionMode() EQU COM_ROUTINE_SAFE_EXECUTION)
    detailText += "SAFE";
  else
    detailText += "FAST";
  detailTextList.append(detailText);

  return detailTextList;
}

const NAString
ElemDDLUdfExecutionMode::getText() const
{
  return "ElemDDLUdfExecutionMode";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdfFinalCall
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdfFinalCall::ElemDDLUdfFinalCall(NABoolean theFinalCall)
  :  ElemDDLNode(ELM_UDF_FINAL_CALL),
     finalCall_(theFinalCall)
{
}

// virtual destructor
ElemDDLUdfFinalCall::~ElemDDLUdfFinalCall(void)
{
}

// cast
ElemDDLUdfFinalCall *
  ElemDDLUdfFinalCall::castToElemDDLUdfFinalCall(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdfFinalCall::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "finalCall? ";
  detailText += YesNo(getFinalCall());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdfFinalCall::getText() const
{
  return "ElemDDLUdfFinalCall";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdfOptimizationHint
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdfOptimizationHint::ElemDDLUdfOptimizationHint(ComUdfOptimizationHintKind optimizationKind,
                                                       CollHeap *h) // default is PARSERHEAP()
         : ElemDDLNode(ELM_UDF_OPTIMIZATION_HINT),
           udfOptimizationKind_(optimizationKind),
           cost_(-1),
           uniqueOutputValuesParseTree_(NULL),
           uniqueOutputValues_(h)
{
}

// virtual destructor
ElemDDLUdfOptimizationHint::~ElemDDLUdfOptimizationHint(void)
{
}

// cast
ElemDDLUdfOptimizationHint *
ElemDDLUdfOptimizationHint::castToElemDDLUdfOptimizationHint(void)
{
  return this;
}

//
// helpers
//

void ElemDDLUdfOptimizationHint::synthesize(void)
{
  if (getOptimizationKind() NEQ COM_UDF_NUMBER_OF_UNIQUE_OUTPUT_VALUES OR
      uniqueOutputValuesParseTree_ EQU NULL)
    return;

  NABoolean    isErrMsgIssued = FALSE;
  ComSInt64    value = 0;
  ItemExpr   * pItemExpr = NULL;
  ConstValue * pConstVal = NULL;
  for (CollIndex i = 0; i < uniqueOutputValuesParseTree_->entries(); i++)
  {
    pItemExpr = (*uniqueOutputValuesParseTree_)[i];
    pConstVal = (ConstValue *)pItemExpr;
    if (NOT pConstVal->canGetExactNumericValue() AND NOT isErrMsgIssued)
    {
      *SqlParser_Diags << DgSqlCode(-3017) << DgString0(pConstVal->getConstStr());
      isErrMsgIssued = TRUE;
    }
    value = pConstVal->getExactNumericValue();
    uniqueOutputValues_.insert(value);
    if (value < -1 AND NOT isErrMsgIssued) // only issue the error message once
    {
      // Error: Expected a positive value or -1 (representing the default SYSTEM setting option)
      NAString valueStr = Int64ToNAString(value);
      *SqlParser_Diags << DgSqlCode(-3017) << DgString0(valueStr);
      isErrMsgIssued = TRUE;
    }
  } // for
} // ElemDDLUdfOptimizationHint::synthesize()

//
// methods for tracing
//

NATraceList
ElemDDLUdfOptimizationHint::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "optimization hint = ";
  detailText +=  LongToNAString((Lng32)getOptimizationKind());
  if (getOptimizationKind() NEQ COM_UDF_NUMBER_OF_UNIQUE_OUTPUT_VALUES)
  {
    detailText +=  " - cost = ";
    detailText +=  LongToNAString((Lng32)getCost());
  }
  else
  {
    detailText += " - number of unique output values = ( ";
    const NAList<ComSInt64> values = getUniqueOutputValues();
    for (CollIndex i = 0; i < values.entries(); i++)
    {
      if (i > 0) detailText += " , ";
      detailText += Int64ToNAString((Int64)values[i]);
    }
    detailText += " )";
  }
  detailTextList.append(detailText);

  return detailTextList;
}

const NAString
ElemDDLUdfOptimizationHint::getText() const
{
  return "ElemDDLUdfOptimizationHint";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdfParallelism
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdfParallelism::ElemDDLUdfParallelism(ComRoutineParallelism parallelism)
  :  ElemDDLNode(ELM_UDF_PARALLELISM),
     parallelism_(parallelism)
{
}

// virtual destructor
ElemDDLUdfParallelism::~ElemDDLUdfParallelism(void)
{
}

// cast
ElemDDLUdfParallelism *
  ElemDDLUdfParallelism::castToElemDDLUdfParallelism(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdfParallelism::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "canBeParallel? ";
  detailText += YesNo(getCanBeParallel());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdfParallelism::getText() const
{
  return "ElemDDLUdfParallelism";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdfSpecialAttributes
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdfSpecialAttributes::ElemDDLUdfSpecialAttributes(const NAString &theSpecialAttributes,
                                                         CollHeap *h) // default is PARSERHEAP()
         : ElemDDLNode(ELM_UDF_SPECIAL_ATTRIBUTES),
           specialAttributesText_(theSpecialAttributes, h)
{
}

// virtual destructor
ElemDDLUdfSpecialAttributes::~ElemDDLUdfSpecialAttributes(void)
{
}

// cast
ElemDDLUdfSpecialAttributes *
ElemDDLUdfSpecialAttributes::castToElemDDLUdfSpecialAttributes(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdfSpecialAttributes::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "special attributes = ";
  detailText += getSpecialAttributesText();
  detailTextList.append(detailText);

  return detailTextList;
}

const NAString
ElemDDLUdfSpecialAttributes::getText() const
{
  return "ElemDDLUdfSpecialAttributes";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdfStateAreaSize
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdfStateAreaSize::ElemDDLUdfStateAreaSize(ComUInt32 sizeInBytes)
  : ElemDDLNode(ELM_UDF_STATE_AREA_SIZE),
    stateAreaSize_(sizeInBytes)
{
}

// virtual destructor
ElemDDLUdfStateAreaSize::~ElemDDLUdfStateAreaSize(void)
{
}

// cast
ElemDDLUdfStateAreaSize *
  ElemDDLUdfStateAreaSize::castToElemDDLUdfStateAreaSize(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdfStateAreaSize::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "StateAreaSize: ";
  detailText += LongToNAString(getStateAreaSize());
  detailTextList.append(detailText);

  return detailTextList;

}

const NAString
ElemDDLUdfStateAreaSize::getText() const
{
  return "ElemDDLUdfStateAreaSize";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUdfVersionTag
// -----------------------------------------------------------------------

// default constructor
ElemDDLUdfVersionTag::ElemDDLUdfVersionTag(const NAString &theVersionTag,
                                           CollHeap *h) // default is PARSERHEAP()
         : ElemDDLNode(ELM_UDF_VERSION_TAG),
           versionTag_(theVersionTag, h)
{
}

// virtual destructor
ElemDDLUdfVersionTag::~ElemDDLUdfVersionTag(void)
{
}

// cast
ElemDDLUdfVersionTag *
ElemDDLUdfVersionTag::castToElemDDLUdfVersionTag(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUdfVersionTag::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "version tag = ";
  detailText += getVersionTag();
  detailTextList.append(detailText);

  return detailTextList;
}

const NAString
ElemDDLUdfVersionTag::getText() const
{
  return "ElemDDLUdfVersionTag";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLUudfParamDef
// -----------------------------------------------------------------------

// default constructor
ElemDDLUudfParamDef::ElemDDLUudfParamDef(ComUudfParamKind uudfParamKind)
  : ElemDDLNode(ELM_UUDF_PARAM_DEF),
    uudfParamKind_(uudfParamKind)
{
}

// virtual destructor
ElemDDLUudfParamDef::~ElemDDLUudfParamDef(void)
{
}

// cast
ElemDDLUudfParamDef *
ElemDDLUudfParamDef::castToElemDDLUudfParamDef(void)
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLUudfParamDef::getDetailInfo() const
{
  NAString uudfParamKindName;
  NAString        detailText;
  NATraceList detailTextList;

  // ComGetUudfParamKindAsLit(getUudfParamKind(), // in
  //                          uudfParamKindName); // out
  // if (uudfParamKindName.isNull())
  //   uudfParamKindName = "OMITTED";
  detailText = "universal function parameter kind = " + uudfParamKindName;
  detailTextList.append(detailText);

  return detailTextList;
}

const NAString
ElemDDLUudfParamDef::getText() const
{
  return "ElemDDLUudfParamDef";
}

