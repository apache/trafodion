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
#ifndef TABLEDESC_H
#define TABLEDESC_H
/* -*-C++-*-
**************************************************************************
*
* File:         TableDesc.h
* Description:  A table descriptor
* Created:      4/27/94
* Language:     C++
*
*
*
*
**************************************************************************
*/


#include "BaseTypes.h"
#include "ObjectNames.h"
#include "ColStatDesc.h"
#include "IndexDesc.h"
#include "ItemConstr.h"
#include "ValueDesc.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class TableDesc;
class TableDescList;
class SelectivityHint;
class CardinalityHint;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class NATable;
class TableAnalysis;

// ***********************************************************************
// TableDesc
//
// One TableDesc is allocated per reference to a qualified table name.
// One or more TableDescs may share a NATable. A TableDesc contains
// some attributes for the table that are specific to a particular
// reference, e.g., the lock mode or CONTROLs that are in effect.
//
// ***********************************************************************

class TableDesc : public NABasicObject
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  TableDesc(BindWA *bindWA, const NATable *table, CorrName &corrName) ;

private:
  TableDesc (const TableDesc &) ; //memleak fix
public:

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  const CorrName &getCorrNameObj() const 	     	  { return corrName_; }
        CorrName &getCorrNameObj()       	     	  { return corrName_; }

  const NAString &getLocationName() const {return corrName_.getLocationName();}
  NABoolean isLocationNameSpecified() const {return corrName_.isLocationNameSpecified();}
  NABoolean isPartitionNameSpecified() const {return corrName_.isPartitionNameSpecified();}
  NABoolean isKeyIndex(const IndexDesc * idesc) const;
  const NATable *getNATable() const                          { return table_; }
  const TableAnalysis *getTableAnalysis() const            { return analysis_; }
  const SelectivityHint * getSelectivityHint()	const	    { return selectivityHint_; }
  SelectivityHint * selectivityHint()	{ return selectivityHint_; }
  const CardinalityHint * getCardinalityHint()	const	    { return cardinalityHint_; }
  CardinalityHint * cardinalityHint()	{ return cardinalityHint_; }

  CostScalar getMinRC() const     { return minRC_ ; };
  void setMinRC(CostScalar rc) { minRC_ = rc; };
  CostScalar getMaxRC() const     { return maxRC_ ; };
  void setMaxRC(CostScalar rc) { maxRC_ = rc; };
  ValueIdSet & predsExecuted() { return predsExecuted_; };
  void setPredsExecuted(ValueIdSet pe) { predsExecuted_ = pe; };

  const ValueIdList &getColumnList() const            	   { return colList_; }
  const ValueIdList &getColumnVEGList() const      	{ return colVEGList_; }

  void getUserColumnList(ValueIdList &userColList) const;
  void getSystemColumnList(ValueIdList &systemColList) const;
  void getIdentityColumn(ValueIdList &systemColList) const;
  NABoolean isIdentityColumnGeneratedAlways(NAString * value = NULL) const; 
  
  const LIST(IndexDesc *) &getIndexes() const              { return indexes_; }
  const LIST(IndexDesc *) &getUniqueIndexes() const        { return uniqueIndexes_; }
  NABoolean hasUniqueIndexes() const       { return uniqueIndexes_.entries() > 0; }

  NABoolean hasSecondaryIndexes() const      { return indexes_.entries() > 1; }
  const LIST(IndexDesc *) &getVerticalPartitions() const { return vertParts_; }
  const IndexDesc * getClusteringIndex() const     { return clusteringIndex_; }

  const LIST(const IndexDesc *) &getHintIndexes() const{ return hintIndexes_; }
  NABoolean hasHintIndexes() const       { return hintIndexes_.entries() > 0; }

  const ValueIdList &getColUpdated() const              { return colUpdated_; }
  const ValueIdList &getCheckConstraints() const  { return checkConstraints_; }
  ValueIdList &checkConstraints() 		  { return checkConstraints_; }

  const ColStatDescList &getTableColStats();
  ColStatDescList &tableColStats()
                         { return (ColStatDescList &)getTableColStats(); }

  NABoolean areHistsCompressed() {return histogramsCompressed_;}

  void histsCompressed(NABoolean flag) { histogramsCompressed_ = flag;}

  // Given a list of base columns, return the corresponding VEG columns
  // which maps base columns to index columns.
  void getEquivVEGCols (const ValueIdList &columnList,
			ValueIdList &VEGColumnList) const;
  void getEquivVEGCols (const ValueIdSet &columnSet,
			ValueIdSet &VEGColumnSet) const;
  ValueId getEquivVEGCol (const ValueId &column) const;
  NABoolean isSpecialObj();

  CostScalar getBaseRowCntIfUniqueJoinCol(const ValueIdSet &joinedCols);

  const ValueIdSet getPrimaryKeyColumns()   { return primaryKeyColumns_; }

  // ---------------------------------------------------------------------
  // Mutator functions
  // ---------------------------------------------------------------------
  void addCheckConstraint(BindWA *bindWA,
  			  const NATable *naTable,
			  const CheckConstraint *constraint,
			  ItemExpr *constraintPred);

  void clearColumnList()                                 { colList_.clear(); }
  void addToColumnList(const ValueId &colId)       { colList_.insert(colId); }
  void addToColumnList(const ValueIdList &clist)   { colList_.insert(clist); }
  void addToColumnVEGList(const ValueId &colId) { colVEGList_.insert(colId); }
  void addColUpdated(const ValueId &colId)      { colUpdated_.insert(colId); }
  void addIndex(IndexDesc *idesc)                  { indexes_.insert(idesc); }
  void addUniqueIndex(IndexDesc *idesc)		   { uniqueIndexes_.insert(idesc); }
  void addVerticalPartition(IndexDesc *idesc)    { vertParts_.insert(idesc); }
  void addHintIndex(const IndexDesc *idesc)    { hintIndexes_.insert(idesc); }
  void setClusteringIndex(IndexDesc *idesc)      { clusteringIndex_ = idesc; }
  void setCorrName(const CorrName &corrName)	     { corrName_ = corrName; }
  void setLocationName(const NAString &locName) {corrName_.setLocationName(locName);}
  void setTableAnalysis(TableAnalysis *analysis)      {analysis_ = analysis; }
  void setSelectivityHint(SelectivityHint *hint)      {selectivityHint_ = hint; }
  void setCardinalityHint(CardinalityHint *hint)      {cardinalityHint_ = hint; }
  void setPrimaryKeyColumns();

  ValueIdList &hbaseTSList() { return hbaseTSList_; }
  ValueIdList &hbaseVersionList() { return hbaseVersionList_; }

  // ---------------------------------------------------------------------
  // Needed by Collections classes
  // ---------------------------------------------------------------------
