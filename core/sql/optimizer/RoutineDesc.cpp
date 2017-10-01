/*********************************************************************
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
* File:         RoutineDesc.cpp
* Description:  A routine descriptor
* Created:      6/29/09
* Language:     C++
*
*
**************************************************************************
*/

#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "BindWA.h"
#include "ComOperators.h"
#include "RoutineDesc.h"
#include "ItemColRef.h"
#include "ParNameLocList.h"
#include "Sqlcomp.h"
#include "ex_error.h"
#include "Cost.h"

// -----------------------------------------------------------------------
// Constructors (but note that much more useful stuff goes on in
// static createRoutineDesc in BindRelExpr.C)
// -----------------------------------------------------------------------


// -----------------------------------------------------------------------
//! RoutineDesc::RoutineDesc Constructor 
//  Without any action parameters
//  (but note that much more useful stuff goes on in 
//  populateRoutineDesc() and createRoutineParams()
// -----------------------------------------------------------------------
RoutineDesc::RoutineDesc(BindWA *bindWA, NARoutine *routine)
                : invocationNum_(bindWA->getRoutineInvocationNum()),
                  actionNameAsGiven_("", bindWA->wHeap()),
                  routine_(routine),
                  digits_(-1),
                  locale_(0),
                  colUDFInParamList_(0, bindWA->wHeap()),
                  colUDFOutputList_(0, bindWA->wHeap()),
                  isUUDF_(FALSE),
                  udfOutColStatDescList_ (bindWA->wHeap()),
                  actionOutColStatDescList_ (bindWA->wHeap())

{
   action_=NULL;
   processRoutineCosting(&udfInitialRowCost_, &udfNormalRowCost_, 
                         &udfFanOut_, routine);
}


// -----------------------------------------------------------------------
//! RoutineDesc::RoutineDesc Constructor 
//  With action parameters
// -----------------------------------------------------------------------
RoutineDesc::RoutineDesc(BindWA *bindWA, NARoutine *routine, NARoutine *action)
                : invocationNum_(bindWA->getRoutineInvocationNum()),
                  actionNameAsGiven_("", bindWA->wHeap()),
                  routine_(routine),
                  action_(action),
                  digits_(-1),
                  locale_(0),
                  colUDFInParamList_(0, bindWA->wHeap()),
                  colUDFOutputList_(0, bindWA->wHeap()),
                  isUUDF_(action==NULL? FALSE : TRUE),
                  analysis_(NULL),
                  udfOutColStatDescList_ (bindWA->wHeap()),
                  actionOutColStatDescList_ (bindWA->wHeap())

{
   processRoutineCosting(&udfInitialRowCost_, &udfNormalRowCost_, 
                         &udfFanOut_, routine);
   processRoutineCosting(&actionInitialRowCost_, &actionNormalRowCost_, 
                         &actionFanOut_, action);
}

// -----------------------------------------------------------------------
//! RoutineDesc::RoutineDesc Constructor 
//  Without action parameters
// -----------------------------------------------------------------------
RoutineDesc::RoutineDesc(CollHeap* h, NARoutine *routine)
                : invocationNum_(0),
                  actionNameAsGiven_("", h),
                  routine_(routine),
                  digits_(-1),
                  locale_(0),
                  colUDFInParamList_(0, h),
                  colUDFOutputList_(0, h),
                  isUUDF_(FALSE),
                  udfOutColStatDescList_ (h),
                  actionOutColStatDescList_ (h)
{
   action_=NULL;
   processRoutineCosting(&udfInitialRowCost_, &udfNormalRowCost_, 
                         &udfFanOut_, routine);
}


// -----------------------------------------------------------------------
//! RoutineDesc::RoutineDesc Constructor 
//  With action parameters
// -----------------------------------------------------------------------
RoutineDesc::RoutineDesc(CollHeap* h, NARoutine *routine, NARoutine *action)
                : invocationNum_(0),
                  actionNameAsGiven_("", h),
                  routine_(routine),
                  action_(action),
                  digits_(-1),
                  locale_(0),
                  colUDFInParamList_(0, h),
                  colUDFOutputList_(0, h),
                  isUUDF_(action==NULL? FALSE : TRUE),
                  analysis_(NULL),
                  colActionInParamList_(0, h),
                  colActionOutputList_(0, h),
                  udfOutColStatDescList_ (h),
                  actionOutColStatDescList_ (h)
{}

