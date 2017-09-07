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
#ifndef EX_SPLIT_BOTTOM_H
#define EX_SPLIT_BOTTOM_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_split_bottom.h
 * Description:  Split Bottom executor node (the server part)
 *               The split bottom node is the root node of ESP fragments
 * Created:      12/6/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ExCollections.h"
#include "ex_frag_inst.h"

////////////////////////////////////////////////////////////////////////////
// contents of this file
////////////////////////////////////////////////////////////////////////////
class ex_split_bottom_tdb;
class ex_split_bottom_tcb;
class SplitBottomRequestMessage;
class SplitBottomSavedMessage;

////////////////////////////////////////////////////////////////////////////
// forward references
////////////////////////////////////////////////////////////////////////////
class ex_send_bottom_tdb;
class ex_send_bottom_tcb;
class ExEspFragInstanceDir;
class ExEspStmtGlobals;
class StmtStats;
////////////////////////////////////////////////////////////////////////////
// Task Definition Block for split bottom node
////////////////////////////////////////////////////////////////////////////
#include "ComTdbSplitBottom.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_split_bottom_tdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ex_split_bottom_tdb
// -----------------------------------------------------------------------
class ex_split_bottom_tdb : public ComTdbSplitBottom
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ex_split_bottom_tdb()
  {}

  virtual ~ex_split_bottom_tdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

  ex_split_bottom_tcb *buildESPTcbTree(ExExeStmtGlobals * glob,
                                       ExEspFragInstanceDir *espInstanceDir,
                                       const ExFragKey &myKey,
                                       const ExFragKey &parentKey,
                                       ExFragInstanceHandle myHandle,
                                       Lng32 numOfParentInstances);

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
  //    If yes, put them in the ComTdbSplitBottom instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

////////////////////////////////////////////////////////////////////////////
// Task control block for split bottom node
////////////////////////////////////////////////////////////////////////////
class ex_split_bottom_tcb : public ex_tcb
{
  friend class SplitBottomRequestMessage;

public:

  // Constructor
  ex_split_bottom_tcb(const ex_split_bottom_tdb & split_bottom_tdb,
		      const ex_tcb * child_tcb,
		      ExExeStmtGlobals * glob,
		      ExEspFragInstanceDir *espInstanceDir,
		      const ExFragKey &myKey,
		      const ExFragKey &parentKey,
		      ExFragInstanceHandle myHandle,
		      Lng32 numOfParentInstances);
        
  ~ex_split_bottom_tcb();  

  inline const ex_split_bottom_tdb & splitBottomTdb() const
                               { return (const ex_split_bottom_tdb &) tdb; }
  
  void freeResources();  // free resources
  void registerSubtasks();
  virtual NABoolean needStatsEntry();
  virtual ExOperStats * doAllocateStatsEntry(CollHeap *heap, ComTdb *tdb);

  // Stub to processCancel() used by scheduler. 
  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
                  { return ((ex_split_bottom_tcb *) tcb)->processCancel(); }

  // For late cancel.
  ExWorkProcRetcode processInactiveSendTopsBelow(NABoolean &foundOne);


  virtual ex_queue_pair	getParentQueue() const; // there isn't a single one

  ex_queue_pair getSendQueue(CollIndex i) const;
  inline ex_send_bottom_tcb *getSendNode(CollIndex i) const
                                 { return sendNodes_[i - firstParentNum_]; }

  ex_send_bottom_tcb *getConsumerSendBottom() const;

  inline ex_expr * partFunction() const
                                  { return splitBottomTdb().partFunction_; }
  inline NABoolean partFuncUsesNarrow() const
                { return (NABoolean) splitBottomTdb().partFuncUsesNarrow_; }
  inline SplitBottomRequestMessage *getMessageStream()
                                                     { return newMessage_; }
  NABoolean reportErrorToMaster();
  inline NABoolean hasTransaction() const      { return workMessageSaved_; }
  void releaseWorkRequest();

  ExWorkProcRetcode work();
  
  virtual Int32 numChildren() const;
  virtual const ex_tcb* getChild(Int32 pos) const;

  void testAllQueues();

  ExFragInstanceHandle getMyFragInstanceHandle()  const {return myHandle_; }

  // A mutator method to let this operator know which send bottom is connected 
  // to a consumer ESP co-located on this ESP's CPU.  Also, sets the 
  // uniformDistributionType_.
  void setLocalSendBottom(CollIndex s);

  // Enforce query CPU limit.
  virtual void cpuLimitExceeded();

private:

