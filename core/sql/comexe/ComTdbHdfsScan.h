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
#ifndef COM_HDFS_SCAN_H
#define COM_HDFS_SCAN_H

#include "ComTdb.h"
#include "ExpLOBinterface.h"
#include "ComQueue.h"

//
// Task Definition Block
//
class ComTdbHdfsScan : public ComTdb
{
  friend class ExHdfsScanTcb;
  friend class ExHdfsScanPrivateState;
  friend class ExOrcScanTcb;
  friend class ExOrcScanPrivateState;
  friend class ExOrcFastAggrTcb;
  friend class ExOrcFastAggrPrivateState;

 protected:
  enum
  {
    USE_CURSOR_MULTI            = 0x0001,
    DO_SPLIT_FILE_OPT           = 0x0002,
    HDFS_PREFETCH               = 0x0004,
    // flag to indicate whther we need to use Compressed Internal Format
    USE_CIF                     = 0x0008,
    // flag to indicate whther we need to use Defragmentation  with Compressed Internal Format
    USE_CIF_DEFRAG              = 0x0010,

    // ignore conversion errors and continue reading the next row.
    CONTINUE_ON_ERROR           = 0x0020,
    LOG_ERROR_ROWS              = 0x0040,
    ASSIGN_RANGES_AT_RUNTIME    = 0x0080,
    TREAT_EMPTY_AS_NULL         = 0x0100,
    USE_LIBHDFS_SCAN            = 0x0200,
    COMPRESSED_FILE             = 0x0400
  };

  // Expression to filter rows.
  ExExprPtr selectPred_;                                     // 00 - 07

  // Expression to move selected rows into buffer pool.
  ExExprPtr moveExpr_;                                       // 08 - 15

  // Convert selection pred cols in HDFS row from exploded format 
  // with all types in ascii to aligned format with the correct 
  // binary type for each column 
  ExExprPtr convertExpr_;                                    // 16 - 23

  // Convert move expr only cols in HDFS row from exploded format 
  // with all types in ascii to aligned format with the correct 
  // binary type for each column 
  ExExprPtr moveColsConvertExpr_;                            // 24 - 31

  NABasicPtr hostName_;                                      // 32 - 39

  ExCriDescPtr workCriDesc_;                                 // 40 - 47

  Int64 moveExprColsRowLength_;                              // 48 - 55
  UInt16 moveExprColsTuppIndex_;                             // 56 - 57

  short type_;                                      
  char filler1_[4];                                          // 58 - 63

  // I/O buffer for interface to hdfs access layer.
  Int64 hdfsBufSize_;                                        // 64 - 71

  // max length of the row converted from hdfs to SQL format, before 
  // projection.
  Int64 hdfsSqlMaxRecLen_;                                   // 72 - 79

  // max length of output row
  Int64 outputRowLength_;                                    // 80 - 87

  // index into atp of output row tupp
  UInt16 tuppIndex_;                                         // 88 - 89

  UInt16 workAtpIndex_;                                      // 90 - 91

  char recordDelimiter_;                                     // 92 - 92

  char columnDelimiter_;                                     // 93 - 93

  // index into atp of source ascii row(hive, hdfs) which will be converted to sql row
  UInt16 asciiTuppIndex_;                                    // 94 - 95

  UInt32 flags_;                                             // 96 - 99

  UInt16 port_;                                              // 100 - 101

  UInt16 convertSkipListSize_;                               // 102 - 103
  Int16Ptr convertSkipList_;                                 // 104 - 111

  Int64 asciiRowLen_;                                        // 112 - 119

  // cumulative list of all scanInfo entries for all esps (nodeMap entries)
  QueuePtr hdfsFileInfoList_;                                // 120-127

  // range of files in hdfsFileInfoList_ that each esp need to read from.
  // start at 'Begin' entry and read 'Num' entries.
  // At runtime, each esp will pick one of the ranges.
  // Later, we may align esps with the local files.
  QueuePtr hdfsFileRangeBeginList_;                          // 128 - 135
  QueuePtr hdfsFileRangeNumList_;                            // 136 - 143

