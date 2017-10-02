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
**************************************************************************
*
* File:         TableDesc.C
* Description:  A table descriptor
* Created:      4/27/94
* Language:     C++
* Status:       Experimental
*
*
**************************************************************************
*/

#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "BindWA.h"
#include "ComOperators.h"
#include "ItemColRef.h"
#include "ParNameLocList.h"
#include "Sqlcomp.h"
#include "ex_error.h"
#include "Cost.h"      /* for lookups in the defaults table */
#include "Analyzer.h"
#include "HDFSHook.h"

// -----------------------------------------------------------------------
// Constructor (but note that much more useful stuff goes on in
// static createTableDesc in BindRelExpr.C)
// -----------------------------------------------------------------------
TableDesc::TableDesc(BindWA *bindWA,
                     const NATable *table,
                     CorrName& corrName)
         	: table_(table),
                  indexes_(bindWA->wHeap()),
                  uniqueIndexes_(bindWA->wHeap()),
                  vertParts_(bindWA->wHeap()),
                  hintIndexes_(bindWA->wHeap()),
                  colStats_(bindWA->wHeap()),
                  corrName_("",bindWA->wHeap()),
                  analysis_(NULL)
{
  corrName.applyDefaults(bindWA, bindWA->getDefaultSchema());
  corrName_ = corrName ;
  selectivityHint_ = NULL;
  cardinalityHint_ = NULL;
  histogramsCompressed_ = FALSE;
  minRC_ = csOne;
  maxRC_ = COSTSCALAR_MAX;

  // Fix up the name location list to help with the computing
  // of view text, check constraint search condition text, etc.
  //
  if (corrName.getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE)  // -- Triggers
	  return;

  ParNameLocList *pNameLocList = bindWA->getNameLocListPtr();
  if (pNameLocList)
  {
    ParNameLoc * pNameLoc
      = pNameLocList->getNameLocPtr(corrName_.getNamePosition());
    if (pNameLoc AND pNameLoc->getExpandedName(FALSE).isNull())
    {
      pNameLoc->setExpandedName(corrName_.getQualifiedNameObj().
                                getQualifiedNameAsAnsiString());
    }
  }
}


// -----------------------------------------------------------------------
// Add a CheckConstraint to the TableDesc.
//
// A table check constraint "CHECK (pred)" evaluates as
// "WHERE (pred) IS NOT FALSE" (i.e. TRUE or NULL);
// see ANSI 4.10 and in particular 11.21 GR 5+6.
//
// A view check constraint is the WHERE clause of a WITH CHECK OPTION view;
// it must evaluate to TRUE:
// "WHERE pred" (FALSE or NULL *fails*).
//
// Note that if the pred is "IS NOT NULL" (certainly a common CHECK pred),
// we can optimize the run-time a tiny bit by not having to use a BoolVal
// (because "IS NOT NULL" only returns TRUE or FALSE, never UNKNOWN/NULL).
//
// This method should be kept in synch with NAColumn::getNotNullViolationCode.
// -----------------------------------------------------------------------
void TableDesc::addCheckConstraint(BindWA *bindWA,
				   const NATable *naTable,
				   const CheckConstraint *constraint,
				   ItemExpr *constrPred)
{
  BoolVal *ok = new (bindWA->wHeap()) BoolVal(ITM_RETURN_TRUE);

  RaiseError *error = new (bindWA->wHeap())
    RaiseError(0,
               constraint->getConstraintName().getQualifiedNameAsAnsiString(),
               naTable->getTableName().getQualifiedNameAsAnsiString(),
               bindWA->wHeap());

  if (constraint->isViewWithCheckOption())
    {
      if (constraint->isTheCascadingView())
	error->setSQLCODE(EXE_CHECK_OPTION_VIOLATION);		// cascadING
      else
	error->setSQLCODE(EXE_CHECK_OPTION_VIOLATION_CASCADED);	// cascadED
      constrPred = new (bindWA->wHeap()) IfThenElse(constrPred, ok, error);
    }
  else if (constrPred->isISNOTNULL())
    {
      error->setSQLCODE(EXE_TABLE_CHECK_CONSTRAINT);
      constrPred = new (bindWA->wHeap()) IfThenElse(constrPred, ok, error);
    }
  else
    {
      constrPred = new (bindWA->wHeap()) UnLogic(ITM_IS_FALSE, constrPred);
      error->setSQLCODE(EXE_TABLE_CHECK_CONSTRAINT);
      constrPred = new (bindWA->wHeap()) IfThenElse(constrPred, error, ok);
    }

  // IfThenElse only works if Case is its parent.
  constrPred = new (bindWA->wHeap()) Case (NULL, constrPred);

  constrPred->bindNode(bindWA);
  CMPASSERT(!bindWA->errStatus());

  checkConstraints_.insert(constrPred->getValueId());

} // TableDesc::addCheckConstraint

