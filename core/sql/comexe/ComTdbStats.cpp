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
* File:         ComTdbStats.cpp
* Description:  
*
* Created:      12/1/1999
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbStats.h"
#include "ComTdbCommon.h"
#include "ComTdbRoot.h"
#include "ComCextdecs.h"

// Default Constructor.  This is used by the ComTdb::fixupVTblPtr()
// routine which fixes up the virtual function table pointer after
// a node has been unpacked.
ComTdbStats::ComTdbStats() 
: ComTdb(ComTdb::ex_STATS, eye_STATS)
{
}

ComTdbStats::ComTdbStats(ULng32 tupleLen,
			 ULng32 returnedTupleLen,
			 ULng32 inputTupleLen,
			 ex_cri_desc *criDescParentDown,
			 ex_cri_desc *criDescParentUp,
			 queue_index queueSizeDown,
			 queue_index queueSizeUp,
			 Lng32 numBuffers,
			 ULng32 bufferSize,
			 ex_expr *scanExpr,
			 ex_expr *inputExpr,
			 ex_expr *projExpr,
			 ex_cri_desc *workCriDesc,
			 UInt16 stats_row_atp_index,
			 UInt16 input_row_atp_index
			 )
 : ComTdb(ComTdb::ex_STATS,
	   eye_STATS,
	   (Cardinality) 0.0,
	   criDescParentDown,
	   criDescParentUp,
	   queueSizeDown,
	   queueSizeUp,
	   numBuffers,
	   bufferSize),
   scanExpr_(scanExpr),
   inputExpr_(inputExpr),
   projExpr_(projExpr),
   workCriDesc_(workCriDesc),
   tupleLen_(tupleLen),
   returnedTupleLen_(returnedTupleLen),
   inputTupleLen_(inputTupleLen),
   statsTupleAtpIndex_(stats_row_atp_index),
   inputTupleAtpIndex_(input_row_atp_index)
{
}

const char * ComTdbStats::getExpressionName(Int32 expNum) const
{
  switch(expNum)
    {
    case 0:
      return "Scan Expr";
    case 1:
      return "Input Expr";
    default:
      return 0;
    }  
}

ex_expr *ComTdbStats::getExpressionNode(Int32 expNum)
{
  switch(expNum)
    {
    case 0:
      return scanExpr_;
    case 1:
      return inputExpr_;
    default:
      return 0;
    }  
}

Long ComTdbStats::pack(void * space)
{
  scanExpr_.pack(space);
  inputExpr_.pack(space);
  projExpr_.pack(space);
  workCriDesc_.pack(space);

  return ComTdb::pack(space);
}

Lng32 ComTdbStats::unpack(void * base, void * reallocator)
{
  if (scanExpr_.unpack(base, reallocator)) return -1;
  if (inputExpr_.unpack(base, reallocator)) return -1;
  if (projExpr_.unpack(base, reallocator)) return -1;
  if (workCriDesc_.unpack(base, reallocator)) return -1;

  return ComTdb::unpack(base, reallocator);
}

/************************************************************************
CompilationStatsData 

Copy the CompilationStatsData fields

************************************************************************/
CompilationStatsData& 
CompilationStatsData::operator=(CompilationStatsData &csd)
{
  compileStartTime_       = csd.compileStartTime_;
  compileEndTime_         = csd.compileEndTime_;
  compilerId_             = csd.compilerId_;
  cmpCpuTotal_            = csd.cmpCpuTotal_;
  cmpCpuBinder_           = csd.cmpCpuBinder_;
  cmpCpuNormalizer_       = csd.cmpCpuNormalizer_;
  cmpCpuAnalyzer_         = csd.cmpCpuAnalyzer_;
  cmpCpuOptimizer_        = csd.cmpCpuOptimizer_;
  cmpCpuGenerator_        = csd.cmpCpuGenerator_;
  metadataCacheHits_      = csd.metadataCacheHits_;
  metadataCacheLookups_   = csd.metadataCacheLookups_;
  queryCacheState_        = csd.queryCacheState_;
  histogramCacheHits_     = csd.histogramCacheHits_;
  histogramCacheLookups_  = csd.histogramCacheLookups_;
  stmtHeapSize_           = csd.stmtHeapSize_;
  cxtHeapSize_            = csd.cxtHeapSize_;
  optTasks_               = csd.optTasks_;
  optContexts_            = csd.optContexts_;
  isRecompile_            = csd.isRecompile_;
  compileInfo_            = csd.compileInfo_;

  return *this;
}
/************************************************************************
CompilationStatsData - pack

pack the NABasicPtr members

************************************************************************/
Long CompilationStatsData::pack(void * space)
{
  compilerId_.pack(space);
  compileInfo_.pack(space);
  
  return NAVersionedObject::pack(space);
}
/************************************************************************
CompilationStatsData - unpack

unpack the NABasicPtr members

************************************************************************/
Lng32 CompilationStatsData::unpack(void * base, void * reallocator)
{
  if (compilerId_.unpack(base)) return -1;
  if (compileInfo_.unpack(base)) return -1;

  return NAVersionedObject::unpack(base, reallocator);
}

CompilerStatsInfo::CompilerStatsInfo()
     : totalOps_(0), exchangeOps_(0), dp2Ops_(0),
       hj_(0), mj_(0), nj_(0),
       espTotal_(0), espLevels_(0),
       dop_(0), affinityNumber_(0),
       totalFragmentSize_(0), masterFragmentSize_(0),
       espFragmentSize_(0), dp2FragmentSize_(0),
       dp2RowsAccessed_(0), dp2RowsUsed_(0),
       dp2RowsAccessedForFullScan_(-1),
       flags_(0),
       bmo_(0),
       udr_(0),
       ofSize_(0),
       ofMode_((UInt16)ComTdb::OFM_DISK),
       collectStatsType_((UInt16)ComTdb::NO_STATS),
       queryType_((Int16)ComTdbRoot::SQL_UNKNOWN),
       subqueryType_((Int16)ComTdbRoot::SQL_STMT_NA),
       bmoMemLimitPerNode_(0),
       estBmoMemPerNode_(0)
{

}

