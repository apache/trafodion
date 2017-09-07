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
* File:         ComTdbSplitTop.h
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

#ifndef COM_SPLIT_TOP_H
#define COM_SPLIT_TOP_H

#include "ComTdb.h"
#include "PartInputDataDesc.h"
#include "ComExtractInfo.h"

////////////////////////////////////////////////////////////////////////////
// Contents of this file
////////////////////////////////////////////////////////////////////////////

class ComTdbSplitTop;

////////////////////////////////////////////////////////////////////////////
// Forward references
////////////////////////////////////////////////////////////////////////////

class ExPartInputDataDesc;
//class ComTdbPartnAccess;

////////////////////////////////////////////////////////////////////////////
// Task Definition Block for split top node
////////////////////////////////////////////////////////////////////////////
class ComTdbSplitTop : public ComTdb
{
  friend class ex_split_top_tcb;
  friend class ex_split_top_private_state;

public:
  // Constructors
  ComTdbSplitTop() : ComTdb(ex_SPLIT_TOP,"FAKE") {}
  ComTdbSplitTop(ComTdb              *child,
		 ex_expr             *childInputPartFunction,
		 Lng32                inputPartAtpIndex,
		 ex_expr             *mergeKeyExpr,
		 Lng32                mergeKeyAtpIndex,
		 Lng32                mergeKeyLength,
		 ExPartInputDataDesc *partInputDataDesc,
		 Lng32                partInputDataAtpIndex,
		 Lng32                paPartNoAtpIndex,
		 ex_cri_desc         *givenCriDesc,
		 ex_cri_desc         *returnedCriDesc,
		 ex_cri_desc         *downCriDesc,
		 ex_cri_desc         *workCriDesc,
		 NABoolean           bufferedInserts,
		 queue_index         fromParent,
		 queue_index         toParent,
		 Cardinality         estimatedRowCount,
                 Lng32                bottomNumParts,
		 Int32	             streamTimeout,
		 Lng32                sidNumBuffers,
		 ULng32       sidBufferSize
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

  virtual short getClassSize()   { return (short)sizeof(ComTdbSplitTop); }
  
  Int32       orderedQueueProtocol() const;
  inline NABoolean needToSendInputData() const { return !partInputDataDesc_.isNull(); }
  
  const NABoolean bufferedInserts() const
  { return ((splitTopFlags_ & BUFFERED_INSERTS) != 0); }

  UInt32 isSystemIdentity() const { return splitTopFlags_ & SYSTEM_IDENTITY ;}
  void setSystemIdentity() { splitTopFlags_ |= SYSTEM_IDENTITY ;}

  UInt32 isLRUOperation() const { return splitTopFlags_ & LRU_OPERATION;}
  void setLRUOperation() { splitTopFlags_ |= LRU_OPERATION ;}

  Long      pack(void *);
  Lng32      unpack(void *, void * reallocator);
  
  void      display() const;

  Int32 getBottomNumParts() {return bottomNumParts_;}

  // for GUI
  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const;
  virtual const char *getNodeName() const { return "EX_SPLIT_TOP"; };
  virtual Int32 numExpressions() const;
  virtual ex_expr* getExpressionNode(Int32 pos);
  virtual const char * getExpressionName(Int32 pos) const;

  // for showplan
  virtual void displayContents(Space *space, ULng32 flag);

  // For parallel extract
  NABoolean getExtractProducerFlag() const 
  { return (splitTopFlags_ & EXTRACT_PRODUCER) ? TRUE : FALSE; }
  void setExtractProducerFlag() { splitTopFlags_ |= EXTRACT_PRODUCER; }
  NABoolean getExtractConsumerFlag() const 
  { return (splitTopFlags_ & EXTRACT_CONSUMER) ? TRUE : FALSE; }
  void setExtractConsumerFlag() { splitTopFlags_ |= EXTRACT_CONSUMER; }

  const char *getExtractSecurityKey() const
  { return (extractProducerInfo_ ?
            extractProducerInfo_->getSecurityKey() : NULL); }

  void setExtractProducerInfo(ComExtractProducerInfo *e)
  { extractProducerInfo_ = e; }

  NABoolean isMWayRepartition() const 
  { return (splitTopFlags_ & MWAY_REPARTITION) ? TRUE : FALSE; }
  void setMWayRepartitionFlag() {splitTopFlags_ |= MWAY_REPARTITION; }

  NABoolean isStaticPaAffinity() const 
  { return (splitTopFlags_ & STATIC_PA_AFFINATY) ? TRUE : FALSE; }
  void setStaticPaAffinity() {splitTopFlags_ |= STATIC_PA_AFFINATY; }

// LCOV_EXCL_START
  NABoolean getSetupSharedPool() const 
  { return (splitTopFlags_ & SETUP_SHARED_POOL) ? TRUE : FALSE; }
  void setSetupSharedPool() {splitTopFlags_ |= SETUP_SHARED_POOL; }
// these 3 lines won't be covered, feature not yet activated.
// LCOV_EXCL_STOP

  NABoolean getUseExtendedPState() const 
  { return (splitTopFlags_ & USE_EXTENDED_PSTATE) ? TRUE : FALSE; }
  void setUseExtendedPState() {splitTopFlags_ |= USE_EXTENDED_PSTATE; }

// LCOV_EXCL_START
  Int32 getNumSharedBuffers() const { return numSharedBuffs_; }
  void setNumSharedBuffers(Int32 n) {numSharedBuffs_ = n; }
// these 2 lines won't be covered, feature not yet activated.
// LCOV_EXCL_STOP

  // For SeaMonster
  NABoolean getExchangeUsesSM() const
  { return (splitTopFlags_ & SPLT_EXCH_USES_SM) ? TRUE : FALSE; }
  void setExchangeUsesSM() { splitTopFlags_ |= SPLT_EXCH_USES_SM; }

protected:

  enum split_top_flags { NO_FLAGS = 0x0000,
                         // this flag indicates if this PAPA node is
                         // used to send data to be inserted using
                         // VSBB or sidetree inserts.
                         BUFFERED_INSERTS = 0x0001,
                         // This flag is set when the system generates
                         // value for an IDENTITY column.
                         SYSTEM_IDENTITY  = 0x0002,
                         EXTRACT_PRODUCER = 0x0004,
                         EXTRACT_CONSUMER = 0x0008,
                         LRU_OPERATION = 0x0010,
                         // This flag is set when repartition from n-way
                         // to m-way, other partition schema are the same
                         MWAY_REPARTITION = 0x0020 ,
                         // Set this flag if this PAPA associate one PA
                         // to access a fixed set of partitions
	                 STATIC_PA_AFFINATY = 0x0040,
                         // Set this flag for this PAPA node to setup
                         // a buffer pool for all PA to use
                         SETUP_SHARED_POOL = 0x0080,
                         // This flag indicates if this SplitTop needs to use
                         // the extended version of the PState class.  The extended
                         // version contains state required for GET_NEXT_N processing.
                         // The non-extended version of the PState is much smaller
                         // and can save a lot of memory when very large queues are used.
                         USE_EXTENDED_PSTATE = 0x0100,
                         // flag to indicate if the split/send top is using
                         // SeaMonster to send data
                         SPLT_EXCH_USES_SM   = 0x0200,
  };

  UInt32       splitTopFlags_;                     // 00-03

  // number of bottom partitions (may be modified at runtime)  
  Int32        bottomNumParts_;                    // 04-07
  
  // an expression used to determine to which input partition to send
  // a particular input queue entry; in most cases this is NULL, meaning
  // that an input queue entry is sent to all input partitions
  ExExprPtr    childInputPartFunction_;            // 08-15
  Int32        inputPartAtpIndex_;                 // 16-19

  // the merge key expression helps in merging sorted input partition to a
  // sorted result stream of tuples, it encodes a binary key of length
  // mergeKeyLength_ for this child
  Int32        mergeKeyAtpIndex_;                  // 20-23
  ExExprPtr    mergeKeyExpr_;                      // 24-31
  Int32        mergeKeyLength_;                    // 32-35
  
  // If the split top node is responsible to pass partition input data
  // down to its child queues, a pointer to the partition input data
  // descriptor that describes the part. input values (fixed assignment)
  Int32                  partInputDataAtpIndex_;   // 36-39
  ExPartInputDataDescPtr partInputDataDesc_;       // 40-47

  // child tdb (gets replicated bottomNumESPs_ times)
  ComTdbPtr    child_;                             // 48-55
  
  ExCriDescPtr downCriDesc_;                       // 56-63                    
  ExCriDescPtr workCriDesc_;                       // 64-71

  // ATP index where the partition number for the PA child is put
  Int32        paPartNoAtpIndex_;                  // 72-75

  // BertBert VV
  // Timeout (.01 seconds) for waiting on a streaming cursor.
  // If streamTimeout_ == 0 then don't wait.
  // If streamTimeout_ < 0 then never timeout
  Int32 streamTimeout_;				   // 76-79
  // BertBert ^^

  ComExtractProducerInfoPtr extractProducerInfo_;  // 80-87

  // number of buffers in the shared pool, valid if SETUP_SHARED_POOL is set
  Int32        numSharedBuffs_;                    // 88-91
  char fillersComTdbSplitTop_[28];                 // 91-119

};


#endif /* EX_SPLIT_TOP_H */
