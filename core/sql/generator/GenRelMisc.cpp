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
* File:         GenRelMisc.C
* RCS:          $Id: GenRelMisc.cpp,v 1.1 2007/10/09 19:38:54  Exp $
* Description:  MapValueId/Root/Tuple operators
*
* Created:      5/17/94
* Modified:     $ $Date: 2007/10/09 19:38:54 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
******************************************************************************
*/
#define   SQLPARSERGLOBALS_FLAGS
#include "ComOptIncludes.h"
#include "GroupAttr.h"
#include "ItemColRef.h"
#include "RelEnforcer.h"
#include "RelJoin.h"
#include "RelExeUtil.h"
#include "RelMisc.h"
#include "RelSet.h"
#include "RelUpdate.h"
#include "RelScan.h"
#include "RelDCL.h"
#include "PartFunc.h"
#include "Cost.h"
#include "GenExpGenerator.h"
#include "GenResources.h"
#include "ComTdbRoot.h"
#include "ComTdbTuple.h"
#include "ComTdbUnion.h"
#include "ComTdbTupleFlow.h"
#include "ComTdbTranspose.h"
#include "ComTdbSort.h"
#include "ComTdbPackRows.h"
#include "ComTdbDDL.h"
#include "ComTdbExeUtil.h"
#include "ComTdbFirstN.h"
#include "ComTdbStats.h"
#include "ComTdbCancel.h"
#include "ExplainTuple.h"
#include "ComTdbHbaseAccess.h"
#include "ComTdbExplain.h"
#include "SchemaDB.h"
#include "ControlDB.h"
#include "NATable.h"
#include "BindWA.h"
#include "ComTransInfo.h"
#include "DefaultConstants.h"
#include "FragDir.h"
#include "PartInputDataDesc.h"
#include "ExpSqlTupp.h"
#include "sql_buffer.h"
#include "ComQueue.h"
#include "ComSqlId.h"
#include "MVInfo.h"
#include "SequenceGeneratorAttributes.h"
#include "CompilationStats.h"
#include "RelRoutine.h"
#include "hs_cont.h"
#include "ComUnits.h"

#include "StmtDDLCleanupObjects.h"

#ifndef HFS2DM
#define HFS2DM
#endif // HFS2DM

#include "ComDefs.h"            // to get common defines (ROUND8)
#include "CmpStatement.h"
#include "ComSmallDefs.h"
#include "sql_buffer_size.h"
#include "ExSqlComp.h"		// for NAExecTrans

#include "ComLocationNames.h"
#include "ComDistribution.h"
#include "OptimizerSimulator.h"

#include "ComCextdecs.h"

#include "TrafDDLdesc.h"

#include "SqlParserGlobals.h"   // Parser Flags

// this comes from GenExplain.cpp (sorry, should have a header file)
TrafDesc * createVirtExplainTableDesc();
void deleteVirtExplainTableDesc(TrafDesc *);

/////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////

 
/////////////////////////////////////////////////////////
//
// MapValueIds::codeGen()
//
/////////////////////////////////////////////////////////
short MapValueIds::codeGen(Generator * generator)
{

  // The MapValueIds node does not result in any executor nodes.  It
  // does not generate a TDB.  It simply 'maps' its lower values to
  // its upper values.  It does this by doing the following for each
  // upper and lower pairing.
  //
  //   - creates a new expressions which converts the lower value to
  //     the type of the upper value.
  //
  //   - Associates this new expression with the ValueId of the upper
  //     value, by replacing the ItemExpr of the upper valueId with
  //     this new expression.
  //
  // The end result is that in nodes above this MapValueIds node,
  // references to the ValueIds of the upper list, will get the new
  // expression (the Cast of the lower value).
  //
  // This code was initially placed in the MapValueIds::preCodeGen(),
  // but this caused some problems with Query trees involving
  // triggers.  The issue was that with triggers it is possible for a
  // ValueId to be sourced (produced) in two different subtrees of the
  // query tree.  One being a MapValueId.  So when the expressions of
  // the upper valueIds were replaced, it not only affected references
  // to the ValueIds above the MapValueIds, it affected the references
  // to the ValueIds in other subtrees.


  // generate code for the child
  child(0)->codeGen(generator);

  const ValueIdList & topValues = map_.getTopValues();

  for (CollIndex i = 0; i < topValues.entries(); i++)
    {
      ValueId valId = topValues[i];

      // if this value id is mapped to a different one (mappedId ) by
      // this node, then convert the "mappedId" to the type of this
      // 'valid'
      //
      ValueId mappedId;
      map_.mapValueIdDown(valId,mappedId);

      if (mappedId != valId)
	{

	  // Convert the source (lower) value to the type of the
          // target (upper) value.
          //
	  ItemExpr * convValue
	    = new(generator->wHeap()) Cast (mappedId.getItemExpr(),
			&(valId.getType()));

	  // bind/type propagate the new node
	  convValue->bindNode(generator->getBindWA());

          // Replace upper value with converted value.
          //
          valId.replaceItemExpr(convValue);

	}
    }

  // No TDB was generated.  Parent will retrieve childs TDB
  //
  return 0;
}

/////////////////////////////////////////////////////////
//
// Some helper classes and functions for PartitionAccess::codeGen
//
/////////////////////////////////////////////////////////

class NodeCountHelper {
friend class NodeHashHelper;
public:
  UInt32 getCount(void) { return nodeDiskCount_;}
  void incCount(void) { nodeDiskCount_++; }
  inline NABoolean operator==(const NodeCountHelper& other) const
  {
    return (nodeDiskCount_ == other.nodeDiskCount_);
  }

private:
  UInt32 nodeDiskCount_;
};

class NodeNameHelper {
friend class NodeHashHelper;
friend ULng32 nodeNameHashFunc( const NodeNameHelper& n );

public:
  const char * const getNodeName(void) { return nodeName_; }
  ULng32 hash() const { return nodeNameHashFunc(*this); }
  inline NABoolean operator==(const NodeNameHelper& other) const
  {
    if (stricmp(nodeName_, other.nodeName_) != 0)
	return FALSE;
    else
        return TRUE;
  }
private:
  char nodeName_[ComGUARDIAN_SYSTEM_NAME_PART_CHAR_MAX_LEN + 1];
};

ULng32 nodeNameHashFunc( const NodeNameHelper& n )
{
  ULng32 retval = 0;
  const char * const c = n.nodeName_;
  Int32 i = 0;
  do {
    retval += c[i++];
  } while (c[i]);
  return retval;
}


class NodeDiskNameHelper {
friend class NodeHashHelper;
friend ULng32 nodeDiskNameHashFunc( const NodeDiskNameHelper& n );

public:
  inline NABoolean operator==(const NodeDiskNameHelper& other) const
  {
    if (stricmp(nodeDiskName_, other.nodeDiskName_) != 0)
	return FALSE;
    else
        return TRUE;
  }

  ULng32 hash() const { return nodeDiskNameHashFunc(*this); }

private:
  char nodeDiskName_[
     ComGUARDIAN_SYSTEM_NAME_PART_CHAR_MAX_LEN + 1    // + 1 for the dot.
   + ComGUARDIAN_VOLUME_NAME_PART_CHAR_MAX_LEN + 1    // + 1 for the \0.
                    ];
};

ULng32 nodeDiskNameHashFunc( const NodeDiskNameHelper& n )
{
  ULng32 retval = 0;
  const char * const c = n.nodeDiskName_;
  Int32 i = 0;
  do {
    retval += c[i++];
  } while (c[i]);
  return retval;
}

class NodeHashHelper {

public:
  NodeHashHelper  ( const char * partn )
    {
      memmove(nodeName_.nodeName_, partn, sizeof nodeName_.nodeName_);
          // Convert 1st dot into a null termininator.
      char * dotPos = strchr(nodeName_.nodeName_, '.');
      *dotPos = '\0';
      memmove(nodeDiskName_.nodeDiskName_, partn,
                                 sizeof nodeDiskName_.nodeDiskName_);
          // Convert 2nd dot into a null termininator.
      dotPos = strchr(nodeDiskName_.nodeDiskName_, '.');
      dotPos = strchr(dotPos+1, '.');
      *dotPos = '\0';
      nodeDiskCount_.nodeDiskCount_ = 1;
    }
  NodeNameHelper * getNodeName(void) { return &nodeName_; }
  NodeDiskNameHelper * getNodeDiskName(void) { return &nodeDiskName_; }
  NodeCountHelper * getNodeDiskCount(void) { return &nodeDiskCount_ ; }
private:
  NodeHashHelper();
  NodeCountHelper nodeDiskCount_ ;
  NodeNameHelper nodeName_;
  NodeDiskNameHelper nodeDiskName_;
};

static void replaceBaseValue(ItemExpr *incomingExpr, 
			     ItemExpr *resultExpr, 
			     CollHeap *heap,
			     NABoolean setFound) 
{
  static THREAD_P NABoolean found = FALSE;

  found = setFound;
  Int32 nc = incomingExpr->getArity();

  // ITM_ASSIGN operator will not be a leaf node.
  if ((nc != 0) && (found == FALSE)) //have not found yet.
     {
       ItemExpr *child1 = incomingExpr->child(1);
       ItemExpr *newExpr = NULL;
       if (incomingExpr->getOperatorType() == ITM_ASSIGN &&
	   (child1 != NULL && child1->getOperatorType() == ITM_ASSIGN)
	   )
	 {
	   newExpr = new(heap) BiArith(ITM_PLUS,
				       resultExpr,
				       child1);

	   newExpr->synthTypeAndValueId(TRUE);
	   //set type to original type after if has been changed to BigNum above
	   newExpr->getValueId().changeType(new (heap) SQLLargeInt(heap, 1 /* signed */,
								    0 /* not null */));
	   incomingExpr->setChild(1,newExpr);
	   found = TRUE;
	   return;
	 }

       for (Lng32 i = 0; i < (Lng32)nc; i++)
	{
	  if (found == FALSE)
	    replaceBaseValue(incomingExpr->child(i), resultExpr, heap, found);
	}
    } 
  return; 
}

short GenericUtilExpr::codeGen(Generator * generator)
{
  GenAssert(0, "GenericUtilExpr::codeGen. Should not reach here.");

  return 0;
}

/////////////////////////////////////////////////////////
//
// DDLExpr::codeGen()
//
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
//
// ExeUtilExpr::codeGen()
//
/////////////////////////////////////////////////////////
const char * DDLExpr::getVirtualTableName()
{ return (producesOutput() ? "DDL_EXPR__" : NULL); }

TrafDesc *DDLExpr::createVirtualTableDesc()
{
  TrafDesc * table_desc = NULL;
  if (producesOutput())
    {
      table_desc = 
        Generator::createVirtualTableDesc(getVirtualTableName(),
                                          NULL, // let it decide what heap to use
                                          ComTdbDDL::getVirtTableNumCols(),
                                          ComTdbDDL::getVirtTableColumnInfo(),
                                          ComTdbDDL::getVirtTableNumKeys(),
                                          ComTdbDDL::getVirtTableKeyInfo());
    }
  return table_desc;
}

short DDLExpr::codeGen(Generator * generator)
{
  Space * space = generator->getSpace();

  generator->verifyUpdatableTransMode(NULL, generator->getTransMode(), NULL);

  // remove trailing blanks and append a semicolon, if one is not present.
  char * ddlStmt = NULL;

  Int32 i = strlen(getDDLStmtText());
  while ((i > 0) && (getDDLStmtText()[i-1] == ' '))
    i--;

  if (getDDLStmtText()[i-1] == ';')
    i--;
 
  ddlStmt = space->allocateAlignedSpace(i+2);
  strncpy(ddlStmt, getDDLStmtText(), i);
  
  // add a semicolon to the end of str (required by the parser)
  ddlStmt[i++]   = ';';
 
  ddlStmt[i] = '\0';

  ex_cri_desc * givenDesc
    = generator->getCriDesc(Generator::DOWN);
  ex_cri_desc * returnedDesc = givenDesc;
  ex_cri_desc * workCriDesc = NULL;
  const Int32 work_atp = 1;
  const Int32 ddl_row_atp_index = 2;
  if (producesOutput())
    {
      // allocate a map table for the retrieved columns
      generator->appendAtEnd();

      returnedDesc = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);
      workCriDesc = new(space) ex_cri_desc(4, space);
      short rc = processOutputRow(generator, work_atp, ddl_row_atp_index,
                                  returnedDesc);
      if (rc)
        {
          return -1;
        }
    }

  NAString catSchName = 
    generator->currentCmpContext()->schemaDB_->getDefaultSchema().getSchemaNameAsAnsiString();
  
  CMPASSERT(!catSchName.isNull());		     // not empty
  CMPASSERT(catSchName.first('.') != NA_NPOS);	     // quick test: 'cat.sch'
  
  char * catSchNameStr = space->allocateAlignedSpace(catSchName.length() + 1);
  strcpy(catSchNameStr, catSchName.data());

  ComTdbDDL * ddl_tdb = NULL;

  if (returnStatus_)
    {
      ComTdbDDLwithStatus *ddl_ws_tdb = new(space)
        ComTdbDDLwithStatus(ddlStmt,
                            strlen(ddlStmt),
                            (Int16)getDDLStmtTextCharSet(),
                            catSchNameStr, strlen(catSchNameStr),
                            0, 0, // no input expr
                            0, 0, // no output expr
                            workCriDesc, (producesOutput() ? ddl_row_atp_index : 0),
                            givenDesc,
                            returnedDesc,
                            (queue_index)getDefault(GEN_DDL_SIZE_DOWN),
                            (queue_index)getDefault(GEN_DDL_SIZE_UP),
                            getDefault(GEN_DDL_NUM_BUFFERS),
                            getDefault(GEN_DDL_BUFFER_SIZE));
      
      ddl_ws_tdb->setReturnStatus(TRUE);

      if (isCleanup_)
        {
          ddl_ws_tdb->setMDcleanup(TRUE);

          StmtDDLCleanupObjects * co = 
            getExprNode()->castToElemDDLNode()->castToStmtDDLCleanupObjects();

          if (co->checkOnly())
            ddl_ws_tdb->setCheckOnly(TRUE);

          if (co->returnDetails())
            ddl_ws_tdb->setReturnDetails(TRUE);

        }
      else if (initHbase())
        ddl_ws_tdb->setInitTraf(TRUE);

      ddl_tdb = ddl_ws_tdb;
    }
  else
    ddl_tdb = new(space)
      ComTdbDDL(ddlStmt,
                strlen(ddlStmt),
                (Int16)getDDLStmtTextCharSet(),
                catSchNameStr, strlen(catSchNameStr),
                0, 0, // no input expr
                0, 0, // no output expr
                workCriDesc, (producesOutput() ? ddl_row_atp_index : 0),
                givenDesc,
                returnedDesc,
                (queue_index)getDefault(GEN_DDL_SIZE_DOWN),
                (queue_index)getDefault(GEN_DDL_SIZE_UP),
                getDefault(GEN_DDL_NUM_BUFFERS),
                getDefault(GEN_DDL_BUFFER_SIZE));
  
  if (isHbase_)
    {
      ddl_tdb->setHbaseDDL(TRUE);
      
      if (hbaseDDLNoUserXn_)
        ddl_tdb->setHbaseDDLNoUserXn(TRUE);
    }
  
  generator->initTdbFields(ddl_tdb);
      
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(ddl_tdb, 0, 0, generator));
  }

  // no tupps are returned
  generator->setCriDesc((ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
			Generator::UP);
  generator->setGenObj(this, ddl_tdb);

  // Set the transaction flag.
  if (xnNeeded())
    {
      if (NOT isHbase_)
	generator->setTransactionFlag(-1);
      else if (getExprNode() && 
               getExprNode()->castToStmtDDLNode()->ddlXns() &&
               (NOT hbaseDDLNoUserXn_))
        {
          // treat like a transactional IUD operation which need to be
          // aborted in case of an error.
          generator->setFoundAnUpdate(TRUE);
	  generator->setUpdAbortOnError(TRUE);
          
          generator->setTransactionFlag(-1);
        }
      else if (NOT hbaseDDLNoUserXn_) 
	generator->setTransactionFlag(-1);
    }

  return 0;
}

/////////////////////////////////////////////////////////
//
// ExeUtilMetadataUpgrade::codeGen()
//
/////////////////////////////////////////////////////////
short ExeUtilMetadataUpgrade::codeGen(Generator * generator)
{
  ExpGenerator * expGen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  // allocate a map table for the retrieved columns
  generator->appendAtEnd();

  ex_cri_desc * givenDesc
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc
    = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);

  ex_cri_desc * workCriDesc = new(space) ex_cri_desc(4, space);
  const Int32 work_atp = 1;
  const Int32 exe_util_row_atp_index = 2;

  short rc = processOutputRow(generator, work_atp, exe_util_row_atp_index,
                              returnedDesc);
  if (rc)
    {
      return -1;
    }

  ComTdbDDLwithStatus * upgd_tdb = new(space)
    ComTdbDDLwithStatus(NULL, 0, 0,
                        NULL, 0, 
                        0, 0, // no input expr
                        0, 0, // no output expr
                        NULL, 0,
                        givenDesc,
                        returnedDesc,
                        (queue_index)getDefault(GEN_DDL_SIZE_DOWN),
                        (queue_index)getDefault(GEN_DDL_SIZE_UP),
                        getDefault(GEN_DDL_NUM_BUFFERS),
                        getDefault(GEN_DDL_BUFFER_SIZE));

  if (getMDVersion())
    upgd_tdb->setGetMDVersion(TRUE);
  else if (getSWVersion())
    upgd_tdb->setGetSWVersion(TRUE);
  else
    upgd_tdb->setMDupgrade(TRUE);

  generator->initTdbFields(upgd_tdb);
  
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(upgd_tdb, 0, 0, generator));
  }

  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);
  
  generator->setGenObj(this, upgd_tdb);

  // Reset the transaction flag.
  generator->setTransactionFlag(0);

  return 0;
}

/////////////////////////////////////////////////////////
//
// FirstN::codeGen()
//
/////////////////////////////////////////////////////////
short FirstN::codeGen(Generator * generator)
{
  ExpGenerator* expGen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  ex_cri_desc * given_desc = generator->getCriDesc(Generator::DOWN);

  // generate code for my child
  child(0)->codeGen(generator);
  ComTdb * child_tdb = (ComTdb *)(generator->getGenObj());
  ExplainTuple *childExplainTuple = generator->getExplainTuple();

  ex_cri_desc * returned_desc = generator->getCriDesc(Generator::UP);

  ex_cri_desc * work_cri_desc = NULL;
  ex_expr * firstNRowsExpr = NULL;
  if (firstNRowsParam_)
    {
      Int32 work_atp = 1; // temps
      Int32 work_atp_index = 2;  // where the result row will be
      work_cri_desc = new(space) ex_cri_desc(3, space);

      // input param is typed as nullable. Make it non-nullable and unsigned.
      NAType * newNAT = 
        firstNRowsParam_->getValueId().getType().newCopy(generator->wHeap());
      newNAT->setNullable(FALSE, FALSE);
      
      Cast * fnp = new(generator->wHeap()) Cast(firstNRowsParam_, newNAT);
      fnp->bindNode(generator->getBindWA());
      
      ValueIdList vidL;
      vidL.insert(fnp->getValueId());

      UInt32 firstNValLen = 0;
      expGen->generateContiguousMoveExpr(vidL,
                                         0, // no convert nodes,
                                         work_atp, work_atp_index,
                                         ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
                                         firstNValLen, &firstNRowsExpr,
                                         NULL, ExpTupleDesc::SHORT_FORMAT);
    }
  
  ComTdbFirstN * firstN_tdb
    = new(space) ComTdbFirstN(
                              child_tdb,
                              getFirstNRows(),
                              firstNRowsExpr,
                              work_cri_desc,
                              given_desc,
                              returned_desc,
                              child_tdb->getMaxQueueSizeDown(),
                              child_tdb->getMaxQueueSizeUp(),
                              1, 4096);
  
  generator->initTdbFields(firstN_tdb);
  
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
                               addExplainInfo(firstN_tdb, childExplainTuple, 0, generator));
  }
  
  generator->setGenObj(this, firstN_tdb);
  
  return 0;
}


/////////////////////////////////////////////////////////
//
// RelRoot::genSimilarityInfo()
//
/////////////////////////////////////////////////////////
TrafQuerySimilarityInfo * RelRoot::genSimilarityInfo(Generator *generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();

  NABoolean recompOnTSMismatch = FALSE;
  NABoolean errorOnTSMismatch = FALSE;

  // generate the similarity info.
  Space * space = generator->getSpace();

  NABoolean disableAutoRecomp   = (CmpCommon::getDefault(AUTOMATIC_RECOMPILATION) == DF_OFF);

  Queue * siList = new(space) Queue(space);
  TrafQuerySimilarityInfo * qsi = NULL;

  if (generator->getTrafSimTableInfoList().entries() > 0)
    qsi = new(space) TrafQuerySimilarityInfo(siList);

  CollIndex i = 0;

  for (CollIndex i = 0; i < generator->getTrafSimTableInfoList().entries(); i++)
    {
      TrafSimilarityTableInfo * genTsi =
	(TrafSimilarityTableInfo *)(generator->getTrafSimTableInfoList()[i]);
      
      char * genTablename =
        space->allocateAndCopyToAlignedSpace(genTsi->tableName(), str_len(genTsi->tableName()), 0);
      char * genRootDir = 
        space->allocateAndCopyToAlignedSpace(genTsi->hdfsRootDir(), str_len(genTsi->hdfsRootDir()), 0);
 
      char * genHdfsHostName =
        space->allocateAndCopyToAlignedSpace(genTsi->hdfsHostName(), str_len(genTsi->hdfsHostName()), 0);
        
      TrafSimilarityTableInfo * si = 
        new(space) TrafSimilarityTableInfo(genTablename,   
                                           genTsi->isHive(),
                                           genRootDir,
                                           genTsi->modTS(),
                                           genTsi->numPartnLevels(),
                                           NULL,
                                           genHdfsHostName,
                                           genTsi->hdfsPort());
      qsi->siList()->insert(si);
    }

  if (qsi)
    {
      qsi->setDisableAutoRecomp(disableAutoRecomp);
    }

  return qsi;
}

