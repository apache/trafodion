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
#ifndef INDEXDESC_H
#define INDEXDESC_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         IndexDesc.h
 * Description:  Index descriptors (an IndexDesc is to an index what
 *               a TableDesc is to a base table)
 *               
 * Created:      4/21/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "NABasicObject.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class IndexDesc;

// -----------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------
class TableDesc;
class NATable;
class NAFileSet;
class PartitioningFunction;
class NAColumnArray;
class CmpContext;
class FileScanCostList;
class AccessPathAnalysis;
#ifndef MDAM_FLAGS
#define MDAM_FLAGS
enum MdamFlags	{ UNDECIDED,
		   MDAM_ON,
		   MDAM_OFF
		};
#endif
#ifndef INDEX_JOIN_SLECTIVITY
#define INDEX_JOIN_SLECTIVITY
enum IndexJoinSelectivityEnum { 
				INDEX_ONLY_INDEX,
				INDEX_JOIN_VIABLE, 
				EXCEEDS_BT_SCAN
			      }; 	 
#endif
// -----------------------------------------------------------------------
// An IndexDesc contains value ids for the columns of an index and for
// its key and its ordering. Like a TableDesc mirrors the information
// contained in an NATable object, an IndexDesc mirrors the information
// contained in an NAFileSet object.
// -----------------------------------------------------------------------
class IndexDesc : public NABasicObject
{

public:

  IndexDesc(TableDesc *tdesc, NAFileSet *fileSet, CmpContext* context);

  // accessor functions
  const QualifiedName& getIndexName() const;
  const NAString&  getExtIndexName() const;

  Lng32 getRecordLength() const;
  Lng32 getKeyLength() const;
  Lng32 getIndexLevels() const                     { return indexLevels_; }
  
  const NAColumnArray & getAllColumns() const;
  
  const ValueIdList & getIndexColumns() const	  { return indexColumns_; }
  const ValueIdList & getIndexKey() const	  { return indexKey_; }

  // return the set of index columns that are not part of
  // the key:
  void getNonKeyColumnSet(ValueIdSet& nonKeyColumnSet) const;

  // return the list of index columns that are not part of the key:
  void getNonKeyColumnList(ValueIdList& nonKeyColumnList) const;

  const ValueIdList & getOrderOfKeyValues() const { return orderOfKeyValues_; }
  const ValueIdList & getPartitioningKey() const  { return partitioningKey_; }

  const ValueIdList & getOrderOfPartitioningKeyValues() const 
                                    { return orderOfPartitioningKeyValues_; }

  void setIndexLevels(Lng32 indexLevels) { indexLevels_ = indexLevels ; }
  void setOrderOfKeyValues(const ValueIdList &no) { orderOfKeyValues_ = no; }

  NABoolean containsClusteringKey() const
                                  { return clusteringKey_.entries() > 0; }

  const ValueIdList & getClusteringKeyCols() const 
						{ return clusteringKey_; }

  PartitioningFunction * getPartitioningFunction() const 
                                                     { return partFunc_; }

  NABoolean isClusteringIndex() const     { return clusteringIndexFlag_; }

  const NAFileSet * getNAFileSet() const              { return fileSet_; }

  TableDesc *getPrimaryTableDesc() const      	    { return tableDesc_; }

  MdamFlags pruneMdam(const ValueIdSet& preds,
			 NABoolean indexOnlyIndex,
			 IndexJoinSelectivityEnum& selectivityEnum,
			 const GroupAttributes * groupAttr = NULL,
			 const ValueIdSet * inputValues = NULL) const;
  NABoolean isUniqueIndex() const;

  // Partitioning 
  NABoolean isPartitioned() const;
  
  // mutator functions
  void markAsClusteringIndex()        { clusteringIndexFlag_ = TRUE; }

  // Is it recommended by user hint, and what delta to use
  int indexHintPriorityDelta() const;

