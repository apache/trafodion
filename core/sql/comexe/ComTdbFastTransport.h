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
* File:         ComTdbFastTransport.h
* Description:  TDB class for user-defined routines
*
* Created:      11/05/2012
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COM_TDB_FAST_TRANSPORT_H
#define COM_TDB_FAST_TRANSPORT_H

#include "ComTdb.h"
#include "ComQueue.h"

//
// Classes defined in this file
//

class  ComTdbFastExtract;


// ComTdbFastExtract class
//

class ComTdbFastExtract : public ComTdb
{
public:

  friend class ExFastExtractTdb;
  friend class ExFastExtractTcb;
  friend class ExHdfsFastExtractTcb;

  enum
  {
    TARGET_FILE                 = 0x0001,
    TARGET_SOCKET               = 0x0002,
    COMPRESS_LZO                = 0x0004,
    APPEND                      = 0x0008,
    INCLUDE_HEADER              = 0x0010,
    HIVE_INSERT                 = 0x0020,
    OVERWRITE_HIVE_TABLE        = 0x0040,
    SEQUENCE_FILE               = 0x0080,
    PRINT_DIAGS                 = 0x0100,
    HDFS_COMPRESSED             = 0x0200,
    CONTINUE_ON_ERROR           = 0x0400
  };

  ComTdbFastExtract ()
  : ComTdb(ex_FAST_EXTRACT, "eye_FAST_EXTRACT")
  {}

  ComTdbFastExtract (
    ULng32 flags,
    Cardinality estimatedRowCount,
    char * targetName,
    char * hdfsHost,
    Lng32 hdfsPort,
    char * hiveTableName,
    char * delimiter,
    char * header,
    char * nullString,
    char * recordSeparator,
    ex_cri_desc *criDescParent,
    ex_cri_desc *criDescReturned,
    ex_cri_desc *workCriDesc,
    queue_index downQueueMaxSize,
    queue_index upQueueMaxSize,
    Lng32 numOutputBuffers,
    ULng32 outputBufferSize,
    UInt16 numIOBuffers,
    UInt16 ioTimeout,
    ex_expr *inputExpr,
    ex_expr *outputExpr,
    ULng32 requestRowLen,
    ULng32 outputRowLen,
    ex_expr * childDataExprs,
    ComTdb * childTdb,
    Space *space,
    unsigned short childDataTuppIndex,
    unsigned short cnvChildDataTuppIndex,
    ULng32 cnvChildDataRowLen,
    Int64 hdfBuffSize,
    Int16 replication
    );

  virtual ~ComTdbFastExtract();

  //----------------------------------------------------------------------
  // Redefine virtual functions required for versioning
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
  virtual short getClassSize()
  {
    return (short) sizeof(ComTdbFastExtract);
  }

  //----------------------------------------------------------------------
  // Pack/unpack
  //----------------------------------------------------------------------
  Long pack(void *);
  Lng32 unpack(void *, void *);

  //----------------------------------------------------------------------
  // Other required TDB support functions
  //----------------------------------------------------------------------
  Int32 orderedQueueProtocol() const
  {
    return -1;
  }
  void display() const
  {
    // All TDBs have an no-op display() function. Not sure why.
  }
  virtual Int32 numChildren() const
  {
    return 1;
  }
  virtual const char *getNodeName() const
  {
    return "EX_FAST_EXTRACT";
  }
  virtual const ComTdb *getChild() const
  {
      return childTdb_;
  }

  virtual const ComTdb* getChild(Int32 pos) const
  {
    if (pos == 0)
      return childTdb_;
    else
      return NULL;
  }

  virtual Int32 numExpressions() const
  {
    return 3;
  }
  virtual const char *getExpressionName(Int32 pos) const
  {
    if (pos == 0)
      return "inputExpr_";
    else if (pos == 1)
      return "outputExpr_";
    else if (pos == 2)
      return( "childDataExpr_" );
    else
      return NULL;
  }

  virtual ex_expr *getExpressionNode(Int32 pos)
  {
    if (pos == 0)
      return inputExpr_;
    else if (pos == 1)
      return outputExpr_;
    else if (pos == 2)
      return childDataExpr_;
    else
      return NULL;
  }

  virtual void displayContents(Space *space, ULng32 flag);

  NABoolean getIsAppend() const
  {
    return ((flags_ & APPEND) != 0);
  }
  ;
  void setIsAppend(short value)
  {
    if (value)
      flags_ |= APPEND;
    else
      flags_ &= ~APPEND;
  }
  NABoolean getIsHiveInsert() const
  {
    return ((flags_ & HIVE_INSERT) != 0);
  }
  ;
  void setIsHiveInsert(short value)
  {
    if (value)
      flags_ |= HIVE_INSERT;
    else
      flags_ &= ~HIVE_INSERT;
  }
  NABoolean getIncludeHeader() const
  {
    return ((flags_ & INCLUDE_HEADER) != 0);
  }
  ;
  void setIncludeHeader(short value)
  {
    if (value)
      flags_ |= INCLUDE_HEADER;
    else
      flags_ &= ~INCLUDE_HEADER;
  }
  NABoolean getTargetFile() const
  {
    return ((flags_ & TARGET_FILE) != 0);
  }
  ;
  void setTargetFile(short value)
  {
    if (value)
    {
      flags_ |= TARGET_FILE;
      flags_ &= ~TARGET_SOCKET;
    }
    else
      flags_ &= ~TARGET_FILE;
  }
  NABoolean getTargetSocket() const
  {
    return ((flags_ & TARGET_SOCKET) != 0);
  }
  ;
  void setTargetSocket(short value)
  {
    if (value)
    {
      flags_ |= TARGET_SOCKET;
      flags_ &= ~TARGET_FILE;
    }
    else
      flags_ &= ~TARGET_SOCKET;
  }
  NABoolean getCompressLZO() const
  {
    return ((flags_ & COMPRESS_LZO) != 0);
  }
  ;
  void setCompressLZO(short value)
  {
    if (value)
      flags_ |= COMPRESS_LZO;
    else
      flags_ &= ~COMPRESS_LZO;
  }
  NABoolean getPrintDiags() const {return ((flags_ & PRINT_DIAGS) != 0);};
  void setPrintDiags(short value)
  {
	if (value)
	 flags_ |= PRINT_DIAGS;
	else
	 flags_ &= ~PRINT_DIAGS;
  }

