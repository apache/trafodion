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
* File:         ComTdbExeUtil.cpp
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbExeUtil.h"
#include "ComTdbCommon.h" 
#include "ComSmallDefs.h"

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtil
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtil::ComTdbExeUtil(Lng32 type,
			     char * query,
			     ULng32 querylen,
			     Int16 querycharset,
			     char * tableName,
			     ULng32 tableNameLen,
			     ex_expr * input_expr,
			     ULng32 input_rowlen,
			     ex_expr * output_expr,
			     ULng32 output_rowlen,
			     ex_expr_base * scan_expr,
			     ex_cri_desc * work_cri_desc,
			     const unsigned short work_atp_index,
			     ex_cri_desc * given_cri_desc,
			     ex_cri_desc * returned_cri_desc,
			     queue_index down,
			     queue_index up,
			     Lng32 num_buffers,
			     ULng32 buffer_size)
     : ComTdbGenericUtil(query, querylen, querycharset, tableName, tableNameLen,
			 input_expr, input_rowlen,
			 output_expr, output_rowlen,
			 work_cri_desc, work_atp_index,
			 given_cri_desc, returned_cri_desc,
			 down, up, 
			 num_buffers, buffer_size),
       type_(type),
       child_(NULL),
       scanExpr_(scan_expr),
       flags_(0),
       explOptionsStr_(NULL)
{
  setNodeType(ComTdb::ex_EXE_UTIL);
}

Long ComTdbExeUtil::pack(void * space)
{
  child_.pack(space);
  scanExpr_.pack(space);
  if (explOptionsStr_) 
    explOptionsStr_.pack(space);
  if (NEOCatalogName_) 
    NEOCatalogName_.pack(space);
  return ComTdbGenericUtil::pack(space);
}

Lng32 ComTdbExeUtil::unpack(void * base, void * reallocator)
{
  if(child_.unpack(base, reallocator)) return -1;
  if(scanExpr_.unpack(base, reallocator)) return -1;
  if(explOptionsStr_.unpack(base))
    return -1;
  if(NEOCatalogName_.unpack(base)) 
    return -1;
  return ComTdbGenericUtil::unpack(base, reallocator);
}

