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
 * File:         IndexDesc.C
 * Description:  Index descriptors
 *               Index descriptors contain the value ids, uniqueness, key
 *               and ordering information for a particular TableDesc
 * Created:      4/21/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "ValueDesc.h"
#include "PartFunc.h"
#include "IndexDesc.h"
#include "RelScan.h"
#include "CmpContext.h"
#include "CostScalar.h"
#include "ScanOptimizer.h"
#include "AppliedStatMan.h"

// -----------------------------------------------------------------------
// make an IndexDesc from an existing TableDesc and an NAFileSet
// -----------------------------------------------------------------------
IndexDesc::IndexDesc(TableDesc *tdesc, 
                     NAFileSet *fileSet, 
                     CmpContext* cmpContext)
     : tableDesc_(tdesc), clusteringIndexFlag_(FALSE), 
       partFunc_(NULL),
       fileSet_(fileSet), cmpContext_(cmpContext), scanBasicCosts_(NULL)
{
  DCMPASSERT( tdesc != NULL AND fileSet != NULL );

  Lng32 ixColNumber;
  ValueId keyValueId;
  ValueId baseValueId;

  const NATable *naTable = tdesc->getNATable();

  indexLevels_ = fileSet_->getIndexLevels();

  // ---------------------------------------------------------------------
  // Make the column list for the index or vertical partition.
  // Any reference to index also holds for vertical partitions.
  // ---------------------------------------------------------------------
  const NAColumnArray & allColumns = fileSet_->getAllColumns();

  // any index gets a new set of IndexColumn
  // item expressions and new value ids
  CollIndex i = 0;
  for (i = 0; i < allColumns.entries(); i++)
    {
      ItemExpr *baseItemExpr = NULL;

      // make a new IndexColumn item expression, indicate how it is
      // defined (in terms of base table columns) and give a value
      // id to the new IndexColumn expression
      if (allColumns[i]->getPosition() >= 0)
	{
	  baseValueId =
	    tdesc->getColumnList()[allColumns[i]->getPosition()];
	  baseItemExpr = baseValueId.getItemExpr();
	}
      else
	{
	  // this column doesn't exist in the base table.
	  // This is the KEYTAG column of sql/mp indices.
	  ItemExpr * keytag = new(wHeap())
            NATypeToItem((NAType *)(allColumns[i]->getType()));
	  keytag->synthTypeAndValueId();
	  baseValueId = keytag->getValueId();

	  baseItemExpr = NULL;
	}

#pragma nowarn(1506)   // warning elimination 
      IndexColumn *ixcol = new(wHeap()) IndexColumn(fileSet_,i,baseValueId);
#pragma warn(1506)  // warning elimination 
      ixcol->synthTypeAndValueId();

      // add the newly obtained value id to the index column list
      indexColumns_.insert(ixcol->getValueId());

      // if the index column is defined as a 1:1 copy of a base
      // column, add it as an equivalent index column (EIC) to the
      // base column item expression
      if ((baseItemExpr) &&
	  (baseItemExpr->getOperatorType() == ITM_BASECOLUMN))
	((BaseColumn *) baseItemExpr)->addEIC(ixcol->getValueId());
    }

  // ---------------------------------------------------------------------
  // make the list of access key columns in the index and make a list
  // of the order that the index provides
  // ---------------------------------------------------------------------
  const NAColumnArray & indexKeyColumns = fileSet_->getIndexKeyColumns();
  for (i = 0; i < indexKeyColumns.entries(); i++)
    {
      // which column of the index is this (usually this will be == i)
#pragma nowarn(1506)   // warning elimination 

      if ( !naTable->isHbaseTable() )
         ixColNumber = allColumns.index(indexKeyColumns[i]);
      else {
         // For Hbase tables, a new NAColumn is created for every column
         // in an index. The above pointer-based lookup for the key column
         // in base table will only find the index column itself. The
         // fix is to lookup by the column name and type as is 
         // implemented by the getColumnPosition() method.
         ixColNumber = allColumns.getColumnPosition(*indexKeyColumns[i]);
         CMPASSERT(ixColNumber >= 0);
      }

#pragma warn(1506)  // warning elimination 

      // insert the value id of the index column into the key column
      // value id list
      keyValueId = indexColumns_[ixColNumber];
      indexKey_.insert(keyValueId);

      // insert the same value id into the order list, if the column
      // is in ascending order, otherwise insert the inverse of the
      // column
      if (indexKeyColumns.isAscending(i))
	{
	  orderOfKeyValues_.insert(keyValueId);
	}
      else
	{
	  InverseOrder *invExpr = new(wHeap())
	    InverseOrder(keyValueId.getItemExpr());
	  invExpr->synthTypeAndValueId();
	  orderOfKeyValues_.insert(invExpr->getValueId());
	}
    }

  // ---------------------------------------------------------------------
  // Find the clustering key columns in the index and store their value
  // ids in clusteringKey_
  // ---------------------------------------------------------------------
  NABoolean found = TRUE;
  const NAColumnArray & clustKeyColumns =
                      naTable->getClusteringIndex()->getIndexKeyColumns();

  for (i = 0; i < clustKeyColumns.entries() AND found; i++)
    {
      // which column of the index is this?
#pragma nowarn(1506)   // warning elimination 
      ixColNumber = allColumns.index(clustKeyColumns[i]);
#pragma warn(1506)  // warning elimination 

      found = (ixColNumber != NULL_COLL_INDEX);

      if (found)
	{
	  // insert the value id of the index column into the clustering key
	  // value id list
	  keyValueId = indexColumns_[ixColNumber];
	  clusteringKey_.insert(keyValueId);
	}
      else
	{
	  // clustering key isn't contained in this index, clear the
	  // list that is supposed to indicate the clustering key
	  clusteringKey_.clear();
	}
    }

  // ---------------------------------------------------------------------
  // make the list of partitioning key columns in the index and make a list
  // of the order that the partitioning provides
  // ---------------------------------------------------------------------
  const NAColumnArray & partitioningKeyColumns 
                                    = fileSet_->getPartitioningKeyColumns();
  for (i = 0; i < partitioningKeyColumns.entries(); i++)
    {
      // which column of the index is this 
#pragma nowarn(1506)   // warning elimination 
      ixColNumber = allColumns.index(partitioningKeyColumns[i]);
#pragma warn(1506)  // warning elimination 

      // insert the value id of the index column into the partitioningkey column
      // value id list
      keyValueId = indexColumns_[ixColNumber];
      partitioningKey_.insert(keyValueId);

      // insert the same value id into the order list, if the column
      // is in ascending order, otherwise insert the inverse of the
      // column
      if (partitioningKeyColumns.isAscending(i))
	{
	  orderOfPartitioningKeyValues_.insert(keyValueId);
	}
      else
	{
	  InverseOrder *invExpr = new(wHeap())
	    InverseOrder(keyValueId.getItemExpr());
	  invExpr->synthTypeAndValueId();
	  orderOfPartitioningKeyValues_.insert(invExpr->getValueId());
	}
    }

  // ---------------------------------------------------------------------
  // If this index is partitioned, find the partitioning key columns
  // and build a partitioning function.
  // ---------------------------------------------------------------------
  if ((fileSet_->getCountOfFiles() > 1) ||
      (fileSet_->getPartitioningFunction() &&
       fileSet_->getPartitioningFunction()->
       isARoundRobinPartitioningFunction()))
    partFunc_ = fileSet_->getPartitioningFunction()->
      createPartitioningFunctionForIndexDesc(this);
  
} // IndexDesc::IndexDesc()