short RelRoot::codeGen(Generator * generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  Space * space;
  FragmentDir *compFragDir = generator->getFragmentDir();
  NABoolean childTdbIsNull = FALSE;

  // -- MVs
  // Mark in the generator that we are doing an INTERNAL REFRESH command now.
  if (isRootOfInternalRefresh())
    generator->setInternalRefreshStatement();

  MapTable * map_table = generator->appendAtEnd();

  // create the fragment (independent code space) for the master executor
  CollIndex myFragmentId = compFragDir->pushFragment(FragmentDir::MASTER);

  // When the master executor gets the generated space, it assumes that
  // the root node is stored at the very beginning of the space, so
  // make sure that the master fragment is fragment 0 (the code
  // below makes sure that the root node is at the beginning of its
  // fragment).
  GenAssert(myFragmentId == 0,"NOT myFragmentId == 0");

  // now we can get a hold of the space object for this fragment
  space = generator->getSpace();

  // usually space for a node is generated after generating code for
  // its children. In case of root, generate the root tdb first.
  // This is done so that the start of the generated code space
  // could point to the root tdb. The tdb, however, is not initialized
  // yet.
  ComTdbRoot *root_tdb = new(space) ComTdbRoot();
  generator->initTdbFields(root_tdb);

  // tell the root tdb whether we collect statistics or not
  if (generator->computeStats())
    {
      root_tdb->setCollectStats(generator->computeStats());
      root_tdb->setCollectStatsType(generator->collectStatsType());
      root_tdb->setCollectRtsStats(generator->collectRtsStats());
    }

   

  //For consistent display of overflow_mode in stats.
  root_tdb->setOverflowMode(generator->getOverflowMode());

    // set the object for the top level fragment
  compFragDir->setTopObj((char *) root_tdb);

  // Copy the current context-wide TransMode,
  // then overlay with this stmt's "FOR xxx ACCESS" setting, if any.
  TransMode * transMode = new(space) TransMode();
  transMode->updateTransMode(generator->getTransMode());
  //
  if (accessOptions().accessType() != TransMode::ACCESS_TYPE_NOT_SPECIFIED_)
    {
      // "FOR xxx ACCESS" becomes an IsolationLevel, and both IL and AccessMode
      // are set in the transMode
      transMode->updateAccessModeFromIsolationLevel(
        TransMode::ATtoIL(accessOptions().accessType()));

      transMode->setStmtLevelAccessOptions();
    }
  else if ( ( generator->currentCmpContext()->internalCompile() == CmpContext::INTERNAL_MODULENAME) ||
            ( CmpCommon::statement()->isSMDRecompile() )
          )
    {
      // As a nicety to everyone writing a trusted .mdf (RFORK, etc),
      // we set this flag so that cli/Statement::execute() will not
      // recompile those trusted stmts due to any TransMode mismatch.
      // (Otherwise, everyone would need to add "FOR xxx ACCESS" to each stmt!)
      //
      transMode->setStmtLevelAccessOptions();
    }

  ex_expr * input_expr = 0;
  ex_expr * output_expr = 0;
  CollIndex i;

  ex_expr * pkey_expr = NULL;
  ULng32 pkey_len = 0;
  
  ex_expr* pred_expr = NULL;

  ULng32 cacheVarsSize = 0;
  //  unsigned long tablenameCacheVarsSize = 0;

  // max number of rows in user rowwise rowset.
  Lng32 rwrsMaxSize = 0;

  // index into the user params to find the value of the number of
  // actual rows in the rowwise rowset buffer.
  short rwrsInputSizeIndex = 0;

  // index into the user params to find the value of the max length
  // of each row in the user rowwise rowset buffer.
  short rwrsMaxInputRowlenIndex = 0;

  // index into the user params to find the value of the address
  // of rowwise rowset buffer in user space.
  short rwrsBufferAddrIndex = 0;

  // index into user params to find the value of the partition number
  // where this rwrs need to be shipped to.
  short rwrsPartnNumIndex = -1; // not specified

  // length of the each internal tuple where user's row will be moved in
  // at runtime.
  Lng32 rwrsMaxInternalRowlen = 0;

  RWRSInfo *rwrsInfo = NULL;
  char *rwrsInfoBuf = NULL;
  if (getHostArraysArea() && getHostArraysArea()->getRowwiseRowset())
    {
      rwrsInfo = (RWRSInfo *) new (space) char[sizeof(RWRSInfo)];

      rwrsInfoBuf = (char*)rwrsInfo;

      rwrsMaxSize = 
	(Lng32)((ConstValue*)getHostArraysArea()->rwrsMaxSize())->
	getExactNumericValue();

      NABoolean packedFormat      = FALSE;
      NABoolean compressed        = FALSE;
      NABoolean dcompressInMaster = FALSE;
      NABoolean compressInMaster  = FALSE;
      NABoolean partnNumInBuffer  = FALSE;
      getHostArraysArea()->getBufferAttributes(packedFormat,
					       compressed, dcompressInMaster,
					       compressInMaster,
					       partnNumInBuffer);
      rwrsInfo->setRWRSisCompressed(compressed);
      rwrsInfo->setDcompressInMaster(dcompressInMaster);
      rwrsInfo->setPartnNumInBuffer(partnNumInBuffer);
    }

  // inputVars() can contain multiple references to the same
  // param/hostvar value, if it is specified more than once in
  // a statement. (Seems like a bug to me, but no one is around
  // to fix it, so i will just work around it). The CharacteristicInputs
  // do not contain duplicate references. Use them to create a non-duplicate
  // newInputVars list.
  ValueIdList newInputVars, cacheVars, rwrsVars;
  NABoolean userInputVars = FALSE;
  short entry = 1;
  for (i = 0; i < inputVars().entries(); i++)
  {
    // CharacteristicInputs contains constants. Don't add
    // them as input. Add hostvar/params to map table, if not
    // already added. This will remove duplicates.
    // Add the non-duplicate input val-ids to the newInputVars list.
    ValueId val_id = inputVars()[i];
    ItemExpr * item_expr = val_id.getItemExpr();
    
    // We create a dummy host var in case it gets a value id from an
    // assignment in compound statements. Such variable will not need
    // to be processed by Cli since it gets its value inside the statement
    NABoolean blankHV = FALSE ;
    if (item_expr->previousHostVar()) {
      Int32 j = 0;
      for (j = 0; j < i; j++) {
	ItemExpr *ie = inputVars()[j].getItemExpr();
	if (ie->getOperatorType() == ITM_HOSTVAR) {
	  if (item_expr->previousName() == ((HostVar *) ie)->getName()) {
	    break;
	  }
	}
      }
      
      if (i == j) {
	NAString str1 = "previousHV__";
	char str2[30];
	str_itoa(i, str2);
	str1 += str2;
	item_expr = new(generator->wHeap()) HostVar(str1,
						    new(generator->wHeap()) SQLUnknown(generator->wHeap()));
	item_expr->bindNode(generator->getBindWA());
	blankHV = TRUE;
	val_id = item_expr->getValueId();
      }
    }
    
    OperatorTypeEnum op = item_expr->getOperatorType();
    
    if ((op == ITM_HOSTVAR) ||
	(op == ITM_DYN_PARAM)){
      userInputVars = TRUE;
      
      // Vicz: filter out the OUT HostVar/DynamicParam
      
      ComColumnDirection paramMode = item_expr->getParamMode();
      
      if(paramMode == COM_OUTPUT_COLUMN)
	continue;
      
    }
    
    // the list of operator types that was present here in R1.8 code in
    // almost equivalent to isAUserSuppliedInput()
    // except for Constant which cannot be handled here as its atpindex
    // should be 0 and not 2.
    // The num_tupps local variable just outside this IF will create
    // attributes with atpindex 2.
    // Constants are added later to the MapTable.
    if (((item_expr->isAUserSuppliedInput()) && // for evaluate once functions
	 (op != ITM_CONSTANT)) &&
	(! generator->getMapInfoAsIs(val_id))) // not added yet
      {
	MapInfo *map = generator->addMapInfoToThis(generator->getLastMapTable(),
						   val_id, NULL);
	
	// Transfer the information on rowsets that is in this host variable
	// into its attribute so we know this information at run time
	if (op == ITM_HOSTVAR || op == ITM_DYN_PARAM)
	  {
	    Attributes *attr = map->getAttr();
	    UInt32 rowsetInfo;
	    
	    if (op == ITM_HOSTVAR)
	      {
		HostVar *hv = (HostVar *) (val_id.getItemExpr());
		rowsetInfo = hv->getRowsetInfo();
		if (blankHV)
		  {
		    attr->setBlankHV();
		  }
	      }
	    else  // (op == ITM_DYN_PARAM)
	      {
		DynamicParam *dp = (DynamicParam *) (val_id.getItemExpr());
		rowsetInfo = dp->getRowsetInfo();

		if (dp->isDPRowsetForInputSize())
		  rwrsInputSizeIndex = entry;
		else if (dp->isRowwiseRowsetInputMaxRowlen())
		  rwrsMaxInputRowlenIndex = entry;
		else if (dp->isRowwiseRowsetInputBuffer())
		  rwrsBufferAddrIndex = entry;
		else if (dp->isRowwiseRowsetPartnNum())
		  rwrsPartnNumIndex = entry;
	      }
	    
	    attr->setRowsetInfo((Int16)rowsetInfo);
	  }
	
	if ((op == ITM_DYN_PARAM) &&
	    ((DynamicParam *)item_expr)->isRowInRowwiseRowset())
	  {
	    rwrsVars.insert(val_id);
	  }
	else if (op == ITM_CACHE_PARAM)
	  {
	    // This is a parameter generated by Query Caching
	    cacheVars.insert(val_id);
	  }
	else
	  {
	    newInputVars.insert(val_id);
	    entry++;
	  }
      }
  } // for
  
  Int32 num_tupps = 2; /* atp_index 0 for constants, 1 for temps */

  // create a row(tuple) with input param/hostvar values and pass
  // it to the child.
  // assign offset to elements in the input vars list.
  // Offsets are assigned in the input row tuple (atp index = 2).
  Attributes ** attrs = new(generator->wHeap())
	Attributes * [newInputVars.entries()];
  for (i = 0; i < newInputVars.entries(); i++)
    {
      attrs[i] = generator->addMapInfo(newInputVars[i], NULL)->getAttr();
    }

  ULng32 input_vars_size = 0;
  exp_gen->processAttributes(newInputVars.entries(), attrs,
			     ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			     input_vars_size, 0 /*atp*/, num_tupps/*atpIdx*/);

  //++Triggers,
  // Save the offsets of triggerStatus and UniqueExecuteId,
  // so ex_root_tcb can set it's value
  Lng32 triggersStatusOffset = -1;
  Lng32 uniqueExecuteIdOffset = -1;

  for (i = 0; i < newInputVars.entries(); i++)
    {
      ItemExpr * item_expr = (newInputVars)[i].getItemExpr();
      if (item_expr->getOperatorType() == ITM_UNIQUE_EXECUTE_ID)
	uniqueExecuteIdOffset = attrs[i]->getOffset();
      if (item_expr->getOperatorType() == ITM_GET_TRIGGERS_STATUS)
	{
	  GenAssert(getTriggersList()->entries()>0,
		    "No triggers, yet TriggerStatusOffset != -1");
	  triggersStatusOffset = attrs[i]->getOffset();
	}
    }
  
  //--Triggers,

  num_tupps += ((newInputVars.entries() > 0) ? 1 : 0); // plus 1 to hold the input
                                                      // params and hostvars.

  if (updateCurrentOf())
    {
      GenAssert(pkeyList().entries() > 0, "pkeyList().entries() must be > 0");

      // create a row(tuple) with pkey hostvar values and pass
      // it to the child.
      // assign offset to elements in the pkeyList.
      // Offset is assigned at atp index which one greater than where
      // the input num_tupps is.
      Attributes ** attrs = new(generator->wHeap())
	Attributes * [pkeyList().entries()];
      for (i = 0; i < pkeyList().entries(); i++)
	{
	  attrs[i] = generator->addMapInfo(pkeyList()[i], NULL)->getAttr();
	}

      exp_gen->processAttributes(pkeyList().entries(), attrs,
				 ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
				 pkey_len,0/*atp*/,num_tupps/*atpIdx*/);
      num_tupps += 1;
    }

  // Process expressions generated by Query Caching.
  Attributes ** cachedAttrs = NULL;
  if (cacheVars.entries() > 0) {
    cachedAttrs = new(generator->wHeap())
	Attributes * [cacheVars.entries()];
    for (i = 0; i < cacheVars.entries(); i++)
    {
      cachedAttrs[i] = generator->addMapInfo(cacheVars[i], NULL)->getAttr();
    }

    exp_gen->processAttributes(cacheVars.entries(), cachedAttrs,
			       ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			       cacheVarsSize,0/*atp*/,num_tupps/*atpIdx*/);
    num_tupps += 1;
  }

  // Space needed for query caching. This is where the values of ConstantParameters
  // will go
  //  char *queryCacheParameterBuffer = (char *) new (space) char[cacheVarsSize];
  NABoolean qCacheInfoIsClass = FALSE;
  if (CmpCommon::getDefault(QUERY_CACHE_RUNTIME) == DF_ON)
    qCacheInfoIsClass = TRUE;

  char *parameterBuffer = (char *) new (space) char[cacheVarsSize];
  char *qCacheInfoBuf = NULL;
  if (qCacheInfoIsClass)
    {
      QCacheInfo *qCacheInfo =
	(QCacheInfo *) new (space) char[sizeof(QCacheInfo)];
      qCacheInfo->setParameterBuffer(parameterBuffer);
      qCacheInfoBuf = (char*)qCacheInfo;
    }
  else
    qCacheInfoBuf = parameterBuffer;

  // Check for reasons why the query plan should not be cached.
  // Note: This does not influence the use of cache parameters,
  // it's too late at this time to undo that.
  const LIST(CSEInfo *) *cseInfoList = CmpCommon::statement()->getCSEInfoList();

  if (cseInfoList &&
      CmpCommon::getDefault(CSE_CACHE_TEMP_QUERIES) == DF_OFF)
    for (CollIndex i=0; i<cseInfoList->entries(); i++)
      if (cseInfoList->at(i)->usesATempTable())
        generator->setNonCacheableCSEPlan(TRUE);

  // compute offsets for rwrs attrs. Offsets are computed separately
  // for rwrs vars since these values will be moved as part of input
  // row at runtime. This input row should only contain values which are
  // being inserted (ex, in the VALUES clause) and not any other input
  // values (like, input size, buffer, etc). 
  // If rwrs vars was included in newInputVars before computing
  // the offsets, then these offsets will also include the non-rwrs
  // vars which will not be correct.
  //
  // Do not assign any atp index at this time.
  // atp index will be determined at runtime
  // when the actual rows that are extracted from the rowset, 
  // processed and moved up the queue.
  Attributes ** rwrsAttrs = NULL;

  // next var is used if buffer need to be decompressed using the unicode
  // decoding alogorithm. 
  NABoolean useUnicodeDcompress = FALSE;

  if (rwrsVars.entries() > 0) 
    {
      rwrsAttrs = new(generator->wHeap())
	Attributes * [rwrsVars.entries()];
      for (i = 0; i < rwrsVars.entries(); i++)
	{
	  rwrsAttrs[i] = generator->addMapInfo(rwrsVars[i], NULL)->getAttr();

	  if (rwrsAttrs[i]->getCharSet() != CharInfo::ISO88591)
	    useUnicodeDcompress = TRUE;
	}
      
      // assign offsets.
      // No real atp index is to be assigned.
      // Cannot make it -1 as processAttrs doesn't like that.
      // Make atp index to be SHRT_MAX (out of reach).
      ULng32 len;
      exp_gen->processAttributes(rwrsVars.entries(), rwrsAttrs,
				 ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
				 len, 
				 0/*atp*/, SHRT_MAX/*atpIdx*/);
      rwrsMaxInternalRowlen = len;
    }

  if (rwrsInfo)
    {
      rwrsInfo->setRwrsInfo(rwrsMaxSize, rwrsInputSizeIndex,
			    rwrsMaxInputRowlenIndex, rwrsBufferAddrIndex,
			    rwrsPartnNumIndex,
			    rwrsMaxInternalRowlen);

      if (rwrsInfo->rwrsIsCompressed())
	rwrsInfo->setUseUnicodeDcompress(useUnicodeDcompress);
    }

  // generate the input expression to move in hostvar/param values
  // from user area.
  // Include rwrs in newInputVars before generating the input expr
  // since we want all of these vars to be returned to the used when
  // they are 'described' at runtime.
  if ((newInputVars.entries() > 0) ||
      (rwrsVars.entries() > 0))
    {
      newInputVars.insert(rwrsVars);
      exp_gen->generateInputExpr(newInputVars, ex_expr::exp_INPUT_OUTPUT,
				 &input_expr);
    }

  ex_cri_desc * cri_desc = new(space) ex_cri_desc(num_tupps, space);
  generator->setCriDesc(cri_desc, Generator::DOWN);
  generator->setInputExpr((void *)input_expr);

  ExplainDesc *explainDesc = NULL;

  if(!generator->explainDisabled())
    {
      // Create a space object for the explain fragment
      if(generator->getExplainFragDirIndex() == NULL_COLL_INDEX)
	{
	  // Create an Explain Fragment
	  generator->setExplainFragDirIndex(
	       generator->getFragmentDir()->pushFragment(FragmentDir::EXPLAIN,0));
	  generator->getFragmentDir()->popFragment();
	}

      ExplainFunc explainFunc;
      TrafDesc *explainDescStruct = explainFunc.createVirtualTableDesc();

      Space *explainSpace = generator->getFragmentDir()->
	getSpace(generator->getExplainFragDirIndex());

      TrafTableDesc *tableDesc = explainDescStruct->tableDesc();

      // Determine the length of the Explain Tuple.
      Lng32 recLength = tableDesc->record_length;

      // Determine the number of columns in the Explain Tuple.
      Lng32 numCols = tableDesc->colcount;

      explainDesc =
	new(explainSpace) ExplainDesc(numCols, recLength, explainSpace);

      TrafDesc *cols = tableDesc->columns_desc;

      // For each column of the Virtual Explain Table, extract the
      // relevant info. from the table desc and put it into the ExplainDesc
      for(Int32 c = 0; c < numCols; c++ /* no pun intended */)
	{
	  TrafColumnsDesc *colsDesc = (cols->columnsDesc());

	  explainDesc->setColDescr(c,
				   colsDesc->datatype,
				   colsDesc->length,
				   colsDesc->offset,
				   colsDesc->isNullable());

	  cols = cols->next;
	}

      explainFunc.deleteVirtualTableDesc(explainDescStruct);

      compFragDir->setTopObj(generator->getExplainFragDirIndex(),
			     (char *)explainDesc);
    }

  // Take note of whether this is a parallel extract query before
  // generating the child tree.
  NABoolean isExtractProducer = (numExtractStreams_ > 0 ? TRUE : FALSE);
  NABoolean isExtractConsumer =
    (childOperType() == REL_EXTRACT_SOURCE ? TRUE : FALSE);

  // the tree below needs to know if this is a LRU operation, hence
  // make this check before the children are codeGened.
  if (containsLRU())
  {
    generator->setLRUOperation(TRUE);
  }
  if (getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_) 
  {
    generator->setTolerateNonFatalError(TRUE);
  }

  // Copy #BMOs value from Root node into the fragment
  compFragDir->setNumBMOs(myFragmentId, getNumBMOs());
  compFragDir->setBMOsMemoryUsage(myFragmentId, getBMOsMemoryUsage().value());

  // generate child tree
  child(0)->codeGen(generator);
  ComTdb * child_tdb = (ComTdb *)(generator->getGenObj());
  if (child_tdb == (ComTdb *)NULL)
      childTdbIsNull = TRUE;


  // Remap the allocation of ESPs to Nodes/CPUs.
  if (ActiveSchemaDB()->getDefaults().getAsLong(AFFINITY_VALUE) == -2)
     generator->remapESPAllocationRandomly();
  else
     generator->remapESPAllocationAS();

  generator->compilerStatsInfo().affinityNumber()
    = generator->getAffinityValueUsed();

  // if an output expression is present, generate it.
  if (compExpr_.entries() > 0)
  {
    // Special cases to consider are
    // * stored procedure result sets
    // * parallel extract consumers
    // 
    // In these plans we want special table and column names in the
    // output expression. The names will come from the root's child
    // node and be pointed to by these two variables. For all other
    // ("normal") statements, these two pointers will remain NULL.
    //
    ConstNAStringPtr *colNamesForExpr = NULL;
    ConstQualifiedNamePtr *tblNamesForExpr = NULL;

    OperatorTypeEnum childType =
      child(0)->castToRelExpr()->getOperatorType();

    ComUInt32 numColumns = getRETDesc()->getDegree();

    if ((childType == REL_SP_PROXY || isExtractConsumer) &&
        numColumns > 0)
    {
      ProxyFunc *proxy;

      if (childType == REL_SP_PROXY)
      {
        // This is a stored procedure result set
        proxy = (ProxyFunc *) child(0)->castToRelExpr();
      }
      else
      {
        // This is an extract consumer. The extract operator is not
        // the direct child of the root. An exchange operator sits
        // between the two.
        GenAssert(childType == REL_EXCHANGE,
                  "Child of root should be exchange for consumer query");
        GenAssert(child(0)->child(0),
                  "Child of root should not be a leaf for consumer query");
                  
        OperatorTypeEnum grandChildType = 
          child(0)->child(0)->castToRelExpr()->getOperatorType();

        GenAssert(grandChildType == REL_EXTRACT_SOURCE,
                  "Grandchild of root has unexpected type for consumer query");

        proxy = (ProxyFunc *) child(0)->child(0)->castToRelExpr();
      }

      // Populate the table and column name collections that will be
      // used below to generate the output expression.
      colNamesForExpr = new (generator->wHeap())
        ConstNAStringPtr[numColumns];
      tblNamesForExpr = new (generator->wHeap())
        ConstQualifiedNamePtr[numColumns];

      for (ComUInt32 i = 0; i < numColumns; i++)
      {
        colNamesForExpr[i] = proxy->getColumnNameForDescriptor(i);
        tblNamesForExpr[i] = proxy->getTableNameForDescriptor(i);
      }
    }

    exp_gen->generateOutputExpr(compExpr_,
                                ex_expr::exp_INPUT_OUTPUT,
                                &output_expr,
                                getRETDesc(), 
                                getSpOutParams(),
                                colNamesForExpr,
                                tblNamesForExpr);
  }

  if (getPredExprTree())
    {
      //      ItemExpr * newPredTree = executorPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(getPredExprTree()->getValueId(), ex_expr::exp_SCAN_PRED,
			    &pred_expr);
    }

  // if child's primary key columns are to be returned to be passed
  // on to UPDATE WHERE CURRENT OF query, generate an
  // expression to compute the pkey row.
  ex_cri_desc * work_cri_desc = NULL;

  if (updatableSelect() == TRUE)
    {
      GenAssert(pkeyList().entries() > 0, "pkeyList().entries() must be > 0");

      work_cri_desc = new(space) ex_cri_desc(3, space);

      exp_gen->generateContiguousMoveExpr(pkeyList(),
					  1, // add convert nodes,
					  1, 2,
					  ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
					  pkey_len, &pkey_expr,
					  NULL, ExpTupleDesc::SHORT_FORMAT);
    }

  // if this is an 'update where current of' query, pass in
  // the fetched cursor name or hvar number root tdb.
  char * fetchedCursorName = NULL;
  short  fetchedCursorHvar = -1;
  if (updateCurrentOf())
    {
      if (currOfCursorName_->getOperatorType() == ITM_CONSTANT)
	{
	  // cursor name is specified as a literal
	  NABoolean tv;
	  ConstValue * cv = currOfCursorName_->castToConstValue(tv);

	  fetchedCursorName =
	    space->allocateAndCopyToAlignedSpace((char *)(cv->getConstValue()),
						 cv->getStorageSize(),
						 0);
	}
      else
	{
	  // cursor name was specified as a hvar
	  HostVar * cursorHvar = (HostVar *)currOfCursorName_;

	  // search for this hostvar in the input hvar list
	  for (i = 0; i < inputVars().entries(); i++)
	    {
	      ValueId val_id = inputVars()[i];
	      if (cursorHvar->getOperatorType() == ITM_HOSTVAR)
		{
		  HostVar * hv = (HostVar *)(val_id.getItemExpr());

		  if (hv->getName() == cursorHvar->getName())
		    fetchedCursorHvar = (short)i+1; // 1-based
		}
	    } // more input

	} // cursor name is in a hvar
    }

  // Create a list of update columns for UPDATE CURRENT OF or updateable
  // SELECT statements (cursors). For UPDATE CURRENT OF, the update columns
  // are obtained from the GenericUpate Node. For cursors, the update columns
  // are obtained from this (RelRoot) node.
  Lng32 numUpdateCol = 0;
  Lng32 *updateColList = NULL;

  if (updateCurrentOf()) // UPDATE/DELETE ... where CURRENT OF query.
  {
    GenAssert(updateCol().entries() == 0,
              "UPDATE CURRENT OF: updateCol non-zero");

    // Get the update node.
    GenericUpdate *update = (GenericUpdate*)generator->updateCurrentOfRel();

    GenAssert(update != NULL, "UPDATE CURRENT OF: NULL update node");

    /*
    // create the update col list for UPDATE...WHERE CURRENT OF CURSOR only.
    // The updateCurrentOf() is set for both UPDATE and DELETE queries.
    if ((update->getOperatorType() == REL_DP2_UPDATE_CURSOR) ||
	(update->getOperatorType() == REL_DP2_UPDATE_UNIQUE))
      {
	// Get the list of assignment expressions from the STOI of the
	// update node.
        SqlTableOpenInfo *updateStoi = update->getOptStoi()->getStoi();

	// Allocate an array for the column list.
	numUpdateCol = updateStoi->getColumnListCount();

	GenAssert(numUpdateCol > 0,
                  "UPDATE CURRENT OF: No update columns");

	updateColList = new(space) Lng32[numUpdateCol];

	// Populate the array with the update columns from the left side
	// of each expression which is a column.
	for (i = 0; (Lng32)i < numUpdateCol; i++)
	  {
            updateColList[i] = updateStoi->getUpdateColumn(i);
	  }
      } // update...where current of.
    */
  }
  else if (updatableSelect())
    {
      numUpdateCol = updateCol().entries();
      
      if (numUpdateCol > 0)
        {
          // Allocate an array for the column list.
      updateColList = new(space) Lng32[numUpdateCol];

      // Populate the array with the update columns of this node.
      for (i = 0; (Lng32)i < numUpdateCol; i++)
      {
        ValueId val_id = updateCol()[i];
        GenAssert(val_id.getItemExpr()->getOperatorType() == ITM_BASECOLUMN,
                  "UpdateCol should be BaseColumn");
        BaseColumn *col = (BaseColumn*)val_id.getItemExpr();
        updateColList[i] = col->getColNumber();
      }
    }
    else
    {
      // All columns are updateable.
      numUpdateCol = -1;
    }
  }

  // copy all the tables open information into the generator space
  // and pass it to the root tdb, which is used to check the
  // security when opening the sql tables or views.
  short noOfTables = (short) generator->getSqlTableOpenInfoList().entries();
  SqlTableOpenInfo **stoiList;
  stoiList = new (space) SqlTableOpenInfo*[noOfTables];
  
  // The Executor Statement class has logic to retry blown away
  // opens on some statements. We can safely do so if we know that
  // disk has not been dirtied, and no rows have been returned
  // to the application.

  // The Executor can deduce this if the blown away open occurs
  // on the first input row, and if the plan writes to at most
  // one object. This follows because an open can be blown away
  // only if no locks are held on any partition of the object.

  // The two variables below are used to compute if the plan
  // accesses more than one object. If not, and if the query is
  // an IUD, we will flag the root tdb as retryable.

  NABoolean moreThanOneTable = FALSE;
  char *firstTable = NULL;

  // ++ Triggers
  // While copying the stoi info, make sure the subjectTable flag is
  // not set more than once for the same table ansi name
  LIST(NAString) *subjectTables = NULL;
  if (getTriggersList())
    subjectTables = new (generator->wHeap())
      LIST(NAString) (generator->wHeap());
  short j=0;

  for (; j < noOfTables; j++)
  {
    stoiList[j] = new (space) SqlTableOpenInfo;
    SqlTableOpenInfo * genStoi =
		(SqlTableOpenInfo *)generator->getSqlTableOpenInfoList()[j];
    *(stoiList[j]) = *genStoi;

    if (moreThanOneTable)
    {
      // No need to check any further.
    }
    else
    {
      if (firstTable == NULL)
      {
        firstTable = genStoi->fileName();
      }
      else if (stricmp( firstTable, genStoi->fileName()))
      {
        // there is more than one distinct object name in the Stoi list
        moreThanOneTable = TRUE;
      }
    }
    stoiList[j]->setFileName(new (space) char[strlen(genStoi->fileName()) + 1]);
    strcpy(stoiList[j]->fileName(), genStoi->fileName());

    stoiList[j]->setAnsiName(new (space) char[strlen(genStoi->ansiName()) + 1]);
    strcpy(stoiList[j]->nonConstAnsiName(), genStoi->ansiName());

    // -- Triggers
    // Prevent duplicate entries for the same subject table
    // and entries for views
    NAString const ansiName(genStoi->ansiName(), generator->wHeap());
    if (genStoi->subjectTable() && subjectTables &&
	!(subjectTables->contains(ansiName)) && !genStoi->isView())
    {
      stoiList[j]->setSubjectTable(TRUE);
      subjectTables->insert(ansiName);
    }
    else
      stoiList[j]->setSubjectTable(FALSE);

    if (genStoi->getColumnListCount())
    {
      stoiList[j]->setColumnList(new (space)
                     short[genStoi->getColumnListCount()]);
      for (short k = 0; k < genStoi->getColumnListCount(); k++)
      {
        stoiList[j]->setUpdateColumn(k,genStoi->getUpdateColumn(k));
      }
    }
  }

  // copy the triggers list into the generator space
  // and pass it to the root tdb, where it is used to check the
  // enable/disable status of triggers.
  short triggersCount = 0;
  ComTimestamp *triggersList = NULL;
  if (getTriggersList())
  {
    GenAssert(subjectTables && (subjectTables->entries() > 0),
	      "Mismatch: Triggers without Subject Tables");
    delete subjectTables;
    subjectTables = NULL;

    triggersCount = (short) getTriggersList()->entries();
    triggersList = new (space) ComTimestamp[triggersCount];

    for (short k=0; k < triggersCount; k++)
      triggersList[k] = getTriggersList()->at(k);
  }

  // copy uninitializedMvList into generator space 
  // to pass to root tdb  
  UninitializedMvName *newMvList = NULL; 
  short uninitializedMvCount = 0;  
  
  if (uninitializedMvList_) 
  {            
      uninitializedMvCount = (short)uninitializedMvList_->entries();
      if( uninitializedMvCount != 0 )
      {
          newMvList = new (space) UninitializedMvName[uninitializedMvCount];
          for( short i = 0; i < uninitializedMvCount; i++ )
          {          
              UninitializedMvName *pMvName = uninitializedMvList_->at(i);

              GenAssert( pMvName, "UninitializedMvName is invalid." );

              newMvList[i].setPhysicalName( pMvName->getPhysicalName() );
              newMvList[i].setAnsiName( pMvName->getAnsiName() );                            
          }
      }
  } 


  // if there were any views referenced in the query, copy the stoi
  // to root tdb. This is used at runtime to check for existence.
  Queue * viewStoiList = NULL;
  if (getViewStoiList().entries() > 0)
    {
      for (CollIndex i = 0; i < getViewStoiList().entries(); i++)
	{

	  if (! viewStoiList)
	    viewStoiList = new(space) Queue(space);

	  SqlTableOpenInfo * stoi = new(space) SqlTableOpenInfo;
	  *stoi = *getViewStoiList()[i]->getStoi();

	  stoi->setFileName(
	    new(space)  char[strlen(getViewStoiList()[i]->getStoi()->fileName()) + 1]);
	  strcpy(stoi->fileName(), getViewStoiList()[i]->getStoi()->fileName());

	  stoi->setAnsiName(
	    new(space)  char[strlen(getViewStoiList()[i]->getStoi()->ansiName()) + 1]);
	  strcpy(stoi->nonConstAnsiName(), getViewStoiList()[i]->getStoi()->ansiName());

          if (getViewStoiList()[i]->getStoi()->getColumnListCount())
	  {
	    stoi->setColumnList(new (space)
	      short[getViewStoiList()[i]->getStoi()->getColumnListCount()]);
	    for (short k = 0; k < getViewStoiList()[i]->getStoi()->getColumnListCount(); k++)
	      {
		stoi->setUpdateColumn(k,getViewStoiList()[i]->getStoi()->getUpdateColumn(k));
	      }
	  }

	  if (CmpCommon::getDefault(VALIDATE_VIEWS_AT_OPEN_TIME) == DF_ON)
	    stoi->setValidateViewsAtOpenTime(TRUE);
          else
            stoi->setValidateViewsAtOpenTime(FALSE);

	  viewStoiList->insert(stoi);

	  // if this view name was used as a variable(hvar, envvar or define),
	  // then add it to the latename info list.
	  HostVar * hv = getViewStoiList()[i]->getCorrName().getPrototype();
	  if (hv != NULL)
	    {
	      LateNameInfo* lateNameInfo = new(generator->wHeap()) LateNameInfo();

	      char * varName;
	      GenAssert(hv->getName().data(), "Hostvar pointer must have name");

	      lateNameInfo->setEnvVar(hv->isEnvVar());

	      lateNameInfo->setCachedParam(hv->isCachedParam());

	      varName = convertNAString(hv->getName(), generator->wHeap());
	      strcpy(lateNameInfo->variableName(), varName);

	      char * prototypeValue =  convertNAString(hv->getPrototypeValue(),
						       generator->wHeap());
	      char * compileTimeAnsiName = prototypeValue;
	      lateNameInfo->setVariable(1);

	      lateNameInfo->setView(1);

              lateNameInfo->setCompileTimeName(compileTimeAnsiName, space);
	      lateNameInfo->setLastUsedName(compileTimeAnsiName, space);
	      lateNameInfo->setNameSpace(COM_TABLE_NAME);

	      lateNameInfo->setInputListIndex(-1);

	      generator->addLateNameInfo(lateNameInfo);

	    } // hv
	} // for
    }

  // UDR Security
  short noOfUdrs = generator->getBindWA()->getUdrStoiList().entries ();

  SqlTableOpenInfo **udrStoiList = NULL;
  if ( noOfUdrs )
  {
    udrStoiList = new (space) SqlTableOpenInfo*[noOfUdrs];
    BindWA *bindWA = generator->getBindWA ();

    for (short udrIdx=0; udrIdx < noOfUdrs; udrIdx++)
    {
      udrStoiList[udrIdx] = new (space) SqlTableOpenInfo;
      SqlTableOpenInfo *genUdrStoi =
       (SqlTableOpenInfo *)bindWA->getUdrStoiList()[udrIdx]->getUdrStoi();

      *(udrStoiList[udrIdx]) = *genUdrStoi;

      udrStoiList[udrIdx]->setAnsiName(
			 new (space)  char[strlen(genUdrStoi->ansiName()) + 1]
				      );
      strcpy(udrStoiList[udrIdx]->nonConstAnsiName(), genUdrStoi->ansiName());
    }
  }

  // setting transaction type flags in the transmode object
  // Determines what type of transaction will be started for this statement,
  // if autocommit is ON.

  // setting accessMode to read_write if it was set to read_only by MX.
  // if isolation_level is read_uncommitted we set accessmode to read only
  // This causes trouble when we try to do DDL or IUD so we are resetting
  // accessMode here. From here on accesMode is used only to start the transaction
  if ((transMode->accessMode() == TransMode::READ_ONLY_) &&
      (generator->needsReadWriteTransaction()))
	transMode->accessMode() = TransMode::READ_WRITE_ ;


  if (generator->withNoRollbackUsed() ||
      (transMode->getRollbackMode() == TransMode::NO_ROLLBACK_))
  {
      if (generator->withNoRollbackUsed())
        transMode->rollbackMode() = TransMode::NO_ROLLBACK_IN_IUD_STATEMENT_ ;

      //      if (childOperType().match(REL_ANY_GEN_UPDATE))
      //        generator->setAqrEnabled(FALSE);

      // AIInterval is set to don't abort (i.e. 0). 
      // A setting of -2 is equivalent to a setting of 0 but has the additional meaning
      // that it will never be overriden by the executor.
      if (transMode->getAutoAbortIntervalInSeconds() == -1)
        transMode->autoAbortIntervalInSeconds() = -2;
  }
  else if ((NOT generator->needsReadWriteTransaction()) &&
	    transMode->accessMode() != TransMode::READ_ONLY_SPECIFIED_BY_USER_ &&
	    containsLRU() == FALSE &&
	    updatableSelect() == FALSE)
  {
    transMode->accessMode() = TransMode::READ_ONLY_ ;
  }

  if (transMode->getAutoAbortIntervalInSeconds() == -1)
  {
      if (transMode->accessMode() == TransMode::READ_ONLY_SPECIFIED_BY_USER_)
	transMode->autoAbortIntervalInSeconds() = -2;
      else if (transMode->accessMode() == TransMode::READ_ONLY_)
	transMode->autoAbortIntervalInSeconds() = 0;
  }

  // create the latename info List to be passed on to root_tdb.
  LateNameInfoList * lnil = NULL;
  Int32 numEntries = 0;
  if  (generator->getLateNameInfoList().entries() > 0)
    numEntries = generator->getLateNameInfoList().entries();
  lnil =
    (LateNameInfoList *)
      space->allocateMemory( sizeof(LateNameInfoList) +
                             (numEntries * sizeof(LateNameInfo)) );

  // Initialize LNIL from real LNIL object (this copies vtblptr into lnil).
  LateNameInfoList lnild;
  memcpy((char *)lnil,&lnild,sizeof(LateNameInfoList));

  lnil->setNumEntries(generator->getLateNameInfoList().entries());

  // This allocates an array of 64-bit pointers in lnil.
  lnil->allocateList(space,numEntries);

  // This sets up the array elements to point to the LateNameInfo objects.
  for (j = 0; j < numEntries; j++)
    lnil->setLateNameInfo(j,((LateNameInfo *)(lnil + 1)) + j);

  NABoolean viewPresent = FALSE;
  NABoolean variablePresent = FALSE;

  // olt opt is only done for tablenames which are literals
  NABoolean doTablenameOltOpt = TRUE;
  if  (generator->getLateNameInfoList().entries() > 0)
    {
      for (CollIndex i = 0;
	   i < generator->getLateNameInfoList().entries(); i++)
	{
	  LateNameInfo * tgt = &(lnil->getLateNameInfo(i));
	  LateNameInfo * src = (LateNameInfo *)generator->getLateNameInfoList()[i];
	  if (src->isVariable())
	    {
              doTablenameOltOpt = FALSE;
	    }

	  // *tgt = *src wouldn't work since it doesn't copy over the vtblptr.
	  memmove(tgt,src,sizeof(LateNameInfo));
	  // find the position of this hostvar in input var list.
	  if ((src->isVariable()) && (! src->isEnvVar()))
	    {
	      if (tgt->isCachedParam())
		{
		  tgt->setCachedParamOffset((Lng32)cachedAttrs[tgt->getInputListIndex()-1]->getOffset());
		}
	      else
		{
		  NABoolean found = FALSE;
		  for (CollIndex i = 0; ((i < newInputVars.entries()) && (! found)); i++)
		    {
		      ValueId val_id = newInputVars[i];
		      ItemExpr * item_expr = val_id.getItemExpr();
		      if (item_expr->getOperatorType() == ITM_HOSTVAR)
			{
			  HostVar * inputHV = (HostVar *)item_expr;
			  if ((inputHV->getName().length() == strlen(src->variableName())) &&
			      (strcmp(inputHV->getName().data(), src->variableName()) == 0))
			    {
			      found = TRUE;
			      tgt->setInputListIndex((short)(i+1));
			    }
			} // hostvar in input list
		    } // for

		  if (! found)
		    GenAssert(0, "Must find prototype hvar in input hvar");
		}
	    } // not an env var or a define.

      if (tgt->compileTimeAnsiName()[0] == '\\')
        {
	  if (NOT tgt->isVariable())
	    {
              tgt->setAnsiPhySame(TRUE);
	    }
	  else
	    {
	      if (tgt->isEnvVar())
		tgt->setAnsiPhySame(TRUE);
	      else
		{
		  // hostvar
		  // If prototype is a fully qualified name, then
		  // ansi-phy names are the same.
		  if (tgt->compileTimeAnsiName()[0] == '\\')
		    tgt->setAnsiPhySame(TRUE);
		};
	    }
        }
      else
        {
	  if (tgt->isVariable())
	    {
	      QualifiedName qn(tgt->compileTimeAnsiName(), 1,
			       generator->wHeap(),
			       generator->getBindWA());
	      qn.applyDefaults(generator->currentCmpContext()->schemaDB_->getDefaultSchema());
	      char * compileTimeAnsiName = space->AllocateAndCopyToAlignedSpace(
		   qn.getQualifiedNameAsAnsiString(), 0);
              tgt->setCompileTimeName(compileTimeAnsiName, space);
	    }
	} // else
      
      if (tgt->isView())
	viewPresent = TRUE;
      
      if (tgt->isVariable())
	variablePresent = TRUE;

      if (tgt->lastUsedAnsiName()[0] != '\0')
      {
	// VO, Metadata Indexes
	if (tgt->getNameSpace() == COM_INDEX_NAME)
	  // This lni is for an index - don't copy the compile time ansi name if the
	  // query is from a system module
	  if ( (generator->currentCmpContext()->internalCompile() != CmpContext::INTERNAL_MODULENAME) &&
	       !CmpCommon::statement()->isSMDRecompile() )
	    tgt->setLastUsedName(tgt->compileTimeAnsiName(),space);
      }

      // Special handling for case where we are recompiling a system module 
      // query. We need to resolve the name here since it will not go 
      // through the resolveNames in the CLI. Do this only for NSK
	} // for
    }

  lnil->setViewPresent(viewPresent);

  lnil->setVariablePresent(variablePresent);

  // Generate info to do similarity check.
  TrafQuerySimilarityInfo * qsi = genSimilarityInfo(generator);

  // generate the executor fragment directory <exFragDir> (list of all
  // fragments of the plan that are executed locally or are downloaded
  // to DP2 or to ESPs) from the generator's copy <compFragDir> and attach
  // it to the root_tdb
  NABoolean fragmentQuotas = CmpCommon::getDefault(ESP_MULTI_FRAGMENTS) == DF_ON;
  ExFragDir *exFragDir =
    new(space) ExFragDir(compFragDir->entries(),space,
                         CmpCommon::getDefault(ESP_MULTI_FRAGMENTS) == DF_ON, 
                         fragmentQuotas, 
                         (UInt16)CmpCommon::getDefaultLong(ESP_MULTI_FRAGMENT_QUOTA_VM),
                           (UInt8)CmpCommon::getDefaultLong(ESP_NUM_FRAGMENTS));

  // We compute the space needed in execution time for input Rowset variables
  for (i = 0; i < inputVars().entries(); i++)
  {
      ValueId val_id = inputVars()[i];
      ItemExpr * item_expr = val_id.getItemExpr();
      OperatorTypeEnum op = item_expr->getOperatorType();
      if (op == ITM_HOSTVAR) {
        HostVar *hostVar = (HostVar *) item_expr;
        if (hostVar->getType()->getTypeQualifier() == NA_ROWSET_TYPE) {
          Lng32 thisSize = hostVar->getType()->getTotalSize();
          input_vars_size += thisSize;
        }
      }
  }

  // find out if this is a delete where current of query.
  NABoolean delCurrOf = FALSE;
  short baseTablenamePosition = -1;
  if (fetchedCursorName || (fetchedCursorHvar >= 0)) // upd/del curr of
    {
      if (childOperType() == REL_UNARY_DELETE)
	delCurrOf = TRUE;
      else
	delCurrOf = FALSE;
    }

  // The ansi names of the table specified in the cursor stmt must match
  // the name specified in the upd/del where curr of stmt. This check
  // is done at runtime.
  // basetablenameposition is the index in the latenameinfolist of
  // the entry whose lastUsedAnsiName contains the name of the table.
  baseTablenamePosition = -1;
  if (updatableSelect())
    {
      // if this is an updatable select, find the ansi name of the basetable.
      // This name will be used at runtime to compare to the tablename
      // specified in an 'upd/del where current of' stmt. The two tablenames
      // must be the same.
      for (Int32 n = 0; n < noOfTables; n++)
	{
	  SqlTableOpenInfo * stoi = stoiList[n];

	  if (NOT stoi->isIndex())
	    {
	      if (baseTablenamePosition == -1)
		{
		  baseTablenamePosition = n;
		}
	    }
	}
      if (baseTablenamePosition == -1)
	{
	  // no base table access used. Only index access is used.
	  // The ansiname field in latenameInfo struct is the ansi name
	  // of the base table. Use that.
	  baseTablenamePosition = 0;
	}
    }
  else if (updateCurrentOf())
    {
      // if this is an update/delete current of query, find the index of
      // the base table. There might be other tables used in the plan
      // for index maintanence and they will either be indices or
      // specialTables with the special type being an INDEX_TABLE.
      // Look only for the true base tables.
      for (Int32 n = 0; n < noOfTables; n++)
	{
	  SqlTableOpenInfo * stoi = stoiList[n];

	  if ((NOT stoi->isIndex()) &&
	      (NOT stoi->specialTable()) &&
              (stoi->getUpdateAccess() ||
               stoi->getDeleteAccess() ))
	    {
	      if (baseTablenamePosition == -1)
		{
		  baseTablenamePosition = n;
		}
	    }
	}

      if (baseTablenamePosition == -1)
	{
	  // No base table found in the stoi list.
	  // Raise an error.
	  GenAssert(0, "Must find updelTableNamePosition!");
	}
    }

  // find out if this was an update,delete or insert query.
  NABoolean updDelInsert = FALSE;
  if (childOperType().match(REL_ANY_GEN_UPDATE))
    updDelInsert = TRUE;

  // Do OLT optimization if:
  //  -- OLT optimization is possible
  //  -- and no upd/del where current of
  //  -- and no late name resolution
  //  -- and no views in query
  NABoolean doOltQryOpt = FALSE;
  if ((oltOptInfo().oltCliOpt()) &&
      (viewStoiList == NULL) &&          // no views
      (doTablenameOltOpt == TRUE) &&                  // no late name info
      (fetchedCursorName == NULL) &&     // no upd/del curr of
      (fetchedCursorHvar < 0) &&
      (delCurrOf == FALSE) &&
      (getFirstNRows() == -1))          // no firstn specified
    {
      doOltQryOpt = TRUE;
    }

  // At runtime, we try to internally reexecute a statement in case of
  // lost opens, if that query has not affected the database(inserted/updated/
  // deleted a row), or a row has not been returned to the application.
  // Do not retry for lost opens of IUD queries if there are more
  // than one tables in the query. This is because we don't know if the db
  // was affected when the open was lost on the non-IUD table in the query.
  // For ex: in an insert...select query, an open could be blown away for
  // the select part of the query.
  // If there is only one table in this query, then that row will get locked
  // during IUD and the open could not be blown away.
  // If some day we put in a scheme to detect that the database
  // was not affected for a multi-table IUD, we can retry for lost
  // opens.
  NABoolean retryableStmt = TRUE;

  if (generator->aqrEnabled())
    {
      retryableStmt = FALSE;
    }

  if (updDelInsert && moreThanOneTable)
    retryableStmt = FALSE;

  if (childOperType() == REL_DDL)
    retryableStmt = FALSE;

  if (isExtractProducer || isExtractConsumer)
    retryableStmt = FALSE;

  // For now we mark statements containing UDRs as non-retryable.
  // This is to avoid executing a stored procedure body multiple times
  // inside a single application request. Currently the only UDR-
  // containing statment is CALL.
  //
  // There are scenarios however in which it would be correct (and
  // helpful) to retry a UDR-containing statement. Perhaps in the
  // future the restriction can be lifted in some cases.  Safe retry
  // scenarios include a subquery input parameter returning a blown
  // away open error before the stored procedure body has executed,
  // and UDR bodies that only do computation and not transactional work.
  if (noOfUdrs > 0)
    retryableStmt = FALSE;

  short maxResultSets = generator->getBindWA()->getMaxResultSets();

  char *queryCostInfoBuf = NULL;
  QueryCostInfo *queryCostInfo =
    (QueryCostInfo *) new (space) char[sizeof(QueryCostInfo)];
  // fill in cost. Taken from explain code in GenExplain.cpp
  if (getRollUpCost())
    {
      double cpu, io, msg, idle, seqIOs, randIOs, total, cardinality;
      double totalMemPerCpu, totalMemPerCpuInKB;
      short maxCpuUsage;
      Lng32 dummy;
      Cost const *operatorCost = getRollUpCost();
      const NABoolean inMaster = generator->getEspLevel() == 0;
      operatorCost->getExternalCostAttr(cpu, io, msg, idle, seqIOs, randIOs, total, dummy);
//      operatorCost->getOcmCostAttr(cpu, io, msg, idle, dummy);
//      total = MINOF(operatorCost->convertToElapsedTime(), 1e32).getValue();

      cardinality = MINOF(getEstRowsUsed(), 1e32).value();

      // get the totalMem that is being used divide by max dop to get an 
      // an estimate of memory usage per cpu. Convert to KB units.
      totalMemPerCpu = 
	generator->getTotalEstimatedMemory() /
	((generator->compilerStatsInfo().dop() > 0) ? 
	 generator->compilerStatsInfo().dop() : 1);
      totalMemPerCpuInKB = totalMemPerCpu / 1024 ;
      maxCpuUsage = generator->getMaxCpuUsage() ;
      queryCostInfo->setCostInfo(cpu, io, msg, idle, seqIOs, randIOs, total,
                                 cardinality, totalMemPerCpuInKB, maxCpuUsage);

      // if resourceUsage need to be set (low/medium/high), set it here.
      // For now, set to 0 which indicates that this value is not 
      // being returned.
      queryCostInfo->setResourceUsage(0);
    }
  
  queryCostInfoBuf = (char*)queryCostInfo;
  //
  // CompilationStatsData  
  CompilationStats* stats = CURRENTSTMT->getCompilationStats();
  char *compilerId = new (space) char[COMPILER_ID_LEN];
  str_cpy_all(compilerId, generator->currentCmpContext()->getCompilerId(),
              COMPILER_ID_LEN);
  Int32 cLen = stats->getCompileInfoLen();
  //
  // make it 1 at minimum
  cLen = ( cLen < 1 ) ? 1 : cLen;

  char *compileInfo = new (space) char[cLen];
  stats->getCompileInfo(compileInfo);      
  //
  // Some of the fields are set here but modified later after generator phase is
  // complete (such as compileEndTime, CMP_PHASE_ALL, and CMP_PHASE_GENERATOR)
  CompilationStatsData *compilationStatsData =
    (CompilationStatsData *) new (space) 
        CompilationStatsData(stats->compileStartTime(),
            stats->compileEndTime(),  
            compilerId, 
            stats->cmpPhaseLength(CompilationStats::CMP_PHASE_ALL),
            stats->cmpPhaseLength(CompilationStats::CMP_PHASE_BINDER),
            stats->cmpPhaseLength(CompilationStats::CMP_PHASE_NORMALIZER),
            stats->cmpPhaseLength(CompilationStats::CMP_PHASE_ANALYZER),
            stats->cmpPhaseLength(CompilationStats::CMP_PHASE_OPTIMIZER),
            stats->cmpPhaseLength(CompilationStats::CMP_PHASE_GENERATOR), 
            stats->metadataCacheHits(),
            stats->metadataCacheLookups(),
            stats->getQueryCacheState(),
            stats->histogramCacheHits(),
            stats->histogramCacheLookups(),
            stats->stmtHeapCurrentSize(),
            stats->cxtHeapCurrentSize(),
            stats->optimizationTasks(),
            stats->optimizationContexts(),
            stats->isRecompile(),
            compileInfo,
            stats->getCompileInfoLen());  
  //
  // CompilerStats 
  char *compilerStatsInfoBuf = NULL;
  CompilerStatsInfo *compilerStatsInfo =
    (CompilerStatsInfo *) new (space) char[sizeof(CompilerStatsInfo)];
  compilerStatsInfoBuf = (char*)compilerStatsInfo;
  *compilerStatsInfo = generator->compilerStatsInfo();  

  // remove the duplicated entries from the schema label list, and put the
  // unique entries in TDB. During execution time, we will check the 
  // LastModTimestamp of the schema label. If the lastModTimestamp has changed,
  // a timestamp mismatch will be returned, allowing for recompilation of the
  // query


  NABoolean validateSSTSFlag = TRUE;

  CollIndex numObjectUIDs = generator->objectUids().entries();
  Int64 *objectUIDsPtr = NULL;
  if (numObjectUIDs > 0)
    {
      objectUIDsPtr  = new (space) Int64[numObjectUIDs];
      for (CollIndex i = 0; i < numObjectUIDs; i++)
        objectUIDsPtr[i] = generator->objectUids()[i];
    }
  
  Queue * listOfSnapshotscanTables =  NULL;
  NAString tmpLocNAS;
  char * tmpLoc = NULL;
  Int64 numObjectNames = generator->objectNames().entries();
  if (numObjectNames >0)
  {
    listOfSnapshotscanTables = new(space) Queue(space);
    for (Lng32 i=0 ; i <generator->objectNames().entries(); i++)
    {
     char * nm = space->allocateAlignedSpace(generator->objectNames()[i].length() + 1);
     strcpy(nm, generator->objectNames()[i].data());
     listOfSnapshotscanTables->insert(nm);
    }

    tmpLocNAS = generator->getSnapshotScanTmpLocation();
    CMPASSERT(tmpLocNAS[tmpLocNAS.length()-1] =='/');
    tmpLoc = space->allocateAlignedSpace(tmpLocNAS.length() + 1);
    strcpy(tmpLoc, tmpLocNAS.data());
  }


  // for describe type commands(showshape, showplan, explain) we don't
  // need to pass in the actual param values even if the query contains
  // params. Reset input_expr. This is done to avoid returning
  // an error later if the actual param value is not set.
 OperatorTypeEnum child_op_type = childOperType();
  if (child_op_type == REL_EXE_UTIL || child_op_type == REL_DESCRIBE)
  {
    RelExpr * lc = child(0)->castToRelExpr();
    OperatorTypeEnum actual_op_type = lc->getOperatorType();

    if (actual_op_type == REL_EXE_UTIL)
    {
      ExeUtilExpr *e = (ExeUtilExpr *)lc;
      if (e->getExeUtilType() == ExeUtilExpr::DISPLAY_EXPLAIN_)
        input_expr = NULL;
    }
    else if (actual_op_type == REL_DESCRIBE)
    {
       Describe *d = (Describe *)lc;
       if (d->getFormat() == Describe::SHAPE_ 
         ||  d->getFormat() == Describe::PLAN_)
           input_expr = NULL;
    }
  }

  // ---------------------------------------------------------------------
  // now initialize the previously allocated root tdb. note that this
  // init *must* come before we fill in the exFragDir's info because there
  // we compute how much space is used by the fragment the root is in.
  // This init() call passes in the space object. The root might allocate
  // more space for its uses inside init().
  // ---------------------------------------------------------------------
  root_tdb->init(child_tdb,
                 cri_desc, // input to child
                 (InputOutputExpr *)input_expr,
                 (InputOutputExpr *)output_expr,
                 input_vars_size,
                 pkey_expr,
                 pkey_len,
                 pred_expr,
                 work_cri_desc,
                 exFragDir,
                 transMode,
                 fetchedCursorName,
                 fetchedCursorHvar,
		 delCurrOf,
                 numUpdateCol,
                 updateColList,
                 (outputVarCntValid() && outputVarCnt()),
                 noOfTables,
                 getFirstNRows(),
                 userInputVars,
                 (getRollUpCost() ?
                  getRollUpCost()->displayTotalCost().getValue() : 0),
                 stoiList,
                 lnil,
                 viewStoiList,
                 qsi,
                 space,
		 uniqueExecuteIdOffset, //++ Triggers -
		 triggersStatusOffset,
		 triggersCount,
		 triggersList,
                 (short)generator->getTempTableId(),
		 (short)baseTablenamePosition,
		 updDelInsert,
		 retryableStmt,
		 getGroupAttr()->isStream(),
		 // next flag is set for destructive stream access protocol.
		 // Not needed for hbase/seabase access.
		 (getGroupAttr()->isEmbeddedUpdateOrDelete() &&
		  (NOT hdfsAccess())),
		 CmpCommon::getDefaultNumeric(STREAM_TIMEOUT),
		 generator->getPlanId(),
		 qCacheInfoBuf,
		 cacheVarsSize,
		 udrStoiList,
		 noOfUdrs,
                 maxResultSets,
		 queryCostInfoBuf,
		 newMvList,
		 uninitializedMvCount,
		 compilerStatsInfoBuf,
		 rwrsInfoBuf,
                 numObjectUIDs ,
                 objectUIDsPtr,
                 compilationStatsData,
                 tmpLoc,
                 listOfSnapshotscanTables);

  root_tdb->setTdbId(generator->getAndIncTdbId());
  
  if (childTdbIsNull)
     root_tdb->setChildTdbIsNull(); 
  if (generator->explainInRms())
     root_tdb->setExplainInRms();

  OperatorTypeEnum childOper = childOperType();

  if (qCacheInfoIsClass)
    root_tdb->setQCacheInfoIsClass(TRUE);

  if (getHostArraysArea() && getHostArraysArea()->getRowwiseRowset())
    {
      root_tdb->setRowwiseRowsetInput(TRUE);
    }
  else
    {
      NABoolean singleRowInput = TRUE;
      if ((input_expr) && ((InputOutputExpr *)input_expr)->isCall())
	singleRowInput = FALSE;

      if (CmpCommon::getDefault(COMP_BOOL_92) == DF_OFF)
	singleRowInput = FALSE;

      root_tdb->setSingleRowInput(singleRowInput);
    }

  if (childOper == REL_DDL)
    {
      root_tdb->setDDLQuery(TRUE);
    }

    root_tdb->setCIFON(isCIFOn_);
    if (generator->currentCmpContext()->isEmbeddedArkcmp())
      //if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
      root_tdb->setEmbeddedCompiler(TRUE);
   else
     root_tdb->setEmbeddedCompiler(FALSE);
  // We check to see if this tree corresponds to a compound statement so
  // we know this at execution time

  RelExpr* checkNode = child(0);

  if ( checkNode->getOperatorType() == REL_EXCHANGE )
    checkNode = checkNode->child(0);

  if ( checkNode->getOperatorType() == REL_PARTITION_ACCESS )
    checkNode = checkNode->child(0);

  if (checkNode->getOperatorType() == REL_COMPOUND_STMT ||
      (checkNode->getOperatorType() == REL_UNION ||
       checkNode->getOperatorType() == REL_MERGE_UNION)
      &&
      ((Union *) (RelExpr *) checkNode)->getUnionForIF()) {
    root_tdb->setCompoundStatement();
  }

  root_tdb->setDoOltQueryOpt(doOltQryOpt);

  root_tdb->setQueryType(ComTdbRoot::SQL_OTHER);

  // set the EMS Event Experience Level information
  // the default is ADVANCED if it is not specified
  if (CmpCommon::getDefault(USER_EXPERIENCE_LEVEL) == DF_BEGINNER)
  {
      root_tdb->setEMSEventExperienceLevelBeginner(TRUE);
  } 
  if (CmpCommon::getDefault(UNC_PROCESS) == DF_ON)
  {
      root_tdb->setUncProcess(TRUE);
  }

  
  // If this is a ustat query set the query type so WMS can monitor it
  if (childOper == REL_DDL)
    {
      DDLExpr *ddlExpr = (DDLExpr *)child(0)->castToRelExpr();
      char * stmt = ddlExpr->getDDLStmtText();
      NAString ddlStr = NAString(stmt);
      ddlStr = ddlStr.strip(NAString::leading, ' ');
      // If this is a ustat statement, set the type 
      Int32 foundUpdStat = 0;
      
      // check if the first token is UPDATE
      size_t position = ddlStr.index("UPDATE", 0, NAString::ignoreCase);
      if (position == 0)
        {
          // found UPDATE. See if the next token is STATISTICS.
          ddlStr = ddlStr(6, ddlStr.length()-6); // skip over UPDATE
          ddlStr = ddlStr.strip(NAString::leading, ' ');
          
          position = ddlStr.index("STATISTICS", 0, NAString::ignoreCase);
          if (position == 0)
            foundUpdStat = -1;
        }
      if (foundUpdStat)
        { 
          root_tdb->setQueryType(ComTdbRoot::SQL_CAT_UTIL);     
        }
    }
  // Disable Cancel for some queries.  But start the logic with 
  // "all queries can be canceled."
  root_tdb->setMayNotCancel(FALSE);

  // Disallow cancel.
  if (CmpCommon::getDefault(COMP_BOOL_20) == DF_ON)
    root_tdb->setMayNotCancel(TRUE);

  if (generator->mayNotCancel())
    root_tdb->setMayNotCancel(TRUE);

  if (updDelInsert)
    {
      if ((childOper == REL_UNARY_INSERT) ||
	  (childOper == REL_LEAF_INSERT) ||
	  (childOper == REL_INSERT_CURSOR))
	root_tdb->setQueryType(ComTdbRoot::SQL_INSERT_NON_UNIQUE);
      else if ((childOper == REL_UNARY_UPDATE) ||
	       (childOper == REL_LEAF_UPDATE) ||
	       (childOper == REL_UPDATE_CURSOR))
	root_tdb->setQueryType(ComTdbRoot::SQL_UPDATE_NON_UNIQUE);
      else if ((childOper == REL_UNARY_DELETE) ||
	       (childOper == REL_LEAF_DELETE) ||
	       (childOper == REL_DELETE_CURSOR))
	root_tdb->setQueryType(ComTdbRoot::SQL_DELETE_NON_UNIQUE);
    }
  
  if (output_expr)
    root_tdb->setQueryType(ComTdbRoot::SQL_SELECT_NON_UNIQUE);
  
  if ((updDelInsert) &&
      (root_tdb->getQueryType() == ComTdbRoot::SQL_INSERT_NON_UNIQUE) &&
      (rwrsInfo))
    {
      root_tdb->setQueryType(ComTdbRoot::SQL_INSERT_RWRS);
    }
  else if ((child(0)) &&
	   (child(0)->castToRelExpr()->getOperatorType() == REL_UTIL_INTERNALSP))
    {
       root_tdb->setQueryType(ComTdbRoot::SQL_CAT_UTIL);
    }
  else if ((child(0)) &&
	   (child(0)->castToRelExpr()->getOperatorType() == REL_DESCRIBE))
    {
      root_tdb->setSubqueryType(ComTdbRoot::SQL_DESCRIBE_QUERY);
    }
  else if ((child(0)) &&
	   (child(0)->castToRelExpr()->getOperatorType() == REL_EXE_UTIL))
    {
      root_tdb->setQueryType(ComTdbRoot::SQL_EXE_UTIL);
      ExeUtilExpr * exeUtil = (ExeUtilExpr*)child(0)->castToRelExpr();
      if (exeUtil->getExeUtilType() == ExeUtilExpr::CREATE_TABLE_AS_)
      {
        if (CmpCommon::getDefault(REDRIVE_CTAS) == DF_OFF)
	  root_tdb->setQueryType(ComTdbRoot::SQL_INSERT_NON_UNIQUE);
        else
          root_tdb->setSubqueryType(ComTdbRoot::SQL_STMT_CTAS);
      }
      else if (exeUtil->getExeUtilType() == ExeUtilExpr::GET_STATISTICS_)
          root_tdb->setSubqueryType(ComTdbRoot::SQL_STMT_GET_STATISTICS);
      else if (exeUtil->getExeUtilType() == ExeUtilExpr::DISPLAY_EXPLAIN_)
	{
          root_tdb->setSubqueryType(ComTdbRoot::SQL_DISPLAY_EXPLAIN);

	   if (CmpCommon::getDefault(EXE_UTIL_RWRS) == DF_ON)
	     root_tdb->setExeUtilRwrs(TRUE);
	}
       else if (exeUtil->getExeUtilType() == ExeUtilExpr::HBASE_COPROC_AGGR_)
          root_tdb->setQueryType(ComTdbRoot::SQL_SELECT_NON_UNIQUE);
       else if (exeUtil->getExeUtilType() == ExeUtilExpr::HBASE_LOAD_)
       {
         root_tdb->setSubqueryType(ComTdbRoot::SQL_STMT_HBASE_LOAD);
       }
       else if (exeUtil->getExeUtilType() == ExeUtilExpr::HBASE_UNLOAD_)
       {
         root_tdb->setSubqueryType(ComTdbRoot::SQL_STMT_HBASE_UNLOAD);
       }
       else if (exeUtil->getExeUtilType() == ExeUtilExpr::LOB_EXTRACT_)
	 {
	   root_tdb->setSubqueryType(ComTdbRoot::SQL_STMT_LOB_EXTRACT);
	 }
       else if(exeUtil->getExeUtilType() == ExeUtilExpr::LOB_UPDATE_UTIL_)
         {
           root_tdb->setSubqueryType(ComTdbRoot::SQL_STMT_LOB_UPDATE_UTIL
); 
         }

      else if (exeUtil->isExeUtilQueryType())
	{
	   root_tdb->setQueryType(ComTdbRoot::SQL_EXE_UTIL);
	}
    }
  else if ((child(0)) &&
	   (child(0)->castToRelExpr()->getOperatorType() == REL_DDL))
    {
      DDLExpr *ddlExpr = (DDLExpr *)child(0)->castToRelExpr();
      
      if (ddlExpr->producesOutput())
        root_tdb->setQueryType(ComTdbRoot::SQL_EXE_UTIL);     
    }
  else if (generator->getBindWA()->hasCallStmts())
  {
    // In this version of the compiler we assume any statement that
    // contains UDRs is either a CALL statement.
    if (maxResultSets > 0)
      root_tdb->setQueryType(ComTdbRoot::SQL_CALL_WITH_RESULT_SETS);
    else
      root_tdb->setQueryType(ComTdbRoot::SQL_CALL_NO_RESULT_SETS);
  }
  else
  {
    OperatorTypeEnum currChildOper = 
      child(0)->castToRelExpr()->getOperatorType();
    
    if (currChildOper == REL_CONTROL_QUERY_DEFAULT)
    {
      root_tdb->setMayNotCancel(TRUE);
      ControlQueryDefault * cqd = 
        (ControlQueryDefault*)child(0)->castToRelExpr();
      if (cqd->dynamic())
      {
        if (cqd->getAttrEnum() == CATALOG)
          root_tdb->setQueryType(ComTdbRoot::SQL_SET_CATALOG); 
        else if (cqd->getAttrEnum() == SCHEMA) 
          root_tdb->setQueryType(ComTdbRoot::SQL_SET_SCHEMA); 
        else
          root_tdb->setQueryType(ComTdbRoot::SQL_CONTROL); 
      }
      else
        root_tdb->setQueryType(ComTdbRoot::SQL_CONTROL); 
    }
    else if ((currChildOper == REL_CONTROL_QUERY_SHAPE) ||
             (currChildOper == REL_CONTROL_TABLE))
    {
      root_tdb->setMayNotCancel(TRUE);
      root_tdb->setQueryType(ComTdbRoot::SQL_CONTROL);
    }
    else if (currChildOper == REL_TRANSACTION)
    {
      root_tdb->setMayNotCancel(TRUE);
      if (((RelTransaction*)child(0)->castToRelExpr())->getType() == SET_TRANSACTION_)
        root_tdb->setQueryType(ComTdbRoot::SQL_SET_TRANSACTION); 
    }
    else if (currChildOper == REL_SP_PROXY)
    {
      // This is a stored procedure result set
      root_tdb->setQueryType(ComTdbRoot::SQL_SP_RESULT_SET);
    }
    else if (currChildOper == REL_EXE_UTIL)
    {
      ExeUtilExpr * exeUtil = (ExeUtilExpr*)child(0)->castToRelExpr();
      if (exeUtil->getExeUtilType() == ExeUtilExpr::CREATE_TABLE_AS_)
	root_tdb->setQueryType(ComTdbRoot::SQL_INSERT_NON_UNIQUE);
    }
    else if (REL_EXPLAIN == currChildOper)
      root_tdb->setMayNotCancel(TRUE);
    else if (REL_SET_TIMEOUT == currChildOper)
      root_tdb->setMayNotCancel(TRUE);
    else if (REL_CONTROL_RUNNING_QUERY == currChildOper)
      root_tdb->setMayNotCancel(TRUE);
  }

  if (generator->isFastExtract())
  {
    root_tdb->setQueryType(ComTdbRoot::SQL_SELECT_UNLOAD);
  }

  if (child(0) && child(0)->castToRelExpr() && 
      child(0)->castToRelExpr()->getOperator().match(REL_ANY_HBASE))
    {
      RelExpr * childExpr = child(0)->castToRelExpr();
      OperatorTypeEnum currChildOper = childExpr->getOperatorType();
      
      if ((childExpr->getOperator().match(REL_ANY_HBASE_GEN_UPDATE)) &&
	  (NOT output_expr))
	{
	  GenericUpdate * gu = (GenericUpdate *)childExpr;
	  
	  if (gu->uniqueHbaseOper())
	    {
	      if (currChildOper == REL_HBASE_UPDATE)
		root_tdb->setQueryType(ComTdbRoot::SQL_UPDATE_UNIQUE);
	      else if (currChildOper == REL_HBASE_DELETE)
		root_tdb->setQueryType(ComTdbRoot::SQL_DELETE_UNIQUE);
	      else
		root_tdb->setQueryType(ComTdbRoot::SQL_INSERT_UNIQUE);
	    }
	  else
	    {
	      if (currChildOper == REL_HBASE_UPDATE)
		root_tdb->setQueryType(ComTdbRoot::SQL_UPDATE_NON_UNIQUE);
	      else if (currChildOper == REL_HBASE_DELETE)
		root_tdb->setQueryType(ComTdbRoot::SQL_DELETE_NON_UNIQUE);
	      else
		root_tdb->setQueryType(ComTdbRoot::SQL_INSERT_NON_UNIQUE);
	    }
	}
      else if (currChildOper == REL_HBASE_ACCESS)
	{
	  HbaseAccess * ha = (HbaseAccess *)childExpr;
	  if (ha->uniqueHbaseOper())
	    root_tdb->setQueryType(ComTdbRoot::SQL_SELECT_UNIQUE);
	  else
	    root_tdb->setQueryType(ComTdbRoot::SQL_SELECT_NON_UNIQUE);
	}
    }

  // To help determine if it is safe to suspend.
  if (child(0) && 
      child(0)->castToRelExpr())
  {
    OperatorTypeEnum currChildOper = 
        child(0)->castToRelExpr()->getOperatorType();
    if ((REL_DDL == currChildOper) ||
        (REL_TRANSACTION == currChildOper) ||
        (REL_EXE_UTIL == currChildOper))
      root_tdb->setMayAlterDb(TRUE);

    if (REL_LOCK == currChildOper)
      root_tdb->setSuspendMayHoldLock(TRUE);
  }

  if (generator->anySerialiableScan())
      root_tdb->setSuspendMayHoldLock(TRUE);
  
  root_tdb->setOdbcQuery(CmpCommon::getDefault(ODBC_PROCESS) == DF_ON);

  if (generator->getTolerateNonFatalError()) {
    root_tdb->setTolerateNonFatalError(TRUE);
    if (CmpCommon::getDefault(NOT_ATOMIC_FAILURE_LIMIT,0) == DF_SYSTEM)
      root_tdb->setNotAtomicFailureLimit(ComCondition::NO_LIMIT_ON_ERROR_CONDITIONS);
    else
      root_tdb->setNotAtomicFailureLimit(CmpCommon::getDefaultLong(NOT_ATOMIC_FAILURE_LIMIT));
  }

  if (generator->embeddedIUDWithLast1()) {
    root_tdb->setEmbeddedIUDWithLast1(TRUE);
  }

  if (generator->embeddedInsert()) {
    root_tdb->setEmbeddedInsert(TRUE);
  }

  if (containsLRU())
    {
      root_tdb->setLRUOperation(TRUE);
    }

  if (generator->aqrEnabled())
    root_tdb->setAqrEnabled(TRUE);

  if (generator->cantReclaimQuery())
    root_tdb->setCantReclaimQuery(TRUE);

    // if a transaction is needed at runtime to execute this query,
  // set that information in the root tdb. Generator synthesized
  // this information based on the kind of query or if START_XN
  // define was set.
  // Certain queries (insert, update, delete) ALWAYS require a transaction
  // at runtime.
  // After parser support for REPEATABLE ACCESS, etc, is in, this
  // information will come from the parse tree for scans.
  if (generator->isTransactionNeeded())
    {
      root_tdb->setTransactionReqd();

      if (generator->foundAnUpdate())
	{
	  if (generator->updAbortOnError() == TRUE)
	    {
	      // if transaction has to be aborted at runtime after an error,
	      // set that info in root_tdb.
	      root_tdb->setUpdAbortOnError(-1);
	    }
	  else if (generator->updPartialOnError() == TRUE)
	    {
	      root_tdb->setUpdPartialOnError(-1);
	    }
	  else if (generator->updErrorInternalOnError() == TRUE)
	    {
	      root_tdb->setUpdErrorOnError(-1);
	    }
	  else if (generator->updErrorOnError() == FALSE)
	    {
	      if (generator->updSavepointOnError() == TRUE)
		{
		  root_tdb->setUpdSavepointOnError(-1);
		}
	      else
		root_tdb->setUpdAbortOnError(-1);
	    }
	  else
	    root_tdb->setUpdErrorOnError(-1);
	}
      else
	{
	  root_tdb->setUpdErrorOnError(-1);
	}
    } // transactionNeeded

  if ((oltOptLean()) &&
      (doOltQryOpt))
    {
      if ((NOT root_tdb->getUpdAbortOnError()) &&
	  (NOT root_tdb->getUpdSavepointOnError()) &&
	  (NOT root_tdb->getUpdPartialOnError()) &&
	  (retryableStmt) &&
	  (NOT root_tdb->thereIsACompoundStatement()))
	{
	  root_tdb->setDoOltQueryOptLean(TRUE);

	  child_tdb->setDoOltQueryOptLean(TRUE);
	}
    }

  if (generator->dp2XnsEnabled())
    {
      root_tdb->setDp2XnsEnabled(generator->dp2XnsEnabled());
    }

  
  if (generator->processLOB()) {
     root_tdb->setProcessLOB(TRUE);
     root_tdb->setUseLibHdfs(CmpCommon::getDefault(USE_LIBHDFS) == DF_ON);
  } 

  // Self-referencing updates
  if (avoidHalloween_)
  {
    if (Generator::DP2LOCKS == generator->getHalloweenProtection())
    {
      // Plan was generated without resetting the generator's 
      // HalloweenProtectionType from DP2Locks, therefore we are
      // using DP2 locks, and cannot allow auto commit off.
      root_tdb->setCheckAutoCommit(TRUE);
    }
  }
  else if (CmpCommon::getDefault(AQR_WNR_DELETE_NO_ROWCOUNT) == DF_ON)
  {
    // Allow non-ACID AQR of NO ROLLBACK DELETE that may have changed 
    // target. Query type (DELETE vs others) and WNR will be evaluated
    // at runtime.
    root_tdb->setAqrWnrDeleteContinue(TRUE);
  }
  if (CmpCommon::getDefault(PSHOLD_CLOSE_ON_ROLLBACK) == DF_ON)
    root_tdb->setPsholdCloseOnRollback(TRUE);
  else
    root_tdb->setPsholdCloseOnRollback(FALSE);
  if (CmpCommon::getDefault(PSHOLD_UPDATE_BEFORE_FETCH) == DF_ON)
    root_tdb->setPsholdUpdateBeforeFetch(TRUE);
  else
    root_tdb->setPsholdUpdateBeforeFetch(FALSE);

  root_tdb->setAbendType(
    (Lng32) CmpCommon::getDefaultNumeric(COMP_INT_38) );

  double cpuLimitCheckFreq = CmpCommon::getDefaultNumeric(COMP_INT_48);
  if (cpuLimitCheckFreq > SHRT_MAX)
    cpuLimitCheckFreq = SHRT_MAX;
  root_tdb->setCpuLimitCheckFreq((short) cpuLimitCheckFreq);

  // Config query execution limits.
  Lng32 cpuLimit = (Lng32) CmpCommon::getDefaultNumeric(QUERY_LIMIT_SQL_PROCESS_CPU);
  if (cpuLimit > 0)
    root_tdb->setCpuLimit(cpuLimit);  

  if (CmpCommon::getDefault(QUERY_LIMIT_SQL_PROCESS_CPU_DEBUG) == DF_ON)
    root_tdb->setQueryLimitDebug();

  if (generator->inMemoryObjectDefn())
    root_tdb->setInMemoryObjectDefn(TRUE);
	
  if (CmpCommon::getDefault(READONLY_CURSOR) == DF_ON)
    root_tdb->setCursorType(SQL_READONLY_CURSOR);
  else
    root_tdb->setCursorType(SQL_UPDATABLE_CURSOR);
	
  if (CmpCommon::getDefault(WMS_QUERY_MONITORING) == DF_ON)
    root_tdb->setWmsMonitorQuery(TRUE);
  else
    root_tdb->setWmsMonitorQuery(FALSE);

  if (CmpCommon::getDefault(WMS_CHILD_QUERY_MONITORING) == DF_ON)
    root_tdb->setWmsChildMonitorQuery(TRUE);
  else
    root_tdb->setWmsChildMonitorQuery(FALSE);

  if (hdfsAccess())
    root_tdb->setHdfsAccess(TRUE);

  if(generator->hiveAccess())
    root_tdb->setHiveAccess(TRUE);

  root_tdb->setBmoMemoryLimitPerNode(ActiveSchemaDB()->getDefaults().getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB));
  root_tdb->setEstBmoMemoryPerNode(generator->getTotalBMOsMemoryPerNode().value());

  Int32 numSikEntries = securityKeySet_.entries();
  if (numSikEntries > 0)
  {
    ComSecurityKey * sikValues = new (space) ComSecurityKey[numSikEntries];

    for (Int32 sv = 0; sv < numSikEntries; sv++)
      sikValues[sv] = securityKeySet_[sv];

    SecurityInvKeyInfo * sikInfo = new (space) SecurityInvKeyInfo(
                                     numSikEntries, sikValues); 
    root_tdb->setSikInfo(sikInfo);
  }

  if (!generator->explainDisabled())
    {
      // finish up EXPLAIN
      ExplainTuple *rootExplainTuple =
	addExplainInfo(root_tdb, 0, 0, generator);
      explainDesc->setExplainTreeRoot(rootExplainTuple);

      ExplainTuple *childExplainTuple = generator->getExplainTuple();

      rootExplainTuple->child(0) = childExplainTuple;
      if(childExplainTuple)
	{
	  childExplainTuple->setParent(rootExplainTuple);
	  rootExplainTuple->setChildSeqNum(
	       0,
	       childExplainTuple->getSeqNum());
	}

      generator->setExplainTuple(rootExplainTuple);
    }

  // Generate a list of scratch file options
  exFragDir->setScratchFileOptions(genScratchFileOptions(generator));

  // move ESP nodemask into frag dir
  exFragDir->setNodeMask((ULng32) getDefault(PARALLEL_ESP_NODEMASK));

  // generate the partition input data descriptor from the compile-time
  // partitioning attributes
  ExPartInputDataDesc **partInputDataDescs =
    new(generator->wHeap()) ExPartInputDataDesc *[compFragDir->entries()];
  ExEspNodeMap **nodeMap =
    new(generator->wHeap()) ExEspNodeMap *[compFragDir->entries()];
  for (i = 0; i < compFragDir->entries(); i++)
    {
      if (compFragDir->getPartitioningFunction(i) != NULL)
	{
	  // This fragment has partitioning info, generate it
	  ((PartitioningFunction *) compFragDir->getPartitioningFunction(i))->
	    codeGen(generator,
		    compFragDir->getPartInputDataLength(i));
	  partInputDataDescs[i] =
	    (ExPartInputDataDesc *) (generator->getGenObj());
	  NodeMap::codeGen(compFragDir->getPartitioningFunction(i),
			   compFragDir->getNumESPs(i),
			   generator);
	  nodeMap[i] = (ExEspNodeMap *) (generator->getGenObj());
	}
      else
        {
	  partInputDataDescs[i] = NULL;
          nodeMap[i] = NULL;
        }
    }

  // remove maptable for child tree
  generator->removeAll(map_table);

  // remove my map table
  generator->removeLast();
  generator->setMapTable(NULL);

  // move data entry by entry from the generator's copy into the executor copy
  Lng32 offset = 0;
  Lng32 currLength;
  Lng32 compressThreshold = getDefault(FRAG_COMPRESSION_THRESHOLD);
  NABoolean anyEspFragments = FALSE;

  for (i = 0; i < compFragDir->entries(); i++)
    {
      // translate fragment type enums
      ExFragDir::ExFragEntryType runTimeType = ExFragDir::MASTER;
      currLength = compFragDir->getFragmentLength(i);
      compilerStatsInfo->totalFragmentSize() += currLength;
      switch (compFragDir->getType(i))
	{
	case FragmentDir::MASTER:
	  runTimeType = ExFragDir::MASTER;
	  compilerStatsInfo->masterFragmentSize() += currLength;
	  break;
	case FragmentDir::DP2:
	  runTimeType = ExFragDir::DP2;
	  compilerStatsInfo->dp2FragmentSize() += currLength;
	  break;
	case FragmentDir::ESP:
	  runTimeType = ExFragDir::ESP;
          anyEspFragments = TRUE;
	  compilerStatsInfo->espFragmentSize() += currLength;
	  break;
	case FragmentDir::EXPLAIN:
	  runTimeType = ExFragDir::EXPLAIN;
	  compilerStatsInfo->masterFragmentSize() += currLength;
	  break;
	default:
	  ABORT("Internal error, invalid fragment type");
	}

      // take the pointer of the top-level object in this fragment and
      // convert it to a fragment-relative offset
      Lng32 offsetOfTopNode = compFragDir->getSpace(i)->
	convertToOffset((char *)(compFragDir->getTopNode(i)));

      // now set the values of the previously allocated directory entry

      NABoolean mlimitPerNode = CmpCommon::getDefaultLong(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) > 0;
      UInt16 BMOsMemoryUsage = 0;
      if (mlimitPerNode == TRUE)
        BMOsMemoryUsage = (UInt16)compFragDir->getBMOsMemoryUsage(i);
      else if (compFragDir->getNumBMOs(i) > 1 ||
               (compFragDir->getNumBMOs(i) == 1 && CmpCommon::getDefault(EXE_SINGLE_BMO_QUOTA) == DF_ON))
        BMOsMemoryUsage = (UInt16)CmpCommon::getDefaultLong(EXE_MEMORY_AVAILABLE_IN_MB);

      exFragDir->set(i,
		     runTimeType,
		     (ExFragId) compFragDir->getParentId(i),
		     offset,
		     currLength,
		     -offsetOfTopNode,
		     partInputDataDescs[i],
		     nodeMap[i],
		     compFragDir->getNumESPs(i),
		     compFragDir->getEspLevel(i),
		     compFragDir->getNeedsTransaction(i),
                     (compressThreshold > 0 &&
                      runTimeType == ExFragDir::ESP &&
                      compressThreshold <= compFragDir->getNumESPs(i))?
                       TRUE: FALSE,  // DP2 fragment will be compressed later
                                     // if parent ESP fragment is compressed
                                     // see executor/ex_frag_rt.cpp
		     compFragDir->getSoloFragment(i),
		     BMOsMemoryUsage,
		     compFragDir->getNumBMOs(i) > 0
                     );
      offset += currLength;
    } // for each fragment

  compilerStatsInfo->totalFragmentSize() /= 1024;
  compilerStatsInfo->masterFragmentSize() /= 1024;
  compilerStatsInfo->espFragmentSize() /= 1024;
  compilerStatsInfo->dp2FragmentSize() /= 1024;
  compilerStatsInfo->collectStatsType() = generator->collectStatsType();
  compilerStatsInfo->udr() = noOfUdrs;
  compilerStatsInfo->ofMode() = generator->getOverflowMode();
  compilerStatsInfo->ofSize() = 0;
  compilerStatsInfo->bmo() = generator->getTotalNumBMOs();
  compilerStatsInfo->queryType() = (Int16)root_tdb->getQueryType();
  compilerStatsInfo->subqueryType() = (Int16)root_tdb->getSubqueryType();
  compilerStatsInfo->bmoMemLimitPerNode() = root_tdb->getBmoMemoryLimitPerNode();
  compilerStatsInfo->estBmoMemPerNode() = root_tdb->getEstBmoMemoryPerNode();

  NADELETEBASIC(partInputDataDescs, generator->wHeap());
  NADELETEBASIC(nodeMap, generator->wHeap());

  // Genesis 10-990114-6293:
  // don't recompile a SELECT query if transmode changes to READ ONLY.
  if (readOnlyTransIsOK()) root_tdb->setReadonlyTransactionOK();

  // Inserts into non-audited indexes do not need to run in a transaction,
  // if one does not exist. If one exists (which is the case during a create
  // index operation), need to pass transid to all ESPs during the load
  // index phase, otherwise they will get error 73s returned when they open
  // the index. Store this information in the root TDB, so that the transaction
  // can be passed to ESPs if needed.  Dp2Insert::codeGen has set this
  // generator flag.

  NABoolean recompWarn =
    (CmpCommon::getDefault(RECOMPILATION_WARNINGS) == DF_ON);
  if (recompWarn) root_tdb->setRecompWarn();

  // Set the FROM_SHOWPLAN flag if the statement is from a showplan
  const NAString * val =
    ActiveControlDB()->getControlSessionValue("SHOWPLAN");
  if ( !(childOperType_ == REL_CONTROL_SESSION)
       && (val) && (*val == "ON") )
    root_tdb->setFromShowplan();

  if (CmpCommon::getDefault(EXE_LOG_RETRY_IPC) == DF_ON)
    root_tdb->setLogRetriedIpcErrors(TRUE);

  if (anyEspFragments)
  {
    if (generator->getBindWA()->queryCanUseSeaMonster() &&
        generator->getQueryUsesSM())
      root_tdb->setQueryUsesSM();
  }

  generator->setGenObj(this, root_tdb);

  return 0;

} // RelRoot::codeGen()

