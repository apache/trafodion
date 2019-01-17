/*********************************************************************
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
* File:         ComTdbExeUtil.h
* Description:
*
* Created:      12/11/2005
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMTDBEXEUTIL_H
#define COMTDBEXEUTIL_H

#include "ComTdb.h"
#include "ComTdbDDL.h"
#include "ComQueue.h"
#include "ComCharSetDefs.h"

////////////////////////////////////////////////////////////////////
// class ComTdbExeUtil
////////////////////////////////////////////////////////////////////
static const ComTdbVirtTableColumnInfo exeUtilVirtTableColumnInfo[] =
{
  { "UTIL_OUTPUT",       0, COM_USER_COLUMN, REC_BYTE_V_ASCII,     2000, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0}
};

class ComTdbExeUtil : public ComTdbGenericUtil
{
  friend class ExExeUtilTcb;
  friend class ExExeUtilPrivateState;

public:
  enum ExeUtilType
  {
    DISPLAY_EXPLAIN_         = 2,
    MAINTAIN_OBJECT_         = 3,
    LOAD_VOLATILE_TABLE_     = 4,
    CLEANUP_VOLATILE_SCHEMA_ = 5,
    GET_VOLATILE_INFO        = 6,
    CREATE_TABLE_AS_         = 7,
    GET_MAINTAIN_INFO_       = 9,
    GET_STATISTICS_          = 10,
    USER_LOAD_               = 11,
    REGION_STATS_        = 12,
    LONG_RUNNING_            = 13,
    GET_METADATA_INFO_       = 14,
    GET_VERSION_INFO_        = 15,
    SUSPEND_ACTIVATE_        = 16,
    LOB_INFO_                = 17,
    SHOW_SET_                = 19,
    AQR_                     = 20,
    DISPLAY_EXPLAIN_COMPLEX_ = 21,
    GET_UID_                 = 22,
    POP_IN_MEM_STATS_        = 23,
    GET_ERROR_INFO_          = 26,
    LOB_EXTRACT_             = 27,
    LOB_SHOWDDL_             = 28,
    GET_HIVE_METADATA_INFO_  = 29,
    HIVE_MD_ACCESS_          = 30,
    AQR_WNR_INSERT_          = 31,
    HBASE_LOAD_              = 32,
    HBASE_UNLOAD_            = 33,
    HBASE_UNLOAD_TASK_       = 34,
    GET_QID_                 = 35,
    HIVE_TRUNCATE_           = 36,
    LOB_UPDATE_UTIL_         = 37,
    HIVE_QUERY_              = 38
  };

  ComTdbExeUtil()
  : ComTdbGenericUtil()
  {}

  ComTdbExeUtil(Lng32 type,
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
		ULng32 buffer_size
		);

  char * getTableName()  { return objectName_; }
  char * getObjectName() { return objectName_; }

  void setChildTdb(ComTdb * child)  { child_ = child; };

  static Int32 getVirtTableNumCols()
  {
    return sizeof(exeUtilVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)exeUtilVirtTableColumnInfo;
  }

  static Int32 getVirtTableNumKeys()
  {
    return 0;
  }

  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return NULL;
  }

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize()        { return (short)sizeof(ComTdbExeUtil); }

  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return (child_ ? 1 : 0); }
  virtual const char *getNodeName() const
  {
    if (type_ == CREATE_TABLE_AS_)
      return "CREATE_TABLE_AS";
    else
      return "EX_EXE_UTIL";
  };

  virtual Int32 numExpressions() const
    {
      return (ComTdbGenericUtil::numExpressions() + 1);
    }

  virtual ex_expr* getExpressionNode(Int32 pos) {
    if (pos >= numExpressions())
      return NULL;
    else
      if (pos < ComTdbGenericUtil::numExpressions())
	return ComTdbGenericUtil::getExpressionNode(pos);
      else
	return scanExpr_;
  }

  virtual const char * getExpressionName(Int32 pos) const {
    if (pos >= numExpressions())
      return NULL;
    else
      if (pos < ComTdbGenericUtil::numExpressions())
	return ComTdbGenericUtil::getExpressionName(pos);
      else
	return "scanExpr_";
  }

  ExeUtilType getType() { return (ExeUtilType)type_;}

  void setExplOptionsStr(char * expl)
  {
    explOptionsStr_ = expl;
  }
  char * explOptionsStr() { return explOptionsStr_; }

  const char * getNEOCatalogName()  { return NEOCatalogName_; }
  void setNEOCatalogName(char * catalog) { NEOCatalogName_ = catalog; }

  void setType(ExeUtilType type) { type_ = type; }

protected:
  Lng32 type_;                               // 00-03
  UInt32 flags_;                            // 04-07
  ComTdbPtr    child_;                      // 08-15

  // expression to evaluate the predicate on the returned row
  ExExprBasePtr scanExpr_;                  // 16-23

  NABasicPtr explOptionsStr_;               // 24-31

  // Set to the NEO catalog name
  NABasicPtr NEOCatalogName_;               // 32-39

  char fillersComTdbExeUtil_[104];          // 40-135

};


static const ComTdbVirtTableColumnInfo exeUtilDisplayExplainVirtTableColumnInfo[] =
{
  { "EXPLAIN OUTPUT",     0,             COM_USER_COLUMN, REC_BYTE_V_ASCII,     4000, FALSE, SQLCHARSETCODE_UTF8, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0 }
};

static const ComTdbVirtTableColumnInfo exeUtilDisplayExplainVirtTableOptionXColumnInfo[] =
{
  { "EXPLAIN OUTPUT(FORMATTED)",   0,    COM_USER_COLUMN, REC_BYTE_V_ASCII,     79, FALSE, SQLCHARSETCODE_UTF8, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  }
};

class ComTdbExeUtilDisplayExplain : public ComTdbExeUtil
{
  friend class ExExeUtilDisplayExplainTcb;
  friend class ExExeUtilDisplayExplainPrivateState;

public:
  ComTdbExeUtilDisplayExplain()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilDisplayExplain(char * query,
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
			      ULng32 buffer_size
			      );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilDisplayExplain);}

  virtual const char *getNodeName() const
  {
    return "DISPLAY_EXPLAIN";
  };

  static Int32 getVirtTableNumCols()
  {
    return sizeof(exeUtilDisplayExplainVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)exeUtilDisplayExplainVirtTableColumnInfo;
  }

  static ComTdbVirtTableColumnInfo * getVirtTableOptionXColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)exeUtilDisplayExplainVirtTableOptionXColumnInfo;
  }

  char * getModuleName() { return moduleName_; }
  char * getStmtName() { return stmtName_; }

  Lng32 getColDescSize() { return colDescSize_;}
  Lng32 getOutputRowSize() { return outputRowSize_; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  NABoolean isOptionE() { return ((flags_ & OPTION_E) != 0); };
  NABoolean isOptionF() { return ((flags_ & OPTION_F) != 0); };
  NABoolean isOptionM() { return ((flags_ & OPTION_M) != 0); };
  NABoolean isOptionN() { return ((flags_ & OPTION_N) != 0); };
  NABoolean isOptionC() { return ((flags_ & OPTION_C) != 0); };
  NABoolean isOptionP() { return ((flags_ & OPTION_P) != 0); };

  void setOptionE(NABoolean v) 
  {(v ? flags_ |= OPTION_E : flags_ &= ~OPTION_E); };
  void setOptionF(NABoolean v) 
  {(v ? flags_ |= OPTION_F : flags_ &= ~OPTION_F); };
  void setOptionM(NABoolean v) 
  {(v ? flags_ |= OPTION_M : flags_ &= ~OPTION_M); };
  void setOptionN(NABoolean v) 
  {(v ? flags_ |= OPTION_N : flags_ &= ~OPTION_N); };
  void setOptionC(NABoolean v) 
  {(v ? flags_ |= OPTION_C : flags_ &= ~OPTION_C); };
  void setOptionP(NABoolean v) 
  {(v ? flags_ |= OPTION_P : flags_ &= ~OPTION_P); };

private:
  enum OpToFlag
  {
    OPTION_F      = 0x0001,
    OPTION_E      = 0x0002,
    OPTION_M      = 0x0004,
    OPTION_N      = 0x0008,
    OPTION_C      = 0x0010,
    OPTION_P      = 0x0020
  };

  UInt32 flags_;                                      // 00-03
  UInt32 filler_;                                     // 04-07
  NABasicPtr moduleName_;                             // 08-15
  NABasicPtr stmtName_;                               // 16-23

  Lng32 colDescSize_;
  Lng32 outputRowSize_;

  char fillersComTdbExeUtilDisplayExplain_[96];      // 32-127
};

class ComTdbExeUtilDisplayExplainComplex : public ComTdbExeUtil
{
  friend class ExExeUtilDisplayExplainComplexTcb;
  friend class ExExeUtilDisplayExplainShowddlTcb;
  friend class ExExeUtilDisplayExplainComplexPrivateState;

public:
  enum ExplainType
  {
    CREATE_TABLE_,
    CREATE_INDEX_,
    CREATE_MV_,
    CREATE_TABLE_AS
  };

  ComTdbExeUtilDisplayExplainComplex()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilDisplayExplainComplex(
       Lng32 explainType,
       char * qry1,
       char * qry2,
       char * qry3,
       char * qry4,
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
       ULng32 buffer_size
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilDisplayExplainComplex);}

  virtual const char *getNodeName() const
  {
    return "DISPLAY_EXPLAIN_COMPLEX";
  };

  void setIsVolatile(NABoolean v)
  {(v ? flags_ |= IS_VOLATILE : flags_ &= ~IS_VOLATILE); };
  NABoolean isVolatile() { return (flags_ & IS_VOLATILE) != 0; };

  void setIsShowddl(NABoolean v)
  {(v ? flags_ |= IS_SHOWDDL : flags_ &= ~IS_SHOWDDL); };
  NABoolean isShowddl() { return (flags_ & IS_SHOWDDL) != 0; };

  void setNoLabelStats(NABoolean v)
  {(v ? flags_ |= NO_LABEL_STATS : flags_ &= ~NO_LABEL_STATS); };
  NABoolean noLabelStats() { return (flags_ & NO_LABEL_STATS) != 0; };

  void setLoadIfExists(NABoolean v)
  {(v ? flags_ |= LOAD_IF_EXISTS : flags_ &= ~LOAD_IF_EXISTS); };
  NABoolean loadIfExists() { return (flags_ & LOAD_IF_EXISTS) != 0; };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  enum
  {
    IS_VOLATILE    = 0x0001,
    IS_SHOWDDL     = 0x0002,
    NO_LABEL_STATS = 0x0004,
    LOAD_IF_EXISTS = 0x0008
  };

  NABasicPtr qry1_;                                      // 00-07
  NABasicPtr qry2_;                                      // 08-15
  NABasicPtr qry3_;                                      // 16-23
  NABasicPtr qry4_;                                      // 24-31

  UInt32 flags_;                                         // 32-35

  Lng32 explainType_;                                     // 36-39

  char fillersComTdbExeUtilDisplayExplainComplex_[96];   // 40-135
};

class ComTdbExeUtilMaintainObject : public ComTdbExeUtil
{
  friend class ExExeUtilMaintainObjectTcb;
  friend class ExExeUtilMaintainObjectPrivateState;

public:
  enum ObjectType
  {
    TABLE_ = 0,
    INDEX_,
    MV_,
    MVGROUP_,
    MV_INDEX_,
    MV_LOG_,
    CATALOG_,
    SCHEMA_,
    CLEAN_MAINTAIN_,
    NOOP_
  };

  // if multiple tables are to be maintained together, this define
  // indicates the max number of tables which could be specified.
  // Should match the number set in 
  // ExeUtilMaintainObject::bindNode. Maybe we can define this at
  // a common place. TBD.
#define MAX_MULTI_TABLES 100

  ComTdbExeUtilMaintainObject()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilMaintainObject(char * objectName,
			      ULng32 objectNameLen,
			      char *schemaName,
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
			      ULng32 buffer_size
			     );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void setParams(NABoolean reorgTable,
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
		 );

  void setOptionsParams(char* reorgTableOptions,
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
			char* cleanMaintainCITOptions);

  void setLists(Queue* indexList,
		Queue* refreshMvgroupList,
		Queue* refreshMvsList,
		Queue* reorgMvgroupList,
		Queue* reorgMvsList,
		Queue* reorgMvsIndexList,
		Queue* updStatsMvgroupList,
		Queue* updStatsMvsList,
		Queue* multiTablesNamesList,
		Queue* skippedMultiTablesNamesList);

  void setControlParams
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
   NABoolean enableUpdStatsMvslog,
   NABoolean disableUpdStatsMvslog,
   NABoolean resetUpdStatsMvslog,
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
   );

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilMaintainObject);}

  virtual const char *getNodeName() const
  {
    return "MAINTAIN_OBJECT";
  };

  void setInitializeMaintain(NABoolean v)
  {(v ? flags_ |= INITIALIZE_MAINTAIN : flags_ &= ~INITIALIZE_MAINTAIN); };
  NABoolean initializeMaintain() { return (flags_ & INITIALIZE_MAINTAIN) != 0; };

  void setReInitializeMaintain(NABoolean v)
  {(v ? flags_ |= REINITIALIZE_MAINTAIN : flags_ &= ~REINITIALIZE_MAINTAIN); };
  NABoolean reInitializeMaintain() { return (flags_ & REINITIALIZE_MAINTAIN) != 0; };

  void setDropMaintain(NABoolean v)
  {(v ? flags_ |= DROP_MAINTAIN : flags_ &= ~DROP_MAINTAIN); };
  NABoolean dropMaintain() { return (flags_ & DROP_MAINTAIN) != 0; };

  void setCreateView(NABoolean v)
  {(v ? flags_ |= CREATE_VIEW : flags_ &= ~CREATE_VIEW); };
  NABoolean createView() { return (flags_ & CREATE_VIEW) != 0; };

  void setDropView(NABoolean v)
  {(v ? flags_ |= DROP_VIEW : flags_ &= ~DROP_VIEW); };
  NABoolean dropView() { return (flags_ & DROP_VIEW) != 0; };

  void setReorgTable(NABoolean v)
  {(v ? flags_ |= REORG_TABLE : flags_ &= ~REORG_TABLE); };
  NABoolean reorgTable() { return (flags_ & REORG_TABLE) != 0; };

  void setReorgIndex(NABoolean v)
  {(v ? flags_ |= REORG_INDEX : flags_ &= ~REORG_INDEX); };
  NABoolean reorgIndex() { return (flags_ & REORG_INDEX) != 0; };

  void setUpdStatsTable(NABoolean v)
  {(v ? flags_ |= UPD_STATS_TABLE : flags_ &= ~UPD_STATS_TABLE); };
  NABoolean updStatsTable() { return (flags_ & UPD_STATS_TABLE) != 0; };

  void setUpdStatsMvlog(NABoolean v)
  {(v ? flags_ |= UPD_STATS_MVLOG : flags_ &= ~UPD_STATS_MVLOG); };
  NABoolean updStatsMvlog() { return (flags_ & UPD_STATS_MVLOG) != 0; };

  void setRefreshMvs(NABoolean v)
  {(v ? flags_ |= REFRESH_MVS : flags_ &= ~REFRESH_MVS); };
  NABoolean refreshMvs() { return (flags_ & REFRESH_MVS) != 0; };

  void setRefreshMvgroup(NABoolean v)
  {(v ? flags_ |= REFRESH_MVGROUP : flags_ &= ~REFRESH_MVGROUP); };
  NABoolean refreshMvgroup() { return (flags_ & REFRESH_MVGROUP) != 0; };

  void setReorgMvgroup(NABoolean v)
  {(v ? flags_ |= REORG_MVGROUP : flags_ &= ~REORG_MVGROUP); };
  NABoolean reorgMvgroup() { return (flags_ & REORG_MVGROUP) != 0; };

  void setReorgMvs(NABoolean v)
  {(v ? flags_ |= REORG_MVS : flags_ &= ~REORG_MVS); };
  NABoolean reorgMvs() { return (flags_ & REORG_MVS) != 0; };

  void setReorgMvsIndex(NABoolean v)
  {(v ? flags_ |= REORG_MVS_INDEX : flags_ &= ~REORG_MVS_INDEX); };
  NABoolean reorgMvsIndex() { return (flags_ & REORG_MVS_INDEX) != 0; };

  void setUpdStatsMvs(NABoolean v)
  {(v ? flags_ |= UPD_STATS_MVS : flags_ &= ~UPD_STATS_MVS); };
  NABoolean updStatsMvs() { return (flags_ & UPD_STATS_MVS) != 0; };

  void setUpdStatsMvgroup(NABoolean v)
  {(v ? flags_ |= UPD_STATS_MVGROUP : flags_ &= ~UPD_STATS_MVGROUP); };
  NABoolean updStatsMvgroup() { return (flags_ & UPD_STATS_MVGROUP) != 0; };

  void setContinueOnError(NABoolean v)
  {(v ? flags_ |= CONTINUE_ON_ERROR : flags_ &= ~CONTINUE_ON_ERROR); };
  NABoolean continueOnError() { return (flags_ & CONTINUE_ON_ERROR) != 0; };

  void setDisplay(NABoolean v)
  {(v ? flags_ |= DISPLAY : flags_ &= ~DISPLAY); };
  NABoolean display() { return (flags_ & DISPLAY) != 0; };

  void setDisplayDetail(NABoolean v)
  {(v ? flags_ |= DISPLAY_DETAIL : flags_ &= ~DISPLAY_DETAIL); };
  NABoolean displayDetail() { return (flags_ & DISPLAY_DETAIL) != 0; };

  void setDoSpecifiedTask(NABoolean v)
  {(v ? flags_ |= DO_SPECIFIED_TASK : flags_ &= ~DO_SPECIFIED_TASK); };
  NABoolean doSpecifiedTask() { return (flags_ & DO_SPECIFIED_TASK) != 0; };

  void setSkipRefreshMvs(NABoolean v)
  {(v ? flags_ |= SKIP_REFRESH_MVS : flags_ &= ~SKIP_REFRESH_MVS); };
  NABoolean skipRefreshMvs() { return (flags_ & SKIP_REFRESH_MVS) != 0; };

  void setSkipReorgMvs(NABoolean v)
  {(v ? flags_ |= SKIP_REORG_MVS : flags_ &= ~SKIP_REORG_MVS); };
  NABoolean skipReorgMvs() { return (flags_ & SKIP_REORG_MVS) != 0; };


  void setSkipUpdStatsMvs(NABoolean v)
  {(v ? flags_ |= SKIP_UPD_STATS_MVS : flags_ &= ~SKIP_UPD_STATS_MVS); };
  NABoolean skipUpdStatsMvs() { return (flags_ & SKIP_UPD_STATS_MVS) != 0; };

  void setGetStatus(NABoolean v)
  {(v ? flags_ |= GET_STATUS : flags_ &= ~GET_STATUS); };
  NABoolean getStatus() { return (flags_ & GET_STATUS) != 0; };

  void setGetDetails(NABoolean v)
  {(v ? flags_ |= GET_DETAILS : flags_ &= ~GET_DETAILS); };
  NABoolean getDetails() { return (flags_ & GET_DETAILS) != 0; };

  void setRun(NABoolean v)
  {(v ? flags_ |= RUN : flags_ &= ~RUN); };
  NABoolean run() { return (flags_ & RUN) != 0; };

  void setIfNeeded(NABoolean v)
  {(v ? flags_ |= IF_NEEDED : flags_ &= ~IF_NEEDED); };
  NABoolean ifNeeded() { return (flags_ & IF_NEEDED) != 0; };

  void setAllSpecified(NABoolean v)
  {(v ? flags_ |= ALL_SPECIFIED : flags_ &= ~ALL_SPECIFIED); };
  NABoolean allSpecified() { return (flags_ & ALL_SPECIFIED) != 0; };

  

 void setSchemaLabelStats(NABoolean v)
  {(v ? flags2_ |= GET_SCHEMA_LABEL_STATS : flags2_ &= ~GET_SCHEMA_LABEL_STATS); };
 NABoolean getSchemaLabelStats() { return (flags2_ & GET_SCHEMA_LABEL_STATS) != 0; }

 void setTableLabelStats(NABoolean v)
  {(v ? flags2_ |= GET_TABLE_LABEL_STATS : flags2_ &= ~GET_TABLE_LABEL_STATS); };
 NABoolean getTableLabelStats() { return (flags2_ & GET_TABLE_LABEL_STATS) != 0; }
void setIndexLabelStats(NABoolean v)
  {(v ? flags2_ |= GET_INDEX_LABEL_STATS : flags2_ &= ~GET_INDEX_LABEL_STATS); };
 NABoolean getIndexLabelStats() { return (flags2_ & GET_INDEX_LABEL_STATS) != 0; }
void setLabelStatsIncIndexes(NABoolean v)
  {(v ? flags2_ |= GET_LABEL_STATS_INC_INDEXES : flags2_ &= ~GET_LABEL_STATS_INC_INDEXES); };
 NABoolean getLabelStatsIncIndexes() { return (flags2_ & GET_LABEL_STATS_INC_INDEXES) != 0; }

void setLabelStatsIncInternal(NABoolean v)
  {(v ? flags2_ |= GET_LABEL_STATS_INC_INTERNAL : flags2_ &= ~GET_LABEL_STATS_INC_INTERNAL); };
 NABoolean getLabelStatsIncInternal() { return (flags2_ & GET_LABEL_STATS_INC_INTERNAL) != 0; }

void setLabelStatsIncRelated(NABoolean v)
  {(v ? flags2_ |= GET_LABEL_STATS_INC_RELATED : flags2_ &= ~GET_LABEL_STATS_INC_RELATED); };
 NABoolean getLabelStatsIncRelated() { return (flags2_ & GET_LABEL_STATS_INC_RELATED) != 0; }



  void setRunFrom(Int64 v) { from_ = v; }
  void setRunTo(Int64 v) { to_ = v; }
  Int64 runFrom() { return from_; }
  Int64 runTo() { return to_; }

  void setDisableReorgTable(NABoolean v)
  {(v ? controlFlags_ |= DISABLE_REORG_TABLE : controlFlags_ &= ~DISABLE_REORG_TABLE); };
  NABoolean disableReorgTable() { return (controlFlags_ & DISABLE_REORG_TABLE) != 0; };

  void setDisableReorgIndex(NABoolean v)
  {(v ? controlFlags_ |= DISABLE_REORG_INDEX : controlFlags_ &= ~DISABLE_REORG_INDEX); };
  NABoolean disableReorgIndex() { return (controlFlags_ & DISABLE_REORG_INDEX) != 0; };

  void setDisableUpdStatsTable(NABoolean v)
  {(v ? controlFlags_ |= DISABLE_UPD_STATS_TABLE : controlFlags_ &= ~DISABLE_UPD_STATS_TABLE); };
  NABoolean disableUpdStatsTable() { return (controlFlags_ & DISABLE_UPD_STATS_TABLE) != 0; };

  void setDisableUpdStatsMvs(NABoolean v)
  {(v ? controlFlags_ |= DISABLE_UPD_STATS_MVS : controlFlags_ &= ~DISABLE_UPD_STATS_MVS); };
  NABoolean disableUpdStatsMvs() { return (controlFlags_ & DISABLE_UPD_STATS_MVS) != 0; };

 void setDisableRefreshMvs(NABoolean v)
  {(v ? controlFlags2_ |= DISABLE_REFRESH_MVS : controlFlags2_ &= ~DISABLE_REFRESH_MVS); };
  NABoolean disableRefreshMvs() { return (controlFlags2_ & DISABLE_REFRESH_MVS) != 0; };

  void setDisableReorgMvs(NABoolean v)
  {(v ? controlFlags_ |= DISABLE_REORG_MVS : controlFlags_ &= ~DISABLE_REORG_MVS); };
  NABoolean disableReorgMvs() { return (controlFlags_ & DISABLE_REORG_MVS) != 0; };

  void setEnableReorgTable(NABoolean v)
  {(v ? controlFlags_ |= ENABLE_REORG_TABLE : controlFlags_ &= ~ENABLE_REORG_TABLE); };
  NABoolean enableReorgTable() { return (controlFlags_ & ENABLE_REORG_TABLE) != 0; };

  void setEnableReorgIndex(NABoolean v)
  {(v ? controlFlags_ |= ENABLE_REORG_INDEX : controlFlags_ &= ~ENABLE_REORG_INDEX); };
  NABoolean enableReorgIndex() { return (controlFlags_ & ENABLE_REORG_INDEX) != 0; };

  void setEnableUpdStatsTable(NABoolean v)
  {(v ? controlFlags_ |= ENABLE_UPD_STATS_TABLE : controlFlags_ &= ~ENABLE_UPD_STATS_TABLE); };
  NABoolean enableUpdStatsTable() { return (controlFlags_ & ENABLE_UPD_STATS_TABLE) != 0; };

  void setEnableUpdStatsMvs(NABoolean v)
  {(v ? controlFlags_ |= ENABLE_UPD_STATS_MVS : controlFlags_ &= ~ENABLE_UPD_STATS_MVS); };
  NABoolean enableUpdStatsMvs() { return (controlFlags_ & ENABLE_UPD_STATS_MVS) != 0; };

  void setEnableRefreshMvs(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_REFRESH_MVS : controlFlags2_ &= ~ENABLE_REFRESH_MVS); };
  NABoolean enableRefreshMvs() { return (controlFlags2_ & ENABLE_REFRESH_MVS) != 0; };

  void setEnableReorgMvsIndex(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_REORG_MVS_INDEX : controlFlags2_ &= ~ENABLE_REORG_MVS_INDEX); };
  NABoolean enableReorgMvsIndex() { return (controlFlags2_ & ENABLE_REORG_MVS_INDEX) != 0; };

  void setDisableReorgMvsIndex(NABoolean v)
  {(v ? controlFlags2_ |= DISABLE_REORG_MVS_INDEX : controlFlags2_ &= ~DISABLE_REORG_MVS_INDEX); };
  NABoolean disableReorgMvsIndex() { return (controlFlags2_ & DISABLE_REORG_MVS_INDEX) != 0; };

  void setResetReorgMvsIndex(NABoolean v)
  {(v ? controlFlags2_ |= RESET_REORG_MVS_INDEX : controlFlags2_ &= ~RESET_REORG_MVS_INDEX); };
  NABoolean resetReorgMvsIndex() { return (controlFlags2_ & RESET_REORG_MVS_INDEX) != 0; };

 void setEnableReorgMvs(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_REORG_MVS : controlFlags2_ &= ~ENABLE_REORG_MVS); };
  NABoolean enableReorgMvs() { return (controlFlags2_ & ENABLE_REORG_MVS) != 0; };

  void setResetReorgTable(NABoolean v)
  {(v ? controlFlags_ |= RESET_REORG_TABLE : controlFlags_ &= ~RESET_REORG_TABLE); };
  NABoolean resetReorgTable() { return (controlFlags_ & RESET_REORG_TABLE) != 0; };

  void setResetUpdStatsTable(NABoolean v)
  {(v ? controlFlags_ |= RESET_UPD_STATS_TABLE : controlFlags_ &= ~RESET_UPD_STATS_TABLE); };
  NABoolean resetUpdStatsTable() { return (controlFlags_ & RESET_UPD_STATS_TABLE) != 0; };

  void setResetUpdStatsMvs(NABoolean v)
  {(v ? controlFlags_ |= RESET_UPD_STATS_MVS : controlFlags_ &= ~RESET_UPD_STATS_MVS); };
  NABoolean resetUpdStatsMvs() { return (controlFlags_ & RESET_UPD_STATS_MVS) != 0; };

  void setResetRefreshMvs(NABoolean v)
  {(v ? controlFlags2_ |= RESET_REFRESH_MVS : controlFlags2_ &= ~RESET_REFRESH_MVS); };
  NABoolean resetRefreshMvs() { return (controlFlags2_ & RESET_REFRESH_MVS) != 0; };

  void setResetReorgMvs(NABoolean v)
  {(v ? controlFlags2_ |= RESET_REORG_MVS : controlFlags2_ &= ~RESET_REORG_MVS); };
  NABoolean resetReorgMvs() { return (controlFlags2_ & RESET_REORG_MVS) != 0; };

  void setEnableUpdStatsMvlog(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_UPD_STATS_MVLOG : controlFlags2_ &= ~ENABLE_UPD_STATS_MVLOG); };
  NABoolean enableUpdStatsMvlog() { return (controlFlags2_ & ENABLE_UPD_STATS_MVLOG) != 0; };

  void setDisableUpdStatsMvlog(NABoolean v)
  {(v ? controlFlags2_ |= DISABLE_UPD_STATS_MVLOG : controlFlags2_ &= ~DISABLE_UPD_STATS_MVLOG); };
  NABoolean disableUpdStatsMvlog() { return (controlFlags2_ & DISABLE_UPD_STATS_MVLOG) != 0; };

  void setResetUpdStatsMvlog(NABoolean v)
  {(v ? controlFlags2_ |= RESET_UPD_STATS_MVLOG : controlFlags2_ &= ~RESET_UPD_STATS_MVLOG); };
  NABoolean resetUpdStatsMvlog() { return (controlFlags2_ & RESET_UPD_STATS_MVLOG) != 0; };

  void setResetReorgIndex(NABoolean v)
  {(v ? controlFlags2_ |= RESET_REORG_INDEX : controlFlags2_ &= ~RESET_REORG_INDEX); };
  NABoolean resetReorgIndex() { return (controlFlags2_ & RESET_REORG_INDEX) != 0; };

  void setEnableRefreshMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_REFRESH_MVGROUP : controlFlags2_ &= ~ENABLE_REFRESH_MVGROUP); };
  NABoolean enableRefreshMvgroup() { return (controlFlags2_ & ENABLE_REFRESH_MVGROUP) != 0; };

  void setEnableTableLabelStats(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_GET_TABLE_LABEL_STATS : controlFlags2_ &= ~ENABLE_GET_TABLE_LABEL_STATS); };
  NABoolean enableTableLabelStats() { return (controlFlags2_ & ENABLE_GET_TABLE_LABEL_STATS) != 0; };
  void setDisableTableLabelStats(NABoolean v)
  {(v ? controlFlags2_ |= DISABLE_GET_TABLE_LABEL_STATS : controlFlags2_ &= ~DISABLE_GET_TABLE_LABEL_STATS); };
  NABoolean disableTableLabelStats() { return (controlFlags2_ & DISABLE_GET_TABLE_LABEL_STATS) != 0; };
  void setResetTableLabelStats(NABoolean v)
  {(v ? controlFlags2_ |= RESET_GET_TABLE_LABEL_STATS : controlFlags2_ &= ~RESET_GET_TABLE_LABEL_STATS); };
  NABoolean resetTableLabelStats() { return (controlFlags2_ & RESET_GET_TABLE_LABEL_STATS) != 0; };

 void setEnableIndexLabelStats(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_GET_INDEX_LABEL_STATS : controlFlags2_ &= ~ENABLE_GET_INDEX_LABEL_STATS); };
  NABoolean enableIndexLabelStats() { return (controlFlags2_ & ENABLE_GET_INDEX_LABEL_STATS) != 0; };
 void setDisableIndexLabelStats(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_GET_INDEX_LABEL_STATS : controlFlags2_ &= ~ENABLE_GET_INDEX_LABEL_STATS); };
  NABoolean disableIndexLabelStats() { return (controlFlags2_ & DISABLE_GET_INDEX_LABEL_STATS) != 0; };
 void setResetIndexLabelStats(NABoolean v)
  {(v ? controlFlags2_ |= RESET_GET_INDEX_LABEL_STATS : controlFlags2_ &= ~RESET_GET_INDEX_LABEL_STATS); };
  NABoolean resetIndexLabelStats() { return (controlFlags2_ & RESET_GET_INDEX_LABEL_STATS) != 0; };

  void setEnableUpdStatsMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_UPD_STATS_MVGROUP : controlFlags2_ &= ~ENABLE_UPD_STATS_MVGROUP); };
  NABoolean enableUpdStatsMvgroup() { return (controlFlags2_ & ENABLE_UPD_STATS_MVGROUP) != 0; };

  void setDisableRefreshMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= DISABLE_REFRESH_MVGROUP : controlFlags2_ &= ~DISABLE_REFRESH_MVGROUP); };
  NABoolean disableRefreshMvgroup() { return (controlFlags2_ & DISABLE_REFRESH_MVGROUP) != 0; };

  void setDisableReorgMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= DISABLE_REORG_MVGROUP : controlFlags2_ &= ~DISABLE_REORG_MVGROUP); };
  NABoolean disableReorgMvgroup() { return (controlFlags2_ & DISABLE_REORG_MVGROUP) != 0; };

  void setEnableReorgMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= ENABLE_REORG_MVGROUP : controlFlags2_ &= ~ENABLE_REORG_MVGROUP); };
  NABoolean enableReorgMvgroup() { return (controlFlags2_ & ENABLE_REORG_MVGROUP) != 0; };  

  void setDisableUpdStatsMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= DISABLE_UPD_STATS_MVGROUP : controlFlags2_ &= ~DISABLE_UPD_STATS_MVGROUP); };
  NABoolean disableUpdStatsMvgroup() { return (controlFlags2_ & DISABLE_UPD_STATS_MVGROUP) != 0; };

  void setResetRefreshMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= RESET_REFRESH_MVGROUP : controlFlags2_ &= ~RESET_REFRESH_MVGROUP); };
  NABoolean resetRefreshMvgroup() { return (controlFlags2_ & RESET_REFRESH_MVGROUP) != 0; };

  void setResetReorgMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= RESET_REORG_MVGROUP : controlFlags2_ &= ~RESET_REORG_MVGROUP); };
  NABoolean resetReorgMvgroup() { return (controlFlags2_ & RESET_REORG_MVGROUP) != 0; };

  void setResetUpdStatsMvgroup(NABoolean v)
  {(v ? controlFlags2_ |= RESET_UPD_STATS_MVGROUP : controlFlags2_ &= ~RESET_UPD_STATS_MVGROUP); };
  NABoolean resetUpdStatsMvgroup() { return (controlFlags2_ & RESET_UPD_STATS_MVGROUP) != 0; };

  void setForceReorgTable(NABoolean v)
  {(v ? controlFlags_ |= FORCE_REORG_TABLE : controlFlags_ &= ~FORCE_REORG_TABLE); };
  NABoolean forceReorgTable() { return (controlFlags_ & FORCE_REORG_TABLE) != 0; };

  void setForceReorgIndex(NABoolean v)
  {(v ? controlFlags_ |= FORCE_REORG_INDEX : controlFlags_ &= ~FORCE_REORG_INDEX); };
  NABoolean forceReorgIndex() { return (controlFlags_ & FORCE_REORG_INDEX) != 0; };

  void setForceUpdStatsTable(NABoolean v)
  {(v ? controlFlags_ |= FORCE_UPD_STATS_TABLE : controlFlags_ &= ~FORCE_UPD_STATS_TABLE); };
  NABoolean forceUpdStatsTable() { return (controlFlags_ & FORCE_UPD_STATS_TABLE) != 0; };

  void setForceUpdStatsMvs(NABoolean v)
  {(v ? controlFlags_ |= FORCE_UPD_STATS_MVS : controlFlags_ &= ~FORCE_UPD_STATS_MVS); };
  NABoolean forceUpdStatsMvs() { return (controlFlags_ & FORCE_UPD_STATS_MVS) != 0; };

  void setCleanMaintainCIT(NABoolean v)
  {(v ? flags_ |= CLEAN_MAINTAIN_CIT : flags_ &= ~CLEAN_MAINTAIN_CIT); };
  NABoolean cleanMaintainCIT() { return (flags_ & CLEAN_MAINTAIN_CIT) != 0; };

  void setNoControlInfoUpdate(NABoolean v)
  {(v ? flags_ |= NO_CONTROL_INFO_UPDATE : flags_ &= ~NO_CONTROL_INFO_UPDATE); };
  NABoolean noControlInfoUpdate() { return (flags_ & NO_CONTROL_INFO_UPDATE) != 0; };

  void setNoControlInfoTable (NABoolean v)
  {(v ? flags_ |= NO_CONTROL_INFO_TABLE : flags_ &= ~NO_CONTROL_INFO_TABLE); };
  NABoolean noControlInfoTable() { return (flags_ & NO_CONTROL_INFO_TABLE) != 0; };

  void setNoOutput(NABoolean v)
  {(v ? flags_ |= NO_OUTPUT : flags_ &= ~NO_OUTPUT); };
  NABoolean noOutput() { return (flags_ & NO_OUTPUT) != 0; };

  char * getReorgTableOptions()    { return reorgTableOptions_; }
  char * getReorgIndexOptions()    { return reorgIndexOptions_; }
  char * getUpdStatsTableOptions() { return updStatsTableOptions_; }
  char * getUpdStatsMvlogOptions() { return updStatsMvlogOptions_; }
  char * getUpdStatsMvsOptions() { return updStatsMvsOptions_; }
  char * getUpdStatsMvgroupOptions() { return updStatsMvgroupOptions_; }
  char * getRefreshMvgroupOptions()    { return refreshMvgroupOptions_; }
  char * getRefreshMvsOptions()    { return refreshMvsOptions_; }
  char * getReorgMvgroupOptions()    { return reorgMvgroupOptions_; }
  char * getReorgMvsOptions()      { return reorgMvsOptions_; }
  char * getReorgMvsIndexOptions() { return reorgMvsIndexOptions_; }
  char * getCleanMaintainCITOptions() { return cleanMaintainCITOptions_; }

  Queue* getIndexList()       { return indexList_; }
  Queue* getRefreshMvgroupList()   { return refreshMvgroupList_; }
  Queue* getRefreshMvsList()       { return refreshMvsList_; }
  Queue* getReorgMvgroupList()     { return reorgMvgroupList_; }
  Queue* getReorgMvsList()         { return reorgMvsList_; }
  Queue* getReorgMvsIndexList()    { return reorgMvsIndexList_; }
  Queue* getUpdStatsMvgroupList()  { return updStatsMvgroupList_; }
  Queue* getUpdStatsMvsList()      { return updStatsMvsList_; }
  Queue* getMultiTablesNamesList() { return multiTablesNamesList_; }
  Queue* getSkippedMultiTablesNamesList() { return skippedMultiTablesNamesList_; }

  NABoolean isControl() { return (controlFlags_ != 0); };

  NABoolean isControl2() { return (controlFlags2_ != 0); };

  char * getParentTableName()  { return parentTableName_; }
  char *getSchemaName() {return schemaName_;}

  void setMaintainedTableCreateTime(Int64 createTime)
  { maintainedTableCreateTime_ = createTime;}

  Int64 getMaintainedTableCreateTime() { return maintainedTableCreateTime_; }

  void setParentTableObjectUID(Int64 objectUID)
  { parentTableObjectUID_ = objectUID;}

  Int64 getParentTableObjectUID() { return parentTableObjectUID_; }

  void setMultiTablesCreateTimeList(Queue* mtctl)
  { multiTablesCreateTimeList_ = mtctl;}
  Queue * getMultiTablesCreateTimeList()
  { return multiTablesCreateTimeList_; }

  NABoolean isCatalog() { return (ot_ == CATALOG_); };
  NABoolean isSchema() { return (ot_ == SCHEMA_); };  
  NABoolean isMV() { return (ot_ == MV_); };

  void setShortFormat(NABoolean v)
  {(v ? formatFlags_ |= SHORT_ : formatFlags_ &= ~SHORT_); };
  NABoolean shortFormat() { return (formatFlags_ & SHORT_) != 0; };

  void setLongFormat(NABoolean v)
  {(v ? formatFlags_ |= LONG_ : formatFlags_ &= ~LONG_); };
  NABoolean longFormat() { return (formatFlags_ & LONG_) != 0; };

  void setDetailFormat(NABoolean v)
  {(v ? formatFlags_ |= DETAIL_ : formatFlags_ &= ~DETAIL_); };
  NABoolean detailFormat() { return (formatFlags_ & DETAIL_) != 0; };

  void setTokenFormat(NABoolean v)
  {(v ? formatFlags_ |= TOKEN_ : formatFlags_ &= ~TOKEN_); };
  NABoolean tokenFormat() { return (formatFlags_ & TOKEN_) != 0; };

  void setCommandFormat(NABoolean v)
  {(v ? formatFlags_ |= COMMAND_ : formatFlags_ &= ~COMMAND_); };
  NABoolean commandFormat() { return (formatFlags_ & COMMAND_) != 0; };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------

  void displayContents(Space *space, ULng32 flag);

protected:
  //enum for flags_
  enum
  {
    REORG_TABLE          = 0x0001,
    REORG_INDEX          = 0x0002,
    UPD_STATS_TABLE      = 0x0004,
    UPD_STATS_MVLOG      = 0x0008,
    REFRESH_MVGROUP      = 0x0010,
    REFRESH_MVS          = 0x0020,
    REORG_MVS            = 0x0040,
    REORG_MVS_INDEX      = 0x0080,
    CONTINUE_ON_ERROR    = 0x0100,
    DISPLAY              = 0x0200,
    DISPLAY_DETAIL       = 0x0400,
    DO_SPECIFIED_TASK    = 0x0800,
    SKIP_REFRESH_MVS     = 0x1000,
    GET_STATUS           = 0x2000,
    INITIALIZE_MAINTAIN  = 0x4000,
    REINITIALIZE_MAINTAIN  = 0x8000,
    DROP_MAINTAIN          = 0x10000,
    UPD_STATS_MVS          = 0x20000,
    UPD_STATS_MVGROUP      = 0x40000,
    REORG_MVGROUP          = 0x80000,
    SKIP_REORG_MVS         = 0x100000,
    SKIP_UPD_STATS_MVS     = 0x200000,
    CLEAN_MAINTAIN_CIT     = 0x400000,
    GET_DETAILS            = 0x800000,
    CREATE_VIEW            = 0x1000000,
    DROP_VIEW              = 0x2000000,
    RUN                    = 0x4000000,
    IF_NEEDED              = 0x8000000,
    ALL_SPECIFIED          = 0x10000000,
    NO_CONTROL_INFO_UPDATE = 0x20000000,
    NO_OUTPUT              = 0x40000000,
    NO_CONTROL_INFO_TABLE  = 0x80000000,
   
  };
  //enum for flags2
  enum
    {
      
      GET_TABLE_LABEL_STATS = 0x0001,
      GET_INDEX_LABEL_STATS = 0x0002,
      GET_LABEL_STATS_INC_INDEXES = 0x0004,
      GET_LABEL_STATS_INC_INTERNAL = 0x0008,
      GET_LABEL_STATS_INC_RELATED = 0x0010,
      GET_SCHEMA_LABEL_STATS      = 0x0020

    };

  enum
  {
    DISABLE_REORG_TABLE       = 0x0001,
    ENABLE_REORG_TABLE        = 0x0002,
    DISABLE_REORG_INDEX       = 0x0004,
    ENABLE_REORG_INDEX        = 0x0008,
    DISABLE_UPD_STATS_TABLE   = 0x0010,
    ENABLE_UPD_STATS_TABLE    = 0x0020,
    RESET_REORG_TABLE         = 0x0040,
    RESET_UPD_STATS_TABLE     = 0x0080,
    FORCE_REORG_TABLE         = 0x0100,
    FORCE_REORG_INDEX         = 0x0200,
    FORCE_UPD_STATS_TABLE     = 0x0400,
    DISABLE_UPD_STATS_MVS     = 0x0800,
    ENABLE_UPD_STATS_MVS      = 0x1000,
    RESET_UPD_STATS_MVS       = 0x2000,
    FORCE_UPD_STATS_MVS       = 0x4000,
    DISABLE_REORG_MVS         = 0x8000
  };

  enum
  {
    ENABLE_REORG_MVS          =     0x0001,
    RESET_REORG_MVS           =     0x0002,
    DISABLE_REFRESH_MVS       =     0x0004,
    ENABLE_REFRESH_MVS        =     0x0008,
    RESET_REFRESH_MVS         =     0x0010,
    RESET_REORG_INDEX         =     0x0020,
    ENABLE_UPD_STATS_MVLOG    =     0x0040,
    DISABLE_UPD_STATS_MVLOG   =     0x0080,
    RESET_UPD_STATS_MVLOG     =     0x0100,
    ENABLE_REORG_MVS_INDEX    =     0x0200,
    DISABLE_REORG_MVS_INDEX   =     0x0400,
    RESET_REORG_MVS_INDEX     =     0x0800,
    ENABLE_REFRESH_MVGROUP    =     0x1000,
    DISABLE_REFRESH_MVGROUP   =     0x2000,
    RESET_REFRESH_MVGROUP     =     0x4000,
    ENABLE_REORG_MVGROUP      =     0x8000,
    DISABLE_REORG_MVGROUP     =     0x10000,
    RESET_REORG_MVGROUP       =     0x20000,
    ENABLE_UPD_STATS_MVGROUP   =    0x40000,
    DISABLE_UPD_STATS_MVGROUP  =    0x80000,
    RESET_UPD_STATS_MVGROUP    =    0x100000,
    ENABLE_GET_TABLE_LABEL_STATS =  0x200000,
    DISABLE_GET_TABLE_LABEL_STATS = 0x400000,
    RESET_GET_TABLE_LABEL_STATS=    0x800000,
    ENABLE_GET_INDEX_LABEL_STATS =  0x200000,
    DISABLE_GET_INDEX_LABEL_STATS = 0x400000,
    RESET_GET_INDEX_LABEL_STATS=    0x800000
  };

  enum GetStatsFormat
  {
    SHORT_   = 0x0001,
    LONG_    = 0x0002,
    DETAIL_  = 0x0004,
    TOKEN_   = 0x0008,
    COMMAND_ = 0x0010
  };

  NABasicPtr reorgTableOptions_;                          // 00-07
  NABasicPtr reorgIndexOptions_;                          // 08-15
  NABasicPtr updStatsTableOptions_;                       // 16-23
  NABasicPtr updStatsMvlogOptions_;                       // 24-31
  NABasicPtr refreshMvgroupOptions_;                      // 32-39
  NABasicPtr refreshMvsOptions_;                          // 40-47
  NABasicPtr reorgMvsOptions_;                            // 48-55
  NABasicPtr reorgMvsIndexOptions_;                       // 56-63

  // list of indexes on the table. Used with reorgIndex task
  QueuePtr indexList_;                               // 64-71

  // list of mvgroups on the table. Used with refreshMvgroup task
  QueuePtr refreshMvgroupList_;                           // 72-79

  // list of mvs on the table. Used with reorgMvs task
  QueuePtr refreshMvsList_;                               // 80-87

  // list of mvs on the table. Used with reorgMvs task
  QueuePtr reorgMvsList_;                                 // 88-95

  // list of indexes on the mvs. Used with reorgMvsIndex task
  QueuePtr reorgMvsIndexList_;                            // 96-103

  UInt32 flags_;                                          // 104-107

  UInt16 controlFlags_;                                   // 108-109

  UInt16 ot_;                                             // 110-111

  NABasicPtr parentTableName_;                            // 112-119
  UInt32 parentTableNameLen_;                             // 120-123

  UInt16 formatFlags_;                                    // 124-125
  UInt16 filler2_;                                        // 126-127

  Int64 maintainedTableCreateTime_;                       // 128-135
  Int64 parentTableObjectUID_;                            // 136-143

  NABasicPtr updStatsMvsOptions_;                         // 144-151
  NABasicPtr updStatsMvgroupOptions_;                     // 152-159
  NABasicPtr cleanMaintainCITOptions_;                    // 160-167

  // list of mvgroups on the table. Used with updStatsMvgroup task
  QueuePtr updStatsMvgroupList_;                           // 168-175

  // list of mvs on the table. Used with updStatsMvs task
  QueuePtr updStatsMvsList_;                               // 176-183

  // list of mvgroups on the table. Used with reorgMvgroup task
  QueuePtr reorgMvgroupList_;                              // 184-191
  NABasicPtr reorgMvgroupOptions_;                         // 192-199

  char filler3_[8];                                        // 200-207

  // Additional set of control flags
  UInt32 controlFlags2_;                                  // 208-211

  UInt32 flags2_;                                         // 212-215

  // start and end time to run maintain operations.
  // Used with 'RUN' option.
  Int64 from_;                                            // 216-223
  Int64 to_;                                              // 224-231

  // names of tables being reorged. Valid when MULTI_REORG is set.
  // Used when multiple tables are being reorged.
  QueuePtr multiTablesNamesList_;                         // 232-239

  QueuePtr multiTablesCreateTimeList_;                    // 240-247

  // names of tables being skipped due to error during compilation.
  // Used when multiple tables are being reorged.
  QueuePtr skippedMultiTablesNamesList_;                  // 248-255
  NABasicPtr schemaName_;                            // 256-263
  UInt32 schemaNameLen_;                             // 264-267
  char filler4_[4];                                  // 268-271
                       

};

class ComTdbExeUtilLoadVolatileTable : public ComTdbExeUtil
{
  friend class ExExeUtilLoadVolatileTableTcb;
  friend class ExExeUtilLoadVolatileTablePrivateState;

public:
  ComTdbExeUtilLoadVolatileTable()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilLoadVolatileTable(char * tableName,
				 ULng32 tableNameLen,
				 char * insertQuery,
				 char * updStatsQuery,
				 Int16 querycharset,
				 Int64 threshold,
				 ex_cri_desc * work_cri_desc,
				 const unsigned short work_atp_index,
				 ex_cri_desc * given_cri_desc,
				 ex_cri_desc * returned_cri_desc,
				 queue_index down,
				 queue_index up,
				 Lng32 num_buffers,
				 ULng32 buffer_size
				 );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilLoadVolatileTable);}

  virtual const char *getNodeName() const
  {
    return "LOAD_VOLATILE_TABLE";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  NABasicPtr insertQuery_;                                   // 00-07
  NABasicPtr updStatsQuery_;                                 // 08-15

  // automatic update stats is done if num rows inserted exceeds threshold.
  Int64      threshold_;                                     // 16-23
  UInt32 flags_;                                             // 24-27

  char fillersComTdbExeUtilLoadVolatileTable_[116];          // 28-147
};

class ComTdbExeUtilCleanupVolatileTables : public ComTdbExeUtil
{
  friend class ExExeUtilCleanupVolatileTablesTcb;
  friend class ExExeUtilCleanupVolatileTablesPrivateState;

public:
  ComTdbExeUtilCleanupVolatileTables()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilCleanupVolatileTables(char * catName,
				     ULng32 catNameLen,
				     ex_cri_desc * work_cri_desc,
				     const unsigned short work_atp_index,
				     ex_cri_desc * given_cri_desc,
				     ex_cri_desc * returned_cri_desc,
				     queue_index down,
				     queue_index up,
				     Lng32 num_buffers,
				     ULng32 buffer_size
				     );

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilCleanupVolatileTables);}

  virtual const char *getNodeName() const
  {
    return "CLEANUP_VOLATILE_TABLES";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  void setCleanupAllTables(NABoolean v)
  {(v ? flags_ |= CLEANUP_ALL_TABLES : flags_ &= ~CLEANUP_ALL_TABLES); };
  NABoolean cleanupAllTables() { return (flags_ & CLEANUP_ALL_TABLES) != 0; };
  void setCleanupHiveCSETables(NABoolean v)
  {(v ? flags_ |= CLEANUP_HIVE_CSE_TABLES : flags_ &= ~CLEANUP_HIVE_CSE_TABLES); }
  NABoolean cleanupHiveCSETables() { return (flags_ & CLEANUP_HIVE_CSE_TABLES) != 0; }

private:
  enum
  {
    // cleanup obsolete and active schemas/tables.
    CLEANUP_ALL_TABLES          = 0x0001,
    // cleanup Hive tables used for common subexpressions
    CLEANUP_HIVE_CSE_TABLES     = 0x0002
  };

  UInt32 flags_;                                             // 00-03

  char fillersComTdbExeUtilCleanupVolatileTables_[116];      // 04-119
};

class ComTdbExeUtilGetVolatileInfo : public ComTdbExeUtil
{
  friend class ExExeUtilGetVolatileInfoTcb;
  friend class ExExeUtilGetVolatileInfoPrivateState;

public:
  ComTdbExeUtilGetVolatileInfo()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilGetVolatileInfo(
			       char * param1,
			       char * param2,
			       ex_cri_desc * work_cri_desc,
			       const unsigned short work_atp_index,
			       ex_cri_desc * given_cri_desc,
			       ex_cri_desc * returned_cri_desc,
			       queue_index down,
			       queue_index up,
			       Lng32 num_buffers,
			       ULng32 buffer_size
			       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetVolatileInfo);}

  virtual const char *getNodeName() const
  {
    return "GET_VOLATILE_INFO";
  };

  void setAllSchemas(NABoolean v)
  {(v ? flags_ |= ALL_SCHEMAS : flags_ &= ~ALL_SCHEMAS); };
  NABoolean allSchemas() { return (flags_ & ALL_SCHEMAS) != 0; };

  void setAllTables(NABoolean v)
  {(v ? flags_ |= ALL_TABLES : flags_ &= ~ALL_TABLES); };
  NABoolean allTables() { return (flags_ & ALL_TABLES) != 0; };

  void setAllTablesInASession(NABoolean v)
  {(v ? flags_ |= ALL_TABLES_IN_A_SESSION : flags_ &= ~ALL_TABLES_IN_A_SESSION); };
  NABoolean allTablesInASession() { return (flags_ & ALL_TABLES_IN_A_SESSION) != 0; };

  void setVTCatSpecified(NABoolean v)
  {(v ? flags_ |= VT_CAT_SPECIFIED : flags_ &= ~VT_CAT_SPECIFIED); };
  NABoolean vtCatSpecified() { return (flags_ & VT_CAT_SPECIFIED) != 0; };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  enum GetType
  {
    ALL_SCHEMAS                 = 0x0001,
    ALL_TABLES                  = 0x0002,
    ALL_TABLES_IN_A_SESSION     = 0x0004,
    VT_CAT_SPECIFIED            = 0x0008
  };

  NABasicPtr param1_;                                        // 00-07
  NABasicPtr param2_;                                        // 08-15

  UInt32 flags_;                                             // 16-19

  char fillersComTdbExeUtilGetVolatileInfo_[116];            // 20-135
};

class ComTdbExeUtilGetErrorInfo : public ComTdbExeUtil
{
  friend class ExExeUtilGetErrorInfoTcb;
  friend class ExExeUtilGetErrorInfoPrivateState;

public:
  ComTdbExeUtilGetErrorInfo()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilGetErrorInfo(
			    Lng32 errNum,
			    ex_cri_desc * work_cri_desc,
			    const unsigned short work_atp_index,
			    ex_cri_desc * given_cri_desc,
			    ex_cri_desc * returned_cri_desc,
			    queue_index down,
			    queue_index up,
			    Lng32 num_buffers,
			    ULng32 buffer_size
			    );
  
  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetErrorInfo);}

  virtual const char *getNodeName() const
  {
    return "GET_ERROR_INFO";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  Lng32 errNum_;                                           // 00-03
  UInt32 flags_;                                           // 04-07

  char fillersComTdbExeUtilGetErrorInfo_[80];              // 08-87
};

class ComTdbExeUtilCreateTableAs : public ComTdbExeUtil
{
  friend class ExExeUtilCreateTableAsTcb;
  friend class ExExeUtilCreateTableAsPrivateState;

public:
  ComTdbExeUtilCreateTableAs()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilCreateTableAs(char * tableName,
			     ULng32 tableNameLen,
			     char * createStmtStr,
			     char * siStmtStr,
			     char * viStmtStr,
			     char * usStmtStr,
			     Int64 threshold,
			     ex_cri_desc * work_cri_desc,
			     const unsigned short work_atp_index,
			     ex_cri_desc * given_cri_desc,
			     ex_cri_desc * returned_cri_desc,
			     queue_index down,
			     queue_index up,
			     Lng32 num_buffers,
			     ULng32 buffer_size
			     );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilCreateTableAs);}

  virtual const char *getNodeName() const
  {
    return "CREATE_TABLE_AS";
  };

  void setLoadIfExists(NABoolean v)
  {(v ? flags_ |= LOAD_IF_EXISTS : flags_ &= ~LOAD_IF_EXISTS); };
  NABoolean loadIfExists() { return (flags_ & LOAD_IF_EXISTS) != 0; };

  void setNoLoad(NABoolean v)
  {(v ? flags_ |= NO_LOAD : flags_ &= ~NO_LOAD); };
  NABoolean noLoad() { return (flags_ & NO_LOAD) != 0; };

  void setIsVolatile(NABoolean v)
  {(v ? flags_ |= IS_VOLATILE : flags_ &= ~IS_VOLATILE); };
  NABoolean isVolatile() { return (flags_ & IS_VOLATILE) != 0; };

  void setDeleteData(NABoolean v)
  {(v ? flags_ |= DELETE_DATA : flags_ &= ~DELETE_DATA); };
  NABoolean deleteData() { return (flags_ & DELETE_DATA) != 0; };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  enum
  {
    LOAD_IF_EXISTS = 0x0001,
    NO_LOAD        = 0x0002,
    IS_VOLATILE    = 0x0004,
    DELETE_DATA    = 0x0008
  };

  // CREATE stmt
  NABasicPtr ctQuery_;                               // 00-07

  // Sidetree INSERT...SELECT stmt
  NABasicPtr siQuery_;                               // 08-15

  // VSBB INSERT...SELECT stmt
  NABasicPtr viQuery_;                               // 16-23

  // UPD STATS stmt
  NABasicPtr usQuery_;                               // 24-31

  // automatic update stats is done if num rows inserted exceeds threshold.
  Int64      threshold_;                             // 32-39

  UInt32 flags_;                                     // 40-43

  char fillersComTdbExeUtilCreateTableAs_[92];       // 44-135
};

class ComTdbExeUtilHiveTruncate : public ComTdbExeUtil
{
public:
  // flags
  enum
  {
    TRUNC_DROP_TABLE_ON_DEALLOC = 0x0001,
    IS_LEGACY                   = 0x0002,
    IS_EXTERNAL                 = 0x0004,
    IF_EXISTS                   = 0x0008,
    TABLE_NOT_EXISTS            = 0x0010
  };

  ComTdbExeUtilHiveTruncate()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilHiveTruncate(char * tableName,
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
                            ULng32 buffer_size
                            );
  
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilHiveTruncate);}

  virtual const char *getNodeName() const
  {
    return "HIVE_TRUNCATE";
  };

  char * getTableLocation() const
  {
    return tableLocation_;
  }

  char * getHdfsHost() const
  {
    return hdfsHost_;
  }

  Lng32 getHdfsPort() const
  {
    return hdfsPort_;
  }

  Int64 getModTS() const
  {
    return modTS_;
  }

  char * getPartnLocation() const 
  {
    return partnLocation_;
  }

  char * getHiveTableName() const
  {
    return hiveTableName_;
  }

  char * getHiveTruncQuery() const
  {
    return hiveTruncQuery_;
  }

  void setDropOnDealloc(NABoolean v)
  {(v ? flags_ |= TRUNC_DROP_TABLE_ON_DEALLOC : flags_ &= ~TRUNC_DROP_TABLE_ON_DEALLOC); }
  NABoolean getDropOnDealloc() { return (flags_ & TRUNC_DROP_TABLE_ON_DEALLOC) != 0; }

  void setIsLegacy(NABoolean v)
  {(v ? flags_ |= IS_LEGACY : flags_ &= ~IS_LEGACY); }
  NABoolean getIsLegacy() { return (flags_ & IS_LEGACY) != 0; }

  void setIsExternal(NABoolean v)
  {(v ? flags_ |= IS_EXTERNAL : flags_ &= ~IS_EXTERNAL); }
  NABoolean getIsExternal() { return (flags_ & IS_EXTERNAL) != 0; }

  void setIfExists(NABoolean v)
  {(v ? flags_ |= IF_EXISTS : flags_ &= ~IF_EXISTS); }
  NABoolean getIfExists() { return (flags_ & IF_EXISTS) != 0; }

  void setTableNotExists(NABoolean v)
  {(v ? flags_ |= TABLE_NOT_EXISTS : flags_ &= ~TABLE_NOT_EXISTS); }
  NABoolean getTableNotExists() { return (flags_ & TABLE_NOT_EXISTS) != 0; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  NABasicPtr hiveTableName_;                     // 00-07
  NABasicPtr tableLocation_;                     // 08-15
  NABasicPtr partnLocation_;                     // 16-23
  NABasicPtr hdfsHost_;                          // 24-31
  Int64 modTS_;                                  // 32-39
  Int32 hdfsPort_;                               // 40-43
  UInt32 flags_;                                 // 44-47
  NABasicPtr hiveTruncQuery_;                    // 48-55
};

class ComTdbExeUtilHiveQuery : public ComTdbExeUtil
{
public:
  ComTdbExeUtilHiveQuery()
  : ComTdbExeUtil()
  {}
  ComTdbExeUtilHiveQuery(char * hiveQuery,
                         ULng32 hiveQueryLen,
                         ex_cri_desc * given_cri_desc,
                         ex_cri_desc * returned_cri_desc,
                         queue_index down,
                         queue_index up,
                         Lng32 num_buffers,
                         ULng32 buffer_size
                         );
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilHiveQuery);}
  virtual const char *getNodeName() const
  {
    return "HIVE_QUERY";
  };
  char * getHiveQuery() const
  {
    return hiveQuery_;
  }
  void displayContents(Space *space, ULng32 flag);
private:
  NABasicPtr hiveQuery_;                     // 00-07
  UInt32 hiveQueryLen_;                      // 08-11
  UInt32 flags_;                             // 12-15
};
class ComTdbExeUtilGetStatistics : public ComTdbExeUtil
{
  friend class ExExeUtilGetStatisticsTcb;
  friend class ExExeUtilGetRTSStatisticsTcb;
  friend class ExExeUtilGetProcessStatisticsTcb;
  friend class ExExeUtilGetStatisticsPrivateState;

public:
  ComTdbExeUtilGetStatistics()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilGetStatistics(
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
       ULng32 buffer_size
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetStatistics);}

  virtual const char *getNodeName() const
  {
    return "GET_STATISTICS";
  };

  void setCompilerStats(NABoolean v)
  {(v ? flags_ |= COMPILER_STATS : flags_ &= ~COMPILER_STATS); };
  NABoolean compilerStats() { return (flags_ & COMPILER_STATS) != 0; };

  void setExecutorStats(NABoolean v)
  {(v ? flags_ |= EXECUTOR_STATS : flags_ &= ~EXECUTOR_STATS); };
  NABoolean executorStats() { return (flags_ & EXECUTOR_STATS) != 0; };

  void setOtherStats(NABoolean v)
  {(v ? flags_ |= OTHER_STATS : flags_ &= ~OTHER_STATS); };
  NABoolean otherStats() { return (flags_ & OTHER_STATS) != 0; };

  void setDetailedStats(NABoolean v)
  {(v ? flags_ |= DETAILED_STATS : flags_ &= ~DETAILED_STATS); };
  NABoolean detailedStats() { return (flags_ & DETAILED_STATS) != 0; };

  void setOldFormat(NABoolean v)
  {(v ? flags_ |= OLD_FORMAT : flags_ &= ~OLD_FORMAT); };
  NABoolean oldFormat() { return (flags_ & OLD_FORMAT) != 0; };

  void setShortFormat(NABoolean v)
  {(v ? flags_ |= SHORT_FORMAT : flags_ &= ~SHORT_FORMAT); };
  NABoolean shortFormat() { return (flags_ & SHORT_FORMAT) != 0; };

  void setTokenizedFormat(NABoolean v)
  {(v ? flags_ |= TOKENIZED_FORMAT : flags_ &= ~TOKENIZED_FORMAT); };
  NABoolean tokenizedFormat() { return (flags_ & TOKENIZED_FORMAT) != 0; };

  void setSingleLineFormat(NABoolean v)
  {(v ? flags_ |= SINGLELINE_FORMAT : flags_ &= ~SINGLELINE_FORMAT); };
  NABoolean singleLineFormat() { return (flags_ & SINGLELINE_FORMAT) != 0; };
  
  short getStatsReqType() { return statsReqType_; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);
  inline const char * getStmtName() const { return stmtName_.getPointer(); }

protected:
  enum
  {
    COMPILER_STATS   = 0x0001,
    EXECUTOR_STATS   = 0x0002,
    OTHER_STATS      = 0x0004,
    DETAILED_STATS   = 0x0008,
    OLD_FORMAT       = 0x0010,
    SHORT_FORMAT     = 0x0020,
    TOKENIZED_FORMAT = 0x0040,
    SINGLELINE_FORMAT = 0x0100
  };

  NABasicPtr stmtName_;                                        // 00-07

  UInt32 flags_;                                               // 08-11

  char filler1_[4];                                            // 12-15
  short statsReqType_;                                         // 16-17
  short statsMergeType_;                                       // 18-19

  short activeQueryNum_;                                       // 20-21

  char fillersComTdbExeUtilGetStatistics_[106];                // 22-127
};

class ComTdbExeUtilGetProcessStatistics : public ComTdbExeUtilGetStatistics
{

public:
  ComTdbExeUtilGetProcessStatistics()
  : ComTdbExeUtilGetStatistics()
  {}

  ComTdbExeUtilGetProcessStatistics(
				  char * pid,
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
				  ULng32 buffer_size
				  )
  : ComTdbExeUtilGetStatistics(pid, 
			       statsReqType, statsMergeType, activeQueryNum,
			       //SQLCLI_STATS_REQ_QID, SQLCLI_DEFAULT_STATS, -1,
			       work_cri_desc, work_atp_index,
			       given_cri_desc, returned_cri_desc,
			       down, up, num_buffers, buffer_size)
    {
      setNodeType(ComTdb::ex_PROCESS_STATISTICS);
    };
  

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetProcessStatistics);}

  virtual const char *getNodeName() const
  {
    return "PROCESS_STATISTICS";
  };

  inline const char * getPid() const { return getStmtName(); };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);
private:
};

///////////////////////////////////////////////////////////////////////////
static const ComTdbVirtTableColumnInfo exeUtilGetUIDVirtTableColumnInfo[] =
{
  { "GET_UID_OUTPUT",   0,    COM_USER_COLUMN, REC_BIN64_SIGNED,     8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

class ComTdbExeUtilGetUID : public ComTdbExeUtil
{
  friend class ExExeUtilGetUIDTcb;
  friend class ExExeUtilGetUIDPrivateState;

public:
  ComTdbExeUtilGetUID()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilGetUID(
       Int64 uid,
       ex_cri_desc * work_cri_desc,
       const unsigned short work_atp_index,
       ex_cri_desc * given_cri_desc,
       ex_cri_desc * returned_cri_desc,
       queue_index down,
       queue_index up,
       Lng32 num_buffers,
       ULng32 buffer_size
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetUID);}

  virtual const char *getNodeName() const
  {
    return "GET_UID";
  };

  static Int32 getVirtTableNumCols()
  {
    return sizeof(exeUtilGetUIDVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)exeUtilGetUIDVirtTableColumnInfo;
  }

  static Int32 getVirtTableNumKeys()
  {
    return 0;
  }

  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return NULL;
  }

  Int64 getUID() { return uid_; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  Int64 uid_;                                        // 00-07

  UInt32 flags_;                                     // 08-15

  char fillersComTdbExeUtilGetUID_[108];             // 16-133
};

///////////////////////////////////////////////////////////////////////////
static const ComTdbVirtTableColumnInfo exeUtilGetQIDVirtTableColumnInfo[] =
{
  { "GET_QID_OUTPUT",   0,    COM_USER_COLUMN, REC_BYTE_V_ASCII,     160, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

class ComTdbExeUtilGetQID : public ComTdbExeUtil
{
  friend class ExExeUtilGetQIDTcb;
  friend class ExExeUtilGetQIDPrivateState;

public:
  ComTdbExeUtilGetQID()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilGetQID(
                      char * stmtName,
                      ex_cri_desc * work_cri_desc,
                      const unsigned short work_atp_index,
                      ex_cri_desc * given_cri_desc,
                      ex_cri_desc * returned_cri_desc,
                      queue_index down,
                      queue_index up,
                      Lng32 num_buffers,
                      ULng32 buffer_size
                      );
  
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetQID);}

  virtual const char *getNodeName() const
  {
    return "GET_QID";
  };

  static Int32 getVirtTableNumCols()
  {
    return sizeof(exeUtilGetQIDVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)exeUtilGetQIDVirtTableColumnInfo;
  }

  static Int32 getVirtTableNumKeys()
  {
    return 0;
  }

  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return NULL;
  }

  char * getStmtName() { return stmtName_; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  NABasicPtr stmtName_;

  UInt32 flags_;                               

  char fillersComTdbExeUtilGetQID_[108];             
};

class ComTdbExeUtilPopulateInMemStats : public ComTdbExeUtil
{
  friend class ExExeUtilPopulateInMemStatsTcb;
  friend class ExExeUtilPopulateInMemStatsPrivateState;

public:
  ComTdbExeUtilPopulateInMemStats()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilPopulateInMemStats(
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
       ULng32 buffer_size
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilPopulateInMemStats);}

  virtual const char *getNodeName() const
  {
    return "POP_IN_MEM_STATS";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  inline const char * getInMemHistogramsTableName() const
     { return inMemHistogramsTableName_.getPointer() ; } ;

  inline const char * getInMemHistintsTableName() const
     { return inMemHistintsTableName_.getPointer() ; } ;

  inline const char * getSourceTableCatName() const
     { return sourceTableCatName_.getPointer() ; } ;

  inline const char * getSourceTableSchName() const
     { return sourceTableSchName_.getPointer() ; } ;

  inline const char * getSourceTableObjName() const
     { return sourceTableObjName_.getPointer() ; } ;

  inline const char * getSourceHistogramsTableName() const
     { return sourceHistogramsTableName_.getPointer(); } ;

  inline const char * getSourceHistintsTableName() const
     { return sourceHistintsTableName_.getPointer() ; } ;

private:
  Int64 uid_;                                        // 00-07
  NABasicPtr inMemHistogramsTableName_;              // 08-15
  NABasicPtr inMemHistintsTableName_;                // 16-23
  NABasicPtr sourceTableCatName_;                    // 24-31
  NABasicPtr sourceTableSchName_;                    // 32-39
  NABasicPtr sourceTableObjName_;                    // 40-47
  NABasicPtr sourceHistogramsTableName_;             // 48-55
  NABasicPtr sourceHistintsTableName_;               // 56-63

  UInt32 flags_;                                     // 64-67

  char fillersComTdbExeUtilPopInMemStats_[108];      // 68-175
};

class ComTdbExeUtilAqrWnrInsert : public ComTdbExeUtil
{
  friend class ExExeUtilAqrWnrInsertTcb;
public:
  ComTdbExeUtilAqrWnrInsert()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilAqrWnrInsert(char * tableName,
			      ULng32 tableNameLen,
			      ex_cri_desc * work_cri_desc,
			      const unsigned short work_atp_index,
			      ex_cri_desc * given_cri_desc,
			      ex_cri_desc * returned_cri_desc,
			      queue_index down,
			      queue_index up,
			      Lng32 num_buffers,
			      ULng32 buffer_size
			      );
#if 0
  no need to pack/unpack until this subclass has some ptr type members.
  Long pack (void *);
  Lng32 unpack(void *, void * reallocator);
#endif

  void lockTarget(bool lt) { lt ? aqrWnrInsflags_ |=  LOCK_TARGET :
  aqrWnrInsflags_ &= ~LOCK_TARGET ; }
  bool doLockTarget() const { return aqrWnrInsflags_ & LOCK_TARGET; }

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilAqrWnrInsert);}

  virtual const char *getNodeName() const
  {
    return "AQRWNR_INSERT_UTIL";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:

  enum
  {
    LOCK_TARGET             = 0x00000001
  };

  UInt32 aqrWnrInsflags_;                         // 00-03

  char fillersComTdbExeUtilAqrWnrInsert_[20];     // 20-39
};
class ComTdbExeUtilLongRunning : public ComTdbExeUtil
{
public:
  ComTdbExeUtilLongRunning()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilLongRunning(char * tableName,
			  ULng32 tableNameLen,
			  ex_cri_desc * work_cri_desc,
			  const unsigned short work_atp_index,
			  ex_cri_desc * given_cri_desc,
			  ex_cri_desc * returned_cri_desc,
			  queue_index down,
			  queue_index up,
			  Lng32 num_buffers,
			  ULng32 buffer_size
			  );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);


  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilLongRunning);}

  virtual const char *getNodeName() const
  {
    return "LONG_RUNNING";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  void setLongRunningDelete(NABoolean v)
  {(v ? flags_ |= LR_DELETE : flags_ &= ~LR_DELETE); };
  NABoolean longRunningDelete() { return (flags_ & LR_DELETE) != 0; };

  void setLongRunningUpdate(NABoolean v)
  {(v ? flags_ |= LR_UPDATE : flags_ &= ~LR_UPDATE); };
  NABoolean longRunningUpdate() { return (flags_ & LR_UPDATE) != 0; };

 void setLongRunningInsertSelect(NABoolean v)
  {(v ? flags_ |= LR_INSERT_SELECT : flags_ &= ~LR_INSERT_SELECT); };
  NABoolean longRunningInsertSelect() { return (flags_ & LR_INSERT_SELECT) != 0; };

 void setLongRunningQueryPlan(NABoolean v)
  {(v ? flags_ |= LR_QUERY_PLAN : flags_ &= ~LR_QUERY_PLAN); };
  NABoolean longRunningQueryPlan() { return (flags_ & LR_QUERY_PLAN) != 0; };

  void setUseParserflags(NABoolean v)
  {(v ? flags_ |= LR_PARSERFLAGS : flags_ &= ~LR_PARSERFLAGS); };
  NABoolean useParserflags() { return (flags_ & LR_PARSERFLAGS) != 0; };

 char * getLruStmt() { return lruStmt_; };
 void setLruStmt(char * stmt) { lruStmt_ = stmt;};

 Int64 getLruStmtLen() { return lruStmtLen_; };
 void setLruStmtLen(Int64 len) { lruStmtLen_ = len; };

 char * getLruStmtWithCK() { return lruStmtWithCK_; };
 void setLruStmtWithCK(char * stmt) { lruStmtWithCK_ = stmt;};

 Int64 getLruStmtWithCKLen() { return lruStmtWithCKLen_; };
 void setLruStmtWithCKLen (Int64 len) { lruStmtWithCKLen_ = len; };

 char *getPredicate() { return predicate_; };
 void setPredicate(Space *space, char *predicate);

 Int64 getPredicateLen() { return predicateLen_; };

 ULng32 getMultiCommitSize() {return multiCommitSize_;}
 void setMultiCommitSize(ULng32 multiCommitSize)
 {multiCommitSize_ = multiCommitSize;};

private:

  enum GetLongRunningType
    {
      LR_DELETE                  = 0x0001,
      LR_UPDATE                  = 0x0002,
      LR_INSERT_SELECT           = 0x0004,
      LR_QUERY_PLAN              = 0x0008,
      LR_PARSERFLAGS             = 0x0010
    };

  // Type of Long Running operation
  UInt32 flags_;                                // 00-03

  UInt32 multiCommitSize_;                      // 04-07

  // Statement1 string
  NABasicPtr lruStmt_;                          // 08-15

  // Statement1 length
  Int64 lruStmtLen_;                            // 16-23

  // Statement with CK string
  NABasicPtr lruStmtWithCK_;                    // 24-31

  // Statement with CK  length
  Int64 lruStmtWithCKLen_;                      // 32-39

  NABasicPtr predicate_;                        // 40-48

  Int64 predicateLen_;                          // 49-56

  char fillersComTdbExeUtilLongRunning_[72];    // 57-128
};

class ComTdbExeUtilShowSet : public ComTdbExeUtil
{
  friend class ExExeUtilShowSetTcb;
  friend class ExExeUtilShowSetPrivateState;

public:
  enum ShowSetType
  {
    ALL_                 = 1,
    EXTERNALIZED_,
    SINGLE_
  };

  ComTdbExeUtilShowSet()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilShowSet(
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
       ULng32 buffer_size
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilShowSet);}

  virtual const char *getNodeName() const
  {
    return "SHOWSET";
  };

  UInt16 getType() { return type_;}

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:

  NABasicPtr param1_;                                        // 00-07
  NABasicPtr param2_;                                        // 08-15

  UInt16 type_;
  UInt16 flags_;                                             // 16-19

  char fillersComTdbExeUtilShowSet_[116];                    // 20-135
};

class ComTdbExeUtilAQR : public ComTdbExeUtil
{
  friend class ExExeUtilAQRTcb;
  friend class ExExeUtilAQRPrivateState;

public:
  enum AQRTask
  {
    NONE_ = -1,
    GET_ = 0,
    ADD_, DELETE_, UPDATE_,
    CLEAR_, RESET_
  };

  ComTdbExeUtilAQR()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilAQR(
       Lng32 task,
       ex_cri_desc * given_cri_desc,
       ex_cri_desc * returned_cri_desc,
       queue_index down,
       queue_index up,
       Lng32 num_buffers,
       ULng32 buffer_size
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void setParams(Lng32 sqlcode,
		 Lng32 nskcode,
		 Lng32 retries,
		 Lng32 delay,
		 Lng32 type)
  {
    sqlcode_ = sqlcode;
    nskcode_ = nskcode;
    retries_ = retries;
    delay_ = delay;
    type_ = type;
  }

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilAQR);}

  virtual const char *getNodeName() const
  {
    return "AQR";
  };

  Lng32 getTask() { return task_; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  Lng32 task_;
  Lng32 sqlcode_;
  Lng32 nskcode_;
  Lng32 retries_;
  Lng32 delay_;
  Lng32 type_;
  UInt32 flags_;

  char fillersComTdbExeUtilAQR_[76];                   // 24-103
};

class ComTdbExeUtilGetMetadataInfo : public ComTdbExeUtil
{
  friend class ExExeUtilGetMetadataInfoTcb;
  friend class ExExeUtilGetMetadataInfoComplexTcb;
  friend class ExExeUtilGetMetadataInfoVersionTcb;
  friend class ExExeUtilGetHiveMetadataInfoTcb;
  friend class ExExeUtilGetMetadataInfoPrivateState;

public:
  enum QueryType
  {
    NO_QUERY_            = -1,

    CATALOGS_ = 1,

    SCHEMAS_IN_CATALOG_,
    VIEWS_IN_CATALOG_,
    INVALID_VIEWS_IN_CATALOG_,
    SEQUENCES_IN_CATALOG_,
    TABLES_IN_CATALOG_,
    OBJECTS_IN_CATALOG_,
    HIVE_REG_TABLES_IN_CATALOG_,
    HIVE_REG_VIEWS_IN_CATALOG_,
    HIVE_REG_SCHEMAS_IN_CATALOG_,
    HIVE_REG_OBJECTS_IN_CATALOG_,
    HIVE_EXT_TABLES_IN_CATALOG_,
    HBASE_REG_TABLES_IN_CATALOG_,

    FUNCTIONS_IN_SCHEMA_,
    INDEXES_IN_SCHEMA_,
    INVALID_VIEWS_IN_SCHEMA_,
    LIBRARIES_IN_SCHEMA_,
    OBJECTS_IN_SCHEMA_,
    PROCEDURES_IN_SCHEMA_,
    SEQUENCES_IN_SCHEMA_,
    TABLE_FUNCTIONS_IN_SCHEMA_,
    TABLES_IN_SCHEMA_,
    VIEWS_IN_SCHEMA_,

    INDEXES_ON_TABLE_,
    OBJECTS_ON_TABLE_,
    VIEWS_ON_TABLE_,
    VIEWS_ON_VIEW_,

    FUNCTIONS_FOR_LIBRARY_,
    PARTITIONS_FOR_INDEX_,
    PARTITIONS_FOR_TABLE_,
    PROCEDURES_FOR_LIBRARY_,
    TABLE_FUNCTIONS_FOR_LIBRARY_,

    OBJECTS_IN_VIEW_,
    TABLES_IN_VIEW_,
    VIEWS_IN_VIEW_,

    ROLES_,
    ROLES_FOR_ROLE_,
    ROLES_FOR_USER_,
    USERS_,
    USERS_FOR_ROLE_,

    FUNCTIONS_FOR_ROLE_,
    INDEXES_FOR_ROLE_,
    LIBRARIES_FOR_ROLE_,
    PRIVILEGES_FOR_ROLE_,
    PROCEDURES_FOR_ROLE_,
    SCHEMAS_FOR_ROLE_,
    TABLES_FOR_ROLE_,
    TABLE_FUNCTIONS_FOR_ROLE_,
    VIEWS_FOR_ROLE_,

    FUNCTIONS_FOR_USER_,
    INDEXES_FOR_USER_,
    LIBRARIES_FOR_USER_,
    PRIVILEGES_FOR_USER_,
    PROCEDURES_FOR_USER_,
    SCHEMAS_FOR_USER_,
    TABLES_FOR_USER_,
    TABLE_FUNCTIONS_FOR_USER_,
    VIEWS_FOR_USER_, 

    PRIVILEGES_ON_LIBRARY_,
    PRIVILEGES_ON_PROCEDURE_,
    PRIVILEGES_ON_ROUTINE_,
    PRIVILEGES_ON_TABLE_,
    PRIVILEGES_ON_SCHEMA_,
    PRIVILEGES_ON_SEQUENCE_,
    PRIVILEGES_ON_VIEW_,

    COMPONENTS_,
    COMPONENT_OPERATIONS_,
    COMPONENT_PRIVILEGES_,

    HBASE_OBJECTS_
    
    // Not supported at this time 
    // Kept around because we may support synonyms, triggers, or MV sometime
    
    //MVGROUPS_FOR_USER_,
    //MVS_FOR_USER_,
    //SYNONYMS_FOR_USER_,
    //TRIGGERS_FOR_USER_,
    //MVS_IN_MV_,
    
    //OBJECTS_IN_MV_,
    //TABLES_IN_MV_,
    
    //IUDLOG_TABLES_IN_SCHEMA_,
    //MVS_IN_SCHEMA_,
    //MVGROUPS_IN_SCHEMA_,
    //RANGELOG_TABLES_IN_SCHEMA_,
    //SYNONYMS_IN_SCHEMA_,
    //TRIGTEMP_TABLES_IN_SCHEMA_,
    
    //INDEXES_ON_MV_,
    //IUDLOG_TABLE_ON_MV_,
    //MVS_ON_MV_,
    //PRIVILEGES_ON_MV_,
    //RANGELOG_TABLE_ON_MV_,
    //TRIGTEMP_TABLE_ON_MV_,
    
    //IUDLOG_TABLE_ON_TABLE_,
    //MVS_ON_TABLE_,
    //MVGROUPS_ON_TABLE_,
    //RANGELOG_TABLE_ON_TABLE_,
    //SYNONYMS_ON_TABLE_,
    //TRIGTEMP_TABLE_ON_TABLE_,
    
    //MVS_ON_VIEW_,
  };

  ComTdbExeUtilGetMetadataInfo()
       : ComTdbExeUtil()
  {}

  ComTdbExeUtilGetMetadataInfo(
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
       char * zkPort
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  const char * server() const { return server_; }
  const char * zkPort() const { return zkPort_;}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetMetadataInfo);}

  virtual const char *getNodeName() const
  {
    return "GET_METADATA_INFO";
  };

  QueryType queryType() { return queryType_; }

  void setNoHeader(NABoolean v)
  {(v ? flags_ |= NO_HEADER : flags_ &= ~NO_HEADER); };
  NABoolean noHeader() { return (flags_ & NO_HEADER) != 0; };

  void setUserObjs(NABoolean v)
  {(v ? flags_ |= USER_OBJS : flags_ &= ~USER_OBJS); };
  NABoolean userObjs() { return (flags_ & USER_OBJS) != 0; };

  void setSystemObjs(NABoolean v)
  {(v ? flags_ |= SYSTEM_OBJS : flags_ &= ~SYSTEM_OBJS); };
  NABoolean systemObjs() { return (flags_ & SYSTEM_OBJS) != 0; };

  void setAllObjs(NABoolean v)
  {(v ? flags_ |= ALL_OBJS : flags_ &= ~ALL_OBJS); };
  NABoolean allObjs() { return (flags_ & ALL_OBJS) != 0; };

  void setExternalObjs(NABoolean v)
  {(v ? flags_ |= EXTERNAL_OBJS : flags_ &= ~EXTERNAL_OBJS); };
  NABoolean externalObjs() { return (flags_ & EXTERNAL_OBJS) != 0; };

  void setGroupBy(NABoolean v)
  {(v ? flags_ |= GROUP_BY : flags_ &= ~GROUP_BY); };
  NABoolean groupBy() { return (flags_ & GROUP_BY) != 0; };

  void setOrderBy(NABoolean v)
  {(v ? flags_ |= ORDER_BY : flags_ &= ~ORDER_BY); };
  NABoolean orderBy() { return (flags_ & ORDER_BY) != 0; };

  void setGetVersion(NABoolean v)
  {(v ? flags_ |= GET_VERSION : flags_ &= ~GET_VERSION); };
  NABoolean getVersion() { return (flags_ & GET_VERSION) != 0; };

  void setGetObjectUid(NABoolean v)
  {(v ? flags_ |= GET_OBJECT_UID : flags_ &= ~GET_OBJECT_UID); };
  NABoolean getObjectUid() { return (flags_ & GET_OBJECT_UID) != 0; };

  void setReturnFullyQualNames(NABoolean v)
  {(v ? flags_ |= RETURN_FULLY_QUAL_NAMES : flags_ &= ~RETURN_FULLY_QUAL_NAMES); };
  NABoolean returnFullyQualNames() { return (flags_ & RETURN_FULLY_QUAL_NAMES) != 0; };

 void setIsIndex(NABoolean v)
  {(v ? flags_ |= IS_INDEX : flags_ &= ~IS_INDEX); };
  NABoolean isIndex() { return (flags_ & IS_INDEX) != 0; };

  void setIsMv(NABoolean v)
  {(v ? flags_ |= IS_MV : flags_ &= ~IS_MV); };
  NABoolean isMv() { return (flags_ & IS_MV) != 0; };

  void setIsHbase(NABoolean v)
  {(v ? flags_ |= IS_HBASE : flags_ &= ~IS_HBASE); };
  NABoolean isHbase() { return (flags_ & IS_HBASE) != 0; };

  void setCascade(NABoolean v)
  {(v ? flags_ |= CASCADE : flags_ & CASCADE) != 0; };
  NABoolean cascade() { return (flags_ & CASCADE) != 0; };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

protected:
  enum
  {
    NO_HEADER    = 0x0001,
    USER_OBJS    = 0x0002,
    SYSTEM_OBJS  = 0x0004,
    ALL_OBJS     = 0x0008,
    GROUP_BY     = 0x0010,
    ORDER_BY     = 0x0020,
    GET_VERSION  = 0x0040,
    RETURN_FULLY_QUAL_NAMES = 0x0080,
    GET_OBJECT_UID = 0x0100,
    IS_INDEX     = 0x0200,
    IS_MV        = 0x0400,
    IS_HBASE   = 0x0800,
    EXTERNAL_OBJS = 0x1000,
    CASCADE      = 0x2000
  };

  char * getCat() { return cat_; }
  char * getSch() { return sch_; }
  char * getObj() { return obj_; }
  char * getPattern() { return pattern_; }
  char * getParam1() { return param1_; }

  QueryType queryType_;                              // 00-03

  char filler1_[4];                                  // 04-07

  // catalog name
  NABasicPtr cat_;                                   // 08-15

  // schema name
  NABasicPtr sch_;                                   // 16-23

  // object name
  NABasicPtr obj_;                                   // 24-31

  NABasicPtr pattern_;                               // 32-39

  NABasicPtr param1_;                                // 40-47

  UInt32 flags_;                                     // 48-51

  char filler2_[4];                                    // 52-55

  NABasicPtr server_;
  NABasicPtr zkPort_;

  char fillersComTdbExeUtilGetMetadataInfo_[80];     // 56-143
};

class ComTdbExeUtilGetHiveMetadataInfo : public ComTdbExeUtilGetMetadataInfo
{

public:

  ComTdbExeUtilGetHiveMetadataInfo()
       : ComTdbExeUtilGetMetadataInfo()
  {}

  ComTdbExeUtilGetHiveMetadataInfo(
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
       ULng32 buffer_size
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetHiveMetadataInfo);}

  virtual const char *getNodeName() const
  {
    return "GET_HIVE_METADATA_INFO";
  };

 // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

 private:

  NABasicPtr unused1_;
  NABasicPtr unused2_;
  NABasicPtr unused3_;
  NABasicPtr unused4_;
};

class ComTdbExeUtilLobExtract : public ComTdbExeUtil
{
  friend class ExExeUtilLobExtractTcb;
  friend class ExExeUtilFileExtractTcb;
  friend class ExExeUtilFileLoadTcb;
  friend class ExExeUtilLobExtractPrivateState;

public:
  enum ExtractToType
  {
    TO_FILE_, TO_STRING_, TO_BUFFER_, TO_EXTERNAL_FROM_STRING_,
    TO_EXTERNAL_FROM_FILE_, RETRIEVE_LENGTH_, RETRIEVE_HDFSFILENAME_,
    RETRIEVE_OFFSET_,NOOP_
  };
  

  ComTdbExeUtilLobExtract()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilLobExtract
    (
     char * handle,
     Lng32 handleLen,
     ExtractToType toType,
     Int64 bufAddr,
     Int64 extractSize,
     Int64 intParam1,
     Int64 intParam2,
     Int32 lobStorageType,
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
     ULng32 buffer_size
     );

  Long pack(void * space);
  Lng32 unpack(void * base, void * reallocator);

  char * getHandle() { return handle_; }
  Lng32 getHandleLen() { return handleLen_; }
  char * getFileName() { return stringParam1_; }
  char * getStringParam1() { return stringParam1_; }
  char * getStringParam2() { return stringParam2_; }
  char * getStringParam3() { return stringParam3_; }

  char * getLobHdfsServer() { return lobHdfsServer_; }
  Lng32 getLobHdfsPort() { return lobHdfsPort_; }

  ExtractToType getToType() { return (ExtractToType)toType_; }
 
  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilLobExtract);}

  virtual const char *getNodeName() const
  {
    return "LOB_EXTRACT";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  void setHandleInStringFormat(NABoolean v)
  {(v ? flags_ |= STRING_FORMAT : flags_ &= ~STRING_FORMAT); };
  NABoolean handleInStringFormat() { return (flags_ & STRING_FORMAT) != 0; };

 void setSrcIsFile(NABoolean v)
  {(v ? flags_ |= SRC_IS_FILE : flags_ &= ~SRC_IS_FILE); };
  NABoolean srcIsFile() { return (flags_ & SRC_IS_FILE) != 0; };

  // whether to create the file before load. Valid with toType of TO_EXTERNAL_FROM_*
 void setWithCreate(NABoolean v)
  {(v ? flags_ |= WITH_CREATE : flags_ &= ~WITH_CREATE); };
  NABoolean withCreate() { return (flags_ & WITH_CREATE) != 0; };
 
  void setRetrieveLength(NABoolean v)
  {(v ? flags_ |= RETRIEVE_LENGTH : flags_ &= ~RETRIEVE_LENGTH); };
  NABoolean retrieveLength() { return (flags_ & RETRIEVE_LENGTH) != 0; };
 
  void setRetrieveHdfsFileName(NABoolean v)
  {(v ? flags_ |= RETRIEVE_HDFSFILENAME : flags_ &= ~RETRIEVE_HDFSFILENAME); };
  NABoolean retrieveHdfsFileName() { return (flags_ & RETRIEVE_HDFSFILENAME) != 0; };
 void setRetrieveOffset(NABoolean v)
  {(v ? flags_ |= RETRIEVE_OFFSET : flags_ &= ~RETRIEVE_OFFSET); };
  NABoolean retrieveOffset() { return (flags_ & RETRIEVE_OFFSET) != 0; };
  void setErrorIfNotExists(NABoolean v)
  {(v ? flags_ |= ERROR_IF_NOT_EXISTS : flags_ &= ~ERROR_IF_NOT_EXISTS); };
  NABoolean errorIfNotExists() { return (flags_ & ERROR_IF_NOT_EXISTS) != 0; };

  void setErrorIfExists(NABoolean v)
  {(v ? flags_ |= ERROR_IF_EXISTS : flags_ &= ~ERROR_IF_EXISTS); };
  NABoolean errorIfExists() { return (flags_ & ERROR_IF_EXISTS) != 0; };
  
  void setTruncateExisting(NABoolean v)
  {(v ? flags_ |= TRUNCATE_EXISTING : flags_ &= ~TRUNCATE_EXISTING); };
  NABoolean truncateExisting() { return (flags_ & TRUNCATE_EXISTING) != 0; };
  
   void setAppendOrCreate(NABoolean v)
  {(v ? flags_ |= APPEND_OR_CREATE : flags_ &= ~APPEND_OR_CREATE); };
  NABoolean appendOrCreate() { return (flags_ & APPEND_OR_CREATE) != 0; };

  void setExtractSizeIOAddr(Int64 ext_addr){ extractSizeIOAddr_ = ext_addr;};
  Int64 dataExtractSizeIOAddr() { return extractSizeIOAddr_;}
  void setTotalBufSize(Int64 bufSize) { totalBufSize_ = bufSize;};
  Int64 getTotalBufSize() { return totalBufSize_;};
  void setBufAddr(Int64 bufAddr) {bufAddr_ = bufAddr;};
  Int64 getBufAddr() { return bufAddr_;};

private:
  enum
  {
    STRING_FORMAT    = 0x0001,
    SRC_IS_FILE           = 0x0002,
    WITH_CREATE        = 0x0004,
    RETRIEVE_LENGTH    =0x0008,
    ERROR_IF_NOT_EXISTS =0x0010,
    ERROR_IF_EXISTS     =0x0020,
    TRUNCATE_EXISTING = 0x0040,
    APPEND_OR_CREATE = 0x0080,
    RETRIEVE_HDFSFILENAME= 0x0100,
    RETRIEVE_OFFSET=0x0200
   
  
    
  };

  NABasicPtr handle_;                                      // 00-07
  short toType_;                                           // 08-09
  Int32 flags_;    
  Lng32 handleLen_;
  Int64 extractSizeIOAddr_; // as passed in via syntax size
  Int64 totalBufSize_; // buf size
  Int64 bufAddr_ ; //buffer addressed as passed in by user via syntax
  Int64 intParam1_;
  Int64 intParam2_;
  Lng32 lobStorageType_ ; // valid when  extract is from a file.
  Lng32 lobHdfsPort_;
  NABasicPtr lobHdfsServer_;
  NABasicPtr stringParam1_;
  NABasicPtr stringParam2_;
  NABasicPtr stringParam3_;

  char fillersComTdbExeUtilLobExtract_[40];            
};


class ComTdbExeUtilLobUpdate : public ComTdbExeUtil
{
   friend class ExExeUtilLobUpdateTcb;
   friend class ExExeUtilPrivateState;

public:
  enum UpdateFromType
  {
    FROM_BUFFER_, FROM_STRING_, FROM_EXTERNAL_,NOOP_
  };
 

  ComTdbExeUtilLobUpdate()
  : ComTdbExeUtil()
  {}
  ComTdbExeUtilLobUpdate
    (
     char * handle,
     Lng32 handleLen,
     UpdateFromType fromType,
     Int64 bufAddr,
     Int64 updateSize,
     Int32 lobStorageType,
     char * lobHdfsServer,
     Lng32 lobHdfsPort,
     char *loc,
     ex_expr * input_expr,
     ULng32 input_rowlen,
     ex_cri_desc * work_cri_desc,
     const unsigned short work_atp_index,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index down,
     queue_index up,
     Lng32 num_buffers,
     ULng32 buffer_size
     );
  Long pack(void * space);
  Lng32 unpack(void * base, void * reallocator);

  char * getHandle() { return handle_; }
  Lng32 getHandleLen() { return handleLen_; }
  char * getLobHdfsServer() { return lobHdfsServer_; }
  Lng32 getLobHdfsPort() { return lobHdfsPort_; }
  char * getLobLocation() { return lobLoc_;}

  UpdateFromType getFromType() { return (UpdateFromType)fromType_; }
 
  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilLobUpdate);}

  virtual const char *getNodeName() const
  {
    return "LOB_UPDATE_UTIL";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  void setAppend(NABoolean v)
  {(v ? flags_ |= APPEND_ : flags_ &= ~APPEND_); };
  NABoolean isAppend() { return (flags_ & APPEND_) != 0; };

  void setErrorIfExists(NABoolean v)
  {(v ? flags_ |= ERROR_IF_EXISTS_ : flags_ &= ~ERROR_IF_EXISTS_); };
  NABoolean isErrorIfExists() { return (flags_ & ERROR_IF_EXISTS_) != 0; };

  void setTruncate(NABoolean v)
  {(v ? flags_ |= TRUNCATE_ : flags_ &= ~TRUNCATE_); };
  NABoolean isTruncate() { return (flags_ & TRUNCATE_) != 0; };

  void setReplace(NABoolean v)
  {(v ? flags_ |= REPLACE_ : flags_ &= ~REPLACE_); };
  NABoolean isReplace() { return (flags_ & REPLACE_) != 0; };

  void setLobLocking(NABoolean v)
  {(v ? flags_ |= LOB_LOCKING_ : flags_ &= ~LOB_LOCKING_); };
  NABoolean lobLocking() { return (flags_ & LOB_LOCKING_) != 0; };

  void setUpdateSize(Int64 upd_size){ updateSize_ = upd_size;};
  Int64 updateSize() { return updateSize_;}
  void setTotalBufSize(Int64 bufSize) { totalBufSize_ = bufSize;};
  Int64 getTotalBufSize() { return totalBufSize_;};
  void setBufAddr(Int64 bufAddr) {bufAddr_ = bufAddr;};
  Int64 getBufAddr() { return bufAddr_;};
 void setLobMaxSize(Int64 lms) {lobMaxSize_ = lms;};
  Int64 getLobMaxSize() { return lobMaxSize_;};
 void setLobMaxChunkSize(Int64 lmcs) {lobMaxChunkSize_ = lmcs;};
  Int64 getLobMaxChunkSize() { return lobMaxChunkSize_;};
 void setLobGCLimit(Int64 gcl ) {lobGCLimit_ = gcl;};
  Int64 getLobGCLimit() { return lobGCLimit_;};
private:
  enum
    {
      ERROR_IF_EXISTS_ = 0x0001,
      TRUNCATE_ = 0x0002,
      APPEND_ = 0x0004,
      REPLACE_=0x0008,
      LOB_LOCKING_=0x0010
    };
  NABasicPtr handle_;
  Int32 handleLen_;
  short fromType_;
  Int32 flags_;
  Int64 updateSize_;
  Int64 totalBufSize_;
  Int64 bufAddr_;
  Lng32 lobStorageType_;
  Lng32 lobHdfsPort_;
  NABasicPtr lobHdfsServer_;
  NABasicPtr lobLoc_;
  Int64 lobMaxSize_;
  Int64 lobMaxChunkSize_;
  Int64 lobGCLimit_;
};
 
class ComTdbExeUtilLobShowddl : public ComTdbExeUtil
{
  friend class ExExeUtilLobShowddlTcb;
  friend class ExExeUtilLobShowddlPrivateState;

public:
  ComTdbExeUtilLobShowddl()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilLobShowddl
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
     ULng32 buffer_size
     );

  Long pack(void * space);
  Lng32 unpack(void * base, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilLobShowddl);}

  virtual const char *getNodeName() const
  {
    return "LOB_SHOWDDL";
  };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  short getLOBnum(short i);
  char * getLOBloc(short i);
  NABoolean getIsExternalLobCol(short i);
  UInt16 numLOBs() { return numLOBs_; }

  char * getLOBnumArray() { return lobNumArray_; }
  char * getLOBlocArray() { return lobLocArray_; }
  char * getLOBtypeArray() { return lobTypeArray_;}
 
  char * schName() { return schName_; };
private:
  UInt32 flags_;                                    

  UInt16 numLOBs_;                                  
  short maxLocLen_;
  
  // array of shorts. numLOBs entries. 
  // Each entry is the lobNum.
  NABasicPtr lobNumArray_;                           

  // array of string, null terminated. numLOBs entries. 
  // Each entry is the storage location of lob data file.
  NABasicPtr lobLocArray_;                          
  //array of int32. numLOBs entries
  NABasicPtr lobTypeArray_; 
  Int64 objectUID_;                                  

  NABasicPtr schName_;                               
  short schNameLen_;                                

  short sdOptions_;                                 
  char fillersComTdbExeUtilLobShowddl_[4];           
};


static const ComTdbVirtTableColumnInfo hiveMDTablesColInfo[] =
{                                                                                     
  { "CATALOG_NAME",   0,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",    1,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",     2,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "FILE_FORMAT",    3,      COM_USER_COLUMN, REC_BYTE_F_ASCII,     24, FALSE , SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "CREATE_TIME",    4,      COM_USER_COLUMN, REC_BIN64_SIGNED,      8, FALSE , SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "NUM_COLS",       5,      COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE , SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "NUM_PART_COLS",  6,      COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE , SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "NUM_BUCKET_COLS",7,      COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE , SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "NUM_SORT_COLS",  8,      COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE , SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "HIVE_OWNER",     9,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "FIELD_DELIMITER",   10,  COM_USER_COLUMN, REC_BIN16_SIGNED,      2, FALSE, SQLCHARSETCODE_UNKNOWN,   0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "RECORD_TERMINATOR", 11,  COM_USER_COLUMN, REC_BIN16_SIGNED,       2, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "NULL_FORMAT",       12,  COM_USER_COLUMN, REC_BYTE_F_ASCII,       8, FALSE, SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "LOCATION",          13,  COM_USER_COLUMN, REC_BYTE_F_ASCII,    1024, FALSE , SQLCHARSETCODE_UTF8,    0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "HIVE_TABLE_TYPE",   14,  COM_USER_COLUMN, REC_BYTE_F_ASCII,     128, FALSE , SQLCHARSETCODE_UTF8,    0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDTablesColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
  char fileFormat[24];
  Int64 createTime;
  Int32 numCols;
  Int32 numPartCols;
  Int32 numBucketCols;
  Int32 numSortCols;
  char  hiveOwner[256];
  Int16 fieldDelimiter;
  Int16 recordTerminator;
  char nullFormat[8];
  char location[1024];
  char hiveTableType[128];
};

#define HIVEMD_DATA_TYPE_LEN 32
#define HIVEMD_DISPLAY_DATA_TYPE_LEN 96
#define HIVEMD_CHARSET_LEN 40
#define HIVEMD_DT_QUALIFIER_LEN 28
static const ComTdbVirtTableColumnInfo hiveMDColumnsColInfo[] =
{                                                                                     
  { "CATALOG_NAME",          0, COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE, SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",           1, COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE, SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",            2, COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE, SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "COLUMN_NAME",           3, COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE, SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "SQL_DATA_TYPE",         4, COM_USER_COLUMN, REC_BYTE_F_ASCII,     HIVEMD_DATA_TYPE_LEN, FALSE, SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "FS_DATA_TYPE",          5, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "DISPLAY_DATA_TYPE",     6, COM_USER_COLUMN, REC_BYTE_F_ASCII,     HIVEMD_DISPLAY_DATA_TYPE_LEN, FALSE, SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "HIVE_DATA_TYPE",        7, COM_USER_COLUMN, REC_BYTE_F_ASCII,     HIVEMD_DATA_TYPE_LEN, FALSE, SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "COLUMN_SIZE",           8, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "CHARACTER_SET",         9, COM_USER_COLUMN, REC_BYTE_F_ASCII,     HIVEMD_CHARSET_LEN, FALSE, SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}, 
  { "COLUMN_PRECISION",     10, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "COLUMN_SCALE",         11, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "DT_CODE",              12, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "NULLABLE",             13, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "COLUMN_NUMBER",        14, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}, 
  { "PART_COL_NUMBER",      15, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}, 
  { "BUCKET_COL_NUMBER",    16, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}, 
  { "SORT_COL_NUMBER",      17, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}, 
  { "DATETIME_QUALIFIER",   18, COM_USER_COLUMN, REC_BYTE_F_ASCII,     HIVEMD_DT_QUALIFIER_LEN, FALSE, SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "DATETIME_START_FIELD", 19, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "DATETIME_END_FIELD",   20, COM_USER_COLUMN, REC_BIN32_SIGNED,      4, FALSE, SQLCHARSETCODE_UNKNOWN,  0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "DEFAULT_VALUE",        21, COM_USER_COLUMN, REC_BYTE_F_ASCII,    240, FALSE, SQLCHARSETCODE_UTF8,     0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
};

struct HiveMDColumnsColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
  char colName[256];
  char sqlDatatype[32];
  Lng32 fsDatatype;
  char displayDatatype[96];
  char hiveDatatype[32];
  Lng32 colSize;
  char charSet[40];
  Lng32 colPrecision;
  Lng32 colScale;
  Lng32 dtCode;
  Lng32 nullable;
  Lng32 colNum;
  Lng32 partColNum;
  Lng32 bucketColNum;
  Lng32 sortColNum;
  char dtQualifier[28];
  Lng32 dtStartField;
  Lng32 dtEndField;
  char defVal[240];
};

static const ComTdbVirtTableKeyInfo hiveMDColumnsVirtTableKeyInfo[] =
{
  //  colname keyseqnumber   tablecolnumber   ordering
  {    NULL,          1,            0,            0 , 0, NULL, NULL },
  {    NULL,          2,            1,            0 , 0, NULL, NULL },
  {    NULL,          2,            2,            0 , 0, NULL, NULL }
};

static const ComTdbVirtTableColumnInfo hiveMDPKeysColInfo[] =
{                                                                                     
  { "CATALOG_NAME",   0,       COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",     1,    COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",        2, COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "COLUMN_NAME",     3,    COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "ORDINAL_POSITION", 4,          COM_USER_COLUMN, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDPKeysColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
  char colName[256];
  Lng32 ordPos;
};

static const ComTdbVirtTableColumnInfo hiveMDFKeysColInfo[] =
{                                                                                     
  { "CATALOG_NAME",   0,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",   1,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",      2,   COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "COLUMN_NAME",    3,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "KEY_SEQ",      4,     COM_USER_COLUMN, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDFKeysColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
  char colName[256];
  Lng32 keySeq;
};

static const ComTdbVirtTableColumnInfo hiveMDViewsColInfo[] =
{                                                                                    
  { "CATALOG_NAME",   0,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",   1,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "VIEW_NAME",    2,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDViewsColInfoStruct
{
  char catName[256];
  char schName[256];
  char viewName[256];
};

static const ComTdbVirtTableColumnInfo hiveMDAliasColInfo[] =
{                                                                                    
  { "CATALOG_NAME",   0,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",    1,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",    2,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "ALIAS_NAME",   3,   COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL
,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDAliasColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
  char aliasName[256];
};

static const ComTdbVirtTableColumnInfo hiveMDSynonymColInfo[] =
{                                                                                    
  { "CATALOG_NAME",  0,       COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",   1,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",    2,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "SYN_NAME",   3,   COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL
,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDSynonymColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
  char synName[256];
};

static const ComTdbVirtTableColumnInfo hiveMDSysTablesColInfo[] =
{                                                                                     
  { "CATALOG_NAME",    0,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",    1,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",     2,    COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDSysTablesColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
};

static const ComTdbVirtTableColumnInfo hiveMDSchemasColInfo[] =
{                                                                                     
  { "CATALOG_NAME",    0,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",    1,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDSchemasColInfoStruct
{
  char catName[256];
  char schName[256];
};

class ComTdbExeUtilHiveMDaccess : public ComTdbExeUtil
{
  friend class ExExeUtilHiveMDaccessTcb;

public:
  enum MDType
  {
    NOOP_,
    TABLES_,
    COLUMNS_,
    PKEYS_,
    VIEWS_,
    FKEYS_,
    ALIAS_,
    SYNONYMS_,
    SYSTEM_TABLES_,
    SCHEMAS_
  };

  // Constructors

  // Default constructor (used in ComTdb::fixupVTblPtr() to extract
  // the virtual table after unpacking.

  ComTdbExeUtilHiveMDaccess();

  // Constructor used by the generator.
  ComTdbExeUtilHiveMDaccess(
	       MDType type,
	       ULng32 returnedTuplelen,
	       ex_cri_desc *criDescParentDown,
	       ex_cri_desc *criDescParentUp,
	       ex_cri_desc *workCriDesc,
	       unsigned short workAtpIndex,
	       queue_index queueSizeDown,
	       queue_index queueSizeUp,
	       Lng32 numBuffers,
	       ULng32 bufferSize,
	       ex_expr *scanExpr,
	       char * hivePredStr,
               char * catalogName,
               char * schemaName,
               char * objectName
	       );

  char * hivePredStr() { return hivePredStr_;}
  char * getCatalog()  { return catalog_; }
  char * getSchema()   { return schema_; }
  char * getObject()   { return object_; }

  // This always returns TRUE for now
  Int32 orderedQueueProtocol() const { return -1; };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(ComTdbExeUtilHiveMDaccess); }

  // Pack and Unpack routines
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // For the GUI, Does nothing right now
  void display() const {};

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

  // Virtual routines to provide a consistent interface to TDB's

  virtual const ComTdb *getChild(Int32 /*child*/) const { return NULL; };

  // numChildren always returns 0 for ComTdbStats
  virtual Int32 numChildren() const { return 0; };

  virtual const char *getNodeName() const { return "EX_HIVE_MD"; };

  // numExpressions always returns 2 for ComTdbStats
  virtual Int32 numExpressions() const;
  
  // The names of the expressions
  virtual const char * getExpressionName(Int32) const;

  // The expressions themselves
  virtual ex_expr* getExpressionNode(Int32);

  static Int32 getVirtTableNumCols(char * name)
  {
    if (strcmp(name, "TABLES") == 0)
      return sizeof(hiveMDTablesColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else if (strcmp(name, "COLUMNS") == 0)
      return sizeof(hiveMDColumnsColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else if (strcmp(name, "PKEYS") == 0)
      return sizeof(hiveMDPKeysColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else if (strcmp(name, "FKEYS") == 0)
      return sizeof(hiveMDFKeysColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else if (strcmp(name, "VIEWS") == 0)
      return sizeof(hiveMDViewsColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else if (strcmp(name, "ALIAS") == 0)
      return sizeof(hiveMDAliasColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else if (strcmp(name, "SYNONYMS") == 0)
      return sizeof(hiveMDSynonymColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else if (strcmp(name, "SYSTEM_TABLES") == 0)
      return sizeof(hiveMDSysTablesColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else if (strcmp(name, "SCHEMAS") == 0)
      return sizeof(hiveMDSchemasColInfo)/sizeof(ComTdbVirtTableColumnInfo);
    else
      return -1;
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo (char * name)
  {
    if (strcmp(name, "TABLES") == 0)
      return  (ComTdbVirtTableColumnInfo*)hiveMDTablesColInfo;
    else if (strcmp(name, "COLUMNS") == 0)
      return (ComTdbVirtTableColumnInfo*)hiveMDColumnsColInfo;
    else if (strcmp(name, "PKEYS") == 0)
      return (ComTdbVirtTableColumnInfo*)hiveMDPKeysColInfo;
    else if (strcmp(name, "FKEYS") == 0)
      return (ComTdbVirtTableColumnInfo*)hiveMDFKeysColInfo;
    else if (strcmp(name, "VIEWS") == 0)
      return (ComTdbVirtTableColumnInfo*)hiveMDViewsColInfo;
    else if (strcmp(name, "ALIAS") == 0)
      return (ComTdbVirtTableColumnInfo*)hiveMDAliasColInfo;
    else if (strcmp(name, "SYNONYMS") == 0)
      return (ComTdbVirtTableColumnInfo*)hiveMDSynonymColInfo;
    else if (strcmp(name, "SYSTEM_TABLES") == 0)
      return (ComTdbVirtTableColumnInfo*)hiveMDSysTablesColInfo;
    else if (strcmp(name, "SCHEMAS") == 0)
      return (ComTdbVirtTableColumnInfo*)hiveMDSchemasColInfo;
    else
      return NULL;
  }

  static Int32 getVirtTableNumKeys(char * name)
  {
    if (strcmp(name, "COLUMNS") == 0)
      return sizeof(hiveMDColumnsVirtTableKeyInfo)/sizeof(ComTdbVirtTableKeyInfo);
    else
      return 0;
  }

  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo(char * name)
  {
    if (strcmp(name, "COLUMNS") == 0)
      return (ComTdbVirtTableKeyInfo *)hiveMDColumnsVirtTableKeyInfo;
    else
      return NULL;
  }

private:

  Int32 mdType_;                                                   // 00 - 03

  char fillers_[4];                                                // 04 - 07
  
  NABasicPtr hivePredStr_;                                         // 08 - 15

  NABasicPtr catalog_;                                             // 16 - 23

  NABasicPtr schema_;                                              // 24 - 31

  NABasicPtr object_;                                              // 32 - 39
};

class ComTdbExeUtilHBaseBulkLoad : public ComTdbExeUtil
{
  friend class ExExeUtilHBaseBulkLoadTcb;
  friend class ExExeUtilHbaseLoadPrivateState;

public:
  ComTdbExeUtilHBaseBulkLoad()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilHBaseBulkLoad(char * tableName,
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
                             char * logLocation
                             );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilHBaseBulkLoad);}

  virtual const char *getNodeName() const
  {
    return "HBASE_BULK_LOAD";
  };

  char * getErrCountTable() { return errCountTable_ ; }
  char * getLoggingLocation() {return loggingLocation_;}

  void setPreloadCleanup(NABoolean v)
  {(v ? flags_ |= PRE_LOAD_CLEANUP : flags_ &= ~PRE_LOAD_CLEANUP); };
  NABoolean getPreloadCleanup() { return (flags_ & PRE_LOAD_CLEANUP) != 0; };

  void setPreparation(NABoolean v)
  {(v ? flags_ |= PREPARATION : flags_ &= ~PREPARATION); };
  NABoolean getPreparation() { return (flags_ & PREPARATION) != 0; };

  void setKeepHFiles(NABoolean v)
  {(v ? flags_ |= KEEP_HFILES : flags_ &= ~KEEP_HFILES); };
  NABoolean getKeepHFiles() { return (flags_ & KEEP_HFILES) != 0; };

  void setTruncateTable(NABoolean v)
  {(v ? flags_ |= TRUNCATE_TABLE : flags_ &= ~TRUNCATE_TABLE); };
  NABoolean getTruncateTable() { return (flags_ & TRUNCATE_TABLE) != 0; };

  void setNoRollback(NABoolean v)
  {(v ? flags_ |= NO_ROLLBACK : flags_ &= ~NO_ROLLBACK); };
  NABoolean getNoRollback() { return (flags_ & NO_ROLLBACK) != 0; };

  void setLogErrorRows(NABoolean v)
  {(v ? flags_ |= LOG_ERROR_ROWS : flags_ &= ~LOG_ERROR_ROWS); };
  NABoolean getLogErrorRows() { return (flags_ & LOG_ERROR_ROWS) != 0; };

  void setContinueOnError(NABoolean v)
  {(v ? flags_ |= CONTINUE_ON_ERROR : flags_ &= ~CONTINUE_ON_ERROR); };
  NABoolean getContinueOnError() { return (flags_ & CONTINUE_ON_ERROR) != 0; };

  void setSecure(NABoolean v)
  {(v ? flags_ |= SECURE : flags_ &= ~SECURE); };
  NABoolean getSecure() { return (flags_ & SECURE) != 0; };

  void setNoDuplicates(NABoolean v)
    {(v ? flags_ |= NO_DUPLICATES : flags_ &= ~NO_DUPLICATES); };
  NABoolean getNoDuplicates() { return (flags_ & NO_DUPLICATES) != 0; };

  void setRebuildIndexes(NABoolean v)
    {(v ? flags_ |= REBUILD_INDEXES : flags_ &= ~REBUILD_INDEXES); };
  NABoolean getRebuildIndexes() { return (flags_ & REBUILD_INDEXES) != 0; };

  void setConstraints(NABoolean v)
    {(v ? flags_ |= CONSTRAINTS : flags_ &= ~CONSTRAINTS); };
  NABoolean getConstraints() { return (flags_ & CONSTRAINTS) != 0; };

  void setNoOutput(NABoolean v)
    {(v ? flags_ |= NO_OUTPUT : flags_ &= ~NO_OUTPUT); };
  NABoolean getNoOutput() { return (flags_ & NO_OUTPUT) != 0; };

  void setIndexTableOnly(NABoolean v)
    {(v ? flags_ |= INDEX_TABLE_ONLY : flags_ &= ~INDEX_TABLE_ONLY); };
  NABoolean getIndexTableOnly() { return (flags_ & INDEX_TABLE_ONLY) != 0; };
  void setUpsertUsingLoad(NABoolean v)
    {(v ? flags_ |= UPSERT_USING_LOAD : flags_ &= ~UPSERT_USING_LOAD); };
  NABoolean getUpsertUsingLoad() { return (flags_ & UPSERT_USING_LOAD) != 0; };

  void setForceCIF(NABoolean v)
    {(v ? flags_ |= FORCE_CIF : flags_ &= ~FORCE_CIF); };
  NABoolean getForceCIF() { return (flags_ & FORCE_CIF) != 0; };

  void setUpdateStats(NABoolean v)
    {
    (v ? flags_ |= UPDATE_STATS : flags_ &= ~UPDATE_STATS); };
  NABoolean getUpdateStats() { return (flags_ & UPDATE_STATS) != 0; };
  void setMaxErrorRows (Int32 v) {    maxErrorRows_ = v; }

  Int32 getMaxErrorRows() const  { return maxErrorRows_; }

  void setHasUniqueIndexes(NABoolean v)
    {(v ? flags_ |= HAS_UNIQUE_INDEXES : flags_ &= ~HAS_UNIQUE_INDEXES); };
  NABoolean getHasUniqueIndexes() { return (flags_ & HAS_UNIQUE_INDEXES) != 0; };


  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  enum
  {
    PRE_LOAD_CLEANUP = 0x0001,
    PREPARATION      = 0x0002,
    KEEP_HFILES      = 0x0004,
    TRUNCATE_TABLE   = 0x0008,
    NO_ROLLBACK      = 0x0010,
    LOG_ERROR_ROWS   = 0x0020,
    SECURE           = 0x0040,
    NO_DUPLICATES    = 0x0080,
    REBUILD_INDEXES  = 0x0100,
    CONSTRAINTS      = 0x0200,
    NO_OUTPUT        = 0x0400,
    INDEX_TABLE_ONLY = 0x0800,
    UPSERT_USING_LOAD= 0x1000,
    FORCE_CIF        = 0x2000,
    UPDATE_STATS     = 0x4000,
    CONTINUE_ON_ERROR= 0x8000,
    HAS_UNIQUE_INDEXES= 0x10000
  };

  // load stmt
  NABasicPtr ldQuery_;                               // 00-07

  UInt32 flags_;                                     // 08-11
  Int32  maxErrorRows_;                              // 12-15
  NABasicPtr errCountTable_;                         // 16-23
  NABasicPtr loggingLocation_;                       // 24-31
  char fillersExeUtilHbaseLoad_[8];                  // 32-39
};

//******************************************************
// Bulk Unload
//**************************8

class ComTdbExeUtilHBaseBulkUnLoad : public ComTdbExeUtil
{
  friend class ExExeUtilHBaseBulkUnLoadTcb;
  friend class ExExeUtilHbaseUnLoadPrivateState;

public:
  enum ScanTypeEnum
  {
    REGULAR_SCAN = 0,
    SNAPSHOT_SCAN_CREATE = 1,
    SNAPSHOT_SCAN_EXISTING = 2
  };
  ComTdbExeUtilHBaseBulkUnLoad()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilHBaseBulkUnLoad(char * tableName,
                             ULng32 tableNameLen,
                             char * ldStmtStr,
                             char * extractLocation,
                             ex_cri_desc * work_cri_desc,
                             const unsigned short work_atp_index,
                             ex_cri_desc * given_cri_desc,
                             ex_cri_desc * returned_cri_desc,
                             queue_index down,
                             queue_index up,
                             Lng32 num_buffers,
                             ULng32 buffer_size
                             );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilHBaseBulkUnLoad);}

  virtual const char *getNodeName() const
  {
    return "HBASE_BULK_UNLOAD";
  };



  void setEmptyTarget(NABoolean v)
  {(v ? flags_ |= EMPTY_TARGET : flags_ &= ~EMPTY_TARGET); };
  NABoolean getEmptyTarget() { return (flags_ & EMPTY_TARGET) != 0; };

  void setLogErrors(NABoolean v)
  {(v ? flags_ |= LOG_ERRORS : flags_ &= ~LOG_ERRORS); };
  NABoolean getLogErrors() { return (flags_ & LOG_ERRORS) != 0; };

  void setNoOutput(NABoolean v)
    {(v ? flags_ |= NO_OUTPUT : flags_ &= ~NO_OUTPUT); };
  NABoolean getNoOutput() { return (flags_ & NO_OUTPUT) != 0; };

  void setCompressType(UInt8 v){
    compressType_ = v;
  };
  UInt8 getCompressType() {
    return compressType_;
  };
  void setOneFile(NABoolean v)
    {(v ? flags_ |= ONE_FILE : flags_ &= ~ONE_FILE); };
  NABoolean getOneFile() { return (flags_ & ONE_FILE) != 0; };

  void setMergePath(char * v){
    mergePath_ = v;
  }
  char * getMergePath() const {
    return mergePath_;
  }

  void setExtractLocation(char * v){
     extractLocation_ = v;
  }
  char * getExtractLocation() const {
     return extractLocation_;
  }
  void setSkipWriteToFiles(NABoolean v)
      {(v ? flags_ |= SKIP_WRITE_TO_FILES : flags_ &= ~SKIP_WRITE_TO_FILES); };
  NABoolean getSkipWriteToFiles() { return (flags_ & SKIP_WRITE_TO_FILES) != 0; };

  void setOverwriteMergeFile(NABoolean v)
      {(v ? flags_ |= OVERWRITE_MERGE_FILE : flags_ &= ~OVERWRITE_MERGE_FILE); };
  NABoolean getOverwriteMergeFile() { return (flags_ & OVERWRITE_MERGE_FILE) != 0; };

  void setScanType(UInt8 v){
    //scan type values are set originally in parser can be only o, 1 or 2.
    assert(v == REGULAR_SCAN || v == SNAPSHOT_SCAN_CREATE || v == SNAPSHOT_SCAN_EXISTING);
    scanType_ = v;
  };
  UInt8 getScanType() {
    return scanType_;
  };
  void setSnapshotSuffix(char * v){
    snapshotSuffix_ = v;
  }
  char * getSnapshotSuffix() const {
     return snapshotSuffix_;
  }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space, ULng32 flag);

private:
  enum
  {
    EMPTY_TARGET        = 0x0001,
    LOG_ERRORS          = 0x0002,
    ONE_FILE            = 0x0004,
    NO_OUTPUT           = 0x0008,
    SKIP_WRITE_TO_FILES = 0x0010,
    OVERWRITE_MERGE_FILE= 0x0020
  };

  // load stmt
  NABasicPtr   uldQuery_;                               // 00 - 07
  NABasicPtr   mergePath_;                              // 08 - 15
  NABasicPtr   extractLocation_;                        // 16 - 23
  UInt32       flags_;                                  // 24 - 27
  UInt8        compressType_;                           // 28 - 28
  UInt8        scanType_;                               // 29 - 29
  NABasicPtr   snapshotSuffix_;                         // 30 - 37
  char         fillersExeUtilHbaseUnLoad_[16];          // 38 - 53
};

#define STATS_NAME_MAX_LEN 256
#define STATS_REGION_NAME_MAX_LEN 512

static const ComTdbVirtTableColumnInfo comTdbRegionStatsVirtTableColumnInfo[] =
  {
    { "CATALOG_NAME",                   0, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "SCHEMA_NAME",                    1, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "OBJECT_NAME",                    2, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "REGION_SERVER",                  3, COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "REGION_NUM",                     4, COM_USER_COLUMN, REC_BIN64_SIGNED,  4, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "REGION_NAME",                    5, COM_USER_COLUMN, REC_BYTE_F_ASCII,  512, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "NUM_STORES",                     6, COM_USER_COLUMN, REC_BIN32_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "NUM_STORE_FILES",                7, COM_USER_COLUMN, REC_BIN32_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "STORE_FILE_UNCOMP_SIZE",         8, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "STORE_FILE_SIZE",                9, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "MEM_STORE_SIZE",                10, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "READ_REQUESTS_COUNT",           11, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "WRITE_REQUESTS_COUNT",          12, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  }
  };

struct ComTdbRegionStatsVirtTableColumnStruct
{
  char   catalogName[STATS_NAME_MAX_LEN];
  char   schemaName[STATS_NAME_MAX_LEN];
  char   objectName[STATS_NAME_MAX_LEN];
  char   regionServer[STATS_NAME_MAX_LEN];
  Int64  regionNum;
  char   regionName[STATS_REGION_NAME_MAX_LEN];
  Lng32  numStores;
  Lng32  numStoreFiles;
  Int64  storeFileUncompSize;
  Int64  storeFileSize;
  Int64  memStoreSize;
  Int64  readRequestsCount;
  Int64  writeRequestsCount;
};

static const ComTdbVirtTableColumnInfo comTdbClusterStatsVirtTableColumnInfo[] =
  {
    { "REGION_SERVER",                  0, COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "REGION_NAME",                    1, COM_USER_COLUMN, REC_BYTE_F_ASCII,  512, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "CATALOG_NAME",                   1, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "SCHEMA_NAME",                    2, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "OBJECT_NAME",                    3, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "NUM_STORES",                     4, COM_USER_COLUMN, REC_BIN32_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "NUM_STORE_FILES",                5, COM_USER_COLUMN, REC_BIN32_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "STORE_FILE_UNCOMP_SIZE",         6, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "STORE_FILE_SIZE",                7, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "MEM_STORE_SIZE",                 8, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "READ_REQUESTS_COUNT",            9, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "WRITE_REQUESTS_COUNT",          10, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  }
  };

struct ComTdbClusterStatsVirtTableColumnStruct
{
  char   regionServer[STATS_NAME_MAX_LEN];
  char   regionName[STATS_REGION_NAME_MAX_LEN];
  char   catalogName[STATS_NAME_MAX_LEN];
  char   schemaName[STATS_NAME_MAX_LEN];
  char   objectName[STATS_NAME_MAX_LEN];
  Lng32  numStores;
  Lng32  numStoreFiles;
  Int64  storeFileUncompSize;
  Int64  storeFileSize;
  Int64  memStoreSize;
  Int64  readRequestsCount;
  Int64  writeRequestsCount;
};


class ComTdbExeUtilRegionStats : public ComTdbExeUtil
{
  friend class ExExeUtilRegionStatsTcb;
  friend class ExExeUtilRegionStatsPrivateState;

public:
  ComTdbExeUtilRegionStats()
       : ComTdbExeUtil()
  {}
  
  ComTdbExeUtilRegionStats(
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
       ULng32 buffer_size
       );
  
  void setIsIndex(NABoolean v)
  {(v ? flags_ |= IS_INDEX : flags_ &= ~IS_INDEX); };
  NABoolean isIndex() { return (flags_ & IS_INDEX) != 0; };

  void setDisplayFormat(NABoolean v)
  {(v ? flags_ |= DISPLAY_FORMAT : flags_ &= ~DISPLAY_FORMAT); };
  NABoolean displayFormat() { return (flags_ & DISPLAY_FORMAT) != 0; };

  void setSummaryOnly(NABoolean v)
  {(v ? flags_ |= SUMMARY_ONLY : flags_ &= ~SUMMARY_ONLY); };
  NABoolean summaryOnly() { return (flags_ & SUMMARY_ONLY) != 0; };

  void setClusterView(NABoolean v)
  {(v ? flags_ |= CLUSTER_VIEW : flags_ &= ~CLUSTER_VIEW); };
  NABoolean clusterView() { return (flags_ & CLUSTER_VIEW) != 0; };
  const NABoolean clusterView() const { return (flags_ & CLUSTER_VIEW) != 0; };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilRegionStats);}

  virtual const char *getNodeName() const 
  { 
    return (clusterView() ? "GET_CLUSTER_STATS" : "GET_REGION_STATS");
  };

  int getVirtTableNumCols()
  {
    return 
      (clusterView() 
       ? sizeof(comTdbClusterStatsVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo)
       : sizeof(comTdbRegionStatsVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo));
  }

  ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)
      (clusterView() 
       ? comTdbClusterStatsVirtTableColumnInfo 
       : comTdbRegionStatsVirtTableColumnInfo);
  }

  int getVirtTableNumKeys()
  {
    return 0;
  }

  ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return NULL;
  }

