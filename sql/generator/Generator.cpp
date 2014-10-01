/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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
Generator::Generator(CmpContext* currentCmpContext)
{
  currentCmpContext_ = currentCmpContext;
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

  updateWithinCS_ = FALSE;

  isInternalRefreshStatement_ = FALSE;

  savedGenLeanExpr_ = FALSE;

  lruOperation_ = FALSE;

  queryUsesSM_ = FALSE;

  genSMTag_ = 0;

  tempSpace_ = NULL;

  downrevCompileMXV_ = COM_VERS_CURR_PLAN;

  numBMOs_ = 0;
  totalNumBMOsPerCPU_ = 0;

  BMOsMemoryPerFrag_ = 0;
  totalBMOsMemoryPerCPU_ = 0;

  nBMOsMemoryPerCPU_ = 0;

  BMOsMemoryLimitPerCPU_ = 0;

  totalNumBMOsPerCPU_ = 0;

  BMOsMemoryPerFrag_ = 0;
  totalBMOsMemoryPerCPU_ = 0;

  nBMOsMemoryPerCPU_ = 0;

  BMOsMemoryLimitPerCPU_ = 0;
  
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
  totalOverflowMemory_ = 0.0;
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

  NAString NExLogPth ;
  CmpCommon::getDefault(PCODE_NE_LOG_PATH, NExLogPth, FALSE);
  Int32 logPathLen = NExLogPth.length() ;
  if ( logPathLen > 0 )
  {
    strncpy( &NExLogPathNam_[0], NExLogPth.data(), logPathLen );
    NExLogPathNam_[logPathLen] = '\0';
  }

  if ( NExLogPathNam_[0] != '\0' )
     NExDbgInfoObj_.setNExLogPath( &NExLogPathNam_[0] );
  else
     NExDbgInfoObj_.setNExLogPath( NULL );

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

  if (getRightSideOfOnlj() && makeOnljRightQueuesBig_)
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
     tdb->getNodeType() != ComTdb::ex_EID_ROOT &&
     (ActiveSchemaDB()->getDefaults().getToken(USE_LARGE_QUEUES) == DF_ON)) {

    tdb->setQueueResizeParams(largeQueueSize_,
                              largeQueueSize_,
                              queueResizeLimit_,
                              queueResizeFactor_);
  }

  tdb->setTdbId(getAndIncTdbId());

  // set plan version to R2, if downrev compile needed.
  // After R2.1 or whenever full versioning support is added,
  // the next 3 lines should be removed.
  if (downrevCompileNeeded())
    {
      tdb->setPlanVersion(getDownrevCompileMXV());
    }
  else   
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
      else if (tmp == "MEASURE")
	collectStatsType_ = ComTdb::MEASURE_STATS;
      else if (tmp == "PERTABLE")
	collectStatsType_ = ComTdb::PERTABLE_STATS;
      else if (tmp == "OPERATOR")
	collectStatsType_ = ComTdb::OPERATOR_STATS;
      else
	computeStats_ = FALSE;
    }

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
       (expr_node->child(0)->getOperatorType() == REL_SP_PROXY)))
      computeStats_ = FALSE;
   if (expr_node->child(0) &&
    (expr_node->child(0)->getOperatorType() == REL_EXE_UTIL &&
	((((ExeUtilExpr*)expr_node->child(0)->castToRelExpr())->getExeUtilType() == 
	 ExeUtilExpr::GET_STATISTICS_) ||
        (((ExeUtilExpr*)expr_node->child(0)->castToRelExpr())->getExeUtilType() == 
	 ExeUtilExpr::DISPLAY_EXPLAIN_))))
      computeStats_ = FALSE;
   
#ifdef _DEBUG
  if (getenv("NO_DETAILED_STATS"))
    computeStats_ = FALSE;
#endif

  setUpdatableSelect(((RelRoot *)expr_node)->updatableSelect());

  if (((RelRoot *)expr_node)->downrevCompileNeeded())
    {
      setDownrevCompileMXV(((RelRoot *)expr_node)->getDownrevCompileMXV());
    }

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
	      if ((val) && (*val == "ON"))
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