// -----------------------------------------------------------------------
//! RoutineDesc::RoutineDesc Copy Constructor 
// -----------------------------------------------------------------------
RoutineDesc::RoutineDesc(const RoutineDesc &rdesc, CollHeap *h)
                : invocationNum_(rdesc.invocationNum_),
                  actionNameAsGiven_(rdesc.actionNameAsGiven_, h),
                  routine_(rdesc.routine_), // Pointer to cached object
                  analysis_(NULL),
                  action_(rdesc.action_),   // Pointer to cached object
                  digits_(rdesc.digits_),   
                  locale_(rdesc.locale_),
                  isUUDF_(rdesc.isUUDF_),
                  colUDFInParamList_(rdesc.colUDFInParamList_, h),
                  colUDFOutputList_(rdesc.colUDFOutputList_, h),
                  udfFanOut_(rdesc.udfFanOut_),
                  udfInitialRowCost_ (rdesc.udfInitialRowCost_),
                  udfNormalRowCost_ (rdesc.udfNormalRowCost_),
                  udfOutColStatDescList_ (rdesc.udfOutColStatDescList_,h),
                  colActionInParamList_(rdesc.colActionInParamList_, h),
                  colActionOutputList_(rdesc.colActionOutputList_, h),
                  actionFanOut_(rdesc.actionFanOut_),
                  actionInitialRowCost_ (rdesc.actionInitialRowCost_),
                  actionNormalRowCost_ (rdesc.actionNormalRowCost_),
                  actionOutColStatDescList_ (rdesc.actionOutColStatDescList_,h)
{}

// -----------------------------------------------------------------------
// ! Print function for RoutineDesc
// -----------------------------------------------------------------------
void RoutineDesc::print(FILE* ofd, const char* indent, const char* title)
{
#ifndef NDEBUG
  BUMP_INDENT(indent);
  cout << title << " " << this << " NARoutine=" << (void*)routine_
        << " actionNARoutine=" << (void*)action_ << " invocation=" 
        << invocationNum_ << " isUUDF_ =" << isUUDF_ 
        << "actionNameAsGiven=" << actionNameAsGiven_ << endl;
  ValueIdList emptySelectList;
  colUDFInParamList_.print(ofd, indent, "RoutineDesc::colUDFInParamList_");
  colUDFOutputList_.print(ofd, indent, "RoutineDesc::colUDFOutputList_");
  colActionInParamList_.print(ofd, indent, "RoutineDesc::colActionaramList_");
  colActionOutputList_.print(ofd, indent, "RoutineDesc::colActionputList_");
  udfOutColStatDescList_.print(emptySelectList, ofd, indent, "RoutineDesc::udfOutColStatDescList_");
  actionOutColStatDescList_.print(emptySelectList, ofd, indent, "RoutineDesc::actionOutColStatDescList_");

#endif
} // RoutineDesc::print()


// -----------------------------------------------------------------------
// ! getExternalName function for RoutineDesc
// -----------------------------------------------------------------------
const ComString &RoutineDesc::getExternalName() const
{ 
  if (isUUDF_) 
  { 
    return action_->getExternalName(); 
  } 
  else 
  { 
    return  routine_->getExternalName(); 
  } 
} // RoutineDesc::getExternalName()


// -----------------------------------------------------------------------
// ! createRoutineParams function for RoutineDesc
// Instantiates Formal Routines parameters, binds them and adds them to 
// the appropriate input/output list depending on the parameter type.
// -----------------------------------------------------------------------

