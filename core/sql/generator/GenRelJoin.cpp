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
* File:         GenRelJoin.C
* Description:  Join operators
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
#include "ComDefs.h"            // to get common defines (ROUND8)
#include "limits.h"
#include "ComOptIncludes.h"
#include "RelJoin.h"
#include "GroupAttr.h"
#include "Analyzer.h"
#include "Generator.h"
#include "GenExpGenerator.h"
//#include "ex_stdh.h"
#include "ExpCriDesc.h"
#include "ComTdb.h"
//#include "ex_tcb.h"
#include "ComTdbOnlj.h"
#include "ComTdbHashj.h"
#include "ComTdbMj.h"
#include "ComTdbTupleFlow.h"
#include "HashBufferHeader.h"
#if 0
// unused feature, done as part of SQ SQL code cleanup effort
#include "ComTdbSimpleSample.h"
#endif // if 0
#include "DefaultConstants.h"
#include "HashRow.h"
#include "hash_table.h" // for HashTableHeader
#include "ExpSqlTupp.h" // for sizeof(tupp_descriptor)
#include "sql_buffer.h"
#include "sql_buffer_size.h"
#include "CmpStatement.h"
#include "ComUnits.h"

/////////////////////////////////////////////////////////////////////
//
// Contents:
//
//   HashJoin::codeGen()
//   MergeJoin::codeGen()
//   NestedJoin::codeGen()
//   NestedJoinFlow::codeGen()
//   Join::instantiateValuesForLeftJoin()
//
/////////////////////////////////////////////////////////////////////