CollHeap* Generator::wHeap()
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


// map ESPs randomly
void
Generator::remapESPAllocation()
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

desc_struct* Generator::createColDescs(
  const char * tableName,
  ComTdbVirtTableColumnInfo * columnInfo,
  Int16 numCols,
  UInt32 &offset)
{
  if (! columnInfo)
    return NULL;

  desc_struct * first_col_desc = NULL;
  desc_struct * prev_desc = NULL;
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
      desc_struct * col_desc = readtabledef_make_column_desc(
                                    tableName,
                                    info->colName,
                                    i,
                                    info->datatype,
                                    info->length,
                                    tmpOffset,
                                    (short) info->nullable,
                                    (NABoolean) FALSE,    // tablenameMustBeAllocated = FALSE
                                    (desc_struct *) NULL, // passedDesc = NULL
                                    info_charset);

      // Virtual tables use SQLARK_EXPLODED_FORMAT in which numeric column
      // values are aligned.  Ignore readtabledef_make_column_desc's
      // offset calculation which doesn't reflect column value alignment.
      offset = colOffset + info->length;

      // EXPLAIN__ table uses 22-bit precision REAL values
      if (info->datatype == REC_FLOAT32)
        col_desc->body.columns_desc.precision = 22;

      col_desc->body.columns_desc.precision = info->precision;
      if (DFS2REC::isInterval(info->datatype))
	col_desc->body.columns_desc.intervalleadingprec = info->precision;

      col_desc->body.columns_desc.scale = info->scale;
      if ((DFS2REC::isInterval(info->datatype)) ||
	  (DFS2REC::isDateTime(info->datatype)))
	col_desc->body.columns_desc.datetimefractprec = info->scale;

      col_desc->body.columns_desc.datetimestart = (rec_datetime_field)info->dtStart;
      col_desc->body.columns_desc.datetimeend = (rec_datetime_field)info->dtEnd;

      col_desc->body.columns_desc.upshift = (info->upshifted ? 1 : 0);
      col_desc->body.columns_desc.caseinsensitive = (short)FALSE;

      col_desc->body.columns_desc.pictureText = new HEAP char[340];
      NAType::convertTypeToText(col_desc->body.columns_desc.pictureText,//OUT
				col_desc->body.columns_desc.datatype,
				col_desc->body.columns_desc.length,
				col_desc->body.columns_desc.precision,
				col_desc->body.columns_desc.scale,
				col_desc->body.columns_desc.datetimestart,
				col_desc->body.columns_desc.datetimeend,
				col_desc->body.columns_desc.datetimefractprec,
				col_desc->body.columns_desc.intervalleadingprec,
				col_desc->body.columns_desc.upshift,
				col_desc->body.columns_desc.caseinsensitive,
				(CharInfo::CharSet)col_desc->body.columns_desc.character_set,
				(CharInfo::Collation)col_desc->body.columns_desc.collation_sequence,
				NULL
				);

      col_desc->body.columns_desc.defaultClass  = info->defaultClass;
      if (info->defaultClass == COM_NO_DEFAULT)
	col_desc->body.columns_desc.defaultvalue = NULL;
      else
	col_desc->body.columns_desc.defaultvalue = (char*)info->defVal;

      col_desc->body.columns_desc.colclass = 'U';
      col_desc->body.columns_desc.addedColumn = 0;
      if (strcmp(info->columnClass, COM_SYSTEM_COLUMN_LIT) == 0)
	col_desc->body.columns_desc.colclass = 'S';
      else if (strcmp(info->columnClass, COM_ADDED_USER_COLUMN_LIT) == 0)
	{
	  col_desc->body.columns_desc.colclass = 'A';
	  col_desc->body.columns_desc.addedColumn = 1;
	}

      if (info->colHeading)
	col_desc->body.columns_desc.heading = (char*)info->colHeading;
      else
	col_desc->body.columns_desc.hbaseColFam = NULL;

      if (info->hbaseColFam)
	col_desc->body.columns_desc.hbaseColFam = (char*)info->hbaseColFam;
      else
	col_desc->body.columns_desc.hbaseColFam = NULL;

      if (info->hbaseColQual)
	col_desc->body.columns_desc.hbaseColQual = (char*)info->hbaseColQual;
      else
	col_desc->body.columns_desc.hbaseColQual = NULL;

      col_desc->body.columns_desc.hbaseColFlags = info->hbaseColFlags;

      col_desc->body.columns_desc.paramDirection =
           CmGetComDirectionAsComParamDirection(info->paramDirection);
      col_desc->body.columns_desc.isOptional = info->isOptional;

      if (!first_col_desc)
	first_col_desc = col_desc;
      else
	prev_desc->header.next = col_desc;

      prev_desc = col_desc;
    }

  return first_col_desc;
}

