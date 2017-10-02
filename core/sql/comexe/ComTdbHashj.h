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
* File:         ComTdbHashj.h
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

#ifndef COM_HASHJ_H
#define COM_HASHJ_H

#include "ComTdb.h"

/////////////////////////////////////////////////////////////////////////////
// Task Definition Block
// for a description of the expressions see GerRelJoin.C
/////////////////////////////////////////////////////////////////////////////
class ComTdbHashj : public ComTdb {
  friend class ex_hashj_tcb;
  friend class ExUniqueHashJoinTcb;
  friend class ex_hashj_private_state;

public:
  ComTdbHashj();
  ComTdbHashj(ComTdb * leftChildTdb,
	      ComTdb * rightChildTdb,
	      ex_cri_desc * givenCriDesc,
	      ex_cri_desc * returnedCriDesc,
	      ex_expr * rightHashExpr,
	      ex_expr * rightMoveInExpr,
	      ex_expr * rightMoveOutExpr,
	      ex_expr * rightSearchExpr,
	      ex_expr * leftHashExpr,
	      ex_expr * leftMoveExpr,
	      ex_expr * leftMoveInExpr,
	      ex_expr * leftMoveOutExpr,
	      ex_expr * probeSearchExpr1,
	      ex_expr * probeSearchExpr2,
	      ex_expr * leftJoinExpr,
	      ex_expr * nullInstForLeftJoinExpr,
	      ex_expr * beforeJoinPred1,
	      ex_expr * beforeJoinPred2,
	      ex_expr * afterJoinPred1,
	      ex_expr * afterJoinPred2,
	      ex_expr * afterJoinPred3,
	      ex_expr * afterJoinPred4,
	      ex_expr * afterJoinPred5,
	      ex_expr * checkInputPred,
	      ex_expr * moveInputExpr,
	      Lng32 inputValuesLen,
	      short prevInputTuppIndex,
	      ULng32 rightRowLength,
	      ULng32 extRightRowLength,
	      ULng32 leftRowLength,
	      ULng32 extLeftRowLength,
	      ULng32 instRowForLeftJoinLength,
	      ex_cri_desc * workCriDesc,
	      short leftRowAtpIndex,
	      short extLeftRowAtpIndex,
	      short rightRowAtpIndex,
	      short extRightRowAtpIndex1,
	      short extRightRowAtpIndex2,
	      short hashValueAtpIndex,
	      short instRowForLeftJoinAtpIndex,
	      short returnedLeftRowAtpIndex,
	      short returnedRightRowAtpIndex,
	      short returnedInstRowForLeftJoinAtpIndex,
	      unsigned short memUsagePercent,
	      short pressureThreshold,
              short scrThreshold,
	      queue_index down,
	      queue_index up,
	      Int32 isSemiJoin,
	      Int32 isLeftJoin,
	      Int32 isAntiSemiJoin,
	      Int32 isUniqueHashJoin,
	      Int32 isNoOverflow,
	      Int32 isReuse,
	      Lng32 numBuffers,
	      ULng32 bufferSize,
	      ULng32 hashBufferSize,
	      Cardinality estimatedRowount,
	      Cardinality innerExpectedRows,
	      Cardinality outerExpectedRows,
	      Int32 isRightJoin,
	      ex_expr *rightJoinExpr,
	      ex_expr *nullInstForRightJoinExpr,
	      short instRowForRightJoinAtpIndex,
	      short returnedInstRowForRightJoinAtpIndex,
	      ULng32 instRowForRightJoinLength,
	      unsigned short minBuffersToFlush,
	      ULng32 numInBatch,
	      ex_expr * checkInnerNullExpr,
	      ex_expr * checkOuterNullExpr,
              short hjGrowthPercent,
	      short minMaxValsAtpIndex,
	      ULng32 minMaxRowLength,
              ex_expr * minMaxExpr,
	      ex_cri_desc * leftDownCriDesc
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

  virtual short getClassSize()     { return (short)sizeof(ComTdbHashj); }

  ///////////////////////////////////////////////////////////////////////
  // public member functions
  ///////////////////////////////////////////////////////////////////////
  void display() const;
  // ex_queue_pair  getParentQueue() const;
  Int32 orderedQueueProtocol() const;
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  inline ComTdb * getLeftChildTdb();
  inline ComTdb * getRightChildTdb();

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  virtual void displayContents(Space *space,ULng32 flag);

  // ****  information for GUI  *** -------------
				 
  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return 2; }
  virtual const char *getNodeName() const { return "EX_HASHJ"; };
  virtual Int32 numExpressions() const { return 26; }
  virtual ex_expr* getExpressionNode(Int32 pos) {
    if (pos == 0)
      return rightHashExpr_;
    else if (pos == 1)
      return rightMoveInExpr_;
    else if (pos == 2)
      return rightMoveOutExpr_;
    else if (pos == 3)
      return rightSearchExpr_;
    else if (pos == 4)
      return leftHashExpr_;
    else if (pos == 5)
      return leftMoveExpr_;
    else if (pos == 6)
      return leftMoveInExpr_;
    else if (pos == 7)
      return leftMoveOutExpr_;
    else if (pos == 8)
      return probeSearchExpr1_;
    else if (pos == 9)
      return probeSearchExpr2_;
    else if (pos == 10)
      return leftJoinExpr_;
    else if (pos == 11)
      return nullInstForLeftJoinExpr_;
    else if (pos == 12)
      return beforeJoinPred1_;
    else if (pos == 13)
      return beforeJoinPred2_;
    else if (pos == 14)
      return afterJoinPred1_;
    else if (pos == 15)
      return afterJoinPred2_;
    else if (pos == 16)
      return afterJoinPred3_;
    else if (pos == 17)
      return afterJoinPred4_;
    else if (pos == 18)
      return afterJoinPred5_;
    else if (pos == 19)
      return checkInputPred_;
    else if (pos == 20)
      return moveInputExpr_;
    else if (pos == 21)
      return rightJoinExpr_;
    else if (pos == 22)
      return nullInstForRightJoinExpr_;
    else if (pos == 23)
      return checkInnerNullExpr_;
    else if (pos == 24)
      return checkOuterNullExpr_;
    else if (pos == 25)
      return minMaxExpr_;
    else
      return NULL;
  }
  virtual const char * getExpressionName(Int32 pos) const {
    if (pos == 0)
      return "rightHashExpr_";
    else if (pos == 1)
      return "rightMoveInExpr_";
    else if (pos == 2)
      return "rightMoveOutExpr_";
    else if (pos == 3)
      return "rightSearchExpr_";
    else if (pos == 4)
      return "leftHashExpr_";
    else if (pos == 5)
      return "leftMoveExpr_";
    else if (pos == 6)
      return "leftMoveInExpr_";
    else if (pos == 7)
      return "leftMoveOutExpr_";
    else if (pos == 8)
      return "probeSearchExpr1_";
    else if (pos == 9)
      return "probeSearchExpr2_";
    else if (pos == 10)
      return "leftJoinExpr_";
    else if (pos == 11)
      return "nullInstForLeftJoinExpr_";
    else if (pos == 12)
      return "beforeJoinPred1_";
    else if (pos == 13)
      return "beforeJoinPred2_";
    else if (pos == 14)
      return "afterJoinPred1_";
    else if (pos == 15)
      return "afterJoinPred2_";
    else if (pos == 16)
      return "afterJoinPred3_";
    else if (pos == 17)
      return "afterJoinPred4_";
    else if (pos == 18)
      return "afterJoinPred5_";
    else if (pos == 19)
      return "checkInputPred_";
    else if (pos == 20)
      return "moveInputExpr_";
    else if (pos == 21)
      return "rightJoinExpr_";
    else if (pos == 22)
      return "nullInstForRightJoinExpr_";
    else if (pos == 23)
      return "checkInnerNullExpr_";
    else if (pos == 24)
      return "checkOuterNullExpr_";
    else if (pos == 25)
      return "minMaxExpr_";
    else
      return NULL;
  }   
  //---------------------------------------------   
   