const QualifiedName& IndexDesc::getIndexName() const 
{ 
  return fileSet_->getFileSetName(); 
} // IndexDesc::getIndexName()

const NAString& IndexDesc::getExtIndexName() const 
{ 
  return fileSet_->getExtFileSetName(); 
} // IndexDesc::getExtIndexName()

Lng32 IndexDesc::getRecordLength() const
{ 
  return fileSet_->getRecordLength(); 
} // IndexDesc::getRecordLength() 

Lng32 IndexDesc::getKeyLength() const
{
	return fileSet_->getKeyLength();
} // IndexDesc::getKeyLength()

const NAColumnArray & IndexDesc::getAllColumns() const
{
  return fileSet_->getAllColumns(); 
} // IndexDesc::getAllColumns()




// Partitioning information
NABoolean IndexDesc::isPartitioned() const
{
  if (partFunc_ AND (partFunc_->getCountOfPartitions() > 1))
    return TRUE;
  else
    return FALSE;
}

// Should the plan priority of this index adjusted, due to a hint?
// Returns:
//   0:          if there are no hints or the index isn't mentioned in a hint
//   otherwise:  the value to add to the plan priority
//               (a positive value indicates a more desirable plan)
int IndexDesc::indexHintPriorityDelta() const
{
  if (tableDesc_->getHintIndexes().entries() == 0)
    return 0;

  CollIndex found = tableDesc_->getHintIndexes().index(this);

  if (found == NULL_COLL_INDEX)
    // no hints or index not listed in the hint
    return 0;
  else
    // index mentioned in hint, all indexes get the same priority, so
    // we select the best one based on cost
    return INDEX_HINT_PRIORITY;
}