//  NABoolean operator == (const TableDesc & rhs) { return (table_ == rhs.table_) &&
//							 (corrName_ == rhs.corrName_); }
  NABoolean operator == (const TableDesc & rhs) { return (&(*this) == &rhs); }


  // ---------------------------------------------------------------------
  // Print/debug
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
		      const char* indent = DEFAULT_INDENT,
                      const char* title = "TableDesc");

  //
  // 64-bit Project: Cast 'this' to "long" first to avoid C++ error
  //
#ifdef NA_64BIT
  ULng32 hash() const { return (ULng32) ((Long)this/8);}
#else
  ULng32 hash() const { return (ULng32) this/4;}
#endif


  // get local predicates for this table
  ValueIdSet getLocalPreds();

  // Is there any column which has a local predicates and no or dirty stats
  NABoolean isAnyHistWithPredsFakeOrSmallSample(const ValueIdSet &localPreds);


  // This method computes the ratio of selectivity obtained with and without hint
  // and sets that in the Hint

  void setBaseSelectivityHintForScan(CardinalityHint *cardHint,
					 CostScalar baseSelectivity);

  void setBaseSelectivityHintForScan(SelectivityHint *selHint,
					 CostScalar baseSelectivity);

  ValueIdSet getDivisioningColumns() ;

  ValueIdSet getSaltColumnAsSet() ;
  NABoolean hasIdentityColumnInClusteringKey() const ;

  // helper function for Hive tables
  static NABoolean splitHiveLocation(const char *tableLocation,
                                     NAString &hdfsHost,
                                     Int32 &hdfsPort,
                                     NAString &tableDir,
                                     ComDiagsArea *diags,
                                     int hdfsPortOverride);

