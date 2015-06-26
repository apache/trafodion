/**********************************************************************
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
/**********************************************************************/
#ifndef UDFDLLINTERACTION_H
#define UDFDLLINTERACTION_H
/* -*-C++-*-
/**************************************************************************
*
* File:         UdfDllInteraction.h
* Description:  Classes that define interaction of a TMUDF with a DLL
* Created:      3/01/2010
* Language:     C++
*
*************************************************************************
*/

#include "LmCommon.h"
#include "sqludr.h"
#include "BindWA.h"
#include "ExExeUtilCli.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------

class TMUDFDllInteraction;

// forward references
class TableMappingUDF;
namespace tmudr {
  class UDRInvocationInfo;
}


// if other RelExprs need a DLL interation we may need
// to define a common base class then.

// class TMUDFDllInteraction represents the metadata needed to interact
// with a specific compiler interface for one or more TMUDFs. It can
// be shared between multiple invocations of these TMUDFs.

// Todo: A cache, for now we create one for each TableMappingUDR RelExpr.

class TMUDFDllInteraction : public NABasicObject
{

  public :

  TMUDFDllInteraction();
  NABoolean describeParamsAndMaxOutputs(TableMappingUDF * tmudfNode,
                                        BindWA * bindWA);
  NABoolean createOutputInputColumnMap(TableMappingUDF * tmudfNode,
                                       ValueIdMap &result);
  NABoolean describeDataflow(TableMappingUDF * tmudfNode,
                             ValueIdSet &valuesRequiredByParent,
                             ValueIdSet &selectionPreds,
                             ValueIdSet &predsEvaluatedByUDF,
                             ValueIdSet &predsToPushDown);
  NABoolean describeConstraints(TableMappingUDF * tmudfNode);
  NABoolean describeStatistics(TableMappingUDF * tmudfNode,
                               const EstLogPropSharedPtr& inputEstLogProp);
  NABoolean degreeOfParallelism(TableMappingUDF * tmudfNode,
                                TMUDFPlanWorkSpace * pws,
                                int &dop);
  NABoolean finalizePlan(TableMappingUDF * tmudfNode,
                         tmudr::UDRPlanInfo *planInfo);

  // methods for retrieving statistics specified by the UDR
  CostScalar getResultCardinality(TableMappingUDF *tmudfNode);
  CostScalar getCardinalityScaleFactorFromFunctionType(
       TableMappingUDF *tmudfNode);
  CostScalar getOutputColumnUEC(TableMappingUDF *tmudfNode,
                                int colNum);

  // helper methods for routine invocation and error handling
  NABoolean invokeRoutine(tmudr::UDRInvocationInfo::CallPhase cp,
                          TableMappingUDF * tmudfNode,
                          tmudr::UDRPlanInfo *planInfo = NULL,
                          ComDiagsArea *diags = NULL);
  static void processReturnStatus(const tmudr::UDRException &e, 
                                  TableMappingUDF *tmudfNode);
  static void processReturnStatus(const tmudr::UDRException &e, 
                                  const char * routineName,
                                  ComDiagsArea *diags = NULL);

private:
  // do not use this for things other than routine invocations,
  // since that could lead to resource leaks
  ExeCliInterface cliInterface_;
};

// Class used to convert Trafodion classes to and from the C++
// compiler interface classes defined in sqludr.h.

// This class is a friend of many of the C++ compiler interface classes
// and therefore it can set private data members that the UDF writer
// should not set directly

class TMUDFInternalSetup
{
public:

  // these methods could be made methods of the C++ interface itself, but
  // that would expose them to the UDF writer, who should not call them,
  // therefore we make this class a friend and implement them here as static
  // member functions

  // methods to convert Trafodion objects to tmudr objects
  // (allocated on system heap, if needed)

  static tmudr::UDRInvocationInfo *createInvocationInfoFromRelExpr(
       TableMappingUDF * tmudfNode,
       char *&constBuffer,
       int &constBufferLength,
       ComDiagsArea *diags);
  static NABoolean setTypeInfoFromNAType(
       tmudr::TypeInfo &tgt,
       const NAType *src,
       ComDiagsArea *diags);
  static tmudr::ColumnInfo *createColumnInfoFromNAColumn(
       const NAColumn *src,
       ComDiagsArea *diags);
  static NABoolean setTableInfoFromNAColumnArray(
       tmudr::TableInfo &tgt,
       const NAColumnArray *src,
       ComDiagsArea *diags);
  static NABoolean setPredicateInfoFromValueIdSet(
       tmudr::UDRInvocationInfo *tgt,
       const ValueIdList &udfOutputColumns,
       const ValueIdSet &predicates,
       ValueIdList &convertedPredicates,
       NABitVector &usedColPositions);
  static NABoolean removeUnusedColumnsAndPredicates(
       tmudr::UDRInvocationInfo *tgt);
  static NABoolean createConstraintInfoFromRelExpr(
       TableMappingUDF * tmudfNode);
  static NABoolean setChildOutputStats(
       tmudr::UDRInvocationInfo *tgt,
       TableMappingUDF * tmudfNode,
       const EstLogPropSharedPtr& inputLP);

  // methods to convert tmudr objects to Trafodion objects (allocated on NAHeap)
  static NAType *createNATypeFromTypeInfo(
       const tmudr::TypeInfo &src,
       int colNumForDiags,
       NAHeap *heap,
       ComDiagsArea *diags);
  static NAColumn *createNAColumnFromColumnInfo(
       const tmudr::ColumnInfo &src,
       int position,
       NAHeap *heap,
       ComDiagsArea *diags);
  static NAColumnArray * createColumnArrayFromTableInfo(
       const tmudr::TableInfo &tableInfo,
       TableMappingUDF * tmudfNode,
       NAHeap *heap,
       ComDiagsArea *diags);
  static NABoolean createConstraintsFromConstraintInfo(
       const tmudr::TableInfo &tableInfo,
       TableMappingUDF * tmudfNode,
       NAHeap *heap);

  // invoke private constructors/destructors of the interface structs
  static tmudr::UDRPlanInfo *createUDRPlanInfo(
       tmudr::UDRInvocationInfo *invocationInfo,
       int planNum);
  static void setOffsets(tmudr::UDRInvocationInfo *invocationInfo,
                         ExpTupleDesc *inParamTupleDesc,
                         ExpTupleDesc *outputTupleDesc,
                         ExpTupleDesc **inputTupleDescs);
  static void deleteUDRInvocationInfo(tmudr::UDRInvocationInfo *toDelete);
  static void deleteUDRPlanInfo(tmudr::UDRPlanInfo *toDelete);
};

#endif /* UDFDLLINTERACTION_H */
