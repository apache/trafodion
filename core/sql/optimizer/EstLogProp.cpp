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
******************************************************************************
*
* File:         EstLogProp.cpp
* Description:  Estimated logical Properties
*
* Created:      10/12/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "EstLogProp.h"
#include "GroupAttr.h"
#include "VEGTable.h"
#include "ItemOther.h"
#include "RelUpdate.h"
#include "opt.h"

/////////////////////
#include "Analyzer.h"
#include "AppliedStatMan.h"
/////////////////////


THREAD_P ObjectCounter (*EstLogProp::counter_)(0);

EstLogProp::EstLogProp(CostScalar card,
		       ValueIdSet *preds,
                       SemiTSJEnum inputForSemiTSJ,
		       CANodeIdSet *nodeSet,
		       NABoolean cacheable,
                       NAMemory * h)
             : resultCardinality_(card),
               maxCardinality_((Cardinality)-1),
               columnStats_(h),
               inputForSemiTSJ_(inputForSemiTSJ),
		cacheable_(cacheable),
	       isCardinalityEqOne_(FALSE)
{
  if (preds != NULL)
    unresolvedPreds_ = *preds;
  nodeSet_ = nodeSet;
  (*counter_).incrementCounter();
}

EstLogProp::EstLogProp(const EstLogProp &other, NAMemory * h)
           : columnStats_(h),
	     isCardinalityEqOne_(other.isCardinalityEqOne_)
{
  resultCardinality_ = other.resultCardinality_;
  maxCardinality_    = other.maxCardinality_;
  nodeSet_           = other.nodeSet_;
  columnStats_       = other.columnStats_;
  unresolvedPreds_   = other.unresolvedPreds_;
  inputForSemiTSJ_   = other.inputForSemiTSJ_;
  cacheable_	     = other.cacheable_;
  (*counter_).incrementCounter();

#ifndef NDEBUG
  columnStats_.verifyInternalConsistency(0,columnStats_.entries()) ;
#endif
}

EstLogProp & EstLogProp::operator= (const EstLogProp &other)
{
  resultCardinality_  = other.resultCardinality_;
  maxCardinality_     = other.maxCardinality_;
  columnStats_        = other.columnStats_;
  unresolvedPreds_    = other.unresolvedPreds_;
  inputForSemiTSJ_    = other.inputForSemiTSJ_;
  isCardinalityEqOne_ = other.isCardinalityEqOne_;
  nodeSet_           = other.nodeSet_;
  cacheable_	     = other.cacheable_;

#ifndef NDEBUG
  columnStats_.verifyInternalConsistency(0,columnStats_.entries()) ;
#endif

  return *this;
}

EstLogProp::~EstLogProp()
{
  //get Handle to queryAnalysis
  QueryAnalysis *qa = QueryAnalysis::Instance();

  // check the destructor for efficiency
  if ((cacheable_) &&
      (nodeSet_) &&
      (qa) && (qa->isAnalysisON()))
  {
    AppliedStatMan *appStatMan = qa->getASM();
    if(appStatMan)
      appStatMan->removeEntryIfThisObjectIsCached(this);

    nodeSet_ = NULL;
  }
  unresolvedPreds_.clear();
  columnStats_.clear();
  (*counter_).decrementCounter();
}

NABoolean EstLogProp::reconcile(const EstLogProp &other)
{
  // simple logic for now: merge unresolved preds lists,
  // use the average for the cardinality,
  // use the max values for numBaseTables_ and numJoinedTables_,

  return FALSE;

  unresolvedPreds_ += other.unresolvedPreds_;
  resultCardinality_ = (resultCardinality_ + other.resultCardinality_) / 2;

} // EstLogProp::reconcile

COMPARE_RESULT EstLogProp::compareEstLogProp (const EstLogPropSharedPtr &other) const
{
  
  if (this == other.get())
    return SAME;

  // First thing that we may want to compare is the CANodeSets of the EstLogProp
  // if these are NOT NULL

  // This would work if Query Analizer created nodeSet_ for this and other
  if ((nodeSet_ != NULL) && (other->nodeSet_ != NULL))
  {
    if ((*nodeSet_) == (*(other->nodeSet_)))
      return SAME;
    else
      return INCOMPATIBLE;
  }

  // This is the old logic after removing heuristic returning SAME for close 
  // EstLogProp like resultCardinality_/other->resultCardinality in [0.8,1.2]
  // That heuristics was incompatible with Cascades assumption that in the case
  // when pruning is on we cannot have 2 different context for optimization 
  // if their comparison returns SAME.
  if ( resultCardinality_ == other->resultCardinality_  AND
       ( // Check for the case where we have two "empty" input logical properties.
         ( columnStats_.entries() == 0 AND other->columnStats_.entries() == 0) 
         OR
         ( columnStats_       == other->columnStats_ AND
	   unresolvedPreds_   == other->unresolvedPreds_ AND
	   inputForSemiTSJ_   == other->inputForSemiTSJ_ )
       )
     )
    return SAME;

  return INCOMPATIBLE;
}