const ComTdb* ComTdbExeUtil::getChild(Int32 pos) const
{
  if (pos == 0)
    return child_;
  else
    return NULL;
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilDisplayExplain
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilDisplayExplain::ComTdbExeUtilDisplayExplain
(char * query,
 ULng32 querylen,
 Int16 querycharset,
 char * moduleName,
 char * stmtName,
 ex_expr * input_expr,
 ULng32 input_rowlen,
 ex_expr * output_expr,
 ULng32 output_rowlen,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 Lng32 colDescSize,
 Lng32 outputRowSize,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::DISPLAY_EXPLAIN_,
		     query, querylen, querycharset,
		     NULL, 0,
		     input_expr, input_rowlen,
		     output_expr, output_rowlen,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       moduleName_(moduleName),
       stmtName_(stmtName),
       colDescSize_(colDescSize),
       outputRowSize_(outputRowSize),
       flags_(0)
{
  setNodeType(ComTdb::ex_DISPLAY_EXPLAIN);
}

Long ComTdbExeUtilDisplayExplain::pack(void * space)
{
  if (moduleName_) 
    moduleName_.pack(space);
  if (stmtName_) 
    stmtName_.pack(space);
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilDisplayExplain::unpack(void * base, void * reallocator)
{
  if(moduleName_.unpack(base))
    return -1;
  if(stmtName_.unpack(base))
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilDisplayExplain::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbExeUtilDisplayExplain :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "optionN = %d, optionF = %d, optionC = %d, optionP = %d, optionE = %d, optionM = %d", 
                  isOptionN(), isOptionF(), isOptionC(), isOptionP(),
                  isOptionE(), isOptionM());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilDisplayExplainComplex
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilDisplayExplainComplex::ComTdbExeUtilDisplayExplainComplex
(Lng32 explainType,
 char * qry1, char * qry2, char * qry3, char * qry4,
 char * objectName,
 Lng32 objectNameLen,
 ex_expr * input_expr,
 ULng32 input_rowlen,
 ex_expr * output_expr,
 ULng32 output_rowlen,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::DISPLAY_EXPLAIN_COMPLEX_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     objectName, objectNameLen,
		     input_expr, input_rowlen,
		     output_expr, output_rowlen,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       explainType_(explainType),
       qry1_(qry1), qry2_(qry2), qry3_(qry3), qry4_(qry4),
       flags_(0)
{
  setNodeType(ComTdb::ex_DISPLAY_EXPLAIN_COMPLEX);
}

Long ComTdbExeUtilDisplayExplainComplex::pack(void * space)
{
  if (qry1_)
    qry1_.pack(space);

  if (qry2_)
    qry2_.pack(space);

  if (qry3_)
    qry3_.pack(space);

  if (qry4_)
    qry4_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilDisplayExplainComplex::unpack(void * base, void * reallocator)
{
  if (qry1_.unpack(base))
    return -1;
  if (qry2_.unpack(base))
    return -1;
  if (qry3_.unpack(base))
    return -1;
  if (qry4_.unpack(base))
    return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilDisplayExplainComplex::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];

      str_sprintf(buf, "\nFor ComTdbExeUtilDisplayExplainComplex :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  
      if (qry1_)
	{
	  char query[400];
	  if (strlen(qry1_) > 390)
	    {
	      strncpy(query, qry1_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, qry1_);
	  
	  str_sprintf(buf,"Qry1 = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      
      if (qry2_)
	{
	  char query[400];
	  if (strlen(qry2_) > 390)
	    {
	      strncpy(query, qry2_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, qry2_);
	  
	  str_sprintf(buf,"Qry2 = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      
      if (qry3_)
	{
	  char query[400];
	  if (strlen(qry3_) > 390)
	    {
	      strncpy(query, qry3_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, qry3_);
	  
	  str_sprintf(buf,"Qry3 = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      
      if (qry4_)
	{
	  char query[400];
	  if (strlen(qry4_) > 390)
	    {
	      strncpy(query, qry4_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, qry4_);
	  
	  str_sprintf(buf,"Qry4 = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (objectName_)
	{
	  str_sprintf(buf,"ObjectName = %s ",getObjectName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
    }

  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilMaintainObject
//
//////////////////////////////////////////////////////////////////////////
ComTdbExeUtilMaintainObject::ComTdbExeUtilMaintainObject(
     char * objectName,
     ULng32 objectNameLen,
     char * schemaName,
     ULng32 schemaNameLen,
     UInt16 ot,
     char * parentTableName,
     ULng32 parentTableNameLen,
     ex_expr * input_expr,
     ULng32 input_rowlen,
     ex_expr * output_expr,
     ULng32 output_rowlen,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::MAINTAIN_OBJECT_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     objectName, objectNameLen,
		     input_expr, input_rowlen,
		     output_expr, output_rowlen,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       ot_(ot),
       schemaName_(schemaName),
       schemaNameLen_(schemaNameLen),
       parentTableName_(parentTableName),
       parentTableNameLen_(parentTableNameLen),
       flags_(0),
       controlFlags_(0),
       controlFlags2_(0),
       formatFlags_(0),
       maintainedTableCreateTime_(0),
       parentTableObjectUID_(0),
       from_(0),
       to_(0),
       flags2_(0)
{
  setNodeType(ComTdb::ex_MAINTAIN_OBJECT);
}

Long ComTdbExeUtilMaintainObject::pack(void * space)
{

  reorgTableOptions_.pack(space);
  reorgIndexOptions_.pack(space);
  updStatsTableOptions_.pack(space);
  updStatsMvlogOptions_.pack(space);
  updStatsMvsOptions_.pack(space);
  updStatsMvgroupOptions_.pack(space);
  refreshMvgroupOptions_.pack(space);
  refreshMvsOptions_.pack(space);
  reorgMvgroupOptions_.pack(space);
  reorgMvsOptions_.pack(space);
  reorgMvsIndexOptions_.pack(space);
  cleanMaintainCITOptions_.pack(space);

  indexList_.pack(space);
  refreshMvgroupList_.pack(space);
  refreshMvsList_.pack(space);
  reorgMvgroupList_.pack(space);
  reorgMvsList_.pack(space);
  reorgMvsIndexList_.pack(space);
  updStatsMvgroupList_.pack(space);
  updStatsMvsList_.pack(space);
  multiTablesNamesList_.pack(space);
  multiTablesCreateTimeList_.pack(space);
  skippedMultiTablesNamesList_.pack(space);

  if (parentTableName_) 
    parentTableName_.pack(space);
  if (schemaName_)
    schemaName_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilMaintainObject::unpack(void * base, void * reallocator)
{
  if(reorgTableOptions_.unpack(base)) return -1;
  if(reorgIndexOptions_.unpack(base)) return -1;
  if(updStatsTableOptions_.unpack(base)) return -1;
  if(updStatsMvlogOptions_.unpack(base)) return -1;
  if(updStatsMvsOptions_.unpack(base)) return -1;
  if(updStatsMvgroupOptions_.unpack(base)) return -1;
  if(refreshMvgroupOptions_.unpack(base)) return -1;
  if(refreshMvsOptions_.unpack(base)) return -1;
  if(reorgMvgroupOptions_.unpack(base)) return -1;
  if(reorgMvsOptions_.unpack(base)) return -1;
  if(reorgMvsIndexOptions_.unpack(base)) return -1;
  if(cleanMaintainCITOptions_.unpack(base)) return -1;

  if(indexList_.unpack(base, reallocator)) return -1;
  if(refreshMvgroupList_.unpack(base, reallocator)) return -1;
  if(refreshMvsList_.unpack(base, reallocator)) return -1;
  if(reorgMvgroupList_.unpack(base, reallocator)) return -1;
  if(reorgMvsList_.unpack(base, reallocator)) return -1;
  if(reorgMvsIndexList_.unpack(base, reallocator)) return -1;
  if(updStatsMvgroupList_.unpack(base, reallocator)) return -1;
  if(updStatsMvsList_.unpack(base, reallocator)) return -1;
  if(multiTablesNamesList_.unpack(base, reallocator)) return -1;
  if(multiTablesCreateTimeList_.unpack(base, reallocator)) return -1;
  if(skippedMultiTablesNamesList_.unpack(base, reallocator)) return -1;

  if(parentTableName_.unpack(base)) return -1;
  if(schemaName_.unpack(base)) return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilMaintainObject::setParams(NABoolean reorgTable,
					   NABoolean reorgIndex,
					   NABoolean updStatsTable,
					   NABoolean updStatsMvlog,
					   NABoolean updStatsMvs,
					   NABoolean updStatsMvgroup,
					   NABoolean refreshMvgroup,
					   NABoolean refreshMvs,
 					   NABoolean reorgMvgroup,
					   NABoolean reorgMvs,
					   NABoolean reorgMvsIndex,
 					   NABoolean continueOnError,
					   NABoolean cleanMaintainCIT,
					   NABoolean getSchemaLabelStats, 
					   NABoolean getLabelStats,
					    NABoolean getTableLabelStats,
					   NABoolean getIndexLabelStats,
					   NABoolean getLabelStatsIncIndexes,
					   NABoolean getLabelStatsIncInternal,
				           NABoolean getLabelStatsIncRelated
					    )
{
  setReorgTable(reorgTable);
  setReorgIndex(reorgIndex);
  setUpdStatsTable(updStatsTable);
  setUpdStatsMvlog(updStatsMvlog);
  setUpdStatsMvs(updStatsMvs);
  setUpdStatsMvgroup(updStatsMvgroup);
  setRefreshMvgroup(refreshMvgroup);
  setRefreshMvs(refreshMvs);
  setReorgMvgroup(reorgMvgroup);
  setReorgMvs(reorgMvs);
  setReorgMvsIndex(reorgMvsIndex);
  setContinueOnError(continueOnError);
  setCleanMaintainCIT(cleanMaintainCIT);
  setSchemaLabelStats(getSchemaLabelStats); 
  setTableLabelStats(getTableLabelStats); 
  setIndexLabelStats(getIndexLabelStats);
  setLabelStatsIncIndexes(getLabelStatsIncIndexes);
  setLabelStatsIncInternal(getLabelStatsIncInternal);
  setLabelStatsIncRelated(getLabelStatsIncRelated);

  
}

void ComTdbExeUtilMaintainObject::setOptionsParams
(char* reorgTableOptions,
 char* reorgIndexOptions,
 char* updStatsTableOptions,
 char* updStatsMvlogOptions,
 char* updStatsMvsOptions,
 char* updStatsMvgroupOptions,
 char* refreshMvgroupOptions,
 char* refreshMvsOptions,
 char* reorgMvgroupOptions,
 char* reorgMvsOptions,
 char* reorgMvsIndexOptions,
 char* cleanMaintainCITOptions)
{
  reorgTableOptions_ = reorgTableOptions;
  reorgIndexOptions_ = reorgIndexOptions;
  updStatsTableOptions_ = updStatsTableOptions;
  updStatsMvlogOptions_ = updStatsMvlogOptions;
  updStatsMvsOptions_ = updStatsMvsOptions;
  updStatsMvgroupOptions_ = updStatsMvgroupOptions;
  refreshMvgroupOptions_ = refreshMvgroupOptions;
  refreshMvsOptions_ = refreshMvsOptions;
  reorgMvgroupOptions_ = reorgMvgroupOptions;
  reorgMvsOptions_ = reorgMvsOptions;
  reorgMvsIndexOptions_ = reorgMvsIndexOptions;
  cleanMaintainCITOptions_ = cleanMaintainCITOptions;
}

void ComTdbExeUtilMaintainObject::setLists(Queue* indexList,
					   Queue* refreshMvgroupList,
					   Queue* refreshMvsList,
					   Queue* reorgMvgroupList,
					   Queue* reorgMvsList,
					   Queue* reorgMvsIndexList,
					   Queue* updStatsMvgroupList,
					   Queue* updStatsMvsList,
					   Queue* multiTablesNamesList,
					   Queue* skippedMultiTablesNamesList)
{
  indexList_    = indexList;
  refreshMvgroupList_ = refreshMvgroupList;
  refreshMvsList_    = refreshMvsList;
  reorgMvgroupList_ = reorgMvgroupList;
  reorgMvsList_      = reorgMvsList;
  reorgMvsIndexList_ = reorgMvsIndexList;
  updStatsMvgroupList_ = updStatsMvgroupList;
  updStatsMvsList_      = updStatsMvsList;
  multiTablesNamesList_ = multiTablesNamesList;
  skippedMultiTablesNamesList_ = skippedMultiTablesNamesList;
 }

void ComTdbExeUtilMaintainObject::setControlParams
(NABoolean disableReorgTable, 
 NABoolean enableReorgTable, 
 NABoolean disableReorgIndex,
 NABoolean enableReorgIndex,
 NABoolean disableUpdStatsTable,
 NABoolean enableUpdStatsTable,
 NABoolean disableUpdStatsMvs,
 NABoolean enableUpdStatsMvs,
 NABoolean disableRefreshMvs,
 NABoolean enableRefreshMvs,
 NABoolean disableReorgMvs,
 NABoolean enableReorgMvs,
 NABoolean resetReorgTable,
 NABoolean resetUpdStatsTable,
 NABoolean resetUpdStatsMvs,
 NABoolean resetRefreshMvs,
 NABoolean resetReorgMvs,
 NABoolean resetReorgIndex,
 NABoolean enableUpdStatsMvlog,
 NABoolean disableUpdStatsMvlog,
 NABoolean resetUpdStatsMvlog,
 NABoolean enableReorgMvsIndex,
 NABoolean disableReorgMvsIndex,
 NABoolean resetReorgMvsIndex,
 NABoolean enableRefreshMvgroup,
 NABoolean disableRefreshMvgroup,
 NABoolean resetRefreshMvgroup,
 NABoolean enableReorgMvgroup,
 NABoolean disableReorgMvgroup,
 NABoolean resetReorgMvgroup,
 NABoolean enableUpdStatsMvgroup,
 NABoolean disableUpdStatsMvgroup,
 NABoolean resetUpdStatsMvgroup,
 NABoolean enableTableLabelStats,
 NABoolean disableTableLabelStats,
 NABoolean resetTableLabelStats,
 NABoolean enableIndexLabelStats,
 NABoolean disableIndexLabelStats,
 NABoolean resetIndexLabelStats
)
{
  setDisableReorgTable(disableReorgTable);
  setDisableReorgIndex(disableReorgIndex);
  setDisableUpdStatsTable(disableUpdStatsTable);
  setDisableUpdStatsMvs(disableUpdStatsMvs);
  setDisableRefreshMvs(disableRefreshMvs);
  setDisableReorgMvs(disableReorgMvs);
  setEnableReorgTable(enableReorgTable);
  setEnableReorgIndex(enableReorgIndex);
  setEnableUpdStatsTable(enableUpdStatsTable);
  setEnableUpdStatsMvs(enableUpdStatsMvs);
  setEnableRefreshMvs(enableRefreshMvs);
  setEnableReorgMvs(enableReorgMvs);
  setResetReorgTable(resetReorgTable);
  setResetUpdStatsTable(resetUpdStatsTable);
  setResetUpdStatsMvs(resetUpdStatsMvs);
  setResetRefreshMvs(resetRefreshMvs);
  setResetReorgMvs(resetReorgMvs);
  setResetReorgIndex(resetReorgIndex);
  setEnableUpdStatsMvlog(enableUpdStatsMvlog);
  setDisableUpdStatsMvlog(disableUpdStatsMvlog);
  setResetUpdStatsMvlog(resetUpdStatsMvlog);
  setEnableReorgMvsIndex(enableReorgMvsIndex);
  setDisableReorgMvsIndex(disableReorgMvsIndex);
  setResetReorgMvsIndex(resetReorgMvsIndex);
  setEnableRefreshMvgroup(enableRefreshMvgroup);
  setDisableRefreshMvgroup(disableRefreshMvgroup);
  setResetRefreshMvgroup(resetRefreshMvgroup);
  setEnableReorgMvgroup(enableReorgMvgroup);
  setDisableReorgMvgroup(disableReorgMvgroup);
  setResetReorgMvgroup(resetReorgMvgroup);
  setEnableUpdStatsMvgroup(enableUpdStatsMvgroup);
  setDisableUpdStatsMvgroup(disableUpdStatsMvgroup);
  setResetUpdStatsMvgroup(resetUpdStatsMvgroup);
  setEnableTableLabelStats(enableTableLabelStats);
  setDisableTableLabelStats(disableTableLabelStats);
  setResetTableLabelStats(resetTableLabelStats);
  setEnableIndexLabelStats(enableIndexLabelStats);
  setDisableIndexLabelStats(disableIndexLabelStats);
  setResetIndexLabelStats(resetIndexLabelStats);

}

void ComTdbExeUtilMaintainObject::displayContents(Space * space,
						 ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilMaintainObject :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (getTableName() != NULL)
	{
	  str_sprintf(buf,"Tablename = %s ",getTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					       sizeof(short));
	}
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}


///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilLoadVolatileTable
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilLoadVolatileTable::ComTdbExeUtilLoadVolatileTable
(char * tableName,
 ULng32 tableNameLen,
 char * insertQuery,
 char * updStatsQuery,
 Int16 queryCharSet,
 Int64 threshold,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::LOAD_VOLATILE_TABLE_,
		     NULL, 0, queryCharSet/*for insertQuery & updStatsQuery*/,
		     tableName, tableNameLen,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       insertQuery_(insertQuery),
       updStatsQuery_(updStatsQuery),
       threshold_(threshold),
       flags_(0)
{
  setNodeType(ComTdb::ex_LOAD_VOLATILE_TABLE);
}

Long ComTdbExeUtilLoadVolatileTable::pack(void * space)
{
  if (insertQuery_) 
    insertQuery_.pack(space);
  if (updStatsQuery_) 
    updStatsQuery_.pack(space);
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilLoadVolatileTable::unpack(void * base, void * reallocator)
{
  if(insertQuery_.unpack(base))
    return -1;
  if(updStatsQuery_.unpack(base))
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilLoadVolatileTable::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilLoadVolatileTable :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (getTableName() != NULL)
	{
	  str_sprintf(buf,"Tablename = %s ",getTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (insertQuery_)
	{
	  char query[400];
	  if (strlen(insertQuery_) > 390)
	    {
	      strncpy(query, insertQuery_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, insertQuery_);

	  str_sprintf(buf,"Insert Query = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (updStatsQuery_)
	{
	  char query[400];
	  if (strlen(updStatsQuery_) > 390)
	    {
	      strncpy(query, updStatsQuery_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, updStatsQuery_);

	  str_sprintf(buf,"UpdStats Query = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      str_sprintf(buf,"Threshold = %ld ", threshold_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}


///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilCleanupVolatileTables
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilCleanupVolatileTables::ComTdbExeUtilCleanupVolatileTables
(char * catName,
 ULng32 catNameLen,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::CLEANUP_VOLATILE_SCHEMA_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     catName, catNameLen,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0)
{
  setNodeType(ComTdb::ex_CLEANUP_VOLATILE_TABLES);
}

void ComTdbExeUtilCleanupVolatileTables::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilCleanupVolatileTables :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (getTableName() != NULL)
	{
	  str_sprintf(buf,"Tablename = %s ",getTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}


///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilGetVotalileInfo
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilGetVolatileInfo::ComTdbExeUtilGetVolatileInfo
(
 char * param1,
 char * param2,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::GET_VOLATILE_INFO,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       param1_(param1), param2_(param2)
{
  setNodeType(ComTdb::ex_GET_VOLATILE_INFO);
}

Long ComTdbExeUtilGetVolatileInfo::pack(void * space)
{
  if (param1_) 
    param1_.pack(space);
  if (param2_) 
    param2_.pack(space);
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilGetVolatileInfo::unpack(void * base, void * reallocator)
{
  if(param1_.unpack(base))
    return -1;
  if(param2_.unpack(base))
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilGetVolatileInfo::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilGetVotalileInfo :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (getTableName() != NULL)
	{
	  str_sprintf(buf,"Tablename = %s ",getTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilGetErrorInfo
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilGetErrorInfo::ComTdbExeUtilGetErrorInfo
(
 Lng32 errNum,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::GET_ERROR_INFO_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       errNum_(errNum)
{
  setNodeType(ComTdb::ex_GET_ERROR_INFO);
}

void ComTdbExeUtilGetErrorInfo::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilGetErrorInfo :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf,"ErrorNum = %d ", errNum_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilCreateTableAs
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilCreateTableAs::ComTdbExeUtilCreateTableAs
(char * tableName,
 ULng32 tableNameLen,
 char * ctQuery,
 char * siQuery,
 char * viQuery,
 char * usQuery,
 Int64 threshold,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::CREATE_TABLE_AS_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     tableName, tableNameLen,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       ctQuery_(ctQuery), 
       siQuery_(siQuery), 
       viQuery_(viQuery),
       usQuery_(usQuery),
       threshold_(threshold),
       flags_(0)
{
  setNodeType(ComTdb::ex_CREATE_TABLE_AS);
}

Long ComTdbExeUtilCreateTableAs::pack(void * space)
{
  if (ctQuery_) 
    ctQuery_.pack(space);
  if (siQuery_) 
    siQuery_.pack(space);
  if (viQuery_) 
    viQuery_.pack(space);
  if (usQuery_) 
    usQuery_.pack(space);
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilCreateTableAs::unpack(void * base, void * reallocator)
{
  if(ctQuery_.unpack(base))
    return -1;
  if(siQuery_.unpack(base))
    return -1;
  if(viQuery_.unpack(base))
    return -1;
  if(usQuery_.unpack(base))
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilCreateTableAs::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilCreateTableAs :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (getTableName() != NULL)
	{
	  str_sprintf(buf,"Tablename = %s ",getTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (ctQuery_)
	{
	  char query[400];
	  if (strlen(ctQuery_) > 390)
	    {
	      strncpy(query, ctQuery_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, ctQuery_);

	  str_sprintf(buf,"Create Query = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (siQuery_)
	{
	  char query[400];
	  if (strlen(siQuery_) > 390)
	    {
	      strncpy(query, siQuery_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, siQuery_);

	  str_sprintf(buf,"Sidetree Insert Query = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (viQuery_)
	{
	  char query[400];
	  if (strlen(viQuery_) > 390)
	    {
	      strncpy(query, viQuery_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, viQuery_);

	  str_sprintf(buf,"VSBB Insert Query = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (usQuery_)
	{
	  char query[400];
	  if (strlen(usQuery_) > 390)
	    {
	      strncpy(query, usQuery_, 390);
	      query[390] = 0;
	      strcat(query, "...");
	    }
	  else
	    strcpy(query, usQuery_);

	  str_sprintf(buf,"UpdStats Query = %s ",query);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilHiveTruncate
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilHiveTruncate::ComTdbExeUtilHiveTruncate(
     char * tableName,
     ULng32 tableNameLen,
     char * hiveTableName,
     char * tableLocation,
     char * partnLocation,
     char * hostName,
     Lng32 portNum,
     Int64 modTS,
     char * hiveTruncQuery,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::HIVE_TRUNCATE_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     tableName, tableNameLen,
		     NULL, 0,
		     NULL, 0,
		     NULL,
                     NULL, 0,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       hiveTableName_(hiveTableName),
       tableLocation_(tableLocation),
       partnLocation_(partnLocation),
       hdfsHost_(hostName),
       hdfsPort_(portNum),
       modTS_(modTS),
       hiveTruncQuery_(hiveTruncQuery)
{
  setNodeType(ComTdb::ex_HIVE_TRUNCATE);
}

Long ComTdbExeUtilHiveTruncate::pack(void * space)
{
  if (tableLocation_)
    tableLocation_.pack(space);

  if (hdfsHost_)
    hdfsHost_.pack(space);

  if (partnLocation_)
    partnLocation_.pack(space);

  if (hiveTableName_)
    hiveTableName_.pack(space);

  if (hiveTruncQuery_)
    hiveTruncQuery_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilHiveTruncate::unpack(void * base, void * reallocator)
{
  if(tableLocation_.unpack(base))
    return -1;
  
  if(hdfsHost_.unpack(base))
    return -1;

  if (partnLocation_.unpack(base))
    return -1;

  if (hiveTableName_.unpack(base))
    return -1;

  if (hiveTruncQuery_.unpack(base))
    return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilHiveTruncate::displayContents(Space * space,
					      ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[500];
      str_sprintf(buf, "\nFor ComTdbExeUtilHiveTruncate :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (getTableName() != NULL)
	{
	  str_sprintf(buf,"Tablename = %s ",getTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					       sizeof(short));
	}

      if (getHiveTableName() != NULL)
	{
	  str_sprintf(buf,"Hive Tablename = %s ",getHiveTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					       sizeof(short));
	}

      if (getTableLocation() != NULL)
	{
	  str_sprintf(buf,"tableLocation_ = %s ", getTableLocation());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					       sizeof(short));
	}

      if (getPartnLocation() != NULL)
	{
	  str_sprintf(buf,"partnLocation_ = %s ", getPartnLocation());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					       sizeof(short));
	}
 
      if (getHiveTruncQuery() != NULL)
        {
          
        }
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }

}

///////////////////////////////////////////////////////////////////////////
//
ComTdbExeUtilHiveQuery::ComTdbExeUtilHiveQuery(
     char * hiveQuery,
     ULng32 hiveQueryLen,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::HIVE_QUERY_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     NULL,
                     NULL, 0,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       hiveQuery_(hiveQuery),
       hiveQueryLen_(hiveQueryLen)
{
  setNodeType(ComTdb::ex_HIVE_QUERY);
}
Long ComTdbExeUtilHiveQuery::pack(void * space)
{
  if (hiveQuery_)
    hiveQuery_.pack(space);
  return ComTdbExeUtil::pack(space);
}
Lng32 ComTdbExeUtilHiveQuery::unpack(void * base, void * reallocator)
{
  if(hiveQuery_.unpack(base))
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}
void ComTdbExeUtilHiveQuery::displayContents(Space * space,
					      ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  if(flag & 0x00000008)
    {
      char buf[500];
      str_sprintf(buf, "\nFor ComTdbExeUtilHiveQuery :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      if (getHiveQuery() != NULL)
	{
	  str_sprintf(buf,"HiveQuery = %s ",getHiveQuery());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					       sizeof(short));
	}
    }
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}
// Methods for class ComTdbExeUtilGetStatistics
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilGetStatistics::ComTdbExeUtilGetStatistics
(
     char * stmtName,
     short statsReqType,
     short statsMergeType,
     short activeQueryNum,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::GET_STATISTICS_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       stmtName_(stmtName),
       statsReqType_(statsReqType),
       statsMergeType_(statsMergeType),
       activeQueryNum_(activeQueryNum)
{
  setNodeType(ComTdb::ex_GET_STATISTICS);
}

Long ComTdbExeUtilGetStatistics::pack(void * space)
{
  if (stmtName_) 
    stmtName_.pack(space);
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilGetStatistics::unpack(void * base, void * reallocator)
{
  if(stmtName_.unpack(base))
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilGetStatistics::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilGetStatistics :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if ( stmtName_ != (NABasicPtr)NULL )
	{
	  str_sprintf(buf,"StmtName = %s ", getStmtName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

void ComTdbExeUtilGetProcessStatistics::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilGetProcessStatistics :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if ( stmtName_ != (NABasicPtr)NULL )
	{
	  str_sprintf(buf,"Pid = %s ", getPid());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilGetUID
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilGetUID::ComTdbExeUtilGetUID
(
     Int64 uid,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::GET_UID_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       uid_(uid)
{
  setNodeType(ComTdb::ex_GET_UID);
}

Long ComTdbExeUtilGetUID::pack(void * space)
{
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilGetUID::unpack(void * base, void * reallocator)
{
  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilGetUID::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbExeUtilGetUID :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf,"UID = %ld", uid_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilGetQID
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilGetQID::ComTdbExeUtilGetQID
(
 char * stmtName,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
  : ComTdbExeUtil(ComTdbExeUtil::GET_QID_,
                  NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
                  NULL, 0,
                  NULL, 0,
                  NULL, 0,
                  NULL,
                  work_cri_desc, work_atp_index,
                  given_cri_desc, returned_cri_desc,
                  down, up, 
                  num_buffers, buffer_size),
    flags_(0),
    stmtName_(stmtName)
{
  setNodeType(ComTdb::ex_GET_QID);
}

Long ComTdbExeUtilGetQID::pack(void * space)
{
  if (stmtName_)
    stmtName_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilGetQID::unpack(void * base, void * reallocator)
{
  if (stmtName_.unpack(base))
    return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilGetQID::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbExeUtilGetQID :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf,"stmtName_ = %s ", getStmtName());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilPopulateInMemStats
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilPopulateInMemStats::ComTdbExeUtilPopulateInMemStats
(
     Int64 uid,
     char * inMemHistogramsTableName,
     char * inMemHistintsTableName,
     char * sourceTableCatName,
     char * sourceTableSchName,
     char * sourceTableObjName,
     char * sourceHistogramsTableName,
     char * sourceHistintsTableName,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::POP_IN_MEM_STATS_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       uid_(uid),
       inMemHistogramsTableName_(inMemHistogramsTableName),
       inMemHistintsTableName_(inMemHistintsTableName),
       sourceTableCatName_(sourceTableCatName),
       sourceTableSchName_(sourceTableSchName),
       sourceTableObjName_(sourceTableObjName),
       sourceHistogramsTableName_(sourceHistogramsTableName),
       sourceHistintsTableName_(sourceHistintsTableName)
{
  setNodeType(ComTdb::ex_POP_IN_MEM_STATS);
}

Long ComTdbExeUtilPopulateInMemStats::pack(void * space)
{
  if (inMemHistogramsTableName_)
    inMemHistogramsTableName_.pack(space);
  if (inMemHistintsTableName_)
    inMemHistintsTableName_.pack(space);
  if (sourceTableCatName_)
    sourceTableCatName_.pack(space);
  if (sourceTableSchName_)
    sourceTableSchName_.pack(space);
  if (sourceTableObjName_)
    sourceTableObjName_.pack(space);
  if (sourceHistogramsTableName_)
    sourceHistogramsTableName_.pack(space);
  if (sourceHistintsTableName_)
    sourceHistintsTableName_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilPopulateInMemStats::unpack(void * base, void * reallocator)
{
  if (inMemHistogramsTableName_.unpack(base))
    return -1;
  if (inMemHistintsTableName_.unpack(base))
    return -1;
  if (sourceTableCatName_.unpack(base))
    return -1;
  if (sourceTableSchName_.unpack(base))
    return -1;
  if (sourceTableObjName_.unpack(base))
    return -1;
  if (sourceHistogramsTableName_.unpack(base))
    return -1;
  if (sourceHistintsTableName_.unpack(base))
    return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilPopulateInMemStats::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilPopulateInMemStats :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf,"UID = %ld", uid_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if ((char *)inMemHistogramsTableName_ != (char *)NULL)
	{
	  str_sprintf(buf,"inMemHistogramsTableName_ = %s ", getInMemHistogramsTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      if ((char *)inMemHistintsTableName_ != (char *)NULL)
	{
	  str_sprintf(buf,"inMemHistintsTableName_ = %s ", getInMemHistintsTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if ((char *)sourceTableCatName_ != (char *)NULL)
	{
	  str_sprintf(buf,"sourceTableCatName_ = %s ", getSourceTableCatName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      if ((char *)sourceTableSchName_ != (char *)NULL)
	{
	  str_sprintf(buf,"sourceTableSchName_ = %s ", getSourceTableSchName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      if ((char *)sourceTableObjName_ != (char *)NULL)
	{
	  str_sprintf(buf,"sourceTableCatName_ = %s ", getSourceTableObjName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      if ((char *)sourceHistogramsTableName_ != (char *)NULL)
	{
	  str_sprintf(buf,"sourceHistogramsTableName_ = %s ", getSourceHistogramsTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      if ((char *)sourceHistintsTableName_ != (char *)NULL)
	{
	  str_sprintf(buf,"sourceHistintsTableName_ = %s ", getSourceHistintsTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
	
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilAqrWnrInsert
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilAqrWnrInsert::ComTdbExeUtilAqrWnrInsert(
     char * tableName,
     ULng32 tableNameLen,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::AQR_WNR_INSERT_,
		     (char *) eye_AQR_WNR_INS, 
                     0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     tableName, tableNameLen,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       aqrWnrInsflags_(0)
{
  setNodeType(ComTdb::ex_ARQ_WNR_INSERT);
}

void ComTdbExeUtilAqrWnrInsert::displayContents(Space * space,
					      ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilAqrWnrInsert:");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if (getTableName() != NULL)
        {
          str_sprintf(buf,"Tablename = %s ",getTableName());
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					           sizeof(short));
        }
      str_sprintf(buf, "Lock target = %s ",  doLockTarget() ? "ON" : "OFF");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					       sizeof(short));

    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}
//////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilLongRunning
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilLongRunning::ComTdbExeUtilLongRunning(
     char * tableName,
     ULng32 tableNameLen,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::LONG_RUNNING_,
 		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     tableName, tableNameLen,
  		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       lruStmt_(NULL),
       lruStmtLen_(0),
       lruStmtWithCK_(NULL),
       lruStmtWithCKLen_(0),
       predicate_(NULL),
       predicateLen_(0),
       multiCommitSize_(0)
{
  setNodeType(ComTdb::ex_LONG_RUNNING);
}

Long ComTdbExeUtilLongRunning::pack(void * space)
{
  if (lruStmt_)
    lruStmt_.pack(space);

  if(lruStmtWithCK_)
    lruStmtWithCK_.pack(space);

  if (predicate_)
    predicate_.pack(space); 

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilLongRunning::unpack(void * base, void * reallocator)
{

  if(lruStmt_.unpack(base))
    return -1;
  
  if(lruStmtWithCK_.unpack(base))
    return -1;

  if (predicate_.unpack(base))
     return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilLongRunning::setPredicate(Space *space, char *predicate)
{
   if (predicate != NULL)
   {
      predicateLen_ = strlen(predicate);
      predicate_ = space->allocateAlignedSpace
        ((ULng32)predicateLen_ + 1);
      strcpy(predicate_, predicate);
   }
}

void ComTdbExeUtilLongRunning::displayContents(Space * space,
					      ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilLongRunning :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (getTableName() != NULL)
	{
	  str_sprintf(buf,"Tablename = %s ",getTableName());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), 
					       sizeof(short));
	}
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
    }
}




///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilGetMetadataInfo
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilGetMetadataInfo::ComTdbExeUtilGetMetadataInfo
(
     QueryType queryType,
     char *    cat,
     char *    sch,
     char *    obj,
     char *    pattern,
     char *    param1,
     ex_expr_base * scan_expr,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size,
     char * server,
     char * zkPort)
     : ComTdbExeUtil(ComTdbExeUtil::GET_METADATA_INFO_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     scan_expr,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       queryType_(queryType),
       cat_(cat), sch_(sch), obj_(obj),
       pattern_(pattern),
       param1_(param1),
       flags_(0),
       server_(server),
       zkPort_(zkPort)
{
  setNodeType(ComTdb::ex_GET_METADATA_INFO);
}

Long ComTdbExeUtilGetMetadataInfo::pack(void * space)
{
  if (cat_) 
    cat_.pack(space);
  if (sch_) 
    sch_.pack(space);
  if (obj_) 
    obj_.pack(space);
  if (pattern_) 
    pattern_.pack(space);
  if (param1_) 
    param1_.pack(space);
  if (server_)
    server_.pack(space);
  if (zkPort_)
    zkPort_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilGetMetadataInfo::unpack(void * base, void * reallocator)
{
  if (cat_.unpack(base))
    return -1;
  if (sch_.unpack(base))
    return -1;
  if (obj_.unpack(base))
    return -1;
  if (pattern_.unpack(base))
    return -1;
  if (param1_.unpack(base))
    return -1;
  if (server_.unpack(base))
    return -1;
  if (zkPort_.unpack(base))
    return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilGetMetadataInfo::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilGetMetadataInfo :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf, "QueryType: %d", queryType_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if (getCat() != NULL)
	{
	  str_sprintf(buf,"Catalog = %s ", getCat());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (getSch() != NULL)
	{
	  str_sprintf(buf,"Schema = %s ", getSch());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (getObj() != NULL)
	{
	  str_sprintf(buf,"Object = %s ", getObj());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (getPattern() != NULL)
	{
	  str_sprintf(buf,"Pattern = %s ", getPattern());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (getParam1() != NULL)
	{
	  str_sprintf(buf,"Param1 = %s ", getParam1());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      str_sprintf(buf, "Flags = %x",flags_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilGetHiveMetadataInfo
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilGetHiveMetadataInfo::ComTdbExeUtilGetHiveMetadataInfo
(
     QueryType queryType,
     char *    cat,
     char *    sch,
     char *    obj,
     char *    pattern,
     char *    param1,
     ex_expr_base * scan_expr,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtilGetMetadataInfo(
				    queryType,
				    cat, sch, obj, pattern, param1,
				    scan_expr,
				    work_cri_desc, work_atp_index,
				    given_cri_desc, returned_cri_desc,
				    down, up, 
				    num_buffers, buffer_size,
                                    NULL, NULL),
       unused1_(NULL),
       unused2_(NULL),
       unused3_(NULL),
       unused4_(NULL)
{
  setType(ComTdbExeUtil::GET_HIVE_METADATA_INFO_);
  setNodeType(ComTdb::ex_GET_HIVE_METADATA_INFO);
}

Long ComTdbExeUtilGetHiveMetadataInfo::pack(void * space)
{
  return ComTdbExeUtilGetMetadataInfo::pack(space);
}

Lng32 ComTdbExeUtilGetHiveMetadataInfo::unpack(void * base, void * reallocator)
{
  return ComTdbExeUtilGetMetadataInfo::unpack(base, reallocator);
}

void ComTdbExeUtilGetHiveMetadataInfo::displayContents(Space * space,ULng32 flag)
{
  ComTdbExeUtilGetMetadataInfo::displayContents(space,flag & 0xFFFFFFFE);
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}


///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilShowSet
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilShowSet::ComTdbExeUtilShowSet
(
     UInt16 type,
     char * param1,
     char * param2,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::SHOW_SET_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       type_(type), flags_(0),
       param1_(param1), param2_(param2)
{
  setNodeType(ComTdb::ex_SHOW_SET);
}

Long ComTdbExeUtilShowSet::pack(void * space)
{
  if (param1_) 
    param1_.pack(space);
  if (param2_) 
    param2_.pack(space);
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilShowSet::unpack(void * base, void * reallocator)
{
  if(param1_.unpack(base))
    return -1;
  if(param2_.unpack(base))
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilShowSet::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbExeUtilShowSet :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilAQR
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilAQR::ComTdbExeUtilAQR
(
     Lng32 task,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::AQR_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     NULL, 0,
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     NULL, 0,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       task_(task),
       flags_(0)
{
  setNodeType(ComTdb::ex_AQR);
}

Long ComTdbExeUtilAQR::pack(void * space)
{
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilAQR::unpack(void * base, void * reallocator)
{
  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilAQR::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbExeUtilAQR :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}


///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilLobExtract
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilLobExtract::ComTdbExeUtilLobExtract
(
 char * handle,
 Lng32 handleLen,
 ExtractToType toType,
 Int64 bufAddr,
 Int64 extractSizeAddr,
 Int64 intParam1,
 Int64 intParam2,
 Lng32 lobStorageType,
 char * stringParam1,
 char * stringParam2,
 char * stringParam3,
 char * lobHdfsServer,
 Lng32 lobHdfsPort,
  ex_expr * input_expr,
 ULng32 input_rowlen,
 ex_cri_desc * work_cri_desc,
 const unsigned short work_atp_index,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
  : ComTdbExeUtil(ComTdbExeUtil::LOB_EXTRACT_,
		  NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		  NULL, 0,
		  input_expr, input_rowlen,
		  NULL, 0,
		  NULL,
		  work_cri_desc, work_atp_index,
		  given_cri_desc, returned_cri_desc,
		  down, up, 
		  num_buffers, buffer_size),
    handle_(handle),
    handleLen_(handleLen),
    toType_((short)toType),
    bufAddr_(bufAddr),
    extractSizeIOAddr_(extractSizeAddr),
    lobStorageType_(lobStorageType),
    stringParam1_(stringParam1),
    stringParam2_(stringParam2),
    stringParam3_(stringParam3),
    lobHdfsServer_(lobHdfsServer),
    lobHdfsPort_(lobHdfsPort),
    totalBufSize_(0),
    flags_(0)
{
  setNodeType(ComTdb::ex_LOB_EXTRACT);
  if (toType_ == ExtractToType::TO_FILE_)
      {
	// extractSize_ is irrelevant since the whole lob will be read into the output file
	// bufAddr_ is not passed in by user. It is a CQD value LOB_MAX_CHUNK_MEM_SIZE
	extractSizeIOAddr_ = 0;
	bufAddr_ = 0;
	
      }

}

Long ComTdbExeUtilLobExtract::pack(void * space)
{
  if (handle_)
    handle_.pack(space);

  if (stringParam1_)
    stringParam1_.pack(space);
  
  if (stringParam2_)
    stringParam2_.pack(space);
  
  if (stringParam3_)
    stringParam3_.pack(space);
  
  if (lobHdfsServer_)
    lobHdfsServer_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilLobExtract::unpack(void * base, void * reallocator)
{
  if (handle_.unpack(base))
    return -1;

  if (stringParam1_.unpack(base))
    return -1;

  if (stringParam2_.unpack(base))
    return -1;

  if (stringParam3_.unpack(base))
    return -1;

  if (lobHdfsServer_.unpack(base))
    return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilLobExtract::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbExeUtilLobExtract :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}
///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilLobUpdate
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilLobUpdate::ComTdbExeUtilLobUpdate
(
     char * handle,
     Lng32 handleLen,
     UpdateFromType fromType,
     Int64 bufAddr,
     Int64 updateSize,
     Int32 lobStorageType,
     char * lobHdfsServer,
     Lng32 lobHdfsPort,
     char *lobLoc,
     ex_expr * input_expr,
     ULng32 input_rowlen,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
  : ComTdbExeUtil(ComTdbExeUtil::LOB_UPDATE_UTIL_,
                   NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
                   NULL, 0,
                   input_expr, input_rowlen,
                   NULL, 0,
                   NULL,
                   work_cri_desc, work_atp_index,
                   given_cri_desc, returned_cri_desc,
                   down, up, 
                   num_buffers, buffer_size),
    handle_(handle),
    handleLen_(handleLen),
    fromType_((short)fromType),
    bufAddr_(bufAddr),
    updateSize_(updateSize),
    lobStorageType_(lobStorageType),
    lobHdfsServer_(lobHdfsServer),
    lobHdfsPort_(lobHdfsPort),
    lobLoc_(lobLoc),
    totalBufSize_(0),
    flags_(0)
{
  setNodeType(ComTdb::ex_LOB_UPDATE_UTIL);
}
Long ComTdbExeUtilLobUpdate::pack(void * space)
{
  if (handle_)
    handle_.pack(space);
  if (lobHdfsServer_)
    lobHdfsServer_.pack(space);
  if (lobLoc_)
    lobLoc_.pack(space);
  return ComTdbExeUtil::pack(space);
}
Lng32 ComTdbExeUtilLobUpdate::unpack(void * base, void * reallocator)
{
  if (handle_.unpack(base))
    return -1;
  if (lobHdfsServer_.unpack(base))
    return -1;
  if(lobLoc_.unpack(base))
    return -1;
 return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilLobUpdate::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbExeUtilLobUpdate :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}
///////////////////////////////////////////////////////////////////////////
//
// Methods for class ComTdbExeUtilLobShowddl
//
///////////////////////////////////////////////////////////////////////////
ComTdbExeUtilLobShowddl::ComTdbExeUtilLobShowddl
(
 char * tableName,
 char * schName,
 short schNameLen,
 Int64 objectUID,
 Lng32 numLOBs,
 char * lobNumArray,
 char * lobLocArray,
 char * lobTypeArray,
 short maxLocLen,
 short sdOptions,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up,
 Lng32 num_buffers,
 ULng32 buffer_size)
  : ComTdbExeUtil(ComTdbExeUtil::LOB_SHOWDDL_,
		  NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		  tableName, strlen(tableName),
		  NULL, 0,
		  NULL, 0,
		  NULL,
		  NULL, 0,
		  given_cri_desc, returned_cri_desc,
		  down, up, 
		  num_buffers, buffer_size),
    flags_(0),
    objectUID_(objectUID),
    numLOBs_(numLOBs),
    lobNumArray_(lobNumArray),
    lobLocArray_(lobLocArray),
    lobTypeArray_(lobTypeArray),
    maxLocLen_(maxLocLen),
    sdOptions_(sdOptions),
    schName_(schName),
    schNameLen_(schNameLen)
{
  setNodeType(ComTdb::ex_LOB_SHOWDDL);
}

Long ComTdbExeUtilLobShowddl::pack(void * space)
{
  if (schName_) 
    schName_.pack(space);

 if (lobNumArray_) 
    lobNumArray_.pack(space);

 if (lobLocArray_) 
    lobLocArray_.pack(space);

 if (lobTypeArray_)
   lobTypeArray_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilLobShowddl::unpack(void * base, void * reallocator)
{
  if(schName_.unpack(base))
    return -1;

  if(lobNumArray_.unpack(base)) 
    return -1;

  if(lobLocArray_.unpack(base)) 
    return -1;

  if(lobTypeArray_.unpack(base))
    return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilLobShowddl::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbExeUtilLobShowddl :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}


short ComTdbExeUtilLobShowddl::getLOBnum(short i)
{
  if ((i > numLOBs_) || (i <= 0))
    return -1;

  short lobNum = *((short*)&getLOBnumArray()[2*(i-1)]);

  return lobNum;
}

NABoolean ComTdbExeUtilLobShowddl::getIsExternalLobCol(short i)
{

  NABoolean isExternal = (*((Int32*)&getLOBtypeArray()[4*(i-1)]) == Lob_External_HDFS_File);

  return isExternal;
}
char * ComTdbExeUtilLobShowddl::getLOBloc(short i)
{
  if ((i > numLOBs_) || (i <= 0))
    return NULL;

  char * lobLoc = &getLOBlocArray()[maxLocLen_*(i-1)];

  return lobLoc;
}
/////////////////////////////////////////////////////////////////////////////////
// class ComTdbExeUtilHiveMDaccess
/////////////////////////////////////////////////////////////////////////////////
ComTdbExeUtilHiveMDaccess::ComTdbExeUtilHiveMDaccess() 
  : ComTdbExeUtil()
{
}

ComTdbExeUtilHiveMDaccess::ComTdbExeUtilHiveMDaccess(
			   MDType type,
			   ULng32 tupleLen,
			   ex_cri_desc *criDescParentDown,
			   ex_cri_desc *criDescParentUp,
			   ex_cri_desc *workCriDesc,
			   unsigned short workAtpIndex,
			   queue_index queueSizeDown,
			   queue_index queueSizeUp,
			   Lng32 numBuffers,
			   ULng32 bufferSize,
			   ex_expr *scanPred,
			   char * hivePredStr,
                           char * catalogName,
                           char * schemaName,
                           char * objectName)
  : ComTdbExeUtil(ComTdbExeUtil::HIVE_MD_ACCESS_,
		  0, 0, 0, // query,querylen,querycharset
		  NULL, 0, // tablename,tablenamelen
		  NULL, tupleLen, 
		  NULL, tupleLen,
		  scanPred,
		  workCriDesc, workAtpIndex,
		  criDescParentDown, criDescParentUp,
		  queueSizeDown, queueSizeUp,
		  numBuffers, bufferSize),
    mdType_(type),
    hivePredStr_(hivePredStr),
    catalog_(catalogName),
    schema_(schemaName),
    object_(objectName)
{
  setNodeType(ComTdb::ex_HIVE_MD_ACCESS);
}

// Return the number of expressions held by the explain TDB (2)
// They are enumerated as: 0 - scanPred, 1 - paramsExpr
Int32
ComTdbExeUtilHiveMDaccess::numExpressions() const
{
  return(1);
}
 
// Return the expression names of the explain TDB based on some 
// enumeration. 0 - scanPred, 1 - paramsExpr
const char *
ComTdbExeUtilHiveMDaccess::getExpressionName(Int32 expNum) const
{
  switch(expNum)
    {
    case 0:
      return "Scan Expr";
    default:
      return 0;
    }  
}

// Return the expressions of the explain TDB based on some 
// enumeration. 0 - scanPred, 1 - paramsExpr
ex_expr *
ComTdbExeUtilHiveMDaccess::getExpressionNode(Int32 expNum)
{
  switch(expNum)
    {
    case 0:
      return scanExpr_;
    default:
      return 0;
    }  
}

// Pack the explainTdb: Convert all pointers to offsets relative
// to the space object.
Long ComTdbExeUtilHiveMDaccess::pack(void * space)
{
  if (hivePredStr_) 
    hivePredStr_.pack(space);
  if (catalog_)
    catalog_.pack(space);
  if (schema_)
    schema_.pack(space);
  if (object_)
    object_.pack(space);

  return ComTdbExeUtil::pack(space);
}

// Unpack the explainTdb.: Convert all offsets relative to base
// to pointers
Lng32 ComTdbExeUtilHiveMDaccess::unpack(void * base, void * reallocator)
{
  if (hivePredStr_.unpack(base))
    return -1;
  if (catalog_.unpack(base))
    return -1;
  if (schema_.unpack(base))
    return -1;
  if (object_.unpack(base))
    return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}

void ComTdbExeUtilHiveMDaccess::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[2000];
      str_sprintf(buf, "\nFor ComTdbExeUtilHiveMDaccess :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if (hivePredStr())
	{
	  str_sprintf(buf,"hivePredStr_ = %s", hivePredStr());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      if (getCatalog())
	{
	  str_sprintf(buf,"catalog_ = %s", getCatalog());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      if (getSchema())
	{
	  str_sprintf(buf,"schema_ = %s", getSchema());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
      if (getObject())
	{
	  str_sprintf(buf,"object_ = %s", getObject());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

//*********************************************
//ComTdbExeUtilHBaseBulkLoad
//********************************************
ComTdbExeUtilHBaseBulkLoad::ComTdbExeUtilHBaseBulkLoad(char * tableName,
                           ULng32 tableNameLen,
                           char * ldStmtStr,
                           ex_cri_desc * work_cri_desc,
                           const unsigned short work_atp_index,
                           ex_cri_desc * given_cri_desc,
                           ex_cri_desc * returned_cri_desc,
                           queue_index down,
                           queue_index up,
                           Lng32 num_buffers,
                           ULng32 buffer_size,
                           char * errCountTab,
                           char * loggingLoc
                           )
    : ComTdbExeUtil(ComTdbExeUtil::HBASE_LOAD_,
                    NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
                    tableName, tableNameLen,
                    NULL, 0,
                    NULL, 0,
                    NULL,
                    work_cri_desc, work_atp_index,
                    given_cri_desc, returned_cri_desc,
                    down, up,
                    num_buffers, buffer_size),
      ldQuery_(ldStmtStr),
      flags_(0),
      maxErrorRows_(0),
      errCountTable_(errCountTab),
      loggingLocation_(loggingLoc)


    {
      setNodeType(ComTdb::ex_HBASE_LOAD);
    }

Long ComTdbExeUtilHBaseBulkLoad::pack(void * space)
{
  if (ldQuery_)
    ldQuery_.pack(space);
  if(errCountTable_)
    errCountTable_.pack(space);
  if(loggingLocation_)
    loggingLocation_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilHBaseBulkLoad::unpack(void * base, void * reallocator)
{
  if(ldQuery_.unpack(base))
    return -1;
  if(errCountTable_.unpack(base))
    return -1;
  if(loggingLocation_.unpack(base))
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}
void ComTdbExeUtilHBaseBulkLoad::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);

   if (flag  & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilHbaseLoad :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if (getTableName() != NULL)
        {
          str_sprintf(buf,"Tablename = %s ",getTableName());
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

      if (ldQuery_)
        {
          char query[400];
          if (strlen(ldQuery_) > 390)
            {
              strncpy(query, ldQuery_, 390);
              query[390] = 0;
              strcat(query, "...");
            }
          else
            strcpy(query, ldQuery_);

          str_sprintf(buf,"ld Query = %s ",query);
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

     if (getLogErrorRows()) {
        if (loggingLocation_) {
           str_sprintf(buf, "Logging location = %s ", loggingLocation_.getPointer()); 
           space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }
     }
     if (maxErrorRows_ > 0) {
        str_sprintf(buf, "Max Error Rows = %d", maxErrorRows_);
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        if (errCountTable_) {
           str_sprintf(buf, "Error Counter Table Name = %s ", errCountTable_.getPointer()); 
           space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }
     }
  }

  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}


//*********************************************
//ComTdbExeUtilHBaseBulkUnLoad
//********************************************
ComTdbExeUtilHBaseBulkUnLoad::ComTdbExeUtilHBaseBulkUnLoad(char * tableName,
                           ULng32 tableNameLen,
                           char * uldStmtStr,
                           char * extractLocation,
                           ex_cri_desc * work_cri_desc,
                           const unsigned short work_atp_index,
                           ex_cri_desc * given_cri_desc,
                           ex_cri_desc * returned_cri_desc,
                           queue_index down,
                           queue_index up,
                           Lng32 num_buffers,
                           ULng32 buffer_size
                           )
    : ComTdbExeUtil(ComTdbExeUtil::HBASE_UNLOAD_,
                    NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
                    tableName, tableNameLen,
                    NULL, 0,
                    NULL, 0,
                    NULL,
                    work_cri_desc, work_atp_index,
                    given_cri_desc, returned_cri_desc,
                    down, up,
                    num_buffers, buffer_size),
      uldQuery_(uldStmtStr),
      flags_(0),
      compressType_(0),
      extractLocation_(extractLocation),
      scanType_(0),
      snapshotSuffix_(NULL)
    {
    setNodeType(ComTdb::ex_HBASE_UNLOAD);
    }

Long ComTdbExeUtilHBaseBulkUnLoad::pack(void * space)
{
  if (uldQuery_)
    uldQuery_.pack(space);

  if (mergePath_)
    mergePath_.pack(space);
  if (extractLocation_)
    extractLocation_.pack(space);
  if (snapshotSuffix_)
    snapshotSuffix_.pack(space);

  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilHBaseBulkUnLoad::unpack(void * base, void * reallocator)
{
  if(uldQuery_.unpack(base))
    return -1;
  if(mergePath_.unpack(base))
    return -1;
  if(extractLocation_.unpack(base))
      return -1;
  if(snapshotSuffix_.unpack(base))
      return -1;

  return ComTdbExeUtil::unpack(base, reallocator);
}
void ComTdbExeUtilHBaseBulkUnLoad::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);

  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbExeUtilHbaseUnLoad :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if (getTableName() != NULL)
        {
          str_sprintf(buf,"Tablename = %s ",getTableName());
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

      if (uldQuery_)
        {
          char query[400];
          if (strlen(uldQuery_) > 390)
            {
              strncpy(query, uldQuery_, 390);
              query[390] = 0;
              strcat(query, "...");
            }
          else
            strcpy(query, uldQuery_);

          str_sprintf(buf,"uld Query = %s ",query);
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

/////NEED TO ADD rthe remaning INFO
    }

  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

ComTdbExeUtilRegionStats::ComTdbExeUtilRegionStats
(
     char * tableName,
     ex_expr_base * input_expr,
     ULng32 input_rowlen,
     ex_expr_base * scan_expr,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::REGION_STATS_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     tableName, strlen(tableName),
		     input_expr, input_rowlen,
		     NULL, 0,
		     scan_expr,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0)
{
  setNodeType(ComTdb::ex_REGION_STATS);
}

ComTdbExeUtilLobInfo::ComTdbExeUtilLobInfo
(
     char * tableName,
      Int64 objectUID,
     Lng32 numLOBs,
     char *lobColArray,
     char * lobNumArray,
     char * lobLocArray,
     char *lobTypeArray,
     Int32 hdfsPort,
     char *hdfsServer,
     NABoolean tableFormat,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size)
     : ComTdbExeUtil(ComTdbExeUtil::LOB_INFO_,
		     NULL, 0, (Int16)SQLCHARSETCODE_UNKNOWN,
		     tableName, strlen(tableName),
		     NULL, 0,
		     NULL, 0,
		     NULL,
		     work_cri_desc, work_atp_index,
		     given_cri_desc, returned_cri_desc,
		     down, up, 
		     num_buffers, buffer_size),
       flags_(0),
       objectUID_(objectUID),
       numLOBs_(numLOBs),
       lobColArray_(lobColArray),
       lobNumArray_(lobNumArray),
       lobLocArray_(lobLocArray),
       lobTypeArray_(lobTypeArray),
       hdfsPort_(0),
       hdfsServer_(hdfsServer),
       tableFormat_(tableFormat)
{
  setNodeType(ComTdb::ex_LOB_INFO);
}

Long ComTdbExeUtilLobInfo::pack(void * space)
{
  if (lobColArray_) 
    lobColArray_.pack(space);

 if (lobNumArray_) 
    lobNumArray_.pack(space);

 if (lobLocArray_) 
    lobLocArray_.pack(space);

 if(lobTypeArray_)
   lobTypeArray_.pack(space);

 if (hdfsServer_) 
    hdfsServer_.pack(space);
  return ComTdbExeUtil::pack(space);
}

Lng32 ComTdbExeUtilLobInfo::unpack(void * base, void * reallocator)
{
  if (lobColArray_.unpack(base))
    return -1;

  if(lobNumArray_.unpack(base)) 
    return -1;

  if(lobLocArray_.unpack(base)) 
    return -1;

  if(lobTypeArray_.unpack(base))
    return -1;

  if(hdfsServer_.unpack(base)) 
    return -1;
  return ComTdbExeUtil::unpack(base, reallocator);
}