NABoolean RoutineDesc::createRoutineParams(BindWA *bindWA, RDParamHandling pHandling) 
{

  NARoutine *routine = NULL;
  ValueIdList *inParamList = NULL;
  ValueIdList *outputList = NULL;

  switch ( pHandling ) 
  {
     case RD_UDFONLY:
             routine = routine_;
             inParamList = &colUDFInParamList_;
             outputList = &colUDFOutputList_;
             break;
     case RD_ACTIONONLY:
             routine = action_;
             inParamList = &colActionInParamList_;
             outputList = &colActionOutputList_;
             break;
     case RD_AUTOMATIC:
             routine = isUUDF_ ? action_ : routine_;
             inParamList = isUUDF_ ? &colActionInParamList_ : &colUDFInParamList_;
             outputList = isUUDF_ ? &colActionOutputList_ : &colUDFOutputList_;
             break;
  }

  if (routine == NULL ) return FALSE;

  
  // Depending on the state of the isUUDF flag, we will either
  // access the UDF or the Action lists.

  
      
  for (CollIndex i = 0; i < (CollIndex ) routine->getParamCount(); i++) {
     if (createRoutineParam(bindWA, i, routine, inParamList, outputList) == FALSE)
       return FALSE;
  }
  return TRUE;
}

// -----------------------------------------------------------------------
// ! createRoutineParam function for RoutineDesc
// Instantiates a Formal Routines parameter, binds it and adds it to 
// the appropriate input/output list depending on the parameter type.
// -----------------------------------------------------------------------

NABoolean RoutineDesc::createRoutineParam(BindWA *bindWA, CollIndex i, 
                                          NARoutine *routine,
                                          ValueIdList *inParamList,
                                          ValueIdList *outputList)
{
    if (!routine || !inParamList || !outputList) return FALSE;
    if (i < 0 || i >= (CollIndex) routine->getParamCount()) return FALSE;

    const NAColumnArray & params = routine->getParams();
    ColStatDescList &colStsDescList = getEffOutParamColStatDescList();

    RoutineParam *routineParam = new (bindWA->wHeap()) RoutineParam(params[i],
                                                                    i, 
                                                                    this);

    if ( routineParam == NULL ) return FALSE;


    if ((params[i]->getColName() == "INVAL" ) ||
       (params[i]->getColName() == "" ))
       {

          NAString tmpStr(CmpCommon::statementHeap());
          char num[50];

          tmpStr += routine->getExternalName();
          if (routineParam->getParamMode() == COM_OUTPUT_COLUMN)
             tmpStr += "_OUTPUT_";
          else
             tmpStr += "_INPUT_";
          sprintf(num, "%d", i);
          tmpStr += num;
          routineParam->setName(tmpStr);
       }

    routineParam->bindNode(bindWA);
   
    if (bindWA->errStatus()) return FALSE;

    const char *colName = routineParam->getName();
    const Int32  nameLen = routineParam->getName().length();
   
    UInt32 numUecs = routine->getUecValues().entries();
    HistogramSharedPtr emptyHist(new (HISTHEAP) Histogram(HISTHEAP));

    // Set up the costing for the output column 
    CostScalar fanOut(routine->getUdfFanOut());

    if (routineParam->getParamMode() == COM_OUTPUT_COLUMN)
    {
       outputList->insert(routineParam->getValueId());

       //  For now we only setup costing information for UDFs

       if (numUecs != 0 && (i <= numUecs))
       {
         ComUID id(ColStats::nextFakeHistogramID());

          ColStatsSharedPtr outputStats(
                            new (HISTHEAP) ColStats(id,
                                                    routine->getUecValues()[i],
                                                    fanOut,
                                                    fanOut,
                                                    FALSE,
                                                    FALSE,
                                                    emptyHist,
                                                    FALSE,
                                                    1.0,
                                                    1.0,
                                                    0,
                                                    HISTHEAP, 
                                                    TRUE// allowMinusOne
                                                    ));
      
            // Setting this flag will ensure that the compiler does not start
            // to look for this column name in the NAColumn. As there does not
            // exist a column for a constant
          outputStats->setVirtualColForHist(TRUE);
          outputStats->setIsCompressed(TRUE);
      
          ColStatDescSharedPtr tmpColStatDescPtr(new (HISTHEAP) 
                                                   ColStatDesc(outputStats, 
                                                      routineParam->getValueId(),
                                                      HISTHEAP), HISTHEAP);
   
          colStsDescList.insert(tmpColStatDescPtr);
       }
    }
    else
    {
          inParamList->insert(routineParam->getValueId());
    }
  return TRUE;
} // RoutineDesc::createRoutineParam()


