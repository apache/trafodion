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
* File:         PartInputDataDesc.h
* Description:  Data structures related to generating partition input
*               tuples for ESPs and partitioned access nodes.
*               
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef PART_INPUT_DATA_DESC_H
#define PART_INPUT_DATA_DESC_H

#include "NAVersionedObject.h"
#include "ExpCriDesc.h"
#include "ex_expr.h"

// -----------------------------------------------------------------------
class ex_cri_desc;
class ComDiagsArea;
class ex_expr;

// -----------------------------------------------------------------------
// Generated part of the partition descriptor
// -----------------------------------------------------------------------

class ExPartInputDataDesc : public NAVersionedObject
{
public:

  ExPartInputDataDesc() : NAVersionedObject(-1)
  {}

  virtual ~ExPartInputDataDesc() {};

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual char *findVTblPtr(short classID);

  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(ExPartInputDataDesc); }

  enum ExPartitioningType
    {
      HASH_PARTITIONED  = 1,
      RANGE_PARTITIONED = 2,
      ROUNDROBIN_PARTITIONED = 3,
      HASH1_PARTITIONED = 4,
      HASH2_PARTITIONED = 5
    };

  inline ExPartitioningType getPartitioningType() const
                                 { return (ExPartitioningType) partType_; }
  inline ex_cri_desc *getPartInputCriDesc()
                                              { return partInputCriDesc_; }
  inline Lng32 getPartInputDataLength() const
                                           { return partInputDataLength_; }
  inline Lng32 getNumPartitions() const
                                                 { return numPartitions_; }

  // copy the partition input value for partition range <fromPartNum> to
  // (and including) <toPartNum> into buffer <buffer> of length <bufferLength>
  // (must be at least <partInputDataLength_>)
  virtual void copyPartInputValue(Lng32 fromPartNum,
				  Lng32 toPartNum,
				  char *buffer,
				  Lng32 bufferLength) {}

  void fixupVTblPtr();
  virtual Long pack(void * space);
  virtual Lng32 unpack(void *base, void * reallocator);
  virtual Lng32 evalExpressions(Space * space,
			       CollHeap * exHeap,
			       ComDiagsArea **diags) { return 0; };

protected:

  ExPartInputDataDesc(ExPartitioningType partType,
		      ex_cri_desc *partInputCriDesc,
		      Lng32 partInputDataLength,
		      Lng32 numPartitions);

private:

  // record descr. of part input data
  ExCriDescPtr    partInputCriDesc_;                 // 00-07

  // hash, range, RR partitioning? (enums aren't portable!!)
  Int32           partType_;                         // 08-11

  // length of part. input data
  Int32           partInputDataLength_;              // 12-15

  // # of partitions
  Int32           numPartitions_;                    // 16-19

  char            fillersExPartInputDataDesc_[36];   // 20-55
};


// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExPartInputDataDesc
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<ExPartInputDataDesc> ExPartInputDataDescPtr;


class ExHashPartInputData : public ExPartInputDataDesc
{
public:
  ExHashPartInputData() : ExPartInputDataDesc(HASH_PARTITIONED,NULL,0,0) {}

  ExHashPartInputData(ex_cri_desc *partInputCriDesc,
		      Lng32 numPartitions);
  void copyPartInputValue(Lng32 fromPartNum,
			  Lng32 toPartNum,
			  char *buffer,
			  Lng32 bufferLength);

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
    ExPartInputDataDesc::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(ExHashPartInputData); }

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);

};

class ExRoundRobinPartInputData : public ExPartInputDataDesc
{
public:
  ExRoundRobinPartInputData() : ExPartInputDataDesc
  (ROUNDROBIN_PARTITIONED,NULL,0,0) {}

  ExRoundRobinPartInputData(ex_cri_desc *partInputCriDesc,
			    Lng32 numPartitions,
                            Lng32 numOrigRRPartitions);

  void copyPartInputValue(Lng32 fromPartNum,
			  Lng32 toPartNum,
			  char *buffer,
			  Lng32 bufferLength);

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
    ExPartInputDataDesc::populateImageVersionIDArray();
  }

  virtual short getClassSize()
                         { return (short)sizeof(ExRoundRobinPartInputData); }

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);
private:

  Int32 numOrigRRPartitions_;                     // 00-03
  char fillersExRoundRobinPartInputData_[20];     // 04-23
};

class ExRangePartInputData : public ExPartInputDataDesc
{
public:
  ExRangePartInputData() : ExPartInputDataDesc(RANGE_PARTITIONED,NULL,0,0) {}

  ExRangePartInputData(ex_cri_desc *partInputCriDesc,
		       Lng32 partInputDataLength,
		       Lng32 partKeyLength,
		       Lng32 exclusionIndicatorOffset,
		       Lng32 numPartitions,
		       Space *space,
		       Lng32 useExpressions);

