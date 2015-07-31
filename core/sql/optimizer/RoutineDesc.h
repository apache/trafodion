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
#ifndef ROUTINEDESC_H
#define ROUTINEDESC_H
/* -*-C++-*-
**************************************************************************
*
* File:         RoutineDesc.h
* Description:  A routine descriptor
* Created:      6/29/09
* Language:     C++
*
*************************************************************************
*/


#include "BaseTypes.h"
#include "ComSmallDefs.h"
#include "ObjectNames.h"
#include "ItemConstr.h"
#include "ValueDesc.h"
#include "ColStatDesc.h"
#include "CostVector.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class RoutineDesc;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class NARoutine;
class RoutineAnalysis;

/*!
*  \brief RoutineDesc Class.
*         The RoutineDesc describes an instance of a Routine similarly
*         to how the TableDesc is used to represent an instance of a table.
*
*  One RoutineDesc is allocated per innvokation of a UDF.
*  One or more RoutineDescs may share a NARoutine. 
*  A RoutineDesc contains some attributes for the Routine that are specific 
*  to a particular innvocation.
*  In particular it holds the RoutineParam ItemExpr representation of the
*  formal parameters and outputs of the routine.
*  If the Routine represented is a scalar routine the RoutineDesc is all
*  we need, however, if the routine is a table type routine, we will also
*  allocate a TableDesc describing a virtual table that represents the 
*  results from the Routine.
* 
*/ 


//! RoutineDesc Class
class RoutineDesc : public NABasicObject
{
public:

  // ---------------------------------------------------------------------
  // Enums for paramaeter processing
  // ---------------------------------------------------------------------
  enum RDParamHandling
  {
     RD_UDFONLY     // process parameters for UDF only
    ,RD_ACTIONONLY  // process parameters for Action only
    ,RD_AUTOMATIC   // process parameters based on state of isUUDF_ flag
  };
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------

  //! RoutineDesc Constructor
  //  Without an Action
  RoutineDesc(BindWA *bindWA, NARoutine *routine) ;

  //! RoutineDesc Constructor
  RoutineDesc(BindWA *bindWA, NARoutine *routine, NARoutine *action) ;

  //! RoutineDesc Constructor that doesn't require a BindWA
  //  Without an Action
  RoutineDesc(CollHeap* h, NARoutine *routine); 

  //! RoutineDesc Constructor that doesn't require a BindWA
  RoutineDesc(CollHeap* h, NARoutine *routine, NARoutine *action) ;

  //! RoutineDesc Copy Constructor 
  RoutineDesc (const RoutineDesc &, CollHeap *h);
public:

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  //! getInvocationNum accessor function
  inline const Int32 &getInvocationNum() const       { return invocationNum_; }

  //! getActionQualifiedNameObj accessor function
  inline const NAString &getActionNameAsGiven() const{ return actionNameAsGiven_; }

  //! getQualifiedNameObj accessor function
  inline NAString &getActionNameAsGiven()            { return actionNameAsGiven_; }


  //! getInvocationNum accessor function
  inline const Int32 getDigits() const               { return digits_; }

  //! getLocale accessor function
  inline const Int32 &getLocale() const              { return locale_; }

  //! getNARoutine accessor function
  inline NARoutine *getNARoutine() const             { return routine_; }

  //! getNARoutineRW accessor function
  inline NARoutine *getNARoutineRW()                 { return routine_; }

  //! getActionNARoutine accessor function
  inline NARoutine *getActionNARoutine() const       { return action_; }

  //! getActionNARoutineRW accessor function
  inline NARoutine *getActionNARoutineRW()           { return action_; }

  //! getEffectiveNARoutine accessor function
  inline NARoutine *getEffectiveNARoutine() const    
         { return isUUDF_ ? action_ : routine_; }

  const RoutineAnalysis *getRoutineAnalysis() const
         { return analysis_; }
  
  //! getExternalName accessor function
  const ComString &getExternalName() const ;

  //! getInParamColumnList accessor function
  //  returns a constant reference to a list of formal input parameters
  inline const ValueIdList &getInParamColumnList() const       
         { return isUUDF_ ? colActionInParamList_ : colUDFInParamList_; }


  //! getInParamColumnList accessor function
  //  returns a list of formal input parameters
  inline ValueIdList &getInParamColumnList() 
         { return isUUDF_ ? colActionInParamList_ : colUDFInParamList_; }

  //! getOutputColumnList accessor function
  //  returns a constant reference to a list of formal output parameters
  inline const ValueIdList &getOutputColumnList() const      
         { return isUUDF_? colActionOutputList_ : colUDFOutputList_; }

