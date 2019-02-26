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
 * File:         Generator.cpp
 * Description:  Methods which are used by the high level generator.
 *
 * Created:      4/15/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */
#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include "dfs2rec.h"

#include "TrafDDLdesc.h"

#include "ComOptIncludes.h"
#include "GroupAttr.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "ComSysUtils.h"
#include "ExplainTuple.h"
#include "BindWA.h"
#include "SchemaDB.h"
#include "ComTransInfo.h"
#include "CmpContext.h"
#include "CmpStatement.h"
#include "CmpSqlSession.h"
#include "ControlDB.h"
#include "RelMisc.h"
#include "RelExeUtil.h"
#include "ComCextdecs.h"

#include "logmxevent.h"

#include "ComTdb.h"
#include "LateBindInfo.h"

#include "exp_tuple_desc.h"
#include "exp_function.h"
#include "ComSqlId.h"

#define   SQLPARSERGLOBALS_FLAGS
#include "SqlParserGlobals.h"   // Parser Flags
#include "ComTrace.h"
#include "ComDistribution.h"
#include "CmUtil.h"

#include "PCodeExprCache.h"
#include "TrafDDLdesc.h"

// -----------------------------------------------------------------------
// When called within arkcmp.exe, fixupVTblPtr() translates to a call
// to fix up the TDB's to the Compiler version. This function is also
// defined in ExComTdb.cpp. That code, however, fixes up the TDB's to
// the Executor version.
// -----------------------------------------------------------------------

// To prevent redefinition problem on platforms where the executor is (still)
// directly linked to arkcmp.exe.



///////////////////////////////////////////////////
// class Generator
//////////////////////////////////////////////////
Generator::Generator(CmpContext* currentCmpContext) :
    currentCmpContext_(currentCmpContext)
    ,objectUids_(wHeap(), 1)
    ,objectNames_(wHeap(),0)
    ,snapshotScanTmpLocation_(NULL)
    ,baseFileDescs_(wHeap())       
    ,baseStoiList_(wHeap())   
    ,numOfVpsPerBase_(wHeap())              
    ,vpFileDescs_(wHeap())       
    ,lateNameInfoList_(wHeap())  
    ,genOperSimInfoList_(wHeap())  
    ,stoiList_(wHeap())  
    ,insertNodesList_(wHeap())  
    ,avgVarCharSizeList_(wHeap())  
    ,trafSimTableInfoList_(wHeap())
    ,bmoQuotaMap_(wHeap())
{
  // nothing generated yet.
  genObj = 0;
  genObjLength = 0;

  // no up or down descriptors.
  up_cri_desc = 0;
  down_cri_desc = 0;

  // fragment directory and resource entries for ESPs
  fragmentDir_      = new (wHeap()) FragmentDir(wHeap());

  firstMapTable_ = NULL;
  lastMapTable_  = NULL;

  tableId_     = 0;
  tempTableId_ = 0;
  tdbId_       = 0;
  pertableStatsTdbId_ = 0;

  bindWA = 0;

  flags_ = 0;
  flags2_ = 0;

  imUpdateRel_ = NULL;
  imUpdateTdb_ = NULL;

  updateCurrentOfRel_ = NULL;

  explainIsDisabled_ = FALSE;

  affinityValueUsed_ = 0;

  dynQueueSizeValuesAreValid_ = FALSE;

  orderRequired_ = FALSE;

  foundAnUpdate_ = FALSE;

  nonCacheableMVQRplan_ = FALSE;

  nonCacheableCSEPlan_ = FALSE;

  updateWithinCS_ = FALSE;

  isInternalRefreshStatement_ = FALSE;

  savedGenLeanExpr_ = FALSE;

  lruOperation_ = FALSE;

  queryUsesSM_ = FALSE;

  genSMTag_ = 0;

  tempSpace_ = NULL;

  numBMOs_ = 0;

  totalBMOsMemoryPerNode_ = 0;

  BMOsMemoryLimitPerNode_ = 0;

  totalNumBMOs_ = 0;

  numESPs_ = 1;

  totalNumESPs_ = 0;

  espLevel_ = 0;

  halloweenProtection_ = NOT_SELF_REF;
  collectRtsStats_ = TRUE;

  makeOnljRightQueuesBig_ = FALSE;
  makeOnljLeftQueuesBig_ = FALSE;
  onljLeftUpQueue_ = 0;
  onljLeftDownQueue_ = 0;
  onljRightSideUpQueue_ = 0;
  onljRightSideDownQueue_ = 0;

  // Used to specify queue size to use on RHS of flow or nested join.
  // Initialized to 0 (meaning don't use).  Set to a value by SplitTop
  // when on RHS of flow and there is a large degree of fanout. In this
  // situation, large queues are required above the split top (between
  // Split Top and Flow/NestedJoin).  Also used to set size of queues
  // below a SplitTop on RHS, but these do not need to be as large.
  largeQueueSize_ = 0;

  totalEstimatedMemory_ = 0.0;
  operEstimatedMemory_ = 0;

  maxCpuUsage_ = 0;

  // Exploded format is the default data format.
  // This can be changed via the CQD COMPRESSED_INTERNAL_FORMAT 'ON'
  setExplodedInternalFormat();
  overflowMode_ = ComTdb::OFM_DISK;
  // By default, set the query to be reclaimed
  setCantReclaimQuery(FALSE);

  avgVarCharSizeList_.clear();

  tupleFlowLeftChildAttrs_ = NULL;
  //avgVarCharSizeValList_.clear();
  initNNodes();

  currentEspFragmentPCG_ = NULL;
  planExpirationTimestamp_ = -1;

  // Initialize the NExDbgInfoObj_ object within the Generator object.
  //
  NExDbgInfoObj_.setNExDbgLvl( getDefaultAsLong(PCODE_NE_DBG_LEVEL) );
  NExDbgInfoObj_.setNExStmtSrc( NULL );
  NExDbgInfoObj_.setNExStmtPrinted( FALSE );

  NExLogPathNam_[0] = '\0' ;

  //
  // If the PCode Expression Cache debugging CQDs are set up,
  // we set the debugging-enabled flag in the current PCode
  // Expression Cache object for the current Context and
  // save the pathname for the *directory* we will use to
  // leave debugging log files.
  // NOTE: The same directory is used for debugging log
  // files for debugging the Native Expressions feature
  // but the names of the log files have a different prefix.
  // Also, the PCODE_DEBUG_LOGDIR cqd is shared by the two
  // debugging capabilities, so if that cqd is set up, then
  // we also save the pathname of the Native Expressions
  // debugging log file at this time.
  //
  Int32 PCEC_Dbg = getDefaultAsLong(PCODE_EXPR_CACHE_DEBUG) ;
  CURROPTPCODECACHE->setPCECLoggingEnabled( PCEC_Dbg );

  NAString PCDLogDir ;
  CmpCommon::getDefault(PCODE_DEBUG_LOGDIR, PCDLogDir, FALSE);
  Int32 logDirLen = PCDLogDir.length() ;

  //
  // If a PCode Debug Log Directory has already been specified
  // then do nothing.  There is no known reason for changing
  // log directories in the middle of a session and, furthermore,
  // if we allowed it, only one context would see the new value
  // for the CQD, so the overall results would probably not be
  // what the user (developer) was hoping for.
  //
  if ( logDirLen > 0 && CURROPTPCODECACHE->getPCDlogDirPath() == NULL )
  {
     CURROPTPCODECACHE->setPCDlogDirPath( &PCDLogDir );
  }

#define MAX_UNIQ_PART (8+4+16)
  if ( logDirLen < (sizeof(NExLogPathNam_) - MAX_UNIQ_PART - 1 ) )
  {
    strncpy( NExLogPathNam_ , PCDLogDir.data(), logDirLen );

    // Add a unique value to end of PCODE_DEBUG_LOGDIR name
    sprintf( &NExLogPathNam_[logDirLen], "/NELOG.%x.%lx"
           , CURROPTPCODECACHE->getUniqFileNamePid()
           , CURROPTPCODECACHE->getUniqFileNameTime() );

    NExDbgInfoObj_.setNExLogPath( &NExLogPathNam_[0] );
  }

  // Initialize other member variables.
  //
  computeStats_ = FALSE;
  explainInRms_ = TRUE;
  topNRows_ = 0;
}

void Generator::initTdbFields(ComTdb *tdb)
{
  if (!dynQueueSizeValuesAreValid_)
    {
      // get the values from the default table if this is the first time
      // we are calling this method
      NADefaults &def = ActiveSchemaDB()->getDefaults();

      initialQueueSizeDown_ =
	(ULng32) def.getAsULong(DYN_QUEUE_RESIZE_INIT_DOWN);
      initialQueueSizeUp_   =
	(ULng32) def.getAsULong(DYN_QUEUE_RESIZE_INIT_UP);
      initialPaQueueSizeDown_ =
	(ULng32) def.getAsULong(DYN_PA_QUEUE_RESIZE_INIT_DOWN);
      initialPaQueueSizeUp_   =
	(ULng32) def.getAsULong(DYN_PA_QUEUE_RESIZE_INIT_UP);
      queueResizeLimit_     = (short) def.getAsULong(DYN_QUEUE_RESIZE_LIMIT);
      queueResizeFactor_    = (short) def.getAsULong(DYN_QUEUE_RESIZE_FACTOR);
      makeOnljLeftQueuesBig_ = 
          (def.getToken(GEN_ONLJ_SET_QUEUE_LEFT) == DF_ON);
      onljLeftUpQueue_ = 
          (ULng32) def.getAsULong(GEN_ONLJ_LEFT_CHILD_QUEUE_UP);
      onljLeftDownQueue_ = 
          (ULng32) def.getAsULong(GEN_ONLJ_LEFT_CHILD_QUEUE_DOWN);
      makeOnljRightQueuesBig_ = 
         (def.getToken(GEN_ONLJ_SET_QUEUE_RIGHT) == DF_ON);
      onljRightSideUpQueue_ = 
          (ULng32) def.getAsULong(GEN_ONLJ_RIGHT_SIDE_QUEUE_UP);
      onljRightSideDownQueue_ = 
          (ULng32) def.getAsULong(GEN_ONLJ_RIGHT_SIDE_QUEUE_DOWN);

      dynQueueSizeValuesAreValid_ = TRUE;
    }

  if  (ActiveSchemaDB()->getDefaults().getToken(DYN_QUEUE_RESIZE_OVERRIDE) == DF_ON)
    {
      tdb->setQueueResizeParams(tdb->getMaxQueueSizeDown(), tdb->getMaxQueueSizeUp(),
                                queueResizeLimit_,queueResizeFactor_);
    }
  //Typically the sequence operaotr may have to deal with a large numer of rows when 
  //it's part of the IM tree that performs elimination of dups. 
  if ((tdb->getNodeType() == ComTdb::ex_SEQUENCE_FUNCTION) && isEffTreeUpsert())
    {
     tdb->setQueueResizeParams(tdb->getMaxQueueSizeDown(), tdb->getMaxQueueSizeUp(),
                                queueResizeLimit_,queueResizeFactor_); 
    }
   // Make the size of the upQ of ONLJ the same as that of the upQ
   // of the right child. 
   if ((tdb->getNodeType() == ComTdb::ex_ONLJ || getRightSideOfOnlj()) 
        && makeOnljRightQueuesBig_)
    {
      tdb->setQueueResizeParams(onljRightSideDownQueue_,
			        onljRightSideUpQueue_,
			        queueResizeLimit_,
			        queueResizeFactor_);
    }
  else
    {
      tdb->setQueueResizeParams(initialQueueSizeDown_,
			        initialQueueSizeUp_,
			        queueResizeLimit_,
			        queueResizeFactor_);
    }


  // If large queue sizes are specified, then adjust the up and down
  // queue sizes.  This is used when a SplitTop appears on the RHS of
  // a Flow/NestedJoin.  Above the SplitTop the queue sizes may be
  // very large (e.g. 128K), below the split top they will be modestly
  // large (e.g. 2048)
  if(largeQueueSize_ > 0 &&
     tdb->getInitialQueueSizeDown() < largeQueueSize_ &&
     tdb->getNodeType() != ComTdb::ex_ROOT &&
     (ActiveSchemaDB()->getDefaults().getToken(USE_LARGE_QUEUES) == DF_ON)) {

    tdb->setQueueResizeParams(largeQueueSize_,
                              largeQueueSize_,
                              queueResizeLimit_,
                              queueResizeFactor_);
  }
 
  tdb->setTdbId(getAndIncTdbId());

  tdb->setPlanVersion(ComVersion_GetCurrentPlanVersion());
    
  if (computeStats())
    {
      tdb->setCollectStats(computeStats());
      tdb->setCollectStatsType(collectStatsType());
    }

  compilerStatsInfo().totalOps()++;
}