// Print function
#pragma nowarn(770)   // warning elimination
void IndexDesc::print(FILE* ofd, const char* indent, const char* title)
{
#pragma nowarn(1506)   // warning elimination 
  BUMP_INDENT(indent);
#pragma warn(1506)  // warning elimination  
  cout << title << " " << this << " "
    << indexLevels_ << "," << clusteringIndexFlag_
    << " pf=" << partFunc_ << " fs=" << fileSet_
    << endl;
  indexColumns_.print(ofd, indent, "IndexDesc::indexColumns_");
  indexKey_.print(ofd, indent, "IndexDesc::indexKey_");
  orderOfKeyValues_.print(ofd, indent, "IndexDesc::orderOfKeyValues_");
  clusteringKey_.print(ofd, indent, "IndexDesc::clusteringKey_");
}
#pragma warn(770)  // warning elimination 

// Get the statement heap
CollHeap* IndexDesc::wHeap()
{ 
  return (cmpContext_) ? cmpContext_->statementHeap() : 0; 
}

CostScalar IndexDesc::getKbForLocalPred() const
{
   AppliedStatMan * appStatMan = QueryAnalysis::ASM();
   if ( !appStatMan ) 
      return csMinusOne;

   const TableAnalysis * tAnalysis = getPrimaryTableDesc()->getTableAnalysis();

   if ( !tAnalysis ) 
      return csMinusOne;

   CANodeId tableId = tAnalysis->getNodeAnalysis()->getId();
   const ValueIdList &keys = getIndexKey();

   EstLogPropSharedPtr estLPPtr = appStatMan->
             getStatsForLocalPredsOnPrefixOfColList(tableId, keys);

   if ( ! estLPPtr->getColStats().containsAtLeastOneFake() )
      return estLPPtr->getResultCardinality() * getRecordSizeInKb();
   else
      return csMinusOne;
}

CostScalar
IndexDesc::getKbPerVolume() const
{
  return getRecordSizeInKb() * tableDesc_->getTableColStats()[0]->getColStats()->
	  getRowcount()/ (partFunc_?
			((NodeMap *)(partFunc_->getNodeMap()))->getNumOfDP2Volumes()
			:1);
}

CostScalar
IndexDesc::getRecordSizeInKb() const
{

  // Get this info from the file set:
  // record length is given in bytes, thus divide over
  // 1024 to get Kb:
  // Don't get the ceiling, we want a fraction if
  CostScalar kbPerRow = getNAFileSet()->getRecordLength() / 1024.;

  return kbPerRow;

}

