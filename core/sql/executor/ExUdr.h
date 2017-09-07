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
#ifndef __EX_UDR_H
#define __EX_UDR_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExUdr.h
 * Description:  TDB/TCB for user-defined routines
 *
 * Created:      2/8/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "ComTdbUdr.h"
#include "ex_tcb.h"
#include "UdrExeIpc.h"
#include "ComSmallDefs.h"
#include "ExStats.h"

// -----------------------------------------------------------------------
// Forward class declarations
// -----------------------------------------------------------------------
class IpcEnvironment;
class UdrMessageObj;
enum UdrIpcObjectType;
class UdrClientDataStream;
class UdrClientControlStream;
class UdrDataBuffer;
class sql_buffer;
class ExExeStmtGlobals;
class ExUdrServer;
class ExRsInfo;
class ExUDRBaseStats;
class ExUDRStats;

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExUdrTdb;
class ExUdrTcb;


// -----------------------------------------------------------------------
// ExUdrTdb
// -----------------------------------------------------------------------
class ExUdrTdb : public ComTdbUdr
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExUdrTdb()
  {
  }

  virtual ~ExUdrTdb()
  {
  }

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

  // ---------------------------------------------------------------------
  // Public accessor functions
  // ---------------------------------------------------------------------

  // CRI desc for the work ATP. Will be NULL if the UDR
  // has no input or output parameters.
  inline ex_cri_desc *getWorkCriDesc() const
  {
    return workCriDesc_;
  }

  // SQL name and all external names
  inline const char *getSqlName() const
  {
    return sqlName_;
  }
  inline const char *getRoutineName() const
  {
    return routineName_;
  }
  inline const char *getSignature() const
  {
    return routineSignature_;
  }
  inline const char *getContainerName() const
  {
    return containerName_;
  }
  inline const char *getPathName() const
  {
    return externalPath_;
  }
  inline const char *getLibrarySqlName() const
  {
    return librarySqlName_;
  }

  // Number of parameters and result sets
  inline ULng32 getNumParams() const
  {
    return numParams_;
  }
  inline ULng32 getNumInputValues() const
  {
    return numInputValues_;
  }
  inline ULng32 getNumOutputValues() const
  {
    return numOutputValues_;
  }
  inline ULng32 getMaxResultSets() const
  {
    return maxResultSets_;
  }

  // UDR flags
  inline ULng32 getUdrFlags() const
  {
    return flags_;
  }

  NABoolean isResultSetProxy() const
  {
    return ((flags_ & UDR_RESULT_SET) ? TRUE : FALSE);
  }
  NABoolean isTmudf() const { return flags_&UDR_TMUDF ? TRUE : FALSE ;}
  // Other UDR metadata
  inline ULng32 getStateAreaSize() const
  {
    return stateAreaSize_;
  }
  inline ComRoutineType getUdrType() const
  {
    return (ComRoutineType) udrType_;
  }
  inline ComRoutineLanguage getLanguage() const
  {
    return (ComRoutineLanguage) languageType_;
  }
  inline ComRoutineParamStyle getParamStyle() const
  {
    return (ComRoutineParamStyle) paramStyle_;
  }
  inline ComRoutineExternalSecurity getExternalSecurity() const
  {
    return (ComRoutineExternalSecurity) externalSecurity_;
  }
  inline Int32 getRoutineOwnerId() const
  {
    return routineOwnerId_;
  }
  inline ComRoutineSQLAccess getSqlAccessMode() const
  {
    return (ComRoutineSQLAccess) sqlAccessMode_;
  }
  inline ComRoutineTransactionAttributes getTransactionAttrs() const
  {
    return (ComRoutineTransactionAttributes) transactionAttrs_;
  }

  // Expressions to copy data values to/from message buffers
  inline ex_expr *getInputExpression() const
  {
    return inputExpr_;
  }
  inline ex_expr *getOutputExpression() const
  {
    return outputExpr_;
  }
  // expression for copying child table input into a sqlbuffer
  inline ex_expr *getChildInputExpr(Int32 pos) const
  {
    return childInputExprs_[pos];
  }

  // Predicate expression
  ex_expr *getPredicate() const { return scanExpr_; }

  // Defaults for the output buffer pool
  inline ULng32 getNumOutputBuffers() const
  {
    return numBuffers_; // this field comes from the superclass
  }
  inline ULng32 getOutputSqlBufferSize() const
  {
    return bufferSize_; // this field comes from the superclass
  }

  // Defaults for the input buffer pool
  inline ULng32 getNumInputBuffers() const
  {
    return numChildTableInputs_; // each child input needs one buffer
  }
  inline ULng32 getInputSqlBufferSize() const
  {
    return bufferSize_; // this field comes from the superclass
  }                        // keep it the same as output bufefr size for now
  // Default size of sql_buffers in message objects
  inline ULng32 getRequestSqlBufferSize() const
  {
    return requestSqlBufferSize_;
  }
  inline ULng32 getReplySqlBufferSize() const
  {
    return replySqlBufferSize_;
  }

  // Size of a single request/reply/output row
  inline ULng32 getRequestRowLen() const
  {
    return requestRowLen_;
  }
  inline ULng32 getReplyRowLen() const
  {
    return replyRowLen_;
  }
  inline ULng32 getOutputRowLen() const
  {
    return outputRowLen_;
  }

  // Attributes for input and output parameters. No checks
  // for NULL pointers are done and no bounds checking is done.
  // Only call these functions if you are sure workCriDesc_
  // exists and the index is valid.
  inline Attributes *getRequestAttr(UInt32 i) const
  {
    return workCriDesc_->getTupleDescriptor(requestTuppIndex_)->getAttr(i);
  }
  inline AttributesPtr *getRequestAttrs() const
  {
    return workCriDesc_->getTupleDescriptor(requestTuppIndex_)->attrs();
  }
  inline Attributes *getReplyAttr(UInt32 i) const
  {
    return workCriDesc_->getTupleDescriptor(replyTuppIndex_)->getAttr(i);
  }
  inline AttributesPtr *getReplyAttrs() const
  {
    return workCriDesc_->getTupleDescriptor(replyTuppIndex_)->attrs();
  }
  inline Attributes *getChildTableAttr(UInt32 tabInd, 
				       UInt32 colInd) const
  {
    UInt32 childTuppIndex = udrChildTableDescInfo_[tabInd]->
      getOutputTuppIndex();
    return workCriDesc_->getTupleDescriptor(childTuppIndex)->getAttr(colInd);
  }
  
  // Tuple descriptors for the input and output rows
  inline ExpTupleDesc *getRequestTuple() const
  {
    return workCriDesc_->getTupleDescriptor(requestTuppIndex_);
  }
  inline ExpTupleDesc *getReplyTuple() const
  {
    return workCriDesc_->getTupleDescriptor(replyTuppIndex_);
  }
  inline ExpTupleDesc *getChildTuple(UInt32 tabInd) const
  {
     UInt32 childTuppIndex = udrChildTableDescInfo_[tabInd]->
      getOutputTuppIndex();
    return workCriDesc_->getTupleDescriptor(childTuppIndex);
  }

  // Number of tupps in the output row
  inline unsigned short getNumOutputTupps() const
  {
    return criDescUp_->noTuples();
  }

  // ATP index in the work ATP for the input and output rows
  inline unsigned short getRequestTuppIndex() const
  {
    return requestTuppIndex_;
  }
  inline unsigned short getReplyTuppIndex() const
  {
    return replyTuppIndex_;
  }
 
  inline const UdrTableDescInfo *getTableDescInfo(UInt32 i) const
  {
    return udrChildTableDescInfo_[i];
  }

  inline Int32 getJavaDebugPort() const
  {
    return javaDebugPort_;
  }

  inline Int32 getJavaDebugTimeout() const
  {
    return javaDebugTimeout_;
  }
 
  
 
