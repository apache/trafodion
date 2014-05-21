/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef EX_SEQUENCE_GENERATOR_H
#define EX_SEQUENCE_GENERATOR_H


/* -*-C++-*-
******************************************************************************
*
* File:         ExSequenceGenerator.h
* Description:  Class declarations for ExSequenceGeneratorTcb
*               
*               
* Created:      03/04/2008
* Language:     C++
*
*
*
*
******************************************************************************
*/

//
// Task Definition Block for the Sequence Generator:
//

#include "ComTdbSequenceGenerator.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExSequenceGeneratorTdb;
class ExNextValueForTdb;
class ExNextValueForPrivateState;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExSequenceGeneratorTdb
// -----------------------------------------------------------------------
class ExSequenceGeneratorTdb : public ComTdbSequenceGenerator
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  NA_EIDPROC ExSequenceGeneratorTdb()
  {}

  NA_EIDPROC virtual ~ExSequenceGeneratorTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbSequenceGenerator instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


//
// Task control block from the Sequence Generator
//

class ExSequenceGeneratorTcb : public ex_tcb
{
public:

  // Constructor called during build phase (ExSequenceGeneratorTdb::build()).
  ExSequenceGeneratorTcb(const ExSequenceGeneratorTdb & sequenceGeneratorTdb,
                         const ex_tcb &childTdb,    
                         ex_globals *glob);
        
  // Default destructor
  ~ExSequenceGeneratorTcb();  

  // Free resources
  void  freeResources();

  // Transaction support

  short beginTransaction();
  short commitTransaction();
  short rollbackTransaction();
  void  cleanTransaction();

  // The work procedure for ExSequenceGeneratorTcb.
  //
  short work();

  // The queue pair used to communicate with the parent TCB
  ex_queue_pair  getParentQueue() const;

  // A virtual function used by the GUI.  Will always return 0 for
  // ExSequenceGeneratorTcb
  virtual Int32 numChildren() const;
  virtual const ex_tcb *getChild(Int32 pos) const;

  void addTransactionCount() { transactions_++; };
  Int64 getTransactionCount() { return transactions_; };

private:

  // A reference to the cooresponding TDB (ExSequenceGeneratorTdb)
  inline ExSequenceGeneratorTdb &sequenceGeneratorTdb() const;

  // private state


  // The child TCB of this SequenceGenerator node.
  //
  const ex_tcb *childTcb_;

  // Queues used to communicate with the parent TCB.
  ex_queue_pair  qParent_;

  // The queue pair used to communicate with the child node.
  // This queue is allocated by the child node.  This data
  // member is initialized by calling getParentQueue() on the child TCB.
  //
  ex_queue_pair  childQueue_;

  // Possible states of the work procedure.
  enum sequenceGeneratorWorkState {
    SEQGEN_NEWBLOCK,
    SEQGEN_WAITFORBLOCK,
    SEQGEN_DONE,
    SEQGEN_ERROR_IN_SG,
    SEQGEN_ERROR_FROM_CHILD,
    SEQGEN_CLEANUP_AND_RETRY,
    SEQGEN_SEND_Q_NO_DATA
    };

  // The current state of the work procedure.
  sequenceGeneratorWorkState workState_;

  // The size of the virtual block of values.
  Int64 blockSize_;

  // tupp to hold the blockSize_
  tupp blockSizeTupp_;

  // Record number of transactions should
  // we want to report statistics
  Int64 transactions_;

  // Current transaction
  ExTransaction * currTransaction_;
  Lng32 xnReturnCode_;

  // Retry for update table timeout
  Lng32 retryTimeout_;
  Lng32 retryTimeoutMax_;

};

//  Inline procedures

inline ExSequenceGeneratorTdb &
ExSequenceGeneratorTcb::sequenceGeneratorTdb() const
{
  return (ExSequenceGeneratorTdb &) tdb;
};


// -----------------------------------------------------------------------
// ExNextValueForTdb
// -----------------------------------------------------------------------
class ExNextValueForTdb : public ComTdbNextValueFor
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  NA_EIDPROC ExNextValueForTdb()
  {}

  NA_EIDPROC virtual ~ExNextValueForTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual ex_tcb *build(ex_globals *globals);

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
  // if you reach here with a intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the ComTdbSequenceGenerator instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//
