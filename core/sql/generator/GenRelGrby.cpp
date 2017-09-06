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
* File:         GenRelGrby.C
* Description:  Aggregate and grouping operators
*               
*               
* Created:      5/17/94
* Language:     C++
*
*
*
*
******************************************************************************
*/
#include "ComOptIncludes.h"
#include "GroupAttr.h"
#include "RelGrby.h"
#include "Generator.h"
#include "GenExpGenerator.h"
//#include "ex_stdh.h"
#include "ExpCriDesc.h"
#include "ComTdb.h"
//#include "ex_tcb.h"
#include "HashRow.h"
#include "hash_table.h" // for HashTableHeader
#include "ComTdbHashGrby.h"
#include "ComTdbSortGrby.h"
#include "DefaultConstants.h"
#include "ItmBitMuxFunction.h"
#include "ComUnits.h"
//#include "ExStats.h"

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   GroupByAgg::genAggrGrbyExpr()
//   HashGroupBy::codeGen()
//   GroupByAgg::codeGen()
//
//////////////////////////////////////////////////////////////////////

// Called by GroupByAgg::codeGen()
short GroupByAgg::genAggrGrbyExpr(Generator * generator,
				  ValueIdSet &aggregateExpr,
				  ValueIdSet &groupExpr,
				  ValueIdList &rollupGroupExprList,
				  ValueIdSet &selectionPred,
				  Int32 workAtp,
				  Int32 workAtpIndex,
				  short returnedAtpIndex, 
				  ex_expr ** aggrExpr,
				  ex_expr ** grbyExpr,
				  ex_expr ** moveExpr,
				  ex_expr ** havingExpr,
				  ComTdb ** childTdb,
				  ExpTupleDesc ** tupleDesc) {
  ExpGenerator * expGen = generator->getExpGenerator();
  Space * space = generator->getSpace();
  
  // remember the end of the current MT
  MapTable * lastMapTable = generator->getLastMapTable();

  // create extra MT to manipultate the childs MT
  MapTable * childMapTable = generator->appendAtEnd();

  // generate code for child tree
  child(0)->codeGen(generator);
  *childTdb = (ComTdb *)(generator->getGenObj());

  // "unlink" the childs MT
  generator->unlinkNext(lastMapTable);

  ////////////////////////////////////////////////////////////
  // Before generating any expression for this node, set the
  // the expression generation flag not to generate float
  // validation PCode. This is to speed up PCode evaluation
  ////////////////////////////////////////////////////////////
  generator->setGenNoFloatValidatePCode(TRUE);

  ULng32 recLen = 0;
  ValueId valId;

  // find the number of aggregate and groupby entries
  ULng32 numAttrs = 0;
  if (NOT aggregateExpr.isEmpty())
    numAttrs += aggregateExpr.entries();
  if (isRollup() && (NOT rollupGroupExprList.isEmpty()))
    numAttrs += rollupGroupExprList.entries();
  else if (NOT groupExpr.isEmpty())
    numAttrs += groupExpr.entries();

  NABoolean isAggrOneRow_ = FALSE;

  // ITM_ONE_ROW can occur in a scalar groupby or in a groupBy with 
  // grouping columns. The second possibility occurs only when the 
  // subquery that contained the ITM_ONE_ROW predicate has been 
  // processed by subquery unnesting.
  if (aggregateExpr.entries() == 1)
  {
    ValueId exprId;
    aggregateExpr.getFirst(exprId);
    
    if ( exprId.getItemExpr()->getOperatorType() == ITM_ONE_ROW )
    {
      isAggrOneRow_ = TRUE;
    }
  }

  // add the elements in the aggr list to the map table
  Int32 i = 0;
  Attributes ** attrs = NULL;

  if ( NOT isAggrOneRow_ )
    {
    attrs = new(generator->wHeap()) Attributes * [numAttrs];
    if (NOT aggregateExpr.isEmpty()) {
      for (valId = aggregateExpr.init();
	   aggregateExpr.next(valId);
	   aggregateExpr.advance(valId), i++) {
        attrs[i] = (generator->addMapInfo(valId, 0))->getAttr();
      }
    }
  }

  // -----------------------------------------------------------------------
  // The values of this moveSet has to be copied from the child's outputs
  // into our buffer at run-time. The moveExpr is going to do this and it
  // will be code-generated from this moveSet later in this method.
  // -----------------------------------------------------------------------
  ValueIdSet moveSet;

  if ( isAggrOneRow_ )
  {
    // ---------------------------------------------------------------------
    // First, collect all the inst-null values under ITM_ONE_ROW. We should
    // have the same number of values as the child's outputs.
    // ---------------------------------------------------------------------
    GenAssert(NOT aggregateExpr.isEmpty(),
              "both groupby and aggregate are empty");
    ValueIdSet inulls;
    for (ValueId vid = aggregateExpr.init();
                       aggregateExpr.next(vid);
                       aggregateExpr.advance(vid))
    {
      GenAssert(vid.getItemExpr()->getOperatorType() == ITM_ONE_ROW,
                "aggregateExpr of one-row aggr must be rooted by ITM_ONE_ROW");

      // The child of ITM_ONE_ROW might be an ITEM_LIST backbone. This code
      // assumes that the LIST backbone is a left-linear tree.
      //
      ValueId listBackboneVid = vid.getItemExpr()->child(0)->getValueId();
      NABoolean moreToGo = TRUE;
      do {
        ValueId listItemVid;
        if (listBackboneVid.getItemExpr()->getOperatorType() == ITM_ITEM_LIST)
          listItemVid = listBackboneVid.getItemExpr()->child(1)->getValueId();
        else
        {
          // reach the end of the backbone. handle the remaining one item and
          // exit.
          listItemVid = listBackboneVid;
          moreToGo = FALSE;
        }
 
        if (listItemVid.getItemExpr()->getOperatorType() == ITM_CAST)
        {
          // The code above might have inserted a ITM_CAST on top of INST_NULL.
          listItemVid = listItemVid.getItemExpr()->child(0)->getValueId();
        }
 
        GenAssert(
          listItemVid.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL,
          "ITM_ONE_ROW's logical leaf nodes can only be inst-null values");
 
        inulls.insert(listItemVid); // collect the value id.
 
        // Go to the next item. Here is where we assume the tree is left linear.
        if (moreToGo)
        listBackboneVid = listBackboneVid.getItemExpr()->child(0)->getValueId();
      } while (moreToGo);
    }

    /* this might not hold due to veg rewrite. for example inst-null(1) can
       be a result of rewriting inst-null(veg{x.a,1}). child will no longer
       produce x.a if x.a = 1 has been evaluated at it. In that case, child's
       output set can be empty while we still find inull(1) under the aggr.

    GenAssert(inulls.entries() ==
              child(0)->getGroupAttr()->getCharacteristicOutputs().entries(),
             "inst-null values under ITM_ONE_ROW not same as child's outputs");
    */

    moveSet += inulls;

    // ---------------------------------------------------------------------
    // Also add my own outputs.
    // ---------------------------------------------------------------------
    //
    // sol 10-070112-1749 (wangth)
    // since isAggrOneRow_ is true we know the child node of sort_grby will
    // either return only one row or no row at all. at runtime the executor
    // does following (see ex_sort_grby_tcb::work in ex_sort_grby.cpp):
    //
    //   - if one row is returned from child node, evaluate it with
    //     aggrExpr.moveExpr_ and return the results upstream.
    //   - if no row is returned from child node, don't do any evaluation
    //     but simply return NULL upstream. this can cause problem if
    //     the select object is not of column type. for example, if select
    //     contains expression such as ISNULL(), the evaluation of the ISNULL
    //     expression with no row may result in a not-NULL value.
    //
    // to solve above problem we have two options:
    //
    // 1. generate two moveExpr_ expressions, one used for one row scenario,
    //    and the other used for no row scenario.
    // 2. if select contains expressions other than column, do not include
    //    those expressions in moveExpr_. instead, let the parent node of
    //    sort_grby do code generation for those expressions. this won't
    //    cause any performance setback because the parent node will evaluate
    //    the expressions (such as ISNULL) only once.
    //
    // we choose the second option. the first option is a lot more complex
    // and risky.
    //
    const ValueIdSet & charOutputs = getGroupAttr()->getCharacteristicOutputs();
    for (valId = charOutputs.init();
         charOutputs.next(valId);
         charOutputs.advance(valId))
      {
        OperatorTypeEnum opType = valId.getValueDesc()->getItemExpr()->getOperatorType();
        if (opType == ITM_BASECOLUMN || opType == ITM_INDEXCOLUMN ||
            opType == ITM_REFERENCE || opType == ITM_BASECOL)
          // only do code gen for columns (but not other expressions)

	  // add only those value ids that are not already in the groupExpr
	  if (NOT groupExpr.contains(valId))
	      moveSet.insert(valId);
      }

    // ----------------------------------------------------------------------
    // Set up the destinations of this moveSet (if any). It's going to reside
    // in the workAtp together with the aggregates.
    // ----------------------------------------------------------------------
    attrs = new(generator->wHeap()) Attributes * [numAttrs+moveSet.entries()];

    for (valId = aggregateExpr.init();
                 aggregateExpr.next(valId);
                 aggregateExpr.advance(valId), i++)
    {
      attrs[i] = (generator->addMapInfo(valId, 0))->getAttr();
    }

    for (valId = moveSet.init();
                 moveSet.next(valId);
                 moveSet.advance(valId), i++)
    {
      attrs[i] = (generator->addMapInfo(valId, 0))->getAttr();
    }
  } // aggregate is ITM_ONE_ROW

  // create a copy of the group by set. This set is used
  // to move the incoming values to current group buffer.
  ValueIdList moveValIdList;
  ValueIdList gbyValIdList;
  ValueIdSet searchValIdSet;
  
  if (isRollup() && (NOT rollupGroupExprList.isEmpty())) {
    for (CollIndex j = 0; j < rollupGroupExprList.entries(); j++) {
      valId = rollupGroupExprList[j];

      ItemExpr * itemExpr = valId.getItemExpr();
      
      // add this converted value to the map table.
      ItemExpr * convNode = NULL;
      convNode = new(generator->wHeap()) Convert (itemExpr);

      // bind/type propagate the new node
      convNode->bindNode(generator->getBindWA());    
      
      attrs[i++] = 
        (generator->addMapInfo(convNode->getValueId(), 0))->getAttr();
      moveValIdList.insert(convNode->getValueId());
      gbyValIdList.insert(valId);
      
      // add the search condition
      BiRelat * biRelat = new(generator->wHeap())
        BiRelat(ITM_EQUAL, itemExpr, convNode);
      biRelat->setSpecialNulls(-1);
      biRelat->bindNode(generator->getBindWA());
      
      biRelat->rollupColumnNum() = j+1;
      
      searchValIdSet.insert(biRelat->getValueId());
    }
  }
  else if (NOT groupExpr.isEmpty()) {
    for (valId = groupExpr.init();
         groupExpr.next(valId);
         groupExpr.advance(valId), i++) {
      
      ItemExpr * itemExpr = valId.getItemExpr();
      
      // add this converted value to the map table.
      Convert * convNode = new(generator->wHeap()) Convert (itemExpr);
      
      // bind/type propagate the new node
      convNode->bindNode(generator->getBindWA());    
      
      attrs[i] = 
	(generator->addMapInfo(convNode->getValueId(), 0))->getAttr();
      moveValIdList.insert(convNode->getValueId());
      gbyValIdList.insert(valId);
      
      // add the search condition
      BiRelat * biRelat = new(generator->wHeap())
	BiRelat(ITM_EQUAL, itemExpr, convNode);
      biRelat->setSpecialNulls(-1);
      biRelat->bindNode(generator->getBindWA());
      
      searchValIdSet.insert(biRelat->getValueId());
    }
  }
  
  numAttrs = ( isAggrOneRow_ ?  numAttrs + moveSet.entries() : numAttrs );

  // Create the descriptor describing the aggr row and assign offset to attrs. 
  expGen->processAttributes(numAttrs,
			    attrs,
			    ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			    recLen,
			    workAtp,
			    workAtpIndex,
			    tupleDesc,
			    (isRollup() ? ExpTupleDesc::LONG_FORMAT : ExpTupleDesc::SHORT_FORMAT));
 
  NADELETEBASIC(attrs, generator->wHeap());

  // add the childs MT again, we need it to generate the following expressions
  generator->appendAtEnd(childMapTable);

  if (NOT aggregateExpr.isEmpty())
  {
    if (isAggrOneRow_)
    {
      // --------------------------------------------------------------------
      // generate aggregate expression now. The function will also generate
      // an initializeExpr to initialize all the values in moveSet to null.
      // --------------------------------------------------------------------
      expGen->generateAggrExpr(aggregateExpr, ex_expr::exp_AGGR,
			       aggrExpr, 0, 
                               (NOT groupExpr.isEmpty()), &moveSet);

      ((AggrExpr *)*(aggrExpr))->setOneRowAggr();
    }
    else
    {
      // generate aggregate expression 
      expGen->generateAggrExpr(aggregateExpr, ex_expr::exp_AGGR, aggrExpr, 
                               0, (NOT groupExpr.isEmpty()));
    }
  }

  MapTable *returnedMapTable_ = NULL;

  if (NOT groupExpr.isEmpty()) {
    // generate the move expression. This is used to move the incoming
    // grouping values to the aggr buffer.
    expGen->generateListExpr(moveValIdList,
			     ex_expr::exp_ARITH_EXPR,
			     moveExpr);

    // generate the search expression. This expression is used
    // to look for a matching value in the hash table.
    ItemExpr * newPredTree =
      searchValIdSet.rebuildExprTree(ITM_AND, TRUE, TRUE);
      
    expGen->generateExpr(newPredTree->getValueId(),
			 ex_expr::exp_SCAN_PRED,
			 grbyExpr);
  }
  
   if ( isAggrOneRow_ && (NOT moveSet.isEmpty()) )
   {
     // --------------------------------------------------------------------
     // Generate the moveExpr which moves values from the child's outputs
     // (atp0) to the workAtp (atp1).
     // --------------------------------------------------------------------
     expGen->generateSetExpr(moveSet,
                             ex_expr::exp_ARITH_EXPR,
                             moveExpr);
   }

  // Change the atp of aggregate values to 0.
  // All references to these values from this point on
  // will be at atp = 0, atp_index = last entry in returned desc.
  // Offset will be the same as in the workAtp.
  if (NOT aggregateExpr.isEmpty()) {
    for (valId = aggregateExpr.init();
	 aggregateExpr.next(valId);
	 aggregateExpr.advance(valId)) {
      Attributes * attr = generator->getMapInfo(valId)->getAttr();
      attr->setAtp(0);
      attr->setAtpIndex(returnedAtpIndex);
    }
  }
  
  // remove the child's map table. Nothing from child's context
  // should be visible from here on upwards.
  generator->removeAll(lastMapTable);

  if ( returnedMapTable_ )
   generator->appendAtEnd( returnedMapTable_ );

  if ( isAggrOneRow_ )
  {
    // -------------------------------------------------------------------
    // Both the selection predicates and the parent expects outputs in
    // atp0. Set the values in moveSet to reference atp0, so that the
    // selection predicates and parent could be generated properly.
    // -------------------------------------------------------------------
    for (valId = moveSet.init(); 
	         moveSet.next(valId); 
	         moveSet.advance(valId)) 
    {
 
      MapInfo    * mapInfo = generator->getMapInfo(valId);
      Attributes * colAttr = mapInfo->getAttr();
      colAttr->setAtp(0);

      // This code assumes that returnedAtpIndex == workAtpIndex.
      colAttr->setAtpIndex(returnedAtpIndex);

      // code has been generated for valId and a value is available.
      // Mark it so. The valId should have already been marked "generated"
      // while we were generating the moveExpr.
      //
      mapInfo->codeGenerated();
    }
  } // if ITM_ONE_ROW and outputs exist

  // the moveValIdList (set of convert nodes) was used to move the incoming
  // group by values to the aggregate buffer. The child of the convert node
  // is the original grouping column. From this point on, the grouping value
  // is available in the aggregate buffer. Change the buffer attributes of
  // the grouping values in the map table to point to this new location.
  // Also, the atp value of the grouping columns is now 0.
  // This change of attributes is not done if the grouping column
  // is a 'constant'.
  if (NOT groupExpr.isEmpty()) {
    ValueId convValId;

    for (CollIndex i = 0; i < moveValIdList.entries(); i++) {
      convValId = moveValIdList[i];
      Attributes * newGroupColAttr =
	(generator->getMapInfo(convValId))->getAttr();
	
      ValueId valId = gbyValIdList[i];

      // change the location of the grouping columns, unless they
      // are input to this node. Input values already have location
      // assigned to them.
      if ((NOT getGroupAttr()->getCharacteristicInputs().contains(valId)) &&
           (valId.getItemExpr()->getOperatorType() != ITM_CONSTANT)) 
        {
          MapInfo * mapInfo = generator->addMapInfo(valId, 0);
          Attributes * oldGroupColAttr = mapInfo->getAttr();
	  
          oldGroupColAttr->copyLocationAttrs(newGroupColAttr);
          oldGroupColAttr->setAtp(0);
          oldGroupColAttr->setAtpIndex(returnedAtpIndex);
	  
          // code has been generated for valId and a value is available.
          // Mark it so.
          mapInfo->codeGenerated();
        }
    }
  }
  
  // generate having expression, if present
  if (NOT selectionPred.isEmpty()) {
    ItemExpr * newPredTree =
      selectionPred.rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(),
			    ex_expr::exp_SCAN_PRED,
			    havingExpr);
  }
  
  // reset the expression generation flag to generate float validation pcode
  generator->setGenNoFloatValidatePCode(FALSE);

  return 0;
} // GroupByAgg::genAggrGrbyExpr()

