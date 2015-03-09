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
#include "ItemLog.h"
#include "ItemOther.h"
#include "NARoutine.h"
#include "SchemaDB.h"
#include "GroupAttr.h"
#include "exp_attrs.h"
#include "LmError.h"
#include "ComUser.h"


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
  tmudr::UDR *udrInterface = tmudfNode->getUDRInterface();

#ifndef NDEBUG
  if (invocationInfo->getDebugFlags() & tmudr::UDRInvocationInfo::PRINT_INVOCATION_INFO_INITIAL)
    invocationInfo->print();
#endif

  try
    {
      if (!udrInterface)
        {
          // try to allocate a user-provided class for compiler interaction
          if (createInterfaceObjectPtr_)
            {
              tmudr::CreateInterfaceObjectFunc factoryFunc =
                reinterpret_cast<tmudr::CreateInterfaceObjectFunc>(
                     createInterfaceObjectPtr_);

              // call the factory method to create an object that is
              // derived from UDR
              udrInterface = (*factoryFunc)();
            }
          else
            // just use the default implementation in the base class
            udrInterface = new tmudr::UDR();

          tmudfNode->setUDRInterface(udrInterface);
        }

#ifndef NDEBUG
      if (invocationInfo->getDebugFlags() &
          tmudr::UDRInvocationInfo::DEBUG_INITIAL_COMPILE_TIME_LOOP)
        udrInterface->debugLoop();
#endif

      TMUDFInternalSetup::setCallPhase(
           invocationInfo,
           tmudr::UDRInvocationInfo::COMPILER_INITIAL_CALL);
      udrInterface->describeParamsAndColumns(*invocationInfo);
      TMUDFInternalSetup::resetCallPhase(invocationInfo);
    }
  catch (tmudr::UDRException e)
    {
      processReturnStatus(e, tmudfNode);
      bindWA->setErrStatus();
      return FALSE;
    }
  catch (...)
    {
      processReturnStatus(
           tmudr::UDRException(
                38900, "Unknown exception during describeParamsAndColumns()"),
           tmudfNode);
      return FALSE;
    }

  // copy the formal parameter list back into the RelExpr
  NAHeap *outHeap = CmpCommon::statementHeap();
  NAColumnArray * modifiedParameterArray =
    new(outHeap) NAColumnArray(outHeap);

  for (int p=0; p<invocationInfo->getFormalParameters().getNumColumns(); p++)
    {
      NAColumn *newParam = 
        TMUDFInternalSetup::createNAColumnFromColumnInfo(
             invocationInfo->getFormalParameters().getColumn(p),
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

  // copy the output columns back into the RelExpr
  NAColumnArray * outColArray =
    TMUDFInternalSetup::createColumnArrayFromTableInfo(
         invocationInfo->out(),
         tmudfNode,
         outHeap,
         CmpCommon::diags());
  if (outColArray == NULL)
    {
      bindWA->setErrStatus();
      return FALSE;
    }

  tmudfNode->setOutputParams(outColArray);

  // copy the query partition by and order by back into childInfo
  for (int c=0; c<tmudfNode->getArity(); c++)
    {
      TableMappingUDFChildInfo *childInfo = tmudfNode->getChildInfo(c);
      const ValueIdList &childCols = childInfo->getOutputs();
      const tmudr::PartitionInfo &childPartInfo = invocationInfo->in(c).getQueryPartitioning();
      const tmudr::OrderInfo &childOrderInfo = invocationInfo->in(c).getQueryOrdering();
      TMUDFInputPartReq childPartType;
      ValueIdSet childPartKey;
      ValueIdList childOrderBy;

      switch (childPartInfo.getType())
        {
        case tmudr::PartitionInfo::ANY:
          childPartType = ANY_PARTITIONING;
          break;

        case tmudr::PartitionInfo::SERIAL:
          childPartType = NO_PARTITIONING;
          break;

        case tmudr::PartitionInfo::PARTITION:
          {
            childPartType = SPECIFIED_PARTITIONING;
            // convert column numbers back to ValueIds
            for (int p=0; p<childPartInfo.size(); p++)
              {
                int colNum = childPartInfo[p];

                if (colNum < childCols.entries() && colNum >= 0)
                  {
                    childPartKey += childCols[colNum];
                  }
                else
                  processReturnStatus(
                       tmudr::UDRException(
                            38900,
                            "Invalid child column number %d used in partition by key of child table %d with %d columns",
                            colNum, c, childCols.entries()),
                       tmudfNode);
                            
                            
              }
          }
          break;

        case tmudr::PartitionInfo::REPLICATE:
          childPartType = REPLICATE_PARTITIONING;
          break;

        default:
          processReturnStatus(
               tmudr::UDRException(
                    38900,
                    "Invalid partitioning type %d used in partition by key of child table %d",
                    static_cast<int>(childPartInfo.getType()), c),
               tmudfNode);
          break;
        }

      for (int oc=0; oc<childOrderInfo.getNumEntries(); oc++)
        {
          int colNum = childOrderInfo.getColumnNum(oc);

          if (colNum < childCols.entries() && colNum >= 0)
            {
              if (childOrderInfo.getOrderType(oc) == tmudr::OrderInfo::DESCENDING)
                {
                  ItemExpr *inv = new(CmpCommon::statementHeap())
                    InverseOrder(childCols[colNum].getItemExpr());
                  inv->synthTypeAndValueId();
                  childOrderBy.insert(inv->getValueId());
                }
              childOrderBy.insert(childCols[colNum]);
            }
          else
            processReturnStatus(
                 tmudr::UDRException(
                      38900,
                      "Invalid child column number %d used in order by key of child table %d with %d columns",
                      colNum, c, childCols.entries()),
                 tmudfNode);
        }

      // now transfer all this info into childInfo
      childInfo->setPartitionType(childPartType);
      childInfo->setPartitionBy(childPartKey);
      childInfo->setOrderBy(childOrderBy);
    }

  return TRUE;
}

NABoolean TMUDFDllInteraction::createOutputInputColumnMap(
     TableMappingUDF * tmudfNode,
     ValueIdMap &result)
{
  tmudr::UDRInvocationInfo * invocationInfo = tmudfNode->getInvocationInfo();
  tmudr::TableInfo & outputTableInfo = invocationInfo->out();
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
                             getOutputs()[p.getInputColumnNum()]);
        }
    }

  return TRUE;
}