// TableDesc::isKeyIndex()
// Parameter is an secondary index on the table. Table checks to see
// if the keys of the secondary index is built using the primary key
// of the table. If it is return true otherwise false.
NABoolean TableDesc::isKeyIndex(const IndexDesc * idesc) const
{
  ValueIdSet pKeyColumns = clusteringIndex_->getIndexKey();
  ValueIdSet indexColumns = idesc->getIndexKey();
  ValueIdSet basePKeys=pKeyColumns.convertToBaseIds();


  for(ValueId id = indexColumns.init(); indexColumns.next(id);
			indexColumns.advance(id))
  {
	ValueId baseId = ((BaseColumn *)(((IndexColumn *)id.getItemExpr())->
			  getDefinition().getItemExpr()))->getValueId();
	if(NOT basePKeys.contains(baseId))
	{
	   return FALSE;
	}
  }

  return TRUE;


}

// this method sets the primary key columns. It goes through all the columns
// of the table, and collects the columns which are marked as primary keys
void TableDesc::setPrimaryKeyColumns()
{
  ValueIdSet primaryColumns;

  for ( CollIndex j = 0 ; j < colList_.entries() ; j++ )
    {
      
      ValueId valId = colList_[j];
      
      NAColumn *column = valId.getNAColumn();

      if ( column->isPrimaryKey() )
      {
	primaryColumns.insert(valId) ;
        // mark column as referenced for histogram, as we may need its histogram
        // during plan generation
        if ((column->isUserColumn() || column->isSaltColumn() ) &&
            (column->getNATable()->getSpecialType() == ExtendedQualName::NORMAL_TABLE) )
              column->setReferencedForMultiIntHist();
      }
    }

  primaryKeyColumns_ = primaryColumns;
}

// -----------------------------------------------------------------------------
// NABoolean TableDesc::isSpecialObj() returns TRUE if the table is an internal
// table such as HISTOGRM, HISTINTS, DESCRIBE, or an SMD, UMD, or an MVUMD table.
// One of its usage is during
// getTableColStatas, where we do not want the compiler to print no stats
// warning.
// -----------------------------------------------------------------------------
NABoolean TableDesc::isSpecialObj()
{
  const NATable * naTable = getNATable();
  if (naTable->isUMDTable()   ||
      naTable->isSMDTable()   ||
      naTable->isMVUMDTable() ||
      naTable->isTrigTempTable() )
    return TRUE;

  const NAString& fileName = getCorrNameObj().getQualifiedNameObj().getObjectName();
  if ( ( fileName == "DESCRIBE__") ||	  // for non_dml statements such as showddl etc.
  (fileName == "HISTOGRM")   ||		  // following are used during update stats
  (fileName == "HISTINTS")   )
    return TRUE;
  else
    return FALSE;
}


// -----------------------------------------------------------------------
// TableDesc::getUserColumnList()
// -----------------------------------------------------------------------
void TableDesc::getUserColumnList(ValueIdList &columnList) const
{
  for (CollIndex i = 0; i < colList_.entries(); i++) {
    ValueId valId = colList_[i];
    NAColumn *column = valId.getNAColumn();
    if (column->isUserColumn())
      columnList.insert(valId);
  }
}

// -----------------------------------------------------------------------
// TableDesc::getSystemColumnList()
// -----------------------------------------------------------------------
void TableDesc::getSystemColumnList(ValueIdList &columnList) const
{
  for (CollIndex i = 0; i < colList_.entries(); i++) {
    ValueId valId = colList_[i];
    NAColumn *column = valId.getNAColumn();
    if (column->isSystemColumn())
      columnList.insert(valId);
  }
}


