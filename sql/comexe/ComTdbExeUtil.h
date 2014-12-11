/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2005-2014 Hewlett-Packard Development Company, L.P.
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
    LOAD_                    = 0,
    REORG_                   = 1,
    DISPLAY_EXPLAIN_         = 2,
    MAINTAIN_OBJECT_         = 3,
    LOAD_VOLATILE_TABLE_     = 4,
    CLEANUP_VOLATILE_SCHEMA_ = 5,
    GET_VOLATILE_INFO        = 6,
    CREATE_TABLE_AS_         = 7,
    FAST_DELETE_             = 8,
    GET_MAINTAIN_INFO_       = 9,
    GET_STATISTICS_          = 10,
    USER_LOAD_               = 11,
    LONG_RUNNING_            = 13,
    GET_METADATA_INFO_       = 14,
    GET_VERSION_INFO_        = 15,
    SUSPEND_ACTIVATE_        = 16,
    SHOW_SET_                = 19,
    AQR_                     = 20,
    DISPLAY_EXPLAIN_COMPLEX_ = 21,
    GET_UID_                 = 22,
    POP_IN_MEM_STATS_        = 23,
    REPLICATE_               = 24,
    GET_ERROR_INFO_          = 26,
    LOB_EXTRACT_             = 27,
    LOB_SHOWDDL_             = 28,
    GET_HIVE_METADATA_INFO_  = 29,
    HIVE_MD_ACCESS_          = 30,
    AQR_WNR_INSERT_          = 31,
    UPGRADE_MD_              = 32,
    HBASE_LOAD_              = 33,
    HBASE_UNLOAD_            = 34,
    HBASE_UNLOAD_TASK_       = 35
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
    if (type_ == LOAD_)
      return "LOAD_UTIL";
    else if (type_ == CREATE_TABLE_AS_)
      return "CREATE_TABLE_AS";
    else
      return "EX_EXE_UTIL";
  };

NA_EIDPROC
  virtual Int32 numExpressions() const
    {
      return (ComTdbGenericUtil::numExpressions() + 1);
    }

NA_EIDPROC
  virtual ex_expr* getExpressionNode(Int32 pos) {
    if (pos >= numExpressions())
      return NULL;
    else
      if (pos < ComTdbGenericUtil::numExpressions())
	return ComTdbGenericUtil::getExpressionNode(pos);
      else
	return scanExpr_;
  }

NA_EIDPROC
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
#pragma warn(1506)  // warning elimination


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
			      char optionX, // explain format desired
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

  void setOptionX(char c);              // move from char to mask_

  NABoolean isOptionE() { return ((flags_ & OPTION_E) != 0); };
  NABoolean isOptionF() { return ((flags_ & OPTION_F) != 0); };
  NABoolean isOptionM() { return ((flags_ & OPTION_M) != 0); };
  NABoolean isOptionN() { return ((flags_ & OPTION_N) != 0); };