  NABasicPtr tableName_;                                     // 144 - 151
  UInt32 rangeTailIOSize_;                                   // 152 - 155
  UInt32 maxErrorRows_;                                      // 156 - 159
  NABasicPtr errCountTable_;                                  // 160 - 167
  NABasicPtr loggingLocation_;                                // 168 - 175
  NABasicPtr errCountRowId_;                                  // 176 - 183
  UInt32  hiveScanMode_;                                      // 184 - 187
  UInt16 origTuppIndex_;                                      // 188 - 189
  char fillersComTdbHdfsScan1_[2];                            // 190 - 191
  NABasicPtr nullFormat_;                                     // 192 - 199

  // next 4 params are used to check if data under hdfsFileDir
  // was modified after query was compiled.
  NABasicPtr hdfsRootDir_;                                     // 200 - 207
  Int64  modTSforDir_;                                         // 208 - 215
  Lng32  numOfPartCols_;                                       // 216 - 219
  Lng32  hdfsIoByteArraySizeInKB_;                             // 220 - 223
  QueuePtr hdfsDirsToCheck_;                                   // 224 - 231
    
public:
  enum HDFSFileType
  {
    UNKNOWN_ = 0,
    TEXT_ = 1,
    SEQUENCE_ = 2,
    ORC_ = 3
  };

  // Constructor
  ComTdbHdfsScan(); // dummy constructor. Used by 'unpack' routines.
  
  ComTdbHdfsScan(
		 char * tableName,
                 short type,
                 ex_expr * select_pred,
		 ex_expr * move_expr,
                 ex_expr * convert_expr,
                 ex_expr * move_convert_expr,
                 UInt16 convertSkipListSize,
                 Int16 * convertSkipList,
                 char * hdfsHostName,
                 tPort  hdfsPort, 
		 Queue * hdfsFileInfoList,
		 Queue * hdfsFileRangeBeginList,
		 Queue * hdfsFileRangeNumList,
                 char recordDelimiter,
                 char columnDelimiter,
                 char * nullFormat,
		 Int64 hdfsBufSize,
                 UInt32 rangeTailIOSize,
		 Int64 hdfsSqlMaxRecLen,
                 Int64 outputRowLength,
		 Int64 asciiRowLen,
                 Int64 moveColsRowLen,
		 const unsigned short tuppIndex,
		 const unsigned short asciiTuppIndex,
		 const unsigned short workAtpIndex,
                 const unsigned short moveColsTuppIndex,
                 const unsigned short origTuppIndex,
		 ex_cri_desc * work_cri_desc,
		 ex_cri_desc * given_cri_desc,
		 ex_cri_desc * returned_cri_desc,
		 queue_index down,
		 queue_index up,
		 Cardinality estimatedRowCount,
                 Int32  numBuffers,
                 UInt32  bufferSize,

                 char * errCountTable = NULL,
                 char * loggingLocation = NULL,
                 char * errCountId = NULL,

                 // next 4 params are used to check if data under hdfsFileDir
                 // was modified after query was compiled.
                 char * hdfsRootDir  = NULL,
                 Int64  modTSforDir   = -1,
                 Lng32  numOfPartCols = -1,
                 Queue * hdfsDirsToCheck = NULL
                 );

  ~ComTdbHdfsScan();

  //----------------------------------------------------------------------
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

  virtual short getClassSize() { return (short)sizeof(ComTdbHdfsScan); }  
  Long pack(void *);

  Lng32 unpack(void *, void * reallocator);
  
  void display() const;

  inline ComTdb * getChildTdb();

  Int32 orderedQueueProtocol() const;