/////////////////////////////////////////////////////////
//
// HashGroupBy::codeGen()
//
/////////////////////////////////////////////////////////
short HashGroupBy::codeGen(Generator * generator) {
  Space * space = generator->getSpace();

  // create a map tables for returned group by and aggregate values
  MapTable * myMapTable1 = generator->appendAtEnd();
  MapTable * myMapTable2 = generator->appendAtEnd();

  ////////////////////////////////////////////////////////////////////////////
  //
  // Layout at this node:
  //
  // |-------------------------------------------------|
  // | input data  |  Grouped data | child's data      |
  // | ( I tupps ) |  ( 1 tupp )   | ( C tupps )       |
  // |-------------------------------------------------|
  // <-- returned row to parent --->
  // <------------ returned row from child ------------>
  //
  // input data:        the atp input to this node by its parent. 
  // grouped data:      tupp where the aggr/grouped result is moved
  // child data:        tupps appended by the left child
  //
  // Input to child:    I + 1 tupps
  //
  // Tupps returned from child are only used to create the
  // grouped data. They are not returned to parent.
  //
  ////////////////////////////////////////////////////////////////////////////

  ex_cri_desc * givenDesc
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc 
#pragma nowarn(1506)   // warning elimination 
    = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space); 
#pragma warn(1506)  // warning elimination 
  
  generator->setCriDesc(returnedDesc, Generator::DOWN);

  short returnedAtpIndex = (short) (returnedDesc->noTuples() - 1);

  ///////////////////////////////////////////////////////////////////////////
  // in all the computation below, the grouped/aggregated rows is available
  // in a temporary work atp. They are used from there to aggregate / hash /
  // group values.
  // At runtime, the work atp is passed as the second atp to the
  // expression evaluation procedures. So, the atp value for all group /
  // aggregate values ids  are set to 1 (which corresponds to the second
  // atp -- it is zero based).
  // Before returning from this proc, these value ids are moved to the
  // map table that is being returned.
  // The work atp contains 6 entries:
  // index    what
  // -------------------------------------------------
  //   0      constants
  //   1      temps
  //   2      grouped row in the hash buffer
  //   3      another grouped row in the hash buffer, this time
  //          interpreted as an overflow buffer
  //   4      the calculated hash value
  //   5      grouped row in the result buffer
  //////////////////////////////////////////////////////////////////////////
  short workAtpPos = 1; // second atp 
  short hbRowAtpIndex = 2;
  short ofRowAtpIndex = 3;
  short hashValueAtpIndex = 4;
  short resultRowAtpIndex = 5;
  short bitMuxAtpIndex = 6;

  ex_cri_desc * workCriDesc = new(space) ex_cri_desc(7, space);

  ComTdb * childTdb = 0;

  // The hashExpr takes a child row and calculates the hash value. The hash
  // value is stored in the hashValueTupp_ of the hash_grby_tcb.
  ex_expr * hashExpr = 0;

  // The bitMuxExpr takes a child row and computes the entry in the 
  // bitMuxTable of the row's group. The entry is stored in the 
  // hashValueTupp_ of the hash_grby_tcb (like hashExpr above).
  //
  ex_expr * bitMuxExpr = 0;

  // The bitMuxAggrExpr is equivalent to hbAggrExpr except that since
  // the BitMux table automatically computes some aggregates (i.e. count),
  // the bitMuxAggrExpr does not recompute these aggregates.
  //
  ex_expr * bitMuxAggrExpr = 0;

  // The hbMoveInExpr is used to move the grouping columns from the child
  // row into the hash buffer. This is done if a new group is started.
  ex_expr * hbMoveInExpr = 0;

  // The ofMoveInExpr is used to move the grouping columns from the overflow
  // buffer into the hash buffer. This is done if a new group is started.
  // In case of a partial hash group this expression is not generated.
  // Note that the overflow buffer and the hash buffer contain rows with
  // identical format.
  ex_expr * ofMoveInExpr = 0;

  // The resMoveInExpr is used to move the grouping columns from the child
  // row into the result buffer. This is done in case of partial hash groups
  // if the current row doesn't belong to any group in the hash buffer. In
  // case the row estabilshes a new (partial) group in the result buffer. The
  // parent takes care of merging partial groups. Partial groups are used to
  // avoid overflow handling (overflow handling can't be used in case the
  // hash grouping is executed in DP2) or in case of parallel execution.
  // This expression is also generated in the case of no aggregates, where 
  // the root node (non partial) also needs to copy rows from child input
  // to the result buffer (that row/group is returned up immediately.)
  ex_expr * resMoveInExpr = 0;

  // The hbAggrExpr is used to aggregate a child row into an existing group
  // in the hash buffer.
  ex_expr * hbAggrExpr = 0;

  // The ofAggrExpr is used to aggregate a row from the overflow buffer into
  // an existing group in the hash buffer.
  // In case of a partial hash group this expression is not generated.
  ex_expr * ofAggrExpr = 0;

  // The resAggrExpr is used to aggregate a child row into a partial group
  // in the result buffer.
  // This expression is only generated in case of a partial hash group.
  ex_expr * resAggrExpr = 0;

  // The havingExpr is used to decide whether a a group in the hash buffer
  // qualifies and is therefore moved into the result buffer.
  ex_expr * havingExpr = 0;

  // The moveOutExpr is used to move a qualifiying group from the hash
  // buffer to the result buffer
  ex_expr * moveOutExpr = 0;

  // The hbSearchExpr is used to serach the hash table for the group to which
  // the child row belongs.
  ex_expr * hbSearchExpr = 0;

  // The ofSearchExpr is used to search the hash table for the group to which
  // a row in a overflow buffer belongs
  ex_expr * ofSearchExpr = 0;
 
  ExpTupleDesc * tupleDesc = 0;

  ExpGenerator * expGen = generator->getExpGenerator();

  // if the hash grouping is executed in DP2, we don't do overflow
  // handling. This also means, that it is a partial group by
  // Do not do overflow handling for any partial groupby.
  //
  NABoolean isPartialGroupBy = (isAPartialGroupByNonLeaf() ||
                                isAPartialGroupByLeaf());

  // The old way, only groupbys in DP2 are considered partial
  //
  if (CmpCommon::getDefault(COMP_BOOL_152) == DF_ON) {
    isPartialGroupBy = executeInDP2();
  }

  // set flag to enable pcode for indirect varchar
  NABoolean vcflag = expGen->handleIndirectVC();
  if (CmpCommon::getDefault(VARCHAR_PCODE) == DF_ON) {
    expGen->setHandleIndirectVC( TRUE );
  }

  // If executing in DP2, then it should be a partial
  //
  GenAssert(!executeInDP2() || isPartialGroupBy, "Invalid Groupby in DP2");

  // Get the value from the defaults table that specifies
  // whether to use BitMux or not.
  NABoolean useBitMux = (CmpCommon::getDefault(HGB_BITMUX) == DF_ON);

  useBitMux = FALSE;

  // split the aggregate into a lower and an upper aggregate function.
  // Impotant assumption for this code: The lower and the upper aggregate
  // have identical format. 
  ValueId valId;
  ValueIdList originalAggrValIds(aggregateExpr());

  GenAssert(aggregateEvaluationCanBeStaged(),
	    "illegal aggregate function for hash grouping");

  ValueIdList lowerAggrValIdList;
  ValueIdList upperAggrValIdList;
  for (valId = aggregateExpr().init();
       aggregateExpr().next(valId);
       aggregateExpr().advance(valId)) {
    Aggregate *a = (Aggregate *) valId.getItemExpr();
    ItemExpr * newResult =
      a->rewriteForStagedEvaluation(lowerAggrValIdList,
				    upperAggrValIdList, TRUE);
  };

  ValueIdSet lowerAggrValIds(lowerAggrValIdList);
  ValueIdSet upperAggrValIds(upperAggrValIdList);

  // the value Ids of the grouping expression. 
  ValueIdSet &groupValIds = groupExpr();

  // the size of the grouped row in the hash buffer (not including the 
  // HashRow header).
  ULng32 groupedRowLength = 0;

  // find the number of aggregate and groupby entries
  ULng32 numAttrs
    = ((NOT lowerAggrValIds.isEmpty() ) ? lowerAggrValIds.entries() : 0)
    + ((NOT groupValIds.isEmpty() ) ? groupValIds.entries() : 0);
  
  Attributes ** attrs = new(generator->wHeap()) Attributes * [numAttrs];
					  
  // add the elements in the aggr list to the map table
  Int32 i = 0;
  if (NOT lowerAggrValIds.isEmpty()) {
    for (valId = lowerAggrValIds.init();
	 lowerAggrValIds.next(valId);
	 lowerAggrValIds.advance(valId), i++) {
      attrs[i] = (generator->addMapInfo(valId, 0))->getAttr();

      // For any varchar aggregates to be treated as fixed values in
      // Aligned format (CIF)
      attrs[i]->setForceFixed();
    }
  };

  // create a copy of the group by set. This set is used
  // to move the incoming values to the hash buffer. It is also
  // used to generate the search expression to decide if an incoming
  // row belongs to an existing group
  ValueIdList hbMoveValIds;
  ValueIdList gbyValIdList;
  ValueIdSet hbSearchValIds;

  // remember index of first group attr
  Int32 attrIdx = i;
  
  if (NOT groupValIds.isEmpty()) {
    for (valId = groupValIds.init();
	 groupValIds.next(valId);
	 groupValIds.advance(valId), i++) {
      
      ItemExpr * itemExpr = valId.getItemExpr();
	  
      // add this converted value to the map table.
      Convert * convNode = new(generator->wHeap()) Convert (itemExpr);
	  
      // bind/type propagate the new node
      convNode->bindNode(generator->getBindWA());    
	  
      attrs[i] = 
	(generator->addMapInfo(convNode->getValueId(), 0))->getAttr();
      hbMoveValIds.insert(convNode->getValueId());
      gbyValIdList.insert(valId);

      // add the search condition
      BiRelat * biRelat = new(generator->wHeap())
	BiRelat(ITM_EQUAL, itemExpr, convNode);
      biRelat->setSpecialNulls(-1);
      biRelat->bindNode(generator->getBindWA());
      if (generator->getBindWA()->errStatus())
        GenExit();
      hbSearchValIds.insert(biRelat->getValueId());
    }
  };

  // Incoming records will be divided equally (sans data skew) among the ESPs
  // so each HGB instance will handle only its share (i.e. divide by #esps)
  Lng32 saveNumEsps = generator->getNumESPs();

  // generate code for child tree
  child(0)->codeGen(generator);
  childTdb = (ComTdb *)(generator->getGenObj());
  
  // This value was originally set inside generator by my parent exchange node
  // as a global variable. Now need to reset the saveNumEsps value back into
  // generator since codegen of my children exchange nodes may have changed it.
  generator->setNumESPs(saveNumEsps);

  ////////////////////////////////////////////////////////////
  // Before generating any expression for this node, set the
  // the expression generation flag not to generate float
  // validation PCode. This is to speed up PCode evaluation
  ////////////////////////////////////////////////////////////
  generator->setGenNoFloatValidatePCode(TRUE);

  ExpTupleDesc::TupleDataFormat tupleFormat = generator->getInternalFormat();

  //determine the tuple format and whether we want to resize rows or not
  NABoolean bmo_affinity = (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO_AFFINITY) == DF_ON);
  NABoolean resizeCifRecord = FALSE;
  NABoolean considerBufferDefrag = FALSE;

  if (! bmo_affinity &&
      getCachedTupleFormat() != ExpTupleDesc::UNINITIALIZED_FORMAT &&
      CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_SYSTEM &&
      CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO) == DF_SYSTEM)
  {
    resizeCifRecord = getCachedResizeCIFRecord();
    tupleFormat = getCachedTupleFormat();
    considerBufferDefrag = getCachedDefrag() && resizeCifRecord;
  }
  else
  {
     tupleFormat = determineInternalFormat(hbMoveValIds,
                                          this,
                                          resizeCifRecord,
                                          generator,
                                          bmo_affinity,
                                          considerBufferDefrag);
     considerBufferDefrag = considerBufferDefrag && resizeCifRecord;
  }

  // If generating a contiguous move expression where the target tuple data
  // format is a disk format - Packed or Aligned - then header information
  // must be gathered during processing the attributes. This allows a new 
  // header clause to be generated during endExprGen()
  ExpHdrInfo *hdrInfo = NULL;
  if ( ExpTupleDesc::isDiskFormat(tupleFormat) )
    hdrInfo = new( generator->wHeap() )ExpHdrInfo();

  // generate resMoveInExpr and resAggrExpr required in case of
  // partial group by
  if ( isPartialGroupBy || upperAggrValIds.isEmpty() ) {
    // first add a maptable, so that we can get rid of all the
    // stuff added to the map table by the following statements
    MapTable * tempMapTable  = generator->appendAtEnd();
    MapTable * tempMapTable1 = generator->appendAtEnd();
    expGen->processAttributes(numAttrs,
			      attrs,
			      tupleFormat,
			      groupedRowLength,
			      workAtpPos,
			      resultRowAtpIndex,
			      0,
			      ExpTupleDesc::SHORT_FORMAT,
                              0,
                              hdrInfo);

    // generate resAggrExpr 
    if (NOT lowerAggrValIds.isEmpty()) {
      expGen->generateAggrExpr(lowerAggrValIds, ex_expr::exp_AGGR, 
               &resAggrExpr, 0, (NOT groupValIds.isEmpty()));
    }

    // create a copy of the group by set. This set is used
    // to move the incoming values to the result buffer.
    ValueIdSet resMoveValIds;

    if (NOT groupValIds.isEmpty()) {
      for (valId = groupValIds.init();
	   groupValIds.next(valId);
	   groupValIds.advance(valId), attrIdx++) {
      
	ItemExpr * itemExpr = valId.getItemExpr();
	  
	// add this converted value to the map table.
	Convert * convNode = new(generator->wHeap()) Convert (itemExpr);
	  
	// bind/type propagate the new node
	convNode->bindNode(generator->getBindWA());    
	  
	generator->addMapInfo(convNode->getValueId(), attrs[attrIdx]);
	resMoveValIds.insert(convNode->getValueId());

      }

      // generate the move expression. This is used to move the incoming
      // grouping values to the result buffer.
      expGen->generateSetExpr(resMoveValIds,
			      ex_expr::exp_ARITH_EXPR,
			      &resMoveInExpr,
                              -1,
                              hdrInfo);
    };

    // the extra extressions are generated. Lets get rid of the additional
    // map table entries
    generator->removeAll(tempMapTable);
  };

  // change attributes and offsets to represent the rows in the
  // hash buffer
  // Offsets are based on the row starting after the HashRow structure.
  expGen->processAttributes(numAttrs,
			    attrs,
			    tupleFormat,
			    groupedRowLength,
			    workAtpPos,
			    hbRowAtpIndex,
			    0,
			    ExpTupleDesc::SHORT_FORMAT,
                            0,
                            hdrInfo);

  // the size of the grouped row in the hash buffer
  ULng32 extGroupRowLength = groupedRowLength + sizeof(HashRow);

  NADELETEBASIC(attrs, generator->wHeap());

  // generate the hash computation function
  ItemExpr * leftExpr = 0;
  for (valId = groupValIds.init();
       groupValIds.next(valId);     
       groupValIds.advance(valId)) {

    ItemExpr * itemExpr = valId.getItemExpr();

    BuiltinFunction * hashFunction = new(generator->wHeap()) Hash(itemExpr);

    if (leftExpr)
      leftExpr = new(generator->wHeap()) HashComb(leftExpr, hashFunction);
    else
      leftExpr = hashFunction;
  }

  leftExpr = new (generator->wHeap()) Cast(leftExpr, new (generator->wHeap())
					   SQLInt(generator->wHeap(), FALSE, FALSE));
  leftExpr->setConstFoldingDisabled(TRUE);      

  // bind/type propagate the hash evaluation tree
  leftExpr->bindNode(generator->getBindWA());
      
  // add the root value id to the map table. This is the hash value.
  Attributes * mapAttr 
    = (generator->addMapInfo(leftExpr->getValueId(), 0))->getAttr();
  mapAttr->setAtp(workAtpPos);
  mapAttr->setAtpIndex(hashValueAtpIndex);
  ULng32 len;
  ExpTupleDesc::computeOffsets(mapAttr, 
                               ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
                               len);

  // generate code to evaluate the hash expression
  expGen->generateArithExpr(leftExpr->getValueId(),
                            ex_expr::exp_ARITH_EXPR,
                            &hashExpr);

  // Construct the BitMux expression. The BitMux expression computes the 
  // binary encoded key for the grouping operation. The key is stored in 
  // the bitMuxTupp_ of the workAtp.
  //
  // First, construct a list of the item expressions representing the
  // grouping attributes.
  //
  LIST(ItemExpr*) bitMuxAttrList(generator->wHeap());
  for(valId = groupValIds.init();
      groupValIds.next(valId);
      groupValIds.advance(valId)) {

    ItemExpr * ie = valId.getItemExpr();
    const NAType &valType = valId.getType();
    
    if(valType.getTypeQualifier() == NA_CHARACTER_TYPE) {
      
      const CharType &chType = (CharType&)valType;
      
      if ((chType.isCaseinsensitive()) &&
	  (NOT chType.isUpshifted())) {
	ie = new (generator->wHeap()) Upper(ie);
	ie = ie->bindNode(generator->getBindWA());
      }
      // At least for now, don't use BitMux if CZECH collation is involved.
      if (chType.getCollation() == CharInfo::CZECH_COLLATION )
         useBitMux = FALSE;
    }
    
    bitMuxAttrList.insert(ie);
  }

  // Allocate and bind the bit muxing item expression
  //
  ItemExpr *bitMuxItemExpr = new(generator->wHeap())
    ItmBitMuxFunction(bitMuxAttrList);
  bitMuxItemExpr->setConstFoldingDisabled(TRUE);      

  bitMuxItemExpr->bindNode(generator->getBindWA());
  
  // Alter the map table so that the result of the bitMux expression
  // side effects the bitMuxAtpIndex of the workAtp.
  //
  ULng32 keyLength;
  mapAttr = generator->addMapInfo(bitMuxItemExpr->getValueId(), 0)->getAttr();
  mapAttr->setAtp(workAtpPos);
  mapAttr->setAtpIndex(bitMuxAtpIndex);
  ExpTupleDesc::computeOffsets(mapAttr,
			       ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			       keyLength);

  // a simple heuristic for now until BitMux limitations are better understood
  if (keyLength > 12 ||

      // BitMux becomes a burden once its table gets full, as it adds unneeded
      // checks for every newly incoming group. Don't use BitMux when the
      // groups held in memory are expected to exceed the bitmux table size!

      1000 * 1024 <   // bitmux table size (taken from ex_hash_grby.cpp)
      (Cardinality) getEstRowsUsed().getValue()  *   // expected #groups
      // Approximate memory used per each group in the BitMux table
      // (Detail in ExBitMapTable.cpp; "+3+7" is a cheap ROUND4/8 substitute)
      ( extGroupRowLength + keyLength + 2*sizeof(char *) + 3 + 7 )

      )
    useBitMux = FALSE;

  // There is an arbitrary limit in the standard expression evaluation
  // scheme of MAX_OPERANDS arguments (including the result) for any
  // operation. Thus, the number of items in the BitMux attribute list
  // must be less than MAX_OPERANDS.
  //
  if(bitMuxAttrList.entries() < ex_clause::MAX_OPERANDS && useBitMux) {
    // Finally, generate the clauses for the bit muxing expression.
    //
    expGen->generateArithExpr(bitMuxItemExpr->getValueId(),
			      ex_expr::exp_ARITH_EXPR,
			      &bitMuxExpr);
  }

  // If the BitMux expression is successfully generated.
  //
  // countOffset is the offset within the BitMux buffer to store the count
  // for the row. By default, this is at offset zero which is where the
  // hash value is normally stored for hash buffers. If there is a count(*)
  // aggregate is the lower aggregates, then the countOffset will point
  // to that location instead.
  //
  short bitMuxCountOffset = 0;

  if (NOT lowerAggrValIds.isEmpty()) {
    // generate the hash buffer aggregate expression 
    expGen->generateAggrExpr(lowerAggrValIds, ex_expr::exp_AGGR,
			     &hbAggrExpr, 0, 
                             (NOT groupValIds.isEmpty()));

    if (bitMuxExpr) {
      // Generate the BitMux table aggregate expression. For now the only
      // difference is that the BitMux table already has the count available
      // so the BitMux aggregate expression does not need to recompute the
      // count.
      //
      // First, duplicate the aggregate value id set except for any
      // count(*) aggregates.
      //
      ValueIdSet bitMuxAggrValIds;
      for (valId = lowerAggrValIds.init();
	   lowerAggrValIds.next(valId);
	   lowerAggrValIds.advance(valId), i++) {
	Aggregate *aggr = (Aggregate*)valId.getItemExpr();

	if(aggr->getOperatorType() == ITM_COUNT) {
	  Attributes * attr = generator->getMapInfo(valId)->getAttr();
	  if(bitMuxCountOffset == 0) {
	    bitMuxCountOffset = (short)attr->getOffset();
	  } else {
	    attr->setOffset(bitMuxCountOffset);
	  }
	  continue;
	}

	bitMuxAggrValIds += valId;
      }

      // If there are still aggregates, generate the BitMux aggregate 
      // expression.
      //
      if(NOT bitMuxAggrValIds.isEmpty())
	expGen->generateAggrExpr(bitMuxAggrValIds, ex_expr::exp_AGGR,
				 &bitMuxAggrExpr, 0, 
				 (NOT groupValIds.isEmpty()));
    }
  }

  
  if (NOT groupValIds.isEmpty()) {

    // generate the move expression. This is used to move the incoming
    // grouping values to the hash buffer.
    expGen->generateListExpr(hbMoveValIds,
			     ex_expr::exp_ARITH_EXPR,
			     &hbMoveInExpr,
                             -1, -1,
                             hdrInfo
                             );
    // generate the search expression. This expression is used
    // to look for a matching value in the hash table.
    ItemExpr * newPredTree =
      hbSearchValIds.rebuildExprTree(ITM_AND, TRUE, TRUE);
      
    expGen->generateExpr(newPredTree->getValueId(),
			 ex_expr::exp_SCAN_PRED,
			 &hbSearchExpr);

  };

  // remove all the map tables generated by the child. This leaves us
  // with the map table describing the grouped row in the hash buffer
  generator->removeAll(myMapTable2);

  // generate the expression which moves a result row to the result buffer
  ValueIdList resultValIds;
  ULng32 resultRowLength;
  MapTable * resultValMapTable = NULL;
  ValueIdList moveOutValIdList;
  ValueIdList gendAggrValIdList;

  // add the valIds of the aggregate expressions
  if (NOT lowerAggrValIds.isEmpty()) {

    for (CollIndex i = 0; i < lowerAggrValIdList.entries(); i++)
      {
	valId = lowerAggrValIdList[i];
	ValueId origValId = originalAggrValIds[i];

	// convert lowerAggrValIds to original aggr type before
	// adding to the result row, if the two types are not the same.
	ValueId moveOutValId;
	if (! (valId.getType() == origValId.getType()))
	  {
	    ItemExpr * item_expr = new(generator->wHeap()) Cast(valId.getItemExpr(),
						     &(origValId.getType()));
	    item_expr->bindNode(generator->getBindWA());
	    moveOutValId = item_expr->getValueId();
	  }
	else
	  moveOutValId = valId;

	resultValIds.insert(valId);
	moveOutValIdList.insert(moveOutValId);
	gendAggrValIdList.insert(moveOutValId);
      }
  };
  
  // add the valIds for grouping
  if (NOT hbMoveValIds.isEmpty()) {
    for (CollIndex i = 0; i < hbMoveValIds.entries(); i++)
      {
	valId = hbMoveValIds[i];
	moveOutValIdList.insert(valId);
	resultValIds.insert(valId);
      }
  };
    
  expGen->generateContiguousMoveExpr(moveOutValIdList, //resultValIds,
				     -1, // add convert nodes
				     workAtpPos,
				     resultRowAtpIndex,
				     tupleFormat,
				     resultRowLength,
				     &moveOutExpr, 
				     &tupleDesc,
				     ExpTupleDesc::SHORT_FORMAT,
				     &resultValMapTable);

  // if we do overflow handling, we have to generate ofMoveInExpr,
  // ofSearchExpr, and ofAggrExpr
  if (!isPartialGroupBy) {
    // the map table describes a row in the hash buffer. This is the
    // row we read in from the temporary file. Thus, we change the
    // atp index accordingly. This guarantees, that the row in the
    // hash buffer always has atpindex hbRowAtpIndex.
    // The row from the overflow buffer is always the first parameter
    // for expressions. Thus, the atp is 0.
    for (CollIndex ix = 0; ix < resultValIds.entries(); ix++) {
      valId = resultValIds[ix];
      Attributes * attr = generator->getMapInfo(valId)->getAttr();
      attr->setAtpIndex(ofRowAtpIndex);
      attr->setAtp(0);
    };

    // add the elements of the upper aggregate to the map table
    attrs = new(generator->wHeap()) Attributes * [numAttrs];
    Int32 i = 0;
    if (NOT upperAggrValIds.isEmpty()) {
      for (valId = upperAggrValIds.init();
	   upperAggrValIds.next(valId);
	   upperAggrValIds.advance(valId), i++) {
	attrs[i] = (generator->addMapInfo(valId, 0))->getAttr();

        // For any varchar aggregates to be treated as fixed values in
        // Aligned format (CIF)
        attrs[i]->setForceFixed();
      }
    };

    // create a copy of the group by set. This set is used
    // to move the incoming values from the overflow buffer to the
    // hash buffer. It is also used to generate the search expression
    // to decide if a row from the overflow buffer belongs to an
    // existing group
    ValueIdSet ofMoveValIds;
    ValueIdSet ofSearchValIds;
  
    if (NOT hbMoveValIds.isEmpty()) {
      for (CollIndex j = 0; j < hbMoveValIds.entries(); j++,i++) {
	valId = hbMoveValIds[j];

	ItemExpr * itemExpr = valId.getItemExpr();
	Attributes * hbAttr = (generator->getMapInfo(valId))->getAttr();

	// copy this value and add it to the map table.
	Convert * newItemExpr = new(generator->wHeap()) Convert (itemExpr);
	  
	// bind/type propagate the new node
	newItemExpr->bindNode(generator->getBindWA());    
	  
	attrs[i] = 
	  (generator->addMapInfo(newItemExpr->getValueId(), 0))->getAttr();
	ofMoveValIds.insert(newItemExpr->getValueId());

	// add the search condition
	BiRelat * biRelat = new(generator->wHeap())
	  BiRelat(ITM_EQUAL, itemExpr, newItemExpr);
	biRelat->setSpecialNulls(-1);
	biRelat->bindNode(generator->getBindWA());
	ofSearchValIds.insert(biRelat->getValueId());
      }
    };

    
    // set the attributes and offsets to represent the rows in the
    // hash buffer
    // Offsets are based on the row starting after the HashRow structure.
    expGen->processAttributes(numAttrs,
                              attrs,
                              tupleFormat,
                              groupedRowLength,
                              workAtpPos,
                              hbRowAtpIndex,
                              0,
                              ExpTupleDesc::SHORT_FORMAT,
                              0,
                              hdrInfo);

    NADELETEBASIC(attrs, generator->wHeap());

    // generate the overflow buffer aggregate expression which aggregates
    // rows from the overflow buffer into groups in the hash buffer
    if (NOT upperAggrValIds.isEmpty())
      expGen->generateAggrExpr(upperAggrValIds, ex_expr::exp_AGGR,
			       &ofAggrExpr, 0, 
                               (NOT hbMoveValIds.isEmpty()));

    if (NOT hbMoveValIds.isEmpty()) {
      // generate the move expression. This is used to move the
      // grouping values from the overflow buffer to the hash buffer.
      expGen->generateSetExpr(ofMoveValIds,
			      ex_expr::exp_ARITH_EXPR,
			      &ofMoveInExpr);

      // generate the search expression. This expression is used
      // to look for a matching value in the hash table.
      ItemExpr * newPredTree =
	ofSearchValIds.rebuildExprTree(ITM_AND, TRUE, TRUE);
      
      expGen->generateExpr(newPredTree->getValueId(),
			   ex_expr::exp_SCAN_PRED,
			   &ofSearchExpr );
    };
  };

  if (hdrInfo)
    NADELETEBASIC( hdrInfo, generator->wHeap() );

  // remove all the map tables generated by this node so far
  generator->removeAll(myMapTable1);

  // add the resultMapTable
  generator->appendAtEnd(resultValMapTable);

  // insert the original valueIds into the map table and
  // change the atp of aggregate values to 0.
  // All references to these values from this point on
  // will be at atp = 0, atp_index = resultRowAtpIndex.
  for (CollIndex ix = 0; ix <  lowerAggrValIdList.entries(); ix++) {
    //    valId = lowerAggrValIdList[ix];
    valId = gendAggrValIdList[ix];
      
    // get the "new" attributes
    Attributes * newAggrAttr =
      (generator->getMapInfo(valId))->getAttr();
    
    // insert the original valIds into the map table
    ValueId originalValId = originalAggrValIds[ix];
    MapInfo * originalMapInfo = generator->addMapInfo(originalValId, 0);
    Attributes * originalAggrAttr = originalMapInfo->getAttr();
    
    // set the attributes of the original valIds
    originalAggrAttr->copyLocationAttrs(newAggrAttr);
    originalAggrAttr->setAtp(0);
    originalAggrAttr->setAtpIndex(returnedAtpIndex);
    
    // code has been generated for originalValId and a
    // value is available. Mark it so.
    originalMapInfo->codeGenerated();
  };

  // the hbMoveValIds (set of convert nodes) was used to move the incoming
  // group by values to the aggregate buffer. The child of the convert node
  // is the original grouping column. From this point on, the grouping value
  // is available in the result buffer. Change the buffer attributes of
  // the grouping values in the map table to point to this new location.
  // Also, the atp value of the grouping columns is now 0.
  // This change of attributes is not done if the grouping column
  // is a 'constant'.
  if (NOT hbMoveValIds.isEmpty()) {
    ValueId convValId;

    for (CollIndex i = 0; i < hbMoveValIds.entries(); i++) {
      convValId = hbMoveValIds[i];
      Attributes * newGroupColAttr =
	(generator->getMapInfo(convValId))->getAttr();
	
      ValueId valId = gbyValIdList[i];

      // change the location of the grouping columns, unless they
      // are input to this node. Input values already have location
      // assigned to them.
      if ((NOT getGroupAttr()->getCharacteristicInputs().contains(valId)) &&
           (valId.getItemExpr()->getOperatorType() != ITM_CONSTANT)) {
	MapInfo * mapInfo = generator->addMapInfo(valId, 0);
	Attributes * oldGroupColAttr = mapInfo->getAttr();
	  
	oldGroupColAttr->copyLocationAttrs(newGroupColAttr);
	oldGroupColAttr->setAtp(0);
	oldGroupColAttr->setAtpIndex(returnedAtpIndex);
	  
	// code has been generated for valId and a value is available.
	// Mark it so.
	mapInfo->codeGenerated();
      }
    }
  };

  // generate having expression, if present
  if (NOT selectionPred().isEmpty()) {
    ItemExpr * newPredTree =
      selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
    expGen->generateExpr(newPredTree->getValueId(),
	             ex_expr::exp_SCAN_PRED, &havingExpr);

  };

  ExplainTuple *childExplainTuple = generator->getExplainTuple();

  returnedDesc->