private:
  enum OpToFlag
  {
    OPTION_F      = 0x0001,
    OPTION_E      = 0x0002,
    OPTION_M      = 0x0004,
    OPTION_N      = 0x0008,
    OPTION_OFF    = 0xfff0
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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

class ComTdbExeUtilReorg : public ComTdbExeUtil
{
  friend class ExExeUtilReorgTcb;
  friend class ExExeUtilReorgCheckTcb;
  friend class ExExeUtilReorgCheckSchemaTcb;
  friend class ExExeUtilReorgCheckCatalogTcb;
  friend class ExExeUtilReorgCheckReturnSummaryTcb;
  friend class ExExeUtilReorgCheckGenerateCheckCommandsTcb;
  friend class ExExeUtilReorgCheckGenerateMaintainTcb;
  friend class ExExeUtilReorgCheckRunMaintainTcb;
  friend class ExExeUtilReorgCheckAllTcb;
  friend class ExExeUtilReorgInitializeTcb;
  friend class ExExeUtilReorgPrivateState;

public:
  enum ObjectType
  {
    TABLE_  = 0,
    INDEX_  = 1,
    MV_     = 2,
    SCHEMA_ = 3,
    CATALOG_= 4
  };

  enum ReorgStatus
  {
    INITIAL_ = 0,
    STARTED_,
    REORG_NEEDED_,
    REORG_NOT_NEEDED_,
    COMPLETED_,
    NOT_AVAILABLE_,
    SUSPENDED_
  };

  ComTdbExeUtilReorg()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilReorg(
                     char *catalogName,
                     ULng32 catalogNameLen,
                     char * tableName,
		     ULng32 tableNameLen,
		     Int64 objectUID,
		     ObjectType ot,
                     Lng32 numTables,
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

  void setParams
    (Lng32 concurrency,
     Lng32 firstPartn, Lng32 lastPartn,
     Lng32 rate,
     Lng32 dslack, Lng32 islack,
     Lng32 delay,
     Lng32 priority,
     NABoolean noDealloc,
     char * partnName, UInt32 partnNameLen,
     NABoolean displayOnly,
     NABoolean noOutputMsg,
     NABoolean returnSummary,
     NABoolean returnDetailOutput,
     NABoolean getStatus,
     NABoolean suspend,
     NABoolean stop,
     NABoolean newReorg,
     NABoolean doCheck,
     NABoolean doReorg,
     NABoolean reorgIfNeeded,
     NABoolean doCompact,
     NABoolean doOverride,
     Lng32 compressionType,
     NABoolean updateReorgDB,
     NABoolean useReorgDB,
     NABoolean returnFast,
     Lng32 rowsetSize,
     Lng32 firstTable,
     Lng32 lastTable,
     char * whereClauseStr,
     NABoolean generateMaintainCommands,
     NABoolean showMaintainCommands,
     NABoolean runMaintainCommands,
     Lng32 maxMaintainTables,
     NABoolean reorgCheckAll,
     NABoolean generateCheckCommands,
     Lng32 concurrentCheckSessions,
     NABoolean continueOnError,
     NABoolean systemObjectsOnly,
     NABoolean debugOutput,
     NABoolean initialize,
     NABoolean reinitialize,
     NABoolean drop,
     NABoolean createView,
     NABoolean dropView);
  

  void setLists(Queue* reorgPartnsSegmentNameList,
		Queue* reorgPartnsNameList,
		Queue* reorgPartnsGuaNameList,
                Queue* reorgPartnsCpuList,
                Queue* reorgPartnsDp2List,
		Queue* reorgPartnsTableNumList,
		Queue* reorgTableNamesList,
		Queue* reorgTableObjectUIDList);

  Queue* getReorgPartnsSegmentNameList() {return reorgPartnsSegmentNameList_;}
  Queue* getReorgPartnsNameList()        {return reorgPartnsNameList_; }
  Queue* getReorgPartnsGuaNameList()     {return reorgPartnsGuaNameList_; }
  Queue* getReorgPartnsCpuList()         {return reorgPartnsCpuList_; }
  Queue* getReorgPartnsDp2List()         {return reorgPartnsDp2List_; }
  Queue* getReorgPartnsTableNumList()    {return reorgPartnsTableNumList_; }
  Queue* getReorgTableNamesList()        {return reorgTableNamesList_; }
  Queue* getReorgTableObjectUIDList()    {return reorgTableObjectUIDList_; }

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilReorg);}

  virtual const char *getNodeName() const
  {
    return "REORG";
  };

  NABoolean isIndex() { return (ot_ == INDEX_); };
  NABoolean isTable() { return (ot_ == TABLE_); };
  NABoolean isMv() { return (ot_ == MV_); };
  NABoolean isSchema() { return (ot_ == SCHEMA_); };
  NABoolean isCatalog() { return (ot_ == CATALOG_); };
    
  void setDisplayOnly(NABoolean v)
  {(v ? flags_ |= DISPLAY_ONLY : flags_ &= ~DISPLAY_ONLY); };
  NABoolean displayOnly() { return (flags_ & DISPLAY_ONLY) != 0; };

  void setNoOutputMsg(NABoolean v)
  {(v ? flags_ |= NO_OUTPUT_MSG : flags_ &= ~NO_OUTPUT_MSG); };
  NABoolean noOutputMsg() { return (flags_ & NO_OUTPUT_MSG) != 0; };

  void setReturnSummary(NABoolean v)
  {(v ? flags_ |= RETURN_SUMMARY : flags_ &= ~RETURN_SUMMARY); };
  NABoolean returnSummary() { return (flags_ & RETURN_SUMMARY) != 0; };

  void setReturnDetailOutput(NABoolean v)
  {(v ? flags_ |= RETURN_DETAIL_OUTPUT : flags_ &= ~RETURN_DETAIL_OUTPUT); };
  NABoolean returnDetailOutput() { return (flags_ & RETURN_DETAIL_OUTPUT) != 0; };

  void setGetStatus(NABoolean v)
  {(v ? flags_ |= GET_STATUS : flags_ &= ~GET_STATUS); };
  NABoolean getStatus() { return (flags_ & GET_STATUS) != 0; };

  void setSuspend(NABoolean v)
  {(v ? flags_ |= SUSPEND : flags_ &= ~SUSPEND); };
  NABoolean getSuspend() { return (flags_ & SUSPEND) != 0; };

  void setStop(NABoolean v)
  {(v ? flags_ |= STOP : flags_ &= ~STOP); };
  NABoolean getStop() { return (flags_ & STOP) != 0; };

  void setNewReorg(NABoolean v)
  {(v ? flags_ |= NEW_REORG : flags_ &= ~NEW_REORG); };
  NABoolean getNewReorg() { return (flags_ & NEW_REORG) != 0; };

  void setDoCheck(NABoolean v)
  {(v ? flags_ |= DO_CHECK : flags_ &= ~DO_CHECK); };
  NABoolean getDoCheck() { return (flags_ & DO_CHECK) != 0; };

  void setDoReorg(NABoolean v)
  {(v ? flags_ |= DO_REORG : flags_ &= ~DO_REORG); };
  NABoolean getDoReorg() { return (flags_ & DO_REORG) != 0; };

  void setNoDealloc(NABoolean v)
  {(v ? flags_ |= NO_DEALLOC : flags_ &= ~NO_DEALLOC); };
  NABoolean noDealloc() { return (flags_ & NO_DEALLOC) != 0; };

  void setMultiReorg(NABoolean v)
  {(v ? flags_ |= MULTI_REORG : flags_ &= ~MULTI_REORG); };
  NABoolean getMultiReorg() { return (flags_ & MULTI_REORG) != 0; };

  void setDoCompact(NABoolean v)
    {(v ? flags_ |= DO_COMPACT : flags_ &= ~DO_COMPACT); };
  NABoolean getDoCompact() { return (flags_ & DO_COMPACT) != 0; };

  void setDoOverride(NABoolean v)
    {(v ? flags_ |= DO_OVERRIDE : flags_ &= ~DO_OVERRIDE); };
  NABoolean getDoOverride() { return (flags_ & DO_OVERRIDE) != 0; };

  void setUpdateReorgDB(NABoolean v)
    {(v ? flags_ |= UPDATE_REORG_DB : flags_ &= ~UPDATE_REORG_DB); };
  NABoolean getUpdateReorgDB() { return (flags_ & UPDATE_REORG_DB) != 0; };

  void setUseReorgDB(NABoolean v)
    {(v ? flags_ |= USE_REORG_DB : flags_ &= ~USE_REORG_DB); };
  NABoolean getUseReorgDB() { return (flags_ & USE_REORG_DB) != 0; };

  void setReturnFast(NABoolean v)
    {(v ? flags_ |= RETURN_FAST : flags_ &= ~RETURN_FAST); };
  NABoolean getReturnFast() { return (flags_ & RETURN_FAST) != 0; };

  void setGenerateMaintainCommands(NABoolean v)
    {(v ? flags_ |= GENERATE_MAINTAIN_COMMANDS : flags_ &= ~GENERATE_MAINTAIN_COMMANDS); };
  NABoolean getGenerateMaintainCommands() { return (flags_ & GENERATE_MAINTAIN_COMMANDS) != 0; };

  void setShowMaintainCommands(NABoolean v)
    {(v ? flags_ |= SHOW_MAINTAIN_COMMANDS : flags_ &= ~SHOW_MAINTAIN_COMMANDS); };
  NABoolean getShowMaintainCommands() { return (flags_ & SHOW_MAINTAIN_COMMANDS) != 0; };

  void setRunMaintainCommands(NABoolean v)
    {(v ? flags_ |= RUN_MAINTAIN_COMMANDS : flags_ &= ~RUN_MAINTAIN_COMMANDS); };
  NABoolean getRunMaintainCommands() { return (flags_ & RUN_MAINTAIN_COMMANDS) != 0; };

  void setGenerateCheckCommands(NABoolean v)
    {(v ? flags_ |= GENERATE_CHECK_COMMANDS : flags_ &= ~GENERATE_CHECK_COMMANDS); };
  NABoolean getGenerateCheckCommands() { return (flags_ & GENERATE_CHECK_COMMANDS) != 0; };

  void setContinueOnError(NABoolean v)
  {(v ? flags_ |= CONTINUE_ON_ERROR : flags_ &= ~CONTINUE_ON_ERROR); };
  NABoolean continueOnError() { return (flags_ & CONTINUE_ON_ERROR) != 0; };

  void setSystemObjectsOnly(NABoolean v)
  {(v ? flags_ |= SYSTEM_OBJECTS_ONLY : flags_ &= ~SYSTEM_OBJECTS_ONLY); };
  NABoolean systemObjectsOnly() { return (flags_ & SYSTEM_OBJECTS_ONLY) != 0; };

  void setInitializeDB(NABoolean v)
  {(v ? flags_ |= INITIALIZE_DB : flags_ &= ~INITIALIZE_DB); };
  NABoolean initializeDB() { return (flags_ & INITIALIZE_DB) != 0; };

  void setReInitializeDB(NABoolean v)
  {(v ? flags_ |= REINITIALIZE_DB : flags_ &= ~REINITIALIZE_DB); };
  NABoolean reInitializeDB() { return (flags_ & REINITIALIZE_DB) != 0; };

  void setDropDB(NABoolean v)
  {(v ? flags_ |= DROP_DB : flags_ &= ~DROP_DB); };
  NABoolean dropDB() { return (flags_ & DROP_DB) != 0; };

  void setCreateView(NABoolean v)
  {(v ? flags_ |= CREATE_VIEW : flags_ &= ~CREATE_VIEW); };
  NABoolean createView() { return (flags_ & CREATE_VIEW) != 0; };

  void setDropView(NABoolean v)
  {(v ? flags_ |= DROP_VIEW : flags_ &= ~DROP_VIEW); };
  NABoolean dropView() { return (flags_ & DROP_VIEW) != 0; };

  void setReorgIfNeeded(NABoolean v)
  {(v ? flags_ |= REORG_IF_NEEDED : flags_ &= ~REORG_IF_NEEDED); };
  NABoolean getReorgIfNeeded() { return (flags_ & REORG_IF_NEEDED) != 0; };

  void setReorgCheckAll(NABoolean v)
  {(v ? flags_ |= REORG_CHECK_ALL : flags_ &= ~REORG_CHECK_ALL); };
  NABoolean getReorgCheckAll() { return (flags_ & REORG_CHECK_ALL) != 0; };

  void setCleanupDB(NABoolean v)
  {(v ? flags_ |= CLEANUP_DB : flags_ &= ~CLEANUP_DB); };
  NABoolean getCleanupDB() { return (flags_ & CLEANUP_DB) != 0; };

  void setDebugOutput(NABoolean v)
  {(v ? flags2_ |= DEBUG_OUTPUT : flags2_ &= ~DEBUG_OUTPUT); };
  NABoolean getDebugOutput() { return (flags2_ & DEBUG_OUTPUT) != 0; };

  void setCollectReorgStats(NABoolean v)
  {(v ? flags2_ |= COLLECT_REORG_STATS : flags2_ &= ~COLLECT_REORG_STATS); };
  NABoolean getCollectReorgStats() { return (flags2_ & COLLECT_REORG_STATS) != 0; };
  
  void setVerify(NABoolean v)
  {(v ? flags2_ |= REORG_VERIFY : flags2_ &= ~REORG_VERIFY); };
  NABoolean getVerify() { return (flags2_ & REORG_VERIFY) != 0; };

  ObjectType getObjectType() { return (ObjectType)ot_; };

  Int64 &statusAfterTS() { return statusAfterTS_; }
  Lng32  &statusSummaryOptions() { return statusSummaryOptions_; }

  Int64 &cleanupToTS() { return cleanupToTS_; }

  Lng32 getNumTables() { return numTables_; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

private:
  enum
    {
      LOAD_INDEX    = 0x0001,
      DISPLAY_ONLY  = 0x0002,
      NO_OUTPUT_MSG = 0x0004,
      GET_STATUS    = 0x0008,
      SUSPEND       = 0x0010,
      NEW_REORG     = 0x0020,
      DO_CHECK      = 0x0040,
      DO_REORG      = 0x0080,
      NO_DEALLOC    = 0x0100,
      MULTI_REORG   = 0x0200,
      RETURN_SUMMARY= 0x0400,
      RETURN_DETAIL_OUTPUT = 0x0800,
      DO_COMPACT    = 0x1000,
      DO_OVERRIDE   = 0x2000,
      STOP          = 0x4000,
      UPDATE_REORG_DB = 0x8000,
      USE_REORG_DB    = 0x10000,
      GENERATE_MAINTAIN_COMMANDS = 0x20000,
      CONTINUE_ON_ERROR = 0x40000,
      SYSTEM_OBJECTS_ONLY  = 0x80000,
      INITIALIZE_DB       = 0x100000,
      REINITIALIZE_DB     = 0x200000,
      DROP_DB             = 0x400000,
      CREATE_VIEW         = 0x800000,
      DROP_VIEW               = 0x1000000,
      GENERATE_CHECK_COMMANDS = 0x2000000,
      REORG_IF_NEEDED         = 0x4000000,
      RETURN_FAST             = 0x8000000,
      REORG_CHECK_ALL         = 0x10000000,
      CLEANUP_DB              = 0x20000000,
      RUN_MAINTAIN_COMMANDS   = 0x40000000,
      SHOW_MAINTAIN_COMMANDS  = 0x80000000
    };
  
  enum
    {
      DEBUG_OUTPUT                   = 0x0001,
      COLLECT_REORG_STATS            = 0x0002,
      REORG_VERIFY                   = 0x0004
    };
  
  enum
  {
    STATUS_SUMMARY_SHORT  = 0x0001,
    STATUS_SUMMARY_TABLE  = 0x0002,
    STATUS_SUMMARY_ERROR  = 0x0004,
    STATUS_SUMMARY_DETAIL = 0x0008
  };

  // these summary enums are used with REORG CHECK command
  enum
  {
    CHECK_SUMMARY_SHORT     = 0x0001,
    CHECK_SUMMARY_TOKENIZED= 0x0002, 
    CHECK_SUMMARY_DETAIL    = 0x0004,
    CHECK_SUMMARY_IF_NEEDED = 0x0008 
  };


  Int64 getReorgTableObjectUID() { return reorgTableObjectUID_; }

  char * getPartnName() { return partnName_; }
  char * getCatalogName() 
    {
      return catName_;
    }

  char * getWhereClauseStr()
    {
      return whereClauseStr_;
    }

  ObjectType ot_;          // 00-03
  Lng32 concurrency_;      // 04-07
  Lng32 firstPartn_;       // 08-11
  Lng32 lastPartn_;        // 12-15
  Lng32 rate_;             // 16-19
  Lng32 dslack_;           // 20-23
  Lng32 islack_;           // 24-27
  Lng32 delay_;            // 28-31
  Lng32 priority_;         // 32-35
  UInt32 flags_;           // 36-39

  NABasicPtr partnName_;   // 40-47
  UInt32 partnNameLen_;    // 48-51

  UInt32 flags2_;                                         // 52-55

  QueuePtr reorgPartnsSegmentNameList_;                   // 56-63

  // partition name as given by user
  QueuePtr reorgPartnsNameList_;                          // 64-71

  // guardian name of the partitions
  QueuePtr reorgPartnsGuaNameList_;                       // 72-79

  // partition cpu number
  QueuePtr reorgPartnsCpuList_;                           // 80-87

  // partition dp2 name
  QueuePtr reorgPartnsDp2List_;                           // 88-95

  // tablenum corresponding to this partition.
  // Used when multiple tables are being reorged.
  QueuePtr reorgPartnsTableNumList_;                      // 96-103

  // names of tables being reorged. Valid when MULTI_REORG is set.
  // Used when multiple tables are being reorged.
  QueuePtr reorgTableNamesList_;                          // 104-111

  Int64 statusAfterTS_;                                   // 112-119

  Lng32  statusSummaryOptions_;                            // 120-123

  Int32 numTables_;                                       // 124-127

  Lng32 compressionType_;                                 // 136-139

  Lng32 maxMaintainTables_;                               // 140-143
   
  NABasicPtr catName_;                                    // 144-151
  UInt32 catNameLen_;                                     // 152-155

  Int32 rowsetSize_;                                      // 156-159

  Lng32 firstTable_;                                      // 160-163
  Lng32 lastTable_;                                       // 164-167
  
  Lng32 concurrentCheckSessions_;                         // 168-171
  
  char  filler1_[4];                                      // 172-175
  NABasicPtr whereClauseStr_;                             // 176-183
   
  Int64 cleanupToTS_;                                     // 184-191

  // UID of table being reorged. Valid when single table is being reorged.
  Int64    reorgTableObjectUID_;                          // 192-199

  // UIDs of tables being reorged. Valid when MULTI_REORG is set.
  // Used when multiple tables are being reorged.
  QueuePtr reorgTableObjectUIDList_;                      // 200-207

  char fillersComTdbExeUtilReorg_[32];                    // 208-239
};

class ComTdbExeUtilReplicate : public ComTdbExeUtil
{

public:
  enum ObjectType
  {
    UNKNOWN_ = -1,
    TABLE_ = 0,
    INDEX_ = 1,
    MV_    = 2,
    SCHEMA_ = 3,
    SCHEMA_DDL_ = 4,
    SCHEMA_STATS_ = 5,
    OBJECT_DDL_ = 6,
    REPL_RECOVER_ = 7,
    REPL_STATUS_ = 8,
    REPL_ABORT_ = 9,
    REPL_INITIALIZE_ = 10, 
    REPL_AUTHORIZATION_ = 11
  };

  enum ReplType
  {
    UNKNOWN_REPLTYPE_ = -1,
    FULL_ = 0,
    INCREMENTAL_ = 1
  };

  enum FormatFlags
  {
    SHORT_   = 0x0001,
    MEDIUM_  = 0x0002,
    LONG_    = 0x0004
  };

  ComTdbExeUtilReplicate()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilReplicate(char * sourceName,
			 UInt32 sourceNameLen,
			 char * targetName,
			 UInt32 targetNameLen,
			 char * sourceSchema,
			 UInt32 sourceSchemaLen,
			 char * targetSystem,
			 UInt32 targetSystemLen,
			 char * targetSchema,
			 UInt32 targetSchemaLen,
			 char * ddlInputStr,
			 UInt32 ddlInputStrLen,
			 char * schGetTablesStmt,
			 UInt32 schGetTablesStmtLen,
			 char * schTableReplStmt,
			 UInt32 schTableReplStmtLen,
			 char * indexReplStmt,
			 UInt32 indexReplStmtLen,
			 char * uniqImplIndexReplStmt,
			 UInt32 uniqImplIndexReplStmtLen,
			 char * controlQueryId,
			 UInt32 controlQueryIdLen,
			 char * srcCatName,
			 char * tgtCatName,
			 ObjectType ot,
			 ex_expr * input_expr,
			 UInt32 input_rowlen,
			 ex_expr * output_expr,
			 UInt32 output_rowlen,
			 ex_cri_desc * work_cri_desc,
			 const unsigned short work_atp_index,
			 ex_cri_desc * given_cri_desc,
			 ex_cri_desc * returned_cri_desc,
			 queue_index down,
			 queue_index up,
			 Lng32 num_buffers,
			 UInt32 buffer_size
			 );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void setParams(Lng32 concurrency,
		 Lng32 rate,
		 Lng32 priority,
		 Lng32 delay,
		 Int64 srcObjectUid,
		 NABoolean compress,
		 NABoolean purgedataTgt,
		 NABoolean incremental,
		 NABoolean validateTgtDDL,
		 NABoolean createTgtDDL,
		 NABoolean validateData,
		 NABoolean tgtObjsOnline,
		 NABoolean tgtObjsOffline,
		 NABoolean tgtObjsAudited,
		 NABoolean tgtObjsUnaudited,
		 NABoolean returnPartnDetails,
		 NABoolean transform,
		 NABoolean getStatus,
		 NABoolean suspend,
		 NABoolean resume,
		 NABoolean stop,
		 NABoolean authid,
		 NABoolean copyBothDataAndStats,
		 NABoolean displayInternalCmd);

  void setLists(Queue* replIndexList,
		Queue* replIndexTypeList,
		Queue* replPartnsSegmentNameList,
		Queue* replPartnsNameList,
		Queue* replPartnsGuaNameList,
                Queue* replPartnsCpuList,
                Queue* replPartnsDp2List);

  Queue* getReplIndexList()             {return replIndexList_;}
  Queue* getReplIndexTypeList()         {return replIndexTypeList_;}
  Queue* getReplPartnsSegmentNameList() {return replPartnsSegmentNameList_;}
  Queue* getReplPartnsNameList()        {return replPartnsNameList_; }
  Queue* getReplPartnsGuaNameList()     {return replPartnsGuaNameList_; }
  Queue* getReplPartnsCpuList()         {return replPartnsCpuList_; }
  Queue* getReplPartnsDp2List()         {return replPartnsDp2List_; }

  char * getMgbltyCatalogName()  { return mgbltyCatalogName_; }
  void setMgbltyCatalogName(char * catalog) { mgbltyCatalogName_ = catalog; }

  char * getTestTgtMgbltyCatalogName() { return testTgtMgbltyCatalogName_; }
  void setTestTgtMgbltyCatalogName(char * catalog) { testTgtMgbltyCatalogName_ = catalog; }

  void setRoleName(char * role) { roleName_ = role; }

  void setNumSrcPartns(Lng32 srcP) { numSrcPartns_ = srcP; }

  char * sourceName()     { return sourceName_; }
  char * targetName()     { return targetName_; }
  char * targetSystem()   { return targetSystem_; }
  char * srcCatName()     { return srcCatName_; }
  char * tgtCatName()     { return tgtCatName_; }
  char * indexReplStmt()  { return indexReplStmt_; }
  char * uniqImplIndexReplStmt()  { return uniqImplIndexReplStmt_; }
  char * controlQueryId() { return controlQueryId_; }
  char * ddlInputStr()    { return ddlInputStr_; }
  char * ipAddr()         { return ipAddr_; }
  char * sourceSchema()   { return sourceSchema_; }
  char * targetSchema()   { return targetSchema_; }
  char * schTableReplStmt() { return schTableReplStmt_; }
  char * roleName()       { return roleName_;}

  Int32 portNum() { return portNum_; }

  Int64 srcObjectUid() { return srcObjectUid_; }

  void setIpAddr(char * ipAddr) { ipAddr_ = ipAddr; }
  void setPortNum(Int32 portNum) { portNum_ = portNum; }

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilReplicate);}

  virtual const char *getNodeName() const
  {
    return "REPLICATE";
  };

  NABoolean isIndex() { return (ot_ == INDEX_); };
  NABoolean isTable() { return (ot_ == TABLE_); };
  NABoolean isMv() { return (ot_ == MV_); };

  void setCompress(NABoolean v)
  {(v ? flags_ |= COMPRESS : flags_ &= ~COMPRESS); };
  NABoolean compress() { return (flags_ & COMPRESS) != 0; };

  void setGetStatus(NABoolean v)
  {(v ? flags_ |= GET_STATUS : flags_ &= ~GET_STATUS); };
  NABoolean getStatus() { return (flags_ & GET_STATUS) != 0; };

  void setSuspend(NABoolean v)
  {(v ? flags_ |= SUSPEND : flags_ &= ~SUSPEND); };
  NABoolean getSuspend() { return (flags_ & SUSPEND) != 0; };

  void setResume(NABoolean v)
  {(v ? flags_ |= RESUME : flags_ &= ~RESUME); };
  NABoolean getResume() { return (flags_ & RESUME) != 0; };

  void setAbort(NABoolean v)
  {(v ? flags_ |= ABORT : flags_ &= ~ABORT); };
  NABoolean getAbort() { return (flags_ & ABORT) != 0; };

  void setValidateTgtDDL(NABoolean v)
  {(v ? flags_ |= VALIDATE_TGT_DDL : flags_ &= ~VALIDATE_TGT_DDL); };
  NABoolean validateTgtDDL() { return (flags_ & VALIDATE_TGT_DDL) != 0; };

  void setValidateData(NABoolean v)
  {(v ? flags_ |= VALIDATE_DATA : flags_ &= ~VALIDATE_DATA); };
  NABoolean validateData() { return (flags_ & VALIDATE_DATA) != 0; };

  void setCreateTgtDDL(NABoolean v)
  {(v ? flags_ |= CREATE_TGT_DDL : flags_ &= ~CREATE_TGT_DDL); };
  NABoolean createTgtDDL() { return (flags_ & CREATE_TGT_DDL) != 0; };

  void setSchemaRepl(NABoolean v)
  {(v ? flags_ |= SCHEMA_REPL : flags_ &= ~SCHEMA_REPL); };
  NABoolean schemaRepl() { return (flags_ & SCHEMA_REPL) != 0; };

  void setInListRepl(NABoolean v)
  {(v ? flags_ |= IN_LIST_REPL : flags_ &= ~IN_LIST_REPL); };
  NABoolean inListRepl() { return (flags_ & IN_LIST_REPL) != 0; };

  void setPurgedataTgt(NABoolean v)
  {(v ? flags_ |= PURGEDATA_TGT : flags_ &= ~PURGEDATA_TGT); };
  NABoolean purgedataTgt() { return (flags_ & PURGEDATA_TGT) != 0; };

  void setTgtActions(NABoolean v)
  {(v ? flags_ |= TGT_ACTIONS : flags_ &= ~TGT_ACTIONS); };
  NABoolean tgtActions() { return (flags_ & TGT_ACTIONS) != 0; };

  void setAlterTgtOnline(NABoolean v)
  {(v ? flags_ |= ALTER_TGT_ONLINE : flags_ &= ~ALTER_TGT_ONLINE); };
  NABoolean alterTgtOnline() { return (flags_ & ALTER_TGT_ONLINE) != 0; };

  void setAlterTgtOffline(NABoolean v)
  {(v ? flags_ |= ALTER_TGT_OFFLINE : flags_ &= ~ALTER_TGT_OFFLINE); };
  NABoolean alterTgtOffline() { return (flags_ & ALTER_TGT_OFFLINE) != 0; };

  void setAlterTgtAudited(NABoolean v)
  {(v ? flags_ |= ALTER_TGT_AUDITED : flags_ &= ~ALTER_TGT_AUDITED); };
  NABoolean alterTgtAudited() { return (flags_ & ALTER_TGT_AUDITED) != 0; };

  void setAlterTgtUnaudited(NABoolean v)
  {(v ? flags_ |= ALTER_TGT_UNAUDITED : flags_ &= ~ALTER_TGT_UNAUDITED); };
  NABoolean alterTgtUnaudited() { return (flags_ & ALTER_TGT_UNAUDITED) != 0; };

  void setReturnPartnDetails(NABoolean v)
  {(v ? flags_ |= RETURN_PARTN_DETAILS : flags_ &= ~RETURN_PARTN_DETAILS); };
  NABoolean returnPartnDetails() { return (flags_ & RETURN_PARTN_DETAILS) != 0; };

  void setTransform(NABoolean v)
  {(v ? flags_ |= TRANSFORM : flags_ &= ~TRANSFORM); };
  NABoolean transform() { return (flags_ & TRANSFORM) != 0; };

  void setDebugTarget(NABoolean v)
  {(v ? flags_ |= DEBUG_TARGET : flags_ &= ~DEBUG_TARGET); };
  NABoolean debugTarget() { return (flags_ & DEBUG_TARGET) != 0; };

  void setRecoverSource(NABoolean v)
  {(v ? flags_ |= RECOVER_SOURCE : flags_ &= ~RECOVER_SOURCE); };
  NABoolean recoverSource() { return (flags_ & RECOVER_SOURCE) != 0; };

  void setRecoverTarget(NABoolean v)
  {(v ? flags_ |= RECOVER_TARGET : flags_ &= ~RECOVER_TARGET); };
  NABoolean recoverTarget() { return (flags_ & RECOVER_TARGET) != 0; };

  void setInitializeReplicate(NABoolean v)
  {(v ? flags_ |= INITIALIZE_REPLICATE : flags_ &= ~INITIALIZE_REPLICATE); };
  NABoolean initializeReplicate() { return (flags_ & INITIALIZE_REPLICATE) != 0; };

  void setReinitializeReplicate(NABoolean v)
  {(v ? flags_ |= REINITIALIZE_REPLICATE : flags_ &= ~REINITIALIZE_REPLICATE); };
  NABoolean reinitializeReplicate() { return (flags_ & REINITIALIZE_REPLICATE) != 0; };

  void setDropReplicate(NABoolean v)
  {(v ? flags_ |= DROP_REPLICATE : flags_ &= ~DROP_REPLICATE); };
  NABoolean dropReplicate() { return (flags_ & DROP_REPLICATE) != 0; };

  void setCleanup(NABoolean v)
  {(v ? flags_ |= CLEANUP : flags_ &= ~CLEANUP); };
  NABoolean getCleanup() { return (flags_ & CLEANUP) != 0; };


  void setReplicateSchemaStats(NABoolean v)
  {(v ? flags_ |= REPLICATE_SCHEMA_STATS : flags_ &= ~REPLICATE_SCHEMA_STATS); };
  NABoolean replicateSchemaStats() { return (flags_ & REPLICATE_SCHEMA_STATS) != 0; };

  void setReplicateSchemaDDL(NABoolean v)
  {(v ? flags_ |= REPLICATE_SCHEMA_DDL : flags_ &= ~REPLICATE_SCHEMA_DDL); };
  NABoolean replicateSchemaDDL() { return (flags_ & REPLICATE_SCHEMA_DDL) != 0; };

  void setTgtCreate(NABoolean v)
  {(v ? flags_ |= TGT_CREATE : flags_ &= ~TGT_CREATE); };
  NABoolean tgtCreate() { return (flags_ & TGT_CREATE) != 0; };

  void setTgtProcess(NABoolean v)
  {(v ? flags_ |= TGT_PROCESS : flags_ &= ~TGT_PROCESS); };
  NABoolean tgtProcess() { return (flags_ & TGT_PROCESS) != 0; };

  void setTgtDrop(NABoolean v)
  {(v ? flags_ |= TGT_DROP : flags_ &= ~TGT_DROP); };
  NABoolean tgtDrop() { return (flags_ & TGT_DROP) != 0; };

  void setGetDetails(NABoolean v)
  {(v ? flags_ |= GET_DETAILS : flags_ &= ~GET_DETAILS); };
  NABoolean getDetails() { return (flags_ & GET_DETAILS) != 0; };

  void setReplicateWarnings(NABoolean v)
  {(v ? flags_ |= REPLICATE_WARNINGS : flags_ &= ~REPLICATE_WARNINGS); };
  NABoolean replicateWarnings() { return (flags_ & REPLICATE_WARNINGS) != 0; };

  void setAddConfig(NABoolean v)
  {(v ? flags2_ |= ADD_CONFIG : flags2_ &= ~ADD_CONFIG); };
  NABoolean addConfig() { return (flags2_ & ADD_CONFIG) != 0; };

  void setRemoveConfig(NABoolean v)
  {(v ? flags2_ |= REMOVE_CONFIG : flags2_ &= ~REMOVE_CONFIG); };
  NABoolean removeConfig() { return (flags2_ & REMOVE_CONFIG) != 0; };

  void setDisableSchemaCreate(NABoolean v)
  {(v ? flags2_ |= DISABLE_SCHEMA_CREATE : flags2_ &= ~DISABLE_SCHEMA_CREATE); };
  NABoolean disableSchemaCreate() { return (flags2_ & DISABLE_SCHEMA_CREATE) != 0; };

  void setLockSchema(NABoolean v)
  {(v ? flags2_ |= LOCK_SCHEMA : flags2_ &= ~LOCK_SCHEMA); };
  NABoolean lockSchema() { return (flags2_ & LOCK_SCHEMA) != 0; };

  void setNoValidateRole(NABoolean v)
  {(v ? flags2_ |= NO_VALIDATE_ROLE : flags2_ &= ~NO_VALIDATE_ROLE); };
  NABoolean noValidateRole() { return (flags2_ & NO_VALIDATE_ROLE) != 0; };

  void setSQTargetType(NABoolean v)
  {(v ? flags2_ |= SQ_TARGET_TYPE : flags2_ &= ~SQ_TARGET_TYPE); };
  NABoolean isSQTargetType() { return (flags2_ & SQ_TARGET_TYPE) != 0; };

  void setIncremental(NABoolean v)
  {(v ? flags2_ |= INCREMENTAL : flags2_ &= ~INCREMENTAL); };
  NABoolean incremental() { return (flags2_ & INCREMENTAL) != 0; };

  void setReplicateAuthid(NABoolean v)
  {(v ? flags2_ |= AUTHORIZATION_REPL : flags2_ &= ~AUTHORIZATION_REPL); };
  NABoolean replicateAuthid() { return (flags2_ & AUTHORIZATION_REPL) != 0; };

  void setCopyBothDataAndStats(NABoolean v)
  {(v ? flags2_ |= COPY_BOTH_DATA_AND_STATS : flags2_ &= ~COPY_BOTH_DATA_AND_STATS); };
  NABoolean copyBothDataAndStats() { return (flags2_ & COPY_BOTH_DATA_AND_STATS) != 0; };

  void setDisplayInternalCmd(NABoolean v)
  {(v ? flags2_ |= DISPLAY_INTERNAL_CMD : flags2_ &= ~DISPLAY_INTERNAL_CMD); };
  NABoolean displayInternalCmd() { return (flags2_ & DISPLAY_INTERNAL_CMD) != 0; };

  void setIncrementalDisallowed(NABoolean v)
  {(v ? flags2_ |= INCREMENTAL_DISALLOWED : flags2_ &= ~INCREMENTAL_DISALLOWED); };
  NABoolean incrementalDisallowed() { return (flags2_ & INCREMENTAL_DISALLOWED) != 0; };

  void setValidateDDL(NABoolean v)
  {(v ? flags2_ |= VALIDATE_DDL : flags2_ &= ~VALIDATE_DDL); };
  NABoolean validateDDL() { return (flags2_ & VALIDATE_DDL) != 0; };

  void setValidateAndPurgedata(NABoolean v)
  {(v ? flags2_ |= VALIDATE_AND_PURGEDATA:  flags2_ &= ~VALIDATE_AND_PURGEDATA); };
  NABoolean validateAndPurgedata() { return (flags2_ & VALIDATE_AND_PURGEDATA) != 0; };

  void setWaitedStartup(NABoolean v)
  {(v ? flags2_ |= WAITED_STARTUP:  flags2_ &= ~WAITED_STARTUP); };
  NABoolean getWaitedStartup() { return (flags2_ & WAITED_STARTUP) != 0; };

  void setForceError(char * fe, Int32 len)
  {
    if ((len <= 0) || (len > 7))
      {
	forceError_[0] = 0;
	return;
      }

    str_cpy_all(forceError_, fe, len);
    forceError_[len] = 0;
  }
  char * getForceError()
  { return (strlen(forceError_) > 0 ? forceError_ : NULL); }

  void setNumRetries(Int32 r) { numRetries_ = r; }
  Int32 numRetries() { return numRetries_; }

  void setShortFormat(NABoolean v)
  {(v ? formatFlags_ |= SHORT_ : formatFlags_ &= ~SHORT_); };
  NABoolean shortFormat() { return (formatFlags_ & SHORT_) != 0; };

  void setMediumFormat(NABoolean v)
  {(v ? formatFlags_ |= MEDIUM_ : formatFlags_ &= ~MEDIUM_); };
  NABoolean mediumFormat() { return (formatFlags_ & MEDIUM_) != 0; };

  void setLongFormat(NABoolean v)
  {(v ? formatFlags_ |= LONG_ : formatFlags_ &= ~LONG_); };
  NABoolean longFormat() { return (formatFlags_ & LONG_) != 0; };

  ObjectType getObjectType() { return (ObjectType)ot_; };
 
  ObjectType getObjectTypeForStats();

  void setCleanupFrom(Int64 v) { from_ = v; }
  void setCleanupTo(Int64 v) { to_ = v; }
  Int64 cleanupFrom() { return from_; }
  Int64 cleanupTo() { return to_; }

  Int32 & srcImpUnqIxPos() { return srcImpUnqIxPos_; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, UInt32 flag);

  void setVersion(short v)
  { version_ = v;};

  short getVersion()
  { return version_; };

  short numPartns()
  { 
     if (replPartnsGuaNameList_ != (QueuePtr)NULL) 
        return replPartnsGuaNameList_->numEntries();
     else
        return 0;
  }

  short concurrency()
  { return (short)concurrency_; }

  short compressionType()
  { return compressionType_; }

  void setCompressionType(short compressionType)
  { compressionType_ = compressionType; }
 
  short diskPool()
  { return diskPool_; }

  void setDiskPool(short diskPool) 
  { diskPool_ = diskPool; }

  static const char *getObjectType(ObjectType ot);

  static const char *getReplType(ReplType replType);