// -----------------------------------------------------------------------
// TableDesc::getIdentityColumn()
// -----------------------------------------------------------------------
void TableDesc::getIdentityColumn(ValueIdList &columnList) const
{
  for (CollIndex i = 0; i < colList_.entries(); i++) 
    {
      ValueId valId = colList_[i];
      NAColumn *column = valId.getNAColumn();
      if (column->isIdentityColumn())
	{
	columnList.insert(valId);
	break; // Break when you find the first,
	// as there can only be one Identity column per table.
	}
    }
}


NABoolean TableDesc::isIdentityColumnGeneratedAlways(NAString * value) const 
{
  // Determine if an IDENTITY column exists and
  // has the default class of GENERATED ALWAYS AS IDENTITY.
  // Do not return TRUE, if the table type is an INDEX_TABLE.

  NABoolean result = FALSE;
  
  for (CollIndex j = 0; j < colList_.entries(); j++)
    {
      ValueId valId = colList_[j];
      NAColumn *column = valId.getNAColumn();

      if(column->isIdentityColumnAlways())
        {
	  if (getNATable()->getObjectType() != COM_INDEX_OBJECT)
	  {
	    if (value != NULL)
	      *value = column->getColName();
            result = TRUE;
	  }
        }
    }

    return result;
}

NABoolean TableDesc::hasIdentityColumnInClusteringKey() const
{
  ValueIdSet pKeyColumns = clusteringIndex_->getIndexKey();
  NAColumn * column = NULL;
  for(ValueId id = pKeyColumns.init(); pKeyColumns.next(id);
      pKeyColumns.advance(id))
  {
      column = id.getNAColumn();
      if (column && column->isIdentityColumn())
          return TRUE;
  }
  return FALSE;
}
 
// -----------------------------------------------------------------------
// Given a column list providing identifiers for columns of this table,
// this method returns a list of VEG expressions and/or base columns that
// show the equivalence of base columns with index columns.
// -----------------------------------------------------------------------
void TableDesc::getEquivVEGCols (const ValueIdList& columnList,
				 ValueIdList &VEGColumnList) const
{
  for (CollIndex i=0; i < columnList.entries(); i++)
    VEGColumnList.insert(getEquivVEGCol(columnList[i]));
}

void TableDesc::getEquivVEGCols (const ValueIdSet& columnSet,
				 ValueIdSet &VEGColumnSet) const
{
  for (ValueId v=columnSet.init();
       columnSet.next(v);
       columnSet.advance(v))
    VEGColumnSet += getEquivVEGCol(v);
}

ValueId TableDesc::getEquivVEGCol (const ValueId& column) const
{
  BaseColumn *bc = column.castToBaseColumn();

  CMPASSERT(bc->getTableDesc() == this);
  return getColumnVEGList()[bc->getColNumber()];
}

