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
* File:         ComTdbHashGrby.h
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

#ifndef COM_HASH_GRBY_H
#define COM_HASH_GRBY_H

//
// Task Definition Block
//
#include "ComTdb.h"

class ComTdbHashGrby : public ComTdb {
  friend class ex_hash_grby_tcb;
  friend class ex_hash_grby_private_state;

protected:

  ComTdbPtr      childTdb_;                      // 00-07
  ExExprPtr      hashExpr_;                      // 08-15
  ExExprPtr      bitMuxExpr_;                    // 16-23
  ExExprPtr      bitMuxAggrExpr_;                // 24-31
  ExExprPtr      hbMoveInExpr_;                  // 32-39
  ExExprPtr      ofMoveInExpr_;                  // 40-47
  ExExprPtr      resMoveInExpr_;                 // 48-55
  ExExprPtr      hbAggrExpr_;                    // 56-63
  ExExprPtr      ofAggrExpr_;                    // 64-71
  ExExprPtr      resAggrExpr_;                   // 72-79
  ExExprPtr      havingExpr_;                    // 80-87
  ExExprPtr      moveOutExpr_;                   // 88-95
  ExExprPtr      hbSearchExpr_;                  // 96-103
  ExExprPtr      ofSearchExpr_;                  // 104-111
  ExCriDescPtr   workCriDesc_;                   // 112-119
  UInt32         keyLength_;                     // 120-123
  UInt32         resultRowLength_;               // 124-127
  UInt32         extGroupedRowLength_;           // 128-131
  Int32          isPartialGroup_;                // 132-135
  Int16          hbRowAtpIndex_;                 // 136-137
  Int16          ofRowAtpIndex_;                 // 138-139
  Int16          hashValueAtpIndex_;             // 140-141
  Int16          bitMuxAtpIndex_;                // 142-143
  Int16          bitMuxCountOffset_;             // 144-145
  Int16          resultRowAtpIndex_;             // 146-147
  Int16          returnedAtpIndex_;              // 148-149
  UInt16         memUsagePercent_;               // 150-151
  Int16          pressureThreshold_;             // 152-153
  UInt16         scratchThresholdPct_;           // 154-155
  UInt16         hashGroupByFlags_;              // 156-157
  UInt16         memoryQuotaMB_;                 // 158-159
  UInt32         partialGrbyFlushThreshold_;     // 160-163
  UInt32         partialGrbyRowsPerCluster_;     // 164-167
  UInt16         partialGrbyMemoryMB_;           // 168-169
  UInt16         minBuffersToFlush_;             // 170-171
  UInt32         initialHashTableSize_;          // 172-175
  UInt32         numInBatch_;                    // 176-179
  UInt16         forceOverflowEvery_;            // 180-181
  UInt16         hgbGrowthPercent_;              // 182-183
  Float32        hgbMemEstInMbPerCpu_;           // 184-187
  Int16          scratchIOVectorSize_;           // 188-189
  UInt16         bmoMinMemBeforePressureCheck_;  // 190-191
  UInt16         bmoMaxMemThresholdMB_;          // 192-193
  char           fillersComTdbHashGrby_[6];      // 194-199
 
public:

NA_EIDPROC
  ComTdbHashGrby();                // dummy constructor.
                                     // Used by 'unpack' routines.
  
NA_EIDPROC
  ComTdbHashGrby(ComTdb * childTdb,
		 ex_cri_desc * givenDesc,
		 ex_cri_desc * returnedDesc,
		 ex_expr * hashExpr,
		 ex_expr * bitMuxExpr,
		 ex_expr * bitMuxAggrExpr,
		 ex_expr * hbMoveInExpr,
		 ex_expr * ofMoveInExpr,
		 ex_expr * resMoveInExpr,
		 ex_expr * hbAggrExpr,
		 ex_expr * ofAggrExpr,
		 ex_expr * resAggrExpr,
		 ex_expr * havingExpr,
		 ex_expr * moveOutExpr,
		 ex_expr * hbSearchExpr,
		 ex_expr * ofSearchExpr,
		 ULng32 keyLength,
		 ULng32 resultRowLength,
		 ULng32 extGroupRowLength,
		 ex_cri_desc * workCriDesc,
		 short hbRowAtpIndex,
		 short ofRowAtpIndex,
		 short hashValueAtpIndex,
		 short bitMuxAtpIndex,
		 short bitMuxCountOffset,
		 short resultRowAtpIndex,
		 short returnedAtpIndex,
		 unsigned short memUsagePercent,
		 short pressureThreshold,
                 short scrThreshold,
		 queue_index fromParent,
		 queue_index toParent,
		 NABoolean isPartialGroup,
		 Cardinality estimatedRowCount,
		 Lng32 numBuffers,
		 ULng32 bufferSize,
		 ULng32 partialGrbyFlushThreshold,
		 ULng32 partialGrbyRowsPerCluster,
		 ULng32 initialHashTableSize,
		 unsigned short minBuffersToFlush,
		 ULng32 numInBatch,
                 short hgbGrowthPercent
		 );
  
NA_EIDPROC
  ~ComTdbHashGrby();

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  NA_EIDPROC virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  NA_EIDPROC virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }

  NA_EIDPROC virtual short getClassSize()
                                 { return (short)sizeof(ComTdbHashGrby); }  
  Long pack(void *);

