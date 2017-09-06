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
#include "hdfs.h"  // tPort 

//
// Task Definition Block
//
class ComTdbBlockingHdfsScan : public ComTdb
{
  friend class ExBlockingHdfsScanTcb;
  friend class ExHdfsScanTcb;
  friend class ExBlockingHdfsScanPrivateState;

protected:
  enum
  {
    BLOCKING_OPER    = 0x0001
  };

  // Expression to filter rows.
  ExExprPtr selectPred_;                                     // 00 - 07

  // Expression to move selected rows into buffer pool.
  ExExprPtr moveExpr_;                                       // 08 - 15

  // Convert HDFS row from exploded format with all types in ascii 
  // to aligned format with the correct binary type for each column 
  ExExprPtr convertExpr_;                                    // 16 - 23

  NABasicPtr hostName_;                                      // 24 - 31

  // For now, a simple name. Partitioning handled elsewhere.
  NABasicPtr hdfsFileName_;                                  // 32 - 39

  ExCriDescPtr workCriDesc_;                                 // 40 - 47

  // Start reading the first row from this offset.
  Int64 hdfsOffset_;                                         // 48 - 55

  // Stop reading at this length, plus however many 
  // bytes it takes to complete the final record.    
  Int64 hdfsLength_;                                         // 56 - 63    

  // max length of the row converted from hdfs to SQL format, before 
  // projection.
  Int64 hdfsSqlMaxRecLen_;                                   // 64 - 71

  // max length of output row
  Int64 outputRowLength_;                                    // 72 - 79

  // index into atp of output row tupp
  UInt16 tuppIndex_;                                         // 80 - 81

  // hadoop port num. An unsigned short in hdfs.h, subject to change.
  UInt16 port_;

  UInt16 workAtpIndex_;                                      // 82 - 83

  // offsets for hdfs Aligned format row 
  UInt16 bitmapEntryOffset_;                                 // 84 - 85
  UInt16 bitmapOffset_;                                      // 86 - 87
  UInt16 firstFixedOffset_;                                  // 88 - 89   

  char recordDelimiter_;                                     // 90 - 90

  char columnDelimiter_;                                     // 91 - 91

  UInt16 flags_;                                                    // 92-93

  // index into atp of source ascii row(hive, hdfs) which will be converted to sql row
  UInt16 asciiTuppIndex_;                                         // 94 - 95

  Int64 asciiRowLen_;                                            // 96 - 103
  char fillersComTdbBlockingHdfsScan1_[8];        // 104 - 111

public:
  // Constructor
  ComTdbBlockingHdfsScan(); // dummy constructor. Used by 'unpack' routines.
  
  ComTdbBlockingHdfsScan(
                 ex_expr * select_pred,
		 ex_expr * move_expr,
                 ex_expr * convert_expr,
                 char * hdfsHostName,
                 tPort  hdfsPort, 
                 char * hdfsFileName, 
                 char recordDelimiter,
                 char columnDelimiter,
		 Int64 hdfsOffset,
		 Int64 hdfsLength,
		 Int64 hdfsSqlMaxRecLen,
                 Int64 outputRowLength,
		 Int64 asciiRowLen,
		 const unsigned short tuppIndex,
		 const unsigned short asciiTuppIndex,
		 const unsigned short workAtpIndex,
		 ex_cri_desc * work_cri_desc,
		 ex_cri_desc * given_cri_desc,
		 ex_cri_desc * returned_cri_desc,
		 queue_index down,
		 queue_index up,
		 Cardinality estimatedRowCount,
                 Int32  numBuffers,
                 UInt32  bufferSize
                 );

  ~ComTdbBlockingHdfsScan();

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

  virtual short getClassSize() { return (short)sizeof(ComTdbBlockingHdfsScan); }  
  Long pack(void *);

  Lng32 unpack(void *, void * reallocator);
  
  void display() const;

  inline ComTdb * getChildTdb();

  Int32 orderedQueueProtocol() const;

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  virtual void displayContents(Space *space,ULng32 flag);

  virtual const ComTdb* getChild(Int32 pos) const;

  virtual Int32 numChildren() const { return 0; }

  virtual const char *getNodeName() const { return "EX_HDFS_SCAN"; };

  virtual Int32 numExpressions() const { return 3; }

  virtual ex_expr* getExpressionNode(Int32 pos) {
     if (pos == 0)
	return selectPred_;
     else if (pos == 1)
	return moveExpr_;
     else if (pos == 2)
       return convertExpr_;
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
     else
	return NULL;
  }
  
  ExpTupleDesc *getHdfsSqlRowDesc() const
  {
    workCriDesc_->getTupleDescriptor(workAtpIndex_);
  }

  void setBlockingOper(NABoolean v)
  {(v ? flags_ |= BLOCKING_OPER : flags_ &= ~BLOCKING_OPER); };
  NABoolean blockingOper() { return (flags_ & BLOCKING_OPER) != 0; };

  ExpTupleDesc *getHdfsAsciiRowDesc() const
  {
    workCriDesc_->getTupleDescriptor(workAtpIndex_+1);
  }
  
  UInt16 bitmapEntryOffset()
  {
    return bitmapEntryOffset_;// offset to write bitmap offset to
  }
  void setBitmapEntryOffset(UInt16 v)
  {
    bitmapEntryOffset_ = v;// offset to write bitmap offset to
  }

  UInt16 bitmapOffset()
  {
    return bitmapOffset_;// bitmap offset value
  }
  void setBitmapOffset(UInt16 v)
  {
    bitmapOffset_ = v;// bitmap offset value
  }

  UInt16 firstFixedOffset()
  {
    return firstFixedOffset_;// first fixed field offset
  }
  void setFirstFixedOffset(UInt16 v )
  {
    firstFixedOffset_ = v;
  }
  
};

inline ComTdb * ComTdbBlockingHdfsScan::getChildTdb()
{
  return NULL;
};

/*****************************************************************************
  Description : Return ComTdb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
  History     : Yeogirl Yun                                      8/22/95
                 Initial Revision.
*****************************************************************************/
inline const ComTdb* ComTdbBlockingHdfsScan::getChild(Int32 pos) const
{
  return NULL;
};

#endif
  
