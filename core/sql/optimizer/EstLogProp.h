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
#ifndef ESTLOGPROP_HDR
#define ESTLOGPROP_HDR
/* -*-C++-*-
**************************************************************************
*
* File:         EstLogProp.h
* Description:  This file includes the class definition for 
*               estimated logical properties.
* Created:      April 1, 1994
* Language:     C++
*
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------
//  Due to the order of declarations in other headers, the following
//  include and declaration must come before including the other headers.
// -----------------------------------------------------------------------
#include "SharedPtr.h"
class EstLogProp;
typedef IntrusiveSharedPtr<EstLogProp> EstLogPropSharedPtr;

// -----------------------------------------------------------------------
//  Include Files
// -----------------------------------------------------------------------
#include "BaseTypes.h"
#include "ColStatDesc.h"
#include "OperTypeEnum.h"
#include "RelJoin.h"
#include "CompException.h"

// Forward class declaration
class CANodeIdSet;

// -----------------------------------------------------------------------
//  Estimated Logical Properties
// -----------------------------------------------------------------------
class EstLogProp : public NABasicObject
{
public:
  INTRUSIVE_SHARED_PTR(EstLogProp);


  // ---------------------------------------------------------------------
  // In order to handle ANTI_SEMI_JOINS, we need the inputForSemiTSJ flag
  // to be an enum, not just an NABoolean
  // ---------------------------------------------------------------------
  enum SemiTSJEnum { NOT_SEMI_TSJ=FALSE,    // previous value of "FALSE"
                     SEMI_TSJ,              // previous value of "TRUE"
                     ANTI_SEMI_TSJ } ;      // new value : for case of AntiSemiJoin

  // ---------------------------------------------------------------------
  //  Constructor, copy constructor, and assignment operator
  // ---------------------------------------------------------------------
  EstLogProp (CostScalar card = -1,
              ValueIdSet *preds = NULL,
              SemiTSJEnum inputForSemiTSJ = NOT_SEMI_TSJ,
	      CANodeIdSet *nodeSet = NULL,
	      NABoolean cacheable = FALSE,
              NAMemory * h=HISTHEAP);
  EstLogProp (const EstLogProp &other, NAMemory * h=HISTHEAP);
  EstLogProp & operator= (const EstLogProp &other);
  NABoolean operator == (const EstLogProp & other) const;

  // ---------------------------------------------------------------------
  //  Destructor
  // ---------------------------------------------------------------------
  ~EstLogProp();

  // ---------------------------------------------------------------------
  //  Accessor Functions
  // ---------------------------------------------------------------------
  CostScalar getResultCardinality() const 
  { CCMPASSERT (resultCardinality_ >= 0) ; 
    return resultCardinality_; 
  }
  void setResultCardinality(CostScalar v)
  { CCMPASSERT (v >= 0) ;
    v.round();
    v.minCsOne();
    if ( v.getValue() > COSTSCALAR_MAX )
      resultCardinality_ = COSTSCALAR_MAX;
    else
      resultCardinality_ = v; 
  }   

  // big memory growth percent (to be used by SSD overlow enhancement project)
  short getBMOgrowthPercent() const
    { return 
        RelExpr::bmoGrowthPercent(getResultCardinality(), getMaxCardEst());
    }

  CostScalar getMaxCardEst() const 
    { //CCMPASSERT (maxCardinality_ >= 0) ; 
      return maxCardinality_; 
    }

  void setMaxCardEst(CostScalar v)
    { //CCMPASSERT (v >= 0) ; 
      if ( v.getValue() > COSTSCALAR_MAX )
        maxCardinality_ = COSTSCALAR_MAX;
      else
        maxCardinality_ = v; 
    }   

  inline ColStatDescList &colStats()              { return columnStats_; }
  inline const ColStatDescList &getColStats() const  
                                                  { return columnStats_; }

  inline const ValueIdSet &getUnresolvedPreds() const 
                                                {return unresolvedPreds_;}
  inline ValueIdSet &unresolvedPreds()          {return unresolvedPreds_;}

  inline SemiTSJEnum inputForSemiTSJ()          { return inputForSemiTSJ_; }
  inline SemiTSJEnum getInputForSemiTSJ() const  
                                              { return inputForSemiTSJ_; }
  inline  CANodeIdSet* getNodeSet()  const    {return nodeSet_;}
  inline void setNodeSet(CANodeIdSet* vset)
                                              { nodeSet_ = vset; }

  inline void setCacheableFlag(NABoolean flag)
					      { cacheable_ = flag; }
  inline NABoolean isCacheable() const {return cacheable_;}


  //++MV
  inline NABoolean isCardinalityEqOne() const { return isCardinalityEqOne_; }
  inline void setCardinalityEqOne() { isCardinalityEqOne_ = TRUE; }
  inline void resetCardinalityEqOne() { isCardinalityEqOne_ = FALSE; }
  //--MV

  // Returns the (possibly multi-column) uec value for the
  // ValueId-specified column(s) in the parameter list.
  //
  // NB: in the case where we cannot find uec information for one or more
  // of 'columns' (i.e., histogram isn't available), this method returns
  // -1.  Anyone using this method should check for this value!
  inline CostScalar getAggregateUec (const ValueIdSet & columns) const
  {
    // simply calls the CSDL method of the same name
    return columnStats_.getAggregateUec (columns) ; 
  }

  // mutator:
  inline void setInputForSemiTSJ (SemiTSJEnum flag) 
                                               { inputForSemiTSJ_ = flag; }

  // Following method returns cardinality of the busiest stream

  CostScalar getCardOfBusiestStream(const PartitioningFunction* partFunc,
                                    Lng32 numOfParts,
                                    GroupAttributes  * grpAttr,
                                    Lng32 countOfCPUs = 1,
                                    NABoolean isUnderNestedJoin = FALSE);

  // ---------------------------------------------------------------------
  // merge two estimated log props ('this' is the merged version,
  // 'other' stays the same). The method returns TRUE, if reoptimization
  // should be done.
  // ---------------------------------------------------------------------
  NABoolean reconcile(const EstLogProp &other);

  // ---------------------------------------------------------------------
  // compare two estimated logical properties
  // ---------------------------------------------------------------------
  COMPARE_RESULT compareEstLogProp (const EstLogPropSharedPtr &other) const;

  // ---------------------------------------------------------------------
  // From the given ColStatDescList, populate columnStats_ with column
  // descriptors that are useful based on the characteristic outputs for
  // the group.  And, also with those that represent the current state of
  // the input parameters (Outer References).
  // ---------------------------------------------------------------------
  void pickOutputs( ColStatDescList & columnStats,
		    const EstLogPropSharedPtr& inputEstLogProp,
		    const ValueIdSet charOutputs,
		    const ValueIdSet predSet);
  
  void pickOutputsForUpdate( ColStatDescList columnStats, 
                             const EstLogPropSharedPtr& inputEstLogProp, 
                             const RelExpr & relExpr,
                             const ValueIdSet updateExprOutputs,
                             const ValueIdSet predSet);

  void mapOutputsForUpdate (const GenericUpdate & relExpr, 
			    const ValueIdMap & updateSelectValueIdMap);

  // ---------------------------------------------------------------------
  // Display function
  // ---------------------------------------------------------------------
  void print(FILE* ofd = stdout,
	     const char * prefix = DEFAULT_INDENT,
	     const char * suffix = "") const;

private:

  // The estimated base cardinality for the table to be scanned
  CostScalar       resultCardinality_; // after restriction by predicates
  CostScalar       maxCardinality_;    // maximum cardinality estimate
  CANodeIdSet*     nodeSet_;           // nodeSet to identify the EstLogProp
  ColStatDescList  columnStats_;       // column statistics
  ValueIdSet       unresolvedPreds_;   // preds that have not yet effected column
                                       // stats - ex. preds involving host vars
  SemiTSJEnum      inputForSemiTSJ_;   // For TSJ: pass semi-join flag to child
  NABoolean	   cacheable_;	       // check to see if these properties are cacheable by ASM
  
  //++MV
  // This is a non-estimated information , which sets to TRUE if the cardinality
  // equals 1.
  NABoolean	   isCardinalityEqOne_;
  //--MV

  // object counter
  static THREAD_P ObjectCounter *counter_;

};  // EstLogProp


#endif