RelExpr * Generator::preGenCode(RelExpr * expr_node)
{
  // initialize flags
  //  flags_ = 0;
  //  flags2_ = 0;

  foundAnUpdate_ = FALSE;

  // create an expression generator.
  exp_generator = new (wHeap()) ExpGenerator(this);

  // later get it from CmpContext after parser sets it there.
  const NAString * val =
    ActiveControlDB()->getControlSessionValue("SHOWPLAN");
  if ((val) && (*val == "ON"))
    exp_generator->setShowplan(1);

  // the following is to support transaction handling for
  // CatAPIRequest.  Not to start transaction if TRANSACTION OFF.
  const NAString * tval =
    ActiveControlDB()->getControlSessionValue("TRANSACTION");
  if ((tval) && (*tval == "OFF"))
    exp_generator->setNoTransaction(1);

  if (CmpCommon::context()->GetMode() == STMT_STATIC)
    staticCompMode_ = TRUE;
  else
    staticCompMode_ = FALSE;

  // remember whether expression has order by clause
  if (NOT ((RelRoot *)expr_node)->reqdOrder().isEmpty())
    orderRequired_ = TRUE;

  NAString tmp;

  computeStats_ = FALSE;
  CmpCommon::getDefault(DETAILED_STATISTICS, tmp, -1);
  if ((tmp != "OFF") &&
      (! Get_SqlParser_Flags(DISABLE_RUNTIME_STATS)))

    {
      computeStats_ = TRUE;
      if ((tmp == "ALL") || (tmp == "ON"))
      	collectStatsType_ = ComTdb::ALL_STATS;
      else if (tmp == "ACCUMULATED")
	collectStatsType_ = ComTdb::ACCUMULATED_STATS;
      else if (tmp == "PERTABLE")
	collectStatsType_ = ComTdb::PERTABLE_STATS;
      else if (tmp == "OPERATOR")
	collectStatsType_ = ComTdb::OPERATOR_STATS;
      else
	computeStats_ = FALSE;
    }
    else
       explainInRms_ = FALSE;

  if (CmpCommon::getDefault(COMP_BOOL_156) == DF_ON)
    collectRtsStats_ = TRUE;
  else
    collectRtsStats_ = FALSE;

  if (CmpCommon::getDefault(COMP_BOOL_166) == DF_OFF)
    r251HalloweenPrecode_ = true;
  else 
    r251HalloweenPrecode_ = false;

  precodeHalloweenLHSofTSJ_ = false;
  precodeRHSofNJ_ = false;
  unblockedHalloweenScans_ = 0;
  halloweenSortForced_ = false;
  halloweenESPonLHS_ = false;

  CmpCommon::getDefault(OVERFLOW_MODE, tmp, -1);
  if (tmp == "SSD")
    overflowMode_ = ComTdb::OFM_SSD;
  else if (tmp == "MMAP")
    overflowMode_ = ComTdb::OFM_MMAP;
  else
    overflowMode_ = ComTdb::OFM_DISK;
  // turn computeStats off if this is a SELECT from statistics virtual
  // table stmt, a control stmt or a SPJ result sets proxy statement.
  if (expr_node->child(0) &&
      ((expr_node->child(0)->getOperatorType() == REL_STATISTICS) ||
       (expr_node->child(0)->getOperatorType() == REL_CONTROL_QUERY_SHAPE) ||
       (expr_node->child(0)->getOperatorType() == REL_CONTROL_QUERY_DEFAULT) ||
       (expr_node->child(0)->getOperatorType() == REL_CONTROL_TABLE) ||
       (expr_node->child(0)->getOperatorType() == REL_CONTROL_SESSION) ||
       (expr_node->child(0)->getOperatorType() == REL_SET_SESSION_DEFAULT) ||
       (expr_node->child(0)->getOperatorType() == REL_TRANSACTION) ||
       (expr_node->child(0)->getOperatorType() == REL_DESCRIBE) ||
       (expr_node->child(0)->getOperatorType() == REL_LOCK) ||
       (expr_node->child(0)->getOperatorType() == REL_UNLOCK) ||
       (expr_node->child(0)->getOperatorType() == REL_SET_TIMEOUT) ||
       (expr_node->child(0)->getOperatorType() == REL_CONTROL_RUNNING_QUERY) ||
       (expr_node->child(0)->getOperatorType() == REL_SP_PROXY)))
   {
      computeStats_ = FALSE;
      explainInRms_ = FALSE;
   }
   if (expr_node->child(0) &&
      ((expr_node->child(0)->getOperatorType() == REL_DDL) ||
      (expr_node->child(0)->getOperatorType() == REL_CALLSP) ||
      (expr_node->child(0)->getOperatorType() == REL_EXE_UTIL)))
     explainInRms_ = FALSE;
   if (expr_node->child(0) &&
    (expr_node->child(0)->getOperatorType() == REL_EXE_UTIL &&
	((((ExeUtilExpr*)expr_node->child(0)->castToRelExpr())->getExeUtilType() == 
	 ExeUtilExpr::GET_STATISTICS_) ||
        (((ExeUtilExpr*)expr_node->child(0)->castToRelExpr())->getExeUtilType() == 
	 ExeUtilExpr::DISPLAY_EXPLAIN_))))
    {
      computeStats_ = FALSE;
      explainInRms_ = FALSE;
    }
   
#ifdef _DEBUG
  if (getenv("NO_DETAILED_STATS"))
  {
    computeStats_ = FALSE;
    explainInRms_ = FALSE;
  }
#endif

  setUpdatableSelect(((RelRoot *)expr_node)->updatableSelect());

  // see if aqr could be done
  NABoolean aqr = FALSE;
  // Only dynamic queries from odbc/jdbc, NCI or mxci will enable AQR.
  if ((staticCompMode_ == FALSE) &&
      ((CmpCommon::getDefault(IS_SQLCI) == DF_ON) ||
       (CmpCommon::getDefault(NVCI_PROCESS) == DF_ON) ||
       (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON)))
    {
      // Users can enable aqr by setting AUTO_QUERY_RETRY to ON.
      if (CmpCommon::getDefault(AUTO_QUERY_RETRY) == DF_ON)
	{
	  aqr = TRUE;
	}
      else if (CmpCommon::getDefault(AUTO_QUERY_RETRY) == DF_SYSTEM)
	{
	  if (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
	    {
	      // if internal query from executor for explain, enable aqr.
	      const NAString * val =
		ActiveControlDB()->getControlSessionValue("EXPLAIN");
	      if (((val) && (*val == "ON")) ||
                  (exp_generator->getShowplan()))
		{
		  aqr = TRUE;
		}
	      else
		{
		  aqr = FALSE;
		}
	    }
	  else
	    {
	      aqr = TRUE;
	    }
	}
    }
  setAqrEnabled(aqr);

  // pre code gen.
  ValueIdSet pulledInputs;
  return expr_node->preCodeGen(
       this,
       expr_node->getGroupAttr()->getCharacteristicInputs(),
       pulledInputs);
}


void Generator::genCode(const char *source, RelExpr * expr_node)
{
  // Set the plan Ident. to be a time stamp.
  // This is used by EXPLAIN as the planID.

  planId_ = NA_JulianTimestamp();

  explainNodeId_ = 0;

  explainTuple_ = NULL;

  stmtSource_ = source;

  NExDbgInfoObj_.setNExStmtSrc( (char *)source );

  explainFragDirIndex_ = NULL_COLL_INDEX;

  explainIsDisabled_ = 0;
  if (CmpCommon::getDefault(GENERATE_EXPLAIN) == DF_OFF)
    disableExplain();

  foundAnUpdate_ = FALSE;
  
  // walk through the tree of RelExpr and ItemExpr objects, generating
  // ComTdb, ex_expr and their relatives
  expr_node->codeGen(this);

  // pack each fragment independently; packing converts pointers to offsets
  // relative to the start of the fragment
  for (CollIndex i = 0; i < fragmentDir_->entries(); i++)
  {
    Space *fragSpace = fragmentDir_->getSpace(i);
    char *fragTopNode = fragmentDir_->getTopNode(i);

    switch (fragmentDir_->getType(i))
    {
      case FragmentDir::MASTER:
        ComTdbPtr((ComTdb *)fragTopNode).pack(fragSpace);
        break;

      case FragmentDir::DP2:
        GenAssert(0,"DP2 fragments not supported");
        break;

      case FragmentDir::ESP:
        ComTdbPtr((ComTdb *)fragTopNode).pack(fragSpace);
        break;

      case FragmentDir::EXPLAIN:
        ExplainDescPtr((ExplainDesc *)fragTopNode).pack(fragSpace);
        break;
    }
  }

}

Generator::~Generator()
{
  // cleanup
  if (fragmentDir_)
    NADELETE(fragmentDir_,  FragmentDir, wHeap());
}

// moves the generated code into out_buf. If the generated code
// is allocated from a list of buffers, then each of the buffer is
// moved contiguously to out_buf. The caller MUST have allocated
// sufficient space in out_buf to contain the generated code.
char * Generator::getFinalObj(char * out_buf, ULng32 out_buflen)
{
  if (out_buflen < (ULng32)getFinalObjLength())
    return NULL;

  // copy the objects of all spaces into one big buffer
  Lng32 outputLengthSoFar = 0;
  for (CollIndex i = 0; i < fragmentDir_->entries(); i++)
    {
      // copy the next space into the buffer
      if (fragmentDir_->getSpace(i)->makeContiguous(
	   &out_buf[outputLengthSoFar],
	   out_buflen - outputLengthSoFar) == 0)
	return NULL;
      outputLengthSoFar += fragmentDir_->getFragmentLength(i);
    }
  return out_buf;
}

void Generator::doRuntimeSpaceComputation(char * root_tdb,
					  char * fragTopNode,
					  Lng32 &tcbSize)
{
  tcbSize = 0;
  // compute space.
  tcbSize = SQL_EXEC_GetTotalTcbSpace(root_tdb, fragTopNode);
}

void Generator::setTransactionFlag(NABoolean transIsNeeded,
				   NABoolean isNeededForAllFragments)
{
  if (transIsNeeded) // if transaction is needed
    {
      // remember it for the entire statement...
      flags_ |= TRANSACTION_FLAG;

      if (fragmentDir_->entries() > 0)
	{
	  // ...and also for the current fragment...
	  fragmentDir_->setNeedsTransaction(fragmentDir_->getCurrentId(),TRUE);
	  // ...and for the root fragment
	  fragmentDir_->setNeedsTransaction(0,TRUE);
	  
	  if (isNeededForAllFragments)
	    fragmentDir_->setAllEspFragmentsNeedTransaction();
	}
    }
}

void Generator::resetTransactionFlag()
{
  flags_ &= ~TRANSACTION_FLAG;
}

TransMode * Generator::getTransMode()
{
  return CmpCommon::transMode() ;
}

// Verify that the current transaction mode is suitable for
// update, delete, insert, or ddl operation.
// Clone of code in GenericUpdate::bindNode(),
// which cannot (?) detect all cases of transgression (at least the DDL ones).
//
void Generator::verifyUpdatableTransMode(StmtLevelAccessOptions *sAxOpt,
					 TransMode * tm,
					 TransMode::IsolationLevel *ilForUpd)
{
  Lng32 sqlcodeA = 0, sqlcodeB = 0;

  //  if (getTransMode()->isolationLevel() == TransMode::READ_UNCOMMITTED_)
  //    sqlcodeA = -3140;
  // if (getTransMode()->accessMode()     != TransMode::READ_WRITE_)
  //   sqlcodeB = -3141;

  TransMode::IsolationLevel il;
  ActiveSchemaDB()->getDefaults().getIsolationLevel
    (il,
     CmpCommon::getDefault(ISOLATION_LEVEL_FOR_UPDATES));
  verifyUpdatableTrans(sAxOpt, tm,
		       il,
		       sqlcodeA, sqlcodeB);
  if (ilForUpd)
    *ilForUpd = il;

  if (sqlcodeA || sqlcodeB)
    {
      // 3140 The isolation level cannot be READ UNCOMMITTED.
      // 3141 The transaction access mode must be READ WRITE.
      if (sqlcodeA) *CmpCommon::diags() << DgSqlCode(sqlcodeA);
      if (sqlcodeB) *CmpCommon::diags() << DgSqlCode(sqlcodeB);
      GenExit();
    }
  setNeedsReadWriteTransaction(TRUE);
}

CollIndex Generator::addFileDesc(const IndexDesc* desc, SqlTableOpenInfo* stoi)
  {
  CollIndex index = baseFileDescs_.entries();
  baseFileDescs_.insert(desc);
  baseStoiList_.insert(stoi);
  numOfVpsPerBase_.insert(0);   // no vertical partitions for this entry
  return index;
  }

CollIndex Generator::addVpFileDesc(const IndexDesc* vpDesc,
                                   SqlTableOpenInfo* stoi,
                                   CollIndex& vpIndex)
  {
  const TableDesc* baseDesc = vpDesc->getPrimaryTableDesc();
  CollIndex i = 0;
  for (i = 0; i < baseFileDescs_.entries(); i++)
    {
    if (numOfVpsPerBase_[i] &&
        baseDesc == baseFileDescs_[i]->getPrimaryTableDesc())
      break;
    }
  if (i == baseFileDescs_.entries())
    {  // no base entry found, add new base file descriptor
    i = addFileDesc(vpDesc, stoi);
    }
  vpIndex = numOfVpsPerBase_[i]; // vp descriptor index
  numOfVpsPerBase_[i]++;
  vpFileDescs_.insert(vpDesc);  // add to list of all vp descriptors
  return i;
  }

NAHeap* Generator::wHeap()
{
  return (currentCmpContext_) ? currentCmpContext_->statementHeap() : 0;
}

void Generator::setGenObj(const RelExpr * node, ComTdb * genObj_)
{
  genObj = genObj_;

  // if my child needs to return first N rows, then get that number from
  // my child and set it in my tdb. At runtime, my tcb's work method will
  // ask my child for N rows with a GET_N request.
  //  if (node && node->child(0) && genObj)
  //    genObj->firstNRows() = (Int32)((RelExpr *)node)->child(0)->getFirstNRows();
};

ComTdbRoot * Generator::getTopRoot()
{
  for (CollIndex i = 0; i < fragmentDir_->entries(); i++)
    {
      if (fragmentDir_->getType(i) == FragmentDir::MASTER) {
        ComTdb *root = (ComTdb *)(fragmentDir_->getTopNode(i));

        GenAssert(root->getNodeType() == ComTdb::ex_ROOT, "Bad Top Root");
        return (ComTdbRoot *)root;
      }
    }
  return NULL;
}

const Space* Generator::getTopSpace() const
{
  for (CollIndex i = 0; i < fragmentDir_->entries(); i++) {
    if (fragmentDir_->getType(i) == FragmentDir::MASTER) {
      return fragmentDir_->getSpace(i);
    }
  }
  return NULL;
}

//
// Handle user specified ESP remapping case.  
//
// Return false if the specification is not correct. For spec, refer to
// COMP_STRING_2 in the comment section for method remapESPAllocationAS().
//
NABoolean remapESPAllocationViaUserInputs(FragmentDir *fragDir,
                              const char *espOrder,
                              CollHeap *heap)
{

  CollIndex i;

  // if CycleSegs TRUE, will cause each ESP layer to start with the next
  // CPU in the list.
  //
  NABoolean cycleSegs =
    (ActiveSchemaDB()->getDefaults()).getAsLong(CYCLIC_ESP_PLACEMENT);
  Int32 numCPUs = gpClusterInfo->numOfPhysicalSMPs();

  ULng32 *utilcpus = new (heap) ULng32[numCPUs];
  ULng32 *utilsegs = new (heap) ULng32[numCPUs];

  // Parse the espOrderString is specified.
  //
  
  // Indicates if the espOrderString is properly specified.
  //
  NABoolean espOrderOK = FALSE;
  if(espOrder && *espOrder) {

    espOrderOK = TRUE;
    const char *espOrderp = espOrder;

    for (i = 0; i < (CollIndex)numCPUs && espOrderOK && *espOrderp; i++) {

      Lng32 seg = 0;
      Lng32 cpu = 0;
      Lng32 state = 0;

      if(*espOrderp >= '0' && *espOrderp <= '9') {
        state++;

        seg = atoi(espOrderp);
        while(*espOrderp >= '0' && *espOrderp <= '9')
          espOrderp++;
      }

      if(*espOrderp == ':') {
        espOrderp++;
        state++;
      }

      if(*espOrderp >= '0' && *espOrderp <= '9') {
        state++;

        cpu = atoi(espOrderp);
        while(*espOrderp >= '0' && *espOrderp <= '9')
          espOrderp++;
      }

      if(*espOrderp == ',')
        espOrderp++;

      if(state == 3) {

        utilcpus[i] = cpu;
        utilsegs[i] = seg;

      } else {
        espOrderOK = FALSE;
      }
    }
    
  }
  
  Int32 numEntries = i;

  if(!espOrderOK) {

    return FALSE;

  } else {
    // Remap Each ESP fragment.
    //

    Int32 nextCPUToUse = 0;

    for (i = 0; i < fragDir->entries(); i++) {
      if (fragDir->getPartitioningFunction(i) != NULL &&
          fragDir->getType(i) == FragmentDir::ESP)
        {

          // Get the node map for this ESP fragment.
          //
          NodeMap *nodeMap =
            (NodeMap *)fragDir->getPartitioningFunction(i)->getNodeMap();
          // If this node map qualified for remapping ...
          //
          if (nodeMap->getNumEntries() <= (CollIndex)numCPUs) {


            // Copy the existing node map for this ESP fragment.
            // Need to make a copy because this node map could be
            // shared with other node maps.
            //
            nodeMap = nodeMap->copy(heap);
            
            // Reset for each ESP layer, unless cycleSegs was specified.
            //
            if(!cycleSegs)
              nextCPUToUse = 0;

            // Remap each entry in the node map for this fragment.
            //
            for(CollIndex j=0; j < nodeMap->getNumEntries(); j++) {

              // The index into the CPU and Segment maps.  This
              // cpuNumber is the number relative to the whole
              // system (all segments)
              //
              ULng32 cpuNumber = nextCPUToUse++;

              // Wrap around if at end of list.
              //
              if(nextCPUToUse == numEntries) {
                nextCPUToUse = 0;
              }

              // Get the cpu based on the CPU map.
              // This cpu is the cpu number for a specific segment.
              //
              Lng32 cpu = (Lng32)utilcpus[cpuNumber];
              Lng32 seg = (Lng32)utilsegs[cpuNumber];

              // Set the cpu and segment for this node map entry.
              //
              nodeMap->setNodeNumber(j, cpu);
              nodeMap->setClusterNumber(j, seg);
            }

            // After remapping the node map (copy), make it the
            // node map for this ESP fragment.
            //
            PartitioningFunction *partFunc = (PartitioningFunction *)
              (fragDir->getPartitioningFunction(i));
            partFunc->replaceNodeMap(nodeMap);

          }
        }
      
    }
  }
  return TRUE;
}

// remapESPAllocationAS: Called by RelRoot::codeGen()

// NOTE: This is likely not working at this time! Keep the value
// of CQD AFFINITY_VALUE at '-2' for now.

// Re-assign each ESP to a CPU for adaptive segmentation based on the
// affinity value.
//
// To disable ESP remapping, set the CQD AFFINITY_VALUE to '-2' (default)
// Settings for AFFINITY_VALUE:
//
//   -4 - Use session based remapping (use an affinity value based on
//        the location of the MXCMP (and Master EXE)). On Linux, this 
//        option is the same as -3 as the location of the mxsrvr processes
//        are running on communication nodes which are different than
//        the SQL nodes. It is difficult to know which SQL node to use
//        when a connection is established on a communication node. This is
//        the default.
//   -3 - Use session based remapping (use an affinity value based on
//        session ID)
//   -2 - Disable ESP remapping 
//   -1 - Use random ESP remapping (use a random affinity value)
//   positive integer: remap ESPs based on given value.
//
// Other settings:
//
// CYCLIC_ESP_PLACEMENT - Use a different affinity value for each ESP
// layer of the query.  The affinity value is incremented after each
// ESP layer.
//
// DEFAULT_DEGREE_OF_PARALLELISM - Used to specify the
// affinityGroupThreshold.  ESP layers that are smaller than the
// affinityGroupThreshold will be placed randomly within the segment
// specified by the affinity value.
//
// Experimental Settings:
//
// COMP_BOOL_171 - shiftESPs - Default OFF (FALSE).  If TRUE (ON),
// then for ESP layers that use all CPUs, shift the ESP mapping within
// each segment, based on the affinity value.  For example, a two
// segment node map which uses all CPUs (\S1:0-15, S2:0-15) would be
// remapped to : (\S1:3-15,0-2 \S2:3-15,0-2) for an affinity value of
// 3.  Here the node map is shifted by three for each segment.
//
// COMP_BOOL_172 - shiftESPs2 - Default OFF (FALSE).  If TRUE (ON),
// then for ESP layers that use all CPUs, shift the ESP mapping across
// all segments, based on the affinity value.  For example, a two
// segment node map which uses all CPUs (\S1:0-15, S2:0-15) would be
// remapped to : (\S1:3-15 \S2:0-15 \S1:0-2) for an affinity value of
// 3.  Here the node map is shifted by three across all segments.
//
// COMP_STRING_2 - remapString - Default empty (do not use remap
// string).  If set, must be of the form:
//
//   "<seg_number>:<cpu_number>[,<seg_number>:<cpu_number>]..."
//
// and must contain numCPUs entries. Any deviation from this form will
// cause it to be ignored.  If set properly, the string specifies a
// different ordering of the CPUs for the purposes of remapping.  For
// instance if the remapString were set to:
// "1:0,1:8,1:1,1:9,1:2,1:10,1:3,1:11,...", then segment 1, CPU 0
// would be treated as CPU 0, segment 1, CPU 8 would be treated as CPU
// 1 and so on.  With this control, it is possible to map each
// adaptive segment to any subset of CPUs.
//
void
Generator::remapESPAllocationAS()
{
  // If set, defines a new ordering of the CPUs.
  //
  const char *espOrder =
    ActiveSchemaDB()->getDefaults().getValue(COMP_STRING_2);

  if(espOrder && *espOrder) {
    if(remapESPAllocationViaUserInputs(fragmentDir_, espOrder, wHeap())) {
      return;
    }
  }

  CollIndex i;

  Lng32 affinityDef = ActiveSchemaDB()->getDefaults().getAsLong(AFFINITY_VALUE);


  // Check is ESP mapping is enabled.
  // '-2' or less indicates that it is disabled.
  //
  if(affinityDef >= -1 || affinityDef == -3 || affinityDef == -4) {

    // Affinity_value of '-1', use a random affinity value for this query.
    //
    NABoolean useRand = (affinityDef == -1);

    NABoolean useSession = (affinityDef == -3);

    NABoolean useLocation = (affinityDef == -4);

    if ( useLocation ) {
       useSession = TRUE;
       useLocation = FALSE;
    }

    // if CycleSegs TRUE, will cause ESP layers after layersInCylce to use the
    // next affinity_value.
    //
    ULng32 layersInCycle =
      ((ActiveSchemaDB()->getDefaults()).getAsLong(CYCLIC_ESP_PLACEMENT));
      
    NABoolean cycleSegs = (layersInCycle > 0);

    // if shiftESPs TRUE, then shift node map within each segment.
    //
    NABoolean shiftESPs =
      (CmpCommon::getDefault(COMP_BOOL_171) == DF_ON);

    // if shiftESPs TRUE, then shift node map across all segments.
    //
    NABoolean shiftESPs2 =
      (CmpCommon::getDefault(COMP_BOOL_172) == DF_ON);

    // If set, defines a new ordering of the CPUs.
    //
    const char *remap =
      ActiveSchemaDB()->getDefaults().getValue(COMP_STRING_2);

    // The affinityGroupThreshold, specifies the ESP layer size below
    // which a random affinity value is used.
    //
    ULng32 affinityGroupThreshold =
      ActiveSchemaDB()->getDefaults().getAsLong(DEFAULT_DEGREE_OF_PARALLELISM);

    // A list of nodes in this cluster
    //
    const NAArray<CollIndex> &cpuArray(gpClusterInfo->getCPUArray());
    Int32 numCPUs = cpuArray.entries();
    Int32 numSegs = 1;

    CollIndex espsPerNode = 
           ActiveSchemaDB()->getDefaults().getNumOfESPsPerNode();

    // Adjust the affinityGroupThreshold so that numSegsThreshold will
    // be a power of 2.
    //
    {
      ULng32 agThreshold = numCPUs;

      while (affinityGroupThreshold < agThreshold)
        agThreshold /= 2;

      affinityGroupThreshold = agThreshold;
    }
    if(!affinityGroupThreshold) affinityGroupThreshold = 1;

    // The number of adaptive segments at the affinityGroupThreshold.
    // (Should result in a value that is a power of 2).
    //
    ULng32 numSegsThreshold = (numCPUs/affinityGroupThreshold);

    // The bit mask used to mask the specified part of the affinity value
    // (as opposed to the random portion of the affinity value)
    //
    ULng32 nsThresholdMask = (numSegsThreshold - 1);

    // Contruct a random affinity value.  Used when a random affinity
    // value is requested and when the ESP layer size is less that the
    // adjusted affinityGroupThreshold.
    //
    ULng32 randAffinity = (ULng32)(getPlanId() & 0x7FFFFFFF);
    randAffinity = randAffinity ^ (ULng32)((Long)this);

    // The affinity value to use for this query.
    //
    ULng32 affinity;
    if (useRand) {
      affinity = randAffinity;
    } else if(useSession) {

      const char *sessionId =
        ActiveSchemaDB()->getDefaults().getValue(SESSION_ID);

      Lng32 length = strlen(sessionId);

      length = (length > 43 ? 43 : length);

      affinity = ExHDPHash::hash(sessionId, ExHDPHash::NO_FLAGS, length);

    } else if(useLocation) {

      const char *sessionId =
        ActiveSchemaDB()->getDefaults().getValue(SESSION_ID);

      Lng32 length = strlen(sessionId);

      Int64 segmentNum_l = 1;
      Int64 cpu_l = 0;
      Int64 pin_l = 0;
      Int64 schemaNameCreateTime = 0;
      Int64 sessionUniqNum;
      Lng32 userNameLen = 0;
      Lng32 userSessionNameLen = 0;

      ComSqlId::extractSqlSessionIdAttrs((char *)sessionId,
                                         length,
                                         segmentNum_l,
                                         cpu_l,
                                         pin_l,
                                         schemaNameCreateTime,
                                         sessionUniqNum,
                                         userNameLen, NULL,         // Not Used
                                         userSessionNameLen, NULL); // Not Used

      affinity = (ULng32)((segmentNum_l - 1) + (numSegs * cpu_l));

    } else {
      affinity = (ULng32)affinityDef;
    }

    // Save the affinity value in the Generator so it can be used by Explain.
    //
    setAffinityValueUsed(affinity);

    // Create a CPU map based on the remapString if specified
    // properly, or the super node map otherwise.
    //
    
      // Allocate structures to hold the CPU and Segment maps.
      // Allocate the segment used map.  Used to determine if a given
      // ESP layer uses all segments.
      //
      ULng32 *cpus = new (wHeap()) ULng32[numCPUs];

          CollIndex segment = 0;
          CollIndex cpu = 0;

#ifdef _DEBUG
    if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
        (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON )) {
      OptDebug* optDbg=CmpCommon::context()->getOptDbg();
      optDbg->stream() << "numCPUs=" << numCPUs << endl;
      optDbg->stream() << "segment=" << segment << endl;
      optDbg->stream() << "cpus array:" << endl;
    }
#endif

          for (i = 0; i < (CollIndex)numCPUs; i++) {

            cpus[i] = cpu;

#ifdef _DEBUG
    if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
        (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON )) {
      OptDebug* optDbg = CmpCommon::context()->getOptDbg();
      optDbg->stream() << "segment=" << segment  ;
      optDbg->stream() << ", cpu=" << cpu << endl;
      optDbg->stream() << "cpus[" << i << "]=" << cpus[i] << endl;
    }
#endif

            // advance to next cpu
            //
            if((i % espsPerNode) == (espsPerNode-1))
	       cpu++;
            if (cpu >= (cpuArray.entries() * espsPerNode)) {
              cpu = 0;
            }
        }

      ULng32 espLayersInCurrentCycle = 0;
      
#ifdef _DEBUG
    if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
        (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON )) {
      OptDebug* optDbg = CmpCommon::context()->getOptDbg();
      optDbg->stream()
	<< "Remap::" << endl
	<< "affinity = " << affinity << endl
	<< "numSegs = " << numSegs << endl
	<< "numCPUs = " << numCPUs << endl
	<< "cpus (0,1,5) = " << cpus[0] << " " << cpus[1] << " " << cpus[5] << endl
	<< "entries = " << fragmentDir_->entries() << endl;
    }