  void setOverwriteHiveTable(short value)
  {
    if (value)
      flags_ |= OVERWRITE_HIVE_TABLE;
    else
      flags_ &= ~OVERWRITE_HIVE_TABLE;
  }
  NABoolean getOverwriteHiveTable() const
  {
    return ((flags_ & OVERWRITE_HIVE_TABLE) != 0);
  }
  ;

  void setSequenceFile(short value)
  {
    if (value)
      flags_ |= SEQUENCE_FILE;
    else
      flags_ &= ~SEQUENCE_FILE;
  }
  NABoolean getIsSequenceFile() const
  {
    return ((flags_ & SEQUENCE_FILE) != 0);
  }
  ;


  void setHdfsCompressed(UInt32 value)
  {
    if (value)
      flags_ |= HDFS_COMPRESSED;
    else
      flags_ &= ~HDFS_COMPRESSED;
  }
  NABoolean getHdfsCompressed() const
  {
    return ((flags_ & HDFS_COMPRESSED) != 0);
  }
  ;
  void setContinueOnError(NABoolean value)
  { value ? flags_ |= CONTINUE_ON_ERROR : flags_ &= ~CONTINUE_ON_ERROR; }
  NABoolean getContinueOnError() const
  { return ((flags_ & CONTINUE_ON_ERROR) != 0); }
  
  inline const char *getTargetName() const { return targetName_; }
  inline const char *getHdfsHostName() const { return hdfsHostName_; }
  inline const Int32 getHdfsPortNum() const {return (Int32)hdfsPortNum_;}
  inline const char *getHiveTableName() const { return hiveTableName_; }
  inline const char *getHeader() const { return header_; }
  inline const char *getNullString() const { return nullString_; }
  inline const char *getDelimiter() const { return delimiter_; }
  inline const char *getRecordSeparator() const
  { return recordSeparator_; }

  Int32 getNumIOBuffers() {return (Int32)numIOBuffers_;}
  Int32 getIoTimeout() {return (Int32)ioTimeout_;}
  Int64 getHdfsIoBufferSize() const
  {
    return hdfsIOBufferSize_;
  }

  void setHdfsIoBufferSize(Int64 hdfsIoBufferSize)
  {
    hdfsIOBufferSize_ = hdfsIoBufferSize;
  }

  Int16 getHdfsReplication() const
  {
    return hdfsReplication_;
  }

  void setHdfsReplication(Int16 hdfsReplication)
  {
    hdfsReplication_ = hdfsReplication;
  }
  UInt32 getChildDataRowLen() const
  {
    return childDataRowLen_;
  }

  void setModTSforDir(Int64 v) { modTSforDir_ = v; }
  Int64 getModTSforDir() const { return modTSforDir_; }

  void setHdfsIoByteArraySize(int size)             
    { hdfsIoByteArraySizeInKB_ = size; }                          
  Lng32 getHdfsIoByteArraySize()                             
    { return hdfsIoByteArraySizeInKB_; }
protected:
  NABasicPtr   targetName_;                                  // 00 - 07
  NABasicPtr   delimiter_;                                   // 08 - 15
  NABasicPtr   header_;                                      // 16 - 23
  NABasicPtr   nullString_;                                  // 24 - 31
  NABasicPtr   recordSeparator_;                             // 32 - 39
  ExExprPtr    inputExpr_;                                   // 40 - 47
  ExExprPtr    outputExpr_;                                  // 48 - 55
  ExExprPtr    childDataExpr_;                               // 56 - 63
  ComTdbPtr    childTdb_;                                    // 64 - 71
  ExCriDescPtr workCriDesc_;                                 // 72 - 79
  UInt32       flags_;                                       // 80 - 83
  UInt32       requestRowLen_;                               // 84 - 87
  UInt32       outputRowLen_;                                // 88 - 91
  UInt16       childDataTuppIndex_;                          // 92 - 93
  UInt16       cnvChildDataTuppIndex_;                       // 94 - 95
  UInt16       numIOBuffers_;				     // 96 - 97
  Int16        hdfsReplication_;                             // 98 - 99
  Int32        hdfsPortNum_;                                 // 100 - 103
  NABasicPtr   hiveTableName_;                               // 104 - 111
  Int64        hdfsIOBufferSize_;                            // 112 - 120
  NABasicPtr   hdfsHostName_  ;                              // 121 - 127
  UInt16       ioTimeout_;                                   // 128 - 129
  UInt16       filler_;                                      // 130 - 131
  UInt32       childDataRowLen_;                             // 132 - 135
  Int64        modTSforDir_;                                 // 136 - 143
  Lng32        hdfsIoByteArraySizeInKB_;                     // 144 - 147 

  // Make sure class size is a multiple of 8
  char fillerComTdbFastTransport_[4];                        // 148 - 151

};

#endif // COM_TDB_FAST_TRANSPORT_H