NA_EIDPROC
  Lng32 unpack(void *, void * reallocator);
  
NA_EIDPROC
  void display() const;

NA_EIDPROC
  inline ComTdb * getChildTdb();

NA_EIDPROC
  Int32 orderedQueueProtocol() const;

  AggrExpr * hbAggrExpr()  const { return (AggrExpr*)((ex_expr*)hbAggrExpr_); }
  AggrExpr * ofAggrExpr()  const { return (AggrExpr*)((ex_expr*)ofAggrExpr_); }
  AggrExpr * resAggrExpr() const { return (AggrExpr*)((ex_expr*)resAggrExpr_); }

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual void displayContents(Space *space,ULng32 flag);

#if 0
NA_EIDPROC 
  //virtual
  void allocateStatsEntry(Int64 operID,
				  Int64 estRowsReturned,
				  Space * space);
#endif
  
   // **** GUI ****
NA_EIDPROC
  virtual const ComTdb* getChild(Int32 pos) const;

NA_EIDPROC
  virtual Int32 numChildren() const { return 1; };

NA_EIDPROC
  virtual const char *getNodeName() const { return "EX_HASH_GRBY"; };

NA_EIDPROC
  virtual Int32 numExpressions() const { return 13; };

NA_EIDPROC
  virtual ex_expr * getExpressionNode(Int32 pos) {
  if (pos == 0)
    return hashExpr_;
  else if (pos == 1)
    return bitMuxExpr_;
  else if (pos == 2)
    return bitMuxAggrExpr_;
  else if (pos == 3)
    return hbMoveInExpr_;
  else if (pos == 4)
    return ofMoveInExpr_;
  else if (pos == 5)
    return resMoveInExpr_;
  else if (pos == 6)
    return hbAggrExpr_;
  else if (pos == 7)
    return ofAggrExpr_;
  else if (pos == 8)
    return resAggrExpr_;
  else if (pos == 9)
    return havingExpr_;
  else if (pos == 10)
    return moveOutExpr_;
  else if (pos == 11)
    return hbSearchExpr_;
  else if (pos == 12)
    return ofSearchExpr_;
  else
    return NULL;
};