private:
  enum
  {
    COMPRESS                = 0x00000001,
    GET_STATUS              = 0x00000002,
    SUSPEND                 = 0x00000004,
    RESUME                  = 0x00000008,
    ABORT                   = 0x00000010,
    VALIDATE_DATA           = 0x00000020,
    SCHEMA_REPL             = 0x00000040,
    PURGEDATA_TGT           = 0x00000080,
    TGT_ACTIONS             = 0x00000100,
    VALIDATE_TGT_DDL        = 0x00000200,
    CREATE_TGT_DDL          = 0x00000400,
    ALTER_TGT_OFFLINE       = 0x00000800,
    ALTER_TGT_ONLINE        = 0x00001000,
    ALTER_TGT_AUDITED       = 0x00002000,
    ALTER_TGT_UNAUDITED     = 0x00004000,
    RETURN_PARTN_DETAILS    = 0x00008000,
    INITIALIZE_REPLICATE    = 0x00010000,
    REINITIALIZE_REPLICATE  = 0x00020000,
    DROP_REPLICATE          = 0x00040000,
    TRANSFORM               = 0x00080000,
    GET_DETAILS             = 0x00100000,
    DEBUG_TARGET            = 0x00200000,
    RECOVER_SOURCE          = 0x00400000,
    RECOVER_TARGET          = 0x00800000,
    VALIDATE_PRIVS          = 0x01000000,
    REPLICATE_SCHEMA_STATS  = 0x02000000,
    TGT_CREATE              = 0x04000000,
    TGT_PROCESS             = 0x08000000,
    TGT_DROP                = 0x10000000,
    REPLICATE_WARNINGS      = 0x20000000,
    CLEANUP                 = 0x40000000,
    REPLICATE_SCHEMA_DDL    = 0x80000000
  };

  enum
  {
    ADD_CONFIG              = 0x00000001,
    REMOVE_CONFIG           = 0x00000002,
    IN_LIST_REPL            = 0x00000004,
    DISABLE_SCHEMA_CREATE   = 0x00000008,
    LOCK_SCHEMA             = 0x00000010,
    NO_VALIDATE_ROLE        = 0x00000020,
    SQ_TARGET_TYPE          = 0x00000040,
    INCREMENTAL             = 0x00000080,
    AUTHORIZATION_REPL      = 0x00000100,
    COPY_BOTH_DATA_AND_STATS= 0x00000200,
    DISPLAY_INTERNAL_CMD    = 0x00000400,
    INCREMENTAL_DISALLOWED  = 0x00000800,
    VALIDATE_DDL            = 0x00001000,
    VALIDATE_AND_PURGEDATA  = 0x00002000,
    WAITED_STARTUP          = 0x00004000
  };

  NABasicPtr sourceName_;
  NABasicPtr targetName_;
  NABasicPtr sourceSchema_;
  NABasicPtr targetSystem_;
  NABasicPtr ipAddr_;
  NABasicPtr ddlInputStr_;
  NABasicPtr schGetTablesStmt_;
  NABasicPtr schTableReplStmt_;
  NABasicPtr indexReplStmt_;
  NABasicPtr uniqImplIndexReplStmt_;
  NABasicPtr controlQueryId_;
  NABasicPtr srcCatName_;
  NABasicPtr tgtCatName_;
  NABasicPtr roleName_;

  // Set to the manageability catalog name
  NABasicPtr mgbltyCatalogName_;

  NABasicPtr testTgtMgbltyCatalogName_;

  UInt32     sourceLen_;
  UInt32     targetLen_;
  UInt32     sourceSchemaLen_;
  UInt32     targetSystemLen_;
  UInt32     ddlInputStrLen_;
  UInt32     schGetTablesStmtLen_;
  UInt32     schTableReplStmtLen_;
  UInt32     indexReplStmtLen_;
  UInt32     uniqImplIndexReplStmtLen_;
  UInt32     controlQueryIdLen_;

  ObjectType ot_;
  Lng32      concurrency_;
  Lng32      rate_;
  Lng32      priority_;
  Lng32      delay_;

  Int64      srcObjectUid_;

  UInt16     tgtStatsType_;
  UInt16     formatFlags_;

  UInt32     flags_;

  short      version_;
  short      compressionType_;

  // number of source partitions for a table being replicated.
  // this field is valid only in case of TGT_ACTIONS at target.
  // Used to validate that source and target have the same num
  // of partitions.
  Int32      numSrcPartns_;

  Int32      srcImpUnqIxPos_;

  Int64      from_;
  Int64      to_;

  char       forceError_[8];
  UInt32     flags2_;
  Int32      portNum_;

  Int32      numRetries_;
  short      diskPool_;
  UInt16     targetSchemaLen_;

  QueuePtr replIndexList_;

  QueuePtr replIndexTypeList_;

  QueuePtr replPartnsSegmentNameList_;

  // partition name as given by user
  QueuePtr replPartnsNameList_;

  // guardian name of the partitions
  QueuePtr replPartnsGuaNameList_;

  // partition cpu number
  QueuePtr replPartnsCpuList_;

  // partition dp2 name
  QueuePtr replPartnsDp2List_;

  // target schema
  NABasicPtr targetSchema_;

  char       fillersComTdbExeUtilReplicate_[72];
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
  // Should match the number set in ExeUtilReorg::bindNode and
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

  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

  void setCleanupAllTables(NABoolean v)
  {(v ? flags_ |= CLEANUP_ALL_TABLES : flags_ &= ~CLEANUP_ALL_TABLES); };
  NABoolean cleanupAllTables() { return (flags_ & CLEANUP_ALL_TABLES) != 0; };