#endif


      // Remap Each ESP fragment.
      //

      for (i = 0; i < fragmentDir_->entries(); i++) {
        if (fragmentDir_->getPartitioningFunction(i) != NULL &&
            fragmentDir_->getType(i) == FragmentDir::ESP)
          {

            // Get the node map for this ESP fragment.
            //
            NodeMap *nodeMap =
              (NodeMap *)fragmentDir_->getPartitioningFunction(i)->getNodeMap();
#ifdef _DEBUG
    if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
        (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON )) {
      OptDebug* optDbg = CmpCommon::context()->getOptDbg();
      optDbg->stream()
	<< "NodeMap::" << endl
	<< "entries = " << nodeMap->getNumEntries() << endl;
    }
#endif
            // If this node map qualified for remapping ...
            //
            if ((nodeMap->getNumEntries() != 1) &&
                (nodeMap->getNumEntries() <= (CollIndex)numCPUs)) {


#ifdef _DEBUG
    if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
        (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON )) {
      OptDebug* optDbg = CmpCommon::context()->getOptDbg();
      optDbg->stream()
	<< "shiftESPs2 = " << shiftESPs2 << endl;
    }
#endif

                // Contruct a new affinity value composed of the
                // specified portion of the affinity value and the
                // random portion for ESP layers less than the
                // threshold.
                //
                ULng32 newAffinity =
                  ((affinity & nsThresholdMask)|(randAffinity & ~nsThresholdMask));

                // The skip distance between CPUs. (can also be
                // thought of as the number of adaptive segments to
                // choose from for this ESP layer. '
                //
                ULng32 skip = (numCPUs/nodeMap->getNumEntries());

                // The starting offset for the node map.  Can also be
                // thought of as the choice of adaptive segment for
                // this ESP layer.
                //
                ULng32 offset = (newAffinity % skip);

                // Make sure we do not contruct a map that would
                // exceed the number of CPUs
                //
                GenAssert((skip * (nodeMap->getNumEntries()-1)) + offset
                          < (CollIndex)numCPUs,
                          "Bad Auto Remap Calculation");

                // Copy the existing node map for this ESP fragment.
                // Need to make a copy because this node map could be
                // shared with other node maps.
                //
                nodeMap = nodeMap->copy(wHeap());

#ifdef _DEBUG
		if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
		    (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON )) {
                  OptDebug* optDbg = CmpCommon::context()->getOptDbg();
		  optDbg->stream()
		    << "Remapping::" << endl
		    << "skip = " << skip << endl
		    << "offset = " << offset << endl;
		}
#endif

                // Remap each entry in the node map for this fragment.
                //
                for(CollIndex j=0; j < nodeMap->getNumEntries(); j++) {

                  // The index into the CPU and Segment maps.  This
                  // cpuNumber is the number relative to the whole
                  // system (all segments)
                  //
                  ULng32 cpuNumber = (j * skip) + offset;

                  // If shiftESP2 is specified, circular shift Full ESP layers
                  // across all segments based on the affinity value
                  //
                  if(shiftESPs2 &&
                     (nodeMap->getNumEntries() == (CollIndex)numCPUs)) {

                    cpuNumber = (cpuNumber + (newAffinity%numCPUs)) % numCPUs;
                  }

                  // Get the cpu based on the CPU map.
                  // This cpu is the cpu number for a specific segment.
                  //
                  Lng32 cpu = (Lng32)cpus[cpuNumber];
#ifdef _DEBUG
		if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
		    (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON )) {
                  OptDebug* optDbg = CmpCommon::context()->getOptDbg();
		  optDbg->stream()
		    << " cpu = " << cpu << endl
		    << " cpuNumber = " << cpuNumber << endl;
		}
#endif

                  // If shiftESP is specified, circular shift Full ESP layers
                  // within each segment based on the affinity value
                  // (assume 16 CPUs per segment for now).
                  //
                  if(shiftESPs &&
                     (nodeMap->getNumEntries() == (CollIndex)numCPUs)) {

                    cpu = (cpu + (Lng32)(newAffinity%16)) % 16;

                  }

                  // Set the cpu and segment for this node map entry.
                  //
                  nodeMap->setNodeNumber(j, cpuArray[cpu % numCPUs]);
                }

                // After remapping the node map (copy), make it the
                // node map for this ESP fragment.
                //
                PartitioningFunction *partFunc = (PartitioningFunction *)
                  (fragmentDir_->getPartitioningFunction(i));
                partFunc->replaceNodeMap(nodeMap);

                espLayersInCurrentCycle++;

                // Count an ESP fragment with BMOs twice. This is an 
                // approximation to the real logic used in run-time to
                // pack ESPs with BMO operators into ESP proceses. 
                if ( fragmentDir_->getNumBMOs(i) > 0 )
                   espLayersInCurrentCycle++;
   
                // If cycleSegs is specified, use a different affinity
                // value for layersInCycle # of ESP layers.
                //
                if (cycleSegs &&
                    (espLayersInCurrentCycle >= layersInCycle)) {
                  affinity += espsPerNode;
                  espLayersInCurrentCycle = 0;
                }
                
                // Cycle the random affintity, rather than generating
                // a new one each time
                //
                randAffinity += numSegsThreshold;
            }
          }
      }
      //     delete [] cpus;
      //     NADELETEBASIC(cpus, wHeap());
  }
}


// map ESPs randomly
void Generator::remapESPAllocationRandomly()
{
   if (!fragmentDir_ || !fragmentDir_->containsESPLayer())
     return; 

   for (Int32 i = 0; i < fragmentDir_->entries(); i++) {

     if (fragmentDir_->getPartitioningFunction(i) != NULL &&
         fragmentDir_->getType(i) == FragmentDir::ESP)
     {

       // Get the node map for this ESP fragment.
       NodeMap *nodeMap =
          (NodeMap *)fragmentDir_->getPartitioningFunction(i)->getNodeMap();

       for (CollIndex j=0; j<nodeMap->getNumEntries(); j++) {
 
          // if ESP-RegionServer colocation logic is off, then assign any node
         if (CmpCommon::getDefault(TRAF_ALLOW_ESP_COLOCATION) == DF_OFF)
           nodeMap->setNodeNumber(j, ANY_NODE);
         nodeMap->setClusterNumber(j, 0);

       }

       // After remapping the node map (copy), make it the
       // node map for this ESP fragment.
       PartitioningFunction *partFunc = (PartitioningFunction *)
                           (fragmentDir_->getPartitioningFunction(i));
       partFunc->replaceNodeMap(nodeMap);
     }
   }
}