private:
  // ---------------------------------------------------------------------
  // !!!!!!! IMPORTANT -- NO DATA MEMBERS ALLOWED IN EXECUTOR TDB !!!!!!!!
  // *********************************************************************
  // The Executor TDB's are only used for the sole purpose of providing a
  // way to supplement the Compiler TDB's (in comexe) with methods whose
  // implementation depends on Executor objects. This is done so as to
  // decouple the Compiler from linking in Executor objects unnecessarily.
  //
  // When a Compiler generated TDB arrives at the Executor, the same data
  // image is "cast" as an Executor TDB after unpacking. Therefore, it is
  // a requirement that a Compiler TDB has the same object layout as its
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the ComTdbUdr instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//----------------------------------------------------------------------
// Task control block
//----------------------------------------------------------------------
class ExUdrTcb : public ex_tcb
{
  typedef ex_tcb super;
  friend class ExUdrTdb;

public:

  //
  // State transitions for a down queue entry
  //   NOT_STARTED ->
  //       { STARTED, CANCEL_BEFORE_SEND } ->
  //           NOT_STARTED
  //   NOT_STARTED -> STARTED -> CANCEL_AFTER_SEND -> NOT_STARTED
  //   NOT_STARTED -> STARTED -> PRODUCE_ERROR_REPLY -> PRODUCE_EOD_AFTER_ERROR -> NOT_STARTED
  //
  enum UdrTcbSendStep
  {
    NOT_STARTED,
    STARTED,
    CANCEL_BEFORE_SEND,
    CANCEL_AFTER_SEND,
    PRODUCE_ERROR_REPLY,
    PRODUCE_EOD_AFTER_ERROR
  };
  enum TmudfState
    {
      INITIAL =0,
      READING_FROM_CHILD
    };
  ExUdrTcb(const ExUdrTdb &tdb, 
	   const ex_tcb **childTcbs,
	   ex_globals *glob);
  ~ExUdrTcb();