  // The 3 "force" settings below are only for overflow testing
  UInt16 forceOverflowEvery() {return forceOverflowEvery_;}
  void setForceOverflowEvery(UInt16 times)
  {
    // don't force when "no overflow" is enforced
    forceOverflowEvery_ = ( hjFlags_ & NO_OVERFLOW ) ? 0 : times ;
  }
  UInt16 forceHashLoopAfterNumBuffers() 
  { return forceHashLoopAfterNumBuffers_; }
  void setForceHashLoopAfterNumBuffers(UInt16 numBuffers)
  { forceHashLoopAfterNumBuffers_ = numBuffers; }
  UInt16 forceClusterSplitAfterMB() { return forceClusterSplitAfterMB_;}
  void setForceClusterSplitAfterMB(UInt16 mb)
  { forceClusterSplitAfterMB_ = mb; }



  NABoolean bufferedWrites() { return (hjFlags_ & BUFFERED_WRITES_) != 0;}
  void setBufferedWrites(NABoolean v)
  {(v ? hjFlags_ |= BUFFERED_WRITES_ : hjFlags_ &= ~BUFFERED_WRITES_);}
  NABoolean logDiagnostics() { return (hjFlags_ & LOG_DIAGNOSTICS_) != 0;}
  void setLogDiagnostics(NABoolean v)
  {(v ? hjFlags_ |= LOG_DIAGNOSTICS_ : hjFlags_ &= ~LOG_DIAGNOSTICS_);}
  NABoolean disableCmpHintsOverflow() 
  { return (flags_ & DISABLE_CMP_HINTS_OVERFLOW_) != 0;}
  void setDisableCmpHintsOverflow(NABoolean v)
  {(v ? flags_ |= DISABLE_CMP_HINTS_OVERFLOW_ : 
    flags_ &= ~DISABLE_CMP_HINTS_OVERFLOW_);}

