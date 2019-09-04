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

#ifndef REPLICATE_H_
#define REPLICATE_H_

#include <list>
using namespace std;

#include "lock.h"
#include "config.h"
#include "device.h"
#include "process.h"
#include "open.h"
#include "notice.h"
#include "nameserverconfig.h"



class CReplObj
{
 protected:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CReplObj();
    virtual ~CReplObj();

    virtual bool replicate(struct internal_msg_def *& msg) = 0;
    int replSize() { return replSize_; }

    enum ReplType {ReplSet, ReplClose, ReplOpen, ReplNotice, ReplEvent,
                   ReplClone, ReplProcess, ReplExit, ReplKill, ReplDevice};

    void * operator new(size_t size) throw();
    void operator delete(void *deadObject, size_t size);
    void validateObj(void);

protected:
    int replSize_;
    const int msgAlignment_;

private:
    CReplObj * nextFree_;
    static CLock      freeListLock_;

    static int calcAllocSize();

    enum { ALLOC_OBJS = 150 };
    static CReplObj *freeListHead_;
    static int objAllocSize_;
};


class CReplActivateSpare: public CReplObj
{
public:
    CReplActivateSpare(int sparePnid, int downPnid);
    virtual ~CReplActivateSpare();

    bool replicate(struct internal_msg_def *& msg);

private:
    int downPnid_;
    int sparePnid_;
};

class CReplConfigData: public CReplObj
{
public:
    CReplConfigData(CConfigKey* key);
    virtual ~CReplConfigData();

    bool replicate(struct internal_msg_def *& msg);

private:
    CConfigKey *key_;
};

class CReplOpen: public CReplObj
{
public:
    CReplOpen(CProcess *process, COpen *open);
    virtual ~CReplOpen();

    bool replicate(struct internal_msg_def *& msg);

private:
    CProcess * process_;
    COpen * open_;

};

class CReplEvent: public CReplObj
{
public:
    CReplEvent( int event_id
              , int length
              , char *data
              , int targetNid
              , int targetPid
              , Verifier_t targetVerifier);
    CReplEvent(int, int, char *, int, int);
    virtual ~CReplEvent();

    bool replicate(struct internal_msg_def *& msg);

    void * operator new(size_t size);
    void operator delete(void *deadObject, size_t size);

private:
    int event_id_;
    int length_;
    int targetNid_;
    int targetPid_;
    Verifier_t targetVerifier_;

    enum {SMALL_DATA_SIZE=50};
    char data_[SMALL_DATA_SIZE];
    char *bigData_;
};

class CReplProcess: public CReplObj
{
public:
    CReplProcess(CProcess* process);
    virtual ~CReplProcess();

    bool replicate(struct internal_msg_def *& msg);

private:
    CProcess* process_;
    int nameLen_;
    int infileLen_;
    int outfileLen_;
#ifdef NAMESERVER_PROCESS
    int pathLen_;
    int ldpathLen_;
    int programLen_;
#endif
    int argvLen_;
};

class CReplProcInit: public CReplObj
{
public:
    CReplProcInit(CProcess* process, void * tag, int result, int parentNid);
    virtual ~CReplProcInit();

    bool replicate(struct internal_msg_def *& msg);

private:
    int       result_;
    int       nid_;
    int       pid_;
    Verifier_t verifier_;
    STATE     state_;
    int       parentNid_;
    void *    tag_;
    char name_[MAX_PROCESS_NAME];
};

class CReplClone: public CReplObj
{
public:
    CReplClone(CProcess* process);
    virtual ~CReplClone();

    bool replicate(struct internal_msg_def *& msg);

private:
    CProcess* process_;
    int nameLen_;
    int portLen_;
    int infileLen_;
    int outfileLen_;
    int argvLen_;
#ifdef NAMESERVER_PROCESS
    int pathLen_;
    int ldpathLen_;
    int programLen_;
#endif
};

class CReplExit: public CReplObj
{
public:
    CReplExit(int nid, int pid, Verifier_t verifier, const char * name, bool abended);
    virtual ~CReplExit();

    bool replicate(struct internal_msg_def *& msg);

private:
    int nid_;
    int pid_;
    Verifier_t verifier_;
    char name_[MAX_PROCESS_NAME];
    bool abended_;
};

class CReplExitNs: public CReplObj
{
public:
    CReplExitNs( int nid
               , int pid
               , Verifier_t verifier
               , const char *name
               , bool abended
               , struct message_def *msg
               , int  sockFd
               , int  origPNid );
    virtual ~CReplExitNs();

    bool replicate(struct internal_msg_def *& msg);

private:
    int nid_;
    int pid_;
    Verifier_t verifier_;
    char name_[MAX_PROCESS_NAME];
    bool abended_;
    struct message_def *msg_;
    int  sockFd_;
    int  origPNid_;
};

class CReplKill: public CReplObj
{
public:
    CReplKill(int nid, int pid, Verifier_t verifier, bool abort);
    virtual ~CReplKill();

    bool replicate(struct internal_msg_def *& msg);

private:
    int nid_;
    int pid_;
    Verifier_t verifier_;
    bool abort_;
};