private:
  enum
  {
    // cleanup obsolete and active schemas/tables.
    CLEANUP_ALL_TABLES          = 0x0001
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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

class ComTdbExeUtilFastDelete : public ComTdbExeUtil
{
public:
  ComTdbExeUtilFastDelete()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilFastDelete(char * tableName,
			  ULng32 tableNameLen,
			  char * primaryPartnLoc,
			  Queue * indexList,
			  char * stmt,
			  ULng32 stmtLen,
			  Lng32 numEsps,
			  Int64 objectUID,
			  Lng32 numLOBs,
			  char * lobNumArray,
			  ex_cri_desc * work_cri_desc,
			  const unsigned short work_atp_index,
			  ex_cri_desc * given_cri_desc,
			  ex_cri_desc * returned_cri_desc,
			  queue_index down,
			  queue_index up,
			  Lng32 num_buffers,
			  ULng32 buffer_size,
			  NABoolean ishiveTruncate = FALSE,
			  char * hiveTableLocation = NULL,
                          char * hiveHostName = NULL,
                          Lng32 hivePortNum = 0
			  );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);


  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilFastDelete);}

  virtual const char *getNodeName() const
  {
    if (isHiveTruncate())
      return "HIVE_TRUNCATE";
    else
      return "FAST_DELETE";
  };

  Queue* getIndexList()       { return indexList_; }

  char * purgedataStmt()      { return purgedataStmt_; }

  char * getPrimaryPartnLoc()  { return primaryPartnLoc_; }

  Lng32 getNumEsps() { return numEsps_;}

  char * getLOBnumArray() { return lobNumArray_; }

  short getLOBnum(short i);

  UInt16 numLOBs() { return numLOBs_; }

  Int64 getObjectUID() { return objectUID_; }

  char * getHiveTableLocation() const
  {
    return hiveTableLocation_;
  }

  char * getHiveHdfsHost() const
  {
    return hiveHdfsHost_;
  }

  Lng32 getHiveHdfsPort() const
  {
    return hiveHdfsPort_;
  }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

  void setDoPurgedataCat(NABoolean v)
  {(v ? flags_ |= DO_PURGEDATA_CAT : flags_ &= ~DO_PURGEDATA_CAT); };
  NABoolean doPurgedataCat() { return (flags_ & DO_PURGEDATA_CAT) != 0; };

  void setReturnPurgedataWarn(NABoolean v)
  {(v ? flags_ |= RETURN_PURGEDATA_WARN : flags_ &= ~RETURN_PURGEDATA_WARN); };
  NABoolean returnPurgedataWarn() { return (flags_ & RETURN_PURGEDATA_WARN) != 0; };

  void setIsMV(NABoolean v)
  {(v ? flags_ |= IS_MV: flags_ &= ~IS_MV); };
  NABoolean isMV() { return (flags_ & IS_MV) != 0; };

  void setDoParallelDelete(NABoolean v)
  {(v ? flags_ |= DO_PARALLEL_DELETE: flags_ &= ~DO_PARALLEL_DELETE); };
  NABoolean doParallelDelete() { return (flags_ & DO_PARALLEL_DELETE) != 0; };

  void setDoParallelDeleteIfXn(NABoolean v)
  {(v ? flags_ |= DO_PARALLEL_DELETE_IF_XN: flags_ &= ~DO_PARALLEL_DELETE_IF_XN); };
  NABoolean doParallelDeleteIfXn() { return (flags_ & DO_PARALLEL_DELETE_IF_XN) != 0; };

  void setOfflineTable(NABoolean v)
  {(v ? flags_ |= OFFLINE_TABLE : flags_ &= ~OFFLINE_TABLE); };
  NABoolean offlineTable() { return (flags_ & OFFLINE_TABLE) != 0; };

  void setIsHiveTruncate(NABoolean v)
  {(v ? flags_ |= IS_HIVE_TRUNCATE : flags_ &= ~IS_HIVE_TRUNCATE); };
  NABoolean isHiveTruncate() const { return (flags_ & IS_HIVE_TRUNCATE) != 0; };

  void setDoLabelPurgedata(NABoolean v)
  {(v ? flags_ |= DO_LABEL_PURGEDATA: flags_ &= ~DO_LABEL_PURGEDATA); };
  NABoolean doLabelPurgedata() { return (flags_ & DO_LABEL_PURGEDATA) != 0; };