  // On return:
  //   the amount of data that the local predicate will produce, when 
  //    table analysis is performed and all stats on the referenced
  //    columns are available and not faked;
  //   -1; otherwise.
  //
  CostScalar getKbForLocalPred() const;

  // new methods added for costing purposes:
  CostScalar getKbPerVolume() const;
  CostScalar getRecordSizeInKb() const;
  CostScalar getBlockSizeInKb() const;
  CostScalar getEstimatedRecordsPerBlock() const;
  CostScalar getEstimatedIndexBlocksLowerBound(const CostScalar& probes) const;
  
  // new access method added for reusing simple cost vectors 
  FileScanCostList * getBasicCostList() const {return scanBasicCosts_;} 

  void setBasicCostList(FileScanCostList * basicCosts )
      { scanBasicCosts_ = basicCosts; }

  AccessPathAnalysis * getAccessPathAnalysis() const {return accessPathAnalysis_;}
  void setAccessPathAnalysis(AccessPathAnalysis * accessPath)
  {  accessPathAnalysis_ = accessPath;}

  // ---------------------------------------------------------------------
  // Print/debug
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
		      const char* indent = DEFAULT_INDENT,
                      const char* title = "IndexDesc");

  // -----------------------------------------------------------------------
  // get the working heap, this heap is expected to be cleanup at the end
  // of statement.
  // -----------------------------------------------------------------------
  CollHeap* wHeap();
  
private:

  // A pointer to the table descriptor for the base table.
  //
  TableDesc *tableDesc_;

  // ---------------------------------------------------------------------
  // The number of levels in the index between the root and a leaf page.
  // If the root is one level from the leaf then indexLevel_ is set to 1.
  // ---------------------------------------------------------------------
  Lng32 indexLevels_;
  
  // ---------------------------------------------------------------------
  // all columns (index key and non-key columns) of the index
  // ---------------------------------------------------------------------
  ValueIdList indexColumns_;

  // ---------------------------------------------------------------------
  // the value ids that comprise the key of the index file
  // ---------------------------------------------------------------------
  ValueIdList indexKey_;

  // ---------------------------------------------------------------------
  // a list of expressions by which the index file is ordered
  // usually same as indexKey_, except for descending order
  // ---------------------------------------------------------------------
  ValueIdList orderOfKeyValues_;

  // ---------------------------------------------------------------------
  // the value ids that comprise the partitioning key of the index file
  // ---------------------------------------------------------------------
  ValueIdList partitioningKey_;

  // ---------------------------------------------------------------------
  // a list of expressions by which the index file is partitioned
  // usually same as partitioningKey_, except for descending order
  // ---------------------------------------------------------------------
  ValueIdList orderOfPartitioningKeyValues_;

  // ---------------------------------------------------------------------
  // The value ids of the IndexColumn exprs that form the clustering key
  // or an empty list, if the clustering key isn't part of the index
  // ---------------------------------------------------------------------
  ValueIdList clusteringKey_;

  // ---------------------------------------------------------------------
  // Flag for indicating whether this is a clustering index.
  // ---------------------------------------------------------------------
  NABoolean clusteringIndexFlag_;

  // ---------------------------------------------------------------------
  // Partitioning function, if the index is horizontally partitioned.
  // ---------------------------------------------------------------------
  PartitioningFunction * partFunc_;

  // ---------------------------------------------------------------------
  // a pointer to the schema information
  // ---------------------------------------------------------------------
  NAFileSet * fileSet_;

  // -----------------------------------------------------------------------
  // a pointer to current CmpContext, containing the global info 
  // -----------------------------------------------------------------------
  CmpContext* cmpContext_; 

  // -----------------------------------------------------------------------
  // a pointer to list of basic cost objects to reuse in FileScanOptimizer  
  // -----------------------------------------------------------------------
  FileScanCostList * scanBasicCosts_; 

  // -----------------------------------------------------------------------
  // a pointer to Acess Path Analysis (generated in the amalyzer)  
  // -----------------------------------------------------------------------
  AccessPathAnalysis * accessPathAnalysis_; 

};

#endif