  //! getOutputColumnList accessor function
  //  returns a reference to a list of formal output parameters
  inline ValueIdList &getOutputColumnList()
         { return isUUDF_ ? colActionOutputList_ : colUDFOutputList_; }


  // Accessor functions that looks at the UDF formal parameters 
  //! getUDFInParamColumnList accessor function
  //  returns a constant reference to a list of formal input parameters
  inline const ValueIdList &getUDFInParamColumnList() const       
         { return colUDFInParamList_; }

  //! getUDFInParamColumnList accessor function
  //  returns a list of formal input parameters
  inline ValueIdList &getUDFInParamColumnList() 
         { return colUDFInParamList_; }

  //! getUDFInParamColumnListCount accessor function
  //  returns a count of formal input parameters
  inline const ComUInt32 getUDFInParamColumnListCount() const       
         { return colUDFInParamList_.entries(); }

  //! getUDFOutputColumnList accessor function
  //  returns a constant reference to a list of formal output parameters
  inline const ValueIdList &getUDFOutputColumnList() const      
         { return colUDFOutputList_; }

  //! getUDFOutputColumnList accessor function
  //  returns a reference to a list of formal output parameters
  inline ValueIdList &getUDFOutputColumnList()
         { return colUDFOutputList_; }

  //! getUDFOutputParamColumnListCount accessor function
  //  returns a list of formal input parameters
  inline const ComUInt32 getUDFOutputColumnListCount() const       
         { return colUDFOutputList_.entries(); }

  // Accessor functions that looks at the Action formal parameters 
  //! getActionInParamColumnList accessor function
  //  returns a constant reference to a list of formal input parameters
  inline const ValueIdList &getActionInParamColumnList() const       
         { return colActionInParamList_; }

  //! getActionInParamColumnList accessor function
  //  returns a list of formal input parameters
  inline ValueIdList &getActionInParamColumnList() 
         { return colActionInParamList_; }

  //! getActionInParamColumnListCount accessor function
  //  returns a count of formal input parameters
  inline const ComUInt32 getActionInParamColumnListCount() const       
         { return colActionInParamList_.entries(); }

  //! getActionOutputColumnList accessor function
  //  returns a constant reference to a list of formal output parameters
  inline const ValueIdList &getActionOutputColumnList() const      
         { return colActionOutputList_; }

  //! getActionOutputColumnList accessor function
  //  returns a reference to a list of formal output parameters
  inline ValueIdList &getActionOutputColumnList()
         { return colActionOutputList_; }

  //! getActionOutputParamColumnListCount accessor function
  //  returns a list of formal input parameters
  inline const ComSInt32 getActionOutputParamColumnListCount() const       
         { return colActionOutputList_.entries(); }

  //! isUUDFRoutine accessor function
  //  returns a NABoolean indicating if this Routine is a UUDF type
  inline const NABoolean   isUUDFRoutine() const       { return isUUDF_; }

  //! getUDFOutParamColStatDescList accessor function
  //  returns a pointer to a StatsList holding UEC values for each
  //  output column.
  inline ColStatDescList  &getUdfOutParamColStatDescList() 
         { return udfOutColStatDescList_; }

  //! getUDFInitialRowCostVector accessor function
  inline SimpleCostVector  &getUdfInitialRowCostVector() 
         { return udfInitialRowCost_; }

  //! getUDFNormalRowCostVector accessor function
  inline SimpleCostVector  &getUdfNormalRowCostVector()  
         { return udfNormalRowCost_; }

  //! getUDFFanOut accessor function
  inline Int32  &getUdfFanOut()  { return udfFanOut_; }

  //! getActionOutParamColStatDescList accessor function
  //  returns a pointer to a StatsList holding UEC values for each
  //  output column.
  inline ColStatDescList  &getActionOutParamColStatDescList() 
         { return actionOutColStatDescList_; }

  //! getActionInitialRowCostVector accessor function
  inline SimpleCostVector  &getActionInitialRowCostVector() 
         { return actionInitialRowCost_; }

  //! getActionNormalRowCostVector accessor function
  inline SimpleCostVector  &getActionNormalRowCostVector()  
         { return actionNormalRowCost_; }

  //! getActionFanOut accessor function
  inline Int32  &getActionFanOut()  { return actionFanOut_; }

  //! getEffOutParamColStatDescList accessor function
  //  returns a pointer to a StatsList holding UEC values for each
  //  output column.
  inline ColStatDescList  &getEffOutParamColStatDescList() 
         { return isUUDF_ == FALSE ? 
                          udfOutColStatDescList_ : actionOutColStatDescList_; }