// -----------------------------------------------------------------------
// Statistics stuff
// -----------------------------------------------------------------------
const ColStatDescList &TableDesc::getTableColStats()
{
    // HIST_NO_STATS_UEC can never be greater than HIST_NO_STATS_ROWCOUNT.
    // If the customer has done an illegal setting, ignore that, and set
    // it to maximum permissible value

    if (CURRSTMT_OPTDEFAULTS->defNoStatsUec() > CURRSTMT_OPTDEFAULTS->defNoStatsRowCount())
    {
      CURRSTMT_OPTDEFAULTS->setNoStatsUec(CURRSTMT_OPTDEFAULTS->defNoStatsRowCount());
    }

    if (colStats_.entries() > 0)
    {
      if (!areHistsCompressed() && (CmpCommon::getDefault(COMP_BOOL_18) != DF_ON) )
      {
	// compress histograms based on query predicates
	compressHistogramsForCurrentQuery();
      }
      return colStats_;
    }

    // For each ColStat, create a ColStat descriptor.
    StatsList &stats = ((NATable *)table_)->getStatistics() ;

    // if for some reason, no histograms were returned by update statistics
    // generate fake histograms for the table

    if ( stats.entries() == 0 ) 
       stats = ((NATable *)table_)->generateFakeStats();

    const NAColumnArray &columnList = ((NATable *)table_)->getNAColumnArray();
    const ValueIdList & tableColList = getColumnList();
    colStats_.insertByPosition(stats, columnList, tableColList); 

    // done creating a ColStatDesc for each ColStats;
    // ==> now store the multi-column uec information
    MultiColumnUecList * groupUecs = new (CmpCommon::statementHeap())
      MultiColumnUecList (stats, getColumnList()) ;
    CostScalar rowcount = stats[0]->getRowcount();
    groupUecs->initializeMCUecForUniqueIndxes(*this, rowcount);

    colStats_.setUecList (groupUecs) ;

    if (CmpCommon::getDefault(USTAT_COLLECT_MC_SKEW_VALUES) == DF_ON)
    {
       MultiColumnSkewedValueLists* mcSkewedValueLists= new (CmpCommon::statementHeap())
         MultiColumnSkewedValueLists(stats, getColumnList()) ;
       colStats_.setMCSkewedValueLists (mcSkewedValueLists) ;
    }

    // -------------------------------------------------------------
    // set the UpStatsNeeded flag
    CostScalar needStatsRowcount =
      CostPrimitives::getBasicCostFactor(HIST_ROWCOUNT_REQUIRING_STATS) ;
    if (CURRSTMT_OPTDEFAULTS->ustatAutomation()) needStatsRowcount = 1;

    CMPASSERT ( colStats_.entries() > 0 ) ;  // must have at least one histogram!
    CostScalar tableRowcount = colStats_[0]->getColStats()->getRowcount() ;

    if ( ( tableRowcount >= needStatsRowcount ) &&
	 !(isSpecialObj()) )  // UpStatsNeeded flag is used for 6007 warning,
			     // we do not want to give this warning for
			     // smdTables, umdTables, MVUmd tables and other special tables
      {
        for ( CollIndex i = 0 ; i < colStats_.entries() ; i++ )
          colStats_[i]->getColStatsToModify()->setUpStatsNeeded (TRUE) ;
      }
    // -------------------------------------------------------------

    // ENFORCE ROWCOUNT!  When we read them in, all stats from the same
    // table should report the same rowcount.  Currently there's a
    // problem with SYSKEY histograms having a default rowcount value --
    // which brings up the following question: are SYSKEY histograms
    // ever used in histogram synthesis?
    NABoolean printNoStatsWarning;
    if (isSpecialObj() )
  printNoStatsWarning = FALSE;
    else
  printNoStatsWarning = TRUE;
    colStats_.enforceInternalConsistency(0,colStats_.entries(), printNoStatsWarning) ;

  /*  Estimate index levels based on row count, row size and key size of the index.
  Estimation will be based upon the following assumptions:

  a. There is no slack in data blocks. Slack is  the percentage in the  data block
  that is  kept empty for future inserts into  the block. For  sequential inserts
  DP2  does not leave  any slack in data blocks  but inserts in between  rows
  causes DP2 to  move following rows in the block into a new block.

  b. DP2 encodes and optimizes key storage. Basically it will keeps the portion of
  the key that is  necessary to distinguish the  blocks in next level  of index or
  data blocks. We will assume that the whole key is being used.

  c.  Data is  uniformly distributed  among all  the partitions.  Obviously it  is
  possible that one the partitions has more data thus more data blocks leading  to
  more index level than the other ones.
  */

  // compress histograms based on query predicates
  if (CmpCommon::getDefault(COMP_BOOL_18) != DF_ON)
    compressHistogramsForCurrentQuery();

  return colStats_;
}


ValueIdSet TableDesc::getLocalPreds()
{
  ValueIdSet localPreds;
  localPreds.clear();

  // We can get this information from TableAnalysis
  const TableAnalysis * tableAnalysis = getTableAnalysis();

  // if no tableAnalysis exists, return FALSE
  if(tableAnalysis)
    localPreds = tableAnalysis->getLocalPreds();

  return localPreds;
}

// Is there any column which has a local predicates and no stats
NABoolean TableDesc::isAnyHistWithPredsFakeOrSmallSample(const ValueIdSet &localPreds)
{
  // if there are no local predicates return FALSE;
  if (localPreds.isEmpty())
    return FALSE;

  const ColStatDescList & colStatsList = getTableColStats();
  // for each predicate, check to see if stats exist
  for (ValueId id = localPreds.init();
       localPreds.next(id);
       localPreds.advance(id))
       {
		ColStatsSharedPtr colStats = colStatsList.getColStatsPtrForPredicate(id);

		if (colStats == NULL)
		return FALSE;

		if (colStats->isOrigFakeHist() || colStats->isSmallSampleHistogram())
		return TRUE;
       }
       
       return FALSE;
}

