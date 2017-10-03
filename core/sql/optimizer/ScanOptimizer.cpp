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
 * File:         ScanOptimizer.cpp
 * RCS:
 * Description:  Costing for leaf operators
 * Code location: ScanOptimizer.C
 *
 * Created:      //96
 * Language:     C++
 * Purpose:	 Simple Cost Vector Reduction
 *
 *
 *
 *****************************************************************************
 */

#include "stdlib.h"

#include <sys/io.h>

#include "MdamDebug.h"
#include "ScanOptimizer.h"
#include "SimpleScanOptimizer.h"
#include "NAFileSet.h"
#include "ItemColRef.h"
#include "NATable.h"
#include "ItemOther.h"
#include "CmpContext.h"
#include "Sqlcomp.h"
#include "ControlDB.h"
#include "ItemLog.h"
#include "../exp/exp_ovfl_ptal.h" //to check overflow
#include "CmpStatement.h"
#include "mdam.h"
#include "OptRange.h"

// -----------------------------------------------------------------------
// These defines are set because as of today there is no
// mechanism to assert for preconditions. CMPASSERT is an
// internal error reporting system and it does not get disabled
// in release code.
// This is my own solution but a general solution must be found
// an agreed upon.
// -----------------------------------------------------------------------


#undef FSOWARNING

#ifndef NDEBUG
#define FSOWARNING(x) fprintf(stdout, "FileScan optimizer warning: %s\n", x);
#else
#define FSOWARNING(x)
#endif

#ifdef MDAM_TRACE

THREAD_P FILE *MdamTrace::outputFile_ = NULL;
THREAD_P NABoolean MdamTrace::doPrint_ = FALSE;
THREAD_P NABoolean MdamTrace::initialized_ = FALSE;
THREAD_P const char* MdamTrace::msgHeader_ = NULL;
THREAD_P const char* MdamTrace::overrideHeader_ = NULL;
THREAD_P const char* MdamTrace::indent_ = "\t";
THREAD_P Int32 MdamTrace::hStdOut_ = -1;
THREAD_P NABoolean MdamTrace::okToRedirectStdOut_ = FALSE;
THREAD_P FILE* MdamTrace::console_ = NULL;
THREAD_P enum MdamTraceLevel MdamTrace::level_ = MDAM_TRACE_LEVEL_NONE;

// use MTL3 to debug MDAM issues
//THREAD_P enum MdamTraceLevel MdamTrace::level_ = MTL3;

void MdamTrace::setHeader(const char *override)
{
  overrideHeader_ = override;
}

const char* MdamTrace::getHeader()
{
  if(overrideHeader_)
    return overrideHeader_;
  else if(msgHeader_)
    return msgHeader_;
  else
    return "";
}

void MdamTrace::mayInit(){
  if(initialized_)
    return;
  initialized_ = TRUE;
  // Note: the "MDAM_DEBUG" string is split into two adjacent strings so
  // C++ preprocessor does not perform macro substitution on MDAM_DEBUG.
  //
  if (getenv("MDAM_""DEBUG"))
    {
      doPrint_ = TRUE;
    }

  const char *debugFileName = getenv("MDAM_""DEBUG_""FILE");
  if (debugFileName)
    {
      outputFile_ = fopen(debugFileName, "a");
      if(!outputFile_)
	outputFile_ = stdout;
    }
  else
    outputFile_ = stdout;

  msgHeader_ = "[MDAM_DEBUG] ";

  if(outputFile_ == stdout){
    okToRedirectStdOut_ = FALSE; // no need to redirect
  }
  else if((hStdOut_ = dup(fileno(stdout))) == -1){
    okToRedirectStdOut_ = FALSE; // cannot duplicate stdout desc, don't redirect
  }
  else{
    console_ = fdopen(hStdOut_, "w");
    if(console_ == NULL){
      okToRedirectStdOut_ = FALSE; // cannot create stream, don't redirect
    }
    else{
      okToRedirectStdOut_ = TRUE; // ok to redirect
    }
  }
}

void MdamTrace::redirectStdOut()
{
  mayInit();
  if(okToRedirectStdOut_){

    *stdout = *outputFile_;
    ios::sync_with_stdio();
  }
}

void MdamTrace::restoreStdOut()
{
  mayInit();
  if(okToRedirectStdOut_){

    *stdout = *console_;
    ios::sync_with_stdio();
  }
}

void MdamTrace::print(const char *formatString, ...)
{
  mayInit();
  if (doPrint_)
  {
    va_list args;
    va_start(args, formatString);
    fprintf(outputFile_, "%s", getHeader());
    vfprintf(outputFile_, formatString, args);
    fprintf(outputFile_, "\n");
    fflush(outputFile_);
  }
}

void MdamTrace::printTaskMonitor(TaskMonitor &mon,
				 const char *msg)
{
  mayInit();

  if(!mon.goodcount())
    return;
  if(!doPrint_)
    return;

  ostringstream mon_text;
  mon_text << mon;

  fprintf(outputFile_, "%s%s %s\n", getHeader(), msg, mon_text.str().c_str());
  fflush(outputFile_);
}

void MdamTrace::printCostObject(ScanOptimizer *opt,
				const Cost *cost,
				const char *msg)
{
  mayInit();

  printf("%s%s >>\n", getHeader(), msg);
  opt->printCostObject(cost);
  printf("\n");
  fflush(stdout);
}

void MdamTrace::printFileScanInfo(ScanOptimizer *opt,
				  const ValueIdSet &partKeyPreds)
{
  mayInit();

  printf("\n%sScan Information:\n", getHeader());
  if (opt->getContext()
    .getReqdPhysicalProperty()->getPerformanceGoal() == NULL
    OR
    opt->getContext().getReqdPhysicalProperty()->getPerformanceGoal() ==
    CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal())
  {
    printf("%sOptimizing for last row.\n", indent_);
  }
  else
  {
    printf("%sOptimizing for first row.\n", indent_);
  }

  printf("%sIndex Key: ", indent_);
  opt->getIndexDesc()->getIndexKey().display();

  if (opt->getIndexDesc()->getNAFileSet()->isKeySequenced())
  {
    printf("%s%sTable is key sequenced.\n", indent_, indent_);
  }
  else
  {
    printf("%s%sTable is NOT key sequenced.\n", indent_, indent_);
  }

  printf("%sSelection predicates: ", indent_);
  opt->getRelExpr().getSelectionPred().display();

  printf("%sPartitioning Key Predicates: ", indent_);
  partKeyPreds.display();

  printf("%sExternal inputs: ", indent_);
  opt->getExternalInputs().display();

  printf("%sCharacteristic outputs: ", indent_);
  opt->getRelExpr().getGroupAttr()->getCharacteristicOutputs().display();


  printf("%sReceiving [%.4f] rows (probes)\n", indent_,
              opt->getContext().getInputLogProp()->
              getResultCardinality().value());
  if ((opt->getContext().getInputPhysicalProperty() != NULL) AND
    (opt->getContext().getInputPhysicalProperty()->getNjOuterOrder() != NULL))
  {
    printf("%sIncoming rows with ordering: ", indent_);
    opt->getContext().getInputPhysicalProperty()->
      getNjOuterOrder()->display();
  }
  else
  {
    printf("%sIncoming rows unordered", indent_);
  }

  if ((opt->getContext().getInputLogProp()
       ->getResultCardinality()).isGreaterThanZero() )
  {
    if (opt->getContext().getInputLogProp()->getColStats().entries() > 0 )
    {
      printf("%sStatistics: ", indent_);
      opt->getContext().getInputLogProp()->getColStats().display();
    }
    else
    {
      printf("%sNO statistics.\n", indent_);
    }
  }
  printf("\n");
  fflush(stdout);

}


void MdamTrace::printBasicCost(FileScanOptimizer *opt,
			       SimpleCostVector &prefixFR,
			       SimpleCostVector &prefixLR,
			       const char *msg)
{
  Cost *costPtr = opt->computeCostObject(prefixFR, prefixLR);

  printf("%s%s\n", getHeader(), msg);
  opt->printCostObject(costPtr);
  delete costPtr;
  fflush(stdout);
}



MdamTraceLevel MdamTrace::level()
{
  return level_;
}

void MdamTrace::setLevel(enum MdamTraceLevel l)
{
  level_ = l;
}

#endif // if MDAM_TRACE

enum restrictCheckStrategy { MAJORITY_WITH_PREDICATES=1, TOTAL_UECS=2, BOTH=3 };

static NABoolean checkMDAMadditionalRestriction(
    const ColumnOrderList& keyPredsByCol,
    const CollIndex& lastColumnPosition, 
    const Histograms& hist,
    restrictCheckStrategy strategy,
    CollIndex&  noOfmissingKeyColumns, CollIndex&  presentKeyColumns)
{
   KeyColumns::KeyColumn::KeyColumnType typeOfRange = KeyColumns::KeyColumn::EMPTY;
   CollIndex index = 0;

   NABoolean checkLeadingDivColumns =
             (CmpCommon::getDefault(MTD_GENERATE_CC_PREDS) == DF_ON);

   Lng32 mtd_mdam_uec_threshold = (Lng32)(ActiveSchemaDB()->getDefaults()).
                                     getAsLong(MTD_MDAM_NJ_UEC_THRESHOLD);

   if ( mtd_mdam_uec_threshold < 0 )
     checkLeadingDivColumns = FALSE;

   CostScalar totalRC = hist.getRowCount().getCeiling();

   float totalUEC_threshold = 1;

   Lng32 minRC = (ActiveSchemaDB()->getDefaults()).getAsLong(MDAM_TOTAL_UEC_CHECK_MIN_RC_THRESHOLD);

   if ( totalRC > minRC )
     (ActiveSchemaDB()->getDefaults()).getFloat(MDAM_TOTAL_UEC_CHECK_UEC_THRESHOLD, totalUEC_threshold);

   totalUEC_threshold *= totalRC.getValue();

   NABoolean isLeadingDivisionColumn = FALSE;
   NABoolean isLeadingSaltColumn = FALSE;

   CostScalar totalUecsForPredicatelessKeyColumns = 1;
   CostScalar totalUecsForCurrentPredicatelessKeyColumnGroup = 1;

   ValueIdSet currentPredicatelessKeyColumnGroup;

   for (index = 0; index < lastColumnPosition; index++)
   {
     if (keyPredsByCol.getPredicateExpressionPtr(index) != NULL)
     {
       typeOfRange = keyPredsByCol.getPredicateExpressionPtr(index)->getType();       
     }
     else
       typeOfRange= KeyColumns::KeyColumn::EMPTY;

     isLeadingDivisionColumn = FALSE;
     isLeadingSaltColumn = FALSE;

     ValueId columnVid = keyPredsByCol.getKeyColumnId(index);

     if ( checkLeadingDivColumns )
     {
       // Check if the key column is a leading divisioning column
       isLeadingDivisionColumn = columnVid.isDivisioningColumn();

       // Check if the key column is a leading salted column
       isLeadingSaltColumn = columnVid.isSaltColumn();
     }

     if (typeOfRange == KeyColumns::KeyColumn::EMPTY) {

        // if not check leading DIV columns or check leading div columns
        // and the current key column is not divisioning, increase the
        // the non-key-predicate columns.
        if ( checkLeadingDivColumns == FALSE ||
             (!isLeadingDivisionColumn && !isLeadingSaltColumn)
           )
           noOfmissingKeyColumns++;

        // accumulate the product of uecs for columns without predicates in the current
        // group
        totalUecsForCurrentPredicatelessKeyColumnGroup *= hist.getColStatsForColumn(
                   columnVid).getTotalUec().getCeiling();

        // accumulate the column valud Id at the same time for MC UEC lookup later on.
        currentPredicatelessKeyColumnGroup.insert(columnVid);
     } else {

        checkLeadingDivColumns = FALSE;
        presentKeyColumns++;

        // If the set of key columns without predicate is not empty, fetch the MC UEC 
        // for the entire set. If the MC UEC exists, replace the current accumualted
        // total UEC with the MC UEC.
        //
        // We will set the set to empty so that the fetching MC UEC logic will not kick in 
        // until a new key column without predicates is seen.
        if ( currentPredicatelessKeyColumnGroup.entries() > 1 ) {

           // fetch MC UEC from key coluymns for column set currentPredicatelessKeyColumnGroup 
           const MultiColumnUecList* MCUL = hist.getColStatDescList().getUecList();

           ValueIdSet theLargestSubset = 
                   MCUL->largestSubset(currentPredicatelessKeyColumnGroup.convertToBaseIds());

           if ( theLargestSubset.entries() == currentPredicatelessKeyColumnGroup.entries() ) 
           {
              CostScalar mcUEC = MCUL->lookup(theLargestSubset);

              if ( mcUEC != csMinusOne )
                 totalUecsForCurrentPredicatelessKeyColumnGroup = mcUEC;
           }

           currentPredicatelessKeyColumnGroup.clear();
        }

        totalUecsForPredicatelessKeyColumns *= 
                totalUecsForCurrentPredicatelessKeyColumnGroup;

        totalUecsForCurrentPredicatelessKeyColumnGroup = 1;
     }
   }     

   switch ( strategy ) {
     case MAJORITY_WITH_PREDICATES:
        return (presentKeyColumns > noOfmissingKeyColumns);

     case TOTAL_UECS:
        return ( totalUecsForPredicatelessKeyColumns < totalUEC_threshold );

     case BOTH:
        return ( presentKeyColumns > noOfmissingKeyColumns &&
                 totalUecsForPredicatelessKeyColumns < totalUEC_threshold );

     default:
       return FALSE;
  }
  return FALSE;
}


// MDAM Cost Workarea
//
// This object is used to compute the optimal MDAM cost.
//
// It is allocated on the stack only.

class NewMDAMCostWA
{
public:
  NewMDAMCostWA(FileScanOptimizer & optimizer,
    NABoolean mdamForced,
    MdamKey *mdamKeyPtr,
    const Cost *costBoundPtr,
    const ValueIdSet & exePreds,
    const CostScalar & singleSubsetSize);
  void compute();
  NABoolean isMdamWon() const;
  NABoolean hasNoExePreds() const;
  const CostScalar & getNumKBytes() const;
  void computeDisjunct();
  Cost * getScmCost();

private:
  // output
  NABoolean mdamWon_;
  NABoolean noExePreds_;
  CostScalar numKBytes_;
  Cost * scmCost_;

  // input
  const NABoolean mdamForced_;
  MdamKey *mdamKeyPtr_;
  const Cost *costBoundPtr_;
  FileScanOptimizer & optimizer_;
  const CostScalar singleSubsetSize_;

  // work variables

  // estimated # of rows upper bound of the scan (TODO: per scan probe?)
  const CostScalar innerRowsUpperBound_;
  // estimated # of blocks upper bound of the scan (TODO: per scan probe?)
  // This is computed from innerRowsUpperBound_ and estimatedRecordsPerBlock_;
  // the method of computation doesn't allow us to make this const (sigh)
  CostScalar innerBlocksUpperBound_;
  // estimated # of rows per block of the scan
  const CostScalar estimatedRowsPerBlock_;

  // The disjunct index currently being computed by compute()
  // TODO: does this really need to be a member?
  CollIndex disjunctIndex_;

  // Some terminology:
  //
  // Unfortunately, the term "probe" is overloaded, referring to
  // two very different concepts. 
  //
  // "Scan probe" -- This is the usual Optimizer meaning of the
  // term "probe". It means an input row sent to a relational
  // operator. Many Scan nodes will have just one probe per 
  // statement execution. A Scan node for the inner table of a
  // nested join may have any number of probes. "Scan probe" is
  // not to be confused with "MDAM probe", which is a lower-level
  // concept.
  // "MDAM probe" -- This is the run-time act of materializing the
  // next value for some key column. A subset is created with the
  // begin key reflecting the last value materialized (or the first
  // possible value in the interval if this is the first traversal
  // to this interval). The end key reflects the end of the interval.
  // The run-time fetches at most one row from this subset then
  // closes the subset. The row fetched (if any) gives the next 
  // value of the key column.
  // "MDAM fetch" -- This is the run-time act of fetching rows that
  // satisfy the MDAM key predicates. Zero or more subsets will be
  // generated at run time based on interval boundaries and/or key
  // column values materialized by MDAM probes.

  NABoolean isMultipleScanProbes_; // if true, there may be multiple scan probes
  CostScalar incomingScanProbes_;  // number of scan probes

  // The next few variables are the values for the optimum prefix
  // of the last disjunct costed. Note that since the run-time does
  // MDAM probes until it fails to find another key column value,
  // disjunctOptMDAMProbeRows_ < disjunctMDAMProbeSubsets_. The
  // one exception to this case is when we do "dense" access; then
  // disjunctMdamProbeSubsets_ = 0.

  CostScalar disjunctOptMDAMFetchRows_;  // number of rows fetched by MDAM key predicates
  CostScalar disjunctOptMDAMFetchSubsets_; // number of MDAM fetch subsets
  CostScalar disjunctOptMDAMProbeRows_;  // number of MDAM probes
  CostScalar disjunctOptMDAMProbeSubsets_; // number of MDAM probe subsets

  // set to FALSE if computeDisjunct() finds a heuristic reason that
  // we should not use MDAM

  NABoolean disjunctMdamOK_;

  // scratch space
  const ValueIdSet & exePreds_;
  const ScanForceWildCard *scanForcePtr_;

  // Outer histograms is used in multiple probes, where it is joined with
  // the disjunct histograms and also used to compute # of failed probes
  const Histograms outerHistograms_;
};

// stack allocated only
// work area to find out the optimal disjunct prefix
class NewMDAMOptimalDisjunctPrefixWA{
public:
  NewMDAMOptimalDisjunctPrefixWA(
    FileScanOptimizer & optimizer,
    const ColumnOrderList & keyPredsByCol,
    const ValueIdSet & disjunctKeyPreds,
    const ValueIdSet & exePreds,
    const Histograms & outerHistograms,
    MdamKey *mdamKeyPtr,
    const ScanForceWildCard *scanForcePtr,
    NABoolean mdamForced,
    NABoolean isMultipleProbes,
    const CostScalar & incomingScanProbes,
    const CostScalar & estimatedRecordsPerBlock,
    const CostScalar & innerRowsUpperBound,
    const CostScalar & innerBlocksUpperBound,
    const CostScalar & singleSubsetSize,
    CollIndex disjunctIndex);

  ~NewMDAMOptimalDisjunctPrefixWA();

  CollIndex getOptStopColumn() const;
  const CostScalar & getOptMDAMFetchRows() const;
  const CostScalar & getOptMDAMFetchSubsets() const;
  const CostScalar & getOptMDAMProbeRows() const;
  const CostScalar & getOptMDAMProbeSubsets() const;
  const ValueIdSet & getOptKeyPreds() const;

  void compute(NABoolean & noExePreds /* out */);

  CollIndex getStopColumn() const;

private:

  void applyPredsToHistogram(const ValueIdSet * predsPtr);

  NABoolean isColumnDense(CollIndex columnPosition);

  void calculateMetricsFromKeyPreds(const ValueIdSet * predsPtr, const CostScalar & maxUEC,
                                    CostScalar & UECFromPreds /*out*/, CostScalar & IntervalCountFromPreds /*out*/);
  void calculateMetricsFromKeyPred(const ValueId & keyPred, const CostScalar & maxUEC, 
    CostScalar & UECFromPreds /*out*/, CostScalar & IntervalCountFromPreds /*out*/,
    int & lessCount /*in/out*/, int & greaterCount /*in/out*/);

  NABoolean isMinimalCost(Cost * currentCost,
                          CollIndex columnPosition,
                          NABoolean & forced /* out */);

  // --------------------------- output --------------------------

  // metrics for optimal stop column found so far (and ultimately output)
  CostScalar optMDAMFetchRows_;  // number of rows fetched by MDAM key predicates
  CostScalar optMDAMFetchSubsets_; // number of MDAM fetch subsets
  CostScalar optMDAMProbeRows_;  // number of rows returned by MDAM probe subsets
  CostScalar optMDAMProbeSubsets_; // number of MDAM probe subsets

  CollIndex optStopColumn_;
  // ---------------------------- input -----------------------------
  FileScanOptimizer & optimizer_;
  // array of pointers to key predicates ordered on key columns
  const ColumnOrderList & keyPredsByCol_;
  // key predicates of the disjunct
  const ValueIdSet & disjunctKeyPreds_;
  // executor predicates of the disjunct
  const ValueIdSet & exePreds_;
  // Outer histograms is used in multiple probes, where it is joined with
  // the disjunct histograms and also used to compute # of failed probes
  const Histograms & outerHistograms_;
  // User specified scan force pattern
  const ScanForceWildCard * scanForcePtr_;
  const NABoolean isMultipleProbes_;
  const NABoolean mdamForced_;
  // # of probes
  CostScalar incomingScanProbes_;
  // estimated # of records per block of the scan
  const CostScalar estimatedRecordsPerBlock_;
  // estimated # of rows upper bound of the scan
  const CostScalar innerRowsUpperBound_;
  // estimated # of blocks upper bound of the scan
  const CostScalar innerBlocksUpperBound_;
  // estimated # of rows in a single subset scan (before
  // application of executor predicates)
  const CostScalar singleSubsetSize_;
  const CollIndex disjunctIndex_;
  // ---------------------------- input with side effects -----------
  MdamKey * mdamKeyPtr_;
  // ------------------------------ scratch space ---------------------
  IndexDescHistograms disjunctHistograms_;

  // metrics for optimal stop column found so far
  Cost * optimalCost_;
  
  NABoolean crossProductApplied_;

  const NABoolean multiColUecInfoAvail_;

  NABoolean MCUECOfPriorPrefixFound_;
  CostScalar MCUECOfPriorPrefix_;
};


// stack allocated only
// work area to find out the MDAM cost
class MDAMCostWA
{
public:
  MDAMCostWA(FileScanOptimizer & optimizer,
    NABoolean mdamForced,
    MdamKey *mdamKeyPtr,
    const Cost *costBoundPtr,
    const ValueIdSet & exePreds,
    SimpleCostVector & disjunctsFR,
    SimpleCostVector & disjunctsLR);
  void compute();
  NABoolean isMdamWon() const;
  NABoolean hasNoExePreds() const;
  const CostScalar & getNumKBytes() const;
  void computeDisjunct();
  Cost * getScmCost();
private:
  // output
  NABoolean mdamWon_;
  NABoolean noExePreds_;
  CostScalar numKBytes_;
  // input
  const NABoolean mdamForced_;
  MdamKey *mdamKeyPtr_;
  const Cost *costBoundPtr_;
  FileScanOptimizer & optimizer_;
  // -- side effects
  SimpleCostVector & disjunctsFR_;
  SimpleCostVector & disjunctsLR_;
  Cost * scmCost_;
  // scrach space
  const ValueIdSet & exePreds_;
  // estimated # of rows upper bound of the scan
  const CostScalar innerRowsUpperBound_;
  // estimated # of blocks upper bound of the scan
  // This is computed from innerRowsUpperBound_ and estimatedRecordsPerBlock_
  // Consider making it const and move the computation to the constructor
  CostScalar innerBlocksUpperBound_;
  // estimated # of records per block of the scan
  const CostScalar estimatedRecordsPerBlock_;
  const NABoolean isMultipleProbes_;
  const ScanForceWildCard *scanForcePtr_;
  // Consider making it const and move the computation to the constructor
  CostScalar incomingProbes_;
  // Histograms to apply first column predicates from different disjunct
  // This is used to compute whether there is disjunct overlaps.
  IndexDescHistograms firstColumnHistogram_;
  // Outer histograms is used in multiple probes, where it is joined with
  // the disjunct histograms and also used to compute # of failed probes
  const Histograms outerHistograms_;
  CollIndex disjunctIndex_;
  // -- optimal prefix cost factors of each disjunct
  CostScalar disjunctOptRows_;
  CostScalar disjunctOptRqsts_;
  CostScalar disjunctOptProbesForSubsetBoundaries_;
  CostScalar disjunctOptSeeks_;
  CostScalar disjunctOptSeqKBRead_;
  ValueIdSet disjunctOptKeyPreds_;
  NABoolean disjunctMdamOK_;
  CollIndex disjunctNumLeadingPartPreds_;
  CostScalar disjunctFailedProbes_;
};


// stack allocated only
// work area to find out the optimal disjunct prefix
class MDAMOptimalDisjunctPrefixWA{
public:
  MDAMOptimalDisjunctPrefixWA(
    FileScanOptimizer & optimizer,
    const ColumnOrderList & keyPredsByCol,
    const ValueIdSet & disjunctKeyPreds,
    const ValueIdSet & exePreds,
    const Histograms & outerHistograms,
    IndexDescHistograms & firstColumnHistogram,
    NABoolean & noExePreds,
    MdamKey *mdamKeyPtr,
    const ScanForceWildCard *scanForcePtr,
    NABoolean mdamForced,
    NABoolean isMultipleProbes,
    const CostScalar & incomingProbes,
    const CostScalar & estimatedRecordsPerBlock,
    const CostScalar & innerRowsUpperBound,
    const CostScalar & innerBlocksUpperBound,
    CollIndex disjunctIndex);

  ~MDAMOptimalDisjunctPrefixWA();

  CollIndex getStopColumn() const;
  CollIndex getNumLeadingPartPreds() const;
  const CostScalar & getFailedProbes() const;
  const CostScalar & getOptRows() const;
  const CostScalar & getOptRqsts() const;
  const CostScalar & getOptProbesForSubsetBoundaries() const;
  const CostScalar & getOptSeeks() const;
  const CostScalar & getOptSeqKBRead() const;
  const ValueIdSet & getOptKeyPreds() const;

  void compute();

  const CostScalar rcAfterApplyFirstKeyPreds() const
  { return rcAfterApplyFirstKeyPreds_; }

private:
  void processLeadingColumns();
  void processNonLeadingColumn();
  void applyPredsToHistogram();
  void computeDensityOfColumn();
  void updatePositions();
  void updateStatistics();
  void updateMinPrefix();
  void processSingleSubsetPrefix();
  void computeProbesDisjunct();
  NABoolean missingKeyColumnExists() const;
  // --------------------------- output --------------------------
  // # of failed probes
  CostScalar failedProbes_;
  // optimal prefix's # of rows
  CostScalar optRows_;
  // optimal prefix's # of requests: #effective probes * #subsets
  CostScalar optRqsts_;
  // optimal prefix's # of requests to find subset boudaries
  CostScalar optRqstsForSubsetBoundaries_;
  // optimal prefix's # of seeks
  CostScalar optSeeks_;
  // optimal prefix's # of KB read
  CostScalar optSeqKBRead_;
  // optimal prefix's ey predicates
  ValueIdSet optKeyPreds_;
  // The last column of the optimal prefix, which is 0 based
  CollIndex stopColumn_;
  // The # of partition predicates on the 1st column
  CollIndex numLeadingPartPreds_;
  // ---------------------------- input -----------------------------
  FileScanOptimizer & optimizer_;
  // array of pointers to key predicates ordered on key columns
  const ColumnOrderList & keyPredsByCol_;
  // key predicates of the disjunct
  const ValueIdSet & disjunctKeyPreds_;
  // executor predicates of the disjunct
  const ValueIdSet & exePreds_;
  // Outer histograms is used in multiple probes, where it is joined with
  // the disjunct histograms and also used to compute # of failed probes
  const Histograms & outerHistograms_;
  // User specified scan force pattern
  const ScanForceWildCard * scanForcePtr_;
  const NABoolean isMultipleProbes_;
  const NABoolean mdamForced_;
  // # of probes
  const CostScalar incomingProbes_;
  // estimated # of records per block of the scan
  const CostScalar estimatedRecordsPerBlock_;
  // estimated # of rows upper bound of the scan
  const CostScalar innerRowsUpperBound_;
  // estimated # of blocks upper bound of the scan
  const CostScalar innerBlocksUpperBound_;
  const CollIndex disjunctIndex_;
  // ---------------------------- input with side effects -----------
  IndexDescHistograms & firstColumnHistogram_;
  NABoolean & noExePreds_;
  MdamKey * mdamKeyPtr_;
  // ------------------------------ scrach space ---------------------
  IndexDescHistograms disjunctHistograms_;
  const NABoolean multiColUecInfoAvail_;
  // Last key column position we need to consider
  // when computing optimal prefix. It is one based
  const CollIndex lastColumnPosition_;
  // Whether the multiple subsets overlaps on the first column
  // when comparing to the previous disjunct
  NABoolean firstColOverlaps_;
  // Estimated rows iff multiple probes
  CostScalar multiProbesDataRows_;
  // >>>>>>>>>>>>>>>>> Current prefix related members <<<<<<<<<<<<<<<<<
  // # of subsets of each effective probe at the current level
  CostScalar prefixSubsets_;
  // cumulative # of subsets of each effective probe
  // Why do we care? MDAM is a recursive algorithm. It first materializes
  // values for the first key column. For each of those, it materializes 
  // values for the second key column. And so on. Each of these levels adds
  // progressively more cost which we must take into account. If we look
  // only at prefixSubsets_ (that is, the current column level), we may be
  // misled into thinking that adding more levels of column traversal is
  // free. Which it is not. Moreover, as the number of rows approaches the
  // total number of rows in the table, it is akin to adding an additional
  // table scan.
  CostScalar cumulativePrefixSubsets_;
  // Cached value of CQD MDAM_PROBE_TAX, to which cumulativePrefixSubsets_ 
  // is multiplied when computing cost
  double probeTax_;
  // # of subset seeks of each effective probe
  CostScalar prefixSubsetsAsSeeks_;
  // # of rows of all probes at the current column level
  CostScalar prefixRows_;
  // # of seeks of all probes.
  CostScalar prefixRqsts_;
  // # of additional seeks of all probes to read index blocks
  // to find subset boundaries for a sparse column
  CostScalar prefixRqstsForSubsetBoundaries_;
  // # of seeks of all probes
  CostScalar prefixSeeks_;
  // # of KB read of all probes
  CostScalar prefixKBRead_;
  // accumulated key prdicates for the current prefix
  ValueIdSet prefixKeyPreds_;
  // pointer to key predicates to be applied to the disjunct histograms
  const ValueIdSet *keyPredsPtr2Apply2Histograms_;
  // Current column position
  CollIndex prefixColumnPosition_;
  // Whether we have an equal predicate on the current column
  NABoolean curPredIsEqual_;
  // >>>>>>>>>>>>>>>>>> Previous column related memebers <<<<<<<<<<<<<<<
  // Whether we had an equal predicate on the previous column
  NABoolean prevPredIsEqual_;
  // The uec for previous column after applying predicate on the column
  CostScalar uecForPreviousCol_;
  // The uec for previous column before applying predicate on teh column
  CostScalar uecForPreviousColBeforeAppPreds_;
  // Take into account the distance betweem the uecs and effects of that on
  // cache hit
  CostScalar uecForPrevColForSeeks_;
  NABoolean firstRound_;
  NABoolean crossProductApplied_;
  NABoolean prevColChosen_;
  CostScalar sumOfUecs_; // The sum of the uecs for all columns
  CostScalar sumOfUecsSoFar_;
  CostScalar blocksToReadPerUec_;
  Cost* pMinCost_;

  // the rowcount as a result of applying the first non-empty key predicate
  // (if any). The value is useful to estimate the number of rows returned
  // for all probes into the inner table with leading key columns absent
  // of predicates.
  CostScalar rcAfterApplyFirstKeyPreds_;
};

#ifndef NDEBUG

static Int32
ScanOptimizerTest1(const FileScan& associatedFileScan
                   ,const CostScalar& resultSetCardinality
                   ,const Context& myContext
                   ,const ValueIdSet &externalInputs
                   ,CollHeap* heap)
{
  // Test shared basic cost objects;

  // Don't bother if sharing is disabled
  //
  if (CURRSTMT_OPTDEFAULTS->reuseBasicCost()) {
    FileScanOptimizer scanOpt(associatedFileScan,
                              resultSetCardinality,
                              myContext,
                              externalInputs);

    // Attempt to get a shared basic cost object.
    //
    SearchKey *searchKey = NULL;
    MdamKey *mdamKey = NULL;
    Cost *cost1 = scanOpt.optimize(searchKey,
                                   mdamKey);


    // Make sure that this time the basic cost object is regenerated
    //
    NABoolean reUseBasicCost = CURRSTMT_OPTDEFAULTS->reuseBasicCost();
    CURRSTMT_OPTDEFAULTS->setReuseBasicCost(FALSE);
    searchKey = NULL;
    mdamKey = NULL;
    Cost *cost2 = scanOpt.optimize(searchKey,
                                   mdamKey);
    CURRSTMT_OPTDEFAULTS->setReuseBasicCost(reUseBasicCost);


    if(cost1->compareCosts(*cost2) != SAME) {
      fprintf(stdout,"Test1 Failed ========\n");
      fprintf(stdout,"Cost1 (shared)\n");
      cost1->print();
      fprintf(stdout,"Cost2\n");
      cost2->print();
    } else {
      fprintf(stdout,"Test1 Passed ====\n");
    }
    delete cost1;
    delete cost2;
  }
  return 0;
}

static Int32
ScanOptimizerTest2(const FileScan& associatedFileScan
                   ,const CostScalar& resultSetCardinality
                   ,const Context& myContext
                   ,const ValueIdSet &externalInputs
                   ,CollHeap* heap)
{
  // Test that the simple scan optimizer produces the same cost as the
  // original.

  static Int32 test2Cnt = 0;
  static Int32 totalCnt = 0;
  static Int32 failCnt = 0;

  totalCnt++;
  if(ScanOptimizer::useSimpleFileScanOptimizer(associatedFileScan,
                                               myContext,
                                               externalInputs)) {
    test2Cnt++;
    SimpleFileScanOptimizer simpleScanOpt(associatedFileScan,
                                          resultSetCardinality,
                                          myContext,
                                          externalInputs);

    FileScanOptimizer complexScanOpt(associatedFileScan,
                                     resultSetCardinality,
                                     myContext,
                                     externalInputs);



    // Make sure that the basic cost object is regenerated
    //
    NABoolean reUseBasicCost = CURRSTMT_OPTDEFAULTS->reuseBasicCost();
    CURRSTMT_OPTDEFAULTS->setReuseBasicCost(FALSE);

    SearchKey *searchKey1 = NULL;
    MdamKey *mdamKey1 = NULL;
    Cost *cost1 = simpleScanOpt.optimize(searchKey1,
                                         mdamKey1);

    SearchKey *searchKey2 = NULL;
    MdamKey *mdamKey2 = NULL;
    Cost *cost2 = complexScanOpt.optimize(searchKey2,
                                          mdamKey2);

    CURRSTMT_OPTDEFAULTS->setReuseBasicCost(reUseBasicCost);

    if(mdamKey2) {
      fprintf(stdout,"Test 2.5 Failed MDAM ================\n");

      fprintf(stdout,"Key Columns\n");
      searchKey1->getKeyColumns().display();

      fprintf(stdout,"Key Predicates\n");
      searchKey1->getKeyPredicates().display();

      fprintf(stdout,"Exec Predicates\n");
      searchKey1->getExecutorPredicates().display();

      failCnt++;
    } else if(cost1->compareCosts(*cost2) != SAME) {
      ElapsedTime et1 (cost1->convertToElapsedTime(NULL));
      ElapsedTime et2 (cost2->convertToElapsedTime(NULL));

      CostScalar perDiff;
      if(et1 != et2) {
        perDiff = ((et1 > et2) ? (et1-et2) : (et2-et1))/et2;
      } else {
        CostScalar tc_et1 = cost1->getTotalCost().getElapsedTime
          (*(CURRSTMT_OPTDEFAULTS->getResourcePerformanceGoal()),
            CURRSTMT_OPTDEFAULTS->getDefaultCostWeight());
        CostScalar tc_et2 = cost2->getTotalCost().getElapsedTime
          (*(CURRSTMT_OPTDEFAULTS->getResourcePerformanceGoal()),
           CURRSTMT_OPTDEFAULTS->getDefaultCostWeight());

        perDiff = ((tc_et1 > tc_et2) ? (tc_et1-tc_et2) : (tc_et2-tc_et1))/tc_et2;

      }

      perDiff = perDiff * 100;
      if (perDiff > 2.0) {
        fprintf(stdout,"Test 2 Failed %6.4f %6.4f %6.3f ",
                et1.value(), et2.value(), perDiff.value());

        Int32 i = (Int32)(perDiff.getCeiling().getValue());
        i = ((i > 70) ? 70 : i);
        for(; i > 0; i--) {
          fprintf(stdout, "=");
        }
        fprintf(stdout, "\n");
        failCnt++;

//        fprintf(stdout,"Simple Cost\n");
//        cost1->print();
//        fprintf(stdout,"Complex Cost\n");
//        cost2->print();

        fprintf(stdout,"Test2 Stats (%d:%d:%d)(%d) ====\n", totalCnt, test2Cnt, failCnt,
                (Int32)(100*((float)test2Cnt/(float)totalCnt)));

      } else {
        fprintf(stdout,"Test2 Passed (%d:%d:%d)(%d) %6.3f\n",
                totalCnt, test2Cnt, failCnt, (Int32)(100*((float)test2Cnt/(float)totalCnt)),
                perDiff.value());
      }
    } else {
      fprintf(stdout,"Test2 Passed (%d:%d:%d)(%d) ====\n", totalCnt, test2Cnt, failCnt,
              (Int32)(100*((float)test2Cnt/(float)totalCnt)));
    }
    delete cost1;
    delete cost2;
  }

  return 0;
}

//THREAD_P TaskMonitor* simpleFSOMonPtr = NULL;
//THREAD_P TaskMonitor* complexFSOMonPtr = NULL;

static Int32
ScanOptimizerTest3(const FileScan& associatedFileScan
                   ,const CostScalar& resultSetCardinality
                   ,const Context& myContext
                   ,const ValueIdSet &externalInputs
                   ,CollHeap* heap)
{
  // Test the performance of the simple scan optimizer compared to the
  // original.

  if(ScanOptimizer::useSimpleFileScanOptimizer(associatedFileScan,
                                               myContext,
                                               externalInputs)) {

    SimpleFileScanOptimizer simpleScanOpt(associatedFileScan,
                                          resultSetCardinality,
                                          myContext,
                                          externalInputs);

    FileScanOptimizer complexScanOpt(associatedFileScan,
                                     resultSetCardinality,
                                     myContext,
                                     externalInputs);


    // Make sure that the basic cost object is regenerated
    //
    NABoolean reUseBasicCost = CURRSTMT_OPTDEFAULTS->reuseBasicCost();
    CURRSTMT_OPTDEFAULTS->setReuseBasicCost(FALSE);

    CURRENTSTMT->getSimpleFSOMonPtr()->enter();

    Int32 i;
    for(i = 0; i < 20; i++) {
      SearchKey *searchKey1 = NULL;
      MdamKey *mdamKey1 = NULL;
      Cost *cost1 = simpleScanOpt.optimize(searchKey1,
                                           mdamKey1);
      delete cost1;
    }
    CURRENTSTMT->getSimpleFSOMonPtr()->exit();

    CURRENTSTMT->getComplexFSOMonPtr()->enter();

    for(i = 0; i < 20; i++) {
      SearchKey *searchKey2 = NULL;
      MdamKey *mdamKey2 = NULL;
      Cost *cost2 = complexScanOpt.optimize(searchKey2,
                                            mdamKey2);
      delete cost2;
    }
    CURRENTSTMT->getComplexFSOMonPtr()->exit();
    CURRSTMT_OPTDEFAULTS->setReuseBasicCost(reUseBasicCost);

    cout << "Test3  simpleFSO  : "<< *(CURRENTSTMT->getSimpleFSOMonPtr()) << endl;
    cout << "Test3  complexFSO : "<< *(CURRENTSTMT->getComplexFSOMonPtr()) << endl;

  }
  return 0;
}

static Int32
ScanOptimizerTest4(const FileScan& associatedFileScan
                   ,const CostScalar& resultSetCardinality
                   ,const Context& myContext
                   ,const ValueIdSet &externalInputs
                   ,CollHeap* heap)
{
  // Test that the simple scan optimizer produces the same cost as the
  // original.

  static Int32 test4Cnt = 0;
  static Int32 totalCnt = 0;
  static Int32 failCnt = 0;

  if(!ScanOptimizer::useSimpleFileScanOptimizer(associatedFileScan,
                                                myContext,
                                                externalInputs)) {

    totalCnt++;

    FileScanOptimizer complexScanOpt(associatedFileScan,
                                     resultSetCardinality,
                                     myContext,
                                     externalInputs);


    // Make sure that the basic cost object is regenerated
    //
    NABoolean reUseBasicCost = CURRSTMT_OPTDEFAULTS->reuseBasicCost();
    CURRSTMT_OPTDEFAULTS->setReuseBasicCost(FALSE);

    SearchKey *searchKey2 = NULL;
    MdamKey *mdamKey2 = NULL;
    Cost *cost2 = complexScanOpt.optimize(searchKey2,
                                          mdamKey2);

    CURRSTMT_OPTDEFAULTS->setReuseBasicCost(reUseBasicCost);

    // If didn't choose SimpleScanOptimizer, but could have (should have)...
    //
    if(searchKey2 &&
        (myContext.getInputLogProp()->getColStats().entries() == 0))
    {
      test4Cnt++;
      SimpleFileScanOptimizer simpleScanOpt(associatedFileScan,
                                            resultSetCardinality,
                                            myContext,
                                            externalInputs);

      // Make sure that the basic cost object is regenerated
      //
      NABoolean reUseBasicCost = CURRSTMT_OPTDEFAULTS->reuseBasicCost();
      CURRSTMT_OPTDEFAULTS->setReuseBasicCost(FALSE);

      SearchKey *searchKey1 = NULL;
      MdamKey *mdamKey1 = NULL;
      Cost *cost1 = simpleScanOpt.optimize(searchKey1,
                                           mdamKey1);

      CURRSTMT_OPTDEFAULTS->setReuseBasicCost(reUseBasicCost);

      if(cost1->compareCosts(*cost2) != SAME) {
        ElapsedTime et1 (cost1->convertToElapsedTime(NULL));
        ElapsedTime et2 (cost2->convertToElapsedTime(NULL));

        CostScalar perDiff;
        if(et1 != et2) {
          perDiff = ((et1 > et2) ? (et1-et2) : (et2-et1))/et2;
        } else {
          CostScalar tc_et1 = cost1->getTotalCost().getElapsedTime
            (*(CURRSTMT_OPTDEFAULTS->getResourcePerformanceGoal()),
             CURRSTMT_OPTDEFAULTS->getDefaultCostWeight());
          CostScalar tc_et2 = cost2->getTotalCost().getElapsedTime
            ( *(CURRSTMT_OPTDEFAULTS->getResourcePerformanceGoal()),
              CURRSTMT_OPTDEFAULTS->getDefaultCostWeight() );

          perDiff = ((tc_et1 > tc_et2) ? (tc_et1-tc_et2)
                                        : (tc_et2-tc_et1))/tc_et2;

        }

        perDiff = perDiff * 100;
        if (perDiff > 2.0) {
          fprintf(stdout,"Test 4 Failed %6.4f %6.4f %6.3f ",
                  et1.value(), et2.value(), perDiff.value());

          Int32 i = (Int32)(perDiff.getCeiling().getValue());
          i = ((i > 70) ? 70 : i);
          for(; i > 0; i--) {
            fprintf(stdout, "=");
          }
          fprintf(stdout, "\n");
          failCnt++;
        } else {
          fprintf(stdout,"Test4 Passed (%d:%d:%d) %6.3f\n",
                  totalCnt, test4Cnt, failCnt, perDiff.value());
        }
      } else {
        fprintf(stdout,"Test4 Passed (%d:%d:%d) ====\n",
            totalCnt, test4Cnt, failCnt);
      }
      delete cost1;
    }
    delete cost2;

  }
  return 0;
}

static Int32
ScanOptimizerTest5(const FileScan& associatedFileScan
                   ,const CostScalar& resultSetCardinality
                   ,const Context& myContext
                   ,const ValueIdSet &externalInputs
                   ,CollHeap* heap)
{
  // Test that the simple scan optimizer produces the same cost as the
  // original for cases of Multiprobe.

  static Int32 test5Cnt = 0;
  static Int32 totalCnt = 0;
  static Int32 failCnt = 0;

  totalCnt++;
  if(ScanOptimizer::useSimpleFileScanOptimizer(associatedFileScan,
                                               myContext,
                                               externalInputs)) {

    CostScalar repeatCount = myContext.getPlan()->getPhysicalProperty()->
      getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2();

    if ((repeatCount > 1.0) OR
        (myContext.getInputLogProp()->getColStats().entries() > 0)) {

      test5Cnt++;
      SimpleFileScanOptimizer simpleScanOpt(associatedFileScan,
                                            resultSetCardinality,
                                            myContext,
                                            externalInputs);

      FileScanOptimizer complexScanOpt(associatedFileScan,
                                       resultSetCardinality,
                                       myContext,
                                       externalInputs);



      // Make sure that the basic cost object is regenerated
      //
      NABoolean reUseBasicCost = CURRSTMT_OPTDEFAULTS->reuseBasicCost();
      CURRSTMT_OPTDEFAULTS->setReuseBasicCost(FALSE);

      SearchKey *searchKey1 = NULL;
      MdamKey *mdamKey1 = NULL;
      Cost *cost1 = simpleScanOpt.optimize(searchKey1,
                                           mdamKey1);

      SearchKey *searchKey2 = NULL;
      MdamKey *mdamKey2 = NULL;
      Cost *cost2 = complexScanOpt.optimize(searchKey2,
                                            mdamKey2);

      CURRSTMT_OPTDEFAULTS->setReuseBasicCost(reUseBasicCost);

      if(mdamKey2) {
        fprintf(stdout,"Test 5.5 Failed MDAM ================\n");

        fprintf(stdout,"Key Columns\n");
        searchKey1->getKeyColumns().display();

        fprintf(stdout,"Key Predicates\n");
        searchKey1->getKeyPredicates().display();

        fprintf(stdout,"Exec Predicates\n");
        searchKey1->getExecutorPredicates().display();

        failCnt++;
      } else if(cost1->compareCosts(*cost2) != SAME) {
        ElapsedTime et1 (cost1->convertToElapsedTime(NULL));
        ElapsedTime et2 (cost2->convertToElapsedTime(NULL));

        CostScalar perDiff;
        if(et1 != et2) {
          perDiff = ((et1 > et2) ? (et1-et2) : (et2-et1))/et2;
        } else {
          CostScalar tc_et1 = cost1->getTotalCost().getElapsedTime
            (*(CURRSTMT_OPTDEFAULTS->getResourcePerformanceGoal()),
            CURRSTMT_OPTDEFAULTS->getDefaultCostWeight());
          CostScalar tc_et2 = cost2->getTotalCost().getElapsedTime
            (*(CURRSTMT_OPTDEFAULTS->getResourcePerformanceGoal()),
             CURRSTMT_OPTDEFAULTS->getDefaultCostWeight());

          perDiff = ((tc_et1 > tc_et2) ? (tc_et1-tc_et2) : (tc_et2-tc_et1))/tc_et2;

        }

        perDiff = perDiff * 100;
        if (perDiff > 2.0) {
          fprintf(stdout,"Test 5 Failed %6.4f %6.4f %6.3f ",
                  et1.value(), et2.value(), perDiff.value());

          Int32 i = (Int32)(perDiff.getCeiling().getValue());
          i = ((i > 70) ? 70 : i);
          for(; i > 0; i--) {
            fprintf(stdout, "=");
          }
          fprintf(stdout, "\n");
          failCnt++;

          //        fprintf(stdout,"Simple Cost\n");
          //        cost1->print();
          //        fprintf(stdout,"Complex Cost\n");
          //        cost2->print();

          fprintf(stdout,"Test5 Stats (%d:%d:%d)(%d) ====\n", totalCnt, test5Cnt, failCnt,
                  (Int32)(100*((float)test5Cnt/(float)totalCnt)));

        } else {
          fprintf(stdout,"Test5 Passed (%d:%d:%d)(%d) %6.3f\n",
                  totalCnt, test5Cnt, failCnt, (Int32)(100*((float)test5Cnt/(float)totalCnt)), perDiff.value());
        }
      } else {
        fprintf(stdout,"Test5 Passed (%d:%d:%d)(%d) ====\n", totalCnt, test5Cnt, failCnt,
                (Int32)(100*((float)test5Cnt/(float)totalCnt)));
      }
      delete cost1;
      delete cost2;
    }
  }

  return 0;
}

static Int32
ScanOptimizerAllTests(const FileScan& associatedFileScan
                      ,const CostScalar& resultSetCardinality
                      ,const Context& myContext
                      ,const ValueIdSet &externalInputs
                      ,CollHeap* heap)

{
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  ULng32 testsToRun = defs.getAsULong(FSO_RUN_TESTS);

  if(testsToRun & 0x01) {
    ScanOptimizerTest1(associatedFileScan
                       ,resultSetCardinality
                       ,myContext
                       ,externalInputs
                       ,heap);
  }

  if(testsToRun & 0x02) {
    ScanOptimizerTest2(associatedFileScan
                       ,resultSetCardinality
                       ,myContext
                       ,externalInputs
                       ,heap);
  }

  if(testsToRun & 0x04) {
    ScanOptimizerTest3(associatedFileScan
                       ,resultSetCardinality
                       ,myContext
                       ,externalInputs
                       ,heap);
  }

  if(testsToRun & 0x08) {
    ScanOptimizerTest4(associatedFileScan
                       ,resultSetCardinality
                       ,myContext
                       ,externalInputs
                       ,heap);
  }

  if(testsToRun & 0x10) {
    ScanOptimizerTest5(associatedFileScan
                       ,resultSetCardinality
                       ,myContext
                       ,externalInputs
                       ,heap);
  }

  return 0;
}
#endif


// -----------------------------------------------------------------------
// getDp2CacheSizeInBlocks
//   Given a block size, this method returns the number of blocks in
// cache for blocks of that size.
// -----------------------------------------------------------------------
CostScalar
getDP2CacheSizeInBlocks(const CostScalar& blockSizeInKb)
{
    CostScalar cacheSizeInBlocks = csZero;

    if (blockSizeInKb >= 64.)
      cacheSizeInBlocks = 
        CostPrimitives::getBasicCostFactor(NCM_CACHE_SIZE_IN_BLOCKS);
    else if (blockSizeInKb == 32.)
      cacheSizeInBlocks =
        CostPrimitives::getBasicCostFactor(DP2_CACHE_32K_BLOCKS);
    else if (blockSizeInKb == 16.)
      cacheSizeInBlocks =
        CostPrimitives::getBasicCostFactor(DP2_CACHE_16K_BLOCKS);
    else if (blockSizeInKb == 8.)
      cacheSizeInBlocks =
        CostPrimitives::getBasicCostFactor(DP2_CACHE_8K_BLOCKS);
    else if (blockSizeInKb == 4.)
      cacheSizeInBlocks =
        CostPrimitives::getBasicCostFactor(DP2_CACHE_4096_BLOCKS);
    else if (blockSizeInKb == 2.)
      cacheSizeInBlocks =
        CostPrimitives::getBasicCostFactor(DP2_CACHE_2048_BLOCKS);
    else if (blockSizeInKb == 1.)
      cacheSizeInBlocks =
        CostPrimitives::getBasicCostFactor(DP2_CACHE_1024_BLOCKS);
    else if (blockSizeInKb == 0.5)
      cacheSizeInBlocks =
        CostPrimitives::getBasicCostFactor(DP2_CACHE_512_BLOCKS);
    else
      cacheSizeInBlocks =
        CostPrimitives::getBasicCostFactor(NCM_CACHE_SIZE_IN_BLOCKS);

  return cacheSizeInBlocks;

}

// -----------------------------------------------------------------------
//  removeConstantsFromTargetSortKey
//
//    This method removes columns that are covered by constants on the left
//  (source) side of a binary operator from the passed in right child
// (target) sort key. This is useful for example, for the following scenario:
//   create table foo (pnum int, primary key pnum);
//   create table bar (empnum int, pnum int, primary key (empnum, pnum));
//   insert into bar select 100,pnum from foo;
//
//   Target column empnum is equivalent to 100 on the source side but
// not on the target side. Still, all the PROBES to the target table
// will have their empnum value equal to 100. If we can realize this on
// the target side then the ordersMatch method will be able to determine
// that the probes are completely in order. This is genesis case
// 10-000925-2516.  If we don't do this the ordersMatch method could
// fail an assertion because we are counting on columns on one side
// of the map that are covered by constants are also covered by
// constants on the other side of the map.
//
//
// INPUT PARAMS:
//   targetSortKey: The right child sort key columns
//   map : the map that maps values from one side of the op to the other
//
// OUTPUT PARAMS:
//   rightChildSortKey, possibly modified.
//
// RETURN VALUE:
//   None.
// -----------------------------------------------------------------------
void
removeConstantsFromTargetSortKey(ValueIdList* targetSortKey,
                                 ValueIdMap* map)
{
  // Map the target sort key cols to the source
  ValueIdList mappedTargetSortKey;
  map->rewriteValueIdListDown(
         *targetSortKey,
         mappedTargetSortKey);
  // Remove from the mapped sort key any cols that are equal to constants
  ValueIdSet charInputs; // empty : we only want to remove constants
  mappedTargetSortKey.removeCoveredExprs(charInputs);
  if (mappedTargetSortKey.entries() < targetSortKey->entries())
  {
    // Map back the mapped target sort key cols
    ValueIdList remappedTargetSortKey;
    map->rewriteValueIdListUp(
           remappedTargetSortKey,
           mappedTargetSortKey);
    // Set the target sort key to the version with columns covered by
    // constants now removed
    *targetSortKey = remappedTargetSortKey;
  }

}

//  isOrderedNJFeasible
//
//  This method checks to see if leading column of left child and
//  right child of a NJ are same. This method gets called from
//  insert, update, delete computeOperatorCost methods if IPP is
//  being passed. The reason for calling this method is these operators
//  manipulate left and right sort keys. If we don't make sure atleast
//  leading key cols are same then ordersMatch method will assert.

// INPUT PARAMS:
//   leftKeys: The left child sort key columns
//   rightKeys: The right child sort key columns
//
//
// RETURN VALUE:
//  TRUE if leading cols are same, FALSE otherwise.

NABoolean
isOrderedNJFeasible (ValueIdList leftKeys, ValueIdList rightKeys)
{
  if (leftKeys.isEmpty() || rightKeys.isEmpty())
    return FALSE;

  ValueId lKeyCol = leftKeys[0];
  // Remove any inverse node on the leading left table
  ValueId noInverseLKeyCol =
    lKeyCol.getItemExpr()->removeInverseOrder()->getValueId();
  NABoolean lKeyColIsDesc = FALSE;
  if (noInverseLKeyCol != lKeyCol)
    lKeyColIsDesc = TRUE;

  ValueId rKeyCol = rightKeys[0];
  // Remove any inverse node on the leading right table
  ValueId noInverseRKeyCol =
    rKeyCol.getItemExpr()->removeInverseOrder()->getValueId();
  NABoolean rKeyColIsDesc = FALSE;
  if (noInverseRKeyCol != rKeyCol)
    rKeyColIsDesc = TRUE;

  // Return orderedNJ as not feasible for divisioning columns 
  // where the order is set to DESCENDING. 
  if(lKeyColIsDesc || rKeyColIsDesc)
  {
    if (lKeyColIsDesc)
    {
      BaseColumn *lBaseColumn = noInverseLKeyCol.castToBaseColumn();
      if(lBaseColumn && lBaseColumn->getNAColumn()->isComputedColumn())
	return FALSE;
    }
    if (rKeyColIsDesc)
    {
      BaseColumn *rBaseColumn = noInverseRKeyCol.castToBaseColumn();
      if(rBaseColumn && rBaseColumn->getNAColumn()->isComputedColumn())
	return FALSE;
    }
  }

  // Leading column of the left table sort key and the
  // leading column of the right table sort key must be
  // the same. If one is DESC, they must both be DESC.
  if ((noInverseLKeyCol == noInverseRKeyCol) AND
      (lKeyColIsDesc == rKeyColIsDesc))
    return TRUE;
  else
    return FALSE;
}

// -----------------------------------------------------------------------
//  ordersMatch method
//    This method determines if the inner table sort key is in the same
//  order as the outer table sort key. Only applicable when processing
//  the right child of a nested join operator.
//
// INPUT PARAMS:
//   ipp: The input physical properties
//   indexDesc: The index descriptor of the access path
//   innerOrder : the sort key for the access path
//   charInputs: The characteristic inputs for this operator
//   partiallyInOrderOK: TRUE if the order can be used even if the probes
//                       are not completely in order. This will be TRUE
//                       for read, update and delete, and FALSE for insert.
// OUTPUT PARAMS:
//   probesForceSynchronousAccess: TRUE if the probes are completely
//     in order across multiple partitions and the clustering key is
//     the same as the partitioning key. FALSE otherwise.
// RETURN VALUE:
//   TRUE if there is a match between the probes order and the
//   table's (or index's) order. FALSE otherwise.
// -----------------------------------------------------------------------
NABoolean
ordersMatch(const InputPhysicalProperty* ipp,
            const IndexDesc* indexDesc,
            const ValueIdList* innerOrder,
            const ValueIdSet& charInputs,
            NABoolean partiallyInOrderOK,
            NABoolean& probesForceSynchronousAccess,
	    NABoolean noCmpAssert)
{

  ValueIdList innerOrderProbeCols;
  // temporary var to keep column Ids without inverse for possible
  // use to get UEC for this column from histograms
  ValueIdList innerOrderProbeColsNoInv;

  CollIndex numInOrderCols = 0;

  NABoolean partiallyInOrder = FALSE;
  NABoolean fullyInOrder = FALSE;

  probesForceSynchronousAccess = FALSE;

  if ((ipp != NULL) AND (!(ipp->getAssumeSortedForCosting())) AND
     (!(ipp->getExplodedOcbJoinForCosting())))
  {
    // Shouldn't have an ipp if there are no outer order columns!
    if ((ipp->getNjOuterOrder() == NULL) OR
         ipp->getNjOuterOrder()->isEmpty())
    {
      if (NOT noCmpAssert)
	CCMPASSERT(FALSE);
      return FALSE;
    }
    // An ipp should also have the outer expression partitioning function!
    if (ipp->getNjOuterOrderPartFunc() == NULL)
    {
      if (NOT noCmpAssert)
	CCMPASSERT(FALSE);
      return FALSE;
    }
    // Should not have passed the ipp if this access path could not
    // use the outer order, so there MUST be at least one sort key column!
    if (innerOrder->isEmpty())
    {
      if (NOT noCmpAssert)
	CCMPASSERT(FALSE);
      return FALSE;
    }

    // Get the physical partitioning function for the access path
    const PartitioningFunction* physicalPartFunc =
      indexDesc->getPartitioningFunction();
    // If the outer order is not a DP2 sort order, and the access path
    // is range partitioned, then need to check if the probes will force
    // synchronous access to the partitions of this access path. The
    // probes will force synchronous access if the leading partitioning
    // key column is the same as the leading clustering key column.
    if ((ipp->getNjDp2OuterOrderPartFunc() == NULL) AND
        (physicalPartFunc != NULL)) // will be NULL for unpart tables
    {
      const RangePartitioningFunction* rpf =
        physicalPartFunc->castToRangePartitioningFunction();
      if (rpf != NULL)
      {
        ValueIdList partKeyAsList;
        // Get the partitioning key as a list
        partKeyAsList = rpf->getKeyColumnList();
        CCMPASSERT(NOT partKeyAsList.isEmpty());

        // Get the leading partitioning key column
        ValueId leadingPartKeyCol = partKeyAsList[0];

        // Get the leading clustering key column - remove any INVERSE node
        ValueId leadingClustKeyCol = (*innerOrder)[0];
        leadingClustKeyCol =
          leadingClustKeyCol.getItemExpr()->removeInverseOrder()->getValueId();

        if (leadingClustKeyCol == leadingPartKeyCol)
          probesForceSynchronousAccess = TRUE;
      } // end if a range partitioned table
    } // end if not a DP2 sort order and a partitioned table

    // Determine which columns of the index sort key are probe columns,
    // up to the first column not covered by a constant or probe column.
    // To do this we call the "findNJEquiJoinCols" method. The equijoin
    // cols are the probe columns, i.e. the values coming from the outer
    // child. For read these will be the equijoin columns, for write
    // they are the key values of the records that need to be written.
    //
    // Note that for write, all the call to this method really does
    // is eliminate any columns that are covered by constants or
    // params/host vars. This is because all key columns will
    // always be probe columns, since we always need all the key cols
    // to accurately determine the record to insert/update/delete. So,
    // we could just call the method "removeCoveredExprs" for write.
    // This would require adding another parameter to the ordersMatch
    // method to distinguish write from read. This is considered
    // undesirable, and so is not done.

    ValueIdList uncoveredCols;
    innerOrderProbeCols =
      innerOrder->findNJEquiJoinCols(
        ipp->getNjOuterCharOutputs(),
        charInputs,
        uncoveredCols);

    // There MUST be some probe columns, otherwise there should not
    // have been an ipp!
    if (innerOrderProbeCols.isEmpty())
    {
      if (NOT noCmpAssert)
	CCMPASSERT(FALSE);
      return FALSE;
    }

    ValueIdList njOuterOrder = *(ipp->getNjOuterOrder());
    // Sol 10-040303-3781. The number of entries of innerOrderProbCols(5)
    // could be greater than of njOuterOrder(3). In this case we hit ABORT
    // in Collections.cpp for unused element of njOuterOrder. The
    // following restriction on loop iteration prevents it. We need to
    // investigate details why we got innerOredrProbCols bigger than
    // njOuetrOredr iin the first place. That corresponding case will
    // be created.
    CollIndex maxInOrderCols =
        MINOF(njOuterOrder.entries(), innerOrderProbeCols.entries());
    fullyInOrder = TRUE;

    // Determine if the leading inner order column and the leading
    // column of the outer order are the same.
    ValueId innerOrderCol;
    ValueId noInverseInnerOrderCol;
    NABoolean innerOrderColIsDesc = FALSE;
    ValueId outerOrderCol;
    ValueId noInverseOuterOrderCol;
    NABoolean outerOrderColIsDesc = FALSE;

    do
    {
      // Remove any inverse node on the inner order column
      // and remember if there was one.
      innerOrderCol = innerOrderProbeCols[numInOrderCols];
      noInverseInnerOrderCol =
        innerOrderCol.getItemExpr()->removeInverseOrder()->getValueId();
      innerOrderProbeColsNoInv.insert(noInverseInnerOrderCol);
      innerOrderColIsDesc = FALSE;
      if (noInverseInnerOrderCol != innerOrderCol)
        innerOrderColIsDesc = TRUE;

      // Remove any inverse node on the leading outer order column
      // and remember if there was one.
      outerOrderCol = njOuterOrder[numInOrderCols];
      noInverseOuterOrderCol =
        outerOrderCol.getItemExpr()->removeInverseOrder()->getValueId();
      outerOrderColIsDesc = FALSE;
      if (noInverseOuterOrderCol != outerOrderCol)
        outerOrderColIsDesc = TRUE;

      // The column of the inner table sort key and the
      // the column of the outer table sort key must be
      // the same. If one is DESC, they must both be DESC.
      if ((noInverseInnerOrderCol != noInverseOuterOrderCol) OR
          (innerOrderColIsDesc != outerOrderColIsDesc))
        fullyInOrder = FALSE;
      else if (numInOrderCols == 0)
      {
        // The leading inner order column is in the same order as the
        // leading outer order column, so the probes are at least
        // partially in order. If all remaining inner order columns
        // are in order then the probes will be completely in order.
        partiallyInOrder = TRUE;
        // If there are fewer inner order columns than outer order columns,
        // then it is possible the probes are completely in order. Set
        // the flag so we will continue looping to compare any
        // remaining inner order columns.
        //if (innerOrderProbeCols.entries() <= njOuterOrder.entries())
        //  fullyInOrder = TRUE;
      }

      numInOrderCols++; // advance to the next sortkey column, if any
    } while ((numInOrderCols < maxInOrderCols) AND fullyInOrder);

    // Since there is an ipp, the probes must be at least partially in the
    // same order,  because we checked this before passing the ipp!
    if (NOT partiallyInOrder)
    {
      if (NOT noCmpAssert)
	CCMPASSERT(FALSE);
      return FALSE;
    }
  } // end if ipp exists


  if (fullyInOrder)
  {
    return TRUE;
  }
  else if (partiallyInOrder AND partiallyInOrderOK)
  {
    // Compute the cardinality of the in-order inner order columns.
    // This is the rowcount of the table divided by the unique entry
    // count (uec) of the in-order columns, i.e. the total number
    // of rows for each in-order column value. Divide this by the
    // rows per block to arrive at the total number of blocks that
    // might need to be read before all rows from a given block
    // are read. If this number of blocks is smaller than the cache
    // size, then we can cost this the same as the completely
    // in order case. So return TRUE. Otherwise, we must cost the
    // same as in the completely un-ordered case, so return FALSE.

    // Get a list of colstats. Each colstats contains the
    // histogram data for one column of the table.
    const ColStatDescList& csdl =
      indexDesc->getPrimaryTableDesc()->getTableColStats();

    CollIndex baseTableColIndex;
    CostScalar currentColUec;
    CostScalar totalInOrderColsUec = csOne;

    // We could get the rowcount from any of the columns, so just
    // arbitrarily get it from column 0.
    CostScalar rowcount = csdl[0]->getColStats()->getRowcount();

    // Compute the total number of unique values for all the in-order
    // key columns.
    for (CollIndex keyColIndex = 0; keyColIndex < numInOrderCols; keyColIndex++)
    {
      // Sol 10-040303-3781. Previously we were retrieving columns from
      // innerOrderProbeCols list.  The column 0 of innerOrderProbCols had the
      // type ITM_INVERSE. This type was not processed in getColStatDescForColumn,
      // as a result baseTableColIndex was left uninitialized and we hit
      // ABORT in Collections.cpp. Another change - if column cannot be found
      // and getColStatDescIndexForColumn returns FALSE we should consider it
      // as situation with m=not matching columns and return FALSE as we did
      // already many times in this method.

      if (NOT csdl.getColStatDescIndexForColumn(
            baseTableColIndex, innerOrderProbeColsNoInv[keyColIndex])
         )
      {
	if (NOT noCmpAssert)
	  CCMPASSERT(FALSE);
        return FALSE;
      }
      currentColUec = csdl[baseTableColIndex]->getColStats()->getTotalUec();
      totalInOrderColsUec = totalInOrderColsUec * currentColUec;
    }

    // Divide the rowcount by the total uec for the in-order cols to
    // get the # of rows for each unique in-order key value.
    CostScalar rowcountPerKeyValue =
      (rowcount / totalInOrderColsUec).getCeiling();

    CostScalar blockSizeInKb = indexDesc->getBlockSizeInKb();
    // Get the # of blocks for each unique in-order key value.
    CostScalar rowsPerBlock =
      (blockSizeInKb / indexDesc->getRecordSizeInKb()).getFloor();
    CostScalar blocksPerKeyValue =
      (rowcountPerKeyValue /  rowsPerBlock).getCeiling();

    // Get the # of cache blocks available for this size of data blocks.
    CostScalar cacheSizeInBlocks = getDP2CacheSizeInBlocks(blockSizeInKb);

    // If the # of blocks for each unique in-order key value fits in
    // cache, then we will never have to read any block twice. Return TRUE.
    if (blocksPerKeyValue <= cacheSizeInBlocks)
      return TRUE;
    else
      return FALSE;
  } // end if partially in order
  else
  {
    return FALSE;
  }

} // ordersMatch()

// This should be a member of ItemExpr...
// -----------------------------------------------------------------------
// INPUT:
//      columnId: A valueId that represents the column. It has to
//      be either a VegRef, a base column, or a index column.
//
//      equalityPredId: A valueid that represents the predicate
//      it has to be either a VegPred or an Equality pred.
//
// OUTPUT:
//      TRUE if the pred references the
//      columnId in any of its operands.
//
// -----------------------------------------------------------------------
static NABoolean
predReferencesColumn(const ItemExpr *predIEPtr
                     ,const ValueId& columnId)
{

  NABoolean itDoes = FALSE;

  DCMPASSERT(predIEPtr->isAPredicate());

  // If the columnId is a VEGRef expand it
  // and prove that some of its members
  // reference the VEGPred, if it is a join
  // pred:
  const ItemExpr *colIdIEPtr = columnId.getItemExpr();
  ValueIdSet columnSet;
  if (colIdIEPtr->getOperatorType()
      ==
      ITM_VEG_REFERENCE)
    {
      const VEG *exprVEGPtr =
                        ((VEGReference *)colIdIEPtr)->getVEG();
      columnSet  = exprVEGPtr->getAllValues();
    }
  else
    {
      // it must be a column:
      columnSet.insert(columnId);
    }

  // This loop is only here because of the
  // posibility of the columnId being
  // a VEGReference, in which case
  // we need to test each member of its
  // VEGGroup for being referenced by the
  // predicate:
  for(ValueId colId=columnSet.init();
      columnSet.next(colId);
      columnSet.advance(colId))
    {
      if (predIEPtr->referencesTheGivenValue(colId))
        {
          // exit the loop:
          itDoes = TRUE;
          break;
        }
    } // for every referenced column

  return itDoes;
}


// -----------------------------------------------------------------------
// This method computes an upper bound for the total blocks in
// a table
// -----------------------------------------------------------------------
static void
computeBlocksUpperBound(
     CostScalar &totalBlocksUpperBound /* out */
     ,const CostScalar& totalRows
     ,const CostScalar& recordsPerBlock)
{
  totalBlocksUpperBound =
    CostScalar( totalRows / recordsPerBlock).getCeiling();
}

// -----------------------------------------------------------------------
// This method computes how many subsets will be read
// -----------------------------------------------------------------------

static void
computeBeginBlocksLowerBound(
     CostScalar &beginBlocksLowerBound /* out */
     ,const CostScalar& uniqueSuccDataRqsts
     ,const CostScalar& blocksUpperBound)

{
  beginBlocksLowerBound =
    CostScalar(MINOF(uniqueSuccDataRqsts.getCeiling().getValue(),
                     blocksUpperBound.getValue()));
}




// -----------------------------------------------------------------------
// This method computes the blocks to be read
// -----------------------------------------------------------------------

static void
computeTotalBlocksLowerBound(
     CostScalar &totalBlocksLowerBound /* out */
     ,const CostScalar& uniqueSuccDataRqsts
     ,const CostScalar& rowsPerSuccRqst
     ,const CostScalar& recordsPerBlock
     ,const CostScalar& innerBlocksUpperBound
     )
{

  // -----------------------------------------------------------------------
  // This formula was updated after the code review of Scan costing
  // The 0.5 below (and the
  // getFloor()) are added to compute the ROUND of the expression.
  // -----------------------------------------------------------------------

  totalBlocksLowerBound =
    MINOF( ( uniqueSuccDataRqsts
             +
             (uniqueSuccDataRqsts
              *
              ((rowsPerSuccRqst.getCeiling()-1)/recordsPerBlock)
              +
              CostScalar(0.5)).getFloor() ),
           innerBlocksUpperBound );
}


#ifndef NDEBUG

void
ScanOptimizer::printCostObject(const Cost * costPtr) const
{
  if (costPtr)
  {
//    printf("Cost Vector: ");
//    costPtr->print();

    printf("Elapsed time: ");
    ElapsedTime et =
      costPtr->
      convertToElapsedTime(getContext().
      getReqdPhysicalProperty());
      if (et <= 0.0)
      {
	FSOWARNING("negative or zero elapsed time");
      }
      else
      {
	printf("%f", et.getValue());
      }
  }
  else
  {
    printf("NULL Cost");
  }
  printf("\n");
}

#endif


//-------------------------------------------------------
// Methods for Histograms
//-----------------------------------------------------

// -----------------------------------------------------------------------
// Input:
// const TableDesc& tableDesc: the tableDesc for the base table
//    associated with this Scan
// -----------------------------------------------------------------------
Histograms::Histograms(  const ColStatDescList& colStatDescList )
           :colStatDescList_(CmpCommon::statementHeap())
{
  // -----------------------------------------------------------------------
  // Create a list of Histogram for the
  // columns of tableDesc
  // -----------------------------------------------------------------------

  // The following creates a deep copy of all the ScanHistograms
  // associated with the table:
  colStatDescList_.makeDeepCopy (colStatDescList) ;
} // Histograms(...)

void
Histograms::append(const ColStatDescSharedPtr& colStatDesc)
{
  colStatDescList_.insertDeepCopyAt(entries(), colStatDesc);
}


Histograms::~Histograms()
{
  // Delete every ColStatDescList that was inserted into the histogram list:
  // colStatDescList_ should be responsible for this!
  /*
    for (CollIndex i=0; i <= entries(); i++)
    {
      delete colStatDescList_[i];
    }
    */
}

CostScalar
Histograms::getRowCount() const
{
  DCMPASSERT(getColStatDescList().entries() > 0);
  return getColStatDescList()[0]->getColStats()->getRowcount();
}


const ColStatDescList&
Histograms::getColStatDescList() const
{
  return colStatDescList_;
}


NABoolean
Histograms::containsAtLeastOneFake() const
{
  return colStatDescList_.containsAtLeastOneFake();
} // containsAtLeastOneFake()


NABoolean
Histograms::getColStatDescForColumn(CollIndex index,
				    const ValueId& column) const
{
  return getColStatDescList().getColStatDescIndexForColumn(index,column);
}


const ColStats&
Histograms::getColStatsForColumn(const ValueId& column) const
{
  ColStatsSharedPtr colStatsPtr =
    getColStatDescList().getColStatsPtrForColumn(column);

  // There must be statistics for ALL columns
  // (even though some of them may contain fake histograms)
  DCMPASSERT(colStatsPtr != NULL);

  return *colStatsPtr;

}// getColStatsForColumn(...)

CostScalar Histograms::getUecCountForColumns(const ValueIdSet& key) const
{
  if(key.entries() >1)
    if ( getColStatDescList().getUecList() )
       return getColStatDescList().getUecList()->lookup(key);
    else
       return csZero;
  else
  {
    ValueId vid;
    key.getFirst(vid);
    return getColStatsForColumn(vid).getTotalUec().getCeiling();
  }
}

ColStatsSharedPtr
Histograms::getColStatsPtrForColumn(const ValueId& column) const
{
  // $$$ This was added to cover the case when
  // $$$ no col stat is available. This is true
  // $$$ today sometimes due to prototype code...
  ColStatsSharedPtr colStatsPtr =
    getColStatDescList().getColStatsPtrForColumn(column);

  return colStatsPtr;

}// getColStatsForColumn(...)

void
Histograms::displayHistogramForColumn(const ValueId& column) const
{
  getColStatsForColumn(column).display();

}// getColStatsForColumn(...)

void
Histograms::applyPredicates(const ValueIdSet& predicates, 
                            const RelExpr & scan,
			    const SelectivityHint * selHint, 
			    const CardinalityHint * cardHint,
			    OperatorTypeEnum opType
                            )
{
   CostScalar initialRowCount = getRowCount();
   // No need to apply preds to empty histograms:
   if (initialRowCount.isGreaterThanZero())
     {

       ValueIdSet outerRefs;
       CollIndex numOuterColStats = 0;
       ValueIdSet unresolvedPreds;
        colStatDescList_.estimateCardinality(initialRowCount,
                                            predicates,
                                            outerRefs,
                                            NULL,
                                            selHint,
                                            cardHint,
                                            numOuterColStats,
                                            unresolvedPreds,
                                            INNER_JOIN_MERGE,
                                            opType,
                                            NULL);
     }

     // check to see if the rowcount computed from histogram lies within
     // the range obtained after executing predicates on a sample
     // do that only if no hint provided 
    if ((scan.getOperatorType() == REL_SCAN)  && !cardHint && !selHint)
    {
      NABoolean flag = ((Scan &)scan).checkForCardRange(predicates, initialRowCount);
    }
}// applyPredicates(...)

// -----------------------------------------------------------------------
// The following method is based on method
// RelExpr::synthEstLogPropForUnaryLeafOp (...)
// which is on file OptLogRelExpr.cpp
//
// -----------------------------------------------------------------------
void
Histograms::applyPredicatesWhenMultipleProbes(
     const ValueIdSet& predicates
     ,const EstLogProp& inputEstLogProp
     ,const ValueIdSet& inputValues
     ,const NABoolean isMDAM
     ,const SelectivityHint * selHint
     ,const CardinalityHint * cardHint
     ,NABoolean *indexJoin
     ,OperatorTypeEnum opType
      )
{


  CostScalar innerRowCount = getRowCount();

  MergeType mergeMethod = INNER_JOIN_MERGE; // starting assumption

  // -----------------------------------------------------------------
  // First, determine if we seem to be in an index-driven nested join.
  // If so, remove base table column statistics that are
  // provided as outer references from the index table.
  // -----------------------------------------------------------------
  const ColStatDescList &outerColStatsList = inputEstLogProp.getColStats();
  ColStatDescList &innerColStatsList  =  colStatDescList_;

  CollIndex outerRefCount = outerColStatsList.entries();

  NAList<CollIndex> innerIndexColumnsToRemove(CmpCommon::statementHeap());
  NABoolean indexNestedJoin = FALSE;
  if ( NOT isMDAM
       AND
       (indexNestedJoin=
        isAnIndexJoin(inputEstLogProp, &innerIndexColumnsToRemove)) )
    {
      DCMPASSERT(innerIndexColumnsToRemove.entries() > 0);
      for (CollIndex i=0; i < innerIndexColumnsToRemove.entries(); i++)
        {
          innerColStatsList.removeAt(i);
        }
    }
  if (indexJoin != NULL)
    *indexJoin = indexNestedJoin;

  // -----------------------------------------------------------------
  // Second, pre-pend the outer reference column statistics onto the
  // [remaining] child's column statistics.
  // -----------------------------------------------------------------
  CostScalar innerScale;
  if (indexNestedJoin OR inputEstLogProp.getInputForSemiTSJ())
    {
      innerScale =  1.;
    }
  else
    {
      innerScale =  innerRowCount;
    }
  innerColStatsList.prependDeepCopy( outerColStatsList, // source
                                    outerRefCount,     // limit
                                    innerScale );      // scale

  if (entries() > 0)
    innerRowCount = ((*this)[0]).getColStats()->getRowcount();

  // -----------------------------------------------------------------
  // Third, scale the original entries in the child's column stats
  // The actual determination of how this is done is based on knowing
  // whether or not this is an index-driven nested join.....
  // I.e., if it's an index-based NJ, then both RowCount and UEC of
  // the 'base' table are scaled; if it's not index-based, then the
  // scaling depends upon whether or not this scan is the right child
  // of a semi-TSJ.  If not scaling is done as with join cross-products,
  // otherwise the right side isn't scaled.
  // -----------------------------------------------------------------
  CostScalar outerScale = 1;
  if (indexNestedJoin)
    {
      // ColStatDesc's from 0 up to outerRefCount came from outer,
      // so no need to synchronize them (why?)
      for (CollIndex i = outerRefCount;  i < entries(); i++)
        {
          ColStatDescSharedPtr columnStatDesc = innerColStatsList[i];
          ColStatsSharedPtr columnStats = columnStatDesc->getColStatsToModify();
          columnStats->copyAndScaleHistogram ( 1 );

          CostScalar oldCount =
            columnStatDesc->getColStats()->getRowcount();

          if (oldCount != innerRowCount)
            columnStatDesc->synchronizeStats (oldCount, innerRowCount);
        }
    }
  else // not an indexNestedJoin
    {
      if (outerRefCount > 0)
        {
          // Either the method for doing the join, or the scale factor of
          // the remaining columns in the StatDescList, must be updated.
          if ( inputEstLogProp.getInputForSemiTSJ() )
            mergeMethod = SEMI_JOIN_MERGE;
          else
            outerScale = outerColStatsList[0]->getColStats()->getRowcount();
        }

      for (CollIndex i = outerRefCount;  i < entries(); i++)
        {
          ColStatDescSharedPtr columnStatDesc = innerColStatsList[i];
          ColStatsSharedPtr columnStats = columnStatDesc->getColStatsToModify();
          columnStats->copyAndScaleHistogram( outerScale );
        }
    }


  ValueIdSet outerReferences;

  // Get the set of outer references.  If input value is a VEGReference,then
  // include this VEGReference only if the group consists of no constants.
  if (inputValues.entries() > 0)
    inputValues.getOuterReferences (outerReferences);

  ValueIdSet dummySet;
  // Apply the effect of my selection predicates on columnStats,
  // returning the resultant rowcount.
  innerRowCount =
    innerColStatsList.estimateCardinality(innerRowCount,       /*in*/
                                          predicates,/*in*/
                                          outerReferences,   /*in*/
                                          NULL,
                                          selHint,	    /*in*/
                                          cardHint,	    /*in*/
                                          outerRefCount,   /*in/out*/
                                          dummySet,
                                          /*in/out*/
                                          mergeMethod,      /*in*/
                                          opType,
                                          NULL);

} // Histograms::applyPredicatesWhenMultipleProbes(...)

void
Histograms::applyPredicate(const ValueId& predicate, 
                           const RelExpr & scan,
			   const SelectivityHint * selHint,
			   const CardinalityHint * cardHint,
			   OperatorTypeEnum opType
                           )
{
  ValueIdSet vis;
  vis.insert(predicate);
  applyPredicates(vis, scan, selHint, cardHint, opType);
} // applyPredicate(...)

NABoolean
Histograms::isAnIndexJoin(const EstLogProp& inputEstLogProp
                          ,LIST(CollIndex) *innerIndexColumnListPtr) const
{

  // --------------------------------------------------------------------
  // We are in an index join if
  // - We are receiving multiple probes
  // AND
  // -- one of the outer column statistics is the same as one of
  //    the inner statistics
  // --------------------------------------------------------------------

  NABoolean indexNestedJoin = FALSE;
  if ( (inputEstLogProp.getResultCardinality().isGreaterThanZero())
       AND NOT inputEstLogProp.getColStats().isEmpty() )

    {

      const ColStatDescList &outerColStatsList =
       inputEstLogProp.getColStats();

      const ColStatDescList& innerColStatsList = colStatDescList_;
      ColStatDescSharedPtr innerColumnStatDesc;
      for (CollIndex i = 0; i < outerColStatsList.entries(); i++)
        {
          ColStatDescSharedPtr outerStatDesc = outerColStatsList[i];

          for (CollIndex j=0; j < innerColStatsList.entries(); j++)
            {
              innerColumnStatDesc = innerColStatsList[j];

              if (outerStatDesc->getMergeState().contains(
                   innerColumnStatDesc->getMergeState()) )
                {
                  indexNestedJoin = TRUE;
                  if (innerIndexColumnListPtr != NULL)
                    {
                      innerIndexColumnListPtr->insert(i);
                    }
                  else
                    {
                      // All we want is to know whether we are in an index
                      // join, thus break early
                      break;
                    }
                }
            }  // for every inner column

          if ( (innerIndexColumnListPtr != NULL)
              AND
               indexNestedJoin )
            {
              // All we want is to know whether we are in an index join,
              // thus break early
              break;
            }
        } // for every outer column

    } // if we are receiving multiple probes

  return indexNestedJoin;

} // Histograms::isAnIndexJoin() const


void
Histograms::display() const
{
  print();
}// display()

void
Histograms::print (FILE *ofd,
		   const char * prefix,
		   const char * suffix) const
{
#ifndef NDEBUG
     ValueIdList emptySelectList;
     getColStatDescList().print(emptySelectList) ;
#endif
}// print()

//-------------------------------------------------------
// Methods for IndexDescHistograms
//-----------------------------------------------------



// -----------------------------------------------------------------------
// Input:
// const IndexDesc& indexDesc: the indexDesc for the base table
//    associated with this Scan. This index desc is stored through
// a reference, thus it must survive as long this instance is alive
// ("association" relationship)
//
// columnPosition: a one (1) based position of the column. The first
// position is one (1).
// If columnPosition is cero, then an empty IndexDescHistogram is
// created.
// -----------------------------------------------------------------------
IndexDescHistograms::IndexDescHistograms(const IndexDesc& indexDesc,
                                         const CollIndex columnPosition):
     indexDesc_(indexDesc)
{

  const ValueIdList &keyColumns = indexDesc.getIndexKey();
  // only add the ColStatDesc's for the index columns UP to
  // the column position: (for SQL/MP secondary indexes,
  // ignore the keytag column, which is the first keyColumn)
  CollIndex localColPosition = columnPosition;
  CollIndex columnIndex = 1;


  for (CollIndex i=columnIndex; i <= localColPosition; i++)
    {
      appendHistogramForColumnPosition(i);
    } // for every key column

} // IndexDescHistograms::IndexDescHistograms(const IndexDesc& indexDesc)

void
IndexDescHistograms::appendHistogramForColumnPosition(
     const CollIndex& columnPosition)
{

  // columnPosition == 0 means an empty colstatdesclist
  if (columnPosition > 0)
    {
      // Get the ColStatDescList for the base table:
      DCMPASSERT(getIndexDesc().getPrimaryTableDesc());

      const ColStatDescList &primaryTableCSDL =
        getIndexDesc().getPrimaryTableDesc()->getTableColStats();
      const ValueIdList &keyColumns = getIndexDesc().getIndexKey();

      DCMPASSERT(columnPosition > 0
                 AND
                 columnPosition <= keyColumns.entries());

      CollIndex j;
      // remember that columnPosition is one based:
      if (primaryTableCSDL.
          getColStatDescIndexForColumn(j, keyColumns[columnPosition-1]))
        {
          // There could be a single colStatDesc for two columns,
          // thus check whether it was already inserted:
	  ValueId column = primaryTableCSDL[j]->getColumn();
          if (NOT contains(column))
            {
              append(primaryTableCSDL[j]);
              setCSDLUecList (primaryTableCSDL.getUecList()) ;
              // synchronize with existing histograms:
              if (entries() > 1)
                {
                  ColStatDesc& lastHistogram =
                    *getColStatDescList()[entries()-1];
                  const ColStatDesc& previousToLastHistogram =
                    *getColStatDescList()[entries()-2];
                  lastHistogram.
                    synchronizeStats(lastHistogram.
                                     getColStats()->getRowcount(),
                                     previousToLastHistogram.
                                     getColStats()->getRowcount());
                }
            }
        }
      else
        {
          // There must be a ColStatDesc for every key column!
          CMPABORT;
        }

      // propagate all base-table multi-col uec info : easiest way
      // to do it, and there's no reason not to point to the complete
      // list
      setCSDLUecList (primaryTableCSDL.getUecList()) ;
    } // columnPosition > 0

} // IndexDescHistograms::appendHistogramForColumnPosition(...)

// returns true if we have multicolumnuec information available,
// otherwise false.
NABoolean IndexDescHistograms::isMultiColUecInfoAvail() const
{
    return getIndexDesc().getPrimaryTableDesc()->
        getTableColStats().getUecList()->containsMCinfo();
}


//-------------------------------------------------------------------
//This function is used to get a better estimate of how many uec's
//that need to be skipped by MDAM for a specific column.
//It can compute this number there is multi column uec information for
//the columns till the column under consideration. Then it needs
//uec/multi-col uec  information of some preceding columns.
//Example : Columns a, b, c, d . We are trying to compute the estimated
//uec for d
//Answer: can be multicolUec for ((a and/or b and/or c) and d)
//  ----------------------------
//      multicolUec for a and/or b and/or c(best we can do is a,b,c)
//
//if the denominator is a,b,c then the numerator must be a,b,c,d
// we must have comparable sets as numerator and denominator
//
//At one time this method actually computed this ratio, but in its
//current incarnation it returns just the UEC of the one MC histogram
//that matches the current key prefix. The caller is expected to
//compute this ratio after two consecutive calls on consecutive 
//columns. (This is the meaning of the strange comment below, 
//"The following code will never be exercised and it's intentional.")
//
//Input: columnOrderList which has the columnIdList for the index/table
//       indexOfcolum in the order list that we have to compute uec info for
//Output: estimateduec for the column and true as return value
//        if there isn't enough information then we return false.
//-----------------------------------------------------------------------------
NABoolean
IndexDescHistograms::estimateUecUsingMultiColUec(
           const ColumnOrderList& keyPredsByCol,/*in*/
           const CollIndex& indexOfColumn,/*in*/
           CostScalar& estimatedUec/*out*/)
{
   ValueIdList columnList= keyPredsByCol.getColumnList();
#ifndef NDEBUG
        if(getenv("MDAM_MCUEC")){
          fprintf(stdout,"\n\n---columnList before reduction \n\n----");
          columnList.print();
        }

#endif
//remove everything beyond the column under consideration
   for ( CollIndex i = keyPredsByCol.entries()-1; i>indexOfColumn; i--){
        columnList.removeAt(i);
   }

   ValueIdSet columnSet(columnList);
   NAList<CostScalar> uecCount(CmpCommon::statementHeap());

   const MultiColumnUecList * MCUL =
        getIndexDesc().getPrimaryTableDesc()->getTableColStats().getUecList();
#ifndef NDEBUG
        if(getenv("MDAM_MCUEC")){
          fprintf(stdout,"\n\n---columnList for Index[%d]:  \n", indexOfColumn);
          columnList.print();
          fprintf(stdout, "\n\n---List of MCUEC info.----\n\n");
          MCUL->print();
        }
#endif
//get all the valueIdSets that contains the column and columns from the
//columnList only
   LIST(ValueIdSet) * listOfSubsets=
     MCUL->getListOfSubsetsContainsColumns(columnList,uecCount);

   if(listOfSubsets->entries()==0) return FALSE;
#ifndef NDEBUG
        if(getenv("MDAM_MCUEC")){
          fprintf(stdout,"\n\n---Got my value ID list-----\n\n");
          for(CollIndex k=0;k<listOfSubsets->entries();k++){
            fprintf(stdout,"\n\n----Set no.[%d]:----\n\n",k);
            (*listOfSubsets)[k].print();
          }
        }
#endif
//Trying to find the matching denominator for the numerator
   CostScalar uecWithoutColumn=0;
   CollIndex entriesInSubset=0;
   NABoolean perfectDenom = FALSE;
   ValueIdSet vidSet;
   CollIndex indexForUecCount=0;
   ValueId idInBaseCol;
   //corresponding valueId in the base table for this column
   //this is necessary because multicolUec lists are always stored
   //in base table valueIds.
    idInBaseCol=((BaseColumn *)((IndexColumn *)
        (columnList[indexOfColumn].getItemExpr()))
        ->getDefinition().getItemExpr())->getValueId();
   // It is not clear to me how the above formual to compute MCUEC works.
   // But, it's very clear it didn't compute the correct MCUEC for the query.
   // In this case the keyCols were (A,B,C,D) and we wanted MCUEC(A,B). UEC(A) = 250K
   // and UEC(B) = 50Mil, but there existed a single Interval for MC(A,B) with 55Mil UEC. 
   // Instead of returning 55mil UEC, the above formula MC(A,B)/MC(A) returned 221 
   // (55727500/252136 -> 221). The new code below is similar to how we compute MCUEC in the 
   // rest of the compiler code.

   for (CollIndex j=0; j<listOfSubsets->entries();j++)
   {
     vidSet = (*listOfSubsets)[j];
#ifndef NDEBUG
     if(getenv("MDAM_MCUEC"))
     {
       fprintf(stdout, "Now checkout this set\n");
       vidSet.print();
     }
#endif
     if (vidSet.entries() == columnList.entries())
     {
       estimatedUec = MCUL->lookup(vidSet);
#ifndef NDEBUG
     if(getenv("MDAM_MCUEC"))
     {
       fprintf(stdout, "entry size matches\n");
       fprintf(stdout, "MC UEC=%f\n", estimatedUec.value());
     }
#endif
       return TRUE;
     }
   }
   return FALSE;
   // The following code will never be exercised and it's intentional.

   for (CollIndex j=0; j<listOfSubsets->entries();j++)
   {
     vidSet = (*listOfSubsets)[j];
     //remove the column in question then see
     //if we can find a match for the denominator
     //To find the denominator we remove the last column from the
     //vidSet(numerator), this gives us the prefix of the numerator
     //or in other words the denominator.
        vidSet.remove(idInBaseCol);
     //Do we have a matching denominator
        perfectDenom = MCUL->findDenom(vidSet);
#ifndef NDEBUG
  if(getenv("MDAM_MCUEC"))
  {
    fprintf(stdout, "largest subset returned\n\n");
    vidSet.print();
  }
#endif
  //choose the MC uecs with the most entries so for col d , best would
  // be abcd/abc even if you have abd/ab. But if there are two of same
  // entries select the one with higher uec count for the denominator.
        if(perfectDenom AND (vidSet.entries()>entriesInSubset OR
          (vidSet.entries()==entriesInSubset AND
           MCUL->lookup(vidSet) > uecWithoutColumn)))
        {
          perfectDenom = FALSE;
          entriesInSubset=vidSet.entries();
          uecWithoutColumn = MCUL->lookup(vidSet);
          indexForUecCount=j;
          //best possible situation for d
          //we have found a,b,c,d/a,b,c;
          if(entriesInSubset==indexOfColumn) break;
        }
   }
   if(entriesInSubset>0)
   {
      estimatedUec=uecCount[indexForUecCount]/uecWithoutColumn;
      return TRUE;
   }

   return FALSE;
}

// -----------------------------------------------------------------------
// Use this function on a ScanHistograms instance to find the
// number of probes that fail to match with the scan's rows.
//
// IMPORTANT: It is assummed that this ScanHistograms are the
//  result of the cross product of the outer table's ScanHistograms
//  with that of the inner table's ScanHistograms and that
//  appropiate key predicates have been applied.
//
// INPUT:
//   outerHistograms the ScanHistograms for the probes
//   keyPreds: the key predicates associated with the scan
// RETURN:
//    A CostScalar >= 0 that denotes the number of probes that failed
//    to match any data for the inner scan under a nested join.
// -----------------------------------------------------------------------

CostScalar
IndexDescHistograms::computeFailedProbes(const Histograms& outerHistograms
				    ,const ValueIdSet& keyPreds
				    ,const ValueIdSet& inputValues
				    ,const ValueIdSet& operatorValues
				    ) const

{

  CostScalar
      maxFailedProbes = csZero;

  // match histograms for corresponding columns
  // in joined histograms to find out the number
  // of probes that match from each, then find
  // the max:
  ColStatsSharedPtr outerColStats;
  ColStatsSharedPtr joinedColStats;
  CostScalar
    currentFailedProbes = 0;
  for (ValueId keyPred = keyPreds.init();
       keyPreds.next(keyPred);
       keyPreds.advance(keyPred))
    {

       if ( keyPred.getItemExpr()->
            isANestedJoinPredicate(inputValues,
                                   operatorValues) )
         {
           // Find out if current predicate is a joined predicate,
           // which it is when the predicate refers to a column in
           // the outer statistics.
           outerColStats =
             outerHistograms.getColStatsPtrForPredicate(keyPred);
           if (outerColStats !=NULL)
             {
               // It is a joined predicate!
               // find the joined hist. for the predicate:
               joinedColStats =
                 this->getColStatsPtrForPredicate(keyPred);
               DCMPASSERT(joinedColStats != NULL);
               // find the failed probes for these histograms:
               currentFailedProbes =
                 outerColStats->countFailedProbes(joinedColStats);
             }
           else
             {
               // should never be here:
#ifndef _DEBUG
               FSOWARNING("outerColStats is NULL");
#endif
               currentFailedProbes = csZero;
             }
           if (currentFailedProbes > maxFailedProbes)
             {
               maxFailedProbes = currentFailedProbes;
             }
         }  // only if it is a join pred

    }// for all predicates

  // maxFailedProbes contain the failed probes for that
  // predicate with the most failed probes, and this is
  // what we need:

  //if (maxFailedProbes < 0)
  //  {
  //    FSOWARNING("maxFailedPorbes < 0");
  //    maxFailedProbes =0.;
  //  }
  maxFailedProbes.minCsZero();

  return maxFailedProbes;

} // IndexDescHistograms::computeFailedProbes(...)



//-------------------------------------------------------
// Methods for ScanOptimizer:
//-----------------------------------------------------

ScanOptimizer::ScanOptimizer(const FileScan& associatedFileScan
                             ,const CostScalar& resultSetCardinality
                             ,const Context& myContext
                             ,const ValueIdSet& externalInputs) :
  probes_(0)
  ,uniqueProbes_(0)
  ,successfulProbes_(0)
  ,duplicateSuccProbes_(0)
  ,fileScan_(associatedFileScan)
  ,resultSetCardinality_(resultSetCardinality)
  ,context_(myContext)
  ,externalInputs_(externalInputs)
  ,numberOfBlocksToReadPerAccess_(-1)
  ,estRowsAccessed_(0)
  ,inOrderProbes_(FALSE)
  ,probesForceSynchronousAccess_(FALSE)
  ,tuplesProcessed_(0)
{
}

ScanOptimizer::~ScanOptimizer()
{
};

// Determine if the SimpleFileScanOptimizer can be used for the given
// scan with the given context.
// Return : TRUE - Use the SimpleFileScanOptimizer
//          FALSE - Don't use the SimpleFileScanOptimizer
//
NABoolean
ScanOptimizer::useSimpleFileScanOptimizer(const FileScan& associatedFileScan
                                          ,const Context& myContext
                                          ,const ValueIdSet &externalInputs)
{
#ifndef NDEBUG
  if (CmpCommon::getDefault(MDAM_TRACING) == DF_ON )
    MdamTrace::setLevel(MTL2);
    //MdamTrace::setLevel(MDAM_TRACE_LEVEL_ALL);
#endif

  //
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  ULng32 fsoToUse = defs.getAsULong(FSO_TO_USE);

  // fsoToUse -
  // IF 0 - Use original "Complex" File Scan Optimizer. (Default)
  // IF 1 - Use logic below to determine FSO to use.
  // IF 2 - Use logic below to determine FSO to use, but also use new
  //        executor predicate costing.
  // IF >2 - Always use new "Simple" File Scan Optimizer.

  // Is the complex FSO forced;
  //
  if(fsoToUse == 0)
  {
    return FALSE;
  }

  // Is the simple FSO forced;
  //
  if(fsoToUse > 2)
  {
    return TRUE;
  }

  // Milestone 3
  //
  //- Simple single-subset scans
  //- with or without predicates
  //- With or without partKeyPreds for logical SubPartitioning.
  //- MDAM will not be considered
  //

  //- with or without predicates
  //
  ValueIdSet vs;
  ValueIdSet exePreds(associatedFileScan.getSelectionPred());

  if((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
     exePreds.entries())
  {	  
    ItemExpr *inputItemExprTree = exePreds.rebuildExprTree(ITM_AND,FALSE,FALSE);
    ItemExpr* resultOld = revertBackToOldTree(CmpCommon::statementHeap(), 
					      inputItemExprTree);
    exePreds.clear();
    resultOld->convertToValueIdSet(exePreds, NULL, ITM_AND, FALSE);
    doNotReplaceAnItemExpressionForLikePredicates(resultOld,exePreds,resultOld);
  }

  const Disjuncts *curDisjuncts = &(associatedFileScan.getDisjuncts());

  //-  With or without partKeyPreds for logical SubPartitioning.
  //
  const LogPhysPartitioningFunction *logPhysPartFunc =
    myContext.getPlan()->getPhysicalProperty()->getPartitioningFunction()->
    castToLogPhysPartitioningFunction();

  ValueIdSet partKeyPreds;

  if ( logPhysPartFunc != NULL )
  {
    LogPhysPartitioningFunction::logPartType
      logPartType = logPhysPartFunc->getLogPartType();

    if (logPartType == LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING ||
        logPartType == LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING)
    {

      partKeyPreds = logPhysPartFunc->getPartitioningKeyPredicates();

      exePreds += partKeyPreds;

      curDisjuncts = new HEAP MaterialDisjuncts(exePreds);
    }
  }

  // check if MDAM can be considered

  switch(getMdamStatus(associatedFileScan, myContext))
  {
    case ScanForceWildCard::MDAM_OFF:
      // If MDAM is forced OFF, don't bother checking executor preds for
      // possibility of MDAM, just use the simple file scan optimizer.
      //
      return TRUE;

    case ScanForceWildCard::MDAM_FORCED:
      // If MDAM is forced ON, must consider MDAM so don't use simple
      // file scan optimizer.
      //
      return FALSE;

    default:
      // If MDAM is enabled (not forced on or off), continue on to
      // examine the selection preds to see if MDAM should be
      // considered.
      break;
  }

  ValueIdSet nonKeyColumnSet;
  const IndexDesc *indexDesc = associatedFileScan.getIndexDesc();
  indexDesc->getNonKeyColumnSet(nonKeyColumnSet);

  const ValueIdList indexKey = indexDesc->getIndexKey();
  IndexDescHistograms histograms(*indexDesc,indexKey.entries());

  // HBASE work. Remove after stats for Hbase is done!!!
  NABoolean isHbaseTable = indexDesc->getPrimaryTableDesc()->getNATable()->isHbaseTable();
  if (! isHbaseTable && histograms.containsAtLeastOneFake())
  {
    MDAM_DEBUG0(MTL2, "Fake histogram found, MDAM NOT considered.");
    MDAM_DEBUGX(MTL2, histograms.display());
    return TRUE;
  }
  else
  {
    const Disjuncts &disjuncts = associatedFileScan.getDisjuncts();
    if (disjuncts.entries() > MDAM_MAX_NUM_DISJUNCTS)
    {
      // There are too many disjuncts. MDAM cannot be considered
      return TRUE;
    }
    else
    {
      // Since there is an index join, we cannot consider MDAM
      if(histograms.isAnIndexJoin(*(myContext.getInputLogProp())) )
	return TRUE;
    }
  }


  //- Simple single-subset scans
  //

  if (exePreds.entries() > 0)
  {
    SearchKey searchKey(indexDesc->getIndexKey()
                       ,indexDesc->getOrderOfKeyValues()
                       ,externalInputs
                       ,(NOT associatedFileScan.getReverseScan())
                       ,exePreds
                       ,*curDisjuncts
                       ,nonKeyColumnSet
                       ,indexDesc
                       );

    if ((! searchKey.isUnique()) &&
	(searchKey.getKeyPredicates().entries() > 0))
    {
      ColumnOrderList keyPredsByCol(indexDesc->getIndexKey());
      searchKey.getKeyPredicatesByColumn(keyPredsByCol);

      CollIndex singleSubsetPrefixColumn;
      NABoolean itIsSingleSubset =
       keyPredsByCol.getSingleSubsetOrder(singleSubsetPrefixColumn);

      ValueIdSet singleSubsetPreds;

      for (CollIndex pred = 0; pred <= singleSubsetPrefixColumn; pred++)
      {
	const ValueIdSet *preds = keyPredsByCol[pred];
	if(preds)
	{
	  singleSubsetPreds += *preds;
	}
      }

      // Not a simple single subset. Could not use any key preds for a
      // single subset, must check if MDAM might be better
      //

      if (singleSubsetPreds.isEmpty())
      {
	return FALSE;
      }

      // If not all the keys are covered by the key preds and there
      // are multiple disjuncts, must consider MDAM.
      //

      if (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
      {
	if ((singleSubsetPrefixColumn+1 < keyPredsByCol.entries()) &&
	    (curDisjuncts->entries() >= 1) && curDisjuncts->containsAndorOrPredsInRanges())
	{
	  return FALSE;
	}
      }
      else
      {
	if ((singleSubsetPrefixColumn+1 < keyPredsByCol.entries()) &&
	    (curDisjuncts->entries() > 1))
	{
	  return FALSE;
	}
      }
    }

    // If there are executor preds that could not be used as key predicates
    // and there are multiple disjuncts..

    if (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
    {
      if ((searchKey.getKeyPredicates().entries() == 0) &&
	  (curDisjuncts->entries() >= 1) && curDisjuncts->containsAndorOrPredsInRanges())
      {
	return FALSE;
      }
    }
    else
    {
      if ((searchKey.getKeyPredicates().entries() == 0) &&
	  (curDisjuncts->entries() > 1))
      {
	return FALSE;
      }
    }
  }
  // Use SimpleFileScanOptimizer
  //
  return TRUE;
}

NABoolean ScanOptimizer::canStillConsiderMDAM(const ValueIdSet partKeyPreds,
					 const ValueIdSet nonKeyColumnSet,
					 const Disjuncts &curDisjuncts,
					 const IndexDesc * indexDesc,
					 const ValueIdSet externalInputs,
					 NABoolean mdamFlag)
{

  NABoolean canDoMdam = TRUE;
  const ValueIdList indexKey = indexDesc->getIndexKey();

  // -----------------------------------------------------------------------
  // Check whether MDAM can be considered for this node:
  // -----------------------------------------------------------------------

  if(CURRSTMT_OPTDEFAULTS->indexEliminationLevel() != OptDefaults::MINIMUM
     AND mdamFlag == MDAM_OFF)
       	canDoMdam = FALSE;

  else
  if (NOT indexDesc->getNAFileSet()->isKeySequenced())
  {
    // -----------------------------------------------------------
    //  Don't consider MDAM for relative and entry-sequence files:
    // -----------------------------------------------------------

    canDoMdam = FALSE;
    MDAM_DEBUG0(MTL2, "File is not key sequenced, MDAM NOT considered.");

  }
  else
  {
    // -----------------------------------------------------------
    // Don't consider MDAM when one of the disjunct does
    // not contain key predicates:
    // -----------------------------------------------------------

    // If at least one of the disjunts in the mdam key does not contain
    // key predicates then using MDAM results in a full table
    // scan, thus don't bother to consider it in this case:
    CollIndex i = 0;
    MdamKey mdamKey(indexKey
		,externalInputs
		,curDisjuncts
		,nonKeyColumnSet
		,indexDesc
		);
    for (i=0; i < mdamKey.getKeyDisjunctEntries(); i++)
    {
      ColumnOrderList keyPredsByCol(indexKey);
      mdamKey.getKeyPredicatesByColumn(keyPredsByCol,i);
      ValueIdSet preds;
      keyPredsByCol.getAllPredicates(preds);
      if (preds.isEmpty())
	{
	// This disjunct does not have key preds,
	// don't consider MDAM
	canDoMdam = FALSE;
	MDAM_DEBUG1(MTL2,
            "disjunct[%d] has no key predicate, MDAM NOT considered", i);
	break;
	}
    }
  }
  return canDoMdam;
}

ScanOptimizer *
ScanOptimizer::getScanOptimizer(const FileScan& associatedFileScan
                                ,const CostScalar& resultSetCardinality
                                ,const Context& myContext
                                ,const ValueIdSet &externalInputs
                                ,CollHeap* heap)
{

#ifndef NDEBUG
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  if(defs.getAsULong(FSO_RUN_TESTS) > 0) {

    // Run all Scan Optimizer Tests before constructing one
    //
    ScanOptimizerAllTests(associatedFileScan
                          ,resultSetCardinality
                          ,myContext
                          ,externalInputs
                          ,heap);
  }
#endif

  if (useSimpleFileScanOptimizer(associatedFileScan,
				 myContext,
				 externalInputs)) 
  {

    return new(heap) SimpleFileScanOptimizer(associatedFileScan,
                                             resultSetCardinality,
                                             myContext,
                                             externalInputs);
  } 
  else 
  {

    return new(heap) FileScanOptimizer(associatedFileScan,
                                       resultSetCardinality,
                                       myContext,
                                       externalInputs);
  }
}

// Checks for overflow before setting the numberOfBlocksToReadPerAccess_.
// If overflow, set to INT_MAX
//
void
ScanOptimizer::setNumberOfBlocksToReadPerAccess(const CostScalar& blocks)
{
  //overflow occuring while casting it to long
  //the below is a check to avoid the overflow
  //CR 10-010815-4585
  const double val = blocks.getValue();

  if(val < double(INT_MAX)) {
    Lng32 lval = Lng32(val);
    DCMPASSERT(lval > -1);
    numberOfBlocksToReadPerAccess_ = lval;
  }
  else
    numberOfBlocksToReadPerAccess_ = INT_MAX;
}

CollIndex ScanOptimizer::getNumActivePartitions() const
{
  // This is a patch to fix plan cgange in TPCC Q7 during Neo_Compiler 
  // integration. Since we try to use estimated number of AP in costing
  // we need to be consistent and use it everywhere in costing. Hopefully
  // it won't break anything to define correcttly the number of PAs.
  // All active partition usage need a separate project to clean it up.

  if ( CmpCommon::getDefault(COMP_BOOL_32) == DF_OFF )
  {
     return getEstNumActivePartitionsAtRuntime();
  }

  PartitioningFunction *pf =
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction();

  DCMPASSERT(pf != NULL);

  // All this casting away is because mutable is not supported, thuse NodeMap has
  // non-mutable cached members....
  CollIndex actParts = ((NodeMap *)(pf->getNodeMap()))->getNumActivePartitions();

  return actParts;

}

CollIndex ScanOptimizer::getEstNumActivePartitionsAtRuntime() const
{
  PartitioningFunction *pf =
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction();

  DCMPASSERT(pf != NULL);

  CollIndex actParts = ((NodeMap *)(pf->getNodeMap()))->getNumActivePartitions();

  //Check if the partitioning function is of type LOGPHYS_PARTITIONING_FUNCTION.
  //If we have this node then we are sure that this partitioning function will
  //have both logical and physical partitioning function.
  if(pf->isALogPhysPartitioningFunction())
  {

    PartitioningFunction *phyPartFunc = NULL;
    phyPartFunc = ((LogPhysPartitioningFunction *)pf)->getPhysPartitioningFunction();

    if(phyPartFunc != NULL )
    {
      //We use the estimated active partitions only for range and hash
      //partitioning functions.
      if ((phyPartFunc->isARangePartitioningFunction())
	OR
	  (phyPartFunc->isATableHashPartitioningFunction()))
	{
         actParts = ((NodeMap *)(pf->getNodeMap()))->getEstNumActivePartitionsAtRuntime();
	}
    }
  }

  if ( actParts > 1 ) 
     actParts = MINOF(actParts, getFileScan().getComputedNumOfActivePartiions());

  return actParts;
}

// look at physical partition function of indexDesc to determine active partitions(AP)
// for salted table. Adjust AP count if partitionkey predicate is a constant
CollIndex ScanOptimizer::getEstNumActivePartitionsAtRuntimeForHbaseRegions() const
{
  CollIndex actParts;
  PartitioningFunction *pf =
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction();

  DCMPASSERT(pf != NULL);

    // get estimated active partition count
  CollIndex estActParts = ((NodeMap *)(pf->getNodeMap()))->getEstNumActivePartitionsAtRuntime();

  // if partition key predicate is constant, then estimated active partition count will
  // be set to one by computeDP2CostDataThatDependsOnSPP() method. 
  // Use this value for costing REL_HBASE_ACCESS operator
  if (estActParts == 1 AND (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON))
    return estActParts;
  
  // return the #region servers via the partition function describing the physical Hbase table
  PartitioningFunction * physicalPartFunc = getIndexDesc()->getPartitioningFunction();
  if (physicalPartFunc == NULL) // single region
   actParts =  1;
  else // multi-region case
    actParts =  ((NodeMap *)(physicalPartFunc->getNodeMap()))->getNumActivePartitions();

  // if partition key predicate is constant, then estimated active partition count will 
  // be one, use that value
  if (estActParts == 1 AND (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON))
    actParts = estActParts;

  if ( actParts > 1 ) 
     actParts = MINOF(actParts, getFileScan().getComputedNumOfActivePartiions());

  return actParts;
}

CollIndex ScanOptimizer::getNumActiveDP2Volumes() const
{
  PartitioningFunction *pf =
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction();;

  DCMPASSERT(pf != NULL);

  // All this casting away is because mutable is not supported, thuse NodeMap has
  // non-mutable cached members....
  CollIndex actVols = ((NodeMap *)(pf->getNodeMap()))->getNumActiveDP2Volumes();

  // Assume at least one active volume even if node map indicates otherwise.
  return MIN_ONE(actVols);

}

// Static method, used by useSimpleFileScanOptimizer()
// Determine whether MDAM is Forced ON, Forced OFF, or ENABLED.
//
// MDAM can be forced ON by:
//    CONTROL QUERY SHAPE
//    CONTROL TABLE
//    INLINING
//
// MDAM can be forced OFF by:
//    CONTROL QUERY DEFAULT MDAM_SCAN_METHOD 'OFF'
//    CONTROL QUERY SHAPE
//    CONTROL TABLE
//    MDAM Flag (from index elimination project)
//    Too many disjuncts
//
ScanForceWildCard::scanOptionEnum
ScanOptimizer::getMdamStatus(const FileScan& fileScan
                             ,const Context& myContext)
{

  // Soln:10-050620-8905
  // Executor does not support MDAM for streams due to some limitations in executor and DP2,
  // hence MDAM is disabled for streams.
  
	if( fileScan.getGroupAttr()->isStream() )
		return ScanForceWildCard::MDAM_OFF;

  // Soln:10-050620-8905


  //----------------------------------------------------
  // First we check that MDAM is not disabled globaly by
  // set define MDAM OFF
  //----------------------------------------------------

#ifndef NDEBUG
  // quick force for MDAM for QA:
  char *cstrMDAMStatus = getenv("MDAM");
  if ((cstrMDAMStatus != NULL) &&
      ( strcmp(cstrMDAMStatus,"OFF")==0 ))
    {
      return ScanForceWildCard::MDAM_OFF;
    }
  else if ((cstrMDAMStatus != NULL) &&
           ( strcmp(cstrMDAMStatus,"ON")==0 ))
    {
      return ScanForceWildCard::MDAM_FORCED;
    }
#endif

  // First check of MDAM is forced on by a CONTROL QUERY SHAPE,
  // CONTROL TABLE

  // Now, Check if MDAM is Forced (which is done via Control Query Shape)
  // The forcing information is passed through the context.
  //
  const ReqdPhysicalProperty* propertyPtr = myContext.getReqdPhysicalProperty();
  if ( propertyPtr
       && propertyPtr->getMustMatch()
       && (propertyPtr->getMustMatch()->getOperatorType()
	   == REL_FORCE_ANY_SCAN))
    {
      ScanForceWildCard* scanForcePtr =
        (ScanForceWildCard*)propertyPtr->getMustMatch();
      if (scanForcePtr->getMdamStatus() == ScanForceWildCard::MDAM_FORCED)
        return ScanForceWildCard::MDAM_FORCED;
    }

  // Is Mdam Forced ON by a CONTROL TABLE statement for this table.
  //
  const NAString * val =
    ActiveControlDB()->getControlTableValue(
      fileScan.getIndexDesc()->getNAFileSet()->getExtFileSetName(), "MDAM");
  if ((val) && (*val == "ON")) {
    return ScanForceWildCard::MDAM_FORCED;
  }

  // During inlining the binder may force mdam on a table.
  //
  if (fileScan.getInliningInfo().isMdamForcedByInlining())
  {
    return ScanForceWildCard::MDAM_FORCED;
  }

  // Check if the CQD forces MDAM OFF.
  //
  if (CmpCommon::getDefault(MDAM_SCAN_METHOD) == DF_OFF)
    return ScanForceWildCard::MDAM_OFF;

  // Now, Check if MDAM is Forced OFF (which is done via Control Query
  // Shape) The forcing information is passed through the context.
  //
  if ( propertyPtr
       && propertyPtr->getMustMatch()
       && (propertyPtr->getMustMatch()->getOperatorType()
	   == REL_FORCE_ANY_SCAN))
    {
      ScanForceWildCard* scanForcePtr =
        (ScanForceWildCard*)propertyPtr->getMustMatch();
      if (scanForcePtr->getMdamStatus() == ScanForceWildCard::MDAM_OFF)
        return ScanForceWildCard::MDAM_OFF;
    }

  // Is Mdam Forced OFF by a CONTROL TABLE statement for this table.
  //
  val =
    ActiveControlDB()->getControlTableValue(
      fileScan.getIndexDesc()->getNAFileSet()->getExtFileSetName(), "MDAM");
  if ((val) && (*val == "OFF")) {
    return ScanForceWildCard::MDAM_OFF;
  }

  // The Index elimination project added the MdamFlag which can force
  // MDAM off
  //
  if(CURRSTMT_OPTDEFAULTS->indexEliminationLevel() != OptDefaults::MINIMUM
     AND fileScan.getMdamFlag() == MDAM_OFF 
	 && (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) != DF_ON )
	 )
  { 
    return ScanForceWildCard::MDAM_OFF;
  }

  // If the number of disjuncts exceeds the maximum, MDAM is forced OFF
  //
  if(fileScan.getDisjuncts().entries() > MDAM_MAX_NUM_DISJUNCTS) {
    return ScanForceWildCard::MDAM_OFF;
  }

  // If MDAM is not forced ON or OFF, then it will be determined by
  // the system.
  //
  return ScanForceWildCard::MDAM_SYSTEM;
}

// Static version of isMdamForced(), used by useSimpleFileScanOptimizer()
//
NABoolean
ScanOptimizer::isMdamForced(const FileScan& fileScan
                            ,const Context& myContext)
{

  // Soln:10-050620-8905
  // Executor does not support MDAM for streams due to some limitations in executor and DP2,
  // hence MDAM is disabled for streams.
  
	if( fileScan.getGroupAttr()->isStream() )
		return FALSE;

  // Soln:10-050620-8905

  //----------------------------------------------------
  // First we check that MDAM is not disabled globaly by
  // set define MDAM OFF
  //----------------------------------------------------

#ifndef NDEBUG
  // quick force for MDAM for QA:
  char *cstrMDAMStatus = getenv("MDAM");
  if ((cstrMDAMStatus != NULL) &&
      ( strcmp(cstrMDAMStatus,"OFF")==0 ))
    {
      return FALSE;
    }
  else if ((cstrMDAMStatus != NULL) &&
           ( strcmp(cstrMDAMStatus,"ON")==0 ))
    {
      return TRUE;
    }
#endif

  //---------------------------------------------------------------
  // Now, Check if MDAM is Forced (which is done via Control Query Shape)
  // The forcing information is passed through the context.
  //---------------------------------------------------------------

  const ReqdPhysicalProperty* propertyPtr = myContext.getReqdPhysicalProperty();
  if ( propertyPtr
       && propertyPtr->getMustMatch()
       && (propertyPtr->getMustMatch()->getOperatorType()
	   == REL_FORCE_ANY_SCAN))
    {
      ScanForceWildCard* scanForcePtr =
        (ScanForceWildCard*)propertyPtr->getMustMatch();
      if (scanForcePtr->getMdamStatus() == ScanForceWildCard::MDAM_FORCED)
        return TRUE;
      //else return FALSE;
    }
  //else
    //return FALSE;
  const NAString * val =
	   //ct-bug-10-030102-3803 -Begin
    ActiveControlDB()->getControlTableValue(
	 fileScan.getIndexDesc()->getPrimaryTableDesc()
         ->getCorrNameObj().getUgivenName(), "MDAM");
	   //ct-bug-10-030102-3803 -End
  if ((val) && (*val == "ON"))
    return TRUE;

  // During inlining the binder may force mdam on a table.
  if (fileScan.getInliningInfo().isMdamForcedByInlining())
  {
    return TRUE;
  }

  return FALSE;
}


NABoolean
ScanOptimizer::isMdamEnabled() const
{
  // give support to MDAM defines:
  // MDAM is enabled by default
  // MDAM ON: enable MDAM
  // MDAM OFF: disable MDAM

  // -----------------------------------------------------------------------
  // Mdam is enabled by default. Its only disabled globaly by
  // setting the default MDAM_SCAN_METHOD to "OFF"
  // You can re-enable it by
  // setting the MDAM_SCAN_METHOD to "ON"
  // Note that this code assumes MDAM is ON by default and
  // thus will enable MDAM if anything other than "OFF" is set
  // in the default MDAM_SCAN_METHOD
  // $$$ A warning should be printed if MDAM_SCAN_METHOD is
  // $$$ set to something other than "OFF" or "ON"
  // -----------------------------------------------------------------------
  NABoolean mdamIsEnabled = TRUE;

   // Soln:10-050620-8905
  // Executor does not support MDAM for streams due to some limitations in executor and DP2,
  // hence MDAM is disabled for streams.
  
   if( getRelExpr().getGroupAttr()->isStream() )
	   mdamIsEnabled = FALSE;

  // Soln:10-050620-8905

  // -----------------------------------------------------------------------
  // Check the status of the enabled/disabled flag in
  // the defaults:
  // -----------------------------------------------------------------------
  if (CmpCommon::getDefault(MDAM_SCAN_METHOD) == DF_OFF)
    mdamIsEnabled = FALSE;

  // -----------------------------------------------------------------------
  // Mdam can also be disabled for a particular scan via Control
  // Query Shape. The information is passed by the context.
  // -----------------------------------------------------------------------
  if (mdamIsEnabled)
    {

      const ReqdPhysicalProperty* propertyPtr =
        getContext().getReqdPhysicalProperty();
      if ( propertyPtr
           && propertyPtr->getMustMatch()
           && (propertyPtr->getMustMatch()->getOperatorType()
               == REL_FORCE_ANY_SCAN))
        {
          ScanForceWildCard* scanForcePtr =
            (ScanForceWildCard*)propertyPtr->getMustMatch();
          if (scanForcePtr->getMdamStatus() == ScanForceWildCard::MDAM_OFF)
            mdamIsEnabled = FALSE;
        }
    }

  // -----------------------------------------------------------------------
  // Mdam can also be disabled for a particular table via a Control
  // Table command.
  // -----------------------------------------------------------------------
  if (mdamIsEnabled)
    {
      const NAString * val =
	   //ct-bug-10-030102-3803 -Begin
	ActiveControlDB()->getControlTableValue(
	  getIndexDesc()->getPrimaryTableDesc()
          ->getCorrNameObj().getUgivenName(), "MDAM");
	   //ct-bug-10-030102-3803 -End
      if ((val) && (*val == "OFF")) // CT in effect
	{
	  mdamIsEnabled = FALSE;
	}
    }

  const IndexDesc* idesc = getIndexDesc();

  NABoolean isHbaseNativeTable = 
    idesc->getPrimaryTableDesc()->getNATable()->isHbaseCellTable() ||
    idesc->getPrimaryTableDesc()->getNATable()->isHbaseRowTable();
  if (isHbaseNativeTable)
    mdamIsEnabled = FALSE;

  // If the table to be optimized is the base table and has divisioning
  // columns defined on it, no more check is necessary.
  if ( idesc == idesc->getPrimaryTableDesc()->getClusteringIndex() ) {

    const ValueIdSet divisioningColumns =
           idesc->getPrimaryTableDesc()->getDivisioningColumns();

    if ( divisioningColumns.entries() > 0 &&
         (CmpCommon::getDefault(MTD_GENERATE_CC_PREDS) == DF_ON) )
       return mdamIsEnabled;
  }


  if (mdamIsEnabled)
    {
    CostScalar repeatCount = getContext().getPlan()->getPhysicalProperty()->
    getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2() ;

    // If # of probes for NJ is <= cqd(MDAM_UNDER_NJ_PROBES_THRESHOLD),
    // we will use MDAM regardless of the setting of CQD MDAM_SCAN_METHOD.
    // When the CQD is 0, MDAM under NJ is not considered.

    Lng32 mdam_under_nj_probes =
         (ActiveSchemaDB()->getDefaults()).getAsLong(
           MDAM_UNDER_NJ_PROBES_THRESHOLD);

    if ( mdam_under_nj_probes > 0 )
    {
       if ( repeatCount <= mdam_under_nj_probes &&
         getContext().getInputLogProp()->getColStats().entries() > 0
       )
      return TRUE;
    }

    // If the input logical property indicates exactly one, then we
    // allow MDAM in spite of NJ.
    NABoolean isInputCardinalityOne = 
      getContext().getInputLogProp()->isCardinalityEqOne();
    if (isInputCardinalityOne)
      return TRUE;

    /*
     Right side Scan of a Nested Join will use MDAM disjuncts if and only if
     MDAM_SCAN_METHOD CQD's value is MAXIMUM.
    */
    if ((repeatCount.isGreaterThanOne()) OR
      (getContext().getInputLogProp()->getColStats().entries() > 0)
          && (CmpCommon::getDefault(MDAM_SCAN_METHOD) != DF_MAXIMUM))
       mdamIsEnabled = FALSE;
   }

  return mdamIsEnabled;
} //  isMdamEnabled()

const CostScalar
ScanOptimizer::getIndexLevelsSeeks() const
{
  //CostScalar indexLevelsSeeks =
  //  CostScalar(getIndexDesc()->getIndexLevels()) - csOne;
  //indexLevelsSeeks =
  //    (indexLevelsSeeks < 0 ? csZero : indexLevelsSeeks);
  //return indexLevelsSeeks;
    if ( getIndexDesc()->getIndexLevels() > 1 )
      return CostScalar( getIndexDesc()->getIndexLevels() - 1);
    else
      return csZero;
}

// -----------------------------------------------------------------------
// INPUT:
// firstRow = the first row cost  per scan
// lastRow = the last row cost per scan
// OUTPUT:
//   The Cost object. This Cost object takes into account the
//    fact that the scan may be reading the data synchronously.
// -----------------------------------------------------------------------

Cost*
ScanOptimizer::computeCostObject(
     const SimpleCostVector& firstRow /* IN */
     ,const SimpleCostVector& lastRow /* IN */
     ) const
{

  // -----------------------------------------------------------------------
  // This routine computes the Cost object for the physical scan out of the
  // simple cost vectors for the first and last row.
  //
  // The cost vector needs two pieces of information to
  // compute the total cost:
  // countOfCpus
  // planFragmentsPerCPU.
  //--------------------------------------------------------------------------


  // With the new AP code scan costing is always done after synthesis:
  DCMPASSERT(getContext().getPlan()->getPhysicalProperty());

  const CostScalar activePartitions = getNumActivePartitions();

  Lng32 countOfCPUsExecutingDP2s = MINOF((Lng32)activePartitions.getValue(),
    getContext().getPlan()->getPhysicalProperty()->getCurrentCountOfCPUs());
  


  // -----------------------------------------------------------------------
  // Compute the degree of I/O parallelism:
  // -----------------------------------------------------------------------

  // get the logPhysPartitioningFunction, which we will use
  // to get the number of PAs:
  const LogPhysPartitioningFunction* lppf =
    getContext().getPlan()->getPhysicalProperty()->
   getPartitioningFunction()->castToLogPhysPartitioningFunction();

  Lng32 countOfPAs = 1;
  SimpleCostVector
    tempFirst = firstRow
    ,tempLast = lastRow
    ;

  // get partitions per CPU
  CostScalar  partitionsPerCPU = csOne;


  // Compute the number of PAs and see if synchronous access is
  // occuring. If there is synchronous access, then we need to
  // adjust the cost, since the cost was computed assuming that
  // asynchronous access would occur.
  if (lppf == NULL)  // Unpartitioned, Trafodion, native HBase or hive tables?
    {
      PartitioningFunction* partFunc = getContext().getPlan()
                ->getPhysicalProperty()-> getPartitioningFunction();

      if ( partFunc->castToSinglePartitionPartitioningFunction() )
         countOfPAs = 1;
      else {
         const FileScan& fileScan = getFileScan();

         if ( fileScan.isHbaseTable() || fileScan.isHiveTable() ||
              fileScan.isSeabaseTable())
            countOfPAs = partFunc->getCountOfPartitions();
         else
            DCMPASSERT(FALSE);
      }
    }
  else
    {
      // This is a partitioned table. Figure out
      // how much parallelism is involved and penalize
      // if not fully parallel:

      // Get the number of PAs.
      countOfPAs = lppf->getNumOfClients();

      // If we are under a nested join, and the probes are in a complete
      // ESP process order, and the clustering key and partitioning
      // key are the same, then the probes will force each ESP to
      // access all it's partitions synchronously. So, the level of
      // asynchronous access is limited to the number of ESPs. So, limit
      // the number of PAs by the number of ESPs, i.e. the number of
      // partitions in the logical partitioning function.
      if (getProbesForceSynchronousAccessFlag())
      {
        PartitioningFunction* logPartFunc =
          lppf->getLogPartitioningFunction();
        Lng32 numParts = logPartFunc->getCountOfPartitions();
        countOfPAs = MINOF(numParts,countOfPAs);
      }

      // Limit the number of PAs by the number of active partitions.
      // This won't need to be done here when we start doing it when
      // we synthesize the physical properties. It is not being done
      // there now because the active partitions estimate is inaccurate
      // sometimes and so we cannot rely on it now for setting
      // something that will be relied upon by the executor.
      countOfPAs =
        MINOF((Lng32)activePartitions.getValue(),countOfPAs);

      Lng32 numOfDP2Volumes =
        MIN_ONE( ((NodeMap *)(lppf->getNodeMap()))->getNumActiveDP2Volumes() );
      // Limit the number of PAs by the number of DP2 volumes.
      // This won't need to be done here when we start doing it when
      // we synthesize the physical properties. It is not being done
      // there now because the number of DP2 volumes estimate is inaccurate
      // sometimes and so we cannot rely on it now for setting
      // something that will be relied upon by the executor.
      // Assume at least one DP2 volume even if node map indicates otherwise.
      countOfPAs =
        MINOF(numOfDP2Volumes,countOfPAs);

      // Now compute the number of partitions that will be processed
      // by each stream (PA). If this number is greater than 1, then
      // it means that some streams (PAs) are accessing more than
      // one partition synchronously.
      const CostScalar partsPerPA = activePartitions/countOfPAs;

      if (partsPerPA.isGreaterThanOne())
        {

          // ---------------------------------------------------------------
          // One PA is accessing more than one partition.
          // this means that every PA is doing:
          //   ******SEQUENTIAL ACCESS********
          // (it is accessing partsPerPA partitions sequentially).
          // Thus we need to create a cost vector that
          // adds up the CPU and I/O respectively.
          //
          // We do it here because
          // the number of plan fragments is used by the Cost factor
          // to convert the "per-stream" cost in the simple cost vectors
          // to a "per-cpu" cost. This is done by "overlapped" adding each
          // simple cost vector "plan fragment" times. This is OK for the
          // case of asynchronous acces of partitions. But if we need
          // to access the partitions synchronously, i.e sequentially, then
          // we want to do "full" addition because no overlapping will
          // exists for partitions in the same CPU. We do this below.
          // ----------------------------------------------------------------

          tempLast.setCPUTime(tempLast.getCPUTime() * partsPerPA);
	  tempLast.setIOTime (tempLast.getIOTime() * partsPerPA);

          // The first row costs should NOT be increased, they
          // are already for one row only and they do not show any
          // asynchronous cost savings, so no need to increase them
          // for synchronous access!
          // tempFirst.setCPUCost(tempFirst.getCPUCost() * partsPerPA);
          // tempFirst.setNumKBytes(tempLast.getNumKBytes() * partsPerPA);
          // tempFirst.setNumSeeks(tempLast.getNumSeeks() * partsPerPA);

        }
      else
        {
          // -------------------------------------------------------
          // Else there is one partition per PA, thus it is
          //   ******ASYNCHRONOUS ACCESS********
          // let the cost object do its work,
          // that is, overlapp add the partitionsPerCPU plan fragments
          // to obtain the cost per cpu
          // -------------------------------------------------------
        }


    } // lppf != NULL

  // Compute the number of streams per CPU:
  partitionsPerCPU =
    ((CostScalar)countOfPAs/(CostScalar)countOfCPUsExecutingDP2s).getCeiling();

  // Save the idle time of LR and FR and then reset it
  // after the Cost object has been constructed. Do this
  // because scan uses idle time to store the cost of
  // opening a table and in the case of multiple
  // partitions the Cost constructor modifies  idle time.

  // the idle time for FR and LR is the same, so get it from any of them:
  CostScalar idleTime = tempLast.getIdleTime();

  // IOTime is already per volume and should not be incremented
  CostScalar lrIOTime = tempLast.getIOTime();

  CostScalar frIOTime = tempFirst.getIOTime();

#ifndef NDEBUG
  Lng32 planFragmentsPerCPU = (Lng32)partitionsPerCPU.getValue();

  if ( planFragmentsPerCPU > 1 AND CURRSTMT_OPTGLOBALS->synCheckFlag )
    (*CURRSTMT_OPTGLOBALS->asynchrMonitor).enter();
#endif  //NDEBUG

  tempLast.setIdleTime(0.);
  tempFirst.setIdleTime(0.);
  Cost * costPtr = new HEAP Cost(&tempFirst,
				 &tempLast,
                                 NULL,
				 countOfCPUsExecutingDP2s,
                                 (Lng32)partitionsPerCPU.getValue());

  costPtr->cpfr().setIdleTime(idleTime);
  costPtr->cplr().setIdleTime(idleTime);
  costPtr->totalCost().setIdleTime(idleTime);

  costPtr->cpfr().setIOTime(frIOTime);
  costPtr->cplr().setIOTime(lrIOTime);
  costPtr->totalCost().setIOTime(lrIOTime);

#ifndef NDEBUG
  if ( planFragmentsPerCPU > 1 AND CURRSTMT_OPTGLOBALS->synCheckFlag)
    (*CURRSTMT_OPTGLOBALS->asynchrMonitor).exit();
#endif  //NDEBUG

  DCMPASSERT(costPtr != NULL);

  return costPtr;
} // computeCostObject(...)
// -----------------------------------------------------------------------
// Use this routine to find a basic cost object to share.
// INPUT:
//   implicitly uses Context, IndexDesc and scanBasicCosts_ of indexDesc
// OUTPUT
//   sharedCostFound is TRUE if on the basic cost objects can be reused
//   function returns pointer to this object
//   if sharedCostFound is FALSE the function returns the pointer to
//   the new object in the list and its simple cost vectors need to be
//   recomputed
// -----------------------------------------------------------------------

FileScanBasicCost *
ScanOptimizer::shareBasicCost(NABoolean &sharedCostFound)
{
  const Context & currentContext  =  getContext();
  IndexDesc * indexDesc = (IndexDesc *)getIndexDesc();
  FileScanCostList *
    fileScanCostList = (FileScanCostList *)indexDesc->getBasicCostList();
  if ( fileScanCostList == NULL )
  {
    // first call to the function for this index descriptor,
    // create a new FileScanCostList object and save pointer to it
    // in index descriptor attribute.
    fileScanCostList = new (STMTHEAP) FileScanCostList (STMTHEAP);
    DCMPASSERT(fileScanCostList != NULL);
    indexDesc->setBasicCostList(fileScanCostList);
  }

  // go through the list of BasicCost objects trying to find
  // basic cost to reuse
  CollIndex maxc = fileScanCostList->entries();
  for (CollIndex i = 0; i < maxc; i++)
  {
    FileScanBasicCost * existingBasicCostPtr = (*fileScanCostList)[i];
    if ( existingBasicCostPtr->hasSameBasicProperties(currentContext) )
    {
      sharedCostFound = TRUE;
      return existingBasicCostPtr;
    }
  }

  // no basic cost to share, create a new BasicCost object and
  // return its pointer to the calling function. In this case
  // computeCostVector function will work with this new BasicCost
  // object memory when calculating the cost.
  sharedCostFound = FALSE;
  FileScanBasicCost * newBasicCostPtr = new (CmpCommon::statementHeap())
    FileScanBasicCost((const Context *)&currentContext);
  fileScanCostList->insert(newBasicCostPtr);
  return newBasicCostPtr;
}

// -----------------------------------------------------------------------
// Methods for FileScanOptimizer:
// -----------------------------------------------------------------------

FileScanOptimizer::~FileScanOptimizer()
{
};

Cost *
FileScanOptimizer::optimize(SearchKey*& searchKeyPtr   /* out */
			    , MdamKey*& mdamKeyPtr     /* out */
			    )
{
  MDAM_DEBUG0(MTL2, "BEGIN Scan Costing ********************************");
  // -----------------------------------------------------------------------
  // The best cost is determined amongst these cases:
  // 1.- Single subset
  // 2.- MDAM common
  // 3,- MDAM disjuncts
  //
  // This method computes the cheapest key and returns its cost
  //
  // -----------------------------------------------------------------------



  // This method computes either a MdamKey or a SearchKey,
  // however, the receiving parameters must be NULL:
  DCMPASSERT(searchKeyPtr == NULL);
  DCMPASSERT(mdamKeyPtr == NULL);

  ValueIdSet partKeyPreds;
  NABoolean eliminateExePreds = TRUE;

  LogPhysPartitioningFunction *logPhysPartFunc =
    (LogPhysPartitioningFunction *)  // cast away const
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction()->
    castToLogPhysPartitioningFunction();

  if ( logPhysPartFunc != NULL )
  {
    LogPhysPartitioningFunction::logPartType logPartType =
      logPhysPartFunc->getLogPartType();

// Partitioning Key Predicates are only of interest if
// we have LOGICAL_SUBPARTITIONING or HORIZONTAL_PARTITION_SLICING
// LOGICAL_SUBPARTITIONING
//    More than one PA may access a given DP2 partition and a PA may
//    also access more than one DP2 partition. The PA nodes divide
//    the table into exclusive ranges that are defined by the
//    clustering key of the table which is also the partitioning key.
//
// HORIZONTAL_PARTITION_SLICING
//    Similar to LOGICAL_SUBPARTITIONING, except that the clustering
//    key is not the partitioning key of the table.
//  (HORIZONTAL_PARTITION_SLICING is currently not implemented (6/20/2001)
//
//
// If we have partitioning key predicates with one of the types of partitioning
//   mentioned above we will try to use these as access key predicates for
//   single subset or multiple subset.
// Access keys are created from the disjuncts that were created when the
//   rel expr was set up.  This was only done once, since the selection
//   predicates normally do not change.
// Since we want to add partitioning keys to the selection predicates we must
//   materialize new disjuncts that include the partitioning keys.
// We will build the new disjuncts and we will later choose to use the new
//   one or the old one based on whether or not we have partitioning key
//   predicates.

    if (    logPartType == LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING
	 OR logPartType == LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING
       )
       partKeyPreds = logPhysPartFunc->getPartitioningKeyPredicates();
  }

  Disjuncts * curDisjuncts = NULL; // = (Disjuncts&)getDisjuncts();
  if (NOT partKeyPreds.isEmpty())
    {
     ValueIdSet newPreds = (ValueIdSet)getRelExpr().getSelectionPred();
     newPreds += partKeyPreds;
     curDisjuncts = new HEAP MaterialDisjuncts(newPreds);
     eliminateExePreds = FALSE;
    }

  MDAM_DEBUGX(MTL2, MdamTrace::printFileScanInfo(this, partKeyPreds));


  // The cost of the winning key will be put here:
  Cost *winnerCostPtr = NULL;

  CostScalar winnerCostPtrNumKBytes;

  // To distinguish between MDAMCommon and MDAMDisjuncts methods
  NABoolean mdamTypeIsCommon = FALSE;

  MdamKey *sharedMdamCommonKeyPtr = NULL;
  MdamKey *sharedMdamDisjunctsKeyPtr = NULL;

  IndexDescHistograms histograms(*getIndexDesc()
                                  ,getIndexDesc()->getIndexKey().entries());

  // All histograms must have been created for non-duplicate key columns
  // This DCMPASSERT was used earlier to check for all duplicate entries too
  // but that is incorrect because we do not want to have duplicate entries
  // in the ColStatDescList. Hence modified the DCMPASSERT such that all
  // key columns should have only one entry in the ColStatDescList -
  // as part of the fix for Sol: 10-031105-1054

  ValueIdSet nonDuplicateKeyColumns = getIndexDesc()->getIndexKey();
  DCMPASSERT(histograms.entries() == nonDuplicateKeyColumns.entries());
  NABoolean
   isIndexJoin = histograms.isAnIndexJoin(*(getContext().getInputLogProp()));

#ifndef NDEBUG
 if (isIndexJoin) (*CURRSTMT_OPTGLOBALS->indexJoinMonitor).enter();
#endif

  // nonkeycolumns are needed by SearchKey and mdamkey:
  ValueIdSet nonKeyColumnSet;
  const IndexDesc * indexDesc = getIndexDesc();
  indexDesc->getNonKeyColumnSet(nonKeyColumnSet);

  NABoolean mdamForced = isMdamForced();

  // We have already done the check to see if MDAM can be done or not, while
  // testing to see if we can use simpleFileScanOptimizer. So, there is no need to
  // test it again. The only condition when it would not have been done, would be
  // when we force the compiler to use old FileScan optimizer. Check if MDAM can be
  // performed only in that condition.

  NABoolean canDoMdam = TRUE;
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  ULng32 fsoToUse = defs.getAsULong(FSO_TO_USE);
  if (fsoToUse == 0)
  {
    // force the compiler to use old FileScan Optimizer
    if (isMdamEnabled() AND NOT mdamForced)
    {
      if ( isIndexJoin || hasTooManyDisjuncts())
      {
	canDoMdam = FALSE;
      }
      else
      {
       if (histograms.containsAtLeastOneFake())
       {
	  // -------------------------------------------------------------
	  // If at least one histogram is fake, then we can't do
	  // MDAM (histograms contains a ColStatDescList with
	  // entries for key columns only)
	  // --------------------------------------------------------------
	  canDoMdam = FALSE;
  	  MDAM_DEBUG0(MTL2, "Fake histogram found, MDAM NOT considered.");
          MDAM_DEBUGX(MTL2, histograms.display());

	}
       else
       {
	 ValueIdSet externalInputs = getExternalInputs();
	 NABoolean mdamFlag = getMdamFlag();

	 if (NOT partKeyPreds.isEmpty())
	 {
	   canDoMdam = ScanOptimizer::canStillConsiderMDAM(partKeyPreds,
						 nonKeyColumnSet,
						 *curDisjuncts,
						 indexDesc,
						 externalInputs,
						 mdamFlag);
	 }
	 else
	 {
	   canDoMdam = ScanOptimizer::canStillConsiderMDAM(partKeyPreds,
						 nonKeyColumnSet,
						 getDisjuncts(),
						 indexDesc,
						 externalInputs,
						 mdamFlag);
	 }

       }
      }
    }
  } // fsotouse == 0

  // At this point if canDoMDam is TRUE then we can consider
  // MDAM. However, if we are forcing MDAM we will consider it
  // regardless of the state of canDoMdam.

  ValueIdSet exePred;
  NABoolean mdamIsWinner = FALSE;

  // For the MDAM costing rewrite, we need the subset size as calculated
  // by single subset scan. So we need to cost single subset scan even
  // if MDAM is forced.
  NABoolean singleSubsetMetricsNeeded = 
    (CmpCommon::getDefault(MDAM_COSTING_REWRITE) == DF_ON);

  if (mdamForced   // Force mdam case:
      AND
      NOT singleSubsetMetricsNeeded
      AND
      getIndexDesc()->getNAFileSet()->isKeySequenced())
    { // Mdam is our only choice:

      MDAM_DEBUG0(MTL2, "MDAM only (forced).");

      // Create mdam disjuncts key:
      if (partKeyPreds.isEmpty())
         mdamKeyPtr =  new HEAP
	    MdamKey(getIndexDesc()->getIndexKey()
		,getExternalInputs()
		,getDisjuncts()
                ,nonKeyColumnSet
                ,getIndexDesc()
		);
      else
         mdamKeyPtr =  new HEAP
	    MdamKey(getIndexDesc()->getIndexKey()
		,getExternalInputs()
		,*curDisjuncts
                ,nonKeyColumnSet
                ,getIndexDesc()
		);

      DCMPASSERT(mdamKeyPtr != NULL);
      // Cost the mdam disjuncts:


      winnerCostPtr =
         computeCostForMultipleSubset
         ( mdamKeyPtr
           ,NULL
           ,mdamForced
	   ,winnerCostPtrNumKBytes
           ,eliminateExePreds //TRUE eliminate exePreds if possible
           ,mdamTypeIsCommon  //FALSE, this is Disjuncts case
           ,sharedMdamDisjunctsKeyPtr
         );
      mdamIsWinner = TRUE;

    } // Mdam is forced
  else if ( NOT isMdamEnabled()
            OR
            NOT canDoMdam)
    {
      MDAM_DEBUG0(MTL2, "Single subset only.");

      // Single subset is our only choice
      // (mdam is not enabled OR mdam is enabled but the
      // file is not key sequenced)

      // Generate the search key to be returned, then
      // estimate its cost:
      ValueIdSet selPreds = getRelExpr().getSelectionPred();
      if ((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
	  (selPreds.entries()))
      {
	ItemExpr * inputItemExprTree = selPreds.rebuildExprTree(ITM_AND,FALSE,FALSE);
	ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(), inputItemExprTree);
	exePred.clear();
	resultOld->convertToValueIdSet(exePred, NULL, ITM_AND, FALSE);
	doNotReplaceAnItemExpressionForLikePredicates(resultOld,exePred,resultOld);
    }
      else
      {
        exePred = selPreds;
      }
	exePred += partKeyPreds;

      if (partKeyPreds.isEmpty())
         searchKeyPtr =  new(CmpCommon::statementHeap())
	    SearchKey(getIndexDesc()->getIndexKey()
		  ,getIndexDesc()->getOrderOfKeyValues()
		  ,getExternalInputs()
		  ,isForwardScan()
		  ,exePred
                  ,getDisjuncts()
                  ,nonKeyColumnSet
                 ,getIndexDesc()
                  );
      else
         searchKeyPtr =  new(CmpCommon::statementHeap())
	    SearchKey(getIndexDesc()->getIndexKey()
		  ,getIndexDesc()->getOrderOfKeyValues()
		  ,getExternalInputs()
		  ,isForwardScan()
		  ,exePred
                  ,*curDisjuncts
                  ,nonKeyColumnSet
                  ,getIndexDesc()
                  );

      winnerCostPtr = 
	computeCostForSingleSubset
	(
	 *searchKeyPtr
	 ,FALSE /* do not break */
	 ,winnerCostPtrNumKBytes   /* output */
	 );
    } // generate search key

  else
    {
      MDAM_DEBUG0(MTL2, "Compare single subset and MDAMs.");
      //------------------------------------------------
      // MDAM and SearchKey are both
      // enabled do the following:
      //
      // 1.- Compute costs for Mdam and SingleSubset
      // 2.- Compare costs
      // 3.- Create the Key for the winner (only one can win)

      // -----------------------------------------------------------
      // Create the keys:
      // ------------------------------------------------------------

      DCMPASSERT(winnerCostPtr == NULL);

      // Search key:
      ValueIdSet selPreds = getRelExpr().getSelectionPred();
      if((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
	 (selPreds.entries()))
      {
	ItemExpr * inputItemExprTree = selPreds.rebuildExprTree(ITM_AND,FALSE,FALSE);
	ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(), inputItemExprTree);
	exePred.clear();
	resultOld->convertToValueIdSet(exePred, NULL, ITM_AND, FALSE);
	doNotReplaceAnItemExpressionForLikePredicates(resultOld,exePred,resultOld);
//	doNotReplaceAnItemExpression(resultOld,exePred,resultOld);

//	exePred.clear();
//	revertBackToOldTreeUsingValueIdSet(selPreds, exePred);
//	ItemExpr* resultOld =  exePred.rebuildExprTree(ITM_AND,FALSE,FALSE);	
//	doNotReplaceAnItemExpressionForLikePredicates(resultOld,exePred,resultOld);

      }
      else
      {
        exePred = getRelExpr().getSelectionPred();
      }

	exePred += partKeyPreds;

      if (partKeyPreds.isEmpty())
         searchKeyPtr =  new(CmpCommon::statementHeap())
	    SearchKey(getIndexDesc()->getIndexKey()
		  ,getIndexDesc()->getOrderOfKeyValues()
		  ,getExternalInputs()
		  ,isForwardScan()
		  ,exePred
                  ,getDisjuncts()
                  ,nonKeyColumnSet
                  ,getIndexDesc()
		  );
      else
         searchKeyPtr =  new(CmpCommon::statementHeap())
	    SearchKey(getIndexDesc()->getIndexKey()
		  ,getIndexDesc()->getOrderOfKeyValues()
		  ,getExternalInputs()
		  ,isForwardScan()
		  ,exePred
                  ,*curDisjuncts
                  ,nonKeyColumnSet
                  ,getIndexDesc()
		  );


      Cost
	*singleSubsetCostPtr = NULL,
	*mdamCommonCostPtr = NULL,
	*mdamDisjunctsCostPtr = NULL;

      CostScalar
	singleSubsetCostPtrNumKBytes,
	mdamCommonCostPtrNumKBytes,
	mdamDisjunctsCostPtrNumKBytes;


      MdamKey
	*mdamCommonKeyPtr = NULL,
	*mdamDisjunctsKeyPtr = NULL;



      // -----------------------------------------------------
      // Keys will compete. The following will take place:
      // 1.- Only one key can win
      // 2.- The winning key will be set to either
      //     searchKeyPtr or mdamKeyPtr
      // 3.- The loser's keys and costs will be deleted
      //     and pointers set to NULL
      // ----------------------------------------------------

      // ------------------------------------------------------------
      // Compute single subset cost:
      // -------------------------------------------------------------
      MDAM_DEBUG0(MTL2, "Compute Single Subset Cost (Cost Bound)");
      winnerCostPtr = singleSubsetCostPtr =
        computeCostForSingleSubset(*searchKeyPtr
                                   ,TRUE /* Break if there is a conflict */
				   ,singleSubsetCostPtrNumKBytes    /* output */
                                  );
      winnerCostPtrNumKBytes = singleSubsetCostPtrNumKBytes;

      MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this, singleSubsetCostPtr,
	"Single Subset Cost"));

      // ------------------------------------------------------------
      // Compute mdam common cost:
      // -------------------------------------------------------------

      // If there is only a single disjunct, let
      // mdam disjuncts take care of it (it is
      // a little bit cheaper to construct a mdam key
      // for the disjuncts case):
      ValueIdSet commonPreds;
	  // Chose either the new disjuncts w/partitioning keys if we have
	  //   partitoning keys, otherwise use the original
      if ((NOT partKeyPreds.isEmpty()  AND
           curDisjuncts->entries() > 1)  OR
          (getDisjuncts().entries() > 1))
        {

          // Construct common preds:
          ValueIdSet commonPreds;
          if (partKeyPreds.isEmpty())
             commonPreds = getDisjuncts().getCommonPredicates();
          else
             commonPreds = curDisjuncts->getCommonPredicates();
          // Create disjuncts for this set of preds:
          if (NOT commonPreds.isEmpty())
          {
	    MDAM_DEBUG0(MTL2, "Consider Mdam Common Costing");
              Disjuncts *commonDisjunctsPtr = new HEAP
                MaterialDisjuncts(commonPreds);
              DCMPASSERT(commonDisjunctsPtr != NULL);

              mdamCommonKeyPtr =  new HEAP
                MdamKey(getIndexDesc()->getIndexKey()
                        ,getExternalInputs()
                        ,*commonDisjunctsPtr
                        ,nonKeyColumnSet
                        ,getIndexDesc()
                        );

              // Try this case only if there are key preds
              // in the common preds (one example where common
              // has no key pres is A=1 or A=3, where A is
              // a key column):
              ColumnOrderList keyPredsByCol(getIndexDesc()->getIndexKey());
              mdamCommonKeyPtr->getKeyPredicatesByColumn(keyPredsByCol,0);
              ValueIdSet keyPreds;
              keyPredsByCol.getAllPredicates(keyPreds);
              if (NOT keyPreds.isEmpty())
              {
                  DCMPASSERT(mdamCommonKeyPtr != NULL);

                  mdamTypeIsCommon = TRUE;
                  mdamCommonCostPtr =
                    computeCostForMultipleSubset
                    ( mdamCommonKeyPtr
                      ,winnerCostPtr /* bound */
                      ,mdamForced
		      ,mdamCommonCostPtrNumKBytes  /* output */
                      ,FALSE         /* don't eliminate exePreds */
                      ,mdamTypeIsCommon // TRUE this is Common case
                      ,sharedMdamCommonKeyPtr
                    );
	  	  MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this,
		                                               mdamCommonCostPtr,
							       "MDAM Common Cost"));
              } // there are  key preds in common preds
              else
              {
		MDAM_DEBUG0(MTL2,
                    "Common predicates has no key predicate, MDAM NOT considered");
		MDAM_DEBUGX(MTL2, commonPreds.display());
              } // NO common key preds in common preds

          } // common preds are not empty
	  else{
	    MDAM_DEBUG0(MTL2, "No Common Predicate,\n"
			"MDAM Common Costing NOT considered");
	  }


          // -----------------------------------------------------
          // Either the search key or mdam common key
          // must win. The loser's cost and key will be
          // deleted:
          // ------------------------------------------------------

          if (mdamCommonCostPtr != NULL)
          {
              // search key lost, delete it:
              winnerCostPtr = mdamCommonCostPtr;
              winnerCostPtrNumKBytes = mdamCommonCostPtrNumKBytes;
              mdamKeyPtr = mdamCommonKeyPtr;
              delete searchKeyPtr;
              searchKeyPtr = NULL;
              delete singleSubsetCostPtr;
              singleSubsetCostPtr = NULL;
          }
          else
          {
              // mdam common lost, delete it:
              winnerCostPtr = singleSubsetCostPtr;
              winnerCostPtrNumKBytes = singleSubsetCostPtrNumKBytes;
	      if (mdamCommonKeyPtr != sharedMdamCommonKeyPtr)
                delete mdamCommonKeyPtr;
              mdamCommonKeyPtr = NULL;
          }
      }
      else{
	MDAM_DEBUG0(MTL2, "MDAM Common Costing NOT applies");
      }

      DCMPASSERT( (searchKeyPtr == NULL AND mdamCommonKeyPtr != NULL)
                 OR
                 (searchKeyPtr != NULL AND mdamCommonKeyPtr == NULL) );


      //---------------------------------
      // Compute mdam disjuncts cost
      //---------------------------------

      MDAM_DEBUG0(MTL2, "Consider Mdam Disjuncts Costing");

      if (partKeyPreds.isEmpty())
	  // Chose either the new disjuncts w/partitioning keys if we have
	  //   partitoning keys, otherwise use the original
         mdamDisjunctsKeyPtr =  new HEAP
            MdamKey(getIndexDesc()->getIndexKey()
                ,getExternalInputs()
                ,getDisjuncts()
                ,nonKeyColumnSet
                ,getIndexDesc()
                );
      else
         mdamDisjunctsKeyPtr =  new HEAP
            MdamKey(getIndexDesc()->getIndexKey()
                ,getExternalInputs()
                ,*curDisjuncts
                ,nonKeyColumnSet
                ,getIndexDesc()
                );


      DCMPASSERT(mdamDisjunctsKeyPtr != NULL);

      MDAM_DEBUG0(MTL2, "call computeCostForMultipleSubset()");

      mdamTypeIsCommon = FALSE;
      mdamDisjunctsCostPtr =
        computeCostForMultipleSubset
        ( mdamDisjunctsKeyPtr
          ,winnerCostPtr   /* cost bound */
          ,mdamForced
	  ,mdamDisjunctsCostPtrNumKBytes	  /* output */
          ,eliminateExePreds //TRUE eliminate exePreds if possible
          ,mdamTypeIsCommon  //FALSE this is Disjuntcs case
          ,sharedMdamDisjunctsKeyPtr
        );

      MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this, mdamDisjunctsCostPtr,
					      "MDAM Disjuncts Cost"));

      if (mdamDisjunctsCostPtr != NULL)
      {
          // mdamDisjuncts won and search key or mdam common lost:
          winnerCostPtr = mdamDisjunctsCostPtr;
          winnerCostPtrNumKBytes = mdamDisjunctsCostPtrNumKBytes;
          mdamKeyPtr = mdamDisjunctsKeyPtr;
          if (searchKeyPtr == NULL)
            {
              // mdam common lost, delete it (because
              // a NULL searchKeyPtr means it was not
              // competing):
              DCMPASSERT(mdamCommonKeyPtr != NULL);
              delete mdamCommonCostPtr;
              mdamCommonCostPtr = NULL;
	      if (mdamCommonKeyPtr != sharedMdamCommonKeyPtr)
                delete mdamCommonKeyPtr;
              mdamCommonKeyPtr = NULL;
            }
          else
            {
              // search key lost, delete it:
              DCMPASSERT(searchKeyPtr != NULL);
              delete searchKeyPtr;
              searchKeyPtr = NULL;
              delete singleSubsetCostPtr;
              singleSubsetCostPtr = NULL;
            }
      }
      else
      {
          // mdam disjuncts lost, delete it:
          // the winner stays the same
          if (mdamDisjunctsKeyPtr != sharedMdamDisjunctsKeyPtr)
            delete mdamDisjunctsKeyPtr;
          mdamDisjunctsKeyPtr = NULL;
      }

      if (searchKeyPtr)
      {
	MDAM_DEBUG0(MTL2, "Winner is Single Subset");
      }
      else if (mdamCommonKeyPtr)
      {
	MDAM_DEBUG0(MTL2, "Winner is Mdam Common");
	mdamIsWinner = TRUE;
      }
      else if (mdamDisjunctsKeyPtr)
      {
	MDAM_DEBUG0(MTL2, "Winner is Mdam Disjuncts");
	mdamIsWinner = TRUE;
      }

  } // single subset vs others

  DCMPASSERT(winnerCostPtr != NULL); // somebody must win

  // one of the keys must be set:
  DCMPASSERT(searchKeyPtr != NULL OR mdamKeyPtr != NULL);
  // but not both:
  DCMPASSERT(NOT(searchKeyPtr != NULL AND mdamKeyPtr != NULL));


  MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this, winnerCostPtr,
      "Winner Cost"));

  // compute blocks read per access
  computeNumberOfBlocksToReadPerAccess(*winnerCostPtr
                                       ,mdamIsWinner, winnerCostPtrNumKBytes);
#ifndef NDEBUG
if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic2()) {
  if ( isIndexJoin )
    (*CURRSTMT_OPTGLOBALS->indexJoinMonitor).exit();
}
#endif  //NDEBUG

  MDAM_DEBUG0(MTL2, "END   Scan Costing ********************************\n\n");
  return winnerCostPtr;

} // FileScanOptimizer::optimize()

// Pass the join histograms when available
void
FileScanOptimizer::computeNumberOfBlocksToReadPerAccess(
            const Cost& scanCost, NABoolean &isMDAM, CostScalar numKBytes)
{

  // $$ Needs code to check performance goal, for now
  // $$ assume it is last row.

  CostScalar blockSizeInKb = getIndexDesc()->getBlockSizeInKb();


  // KB for index blocks plus data blocks for one partition for all
  // probes:
  CostScalar blocksForAllProbes =
    (numKBytes/blockSizeInKb).getCeiling();

  CostScalar probes = MIN_ONE(scanCost.getCplr().getNumProbes());


  // Substract index blocks to obtain the data blocks:
  CostScalar indexBlocksForAllProbes =
    getIndexDesc()->getEstimatedIndexBlocksLowerBound(probes);

  CostScalar dataBlocksForAllProbes =
    MIN_ONE(blocksForAllProbes - indexBlocksForAllProbes);

  // Estimate blocks per probe:
  CostScalar dataBlocksPerProbe = MIN_ONE(dataBlocksForAllProbes / probes);

  CostScalar dp2CacheSize = getDP2CacheSizeInBlocks(blockSizeInKb);


  // -------------------------------------------------------------
  // Compute the blocks in the inner table:
  // -------------------------------------------------------------
  const CostScalar totalRowsInInnerTable =
    getRawInnerHistograms().getRowCount().getCeiling();

  const CostScalar estimatedRecordsPerBlock =
    getIndexDesc()->getEstimatedRecordsPerBlock();

  CostScalar totalBlocksInFullTableScan;
  computeBlocksUpperBound(
       totalBlocksInFullTableScan /* out */
       ,totalRowsInInnerTable /* in */
       ,estimatedRecordsPerBlock /* in */
       );


  CostScalar blocksToRead;
   if (NOT probes.isGreaterThanOne() AND NOT isMDAM)
    {
      // One probe, non-mdam, is always one access, so
      // the blocks to read are the data blocks hit by the probe.
      blocksToRead = dataBlocksPerProbe;
    }
   // Below we handle the multiple probe case, MDAM and non-MDAM.
   // We put together MDAM with non-MDAM because we see MDAM access
   // as an instance of a "unique access"". This is, we think of
   // MDAM as if all of the blocks read in a single MDAM access were
   // contigous. This assumption results in that the minimum number
   // of blocks to read in the MDAM case will be given by the total
   // blocks read per MDAM access (i.e. per probe). This is a good
   // compromise.
  else if (getInOrderProbesFlag())
    {
      // Multiple probe case (NJ), in-order probes,
      // maybe single probe MDAM or multiple in-order probe MDAM
      // We read all of the blocks that the probes hit if the
      // probes are "near" or just the blocks per probe if the probes
      // are "apart". The reason is that if the probes are near read-ahead
      // will be of benefit because probes that are waiting to be served
      // will hit the cache when there is read ahead

      const CostScalar density =
        CostPrimitives::getBasicCostFactor(COST_PROBE_DENSITY_THRESHOLD);
      DCMPASSERT(density >= 0 AND density <= 1.);

      // The density tells how "apart" are the probes
      // If the ratio of the total blocks that the
      // probes are hitting to the total blocks in
      // the table is greater than the "density" then
      // we say the blocks that the probes are
      // hitting are "close" to each other and thus
      // it is better to read all of the blocks for all
      // of the probes (Note that we are implicitly assuming
      // that every probe hits a different block. This is
      // obviously not the case in general. We could use
      // histograms to decide this)
      if (dataBlocksForAllProbes/totalBlocksInFullTableScan >= density)
        {
          // The probes are "near" each other, read all of the
          // blocks they touch (limit the blocks by the
          // table size):
          blocksToRead = MINOF(totalBlocksInFullTableScan.getValue()
                               ,dataBlocksForAllProbes.getValue());

        }
      else
        {
          // Blocks are apart, thus only read the blocks that
          // a single probe hits:
          blocksToRead = dataBlocksPerProbe;
        }
    } // MP, in-order
  else
    {  // NJ, single subset, random, maybe multiple random probe MDAM

      // Because we are randonmly probing the table
      // the probes cannot benefit by reading ahead, thus
      // no need to do read-ahead (unless the full table fits
      // in the DP2 cache!)
      if (totalBlocksInFullTableScan <= dp2CacheSize)
        {
          blocksToRead = totalBlocksInFullTableScan;
        }
      else
        {
          blocksToRead = dataBlocksPerProbe;
        }
    }


  DCMPASSERT(blocksToRead.isGreaterThanZero());

  //overflow occuring while casting it to long
	//the below is a check to avoid the overflow
	//CR 10-010815-4585
	if(blocksToRead.getValue() < double(INT_MAX))
  setNumberOfBlocksToReadPerAccess(Lng32(blocksToRead.getValue()));
  else
  setNumberOfBlocksToReadPerAccess(INT_MAX);

} // FileScanOptimizer::computeNumberOfBlocksToReadPerAccess(...)


// -----------------------------------------------------------------------
// Use this routine to compute the cost of a given SearchKey
// INPUT:
//  sarchKey: the SearchKey to cost
//  histograms: Raw (i.e. predicates have not been applied)
//              histograms for the scan table
//  breakOnConflict: If TRUE then the Cost* returned will be NULL
//                   to indicate that there is a conflict in
//                   the predicate expression associated with the key,
//                   If FALSE the routine returns the cost of the
//                   SearchKey
//
// OUTPUT:
//  A NON-NULL Cost* indicating the Cost for searchKey if breakOnConflict
//  was not true OR (if breakOnConflict was true and there was
//  a conflict in the predicate expression)
//
//  A NULL Cost* indicating that breakOnConflict was true and there
//    was a conflict in the predicate expression.
// -----------------------------------------------------------------------
Cost*
FileScanOptimizer::computeCostForSingleSubset(
     SearchKey& searchKey,
     const NABoolean& weAreConsideringMDAM,
     CostScalar & numKBytes)

{
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    return  scmComputeCostForSingleSubset();
  }

  MDAM_DEBUG0(MTL2, "BEGIN Single Subset Costing --------");

  // This was added as part of the project
  // to reduce compilation time by reusing simple cost vectors. 09/18/00

  NABoolean sharedCostFound = FALSE;
  FileScanBasicCost *
    fileScanBasicCostPtr = shareBasicCost(sharedCostFound);
  SimpleCostVector &
    firstRow = fileScanBasicCostPtr->getFRBasicCostSingleSubset();
  SimpleCostVector &
    lastRow = fileScanBasicCostPtr->getLRBasicCostSingleSubset();


#ifndef NDEBUG
  (*CURRSTMT_OPTGLOBALS->singleSubsetCostMonitor).enter();
#endif  //NDEBUG

  // $$$ This only works for key sequenced files, remove later
  //  DCMPASSERT(getIndexDesc()->getNAFileSet()->isKeySequenced());
  // $$$ Add special code for entry sequence and relative
  // $$$ sequence files

  // -----------------------------------------------------------------------
  // Compute SearchKey dependant info:
  // -----------------------------------------------------------------------


  const ValueIdSet &exePreds = searchKey.getExecutorPredicates();

  ColumnOrderList keyPredsByCol(getIndexDesc()->getIndexKey());
  searchKey.getKeyPredicatesByColumn(keyPredsByCol);
  ValueIdSet keyPredicates;
  keyPredsByCol.getAllPredicates(keyPredicates);

  // -----------------------------------------------------------------------
  // Chech for a conflicting predicate expression and compute the
  // single subset predicates.
  //
  // If there is a conflict and we are considering MDAM, favor mdam
  // by returning a NULL cost for the single subset:
  //
  // The single subset preds are all those key preds up to
  // (but not including) the first missing key column
  // -----------------------------------------------------------------------

  // Compute the column position just before the first missing
  // key column (singleSubsetPrefixColumn, if it is greater than cero,
  // otherwise the code needs to explicitly check for the possibility
  // of not having predicates in the zeroth order):
  CollIndex singleSubsetPrefixColumn;
  NABoolean itIsSingleSubset =
    keyPredsByCol.getSingleSubsetOrder(singleSubsetPrefixColumn);
  DCMPASSERT(itIsSingleSubset);

  // return NULL if conflict, compute single subset preds:
  const ValueIdSet *nonMissingKeyColumnPredsPtr = NULL;
  ValueIdSet singleSubsetPreds;
  const ValueIdSet *partPreds = NULL;
  CollIndex leadingPartPreds = 0;

  for (CollIndex i=0; i <= singleSubsetPrefixColumn; i++)
    {
      if (i == leadingPartPreds)
        {
         partPreds = keyPredsByCol[i];
         if (partPreds AND NOT partPreds->isEmpty())
           {
            ValueId predId;
            BiRelat * predIEptr;
            for(predId=partPreds->init();
                partPreds->next(predId);
                partPreds->advance(predId))
              {
               if (predId.getItemExpr()->getArity() == 2)
                 {
                  predIEptr=(BiRelat *)predId.getItemExpr();
                  if (predIEptr->isaPartKeyPred())
                    {
                     leadingPartPreds += 1;
                     break;
                    }
                 }
              }
           }
         }
      if (keyPredsByCol.getPredicateExpressionPtr(i))
	{
          NABoolean unUsablePred =
                ((keyPredsByCol.getPredicateExpressionPtr(i)->getType() ==
		 KeyColumns::KeyColumn:: CONFLICT)
		OR
		(keyPredsByCol.getPredicateExpressionPtr(i)->getType() ==
		 KeyColumns::KeyColumn::CONFLICT_EQUALS));
	  if (  weAreConsideringMDAM
		AND
		unUsablePred )
	    {
#ifndef NDEBUG
	      (*CURRSTMT_OPTGLOBALS->singleSubsetCostMonitor).exit();
#endif  //NDEBUG
	      MDAM_DEBUG0(MTL2,
                  "Single Subset Cost is NULL, conflicts in predicate.");
	      MDAM_DEBUG0(MTL2, "END   Single Subset Costing --------\n");
              return NULL;
	    }

         nonMissingKeyColumnPredsPtr = keyPredsByCol[i];
         if (unUsablePred)
            {
              ValueIdSet setOfPreds = searchKey.getExecutorPredicates();
              setOfPreds.insert(*nonMissingKeyColumnPredsPtr);
              searchKey.setExecutorPredicates(setOfPreds);
            }

	  if (nonMissingKeyColumnPredsPtr != NULL)
	    {
	      singleSubsetPreds.insert(*nonMissingKeyColumnPredsPtr);
	    }
	}
      else
        {
          // The only case when an order less than or equal
          // to the single subset prefix order is empty is
          // when the order is cero:
          DCMPASSERT(singleSubsetPrefixColumn==0);
          break;
        }

    } // for every key col in the sing. subset prefix

  // If we reach here then singleSubsetPreds have been correctly computed

  // -----------------------------------------------------------------------
  // Estimate seeks and sequential_kb_read:
  // -----------------------------------------------------------------------
  CostScalar
    seeks // total number of disk arm movements
    ,seq_kb_read=csZero; // total number of blocks that were read sequentially


  // Do the shared cost return here because of the possibility of resetting
  //  the executor predicates above.

    if ( sharedCostFound AND
       firstRow.getCPUTime() > csZero AND
       lastRow.getCPUTime() > csZero
     )
  {
    if ( CURRSTMT_OPTDEFAULTS->reuseBasicCost() )
    {
      Cost * costPtr = computeCostObject(firstRow, lastRow);
      numKBytes = fileScanBasicCostPtr->getSingleSubsetNumKBytes();
      MDAM_DEBUG0(MTL2, "Reuse Basic Cost");
      MDAM_DEBUG0(MTL2, "END   Single Subset Costing --------\n");
      return costPtr;
    }
    else
    {
      firstRow.reset();
      lastRow.reset();
    }
  }


	CostScalar temp_dataRows=csZero;
  CostScalar * dataRows=&temp_dataRows; // the number of rows that match
  // Are we under a nested join?

  // -----------------------------------------------------------------------
  //  The probes are the TOTAL probes coming from the scan's parent. But
  //  depending on the key predicates and the partitioning key, the scan
  //  may be actually receiving fewer probes. The probes that the
  //  scan is actually receiving is given by the repeat count.
  // -----------------------------------------------------------------------
  CostScalar repeatCount = getContext().getPlan()->getPhysicalProperty()->
    getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2() ;

  // Costing works estimating the resources of the whole, unpartitioned
  // table, thus get the total probes by multiplying by the active
  // partitions
  CostScalar
    probes = repeatCount * getNumActivePartitions()
    ,successfulProbes = csOne; // all the probes that matched data

  CostScalar finalRows=getResultSetCardinality();

  // Patch for Sol.10-031024-0755 (Case 10-031024-9406). When RI
  // constraints were implemented, corresponding histograms, cardinality
  // and costing were forgotten. As a result nested join into referenced
  // table does not pass histogram information to the right child.
  // If the right child does not have histograms in inputLogProp
  // it considers this join as a cross product. So, for file_scan_unique
  // the number of rows is the product of the number of right child rows
  // and the number of probes. Then the cost of file_scan_unique is greatly
  // overestimated. The patch will set this number by the number of
  // probes which should be OK for RI constraints check.
  // The second part of this patch is in categorizeProbes. The number of
  // rows to cost was 0 in case of empty outputHistograms which is not
  // right if we have a real cross product, I changed it to the product
  // of probes and table cardinality.
  if ( (CmpCommon::getDefault(COMP_BOOL_34) == DF_OFF) AND
        searchKey.isUnique()
     )
     finalRows = probes;

  DCMPASSERT(repeatCount <= probes);

  const CostScalar estimatedRecordsPerBlock =
    getIndexDesc()->getEstimatedRecordsPerBlock();
  const CostScalar blockSizeInKb = getIndexDesc()->getBlockSizeInKb();


  // Is this a Scan on the right-hand side of a Nested Join?
  // Typically, for nested joins, the repeatCount will be greater than
  // one AND the input ColStats will be non-empty.  However, in some
  // cases such as cross-products, the input ColStats are empty.
  // Also, in some cases repeat count is 1, but ColStats is not emtpy.
  // Here we treat this case as a multiprobe.
  //
  if ((repeatCount.isGreaterThanOne()) OR
      (getContext().getInputLogProp()->getColStats().entries() > 0))
    {

// CR 10-010822-4815
// Query 04 in OBI was producing a bad plan as the seeks for Nested Join were
// computed without taking into account if the blocks of inner table are
// consecutive or not. This is fixed by computing the seeks after getting the
// right count of blocks by getting the rows for the key predicates on the leading
// columns with a Value Equality Group referencing a constant.
// "totalPreds" is the set of all predicates of all key columns with a constant
// for all the index values find the keyPredicates, for every predicate check
// if it is a VEG and that VEG has a constant, if so then add the predicate to
// the "totalPreds". Repeat this only for the leading columns, if the column
// doesn't have a constant then stop searching. Apply the "totalPreds" to the
// histograms to get the rowcount, divide the rowcount by recordsperblock to
// get the blocks. If no constant then pass the total blocks for inner table

	ValueIdSet totalPreds;
	CollIndex columnWithAConstExpr = NULL_COLL_INDEX;
	NABoolean hasAtleastOneConstExpr = false;
	CostScalar innerBlocksInSequence; //Blocks to be passed to compute seeks
	const ValueIdSet *keyPreds = NULL;

	for (CollIndex Indx=0; Indx <= singleSubsetPrefixColumn; Indx++)
	{
          keyPreds = keyPredsByCol[Indx];	//column which is part of key
	  NABoolean curFound = FALSE;

	  if (keyPreds AND NOT keyPreds->isEmpty())
          {
	    ValueId predId;

	    for(predId=keyPreds->init(); //Inner for loop for all the
	        keyPreds->next(predId);	 //the predicates of the column
	        keyPreds->advance(predId))
            {
		const ItemExpr *pred = predId.getItemExpr();
		if ( ((VEGPredicate *)pred)->getOperatorType()
			== ITM_VEG_PREDICATE )
		{
 		  const VEG * predVEG = ((VEGPredicate *)pred)->getVEG();
		  const ValueIdSet & VEGGroup = predVEG->getAllValues();

		  if (VEGGroup.referencesAConstExpr())
                  {
		    totalPreds += predId;
		    columnWithAConstExpr = Indx;
		    hasAtleastOneConstExpr = true;
		    curFound = TRUE;
                  }//end of check for A Constant Expression
		  else
		    break;
		}//end of "if" Operator to be VEG predicate

             }//end of inner loop "for(predId=partPreds->init();"

          }//end of "if" predicates to be non empty

	  if (NOT curFound)
	    break;

	}//end of outer loop "for (CollIndex i=0; i <= singleSubsetPre..



      // it seems that we are under a nested join thus...
      CostScalar
          failedProbes = csZero	   // all the probes that fail to matched data
	 ,uniqueSuccProbes = csZero   // all the probes that don't have duplicates
	 ,duplicateSuccProbes =csZero // duplicates among the unique succ. probes
         ,uniqueFailedProbes =csZero; //
 				// unique failed probes to compute seeks

      // -------------------------------------------------------------
      // Compute the blocks in the inner table:
      // -------------------------------------------------------------
      const CostScalar
			totalRowsInInnerTable =
        getRawInnerHistograms().getRowCount().getCeiling();

      CostScalar innerBlocksUpperBound;
      computeBlocksUpperBound(
           innerBlocksUpperBound
           ,totalRowsInInnerTable
           ,estimatedRecordsPerBlock);


      // -------------------------------------------------------------
      // Compute innerHistograms
      // -------------------------------------------------------------

      // first get the histograms for the probes:
      Histograms
          outerHistograms(getContext().getInputLogProp()->getColStats());

      // Then get the histograms for the inner table (this scan):
      IndexDescHistograms innerHistograms(*getIndexDesc(),
                                          singleSubsetPrefixColumn+1);

      // It is better to initialize it to the inner table cardinality than
      // left it as zero. dataRows will be adjusted in categorizeProbes
      // based on outerHistograms information.
      *dataRows = innerHistograms.getRowCount();

      categorizeProbes(successfulProbes /* out */
		       ,uniqueSuccProbes /* out */
		       ,duplicateSuccProbes /* out */
                       ,failedProbes /* out */
		       ,uniqueFailedProbes	 
		       ,probes
		       ,singleSubsetPreds
		       ,outerHistograms
                       ,FALSE // this is not MDAM!
                       ,dataRows
		       );
      CostScalar uniqueProbes =
           uniqueSuccProbes + uniqueFailedProbes; 

      setProbes(probes);
      setSuccessfulProbes(successfulProbes);
      setUniqueProbes(uniqueSuccProbes + uniqueFailedProbes);
      setDuplicateSuccProbes(duplicateSuccProbes);

      // Patch for Sol.10-031024-0755 (Case 10-031024-9406). See above.
      // If the number of probes is relatively small then we want to use
      // file_scan_unique instead of full index scan. Each probe will
      // bring one block to DP2 cache.
      if ( (CmpCommon::getDefault(COMP_BOOL_34) == DF_OFF) AND
            searchKey.isUnique() AND
        outerHistograms.isEmpty()
         )
        *dataRows = probes*estimatedRecordsPerBlock;

      // -----------------------------------------------------------
      // Compute the rows and blks per successful probe:
      // -----------------------------------------------------------
      // $$$ It would be best to get blocks instead of rows...


      CostScalar rowsPerSuccessfulProbe = csZero;
      if (NOT successfulProbes.isLessThanOne())
	{
	  rowsPerSuccessfulProbe = *dataRows/successfulProbes;
	}

      CostScalar blksPerSuccProbe =
          ( rowsPerSuccessfulProbe / estimatedRecordsPerBlock).getCeiling();
      // No successful probe can grab less than one block:
      if (NOT successfulProbes.isLessThanOne())
	{
	  blksPerSuccProbe.minCsOne();
	}


      // --------------------------------------------------------------
      // Check whether the whole answer set fits into cache:
      // --------------------------------------------------------------

      // compute the blocks in the answer set after key predicates
      // were applied (they cannot be more than the total blocks in
      // the raw table):
      // getCeiling() because it is an upper bound...
      const CostScalar totalUniqueSuccProbes =
          uniqueSuccProbes * getNumActivePartitions();
      CostScalar totalBlocksLowerBound; // the blocks in the answer set
      computeTotalBlocksLowerBound(
           totalBlocksLowerBound /* out */
           ,totalUniqueSuccProbes
           ,rowsPerSuccessfulProbe
           ,estimatedRecordsPerBlock
           ,innerBlocksUpperBound /* the total blocks in the inner table */
           );


      CostScalar cacheSizeInBlocks =
	getDP2CacheSizeInBlocks(blockSizeInKb);
      CostScalar indexBlocksLowerBound =
	getIndexDesc()->getEstimatedIndexBlocksLowerBound(probes);

      // The begin blocks denote the starting data
      // block for the subset that each succ. probe generates
      CostScalar beginBlocksLowerBound;
      computeBeginBlocksLowerBound(
           beginBlocksLowerBound /* out */
           ,uniqueSuccProbes
           ,innerBlocksUpperBound);

      // --------------------------------------------------------------------
      // At this point we are looking at the selectivity for
      // *ALL* partitions, thus the test for whether the answer set
      // fits in the cache must account for the cache in all partitions:
      // --------------------------------------------------------------------

      // First determine whether probes order matches the scan's
      // clustering order:
      const InputPhysicalProperty* ippForMe =
        getContext().getInputPhysicalProperty();
      NABoolean partiallyInOrderOK = TRUE;
      NABoolean probesForceSynchronousAccess = FALSE;
      if ((ippForMe != NULL) AND
          ordersMatch(ippForMe,
                      getIndexDesc(),
                      &(getIndexDesc()->getOrderOfKeyValues()),
                      getRelExpr().getGroupAttr()->getCharacteristicInputs(),
                      partiallyInOrderOK,
                      probesForceSynchronousAccess))
      {
        setInOrderProbesFlag(TRUE);
        setProbesForceSynchronousAccessFlag(probesForceSynchronousAccess);
      }
      else if ((ippForMe != NULL) AND
	      (ippForMe->getAssumeSortedForCosting()) AND
              (!(ippForMe->getExplodedOcbJoinForCosting())) AND
              (ippForMe->getNjOuterOrderPartFuncForNonUpdates() == NULL))

      {
         // assumeSortedForCosting_ flag is set for two cases:
         // case 1: when input is rowset.
         // case 2: when left child partFunc njOuterPartFuncForNonUpdates_ 
         //         is passed for NJ plan 0. This is only for cost estimation 
         //         of exchange operator and not scan operator.
         // So we should come here only for case1. 
         // To avoid case2, we check njOuterPartFuncForNonUpdates_ for NULL.
	 setInOrderProbesFlag(TRUE);
      }
      else
      {
        setInOrderProbesFlag(FALSE);
      }

      // Determine amount of DP2 cache is available per concurrent user.
      NADefaults &defs                 = ActiveSchemaDB()->getDefaults();
      const CostScalar concurrentUsers = defs.getAsLong(NUMBER_OF_USERS);
      CostScalar cacheForAllVolumes    =   cacheSizeInBlocks
                                         * getNumActiveDP2Volumes()
                                         / concurrentUsers;

      NABoolean inOrderProbes = getInOrderProbesFlag();
      CostScalar seekComputedWithDp2ReadAhead;
      // See if blocks to be read fit in cache.


      if( hasAtleastOneConstExpr )
      {
	IndexDescHistograms innerHistograms(*getIndexDesc(),
                                        columnWithAConstExpr+1);
	const SelectivityHint * selHint = getIndexDesc()->getPrimaryTableDesc()->getSelectivityHint();
	const CardinalityHint * cardHint = getIndexDesc()->getPrimaryTableDesc()->getCardinalityHint();

	innerHistograms.applyPredicates(totalPreds, getRelExpr(), selHint, cardHint, REL_SCAN);

	//GET ROW count
	const CostScalar totalRowCount = innerHistograms.getRowCount();

	//GET BLOCKS which are in sequence
	innerBlocksInSequence = totalRowCount / estimatedRecordsPerBlock;
      }
      else
	innerBlocksInSequence = innerBlocksUpperBound;

      if (totalBlocksLowerBound <= cacheForAllVolumes)
      {
	  // It fits, FULL cache benefit:
	  computeIOForFullCacheBenefit(seeks /* out */
				       ,seq_kb_read /* out */
				       ,beginBlocksLowerBound
				       ,totalBlocksLowerBound
				       ,indexBlocksLowerBound);
          computeSeekForDp2ReadAheadAndProbeOrder(
              seekComputedWithDp2ReadAhead
              ,finalRows
              ,uniqueProbes
              ,beginBlocksLowerBound
              ,totalBlocksLowerBound
              ,innerBlocksInSequence
              ,cacheForAllVolumes
              ,inOrderProbes);

          seeks = indexBlocksLowerBound +
              seekComputedWithDp2ReadAhead.getCeiling();

      }
      else
      {
	  // the answer set does not fit in its entirety,

	  if (getInOrderProbesFlag())
	    {

	      // probes order and inner order match!

	      // We have three cases:
	      // 1.- inner & outer match no duplicates
	      // 2.- inner & outer match duplicates, data
	      //     each successful duplicate matches fits
	      //     in cache
	      // 3.- inner & outer match duplicates, data
	      //     each successful duplicate matches does not fit
	      //     in cache
	      if ( NOT duplicateSuccProbes.isGreaterThanZero() )
              {
		  // in order, no duplicates
		  // blocks are read once independently of the cache:
		  computeIOForFullCacheBenefit(
                        seeks
		       ,seq_kb_read
		       ,beginBlocksLowerBound
		       ,totalBlocksLowerBound
		       ,indexBlocksLowerBound);
                  computeSeekForDp2ReadAheadAndProbeOrder(
                        seekComputedWithDp2ReadAhead
                       ,finalRows
                       ,uniqueProbes
                       ,beginBlocksLowerBound
                       ,totalBlocksLowerBound
                       ,innerBlocksInSequence
                       ,cacheForAllVolumes
                       ,inOrderProbes);

                  seeks = indexBlocksLowerBound +
                      seekComputedWithDp2ReadAhead.getCeiling();
              }
	      else // in order but duplicates exist
              {
	        // there are some successful duplicates
		if (blksPerSuccProbe <= cacheSizeInBlocks)
		{
		      // in order, duplicates, data for duplicates
		      // fits in cache:

		      // blocks are read once independent of the cache:
		      computeIOForFullCacheBenefit(
                          seeks /* out */
			 ,seq_kb_read /* out */
			 ,beginBlocksLowerBound
			 ,totalBlocksLowerBound
			 ,indexBlocksLowerBound);
                      computeSeekForDp2ReadAheadAndProbeOrder(
                           seekComputedWithDp2ReadAhead
                          ,finalRows
                          ,uniqueProbes
                          ,beginBlocksLowerBound
                          ,totalBlocksLowerBound
                          ,innerBlocksInSequence
                          ,cacheForAllVolumes
                          ,inOrderProbes);

                      seeks = indexBlocksLowerBound +
                          seekComputedWithDp2ReadAhead.getCeiling();
		}
		else // data blocks per duplicate does not fit in cache!
		{
		      // in order, duplicates, data for duplicates
		      // does not fits in cache:

		      // extra blocks will need to be read for
		      // duplicates:
		      const CostScalar
			extraDuplicateProbes = duplicateSuccProbes;
		      const CostScalar extraDuplicateBlocks =
			extraDuplicateProbes * blksPerSuccProbe;

		      const CostScalar
			seqBlocksRead =
 			CostScalar(
			     totalBlocksLowerBound
			     + indexBlocksLowerBound
                             +extraDuplicateBlocks
                             ).
			getCeiling();

		      seq_kb_read = seqBlocksRead * blockSizeInKb;

		      // We assume that the index blocks will be there
		      // for the duplicate to find its data,
		      // thus no need to add the extraDuplicate
		      // probes index blocks seeks.
                      seeks =
			CostScalar(beginBlocksLowerBound
				   + indexBlocksLowerBound).getCeiling();
                      computeSeekForDp2ReadAheadAndProbeOrder(
                          seekComputedWithDp2ReadAhead
                          ,finalRows
                          ,uniqueProbes
                          ,beginBlocksLowerBound
                          ,totalBlocksLowerBound
                          ,innerBlocksInSequence
                          ,cacheForAllVolumes
                          ,inOrderProbes);

                   seeks = indexBlocksLowerBound +
                       seekComputedWithDp2ReadAhead.getCeiling();


		} // if data for duplicates does not fit

              } // if there are duplicates
	    }
	  else // if orders don't match
	    {
	      // orders don't match, RANDOM case:
	      computeIOForRandomCase(seeks
				     ,seq_kb_read
				     ,blksPerSuccProbe
				     ,beginBlocksLowerBound
				     ,totalBlocksLowerBound
				     ,successfulProbes
				     ,failedProbes
				     ,indexBlocksLowerBound);

              computeSeekForDp2ReadAheadAndProbeOrder(
                 seekComputedWithDp2ReadAhead, finalRows, uniqueProbes,
		 beginBlocksLowerBound, totalBlocksLowerBound,
		 innerBlocksInSequence,cacheForAllVolumes,
                 inOrderProbes);

                 seeks = indexBlocksLowerBound +
              seeks = indexBlocksLowerBound + 
                 seekComputedWithDp2ReadAhead.getCeiling();
            }

      } // if answer set does not fit into cache
    } // if probes > 0 and there are stats for probes
  else
    {
      // not a nested join, thus no cache benefit is possible

      // make sure probes are at least one (to satisfy Roll-up formulas):
      successfulProbes = probes = csOne;

      // probes = 1, no cache benefit


      // Apply those key predicates that reference key columns
      // before the first missing key to the histograms:
      IndexDescHistograms innerHistograms(*getIndexDesc(),
                                          singleSubsetPrefixColumn+1);

      innerHistograms.applyPredicates(singleSubsetPreds, getRelExpr(), NULL, NULL, REL_SCAN);

      // Now, compute the number of rows:
      *dataRows = innerHistograms.getRowCount();

      DCMPASSERT(*dataRows >= csZero);

      seeks = seq_kb_read = csZero;
      if (dataRows->isGreaterThanZero())
        {


          // Compute the index blocks touched:
          CostScalar indexBlocks =
            getIndexDesc()->getIndexLevels() - 1;
          //if ( indexBlocks < CostScalar(0.0) )
          //     indexBlocks = CostScalar(0.0);
          indexBlocks.minCsZero();

          // All rows are contiguous (single subset), thus compute
          // the i/o:
          // and all rows are packed together:
          CostScalar blocksRead =
            (*dataRows/estimatedRecordsPerBlock).getCeiling()
            +
            indexBlocks;

          seq_kb_read = blocksRead*blockSizeInKb;



          // The seeks are the one for each index block traversed
          // (except the root) plus one for the data block:
          seeks =
            indexBlocks
            +
            csOne;

          // If we have partitioning key predicates then we will be
          // reading different parts of the partition by different esps.
          // There will be some seeks going from one esp reading to
          // another.  Assume that we will at least read ahead
          // the normal amount (for now reduce possible seeks by 100).
          NADefaults &defs                 = ActiveSchemaDB()->getDefaults();
          const CostScalar maxDp2ReadInBlocks =
               CostScalar(defs.getAsULong(DP2_MAX_READ_PER_ACCESS_IN_KB))/
               blockSizeInKb;

          if (leadingPartPreds > 0 AND blocksRead > maxDp2ReadInBlocks)
          {
             seeks += blocksRead / maxDp2ReadInBlocks / 100;
          }

          // temporary patch for solution 10-041104-1450. To make hash join
          // look more expensive that nested join if right table is not very
          // small we add one seek to the right scan.
          if ( blocksRead > csTwo )
          {
             seeks++;
             blocksRead++;
          }

          DCMPASSERT(seeks <= blocksRead);

      }

    } // if probes == 1.0


  // -----------------------------------------------------------------------
  // We have computed the transfer cost (seeks, seq_kb_read),
  // now compute the cost vectors:
  // -----------------------------------------------------------------------

  CostScalar seqKBytesPerScan;

  // Ceilings for the final values are taken inside computeCostVectors
#ifndef NDEBUG
  (*CURRSTMT_OPTGLOBALS->singleVectorCostMonitor).enter();
#endif  //NDEBUG
  computeCostVectors(firstRow // out
		     ,lastRow // out
		     ,seqKBytesPerScan //out
		     ,*dataRows
		     ,probes
		     ,successfulProbes
		     ,seeks
		     ,seq_kb_read
		     ,keyPredicates
		     ,exePreds
		     ,probes
		     );
#ifndef NDEBUG
   (*CURRSTMT_OPTGLOBALS->singleVectorCostMonitor).exit();
   (*CURRSTMT_OPTGLOBALS->singleObjectCostMonitor).enter();
   CURRSTMT_OPTGLOBALS->synCheckFlag = TRUE;
#endif  //NDEBUG

  // ---------------------------------------------------------------------
  // Done!, create the cost vector:
  // ---------------------------------------------------------------------

   Cost *costPtr = computeCostObject(firstRow, lastRow);

#ifndef NDEBUG
   CURRSTMT_OPTGLOBALS->synCheckFlag = FALSE;
   (*CURRSTMT_OPTGLOBALS->singleObjectCostMonitor).exit();
   (*CURRSTMT_OPTGLOBALS->singleSubsetCostMonitor).exit();
#endif  //NDEBUG

   numKBytes = seqKBytesPerScan;

   fileScanBasicCostPtr->setSingleSubsetNumKBytes(numKBytes);

   MDAM_DEBUG0(MTL2, "END   Single Subset Costing --------\n");
   return costPtr;

}// computeCostForSingleSubset(...)

#ifndef NDEBUG
void
FileScanOptimizer::runMdamTests
   ( const MdamKey* mdamKeyPtr,
     const Cost * costBoundPtr,
     NABoolean mdamForced,
     ValueIdSet exePreds,
     NABoolean checkExePreds,
     NABoolean mdamTypeIsCommon
   )
{
  enum MdamTraceLevel origLevel = MdamTrace::level();
  MdamTrace::setLevel(MTL1);
  MDAM_DEBUG1(MTL1, "Consider MDAM for Query:\n%s\n",
	      CmpCommon::statement()->userSqlText());


  NABoolean reUseBasicCost = CURRSTMT_OPTDEFAULTS->reuseBasicCost();
  CURRSTMT_OPTDEFAULTS->setReuseBasicCost(FALSE);

  CostScalar numKBytesOld;
  MdamKey *sharedMdamKeyPtrOld = NULL;
  MdamKey *mdamKeyPtrOld =
    new HEAP MdamKey(mdamKeyPtr->getKeyColumns(),
		     mdamKeyPtr->getOperatorInputs(),
		     mdamKeyPtr->getDisjuncts(),
		     mdamKeyPtr->getNonKeyColumnSet(),
		     mdamKeyPtr->getIndexDesc());

  SET_MDAM_TRACE_HEADER("[Old Mdam Costing] ");
  DECLARE_MDAM_MONITOR(oldMdamMon);
  ENTER_MDAM_MONITOR(oldMdamMon);
  Cost *costOld = oldComputeCostForMultipleSubset(mdamKeyPtrOld,
  					   costBoundPtr,
					   mdamForced,
					   numKBytesOld,
					   checkExePreds,
					   mdamTypeIsCommon,
					   sharedMdamKeyPtrOld);
  EXIT_MDAM_MONITOR(oldMdamMon);
  PRINT_MDAM_MONITOR(oldMdamMon, "Old MDAM Costing Time: ");

  CostScalar numKBytesNew;
  MdamKey *sharedMdamKeyPtrNew = NULL;
  MdamKey *mdamKeyPtrNew =
    new HEAP MdamKey(mdamKeyPtr->getKeyColumns(),
		     mdamKeyPtr->getOperatorInputs(),
		     mdamKeyPtr->getDisjuncts(),
		     mdamKeyPtr->getNonKeyColumnSet(),
		     mdamKeyPtr->getIndexDesc());

  SET_MDAM_TRACE_HEADER("[New Mdam Costing] ");
  DECLARE_MDAM_MONITOR(newMdamMon);
  ENTER_MDAM_MONITOR(newMdamMon);
  Cost *costNew = newComputeCostForMultipleSubset(mdamKeyPtrNew,
				           costBoundPtr,
					   mdamForced,
					   numKBytesNew,
					   exePreds,
					   checkExePreds,
					   mdamTypeIsCommon,
					   sharedMdamKeyPtrNew);
  EXIT_MDAM_MONITOR(newMdamMon);
  PRINT_MDAM_MONITOR(newMdamMon, "New MDAM Costing Time: ");

  SET_MDAM_TRACE_HEADER(NULL);

  if(costOld && costNew)
  {
    COMPARE_RESULT result = costOld->compareCosts(*costNew);
    if(result != SAME)
    {
      MDAM_DEBUG0(MTL1, "Different MDAM Cost Results");
      MDAM_DEBUGX(MTL1, MdamTrace::printCostObject(this, costOld, "Old Cost"));
      MDAM_DEBUGX(MTL1, MdamTrace::printCostObject(this, costNew, "New Cost"));
    }
    else {
      MDAM_DEBUG0(MTL1, "Same MDAM Cost Results");
      MDAM_DEBUGX(MTL1,
		  MdamTrace::printCostObject(this, costOld, "Old/New Cost"));
    }
  }
  else if((costOld && !costNew) || (!costOld && costNew))
  {
      MDAM_DEBUG0(MTL1, "Different MDAM Cost Results");
      MDAM_DEBUGX(MTL1, MdamTrace::printCostObject(this, costOld, "Old Cost"));
      MDAM_DEBUGX(MTL1, MdamTrace::printCostObject(this, costNew, "New Cost"));
  }

  if(numKBytesNew != numKBytesOld){
      MDAM_DEBUG0(MTL1, "Different MDAM numKBytes value");
      MDAM_DEBUG1(MTL1, "Old Value is %f", numKBytesOld.getValue());
      MDAM_DEBUG1(MTL1, "New Value is %f", numKBytesNew.getValue());
  }

  CURRSTMT_OPTDEFAULTS->setReuseBasicCost(reUseBasicCost);
  MdamTrace::setLevel(origLevel);
}
#endif

NABoolean FileScanOptimizer::isMDAMFeasibleForHBase(const IndexDesc* idesc, ValueIdSet& preds)
{
   Lng32 threshold = (Lng32)(ActiveSchemaDB()->getDefaults()).
                              getAsLong(MDAM_NO_STATS_POSITIONS_THRESHOLD);

   if ( preds.isEmpty() || threshold == 0 )
     return FALSE;

   const ColStatDescList& csdl = 
          idesc->getPrimaryTableDesc()->getTableColStats();

   // If any key column has real stats, then go with the normal mdamp code path
   for (CollIndex k=0; k<idesc->getIndexKey().entries(); k++) {

      ColStatsSharedPtr colStat = 
           csdl.getColStatsPtrForColumn((idesc->getIndexKey()[k]));

      if ( colStat && colStat->isFakeHistogram() == FALSE )
         return FALSE;
   }


   ARRAY(Int32) uecsByKeyColumns(HEAP);
   ARRAY(Int32) rangesByKeyColumns(HEAP);
 
   NABoolean possiblyUseMdam = FALSE;

   for (ValueId e=preds.init(); preds.next(e); preds.advance(e)) 
   {
      if (e.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC)
      {
        RangeSpecRef*     rangeIE  = (RangeSpecRef *) e.getItemExpr();
        OptNormRangeSpec* destObj  = rangeIE->getRangeObject();
        ItemExpr*         rangeCol = destObj->getRangeExpr();

        // check whether this RangeSpecRef is on one of the key columns
        for (CollIndex k=0; k<idesc->getIndexKey().entries(); k++)
          if (rangeCol->containsTheGivenValue(idesc->getIndexKey()[k]))
            {
               possiblyUseMdam = TRUE;
               Int32 totalUec = destObj->getTotalDistinctValues(HEAP);

               if ( totalUec == -1 ) {
                  return FALSE; // can not determine #distinct values covered
               }

               // compute and store #uecs
               if ( !uecsByKeyColumns.used(k) || totalUec > uecsByKeyColumns[k] ) 
                  uecsByKeyColumns.insert(k, totalUec);

               // compute and store #ranges
               Int32 ranges = destObj->getTotalRanges();

               if ( !rangesByKeyColumns.used(k) || ranges > rangesByKeyColumns[k] ) 
                  rangesByKeyColumns.insert(k, ranges);

               break;
           }
      }
   }


   if ( possiblyUseMdam && uecsByKeyColumns.used(0) && uecsByKeyColumns[0] > 0 ) {

      // find the last key column with range spec.
      CollIndex last;
      for (last=rangesByKeyColumns.entries()-1; last>=0; last--) {
         if ( rangesByKeyColumns.used(last) )
           break;
      }

      Int32 numMDAMColumns = 1;
      for (CollIndex j=0; j<last; j++) {
         if ( uecsByKeyColumns.used(j) )
           numMDAMColumns *= uecsByKeyColumns[j];
         else
           return FALSE;
      }

      numMDAMColumns *= rangesByKeyColumns[last];

      return ( numMDAMColumns <= threshold );
   }

   return FALSE;
}

Cost*
FileScanOptimizer::computeCostForMultipleSubset
   ( MdamKey* mdamKeyPtr,
     const Cost * costBoundPtr,
     NABoolean mdamForced,
     CostScalar & numKBytes,
     NABoolean checkExePreds,
     NABoolean mdamTypeIsCommon,
     MdamKey *&sharedMdamKeyPtr
   )
{

  ValueIdSet exePreds;
  ValueIdSet selPreds = getRelExpr().getSelectionPred();

  const IndexDesc *indexDesc = getFileScan().getIndexDesc();
  NABoolean isHbaseTable = indexDesc->getPrimaryTableDesc()->getNATable()->isHbaseTable();
  if ( isHbaseTable && isMDAMFeasibleForHBase(indexDesc, selPreds) ) {
     return new (HEAP) Cost();
  }

  if ((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
      (selPreds.entries()))
  {
    ItemExpr *inputItemExprTree = selPreds.rebuildExprTree(ITM_AND,FALSE,FALSE);
    ItemExpr* resultOld = revertBackToOldTree(CmpCommon::statementHeap(), 
					      inputItemExprTree);
    resultOld->convertToValueIdSet(exePreds, NULL, ITM_AND, FALSE);
    doNotReplaceAnItemExpressionForLikePredicates(resultOld,exePreds,resultOld);

    //revertBackToOldTreeUsingValueIdSet(selPreds, exePreds);		
    //ItemExpr* resultOld =  exePreds.rebuildExprTree(ITM_AND,FALSE,FALSE);
    //doNotReplaceAnItemExpressionForLikePredicates(resultOld,exePreds,resultOld);
  }
  else
  {
    exePreds = selPreds;
  }
  // need to change this exePreds= exePreds - disjunct predicates.
  Disjunct disjunct;
  if ( CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
  {
    for (CollIndex i=0; i < mdamKeyPtr->getDisjuncts().entries(); i++)
    {
      mdamKeyPtr->getDisjuncts().get(disjunct,i);
      ValueIdSet vdset = disjunct.getAsValueIdSet();
      ValueIdSet temp;
      for (ValueId predId = vdset.init();
	   vdset.next(predId);
	   vdset.advance(predId) )
      {
	if (predId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC)
	{
	  if (predId.getItemExpr()->child(1)->castToItemExpr()->getOperatorType() == ITM_AND)
	  {
	    predId.getItemExpr()->child(1)->convertToValueIdSet(temp, NULL, ITM_AND, FALSE);
	    exePreds -= temp;
	  }
	  else
	    exePreds -= predId.getItemExpr()->child(1)->castToItemExpr()->getValueId();
      // New method use: exePreds -= predId.getItemExpr()->child(1)->castToItemExpr()->getValueId();
	}
	else
	  exePreds -= predId;
      }// for (1)
    }// for(2)
  }// if

  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    if (CmpCommon::getDefault(MDAM_COSTING_REWRITE) == DF_ON)
      return scmRewrittenComputeCostForMultipleSubset(mdamKeyPtr,
				           costBoundPtr,
					   mdamForced,
					   numKBytes,
					   exePreds,
					   checkExePreds,
					   sharedMdamKeyPtr);
    else
      return scmComputeCostForMultipleSubset(mdamKeyPtr,
				           costBoundPtr,
					   mdamForced,
					   numKBytes,
					   exePreds,
					   checkExePreds,
					   mdamTypeIsCommon,
					   sharedMdamKeyPtr);
  }

#ifndef NDEBUG
  if(getenv("MDAM_TEST"))
  {
    runMdamTests(mdamKeyPtr,
                 costBoundPtr,
                 mdamForced,
		 exePreds,
                 checkExePreds,
                 mdamTypeIsCommon);
  }
#endif

  if(CURRSTMT_OPTDEFAULTS->useNewMdam())
  {
    return newComputeCostForMultipleSubset(mdamKeyPtr,
				           costBoundPtr,
					   mdamForced,
					   numKBytes,
					   exePreds,
					   checkExePreds,
					   mdamTypeIsCommon,
					   sharedMdamKeyPtr);
  }
  else
  {
    return oldComputeCostForMultipleSubset(mdamKeyPtr,
  					   costBoundPtr,
					   mdamForced,
					   numKBytes,
					   checkExePreds,
					   mdamTypeIsCommon,
					   sharedMdamKeyPtr);
  }

}

// -----------------------------------------------------------------------
// Use this routine to compute the cost of a given MdamKey
// INPUT:
//  mdamKeyiPtr: pointer to the MdamKey to cost
//  costBoundPtr: A cost bound. If the cost of this MdamKey ever
//                exceeds this cost bound, then return NULL
//  mdamForced:
//  checkExePred:
//  mdamTypeIsCommon: TRUE if called to cost MDAMCommon,
//                    FALSE if called to cost MDAMDisjuncts
//     the value of mdamTypeIsCommon is used to save if necessary
//    (and share later) the corresponding stopColumn and sparceColumns
//     information about the current mdamKey
//
// OUTPUT:
// Return value:
//  A NON-NULL Cost* indicating the Cost for mdamKey if the cost did not
//    exceed the given cost bound.
//  A NULL Cost* indicating that the Cost for mdamKey exceeded
//    the given cost bound.
// sharedMdamKeyPtr: pointer to mdamKey used or shared,
//    if sharedMdamKeyPtr == &mdamKey then the key should not be deleted
//    even if it lost right now, because we may reuse BasicCost, stopColumn
//    and sparceCoilum information for this mdamKey later
// -----------------------------------------------------------------------

Cost*
FileScanOptimizer::oldComputeCostForMultipleSubset
   ( MdamKey* mdamKeyPtr,
     const Cost * costBoundPtr,
     NABoolean mdamForced,
     CostScalar & numKBytes,
     NABoolean checkExePreds,
     NABoolean mdamTypeIsCommon,
     MdamKey *&sharedMdamKeyPtr
   )
{
  MDAM_DEBUG0(MTL2,"BEGIN MDAM Costing --------");
  MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this, costBoundPtr, "Cost Bound"));
  // This was added as part of the project
  // to reduce compilation time by reusing simple cost vectors.
  NABoolean sharedCostFound = FALSE;
  SimpleCostVector * disjunctsFRPtr;
  SimpleCostVector * disjunctsLRPtr;

  FileScanBasicCost * fileScanBasicCostPtr = shareBasicCost(sharedCostFound);

  if ( mdamTypeIsCommon )
  {
    disjunctsFRPtr = &(fileScanBasicCostPtr->getFRBasicCostMdamCommon());
    disjunctsLRPtr = &(fileScanBasicCostPtr->getLRBasicCostMdamCommon());
    numKBytes = fileScanBasicCostPtr->getMdamCommonNumKBytes();
  }
  else
  {
    disjunctsFRPtr = &(fileScanBasicCostPtr->getFRBasicCostMdamDisjuncts());
    disjunctsLRPtr = &(fileScanBasicCostPtr->getLRBasicCostMdamDisjuncts());
    numKBytes = fileScanBasicCostPtr->getMdamDisjunctsNumKBytes();
  }

  SimpleCostVector & disjunctsFR = *disjunctsFRPtr;
  SimpleCostVector & disjunctsLR = *disjunctsLRPtr;
  sharedMdamKeyPtr = fileScanBasicCostPtr->getMdamKeyPtr(mdamTypeIsCommon);

  if ( sharedCostFound AND
       sharedMdamKeyPtr AND
       disjunctsFRPtr->getCPUTime() > csZero AND
       disjunctsLRPtr->getCPUTime() > csZero
     )
  {
    if ( CURRSTMT_OPTDEFAULTS->reuseBasicCost() )
    {
      MDAM_DEBUG0(MTL2, "Use cached MDAM cost");
      Cost * costPtr = computeCostObject(disjunctsFR, disjunctsLR);
      if ( costBoundPtr != NULL )
      {
        COMPARE_RESULT result =
          costPtr->compareCosts(*costBoundPtr,
                                 getContext().getReqdPhysicalProperty());
        if ( result == MORE OR result == SAME )
	{
          delete costPtr;
	  MDAM_DEBUG0(MTL2, "MDAM Costing returning NULL cost");
	  MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");
          return NULL;
	}
      }
      mdamKeyPtr->reuseMdamKeyInfo(sharedMdamKeyPtr);
      MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this, costPtr,
          "Returning cached MDAM Cost"));
      MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");
      return costPtr;
    }
    else
    {
      disjunctsFR.reset();
      disjunctsLR.reset();
    }
  }


#ifndef NDEBUG
      (*CURRSTMT_OPTGLOBALS->multSubsetCostMonitor).enter();
#endif

  // MDAM only works for key sequenced files.
  DCMPASSERT(getIndexDesc()->getNAFileSet()->isKeySequenced());
  DCMPASSERT(getIndexDesc()->getIndexKey().entries() > 0);

  const ValueIdSet &exePreds = getRelExpr().getSelectionPred();

  // -----------------------------------------------------------------------
  // For every disjunct:
  // Note:
  // $$$ Need to add code to detect disjunct overlapping
  // -----------------------------------------------------------------------
  // create the histograms for this disjunct:
  IndexDescHistograms
    firstColumnHistogram(*getIndexDesc(),CollIndex(1));

  // Declare counters that keep track of counters for the
  // MDAM tree created from all the disjuncts:
  CostScalar
    // rows that all subsets of all disjuncts contain:
    totalRows
    // estimated subsets of contigous data that the MDAM tree created
    // from all disjuncts contains (for all probes):
    ,totalRqsts
    // incoming probes that create an empty mdam tree:
    ,totalFailedProbes
    // probes for data to get the next subset boundary:
    ,totalProbesForSubsetBoundaries
    ,totalSeeks // total number of disk arm movements
    ,totalDisjunctPreds=0 // the sum of predicates in all disjuncts
    ,totalSeq_kb_read; // total number of blocks that were read sequentially

  CostScalar seqKBytesPerScan;

  ValueIdSet totalKeyPreds;
  CollIndex leadingPartPreds = 0;


  // -----------------------------------------------------------------------
  //  The probes are the TOTAL probes coming from the scan's parent. But
  //  depending on the key predicates and the partitioning key, the scan
  //  may be actually receiving fewer probes. The probes that the
  //  scan is actually receiving is given by the repeat count.
  // -----------------------------------------------------------------------
  CostScalar repeatCount = getContext().getPlan()->getPhysicalProperty()->
    getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2() ;

  // Costing works estimating the resources of the whole, unpartitioned
  // table, thus get the total probes by multiplying by the active
  // partitions
  CostScalar
    incomingProbes = repeatCount * getNumActivePartitions();

  DCMPASSERT(repeatCount <= incomingProbes);


  // Is this a Scan on the right-hand side of a Nested Join?
  // Typically, for nested joins, the repeatCount will be greater than
  // one AND the input ColStats will be non-empty.  However, in some
  // cases such as cross-products, the input ColStats are empty.
  // Also, in some cases repeat count is 1, but ColStats is not emtpy.
  // Here we treat this case as a multiprobe.
  //
  const NABoolean isMultipleProbes =
    ( (repeatCount.isGreaterThanOne()) OR
      (getContext().getInputLogProp()->getColStats().entries() > 0) );

  // -------------------------------------------------------------
  // Compute upper bounds:
  // -------------------------------------------------------------

  CostScalar innerRowsUpperBound =
    getRawInnerHistograms().getRowCount().getCeiling();

  const CostScalar estimatedRecordsPerBlock =
    getIndexDesc()->getEstimatedRecordsPerBlock();

  CostScalar innerBlocksUpperBound;
  computeBlocksUpperBound(
       innerBlocksUpperBound /* out*/
       ,innerRowsUpperBound
       ,estimatedRecordsPerBlock);


  // -----------------------------------------------------------
  // scanForcePtr (if exists)
  // Find if the scan is being forced
  // -----------------------------------------------------------
  ScanForceWildCard* scanForcePtr = NULL;
  const ReqdPhysicalProperty* propertyPtr =
      getContext().getReqdPhysicalProperty();
  if ( propertyPtr
       && propertyPtr->getMustMatch()
       && (propertyPtr->getMustMatch()->getOperatorType()
	   == REL_FORCE_ANY_SCAN))
    {
      scanForcePtr =
	(ScanForceWildCard*)propertyPtr->getMustMatch();
    }


  // -----------------------------------------------------------------------
  // Set up outer statistics and adjust incoming probes to one if
  // needed:
  // -----------------------------------------------------------------------
  const Histograms *outerHistogramsPtr = NULL;
  if (isMultipleProbes)
    {
      MDAM_DEBUG0(MTL2, "Mdam scan is multiple probes");
      // The outer histograms will be used if we are under a nested join
      outerHistogramsPtr = new HEAP
	Histograms(getContext().getInputLogProp()->getColStats());
      DCMPASSERT(outerHistogramsPtr != NULL);
    }
  else {
    MDAM_DEBUG0(MTL2, "Mdam scan is a single probe");
    // make sure that probes are at least one, to satisfy
    // Roll-up formulas....
    incomingProbes.minCsOne();

  }
  //the following is the flag identifying if exepreds can be empty or not.

  NABoolean noExePreds = TRUE;
  NABoolean proceedViaCosting = TRUE;
  // -----------------------------------------------------------------------
  //  Loop through every disjunct and:
  //   1.- Find the optimal disjunct prefix for the disjunct
  //   2.- If the optimal disjunct prefix exceeds the cost bound
  //       return with NULL (signaling that MDAM is too expensive)
  //   3.- Keep track of the sum of the costs of all disjuncts
  // -----------------------------------------------------------------------
  for (CollIndex disjunctIndex=0;
       disjunctIndex < mdamKeyPtr->getKeyDisjunctEntries();
       disjunctIndex++)
    {
      // -------------------------------------------------------------
      // current disjunct counters:
      // -------------------------------------------------------------
      CostScalar
        // rows that all subsets of this disjunct contain:
	disjunctRows
        // subsets of contigous data that this disjunct is made up
        // (for a single probe):
	,disjunctSubsets = csOne
        // subsets of contigous data that this disjunct is made up
        // (for *all* probes):
        ,disjunctSubsetsAsSeeks = csOne
	,disjunctRqsts = csOne
        // incoming probes that create an empty mdam tree:
        ,disjunctFailedProbes = csZero
        // probes for data to get the next subset boundary:
        ,disjunctProbesForSubsetBoundaries = csZero
        // total number of seeks for the current disjunct:
	,disjunctSeeks
        // total number of kb transfered from the physical file to the
        // DP2 current disjunct:
	,uniqueProbes=csOne
        ,disjunctSeq_kb_read;

      CostScalar sumOfUecs = csOne;
      CostScalar sumOfUecsSoFar = csOne;
      CostScalar blocksToReadPerUec = csZero;
      // The stop column keeps track of the optimal prefix
      CollIndex stopColumn = 0;
      CostScalar temp_dataRows = csZero;
      CostScalar * dataRows= &temp_dataRows;

      // -------------------------------------------------------------
      // Set up MdamKey dependant info:
      // -------------------------------------------------------------
      // Create empty list of histograms (histograms for every
      // column are added as needed)
      IndexDescHistograms disjunctHistograms(*getIndexDesc(),0);
      //Need to know if we can use multi column uec to better estimate
      //disjunct subsets.
      NABoolean multiColUecInfoAvail =
          disjunctHistograms.isMultiColUecInfoAvail();
      NABoolean temp=FALSE;
      NABoolean * allKeyPredicates=&temp;

      // get the key preds for this disjunct:
      ValueIdSet disjunctKeyPreds;
      mdamKeyPtr->getKeyPredicates(disjunctKeyPreds,
                                   allKeyPredicates,
                                   disjunctIndex);
      //keeps a eye on the possibility of having no executorpredicates
      if(NOT (*allKeyPredicates))
	noExePreds = FALSE;

      // return with a NULL cost if there are no key predicates
      // and we are not forcing MDAM (a null costBound is only
      // given when MDAM is being forced):
      if (disjunctKeyPreds.isEmpty() AND costBoundPtr != NULL)
      {
	MDAM_DEBUG1(MTL2, "disjunct[%d] without any key predicate,"
		    "MDAM is worthless, returning NULL!\n", disjunctIndex);
        MDAM_DEBUGX(MTL2, getDisjuncts().print());

#ifndef NDEBUG
        (*CURRSTMT_OPTGLOBALS->multSubsetCostMonitor).exit();
#endif
	MDAM_DEBUG0(MTL2, "MDAM Costing returning NULL cost");
	MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");

        return NULL; // full table scan, MDAM is worthless here
      }

      // Tabulate the key predicates using the key columns as
      // the index:
      ColumnOrderList keyPredsByCol(getIndexDesc()->getIndexKey());
      mdamKeyPtr->getKeyPredicatesByColumn(keyPredsByCol,disjunctIndex);

      MDAM_DEBUG1(MTL2, "Disjunct: %d, keyPredsByCol:", disjunctIndex);
      MDAM_DEBUGX(MTL2, keyPredsByCol.print());

      // At this point, the disjunct MUST contain key preds:
      // DCMPASSERT(keyPredsByCol.containsPredicates());

      // -------------------------------------------------------------------
      // If we are receiving probes, find out how many of the probes
      // will generate a non-empty MDAM-tree:
      // --------------------------------------------------------------------

      if (isMultipleProbes)
	{
	  // -------------------------------------------------------------
	  // we are receiving probes, compute which are
	  // successful...
          // We assume each disjunct is independant and that the
          // probes being received apply to the current disjunct
          // only. In reality, the probes being received apply
          // to the whole MDAM tree built from all the disjuncts,
          // because, in general, the disjuncts are overlapping.
          //
          // In the executor, a new MDAM tree is built for every
          // probe. Thus a failed probe in MDAM really means a probe
          // that produces a tree with a single, empty, interval.
          // Consider the case:
          // select * from t1,t2 where t2.a=t1.a OR t2.a=t1.b;
          // and we are evaluting the cost of the inner table of
          // the NJoin:
          //               NJ
          //              /  \
          //            t1   t2
          //
          // t1 has a primary key on (a,b) and t2  has a primary key on a.
          // The tables are like:
          // t1({a,b}) = {(1,3),(5,11),(10,11)}
          // t2({a}) = {1,2,3,4,6,7,8,9,10,12,13,14,15}
          // Thus, t2 is receiving 3 probes: {(1,1),(5,5),(10,10)}
          // For probe 1, the mdam tree built for t2 is
          // made up of the intervals (for a): [1,1],[3,3]
          // for probe 2: EMPTY
          // for probe 3: [10,10]
          // Thus, probes 1 and 3 are "successful" and probe 2 is a
          // a "failed" probe. Note that a "failed" probe in MDAM
          // has zero cost (other than the overhead in finding out
          // that the tree is empty) since empty intervals *do not*
          // generate data lookups.
          //
          // To get some estimate on the "successful probes", we assume
          // that each disjunct gives raise to an independant MDAM tree.
          // In other words, we assume that every disjunct's data
          // does not overlap with each other.
	  // -------------------------------------------------------------

          // --------------------------------------------------------------
          // Compute the disjunct key predicates:
          // --------------------------------------------------------------
	  MDAM_DEBUG0(MTL2, "disjunctKeyPreds before recomputing");
	  MDAM_DEBUGX(MTL2, disjunctKeyPreds.display());
          disjunctKeyPreds.clear();
          keyPredsByCol.getAllPredicates(disjunctKeyPreds);
	  MDAM_DEBUG0(MTL2, "disjunctKeyPreds after recomputing");
	  MDAM_DEBUGX(MTL2, disjunctKeyPreds.display());
          // there must be preds in order to consider MDAM:
          DCMPASSERT( disjunctKeyPreds.entries() > 0 );
          CostScalar
            disSuccProbes
	    ,disUniSucPrbs /* dummy */
	    ,disDupSucPrbs /* dummy */
	    ,disFldPrbs /* dummy */
	    ,disUniFailedProbes;

      	  categorizeProbes(disSuccProbes /* out */
			   ,disUniSucPrbs /* out, but don't care */
			   ,disDupSucPrbs /* out, but don't care */
			   ,disFldPrbs /* out, but don't care */
			   ,disUniFailedProbes
			   ,incomingProbes
			   ,disjunctKeyPreds
			   ,*outerHistogramsPtr
                           ,TRUE // this is MDAM!
                           ,dataRows
			   );

	  MDAM_DEBUG1(MTL2, "Incoming Probes %f", incomingProbes.value());
	  MDAM_DEBUGX(MTL2, outerHistogramsPtr->print());
	  MDAM_DEBUGX(MTL2, disjunctKeyPreds.display());
	  MDAM_DEBUG1(MTL2, "categorizeProbes returns rows: %f", dataRows->value());
	  uniqueProbes = disUniSucPrbs + disUniFailedProbes;
          disjunctFailedProbes = incomingProbes - disSuccProbes;
          DCMPASSERT(disjunctFailedProbes >= csZero);

	} // if we are receiving probes


      // ---------------------------------------------------------
      // We need to compute the
      // prefix of this disjunct such that its cost is minimum
      // and set the stop column accordingly.
      //
      // Will will find that
      // prefix of the list of key columns such that
      // the subsets produced by the predicate expression that is formed by
      // predicates referring to columns in that prefix and
      // the histogram data for their columns
      // is of the least cost.
      //
      // We do this by advancing
      // the column position, recomputing the prefix cost,
      // and keeping track of the minimum prefix.
      // ---------------------------------------------------------

      // -------------------------------------------------------
      // Declare data needed to keep track of the minimum prefix:
      // -------------------------------------------------------
      Cost *minimumPrefixCostPtr = NULL;

      CostScalar
        minimumRows = csZero
        ,minimumRqsts
        ,minimumFailedProbes
        ,minimumProbesForSubsetBoundaries
        ,minimumSeeks = csZero
        ,minimumSeq_kb_read = csZero;

      ValueIdSet minimumKeyPreds;

      SimpleCostVector
        prefixFR
        ,prefixLR
        ;

      const ValueIdSet *predsPtr = NULL;
      Cost *prefixBoundPtr = NULL;
      ValueIdSet singleSubsetPrefixPredicates;

      // -------------------------------------------------------
      // Advance position by position and keep track of
      // minimum prefix:
      // -------------------------------------------------------


      // First find the last column position:
      // The order (i.e., column position) must be varied
      // up to the last column position that references
      // key predicates. Find the lastColumnPosition:
      // walk from the last key column until you find
      // a column position that references a key pred:
      ValueId keyCol;
      NABoolean duplicateFound=FALSE;
      const ItemExpr* predIEPtr = NULL;
      NABoolean foundLastColumn = FALSE;
      CollIndex lastColumnPosition = keyPredsByCol.entries() - CollIndex(1);
      for (;
           lastColumnPosition >= 0;
           lastColumnPosition--)
        {

          // don't bother if the key contains only one column:
          if (lastColumnPosition > CollIndex(0))
            {
              keyCol = getIndexDesc()->getIndexKey()[lastColumnPosition];
	      //following is guard against the situation where we can have
	      //key columns(a,b,a) and predicate a=3 we would chose second
	      //'a' as last coulumn whereas we know that we would never skip
	      //the first 'a' and apply the predicate to the second 'a'
	      for( CollIndex otherColumns=lastColumnPosition-CollIndex(1);
			(otherColumns+1)>=1;otherColumns--)
		{
		if(keyCol==getIndexDesc()->getIndexKey()[otherColumns])
		{
			duplicateFound=TRUE;
			break;
		}
					}
		if(duplicateFound)
		{
			duplicateFound=FALSE;
			continue;
		}
		// The predsPtr may be NULL, and keyPredsBy Col has already
		// sorted the preds by columns, no need to check again
		// through predIEPtr, this is fixed in the new code.
              predsPtr = keyPredsByCol[lastColumnPosition];
              // any predicate in the set may refer to the key column:
              for (ValueId predId = predsPtr->init();
                   predsPtr->next(predId);
                   predsPtr->advance(predId))
                {
                  predIEPtr = predId.getItemExpr();
                  if (predReferencesColumn(predIEPtr, keyCol))
                    {
                      foundLastColumn = TRUE;
                      break;
                    }
                }
            }
          else
            {
              // We've reached the first column, it MUST reference a
              // predicate:
              foundLastColumn = TRUE;
            }
          if (foundLastColumn)
            {
              break;
            }
        } // while we haven't found the lastColumnPosition
      predsPtr = NULL;
      // make column position start from one:
      lastColumnPosition++;
      DCMPASSERT(foundLastColumn);
      DCMPASSERT(lastColumnPosition > 0);


      // Now compute the optimum prefix:

      NABoolean firstRound = TRUE;
      // save the row count of the first column of the
      // previous (or the first) disjunct so that we
      // can correctly compute the sparse positionings
      // for the first column:
      CostScalar firstColRowCntAfter;
      CostScalar firstColRowCntBefore;
      // use this variable to test for overlapping disjunct:
      NABoolean firstColOverlaps = FALSE;

      // The disjunct key preds will actually represent the
      // prefix key preds:
      disjunctKeyPreds.clear();

      CostScalar uecForPreviousCol = csOne;
      CostScalar uecForPrevColForSeeks = csOne;
      CostScalar uecForPreviousColBeforeAppPreds = csOne;
      CostScalar orderRowCount;
      NABoolean crossProductApplied = FALSE;
      CollIndex leadingPartPreds = 0;
      CollIndex minLeadingPartPreds = 2000;  // Can not be 2000 - used to get min



      const ColStatDescList &primaryTableCSDL =
        getIndexDesc()->getPrimaryTableDesc()->getTableColStats();
      const ValueIdList &keyColumns = getIndexDesc()->getIndexKey();

      for (CollIndex colNum=0;colNum < keyColumns.entries();colNum++)
      {
        CollIndex indexInList;
        primaryTableCSDL.
          getColStatDescIndexForColumn(indexInList, keyColumns[colNum]);
        sumOfUecs = sumOfUecs + primaryTableCSDL[indexInList]->getColStats()->
                                                    getTotalUec().getCeiling();
      }
      //Following two variable are used to keep track of the predicate on the
      //current column to be used by the next column. It is reasonable to use
      //MDAM on the later column if the current column has equality predicate
      //on it. Without special code it does not work because the cost of pred
      //application out does the reduction in row send to executor in dp2
      NABoolean prevPredIsEqual= FALSE;
      NABoolean curPredIsEqual= FALSE;
      NABoolean prevColChosen= FALSE;
      CollIndex previousColumn=0;
      for (CollIndex prefixColumnPosition = 0;
           prefixColumnPosition <  lastColumnPosition;
           prefixColumnPosition++)
        {

          // Because of a VEG predicate can contain more than one
          // predicate encoded, add histograms incrementally so that
          // the reduction of a VEG predicate for a later column
          // than the current column position does not affect the
          // distribution of the current column.
          // Note that the append method receives a one-based column position,
          // so add one to the prefix because the prefix is zero based:
          disjunctHistograms.
            appendHistogramForColumnPosition(prefixColumnPosition+1);

          // The very first time,
          // Apply preds to firs column histograms
          // (in order to detect disjunt overlapping) and
          // if there is a single
          // subset prefix, advance the counter to its last
          // column position and gather its predicates.
          if (prefixColumnPosition == 0 )
            {
              // update first col. hist:
              DCMPASSERT(prefixColumnPosition==0);
              firstColRowCntBefore =
                firstColumnHistogram.getColStatsForColumn(
                     getIndexDesc()->getIndexKey()[0]).
                getRowcount().getCeiling();
              predsPtr = keyPredsByCol[0];
	      const SelectivityHint * selHint = getIndexDesc()->getPrimaryTableDesc()->getSelectivityHint();
	      const CardinalityHint * cardHint = getIndexDesc()->getPrimaryTableDesc()->getCardinalityHint();

              if (predsPtr AND NOT predsPtr->isEmpty())
                {
                  firstColumnHistogram.
                    applyPredicates(*predsPtr, getRelExpr(), selHint, cardHint, REL_SCAN);

                  ValueId predId;
                  BiRelat * predIEptr;
                  for(predId=predsPtr->init();
                      predsPtr->next(predId);
                      predsPtr->advance(predId))
                     {
                       if (predId.getItemExpr()->getArity() == 2)
                         {
                           predIEptr=(BiRelat *)predId.getItemExpr();
                           if (predIEptr->isaPartKeyPred())
                             {
                               leadingPartPreds += 1;
                               break;
                             }
                         }
                     }

                }
              firstColRowCntAfter =
                firstColumnHistogram.getColStatsForColumn(
                     getIndexDesc()->getIndexKey()[0]).
                getRowcount().getCeiling();

              // compute the order of the single subset prefix:
              // This order is the column position for that
              // column that denotes a single subset
              // i.e., consider a table t1(A,B,C,D)
              // with pk (A,B,C)
              // for A=1, the order is 0
              // for A=1 AND B=2 AND C=3, the order is 2.
              // itIsSingleSubset is FALSE only for the case
              // that there is a IN pred in the first column.
              CollIndex singleSubsetPrefixColumn;
              NABoolean itIsSingleSubset =
                keyPredsByCol.
                getSingleSubsetOrder(singleSubsetPrefixColumn);

              // Apply single subset preds to histograms and compute
              // statistics:
              if (itIsSingleSubset)
                {
                  for (CollIndex singleSubsetColPosition=0;
                       singleSubsetColPosition <= singleSubsetPrefixColumn;
                       singleSubsetColPosition++)
                    {
                      // Obtain the predicates in i-th order and insert
                      // them into the sing. subset preds:
                      predsPtr = keyPredsByCol[singleSubsetColPosition];

                      if (predsPtr AND NOT predsPtr->isEmpty())
                        {
                          singleSubsetPrefixPredicates.insert(*predsPtr);
                        }
                    } // for every key col in the sing. subset prefix


                  // Set the preds ptr to all preds. in the
                  // prefix:
                  predsPtr = &singleSubsetPrefixPredicates;

                  // Advance the column counter to the
                  // last column of the prefix:
                  prefixColumnPosition = singleSubsetPrefixColumn;
                  for (CollIndex i=1; i <= prefixColumnPosition; i++)
                    {
                      disjunctHistograms.appendHistogramForColumnPosition(i+1);
                    }

                  // A single subset prefix has only
                  // one positioning by definition:
                  disjunctSubsets = csOne;


                  // These are the subsets that each probe is going
                  // to read
                  // (the subsets cannot be higher that the number
                  //  of rows in the raw table):
                  disjunctSubsets = MINOF(disjunctSubsets.getValue(),
                                          innerRowsUpperBound.getValue());
                  disjunctSubsetsAsSeeks = disjunctSubsets;

                  // Since this is a single subset,
                  // no boundary probes
                  // are necessary:
                  disjunctProbesForSubsetBoundaries = csZero;


                } // it is prefixColumnPosition==0 and singlesubsets

              // The uec is used as a multiplier, thus initialize it
              // to one for the column previous to the very first column:
              uecForPreviousCol = csOne;
              uecForPrevColForSeeks = csOne;

            } // if prefix column position is inside a single subset prefix
          else
            { // The prefix column position is in a column after
              // the single subset prefix



              // gather predicates for columns after single subset
              // prefix:
              if (keyPredsByCol[prefixColumnPosition])
                {
                  // the column has predicates,
                  // accumulate them in the pred set:
                  predsPtr = keyPredsByCol[prefixColumnPosition];
                }

              // $$$ if the column is of type IN list, then
              // $$$ the uec should be equal to the IN list entries
              // $$$ times the previous UEC
              // $$$ If the hists can handle IN lists then
              // $$$ the next line is correct.

                uecForPreviousCol = disjunctHistograms.getColStatsForColumn(
                   getIndexDesc()->
                   getIndexKey()[prefixColumnPosition-1]).getTotalUec().
                getCeiling();




                CostScalar estimatedUec = csOne;
                if(multiColUecInfoAvail AND
                    uecForPreviousCol.isGreaterThanOne() AND
		    prefixColumnPosition>1 AND
                  disjunctHistograms.estimateUecUsingMultiColUec(
                                                    keyPredsByCol,/*in*/
                                                    prefixColumnPosition-1,/*in*/
                                                    estimatedUec/*out*/))
                {
                  uecForPreviousCol =
                      (uecForPreviousCol/uecForPreviousColBeforeAppPreds)
                       *estimatedUec;
                  uecForPreviousColBeforeAppPreds = estimatedUec;
                }
                sumOfUecsSoFar = sumOfUecsSoFar + uecForPreviousColBeforeAppPreds;
                uecForPrevColForSeeks = uecForPreviousCol;
                CostScalar uecPerBlock = uecForPreviousColBeforeAppPreds
                                          / blocksToReadPerUec;


                // The following formula was added so that we do not count every
                // subset as a seek. For example let's say that so far we have
                // computed 200 blocks for 20 subsets then so far each subset
                // would access 10 blocks. Now if the next column has 20 uecs
                // a naive algorithm would increase the subset by a factor of
                // 20 and we would have 400 subsets equivalent to 400 seeks.
                // But in reality these 20 uecs are going to create seeks in the
                // 10 blocks so we know that it cannot create more than 10 seeks
                // we further more reduce it by the location of the column and how
                // "together" each of these seeks are.
                if(uecForPrevColForSeeks.isGreaterThanOne() AND
                    NOT uecPerBlock.isLessThanOne() )
                {
                  uecForPrevColForSeeks = MIN_ONE(uecForPrevColForSeeks / uecPerBlock);
                  uecForPrevColForSeeks = MIN_ONE(uecForPrevColForSeeks *
                    MAXOF((sumOfUecs - sumOfUecsSoFar)/sumOfUecs,1/16));
                }
                else if (uecForPrevColForSeeks.isGreaterThanOne())
                {
                  uecForPrevColForSeeks = MIN_ONE(uecForPrevColForSeeks *
                    MAXOF((sumOfUecs - sumOfUecsSoFar)/sumOfUecs,1/16));
                }

            } // prefix column position greater than zero

          // ---------------------------------------------------------
          // The disjunctKeyPreds, at this point, must reflect the
          // keypreds in the current prefix,
          // therefore append this column's key preds:
          // ---------------------------------------------------------
          if (predsPtr)
            {
              DCMPASSERT(predsPtr != NULL);
              disjunctKeyPreds.insert(*predsPtr);


	    }

          // At this point
          // *predsPtr contain the key predicates
          //  for the prefix.

	  MDAM_DEBUG1(MTL2, "Prefix: %d:", prefixColumnPosition);

          NABoolean getRowCount=TRUE;
          // ----------------------------------------------------------------
          // Apply predicates:
          // Apply *predsPtr to the histograms for this disjunct
          // so that we can obtain the positionings and the
          // rows produced by the current disjunct:
          // ----------------------------------------------------------------



          uecForPreviousColBeforeAppPreds =
              disjunctHistograms.getColStatsForColumn(
                getIndexDesc()->getIndexKey()[prefixColumnPosition]).
                getTotalUec().getCeiling();

	  if ( predsPtr AND
               (NOT predsPtr->isEmpty())
              )
            {
	      MDAM_DEBUG0(MTL2, "Applying predicates to disjunct histograms");
	      MDAM_DEBUGX(MTL2, predsPtr->print());

              if (NOT crossProductApplied
                  AND
                  isMultipleProbes)
	      {
		  MDAM_DEBUG0(MTL2, "Applying cross product");

                  // Only apply the cross product once
                  crossProductApplied = TRUE;
		  if(prefixColumnPosition == lastColumnPosition-1)
                  {
                    getRowCount=FALSE;
                    orderRowCount = *dataRows;
		    MDAM_DEBUG1(MTL2, "orderRowCount comes from outerHist: %f",
                        orderRowCount.value());

                  }else
                  {
		    const SelectivityHint * selHint = getIndexDesc()->getPrimaryTableDesc()->getSelectivityHint();
		    const CardinalityHint * cardHint = getIndexDesc()->getPrimaryTableDesc()->getCardinalityHint();

		    disjunctHistograms.applyPredicatesWhenMultipleProbes(
                       *predsPtr
                       ,*(getContext().getInputLogProp())
                       ,getRelExpr().getGroupAttr()->getCharacteristicInputs()
                       ,TRUE // doing MDAM!
		       ,selHint
		       ,cardHint
                       ,NULL
		       ,REL_SCAN);
                  }
                }
              else
                {
		    const SelectivityHint * selHint = getIndexDesc()->getPrimaryTableDesc()->getSelectivityHint();
		    const CardinalityHint * cardHint = getIndexDesc()->getPrimaryTableDesc()->getCardinalityHint();

		    disjunctHistograms.applyPredicates(
                       *predsPtr, getRelExpr(), selHint, cardHint, REL_SCAN
                       );


                }
            }
              prevPredIsEqual = curPredIsEqual;
              curPredIsEqual = FALSE;
              if (keyPredsByCol.getPredicateExpressionPtr(prefixColumnPosition) AND
	         (keyPredsByCol.getPredicateExpressionPtr(prefixColumnPosition)->
			  getType()==KeyColumns::KeyColumn::CONFLICT_EQUALS OR
			  keyPredsByCol.getPredicateExpressionPtr(prefixColumnPosition)->
			  getType()== KeyColumns::KeyColumn::EQUAL))
		  {
                    curPredIsEqual = TRUE;
	            previousColumn=prefixColumnPosition;
		  }


	  if(getRowCount){
            orderRowCount = disjunctHistograms.getRowCount();
	    MDAM_DEBUG1(MTL2, "orderRowCount comes from disjunctHist: %f",
                orderRowCount.value());
          }
          if (prefixColumnPosition == 0)
            {
              // Below, we test a sufficient condition for
              // dijunct overlap.
              //
              // This disjunct overlaps the previous if:
              //
              // The application of preds did not change
              // the rowcount of the first col hist.
              // AND
              // there is a previous disjunct
              // AND
              // the rowcount of the first col hist.
              // is the same as the rowcount of
              // the first col. hist.
              // after first col. preds. were applied.
              // NOTE that we can only detect overlap
              // for the very first column (prefixColumnPosotion == 0))
              firstColOverlaps =
                ( (firstColRowCntBefore == firstColRowCntAfter)
                  AND
                  (disjunctIndex > 0)
                  AND
                  (firstColRowCntAfter == orderRowCount) );

            } // compute overlap flag for first column


          //-----------------------------------------------------
          // Compute density of this column
          //-----------------------------------------------------
          // Sparse dense force flags:
          NABoolean sparseForced = FALSE;
          NABoolean denseForced = FALSE;

          // Check if scan is being forced
          // if so check if density is forced too
          if (scanForcePtr && mdamForced)
            {
              sparseForced =
                ((scanForcePtr->getEnumAlgorithmForColumn(prefixColumnPosition)
                  == ScanForceWildCard::COLUMN_SPARSE)
                 ? TRUE : FALSE);
              denseForced =
                ((scanForcePtr->getEnumAlgorithmForColumn(prefixColumnPosition)
                  == ScanForceWildCard::COLUMN_DENSE)
                 ? TRUE : FALSE);
            }

          if (sparseForced OR denseForced)
            {
              if (sparseForced)
                {
                  mdamKeyPtr->setColumnToSparse(prefixColumnPosition);
                }
              else if (denseForced)
                {
                  mdamKeyPtr->setColumnToDense(prefixColumnPosition);
                }
            }
          else
            {
              // -------------------------------------------------------
              // Sparse/dense was not forced, calculate it from
              // histograms:
              // -------------------------------------------------------

              // With single col. histograms we can only do
              // a good job estimating thisfor the first column...
              if (prefixColumnPosition == 0)
                {
                  const ColStats  &firstColumnColStats =
                    disjunctHistograms.
                    getColStatsForColumn(getIndexDesc()->getIndexKey()[0]);

                  // $$$ We may want to put the threshold in the
                  // $$$ defaults table:
                  const CostScalar DENSE_THRESHOLD = 0.90;
                  const CostScalar density =
                    (firstColumnColStats.getTotalUec().getCeiling()) /
                    ( firstColumnColStats.getMaxValue().getDblValue()
                      - firstColumnColStats.getMinValue().getDblValue()
                      + 1.0 );

                  if ( density > DENSE_THRESHOLD )
                    {
                      // It is dense!!!
                      mdamKeyPtr->setColumnToDense(prefixColumnPosition);
                    }
                  else
                    {
                      // It is sparse!!!
                      mdamKeyPtr->setColumnToSparse(prefixColumnPosition);
                    }
                } // if order == 0
              else
                {
                  // order > 0, always sparse
                  mdamKeyPtr->setColumnToSparse(prefixColumnPosition);
                }
            } // dense/sparse not forced






          // -------------------------------------------------------
          // Update positionings:
          //
          // Assume that the all of the distinct elements for this
          // column exist for every distinct element of
          // the previous column.
          // -------------------------------------------------------

          // Note that there cannot be more positionings and more
          // subsets than there are rows in the table

          // The positionings include probing for the
          // next subset
          if (prefixColumnPosition == 0)
            {
              disjunctSubsets = uecForPreviousCol;
              disjunctSubsets =
                MINOF(innerRowsUpperBound.getValue(),
                      disjunctSubsets.getValue());
              disjunctSubsetsAsSeeks = disjunctSubsets;

              disjunctProbesForSubsetBoundaries = csZero;
            }
          else // prefixColumnPosition > 0
            {
              // Do not add subsets for the first column
              // (i.e. we are in order 1) if we already added them
              // in a previous subset:
              if ( (prefixColumnPosition == 1 AND NOT (firstColOverlaps) )
                   OR
                   prefixColumnPosition > 1)
                {
                  // the UEC for the previous column
                  // may be zero (if the table was empty
                  // or all the rows are eliminated after
                  // preds were applied.) In this
                  // case, don't multiply:
                  if (uecForPreviousCol.isGreaterThanZero())
                    {
                      disjunctSubsets *= uecForPreviousCol;
                      disjunctSubsets =
                        MINOF(innerRowsUpperBound.getValue(),
                              disjunctSubsets.getValue());
                    }
                  if( uecForPrevColForSeeks.isGreaterThanZero())
                  {
                    disjunctSubsetsAsSeeks *= uecForPrevColForSeeks;
                    disjunctSubsetsAsSeeks =
                       MINOF(innerRowsUpperBound.getValue(),
                             disjunctSubsetsAsSeeks .getValue());
                  }

                  // If the previous column is sparse, then for each subset
                  // we need to make an extra probe to find the subset
                  // boundary IF we are not in the second column
                  // and the first column overlaps:
                  if  (mdamKeyPtr->isColumnSparse(prefixColumnPosition-1))
                  {
                    if ( NOT disjunctProbesForSubsetBoundaries.isGreaterThanZero() )
                    {
                      disjunctProbesForSubsetBoundaries = csOne;
                    }
                    disjunctProbesForSubsetBoundaries *= uecForPreviousCol;
                    disjunctProbesForSubsetBoundaries =
                        MINOF(innerRowsUpperBound.getValue(),
                              disjunctProbesForSubsetBoundaries.getValue());
                  }

                } // non-overlapping

            } // order > 0

          // -------------------------------------------------------------
          // Update statistics:
          // --------------------------------------------------------------

          // the subsets requests are issued for every probe:
          CostScalar effectiveProbes = incomingProbes - disjunctFailedProbes;
          disjunctRqsts =  disjunctSubsets * effectiveProbes;

          // the disjunct rows are those rows from the
          // inner table that are kept after the
          // application of predicates.
          // We need the rows for *all* probes,
          // thus we should not divide over the number of
          // probes:
          disjunctRows = orderRowCount;

          // --------------------------------------------------------------
          // A successful request is equivalent to a successful probe
          // in the SearchKey case (single subset), thus we need
          // the rows per successful request, which we compute
          // below.
          // --------------------------------------------------------------

          CostScalar subsetsPerBlock = csOne;
          CostScalar rowsPerSubset = csZero;

          if( NOT disjunctRqsts.isLessThanOne() AND
                disjunctRows.isGreaterThanZero() )
          {
             rowsPerSubset = disjunctRows / disjunctRqsts;
             subsetsPerBlock = estimatedRecordsPerBlock / rowsPerSubset;
          } // if condition is FALSE we don't change these values

          // ------------------------------------------------------------
          // Compute seq_kb_read and seeks for this disjunct
          //
          // The i/o is computed in a per-probe basis (thus the
          // use of disjunctSubsets instead of disjunctRequests).
          // It will be scaled up to number of probes below.
          //
          // -------------------------------------------------------------

          // Get the blocks read per probe:
          CostScalar disjunctBlocksToRead;
          computeTotalBlocksLowerBound(
               disjunctBlocksToRead
               ,disjunctSubsetsAsSeeks
               ,rowsPerSubset
               ,estimatedRecordsPerBlock
               ,innerBlocksUpperBound);
          blocksToReadPerUec = MIN_ONE(disjunctBlocksToRead / disjunctSubsets);


          CostScalar beginBlocksLowerBound = csZero;
          computeBeginBlocksLowerBound(
               beginBlocksLowerBound
               ,MINOF( (disjunctSubsetsAsSeeks /subsetsPerBlock).getCeiling(),
                        disjunctSubsetsAsSeeks.getValue())
               ,innerBlocksUpperBound);



          // disjunctSubsets + disjunctProbesForSubsetBoundaries are basically the number of probes
          //to all the  appropriate values. Thus can be used to compute the number of index blocks
          //that will be used in for MDAM access.
          CostScalar indexBlocksLowerBound = getIndexDesc()->
                                              getEstimatedIndexBlocksLowerBound(
                                              MINOF(((disjunctSubsets +
                                              disjunctProbesForSubsetBoundaries)
                                              /subsetsPerBlock).getCeiling(),
                                              disjunctSubsets.getValue()));
          // Assume that every disjunct does not overlap
          // with the previous. Since we computed all
          // rows to read (and not a lower bound),
          // the following routine will compute
          // the correct cost, despite its name.
          computeIOForFullCacheBenefit(disjunctSeeks /* out */
                                       ,disjunctSeq_kb_read /* out */
                                       ,beginBlocksLowerBound
                                       ,disjunctBlocksToRead
                                       ,indexBlocksLowerBound);


          // -------------------------------------------------------------
          // The total cost for the disjunct is the cost of all
          // the probes times the cost for this disjunct:
          // -------------------------------------------------------------
          NABoolean changeBack = FALSE;
          if((disjunctSeeks-beginBlocksLowerBound).getValue()>=5)
          {
            changeBack = TRUE;
            disjunctSeeks = disjunctSeeks-5;
            disjunctSeq_kb_read = disjunctSeq_kb_read-CostScalar(5)
                                            *getIndexDesc()->getBlockSizeInKb();
          }

          disjunctSeeks *= effectiveProbes;
          disjunctSeq_kb_read *= effectiveProbes;

          if(changeBack)
          {
            disjunctSeeks = disjunctSeeks+5;
            disjunctSeq_kb_read = disjunctSeq_kb_read+CostScalar(5)
                                            *getIndexDesc()->getBlockSizeInKb();
          }



          //-----------------------------------------------------
          // Update the minimum prefix:
          //-----------------------------------------------------

          // Compute current prefix's costs:
          prefixFR.reset();
          prefixLR.reset();
          // getCeiling()'s for final values are calculated
          // inside computeCostVectors...
#ifndef NDEBUG
	  (*CURRSTMT_OPTGLOBALS->multVectorCostMonitor).enter();
#endif  //NDEBUG

	  MDAM_DEBUG2(MTL2, "Disjunct: %d, Prefix Column: %d",
              disjunctIndex, prefixColumnPosition);
	  MDAM_DEBUG1(MTL2, "Incoming Probes: %f:", incomingProbes.value());
	  MDAM_DEBUG1(MTL2, "Prefix Failed Probes: %f:",
              disjunctFailedProbes.value());
	  MDAM_DEBUG1(MTL2, "Prefix Subsets: %f:", disjunctSubsets.value());
	  MDAM_DEBUG1(MTL2, "Prefix Requests (probes * Subsets): %f:",
              disjunctRqsts.value());
	  MDAM_DEBUG1(MTL2, "Prefix Rows: %f:", disjunctRows.value());
	  MDAM_DEBUG1(MTL2, "Prefix Seeks %f:", disjunctSeeks.value());
	  MDAM_DEBUG1(MTL2, "Prefix KB Read: %f:", disjunctSeq_kb_read.value());

          computeCostVectorsForMultipleSubset(
              prefixFR
             ,prefixLR
	     ,seqKBytesPerScan
             ,disjunctRows
             ,disjunctRqsts + disjunctProbesForSubsetBoundaries
             ,disjunctFailedProbes
             ,disjunctSeeks
             ,disjunctSeq_kb_read
             ,disjunctKeyPreds
             ,exePreds
             ,incomingProbes
             ,CostScalar(disjunctKeyPreds.entries())
             );

#ifndef NDEBUG
          (*CURRSTMT_OPTGLOBALS->multVectorCostMonitor).exit();
#endif  //NDEBUG

          // Does the prefix exceeds the minimum prefix cost:
          // If MDAM is forced then the user can specify
          // up to which key column predicates will be included
          // in the mdam key. If the user does not specify the
          // column and MDAM is forced, all of the key predicates
          // in the disjunct are included in the mdam key.

          NABoolean minimumExceedsFrAndLrCosts = FALSE;


#ifndef NDEBUG
	  // -------------------------------------------------------
	  // Back door to force mdam for QA tests:
	  // -------------------------------------------------------
	  char *cstrMDAMStatus = getenv("MDAM");
	  if ((cstrMDAMStatus != NULL) &&
	      ( strcmp(cstrMDAMStatus,"ON")==0 ))
	    {
	      minimumExceedsFrAndLrCosts = TRUE;
	      proceedViaCosting = FALSE;
	    }
	  else
	    {
#endif
	      if (scanForcePtr && mdamForced)
		{
		  proceedViaCosting = FALSE;
		  if(noExePreds AND scanForcePtr
                      ->getNumMdamColumns()<lastColumnPosition)
                     noExePreds=FALSE;
		  if (prefixColumnPosition < scanForcePtr->getNumMdamColumns())
		    {
		      // The user wants this column as part
		      // of the mdam key,
		      // simulate that the cost is exceeded
		      // so that it is included:
		      minimumExceedsFrAndLrCosts = TRUE;

		    }
		  else
		    {
		      if (scanForcePtr->getMdamColumnsStatus()
			  == ScanForceWildCard::MDAM_COLUMNS_ALL)
			{
			  // Unconditionally force all of the columns:
			  minimumExceedsFrAndLrCosts = TRUE;
			  // proceedViaCosting is FALSE
			}
		      else if (scanForcePtr->getMdamColumnsStatus()
			       == ScanForceWildCard::MDAM_COLUMNS_NO_MORE)
			{
			  // Unconditionally reject this and later columns:
			  minimumExceedsFrAndLrCosts = FALSE;
                          curPredIsEqual = FALSE;
                          prevPredIsEqual = FALSE;
			  // proceedViaCosting is FALSE;
			}
		      else if (scanForcePtr->getMdamColumnsStatus()
			       ==
			       ScanForceWildCard::MDAM_COLUMNS_REST_BY_SYSTEM)
			{
			  // Let the system decide based on costing:
			  proceedViaCosting = TRUE;
			}
		    }

		} // if scanForcePtr
#ifndef NDEBUG
	    } // try force statament
#endif


	  if (NOT proceedViaCosting)
	  {
	    MDAM_DEBUG0(MTL2, "Order not considered due to forcing");
	  }

	  MDAM_DEBUGX(MTL2,
	    MdamTrace::printBasicCost(this, prefixFR, prefixLR, "Cost for prefix in oldComputeCostForMultipleSubset():"));


          if (proceedViaCosting)
            {
              // Mdam has not been forced, or forced but with the choice
              // of system decision for MDAM column. proceed with costing:
              minimumExceedsFrAndLrCosts =
                NOT exceedsBound(minimumPrefixCostPtr,
                                 prefixFR
                                 ,prefixLR
                                 );
            }



          // If it is the first time we go around this
          // loop update the minimum.
          // Also updated if the current minimum exceeded
          // the current prefix:
	  // This if is simplifed in the new MDAM costing code
          if (firstRound
              OR minimumExceedsFrAndLrCosts
              OR (prevColChosen
                  AND keyPredsByCol[prefixColumnPosition]
                  AND NOT predsPtr->isEmpty()
                  AND ((prevPredIsEqual AND prefixColumnPosition==previousColumn+1)
                       OR (curPredIsEqual AND prevPredIsEqual))))
            {
	      // The if below seems redundant, removed in the new MDAM costing code
	      if(prefixColumnPosition>previousColumn)prevPredIsEqual=FALSE;
              // We found a new minimum, initialize its data:

              firstRound = FALSE;
              minimumRows = disjunctRows;
              // the successful probes must be multiplied by
              // the positions since every probe must be charged
              // by the disjunct positionings...
              minimumRqsts = disjunctRqsts;
              minimumFailedProbes = disjunctFailedProbes;
              minimumProbesForSubsetBoundaries =
                        disjunctProbesForSubsetBoundaries;
              minimumSeeks = disjunctSeeks;
              minimumSeq_kb_read = disjunctSeq_kb_read;
              // The new minimum is always a longer prefix
              // never a shorter one, so minimumKeyPreds
              // always contains the prev. predicates,
              // thus just insert the key preds,
              // dont'clear them:
              minimumKeyPreds.insert(disjunctKeyPreds);

              delete minimumPrefixCostPtr;
              minimumPrefixCostPtr = computeCostObject(prefixFR,prefixLR);

              DCMPASSERT(minimumPrefixCostPtr != NULL);

              stopColumn = prefixColumnPosition;
              prevColChosen = TRUE;

	      MDAM_DEBUG1(MTL2, "Minimum prefix column position: %d",
                  prefixColumnPosition);
	      MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this,
                  minimumPrefixCostPtr, "Minimum so far"));

	      } // update minimum prefix
          else if (proceedViaCosting AND NOT minimumExceedsFrAndLrCosts) {
             if(prefixColumnPosition == lastColumnPosition-1)
             {//we have reached the lastcolumnPosition and its cost
              //was exceeded for the lastcolumn so
               //there will be additional exepreds
               noExePreds = FALSE;
             }
          else  //  This is not the last column and it wasn't chosen
             prevColChosen = FALSE;
          }
       } // for every Column Position after the prefix order (but the last)

      // delete the the minimum prefix cost, is not needed anymore:
      delete minimumPrefixCostPtr;


      // --------------------------------------------------------------
      // Sum up the statistics for all the disjuncts costed so far
      // --------------------------------------------------------------

      totalRows += minimumRows;

      // Note that the total requests will be potentially be
      // as large as # of disjuncts times the number of
      // requests per disjunct
      totalRqsts += minimumRqsts+minimumProbesForSubsetBoundaries;
      totalFailedProbes += minimumFailedProbes;
      totalSeeks += minimumSeeks;

      //  If we have partitioning key predicates then we will be reading different
      //   parts of the partition by different esps.  There will be some seeks going
      //   from one esp reading to another.  Assume that we will at least read ahead
      //   the normal amount (for now reduce possible seeks by 100).
      const CostScalar blockSizeInKb = getIndexDesc()->getBlockSizeInKb();
      NADefaults &defs                 = ActiveSchemaDB()->getDefaults();
      const CostScalar maxDp2ReadInBlocks =
      CostScalar(defs.getAsULong(DP2_MAX_READ_PER_ACCESS_IN_KB))/blockSizeInKb;

      if (leadingPartPreds == mdamKeyPtr->getKeyDisjunctEntries()
           AND (totalRows / estimatedRecordsPerBlock) > maxDp2ReadInBlocks)
        {
          totalSeeks +=
              (totalRows / estimatedRecordsPerBlock)/maxDp2ReadInBlocks/100;
        }

      totalSeq_kb_read += minimumSeq_kb_read;

      totalKeyPreds.insert(minimumKeyPreds);

      totalDisjunctPreds += minimumKeyPreds.entries();

      // Set the stop column for current disjunct:
      mdamKeyPtr->setStopColumn(disjunctIndex, stopColumn);



      // Compute the cost of all the disjuncts analyzed so far:
      disjunctsFR.reset();
      disjunctsLR.reset();
      // Since we saved this mdamKey information - don't delete it
      // in FileScanOptimizer::optimize(), return this key for checking
      sharedMdamKeyPtr = mdamKeyPtr;
      fileScanBasicCostPtr->setMdamKeyPtr(mdamKeyPtr,mdamTypeIsCommon);

      // Is this restricted to the first column and it either has no predicate
      //   or is a single disjunct - not using mdam  OR
      // Has the input cost bound been exceeded by the disjuncts costed
      // so far?

      // Detect if MDAM make sense

      NABoolean mdamMakeSense = TRUE;

      if (NOT mdamForced  AND
          stopColumn == 0) {

	if (keyPredsByCol.getPredicateExpressionPtr(0) == NULL )
	  mdamMakeSense = FALSE;
	else if (mdamKeyPtr->getKeyDisjunctEntries() == 1) {
	  // When there is a conflict in single subset, mdam should handle it
	  const ColumnOrderList::KeyColumn* currKeyColumn =
	    keyPredsByCol.getPredicateExpressionPtr(0);

	  NABoolean conflict =
	    (    (currKeyColumn->getType() == KeyColumns::KeyColumn:: CONFLICT)
		 OR
		 (currKeyColumn->getType() ==
		  KeyColumns::KeyColumn::CONFLICT_EQUALS));

	  if( NOT conflict ) // single subset should be chosen.
	    mdamMakeSense = FALSE;

	}
      }

      if ( mdamMakeSense )
      {
#ifndef NDEBUG
      (*CURRSTMT_OPTGLOBALS->multVectorCostMonitor).enter();
#endif  //NDEBUG
        computeCostVectorsForMultipleSubset(disjunctsFR /* out */
                                          ,disjunctsLR /* out */
					  ,seqKBytesPerScan /* out */
                                          ,totalRows
                                          ,totalRqsts
                                          ,totalFailedProbes
                                          ,totalSeeks
                                          ,totalSeq_kb_read
                                          ,totalKeyPreds
                                          ,exePreds
                                          ,incomingProbes
                                          ,totalDisjunctPreds
                                          );
#ifndef NDEBUG
      (*CURRSTMT_OPTGLOBALS->multVectorCostMonitor).exit();
#endif  //NDEBUG
        if ( exceedsBound(costBoundPtr,disjunctsFR,disjunctsLR) )
        {
	   mdamMakeSense = FALSE;
           if ( disjunctIndex+1 <
                mdamKeyPtr->getKeyDisjunctEntries() )
           {
             // If we didn't go through all disjuncts yet -
             // invalidate computed basic cost to prevent
             // using this incomplete cost later.
             disjunctsFR.reset();
             disjunctsLR.reset();
           }
        }
      }
      if ( NOT mdamMakeSense )
        {
#ifndef NDEBUG
	  (*CURRSTMT_OPTGLOBALS->multSubsetCostMonitor).exit();
#endif  //NDEBUG
          // Yes!, no need to continue, MDAM does not make sense
          // for this scan:
	  MDAM_DEBUG0(MTL2, "MDAM Costing returning NULL cost");
	  MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");
          return NULL;
	}

      MDAM_DEBUG1(MTL2, "Stop Column: %d", stopColumn);

    } // for every disjunct

  MDAM_DEBUG1(MTL2, "Total rows for this MDAM scan: %f:", totalRows.value());

#ifndef NDEBUG
  (*CURRSTMT_OPTGLOBALS->multObjectCostMonitor).enter();
  CURRSTMT_OPTGLOBALS->synCheckFlag = TRUE;
#endif  //NDEBUG

  // ---------------------------------------------------------------------
  // Done!, MDAM won! create the cost vector:
  // ---------------------------------------------------------------------

  Cost *costPtr = computeCostObject(disjunctsFR
                                    ,disjunctsLR
                                    );
#ifndef NDEBUG
  CURRSTMT_OPTGLOBALS->synCheckFlag = FALSE;
  (*CURRSTMT_OPTGLOBALS->multObjectCostMonitor).exit();
#endif  //NDEBUG

  //noExePreds is true, great set the flag in MDAM
  if(noExePreds AND checkExePreds)
  {
    mdamKeyPtr->setNoExePred();
  }

#ifndef NDEBUG
  (*CURRSTMT_OPTGLOBALS->multSubsetCostMonitor).exit();
#endif  //NDEBUG

  numKBytes = seqKBytesPerScan; //totalSeq_kb_read;

  if ( checkExePreds )
    fileScanBasicCostPtr->setMdamDisjunctsNumKBytes(numKBytes);
  else
    fileScanBasicCostPtr->setMdamCommonNumKBytes(numKBytes);

  MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this, costPtr,
      "Returning MDAM Cost"));
  MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");
  return costPtr;

} // oldComputeCostForMultipleSubset(...)

// -----------------------------------------------------------------------
// This function will check if current context has the same basic physical
// properties as the one of existing BasicCost objects. First it checks
// the groupId, sinice the goupId defines the set of row produced by the
// group members. This will take into account all predicates pushed to the
// scan. Then it checks the number of incoming probes tthat could be
// different for the same groupId. Finally it checks input logical property
// passed from the left child of nested join to the right one. If both
// pointers currentIPP and existingIPP are NULL it returns TRUE, if
// only one of them is NULL, it returns FALSE. If both are not NULL
// and not equal to eash other, it compares corresponding lists of
// NjOuterOrder columns
// -----------------------------------------------------------------------
NABoolean
FileScanBasicCost::hasSameBasicProperties(const Context & currentContext) const
{
   if ( basicCostContext_->getGroupId() != currentContext.getGroupId() )
     return FALSE;

   if ( (basicCostContext_->getInputLogProp())->getResultCardinality()
         != (currentContext.getInputLogProp())->getResultCardinality() )
     return FALSE;

   if ((basicCostContext_->getInputLogProp()->getColStats().entries() > 0)
       != (currentContext.getInputLogProp()->getColStats().entries() > 0))
     return FALSE;

   if ( (basicCostContext_->getReqdPhysicalProperty())->getOcbEnabledCostingRequirement()
       != (currentContext.getReqdPhysicalProperty())->getOcbEnabledCostingRequirement() )
     return FALSE;

   LogPhysPartitioningFunction *logPhysPartFunc =
    (LogPhysPartitioningFunction *)  // cast away const
    currentContext.getPlan()->getPhysicalProperty()->getPartitioningFunction()->
    castToLogPhysPartitioningFunction();

  if ( logPhysPartFunc != NULL )
  {
    LogPhysPartitioningFunction::logPartType logPartType =
      logPhysPartFunc->getLogPartType();

    if (    logPartType == LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING
	 OR logPartType == LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING
       )
      {
        // If we have LOGICAL_SUBPARTITIONING then extra predicates have
        // been added to selection preds.  We don't want to steal plans
        // without the extra predicates.
        // The problem is to find a plan with the extra predicates -
        // the commented out code did not do this right
        //if (NOT basicCostContext_->getPlan())
         return FALSE;
      }
   }

   const InputPhysicalProperty*
	 currentIPP = currentContext.getInputPhysicalProperty();
   const InputPhysicalProperty*
	 existingIPP = basicCostContext_->getInputPhysicalProperty();

   if ( currentIPP == NULL OR currentIPP->getAssumeSortedForCosting())
   {
     if ( existingIPP == NULL OR existingIPP->getAssumeSortedForCosting())
       return TRUE;
     else
       return FALSE;
   }

   // currentIPP != NULL
   if ( existingIPP == NULL OR existingIPP->getAssumeSortedForCosting())
      return FALSE;

   // currentIPP != NULL AND existingIPP != NULL
   return ( currentIPP == existingIPP OR
     *(currentIPP->getNjOuterOrder()) == *(existingIPP->getNjOuterOrder()) );
}

// -----------------------------------------------------------------------
// Use this routine to compare the current cost with a given bound.
// INPUT:
//    costBoundPtr: the bound to exceed
//    firstRow: A first row simple cost vector
//    lastRow: A last row simple cost vector
// OUTPUT:
//   TRUE if the cost vector created from firstRow and lastRow is of the
//   cost as *costBoundPtr or if it is more expensive than *costBoundPtr
// -----------------------------------------------------------------------

NABoolean
FileScanOptimizer::exceedsBound(const Cost *costBoundPtr,
				const SimpleCostVector& firstRow
				,const SimpleCostVector& lastRow
                                ) const
{


  if (costBoundPtr == NULL)
    {
      return FALSE;
    }
  else
    {

      const Cost *costSoFarPtr = computeCostObject(firstRow, lastRow);

      COMPARE_RESULT result =
	costSoFarPtr->compareCosts(*costBoundPtr,
			       getContext().getReqdPhysicalProperty());

      delete costSoFarPtr;

      // since doing MDAM adds overhead also say that we exceeded
      // a bound when costs match exactly:
      if (result == MORE OR result == SAME)
	{

	  MDAM_DEBUG0(MTL2, "Cost exceeded");

	  return TRUE; // cost bound has been exceeded, return

	}


    } // cost bound comparison

  return FALSE;

} // FileScanOptimizer::exceedsBound(...)

/////////////////////////////////////////////////////////////////////
// fix the estimation
// for seeks beginBlocksLowerBound = no. of unique entries WHICH ARE
// SUCCESSFUL in matching with inner table totalBlocksLowerBound = no.
// of blocks returning from the OUTER TABLE after applying the predicate
// innerBlocksUpperBound = Total size of inner table
// DBB = Distance between the blocks
// MSD = Distance for average seek
// CostScalar(0.025) = (seek time is between .2ms and 8ms ignore latency,
// so fixed is .2ms/8ms=0.025) (0.008) Limiting the rate of increase upto
// twice that of inner blocks and then using a step function
// STEP FUNCTION so as to decrease the rate of increase after twice of
// inner blocks cost for fixed seeks is the, % of probes which can be read
// with minimum seek time finalRows are the final resulting rows, matched
// with outertable and qualified with < predicate To the readAhead cost
// the Mrows * finalRows is added, this is done in order to calculate the
// cost of moving the rows to the executor. Once the rows from the outer
// table come in and they are compared with the innertable rows, the
// successful rows will be returned to the executor, so the
// more the number of rows returned to the executor the more the cost.
////////////////////////////////////////////////////////////////////////

void
FileScanOptimizer::computeSeekForDp2ReadAheadAndProbeOrder(
     CostScalar& seekComputedWithDp2ReadAhead,
     const CostScalar& finalRows,
     const CostScalar& uniqueProbes,
     const CostScalar& beginBlocksLowerBound,
     const CostScalar& totalBlocksLowerBound,
     const CostScalar& innerBlocksUpperBound,
     const CostScalar& dp2CacheSize,
     const NABoolean inOrderProbes
    )const
{
   NADefaults &defs                 = ActiveSchemaDB()->getDefaults();
   const CostScalar blockSizeInKb = getIndexDesc()->getBlockSizeInKb();
   const CostScalar maxDp2ReadInBlocks =
     CostScalar(defs.getAsULong(DP2_MAX_READ_PER_ACCESS_IN_KB))/blockSizeInKb;
   const CostScalar blocksForMoreThanOneSeek =
       defs.getAsLong(DP2_MINIMUM_FILE_SIZE_FOR_SEEK_IN_BLOCKS);


  if(inOrderProbes)
    {
      //uniqueProbes calculated from CategorizeProbes should be atleast one.
      CCMPASSERT(uniqueProbes > csZero);

      const CostScalar DBB=(innerBlocksUpperBound / uniqueProbes);
      const CostScalar MSD = defs.getAsULong(NJ_MAX_SEEK_DISTANCE);
      const CostScalar Ulimit = defs.getAsDouble(NJ_INC_UPTOLIMIT);
      const CostScalar Alimit = defs.getAsDouble(NJ_INC_AFTERLIMIT);
      const CostScalar Mrows  = defs.getAsDouble(NJ_INC_MOVEROWS);

      CostScalar fixedSeeks;
      CostScalar Limit = csTwo * innerBlocksUpperBound;

      if (uniqueProbes > Limit)
	{
	  CostScalar extraProbes = uniqueProbes - Limit;
	  fixedSeeks = Ulimit * Limit + Alimit * extraProbes;
	}
      else
	{
	  fixedSeeks = Ulimit * (uniqueProbes-1);
	}

      CostScalar variableSeeks = uniqueProbes - fixedSeeks;

      seekComputedWithDp2ReadAhead = csOne + fixedSeeks +
          (variableSeeks) * (DBB/MSD)  + Mrows * finalRows;

   }
  else
  {
    if(innerBlocksUpperBound <= dp2CacheSize)
    {
      seekComputedWithDp2ReadAhead =
          MINOF(beginBlocksLowerBound,
                innerBlocksUpperBound/maxDp2ReadInBlocks);
    }
    else
    {
      seekComputedWithDp2ReadAhead = beginBlocksLowerBound;
    }
  }
}

void
FileScanOptimizer::computeIOForFullCacheBenefit(
     CostScalar& seeks /* out */
     ,CostScalar& seq_kb_read /* out */
     ,const CostScalar& beginBlocksLowerBound
     ,const CostScalar& totalBlocksLowerBound
     ,const CostScalar& indexBlocks) const
{

  // -----------------------------------------------------------------------
  // No block will be read more than once.
  // -----------------------------------------------------------------------
  //For the sake of a cleaner interface lets have indexBlocks as a constant
  //reference parameter. But that means we have to load it into a cost scalar
  //that can be updated.

  seq_kb_read =
    (totalBlocksLowerBound
     + indexBlocks) * getIndexDesc()->getBlockSizeInKb();

  // There cannot be more index blocks than the log of the total data blocks:

  seeks =
    beginBlocksLowerBound + indexBlocks;




}

void
FileScanOptimizer::computeIOForRandomCase(
     CostScalar& seeks /* out */
     ,CostScalar& seq_kb_read /* out */
     ,const CostScalar& blksPerSuccProbe
     ,const CostScalar& beginBlocksLowerBound
     ,const CostScalar& totalBlocksLowerBound
     ,const CostScalar& successfulProbes
     ,const CostScalar& failedProbes
     ,const CostScalar& indexBlocksLowerBound) const
{
  const CostScalar cacheInBlocks =
    getDP2CacheSizeInBlocks(getIndexDesc()->getBlockSizeInKb());


 ULng32 ulCache = (ULng32)(cacheInBlocks.getCeiling().getValue());
 ULng32 ulBlks = (ULng32)(blksPerSuccProbe.getValue());
  if (ulBlks == (ULng32)0)
    {
      FSOWARNING("Bad cast in FileScanOptimizer::computeIOForRandomCase(...)");
      ulBlks = UINT_MAX;
    }
  CostScalar extraMemory =  ulCache % ulBlks;

  const CostScalar availableCacheInBlocks =  cacheInBlocks - extraMemory;

  // -----------------------------------------------------------------------
  // If the cache is bigger or equal than the total blocks to be read, then
  // the hit probability is trivially one. Else, it is the fraction
  // of the available cache over the total blocks to be read:
  // -----------------------------------------------------------------------
  CostScalar cacheHitProbability = csOne;
  if (availableCacheInBlocks < totalBlocksLowerBound )
    {
      cacheHitProbability =
        availableCacheInBlocks  / totalBlocksLowerBound;
    }

  // sanity check:
  DCMPASSERT(cacheHitProbability >= 0 AND cacheHitProbability <= 1.0);

  const CostScalar extraBeginBlocks =
    successfulProbes - beginBlocksLowerBound;

  const CostScalar extraDataBlocks =
    successfulProbes * blksPerSuccProbe
    - totalBlocksLowerBound;
  // -----------------------------------------------------------------------
  // Failed probes is considered below because we assume that
  // to know that a probe failed we need to reach to the
  // bottom of the b-tree and grab the first data block.
  // -----------------------------------------------------------------------
  seq_kb_read =
    (
         (totalBlocksLowerBound + indexBlocksLowerBound + failedProbes)
         + (csOne - cacheHitProbability)
         *extraDataBlocks

         )  * getIndexDesc()->getBlockSizeInKb();

  // We assume that once an index block is loaded, it remains
  // in cache:
  seeks =
    beginBlocksLowerBound // data seeks for all probes
    + indexBlocksLowerBound // index seeks
    + failedProbes
    + (csOne - cacheHitProbability) * extraBeginBlocks;

}


void
FileScanOptimizer::computeIOForFullTableScan(
     CostScalar& dataRows /* out */
     ,CostScalar& seeks /* out */
     ,CostScalar& seq_kb_read /* out */
     ,const CostScalar& probes
     ) const
{

  const CostScalar indexBlocksLowerBound = getIndexDesc()->
                                  getEstimatedIndexBlocksLowerBound(probes);
  // Assume that every record in the table will be read:

  dataRows = getRawInnerHistograms().getRowCount();

  const CostScalar blksInTable =
    CostScalar( dataRows
		/ getIndexDesc()->getEstimatedRecordsPerBlock());


  seq_kb_read = (probes * blksInTable) * getIndexDesc()->getBlockSizeInKb();

  seeks =
    probes // data seeks
    +
    indexBlocksLowerBound; // index seeks

} // FileScanOptimizer::computeIOForFullTableScan(...)

void
FileScanOptimizer::computeCostVectorsForMultipleSubset(
     SimpleCostVector& firstRow /* out */
     ,SimpleCostVector& lastRow /* out */
     ,CostScalar& seqKBytesPerScan /* out */
     ,const CostScalar& totalRows
     ,const CostScalar& subsetRequests // a request for a subset of data
     ,const CostScalar& failedProbes // produce and EMPTY mdam tree
     ,const CostScalar& seeks
     ,const CostScalar& seq_kb_read
     ,const ValueIdSet& keyPreds
     ,const ValueIdSet& exePreds
     ,const CostScalar& incomingProbes // probes incoming to the operator
     ,const CostScalar& mdamNetPredCnt // the sum of predicates in all disjuncts
     ) const
{

  // charge the cost of building the mdam network:
  // We assume that the cost of building
  // the  mdam network in the executor is
  // a linear function of the mdam key predicates:

  // The cost it takes to start building the mdam network:
  CostScalar mdamNetCostOvh;

  // The cost that takes to build the mdam network per predicate:
  // (we assume that the cost to build the mdam network is a linear function
  // of the key predicates)
  CostScalar mdamNetCostPerPred;
  
  if ( CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
  {
    mdamNetCostOvh = 0.0;
    mdamNetCostPerPred = 0.0;
  }
  else
  {
    mdamNetCostOvh = CostPrimitives::getBasicCostFactor(MDAM_CPUCOST_NET_OVH);	
    mdamNetCostPerPred = CostPrimitives::getBasicCostFactor(MDAM_CPUCOST_NET_PER_PRED);
  }

  // The cost to start up the mdam network and to finish building it,
  // for one probe:
  CostScalar mdamNetBuildCost =
    mdamNetCostOvh + mdamNetCostPerPred * mdamNetPredCnt;

  // The cost per partition:
  const CostScalar activePartitions =   getNumActivePartitions();

  mdamNetBuildCost = mdamNetBuildCost/activePartitions;


  // The first row must build the net no matter what, but only for the
  // first probe (because we assume that the first probe finds the first
  // row):
  firstRow.addInstrToCPUTime(mdamNetBuildCost);
  // Also add it to the idle time so that it gets reflected
  // on the elapsed time and is a factor in comparison with
  // the single subset case:
   CostScalar mdamNetOverheadTime =
    mdamNetBuildCost
    *
    CostPrimitives::getBasicCostFactor(MSCF_ET_CPU);

  firstRow.addToIdleTime(mdamNetOverheadTime);

  // But the last row builds it for *all* probes:
  // the successful probes pay full price:
  mdamNetBuildCost = mdamNetBuildCost * MIN_ONE(incomingProbes-failedProbes);
  // fail probes only pay initial overhead:
  mdamNetBuildCost += failedProbes * mdamNetCostOvh;
  lastRow.addInstrToCPUTime(mdamNetBuildCost);
  // Add also to idel time so that it makes a difference compared
  // to search key (usually IO dominates over CPU, so if we add
  // only to CPU then it won't make a difference an MDAM may look
  // like a winner when in fact it is a loser)
  mdamNetOverheadTime =
    mdamNetBuildCost
    *
    CostPrimitives::getBasicCostFactor(MSCF_ET_CPU);
  lastRow.addToIdleTime(mdamNetOverheadTime);

  // finish costing:
  computeCostVectors(firstRow // out
                     ,lastRow // out
		     ,seqKBytesPerScan
                     ,totalRows
                     ,subsetRequests
                     ,subsetRequests // all are successful
                     ,seeks
                     ,seq_kb_read
                     ,keyPreds
                     ,exePreds
                     ,incomingProbes
                     );

} // FileScanOptimizer::computeCostVectorsForMultipleSubset(...)


void
FileScanOptimizer::computeCostVectors(
     SimpleCostVector& firstRow /* out */
     ,SimpleCostVector& lastRow /* out */
     ,CostScalar& seqKBytesPerScan /* out */
     ,const CostScalar& totalRows
     ,const CostScalar& subsetRequests // a request for a subset of data
     ,const CostScalar& successfulSubsetRequests // requests that hit data
     ,const CostScalar& seeks
     ,const CostScalar& seq_kb_read
     ,const ValueIdSet& keyPreds
     ,const ValueIdSet& exePreds
     ,const CostScalar& incomingProbes // probes incoming to the operator
     ) const
{
  DCMPASSERT(totalRows >= 0);
  DCMPASSERT(subsetRequests >= 0);
  DCMPASSERT(successfulSubsetRequests <= subsetRequests);

  // -----------------------------------------------------------------------
  // The cost that we need to compute is that of a single scan.
  // However, the data that we are getting corresponds
  // to that for scanning all the data to answer the
  // original query. This method partitions that data so
  // that the cost per scan can be created.
  // It takes into account:
  //  - several scans may be working in the same cpu (cpu contention)
  //  - the scan is being done syncronously or asynchronously
  //  - several CPUS are available for scanning the data (data parallelism)
  //
  // -----------------------------------------------------------------------


  // -----------------------------------------------------------------------
  // To estimate the cost per scan we need to know how many
  // partitions are actually doing work. Then, the cost per
  // scan will be computed in general by dividing the total cost
  // over the active partitions.
  // -----------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------
  //  getEstNumActivePartitionsAtRuntime() will return the estimated number of
  //  active partitions at runtime.This variable is set to the maximum number
  //  of partitions that can be active,if a query has an equality predicate
  //  with a host/parameter variable on leading partition column.This is done
  //  is method OptPysRelExpr::computeDP2CostThatDependsonSPP().
  //  If this is not set, this function will return the active partition
  //  determined by getNumActivePartition() function.
  //  This is currently used only for costing of scans.

  const CostScalar activePartitions = getEstNumActivePartitionsAtRuntime();

  CostScalar totalRowsInResultTable = (CostScalar)getResultSetCardinality();

  // Patch for Sol.10-031024-0755 (Case 10-031024-9406).
  // This change can be made unconditional. TotalRowsInResultTable (which
  // is selected number of rows, so it is a misnomer) cannot be greater
  // than the total number of rows scanned, defined by input dataRows
  if ( CmpCommon::getDefault(COMP_BOOL_35) == DF_OFF )
     totalRowsInResultTable = MINOF(totalRowsInResultTable,totalRows);

  // -----------------------------------------------------------------------
  // Scale down the total factors to single scan:
  // We assume uniform distribution, i.e. all active partitions are
  // doing the same ammount of work.
  // -----------------------------------------------------------------------
  const CostScalar
    rowsPerScan = CostScalar(totalRows/activePartitions).getCeiling()
    ,selectedRowsPerScan =
        CostScalar(totalRowsInResultTable/activePartitions).getCeiling()
    ,requestsPerScan = CostScalar(subsetRequests/activePartitions).getCeiling()
    ,successfulRequestsPerScan =
    CostScalar(successfulSubsetRequests/activePartitions).getCeiling()
    ,seeksPerScan = CostScalar(seeks/activePartitions).getCeiling()
    ,probesPerScan = CostScalar(incomingProbes/activePartitions).getCeiling()
    ;
  CostScalar seq_kb_readPerScan =
    CostScalar(MAXOF(seq_kb_read/activePartitions,4.0));


  // -----------------------------------------------------------------------
  // Set up some parameters in preparation for costing:
  // -----------------------------------------------------------------------

  const CostScalar
    recordsPerBlock =
    getIndexDesc()->getEstimatedRecordsPerBlock().getCeiling();

  const CostScalar
    kbPerBlock = getIndexDesc()->getBlockSizeInKb();


  const CostScalar
    dp2MessageBufferSizeInKb =
    CostPrimitives::getBasicCostFactor(DP2_MESSAGE_BUFFER_SIZE)
    ,recordSizeInKb =  getIndexDesc()->getRecordSizeInKb();

  // rows that fit in dp2 buffer (rfdp2):
  const double
    rfdp2 =
    (dp2MessageBufferSizeInKb / recordSizeInKb).getFloor().getValue();

  // Assume that the first row is obtained in the first
  // successful request:
  const CostScalar
    rowsPerScanForFirstRow =
    (successfulRequestsPerScan.isGreaterThanZero() ?
     MINOF(
          (rowsPerScan/successfulRequestsPerScan).getCeiling()
          , rfdp2)
     : csZero);

  const CostScalar
    selectedRowsPerScanForFirstRow =
    (successfulRequestsPerScan.isGreaterThanZero() ?
     MINOF(
          (selectedRowsPerScan/successfulRequestsPerScan).getCeiling()
          , rfdp2)
     : csZero);

  // -----------------------------------------------------------------------
  // Now cost each bucket of the simple cost vector:
  // -----------------------------------------------------------------------


  // -----------------------------------------------------------------------
  // CPU:
  // Instructions to traverse the B-Tree for successful requests
  // +
  // Instructions to traverse the B-Tree for unsuccessful requests
  // -----------------------------------------------------------------------

  CostScalar
    cpuInstructionsFR = csZero
    ,cpuInstructionsLR = csZero;

  if (requestsPerScan.isGreaterThanZero())
    {
      // We do a binary search after we get the data block in memory to find
      // the matching row. So in worst case scenario it would just be
      // ln(recordsPerBlock). log function will give us the natural logarithm.
      // se to get logX = lnX/log2 1.44 * lnX.
      const CostScalar cpuForBTreeComparisons =
        ( 1.44 * log((getIndexDesc()->
           getEstimatedRecordsPerBlock()).getValue())
          * getIndexDesc()->getIndexKey().entries()
          * CostPrimitives:: getBasicCostFactor(CPUCOST_PREDICATE_COMPARISON)
          * getIndexDesc()->getIndexLevels() );

      if (successfulRequestsPerScan.isGreaterThanZero())
        {

          // Successful requests:
          // We assume that the cpu cost for *each* successful request is
          // a constant overhead plus a
          // function of the number of index blocks traversed and the
          // number of key predicates.
          //
          // Number of comparisons that are
          // performed to traverse the non-leaf
          // levels and arrive at the leaf. We assume that the search within
          // the index, for each succ. probe,
          // has to perform key comparisons with half the number
          // of rows on each page (root + non-leaf pages + leaf page that
          // contains the begin key value). We take into account the number
          // of key columns that participate in the comparison.


          cpuInstructionsLR +=           // LR CPU INSTRUCTION CALC:
            successfulRequestsPerScan
            *
            ( cpuForBTreeComparisons
              +
              CostPrimitives::getBasicCostFactor(CPUCOST_DATARQST_OVHD) );
        }

      // unsuccessful requests:
      const CostScalar
        failedRequestsPerScan =
        requestsPerScan - successfulRequestsPerScan;
      if (failedRequestsPerScan.isGreaterThanZero())
        {
          // Assume failed requests traverse half the B-Tree to find
          // out they failed

          cpuInstructionsLR +=           // LR CPU INSTRUCTION CALC:
            failedRequestsPerScan
            *
            ( (cpuForBTreeComparisons/2).getCeiling()
              +
              CostPrimitives::getBasicCostFactor(CPUCOST_DATARQST_OVHD) );

        }

      // assume that the first row gets its data in the successful request:

      // FR CPU INSTRUCTION CALC:
      cpuInstructionsFR +=           // FR CPU INSTRUCTION CALC:
        cpuInstructionsLR/requestsPerScan;





      // ------------------------------------------------------------
      // There is one copy of every row to be made to bring the
      // rows into the exeindp2: (more copies are done to ship
      // to the master, but these copies are costed by the exchange)
      // ------------------------------------------------------------
      const CostScalar numberOfCopiesOfRows = csOne;
      CostScalar instrPerRowMovedOutOfTheCache =
        CostScalar(CostPrimitives
                   ::getBasicCostFactor(CPUCOST_COPY_ROW_OVERHEAD))
        +
        numberOfCopiesOfRows
        *
        recordSizeInKb
        *
        CostScalar(1024.) // convert kb to bytes
        *
        CostScalar(CostPrimitives
                   ::getBasicCostFactor(CPUCOST_COPY_ROW_PER_BYTE))
        ;

      // locking per row:
      // $$ in the future set this var with info obtained from the
      // $$ enviroment (it should have the value 0 or 1):
      const CostScalar locking = csZero;
      instrPerRowMovedOutOfTheCache +=
        locking * CostScalar(CostPrimitives
                              ::getBasicCostFactor(CPUCOST_LOCK_ROW));

      // key encoding/decoding per row:
      // $$$ In the future we need info from the IndexDesc regarding
      // the key length:
      const CostScalar keyLength = csZero;
      instrPerRowMovedOutOfTheCache +=
        keyLength * CostScalar(CostPrimitives
                               ::getBasicCostFactor(CPUCOST_SCAN_KEY_LENGTH));

      // Add a per kb cost for the cost of moving
      // every block from disk to DP2:
      CostScalar instrToCopyDataFromDiskToDP2Cache =
        seq_kb_readPerScan
        *
        CostScalar(CostPrimitives::getBasicCostFactor(
                                           CPUCOST_SCAN_DSK_TO_DP2_PER_KB));
      // Add a per seek cost for the cost of moving
      // data from disk to DP2:
      instrToCopyDataFromDiskToDP2Cache +=
        seeksPerScan
        *
        CostScalar(CostPrimitives::getBasicCostFactor(
                                           CPUCOST_SCAN_DSK_TO_DP2_PER_SEEK));
      // $$$ First row
      // Add this later

      // LR CPU INSTRUCTION CALC:
      cpuInstructionsLR += instrToCopyDataFromDiskToDP2Cache;

      // CPUCOST_SCAN_OVH_PER_KB
      // is a catch all cost *per KB*
      // This catches all overhead that happens when data is moved
      // from the dp2 cache into the exe in dp2.
      instrPerRowMovedOutOfTheCache +=
        recordSizeInKb
        *
        // Catch all overhead per KB:
        // this is a per kb read overhead:
        CostScalar(CostPrimitives::getBasicCostFactor(
                                           CPUCOST_SCAN_OVH_PER_KB))
        ;

      // Add a a per-row overhead:
      instrPerRowMovedOutOfTheCache +=
        CostScalar(CostPrimitives::getBasicCostFactor(
                                           CPUCOST_SCAN_OVH_PER_ROW));

      // Only rows that are selected are moved out of the cache:
      // First row
      cpuInstructionsFR += // FR CPU INSTRUCTION CALC:
        instrPerRowMovedOutOfTheCache
        *
        selectedRowsPerScanForFirstRow;

      // LR CPU INSTRUCTION CALC:
      cpuInstructionsLR += // LR CPU INSTRUCTION CALC:
        instrPerRowMovedOutOfTheCache
        *
        selectedRowsPerScan;


      // Compute cost for executor predicates
      const CostScalar instrPerExePredEvaluationPerRow =
        CostPrimitives::getBasicCostFactor(CPUCOST_PREDICATE_COMPARISON ) *
        exePreds.entries();

      // First row
      cpuInstructionsFR += // FR CPU INSTRUCTION CALC:
        instrPerExePredEvaluationPerRow
        *
        rowsPerScanForFirstRow;

      // LR CPU INSTRUCTION CALC:
      cpuInstructionsLR += // LR CPU INSTRUCTION CALC:
        instrPerExePredEvaluationPerRow
        *
        rowsPerScan;

    } // if requests per scan > 0

  DCMPASSERT(cpuInstructionsFR <= cpuInstructionsLR);

  firstRow.addInstrToCPUTime(cpuInstructionsFR);
  lastRow.addInstrToCPUTime(cpuInstructionsLR);

  // -----------------------------------------------------------------------
  // SEEKS:
  // -----------------------------------------------------------------------
  CostScalar
    seeksFR = csZero
    ,seeksLR = csZero;
  // no seeks if no requests:
  if (requestsPerScan.isGreaterThanZero())
    {
      // Assume first row is obtained in the first request:
      seeksFR = (seeksPerScan/requestsPerScan).getCeiling();
      seeksLR = seeksPerScan;
    }

  firstRow.addSeeksToIOTime(seeksFR);
  lastRow.addSeeksToIOTime(seeksLR);

  DCMPASSERT(seeksFR <= seeksLR);

  // -----------------------------------------------------------------------
  // KB TRANSFERED
  // -----------------------------------------------------------------------
  CostScalar
    seqKBFR = csZero
    ,seqKBLR = csZero;
  // no kb transfered if no requests:
  if (successfulRequestsPerScan.isGreaterThanZero())
    {


      // Compute the kb that need to be transfered for first row:

      // The first row must read just enough to fill the DP2 buffer:
      // This is the minimum of:
      // seq_kb_read for all rows that qualify the query and
      // seq_kb_read needed to fill the DP2 buffer:

      const CostScalar maxBlksPerScanForFirstRow =
        CostScalar(rowsPerScanForFirstRow/recordsPerBlock).getCeiling();

      CostScalar
        maxKbReadPerScanForFirstRow = maxBlksPerScanForFirstRow * kbPerBlock;


      // Add bytes due to the effect of selectivity on read-ahead:
      const CostScalar totalRowsInRawTable =
          getRawInnerHistograms().getRowCount().getCeiling();

      // But don't go into this logic if the fraction is zero
      const CostScalar ioTransferCostPrefetchMissesFraction =
        getDefaultAsDouble(IO_TRANSFER_COST_PREFETCH_MISSES_FRACTION);

      if (ioTransferCostPrefetchMissesFraction.isGreaterThanZero()
          AND
          totalRowsInRawTable.isGreaterThanZero())
      {

        // 03/16/98: Observation:
        // It takes around 8 seconds to scan 100,000 wisconsin rows
        // when there is an executor predicate that selects only
        // one record.
        // It takes around 12 seconds to scan 100,000 wisconsin rows
        // when there is no executor predicates.
        // In both cases the KB transfered is the same (full table
        // scan). Thus it seems that the speed of transfer depends
        // on the selectivity of the executor predicates.
        // Note that is in only I/O time, not CPU. This
        // anomaly could be explained by the fact that as
        // the selectivity of the executor predicate decrease
        // the DP2 has to do more work to manage the buffers
        // that will be sent to the ESP, thus stopping the
        // data transfer and making the disk miss the read ahead.
        // When the executor is very selective, the disk read ahead
        // is not interrupted and the speed is faster.
        // Currently, the speed of transfer is a constant, not
        // a function. Therefore, to accommodate this fact,
        // I'll add bytes to the sequential read result as
        // the selectivity of the exe. preds compared to
        // the sequential read increases.



        // the maximum read ahead is when only one row is selected
        // the minimum when all rows are selected

        // read ahead reduction happens because when the dp2 buffer
        // fills out, dp2 must stop reading until it sends the
        // buffer to the ESP requesting data. Thus, there cannot
        // be read ahead reduction when we read less (per
        // probe) than the
        // dp2 buffer's size.
        CostScalar kbReadAgainstKbSelectedRatio = csZero;

        CostScalar seq_kb_readPerScanPerRqst =
          (seq_kb_readPerScan / activePartitions).getCeiling()
          /
          successfulRequestsPerScan;
        if (seq_kb_readPerScanPerRqst
            >
            CostPrimitives::getBasicCostFactor(DP2_MESSAGE_BUFFER_SIZE))
          {


            // Read ahead reduction, figure out reduction factor:
            CostScalar rowsInResultTablePerScan =
              (totalRowsInResultTable/activePartitions).getCeiling();

            CostScalar blocksInResultTablePerScan =
              (rowsInResultTablePerScan
              /
              getIndexDesc()->getEstimatedRecordsPerBlock()).getCeiling();

            CostScalar kbInResultTablePerScan =
              blocksInResultTablePerScan * getIndexDesc()->getBlockSizeInKb();

            kbReadAgainstKbSelectedRatio =
              kbInResultTablePerScan / seq_kb_readPerScan;

            // The ratio above should be alwasy less than 1, but
            // because of synthesis problems it may be greater than
            // one. This would screw costing so correct it
            // (best we can do is to make it zero)
            //DCMPASSERT(seq_kb_readPerScan >= kbInResultTablePerScan);
            if ( kbReadAgainstKbSelectedRatio.isGreaterThanOne() )
              {
                 kbReadAgainstKbSelectedRatio = csZero;
              }


          }


        // As the scan executor predicates are more selective,
        // the scan needs to read more data because of prefetch misses:


        seq_kb_readPerScan +=
          seq_kb_readPerScan *
          ioTransferCostPrefetchMissesFraction * kbReadAgainstKbSelectedRatio;

        maxKbReadPerScanForFirstRow +=
          maxKbReadPerScanForFirstRow *
          ioTransferCostPrefetchMissesFraction * kbReadAgainstKbSelectedRatio;

      }

      // we can't read more blocks for the
      // first row than for the entire scan:
      seqKBFR =
        MINOF(maxKbReadPerScanForFirstRow, seq_kb_readPerScan);
      seqKBLR =
        seq_kb_readPerScan;

    } // requestsPerScan > 0
  // else if no successful requests then some data
  // may still be read (i.e. to scan the index levels)

  firstRow.addKBytesToIOTime(seqKBFR);
  lastRow.addKBytesToIOTime(seqKBLR);

  seqKBytesPerScan = seqKBLR;

  DCMPASSERT(seqKBFR <= seqKBLR);

  // -----------------------------------------------------------------------
  // NORMAL MEMORY
  // -----------------------------------------------------------------------

  // A buffer to evaluate expressions:

  /*j  Currently not used 05/08/01, commented out for future
    firstRow.addToNormalMemory(dp2MessageBufferSizeInKb);
    lastRow.addToNormalMemory(dp2MessageBufferSizeInKb);
  j*/


  // -----------------------------------------------------------------------
  // PERSISTENT MEMORY
  // -----------------------------------------------------------------------

  const CostScalar cacheSizeInBlocks =
    getDP2CacheSizeInBlocks(getIndexDesc()->getBlockSizeInKb());
  const CostScalar dp2CacheInKb =
    cacheSizeInBlocks * getIndexDesc()->getBlockSizeInKb();

  /*j  Currently not used 05/08/01, commented out for future
  firstRow.addToPersistentMemory(dp2CacheInKb);
  lastRow.addToPersistentMemory(dp2CacheInKb);
  j*/

  // -----------------------------------------------------------------------
  // NUM. LOCAL MESSAGES
  // -----------------------------------------------------------------------
  //no local messages

  // -----------------------------------------------------------------------
  // KB REMOTE MESSAGES TRANSFER
  // -----------------------------------------------------------------------
  // no local messages

  // -----------------------------------------------------------------------
  // NUM. REMOTE MESSAGES
  // -----------------------------------------------------------------------
  //no remote messages

  // -----------------------------------------------------------------------
  // KB REMOTE MESSAGES TRANSFER
  // -----------------------------------------------------------------------
  // no remote msg


  // -----------------------------------------------------------------------
  // IDLE TIME:*************************************************************
  // -----------------------------------------------------------------------

  // All scan initialization cost (opening a file, etc.) are
  // added to idle since it does not overlapp with CPU.
  // We cannot add it to blocking since the scan is not a
  // blocking operator.
  // -----------------------------------------------------------------------

  // No idle time
  CostScalar
    idleTimeFR = csZero
    ,idleTimeLR = csZero;

  // the openFile() method really computes all the initial time
  // that it is spent by setting up the DP2 access for a given
  // scan.

  // CPUCOST_SUBSET_OPEN lumps together all the overhead needed
  // to set-up the access to each partition. Thus it is a blocking
  // cost, nothing can overlap with it.
  // Since scans are not blocking, by definition, put the cost
  // in idle time:
  // this is the cost for opening all the partitions but the first partition.
  // The first partition is opened before the ScanOptimizer is called. During
  // opening the first partition, the necessary info on the file is also
  // acquired so it is more expensive and cannot be overlaid.
  //  Root accounts for the cost of opening the first partition of all the tables.

  CostScalar openFileCPU =
    CostPrimitives::getBasicCostFactor(CPUCOST_SUBSET_OPEN_AFTER_FIRST)
    *
    MAXOF(0, (activePartitions.getValue() - 1) );

  idleTimeFR = idleTimeLR =
   openFileCPU * CostPrimitives::getBasicCostFactor(MSCF_ET_CPU);


  firstRow.addToIdleTime(idleTimeFR);
  lastRow.addToIdleTime(idleTimeLR);

  // -----------------------------------------------------------------------
  // DISK*******************************************************************
  // -----------------------------------------------------------------------
  // no disk access for scan

  // -----------------------------------------------------------------------
  // PROBES:          ******************************************************
  // -----------------------------------------------------------------------

  firstRow.setNumProbes(probesPerScan);
  lastRow.setNumProbes(probesPerScan);


} // computeCostVectors(...)

// -----------------------------------------------------------------------
// INPUT:
//      probes: the total probes coming in
//      preds: the predicates to compute the join
//
// OUTPUT:
//      successfuleProbes: those probes that match any data
//      uniqueSuccProbes: those successful probes that do not have duplicates
//      duplicateSuccProbes: successfulProbes - uniqueSuccProbes
//      failedProbes: probes - successfulProbes
// -----------------------------------------------------------------------
void
ScanOptimizer::categorizeProbes(CostScalar& successfulProbes /* out */
                                ,CostScalar& uniqueSuccProbes /* out */
                                ,CostScalar& duplicateSuccProbes /* out */
                                ,CostScalar& failedProbes /* out */
                                ,CostScalar& uniqueFailedProbes 
                                ,const CostScalar& probes
                                ,const ValueIdSet& preds
                                ,const Histograms& outerHistograms
                                ,const NABoolean isMDAM
                                ,CostScalar * dataRows
                                ) const
{


  if (outerHistograms.isEmpty())
  {
    successfulProbes = uniqueSuccProbes = probes;
    duplicateSuccProbes = failedProbes = csZero;
    // Patch for Sol.10-031024-0755 (Case 10-031024-9406). When RI
    // constraints were implemented, corresponding histograms, cardinality
    // and costing were forgotten. As a result nested join into referenced
    // table does not pass histogram information to the right child.
    // If the right child does not have histograms in inputLogProp
    // it considers this join as a cross product.
    // This is the second part of the patch. The number of rows to cost was
    // left as 0 in case of empty outputHistograms which is not right if we
    // have a real cross product or do a full scan of the table for each
    // probe. Therefore the cost of index_scan was underestimated and we
    // picking up index scan (containing referenced column) instead of
    // file_scan_unique of primary index or index with referenced column
    // as a key.
    // I changed dataRows to the product of probes and table cardinality.
    if ( CmpCommon::getDefault(COMP_BOOL_35) == DF_OFF )
     *dataRows *= probes;

  }
  else
  {

      // If there are input values we are in a nested join case,
      // else it must be a cartesian product:
      const ValueIdSet& inputValues =
	getRelExpr().getGroupAttr()->getCharacteristicInputs();

      // get this filescan statistics:
      IndexDescHistograms
        joinedHistograms(*getIndexDesc(),
                         getIndexDesc()->getIndexKey().entries() );

      // Since there are input values, apply the join preds
          // to estimate the failed probes:
	// Apply the join predicates to the local statistics:
      const SelectivityHint * selHint = getIndexDesc()->getPrimaryTableDesc()->getSelectivityHint();
      const CardinalityHint * cardHint = getIndexDesc()->getPrimaryTableDesc()->getCardinalityHint();

      joinedHistograms.applyPredicatesWhenMultipleProbes(
                   preds
                   ,*(getContext().getInputLogProp())
                   ,inputValues
                   ,isMDAM
		   ,selHint
		   ,cardHint
                   ,NULL
		   ,REL_SCAN);

      failedProbes = csZero; // assume no probes will fail
      if (NOT inputValues.isEmpty())
	{

        if (NOT preds.isEmpty())
	    {


	      // and compute the failed probes:
	      const ValueIdSet& operatorValues =
		getIndexDesc()->getIndexKey();
	      failedProbes =
		joinedHistograms.computeFailedProbes(outerHistograms
						     ,preds
						     ,inputValues
						     ,operatorValues
						     );
              // odbc unfriendly assertion
	      //DCMPASSERT(probes >= failedProbes);
              // When histograms are ready, remove the block
              // below and uncomment the assertion above.
              if (probes < failedProbes)
                {
                  FSOWARNING("probes < failedProbes");
                  failedProbes = probes;
                }

	    }
	} // if there are input values

      if(dataRows) *dataRows = joinedHistograms.getRowCount();
      // ------------------------------------------------------------------
      // Compute successful probes:
      // -------------------------------------------------------------------
      successfulProbes = probes - failedProbes;
//      successfulProbes.round(); // to make it zero if very small
      successfulProbes.roundIfZero();
//      if (successfulProbes < 0)
//      {
//        FSOWARNING("successfulProbes < 0.");
//        successfulProbes = 0.;
//      }
      successfulProbes.minCsZero();

      // --------------------------------------------------------------
      // Compute unique probes:
      // -------------------------------------------------------------
      // Combine unique entry count of the probes:
      // (assume independance of columns and compute
      // Compute combined UEC of outer histograms):
      CostScalar uniqueEntryCountOfProbes = csOne;
      CostScalar histUEC = csZero;
      for (CollIndex i=0; i < outerHistograms.entries(); i++)
      {
        histUEC = outerHistograms[i].getColStats()
            ->getTotalUec().getCeiling();

        //we use the method EXP_REAL64_OV_MUL to get the product of the first two
	//values passed in and if there occurs a overflow or underflow then
	//"overflow" gets TRUE and the "tempvariable" get junk value, if there
	//is no overflow then "overflow" gets false and the product is stored
	//in tempvariable and later assigned it to uniqueEntryCountOfProbes
	uniqueEntryCountOfProbes *= histUEC;
      }

      //should not be more than # of probes
      if (uniqueEntryCountOfProbes > probes)
        uniqueEntryCountOfProbes = probes;

      //#should not be less than 1
      // if (uniqueEntryCountOfProbes < 1) uniqueEntryCountOfProbes = 1;
      uniqueEntryCountOfProbes.minCsOne();

      // The unique successful probes are the successful
      // fraction of their UEC:
      uniqueSuccProbes =
	MINOF(
	     ((successfulProbes/probes)*uniqueEntryCountOfProbes).getValue(),
	     successfulProbes.getValue());

      uniqueFailedProbes =
           MINOF( ((failedProbes/probes)*uniqueEntryCountOfProbes).getValue(),
		    successfulProbes.getValue() ); 

      // --------------------------------------------------------------
      // Compute successful probes that are duplicates of another
      // successful probe:
      // --------------------------------------------------------------
      duplicateSuccProbes = successfulProbes - uniqueSuccProbes;

  }

} // FileScanOptimizer::categorizeProbes(...)

NABoolean FileScanOptimizer::hasTooManyDisjuncts() const
{
  const Disjuncts &disjuncts = getDisjuncts();
  CollIndex numOfEntries = disjuncts.entries();
  if(numOfEntries > MDAM_MAX_NUM_DISJUNCTS)
    return TRUE;
  else
    return FALSE;
}

// eof
// Implementation of MDAMCostWA methods

MDAMCostWA::MDAMCostWA
(FileScanOptimizer & optimizer,
 NABoolean mdamForced,
 MdamKey *mdamKeyPtr,
 const Cost *costBoundPtr,
 const ValueIdSet & exePreds,
 SimpleCostVector & disjunctsFR,
 SimpleCostVector & disjunctsLR) :
   mdamWon_(FALSE)
  ,noExePreds_(TRUE)
  ,numKBytes_(0)
  ,mdamForced_(mdamForced)
  ,mdamKeyPtr_(mdamKeyPtr)
  ,costBoundPtr_(costBoundPtr)
  ,optimizer_(optimizer)
  ,disjunctsFR_(disjunctsFR)
  ,disjunctsLR_(disjunctsLR)
  ,scmCost_(NULL)
  ,exePreds_(exePreds) //optimizer.getRelExpr().getSelectionPred())
  ,innerRowsUpperBound_( optimizer.getRawInnerHistograms().
			 getRowCount().getCeiling() )
  ,innerBlocksUpperBound_(0)
  ,estimatedRecordsPerBlock_( optimizer.getIndexDesc()->
			      getEstimatedRecordsPerBlock() )
  ,isMultipleProbes_(optimizer.isMultipleProbes())
  ,scanForcePtr_(optimizer.findScanForceWildCard())
  ,incomingProbes_(optimizer.getIncomingProbes())
  ,firstColumnHistogram_( *(optimizer.getIndexDesc()), 1 )
  ,outerHistograms_(optimizer.getContext().getInputLogProp()->getColStats())
  ,disjunctIndex_(0)
  ,disjunctOptRows_(0)
  ,disjunctOptRqsts_(0)
  ,disjunctOptProbesForSubsetBoundaries_(0)
  ,disjunctOptSeeks_(0)
  ,disjunctOptSeqKBRead_(0)
  ,disjunctOptKeyPreds_()
  ,disjunctMdamOK_(FALSE)
  ,disjunctNumLeadingPartPreds_(0)
  ,disjunctFailedProbes_(0)
{}

// This function computes whether MDAM wins over the cost bound
// It sums up cost factors for all disjuncts, calculates the cost
// and compares with the cost bound.
void MDAMCostWA::compute()
{
  if(isMultipleProbes_)
  {
    MDAM_DEBUG0(MTL2, "Mdam scan is multiple probes");
  }
  else {
    incomingProbes_ = 1;
    MDAM_DEBUG0(MTL2, "Mdam scan is a single probe");
  }

  // total stats counters for all disjuncts
  CostScalar
    totalRows // rows
    ,totalRqsts // probes
    ,totalFailedProbes // probes that create empty mdam trees
    ,totalProbesForSubsetBoundaries // probes to get the next subset boundary
    ,totalSeeks // total number of disk arm movements
    ,totalDisjunctPreds // the sum of predicates in all disjuncts
    ,totalSeqKBRead; // total number of blocks that were read sequentially
  CollIndex totalNumLeadingPartPreds = 0;
  CostScalar seqKBytesPerScan;
  ValueIdSet totalKeyPreds;

  // Compute upper bounds:
  computeBlocksUpperBound(innerBlocksUpperBound_ /* out*/
    ,innerRowsUpperBound_ /* in */
    ,estimatedRecordsPerBlock_ /*in*/);

  const CostScalar recordSizeInKb = optimizer_.getIndexDesc()->getRecordSizeInKb();
  const CostScalar blockSizeInKb =
    optimizer_.getIndexDesc()->getBlockSizeInKb();
  NADefaults & defs = ActiveSchemaDB()->getDefaults();
  const CostScalar maxDp2ReadInBlocks =
    CostScalar(defs.getAsULong(DP2_MAX_READ_PER_ACCESS_IN_KB))/
    blockSizeInKb;
  // -----------------------------------------------------------------------
  //  Loop through every disjunct and:
  //   1 Find the optimal disjunct prefix for the disjunct
  //   2 Compute the sum of the costs of all disjuncts
  //   3 If the cost exceeds the bound, mdam lose and return
  // -----------------------------------------------------------------------
  for (CollIndex disjunctIndex=0;
       disjunctIndex < mdamKeyPtr_->getKeyDisjunctEntries();
       disjunctIndex++)
    {
      disjunctIndex_ = disjunctIndex;
      computeDisjunct();
      if(NOT disjunctMdamOK_)
	{
	  mdamWon_ = FALSE;
// 	  // invalidate the cache
// 	  disjunctsFR_.reset();
// 	  disjunctsLR_.reset();
          MDAM_DEBUG0(MTL2, "Mdam scan lost because disjunctMdamOK_ is false");
	  return;
	}

      // Sum up the statistics for all the disjuncts costed so far
      totalRows += disjunctOptRows_;
      totalRqsts += disjunctOptRqsts_ +	disjunctOptProbesForSubsetBoundaries_;
      totalSeeks += disjunctOptSeeks_;
      totalSeqKBRead += disjunctOptSeqKBRead_;
      totalKeyPreds.insert(disjunctOptKeyPreds_);
      totalDisjunctPreds += disjunctOptKeyPreds_.entries();
      totalNumLeadingPartPreds += disjunctNumLeadingPartPreds_;
      // This may not be correct,
      // If a probe failes for one disjunct, it may not fail for another
      // Need to investigate after R2.0.
      totalFailedProbes += disjunctFailedProbes_;
      // If we have partitioning key predicates then we will be reading
      // different parts of the partition by different esps.  There will be
      // some seeks going from one esp reading to another.  Assume
      // that we will at least read ahead
      // the normal amount (for now reduce possible seeks by 100).

      // If there is partition predicates in the disjunct key predicates,
      // What would be the effects on the seeks ?
      if (totalNumLeadingPartPreds == mdamKeyPtr_->getKeyDisjunctEntries()
	  AND (totalRows / estimatedRecordsPerBlock_) > maxDp2ReadInBlocks)
	totalSeeks += (totalRows / estimatedRecordsPerBlock_)/
	  maxDp2ReadInBlocks/100;

      // Compute the cost of all the disjuncts analyzed so far:
      if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
      {
        if (optimizer_.getIndexDesc()->getPrimaryTableDesc()->getNATable()->isHbaseTable())
        {
          // Cost of sending 'probes' to materialize values of prefix key column(s) with 
          // missing predicate(s) should be accounted. For each such probe Hbase Region server
          // returns 1 row. So, totalRqsts (probes) should be added to seeks as well as 
          // rows processed.
          CostScalar totRowsProcessed = totalRows + totalRqsts;
          CostScalar totSeeks = totalSeeks + totalRqsts;
          scmCost_ = optimizer_.scmComputeMDAMCostForHbase(totRowsProcessed, totSeeks, 
                                                           totalSeqKBRead, incomingProbes_);

        }
        else 
        {
	  CostScalar activePartitions = optimizer_.getEstNumActivePartitionsAtRuntime();
	  CostScalar totalRowsInResultTable = (CostScalar)optimizer_.getResultSetCardinality();

	  // Patch for Sol.10-031024-0755 (Case 10-031024-9406).
	  totalRowsInResultTable = MINOF(totalRowsInResultTable,totalRows);

	  CostScalar
	    rowsPerScan = CostScalar(totalRows/activePartitions).getCeiling()
	    ,selectedRowsPerScan =
	    CostScalar(totalRowsInResultTable/activePartitions).getCeiling()
	    ,seeksPerScan = CostScalar(totalSeeks/activePartitions).getCeiling()
	    ,seqKBPerScan = CostScalar(MAXOF(totalSeqKBRead/activePartitions,4.0))
	    ,probesPerScan = CostScalar(incomingProbes_/activePartitions).getCeiling();

	  // Factor in row sizes.
	  CostScalar rowSize = recordSizeInKb * csOneKiloBytes;
	  CostScalar outputRowSize = optimizer_.getRelExpr().getGroupAttr()->getRecordLength();
	  CostScalar rowSizeFactor = optimizer_.scmRowSizeFactor(rowSize);
	  CostScalar outputRowSizeFactor = optimizer_.scmRowSizeFactor(outputRowSize);
	  rowsPerScan *= rowSizeFactor;
	  selectedRowsPerScan *= outputRowSizeFactor;

          CostScalar rowSizeFactorSeqIO = optimizer_.scmRowSizeFactor
                                                       (rowSize, ScanOptimizer::SEQ_IO_ROWSIZE_FACTOR);
          CostScalar rowSizeFactorRandIO = optimizer_.scmRowSizeFactor
                                                       (rowSize, ScanOptimizer::RAND_IO_ROWSIZE_FACTOR);

	  CostScalar ioSeqPerScan = (seqKBPerScan/blockSizeInKb).getCeiling() * rowSizeFactorSeqIO;
	  CostScalar ioRandPerScan = seeksPerScan * rowSizeFactorRandIO;

	  scmCost_ =  
	    optimizer_.scmCost(rowsPerScan, selectedRowsPerScan, csZero, ioRandPerScan, 
                               ioSeqPerScan, probesPerScan, rowSize, csZero, outputRowSize, csZero);
        }

        // scale up mdam cost by factor of NCM_MDAM_COST_ADJ_FACTOR --default is 1
        CostScalar costAdj = (ActiveSchemaDB()->getDefaults()).getAsDouble(NCM_MDAM_COST_ADJ_FACTOR);
        scmCost_->cpScmlr().scaleByValue(costAdj);

	if (costBoundPtr_ != NULL &&
	    costBoundPtr_->scmCompareCosts(*scmCost_) == LESS)
        {
	  mdamWon_ = FALSE;
          MDAM_DEBUG0(MTL2, "Mdam scan lost due to higher cost determined by scmCompareCosts()");
          return;
	}
      }
      else
      {
	disjunctsFR_.reset();
	disjunctsLR_.reset();

	optimizer_.computeCostVectorsForMultipleSubset(disjunctsFR_ /* out */
						       ,disjunctsLR_ /* out */
						       ,seqKBytesPerScan /* out */
						       ,totalRows
						       ,totalRqsts
						       ,totalFailedProbes
						       ,totalSeeks
						       ,totalSeqKBRead
						       ,totalKeyPreds
						       ,exePreds_
						       ,incomingProbes_
						       ,totalDisjunctPreds
						       );

	// checking some adjustments, 
	// Need to introduce some Tunable factor for 100
	if ( (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON) &&
		(totalRows < CostScalar(100)) ){
			disjunctsFR_ = disjunctsFR_*(totalRows/CostScalar(100));
			disjunctsLR_ = disjunctsLR_*(totalRows/CostScalar(100));
	  }


	if ( optimizer_.exceedsBound(costBoundPtr_, disjunctsFR_, disjunctsLR_) )
        {
	  mdamWon_ = FALSE;
// 	  // invalidate the cache
// 	  disjunctsFR_.reset();
// 	  disjunctsLR_.reset();
          MDAM_DEBUG0(MTL2, "Mdam scan lost due to exceeding cost bound");
          return;
	}
      }
    } // for every disjunct

  // update rows accessed
  optimizer_.setEstRowsAccessed(totalRows);
  mdamWon_ = TRUE;
  numKBytes_ = seqKBytesPerScan;
}

// This function computes the cost factors of a MDAM disjunct
// It computes the members below:
//   disjunctMdamOK_
//   disjunctOptRows_
//   disjunctOptRqsts_
//   disjunctOptProbesForSubsetBoundaries_
//   disjunctOptSeeks_
//   disjunctOptSeqKBRead_
//   disjunctOptKeyPreds_
//   disjunctNumLeadingPartPreds_
//   disjunctFailedProbes_
//   noExePreds_
void MDAMCostWA::computeDisjunct()
{
  // get the key preds for this disjunct:
  NABoolean allKeyPredicates = FALSE;
  ValueIdSet disjunctKeyPreds;
  mdamKeyPtr_->getKeyPredicates(disjunctKeyPreds /*out*/,
			       &allKeyPredicates /*out*/,
			       disjunctIndex_ /*in*/);
  if( NOT (allKeyPredicates) )
    noExePreds_ = FALSE;


  // return with a NULL cost if there are no key predicates
  // "costBoundPtr_ == NULL" means "MDAM is forced"
  if (disjunctKeyPreds.isEmpty() AND costBoundPtr_ != NULL) {
    MDAM_DEBUG0(MTL2, "MDAMCostWA::computeDisjunct(): disjunctKeyPreds is empty"); 
    disjunctMdamOK_ = FALSE;
    return; // full table scan, MDAM is worthless here
  }

  // Tabulate the key predicates using the key columns as the index
  ColumnOrderList keyPredsByCol(optimizer_.getIndexDesc()->getIndexKey());
  mdamKeyPtr_->getKeyPredicatesByColumn(keyPredsByCol,disjunctIndex_);

  MDAM_DEBUG1(MTL2, "Disjunct: %d, keyPredsByCol:", disjunctIndex_);
  MDAM_DEBUGX(MTL2, keyPredsByCol.print());
  MDAM_DEBUG0(MTL2, "disjunctKeyPreds no recomputing");
  MDAM_DEBUGX(MTL2, disjunctKeyPreds.display());

  // compute the optimal prefix and the associated min cost
  MDAMOptimalDisjunctPrefixWA prefixWA(optimizer_,
				       keyPredsByCol,
				       disjunctKeyPreds,
				       exePreds_,
				       outerHistograms_,
				       firstColumnHistogram_,
				       noExePreds_,
				       mdamKeyPtr_,
				       scanForcePtr_,
				       mdamForced_,
				       isMultipleProbes_,
				       incomingProbes_,
				       estimatedRecordsPerBlock_,
				       innerRowsUpperBound_,
				       innerBlocksUpperBound_,
				       disjunctIndex_);

  prefixWA.compute();
  CollIndex stopColumn = prefixWA.getStopColumn();
  disjunctNumLeadingPartPreds_ += prefixWA.getNumLeadingPartPreds();
  disjunctFailedProbes_ = prefixWA.getFailedProbes();
  disjunctOptRows_ = prefixWA.getOptRows();
  disjunctOptRqsts_ = prefixWA.getOptRqsts();
  disjunctOptProbesForSubsetBoundaries_ =
                        prefixWA.getOptProbesForSubsetBoundaries();
  disjunctOptSeeks_ = prefixWA.getOptSeeks();
  disjunctOptSeqKBRead_ = prefixWA.getOptSeqKBRead();
  disjunctOptKeyPreds_ = prefixWA.getOptKeyPreds();
  // Set the stop column for current disjunct:
  mdamKeyPtr_->setStopColumn(disjunctIndex_, stopColumn);

  NABoolean mdamMakeSense = TRUE;
  if (NOT mdamForced_  AND stopColumn == 0) {
    if (keyPredsByCol.getPredicateExpressionPtr(0) == NULL )
      mdamMakeSense = FALSE;
    else if (mdamKeyPtr_->getKeyDisjunctEntries() == 1) {
      // When there is a conflict in single subset, mdam should handle it
      const ColumnOrderList::KeyColumn* currKeyColumn =
	keyPredsByCol.getPredicateExpressionPtr(0);

      NABoolean conflict =
	(    (currKeyColumn->getType() == KeyColumns::KeyColumn:: CONFLICT)
	     OR
	     (currKeyColumn->getType() ==
	      KeyColumns::KeyColumn::CONFLICT_EQUALS)   
	     OR
	     // added for patching (since it is a single disjunct now, but not a conflict)
	     // where a =10 or a=20 or a=30
	     (currKeyColumn->getType() ==
	      KeyColumns::KeyColumn::INLIST) );
      
      if( NOT conflict ) { // single subset should be chosen.
        MDAM_DEBUG0(MTL2, "MDAMCostWA::computeDisjunct(): conflict predicate for single subset, force MDAM off"); 
	mdamMakeSense = FALSE;
      }
    }
  }

  CollIndex noOfmissingKeyColumnsTot = 0;
  CollIndex presentKeyColumnsTot = 0;


  const IndexDesc *idesc = optimizer_.getFileScan().getIndexDesc();
  const ColStatDescList& csdl = idesc->getPrimaryTableDesc()->getTableColStats();
  Histograms hist(csdl);
	 
  Lng32 checkOption = (ActiveSchemaDB()->getDefaults()).getAsLong(MDAM_APPLY_RESTRICTION_CHECK);

  if(CURRSTMT_OPTDEFAULTS->indexEliminationLevel() != OptDefaults::MINIMUM
     && (!mdamForced_)
	 && (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
	 && checkOption >= 1 
	 && (!checkMDAMadditionalRestriction(
                                    keyPredsByCol,
                                    optimizer_.computeLastKeyColumnOfDisjunct(keyPredsByCol),
                                    hist,
                                    (restrictCheckStrategy)checkOption,
                                    noOfmissingKeyColumnsTot,
                                    presentKeyColumnsTot))
   )
  {
        
    MDAM_DEBUG0(MTL2, "MDAMCostWA::computeDisjunct(): MDAM additional restriction check failed"); 
    mdamMakeSense = FALSE;
  }
  disjunctMdamOK_ = mdamMakeSense;
}

NABoolean MDAMCostWA::isMdamWon() const
{
  return mdamWon_;
}

NABoolean MDAMCostWA::hasNoExePreds() const
{
  return noExePreds_;
}

const CostScalar & MDAMCostWA::getNumKBytes() const
{
  return numKBytes_;
}

Cost * MDAMCostWA::getScmCost()
{
  return scmCost_;
}


// Implementation of MDAMOptimalDisjunctPrefixWA methods

MDAMOptimalDisjunctPrefixWA::MDAMOptimalDisjunctPrefixWA
(FileScanOptimizer & optimizer,
 const ColumnOrderList & keyPredsByCol,
 const ValueIdSet & disjunctKeyPreds,
 const ValueIdSet & exePreds,
 const Histograms & outerHistograms,
 IndexDescHistograms & firstColumnHistogram,
 NABoolean & noExePreds,
 MdamKey *mdamKeyPtr,
 const ScanForceWildCard *scanForcePtr,
 NABoolean mdamForced,
 NABoolean isMultipleProbes,
 const CostScalar & incomingProbes,
 const CostScalar & estimatedRecordsPerBlock,
 const CostScalar & innerRowsUpperBound,
 const CostScalar & innerBlocksUpperBound,
 CollIndex disjunctIndex)

 :
  failedProbes_(0)
  ,optRows_(0)
  ,optRqsts_(0)
  ,optRqstsForSubsetBoundaries_(0)
  ,optSeeks_(0)
  ,optSeqKBRead_(0)
  ,optKeyPreds_()
  ,stopColumn_(0)
  ,numLeadingPartPreds_(0)
  ,optimizer_(optimizer)
  ,keyPredsByCol_(keyPredsByCol)
  ,disjunctKeyPreds_(disjunctKeyPreds)
  ,exePreds_(exePreds)
  ,outerHistograms_(outerHistograms)
  ,scanForcePtr_(scanForcePtr)
  ,isMultipleProbes_(isMultipleProbes)
  ,mdamForced_(mdamForced)
  ,incomingProbes_(incomingProbes)
  ,estimatedRecordsPerBlock_(estimatedRecordsPerBlock)
  ,innerRowsUpperBound_(innerRowsUpperBound)
  ,innerBlocksUpperBound_(innerBlocksUpperBound)
  ,disjunctIndex_(disjunctIndex)
  ,firstColumnHistogram_(firstColumnHistogram)
  ,noExePreds_(noExePreds)
  ,mdamKeyPtr_(mdamKeyPtr)
  ,disjunctHistograms_( *(optimizer.getIndexDesc()), 0 )
  ,multiColUecInfoAvail_(disjunctHistograms_.isMultiColUecInfoAvail())
  ,lastColumnPosition_(optimizer.computeLastKeyColumnOfDisjunct(keyPredsByCol))
  ,firstColOverlaps_(FALSE)
  ,prefixSubsets_(csOne) // MDAM subsets
  ,cumulativePrefixSubsets_(csZero)
  ,probeTax_(ActiveSchemaDB()->getDefaults().getAsDouble(MDAM_PROBE_TAX))
  ,prefixSubsetsAsSeeks_(csOne) // MDAM subsets for all probes
  ,prefixRows_(0)
  ,prefixRqsts_(csOne)
  ,prefixRqstsForSubsetBoundaries_(0)
  ,multiProbesDataRows_(0)
  ,prefixSeeks_(0)
  ,prefixKBRead_(0)
  ,prefixKeyPreds_()
  ,keyPredsPtr2Apply2Histograms_(NULL)
  ,prefixColumnPosition_(0)
  ,curPredIsEqual_(FALSE)
  ,prevPredIsEqual_(FALSE)
  // The uec is used as a multiplier. Intialize the ones
  // for the column previous to the very first column
  ,uecForPreviousCol_(csOne)
  ,uecForPreviousColBeforeAppPreds_(csOne)
  ,uecForPrevColForSeeks_(csOne)
  ,firstRound_(TRUE)
  ,crossProductApplied_(FALSE)
  ,prevColChosen_(FALSE)
  ,sumOfUecs_(csOne)
  ,sumOfUecsSoFar_(csOne)
  ,blocksToReadPerUec_(0)
  ,pMinCost_(NULL)
  ,rcAfterApplyFirstKeyPreds_(0)
{}



MDAMOptimalDisjunctPrefixWA::~MDAMOptimalDisjunctPrefixWA()
{
  if(pMinCost_)
    delete pMinCost_;
}

// This method find if there are any intervenning missing key column present 
NABoolean MDAMOptimalDisjunctPrefixWA::missingKeyColumnExists() const
{
   KeyColumns::KeyColumn::KeyColumnType typeOfRange = KeyColumns::KeyColumn::EMPTY;
   CollIndex index = 0;
   for (index = 0; index < keyPredsByCol_.entries(); index++)
   {
     if (keyPredsByCol_.getPredicateExpressionPtr(index) != NULL)
       typeOfRange = keyPredsByCol_.getPredicateExpressionPtr(index)->getType();
     else
       typeOfRange= KeyColumns::KeyColumn::EMPTY;
     if(typeOfRange == KeyColumns::KeyColumn::EMPTY )
       break;
   }
   if( index < (lastColumnPosition_ - 1))
   {
     return TRUE;
   }
   return FALSE;
}

// This function computes the optimal prefix of the disjunct
void MDAMOptimalDisjunctPrefixWA::compute()
{

  CostScalar uniqueProbes = csOne; // dummy var
  // compute the number of failed probes for the disjunct
  computeProbesDisjunct();

  // compute the sum of uecs for all columns
  const ColStatDescList & primaryTableCSDL =
    optimizer_.getIndexDesc()->getPrimaryTableDesc()->getTableColStats();
  const ValueIdList & keyColumns = optimizer_.getIndexDesc()->getIndexKey();
  for (CollIndex colNum = 0; colNum < keyColumns.entries(); colNum++)
    {
      CollIndex indexInList;
      primaryTableCSDL.
	getColStatDescIndexForColumn(indexInList, keyColumns[colNum]);
      sumOfUecs_ += primaryTableCSDL[indexInList]->getColStats()->
	getTotalUec().getCeiling();
    }

  processLeadingColumns();
  prefixColumnPosition_++;
  while (prefixColumnPosition_ < lastColumnPosition_)
    {
      processNonLeadingColumn();
      prefixColumnPosition_++;
    }
}

// This function compute the number of failed probes.
// It has side effects on member failedProbes_
void MDAMOptimalDisjunctPrefixWA::
computeProbesDisjunct()
{
  if (isMultipleProbes_)
    {
      CostScalar
	disSuccProbes
	,disUniSucPrbs       /* dummy */
	,disDupSucPrbs       /* dummy */
	,disFldPrbs          /* dummy */
	,disUniFailedProbes; /* dummy */


      // Compute the RC by applying 1st non-empty key predicate.
      // Any columns before the key column with the key predicate
      // should have no predicates. Do this only if the 1st key predicate
      // is not the same as the current predicate (disjunctKeyPreds_) to process.

      CollIndex keyLength =(optimizer_.getIndexDesc()->getIndexKey()).entries();
      const ValueIdSet divisioningColumns =
           optimizer_.getIndexDesc()->
              getPrimaryTableDesc()->getDivisioningColumns();

      NABoolean sameKeyPreds = FALSE;
      if ( keyLength > 0 && divisioningColumns.entries() > 0 &&
           CmpCommon::getDefault(MTD_GENERATE_CC_PREDS) == DF_ON )
      {
         const ValueIdSet *predsPtr = NULL;
         for (CollIndex i=0; i<keyLength; i++) {
          predsPtr = keyPredsByCol_[i];
          sameKeyPreds = ( disjunctKeyPreds_ == *predsPtr );
          if (predsPtr AND NOT predsPtr->isEmpty() && i>0 && !sameKeyPreds) {
   
            optimizer_.categorizeProbes(disSuccProbes /* out */
              ,disUniSucPrbs                          /* out, not used */
              ,disDupSucPrbs                          /* out, not used */
              ,disFldPrbs                             /* out, not used */
              ,disUniFailedProbes                     /* out, not used */
              ,incomingProbes_
              ,*predsPtr
              ,outerHistograms_
              ,TRUE // this is MDAM!
              ,&rcAfterApplyFirstKeyPreds_ // cache the RC 
              );
   
              break;
           }
         }
      }

      // do the real work to figure out # of probes
      optimizer_.categorizeProbes(disSuccProbes /* out */
	,disUniSucPrbs                          /* out, not used */
	,disDupSucPrbs                          /* out, not used */
	,disFldPrbs                             /* out, not used */
	,disUniFailedProbes                     /* out, not used */
	,incomingProbes_
	,disjunctKeyPreds_
	,outerHistograms_
	,TRUE // this is MDAM!
	,&multiProbesDataRows_
	);

      if ( sameKeyPreds )
        rcAfterApplyFirstKeyPreds_ = multiProbesDataRows_;

      MDAM_DEBUG1(MTL2, "Incoming Probes %f", incomingProbes_.value());
      MDAM_DEBUGX(MTL2, outerHistograms_.print());
      MDAM_DEBUGX(MTL2, disjunctKeyPreds_.display());
      MDAM_DEBUG1(MTL2, "categorizeProbes returns rows: %f",
          multiProbesDataRows_.value());
      failedProbes_ = incomingProbes_ - disSuccProbes;
      DCMPASSERT(failedProbes_ >= csZero);
    }
}

// This function processes the leading columns and initialize the
// optimal prefix. If there is a leading single subset, we include
// the single subset in our optimal prefix.
// It updates the firstColumnHistogram_, computes firstColOverlaps_
// It initializes disjunctHistograms_, prefixKeyPreds_,
// and other memebers related to the current column.
void MDAMOptimalDisjunctPrefixWA::processLeadingColumns()
{
  disjunctHistograms_.appendHistogramForColumnPosition(1);
  // compute whether there is overlap on the first column
  CostScalar firstColRowCntBefore =
      firstColumnHistogram_.getColStatsForColumn
    ( optimizer_.getIndexDesc()->getIndexKey()[0]).getRowcount().getCeiling();
  // Apply first col predicate of the disjunct to first col histogram
  const ValueIdSet *predsPtr = keyPredsByCol_[0];
  if (predsPtr AND NOT predsPtr->isEmpty())
    {
      prefixKeyPreds_.insert(*predsPtr);
      const SelectivityHint * selHint = optimizer_.getIndexDesc()->getPrimaryTableDesc()->getSelectivityHint();
      const CardinalityHint * cardHint = optimizer_.getIndexDesc()->getPrimaryTableDesc()->getCardinalityHint();

      firstColumnHistogram_.applyPredicates(*predsPtr, (Scan &)optimizer_.getRelExpr(), selHint, cardHint, REL_SCAN);
      // accumulate the number of partitioning key predicates
      ValueId predId;
      BiRelat * predIEptr;
      for(predId = predsPtr->init();
	  predsPtr->next(predId);
	  predsPtr->advance(predId))
      {
	ValueIdSet vdset;
	if (predId.getItemExpr()->getArity() == 2)
	{
	  // making change for blocking ITM_OR
	  if (predId.getItemExpr()->getOperatorType() == ITM_OR) 
	  {
	    predId.getItemExpr()->getLeafPredicates(vdset);
	    for (ValueId predId = vdset.init();
		 vdset.next(predId);
		 vdset.advance(predId) )
	    {
	      predIEptr=(BiRelat *)predId.getItemExpr();
	      if (predIEptr->isaPartKeyPred())
	      {
		numLeadingPartPreds_++;		     
	      }
	    }
	  }
	  else
	  {
	    predIEptr=(BiRelat *)predId.getItemExpr();
	    if (predIEptr->isaPartKeyPred())
	    {
	      numLeadingPartPreds_++;
	      break;
	    }
	  }//else
	}
      }
    }
  CostScalar firstColRowCntAfter =
    firstColumnHistogram_.getColStatsForColumn
    (optimizer_.getIndexDesc()->getIndexKey()[0]).getRowcount().getCeiling();

  processSingleSubsetPrefix();

  keyPredsPtr2Apply2Histograms_ = &prefixKeyPreds_;

  applyPredsToHistogram(); // modify prefixRows_

  if (predsPtr==NULL OR predsPtr->isEmpty()) {

    // Adjust prefixRows_ based on the rowcount as a result of applying 
    // the first non-empty key predicate (if any). The value has been 
    // computed by callling ScanOptimizer::categorizeProbes(). The result is
    // stored in ScanOptimizer::rcAfterApplyFirstKeyPreds_;
    CostScalar rc = rcAfterApplyFirstKeyPreds();
    if ( rc.isGreaterThanZero() )
      prefixRows_ = rc;
  }

  firstColOverlaps_ =
    ( (firstColRowCntBefore == firstColRowCntAfter)
    AND
    (disjunctIndex_ > 0)
    AND
    (firstColRowCntAfter == prefixRows_) );

//  firstColOverlaps_ = FALSE;

  computeDensityOfColumn();
  updatePositions(); // may better be decomposed on whether column == 0
  updateStatistics();
  updateMinPrefix();
}

// This function includes the leading single subset to the optimal prefix
// It has side effects on disjunctHistograms_, prefixKeyPreds_,
// prefixSubsets_, disjunctSubSetsAsSeeks_,
// prefixRqstsForSubsetBoundaries_, prefixColumnPosition_,
void MDAMOptimalDisjunctPrefixWA::processSingleSubsetPrefix()
{
  // hasSingleSubset is FALSE iff there is a IN pred in the first column.
  // The subset order is the last column position(0-based) for the subset
  // i.e., consider a table t(A,B,C,D) with pk (A,B,C)
  // for A=1, the order is 0
  // for A=1 AND B=2 AND C=3, the order is 2.
  CollIndex subsetOrder;
  NABoolean hasSingleSubsetPrefix;

  if (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
    hasSingleSubsetPrefix = keyPredsByCol_.getSingleSubsetOrderForMdam(subsetOrder /*out ref*/);
  else
    hasSingleSubsetPrefix = keyPredsByCol_.getSingleSubsetOrder(subsetOrder /*out ref*/);

  if(NOT hasSingleSubsetPrefix)
    return;

  // The loops below starts at 1 because the 0th column has already been added
  for (CollIndex singleSubsetColPosition=1;
       singleSubsetColPosition <= subsetOrder;
       singleSubsetColPosition++)
  {
    disjunctHistograms_.appendHistogramForColumnPosition(singleSubsetColPosition+1);

    const ValueIdSet *predsPtr = keyPredsByCol_[singleSubsetColPosition];
    if (predsPtr AND NOT predsPtr->isEmpty())
      prefixKeyPreds_.insert(*predsPtr);
  }

  // A single subset prefix has only one positioning by definition:
  // These are the subsets that each probe is going to read
  // The subsets cannot be higher that the number of rows in the raw table
  prefixSubsets_ = MINOF(1.0, innerRowsUpperBound_.getValue());
  prefixSubsetsAsSeeks_ = prefixSubsets_;
  // Since this is a single subset, no boundary probes are necessary:
  prefixRqstsForSubsetBoundaries_ = csZero;

  prefixColumnPosition_ = subsetOrder;
}

// This function is called in a loop to scan each column for the optimal
// disjunct prefix.  Assume processLeadingColumn() is called before to
// initialize the optimal prefix.
// It has side effects on disjunctHistograms_, dijunctKeyPreds_,
// and other members that keeps track of the current column.
void MDAMOptimalDisjunctPrefixWA::processNonLeadingColumn()
{
  // Because of a VEG predicate can contain more than one
  // predicate encoded, add histograms incrementally so that
  // the reduction of a VEG predicate for a later column
  // than the current column position does not affect the
  // distribution of the current column.
  // Note that the append method receives a one-based column position,
  // so add one to the prefix because the prefix is zero based:
  disjunctHistograms_.
    appendHistogramForColumnPosition(prefixColumnPosition_+1);

  const ValueIdSet *predsPtr = keyPredsByCol_[prefixColumnPosition_];

  keyPredsPtr2Apply2Histograms_ = predsPtr;

  if( predsPtr AND NOT predsPtr->isEmpty() ) {
       // accumulate them in the pred set:
    prefixKeyPreds_.insert(*predsPtr);
  }
  // $$$ if the column is of type IN list, then
  // $$$ the uec should be equal to the IN list entries
  // $$$ times the previous UEC
  // $$$ If the hists can handle IN lists then
  // $$$ the next line is correct.
              
  MDAM_DEBUG1(MTL2, "processNonLeadingColumn(), prefixCOlumnPosition_-1: %d:", prefixColumnPosition_-1);

  const ColStats& colStats = disjunctHistograms_.getColStatsForColumn
    ( optimizer_.getIndexDesc()->getIndexKey()[prefixColumnPosition_-1] );

  MDAM_DEBUGX(MTL2, colStats.print());

  uecForPreviousCol_ = colStats.getTotalUec().getCeiling();

  MDAM_DEBUG1(MTL2, "processNonLeadingColumn(), total UEC, uecForPreviousCol_: %f:", uecForPreviousCol_.value());
      

  CostScalar estimatedUec = csOne;

  if(multiColUecInfoAvail_ AND
     uecForPreviousCol_.isGreaterThanOne() AND
     prefixColumnPosition_ > 1 AND
     disjunctHistograms_.
     estimateUecUsingMultiColUec(keyPredsByCol_, /*in*/
				 prefixColumnPosition_ - 1, /*in*/
				 estimatedUec /*out*/))
    {

      uecForPreviousCol_ = MIN_ONE(
	(uecForPreviousCol_ / uecForPreviousColBeforeAppPreds_)
	*estimatedUec);

      MDAM_DEBUG1(MTL2, "processNonLeadingColumn(), MC uec : %f:", estimatedUec.value());
      MDAM_DEBUG1(MTL2, "processNonLeadingColumn(), modify by MC uec, uecForPreviousCol_: %f:", uecForPreviousCol_.value());

      uecForPreviousColBeforeAppPreds_ = estimatedUec;
    }
  sumOfUecsSoFar_ += uecForPreviousColBeforeAppPreds_;
  uecForPrevColForSeeks_ = uecForPreviousCol_;
  // compute number of blocks per uec of previous columns. to be used when "if" condition just below
  // is true
  CostScalar uecPerBlock = uecForPreviousColBeforeAppPreds_
    / blocksToReadPerUec_;

  // what is this formula?
  // The following formula was added so that we do not count every
  // subset as a seek. For example let's say that so far we have
  // computed 200 blocks for 20 subsets then so far each subset
  // would access 10 blocks. Now if the next column has 20 uecs
  // a naive algorithm would increase the subset by a factor of
  // 20 and we would have 400 subsets equivalent to 400 seeks.
  // But in reality these 20 uecs are going to create seeks in the
  // 10 blocks so we know that it cannot create more than 10 seeks
  // we further more reduce it by the location of the column and how
  // "together" each of these seeks are.
  /*****
  if( uecForPrevColForSeeks_.isGreaterThanOne() AND
      uecPerBlock.isGreaterThanOne())
    {
      // This limits the multiplication of uec due to the new column to be no
      // more than the number of blocks per uec of previous columns.
      uecForPrevColForSeeks_ = uecPerBlock;
      // This further reduces the multiplication of uec considering the caching effects
      // of DP2 read-ahead. The higher is the column, the more effects is there.
      CostScalar reductionFactor =
	       MAXOF( (sumOfUecs_ - sumOfUecsSoFar_)/sumOfUecs_,
	               1.0/CURRSTMT_OPTDEFAULTS->readAheadMaxBlocks()
		    );

       // don't consider read ahead cache benefit, it unduly favor mdam plans 
       //uecForPrevColForSeeks_ = MIN_ONE(  uecForPrevColForSeeks_ * reductionFactor);
    }
  else if (uecForPrevColForSeeks_.isGreaterThanOne())
    {
      CostScalar reductionFactor =
	       MAXOF( (sumOfUecs_ - sumOfUecsSoFar_)/sumOfUecs_,
	               1.0/CURRSTMT_OPTDEFAULTS->readAheadMaxBlocks()
		    );

       // don't consider read ahead cache benefit, it unduly favor mdam plans 
       //uecForPrevColForSeeks_ = MIN_ONE( uecForPrevColForSeeks_ * reductionFactor);
    }
  *****/
  applyPredsToHistogram();
  computeDensityOfColumn();
  updatePositions();
  updateStatistics();
  updateMinPrefix();
}

// This function applies predicates to the disjunctHistograms_
// member. It has side effects on members
// crossProductApplied_, prefixRows_, disjunctHistograms_,
// prevPredIsEqual_, currPredIsEqual_
void MDAMOptimalDisjunctPrefixWA::applyPredsToHistogram()
{
  NABoolean getRowCount = TRUE;
  uecForPreviousColBeforeAppPreds_ = disjunctHistograms_.
    getColStatsForColumn(optimizer_.getIndexDesc()->
			 getIndexKey()[prefixColumnPosition_]).
    getTotalUec().getCeiling();
  const ValueIdSet * predsPtr = keyPredsPtr2Apply2Histograms_;
  if ( predsPtr AND
       (NOT predsPtr->isEmpty())
       )
    {
      const SelectivityHint * selHint = optimizer_.getIndexDesc()->getPrimaryTableDesc()->getSelectivityHint();
      const CardinalityHint * cardHint = optimizer_.getIndexDesc()->getPrimaryTableDesc()->getCardinalityHint();

      MDAM_DEBUG0(MTL2, "Applying predicates to disjunct histograms");
      MDAM_DEBUGX(MTL2, predsPtr->print());
      if (NOT crossProductApplied_
	  AND
	  isMultipleProbes_)
	{
	  MDAM_DEBUG0(MTL2, "Applying cross product");
	  // If this is multiple probes, we need to use joined histograms to
	  // estimate the cost factors (rows, uecs)
	  // Therefore, apply the cross product to the disjunctHistograms only once.
	  // Use the disjunctHistograms as normal after cross product is applied.
	  // ??? It seems that we need do the cross product even though there
	  // ??? is no pred on the current column. This is not being done?
	  crossProductApplied_ = TRUE;
	  if(prefixColumnPosition_ == lastColumnPosition_ - 1)
	    {
	      // If the column is the last key column, we can skip this
	      // by using the data rows already computed.
	      // ??? uec would not be affected?
	      // ??? if the join is on a non-key column, and there is no correlation
	      // ??? from the join column to the current column, this would be true
	      // ??? What about other cases.
	      getRowCount = FALSE;
	      prefixRows_ = multiProbesDataRows_;
	      MDAM_DEBUG1(MTL2, "prefixRows_ comes from outerHist: %f",
                          prefixRows_.value());
	    }else
	  {
	      disjunctHistograms_.
		applyPredicatesWhenMultipleProbes(
		  *predsPtr
		  ,*(optimizer_.getContext().getInputLogProp())
		  ,optimizer_.getRelExpr().getGroupAttr()->
		  getCharacteristicInputs()
		  ,TRUE // doing MDAM!
		  ,selHint
		  ,cardHint
		  ,NULL
		  ,REL_SCAN);
	    }
	}
      else
	disjunctHistograms_.applyPredicates(*predsPtr, (Scan &)optimizer_.getRelExpr(), selHint, cardHint, REL_SCAN);
    }

  prevPredIsEqual_ = curPredIsEqual_;

  // compute curPredIsEqual_ for the current column
  if (keyPredsByCol_.getPredicateExpressionPtr(prefixColumnPosition_) AND
      (keyPredsByCol_.getPredicateExpressionPtr(prefixColumnPosition_)->
       getType()==KeyColumns::KeyColumn::CONFLICT_EQUALS OR
       keyPredsByCol_.getPredicateExpressionPtr(prefixColumnPosition_)->
       getType()== KeyColumns::KeyColumn::EQUAL))
  {
    curPredIsEqual_ = TRUE;
  }
  else{
    curPredIsEqual_ = FALSE;
  }
  if(getRowCount){
    prefixRows_ = disjunctHistograms_.getRowCount();
    MDAM_DEBUG1(MTL2, "prefixRows_ comes from disjunctHist: %f", prefixRows_.value());
  }
}

// This function computes whether the column being scaned is dense or
// sparse. It has side effects on memebers *mdamKeyPtr_
void MDAMOptimalDisjunctPrefixWA::computeDensityOfColumn()
{
  // Sparse dense force flags:
  NABoolean sparseForced = FALSE;
  NABoolean denseForced = FALSE;

  // Check if scan is being forced
  // if so check if density is forced too
  if (scanForcePtr_ && mdamForced_)
    {
      sparseForced =
	((scanForcePtr_->getEnumAlgorithmForColumn(prefixColumnPosition_)
	  == ScanForceWildCard::COLUMN_SPARSE)
	 ? TRUE : FALSE);
      denseForced =
	((scanForcePtr_->getEnumAlgorithmForColumn(prefixColumnPosition_)
	  == ScanForceWildCard::COLUMN_DENSE)
	 ? TRUE : FALSE);
    }

  if (sparseForced OR denseForced)
    {
      if (sparseForced)
	mdamKeyPtr_->setColumnToSparse(prefixColumnPosition_);
      else if (denseForced)
	mdamKeyPtr_->setColumnToDense(prefixColumnPosition_);
    }
  else
    {
      // Sparse/dense was not forced, calculate it from histograms
      // With single col. histograms we can only do
      // a good job estimating this for the first column
      if (prefixColumnPosition_ == 0)
	{
	  // why not use firstColHistogram ?
	  const ColStats & firstColumnColStats =
	    disjunctHistograms_.
	    getColStatsForColumn(optimizer_.getIndexDesc()->getIndexKey()[0]);
	  // may want to put the threshold in the defaults table:
	  const CostScalar DENSE_THRESHOLD = 0.90;
	  const CostScalar density =
	    (firstColumnColStats.getTotalUec().getCeiling()) /
	    ( firstColumnColStats.getMaxValue().getDblValue()
	      - firstColumnColStats.getMinValue().getDblValue()
	      + 1.0 );
	  if ( density > DENSE_THRESHOLD )
	    // It is dense!!!
	    mdamKeyPtr_->setColumnToDense(prefixColumnPosition_);
	  else
	    // It is sparse!!!
	    mdamKeyPtr_->setColumnToSparse(prefixColumnPosition_);
	} // if order == 0
      else
	// order > 0, always sparse
	mdamKeyPtr_->setColumnToSparse(prefixColumnPosition_);
    } // dense/sparse not forced
}

// This function updates the subset and seek counts cost factors
// for the current prefix. It has side effects on members prefixSubsets_,
// prefixSubsetsAsSeeks_, prefixRqstsForSubsetBoundaries_
void MDAMOptimalDisjunctPrefixWA::updatePositions()
{
  // Note that there cannot be more positionings and more
  // subsets than there are rows in the table

  MDAM_DEBUG1(MTL2, "updatePositions() entered, prefixColumnPosition_: %d:", prefixColumnPosition_);

  // The positionings include probing for the next subset
  if (prefixColumnPosition_ == 0)
    {
      prefixSubsets_ =
	MINOF(innerRowsUpperBound_.getValue(),
	      uecForPreviousCol_.getValue());
      prefixSubsetsAsSeeks_ = prefixSubsets_;
      prefixRqstsForSubsetBoundaries_ = csZero;

      MDAM_DEBUG1(MTL2, "updatePositions(), prefixSubsets_ set: %f:", prefixSubsets_.value());
      MDAM_DEBUG1(MTL2, "updatePositions(), uecForPreviousCol_: %f:", uecForPreviousCol_.value());
    }
  else
    {
      // Do not add subsets for the first column
      // (i.e. we are in order 1) if we already added them
      // in a previous subset:
      if ( (prefixColumnPosition_ == 1 AND NOT firstColOverlaps_ )
	   OR
	   prefixColumnPosition_ > 1)
	{
	  // the UEC for the previous column
	  // may be zero (if the table was empty
	  // or all the rows are eliminated after
	  // preds were applied.) In this
	  // case, don't multiply:
	  if (uecForPreviousCol_.isGreaterThanZero())
	    {
              MDAM_DEBUG1(MTL2, "updatePositions(), prefixSubsets_ before change: %f:", prefixSubsets_.value());
              MDAM_DEBUG1(MTL2, "updatePositions(), uecForPreviousCol_: %f:", uecForPreviousCol_.value());

              prefixSubsets_ *= uecForPreviousCol_;
              MDAM_DEBUG1(MTL2, "updatePositions(), updated prefixSubsets_ after change: %f:", prefixSubsets_.value());

	      prefixSubsets_ =
		MINOF(innerRowsUpperBound_.getValue(),
		      prefixSubsets_.getValue());

              MDAM_DEBUG1(MTL2, "updatePositions(), updated prefixSubsets_ bounded: %f:", prefixSubsets_.value());
	    }
	  if( uecForPrevColForSeeks_.isGreaterThanZero())
	    {
	      prefixSubsetsAsSeeks_ *= uecForPrevColForSeeks_;
	      prefixSubsetsAsSeeks_ =
	        MINOF(innerRowsUpperBound_.getValue(),
	              prefixSubsetsAsSeeks_.getValue());
	    }

	  // If the previous column is sparse, then for each subset
	  // we need to make an extra probe to find the subset
	  // boundary IF we are not in the second column
	  // and the first column overlaps:
	  if  (mdamKeyPtr_->isColumnSparse(prefixColumnPosition_-1))
	    {
	      if (NOT prefixRqstsForSubsetBoundaries_.isGreaterThanZero() )
		{
		  prefixRqstsForSubsetBoundaries_ = csOne;
		}
	      prefixRqstsForSubsetBoundaries_ *= uecForPreviousCol_;
	      prefixRqstsForSubsetBoundaries_ =
		MINOF(innerRowsUpperBound_.getValue(),
		      prefixRqstsForSubsetBoundaries_.getValue());
	    }
	} // non-overlapping
    }  // prefixColumnPosition > 0
}

// This function updates cost factors for the current prefix
// It has side effects on members prefixRqsts_,
// blocksToReadPerUec_, prefixSeeks_, prefixKBRead_
void MDAMOptimalDisjunctPrefixWA::updateStatistics()
{
  // the subsets requests are issued for every probe:
  CostScalar effectiveProbes =
    incomingProbes_ - failedProbes_;
  prefixRqsts_ = prefixSubsets_ * effectiveProbes;

    // --------------------------------------------------------------
  // A successful request is equivalent to a successful probe
  // in the SearchKey case (single subset), thus we need
  // the rows per successful request, which we compute
  // below.
  // --------------------------------------------------------------

  CostScalar subsetsPerBlock = csOne;
  CostScalar rowsPerSubset = csZero;

  if( NOT prefixRqsts_.isLessThanOne() AND
       prefixRows_.isGreaterThanZero())
  {
     rowsPerSubset = prefixRows_ / prefixRqsts_;
     subsetsPerBlock = estimatedRecordsPerBlock_ / rowsPerSubset;
  } // if condition is FALSE we don't change these values

  // ------------------------------------------------------------
  // Compute seqKBRead and seeks for this disjunct
  // The i/o is computed in a per-probe basis (thus the
  // use of disjunctSubsets instead of disjunctRequests).
  // It will be scaled up to number of probes below.
  // -------------------------------------------------------------
  // Get the blocks read per probe:
  CostScalar disjunctBlocksToRead;
  computeTotalBlocksLowerBound(disjunctBlocksToRead /*out*/
			       ,prefixSubsetsAsSeeks_
			       ,rowsPerSubset
			       ,estimatedRecordsPerBlock_
			       ,innerBlocksUpperBound_);
  // prefixSubsets_ is the uec
  blocksToReadPerUec_ = MIN_ONE(disjunctBlocksToRead / prefixSubsets_);
  CostScalar beginBlocksLowerBound;
  computeBeginBlocksLowerBound(beginBlocksLowerBound /*out*/
      ,MINOF(  (prefixSubsetsAsSeeks_ /subsetsPerBlock).getCeiling(),
	       prefixSubsetsAsSeeks_.getValue()  )
      ,innerBlocksUpperBound_);

  // disjunctSubsets + prefixRqstsForSubsetBoundaries are basically
  // the number of requests to all the appropriate values.
  // Thus can be used to compute the number of index blocks
  // that will be used in for MDAM access.
  CostScalar indexBlocksLowerBound = optimizer_.getIndexDesc()->
    getEstimatedIndexBlocksLowerBound(
	  MINOF(  (  (prefixSubsets_ + prefixRqstsForSubsetBoundaries_)
		     /subsetsPerBlock).getCeiling(),
		  prefixSubsets_.getValue()  )  );
  // Assume that every disjunct does not overlap
  // with the previous. Since we computed all
  // rows to read (and not a lower bound),
  // the following routine will compute
  // the correct cost, despite its name.
  optimizer_.computeIOForFullCacheBenefit(prefixSeeks_ /* out */
      ,prefixKBRead_ /* out */
      ,beginBlocksLowerBound
      ,disjunctBlocksToRead
      ,indexBlocksLowerBound);

  // -------------------------------------------------------------
  // The total cost for the disjunct is the cost of all
  // the probes times the cost for this disjunct:
  // -------------------------------------------------------------
  NABoolean changeBack = FALSE;
  if((prefixSeeks_ - beginBlocksLowerBound).getValue()>=5)
    {
      changeBack = TRUE;
      prefixSeeks_ -= 5;
      prefixKBRead_ = prefixKBRead_ - CostScalar(5)
	* optimizer_.getIndexDesc()->getBlockSizeInKb();
    }

  prefixSeeks_ *= effectiveProbes;
  prefixKBRead_ *= effectiveProbes;

  if(changeBack)
    {
      prefixSeeks_ += 5;
      prefixKBRead_ += CostScalar(5)
	* optimizer_.getIndexDesc()->getBlockSizeInKb();
    }
}

// This function updatess the optimal prefix cost found so far
// It has side effects on members optRows_, optRqsts_,
// optRqstsForSubsetBoundaries_, optSeeks_, optSeqKBRead_, optKeyPreds_,
// stopColumn_, prevColChosen_, noExePreds_, prevPredIsEqual, curPredIsEqual_
void MDAMOptimalDisjunctPrefixWA::updateMinPrefix()
{
  SimpleCostVector prefixFR, prefixLR;
  CostScalar seqKBytesPerScan;
  Cost *scmCost = NULL;

  cumulativePrefixSubsets_ += prefixSubsets_;

  MDAM_DEBUG2(MTL2, "Disjunct: %d, Prefix Column: %d", disjunctIndex_, prefixColumnPosition_);
  MDAM_DEBUG1(MTL2, "Incoming Probes: %f:", incomingProbes_.value());
  MDAM_DEBUG1(MTL2, "Disjunct Failed Probes: %f:", failedProbes_.value());
  MDAM_DEBUG1(MTL2, "Prefix Subsets: %f:", prefixSubsets_.value());
  MDAM_DEBUG1(MTL2, "Cumulative Prefix Subsets: %f:", cumulativePrefixSubsets_.value());
  MDAM_DEBUG1(MTL2, "Prefix Requests (probes * Subsets): %f:", prefixRqsts_.value());
  MDAM_DEBUG1(MTL2, "Prefix Rows: %f:", prefixRows_.value());
  MDAM_DEBUG1(MTL2, "Prefix Seeks %f:", prefixSeeks_.value());
  MDAM_DEBUG1(MTL2, "Prefix KB Read: %f:", prefixKBRead_.value());

  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    // Factor in row sizes.
    CostScalar rowSize = optimizer_.getIndexDesc()->getRecordSizeInKb() * csOneKiloBytes;
    CostScalar outputRowSize = optimizer_.getRelExpr().getGroupAttr()->getRecordLength();

    CostScalar rowSizeFactor = optimizer_.scmRowSizeFactor(rowSize);
    CostScalar outputRowSizeFactor = optimizer_.scmRowSizeFactor(outputRowSize);

    // adding cumulativePrefixSubsets_ represents the row handling costs of the probes of
    // the MDAM algorithm as it traverses over key columns; the algorithm is recursive
    // and thus has cumulative costs
    CostScalar scmPrefixRows = (prefixRows_ + (cumulativePrefixSubsets_ * probeTax_)) * rowSizeFactor;
    CostScalar scmPrefixOutputRows = prefixRows_ * outputRowSizeFactor;

    CostScalar rowSizeFactorSeqIO = optimizer_.scmRowSizeFactor(rowSize, 
                                    ScanOptimizer::SEQ_IO_ROWSIZE_FACTOR);
    CostScalar rowSizeFactorRandIO = optimizer_.scmRowSizeFactor(rowSize,
                                    ScanOptimizer::RAND_IO_ROWSIZE_FACTOR);

    CostScalar scmPrefixIOSeq = (prefixKBRead_/optimizer_.getIndexDesc()->getBlockSizeInKb()).getCeiling() * rowSizeFactorSeqIO;
    CostScalar scmPrefixIORand = prefixSeeks_ * rowSizeFactorRandIO;



    // factor in the # of partitions (scm compares at the 
    // per-partition base)
    CostScalar numActivePartitions;
    if (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON)
      numActivePartitions  = 
        optimizer_.getEstNumActivePartitionsAtRuntimeForHbaseRegions();
     else
       numActivePartitions = optimizer_.getEstNumActivePartitionsAtRuntime();

    scmPrefixRows /= numActivePartitions;
    scmPrefixOutputRows /= numActivePartitions;
    scmPrefixIORand /= numActivePartitions;
    scmPrefixIOSeq /= numActivePartitions;

    scmCost =  
      optimizer_.scmCost(scmPrefixRows, scmPrefixOutputRows, csZero, scmPrefixIORand, 
                         scmPrefixIOSeq, incomingProbes_, rowSize, csZero, outputRowSize, csZero);

    MDAM_DEBUG1(MTL2, "  Inputs to NCM cost for prefix: incomingProbes_: %f:", incomingProbes_.value());
    MDAM_DEBUG1(MTL2, "  scmPrefixRows: %f:", scmPrefixRows.value());
    MDAM_DEBUG1(MTL2, "  scmPrefixOutputRows: %f:", scmPrefixOutputRows.value());
    MDAM_DEBUG1(MTL2, "  scmPrefixIORand: %f:", scmPrefixIORand.value());
    MDAM_DEBUG1(MTL2, "  scmPrefixIOSeq: %f:", scmPrefixIOSeq.value());

    MDAM_DEBUG1(MTL2, "NCM cost for prefix: %f:", scmCost->convertToElapsedTime(NULL).value());
  }
  else
  {
    optimizer_.computeCostVectorsForMultipleSubset
      (prefixFR /*out*/
       ,prefixLR /*out*/
       ,seqKBytesPerScan /*out unused*/
       ,prefixRows_
       ,prefixRqsts_ + prefixRqstsForSubsetBoundaries_
       ,failedProbes_
       ,prefixSeeks_
       ,prefixKBRead_
       ,prefixKeyPreds_
       ,exePreds_
       ,incomingProbes_
       ,CostScalar(prefixKeyPreds_.entries())
       );
  }

  // Does the prefix exceeds the minimum prefix cost:
  // If MDAM is forced then the user can specify
  // up to which key column predicates will be included
  // in the mdam key. If the user does not specify the
  // column and MDAM is forced, all of the key predicates
  // in the disjunct are included in the mdam key.

  NABoolean newMinimumFound = FALSE;
  NABoolean proceedViaCosting = TRUE;

  if (scanForcePtr_ && mdamForced_)
    {
      proceedViaCosting = FALSE;
      if(noExePreds_ AND
	 scanForcePtr_->getNumMdamColumns() < lastColumnPosition_)
	noExePreds_ = FALSE;
      if (prefixColumnPosition_ < scanForcePtr_->getNumMdamColumns())
	{
	  // The user wants this column as part of the mdam key,
	  // so pretend that the cost is lower when the column is added
	  newMinimumFound = TRUE;
	}
      else
	{
	  if (scanForcePtr_->getMdamColumnsStatus()
	      == ScanForceWildCard::MDAM_COLUMNS_ALL)
	    {
	      // Unconditionally force all of the columns:
	      newMinimumFound = TRUE;
	    }
	  else if (scanForcePtr_->getMdamColumnsStatus()
		   == ScanForceWildCard::MDAM_COLUMNS_NO_MORE)
	    {
	      // Unconditionally reject this and later columns:
	      newMinimumFound = FALSE;
	      prevPredIsEqual_ = FALSE; // ?
	      curPredIsEqual_ = FALSE; // ?
	    }
	  else if (scanForcePtr_->getMdamColumnsStatus()
		   ==
		   ScanForceWildCard::MDAM_COLUMNS_REST_BY_SYSTEM)
	    {
	      // Let the system decide based on costing:
	      proceedViaCosting = TRUE;
	    }
	}
    } // if scanForcePtr

  MDAM_DEBUGX(MTL2,
    MdamTrace::printBasicCost(&optimizer_, prefixFR, prefixLR, "Cost for prefix in updateMinPrefix():"));
  if (proceedViaCosting)
    {
      // Mdam has not been forced, or forced but with the choice
      // of system decision for MDAM column. proceed with costing:
      if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
      {
	DCMPASSERT(scmCost != NULL);
   
        newMinimumFound = (pMinCost_ == NULL) ? TRUE :
            (scmCost->scmCompareCosts(*pMinCost_) == LESS);
      }
      else
      {
	newMinimumFound =
	  NOT optimizer_.exceedsBound(pMinCost_,
				      prefixFR
				      ,prefixLR);
      }
    }

  // If it is the first time we go around this
  // loop update the minimum.
  // Also updated if the current minimum exceeded
  // the current prefix:

  // If the previous column is chosen and the predicate
  // on the previous column is equal predicate and there is a predicate
  // on the current column, unconditionally include the current column
  // because it is going to reduce the amount of data accessed.
  if (firstRound_
      OR newMinimumFound
      OR (prevColChosen_
	  AND keyPredsByCol_[prefixColumnPosition_]
	  AND NOT prefixKeyPreds_.isEmpty()
	  AND prevPredIsEqual_))
    {
      // We found a new minimum, initialize its data:
      firstRound_ = FALSE;
      optRows_ = prefixRows_;
      optRqsts_ = prefixRqsts_;

      MDAM_DEBUG1(MTL2, "<<<<<Update optRqsts_ as %f\n", prefixRqsts_.value());
      MDAM_DEBUG1(MTL2, "prefixColumnPosition_ =%d\n", prefixColumnPosition_);
      MDAM_DEBUG1(MTL2, "newMinimumFound=%d\n", newMinimumFound);

      optRqstsForSubsetBoundaries_ = prefixRqstsForSubsetBoundaries_;
      optSeeks_ = prefixSeeks_;
      optSeqKBRead_ = prefixKBRead_;
      optKeyPreds_.insert(prefixKeyPreds_); // is a copy more efficient?

      stopColumn_ = prefixColumnPosition_;

      prevColChosen_ = TRUE;

      delete pMinCost_;
      if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
	pMinCost_ = scmCost;
      else
	pMinCost_ = optimizer_.computeCostObject(prefixFR,prefixLR);
      DCMPASSERT(pMinCost_ != NULL);

    } // update minimum prefix
  else if (proceedViaCosting AND NOT newMinimumFound) {
    if(prefixColumnPosition_ == lastColumnPosition_ - 1)
      {//we have reached the lastcolumnPosition and its cost
	//was exceeded for the lastcolumn so
	//there will be additional exepreds
	noExePreds_ = FALSE;
      }
    else  //  This is not the last column and it wasn't chosen
      prevColChosen_ = FALSE;
  }
}


CollIndex MDAMOptimalDisjunctPrefixWA::getStopColumn() const
{ return stopColumn_; }

CollIndex MDAMOptimalDisjunctPrefixWA::getNumLeadingPartPreds() const
{ return numLeadingPartPreds_; }

const CostScalar & MDAMOptimalDisjunctPrefixWA::getOptRows() const
{ return optRows_; }

const CostScalar & MDAMOptimalDisjunctPrefixWA::getOptRqsts() const
{ return optRqsts_; }

const CostScalar & MDAMOptimalDisjunctPrefixWA::getFailedProbes() const
{ return failedProbes_; }

const CostScalar &
MDAMOptimalDisjunctPrefixWA::getOptProbesForSubsetBoundaries() const
{ return optRqstsForSubsetBoundaries_; }

const CostScalar & MDAMOptimalDisjunctPrefixWA::getOptSeeks() const
{ return optSeeks_; }

const CostScalar & MDAMOptimalDisjunctPrefixWA::getOptSeqKBRead() const
{ return optSeqKBRead_; }

const ValueIdSet & MDAMOptimalDisjunctPrefixWA::getOptKeyPreds() const
{ return optKeyPreds_; }


// ZZZZZ New MDAM costing code goes here

NewMDAMCostWA::NewMDAMCostWA
(FileScanOptimizer & optimizer,
 NABoolean mdamForced,
 MdamKey *mdamKeyPtr,
 const Cost *costBoundPtr,
 const ValueIdSet & exePreds,
 const CostScalar & singleSubsetSize) :
   mdamWon_(FALSE)
  ,noExePreds_(TRUE)
  ,numKBytes_(0)
  ,scmCost_(NULL)
  ,mdamForced_(mdamForced)
  ,mdamKeyPtr_(mdamKeyPtr)
  ,costBoundPtr_(costBoundPtr)
  ,optimizer_(optimizer)
  ,singleSubsetSize_(singleSubsetSize)
  ,isMultipleScanProbes_(optimizer.isMultipleProbes())
  ,incomingScanProbes_(optimizer.getIncomingProbes())
  ,disjunctMdamOK_(FALSE)

  ,innerRowsUpperBound_( optimizer.getRawInnerHistograms().
			 getRowCount().getCeiling() )
  ,innerBlocksUpperBound_(0) // initialized below
  ,estimatedRowsPerBlock_( optimizer.getIndexDesc()->
			      getEstimatedRecordsPerBlock() )
  ,disjunctIndex_(0)

  ,exePreds_(exePreds) //optimizer.getRelExpr().getSelectionPred())
  ,scanForcePtr_(optimizer.findScanForceWildCard())
  ,outerHistograms_(optimizer.getContext().getInputLogProp()->getColStats())
{
  // Compute inner blocks upper bound
  computeBlocksUpperBound(innerBlocksUpperBound_ /* out*/
    ,innerRowsUpperBound_ /* in */
    ,estimatedRowsPerBlock_ /*in*/);
}


// This function computes whether MDAM wins over the cost bound
// It sums up cost factors for all disjuncts, calculates the cost
// and compares with the cost bound.
void NewMDAMCostWA::compute()
{
  if (isMultipleScanProbes_)
  {
    MDAM_DEBUG0(MTL2, "Mdam scan is multiple probes");
  }
  else 
  {
    incomingScanProbes_ = 1; // TODO: why is this necessary?
    MDAM_DEBUG0(MTL2, "Mdam scan is a single probe");
  }

  // the next few variables are sums over the set of disjuncts; all of these
  // are actually upper bounds
  CostScalar sumMDAMFetchRows;    // sum of rows fetched by MDAM key predicates
  CostScalar sumMDAMProbeRows;    // sum of rows returned on MDAM probes
  CostScalar sumMDAMFetchSubsets; // sum of MDAM fetch subsets
  CostScalar sumMDAMProbeSubsets; // sum of MDAM probe subsets

  CostScalar totalSeqKBRead;

  // -----------------------------------------------------------------------
  //  Loop through every disjunct and:
  //   1 Find the optimal disjunct prefix for the disjunct
  //   2 Compute the sum of the metrics of all disjuncts so far
  // -----------------------------------------------------------------------
  for (CollIndex disjunctIndex=0;
       disjunctIndex < mdamKeyPtr_->getKeyDisjunctEntries();
       disjunctIndex++)
    {
      // 1 find optimal prefix for disjunct
      disjunctIndex_ = disjunctIndex;
      computeDisjunct();
      if(NOT disjunctMdamOK_)
        {
          mdamWon_ = FALSE;
          MDAM_DEBUG0(MTL2, "Mdam scan lost because disjunctMdamOK_ is false");
          return;
        }

      // 2 Compute the sum of the costs of the disjuncts seen so far

      // Add in counts from the disjunct just processed
      sumMDAMFetchRows += disjunctOptMDAMFetchRows_;
      sumMDAMProbeRows += disjunctOptMDAMProbeRows_;
      sumMDAMFetchSubsets += disjunctOptMDAMFetchSubsets_;
      sumMDAMProbeSubsets += disjunctOptMDAMProbeSubsets_;
    } // for every disjunct

  // Now compute the cost and see if MDAM wins or loses.

  // Note: The older version of this code computed this cost within
  // the disjunct loop in hopes of taking an early out if the cost
  // bound was exceeded. We don't do that here, because the I/O cost
  // is not additive. Adding disjuncts can actually lower the I/O
  // cost by increasing the amount of sequential I/O and lowering
  // the amount of disk seeks.  

  // Worst case for sequential I/O is that we read all the blocks that a single
  // subset scan would
  CostScalar seqBlocksReadUpperBound = 
    (singleSubsetSize_/optimizer_.getIndexDesc()->getEstimatedRecordsPerBlock()).getCeiling(); 

  if (optimizer_.getIndexDesc()->getPrimaryTableDesc()->getNATable()->isHbaseTable())
  {
    CostScalar totRowsProcessed = sumMDAMFetchRows + sumMDAMProbeRows;

    CostScalar avgMDAMFetchSubsetSize = sumMDAMFetchRows / sumMDAMFetchSubsets;
    CostScalar avgMDAMFetchBlocks = 
      (avgMDAMFetchSubsetSize/optimizer_.getIndexDesc()->getEstimatedRecordsPerBlock()).getCeiling(); 
    CostScalar seqBlocksFetchUpperBound = avgMDAMFetchBlocks * sumMDAMFetchSubsets;
    CostScalar seqBlocksProbeUpperBound = sumMDAMProbeSubsets;
    CostScalar refinedSeqBlocksReadUpperBound = MINOF(seqBlocksReadUpperBound,
      seqBlocksFetchUpperBound + seqBlocksProbeUpperBound);

    CostScalar seeks = csZero;
    if (refinedSeqBlocksReadUpperBound < seqBlocksReadUpperBound)
      {
        // Here we know that in the worst case, we won't touch all the blocks 
        // that a single subset scan will. Let's assume worst case placement
        // of the blocks we do touch -- that is, spread them out as much
        // as possible. The number of gaps is the number of seeks.
        CostScalar maxNumberOfChunks = MINOF(refinedSeqBlocksReadUpperBound,
          seqBlocksReadUpperBound - refinedSeqBlocksReadUpperBound);

        // We don't count a seek for the first block. (Single subset scan
        // costing doesn't count it either. So this makes it apples-to-apples.)
        seeks = maxNumberOfChunks - csOne;
      }

    // Add into the seeks some overhead per subset. Not every subset will cause
    // a seek, but there is some overhead per subset. (We subtract one, because
    // simple scans don't charge for their one subset. That makes the comparison
    // more applies-to-apples.)

    CostScalar totalSubsets = sumMDAMProbeSubsets + sumMDAMFetchSubsets - csOne;
    CostScalar MDAMSubsetAdjustmentFactor = ActiveSchemaDB()->getDefaults().getAsDouble(MDAM_SUBSET_FACTOR);
    totalSubsets *= MDAMSubsetAdjustmentFactor;
    seeks += totalSubsets;

    CostScalar rowSize = optimizer_.getIndexDesc()->getRecordSizeInKb() * csOneKiloBytes;
    CostScalar rowSizeFactor = optimizer_.scmRowSizeFactor(rowSize);
    totRowsProcessed *= rowSizeFactor;

    CostScalar randIORowSizeFactor = 
      optimizer_.scmRowSizeFactor(rowSize, ScanOptimizer::RAND_IO_ROWSIZE_FACTOR);
    seeks *= randIORowSizeFactor;

    CostScalar seqIORowSizeFactor = 
      optimizer_.scmRowSizeFactor(rowSize, ScanOptimizer::SEQ_IO_ROWSIZE_FACTOR);
    refinedSeqBlocksReadUpperBound *= seqIORowSizeFactor;

    CostScalar totalSeqKBRead = refinedSeqBlocksReadUpperBound *
      optimizer_.getIndexDesc()->getBlockSizeInKb();

    scmCost_ = optimizer_.scmComputeMDAMCostForHbase(totRowsProcessed, seeks, 
                                                     totalSeqKBRead, incomingScanProbes_);
  }
  else // Not an HBase table
  {
    // None of the other storage engines we support currently have a direct access structure
    // so MDAM doesn't make sense
    disjunctMdamOK_ = FALSE;
    mdamWon_ = FALSE;
    MDAM_DEBUG0(MTL2, "Mdam scan lost because its not an HBase table");
    return;
  }

  // scale up mdam cost by factor of NCM_MDAM_COST_ADJ_FACTOR --default is 1
  CostScalar costAdj = (ActiveSchemaDB()->getDefaults()).getAsDouble(NCM_MDAM_COST_ADJ_FACTOR);
  scmCost_->cpScmlr().scaleByValue(costAdj);

  // If the cost exceeds the bound, MDAM loses

  if (costBoundPtr_ != NULL &&
      costBoundPtr_->scmCompareCosts(*scmCost_) == LESS)
  {
    mdamWon_ = FALSE;
    MDAM_DEBUG0(MTL2, "Mdam scan lost due to higher cost determined by scmCompareCosts()");
    return;
  }

  // update rows accessed
  optimizer_.setEstRowsAccessed(sumMDAMFetchRows + sumMDAMProbeRows);
  mdamWon_ = TRUE;
  numKBytes_ = 0; // seqKBytesPerScan; // TODO: where does this come from?
}

// This function computes the cost factors of a MDAM disjunct
// It computes the members below:
//   disjunctMdamOK_
//   disjunctOptMDAMFetchRows_
//   disjunctOptMDAMProbeRows_
//   disjunctOptMDAMFetchSubsets_
//   disjunctOptMDAMProbeSubsets_
//   noExePreds_
// It also side effects member mdamKeyPtr_
void NewMDAMCostWA::computeDisjunct()
{
  MDAM_DEBUG1(MTL2, "NewMDAMCostWA::computeDisjunct processing disjunct %d\n",disjunctIndex_);
 
  // get the key preds for this disjunct:
  NABoolean allKeyPredicates = FALSE;
  ValueIdSet disjunctKeyPreds;
  mdamKeyPtr_->getKeyPredicates(disjunctKeyPreds /*out*/,
			       &allKeyPredicates /*out*/,
			       disjunctIndex_ /*in*/);
  if( NOT (allKeyPredicates) )
    noExePreds_ = FALSE;


  // return with a NULL cost if there are no key predicates
  // "costBoundPtr_ == NULL" means "MDAM is forced"
  if (disjunctKeyPreds.isEmpty() AND costBoundPtr_ != NULL)
    {
      MDAM_DEBUG0(MTL2, "NewMDAMCostWA::computeDisjunct(): disjunctKeyPreds is empty"); 
      disjunctMdamOK_ = FALSE;
      return; // full table scan, MDAM is worthless here
    }

  // Tabulate the key predicates using the key columns as the index
  ColumnOrderList keyPredsByCol(optimizer_.getIndexDesc()->getIndexKey());
  mdamKeyPtr_->getKeyPredicatesByColumn(keyPredsByCol,disjunctIndex_);

  MDAM_DEBUG1(MTL2, "Disjunct: %d, keyPredsByCol:", disjunctIndex_);
  MDAM_DEBUGX(MTL2, keyPredsByCol.print());
  MDAM_DEBUG0(MTL2, "disjunctKeyPreds no recomputing");
  MDAM_DEBUGX(MTL2, disjunctKeyPreds.display());

  // compute the optimal prefix and the associated min cost
  NewMDAMOptimalDisjunctPrefixWA prefixWA(optimizer_,
				       keyPredsByCol,
				       disjunctKeyPreds,
				       exePreds_,
				       outerHistograms_,
				       mdamKeyPtr_,
				       scanForcePtr_,
				       mdamForced_,
				       isMultipleScanProbes_,
				       incomingScanProbes_,
				       estimatedRowsPerBlock_,
				       innerRowsUpperBound_,
				       innerBlocksUpperBound_,
                                       singleSubsetSize_,
				       disjunctIndex_);

  prefixWA.compute(noExePreds_ /*out */);
  CollIndex stopColumn = prefixWA.getStopColumn();
  disjunctOptMDAMFetchRows_ = prefixWA.getOptMDAMFetchRows();
  disjunctOptMDAMProbeRows_ = prefixWA.getOptMDAMProbeRows();
  disjunctOptMDAMFetchSubsets_ = prefixWA.getOptMDAMFetchSubsets();
  disjunctOptMDAMProbeSubsets_ = prefixWA.getOptMDAMProbeSubsets();

  // Set the stop column for current disjunct:
  mdamKeyPtr_->setStopColumn(disjunctIndex_, stopColumn);

  NABoolean mdamMakeSense = TRUE;
  if (NOT mdamForced_  AND stopColumn == 0) {
    if (keyPredsByCol.getPredicateExpressionPtr(0) == NULL )
      mdamMakeSense = FALSE;
    else if (mdamKeyPtr_->getKeyDisjunctEntries() == 1) 
      {
        // When there is a conflict in single subset, mdam should handle it
        const ColumnOrderList::KeyColumn* currKeyColumn =
          keyPredsByCol.getPredicateExpressionPtr(0);

        NABoolean conflict =
          (  (currKeyColumn->getType() == KeyColumns::KeyColumn:: CONFLICT)
             OR
             (currKeyColumn->getType() ==
              KeyColumns::KeyColumn::CONFLICT_EQUALS)   
             OR
             // added for patching (since it is a single disjunct now, but not a conflict)
             // where a =10 or a=20 or a=30
             (currKeyColumn->getType() ==
              KeyColumns::KeyColumn::INLIST) );
      
      if( NOT conflict ) 
          { 
            // single subset should be chosen.
            MDAM_DEBUG0(MTL2, "NewMDAMCostWA::computeDisjunct(): conflict predicate for single subset, force MDAM off"); 
            mdamMakeSense = FALSE;
          }
      }
  }

  CollIndex noOfmissingKeyColumnsTot = 0;
  CollIndex presentKeyColumnsTot = 0;


  const IndexDesc *idesc = optimizer_.getFileScan().getIndexDesc();
  const ColStatDescList& csdl = idesc->getPrimaryTableDesc()->getTableColStats();
  Histograms hist(csdl);
	 
  Lng32 checkOption = (ActiveSchemaDB()->getDefaults()).getAsLong(MDAM_APPLY_RESTRICTION_CHECK);

  if(CURRSTMT_OPTDEFAULTS->indexEliminationLevel() != OptDefaults::MINIMUM
     && (!mdamForced_)
	 && (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
	 && checkOption >= 1 
	 && (!checkMDAMadditionalRestriction(
                                    keyPredsByCol,
                                    optimizer_.computeLastKeyColumnOfDisjunct(keyPredsByCol),
                                    hist,
                                    (restrictCheckStrategy)checkOption,
                                    noOfmissingKeyColumnsTot,
                                    presentKeyColumnsTot))
   )
  {
        
    MDAM_DEBUG0(MTL2, "NewMDAMCostWA::computeDisjunct(): MDAM additional restriction check failed"); 
    mdamMakeSense = FALSE;
  }
  disjunctMdamOK_ = mdamMakeSense; 
}

NABoolean NewMDAMCostWA::isMdamWon() const
{
  return mdamWon_;
}

NABoolean NewMDAMCostWA::hasNoExePreds() const
{
  return noExePreds_;
}

const CostScalar & NewMDAMCostWA::getNumKBytes() const
{
  return numKBytes_;
}

Cost * NewMDAMCostWA::getScmCost()
{
  return scmCost_;
}


// Implementation of NewMDAMOptimalDisjunctPrefixWA methods

NewMDAMOptimalDisjunctPrefixWA::NewMDAMOptimalDisjunctPrefixWA
(FileScanOptimizer & optimizer,
 const ColumnOrderList & keyPredsByCol,
 const ValueIdSet & disjunctKeyPreds,
 const ValueIdSet & exePreds,
 const Histograms & outerHistograms,
 MdamKey *mdamKeyPtr,
 const ScanForceWildCard *scanForcePtr,
 NABoolean mdamForced,
 NABoolean isMultipleProbes,
 const CostScalar & incomingScanProbes,
 const CostScalar & estimatedRecordsPerBlock,
 const CostScalar & innerRowsUpperBound,
 const CostScalar & innerBlocksUpperBound,
 const CostScalar & singleSubsetSize,
 CollIndex disjunctIndex)

 :
   optMDAMFetchRows_(0)
  ,optMDAMFetchSubsets_(0)
  ,optMDAMProbeRows_(0)
  ,optMDAMProbeSubsets_(0)
  ,optStopColumn_(0)
  ,optimizer_(optimizer)
  ,keyPredsByCol_(keyPredsByCol)
  ,disjunctKeyPreds_(disjunctKeyPreds)
  ,exePreds_(exePreds)
  ,outerHistograms_(outerHistograms)
  ,scanForcePtr_(scanForcePtr)
  ,isMultipleProbes_(isMultipleProbes)
  ,mdamForced_(mdamForced)
  ,incomingScanProbes_(incomingScanProbes)
  ,estimatedRecordsPerBlock_(estimatedRecordsPerBlock)
  ,innerRowsUpperBound_(innerRowsUpperBound)
  ,innerBlocksUpperBound_(innerBlocksUpperBound)
  ,singleSubsetSize_(singleSubsetSize)
  ,disjunctIndex_(disjunctIndex)
  ,mdamKeyPtr_(mdamKeyPtr)
  ,disjunctHistograms_( *(optimizer.getIndexDesc()), 0 )
  ,optimalCost_(NULL)
  ,crossProductApplied_(FALSE)
  ,multiColUecInfoAvail_(disjunctHistograms_.isMultiColUecInfoAvail())
  ,MCUECOfPriorPrefixFound_(FALSE)
  ,MCUECOfPriorPrefix_(csZero)
{}



NewMDAMOptimalDisjunctPrefixWA::~NewMDAMOptimalDisjunctPrefixWA()
{
  if(optimalCost_)
    delete optimalCost_;
}


// This function computes the optimal prefix of the disjunct

// If we decide that the optimal prefix does not use all of the
// key predicates, then the output parameter noExePreds will be
// set to FALSE. Otherwise we leave it untouched.

void NewMDAMOptimalDisjunctPrefixWA::compute(NABoolean & noExePreds /*out*/)
{
  // Cumulative probe counters (MDAM is a recursive algorithm; this
  // represents the cost of recursion)

  CostScalar cumulativeMDAMProbeRows;
  CostScalar cumulativeMDAMProbeSubsets;

  CostScalar previousColumnMDAMProbeRows(csOne);

  const ValueIdList & keyColumns = optimizer_.getIndexDesc()->getIndexKey();
  for (CollIndex i = 0; i < keyColumns.entries(); i++)
  {
    MDAM_DEBUG1(MTL2,"New Prefix compute, exploring column %d\n",i);

    // Apply key predicates for that column to the histograms

    // Because of a VEG predicate can contain more than one
    // predicate encoded, add histograms incrementally so that
    // the reduction of a VEG predicate for a later column
    // than the current column position does not affect the
    // distribution of the current column.
    // Note that the append method receives a one-based column position,
    // so add one to the prefix because the prefix is zero based.

    disjunctHistograms_.appendHistogramForColumnPosition(i+1);
    CostScalar ColumnUECBeforePreds = disjunctHistograms_.getColStatsForColumn(
                   optimizer_.getIndexDesc()->
                   getIndexKey()[i]).getTotalUec().getCeiling();
    MDAM_DEBUG1(MTL2,"Column UEC from histograms before applying key predicates: %f",ColumnUECBeforePreds.value());

    const ValueIdSet * predsPtr = keyPredsByCol_[i];
    applyPredsToHistogram(predsPtr);
    CostScalar columnUEC = disjunctHistograms_.getColStatsForColumn(
                   optimizer_.getIndexDesc()->
                   getIndexKey()[i]).getTotalUec().getCeiling();
    MDAM_DEBUG1(MTL2,"Column UEC from histograms: %f",columnUEC.value());

    // Determine if column is dense or sparse and set it accordingly

    NABoolean isDense = isColumnDense(i);
    if (isDense)
      mdamKeyPtr_->setColumnToDense(i);
    else
      mdamKeyPtr_->setColumnToSparse(i);

    // Analyze the key predicates
    
    // We need to estimate how many
    // MDAM intervals will result from them (as that determines
    // subset counts), and also we need to know the estimated 
    // upper bound on the UEC of the key column. (Example: If we
    // have a range predicate of the form A > ? AND A < ?, we 
    // estimate the selectivity as the square of the default
    // selectivity for a ">" predicate. But when this is applied
    // to histograms, it does not affect the histogram UEC
    // estimate, only its cardinality estimate.)

    CostScalar MDAMIntervalEstimate(csOne);
    CostScalar MDAMUECEstimate(columnUEC);

    if (predsPtr AND (NOT predsPtr->isEmpty()))
    {
      calculateMetricsFromKeyPreds(predsPtr, ColumnUECBeforePreds,
        MDAMUECEstimate /*out*/, MDAMIntervalEstimate /*out*/);
    
      MDAM_DEBUG1(MTL2,"Column UEC from predicates: %f",MDAMUECEstimate.value());
      MDAM_DEBUG1(MTL2,"Interval estimate from predicates: %f",MDAMIntervalEstimate.value());
    }

    if (MDAMUECEstimate < columnUEC)
      columnUEC = MDAMUECEstimate;

    MDAM_DEBUG1(MTL2,"Minimal column UEC: %f",columnUEC.value());

    // Calculate bound on column UEC from multicolumn histograms

    // For example, it might be the case that there is a functional dependency
    // within the set of key columns. If A, B, C, D has the same UEC as A, B, C,
    // then we know that traversing to D results in at most one MDAM probe.
    // So, we can use the ratio of the UEC of A, B, C, D divided by that of
    // A, B, C as an upper bound on column UEC. Among others, this scenario
    // happens when we have a "_SALT_" column or a "_DIVISION_n_" column.

    if (multiColUecInfoAvail_)
      {
        // obtain the UEC of this prefix 
        CostScalar UECFromMCHistograms = csOne;
        NABoolean UECFromMCFound = disjunctHistograms_.
          estimateUecUsingMultiColUec(keyPredsByCol_, /*in*/
                                      i, /*in*/
                                      UECFromMCHistograms /*out*/);
        if (UECFromMCFound)
          {
            MDAM_DEBUG2(MTL2, "Column UEC of prefix %d from MC histograms: %f:", 
              i, UECFromMCHistograms.value());

            if (MCUECOfPriorPrefixFound_)
              {  
                // we have UEC of this prefix and the prior prefix, so we can
                // compute their ratio
                CostScalar columnUECFromMCUECRatio = UECFromMCHistograms / MCUECOfPriorPrefix_;

                MDAM_DEBUG1(MTL2, "Ratio of prefix UEC to prior prefix UEC: %f:",
                  columnUECFromMCUECRatio.value());

                if (columnUECFromMCUECRatio < columnUEC)
                  columnUEC = columnUECFromMCUECRatio;

                MDAM_DEBUG1(MTL2, "Minimal column UEC: %f", columnUEC.value()); 
              }

            MCUECOfPriorPrefix_ = UECFromMCHistograms; // save for next column          
          }        

        MCUECOfPriorPrefixFound_ = UECFromMCFound;                
      }      

    // Calculate the number of probe rows and probe subsets for that column

    // Each probe subset returns at most one row; there will be exactly one
    // probe subset that returns no row for each MDAM interval. Note that
    // for dense columns, we don't do a scan for the probe; we just increment
    // the last value we used, so MDAMProbeSubsets is zero in that case.
 
    CostScalar MDAMProbeRows = previousColumnMDAMProbeRows * columnUEC;
    CostScalar MDAMProbeSubsets =
      isDense ? csZero : previousColumnMDAMProbeRows * (columnUEC + MDAMIntervalEstimate);

    MDAM_DEBUG1(MTL2,"MDAM Probe Rows on this column: %f",MDAMProbeRows.value());
    MDAM_DEBUG1(MTL2,"MDAM Probe Subsets on this column: %f",MDAMProbeSubsets.value());
    

    // Calculate the number of fetch rows and fetch subsets for that column

    CostScalar MDAMFetchRows = disjunctHistograms_.getColStatsForColumn(
                   optimizer_.getIndexDesc()->
                   getIndexKey()[i]).getRowcount().getCeiling();
    CostScalar MDAMFetchSubsets = previousColumnMDAMProbeRows * MDAMIntervalEstimate;

    MDAM_DEBUG1(MTL2,"MDAM Fetch Rows on this column: %f",MDAMFetchRows.value());
    MDAM_DEBUG1(MTL2,"MDAM Fetch Subsets on this column: %f",MDAMFetchSubsets.value());

    // Calculate cost assuming this column is the stop column

    // A subset does not necessarily cause a disk seek. And disk seeks in
    // general can't be predicted here as they are not additive (the block we
    // need next might be the current block, or might be sequential after this
    // one). So we don't try to estimate disk seeks here. Nor do we try to
    // estimate sequential I/O here. 

    // However a subset does cause some positioning overhead in the Region
    // Server apart from disk activity, as it involves traversal of index
    // blocks (perhaps in memory). So, we include some cost in the
    // totalSeeks param here.

    CostScalar totalRowsProcessed = cumulativeMDAMProbeRows + MDAMFetchRows;

    CostScalar totalSeeks = cumulativeMDAMProbeSubsets + MDAMFetchSubsets;
    CostScalar MDAMSubsetAdjustmentFactor = ActiveSchemaDB()->getDefaults().getAsDouble(MDAM_SUBSET_FACTOR);
    totalSeeks *= MDAMSubsetAdjustmentFactor;

    CostScalar totalSeqKBRead = csZero;  // not estimated here
    Cost * currentCost = optimizer_.scmComputeMDAMCostForHbase(totalRowsProcessed, totalSeeks, 
                                                           totalSeqKBRead, incomingScanProbes_);   

    // If the cost is a new minimum (or if this is the lead column), capture it

    NABoolean forced = FALSE;
    if (isMinimalCost(currentCost,i,forced /* out */))
    //if ((optimalCost_ == NULL) OR (currentCost->compareCosts(*optimalCost_) == LESS))
      {
        // Capture new minimums
        if (optimalCost_)
          delete optimalCost_;

        optMDAMFetchRows_ = MDAMFetchRows;
        optMDAMFetchSubsets_ = MDAMFetchSubsets;
        optMDAMProbeRows_ = cumulativeMDAMProbeRows;
        optMDAMProbeSubsets_ = cumulativeMDAMProbeSubsets;

        optimalCost_ = currentCost;
        optStopColumn_ = i;

        MDAM_DEBUG1(MTL2,"Column %d has optimal cost so far for this disjunct:",i);
        MDAM_DEBUG1(MTL2,"  Optimal MDAM Probe Rows: %f",optMDAMProbeRows_.value());
        MDAM_DEBUG1(MTL2,"  Optimal MDAM Probe Subsets: %f",optMDAMProbeSubsets_.value());
        MDAM_DEBUG1(MTL2,"  Optimal MDAM Fetch Rows: %f",optMDAMFetchRows_.value());
        MDAM_DEBUG1(MTL2,"  Optimal MDAM Fetch Subsets: %f",optMDAMFetchSubsets_.value());
        MDAM_DEBUGX(MTL2,MdamTrace::printCostObject(&optimizer_,optimalCost_,"Optimal cost"));
      }
    else
      {
        MDAM_DEBUGX(MTL2,MdamTrace::printCostObject(&optimizer_,currentCost,"Not optimal cost"));
        delete currentCost;

        // if this is the last column and it is not optimal, indicate that there
        // are some key predicates that must be generated as executor predicates
        if (i == keyColumns.entries() - 1)
          noExePreds = FALSE;
    }

    if (forced)
      {
        MDAM_DEBUG0(MTL2,"  (The choice above was forced.)");
      }

    // Update cumulative counters for the next column

    cumulativeMDAMProbeRows += MDAMProbeRows;
    cumulativeMDAMProbeSubsets += MDAMProbeSubsets;
    previousColumnMDAMProbeRows = MDAMProbeRows; 

    MDAM_DEBUG1(MTL2,"Cumulative MDAM Probe Rows including this column: %f",cumulativeMDAMProbeRows.value());
    MDAM_DEBUG1(MTL2,"Cumulative MDAM Probe Subsets including this column: %f\n",cumulativeMDAMProbeSubsets.value()); 
  }
}


// This function applies predicates to the disjunctHistograms_
// member. It has side effects on members
// crossProductApplied_ and disjunctHistograms_
void NewMDAMOptimalDisjunctPrefixWA::applyPredsToHistogram(const ValueIdSet * predsPtr)
{
  if ( predsPtr AND
       (NOT predsPtr->isEmpty())
       )
    {
      const SelectivityHint * selHint = optimizer_.getIndexDesc()->getPrimaryTableDesc()->getSelectivityHint();
      const CardinalityHint * cardHint = optimizer_.getIndexDesc()->getPrimaryTableDesc()->getCardinalityHint();

      MDAM_DEBUG0(MTL2, "Applying predicates to disjunct histograms");
      MDAM_DEBUGX(MTL2, predsPtr->print());
      if (NOT crossProductApplied_
          AND
          isMultipleProbes_)
        {
          MDAM_DEBUG0(MTL2, "Applying cross product");
          // If this is multiple probes, we need to use joined histograms to
          // estimate the cost factors (rows, uecs)
          // Therefore, apply the cross product to the disjunctHistograms only once.
          // Use the disjunctHistograms as normal after cross product is applied.
          // ??? It seems that we need do the cross product even though there
          // ??? is no pred on the current column. This is not being done?
          crossProductApplied_ = TRUE;
          disjunctHistograms_.
                applyPredicatesWhenMultipleProbes(
                  *predsPtr
                  ,*(optimizer_.getContext().getInputLogProp())
                  ,optimizer_.getRelExpr().getGroupAttr()->
                  getCharacteristicInputs()
                  ,TRUE // doing MDAM!
                  ,selHint
                  ,cardHint
                  ,NULL
                  ,REL_SCAN);
        }
      else
        disjunctHistograms_.applyPredicates(*predsPtr, (Scan &)optimizer_.getRelExpr(), selHint, cardHint, REL_SCAN);
    }
}

// This function computes whether the column being scaned is dense or
// sparse. Returns TRUE if dense, FALSE if sparse.
NABoolean NewMDAMOptimalDisjunctPrefixWA::isColumnDense(CollIndex columnPosition)
{
  NABoolean rc = FALSE;  // assume column is sparse
  ScanForceWildCard::scanOptionEnum forceOption = ScanForceWildCard::UNDEFINED;

  // Check if scan is being forced
  // if so check if density is forced too
  if (scanForcePtr_ && mdamForced_)
    forceOption = scanForcePtr_->getEnumAlgorithmForColumn(columnPosition);

  if (forceOption == ScanForceWildCard::COLUMN_DENSE)
    {
      rc = TRUE;
      MDAM_DEBUG0(MTL2,"Column is dense (forced)");
    }
  else if (forceOption == ScanForceWildCard::COLUMN_SPARSE)
    {
      rc = FALSE;
      MDAM_DEBUG0(MTL2,"Column is sparse (forced)");
    }
  else
    {
      // Sparse/dense was not forced, calculate it from histograms
      // With single column histograms we can only do
      // a good job estimating this for the first column
      if (columnPosition == 0)
	{
	  // why not use firstColHistogram ?
	  const ColStats & firstColumnColStats =
	    disjunctHistograms_.
	    getColStatsForColumn(optimizer_.getIndexDesc()->getIndexKey()[0]);
	  // may want to put the threshold in the defaults table:
	  const CostScalar DENSE_THRESHOLD = 0.90;
	  const CostScalar density =
	    (firstColumnColStats.getTotalUec().getCeiling()) /
	    ( firstColumnColStats.getMaxValue().getDblValue()
	      - firstColumnColStats.getMinValue().getDblValue()
	      + 1.0 );
	  if ( density > DENSE_THRESHOLD )
            {
              // It is dense!!!
              rc = TRUE;
              MDAM_DEBUG0(MTL2,"Column is dense (from histogram)");
            }
          else
            {
              // It is sparse!!!
              rc = FALSE;
              MDAM_DEBUG0(MTL2,"Column is sparse (from histogram)");
            }
        } // if columnPosition == 0
      else
        {
          // columnPosition > 0, always sparse
          rc = FALSE;
          MDAM_DEBUG0(MTL2,"Column is sparse (non-leading column)");
        }
    } // dense/sparse not forced

  return rc;   
}


void NewMDAMOptimalDisjunctPrefixWA::calculateMetricsFromKeyPreds(const ValueIdSet * predsPtr, 
  const CostScalar & maxUEC, CostScalar & UECFromPreds, CostScalar & IntervalCountFromPreds)
{
  // If the key predicates for a column are all of the form
  // column op constant (or column IS NULL), then the UEC of
  // the result after applying key predicates can be read from
  // the histogram. If even some of the key predicates are 
  // of this form, the UEC of the resulting histogram will
  // still be a reasonable bound.

  // But if there are key predicates, and none has a constant,
  // then the row count of the histogram will be reduced but
  // not the UEC. This is a problem for MDAM costing because
  // we depend on the (true) UEC to determine the number of
  // MDAM probes for the column.

  // An example of this situation is a predicate X = ? on a
  // column with UEC 1 million. We know that after X = ? is
  // applied, the true UEC will be 1. However, the histogram
  // will show reduced row counts but a UEC of 1,000,000.

  // It's quite reasonable for the histogram code to behave
  // this way, actually. Its foremost goal is to predict
  // cardinalities. And scaling down the row count while
  // retaining the 1 million UEC models the column after
  // applying X = ? as a probability distribution.

  // So, this method attempts to approximate the true UEC
  // for such predicates by looking at the predicates
  // themselves.

  // For MDAM costing, we prefer to overestimate this UEC
  // rather than underestimate (as MDAM performance degrades
  // poorly when we underestimate). So, for example, when
  // estimating an OR, we don't try to account for a possible
  // non-empty intersection of values but instead just add
  // the UECs. That is a reasonable upper bound without it
  // being grossly overestimated.

  // A fine point concerns the handling of range predicates.
  // The default selectivity for A > ? is 1/3 (well, it's 
  // the value of CQD HIST_DEFAULT_SEL_FOR_PRED_RANGE).
  // In a uniform distribution, this would translate into
  // a UEC of 1/3 the original. For simplicity, this is the
  // calculation we use. It's not quite right though. If we
  // have a distribution that is skewed to the left or right
  // (e.g. an exponential distribution), it would be more
  // precise to find the point in the histogram where the
  // *row count* to the right or left is 1/3 of the total,
  // then compute the UEC of that subset of the histogram.
  // We'll leave that complexity to a future improvement
  // if and when it seems needed.

  int lessCount = 0;
  int greaterCount = 0;

  ValueId vid = predsPtr->init();
  predsPtr->next(vid);  // expect it to return true, as caller insured predsPtr was not empty
  calculateMetricsFromKeyPred(vid, maxUEC,
                              UECFromPreds /*out*/, IntervalCountFromPreds /*out*/,
                              lessCount /*in/out*/, greaterCount /*in/out*/);
  predsPtr->advance(vid);

  while (predsPtr->next(vid))
    {      
      CostScalar UECFromPreds1;
      CostScalar IntervalCountFromPreds1;
      calculateMetricsFromKeyPred(vid, maxUEC,
                                  UECFromPreds1 /*out*/, IntervalCountFromPreds1 /*out*/,
                                  lessCount /*in/out*/, greaterCount /*in/out*/);
      // this predicate is ANDed with the previous; so the
      // UEC is at most the min of the two (but see below for
      // special handling of the case A > ? AND A < ?)
      UECFromPreds = MINOF(UECFromPreds,UECFromPreds1);
      predsPtr->advance(vid);
    }

  if (UECFromPreds > maxUEC)
    UECFromPreds = maxUEC;

  // special handling if both A < ? and A > ? present
  if ((lessCount > 0) && (greaterCount > 0)) 
    {
      CostScalar selectionFactor = 
        CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_PRED_RANGE);
      CostScalar andedRange = maxUEC * selectionFactor * selectionFactor;
      if (andedRange < UECFromPreds)
        UECFromPreds = andedRange;
      if (UECFromPreds < csOne)
        UECFromPreds = csOne;
    }

}

void NewMDAMOptimalDisjunctPrefixWA::calculateMetricsFromKeyPred(const ValueId & keyPred, const CostScalar & maxUEC, 
 CostScalar & UECFromPreds /*out*/, CostScalar & IntervalCountFromPreds /*out*/,
 int & lessCount /*in/out*/, int & greaterCount /*in/out*/)
{
  // TODO: Add logic to make sure our key column is always on the left (otherwise our lessCount and greaterCount
  // will be inaccurate)

  ItemExpr * ie = keyPred.getItemExpr();
  switch (ie->getOperatorType())
    {
      case ITM_LESS:
      case ITM_LESS_EQ:
        {
          lessCount++;
          UECFromPreds = maxUEC * CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_PRED_RANGE);
          if (UECFromPreds < csOne)
            UECFromPreds = csOne;
          IntervalCountFromPreds = csOne;
          break;
        }
      case ITM_GREATER:
      case ITM_GREATER_EQ:
        {
          greaterCount++;
          UECFromPreds = maxUEC * CostPrimitives::getBasicCostFactor(HIST_DEFAULT_SEL_FOR_PRED_RANGE);
          if (UECFromPreds < csOne)
            UECFromPreds = csOne;
          IntervalCountFromPreds = csOne;
          break;
        }
      case ITM_EQUAL:
      case ITM_IS_NULL:
        {
          UECFromPreds = csOne;
          IntervalCountFromPreds = csOne;
          break;
        }
      case ITM_OR:
        {
          int localLessCount = 0;
          int localGreaterCount = 0;
          CostScalar UECFromPreds0;
          CostScalar IntervalCountFromPreds0;
          calculateMetricsFromKeyPred(ie->child(0),maxUEC,
                                      UECFromPreds0,IntervalCountFromPreds0,
                                      localLessCount,localGreaterCount);
          CostScalar UECFromPreds1;
          CostScalar IntervalCountFromPreds1;
          calculateMetricsFromKeyPred(ie->child(1),maxUEC,
                                      UECFromPreds1,IntervalCountFromPreds1,
                                      localLessCount,localGreaterCount);
          // we'll be pessimistic here and assume no overlap in the values satisfying each leg of the OR
          UECFromPreds = UECFromPreds0 + UECFromPreds1;
          IntervalCountFromPreds = IntervalCountFromPreds0 + IntervalCountFromPreds1;
          break;
        }
      case ITM_AND:
        {
          CostScalar UECFromPreds0;
          CostScalar IntervalCountFromPreds0;
          calculateMetricsFromKeyPred(ie->child(0),maxUEC,
                                      UECFromPreds0,IntervalCountFromPreds0,
                                      lessCount,greaterCount);
          CostScalar UECFromPreds1;
          CostScalar IntervalCountFromPreds1;
          calculateMetricsFromKeyPred(ie->child(1),maxUEC,
                                      UECFromPreds1,IntervalCountFromPreds1,
                                      lessCount,greaterCount);
          // the most pessimistic assumption we can make is that the same set of rows that satisfies
          // one leg of the AND is a subset of those that satisfy the other leg
          UECFromPreds = MINOF(UECFromPreds0,UECFromPreds1);
          IntervalCountFromPreds = MINOF(IntervalCountFromPreds0,IntervalCountFromPreds1);
          break;
        }
      default:
        {
          UECFromPreds = csOne;  // unexpected operator
          IntervalCountFromPreds = csOne;
          DCMPASSERT(ie->getOperatorType() != ITM_AND);
          break;
        }
    }
}


// This function determines if the "currentCost" is a new minimum and returns TRUE if so,
// FALSE otherwise. If the decision was forced, the "forced" parameter will be set to TRUE,
// FALSE otherwise.

NABoolean NewMDAMOptimalDisjunctPrefixWA::isMinimalCost(Cost * currentCost,
                                                        CollIndex columnPosition,
                                                        NABoolean & forced /* out */)
{
  NABoolean newMinimumFound = FALSE;
  forced = FALSE;  // assume not forced

  if (scanForcePtr_ && mdamForced_)
    {
      if (columnPosition < scanForcePtr_->getNumMdamColumns())
        {
          // The user wants this column as part of the mdam key,
          // so pretend that the cost is lower when the column is added
          newMinimumFound = TRUE;
          forced = TRUE;
        }
      else
        {
          if (scanForcePtr_->getMdamColumnsStatus()
              == ScanForceWildCard::MDAM_COLUMNS_ALL)
            {
              // Unconditionally force all of the columns:
              newMinimumFound = TRUE;
              forced = TRUE;
            }
          else if (scanForcePtr_->getMdamColumnsStatus()
                   == ScanForceWildCard::MDAM_COLUMNS_NO_MORE)
            {
              // Unconditionally reject this and later columns:
              newMinimumFound = FALSE;
              forced = TRUE;
            }
          else
            {
              DCMPASSERT(scanForcePtr_->getMdamColumnsStatus()
                   == ScanForceWildCard::MDAM_COLUMNS_REST_BY_SYSTEM);
              // leave forced as FALSE
            }
        }
    } // if scanForcePtr_ && mdamForced_

  if (NOT forced)
    {
      // Mdam has not been forced, or forced but with the choice
      // of system decision for MDAM column. proceed with costing:
      
      DCMPASSERT(currentCost != NULL);  
      newMinimumFound = (optimalCost_ == NULL) ? TRUE :
        (currentCost->scmCompareCosts(*optimalCost_) == LESS);
    }

  return newMinimumFound;
}

const CostScalar & NewMDAMOptimalDisjunctPrefixWA::getOptMDAMProbeRows() const
{
  return optMDAMProbeRows_;
}

const CostScalar & NewMDAMOptimalDisjunctPrefixWA::getOptMDAMProbeSubsets() const
{
  return optMDAMProbeSubsets_;
}

const CostScalar & NewMDAMOptimalDisjunctPrefixWA::getOptMDAMFetchRows() const
{
  return optMDAMFetchRows_;
}

const CostScalar & NewMDAMOptimalDisjunctPrefixWA::getOptMDAMFetchSubsets() const
{
  return optMDAMFetchSubsets_;
}

CollIndex NewMDAMOptimalDisjunctPrefixWA::getStopColumn() const
{
  return optStopColumn_;
}


// ZZZZZ End of new MDAM costing code


// return true if has resuable shared basic cost for this mdam
NABoolean
FileScanOptimizer::getSharedCost(FileScanBasicCost * &fileScanBasicCostPtr /*out, never NULL*/
				 ,NABoolean & hasLostBefore /*out*/
				 ,SimpleCostVector * &disjunctsFRPtr /*out never NULL*/
				 ,SimpleCostVector * &disjunctsLRPtr /*out never NULL*/
				 ,CostScalar & numKBytes /*out*/
				 ,MdamKey* & sharedMdamKeyPtr /*out*/
				 ,NABoolean mdamTypeIsCommon /*in*/)
{

  NABoolean sharedCostFound = FALSE;
  fileScanBasicCostPtr = shareBasicCost(sharedCostFound);

  if ( mdamTypeIsCommon )
  {
    hasLostBefore = fileScanBasicCostPtr->hasMdamCommonLost();
    disjunctsFRPtr = &(fileScanBasicCostPtr->getFRBasicCostMdamCommon());
    disjunctsLRPtr = &(fileScanBasicCostPtr->getLRBasicCostMdamCommon());
    numKBytes = fileScanBasicCostPtr->getMdamCommonNumKBytes();
  }
  else
  {
    hasLostBefore = fileScanBasicCostPtr->hasMdamDisjunctsLost();
    disjunctsFRPtr = &(fileScanBasicCostPtr->getFRBasicCostMdamDisjuncts());
    disjunctsLRPtr = &(fileScanBasicCostPtr->getLRBasicCostMdamDisjuncts());
    numKBytes = fileScanBasicCostPtr->getMdamDisjunctsNumKBytes();
  }

  sharedMdamKeyPtr = fileScanBasicCostPtr->getMdamKeyPtr(mdamTypeIsCommon);
  return ( sharedCostFound AND
       sharedMdamKeyPtr AND
	   //       disjunctsFRPtr->getCPUTime() > csZero AND
	   //       disjunctsLRPtr->getCPUTime() > csZero AND
       CURRSTMT_OPTDEFAULTS->reuseBasicCost() );
}


Cost* FileScanOptimizer::newComputeCostForMultipleSubset
( MdamKey* mdamKeyPtr,
  const Cost * costBoundPtr,
  NABoolean mdamForced,
  CostScalar & numKBytes,
  ValueIdSet exePreds,
  NABoolean checkExePreds,
  NABoolean mdamTypeIsCommon,
  MdamKey *&sharedMdamKeyPtr )
{
  MDAM_DEBUG0(MTL2,"BEGIN MDAM Costing --------");
  MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this,
      costBoundPtr, "Cost Bound"));
  // MDAM only works for key sequenced files.
  DCMPASSERT(getIndexDesc()->getNAFileSet()->isKeySequenced());
  DCMPASSERT(getIndexDesc()->getIndexKey().entries() > 0);

  FileScanBasicCost *fileScanBasicCostPtr;
  SimpleCostVector *disjunctsFRPtr;
  SimpleCostVector *disjunctsLRPtr;
  NABoolean hasLostBefore = FALSE;
  NABoolean shareCost = getSharedCost(fileScanBasicCostPtr,
				      hasLostBefore,
				      disjunctsFRPtr,
				      disjunctsLRPtr,
				      numKBytes,
				      sharedMdamKeyPtr,
				      mdamTypeIsCommon);

  SimpleCostVector & disjunctsFR = *disjunctsFRPtr;
  SimpleCostVector & disjunctsLR = *disjunctsLRPtr;

  if(shareCost){
    MDAM_DEBUG0(MTL2, "Use cached MDAM cost");
    if(hasLostBefore){
      MDAM_DEBUG0(MTL2, "MDAM Costing returning NULL cost");
      MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");
      return NULL;
    }
    else {
      Cost * costPtr = computeCostObject(disjunctsFR, disjunctsLR);
//       if ( costBoundPtr != NULL )
//       {
//         COMPARE_RESULT result =
//           costPtr->compareCosts(*costBoundPtr,
//                                  getContext().getReqdPhysicalProperty());
//         if ( result == MORE OR result == SAME )
// 	{
//           delete costPtr;
// 	  MDAM_DEBUG0(MTL2, "MDAM Costing returning NULL cost");
// 	  MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");
//           return NULL;
// 	}
//       }

      // use cached MDAM Cost
      mdamKeyPtr->reuseMdamKeyInfo(sharedMdamKeyPtr);
      MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this, costPtr,
          "Returning cached MDAM Cost"));
      MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");
      return costPtr;
    }
  }

  // proceed with MDAM costing
  disjunctsFR.reset();
  disjunctsLR.reset();
  sharedMdamKeyPtr = mdamKeyPtr;
  fileScanBasicCostPtr->setMdamKeyPtr(mdamKeyPtr,mdamTypeIsCommon);

  MDAMCostWA costWA(*this,
		    mdamForced,
		    mdamKeyPtr,
		    costBoundPtr,
		    exePreds,
		    disjunctsFR,
		    disjunctsLR);
  costWA.compute();
  NABoolean isMdamWon = costWA.isMdamWon();
  if(NOT isMdamWon) {
    MDAM_DEBUG0(MTL2, "MDAM Costing returning NULL cost");
    MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");

    if(mdamTypeIsCommon)
      fileScanBasicCostPtr->setMdamCommonLost(TRUE);
    else
      fileScanBasicCostPtr->setMdamDisjunctsLost(TRUE);

    return NULL;
  }
  // MDAM won! create the cost vector:
  Cost *costPtr = computeCostObject(disjunctsFR
                                    ,disjunctsLR);

  NABoolean noExePreds = costWA.hasNoExePreds();
  //noExePreds is true, great set the flag in MDAM
  if ( CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
  {
    if(!exePreds.entries()) 
      noExePreds = TRUE;
  }
  if(noExePreds AND checkExePreds)
  {
      mdamKeyPtr->setNoExePred();
  }
  numKBytes = costWA.getNumKBytes();

  if ( checkExePreds )
    fileScanBasicCostPtr->setMdamDisjunctsNumKBytes(numKBytes);
  else
    fileScanBasicCostPtr->setMdamCommonNumKBytes(numKBytes);

  MDAM_DEBUGX(MTL2, MdamTrace::printCostObject(this, costPtr,
      "Returning MDAM Cost"));
  MDAM_DEBUG0(MTL2, "END   MDAM Costing --------\n");
  return costPtr;
} // newComputeCostForMultipleSubset(...)

Cost* 
FileScanOptimizer::scmComputeCostForMultipleSubset(MdamKey* mdamKeyPtr,
						   const Cost * costBoundPtr,
						   NABoolean mdamForced,
						   CostScalar & numKBytes,
						   ValueIdSet exePreds,
						   NABoolean checkExePreds,
						   NABoolean mdamTypeIsCommon,
						   MdamKey *&sharedMdamKeyPtr )
{
  // MDAM only works for key sequenced files.
  DCMPASSERT(getIndexDesc()->getNAFileSet()->isKeySequenced());
  DCMPASSERT(getIndexDesc()->getIndexKey().entries() > 0);

  sharedMdamKeyPtr = mdamKeyPtr;

  SimpleCostVector a(0,0,0,0,0,0,0,0,0,0);
  SimpleCostVector b(0,0,0,0,0,0,0,0,0,0);
    
  MDAMCostWA costWA(*this,
		    mdamForced,
		    mdamKeyPtr,
		    costBoundPtr,
		    exePreds,
                    a,
                    b);

  costWA.compute();
  NABoolean isMdamWon = costWA.isMdamWon();
  if(NOT isMdamWon)
    return NULL;  

  NABoolean noExePreds = costWA.hasNoExePreds();
  //noExePreds is true, great set the flag in MDAM
  if(noExePreds AND checkExePreds)
  {
      mdamKeyPtr->setNoExePred();
  }
  numKBytes = costWA.getNumKBytes();

  // MDAM won! create the cost vector:
  return costWA.getScmCost();

} // scmComputeCostForMultipleSubset(...)


Cost* 
FileScanOptimizer::scmRewrittenComputeCostForMultipleSubset(MdamKey* mdamKeyPtr,
						   const Cost * costBoundPtr,
						   NABoolean mdamForced,
						   CostScalar & numKBytes,
						   ValueIdSet exePreds,
						   NABoolean checkExePreds,
						   MdamKey *&sharedMdamKeyPtr )
{
  // MDAM only works for key sequenced files.
  DCMPASSERT(getIndexDesc()->getNAFileSet()->isKeySequenced());
  DCMPASSERT(getIndexDesc()->getIndexKey().entries() > 0);

  sharedMdamKeyPtr = mdamKeyPtr;
    
  NewMDAMCostWA costWA(*this,
                       mdamForced,
                       mdamKeyPtr,
                       costBoundPtr,
                       exePreds,
                       singleSubsetSize_);

  costWA.compute();
  NABoolean isMdamWon = costWA.isMdamWon();
  if(NOT isMdamWon)
    return NULL;  

  NABoolean noExePreds = costWA.hasNoExePreds();
  //noExePreds is true, great set the flag in MDAM
  if(noExePreds AND checkExePreds)
    {
      mdamKeyPtr->setNoExePred();
    }
  numKBytes = costWA.getNumKBytes();

  // MDAM won! return the cost vector
  return costWA.getScmCost();

} // scmRewrittenComputeCostForMultipleSubset(...)


NABoolean FileScanOptimizer::isMultipleProbes() const
{
  CostScalar repeatCount = getContext().getPlan()->getPhysicalProperty()->
    getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2() ;
  CollIndex numCols = getContext().getInputLogProp()->getColStats().entries();

  return (repeatCount.isGreaterThanOne()) OR (numCols > 0);
}

// return the forced scan wild card if exists
// otherwise return NULL
const ScanForceWildCard* FileScanOptimizer::findScanForceWildCard() const
{
  const ScanForceWildCard* scanForcePtr = NULL;
  const ReqdPhysicalProperty* propertyPtr =
    getContext().getReqdPhysicalProperty();
  if ( propertyPtr
       && propertyPtr->getMustMatch()
       && (propertyPtr->getMustMatch()->getOperatorType()
	   == REL_FORCE_ANY_SCAN))
    {
      scanForcePtr =
	(const ScanForceWildCard*)propertyPtr->getMustMatch();
    }
  return scanForcePtr;
}

// return the last key column in a disjunct, one based
CollIndex FileScanOptimizer::computeLastKeyColumnOfDisjunct
(const ColumnOrderList & keyPredsByCol)
{
  // The number of entries in keyPredsByCol should be equal
  // to the number columns in the key which should be greater than zero.
  Lng32 lastColumnPosition = (Lng32)(keyPredsByCol.entries()) - 1;
  DCMPASSERT(lastColumnPosition >= 0);
  // Find the last column position:
  // The order (i.e., column position) must be varied
  // up to the last column position that references
  // key predicates. Find the lastColumnPosition:
  // walk from the last key column until you find
  // a column position that references a key pred:
  ValueId keyCol;
  NABoolean foundLastColumn = FALSE;
  while( NOT foundLastColumn && lastColumnPosition >=0 )
    {
      //following is guard against the situation where we can have
      //key columns(a,b,a) and predicate a=3 we would chose second
      //'a' as last coulumn whereas we know that we would never skip
      //the first 'a' and apply the predicate to the second 'a'
      NABoolean duplicateFound=FALSE;
      keyCol = getIndexDesc()->getIndexKey()[lastColumnPosition];
      for( Lng32 preColumn = lastColumnPosition - 1;
	   preColumn >= 0; preColumn--)
	{
	  if(keyCol==getIndexDesc()->getIndexKey()[preColumn])
	    {
	      duplicateFound=TRUE;
	      break;
	    }
	}

      if ( NOT duplicateFound ) {
	// any predicate in the set may refer to the key column:
	const ValueIdSet *predsPtr = keyPredsByCol[lastColumnPosition];
	if(predsPtr && NOT predsPtr->isEmpty())
	{
	  foundLastColumn = TRUE;
	}
      }

      if( NOT foundLastColumn ){
	lastColumnPosition--;
      }
    }

  // if not found last column (e.g. MDAM is forced, but there is no key pred)
  // set it to the first column
  if( NOT foundLastColumn ){
    lastColumnPosition = 0;
  }

  // make column position start from one:
  lastColumnPosition++;
  return lastColumnPosition;
}

const CostScalar FileScanOptimizer::getIncomingProbes() const
{
   // repeat count is the actual receiving probes.
  const CostScalar repeatCount =
    getContext().getPlan()->getPhysicalProperty()->
    getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2();
  const CostScalar incomingProbes = repeatCount * getNumActivePartitions();
  DCMPASSERT(incomingProbes >= repeatCount);
  return incomingProbes;
}

NABoolean FileScanBasicCost::hasMdamCommonLost() const
{
  return mdamCommonLost_;
}

NABoolean FileScanBasicCost::hasMdamDisjunctsLost() const
{
  return mdamDisjunctsLost_;
}

void FileScanBasicCost::setMdamCommonLost(NABoolean mdamCommonLost)
{
  mdamCommonLost_ = mdamCommonLost;
}

void FileScanBasicCost::setMdamDisjunctsLost(NABoolean mdamDisjunctsLost)
{
  mdamDisjunctsLost_ = mdamDisjunctsLost;
}