short HashJoin::codeGen(Generator * generator) {

  // Decide if this join can use the Unique Hash Join option.  This
  // option can be significantly faster than the regular hash join,
  // but does not support many features.
  //
  NABoolean useUniqueHashJoin = canUseUniqueHashJoin();

  ExpGenerator * expGen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  GenAssert( ! isSemiJoin()  || ! isAntiSemiJoin(),
	     "Node can not be both semi-join and anti-semi-join" );
  GenAssert( ! isReuse()  ||  isNoOverflow(),
	     "Reuse of inner table requires no-overflow!" );

  // set flag to enable pcode for indirect varchar
  NABoolean vcflag = expGen->handleIndirectVC();
  if (CmpCommon::getDefault(VARCHAR_PCODE) == DF_ON) {
    expGen->setHandleIndirectVC( TRUE );
  }

  // values are returned from the right when it is not (anti) semi join
  NABoolean rightOutputNeeded = ! isSemiJoin() && ! isAntiSemiJoin() ;

  // the minMaxExpr is used when the min max optimization is in
  // effect.  It is evaluated at the same time as the rightHashExpr
  // and computes the min and max values for one or more of the join
  // values coming from the right (inner) side.
  ex_expr * minMaxExpr = 0;

  // the rightHashExpr takes a row from the right child (the "build
  // table") and calculates the hash value. After the expression is
  // evaluated, the hash value is available in the hashValueTupp_ of
  // the hash join tcb (compare the description of the workAtp).
  ex_expr * rightHashExpr = 0;

  // the rightMoveInExpr expression moves the incoming right row
  // to a contiguous buffer. This buffer is later inserted into the
  // hash table. The row in the buffer looks like a right row with a
  // header added to it. Thus, it is called "extended" right row. The
  // row header contains the hash value and a pointer for the hash
  // chain.
  ex_expr * rightMoveInExpr = 0;

  // the rightMoveOutExpr moves a "extended" right row to a regular right
  // row (one without a row header). This is done if the "extended" right
  // row is part of a result.
  // This expression is not really required. The "extended" row in the
  // hash buffer is always contiguous. So is the row in the result buffer.
  // Thus, the executor can simply use a byte move. This is true, if the
  // "extended" row in the hash buffer has the following format:
  //
  // |------------------------------------------------------|
  // | Header | right columns | hash expression (see below) |
  // |------------------------------------------------------|
  //
  // A byte move just skips the header (hash value & next pointer) and
  // hash expression. It only moves the right columns. The following code
  // guarantees that the hash expression always follows the right row
  // data.
  //
  // Please note that for now the generator still generates the
  // rightMoveOutExpr. It is simply not used by the executor.
  ex_expr * rightMoveOutExpr = 0;

  // the rightSearchExpr compares two "extended" rows from the right child.
  // Both rows are stored in the contiguous buffer of the appropriate
  // cluster. The expression is used while chaining rows into the hash
  // table. The result of the expression is boolean.
  ex_expr *rightSearchExpr = 0;

  // the leftHashExpr takes a row from the left child and calculates
  // the hash value. After the expression is evaluated, the hash value is
  // available in the hashValueTupp_ of the hash join tcb.
  ex_expr * leftHashExpr = 0;

  // the leftMoveExpr moves the incoming left row dircetly to the parents
  // buffer. This happens during phase 2 of the hash join if the hash table
  // is immediately probed with the left row. If the row qualifies, there
  // is no need to move it to an "extended" left row first.
  ex_expr * leftMoveExpr = 0;

  // the leftMoveInExpr moves the incomming left row into a contiguous
  // buffer. The row in the buffer is also extended by a row header to
  // store the hash value and a hash chain pointer.
  ex_expr * leftMoveInExpr = 0;

  // the leftMoveOutExpr is used to move an "extended" left row to the
  // parents Atp if the row is part of the result. The expression gets
  // rid of the row header. Again, this expression is not really required.
  // The same arguments as for the rightMoveOutExpr holds. For the
  // leftMoveOutExpr it is even easier, because there is no additional
  // data (hash expression). Again, for now the expression is generated but
  // ignored.
  ex_expr * leftMoveOutExpr = 0;

  // the probeSearchExpr1 compares an incoming left row with an "extended"
  // right row. This expression is used if the hash table is probed right
  // away with a left row without moving the left row into a contiguous
  // buffer. This happens during phase 2 of the hash join. The result of the
  // expression is boolean.
  ex_expr * probeSearchExpr1 = 0;

  // the probeSearchExpr2 compares an "extended" left row with an "extended"
  // right row. The rsult of the expression is boolean. Ths expression is
  // used during phase 3 of the hash join
  ex_expr * probeSearchExpr2 = 0;

  // the leftJoinExpr is used in some left join cases, were we have to
  // null instantiate a row.
  ex_expr * leftJoinExpr = 0;

  // the nullInstForLeftJoinExpr puts null values into the null
  // instantiated row. The instatiatied row is a right row without
  // a row header.
  ex_expr * nullInstForLeftJoinExpr = 0;

  // the rightJoinExpr is used in some right join cases, where we have to
  // null instantiate a row from the left.
  ex_expr * rightJoinExpr = 0;

  // the nullInstForRightJoinExpr puts null values into the null instantiated
  // row. The instatiatied row is a right row without a row header.
  ex_expr * nullInstForRightJoinExpr = 0;

  // the beforeJoinPred1 compares a row from the left child with an
  // "extended" right row. It is used during phase 2 of the hash join.
  // The result is boolean.
  ex_expr * beforeJoinPred1 = 0;

  // the beforeJoinPred2 compares an "extended" left  row with an
  // "extended" right row. It is used during phase 3 of the hash join.
  // The result is boolean.
  ex_expr * beforeJoinPred2 = 0;

  // the afterJoinPred1 compares a row from the left child with an
  // "extended" right row. It is used during phase 2 of workProbe,
  // when there is a matching row. Used when there is no overflow,
  // and the left row is a composite row.
  // The result is boolean.
  ex_expr * afterJoinPred1 = 0;
  
  // the afterJoinPred2 compares an "extended" left  row with an
  // "extended" right row. It is used during phase 3 of workProbe,
  // when there is a matching row. This is used after an overflow,
  // when the left row comes from a hash buffer.
  // The result is boolean.
  ex_expr * afterJoinPred2 = 0;

  // variation of afterJoinPred1, used when the "extended" right row has to
  // be the NULL instantiated part in a left join. Compares a row from the
  // left child with an "NullInstantiated" null tuple representing a right row.
  // Used during phase 2 of workProbe when there is no matching right row.
  // Used when there is no overflow, and the left row is a composite row.
  // The result is boolean.
  ex_expr * afterJoinPred3 = 0;

  // variation of afterJoinPred2, used when the "extended" right row has to
  // be the NULL instantiated part in a left join. Compares an "extended" left
  // row with an "NullInstantiated" null tuple representing a right row.
  // Used during phase 3 of workProbe when there is no matching right row.
  // This is used after an overflow, when the left row comes from a hash buffer.
  // The result is boolean.
  ex_expr * afterJoinPred4 = 0;

  // the afterJoinPred5 compares a "NullInstantiated" null tuple representing a
  // left  row with a "NullInstantiated" right row. It is used during workReturnRightRows
  // to process the rows from the right which did not have a matching left row.
  // The result is boolean.
  ex_expr * afterJoinPred5 = 0;

  const ValueIdSet &rightOutputCols =
    child(1)->getGroupAttr()->getCharacteristicOutputs();

  // right table columns to be inserted into the return buffer
  // (if not semi join)

  ValueIdList rightOutputValIds;

  // right table columns to be inserted into the hash buffer. For
  // "normal" cases these columns are identical to rightOutputValIds.
  // However, if the join predicate involves expressions, we also
  // move these expressions into the hash buffer
  ValueIdList rightBufferValIds;

  ValueId valId;

  // add only those ValueIds to rightOutputValIds and rightBufferValIds
  // which are not part of the input.
  for (valId = rightOutputCols.init();
       rightOutputCols.next(valId);
       rightOutputCols.advance(valId))
    if (NOT getGroupAttr()->getCharacteristicInputs().contains(valId)) {
      rightOutputValIds.insert(valId);
      rightBufferValIds.insert(valId);
    };


  // left table columns to be inserted into the hash buffer and the
  // return buffer
  const ValueIdSet &leftOutputCols =
    child(0)->getGroupAttr()->getCharacteristicOutputs();

  ValueIdList leftOutputValIds;

  // UniqueHashJoin does not MOVE the left values.
  // It simply passes them to the parent queue using copyAtp()
  // So do not build the list of left outputs.
  //
  if (!useUniqueHashJoin) {

    // add only those ValueIds to leftOutputValIds
    // which are not part of the input.
    for (valId = leftOutputCols.init();
         leftOutputCols.next(valId);
         leftOutputCols.advance(valId))
      if (NOT getGroupAttr()->getCharacteristicInputs().contains(valId))
        leftOutputValIds.insert(valId);
  }


  // allocate 2 map tables, so that we can later remove the right child MT
  MapTable * myMapTable0 = generator->appendAtEnd();
  MapTable * myMapTable1 = generator->appendAtEnd();

  ex_cri_desc * givenDesc = generator->getCriDesc(Generator::DOWN);

  // Incoming records will be divided equally (sans data skew) among the ESPs
  // so each HJ instance will handle only its share (i.e. divide by #esps)
  Lng32 saveNumEsps = generator->getNumESPs();

  ////////////////////////////////////////////////////////////////////////////
  // generate the right child
  ////////////////////////////////////////////////////////////////////////////
  generator->setCriDesc(givenDesc, Generator::DOWN);
  child(1)->codeGen(generator);
  ComTdb * rightChildTdb = (ComTdb *)(generator->getGenObj());
  ExplainTuple *rightExplainTuple = generator->getExplainTuple();

  // This value was originally set inside generator by my parent exchange node
  // as a global variable. Now need to reset the saveNumEsps value back into
  // generator since codegen of child(1) may have changed it.
  generator->setNumESPs(saveNumEsps);

  // A MapTable for the Min and Max values used by the min/max
  // optimization.
  MapTable *myMapTableMM = NULL;

  // Normally the left down request is the same as the parent request
  // (givenDesc).  But if this HashJoin is doing the min max
  // optimization, then the left down request will contain an extra
  // tuple for the min and max values.  In this case, we will create a
  // new criDesc for the left down request
  ex_cri_desc * leftDownDesc = givenDesc;

  // The length of the min max tuple used by the min/max optimization.
  ULng32 minMaxRowLength = 0;

  // If we are doing min/max optimization, get the min/max values into
  // the map table and marked as codegen'ed before we codegen the left
  // child.  These are just the placeholders for the computed min/max
  // values.  Later we will map the actual min/max values to the at
  // same location in the min/max tuple.
  if(getMinMaxVals().entries())
    {

      // This map table is a placeholder, used so we can unlink the
      // min/max mapTable (myMapTableMM)
      MapTable * myMapTableMM1 = generator->appendAtEnd();

      // Add the map table for the min and max values.
      myMapTableMM = generator->appendAtEnd();
      
      // Allocate a new leftDownDesc which will have one additional
      // tuple for the min/max values
      leftDownDesc = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);

      // The index of the min/max tuple in the down request.
      short minMaxValsDownAtpIndex = leftDownDesc->noTuples()-1;

      // Layout the min/max values in the min/max tuple and add
      // corresponding entries into the min/max mapTable
      // (myMapTableMM).
      expGen->processValIdList(getMinMaxVals(),
                               ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
                               minMaxRowLength, 0, minMaxValsDownAtpIndex);

      // Mark these as codegen'ed so the scans that use these will not
      // attempt to generate code for them.
      generator->getMapInfo(getMinMaxVals()[0])->codeGenerated();
      generator->getMapInfo(getMinMaxVals()[1])->codeGenerated();

      generator->unlinkNext(myMapTableMM1);
    }

  // before we generate the left child, we have to remove the map table of
  // the right child, because the left child should not see its siblings
  // valueIds (especially since we sometimes see duplicate valueIds due
  // to VEGs). Because of these duplicates we have to make sure that
  // the order of valueIds in the map table is later right child valueIds,
  // left child valueIds. To make a long story short, we first generate
  // the right child, unchain the map table of the right child, generate
  // the left child and then chain the right childs map table in front
  // of the lefts childs map table. The map table interface is not well
  // suited for these kinds of chain/unchain operations. This is why we
  // see so many empty helper map tables in this method here. There are
  // two other possible solutions to this problem:
  // 1 - extend the map table interface to make these manipulations easier
  // 2 - make changes in the normalizer/VEG code to avoid duplicate
  //     valueIds.
  // For now we have all these extra map tables!

  // ok, here we go: unchain right child map table
  generator->unlinkNext(myMapTable0);
  
  // Add the min/max maptable so the left child scans that use them will
  // have access to them.
  if(myMapTableMM)
    generator->appendAtEnd(myMapTableMM);

  // allocate 2 map tables, so that we can later remove the left child MT
  MapTable * myMapTable2 = generator->appendAtEnd();
  MapTable * myMapTable3 = generator->appendAtEnd();

  ////////////////////////////////////////////////////////////////////////////
  // generate code for left child
  ////////////////////////////////////////////////////////////////////////////
  generator->setCriDesc(leftDownDesc, Generator::DOWN);
  child(0)->codeGen(generator);
  ComTdb * leftChildTdb = (ComTdb *)(generator->getGenObj());
  ExplainTuple *leftExplainTuple = generator->getExplainTuple();
  ex_cri_desc * leftChildDesc = generator->getCriDesc(Generator::UP);

  // This value was originally set inside generator by my parent exchange node
  // as a global variable. Now need to reset the saveNumEsps value back into
  // generator since codegen of my child(0) may have changed it.
  generator->setNumESPs(saveNumEsps);

  short returnedTuples;
  short returnedLeftRowAtpIndex = -1;
  short returnedRightRowAtpIndex = -1;
  short returnedInstRowAtpIndex = -1;
  short returnedInstRowForRightJoinAtpIndex = -1;

  // The work atp is passed as the second atp to the expression evaluation
  // procs. Hence, its atp value is set to 1.
  short workAtpPos = 1; // second atp

  short leftRowAtpIndex = -1;
  short extLeftRowAtpIndex = -1;
  short rightRowAtpIndex = -1;
  short extRightRowAtpIndex1 = -1;
  short extRightRowAtpIndex2 = -1;
  short hashValueAtpIndex = -1;

  // The atpIndex of the min/max tuple in the workAtp.
  short minMaxValsAtpIndex = -1;

  short numOfATPIndexes = -1;

  short instRowForLeftJoinAtpIndex = -1;
  short instRowForRightJoinAtpIndex = -1;
  short prevInputTuppIndex = -1;

  // The Unique Hash Join has a different layout for the returned row
  // and for the work Atp.
  //
  if(useUniqueHashJoin) {
    ////////////////////////////////////////////////////////////////////////////
    // Unique Hash Join
    // Layout of row returned by this node.
    //
    // |---------------------------------------------------
    // | input data  | left child's data | hash table row |
    // |             |                   |                |
    // |             |                   |                |
    // | ( I tupps ) | ( L tupps )       | ( 0/1 tupp )   |
    // |---------------------------------------------------
    //
    // input data:       the atp input to this node by its parent.
    // left child data:  the tupps from the left row. Only, if the left row
    //                   is required.
    // hash table row:   the row from the hash table (right child). In case of
    //                   a semi join, this row is not returned
    //
    ////////////////////////////////////////////////////////////////////////////
    returnedTuples = leftChildDesc->noTuples();

    // Right row goes in last entry.
    if ( rightOutputNeeded && rightOutputValIds.entries() > 0) {
      returnedRightRowAtpIndex = returnedTuples++;
    }

    /////////////////////////////////////////////////////////////////////////
    // Unique Hash Join Layout of WorkATP
    // in all the computation below, the hashed row value ids are available
    // in a temporary work atp. They are used from there to build the
    // hash table.
    // Before returning from this proc, these value ids are moved to the
    // map table that is being returned. The work atp contains the following
    // entries:
    // index     what
    // -------------------------------------------------------
    //   0       constants
    //   1       temporaries
    //   2       a row from the left child (not used)
    //   3       an "extended" row from the right child
    //   4       another "extended" row from the right child
    //   5       the calculated hash value
    //   6       the previous input (in case of HT reuse)
    /////////////////////////////////////////////////////////////////////////

    // not used by unique hash join, but needed for some common code
    extLeftRowAtpIndex = 2;

    // not used by unique hash join, but used when generating
    // rightMoveOutExpr.  unique hash join does not use this
    // but needs the maptable generated as a by product.
    rightRowAtpIndex = 2;

    extRightRowAtpIndex1 = 3;
    extRightRowAtpIndex2 = 4;
    hashValueAtpIndex = 5;

    numOfATPIndexes = 6;

    if(isReuse())
      prevInputTuppIndex = numOfATPIndexes++;

    // If using min/max optimization, add one more tuple for min/max values.
    if(getMinMaxVals().entries())
      minMaxValsAtpIndex = numOfATPIndexes++;

  } else {

    ////////////////////////////////////////////////////////////////////////////
    // Regular Hybrid Hash Join
    // Layout of row returned by this node.
    //
    // |---------------------------------------------------------------------|
    // | input data  | left child's data | hash table row | instantiated for |
    // |             |                   |                | right row (left  |
    // |             |                   |                |  join)           |<|
    // | ( I tupps ) | ( 0/1 tupp )      | ( 0/1 tupp )   |  ( 0/1 tupp )    | |
    // |---------------------------------------------------------------------| |
    //                                                                         |
    //                                   |-------------------------------------|
    //                                   |    ------------------------
    //                                   |    | instantiated for      |
    //                                   |--->| left row (right join) |
    //                                        |   (0/1 tupp)          |
    //                                        -------------------------
    //
    // input data:       the atp input to this node by its parent.
    // left child data:  a tupp with the left row. Only, if the left row
    //                   is required. I.e., in certain cases the left row is
    //                   returned.
    // hash table row:   the row from the hash table (right child). In case of
    //                   a semi join, this row is not returned
    // instantiated for
    //          right row: For some left join cases, the hash table rows or
    //                   the null values are instantiated. See proc
    //                   Join::instantiateValuesForLeftJoin for details at
    //                   the end of this file.
    // instantiated for
    //          left row: For some right join cases, the hash table rows or
    //                   the null values are instantiated. See proc
    //                   Join::instantiateValuesForRightJoin for details at
    //                   the end of this file.
    //
    // Returned row to parent contains:
    //
    //   I + 1 tupp, if the left row is not returned
    //   I + 1 tupp, if this is a semi join. Rows from right are not returned.
    //
    // If this is not a semi join, then:
    //    I + 2 tupps, if instantiation is not done.
    //    I + 3 tupps, if instantiation is done only for Left Outer Join.
    //    I + 4 tupps, if instantiation is done only for Full Outer Join.
    //
    ////////////////////////////////////////////////////////////////////////////

#pragma nowarn(1506)   // warning elimination
    returnedTuples = givenDesc->noTuples();
#pragma warn(1506)  // warning elimination

    if (leftOutputValIds.entries())
      returnedLeftRowAtpIndex = returnedTuples++;

    if ( rightOutputNeeded ) {
      returnedRightRowAtpIndex = returnedTuples++;
      if (nullInstantiatedOutput().entries() > 0)
        returnedInstRowAtpIndex = returnedTuples++;
    }

    if (nullInstantiatedForRightJoinOutput().entries() > 0)
      returnedInstRowForRightJoinAtpIndex = returnedTuples++;

    /////////////////////////////////////////////////////////////////////////
    // Regular Hybrid Hash Join Layout of WorkATP
    // in all the computation below, the hashed row value ids are available
    // in a temporary work atp. They are used from there to build the
    // hash table, apply the after predicate, join predicate etc.
    // Before returning from this proc, these value ids are moved to the
    // map table that is being returned. The work atp contains the following
    // entries:
    // index     what
    // -------------------------------------------------------
    //   0       constants
    //   1       temporaries
    //   2       a row from the left child
    //   3       an "extended" row from the left child
    //   4       a row from the right child
    //   5       an "extended" row from the right child
    //   6       another "extended" row from the right child
    //   7       the calculated hash value
    //   8       the instatiated right row for left join
    //   9       the instatiated left row for right join
    //   10/11/12    the previous input (in case of HT reuse)
    /////////////////////////////////////////////////////////////////////////

    leftRowAtpIndex = 2;
    extLeftRowAtpIndex = 3;
    rightRowAtpIndex = 4;
    extRightRowAtpIndex1 = 5;
    extRightRowAtpIndex2 = 6;
    hashValueAtpIndex = 7;

    numOfATPIndexes = 8;

    if (nullInstantiatedOutput().entries() > 0)
      instRowForLeftJoinAtpIndex = numOfATPIndexes++;

    if (nullInstantiatedForRightJoinOutput().entries() > 0)
      instRowForRightJoinAtpIndex = numOfATPIndexes++;

    if(isReuse())
      prevInputTuppIndex = numOfATPIndexes++;

    // If using min/max optimization, add one more tuple for min/max values.
    if(getMinMaxVals().entries())
      minMaxValsAtpIndex = numOfATPIndexes++;
  }

  // If this HashJoin is doing min/max optimization, then generate the
  // aggregate expressions to compute the Min and Max values.
  if(getMinMaxVals().entries()) {

    // Link in the map table for the right child values.
    MapTable *lastMap = generator->getLastMapTable();
    generator->appendAtEnd(myMapTable1);
    
    // A List to contain the Min and Max aggregates
    ValueIdList mmPairs;

    for (CollIndex mmCol = 0; mmCol < getMinMaxCols().entries(); mmCol++) {

      // Cast the value coming from the right child to the common type
      // used to compute the Min/Max values.
      //
      ItemExpr *rightCol = getMinMaxCols()[mmCol].getItemExpr();

      rightCol = new(generator->wHeap()) 
        Cast(rightCol, getMinMaxVals()[(mmCol*2)].getType().newCopy(generator->wHeap()));

      // The min and max aggregates
      ItemExpr *minVal = new(generator->wHeap()) Aggregate(ITM_MIN, rightCol, FALSE);
      ItemExpr *maxVal = new(generator->wHeap()) Aggregate(ITM_MAX, rightCol, FALSE);

      minVal->bindNode(generator->getBindWA());
      maxVal->bindNode(generator->getBindWA());

      ValueId minId = minVal->getValueId();
      ValueId maxId = maxVal->getValueId();

      Attributes * mapAttr;

      // Map the min aggregate to be the same as the min placeholder
      // (system generated hostvar).  Set the ATP/ATPIndex to refer to
      // the min/max tuple in the workAtp
      mapAttr = (generator->getMapInfo(getMinMaxVals()[(mmCol*2)]))->getAttr();
      mapAttr = (generator->addMapInfo(minId, mapAttr))->getAttr();
      mapAttr->setAtp(workAtpPos);
      mapAttr->setAtpIndex(minMaxValsAtpIndex);

      // Map the max aggregate to be the same as the min placeholder
      // (system generated hostvar).  Set the ATP/ATPIndex to refer to
      // the min/max tuple in the workAtp
      mapAttr = (generator->getMapInfo(getMinMaxVals()[((mmCol*2)+1)]))->getAttr();
      mapAttr = (generator->addMapInfo(maxId, mapAttr))->getAttr();
      mapAttr->setAtp(workAtpPos);
      mapAttr->setAtpIndex(minMaxValsAtpIndex);

      // Insert into list
      mmPairs.insert(minId);
      mmPairs.insert(maxId);

    }

    // Generate the min/max expression.
    expGen->generateAggrExpr(mmPairs, ex_expr::exp_AGGR, &minMaxExpr, 
                             0, true);
    
    // No longer need the right child map table.
    generator->unlinkNext(lastMap);
  }

  ex_cri_desc * returnedDesc = new(space) ex_cri_desc(returnedTuples, space);

  // now the unchain/chain business described above. First, unchain the
  // left child map table
  generator->unlinkNext(myMapTable0);
  // add the right child map table
  generator->appendAtEnd(myMapTable1);
  // and finaly add the left child map table
  generator->appendAtEnd(myMapTable2);
  // Here is how the map table list looks like now:
  // MT -> MT0 -> MT1 -> RC MT -> MT2 -> MT3 -> LC MT

  ex_cri_desc * workCriDesc = new(space) ex_cri_desc(numOfATPIndexes, space);

  // This value was originally set inside generator by my parent exchange node
  // as a global variable. Now need to reset the saveNumEsps value back into
  // generator since codegen of my child(0) may have changed it.
  generator->setNumESPs(saveNumEsps);

  // make sure the expressions that are inserted into the hash table
  // include the expression(s) to be hashed (this restriction may be
  // lifted some day in the future, but in all "normal" cases this happens
  // anyway)

  LIST(CollIndex) hashKeyColumns(generator->wHeap());

  ////////////////////////////////////////////////////////////
  // Before generating any expression for this node, set the
  // the expression generation flag not to generate float
  // validation PCode. This is to speed up PCode evaluation
  ////////////////////////////////////////////////////////////
  generator->setGenNoFloatValidatePCode(TRUE);

  ////////////////////////////////////////////////////////////
  // Generate the hash computation expression for the left
  // and right children.
  //
  // The hash value is computed as:
  //   hash_value = Hash(col1) + Hash(col2) .... + Hash(colN)
  ////////////////////////////////////////////////////////////
  ItemExpr * buildHashTree = NULL;
  ItemExpr * probeHashTree = NULL;

  // construct the rightHashExpr and leftHashExpr if
  // it's a full outer join and the right rows
  // have to be returned as well. During the probe
  // for every left row, we mark the right row
  // that matches. At the end (after returning all left rows for left joins)
  // we go thro all the clusters and null instantiate the left row
  // for every right row that is not marked. For this reason, we need
  // the left and right rows into the cluster. For getting the row into a cluster
  // we need the hashValue and hence rightHashExpr and leftHashExpr.
  // TBD Hema
  //      ||
  //      (nullInstantiatedForRightJoinOutput().entries() > 0))

  if (! getEquiJoinPredicates().isEmpty()) {
    ///////////////////////////////////////////////////////////////////////
    // getEquiJoinPredicates() is a set containing predicates of the form:
    //  <left value1> '=' <right value1>, <left value2> '=' <right value2> ..
    //
    // The right values are the hash table key values
    ///////////////////////////////////////////////////////////////////////
    for (valId = getEquiJoinPredicates().init();
	 getEquiJoinPredicates().next(valId);
	 getEquiJoinPredicates().advance(valId)) {
      ItemExpr * itemExpr = valId.getItemExpr();

      // call the pre-code generator to make sure we handle cases of
      // more difficult type conversion, such as complex types.
      itemExpr->preCodeGen(generator);

      // get the left (probing) and the right (building) column value
      // to be hashed and convert them to a common data type
      ItemExpr * buildHashCol = itemExpr->child(1);
      ItemExpr * probeHashCol = itemExpr->child(0);

      // Search for the expression to be hashed in the build table and
      // make a lookup table that links hash key columns to columns in
      // the hash table. For now, if the expression to be hashed is not
      // yet part of the hash table, then add it. This should only happen
      // for join predicates involving expressions, e.g. t1.a = t2.b + 5.
      // The lookup table and the list rightBufferValIds is later used when
      // moving the building tuple into the hash table and for
      // generating the build search expression.
      NABoolean found = FALSE;
      ValueId buildHashColValId = buildHashCol->getValueId();
      for (CollIndex ix = 0; ix < rightBufferValIds.entries() AND NOT found; ix++) {
	if (rightBufferValIds[ix] == buildHashColValId) {
	  // found the build hash column in the column layout of the
	  // hash table, remember that hash key column
	  // "hashKeyColumns.entries()" can be found in column "ix" of
	  // the hash table.
	  hashKeyColumns.insert(ix);
	  found = TRUE;
	}
      }

      if (NOT found) {
	// hash value is not yet contained in hash table, add it and
	// remember that it is stored in column "rightOutputValIds.entries()"
	hashKeyColumns.insert(rightBufferValIds.entries());
	rightBufferValIds.insert(buildHashColValId);
      }

      // now finish the job by adding type conversion to a common type
      // even for cases that use specialized comparison operators for
      // slightly mismatching types (e.g. the above procedure will not
      // create a type conversion operator for a comparison between a
      // 16 bit integer and a 32 bit integer)
      const NAType & buildType = buildHashCol->getValueId().getType();
      const NAType & probeType = probeHashCol->getValueId().getType();

      if (NOT (buildType == probeType)) {
	// seems like the executor would use a special-purpose
	// comparison operator between two different data types,
	// but this isn't possible for this case where we hash
	// both sides separately

	// find the common datatype that fits both columns
	UInt32 flags =
              ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
               ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);

	const NAType *resultType = buildType.synthesizeType(
							    SYNTH_RULE_UNION,
							    buildType,
							    probeType,
							    generator->wHeap(),
                                                        &flags);
	CMPASSERT(resultType);

	// matchScales() has been called in preCodeGen()

	// add type conversion operators if necessary
	if (NOT (buildType == *resultType)) {
	  buildHashCol = new(generator->wHeap()) Cast(buildHashCol,resultType);
	}
	if (NOT (probeType == *resultType)) {
	  probeHashCol = new(generator->wHeap()) Cast(probeHashCol,resultType);
	}
      }

      NABoolean doCasesensitiveHash = FALSE;

      if (buildType.getTypeQualifier() == NA_CHARACTER_TYPE &&
          probeType.getTypeQualifier() == NA_CHARACTER_TYPE) {

        const CharType &cBuildType = (CharType&)buildType;
        const CharType &cProbeType = (CharType&)probeType;

        if (cBuildType.isCaseinsensitive() != cProbeType.isCaseinsensitive())
           doCasesensitiveHash = TRUE;
      }


      // make the hash function for the right value, to be hashed
      // into the hash table while building the hash table.
      buildHashCol->bindNode(generator->getBindWA());
      BuiltinFunction * hashFunction =
	new(generator->wHeap()) Hash(buildHashCol);
      if (buildHashTree) {
	buildHashTree = new(generator->wHeap()) HashComb(buildHashTree,
							 hashFunction);
      }
      else {
	buildHashTree = hashFunction;
      }

      if (doCasesensitiveHash)
	{
	  ((Hash*)hashFunction)->setCasesensitiveHash(TRUE);
	  
	  ((HashComb*)buildHashTree)->setCasesensitiveHash(TRUE);
	}

      // make the hash function for the left value. This value is to
      // be hashed into the hash table while probing the hash table.
      hashFunction = new(generator->wHeap()) Hash(probeHashCol);
      if (probeHashTree) {
	probeHashTree = new(generator->wHeap()) HashComb(probeHashTree,
							 hashFunction);
      }
      else {
	probeHashTree = hashFunction;
      }

      if (doCasesensitiveHash)
	{
	  ((Hash*)hashFunction)->setCasesensitiveHash(TRUE);
	  
	  ((HashComb*)probeHashTree)->setCasesensitiveHash(TRUE);
	}
    }

    // hash value is an unsigned long. (compare typedef SimpleHashValue in
    // common/BaseType.h). It could be made a bigger datatype,
    // if need be.
    buildHashTree = new (generator->wHeap())
	Cast(buildHashTree, new (generator->wHeap()) SQLInt(generator->wHeap(), FALSE, FALSE));
    probeHashTree = new (generator->wHeap())
	Cast(probeHashTree, new (generator->wHeap()) SQLInt(generator->wHeap(), FALSE, FALSE));
    buildHashTree->setConstFoldingDisabled(TRUE);
    probeHashTree->setConstFoldingDisabled(TRUE);

    // bind/type propagate the hash evaluation tree
    buildHashTree->bindNode(generator->getBindWA());
    probeHashTree->bindNode(generator->getBindWA());

    // add the build root value id to the map table. This is the hash value.
    Attributes * mapAttr;
    mapAttr = (generator->addMapInfo(buildHashTree->getValueId(), 0))->
      getAttr();
    mapAttr->setAtp(workAtpPos);
    mapAttr->setAtpIndex(hashValueAtpIndex);
    ULng32 len;
    ExpTupleDesc::computeOffsets(mapAttr,
				 ExpTupleDesc::SQLARK_EXPLODED_FORMAT, len);

    // add the probe root value id to the map table. This is the hash value.
    mapAttr = (generator->addMapInfo(probeHashTree->getValueId(), 0))->
      getAttr();

    mapAttr->copyLocationAttrs(generator->
			       getMapInfo(buildHashTree->
					  getValueId())->getAttr());

    // generate code to evaluate the hash expression
    expGen->generateArithExpr(buildHashTree->getValueId(),
			      ex_expr::exp_ARITH_EXPR,
			      &rightHashExpr);

    // generate the probe hash expression
    expGen->generateArithExpr(probeHashTree->getValueId(),
			      ex_expr::exp_ARITH_EXPR,
			      &leftHashExpr);
  };

  
  // only the case of single column in the NOT IN is handled.
  // these 2 expression (checkInnerNullExpr_ and checkOuterNullExpr_) 
  // will be empty for the multi-column case 
  // generate the check inner null expression
  ex_expr *checkInnerNullExpression = 0;
  if (!(getCheckInnerNullExpr().isEmpty()))
  {
    ItemExpr * newExprTree = getCheckInnerNullExpr().rebuildExprTree(ITM_AND,TRUE,TRUE); 
    expGen->generateExpr(newExprTree->getValueId(),ex_expr::exp_SCAN_PRED,
			  &checkInnerNullExpression);

  }
    
  // generate the check outer null expression
  ex_expr *checkOuterNullExpression = 0;
   if (!(getCheckOuterNullExpr().isEmpty()))
  {
    ItemExpr * newExprTree = getCheckOuterNullExpr().rebuildExprTree(ITM_AND,TRUE,TRUE); 
    expGen->generateExpr(newExprTree->getValueId(),ex_expr::exp_SCAN_PRED,
			  &checkOuterNullExpression);

  }

  // allocate two map tables, so that we later can remove the left childs map
  // table
  MapTable * myMapTable4 = generator->appendAtEnd();
  MapTable * myMapTable5 = generator->appendAtEnd();
  // MT -> MT0 -> MT1 -> RC MT -> MT2 -> MT3 -> LC MT -> MT4 -> MT5

  //determine the tuple format and whether we want to resize rows or not
  // base the decision on the right side and left side 
  NABoolean bmo_affinity = (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO_AFFINITY) == DF_ON);
  NABoolean resizeCifRecord = FALSE;
  ExpTupleDesc::TupleDataFormat tupleFormat ;
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
     tupleFormat = determineInternalFormat( rightBufferValIds,
                                           leftOutputValIds,
                                           this,
                                           resizeCifRecord,
                                           generator,
                                           bmo_affinity,
                                           considerBufferDefrag,
                                           useUniqueHashJoin);
     considerBufferDefrag = considerBufferDefrag && resizeCifRecord;
  }
  // generate the rightMoveInExpr
  ValueIdList rightMoveTargets1;
  MapTable *rightExtValMapTable = NULL;
  ULng32 rightRowLength = 0;
  if (rightBufferValIds.entries() > 0) {
    // Offsets are based on the row starting after the HashRow structure.
    expGen->generateContiguousMoveExpr(rightBufferValIds,
				       -1, // add convert nodes
				       workAtpPos,
				       extRightRowAtpIndex1,
				       tupleFormat,
				       rightRowLength,
				       &rightMoveInExpr,
				       0, // tuple descriptor
				       ExpTupleDesc::SHORT_FORMAT,
				       &rightExtValMapTable,
				       &rightMoveTargets1);
  };

  ULng32 extRightRowLength = rightRowLength + sizeof(HashRow);

  // MT -> MT0 -> MT1 -> RC MT -> MT2 -> MT3 -> LC MT
  //    -> MT4 -> MT5 -> ERC (new valIds)

  // remove the map table from the right child. We don't need it anymore
  // first un-chain the left child's map table
  generator->unlinkNext(myMapTable2);
  // now delete the right child's map table
  // This will delete MT1 -> RC MT and MT2, so we cannot reference those
  // any more..
  generator->removeAll(myMapTable0);
  // make sure we cannot reference them anymore..
  myMapTable1 = NULL;
  myMapTable2 = NULL;
  // and append the left childs map table again
  generator->appendAtEnd(myMapTable3);
  // MT -> MT0 -> MT3 -> LC MT -> MT4 -> MT5 -> ERC (new valIds)


  // generate leftMoveExpr to move a left child row directly to the
  // parents buffer
  ULng32 leftRowLength = 0;
  if (leftOutputValIds.entries() > 0) {
    expGen->generateContiguousMoveExpr(leftOutputValIds,
				       -1,
				       workAtpPos,
				       leftRowAtpIndex,
				       tupleFormat,
				       leftRowLength,
				       &leftMoveExpr);
    // get rid of the map table which was just appended by the last call
    generator->removeLast();
  };
  // MT -> MT0 -> MT3 -> LC MT -> MT4 -> MT5 -> ERC (new valIds)
  // generate the leftMoveInExpr
  ValueIdList leftMoveTargets;
  MapTable *leftExtValMapTable = NULL;
  if (leftOutputValIds.entries() > 0) {
    // Offsets are based on the row starting after the HashRow structure.
    expGen->generateContiguousMoveExpr(leftOutputValIds,
				       -1, // add convert nodes
				       workAtpPos,
				       extLeftRowAtpIndex,
				       tupleFormat,
				       leftRowLength,
				       &leftMoveInExpr,
				       0,
				       ExpTupleDesc::SHORT_FORMAT,
				       &leftExtValMapTable,
				       &leftMoveTargets);
  };

  ULng32 extLeftRowLength = leftRowLength + sizeof(HashRow);

  // MT -> MT0 -> MT3 -> LC MT -> MT4 -> MT5
  //    -> ERC (new valIds) -> ELC (new valIds)
  // add the map table of the "extended" right row
  if(rightExtValMapTable) {
    generator->appendAtEnd(rightExtValMapTable);
  }
  // MT -> MT0 -> MT3 -> LC MT -> MT4 -> MT5
  //    -> ERC (new valIds) -> ELC (new valIds) -> ERC (old Ids)
  // generate  probeSearchExpr1
  if (! getEquiJoinPredicates().isEmpty()) {
    ItemExpr * newPredTree;
    newPredTree = getEquiJoinPredicates().rebuildExprTree(ITM_AND,TRUE,TRUE);
    expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			  &probeSearchExpr1);
  }

  // generate beforeJoinPred1
  if (! joinPred().isEmpty()) {
    ItemExpr * newPredTree;
    newPredTree = joinPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
    expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			 &beforeJoinPred1);
  }

  // generate afterJoinPred1
  if (! selectionPred().isEmpty()) {
    ItemExpr * newPredTree;
    newPredTree = selectionPred().rebuildExprTree(ITM_AND,TRUE, TRUE);
    expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
                         &afterJoinPred1);
  }

  // MT -> MT0 -> MT3 -> LC MT -> MT4 -> MT5
  //    -> ERC (new valIds) -> ELC (new valIds) -> ERC (old Ids)
  // remove MapTable for left child row. First un-chain the extended
  // right row tupps
  generator->unlinkNext(myMapTable4);
  // now we can savely delete the MapTable for the left child row
  generator->unlinkNext(myMapTable0);
  // add the MapTable for the "extended" right row again
  generator->appendAtEnd(myMapTable5);


  // For unique Hash Join, there is no leftExtValMapTable.
  //
  if(!useUniqueHashJoin) {
    // add the MapTable for the "extended" left row
    // if it exists.  It will not exist for Unique Hash Join
    generator->appendAtEnd(leftExtValMapTable);
  }

  // MT -> MT0 -> MT5 -> ERC (new valIds) -> ELC (new valIds)
  //    -> ERC (old Ids) -> ELC (old Ids)

  if (! getEquiJoinPredicates().isEmpty()) {
    // Generate rightSearchExpr for the build table. This expression
    // compares two "extended" right rows. The MapTable contains
    // only one of these rows (extRightRowAtpIndex1). To get the
    // MapTable and ValueIdList for the second row, we go thru the
    // rightBufferValIds and create new itemexpressions from this list.
    ValueIdList rightMoveTargets2;
    CollIndex i = 0;
    for (i = 0; i < rightBufferValIds.entries(); i++) {
      // create the new item xpression
      ItemExpr * newCol = new(generator->wHeap())
	NATypeToItem((NAType *)&(rightBufferValIds[i].getType()));
      newCol->synthTypeAndValueId();

      // copy the attributes from the first entended row
      Attributes * originalAttr =
	generator->getMapInfo(rightMoveTargets1[i])->getAttr();

      Attributes * newAttr =
	generator->addMapInfo(newCol->getValueId(), 0)->getAttr();
      newAttr->copyLocationAttrs(originalAttr);

      // only atpindex is different
      newAttr->setAtpIndex(extRightRowAtpIndex2);

      // add the new valueId to the list of movetargets
      rightMoveTargets2.insert(newCol->getValueId());
    };

    // MT -> MT0 -> MT5 -> ERC (new valIds) -> ELC (new valIds)
    //    -> ERC (old Ids) -> ELC (old Ids) -> ERC2 (new valIds)
    ValueIdSet searchExpr;

    for (i = 0; i < hashKeyColumns.entries(); i++) {
      // hashKeyColumns[i] remembers which column in the hash table
      // the i-th hash key column is.
      CollIndex hashColNum = hashKeyColumns[i];
      ItemExpr *eqNode =
	new(generator->wHeap()) BiRelat(ITM_EQUAL,
		    rightMoveTargets1[hashColNum].getItemExpr(),
		    rightMoveTargets2[hashColNum].getItemExpr(),
		    // specialNulls == TRUE  means that the right search
		    // expression would treat NULL values as identical (when
		    // a new row is inserted into the hash-table); hence such
		    // rows would be chained.
		    // Note: NULL values in the hash table are treated as
		    // non-identical by the probe search expressions (when
		    // probing with left rows); i.e., the above chain of right
		    // rows with NULLs would never be matched!
		    TRUE);
      eqNode->bindNode(generator->getBindWA());
      // collect all the comparison preds in a value id set
      searchExpr += eqNode->getValueId();
    }

    // AND the individual parts of the search expression together and
    // generate the expression (note that code for the build table columns
    // and for the move targets has been generated before, only the
    // comparison code itself should be generated here)
    ItemExpr * newPredTree = searchExpr.rebuildExprTree(ITM_AND,TRUE,TRUE);
    newPredTree->bindNode(generator->getBindWA());
    expGen->generateExpr(newPredTree->getValueId(),
			 ex_expr::exp_SCAN_PRED,
			 &rightSearchExpr);
  }

  // The Unique Hash Join does not use the probeSearchExpr2 since it
  // does not support Overflow
  //
  if(!useUniqueHashJoin) {
    // generate  probeSearchExpr2
    if (! getEquiJoinPredicates().isEmpty()) {
      ItemExpr * newPredTree;
      newPredTree = getEquiJoinPredicates().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
                           &probeSearchExpr2);
    }
  }

  // generate beforeJoinPred2
  if (! joinPred().isEmpty()) {
    ItemExpr * newPredTree;
    newPredTree = joinPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
    expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			 &beforeJoinPred2);
  }

  // generate afterJoinPred2
  if (! selectionPred().isEmpty()) {
    ItemExpr * newPredTree;
    newPredTree = selectionPred().rebuildExprTree(ITM_AND,TRUE, TRUE);
    expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			 &afterJoinPred2);
  }

  // generate the rightMoveOutExpr
  rightRowLength = 0;
  MapTable * rightValMapTable = NULL;
  Int32 bulkMoveOffset = -2;        // try to generate a bulk move

  if ( rightOutputValIds.entries() > 0  &&  rightOutputNeeded ) {
    ValueIdList *rmo;
    if(tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) {
      // For aligned rows, we cannot return just the prefix of the row.
      // This is because the columns in the row may have beed rearraged and
      // the values may not all be at fixed offsets.
      //
      // So for aligned rows, return the whole right buffer.
      rmo = &rightBufferValIds;
    } else {
      // For exploded rows, we can return just the prefix.
      // So, return just the values that are needed above.
      rmo = &rightOutputValIds;
    }

    expGen->generateContiguousMoveExpr(*rmo,
				       -1, // add convert nodes
				       workAtpPos,
				       rightRowAtpIndex,
				       tupleFormat,
				       rightRowLength,
				       &rightMoveOutExpr,
				       0,
				       ExpTupleDesc::SHORT_FORMAT,
				       &rightValMapTable,
                                       NULL,   // target Value Id List
                                       0,      // start offset
                                       &bulkMoveOffset);
  }

  // generate the leftMoveOutExpr
  MapTable * leftValMapTable = NULL;
  if (leftOutputValIds.entries() > 0) {
    expGen->generateContiguousMoveExpr(leftOutputValIds,
				       -1, // add convert nodes
				       workAtpPos,
				       leftRowAtpIndex,
				       tupleFormat,
				       leftRowLength,
				       &leftMoveOutExpr,
				       0,
				       ExpTupleDesc::SHORT_FORMAT,
				       &leftValMapTable);
  }

  // remove the MapTables describing the extended rows
  if (rightExtValMapTable) {
      // Unlinks leftExtValMapTable
    generator->unlinkNext(rightExtValMapTable);
  } else {
  // If we do not have a rightExtValMapTable, the code below will remove
  // the leftExtValMapTable, and whatever follows, from the main maptable 
  // chain.
  // If we don't do this the removeAll() 4 lines below, will delete the
  // leftExtValMapTable (if it existed) and we need it later on..
     if (leftExtValMapTable) {
       // Note that leftExtValMapTable may now have ERC2 (new valIds) as a child
       // at this point....
       // If rightExtValMapTable, existsd we now are on a separate chain...
       generator->unlinkMe(leftExtValMapTable);
     }
  }
  // At this poin we have something like this..
  // MT -> MT0 -> MT5 -> ERC (new valIds) -> ELC (new valIds)
  //    -> ERC (old Ids) -> ELC (old Ids) -> XX potentialy more nodes here XX
  // and everything from MT5 and onwards will now be deleted!!
  generator->removeAll(myMapTable0);
  myMapTable5 = NULL; // make sure we cannot use the stale data anymore..
  rightExtValMapTable = NULL; // make sure we cannot use the stale data anymore.
  // Here is how the map table list looks like now:
  // MT -> MT0

  ULng32 instRowLength = 0;
  ULng32 instRowForRightJoinLength = 0;
  MapTable *instNullForLeftJoinMapTable = NULL;
  // add MapTable for the right row
  if ( rightOutputNeeded ) {
    generator->appendAtEnd(rightValMapTable);
    // Here is how the map table list looks like now:
    // MT -> MT0 -> RV

    // generate nullInstExpr. instantiateValuesForLeftJoin generates
    // 2 expressions. The first one is to evaluate the right expression
    // and move the result into the null instantiated row. The second one
    // is to initialize the instantiated row with null values.
    // instantiateValuesForLeftJoin also adds info for the instatiated
    // null row to the MapTable.
    if (nullInstantiatedOutput().entries() > 0) {
      instantiateValuesForLeftJoin(generator,
				   workAtpPos, instRowForLeftJoinAtpIndex,
				   &leftJoinExpr, &nullInstForLeftJoinExpr,
				   &instRowLength,
				   &instNullForLeftJoinMapTable,
				   tupleFormat);
    };
      
      // Check point.
      if (isLeftJoin() && !selectionPred().isEmpty())
	{
      MapTable * myMapTableX = generator->appendAtEnd();

      // Add back the left map table temporarily for generating the sel pred.
      // for Phase 2.
      generator->appendAtEnd(myMapTable3);
      // XXX -> MT3 -> LC MT -> MT4 -> MT5 -> ERC (new valIds)

      // generate afterJoinPred3
      ItemExpr * newPredTree;
      newPredTree = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
                           &afterJoinPred3);

      // myMapTable3 may be needed later on, unlink before we remove myMapTableX

      // For those that might use myMapTable3 below, beware that the chain
      // starting with myMapTable3 now may have additional map nodes appended 
      // as a result of the call to generateExpr() above, as compared to what
      // looked like before when we unlinked myMapTable3 from the main chain.
      // At this point the myMapTable3 chain will look something like this:
      // MT3 -> LC MT -> MT4 -> MT5 -> ERC (new valIds) -> XX 
      // where XX represents whatever mapTables got added above 

      generator->unlinkMe(myMapTable3);

      // This is how the check point is made use of.
      generator->removeAll(myMapTableX);

      // For Left Joins (including full joins), generate afterJoinPred4
      // 
	{
	  // Add back the left extended map table temporarily for
	  // generating the selection predicate for Phase 3.

	  generator->appendAtEnd(leftExtValMapTable);

	  // generate afterJoinPred4
	  newPredTree = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
	  expGen->generateExpr(newPredTree->getValueId(),
			       ex_expr::exp_SCAN_PRED,
			       &afterJoinPred4);

	  // This is how the check point is made use of.
	  generator->removeAll(myMapTableX);
          // we just deleted leftExtValMapTable...
          // make sure we don't reference it 
          leftExtValMapTable = NULL;

	}

	generator->removeLast(); // It should actually remove myMapTableX.
      } //if (isLeftJoin() && !selectionPred().isEmpty())

      // set the atp for right row values back to 0.
      // Set the atp_index to the last returned tupp.
      if (rightOutputValIds.entries()) {
	for (CollIndex ix = 0; ix < rightOutputValIds.entries(); ix++) {
	  valId = rightOutputValIds[ix];
	  // do this only, if the valueId is not input to this node
	  if (NOT getGroupAttr()->getCharacteristicInputs().contains(valId)) {
	    MapInfo * map_info = generator->getMapInfoAsIs(valId);
	    if (map_info) {
	      Attributes * attr = map_info->getAttr();
	      attr->setAtp(0);
	      attr->setAtpIndex(returnedRightRowAtpIndex);
	    };
	  };
	};
      };

      // set the atp index for values in the instantiated row.
      if (nullInstantiatedOutput().entries()) {
	for (CollIndex ix = 0; ix < nullInstantiatedOutput().entries(); ix++) {
	  valId = nullInstantiatedOutput()[ix];

	  Attributes * attr = generator->getMapInfo(valId)->getAttr();
	  attr->setAtp(0);
	  attr->setAtpIndex(returnedInstRowAtpIndex);
	}
      };
   }; //  if ( rightOutputNeeded )


  if(useUniqueHashJoin) {
    // Add the MapTable for the left child
    // unique Hash Join passes the left child values as is (copyAtp()).
    //
    generator->appendAtEnd(myMapTable3);
  } else {
    // add the MapTable for the left row
    //
    generator->appendAtEnd(leftValMapTable);
  }

  // Here is how the map table list looks like now:
  // MT -> MT0 -> RV -> LV

  /***************** Generate the nullInstForRightJoinExprs *************/

  // generate nullInstForRightJoinExpr. instantiateValuesForRightJoin
  // generates 2 expressions. The first one is to evaluate the right
  // expression and move the result into the null instantiated row.
  // The second one is to initialize the instantiated row with null values.
  // instantiateValuesForRightJoin also adds info for the instatiated
  // null row to the MapTable.

  if (nullInstantiatedForRightJoinOutput().entries() > 0) {
    instantiateValuesForRightJoin(generator,
				  workAtpPos, instRowForRightJoinAtpIndex,
				  &rightJoinExpr, &nullInstForRightJoinExpr,
				  &instRowForRightJoinLength,
				  NULL, // Don't need a MapTable back. At
				       // this point, we have generated all
				       // the necessary expressions. This
				       // code is here to be consistent with the
                                       // this one's counterpart
				       // - instantiateValuesForLeftJoin
				  tupleFormat);
  }  // nullInstantiatedForRightJoinOutput()

  if (isRightJoin()&& !selectionPred().isEmpty())
    {
      // Use the check point technique that is used in
      // generating the afterJoinPred3 & afterJoinPred4
      // for isLeftJoin()
      MapTable * myMapTableX = generator->appendAtEnd();

      // add the null instantitated columns maptable back
      // to generate the after join selection predicted.
      // For this expression, the values should all be
      // available at instNullForLeftJoinMapTable and
      // instRowForLeftJoinAtpIndex
      generator->appendAtEnd(instNullForLeftJoinMapTable);

      // generate afterJoinPred5
      // We only need one predicate here, since the nullInstantiated
      // versions of both the left row and the right row are
      // available in their respective nullInstantiated tupp.
      // Note that the left join case will needs both afterJoinPred3 and
      // afterJoinPred4.
      ItemExpr * newPredTree;
      newPredTree = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
                           &afterJoinPred5);

      // This is how the check point is made use of.
      generator->removeAll(myMapTableX);
      
      // We just deleted instNullForLeftJoinMapTable, make sure we don't use
      // it any more..
      instNullForLeftJoinMapTable = NULL;

      generator->removeLast(); // It should actually remove myMapTableX.
    }

      // set the atp for the left row values back to 0
      if (leftOutputValIds.entries()) {
	for (CollIndex ix = 0; ix < leftOutputValIds.entries(); ix++) {
	  valId = leftOutputValIds[ix];

	  MapInfo * map_info =
	generator->getMapInfoFromThis(leftValMapTable, valId);
      if (map_info) {
	Attributes * attr = map_info->getAttr();
	attr->setAtp(0);
	attr->setAtpIndex(returnedLeftRowAtpIndex);
      };
    };
  };

  // set the atp index for values in the instantiated row.
  if (nullInstantiatedForRightJoinOutput().entries()) {
    for (CollIndex ix = 0; ix < nullInstantiatedForRightJoinOutput().entries(); ix++) {
      valId = nullInstantiatedForRightJoinOutput()[ix];

      Attributes * attr = generator->getMapInfo(valId)->getAttr();
      attr->setAtp(0);
      attr->setAtpIndex(returnedInstRowForRightJoinAtpIndex);
    }
  };

  // determine the expected size of the inner table
  Cardinality innerExpectedRows = (Cardinality) child(1)->getGroupAttr()->
    getOutputLogPropList()[0]->getResultCardinality().value();

  // If this HJ is performed within ESPs, then number of records
  // processed by each ESP is a subset of total records.
  // Inner side -- divide only for type 1 join !! (type 2 join sends all the
  // rows to each ESP, and for the rest we assume the same as a worst case).
  if ( saveNumEsps > 0 && getParallelJoinType() == 1 )
    innerExpectedRows /= (Cardinality) saveNumEsps ;

  // determine the expected size of the outer table
  Cardinality outerExpectedRows = (Cardinality) child(0)->getGroupAttr()->
      getOutputLogPropList()[0]->getResultCardinality().value();

  // If this HJ is performed within ESPs, then number of records
  // processed by each ESP is a subset of total records.
  if ( saveNumEsps > 0 )
    outerExpectedRows /= (Cardinality) saveNumEsps ;

  // determine the size of the HJ buffers. This hash buffer is used to store
  // the incoming (inner or outer) rows, and may be written to disk (overflow)

  // first determine the minimum size for the hash table buffers.
  // a buffer has to store at least one extended inner or outer row
  // plus overhead such as hash buffer header etc
  ULng32 minHBufferSize = MAXOF(extLeftRowLength, extRightRowLength) +
			  ROUND8(sizeof(HashBufferHeader)) + 8;

  // determine the minimum result sql buffer size (may need to store result
  // rows comprising of both inner and outer incoming rows)
  ULng32 minResBufferSize = leftRowLength + sizeof(tupp_descriptor);
  if ( rightOutputNeeded )
    minResBufferSize += rightRowLength + sizeof(tupp_descriptor);

  // get the default value for the (either hash or result) buffer size
  ULng32 bufferSize = (ULng32) getDefault(GEN_HSHJ_BUFFER_SIZE);

  // currently the default hash buffer size is 56K (DP2 can not take anything
  // larger), so if this Hash-Join may overflow, and the input row exceeds
  // this size, we issue an error. If overflow is not allowed, then we may
  // resize the hash buffer to accomodate the larger input row.
  ULng32 hashBufferSize = bufferSize ;
  if ( minHBufferSize > bufferSize ) {
    // On linux we can handle any size of overflow buffer
    hashBufferSize = minHBufferSize ; // use a larger Hash-Buffer    
  }

  // adjust up the result buffer size, if needed
  bufferSize = MAXOF(minResBufferSize, bufferSize);

  // determione the memory usage (amount of memory as percentage from total
  // physical memory used to initialize data structures)
  unsigned short memUsagePercent =
    (unsigned short) getDefault(BMO_MEMORY_USAGE_PERCENT);

  // determine the size of the up queue. We should be able to keep at least
  // result buffer worth od data in the up queue
  queue_index upQueueSize = (queue_index)(bufferSize / minResBufferSize);
  // we want at least 4 entries in the up queue
  upQueueSize = MAXOF(upQueueSize,(queue_index) 4);
  // the default entry might be even larger
  upQueueSize = MAXOF(upQueueSize, (queue_index)getDefault(GEN_HSHJ_SIZE_UP));

  // Support for a RE-USE of the hash table, in case the input values for
  // the right/inner child are the same as in the previous input.
  ex_expr * moveInputExpr = 0;
  ex_expr * checkInputPred = 0;
  ULng32 inputValuesLen = 0;

  if ( isReuse() ) { // only if testing is needed
    // Create a copy of the input values. This set represent the saved input
    // values which are compared to the incoming values.

    // Generate expression to move the relevant input values
    if (! moveInputValues().isEmpty() ) {
      ValueIdList vid_list( moveInputValues() );
      expGen->generateContiguousMoveExpr(vid_list,
					 0, // dont add conv nodes
					 workAtpPos,
					 prevInputTuppIndex,
					 tupleFormat,
					 inputValuesLen, &moveInputExpr);
    }

    // generate expression to see if the relevant input values have changed.
    // If changed, then we need to rebuild the hash table.
    if (! checkInputValues().isEmpty() ) {
      ItemExpr * newPredTree =
	checkInputValues().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &checkInputPred);
    }
  }


  if(useUniqueHashJoin) {
    // The unique hash join does not use the rightMoveOutExpr,
    // however, it was generated to get the mapTable (could use
    // processAttributes()).
    // Set it to NULL here.
    //
    rightMoveOutExpr = NULL;
    rightRowAtpIndex = -1;

    // The Unique Hash Join does not have an extended left row.
    // However, extLeftRowLength was set to a non-zero value above.
    //
    extLeftRowLength = 0;

    // Check to make sure things are as we expect for the unique hash join
    //
    GenAssert(!rightMoveOutExpr, "Bad Unique Hash Join: rightMoveOutExpr");
    GenAssert(!leftMoveExpr, "Bad Unique Hash Join: leftMoveExpr");
    GenAssert(!leftMoveInExpr, "Bad Unique Hash Join: leftMoveInExpr");
    GenAssert(!leftMoveOutExpr, "Bad Unique Hash Join: leftMoveOutExpr");
    GenAssert(!probeSearchExpr2, "Bad Unique Hash Join: probeSearchExpr2");
    GenAssert(!leftJoinExpr, "Bad Unique Hash Join: leftJoinExpr");
    GenAssert(!nullInstForLeftJoinExpr,
              "Bad Unique Hash Join: nullInstForLeftJoinExpr");
    GenAssert(!beforeJoinPred1, "Bad Unique Hash Join: beforeJoinPred1");
    GenAssert(!beforeJoinPred2, "Bad Unique Hash Join: beforeJoinPred2");
    GenAssert(!afterJoinPred1, "Bad Unique Hash Join: afterJoinPred1");
    GenAssert(!afterJoinPred2, "Bad Unique Hash Join: afterJoinPred2");
    GenAssert(!afterJoinPred3, "Bad Unique Hash Join: afterJoinPred3");
    GenAssert(!afterJoinPred4, "Bad Unique Hash Join: afterJoinPred4");
    GenAssert(leftRowLength == 0, "Bad Unique Hash Join: leftRowLength");
    GenAssert(extLeftRowLength == 0, "Bad Unique Hash Join: extLeftRowLength");
    GenAssert(instRowLength == 0, "Bad Unique Hash Join: instRowLength");
    GenAssert(leftRowAtpIndex == -1, "Bad Unique Hash Join: leftRowAtpIndex");
    GenAssert(rightRowAtpIndex == -1, "Bad Unique Hash Join: rightRowAtpIndex");
    GenAssert(instRowForLeftJoinAtpIndex == -1,
              "Bad Unique Hash Join: instRowForLeftJoinAtpIndex");
    GenAssert(returnedLeftRowAtpIndex == -1,
              "Bad Unique Hash Join: returnedLeftRowAtpIndex");
    GenAssert(returnedInstRowAtpIndex == -1,
              "Bad Unique Hash Join: returnedInstRowAtpIndex");
    GenAssert(!rightJoinExpr, "Bad Unique Hash Join: rightJoinExpr");
    GenAssert(!nullInstForRightJoinExpr,
              "Bad Unique Hash Join: nullInstForRightJoinExpr");
    GenAssert(instRowForRightJoinAtpIndex == -1,
              "Bad Unique Hash Join: instRowForRightJoinAtpIndex");
    GenAssert(returnedInstRowForRightJoinAtpIndex == -1,
              "Bad Unique Hash Join: returnedInstRowForRightJoinAtpIndex");
    GenAssert(instRowForRightJoinLength == 0,
              "Bad Unique Hash Join: instRowForRightJoinLength");
  }


   short scrthreshold = (short) CmpCommon::getDefaultLong(SCRATCH_FREESPACE_THRESHOLD_PERCENT);
   short hjGrowthPercent = 
     getGroupAttr()->getOutputLogPropList()[0]->getBMOgrowthPercent();
  // now we have generated all the required expressions and the MapTable
  // reflects the returned rows. Let's generate the hash join TDB now
  ComTdbHashj * hashj_tdb =
    new(space) ComTdbHashj(leftChildTdb,
			   rightChildTdb,
			   givenDesc,
			   returnedDesc,
			   rightHashExpr,
			   rightMoveInExpr,
			   rightMoveOutExpr,
			   rightSearchExpr,
			   leftHashExpr,
			   leftMoveExpr,
			   leftMoveInExpr,
			   leftMoveOutExpr,
			   probeSearchExpr1,
			   probeSearchExpr2,
			   leftJoinExpr,
			   nullInstForLeftJoinExpr,
			   beforeJoinPred1,
			   beforeJoinPred2,
			   afterJoinPred1,
			   afterJoinPred2,
			   afterJoinPred3,
			   afterJoinPred4,
			   afterJoinPred5,
			   checkInputPred,
			   moveInputExpr,
			   (Lng32)inputValuesLen,
			   prevInputTuppIndex,
			   rightRowLength,
			   extRightRowLength,
			   leftRowLength,
			   extLeftRowLength,
			   instRowLength,
			   workCriDesc,
			   leftRowAtpIndex,
			   extLeftRowAtpIndex,
			   rightRowAtpIndex,
			   extRightRowAtpIndex1,
			   extRightRowAtpIndex2,
			   hashValueAtpIndex,
			   instRowForLeftJoinAtpIndex,
			   returnedLeftRowAtpIndex,
			   returnedRightRowAtpIndex,
			   returnedInstRowAtpIndex,
			   memUsagePercent,
			   (short)getDefault(GEN_MEM_PRESSURE_THRESHOLD),
                           scrthreshold,
			   (queue_index)getDefault(GEN_HSHJ_SIZE_DOWN),
			   upQueueSize,
			   isSemiJoin(),
			   isLeftJoin(),
			   isAntiSemiJoin(),
                           useUniqueHashJoin,
			   (isNoOverflow() ||
			    (CmpCommon::getDefault(EXE_BMO_DISABLE_OVERFLOW)
			     == DF_ON)),
			   isReuse(),
			   (Lng32)getDefault(GEN_HSHJ_NUM_BUFFERS),
			   bufferSize,
			   hashBufferSize,
			   (Cardinality) getGroupAttr()->
			   getOutputLogPropList()[0]->
			   getResultCardinality().value(),
			   innerExpectedRows,
			   outerExpectedRows,
			   isRightJoin(),
			   rightJoinExpr,
			   nullInstForRightJoinExpr,
			   instRowForRightJoinAtpIndex,
			   returnedInstRowForRightJoinAtpIndex,
			   instRowForRightJoinLength,
			   // To get the min number of buffers per a flushed
			   // cluster before is can be flushed again
			   (unsigned short)
			   getDefault(EXE_NUM_CONCURRENT_SCRATCH_IOS)
			     + (short)getDefault(COMP_INT_66), // for testing
			   (UInt32) getDefault(COMP_INT_67), // for batch num
			     //+ (short)getDefault(COMP_INT_66), // for testing
			   checkInnerNullExpression,
                           checkOuterNullExpression,
                           hjGrowthPercent,

                           // For min/max optimization.
			   minMaxValsAtpIndex,
			   minMaxRowLength,
			   minMaxExpr,
			   leftDownDesc
			   );

  generator->initTdbFields(hashj_tdb);
  hashj_tdb->setOverflowMode(generator->getOverflowMode());
  if (CmpCommon::getDefault(EXE_BMO_SET_BUFFERED_WRITES) == DF_ON)
    hashj_tdb->setBufferedWrites(TRUE);
  if (CmpCommon::getDefault(EXE_DIAGNOSTIC_EVENTS) == DF_ON)
    hashj_tdb->setLogDiagnostics(TRUE);
  if (useUniqueHashJoin || // UHJ avoids check for early overflow
      CmpCommon::getDefault(EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW_HASH) == DF_ON
       // If CQD value is SYSTEM, then no compiler hints checks for HDD
      || 
      (((generator->getOverflowMode() == ComTdb::OFM_DISK ) ||
       ( generator->getOverflowMode() == ComTdb::OFM_MMAP))
       && 
      CmpCommon::getDefault(EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW_HASH) == 
      DF_SYSTEM ))
    hashj_tdb->setDisableCmpHintsOverflow(TRUE);
  hashj_tdb->setBmoMinMemBeforePressureCheck((Int16)getDefault(EXE_BMO_MIN_SIZE_BEFORE_PRESSURE_CHECK_IN_MB));
  
  if(generator->getOverflowMode() == ComTdb::OFM_SSD )
    hashj_tdb->setBMOMaxMemThresholdMB((UInt16)(ActiveSchemaDB()->
				   getDefaults()).
			  getAsLong(SSD_BMO_MAX_MEM_THRESHOLD_IN_MB));
  else
    hashj_tdb->setBMOMaxMemThresholdMB((UInt16)(ActiveSchemaDB()->
				   getDefaults()).
			  getAsLong(EXE_MEMORY_AVAILABLE_IN_MB));

  hashj_tdb->setScratchIOVectorSize((Int16)getDefault(SCRATCH_IO_VECTOR_SIZE_HASH));
  hashj_tdb->
    setForceOverflowEvery((UInt16)(ActiveSchemaDB()->
				   getDefaults()).
			  getAsULong(EXE_TEST_HASH_FORCE_OVERFLOW_EVERY));
  
  hashj_tdb->
    setForceHashLoopAfterNumBuffers((UInt16)(ActiveSchemaDB()->
					     getDefaults()).
		    getAsULong(EXE_TEST_FORCE_HASH_LOOP_AFTER_NUM_BUFFERS));

  hashj_tdb->
    setForceClusterSplitAfterMB((UInt16) (ActiveSchemaDB()->getDefaults()).
		    getAsULong(EXE_TEST_FORCE_CLUSTER_SPLIT_AFTER_MB));

  ((ComTdb*)hashj_tdb)->setCIFON((tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT));
				    
  // The CQD EXE_MEM_LIMIT_PER_BMO_IN_MB has precedence over the mem quota sys
  NADefaults &defs            = ActiveSchemaDB()->getDefaults();

  UInt16 mmu = (UInt16)(defs.getAsDouble(EXE_MEM_LIMIT_PER_BMO_IN_MB));

  UInt16 numBMOsInFrag = (UInt16)generator->getFragmentDir()->getNumBMOs();
        
  double memQuota = 0;
  double memQuotaRatio;
  Lng32 numStreams;
  double bmoMemoryUsagePerNode = getEstimatedRunTimeMemoryUsage(TRUE, &numStreams).value();
  if (mmu != 0) {
    memQuota = mmu;
    hashj_tdb->setMemoryQuotaMB(mmu);
  } else {
    // Apply quota system if either one the following two is true:
    //   1. the memory limit feature is turned off and more than one BMOs 
    //   2. the memory limit feature is turned on
    NABoolean mlimitPerNode = defs.getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) > 0;
    
    if ( mlimitPerNode || numBMOsInFrag > 1 ||
         (numBMOsInFrag == 1 && CmpCommon::getDefault(EXE_SINGLE_BMO_QUOTA) == DF_ON)) {
        memQuota = 
           computeMemoryQuota(generator->getEspLevel() == 0,
                              mlimitPerNode,
                              generator->getBMOsMemoryLimitPerNode().value(),
                              generator->getTotalNumBMOs(),
                              generator->getTotalBMOsMemoryPerNode().value(),
                              numBMOsInFrag, 
                              bmoMemoryUsagePerNode,
                              numStreams,
                              memQuotaRatio
                             );
    }                                
    Lng32 hjMemoryLowbound = defs.getAsLong(BMO_MEMORY_LIMIT_LOWER_BOUND_HASHJOIN);
    Lng32 memoryUpperbound = defs.getAsLong(BMO_MEMORY_LIMIT_UPPER_BOUND);

    if ( memQuota < hjMemoryLowbound ) {
       memQuota = hjMemoryLowbound;
       memQuotaRatio = BMOQuotaRatio::MIN_QUOTA;
    }
    else if (memQuota >  memoryUpperbound)
       memQuota = memoryUpperbound;
       memQuotaRatio = BMOQuotaRatio::MIN_QUOTA;
    hashj_tdb->setMemoryQuotaMB( UInt16(memQuota) );
    hashj_tdb->setBmoQuotaRatio(memQuotaRatio);
  }

  if (beforeJoinPredOnOuterOnly())
    hashj_tdb->setBeforePredOnOuterOnly();

  generator->addToTotalOverflowMemory(
                      getEstimatedRunTimeOverflowSize(memQuota)
                                     );

  double hjMemEst = getEstimatedRunTimeMemoryUsage(hashj_tdb);
  hashj_tdb->setEstimatedMemoryUsage(hjMemEst / 1024);
  generator->addToTotalEstimatedMemory(hjMemEst);

  if ( generator->getRightSideOfFlow() )
    hashj_tdb->setPossibleMultipleCalls(TRUE);

  // Internal CQD -- if set, enforce a minimum number of clusters
  UInt16 nc =
    (UInt16)(ActiveSchemaDB()->
	     getDefaults()).getAsDouble(EXE_HJ_MIN_NUM_CLUSTERS);
  if (nc != 0)
    hashj_tdb->setNumClusters(nc);

  hashj_tdb->setMemoryContingencyMB(getDefault(PHY_MEM_CONTINGENCY_MB));
  float bmoCtzFactor;
  defs.getFloat(BMO_CITIZENSHIP_FACTOR, bmoCtzFactor);
  hashj_tdb->setBmoCitizenshipFactor((Float32)bmoCtzFactor);


  // For now, use variable for all CIF rows based on resizeCifRecord
  if(resizeCifRecord){ //tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) {
    hashj_tdb->setUseVariableLength();
    if(considerBufferDefrag)
    {
      hashj_tdb->setConsiderBufferDefrag();
    }
  }

  hashj_tdb->setHjMemEstInKBPerNode(bmoMemoryUsagePerNode / 1024);
  if (!generator->explainDisabled()) {

    generator->setExplainTuple(
       addExplainInfo(hashj_tdb, leftExplainTuple, rightExplainTuple, generator));

  }

  hashj_tdb->setReturnRightOrdered( returnRightOrdered() );

  // Only for anti-semi-join with no search expr
  //  Make a guess about the likelihood of any row from the right, in which case
  //  delay requesting left rows, that are probably then not needed.
  hashj_tdb->setDelayLeftRequest( innerExpectedRows > 100 ||
				  outerExpectedRows > 100000 );

  // If using the min/max optimization, must delay the left request.
  // This is because we send the min/max values with the left request
  // and they are not available until phase1 is complete.
  if(minMaxExpr)
    hashj_tdb->setDelayLeftRequest(true);

  // Query limits.
  if ((afterJoinPred1 != NULL) || (afterJoinPred2 != NULL))
  {
    ULng32 joinedRowsBeforePreempt = 
        (ULng32)getDefault(QUERY_LIMIT_SQL_PROCESS_CPU_XPROD);

    if ((joinedRowsBeforePreempt > 0))
      hashj_tdb->setXproductPreemptMax(joinedRowsBeforePreempt);
  }

  // if an Insert/Update/Delete operation exists below the left
  // child, we need to turn off a hash join optimization which
  // cancels the left side if inner table is empty
  if( child(0)->seenIUD() )
  {
    hashj_tdb->setLeftSideIUD();
  }  


  // restore the original down cri desc since this node changed it.
  generator->setCriDesc(givenDesc, Generator::DOWN);

  // set the new up cri desc.
  generator->setCriDesc(returnedDesc, Generator::UP);

  generator->setGenObj(this, hashj_tdb);

  // reset the expression generation flag to generate float validation pcode
  generator->setGenNoFloatValidatePCode(FALSE);

  // reset the handleIndirectVC flag to its initial value
  expGen->setHandleIndirectVC( vcflag );

  return 0;

}


