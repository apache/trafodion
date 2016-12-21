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
****************************************************************************
*
* File:         ComTdbExe.cpp
* Description:  Implementation for those methods of ComTDB which have to
*               be implemented in the executor project because of their
*               dependencies on other executor objects.
*
* Created:      5/6/98
* Language:     C++
*
****************************************************************************
*/

#include "ComPackDefs.h"
#include "ExCollections.h"
#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_hash_grby.h"
#include "ex_sort_grby.h"
#include "ExFirstN.h"
#include "ExTranspose.h"
#include "ExPackedRows.h"
#include "ExPack.h"
#include "ExSample.h"
#include "ExSimpleSample.h"
#include "ex_tuple.h"
#include "ExCompoundStmt.h"
#include "ex_union.h"
#include "ex_onlj.h"
#include "ex_tuple_flow.h"
#include "ExFastTransport.h"


#ifndef __EID
#include "ex_root.h"
#include "ex_mj.h"
#include "ex_hashj.h"
#include "ex_control.h"
#include "ex_split_top.h"
#include "ex_split_bottom.h"
#include "ex_send_top.h"
#include "ex_send_bottom.h"
#include "PartInputDataDesc.h"
#include "ex_stored_proc.h"
#include "ex_sort.h"
#include "ExExplain.h"
#include "ex_ddl.h"
#include "ExExeUtil.h"
#include "ex_transaction.h"
#include "ExSequence.h"
#include "ExStats.h"
#include "ex_exe_stmt_globals.h"
#include "ex_timeout.h"   
#include "ExUdr.h"
#include "ExProbeCache.h"
#include "ExCancel.h"
#include "ExHdfsScan.h"
#include "ExHbaseAccess.h"


#endif

// -----------------------------------------------------------------------
// When called within tdm_sqlcli.dll, fixupVTblPtr() translates to a call
// to fix up the TDB's to the Executor version. This function is also
// defined in Generator.cpp. That code, however, fixes up the TDB's to
// the Compiler version.
// -----------------------------------------------------------------------
// LCOV_EXCL_START
void ComTdb::fixupVTblPtr()
{
  fixupVTblPtrExe();
}
// LCOV_EXCL_STOP

char *ComTdb::findVTblPtr(short classID)
{
  return findVTblPtrExe(classID);
}

// -----------------------------------------------------------------------
// This method fixes up a TDB object which is retrieved from disk or
// received from another process to the Executor version of the TDB
// for its node type. There is a similar method called fixupVTblPtrCom()
// implemented in the comexe project which fixes up a TDB object to the
// Compiler version of the TDB.
// -----------------------------------------------------------------------
// LCOV_EXCL_START
void ComTdb::fixupVTblPtrExe()
{
	ex_assert(0,"fixupVTblPtrExe() shouldn't be called"); // method retired
}
// LCOV_EXCL_STOP


// -----------------------------------------------------------------------
// This method returns the virtual function table pointer for an object
// with the given class ID as a "executor TDB" (the one with a build()
// method defined). There is a similar method called findVTblPtrCom()
// implemented in the comexe project (in ComTdb.cpp) which returns the
// pointer for an "compiler TDB".
// -----------------------------------------------------------------------
NA_EIDPROC char *ComTdb::findVTblPtrExe(short classID)
{
  char *vtblptr = NULL;
  switch (classID)
  {

    case ex_HASH_GRBY:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_hash_grby_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_SORT_GRBY:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_sort_grby_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }
   case ex_FIRST_N:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExFirstNTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_TRANSPOSE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExTransposeTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_UNPACKROWS:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExUnPackRowsTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_PACKROWS:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExPackRowsTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_SAMPLE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExSampleTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

#if 0
// unused feature, done as part of SQ SQL code cleanup effort
    case ex_SIMPLE_SAMPLE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExSimpleSampleTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }
#endif // if 0

    case ex_LEAF_TUPLE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExTupleLeafTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_COMPOUND_STMT:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr, ExCatpoundStmtTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

// LCOV_EXCL_START

    case ex_NON_LEAF_TUPLE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExTupleNonLeafTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

#ifndef __EID

// LCOV_EXCL_STOP

    case ex_CONTROL_QUERY:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExControlTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_ROOT:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_root_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }
#endif

    case ex_ONLJ:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExOnljTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_HASHJ:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_hashj_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_MJ:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_mj_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_UNION:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_union_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }
    case ex_FAST_EXTRACT:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExFastExtractTdb);
#pragma warn(1506)  // warning elimination
      break;
    }

