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
/*
 * ExFastTransportIO.h
 *
 *  Created on: Nov 05, 2012
 */


#ifndef __EX_FAST_TRANSPORT_H
#define __EX_FAST_TRANSPORT_H

#include "ComTdbFastTransport.h"
#include "ex_tcb.h"
#include "ComSmallDefs.h"
#include "ExStats.h"
#include "HdfsClient_JNI.h"
#include "ExpLOBinterface.h"
#include "ex_exe_stmt_globals.h"
// -----------------------------------------------------------------------
// Forward class declarations
// -----------------------------------------------------------------------
class sql_buffer;
class ExExeStmtGlobals;
class SequenceFileWriter;
class HdfsClient;

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------

class ExFastExtractTdb;
class ExFastExtractTcb;

class IOBuffer
{

public:

  friend class ExFastExtractTcb;
  enum BufferStatus {PARTIAL = 0, FULL, EMPTY, ERR};

  IOBuffer(char *buffer, Int32 bufSize)
    : bytesLeft_(bufSize),
      bufSize_(bufSize),
      status_(EMPTY)
  {
    data_ = buffer;
    //memset(data_, '\0', bufSize);

  }

  ~IOBuffer()
  {
  }

  void setStatus(BufferStatus val)
  {
    status_ = val;
  }
  BufferStatus getStatus()
  {
    return status_ ;
  }

  char* data_;
  Int32 bytesLeft_;
  Int32 bufSize_;
  BufferStatus status_;

};
//----------------------------------------------------------------------
// Task control block