// This method computes the base selectivity for this table. It is defined
// cardinality after applying all local predicates with empty input logical
// properties over the base cardinality without hints

CostScalar Scan::computeBaseSelectivity() const
{
  CostScalar scanCardWithoutHint = getGroupAttr()->outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->\
    				  getColStats().getScanRowCountWithoutHint();

  double cardAfterLocalPreds = scanCardWithoutHint.getValue();
  double baseRowCount = getTableDesc()->tableColStats()[0]->getColStats()->getRowcount().getValue() ;
  
  // both the minimum and the base row count have to be minimum 1.
  // This is ensured in the called routines. So no need to check here.

  return cardAfterLocalPreds/baseRowCount;
}

// This method computes the ratio of selectivity obtained with and without hint
// and sets that in the cardinalityHint 

void
TableDesc::setBaseSelectivityHintForScan(CardinalityHint *cardHint,
					 CostScalar baseSelectivity)
{
  double cardinalityHint = cardHint->getScanCardinality().getValue();
  CostScalar baseRowCount = tableColStats()[0]->getColStats()->getRowcount().getValue();
  double selectivityProportion;

  // cardinalityHint is minimum one. This is checked at the time of setting
  // it in the tableDesc. So, is the baseRowCount

  double selectivityFactor = cardinalityHint / baseRowCount.getValue();
  // selectivityProportion becomes invalid if selectivityFactor is
  // less than baseSelectivity. That causes an assertion failure
  // This was never caught earlier, maybe becasue we never tested hints with OR 
  // predicates.
  selectivityProportion = log(selectivityFactor) / log(baseSelectivity.getValue());
  cardHint->setBaseScanSelectivityFactor(selectivityProportion);
  cardHint->setScanSelectivity(selectivityFactor);
  setCardinalityHint(cardHint);
  return;
}

// This method computes the ratio of selectivity obtained with and without hint
// and sets that in the selectivityHint 

void
TableDesc::setBaseSelectivityHintForScan(SelectivityHint *selHint,
					 CostScalar baseSelectivity)
{
  double selectivityFactor = selHint->getScanSelectivityFactor();

  double selectivityProportion;
  if (selectivityFactor == 0)
    selectivityProportion = 1.0;
  else
    selectivityProportion = log(selectivityFactor) / log(baseSelectivity.getValue());

  selHint->setBaseScanSelectivityFactor(selectivityProportion);
  setSelectivityHint(selHint);
  return;
}


// -----------------------------------------------------------------------
// Print function for TableDesc
// -----------------------------------------------------------------------
void TableDesc::print(FILE* ofd, const char* indent, const char* title)
{
#ifndef NDEBUG
  BUMP_INDENT(indent);
  cout << title << " " << this << " NATable=" << (void*)table_
	<< " ix=" << indexes_.entries() << "," << clusteringIndex_
	<< " stat=" << colStats_.entries() << endl;
  for (CollIndex i = 0; i < indexes_.entries(); i++)
    indexes_[i]->print(ofd, indent, "TableDesc::indexes");
  clusteringIndex_->print(ofd, indent, "TableDesc::clusteringIndex_");
  corrName_.print(ofd, indent);
  colList_.print(ofd, indent, "TableDesc::colList_");
  colVEGList_.print(ofd, indent, "TableDesc::colVEGList_");
  cout << "cnstrnt=" << checkConstraints_.entries() << endl << endl;

#endif
} // TableDesc::print()

// -----------------------------------------------------------------------
// Print function for TableDescList
// -----------------------------------------------------------------------
void TableDescList::print(FILE* ofd, const char* indent, const char* title)
{
#ifndef NDEBUG
  BUMP_INDENT(indent);

  for (CollIndex i = 0; i < entries(); i++)
    {
      fprintf(ofd,"%s%s[%2d] = (%p)\n",NEW_INDENT,title,i,at(i));
      // at(i)->print(ofd,indent);
    }
#endif
} // TableDescList::print()
CardinalityHint::CardinalityHint(CostScalar scanCardinality)
{
  scanCardinality_ = scanCardinality;
  scanSelectivity_ = -1.0;
  localPreds_.clear();
  baseSelectivity_ = -1.0;
}

