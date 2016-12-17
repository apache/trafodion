// **********************************************************************
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
// **********************************************************************
#ifndef COM_HBASE_ACCESS_H
#define COM_HBASE_ACCESS_H

#include "ComTdb.h"
#include "ComQueue.h"
#include "ComKeyRange.h" // keyInfo_ dereferenced in some inline methods.

static const ComTdbVirtTableColumnInfo hbaseTableColumnInfo[] =
{                                                                                     
  { "ROW_ID",               0,   COM_USER_COLUMN, REC_BYTE_V_ASCII,    128, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,"cf", "1",COM_UNKNOWN_DIRECTION_LIT, 0},
  { "COL_FAMILY",       1,   COM_USER_COLUMN, REC_BYTE_V_ASCII,    128, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,"cf", "2",COM_UNKNOWN_DIRECTION_LIT, 0},
  { "COL_NAME",          2,   COM_USER_COLUMN, REC_BYTE_V_ASCII,    128, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,"cf", "3",COM_UNKNOWN_DIRECTION_LIT, 0},
  { "COL_TIMESTAMP", 3,   COM_USER_COLUMN, REC_BIN64_SIGNED,       8, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,"cf", "4",COM_UNKNOWN_DIRECTION_LIT, 0},  
  { "COL_VALUE",         4,   COM_USER_COLUMN, REC_BYTE_V_ASCII,    256, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" , "cf","5",COM_UNKNOWN_DIRECTION_LIT, 0}

};

#define HBASE_ROW_ID_INDEX 0
#define HBASE_COL_FAMILY_INDEX 1
#define HBASE_COL_NAME_INDEX 2
#define HBASE_COL_TS_INDEX 3
#define HBASE_COL_VALUE_INDEX 4


static const ComTdbVirtTableKeyInfo hbaseTableKeyInfo[] =
{
  // columnname      keyseqnumber tablecolnumber ordering nonkeycol
  {    "ROW_ID",                 1,                                0,            0  ,         0, NULL, NULL}
};

struct HbaseTableColInfoStruct
{
  char rowId[128];
  char colFamily[128];
  char colName[128];
  Int64 colTS;
  char colVal[256];
};

static const ComTdbVirtTableColumnInfo hbaseTableRowwiseColumnInfo[] =
{                                                                                     
  { "ROW_ID",                 1, COM_USER_COLUMN, REC_BYTE_V_ASCII,    128,   FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "", "cf", "1", COM_UNKNOWN_DIRECTION_LIT, 0},
  { "COLUMN_DETAILS", 0, COM_USER_COLUMN, REC_BYTE_V_ASCII,   1024, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "", "cf", "2", COM_UNKNOWN_DIRECTION_LIT, 0}
};

#define HBASE_ROW_ROWID_INDEX 0
#define HBASE_COL_DETAILS_INDEX 1

static const ComTdbVirtTableKeyInfo hbaseTableRowwiseKeyInfo[] =
{
  // indexname keyseqnumber tablecolnumber ordering
  {    NULL,          1,                                0,            0  , 0, NULL, NULL}
};

struct HbaseTableRowwiseColInfoStruct
{
  char rowId[128];
  char colInfo;
};

class ComTdbHbaseAccess : public ComTdb
{
  friend class ExHbaseAccessTcb;
  friend class ExHbaseAccessScanTcb;
  friend class ExHbaseAccessScanRowwiseTcb;
  friend class ExHbaseAccessGetTcb;
  friend class ExHbaseAccessGetRowwiseTcb;
  friend class ExHbaseAccessSelectTcb;
  friend class ExHbaseAccessDeleteTcb;  
  friend class ExHbaseAccessInsertTcb;
  friend class ExHbaseAccessInsertRowwiseTcb;
  friend class ExHbaseAccessInsertSQTcb;
  friend class ExHbaseAccessUpsertVsbbSQTcb;
  friend class ExHbaseAccessSQRowsetTcb;
  friend class ExHbaseAccessDDLTcb;
  friend class ExHbaseAccessInitMDTcb;
  friend class ExHbaseAccessGetTablesTcb;
  friend class ExHbaseAccessUMDTcb;
  friend class ExHbaseUMDtrafUniqueTaskTcb;
  friend class ExHbaseUMDnativeUniqueTaskTcb;
  friend class ExHbaseUMDtrafSubsetTaskTcb;
  friend class ExHbaseUMDnativeSubsetTaskTcb;
  friend class ExHbaseCoProcAggrTcb;
  friend class ExMetadataUpgradeTcb;
  friend class ExHbaseAccessBulkLoadPrepSQTcb;
  friend class ExHbaseAccessBulkLoadTaskTcb;

public:
  enum ComTdbAccessType
  {
    NOOP_,
    SELECT_,
    INSERT_,
    UPSERT_,
    UPSERT_LOAD_,
    UPDATE_,
    MERGE_,
    DELETE_,
    COPROC_,
    CREATE_,
    DROP_,
    GET_TABLES_,
    INIT_MD_,
    DROP_MD_,
    UPGRADE_MD_,
    BULK_LOAD_PREP_,
    BULK_LOAD_TASK_
  };

  const char * getAccessTypeStr(UInt16 v)
  {
    switch (ComTdbAccessType(v))
      {
      case NOOP_: return "NOOP_"; break;
      case SELECT_: return "SELECT_"; break;
      case INSERT_: return "INSERT_"; break;
      case UPSERT_: return "UPSERT_"; break;
      case UPSERT_LOAD_: return "UPSERT_LOAD_"; break;
      case DELETE_: return "DELETE_"; break;
      case UPDATE_: return "UPDATE_"; break;
      case MERGE_: return "MERGE_"; break;
      case CREATE_: return "CREATE_"; break;
      case DROP_: return "DROP_"; break;
      case GET_TABLES_: return "GET_TABLES_"; break;
      case COPROC_: return "COPROC_"; break;
      case UPGRADE_MD_: return "UPGRADE_MD_"; break;
      case BULK_LOAD_PREP_: return "BULK_LOAD_PREP"; break;
      case BULK_LOAD_TASK_: return "BULK_LOAD_TASK"; break;
      default: return "NOOP_"; break;
      };

    return NULL;
  };

  ComTdbAccessType getAccessType() const { return ComTdbAccessType(accessType_); }

  class HbaseScanRows : public NAVersionedObject
  {
  public:
    HbaseScanRows()
      :  NAVersionedObject(-1)
      {}

    virtual unsigned char getClassVersionID()
    {
      return 1;
    }

    virtual void populateImageVersionIDArray()
    {
      setImageVersionID(0,getClassVersionID());
    }
    
    virtual short getClassSize() { return (short)sizeof(HbaseScanRows); }

    virtual Long pack(void * space);
    
    virtual Lng32 unpack(void * base, void * reallocator);

    char * beginRowId() { return beginRowId_; }
    char * endRowId() { return endRowId_; }

    Queue* colNames()       { return colNames_; }

    NABasicPtr beginRowId_;
    NABasicPtr endRowId_;

    Lng32 beginKeyExclusive_;
    Lng32 endKeyExclusive_;

    QueuePtr colNames_;

    Int64 colTS_;
  };
  
 class HbaseGetRows : public NAVersionedObject
  {
  public:
    HbaseGetRows()
      :  NAVersionedObject(-1)
      {}

    virtual unsigned char getClassVersionID()
    {
      return 1;
    }

    virtual void populateImageVersionIDArray()
    {
      setImageVersionID(0,getClassVersionID());
    }
    
    virtual short getClassSize() { return (short)sizeof(HbaseGetRows); }

    virtual Long pack(void * space);
    
    virtual Lng32 unpack(void * base, void * reallocator);

    Queue * rowIds() { return rowIds_; }
    Queue* colNames()       { return colNames_; }

    QueuePtr rowIds_;
    QueuePtr colNames_;

    Int64 colTS_;
  };

  class HbasePerfAttributes  : public NAVersionedObject
  {
  public:
  HbasePerfAttributes()
    :  NAVersionedObject(-1),
      flags_(0),
      numCacheRows_(100)
	{}
    
    virtual unsigned char getClassVersionID()
    {
      return 1;
    }
    
    virtual void populateImageVersionIDArray()
    {
      setImageVersionID(0,getClassVersionID());
    }
    
    virtual short getClassSize() { return (short)sizeof(HbasePerfAttributes); }
    
    virtual Long pack(void * space);
    
    virtual Lng32 unpack(void * base, void * reallocator);
    
    void setNumCacheRows(UInt32 n) { numCacheRows_ = n;}
    UInt32 numCacheRows() { return numCacheRows_; }

    void setCacheBlocks(NABoolean v)
    {(v ? flags_ |= CACHE_BLOCKS : flags_ &= ~CACHE_BLOCKS); };
    NABoolean cacheBlocks() { return (flags_ & CACHE_BLOCKS) != 0; };
    
    void setUseMinMdamProbeSize(NABoolean v)
    {(v ? flags_ |= USE_MIN_MDAM_PROBE_SIZE : 
      flags_ &= ~USE_MIN_MDAM_PROBE_SIZE); };
    NABoolean useMinMdamProbeSize() 
    { return (flags_ & USE_MIN_MDAM_PROBE_SIZE) != 0; };

    void setUseSmallScanner(NABoolean v)
    {(v ? flags_ |= USE_SMALL_SCANNER :
      flags_ &= ~USE_SMALL_SCANNER); };
    NABoolean useSmallScanner()
    { return (flags_ & (USE_SMALL_SCANNER | USE_SMALL_SCANNER_FOR_MDAM)) != 0; };
    void setUseSmallScannerForProbes(NABoolean v)
    {(v ? flags_ |= USE_SMALL_SCANNER_FOR_PROBES :
      flags_ &= ~USE_SMALL_SCANNER_FOR_PROBES); };
    NABoolean useSmallScannerForProbes()
    { return (flags_ & USE_SMALL_SCANNER_FOR_PROBES) != 0; };

    void setUseSmallScannerForMDAMifNeeded(UInt32 numRowRetrieved){
        //if last scan of mdam fitted in one block, and small scanner CQD is either ON or SYSTEM (this is summarized in USE_SMALL_SCANNER_FOR_PROBES)
        //then next MDAM scan can use small scanner. Most likely it is about same size as previous one.
        if ((numRowRetrieved < maxNumRowsPerHBaseBlock_) && ((flags_ & USE_SMALL_SCANNER_FOR_PROBES) != 0))
            flags_ |= USE_SMALL_SCANNER_FOR_MDAM;
        else
            flags_ &= ~USE_SMALL_SCANNER_FOR_MDAM;
    }

    void setMaxNumRowsPerHbaseBlock(UInt32 n) { maxNumRowsPerHBaseBlock_ = n;}
    UInt32 maxNumRowsPerHbaseBlock() { return maxNumRowsPerHBaseBlock_; }

    void setDopParallelScanner(Float32 f) { dopParallelScanner_ = f;}
    Float32 dopParallelScanner() { return dopParallelScanner_; }

  private:
    enum
    {
      CACHE_BLOCKS                 = 0x0001,
      USE_MIN_MDAM_PROBE_SIZE      = 0x0002,
      USE_SMALL_SCANNER            = 0x0004,
      USE_SMALL_SCANNER_FOR_PROBES = 0x0008,
      USE_SMALL_SCANNER_FOR_MDAM   = 0x0010
    };
    
    UInt32 flags_;
    UInt32 numCacheRows_;
    UInt32 maxNumRowsPerHBaseBlock_;
    Float32 dopParallelScanner_;
  };

  // ---------------------------------------------------------------------
  // Template instantiation to produce a 64-bit pointer emulator class
  // for HbasePerfAttributes
  // ---------------------------------------------------------------------
  typedef NAVersionedObjectPtrTempl<HbasePerfAttributes> HbasePerfAttributesPtr;
  

  class HbaseSnapshotScanAttributes  : public NAVersionedObject
  {
  public:
    HbaseSnapshotScanAttributes()
    :  NAVersionedObject(-1),
      flags_(0),
      snapScanTmpLocation_(NULL),
      snapshotScanTimeout_(0),
      snapshotName_(NULL)
        {}

    virtual unsigned char getClassVersionID()
    {
      return 1;
    }

    virtual void populateImageVersionIDArray()
    {
      setImageVersionID(0,getClassVersionID());
    }

    virtual short getClassSize() { return (short)sizeof(HbaseSnapshotScanAttributes); }

    virtual Long pack(void * space);

    virtual Lng32 unpack(void * base, void * reallocator);

    void setUseSnapshotScan(NABoolean v)
      {(v ? flags_ |= TRAF_USE_SNAPSHOT_SCAN_ATTR : flags_ &= ~TRAF_USE_SNAPSHOT_SCAN_ATTR); };
    NABoolean getUseSnapshotScan() { return (flags_ & TRAF_USE_SNAPSHOT_SCAN_ATTR) != 0; };

    char * getSnapScanTmpLocation() const { return snapScanTmpLocation_; }
    void   setSnapScanTmpLocation(char *  v) { snapScanTmpLocation_= v; }

    // The number of times we try to get the scanner before giving up
    UInt32 getSnapshotScanTimeout() const { return snapshotScanTimeout_; }
    void setSnapshotScanTimeout(UInt32 v) { snapshotScanTimeout_ = v;}

    char * getSnapshotName() const { return snapshotName_; }
    void   setSnapshotName(char *  v) { snapshotName_= v; }

  private:
    enum
    {
      TRAF_USE_SNAPSHOT_SCAN_ATTR               = 0x0001
    };


    NABasicPtr snapScanTmpLocation_;
    NABasicPtr snapshotName_;
    UInt32     snapshotScanTimeout_;
    UInt32     flags_;
  };
  typedef NAVersionedObjectPtrTempl<HbaseSnapshotScanAttributes> HbaseSnapshotScanAttributesPtr;
	
  class HbaseAccessOptions : public NAVersionedObject
  {
  public:
  HbaseAccessOptions(Lng32 v = 0) :
    versions_(v)
    {
    }
    
    virtual unsigned char getClassVersionID()
    {
      return 1;
    }

    virtual void populateImageVersionIDArray()
    {
      setImageVersionID(0,getClassVersionID());
    }

    virtual short getClassSize() { return (short)sizeof(HbaseAccessOptions); }

    Lng32 getNumVersions() { return versions_; }

    NABoolean multiVersions() { return (versions_ != 0);}
  private:
    // 0, version not specified, return default of 1.
    // -1, return max versions
    // -2, return all versions.
    // N, return N versions.
    Lng32 versions_; 
    char filler_[4];
  };

 // ---------------------------------------------------------------------
  // Template instantiation to produce a 64-bit pointer emulator class
  // for HbaseAccessOptions
  // ---------------------------------------------------------------------
  typedef NAVersionedObjectPtrTempl<HbaseAccessOptions> HbaseAccessOptionsPtr;
  	
  // Constructors

  // Default constructor (used in ComTdb::fixupVTblPtr() to extract
  // the virtual table after unpacking.

  ComTdbHbaseAccess();

  // Constructor used by the generator.
  ComTdbHbaseAccess(
		    ComTdbAccessType type,
		    char * tableName,

		    ex_expr *convertExpr,
		    ex_expr *scanExpr,
		    ex_expr *rowIdExpr,
		    ex_expr *updateExpr,
		    ex_expr *mergeInsertExpr,
		    ex_expr *mergeInsertRowIdExpr,
		    ex_expr *mergeUpdExpr,
		    ex_expr *returnFetchExpr,
		    ex_expr *returnUpdateExpr,
		    ex_expr *returnMergeInsertExpr,
		    ex_expr *encodedKeyExpr,
		    ex_expr *keyColValExpr,
		    ex_expr *hbaseFilterExpr,

		    UInt32 asciiRowLen,
		    UInt32 convertRowLen,
		    UInt32 updateRowLen,
		    UInt32 mergeInsertRowLen,
		    UInt32 returnFetchedRowLen,
		    UInt32 returnUpdatedRowLen,

		    UInt32 rowIdLen,
		    UInt32 outputRowLen,
		    UInt32 rowIdAsciiRowLen,
		    UInt32 keyLen,
		    UInt32 keyColValLen,
		    UInt32 hbaseFilterValRowLen,

		    const UInt16 asciiTuppIndex,
		    const UInt16 convertTuppIndex,
		    const UInt16 updateTuppIndex,
		    const UInt16 mergeInsertTuppIndex,
		    const UInt16 mergeInsertRowIdTuppIndex,
		    const UInt16 returnedFetchedTuppIndex,
		    const UInt16 returnedUpdatedTuppIndex,
		    
		    const UInt16 rowIdTuppIndex,
		    const UInt16 returnedTuppIndex,
		    const UInt16 rowIdAsciiTuppIndex,
		    const UInt16 keyTuppIndex,
		    const UInt16 keyColValTuppIndex,
		    const UInt16 hbaseFilterValTuppIndex,

                    const UInt16 hbaseTimestampTuppIndex,
                    const UInt16 hbaseVersionTuppIndex,
                    
		    Queue * listOfScanRows,
		    Queue * listOfGetRows,
		    Queue * listOfFetchedColNames,
		    Queue * listOfUpDeldColNames,
		    Queue * listOfMergedColNames,

		    keyRangeGen * keyInfo,
		    char * keyColName,

		    ex_cri_desc *workCriDesc,
		    ex_cri_desc *criDescParentDown,
		    ex_cri_desc *criDescParentUp,
		    queue_index queueSizeDown,
		    queue_index queueSizeUp,
		    Cardinality expectedRows,
		    Lng32 numBuffers,
		    ULng32 bufferSize,
		    char * server,
		    char * zkPort,
		    HbasePerfAttributes * hbasePerfAttributes,
		    Float32 samplingRate = -1,
		    HbaseSnapshotScanAttributes * hbaseSnapshotScanAttributes = NULL,

                    HbaseAccessOptions * hbaseAccessOptions = NULL,

                    char * pkeyColName = NULL
	       );
  
  ComTdbHbaseAccess(
		    ComTdbAccessType type,
		    char * tableName,
		    
		    const UInt16 returnedTuppIndex,
		    Queue * colFamNameList,
		    
		    ex_cri_desc *workCriDesc,
		    ex_cri_desc *criDescParentDown,
		    ex_cri_desc *criDescParentUp,
		    queue_index queueSizeDown,
		    queue_index queueSizeUp,
		    Cardinality expectedRows,
		    Lng32 numBuffers,
		    ULng32 bufferSize,
		    char * server,
		    char * zkPort
		    );

  ~ComTdbHbaseAccess();

  static Int32 getVirtTableNumCols()
  {
    return sizeof(hbaseTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }
  
  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)hbaseTableColumnInfo;
  }
  
  static Int32 getVirtTableNumKeys()
  {
    return sizeof(hbaseTableKeyInfo)/sizeof(ComTdbVirtTableKeyInfo);
  }
  
  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return (ComTdbVirtTableKeyInfo *)hbaseTableKeyInfo;
  }
  
  // rowwise table format
 static Int32 getVirtTableRowwiseNumCols()
  {
    return sizeof(hbaseTableRowwiseColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }
  
  static ComTdbVirtTableColumnInfo * getVirtTableRowwiseColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)hbaseTableRowwiseColumnInfo;
  }
  
  static Int32 getVirtTableRowwiseNumKeys()
  {
    return sizeof(hbaseTableRowwiseKeyInfo)/sizeof(ComTdbVirtTableKeyInfo);
  }
  
  static ComTdbVirtTableKeyInfo * getVirtTableRowwiseKeyInfo()
  {
    return (ComTdbVirtTableKeyInfo *)hbaseTableRowwiseKeyInfo;
  }
  
  
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

  virtual short getClassSize() { return (short)sizeof(ComTdbHbaseAccess); }

  // Pack and Unpack routines
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // For the GUI, Does nothing right now
  void display() const {};

  void displayRowId(Space * space, char * inputRowIdBuf);

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC void displayContents(Space *space, ULng32 flag);

  // Virtual routines to provide a consistent interface to TDB's

  virtual const ComTdb *getChild(Int32 /*child*/) const { return NULL; };

  // numChildren always returns 0 for ComTdbStats
  virtual Int32 numChildren() const { return 0; };

  virtual const char *getNodeName() const;
  // virtual const char *getNodeName() const { return "EX_HBASE_ACCESS"; };

  // numExpressions always returns 2 for ComTdbStats
  virtual Int32 numExpressions() const;
  
  // The names of the expressions
  virtual const char * getExpressionName(Int32) const;

  // The expressions themselves
  virtual ex_expr* getExpressionNode(Int32);

  keySingleSubsetGen * keySubsetGen() const
  {
    return
      (keyInfo_ && keyInfo_->castToKeySingleSubsetGen() ? 
       keyInfo_->castToKeySingleSubsetGen() :
       NULL);    
  }

  keyMdamGen * keyMDAMGen() const
  {
    return
      (keyInfo_ && keyInfo_->castToKeyMdamGen() ? 
       keyInfo_->castToKeyMdamGen() :
       NULL);    
  }

  char * getTableName() { return tableName_; }

  Queue* getColFamNameList()       { return colFamNameList_; }
  Queue* listOfScanRows() const { return listOfScanRows_; }
  Queue* listOfGetRows() const { return listOfGetRows_; }
  Queue* listOfFetchedColNames() { return listOfFetchedColNames_; }
  Queue* listOfUpDeldColNames() { return listOfUpDeldColNames_; }
  Queue* listOfUpdatedColNames() { return listOfUpDeldColNames_; }
  Queue* listOfDeletedColNames() { return listOfUpDeldColNames_; }
  Queue* listOfMergedColNames() { return listOfMergedColNames_; }
  Queue* listOfIndexesAndTable() { return listOfIndexesAndTable_; }
  Queue* listOfOmittedColNames() { return listOfOmittedColNames_; }
  void setListOfIndexesAndTable(Queue* val) {listOfIndexesAndTable_ = val; }
  void setListOfOmittedColNames(Queue* val) {listOfOmittedColNames_ = val; }

  // overloading listOfUpdatedColNames and listOfMergedColNames...for now.
  Queue* listOfHbaseFilterColNames() { return listOfUpDeldColNames_; }
  Queue* listOfHbaseCompareOps() { return listOfMergedColNames_; }

  UInt32 convertRowLen() const { return convertRowLen_;}

  char * keyColName() { return keyColName_; }

  NABoolean isHbaseTimestampNeeded() { return (hbaseTimestampTuppIndex_ > 0); }
  NABoolean isHbaseVersionNeeded() { return (hbaseVersionTuppIndex_ > 0); }

  const char * server() { return server_; }
  const char * zkPort() { return zkPort_;}

  HbasePerfAttributes * getHbasePerfAttributes() 
  { return (HbasePerfAttributes*)hbasePerfAttributes_.getPointer();}
  HbasePerfAttributesPtr getHbasePerfAttributesPtr() { return hbasePerfAttributes_; }

  HbaseAccessOptions * getHbaseAccessOptions() 
  { return (HbaseAccessOptions*)hbaseAccessOptions_.getPointer(); }

  char * getPkeyColName() { return pkeyColName_; }

  NABoolean multiVersions() 
  {
    return 
      (getHbaseAccessOptions() ? getHbaseAccessOptions()->multiVersions() : FALSE);
  }
      
  UInt32 rowIdLen() { return rowIdLen_;}

  void setRowwiseFormat(NABoolean v)
  {(v ? flags_ |= ROWWISE_FORMAT : flags_ &= ~ROWWISE_FORMAT); };
  NABoolean rowwiseFormat() { return (flags_ & ROWWISE_FORMAT) != 0; };

  void setSubsetOper(NABoolean v)
  {(v ? flags_ |= SUBSET_OPER : flags_ &= ~SUBSET_OPER); };
  NABoolean subsetOper() { return (flags_ & SUBSET_OPER) != 0; };

  void setSQHbaseTable(NABoolean v)
  {(v ? flags_ |= SQ_HBASE : flags_ &= ~SQ_HBASE); };
  NABoolean sqHbaseTable() const { return (flags_ & SQ_HBASE) != 0; };

  // Hbase silently inserts a duplicate row. 
  // Hbase doesn't tell whether a row got deleted.
  // if set to ON, follow SQL semantics.
  // Return an error when a duplicate row is inserted.
  // Also, return indication whether a row got deleted or not.
  // This requires a check to be made before doing the IUD operation.
  void setHbaseSqlIUD(NABoolean v)
  {(v ? flags_ |= HBASE_SQL_IUD : flags_ &= ~HBASE_SQL_IUD); };
  NABoolean hbaseSqlIUD() const { return (flags_ & HBASE_SQL_IUD) != 0; };
 
  void setReturnRow(NABoolean v)
  {(v ? flags_ |= RETURN_ROW : flags_ &= ~RETURN_ROW); };
  NABoolean returnRow() { return (flags_ & RETURN_ROW) != 0; };

  void setComputeRowsAffected(NABoolean v)
  {(v ? flags_ |= COMPUTE_ROWS_AFFECTED : flags_ &= ~COMPUTE_ROWS_AFFECTED); };
  NABoolean computeRowsAffected() { return (flags_ & COMPUTE_ROWS_AFFECTED) != 0; };
 
  void setAddSyskeyTS(NABoolean v)
  {(v ? flags_ |= ADD_SYSKEY_TS : flags_ &= ~ADD_SYSKEY_TS); };
  NABoolean addSyskeyTS() { return (flags_ & ADD_SYSKEY_TS) != 0; };
  
  void setUniqueKeyInfo(NABoolean v)
  {(v ? flags_ |= UNIQUE_KEY_INFO : flags_ &= ~UNIQUE_KEY_INFO); };
  NABoolean uniqueKeyInfo() { return (flags_ & UNIQUE_KEY_INFO) != 0; };

  void setVsbbInsert(NABoolean v)
  {(v ? flags_ |= VSBB_INSERT : flags_ &= ~VSBB_INSERT); };
  NABoolean vsbbInsert() const { return (flags_ & VSBB_INSERT) != 0; };

  void setRowsetOper(NABoolean v)
  {(v ? flags_ |= ROWSET_OPER : flags_ &= ~ROWSET_OPER); };
  NABoolean rowsetOper() const { return (flags_ & ROWSET_OPER) != 0; };

  void setCanDoCheckAndUpdel(NABoolean v)
  {(v ? flags_ |= CHECK_AND_UPDEL : flags_ &= ~CHECK_AND_UPDEL); };
  NABoolean canDoCheckAndUpdel() { return (flags_ & CHECK_AND_UPDEL) != 0; };
 
 void setReadUncommittedScan(NABoolean v)
  {(v ? flags_ |= READ_UNCOMMITTED_SCAN : flags_ &= ~READ_UNCOMMITTED_SCAN); };
  NABoolean readUncommittedScan() { return (flags_ & READ_UNCOMMITTED_SCAN) != 0; };

 void setUpdelColnameIsStr(NABoolean v)
  {(v ? flags_ |= UPDEL_COLNAME_IS_STR : flags_ &= ~UPDEL_COLNAME_IS_STR); };
  NABoolean updelColnameIsStr() { return (flags_ & UPDEL_COLNAME_IS_STR) != 0;};

  void setUseHbaseXn(NABoolean v)
  {(v ? flags_ |= USE_HBASE_XN : flags_ &= ~USE_HBASE_XN); };
  NABoolean useHbaseXn() { return (flags_ & USE_HBASE_XN) != 0; };

  void setUseRegionXn(NABoolean v)
  {(v ? flags_ |= USE_REGION_XN : flags_ &= ~USE_REGION_XN); };
  NABoolean useRegionXn() { return (flags_ & USE_REGION_XN) != 0; };
 
  void setAlignedFormat(NABoolean v)
  {(v ? flags_ |= ALIGNED_FORMAT : flags_ &= ~ALIGNED_FORMAT); };
  NABoolean alignedFormat() { return (flags_ & ALIGNED_FORMAT) != 0; };

  void setHbaseMapTable(NABoolean v)
  {(v ? flags_ |= HBASE_MAP_TABLE : flags_ &= ~HBASE_MAP_TABLE); };
  NABoolean hbaseMapTable() { return (flags_ & HBASE_MAP_TABLE) != 0; };

  void setKeyInVCformat(NABoolean v)
  {(v ? flags_ |= KEY_IN_VC_FRMT : flags_ &= ~KEY_IN_VC_FRMT); };
  NABoolean keyInVCformat() { return (flags_ & KEY_IN_VC_FRMT) != 0; };

  void setAsyncOperations(NABoolean v)
  {(v ? flags_ |= ASYNC_OPERATIONS : flags_ &= ~ASYNC_OPERATIONS); };
  NABoolean asyncOperations() { return (flags_ & ASYNC_OPERATIONS) != 0; };

  void setUseCif(NABoolean v)
        {(v ? flags2_ |= USE_CIF : flags_ &= ~USE_CIF); };
  NABoolean getUseCif() { return (flags_ & USE_CIF) != 0; };

  void setCanAdjustTrafParams(NABoolean v)
   {(v ? flags2_ |= TRAF_UPSERT_ADJUST_PARAMS : flags2_ &= ~TRAF_UPSERT_ADJUST_PARAMS); };
   NABoolean getCanAdjustTrafParams() { return (flags2_ & TRAF_UPSERT_ADJUST_PARAMS) != 0; };

   void setIsTrafodionLoadPrep(NABoolean v)
    {(v ? flags2_ |= TRAF_LOAD_PREP : flags2_ &= ~TRAF_LOAD_PREP); };
    NABoolean getIsTrafodionLoadPrep() const { return (flags2_ & TRAF_LOAD_PREP) != 0; };

   void setWBSize(UInt32  v)
    {wbSize_ = v; };
    UInt32 getWBSize() { return wbSize_; };

   void setTrafWriteToWAL(NABoolean v)
     {(v ? flags2_ |= TRAF_UPSERT_WRITE_TO_WAL : flags2_ &= ~TRAF_UPSERT_WRITE_TO_WAL); };
   NABoolean getTrafWriteToWAL() { return (flags2_ & TRAF_UPSERT_WRITE_TO_WAL) != 0; };

  const char * getLoadPrepLocation() const { return LoadPrepLocation_; }
  void setLoadPrepLocation(char * loadPrepLocation) { LoadPrepLocation_ = loadPrepLocation;  }
  const char * getErrCountRowId() const { return errCountRowId_; }
  void setErrCountRowId(char * v) { errCountRowId_ = v; }
  const char * getErrCountTab() const { return errCountTab_; }
  void setErrCountTab(char * v) { errCountTab_ = v; }
  const char * getLoggingLocation() const { return loggingLocation_; }
  void setLoggingLocation(char * v) { loggingLocation_ = v; }

  const Float32 getSamplingRate() const
  {
    return samplingRate_;
  }

  void setSamplingRate(Float32 samplingRate)
  {
    samplingRate_ = samplingRate;
  }

  const char * getSampleLocation() const
  {
    return sampleLocation_;
  }

  void setSampleLocation(char * sampleLocation)
  {
    sampleLocation_ = sampleLocation;
  }

  UInt32 getRowLen()
  {
    UInt32 rowLen;

    switch (accessType_)
    {
       case MERGE_:
          if (mergeInsertRowLen_ > 0)
             rowLen =  mergeInsertRowLen_;
          else if (updateRowLen_ > 0)
            rowLen = updateRowLen_;
          else
             rowLen = convertRowLen_;
          break;
       case UPDATE_:
          rowLen =  updateRowLen_;
          break;
       default:
          rowLen = convertRowLen_;
          break;
    }
    assert(rowLen > 0);
    return rowLen;
  }

  UInt32 getRowIDLen()
  {
     if (keyLen_  > 0)
        return keyLen_;
     else
        return rowIdLen_;
  } 

  void setIsTrafLoadCleanup(NABoolean v)
   {(v ? flags2_ |= TRAF_LOAD_CLEANUP : flags2_ &= ~TRAF_LOAD_CLEANUP); };
  NABoolean getIsTrafLoadCleanup() { return (flags2_ & TRAF_LOAD_CLEANUP) != 0; };

   void setIsTrafLoadKeepHFiles(NABoolean v)
    {(v ? flags2_ |= TRAF_LOAD_KEEP_HFILES : flags2_ &= ~TRAF_LOAD_KEEP_HFILES); };
   NABoolean getIsTrafLoadKeepHFiles() { return (flags2_ & TRAF_LOAD_KEEP_HFILES) != 0; };

   void setIsTrafLoadCompetion(NABoolean v)
     {(v ? flags2_ |= TRAF_LOAD_COMPLETION : flags2_ &= ~TRAF_LOAD_COMPLETION); };
   NABoolean getIsTrafLoadCompletion() { return (flags2_ & TRAF_LOAD_COMPLETION) != 0; };

   void setQuasiSecure(NABoolean v)
     {(v ? flags2_ |= TRAF_LOAD_QUASI_SECURE : flags2_ &= ~TRAF_LOAD_QUASI_SECURE); };
   NABoolean getUseQuasiSecure() { return (flags2_ & TRAF_LOAD_QUASI_SECURE) != 0; };

   void setTakeSnapshot(NABoolean v)
     {(v ? flags2_ |= TRAF_LOAD_TAKE_SNAPSHOT : flags2_ &= ~TRAF_LOAD_TAKE_SNAPSHOT); };
   NABoolean getTakeSnapshot() { return (flags2_ & TRAF_LOAD_TAKE_SNAPSHOT) != 0; };

   void setNoDuplicates(NABoolean v)
     {(v ? flags2_ |= TRAF_LOAD_NO_DUPLICATTES : flags2_ &= ~TRAF_LOAD_NO_DUPLICATTES); };
   NABoolean getNoDuplicates() { return (flags2_ & TRAF_LOAD_NO_DUPLICATTES) != 0; };


   UInt32 getMaxHFileSize() const {
     return maxHFileSize_;
   }

   void setMaxHFileSize(UInt32 maxHFileSize) {
     maxHFileSize_ = maxHFileSize;
   }

   HbaseSnapshotScanAttributes * getHbaseSnapshotScanAttributes()
   { return (HbaseSnapshotScanAttributes*)hbaseSnapshotScanAttributes_.getPointer();}
   HbaseSnapshotScanAttributesPtr getHbaseSnapshotScanAttributesPtr() { return hbaseSnapshotScanAttributes_; }

   void setHbaseRowsetVsbbSize(UInt16 size)
   {  hbaseRowsetVsbbSize_ = size; }
   UInt16 getHbaseRowsetVsbbSize()
   { return hbaseRowsetVsbbSize_; } 

   void setTrafLoadFlushSize(UInt16 size)
   {  trafLoadFlushSize_ = size; }
   UInt16 getTrafLoadFlushSize()
   { return trafLoadFlushSize_; } 

   void setLogErrorRows(NABoolean v)
     {(v ? flags2_ |= TRAF_LOAD_LOG_ERROR_ROWS : flags2_ &= ~TRAF_LOAD_LOG_ERROR_ROWS); };
   NABoolean getLogErrorRows() { return (flags2_ & TRAF_LOAD_LOG_ERROR_ROWS) != 0; };

   void setContinueOnError(NABoolean v)
     {(v ? flags2_ |= TRAF_LOAD_CONTINUE_ON_ERROR : flags2_ &= ~TRAF_LOAD_CONTINUE_ON_ERROR); };
   NABoolean getContinueOnError() { return (flags2_ & TRAF_LOAD_CONTINUE_ON_ERROR) != 0; };

   UInt32 getMaxErrorRows() const{ return maxErrorRows_; }
   void setMaxErrorRows(UInt32 v ) { maxErrorRows_= v; }
  
   void setInsDelPreCondExpr(ExExprPtr exprPtr) {
        insDelPreCondExpr_ = exprPtr;
   }
   void setInsConstraintExpr(ExExprPtr exprPtr) {
      insConstraintExpr_ = exprPtr;
   }
   void setUpdConstraintExpr(ExExprPtr exprPtr) {
      updConstraintExpr_ = exprPtr;
   }
 protected:
  enum
  {
    ROWWISE_FORMAT                   = 0x0001,
    SUBSET_OPER                      = 0x0002,
    SQ_HBASE                         = 0x0004,
    HBASE_SQL_IUD                    = 0x0008,
    RETURN_ROW                       = 0x0010,
    COMPUTE_ROWS_AFFECTED            = 0x0020,
    ADD_SYSKEY_TS                    = 0x0040,
    UNIQUE_KEY_INFO                  = 0x0080,
    VSBB_INSERT                      = 0x0100,
    ROWSET_OPER                      = 0x0200,
    CHECK_AND_UPDEL                  = 0x0400,
    READ_UNCOMMITTED_SCAN            = 0x0800,
    UPDEL_COLNAME_IS_STR             = 0x1000,
    USE_HBASE_XN                     = 0x2000,
    ALIGNED_FORMAT                   = 0x4000,
    ASYNC_OPERATIONS                 = 0x8000,
    USE_CIF                          = 0x10000,
    HBASE_MAP_TABLE                  = 0x20000,
    KEY_IN_VC_FRMT                   = 0x40000,
    USE_REGION_XN                    = 0x80000
  };

  enum
  {
    TRAF_UPSERT_ADJUST_PARAMS        = 0x0001,
    TRAF_UPSERT_UNUSED               = 0x0002,
    TRAF_UPSERT_WRITE_TO_WAL         = 0x0004,
    TRAF_LOAD_PREP                   = 0x0008,
    TRAF_LOAD_COMPLETION             = 0x0010,
    TRAF_LOAD_CLEANUP                = 0x0020,
    TRAF_LOAD_KEEP_HFILES            = 0x0040,
    TRAF_LOAD_QUASI_SECURE           = 0x0080,
    TRAF_LOAD_TAKE_SNAPSHOT          = 0x0100,
    TRAF_LOAD_NO_DUPLICATTES         = 0x0200,
    TRAF_USE_SNAPSHOT_SCAN           = 0x0400,
    TRAF_LOAD_LOG_ERROR_ROWS         = 0x0800,
    TRAF_LOAD_CONTINUE_ON_ERROR      = 0x1000
  };

  UInt16 accessType_;
  UInt16 asciiTuppIndex_;
  UInt16 convertTuppIndex_;

  UInt16 updateTuppIndex_;
  UInt16 mergeInsertTuppIndex_;
  UInt16 mergeInsertRowIdTuppIndex_;
  UInt16 returnedFetchedTuppIndex_;
  UInt16 returnedUpdatedTuppIndex_;

  UInt16 rowIdTuppIndex_;
  UInt16 returnedTuppIndex_;
  UInt16 rowIdAsciiTuppIndex_;
  UInt16 keyTuppIndex_;
  UInt16 keyColValTuppIndex_;
  UInt16 hbaseFilterValTuppIndex_;

  UInt16 hbaseTimestampTuppIndex_;
  UInt16 hbaseVersionTuppIndex_;

  UInt32 asciiRowLen_;
  UInt32 convertRowLen_;
  UInt32 updateRowLen_;
  UInt32 mergeInsertRowLen_;
  UInt32 returnFetchedRowLen_;
  UInt32 returnUpdatedRowLen_;
  
  UInt32 rowIdLen_;
  UInt32 outputRowLen_;
  UInt32 rowIdAsciiRowLen_;
  UInt32 keyLen_;
  UInt32 keyColValLen_;
  UInt32 hbaseFilterValRowLen_;

  // expr to create the hbase row
  ExExprPtr convertExpr_;

  // Expression to filter rows.
  ExExprPtr scanExpr_;                                 

  ExExprPtr rowIdExpr_;

  ExExprPtr updateExpr_;

  ExExprPtr mergeInsertExpr_;
  ExExprPtr mergeInsertRowIdExpr_;

  ExExprPtr mergeUpdScanExpr_;

  ExExprPtr returnFetchExpr_;
  ExExprPtr returnUpdateExpr_;
  ExExprPtr returnMergeInsertExpr_;

  ExExprPtr encodedKeyExpr_;

  ExExprPtr keyColValExpr_;
  ExExprPtr insDelPreCondExpr_;
  ExExprPtr hbaseFilterExpr_;
  ExExprPtr insConstraintExpr_;
  ExExprPtr updConstraintExpr_;

  ExCriDescPtr workCriDesc_;      

  NABasicPtr tableName_;           

  QueuePtr colFamNameList_;

  QueuePtr listOfScanRows_;
  QueuePtr listOfGetRows_;
  QueuePtr listOfFetchedColNames_;
  QueuePtr listOfUpDeldColNames_;
  QueuePtr listOfMergedColNames_;
  QueuePtr listOfIndexesAndTable_; // used by bulk load
  QueuePtr listOfOmittedColNames_;

  // information about key ranges
  keyRangeGenPtr keyInfo_;                             

  NABasicPtr keyColName_;

  UInt32 flags_;
  UInt32 wbSize_;

  NABasicPtr server_;
  NABasicPtr zkPort_;

  HbasePerfAttributesPtr hbasePerfAttributes_;
  Float32 samplingRate_;
  NABasicPtr sampleLocation_;

  NABasicPtr LoadPrepLocation_;
  NABasicPtr errCountRowId_;
  NABasicPtr errCountTab_;
  NABasicPtr loggingLocation_;
  UInt32 flags2_;
  UInt32 maxHFileSize_;
  HbaseSnapshotScanAttributesPtr hbaseSnapshotScanAttributes_;
  UInt32 maxErrorRows_;
  UInt16 hbaseRowsetVsbbSize_; 
  UInt16 trafLoadFlushSize_; 
  HbaseAccessOptionsPtr hbaseAccessOptions_;

  // col name of primary key. Format colFam:colName.
  // Currently used for HbaseMapped tables with a single primary key.
  NABasicPtr pkeyColName_;

  char fillers[2];
};

