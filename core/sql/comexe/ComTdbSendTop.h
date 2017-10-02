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
* File:         ComTdbSendTop.h
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

#ifndef COM_SEND_TOP_H
#define COM_SEND_TOP_H

#include "ComTdb.h"
#include "FragDir.h"
#include "ComExtractInfo.h"

class ExExeStmtGlobals;

////////////////////////////////////////////////////////////////////////////
// Task Definition Block for send top node
////////////////////////////////////////////////////////////////////////////
class ComTdbSendTop : public ComTdb
{
  friend class ex_send_top_tcb;

public:
  // Constructors
  ComTdbSendTop() : ComTdb(ex_SEND_TOP,"FAKE") {}
  ComTdbSendTop(ExFragId         childFragId,
		  ex_expr            *moveInputValues,
		  ex_cri_desc        *givenCriDesc,
		  ex_cri_desc        *returnedCriDesc,
		  ex_cri_desc        *downRecordCriDesc,
		  ex_cri_desc        *upRecordCriDesc,
		  ex_cri_desc        *workCriDesc,
		  Lng32               moveExprTuppIndex,
		  queue_index        fromParent,
		  queue_index        toParent,
		  Lng32               downRecordLength,
		  Lng32               upRecordLength,
		  Lng32	             sendBufferSize,
		  Lng32               numSendBuffers,
		  Lng32               recvBufferSize,
		  Lng32               numRecvBuffers,
		  Cardinality        estNumRowsSent,
		  Cardinality        estNumRowsRecvd,
                  NABoolean          logDiagnostics);

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

  virtual short getClassSize()   { return (short)sizeof(ComTdbSendTop); }
  
  // send top nodes are usually built such that many tcbs exist for a
  // single tdb and that each tcb can use a different form of communication
  //		  long myInstanceNum,
	//		  long childInstanceNum);

  Int32       orderedQueueProtocol() const;
  
  Long      pack(void *);
  Lng32      unpack(void *, void * reallocator);
  
  void      display() const;

  inline Lng32 getDownRecordLength() const   { return downRecordLength_; }
  inline Lng32 getUpRecordLength() const       { return upRecordLength_; }
  inline ExFragId getChildFragId() const         { return childFragId_; }

  inline Lng32 getSendBufferSize() const       { return sendBufferSize_; }
  inline Lng32 getNumSendBuffers() const       { return numSendBuffers_; }
  inline Lng32 getRecvBufferSize() const       { return recvBufferSize_; }
  inline Lng32 getNumRecvBuffers() const       { return numRecvBuffers_; }

  // static methods to help optimizer calculate message buffer sizes
  // before generating a send top/send bottom node pair
  static Lng32 minSendBufferSize(Lng32 downRecLen, Lng32 numRecs = 1);
  static Lng32 minReceiveBufferSize(Lng32 upRecLen, Lng32 numRecs = 1);

  inline NABoolean logDiagnostics() const 
                       { return (sendTopFlags_ & LOG_DIAGNOSTICS) != 0; } 

  // for GUI
  virtual const ComTdb* getChild(Int32 pos) const;
  virtual const ComTdb* getChildForGUI(
	Int32 pos, Lng32 base,
	void * frag_dir) const;
  virtual Int32 numChildren() const;
virtual const char *getNodeName() const { return "EX_SEND_TOP"; };
  virtual Int32 numExpressions() const;
  virtual ex_expr* getExpressionNode(Int32 pos);
  virtual const char * getExpressionName(Int32 pos) const;

  // for showplan
  virtual void displayContents(Space *space, ULng32 flag);

  // For parallel extract
  NABoolean getExtractProducerFlag() const 
  { return (sendTopFlags_ & EXTRACT_PRODUCER) ? TRUE : FALSE; }
  void setExtractProducerFlag() { sendTopFlags_ |= EXTRACT_PRODUCER; }
  NABoolean getExtractConsumerFlag() const 
  { return (sendTopFlags_ & EXTRACT_CONSUMER) ? TRUE : FALSE; }
  void setExtractConsumerFlag() { sendTopFlags_ |= EXTRACT_CONSUMER; }

