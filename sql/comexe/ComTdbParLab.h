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
* File:         ComTdbParLab.h
* Description:  
*
* Created:      6/1/2005 
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMTDBPARLAB_H
#define COMTDBPARLAB_H

#include "Platform.h"
#include "ComSizeDefs.h"
#include "ComTdb.h"
#include "ComQueue.h" // for Queue


class ParallelLabelOpPartInfo : public NAVersionedObject
{
  public:
  ParallelLabelOpPartInfo(void);
  ~ParallelLabelOpPartInfo();

  virtual unsigned char getClassVersionID()
  {
     return 1;
  }

  virtual void populateImageVersionIDArray()
  {
     setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(ParallelLabelOpPartInfo); }
  
  Long pack(void *space)
  {
     guardianName_.pack(space);
     logicalPartName_.pack(space);
     lowKey_.pack(space);
     highKey_.pack(space);
     return NAVersionedObject::pack(space);
  }
 
  Lng32 unpack(void *base, void *reallocator)
  {
     if (guardianName_.unpack(base)) return -1;
     if (logicalPartName_.unpack(base)) return -1;
     if (lowKey_.unpack(base)) return -1;
     if (highKey_.unpack(base)) return -1;
     return NAVersionedObject::unpack(base, reallocator);
  }

  void setGuardianName(char *name) { guardianName_ = name; }
  char *fileName(void) { return guardianName_.getPointer(); }
  
  void setLogicalPartName(char *name) { logicalPartName_ = name; }
  char *logicalPartName(void) { return logicalPartName_.getPointer(); }

  void setPriExt(Lng32 ext) { priExt_ = ext; }
  Lng32 getPriExt(void) { return priExt_; }

  void setSecExt(Lng32 ext) { secExt_ = ext; }
  Lng32 getSecExt(void) { return secExt_; }

  void setMaxExt(Lng32 ext) { maxExt_ = ext; }
  Lng32 getMaxExt(void) { return maxExt_; }

  void setLowKey(char *key) { lowKey_ = key; }
  char *lowKey(void) { return lowKey_.getPointer(); }

  void setHighKey(char *key) { highKey_ = key; }
  char *highKey(void) { return highKey_.getPointer(); }

  private:
     
  NABasicPtr guardianName_;                // 00-07
  NABasicPtr logicalPartName_;             // 08-15
  NABasicPtr lowKey_;                      // 16-23
  NABasicPtr highKey_;                     // 24-31
  Int32 priExt_;                           // 32-35
  Int32 secExt_;                           // 36-39
  Int32 maxExt_;                           // 40-43
  char filler_[36];                        // 44-79
};

typedef NAVersionedObjectPtrTempl<ParallelLabelOpPartInfo> ParallelLabelOpPartInfoPtr;

class ComTdbParallelLabelOp: public ComTdb
{
  friend class ExParallelLabelOpTcb;
  friend class ExParallelLabelOpPrivateState;

  public:

  enum LabelOpType
  {
     DROP_OP       = 0x0001,
     CREATE_OP     = 0x0002,
     ALTER_OP      = 0x0003,
     DISK_COUNT_OP = 0x0004
  };

  // Dummy constructor - used by unpack routines.
  ComTdbParallelLabelOp();

  // Constructor
  ComTdbParallelLabelOp (const char* objectName,
                         short operation,
                         Lng32 numParts,
                         ParallelLabelOpPartInfo *partInfo,
			 short altOpCode,
			 Lng32 timeout,
                         ex_cri_desc * given_cri_desc,
                         ex_cri_desc * returned_cri_desc,
                         queue_index upQueueSize,
                         queue_index downQueueSize,
                         Lng32 numBuffers,
                         ULng32 bufferSize);

  // destructor
  virtual ~ComTdbParallelLabelOp();

  Int32 orderedQueueProtocol() const { return -1; }

  // Redefine virtual functions required for versioning.
  virtual unsigned char getClassVersionID() { return 1; }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }
 
  virtual short getClassSize() { return (short)sizeof(ComTdbParallelLabelOp); }
  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void *reallocator);
 
  // this is a leaf node, i.e., no children 
  virtual const ComTdb *getChild(Int32 pos) const { return NULL; }
  virtual Int32 numChildren() const { return 0; } 

  virtual const char *getNodeName() const { return "EX_PARALLEL_LABEL_OP_TDB"; }
  virtual Int32 numExpressions() const { return 0; }

  virtual ex_expr *getExpressionNode(Int32 pos) { return NULL; }
  virtual const char *getExpressionName(Int32 pos) const { return NULL; }

  void displayContents(Space *space, ULng32 flag);

  inline ParallelLabelOpPartInfo * partInfo(Lng32 i) { return &partInfoArray_[i]; }
  inline Lng32 numParts(void) { return numParts_; }
  inline short getOperation(void) { return operation_; }

  inline UInt32 getAltOpCode(void) { return altOpCode_; }

  Lng32 getTimeout() { return timeout_; }

  protected:

  NABasicPtr objectName_;                     // 00 - 07
  char filler1_[8];                           // 08 - 15
  ParallelLabelOpPartInfoPtr partInfoArray_;  // 16 - 23
  Lng32  numParts_;                            // 24 - 27 
  short operation_;			      // 28 - 29
  short altOpCode_;			      // 30 - 31
  Lng32  timeout_;                             // 32 - 35
  char fillerComTdbParLabOp_[44];             // 36 - 79 
};


