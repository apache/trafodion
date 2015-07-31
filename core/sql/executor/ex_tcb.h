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
#ifndef EX_TCB_H
#define EX_TCB_H

/* -*-C++-*-
******************************************************************************
*
* File:         ex_tcb.h
* Description:  Class declaration for ex_tcb (Task Control Block)
*               
*               
* Created:      5/3/94
* Language:     C++
*
*
*
*
******************************************************************************
*/
#include <sys/time.h>
#include "Platform.h"

//
//      TCB     Task control block
//

// Classes defined in this file

class   ex_tcb;         // Superclass of all task control blocks
class   ExStatisticsArea;
class   ExOperStats;

// forward
class sql_buffer_pool;
class ex_queue_pair;

// -------------------------------------------------------------------------
// TCBs

class ex_tcb : public ExGod
{
public:

  NA_EIDPROC ex_tcb(const ComTdb & tdb,
                    const short in_version,
                    ex_globals * g);    // constructor
  
  NA_EIDPROC virtual ~ex_tcb(); // destroy
  
  NA_EIDPROC virtual void freeResources() = 0;  // free resources

  // register subtasks with scheduler
  NA_EIDPROC virtual void registerSubtasks();

  NA_EIDPROC virtual ex_queue_pair getParentQueue() const = 0;
  
  NA_EIDPROC inline short getVersion() const { return version_; }

  // access child TCBs via virtual methods
  NA_EIDPROC virtual const ex_tcb* getChild(Int32 pos) const = 0;
  NA_EIDPROC virtual Int32 numChildren() const = 0;

  NA_EIDPROC virtual Int32 fixup();

  // return of TRUE means error, FALSE means all Ok.
  NA_EIDPROC virtual NABoolean reOpenTables();
  NA_EIDPROC virtual NABoolean closeTables();

  // this method is called to rollback to a previously set savepoint.
  NA_EIDPROC virtual Int32 rollbackSavepoint();

  // Three officially supported work methods: work, cancel, and resize.
  // The default implementations make use of these work methods. Individual
  // TCBs may have other or additional work methods, but such TCBs may need
  // to redefine some more virtual functions, such as registerSubtasks.
  // The work() method is a pure virtual method, the other two have
  // default implementations that may be sufficient for most TCBs.
  NA_EIDPROC virtual ExWorkProcRetcode work() = 0;
  // default static work proc for scheduler
// warning elimination (removed "inline")
  NA_EIDPROC static ExWorkProcRetcode sWork(ex_tcb *tcb)
                                                   { return tcb->work(); }
  NA_EIDPROC virtual ExWorkProcRetcode workCancel();
  // default static work proc for scheduler

  NA_EIDPROC static ExWorkProcRetcode sCancel(ex_tcb *tcb)
                                             { return tcb->workCancel(); }
  // methods to resize the parent up/down queues
  NA_EIDPROC virtual ExWorkProcRetcode workResize();
  // the static version
  NA_EIDPROC static  ExWorkProcRetcode sResize(ex_tcb *tcb)
                                             { return tcb->workResize(); }
  
  NA_EIDPROC virtual void display() const {};
  
  NA_EIDPROC inline const ComTdb * getTdb() const         { return &tdb; }
  NA_EIDPROC inline ex_globals * getGlobals() const { return globals_; }

  NA_EIDPROC unsigned short getExpressionMode() const;
  NA_EIDPROC inline Space * getSpace() {return globals_->getSpace();}
  NA_EIDPROC inline CollHeap * getHeap() {return globals_->getDefaultHeap();}

  // helper methods to allocate parent queues and register queue resize tasks
  NA_EIDPROC void allocateParentQueues(ex_queue_pair &parentQueues,
				       NABoolean allocatePstate = TRUE);
  NA_EIDPROC void registerResizeSubtasks();
  
  NA_EIDPROC virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  NA_EIDPROC void setStatsEntry(ExOperStats * statsEntry) {
    statsEntry_ = statsEntry; }

  // return stats entry, if stats are enabled.
  NA_EIDPROC ExOperStats * getStatsEntry()
  { 
    return (getGlobals()->statsEnabled() ? statsEntry_ : NULL); 
  }

  // this method find the first set of children in the child tree
  // that have a valid stats area and sets their parent id to the
  // input tdb id.
  NA_EIDPROC void propagateTdbIdForStats(Lng32 tdbId);