  // Should right rows with duplicate key values be returned in the same order
  // as they were read ?
  NABoolean isReturnRightOrdered()  {return (hjFlags_ & RETURN_RIGHT_ORDERED) != 0;}
  void setReturnRightOrdered(NABoolean v)
  {(v ? hjFlags_ |= RETURN_RIGHT_ORDERED : hjFlags_ &= ~RETURN_RIGHT_ORDERED);}  
  // Is this HJ under the right child of a TSJ ?
  NABoolean isPossibleMultipleCalls() 
  {return (hjFlags_ & POSSIBLE_MULTIPLE_CALLS) != 0;}
  void setPossibleMultipleCalls(NABoolean v)
  {(v ? hjFlags_ |= POSSIBLE_MULTIPLE_CALLS : 
    hjFlags_ &= ~POSSIBLE_MULTIPLE_CALLS);}  

  // At execution time, delay sending the request to the left child (until we find
  // that there are no right rows, hence the left rows are needed.)
  //   Used for AntiSemi HJ without join expression -- i.e. All-Or-Nothing . 
  //   When there are right rows, nothing is needed from the left side.
  // The flag should be set based on the likelihood of getting any right rows.
  NABoolean delayLeftRequest()  {return (hjFlags_ & DELAY_LEFT_REQUEST) != 0;}
  void setDelayLeftRequest(NABoolean v)
  {(v ? hjFlags_ |= DELAY_LEFT_REQUEST : hjFlags_ &= ~DELAY_LEFT_REQUEST);}  

  ULng32 memoryQuotaMB() { return (ULng32) memoryQuotaMB_; }
  void setMemoryQuotaMB(UInt16 v) { memoryQuotaMB_ = v; }
  UInt16 numClusters() { return numClusters_; }
  void setNumClusters(UInt16 v) { numClusters_ = v; }

  // Is this Hash Join TDB configured to use the Unique Hash Join TCB.
  //
  NABoolean isUniqueHashJoin() const {return (hjFlags_ & UNIQUE_HASH_JOIN) !=0; };

  // Configure the Hash Join TDB to use the Unique Hash Join TCB.
  //
  void setUniqueHashJoin() {hjFlags_ |= UNIQUE_HASH_JOIN;};

  // Is this Hash Join TDB configured to use variable length Records in HashBuffers
  //
  NABoolean useVariableLength() const {return (hjFlags_ & USE_VAR_LEN) !=0; };