static void initKeyDescStruct(keys_desc_struct * tgt,
			      const ComTdbVirtTableKeyInfo * src)
{
  tgt->indexname = NULL;

  if (src->colName)
    {
      tgt->keyname = new HEAP char[strlen(src->colName) +1];
      strcpy(tgt->keyname, src->colName);
    }
  else
    tgt->keyname = NULL;
      
  tgt->keyseqnumber     = src->keySeqNum;
  tgt->tablecolnumber   = src->tableColNum;
  tgt->ordering         = src->ordering;
  if (src->hbaseColFam)
    {
      tgt->hbaseColFam = new HEAP char[strlen(src->hbaseColFam)+1];
      strcpy(tgt->hbaseColFam, src->hbaseColFam);
    }
  else
    tgt->hbaseColFam = NULL;

  if (src->hbaseColQual)
    {
      tgt->hbaseColQual = new HEAP char[strlen(src->hbaseColQual)+1];
      strcpy(tgt->hbaseColQual, src->hbaseColQual);
    }
  else
    tgt->hbaseColQual = NULL;
}

desc_struct * Generator::createKeyDescs(Int32 numKeys,
					const ComTdbVirtTableKeyInfo * keyInfo)
{
  desc_struct * first_key_desc = NULL;

  if (keyInfo == NULL)
    return NULL;

  // create key descs
  desc_struct * prev_desc = NULL;
  for (Int32 keyNum = 0; keyNum < numKeys; keyNum++)
    {
      desc_struct * key_desc = readtabledef_allocate_desc(DESC_KEYS_TYPE);
      if (prev_desc)
	prev_desc->header.next = key_desc;
      else
       first_key_desc = key_desc;

      prev_desc = key_desc;

      initKeyDescStruct(&key_desc->body.keys_desc,
			&keyInfo[keyNum]);
    }

  return first_key_desc;
}

desc_struct * Generator::createConstrKeyColsDescs(Int32 numKeys,
					ComTdbVirtTableKeyInfo * keyInfo)
{
  desc_struct * first_key_desc = NULL;

  if (keyInfo == NULL)
    return NULL;

  // create key descs
  desc_struct * prev_desc = NULL;
  for (Int32 keyNum = 0; keyNum < numKeys; keyNum++)
    {
      desc_struct * key_desc = readtabledef_allocate_desc(DESC_CONSTRNT_KEY_COLS_TYPE);
      if (prev_desc)
	prev_desc->header.next = key_desc;
      else
       first_key_desc = key_desc;

      prev_desc = key_desc;

      ComTdbVirtTableKeyInfo * src = &keyInfo[keyNum];
      constrnt_key_cols_desc_struct * tgt = &key_desc->body.constrnt_key_cols_desc;
      if (src->colName)
	{
	  tgt->colname = new HEAP char[strlen(src->colName) +1];
	  strcpy(tgt->colname, src->colName);
	}
      else
	tgt->colname = NULL;
      
      tgt->position   = src->tableColNum;
    }

  return first_key_desc;
}

