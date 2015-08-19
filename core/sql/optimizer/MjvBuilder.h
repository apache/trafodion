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
#ifndef MJV_BUILDER_H
#define MJV_BUILDER_H

/* -*-C++-*-
******************************************************************************
*
* File:         MjvBuilder.h
* Description:  Definition of MjvBuilder class hierarchy as part of
*               MvRefreshBuilder class hierarchy.
*
* Created:      07/02/2002
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "MvRefreshBuilder.h"

// classes defined in this file
class MjvBuilder;
class MjvImmInsertBuilder;
class MjvImmDeleteBuilder;
class MjvImmDirectUpdateBuilder;

// forward declarations
class BindWA;

//----------------------------------------------------------------------------
// Builds the refresh tree of an MJV. This class is abstract, and only the
// derived classes implement the buildRefreshTree() method.
//----------------------------------------------------------------------------
class MjvBuilder : public MvRefreshBuilder {
public:

  MjvBuilder(const CorrName      &mvName,
             MVInfoForDML        *mvInfo,
	     Refresh	         *refreshNode,
	     BindWA              *bindWA)
  : MvRefreshBuilder(mvName, mvInfo, refreshNode, bindWA)
  {}

  virtual ~MjvBuilder() {}

protected:
  // build the scanning node of the MJV
  Scan *buildScanOfMJV(const CorrName &corrName, const NATable *baseNaTable) const;

protected:
  // These methods are for building direct update refresh.
  // Used by direct update immediate refresh and by ON REQUEST refresh.

  // build the new record expression when directly updating the MJV
  ItemExpr *buildUpdatePredicate(const CorrName &inputCorrName,
				 const NATable  *baseNaTable,
				 SET(MVColumnInfo*) SetOfAffectedColumns) const;


  // return the expression tree to be assigned to the MJV column
  ItemExpr *getExpressionTreeForColumn(const MVColumnInfo *affectedCol,
				       const NAString     &colName, 
				       const CorrName     &inputCorrName) const;

  // Construct a set of MVColumnInfo objects for all updated MJV columns.
  SET(MVColumnInfo*) collectAllAffectedColumns(const IntegerList   *updatedCols,
					       const QualifiedName &qualTableName ) const;

  // add the NEW@ correlation name to column names (for computed columns)
  void addCorrNameToExpr(ItemExpr *expr, const CorrName &inputCorrName) const;

  // Used by insert refresh.
  void setupBindContext(RelRoot *topNode, 
			RelExpr *scanBlock,
			const QualifiedName *baseTableName);

private:

  // Copy Ctor and = operator are not implemented
  MjvBuilder(const MjvBuilder &other);
  MjvBuilder& operator=(const MjvBuilder &other);

  // build the selection-predicate for scanning the corresponding rows from the
  // MJV.
  ItemExpr *buildEqualityPredOnClusteringIndex(const CorrName &tableName,
					       const NATable  *baseNaTable) const;
}; // class MjvBuilder

//----------------------------------------------------------------------------
// Abstract class, from which all Immediate MJV refresh builders inherit.
//----------------------------------------------------------------------------
class MjvImmediateBuilder : public MjvBuilder {
public:

  // Ctor for use with ON STATEMENT MJVs
  MjvImmediateBuilder(const CorrName      &mvName,
		      MVInfoForDML        *mvInfo,
		      const GenericUpdate *iudNode,
		      BindWA              *bindWA)
  : MjvBuilder(mvName, mvInfo, NULL, bindWA),
    iudNode_(iudNode)
  {}

  virtual ~MjvImmediateBuilder() {}

protected:
  // Accessors
  inline const GenericUpdate *getIudNode() const { return iudNode_; }

  // builds the refresh tree after direct-update/delete operations
  RelExpr *buildDeleteOrDirectUpdateBlock(const CorrName &corrName, 
					  const NATable *baseNaTable) const;
  
  // build the GU node that updates the MJV
  GenericUpdate *buildGUNodeOfMJV(RelExpr *subTreeBelow) const;

  // build the actual GU node that updates the MJV
  virtual GenericUpdate *buildActualGUOfMJV(RelExpr *subTreeBelow) const = 0;

  // avoid any security check in the built refresh tree
  void avoidSecurityCheckInRefreshTree(RelExpr *refreshTree) const;

protected:
  const GenericUpdate *iudNode_; // The IUD node that started the whole process

}; // class MjvImmediateBuilder

//----------------------------------------------------------------------------
// Builds the refresh tree for ON STATEMENT MJV after an insert operation.
//----------------------------------------------------------------------------
class MjvImmInsertBuilder : public MjvImmediateBuilder {
public:
  MjvImmInsertBuilder(const CorrName      &mvName,
	              MVInfoForDML        *mvInfo,
		      const GenericUpdate *iudNode,
		      BindWA              *bindWA)
  : MjvImmediateBuilder(mvName, mvInfo, iudNode, bindWA),
    optimized_(FALSE) // optimize only on demand
  {}

  virtual RelExpr *buildRefreshTree();

  // flag that the optimized version of the tree if requested
  inline void optimizeForFewRows() { optimized_ = TRUE; };

private:

  // build the actual GU node that updates the MJV
  virtual GenericUpdate *buildActualGUOfMJV(RelExpr *subTreeBelow) const;

  // build the block to replace the scan of the base table in the MJV tree
  RelExpr *buildScanBlock() const;

  // build the replacement block when the optimized tree is requested
  RelExpr *buildOptimizedScanBlock() const;

  NABoolean optimized_; // build the optimized version of the tree?

}; // class MjvImmInsertBuilder

//----------------------------------------------------------------------------
// Builds the refresh tree for ON STATEMENT MJV after a delete operation.
//----------------------------------------------------------------------------
class MjvImmDeleteBuilder : public MjvImmediateBuilder {
public:
  MjvImmDeleteBuilder(const CorrName	  &mvName,
		      MVInfoForDML        *mvInfo,
		      const GenericUpdate *iudNode,
		      BindWA              *bindWA)
  : MjvImmediateBuilder(mvName, mvInfo, iudNode, bindWA)
  {}

  virtual RelExpr *buildRefreshTree();

private:

  // build the actual GU node that updates the MJV
  virtual GenericUpdate *buildActualGUOfMJV(RelExpr *subTreeBelow) const;

}; // class MjvImmDeleteBuilder

//----------------------------------------------------------------------------
// Builds the refresh tree for ON STATEMENT MJV after a direct-update
// operation.
//----------------------------------------------------------------------------
class MjvImmDirectUpdateBuilder : public MjvImmediateBuilder {
public:
  MjvImmDirectUpdateBuilder(const CorrName      &mvName,
              		    MVInfoForDML        *mvInfo,
			    const GenericUpdate *iudNode,
			    BindWA              *bindWA)
  : MjvImmediateBuilder(mvName, mvInfo, iudNode, bindWA)
  {}

  virtual RelExpr *buildRefreshTree();
  
private:

  // build the actual GU node that updates the MJV
  virtual GenericUpdate *buildActualGUOfMJV(RelExpr *subTreeBelow) const;

  IntegerList *StoiToIntegerList(SqlTableOpenInfo *stoi) const;

}; // class MjvImmDirectUpdateBuilder

//----------------------------------------------------------------------------
// Builds the refresh tree for ON REQUEST MJV with a single delta.
//----------------------------------------------------------------------------
class MjvOnRequestBuilder : public MjvBuilder {
public:
  MjvOnRequestBuilder(const CorrName      &mvName,
		      MVInfoForDML        *mvInfo,
		      Refresh             *refreshNode,
		      BindWA              *bindWA)
  : MjvBuilder(mvName, mvInfo, refreshNode, bindWA)
  {}

  virtual RelExpr *buildRefreshTree();

protected:
  static const char *getVirtualIndirectColumnName()  { return virtualIndirectColumnName_; }

  // These methods are implemented here for a single delta, and should be over-ridden 
  // to support multi-delta refresh.
  virtual NABoolean isNeedDeletionBlock();
  virtual NABoolean isNeedInsertionBlock();
  virtual RelExpr *buildMjvDeletionBlock();
  virtual RelExpr *buildMjvInsertJoin();

  // All the rest of the methods provide implementation for a specific delta.
  NABoolean isNeedDeletionBlockSpecific(DeltaDefinition *deltaDef);
  NABoolean isNeedInsertionBlockSpecific(DeltaDefinition *deltaDef);
  NABoolean isNeedDirectUpdateSpecific(DeltaDefinition *deltaDef);

  // Methods for the Deletion Block
  RelExpr *buildMjvDeletionBlockSpecific(DeltaDefinition *deltaDef);
  RelExpr *buildIfNode(RelExpr *leftSide, RelExpr *rightSide);
  RelExpr *buildMjvDeleteSubtree(LogsInfo       *logsInfo, 
                                 const CorrName &logCorrName);
  RelExpr *buildMjvUpdateSubtree(LogsInfo       *logsInfo, 
                                 const CorrName &logCorrName);

  // Methods for the Insertion Block
  RelExpr *buildMjvInsertionBlock();
  RelExpr *buildMjvInsertJoinSpecific(DeltaDefinition *deltaDef);

  // Methods for building the Scan log block - used by both 
  // the Deletion and Insertion blocks.
  RelExpr *buildScanLogForOnRequestMjv(const QualifiedName& baseTableName,
				       NABoolean 	    isForDelete);
  // Override method from MvRefreshBuilder.
  virtual ItemExpr *buildSelectionListForScanOnIudLog() const;
  virtual RelExpr *buildLogsScanningBlock(const QualifiedName& baseTable);
  virtual RelRoot *buildRootOverIUDLog(RelExpr *topNode) const;
  virtual RelRoot *buildRootOverRangeBlock(RelExpr *topNode) const;
  virtual RelRoot *buildRootWithUniformSelectList(RelExpr *topNode, ItemExpr *opExpr, const CorrName *nameOverride) const;

  ItemExpr *buildPredicateOnOpType(ComMvIudLogRowType opType);
  ItemExpr *buildPredicateOnIndirect(OperatorTypeEnum val);
  ItemExpr *buildDeleteSidePredicate();
  ItemExpr *buildInsertSidePredicate();

  // "@LOG" is the correlation name given to the output of the 
  // Scan log block, and used by the Deletion Block.
  // For single delta there is only one delete\update block
  virtual const NAString *getLogName() const
  {
    return new (heap_) NAString("@LOG1");
  }

private:
  static const char virtualIndirectColumnName_[];

}; // class MjvOnRequestBuilder

//----------------------------------------------------------------------------
// Builds the refresh tree for ON REQUEST MJV with multiple delta.
//----------------------------------------------------------------------------
class MjvOnRequestMultiDeltaBuilder : public MjvOnRequestBuilder {
public:
  MjvOnRequestMultiDeltaBuilder(const CorrName	    &mvName,
				MVInfoForDML        *mvInfo,
				Refresh             *refreshNode,
				BindWA              *bindWA)
  : MjvOnRequestBuilder(mvName, mvInfo, refreshNode, bindWA),
    deleteBlockCounter_(1)
  {}

protected:
  
  // Check all the deltas instead og just the first one.
  virtual NABoolean isNeedDeletionBlock(); 
  virtual NABoolean isNeedInsertionBlock();

  // Handle all the deltas instead of just the first one.
  virtual RelExpr *buildMjvDeletionBlock();
  virtual RelExpr *buildMjvInsertJoin();

  virtual const NAString *getLogName() const;

  RelExpr *buildDeltaCalculationTree(const MultiDeltaRefreshMatrix& productMatrix);

  ItemExpr *buildAntiSemiJoinPredicate(NAString rightName, NAString leftName);

private:
  void incrementDeleteBlockCounter()
  {
    deleteBlockCounter_++;
  }

private:
  Int32 deleteBlockCounter_;

}; // class MjvOnRequestMultiDeltaBuilder

#endif