ExpTupleDesc::TupleDataFormat HashJoin::determineInternalFormat( const ValueIdList & rightList,
                                                                 const ValueIdList & leftList,
                                                                 RelExpr * relExpr,
                                                                 NABoolean & resizeCifRecord,
                                                                 Generator * generator,
                                                                 NABoolean bmo_affinity,
                                                                 NABoolean & considerBufferDefrag,
                                                                 NABoolean uniqueHJ)
{
  RelExpr::CifUseOptions bmo_cif = RelExpr::CIF_SYSTEM;

   considerBufferDefrag = FALSE;

   if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO) == DF_OFF)
   {
     bmo_cif = RelExpr::CIF_OFF;
     resizeCifRecord = FALSE;
     return ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
   }

   UInt32 maxRowSize = 0;
   //determine whether we want to defragment the buffers or not based on the average row size 
   double ratio = CmpCommon::getDefaultNumeric(COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO);
   double avgRowSize = getGroupAttr()->getAverageVarcharSize(rightList, maxRowSize);
   considerBufferDefrag = ( maxRowSize >0 &&  avgRowSize/maxRowSize < ratio);

   if (!uniqueHJ)
   {
     avgRowSize = getGroupAttr()->getAverageVarcharSize(leftList, maxRowSize);
     considerBufferDefrag = considerBufferDefrag && ( maxRowSize >0 &&  avgRowSize/maxRowSize < ratio);
   }

   if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO) == DF_ON)
   {
     bmo_cif = RelExpr::CIF_ON;
     resizeCifRecord = (rightList.hasVarChars() || leftList.hasVarChars());
     return ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
   }


  //CIF_SYSTEM


  if (bmo_affinity == TRUE)
  {
    if (generator->getInternalFormat() == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
    {
      resizeCifRecord = (rightList.hasVarChars() || leftList.hasVarChars());
      return  ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
    }
    else
    {
      CMPASSERT(generator->getInternalFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT);
      resizeCifRecord = FALSE;
      return  ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
      
    }
  }

  ExpTupleDesc::TupleDataFormat lTupleFormat = generator->getInternalFormat();
  UInt32 lAlignedHeaderSize= 0;
  UInt32 lAlignedVarCharSize = 0;
  UInt32 lExplodedLength = 0;
  UInt32 lAlignedLength = 0;
  double lAvgVarCharUsage = 1;
  NABoolean lResizeRecord = FALSE;

  ExpTupleDesc::TupleDataFormat rTupleFormat = generator->getInternalFormat();
  UInt32 rAlignedHeaderSize= 0;
  UInt32 rAlignedVarCharSize = 0;
  UInt32 rExplodedLength = 0;
  UInt32 rAlignedLength = 0;
  double rAvgVarCharUsage = 1;
  NABoolean rResizeRecord = FALSE;

  rTupleFormat = generator->determineInternalFormat(rightList,
                                                    relExpr,
                                                    rResizeRecord,
                                                    bmo_cif,
                                                    bmo_affinity,
                                                    rAlignedLength,
                                                    rExplodedLength,
                                                    rAlignedVarCharSize,
                                                    rAlignedHeaderSize,
                                                    rAvgVarCharUsage);

  lTupleFormat = generator->determineInternalFormat(leftList,
                                                    relExpr,
                                                    lResizeRecord,
                                                    bmo_cif,
                                                    bmo_affinity,
                                                    lAlignedLength,
                                                    lExplodedLength,
                                                    lAlignedVarCharSize,
                                                    lAlignedHeaderSize,
                                                    lAvgVarCharUsage);


  if (rTupleFormat == ExpTupleDesc::SQLARK_EXPLODED_FORMAT &&
      lTupleFormat == ExpTupleDesc::SQLARK_EXPLODED_FORMAT)
  {
    resizeCifRecord = FALSE;
    return ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  }

  UInt32 rAlignedNonVarSize = rAlignedLength - rAlignedVarCharSize;
  UInt32 lAlignedNonVarSize = lAlignedLength - lAlignedVarCharSize;

  if (rTupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT &&
      lTupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
  {
    resizeCifRecord = (rResizeRecord || lResizeRecord);
    return ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  }
  //at this point one is aligned the other is exploded
  double cifRowSizeAdj = CmpCommon::getDefaultNumeric(COMPRESSED_INTERNAL_FORMAT_ROW_SIZE_ADJ);
  double lEstRowCount = 1;
  double rEstRowCount = 1;

  lEstRowCount = child(0)->getGroupAttr()->getResultCardinalityForEmptyInput().value();
  rEstRowCount = child(1)->getGroupAttr()->getResultCardinalityForEmptyInput().value();



  if ( (rAlignedVarCharSize > rAlignedHeaderSize ||
       lAlignedVarCharSize > lAlignedHeaderSize) &&

       (((rAlignedNonVarSize + rAvgVarCharUsage * rAlignedVarCharSize ) * rEstRowCount +
       (lAlignedNonVarSize + lAvgVarCharUsage * lAlignedVarCharSize ) * lEstRowCount) <
       (rExplodedLength * rEstRowCount + lExplodedLength * lEstRowCount) *  cifRowSizeAdj))
  {
    resizeCifRecord = (rResizeRecord || lResizeRecord);
    return ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  }

  resizeCifRecord = FALSE;
  return ExpTupleDesc::SQLARK_EXPLODED_FORMAT;

}


CostScalar HashJoin::getEstimatedRunTimeMemoryUsage(NABoolean perNode, Lng32 *numStreams)
{
  GroupAttributes * childGroupAttr = child(1).getGroupAttr();
  const CostScalar childRecordSize = childGroupAttr->getCharacteristicOutputs().getRowLength();
  const CostScalar childRowCount = child(1).getPtr()->getEstRowsUsed();
  // Each record also uses a header (HashRow) in memory (8 bytes for 32bit).
  // Hash tables also take memory -- they are about %50 longer than the 
  // number of entries.
  const ULng32 
    memOverheadPerRecord = sizeof(HashRow) + sizeof(HashTableHeader) * 3 / 2 ;

  CostScalar totalHashTableMemory = 
    childRowCount * (childRecordSize + memOverheadPerRecord);
  // one buffer for the outer table
  totalHashTableMemory += ActiveSchemaDB()->getDefaults().getAsLong(GEN_HSHJ_BUFFER_SIZE);

  const PhysicalProperty* const phyProp = getPhysicalProperty() ;
  Lng32 numOfStreams = 1;
  PartitioningFunction * partFunc = NULL;
  if (phyProp)
  {
    partFunc = phyProp -> getPartitioningFunction() ;
    numOfStreams = partFunc->getCountOfPartitions();
    if (numOfStreams <= 0)
       numOfStreams = 1;
    if ( partFunc -> isAReplicationPartitioningFunction() == TRUE ) 
      totalHashTableMemory *= numOfStreams;
  }
  if (numStreams != NULL)
     *numStreams = numOfStreams;
  if (perNode) 
     totalHashTableMemory /= MINOF(MAXOF(((NAClusterInfoLinux*)gpClusterInfo)->getTotalNumberOfCPUs(), 1), numOfStreams);
  else
     totalHashTableMemory /= numOfStreams;
  return totalHashTableMemory;
}

double HashJoin::getEstimatedRunTimeMemoryUsage(ComTdb * tdb)
{
  Lng32 numOfStreams = 1;
  CostScalar totalHashTableMemory = getEstimatedRunTimeMemoryUsage(FALSE, &numOfStreams);
  totalHashTableMemory *= numOfStreams ;
  return totalHashTableMemory.value();
}

double HashJoin::getEstimatedRunTimeOverflowSize(double memoryQuotaMB)
{
  // Setup overflow size for join with formula ov = ((s0-m)/s0)*(s0+s1), where
  // s0 = size of child0, s1 = size of child1 and m the memory quota for NJ
  //
  if ( memoryQuotaMB > 0 ) {

     GroupAttributes * c0 = child(0).getGroupAttr();
     double c0RLen = c0->getCharacteristicOutputs().getRowLength();
     double c0Rows = (child(0).getPtr()->getEstRowsUsed()).getValue();

     GroupAttributes * c1 = child(1).getGroupAttr();
     double c1RLen = c1->getCharacteristicOutputs().getRowLength();
     double c1Rows = (child(1).getPtr()->getEstRowsUsed()).getValue();

     double s0 = c0RLen * c0Rows;
     double s1 = c1RLen * c1Rows;

     Lng32 pipelines = 1;
     const PhysicalProperty* const phyProp = getPhysicalProperty() ;
     if (phyProp)
     {
       PartitioningFunction * partFunc = phyProp -> getPartitioningFunction() ;
       if ( partFunc )
          pipelines = partFunc->getCountOfPartitions();
     }

     double delta = s1 / pipelines - memoryQuotaMB * COM_ONE_MEG ;
     if ( delta > 0 ) {
       double ov = ((delta / s1) * (s0 + s1)) * pipelines;
       return ov;
     }

  } 
   
  return 0;
}

// NABoolean HashJoin::canUseUniqueHashJoin()
// Decide if this join can use the Unique Hash Join option.  This
// option can be significantly faster than the regular hash join, but
// does not support many features.  First, the unique hash join does
// not support overflow, so we must ensure that the inner side can fit
// int memory.  The Unique Hash Join does not support:
//   - Overflow
//   - Outer joins (left, right or full)
//   - selection or join predicates
//   - anti semi join
//
// The Unique Hash Join only supports
//  - unique joins (at most one row per probe, exception semi joins)
//    Note that the method rowsFromLeftHaveUniqueMatch() used below
//    will return TRUE for Semi-Joins regardless of the uniqueness of
//    the join keys
//
//  - joins with equi join predicates (cross products are not supported)
//
// Semi joins are supported by the Unique Hash Join even if the inner
// table contains duplicates.  It actually does this naturally with no
// special code for semi joins. This works because the unique hash
// join implementation expects to find only one match and will not
// look for additional matches after finding the first.  This is the
// exact behavior required by the semi join.  The Unique Hash Join
// could elimiate these duplicates in the build phase, but does not
// currently do this.
//
NABoolean HashJoin::canUseUniqueHashJoin()
{

  // Do not use Unique Hash Join if it is turned OFF
  if(CmpCommon::getDefault(UNIQUE_HASH_JOINS) == DF_OFF)
    {
      return FALSE;
    }


  if(!isLeftJoin() &&
     !isRightJoin() &&
     joinPred().isEmpty() &&
     selectionPred().isEmpty() &&
     !isAntiSemiJoin() &&
     rowsFromLeftHaveUniqueMatch() &&
     !getEquiJoinPredicates().isEmpty() &&
     (CmpCommon::context()->internalCompile() != CmpContext::INTERNAL_MODULENAME) &&
     !(CmpCommon::statement()->isSMDRecompile())
     )
    {
      // If Unique Hash Joins are turned ON, use for all that qualify
      // regardless of cardinalities.
      //
      if(CmpCommon::getDefault(UNIQUE_HASH_JOINS) == DF_ON)
        {
          return TRUE;
        }

      // Otherwise, for UNIQUE_HASH_JOINS == 'SYSTEM', decide based on
      // cardinalities.

      // Make sure Inner side of join is suitable for Unique Hash Join.
      //
      GroupAttributes *rightChildGA = child(1)->getGroupAttr();

      if(rightChildGA->getGroupAnalysis())
        {
          const CANodeIdSet &nodes =
            rightChildGA->getGroupAnalysis()->getAllSubtreeTables();

          UInt32 innerTables =
            (UInt32) getDefault(UNIQUE_HASH_JOIN_MAX_INNER_TABLES);

          // Default is 1GB
          UInt32 innerTableSizeLimitInMB =
            (UInt32) getDefault(UNIQUE_HASH_JOIN_MAX_INNER_SIZE);

          // Default is 100MB
          UInt32 innerTableSizePerInstanceLimitInMB =
            (UInt32) getDefault(UNIQUE_HASH_JOIN_MAX_INNER_SIZE_PER_INSTANCE);

          RowSize rowSize = rightChildGA->getRecordLength();

          // The extended size
          rowSize += sizeof(HashRow);

          // The hash table entries are rounded up.
          // So do the same here.
          //
          rowSize = ROUND8(rowSize);

          Lng32 numPartitions = 1;
          if(getParallelJoinType() == 1)
            {
              const PhysicalProperty* physProp = child(1)->getPhysicalProperty();
              PartitioningFunction *partFunc = physProp->getPartitioningFunction();

              Lng32 numPartitions =  partFunc->getCountOfPartitions();
            }

          if(nodes.entries() == 1)
            {
              TableAnalysis *tabAnalysis =
                nodes.getFirst().getNodeAnalysis()->getTableAnalysis();

              if(tabAnalysis)
                {
                  CostScalar numRows = tabAnalysis->getCardinalityOfBaseTable();
                  CostScalar estRows = child(1)->getEstRowsUsed();

                  if(numRows >= estRows)
                    {
                      CostScalar innerTableSize = numRows * rowSize;
                      CostScalar innerTableSizePerInstance = innerTableSize /
                        numPartitions;

                      // Convert to MBs
                      innerTableSize /= (1024 * 1024);

                      innerTableSizePerInstance /= (1024 * 1024);

                      if(innerTableSize < innerTableSizeLimitInMB &&
                         innerTableSizePerInstance < innerTableSizePerInstanceLimitInMB)
                        {

                          // Use the Unique Hash Join Implementation.
                          //
                          return TRUE;
                        }
                    }
                }
            }
          // single inner table did not qualify for unique hash join.
          // give unique hash join another chance using max cardinality
          // estimate to determine if inner hash table fits in memory.
          if (nodes.entries() <= innerTables)
            {
              CostScalar maxRows = rightChildGA->
                getResultMaxCardinalityForEmptyInput();
              CostScalar innerTableMaxSize = maxRows * rowSize
                / (1024 * 1024);
              CostScalar innerTableMaxSizePerInstance = innerTableMaxSize /
                numPartitions;
              if (innerTableMaxSize < innerTableSizeLimitInMB &&
                  innerTableMaxSizePerInstance < innerTableSizePerInstanceLimitInMB)
                // Use the Unique Hash Join Implementation.
                //
                return TRUE;
            }
        }
    }

  return FALSE;
}

// case of hash anti semi join optimization (NOT IN)
// add/build expression to detect inner and outer null :
// checkOuteNullexpr_ : <outer> IS NULL
// checkInnerNullExpr_: <inner} IS NULL

void HashJoin::addCheckNullExpressions(CollHeap * wHeap)
{
  if(!getIsNotInSubqTransform() )
  {
    return;
  }


  ValueId valId;

  Int32 notinCount = 0;

  for ( valId = getEquiJoinPredicates().init();
        getEquiJoinPredicates().next(valId);
        getEquiJoinPredicates().advance(valId)) 
  {
    ItemExpr * itemExpr = valId.getItemExpr();

    if ((itemExpr->getOperatorType() == ITM_EQUAL) && 
        ((BiRelat *)itemExpr)->getIsNotInPredTransform())
    {
      notinCount++;

      ItemExpr * child0 = itemExpr->child(0);
      ItemExpr * child1 = itemExpr->child(1);

      const NAType &outerType = child0->getValueId().getType();
      const NAType &innerType = child1->getValueId().getType();

      if (innerType.supportsSQLnull()  && 
          !((BiRelat*)itemExpr)->getInnerNullFilteringDetected())
      {
        ItemExpr * itm = new (wHeap) UnLogic(ITM_IS_NULL, child1);

        itm->synthTypeAndValueId(TRUE);

        getCheckInnerNullExpr().insert(itm->getValueId());
        // reuse is disabled in this phase on not in optimization
        setReuse(FALSE);

      }
      
      //outer column
      if (outerType.supportsSQLnull()  && 
          !((BiRelat*)itemExpr)->getOuterNullFilteringDetected())
      {
        ItemExpr * itm = new (wHeap) UnLogic(ITM_IS_NULL, child0);
        itm->synthTypeAndValueId(TRUE);
        getCheckOuterNullExpr().insert(itm->getValueId());
      }

    }
  }
  // assert if more than one notin found
  DCMPASSERT(notinCount = 1);
  
}

//10-060710-7606(port of 10-050706-9430) - Begin
// This function was Reimplemented (7/1/08) soln 10-080518-3261

// This function Recursively navigates the item expression
// tree to collect all the value ids needed for expression
// evaluation.

void gatherValuesFromExpr(ValueIdList &vs,
                          ItemExpr *ie,
                          Generator *generator)
{
  ValueId valId(ie->getValueId());
  if(generator->getMapInfoAsIs(valId))
    {
      // If it is directly in the mapTable record then it is an
      // output of the right child.
      vs.insert(valId);
    }
  else
    {
      // Check for a special case like a CAST(<x>) where <x> is
      // available in the MapTable.
      for (Int32 i=0; i < ie->getArity(); i++)
	{
	  gatherValuesFromExpr(vs, ie->child(i), generator);
	}
    }
}

short MergeJoin::codeGen(Generator * generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  Space * space = generator->getSpace();
  MapTable * my_map_table = generator->appendAtEnd();

  // set flag to enable pcode for indirect varchar
  NABoolean vcflag = exp_gen->handleIndirectVC();
  if (CmpCommon::getDefault(VARCHAR_PCODE) == DF_ON) {
    exp_gen->setHandleIndirectVC( TRUE );
  }

  NABoolean is_semijoin = isSemiJoin();
  NABoolean is_leftjoin = isLeftJoin();
  NABoolean is_anti_semijoin = isAntiSemiJoin();

  // find if the left child and/or the right child will have atmost
  // one matching row. If so, an faster merge join implementation
  // will be used at runtime.
  // This optimization is not used for left or semi joins.
  NABoolean isLeftUnique = FALSE;
  NABoolean isRightUnique = FALSE;
  NABoolean fastMJEval = FALSE;
  if ((! is_semijoin) &&
      (! is_anti_semijoin) &&
      (! is_leftjoin))
    {
#ifdef _DEBUG
      isLeftUnique = (getenv("LEFT_UNIQUE_MJ") ? TRUE : leftUnique());
      isRightUnique = (getenv("RIGHT_UNIQUE_MJ") ? TRUE : rightUnique());
#else
      isLeftUnique = leftUnique();
      isRightUnique = rightUnique();
#endif
      if (isLeftUnique || isRightUnique)
	fastMJEval = TRUE;
    }

  ex_expr * merge_expr = 0;
  ex_expr * comp_expr = 0;
  ex_expr * pre_join_expr = 0;
  ex_expr * post_join_expr = 0;

  ex_expr * left_check_dup_expr = 0;
  ex_expr * right_check_dup_expr = 0;

  ex_expr * left_encoded_key_expr = NULL;
  ex_expr * right_encoded_key_expr = NULL;

  ////////////////////////////////////////////////////////////////////////////
  //
  // Layout of row returned by this node.
  //
  // |------------------------------------------------------------------------|
  // | input data | left child's data | right child's data | instantiated row |
  // | ( I tupps) | ( L tupps )       | ( R tupp )         |  ( 1 tupp )      |
  // |------------------------------------------------------------------------|
  //
  // input data:        the atp input to this node by its parent. This is given
  //                    to both children as input.
  // left child data:   tupps appended by the left child
  // right child data:  tupps appended by the left child
  // instantiated row:  For some left join cases, the
  //                    null values are instantiated. See proc
  //                    Join::instantiateValuesForLeftJoin for details at the
  //                    end of this file.
  //
  // Returned row to parent contains:
  //
  //   I + L tupps, if this is a semi join. Rows from right are not returned.
  //
  // If this is not a semi join, then:
  //    I + L + R tupps, if instantiation is not done.
  //    I + L + R + 1 tupps, if instantiation is done.
  //
  ////////////////////////////////////////////////////////////////////////////
  ex_cri_desc * given_desc = generator->getCriDesc(Generator::DOWN);

  generator->setCriDesc(given_desc, Generator::DOWN);
  child(0)->codeGen(generator);
  ComTdb * child_tdb1 = (ComTdb *)(generator->getGenObj());
  ExplainTuple *leftExplainTuple = generator->getExplainTuple();
  ex_cri_desc * left_child_desc = generator->getCriDesc(Generator::UP);

  generator->setCriDesc(left_child_desc, Generator::DOWN);
  child(1)->codeGen(generator);
  ComTdb * child_tdb2 = (ComTdb *)(generator->getGenObj());
  ExplainTuple *rightExplainTuple = generator->getExplainTuple();
  ex_cri_desc * right_child_desc = generator->getCriDesc(Generator::UP);

  unsigned short returned_tuples;
  short returned_instantiated_row_atp_index = -1;
  short returned_right_row_atp_index = -1;

  ex_cri_desc * work_cri_desc = NULL;
  short instantiated_row_atp_index = -1;
  short encoded_key_atp_index = -1;

  work_cri_desc = new(space) ex_cri_desc(3, space);

  if (is_semijoin || is_anti_semijoin)
    returned_tuples = left_child_desc->noTuples();
  else
    {
      // not a semi join.

      // if right side can return atmost one rows, then no need to
      // save dups. No new row is created at returned_right_row_atp_index
      // in this case.
      if (fastMJEval)
	returned_tuples = right_child_desc->noTuples();
      else
	{
	  returned_tuples = (unsigned short)(left_child_desc->noTuples() + 1);
	  returned_right_row_atp_index = returned_tuples - 1;
	}

      if (nullInstantiatedOutput().entries() > 0)
	{
	  instantiated_row_atp_index = 3;

	  returned_instantiated_row_atp_index = (short) returned_tuples++;
	}
    }

  ex_cri_desc * returned_desc = new(space) ex_cri_desc(returned_tuples, space);
  GenAssert(!getOrderedMJPreds().isEmpty(),"getOrderedMJPreds().isEmpty()");

  ////////////////////////////////////////////////////////////
  // Before generating any expression for this node, set the
  // the expression generation flag not to generate float
  // validation PCode. This is to speed up PCode evaluation
  ////////////////////////////////////////////////////////////
  generator->setGenNoFloatValidatePCode(TRUE);

  NABoolean doEncodedKeyCompOpt = FALSE;
  doEncodedKeyCompOpt = TRUE;
  encoded_key_atp_index = 2;

  // generate expressions to find out if left or right rows are duplicate
  // of the previous rows.
  ValueIdSet right_dup_val_id_set;
  ValueIdList leftChildOfMJPList;//list of left children of orderedMJPreds()
  ValueIdList rightChildOfMJPList;//list of right children of orderedMJPreds()
  ValueId val_id;
  CollIndex i;

  for (i = 0; i < orderedMJPreds().entries(); i++)
    {
      // create a place holder node to represent the previous row,
      // which is exactly the same as
      // the child values except for its atp. At runtime, the previous
      // value is passed to the expression evaluator as the second atp.

      // Usually, the child RelExpr values are the immediate children
      // of the orderedMJPreds.  However, in some cases an immediate
      // child is an expression made up of values coming from the
      // child RelExpr.  Typically, this is a Cast expression
      // introduced by a MapValueId node.  Here, we handle the usual
      // case and the case of the Cast expression.  Other expressions
      // that are not evaluated by the child RelExpr will result in an
      // Assertion.  If we ever trigger this assertion, we may need to
      // make this code more general.


      val_id = orderedMJPreds()[i];

      // Do the right row.
      ValueId child1Vid =
        val_id.getItemExpr()->child(1)->castToItemExpr()->getValueId();

      // Place holder for right values.
      //
      Cast *ph = NULL;

      // Attributes of the right child.
      //
      Attributes *childAttr = NULL;

      // ValueId of value actually supplied by child RelExpr.
      //
      ValueId childOutputVid;

      MapInfo *child1MapInfo = generator->getMapInfoAsIs(child1Vid);

      // If there is no mapInfo for the immediate child, then it is
      // not directly supplied by the child RelExpr.  Check for the
      // case when the immediate child is a CAST and the child of the
      // CAST is supplied directly by the child RelExpr.
      //
      if(!child1MapInfo)
        {

          // If this is a CAST AND ...
          //
          if((child1Vid.getItemExpr()->getOperatorType() == ITM_CAST) ||
            (child1Vid.getItemExpr()->getOperatorType() == ITM_TRANSLATE) ||
            (child1Vid.getItemExpr()->getOperatorType() == ITM_NOTCOVERED))
            {
              ValueId nextChild0Vid =
                child1Vid.getItemExpr()->child(0)->castToItemExpr()->getValueId();

              // If the child of the CAST is in the mapTable (supplied by
              // the child RelExpr)
              //
              if (generator->getMapInfoAsIs(nextChild0Vid))
                {

                  // Remember the actual value supplied by the child RelExpr.
                  //
                  childOutputVid = nextChild0Vid;

                  // Create place holder node.
                  //
                  ph = new (generator->wHeap())
                    Cast(child1Vid.getItemExpr()->child(0),
                         &(nextChild0Vid.getType()));

                  // Attributes for this place holder node.  Same as the
                  // child value, but we will later change the ATP.
                  //
                  childAttr = generator->getMapInfo(nextChild0Vid)->getAttr();
                }
            }
        }
      else
        {

          // The immediate child is supplied by the child RelExpr.
          //

          // Remember the actual value supplied by the child RelExpr.
          //
          childOutputVid = child1Vid;

          // Create place holder node.
          //
          ph = new(generator->wHeap())
            Cast(val_id.getItemExpr()->child(1),
                 &(child1Vid.getType()));

          // Attributes for this place holder node.  Same as the
          // child value, but we will later change the ATP.
          //
          childAttr = generator->getMapInfo(child1Vid)->getAttr();
        }

      // If we did not find a childAttr, then neither the immediate
      // child nor the child of an immediate CAST is supplied by the
      // child RelExpr.  We need to be more general here.
      //
      GenAssert(childAttr, "Merge Join: expression not found");


      ph->bindNode(generator->getBindWA());


      if ( childAttr->getAtpIndex() > 1)
	{
          // Make a mapTable entry for the place holder, just like the
          // child value
          //
	  MapInfo * map_info = generator->addMapInfo(ph->getValueId(),
						     childAttr);
          // Make this mapTable entry refer to ATP 1.
          //
	  map_info->getAttr()->setAtp(1);

	  // mark ph as code generated node since we don't want the
	  // Cast to actually do a conversion.
	  map_info->codeGenerated();
	}

      if(!child1MapInfo)
        {

          // If the immediate child is not supplied by the child RelExpr
          // and it is a CAST node, we need to add the equivalent CAST
          // node for the left side of the DUP expression.
          // Here is the expression we need to generate in this case:
          //                          EQUAL
          //                            |
          //                      /------------\
          //                    CAST           CAST must create equiv node here
          //                     |              |
          //  supplied by Child  A        PlaceHolder-For-A
          //
          ph =
            new(generator->wHeap()) Cast(ph,
                                         &(child1Vid.getType()));
        }

      BiRelat * bi_relat =
	new(generator->wHeap()) BiRelat(ITM_EQUAL,
					val_id.getItemExpr()->child(1),
			     ph);
      // for the purpose of checking duplicates, nulls are equal
      // to other nulls. Mark them so.
      bi_relat->setSpecialNulls(-1);

      bi_relat->bindNode(generator->getBindWA());

      leftChildOfMJPList.insert(val_id.getItemExpr()->child(0)->getValueId());

      // This must be the actual values supplied by Child RelExpr.
      rightChildOfMJPList.insert(childOutputVid);
      right_dup_val_id_set.insert(bi_relat->getValueId());

     if  ((val_id.getItemExpr()->child(0)->getValueId().getType().supportsSQLnull()) ||
	  (val_id.getItemExpr()->child(1)->getValueId().getType().supportsSQLnull()))
       doEncodedKeyCompOpt = FALSE;
    }

  // now generate an expression to see if the left values are less
  // than the right values. This is needed to advance the left child
  // if the expression is true.
  // Note: later, only generate one expression for merge and comp
  // and have it return status indicating if the left row is less than,
  // equal to or greated than the right. Use a CASE statement to do that.

  ExprValueId left_tree = (ItemExpr *) NULL;
  ExprValueId right_tree = (ItemExpr *) NULL;

  ItemExprTreeAsList * left_list = NULL;
  ItemExprTreeAsList * right_list = NULL;

  ValueIdList leftEncodedValIds;
  ValueIdList rightEncodedValIds;
  ULng32 encoded_key_len = 0;

  if (NOT doEncodedKeyCompOpt)
    {
      left_list = new(generator->wHeap()) ItemExprTreeAsList(
	   &left_tree,
	   ITM_ITEM_LIST,
	   RIGHT_LINEAR_TREE);
      right_list = new(generator->wHeap()) ItemExprTreeAsList(
	   &right_tree,
	   ITM_ITEM_LIST,
	   RIGHT_LINEAR_TREE);
    }

  for (i = 0; i < orderedMJPreds().entries(); i++)
    {
      val_id = orderedMJPreds()[i];

      ItemExpr * left_val = val_id.getItemExpr()->child(0);
      ItemExpr * right_val = val_id.getItemExpr()->child(1);

      // if the left and right values do not have the same type,
      // then convert them to a common super type before encoding.
      const NAType & leftType = left_val->getValueId().getType();
      const NAType & rightType = right_val->getValueId().getType();
      if (NOT (leftType == rightType))
	{
         UInt32 flags =
              ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
               ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);

	  // find the common super datatype xs
	  const NAType *resultType =
	    leftType.synthesizeType(
		 SYNTH_RULE_UNION,
		 leftType,
		 rightType,
		 generator->wHeap(),
                &flags);

	  CMPASSERT(resultType);

	  // add type conversion operators if necessary
	  if (NOT (leftType == *resultType))
	    {
	      left_val = new(generator->wHeap()) Cast(left_val,resultType);
	    }

	  if (NOT (rightType == *resultType))
	    {
	      right_val = new(generator->wHeap()) Cast(right_val,resultType);
	    }
	}

      // encode the left and right values before doing the comparison.
      short desc_flag = FALSE;
      if (getLeftSortOrder()[i].getItemExpr()->getOperatorType() == ITM_INVERSE)
	desc_flag = TRUE;

      CompEncode * encoded_left_val
	= new(generator->wHeap()) CompEncode(left_val, desc_flag);
      CompEncode * encoded_right_val
	= new(generator->wHeap()) CompEncode(right_val, desc_flag);

      encoded_left_val->bindNode(generator->getBindWA());
      encoded_right_val->bindNode(generator->getBindWA());

      if (doEncodedKeyCompOpt)
	{
	  leftEncodedValIds.insert(encoded_left_val->getValueId());
	  rightEncodedValIds.insert(encoded_right_val->getValueId());
	}
      else
	{
	  // add the search condition
	  left_list->insert(encoded_left_val);
	  right_list->insert(encoded_right_val);
	}
    }

  ItemExpr * compTree = 0;

  if (NOT doEncodedKeyCompOpt)
    {
      compTree = new(generator->wHeap()) BiRelat(ITM_LESS, left_tree, right_tree);

      compTree = new(generator->wHeap()) BoolResult(compTree);

      // bind/type propagate the comp tree
      compTree->bindNode(generator->getBindWA());
    }

  // At runtime when this merge join expression is evaluated, the left child
  // values are passed in the first atp, and the right child values
  // are passed in the second atp. Change the atp value of right child's output
  // to 1, if it is not already an input value.
  const ValueIdSet & child1CharacteristicOutputs =
    child(1)->castToRelExpr()->getGroupAttr()->getCharacteristicOutputs();

  // create a list of all values returned from right child that are
  // not input to this node.
  ValueIdList rightChildOutput;

  for (val_id = child1CharacteristicOutputs.init();
                child1CharacteristicOutputs.next(val_id);
                child1CharacteristicOutputs.advance(val_id))
    {
      // If it is part of my input or not in the mapTable...  The
      // assumption is that if it is not in the mapTable and it is not
      // directly in the inputs, it can be derived from the inputs
      //
      if (! getGroupAttr()->getCharacteristicInputs().contains(val_id))
	{
	  // This new function takes care of the CAST(<x>) function as well
	  gatherValuesFromExpr(rightChildOutput, val_id.getItemExpr(), generator);
	}
    }

  // Change the atp value of right child's output to 1
  // Atpindex of -1 means leave atpindex as is.
  //
  exp_gen->assignAtpAndAtpIndex(rightChildOutput, 1, -1);

  ExpTupleDesc::TupleDataFormat tupleFormat = generator->getInternalFormat();

  ItemExpr * newPredTree = NULL;
  if (NOT doEncodedKeyCompOpt)
    {
      // orderedMJPreds() is a list containing predicates of the form:
      //  <left value1> '=' <right value1>, <left value2> '=' <right value2> ...
      // Generate the merge expression. This is used to find out if
      // the left and right rows match for equi join.
      newPredTree = orderedMJPreds().rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &merge_expr);


      // generate the comp expression. This expression is used
      // to look for a matching value in the hash table.
      exp_gen->generateExpr(compTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &comp_expr);
    }
  else
    {
      // generate expression to create encoded left key buffer.
      // The work atp where encoded is created is passed in as atp1 at runtime.
      exp_gen->generateContiguousMoveExpr(leftEncodedValIds,
					  0, // don't add convert nodes
					  1, // atp 1
					  encoded_key_atp_index,
					  ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
					  encoded_key_len,
					  &left_encoded_key_expr,
					  0,
					  ExpTupleDesc::SHORT_FORMAT);

      // generate expression to create encoded right key buffer
      // The work atp where encoded is created is passed in as atp0 at runtime.
      exp_gen->generateContiguousMoveExpr(rightEncodedValIds,
					  0, // don't add convert nodes
					  0, // atp 0
					  encoded_key_atp_index,
					  ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
					  encoded_key_len,
					  &right_encoded_key_expr,
					  0,
					  ExpTupleDesc::SHORT_FORMAT);
    }

  /*
  // generate expression to evaluate the
  // non-equi join predicates applied before NULL-instantiation
  if (! joinPred().isEmpty())
    {
      ItemExpr * newPredTree;
      newPredTree = joinPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &pre_join_expr);
    }
    */

  // Change the atp value of right child's output to 0 
  // Second argument -1 means leave atpindex as is.
  exp_gen->assignAtpAndAtpIndex(rightChildOutput, 0, -1);

  // generate expression to save the duplicate rows returned from right
  // child. Do it if rightUnique is false.
  ex_expr * right_copy_dup_expr = NULL;
  ULng32 right_row_len = 0;
  if (NOT fastMJEval)
    {
      ValueIdList resultValIdList;
      exp_gen->generateContiguousMoveExpr(rightChildOutput,
					  -1, // add convert nodes
					  1 /*atp*/, 2 /*atpindex*/,
					  tupleFormat,
					  right_row_len,
					  &right_copy_dup_expr,
					  NULL, ExpTupleDesc::SHORT_FORMAT,
					  NULL,
					  &resultValIdList);

      ValueIdList prevRightValIds;  // the secend operand of dup comparison
      CollIndex i = 0;
      for (i = 0; i < resultValIdList.entries(); i++)
        {
          // create the new item xpression
          ItemExpr * newCol = new(generator->wHeap())
            NATypeToItem((NAType *)&(resultValIdList[i].getType()));
          newCol->synthTypeAndValueId();

          // copy the attributes
          Attributes * originalAttr =
            generator->getMapInfo(resultValIdList[i])->getAttr();

          Attributes * newAttr =
            generator->addMapInfo(newCol->getValueId(), 0)->getAttr();
          newAttr->copyLocationAttrs(originalAttr);

          // set atp
          newAttr->setAtp(1);

          // add the new valueId to the list of 2nd operand
          prevRightValIds.insert(newCol->getValueId());
        }

      // At runtime, duplicate right rows in right child up queue are checked
      // by comparing that row with one of the saved right dup rows.
      ValueIdSet right_dup_val_id_set;
      for (i = 0; i < rightChildOfMJPList.entries(); i++)
	{
	  val_id = rightChildOfMJPList[i];	  
	  CollIndex index = rightChildOutput.index(val_id);
	  
	  BiRelat * bi_relat = 
	    new(generator->wHeap()) BiRelat(ITM_EQUAL, 
					    rightChildOfMJPList[i].getItemExpr(),
					    prevRightValIds[index].getItemExpr());
	  // for the purpose of checking duplicates, nulls are equal
	  // to other nulls. Mark them so.
	  bi_relat->setSpecialNulls(-1);
	  
	  bi_relat->bindNode(generator->getBindWA()); 
	  
	  right_dup_val_id_set.insert(bi_relat->getValueId());
	}
      
      // generate expressions to do the duplicate row checks for right child.
      // The row returned from right child is compared to the saved row returned
      // by the right child.
      newPredTree = right_dup_val_id_set.rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &right_check_dup_expr);  

      for (i = 0; i < resultValIdList.entries(); i++)
	{
	  ValueId resultValId = resultValIdList[i];
	  ValueId rightChildOutputValId = rightChildOutput[i];
	  Attributes * resultAttr = generator->getMapInfo(resultValId)->getAttr();
	  Attributes * rightChildAttr = generator->getMapInfo(rightChildOutputValId)->getAttr();
	  Int32 rightChildAtpIndex = rightChildAttr->getAtpIndex();
	  rightChildAttr->copyLocationAttrs(resultAttr);
	}

      // at runtime, duplicate left rows are checked by comparing the
      // left row with one of the saved right dup rows.
      ValueIdSet left_dup_val_id_set;
      for (i = 0; i < rightChildOfMJPList.entries(); i++)
	{
	  val_id = rightChildOfMJPList[i];
	  CollIndex index = rightChildOutput.index(val_id);

	  BiRelat * bi_relat =
	    new(generator->wHeap()) BiRelat(ITM_EQUAL,
					    leftChildOfMJPList[i].getItemExpr(),
					    resultValIdList[index].getItemExpr());
	  // for the purpose of checking duplicates, nulls are equal
	  // to other nulls. Mark them so.
	  bi_relat->setSpecialNulls(-1);

	  bi_relat->bindNode(generator->getBindWA());

	  left_dup_val_id_set.insert(bi_relat->getValueId());
	}

      // generate expressions to do the duplicate row checks for left child.
      // The row returned from left child is compared to the saved row returned
      // by the right child.
      newPredTree = left_dup_val_id_set.rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &left_check_dup_expr);
    }

  // generate expression to evaluate the
  // non-equi join predicates applied before NULL-instantiation
  if (! joinPred().isEmpty())
    {
      ItemExpr * newPredTree;
      newPredTree = joinPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &pre_join_expr);
    }

  // now change the atp to 0 and atpindex to returned_right_row_atp_index.
  if (NOT fastMJEval)
    {
      exp_gen->assignAtpAndAtpIndex(rightChildOutput, 0, returned_right_row_atp_index);
    }

  ex_expr * lj_expr = 0;
  ex_expr * ni_expr = 0;
  ULng32 rowlen = 0;

  if (nullInstantiatedOutput().entries() > 0)
    {
      instantiateValuesForLeftJoin(generator,
				   0, returned_instantiated_row_atp_index,
				   &lj_expr, &ni_expr,
				   &rowlen,
				   NULL // No MapTable required
				   );
    }

  // set the atp index for values in the instantiated row.
  for (i = 0; i < nullInstantiatedOutput().entries(); i++)
    {
      ValueId val_id = nullInstantiatedOutput()[i];

      Attributes * attr = generator->getMapInfo(val_id)->getAttr();
      attr->setAtp(0);
      attr->setAtpIndex(returned_instantiated_row_atp_index);
      // Do not do bulk move because null instantiate expression
      // is not set in TDB to save execution time, see ComTdbMj below
      attr->setBulkMoveable(FALSE);
    }

  // generate any expression to be applied after the join
  if (! selectionPred().isEmpty())
    {
      ItemExpr * newPredTree = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &post_join_expr);
    }

  bool isOverflowEnabled = (CmpCommon::getDefault(MJ_OVERFLOW) == DF_ON);
  UInt16 scratchThresholdPct
    = (UInt16) getDefault(SCRATCH_FREESPACE_THRESHOLD_PERCENT);

  // Big Memory Operator (BMO) settings
  // Use memory quota only if fragment has more than one BMO.
  UInt16 numBMOsInFrag = (UInt16)generator->getFragmentDir()->getNumBMOs();

  double BMOsMemoryLimit = 0;
  UInt16 quotaMB = 0;
  Lng32 numStreams;
  double memQuotaRatio;
  double bmoMemoryUsage = getEstimatedRunTimeMemoryUsage(TRUE, &numStreams).value();

  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  if ( CmpCommon::getDefaultLong(MJ_BMO_QUOTA_PERCENT) != 0) 
  {
    // Apply quota system if either one the following two is true:
    //   1. the memory limit feature is turned off and more than one BMOs
    //   2. the memory limit feature is turned on
    NABoolean mlimitPerNode = defs.getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) > 0;
  
    if ( mlimitPerNode || numBMOsInFrag > 1 ||
         (numBMOsInFrag == 1 && CmpCommon::getDefault(EXE_SINGLE_BMO_QUOTA) == DF_ON)) {
  
      quotaMB = (UInt16)
          computeMemoryQuota(generator->getEspLevel() == 0,
                             mlimitPerNode,
                             generator->getBMOsMemoryLimitPerNode().value(),
                             generator->getTotalNumBMOs(),
                             generator->getTotalBMOsMemoryPerNode().value(),
                             numBMOsInFrag, 
                             bmoMemoryUsage,
                             numStreams,
                             memQuotaRatio
                             );
    }
    Lng32 mjMemoryLowbound = defs.getAsLong(EXE_MEMORY_LIMIT_LOWER_BOUND_MERGEJOIN);
    Lng32 memoryUpperbound = defs.getAsLong(BMO_MEMORY_LIMIT_UPPER_BOUND);

    if ( quotaMB < mjMemoryLowbound ) {
       quotaMB = (UInt16)mjMemoryLowbound;
       memQuotaRatio = BMOQuotaRatio::MIN_QUOTA;
    }
    else if (quotaMB >  memoryUpperbound)
      quotaMB = memoryUpperbound;
  } else {
    Lng32 quotaMB = defs.getAsLong(EXE_MEMORY_LIMIT_LOWER_BOUND_MERGEJOIN);
  }


  bool yieldQuota = !(generator->getRightSideOfFlow());
  UInt16 quotaPct = (UInt16) getDefault(MJ_BMO_QUOTA_PERCENT);