// this method is used to create both referencing and referenced constraint structs.
desc_struct * Generator::createRefConstrDescStructs(
						    Int32 numConstrs,
						    ComTdbVirtTableRefConstraints * refConstrs)
{
  desc_struct * first_constr_desc = NULL;

  if ((numConstrs == 0) || (refConstrs == NULL))
    return NULL;

   // create constr descs
  desc_struct * prev_desc = NULL;
  for (Int32 constrNum = 0; constrNum < numConstrs; constrNum++)
    {
      desc_struct * constr_desc = readtabledef_allocate_desc(DESC_REF_CONSTRNTS_TYPE);
      if (prev_desc)
	prev_desc->header.next = constr_desc;
      else
       first_constr_desc = constr_desc;

      prev_desc = constr_desc;

      ComTdbVirtTableRefConstraints * src = &refConstrs[constrNum];
      ref_constrnts_desc_struct * tgt = &constr_desc->body.ref_constrnts_desc;
      if (src->constrName)
	{
	  tgt->constrntname = new HEAP char[strlen(src->constrName) +1];
	  strcpy(tgt->constrntname, src->constrName);
	}
      else
	tgt->constrntname = NULL;

      if (src->baseTableName)
	{
	  tgt->tablename = new HEAP char[strlen(src->baseTableName) +1];
	  strcpy(tgt->tablename, src->baseTableName);
	}
      else
	tgt->tablename = NULL;
    }

  return first_constr_desc;
 
}

static Lng32 createDescStructs(char * rforkName,
			      Int32 numCols,
			      ComTdbVirtTableColumnInfo * columnInfo,
			      Int32 numKeys,
			      ComTdbVirtTableKeyInfo * keyInfo,
			      desc_struct* &colDescs,
			      desc_struct* &keyDescs)
{
  colDescs = NULL;
  keyDescs = NULL;
  UInt32 reclen = 0;

  // create column descs
  colDescs = Generator::createColDescs(rforkName, columnInfo, (Int16) numCols,
                                       reclen);

  keyDescs = Generator::createKeyDescs(numKeys, keyInfo);

  return (Lng32) reclen;
}