// constructor defined with local predicates
CardinalityHint::CardinalityHint(CostScalar scanCardinality,
				 const ValueIdSet & localPreds)
{
  scanCardinality_ = scanCardinality;
  scanSelectivity_ = -1.0;
  localPreds_ = localPreds;
  baseSelectivity_ = -1.0;
}
SelectivityHint::SelectivityHint(double selectivityFactor)
{
  selectivityFactor_ = selectivityFactor;
  localPreds_.clear();
  baseSelectivity_ = -1.0;
}

void SelectivityHint::setScanSelectivityFactor (double selectivityFactor)
{
  // This method is called only for selectivityFactor >= 0.0

  if (selectivityFactor > 1.0)
    selectivityFactor_ = 1.0;
  else
    selectivityFactor_ = selectivityFactor ;
}

CostScalar
TableDesc::getBaseRowCntIfUniqueJoinCol(const ValueIdSet &joinedCols)

{
  // get the joining columns for this table
  ValueIdList userColumns;

  // get All user columns for this table;
  getUserColumnList(userColumns);
  ValueIdSet userColumnSet(userColumns);

  ValueIdSet joinedColsCopy(joinedCols);

  ValueIdSet thisTableJoinCols = joinedColsCopy.intersect(userColumnSet);

  if (thisTableJoinCols.isEmpty() )
	return csMinusOne;

  CostScalar baseRowCount = csMinusOne;

  if (thisTableJoinCols.doColumnsConstituteUniqueIndex(this) )
    baseRowCount = tableColStats()[0]->getColStats()->getRowcount();

  return baseRowCount;

} // TableDesc::getBaseRowCntIfUniqueJoinCol


ValueIdSet TableDesc::getComputedColumns(NAColumnBooleanFuncPtrT fptr)
{
  ValueIdSet computedColumns;

  for (CollIndex j=0; j<getClusteringIndex()->getIndexKey().entries(); j++)
    {
      ItemExpr *ck = getClusteringIndex()->getIndexKey()[j].getItemExpr();

      if (ck->getOperatorType() == ITM_INDEXCOLUMN)
        ck = ((IndexColumn *) ck)->getDefinition().getItemExpr();

      CMPASSERT(ck->getOperatorType() == ITM_BASECOLUMN);

      NAColumn* x = ((BaseColumn *) ck)->getNAColumn();

      if (((*x).*fptr)()) 
         computedColumns += ck->getValueId();
    }
   return computedColumns;
}


ValueIdSet TableDesc::getSaltColumnAsSet()
{
  return getComputedColumns(&NAColumn::isSaltColumn);
}

ValueIdSet TableDesc::getDivisioningColumns() 
{
  return getComputedColumns(&NAColumn::isDivisioningColumn);
}