#pragma nowarn(1506)   // warning elimination
  ComTdbMj * mj_tdb =
    new(space) ComTdbMj(child_tdb1,
			child_tdb2,
			given_desc,
			returned_desc,
			(NOT doEncodedKeyCompOpt
			 ? merge_expr : left_encoded_key_expr),
			(NOT doEncodedKeyCompOpt
			 ? comp_expr : right_encoded_key_expr),
			left_check_dup_expr,
			right_check_dup_expr,
			lj_expr,
			0,
                        right_copy_dup_expr,
                        right_row_len,
			rowlen,
			work_cri_desc,
			instantiated_row_atp_index,
			encoded_key_len,
			encoded_key_atp_index,
			pre_join_expr,
			post_join_expr,
			(queue_index)getDefault(GEN_MJ_SIZE_DOWN),
			(queue_index)getDefault(GEN_MJ_SIZE_UP),
			(Cardinality) getGroupAttr()->
			getOutputLogPropList()[0]->getResultCardinality().value(),
			getDefault(GEN_MJ_NUM_BUFFERS),
			getDefault(GEN_MJ_BUFFER_SIZE),
			is_semijoin,
			is_leftjoin,
			is_anti_semijoin,
			isLeftUnique,
			isRightUnique,
			isOverflowEnabled,
			scratchThresholdPct,
			quotaMB,
			quotaPct,
			yieldQuota);