short Sort::generateTdb(Generator * generator,
                        ComTdb * child_tdb,
                        ex_expr * sortKeyExpr,
                        ex_expr * sortRecExpr,
                        ULng32 sortKeyLen,
                        ULng32 sortRecLen,
                        ULng32 sortPrefixKeyLen,
                        ex_cri_desc * given_desc,
                        ex_cri_desc * returned_desc,
                        ex_cri_desc * work_cri_desc,
                        Lng32 saveNumEsps,
                        ExplainTuple *childExplainTuple,
                        NABoolean resizeCifRecord,
                        NABoolean considerBufferDefrag,
                        NABoolean operatorCIF)
{

  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  ULng32 numBuffers = (ULng32)getDefault(GEN_SORT_NUM_BUFFERS);
  
  CostScalar bufferSize = getDefault(GEN_SORT_MAX_BUFFER_SIZE);

  UInt32 bufferSize_as_uint32 = 
    (UInt32)(MINOF(CostScalar(UINT_MAX), bufferSize)).getValue(); 

  // allocate buffer to hold atlease one row
  bufferSize_as_uint32 = MAXOF(bufferSize_as_uint32, sortRecLen);

  GenAssert(sortRecLen <= bufferSize_as_uint32, 
      "Record Len greater than GEN_SORT_MAX_BUFFER_SIZE");
  
  ComTdbSort * sort_tdb = 0;
  // always start with quick sort. Sort will switch to
  // replacement sort in case of overflow at runtime.
  SortOptions *sort_options = new(generator->getSpace()) SortOptions();

  Lng32 max_num_buffers = (Lng32)numBuffers;
  NAString tmp;
  CmpCommon::getDefault(SORT_ALGO, tmp, -1);
  if(tmp == "HEAP")
    sort_options->sortType() = SortOptions::ITER_HEAP;
  else if(tmp == "REPSEL")
    sort_options->sortType() = SortOptions::REPLACEMENT_SELECT;
  else if(tmp == "IQS")
    sort_options->sortType() = SortOptions::ITER_QUICK;
  else if(tmp == "QS")
    sort_options->sortType() = SortOptions::QUICKSORT;
  max_num_buffers = (Lng32)getDefault(GEN_SORT_MAX_NUM_BUFFERS);
  sort_options->internalSort() = TRUE;

  unsigned short threshold = (unsigned short) CmpCommon::getDefaultLong(SCRATCH_FREESPACE_THRESHOLD_PERCENT);
  sort_options->scratchFreeSpaceThresholdPct() = threshold;
  sort_options->sortMaxHeapSize() = (short)getDefault(SORT_MAX_HEAP_SIZE_MB);
  sort_options->mergeBufferUnit() = (short)getDefault(SORT_MERGE_BUFFER_UNIT_56KB);
 
  //512kb default size initiliazed in sort_options.
  if(sortRecLen >= sort_options->scratchIOBlockSize())
  {
    Int32 maxScratchIOBlockSize = (Int32)getDefault(SCRATCH_IO_BLOCKSIZE_SORT_MAX);
    // allocate space for atleast one row.
    maxScratchIOBlockSize = MAXOF(maxScratchIOBlockSize, sortRecLen);

    GenAssert(sortRecLen <= maxScratchIOBlockSize, 
         "sortRecLen is greater than SCRATCH_IO_BLOCKSIZE_SORT_MAX");
    sort_options->scratchIOBlockSize() = MINOF(sortRecLen * 128, maxScratchIOBlockSize);
  }
  
  sort_options->scratchIOVectorSize() = (Int16)getDefault(SCRATCH_IO_VECTOR_SIZE_SORT);

  if (CmpCommon::getDefault(EXE_BMO_SET_BUFFERED_WRITES) == DF_ON)
    sort_options->setBufferedWrites(TRUE);
  if (CmpCommon::getDefault(EXE_DIAGNOSTIC_EVENTS) == DF_ON)
    sort_options->setLogDiagnostics(TRUE);
  
  // Disable Compiler Hints checks if: CQD is ON or if SYSTEM - only for HDD
  if (
      (CmpCommon::getDefault(EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW_SORT) == DF_ON)
      ||
      (
       ((generator->getOverflowMode()== ComTdb::OFM_DISK) ||
       (generator->getOverflowMode()== ComTdb::OFM_MMAP)) 
       && 
       (CmpCommon::getDefault(EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW_SORT) 
        == DF_SYSTEM )
      )
     )
	 sort_options->setDisableCmpHintsOverflow(TRUE);
	 
  if (CmpCommon::getDefault(EXE_BMO_DISABLE_OVERFLOW) == DF_ON)
    sort_options->dontOverflow() = TRUE;
  if (CmpCommon::getDefault(SORT_INTERMEDIATE_SCRATCH_CLEANUP) == DF_ON)
    sort_options->setIntermediateScratchCleanup(TRUE);

  sort_options->setResizeCifRecord(resizeCifRecord);
  sort_options->setConsiderBufferDefrag(considerBufferDefrag);

  short memoryQuotaMB = 0;
  double memoryQuotaRatio;
  Lng32 numStreams;
  double bmoMemoryUsagePerNode = generator->getEstMemPerNode(getKey(), numStreams);

  if (CmpCommon::getDefault(SORT_MEMORY_QUOTA_SYSTEM) != DF_OFF)
  {
    // The CQD EXE_MEM_LIMIT_PER_BMO_IN_MB has precedence over the mem quota sys
    memoryQuotaMB = (UInt16)defs.getAsDouble(EXE_MEM_LIMIT_PER_BMO_IN_MB);

    if (memoryQuotaMB > 0) {
     sort_options->memoryQuotaMB() = memoryQuotaMB;
    } else {
  
      UInt16 numBMOsInFrag = (UInt16)generator->getFragmentDir()->getNumBMOs();
  
      // Apply quota system if either one the following two is true:
      //   1. the memory limit feature is turned off and more than one BMOs
      //   2. the memory limit feature is turned on
      
      NABoolean mlimitPerNode = defs.getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB) > 0;
  
      if ( mlimitPerNode || numBMOsInFrag > 1 ||
         (numBMOsInFrag == 1 && CmpCommon::getDefault(EXE_SINGLE_BMO_QUOTA) == DF_ON)) {
  
          memoryQuotaMB = (short)
             computeMemoryQuota(generator->getEspLevel() == 0,
                                mlimitPerNode,
                                generator->getBMOsMemoryLimitPerNode().value(),
                                generator->getTotalNumBMOs(),
                                generator->getTotalBMOsMemoryPerNode().value(),
                                numBMOsInFrag, 
                                bmoMemoryUsagePerNode,
                                numStreams,
                                memoryQuotaRatio
                               );
  
      }            
      Lng32 sortMemoryLowbound = defs.getAsLong(BMO_MEMORY_LIMIT_LOWER_BOUND_SORT);
      Lng32 memoryUpperbound = defs.getAsLong(BMO_MEMORY_LIMIT_UPPER_BOUND);
  
      if ( memoryQuotaMB < sortMemoryLowbound ) {
         memoryQuotaMB = (short)sortMemoryLowbound;
         memoryQuotaRatio = BMOQuotaRatio::MIN_QUOTA;
      }
      else if (memoryQuotaMB >  memoryUpperbound)
         memoryQuotaMB = memoryUpperbound;
    }
  }

   //BMO settings. By Default set this value to max available 
   //irrespective of quota is enabled or disabled. Sort at run time
   //will manage to check for quota and available physical memory
   //before consuming memory. Note that if memoryQuota is set zero,
   //sort may not do physical memory or memory pressure checks.
  if ( memoryQuotaMB <= 0  && 
       ! sort_options->disableCmpHintsOverflow() ) // compiler hints enabled
   {
     memoryQuotaMB = (UInt16)defs.getAsLong(EXE_MEMORY_AVAILABLE_IN_MB);
   }
   sort_options->memoryQuotaMB() = memoryQuotaMB;

  if(generator->getOverflowMode() == ComTdb::OFM_SSD )
    sort_options->bmoMaxMemThresholdMB() = (UInt16)defs.getAsLong(SSD_BMO_MAX_MEM_THRESHOLD_IN_MB);
  else
    sort_options->bmoMaxMemThresholdMB() = (UInt16)defs.getAsLong(EXE_MEMORY_AVAILABLE_IN_MB);

   sort_options->pressureThreshold() = 
                  (short)getDefault(GEN_MEM_PRESSURE_THRESHOLD);
   
  short sortGrowthPercent = 
    RelExpr::bmoGrowthPercent(getEstRowsUsed(), getMaxCardEst());

  sort_tdb = new(generator->getSpace())
    ComTdbSort(sortKeyExpr,
	       sortRecExpr,
	       sortKeyLen,
	       sortRecLen,
	       sortPrefixKeyLen,
	       returned_desc->noTuples() - 1,
	       child_tdb,
	       given_desc,
	       returned_desc,
	       work_cri_desc,

	       // if sort input is from top, switch the UP and DOWN queue
	       // sizes
	       (sortFromTop()
		? (queue_index)getDefault(GEN_SORT_SIZE_UP)
		: (queue_index)getDefault(GEN_SORT_SIZE_DOWN)),
	       (queue_index)getDefault(GEN_SORT_SIZE_UP),
	       (Cardinality) (getInputCardinality() * getEstRowsUsed()).getValue(),
	       numBuffers,
	       bufferSize_as_uint32,
	       max_num_buffers,
	       sort_options,
           sortGrowthPercent);
  sort_tdb->setCollectNFErrors(this->collectNFErrors());

  sort_tdb->setSortFromTop(sortFromTop());
  sort_tdb->setOverflowMode(generator->getOverflowMode());
  sort_tdb->setTopNSortEnabled(CmpCommon::getDefault(GEN_SORT_TOPN) == DF_ON);
  sort_tdb->setBmoQuotaRatio(memoryQuotaRatio);
  
  if (generator->getUserSidetreeInsert())
    sort_tdb->setUserSidetreeInsert(TRUE);

  if (getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_) 
    sort_tdb->setTolerateNonFatalError(TRUE);

  sort_tdb->setCIFON(operatorCIF);

  generator->initTdbFields(sort_tdb);

  double sortMemEst = generator->getEstMemPerInst(getKey());
  sort_tdb->setEstimatedMemoryUsage(sortMemEst / 1024);
  generator->addToTotalEstimatedMemory(sortMemEst);

  if (sortPrefixKeyLen > 0)
    ((ComTdbSort *)sort_tdb)->setPartialSort(TRUE);  // do partial sort

  if(CmpCommon::getDefaultLong(SORT_REC_THRESHOLD) > 0)
      ((ComTdbSort *)sort_tdb)->setMinimalSortRecs(CmpCommon::getDefaultLong(SORT_REC_THRESHOLD));

  sort_tdb->setMemoryContingencyMB(getDefault(PHY_MEM_CONTINGENCY_MB));
  float bmoCtzFactor;
  defs.getFloat(BMO_CITIZENSHIP_FACTOR, bmoCtzFactor);
  sort_tdb->setBmoCitizenshipFactor((Float32)bmoCtzFactor);
  sort_tdb->setSortMemEstInKBPerNode(bmoMemoryUsagePerNode /1024);
  if (sortNRows())
     sort_tdb->setTopNThreshold(defs.getAsLong(GEN_SORT_TOPN_THRESHOLD));
  if (!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(sort_tdb, childExplainTuple, 0, generator));
  }

  // set the new up cri desc.
  generator->setCriDesc(returned_desc, Generator::UP);

  generator->setGenObj(this, sort_tdb);

  // reset the expression generation flag to generate float validation pcode
  generator->setGenNoFloatValidatePCode(FALSE);

  return 0;
}

