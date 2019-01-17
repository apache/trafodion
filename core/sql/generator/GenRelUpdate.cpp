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
* File:         GenRelUpdate.C
* Description:  update/delete/insert operators
*               
* Created:      5/17/94
* Language:     C++
*
*
******************************************************************************
*/

#define   SQLPARSERGLOBALS_FLAGS   // must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "Platform.h"

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "RelMisc.h"
#include "RelUpdate.h"
#include "RelJoin.h"
#include "ControlDB.h"
#include "GenExpGenerator.h"
#include "ComTdbDp2Oper.h"
#include "ComTdbUnion.h"
#include "ComTdbOnlj.h"
#include "ComTdbHbaseAccess.h"
#include "PartFunc.h"
#include "HashRow.h"
#include "CmpStatement.h"
#include "OptimizerSimulator.h"
#include "ComTdbFastTransport.h"
#include "CmpSeabaseDDL.h"
#include "NAExecTrans.h"
#include <algorithm>
#include "SqlParserGlobals.h"      // must be last #include

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   DeleteCursor::codeGen()
//   Delete::codeGen()
//
//   Insert::codeGen()
//
//   UpdateCursor::codeGen()
//   Update::codeGen()
//
// ##IM: to be REMOVED:
// ## the imCodeGen methods, Generator::im*, the executor imd class.
//
//////////////////////////////////////////////////////////////////////

extern  int CreateAllCharsExpr(const NAType &formalType,
                               ItemExpr &actualValue,
                               CmpContext *cmpContext,
                               ItemExpr *&newExpr);
                               
inline static NABoolean getReturnRow(const GenericUpdate *gu,
				     const IndexDesc *index)
{
  return gu->producesOutputs();
}

static DP2LockFlags initLockFlags(GenericUpdate *gu, Generator * generator)
{
  // fix case 10-040429-7402 by checking gu's statement level access options
  // first before declaring any error 3140/3141.

  TransMode::IsolationLevel ilForUpd;
  generator->verifyUpdatableTransMode(&gu->accessOptions(),
				      generator->getTransMode(),
				      &ilForUpd);
  DP2LockFlags lf;
  if (gu->accessOptions().userSpecified())
    lf = gu->accessOptions().getDP2LockFlags();
  else
    lf = generator->getTransMode()->getDP2LockFlags();

  // stable access with update/delete/insert are treated as
  // read committed.
  if (lf.getConsistencyLevel() == DP2LockFlags::STABLE)
    lf.setConsistencyLevel(DP2LockFlags::READ_COMMITTED);    
  
  if ((ilForUpd != TransMode::IL_NOT_SPECIFIED_) &&
      (NOT gu->accessOptions().userSpecified()))
    {
      TransMode t(ilForUpd);
      lf.setConsistencyLevel(
	   (DP2LockFlags::ConsistencyLevel)t.getDP2LockFlags().getConsistencyLevel());
      lf.setLockState(
	   (DP2LockFlags::LockState)t.getDP2LockFlags().getLockState());
    }

  return lf;
}

void GenericUpdate::setTransactionRequired(Generator *generator, 
					   NABoolean  isNeededForAllFragments)
{
  if (!generator->isInternalRefreshStatement() ||
       getIndexDesc()->getNAFileSet()->isAudited())
  {
    generator->setTransactionFlag(TRUE, isNeededForAllFragments);
  }
  else
  {
    // Internal refresh statement and table is non-audited.
    if (!getTableDesc()->getNATable()->isAnMV()  &&
         getTableName().getSpecialType() != ExtendedQualName::IUD_LOG_TABLE &&
         getTableName().getSpecialType() != ExtendedQualName::GHOST_IUD_LOG_TABLE)
    {
      generator->setTransactionFlag(TRUE, isNeededForAllFragments);
    }
  }
}

///////////////////////////////////////////////////////////
//
// DeleteCursor::codeGen()
//
///////////////////////////////////////////////////////////
short DeleteCursor::codeGen(Generator * generator)
{
  GenAssert(0, "DeleteCursor::codeGen:should not reach here.");

  return 0;
}

static short genUpdExpr(
        Generator * generator, 
        TableDesc * tableDesc,           // IN
        const IndexDesc * indexDesc,     // IN
        ValueIdArray &recExprArray,      // IN
        const Int32 updatedRowAtpIndex,    // IN
        ex_expr** updateExpr,            // OUT
        ULng32 &updateRowLen,     // OUT
        ExpTupleDesc** ufRowTupleDesc,   // OUT fetched/updated RowTupleDesc,
                                         // depending on updOpt (TRUE ->fetched)
        NABoolean updOpt)                // IN
{
  ExpGenerator * expGen = generator->getExpGenerator();

  ExpTupleDesc::TupleDataFormat tupleFormat = 
                   generator->getTableDataFormat( tableDesc->getNATable(), indexDesc);
  NABoolean alignedFormat = tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT;

  // Generate the update expression that will create the updated row
  // given to DP2 at runtime.
  ValueIdList updtRowVidList;
  BaseColumn *updtCol       = NULL,
             *fetchedCol    = NULL;
  Lng32        updtColNum    = -1,
              fetchedColNum = 0;
  ItemExpr   *updtColVal    = NULL,
             *castNode      = NULL;
  CollIndex   recEntries    = recExprArray.entries(),
              colEntries    = indexDesc->getIndexColumns().entries(),
              j             = 0;
  NAColumn   *col;
  NAColumnArray colArray;

  for (CollIndex i = 0; i < colEntries; i++) 
    {
      fetchedCol =
        (BaseColumn *)(((indexDesc->getIndexColumns())[i]).getItemExpr());
      fetchedColNum = fetchedCol->getColNumber();
      
      updtCol =
        (updtCol != NULL
         ? updtCol
         : (j < recEntries
            ? (BaseColumn *)(recExprArray[j].getItemExpr()->child(0)->castToItemExpr())
            : NULL));

      updtColNum = (updtCol ? updtCol->getColNumber() : -1);
      
      if (fetchedColNum == updtColNum)
        {
          updtColVal = recExprArray[j].getItemExpr()->child(1)->castToItemExpr();
          j++;
          updtCol = NULL;
        }
      else
        {
          updtColVal = fetchedCol;
        }
      
      ValueId updtValId = fetchedCol->getValueId();

      castNode = new(generator->wHeap()) Cast(updtColVal, &(updtValId.getType()));

      castNode->bindNode(generator->getBindWA());

      if (((updOpt) && (fetchedColNum == updtColNum)) ||
	  (NOT updOpt))
	{
	  if (updOpt)
	    {
	      // assign the attributes of the fetched col to the
	      // updated col.
	      generator->addMapInfo(
		   castNode->getValueId(),
		   generator->getMapInfo(fetchedCol->getValueId())->getAttr());
	    }

          if ( alignedFormat &&
               (col = updtValId.getNAColumn( TRUE )) &&
               (col != NULL) )
            colArray.insert( col );

	  updtRowVidList.insert(castNode->getValueId());
	}
    }     // for each column

  // Generate the update expression
  //
  if (NOT updOpt)
    {
      // Tell the expression generator that we're coming in for an insert
      // or an update.  This flag will be cleared in generateContigousMoveExpr.
      if ( tupleFormat == ExpTupleDesc::SQLMX_FORMAT ||
           tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT )
         expGen->setForInsertUpdate( TRUE );

      expGen->generateContiguousMoveExpr
	(updtRowVidList,
	 // (IN) Don't add convert nodes, Cast's have already been done.
	 0,
	 // (IN) Destination Atp
	 1, 
	 // (IN) Destination Atp index
	 updatedRowAtpIndex,
	 // (IN) Destination data format
	 tupleFormat,
	 // (OUT) Destination tuple length
	 updateRowLen,
	 // (OUT) Generated expression
	 updateExpr, 
	 // (OUT) Tuple descriptor for destination tuple
	 ufRowTupleDesc, 
	 // (IN) Tuple descriptor format
	 ExpTupleDesc::LONG_FORMAT,
         NULL, NULL, 0, NULL, NULL,
         &colArray);
    }
  else
    {
      // update opt being done. Fetched and updated row are exactly
      // the same. Updated values will overwrite the copy of fetched row
      // at runtime. Change the atp & atpindex for target.
      expGen->assignAtpAndAtpIndex(updtRowVidList,
				   1, updatedRowAtpIndex);

      // No need to generate a header clause since the entire fetched row
      // is copied to the updated row - header is in place.
      expGen->setNoHeaderNeeded( TRUE );
      
      // generate the update expression
      expGen->generateListExpr(updtRowVidList,ex_expr::exp_ARITH_EXPR,
			       updateExpr);

      // restore the header flag
      expGen->setNoHeaderNeeded( FALSE );
    }
  
  return 0;
}

