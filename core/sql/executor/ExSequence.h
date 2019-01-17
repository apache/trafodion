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
#ifndef ExSequence_h
#define ExSequence_h

/* -*-C++-*-
******************************************************************************
*
* File:         ExSequence.h
* Description:  Class declarations for ExSequence. ExSequence performs
*               sequence functions, like running sums, averages, etc.
* Created:      
* Language:     C++
*
*
*
*
******************************************************************************
*/

// External forward declarations
//
class ExSimpleSQLBuffer;

// Task Definition Block
//
#include "ComTdbSequence.h"

#include "NAMemory.h"
#include "NABasicObject.h"
#include "ExpError.h"
#include "cluster.h" 

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExSequenceTdb;
class ExSequenceTcb;
class ExSequencePrivateState;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;
class ex_tdb;
//

// -----------------------------------------------------------------------
// ExSequenceTdb
// -----------------------------------------------------------------------
class ExSequenceTdb : public ComTdbSequence
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExSequenceTdb()
  {};

  virtual ~ExSequenceTdb()
  {};

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbSequence instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//
// Task control block
//
class ExSequenceTcb : public ex_tcb
{
  friend class   ExSequenceTdb;
  friend class   ExSequencePrivateState;
  
public:
  enum RequestState {
    ExSeq_EMPTY,
    ExSeq_WORKING_READ,
    ExSeq_WORKING_RETURN,
    ExSeq_CANCELLED,
    ExSeq_ERROR,
    ExSeq_DONE,
    ExSeq_OVERFLOW_WRITE,
    ExSeq_OVERFLOW_READ,
    ExSeq_END_OF_PARTITION
  };
  
  ExSequenceTcb(const ExSequenceTdb & seq_tdb,    
		const ex_tcb &    child_tcb,
		ex_globals * glob);
  ~ExSequenceTcb();  

  virtual void registerSubtasks();
  void freeResources();
  short work();
  
  inline ExSequenceTdb & myTdb() const;

  ex_queue_pair getParentQueue() const
  {
  return (qparent_);
  }

  inline ex_expr * sequenceExpr() const;
  inline ex_expr * returnExpr() const;
  inline ex_expr * postPred() const;
  inline ex_expr * cancelExpr() const;
  inline ex_expr * checkPartitionChangeExpr() const;

  inline ex_expr * moveExpr() const;

  friend char *GetHistoryRow(void *data, Int32 n,NABoolean leading,Lng32 winSize, Int32&);
  friend char *GetHistoryRowOLAP(void *data, Int32 n,NABoolean leading,Lng32 winSize, Int32&);
  friend char *GetHistoryRowFollowingOLAP(void *data, Int32 n,NABoolean leading,Lng32 winSize, Int32&);

  //inline char * GetFirstHistoryRowPtr();
//
  NABoolean advanceHistoryRow(NABoolean checkMemoryPressure = FALSE);
  //void unAdvanceHistoryRow();
  inline NABoolean isHistoryFull() const;
  inline NABoolean isHistoryEmpty() const;//if the history buffer is empty (i.e. histRowsToReturn_ == 0)
  inline NABoolean canReturnRows() const;
  inline Lng32 numFollowingRows() const;//number of rows following the current row
  void advanceReturnHistoryRow();
  //inline char * getCurrentRetHistRowPtr() 
  inline void updateHistRowsToReturn() ;
  void initializeHistory();
  void createCluster();

  NABoolean removeOLAPBuffer();
  NABoolean shrinkOLAPBufferList();

  inline NABoolean isOverflowStarted();
  inline NABoolean isUnboundedFollowing();

  inline NABoolean canAllocateOLAPBuffer();
  NABoolean addNewOLAPBuffer(NABoolean checkMemoryPressure = TRUE);

  void updateDiagsArea(ex_queue_entry * centry);
  void updateDiagsArea( ExeErrorCode rc_);
  void updateDiagsArea(ComDiagsArea *da);

  NABoolean getPartitionEnd() const
  {
    return partitionEnd_;
  }
  void setPartitionEnd(NABoolean v) 
  {
    partitionEnd_ = v;
  }
  inline Lng32 recLen();
  
  virtual Int32 numChildren() const;
  virtual const ex_tcb* getChild(Int32 pos) const;
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
private:
  const ex_tcb * childTcb_;
  
  ex_queue_pair  qparent_;
  ex_queue_pair  qchild_;  

  atp_struct * workAtp_;
  ExSubtask  * ioEventHandler_;
  HashBuffer * firstOLAPBuffer_;
  HashBuffer * lastOLAPBuffer_;