#pragma nowarn(1506)   // warning elimination 
    setTupleDescriptor(returnedDesc->noTuples() - 1, tupleDesc);
#pragma warn(1506)  // warning elimination 

  // This estimate on the number of groups should work correctly even when
  // under the right side of a NestedLoopJoin (i.e., multiple probes are
  // sent to this HGB, but the estimate is "per a single probe".)
  // This estimate is only used to determine the number of clusters; the
  // hash-table would adapt dynamically to the number of groups.
  Cardinality expectedRows = (Cardinality) getEstRowsUsed().getValue() ;

  // If this HGB is performed within ESPs, then number of records
  // processed by each ESP is a subset of total records.
  if ( saveNumEsps > 0 )
    expectedRows /= (Cardinality) saveNumEsps ;

  // also, make sure that we don't run into overflow problems.
  // estimatedRowCount_ is an estimate. Therfore it is ok to limit it
  
  // the c89 doesn't handle the direct comparison of float and
  // UINT_MAX correctly. For now we use 4294967295.0!!!!!!!
  if (expectedRows > 4294967295.0)
    expectedRows  = (float)4294967295.0;

  // determine the size of the HGB buffers. This buffer size is used for
  // 1 - store the rows in the hash table (HashTableBuffer)
  // 2 - store the result rows (sql_buffer)
  // first determine the minimum size for the hash table buffers.
  // a buffer has to store at least one (extended) group/row plus tupp_desc

  // get the default value for the buffer size
  ULng32 bufferSize = (ULng32) getDefault(GEN_HGBY_BUFFER_SIZE);

  ULng32 hashBufferSize = extGroupRowLength + sizeof(tupp_descriptor);
  bufferSize = MAXOF( hashBufferSize, bufferSize );

  // minimum result buffer size
  ULng32 resBufferSize = resultRowLength + sizeof(tupp_descriptor);
  bufferSize = MAXOF( resBufferSize, bufferSize );

  short scrthreshold = 
    (short) CmpCommon::getDefaultLong(SCRATCH_FREESPACE_THRESHOLD_PERCENT);
  short hgbGrowthPercent = 
    RelExpr::bmoGrowthPercent(getEstRowsUsed(), getMaxCardEst());

  ComTdbHashGrby * hashGrbyTdb = new(space)
    ComTdbHashGrby(childTdb,
		   givenDesc,
		   returnedDesc,
		   hashExpr,
		   bitMuxExpr,
		   bitMuxAggrExpr,
		   hbMoveInExpr,
		   ofMoveInExpr,
		   resMoveInExpr,
		   hbAggrExpr,
		   ofAggrExpr,
		   resAggrExpr,
		   havingExpr,
		   moveOutExpr,
		   hbSearchExpr,
		   ofSearchExpr,
		   keyLength,
		   resultRowLength,
		   extGroupRowLength,
		   workCriDesc,
		   hbRowAtpIndex,
		   ofRowAtpIndex,
		   hashValueAtpIndex,
		   bitMuxAtpIndex,
		   bitMuxCountOffset,
		   resultRowAtpIndex,
		   returnedAtpIndex,
		   (unsigned short)getDefault(BMO_MEMORY_USAGE_PERCENT),
		   (short)getDefault(GEN_MEM_PRESSURE_THRESHOLD),
                   scrthreshold,
		   (queue_index)getDefault(GEN_HGBY_SIZE_DOWN),
		   (queue_index)getDefault(GEN_HGBY_SIZE_UP),
		   isPartialGroupBy,
		   expectedRows,
		   (Lng32)getDefault(GEN_HGBY_NUM_BUFFERS),
		   bufferSize,
		   getDefault(GEN_HGBY_PARTIAL_GROUP_FLUSH_THRESHOLD),
		   getDefault(GEN_HGBY_PARTIAL_GROUP_ROWS_PER_CLUSTER),
		   (ULng32)getDefault(EXE_HGB_INITIAL_HT_SIZE),
		   // To get the min number of buffers per a flushed cluster
		   // before is can be flushed again
		   (unsigned short)getDefault(EXE_NUM_CONCURRENT_SCRATCH_IOS)
		   + (short)getDefault(COMP_INT_66), // for testing
		   (ULng32) getDefault(COMP_INT_67), // numInBatch
                   hgbGrowthPercent
		   );

  generator->initTdbFields(hashGrbyTdb);
  hashGrbyTdb->setOverflowMode(generator->getOverflowMode());
  if (CmpCommon::getDefault(EXE_DIAGNOSTIC_EVENTS) == DF_ON)
    hashGrbyTdb->setLogDiagnostics(TRUE);
  hashGrbyTdb->setBmoMinMemBeforePressureCheck((Int16)getDefault(EXE_BMO_MIN_SIZE_BEFORE_PRESSURE_CHECK_IN_MB));

  if(generator->getOverflowMode() == ComTdb::OFM_SSD )
    hashGrbyTdb->setBMOMaxMemThresholdMB((UInt16)(ActiveSchemaDB()->
				   getDefaults()).
			  getAsLong(SSD_BMO_MAX_MEM_THRESHOLD_IN_MB));
  else
    hashGrbyTdb->setBMOMaxMemThresholdMB((UInt16)(ActiveSchemaDB()->
				   getDefaults()).
			  getAsLong(EXE_MEMORY_AVAILABLE_IN_MB));

  hashGrbyTdb->setScratchIOVectorSize((Int16)getDefault(SCRATCH_IO_VECTOR_SIZE_HASH));

  double memQuota = 0;

  if(isPartialGroupBy) {
    // The Quota system does not apply to Partial GroupBy
    UInt16 partialMem = 
      (UInt16)(ActiveSchemaDB()->getDefaults()).
      getAsULong(EXE_MEMORY_FOR_PARTIALHGB_IN_MB);
    hashGrbyTdb->setPartialGrbyMemoryMB(partialMem);

    // Test the PASS Thru mode.
    if (CmpCommon::getDefault(COMP_BOOL_159) == DF_ON)
      hashGrbyTdb->setPassPartialRows(TRUE);

  } else {

    // The CQD EXE_MEM_LIMIT_PER_BMO_IN_MB has precedence over the mem quota sys

    NADefaults &defs            = ActiveSchemaDB()->getDefaults();

    UInt16 mmu = UInt16(defs.getAsDouble(EXE_MEM_LIMIT_PER_BMO_IN_MB));

    UInt16 numBMOsInFrag = (UInt16)generator->getFragmentDir()->getNumBMOs();

    if (mmu != 0) {
      hashGrbyTdb->setMemoryQuotaMB(mmu);
      memQuota = mmu;
    } else { 

      // Apply quota system if either one the following two is true:
      //   1. the memory limit feature is turned off and more than one BMOs
      //   2. the memory limit feature is turned on
      NABoolean mlimitPerCPU = defs.getAsDouble(EXE_MEMORY_LIMIT_PER_CPU) > 0;

      if ( mlimitPerCPU || numBMOsInFrag > 1 ||
           (numBMOsInFrag == 1 && CmpCommon::getDefault(EXE_SINGLE_BMO_QUOTA) == DF_ON)) {
        memQuota =
           computeMemoryQuota(generator->getEspLevel() == 0,
                              mlimitPerCPU,
                              generator->getBMOsMemoryLimitPerCPU().value(),
                              generator->getTotalNumBMOsPerCPU(),
                              generator->getTotalBMOsMemoryPerCPU().value(),
                              numBMOsInFrag, 
                              generator->getFragmentDir()->getBMOsMemoryUsage()
                             );

        Lng32 hjGyMemoryLowbound = 
            defs.getAsLong(EXE_MEMORY_LIMIT_LOWER_BOUND_HASHGROUPBY);

        if ( memQuota < hjGyMemoryLowbound )
           memQuota = hjGyMemoryLowbound;

        hashGrbyTdb->setMemoryQuotaMB( UInt16(memQuota) );
      }
    }

    generator->addToTotalOverflowMemory(
          getEstimatedRunTimeOverflowSize(memQuota)
                                    );
  }

  generator->addToTotalOverflowMemory(
           getEstimatedRunTimeOverflowSize(memQuota)
                                     );

  // For debugging overflow only (default is zero == not used).
  hashGrbyTdb->
    setForceOverflowEvery((UInt16)(ActiveSchemaDB()->getDefaults()).
			  getAsULong(EXE_TEST_HASH_FORCE_OVERFLOW_EVERY));

  double hashGBMemEst = getEstimatedRunTimeMemoryUsage(hashGrbyTdb);
  generator->addToTotalEstimatedMemory(hashGBMemEst);

  if ( generator->getRightSideOfFlow() ) 
    hashGrbyTdb->setPossibleMultipleCalls(TRUE);

  Lng32 hgbMemEstInKBPerCPU = (Lng32)(hashGBMemEst / 1024) ;
  hgbMemEstInKBPerCPU = hgbMemEstInKBPerCPU/
    (MAXOF(generator->compilerStatsInfo().dop(),1));
  hashGrbyTdb->setHgbMemEstInMbPerCpu
    ( Float32(MAXOF(hgbMemEstInKBPerCPU/1024,1)) );

  // For now use variable size records whenever Aligned format is
  // used.
  if (resizeCifRecord) {//tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) {
    hashGrbyTdb->setUseVariableLength();
    if (considerBufferDefrag)
    {
      hashGrbyTdb->setConsiderBufferDefrag();
    }
  }

  hashGrbyTdb->setCIFON((tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT));
  if(!generator->explainDisabled()) {
    generator->setOperEstimatedMemory(hgbMemEstInKBPerCPU);

    generator->setExplainTuple(
       addExplainInfo(hashGrbyTdb, childExplainTuple, 0, generator));

    generator->setOperEstimatedMemory(0);
  }



  // set the new up cri desc.
  generator->setCriDesc(returnedDesc, Generator::UP);

  // restore the original down cri desc since this node changed it.
  generator->setCriDesc(givenDesc, Generator::DOWN);

  generator->setGenObj(this, hashGrbyTdb);

  // reset the expression generation flag to generate float validation pcode
  generator->setGenNoFloatValidatePCode(FALSE);

  // reset the handleIndirectVC flag to its initial value
  expGen->setHandleIndirectVC( vcflag );

  return 0;
} // HashGroupBy::codeGen()