  char * tableName() { return tableName_; }
  char * getErrCountTable() { return errCountTable_; }
  void   setErrCountTable(char * v ) { errCountTable_ = v; }
  char * getLoggingLocation() { return loggingLocation_; }
  void   setLoggingLocation(char * v ) { loggingLocation_ = v; }
  char * getErrCountRowId() { return errCountRowId_; }
  void   setErrCountRowId(char * v ) { errCountRowId_ = v; }
  void   setHiveScanMode(UInt32 v ) { hiveScanMode_ = v; }
  UInt32 getHiveScanMode() { return hiveScanMode_; }

  Queue* getHdfsFileInfoList() const {return hdfsFileInfoList_;}
  Queue* getHdfsFileRangeBeginList() {return hdfsFileRangeBeginList_;}
  Queue* getHdfsFileRangeNumList() {return hdfsFileRangeNumList_;}

  char * getNullFormat() { return nullFormat_; }

  const NABoolean isTextFile() const { return (type_ == TEXT_);}
  const NABoolean isSequenceFile() const { return (type_ == SEQUENCE_);}  
  const NABoolean isOrcFile() const { return (type_ == ORC_);}

  void setUseCursorMulti(NABoolean v)
  {(v ? flags_ |= USE_CURSOR_MULTI : flags_ &= ~USE_CURSOR_MULTI); };
  NABoolean useCursorMulti() { return (flags_ & USE_CURSOR_MULTI) != 0; };

  void setDoSplitFileOpt(NABoolean v)
  {(v ? flags_ |= DO_SPLIT_FILE_OPT : flags_ &= ~DO_SPLIT_FILE_OPT); };
  NABoolean doSplitFileOpt() { return (flags_ & DO_SPLIT_FILE_OPT) != 0; };

  void setHdfsPrefetch(NABoolean v)
  {(v ? flags_ |= HDFS_PREFETCH : flags_ &= ~HDFS_PREFETCH); };
  NABoolean hdfsPrefetch() { return (flags_ & HDFS_PREFETCH) != 0; };

  void setUseCif(NABoolean v)
  {(v ? flags_ |= USE_CIF : flags_ &= ~USE_CIF); };
  NABoolean useCif() { return (flags_ & USE_CIF) != 0; };
  
  void setUseCifDefrag(NABoolean v)
  {(v ? flags_ |= USE_CIF_DEFRAG : flags_ &= ~USE_CIF_DEFRAG); };
  NABoolean useCifDefrag() { return (flags_ & USE_CIF_DEFRAG) != 0; };

  void setContinueOnError(NABoolean v)
  {(v ? flags_ |= CONTINUE_ON_ERROR : flags_ &= ~CONTINUE_ON_ERROR); };
  NABoolean continueOnError() { return (flags_ & CONTINUE_ON_ERROR) != 0; };
  
  void setLogErrorRows(NABoolean v)
  {(v ? flags_ |= LOG_ERROR_ROWS : flags_ &= ~LOG_ERROR_ROWS); };
  NABoolean getLogErrorRows() { return (flags_ & LOG_ERROR_ROWS) != 0; };
  
  void setAssignRangesAtRuntime(NABoolean v)
  {(v ? flags_ |= ASSIGN_RANGES_AT_RUNTIME : flags_ &= ~ASSIGN_RANGES_AT_RUNTIME); }
  NABoolean getAssignRangesAtRuntime() const
                                { return (flags_ & ASSIGN_RANGES_AT_RUNTIME) != 0; }

  void setUseLibhdfsScan(NABoolean v)
  {(v ? flags_ |= USE_LIBHDFS_SCAN : flags_ &= ~USE_LIBHDFS_SCAN); }
  NABoolean getUseLibhdfsScan() const
                                { return (flags_ & USE_LIBHDFS_SCAN) != 0; }

  void setCompressedFile(NABoolean v)
  {(v ? flags_ |= COMPRESSED_FILE : flags_ &= ~COMPRESSED_FILE); }
  NABoolean isCompressedFile() const
                                { return (flags_ & COMPRESSED_FILE) != 0; }