//////////////////////////////////////////////////////////////
//
// Sort::codeGen()
//
/////////////////////////////////////////////////////////
short Sort::codeGen(Generator * generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  Space * space = generator->getSpace();

   ////////////////////////////////////////////////////////////////////////
  //
  // Layout at this node:
  //
  // |-------------------------------------------------|
  // | input data  |  Sorted data  | child's data      |
  // | ( I tupps ) |  ( 1 tupp )   | ( C tupps )       |
  // |-------------------------------------------------|
  // <-- returned row to parent --->
  // <------------ returned row from child ------------>
  //
  // input data:        the atp input to this node by its parent.
  // sorted data:       tupp where the sorted row is.
  //                    this data is accessed by the key and by the data
  //                    separately.
  //                    The key data is in SQLMX_KEY_FORMAT and the data
  //                    will be in either internal or exploded format.
  // child data:        tupps appended by the child
  //
  // Input to child:    I + 1 tupps
  //
  // Tupps returned from child are only used to create the
  // sorted data. They are not returned to parent.
  //
  /////////////////////////////////////////////////////////////////////////
  // Tupps returned from child are only used to create the
  // sorted data. They are not returned to parent.
  //
  /////////////////////////////////////////////////////////////////////////

  MapTable * last_map_table = generator->getLastMapTable();

  ex_cri_desc * given_desc
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returned_desc
    = new(space) ex_cri_desc(given_desc->noTuples() + 1, space);

  Int32 work_atp = 1; // temps
  Int32 work_atp_index = 2;  // where the result row will be
  ex_cri_desc * work_cri_desc = new(space) ex_cri_desc(3, space);

  //All the records in the table will not be processed by a single sort 
  //instance if multiple sort instances are involved within ESPs.
  Lng32 saveNumEsps = generator->getNumESPs();
  
  if (sortFromTop())
    generator->setCriDesc(returned_desc, Generator::DOWN);

  // generate code for child tree
  child(0)->codeGen(generator);
  
  //This value is set inside generator by my parent exchange node,
  //as a global variable. Reset the saveNumEsps value back into
  //generator since codegen of my children exchange nodes may have
  //changed it. Resetting is performed here so the codegen of right
  //child nodes of my parent gets the proper value.  
  generator->setNumESPs(saveNumEsps);
  
  ComTdb * child_tdb = (ComTdb *)(generator->getGenObj());
  ExplainTuple *childExplainTuple = generator->getExplainTuple();

  // Before generating any expression for this node, set the
  // the expression generation flag not to generate float
  // validation PCode. This is to speed up PCode evaluation
  generator->setGenNoFloatValidatePCode(TRUE);

  // generate an expression to create the input row
  // to be sent to sort.
  // The input row consists of:
  //   n + m values
  // where, n is the number of encoded key columns.
  //        m is the total number of column values.
  // At runtime, a contiguous row of n + m columns is created
  // and then given to sort.
  // sort prefix key columns are indexed from 0 to k where k < n.


  // The data within the Sort buffer will be contiguous in the format
  // | encoded keys | returned column values |
  // ----------------------------------------
  // The keys will be in key format.
  // The returned column values will be in Exploded or Compressed internal
  // format.

  // generate the key encode value id list used for sorting
  UInt32  sortKeyLen = 0;
  UInt32  sortPrefixKeyLen = 0;
  Int32   prefixKeyCnt = getPrefixSortKey().entries();
  ValueIdList sortKeyValIdList;
  CollIndex   sortKeyListIndex;
  CollIndex   sortRecListIndex;
  for (sortKeyListIndex = 0;
       sortKeyListIndex < getSortKey().entries();
       sortKeyListIndex++)
    {
      ItemExpr * skey_node =
        ((getSortKey()[sortKeyListIndex]).getValueDesc())->getItemExpr();

      short desc_flag = FALSE;

      if (skey_node->getOperatorType() == ITM_INVERSE)
	{
	  desc_flag = TRUE;
	}

      if (skey_node->getValueId().getType().getVarLenHdrSize() > 0)
	{
	  // Explode varchars by moving them to a fixed field
	  // whose length is equal to the max length of varchar.
	  // 5/8/98: add support for VARNCHAR

          const CharType& char_type =
		(CharType&)(skey_node->getValueId().getType());

          //no cast to fixed char in the case of collation (Czech) 
          if (!CollationInfo::isSystemCollation(char_type.getCollation()))
	  {
	    skey_node =
	      new(generator->wHeap())
		Cast (skey_node,
		      (new(generator->wHeap())
			 SQLChar(generator->wHeap(),
                          CharLenInfo(char_type.getStrCharLimit(), char_type.getDataStorageSize()),
			  char_type.supportsSQLnull(),
			  FALSE, FALSE, FALSE,
			  char_type.getCharSet(),
			  char_type.getCollation(),
			  char_type.getCoercibility()
				)
		     )
		    );
	  }
	}

      CompEncode * enode
	= new(generator->wHeap()) CompEncode(skey_node, desc_flag);

      enode->bindNode(generator->getBindWA());

      sortKeyLen += enode->getValueId().getType().getTotalSize();

      sortKeyValIdList.insert(enode->getValueId());

      if (sortKeyListIndex < (CollIndex) prefixKeyCnt)
        // sort key length is the prefix sort key length
        // Note we need do this assignment only once. Need a better way
        sortPrefixKeyLen = sortKeyLen;
    }
/*
  // Generate the key encode expression ...
  ex_expr * sortKeyExpr = 0;
  exp_gen->generateContiguousMoveExpr(sortKeyValIdList,
                                      0,     // no conv nodes
                                      work_atp, work_atp_index,
                                      ExpTupleDesc::SQLMX_KEY_FORMAT,
                                      sortKeyLen, &sortKeyExpr,
                                      0,  // no tupp descr
                                      ExpTupleDesc::SHORT_FORMAT);
*/
  // Now generate the returned column value value id list that moves the input
  // data into the sort input buffer using convert nodes.
  ValueIdList sortRecValIdList;
  ValueId     valId;
  for (valId = getGroupAttr()->getCharacteristicOutputs().init();
       getGroupAttr()->getCharacteristicOutputs().next(valId);
       getGroupAttr()->getCharacteristicOutputs().advance(valId))
    {
      // add the convert node
      Convert * convNode = new(generator->wHeap())Convert(valId.getItemExpr());
      convNode->bindNode(generator->getBindWA());
      sortRecValIdList.insert(convNode->getValueId());
    }

  UInt32 sortRecLen = 0;
  ex_expr * sortRecExpr = 0;
  ex_expr * sortKeyExpr = 0;
  ExpTupleDesc * tuple_desc = 0;


  // contains the value ids that are being returned (the sorted values)
  MapTable * returnedMapTable = 0;
  ExpTupleDesc::TupleDataFormat tupleFormat = generator->getInternalFormat();

  // resizeCifRecord indicator that tells us whether we need to resize the CIF row or not
  // if CIF is not used then no resizing is done
  // if CIF is used the based on some logic that will be defined we determine whether we need
  // to resize the row or not
  NABoolean resizeCifRecord = FALSE;
  // Sometimes only the key value id list has entries and there are no
  // additional characteristic outputs.
  // This happens when Sort is used as a blocking operator for NAR (Non Atomic
  // Rowsets).
  NABoolean bmo_affinity = (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO_AFFINITY) == DF_ON);
  NABoolean considerBufferDefrag = FALSE;

  if (sortRecValIdList.entries() > 0)
  {
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
      //apply heuristic to determine the tuple format and whether we need to resize the row or not
     tupleFormat = determineInternalFormat( sortRecValIdList,
                                            this,
                                            resizeCifRecord,
                                            generator,
                                            bmo_affinity,
                                            considerBufferDefrag);
     considerBufferDefrag = considerBufferDefrag && resizeCifRecord;
    }

  }


  if (tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
  {
    exp_gen->generateContiguousMoveExpr(sortKeyValIdList,
                                         0,     // no conv nodes
                                         work_atp, work_atp_index,
                                         ExpTupleDesc::SQLMX_KEY_FORMAT,
                                         sortKeyLen, &sortKeyExpr,
                                         0,  // no tupp descr
                                         ExpTupleDesc::SHORT_FORMAT);

    exp_gen->generateContiguousMoveExpr(sortRecValIdList,
                                        0,   // no convert nodes
                                        work_atp, work_atp_index,
                                        tupleFormat,
                                        sortRecLen, &sortRecExpr,
                                        &tuple_desc, ExpTupleDesc::SHORT_FORMAT);
    sortRecLen += sortKeyLen;
    if (resizeCifRecord)
    {
      // with CIF if we need to resize the rows then we need to keep track of the length
      // of the row with the row itself
      // in this case the allocated space for the row will be as folows
      // |row size --(4 bytes)|sort key (sortkeyLen)| data (data length --may be variable)  |
      // ------------------------------------
      sortRecLen += sizeof(UInt32);
    }
  }
  else
  {
    CMPASSERT(resizeCifRecord == FALSE);
    sortKeyValIdList.insertSet(sortRecValIdList);
    exp_gen->generateContiguousMoveExpr(sortKeyValIdList,
                                          0,     // no conv nodes
                                          work_atp, work_atp_index,
                                          ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
                                          sortRecLen, &sortRecExpr,
                                          &tuple_desc,
                                          ExpTupleDesc::SHORT_FORMAT);

  }


  // add in the total area for the keys since we generated this separately
  // and ensure the size is a factor of 8 so the data aligns correctly


  


  // describe the returned row
  returned_desc->setTupleDescriptor((UInt16)returned_desc->noTuples() - 1,
                                    tuple_desc);

  returnedMapTable = generator->appendAtEnd(); // allocates a new map table
  generator->unlinkLast();

  // Add the returned values to the map table. We get the returned value
  // value id from the char outputs and the item expr from the sort record
  // value id list, starting with the first item expr added to the sort
  // record value id list by the for loop immediately preceding this one.
  sortRecListIndex = 0;
  for (valId = getGroupAttr()->getCharacteristicOutputs().init();
       getGroupAttr()->getCharacteristicOutputs().next(valId);
       getGroupAttr()->getCharacteristicOutputs().advance(valId))
    {
      // ????????????The first time through this loop, sortRecListIndex will point
      // to the first return value convert node. This is because we
      // stopped incrementing the sortRecListIndex after the last sort
      // key was added and we did not increment it when we added the
      // return value convert nodes.
      Attributes *attr =
        generator->getMapInfo(sortRecValIdList[sortRecListIndex++])->getAttr();

      // ...add it to the new map table as if it belonged to
      // the original value id...
      MapInfo * mi =
	generator->addMapInfoToThis(returnedMapTable, valId, attr);

      // All reference to the returned values from this point on
      // will be at atp = 0, atp_index = last entry in returned desc.
      // Offset will be the same as in the workAtp.
      mi->getAttr()->setAtp(0);
      mi->getAttr()->setAtpIndex(returned_desc->noTuples() - 1);

      // ... and make sure no more code gets generated for it.
      mi->codeGenerated();

    }

  // remove all appended map tables and return the returnedMapTable
  generator->removeAll(last_map_table);
  generator->appendAtEnd(returnedMapTable);

  short rc =
    generateTdb(generator,
		child_tdb,
		sortKeyExpr,
                sortRecExpr,
		sortKeyLen,
		sortRecLen,
		sortPrefixKeyLen,
		given_desc,
		returned_desc,
		work_cri_desc,
		saveNumEsps,
		childExplainTuple,
		resizeCifRecord,
		considerBufferDefrag,
		(tupleFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT));

  return rc;
}