  virtual void freeResources();

  // ---------------------------------------------------------------------
  // Standard TCB methods
  // ---------------------------------------------------------------------

  virtual Int32 fixup();
  virtual ExWorkProcRetcode work();
  ExWorkProcRetcode tmudfWork();
  ExWorkProcRetcode buildAndSendTmudfInput();
  ExWorkProcRetcode tmudfCheckSend();
  ExWorkProcRetcode tmudfCheckReceive();
  void registerSubtasks();

  ex_queue_pair getParentQueue() const
  {
    return qParent_;
  }

  Int32 numChildren() const
  {
    return myTdb().numChildTableInputs_;
  }

  const ex_tcb *getChild(Int32 pos) const
  {
    ex_assert((pos >= 0) && (pos < numChildren()), ""); 
    return childTcbs_[pos];

  }

  ex_tcb_private_state *allocatePstates(
    Lng32 &numElems,      // [IN/OUT] desired/actual elements
    Lng32 &pstateLength); // [OUT] length of one element

  //
  // IPC callback functions communicate with the TCB by calling
  // these methods
  //
  inline NABoolean getIpcBroken() const { return (state_ == IPC_ERROR); }
  void reportLoadReply(NABoolean loadWasSuccessful);
  void reportIpcError(IpcMessageStreamBase *s, IpcConnection *connection);
  void reportDataArrival();

  //
  // Public methods that allow data streams to allocate objects
  // and diagnostics on various TCB heaps
  //
  CollHeap *getIpcHeap() const;
  ComDiagsArea *getStatementDiags() const;
  void setStatementDiags(ComDiagsArea *) const;
  ComDiagsArea *getOrCreateStmtDiags() const;

  //
  // Stats area
  //

  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap, ComTdb *tdb);
  void incReplyMsg(Int64 msgBytes)
  {
    if (udrBaseStats_)
      udrBaseStats_->incReplyMsg(msgBytes);
  }

  void incReqMsg(Int64 msgBytes)
  {
    if (udrBaseStats_)
      udrBaseStats_->incReqMsg(msgBytes);
  }

  void setTmUdfInfo(UdrLoadMsg *lm, const ExUdrTdb &udrTdb);