CostScalar
IndexDesc::getBlockSizeInKb() const
{
  // Get this info from the file set:
  // block size is given in bytes, thus divide over
  // 1024 to get Kb:
  CostScalar blockSize = getNAFileSet()->getBlockSize() / 1024.;

  return blockSize;

}


  
CostScalar IndexDesc::getEstimatedRecordsPerBlock() const
{

  // We don't have "spanning rows" so the fractional part
  // is "garbage" and wasted space, therefore 
  // we take the floor of the following measure since
  // a fractional "rows per block" does not make sense:

  const CostScalar blockSize = getNAFileSet()->getBlockSize();

  const CostScalar recordLength = getNAFileSet()->getRecordLength();

  const CostScalar recordsPerBlock = (blockSize / recordLength).getFloor();

  if ((CmpCommon::getDefault(MODE_SEABASE) != DF_ON) ||
      (NOT getPrimaryTableDesc()->getNATable()->isHbaseTable()))
    {
      // A row can never be larger than a block:
      CMPASSERT( recordsPerBlock > csZero );
    }
  else
    {
      // For an hbase table, there is no concept of a SQ like blocksize and
      // the record size can be very large.
      // For now, make recordsPerBlock as 1 if it becomes 0 or less.
      // TBD TBD
      if (recordsPerBlock <= csZero)
	return (CostScalar)1;
    }

  return recordsPerBlock;  

}

// -----------------------------------------------------------------------
// Input: probes, the number of data requests for a
//        subset of data that a physical scan operator issues to
//        DP2.
//        
// Output: The estimated number of index blocks that all of the
//         input requests touch in their way to the data leaves
//         in the B-Tree. 
//        
// -----------------------------------------------------------------------
CostScalar
IndexDesc::getEstimatedIndexBlocksLowerBound(const CostScalar& probes) const
{
  // -----------------------------------------------------------------------
  // This method works by estimating how many blocks all probes
  // touch in their way down in every level of the tree. We need
  // to know the number of blocks in each level.
  // At each level, the number of index blocks touched by all
  // data requests is 
  // MINOF(index blocks in the b-tree at that level, number
  //       of data requests (i.e. probes)).
  // To estimate the number of index blocks at a given level we
  // use:
  // The rule of thumb formula to
  // estimate the number of index blocks in each level of the b-tree:
  // One level: zero blocks (actually one block but i't in cache all the time)
  // Two levels: 5 blocks
  // Three levels: 200 blocks
  // Four levels: 800 blocks
  // T(0) = 0, empty table, no index blocks.
  // T(1) = 0, only a few records, the root node costs 0.
  // T(2) = 5
  // T(n) = T(n-1)*40, n > 2
  // -----------------------------------------------------------------------
  Lng32 levels = getIndexLevels();
  CostScalar indexBlocksLowerBound = csZero;
  if ( levels == 0.0 OR levels == 1.0 )
    {
      // Index blocks touch by all probes for level zero and one
      // (For indexes with one level we assume the blocks are zero
      //  because we assume that the root always stay in cache):
      indexBlocksLowerBound = csZero;
    }
  else 
    {
      CostScalar indexBlocks = 5.0;

      // Index blocks touch by all probes for level two:
      indexBlocksLowerBound = MINOF( indexBlocks, probes );

      // Index blocks touch by all probes for level three and above:
#pragma warning (disable : 4018)   //warning elimination
      for (CollIndex i=2; i < levels; i++)
	{
#pragma warning (default : 4018)   //warning elimination
	  indexBlocks = indexBlocks*40;
	  indexBlocksLowerBound += MINOF( indexBlocks, probes );
	}

    }
  
  return indexBlocksLowerBound;

}

void
IndexDesc::getNonKeyColumnSet(ValueIdSet& nonKeyColumnSet) const
{

  const ValueIdList
    &indexColumns = getIndexColumns(),
    &keyColumns = getIndexKey();

  // clean up input:
  nonKeyColumnSet.clear();

  // Add all index columns
  CollIndex i = 0;
  for (i=0;
       i < indexColumns.entries();
       i++)
    {
      nonKeyColumnSet.insert(indexColumns[i]);
    }


  // And remove all key columns:
  for (i=0;
       i < keyColumns.entries();
       i++)
    {
      nonKeyColumnSet.remove(keyColumns[i]);
      // if this is a secondary index, the base column
      // which is part of the index,
      // may also be present, remove it:
      const ItemExpr *colPtr = keyColumns[i].getItemExpr();
      if (colPtr->getOperatorType()
          ==
          ITM_INDEXCOLUMN)
        {
          const ValueId & colDef = ((IndexColumn *)(colPtr))->getDefinition();
          nonKeyColumnSet.remove(colDef);
        }
    }
    


} // IndexDesc::getNonKeyColumnSet(ValueIdSet& nonKeyColumnSet) const

