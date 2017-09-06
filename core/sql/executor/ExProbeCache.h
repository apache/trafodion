// **********************************************************************
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
// **********************************************************************
#ifndef EX_PROBE_CACHE_H
#define EX_PROBE_CACHE_H

//
// Task Definition Block
//
#include "ComTdbProbeCache.h"
#include "QueueIndex.h"
#include "ExSimpleSqlBuffer.h"
#include "ExStats.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExProbeCacheTdb;
class ExProbeCacheTcb;
class ExProbeCachePrivateState;
class ExPCMgr;
class ExPCE;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExProbeCacheTdb
// -----------------------------------------------------------------------
class ExProbeCacheTdb : public ComTdbProbeCache
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExProbeCacheTdb()
  {}

  virtual ~ExProbeCacheTdb()
  {}

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
  //    If yes, put them in the appropriate ComTdb subclass instead.
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
class ExProbeCacheTcb : public ex_tcb
{
  friend class   ExProbeCachePrivateState;

public:
  // Constructor
  ExProbeCacheTcb(const ExProbeCacheTdb & pc_tdb,    
	    const ex_tcb &    child_tcb,    // child queue pair
	    ex_globals *glob
	    );
  
  ~ExProbeCacheTcb();  
  
  void freeResources();  // free resources
  
  virtual void registerSubtasks();  // register work procedures with scheduler
  ExWorkProcRetcode work();         // do not call this.
  ExWorkProcRetcode workUp();
  ExWorkProcRetcode workDown();
  ExWorkProcRetcode workCancel();

  // The static work procs for scheduler.  Note that ex_tcb base class declares
  // one for cancel.
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb) 
        { return ((ExProbeCacheTcb *) tcb)->workUp(); }
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
        { return ((ExProbeCacheTcb *) tcb)->workDown(); }

  ex_queue_pair getParentQueue() const { return qparent_;}

  virtual Int32 numChildren() const { return 1; }   
  virtual const ex_tcb* getChild(Int32 /*pos*/) const { return childTcb_; }

  virtual NABoolean needStatsEntry();

  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
					    ComTdb *tdb);

  ExProbeCacheStats * getProbeCacheStats() 
    { 
      if (getStatsEntry())
        return getStatsEntry()->castToExProbeCacheStats();
      else
        return NULL;
    }


  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

private:
  /////////////////////////////////////////////////////
  // Private methods.
  /////////////////////////////////////////////////////

  inline ExProbeCacheTdb & probeCacheTdb() const 
      { return (ExProbeCacheTdb &) tdb; }

  inline ex_expr * hashProbeExpr() const 
      { return probeCacheTdb().hashProbeExpr_; };
  
  inline ex_expr * encodeProbeExpr() const 
      { return probeCacheTdb().encodeProbeExpr_; };

  inline ex_expr * moveInnerExpr() const 
      { return probeCacheTdb().moveInnerExpr_; };

  inline ex_expr * selectPred() const 
      { return probeCacheTdb().selectPred_; };

  void  makeReplyToParentUp(ex_queue_entry *pentry_down, 
                           ExProbeCachePrivateState &pstate, 
                           ex_queue::up_status reply_status);
  enum MoveStatus {
    MOVE_OK,
    MOVE_BLOCKED,
    MOVE_ERROR
  };

  MoveStatus moveReplyToCache(ex_queue_entry &reply, ExPCE &pcEntry);

  void cancelInterest(ExPCE *pcEntry);

  /////////////////////////////////////////////////////
  // The various steps of a ExProbeCachePrivateState.
  /////////////////////////////////////////////////////
  enum ProbeCacheStep 
  {
    NOT_STARTED, 
    CACHE_MISS, 
    CACHE_HIT,
    CANCELED_MISS,
    CANCELED_HIT,
    CANCELED_NOT_STARTED,
    DONE_MISS,
    DONE
  };

  /////////////////////////////////////////////////////
  // Private data.
  /////////////////////////////////////////////////////

  const ex_tcb * childTcb_;

  ex_queue_pair  qparent_;
  ex_queue_pair  qchild_;
  queue_index    nextRequest_;  // idx of next down queue entry to be processed
  
  atp_struct     * workAtp_;
  
  tupp_descriptor  probeHashTupp_;
  ULng32 probeHashVal_;
  
  tupp_descriptor probeEncodeTupp_;
  char * probeBytes_;

  ExSimpleSQLBuffer *pool_;

  ExPCMgr * pcm_;

  ExSubtask *workUpTask_;

};

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
class ExProbeCachePrivateState : public ex_tcb_private_state
{
  friend class ExProbeCacheTcb;