  const char *getExtractEsp() const
  { return (extractConsumerInfo_ ?
            extractConsumerInfo_->getEspPhandle() : NULL); }
  const char *getExtractSecurityKey() const
  { return (extractConsumerInfo_ ?
            extractConsumerInfo_->getSecurityKey() : NULL); }

  void setExtractConsumerInfo(ComExtractConsumerInfo *e)
  { extractConsumerInfo_ = e; }

  // For SeaMonster: Does the exchange use SeaMonster?
  NABoolean getExchangeUsesSM() const
  { return (sendTopFlags_ & EXCHANGE_USES_SM) ? TRUE : FALSE; }
  void setExchangeUsesSM() { sendTopFlags_ |= EXCHANGE_USES_SM; }

  // For SeaMonster: The SM tag is either an EXPLAIN ID or, if EXPLAIN
  // is disabled, an integer assigned to this exchange. The tag is
  // always unique within the plan.
  Int32 getSMTag() const { return smTag_; }
  void setSMTag(Int32 tag) { smTag_ = tag; }

  // Should the send top TCB restrict the number of outstanding send
  // buffers to 1. The restriction was enforced unconditionally prior
  // to SeaMonster. With SeaMonster we want to test different values
  // and this attribute allows the restriction to be lifted by setting
  // CQD GEN_SNDT_RESTRICT_SEND_BUFFERS 'OFF'.
  NABoolean getRestrictSendBuffers() const
  { return (sendTopFlags_ & RESTRICT_SENDBUFS) ? TRUE : FALSE; }
  void setRestrictSendBuffers() { sendTopFlags_ |= RESTRICT_SENDBUFS; }

  NABoolean getUseOldStatsNoWaitDepth() const 
  { return (sendTopFlags_ & STATS_NOWAIT_DEP) ? TRUE : FALSE; }
  void setUseOldStatsNoWaitDepth() { sendTopFlags_ |= STATS_NOWAIT_DEP; }

protected:

  enum send_top_flags {
    NO_FLAGS             = 0x0000,
    LOG_DIAGNOSTICS      = 0x0001,
    EXTRACT_PRODUCER     = 0x0002,
    EXTRACT_CONSUMER     = 0x0004,
    STATS_NOWAIT_DEP     = 0x0008,
    EXCHANGE_USES_SM     = 0x0010,
    RESTRICT_SENDBUFS    = 0x0020
  };
  
  UInt32         sendTopFlags_;             // 00-03
  
  // child fragment id
  UInt32         childFragId_;              // 04-07
  
  // an expression to move input values into a contiguous part of a
  // message buffer
  ExExprPtr      moveInputValues_;          // 08-15

  ExCriDescPtr   givenCriDesc_;             // 16-23
  ExCriDescPtr   returnedCriDesc_;          // 24-31
  ExCriDescPtr   downRecordCriDesc_;        // 32-39
  ExCriDescPtr   upRecordCriDesc_;          // 40-47
  ExCriDescPtr   workCriDesc_;              // 48-55
  
  // copied input row's pos. in workAtp
  Int32          moveExprTuppIndex_;        // 56-59

  // guidelines for the executor's runtime parameters

  // recommend size of queue from parent
  UInt32         fromParent_;               // 60-63

  // recommend size of queue to parent
  UInt32         toParent_;                 // 64-67
  
  // length of input or output row
  Int32          downRecordLength_;         // 68-71
  Int32          upRecordLength_;           // 72-75

  // recommended size of each buffer
  Int32          sendBufferSize_;           // 76-79

  // total no of buffers to allocate
  Int32          numSendBuffers_;           // 80-83

  // same for receive buffers
  Int32          recvBufferSize_;           // 84-87
  Int32          numRecvBuffers_;           // 88-91

private:
  // compiler estimate for # input rows
  Float32          p_estNumRowsSent_;       // 92-95

  // est. # returned rows per input row
  Float32          p_estNumRowsRecvd_;      // 96-99

protected:
  Int32          smTag_;                    // 100-103
  ComExtractConsumerInfoPtr
                 extractConsumerInfo_;      // 104-111
  char           fillersComTdbSendTop_[24]; // 112-135
};


#endif /* EX_SEND_TOP_H */