void
IndexDesc::getNonKeyColumnList(ValueIdList& nonKeyColumnList) const
{

  const ValueIdList
    &indexColumns = getIndexColumns(),
    &keyColumns = getIndexKey();

  // clean up input:
  nonKeyColumnList.clear();

  // Add all index columns
  CollIndex i = 0;
  for (i=0;
       i < indexColumns.entries();
       i++)
    {
      nonKeyColumnList.insert(indexColumns[i]);
    }


  // And remove all key columns:
  for (i=0;
       i < keyColumns.entries();
       i++)
    {
      nonKeyColumnList.remove(keyColumns[i]);
      // if this is a secondary index, the base column
      // which is part of the index,
      // may also be present, remove it:
      const ItemExpr *colPtr = keyColumns[i].getItemExpr();
      if (colPtr->getOperatorType()
          ==
          ITM_INDEXCOLUMN)
        {
          const ValueId & colDef = ((IndexColumn *)(colPtr))->getDefinition();
          nonKeyColumnList.remove(colDef);
        }
    }
} // IndexDesc::getNonKeyColumnList(ValueIdSet& nonKeyColumnSet) const

NABoolean IndexDesc::isUniqueIndex() const
{

  return  getNAFileSet()->uniqueIndex();

#pragma nowarn(269)   // warning elimination 
  ValueIdList nonKeyColumnList;
#pragma warn(269)  // warning elimination 
  getNonKeyColumnList(nonKeyColumnList);
  
  // if there are some non-index-key columns(the key of base table),
  // then this is a unique index. The primary key of base table is
  // not needed to define the key of the index. It is, of course,
  // needed to be present in the index as a non-key column.
  if (nonKeyColumnList.entries() > 0)
    return TRUE;
  else
    return FALSE;
}