Lng32 Generator::getRecordLength(ComTdbVirtTableIndexInfo * indexInfo,
                                 ComTdbVirtTableColumnInfo * columnInfoArray)
{
  Lng32 recLen = 0;

  if ((! indexInfo) && (! columnInfoArray))
    return recLen;

  Lng32 keyCount = indexInfo->keyColCount;
  const ComTdbVirtTableKeyInfo * keyInfoArray = indexInfo->keyInfoArray;

  if (! keyInfoArray)
    return recLen;

  for (Int16 keyNum = 0; keyNum < keyCount; keyNum++)
    {
      const ComTdbVirtTableKeyInfo &keyInfo = keyInfoArray[keyNum];
      
      const ComTdbVirtTableColumnInfo &colInfo = columnInfoArray[keyInfo.tableColNum];
      recLen += colInfo.length;
    }
  
  if (indexInfo->nonKeyInfoArray)
    {
      keyCount = indexInfo->nonKeyColCount;
      keyInfoArray = indexInfo->nonKeyInfoArray;
      
      for (Int16 keyNum = 0; keyNum < keyCount; keyNum++)
        {
          const ComTdbVirtTableKeyInfo &keyInfo = keyInfoArray[keyNum];
          
          const ComTdbVirtTableColumnInfo &colInfo = columnInfoArray[keyInfo.tableColNum];
          recLen += colInfo.length;
        }
    }
  
  return recLen;
}

TrafDesc* Generator::createColDescs(
  const char * tableName,
  ComTdbVirtTableColumnInfo * columnInfo,
  Int16 numCols,
  UInt32 &offset,
  NAMemory * space)
{
  if (! columnInfo)
    return NULL;

  TrafDesc * first_col_desc = NULL;
  TrafDesc * prev_desc = NULL;
  for (Int16 colNum = 0; colNum < numCols; colNum++)
    {
      ComTdbVirtTableColumnInfo * info = columnInfo + colNum;

      UInt32 colOffset = ExpTupleDesc::sqlarkExplodedOffsets(offset,
                           info->length,
                           (Int16) info->datatype,
                           info->nullable);

      Int32 i = colNum;                // Don't want colNum altered by the call
      Lng32 tmpOffset = (Lng32) offset;  // Ignore returned offset
      SQLCHARSET_CODE info_charset = info->charset;
      if (info_charset == SQLCHARSETCODE_UNKNOWN && (info->datatype == REC_NCHAR_V_UNICODE ||
                                                     info->datatype == REC_NCHAR_F_UNICODE ||
                                                     info->datatype == REC_NCHAR_V_ANSI_UNICODE))
        info_charset = SQLCHARSETCODE_UCS2;

      char * colname = new GENHEAP(space) char[strlen(info->colName)+1];
      strcpy(colname, info->colName);

      TrafDesc * col_desc = TrafMakeColumnDesc(
           tableName,
           colname, //info->colName,
           i,
           info->datatype,
           info->length,
           tmpOffset,
           info->nullable,
           info_charset,
           space);
      
      // Virtual tables use SQLARK_EXPLODED_FORMAT in which numeric column
      // values are aligned.  Ignore TrafMakeColumnDesc's
      // offset calculation which doesn't reflect column value alignment.
      offset = colOffset + info->length;

      // EXPLAIN__ table uses 22-bit precision REAL values
      if (info->datatype == REC_FLOAT32)
        col_desc->columnsDesc()->precision = 22;

      col_desc->columnsDesc()->precision = info->precision;
      if (DFS2REC::isInterval(info->datatype))
	col_desc->columnsDesc()->intervalleadingprec = info->precision;

      col_desc->columnsDesc()->scale = info->scale;
      if ((DFS2REC::isInterval(info->datatype)) ||
	  (DFS2REC::isDateTime(info->datatype)))
	col_desc->columnsDesc()->datetimefractprec = info->scale;

      col_desc->columnsDesc()->datetimestart = (rec_datetime_field)info->dtStart;
      col_desc->columnsDesc()->datetimeend = (rec_datetime_field)info->dtEnd;

      col_desc->columnsDesc()->setUpshifted(info->upshifted);
      col_desc->columnsDesc()->setCaseInsensitive(FALSE);

      char pt[350];
      NAType::convertTypeToText(pt, //OUT
				col_desc->columnsDesc()->datatype,
				col_desc->columnsDesc()->length,
				col_desc->columnsDesc()->precision,
				col_desc->columnsDesc()->scale,
				col_desc->columnsDesc()->datetimeStart(),
				col_desc->columnsDesc()->datetimeEnd(),
				col_desc->columnsDesc()->datetimefractprec,
				col_desc->columnsDesc()->intervalleadingprec,
				col_desc->columnsDesc()->isUpshifted(),
				col_desc->columnsDesc()->isCaseInsensitive(),
				(CharInfo::CharSet)col_desc->columnsDesc()->character_set,
				(CharInfo::Collation)col_desc->columnsDesc()->collation_sequence,
				NULL
				);
      col_desc->columnsDesc()->pictureText 
        = new GENHEAP(space) char[strlen(pt)+1];
      strcpy(col_desc->columnsDesc()->pictureText, pt);
      col_desc->columnsDesc()->setDefaultClass(info->defaultClass);
      if ((info->defaultClass == COM_NO_DEFAULT) ||
          (info->defVal == NULL))
	col_desc->columnsDesc()->defaultvalue = NULL;
      else
        {
          col_desc->columnsDesc()->defaultvalue = new GENHEAP(space) char[strlen(info->defVal) +1];
          strcpy(col_desc->columnsDesc()->defaultvalue, (char*)info->defVal);
        }

      col_desc->columnsDesc()->colclass = 'U';
      col_desc->columnsDesc()->setAdded(FALSE);
      if (info->columnClass == COM_SYSTEM_COLUMN)
	col_desc->columnsDesc()->colclass = 'S';
      else if (info->columnClass == COM_ADDED_USER_COLUMN)
	{
	  col_desc->columnsDesc()->colclass = 'A';
	  col_desc->columnsDesc()->setAdded(TRUE);
	}
      else if (info->columnClass == COM_ALTERED_USER_COLUMN)
	{
	  col_desc->columnsDesc()->colclass = 'C';
	  col_desc->columnsDesc()->setAdded(TRUE);
	}

      if (info->colHeading)
        {
          col_desc->columnsDesc()->heading = new GENHEAP(space) char[strlen(info->colHeading) + 1];
          strcpy(col_desc->columnsDesc()->heading, info->colHeading);
        }
      else
	col_desc->columnsDesc()->heading = NULL;

      if (info->hbaseColFam)
        {
          col_desc->columnsDesc()->hbaseColFam = new GENHEAP(space) char[strlen(info->hbaseColFam) + 1];
          strcpy(col_desc->columnsDesc()->hbaseColFam, (char*)info->hbaseColFam);
        }
      else
	col_desc->columnsDesc()->hbaseColFam = NULL;

      if (info->hbaseColQual)
        {
          col_desc->columnsDesc()->hbaseColQual = new GENHEAP(space) char[strlen(info->hbaseColQual) + 1];
          strcpy(col_desc->columnsDesc()->hbaseColQual, (char*)info->hbaseColQual);
        }
      else
	col_desc->columnsDesc()->hbaseColQual = NULL;

      col_desc->columnsDesc()->hbaseColFlags = info->hbaseColFlags;

      col_desc->columnsDesc()->setParamDirection(
           CmGetComDirectionAsComParamDirection(info->paramDirection));
      col_desc->columnsDesc()->setOptional(info->isOptional);
      col_desc->columnsDesc()->colFlags = info->colFlags;

      if (!first_col_desc)
	first_col_desc = col_desc;
      else
	prev_desc->next = col_desc;

      prev_desc = col_desc;
    }

  return first_col_desc;
}

static void initKeyDescStruct(TrafKeysDesc * tgt,
                              const ComTdbVirtTableKeyInfo * src,
                              NAMemory * space)
{
  if (src->colName)
    {
      tgt->keyname = new GENHEAP(space) char[strlen(src->colName) +1];
      strcpy(tgt->keyname, src->colName);
    }
  else
    tgt->keyname = NULL;
      
  tgt->keyseqnumber     = src->keySeqNum;
  tgt->tablecolnumber   = src->tableColNum;
  tgt->setDescending(src->ordering ? TRUE : FALSE);
  if (src->hbaseColFam)
    {
      tgt->hbaseColFam = new GENHEAP(space) char[strlen(src->hbaseColFam)+1];
      strcpy(tgt->hbaseColFam, src->hbaseColFam);
    }
  else
    tgt->hbaseColFam = NULL;

  if (src->hbaseColQual)
    {
      tgt->hbaseColQual = new GENHEAP(space) char[strlen(src->hbaseColQual)+1];
      strcpy(tgt->hbaseColQual, src->hbaseColQual);
    }
  else
    tgt->hbaseColQual = NULL;
}

TrafDesc * Generator::createKeyDescs(Int32 numKeys,
                                     const ComTdbVirtTableKeyInfo * keyInfo,
                                     NAMemory * space)
{
  TrafDesc * first_key_desc = NULL;

  if (keyInfo == NULL)
    return NULL;

  // create key descs
  TrafDesc * prev_desc = NULL;
  for (Int32 keyNum = 0; keyNum < numKeys; keyNum++)
    {
      TrafDesc * key_desc = TrafAllocateDDLdesc(DESC_KEYS_TYPE, space);
      if (prev_desc)
	prev_desc->next = key_desc;
      else
       first_key_desc = key_desc;

      prev_desc = key_desc;

      initKeyDescStruct(key_desc->keysDesc(),
			&keyInfo[keyNum], space);
    }

  return first_key_desc;
}

TrafDesc * Generator::createConstrKeyColsDescs(Int32 numKeys,
                                               ComTdbVirtTableKeyInfo * keyInfo,
                                               ComTdbVirtTableColumnInfo * columnInfo,
                                               NAMemory * space)
{
  TrafDesc * first_key_desc = NULL;

  if (keyInfo == NULL)
    return NULL;

  // create key descs
  TrafDesc * prev_desc = NULL;
  for (Int32 keyNum = 0; keyNum < numKeys; keyNum++)
    {
      TrafDesc * key_desc = TrafAllocateDDLdesc(DESC_CONSTRNT_KEY_COLS_TYPE, space);
      if (prev_desc)
	prev_desc->next = key_desc;
      else
       first_key_desc = key_desc;

      prev_desc = key_desc;

      ComTdbVirtTableKeyInfo * src = &keyInfo[keyNum];
      TrafConstrntKeyColsDesc * tgt = key_desc->constrntKeyColsDesc();
      if (src->colName)
	{
	  tgt->colname = new GENHEAP(space) char[strlen(src->colName) +1];
	  strcpy(tgt->colname, src->colName);
	}
      else
	tgt->colname = NULL;
      
      tgt->position   = src->tableColNum;
      ComTdbVirtTableColumnInfo * info = columnInfo +  src->tableColNum;
      if(info->columnClass == COM_SYSTEM_COLUMN )
        tgt->setSystemKey(TRUE);
    }

  return first_key_desc;
}

// ****************************************************************************
// This method creates a set of trafodion descriptors (TrafDesc) based on 
// ComTdbVirtTablePrivInfo 
//
// see ComTdb.h for a description of the ComTdbVirtTablePrivInfo
// see TrafDDLdesc.h for a description of TrafDesc for the priv_desc
// ****************************************************************************
TrafDesc * Generator::createPrivDescs( const ComTdbVirtTablePrivInfo * privInfo,
                                       NAMemory * space)
{
  // When authorization is enabled, each object must have at least one grantee
  // - the system grant to the object owner
  PrivMgrDescList *privGrantees = privInfo[0].privmgr_desc_list;
  DCMPASSERT (privGrantees->entries() > 0);
 
  TrafDesc * priv_desc = TrafAllocateDDLdesc(DESC_PRIV_TYPE, space);
  TrafDesc * first_grantee_desc = NULL;
  TrafDesc * prev_grantee_desc = NULL;

  // generate a TrafPrivGranteeDesc for each grantee and
  // attach to the privileges descriptor (priv_desc)
  for (int i = 0; i < privGrantees->entries(); i++)
    {
      PrivMgrDesc *granteeDesc = (*privGrantees)[i];
      TrafDesc * curr_grantee_desc = TrafAllocateDDLdesc(DESC_PRIV_GRANTEE_TYPE, space);
      if (! first_grantee_desc)
        first_grantee_desc = curr_grantee_desc;

      curr_grantee_desc->privGranteeDesc()->grantee = granteeDesc->getGrantee();

      // generate a TrafPrivBitmap for the object level privs and
      // attach it to the privilege grantee descriptor (curr_grantee_desc)
      TrafDesc * bitmap_desc = TrafAllocateDDLdesc(DESC_PRIV_BITMAP_TYPE, space);
      PrivMgrCoreDesc objDesc = granteeDesc->getTablePrivs();

      bitmap_desc->privBitmapDesc()->columnOrdinal = -1;
      bitmap_desc->privBitmapDesc()->privBitmap = objDesc.getPrivBitmap().to_ulong();
      bitmap_desc->privBitmapDesc()->privWGOBitmap = objDesc.getWgoBitmap().to_ulong();
      curr_grantee_desc->privGranteeDesc()->objectBitmap = bitmap_desc;

      // generate a list of TrafPrivBitmapDesc, one for each column and
      // attach it to the TrafPrivGranteeDesc
      size_t numCols = granteeDesc->getColumnPrivs().entries();
      if (numCols > 0)
        {
          TrafDesc * first_col_desc = NULL;
          TrafDesc * prev_col_desc = NULL;
          for (int j = 0; j < numCols; j++)
            {
              const PrivMgrCoreDesc colBitmap = granteeDesc->getColumnPrivs()[j];
              TrafDesc * curr_col_desc = TrafAllocateDDLdesc(DESC_PRIV_BITMAP_TYPE, space);
              if (! first_col_desc)
                first_col_desc = curr_col_desc;

              curr_col_desc->privBitmapDesc()->columnOrdinal = colBitmap.getColumnOrdinal();
              curr_col_desc->privBitmapDesc()->privBitmap = colBitmap.getPrivBitmap().to_ulong();
              curr_col_desc->privBitmapDesc()->privWGOBitmap = colBitmap.getWgoBitmap().to_ulong();

              if (prev_col_desc)
                prev_col_desc->next = curr_col_desc;
              prev_col_desc = curr_col_desc;
            }
          curr_grantee_desc->privGranteeDesc()->columnBitmaps = first_col_desc;
        }
      else
        curr_grantee_desc->privGranteeDesc()->columnBitmaps = NULL;

      if (prev_grantee_desc)
         prev_grantee_desc->next = curr_grantee_desc;
      prev_grantee_desc = curr_grantee_desc;
    }
    priv_desc->privDesc()->privGrantees = first_grantee_desc;
    return priv_desc;
}


// this method is used to create both referencing and referenced constraint structs.
TrafDesc * Generator::createRefConstrDescStructs(
						    Int32 numConstrs,
						    ComTdbVirtTableRefConstraints * refConstrs,
						    NAMemory * space)
{
  TrafDesc * first_constr_desc = NULL;

  if ((numConstrs == 0) || (refConstrs == NULL))
    return NULL;

   // create constr descs
  TrafDesc * prev_desc = NULL;
  for (Int32 constrNum = 0; constrNum < numConstrs; constrNum++)
    {
      TrafDesc * constr_desc = TrafAllocateDDLdesc(DESC_REF_CONSTRNTS_TYPE, space);
      if (prev_desc)
	prev_desc->next = constr_desc;
      else
       first_constr_desc = constr_desc;

      prev_desc = constr_desc;

      ComTdbVirtTableRefConstraints * src = &refConstrs[constrNum];
      TrafRefConstrntsDesc * tgt = constr_desc->refConstrntsDesc();
      if (src->constrName)
	{
	  tgt->constrntname = new GENHEAP(space) char[strlen(src->constrName) +1];
	  strcpy(tgt->constrntname, src->constrName);
	}
      else
	tgt->constrntname = NULL;

      if (src->baseTableName)
	{
	  tgt->tablename = new GENHEAP(space) char[strlen(src->baseTableName) +1];
	  strcpy(tgt->tablename, src->baseTableName);
	}
      else
	tgt->tablename = NULL;
    }

  return first_constr_desc;
 
}

static Lng32 createDescStructs(char * tableName,
                               Int32 numCols,
                               ComTdbVirtTableColumnInfo * columnInfo,
                               Int32 numKeys,
                               ComTdbVirtTableKeyInfo * keyInfo,
                               TrafDesc* &colDescs,
                               TrafDesc* &keyDescs,
                               NAMemory * space)
{
  colDescs = NULL;
  keyDescs = NULL;
  UInt32 reclen = 0;

  // create column descs
  colDescs = Generator::createColDescs(tableName, columnInfo, (Int16) numCols,
                                       reclen, space);

  keyDescs = Generator::createKeyDescs(numKeys, keyInfo, space);

  return (Lng32) reclen;
}

