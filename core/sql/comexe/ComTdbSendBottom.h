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
* File:         ComTdbSendBottom.h
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

#ifndef COM_SEND_BOTTOM_H
#define COM_SEND_BOTTOM_H

#include "ComTdb.h"
#include "FragInstanceHandle.h"

////////////////////////////////////////////////////////////////////////////
// forward declarations
////////////////////////////////////////////////////////////////////////////
//class ExSendBottomMessageStream;
class ExExeStmtGlobals;
class ExEspFragInstanceDir;
class ExFragKey;

////////////////////////////////////////////////////////////////////////////
// Task Definition Block for send top node
////////////////////////////////////////////////////////////////////////////
class ComTdbSendBottom : public ComTdb
{
  friend class ex_send_bottom_tcb;

public:
  // Constructors
  ComTdbSendBottom() : ComTdb(ex_SEND_BOTTOM,"FAKE") {}
  ComTdbSendBottom(ex_expr      *moveOutputValues,
		     queue_index  toSplit,
		     queue_index  fromSplit,
		     ex_cri_desc  *downCriDesc,
		     ex_cri_desc  *upCriDesc,
		     ex_cri_desc  *workCriDesc,
		     Lng32         moveExprTuppIndex,
		     Lng32         downRecordLength,
		     Lng32         upRecordLength,
		     Lng32         requestBufferSize,
		     Lng32         numRequestBuffers,
		     Lng32	  replyBufferSize,
		     Lng32         numReplyBuffers,
		     Cardinality  estNumRowsRequested,
		     Cardinality  estNumRowsReplied);

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

  virtual short getClassSize()     { return (short)sizeof(ComTdbSendBottom); }
  
  Int32       orderedQueueProtocol() const;
  
  Long      pack(void *);
  Lng32      unpack(void *, void * reallocator);
  
  void      display() const;

  Lng32      getDownRecordLength() const          { return downRecordLength_; }
  Lng32      getUpRecordLength() const              { return upRecordLength_; }
  Lng32      getRequestBufferSize() const        { return requestBufferSize_; }
  Lng32      getNumRequestBuffers() const        { return numRequestBuffers_; }
  Lng32      getReplyBufferSize() const            { return replyBufferSize_; }
  Lng32      getNumReplyBuffers() const            { return numReplyBuffers_; }

  // for GUI
  virtual const ComTdb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const;
virtual const char *getNodeName() const { return "EX_SEND_BOTTOM"; };
  virtual Int32 numExpressions() const;
  virtual ex_expr* getExpressionNode(Int32 pos);
  virtual const char * getExpressionName(Int32 pos) const ;

  virtual ex_send_bottom_tcb * buildInstance(ExExeStmtGlobals * glob,
                                     ExEspFragInstanceDir *espInstanceDir,
                                     const ExFragKey &myKey,
                                     const ExFragKey &parentKey,
                                     ExFragInstanceHandle myHandle,
                                     Lng32 parentInstanceNum,
                                     NABoolean isLocal)       { return NULL; }
// its executor twin is used, ignore

  // For SHOWPLAN
  virtual void displayContents(Space *space, ULng32 flag);

  // For parallel extract
  NABoolean getExtractProducerFlag() const 
  { return (sendBottomFlags_ & EXTRACT_PRODUCER) ? TRUE : FALSE; }
  void setExtractProducerFlag() { sendBottomFlags_ |= EXTRACT_PRODUCER; }

  // For SeaMonster
  NABoolean getExchangeUsesSM() const
  { return (sendBottomFlags_ & SNDB_EXCH_USES_SM) ? TRUE : FALSE; }
  void setExchangeUsesSM() { sendBottomFlags_ |= SNDB_EXCH_USES_SM; }

  Int32 getSMTag() const { return smTag_; }
  void setSMTag(Int32 tag) { smTag_ = tag; }

  NABoolean considerBufferDefrag() const
  { return (sendBottomFlags_ & CONSIDER_BUFFER_DEFRAG) ? TRUE : FALSE; }
  void setConsiderBufferDefrag(NABoolean v )
  {
    (v ? sendBottomFlags_ |= CONSIDER_BUFFER_DEFRAG :
         sendBottomFlags_ &= ~CONSIDER_BUFFER_DEFRAG);

  }

protected:

  enum send_bottom_flags { NO_FLAGS                = 0x0000,
                           EXTRACT_PRODUCER        = 0x0001,
                           CONSIDER_BUFFER_DEFRAG  = 0x0002,
                           SNDB_EXCH_USES_SM       = 0x0004
                         };
  
  ExCriDescPtr   workCriDesc_;                      // 00-07

  // move child's outputs to a contiguous buffer to be sent back to parent
  ExExprPtr      moveOutputValues_;                 // 08-15
  
  UInt32 sendBottomFlags_;                          // 16-19
  
  // copied input row's pos. in workAtp
  Int32          moveExprTuppIndex_;                // 20-23

  // guidelines for the executor's runtime parameters

  // length of input or output row
  Int32          downRecordLength_;                 // 24-27
  Int32          upRecordLength_;                   // 28-31

  // recommended size of each buffer
  Int32          requestBufferSize_;                // 32-35

  // total no of buffers to allocate
  Int32          numRequestBuffers_;                // 36-39

  // same for receive buffers
  Int32          replyBufferSize_;                  // 40-43

  Int32          numReplyBuffers_;                  // 44-47

private:
  // compiler estimate for # input rows
  Float32        p_estNumRowsRequested_;            // 48-51

  // est. # returned rows per input row
  Float32        p_estNumRowsReplied_;              // 52-55

protected:
  Int32          smTag_;                            // 56-59
  char           fillersComTdbSendBottom_[36];      // 60-95
  
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ComTdbSendBottom
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<ComTdbSendBottom> ComTdbSendBottomPtr;


#endif /* EX_SEND_BOTTOM_H */