protected:

  inline const ExUdrTdb &myTdb() const { return (const ExUdrTdb &) tdb; }
  ExExeStmtGlobals *myExeStmtGlobals() const;
  IpcEnvironment *myIpcEnv() const;

  // ---------------------------------------------------------------------
  // Work and cancel subtasks
  // ---------------------------------------------------------------------
  static ExWorkProcRetcode sWork(ex_tcb *tcb)
  {
    return ((ExUdrTcb *) tcb)->work();
  }
  
 
  ExWorkProcRetcode workCancel();

  static ExWorkProcRetcode sWorkCancel(ex_tcb *tcb)
  {
    return ((ExUdrTcb *) tcb)->workCancel();
  }
  // ---------------------------------------------------------------------
  // Work method for tble mapping udfs
  // ---------------------------------------------------------------------
  static ExWorkProcRetcode sTmudfWork(ex_tcb *tcb)
  {
    return ((ExUdrTcb *) tcb)->tmudfWork();
  }
  inline void tickleSchedulerWork()
  {
    ioSubtask_->scheduleAndNoteCompletion();
  }

  //
  // Helper functions used for resource management
  //
  void deallocateMessage(UdrMessageObj *m);
  void releaseControlStream();
  void allocateDataStream();
  void releaseDataStream();
  void releaseServerResources();
  void releaseConnectionToServer();

  //
  // Helper function to send control messages
  //
  NABoolean sendControlMessage(UdrIpcObjectType t,
                               NABoolean callbackRequired);

  // ---------------------------------------------------------------------
  // Helper functions called by the work method. See comments in the
  // .cpp file for descriptions
  // ---------------------------------------------------------------------
  ExWorkProcRetcode buildAndSendRequestBuffer();
  ExWorkProcRetcode checkSend();
  ExWorkProcRetcode checkReceive();
  ExWorkProcRetcode continueRequest();
  UdrDataBuffer *getReplyBuffer();
  UdrDataBuffer *getRequestBuffer();
  NABoolean replyBufferIsEmpty();
  void releaseReplyBuffer();
  void releaseRequestBuffer();
  void releaseChildInputBuffer(Int32 i);
  ExWorkProcRetcode returnSingleRow();
  NABoolean anyOutstandingQueueRequests();
  NABoolean verifyUdrServerProcessId();
  NABoolean insertUpQueueEntry(ex_queue::up_status status,
                               ComDiagsArea *diags = NULL);
  NABoolean serverResourcesAreLoaded() const;
  void addIntegrityCheckFailureToDiagsArea(ComDiagsArea *diags) const;
  void tmudfCancelChildRequests(queue_index parentIndex);

  NABoolean validateDataRow(const tupp &replyTupp, ComDiagsArea *&diags);

  // This TCB implements a state machine. Valid state transitions are
  // defined by the code in setUdrTcbState().
  enum UdrTcbState
  {
    BUILD = 1,
    FIXUP,
    SENDING_LOAD,
    WORK,
    WORK_IO_ACTIVE,
    LOAD_FAILED,
    SENDING_UNLOAD,
    IPC_ERROR,
    SCALAR_INPUT_READY_TO_SEND,
    READ_TABLE_INPUT_FROM_CHILD,
    RETURN_ROWS_FROM_CHILD,
    CHILD_INPUT_READY_TO_SEND,
    DONE
  };

 
  NABoolean setUdrTcbState(UdrTcbState target);
  static const char *getUdrTcbStateString(UdrTcbState s);
  void attemptTransitionToWorkState();

  NABoolean dataRequestsAreTransactional() const;

  //
  // Protected data members
  //
  UdrTcbState state_;
  ex_queue_pair qParent_;
  atp_struct *workAtp_;
  sql_buffer_pool *outputPool_;
  sql_buffer_pool *inputPool_;
  UdrDataBuffer *replyBuffer_;
  UdrDataBuffer *requestBuffer_;
  UdrDataBuffer **childInputBuffers_; // array of child table buffers
  queue_index nextToSend_;
  ExUdrServer *udrServer_;
  UdrClientDataStream *dataStream_;
  UdrClientControlStream *outstandingControlStream_;
  UdrHandle udrHandle_;
  ExSubtask *ioSubtask_;
  IpcProcessId serverProcessId_;
  RSHandle rsHandle_;
  ComUInt32 rsIndex_;
  ExRsInfo *rsInfo_;
  const ex_tcb **childTcbs_;   // array of pointers to child task control blocks
  ex_queue_pair *qChild_;      // array of pointers to child queues
  TmudfState *tmudfStates_;    // array of states of these child queues
  ExUDRBaseStats *udrBaseStats_;
  ExUDRStats *udrStats_;

  Int64 dataMsgsSent_;
  Int64 continueMsgsSent_;

#ifdef UDR_DEBUG
  FILE *traceFile_;
  NABoolean doTrace_;
  NABoolean doStateTrace_;
  NABoolean doIpcTrace_;
  NABoolean trustReplies_;
  void initializeDebugVariables();
  void printDataStreamState();
#endif

}; // class ExUdrTcb

//----------------------------------------------------------------------
// class ExUdrPrivateState
//----------------------------------------------------------------------
class ExUdrPrivateState : public ex_tcb_private_state
{
  friend class ExUdrTcb;

public:
  ExUdrPrivateState();
  ~ExUdrPrivateState();

protected:
  void init();

  ExUdrTcb::UdrTcbSendStep step_;
  Int64 matchCount_;
  ComUInt32 numEodsFromChildTcbs_; // for future use 
  ComSInt32 currentChildTcbIndex_; 
};

#endif // __EX_UDR_H