ExpTupleDesc::TupleDataFormat Sort::determineInternalFormat( const ValueIdList & valIdList,
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

  UInt32 sortKeyLength = getSortKey().getRowLength();
  return generator->determineInternalFormat(valIdList, 
                                            relExpr, 
                                            resizeCifRecord, 
                                            bmo_cif,
                                            bmo_affinity,
                                            considerBufferDefrag,
                                            sortKeyLength);

}
//////////////////////////////////////////////////////////////
//
// SortFromTop::codeGen()
//
/////////////////////////////////////////////////////////
short SortFromTop::codeGen(Generator * generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  MapTable * last_map_table = generator->getLastMapTable();

  ex_cri_desc * given_desc
    = generator->getCriDesc(Generator::DOWN);

  // no tuples returned from this operator
  ex_cri_desc * returned_desc = given_desc;

  // child gets one sorted row created in this operator.
  ex_cri_desc * child_desc
    = new(space) ex_cri_desc(given_desc->noTuples() + 1, space);

  Int32 work_atp = 1; // temps
  Int32 work_atp_index = 2;  // where the result row will be
  ex_cri_desc * work_cri_desc = new(space) ex_cri_desc(3, space);

  //All the records in the table will not be processed by a single sort 
  //instance if multiple sort instances are involved within ESPs.
  Lng32 saveNumEsps = generator->getNumESPs();

  generator->setCriDesc(child_desc, Generator::DOWN);

  // Before generating any expression for this node, set the
  // the expression generation flag not to generate float
  // validation PCode. This is to speed up PCode evaluation
  generator->setGenNoFloatValidatePCode(TRUE);

  MapTable *myMapTable = generator->appendAtEnd();
  
  ULng32 sort_rec_len = 0;
  ULng32 sort_key_len = 0;
  ex_expr * sort_rec_expr = 0;
  ex_expr * sort_key_expr = 0;
  ExpTupleDesc * tuple_desc = 0;

  ValueIdList sortRecVIDlist;
  ValueIdList sortKeyVIDlist;
  CollIndex ii = 0;
  for (ii = 0; ii < getSortKey().entries(); ii++)
    {
      ItemExpr * skey_node =
        ((getSortKey()[ii]).getValueDesc())->getItemExpr();

      if (skey_node->getOperatorType() != ITM_INDEXCOLUMN)
	GenAssert(0, "Must be IndexColumn");

      IndexColumn * ic = (IndexColumn*)skey_node;
      NABoolean found = FALSE;
      CollIndex jj = 0;
      while ((NOT found) && (jj < getSortRecExpr().entries()))
	{
	  const ItemExpr *assignExpr = 
	    getSortRecExpr()[jj].getItemExpr();

	  ItemExpr * tgtCol = assignExpr->child(0)->castToItemExpr();
	  if (tgtCol->getOperatorType() != ITM_BASECOLUMN)
	    GenAssert(0, "Must be BaseColumn");

	  ValueId tgtValueId = tgtCol->getValueId();

	  ValueId srcValueId = 
	    assignExpr->child(1)->castToItemExpr()->getValueId();

	  ItemExpr * srcVal = assignExpr->child(1)->castToItemExpr();

	  if (ic->getNAColumn()->getColName() == 
	      ((BaseColumn*)tgtCol)->getNAColumn()->getColName())
	    {
	      found = TRUE;

	      //***TBD*** need to handle descnding. Get that from index desc.
	      short desc_flag = FALSE;
	      if (ic->getNAColumn()->getClusteringKeyOrdering() == DESCENDING)
		{
		  desc_flag = TRUE;
		}

	      if (skey_node->getValueId().getType().getVarLenHdrSize() > 0)
		{
		  // Explode varchars by moving them to a fixed field
		  // whose length is equal to the max length of varchar.
		  
		  const CharType& char_type =
		    (CharType&)(skey_node->getValueId().getType());
		  
		  //no cast to fixed char in the case of collation (Czech) 
		  if (!CollationInfo::isSystemCollation(char_type.getCollation()))
		    {
		      skey_node =
			new(generator->wHeap())
			Cast (srcVal,
			      (new(generator->wHeap())
			       SQLChar(generator->wHeap(),
                                    CharLenInfo(char_type.getStrCharLimit(), char_type.getDataStorageSize()),
				    char_type.supportsSQLnull(),
				    FALSE, FALSE, FALSE,
				    char_type.getCharSet(),
				    char_type.getCollation(),
				    char_type.getCoercibility()
				    )
			       )
			      );
		    }
		}
	      else
		{
		  skey_node = new(generator->wHeap()) 
		    Cast(srcVal, &tgtValueId.getType());
		}

	      CompEncode * enode
		= new(generator->wHeap()) CompEncode(skey_node, desc_flag);
	      
	      enode->bindNode(generator->getBindWA());
	      
	      sort_key_len += enode->getValueId().getType().getTotalSize();
	      
	      sortKeyVIDlist.insert(enode->getValueId()); 
	    } // if
	  
	  jj++;
	} // while
      
      if (NOT found)
	{
	  GenAssert(0, "Key not found in newRecExprArray");
	}
    } // for
  // genearate sort key expr
  //ex_expr * sort_key_expr = 0;
  if (sortKeyVIDlist.entries() > 0)
  {
    exp_gen->generateContiguousMoveExpr(sortKeyVIDlist,
                                      0,     // no conv nodes
                                      work_atp, work_atp_index,
                                      ExpTupleDesc::SQLMX_KEY_FORMAT,
                                      sort_key_len, &sort_key_expr,
                                      0,  // no tupp descr
                                      ExpTupleDesc::SHORT_FORMAT);
  
  }
  //sortRecVIDlist = sortKeyVIDlist;

  for (ii = 0; ii < getSortRecExpr().entries(); ii++)
    {
      const ItemExpr *assignExpr = 
	getSortRecExpr()[ii].getItemExpr();
      
      ValueId tgtValueId = 
	assignExpr->child(0)->castToItemExpr()->getValueId();
      ValueId srcValueId = 
	assignExpr->child(1)->castToItemExpr()->getValueId();
      
      ItemExpr * ie = NULL;
      ie = new(generator->wHeap())
	Cast(assignExpr->child(1), 
	     &tgtValueId.getType());
      
      ie->bindNode(generator->getBindWA());
      sortRecVIDlist.insert(ie->getValueId());
    } // for

  ExpTupleDesc::TupleDataFormat tupleFormat = generator->getInternalFormat();
  NABoolean resizeCifRecord = FALSE;
  NABoolean considerBufferDefrag = FALSE;

  if (sortRecVIDlist.entries()>0)
  {
    tupleFormat = determineInternalFormat( sortRecVIDlist,
                                           this,
                                           resizeCifRecord,
                                           generator,
                                           FALSE,
                                           considerBufferDefrag);

    exp_gen->generateContiguousMoveExpr(sortRecVIDlist, 
				      0 /*don't add conv nodes*/,
				      work_atp, work_atp_index,
				      tupleFormat,
				      sort_rec_len, &sort_rec_expr,
				      &tuple_desc, ExpTupleDesc::SHORT_FORMAT);
  }
  //sort_key_len = ROUND8(sort_key_len); ?????????

  sort_rec_len += sort_key_len;

  if (resizeCifRecord)
  {
    // with CIF if we need to resize the rows then we need to keep track of the length
    // of the row with the row itself
    // in this case the allocated space for the row will be as folows
    // |row size --(4 bytes)|sort key (sortkeyLen)| data (data length --may be variable)  |
    // ------------------------------------
    sort_rec_len += sizeof(UInt32);
  }


  for (CollIndex i = 0; i < (CollIndex) sortRecVIDlist.entries(); i++)
    {
      ItemExpr * cn = (sortRecVIDlist[i]).getItemExpr();
      
      Attributes *attrib =
	generator->getMapInfo(cn->getValueId())->getAttr();
      
      MapInfo * mi =
	generator->
	addMapInfoToThis(myMapTable, 
			 cn->child(0)->castToItemExpr()->getValueId(), 
			 attrib);
      
      // All reference to the sorted values from this point on
      // will be at atp = 0, atp_index = last entry in child desc.
      // Offset will be the same as in the workAtp.
      mi->getAttr()->setAtp(0);
      mi->getAttr()->setAtpIndex(child_desc->noTuples() - 1);

      // ... and make sure no more code gets generated for it.
      mi->codeGenerated();
    }

  // generate code for child tree
  child(0)->codeGen(generator);
  
  //This value is set inside generator by my parent exchange node,
  //as a global variable. Reset the saveNumEsps value back into
  //generator since codegen of my children exchange nodes may have
  //changed it. Resetting is performed here so the codegen of right
  //child nodes of my parent gets the proper value.  
  generator->setNumESPs(saveNumEsps);
  
  ComTdb * child_tdb = (ComTdb *)(generator->getGenObj());
  ExplainTuple *childExplainTuple = generator->getExplainTuple();

  // remove all appended map tables. No values are returned by
  // this node.
  generator->removeAll(last_map_table);

  short rc =
    generateTdb(generator,
		child_tdb,
                sort_key_expr,
		sort_rec_expr,
		sort_key_len,
		sort_rec_len,
		0, //sort_prefix_key_len,
		given_desc,
		child_desc,
		work_cri_desc,
		saveNumEsps,
		childExplainTuple,
		resizeCifRecord,
		considerBufferDefrag);

  return rc;
}