  NA_EIDPROC void allocateStatsEntry(CollHeap * heap = NULL);

  NA_EIDPROC virtual ExOperStats * doAllocateStatsEntry(CollHeap *heap,
                                                        ComTdb *tdb);

  NA_EIDPROC sql_buffer_pool * getPool() { return pool_; }

  NA_EIDPROC virtual NABoolean resizePoolInfo() { return FALSE; }

  NA_EIDPROC virtual void computeNeededPoolInfo(
       Int32 &numBuffs, 
       UInt32 &bufferSize, 
       UInt32 &poolSize);

  NA_EIDPROC Lng32 getTotalPoolSize()
  { return pool_->getTotalMemorySize();};

  NA_EIDPROC virtual void cleanup();
  // ****  information for GUI  *** -------------

  Int32 objectId;

#ifdef NA_DEBUG_GUI
  static Int32 objectCount;

  void increaseObjectcount() { objectCount++; }
  Int32 getObjectcount() const { return objectCount; }
  void setObjectId() { objectId = objectCount; }
  Int32 getObjectId() const { return objectId; }
#endif

  NA_EIDPROC ComTdb::ex_node_type getNodeType() const { return nodeType_; }

  // QSTUFF
  NA_EIDPROC NABoolean isHoldable() const { return holdable_; }
  // setHoldable is redefined by the partition access tcb
  NA_EIDPROC virtual void      setHoldable(NABoolean h) { holdable_ = h;}
  NA_EIDPROC void      propagateHoldable(NABoolean h);
  // QSTUFF
  
  NA_EIDPROC virtual NABoolean needStatsEntry();
#ifndef __EID
  NA_EIDPROC void mergeStats(ExStatisticsArea *otherStats);
#endif

  NA_EIDPROC virtual void cpuLimitExceeded();
  NA_EIDPROC inline char * getEyeCatcher()
                      { return eyeCatcher_.name_for_sun_compiler; }

  virtual short moveRowToUpQueue(
                                 ex_queue_pair *qparent,
                                 UInt16 tuppIndex,
                                 const char * row, Lng32 len = -1,
                                 short * rc = NULL, NABoolean isVarchar = TRUE);

  short handleError(ex_queue_pair *qparent, ComDiagsArea *inDiagsArea);
  short handleDone(ex_queue_pair *qparent, ComDiagsArea *inDiagsArea);

private:

  ex_eye_catcher eyeCatcher_;           // eye catcher
  ComTdb::ex_node_type  nodeType_;      // TCB type
  const short version_;                 // version

  // remember the executor globals.
  ex_globals * globals_;

  ExOperStats *statsEntry_;

protected:
  const ComTdb & tdb;                   // reference to TDB

  sql_buffer_pool *pool_;

  // QSTUFF
protected:
  // if set to TRUE indicates a holdable cursor. This flag is set at 
  // fixup and stored in the plt entry in the module file
  NABoolean holdable_;
  // QSTUFF

};

// -----------------------------------------------------------------------
// a template to create the PSTATEs of a down queue more easily
// -----------------------------------------------------------------------

// NOTE:
// this is assuming that one can use the default constructor and
// assignment operator of a pstate and that the assignment operator
// doesn't assume the target of the assignment to be initialized

template <class T> class PstateAllocator
{
public:
  
#pragma nowarn(1103)   // warning elimination 
  NA_EIDPROC ex_tcb_private_state *allocatePstates(
#pragma warn(1103)  // warning elimination 
       ex_tcb *tcb, 
       Lng32   &numElems,      // inout, desired/actual elements
       Lng32   &elementLength)
  {
    T *result;
    
    elementLength = sizeof(T);

    // use the array form of operator new to allocate the pstates,
    // assuming that all pstates have a default constructor
    // Note that calling Vector new requires a local implementation of
    // __vec_new() in the executor, because it is part of the C++
    // Runtime library.

//SQ_LINUX #if defined(NA_HSC)
//  Arguments do not match with any function 'new []'.
    result = new(tcb->getSpace()/*,FALSE*/) T[numElems];

    // handle the case where we don't have enough memory
    if (result == NULL)
      {
	if (numElems > 1)
	  {
	    // try again with one quarter of the elements
	    numElems = numElems / 4;
	    return allocatePstates(tcb, numElems, elementLength);
	  }

	// too bad, can't even allocate a single pstate
	numElems = 0;
      }
    
    return result;
  }
};

#endif