#ifndef NAMESERVER_PROCESS
class CReplDevice: public CReplObj
{
public:
    CReplDevice(CLogicalDevice* process);
    virtual ~CReplDevice();

    bool replicate(struct internal_msg_def *& msg);

private:
    CLogicalDevice* ldev_;
};
#endif

#ifndef NAMESERVER_PROCESS
class CReplDump: public CReplObj
{
public:
    CReplDump(CProcess *process);
    virtual ~CReplDump();

    bool replicate(struct internal_msg_def *& msg);

private:
    CProcess * process_;
};
#endif

#ifndef NAMESERVER_PROCESS
class CReplDumpComplete: public CReplObj
{
public:
    CReplDumpComplete(CProcess *process);
    virtual ~CReplDumpComplete();

    bool replicate(struct internal_msg_def *& msg);

private:
    CProcess * process_;
};
#endif

class CReplShutdown: public CReplObj
{
public:
    CReplShutdown(int pnid);
    virtual ~CReplShutdown();

    bool replicate(struct internal_msg_def *& msg);

private:
    int level_;
};

class CReplNameServerAdd: public CReplObj
{
public:
    CReplNameServerAdd(CNameServerConfig *config, CProcess *process);
    virtual ~CReplNameServerAdd();

    bool replicate(struct internal_msg_def *& msg);

private:
    CNameServerConfig *config_;
    CProcess          *process_;
};

class CReplNameServerDelete: public CReplObj
{
public:
    CReplNameServerDelete(CNameServerConfig *config, CProcess *process);
    virtual ~CReplNameServerDelete();

    bool replicate(struct internal_msg_def *& msg);

private:
    CNameServerConfig *config_;
    CProcess          *process_;
};

class CReplNodeAdd: public CReplObj
{
public:
    CReplNodeAdd(CLNodeConfig *lnodeConfig, CProcess *process);
    virtual ~CReplNodeAdd();

    bool replicate(struct internal_msg_def *& msg);

private:
    CLNodeConfig *lnodeConfig_;
    CProcess     *process_;
};

class CReplNodeDelete: public CReplObj
{
public:
    CReplNodeDelete(CPNodeConfig *pnodeConfig, CProcess *process);
    virtual ~CReplNodeDelete();

    bool replicate(struct internal_msg_def *& msg);

private:
    CPNodeConfig *pnodeConfig_;
    CProcess     *process_;
};

class CReplNodeDown: public CReplObj
{
public:
    CReplNodeDown(int pnid);
    virtual ~CReplNodeDown();

    bool replicate(struct internal_msg_def *& msg);

private:
    int pnid_;
};

class CReplNodeName: public CReplObj
{
public:
    CReplNodeName( const char *current_name
                 , const char *new_name
                 , CProcess *process);
    virtual ~CReplNodeName();

    bool replicate(struct internal_msg_def *& msg);

private:
    string current_name_;
    string new_name_;
    CProcess *process_;
};

class CReplNodeUp: public CReplObj
{
public:
    CReplNodeUp(int pnid);
    virtual ~CReplNodeUp();

    bool replicate(struct internal_msg_def *& msg);

private:
    int pnid_;
};

#ifdef EXCHANGE_CPU_SCHEDULING_DATA
class CReplSchedData: public CReplObj
{
public:
    CReplSchedData();
    virtual ~CReplSchedData();

    bool replicate(struct internal_msg_def *& msg);

private:

};
#endif

#ifndef NAMESERVER_PROCESS
class CReplStdioData: public CReplObj
{
public:
    CReplStdioData(int nid, int pid, StdIoType type, ssize_t count, char *data);
    virtual ~CReplStdioData();

    bool replicate(struct internal_msg_def *& msg);

private:
    int nid_;
    int pid_;
    StdIoType type_;
    ssize_t count_;
    char *data_;
};
#endif

#ifndef NAMESERVER_PROCESS
class CReplStdinReq: public CReplObj
{
public:
    CReplStdinReq(int nid, int pid, StdinReqType type, int supplierNid, int supplierPid);
    virtual ~CReplStdinReq();

    bool replicate(struct internal_msg_def *& msg);

private:
    int nid_;
    int pid_;
    StdinReqType type_;
    int supplierNid_;
    int supplierPid_;
};
#endif


class CReplUniqStr: public CReplObj
{
public:
    CReplUniqStr(int nid, int id, const char * dataValue);
    virtual ~CReplUniqStr();

    bool replicate(struct internal_msg_def *& msg);

private:
    int nid_;
    int id_;
    string dataValue_;
};


class CReplicate
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CReplicate();
    virtual ~CReplicate();

    void FillSyncBuffer( struct internal_msg_def *&msg );
    void addItem(CReplObj *item);
    void stats();
    bool haveSyncData();
    inline void SetSyncClusterData() { syncClusterData_ = true; }

private:
    // List of objects to replicate
    typedef list<CReplObj *> replList_t;
    replList_t replList_;


    int maxListSize_;

    bool syncClusterData_;

    CLock replListLock_;
};

#endif
