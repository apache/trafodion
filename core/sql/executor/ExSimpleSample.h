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

#if 0
// unused feature, done as part of SQ SQL code cleanup effort

#ifndef EX_SIMPLE_SAMPLE_H
#define EX_SIMPLE_SAMPLE_H

/* -*-C++-*-
******************************************************************************
*
* File:         ExSimpleSample.h
* Description:  Node to do simple sampling. Select one of every n rows.
*
* Created:      1/18/98
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"

#include "ComTdbSimpleSample.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExSimpleSampleTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExSimpleSampleTdb
// -----------------------------------------------------------------------
class ExSimpleSampleTdb : public ComTdbSimpleSample
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExSimpleSampleTdb()
  {}

  virtual ~ExSimpleSampleTdb()
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
  //    If yes, put them in the ComTdbSimpleSample instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


class ExSimpleSampleTcb : public ex_tcb
{
  friend class ExSimpleSampleTdb;

public:

  // Constructor
  // Construct a TCB node from the given TDB.  This constructor is
  // called during the build phase by ExSimpleSampleTdb::build().
  // It:
  //     - allocates the sql buffer pool
  //     - allocates the ATP's for its child's down queue.
  //     - allocates the up and down queues used to communicate
  //       with the parent.
  //     - initializes local state.
  //
  ExSimpleSampleTcb(const ExSimpleSampleTdb& sampleTdb,
                    const ex_tcb& childTdb,
                    ex_globals *glob);
  // Destructor
  ~ExSimpleSampleTcb();

  // Free up any run-time resources.
  void freeResources();

  // Register all the pack subtasks with the scheduler.
  void registerSubtasks();

  // The basic work method for a TCB. SimpleSample does not use this method.
  ExWorkProcRetcode work();

  // Work method to pass requests from parent down to child.
  ExWorkProcRetcode workDown();

  // Work method to recieve results from child, process and pass up to parent.
  ExWorkProcRetcode workUp();

  // Stub to workUp() used by scheduler.
// warning elimination (removed "inline")
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
  {
    return ((ExSimpleSampleTcb *) tcb)->workUp();
  }

  // Stub to workDown() used by scheduler.
// warning elimination (removed "inline")
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
  {
    return ((ExSimpleSampleTcb *) tcb)->workDown();
  } 

  // Stub to processCancel() used by scheduler.
  // static inline ExWorkProcRetcode sCancel(ex_tcb *tcb)
  // {
  //   return ((ExSimpleSampleTcb *) tcb)->processCancel();
  // }

  // Return the parent queue pair. 
// warning elimination (removed "inline")
  ex_queue_pair getParentQueue() const { return qParent_; }

  // Return a reference to the TDB associated with this TCB.
  inline ExSimpleSampleTdb& simpleSampleTdb() const
  {
    return (ExSimpleSampleTdb &) tdb;
  }

  // SimpleSample has one child.
  virtual Int32 numChildren() const { return 1; }

  // Return the child of the pack node by position.
  virtual const ex_tcb* getChild(Int32 pos) const
  {
    if(pos == 0) return childTcb_;
    return NULL;
  }

protected:

  // Method to initialize value of sampleCountDown_ according to method_.
  void initSampleCountDown();

  // The child TCB of this SimpleSample node.
  const ex_tcb* childTcb_;

  // The queue pair used to communicate with the parent node.
  ex_queue_pair qParent_;

  // The queue pair used to communicate with the child node.
  ex_queue_pair qChild_;

  // Next parent down queue entry to process.
  queue_index nextRqst_;

  // Buffer pool used to allocated tupps.
  // ExSimpleSQLBuffer* pool_;

  // Count down to next sample.
  Lng32 sampleCountDown_;

  // Send next request down to children. (called by workDown())
  // void start();

  // Send EOD to parent. (called when child return Q_NO_DATA in queue)
  // void stop();

  // Process cancel requests. (called when parent cancel its request)
  void processCancel();

};


#endif

#endif // if 0