static void populateRegionDescForEndKey(char* buf, Int32 len, struct TrafDesc* target)
{
   target->hbaseRegionDesc()->beginKey = NULL;
   target->hbaseRegionDesc()->beginKeyLen = 0;
   target->hbaseRegionDesc()->endKey = buf;
   target->hbaseRegionDesc()->endKeyLen = len;
}

static void populateRegionDescAsRANGE(char* buf, Int32 len, struct TrafDesc* target)
{
   target->nodetype = DESC_HBASE_RANGE_REGION_TYPE;
   populateRegionDescForEndKey(buf, len, target);
}

//
// Produce a list of TrafDesc objects. In each object, the body_struct
// field points at hbaseRegion_desc. The order of the keyinfo, obtained from
// org.apache.hadoop.hbase.client.HTable.getEndKey(), is preserved.
//
TrafDesc* Generator::assembleDescs(
     NAArray<HbaseStr >* keyArray,
     NAMemory * space)
{
   if (keyArray == NULL)
     return NULL;
   
   TrafDesc *result = NULL;
   Int32 entries = keyArray->entries();
   Int32 len = 0;
   char* buf = NULL;

   for (Int32 i=entries-1; i>=0; i-- ) {
     len = keyArray->at(i).len;
     if ( len > 0 ) { 
       buf = new GENHEAP(space) char[len];
       memcpy(buf, keyArray->at(i).val, len); 
     } else
       buf = NULL;
     
     TrafDesc* wrapper = 
       TrafAllocateDDLdesc(DESC_HBASE_RANGE_REGION_TYPE, space);
     
     populateRegionDescAsRANGE(buf, len, wrapper);
     
     wrapper->next = result;
     result = wrapper;
   }
   
   return result;
}

TrafDesc * Generator::createVirtualTableDesc
(
     const char * inTableName,
     NAMemory * heap,
     Int32 numCols,
     ComTdbVirtTableColumnInfo * columnInfo,
     Int32 numKeys,
     ComTdbVirtTableKeyInfo * keyInfo,
     Int32 numConstrs,
     ComTdbVirtTableConstraintInfo * constrInfo,
     Int32 numIndexes,
     ComTdbVirtTableIndexInfo * indexInfo,
     Int32 numViews,
     ComTdbVirtTableViewInfo * viewInfo,
     ComTdbVirtTableTableInfo * tableInfo,
     ComTdbVirtTableSequenceInfo * seqInfo,
     NAArray<HbaseStr>* endKeyArray,
     char * snapshotName,
     NABoolean genPackedDesc,
     Int32 * packedDescLen,
     NABoolean isUserTable,
     ComTdbVirtTablePrivInfo * privInfo
 )
{
  // If genPackedDesc is set, then use Space class to allocate descriptors and
  // returned contiguous packed copy of it.
  // This packed copy will be stored in metadata.

  // If heap is set (and genPackedDesc is not set), use the heap passed to
  // us by our caller. For example, we might be called at NATableDB::get time,
  // to create descriptors that are going to live in the NATable cache or on
  // the statement heap rather than in a Generator space.

  // There is some danger in this mixed use of the "space" variable as a
  // base class pointer. The NAMemory and Space classes avoid using virtual
  // functions, so we can't count on polymorphism to pick the right 
  // implementation of a method on "space". Rather, the NAMemory methods
  // will be used unless we explicitly override them. Fortunately, in
  // almost all of this method, and the methods it calls, the only use of
  // "space" is as an operand to the GENHEAP macro, which casts it as an
  // NAMemory * anyway, for use by operator new. That works for both classes.
  // There is one place in this method where we are concerned with contiguous
  // placement of objects. There, we have to cast the "space" variable to
  // class Space to get its methods. (Sorry about the variable naming; it
  // would have been a lot more changes to rename the "space" variable.)

  Space lSpace(ComSpace::GENERATOR_SPACE);
  NAMemory * space = NULL;
  if (genPackedDesc)
    space = &lSpace;
  else if (heap)
    space = heap;
  
  const char * tableName = (tableInfo ? tableInfo->tableName : inTableName);
  TrafDesc * table_desc = TrafAllocateDDLdesc(DESC_TABLE_TYPE, space);
  table_desc->tableDesc()->tablename = new GENHEAP(space) char[strlen(tableName)+1];
  strcpy(table_desc->tableDesc()->tablename, tableName);

  table_desc->tableDesc()->tableDescFlags = 0;
  table_desc->tableDesc()->catUID = 0;
  table_desc->tableDesc()->schemaUID = 0;
  if (tableInfo)
    {
      table_desc->tableDesc()->createTime = tableInfo->createTime;
      table_desc->tableDesc()->redefTime = tableInfo->redefTime;
      table_desc->tableDesc()->objectUID = tableInfo->objUID;
    }
  else
    {
      table_desc->tableDesc()->createTime = 0;
      table_desc->tableDesc()->redefTime = 0;

      ComUID comUID;
      comUID.make_UID();
      Int64 objUID = comUID.get_value();

      table_desc->tableDesc()->objectUID = objUID;
    }

  if (isUserTable)
    table_desc->tableDesc()->setSystemTableCode(FALSE);
  else
    table_desc->tableDesc()->setSystemTableCode(TRUE);

  if (tableInfo)
    table_desc->tableDesc()->setRowFormat(tableInfo->rowFormat);

  if (CmpCommon::context()->sqlSession()->validateVolatileName(tableName))
    table_desc->tableDesc()->setVolatileTable(TRUE);
  else
    table_desc->tableDesc()->setVolatileTable(FALSE);

  if (numViews > 0)
    table_desc->tableDesc()->setObjectType(COM_VIEW_OBJECT);
  else if ((seqInfo) && (! columnInfo))
    table_desc->tableDesc()->setObjectType(COM_SEQUENCE_GENERATOR_OBJECT);
  else
    table_desc->tableDesc()->setObjectType(COM_BASE_TABLE_OBJECT);

  table_desc->tableDesc()->owner = (tableInfo ? tableInfo->objOwnerID : SUPER_USER);
  table_desc->tableDesc()->schemaOwner = (tableInfo ? tableInfo->schemaOwnerID : SUPER_USER);

  if (tableInfo && tableInfo->defaultColFam)
    {
      table_desc->tableDesc()->default_col_fam = 
        new GENHEAP(space) char[strlen(tableInfo->defaultColFam)+1];
      strcpy(table_desc->tableDesc()->default_col_fam, tableInfo->defaultColFam);
    }

  if (tableInfo && tableInfo->allColFams)
    {
      table_desc->tableDesc()->all_col_fams = 
        new GENHEAP(space) char[strlen(tableInfo->allColFams)+1];
      strcpy(table_desc->tableDesc()->all_col_fams, tableInfo->allColFams);
    }

  table_desc->tableDesc()->objectFlags = (tableInfo ? tableInfo->objectFlags : 0);
  table_desc->tableDesc()->tablesFlags = (tableInfo ? tableInfo->tablesFlags : 0);

  TrafDesc * files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, space);
  files_desc->filesDesc()->setAudited(tableInfo ? tableInfo->isAudited : -1);

  table_desc->tableDesc()->files_desc = files_desc;

  TrafDesc * cols_descs = NULL;
  TrafDesc * keys_descs = NULL;
  table_desc->tableDesc()->colcount = numCols;
  table_desc->tableDesc()->record_length =
    createDescStructs(table_desc->tableDesc()->tablename,
		      numCols, columnInfo, numKeys, keyInfo,
		      cols_descs, keys_descs, space);

  TrafDesc * first_constr_desc = NULL;
  if (numConstrs > 0)
    {
      TrafDesc * prev_desc = NULL;
      for (int i = 0; i < numConstrs; i++)
	{
	  TrafDesc * curr_constr_desc = TrafAllocateDDLdesc(DESC_CONSTRNTS_TYPE, space);

	  if (! first_constr_desc)
	    first_constr_desc = curr_constr_desc;

	  curr_constr_desc->constrntsDesc()->tablename = new GENHEAP(space) char[strlen(constrInfo[i].baseTableName)+1];
	  strcpy(curr_constr_desc->constrntsDesc()->tablename, constrInfo[i].baseTableName);
	  
	  curr_constr_desc->constrntsDesc()->constrntname = new GENHEAP(space) char[strlen(constrInfo[i].constrName)+1];
	  strcpy(curr_constr_desc->constrntsDesc()->constrntname, constrInfo[i].constrName);
	  
	  curr_constr_desc->constrntsDesc()->check_constrnts_desc = NULL;
	  curr_constr_desc->constrntsDesc()->setEnforced(constrInfo[i].isEnforced);
	  curr_constr_desc->constrntsDesc()->setNotSerialized(constrInfo[i].notSerialized);

	  switch (constrInfo[i].constrType)
	    {
	    case 0: // unique_constr
	      curr_constr_desc->constrntsDesc()->type = UNIQUE_CONSTRAINT;
	      break;

	    case 1: // ref_constr
	      curr_constr_desc->constrntsDesc()->type = REF_CONSTRAINT;
	      break;

	    case 2: // check_constr
	      curr_constr_desc->constrntsDesc()->type = CHECK_CONSTRAINT;
	      break;

	    case 3: // pkey_constr
	      curr_constr_desc->constrntsDesc()->type = PRIMARY_KEY_CONSTRAINT;
	      break;

	    } // switch

	  curr_constr_desc->constrntsDesc()->colcount = constrInfo[i].colCount;

	  curr_constr_desc->constrntsDesc()->constr_key_cols_desc =
	    Generator::createConstrKeyColsDescs(constrInfo[i].colCount, constrInfo[i].keyInfoArray, columnInfo, space);

	  if (constrInfo[i].ringConstrArray)
	    {
	      curr_constr_desc->constrntsDesc()->referencing_constrnts_desc =
		Generator::createRefConstrDescStructs(constrInfo[i].numRingConstr,
						      constrInfo[i].ringConstrArray, space);
	    }

	  if (constrInfo[i].refdConstrArray)
	    {
	      curr_constr_desc->constrntsDesc()->referenced_constrnts_desc =
		Generator::createRefConstrDescStructs(constrInfo[i].numRefdConstr,
						      constrInfo[i].refdConstrArray, space);
	    }

	  if ((constrInfo[i].constrType == 2) && // check constr
	      (constrInfo[i].checkConstrLen > 0))
	    {
	      TrafDesc * check_constr_desc = TrafAllocateDDLdesc(DESC_CHECK_CONSTRNTS_TYPE, space);
	      
	      check_constr_desc->checkConstrntsDesc()->constrnt_text = 
		new GENHEAP(space) char[constrInfo[i].checkConstrLen + 1];
	      memcpy(check_constr_desc->checkConstrntsDesc()->constrnt_text,
		     constrInfo[i].checkConstrText, constrInfo[i].checkConstrLen);
	      check_constr_desc->checkConstrntsDesc()->constrnt_text
		[constrInfo[i].checkConstrLen] = 0;

	      curr_constr_desc->constrntsDesc()->check_constrnts_desc =
		check_constr_desc;
	    }

	  if (prev_desc)
	    prev_desc->next = curr_constr_desc;

	  prev_desc = curr_constr_desc;
	} // for
    }

  TrafDesc * index_desc = TrafAllocateDDLdesc(DESC_INDEXES_TYPE, space);
  index_desc->indexesDesc()->tablename = table_desc->tableDesc()->tablename;
  index_desc->indexesDesc()->indexname = table_desc->tableDesc()->tablename;
  index_desc->indexesDesc()->keytag = 0; // primary index
  index_desc->indexesDesc()->indexUID = 0;
  index_desc->indexesDesc()->record_length = table_desc->tableDesc()->record_length;
  index_desc->indexesDesc()->colcount = table_desc->tableDesc()->colcount;
  index_desc->indexesDesc()->blocksize = 32*1024;
  index_desc->indexesDesc()->setVolatile(table_desc->tableDesc()->isVolatileTable());
  index_desc->indexesDesc()->hbaseCreateOptions  = NULL;
  index_desc->indexesDesc()->numSaltPartns = 0;
  index_desc->indexesDesc()->setRowFormat(table_desc->tableDesc()->rowFormat());
  if (tableInfo)
  {
    index_desc->indexesDesc()->indexUID = tableInfo->objUID;
    
    index_desc->indexesDesc()->numSaltPartns = tableInfo->numSaltPartns;
    if (tableInfo->hbaseCreateOptions)
      {
        index_desc->indexesDesc()->hbaseCreateOptions  = 
          new GENHEAP(space) char[strlen(tableInfo->hbaseCreateOptions) + 1];
        strcpy(index_desc->indexesDesc()->hbaseCreateOptions, 
               tableInfo->hbaseCreateOptions);
      }
  }
  
  if (numIndexes > 0)
    {
      TrafDesc * prev_desc = index_desc;
      for (int i = 0; i < numIndexes; i++)
	{
	  TrafDesc * curr_index_desc = TrafAllocateDDLdesc(DESC_INDEXES_TYPE, space);
	  
	  prev_desc->next = curr_index_desc;

	  curr_index_desc->indexesDesc()->tablename = new GENHEAP(space) char[strlen(indexInfo[i].baseTableName)+1];
	  strcpy(curr_index_desc->indexesDesc()->tablename, indexInfo[i].baseTableName);

	  curr_index_desc->indexesDesc()->indexname = new GENHEAP(space) char[strlen(indexInfo[i].indexName)+1];
	  strcpy(curr_index_desc->indexesDesc()->indexname, indexInfo[i].indexName);

          curr_index_desc->indexesDesc()->indexUID = indexInfo[i].indexUID;
	  curr_index_desc->indexesDesc()->keytag = indexInfo[i].keytag;
	  curr_index_desc->indexesDesc()->setUnique(indexInfo[i].isUnique);
	  curr_index_desc->indexesDesc()->setExplicit(indexInfo[i].isExplicit);
          curr_index_desc->indexesDesc()->record_length = 
            getRecordLength(&indexInfo[i], columnInfo);
	  curr_index_desc->indexesDesc()->colcount = indexInfo[i].keyColCount + indexInfo[i].nonKeyColCount;
	  curr_index_desc->indexesDesc()->blocksize = 32*1024;

	  curr_index_desc->indexesDesc()->keys_desc = 
	    Generator::createKeyDescs(indexInfo[i].keyColCount, indexInfo[i].keyInfoArray, space);

	  curr_index_desc->indexesDesc()->non_keys_desc = 
	    Generator::createKeyDescs(indexInfo[i].nonKeyColCount, indexInfo[i].nonKeyInfoArray, space);

	  if (CmpCommon::context()->sqlSession()->validateVolatileName(indexInfo[i].indexName))
	    curr_index_desc->indexesDesc()->setVolatile(TRUE);
	  else
	    curr_index_desc->indexesDesc()->setVolatile(FALSE);
          curr_index_desc->indexesDesc()->hbaseCreateOptions  = NULL;
          curr_index_desc->indexesDesc()->numSaltPartns = 
            indexInfo[i].numSaltPartns;
          if (curr_index_desc->indexesDesc()->numSaltPartns > 0)
          {
            // the presence of the files descriptor tells createNAFileSets
            // that the index is salted like the base table
            TrafDesc * ci_files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, space);
            ci_files_desc->filesDesc()->setAudited(TRUE); // audited table
            curr_index_desc->indexesDesc()->files_desc = ci_files_desc;
          }
          curr_index_desc->indexesDesc()->setRowFormat(indexInfo[i].rowFormat);
          if (indexInfo[i].hbaseCreateOptions)
          {
            curr_index_desc->indexesDesc()->hbaseCreateOptions  = 
              new GENHEAP(space) char[strlen(indexInfo[i].hbaseCreateOptions) + 1];
            strcpy(curr_index_desc->indexesDesc()->hbaseCreateOptions, 
               indexInfo[i].hbaseCreateOptions);
          }

	  prev_desc = curr_index_desc;

	}
    }

  TrafDesc * view_desc = NULL;
  if (numViews > 0)
    {
      view_desc = TrafAllocateDDLdesc(DESC_VIEW_TYPE, space);
      
      view_desc->viewDesc()->viewname = new GENHEAP(space) char[strlen(viewInfo[0].viewName)+1];
      strcpy(view_desc->viewDesc()->viewname, viewInfo[0].viewName);

      view_desc->viewDesc()->viewfilename = view_desc->viewDesc()->viewname;
      view_desc->viewDesc()->viewtext = new GENHEAP(space) char[strlen(viewInfo[0].viewText) + 1];
      strcpy(view_desc->viewDesc()->viewtext, viewInfo[0].viewText);

      view_desc->viewDesc()->viewtextcharset = (CharInfo::CharSet)SQLCHARSETCODE_UTF8;

      if (viewInfo[0].viewCheckText)
	{
	  view_desc->viewDesc()->viewchecktext = new GENHEAP(space) char[strlen(viewInfo[0].viewCheckText)+1];
	  strcpy(view_desc->viewDesc()->viewchecktext, viewInfo[0].viewCheckText);
	}
      else
	view_desc->viewDesc()->viewchecktext = NULL;

      if (viewInfo[0].viewColUsages)
	{
	  view_desc->viewDesc()->viewcolusages = new GENHEAP(space) char[strlen(viewInfo[0].viewColUsages)+1];
	  strcpy(view_desc->viewDesc()->viewcolusages, viewInfo[0].viewColUsages);
	}
      else
	view_desc->viewDesc()->viewcolusages = NULL;

      view_desc->viewDesc()->setUpdatable(viewInfo[0].isUpdatable);
      view_desc->viewDesc()->setInsertable(viewInfo[0].isInsertable);
    }

  TrafDesc * seq_desc = NULL;
  if (seqInfo)
    {
      seq_desc = TrafAllocateDDLdesc(DESC_SEQUENCE_GENERATOR_TYPE, space);
      
      seq_desc->sequenceGeneratorDesc()->setSgType((ComSequenceGeneratorType)seqInfo->seqType);

      seq_desc->sequenceGeneratorDesc()->fsDataType = (ComFSDataType)seqInfo->datatype;
      seq_desc->sequenceGeneratorDesc()->startValue = seqInfo->startValue;
      seq_desc->sequenceGeneratorDesc()->increment = seqInfo->increment;

      seq_desc->sequenceGeneratorDesc()->maxValue = seqInfo->maxValue;
      seq_desc->sequenceGeneratorDesc()->minValue = seqInfo->minValue;
      seq_desc->sequenceGeneratorDesc()->cycleOption = 
	(seqInfo->cycleOption ? TRUE : FALSE);
      seq_desc->sequenceGeneratorDesc()->cache = seqInfo->cache;      
      seq_desc->sequenceGeneratorDesc()->objectUID = seqInfo->seqUID;
      
      seq_desc->sequenceGeneratorDesc()->nextValue = seqInfo->nextValue;
      seq_desc->sequenceGeneratorDesc()->redefTime = seqInfo->redefTime;
    }


  // Setup the privilege descriptors for objects including views, tables, 
  // libraries, udrs, sequences, and constraints.
  TrafDesc * priv_desc = NULL;
  if (privInfo)
      priv_desc = createPrivDescs(privInfo, space);

  TrafDesc * i_files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, space);
  i_files_desc->filesDesc()->setAudited(TRUE); // audited table
  index_desc->indexesDesc()->files_desc = i_files_desc;

  index_desc->indexesDesc()->keys_desc  = keys_descs;
  table_desc->tableDesc()->columns_desc = cols_descs;
  table_desc->tableDesc()->indexes_desc = index_desc;
  table_desc->tableDesc()->views_desc = view_desc;
  table_desc->tableDesc()->constrnts_desc = first_constr_desc;
  table_desc->tableDesc()->constr_count = numConstrs; 
  table_desc->tableDesc()->sequence_generator_desc = seq_desc;
  table_desc->tableDesc()->priv_desc = priv_desc;

  if (endKeyArray)
    {
      // create a list of region descriptors
      table_desc->tableDesc()->hbase_regionkey_desc = 
        assembleDescs(endKeyArray, space);
    }

  if (snapshotName != NULL)
    {
      table_desc->tableDesc()->snapshotName = 
        new GENHEAP(space) char[strlen(snapshotName) + 1];
      strcpy(table_desc->tableDesc()->snapshotName, snapshotName);
    }
 else
    table_desc->tableDesc()->snapshotName = NULL;

  if (genPackedDesc && space)
    {
      if (! space->isComSpace() ) // to insure cast (Space *) is safe
        {
          table_desc = NULL;
          return table_desc;
        }  

      Space * trueSpace = (Space *)space; // space really is a Space

      // pack generated desc and move it to a contiguous buffer before return.
      DescStructPtr((TrafDesc*)table_desc).pack(trueSpace);
      Lng32 allocSize = trueSpace->getAllocatedSpaceSize();
      char * contigTableDesc = new HEAP char[allocSize];
 
      if (! trueSpace->makeContiguous(contigTableDesc, allocSize))
        {
          table_desc = NULL;
          return table_desc;
        }

      table_desc = (TrafDesc*)contigTableDesc;

      if (packedDescLen)
        *packedDescLen = allocSize;
    }

  return table_desc;
}