// Used to generate update or insert constraint expressions for update operators
static short genUpdConstraintExpr(Generator * generator,
                                  ItemExpr * constrTree,
                                  const ValueIdSet & constraintColumns,
                                  ValueIdArray & targetRecExprArray,
                                  ex_expr ** targetExpr /* out */)
{
  ExpGenerator * expGen = generator->getExpGenerator();

  // The ValueIds in the constrTree refer to the source values of the columns.
  // Construct a ValueIdMap so we can rewrite the constrTree to refer to the
  // target value of the columns.

  ValueIdMap sourceToTarget;  // top values will be source, bottom will be target
 
  for (ValueId sourceValId = constraintColumns.init();
       constraintColumns.next(sourceValId);
       constraintColumns.advance(sourceValId))
    {
      GenAssert(sourceValId.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN,
      		"unexpected type of constraint expression column");
      NAColumn * sourceCol = ((IndexColumn*)sourceValId.getItemExpr())->getNAColumn();
      ValueId targetValId;
      for (CollIndex ni = 0; (ni < targetRecExprArray.entries()); ni++)
        {
          const ItemExpr *assignExpr = targetRecExprArray[ni].getItemExpr();
          targetValId = assignExpr->child(0)->castToItemExpr()->getValueId();
          NAColumn *targetCol = NULL;         
          if (targetValId.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
            targetCol = ((BaseColumn*)targetValId.getItemExpr())->getNAColumn();
          else if (targetValId.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN)
            targetCol = ((IndexColumn*)targetValId.getItemExpr())->getNAColumn();

          if (targetCol && sourceCol->getPosition() == targetCol->getPosition())
            {
              GenAssert(sourceCol->getNATable() == targetCol->getNATable(),
                        "expecting same NATable for constraint source and target");             

              // We found the target column matching the source column in the
              // targetRecExprArray. Now, an optimization: If the assignment
              // merely moves the old column value to the new, there is no need
              // to map it.

              ValueId rhsValId = assignExpr->child(1)->castToItemExpr()->getValueId();
              NAColumn *rhsCol = NULL;
              if (rhsValId.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
                rhsCol = ((BaseColumn*)rhsValId.getItemExpr())->getNAColumn();
              else if (rhsValId.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN)
                rhsCol = ((IndexColumn*)rhsValId.getItemExpr())->getNAColumn();

              if (rhsCol && rhsCol->getPosition() == targetCol->getPosition())
                {
                  // assignment copies old column value to target without change;
                  // no need to map
                  GenAssert(rhsCol->getNATable() == targetCol->getNATable(),
                            "expecting same NATable for assignment source and target");
                }
              else
                {
                  // the column value is changing (or maybe this is an insert),
                  // so map it
                  sourceToTarget.addMapEntry(sourceValId, targetValId);
                }
              ni = targetRecExprArray.entries();  // found it, no need to search further
            }
        }
    } 

  // If there is anything to map, rewrite the constraint expression
  // and generate it. If there is nothing to map, that means none of
  // the constraint expression columns is changed (which implies this
  // is an update expr and not an insert, by the way). In that case, we
  // don't need to generate the constraint expression as the constraint
  // should already be satisfied by the old values.

  if (sourceToTarget.entries() > 0)
    {
      // map the ValueIds in the constraint tree to target values
      ValueId mappedConstrTree;
      sourceToTarget.rewriteValueIdDown(constrTree->getValueId(),mappedConstrTree /* out */);

      // generate the expression
      expGen->generateExpr(mappedConstrTree, ex_expr::exp_SCAN_PRED,
                           targetExpr);
    }
  else
    {
      targetExpr = NULL;
    }

  return 0;
}

static short genHbaseUpdOrInsertExpr(
			     Generator * generator,
			     NABoolean isInsert,
			     ValueIdArray &updRecExprArray,  // IN
			     const Int32 updateTuppIndex,       // IN
			     ex_expr** updateExpr,        // OUT
			     ULng32 &updateRowLen, // OUT
			     ExpTupleDesc** updateTupleDesc,   // OUT updated RowTupleDesc,
			     Queue* &listOfUpdatedColNames, // OUT
			     ex_expr** mergeInsertRowIdExpr, // out
			     ULng32 &mergeInsertRowIdLen, // OUT
			     const Int32 mergeInsertRowIdTuppIndex, // IN
			     const IndexDesc * indexDesc) // IN
{
  ExpGenerator * expGen = generator->getExpGenerator();
  Space * space          = generator->getSpace();
 
  *updateExpr = NULL;
  updateRowLen = 0;

  // Generate the update expression that will create the updated row
  ValueIdList updRowVidList;

  NABoolean isAligned = FALSE;

  if (indexDesc->getNAFileSet()->isSqlmxAlignedRowFormat())
    isAligned = TRUE;

  ExpTupleDesc::TupleDataFormat tupleFormat;
  if (isAligned)
    tupleFormat = ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  else
    tupleFormat = ExpTupleDesc::SQLARK_EXPLODED_FORMAT;

  listOfUpdatedColNames = NULL;
  if (updRecExprArray.entries() > 0)
    listOfUpdatedColNames = new(space) Queue(space);

  NAColumnArray colArray;
  NAColumn *col;

  for (CollIndex ii = 0; ii < updRecExprArray.entries(); ii++)
    {
      const ItemExpr *assignExpr = updRecExprArray[ii].getItemExpr();
      ValueId assignExprValueId = assignExpr->getValueId();

      ValueId tgtValueId = assignExpr->child(0)->castToItemExpr()->getValueId();
      ValueId srcValueId = assignExpr->child(1)->castToItemExpr()->getValueId();

      // populate the colArray because this info is needed later to identify 
      // the added columns. 
      if ( isAligned )
      {
        col = tgtValueId.getNAColumn( TRUE );
        if ( col != NULL )
          colArray.insert( col );
      }

      ItemExpr * ie = NULL;

      ie = new(generator->wHeap())
	    Cast(assignExpr->child(1), &tgtValueId.getType());

      BaseColumn * bc = 
	(BaseColumn*)(updRecExprArray[ii].getItemExpr()->child(0)->castToItemExpr());
      
      GenAssert(bc->getOperatorType() == ITM_BASECOLUMN,
		"unexpected type of base table column");
      
      const NAColumn *nac = bc->getNAColumn();
      if ((NOT isAligned) && HbaseAccess::isEncodingNeededForSerialization(bc))
	{
	  ie = new(generator->wHeap()) CompEncode
	    (ie, FALSE, -1, CollationInfo::Sort, TRUE);
	}

      ie->bindNode(generator->getBindWA());
      updRowVidList.insert(ie->getValueId());

      if (NOT isAligned)
        {
          NAString cnInList;
          HbaseAccess::createHbaseColId(nac, cnInList);

          char * colNameInList = 
            space->AllocateAndCopyToAlignedSpace(cnInList, 0);
          
          listOfUpdatedColNames->insert(colNameInList);
        }
    }

  if ((isAligned) && (listOfUpdatedColNames) &&
      (updRecExprArray.entries() > 0))
    {
      NAString cnInList(SEABASE_DEFAULT_COL_FAMILY);
      cnInList += ":";
      unsigned char c = 1;
      cnInList.append((char*)&c, 1);
      short len = cnInList.length();
      cnInList.prepend((char*)&len, sizeof(short));
      
      char * colNameInList =
        space->AllocateAndCopyToAlignedSpace(cnInList, 0);
      
      listOfUpdatedColNames->insert(colNameInList);
    }

  // Generate the update expression
  //
  expGen->generateContiguousMoveExpr
    (updRowVidList,
     0, // (IN) Don't add convert nodes, Cast's have already been done.
     1, // (IN) Destination Atp
     updateTuppIndex, // (IN) Destination Atp index
     tupleFormat,
     updateRowLen,      // (OUT) Destination tuple length
     updateExpr,  // (OUT) Generated expression
     updateTupleDesc, // (OUT) Tuple descriptor for destination tuple
     ExpTupleDesc::LONG_FORMAT,
     NULL, NULL, 0, NULL, NULL,
     &colArray); // colArray is needed to identify any added cols.
  
  // Assign attributes to the ASSIGN nodes of the newRecExpArray()
  // This is not the same as the generateContiguousMoveExpr() call
  // above since different valueId's are added to the mapTable.
  // 

  // Assign attributes to the ASSIGN nodes of the updRecExpArray()
  // This is not the same as the generateContiguousMoveExpr() call
  // above since different valueId's are added to the mapTable.
  // 
  for (CollIndex ii = 0; ii < updRecExprArray.entries(); ii++)
    {
      const ItemExpr *assignExpr = updRecExprArray[ii].getItemExpr();
      ValueId assignExprValueId = assignExpr->getValueId();
      Attributes * assignAttr = (generator->addMapInfo(assignExprValueId, 0))->getAttr();

      ValueId updValId = updRowVidList[ii];
      Attributes * updValAttr = (generator->getMapInfo(updValId, 0))->getAttr();      
      assignAttr->copyLocationAttrs(updValAttr);
    }
  for (CollIndex ii = 0; ii < updRecExprArray.entries(); ii++)
    {
      const ItemExpr *assignExpr = updRecExprArray[ii].getItemExpr();
      ValueId assignExprValueId = assignExpr->getValueId();

      ValueId tgtValueId = assignExpr->child(0)->castToItemExpr()->getValueId();
      
      Attributes * colAttr = (generator->addMapInfo(tgtValueId, 0))->getAttr();
      Attributes * assignAttr = (generator->getMapInfo(assignExprValueId, 0))->getAttr();
      
      colAttr->copyLocationAttrs(assignAttr);

      BaseColumn * bc = (BaseColumn *)assignExpr->child(0)->castToItemExpr();
      const NAColumn *nac = bc->getNAColumn();
      if (nac->isAddedColumn())
	{
	  colAttr->setAddedCol(); 

	  Attributes::DefaultClass dc = expGen->getDefaultClass(nac);
	  colAttr->setDefaultClass(dc);

	  Attributes * attr = (*updateTupleDesc)->getAttr(ii);
	  attr->setAddedCol(); 
	  attr->setDefaultClass(dc);
	}

    }

  if ((isInsert) &&
      (updRowVidList.entries() > 0))
    {
      ValueIdList updRowKeyVidList;
      const NAColumnArray &keyColArray = indexDesc->getNAFileSet()->getIndexKeyColumns();
      ULng32 firstKeyColumnOffset = 0;

      for (CollIndex kc=0; kc<keyColArray.entries(); kc++)
        updRowKeyVidList.insert(updRowVidList[keyColArray[kc]->getPosition()]);

      expGen->generateKeyEncodeExpr(indexDesc,
				    1, // (IN) Destination Atp
				    mergeInsertRowIdTuppIndex,
				    ExpTupleDesc::SQLMX_KEY_FORMAT,
				    mergeInsertRowIdLen,
				    mergeInsertRowIdExpr,
				    FALSE,
				    firstKeyColumnOffset,
				    &updRowKeyVidList,
				    TRUE);
    }

  return 0;
}

//
// Create and bind an assign node for each vertical-partition column
// (i.e., a column in a partition of a VP table).  The assign node
// assigns the base table column to the VP column.
//
static void bindVPCols(Generator *generator, 
		       const ValueIdList & vpCols,
		       ValueIdList & resList)
{
  BindWA *bindWA = generator->getBindWA();

  for (CollIndex colNo=0; colNo<vpCols.entries(); colNo++) 
    {
      // Get the VP column -- must be ITM_INDEXCOLUMN
      IndexColumn *vpColItem = (IndexColumn*)vpCols[colNo].getItemExpr();
      GenAssert(vpColItem->getOperatorType() == ITM_INDEXCOLUMN,
      		"unexpected type of vp column");
      
      // Get the corresponding base table column -- must be ITM_BASECOLUMN
      ItemExpr *tblColItem = vpColItem->getDefinition().getItemExpr();
      GenAssert(tblColItem->getOperatorType() == ITM_BASECOLUMN,
		"unexpected type of base table column");
      
      Assign *assign = new (bindWA->wHeap()) Assign(vpColItem, tblColItem, FALSE);
      
      assign->bindNode(bindWA);
      if (bindWA->errStatus()) 
	{ 
	  GenAssert(0,"bindNode of vpCol failed");
	}
      
      resList.insertAt(colNo, assign->getValueId()); 
    }
}
  
short HiveInsert::codeGen(Generator *generator)
{
  if(!generator->explainDisabled()) {

    Space * space = generator->getSpace();

    // a dummy tdb
    ComTdbFastExtract* fe_tdb = new (space) ComTdbFastExtract();

    generator->setExplainTuple( addExplainInfo(fe_tdb, 0, 0, generator));
  }

  return 0;
}


///////////////////////////////////////////////////////////
//
// UpdateCursor::codeGen()
//
///////////////////////////////////////////////////////////
short UpdateCursor::codeGen(Generator * generator)
{
  GenAssert(0, "UpdateCursor::codeGen:should not reach here.");
			
  return 0;
}

//
// This function is for aligned row format only.
// This will order all the fixed fields by their alignment size,
// followed by any added fixed fields,
// followed by all variable fields (original or added).
static void orderColumnsByAlignment(NAArray<BaseColumn *>   columns,
                                    UInt32                  numColumns,
                                    NAArray<BaseColumn *> * orderedCols )
{
  Int16  rc = 0;
  NAList<BaseColumn *> varCols(STMTHEAP, 5);
  NAList<BaseColumn *> addedCols(STMTHEAP, 5);
  NAList<BaseColumn *> align4(STMTHEAP, 5);
  NAList<BaseColumn *> align2(STMTHEAP, 5);
  NAList<BaseColumn *> align1(STMTHEAP, 5);
  BaseColumn *currColumn;
  CollIndex i, k;
  Int32 alignmentSize;

  for( i = 0, k = 0; i <  numColumns; i++ )
  {
    if ( columns.used(i) )
    {
      currColumn = columns[ i ];

      if ( currColumn->getType().isVaryingLen() )
      {
        varCols.insert( currColumn );
      }
      else
      {
        if ( currColumn->getNAColumn()->isAddedColumn() )
        {
          addedCols.insert( currColumn );
          continue;
        }

        alignmentSize = currColumn->getType().getDataAlignment();

        if (8 == alignmentSize)
          orderedCols->insertAt(k++, currColumn );
        else if ( 4 == alignmentSize )
          align4.insert( currColumn );
        else if ( 2 == alignmentSize )
          align2.insert( currColumn );
        else
          align1.insert( currColumn );
      }
    }
  }

  if (align4.entries() > 0)
    for( i = 0; i < align4.entries(); i++ )
      orderedCols->insertAt( k++, align4[ i ] );

  if (align2.entries() > 0)
    for( i = 0; i < align2.entries(); i++ )
      orderedCols->insertAt( k++, align2[ i ] );

  if (align1.entries() > 0)
    for( i = 0; i < align1.entries(); i++ )
      orderedCols->insertAt( k++, align1[ i ] );

  if (addedCols.entries() > 0)
    for( i = 0; i < addedCols.entries(); i++ )
      orderedCols->insertAt( k++, addedCols[ i ] );

  if (varCols.entries() > 0)
    for( i = 0; i < varCols.entries(); i++ )
      orderedCols->insertAt( k++, varCols[ i ] );
}

short Delete::codeGen(Generator * /*generator*/)
{
  return -1;
}

short Insert::codeGen(Generator * /*generator*/)
{
  return -1;
}

short Update::codeGen(Generator * /*generator*/)
{
  return -1;
}

short MergeUpdate::codeGen(Generator * /*generator*/)
{
  return -1;
}

short MergeDelete::codeGen(Generator * /*generator*/)
{
  return -1;
}

short HbaseDelete::codeGen(Generator * generator)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  // allocate a map table for the retrieved columns
  //  generator->appendAtEnd();
  MapTable * last_map_table = generator->getLastMapTable();
 
  ex_expr *scanExpr = 0;
  ex_expr *proj_expr = 0;
  ex_expr *convert_expr = NULL;
  ex_expr * keyColValExpr = NULL;
  ex_expr *preCondExpr = NULL;
  ex_expr *lobExpr = NULL;

  ex_cri_desc * givenDesc 
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc = NULL;

  const Int32 work_atp = 1;
  const Int32 convertTuppIndex = 2;
  const Int32 rowIdTuppIndex = 3;
  const Int32 asciiTuppIndex = 4;
  const Int32 rowIdAsciiTuppIndex = 5;
  const Int32 keyColValTuppIndex = 6;

  ULng32 asciiRowLen = 0; 
  ExpTupleDesc * asciiTupleDesc = 0;

  ex_cri_desc * work_cri_desc = NULL;
  work_cri_desc = new(space) ex_cri_desc(7, space);

  returnedDesc = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);

  NABoolean returnRow = getReturnRow(this, getIndexDesc());

  NABoolean isAlignedFormat = getTableDesc()->getNATable()->isAlignedFormat(getIndexDesc());
  NABoolean isHbaseMapFormat = getTableDesc()->getNATable()->isHbaseMapTable();

  ExpTupleDesc::TupleDataFormat asciiRowFormat = 
    (isAlignedFormat ?
     ExpTupleDesc::SQLMX_ALIGNED_FORMAT :
     ExpTupleDesc::SQLARK_EXPLODED_FORMAT);
  ExpTupleDesc::TupleDataFormat hbaseRowFormat = 
    ExpTupleDesc::SQLARK_EXPLODED_FORMAT;

  ValueIdList asciiVids;
  ValueIdList executorPredCastVids;
  ValueIdList convertExprCastVids;

  NABoolean addDefaultValues = TRUE;
  NABoolean hasAddedColumns = FALSE;
  if (getTableDesc()->getNATable()->hasAddedColumn())
    hasAddedColumns = TRUE;

  ValueIdList columnList;
  ValueIdList srcVIDlist;
  ValueIdList dupVIDlist;
  HbaseAccess::sortValues(retColRefSet_, 
			  columnList,
			  srcVIDlist, dupVIDlist,
			  (getTableDesc()->getNATable()->getExtendedQualName().getSpecialType() == ExtendedQualName::INDEX_TABLE));

  const CollIndex numColumns = columnList.entries();

  if (! getPrecondition().isEmpty())
    {
      ItemExpr * preCondTree = getPrecondition().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(preCondTree->getValueId(), ex_expr::exp_SCAN_PRED,
			   &preCondExpr);
    }

  // build key information
  keyRangeGen * keyInfo = 0;
  expGen->buildKeyInfo(&keyInfo, // out
		       generator,
		       getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
		       getIndexDesc()->getIndexKey(),
		       getBeginKeyPred(),
		       (getSearchKey() && getSearchKey()->isUnique() ? NULL : getEndKeyPred()),
		       getSearchKey(),
		       NULL, //getMdamKeyPtr(),
		       FALSE,
		       ExpTupleDesc::SQLMX_KEY_FORMAT);

  UInt32 keyColValLen = 0;
  char * keyColName = NULL;
  if ((canDoCheckAndUpdel()) &&
      (getSearchKey() && getSearchKey()->isUnique()) &&
      (getBeginKeyPred().entries() > 0))
    {
      expGen->generateKeyColValueExpr(
				      getBeginKeyPred()[0],
				      work_atp, keyColValTuppIndex,
				      keyColValLen,
				      &keyColValExpr);

      if (! keyColValExpr)
	canDoCheckAndUpdel() = FALSE;
      else
	{
	  ItemExpr * col_node = getBeginKeyPred()[0].getItemExpr()->child(0);
	  HbaseAccess::genColName(generator, col_node, keyColName);
	}
    }

  Queue * tdbListOfUniqueRows = NULL;
  Queue * tdbListOfRangeRows = NULL;

  HbaseAccess::genListsOfRows(generator,
			      listOfDelSubsetRows_,
			      listOfDelUniqueRows_,
			      tdbListOfRangeRows,
			      tdbListOfUniqueRows);

  ULng32 convertRowLen = 0;

  ValueIdList lobDelVIDlist;
  for (CollIndex ii = 0; ii < numColumns; ii++)
    {
      ItemExpr * col_node = ((columnList[ii]).getValueDesc())->getItemExpr();
      
      const NAType &givenType = col_node->getValueId().getType();
      int res;    
      ItemExpr *asciiValue = NULL;
      ItemExpr *castValue = NULL;
      
      res = HbaseAccess::createAsciiColAndCastExpr2(
						    generator,        // for heap
						    col_node,
						    givenType,         // [IN] Actual type of HDFS column
						    asciiValue,         // [OUT] Returned expression for ascii rep.
						    castValue,        // [OUT] Returned expression for binary rep.
						    isAlignedFormat 
						    );
      
      GenAssert(res == 1 && asciiValue != NULL && castValue != NULL,
		"Error building expression tree for cast output value");
      asciiValue->synthTypeAndValueId();
      asciiValue->bindNode(generator->getBindWA());
      asciiVids.insert(asciiValue->getValueId());
      
      castValue->bindNode(generator->getBindWA());
      convertExprCastVids.insert(castValue->getValueId());

      if (col_node->getValueId().getType().isLob())
        {
          ItemExpr * ld = new(generator->wHeap())
            LOBdelete(castValue);
          ld->bindNode(generator->getBindWA());
          lobDelVIDlist.insert(ld->getValueId());
        }
      
    } // for (ii = 0; ii < numCols; ii++)

  // Add ascii columns to the MapTable. After this call the MapTable
  // has ascii values in the work ATP at index asciiTuppIndex.
  const NAColumnArray * colArray = NULL;
  unsigned short pcm = expGen->getPCodeMode();
  if ((asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) &&
      (hasAddedColumns))
    {
      colArray = &getIndexDesc()->getAllColumns();

      expGen->setPCodeMode(ex_expr::PCODE_NONE);
    }

  expGen->processValIdList(
			   asciiVids,                             // [IN] ValueIdList
			   asciiRowFormat,                        // [IN] tuple data format
			   asciiRowLen,                           // [OUT] tuple length 
			   work_atp,                              // [IN] atp number
			   asciiTuppIndex,                        // [IN] index into atp
			   &asciiTupleDesc,                       // [optional OUT] tuple desc
			   ExpTupleDesc::LONG_FORMAT,             // [optional IN] desc format
			   0,
			   NULL,
			   (NAColumnArray*)colArray);
   
  work_cri_desc->setTupleDescriptor(asciiTuppIndex, asciiTupleDesc);
  
  ExpTupleDesc * tuple_desc = 0;
  
  expGen->generateContiguousMoveExpr(
				     convertExprCastVids,              // [IN] source ValueIds
				     FALSE,                                // [IN] add convert nodes?
				     work_atp,                             // [IN] target atp number
				     convertTuppIndex,                 // [IN] target tupp index
				     hbaseRowFormat,                        // [IN] target tuple format
				     convertRowLen,             // [OUT] target tuple length
				     &convert_expr,                // [OUT] move expression
				     &tuple_desc,                     // [optional OUT] target tuple desc
				     ExpTupleDesc::LONG_FORMAT,       // [optional IN] target desc format
				     NULL,
				     NULL,
				     0,
				     NULL,
				     FALSE,
				     NULL,
				     FALSE /* doBulkMove */);
  
  if ((asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) &&
      (hasAddedColumns))
    {
      expGen->setPCodeMode(pcm);
    }

  for (CollIndex i = 0; i < columnList.entries(); i++) 
    {
      ValueId colValId = columnList[i];
      ValueId castValId = convertExprCastVids[i];
      
      Attributes * colAttr = (generator->addMapInfo(colValId, 0))->getAttr();
      Attributes * castAttr = (generator->getMapInfo(castValId))->getAttr();
      
      colAttr->copyLocationAttrs(castAttr);
    } // for

  if (getScanIndexDesc() != NULL) 
    {
      for (CollIndex i = 0; i < getScanIndexDesc()->getIndexColumns().entries(); i++)
	{
	  ValueId scanIndexDescVID = getScanIndexDesc()->getIndexColumns()[i];
	  const ValueId indexDescVID = getIndexDesc()->getIndexColumns()[i];
	  
	  CollIndex pos = 0;

	  pos = columnList.index(indexDescVID);
	  if (pos != NULL_COLL_INDEX)
	    {
	      Attributes * colAttr = (generator->addMapInfo(scanIndexDescVID, 0))->getAttr();
	      
	      ValueId castValId = convertExprCastVids[pos];
	      Attributes * castAttr = (generator->getMapInfo(castValId))->getAttr();
	      
	      colAttr->copyLocationAttrs(castAttr);
	    } // if
	  else
	    {
	      pos = columnList.index(scanIndexDescVID);
	      if (pos != NULL_COLL_INDEX)
		{
		  Attributes * colAttr = (generator->addMapInfo(indexDescVID, 0))->getAttr();
		  
		  ValueId castValId = convertExprCastVids[pos];
		  Attributes * castAttr = (generator->getMapInfo(castValId))->getAttr();
		  
		  colAttr->copyLocationAttrs(castAttr);
		} // if
	    } // else
	  
	} // for
    } // getScanIndexDesc != NULL

  // assign location attributes to dup vids that were returned earlier.
  for (CollIndex i = 0; i < srcVIDlist.entries(); i++) 
    {
      ValueId srcValId = srcVIDlist[i];
      ValueId dupValId = dupVIDlist[i];
      
      Attributes * srcAttr = (generator->getMapInfo(srcValId))->getAttr();
      Attributes * dupAttr = (generator->addMapInfo(dupValId, 0))->getAttr();
      
      dupAttr->copyLocationAttrs(srcAttr);
    } // for

  if (addDefaultValues) //hasAddedColumns)
    {
      expGen->addDefaultValues(columnList,
		       getIndexDesc()->getAllColumns(),
		       tuple_desc,
		       TRUE);

      if (asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
        {
          expGen->addDefaultValues(columnList,
                                   getIndexDesc()->getAllColumns(),
                                   asciiTupleDesc,
                                   TRUE);
        }
      else
        {
          // copy default values from convertTupleDesc to asciiTupleDesc
          expGen->copyDefaultValues(asciiTupleDesc, tuple_desc);
        }
    }

  // generate explain selection expression, if present
  //  if ((NOT (getTableDesc()->getNATable()->getExtendedQualName().getSpecialType() == ExtendedQualName::INDEX_TABLE)) &&
  //      (! executorPred().isEmpty()))
  if (! executorPred().isEmpty())
    {
      ItemExpr * newPredTree = executorPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			   &scanExpr);
    }

  ex_expr * lobDelExpr = NULL;
  if (getTableDesc()->getNATable()->hasLobColumn())
    {
      // generate code to delete rows from LOB desc table
      expGen->generateListExpr(lobDelVIDlist, 
                               ex_expr::exp_ARITH_EXPR, &lobDelExpr);
    }

  ULng32 rowIdAsciiRowLen = 0; 
  ExpTupleDesc * rowIdAsciiTupleDesc = 0;
  ex_expr * rowIdExpr = NULL;
  ULng32 rowIdLength = 0;
  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      // dont encode keys for hbase mapped tables since these tables
      // could be populated from outside of traf.
      NABoolean encodeKeys = TRUE;
      if (getTableDesc()->getNATable()->isHbaseMapTable())
        encodeKeys = FALSE;

      HbaseAccess::genRowIdExpr(generator,
				getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
				getHbaseSearchKeys(), 
				work_cri_desc, work_atp,
				rowIdAsciiTuppIndex, rowIdTuppIndex,
				rowIdAsciiRowLen, rowIdAsciiTupleDesc,
				rowIdLength, 
				rowIdExpr,
                                encodeKeys);
    }
  else
    {
      HbaseAccess::genRowIdExprForNonSQ(generator,
					getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
					getHbaseSearchKeys(), 
					work_cri_desc, work_atp,
					rowIdAsciiTuppIndex, rowIdTuppIndex,
					rowIdAsciiRowLen, rowIdAsciiTupleDesc,
					rowIdLength, 
					rowIdExpr);
    }
  
  Queue * listOfFetchedColNames = NULL;
  if (isAlignedFormat)
    {
      listOfFetchedColNames = new(space) Queue(space);
      
      NAString cnInList(SEABASE_DEFAULT_COL_FAMILY);
      cnInList += ":";
      unsigned char c = 1;
      cnInList.append((char*)&c, 1);
      short len = cnInList.length();
      cnInList.prepend((char*)&len, sizeof(short));
      
      char * colNameInList =
        space->AllocateAndCopyToAlignedSpace(cnInList, 0);
      
      listOfFetchedColNames->insert(colNameInList);
    }
  else
    {
      HbaseAccess::genListOfColNames(generator,
				     getIndexDesc(),
				     columnList,
				     listOfFetchedColNames);
    }

  Queue * listOfDeletedColNames = NULL;
  if (csl())
    {
      listOfDeletedColNames = new(space) Queue(space);
      for (Lng32 i = 0; i < csl()->entries(); i++)
	{
	  NAString * nas = (NAString*)(*csl())[i];
	  char * colNameInList = NULL;
	  
	  short len = nas->length();
	  nas->prepend((char*)&len, sizeof(short));

	  colNameInList = 
	    space->AllocateAndCopyToAlignedSpace(*nas, 0);
	  
	  listOfDeletedColNames->insert(colNameInList);
	}
    }

  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      if ((keyInfo && getSearchKey() && getSearchKey()->isUnique()) ||
	  (tdbListOfUniqueRows))
	{
	  // Save node for later use by RelRoot in the case of UPDATE CURRENT OF.
	  generator->updateCurrentOfRel() = (void*)this;
	}
    }

  if (getOptStoi() && getOptStoi()->getStoi())
    generator->addSqlTableOpenInfo(getOptStoi()->getStoi());

  LateNameInfo* lateNameInfo = new(generator->wHeap()) LateNameInfo();
  char * compileTimeAnsiName = (char*)getOptStoi()->getStoi()->ansiName();

  lateNameInfo->setCompileTimeName(compileTimeAnsiName, space);
  lateNameInfo->setLastUsedName(compileTimeAnsiName, space);
  lateNameInfo->setNameSpace(COM_TABLE_NAME);
  if (getIndexDesc()->getNAFileSet()->getKeytag() != 0)
    // is an index.
    {
      lateNameInfo->setIndex(TRUE);
      lateNameInfo->setNameSpace(COM_INDEX_NAME);
    }
  generator->addLateNameInfo(lateNameInfo);
 
  if (returnRow)
    {
      // The hbase row will be returned as the last entry of the returned atp.
      // Change the atp and atpindex of the returned values to indicate that.
      expGen->assignAtpAndAtpIndex(getIndexDesc()->getIndexColumns(),
				   0, returnedDesc->noTuples()-1);

      expGen->assignAtpAndAtpIndex(getScanIndexDesc()->getIndexColumns(),
				   0, returnedDesc->noTuples()-1);
    }

  Cardinality expectedRows = (Cardinality) getEstRowsUsed().getValue();
  ULng32 buffersize = getDefault(GEN_DPSO_BUFFER_SIZE);
  buffersize = MAXOF(3*convertRowLen, buffersize);
  queue_index upqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_UP);
  queue_index downqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_DOWN);
  Int32 numBuffers = getDefault(GEN_DPUO_NUM_BUFFERS);

  char * tablename = NULL;
  if ((getTableDesc()->getNATable()->isHbaseRowTable()) ||
      (getTableDesc()->getNATable()->isHbaseCellTable()) ||
      (getTableName().getQualifiedNameObj().isHbaseMappedName()))
    {
      if (getIndexDesc() && getIndexDesc()->getNAFileSet())
	tablename = space->AllocateAndCopyToAlignedSpace(GenGetQualifiedName(getIndexDesc()->getNAFileSet()->getFileSetName().getObjectName()), 0);
    }
  else
    {
      if (getIndexDesc() && getIndexDesc()->getNAFileSet())
	tablename = space->AllocateAndCopyToAlignedSpace(GenGetQualifiedName(getIndexDesc()->getNAFileSet()->getFileSetName()), 0);
    }

  if (! tablename)
    tablename = 
      space->AllocateAndCopyToAlignedSpace(
					   GenGetQualifiedName(getTableName()), 0);

  NAString serverNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_SERVER);
  NAString zkPortNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_ZOOKEEPER_PORT);
  char * server = space->allocateAlignedSpace(serverNAS.length() + 1);
  strcpy(server, serverNAS.data());
  char * zkPort = space->allocateAlignedSpace(zkPortNAS.length() + 1);
  strcpy(zkPort, zkPortNAS.data());

  ComTdbHbaseAccess::HbasePerfAttributes * hbpa =
    new(space) ComTdbHbaseAccess::HbasePerfAttributes();
  if (CmpCommon::getDefault(HBASE_CACHE_BLOCKS) != DF_OFF)
    hbpa->setCacheBlocks(TRUE);
  // estrowsaccessed is 0 for now, so cache size will be set to minimum
  generator->setHBaseNumCacheRows(getEstRowsAccessed().getValue(), hbpa,rowIdAsciiRowLen) ;

  // create hdfsscan_tdb
  ComTdbHbaseAccess *hbasescan_tdb = new(space) 
    ComTdbHbaseAccess(
		      ComTdbHbaseAccess::DELETE_,
		      tablename,

		      convert_expr,
		      scanExpr,
		      rowIdExpr,
		      NULL, // updateExpr
		      lobDelExpr, // NULL, // mergeInsertExpr
		      NULL, // mergeInsertRowIdExpr
		      NULL, // mergeUpdScanExpr
		      NULL, // projExpr
		      NULL, // returnedUpdatedExpr
		      NULL, // returnMergeUpdateExpr
		      NULL, // encodedKeyExpr
		      keyColValExpr,
		      NULL, // hbaseFilterValExpr

		      asciiRowLen,
		      convertRowLen,
		      0, // updateRowLen
		      0, // mergeInsertRowLen
		      0, // fetchedRowLen
		      0, // returnedRowLen

		      rowIdLength,
		      convertRowLen,
		      rowIdAsciiRowLen,
		      (keyInfo ? keyInfo->getKeyLength() : 0),
		      keyColValLen,
		      0, // hbaseFilterValRowLen

		      asciiTuppIndex,
		      convertTuppIndex,
		      0, // updateTuppIndex
		      0, // mergeInsertTuppIndex
		      0, // mergeInsertRowIdTuppIndex
		      0, // mergeIUDIndicatorTuppIndex
		      0, // returnedFetchedTuppIndex
		      0, // returnedUpdatedTuppIndex

		      rowIdTuppIndex,
		      returnedDesc->noTuples()-1,
		      rowIdAsciiTuppIndex,
		      0, // keyTuppIndex,
		      keyColValTuppIndex,
		      0, // hbaseFilterValTuppIndex

                      0, // hbaseTimestamp
                      0, // hbaseVersion

		      tdbListOfRangeRows,
		      tdbListOfUniqueRows,
		      listOfFetchedColNames,
		      listOfDeletedColNames,
		      NULL,

		      keyInfo,
		      keyColName,

		      work_cri_desc,
		      givenDesc,
		      returnedDesc,
		      downqueuelength,
		      upqueuelength,
                      expectedRows,
		      numBuffers,
		      buffersize,

		      server,
                      zkPort,
		      hbpa
		      );

  generator->initTdbFields(hbasescan_tdb);

  if ((CmpCommon::getDefault(HBASE_ASYNC_OPERATIONS) == DF_ON)
           && getInliningInfo().isIMGU())
     hbasescan_tdb->setAsyncOperations(TRUE);

  if (getTableDesc()->getNATable()->isHbaseRowTable()) //rowwiseHbaseFormat())
    hbasescan_tdb->setRowwiseFormat(TRUE);

  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      hbasescan_tdb->setSQHbaseTable(TRUE);

      if (isAlignedFormat)
        hbasescan_tdb->setAlignedFormat(TRUE);

      if (isHbaseMapFormat)
        {
          hbasescan_tdb->setHbaseMapTable(TRUE);

          if (getTableDesc()->getNATable()->getClusteringIndex()->hasSingleColVarcharKey())
            hbasescan_tdb->setKeyInVCformat(TRUE);
        }

      if ((CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON) &&
	  (NOT noCheck()))
	hbasescan_tdb->setHbaseSqlIUD(TRUE);

      if (getTableDesc()->getNATable()->isEnabledForDDLQI())
        generator->objectUids().insert(
          getTableDesc()->getNATable()->objectUid().get_value());
    }

  if (keyInfo && getSearchKey() && getSearchKey()->isUnique())
    hbasescan_tdb->setUniqueKeyInfo(TRUE);

  if (returnRow)
    hbasescan_tdb->setReturnRow(TRUE);

  if (rowsAffected() != GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED)
    hbasescan_tdb->setComputeRowsAffected(TRUE);

  if (! tdbListOfUniqueRows)
    {
      hbasescan_tdb->setSubsetOper(TRUE);
    }

  if (canDoCheckAndUpdel())
    hbasescan_tdb->setCanDoCheckAndUpdel(TRUE);
  
  if (uniqueRowsetHbaseOper()) {
    hbasescan_tdb->setRowsetOper(TRUE);
    hbasescan_tdb->setHbaseRowsetVsbbSize(getDefault(HBASE_ROWSET_VSBB_SIZE));
  }

  if (csl())
    hbasescan_tdb->setUpdelColnameIsStr(TRUE);

  if (preCondExpr)
    hbasescan_tdb->setInsDelPreCondExpr(preCondExpr);

  if (generator->isTransactionNeeded())
    setTransactionRequired(generator);
  else if (noDTMxn())
    hbasescan_tdb->setUseHbaseXn(TRUE);
  else if (useRegionXn())
    hbasescan_tdb->setUseRegionXn(TRUE);

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(hbasescan_tdb, 0, 0, generator));
  }

  if ((generator->computeStats()) && 
      (generator->collectStatsType() == ComTdb::PERTABLE_STATS
      || generator->collectStatsType() == ComTdb::OPERATOR_STATS))
    {
      hbasescan_tdb->setPertableStatsTdbId((UInt16)generator->
					   getPertableStatsTdbId());
    }

  generator->setFoundAnUpdate(TRUE);

  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);
  generator->setGenObj(this, hbasescan_tdb);

 return 0;
}