class ComTdbHbaseCoProcAccess : public ComTdbHbaseAccess
{
 public:
  enum CoProcType
  {
    AGGR_
  };

  ComTdbHbaseCoProcAccess();

  ComTdbHbaseCoProcAccess(
			  char * tableName,
			  CoProcType type,

			  ex_expr * projExpr,
			  UInt32 projRowLen,
			  const UInt16 projTuppIndex,
			  const UInt16 returnedTuppIndex, 

			  Queue * listOfAggrColNames,

			  ex_cri_desc *workCriDesc,
			  ex_cri_desc *criDescParentDown,
			  ex_cri_desc *criDescParentUp,
			  queue_index queueSizeDown,
			  queue_index queueSizeUp,
			  Cardinality expectedRows,
			  Lng32 numBuffers,
			  ULng32 bufferSize,
			  char * server,
			  char * zkPort,
			  HbasePerfAttributes * hbasePerfAttributes
			  );

  CoProcType getCoProcType() { return (CoProcType)coProcType_;}

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

  virtual short getClassSize() { return (short)sizeof(ComTdbHbaseCoProcAccess); }

 private:
  Int16 coProcType_;

  char filler1_[6];
};

class ComTdbHbaseCoProcAggr : public ComTdbHbaseCoProcAccess
{
 public:
  enum AggrType
  {
    COUNT = 0,
    MIN = 1,
    MAX = 2,
    SUM = 3,
    AVG = 4
  };

