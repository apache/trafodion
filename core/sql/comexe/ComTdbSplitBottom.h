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
* File:         ComTdbSplitBottom.h
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

#ifndef COM_SPLIT_BOTTOM_H
#define COM_SPLIT_BOTTOM_H

#include "ComTdb.h"
#include "FragDir.h"
#include "ComTdbSendBottom.h"
#include "ComExtractInfo.h"

////////////////////////////////////////////////////////////////////////////
// contents of this file
////////////////////////////////////////////////////////////////////////////
class ComTdbSplitBottom;
class SplitBottomPartInfo;

////////////////////////////////////////////////////////////////////////////
// forward references
////////////////////////////////////////////////////////////////////////////
class ComTdbSendBottom;


////////////////////////////////////////////////////////////////////////////
// Class SplitBottomSkewInfo -- helps split bottom by encapsulating the
// skew buster information, including hash values
// of the partitioning keys containing values with very high occurence 
// frequency, either in the data stream that this split bottom is 
// producing, or in the relation to which this split bottom's data will be
// joined.
////////////////////////////////////////////////////////////////////////////

class SplitBottomSkewInfo : public NAVersionedObject
{
  friend class ComTdbSplitBottom;

public:
  SplitBottomSkewInfo () :
      NAVersionedObject(-1),
      numSkewHashValues_(0), 
      skewHashValues_(NULL) {}

  SplitBottomSkewInfo ( Int32 numSkewHashValues,
                        Int64 * skewHashValues ) :
      numSkewHashValues_(numSkewHashValues),
      skewHashValues_(skewHashValues) {}

   const Int64 *getSkewHashValues(void) { return skewHashValues_; };

   const Int32 getNumSkewHashValues(void) { return numSkewHashValues_; };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(SplitBottomSkewInfo); }


  Long      pack(void *);
  Lng32      unpack(void * base, void * reallocator);

private:
  Int32                  numSkewHashValues_;                // 00-03
  char                   fillersSplitBottomSkewInfo_[4];    // 04-07
  Int64Ptr               skewHashValues_;                   // 08-15
  char                   fillersSplitBottomSkewInfo2_[48];  // 16-63
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for SplitBottomSkewInfo
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<SplitBottomSkewInfo> SplitBottomSkewInfoPtr;

////////////////////////////////////////////////////////////////////////////
// Task Definition Block for split bottom node
////////////////////////////////////////////////////////////////////////////
class ComTdbSplitBottom : public ComTdb
{
  friend class ex_split_bottom_tcb;

public:
  // Constructors
  ComTdbSplitBottom() : ComTdb(ex_SPLIT_BOTTOM,"FAKE") {}