// -----------------------------------------------------------------------
// ! populateRoutineDesc() function for RoutineDesc
// Instantiates a Formal Routines parameter, binds it and adds it to 
// the appropriate input/output list depending on the parameter type.
// -----------------------------------------------------------------------
NABoolean RoutineDesc::populateRoutineDesc(BindWA *bindWA, 
                                              NABoolean createRETDesc)
{


  // The input and output parameters will be mapped in from naRoutine or
  // actionNaRoutine. The RoutineDesc holds an instance for each. 
  // This means that if we have an Action, its inputs and outputs will 
  // overlay those of the UDF itself.

    // Instantiate the Formal parameters for the UDF
    // For each NAColumn, allocate a RoutineParam, bind the RoutineParam,
    // and add the ValueId to the RoutineDesc.

  if ( createRoutineParams(bindWA, RD_UDFONLY ) == FALSE)
  {
    bindWA->setErrStatus();
    return FALSE;
  }

    // Instantiate the Formal parameters for the Action
  if (isUUDF_ == TRUE)
  {
     if ( createRoutineParams(bindWA, RD_ACTIONONLY) == FALSE)
     {
       bindWA->setErrStatus();
       return FALSE;
     }
  }


     // Allocate a RETDesc, attach it to the BindScope. This should
     // only be done for RelExprs..
  if (createRETDesc == TRUE) 
  {
    bindWA->getCurrentScope()->setRETDesc(new (bindWA->wHeap())
       RETDesc(bindWA, this));

  }


  return TRUE;

} // RoutineDesc::populateRoutineDesc()


// -----------------------------------------------------------------------
// ! processRoutineCosting() function for RoutineDesc
// Process the costVectors of the UDF, or Action. The cost data is
// copied over from the NARoutine and we apply DEFAULTs to them if they 
// are out of range.
// -----------------------------------------------------------------------
void RoutineDesc::processRoutineCosting(SimpleCostVector *initial, 
                                        SimpleCostVector *normal, 
                                        Int32 *fanOut, 
                                        NARoutine *routine)
{

  if (routine == NULL ) return;

  // Initialize the cost vectors
  *initial = routine->getInitialRowCostVector();
  *normal = routine->getNormalRowCostVector();
  *fanOut = routine->getUdfFanOut();

  // Check to see if we need to use costs from CQDs

  if (initial->getCPUTime() == csMinusOne )
  {
     CostScalar initCpuCost((double) (ActiveSchemaDB()->getDefaults()).
                                     getAsULong(INITIAL_UDF_CPU_COST)
                           );
     initial->setCPUTime(initCpuCost);
  }

  if (initial->getIOTime() == csMinusOne)
  {
     CostScalar initIOCost((double) (ActiveSchemaDB()->getDefaults()).
                                    getAsULong(INITIAL_UDF_IO_COST)
                          );
     initial->setIOTime(initIOCost);
  }

  if (initial->getMessageTime() == csMinusOne)
  {
     CostScalar initMsgCost((double) (ActiveSchemaDB()->getDefaults()).
                                     getAsULong(INITIAL_UDF_MSG_COST)
                           );
     initial->setMSGTime(initMsgCost);
  }


  if (normal->getCPUTime() == csMinusOne )
  {
     CostScalar initCpuCost((double) (ActiveSchemaDB()->getDefaults()).
                                     getAsULong(NORMAL_UDF_CPU_COST)
                           );
     normal->setCPUTime(initCpuCost);
  }

  if (normal->getIOTime() == csMinusOne)
  {
     CostScalar initIOCost((double) (ActiveSchemaDB()->getDefaults()).
                                    getAsULong(NORMAL_UDF_IO_COST)
                          );
     normal->setIOTime(initIOCost);
  }

  if (normal->getMessageTime() == csMinusOne)
  {
     CostScalar initMsgCost((double) (ActiveSchemaDB()->getDefaults()).
                                     getAsULong(NORMAL_UDF_MSG_COST)
                           );
     normal->setMSGTime(initMsgCost);
  }

  if ( *fanOut == -1)
  {
      *fanOut = (ActiveSchemaDB()->getDefaults()).getAsULong(UDF_FANOUT);
  }
} // RoutineDesc::processRoutineCosting()