  // ---------------------------------------------------------------------
  // The split bottom node has a variable number of children. Its first
  // child is the actual child in the generated tree (the query that
  // is executed by this process). All other children are send bottom
  // nodes, used to communicate with one or more requesters. Therefore,
  // the other nodes send input queue entries up through their up-queues
  // and get the first child's output entries in their down queues.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // State of the work() method. IDLE means that no active requests are
  // in the down queues. WORK_ON_REQUEST indicates that a request is
  // active in the down queue to the child and that we can proceed
  // sending rows back. WAIT_TO_REPLY means that some requestors are slow
  // and that the up queue to them is full. WAIT_FOR_MORE_REQUESTORS means
  // that we can't proceed until more requestors have sent their request.
  // WAIT_FOR_PARTITION_INPUT_DATA means that we are waiting for more
  // partition input data to arrive.
  // ---------------------------------------------------------------------
  enum SplitBottomWorkState
    {
      IDLE,
      WORK_ON_REQUEST,
      WAIT_TO_REPLY,
      WAIT_FOR_MORE_REQUESTORS,
      WAIT_FOR_PARTITION_INPUT_DATA,
      WAIT_FOR_LATE_CANCELS,
      LIMITS_EXCEEDED_ERROR,
      CLEANUP
    };

  void setWorkState(SplitBottomWorkState);

  const ex_tcb                *tcbChild_;
  ex_queue_pair               qChild_;
  ExEspFragInstanceDir        *espInstanceDir_;
  ExFragInstanceHandle        myHandle_;
  Lng32                        numOfParentInstances_;
  ARRAY(ex_send_bottom_tcb *) sendNodes_;
  ARRAY(ex_queue*)            sendNodesUpQ_;
  ARRAY(ex_queue*)            sendNodesDownQ_;
  atp_struct                  *workAtp_;
  tupp_descriptor             partNumTupp_;       // target of part # expr.
  struct { 
    ULng32             calculatedPartNum_; // target of part # expr.
    char                      pad[4];             // alignment
    Int64                     partHash_;          // Used for skew buster only,
                                                  // this is the intermediate
                                                  // result of the application
                                                  // of the hash function to the 
                                                  // partitioning key by the 
                                                  // partitioning function.
  }  partNumInfo_;
  tupp_descriptor             convErrTupp_;       // tupp for Narrow flag
  Lng32                        conversionErrorFlg_;// side-effected by Narrow
  tupp_descriptor             partInputDataTupp_; // part input data
  SplitBottomWorkState        workState_;
  ExSubtask                   *ioHandler_;        // for part input message
  ExEspStmtGlobals            *glob_;

  // this data member indicates the only or the first requestor that we
  // are working for (valid if state is not IDLE).
  CollIndex                   currRequestor_;

  // This data member points to one send node that has a full up queue.
  // It avoids checking for all nodes repeatedly. We can't proceed until
  // this one node allows us to send more rows. Valid only for state
  // WAIT_TO_REPLY.
  CollIndex                   fullUpQueue_;

  // This data member points to one send node from which we haven't received
  // a request yet. We cannot proceed until we get a request from that
  // one node. Valid only for state WAIT_FOR_MORE_REQUESTORS.
  CollIndex                   emptyDownQueue_;

  // State of affairs re partition input data and work requests. We can
  // only do work while we have a work request from the master executor.
  // The work request may or may not have a transaction attached.
  // If <staticInputData_> is set to FALSE, the node can only work while
  // it has an active input data request. Otherwise, the partition input
  // data is assumed to be static. Work requests are also received
  // through the newMessage_ stream. Long-lasting messages are saved
  // away so the incoming message stream is free for other requests.
  // The boolean flags indicate whether we have saved messages and can
  // work on them.
  NABoolean                   staticInputData_;
  NABoolean                   workMessageSaved_;
  NABoolean                   inputDataMessageSaved_;
  NABoolean                   releaseMessageSaved_;
  NABoolean                   haveTransaction_;
  NABoolean                   calcPartNumIsValid_;
  NABoolean                   broadcastThisRow_;
  SplitBottomRequestMessage   *newMessage_;
  SplitBottomSavedMessage     *savedDataMessage_;
  SplitBottomSavedMessage     *savedWorkMessage_;
  SplitBottomSavedMessage     *savedReleaseMessage_;

  // for canceling.
  Lng32                        numCanceledPartitions_;

  queue_index                  bugCatcher3041_;   // bugzilla 3041.
  
  // Skew buster members for repartitioning skewed data.
  // Only used if we as combining requests.
  
  enum {
    CoLocated_    = 0,
    RoundRobin_   = 1
  }                           uniformDistributionType_ ;