// ---------------------------------------------------------------------
// Utility Routine: pickOutputs
//
// From the given ColStatDescList, populate columnStats_ with column
// descriptors that are useful based on the characteristic outputs for
// the group.
//
// Always include in the output the current histograms of the input data,
// and, if the histogram is contained in the required output list, then
// this is a useful histogram and will also be output.
//
// ---------------------------------------------------------------------
void EstLogProp::pickOutputs( ColStatDescList & columnStats,
			      const EstLogPropSharedPtr& inputEstLogProp,
			      const ValueIdSet specifiedOutputs,
			      const ValueIdSet predSet)
{

  const ColStatDescList & outerColStatsList = inputEstLogProp->getColStats();

  ValueIdSet colsRequiringHistograms = specifiedOutputs;
  
  // (i) see if the selection predicates contain any constant value or a 
  // constant expression

  // (ii) check if there are any columns of this table being joined to some other
  // columns, which do not appear as characteristics outputs. There should be
  // histograms available for these columns, as these might be needed later.
  // This problem was seen for temporary tables created as normal_tables by the
  // triggers.


  colsRequiringHistograms.addSet(predSet.getColumnsForHistogram());
  colStats().setMCSkewedValueLists(columnStats.getMCSkewedValueLists()) ;

  NABoolean colStatDescAdded = FALSE;

  for (CollIndex i=0; i < columnStats.entries(); i++)
    {
      // we probably don't need 'em all, but this is the easiest way to
      // grab all of the multi-column uec information we'll need later
      colStats().insertIntoUecList (columnStats.getUecList()) ;
      colStats().setScanRowCountWithoutHint(columnStats.getScanRowCountWithoutHint());
      NABoolean found = FALSE;

      // Note: The following inserts into a ColStatDescList should not
      // have to be deep copies.  From this point on, ColStatDescs that
      // describe the output of the calling operator are read-only.

      ColStatDescSharedPtr colStatDesc = columnStats[i];

      // the value-id we're looking for
      const ValueId columnId = colStatDesc->getVEGColumn() ;

      for (CollIndex j=0 ; j < outerColStatsList.entries() ; j++)
	{
	  if (columnId == outerColStatsList[j]->getVEGColumn() OR
              (CmpCommon::context()->showQueryStats()))
            {
              colStats().insert(colStatDesc) ;
              found = TRUE;
              if(!colStatDescAdded)
                colStatDescAdded = TRUE;
              break ; // jump to next ColStatDesc
            }
	}

    // OK, the valueid doesn't match directly -- but there are still a
    // couple of things to check in order to verify whether or not we're
    // interested in keeping the i'th ColStatDesc ...

	ValueId throwaway ; // used by the second clause below

    if ( NOT found  AND
	 (columnId != NULL_VALUE_ID) AND
         (colsRequiringHistograms.contains (columnId) OR
          colsRequiringHistograms.referencesTheGivenValue (columnId, throwaway) OR
	  columnId.isInvolvedInJoinAndConst() OR
          CmpCommon::context()->showQueryStats() )
	)
	{
	  colStats().insert(colStatDesc);
	  found = TRUE;
	  if(!colStatDescAdded)
	    colStatDescAdded = TRUE;
	}
	
	if (CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting())
	{
	  // if the column is referenced for histogram, but is 
	  // not needed beyond this time , then we shall save its  
	  // max freq, which might be used later in costing if this
	  // column is a part of the partitioning key

	  ColStatsSharedPtr stat = colStatDesc->getColStats();
	  if (!(stat->isVirtualColForHist() ) && NOT found &&
                    !(stat->isOrigFakeHist() ) )
	  {
            const ValueId col = colStatDesc->getColumn();
            ColAnalysis * colAnalysis = col.colAnalysis();
            if (colAnalysis)
            {
              NAColumn * column = stat->getStatColumns()[0];

              if (column->isReferencedForHistogram())
              {
                CostScalar maxFreq = columnStats.getMaxFreq(columnId);
                colAnalysis->setMaxFreq(maxFreq);
                colAnalysis->setFinalUec(stat->getTotalUec());
                colAnalysis->setFinalRC(stat->getRowcount());
              }
            }
          }
	}
      } // for columnStats.entries()
      if(!colStatDescAdded && columnStats.entries() > 0)
        colStats().insert(columnStats[0]) ;
} // pickOutputs

// -----------------------------------------------------------------
// This method is used to map colStats of my child to mine
// -----------------------------------------------------------------
void EstLogProp::pickOutputsForUpdate( ColStatDescList colStatsFromScan, 
				      const EstLogPropSharedPtr& inputEstLogProp, 
				      const RelExpr & relExpr,
				      const ValueIdSet updateExprOutputs,
				      const ValueIdSet predSet)
{
  GenericUpdate & updateExpr = (GenericUpdate &) relExpr;

  ValueIdMap & updateSelectValueIdMap = updateExpr.updateToSelectMap();

  // map these to my child's output(or right child's outputs, in case I am a leaf)
  // to get the appropriate colStats;

  ValueIdSet mappedSelectOutputs;
  updateSelectValueIdMap.rewriteValueIdSetDown(updateExprOutputs, mappedSelectOutputs);

  pickOutputs(colStatsFromScan, inputEstLogProp, mappedSelectOutputs, predSet);
  
  // Now for each colStat, find the matching column of the insert, 
  // and create colStat for that
  mapOutputsForUpdate(updateExpr, updateSelectValueIdMap);
}