static const ComTdbVirtTableColumnInfo comTdbParLabDiskCountVirtTableColumnInfo[] =
  {
    { "CATALOG_NAME",         0,          COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,  ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES, FALSE, SQLCHARSETCODE_UTF8 ,0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "SCHEMA_NAME",        1,            COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,  ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES, FALSE, SQLCHARSETCODE_UTF8 ,0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "OBJECT_NAME",         2,           COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,  ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES, FALSE, SQLCHARSETCODE_UTF8 ,0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "ANSI_OBJECT_TYPE" ,    3,          COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "ANSI_NAME_SPACE",       4,        COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,  4 , FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "PARTITION_NAME",         5,      COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   36, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "PARTITION_NUM",           6,     COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,  4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "ROW_COUNT",                 7,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "INSERTED_ROW_COUNT", 8,           COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "DELETED_ROW_COUNT",   9,         COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "UPDATED_ROW_COUNT",   10,         COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "PRIMARY_EXTENTS",         11,   COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "SECONDARY_EXTENTS",    12,          COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "MAX_EXTENTS",                13,    COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "ALLOCATED_EXTENTS",     14,         COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CURRENT_EOF",                 15,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CREATE_TIME",                  16,  COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "LAST_MOD_TIME",              17,    COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "LAST_OPEN_TIME",             18,    COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "FILECODE",                         19,  COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "RECORD_LEN",                   20,  COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CORRUPT_BROKEN",          21,       COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CRASH_OPEN",                   22,  COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "RFORK_EOF",                     23, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "ACCESS_COUNTER",           24, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,         8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COMPRESSION_TYPE",        25,   COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COMPRESSION_TYPE_STR", 26,          COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,    28, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COMPRESSED_EOF_SECTORS", 27,        COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,    8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COMPRESSION_RATIO",        28,      COM_USER_COLUMN_LIT, REC_FLOAT32,   4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT, 0}
                     

  };

struct ComTdbParLabDiskCountVirtTableColumnStruct
{
  char   catalogName_[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES];
  char   schemaName_[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES];
  char   objectName_[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES];
  char   ansiObjectType_[4];
  char   ansiNameSpace_[4];
  char   partitionName_[36];
  Int32  partitionNum_;
  Int64  rowCount_;
  Int64  insertedRowCount_;
  Int64  deletedRowCount_;
  Int64  updatedRowCount_;
  Int32  primaryExtents_;
  Int32  secondaryExtents_;
  Int32  maxExtents_;
  Int32  allocatedExtents_;
  Int64  currentEOF_;
  Int64  createTime_;
  Int64  lastModTime_;
  Int64  lastOpenTime_;
  Int32  fileCode_;
  Int32  recLen_;
  Int32  corruptBroken_;
  Int32  crashOpen_;
  Int64 rforkEOF_;
  Int64 accessCounter_;
  Int32  compressionType_; 
   char  compressionTypeStr_[28]; 
  Int64  compressedEofSectors_;
  Float32 compressionRatio_;
 
 

};


class ComTdbDiskLabelStatistics: public ComTdbParallelLabelOp
{
  friend class ExDiskLabelStatisticsTcb;
  friend class ExParallelLabelOpPrivateState;
  
public:

  ComTdbDiskLabelStatistics()
    : ComTdbParallelLabelOp()
  {};
  
  // Constructor
  ComTdbDiskLabelStatistics (const char* objectName,
			     Queue * tableOpenInfoList,
			     Lng32 returnedRowlen,
			     Lng32 numParts,
			     ParallelLabelOpPartInfo *partInfo,
			     Lng32 timeout,
			     ex_cri_desc * given_cri_desc,
			     ex_cri_desc * returned_cri_desc,
			     queue_index upQueueSize,
			     queue_index downQueueSize,
			     Lng32 numBuffers,
			     ULng32 bufferSize);
  
  virtual short getClassSize() { return (short)sizeof(ComTdbDiskLabelStatistics); }
  
  static Int32 getVirtTableNumCols()
  {
    return sizeof(comTdbParLabDiskCountVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void *reallocator);

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)comTdbParLabDiskCountVirtTableColumnInfo;
  }

  static Int32 getVirtTableNumKeys()
  {
    return 0;
  }

  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return NULL;
  }

  Lng32 getReturnedRowLength(){ return returnedRowLen_; }
  UInt16 getTuppIndex() { return tuppIndex_; }

  NABoolean rowcountsOnly() { return (flags_ & ROWCOUNTS_ONLY) != 0; }
  void setRowcountsOnly(NABoolean v)      
           { (v ? flags_ |= ROWCOUNTS_ONLY : flags_ &= ~ROWCOUNTS_ONLY); }
  
  NABoolean resetAccessCounter() { return (flags_ & RESET_ACCESS_COUNTER) != 0; }
  void setResetAccessCounter(NABoolean v)      
           { (v ? flags_ |= RESET_ACCESS_COUNTER : flags_ &= ~RESET_ACCESS_COUNTER);
	   }
private:
  enum Flags
  {
    ROWCOUNTS_ONLY = 0x0001,
    RESET_ACCESS_COUNTER = 0x0002
  };

  QueuePtr tableOpenInfoList_;                          // 00-07
  
  Lng32   returnedRowLen_;                               // 08-11
  UInt16 tuppIndex_;                                    // 12-13
  UInt16 flags_;                                        // 14-15
};

#endif