// -----------------------------------------------------------------------
// ExFastExtractTdb
// -----------------------------------------------------------------------
class ExFastExtractTdb : public ComTdbFastExtract
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExFastExtractTdb()
  {
  }

  virtual ~ExFastExtractTdb()
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

  inline ULng32 getFlags() const
  {
    return flags_;
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
  inline ex_expr *getChildDataExpr() const
  {
    return childDataExpr_;
  }

  // Defaults for the output buffer pool
  inline ULng32 getNumOutputBuffers() const
  {
    return numBuffers_; // this field comes from the superclass
  }
  inline ULng32 getOutputSqlBufferSize() const
  {
    return bufferSize_; // this field comes from the superclass
  }

  inline ULng32 getNumInputBuffers() const
   {
     return 1;
   }
  inline ULng32 getInputSqlBufferSize() const
  {
    return bufferSize_; // this field comes from the superclass
  }                        // keep it the same as output bufefr size for now


  // Size of a single request/reply/output row
  inline ULng32 getRequestRowLen() const
  {
    return requestRowLen_;
  }
  inline ULng32 getOutputRowLen() const
  {
    return outputRowLen_;
  }

  // Number of tupps in the output row
  inline unsigned short getNumOutputTupps() const
  {
    return criDescUp_->noTuples();
  }

  inline ExpTupleDesc *getChildTuple() const
  {

    return workCriDesc_->getTupleDescriptor(childDataTuppIndex_);
  }
  inline ExpTupleDesc *getChildTuple2() const
  {
    return workCriDesc_->getTupleDescriptor(cnvChildDataTuppIndex_);
  }

  inline Attributes *getChildTableAttr( UInt32 colInd) const
  {

    return workCriDesc_->getTupleDescriptor(childDataTuppIndex_)->getAttr(colInd);
  }
  inline Attributes *getChildTableAttr2( UInt32 colInd) const
  {
    return workCriDesc_->getTupleDescriptor(cnvChildDataTuppIndex_)->getAttr(colInd);
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

class ExFastExtractTcb : public ex_tcb
{
  typedef ex_tcb super;
  friend class ExFastExtractTdb;


private:


public:
  enum FastExtractStates
  {
    EXTRACT_NOT_STARTED = 0,
    EXTRACT_CHECK_MOD_TS,
    EXTRACT_INITIALIZE,
    EXTRACT_PASS_REQUEST_TO_CHILD,
    EXTRACT_RETURN_ROWS_FROM_CHILD,
    EXTRACT_DATA_READY_TO_SEND,
    EXTRACT_SYNC_WITH_IO_THREAD,
    EXTRACT_ERROR,
    EXTRACT_DONE,
    EXTRACT_CANCELED
  };
  enum IOSyncState {PENDING_NONE = 0,
	  	  	  	  	 PENDING_FILEOPEN,
	  	  	  	  	 PENDING_FREEBUFFER,
	  	  	  	  	 PENDING_NOTFULL_QUEUE,
  	  	  	  	  	 PENDING_THREAD_EXIT};



  static const Int32 BUFSZ = 1024 * 1024;

  ExFastExtractTcb(const ExFastExtractTdb &tdb,
           const ex_tcb & childTcb,
           ex_globals *glob);
  ~ExFastExtractTcb();

  virtual void freeResources();

  // ---------------------------------------------------------------------
  // Standard TCB methods
  // ---------------------------------------------------------------------

  virtual ExWorkProcRetcode work();

  void registerSubtasks();

  ex_queue_pair getParentQueue() const
  {
    return qParent_;
  }

  Int32 numChildren() const
  {
    return myTdb().numChildren();
  }

  virtual const ex_tcb *getChild(Int32 pos) const
  {
    ex_assert((pos >= 0) , "");
    if (pos ==0 )
       return childTcb_;
    else
      return NULL;
  }

  ex_tcb_private_state *allocatePstates(
  Lng32 &numElems,      // [IN/OUT] desired/actual elements
  Lng32 &pstateLength); // [OUT] length of one element

  void updateWorkATPDiagsArea(ex_queue_entry * centry);
  void updateWorkATPDiagsArea(ComDiagsArea *da);
  void updateWorkATPDiagsArea(ExeErrorCode rc, const char* msg = NULL);
  void updateWorkATPDiagsArea(const char *file, int line, const char *msg);


  virtual NABoolean needStatsEntry();

  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                            ComTdb *tdb);
  ExFastExtractStats * getFastExtractStats()
    {
      if (getStatsEntry())
        return getStatsEntry()->castToExFastExtractStats();
      else
        return NULL;
    }

  ExHdfsScanStats * getHdfsScanStats()
    {
      if (getStatsEntry())
        return getStatsEntry()->castToExHdfsScanStats();
      else
        return NULL;
    }



protected:

  inline const ExFastExtractTdb &myTdb() const { return (const ExFastExtractTdb &) tdb; }
  ExExeStmtGlobals *myExeStmtGlobals() const;

  virtual void insertUpQueueEntry(ex_queue::up_status status,
                          ComDiagsArea *diags,
                          NABoolean popDownQueue);

  static const char *getTcbStateString(FastExtractStates s);

  //
  // Protected data members
  //
  ex_queue_pair      qParent_;
  atp_struct         *workAtp_;
  sql_buffer_pool    *outputPool_;
  sql_buffer_pool    *inputPool_;
  IOBuffer           *bufferPool_[10];
  Int32              numBuffers_;
  IOBuffer           *currBuffer_;
  const ex_tcb       *childTcb_;
  ex_queue_pair      qChild_;
  int                *sourceFieldsConvIndex_;
  SqlBuffer          *inSqlBuffer_;
  UInt32             maxExtractRowLength_;
  tupp_descriptor    *childOutputTD_;
  NABoolean          endOfData_;
  CollHeap           *heap_;
  ExFastExtractStats *feStats_;
  ExHdfsScanStats    *hdfsStats_;

  time_t              tstart_;

  UInt32             bufferAllocFailuresCount_;

  // modification timestamp of root dir location.
  Int64              modTS_;
}; // class ExFastExtractTcb
/////////////////////////////////////////////////////

class ExHdfsFastExtractTcb : public ExFastExtractTcb
{
  typedef ex_tcb super;

  void convertSQRowToString(ULng32 nullLen,
                            ULng32 recSepLen,
                            ULng32 delimLen,
                            tupp_descriptor* dataDesc,
                            char* targetData,
                            NABoolean & convError);

  friend class ExFastExtractTdb;

public:
  ExHdfsFastExtractTcb(const ExFastExtractTdb &tdb,
           const ex_tcb & childTcb,
           ex_globals *glob);
  ~ExHdfsFastExtractTcb();

  // ---------------------------------------------------------------------
  // Standard TCB methods
  // ---------------------------------------------------------------------

  virtual Int32 fixup();
  virtual ExWorkProcRetcode work();

protected:

  virtual void insertUpQueueEntry(ex_queue::up_status status,
                          ComDiagsArea *diags,
                          NABoolean popDownQueue);
                          
  NABoolean isSequenceFile();
  void createSequenceFileError(Int32 sfwRetCode);
  void createHdfsClientFileError(Int32 hdfsClientRetCode);
  NABoolean isHdfsCompressed();
  NABoolean getEmptyNullString()
  {
    if (myTdb().getNullString() == NULL)
      return TRUE;
    return strlen(myTdb().getNullString()) == 0;
  }


  char hdfsHost_[500];
  int  hdfsPort_;
  char fileName_[1000];
  char targetLocation_[1000];
  NABoolean errorOccurred_;
  SequenceFileWriter* sequenceFileWriter_;
  HdfsClient *hdfsClient_;
}; // class ExHdfsFastExtractTcb

//----------------------------------------------------------------------
// class ExFastExtractPrivateState
//----------------------------------------------------------------------
class ExFastExtractPrivateState : public ex_tcb_private_state
{
  friend class ExFastExtractTcb;
  friend class ExHdfsFastExtractTcb;
  void init()
  {
    step_ = ExFastExtractTcb::EXTRACT_NOT_STARTED;
    matchCount_ = 0;
    successRowCount_ = 0;
    errorRowCount_ = 0;
    processingStarted_ = FALSE;
  }


public:
  ExFastExtractPrivateState();
  ~ExFastExtractPrivateState();

protected:


  ExFastExtractTcb::FastExtractStates step_;
  NABoolean processingStarted_;
  Int64 matchCount_;
  Int64 successRowCount_;
  Int64 errorRowCount_;

};


#endif // __EX_FAST_TRANSPORT_H
