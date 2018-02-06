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

#ifndef PSTARTD_H
#define PSTARTD_H

#include <string.h>

#include <list>
#include <map>
#include <string>
using namespace std;

#include "msgdef.h"
#include "lock.h"
#include "trafconf/trafconfig.h"

class CMonUtil
{
 public:
    CMonUtil( void );
    ~CMonUtil( void );

    int getNid() { return nid_; }
    int getPid() { return pid_; }
    int getPNid() { return pnid_; }
    int getZid() { return zid_; }
    const char *getProcName() { return processName_; }
    bool getTrace() { return trace_; }
    int getVerifier() { return verifier_; }

    char *MPIErrMsg ( int code );

    void processArgs( int argc, char *argv[] );

    bool requestGet ( ConfigType type,
                      const char *group,
                      const char *key,
                      bool resumeFlag,
                      struct Get_reply_def *& regData
                      );

    void requestExit ( void );

    bool requestNewProcess (int nid, PROCESSTYPE type,
                            const char *processName,
                            const char *progName, const char *inFile,
                            const char *outFile,
                            int progArgC, const char *progArgV,
//                            int argBegin[], int argEnd[],
                            int& newNid, int& newPid,
                            char *newProcName);

    bool requestProcInfo( const char *processName, int &nid, int &pid );

    void requestStartup ( void );


 private:
    char   processName_[MAX_PROCESS_PATH];   // current process name
    int    pnid_;          // current process physical node id
    int    zid_;           // current process node id
    int    nid_;           // current process node id
    int    pid_;           // current process process id
    Verifier_t verifier_;  // current process verifier
    char   port_[MPI_MAX_PORT_NAME]; // current process port
    bool   trace_;
};

class CRequest
{
 public:
    CRequest() {}
    virtual ~CRequest() {}

    virtual void performRequest() = 0;
};

class CNodeUpReq: public CRequest
{
 public:
    CNodeUpReq(int nid, char nodeName[], bool requiresDTM);
    virtual ~CNodeUpReq() {}

    void performRequest();

 private:
    int  nid_;
    int  zid_;
    bool requiresDTM_;
    char nodeName_[MPI_MAX_PROCESSOR_NAME];
};

class CShutdownReq: public CRequest
{
 public:
    CShutdownReq() {}
    virtual ~CShutdownReq() {}

    void performRequest() {}
};

class CPStartD : public CLock
{
 public:
    CPStartD( void );
    ~CPStartD( void );

    typedef enum { NodeUp } pStartD_t;

    void enqueueReq(CRequest * req);
    CRequest * getReq( void );
    int getReqCount( void );
    bool loadConfiguration( void );
    void startProcess( CPersistConfig *persistConfig );
    void startProcs ( bool requiresDTM );
    void waitForEvent( void );

 private:

    list<CRequest *>  workQ_;
};

#endif
