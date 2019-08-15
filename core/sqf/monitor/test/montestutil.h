///////////////////////////////////////////////////////////////////////////////
//
// @@@@@@ START COPYRIGHT @@@@@@
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
// @@@@@@ END COPYRIGHT @@@@@@
//
///////////////////////////////////////////////////////////////////////////////

// header file for common test routines.

#ifndef MONTESTUTIL_H
#define MONTESTUTIL_H

#include <set>
#include "msgdef.h"

//
// Monitor test utility 
//
class MonTestUtil
{
 public:
    void processArgs( int argc, char *argv[] );

    void InitLocalIO( int my_pnid = -1 );

    bool requestNodeDown (int nid);

    void requestExit ( void );
    
    bool requestClose( const char * closeProcessName
                     , Verifier_t closeProcessVerifier );

    void requestKill( const char *name, Verifier_t verifier );

    bool requestNewProcess( int nid
                          , PROCESSTYPE type
                          , bool nowait
                          , const char *processName
                          , const char *progName
                          , const char *inFile
                          , const char *outFile
                          , int progArgC
                          , char *progArgV[]
                          , int &newNid
                          , int &newPid
                          , Verifier_t &newVerifier
                          , char *newProcName
                          , bool unhooked = false );

    bool requestNotice( int nid
                      , int pid
                      , Verifier_t verifier
                      , const char *processName
                      , bool cancelFlag
                      , _TM_Txid_External &transid );

    bool requestOpen( const char *processName
                    , Verifier_t openProcessVerifier
                    , int deathNotice
                    , char *port );

    bool requestSendEvent( int targetNid
                         , int targetPid
                         , Verifier_t targetVerifier
                         , const char * targetProcessName
                         , PROCESSTYPE type
                         , int eventId
                         , const char *eventData );

    bool requestProcInfo( const char *processName
                        , int &nid
                        , int &pid
                        , Verifier_t &verifier );
    bool requestSet ( ConfigType type, const char *group, const char *key,
                      const char *value );

    bool requestGet ( ConfigType type, const char *group, const char *key,
                      bool resumeFlag, struct Get_reply_def *& regData );

    void requestShutdown ( ShutdownLevel level );

    void requestStartup ( void );

    bool requestTmReady ( void );

    bool requestNodeInfo ( int targetNid,
                           bool resumeFlag,
                           int lastNid,
                           int lastPNid,
                           struct NodeInfo_reply_def *& nodeData);

    const char * getProcName() { return processName_; }
    char * getPort() { return port_; }
    int getNid() { return nid_; }
    int getPid() { return pid_; }
    void setShutdownBeforeStartup ( bool flag ) { shutdownBeforeStartup_ = flag; }
    bool getShutdownBeforeStartup () { return shutdownBeforeStartup_; }
    void setNodedownBeforeStartup ( bool flag ) { nodedownBeforeStartup_ = flag; }
    bool getNodedownBeforeStartup () { return  nodedownBeforeStartup_; }
    void setTrace ( bool flag ) { trace_ = flag; }
    bool getTrace () { return trace_; }

    bool validateNodeCount(int minNodesExpected);
    int  getNodeCount ( void );
    char * MPIErrMsg ( int code );
    int getVerifier() { return verifier_; }
    bool openProcess( const char *procName
                    , Verifier_t procVerifier
                    , int deathNotice
                    , MPI_Comm &comm);
    bool closeProcess ( MPI_Comm &comm );
    std::list<int> getTests() { return testNums_; }

 private:
    void flush_incoming_msgs( bool *TestShutdown = NULL );

    char   processName_[MAX_PROCESS_PATH];   // current process name
    int    nid_;           // current process node id
    int    pid_;           // current process process id
    Verifier_t verifier_;  // current process verifier
    char   port_[MPI_MAX_PORT_NAME]; // current process port
    bool   shutdownBeforeStartup_;
    bool   nodedownBeforeStartup_;
    bool   trace_;
    std::list<int> testNums_;  // specified test numbers to run
};


void shell_locio_trace(const char *where, const char *format, va_list ap);

int mon_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string);

void get_server_death (char *my_name);

void get_shutdown (char *my_name);

const char *MessageTypeString( MSGTYPE type );

const char *StateString( int state);

#endif