private:
  enum
  {
    DO_PURGEDATA_CAT         = 0x0001,
    RETURN_PURGEDATA_WARN    = 0x0002,
    IS_MV                    = 0x0004,
    DO_PARALLEL_DELETE       = 0x0008,
    DO_PARALLEL_DELETE_IF_XN = 0x0010,
    OFFLINE_TABLE            = 0x0020,
    DO_LABEL_PURGEDATA       = 0x0040,
    IS_HIVE_TRUNCATE         = 0x0080
   };

  // list of indexes on the table.
  QueuePtr indexList_;                               // 00-07

  NABasicPtr purgedataStmt_;                         // 08-15

  NABasicPtr primaryPartnLoc_;                       // 16-23

  UInt32 purgedataStmtLen_;                          // 24-27

  UInt32 flags_;                                     // 28-31

  UInt32 numEsps_;                                   // 32-35

  // next 3 fields are used if table contains LOBs
  UInt16 numLOBs_;                                   // 36-37
  char   filler1_[2];                                // 38-39
  
  // array of shorts. numLOBs entries. 
  // Each entry is the lobNum.
  NABasicPtr lobNumArray_;                           // 40-47

  Int64 objectUID_;                                  // 48-55
  // hiveTable loaction will be extended for partitions later
  NABasicPtr  hiveTableLocation_;                    // 56-63
  NABasicPtr hiveHdfsHost_;                          // 64-71
  Int32 hiveHdfsPort_;                               // 72-75
  char fillersComTdbExeUtilFastDelete_[52];          // 76-127
};