CostScalar Sort::getEstimatedRunTimeMemoryUsage(Generator *generator, NABoolean perNode, Lng32 *numStreams)
{
  GroupAttributes * childGroupAttr = child(0).getGroupAttr();
  Lng32 childRecordSize = 
      childGroupAttr->getCharacteristicOutputs().getRowLength();
  CostScalar rowsUsed;
  if (sortNRows() && (topNRows_ > 0)
            && (topNRows_ <= getDefault(GEN_SORT_TOPN_THRESHOLD)))
     rowsUsed = topNRows_; 
  else
     rowsUsed = getEstRowsUsed();
  CostScalar totalMemory = rowsUsed * childRecordSize;
  CostScalar estMemPerNode;
  CostScalar estMemPerInst;
 
  //TODO: Line below dumps core at times 
  //const CostScalar maxCard = childGroupAttr->getResultMaxCardinalityForEmptyInput();
  const CostScalar maxCard = 0;

  Lng32 numOfStreams = 1;
  const PhysicalProperty* const phyProp = getPhysicalProperty();
  if (phyProp != NULL)
  {
     PartitioningFunction * partFunc = phyProp -> getPartitioningFunction() ;
     numOfStreams = partFunc->getCountOfPartitions();
     if (numOfStreams <= 0)
        numOfStreams = 1;
  }
  if (numStreams != NULL)
     *numStreams = numOfStreams;
  estMemPerNode = totalMemory / MINOF(MAXOF(gpClusterInfo->getTotalNumberOfCPUs(), 1), numOfStreams);
  estMemPerInst = totalMemory / numOfStreams;
  OperBMOQuota *operBMOQuota = new (generator->wHeap()) OperBMOQuota(getKey(), numOfStreams,
                                                  estMemPerNode, estMemPerInst, rowsUsed, maxCard);
  generator->getBMOQuotaMap()->insert(operBMOQuota);
  if (perNode)
     return estMemPerNode;
  else
     return estMemPerInst; 
}

