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
#ifndef MVLOG_H
#define MVLOG_H
/* -*-C++-*-
******************************************************************************
*
* File:         MvLog.h
* Description:  implementation of the MVLOG command
* Created:      09/25/2000
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "RelMisc.h"

// MVLOG command is not supported

class MvLogInternalNames;

//----------------------------------------------------------------------------
// This class implements the MVLOG command, for manual range logging.
// Here is the syntax of the MVLOG command, taken from the external spec
// of the Materialized Views project:
// MVLOG INTO RANGELOG OF [TABLE] table_name (col_name[,col_name...]) 
//       BETWEEN (value[,value...]) AND (value[,value...]);
//
// For example:
// MVLOG INTO RANGELOG OF cdr (date) 
//       BETWEEN (DATE '01/22/1999') AND (DATE '01/22/1999'). 
//
// The BETWEEN predicate must be only on the columns in the clustering index 
// prefix. The main job of the MVLOG command is to divide the range specified
// by the user to sub-ranges according to the way the base table is 
// partitioned. For logging correctness it is imperative that ranges will not
// cross partition boundaries.
//
// MvLog inherits from BinderOnlyNode because it lives only until binding is 
// completed, and is then removed from the RelExpr tree. 
class MvLog : public BinderOnlyNode
{
public:
  MvLog(const QualifiedName *tableName,
	const ItemExpr      *columnNames,
	ItemExpr            *rangeBegin,
	ItemExpr            *rangeEnd,
	CollHeap *oHeap = CmpCommon::statementHeap());
  
  virtual ~MvLog();

  virtual void cleanupBeforeSelfDestruct();

  // All the work is done here.
  RelExpr * bindNode(BindWA *bindWAPtr);

  virtual Int32 getArity() const { return 0; }

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			  LIST(NAString) &llist) const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
			  CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

private: 
  // Methods called from bindNode().
  NABoolean initializeInternalDataStructures();
  void buildBaseTableColumnNamesLists();
  void checkMvLogColsAreCiPrefix() const;
  void buildAllRefNames();
  void getColumnsPositionsInCI(const NAFileSet * pCiFileSet);
  void buildClusteringIndexDefaultValues(const NAFileSet * pCiFileSet);
  void constructBoundriesList();
  void addClusteringIndexBoundries(LIST(ItemExprList*) & ciBoundries,
				   const NAFileSet * pCiFileSet) const;
  void addRangePartitionBoundries(
		LIST(ItemExprList*) & ciBoundries, 
		const RangePartitioningFunction *pRangePartFunction) const;

  NABoolean isTableValidForMvLog() const;
  NABoolean isTableRangePartitioned() const;
  const MvLogInternalNames& getNames() const { return internalNames_; }

  // TUPLE LIST BLOCK methods
  RelRoot     *buildTupleListBlock() const;
  TupleList   *constructTupleListFromBoundries(const LIST(ItemExprList*)& ciBoundries) const;
  RenameTable *constructTupleListPhase1(TupleList *pTupleList) const;
  RenameTable *constructTupleListPhase2(RelExpr * pPhase1) const;
  ItemExpr    *buildTupleListPhase2SelectResults() const;
  RenameTable *buildTupleListPhase2Rename(RelExpr * pTupleListP2) const;
  RenameTable *constructTupleListPhase3(RelExpr * pPhase2) const;
  ItemExpr    *buildTupleListPhase3SelectResults() const;
  RenameTable *buildTupleListPhase3Rename(RelExpr * pPhase3) const;

  // FINAL PHASE
  RelRoot  *constructTupleListBeginGreaterThanEndPhase(RelExpr *pPhase) const;

  // GET FIRST BLOCK methods
  RelExpr  *buildGetFirstNode() const;
  ItemExpr *buildGetFirstSelctionPredicate() const;
  ItemExpr *buildColumnNotEqualBoundriesExpr(ItemExpr * pColumn) const;

  // TSJ
  RelExpr  *buildJoinWithGetFirstBlock(RelExpr *topNode) const;
  
  // returns a BoolValue
  ItemExpr *compareItems(const ItemExpr   *pFirst, 
			 const ItemExpr   *pSecond, 
			 OperatorTypeEnum  compareType,
			 IntegerList      *directionVector) const;  
  
  ItemExpr * buildConditionalColumnList(const ConstStringList&  option1names,
				        const ConstStringList&  option2names,
				        const NAString&         conditionColName,
				        const CorrName&         corrName) const;

  // UNION OVER TSJ  
  RelExpr * buildUnionWithEndRangeTupleList(RelExpr * pTsj) const;
  RelExpr * buildFinalSelectList(RelExpr *topNode);
  RelExpr * buildInsertIntoLog(RelExpr *topNode) const;
  RelExpr * buildScanEpochFromTable(RelExpr *topNode) const;

  void constructEndRangeBoundries(LIST(ItemExprList*)& endRangeBoundry,
                                  NABoolean            needStartRange) const;

 
  ItemExpr *buildEpochSelectList() const;
 

  // For Hash partitioned and non-partitioned tables
  RelExpr *buildSimpleRangeTuple();

  // Shortcut to BinderUtils::appendToExprList();
  void appendToExprList(ItemExprList&            toAddto, 
			const ConstStringList&   columnNames,
			OperatorTypeEnum         itemType = ITM_RENAME_COL) const;

private:
  // Copy Ctor and = operator are not implemented.
  MvLog(const MvLog& other);
  MvLog& operator=(const MvLog& other);

  const QualifiedName	*tableName_;
  const ItemExpr	*pColumnNamesItem_;
  ItemExpr		*pBeginRange_;
  ItemExpr		*pEndRange_;
  NABoolean		 isRangePartitioned_;
  ItemExprList		 pBeginRangeExprList_;
  ItemExprList		 pEndRangeExprList_;
  LIST(ItemExprList*)	 ciBoundries_;	 

  const CorrName   tupleListPhase1CorrName_; 
  const CorrName   tupleListPhase2CorrName_; 
  const CorrName   tupleListPhase3CorrName_; 
  const CorrName   tupleListCorrName_;
  const CorrName   first1NodeCorrName_;
  const CorrName   emptyCorrName_;

  // indicating what part of the CI is used
  IntegerList      pRelevantCiPositionsList_;
  
  ConstStringList  mvLogColNamesList_;    // as recieved from the parser
  ConstStringList  baseTableCiNamesList_; // All the columns of the CI.
  ConstStringList  nonCiColsNamesList_;   // The non-CI column names.
  ItemExprList     nonCiColsExprList_;    // The non-CI column expressions.
  
  ConstStringList  beginColNamesList_;    // with suffix _BEGIN
  ConstStringList  endColNamesList_;	  // with suffix _END
  ConstStringList  first1ColNamesList_;   // with suffix _FIRST
  ConstStringList  brColNamesList_;	  // with suffix _BR
  ConstStringList  erColNamesList_;	  // with suffix _ER

  ItemExprList     colsMinDefualtValList_;
  ItemExprList     colsMaxDefualtValList_;
  IntegerList     *pDirectionVector_;

  static const MvLogInternalNames internalNames_;
  BindWA	   *pBindWA_;
  NATable          *pNaTable_;
  CollHeap         *heap_;

}; // class MvLog

//
class MvLogInternalNames : public NABasicObject
{
public:
  // Accessors for the static const strings.
  static const char *getBeginRangeSuffix() { return beginRangeSuffix_; }
  static const char *getEndRangeSuffix()   { return endRangeSuffix_; }
  static const char *getBrSuffix()         { return brSuffix_; }
  static const char *getErSuffix()         { return erSuffix_; }
  static const char *getEndSuffix()        { return endSuffix_; }
  static const char *getFinalResultName()  { return finalResultName_; }
  static const char *getTupleListName()    { return tupleListName_; }
  static const char *getTupleListRoot1()   { return tupleListRoot1_; }
  static const char *getTupleListRoot2()   { return tupleListRoot2_; }
  static const char *getGetFirstRoot()     { return getFirstRoot_; }
  static const char *getBrIsGreater()      { return brIsGreater_; }
  static const char *getErIsLess()         { return erIsLess_; }
  static const char *getSimpleRangeTuple() { return simpleRangeTuple_; }

private:
  static const char beginRangeSuffix_[];
  static const char endRangeSuffix_[];
  static const char brSuffix_[];
  static const char erSuffix_[];
  static const char endSuffix_[];

  static const char finalResultName_[];
  static const char tupleListName_[];
  static const char tupleListRoot1_[];
  static const char tupleListRoot2_[];
  static const char getFirstRoot_[];
  static const char brIsGreater_[];
  static const char erIsLess_[];
  static const char simpleRangeTuple_[];
};  // class MvLogInternalNames


#endif // MVLOG_H