NA_EIDPROC
  virtual const char * getExpressionName(Int32 pos) const {
  if (pos == 0)
    return "hashExpr_";
  else if (pos == 1)
    return "bitMuxExpr_";
  else if (pos == 2)
    return "bitMuxAggrExpr_";
  else if (pos == 3)
    return "hbMoveInExpr_";
  else if (pos == 4)
    return "ofMoveInExpr_";
  else if (pos == 5)
    return "resMoveInExpr_";
  else if (pos == 6)
    return "hbAggrExpr_";
  else if (pos == 7)
    return "ofAggrExpr_";
  else if (pos == 8)
    return "resAggrExpr_";
  else if (pos == 9)
    return "havingExpr_";
  else if (pos == 10)
    return "moveOutExpr_";
  else if (pos == 11)
    return "hbSearchExpr_";
  else if (pos == 12)
    return "ofSearchExpr_";
  else
    return NULL;
}

  enum groupby_flags { LOG_DIAGNOSTICS_ = 0x0002 
		       ,POSSIBLE_MULTIPLE_CALLS = 0x0004
                       ,PASS_PARTIAL_ROWS = 0x0008
                       ,USE_VAR_LEN = 0x0010
                       ,CONSIDER_BUFFER_DEFRAG = 0x0020
  };

  NABoolean logDiagnostics() { return (hashGroupByFlags_ & LOG_DIAGNOSTICS_) != 0;}
  void setLogDiagnostics(NABoolean v)
  {(v ? hashGroupByFlags_ |= LOG_DIAGNOSTICS_ : hashGroupByFlags_ &= ~LOG_DIAGNOSTICS_);}


  NA_EIDPROC
  NABoolean isPossibleMultipleCalls() 
  {return (hashGroupByFlags_ & POSSIBLE_MULTIPLE_CALLS) != 0;}

  NA_EIDPROC  
  void setPossibleMultipleCalls(NABoolean v)
  {(v ? hashGroupByFlags_ |= POSSIBLE_MULTIPLE_CALLS : 
    hashGroupByFlags_ &= ~POSSIBLE_MULTIPLE_CALLS);}  
  
  NA_EIDPROC
  NABoolean isPassPartialRows() 
  {return (hashGroupByFlags_ & PASS_PARTIAL_ROWS) != 0;}

  NA_EIDPROC  
  void setPassPartialRows(NABoolean v)
  {(v ? hashGroupByFlags_ |= PASS_PARTIAL_ROWS : 
    hashGroupByFlags_ &= ~PASS_PARTIAL_ROWS);}  
  
  // Is this Hash GroupBy TDB configured to use variable length Records in HashBuffers
  //
  NABoolean useVariableLength() const {return (hashGroupByFlags_ & USE_VAR_LEN) !=0; };

  // Configure the Hash GroupBy TDB to use variable length Records in HashBuffers
  //
  void setUseVariableLength() {hashGroupByFlags_ |= USE_VAR_LEN;};

  NABoolean considerBufferDefrag() const {return (hashGroupByFlags_ & CONSIDER_BUFFER_DEFRAG) !=0; };
  void setConsiderBufferDefrag() {hashGroupByFlags_ |= CONSIDER_BUFFER_DEFRAG;};

  NA_EIDPROC  UInt16 forceOverflowEvery()  {return forceOverflowEvery_;}

  NA_EIDPROC  
    void setForceOverflowEvery(UInt16 times)  { forceOverflowEvery_ = times; }
  
  NA_EIDPROC
  ULng32 memoryQuotaMB() { return (ULng32) memoryQuotaMB_; }

  NA_EIDPROC
  void setMemoryQuotaMB(UInt16 v) { memoryQuotaMB_ = v; }

  NA_EIDPROC
  ULng32 partialGrbyMemoryMB() { return (ULng32) partialGrbyMemoryMB_; }

  NA_EIDPROC
  void setPartialGrbyMemoryMB(UInt16 v) { partialGrbyMemoryMB_ = v; }

  NA_EIDPROC
  void setHgbMemEstInMbPerCpu(Float32 s) {hgbMemEstInMbPerCpu_=s;}

  NA_EIDPROC
  Float32 getHgbMemEstInMbPerCpu() {return hgbMemEstInMbPerCpu_;}

  NA_EIDPROC
  Float32 hgbGrowthPercent() {return Float32(hgbGrowthPercent_/100.0);}

  NA_EIDPROC
  Int32 scratchIOVectorSize() { return (Int32) scratchIOVectorSize_; }

  NA_EIDPROC
  void setScratchIOVectorSize(Int16 v) { scratchIOVectorSize_ = v; }

  NA_EIDPROC
  void  setBmoMinMemBeforePressureCheck(UInt16 m)
  { bmoMinMemBeforePressureCheck_ = m ; }

  NA_EIDPROC
  UInt16  getBmoMinMemBeforePressureCheck()
  { return bmoMinMemBeforePressureCheck_; }
  
  NA_EIDPROC
  void  setBMOMaxMemThresholdMB(UInt16 m)
  { bmoMaxMemThresholdMB_ = m ; }

  NA_EIDPROC
  UInt16  getBMOMaxMemThresholdMB()
  { return bmoMaxMemThresholdMB_; }

  NABoolean isNonBMOPartialGroupBy() { return (isPartialGroup_ == TRUE); }

};


NA_EIDPROC
inline ComTdb * ComTdbHashGrby::getChildTdb(){
  return childTdb_;
};


/*****************************************************************************
  Description : Return ComTdb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
  History     : Yeogirl Yun                                      8/22/95
                 Initial Revision.
*****************************************************************************/
NA_EIDPROC
inline const ComTdb* ComTdbHashGrby::getChild(Int32 pos) const {
  if (pos == 0)
    return childTdb_;
  else
    return NULL;
};

#endif