TrafDesc *Generator::createVirtualLibraryDesc(
     const char *libraryName,
     ComTdbVirtTableLibraryInfo *libraryInfo,
     Space * space)
{

  TrafDesc *library_desc = TrafAllocateDDLdesc(DESC_LIBRARY_TYPE, space);
   library_desc->libraryDesc()->libraryName = new GENHEAP(space) char[strlen(libraryName) + 1];
   strcpy(library_desc->libraryDesc()->libraryName, libraryName);
   library_desc->libraryDesc()->libraryFilename = new GENHEAP(space) char[strlen(libraryInfo->library_filename) + 1];
   strcpy(library_desc->libraryDesc()->libraryFilename, libraryInfo->library_filename);
   library_desc->libraryDesc()->libraryVersion = libraryInfo->library_version;
   library_desc->libraryDesc()->libraryUID = libraryInfo->library_UID;
   library_desc->libraryDesc()->libraryOwnerID = libraryInfo->object_owner_id;
   library_desc->libraryDesc()->librarySchemaOwnerID = libraryInfo->schema_owner_id;
   
   return library_desc;
   
}


TrafDesc *Generator::createVirtualRoutineDesc(
     const char *routineName,
     ComTdbVirtTableRoutineInfo *routineInfo,
     Int32 numParams,
     ComTdbVirtTableColumnInfo *paramsArray,
     ComTdbVirtTablePrivInfo *privInfo,
     Space * space)
{
  TrafDesc *routine_desc = TrafAllocateDDLdesc(DESC_ROUTINE_TYPE, space);
   routine_desc->routineDesc()->objectUID = routineInfo->object_uid;
   routine_desc->routineDesc()->routineName = new GENHEAP(space) char[strlen(routineName)+1];
   strcpy(routine_desc->routineDesc()->routineName, routineName);
   routine_desc->routineDesc()->externalName = new GENHEAP(space) char[strlen(routineInfo->external_name)+1];
   strcpy(routine_desc->routineDesc()->externalName, routineInfo->external_name);
   routine_desc->routineDesc()->librarySqlName = NULL; 
   routine_desc->routineDesc()->libraryFileName = new GENHEAP(space) char[strlen(routineInfo->library_filename)+1];
   strcpy(routine_desc->routineDesc()->libraryFileName, routineInfo->library_filename);
   routine_desc->routineDesc()->signature = new GENHEAP(space) char[strlen(routineInfo->signature)+1];
   strcpy(routine_desc->routineDesc()->signature, routineInfo->signature);
   routine_desc->routineDesc()->librarySqlName = new GENHEAP(space) char[strlen(routineInfo->library_sqlname)+1];
   strcpy(routine_desc->routineDesc()->librarySqlName, routineInfo->library_sqlname);
   routine_desc->routineDesc()->libRedefTime = routineInfo->lib_redef_time;
   routine_desc->routineDesc()->libBlobHandle = routineInfo->lib_blob_handle;

   routine_desc->routineDesc()->libVersion = routineInfo->library_version;
   routine_desc->routineDesc()->libObjUID = routineInfo->lib_obj_uid;
   //routine_desc->routineDesc()->libSchName = new GENHEAP(space) char[strlen(routineInfo->lib_sch_name)+1];
   //strcpy(routine_desc->routineDesc()->libSchName ,routineInfo->lib_sch_name);
   routine_desc->routineDesc()->libSchName = routineInfo->lib_sch_name;
   routine_desc->routineDesc()->language  = 
           CmGetComRoutineLanguageAsRoutineLanguage(routineInfo->language_type);
   routine_desc->routineDesc()->UDRType  = 
           CmGetComRoutineTypeAsRoutineType(routineInfo->UDR_type);
   routine_desc->routineDesc()->sqlAccess  = 
           CmGetComRoutineSQLAccessAsRoutineSQLAccess(routineInfo->sql_access);
   routine_desc->routineDesc()->transactionAttributes  = 
           CmGetComRoutineTransactionAttributesAsRoutineTransactionAttributes
                  (routineInfo->transaction_attributes);
   routine_desc->routineDesc()->maxResults = routineInfo->max_results;
   routine_desc->routineDesc()->paramStyle  = 
           CmGetComRoutineParamStyleAsRoutineParamStyle(routineInfo->param_style);
   routine_desc->routineDesc()->isDeterministic = routineInfo->deterministic;
   routine_desc->routineDesc()->isCallOnNull = routineInfo->call_on_null;
   routine_desc->routineDesc()->isIsolate = routineInfo->isolate;
   routine_desc->routineDesc()->externalSecurity  = 
           CmGetRoutineExternalSecurityAsComRoutineExternalSecurity
                  (routineInfo->external_security);
   routine_desc->routineDesc()->executionMode  = 
           CmGetRoutineExecutionModeAsComRoutineExecutionMode
                 (routineInfo->execution_mode);
   routine_desc->routineDesc()->stateAreaSize = routineInfo->state_area_size;
   routine_desc->routineDesc()->parallelism  = 
           CmGetRoutineParallelismAsComRoutineParallelism(routineInfo->parallelism);
   UInt32 reclen; 
   routine_desc->routineDesc()->paramsCount = numParams;
   routine_desc->routineDesc()->params = 
             Generator::createColDescs(routineName, 
                                       paramsArray, (Int16) numParams, reclen, 
                                       space);
   routine_desc->routineDesc()->owner = routineInfo->object_owner_id;
   routine_desc->routineDesc()->schemaOwner = routineInfo->schema_owner_id; 

  // Setup the privilege descriptors for routines.
  TrafDesc * priv_desc = NULL;
  if (privInfo)
      priv_desc = createPrivDescs(privInfo, space);
  routine_desc->routineDesc()->priv_desc = priv_desc;

   return routine_desc;
}

short Generator::genAndEvalExpr(
				CmpContext * cmpContext,
				char * exprStr, Lng32 numChildren, 
				ItemExpr * childNode0, ItemExpr * childNode1,
				ComDiagsArea * diagsArea)
{
  short rc = 0;

  Parser parser(cmpContext);
  BindWA bindWA(ActiveSchemaDB(), cmpContext);

  ItemExpr *parseTree = NULL;
  parseTree = parser.getItemExprTree(exprStr, strlen(exprStr), 
				     CharInfo::ISO88591,
				     numChildren, childNode0, childNode1); 

  if (! parseTree)
    return -1;

  parseTree->bindNode(&bindWA);
  if (bindWA.errStatus())
    return -1;

  char castValBuf[1000];
  Lng32 castValBufLen = 1000;
  Lng32 outValLen = 0;
  Lng32 outValOffset = 0;
  rc = ValueIdList::evaluateTree(parseTree, castValBuf, castValBufLen, 
				 &outValLen, &outValOffset, diagsArea);
  
  if (rc)
    return -1;

  return 0;
}

PhysicalProperty *
Generator::genPartitionedPhysProperty(const IndexDesc * clusIndex)
{
  PlanExecutionEnum plenum = EXECUTE_IN_ESP;

  PartitioningFunction *myPartFunc = NULL;
  if ((clusIndex->getPartitioningFunction()) &&
      (clusIndex->getPartitioningFunction()->isAHash2PartitioningFunction()))
    {
      Lng32 forcedEsps = 0;
      if (CmpCommon::getDefault(PARALLEL_NUM_ESPS, 0) != DF_SYSTEM)
	forcedEsps =
	  ActiveSchemaDB()->getDefaults().getAsLong(PARALLEL_NUM_ESPS);
      else
	forcedEsps = 32; // this seems to be an optimum number
      //forcedEsps = rpp->getCountOfAvailableCPUs();
      
      const Hash2PartitioningFunction * h2pf =
	clusIndex->getPartitioningFunction()->
	castToHash2PartitioningFunction();
      Lng32 numPartns = h2pf->getCountOfPartitions();
      forcedEsps = numPartns;
      if ((forcedEsps <= numPartns) &&
	  ((numPartns % forcedEsps) == 0))
	{
	  NodeMap* myNodeMap = new(CmpCommon::statementHeap())
	    NodeMap(CmpCommon::statementHeap(),
		    forcedEsps,
		    NodeMapEntry::ACTIVE);
	  
	  CollIndex entryNum = 0;
	  Int32 currNodeNum = -1;
	  Int32 i = 0;
	  while (i < forcedEsps)
	    {
	      if (entryNum == h2pf->getNodeMap()->getNumEntries())
		{
		  entryNum = 0;
		  currNodeNum = -1;
		  continue;
		}
	      
	      const NodeMapEntry * entry = 
		h2pf->getNodeMap()->getNodeMapEntry(entryNum);
	      
	      if (entry->getNodeNumber() == currNodeNum)
		{
		  entryNum++;
		  continue;
		}
	      
	      myNodeMap->setNodeMapEntry(i, *entry);
	      currNodeNum = entry->getNodeNumber();
	      
	      entryNum++;
	      i++;
	    }
	  
	  myPartFunc = new(CmpCommon::statementHeap())
	    Hash2PartitioningFunction(
		 h2pf->getPartitioningKey(),
		 h2pf->getKeyColumnList(),
		 forcedEsps, myNodeMap);
	  myPartFunc->createPartitioningKeyPredicates();
	}
    }
  
  if (myPartFunc == NULL)
    myPartFunc = clusIndex->getPartitioningFunction();


  if (!myPartFunc)
    {
      //----------------------------------------------------------
      // Create a node map with a single, active, wild-card entry.
      //----------------------------------------------------------
      NodeMap* myNodeMap = new(CmpCommon::statementHeap())
	NodeMap(CmpCommon::statementHeap(),
		1,
		NodeMapEntry::ACTIVE);
      
      //------------------------------------------------------------
      // The table is not partitioned. No need to start ESPs.
      // Synthesize a partitioning function with a single partition.
      //------------------------------------------------------------
      myPartFunc = new(CmpCommon::statementHeap())
	SinglePartitionPartitioningFunction(myNodeMap);
      plenum = EXECUTE_IN_MASTER;
    }
  
  PhysicalProperty * sppForMe = new(CmpCommon::statementHeap())
    PhysicalProperty(myPartFunc,
		     plenum,
		     SOURCE_VIRTUAL_TABLE);

  return sppForMe;
}

/////////////////////////////////////////////////////////////////
// 
// This next method helps with the fix for Soln 10-071204-9253.
// Sometimes, SORT is used as a blocking operator to make a self-ref 
// update safe from the Halloween problem.  But if the TSJforWrite and
// SORT are executed in parallel and the scan of the self-ref table is
// accessed from the same ESP as the SORT, the resulting plan 
// will not be safe, because the scans can finish asynchronously
// which will allow some SORT to return rows before other scans 
// have finished.  To prevent this, Sort::preCodeGen will call 
// this method to insert an ESP Exchange below the SORT, so that 
// none of the SORT instances will begin returning rows until
// all of the scans have finished.  There is also code in the
// preCodeGen methods of NestedJoin, Exchange, and PartitionAccess
// to help detect the need for Sort::preCodeGen to call this method.
// 
// The method is also used to add an ESP Exchange on top of a
// SequenceGenerator operator.  In this case the Exchange is actually
// treated as an ESP Access operator.
/////////////////////////////////////////////////////////////////