ExpTupleDesc::TupleDataFormat HashGroupBy::determineInternalFormat( const ValueIdList & valIdList,
                                                                   RelExpr * relExpr,
                                                                   NABoolean & resizeCifRecord,
                                                                   Generator * generator,
                                                                   NABoolean bmo_affinity,
                                                                   NABoolean & considerBufferDefrag)
{

  RelExpr::CifUseOptions bmo_cif = RelExpr::CIF_SYSTEM;


  if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO) == DF_OFF)
  {
    bmo_cif = RelExpr::CIF_OFF;
  }
  else
  if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO) == DF_ON)
  {
    bmo_cif = RelExpr::CIF_ON;
  }

  //CIF_SYSTEM

  return generator->determineInternalFormat(valIdList, 
                                            relExpr, 
                                            resizeCifRecord, 
                                            bmo_cif,
                                            bmo_affinity,
                                            considerBufferDefrag);

}

CostScalar HashGroupBy::getEstimatedRunTimeMemoryUsage(NABoolean perCPU)
{
  GroupAttributes * childGroupAttr = child(0).getGroupAttr();
  const CostScalar childRecordSize = childGroupAttr->getCharacteristicOutputs().getRowLength();
  const CostScalar childRowCount = getEstRowsUsed(); // the number of 
                                                     // distinct rows groupped
  // Each record also uses a header (HashRow) in memory (8 bytes for 32bit).
  // Hash tables also take memory -- they are about %50 longer than the 
  // number of entries.
  const ULng32 
    memOverheadPerRecord = sizeof(HashRow) + sizeof(HashTableHeader) * 3 / 2 ;

  // totalHashTableMemory is for all CPUs at this point of time.
  CostScalar totalHashTableMemory = 
    childRowCount * (childRecordSize + memOverheadPerRecord);

  if ( perCPU == TRUE ) {
     const PhysicalProperty* const phyProp = getPhysicalProperty();
     if (phyProp)
     {
       PartitioningFunction * partFunc = phyProp -> getPartitioningFunction() ;

      // totalHashTableMemory is per CPU at this point of time.
       totalHashTableMemory /= partFunc->getCountOfPartitions();
     }
  }
  return totalHashTableMemory;
}