  // Configure the Hash Join TDB to use variable length Records in HashBuffers
  //
  void setUseVariableLength() {hjFlags_ |= USE_VAR_LEN;};

  NABoolean considerBufferDefrag() const {return (hjFlags_ & CONSIDER_BUFFER_DEFRAG) !=0; };

  void setConsiderBufferDefrag() {hjFlags_ |= CONSIDER_BUFFER_DEFRAG;};


  inline void setXproductPreemptMax(UInt32 m) {
    xProductPreemptMax_ = m;
  };

  inline UInt32 getXproductPreemptMax(void) const {
    return xProductPreemptMax_;
  };

  Int32 scratchIOVectorSize() { return (Int32) scratchIOVectorSize_; }
  void setScratchIOVectorSize(Int16 v) { scratchIOVectorSize_ = v; }

  // set this to true if the left side has an IUD operation
  // this lets us disable an optimization which cancels when
  // right side is empty.  The optimization needs to be disabled
  // so that the IUD operation can complete.
  NABoolean leftSideIUD() const 
    {return (hjFlags_ & LEFT_SIDE_IUD_OPERATION) !=0; }; 
  void setLeftSideIUD() { hjFlags_ |= LEFT_SIDE_IUD_OPERATION;};

  NABoolean beforePredOnOuterOnly() const 
    {return (hjFlags2_ & BEFORE_PRED_OUTER_ONLY) !=0; }; 
  void setBeforePredOnOuterOnly() { hjFlags2_ |= BEFORE_PRED_OUTER_ONLY;};

  AggrExpr * minMaxExpr() { return (AggrExpr*)((ex_expr*)minMaxExpr_); }

protected:
  
  enum join_flags { SEMI_JOIN = 0x0001, 
		    LEFT_JOIN = 0x0002,
		    ANTI_SEMI_JOIN = 0x0004, 
		    NO_OVERFLOW = 0x0008,
		    REUSE = 0x0010, 
		    DISABLE_CMP_HINTS_OVERFLOW_ = 0x0020,
		    LOG_DIAGNOSTICS_ = 0x0040,
		    BUFFERED_WRITES_ = 0x0080,
		    RETURN_RIGHT_ORDERED = 0x0100,
		    POSSIBLE_MULTIPLE_CALLS = 0x200,
		    RIGHT_JOIN = 0x400,
		    DELAY_LEFT_REQUEST = 0x800,
                    UNIQUE_HASH_JOIN = 0x1000,
                    USE_VAR_LEN = 0x2000,
                    CONSIDER_BUFFER_DEFRAG = 0x4000,
                    LEFT_SIDE_IUD_OPERATION = 0x8000
  };
  enum join_flags2 { BEFORE_PRED_OUTER_ONLY = 0x0001 };