#pragma warn(1506)  // warning elimination
  generator->initTdbFields(mj_tdb);

  if (CmpCommon::getDefault(EXE_DIAGNOSTIC_EVENTS) == DF_ON)
  {
    mj_tdb->setLogDiagnostics(true);
  }
  mj_tdb->setOverflowMode(generator->getOverflowMode());

  if (!generator->explainDisabled()) {
    generator->setExplainTuple(
         addExplainInfo(mj_tdb, leftExplainTuple, rightExplainTuple, generator));
  }

  generator->setGenObj(this, mj_tdb);

  // restore the original down cri desc since this node changed it.
  generator->setCriDesc(given_desc, Generator::DOWN);

  // set the new up cri desc.
  generator->setCriDesc(returned_desc, Generator::UP);

  // reset the expression generation flag to generate float validation pcode
  generator->setGenNoFloatValidatePCode(FALSE);

  // reset the handleIndirectVC flag to its initial value
  exp_gen->setHandleIndirectVC( vcflag );

  return 0;
} // MergeJoin::codeGen

short NestedJoin::codeGen(Generator * generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  // set flag to enable pcode for indirect varchar
  NABoolean vcflag = exp_gen->handleIndirectVC();
  if (CmpCommon::getDefault(VARCHAR_PCODE) == DF_ON) {
    exp_gen->setHandleIndirectVC( TRUE );
  }

  ex_expr * after_expr = 0;

  NABoolean is_semijoin = isSemiJoin();
  NABoolean is_antisemijoin = isAntiSemiJoin();
  NABoolean is_leftjoin = isLeftJoin();
  NABoolean is_undojoin = isTSJForUndo();
  NABoolean is_setnferror = isTSJForSetNFError();


  ////////////////////////////////////////////////////////////////////////////
  //
  // Layout of row returned by this node.
  //
  // |------------------------------------------------------------------------|
  // | input data  | left child's data | right child's data| instantiated row |
  // | ( I tupps ) |  ( L tupps )      | ( R tupps )       |  ( 1 tupp )      |
  // |------------------------------------------------------------------------|
  //
  // <-- returned row from left ------->
  // <------------------ returned row from right ---------->
  //
  // input data:        the atp input to this node by its parent.
  // left child data:   tupps appended by the left child
  // right child data:  tupps appended by right child
  // instantiated row:  For some left join cases, the
  //                    null values are instantiated. See proc
  //                    Join::instantiateValuesForLeftJoin for details at the end of
  //                    this file.
  //
  // Returned row to parent contains:
  //
  //   I + L tupps, if this is a semi join. Rows from right are not returned.
  //
  //   If this is not a semi join, then:
  //      I + L + R tupps, if instantiation is not done.
  //      I + L + R + 1 tupps, if instantiation is done.
  //
  ////////////////////////////////////////////////////////////////////////////


  ex_cri_desc * given_desc = generator->getCriDesc(Generator::DOWN);

  // It is OK for neither child to exist when generating a merge union TDB
  // for index maintenenace. The children are filled in at build time.
  //
  GenAssert((child(0) AND child(1)) OR (NOT child(0) AND NOT (child(1))),
	    "NestedJoin -- missing one child");

  ComTdb * tdb1 = NULL;
  ComTdb * tdb2 = NULL;
  ExplainTuple *leftExplainTuple = NULL;
  ExplainTuple *rightExplainTuple = NULL;

  //++Triggers
  // insert a temporary map table, so that we can later delete the children's map
  // tables in case the nested join doesn't return values.
  MapTable * beforeLeftMapTable = generator->appendAtEnd();
  //--Triggers

  // MV --
  // We need to know if the right child is a VSBB Insert node.
  NABoolean rightChildIsVsbbInsert  = FALSE;
  NABoolean leftChildIsVsbbInsert   = FALSE;



  GenAssert(!generator->getVSBBInsert(), "Not expecting VSBBInsert flag from parent.");

  if(child(0) && child(1)) {
    // generate code for left child tree

    // - MVs
    child(0)->codeGen(generator);
    leftChildIsVsbbInsert = generator->getVSBBInsert();
    generator->setVSBBInsert(FALSE);
    tdb1 = (ComTdb *)(generator->getGenObj());
    leftExplainTuple = generator->getExplainTuple();

    // Override the queue sizes for the left child, if
    // GEN_ONLJ_SET_QUEUE_LEFT is on.
    if (generator->getMakeOnljLeftQueuesBig())
      {
        short queueResizeLimit   = (short) getDefault(DYN_QUEUE_RESIZE_LIMIT);
        short queueResizeFactor  = (short) getDefault(DYN_QUEUE_RESIZE_FACTOR);
        
        queue_index downSize = generator->getOnljLeftDownQueue();
        queue_index upSize = generator->getOnljLeftUpQueue();

        downSize = MAXOF(downSize, tdb1->getInitialQueueSizeDown());
        upSize = MAXOF(upSize, tdb1->getInitialQueueSizeUp());
        
        tdb1->setQueueResizeParams(downSize, upSize, queueResizeLimit, queueResizeFactor);
      }
  }

  ////////////////////////////////////////////////////////////
  // Before generating any expression for this node, set the
  // the expression generation flag not to generate float
  // validation PCode. This is to speed up PCode evaluation
  ////////////////////////////////////////////////////////////
  generator->setGenNoFloatValidatePCode(TRUE);

  child(0)->isRowsetIterator() ? setRowsetIterator(TRUE) : setRowsetIterator(FALSE);
  NABoolean tolerateNonFatalError = FALSE;
  if (child(0)->getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_)
    {
      tolerateNonFatalError = TRUE;
      generator->setTolerateNonFatalError(TRUE);
      generator->setTolerateNonFatalErrorInFlowRightChild(TRUE);
    }

  if (getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_)
    {
      tolerateNonFatalError = TRUE;
    }

  ex_expr * before_expr = 0;
  // generate join expression, if present.
  if (! joinPred().isEmpty())
    {
      ItemExpr * newPredTree
        = joinPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
                            &before_expr);
    }

  ex_cri_desc * left_child_desc = generator->getCriDesc(Generator::UP);

  // if semi join, save the address of the last map table.
  // This is used later to remove
  // all map tables appended by the right child tree as the right child
  // values are not visible above this node.
  MapTable * save_map_table = 0;
  if (is_semijoin || is_antisemijoin)
    save_map_table = generator->getLastMapTable();

  if(child(0) && child(1)) {
    // give to the second child the returned descriptor from first child
    generator->setCriDesc(left_child_desc, Generator::DOWN);

    // reset the expression generation flag
    generator->setGenNoFloatValidatePCode(FALSE);

    // remember that we're code gen'ing the right side of a join.
    NABoolean wasRightSideOfOnlj = generator->getRightSideOfOnlj();
    NABoolean savComputeRowsetRowsAffected =
      generator->computeRowsetRowsAffected();
    if (getRowsetRowCountArraySize() > 0)
      generator->setComputeRowsetRowsAffected(TRUE);
    generator->setRightSideOfOnlj(TRUE);

    // RHS of NestedJoin starts with LargeQueueSizes not in use (0).
    // If a SplitTop is found, it may set the largeQueueSize to
    // an appropriate value.
    ULng32 largeQueueSize = generator->getLargeQueueSize();
    generator->setLargeQueueSize(0);

    // generate code for right child tree
    child(1)->codeGen(generator);

    // Above the NestedJoin, we restore the LargeQueueSize to what
    // was in effect before.
    generator->setLargeQueueSize(largeQueueSize);

    generator->setRightSideOfOnlj(wasRightSideOfOnlj);
    generator->setComputeRowsetRowsAffected(savComputeRowsetRowsAffected);

    rightChildIsVsbbInsert = generator->getVSBBInsert();

    // Because of the bushy tree optimizer rule, there is a chance that our
    // left child is a VSBB Insert, so we need to pass the flag to our parent.
    generator->setVSBBInsert(leftChildIsVsbbInsert);

    tdb2 = (ComTdb *)(generator->getGenObj());
    rightExplainTuple = generator->getExplainTuple();
  }
 // turn of the the Right Child Only flag. Note we turn it off only after making sure
  // that we are in the same NestedJoinFlow::codeGen method that turned it on in the
  // first place.
  if (tolerateNonFatalError)
    generator->setTolerateNonFatalErrorInFlowRightChild(FALSE);

  ex_cri_desc * right_child_desc = generator->getCriDesc(Generator::UP);

  short returned_instantiated_row_atp_index = -1;

  // set the expression generation flag not to generate float
  // validation PCode again, as it might be reset above
  generator->setGenNoFloatValidatePCode(TRUE);

  // only the left child's rows are returned for semi join.
  unsigned short returned_tuples = left_child_desc->noTuples();
  if (! is_semijoin) {
    returned_tuples = right_child_desc->noTuples();
    if (nullInstantiatedOutput().entries() > 0)
      returned_instantiated_row_atp_index = (short) returned_tuples++;
  }
  ex_cri_desc * returned_desc = new(space) ex_cri_desc(returned_tuples, space);

  ValueIdSet afterPredicates;
  if ( is_semijoin || is_antisemijoin || is_leftjoin )
    {
      afterPredicates = selectionPred();
    }
  else
    {
      GenAssert(joinPred().isEmpty(),"NOT joinPred().isEmpty()");
      // Since this is a form of TSJ selectionPred() should also be empty
      // Since this is a form of TSJ selectionPred() should also be empty
      if (getGroupAttr()->isGenericUpdateRoot() AND (NOT selectionPred().isEmpty()))
	afterPredicates = selectionPred();
      else
	GenAssert(selectionPred().isEmpty(),"NOT selectionPred().isEmpty()");
    }

  ex_expr * lj_expr = 0;
  ex_expr * ni_expr = 0;
  ULng32 rowlen = 0;

  if (nullInstantiatedOutput().entries() > 0)
    {
      instantiateValuesForLeftJoin(generator,
				   0, returned_instantiated_row_atp_index,
				   &lj_expr, &ni_expr,
				   &rowlen,
				   NULL // No MapTable required
				   );
      Attributes *attr     = 0;
      ItemExpr   *itemExpr = 0;

      for (CollIndex i = 0; i < nullInstantiatedOutput().entries(); i++)
        {
          itemExpr = nullInstantiatedOutput()[i].getItemExpr();
          attr = (generator->getMapInfo( itemExpr->getValueId() ))->getAttr();
          // Do not do bulk move because null instantiate expression
          // is not set in TDB to save execution time, see ComTdbOnlj below
          attr->setBulkMoveable(FALSE);
	}
    }

  // right child's values are not returned for semi join. Remove them.
  if (is_semijoin || is_antisemijoin)
    generator->removeAll(save_map_table);

  // generate after join expression, if present.
  if (! afterPredicates.isEmpty())
    {
      ItemExpr * newPredTree = afterPredicates.rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &after_expr);
    }

  //++Triggers
  // no characteristic output, remove values that where generated by the children
  if (!(getGroupAttr()->getCharacteristicOutputs().entries()))
	generator->removeAll(beforeLeftMapTable);
  //--Triggers

  // get the default value for the buffer size
  ULng32 bufferSize = (ULng32) getDefault(GEN_ONLJ_BUFFER_SIZE);

  // adjust the default and compute the size of a buffer that can
  // accommodate five rows. The number five is an arbitrary number. 
  // Too low a number means that at execution time row processing might
  // be blocked waiting for an empty buffer and too large a number might imply
  // waste of memory space
  if (rowlen)
  {
     bufferSize = MAXOF(bufferSize,
                        SqlBufferNeededSize(5, (Lng32)rowlen, SqlBuffer::NORMAL_));
  }



  // is this join used to drive mv logging
  RelExpr *MvLogExpr = this;
  while ((MvLogExpr->child(0)->castToRelExpr()->getOperator() == REL_NESTED_JOIN) || 
         (MvLogExpr->child(0)->castToRelExpr()->getOperator() == REL_LEFT_NESTED_JOIN))
     MvLogExpr = MvLogExpr->child(0)->castToRelExpr();
    
  while ((MvLogExpr->child(1)->castToRelExpr()->getOperator() == REL_NESTED_JOIN) || 
         (MvLogExpr->child(1)->castToRelExpr()->getOperator() == REL_LEFT_NESTED_JOIN) ||
         (MvLogExpr->child(1)->castToRelExpr()->getOperator() == REL_NESTED_JOIN_FLOW))
     MvLogExpr = MvLogExpr->child(1)->castToRelExpr();
    
  RelExpr *rightChildExpr = MvLogExpr->child(1)->castToRelExpr();
  OperatorTypeEnum rightChildOp = rightChildExpr->getOperatorType();
  NABoolean usedForMvLogging = FALSE;