private:
  enum
  {
    IS_INDEX       = 0x0001,
    DISPLAY_FORMAT = 0x0002,
    SUMMARY_ONLY   = 0x0004,
    CLUSTER_VIEW   = 0x0008
  };

  UInt32 flags_;                                     // 00-03

  char fillersComTdbExeUtilRegionStats_[76];      // 04-79
};



// Lob info virtual table info
static const ComTdbVirtTableColumnInfo comTdbLobInfoVirtTableColumnInfo[] =
  {
    { "CATALOG_NAME",                   0, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "SCHEMA_NAME",                    1, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "OBJECT_NAME",                    2, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "COLUMN_NAME",                     3, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "LOB_LOCATION",                    4, COM_USER_COLUMN, REC_BYTE_F_ASCII,  256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "LOB_DATA_FILE",                     5, COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "LOB_DATA_FILE_SIZE_EOD",         6, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  },
    { "LOB_DATA_FILE_SIZE_USED",                7, COM_USER_COLUMN, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0  }
    
  };

#define LOBINFO_MAX_FILE_LEN 256
struct ComTdbLobInfoVirtTableColumnStruct
{
  char   catalogName[LOBINFO_MAX_FILE_LEN];
  char   schemaName[LOBINFO_MAX_FILE_LEN];
  char   objectName[LOBINFO_MAX_FILE_LEN];
  char   columnName[LOBINFO_MAX_FILE_LEN];
  char   lobLocation[LOBINFO_MAX_FILE_LEN];
  char   lobDataFile[LOBINFO_MAX_FILE_LEN];
  Int64  lobDataFileSizeEod;
  Int64  lobDataFileSizeUsed;
};


