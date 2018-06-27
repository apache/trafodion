///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MLIO_H
#define MLIO_H
#ifndef NAMESERVER_PROCESS

#include <list>
#include <set>
#include <vector>
using namespace std;

#include <signal.h>
#include "lock.h"

#include "localio.h"

typedef map<int, Verifier_t> verifierMap_t;  // (pid, verifier)

class CNoticeMsg;

class SQ_LocalIOToClient
{

friend class CMonitor;
friend void *pendingNoticeThread( void *arg );
friend void *processThread( void *arg );
friend void *serialRequestThread( void *arg );
friend void *tmsyncRequestThread( void *arg );

 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
  SQ_LocalIOToClient( int nid );
  ~SQ_LocalIOToClient();

  struct pidVerifier
  {
      Verifier_t  verifier; // verifier first so map to long is (pid:verifier)
      int         pid;
  };
  union pidver
  {
      struct pidVerifier pv;
      long   pnv;
  };
  typedef union pidver pidVerifier_t;
  typedef set<long> bcastPids_t;

  inline int getAcquiredBufferCount() { return(acquiredBufferCountMax); }
  inline int getAvailableBufferCount() { return(availableBufferCountMin); }
  inline int getMissedBufferCount() { return(missedBufferCount); }
  inline int getSharedBufferCount() { return(sharedBuffersMax); }
  inline int getMaxChildDeathCount() { return deadPidsMax_; }
  int sendCtlMsg( int osPid, MonitorCtlType type, int data = -1, int *error = NULL );
  void putOnNoticeQueue( int osPid
                       , Verifier_t verifier
                       , struct message_def *msg
                       , bcastPids_t *bcastPids);
  void nudgeNotifier ( void );
  int getSizeOfMsg ( struct message_def *myMsg );
  int getSizeOfRequest ( struct message_def *myMsg );
  void releaseMsg( SharedMsgDef *shm, bool monitorOwned );
  void addToVerifierMap(int pid, Verifier_t verifier);
  void delFromVerifierMap( int pid );
  Verifier_t getVerifier( int pid );
  inline int getVerifierMapCount() { return verifierMap_.size(); }
  void releaseMsg( pid_t pid, Verifier_t verifier );
  int  initWorker();
  bool isMonitorHeap( struct message_def *msg );
  inline bool isShutdown() { return(shutdown); }
  void shutdownWork( void );
  void waitForNoticeWork( void );
  void handleAlmostDeadPid( pid_t pid );
  inline int getAlmostDeadPids() { return almostDeadPidsTotal_; }

  void handleDeadPid( pid_t pid );
  void recycleProcessBufs( void );

  bool isWDTEnabled ()
     { return ((SharedMemHdr *) clientBuffers)->wdtEnabler != -17958194; }

  inline void refreshWDT(int refreshCounter) 
     { ((SharedMemHdr *) clientBuffers)->lastMonRefresh = refreshCounter; }

  bool noDeadPids() { return deadPidsHead_ == deadPidsTail_; }

private:

  typedef struct
  {
      struct message_def *msg;
      int                pid;
      Verifier_t         verifier;
      bcastPids_t        *bcastPids;
  } PendingNotice;

  // Add a new shared buffer to the set of managed notices
  void manageNotice ( int msgIndex, CNoticeMsg * noticeMsg );

  // Remove a shared buffer from set of managed notices
  void noticeCompleted ( int msgIndex );

  // Container to hold set of outstanding shared buffers used by the
  // monitor to send info to clients.
  typedef map<int, CNoticeMsg *> noticeMap_t;  // LIO buf index, CNotice*
  noticeMap_t                noticeMap_;
  CLock                      noticeMapLock_;
  
  struct timespec nextNoticeCheck_;

  const char *getTypeStr(int type);
  void processLocalIO( siginfo_t *siginfo);
  int  getStartingShell();
  void processNotices() throw();
  void sendNotice(SharedMsgDef *msg, PendingNotice &pn) throw( int, std::exception );
  void handleSSMPNotices();
  struct message_def *acquireMsg( int pid, Verifier_t verifier );
  bool decrNoticeMsgRef ( int bufIndex, int pid, Verifier_t verifier );
  void msgQueueStats( void );

  bool                       shutdown;
  bool                       noticeSignaled; 
  int                        Nid;
  int                        nidBase;
  char                       *clientBuffers;
  size_t                     sharedMemHdrSize_;
  size_t                     sharedBufferSize_;
  int                        cmid;
  int                        qid;
  int                        acquiredBufferCount;
  int                        acquiredBufferCountMax;
  int                        availableBufferCountMin;
  int                        missedBufferCount;
  int                        sharedBuffersMax;
  pthread_t                  serialRequestTid_;
  pthread_t                  pendingNoticesTid_;
  pthread_t                  lioBufCleanupTid_;
  key_t                      sharedSegKeyBase;

  typedef list<PendingNotice> PendingNoticeList;

  PendingNoticeList          pendingNotices_;
  CLock                      pendingNoticesLock_;

  enum { MAX_DEAD_PIDS = 1000 };
  pid_t                      deadPids_[MAX_DEAD_PIDS];
  int                        deadPidsHead_;
  int                        deadPidsTail_;
  int                        deadPidsMax_;
  bool                       deadPidsOverflow_;
  CLock                      deadPidsLock_;

  void examineAlmostDeadPids();
  enum { MAX_ALMOST_DEAD_PIDS = 1000 };
  pid_t                      almostDeadPids_[MAX_ALMOST_DEAD_PIDS];
  int                        almostDeadPidsHead_;
  int                        almostDeadPidsTail_;
  int                        almostDeadPidsTotal_;
  int                        almostDeadPidsHandled_;
  int                        almostDeadPidsDeferred_;
  int                        almostDeadPidsError_;
  CLock                      almostDeadPidsLock_;

  static const int serviceRequestSize[];
  static const int serviceReplySize[];
  static const int requestSize[];

  verifierMap_t              verifierMap_;
  CLock                      verifierMapLock_;

};

class CNoticeMsg
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
 public:
    CNoticeMsg( int bufIndex
              , SharedMsgDef *buf
              , pid_t pid
              , Verifier_t verifier
              , SQ_LocalIOToClient::bcastPids_t *bcastPids);
    ~CNoticeMsg();

    // Indicate the the given pid is no longer using the buffer
    int clientDone ( pid_t pid, Verifier_t verifier );

    int getIndex ( ) { return bufIndex_; }

    int tsSecs () { return timestamp_.tv_sec; }

    void validateObj( void );

    enum { NOTICE_BUF_TIME_LIMIT = 300 };

 private:

    int bufIndex_;
    SharedMsgDef * buf_;
    pid_t pid_;
    Verifier_t verifier_;
    SQ_LocalIOToClient::bcastPids_t *bcastPids_;
    CLock bcastPidsLock_;
    struct timespec    timestamp_;
};


extern SQ_LocalIOToClient *SQ_theLocalIOToClient;

#endif
#endif