#pragma nowarn(1506)   // warning elimination
  ComTdbOnlj * nlj_tdb =
    new(space) ComTdbOnlj(tdb1,
			  tdb2,
			  given_desc,
			  returned_desc,
			  (queue_index)getDefault(GEN_ONLJ_SIZE_DOWN),
			  (queue_index)getDefault(GEN_ONLJ_SIZE_UP),
			  (Cardinality) getGroupAttr()->
			  getOutputLogPropList()[0]->
			  getResultCardinality().value(),
			  getDefault(GEN_ONLJ_NUM_BUFFERS),
			  bufferSize,
			  before_expr,
			  after_expr,
			  lj_expr, 0,
			  0,
			  0,
			  rowlen,
			  is_semijoin,
			  is_antisemijoin,
			  is_leftjoin,
			  is_undojoin,
			  is_setnferror,
			  isRowsetIterator(),
			  isIndexJoin(),
			  rightChildIsVsbbInsert,
			  getRowsetRowCountArraySize(),
			  tolerateNonFatalError,
			  usedForMvLogging 
			  );
#pragma warn(1506)  // warning elimination
// getRowsetRowCountArraySize() should return positive values
// only if isRowsetIterator() returns TRUE.
  GenAssert((((getRowsetRowCountArraySize() > 0) && isRowsetIterator()) ||
	    (getRowsetRowCountArraySize() == 0)),
	    "Incorrect value returned by getRowsetRowCountArray()");

  generator->initTdbFields(nlj_tdb);

  // Make sure that the LHS up queue can grow as large as the RHS down
  // queue.
  if(tdb1->getMaxQueueSizeUp() < tdb2->getInitialQueueSizeDown()) {
    tdb1->setMaxQueueSizeUp(tdb2->getInitialQueueSizeDown());
  }

  // If this NestedJoin itself is not on the RHS of a Flow/NestedJoin,
  // Then reset the largeQueueSize to 0.
  if(NOT generator->getRightSideOfFlow()) {
    generator->setLargeQueueSize(0);
  }

  // If it does not have two children, this is index maintenance code and
  // should not be Explained
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
         addExplainInfo(nlj_tdb, leftExplainTuple, rightExplainTuple, generator));
  }

  // restore the original down cri desc since this node changed it.
  generator->setCriDesc(given_desc, Generator::DOWN);

  // set the new up cri desc.
  generator->setCriDesc(returned_desc, Generator::UP);

  generator->setGenObj(this, nlj_tdb);

  // reset the expression generation flag to generate float validation pcode
  generator->setGenNoFloatValidatePCode(FALSE);

  // reset the handleIndirectVC flag to its initial value
  exp_gen->setHandleIndirectVC( vcflag );

  return 0;
}

