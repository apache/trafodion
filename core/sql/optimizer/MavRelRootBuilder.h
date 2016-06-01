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
#ifndef MAVROOTBUILDER_H
#define MAVROOTBUILDER_H

/* -*-C++-*-
******************************************************************************
*
* File:         MavRelRootBuilder.h
* Description:  Definition of class MavRelRootBuilder for MV 
*               INTERNAL REFRESH command.
*
* Created:      07/15/2001
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/


//----------------------------------------------------------------------------
// This class is used by the calcMAVResults method to calculate the CALC
// virtual table.
//----------------------------------------------------------------------------
class MavRelRootBuilder : public NABasicObject
{
public:
  MavRelRootBuilder(const MVInfoForDML *mvInfo, CollHeap *heap);

  virtual ~MavRelRootBuilder();

  void init();

  RelExpr *buildDeltaCalculationRootNodes(RelExpr   *mvSelectTree, 
                                          NABoolean  canSkipMinMax,
				          NABoolean  wasFullDE);
  RelExpr *buildCalcCalculationRootNodes(RelExpr *dcbTree);

protected:
  enum extraColumnType {AGG_FOR_DELETE, AGG_FOR_INSERT};

  // Methods called by buildDeltaCalculationRootNodes()
  void      fixDeltaColumns      (RelExpr   *mvSelectTree, 
                                  NABoolean  canSkipMinMax,
				  NABoolean  wasFullDE);
  RelRoot  *buildRootForNoGroupBy(RelExpr *topNode);
  RelRoot  *buildRootForDelta    (RelExpr *topNode);

  // Methods called by buildCalcCalculationRootNodes()
  RelRoot  *buildRootForCount (RelExpr *topNode);
  RelRoot  *buildRootForSum   (RelExpr *topNode);
  RelRoot  *buildRootForOthers(RelExpr *topNode);
  RelRoot  *buildRootForGop   (RelExpr *topNode);
  ItemExpr *buildSelfCancelingDeltaPredicate();

  // Utility methods.
  ItemExpr *buildDepColExpr(const CorrName& corrName, Int32 depIndex) const;
  void addStarToSelectList(ItemExprList& selectList);
  void addColsToSelectList(LIST(MVColumnInfo *) listOfCols, 
			   ItemExprList&        selectList,
			    const CorrName&     corrName,
			    const CorrName     *renameTo = NULL);

  // create a name for the new aggregate column - this can be either MIN or MAX
  ColRefName *createExtraAggName(const NAString& mavColName, 
		                 extraColumnType columnType) const;

  ItemExpr *buildExtraMinMaxExpr(const ItemExpr  *pMinMaxExpr, 
				 const NAString&  colName,
				 extraColumnType  columnType) const;

  ItemExpr *buildMinMaxRecomputeOnUpdateCondition(const NAString&  mavColName,
						  ColReference    *deltaInsCol,
						  ColReference    *deltaDelCol,
						  OperatorTypeEnum operatorType,
						  NABoolean        wasFullDE);

  ItemExpr *buildMinMaxRecomputeOnInsertCondition(ColReference *deltaInsCol,
						  ColReference *deltaDelCol);

  void addToMinMaxRecomputeCondition(const NAString&  mavColName,
				     ColReference    *deltaInsCol,
				     ColReference    *deltaDelCol,
				     OperatorTypeEnum operatorType,
				     NABoolean        wasFullDE);

  void handleDeltaMinMaxColumns(ItemExpr        *aggregateExpr,
				const NAString&  colName,
				ItemExprList&    newSelectList,
				NABoolean        wasFullDE);

  void checkForMavWithoutGroupBy(RelExpr *mvSelectTree);
  NABoolean isMavWithoutGroupBy() const { return isMavWithoutGroupBy_; }

private:
  // These are initialized by the Ctor.
  CollHeap                   *heap_;
  Lng32                        posOfCountStar_;
  const MVColumns&	      mavCols_;
  const CorrName              deltaCorrName_;
  const CorrName              mavCorrName_;
  const CorrName              calcCorrName_;

  // These are initialized by init().
  LIST(MVColumnInfo *) groupByCols_;
  LIST(MVColumnInfo *) countCols_;
  LIST(MVColumnInfo *) sumCols_;
  LIST(MVColumnInfo *) otherCols_;

  ItemExprList extraMinMaxColRefs_;

  ItemExpr *minMaxRecomputeOnUpdateCondition_;
  ItemExpr *minMaxRecomputeOnInsertCondition_;

  NABoolean  isMavWithoutGroupBy_;
};

#endif