double HashGroupBy::getEstimatedRunTimeMemoryUsage(ComTdb * tdb)
{
  CostScalar totalHashTableMemory = getEstimatedRunTimeMemoryUsage(FALSE);

  double memoryLimitPerCpu;
  ULng32 memoryQuotaInMB = ((ComTdbHashGrby *)tdb)->memoryQuotaMB();
  if (memoryQuotaInMB)
    memoryLimitPerCpu = memoryQuotaInMB * 1024 * 1024 ;
  else if ( isNotAPartialGroupBy() == FALSE)
  {
    memoryLimitPerCpu = 
      ActiveSchemaDB()->getDefaults().getAsLong(EXE_MEMORY_FOR_PARTIALHGB_IN_MB) * 1024 * 1024;
  }
  else
  {
    memoryLimitPerCpu = 
        ActiveSchemaDB()->getDefaults().getAsLong(EXE_MEMORY_AVAILABLE_IN_MB) * 1024 * 1024 ;
  }

  const PhysicalProperty* const phyProp = getPhysicalProperty();
  Lng32 numOfStreams = 1;
  if (phyProp)
  {
    PartitioningFunction * partFunc = phyProp -> getPartitioningFunction() ;
    numOfStreams = partFunc->getCountOfPartitions();
  }

  CostScalar memoryPerCpu = totalHashTableMemory/numOfStreams ;
  if ( memoryPerCpu > memoryLimitPerCpu ) 
  {
      memoryPerCpu = memoryLimitPerCpu;
  }
  totalHashTableMemory = memoryPerCpu * numOfStreams ;
  return totalHashTableMemory.value();
}