// Task control block from the NextValueFor
//

class ExNextValueForTcb : public ex_tcb
{

friend class ExNextValueForPrivateState;

public:

  // Constructor called during build phase (ExNextValueForTdb::build()).
  ExNextValueForTcb(const ExNextValueForTdb &nextValueForTdb,
                         const ex_tcb *leftChildTdb,    
                         const ex_tcb *rightChildTdb,    
                         ex_globals *glob);
        
  // Default destructor
  ~ExNextValueForTcb();  

  // Free resources
  void  freeResources();

  // The work procedure for ExNextValueForTcb.
  //
  short work();
  // The queue pair used to communicate with the parent TCB
  ex_queue_pair  getParentQueue() const;

  // A virtual function used by the GUI.  Will always return 0 for
  // ExNextValueForTcb
  virtual Int32 numChildren() const;
  virtual const ex_tcb *getChild(Int32 pos) const;

  NA_EIDPROC virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

private:

  // A reference to the cooresponding TDB (ExNextValueForTdb)
  inline ExNextValueForTdb &nextValueForTdb() const;

  void workRefreshFromSGResult(ExNextValueForPrivateState &pstate);
  short workSendResultsToParent(ExNextValueForPrivateState &pstate);
  short processDiagsArea(atp_struct* atp,
			 ExNextValueForPrivateState &pstate);
  short processCancel(ExNextValueForPrivateState &pstate);

  // The child TCB of this NextValueFor node.
  //
  const ex_tcb *leftChildTcb_;
  const ex_tcb *rightChildTcb_;


  // Queues used to communicate with the parent TCB.
  ex_queue_pair  qParent_;

  // The queue pair used to communicate with the child node.
  // This queue is allocated by the child node.  This data
  // member is initialized by calling getParentQueue() on the child TCB.
  //
  ex_queue_pair  qLeft_;
  ex_queue_pair  qRight_;

  // Possible states of the work procedure.
  enum NextValueForStep {
    NVF_START,
    NVF_SENT_REQUEST_TO_SG,
    NVF_GET_RESULT_FROM_SG,
    NVF_SG_REFRESH_DONE,
    NVF_SEND_RESULT_ROWS_UP,
    NVF_DONE,
    NVF_ERROR,
    NVF_CANCEL
    };

 // Possible SG data types
  enum NextValueForSGType {
    NVF_SG_DT_UNSIGNED_SMALLINT = 8,
    NVF_SG_DT_UNSIGNED_INTEGER = 10,
    NVF_SG_DT_LARGEINT = 11
    };

  atp_struct *workAtp_;
  
  // For computing nextValue
  // next N values.
  enum {
    NVF_CURRENT_VALUE = 0,
    NVF_NUM_VALUES = 1
  };

  Int64 valuesFromUpdate_[2];
  tupp_descriptor valuesFromUpdateTupp_;

  // counter so that we don't exceed N_.
  // When we rech N_, we need to sent a request to
  // the sequence generator to get the N_ more values
  ULng32 counterN_;

  ULng32 numInstances_;
  ULng32 instanceNum_;
  unsigned short pid_;

  // Flag to say if we need to request another N_ values
  NABoolean needNextN_;

  // Left and right child queue maintenance
  NABoolean leftDone_;
  NABoolean rightDone_;

  // Maximum value for SG indicator
  NABoolean maxValueWillExceed_;
};

//  Inline procedures

inline ExNextValueForTdb &
ExNextValueForTcb::nextValueForTdb() const
{
  return (ExNextValueForTdb &) tdb;
};



class ExNextValueForPrivateState : public ex_tcb_private_state
{
  friend class ExNextValueForTcb;

  ExNextValueForTcb::NextValueForStep step_;

  Int64 matchCount_; // number of rows returned for this parent row

public:
  ExNextValueForPrivateState();        //constructor
  ~ExNextValueForPrivateState();       // destructor
  void init();
};


#endif