  ComTdbPtr    leftChildTdb_;                       // 00-07
  ComTdbPtr    rightChildTdb_;                      // 08-15
  ExExprPtr    rightHashExpr_;                      // 16-23
  ExExprPtr    rightMoveInExpr_;                    // 24-31
  ExExprPtr    rightMoveOutExpr_;                   // 32-39
  ExExprPtr    rightSearchExpr_;                    // 40-47
  ExExprPtr    leftHashExpr_;                       // 48-55
  ExExprPtr    leftMoveExpr_;                       // 56-63
  ExExprPtr    leftMoveInExpr_;                     // 64-71
  ExExprPtr    leftMoveOutExpr_;                    // 72-79
  ExExprPtr    probeSearchExpr1_;                   // 80-87
  ExExprPtr    probeSearchExpr2_;                   // 88-95
  ExExprPtr    leftJoinExpr_;                       // 96-103
  ExExprPtr    nullInstForLeftJoinExpr_;            // 104-111
  ExExprPtr    beforeJoinPred1_;                    // 112-119
  ExExprPtr    beforeJoinPred2_;                    // 120-127
  ExExprPtr    afterJoinPred1_;                     // 128-135
  ExExprPtr    afterJoinPred2_;                     // 136-143
  ExExprPtr    afterJoinPred3_;                     // 144-151
  ExExprPtr    afterJoinPred4_;                     // 152-159
  ExCriDescPtr workCriDesc_;                        // 160-167
  UInt32       rightRowLength_;                     // 168-171
  UInt32       extRightRowLength_;                  // 172-175
  UInt32       leftRowLength_;                      // 176-179
  UInt32       extLeftRowLength_;                   // 180-183
  UInt32       instRowForLeftJoinLength_;           // 184-187
  Int16        leftRowAtpIndex_;                    // 188-189
  Int16        extLeftRowAtpIndex_;                 // 190-191
  Int16        rightRowAtpIndex_;                   // 192-193
  Int16        extRightRowAtpIndex1_;               // 194-195
  Int16        extRightRowAtpIndex2_;               // 196-197
  Int16        hashValueAtpIndex_;                  // 198-199
  Int16        instRowForLeftJoinAtpIndex_;         // 200-201
  Int16        returnedLeftRowAtpIndex_;            // 202-203
  Int16        returnedRightRowAtpIndex_;           // 204-205
  Int16        returnedInstRowForLeftJoinAtpIndex_; // 206-207
  UInt16       hjFlags_;                            // 208-209
  // index into the workAtp where prev input values tupp will be moved
  Int16        prevInputTuppIndex_;                 // 210-211

private:
  Float32      p_innerExpectedRows_;                // 212-215
  Float32      p_outerExpectedRows_;                // 216-219

protected:
  UInt16       memUsagePercent_;                    // 220-221
  Int16        pressureThreshold_;                  // 222-223
  // Expression to match the input against the previous input
  ExExprPtr    checkInputPred_;                     // 224-231 
  // move/save current input
  ExExprPtr    moveInputExpr_;                      // 232-239
  Int32        inputValuesLen_;                     // 240-243
  UInt16       scratchThresholdPct_;                // 244-245

  // max memory this operator should allocate. In Mbytes.
  UInt16       memoryQuotaMB_;                      // 246-247
  UInt16       numClusters_;                        // 248-249
  Int16        instRowForRightJoinAtpIndex_;        // 250-251
  Int16        returnedInstRowForRightJoinAtpIndex_;// 252-253
  UInt16       minBuffersToFlush_;                  // 254-255
  ExExprPtr    rightJoinExpr_;                      // 256-263
  ExExprPtr    nullInstForRightJoinExpr_;           // 264-271
  UInt32       instRowForRightJoinLength_;          // 272-275
  UInt32       numInBatch_;                         // 276-279
  UInt16       forceOverflowEvery_;                 // 280-281
  UInt16       forceHashLoopAfterNumBuffers_;       // 282-283
  UInt16       forceClusterSplitAfterMB_;           // 284-285
  UInt16       hjGrowthPercent_;                    // 286-287
  UInt32       xProductPreemptMax_;                 // 288-291
  UInt32       hashBufferSize_;                     // 292-295 

private:
  ExExprPtr    checkInnerNullExpr_;                 // 296-303
  ExExprPtr    checkOuterNullExpr_;                 // 304-311
  ExExprPtr    afterJoinPred5_;                     // 312-319
  Float32      hjMemEstInKBPerNode_;                 // 320-323
  Float32      bmoCitizenshipFactor_;               // 324-327
  Int32        pMemoryContingencyMB_;               // 328-331
  Int16        scratchIOVectorSize_;                // 332-333
  UInt16       bmoMinMemBeforePressureCheck_;       // 334-335
  UInt16       bmoMaxMemThresholdMB_;               // 336-337

  // Added for support of the MIN/MAX optimization
  // for HashJoin. Min and Max values are comupted
  // during readInnerChild phase and passed to outer
  // before starting the read from the outer child

  // The atp index in the workAtp where the minmax 
  // tuple resides
  Int16        minMaxValsAtpIndex_;                 // 338-339

  // The length of the min max tuple.
  UInt32       minMaxRowLength_;                    // 340-343

  // The expression to compute the min max values
  ExExprPtr    minMaxExpr_;                         // 344-351