class ComTdbExeUtilGetStatistics : public ComTdbExeUtil
{
  friend class ExExeUtilGetStatisticsTcb;
  friend class ExExeUtilGetRTSStatisticsTcb;
  friend class ExExeUtilGetReorgStatisticsTcb;
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

  short getStatsReqType() { return statsReqType_; }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);
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
    TOKENIZED_FORMAT = 0x0040
  };

  NABasicPtr stmtName_;                                        // 00-07

  UInt32 flags_;                                               // 08-11

  char filler1_[4];                                            // 12-15
  short statsReqType_;                                         // 16-17
  short statsMergeType_;                                       // 18-19

  short activeQueryNum_;                                       // 20-21

  char fillersComTdbExeUtilGetStatistics_[106];                // 22-127
};

class ComTdbExeUtilGetReorgStatistics : public ComTdbExeUtilGetStatistics
{

public:
  ComTdbExeUtilGetReorgStatistics()
  : ComTdbExeUtilGetStatistics()
  {}

  ComTdbExeUtilGetReorgStatistics(
				  char * qid,
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
  : ComTdbExeUtilGetStatistics(qid, 
			       statsReqType, statsMergeType, activeQueryNum,
			       //SQLCLI_STATS_REQ_QID, SQLCLI_DEFAULT_STATS, -1,
			       work_cri_desc, work_atp_index,
			       given_cri_desc, returned_cri_desc,
			       down, up, num_buffers, buffer_size)
    {
      setNodeType(ComTdb::ex_GET_REORG_STATISTICS);
    };
  

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() {return (short)sizeof(ComTdbExeUtilGetReorgStatistics);}

  virtual const char *getNodeName() const
  {
    return "GET_REORG_STATISTICS";
  };

  inline const char * getQid() const { return getStmtName(); };

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

private:
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

private:
  Int64 uid_;                                        // 00-07

  UInt32 flags_;                                     // 08-15

  char fillersComTdbExeUtilGetUID_[108];             // 16-133
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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

    TABLES_IN_SCHEMA_,
    INDEXES_IN_SCHEMA_,
    VIEWS_IN_SCHEMA_,
    LIBRARIES_IN_SCHEMA_,
    MVS_IN_SCHEMA_,
    MVGROUPS_IN_SCHEMA_,
    PRIVILEGES_ON_SCHEMA_,
    PROCEDURES_IN_SCHEMA_,
    SEQUENCES_IN_SCHEMA_,
    SYNONYMS_IN_SCHEMA_,
    FUNCTIONS_IN_SCHEMA_,
    TABLE_FUNCTIONS_IN_SCHEMA_,

    OBJECTS_IN_SCHEMA_,
    INVALID_VIEWS_IN_SCHEMA_,

    INDEXES_ON_TABLE_,
    INDEXES_ON_MV_,
    VIEWS_ON_TABLE_,
    VIEWS_ON_VIEW_,
    MVS_ON_TABLE_,
    MVS_ON_VIEW_,
    MVS_ON_MV_,
    MVGROUPS_ON_TABLE_,
    PRIVILEGES_ON_TABLE_,
    PRIVILEGES_ON_MV_,
    PRIVILEGES_ON_VIEW_,
    SYNONYMS_ON_TABLE_,

    OBJECTS_ON_TABLE_,
    PARTITIONS_FOR_TABLE_,
    PARTITIONS_FOR_INDEX_,

    TABLES_IN_VIEW_,
    VIEWS_IN_VIEW_,
    OBJECTS_IN_VIEW_,
    TABLES_IN_MV_,
    MVS_IN_MV_,
    OBJECTS_IN_MV_,

    ROLES_,

    ROLES_FOR_ROLE_,
    USERS_FOR_ROLE_,

    USERS_,
    CURRENT_USER_,

    CATALOGS_FOR_USER_,
    INDEXES_FOR_USER_,
    LIBRARIES_FOR_USER_,
    MVGROUPS_FOR_USER_,
    MVS_FOR_USER_,
    PRIVILEGES_FOR_USER_,
    PROCEDURES_FOR_USER_,
    ROLES_FOR_USER_,
    SCHEMAS_FOR_USER_,
    SYNONYMS_FOR_USER_,
    TABLES_FOR_USER_,
    TRIGGERS_FOR_USER_,
    VIEWS_FOR_USER_, 
    
    PROCEDURES_FOR_LIBRARY_,
    FUNCTIONS_FOR_LIBRARY_,
    TABLE_FUNCTIONS_FOR_LIBRARY_,

    COMPONENTS_,
    COMPONENT_OPERATIONS_,
    COMPONENT_PRIVILEGES_,
    IUDLOG_TABLES_IN_SCHEMA_,
    RANGELOG_TABLES_IN_SCHEMA_,
    TRIGTEMP_TABLES_IN_SCHEMA_,
    IUDLOG_TABLE_ON_TABLE_,
    RANGELOG_TABLE_ON_TABLE_,
    TRIGTEMP_TABLE_ON_TABLE_,
    IUDLOG_TABLE_ON_MV_,
    RANGELOG_TABLE_ON_MV_,
    TRIGTEMP_TABLE_ON_MV_
    
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
       ULng32 buffer_size
       );

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

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

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
    IS_HBASE   = 0x0800
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
    TO_EXTERNAL_FROM_FILE_, NOOP_
  };

  ComTdbExeUtilLobExtract()
  : ComTdbExeUtil()
  {}

  ComTdbExeUtilLobExtract
    (
     char * handle,
     Lng32 handleLen,
     ExtractToType toType,
     Int64 size,
     Int64 size2,
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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

private:
  enum
  {
    STRING_FORMAT    = 0x0001,
    SRC_IS_FILE           = 0x0002,
    WITH_CREATE        = 0x0004
  };

  NABasicPtr handle_;                                      // 00-07
  short toType_;                                           // 08-09
  short flags_;                                            // 10-11
  Lng32 handleLen_;
  Int64 size_; // row size
  Int64 size2_; // buf size
  Lng32 lobStorageType_ ; // valid when handle is null and extract is from a file.
  Lng32 lobHdfsPort_;
  NABasicPtr lobHdfsServer_;
  NABasicPtr stringParam1_;
  NABasicPtr stringParam2_;
  NABasicPtr stringParam3_;

  char fillersComTdbExeUtilLobExtract_[40];            // 12-55
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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

  short getLOBnum(short i);
  char * getLOBloc(short i);

  UInt16 numLOBs() { return numLOBs_; }

  char * getLOBnumArray() { return lobNumArray_; }
  char * getLOBlocArray() { return lobLocArray_; }

  char * schName() { return schName_; };
private:
  UInt32 flags_;                                     // 00-03

  UInt16 numLOBs_;                                   // 04-05
  short maxLocLen_;
  
  // array of shorts. numLOBs entries. 
  // Each entry is the lobNum.
  NABasicPtr lobNumArray_;                           // 08-15

  // array of string, null terminated. numLOBs entries. 
  // Each entry is the storage location of lob data file.
  NABasicPtr lobLocArray_;                           // 16-23

  Int64 objectUID_;                                  // 24-31

  NABasicPtr schName_;                               // 32-39
  short schNameLen_;                                 // 40-41

  short sdOptions_;                                  // 42-43

  char fillersComTdbExeUtilLobShowddl_[4];           // 44-47
};