  ExProbeCacheTcb::ProbeCacheStep step_;

  ExPCE *pcEntry_;

  Int64 matchCount_; // number of rows returned for this parent row

public:
  ExProbeCachePrivateState();        //constructor
  ~ExProbeCachePrivateState();       // destructor
  void init();
};


///////////////////////////////////////////////////////////////////
// The Probe Cache Entry
///////////////////////////////////////////////////////////////////
class ExPCE
{
  friend class ExPCMgr;
  friend class ExProbeCacheTcb;

  // No constructor.  The ExPCMgr's memset will clear the flags_.everUsed_
  // bit, and this is read by the ExPCMgr::addEntry method, which will 
  // initializes a newly added entry.

  // No destructor needed.

  void release() { 
    ex_assert(refCnt_ != 0, "Probe Cache entry ref count already zero");
    refCnt_--;
  }
  
  ExPCE *nextHashVal_;          // Collision chain for probes with same 
                                // hash value.  
  
  union {
    Lng32 value_;
    struct
    {
      unsigned char useBit_:1;          // for second chance replacement.
      unsigned char canceledPending_:1; // cancel has been propagated.
      unsigned char everUsed_:1;        // to indicate an un-init'd ExPCE.
      unsigned char bitFiller_:5;
      unsigned char byteFiller_[3];
    } flags_;
  };
  
  ULng32 probeHashVal_;    // hash value of probe data.

  ULng32 refCnt_;          // # down queue entries interested in me.
  
  ex_queue::up_status upstateStatus_; // from CACHE_MISS's original reply.
   
  queue_index probeQueueIndex_;   // from the CACHE_MISS's parentQueueIndex

  ComDiagsArea *diagsArea_;   // from CACHE_MISS's original reply.
 
  tupp innerRowTupp_;              // reply tuple for the probe.

  char probeData_[1];             // variable length encoded data.
};

///////////////////////////////////////////////////////////////////
// The Probe Cache Manager
///////////////////////////////////////////////////////////////////
class ExPCMgr : public NABasicObject
{
public:
  ExPCMgr(Space *space, ULng32 numEntries, ULng32 probeLength,
          ExProbeCacheTcb *tcb);

  ~ExPCMgr();
  
  enum AddedOrFound { 
    FOUND,
    ADDED
  };

  AddedOrFound addOrFindEntry( ULng32 probeHashVal, 
                               char * probeBytes, 
                               queue_index nextRequest, 
                               ExPCE * &pcEntry );

private:

            // Reuse unused entry, or use an entry that has never been used.
    ExPCE *addEntry(Int32 bucket, 
                    ULng32 probeHashVal, 
                    char * probeBytes, 
                    queue_index qIdxForCancel);

            // Choose a possible victim for addEntry's second-chance 
            // cache replacement logic.
    ExPCE *getPossibleVictim();

            // Set to max number of probes in cache.  So loading factor is 1.0.
    ULng32 numBuckets_;

            // How many bytes in the probe data?
    ULng32 probeLen_;

            // This points to an array of collision chain headers.
    ExPCE **buckets_;

            // The collision chain entries allocated as an array.  See ExPCMgr
            // for chain semantics.  Note that we declare this as a char *,
            // because the size of the entries in the array are not known
            // when this C++ code is compiled -- see ExPCE::probeData_.
            // Instead, we do our own pointer arithmetic when we must access
            // entries_ as an array, see getPossibleVictim().
    char *entries_;

            // The size of each of entries_'s  array elements.  We need to 
            // keep track of this in our code in order to do correct pointer
            // arithmetic.
    ULng32 sizeofExPCE_;

            // For implementing the "second chance" replacement algorithm.
    ULng32 nextVictim_;

            // Remember our heap and use it in the dtor.
    Space *space_;

            // To access the runtime stats.
    ExProbeCacheTcb *tcb_;

};

#endif  // EX_PROBE_CACHE_H