  ComTdbHbaseCoProcAggr();

  ComTdbHbaseCoProcAggr(
			  char * tableName,

			  ex_expr * projExpr,
			  UInt32 projRowLen,
			  const UInt16 projTuppIndex,
			  const UInt16 returnedTuppIndex, 

			  Queue * listOfAggrTypes,
			  Queue * listOfAggrColNames,

			  ex_cri_desc *workCriDesc,
			  ex_cri_desc *criDescParentDown,
			  ex_cri_desc *criDescParentUp,
			  queue_index queueSizeDown,
			  queue_index queueSizeUp,
			  Cardinality expectedRows,
			  Lng32 numBuffers,
			  ULng32 bufferSize,
			  char * server,
			  char * zkPort,
			  HbasePerfAttributes * hbasePerfAttributes
			  );

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

  virtual short getClassSize() { return (short)sizeof(ComTdbHbaseCoProcAggr); }

  // Pack and Unpack routines
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  Queue* listOfAggrTypes() { return listOfAggrTypes_; }

 private:
  QueuePtr listOfAggrTypes_;
};

// --------------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for SqlTableOpenInfo
// --------------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<ComTdbHbaseAccess::HbaseScanRows> HbaseScanRowsPtr;

// --------------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for HbaseScanRowsPtr
// --------------------------------------------------------------------------
typedef
NAVersionedObjectPtrArrayTempl<HbaseScanRowsPtr> HbaseScanRowsPtrPtr;


#endif