static const ComTdbVirtTableColumnInfo hiveMDTablesColInfo[] =
{                                                                                     
  { "CATALOG_NAME",   0,      COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",     1,    COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",        2,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
};

struct HiveMDTablesColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
};

static const ComTdbVirtTableColumnInfo hiveMDColumnsColInfo[] =
{                                                                                     
  { "CATALOG_NAME",    0,     COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "SCHEMA_NAME",      1,    COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "TABLE_NAME",          2,   COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "COLUMN_NAME",       3,  COM_USER_COLUMN, REC_BYTE_F_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "SQL_DATA_TYPE",     4,  COM_USER_COLUMN, REC_BYTE_F_ASCII,    24,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "FS_DATA_TYPE",        5,  COM_USER_COLUMN, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "COLUMN_SIZE",         6,   COM_USER_COLUMN, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "CHARACTER_SET",     7,   COM_USER_COLUMN, REC_BYTE_F_ASCII,     40,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}, 
  { "COLUMN_PRECISION", 8,  COM_USER_COLUMN, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "COLUMN_SCALE",       9,   COM_USER_COLUMN, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "DT_CODE",                 10,  COM_USER_COLUMN, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "NULLABLE",                11, COM_USER_COLUMN, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "COLUMN_NUMBER",    12,     COM_USER_COLUMN, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}, 
  { "DATETIME_QUALIFIER", 13,    COM_USER_COLUMN, REC_BYTE_F_ASCII,    28,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "DATETIME_START_FIELD", 14,  COM_USER_COLUMN, REC_BIN32_SIGNED,   4,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "DATETIME_END_FIELD",   15, COM_USER_COLUMN, REC_BIN32_SIGNED,   4,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
  { "DEFAULT_VALUE",        16, COM_USER_COLUMN, REC_BYTE_F_ASCII,    240,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0} 
};

struct HiveMDColumnsColInfoStruct
{
  char catName[256];
  char schName[256];
  char tblName[256];
  char colName[256];
  char sqlDatatype[24];
  Lng32 fsDatatype;
  Lng32 colSize;
  char charSet[40];
  Lng32 colPrecision;
  Lng32 colScale;
  Lng32 dtCode;
  Lng32 nullable;
  Lng32 colNum;
  char dtQualifier[28];
  Lng32 dtStartField;
  Lng32 dtEndField;
  char defVal[240];
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
    SYSTEM_TABLES_
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
               char * schemaName
	       );

  char * hivePredStr() { return hivePredStr_;}
  char * getSchema() {return schema_;}

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
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
    else
      return NULL;
  }