  UInt32 getMaxErrorRows() const { return maxErrorRows_;}
  void setMaxErrorRows(UInt32 v ) { maxErrorRows_= v; }
  
  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  virtual void displayContents(Space *space,ULng32 flag);

  virtual const ComTdb* getChild(Int32 pos) const;

  virtual Int32 numChildren() const { return 0; }

  virtual const char *getNodeName() const { return "EX_HDFS_SCAN"; };

  virtual Int32 numExpressions() const { return 4; }

  virtual ex_expr* getExpressionNode(Int32 pos) {
     if (pos == 0)
	return selectPred_;
     else if (pos == 1)
	return moveExpr_;
     else if (pos == 2)
       return convertExpr_;
     else if (pos == 3)
       return moveColsConvertExpr_;
     else
       return NULL;
  }
  
  virtual const char * getExpressionName(Int32 pos) const {
     if (pos == 0)
	return "selectExpr_";
     else if (pos == 1)
	return "moveExpr_";
     else if (pos == 2)
       return "convertExpr_";
     else if (pos ==3)
       return "moveColsConvertExpr_";

     else
	return NULL;
  }
  
  ExpTupleDesc *getHdfsSqlRowDesc() const
  {
    return workCriDesc_->getTupleDescriptor(workAtpIndex_);
  }

  ExpTupleDesc *getHdfsAsciiRowDesc() const
  {
    return workCriDesc_->getTupleDescriptor(asciiTuppIndex_);
  }

  ExpTupleDesc *getHdfsOrigRowDesc() const
  {
    return workCriDesc_->getTupleDescriptor(origTuppIndex_);
  }

  ExpTupleDesc *getMoveExprColsRowDesc() const
  {
    return workCriDesc_->getTupleDescriptor(moveExprColsTuppIndex_);
  }

  Queue * hdfsDirsToCheck() { return hdfsDirsToCheck_; }
 
  char *hdfsRootDir() { return hdfsRootDir_; }
  void setHdfsIoByteArraySize(int size)
    { hdfsIoByteArraySizeInKB_ = size; }
  int getHdfsIoByteArraySize() 
    { return hdfsIoByteArraySizeInKB_; }
};

inline ComTdb * ComTdbHdfsScan::getChildTdb()
{
  return NULL;
};

/*****************************************************************************
  Description : Return ComTdb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
*****************************************************************************/
inline const ComTdb* ComTdbHdfsScan::getChild(Int32 pos) const
{
  return NULL;
};

// ComTdbOrcFastAggr 
class ComTdbOrcFastAggr : public ComTdbHdfsScan
{
  friend class ExOrcFastAggrTcb;
  friend class ExOrcFastAggrPrivateState;

public:
  enum OrcAggrType
  {
    UNKNOWN_ = 0,
    COUNT_      = 1,
    MIN_           = 2,
    MAX_          = 3
  };

  // Constructor
  ComTdbOrcFastAggr(); // dummy constructor. Used by 'unpack' routines.
  
  ComTdbOrcFastAggr(
                char * tableName,
                OrcAggrType type,
                Queue * hdfsFileInfoList,
                Queue * hdfsFileRangeBeginList,
                Queue * hdfsFileRangeNumList,
                ex_expr * proj_expr,
                Int64 projRowLen,
                const unsigned short projTuppIndex,
                const unsigned short returnedTuppIndex,
                ex_cri_desc * work_cri_desc,
                ex_cri_desc * given_cri_desc,
                ex_cri_desc * returned_cri_desc,
                queue_index down,
                queue_index up,
                Int32  numBuffers,
                UInt32  bufferSize
                );
  
  ~ComTdbOrcFastAggr();

  virtual short getClassSize() { return (short)sizeof(ComTdbOrcFastAggr); }  

  virtual const char *getNodeName() const { return "EX_ORC_FAST_AGGR"; };

 private:
   OrcAggrType type_;
};

#endif
  
