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
#ifndef COST_HDR
#define COST_HDR
/* -*-C++-*-
**************************************************************************
*
* File:         Cost.h
* Description:  Cost
* Created:      11/12/96
* Language:     C++
*
* Last Modified: May 2001
* Purpose:	 Simple Cost Vector Reduction
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include "NADefaults.h"
#include "BaseTypes.h"
#include "NAType.h"
#include "ValueDesc.h"
#include "PhyProp.h"
#include "PhyProp.h"
#include "CostScalar.h"
#include "CostVector.h"

#include "DefaultConstants.h"
#include "opt.h"


// -----------------------------------------------------------------------
// Forward references.
// -----------------------------------------------------------------------
class NAType;
class ValueId;
class ValueIdSet;
class ValueIdList;
class ItemExpr;

// -----------------------------------------------------------------------
// The following classes are defined in this file.
// -----------------------------------------------------------------------

class Cost;
class HashJoinCost;
class HashGroupByCost;
class CostWeight;
class ResourceConsumptionWeight;
class CostLimit;
class ElapsedTimeCostLimit;
class ScmElapsedTimeCostLimit;
class CostPrimitives;
class PerformanceGoal;



const Lng32 NORMAL_PLAN_PRIORITY = 0;  // must remain zero
const Lng32 INDEX_HINT_PRIORITY = 100;
const Lng32 JOIN_IMPLEMENTATION_HINT_PRIORITY = 100;
const Lng32 GROUPBY_IMPLEMENTATION_HINT_PRIORITY = 100;
const Lng32 INTERACTIVE_ACCESS_PRIORITY = 1;
const Lng32 INTERACTIVE_ACCESS_MDAM_PRIORITY = 1;
const Lng32 BLOCKING_OPS_FIRST_N_PRIORITY = -1;
const Lng32 HASH_JOIN_FIRST_N_PRIORITY = BLOCKING_OPS_FIRST_N_PRIORITY;
const Lng32 HASH_GROUP_BY_FIRST_N_PRIORITY = BLOCKING_OPS_FIRST_N_PRIORITY;
const Lng32 SORT_FIRST_N_PRIORITY = BLOCKING_OPS_FIRST_N_PRIORITY;
const Lng32 MATERIALIZE_FIRST_N_PRIORITY = BLOCKING_OPS_FIRST_N_PRIORITY;
const Lng32 MVQR_FAVORITE_PRIORITY = INTERACTIVE_ACCESS_PRIORITY;

class PlanPriority
{

public:
  // -----------------------------------------------------------------------
  // constructors (no need for a destructor, it's just a long-wrapper) xxx
  // -----------------------------------------------------------------------

  PlanPriority()
    : level_(0), demotionLevel_(0), riskPremium_(1.0)
  {}

  PlanPriority(const Lng32 val, const Lng32 demotion, CostScalar premium=1.0)
    : level_(val),   //xxx make def value
      demotionLevel_(demotion), //xxx make def value
      riskPremium_(premium)
  {}

  // Copy constructor.
  PlanPriority(const PlanPriority &other)
    : level_(other.level_),
      demotionLevel_(other.demotionLevel_),
      riskPremium_(other.riskPremium_)
  {}

  // ----------------------------------------------------------------------
  // overloaded operators
  // ----------------------------------------------------------------------

  // assignment
  inline PlanPriority & operator = (const PlanPriority &other)
  {
    level_ = other.level_ ;
    demotionLevel_ = other.demotionLevel_;
    riskPremium_ = other.riskPremium_;
    return *this;
  }

  // ----------------------------------------------------------------------
  // PlanPriority arithmetic
  // ----------------------------------------------------------------------

  // op +
  inline PlanPriority operator + (const PlanPriority &other) const
  {
    PlanPriority result(level_ + other.level_, 
                        demotionLevel_ + other.demotionLevel_);
    return result;
  }

  // op +=
  inline PlanPriority & operator += (const PlanPriority &other)
  {
    level_ = level_ + other.level_ ;
    demotionLevel_ = demotionLevel_ + other.demotionLevel_ ;
    return *this ;
  }

  // op -
  inline PlanPriority operator - (const PlanPriority &other) const
  {
    PlanPriority result(level_ - other.level_, 
                        demotionLevel_ - other.demotionLevel_);
    return result;
  }


  // ----------------------------------------------------------------------
  // comparison of PlanPriority objects
  // ----------------------------------------------------------------------

  // op ==
  inline NABoolean operator == (const PlanPriority &other) const
  {
    return ((level_ == other.level_) && (demotionLevel_ == other.demotionLevel_)) ;
  }

  // op !=
  inline NABoolean operator != (const PlanPriority &other) const
  {
    return NOT ( *this == other ) ;
  }

  // op <
  inline NABoolean operator <  (const PlanPriority &other) const
  {
    if (level_ == other.level_)
      return (demotionLevel_ < other.demotionLevel_);
    else
      return ( level_ < other.level_ ) ;
  }

  // op <=
  inline NABoolean operator <=  (const PlanPriority &other) const
  {
    if (level_ == other.level_)
      return (demotionLevel_ <= other.demotionLevel_);
    else
      return ( level_ < other.level_ ) ;
  }

  // op >
  inline NABoolean operator >  (const PlanPriority &other) const
  {
    if (level_ == other.level_)
      return (demotionLevel_ > other.demotionLevel_);
    else
      return ( level_ > other.level_ ) ;
  }

  // op >=
  inline NABoolean operator >=  (const PlanPriority &other) const
  {
    if (level_ == other.level_)
      return (demotionLevel_ >= other.demotionLevel_);
    else
      return ( level_ > other.level_ ) ;
  }

  inline NABoolean isNormal () const
  {
      return ((level_ == NORMAL_PLAN_PRIORITY) AND 
               demotionLevel_ == NORMAL_PLAN_PRIORITY);
  }

  // Reset to normal priority levels
  inline void resetToNormal()
  {
    level_ = NORMAL_PLAN_PRIORITY;
    demotionLevel_ = NORMAL_PLAN_PRIORITY;
  }

  // Rollup is simple now. Add priority level
  inline void rollUpUnary(const PlanPriority &childPriority)
  {
    level_ += childPriority.level_;
    demotionLevel_ += childPriority.demotionLevel_;
    // a way of rolling up risk premiums without intruding into cost details
    if (CmpCommon::getDefault(COMP_BOOL_178) == DF_OFF)
      riskPremium_ *= childPriority.riskPremium_;
  }

  inline void rollUpBinary(const PlanPriority &childPriority0,
                           const PlanPriority &childPriority1)
  {
    level_ += childPriority0.level_ + childPriority1.level_;
    demotionLevel_ += childPriority0.demotionLevel_ + childPriority1.demotionLevel_;
  }

  // Combine siblings is simple now. Add priority level
  inline void combine(const PlanPriority &childPriority)
  {
    level_ += childPriority.level_;
    demotionLevel_ += childPriority.demotionLevel_;
  }

  // Increment the priority level by a certain value
  inline void incrementLevels(Lng32 val, Lng32 dem)
  {
    level_ += val;
    demotionLevel_ += dem;
  }

  // Avoid using this method as much as possible except for display purposes
  inline Lng32 getLevel() const
  {
    return level_ ;
  }

  // Avoid using this method as much as possible except for display purposes
  // With current exception of using it in cost based pruning
  inline Lng32 getDemotionLevel() const
  {
    return demotionLevel_ ;
  }

  // risk premium accessor
  CostScalar riskPremium() const { return riskPremium_; }

private:

  Lng32 level_;
  Lng32 demotionLevel_;  // monotonically decreasing level
  // xxx for now level_ override demotionLevel_. Later on we will orthoganalize this
  CostScalar riskPremium_; 
  // multiplicative factor used to inflate cost of risky operator.
  // = 1.0 for non-risky (ie, robust) operator.
  // > 1.0 for risky operator like nested join.

}; // class PlanPriority

//<pb>
// -----------------------------------------------------------------------
//
// The Cost object expresses the resource consumption characteristics of
// an operation. It is a vehicle created by the costing routines to carry
// information on the resource usage of an operation to Cascades.
//
// -----------------------------------------------------------------------
class Cost : public NABasicObject
{
public:

  // ---------------------------------------------------------------------
  // Constructors.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // This constructor is typically used by an operator to store its local
  // processing costs.
  //
  // The simple cost vectors supplied as arguments should represent the
  // resource usage of an instance of the operator in one plan fragment
  // on one CPU. planFragmentsPerCPU is the total no of instances of the
  // operator that is executing on one CPU. countOfCPUs is the total no
  // of CPUs available for all plan fragments to execute on.
  // 10-16-2006: In order to integrate SCM, we are adding another member
  // to Cost class.
  // ---------------------------------------------------------------------
  Cost(const SimpleCostVector* currentProcessFirstRowCost,
       const SimpleCostVector* currentProcessLastRowCost,
       const SimpleCostVector* currentProcessBlockingCost,
       const Lng32 countOfCPUs,
       const Lng32 planFragmentsPerCPU);

  //-----------------------------------------------------------
  //  Constructor for SCM.
  //-----------------------------------------------------------
  Cost(const SimpleCostVector* currentProcessScmCost);

  //-----------------------------------------------------------
  //  Cost Constructor used in SCM code.
  //-----------------------------------------------------------
  Cost(const SimpleCostVector* currentProcessScmCost,
	     const SimpleCostVector* currentOpDebugInfo);

   //-----------------------------------------------------------
  //  Constructor for an empty Cost object.  Used primarily for
  // initializing a resulting Cost object during a roll-up.
  //-----------------------------------------------------------
  Cost()
    :
      cpfr_(),
      cplr_(),
      cpScmlr_(),
      cpScmDbg_(),
      cpbc1_(),
      cpbcTotal_(),
      //jopfr_(),
      //joplr_(),
      totalCost_(),
      countOfCPUs_(1),
      planFragmentsPerCPU_(1),
      priority_() {}

  //----------------------------------------------------------------------
  // Copy constructor.
  //----------------------------------------------------------------------
  Cost(const Cost &other)
    : cpfr_(other.cpfr_),
      cplr_(other.cplr_),
      cpScmlr_(other.cpScmlr_),
      cpScmDbg_(other.cpScmDbg_),
      cpbc1_(other.cpbc1_),
      cpbcTotal_(other.cpbcTotal_),
      //jopfr_(other.opfr_),
      //joplr_(other.oplr_),
      totalCost_(other.totalCost_),
      countOfCPUs_(other.countOfCPUs_),
      planFragmentsPerCPU_(other.planFragmentsPerCPU_),
      priority_(other.priority_) {}

  //--------------------------------
  //  Effective virtual constructor.
  //--------------------------------
  virtual Cost* duplicate();

  // ---------------------------------------------------------------------
  // Virtual destructor function.
  // ---------------------------------------------------------------------
  virtual ~Cost ();

  // ---------------------------------------------------------------------
  // Accessor methods.
  // ---------------------------------------------------------------------
  inline const SimpleCostVector& getCpfr()        const { return cpfr_; }
  inline const SimpleCostVector& getCplr()        const { return cplr_; }
  inline const SimpleCostVector& getScmCplr()     const { return cpScmlr_; }
  inline const SimpleCostVector& getScmDbg()     const { return cpScmDbg_; }
  inline const SimpleCostVector& getCpbcTotal()   const { return cpbcTotal_; }
  inline const SimpleCostVector& getCpbc1()       const { return cpbc1_; }
//jo  inline const SimpleCostVector& getOpfr()        const { return opfr_; }
//jo  inline const SimpleCostVector& getOplr()        const { return oplr_; }
  inline const SimpleCostVector& getTotalCost()   const { return totalCost_; }

  inline Lng32                    getCountOfCPUs() const {return countOfCPUs_;}

  inline Lng32                    getPlanFragmentsPerCPU() const
                                            {return planFragmentsPerCPU_;}
  inline const PlanPriority& getPlanPriority()   const {return priority_;}

  // ---------------------------------------------------------------------
  // Mutator methods.
  //
  // The following methods provide an interface (mainly for clients who
  // combine cost object in an operator-specific manner) to retrieve
  // handles (through which the vectors may be modified) to the various
  // cost vectors stored in the object.
  //
  // Handles retrieved by these methods should be used with care since
  // any changes made to the vector of the handle are directly propagated
  // to the corresponding vector stored in the Cost object itself.
  // ---------------------------------------------------------------------
  inline SimpleCostVector& cpfr()                        { return cpfr_; }
  inline SimpleCostVector& cplr()                        { return cplr_; }
  inline SimpleCostVector& cpScmlr()                     { return cpScmlr_; }
  inline SimpleCostVector& cpScmDbg()                     { return cpScmDbg_; }
  inline SimpleCostVector& cpbcTotal()             { return cpbcTotal_; }
  inline SimpleCostVector& cpbc1()                      { return cpbc1_; }
//jo  inline SimpleCostVector& opfr()                        { return opfr_; }
//jo  inline SimpleCostVector& oplr()                        { return oplr_; }
  inline SimpleCostVector& totalCost()              { return totalCost_; }
  inline Lng32& countOfCPUs()                       {return countOfCPUs_; }
  inline Lng32& planFragmentsPerCPU()       {return planFragmentsPerCPU_; }
  inline PlanPriority& planPriority()                { return priority_; }

  // ---------------------------------------------------------------------
  // Method for accessing a cost vector as a function of the given
  // performance goal.
  // ---------------------------------------------------------------------
  const SimpleCostVector& getCostVector
                            (const PerformanceGoal* const perfGoal) const;

  // ---------------------------------------------------------------------
  // Comparison function, mandated by Cascades.
  // ---------------------------------------------------------------------
  COMPARE_RESULT compareCosts
	             (const Cost & other,
	              const ReqdPhysicalProperty* const rpp = NULL) const;

  // ---------------------------------------------------------------------
  // Method for representing the cost in terms of an elapsed time.
  // ---------------------------------------------------------------------
  ElapsedTime convertToElapsedTime
	             (const ReqdPhysicalProperty* const rpp = NULL) const;

  // This method is to be used only for displaying total cost information
  // in the context of explain, visual debugger, etc. This is NOT to be used
  // for computing and/or comparing plan cost information. In the context of NCM,
  // internal costs used during plan computation use very different units than
  // the total cost displayed externally to the user, and so these cannot be
  // used interchangably.
  ElapsedTime displayTotalCost 
                    (const ReqdPhysicalProperty* const rpp = NULL) const;

  // ---------------------------------------------------------------------
  // Method for returning the detailed cost components as a descriptive string
  // ---------------------------------------------------------------------
  const NAString getDetailDesc() const;

  // -----------------------------------------------------------------------
  // This method returns cost information to WMS (and possibly other callers).
  // -----------------------------------------------------------------------
  void getExternalCostAttr(double &cpuTime, double &ioTime,
                           double &msgTime, double &idleTime,
                           double &numSeqIOs, double &numRandIOs,
                           double &totalTime, Lng32 &probes) const;

  // ---------------------------------------------------------------------
  // Method for returning OCM cost atrributes.
  // ---------------------------------------------------------------------
  void getOcmCostAttr(double &cpu, double &io, 
                      double &msg, double &idleTime,
                      Lng32 &probes) const;

  // ---------------------------------------------------------------------
  // Method for returning NCM cost atrributes.
  // ---------------------------------------------------------------------
  void getScmCostAttr(double &tcProc, double &tcProd,
                      double &tcSent, double &ioRand,
                      double &ioSeq, Lng32 &probes) const;

  // ---------------------------------------------------------------------
  // Method for returning NCM debug information.
  // ---------------------------------------------------------------------
  void getScmDebugAttr(double &dbg1, double &dbg2,
		       double &dbg3, double &dbg4) const;

  // ---------------------------------------------------------------------
  // Comparison function for SCM, mandated by Cascades.
  // ---------------------------------------------------------------------
  COMPARE_RESULT scmCompareCosts
	             (const Cost & other,
	              const ReqdPhysicalProperty* const rpp = NULL) const;

  // ---------------------------------------------------------------------
  // Method to compute the total cost represented by the 
  // cpScmlr SimpleCostVector of "this" Cost object.
  // ---------------------------------------------------------------------
  CostScalar scmComputeTotalCost 
	             (const ReqdPhysicalProperty* const rpp = NULL) const;

  // ---------------------------------------------------------------------
  // Method for converting a cost to cost limit.
  // ---------------------------------------------------------------------
  CostLimit* convertToCostLimit
	             (const ReqdPhysicalProperty* const rpp = NULL) const;

  // ---------------------------------------------------------------------
  // Overloaded addition for Cost.
  // ---------------------------------------------------------------------
  Cost& operator +=(const Cost & other);

  void mergeOtherChildCost(const Cost& otherChildCost);

  // ---------------------------------------------------------------------
  // Default implementations of plan priority computation
  // ---------------------------------------------------------------------

  // For use with leaf operators
  PlanPriority computePlanPriority( RelExpr* op,
                                          const Context* myContext);

  // For use with unary operators
  PlanPriority computePlanPriority( RelExpr* op,
                                          const Context* myContext,
                                          const Cost* childCost);

  // For use with binary operators
  PlanPriority computePlanPriority( RelExpr* op,
                                          const Context* myContext,
                                          const Cost* child0Cost,
                                          const Cost* child1Cost,
                                    PlanWorkSpace *pws=NULL,
                                    Lng32 planNumber=0);

  // ---------------------------------------------------------------------
  // Functions for debugging.
  // ---------------------------------------------------------------------

  void print(FILE * f = stdout,
	     const char * prefix = "", const char * suffix = "") const;

  void display() const;
//<pb>
private:

  // ---------------------------------------------------------------------
  // PRIVATE DATA.
  // ---------------------------------------------------------------------

  //----------------------------------------------------------------------
  // cpfr_:  Current Process First Row Cost.
  //
  //  This vector reflects the resource usage necessary for the current
  // operator in the expression tree to produce its first row.
  //
  //  Note that this is slightly inaccurate because DP2 will typically
  // return a buffer full of rows in response to its first request because
  // of Virtual Sequential Block Buffering (VSBB) but for all practical
  // purposes we can still view this resource usage vector as pertaining
  // to only the first row.
  //
  //  Note also that for repeat counts greater than one, this vector
  // represents the average usage per probe.
  //----------------------------------------------------------------------
  SimpleCostVector cpfr_;

  //----------------------------------------------------------------------
  // cplr_:  Current Process Last Row Cost.
  //
  //  This vector reflects the resource usage necessary for the current
  // operator in the expression tree to produce its last row.
  //
  //  Note that in the case of a nested loops join we really do mean the
  // very last row (i.e. the last row of the last probe).  Thus, for
  // repeat counts greater than one, this vector represents the total usage
  // for all probes whereas in CPFR above and CPTB and CPBlocking below
  // represent average usage for one of the probes.
  //----------------------------------------------------------------------
  SimpleCostVector cplr_;

  //----------------------------------------------------------------------
  // cpScmlr_:  Current Process Simple Cost Model's Last Row Cost.
  //
  //  This vector reflects the resource usage necessary for the current
  // operator in the expression tree to produce its last row.
  // This vector contains tuples processed, produced, and sent by an operator
  // in addition to IOs if any.
  //----------------------------------------------------------------------
  SimpleCostVector cpScmlr_;

  //----------------------------------------------------------------------
  // cpScmDbg_:  Extra member for Simple Cost Model Debugging
  // Used only for debugging internally by developers.
  //----------------------------------------------------------------------
  SimpleCostVector cpScmDbg_;

  //----------------------------------------------------------------------
  // cpbcTotal_: Current Process Total Blocking Cost.
  //
  //  An operator in an expression tree is called a "blocking" operator if
  // it must wait for all its children operators to fully complete before
  // it can produce its first row.  Examples include sort and
  // sort-group-by.  This vector reflects all resources used by
  // descendants of the current operator executing within the current
  // operator’s process while the current operator was blocked.  If
  // neither the current operator nor any of its descendants is a blocking
  // operator, this vector will be all zeros.
  //
  //  Note that for repeat counts greater than one, this vector represents
  // the average usage per probe.
  //----------------------------------------------------------------------
  SimpleCostVector cpbcTotal_;

  //----------------------------------------------------------------------
  // cpbc1_:  Current Process Blocking Cost for first blocking operator.
  //
  //  Consider an operator which may or may not be blocking but which has
  // at least one descendant which executes within this operator’s process
  // and which is blocking.  Choose the lowest such descendant.  The
  // cpbcTotal_ vector for that operator becomes cpbc1_ for all of its
  // ancestors.
  //
  //  Note that multiple children for an operator will complicate the
  // calculation of both CPBlocking and CPTB above.  We deal with this by
  // combining both children to look like one child.
  //
  //  Note also that for repeat counts greater than one, this vector
  // represents the average usage per probe.
  //
  //----------------------------------------------------------------------
  SimpleCostVector cpbc1_;

  // **********************************************************************
  //  Currently opfr_ and oplr_ are not used.  When CPU maps become
  // available and we can determine in which CPU an operator is executing,
  // we can then make use of these vectors in cost accumulation (roll-up)
  // formulas.
  //
  //  The descriptions below, therefore, describe intended usage not
  // yet fully implemented.
  // **********************************************************************

  //----------------------------------------------------------------------
  // oplr_:  Overlapped Process First Row Cost.
  //
  //  Analogous to _cpfr except it represents the resource usage for child
  // operators executing in a different CPU.  During the time the child
  // operators use these resources, the current operator can not proceed
  // (i.e. is idle) because it will not have received any rows from its
  // children.
  //
  //  Note that for repeat counts greater than one, this vector represents
  // the average usage per probe.
  // ---------------------------------------------------------------------
  //jo SimpleCostVector opfr_;

  //----------------------------------------------------------------------
  // oplr_:  Overlapped Process Last Row Cost.
  //
  //  Analogous to _cplr except it represents the resource usage for child
  // operators executing in a different CPU.
  //
  //  Note that for repeat counts greater than one, this vector represents
  // the total usage for all probes.
  //----------------------------------------------------------------------
  //jo SimpleCostVector oplr_;

  //----------------------------------------------------------------------
  // totalCost_:  the total cost for performing an operation.
  //
  //  A vector of resource usage describing all resources necessary for
  // the current operator and all its descendants to execute.  Query
  // parallelism has no effect on totalCost_ as it does on cplr_.  For the
  // totalCost_ vector, all parallel activity is accounted for as if it
  // actually occurred serially.
  //
  //  Note that for repeat counts greater than one, this vector represents
  // the total usage for all probes.
  //
  //----------------------------------------------------------------------
  SimpleCostVector totalCost_;
  // ---------------------------------------------------------------------
  // Number of CPUs used by this operator
  // ---------------------------------------------------------------------
  Lng32 countOfCPUs_;

  // ---------------------------------------------------------------------
  // Number of plan fragments that compete for the same CPU
  // ---------------------------------------------------------------------
  Lng32 planFragmentsPerCPU_;

  // ---------------------------------------------------------------------
  // The plan priority is used for implementation of preferred plans such
  // as plans generated for optimizer hints, interactive access, and future
  // optimizer heuristics.
  // Plans that have similar priority are compared as usual. Plans with
  // different priorities are judged in favor of the plan with the higher
  // priority regardless of the cost vector values.
  // ---------------------------------------------------------------------
  PlanPriority priority_;

}; // class Cost

//---------------------------------------------------------------
//  Needed for passing pointers to costs as reference parameters.
//---------------------------------------------------------------
typedef Cost* CostPtr;
//<pb>
//------------------------------------------------------------------------
//  A HashJoinCost object contains intermediate resource usage vectors and
// variables computed during hash join preliminary costing which final
// roll-up costing will also use.
//------------------------------------------------------------------------
class HashJoinCost : public Cost
{
public:

  //---------------
  //  Constructors.
  //---------------
  HashJoinCost(const SimpleCostVector* currentProcessFirstRowCost,
               const SimpleCostVector* currentProcessLastRowCost,
               const SimpleCostVector* currentProcessBlockingCost,
               const Lng32              countOfCPUs,
               const Lng32              planFragmentsPerCPU,
               const CostScalar &      stage2WorkFractionForFR_,
               const CostScalar &      stage3WorkFractionForFR_,
               const SimpleCostVector* stage2Cost,
               const SimpleCostVector* stage3Cost,
               const SimpleCostVector* stage1BkCost,
               const SimpleCostVector* stage2BkCost,
               const SimpleCostVector* stage3BkCost);

  //-----------------------------------------------
  //  Constructor for an empty HashJoinCost object.
  //-----------------------------------------------
  HashJoinCost()
    : Cost(),
      stage2WorkFractionForFR_( csOne ),
      stage3WorkFractionForFR_( csZero ),
      stage2Cost_(),
      stage3Cost_(),
      stage1BkCost_(),
      stage2BkCost_(),
      stage3BkCost_()
  {}

  //-------------------
  //  Copy constructor.
  //-------------------
  HashJoinCost(const HashJoinCost &other)
    : Cost(other),
      stage2WorkFractionForFR_(other.stage2WorkFractionForFR_),
      stage3WorkFractionForFR_(other.stage3WorkFractionForFR_),
      stage2Cost_(other.stage2Cost_),
      stage3Cost_(other.stage3Cost_),
      stage1BkCost_(other.stage1BkCost_),
      stage2BkCost_(other.stage2BkCost_),
      stage3BkCost_(other.stage3BkCost_)
  {}

  //--------------------------------
  //  Effective virtual constructor.
  //--------------------------------
  virtual Cost* duplicate();

  //-------------
  //  Destructor.
  //-------------
  ~HashJoinCost();

  //---------------------
  //  Accessor Functions.
  //---------------------
  inline const SimpleCostVector& getStage2Cost() const { return stage2Cost_; }
  inline const SimpleCostVector& getStage3Cost() const { return stage3Cost_; }

  inline const SimpleCostVector& getStage1BKCost() const {return stage1BkCost_;}
  inline const SimpleCostVector& getStage2BKCost() const {return stage2BkCost_;}
  inline const SimpleCostVector& getStage3BKCost() const {return stage3BkCost_;}

  inline const CostScalar& getStage2WorkFractionForFR() const
                                            { return stage2WorkFractionForFR_; }

  inline const CostScalar& getStage3WorkFractionForFR() const
                                            { return stage3WorkFractionForFR_; }

private:

  // -------------
  // PRIVATE DATA.
  // -------------

  //---------------------------------------------------------------------------
  //  Fraction of stage 2 work necessary to produce this hash join's first row.
  // The value must be greater than zero and less than or equal to 1.  Further,
  // the value must be greater than or equal to stage3WorkFractionForFR_
  // below.
  //---------------------------------------------------------------------------
  CostScalar stage2WorkFractionForFR_;

  //---------------------------------------------------------------------------
  //  Fraction of stage 3 work necessary to produce this hash join's first row.
  // The value must be greater than or equal to zero and less than or equal to
  // 1.  Further, the value must be less than or equal to
  // stage2WorkFractionForFR_ above.
  //---------------------------------------------------------------------------
  CostScalar stage3WorkFractionForFR_;

  //----------------------------------------------------------------------------
  //  This vector represents resource usage for stage 2 of a hash join.  Stage 2
  // involves taking rows produced by the left child (outer table), computing a
  // hash function for each row, probing the hash table built in stage 1 when
  // the row hashes to a main memory cluster or writing the row to disk when it
  // hashes to an overflow cluster.
  //
  //  Note that for repeat counts greater than one, this vector represents
  // the cumulative usage for all probes.
  //----------------------------------------------------------------------------
  SimpleCostVector stage2Cost_;

  //----------------------------------------------------------------------------
  //  This vector represents resource usage for stage 3 of a hash join.  Stage 3
  // involves joining the rows overflowed to disk in stage 1 and stage 2.
  //
  //  Note that for repeat counts greater than one, this vector represents
  // the cumulative usage for all probes.
  //----------------------------------------------------------------------------
  SimpleCostVector stage3Cost_;

  SimpleCostVector stage1BkCost_;

  SimpleCostVector stage2BkCost_;

  SimpleCostVector stage3BkCost_;
}; // class HashJoinCost
//<pb>
//------------------------------------------------------------------------------
//  A HashGroupByCost object contains the grouping factor for this Hash GroupBy
// operator--i.e. the percentage of groups which fit in memory.  Final roll-up
// costing uses this number for determining what percentage of last row activity
// below the Hash GroupBy operator becomes blocking activity as part of the
// roll-up
//------------------------------------------------------------------------------
class HashGroupByCost : public Cost
{
public:

  //---------------
  //  Constructors.
  //---------------
  HashGroupByCost(const SimpleCostVector* currentProcessFirstRowCost,
                  const SimpleCostVector* currentProcessLastRowCost,
                  const SimpleCostVector* currentProcessBlockingCost,
                  const Lng32              countOfCPUs,
                  const Lng32              planFragmentsPerCPU,
                  const CostScalar &      groupingFactor);

  //--------------------------------------------------
  //  Constructor for an empty HashGroupByCost object.
  //--------------------------------------------------
  HashGroupByCost()
    : Cost(),
      groupingFactor_( csOne )
  {}

  //-------------------
  //  Copy constructor.
  //-------------------
  HashGroupByCost(const HashGroupByCost &other)
    : Cost(other),
      groupingFactor_(other.groupingFactor_)
  {}

  //--------------------------------
  //  Effective virtual constructor.
  //--------------------------------
  virtual Cost* duplicate();

  //-------------
  //  Destructor.
  //-------------
  ~HashGroupByCost();

  //---------------------
  //  Accessor Functions.
  //---------------------
  inline CostScalar getGroupingFactor() const { return groupingFactor_; }

private:

  // -------------
  // PRIVATE DATA.
  // -------------

  //--------------------------------------------------------------------
  //  Percentage of groups which fit in memory.  Must be between 0 and 1
  // inclusive.
  //--------------------------------------------------------------------
  CostScalar groupingFactor_;

};
//<pb>
// -----------------------------------------------------------------------
// Specify how to weigh (and therefore, to compare) cost objects
// --- abstract base class ---
// -----------------------------------------------------------------------
class CostWeight : public NABasicObject
{
public:

  virtual ~CostWeight() {}

  // ---------------------------------------------------------------------
  // Type-safe pointer casts used for run-time type identification.
  // ---------------------------------------------------------------------
  virtual ResourceConsumptionWeight* castToResourceConsumptionWeight() const;

  // ---------------------------------------------------------------------
  // Virtual method that returns the number of elements in the CostWeight.
  // ---------------------------------------------------------------------
  virtual Lng32 entries() const = 0;

  // ---------------------------------------------------------------------
  // Virtual method for converting a CostVector to a floating point value.
  // ---------------------------------------------------------------------
  virtual double convertToFloatingPointValue(const CostVector& cv) const = 0;

  // ---------------------------------------------------------------------
  // Virtual method for converting a CostVector to an elapsed time.
  // ---------------------------------------------------------------------
  virtual ElapsedTime convertToElapsedTime(const CostVector& cv) const = 0;

  // ---------------------------------------------------------------------
  // Virtual method for converting a CostVector to a CostLimit.
  // ---------------------------------------------------------------------
  virtual CostLimit* convertToCostLimit(const CostVector& cv) const = 0;

  // ---------------------------------------------------------------------
  // Virtual method for comapring two cost vectors.
  // ---------------------------------------------------------------------
  virtual COMPARE_RESULT compareCostVectors(const CostVector& cv1,
				            const CostVector& cv2) const = 0;

}; // class CostWeight
//<pb>
// -----------------------------------------------------------------------
// use weighing factors to determine the scalar cost
// -----------------------------------------------------------------------
class ResourceConsumptionWeight : public CostWeight
{
public:

  ResourceConsumptionWeight(
    const CostScalar & cpuWeight =	csZero,
    const CostScalar & IOWeight             = csZero,
    const CostScalar & MSGWeight            = csZero,
    const CostScalar & IdleTimeWeight         = csZero,
    const CostScalar & numProbesWeight     = csZero
    );

  // ---------------------------------------------------------------------
  // Type-safe pointer casts used for run-time type identification.
  // ---------------------------------------------------------------------
  virtual ResourceConsumptionWeight* castToResourceConsumptionWeight() const;

  // ---------------------------------------------------------------------
  // Virtual method that returns the number of elements in the CostWeight.
  // ---------------------------------------------------------------------
  virtual Lng32 entries() const;

  // ---------------------------------------------------------------------
  // Virtual method for converting a CostVector to a floating point value.
  // ---------------------------------------------------------------------
  virtual double convertToFloatingPointValue(const CostVector& cv) const;

  // ---------------------------------------------------------------------
  // Convert the given CostVector to an elapsed time provided it
  // has the same number of entries() as this CostWeight.
  // ---------------------------------------------------------------------
  virtual ElapsedTime convertToElapsedTime(const CostVector& scv) const;

  // ---------------------------------------------------------------------
  // Virtual method for converting a Cost to a CostLimit.
  // ---------------------------------------------------------------------
  virtual CostLimit* convertToCostLimit(const CostVector& cv) const;

  // ---------------------------------------------------------------------
  // Virtual method for comparing two cost vectors.
  // ---------------------------------------------------------------------
  virtual COMPARE_RESULT compareCostVectors(const CostVector& cv1,
				            const CostVector& cv2) const;

  // A method that provides access to a specific entry.
  CostScalar operator[] (Lng32 ix) const;

private:
  CostScalar weighFactors_[COUNT_OF_SIMPLE_COST_COUNTERS];

}; // class ResourceConsumptionWeight
//<pb>
// -----------------------------------------------------------------------
// Specify  a cost limit
// --- abstract base class ---
// -----------------------------------------------------------------------
class CostLimit : public NABasicObject
{
public:

  virtual ~CostLimit() {}

  // ---------------------------------------------------------------------
  // Virtual copy constructor.
  // ---------------------------------------------------------------------
  virtual CostLimit* copy() const = 0;

  // ---------------------------------------------------------------------
  // Type-safe pointer casts used for run-time type identification.
  // ---------------------------------------------------------------------
  virtual ElapsedTimeCostLimit* castToElapsedTimeCostLimit() const;

  // ---------------------------------------------------------------------
  // Get a double precision value for the limit.
  // ---------------------------------------------------------------------
  virtual double getValue(const ReqdPhysicalProperty* const rpp) const = 0;

  virtual double getCachedValue () = 0;
  virtual void   setCachedValue (double val) = 0;
  // ---------------------------------------------------------------------
  // Comparison function for costs and cost limits.
  // ---------------------------------------------------------------------
  virtual COMPARE_RESULT compareCostLimits
                       (CostLimit* other,
	                const ReqdPhysicalProperty* const rpp = NULL) = 0;

  virtual COMPARE_RESULT compareWithCost
                           (const Cost & other,
	                    const ReqdPhysicalProperty* const rpp = NULL) = 0;

  virtual COMPARE_RESULT compareWithPlanCost
                           (CascadesPlan* plan,
	                    const ReqdPhysicalProperty* const rpp = NULL) = 0;

  virtual void ancestorAccum(const Cost& otherCost,
                             const ReqdPhysicalProperty* const rpp = NULL) = 0;

  virtual void otherKinAccum(const Cost& otherCost) = 0;

  virtual void tryToReduce(const Cost& otherCost,
                           const ReqdPhysicalProperty* const rpp = NULL) = 0;

  virtual void unilaterallyReduce(const ElapsedTime & timeReduction) = 0;

  virtual const PlanPriority& priorityLimit() const = 0;

  virtual void setUpperLimit(ElapsedTime ul) = 0;

private:

}; // class CostLimit
//<pb>
//------------------------------------------------------------------------------
// class ElapsedTimeCostLimit
//
//  An elapsed time cost limit consists of three components:  an initial
// elapsed time and two pointers to accumulated cost objects.
//
//  The initial elapsed time represents the upper limit for the cumulative
// elapsed time of all physical operators in the final query tree.
//
//  One accumulated cost object represents the resource usage of all
// operators on the path from the root to the current operator.  We call
// this ancestor cost.
//
//  The second cost object represents resource usage for all known siblings,
// cousins, uncles, etc of the current operator.  We call this other kin cost.
//
//  To find the remaining elapsed time available for descendant operators of the
// current operator, we first roll up the other kin cost with the ancestor cost
// and calculate the resulting cost's elapsed time.  We then subtract this
// resulting elapsed time from the upper limit.
//
//  We accumulate costs at each level of the query tree, and we potentially
// reduce the upper limit with the discovery of better plans.
//
//  We can also reduce the upper limit unilaterally for some operators with
// known child costs.
//------------------------------------------------------------------------------
class ElapsedTimeCostLimit : public CostLimit
{
public:

  //--------------
  //  Constructors
  //--------------
  ElapsedTimeCostLimit(const ElapsedTime& limit,
                       const PlanPriority& priorityLimit,
                       const Cost*        ancestorCost,
                       const Cost*        otherKinCost)
  : upperLimit_(limit),cachedValue_(0.0),priorityLimit_(priorityLimit)
  {
    CMPASSERT(   ancestorCost != NULL
              && otherKinCost != NULL
              && ancestorCost != otherKinCost);

    ancestorCost_ = new STMTHEAP Cost(*ancestorCost);
    otherKinCost_ = new STMTHEAP Cost(*otherKinCost);
  }

  ElapsedTimeCostLimit(const ElapsedTimeCostLimit& other)
  : upperLimit_(other.upperLimit_),
    priorityLimit_(other.priorityLimit_),
    ancestorCost_(new(CmpCommon::statementHeap()) Cost(*other.ancestorCost_)),
    otherKinCost_(new(CmpCommon::statementHeap()) Cost(*other.otherKinCost_)),
    cachedValue_(other.cachedValue_)
    {};

  //------------
  //  Destructor
  //------------
  virtual ~ElapsedTimeCostLimit()
    { delete ancestorCost_; delete otherKinCost_; }


  virtual CostLimit* copy() const
    { return new(CmpCommon::statementHeap()) ElapsedTimeCostLimit(*this); }

  virtual double getValue(const ReqdPhysicalProperty* const rpp) const;

  double getCachedValue (){ return cachedValue_; }
  void   setCachedValue (double val) { cachedValue_ = val; }

  virtual COMPARE_RESULT compareCostLimits
                           (CostLimit* other,
                            const ReqdPhysicalProperty* const rpp = NULL);

  virtual COMPARE_RESULT compareWithCost
                           (const Cost & other,
	                    const ReqdPhysicalProperty* const rpp = NULL);

  virtual COMPARE_RESULT compareWithPlanCost
                           (CascadesPlan* plan,
	                    const ReqdPhysicalProperty* const rpp = NULL);

  virtual void ancestorAccum(const Cost& otherCost,
                             const ReqdPhysicalProperty* const rpp = NULL);

  virtual void otherKinAccum(const Cost& otherCost);

  virtual void tryToReduce(const Cost& otherCost,
                           const ReqdPhysicalProperty* const rpp = NULL);

  virtual void unilaterallyReduce(const ElapsedTime & timeReduction);

  // ---------------------------------------------------------------------
  // Type-safe pointer casts used for run-time type identification.
  // ---------------------------------------------------------------------
  virtual ElapsedTimeCostLimit* castToElapsedTimeCostLimit() const;

  virtual const PlanPriority& priorityLimit() const {return priorityLimit_;}

  void setUpperLimit(ElapsedTime ul) { upperLimit_ = ul; }

private:

  //--------------------------------------------------------------------
  //  Upper limit for elapsed time of all operators in final query tree.
  //--------------------------------------------------------------------
  ElapsedTime upperLimit_;

  //--------------------------------------------------------------------
  //  Upper limit for plan priority of all operators in final query tree.
  //--------------------------------------------------------------------
  PlanPriority priorityLimit_;

  //--------------------------------------------------------------------------
  //  Accumulated cost of all operators on path from root to current operator.
  //--------------------------------------------------------------------------
  Cost* ancestorCost_;

  //-------------------------------------------------------------
  //  Accumulated cost of all known siblings, cousins and uncles.
  //-------------------------------------------------------------
  Cost* otherKinCost_;

  // This is cached value of getValue() method for this costLimit.
  // It is used to cpeed up compare operations.
  double cachedValue_;

}; // class ElapsedTimeCostLimit


//<pb>
// -------------------------------------------------------------------
// class ScmElapsedTimeCostLimit
// The ScmElapsedTimeCostLimit is NCM (New Cost Model) implementation
// of CostLimit abstract class. This is very similar to OCM (Old Cost Model)
// implementation ElapsedTimeCostLimit.
// ---------------------------------------------------------------------
class ScmElapsedTimeCostLimit : public CostLimit
{
public:

  //--------------
  //  Constructors
  //--------------
  ScmElapsedTimeCostLimit(const ElapsedTime& limit,
                          const PlanPriority& priorityLimit,
                          const Cost*         ancestorCost,
                          const Cost*         otherKinCost)
  : upperLimit_(limit),cachedValue_(0.0),priorityLimit_(priorityLimit)
  {
    CMPASSERT(   ancestorCost != NULL
              && otherKinCost != NULL
              && ancestorCost != otherKinCost);

    ancestorCost_ = new STMTHEAP Cost(*ancestorCost);
    otherKinCost_ = new STMTHEAP Cost(*otherKinCost);
  }

  ScmElapsedTimeCostLimit(const ScmElapsedTimeCostLimit& other)
  : upperLimit_(other.upperLimit_),
    priorityLimit_(other.priorityLimit_),
    ancestorCost_(new(CmpCommon::statementHeap()) Cost(*other.ancestorCost_)),
    otherKinCost_(new(CmpCommon::statementHeap()) Cost(*other.otherKinCost_)),
    cachedValue_(other.cachedValue_)
    {};

  //------------
  //  Destructor
  //------------
  virtual ~ScmElapsedTimeCostLimit()
    { delete ancestorCost_; delete otherKinCost_; }

  virtual CostLimit* copy() const
    { return new(CmpCommon::statementHeap()) ScmElapsedTimeCostLimit(*this); }

  virtual double getValue(const ReqdPhysicalProperty* const rpp) const;

  double getCachedValue (){ return cachedValue_; }
  void   setCachedValue (double val) { cachedValue_ = val; }

  virtual COMPARE_RESULT compareCostLimits
                           (CostLimit* other,
                            const ReqdPhysicalProperty* const rpp = NULL);

  virtual COMPARE_RESULT compareWithCost
                           (const Cost & other,
                            const ReqdPhysicalProperty* const rpp = NULL);

  virtual COMPARE_RESULT compareWithPlanCost
                           (CascadesPlan* plan,
                            const ReqdPhysicalProperty* const rpp = NULL);

  virtual void ancestorAccum(const Cost& otherCost,
                             const ReqdPhysicalProperty* const rpp = NULL);

  virtual void otherKinAccum(const Cost& otherCost);

  virtual void tryToReduce(const Cost& otherCost,
                           const ReqdPhysicalProperty* const rpp = NULL);

  virtual void unilaterallyReduce(const ElapsedTime & timeReduction);

  // ---------------------------------------------------------------------
  // Type-safe pointer casts used for run-time type identification.
  // ---------------------------------------------------------------------
  //virtual ScmElapsedTimeCostLimit* castToScmElapsedTimeCostLimit() const;

  virtual const PlanPriority& priorityLimit() const {return priorityLimit_;}

  void setUpperLimit(ElapsedTime ul) { upperLimit_ = ul; }

private:

  //--------------------------------------------------------------------
  //  Upper limit for elapsed time of all operators in final query tree.
  //--------------------------------------------------------------------
  ElapsedTime upperLimit_;

  //--------------------------------------------------------------------
  //  Upper limit for plan priority of all operators in final query tree.
  //--------------------------------------------------------------------
  PlanPriority priorityLimit_;

  //--------------------------------------------------------------------------
  //  Accumulated cost of all operators on path from root to current operator.
  //--------------------------------------------------------------------------
  Cost* ancestorCost_;

  //-------------------------------------------------------------
  //  Accumulated cost of all known siblings, cousins and uncles.
  //-------------------------------------------------------------
  Cost* otherKinCost_;

  // This is cached value of getValue() method for this costLimit.
  // It is used to cpeed up compare operations.
  double cachedValue_;

}; // class ScmElapsedTimeCostLimit


// -------------------------------------------------------------------
//                             CONSTANTS
//                             =========
// $$$$ Convert all of these to become CostScalars after the latter
// $$$$ is enhanced to support arithmetical operations with different
// $$$$ numeric datatypes supported in C++..
//
// These have all been added to the Defaults table (7/21/97)
//
// They are arranged in an alphabetical order to aid visual searches.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// The executor in DP2 is capable of performing 56Kb transfers,
// Actually, 56k   57344 bytes, but we assume that 1344 bytes are used
// for storing data that is private to DP2/message system.
// With Guardian, the 56K limit applies to the local ESP<->DP2
// and 32K for the non-local ESP<->DP2.
// -----------------------------------------------------------------------
//#define DP2_MESSAGE_BUFFER_SIZE (56E+3)  // used for ESP<->DP2

// -----------------------------------------------------------------------
// Actually, 32k   33568 bytes, but we assume that 1568 bytes are used
// for storing data that is private to the OS/message system.
// -----------------------------------------------------------------------
//#define OS_MESSAGE_BUFFER_SIZE (32E+3)  // used for ESP<->ESP

// -----------------------------------------------------------------------
//                            COST FACTORS
//
// $$$$ Convert all of these to become CostScalars after the latter
// $$$$ is enhanced to support arithmetical operations with different
// $$$$ numeric datatypes supported in C++..
//
// These were all been added to the Defaults table (7/21/97)
//
// They are arranged in an alphabetical order to aid visual searches.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Assume 10 instructions for copying a row.
// -----------------------------------------------------------------------
//#define CPUCOST_COPY_ROW (0.01)

// -----------------------------------------------------------------------
// Assume 100 instructions for reading a row across the DM interface
//  (executor does a DM^GET in DP2)
// -----------------------------------------------------------------------
//#define CPUCOST_DM_GET (0.1)

// -----------------------------------------------------------------------
// Assume 100 instructions for updating/deleting a row across the DM
//   interface (executor does a DM^UPDATE in DP2)
// -----------------------------------------------------------------------
//#define CPUCOST_DM_UPDATE (0.1)

// -----------------------------------------------------------------------
// Assume 2000 instructions  (20ms => same elapsed time as performing
// 1 random IO) for process startup and initialization.
// This number should be reduced when persistent ESP management services
// are implemented.
// -----------------------------------------------------------------------
//#define CPUCOST_ESP_INITIALIZATION (2.000)

// -----------------------------------------------------------------------
// Assume 10 instructions for mapping a particular data stream to
// a particular consumer process.
// -----------------------------------------------------------------------
//#define CPUCOST_EXCHANGE_MAPPING_FUNCTION (0.01)

// -----------------------------------------------------------------------
// Assume 10 instructions per partitioning key column for computing the
// identifier for the hash or range partition.
// -----------------------------------------------------------------------
//#define CPUCOST_EXCHANGE_SPLIT_FUNCTION (0.01)

// -----------------------------------------------------------------------
// Assume 250 instructions for locking a row
// -----------------------------------------------------------------------
//#define CPUCOST_LOCK_ROW (0.25)

// -----------------------------------------------------------------------
// Assume 100 instructions for performing a predicate comparison.
// -----------------------------------------------------------------------
//#define CPUCOST_PREDICATE_COMPARISON (0.1)

// -----------------------------------------------------------------------
// Assume 20,000 instructions for initializing a subset.
// Includes the cost of opening a file and initializing control
// structures, but NOT positioning on the first row of the subset.
// -----------------------------------------------------------------------
//#define CPUCOST_SUBSET_OPEN (20)


// -----------------------------------------------------------------------
// Assume 10 instructions for establishing a reference to a tuple
// in the executor's address space. This includes the copying of
// pointers to the tuple into the executor's data structures.
// -----------------------------------------------------------------------
//#define CPUCOST_TUPLE_REFERENCE (0.01)

//<pb>
// ---------------------------------------------------------------------------
// declaration for class Cost Primitives
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// There are currently four major types of Cost primitives.
//
// 1. Cost primitives for basic executor operations: they represent the costs
//    of common executor operations typically performed by every operator.
//    Examples include allocation of buffer pools and buffers, free tuples
//    from a buffer and ATPs, etc. They are being represented as constant
//    costs and can be retrieved using getBasicCostFactor(opEnum) where
//    opEnum is a member of the enumeration type of operations (see
//    DefaultsTable.h in sqlcomp directory).
//
// 2. Cost primitives for evaluation of executor expressions: each represent
//    the cost of evaluating an expression or subexpression using the general
//    expression evaluator. Examples are evaluations of predicates, copying
//    a row, hashing a set of keys, etc. These are parameterised primitives
//    which are accessed individually using different interfaces. To retrieve
//    the cost of copying a set of columns for example, a call can be made to
//    cpuCostForCopySet(vis) where vis is the ValueIdSet representing those
//    columns. (In the long run, we might consider having these primitives
//    stored under each ItemExpr.)
//
// 3. Cost Primitives related to specific relational operators. Different
//    relational operators have different primitives operations which are not
//    captured by the general framework of the executor, as defined by the
//    primitives operations in (1) and (2). For example, some auxillary data
//    structures may be allocated (hash table and linked list). Also, misc
//    costs such as those associated with the overhead of the task scheduler,
//    which may be different for different operators, are also considered as
//    this type. (In the long run, we might consider having these primitives
//    stored under each RelExpr.)
//
// 4. Basic cost factors. This is a list of fudge factors used in the formulae
//    for combining costs of an operator and its children. Examples are those
//    used to determine how much I/O of one operator can overlap with I/O of
//    another operator.
// ---------------------------------------------------------------------------

class CostPrimitives
{
public:

  // -------------------------------------------------------------------------
  // To copy one instance of vidset from one memory location to another.
  //
  // The method assumes that the values in the vidset are NOT necessarily
  // stored together in a contiguous piece of memory. Thus, we need to copy
  // each individual value over one after another.
  // -------------------------------------------------------------------------
  static double cpuCostForCopySet (const ValueIdSet & vidset);

  // -------------------------------------------------------------------------
  // To copy a row from one memory location to another.
  //
  // As opposed to the above method, this method assume that the row is stored
  // in a contiguous piece of memory. We could exploit the memcpy function and
  // save some overhead in theory.
  // -------------------------------------------------------------------------
  static double cpuCostForCopyRow (Lng32 byteCount);

  // -------------------------------------------------------------------------
  // To compare two instances of vidset. Comparison operators represented
  // here include: EQ, NE, GT, GE, LT, LE, but not LIKE.
  //
  // This does NOT account for the cost of evaluating the members of vidset
  // which might be expressions instead of column references or scalars. The
  // members are assumed to have been evaluated and their values stored in
  // memory before they are compared by this operation.
  //
  // No short circuit mechanism is assumed. That is, the cost returned is one
  // if the two instances are equal. If the client of this method applies this
  // operation on multiple rows and has some ideas of how often it evaluates
  // to TRUE and FALSE, it can adjust this to an average value assuming only
  // half the cost is incurred when evaluated to FALSE.
  // -------------------------------------------------------------------------
  static double cpuCostForCompare (const ValueIdSet & vidset);

  // -------------------------------------------------------------------------
  // To evaluate the Like comparison operation. vid must be associated with
  // the Like builtin function.
  //
  // $$$ This is a tentative measure before cpuCostForEvalFunc() is in place.
  // $$$ Like is a subclass of Function and we should be able to obtain its
  // $$$ cost from there.
  // -------------------------------------------------------------------------
  static double cpuCostForLikeCompare (const ValueId & vid);

  // -------------------------------------------------------------------------
  // To evaluate an arithmetic expression specified by vid.
  //
  // This method walks the skeleton of the item expression of vid which can
  // be made up of +,-,*,/,**, summing up the their costs along the way. It
  // treats any nodes other than those given above as leaf nodes.
  // -------------------------------------------------------------------------
  static double cpuCostForEvalArithExpr (const ValueId & vid);

  // -------------------------------------------------------------------------
  // To evaluate the function specified by vid. vid must be associated with
  // an object of a subclass of Function (defined in ItemFunc.h). The method
  // returns the cost of evaluating the function given that all its arguments
  // have already been evaluated.
  //
  // $$$ In the first release of CostPrimitives.C, the implementation of this
  // $$$ method is just a stub which returns a cost of 0. The plan is to store
  // $$$ the cost of evaluating a function as private members of the class of
  // $$$ the function itself. Changes in ItemFunc.h need to be made.
  // -------------------------------------------------------------------------
  static double cpuCostForEvalFunc (const ValueId & vid);

  // -------------------------------------------------------------------------
  // To evaluate the set of predicates given as vidset and AND the results.
  //
  // This method walks the AND/OR/NOT skeleton of the item expression tree of
  // each predicate in the set up to the point where comparison operators are
  // located. There, it uses cpuCostForCompare() or cpuCostForLikeCompare() to
  // determine the costs of the comparisons, and adds the costs to the costs
  // for evaluating the AND/OR/NOT operations themselves.
  //
  // Since it calls cpuCostForCompare() or cpuCostForLikeCompare once it sees
  // a comparison operator, it has the same assumption that the evaluation of
  // expressions on the two sides of the comparison operator is not accounted
  // for in the cost computed.
  // -------------------------------------------------------------------------
  static double cpuCostForEvalPred (const ValueIdSet & vidset);

  // -------------------------------------------------------------------------
  // To evaluate the expression specified by vid. The method is going to walk
  // the tree rooted at vid, get the costs of evaluating each node on its way
  // and sum up the costs.
  //
  // $$$ In the first release of CostPrimitives.C, the implementation of this
  // $$$ method is just a stub which returns a cost of 0.
  // -------------------------------------------------------------------------
  static double cpuCostForEvalExpr (const ValueId & vid);

  // -------------------------------------------------------------------------
  // To encode an instance of vidset.
  //
  // The encode function is mainly used in Sort to map a list of sort keys to
  // single value which preserves order. The single value is used by ArkSort
  // as a single key based on which the records are sorted.
  //
  // $$$ This function may be replaced by a call to costing method at the
  // $$$ CompEncode node subsequently if we move costs to individual function
  // $$$ subclasses. It might still be worthwhile to have an interface here
  // $$$ so that we don't have to synthesize a CompEncode object just to get
  // $$$ its cost.
  // -------------------------------------------------------------------------
  static double cpuCostForEncode (const ValueIdSet & vidset);

  // -------------------------------------------------------------------------
  // To hash a set of keys stored as vidset.
  //
  // The hash function is used by all hash operators to produce a hash value
  // from a set of hash keys.
  //
  // $$$ This function may be replaced by a call to costing method at the
  // $$$ Hash node subsequently if we move costs to individual function
  // $$$ subclasses. It might still be worthwhile to have an interface here
  // $$$ so that we don't have to synthesize a Hash object just to get its
  // $$$ cost.
  // -------------------------------------------------------------------------
  static double cpuCostForHash (const ValueIdSet & vidset);

  // -------------------------------------------------------------------------
  // To aggregate a row incrementally to the set of aggregates in vidset.
  //
  // Assume we already have somewhere in memory the aggregates up to the last
  // row, the method costs the incremental effort of aggregating a new row to
  // that set of aggregates already stored. We also assume the aggregates are
  // not nested and any arithmetic expressions within the aggregates have been
  // evaluated and stored.
  // -------------------------------------------------------------------------
  static double cpuCostForAggrRow (const ValueIdSet & vidset);

  // -------------------------------------------------------------------------
  // Basic cost factors used in the formulae to combine costs.
  // -------------------------------------------------------------------------
  static double getBasicCostFactor (Lng32 mscf);

private:

};


// -----------------------------------------------------------------------
// This class is used for storing and propagating DP2 costing
// data that is computed during synthesis of the DP2 leaf operators.
// -----------------------------------------------------------------------
class DP2CostDataThatDependsOnSPP : public NABasicObject
{
public:
  DP2CostDataThatDependsOnSPP()
     : highestLeadingPartitionColumnCovered_(-1)
    ,repeatCountForOperatorsInDP2_( csMinusOne )
    ,countOfCPUsExecutingDP2s_(-1)
    ,probesAtBusiestStream_ (csOne)
    ,rCountState(UNKNOWN)
  {}



enum repeatCountSTATE
{
   KEYCOLS_COVERED_BY_CONST=1,
   KEYCOLS_COVERED_BY_PROBE_COLS_CONST,
   KEYCOLS_NOT_COVERED,
   UPDATE_OPERATION,
   UNKNOWN
};
  // -----------------------------------------------------------------------
  // Accessors:
  // -----------------------------------------------------------------------

  CostScalar getProbesAtBusiestStream() const
  { return probesAtBusiestStream_;}
  Lng32 getHighestLeadingPartitionColumnCovered() const
  { return highestLeadingPartitionColumnCovered_; }

  CostScalar getRepeatCountForOperatorsInDP2() const
  { return repeatCountForOperatorsInDP2_; }

  Lng32 getCountOfCPUsExecutingDP2s() const
  { return countOfCPUsExecutingDP2s_; }

  // -----------------------------------------------------------------------
  // Mutators:
  // -----------------------------------------------------------------------

  void setProbesAtBusiestStream(const CostScalar & rc)
      { probesAtBusiestStream_ = rc;}
  void setHighestLeadingPartitionColumnCovered(Lng32 hlpcc)
  {  highestLeadingPartitionColumnCovered_ = hlpcc; }

  void  setRepeatCountForOperatorsInDP2(const CostScalar & rc)
  {  repeatCountForOperatorsInDP2_ = rc; }

  void setRepeatCountState(const repeatCountSTATE rs)
  { rCountState=rs;}
  
  repeatCountSTATE getRepeatCountState()
  { return rCountState;}

  void  setCountOfCPUsExecutingDP2s(Lng32 countCpus)
  { countOfCPUsExecutingDP2s_ = countCpus; }


private:
  Lng32 highestLeadingPartitionColumnCovered_;
  CostScalar repeatCountForOperatorsInDP2_;
  Lng32 countOfCPUsExecutingDP2s_;
  CostScalar probesAtBusiestStream_;
  repeatCountSTATE rCountState;

};



#endif /* COST_HDR */