RelExpr *
Generator::insertEspExchange(RelExpr *oper, 
                             const PhysicalProperty *unPreCodeGendPP)
{
  GroupAttributes *ga = oper->getGroupAttr();

  // Gather some information about partitioning to allow an 
  // assertion to check the assumption that this is safe to do.
  
  PartitioningFunction *pf = unPreCodeGendPP->getPartitioningFunction();
  
  const ValueIdSet &partKeys = pf->getPartitioningKey();

  ValueId pkey;
  partKeys.getFirst(pkey);
  
  NABoolean isRandomRepart = 
    ((pf->isAHashPartitioningFunction() ||
      pf->isATableHashPartitioningFunction()) &&
     (partKeys.entries() == 1) &&
     (pkey.getItemExpr()->getOperatorType() == ITM_RANDOMNUM));

  if (isRandomRepart == FALSE)
    {
      ValueIdSet newInputs;
      ValueIdSet referencedInputs;
      ValueIdSet coveredSubExpr;
      ValueIdSet uncoveredExpr;
      NABoolean isCovered = partKeys.isCovered(newInputs,
                                       *ga,
                                       referencedInputs,
                                       coveredSubExpr,
                                       uncoveredExpr);
      //      if (isCovered == FALSE)
      //GenAssert(0, "Bad assumptions in Generator::insertEspExchange.")
    }        
 
  Exchange *exch = new (CmpCommon::statementHeap()) Exchange(oper);

  exch->setPhysicalProperty(unPreCodeGendPP);
  exch->setGroupAttr(oper->getGroupAttr());
  exch->setEstRowsUsed(oper->getEstRowsUsed());
  exch->setMaxCardEst(oper->getMaxCardEst());
  exch->setInputCardinality(exch->getInputCardinality());
  exch->setOperatorCost(0);
  exch->setRollUpCost(exch->getRollUpCost());

  // Don't let Exchange::preCodeGen eliminate this Exchange.    
  exch->doSkipRedundancyCheck();

  exch->setUpMessageBufferLength( ActiveSchemaDB()->getDefaults().getAsULong
                                 (UNOPTIMIZED_ESP_BUFFER_SIZE_UP) / 1024 );
  exch->setDownMessageBufferLength( ActiveSchemaDB()->getDefaults().getAsULong
                              (UNOPTIMIZED_ESP_BUFFER_SIZE_DOWN) / 1024 ); 

  return exch;
}

/////////////////////////////////////////////////////////////////
// Methods to manipulate the map tables
/////////////////////////////////////////////////////////////////
// appends 'map_table' to the end of the
// list of map tables.
// If no map table is passed in, allocates a new map table
// and appends it.
// Returns pointer to the maptable being added.
MapTable * Generator::appendAtEnd(MapTable * map_table)
{
  MapTable * mt = (map_table ? map_table : (new(wHeap()) MapTable()));

  if (! mt)
    return NULL;

  if (! firstMapTable_)
    {
      firstMapTable_ = mt;
      lastMapTable_  = mt;
    }
  else
    {
      mt->prev() = lastMapTable_;
      lastMapTable_->next() = mt;
      lastMapTable_ = mt;

      while (lastMapTable_->next())
	lastMapTable_ = lastMapTable_->next();
    }

  return mt;
}

// searches for value_id in the list of map tables.
// If mapTable is input, starts the search from there.
// Returns MapInfo, if found.
// Raises assertion, if not found.
MapInfo * Generator::getMapInfo(const ValueId & value_id, MapTable * mapTable)
{
  MapInfo * mi = getMapInfoAsIs(value_id, mapTable);
  if (mi)
    return mi;

  // value not found. Assert.
  NAString unparsed(wHeap());
  value_id.getItemExpr()->unparse(unparsed);
  char errmsg[200];
  sprintf(errmsg, "\nValueId %d (%.100s...) not found in MapTable %p",
	  (CollIndex)value_id, unparsed.data(), this);
  GenAssert(0, errmsg);

  return NULL;
}

// searches for value_id in the list of map tables.
// If mapTable is input, starts the search from there.
// Returns MapInfo, if found.
// Returns NULL, if not found.
MapInfo * Generator::getMapInfoAsIs(const ValueId & value_id,
				    MapTable * mapTable)
{
  // first look for this value_id in the last map table. There
  // is a good chance it will be there.
  MapInfo * mi =
    ((getLastMapTable()->getTotalVids() > 0) ?
     getLastMapTable()->getMapInfoFromThis(value_id) :
     NULL);
  if (mi)
    return mi;

  // now search all the map tables.
  // Do not look in the last map table as we have already searched it.

  if ((!mapTable) && (getLastMapTable() == getMapTable()))
    return NULL ;
  MapTable * mt = (mapTable ? mapTable : getLastMapTable()->prev());
  while (mt)
    {
      if (mt->getTotalVids() > 0)
	{
	  mi = mt->getMapInfoFromThis(value_id);
	  if (mi)
	    return mi;
	}
      if (mt != getMapTable())
	mt = mt->prev();
      else
	break ;
    }

  return NULL;
}

// gets MapInfo from mapTable.
// Raises assertion, if not found.
MapInfo * Generator::getMapInfoFromThis(MapTable * mapTable,
					const ValueId & value_id)
{
  return mapTable->getMapInfoFromThis(value_id);
}


// adds to the last maptable, if value doesn't exist.
// Returns the MapInfo, if that value exists.
MapInfo * Generator::addMapInfo(const ValueId & value_id,
				Attributes * attr)
{
  MapInfo * map_info;

  // return the map information, if already been added to the map table.
  if (map_info = getMapInfoAsIs(value_id))
    return map_info;

  return getLastMapTable()->addMapInfoToThis(value_id, attr);
}

// adds to input mapTable. Does NOT check if the value exists.
// Caller should have checked for that.
// Returns the MapInfo for the added value.
MapInfo * Generator::addMapInfoToThis(MapTable * mapTable,
				      const ValueId & value_id,
				      Attributes * attr)
{
  return mapTable->addMapInfoToThis(value_id, attr);
}

// deletes ALL maptables starting at 'next' of inMapTable.
// If inMapTable is NULL, removes all map tables in generator.
// Makes inMapTable the last map table.
void Generator::removeAll(MapTable * inMapTable)
{
  MapTable *moreToDelete = (inMapTable ? inMapTable->next() : firstMapTable_);
  MapTable *me;

  while (moreToDelete)
    {
      me = moreToDelete;
      moreToDelete = moreToDelete->next();

      me->next() = NULL;  // no dangling pointer
      delete me;
    }

  if (inMapTable)
    {
      inMapTable->next() = NULL;
      lastMapTable_ = inMapTable;
    }
  else
    {
      firstMapTable_ = NULL;
      lastMapTable_ = NULL;
    }
}

// removes the last map table in the list.
void Generator::removeLast()
{
  if (!lastMapTable_)
    return;

  MapTable * newLastMapTable = lastMapTable_->prev();
  delete lastMapTable_;
  lastMapTable_ = newLastMapTable;

  if (lastMapTable_)
    lastMapTable_->next() = NULL;
}

// unlinks the next mapTable in the list and returns it.
// Makes mapTable the last map table.
// Does not delete the next map table.
void Generator::unlinkNext(MapTable * mapTable)
{
  if (mapTable == NULL)
    return;

  MapTable * mt = mapTable->next();
  mapTable->next() = NULL;
  lastMapTable_ = mapTable;

  // return mt;
}

// unlinks the last mapTable in the list and returns it.
MapTable * Generator::unlinkLast()
{
  if (!lastMapTable_)
    return NULL;

  MapTable * lastMapTable = lastMapTable_;
  lastMapTable_ = lastMapTable_->prev();
  lastMapTable_->next() = NULL;

  return lastMapTable;
}

// unlinks the this mapTable in the list, and whatever follows
void Generator::unlinkMe(MapTable * mapTable)
{
  if (mapTable == NULL)
    return;

  MapTable * mt = mapTable->prev();
  if (mt != NULL) 
  {
    mapTable->prev() = NULL;
    mt->next() = NULL;
    lastMapTable_ = mt;
  } 
  else if ( mapTable == firstMapTable_ )
  {
    lastMapTable_ = NULL;
    firstMapTable_ = NULL;
  }
  // else do nothing, we are not on a chain.

  // return mt;
}


void Generator::setMapTable(MapTable * map_table_)
{
  firstMapTable_ = map_table_;
  lastMapTable_ = firstMapTable_;

  if (lastMapTable_)
    while (lastMapTable_->next())
      lastMapTable_ = lastMapTable_->next();
}

const NAString Generator::genGetNameAsAnsiNAString(const QualifiedName& qual)
{
  return qual.getQualifiedNameAsAnsiString();
}

const NAString Generator::genGetNameAsAnsiNAString(const CorrName& corr)
{
  return genGetNameAsAnsiNAString(corr.getQualifiedNameObj());
}

const NAString Generator::genGetNameAsNAString(const QualifiedName& qual)
{
  return qual.getQualifiedNameAsString();
}

const NAString Generator::genGetNameAsNAString(const CorrName& corr)
{
  // This warning is wrong, or at least misleading.  True, at this call we
  // are ignoring (losing) host-variable name, but at a later point in
  // CodeGen the proper hv linkage is set up for correct Late Name Resolution.
  //
  //   HostVar *proto = corr.getPrototype();
  //   if (proto)
  //     cerr << "*** WARNING[] Ignoring variable " << proto->getName()
  //         << ", using prototype value "
  //         << GenGetQualifiedName(corr.getQualifiedNameObj()) << endl;

  return genGetNameAsNAString(corr.getQualifiedNameObj());
}

// returns attributes corresponding to itemexpr ie.
// If clause has already been generated for this ie, then returns
// attribute from operand 0 (result) of clause.
// If not, searches the map table and returns it from there.
Attributes * Generator::getAttr(ItemExpr * ie)
{
  if ((getExpGenerator()->clauseLinked()) && (ie->getClause()))
    return ie->getClause()->getOperand(0);
  else
    return getMapInfo(ie->getValueId())->getAttr();
}

void Generator::addTrafSimTableInfo(TrafSimilarityTableInfo *newST)
{
  for (CollIndex i = 0; i < getTrafSimTableInfoList().entries(); i++)
    {
      TrafSimilarityTableInfo *ti = 
        (TrafSimilarityTableInfo*)getTrafSimTableInfoList()[i];
      if (*ti == *newST)
        {
          // value exists, do not add.
          return;
        }
    }

  getTrafSimTableInfoList().insert(newST);
}

// Helper method used by caching operators to ensure a statement
// execution count is included in their characteristic input.
// The "getOrAdd" semantic is to create the ValueId if it doesn't
// exist, otherwise return a preexisting one.
ValueId Generator::getOrAddStatementExecutionCount()
{
  ValueId execCount;

  for (ValueId x = internalInputs_.init();
       internalInputs_.next(x);
       internalInputs_.advance(x)) {
        if (x.getItemExpr()->getOperatorType() == ITM_EXEC_COUNT) {
          execCount = x;
          break;
        }
  }

  if (execCount == NULL_VALUE_ID)  {
    // nobody has asked for an execution count before, create
    // a new item expression and add it to the internal input
    // values that are to be generated by the root node
    ItemExpr *ec = new(wHeap()) StatementExecutionCount();
    ec->bindNode(getBindWA());
    execCount = ec->getValueId();
    internalInputs_ += execCount;
  }

  return execCount;
}

bool Generator::setPrecodeHalloweenLHSofTSJ(bool newVal)
{
  bool oldVal = precodeHalloweenLHSofTSJ_;
  precodeHalloweenLHSofTSJ_ = newVal;
  return oldVal;
}

bool Generator::setPrecodeRHSofNJ(bool newVal)
{
  bool oldVal = precodeRHSofNJ_;
  precodeRHSofNJ_ = newVal;
  return oldVal;
}

void Generator::setPlanExpirationTimestamp(Int64 t) 
{
  // if t == -1 that has no effect (initial default is -1)
  // Otherwise, use the smaller of planExpirationTimestamp_ and t
  if (t >= 0 && planExpirationTimestamp_ > t)
    planExpirationTimestamp_ = t;
}

////////////////////////////////////////////////////////////////////
// the next comment lines are historical and are not relevant
// any more.
////////////////////////////////////////////////////////////////////
// ##
// For three-part ANSI names (truly qualified names),
// these functions should return NAString (non-ref)
// which is concatenation of
//   getCatalogName() + getSchemaName + getObjectName()
// with a fixed number of bytes of length preceding each field
// -- or some other encoding of a three-part name such that the
// three parts are clearly delineated/unambiguous
// (parsing by dots is no good, since delimited names can have dots).
// -- or perhaps we need to return three separate strings?
// ## But for now, this will do since ANSI names aren't in the 1996 EAP.
//
// ##
// Callers of these functions need to save this corr-to-hostvar linkage
// somewhere (callers passing in non-Ansi-name indexdesc's and fileset names
// need to save an additional item of context to indicate what kind of
// path or role it is -- i.e. Ansi tablename, raw physical partition name of
// a table, Ansi or physical name of an index, etc.).
//
// Upon finding a :hv, Executor must do:
// If hostvar
//   Get the user input value from the variable
// Else (envvar)
//   Do a getenv on the name
//   If no current value and no saved value, error
//   If no current value, then local :hv = saved value, else :hv = current
// Parse value in :hv, resulting in QualifiedName object
// Do a Catalog Manager lookup of this QName (Executor<->CatMan messages)
// Traverse the NATable (or whatever) catalog info tree to find the correct
// physical name for the role/path/context to be opened.
// Open the file.
//
// Eventually (not in first release), we need to save Similarity Check
// info of the prototype table, so that at run-time,
// after the catalog info fetch, and probably also after the open,
// the Similarity Check(s) must be done.
//
// Caching can be done to create a fastpath from var name straight to
// previously-opened file (or at least part of the way, to prev-fetched
// catalog info, if this is a different role).  Worth doing if it cuts
// out the overhead of repeated catalog reads.
//
// Parsing: can reuse/clone some of the code from CorrName::applyPrototype
// and applyDefaults -- for name-part defaulting, it makes sense to me to
// determine which defaults to use the same way as dynamic SQL compilations:
// look first if any dynamic Ansi defaults (SET SCHEMA),
// next at static Tandem extension defaults (DECLARE SCHEMA),
// finally at the (always present) schema and catalog of the module
// (compilation unit) this query is part of.
//
// I assume we can't have all the heavy machinery of the entire Parser
// to be linked into the Executor for just this.  But we do want something
// robust enough to handle identifiers (both regular and delimited)
// and names (one, two, and three-part) in all their glory and idiosyncrasy
// (character sets, valid initial characters, treatment of blank space, etc).
// I think we should move the ShortStringSequence and xxxNameFromStrings and
// transformIdentifier code out of SqlParser.y -- make a suitable .h file
// for public interface -- write a teeny parser just for this problem,
// based on that abstracted code -- and have Executor call that.
// ##
//

const NAString GenGetQualifiedName(const CorrName& corr,
				   NABoolean formatForDisplay)
{
  return corr.getQualifiedNameObj().getQualifiedNameAsString(formatForDisplay);
}


void GeneratorAbort(const char *file, Int32 line, const char * message)
{

  SQLMXLoggingArea::logSQLMXAssertionFailureEvent(file, line, message);

#ifdef _DEBUG
  *CmpCommon::diags() << DgSqlCode(-7000) << DgString0(file)
                      << DgInt0(line) << DgString1(message);

  abort();
  CmpInternalException("GeneratorAbort", __FILE__ , __LINE__).throwException();
#else
  if (CmpCommon::context()->isDoNotAbort())
  {
    *CmpCommon::diags() << DgSqlCode(-7000) << DgString0(file)
                      << DgInt0(line) << DgString1(message);

    CmpInternalException("GeneratorAbort", __FILE__ , __LINE__).throwException();
  }
  else
     NAExit(1);
#endif
}