#ifndef __EID
    case ex_UDR:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExUdrTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }
    case ex_EXPLAIN:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExplainTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_SEQUENCE_FUNCTION:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExSequenceTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_SORT:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExSortTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_SPLIT_TOP:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_split_top_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_SPLIT_BOTTOM:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_split_bottom_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_SEND_TOP:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_send_top_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_SEND_BOTTOM:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ex_send_bottom_tdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_STATS:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExStatsTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_STORED_PROC:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExStoredProcTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }
#endif

    case ex_TUPLE_FLOW:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExTupleFlowTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

#ifndef __EID
    case ex_SET_TIMEOUT:  
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExTimeoutTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_TRANSACTION:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExTransTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_DDL:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExDDLTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_DDL_WITH_STATUS:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExDDLwithStatusTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_DESCRIBE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExDescribeTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_EXE_UTIL:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_FAST_DELETE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilFastDeleteTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_HIVE_TRUNCATE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilHiveTruncateTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_PROCESS_VOLATILE_TABLE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExProcessVolatileTableTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

  case ex_LOAD_VOLATILE_TABLE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilLoadVolatileTableTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }

  case ex_CLEANUP_VOLATILE_TABLES:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilCleanupVolatileTablesTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }
  
  case ex_GET_VOLATILE_INFO:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilGetVolatileInfoTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }


    case ex_PROCESS_INMEMORY_TABLE:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExProcessInMemoryTableTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_CREATE_TABLE_AS:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilCreateTableAsTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_GET_STATISTICS:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilGetStatisticsTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }
case ex_LOB_INFO:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilLobInfoTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }
  case ex_GET_METADATA_INFO:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilGetMetadataInfoTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }

  case ex_GET_HIVE_METADATA_INFO:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilGetHiveMetadataInfoTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }

    case ex_GET_UID:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilGetUIDTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }

   case ex_GET_QID:
    {
      GetVTblPtr(vtblptr,ExExeUtilGetQIDTdb);

      break;
    }

    case ex_POP_IN_MEM_STATS:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilPopulateInMemStatsTdb);
#pragma warn(1506)  // warning elimination 

      break;
    }
  
    case ex_DISPLAY_EXPLAIN:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilDisplayExplainTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_DISPLAY_EXPLAIN_COMPLEX:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilDisplayExplainComplexTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

    case ex_PROBE_CACHE:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExProbeCacheTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

    case ex_LONG_RUNNING:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilLongRunningTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }

// Suspend uses ExCancelTdb

    case ex_SHOW_SET:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilShowSetTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

    case ex_AQR:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilAQRTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

   case ex_GET_ERROR_INFO:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilGetErrorInfoTdb);
#pragma warn(1506)  // warning elimination

      break;
    }
  case ex_PROCESS_STATISTICS:
    {
#pragma nowarn(1506)   // warning elimination 
      GetVTblPtr(vtblptr,ExExeUtilGetProcessStatisticsTdb);
#pragma warn(1506)  // warning elimination 
      break;
    }
  case ex_ARQ_WNR_INSERT:
  {
#pragma nowarn(1506)   // warning elimination
    GetVTblPtr(vtblptr,ExExeUtilAqrWnrInsertTdb);
    break;
#pragma warn(1506)  // warning elimination
  }

   case ex_HDFS_SCAN:
   {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExHdfsScanTdb);
#pragma warn(1506)  // warning elimination
      break;
   }

   case ex_LOB_EXTRACT:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilLobExtractTdb);
#pragma warn(1506)  // warning elimination

      break;
    }
  case ex_LOB_UPDATE_UTIL:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilLobUpdateTdb);
#pragma warn(1506)  // warning elimination

      break;
    }
   case ex_LOB_SHOWDDL:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilLobShowddlTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

  case ex_HIVE_MD_ACCESS:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilHiveMDaccessTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

  case ex_HBASE_ACCESS:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExHbaseAccessTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

  case ex_HBASE_COPROC_AGGR:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExHbaseCoProcAggrTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

  case ex_HBASE_LOAD:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilHBaseBulkLoadTdb);
#pragma warn(1506)  // warning elimination

      break;
    }
  case ex_HBASE_UNLOAD:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExExeUtilHBaseBulkUnLoadTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

  case ex_ORC_AGGR:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExOrcFastAggrTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

    case ex_CANCEL:
    {
#pragma nowarn(1506)   // warning elimination
      GetVTblPtr(vtblptr,ExCancelTdb);
#pragma warn(1506)  // warning elimination

      break;
    }

   case ex_REGION_STATS:
    {
      GetVTblPtr(vtblptr,ExExeUtilRegionStatsTdb);

      break;
    }

#endif
    default:
      ex_assert(0, "findVTblPtrExe(): Cannot find entry of this ClassId"); // LCOV_EXCL_LINE
      break;
  }
  return vtblptr;
}