private:

  Int32 mdType_;                                                   // 00 - 03

  char fillers_[4];                                                // 04 - 07
  
  NABasicPtr hivePredStr_;                                         // 08 - 15

  NABasicPtr schema_;                                              // 16 - 23
};

////////////////////////////////////////////////////////////////////
// class ComTdbExeUtilMetadataUpgrade
////////////////////////////////////////////////////////////////////
class ComTdbExeUtilMetadataUpgrade : public ComTdbExeUtil
{
  enum MUflags
  {
    GET_MD_VERSION          = 0x0001,
    GET_SW_VERSION = 0x0002
  };

public:
  
  ComTdbExeUtilMetadataUpgrade()
  : ComTdbExeUtil() {};
  
  ComTdbExeUtilMetadataUpgrade(
			ex_expr * output_expr,
			ULng32 output_rowlen,
			ex_cri_desc * workCriDesc,
			const unsigned short work_atp_index,
			ex_cri_desc *criDescParentDown,
			ex_cri_desc *criDescParentUp,
			queue_index queueSizeDown,
			queue_index queueSizeUp,
			Lng32 numBuffers,
			ULng32 bufferSize);

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

  virtual short getClassSize()   { return (short)sizeof(ComTdbExeUtilMetadataUpgrade); }
  
  void setGetMDVersion(NABoolean v)
  {(v ? flags_ |= GET_MD_VERSION : flags_ &= ~GET_MD_VERSION); }
  NABoolean getMDVersion() { return (flags_ & GET_MD_VERSION) != 0;}

  void setGetSWVersion(NABoolean v)
  {(v ? flags_ |= GET_SW_VERSION : flags_ &= ~GET_SW_VERSION); }
  NABoolean getSWVersion() { return (flags_ & GET_SW_VERSION) != 0;}

protected:

  UInt32 flags_;                      
  char fillersComTdbMU_[28];  

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
                             ULng32 buffer_size
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

  void setLogErrors(NABoolean v)
  {(v ? flags_ |= LOG_ERRORS : flags_ &= ~LOG_ERRORS); };
  NABoolean getLogErrors() { return (flags_ & LOG_ERRORS) != 0; };

  void setSecure(NABoolean v)
  {(v ? flags_ |= SECURE : flags_ &= ~SECURE); };
  NABoolean getSecure() { return (flags_ & SECURE) != 0; };

  void setNoDuplicates(NABoolean v)
    {(v ? flags_ |= NO_DUPLICATES : flags_ &= ~NO_DUPLICATES); };
  NABoolean getNoDuplicates() { return (flags_ & NO_DUPLICATES) != 0; };

  void setIndexes(NABoolean v)
    {(v ? flags_ |= INDEXES : flags_ &= ~INDEXES); };
  NABoolean getIndexes() { return (flags_ & INDEXES) != 0; };

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

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

private:
  enum
  {
    PRE_LOAD_CLEANUP = 0x0001,
    PREPARATION      = 0x0002,
    KEEP_HFILES      = 0x0004,
    TRUNCATE_TABLE   = 0x0008,
    NO_ROLLBACK      = 0x0010,
    LOG_ERRORS       = 0x0020,
    SECURE           = 0x0040,
    NO_DUPLICATES    = 0x0080,
    INDEXES          = 0x0100,
    CONSTRAINTS      = 0x0200,
    NO_OUTPUT        = 0x0400,
    INDEX_TABLE_ONLY = 0x0800,
    UPSERT_USING_LOAD= 0x1000,
    FORCE_CIF        = 0x2000
  };

  // load stmt
  NABasicPtr ldQuery_;                               // 00-07

  UInt32 flags_;                                     // 08-11

  char fillersExeUtilHbaseLoad_[4];                  // 12-15
};

//******************************************************
// Bulk Unload
//**************************8

class ComTdbExeUtilHBaseBulkUnLoad : public ComTdbExeUtil
{
  friend class ExExeUtilHBaseBulkUnLoadTcb;
  friend class ExExeUtilHbaseUnLoadPrivateState;

public:
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

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

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
  char         fillersExeUtilHbaseUnLoad_[3];           // 29 - 31
};
#endif