  // Supports CoLocated uniform distribution.
  CollIndex                   localSendBotttom_;

  // When there is no colocated ESP, uniform distrbution of a skewed value
  // will send to consumers using a round robin.
  CollIndex                   roundRobinRecipient_;

  // Hash keys of skewed values have been supplied by the optimizer to 
  // the generator and are stored in the TDB .  When the TCB is constructed,
  // these keys will be organized into a hash table.  Then, when we 
  // repartition results for the consumer ESPs, each partitioning key
  // will be used to probe this hash table to determine if it is possibly 
  // a skewed value.
  //
  // The value 1009 is chosen for the number of collision chain headers 
  // because it is prime, and close to 1000.
  //
  // Linkage is done with array indexes (not memory pointers), to allow
  // the links to be noncontiguous to the hash keys, which avoids 
  // duplicating the potentially large (10K entries) array of hash
  // keys.  So next we define a value indicating no link (analogous to
  // null ptr value, if memory pointers were used).
  
  // There is a bug in MS Visual C++ that raises a compiler error 
  // if we do in-place initialization of static const integral member data
  // (see KB Article ID 241569).  So the workaround is to use an enum:

  enum {
    numSkewHdrs_ = 1009,
    noLink_ = -1
  };
  
  // Hash table header. The values stored are array indexes into 
  // skewInfo_.skewHashValues_ and the skewLinks_.
  Int32                         skewHdrs_[numSkewHdrs_];

  // Collisions for the hash table of skewed keys. 
  Int32                        *skewLinks_;

  // For table N-M way repartition validation
  Lng32                        firstParentNum_; // index of the first ESP

  NABoolean broadcastOneRow_;
  // private methods

  ExWorkProcRetcode workUp();
  ExWorkProcRetcode processCancel(); 
  ExWorkProcRetcode workCancelBeforeSent();
  void replyToMessage(IpcMessageStream *sm);
  void acceptNewPartInputValues(NABoolean moreWork);

  // Table N-M way repartition mapping, valid only for hash2 partitions:
  //
  //                     ------------------------------------
  //  numOfTopParts (m)  |    |    |    |    |    |    |    |
  //                     ------------------------------------
  //  feeds                 |  /  \ /  \  /  /  \  /  \  /
  //                     ------------------------------------
  //  numOfBotParts (n)  |      |      |      |      |      |
  //                     ------------------------------------
  //
  // For EPS i where i in [0, n), the number of target ESPs can be found
  // by this function:

  inline Lng32 numOfTargetESPs(Lng32 numOfTopPs, Lng32 numOfBottomPs,
                              Lng32 myIndex)
  {
    ex_assert(myIndex < numOfBottomPs,
              "Invalid N-M repartition mapping at bottom!");
    return ((myIndex * numOfTopPs + numOfTopPs - 1) / numOfBottomPs
           - (myIndex * numOfTopPs) / numOfBottomPs + 1);
  }

  // To find the index of my first parent ESP:
  inline Lng32 myFirstTargetESP(Lng32 numOfTopPs, Lng32 numOfBottomPs,
                              Lng32 myIndex)
  { return ((myIndex * numOfTopPs) / numOfBottomPs); }

};

// -----------------------------------------------------------------------
// A message stream that is used to receive partition input data and
// to signal back that the node is done with the data.
// -----------------------------------------------------------------------
class SplitBottomRequestMessage : public IpcMessageStream
{
public:

  SplitBottomRequestMessage(IpcEnvironment *env,
			      ex_split_bottom_tcb *splitBottom);

  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnReceive(IpcConnection *connection);

private:

  ex_split_bottom_tcb *splitBottom_;
  
};

// -----------------------------------------------------------------------
// Split bottom nodes hold on to certain requests, such as dynamically
// assigned partition input values and transaction work requests. While
// they hold on to those requests they must still be able to receive
// a transaction release request. In order to do this, an extra message
// stream is allocated to hold the message, which frees up the main
// SplitBottomRequestMessage object for further receive operations.
// SplitBottomSavedMessage objects, like SplitBottomRequestMessage
// objects, never directly have their receive() method called directly,
// they receive their messages from other message streams instead.
// However, SplitBottomSavedMessage::send() is called once the split
// bottom node decides to reply to the saved message. The callbacks
// of this node do nothing, all work is done in 
// SplitBottomRequestMessage:actOnReceive().
// -----------------------------------------------------------------------

class SplitBottomSavedMessage : public IpcMessageStream
{
public:

  SplitBottomSavedMessage(IpcEnvironment *env);

  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnReceive(IpcConnection *connection);

};

#endif /* EX_SPLIT_BOTTOM_H */