CompilerStatsInfo& CompilerStatsInfo::operator=(CompilerStatsInfo&csi)
{
  totalOps_ = csi.totalOps_;
  exchangeOps_ = csi.exchangeOps_;
  dp2Ops_ = csi.dp2Ops_;

  hj_ = csi.hj_;
  mj_ = csi.mj_;
  nj_ = csi.nj_;

  espTotal_ = csi.espTotal_;
  espLevels_ = csi.espLevels_;
  dop_ = csi.dop_;
  affinityNumber_ = csi.affinityNumber_;

  totalFragmentSize_ = csi.totalFragmentSize_;
  masterFragmentSize_ = csi.masterFragmentSize_;
  espFragmentSize_ = csi.espFragmentSize_;
  dp2FragmentSize_ = csi.dp2FragmentSize_;

  dp2RowsAccessed_ = csi.dp2RowsAccessed_;
  dp2RowsUsed_     = csi.dp2RowsUsed_;

  dp2RowsAccessedForFullScan_ = csi.dp2RowsAccessedForFullScan_;

  flags_ = csi.flags_;
  collectStatsType_ = csi.collectStatsType_;
  bmo_ = csi.bmo_;
  udr_ = csi.udr_;
  ofSize_ = csi.ofSize_;
  ofMode_ = csi.ofMode_;
  queryType_ = csi.queryType_;
  subqueryType_ = csi.subqueryType_;
  bmoMemLimitPerNode_ = csi.bmoMemLimitPerNode_;
  estBmoMemPerNode_ = csi.estBmoMemPerNode_;
  return *this;
}

void QueryCostInfo :: translateToExternalFormat(SQL_QUERY_COST_INFO *query_cost_info)
{
  query_cost_info->cpuTime 
    = cpuTime();
  query_cost_info->ioTime 
    = ioTime();
  query_cost_info->msgTime 
    = msgTime();
  query_cost_info->idleTime 
    = idleTime();
  query_cost_info->totalTime 
    = totalTime();
  query_cost_info->cardinality 
    = cardinality();
  query_cost_info->estimatedTotalMem
    = totalMem();
  query_cost_info->resourceUsage
    = resourceUsage();
  query_cost_info->maxCpuUsage
    = maxCpuUsage();
}

void CompilerStatsInfo :: translateToExternalFormat(SQL_QUERY_COMPILER_STATS_INFO *query_comp_stats_info, short xnReqd)
{
  query_comp_stats_info->affinityNumber
    = affinityNumber();

  query_comp_stats_info->dop
    = dop();

  query_comp_stats_info->xnNeeded
    = xnReqd;

  query_comp_stats_info->mandatoryCrossProduct
    = mandatoryCrossProduct();

  query_comp_stats_info->missingStats
    = missingStats();
  query_comp_stats_info->numOfJoins
    = totalJoins();
	      
  query_comp_stats_info->fullScanOnTable
    = fullScanOnTable();

  query_comp_stats_info->highDp2MxBufferUsage
    = 0;  // this data member is not present in the TDB yet

  query_comp_stats_info->rowsAccessedForFullScan
    = dp2RowsAccessedForFullScan();

  query_comp_stats_info->dp2RowsAccessed
    = dp2RowsAccessed();

  query_comp_stats_info->dp2RowsUsed
    = dp2RowsUsed();
  query_comp_stats_info->statsCollectionType 
    = collectStatsType();
  query_comp_stats_info->numOfBmos 
    = bmo();
  query_comp_stats_info->numOfUdrs
    = udr();
  query_comp_stats_info->overflowMode
    = ofMode();
  query_comp_stats_info->overflowSize
    = ofSize();
  query_comp_stats_info->queryType
    = queryType();
  query_comp_stats_info->subqueryType
    = subqueryType();
  query_comp_stats_info->bmoMemLimitPerNode = bmoMemLimitPerNode(); 
  query_comp_stats_info->estBmoMemPerNode = estBmoMemPerNode(); 

}

void CompilationStatsData :: translateToExternalFormat(
        SQL_COMPILATION_STATS_DATA *query_cmp_data, 
        Int64 cmpStartTime, Int64 cmpEndTime)
{
  query_cmp_data->compileStartTime  = cmpStartTime;

  query_cmp_data->compileEndTime = cmpEndTime;

  str_cpy_all(query_cmp_data->compilerId,
	      compilerId(),
	      COMPILER_ID_LEN);

  query_cmp_data->cmpCpuTotal
    = cmpCpuTotal();

  query_cmp_data->cmpCpuBinder
    = cmpCpuBinder();

  query_cmp_data->cmpCpuNormalizer
    = cmpCpuNormalizer();

  query_cmp_data->cmpCpuAnalyzer
    = cmpCpuAnalyzer();

  query_cmp_data->cmpCpuOptimizer
    = cmpCpuOptimizer();
  query_cmp_data->cmpCpuGenerator
    = cmpCpuGenerator();

  query_cmp_data->metadataCacheHits
    = metadataCacheHits();

  query_cmp_data->metadataCacheLookups
    = metadataCacheLookups();

  query_cmp_data->queryCacheState
    = queryCacheState();

  query_cmp_data->histogramCacheHits
    = histogramCacheHits();

  query_cmp_data->histogramCacheLookups
    = histogramCacheLookups();

  query_cmp_data->stmtHeapSize
    = stmtHeapSize();

}