/********************************************************************
* Input: Selection predicates for the scan node, boolean indicating if
* it is a indexOnlyIndex, reference parameter that will indicate if 
* IndexJoin is viable or not, GroupAttributes for the group and characteristic
* inputs
* Output: MdamFlag indicating if the index key access is good enough for 
* MDAM access (if a index does not have good MDAM access we have to 
* scan the whole index because single subset also will not have any
* keys to apply)
* IndexJoin flag indicating if index join cost would exceed base table 
* access or not.
********************************************************************/
MdamFlags IndexDesc::pruneMdam(const ValueIdSet& preds,
				  NABoolean indexOnlyIndex,
				  IndexJoinSelectivityEnum& 
				  selectivityEnum /* out*/ ,
				  const GroupAttributes * groupAttr,
				  const ValueIdSet * inputValues) const
{
  CollIndex numEmptyColumns=0;
  CostScalar numSkips = csOne;
  ValueIdSet emptyColumns;
  ValueId vid;
  if(indexOnlyIndex)
    selectivityEnum = INDEX_ONLY_INDEX;
  else
    selectivityEnum = INDEX_JOIN_VIABLE;
  if(preds.isEmpty()) return MDAM_OFF;
  //calculate how many key columns don't have any predicates
  for(CollIndex i=0;i<indexKey_.entries();i++)
  {
    if(preds.referencesTheGivenValue(indexKey_[i],vid))
      break;
    else
      numEmptyColumns++;
  }
  
  //if we don't have any empty columns or we don't have to evaluate if index
  //join is promising or not then just return
  if(numEmptyColumns>=1 OR NOT indexOnlyIndex)
  {
    IndexDescHistograms ixHistogram(*this,
      (indexOnlyIndex?numEmptyColumns:indexKey_.entries()));

    NABoolean multiColUecAvail = ixHistogram.isMultiColUecInfoAvail();
    ColumnOrderList keyPredsByCol(indexKey_);
    for(CollIndex j=0;j<numEmptyColumns;j++)
    {
      emptyColumns.insert(indexKey_[j]);
      if(j==0 OR multiColUecAvail == FALSE)
      {
	//no MCUec so just multiply the empty columns UEC count to 
	//calculate MDAM skips
	numSkips *=(ixHistogram.getColStatsForColumn(indexKey_[j])).
	  getTotalUec().getCeiling();
      }
      else // otherwise try to use MCUec
      {
	
	NABoolean uecFound = FALSE;
	CostScalar correctUec = csOne;
	CostScalar combinedUECCount = csOne;
	// first let's see if there is multiColUec count for the skipped columns
	// so far. If there is that will be number of skips. If there isn't then
	// get the best estimate of UEC count for the current column using MCUec
	// if possible otherwise just using single column histograms. 
	combinedUECCount = ixHistogram.getUecCountForColumns(emptyColumns);
	if(combinedUECCount >0)
	{
	  numSkips = combinedUECCount;
	}
	else
	{
	  uecFound = ixHistogram.estimateUecUsingMultiColUec(keyPredsByCol,j,correctUec);
	  if(uecFound==TRUE)
	  {
	    numSkips *= correctUec;
	  }
	  else
	  {
	    numSkips *=(ixHistogram.getColStatsForColumn(indexKey_[j])).
	    getTotalUec().getCeiling();
	  }
	}
      }
    }


    CostScalar rowCount = ixHistogram.getRowCount();
    CostScalar numIndexBlocks = rowCount /getEstimatedRecordsPerBlock();
    CostScalar numProbes = csOne;
    CostScalar numBaseTableBlocks = csOne;
    CostScalar inputProbes = csOne;

    // Pass any selectivity hint provided by the user
    const SelectivityHint * selHint = tableDesc_->getSelectivityHint();
    const CardinalityHint * cardHint = tableDesc_->getCardinalityHint();

    // If it is an index join then compute the number probes into the base
    // table. If the alternate index is not selective enough, we will have 
    // lots of them making the index quite expensive.
    if(NOT indexOnlyIndex) 
    {
      if((groupAttr->getInputLogPropList()).entries() >0)
      {
	//if there are incoming probes to the index. i.e. if the index join
	//is under another nested join or TSJ then compute result for all 
	//probes. We are using the initial inputEstLogProp to compute the 
	//resulting cardinality. It is possible that for the same group and 
	//different inputEstLogProp would provide less row count per probe.
	//So in FileScanRule::nextSubstitute() we make sure that the context
	//inputEstLogProp is in the error range of this inputEstLogProp. 
	// Ex. select * from lineitem, customer, nation 
	//	  where l_custkey < c_custkey and c_custkey = n_nationkey;
	//Now if we were evaluating lineitem indexes where the outer was customer
	//we would want to exclude alternate index on custkey whereas if nation got
	//pushed below customer then range of values would be fewer and max value
	//being less would make alternate index on custkey quite attractive. 
	
	ixHistogram.
	applyPredicatesWhenMultipleProbes(preds,
					  *((groupAttr->getInputLogPropList())[0]),
					  *inputValues,
 					  TRUE,
					  selHint,
					  cardHint,
					  NULL,
					  REL_SCAN);
	inputProbes = MIN_ONE((groupAttr->getInputLogPropList())[0]->getResultCardinality());
      }
      else
      {
        RelExpr * dummyExpr = new (STMTHEAP) RelExpr(ITM_FIRST_ITEM_OP,
				    NULL,
				    NULL,
				    STMTHEAP);
	ixHistogram.applyPredicates(preds, *dummyExpr, selHint, cardHint, REL_SCAN);
      }

      numProbes = ixHistogram.getRowCount();
      numBaseTableBlocks = rowCount / tableDesc_->getClusteringIndex()->
	getEstimatedRecordsPerBlock();
      double readAhead = CURRSTMT_OPTDEFAULTS->readAheadMaxBlocks();

      // although we compute cardinality from the index for all probes we 
      // do the comparison for per probe. The assumption is that per probe
      // the upper bound of cost is scanning the whole base table.
      if(numProbes/inputProbes + MINOF((numIndexBlocks / readAhead),numSkips)
	> (numBaseTableBlocks/readAhead))
      {
	selectivityEnum = EXCEEDS_BT_SCAN;
      }
    }
    
    //Does the number of skips exceed the cost of scanning the index. 
    if((indexOnlyIndex AND numSkips <= 
      (numIndexBlocks * CURRSTMT_OPTDEFAULTS->mdamSelectionDefault())) OR 
      (NOT indexOnlyIndex AND numSkips + numProbes/inputProbes <= 
		  (numBaseTableBlocks * CURRSTMT_OPTDEFAULTS->mdamSelectionDefault())))
      return MDAM_ON;
  }
  else 
    return MDAM_ON;

  return MDAM_OFF;
}