  // real constructor, used by generator
  ComTdbSplitBottom(ComTdb             *child,
		    ComTdbSendBottom *sendTdb,
		    ex_expr            *partFunction,
		    Lng32               partNoATPIndex,
                    Lng32               partFunctionUsesNarrow,
		    Lng32               conversionErrorFlagATPIndex,
		    Lng32               partInputATPIndex,
		    Lng32               partInputDataLen,
		    Cardinality        estimatedRowCount,
		    ex_cri_desc        *givenCriDesc,
		    ex_cri_desc        *returnedCriDesc,
		    ex_cri_desc        *workCriDesc,
		    NABoolean          combineRequests,
		    Lng32               topNumESPs,
		    Lng32               topNumParts,
		    Lng32               bottomNumESPs,
		    Lng32               bottomNumParts,
                    SplitBottomSkewInfo *skewInfo
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

  virtual short getClassSize() { return (short)sizeof(ComTdbSplitBottom); }
  
  Int32       orderedQueueProtocol() const;

  inline Lng32 getPartInputDataLen() const    { return partInputDataLen_; }

  Long      pack(void *);
  Lng32      unpack(void *, void * reallocator);
  
  void      display() const;

  ComTdbSendBottom * getSendBottomTdb() const         { return sendTdb_; }

  inline ULng32 getPlanVersion() const         { return planVersion_; }

  virtual void setPlanVersion(UInt32 value)     { planVersion_ = value; }

  NABoolean useSkewBuster() const
                      { return (splitBottomFlags_ & SKEWBUSTER) != 0; }

  void setUseSkewBuster(NABoolean v)
  { (v ? splitBottomFlags_ |= SKEWBUSTER : splitBottomFlags_ &= ~SKEWBUSTER); }

  NABoolean doBroadcastSkew() const
                      {  return (splitBottomFlags_ & SKEW_BROADCAST) != 0; }

  void setBroadcastSkew(NABoolean v)
  { (v ? splitBottomFlags_ |= SKEW_BROADCAST :
         splitBottomFlags_ &= ~SKEW_BROADCAST); }

  NABoolean getBroadcastOneRow() const   
                      {  return (splitBottomFlags_ & ONE_ROW_BROADCAST) != 0; }

  void setBroadcastOneRow(NABoolean v) 
  { (v ? splitBottomFlags_ |= ONE_ROW_BROADCAST : 
         splitBottomFlags_ &= ~ONE_ROW_BROADCAST); }

  NABoolean forceSkewRoundRobin() const   
                      { return (splitBottomFlags_ & SKEW_FORCE_RR) != 0; }

  void setForceSkewRoundRobin(NABoolean v) 
  { (v ? splitBottomFlags_ |= SKEW_FORCE_RR: 
         splitBottomFlags_ &= ~SKEW_FORCE_RR); }

  // For SeaMonster
  // * The "exchange uses SM" flag means this exchange uses SeaMonster
  // * The "query uses SM" flag means SeaMonster is used somewhere in 
  //   the query but not necessarily in this fragment
  NABoolean getExchangeUsesSM() const
  { return (splitBottomFlags_ & SPLB_EXCH_USES_SM) ? TRUE : FALSE; }
  void setExchangeUsesSM() { splitBottomFlags_ |= SPLB_EXCH_USES_SM; }

  NABoolean getQueryUsesSM() const
  { return (splitBottomFlags_ & SPLB_QUERY_USES_SM) ? TRUE : FALSE; }
  void setQueryUsesSM() { splitBottomFlags_ |= SPLB_QUERY_USES_SM; }

  Int32 getInitialRoundRobin() const { return initialRoundRobin_; }
  
  void setInitialRoundRobin(Int32 ir) { initialRoundRobin_ = ir; }

  Int32 getFinalRoundRobin() const { return finalRoundRobin_; }
  
  void setFinalRoundRobin(Int32 fr) { finalRoundRobin_ = fr; }

  // These functions allow split bottom tcb initialization methods to read
  // the hash values for the skewed partitioning key values.
  SplitBottomSkewInfo *getSkewInfo()                     { return skewInfo_; }
  const Int32 getNumSkewValues(void) 
                                 { return getSkewInfo()->numSkewHashValues_; }
// those 3 lines won't be covered, code nowhere used
  Int64 const * hashValueArray(void) 
                                    { return getSkewInfo()->skewHashValues_; }

  // for GUI
  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const;
  virtual const char *getNodeName() const { return "EX_SPLIT_BOTTOM"; };
  virtual Int32 numExpressions() const;
  virtual ex_expr* getExpressionNode(Int32 pos);
  virtual const char * getExpressionName(Int32 pos) const;
  
  // for showplan
  virtual void displayContents(Space *space,ULng32 flag);

  // For parallel extract
  NABoolean getExtractProducerFlag() const 
  { return (splitBottomFlags_ & EXTRACT_PRODUCER) ? TRUE : FALSE; }
  void setExtractProducerFlag() { splitBottomFlags_ |= EXTRACT_PRODUCER; }

  const char *getExtractSecurityKey() const
  { return (extractProducerInfo_ ?
            extractProducerInfo_->getSecurityKey() : NULL); }

  void setExtractProducerInfo(ComExtractProducerInfo *e)
  { extractProducerInfo_ = e; }

  NABoolean isMWayRepartition() const
  { return (splitBottomFlags_ & MWAY_REPARTITION) ? TRUE : FALSE; }
  void setMWayRepartitionFlag() {splitBottomFlags_ |= MWAY_REPARTITION; }

  NABoolean isAnESPAccess() const 
  { return (splitBottomFlags_ & ESP_ACCESS) ? TRUE : FALSE; }
  void setIsAnESPAccess() {splitBottomFlags_ |= ESP_ACCESS; }

  NABoolean getQueryLimitDebug() const
  { return (splitBottomFlags_ & QUERY_LIMIT_DEBUG) ? TRUE: FALSE; }
  void setQueryLimitDebug() {splitBottomFlags_ |= QUERY_LIMIT_DEBUG; }

  enum {
    NO_ABEND = 0,
    SIGNED_OVERFLOW,
    ASSERT,
    INVALID_MEMORY,
    SLEEP180,
    INTERNAL_ERROR,
    TEST_LOG4CXX
    };

  Int32 getAbendType(void) const { return abendType_; }

  void setAbendType(Int32 a) { abendType_ = a; }

  void setCpuLimit(Int64 cpuLimit) { cpuLimit_ = cpuLimit; }

  void setCpuLimitCheckFreq(Int32 f) { cpuLimitCheckFreq_ = f; }

protected:

  enum split_bottom_flags { SKEWBUSTER            = 0x0001
                          , SKEW_BROADCAST        = 0x0002 
                          , SKEW_FORCE_RR         = 0x0004
                          , EXTRACT_PRODUCER      = 0x0008
                          , MWAY_REPARTITION      = 0x0010
                          , ESP_ACCESS            = 0x0020
                          , QUERY_LIMIT_DEBUG     = 0x0040
			  , ONE_ROW_BROADCAST     = 0x0080
			  , SPLB_QUERY_USES_SM    = 0x0100
			  , SPLB_EXCH_USES_SM     = 0x0200
                          };
  
  // the combination of a split top / split bottom node maps bottomNumParts_
  // partitions from bottomNumESPs_ processes into topNumParts_ partitions
  // in topNumESPs_ processes.
  Int32                 topNumESPs_;                      // 00-03
  Int32                 topNumParts_;                     // 04-07
  Int32                 bottomNumESPs_;                   // 08-11
  Int32                 bottomNumParts_;                  // 12-15
    
  // an expression used to determine to which output partition to send
  // a value that came from our child's up queue (if partFunction_ is not
  // existent then the value is sent to all output partitions that generated
  // the corresponding input queue entry)
  ExExprPtr             partFunction_;                    // 16-23
  Int32                 partNoATPIndex_;                  // 24-27

  // does the partitioning function possibly fail with a data conversion
  // error?
  Int32                 partFuncUsesNarrow_; // boolean value; 28-31
  Int32                 convErrorATPIndex_;               // 32-35

  // info on where the partition input tupp is stored in the work atp
  Int32                 partInputATPIndex_;               // 36-39

  // record descriptions of local ATPs
  ExCriDescPtr          workCriDesc_;                     // 40-47

  // the partition input values are values sent by some process, identifying
  // the partition that this ESP is working on (this may be a partition number
  // or a begin/end key pair); the partition input values are passed on
  // to our child as an input value
  Int32                 partInputDataLen_;                // 48-51

  // The split bottom node can treat each request coming in as a separate
  // job and execute it. Most of the time, however, all requesters send a
  // request synchronously and the split bottom node is supposed to process
  // all those (identical) requests as a single unit.
  Int32                 combineRequests_;                 // 52-55

  ComTdbPtr             child_;                           // 56-63

  // node to communicate with requester
  ComTdbSendBottomPtr   sendTdb_;                         // 64-71

  UInt16                splitBottomFlags_;                // 72-73

  Int16                 initialRoundRobin_;               // 74-75

  UInt32                planVersion_;                     // 76-79
 
  

  SplitBottomSkewInfoPtr skewInfo_;                       // 80-87

  ComExtractProducerInfoPtr extractProducerInfo_;         // 88-95

  Int32                 abendType_;                       // 96-99

  UInt16                finalRoundRobin_ ;                // 100-101
  Int16                 cpuLimitCheckFreq_;               // 102-103
  Int64                 cpuLimit_;                        // 104-111
  char                  fillersComTdbSplitBottom_[8];     // 112-119

};

#endif /* EX_SPLIT_BOTTOM_H */