double HashGroupBy::getEstimatedRunTimeOverflowSize(double memoryQuotaMB)
{

  if ( memoryQuotaMB > 0 ) {

     CostScalar memoryUsage =
        getEstimatedRunTimeMemoryUsage(TRUE /*per CPU*/);

     double delta = memoryUsage.getValue() - memoryQuotaMB * COM_ONE_MEG ;

     if ( delta > 0 ) {
        const PhysicalProperty* const phyProp = getPhysicalProperty();
        Lng32 pipelines = 1;
   
        if (phyProp)
        {
          PartitioningFunction * partFunc = 
                   phyProp -> getPartitioningFunction() ;
   
          if ( partFunc )
             pipelines = partFunc -> getCountOfPartitions();
        }
   
   
        return delta * pipelines;
     } 
  } 

  return 0;

}

/////////////////////////////////////////////////////////
//
// GroupByAgg::codeGen()
//
//  Used by SortGroupBy and ShortCutGroupBy 
/////////////////////////////////////////////////////////

short GroupByAgg::codeGen(Generator * generator) {

  Space * space = generator->getSpace();
  
  // create a map table for returned group by and aggregate values
  MapTable * myMapTable = generator->appendAtEnd();

  ////////////////////////////////////////////////////////////////////////////
  //
  // Layout at this node:
  //
  // |-------------------------------------------------|
  // | input data  |  Grouped data | child's data      |
  // | ( I tupps ) |  ( 1 tupp )   | ( C tupps )       |
  // |-------------------------------------------------|
  // <-- returned row to parent --->
  // <------------ returned row from child ------------>
  //
  // input data:        the atp input to this node by its parent. 
  // grouped data:      tupp where the aggr/grouped result is moved
  // child data:        tupps appended by the left child
  //
  // Input to child:    I + 1 tupps
  //
  // Tupps returned from child are only used to create the
  // grouped data. They are not returned to parent.
  //
  ////////////////////////////////////////////////////////////////////////////

  ex_cri_desc * givenDesc
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc 
#pragma nowarn(1506)   // warning elimination 
    = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space); 