private:

  ValueIdSet getComputedColumns(NAColumnBooleanFuncPtrT fptr);

  // compress the histograms based on query predicates on this table
  void compressHistogramsForCurrentQuery();

  // ---------------------------------------------------------------------
  // The table name
  // ---------------------------------------------------------------------
  CorrName corrName_;

  // ---------------------------------------------------------------------
  // Table object
  // ---------------------------------------------------------------------
  const NATable *table_;

  // ---------------------------------------------------------------------
  // A List of ValueIds that contains the identifers for the columns
  // provided by this reference to the NATable.
  // ---------------------------------------------------------------------
  ValueIdList  colList_;

  // ---------------------------------------------------------------------
  // A list of VEG expressions and/or base columns that show the
  // equivalences of the base columns with index columns
  // ---------------------------------------------------------------------
  ValueIdList  colVEGList_;

  // ---------------------------------------------------------------------
  // List of indexes (including clustering index and unique index)
  // ---------------------------------------------------------------------
  LIST(IndexDesc *) indexes_;
  IndexDesc *clusteringIndex_;
  LIST(IndexDesc *) uniqueIndexes_;

  // ---------------------------------------------------------------------
  // List of vertical partitions
  // ---------------------------------------------------------------------
  LIST(IndexDesc *) vertParts_;

  // ---------------------------------------------------------------------
  // List of recommended indexes by user hints
  // ---------------------------------------------------------------------
  LIST(const IndexDesc *) hintIndexes_;

  // ---------------------------------------------------------------------
  // List of available column statistics
  // ---------------------------------------------------------------------
  ColStatDescList colStats_;

  // -------------------------------------------------------------------
  // Are histograms of this table compressed
  // --------------------------------------------------------------------
  NABoolean histogramsCompressed_;

  // ---------------------------------------------------------------------
  // A list of expressions, each of which represents a check constraint.
  // ---------------------------------------------------------------------
  ValueIdList checkConstraints_;

  // ---------------------------------------------------------------------
  // List of columns being updated
  // ---------------------------------------------------------------------
  ValueIdList colUpdated_;

  // ---------------------------------------------------------------------
  // The table analysis result from query analyzer
  // ---------------------------------------------------------------------
  TableAnalysis *analysis_;


  // ---------------------------------------------------------------------
  // primary key columns. This is used by GroupByAgg to compute dependency
  // of columns
  // ----------------------------------------------------------------------
  ValueIdSet primaryKeyColumns_;

  // selectivity hint contains the hint given by the user, and all the
  // local predicates on that table to which it corresponds to

  SelectivityHint * selectivityHint_;

  // cardinality hint contains the hint given by the user, and all the
  // local predicates on that table to which it corresponds to

  CardinalityHint * cardinalityHint_;

  // min and max rowcount based on actual cound obtained after executing the query
  // on the sample
  CostScalar minRC_;
  CostScalar maxRC_;
  ValueIdSet predsExecuted_;

  // ---------------------------------------------------------------------
  // Access mode
  // Lock mode
  // CONTROLs that are in effect
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // A List of ValueIds for hbase timestamp values for each of the column
  // in colList_.
  // ---------------------------------------------------------------------
  ValueIdList  hbaseTSList_;

  // ---------------------------------------------------------------------
  // A List of ValueIds for hbase version values for each of the column
  // in colList_.
  // ---------------------------------------------------------------------
  ValueIdList  hbaseVersionList_;


}; // class TableDesc

// ***********************************************************************
// Implementation for inline functions
// ***********************************************************************

// ***********************************************************************
// A list of TableDescs
// ***********************************************************************

class TableDescList : public LIST(TableDesc *)
{
public:
  TableDescList(CollHeap* h/*=0*/): LIST(TableDesc *)(h) { }

  // ---------------------------------------------------------------------
  // Print
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout,
		      const char* indent = DEFAULT_INDENT,
                      const char* title = "TableDescList");
}; // class TableDescList

class SelectivityHint : public NABasicObject
{
public:
  SelectivityHint(double selectivityFactor = -1.0);

  // Destructor
// LCOV_EXCL_START :dd
  virtual ~SelectivityHint()
  {}
// LCOV_EXCL_STOP

  inline double getScanSelectivityFactor () const { return selectivityFactor_     ; }
  void setScanSelectivityFactor (double selectivityFactor);

  inline const ValueIdSet & localPreds() const { return localPreds_; };
  void setLocalPreds(const ValueIdSet &lop) { localPreds_ = lop; }

  inline double getBaseScanSelectivityFactor () const { return baseSelectivity_     ; }
  void setBaseScanSelectivityFactor (double baseSelectivity) {baseSelectivity_ = baseSelectivity; }

private:
  // selectivity hint given by the user in the Select statement
  double selectivityFactor_;

  // set of local predicates on the table for which the hint is given
  ValueIdSet localPreds_;

  // base selectivity obtained after applying all local predicates on a table
  double baseSelectivity_;

};

class CardinalityHint : public NABasicObject
{
public:
  CardinalityHint(CostScalar scanCardinality = 0.0);

  CardinalityHint(CostScalar scanCardinality,
    const ValueIdSet & localPreds);

  // Destructor
// LCOV_EXCL_START :dd
  virtual ~CardinalityHint()
  {}
// LCOV_EXCL_STOP

  inline CostScalar getScanCardinality () const { return scanCardinality_     ; }
  void setScanCardinality (CostScalar scanCardinality) { scanCardinality_ = MIN_ONE_CS(scanCardinality); }

  inline const ValueIdSet & localPreds() const { return localPreds_; };
  void setLocalPreds(const ValueIdSet &lop) { localPreds_ = lop; }

  inline double getBaseScanSelectivityFactor () const { return baseSelectivity_     ; }
  void setBaseScanSelectivityFactor (double baseSelectivity) {baseSelectivity_ = baseSelectivity; }

  inline CostScalar getScanSelectivity () const { return scanSelectivity_     ; }
  void setScanSelectivity (CostScalar scanSelectivity) {scanSelectivity_ = scanSelectivity; }

private:
  // selectivity hint given by the user in the Select statement
  CostScalar scanCardinality_;

  // selectivity hint given by the user in the Select statement
  CostScalar scanSelectivity_;

  // set of local predicates on the table for which the hint is given
  ValueIdSet localPreds_;

    // base selectivity obtained after applying all local predicates on a table
  double baseSelectivity_;

};

#endif  /* TABLEDESC_H */