  HashBuffer * currentOLAPBuffer_ ;
  HashBuffer * currentRetOLAPBuffer_;

  Lng32 currentHistRowInOLAPBuffer_;
  Lng32 currentRetHistRowInOLAPBuffer_;

  char * currentHistRowPtr_ ;
  char * currentRetHistRowPtr_;
  char *lastRow_;

  Lng32 minFollowing_;
  Lng32 numberHistoryRows_ ;
  Lng32 maxNumberHistoryRows_;
  Lng32 histRowsToReturn_ ;
  
  NABoolean partitionEnd_;
  NABoolean unboundedFollowing_;

  Lng32 allocRowLength_;  // allocated size of a row (original rounded up to 8)
  Lng32 maxRowsInOLAPBuffer_ ;
  Lng32 olapBufferSize_;

  Lng32 maxNumberOfOLAPBuffers_;
  Lng32 numberOfOLAPBuffers_;
  Lng32 minNumberOfOLAPBuffers_;

  ClusterDB * clusterDb_; // used to call ::enoughMemory(), etc.
  Cluster * cluster_; // used for overflow calls: flush(), read(), etc
  NABoolean OLAPBuffersFlushed_;
  NABoolean memoryPressureDetected_;
  HashBuffer *  firstOLAPBufferFromOF_;
  Lng32 numberOfOLAPBuffersFromOF_;
  ExeErrorCode rc_;

  Lng32 numberOfWinOLAPBuffers_;
  Lng32 maxNumberOfRowsReturnedBeforeReadOF_ ;
  Lng32 numberOfRowsReturnedBeforeReadOF_;

  NABoolean overflowEnabled_;

  queue_index    processedInputs_;

  CollHeap * heap_;
}; // class ExSequenceTcb

// ExSequenceTcb inline functions
//
inline ex_expr * ExSequenceTcb::sequenceExpr() const 
{ 
  return myTdb().sequenceExpr_; 
};

inline ex_expr * ExSequenceTcb::returnExpr() const 
{ 
  return myTdb().returnExpr_; 
};

inline ex_expr * ExSequenceTcb::postPred() const 
{ 
  return myTdb().postPred_; 
};

inline ex_expr * ExSequenceTcb::cancelExpr() const 
{ 
  return myTdb().cancelExpr_; 
};

inline ex_expr * ExSequenceTcb::checkPartitionChangeExpr() const 
{ 
  return myTdb().checkPartitionChangeExpr_; 
};

inline NABoolean ExSequenceTcb::isHistoryFull() const
{
  return (histRowsToReturn_ == maxNumberHistoryRows_);
};

inline NABoolean ExSequenceTcb::isHistoryEmpty() const
{
  return (histRowsToReturn_ == 0);
};

inline NABoolean ExSequenceTcb::canReturnRows() const
{
  return (numFollowingRows() >= minFollowing_);

};

inline Lng32 ExSequenceTcb::numFollowingRows() const
{
  return histRowsToReturn_ -1;
};

inline void ExSequenceTcb::updateHistRowsToReturn() 
{
  histRowsToReturn_--;
}

inline NABoolean ExSequenceTcb::isOverflowStarted()
{
  return cluster_ && cluster_->getState() == Cluster::FLUSHED ;
}

inline NABoolean ExSequenceTcb::canAllocateOLAPBuffer()
{
  return (numberOfOLAPBuffers_ < maxNumberOfOLAPBuffers_);

}

inline NABoolean ExSequenceTcb::isUnboundedFollowing()
{
  return  unboundedFollowing_;
}
inline Lng32 ExSequenceTcb::recLen() 
{ 
  //return myTdb().recLen_; 
  return allocRowLength_;
};

inline Int32 ExSequenceTcb::numChildren() const 
{ 
  return 1; 
}   

inline const ex_tcb* ExSequenceTcb::getChild(Int32 pos) const
{
   ex_assert((pos >= 0), ""); 
   if (pos == 0)
      return childTcb_;
   else
      return NULL;
}

inline ExSequenceTdb & ExSequenceTcb::myTdb() const
{
        return (ExSequenceTdb &) tdb;
};

// class ExSequencePrivateState
//
class ExSequencePrivateState : public ex_tcb_private_state
{
  friend class ExSequenceTcb;

public:
  ExSequencePrivateState();
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
  ~ExSequencePrivateState();

private:
  ExSequenceTcb::RequestState step_;
  queue_index index_;  
  Int64 matchCount_;
}; // class ExSequencePrivateState

#endif








