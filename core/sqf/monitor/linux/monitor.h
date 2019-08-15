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

#ifndef MONITOR_H_
#define MONITOR_H_

#include "msgdef.h"
#include "cluster.h"
#include "process.h"


#define MAX_PROCESSES            2048
#define MAX_PORTFILEOPEN_RETRIES    60
#define MAX_PORTFILEOPEN_DELAY       5   // seconds (5*60=300 = 5 min)
#define MIN_PORTFILEOPEN_DELAY       1   // seconds (1*60=60  = 1 min)
#define DEFAULT_PORTFILEOPEN_DELAY   2   // seconds (2*60=120 = 2 min)

#define SUCCESS 0
#define FAILURE 1


class CMonitor : public CCluster
{
#ifndef NAMESERVER_PROCESS
friend class SQ_LocalIOToClient;
friend class CExternalReq;
#endif
public:
    int OpenCount;
    int NoticeCount;
    int ProcessCount;
    int NumOutstandingIO;     // Current # of I/Os outstanding
    int NumOutstandingSends;  // Current # of Sends outstanding

#ifdef NAMESERVER_PROCESS
    CMonitor(void);
#else
    CMonitor( int procTermSig );
#endif
    ~CMonitor( void );

    bool  CompleteProcessStartup( struct message_def *msg );
    void  IncOpenCount(void);
    void  IncNoticeCount(void);
    void  IncProcessCount(void);
    void  DecrOpenCount(void);
    void  DecrNoticeCount(void);
    void  DecrProcessCount(void);
#ifndef NAMESERVER_PROCESS
    void  StartPrimitiveProcesses( void );  
#endif
#ifndef NAMESERVER_PROCESS
    void  openProcessMap ( void );
    void  writeProcessMapEntry ( const char * buf );
    void  writeProcessMapBegin( const char *name
                              , int nid
                              , int pid
                              , int verifier
                              , int parentNid
                              , int parentPid
                              , int parentVerifier
                              , const char *program );
    void  writeProcessMapEnd( const char *name
                            , int nid
                            , int pid
                            , int verifier
                            , int parentNid
                            , int parentPid
                            , int parentVerifier
                            , const char *program );
#endif
    int   GetProcTermSig() { return procTermSig_; }
    int   PackProcObjs(char *&buffer);
    void  UnpackProcObjs(char *&buffer, int procCount);
#ifdef USE_SEQUENCE_NUM
    long long GetTimeSeqNum();  // time based seq number
#endif
protected:
private:
    int  Last_error;     // last MPI error returned
#ifndef NAMESERVER_PROCESS
    int  processMapFd;   // file desc for process map file
#endif

#ifdef DELAY_TP
    void Delay_TP(char *tpName);
#endif

    char *ProcCopy(char *bufPtr, CProcess *process);

 private:
    int procTermSig_;

#ifdef USE_SEQUENCE_NUM
    struct timespec savedTime_;  // for time based seq num
    CLock TimeSeqNumLock_;       // lock for savedTime_
#endif
};

#endif /*MONITOR_H_*/