  // The CRI Desc given to the outer child.  Without
  // the min max opt, this is the same as the CRIDesc
  // from the parent.  With the min max opt, it has one
  // additional tuple for the min max values.
  ExCriDescPtr leftDownCriDesc_;                    // 352-369
  UInt16       hjFlags2_;                           // 370-371
  Float32      estMemoryUsage_;                     // 372-375
  Float32        bmoQuotaRatio_;
  

protected:
  inline Int32 isSemiJoin() const;
  inline Int32 isLeftJoin() const;
  inline Int32 isAntiSemiJoin() const;
  inline Int32 isNoOverflow() const;
  inline Int32 isReuse() const;
  inline Int32 isRightJoin() const;
  inline Int32 isFullJoin() const;

  Float32      innerExpectedRows() 
    {return getFloatValue((char*)&p_innerExpectedRows_);}
  Float32      outerExpectedRows() 
    {return getFloatValue((char*)&p_outerExpectedRows_);}

 public:
  void setBmoCitizenshipFactor(Float32 bmoCf)
    { bmoCitizenshipFactor_ = bmoCf; }
  Float32 getBmoCitizenshipFactor(void)
    { return bmoCitizenshipFactor_;}
  void setMemoryContingencyMB(Int32 mCMB)
    {  pMemoryContingencyMB_ = mCMB;} 
  Int32 getMemoryContingencyMB(void)
    { return pMemoryContingencyMB_; }

  void    setHjMemEstInKBPerNode(Float32 s) {hjMemEstInKBPerNode_=s;}
  Float32 getHjMemEstInKBPerNode() {return hjMemEstInKBPerNode_;}
  Float32 hjGrowthPercent() {return Float32(hjGrowthPercent_/100.0);}
  void  setBmoMinMemBeforePressureCheck(UInt16 m)
  { bmoMinMemBeforePressureCheck_ = m ; }
  UInt16  getBmoMinMemBeforePressureCheck()
  { return bmoMinMemBeforePressureCheck_; }
  void  setBMOMaxMemThresholdMB(UInt16 m)
  { bmoMaxMemThresholdMB_ = m ; }
  UInt16  getBMOMaxMemThresholdMB()
  { return bmoMaxMemThresholdMB_; }
  void setEstimatedMemoryUsage(Float32 estMemory)
    { estMemoryUsage_ = estMemory; }
  virtual Float32 getEstimatedMemoryUsage(void)
    { return estMemoryUsage_;}

  void setBmoQuotaRatio(Float32 bmoQuotaRatio)
    { bmoQuotaRatio_ = bmoQuotaRatio; }
  virtual Float32 getBmoQuotaRatio(void)
    { return bmoQuotaRatio_;}
};

inline ComTdb * ComTdbHashj::getLeftChildTdb() {
  return leftChildTdb_;
};

inline ComTdb * ComTdbHashj::getRightChildTdb() {
  return rightChildTdb_;
};

inline Int32 ComTdbHashj::isSemiJoin() const {
  return (hjFlags_ & SEMI_JOIN);
};

inline Int32 ComTdbHashj::isLeftJoin() const {
  return (hjFlags_ & LEFT_JOIN);
};

inline Int32 ComTdbHashj::isRightJoin() const {
  return (hjFlags_ & RIGHT_JOIN);
};

inline Int32 ComTdbHashj::isFullJoin() const {
  return (isLeftJoin() && isRightJoin());
};

inline Int32 ComTdbHashj::isAntiSemiJoin() const {
  return (hjFlags_ & ANTI_SEMI_JOIN);
};

inline Int32 ComTdbHashj::isNoOverflow() const {
  return (hjFlags_ & NO_OVERFLOW);
};

inline Int32 ComTdbHashj::isReuse() const {
  return (hjFlags_ & REUSE);
};

inline Int32 ComTdbHashj::orderedQueueProtocol() const {
  return -1; // return true
};

/*****************************************************************************
  Description : Return ComTdb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
  History     : Yeogirl Yun                                      8/22/95
                 Initial Revision.
*****************************************************************************/
inline const ComTdb* ComTdbHashj::getChild(Int32 pos) const {
  if (pos == 0)
    return leftChildTdb_;
  else if (pos == 1)
    return rightChildTdb_;
  else
    return NULL;
}

#endif