short HbaseUpdate::codeGen(Generator * generator)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  // allocate a map table for the retrieved columns
  //  generator->appendAtEnd();
  MapTable * last_map_table = generator->getLastMapTable();
 
  // Append a new map table for holding attributes that are only used
  // in local expressions. This map table will be removed after all
  // the local expressions are generated.
  //
  MapTable *localMapTable = generator->appendAtEnd();

  ex_expr *scanExpr = 0;
  ex_expr *projExpr = 0;
  ex_expr *convert_expr = NULL;
  ex_expr *updateExpr = NULL;
  ex_expr *mergeInsertExpr = NULL;
  ex_expr *returnUpdateExpr = NULL;
  ex_expr * keyColValExpr = NULL;
  ex_expr * insConstraintExpr  = NULL;
  ex_expr * updConstraintExpr  = NULL;

  ex_cri_desc * givenDesc 
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc = NULL;

  const Int32 work_atp = 1;
  const Int32 convertTuppIndex = 2;
  const Int32 rowIdTuppIndex = 3;
  const Int32 asciiTuppIndex = 4;
  const Int32 rowIdAsciiTuppIndex = 5;
  //  const Int32 keyTuppIndex = 6;
  const Int32 updateTuppIndex = 6;
  const Int32 mergeInsertTuppIndex = 7;
  const Int32 mergeInsertRowIdTuppIndex = 8;
  const Int32 keyColValTuppIndex = 9;
  Int32 mergeIUDIndicatorTuppIndex = 0;
  // Do not use 10 as the next available tupp index. Please use 11 next
  // The 10th tuple index is used by merge statement below.

  Attributes * iudIndicatorAttr = NULL;

  ULng32 asciiRowLen = 0; 
  ExpTupleDesc * asciiTupleDesc = 0;

  ex_cri_desc * work_cri_desc = NULL;
  work_cri_desc = new(space) ex_cri_desc(11, space);

  if (getProducedMergeIUDIndicator() != NULL_VALUE_ID) 
  {
    mergeIUDIndicatorTuppIndex = 10;
    iudIndicatorAttr = 
      (generator->addMapInfo(getProducedMergeIUDIndicator(), 0))->getAttr();
    iudIndicatorAttr->setAtpIndex(mergeIUDIndicatorTuppIndex);
    iudIndicatorAttr->setAtp(work_atp);
    ULng32 iudIndicatorLen;
    ExpTupleDesc::computeOffsets(iudIndicatorAttr,
				 ExpTupleDesc::SQLARK_EXPLODED_FORMAT, 
				 iudIndicatorLen);
    ExpTupleDesc  *iudIndicatorTupleDesc = NULL;
    iudIndicatorTupleDesc = new(generator->getSpace()) 
      ExpTupleDesc(1, // numAttrs
		   &iudIndicatorAttr, // **attrs
		   iudIndicatorLen, // data length
		   ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
		   ExpTupleDesc::LONG_FORMAT,
		   generator->getSpace());
    work_cri_desc->setTupleDescriptor(mergeIUDIndicatorTuppIndex,
				      iudIndicatorTupleDesc);

  }

  NABoolean returnRow = getReturnRow(this, getIndexDesc());

  if (returnRow)
    // one for fetchedRow, one for updatedRow.
    returnedDesc = new(space) ex_cri_desc(givenDesc->noTuples() + 2, space);
  else
    returnedDesc = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);
 
  const Int16 returnedFetchedTuppIndex = (Int16)(returnedDesc->noTuples()-2);
  const Int16 returnedUpdatedTuppIndex = (Int16)(returnedFetchedTuppIndex + 1);

  NABoolean isAlignedFormat = getTableDesc()->getNATable()->isAlignedFormat(getIndexDesc());
  
  ExpTupleDesc::TupleDataFormat asciiRowFormat = 
    (isAlignedFormat ?
     ExpTupleDesc::SQLMX_ALIGNED_FORMAT :
     ExpTupleDesc::SQLARK_EXPLODED_FORMAT);

  ExpTupleDesc::TupleDataFormat hbaseRowFormat = 
    //    ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
    ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
    
  ValueIdList asciiVids;
  ValueIdList executorPredCastVids;
  ValueIdList convertExprCastVids;

  NABoolean addDefaultValues = TRUE;
  NABoolean hasAddedColumns = FALSE;
  if (getTableDesc()->getNATable()->hasAddedColumn())
    hasAddedColumns = TRUE;

  ValueIdList columnList;
  ValueIdList srcVIDlist;
  ValueIdList dupVIDlist;
  HbaseAccess::sortValues(retColRefSet_, columnList,
                          srcVIDlist, dupVIDlist,
			  (getTableDesc()->getNATable()->getExtendedQualName().getSpecialType() == ExtendedQualName::INDEX_TABLE));

  const CollIndex numColumns = columnList.entries();

 // build key information
  keyRangeGen * keyInfo = 0;
  expGen->buildKeyInfo(&keyInfo, // out
		       generator,
		       getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
		       getIndexDesc()->getIndexKey(),
		       getBeginKeyPred(),
		       (getSearchKey() && getSearchKey()->isUnique() ? NULL : getEndKeyPred()),
		       getSearchKey(),
		       NULL, //getMdamKeyPtr(),
		       FALSE,
		       ExpTupleDesc::SQLMX_KEY_FORMAT);

  UInt32 keyColValLen = 0;
  char * keyColName = NULL;
  if ((canDoCheckAndUpdel()) &&
      (getSearchKey() && getSearchKey()->isUnique()) &&
      (getBeginKeyPred().entries() > 0))
    {
      expGen->generateKeyColValueExpr(
				      getBeginKeyPred()[0],
				      work_atp, keyColValTuppIndex,
				      keyColValLen,
				      &keyColValExpr);
      if (! keyColValExpr)
	canDoCheckAndUpdel() = FALSE;
      else
	{
	  ItemExpr * col_node = getBeginKeyPred()[0].getItemExpr()->child(0);
	  HbaseAccess::genColName(generator, col_node, keyColName);
	}
    }

  Queue * tdbListOfUniqueRows = NULL;
  Queue * tdbListOfRangeRows = NULL;

  HbaseAccess::genListsOfRows(generator,
			      listOfUpdSubsetRows_,
			      listOfUpdUniqueRows_,
			      tdbListOfRangeRows,
			      tdbListOfUniqueRows);

  ULng32 convertRowLen = 0;

  for (CollIndex ii = 0; ii < numColumns; ii++)
    {
      ItemExpr * col_node = ((columnList[ii]).getValueDesc())->getItemExpr();
      
      const NAType &givenType = col_node->getValueId().getType();
      int res;    
      ItemExpr *asciiValue = NULL;
      ItemExpr *castValue = NULL;
      
      res = HbaseAccess::createAsciiColAndCastExpr2(
						   generator,        // for heap
						   col_node,
						   givenType,         // [IN] Actual type of HDFS column
						   asciiValue,         // [OUT] Returned expression for ascii rep.
						   castValue,        // [OUT] Returned expression for binary rep.
                                                   isAlignedFormat
						   );
      
      GenAssert(res == 1 && asciiValue != NULL && castValue != NULL,
		"Error building expression tree for cast output value");
      asciiValue->synthTypeAndValueId();
      asciiValue->bindNode(generator->getBindWA());
      asciiVids.insert(asciiValue->getValueId());
      
      castValue->bindNode(generator->getBindWA());
      convertExprCastVids.insert(castValue->getValueId());
    } // for (ii = 0; ii < numCols; ii++)
  
  // Add ascii columns to the MapTable. After this call the MapTable
  // has ascii values in the work ATP at index asciiTuppIndex.
  const NAColumnArray * colArray = NULL;
  unsigned short pcm = expGen->getPCodeMode();
  if ((asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) &&
      (hasAddedColumns))
    {
      colArray = &getIndexDesc()->getAllColumns();
      
      expGen->setPCodeMode(ex_expr::PCODE_NONE);
    }
  
  expGen->processValIdList(
			   asciiVids,                             // [IN] ValueIdList
			   asciiRowFormat,                        // [IN] tuple data format
			   asciiRowLen,                           // [OUT] tuple length 
			   work_atp,                              // [IN] atp number
			   asciiTuppIndex,                        // [IN] index into atp
			   &asciiTupleDesc,                       // [optional OUT] tuple desc
			   ExpTupleDesc::LONG_FORMAT,             // [optional IN] desc format
                           0,
                           NULL,
                           (NAColumnArray*)colArray);
    
  work_cri_desc->setTupleDescriptor(asciiTuppIndex, asciiTupleDesc);
  
  ExpTupleDesc * tuple_desc = 0;
  
  expGen->generateContiguousMoveExpr(
				     convertExprCastVids,              // [IN] source ValueIds
				     FALSE,                                // [IN] add convert nodes?
				     work_atp,                             // [IN] target atp number
				     convertTuppIndex,                 // [IN] target tupp index
				     hbaseRowFormat,                        // [IN] target tuple format
				     convertRowLen,             // [OUT] target tuple length
				     &convert_expr,                // [OUT] move expression
				     &tuple_desc,                     // [optional OUT] target tuple desc
				     ExpTupleDesc::LONG_FORMAT,       // [optional IN] target desc format
				     NULL,
				     NULL,
				     0,
				     NULL,
				     FALSE,
				     NULL,
				     FALSE /* doBulkMove */);
  
  if ((asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) &&
      (hasAddedColumns))
    {
      expGen->setPCodeMode(pcm);
    }

  for (CollIndex i = 0; i < columnList.entries(); i++) 
    {
      ValueId colValId = columnList[i];
      ValueId castValId = convertExprCastVids[i];
      
      Attributes * colAttr = (generator->addMapInfo(colValId, 0))->getAttr();
      Attributes * castAttr = (generator->getMapInfo(castValId))->getAttr();
      
      colAttr->copyLocationAttrs(castAttr);
    } // for
  
  if (getScanIndexDesc() != NULL) 
    {
      for (CollIndex i = 0; i < getScanIndexDesc()->getIndexColumns().entries(); i++)
	{
	  ValueId scanIndexDescVID = getScanIndexDesc()->getIndexColumns()[i];
	  const ValueId indexDescVID = getIndexDesc()->getIndexColumns()[i];
	  
	  CollIndex pos = 0;

	  pos = columnList.index(indexDescVID);
	  if (pos != NULL_COLL_INDEX)
	    {
	      Attributes * colAttr = (generator->addMapInfo(scanIndexDescVID, 0))->getAttr();
	      
	      ValueId castValId = convertExprCastVids[pos];
	      Attributes * castAttr = (generator->getMapInfo(castValId))->getAttr();
	      
	      colAttr->copyLocationAttrs(castAttr);
	    } // if
	  else
	    {
	      pos = columnList.index(scanIndexDescVID);
	      if (pos != NULL_COLL_INDEX)
		{
		  Attributes * colAttr = (generator->addMapInfo(indexDescVID, 0))->getAttr();
		  
		  ValueId castValId = convertExprCastVids[pos];
		  Attributes * castAttr = (generator->getMapInfo(castValId))->getAttr();
		  
		  colAttr->copyLocationAttrs(castAttr);
		} // if
	    } // else
	  
	} // for
    } // getScanIndexDesc != NULL

  // assign location attributes to dup vids that were returned earlier.
  for (CollIndex i = 0; i < srcVIDlist.entries(); i++) 
    {
      ValueId srcValId = srcVIDlist[i];
      ValueId dupValId = dupVIDlist[i];
      
      Attributes * srcAttr = (generator->getMapInfo(srcValId))->getAttr();
      Attributes * dupAttr = (generator->addMapInfo(dupValId, 0))->getAttr();
      
      dupAttr->copyLocationAttrs(srcAttr);
    } // for

  if (addDefaultValues) 
    {
      expGen->addDefaultValues(columnList,
			       getIndexDesc()->getAllColumns(),
			       tuple_desc,
			       TRUE); 

      if (asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
        {
          expGen->addDefaultValues(columnList,
                                   getIndexDesc()->getAllColumns(),
                                   asciiTupleDesc,
                                   TRUE);
        }
      else
        {
          // copy default values from convertTupleDesc to asciiTupleDesc
          expGen->copyDefaultValues(asciiTupleDesc, tuple_desc);
        }
    }

  // generate explain selection expression, if present
  //  if ((NOT (getTableDesc()->getNATable()->getExtendedQualName().getSpecialType() == ExtendedQualName::INDEX_TABLE)) &&
  //      (! executorPred().isEmpty()))
  if (! executorPred().isEmpty())
    {
      ItemExpr * newPredTree = executorPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			   &scanExpr);
    }

  ex_expr * mergeInsertRowIdExpr = NULL;
  ULng32 mergeInsertRowIdLen = 0;
  ExpTupleDesc *updatedRowTupleDesc   = 0;
  ULng32 updateRowLen = 0;
  Queue * listOfUpdatedColNames = NULL;
  genHbaseUpdOrInsertExpr(generator, 
			  FALSE,
			  newRecExprArray(), updateTuppIndex,
			  &updateExpr, updateRowLen, 
			  &updatedRowTupleDesc,
			  listOfUpdatedColNames,
			  NULL, mergeInsertRowIdLen, 0,
			  getIndexDesc());

  work_cri_desc->setTupleDescriptor(updateTuppIndex, updatedRowTupleDesc);
  
  ExpTupleDesc *mergedRowTupleDesc   = 0;
  ULng32 mergeInsertRowLen = 0;
  Queue * listOfMergedColNames = NULL;
  if ((isMerge()) &&
      (mergeInsertRecExprArray().entries() > 0))
    {
      genHbaseUpdOrInsertExpr(generator, 
			      TRUE,
			      mergeInsertRecExprArray(), mergeInsertTuppIndex,
			      &mergeInsertExpr, mergeInsertRowLen, 
			      &mergedRowTupleDesc,
			      listOfMergedColNames,
			      &mergeInsertRowIdExpr, mergeInsertRowIdLen,
			      mergeInsertRowIdTuppIndex,
			      getIndexDesc());
      
      work_cri_desc->setTupleDescriptor(mergeInsertTuppIndex, mergedRowTupleDesc);
    }

  ULng32 rowIdAsciiRowLen = 0; 
  ExpTupleDesc * rowIdAsciiTupleDesc = 0;
  ex_expr * rowIdExpr = NULL;
  ULng32 rowIdLength = 0;
  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      // dont encode keys for hbase mapped tables since these tables
      // could be populated from outside of traf.
      NABoolean encodeKeys = TRUE;
      if (getTableDesc()->getNATable()->isHbaseMapTable())
        encodeKeys = FALSE;

      HbaseAccess::genRowIdExpr(generator,
				getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
				getHbaseSearchKeys(), 
				work_cri_desc, work_atp,
				rowIdAsciiTuppIndex, rowIdTuppIndex,
				rowIdAsciiRowLen, rowIdAsciiTupleDesc,
				rowIdLength, 
				rowIdExpr,
                                encodeKeys);
   }
  else
    {
      HbaseAccess::genRowIdExprForNonSQ(generator,
					getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
					getHbaseSearchKeys(), 
					work_cri_desc, work_atp,
					rowIdAsciiTuppIndex, rowIdTuppIndex,
					rowIdAsciiRowLen, rowIdAsciiTupleDesc,
					rowIdLength, 
					rowIdExpr);
    }
  
  Queue * listOfFetchedColNames = NULL;

  if (isAlignedFormat)
    {
      listOfFetchedColNames = new(space) Queue(space);
      
      NAString cnInList(SEABASE_DEFAULT_COL_FAMILY);
      cnInList += ":";
      unsigned char c = 1;
      cnInList.append((char*)&c, 1);
      short len = cnInList.length();
      cnInList.prepend((char*)&len, sizeof(short));
      
      char * colNameInList =
        space->AllocateAndCopyToAlignedSpace(cnInList, 0);
      
      listOfFetchedColNames->insert(colNameInList);
    }
  else
    {
      HbaseAccess::genListOfColNames(generator,
                                     getIndexDesc(),
                                     columnList,
                                     listOfFetchedColNames);
    }

  ex_expr * mergeUpdScanExpr = NULL;
  if (isMerge() && !mergeUpdatePred().isEmpty()) 
    {
      // Generate expression to evaluate any merge update predicate on the 
      // fetched row
      ItemExpr* updPredTree = 
	mergeUpdatePred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(updPredTree->getValueId(), 
			   ex_expr::exp_SCAN_PRED,
			   &mergeUpdScanExpr);
    }
  else if (getIndexDesc()->isClusteringIndex() && getCheckConstraints().entries())
    {
      // Generate the update and insert constraint check expressions

      // The constraint expressions at this time refer to the source values 
      // of the columns. We want to evaluate the constraints aganst the target 
      // values, though. So, we collect the source column ValueIds here so
      // we can map them to the appropriate target, which is dependent on
      // which constraint expression we are generating.

      ValueId constraintId;
      ValueIdSet constraintColumns;
      for (CollIndex ci = 0; ci < getCheckConstraints().entries(); ci++)
        {
          constraintId = getCheckConstraints()[ci];
          constraintId.getItemExpr()->findAll(ITM_INDEXCOLUMN,
                                              constraintColumns, // out, has append semantics
                                              TRUE, // visitVEGmembers
                                              FALSE); // don't visit index descriptors
        }

      // Prepare the constraint tree for generation

      ItemExpr *constrTree =
        getCheckConstraints().rebuildExprTree(ITM_AND, TRUE, TRUE);

      if (getTableDesc()->getNATable()->hasSerializedEncodedColumn())
        constrTree = generator->addCompDecodeForDerialization(constrTree, isAlignedFormat);

      // Generate the update constraint expression

      genUpdConstraintExpr(generator,
                           constrTree,
                           constraintColumns,
                           newRecExprArray(),
                           &updConstraintExpr /* out */);

      if ((isMerge()) && (mergeInsertRecExprArray().entries() > 0))
        {
          // Generate the insert constraint expression

          genUpdConstraintExpr(generator,
                               constrTree,
                               constraintColumns,
                               mergeInsertRecExprArray(),
                               &insConstraintExpr /* out */);   
        }

    }
 
  if ((getTableDesc()->getNATable()->isSeabaseTable()) &&
      (NOT isMerge()))
    {
      if ((keyInfo && getSearchKey() && getSearchKey()->isUnique()) ||
	  ( tdbListOfUniqueRows))
	{
	  // Save node for later use by RelRoot in the case of UPDATE CURRENT OF.
	  generator->updateCurrentOfRel() = (void*)this;
	}
    }

  if (getOptStoi() && getOptStoi()->getStoi())
    generator->addSqlTableOpenInfo(getOptStoi()->getStoi());

  LateNameInfo* lateNameInfo = new(generator->wHeap()) LateNameInfo();
  char * compileTimeAnsiName = (char*)getOptStoi()->getStoi()->ansiName();

  lateNameInfo->setCompileTimeName(compileTimeAnsiName, space);
  lateNameInfo->setLastUsedName(compileTimeAnsiName, space);
  lateNameInfo->setNameSpace(COM_TABLE_NAME);
  if (getIndexDesc()->getNAFileSet()->getKeytag() != 0)
    // is an index.
    {
      lateNameInfo->setIndex(TRUE);
      lateNameInfo->setNameSpace(COM_INDEX_NAME);
    }
  generator->addLateNameInfo(lateNameInfo);
 
  ex_expr * returnMergeInsertExpr = NULL;
  ULng32 returnedFetchedRowLen = 0;
  ULng32 returnedUpdatedRowLen = 0;
  ULng32 returnedMergeInsertedRowLen = 0;
  Queue *listOfOmittedColNames = NULL;

  if (returnRow)
    {
      const ValueIdList &fetchedOutputs =
	((getScanIndexDesc() != NULL) ?
	 getScanIndexDesc()->getIndexColumns() :
	 getIndexDesc()->getIndexColumns());

       // Generate a project expression to move the fetched row from
      // the fetchedRowAtpIndex in the work Atp to the returnedFetchedAtpIndex 
      // in the return Atp.
      MapTable * returnedFetchedMapTable = 0;
      ExpTupleDesc * returnedFetchedTupleDesc = NULL;
      expGen->generateContiguousMoveExpr
	(fetchedOutputs,
	 1, // add conv nodes
	 0,
	 returnedFetchedTuppIndex,
	 generator->getInternalFormat(),
	 returnedFetchedRowLen,
	 &projExpr, 
	 &returnedFetchedTupleDesc,
	 ExpTupleDesc::SHORT_FORMAT,
	 &returnedFetchedMapTable);

      // assign location attributes to columns referenced in scanIndex and index.
      // If any of these columns/value_ids are being updated, the updated location
      // will be assigned later in this code.
      expGen->assignAtpAndAtpIndex(getScanIndexDesc()->getIndexColumns(),
				   0, returnedFetchedTuppIndex); 
      expGen->assignAtpAndAtpIndex(getIndexDesc()->getIndexColumns(),
				   0, returnedFetchedTuppIndex); 
      
      ValueIdList updatedOutputs;
      ValueIdSet alreadyDeserialized;

      if (isMerge())
	{
	  BaseColumn *updtCol       = NULL,
	    *fetchedCol    = NULL;
	  Lng32        updtColNum    = -1,
	    fetchedColNum = 0;
	  CollIndex   recEntries    = newRecExprArray().entries(),
	    colEntries    = getIndexDesc()->getIndexColumns().entries(),
	    j = 0;
	  ValueId tgtValueId;
	  for (CollIndex ii = 0; ii < colEntries; ii++)
	    {
	      fetchedCol =
		(BaseColumn *)(((getIndexDesc()->getIndexColumns())[ii]).getItemExpr());
	      fetchedColNum = fetchedCol->getColNumber();
	      
	      updtCol =
		(updtCol != NULL ? updtCol :
		 (j < recEntries ?
		  (BaseColumn *)(newRecExprArray()[j].getItemExpr()->child(0)->castToItemExpr()) :
		  NULL));
	      
	      updtColNum = (updtCol ? updtCol->getColNumber() : -1);
	      
	      if (fetchedColNum == updtColNum)
		{
		  const ItemExpr *assignExpr = newRecExprArray()[j].getItemExpr();
		  tgtValueId = assignExpr->child(0)->castToItemExpr()->getValueId();

		  j++;
		  updtCol = NULL;
		}
	      else
		{
		  tgtValueId = fetchedCol->getValueId();
		  alreadyDeserialized += tgtValueId; // if it is necessary to deserialize, that is
		}
 
	      updatedOutputs.insert(tgtValueId);
	    }
	  if (getProducedMergeIUDIndicator() != NULL_VALUE_ID) 
	    updatedOutputs.insert(getProducedMergeIUDIndicator());
	}
      else
	{
	  for (CollIndex ii = 0; ii < newRecExprArray().entries(); ii++)
	    {
	      const ItemExpr *assignExpr = newRecExprArray()[ii].getItemExpr();
	      ValueId tgtValueId = assignExpr->child(0)->castToItemExpr()->getValueId();
	      
	      updatedOutputs.insert(tgtValueId);
	    }
	}

      ValueIdSet outputs = fetchedOutputs; 
      ValueIdSet updatedOutputsSet = updatedOutputs;
      outputs += updatedOutputsSet;
      getGroupAttr()->setCharacteristicOutputs(outputs);
      
      MapTable * returnedUpdatedMapTable = 0;
      ExpTupleDesc * returnedUpdatedTupleDesc = NULL;
      ValueIdList tgtConvValueIdList;

     if (getTableDesc()->getNATable()->hasSerializedEncodedColumn())
	{
	  // if serialized columns are present, then create a new row with
	  // deserialized columns before returning it.
	  expGen->generateDeserializedMoveExpr
	    (updatedOutputs,
	     0, 
	     returnedUpdatedTuppIndex, //projRowTuppIndex,
	     generator->getInternalFormat(),
	     returnedUpdatedRowLen,
	     &returnUpdateExpr, 
	     &returnedUpdatedTupleDesc,
	     ExpTupleDesc::SHORT_FORMAT,
	     tgtConvValueIdList,
	     alreadyDeserialized);
	}
     else
       {
	 expGen->generateContiguousMoveExpr
	   (updatedOutputs,
	    1, // add conv nodes
	    0,
	    returnedUpdatedTuppIndex,
	    generator->getInternalFormat(),
	    returnedUpdatedRowLen,
	    &returnUpdateExpr, 
	    &returnedUpdatedTupleDesc,
	    ExpTupleDesc::SHORT_FORMAT,
	    &returnedUpdatedMapTable,
	    &tgtConvValueIdList);
       }

     Attributes * tgtIUDMergeColConvAttr = NULL;
      const ValueIdList &indexColList = getIndexDesc()->getIndexColumns();
      for (CollIndex ii = 0; ii < tgtConvValueIdList.entries(); ii++)
	{
	  const ValueId &tgtColValueId = updatedOutputs[ii];
	  const ValueId &tgtColConvValueId = tgtConvValueIdList[ii];
	  if (updatedOutputs[ii] == getProducedMergeIUDIndicator())
	  {
	    tgtIUDMergeColConvAttr = 
	      (generator->getMapInfo(tgtColConvValueId, 0))->getAttr();
	    continue;
	  }
	
	  BaseColumn * bc = (BaseColumn*)tgtColValueId.getItemExpr();
	  const ValueId &indexColValueId = indexColList[bc->getColNumber()];
	  
	  Attributes * tgtColConvAttr = (generator->getMapInfo(tgtColConvValueId, 0))->getAttr();
	  Attributes * indexColAttr = (generator->addMapInfo(indexColValueId, 0))->getAttr();
	  
	  indexColAttr->copyLocationAttrs(tgtColConvAttr);
	}

      // Set up the returned tuple descriptor for the updated tuple.
      //
      returnedDesc->setTupleDescriptor(returnedFetchedTuppIndex, 
				       returnedFetchedTupleDesc);
      returnedDesc->setTupleDescriptor(returnedUpdatedTuppIndex, 
				       returnedUpdatedTupleDesc);
      
      /*
      expGen->assignAtpAndAtpIndex(getScanIndexDesc()->getIndexColumns(),
				   0, returnedFetchedTuppIndex); 
      */
      NAColumn *col;
      if (isMerge())
	{
	  ValueIdList mergeInsertOutputs;

	  for (CollIndex ii = 0; ii < mergeInsertRecExprArray().entries(); ii++)
	  {
	      const ItemExpr *assignExpr = mergeInsertRecExprArray()[ii].getItemExpr();
	      ValueId tgtValueId = assignExpr->child(0)->castToItemExpr()->getValueId();
              col = tgtValueId.getNAColumn( TRUE );

              if ((NOT isAlignedFormat) &&
                  ((CmpCommon::getDefault(TRAF_UPSERT_MODE) == DF_MERGE) ||
                   (CmpCommon::getDefault(TRAF_UPSERT_MODE) == DF_OPTIMAL)) && 
                  (((Assign*)assignExpr)->canBeSkipped()) &&
                 (NOT col->isSystemColumn()) &&
                 (NOT col->isClusteringKey()) &&
                 (NOT col->isIdentityColumn()))
              {
                 if (listOfOmittedColNames == NULL)
                    listOfOmittedColNames = new(space) Queue(space);
                 NAString cnInList;
                 HbaseAccess::createHbaseColId(col, cnInList);

                 char * colNameInList = 
                    space->AllocateAndCopyToAlignedSpace(cnInList, 0);
          
                 listOfOmittedColNames->insert(colNameInList);
              }
	      mergeInsertOutputs.insert(tgtValueId);
	  }
	   if (getProducedMergeIUDIndicator() != NULL_VALUE_ID) 
	    mergeInsertOutputs.insert(getProducedMergeIUDIndicator());
	  
	  MapTable * returnedMergeInsertedMapTable = 0;
	  ExpTupleDesc * returnedMergeInsertedTupleDesc = NULL;
	  ValueIdList tgtConvValueIdList;

	  if (getTableDesc()->getNATable()->hasSerializedEncodedColumn())
	    {
	      // if serialized columns are present, then create a new row with
	      // deserialized columns before returning it.
	      expGen->generateDeserializedMoveExpr
		(mergeInsertOutputs,
		 0, 
		 returnedUpdatedTuppIndex, 
		 generator->getInternalFormat(),
		 returnedMergeInsertedRowLen,
		 &returnMergeInsertExpr, 
		 &returnedMergeInsertedTupleDesc,
		 ExpTupleDesc::SHORT_FORMAT,
		 tgtConvValueIdList,
		 alreadyDeserialized);
	    }
	  else
	    {
	      expGen->generateContiguousMoveExpr
		(mergeInsertOutputs,
		 1, // add conv nodes
		 0,
		 returnedUpdatedTuppIndex,
		 generator->getInternalFormat(),
		 returnedMergeInsertedRowLen,
		 &returnMergeInsertExpr, 
		 &returnedMergeInsertedTupleDesc,
		 ExpTupleDesc::SHORT_FORMAT,
		 &returnedMergeInsertedMapTable,
		 &tgtConvValueIdList);
	    }
	  if (getProducedMergeIUDIndicator() != NULL_VALUE_ID)
	    iudIndicatorAttr->copyLocationAttrs(tgtIUDMergeColConvAttr);
	}
    }

  Cardinality expectedRows = (Cardinality) getEstRowsUsed().getValue();
  ULng32 buffersize = getDefault(GEN_DPSO_BUFFER_SIZE);
  buffersize = MAXOF(3*convertRowLen, buffersize);

  queue_index upqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_UP);
  queue_index downqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_DOWN);
  Int32 numBuffers = getDefault(GEN_DPUO_NUM_BUFFERS);

  char * tablename = NULL;
  if ((getTableDesc()->getNATable()->isHbaseRowTable()) ||
      (getTableDesc()->getNATable()->isHbaseCellTable()))
    {
      if (getIndexDesc() && getIndexDesc()->getNAFileSet())
	tablename = space->AllocateAndCopyToAlignedSpace(GenGetQualifiedName(getIndexDesc()->getNAFileSet()->getFileSetName().getObjectName()), 0);
    }
  else
    {
      if (getIndexDesc() && getIndexDesc()->getNAFileSet())
	tablename = space->AllocateAndCopyToAlignedSpace(GenGetQualifiedName(getIndexDesc()->getNAFileSet()->getFileSetName()), 0);
    }

  if (! tablename)
    tablename = 
      space->AllocateAndCopyToAlignedSpace(
					   GenGetQualifiedName(getTableName()), 0);

  NAString serverNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_SERVER);
  NAString zkPortNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_ZOOKEEPER_PORT);
  char * server = space->allocateAlignedSpace(serverNAS.length() + 1);
  strcpy(server, serverNAS.data());
  char * zkPort = space->allocateAlignedSpace(zkPortNAS.length() + 1);
  strcpy(zkPort, zkPortNAS.data());

  ComTdbHbaseAccess::HbasePerfAttributes * hbpa =
    new(space) ComTdbHbaseAccess::HbasePerfAttributes();
  if (CmpCommon::getDefault(HBASE_CACHE_BLOCKS) != DF_OFF)
    hbpa->setCacheBlocks(TRUE);
  // estrowsaccessed is 0 for now, so cache size will be set to minimum
  generator->setHBaseNumCacheRows(getEstRowsAccessed().getValue(), hbpa,asciiRowLen) ;


  // create hdfsscan_tdb
  ComTdbHbaseAccess *hbasescan_tdb = new(space) 
    ComTdbHbaseAccess(
		      (isMerge() ? ComTdbHbaseAccess::MERGE_ : ComTdbHbaseAccess::UPDATE_),
		      tablename,

		      convert_expr,
		      scanExpr,
		      rowIdExpr,
		      updateExpr,
		      mergeInsertExpr,
		      mergeInsertRowIdExpr,
		      mergeUpdScanExpr,
		      projExpr,
		      returnUpdateExpr,
		      returnMergeInsertExpr,
		      NULL, // encodedKeyExpr
		      keyColValExpr,
		      NULL, // hbaseFilterValExpr

		      asciiRowLen,
		      convertRowLen,
		      updateRowLen,
		      mergeInsertRowLen,
		      returnedFetchedRowLen,
		      returnedUpdatedRowLen,
		      
		      ((rowIdLength > 0) ? rowIdLength : mergeInsertRowIdLen),
		      convertRowLen,
		      rowIdAsciiRowLen,
		      (keyInfo ? keyInfo->getKeyLength() : 0),
		      keyColValLen,
		      0, // hbaseFilterValRowLen

		      asciiTuppIndex,
		      convertTuppIndex,
		      updateTuppIndex,
		      mergeInsertTuppIndex,
		      mergeInsertRowIdTuppIndex,
		      mergeIUDIndicatorTuppIndex,
		      returnedFetchedTuppIndex,
		      returnedUpdatedTuppIndex,

		      rowIdTuppIndex,
		      returnedDesc->noTuples()-1,
		      rowIdAsciiTuppIndex,
		      0, // keyTuppIndex,
		      keyColValTuppIndex,
		      0, // hbaseFilterValTuppIndex

                      0, // hbaseTimestamp
                      0, // hbaseVersion

		      tdbListOfRangeRows,
		      tdbListOfUniqueRows,
		      listOfFetchedColNames,
		      listOfUpdatedColNames,
		      listOfMergedColNames,

		      keyInfo,
		      keyColName,

		      work_cri_desc,
		      givenDesc,
		      returnedDesc,
		      downqueuelength,
		      upqueuelength,
                      expectedRows,
		      numBuffers,
		      buffersize,

		      server,
                      zkPort,
		      hbpa
		      );

  generator->initTdbFields(hbasescan_tdb);
  hbasescan_tdb->setListOfOmittedColNames(listOfOmittedColNames);

  if (getTableDesc()->getNATable()->isHbaseRowTable()) 
    hbasescan_tdb->setRowwiseFormat(TRUE);

  if (updConstraintExpr)
    hbasescan_tdb->setUpdConstraintExpr(updConstraintExpr);

  if (insConstraintExpr)
    hbasescan_tdb->setInsConstraintExpr(insConstraintExpr);

  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      hbasescan_tdb->setSQHbaseTable(TRUE);

      if (isAlignedFormat)
        hbasescan_tdb->setAlignedFormat(TRUE);

      if (CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON)
	hbasescan_tdb->setHbaseSqlIUD(TRUE);

      if (getTableDesc()->getNATable()->isEnabledForDDLQI())
        generator->objectUids().insert(
          getTableDesc()->getNATable()->objectUid().get_value());
    }

  if (keyInfo && getSearchKey() && getSearchKey()->isUnique())
    hbasescan_tdb->setUniqueKeyInfo(TRUE);

  if (returnRow)
    hbasescan_tdb->setReturnRow(TRUE);

  if (rowsAffected() != GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED)
    hbasescan_tdb->setComputeRowsAffected(TRUE);

  if (! tdbListOfUniqueRows)
    {
      hbasescan_tdb->setSubsetOper(TRUE);
    }

  if (uniqueRowsetHbaseOper()) {
    hbasescan_tdb->setRowsetOper(TRUE);
    hbasescan_tdb->setHbaseRowsetVsbbSize(getDefault(HBASE_ROWSET_VSBB_SIZE));
  }

  if (canDoCheckAndUpdel())
    hbasescan_tdb->setCanDoCheckAndUpdel(TRUE);

  if (generator->isTransactionNeeded())
    setTransactionRequired(generator);
  else if (noDTMxn())
    hbasescan_tdb->setUseHbaseXn(TRUE);
  else if (useRegionXn())
    hbasescan_tdb->setUseRegionXn(TRUE);

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(hbasescan_tdb, 0, 0, generator));
  }

  if ((generator->computeStats()) && 
      (generator->collectStatsType() == ComTdb::PERTABLE_STATS
      || generator->collectStatsType() == ComTdb::OPERATOR_STATS))
    {
      hbasescan_tdb->setPertableStatsTdbId((UInt16)generator->
					   getPertableStatsTdbId());
    }

  generator->setFoundAnUpdate(TRUE);

  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);
  generator->setGenObj(this, hbasescan_tdb);

  return 0;
}