class ComTdbExeUtilLobInfo : public ComTdbExeUtil
{
  friend class ExExeUtilLobInfoTcb;
  friend class ExExeUtilLobInfoTableTcb;
  friend class ExExeUtilLobInfoPrivateState;

public:
  ComTdbExeUtilLobInfo()
       : ComTdbExeUtil()
  {}
  Lng32 getNumLobs() { return numLOBs_;}
  char *getLobColList(){return lobColArray_;}
  char *getLobNumList() {return lobNumArray_;}
  char *getLobLocList() {return lobLocArray_;}
  char *getLobTypeList(){return lobTypeArray_;}
  Int64 getObjectUID() { return objectUID_;}
  Int32 getHdfsPort() { return hdfsPort_;}
  char *getHdfsServer() {return hdfsServer_;}
  NABoolean isTableFormat() { return tableFormat_;}
  ComTdbExeUtilLobInfo(
       char * tableName,
       Int64 objectUID,
       Lng32 numLOBs,
       char* lobColArray,
       char* lobNumArray,
       char* lobLocArray,
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
       ULng32 buffer_size
       );
  
  

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilRegionStats);}

  virtual const char *getNodeName() const 
  { 
    return "GET_LOB_INFO";
  };

  static int getVirtTableNumCols()
  {
    return sizeof(comTdbLobInfoVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)comTdbLobInfoVirtTableColumnInfo;
  }

  static int getVirtTableNumKeys()
  {
    return 0;
  }

  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return NULL;
  }
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);
private:
  UInt32 flags_;   
  UInt16 numLOBs_;                                   
  
  //array fo strings - column names 
  NABasicPtr lobColArray_;
  // array of shorts. numLOBs entries. 
  // Each entry is the lobNum.
  NABasicPtr lobNumArray_;  
  //array of int32. numLOBS entries
  NABasicPtr lobTypeArray_;                         

  // array of string, null terminated. numLOBs entries. 
  // Each entry is the storage location of lob data file.
  NABasicPtr lobLocArray_;                           
  Int32 hdfsPort_;
  NABasicPtr hdfsServer_;
  Int64 objectUID_;                                                      

  NABoolean tableFormat_;
  
};
#endif