// compress the histograms based on query predicates on this table
void TableDesc::compressHistogramsForCurrentQuery()
{

  // if there are some column statistics
  if ((colStats_.entries() != 0) &&
      (table_) &&
      (table_->getExtendedQualName().getSpecialType() == ExtendedQualName::NORMAL_TABLE))
  { // if 1
    // check if query analysis info is available
    if(QueryAnalysis::Instance()->isAnalysisON())
    { // if 2
      // get a handle to the query analysis
      QueryAnalysis* queryAnalysis = QueryAnalysis::Instance();

      // get a handle to the table analysis
      const TableAnalysis * tableAnalysis = getTableAnalysis();

      if(!tableAnalysis)
        return;

      // iterate over statistics for each column
      for(CollIndex i = 0; i < colStats_.entries(); i++)
      { // for 1
        // Get a handle to the column's statistics descriptor
        ColStatDescSharedPtr columnStatDesc = colStats_[i];

        // get a handle to the ColStats
        ColStatsSharedPtr colStats = columnStatDesc->getColStats();

        // if this is a single column, as opposed to a multicolumn
        if(colStats->getStatColumns().entries() == 1)
        { // if 3
          // get column's value id
          const ValueId columnId = columnStatDesc->getColumn();

          // get column analysis
          ColAnalysis* colAnalysis = queryAnalysis->getColAnalysis(columnId);

          if(!colAnalysis) continue;

          ValueIdSet predicatesOnColumn =
            colAnalysis->getReferencingPreds();

          // we can compress this column's histogram if there
          // is a equality predicate against a constant

          ItemExpr *constant = NULL;

          NABoolean colHasEqualityAgainstConst =
            colAnalysis->getConstValue(constant);

          // if a equality predicate with a constant was found
          // i.e. predicate of the form col = 5
          if (colHasEqualityAgainstConst)
          { // if 4
            if (constant)
              // compress the histogram
              columnStatDesc->compressColStatsForQueryPreds(constant,constant);
          } // if 4
          else{ // else 4

            // since there is no equality predicates we might still
            // be able to compress the column's histogram based on
            // range predicates against a constant. Following are
            // examples of such predicates
            // * col > 1 <-- predicate defines a lower bound
            // * col < 3 <-- predicate defines a upper bound
            // * col >1 and col < 30 <-- window predicate, define both bounds
            ItemExpr * lowerBound = NULL;
            ItemExpr * upperBound = NULL;

            // Extract predicates from range spec and add it to the
            // original predicate set otherwise isARangePredicate() will
            // return FALSE, so histgram compression won't happen.
            ValueIdSet rangeSpecPred(predicatesOnColumn);
            for (ValueId predId= rangeSpecPred.init();
                                 rangeSpecPred.next(predId);
                                 rangeSpecPred.advance(predId))
            {
              ItemExpr * pred = predId.getItemExpr();
              if ( pred->getOperatorType() == ITM_RANGE_SPEC_FUNC )
              {
                ValueIdSet vs;
                ((RangeSpecRef *)pred)->getValueIdSetForReconsItemExpr(vs);
                // remove rangespec vid from the original set
                predicatesOnColumn.remove(predId);
                // add preds extracted from rangespec to the original set
                predicatesOnColumn.insert(vs);
              }
            }

            // in the following loop we iterate over all the predicates
            // on this column. If there is a range predicate e.g. a > 2
            // or a < 3, then we use that to define upper and lower bounds.
            // Given predicate a > 2, we get a lower bound of 2.
            // Given predicate a < 3, we get a upper bound of 3.
            // The bound are then passed down to the histogram
            // compression methods.

            // iterate over predicates to see if any of them is a range
            // predicate e.g. a > 2
            for (ValueId predId= predicatesOnColumn.init();
                                 predicatesOnColumn.next(predId);
                                 predicatesOnColumn.advance(predId))
            { // for 2
              // check if this predicate is a range predicate
              ItemExpr * predicateOnColumn = predId.getItemExpr();
              if (predicateOnColumn->isARangePredicate())
              { // if 5

                // if a predicate is a range predicate we need to find out more
                // information regarding the predicate to see if it can be used
                // to compress the columns histogram. We look for the following:
                // * The predicate is against a constant e.g. a > 3 and not against
                //   another column e.g. a > b
                // Also give a predicate we need to find out what side is the column
                // and what side is the constant. Normally people write a range predicate
                // as a > 3, but the same could be written as 3 < a.
                // Also either on of the operands of the range predicate might be
                // a VEG, if so then we need to dig into the VEG to see where is
                // the constant and where is the column.

                // check the right and left children of this predicate to
                // see if one of them is a constant
                ItemExpr * leftChildItemExpr = (ItemExpr *) predicateOnColumn->getChild(0);
                ItemExpr * rightChildItemExpr = (ItemExpr *) predicateOnColumn->getChild(1);

                // by default assume the literal is at right i.e. predicate of
                // the form a > 2
                NABoolean columnAtRight = FALSE;

                // check if right child of predicate is a VEG
                if ( rightChildItemExpr->getOperatorType() == ITM_VEG_REFERENCE)
                { // if 6
                  // if child is a VEG
                  VEGReference * rightChildVEG = (VEGReference *) rightChildItemExpr;

                  // check if the VEG contains the current column
                  // if it does contain the current column then
                  // the predicate has the column on right and potentially
                  // a constant on the left.
                  if(rightChildVEG->getVEG()->getAllValues().contains(columnId))
                  { // if 7
                    // column is at right i.e. predicate is of the form
                    // 2 < a
                    columnAtRight = TRUE;
                  } // if 7
                } // if 6
                else{ // else 6
                  // child is not a VEG
                  if ( columnId == rightChildItemExpr->getValueId() )
                  { // if 8
                    // literals are at left i.e. predicate is of the form
                    // (1,2) < (a, b)
                    columnAtRight = TRUE;
                  } // if 8
                } // else 6

                ItemExpr * potentialConstantExpr = NULL;

                // check if the range predicate is against a constant
                if (columnAtRight)
                { // if 9
                  // the left child is potentially a constant
                  potentialConstantExpr = leftChildItemExpr;
                } // if 9
                else{ // else 9
                  // the right child is potentially a constant
                  potentialConstantExpr = rightChildItemExpr;
                } // else 9

                // initialize constant to NULL before
                // looking for next constant
                constant = NULL;

                // check if potentialConstantExpr contains a constant.
                // we need to see if this range predicate is a predicate
                // against a constant e.g col > 1 and not a predicate
                // against another column e.g. col > anothercol

                // if the expression is a VEG
                if ( potentialConstantExpr->getOperatorType() == ITM_VEG_REFERENCE)
                { // if 10

                  // expression is a VEG, dig into the VEG to
                  // get see if it contains a constant
                  VEGReference * potentialConstantExprVEG =
                    (VEGReference *) potentialConstantExpr;

                  potentialConstantExprVEG->getVEG()->\
                    getAllValues().referencesAConstValue(&constant);
                } // if 10
                else{ // else 10

                  // express is not a VEG, it is a constant
                  if ( potentialConstantExpr->getOperatorType() == ITM_CONSTANT )
                    constant = potentialConstantExpr;
                } // else 10

                // if predicate involves a constant, does the constant imply
                // a upper bound or lower bound
                if (constant)
                { // if 11
                  // if range predicate has column at right e.g. 3 > a
                  if (columnAtRight)
                  { // if 12
                    if ( predicateOnColumn->getOperatorType() == ITM_GREATER ||
                         predicateOnColumn->getOperatorType() == ITM_GREATER_EQ)
                    { // if 13
                      if (!upperBound)
                        upperBound = constant;
                    } // if 13
                    else
                    { // else 13
                      if (!lowerBound)
                        lowerBound = constant;
                    } // else 13
                  } // if 12
                  else{ // else 12
                    // range predicate has column at left e.g. a < 3
                    if ( predicateOnColumn->getOperatorType() == ITM_LESS ||
                         predicateOnColumn->getOperatorType() == ITM_LESS_EQ)
                    { // if 14
                      if (!upperBound)
                        upperBound = constant;
                    } // if 14
                    else
                    { // else 14
                      if (!lowerBound)
                        lowerBound = constant;
                    } // else 14
                  } // else 12
                } // if 11
              } // if 5
            } // for 2

            // if we found a upper bound or a lower bound
            if (lowerBound || upperBound)
            {
              // compress the histogram based on range predicates
              columnStatDesc->compressColStatsForQueryPreds(lowerBound, upperBound);
            }
          } // else 4
        } // if 3
      } // for 1
    } // if 2
  } // if 1
	  // All histograms compressed. Set the histCompressed flag to TRUE
	  histsCompressed(TRUE);
}

NABoolean TableDesc::splitHiveLocation(const char *tableLocation,
                                       NAString &hdfsHost,
                                       Int32 &hdfsPort,
                                       NAString &tableDir,
                                       ComDiagsArea *diags,
                                       int hdfsPortOverride)
{
  HHDFSDiags hhdfsDiags;

  NABoolean result = HHDFSTableStats::splitLocation(
       tableLocation,
       hdfsHost,
       hdfsPort,
       tableDir,
       hhdfsDiags,
       hdfsPortOverride);

  if (!result)
    {
      if (!hhdfsDiags.isSuccess())
        {
          if (diags)
            (*diags) << DgSqlCode(-1215)
                     << DgString0(tableLocation)
                     << DgString1(hhdfsDiags.getErrMsg());
        }
      else
        CMPASSERT(0);
    }
  else
    CMPASSERT(hhdfsDiags.isSuccess());

  return result;
}
