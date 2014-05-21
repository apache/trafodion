///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
//
///////////////////////////////////////////////////////////////////////////////

// header file for common test routines.

#ifndef MONTESTUTIL_H
#define MONTESTUTIL_H

#include <set>
#include "msgdef.h"

class MonTestUtil
{
 public:
    void processArgs( int argc, char *argv[] );

    void InitLocalIO( int my_pnid = -1 );

    bool requestClose (const char * closeProcessName);

    bool requestNodeDown (int nid);

    void requestExit ( void );

    void requestKill( const char *name );

    bool requestNewProcess (int nid, PROCESSTYPE type, bool nowait,
                            const char *processName,
                            const char *progName, const char *inFile,
                            const char *outFile, int progArgC, char *progArgV[],
                            int& newNid, int& newPid, char *newProcName );

    bool requestNotice( int nid, int pid, bool cancelFlag,
                        _TM_Txid_External &transid );

    bool requestOpen( const char *processName, int deathNotice, char *port );

    // Get process info from monitor by process name
    bool requestProcInfo( const char *processName, int &nid, int &pid );

    bool requestSet ( ConfigType type, const char *group, const char *key,
                      const char *value );

    bool requestGet ( ConfigType type, const char *group, const char *key,
                      bool resumeFlag, struct Get_reply_def *& regData );

    bool requestSendEvent (int targetNid, int targetPid,
                           PROCESSTYPE type, int eventId,
                           const char *eventData );

    void requestShutdown ( ShutdownLevel level );

    void requestStartup ( void );

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
    void setTrace ( bool flag ) { trace_ = flag; }
    bool getTrace () { return trace_; }

    bool validateNodeCount(int minNodesExpected);
    int  getNodeCount ( void );
    char * MPIErrMsg ( int code );
    bool openProcess (const char * procName, int deathNotice, MPI_Comm &comm);
    bool closeProcess ( MPI_Comm &comm );
    std::list<int> getTests() { return testNums_; }

 private:
    void flush_incoming_msgs( bool *TestShutdown = NULL );

    char   processName_[MAX_PROCESS_PATH];   // current process name
    int    nid_;           // current process node id
    int    pid_;           // current process process id
    char   port_[MPI_MAX_PORT_NAME]; // current process port
    bool   shutdownBeforeStartup_;
    bool   trace_;
    std::list<int> testNums_;  // specified test numbers to run
};


void shell_locio_trace(const char *where, const char *format, va_list ap);

int mon_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string);

void get_server_death (char *my_name);

void get_shutdown (char *my_name);

const char *MessageTypeString( MSGTYPE type );

#endif