short NestedJoinFlow::codeGen(Generator * generator)
{
  CostScalar numberOfInputRows = getInputCardinality();

  if ((numberOfInputRows > 1) &&
      (child(1)) &&
      ((child(1)->getOperatorType() == REL_HBASE_DELETE) ||
       (child(1)->getOperatorType() == REL_HBASE_UPDATE)) &&
      (CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON) &&
      (CmpCommon::getDefault(HBASE_UPDEL_CURSOR_OPT) == DF_ON))
    {
      setOperatorType(REL_NESTED_JOIN);

      return NestedJoin::codeGen(generator);
    }

  ExpGenerator * exp_gen = generator->getExpGenerator();
  MapTable * map_table = generator->getMapTable();
  Space * space = generator->getSpace();

  ////////////////////////////////////////////////////////////////////////////
  //
  // Layout of row returned by this node.
  //
  // |---------------------------------|
  // | input data  | left child's data |
  // | ( I tupps ) |  ( L tupps )      |
  // |---------------------------------|
  //
  // <-- returned row from left ------->
  // <- returned row from right ------->
  //
  // input data:        the atp input to this node by its parent.
  // left child data:   tupps appended by the left child
  //
  // Returned row to parent contains:
  //
  //   I + L tupps, since this operator doesn't produce any output.
  //
  ////////////////////////////////////////////////////////////////////////////


  ex_cri_desc * given_desc = generator->getCriDesc(Generator::DOWN);

  ComTdb * tdb1 = NULL;
  ComTdb * tdb2 = NULL;

  ExplainTuple *leftExplainTuple = NULL;
  ExplainTuple *rightExplainTuple = NULL;

  NABoolean tolerateNonFatalError = FALSE;

  if(child(0) && child(1)) {
    // generate code for left child tree
    child(0)->codeGen(generator);
    tdb1 = (ComTdb *)(generator->getGenObj());
    leftExplainTuple = generator->getExplainTuple();
    if (child(0)->isRowsetIterator())
    {
      setRowsetIterator(TRUE);
      if (child(0)->getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_)
      {
	 tolerateNonFatalError = TRUE;
	 generator->setTolerateNonFatalError(TRUE);
	 generator->setTolerateNonFatalErrorInFlowRightChild(TRUE);
      }
    }
    generator->setTupleFlowLeftChildAttrs(child(0)->getGroupAttr());
  }

  ex_cri_desc * left_child_desc = generator->getCriDesc(Generator::UP);

  if(child(0) && child(1)) {
    // give to the second child the returned descriptor from first child
    generator->setCriDesc(left_child_desc, Generator::DOWN);

    // remember that we're code gen'ing the right side of a tuple flow.
    NABoolean wasRightSideOfFlow = generator->getRightSideOfTupleFlow();
    generator->setRightSideOfTupleFlow(TRUE);

    // RHS of Flow starts with LargeQueueSizes not in use (0).
    // If a SplitTop is found, it may set the largeQueueSize to
    // an appropriate value.
    ULng32 largeQueueSize = generator->getLargeQueueSize();
    generator->setLargeQueueSize(0);

    // generate code for right child tree
    child(1)->codeGen(generator);

    // Above the Flow, we restore the LargeQueueSize to what
    // was in effect before.
    generator->setLargeQueueSize(largeQueueSize);
    generator->setRightSideOfTupleFlow(wasRightSideOfFlow);

    tdb2 = (ComTdb *)(generator->getGenObj());
    rightExplainTuple = generator->getExplainTuple();
  }

  // turn of the the Right Child Only flag. Note we turn it off only after making sure
  // that we are in the same NestedJoinFlow::codeGen method that turned it on in the
  // first place.
  if (tolerateNonFatalError)
    generator->setTolerateNonFatalErrorInFlowRightChild(FALSE);

  ex_cri_desc * right_child_desc = generator->getCriDesc(Generator::UP);

  // only the left child's rows are returned for semi join.
  unsigned short returned_tuples = 0;
#ifdef _DEBUG
  if (getenv("RI_DEBUG"))
    returned_tuples = right_child_desc->noTuples();
  else
    returned_tuples = left_child_desc->noTuples();
#else
  returned_tuples = left_child_desc->noTuples();
#endif

  returned_tuples = right_child_desc->noTuples();

  ex_cri_desc * returned_desc = new(space) ex_cri_desc(returned_tuples, space);

  ComTdbTupleFlow * tflow_tdb =
    new(space) ComTdbTupleFlow(tdb1,
			       tdb2,
			       given_desc,
			       returned_desc,
			       0, // no tgt Expr yet
			       0, // no work cri desc yet
			       (queue_index)getDefault(GEN_TFLO_SIZE_DOWN),
			       (queue_index)getDefault(GEN_TFLO_SIZE_UP),
			       (Cardinality) getGroupAttr()->
			       getOutputLogPropList()[0]->
			       getResultCardinality().value(),
#pragma nowarn(1506)   // warning elimination
			       getDefault(GEN_TFLO_NUM_BUFFERS),
			       getDefault(GEN_TFLO_BUFFER_SIZE),
			       generator->getVSBBInsert(),
			       isRowsetIterator(),
			       tolerateNonFatalError);
#pragma warn(1506)  // warning elimination
  generator->initTdbFields(tflow_tdb);

  // turn off the VSBB insert flag in the generator, it has been
  // processed and we don't want other nodes to use it by mistake
  generator->setVSBBInsert(FALSE);

  tflow_tdb->setUserSidetreeInsert(generator->getUserSidetreeInsert());

  // If this Flow itself is not on the RHS of a Flow/NestedJoin,
  // Then reset the largeQueueSize to 0.
  if(NOT generator->getRightSideOfFlow()) {
    generator->setLargeQueueSize(0);
  }
  tflow_tdb->setSendEODtoTgt(sendEODtoTgt_);

  if(!generator->explainDisabled())
    generator->setExplainTuple(addExplainInfo(
	 tflow_tdb, leftExplainTuple, rightExplainTuple, generator));

  // restore the original down cri desc since this node changed it.
  generator->setCriDesc(given_desc, Generator::DOWN);

  // set the new up cri desc.
  generator->setCriDesc(returned_desc, Generator::UP);

  generator->setGenObj(this, tflow_tdb);

  return 0;
}