NABoolean TMUDFDllInteraction::describeDataflow(
     TableMappingUDF * tmudfNode,        // in
     ValueIdSet &valuesRequiredByParent, // out: values the parent needs to
                                         //      require from its children
     ValueIdSet &selectionPreds,         // in:  predicates that could
                                         //      potentially be pushed down
                                         // out: predicates that need to be
                                         //      evaluated on the UDF result
     ValueIdSet &predsEvaluatedByUDF,    // out: predicates evaluated by the
                                         //      UDF (in user-written code)
     ValueIdSet &predsToPushDown)        // out: predicates to be pushed down
                                         //      to the children (expressed
                                         //      in terms of child value ids)
{
  tmudr::UDRInvocationInfo * invocationInfo = tmudfNode->getInvocationInfo();
  tmudr::TableInfo & outputTableInfo = invocationInfo->out();
  int numOutputColumns = outputTableInfo.getNumColumns();
  ValueIdList &udfOutputCols = tmudfNode->getProcOutputParamsVids();
  ValueIdSet udfOutputColSet(udfOutputCols);
  const ValueIdSet &udfCharOutputs =
    tmudfNode->getGroupAttr()->getCharacteristicOutputs();
  ValueIdSet usedOutputColumns(udfCharOutputs);
  ValueIdSet exprsOnOutputColumns(udfCharOutputs);
  ValueIdList predsOfferedToUDF;
  NABitVector usedColPositions;

  // don't clear valuesRequiredByParent, we just add to this set
  // clear remaining output parameters
  predsEvaluatedByUDF.clear();
  predsToPushDown.clear();

  // start with those characteristic outputs that are output columns
  // of the UDF
  usedOutputColumns.intersectSet(udfOutputColSet);

  // next, look into more complex expressions that reference output columns
  exprsOnOutputColumns -= udfOutputColSet;

  // find out which of the UDF outputs are referenced by these
  // more complex expressions
  ValueIdSet temp(udfOutputColSet);
  exprsOnOutputColumns.weedOutUnreferenced(temp);
  usedOutputColumns += temp;

  // loop over the characteristic outputs and translate them into ordinals
  for (ValueId udfOutputCol=usedOutputColumns.init(); 
       usedOutputColumns.next(udfOutputCol);
       usedOutputColumns.advance(udfOutputCol))
    {
      CollIndex ordinal =
        tmudfNode->getProcOutputParamsVids().index(udfOutputCol);

      CMPASSERT(ordinal != NULL_COLL_INDEX)
      usedColPositions += ordinal;
    }

  // set up predicate information from the selection predicates
  if (!TMUDFInternalSetup::setPredicateInfoFromValueIdSet(
           invocationInfo,
           udfOutputCols,
           tmudfNode->selectionPred(),
           predsOfferedToUDF,
           usedColPositions))
    return FALSE;

  // initialize usage info for all columns
  for (int c=0; c<numOutputColumns; c++)
    outputTableInfo.getColumn(c).setUsage(
         (usedColPositions.contains(c) ?
          tmudr::ColumnInfo::USED :
          tmudr::ColumnInfo::NOT_USED));

  try
    {
      TMUDFInternalSetup::setCallPhase(
           invocationInfo,
           tmudr::UDRInvocationInfo::COMPILER_DATAFLOW_CALL);
      tmudfNode->getUDRInterface()->describeDataflowAndPredicates(
           *invocationInfo);
      TMUDFInternalSetup::resetCallPhase(invocationInfo);
    }
  catch (tmudr::UDRException e)
    {
      processReturnStatus(e, tmudfNode);
      return FALSE;
    }
  catch (...)
    {
      processReturnStatus(
           tmudr::UDRException(
                38900, "Unknown exception during describeDataflowAndPredicates()"),
           tmudfNode);
      return FALSE;
    }

  // Remove any unused output columns. Also, just as a sanity check,
  // make sure the UDF didn't change any of the output columns' usage
  for (int x=udfOutputCols.entries()-1; x>=0; x--)
    {
      tmudr::ColumnInfo::ColumnUseCode usage =
        invocationInfo->out().getColumn(x).getUsage();

      if (usedColPositions.contains(x))
        {
          if (usage != tmudr::ColumnInfo::USED)
            {
              processReturnStatus(
                   tmudr::UDRException(
                        38900,
                        "UDF output column %d changed from used to not used.",
                        x),
                   tmudfNode);
              return FALSE;
            }
        }
      else
        {
          if (usage == tmudr::ColumnInfo::USED)
            {
              processReturnStatus(
                   tmudr::UDRException(
                        38900,
                        "UDF output column %d changed from not used to used",
                        x),
                   tmudfNode);
            }
          else if (usage == tmudr::ColumnInfo::NOT_PRODUCED)
            // remove this column from the list
            // of output columns, the UDF allowed it by
            // setting the usage code to NOT_PRODUCED
            tmudfNode->removeOutputParam(x);
        }
    }

  // For each child column marked as "used" by the UDF, treat it as an
  // expression required by the parent.
  // For each predicate marked as pushable to a child, rewrite it in terms of
  // child value ids and use it as a selection predicate for
  // pushdownCoveredExprs. For every predicate marked as evaluated by the
  // UDF, remove it from the selection predicates.

  for (int i=0; i<tmudfNode->getArity(); i++)
    {
      const tmudr::TableInfo &ti = invocationInfo->in(i);
      const tmudr::PartitionInfo &pi = ti.getQueryPartitioning();
      const tmudr::OrderInfo &oi = ti.getQueryOrdering();
      ValueIdList &childOutputs = tmudfNode->getChildInfo(i)->getOutputIds();
      int numInputCols = ti.getNumColumns();

      // mark all columns used by PARTITION BY or ORDER BY as used,
      // in case the UDF didn't
      for (std::vector<int>::const_iterator it = pi.begin();
           it != pi.end();
           it++)
        invocationInfo->setChildColumnUsage(i,*it,tmudr::ColumnInfo::USED);
      for (int oc=0; oc<oi.getNumEntries(); oc++)
        invocationInfo->setChildColumnUsage(i,
                                            oi.getColumnNum(oc),
                                            tmudr::ColumnInfo::USED);

      // go backwards, so we can remove by position from
      // a ValueIdList below
      for (int c=numInputCols-1; c>=0; c--)
        switch (ti.getColumn(c).getUsage())
          {
          case tmudr::ColumnInfo::UNKNOWN:
            // the UDF didn't set any usage info, assume USED
            invocationInfo->setChildColumnUsage(
                 i, c, tmudr::ColumnInfo::USED);
            // fall through to next case
          case tmudr::ColumnInfo::USED:
            // This column is needed, treat it as an expression needed by
            // the parent.
            valuesRequiredByParent += childOutputs[c];
            break;

          case tmudr::ColumnInfo::NOT_PRODUCED:
          case tmudr::ColumnInfo::NOT_USED:
            // Remove the column from the NAColumnArray and ValueIdList
            // describing the child table. We don't distinguish NOT_USED
            // and NOT_PRODUCED on children, since both are set by the UDF.
            tmudfNode->getChildInfo(i)->removeColumn(c);
            break;

          default:
            processReturnStatus(
                 tmudr::UDRException(
                      38900,
                      "Invalid usage code %d for column %d of child %d",
                      ti.getColumn(c).getUsage(), c, i),
                 tmudfNode);
            return FALSE;
          }
    }

  // walk through predicates and handle the evaluation codes assigned to them
  for (int p=0; p<predsOfferedToUDF.entries(); p++)
    {
      int evalCode = static_cast<int>(
           invocationInfo->getPredicate(p).getEvaluationCode());

      // evaluate the predicate on the UDF result if the evaluation
      // code was not set at all or if the EVALUATE_ON_RESULT flag
      // is set
      if (!(evalCode == tmudr::PredicateInfo::UNKNOWN_EVAL ||
            evalCode & tmudr::PredicateInfo::EVALUATE_ON_RESULT))
        selectionPreds -= predsOfferedToUDF[p];

      if (evalCode & tmudr::PredicateInfo::EVALUATE_IN_UDF)
        predsEvaluatedByUDF += predsOfferedToUDF[p];

      if (evalCode & tmudr::PredicateInfo::EVALUATE_IN_CHILD)
        predsToPushDown += predsOfferedToUDF[p];
    }

  // We have removed unused columns from our ValueIdLists.
  // Remove columns that are not used or not produced in
  // the UDRInvocationInfo as well and
  // trim down the predicate list in the UDRInvocationInfo
  // to contain just those predicates that are evaluated in
  // the UDF itself.
  return TMUDFInternalSetup::removeUnusedColumnsAndPredicates(
       invocationInfo);
}