desc_struct * Generator::createVirtualTableDesc(
						const char * inTableName,
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
						ComTdbVirtTableSequenceInfo * seqInfo)
{
  // readtabledef_allocate_desc() requires that HEAP (STMTHEAP)
  // be used for operator new herein

  const char * tableName = (tableInfo ? tableInfo->tableName : inTableName);
  desc_struct * table_desc = readtabledef_allocate_desc(DESC_TABLE_TYPE);
  table_desc->body.table_desc.tablename = new HEAP char[strlen(tableName)+1];
  strcpy(table_desc->body.table_desc.tablename, tableName);
  
  if (tableInfo)
    {
      convertInt64ToUInt32Array(tableInfo->createTime, table_desc->body.table_desc.createtime);
      convertInt64ToUInt32Array(tableInfo->redefTime, table_desc->body.table_desc.redeftime);
      convertInt64ToUInt32Array(tableInfo->objUID, table_desc->body.table_desc.objectUID);
    }
  else
    {
      *(Int64*)&table_desc->body.table_desc.createtime = 0;
      *(Int64*)&table_desc->body.table_desc.redeftime = 0;
      *(Int64*)&table_desc->body.table_desc.objectUID = 0;
    }

  table_desc->body.table_desc.issystemtablecode = 1;
  table_desc->body.table_desc.underlyingFileType = SQLMX;
  table_desc->body.table_desc.rowcount = 100;

  if (CmpCommon::context()->sqlSession()->validateVolatileName(tableName))
    table_desc->body.table_desc.isVolatile = 1;
  else
    table_desc->body.table_desc.isVolatile = 0;

  if (numViews > 0)
    table_desc->body.table_desc.objectType = COM_VIEW_OBJECT;
  else if (seqInfo)
    table_desc->body.table_desc.objectType = COM_SEQUENCE_GENERATOR_OBJECT;
  else
    table_desc->body.table_desc.objectType = COM_BASE_TABLE_OBJECT;

  table_desc->body.table_desc.owner = (tableInfo ? tableInfo->objOwnerID : SUPER_USER);

  desc_struct * files_desc = readtabledef_allocate_desc(DESC_FILES_TYPE);
  //  files_desc->body.files_desc.audit = -1; // audited table
  files_desc->body.files_desc.audit = (tableInfo ? tableInfo->isAudited : -1);

  files_desc->body.files_desc.fileorganization = KEY_SEQUENCED_FILE;
  table_desc->body.table_desc.files_desc = files_desc;

  desc_struct * cols_descs = NULL;
  desc_struct * keys_descs = NULL;
  table_desc->body.table_desc.colcount = numCols;
  table_desc->body.table_desc.record_length =
    createDescStructs(table_desc->body.table_desc.tablename,
		      numCols, columnInfo, numKeys, keyInfo,
		      cols_descs, keys_descs);

  desc_struct * first_constr_desc = NULL;
  if (numConstrs > 0)
    {
      desc_struct * prev_desc = NULL;
      for (int i = 0; i < numConstrs; i++)
	{
	  desc_struct * curr_constr_desc = readtabledef_allocate_desc(DESC_CONSTRNTS_TYPE);

	  if (! first_constr_desc)
	    first_constr_desc = curr_constr_desc;

	  curr_constr_desc->body.constrnts_desc.tablename = new HEAP char[strlen(constrInfo[i].baseTableName)+1];
	  strcpy(curr_constr_desc->body.constrnts_desc.tablename, constrInfo[i].baseTableName);
	  
	  curr_constr_desc->body.constrnts_desc.constrntname = new HEAP char[strlen(constrInfo[i].constrName)+1];
	  strcpy(curr_constr_desc->body.constrnts_desc.constrntname, constrInfo[i].constrName);
	  
	  curr_constr_desc->body.constrnts_desc.indexname = NULL;

	  curr_constr_desc->body.constrnts_desc.check_constrnts_desc = NULL;
	  curr_constr_desc->body.constrnts_desc.isEnforced = 1;

	  switch (constrInfo[i].constrType)
	    {
	    case 0: // unique_constr
	      curr_constr_desc->body.constrnts_desc.type = UNIQUE_CONSTRAINT;
	      break;

	    case 1: // ref_constr
	      curr_constr_desc->body.constrnts_desc.type = REF_CONSTRAINT;
	      break;

	    case 2: // check_constr
	      curr_constr_desc->body.constrnts_desc.type = CHECK_CONSTRAINT;
	      break;

	    case 3: // pkey_constr
	      curr_constr_desc->body.constrnts_desc.type = PRIMARY_KEY_CONSTRAINT;
	      break;

	    } // switch

	  curr_constr_desc->body.constrnts_desc.colcount = constrInfo[i].colCount;

	  curr_constr_desc->body.constrnts_desc.constr_key_cols_desc =
	    Generator::createConstrKeyColsDescs(constrInfo[i].colCount, constrInfo[i].keyInfoArray);

	  if (constrInfo[i].ringConstrArray)
	    {
	      curr_constr_desc->body.constrnts_desc.referencing_constrnts_desc =
		Generator::createRefConstrDescStructs(constrInfo[i].numRingConstr,
						      constrInfo[i].ringConstrArray);
	    }

	  if (constrInfo[i].refdConstrArray)
	    {
	      curr_constr_desc->body.constrnts_desc.referenced_constrnts_desc =
		Generator::createRefConstrDescStructs(constrInfo[i].numRefdConstr,
						      constrInfo[i].refdConstrArray);
	    }

	  if ((constrInfo[i].constrType == 2) && // check constr
	      (constrInfo[i].checkConstrLen > 0))
	    {
	      desc_struct * check_constr_desc = readtabledef_allocate_desc(DESC_CHECK_CONSTRNTS_TYPE);
	      
	      check_constr_desc->body.check_constrnts_desc.seqnumber = 1;
	      check_constr_desc->body.check_constrnts_desc.constrnt_text = 
		new HEAP char[constrInfo[i].checkConstrLen + 1];
	      memcpy(check_constr_desc->body.check_constrnts_desc.constrnt_text,
		     constrInfo[i].checkConstrText, constrInfo[i].checkConstrLen);
	      check_constr_desc->body.check_constrnts_desc.constrnt_text
		[constrInfo[i].checkConstrLen] = 0;

	      curr_constr_desc->body.constrnts_desc.check_constrnts_desc =
		check_constr_desc;
	    }

	  if (prev_desc)
	    prev_desc->header.next = curr_constr_desc;

	  prev_desc = curr_constr_desc;
	} // for
    }

  desc_struct * index_desc = readtabledef_allocate_desc(DESC_INDEXES_TYPE);
  index_desc->body.indexes_desc.tablename = table_desc->body.table_desc.tablename;
  index_desc->body.indexes_desc.indexname = table_desc->body.table_desc.tablename;
  index_desc->body.indexes_desc.keytag = 0; // primary index
  index_desc->body.indexes_desc.record_length = table_desc->body.table_desc.record_length;
  index_desc->body.indexes_desc.colcount = table_desc->body.table_desc.colcount;
  index_desc->body.indexes_desc.isVerticalPartition = 0;
  index_desc->body.indexes_desc.blocksize = 32*1024; //100000; //4096; // doesn't matter.
  index_desc->body.indexes_desc.isVolatile = table_desc->body.table_desc.isVolatile;
  index_desc->body.indexes_desc.hbaseCreateOptions  = NULL;
  index_desc->body.indexes_desc.numSaltPartns = 0;
  if (tableInfo)
  {
      index_desc->body.indexes_desc.numSaltPartns = tableInfo->numSaltPartns;
      if (tableInfo->hbaseCreateOptions)
      {
        index_desc->body.indexes_desc.hbaseCreateOptions  = 
          new HEAP char[strlen(tableInfo->hbaseCreateOptions) + 1];
        strcpy(index_desc->body.indexes_desc.hbaseCreateOptions, 
               tableInfo->hbaseCreateOptions);
      }
  }

  if (numIndexes > 0)
    {
      desc_struct * prev_desc = index_desc;
      for (int i = 0; i < numIndexes; i++)
	{
	  desc_struct * curr_index_desc = readtabledef_allocate_desc(DESC_INDEXES_TYPE);
	  
	  prev_desc->header.next = curr_index_desc;

	  curr_index_desc->body.indexes_desc.tablename = new HEAP char[strlen(indexInfo[i].baseTableName)+1];
	  strcpy(curr_index_desc->body.indexes_desc.tablename, indexInfo[i].baseTableName);

	  curr_index_desc->body.indexes_desc.indexname = new HEAP char[strlen(indexInfo[i].indexName)+1];
	  strcpy(curr_index_desc->body.indexes_desc.indexname, indexInfo[i].indexName);

	  curr_index_desc->body.indexes_desc.keytag = indexInfo[i].keytag;
	  curr_index_desc->body.indexes_desc.unique = indexInfo[i].isUnique;
	  curr_index_desc->body.indexes_desc.isCreatedExplicitly = indexInfo[i].isExplicit;
          curr_index_desc->body.indexes_desc.record_length = 
            getRecordLength(&indexInfo[i], columnInfo);
	  curr_index_desc->body.indexes_desc.colcount = indexInfo[i].keyColCount + indexInfo[i].nonKeyColCount;
	  curr_index_desc->body.indexes_desc.isVerticalPartition = 0;
	  curr_index_desc->body.indexes_desc.blocksize = 32*1024; //100000; //4096; // doesn't matter.

	  curr_index_desc->body.indexes_desc.keys_desc = 
	    Generator::createKeyDescs(indexInfo[i].keyColCount, indexInfo[i].keyInfoArray);

	  curr_index_desc->body.indexes_desc.non_keys_desc = 
	    Generator::createKeyDescs(indexInfo[i].nonKeyColCount, indexInfo[i].nonKeyInfoArray);

	  if (CmpCommon::context()->sqlSession()->validateVolatileName(indexInfo[i].indexName))
	    curr_index_desc->body.indexes_desc.isVolatile = 1;
	  else
	    curr_index_desc->body.indexes_desc.isVolatile = 0;
          curr_index_desc->body.indexes_desc.hbaseCreateOptions  = NULL;
          curr_index_desc->body.indexes_desc.numSaltPartns = 
            indexInfo[i].numSaltPartns;
          if (indexInfo[i].hbaseCreateOptions)
          {
            curr_index_desc->body.indexes_desc.hbaseCreateOptions  = 
              new HEAP char[strlen(indexInfo[i].hbaseCreateOptions) + 1];
            strcpy(curr_index_desc->body.indexes_desc.hbaseCreateOptions, 
               indexInfo[i].hbaseCreateOptions);
          }

	  prev_desc = curr_index_desc;

	}
    }

  desc_struct * view_desc = NULL;
  if (numViews > 0)
    {
      view_desc = readtabledef_allocate_desc(DESC_VIEW_TYPE);
      
      view_desc->body.view_desc.viewname = new HEAP char[strlen(viewInfo[0].viewName)+1];
      strcpy(view_desc->body.view_desc.viewname, viewInfo[0].viewName);

      view_desc->body.view_desc.viewfilename = view_desc->body.view_desc.viewname;
      view_desc->body.view_desc.viewtext = new HEAP char[strlen(viewInfo[0].viewText) + 1];
      strcpy(view_desc->body.view_desc.viewtext, viewInfo[0].viewText);

      view_desc->body.view_desc.viewtextcharset = (CharInfo::CharSet)SQLCHARSETCODE_UTF8;

      if (viewInfo[0].viewCheckText)
	{
	  view_desc->body.view_desc.viewchecktext = new HEAP char[strlen(viewInfo[0].viewCheckText)+1];
	  strcpy(view_desc->body.view_desc.viewchecktext, viewInfo[0].viewCheckText);
	}
      else
	view_desc->body.view_desc.viewchecktext = NULL;

      view_desc->body.view_desc.updatable = viewInfo[0].isUpdatable;
      view_desc->body.view_desc.insertable = viewInfo[0].isInsertable;
    }

  desc_struct * seq_desc = NULL;
  if (seqInfo)
    {
      seq_desc = readtabledef_allocate_desc(DESC_SEQUENCE_GENERATOR_TYPE);
      
      seq_desc->body.sequence_generator_desc.sgType = (ComSequenceGeneratorType)seqInfo->seqType;

      seq_desc->body.sequence_generator_desc.fsDataType = (ComFSDataType)seqInfo->datatype;
      seq_desc->body.sequence_generator_desc.startValue = seqInfo->startValue;
      seq_desc->body.sequence_generator_desc.increment = seqInfo->increment;

      seq_desc->body.sequence_generator_desc.maxValue = seqInfo->maxValue;
      seq_desc->body.sequence_generator_desc.minValue = seqInfo->minValue;
      seq_desc->body.sequence_generator_desc.cycleOption = 
	(seqInfo->cycleOption ? TRUE : FALSE);
      seq_desc->body.sequence_generator_desc.cache = seqInfo->cache;      
      seq_desc->body.sequence_generator_desc.objectUID = seqInfo->seqUID;
      
      seq_desc->body.sequence_generator_desc.nextValue = seqInfo->nextValue;
    }

  // cannot simply point to same files desc as the table one,
  // because then ReadTableDef::deleteTree frees same memory twice (error)
  desc_struct * i_files_desc = readtabledef_allocate_desc(DESC_FILES_TYPE);
  i_files_desc->body.files_desc.audit = -1; // audited table
  i_files_desc->body.files_desc.fileorganization = KEY_SEQUENCED_FILE;
  index_desc->body.indexes_desc.files_desc = i_files_desc;

  index_desc->body.indexes_desc.keys_desc  = keys_descs;
  table_desc->body.table_desc.columns_desc = cols_descs;
  table_desc->body.table_desc.indexes_desc = index_desc;
  table_desc->body.table_desc.views_desc = view_desc;
  table_desc->body.table_desc.constrnts_desc = first_constr_desc;
  table_desc->body.table_desc.constr_count = numConstrs; 
  table_desc->body.table_desc.sequence_generator_desc = seq_desc;

  return table_desc;
}