void IndexProperty::updatePossibleIndexes(SET(IndexProperty *) & indexes, Scan *scan)
{
    // We will compare the new possible index with each one already 
    // in a set. If the new index gives the SAME or LESS promise
    // it will be ignored. If the new index is better than some of
    // existing indexes those will be removed and the new one - added.

    CollIndex numIndexes = indexes.entries(), i=0;
    COMPARE_RESULT comp;
    NABoolean addIndex = TRUE;

    while (i<numIndexes AND addIndex)
    {
        comp = compareIndexPromise(indexes[i]);

        switch(comp)
        {
           
          case LESS:
          case SAME:
            // new index doesn't have a better promise, ignore it.
            addIndex = FALSE;
            break;

          case MORE:
            // new index is more promising than the current one in indexes
            // remove the current index.
            indexes.remove(indexes[i]);
            numIndexes--;
            break;

          case INCOMPATIBLE:
          case UNDEFINED:
            // couldn't decide at this moment, go to the next index
            i++;
            break;

          default:
             // the enumerated type COMPARE_RESULT can not retrun default;
             // so CMPASSERT() is ok
             CMPASSERT(0);
        }

    }

    if ( addIndex )
        indexes.insert(this);

    return;
    
}

COMPARE_RESULT 
IndexProperty::compareIndexPromise(const IndexProperty *ixProp) const
{
    // currently it is the same for indexOnlyScans ans alternateIndexScans.
    // If index key starts from the same (single column) then the smaller index 
    // (having smaller KbPerVolume attribute) is MORE promising, the bigger index
    // is LESS promising, and the same index size has the SAME promise.


    const IndexDesc * index = getIndexDesc();
    const IndexDesc * otherIndex = ixProp->getIndexDesc();
    if ( ((IndexColumn *)(index->getIndexKey()[0]).getItemExpr())->getDefinition() != 
         ((IndexColumn *)(otherIndex->getIndexKey()[0]).getItemExpr())->getDefinition() )

      return INCOMPATIBLE;

     CostScalar myKbForLPred = index->getKbForLocalPred();
     CostScalar othKbForLPred = otherIndex->getKbForLocalPred();
     int myDelta = index->indexHintPriorityDelta();
     int otherDelta = otherIndex->indexHintPriorityDelta();

     if (myDelta != otherDelta)
       {
         // return a result based on index hints given

         if (myDelta > otherDelta)
           // only I am mentioned in the hints
           return MORE;

         return LESS;
       }

     // If stats is available for this and the other index, compare the
     // amount of data accessed through the local predicate. The one
     // that accesses less is more promising. 

     if ( myKbForLPred >= 0 && othKbForLPred >=0 ) 
        {
          if ( myKbForLPred < othKbForLPred )
              return MORE; // more promising
          else {
              if( myKbForLPred > othKbForLPred )
                 return LESS;
              else {  

               // When the amount of data to access is the same, prefer 
               // the index with less # of index key columns

               CollIndex myNumKeyCols = index->getIndexKey().entries();
               CollIndex otherNumKeyCols = otherIndex->getIndexKey().entries();

               if ( myNumKeyCols < otherNumKeyCols )
                 return MORE; // more promissing

               if ( myNumKeyCols > otherNumKeyCols )
                 return LESS;
               else
                 return SAME;
              }
         }
       } 
     
    return INCOMPATIBLE;
}
// eof