short Join::instantiateValuesForLeftJoin(Generator * generator,
					 short atp, short atp_index,
					 ex_expr ** lj_expr,
					 ex_expr ** ni_expr,
					 ULng32 * rowlen,
					 MapTable ** newMapTable,
					 ExpTupleDesc::TupleDataFormat tdf)
{
  //////////////////////////////////////////////////////////////////////////////
  // Special handling for left joins:
  //   A null instantiated row is represented as a missing entry at the
  //   atp_index of the row(s) coming up from the right child. Any use of
  //   a value from this (missing) row is treated as a null value.
  //   This works well for the case when the output of the right child
  //   preserves null. That is, the output becomes null if its operand
  //   from the right side is null.
  //
  //   Nulls are not preserved in two cases:
  //
  //   1) If output of the right child depends upon its input.
  //      For example:
  //       select * from t1 left join (select 10 from t2) on ...
  //       In this example, the constant 10 is needed to evaluate the output coming
  //       in from the right child, but the constant 10 is an input to the right
  //       child and has space allocated at atp_index = 0. So even if the row from
  //       the right is 'missing', the output value will be 10.
  //
  //   2) If output of the right is involved in certain expressions which
  //      do not return null if their operand is null.
  //      For example:
  //       select * from t1 left join (select case when a is null then 10 end from t2)
  //       In this case, the output of right will become 10 if the column 'a' from
  //       right table t2 is missing. But that is not correct. The correct value
  //       is a null value for the whole expression, if left join doesn't find
  //       a match.
  //
  //  To handle these cases, the rows from the right are instantiated before
  //  returning back from the Join node.
  //  Two expressions are generated to do this. One, for the case when a match
  //  is found. The right expression is evaluated and its result moved to a separate
  //  tupp. Two, for the case when a match is not found. Then, a null value is
  //  moved to the location of the expression result.
  //
  //////////////////////////////////////////////////////////////////////////////
  ExpGenerator * exp_gen = generator->getExpGenerator();
  MapTable * map_table = generator->getMapTable();
  Space * space = generator->getSpace();


  ExpTupleDesc::TupleDataFormat tupleFormat = generator->getInternalFormat();

  if (tdf != ExpTupleDesc::UNINITIALIZED_FORMAT)
  {
    tupleFormat = tdf;
  }

  exp_gen->generateContiguousMoveExpr(nullInstantiatedOutput(),
				      0, // don't add convert nodes
				      atp,
				      atp_index,
				      tupleFormat,
				      *rowlen,
				      lj_expr,
				      0, // no need for ExpTupleDesc * tupleDesc
				      ExpTupleDesc::SHORT_FORMAT,
				      newMapTable);

  // generate expression to move null values to instantiate buffer.
  ValueIdSet null_val_id_set;
  for (CollIndex i = 0; i < nullInstantiatedOutput().entries(); i++)
    {
      ValueId val_id = nullInstantiatedOutput()[i];

      ConstValue * const_value  = exp_gen->generateNullConst(val_id.getType());

      ItemExpr * ie = new(generator->wHeap())
	Cast(const_value, &(val_id.getType()));
      ie->bindNode(generator->getBindWA());

      generator->addMapInfo(ie->getValueId(),
			    generator->getMapInfo(val_id)->getAttr());

      null_val_id_set.insert(ie->getValueId());
    }

  ULng32 rowlen2=0;
  exp_gen->generateContiguousMoveExpr(null_val_id_set,
				      0, // don't add convert nodes
				      atp, atp_index,
				      tupleFormat,
				      rowlen2,
				      ni_expr,
				      0, // no need for ExpTupleDesc * tupleDesc
				      ExpTupleDesc::SHORT_FORMAT);

  GenAssert(rowlen2 == *rowlen, "Unexpected row length from expression");

  return 0;
}
short Join::instantiateValuesForRightJoin(Generator * generator,
					 short atp, short atp_index,
					 ex_expr ** rj_expr,
					 ex_expr ** ni_expr,
					 ULng32 * rowlen,
					 MapTable ** newMapTable,
					 ExpTupleDesc::TupleDataFormat tdf)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  MapTable * map_table = generator->getMapTable();
  Space * space = generator->getSpace();

  // Don't need a MapTable back. At this point, we have generated all
  // the necessary expressions. This code is here to be consistent with the
  // this one's counterpart - instantiateValuesForLeftJoin
  GenAssert((newMapTable == NULL), "Don't need a Maptable back");

  ExpTupleDesc::TupleDataFormat tupleFormat = generator->getInternalFormat();

  if (tdf != ExpTupleDesc::UNINITIALIZED_FORMAT)
  {
    tupleFormat = tdf;
  }

  exp_gen->generateContiguousMoveExpr(nullInstantiatedForRightJoinOutput(),
				      0, // don't add convert nodes
				      atp, atp_index,
				      tupleFormat,
				      *rowlen,
				      rj_expr,
				      0, // no need for ExpTupleDesc * tupleDesc
				      ExpTupleDesc::SHORT_FORMAT,
				      newMapTable);


  // generate expression to move null values to instantiate buffer.
  ValueIdSet null_val_id_set;
  for (CollIndex i = 0; i < nullInstantiatedForRightJoinOutput().entries(); i++)
    {
      ValueId val_id = nullInstantiatedForRightJoinOutput()[i];

      ConstValue * const_value  = exp_gen->generateNullConst(val_id.getType());

      ItemExpr * ie = new(generator->wHeap())
	Cast(const_value, &(val_id.getType()));
      ie->bindNode(generator->getBindWA());

      generator->addMapInfo(ie->getValueId(),
			    generator->getMapInfo(val_id)->getAttr());

      null_val_id_set.insert(ie->getValueId());
    }
  
  ULng32 rowlen2=0;
  exp_gen->generateContiguousMoveExpr(null_val_id_set,
				      0, // don't add convert nodes
				      atp, atp_index,
				      tupleFormat,
				      rowlen2,
				      ni_expr,
				      0, // no need for ExpTupleDesc * tupleDesc
				      ExpTupleDesc::SHORT_FORMAT);

  GenAssert(rowlen2 == *rowlen, "Unexpected row length from expression");

  return 0;
}