  ~ExRangePartInputData();

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
    ExPartInputDataDesc::populateImageVersionIDArray();
  }

  virtual short getClassSize()
                              { return (short)sizeof(ExRangePartInputData); }

  void setPartitionStartExpr(Lng32 partNo, ex_expr *expr);
  inline void setPartitionExprAtp(Lng32 atp)   { partRangeExprAtp_ = atp; }
  inline void setPartitionExprAtpIndex(Lng32 atpix)
                                       { partRangeExprAtpIndex_ = atpix; }

  // set the partition start value (done in the generator or at fixup)
  void setPartitionStartValue(Lng32 partNo,
			      char *val);

  // create the partition input value (begin key, end key) for one partition
  // or for a range of partitions
  void copyPartInputValue(Lng32 fromPartNum,
			  Lng32 toPartNum,
			  char *buffer,
			  Lng32 bufferLength);

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);
  virtual Lng32 evalExpressions(Space * space,
			       CollHeap * exHeap,
			       ComDiagsArea **diags);

private:

  // Partition input values as they are being sent:
  //
  // |---begin key---|-filler1-|----end key----|-filler2-|-excl. ind.-|
  //                 ^         ^                         ^
  //                 |         |                         |
  //                 part      aligned                   exclusion
  //                 key       part key                  indicator
  //                 length    length                    offset
  //
  // Note that begin key and end key have exactly the same filler layout.

  // the length of the partitioning key (note that the partition input data
  // consists of a begin key and an end key plus additional info on whether
  // the key range is inclusive and exclusive)
  Int32 partKeyLength_;                                      // 00-03

  // the partition key length, rounded up to the next multiple
  // of the needed alignment (we have an array of partition input data rows)
  Int32 alignedPartKeyLength_;                               // 04-07

  // where does the exclusion indicator start and how long is it
  Int32 exclusionIndicatorOffset_;                           // 08-11
  Int32 exclusionIndicatorLength_;                           // 12-15

  // An array of expressions to fill in values into the partRanges_
  // byte array and atp/atpindex info to evaluate these expressions.
  // The pointer to the array could also be NULL, in which case the
  // byte array partRanges_ has to be filled at compile time.
  ExExprPtrPtr partRangeExpressions_;                        // 16-23
  Int32 useExpressions_;                                     // 24-27
  Int32 partRangeExprAtp_;                                   // 28-31
  Int32 partRangeExprAtpIndex_;                              // 32-35
  Int32 partRangeExprHasBeenEvaluated_;                      // 36-39

  // a byte string with <numPartitions_> records for partition input data,
  // the first record being the key for negative infinity, and the last being
  // the begin key for the last partition (all but the last range are
  // exclusive)
  NABasicPtr partRanges_;                                    // 40-47

  // a similar byte string, but with all values key-encoded, for easier
  // comparison when searching for the partition number of a key
  NABasicPtr keyEncodedPartRanges_;                          // 48-55

  char fillersExRangePartInputData_[40];                     // 56-95

};

class ExHashDistPartInputData : public ExPartInputDataDesc
{
public:
  ExHashDistPartInputData() : ExPartInputDataDesc
  (HASH1_PARTITIONED,NULL,0,0) {}

  ExHashDistPartInputData(ex_cri_desc *partInputCriDesc,
                          Lng32 numPartitions,
                          Lng32 numOrigHashPartitions);

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
    ExPartInputDataDesc::populateImageVersionIDArray();
  }

  virtual short getClassSize()
                           { return (short)sizeof(ExHashDistPartInputData); }

  void copyPartInputValue(Lng32 fromPartNum,
                        Lng32 toPartNum,
                        char *buffer,
                        Lng32 bufferLength);

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);

private:

  Int32 numOrigHashPartitions_;                   // 00-03
  char fillersExHashDistPartInputData_[20];       // 04-23

};

class ExHash2PartInputData : public ExPartInputDataDesc
{
public:
  ExHash2PartInputData() : ExPartInputDataDesc
  (HASH2_PARTITIONED,NULL,0,0) {}

  ExHash2PartInputData(ex_cri_desc *partInputCriDesc,
                       Lng32 numPartitions);

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
    ExPartInputDataDesc::populateImageVersionIDArray();
  }

  virtual short getClassSize()
                           { return (short)sizeof(ExHash2PartInputData); }

  void copyPartInputValue(Lng32 fromPartNum,
                          Lng32 toPartNum,
                          char *buffer,
                          Lng32 bufferLength);

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);

private:

  char fillersExHash2PartInputData[24];           // 00-23

};

#endif // EX_PART_INPUT_H