NA_EIDPROC
void fixupExeVtblPtr(ComTdb * tdb)
{
  for (Int32 i = 0; i < tdb->numChildren(); i++)
    {
      fixupExeVtblPtr((ComTdb *)(tdb->getChild(i)));
    }

  char * vtblPtr = NULL;
  vtblPtr = tdb->findVTblPtrExe(tdb->getClassID());
  tdb->setVTblPtr(vtblPtr);
 
}

NA_EIDPROC
void fixupComVtblPtr(ComTdb * tdb)
{
  for (Int32 i = 0; i < tdb->numChildren(); i++)
    {
      fixupComVtblPtr((ComTdb *)(tdb->getChild(i)));
    }

  char * vtblPtr = NULL;
  vtblPtr = tdb->findVTblPtrCom(tdb->getClassID());
  tdb->setVTblPtr(vtblPtr);
 
}

NA_EIDPROC
void resetBufSize(ex_tcb * tcb, Lng32 &tcbSpaceNeeded, Lng32 &poolSpaceNeeded)
{
  for (Int32 i = 0; i < tcb->numChildren(); i++)
    {
      resetBufSize((ex_tcb *)(tcb->getChild(i)), 
		   tcbSpaceNeeded, poolSpaceNeeded);
    }
  
  if (tcb->getPool())
    {
      Int32 numBuffs = -1;
      UInt32 staticPoolSpaceSize = 0;
      UInt32 dynPoolSpaceSize = 0;

      // compute total pool space needed(static + dynamic) at runtime
      tcb->computeNeededPoolInfo(numBuffs, 
				 staticPoolSpaceSize, dynPoolSpaceSize);

#pragma nowarn(1506)   // warning elimination 
      // size of pool space that was allocated during the build
      // phase. This value is included in tcbSpaceNeeded.
      // compute tcb space needed by subtracting staticPoolSize
      tcbSpaceNeeded -= staticPoolSpaceSize;

      poolSpaceNeeded += dynPoolSpaceSize;
#pragma warn(1506)  // warning elimination 

      if ((tcb->resizePoolInfo()) &&
	  (numBuffs >= 0))
	((ComTdb *)(tcb->getTdb()))->
	  resetBufInfo(numBuffs, (staticPoolSpaceSize + dynPoolSpaceSize));
    }
}

#pragma nowarn(262)   // warning elimination 
NA_EIDPROC
Lng32 getTotalTcbSpace(char*inTdb, char * otherInfo, char * parentMemory)
{
  ComTdb * tdb = (ComTdb*)inTdb;

  Space space(Space::EXECUTOR_SPACE);
  space.setParent((NAHeap*)parentMemory);
  //  space.setType(Space::SYSTEM_SPACE);
  //  space.setType(Space::EXECUTOR_SPACE);

  //  NAHeap heap("temp heap", NAMemory::DERIVED_FROM_SYS_HEAP);
  // NAHeap heap("temp heap", NAMemory::EXECUTOR_MEMORY);
  NAHeap heap("temp heap", (NAHeap*)parentMemory, 32000 /*blocksize*/);
  ex_globals * g = NULL;

#ifndef __EID
  switch (tdb->getNodeType())
    {
// LCOV_EXCL_START
// This method is called from an internal CLI call and the only palce
// where this internal CLI call is make is from generator for Generator.cpp
// for EID Root tdb and so this code is not called for other tdb's
    case ComTdb::ex_ROOT:
      g = new(&heap) ExMasterStmtGlobals(10, NULL, NULL, 0, &space, &heap);
      break;

    case ComTdb::ex_SPLIT_TOP:
      g = new(&heap) ExEspStmtGlobals(10, NULL, 0, &space, &heap, NULL,
                                      NullFragInstanceHandle, 0);
      break;
// LCOV_EXCL_STOP
    case ComTdb::ex_EXPLAIN:
      break;

    default:
      break;

    }
#else
#endif

  g->setComputeSpace(TRUE);

  fixupExeVtblPtr(tdb);
  ex_tcb * tcb = tdb->build(g);
  fixupComVtblPtr(tdb);

  Lng32 totalSpaceNeeded = 0;
  Lng32 tcbSpaceNeeded = 0;
  Lng32 poolSpaceNeeded = 0;
  tcbSpaceNeeded = space.getAllocatedSpaceSize();

  resetBufSize(tcb, tcbSpaceNeeded, poolSpaceNeeded);

  // add a 25% fudge factor to TCB space
  tcbSpaceNeeded = (tcbSpaceNeeded * 125) / 100;
  
  totalSpaceNeeded = tcbSpaceNeeded + poolSpaceNeeded;

  return totalSpaceNeeded;
}
#pragma warn(262)  // warning elimination 