  //! getEffInitialRowCostVector accessor function
  inline SimpleCostVector  &getEffInitialRowCostVector() 
         { return isUUDF_ == FALSE ? 
                          udfInitialRowCost_ : actionInitialRowCost_; }

  //! getEffNormalRowCostVector accessor function
  inline SimpleCostVector  &getEffNormalRowCostVector()  
         { return isUUDF_ == FALSE ? udfNormalRowCost_ : actionNormalRowCost_; }

  //! getEffFanOut accessor function
  inline Int32  &getEffFanOut()  
         { return isUUDF_ == FALSE ? udfFanOut_ : actionFanOut_; }

  // ---------------------------------------------------------------------
  // Mutator functions
  // ---------------------------------------------------------------------

  //! setActionQualName mutator function
  //  sets the Routine's Qualified Name
  inline void setActionNameAsGiven(const NAString &name)  { actionNameAsGiven_ = name; }

  //! setDigits mutator function
  //  sets the digits for a SAS_PUT Action
  inline void setDigits(const Int32 digits) { digits_ = digits; }

  //! setLocale mutator function
  //  sets the locale string for a SAS_PUT Action
  inline void setLocale(const Int32 locale) { locale_ = locale; }

  //! setInvocationNum mutator function
  //  sets the invocation number of the Routine
  inline void setInvocationNum(const Int32 invocationNum) { invocationNum_ = invocationNum; }

  //! setNARoutine mutator function
  //  sets the pointer to the Routine's NARoutine
  //  and process the Costing information
  inline void setNARoutine(NARoutine * routine)           
         { routine_ = routine; 
           processRoutineCosting(&udfInitialRowCost_, 
                                 &udfNormalRowCost_,
                                 &udfFanOut_,
                                 routine);
         }

  //! setActionNARoutine mutator function
  //  sets the pointer to the Routine's Action NARoutine
  //  and process the Costing information
  inline void setActionNARoutine(NARoutine * action)      
         { isUUDF_ = TRUE; 
           action_ = action; 
           processRoutineCosting(&actionInitialRowCost_, 
                                 &actionNormalRowCost_,
                                 &actionFanOut_,
                                 action);
         }

   //! setRoutineAnalysis method to assing the Analysis struct
  void setRoutineAnalysis(RoutineAnalysis *analysis)      {analysis_ = analysis; }
 
  //! populateRoutineDesc method create a routineDesc. The createRETDesc 
  //  flag should be set to FALSE when this routine is used to create a 
  //  routineDesc for UDFunction.  When used to create a routineDesc for 
  //  RelExprs in the binder, the flag should be set to TRUE!
  // --------------------------------------------------------------------
  NABoolean populateRoutineDesc(BindWA *bindWA, 
                                 NABoolean createRETDesc=FALSE);

  //! createRoutineParams mutator function
  //  Instantiates the Formal parameters described in the NARoutine
  //  Binds them and assigns them to the In or Out Params list depending on
  //  its type.
  NABoolean createRoutineParams(BindWA *bindWA, RDParamHandling pHandling);
  NABoolean createRoutineParam(BindWA *bindWA, CollIndex i, NARoutine *routine,
                               ValueIdList *inParamList, ValueIdList *outputList);

  // ---------------------------------------------------------------------
  // Needed by Collections classes
  // ---------------------------------------------------------------------

  //! == operator mutator function
  //  redefines the == operator.  
  NABoolean operator == (const RoutineDesc & rhs) { return (&(*this) == &rhs); }


  // ---------------------------------------------------------------------
  // Print/debug
  // ---------------------------------------------------------------------
  //! print function
  //  used for debugging
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "RoutineDesc");