bool compHBaseQualif ( NAString a  , NAString b)
{
  char * a_str = (char*)(a.data());
  char * b_str = (char*)(b.data());

  return (strcmp (&(a_str[sizeof(short) + sizeof(UInt32)]), &(b_str[sizeof(short)+ sizeof(UInt32)]))<0);
};

extern Int64 getDefaultSlidingSampleSize(Int64 tblRowCount);
extern Int64 getDefaultSampleSize(Int64 tblRowCount);

short HbaseInsert::codeGen(Generator *generator)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  // allocate a map table for the retrieved columns
  MapTable * last_map_table = generator->getLastMapTable();
  NABoolean inlinedActions = FALSE;
  if ((getInliningInfo().hasInlinedActions()) ||
      (getInliningInfo().isEffectiveGU()))
    inlinedActions = TRUE;

  NABoolean returnRow = getReturnRow(this, getIndexDesc());
  if (getIsTrafLoadPrep() || (isUpsert() && inlinedActions))
    returnRow = isReturnRow();

  ex_cri_desc * givenDesc = generator->getCriDesc(Generator::DOWN);
  ex_cri_desc * returnedDesc = givenDesc;

  if (returnRow)
    returnedDesc = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);

  const Int32 returnRowTuppIndex = returnedDesc->noTuples() - 1;

  const Int32 work_atp = 1;
  ex_cri_desc * workCriDesc = NULL;
  const UInt16 insertTuppIndex = 2;
  const UInt16 rowIdTuppIndex = 3;
  const UInt16 loggingTuppIndex = 4;
  const UInt16 projRowTuppIndex = 5;
  workCriDesc = new(space) ex_cri_desc(6, space);

  ULng32 loggingRowLen = 0;

  NABoolean hasAddedColumns = FALSE;
  if (getTableDesc()->getNATable()->hasAddedColumn())
    hasAddedColumns = TRUE;

  ValueIdList insertVIDList;
  ValueIdList keyVIDList;
  NAColumnArray colArray;
  NAColumn *col;

  ValueIdList returnRowVIDList;
  Queue *listOfOmittedColNames = NULL;

  NABoolean isAlignedFormat = getTableDesc()->getNATable()->isAlignedFormat(getIndexDesc());
  NABoolean isHbaseMapFormat = getTableDesc()->getNATable()->isHbaseMapTable();
  Int16 colIndexOfPK1 = -1;

  for (CollIndex ii = 0; ii < newRecExprArray().entries(); ii++)
  {
      const ItemExpr *assignExpr = newRecExprArray()[ii].getItemExpr();
      ValueId tgtValueId = assignExpr->child(0)->castToItemExpr()->getValueId();
      ValueId srcValueId = assignExpr->child(1)->castToItemExpr()->getValueId();

      col = tgtValueId.getNAColumn( TRUE );
      if ((NOT isAlignedFormat) &&
         ((CmpCommon::getDefault(TRAF_UPSERT_MODE) == DF_MERGE) ||
          (CmpCommon::getDefault(TRAF_UPSERT_MODE) == DF_OPTIMAL)) && 
         (((Assign*)assignExpr)->canBeSkipped()) && 
         (NOT col->isSystemColumn()) &&
         (NOT col->isClusteringKey()) &&
         (NOT col->isIdentityColumn()))
      {
          if (listOfOmittedColNames == NULL)
             listOfOmittedColNames = new(space) Queue(space);
          NAString cnInList;
          HbaseAccess::createHbaseColId(col, cnInList);

          char * colNameInList = 
             space->AllocateAndCopyToAlignedSpace(cnInList, 0);
          listOfOmittedColNames->insert(colNameInList);
      }
      else
      {
	if (col->isClusteringKey() && !isAlignedFormat && colIndexOfPK1 == -1)
	  colIndexOfPK1 = (listOfOmittedColNames == NULL) ?  ii : 
	    ii - listOfOmittedColNames->entries();
      }
      colArray.insert( col );

      if (returnRow)
        returnRowVIDList.insert(tgtValueId);

      ItemExpr * child1Expr = assignExpr->child(1);
      const NAType &givenType = tgtValueId.getType();
      
      ItemExpr * ie = new(generator->wHeap())
	Cast(child1Expr, &givenType);

      if ((isHbaseMapFormat) &&
          (getTableDesc()->getNATable()->isHbaseDataFormatString()) &&
          (NOT (DFS2REC::isAnyCharacter(givenType.getFSDatatype()))))
        {
          Lng32 cvl = givenType.getDisplayLength();

          NAType * asciiType = 
            new (generator->wHeap()) SQLVarChar(generator->wHeap(), cvl, givenType.supportsSQLnull());
          ie = new(generator->wHeap()) Cast(ie, asciiType);
        }

      if ((NOT isAlignedFormat) && HbaseAccess::isEncodingNeededForSerialization
	  (assignExpr->child(0)->castToItemExpr()))
	{
	  ie = new(generator->wHeap()) CompEncode
	    (ie, FALSE, -1, CollationInfo::Sort, TRUE);
	}

      ie->bindNode(generator->getBindWA());
      if (generator->getBindWA()->errStatus()) 
	{ 
	  GenAssert(0,"bindNode failed");
	}
      insertVIDList.insert(ie->getValueId());
  }

  const NATable *naTable = getTableDesc()->getNATable();

  ULng32 insertRowLen    = 0;
  ExpTupleDesc * tupleDesc   = 0;
  ExpTupleDesc::TupleDataFormat tupleFormat;

  if (isAlignedFormat)
    tupleFormat = ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  else
    tupleFormat = ExpTupleDesc::SQLARK_EXPLODED_FORMAT;

  ex_expr *insertExpr = 0;
  expGen->generateContiguousMoveExpr(
				     insertVIDList,
				     0, // dont add convert nodes
				     1,  // work atp
				     insertTuppIndex,
				     tupleFormat,
				     insertRowLen,
				     &insertExpr, 
				     &tupleDesc,
				     ExpTupleDesc::LONG_FORMAT,
				     NULL,
				     NULL,
				     0,
				     NULL,
				     NULL,
				     &colArray);

  ex_expr * loggingDataExpr = NULL;
  ExpTupleDesc * loggingDataTupleDesc = NULL;
  //--bulk load error logging
  if (CmpCommon::getDefault(TRAF_LOAD_LOG_ERROR_ROWS) == DF_ON) {
  CmpContext *cmpContext = generator->currentCmpContext();

  ValueIdList loggingDataVids;

  for (CollIndex i = 0; i < insertVIDList.entries(); i++)
  {
    ItemExpr &inputExpr = *(insertVIDList[i].getItemExpr());
    const NAType &formalType = insertVIDList[i].getType();
    ItemExpr *lmExpr = NULL;
    ItemExpr *lmExpr2 = NULL;
    int res;

    lmExpr = &inputExpr;
    res = CreateAllCharsExpr(formalType, // [IN] Child output type
        *lmExpr,                         // [IN] Actual input value
        cmpContext,                      // [IN] Compilation context
        lmExpr2                          // [OUT] Returned expression
        );
    GenAssert(res == 0 && lmExpr != NULL,
        "Error building expression tree for LM child Input value");
    if (lmExpr2)
    {
      lmExpr2->bindNode(generator->getBindWA());
      loggingDataVids.insert(lmExpr2->getValueId());
    }
  } // for (i = 0; i < insertVIDList.entries(); i++)

  if (loggingDataVids.entries()>0)
  {
    expGen->generateContiguousMoveExpr (
      loggingDataVids,                       // [IN] source ValueIds
      FALSE,                                 // [IN] add convert nodes?
      1,                                     // [IN] target atp number (work atp 1)
      loggingTuppIndex,                      // [IN] target tupp index
      // The target format should be exploded format always because the column delimiter
      // added during execution assumes exploded format
      ExpTupleDesc::SQLARK_EXPLODED_FORMAT,  // [IN] target tuple data format 
      loggingRowLen,                         // [OUT] target tuple length
      &loggingDataExpr,                      // [OUT] move expression
      &loggingDataTupleDesc,                 // [optional OUT] target tuple desc
      ExpTupleDesc::LONG_FORMAT              // [optional IN] target desc format
      );

  }
  // Add the tuple descriptor for request values to the work ATP
  workCriDesc->setTupleDescriptor(loggingTuppIndex, loggingDataTupleDesc);
  }
  ////////////
  // If constraints are present, generate constraint expression.
  // Only works for base tables because the constraint information is
  // stored with the table descriptor which doesn't exist for indexes.
  //
  ex_expr * insConstraintExpr = NULL;
  Queue * listOfUpdatedColNames = NULL;
  Lng32 keyAttrPos = -1;
  ex_expr * rowIdExpr = NULL;
  ULng32 rowIdLen = 0;

  const ValueIdList &indexVIDlist = getIndexDesc()->getIndexColumns();
  for (CollIndex ii = 0; ii < newRecExprArray().entries(); ii++)
    {
      const ItemExpr *assignExpr = newRecExprArray()[ii].getItemExpr();
      const ValueId &tgtValueId = assignExpr->child(0)->castToItemExpr()->getValueId();
      const ValueId &indexValueId = indexVIDlist[ii];
      col = tgtValueId.getNAColumn( TRUE );
      ValueId &srcValId = insertVIDList[ii];
      
      Attributes * colAttr = (generator->addMapInfo(tgtValueId, 0))->getAttr();
      Attributes * indexAttr = (generator->addMapInfo(indexValueId, 0))->getAttr();
      Attributes * castAttr = (generator->getMapInfo(srcValId, 0))->getAttr();
      
      colAttr->copyLocationAttrs(castAttr);
      indexAttr->copyLocationAttrs(castAttr);
    }

  ex_expr* preCondExpr = NULL;
  if (! getPrecondition().isEmpty())
  {
    ItemExpr * preCondTree = getPrecondition().rebuildExprTree(ITM_AND,
							       TRUE,TRUE);
    expGen->generateExpr(preCondTree->getValueId(), ex_expr::exp_SCAN_PRED,
			 &preCondExpr);
  }

  ULng32 f;
  expGen->generateKeyEncodeExpr(
				getIndexDesc(),                         // describes the columns
				work_atp,                                      // work Atp
				rowIdTuppIndex,                     // work Atp entry 
				ExpTupleDesc::SQLMX_KEY_FORMAT,         // Tuple format
				rowIdLen,                                 // Key length
				&rowIdExpr,                            // Encode expression
				FALSE,
				f,
				NULL,
				TRUE); // handle serialization
  
  if (getIndexDesc()->isClusteringIndex() && getCheckConstraints().entries())
    {
      ItemExpr *constrTree = 
	getCheckConstraints().rebuildExprTree(ITM_AND, TRUE, TRUE);

      if (getTableDesc()->getNATable()->hasSerializedEncodedColumn())
	constrTree = generator->addCompDecodeForDerialization(constrTree, isAlignedFormat);

      expGen->generateExpr(constrTree->getValueId(), ex_expr::exp_SCAN_PRED,
			   &insConstraintExpr);
    }
  
  listOfUpdatedColNames = new(space) Queue(space);
  std::vector<NAString> columNamesVec;

  if (NOT isAlignedFormat)
    {
      for (CollIndex c = 0; c < colArray.entries(); c++)
        {
          const NAColumn * nac = colArray[c];
          
          NAString cnInList;
          if (isHbaseMapFormat)
            {
              cnInList = nac->getHbaseColFam();
              cnInList += ":";
              cnInList += nac->getColName();

              short len = cnInList.length();
              cnInList.prepend((char*)&len, sizeof(short));
            }
          else
            {
              HbaseAccess::createHbaseColId(nac, cnInList,
                                            (getIndexDesc()->getNAFileSet()->getKeytag() != 0));
            }

          if (getIsTrafLoadPrep())
            {
              UInt32 pos = (UInt32)c +1;
              cnInList.prepend((char*)&pos, sizeof(UInt32));
              columNamesVec.push_back(cnInList);
            }
          else
            {
              char * colNameInList =
                space->AllocateAndCopyToAlignedSpace(cnInList, 0);
              
              listOfUpdatedColNames->insert(colNameInList);
            }
        }
      
      if (getIsTrafLoadPrep())
        {
          std::sort(columNamesVec.begin(), columNamesVec.end(),compHBaseQualif);
          for (std::vector<NAString>::iterator it = columNamesVec.begin() ; it != columNamesVec.end(); ++it)
            {
              NAString cnInList2 = *it;
              char * colNameInList =
                space->AllocateAndCopyToAlignedSpace(cnInList2, 0);
              
              listOfUpdatedColNames->insert(colNameInList);
            }
        }
    }
  else
    {
      NAString cnInList(SEABASE_DEFAULT_COL_FAMILY);
      cnInList += ":";
      unsigned char c = 1;
      cnInList.append((char*)&c, 1);
      short len = cnInList.length();
      cnInList.prepend((char*)&len, sizeof(short));

      char * colNameInList =
        space->AllocateAndCopyToAlignedSpace(cnInList, 0);

      listOfUpdatedColNames->insert(colNameInList);
    }

  // Assign attributes to the ASSIGN nodes of the newRecExpArray()
  // This is not the same as the generateContiguousMoveExpr() call
  // above since different valueId's are added to the mapTable.
  // 
  ULng32 tempInsertRowLen    = 0;
  ExpTupleDesc * tempTupleDesc   = 0;
  expGen->processValIdList(newRecExprArray(),
			   tupleFormat,
			   tempInsertRowLen,
			   0, 
			   returnRowTuppIndex, // insertTuppIndex,
			   &tempTupleDesc,
			   ExpTupleDesc::LONG_FORMAT,
                           0, NULL, &colArray);

  // Add the inserted tuple descriptor to the work cri descriptor.
  //
  if (workCriDesc)
    workCriDesc->setTupleDescriptor(insertTuppIndex, tupleDesc);

  ex_expr * projExpr = NULL;
  ULng32 projRowLen    = 0;
  ExpTupleDesc * projRowTupleDesc   = 0;
  if (returnRow)
    {
      if (getTableDesc()->getNATable()->hasSerializedEncodedColumn())
	{
	  ValueIdList deserColVIDList;
	  ValueIdSet dummy;

	  // if serialized columns are present, then create a new row with
	  // deserialized columns before returning it.
	  expGen->generateDeserializedMoveExpr
	    (returnRowVIDList,
	     0,//work_atp,
	     returnRowTuppIndex, //projRowTuppIndex,
	     generator->getInternalFormat(),
	     projRowLen,
	     &projExpr, 
	     &projRowTupleDesc,
	     ExpTupleDesc::SHORT_FORMAT,
	     deserColVIDList,
	     dummy);
	  
	  workCriDesc->setTupleDescriptor(projRowTuppIndex, projRowTupleDesc);

	  // make the location of returnRowVIDlist point to the newly generated values.
	  for (CollIndex ii = 0; ii < returnRowVIDList.entries(); ii++)
	    {
	      const ValueId &retColVID = returnRowVIDList[ii];
	      const ValueId &deserColVID = deserColVIDList[ii];
	      
	      Attributes * retColAttr = (generator->getMapInfo(retColVID, 0))->getAttr();
	      Attributes * deserColAttr = (generator->addMapInfo(deserColVID, 0))->getAttr();
	      
	      retColAttr->copyLocationAttrs(deserColAttr);
	    }
	  
	  expGen->assignAtpAndAtpIndex(returnRowVIDList,
				       0, returnRowTuppIndex);
	}
      else
	{
	  expGen->processValIdList(returnRowVIDList,
				   tupleFormat,
				   tempInsertRowLen,
				   0, 
				   returnRowTuppIndex);
	}
    }

  ComTdbDp2Oper::SqlTableType stt = ComTdbDp2Oper::NOOP_;
  if (getIndexDesc()->getNAFileSet()->isKeySequenced())
    {
      const NAColumnArray & column_array = getIndexDesc()->getAllColumns();
  
      if ((column_array[0]->isSyskeyColumn()) &&
	  (column_array[0]->getType()->getNominalSize() >= 4)) {
	stt = ComTdbDp2Oper::KEY_SEQ_WITH_SYSKEY_;
      }
      else
	stt = ComTdbDp2Oper::KEY_SEQ_;
    }
  
  Cardinality expectedRows = (Cardinality) getEstRowsUsed().getValue();
  ULng32 buffersize = getDefault(GEN_DP2I_BUFFER_SIZE);
  buffersize = MAXOF(3*insertRowLen, buffersize);

  queue_index upqueuelength =  (queue_index)getDefault(GEN_DP2I_SIZE_UP);
  queue_index downqueuelength = (queue_index)getDefault(GEN_DP2I_SIZE_DOWN);
  Int32 numBuffers = getDefault(GEN_DP2I_NUM_BUFFERS);

  if ((getInsertType() == Insert::VSBB_INSERT_USER || // covers upsert
       getInsertType() == Insert::UPSERT_LOAD)  && // covers upsert using load and bulk load
      generator->oltOptInfo()->multipleRowsReturned())
  {
    downqueuelength = getDefault(HBASE_ROWSET_VSBB_SIZE);
    queue_index dq = 1;
    queue_index bits = downqueuelength;
    while (bits && dq < downqueuelength) {
        bits = bits  >> 1;
        dq = dq << 1;
    }
    downqueuelength = dq;
  }
  char * tablename = NULL;
  if ((getTableDesc()->getNATable()->isHbaseRowTable()) ||
      (getTableDesc()->getNATable()->isHbaseCellTable()) ||
      (getTableName().getQualifiedNameObj().isHbaseMappedName()))
    {
      tablename = space->AllocateAndCopyToAlignedSpace(GenGetQualifiedName(getIndexDesc()->getIndexName().getObjectName()), 0);
    }
  else
    {
      tablename = space->AllocateAndCopyToAlignedSpace(
						       GenGetQualifiedName(getIndexDesc()->getIndexName()), 0);
    }

  NAString serverNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_SERVER);
  NAString zkPortNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_ZOOKEEPER_PORT);
  char * server = space->allocateAlignedSpace(serverNAS.length() + 1);
  strcpy(server, serverNAS.data());
  char * zkPort = space->allocateAlignedSpace(zkPortNAS.length() + 1);
  strcpy(zkPort, zkPortNAS.data());

  ComTdbHbaseAccess::HbasePerfAttributes * hbpa =
    new(space) ComTdbHbaseAccess::HbasePerfAttributes();

  ComTdbHbaseAccess::ComTdbAccessType t;
  if (isUpsert())
    {
      if (getInsertType() == Insert::UPSERT_LOAD)
	t = ComTdbHbaseAccess::UPSERT_LOAD_;
      else
	t = ComTdbHbaseAccess::UPSERT_;
    }
  else
    t = ComTdbHbaseAccess::INSERT_;

  // create hdfsscan_tdb
  ComTdbHbaseAccess *hbasescan_tdb = new(space) 
    ComTdbHbaseAccess(
		      t,
		      tablename,
		      insertExpr,
		      NULL,
		      rowIdExpr,
		      loggingDataExpr, // logging expr
		      NULL, // mergeInsertExpr
		      NULL, // mergeInsertRowIdExpr
		      NULL, // mergeUpdScanExpr
		      NULL, // projExpr
		      projExpr, // returnedUpdatedExpr
		      NULL, // returnMergeUpdateExpr
		      NULL, // encodedKeyExpr, 
		      NULL, // keyColValExpr
		      NULL, // hbaseFilterValExpr

		      0, //asciiRowLen,
		      insertRowLen,
		      loggingRowLen, //loggingRowLen
		      0, // mergeInsertRowLen
		      0, // fetchedRowLen
		      projRowLen, // returnedUpdatedRowLen

		      rowIdLen,
		      0, //outputRowLen,
		      0, //rowIdAsciiRowLen
		      0, // keyLen
		      0, // keyColValLen
		      0, // hbaseFilterValRowLen

		      0, //asciiTuppIndex,
		      insertTuppIndex,
		      loggingTuppIndex, //loggingTuppIndex
		      0, // mergeInsertTuppIndex
		      0, // mergeInsertRowIdTuppIndex
		      0, // mergeIUDIndicatorTuppIndex
		      0, // returnedFetchedTuppIndex
		      projRowTuppIndex, // returnedUpdatedTuppIndex
		      
		      rowIdTuppIndex,
		      returnRowTuppIndex, //returnedDesc->noTuples()-1,
		      0, // rowIdAsciiTuppIndex
		      0, // keyTuppIndex
		      0, // keyColValTuppIndex
		      0, // hbaseFilterValTuppIndex

                      0, // hbaseTimestamp
                      0, // hbaseVersion

		      NULL,
		      NULL, //tdbListOfDelRows,
		      NULL,
		      listOfUpdatedColNames,
		      NULL,

		      NULL,
		      NULL,

		      workCriDesc,
		      givenDesc,
		      returnedDesc,

		      downqueuelength,
		      upqueuelength,
                      expectedRows,
		      numBuffers,
		      buffersize,

		      server, 
                      zkPort,
		      hbpa
		      );

  generator->initTdbFields(hbasescan_tdb);
  hbasescan_tdb->setListOfOmittedColNames(listOfOmittedColNames);

  if ((CmpCommon::getDefault(HBASE_ASYNC_OPERATIONS) == DF_ON)
           && getInliningInfo().isIMGU())
    hbasescan_tdb->setAsyncOperations(TRUE);

  if (preCondExpr)
    hbasescan_tdb->setInsDelPreCondExpr(preCondExpr);

  if (insConstraintExpr)
    hbasescan_tdb->setInsConstraintExpr(insConstraintExpr);

  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      hbasescan_tdb->setSQHbaseTable(TRUE);

      if (isAlignedFormat)
        hbasescan_tdb->setAlignedFormat(TRUE);

      if (isHbaseMapFormat)
        {
          hbasescan_tdb->setHbaseMapTable(TRUE);
          
          if (getTableDesc()->getNATable()->getClusteringIndex()->hasSingleColVarcharKey())
            hbasescan_tdb->setKeyInVCformat(TRUE);
        }

      if (CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON)
	hbasescan_tdb->setHbaseSqlIUD(TRUE);

      if ((isUpsert()) ||
	  (noCheck()))
	hbasescan_tdb->setHbaseSqlIUD(FALSE);

      if (((((getInsertType() == Insert::VSBB_INSERT_USER) || isUpsert() )&& 
           generator->oltOptInfo()->multipleRowsReturned())) ||
	  (getInsertType() == Insert::UPSERT_LOAD))
      {
	hbasescan_tdb->setVsbbInsert(TRUE);
        hbasescan_tdb->setHbaseRowsetVsbbSize(getDefault(HBASE_ROWSET_VSBB_SIZE));
        // Set the VSBB flag in the RelExpr to display in explain
        if (! hbasescan_tdb->hbaseSqlIUD())  
           setVsbbInsert(TRUE);
      }

      
      if (isUpsert() && (getInsertType() == Insert::UPSERT_LOAD))
	{
	  // this will cause tupleflow operator to send in an EOD to this upsert
	  // operator. On seeing that, executor will flush the buffers.
	  generator->setVSBBInsert(TRUE);
	}
      if (xformedEffUpsert())
        generator->setEffTreeUpsert(TRUE);

      //setting parametes for hbase bulk load integration
      hbasescan_tdb->setIsTrafodionLoadPrep(this->getIsTrafLoadPrep());
      if (hbasescan_tdb->getIsTrafodionLoadPrep())
      {
        NAString tlpTmpLocationNAS = ActiveSchemaDB()->getDefaults().getValue(TRAF_LOAD_PREP_TMP_LOCATION);
        char * tlpTmpLocation = space->allocateAlignedSpace(tlpTmpLocationNAS.length() + 1);
        strcpy(tlpTmpLocation, tlpTmpLocationNAS.data());
        hbasescan_tdb->setLoadPrepLocation(tlpTmpLocation);
        hbasescan_tdb->setNoDuplicates(CmpCommon::getDefault(TRAF_LOAD_PREP_SKIP_DUPLICATES) == DF_OFF);
        hbasescan_tdb->setMaxHFileSize(CmpCommon::getDefaultLong(TRAF_LOAD_MAX_HFILE_SIZE));

	ULng32 loadFlushSizeinKB = getDefault(TRAF_LOAD_FLUSH_SIZE_IN_KB);
	ULng32 loadFlushSizeinRows = 0;
	loadFlushSizeinRows = (loadFlushSizeinKB*1024)/hbasescan_tdb->getRowLen() ;
	// largest flush size, runtime cannot handle higher values 
	// without code change
	if (loadFlushSizeinRows >= USHRT_MAX/2)
	  loadFlushSizeinRows = ((USHRT_MAX/2)-1);
	else if (loadFlushSizeinRows < 1)  // make sure we don't fall to zero on really long rows
	  loadFlushSizeinRows = 1;
	hbasescan_tdb->setTrafLoadFlushSize(loadFlushSizeinRows);

        // For sample file, set the sample location in HDFS and the sampling rate.
        // Move later, when sampling not limited to bulk loads.
        if (getCreateUstatSample())
          {
            NAString sampleLocationNAS = ActiveSchemaDB()->getDefaults().getValue(TRAF_SAMPLE_TABLE_LOCATION);
            char * sampleLocation = space->allocateAlignedSpace(sampleLocationNAS.length() + 1);
            strcpy(sampleLocation, sampleLocationNAS.data());
            hbasescan_tdb->setSampleLocation(sampleLocation);

            Int64 totalRows = (Int64)(getInputCardinality().getValue());
            //printf("*** Incoming cardinality is " PF64 ".\n", totalRows);
            Int64 sampleRows;
            if (CmpCommon::getDefault(USTAT_USE_SLIDING_SAMPLE_RATIO) == DF_ON)
              sampleRows = getDefaultSlidingSampleSize(totalRows);
            else
              sampleRows = getDefaultSampleSize(totalRows);
            Float32 sampleRate = (Float32)sampleRows / (Float32)totalRows;
            //printf("*** In HbaseInsert::codeGen(): Sample percentage is %.2f.\n", sampleRate);
            hbasescan_tdb->setSamplingRate(sampleRate);
          }
        hbasescan_tdb->setContinueOnError(CmpCommon::getDefault(TRAF_LOAD_CONTINUE_ON_ERROR) == DF_ON);
        hbasescan_tdb->setLogErrorRows(CmpCommon::getDefault(TRAF_LOAD_LOG_ERROR_ROWS) == DF_ON);
        hbasescan_tdb->setMaxErrorRows((UInt32)CmpCommon::getDefaultNumeric(TRAF_LOAD_MAX_ERROR_ROWS));
        NAString errCountRowIdNAS = CmpCommon::getDefaultString(TRAF_LOAD_ERROR_COUNT_ID);
        char * errCountRowId = NULL;
        if (errCountRowIdNAS.length() > 0)
        {
          errCountRowId = space->allocateAlignedSpace(errCountRowIdNAS.length() + 1);
          strcpy(errCountRowId, errCountRowIdNAS.data());
          hbasescan_tdb->setErrCountRowId(errCountRowId);
        }

        NAString errCountTabNAS = ActiveSchemaDB()->getDefaults().getValue(TRAF_LOAD_ERROR_COUNT_TABLE);
        char * errCountTab = NULL;
        if (errCountTabNAS.length() > 0)
        {
          errCountTab = space->allocateAlignedSpace(errCountTabNAS.length() + 1);
          strcpy(errCountTab, errCountTabNAS.data());
          hbasescan_tdb->setErrCountTab(errCountTab);
        }
        NAString loggingLocNAS = ActiveSchemaDB()->getDefaults().getValue(TRAF_LOAD_ERROR_LOGGING_LOCATION);
        char * loggingLoc = NULL;
        if (loggingLocNAS.length() > 0)
        {
          loggingLoc = space->allocateAlignedSpace(loggingLocNAS.length() + 1);
          strcpy(loggingLoc, loggingLocNAS.data());
          hbasescan_tdb->setLoggingLocation(loggingLoc);
          }
      }

      // setting parameters for upsert statement// not related to the hbase bulk load intergration
      NABoolean traf_upsert_adjust_params =
         (CmpCommon::getDefault(TRAF_UPSERT_ADJUST_PARAMS) == DF_ON);
      if (traf_upsert_adjust_params)
      {
        ULng32 wbSize = getDefault(TRAF_UPSERT_WB_SIZE);
        NABoolean traf_write_toWAL =
                   (CmpCommon::getDefault(TRAF_UPSERT_WRITE_TO_WAL) == DF_ON);

        hbasescan_tdb->setTrafWriteToWAL(traf_write_toWAL);

        hbasescan_tdb->setCanAdjustTrafParams(true);

        hbasescan_tdb->setWBSize(wbSize);

      }
      if (getTableDesc()->getNATable()->isEnabledForDDLQI())
        generator->objectUids().insert(
          getTableDesc()->getNATable()->objectUid().get_value());

      if (colIndexOfPK1 >=0 && t ==  ComTdbHbaseAccess::INSERT_)
	hbasescan_tdb->setColIndexOfPK1(colIndexOfPK1);
    }
  else
    {
      if (getTableDesc()->getNATable()->isHbaseRowTable()) //rowwiseHbaseFormat())
	hbasescan_tdb->setRowwiseFormat(TRUE);
    }

  if (returnRow)
    hbasescan_tdb->setReturnRow(TRUE);

  if (rowsAffected() != GenericUpdate::DO_NOT_COMPUTE_ROWSAFFECTED)
    hbasescan_tdb->setComputeRowsAffected(TRUE);

  if (stt == ComTdbDp2Oper::KEY_SEQ_WITH_SYSKEY_)
    hbasescan_tdb->setAddSyskeyTS(TRUE);
 
  if (generator->isTransactionNeeded())
    setTransactionRequired(generator);
  else if (noDTMxn())
    hbasescan_tdb->setUseHbaseXn(TRUE);
  else if (useRegionXn())
    hbasescan_tdb->setUseRegionXn(TRUE);

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(hbasescan_tdb, 0, 0, generator));
  }

  if ((generator->computeStats()) && 
      (generator->collectStatsType() == ComTdb::PERTABLE_STATS
      || generator->collectStatsType() == ComTdb::OPERATOR_STATS))
    {
      hbasescan_tdb->setPertableStatsTdbId((UInt16)generator->
					   getPertableStatsTdbId());
    }

  generator->setFoundAnUpdate(TRUE);

  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);
  generator->setGenObj(this, hbasescan_tdb);

  return 0;
}