// ------------------------------------------------------------------------------
// create my colStats based on my child's output, by converting the columns to 
// that of mine
// ------------------------------------------------------------------------------
void EstLogProp::mapOutputsForUpdate(const GenericUpdate & updateExpr, 
				     const ValueIdMap & updateSelectValueIdMap)
{

  TableDesc * updateTable = updateExpr.getTableDesc();

  for ( CollIndex i = 0; i < colStats().entries(); i++ )
  {
    ColStatDescSharedPtr colStatPtr = (colStats())[i];     
    const ValueId columnId = colStatPtr->getVEGColumn();

    ValueId updateColVEGOutputId;
    updateSelectValueIdMap.mapValueIdUp(updateColVEGOutputId, columnId);
    ValueId updateBaseColumnId;

    if (updateColVEGOutputId != columnId)
    {
      updateBaseColumnId = updateColVEGOutputId;
     
      ValueIdSet baseColumns;
      updateColVEGOutputId.getItemExpr()->findAll( ITM_BASECOLUMN, baseColumns, TRUE, TRUE );

      // from all the columns extracted, get the one for Insert table
      TableDesc * thisTable = NULL;
      for (ValueId column = baseColumns.init(); baseColumns.next(column);
	  baseColumns.advance(column) )
      {
	ItemExpr * columnExpr = column.getItemExpr();
	thisTable = ((BaseColumn *)columnExpr)->getTableDesc();
	if (thisTable == updateTable)
	{
	  // set my column as the base column
	  updateBaseColumnId = column;
	  break;
	}
       }
       
       ColStatsSharedPtr inColStats = colStatPtr->getColStats();
       ColStatsSharedPtr colStatsForUpdate(new (STMTHEAP) ColStats (*inColStats,STMTHEAP));

       colStatsForUpdate->setStatColumn(updateBaseColumnId.getNAColumn());
       // use this ColStat to generate new ColStat corresponding to the char output
       // of the Update expression

       ColStatDescSharedPtr colStatDescForUpdate(new (STMTHEAP) ColStatDesc(colStatsForUpdate, 
					    updateBaseColumnId,  // ValueId of the column that will be used 
							 // as a column name, VEG and mergeStats
					    STMTHEAP), STMTHEAP);
       colStatDescForUpdate->VEGColumn() = updateColVEGOutputId;
       colStatDescForUpdate->mergeState().clear() ;
       colStatDescForUpdate->mergeState().insert(updateBaseColumnId);

       // Remove the old colStat and insert this colStat into the result colStatDescList
       colStats().removeAt( i );

       colStats().insertDeepCopyAt(i, colStatDescForUpdate, // colStats to be copied
				     1,			   // scale
				     FALSE);

    }
  }
}

// -------------------------------------------------------------------
// EstLogProp::getCardOfBusiestStream
// method returns the cardinality of the busiest stream for the given
// partitioning key
// -------------------------------------------------------------------

CostScalar 
EstLogProp::getCardOfBusiestStream(const PartitioningFunction* partFunc,
                                   Lng32 numOfParts,
                                   GroupAttributes * groupAttr,
                                   Lng32 countOfCPUs,
                                   NABoolean isUnderNestedJoin) 
{
  // if there are no histograms available, return rowCount / number of 
  // partitions as the probesPerStream

  ColStatDescList &colStatsList = this->colStats();
  if ((colStatsList.entries() == 0) || 
      (groupAttr && groupAttr->getIsProbeCacheable()))
  {
	return ( getResultCardinality() / numOfParts).minCsOne();
  }

  CostScalar cardinalityPerStream;
  if (NOT isUnderNestedJoin)
     cardinalityPerStream = colStatsList.getCardOfBusiestStream(
                                         partFunc,
                                         numOfParts,
                                         groupAttr,
                                         countOfCPUs);
  else
  {
    CANodeIdSet* outerNodeSet = getNodeSet();
    cardinalityPerStream = colStatsList.getCardOfBusiestStreamForUnderNJ(
                                    outerNodeSet,
                                    partFunc,
                                    numOfParts,
                                    groupAttr,
                                    countOfCPUs);
  }
  return cardinalityPerStream;

} // EstLogProp::getCardOfBusiestStream

void EstLogProp::print(FILE* ofd, const char* prefix, const char *suffix) const
{
} // EstLogProp::print()

NABoolean EstLogProp::operator == (const EstLogProp & other) const
{
  if (compareEstLogProp(&other) == SAME)
    return TRUE;
  else
    return FALSE;
}