private:
  // Default constructor, copy constructor without heap, and asignment
  // operator are not implemented and not allowed.
  RoutineDesc(); 
  RoutineDesc(const RoutineDesc &);
  RoutineDesc &operator=(RoutineDesc &rdesc); 

  //! processRoutineCosting mutator function
  // copies the cost data from the NARoutine to the routineDesc and
  // applies DEFAULTS if necessary.
  void processRoutineCosting(SimpleCostVector *initial,
                                          SimpleCostVector *normal,
                                          Int32 *fanOut,
                                          NARoutine *routine);

  // ---------------------------------------------------------------------
  //! The routine invocation number 
  //    This number is meant to be used to differentiate multiple 
  //    instances of the same routine in the query. 
  // ---------------------------------------------------------------------
  Int32 invocationNum_;

  // ---------------------------------------------------------------------
  //! The action name
  //  As found in the sql text
  // ---------------------------------------------------------------------
  NAString actionNameAsGiven_;

  // ---------------------------------------------------------------------
  //! The digits following an action name 
  //    This number is meant to be used to drive different format outputs
  //    for SAS_PUT
  // ---------------------------------------------------------------------
  Int32 digits_;

  // ---------------------------------------------------------------------
  //! The optional locale information for an action 
  // ---------------------------------------------------------------------
  Int32 locale_;

  // ---------------------------------------------------------------------
  //! NARoutine object representing the Routine
  // ---------------------------------------------------------------------
  NARoutine *routine_;

  // ---------------------------------------------------------------------
  //! NARoutine object representing the Action of a UUDF Routine
  // ---------------------------------------------------------------------
  NARoutine *action_;

  // ---------------------------------------------------------------------
  //! A List of ValueIds that contains the identifers for the columns
  // representing input paramaters provided by this innvocation to the 
  // UUDF routine NARoutine.
  //
  // The accessor functions above will access the correct list depending
  // on if the UUDF flags is set.
  // ---------------------------------------------------------------------
  ValueIdList  colUDFInParamList_;

  // ---------------------------------------------------------------------
  //! A List of ValueIds that contains the identifers for the columns
  // representing ouputs provided by this innvocation to the NARoutine.
  // ---------------------------------------------------------------------
  ValueIdList  colUDFOutputList_;

  // ---------------------------------------------------------------------
  //! A List of ValueIds that contains the identifers for the columns
  // representing input paramaters provided by this innvocation to the 
  // Action routine ActionNARoutine.
  //
  // The accessor functions above will access the correct list depending
  // on if the UUDF flags is set.
  // ---------------------------------------------------------------------
  ValueIdList  colActionInParamList_;

  // ---------------------------------------------------------------------
  //! A List of ValueIds that contains the identifers for the columns
  // representing ouputs provided by this innvocation to the ActionNARoutine.
  // ---------------------------------------------------------------------
  ValueIdList  colActionOutputList_;

  // ---------------------------------------------------------------------
  //! A Boolean to indicate if the Routine is a UUDF. If it is, it has a
  // second NARoutine describing the action.
  // ---------------------------------------------------------------------
  NABoolean isUUDF_;

  // ---------------------------------------------------------------------
  //! A ColStatsDesc udfOutColStatDescList_. Used to hold the UECs for 
  //  each output of the UDF
  // ---------------------------------------------------------------------
  ColStatDescList udfOutColStatDescList_;

  // ---------------------------------------------------------------------
  //! A SimpleCostVector to hold the initial cost per UDF instance. Needed
  //  a per instance one since CQD defaults needed to be checked at bind time
  //  for each query, not just when the NARoutine was instantiated.
  // ---------------------------------------------------------------------
  SimpleCostVector     udfInitialRowCost_;

  // ---------------------------------------------------------------------
  //! A SimpleCostVector to hold the normal cost per UDF instance. Needed
  //  a per instance one since CQD defaults needed to be checked at bind time
  //  for each query, not just when the NARoutine was instantiated.
  // ---------------------------------------------------------------------
  SimpleCostVector     udfNormalRowCost_;
  RoutineAnalysis * analysis_;

  // ---------------------------------------------------------------------
  //! A Int32 to hold the fanout per routine instance. Needed
  //  a per instance one since CQD defaults needed to be checked at bind time
  //  for each query, not just when the NARoutine was instantiated.
  // ---------------------------------------------------------------------
  Int32     udfFanOut_;

  // ---------------------------------------------------------------------
  //! A ColStatsDesc actionOutColStatDescList_. Used to hold the UECs for 
  //  each output of the Action
  // ---------------------------------------------------------------------
  ColStatDescList actionOutColStatDescList_;

  // ---------------------------------------------------------------------
  //! A SimpleCostVector to hold the initial cost per Action instance. Needed
  //  a per instance one since CQD defaults needed to be checked at bind time
  //  for each query, not just when the NARoutine was instantiated.
  // ---------------------------------------------------------------------
  SimpleCostVector     actionInitialRowCost_;

  // ---------------------------------------------------------------------
  //! A SimpleCostVector to hold the normal cost per Action instance. Needed
  //  a per instance one since CQD defaults needed to be checked at bind time
  //  for each query, not just when the NARoutine was instantiated.
  // ---------------------------------------------------------------------
  SimpleCostVector     actionNormalRowCost_;

  // ---------------------------------------------------------------------
  //! A Int32 to hold the fanout per Action instance. Needed
  //  a per instance one since CQD defaults needed to be checked at bind time
  //  for each query, not just when the NARoutine was instantiated.
  // ---------------------------------------------------------------------
  Int32     actionFanOut_;
}; // class RoutineDesc


#endif  /* ROUTINEDESC_H */