desc_struct *Generator::createVirtualRoutineDesc(
                                  const char *routineName,
                                  ComTdbVirtTableRoutineInfo *routineInfo,
                                  Int32 numParams,
                                  ComTdbVirtTableColumnInfo *paramsArray)
{
   desc_struct *routine_desc = readtabledef_allocate_desc(DESC_ROUTINE_TYPE);
   routine_desc->body.routine_desc.routineName = new HEAP char[strlen(routineName)+1];
   strcpy(routine_desc->body.routine_desc.routineName, routineName);
   routine_desc->body.routine_desc.externalName = new HEAP char[strlen(routineInfo->external_name)+1];
   strcpy(routine_desc->body.routine_desc.externalName, routineInfo->external_name);
   routine_desc->body.routine_desc.librarySqlName = NULL; 
   routine_desc->body.routine_desc.libraryFileName = new HEAP char[strlen(routineInfo->library_filename)+1];
   strcpy(routine_desc->body.routine_desc.libraryFileName, routineInfo->library_filename);
   routine_desc->body.routine_desc.signature = new HEAP char[strlen(routineInfo->signature)+1];
   strcpy(routine_desc->body.routine_desc.signature, routineInfo->signature);
   routine_desc->body.routine_desc.language  = 
           CmGetComRoutineLanguageAsRoutineLanguage(routineInfo->language_type);
   routine_desc->body.routine_desc.UDRType  = 
           CmGetComRoutineTypeAsRoutineType(routineInfo->UDR_type);
   routine_desc->body.routine_desc.sqlAccess  = 
           CmGetComRoutineSQLAccessAsRoutineSQLAccess(routineInfo->sql_access);
   routine_desc->body.routine_desc.transactionAttributes  = 
           CmGetComRoutineTransactionAttributesAsRoutineTransactionAttributes
                  (routineInfo->transaction_attributes);
   routine_desc->body.routine_desc.maxResults = routineInfo->max_results;
   routine_desc->body.routine_desc.paramStyle  = 
           CmGetComRoutineParamStyleAsRoutineParamStyle(routineInfo->param_style);
   routine_desc->body.routine_desc.isDeterministic = routineInfo->deterministic;
   routine_desc->body.routine_desc.isCallOnNull = routineInfo->call_on_null;
   routine_desc->body.routine_desc.isIsolate = routineInfo->isolate;
   routine_desc->body.routine_desc.externalSecurity  = 
           CmGetRoutineExternalSecurityAsComRoutineExternalSecurity
                  (routineInfo->external_security);
   routine_desc->body.routine_desc.executionMode  = 
           CmGetRoutineExecutionModeAsComRoutineExecutionMode
                 (routineInfo->execution_mode);
   routine_desc->body.routine_desc.stateAreaSize = routineInfo->state_area_size;
   routine_desc->body.routine_desc.parallelism  = 
           CmGetRoutineParallelismAsComRoutineParallelism(routineInfo->parallelism);
   UInt32 reclen; 
   routine_desc->body.routine_desc.paramsCount = numParams;
   routine_desc->body.routine_desc.params = 
             Generator::createColDescs(routineName, 
              paramsArray, (Int16) numParams, reclen);
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

void Generator::setHBaseNumCacheRows(double estRowsAccessed, ComTdbHbaseAccess::HbasePerfAttributes * hbpa)
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
  if (numProcesses == 0)
    numProcesses++;
  UInt32 rowsAccessedPerProcess = ceil(estRowsAccessed/numProcesses) ;
  if (rowsAccessedPerProcess < CmpCommon::getDefaultNumeric(HBASE_NUM_CACHE_ROWS_MIN))
    hbpa->setNumCacheRows(CmpCommon::getDefaultNumeric(HBASE_NUM_CACHE_ROWS_MIN));
  else if (rowsAccessedPerProcess < CmpCommon::getDefaultNumeric(HBASE_NUM_CACHE_ROWS_MAX))
    hbpa->setNumCacheRows(rowsAccessedPerProcess);
  else
      hbpa->setNumCacheRows(CmpCommon::getDefaultNumeric(HBASE_NUM_CACHE_ROWS_MAX));

}