NABoolean TMUDFDllInteraction::describeConstraints(
     TableMappingUDF * tmudfNode)
{
  tmudr::UDR *udrInterface = tmudfNode->getUDRInterface();
  tmudr::UDRInvocationInfo *invocationInfo = tmudfNode->getInvocationInfo();

  // set up constraint info for child tables
  if (!TMUDFInternalSetup::createConstraintInfoFromRelExpr(tmudfNode))
    return FALSE;

  try
    {
      TMUDFInternalSetup::setCallPhase(
           invocationInfo,
           tmudr::UDRInvocationInfo::COMPILER_CONSTRAINTS_CALL);
      udrInterface->describeConstraints(*invocationInfo);
      TMUDFInternalSetup::resetCallPhase(invocationInfo);
    }
  catch (tmudr::UDRException e)
    {
      processReturnStatus(e, tmudfNode);
      return FALSE;
    }
  catch (...)
    {
      processReturnStatus(
           tmudr::UDRException(
                38900, "Unknown exception during describeConstraints()"),
           tmudfNode);
      return FALSE;
    }

  // translate resulting constraints on result table back into
  // ItemExprs
  if (!TMUDFInternalSetup::createConstraintsFromConstraintInfo(
           invocationInfo->out(),
           tmudfNode,
           CmpCommon::statementHeap()))
    return FALSE;

  return TRUE;
}