/////////////////////////////////////////////////////////
//
// Tuple::codeGen()
//
/////////////////////////////////////////////////////////
short Tuple::codeGen(Generator * generator)
{
  // code generation for this node doesn't do anything.
  // A Tuple node returns one tuple of expression of constants. This
  // expression is evaluated where it is used. Since this node doesn't
  // produce anything, the returned cri desc is same as the input cri desc.
  // We could do away with generating something here but the
  // parent node expects a child node to return 'something' before it
  // can continue.
  Queue * qList = new(generator->getSpace()) Queue(generator->getSpace());
  ExpGenerator *expGen = generator->getExpGenerator();

  // expression to conditionally return 0 or more rows.
  ex_expr *predExpr = NULL;

  // generate tuple selection expression, if present
  if(NOT selectionPred().isEmpty())
  {
    ItemExpr* pred = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
    expGen->generateExpr(pred->getValueId(),ex_expr::exp_SCAN_PRED,&predExpr);
  }

  ComTdbTupleLeaf *tuple_tdb = new(generator->getSpace())
    ComTdbTupleLeaf(qList,
		    0, // no tuple returned. Length = 0.
		    0, // no tupp index
		    predExpr,
                    generator->getCriDesc(Generator::DOWN),
		    generator->getCriDesc(Generator::DOWN),
		    (queue_index)getDefault(GEN_TUPL_SIZE_DOWN),
		    (queue_index)getDefault(GEN_TUPL_SIZE_UP),
		    (Cardinality) (getInputCardinality() * getEstRowsUsed()).getValue(),
		    getDefault(GEN_TUPL_NUM_BUFFERS),
		    getDefault(GEN_TUPL_BUFFER_SIZE));
  generator->initTdbFields(tuple_tdb);

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
	 addExplainInfo(tuple_tdb, 0, 0, generator));
  }

  generator->setGenObj(this, tuple_tdb);
  generator->setCriDesc(generator->getCriDesc(Generator::DOWN),
			Generator::UP);

  return 0;
}

/////////////////////////////////////////////////////////
//
// TupleList::codeGen()
//
/////////////////////////////////////////////////////////
short TupleList::codeGen(Generator * generator)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen  = generator->getExpGenerator();
  ex_cri_desc * givenDesc
    = generator->getCriDesc(Generator::DOWN);
  ex_cri_desc * returnedDesc
    = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);
  Int32 tuppIndex = returnedDesc->noTuples() - 1;

  // disable common subexpression elimination for now.
  // There is a problem which shows up due to common subexpression
  // elimination. See case 10-040402-2209.
  // After the problem is diagnosed and fixed in tuple list,
  // we won't need to disable subexp elimination.
  generator->getExpGenerator()->setEnableCommonSubexpressionElimination(FALSE);

  Queue * qList = new(generator->getSpace()) Queue(generator->getSpace());
  ULng32 tupleLen = 0;
  ExpTupleDesc * tupleDesc = 0;
  ExprValueId eVid(tupleExprTree());
  ItemExprTreeAsList tupleList(&eVid, ITM_ITEM_LIST);
  CollIndex nTupEntries = (CollIndex) tupleList.entries();
  for (CollIndex i = 0; i < nTupEntries; i++)
    {
      ex_expr * moveExpr = NULL;

      ItemExpr * tuple =
	((ItemExpr *) tupleList[i])->child(0)->castToItemExpr();

      ExprValueId tVid(tuple);
      ItemExprTreeAsList tupleTree(&tVid, ITM_ITEM_LIST);

      ValueIdList convVIDlist;
      BindWA *bindWA = generator->getBindWA();

      NABoolean castTo = castToList().entries() > 0;
      for (CollIndex j = 0; j < tupleExpr().entries(); j++)
	{
	  ItemExpr * castNode = tupleExpr()[j].getItemExpr();
	  ItemExpr * childNode = (ItemExpr *) tupleTree[j];
          if (castTo)
          {
            const NAType &srcType = childNode->getValueId().getType();
            const NAType &tgtType = castNode->getValueId().getType();

            if ((hiveTextInsert()) &&
                (DFS2REC::isBinaryString(tgtType.getFSDatatype())) &&
                (NOT DFS2REC::isAnyCharacter(srcType.getFSDatatype())))
              {
                childNode = new(bindWA->wHeap()) 
                  ZZZBinderFunction(ITM_TO_CHAR, childNode);
                childNode = childNode->bindNode(bindWA);
                if (bindWA->errStatus())
                  return -1;            
              }

	    // When we have ins/upd target cols which are
	    // MP NCHAR in MX-NSK-Rel1 (i.e., SINGLE-byte),
	    // and the source was from a Tuple/TupleList,
	    // then we must do some magic Assign binding
	    // to ensure that the single-byte even-num-of-bytes
	    // "constraint" is not violated.
	    //
	    // Build + copy this "constraint" --
	    // NOTE:  tmpAssign MUST BE ON HEAP -- see TupleList::bindNode() !

	    Assign *tmpAssign = new(bindWA->wHeap())
	      Assign(castToList()[j].getItemExpr(), childNode);

            //***************************************************************
            // 10-0414-2428: Note that this assign is for inserts or updates
            // (1) castTo is set only when insert DMLs
            // (2) Assign constructor argument UserSpecified is set to TRUE
            //***************************************************************
            setInUpdateOrInsert(bindWA, NULL, REL_INSERT);
	    tmpAssign = (Assign *)tmpAssign->bindNode(bindWA);
            setInUpdateOrInsert(bindWA, NULL);
	    childNode = tmpAssign->getSource().getItemExpr();
            //don't allow LOB insert in a tuple list
            if (childNode->getOperatorType() == ITM_LOBINSERT)
              {                                                          
                // cannot have this function in a values list with
                // multiple tuples. Use a single tuple.
                *CmpCommon::diags() << DgSqlCode(-4483);
                GenExit();
                return -1;
                        
              }
            castNode->child(0) = childNode;
          }
          else
          {
            childNode = childNode->bindNode(bindWA);

            if ( (castNode->child(0)) &&
                (castNode->child(0)->getOperatorType() == ITM_INSTANTIATE_NULL) )
            {
              // if this tuplelist is part of a subquery an additional node
              // of type ITM_INSTANTIATE_NULL is placed in the binder; check if
              // that is the case
              castNode->child(0)->child(0) = childNode; // need to fix this
            }
            else
            {
              castNode->child(0) = childNode;
            }
          }

          castNode->bindNode(bindWA);

	  // if any unknown type in the tuple,
	  // coerce it to the target type.
	  childNode->bindNode(bindWA);
	  childNode->getValueId().coerceType(castNode->getValueId().getType());

	  MapInfo * mInfo = generator->getMapInfoAsIs(castNode->getValueId());
	  if (mInfo)
	    mInfo->resetCodeGenerated();
	  castNode->unmarkAsPreCodeGenned();
	  convVIDlist.insert(castNode->getValueId());
	}

      GenAssert(!bindWA->errStatus(), "bindWA");
      expGen->generateContiguousMoveExpr(convVIDlist,
					 0 /*don't add conv nodes*/,
					 0 /*atp*/,
					 tuppIndex,
					 generator->getInternalFormat(),
					 tupleLen,
					 &moveExpr,
					 &tupleDesc,
					 ExpTupleDesc::SHORT_FORMAT);
      qList->insert(moveExpr);
    }

  returnedDesc->setTupleDescriptor(tuppIndex, tupleDesc);

  // generate expression for selection predicate, if it exists
  ex_expr *predExpr = NULL;
  if(NOT selectionPred().isEmpty())
  {
    ItemExpr* pred = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
    expGen->generateExpr(pred->getValueId(),ex_expr::exp_SCAN_PRED,
             &predExpr);
  }

  // Compute the buffer size based on upqueue size and row size.
  // Try to get enough buffer space to hold twice as many records
  // as the up queue.
  ULng32 buffersize = getDefault(GEN_TUPL_BUFFER_SIZE);
  Int32 numBuffers = getDefault(GEN_TUPL_NUM_BUFFERS);
  queue_index upqueuelength = (queue_index)getDefault(GEN_TUPL_SIZE_UP);
  ULng32 cbuffersize =
    ((tupleLen + sizeof(tupp_descriptor))
     * (upqueuelength * 2/numBuffers)) +
    SqlBufferNeededSize(0,0);
  buffersize = buffersize > cbuffersize ? buffersize : cbuffersize;

  ComTdbTupleLeaf *tupleTdb = new(generator->getSpace())
    ComTdbTupleLeaf(qList,
		    tupleLen,
		    tuppIndex,
		    predExpr,
                    givenDesc,
		    returnedDesc,
		    (queue_index)getDefault(GEN_TUPL_SIZE_DOWN),
		    (queue_index)upqueuelength,
		    (Cardinality) (getInputCardinality() * getEstRowsUsed()).getValue(),
		    getDefault(GEN_TUPL_NUM_BUFFERS),
		    buffersize);
  generator->initTdbFields(tupleTdb);

  if(!generator->explainDisabled())
    {
      generator->setExplainTuple(
	   addExplainInfo(tupleTdb, 0, 0, generator));
    }

  generator->setGenObj(this, tupleTdb);
  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);

  return 0;
}

/////////////////////////////////////////////////////////
//
// ExplainFunc::codeGen()
//
/////////////////////////////////////////////////////////
short ExplainFunc::codeGen(Generator * generator)
{
  ExpGenerator * expGen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  // allocate a map table for the retrieved columns
  generator->appendAtEnd();

  ex_expr *explainExpr = 0;

  ex_cri_desc * givenDesc
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc
    = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);

  ex_cri_desc * paramsDesc
    = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);


  // Assumption (for now): retrievedCols contains ALL columns from
  // the table/index. This is because this operator does
  // not support projection of columns. Add all columns from this table
  // to the map table.
  //
  // The row retrieved from filesystem is returned as the last entry in
  // the returned atp.

  const ValueIdList & columnList = getTableDesc()->getColumnList();
  const CollIndex numColumns = columnList.entries();

  Attributes ** attrs = new(generator->wHeap()) Attributes * [numColumns];

  for (CollIndex i = 0; i < numColumns; i++)
    {
     ItemExpr * col_node = ((columnList[i]).getValueDesc())->getItemExpr();

      attrs[i] = (generator->addMapInfo(col_node->getValueId(), 0))->
	getAttr();
    }

  ExpTupleDesc *explTupleDesc = 0;
  ULng32 explTupleLength = 0;
  expGen->processAttributes(numColumns,
			    attrs, ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			    explTupleLength,
			    0, returnedDesc->noTuples() - 1,
			    &explTupleDesc, ExpTupleDesc::SHORT_FORMAT);

  // delete [] attrs;
  // NADELETEBASIC is used because compiler does not support delete[]
  // operator yet. Should be changed back later when compiler supports
  // it.
  NADELETEBASIC(attrs, generator->wHeap());

  // add this descriptor to the work cri descriptor.
  returnedDesc->setTupleDescriptor(returnedDesc->noTuples()-1, explTupleDesc);

  // generate explain selection expression, if present
  if (! selectionPred().isEmpty())
   {
     ItemExpr * newPredTree = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
     expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			   &explainExpr);
   }

  // generate move expression for the parameter list
  ex_expr *moveExpr = 0;

  ExpTupleDesc *tupleDesc = 0;
  ULng32 tupleLength = 0;
  if (! getProcInputParamsVids().isEmpty())
    {

      expGen->generateContiguousMoveExpr(getProcInputParamsVids(),
					 -1,
					 0,
					 paramsDesc->noTuples()-1,
					 ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
					 tupleLength,
					 &moveExpr,
					 &tupleDesc,
					 ExpTupleDesc::LONG_FORMAT);

      // add this descriptor to the work cri descriptor.
      paramsDesc->setTupleDescriptor(paramsDesc->noTuples()-1, tupleDesc);
    }

  // allocate buffer space to contain atleast 2 rows.
  ULng32 bufferSize = (explTupleLength+100/*padding*/) * 2/*rows*/;
  bufferSize = MAXOF(bufferSize, 30000); // min buf size 30000
  Int32 numBuffers = 3; // allocate 3 buffers

  ComTdbExplain *explainTdb
    = new(space)
      ComTdbExplain(givenDesc,	                 // given_cri_desc
		    returnedDesc,		 // returned cri desc
		    8,				 // Down queue size
		    16,				 // Up queue size0
		    returnedDesc->noTuples() - 1, // Index in atp of return
		    // tuple.
		    explainExpr,			 // predicate
		    paramsDesc,			 // Descriptor of params Atp
		    tupleLength,			 // Length of params Tuple
		    moveExpr,			 // expression to calculate
						 // the explain parameters
		    numBuffers,			 // Number of buffers to allocate
                    bufferSize);			 // Size of each buffer
  generator->initTdbFields(explainTdb);

  // Add the explain Information for this node to the EXPLAIN
  // Fragment.  Set the explainTuple pointer in the generator so
  // the parent of this node can get a handle on this explainTuple.
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(explainTdb, 0, 0, generator));
  }
  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);
  generator->setGenObj(this, explainTdb);

  return 0;
}

// PhysTranspose::codeGen() ------------------------------------------
// Generate code (a TDB node with corresponding Expr expressions) for
// the PhysTranspose node.  This node implements the TRANSPOSE operator.
//
// Parameters:
//
// Generator *generator
//    IN/OUT : A pointer to the generator object which contains the state,
//             and tools (e.g. expression generator) to generate code for
//             this node.
//
// Side Effects: Generates an ComTdbTranspose along with all the required
//               expressions for the transpose node.
//
//               Generates explain info.
//
//               Alters the state of the generator, including modifing
//               the map table, setting references to the generated Tdb
//               and explain info. etc.
//
short
PhysTranspose::codeGen(Generator *generator)
{
  // Get handles on expression generator, map table, and heap allocator
  //
  ExpGenerator *expGen = generator->getExpGenerator();
  Space *space = generator->getSpace();

  // Allocate a new map table for this operation
  //
  MapTable *localMapTable = generator->appendAtEnd();

  // Generate the child and capture the task definition block and a description
  // of the reply composite row layout and the explain information.
  //
  child(0)->codeGen(generator);

  ComTdb *childTdb = (ComTdb*)(generator->getGenObj());

  ex_cri_desc *childCriDesc = generator->getCriDesc(Generator::UP);

  ExplainTuple *childExplainTuple = generator->getExplainTuple();


  // Generate the given and returned composite row descriptors
  // Transpose adds a tupp to the row returned by the *child*
  // or the Given row, if no columns of the child are outputs of
  // this node.
  //
  ex_cri_desc *givenCriDesc = generator->getCriDesc(Generator::DOWN);


  // Determine if any of the childs outputs are also outputs of the node.
  // (if not, then don't pass them up the queue, later we will remove them
  // from the map table.)
  //

  // Get the outputs of this node.
  //
  ValueIdSet transOutputs = getGroupAttr()->getCharacteristicOutputs();

  // Get all the ValueIds that this node generates.
  //
  ValueIdSet transVals;

  for(CollIndex v = 0; v < transUnionVectorSize(); v++) {
    transVals.insertList(transUnionVector()[v]);
  }

  // Remove from the outputs, those values that are generated by this
  // node. (if none are left, then the child's outputs are not needed
  // above, otherwise they are.)
  //
  transOutputs -= transVals;

  ex_cri_desc *returnedCriDesc = 0;

  if (transOutputs.isEmpty()) {

    // The child's outputs are not needed above, so do not pass them
    // along.
    //
    returnedCriDesc =
      new(space) ex_cri_desc(givenCriDesc->noTuples() + 1, space);

    // Make all of my child's outputs map to ATP 1. Since they are
    // not needed above, they will not be in the work ATP (0).
    // (Later, they will be reomved from the map table)
    //
    localMapTable->setAllAtp(1);

  } else {

    // The child's outputs are needed above, so must pass them along.
    //
    returnedCriDesc =
      new(space) ex_cri_desc(childCriDesc->noTuples() + 1, space);
  }

  // transposeCols is the last Tp in Atp 0.
  //
  const Int32 transColsAtpIndex = returnedCriDesc->noTuples() - 1;
  const Int32 transColsAtp = 0;

  // The length of the new tuple which will contain the columns
  // generated by transpose.
  //
  ULng32 transColsTupleLen;

  // The Transpose node contains a vector of ValueIdLists. There is
  // one entry for each transpose set, plus one entry for the key
  // values. Each entry contains a list of ValueIdUnion Nodes. The
  // first entry contains a list with one ValueIdUnion node. This node
  // is for the Const. Values (1 - N) representing the Key Values. The
  // other entries contain lists of ValueIdUnion nodes for the
  // Transposed Values. Each of these entries of the vector represent
  // a transpose set.  If the transpose set contains a list of values,
  // then there will be only one ValueIdUnion node in the list.  If
  // the transpose set contains a list of lists of values, then there
  // will be as many ValueIdUnion nodes as there are items in the
  // sublists. Each ValueIdUnion within a list must contain the same
  // number of elements.
  //

  // The cardinality of an entry in the vector is the number of entries
  // in each of its ValueIdUnion nodes.  The cardinality of the first
  // entry (the key values) is equal to the sum of the cardinalities of
  // the subsequent entries (the transpose values).

  // In order to generate the expressions for the transpose node the
  // ValueIdUnion nodes for the values expressions (entries 1 - 3 below)
  // have to be  indexed from 0 - N (0 - 6 in the example below).
  // In order to translate from an index from 0 - N to an index of the
  // vector and an index within the ValueIdUnion, numExprs contains the
  // number of ValExprs for each ValueIdUnion. Eg. if the transUnionVector
  // contains these entries:
  //
  // Vector Num
  // Entry  Entries Card.
  //   0     1       7      ValueIdUnion(1,2,3,4,5,6,7)
  //   1     1       2      ValueIdUnion(A,B)
  //   2     1       3      ValueIdUnion(X,Y,Z)
  //   3     2       2      ValueIdUnion(1,2) , ValueIdUnion('hello', 'world')
  //
  // Then numUnionLists would be 4
  //
  // The numKeyExprs would be 7 (the cardinality of the Key Values, see below)
  //
  // The total cardinality of the rest of the ValueIdUnions also equals 7.
  //
  // numExprs will have 4 (numUnionLists) entries with values 7, 2, 3 and 2.
  //
  // The number of entries in the transUnionVector.
  //
  CollIndex numUnionLists = transUnionVectorSize();

  // Allocate a vector of size numUnionLists to hold the cardinality of each
  // entry in the vector.
  //
  CollIndex *numExprs = new(space) CollIndex[numUnionLists];

  // The Tuple Desc describing the tuple containing the new transpose columns
  // It is generated when the expressions are generated.
  //
  ExpTupleDesc *transColsTupleDesc = 0;

  // Loop index used throughout
  //
  CollIndex unionListNum;

  // Used below for a sanity check (assert).
  //
  CollIndex totNumExprs = 0;

  // Populate the numExprs array.
  //
  for(unionListNum = 0; unionListNum < numUnionLists; unionListNum++) {

    // If the entry of the vector has at least one ValueIdUnion ...
    // (All ValueIdUnions in an entry of the vector should have the
    // same number of enties, so use the first entry)
    //
    if(transUnionVector()[unionListNum].entries() > 0)
      numExprs[unionListNum] =
	((ValueIdUnion *)(transUnionVector()[unionListNum])[0].
	 getValueDesc()->getItemExpr())->entries();
    else
      // If the keyCol was not specified, then the first entry of the
      // vector may contain no ValueIdUnion nodes.
      //
      numExprs[unionListNum] = 0;

    // Used in an assert below.
    //
    totNumExprs += numExprs[unionListNum];
  }

  // The number of Key Exprs. The item expressions should be the Constants
  // 1 -> numKeyExprs or non-existant. The Key Exprs are the first entry.
  //
  const CollIndex numKeyExprs = numExprs[0];

  // If there are no key expressions, the the number of expressions
  // to generate is the totNumExprs.
  //
  const CollIndex numMovExprs = (numKeyExprs == 0 ? totNumExprs : numKeyExprs);

  // The total number of item expressions in the value unions should equal
  // the number of key values in the first ValueIdUnion.
  //
  GenAssert(totNumExprs == numKeyExprs * 2 || numKeyExprs == 0,
	    "Transpose: Internal Error");

  // Allocate space to hold numMovExprs expressions. Each expression will
  // compute one value for each of the ValueIdUnions.
  // A constant Key Value will be computed in each expression,
  // but only value expressions from one vector entry will be computed.
  // All other value expressions will be NULL.
  //
  ex_expr ** transExprs = new(space)ex_expr*[numMovExprs];

  // A list of the ValueIds will be constructed and used to generate
  // the expressions
  //
  ValueIdList ValueIds;

  // A new item expression which casts the result to the proper type
  // will be constructed.
  //
  Cast *castExpr = 0;

  // Loop index
  //
  CollIndex exprNum;

  // For each transpose value expression (also equal to the number of key
  // values if they exist)
  //
  for(exprNum = 0; exprNum < numMovExprs; exprNum++) {

    // Clear the list of value Ids (used in the previous iteration
    // through this loop).
    //
    ValueIds.clear();

    // Contruct Key Expression with Cast node if they exist.
    // The key ValueIdUnion is in position 0.
    //
    if(numKeyExprs != 0) {

      // Get the key ValueIdUnion.
      //
      ValueIdUnion *keyValIdUnion =
	(ValueIdUnion *)((transUnionVector()[0])[0].
			 getValueDesc()->getItemExpr());

      // Extract one (exprNum) entry from the ValueIdUnion and add a
      // Cast to cast it to the proper type.
      //
      castExpr = new(generator->wHeap())
	Cast(keyValIdUnion->getSource(exprNum).getValueDesc()->getItemExpr(),
	     &(keyValIdUnion->getResult().getType()));

      // Bind this new item expression.
      //
      castExpr->bindNode(generator->getBindWA());

      // This should never fail at this point !!!
      //
      GenAssert(! generator->getBindWA()->errStatus(),
		"Transpose: Internal Error");

      // Insert the Value Id for this item expression into the list of
      // Value Ids that will be used to generate the final expression.
      //
      ValueIds.insert(castExpr->getValueId());
    }

    // ValueIds may now contains one ValueId representing the key value.
    //

    // Translate the expression number (exprNum) from 0 -> numKeyExprs - 1
    // to vector entry number (vectorEntryNum) and expression number
    // (valExprNum)
    //

    // Binary expression indexes.
    //
    CollIndex vectorEntryNum, valExprNum;

    // Keep track of how many expressions are covered by the current
    // ValueIdUnion and those previous.
    //
    CollIndex numCoveredExprs;

    // Translate the Unary expression index (exprNum) into a binary
    // index (vectorEntryNum, valExprNum)
    // (A for-loop with no body)
    //
    for((vectorEntryNum = 1,
	 numCoveredExprs = numExprs[1],
	 valExprNum = exprNum);

	// Does this ValueIdUnion cover this expression number
	//
	numCoveredExprs <= exprNum;

	// If not, adjust the indexes and try the next one.
	(numCoveredExprs += numExprs[vectorEntryNum + 1],
	 valExprNum -= numExprs[vectorEntryNum],
	 vectorEntryNum++));

    // At this point:
    // vectorEntryNum is index of the transUnionVector for this exprNum.
    // valExprNum is index into the ValueIdUnion.

    GenAssert(vectorEntryNum > 0 && vectorEntryNum < numUnionLists,
	      "Transpose: Internal Error");

    GenAssert(valExprNum < numExprs[vectorEntryNum],
	      "Transpose: Internal Error");

    // Generate all Value Expressions.
    // One will be all of the value expressions indexed by vectorEntryNum
    // and valExprNum and the others will be NULL.
    //
    for(unionListNum = 1; unionListNum < numUnionLists; unionListNum++) {

      // For Each ValueIdUnion in an entry of the vector.
      //
      for(CollIndex numValUnions = 0;
	  numValUnions < transUnionVector()[unionListNum].entries();
	  numValUnions++) {

	ValueIdUnion *valValIdUnion =
	  (ValueIdUnion *)((transUnionVector()[unionListNum])[numValUnions].
			   getValueDesc()->getItemExpr());

	if(unionListNum == vectorEntryNum){

	  // Construct the value expression cast to the proper type.
	  //
	  castExpr = new(generator->wHeap())
	    Cast(valValIdUnion->
		 getSource(valExprNum).getValueDesc()->getItemExpr(),
		 &(valValIdUnion->getResult().getType()));
	} else {

	  // Construct NULL expression cast to the proper type.
	  //
	  castExpr = new(generator->wHeap())
	    Cast(new (space) ConstValue(),
		 &(valValIdUnion->getResult().getType()));
	}

	// Bind the CASTed itemed expression.
	//
	castExpr->bindNode(generator->getBindWA());

	// This should never fail at this point !!!
	//
	GenAssert(! generator->getBindWA()->errStatus(),
		  "Transpose: Internal Error");

	// Add the ValueId to the list of value ids which will be used
	// to generate the final expression.
	//
	ValueIds.insert(castExpr->getValueId());
      }
    }

    // Generate the expression.

    // Initialize the pointer to the expression to be generated
    // to be NULL.
    //
    transExprs[exprNum] = 0;

    // Generate the expressions.
    //
    // ValueIds - refers to the expressions for 1 key value, and N
    // transpose values, all but one will be NULL.
    //
    // 0 - Do not add conv. nodes.
    //
    // transColsAtp - this expression will be evaluated on the
    // transColsAtp (0) ATP.
    //
    // transColsAtpIndex - within the transColsAtp (0) ATP, the destination
    // for this move expression will be the transColsAtpIndex TP. This should
    // be the last TP of the ATP.
    //
    // SQLARK_EXPLODED_FORMAT - generate the move expression to construct
    // the destination tuple in EXPLODED FORMAT.
    //
    // transColsTupleLen - This is an output which will contain the length
    // of the destination Tuple.  This better return the same value for each
    // interation of this loop.
    //
    // &transExprs[exprNum] - The address of the pointer to the expression
    // which will be generated.
    //
    // &transColsTupleDesc - The address of the tuple descriptor which is
    // generated.  This describes the destination tuple of the move expression.
    // This will be generated only the first time through the loop. The tuple
    // better have the same format each time. A NULL value inticates that
    // the descriptor should not be generated.
    //
    // SHORT_FORMAT - generate the transColsTupleDesc in the SHORT FORMAT.
    //
    expGen->generateContiguousMoveExpr(ValueIds,
				       0,
				       transColsAtp,
				       transColsAtpIndex,
				       generator->getInternalFormat(),
				       transColsTupleLen,
				       &transExprs[exprNum],
				       (exprNum == 0
					? &transColsTupleDesc
					: 0),
				       ExpTupleDesc::SHORT_FORMAT);
    // Set the tuple descriptor in the returned CRI descriptor.
    // This will be set only the first time through the loop.
    //
    if(exprNum == 0) {
      returnedCriDesc->setTupleDescriptor(transColsAtpIndex,
					  transColsTupleDesc);

      // Take the result (ie. the Cast nodes) of generateContiguousMove
      // and set the mapInfo for the ValueIdUnion nodes to be the same so
      // that our parent can access the computed expressions
      // This code is executed only the first time through the loop.
      // The info generated from the expression generator should be the
      // same each time through the loop.
      //

      // An index into the ValueIds that we just generated.
      // (It is assumed that the above code inserts the value Ids into
      // ValueIds in the same order as the loops below).
      //
      CollIndex valIdNum = 0;

      // For each entry in the transUnionVector...
      //
      for(unionListNum = 0; unionListNum < numUnionLists; unionListNum++) {

	// For each ValueIdUnion in the entry...
	//
	for(CollIndex numValUnions = 0;
	    numValUnions < transUnionVector()[unionListNum].entries();
	    numValUnions++) {

	  // Get the ValueId of the ValueIdUnion node.
	  //
	  ValueId valId =
	    (transUnionVector()[unionListNum])[numValUnions];

	  // Add a map entry to the map table for this ValueIdUnion node.
	  // The mapInfo is the same as the mapInfo of the result of
	  // generating an expression for the corresponding Cast expression.
	  //
	  MapInfo * mapInfo =
	    generator->addMapInfoToThis(localMapTable,
                valId,
                generator->getMapInfo(ValueIds[valIdNum])->getAttr());

	  // Indicate that code was generated for this map table entry.
	  //
	  mapInfo->codeGenerated();

	  // Advance the index to the ValueIds.
	  //
	  valIdNum++;
	}
      }
    }

    //localMapTable->removeLast();
    generator->removeLast();
  }

  // Generate expression to evaluate predicate on the transposed row
  //
  ex_expr *afterTransPred = 0;

  if (! selectionPred().isEmpty()) {
    ItemExpr * newPredTree =
      selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);

    expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			 &afterTransPred);
  }

  // Allocate the Transpose TDB
  //
  queue_index upQ = (queue_index)getDefault(GEN_TRSP_SIZE_UP);

  ComTdbTranspose *transTdb =
    new (space) ComTdbTranspose(childTdb,
				transExprs,
				numMovExprs,
				afterTransPred,
				transColsTupleLen,
				transColsAtpIndex,
				givenCriDesc,
				returnedCriDesc,
				(queue_index)getDefault(GEN_TRSP_SIZE_DOWN),
				//(queue_index)getDefault(GEN_TRSP_SIZE_UP),
				upQ,
				(Cardinality) (getInputCardinality() * getEstRowsUsed()).getValue(),
				getDefault(GEN_TRSP_NUM_BUFFERS),
				getDefault(GEN_TRSP_BUFFER_SIZE),
                                space);
  generator->initTdbFields(transTdb);
  // If the child's outputs are not needed above this node,
  // remove the entries from the map table.
  //
  if (transOutputs.isEmpty()) {

    // Remove child's outputs from mapTable, They are not needed
    // above.
    //
    generator->removeAll(localMapTable);
    NADELETEARRAY(numExprs,numUnionLists,CollIndex,space);
    numExprs = NULL;

  }


  // Add the explain Information for this node to the EXPLAIN
  // Fragment.  Set the explainTuple pointer in the generator so
  // the parent of this node can get a handle on this explainTuple.
  //
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(addExplainInfo(transTdb,
					      childExplainTuple,
					      0,
					      generator));
  }

  // Restore the Cri Desc's and set the return object.
  //
  generator->setCriDesc(givenCriDesc, Generator::DOWN);
  generator->setCriDesc(returnedCriDesc, Generator::UP);
  generator->setGenObj(this, transTdb);

  return 0;
}