#pragma warn(1506)  // warning elimination 
  
  generator->setCriDesc(returnedDesc, Generator::DOWN);

  ComTdb * childTdb = 0;

  ex_expr * aggrExpr = 0;
  ex_expr * havingExpr = 0;
  ex_expr * grbyExpr = 0;
  ex_expr * moveExpr = 0;

  ExpTupleDesc * tupleDesc = 0;
  
  genAggrGrbyExpr(generator,
		  aggregateExpr(),
                  groupExpr(),
		  rollupGroupExprList(),
		  selectionPred(),
		  1,
		  returnedDesc->noTuples() - 1, 
		  (short) (returnedDesc->noTuples() - 1),
		  &aggrExpr,
		  &grbyExpr,
		  &moveExpr,
		  &havingExpr,
		  &childTdb,
		  &tupleDesc);


  ExplainTuple *childExplainTuple = generator->getExplainTuple();

  returnedDesc->
#pragma nowarn(1506)   // warning elimination 
    setTupleDescriptor(returnedDesc->noTuples() - 1, tupleDesc); 
 
  ComTdbSortGrby * sortGrbyTdb 
    = new(space) ComTdbSortGrby(aggrExpr,
				grbyExpr,
				moveExpr,
				havingExpr,
				tupleDesc->tupleDataLength(),
				returnedDesc->noTuples()-1,
				childTdb,
				givenDesc,
				returnedDesc,
				(queue_index)getDefault(GEN_SGBY_SIZE_DOWN),
				(queue_index)getDefault(GEN_SGBY_SIZE_UP),
				(Cardinality) getGroupAttr()->
				getOutputLogPropList()[0]->
				getResultCardinality().value(),
				getDefault(GEN_SGBY_NUM_BUFFERS),
				getDefault(GEN_SGBY_BUFFER_SIZE),
				generator->getTolerateNonFatalError());
#pragma warn(1506)  // warning elimination 
  generator->initTdbFields(sortGrbyTdb);

  if (isRollup())
    {
      sortGrbyTdb->setIsRollup(TRUE);

      sortGrbyTdb->setNumRollupGroups(rollupGroupExprList().entries());
    }

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
         addExplainInfo(sortGrbyTdb, childExplainTuple, 0, generator));
  }
 
  // set the new up cri desc.
  generator->setCriDesc(returnedDesc, Generator::UP);

  // restore the original down cri desc since this node changed it.
  generator->setCriDesc(givenDesc, Generator::DOWN);

  generator->setGenObj(this, sortGrbyTdb);

  return 0;
} // GroupByAgg::codeGen()