NABoolean TMUDFDllInteraction::degreeOfParallelism(
     TableMappingUDF * tmudfNode,
     TMUDFPlanWorkSpace * pws,
     int &dop)
{ 
  tmudr::UDR *udrInterface = tmudfNode->getUDRInterface();
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
           tmudr::UDRInvocationInfo::COMPILER_DOP_CALL);
      udrInterface->describeDesiredDegreeOfParallelism(*invocationInfo,
                                                       *udrPlanInfo);
      TMUDFInternalSetup::resetCallPhase(invocationInfo);
    }
  catch (tmudr::UDRException e)
    {
      processReturnStatus(e, tmudfNode);
      return FALSE;
    }
  catch (...)
    {
      processReturnStatus(
           tmudr::UDRException(
                38900, "Unknown exception during describeDesiredDegreeOfParallelismdescribeParamsAndColumns()"),
           tmudfNode);
      return FALSE;
    }

  dop = udrPlanInfo->getDesiredDegreeOfParallelism();
  
  return TRUE;
}

NABoolean TMUDFDllInteraction::finalizePlan(
     TableMappingUDF * tmudfNode,
     tmudr::UDRPlanInfo *planInfo)
{ 
  tmudr::UDR *udrInterface = tmudfNode->getUDRInterface();
  tmudr::UDRInvocationInfo *invocationInfo = tmudfNode->getInvocationInfo();

  try
    {
      TMUDFInternalSetup::setCallPhase(
           invocationInfo,
           tmudr::UDRInvocationInfo::COMPILER_COMPLETION_CALL);
      udrInterface->completeDescription(*invocationInfo, *planInfo);
      TMUDFInternalSetup::resetCallPhase(invocationInfo);
    }
  catch (tmudr::UDRException e)
    {
      processReturnStatus(e, tmudfNode);
      return FALSE;
    }
  catch (...)
    {
      processReturnStatus(
           tmudr::UDRException(
                38900, "Unknown exception during completeDescription()"),
           tmudfNode);
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
                                              TableMappingUDF *tmudfNode)
{
  // this method is just a shortcut to the more verbose form
  processReturnStatus(
       e,
       tmudfNode->getUserTableName().getExposedNameAsAnsiString().data());
}

void TMUDFDllInteraction::processReturnStatus(const tmudr::UDRException &e, 
                                              const char * routineName,
                                              ComDiagsArea *diags)
{
  const char *sqlState = e.getSQLState();
  NABoolean validSQLState = (strncmp(sqlState, "38", 2) == 0);

  if (diags == NULL)
    diags = CmpCommon::diags();

  if (validSQLState)
    for (int pos=2; pos<5; pos++)
      {
        char ch = sqlState[pos];

        // See ISO/ANSI SQL, subclause 23.1 SQLSTATE
        // - Class 38: External routine exception (see above)
        // - Only digits and simple Latin upper case letters
        //   are allowed in SQLSTATE.
        if (!(ch >= '0' && ch <= '9' ||
              ch >= 'A' && ch <= 'Z'))
          validSQLState = FALSE;
      }
                          
  if (validSQLState)
    {
      // valid SQLSTATE for UDRs, class 38 as defined
      // in the ANSI SQL standard
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
  result->funcType_ = tmudr::UDRInvocationInfo::GENERIC;

  // set user ids
  result->currentUser_ = ComUser::getCurrentUsername();

  if (ComUser::getSessionUser() ==
      ComUser::getCurrentUser())
    result->sessionUser_ = result->currentUser_;
  else
    {
      // session user is different, look it up
      char sessionUsername[MAX_USERNAME_LEN + 1];
      Int32 sessionUserLen;

      if (ComUser::getUserNameFromUserID(
               ComUser::getSessionUser(),
               sessionUsername,
               sizeof(sessionUsername),
               sessionUserLen) == FEOK)
        result->sessionUser_ = sessionUsername;
    }

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

      if (i < result->getFormalParameters().getNumColumns())
        paramName = result->getFormalParameters().getColumn(i).getColName();

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
      const ValueIdList &childOrderBy = childInfo-> getOrderBy();

      success = TMUDFInternalSetup::setTableInfoFromNAColumnArray(
           result->inputTableInfo_[c],
           &childColArray,
           diags);
      if (!success)
        return NULL;

      // add child PARTITION BY syntax
      tmudr::PartitionInfo pi;

      switch (childInfo->getPartitionType())
        {
        case ANY_PARTITIONING:
          pi.setType(tmudr::PartitionInfo::ANY);
          break;

        case SPECIFIED_PARTITIONING:
          {
            const ValueIdSet &childPartBy = childInfo->getPartitionBy();
            const ValueIdList &childCols  = childInfo->getOutputs();

            pi.setType(tmudr::PartitionInfo::PARTITION);

            // translate the value ids into ordinal column numbers
            for (ValueId p=childPartBy.init();
                 childPartBy.next(p);
                 childPartBy.advance(p))
              {
                CollIndex ordinal = childCols.index(p);

                CMPASSERT(ordinal != NULL_COLL_INDEX);
                pi.push_back(ordinal);
              }
          }
          break;

        case REPLICATE_PARTITIONING:
          pi.setType(tmudr::PartitionInfo::REPLICATE);
          break;

        case NO_PARTITIONING:
          pi.setType(tmudr::PartitionInfo::SERIAL);
          break;

        default:
          // leave pi uninitialized
          break;
        }

      result->setChildPartitioning(c, pi);

      if (childOrderBy.entries() > 0)
        {
          // add child ORDER BY syntax
          const ValueIdList &childCols  = childInfo->getOutputs();
          tmudr::OrderInfo orderInfo;

          for (int obc=0; obc<childOrderBy.entries(); obc++)
            {
              // translate the value id into an ordinal column number
              CollIndex ordinal = NULL_COLL_INDEX;
              tmudr::OrderInfo::OrderTypeCode orderCode = tmudr::OrderInfo::ASCENDING;

              if (childOrderBy[obc].getItemExpr()->getOperatorType() == ITM_INVERSE)
                {
                  orderCode = tmudr::OrderInfo::DESCENDING;
                  ordinal = childCols.index(
                       childOrderBy[obc].getItemExpr()->child(0).getValueId());
                }
              else
                ordinal = childCols.index(childOrderBy[obc]);

              CMPASSERT(ordinal != NULL_COLL_INDEX);
              orderInfo.addEntry(ordinal, orderCode);
            }

          result->setChildOrdering(c, orderInfo);
        }
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
                }
              else
                {
                  if (!isDecimalPrecision)
                    tgt.d_.sqlType_ = tmudr::TypeInfo::SMALLINT;
                }
              break;
              
            case 4:
              if (isUnsigned)
                {
                  if (!isDecimalPrecision)
                    tgt.d_.sqlType_ = tmudr::TypeInfo::INT_UNSIGNED;
                }
              else
                {
                  if (!isDecimalPrecision)
                    tgt.d_.sqlType_ = tmudr::TypeInfo::INT;
                }
              break;

            case 8:
              CMPASSERT(!isUnsigned);
              if (!isDecimalPrecision)
                tgt.d_.sqlType_ = tmudr::TypeInfo::LARGEINT;
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
            }
          else
            {
              // note that there is no SQL FLOAT in UDFs, SQL FLOAT
              // gets mapped to REAL or DOUBLE PRECISION
              CMPASSERT(tgt.d_.length_ == 8);
              tgt.d_.sqlType_ = tmudr::TypeInfo::DOUBLE_PRECISION;
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
            break;

          case CharInfo::UTF8:
            tgt.d_.charset_ = tmudr::TypeInfo::CHARSET_UTF8;
            break;

          case CharInfo::UCS2:
            tgt.d_.charset_ = tmudr::TypeInfo::CHARSET_UCS2;
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

NABoolean TMUDFInternalSetup::setPredicateInfoFromValueIdSet(
     tmudr::UDRInvocationInfo *tgt,
     const ValueIdList &udfOutputColumns,
     const ValueIdSet &predicates,
     ValueIdList &convertedPredicates,
     NABitVector &usedColPositions)
{
  tmudr::TableInfo & outputTableInfo = tgt->out();
  int numOutputColumns = outputTableInfo.getNumColumns();
  tmudr::ComparisonPredicateInfo *pi;

  // loop over predicates
  for (ValueId pred=predicates.init(); 
       predicates.next(pred);
       predicates.advance(pred))
    {
      pi = NULL;

      // logic specific to the item expression
      switch (pred.getItemExpr()->getOperatorType())
        {
        case ITM_EQUAL:
        case ITM_LESS:
        case ITM_LESS_EQ:
        case ITM_GREATER:
        case ITM_GREATER_EQ:
          {
            // TBD: Do we need to check for "const op col" as well or
            // has this already been normalized?
            BiRelat *comp = static_cast<BiRelat *>(pred.getItemExpr());
            int columnNum = udfOutputColumns.index(comp->child(0).getValueId());
            NABoolean dummy;
            ConstValue *val = comp->child(1)->castToConstValue(dummy);

            if (val != NULL && columnNum != NULL_COLL_INDEX)
              {
                tmudr::PredicateInfo::PredOperator predOp;

                pi = new tmudr::ComparisonPredicateInfo;

                switch (pred.getItemExpr()->getOperatorType())
                  {
                  case ITM_EQUAL:
                    predOp = tmudr::PredicateInfo::EQUAL;
                    break;
                  case ITM_LESS:
                    predOp = tmudr::PredicateInfo::LESS;
                    break;
                  case ITM_LESS_EQ:
                    predOp = tmudr::PredicateInfo::LESS_EQUAL;
                    break;
                  case ITM_GREATER:
                    predOp = tmudr::PredicateInfo::GREATER;
                    break;
                  case ITM_GREATER_EQ:
                    predOp = tmudr::PredicateInfo::GREATER_EQUAL;
                    break;
                  }
                pi->setOperator(predOp);
                pi->setColumnNumber(columnNum);
                pi->setValue(val->getConstStr().data());
                // mark column used in predicate as used
                usedColPositions += columnNum;
              }
          }
          break;

        case ITM_VEG_PREDICATE:
          {
            VEGPredicate *vegPred = static_cast<VEGPredicate *>(pred.getItemExpr());
            VEG *veg = vegPred->getVEG();
            ValueId constValId = veg->getAConstant();
            ValueIdSet udfOutputColsInVEG(udfOutputColumns);
            ValueId colReferencedInVEG;

            udfOutputColsInVEG.intersectSet(veg->getAllValues());
            udfOutputColsInVEG.getFirst(colReferencedInVEG);

            if (constValId != NULL_VALUE_ID &&
                colReferencedInVEG != NULL_VALUE_ID)
              {
                int colNum = udfOutputColumns.index(colReferencedInVEG);

                // we found a VEG that has a constant member and also
                // one of the TMUDF output columns as a member, add
                // an equals predicate between the two
                CMPASSERT(colNum != NULL_COLL_INDEX);
                pi = new tmudr::ComparisonPredicateInfo;
                pi->setOperator(tmudr::PredicateInfo::EQUAL);
                pi->setColumnNumber(colNum);
                pi->setValue(
                     static_cast<ConstValue *>(constValId.getItemExpr())->
                     getConstStr().data());
                // mark column used in predicate as used
                usedColPositions += colNum;
              }
          }
          break;

        default:
          break;

        } // switch

      if (pi)
        {
          // add this predicate to the invocation info
          tgt->predicates_.push_back(pi);
          // also remember its number, so we can later
          // translate it back
          convertedPredicates.insert(pred);
        }
    } // loop over predicates

  return TRUE;
}

NABoolean TMUDFInternalSetup::removeUnusedColumnsAndPredicates(
     tmudr::UDRInvocationInfo *tgt)
{
  // remove unused output columns

  tmudr::TableInfo &outInfo = tgt->out();
  std::vector<int> outputColMap;
  int numDeletedOutCols = 0;

  // Make a map of current output column numbers to the new state
  // with output columns that are NOT_PRODUCED removed. Note that we
  // keep the columns marked as NOT_USED by the normalizer, since
  // the UDF did not indicate that it is ok to drop such columns.
  for (int c=0; c<outInfo.getNumColumns(); c++)
    if (outInfo.getColumn(c).getUsage() == tmudr::ColumnInfo::NOT_PRODUCED)
      {
        outputColMap.push_back(-1);
        numDeletedOutCols++;
      }
    else
      outputColMap.push_back(c-numDeletedOutCols);

  // remove unused predicates and adjust column numbers of remaining ones
  // to reflect deleted output columns
  std::vector<tmudr::PredicateInfo *>::iterator it = tgt->predicates_.begin();

  while (it != tgt->predicates_.end())
    if ((*it)->getEvaluationCode() == tmudr::PredicateInfo::EVALUATE_IN_UDF)
      {
        // keep this predicate, adjust its column numbers and 
        // move on to the next
        (*it)->mapColumnNumbers(outputColMap);
        it++;
      }
    else
      {
        // delete this predicate, set the iterator to the element following it
        tmudr::PredicateInfo *pi = *it;

        it = tgt->predicates_.erase(it);
        delete pi;
      }

  // remove unused output columns (go backwards)
  for (int oc=outInfo.getNumColumns()-1; oc>=0; oc--)
    if (outInfo.getColumn(oc).getUsage() == tmudr::ColumnInfo::NOT_PRODUCED)
      outInfo.deleteColumn(oc);

  // remove unused columns from the table-valued inputs
  for (int i=0; i<tgt->getNumTableInputs(); i++)
    {
      tmudr::TableInfo &inInfo = tgt->inputTableInfo_[i];
      std::vector<int> childOutputColMap;
      int numDeletedCols = 0;

      // first, make a map of current column numbers of this
      // table-valued input to the new state with unused columns
      // removed
      for (int cc=0; cc<inInfo.getNumColumns(); cc++)
        if (inInfo.getColumn(cc).getUsage() == tmudr::ColumnInfo::USED)
          childOutputColMap.push_back(cc-numDeletedCols);
        else
          {
            childOutputColMap.push_back(-1);
            numDeletedCols++;
          }

      if (numDeletedCols > 0)
        {
          tmudr::PartitionInfo newPartInfo;
          tmudr::OrderInfo newOrderInfo;

          // loop in reverse order and remove unused columns
          for (int ic=inInfo.getNumColumns()-1; ic>=0; ic--)
            if (inInfo.getColumn(ic).getUsage() != tmudr::ColumnInfo::USED)
              inInfo.deleteColumn(ic);

          // adjust column numbers in ORDER BY and PARTITION BY lists,
          // if necessary
          inInfo.getQueryPartitioning().mapColumnNumbers(childOutputColMap);
          inInfo.getQueryOrdering().mapColumnNumbers(childOutputColMap);

          // adjust the column numbers in the provenance info of the outputs
          // for removal of unused columns in the table-valued inputs
          for (int oc=0; oc<outInfo.getNumColumns(); oc++)
            {
              tmudr::ColumnInfo &colInfo = outInfo.getColumn(oc);
              const tmudr::ProvenanceInfo &prov = colInfo.getProvenance();

              if (prov.isFromInputTable(i))
                {
                  int oldColNum = prov.getInputColumnNum();

                  if (childOutputColMap[oldColNum] != oldColNum)
                    // column number is going to change, update provenance
                    colInfo.setProvenance(
                         tmudr::ProvenanceInfo(i, childOutputColMap[oldColNum]));
                }            
            }
        } // numDeletedCols > 0
    } // loop over table-valued inputs

  return TRUE;
}

NABoolean TMUDFInternalSetup::createConstraintInfoFromRelExpr(
     TableMappingUDF * tmudfNode)
{
  tmudr::UDRInvocationInfo *tgt = tmudfNode->getInvocationInfo();

  for (int i=0; i<tmudfNode->getArity(); i++)
    {
      const ValueIdSet &childConstraints =
        tmudfNode->child(i)->getGroupAttr()->getConstraints();

      for (ValueId v=childConstraints.init();
           childConstraints.next(v);
           childConstraints.advance(v))
        {
          switch (v.getItemExpr()->getOperatorType())
            {
            case ITM_CARD_CONSTRAINT:
              {
                long minRows, maxRows;
                CardConstraint *cc =
                  static_cast<CardConstraint *>(v.getItemExpr());

                minRows = cc->getLowerBound();
                maxRows = cc->getUpperBound();
                tgt->inputTableInfo_[i].addCardinalityConstraint(
                     tmudr::CardinalityConstraintInfo(minRows, maxRows));
              }
              break;

            case ITM_UNIQUE_OPT_CONSTRAINT:
              {
                // set of unique columns
                const ValueIdSet &uniqueCols =
                  static_cast<UniqueOptConstraint *>(
                       v.getItemExpr())->uniqueCols();
                // outputs produced by child i
                const ValueIdList &childColList =
                  tmudfNode->getChildInfo(i)->getOutputs();
                ValueIdSet childOutputSet = childColList;

                // cross-check the two
                childOutputSet.intersectSet(uniqueCols);

                if (uniqueCols == childOutputSet)
                  {
                    // we found all the unique columns in the child
                    // outputs (should be the common case), continue

                    tmudr::UniqueConstraintInfo ucInfo;

                    // translate ValueIdSet into a set of ordinal numbers
                    for (ValueId u=uniqueCols.init();
                         uniqueCols.next(u);
                         uniqueCols.advance(u))
                      ucInfo.addColumn(childColList.index(u));

                    tgt->inputTableInfo_[i].addUniquenessConstraint(ucInfo);
                  }
              }
              break;

            default:
              // skip this constraint, it's not handled yet
              break;
            }
        }
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
  tmudr::TypeInfo::SQLTypeCode typeCode = src.getSQLType();

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

NABoolean TMUDFInternalSetup::createConstraintsFromConstraintInfo(
     const tmudr::TableInfo &tableInfo,
     TableMappingUDF * tmudfNode,
     NAHeap *heap)
{
  int numConstraints = tableInfo.getNumConstraints();

  for (int c=0; c<tableInfo.getNumConstraints(); c++)
    switch(tableInfo.getConstraint(c).getType())
      {
      case tmudr::ConstraintInfo::CARDINALITY:
        {
          const tmudr::CardinalityConstraintInfo &cc =
            static_cast<const tmudr::CardinalityConstraintInfo &>(
                 tableInfo.getConstraint(c));
          CardConstraint *ccItem = new(heap)
            CardConstraint(cc.getMinNumRows(),
                           cc.getMaxNumRows());

          // attach it to the group attributes
          tmudfNode->getGroupAttr()->addConstraint(ccItem);
        }
        break;

      case tmudr::ConstraintInfo::UNIQUE:
        {
          const tmudr::UniqueConstraintInfo &uc =
            static_cast<const tmudr::UniqueConstraintInfo &>(
                 tableInfo.getConstraint(c));
          const ValueIdList &udfOutputCols =
            tmudfNode->getProcOutputParamsVids();
          ValueIdSet uniqueValSet;
          int numUniqueCols = uc.getNumUniqueColumns();

          // translate the ordinals of the unique cols into ValueIds
          for (int c=0; c<numUniqueCols; c++)
            uniqueValSet += udfOutputCols[uc.getUniqueColumn(c)];

          // make a new ItemExpr constraint expression from the ValueIdSet
          UniqueOptConstraint *ucItem = new(heap)
            UniqueOptConstraint(uniqueValSet);


          // attach it to the group attributes
          tmudfNode->getGroupAttr()->addConstraint(ucItem);
        }
        break;

      default:
        TMUDFDllInteraction::processReturnStatus(
             tmudr::UDRException(
                  38900,
                  "Encountered invalid constraint type after describeConstraints(): %d",
                  static_cast<int>(tableInfo.getConstraint(c).getType())),
             tmudfNode);
        return FALSE;
      }

  return TRUE;
}

tmudr::UDRPlanInfo *TMUDFInternalSetup::createUDRPlanInfo(
     tmudr::UDRInvocationInfo *invocationInfo)
{
  return new tmudr::UDRPlanInfo(invocationInfo);
}

void TMUDFInternalSetup::setCallPhase(
     tmudr::UDRInvocationInfo *invocationInfo,
     tmudr::UDRInvocationInfo::CallPhase cp)
{
  invocationInfo->callPhase_ = cp;
}

void TMUDFInternalSetup::resetCallPhase(
     tmudr::UDRInvocationInfo *invocationInfo)
{
  invocationInfo->callPhase_ = 
    tmudr::UDRInvocationInfo::UNKNOWN_CALL_PHASE;
}

void TMUDFInternalSetup::setOffsets(tmudr::UDRInvocationInfo *invocationInfo,
                                    ExpTupleDesc *paramTupleDesc,
                                    ExpTupleDesc *outputTupleDesc,
                                    ExpTupleDesc **inputTupleDescs)
{
  if (paramTupleDesc)
    {
      CMPASSERT(invocationInfo->getFormalParameters().getNumColumns() ==
                paramTupleDesc->numAttrs());
      invocationInfo->nonConstFormalParameters().setRecordLength(
           paramTupleDesc->tupleDataLength());
      for (int p=0; p<invocationInfo->getFormalParameters().getNumColumns(); p++)
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
      CMPASSERT(invocationInfo->getFormalParameters().getNumColumns() == 0);
      invocationInfo->nonConstFormalParameters().setRecordLength(0);
    }

  // As we move from compile time to runtime, replace the actual
  // parameter list with the formal parameter list, since we
  // can expect the actual parameters at runtime to have the
  // types of the formal parameters. We should not access
  // formal parameters at runtime.
  // Also erase the compile time constant parameters.
  invocationInfo->nonConstActualParameters().setConstBuffer(0, NULL);

  while (invocationInfo->par().getNumColumns() > 0)
    invocationInfo->nonConstActualParameters().deleteColumn(0);

  while (invocationInfo->getFormalParameters().getNumColumns() > 0)
    {
      invocationInfo->nonConstActualParameters().addColumn(
           invocationInfo->nonConstFormalParameters().getColumn(0));
      invocationInfo->nonConstFormalParameters().deleteColumn(0);
    }

  tmudr::TableInfo &ot = invocationInfo->out();

  if (outputTupleDesc)
    {
      CMPASSERT((outputTupleDesc == NULL &&
                 ot.getNumColumns() == 0) ||
                (ot.getNumColumns() == outputTupleDesc->numAttrs()));
      ot.setRecordLength(outputTupleDesc->tupleDataLength());
      for (int oc=0; oc<ot.getNumColumns(); oc++)
        {
          tmudr::ColumnInfo &colInfo = ot.getColumn(oc);
          Attributes *attr = outputTupleDesc->getAttr(oc);

          colInfo.getType().setOffsets(attr->getNullIndOffset(),
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
              tmudr::ColumnInfo &colInfo = it.getColumn(ic);
              Attributes *attr = inputTupleDesc->getAttr(ic);

              colInfo.getType().setOffsets(attr->getNullIndOffset(),
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