// -----------------------------------------------------------------------
// PhyPack::codeGen()
// -----------------------------------------------------------------------
short PhyPack::codeGen(Generator* generator)
{
  // Get handles on expression generator, map table, and heap allocator.
  ExpGenerator* expGen = generator->getExpGenerator();
  Space* space = generator->getSpace();

  // Allocate a new map table for this operation.
  MapTable* localMapTable =
    generator->appendAtEnd();

  // Get a description of what kind of tuple my parent gives me.
  ex_cri_desc* givenCriDesc = generator->getCriDesc(Generator::DOWN);

  // PhyPack adds one tupp to what its parent gives in its return tuple.
  unsigned short returnedNoOfTuples = givenCriDesc->noTuples() + 1;
  ex_cri_desc* returnedCriDesc =
                        new (space) ex_cri_desc(returnedNoOfTuples,space);

  // Get a handle of the last map table before allowing child to code gen.
  MapTable* lastMapTable = generator->getLastMapTable();

  // Create new attributes for the packed columns.
  CollIndex noOfPackedCols = packingExpr().entries();
  Attributes** attrs =
                   new (generator->wHeap()) Attributes * [noOfPackedCols];

  // Add the packed columns to the map table. (using ATP0)
  for(CollIndex i = 0; i < noOfPackedCols; i++)
      attrs[i] = (generator->addMapInfo(packingExpr().at(i),0))->getAttr();

  // PhyPack adds one tupp to the tail of its parent's ATP (stored as ATP0)
  const Int32 atpIndex = returnedNoOfTuples - 1;
  const Int32 atp = 0;

  // To store length of the last tupp introduced by PhyPack.
  ULng32 tupleLen = 0;

  // Will be generated to describe the last tupp introduced by PhyPack.
  ExpTupleDesc* tupleDesc = 0;

  // From here up we go back into Exploded Internal format (if not there
  // already) since it doesn't make sense for column wise rowsets to
  // be in Compressed Internal format yet.
  generator->setExplodedInternalFormat();

  // Fill in offsets and other info in the attributes list and computes the
  // length and descriptor of the resulting tuple generated.
  // The format of packed rows in rowsets is always Exploded Internal format.
  expGen->processAttributes(noOfPackedCols,
                            attrs,
                            ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
                            tupleLen,
                            atp,
                            atpIndex,
                            &tupleDesc,
                            ExpTupleDesc::SHORT_FORMAT);

  // Copies of Attributes have been made to store in the tuple descriptor.
  NADELETEBASIC(attrs,generator->wHeap());

  // Store the computed tuple desc for the new tuple added by PhyPack.
  returnedCriDesc->setTupleDescriptor(returnedNoOfTuples - 1,tupleDesc);

  // Set the DOWN descriptor in the generator to what my child gets from me.
  generator->setCriDesc(returnedCriDesc,Generator::DOWN);

  // Get the child code generated and retrieve useful stuffs from it.
  child(0)->codeGen(generator);
  ComTdb* childTdb = (ComTdb*)(generator->getGenObj());

  // Generate an expression to pack the rows.
  ex_expr* packExpr = 0;
  expGen->generateListExpr(packingExpr(),ex_expr::exp_ARITH_EXPR,&packExpr);

  // Generate selection predicate on the packed cols if there are any.
  ex_expr* predExpr = 0;
  if(NOT selectionPred().isEmpty())
  {
    ItemExpr* pred = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
    expGen->generateExpr(pred->getValueId(),ex_expr::exp_SCAN_PRED,
                         &predExpr);
  }

  //????????? Generate an expression to copy result row to result buffer.
  // ex_expr* copyExpr = 0;
  // expGen->generateContiguousMoveExpr(packingExpr(),
  //                                 -1,                       // add conv
  //                                 1,                        // copy to ATP1
  //                                 returnedNoOfTuples - 1,   // ATP index
  //                                 ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
  //                                 tupleLen,
  //                                 &copyExpr);
  //
  // Note that my parent is gonna find the non-copied version of attributes
  // information in my map table, since it is refering to the vid's of the
  // originals but not the copies. Coincidentally this is good for us since:
  // 1. My parent is expected to see things at ATP0 rather than ATP1.
  // 2. The format (or order) of the tuple doesn't get changed after copying
  //    so that the old attributes are refering to the right offsets.
  //
  // $$$ Seems copyExpr_ is no longer needed after ComTdbPackRows code were written.

  // Values at the child's context as well as the intermediates during expr
  // generation are no longer visible any more from this point.
  generator->removeAll(lastMapTable);

  // Create the Pack node's TDB.
  ComTdbPackRows* packTdb = new (space) ComTdbPackRows (childTdb,
                                              packExpr,
                                              predExpr,
                                              returnedNoOfTuples - 1,
                                              tupleLen,
                                              givenCriDesc,
                                              returnedCriDesc,
                                              (queue_index) 8,
                                              (queue_index) 8);
  generator->initTdbFields(packTdb);

  // Add explain info of this node to the EXPLAIN fragment. Set explainTuple
  // pointer in the generator so the parent of this node can get a handle on
  // this explainTuple.
  //
  if(NOT generator->explainDisabled())
  {
    // Child's explain tuple.
    ExplainTuple* childExplainTuple = generator->getExplainTuple();

    generator->setExplainTuple(addExplainInfo(packTdb,
                                              childExplainTuple,
                                              0,
                                              generator));
  }

  // Set up the new up CRI desc.
  generator->setCriDesc(returnedCriDesc,Generator::UP);

  // Restore original down CRI desc since this node changed it.
  generator->setCriDesc(givenCriDesc,Generator::DOWN);

  // Return generated object (the TDB) to the generator.
  generator->setGenObj(this, packTdb);

  return 0;

}

// -----------------------------------------------------------------------
// TableValuedFunction methods
// -----------------------------------------------------------------------
const char * TableValuedFunction::getVirtualTableName()
{ return "??TableValuedFunction??"; }

TrafDesc *TableValuedFunction::createVirtualTableDesc()
{ return NULL; }

void TableValuedFunction::deleteVirtualTableDesc(TrafDesc *vtd)
{
}

// -----------------------------------------------------------------------
// StatisticsFunc methods
// -----------------------------------------------------------------------
const char * StatisticsFunc::getVirtualTableName()
//{ return "STATISTICS__"; }
{return getVirtualTableNameStr();}

TrafDesc *StatisticsFunc::createVirtualTableDesc()
{
  TrafDesc * table_desc =
    Generator::createVirtualTableDesc(getVirtualTableName(),
				      NULL, // let it decide what heap to use    
				      ComTdbStats::getVirtTableNumCols(),
				      ComTdbStats::getVirtTableColumnInfo(),
				      ComTdbStats::getVirtTableNumKeys(),
				      ComTdbStats::getVirtTableKeyInfo());
  return table_desc;
}


short StatisticsFunc::codeGen(Generator* generator)
{
  ExpGenerator * expGen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  // allocate a map table for the retrieved columns
  generator->appendAtEnd();

  ex_expr *scanExpr = 0;
  ex_expr *projExpr = 0;

  ex_cri_desc * givenDesc
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc
    = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);

  // cri descriptor for work atp has 4 entries:
  // first two entries for consts and temps.
  // Entry 1(index #2) is
  // where the stats row will be moved to evaluate
  // the 'where' predicate.
  // Entry 2(index #3) is where the input row will be built.

  ex_cri_desc * workCriDesc = new(space) ex_cri_desc(4, space);
  const Int32 work_atp = 1;
  const Int32 stats_row_atp_index = 2;
  const Int32 input_row_atp_index = 3;

  // Assumption (for now): retrievedCols contains ALL columns from
  // the table/index. This is because this operator does
  // not support projection of columns. Add all columns from this table
  // to the map table.
  //
  // The row retrieved from filesystem is returned as the last entry in
  // the returned atp.

  Attributes ** attrs =
    new(generator->wHeap())
    Attributes * [getTableDesc()->getColumnList().entries()];

  for (CollIndex i = 0; i < getTableDesc()->getColumnList().entries(); i++)
    {
      ItemExpr * col_node
	= (((getTableDesc()->getColumnList())[i]).getValueDesc())->
	  getItemExpr();

      attrs[i] = (generator->addMapInfo(col_node->getValueId(), 0))->
	getAttr();
    }

  ExpTupleDesc *tupleDesc = 0;
  ULng32 tupleLength = 0;

  // StatisticsFunc must use Exploded Format for now.
  ExpTupleDesc::TupleDataFormat tupleFormat = ExpTupleDesc::SQLARK_EXPLODED_FORMAT;

  expGen->processAttributes(getTableDesc()->getColumnList().entries(),
			    attrs, tupleFormat,
			    tupleLength,
			    work_atp, stats_row_atp_index,
			    &tupleDesc, ExpTupleDesc::LONG_FORMAT);

  // delete [] attrs;
  // NADELETEBASIC is used because compiler does not support delete[]
  // operator yet. Should be changed back later when compiler supports
  // it.
  NADELETEBASIC(attrs, generator->wHeap());

  // add this descriptor to the work cri descriptor.
  workCriDesc->setTupleDescriptor(stats_row_atp_index, tupleDesc);

  // generate stats selection expression, if present
  if (! selectionPred().isEmpty())
   {
     ItemExpr * newPredTree = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
     expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			  &scanExpr);
   }

  // generate move expression for the parameter list
  ex_expr *inputExpr = 0;
  ExpTupleDesc *inputTupleDesc = 0;
  ULng32 inputTupleLength = 0;

  if (! getProcInputParamsVids().isEmpty())
    {

      expGen->generateContiguousMoveExpr(getProcInputParamsVids(),
					 -1,
					 work_atp,
					 input_row_atp_index,
					 tupleFormat,
					 inputTupleLength,
					 &inputExpr,
					 &inputTupleDesc,
					 ExpTupleDesc::LONG_FORMAT);

      // add this descriptor to the work cri descriptor.
      workCriDesc->setTupleDescriptor(input_row_atp_index, inputTupleDesc);
    }

  // The stats row will be returned as the last entry of the returned atp.
  // Change the atp and atpindex of the returned values to indicate that.
  expGen->assignAtpAndAtpIndex(getTableDesc()->getColumnList(),
			       0, returnedDesc->noTuples()-1);

  Lng32 numBuffers = 3;
  Lng32 bufferSize = 10 * tupleLength;
  ComTdbStats *statsTdb = new(space) ComTdbStats
    (
	 tupleLength,			 // Length of stats Tuple
	 tupleLength,                    // Length of returned tuple
	 inputTupleLength,               // length of input tuple
	 givenDesc,	                 // given_cri_desc
	 returnedDesc,		         // returned cri desc
	 8,				 // Down queue size
	 16,				 // Up queue size
	 numBuffers,			 // Number of buffers to
					 // allocate
	 bufferSize,			 // Size of each buffer
	 scanExpr,			 // predicate
	 inputExpr,
	 projExpr,
	 workCriDesc,			 // Descriptor of work Atp
	 stats_row_atp_index,
	 input_row_atp_index);

  generator->initTdbFields(statsTdb);

  // Add the explain Information for this node to the EXPLAIN
  // Fragment.  Set the explainTuple pointer in the generator so
  // the parent of this node can get a handle on this explainTuple.
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(statsTdb, 0, 0, generator));
  }
  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);
  generator->setGenObj(this, statsTdb);

  return 0;
}

// -----------------------------------------------------------------------
// ProxyFunc methods
// -----------------------------------------------------------------------

static void initProxyKeyDescStruct(TrafKeysDesc *tgt, ComUInt32& src)
{
  tgt->keyseqnumber = 1;
  tgt->tablecolnumber = 0;
  tgt->setDescending(FALSE);
}

static Lng32 createDescStructsForProxy(const ProxyFunc &proxy,
                                        char *tableName,
                                        TrafDesc *&colDescs,
                                        TrafDesc *&keyDescs)
{
  colDescs = NULL;
  keyDescs = NULL;
  Lng32 reclen = 0;
  TrafDesc *prev_desc = NULL;
  ComUInt32 numCols = proxy.getNumColumns();

  // Creates and populates column descs
  proxy.populateColumnDesc(tableName, colDescs, reclen);

  // Create key descs
  prev_desc = NULL;
  numCols = 1; 
  for (ComUInt32 keyNum = 0; keyNum < numCols; keyNum++)
  {
    TrafDesc *key_desc = TrafAllocateDDLdesc(DESC_KEYS_TYPE, NULL);
    if (prev_desc)
      prev_desc->next = key_desc;
    else
      keyDescs = key_desc;      
    
    prev_desc = key_desc;
    
    initProxyKeyDescStruct(key_desc->keysDesc(), keyNum);
  }
  
  return reclen;
}

TrafDesc *ProxyFunc::createVirtualTableDesc()
{
  // TrafAllocateDDLdesc() requires that HEAP (STMTHEAP) 
  // be used for operator new herein

  TrafDesc *table_desc = TrafAllocateDDLdesc(DESC_TABLE_TYPE, NULL);
  const char *tableName = getVirtualTableName();
  table_desc->tableDesc()->tablename = new HEAP char[strlen(tableName)+1];
  strcpy(table_desc->tableDesc()->tablename, tableName);

  table_desc->tableDesc()->setSystemTableCode(TRUE);

  TrafDesc *files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, NULL);
  files_desc->filesDesc()->setAudited(TRUE); // audited table
  table_desc->tableDesc()->files_desc = files_desc;
  
  TrafDesc *cols_descs = NULL;
  TrafDesc *keys_descs = NULL;

  table_desc->tableDesc()->colcount = (Int32) getNumColumns();

  table_desc->tableDesc()->record_length =
    createDescStructsForProxy(*this,
                                table_desc->tableDesc()->tablename,
                                cols_descs,
                                keys_descs);
  
  TrafDesc *index_desc = TrafAllocateDDLdesc(DESC_INDEXES_TYPE, NULL);
  index_desc->indexesDesc()->tablename =
    table_desc->tableDesc()->tablename;
  index_desc->indexesDesc()->indexname =
    table_desc->tableDesc()->tablename;
  index_desc->indexesDesc()->keytag = 0; // primary index
  index_desc->indexesDesc()->record_length =
    table_desc->tableDesc()->record_length;
  index_desc->indexesDesc()->colcount =
    table_desc->tableDesc()->colcount;
  index_desc->indexesDesc()->blocksize = 4096; // doesn't matter.

  TrafDesc *i_files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, NULL);
  i_files_desc->filesDesc()->setAudited(TRUE); // audited table
  index_desc->indexesDesc()->files_desc = i_files_desc;

  index_desc->indexesDesc()->keys_desc  = keys_descs;
  table_desc->tableDesc()->columns_desc = cols_descs;
  table_desc->tableDesc()->indexes_desc = index_desc;
 
  return table_desc;
}

short PhysicalExtractSource::codeGen(Generator *)
{
  GenAssert(0, "PhysicalExtractSource::codeGen() should never be called");
  return 0;
}

/////////////////////////////////////////////////////////
//
// ControlRunningQuery::codeGen()
//
/////////////////////////////////////////////////////////
short ControlRunningQuery::codeGen(Generator * generator)
{
  Space * space = generator->getSpace();

  GenAssert((child(0) == NULL) && (child(1) == NULL),
    "ControlRunningQuery does not expect any child.");

  char *qid =
    space->allocateAndCopyToAlignedSpace(queryId_, str_len(queryId_), 0);

  char *pname =
     space->allocateAndCopyToAlignedSpace(pname_, str_len(pname_), 0);

  char *comment =
    space->allocateAndCopyToAlignedSpace(comment_, str_len(comment_), 0);

  ComTdbCancel::Action a = ComTdbCancel::InvalidAction;
  switch (action_)
  {
    case Cancel:
    {
      switch (qs_)
      {
        case ControlQid:    
          a = ComTdbCancel::CancelByQid;    
          break;
        case ControlPname:  
        case ControlNidPid:
          break;
        default: 
          GenAssert(0, "Invalid ControlRunningQuery::qs_"); break;
      }
      break;
    }
   case Suspend: 
   case Activate:
     break;
   default: GenAssert(0, "Invalid ControlRunningQuery::action_");
  }

  if (a == ComTdbCancel::InvalidAction)
  {
    *CmpCommon::diags() << DgSqlCode(-1010);
    GenExit();
    return -1;
  }

  ComTdbCancel * exe_cancel_tdb = new(space)
    ComTdbCancel(qid, pname, nid_, pid_,
                 getDefault(CANCEL_MINIMUM_BLOCKING_INTERVAL),
                 a,  forced_, comment,
                 generator->getCriDesc(Generator::DOWN),
                 generator->getCriDesc(Generator::DOWN),
                 (queue_index)getDefault(GEN_DDL_SIZE_DOWN),
                 (queue_index)getDefault(GEN_DDL_SIZE_UP)
                );

  generator->initTdbFields(exe_cancel_tdb);

  // no tupps are returned
  generator->setCriDesc((ex_cri_desc *)
    (generator->getCriDesc(Generator::DOWN)),
                           Generator::UP);
  generator->setGenObj(this, exe_cancel_tdb);

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(exe_cancel_tdb, 0, 0, generator));
  }

  return 0;
}