void GeneratorExit(const char *file, Int32 line)
{
  UserException(file,line).throwException();
}
/*****************************
 Determine the tuple data format to use based on some heuristics.
 and whether we want to resize or not
 this functions determines the tuple format that we need to for a rel expression
 based on a value Id list which constitutes the row.
 the paramaters and explanatios are as folows:
 const ValueIdList & valIdList, --> this the row we are exemining
 RelExpr * relExpr, --> rel expression that is trying to determine the tuple format
 NABoolean & resizeCifRecord,  --> should we resize the cif record or not -- if exploded format
                                   this boolean ill be set to FALSE
                                     //otherwiase it is TRUE if there varchars FALSE if not
 RelExpr::CifUseOptions bmo_cif, bmo (relexpr's) CIF seeting on , off or system
 NABoolean bmo_affinity,-->if TRUE then the rel expr will use the same tuple format as query one
 UInt32 & alignedLength, --> length of the row in aligned format
 UInt32 & explodedLength, --> length of the row in exploded format
 UInt32 & alignedVarCharSize, --> length of the vachar fields
 UInt32 & alignedHeaderSize, --> length of the header
 double & avgVarCharUsage) --> combined average varchar usgae of all the varchar feilds based
                              on stats info if available

*/
ExpTupleDesc::TupleDataFormat Generator::determineInternalFormat( const ValueIdList & valIdList,
                                                                 RelExpr * relExpr,
                                                                 NABoolean & resizeCifRecord,
                                                                 RelExpr::CifUseOptions bmo_cif,
                                                                 NABoolean bmo_affinity,
                                                                 UInt32 & alignedLength,
                                                                 UInt32 & explodedLength,
                                                                 UInt32 & alignedVarCharSize,
                                                                 UInt32 & alignedHeaderSize,
                                                                 double & avgVarCharUsage,
                                                                 UInt32 prefixLength)

{

  if (bmo_cif == RelExpr::CIF_OFF)
  {
    resizeCifRecord = FALSE;
    return ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  }

  resizeCifRecord = valIdList.hasVarChars();

  if (bmo_cif == RelExpr::CIF_ON)
  {
    return ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  }

  DCMPASSERT (bmo_cif == RelExpr::CIF_SYSTEM);

  //when bmo affinity is true ==> use the same tuple format as the main one
  //valid when bmo cif is sett o system only
  if (bmo_affinity == TRUE)
  {
    if (getInternalFormat() == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
    {
      return  ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
    }
    else
    {
      DCMPASSERT(getInternalFormat() == ExpTupleDesc::SQLARK_EXPLODED_FORMAT);
      resizeCifRecord = FALSE;
      return  ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
    }
  }

  // The aligned and exploded row length and aligned row length and  varchar size
  // are computed first --
  alignedHeaderSize= 0;
  alignedVarCharSize = 0;
  explodedLength = 0;
  alignedLength = 0;

  ExpGenerator * exp_gen = getExpGenerator();

  //compute the aligned row  length, varchar size and header size
  exp_gen->computeTupleSize(valIdList,
                             ExpTupleDesc::SQLMX_ALIGNED_FORMAT,
                             alignedLength,
                             0,
                             &alignedVarCharSize,
                             &alignedHeaderSize);

  alignedLength += prefixLength;

  DCMPASSERT(resizeCifRecord == (alignedVarCharSize > 0));

  // compute the exploded format row size also
  exp_gen->computeTupleSize(valIdList,
                             ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
                             explodedLength,
                             0);
  explodedLength += prefixLength;


  // use exploded format if row size is less than a predefined Min row size....
  // this parameter is set to a low number like 8 or less
  // the idea is that is a row is very shor than using CIF may not be a good idea because
  // of the aligned format header
  if (alignedLength < CmpCommon::getDefaultNumeric(COMPRESSED_INTERNAL_FORMAT_MIN_ROW_SIZE) ||
      explodedLength < CmpCommon::getDefaultNumeric(COMPRESSED_INTERNAL_FORMAT_MIN_ROW_SIZE))
  {
    resizeCifRecord = FALSE;
    return ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  }

  double cifRowSizeAdj = CmpCommon::getDefaultNumeric(COMPRESSED_INTERNAL_FORMAT_ROW_SIZE_ADJ);
  if (resizeCifRecord == FALSE)
  {
    if (alignedLength < explodedLength * cifRowSizeAdj)
    {
      return ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
    }
    else
    {
      //resizeCifRecord is false
      return ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
    }
  }


  double cumulAvgVarCharSize = 0;
  UInt32 CumulTotVarCharSize = 0;
  avgVarCharUsage = 1;

  // compute the average varchar size usage for the whole row based on individuka fileds average
  // varchar usages
  //resizeCifRecord is TRUE

  CollIndex numEntries = valIdList.entries();

  for( CollIndex i = 0; i < numEntries; i++ )
  {
    if (valIdList.at(i).getType().getVarLenHdrSize()>0)
    {
      double avgVarCharSize = 0;
      ValueId vid = valIdList.at(i);

      if (!findCifAvgVarCharSizeToCache(vid, avgVarCharSize))// do we need the cache???
      {
        avgVarCharSize = relExpr->getGroupAttr()->getAverageVarcharSize( valIdList.at(i));
        addCifAvgVarCharSizeToCache( vid, avgVarCharSize);
      }

      if (avgVarCharSize >0)
      {
        cumulAvgVarCharSize += avgVarCharSize;
      }
      else
      {
        cumulAvgVarCharSize += valIdList.at(i).getType().getTotalSize() * avgVarCharUsage;
      }
      CumulTotVarCharSize += valIdList.at(i).getType().getTotalSize();
    }
  }

  if (CumulTotVarCharSize > 0 )
    avgVarCharUsage = cumulAvgVarCharSize / CumulTotVarCharSize;

  UInt32 alignedNonVarSize = alignedLength - alignedVarCharSize;

  // if cumulltative var char size > header size and ...  use aligned format
  if (alignedVarCharSize > alignedHeaderSize &&
      (alignedNonVarSize + avgVarCharUsage * alignedVarCharSize <
       explodedLength * cifRowSizeAdj))
  {
    return ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  }

  resizeCifRecord = FALSE;
  return ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
}

ExpTupleDesc::TupleDataFormat Generator::determineInternalFormat( const ValueIdList & valIdList,
                                                                 RelExpr * relExpr,
                                                                 NABoolean & resizeCifRecord,
                                                                 RelExpr::CifUseOptions bmo_cif,
                                                                 NABoolean bmo_affinity,
                                                                 NABoolean & considerDefrag,
                                                                 UInt32 prefixLength)

{
  considerDefrag = FALSE;
  resizeCifRecord = FALSE;
  if (bmo_cif == RelExpr::CIF_OFF)
  {
    return ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  }

  UInt32 alignedHeaderSize= 0;
  UInt32 alignedVarCharSize = 0;
  UInt32 explodedLength = 0;
  UInt32 alignedLength = 0;
  double avgVarCharUsage = 1;
  ExpTupleDesc::TupleDataFormat tf = determineInternalFormat( valIdList,
                                                    relExpr,
                                                    resizeCifRecord,
                                                    bmo_cif,
                                                    bmo_affinity,
                                                    alignedLength,
                                                    explodedLength,
                                                    alignedVarCharSize,
                                                    alignedHeaderSize,
                                                    avgVarCharUsage,
                                                    prefixLength);

  if (relExpr)
  {
    considerDefrag = considerDefragmentation( valIdList,
                                            relExpr->getGroupAttr(),
                                            resizeCifRecord,
                                            prefixLength);
  }
  return tf;

}

NABoolean Generator::considerDefragmentation( const ValueIdList & valIdList,
                                              GroupAttributes * gattr,
                                              NABoolean  resizeCifRecord,
                                              UInt32 prefixLength)

{
  if (resizeCifRecord &&  CmpCommon::getDefaultNumeric(COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO) > 1)
    return TRUE;

  if (!resizeCifRecord || !gattr)
    return FALSE;

  NABoolean considerDefrag = FALSE;

  //determine if we wantto defragment the buffers or not using the average row size stats
  UInt32 maxRowSize=0;
  double avgRowSize = gattr->getAverageVarcharSize(valIdList, maxRowSize);
  if ( maxRowSize >0 &&
      (prefixLength + avgRowSize)/(prefixLength + maxRowSize) < CmpCommon::getDefaultNumeric(COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO))
  {
    considerDefrag = TRUE;
  }

  return considerDefrag;
}

void Generator::setHBaseNumCacheRows(double estRowsAccessed,
                                     ComTdbHbaseAccess::HbasePerfAttributes * hbpa,
                                     Int32 hbaseRowSize,
                                     Float32 samplePercent
                                     )
{
 
  // compute the number of rows accessed per scan node instance and use it
  // to set HBase scan cache size (in units of number of rows). This cache
  // is in the HBase client, i.e. in the java side of 
  // master executor or esp process. Using this cache avoids RPC calls to the
  // region server for each row, however setting the value too high consumes 
  // memory and can lead to timeout errors as the RPC call that gets a 
  // big chunk of rows can take longer to complete.
  CollIndex myId = getFragmentDir()->getCurrentId();
  Lng32 numProcesses = getFragmentDir()->getNumESPs(myId);
  Lng32 cacheMin = CmpCommon::getDefaultNumeric(HBASE_NUM_CACHE_ROWS_MIN);
  Lng32 cacheMax = CmpCommon::getDefaultNumeric(HBASE_NUM_CACHE_ROWS_MAX);
  if (numProcesses == 0)
    numProcesses++;
  UInt32 rowsAccessedPerProcess = ceil(estRowsAccessed/numProcesses) ;
  Lng32 cacheRows;
  if (rowsAccessedPerProcess < cacheMin)
    cacheRows = cacheMin;
  else if (rowsAccessedPerProcess < cacheMax)
    cacheRows = rowsAccessedPerProcess;
  else
    cacheRows = cacheMax;

  // Reduce the scanner cache if necessary based on the sampling rate (usually
  // only for Update Stats) so that it will return to the client once for every
  // USTAT_HBASE_SAMPLE_RETURN_INTERVAL rows on average. This avoids long stays
  // in the region server (and a possible OutOfOrderScannerNextException), where
  // a random row filter is used for sampling.
  if (cacheRows > cacheMin && samplePercent > 0.0)
  {
    ULng32 sampleReturnInterval =
        ActiveSchemaDB()->getDefaults().getAsULong(USTAT_HBASE_SAMPLE_RETURN_INTERVAL);
    Lng32 newScanCacheSize = (Lng32)(sampleReturnInterval * samplePercent);
    if (newScanCacheSize < cacheRows)
      {
        if (newScanCacheSize >= cacheMin)
          cacheRows = newScanCacheSize;
        else
          cacheRows = cacheMin;
      }
  }

  // Limit the scanner cache size to a fixed number if we are dealing with
  // very wide rows eg rows with varchar(16MB)
  Int32 maxRowSizeInCache = CmpCommon::getDefaultNumeric(TRAF_MAX_ROWSIZE_IN_CACHE)*1024*1024;
  if (hbaseRowSize > maxRowSizeInCache)
    cacheRows = 2;
  hbpa->setNumCacheRows(cacheRows);
}

void Generator::setHBaseCacheBlocks(Int32 hbaseRowSize, double estRowsAccessed,
                                    ComTdbHbaseAccess::HbasePerfAttributes * hbpa)
{
  if (CmpCommon::getDefault(HBASE_CACHE_BLOCKS) == DF_ON)
    hbpa->setCacheBlocks(TRUE);
  else if (CmpCommon::getDefault(HBASE_CACHE_BLOCKS) == DF_SYSTEM)
  {
    float frac = ActiveSchemaDB()->getHbaseBlockCacheFrac();
    float regionServerCacheMemInMB = 
      frac*getDefaultAsLong(HBASE_REGION_SERVER_MAX_HEAP_SIZE);
    float memNeededForScanInMB = (hbaseRowSize*estRowsAccessed)/(1024*1024);
    if (regionServerCacheMemInMB > memNeededForScanInMB)
      hbpa->setCacheBlocks(TRUE);
  }
}

void Generator::setHBaseSmallScanner(Int32 hbaseRowSize, double estRowsAccessed,
                      Lng32 hbaseBlockSize, ComTdbHbaseAccess::HbasePerfAttributes * hbpa)
{
  if (CmpCommon::getDefault(HBASE_SMALL_SCANNER) == DF_SYSTEM)
  {
    if(((hbaseRowSize*estRowsAccessed)<hbaseBlockSize) && (estRowsAccessed>0))//added estRowsAccessed > 0 because MDAM costing is not populating this field correctly
        hbpa->setUseSmallScanner(TRUE);
    hbpa->setUseSmallScannerForProbes(TRUE);
  }else if (CmpCommon::getDefault(HBASE_SMALL_SCANNER) == DF_ON)
  {
      hbpa->setUseSmallScanner(TRUE);
      hbpa->setUseSmallScannerForProbes(TRUE);
  }
  hbpa->setMaxNumRowsPerHbaseBlock(hbaseBlockSize/hbaseRowSize);
}

void Generator::setHBaseParallelScanner(ComTdbHbaseAccess::HbasePerfAttributes * hbpa){
    hbpa->setDopParallelScanner(CmpCommon::getDefaultNumeric(HBASE_DOP_PARALLEL_SCANNER));
}

double Generator::getEstMemPerNode(NAString *key, Lng32 &numStreams)
{
  OperBMOQuota *operBMOQuota = bmoQuotaMap_.get(key); 
  if (operBMOQuota != NULL) {
     numStreams = operBMOQuota->getNumStreams();
     return operBMOQuota->getEstMemPerNode();
  } else {
     numStreams = 0;
     return 0;
  }
}

double Generator::getEstMemForTdb(NAString *key)
{
  OperBMOQuota *operBMOQuota = bmoQuotaMap_.get(key); 
  if (operBMOQuota != NULL) 
     return operBMOQuota->getEstMemForTdb();
  else
     return 0;
}

double Generator::getEstMemPerInst(NAString *key)
{
  OperBMOQuota *operBMOQuota = bmoQuotaMap_.get(key); 
  if (operBMOQuota != NULL) 
     return operBMOQuota->getEstMemPerInst();
  else
     return 0;
}

void Generator::finetuneBMOEstimates()
{
   if (bmoQuotaMap_.entries() == 1)
      return;
   double bmoMemoryLimitPerNode = ActiveSchemaDB()->getDefaults().getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB);
   if (bmoMemoryLimitPerNode == 0)
      return;
   NAHashDictionaryIterator<NAString, OperBMOQuota> iter (bmoQuotaMap_) ;

   double capMemoryRatio = ActiveSchemaDB()->getDefaults().getAsDouble(BMO_MEMORY_ESTIMATE_RATIO_CAP);
   double bmoMemoryEstOutlier = 
      ActiveSchemaDB()->getDefaults().getAsDouble(BMO_MEMORY_ESTIMATE_OUTLIER_FACTOR) * bmoMemoryLimitPerNode * 1024 * 1024;

   double totalEstMemPerNode = totalBMOsMemoryPerNode_.value();
   double bmoMemoryRatio;
   double calcTotalEstMemPerNode = 0;
   double calcOperEstMemPerNode;

   NAString* key;
   OperBMOQuota *operBMOQuota;
   // Find the outliers and set it to the tolerable value first
   iter.reset(); 
   iter.getNext(key,operBMOQuota);
   while(key) {
     calcOperEstMemPerNode = operBMOQuota->getEstMemPerNode();
     if (calcOperEstMemPerNode > bmoMemoryEstOutlier) {
        operBMOQuota->setEstMemPerNode(bmoMemoryEstOutlier);
        calcTotalEstMemPerNode += bmoMemoryEstOutlier;
     }
     else 
       calcTotalEstMemPerNode += calcOperEstMemPerNode;
     iter.getNext(key,operBMOQuota);
   }
   totalBMOsMemoryPerNode_ = calcTotalEstMemPerNode;   
   
   // Then check for the CAP to adjust it again
   totalEstMemPerNode = totalBMOsMemoryPerNode_.value();
   calcTotalEstMemPerNode = 0;
   iter.reset();
   iter.getNext(key,operBMOQuota);
   while(key) {
     calcOperEstMemPerNode = operBMOQuota->getEstMemPerNode();
     bmoMemoryRatio = calcOperEstMemPerNode / totalEstMemPerNode;
     if (capMemoryRatio > 0 && capMemoryRatio <=1 && bmoMemoryRatio > capMemoryRatio) {
        bmoMemoryRatio = capMemoryRatio;
        calcOperEstMemPerNode = bmoMemoryRatio * calcOperEstMemPerNode;
        operBMOQuota->setEstMemPerNode(calcOperEstMemPerNode);
        calcTotalEstMemPerNode += calcOperEstMemPerNode;
     }
     else
        calcTotalEstMemPerNode += calcOperEstMemPerNode;
     iter.getNext(key,operBMOQuota);
   }
   totalBMOsMemoryPerNode_ = calcTotalEstMemPerNode;   
}


