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

#include <stdio.h>
#include <string.h>

#include "replicate.h"
#include "redirector.h"
#include "clusterconf.h"
#include "lock.h"
#include "lnode.h"
#include "pnode.h"
#include "montrace.h"
#include "monsonar.h"

extern int MyPNID;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CMonStats *MonStats;
#ifndef NAMESERVER_PROCESS
extern CRedirector Redirector;
#endif
CReplicate Replicator;

int CReplObj::objAllocSize_ = CReplObj::calcAllocSize();
CReplObj * CReplObj::freeListHead_ = 0;
CLock CReplObj::freeListLock_;

CReplObj::CReplObj(): 
    replSize_ (0),
    msgAlignment_ ( __alignof__(internal_msg_def) - 1 ),
    nextFree_ (NULL)
{
}

CReplObj::~CReplObj()
{
}

void CReplObj::validateObj()
{
    if ((strncmp((const char *)&eyecatcher_, "RPL", 3) !=0 ) &&
        (strncmp((const char *)&eyecatcher_, "RpL", 3) !=0 ))
    {  // Not a valid object
        abort();
    }
}

struct dummy_sizeof_def {};
#ifndef EXCHANGE_CPU_SCHEDULING_DATA
struct dummy1_sizeof_def {};
#endif

// Determine the maximum size of a replication object (excluding CReplEvent)
int CReplObj::calcAllocSize()
{
#ifdef NAMESERVER_PROCESS
    return          max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(sizeof(CReplNameServerAdd),
#else
    return          max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(max(sizeof(CReplNameServerAdd),
#endif
                                                                                                        sizeof(CReplNodeName)),
                                                                                                    sizeof(CReplNodeAdd)),
                                                                                                sizeof(CReplNodeDelete)),
                                                                                            sizeof(dummy_sizeof_def)),
                                                                                        sizeof(dummy_sizeof_def)),
#ifdef EXCHANGE_CPU_SCHEDULING_DATA
                                                                                    sizeof(CReplSchedData)),
#else
                                                                                    sizeof(dummy1_sizeof_def)),
#endif
                                                                                sizeof(CReplActivateSpare)),
                                                                            sizeof(CReplConfigData)),
                                                                        sizeof(CReplOpen)),
                                                                    sizeof(CReplProcInit)),
                                                                sizeof(CReplProcess)),
                                                            sizeof(CReplClone)),
                                                        sizeof(CReplExit)),
                                                    sizeof(CReplKill)),
#ifdef NAMESERVER_PROCESS
                                                sizeof(CReplExitNs)),
#else
                                                sizeof(CReplDevice)),
#endif
                                            sizeof(CReplNodeDown)),
                                        sizeof(CReplNodeUp)),
#ifdef NAMESERVER_PROCESS
                                    sizeof(dummy_sizeof_def)),
#else
                                    sizeof(CReplDump)),
#endif
#ifdef NAMESERVER_PROCESS
                                sizeof(dummy_sizeof_def)),
#else
                                sizeof(CReplDumpComplete)),
#endif
#ifdef NAMESERVER_PROCESS
                            sizeof(dummy_sizeof_def)),
#else
                            sizeof(CReplStdioData)),
#endif
#ifdef NAMESERVER_PROCESS
                        sizeof(dummy_sizeof_def)),
#else
                        sizeof(CReplStdinReq)),
#endif
                    sizeof(CReplShutdown));
}

void * CReplObj::operator new(size_t ) throw()
{
    freeListLock_.lock();

    CReplObj * obj = freeListHead_;

    if (obj)
    {
        freeListHead_ = obj->nextFree_;
    }
    else
    {   // No more CReplObj objects available

        // Allocate a block memory for holding CReplObj objects
        CReplObj * replObj
            = (CReplObj* ) ::new char[ALLOC_OBJS * objAllocSize_];

        if (replObj == 0) return 0;

        // Set returned object pointer
        obj = replObj;

        // Create linked list of free objects
        char * p;
        p = (char *) replObj;
        p += objAllocSize_;
        replObj = (CReplObj *) p;
        freeListHead_ = replObj;

        for (int i=0; i < ALLOC_OBJS-2; i++)
        {
            p += objAllocSize_;
            replObj->nextFree_ = (CReplObj *) p;
            replObj = (CReplObj *) p;
        }

        replObj->nextFree_ = 0;        
    }

    freeListLock_.unlock();

    return obj;
}

void CReplObj::operator delete(void *deadObject, size_t )
{
    if (deadObject == 0) return;

    CReplObj *carcass = (CReplObj *) deadObject;

    freeListLock_.lock();

    carcass->nextFree_ = freeListHead_;
    freeListHead_ = carcass;

    freeListLock_.unlock();
}


CReplActivateSpare::CReplActivateSpare(int sparePnid, int downPnid) 
                  : downPnid_(downPnid)
                  , sparePnid_(sparePnid)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLS", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( spare_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplActivateSpare::CReplActivateSpare";
        trace_printf("%s@%d  - Queuing activate spare, spare pnid=%d, down pnid=%d\n",
                     method_name, __LINE__, sparePnid_, downPnid_);
    }
}

CReplActivateSpare::~CReplActivateSpare()
{
    const char method_name[] = "CReplActivateSpare::~CReplActivateSpare";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
        trace_printf("%s@%d - Activate spare replication for spare pnid=%d, down pnid=%d\n", method_name, __LINE__, sparePnid_, downPnid_ );

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rpls", 4);
}


bool CReplActivateSpare::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplActivateSpare::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d" " - Replicating activate spare, spare pnid=%d, down pnid=%d\n", method_name, __LINE__, sparePnid_, downPnid_);

    // build message to replicate this process kill to other nodes
    msg->type = InternalType_ActivateSpare;
    msg->u.activate_spare.down_pnid = downPnid_;
    msg->u.activate_spare.spare_pnid = sparePnid_;

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplConfigData::CReplConfigData(CConfigKey* key): key_(key)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLA", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( set_def )
                 + strlen(key_->GetValue()) + 1  + msgAlignment_)
                 & ~msgAlignment_;

    if (trace_settings & TRACE_SYNC_DETAIL)
    {
        const char method_name[] = "CReplConfigData::CReplConfigData";
        trace_printf("%s@%d  - Queuing key %s::%s\n",
                     method_name, __LINE__,
                     key_->GetGroup()->GetName(), key_->GetName());
    }
}

CReplConfigData::~CReplConfigData()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rpla", 4);
}

bool CReplConfigData::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplConfigData::replicate";
    TRACE_ENTRY;

    if (trace_settings & TRACE_SYNC)
        trace_printf("%s@%d  - Replicating key %s::%s\n",
                     method_name, __LINE__,
                     key_->GetGroup()->GetName(), key_->GetName());

    // Build message to replicate this key/value to other nodes
    msg->type = InternalType_Set;
    msg->u.set.type = key_->GetGroup()->GetType();
    strcpy(msg->u.set.group, key_->GetGroup()->GetName());
    strcpy(msg->u.set.key, key_->GetName());
    strcpy(&msg->u.set.valueData, key_->GetValue());

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

//===========================================================================
CReplUniqStr::CReplUniqStr(int nid, int id, const char * dataValue)
    : nid_(nid), id_(id), dataValue_(dataValue)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLR", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( uniqstr_def )
                 + dataValue_.size() + 1  + msgAlignment_)
                 & ~msgAlignment_;

    if (trace_settings & TRACE_SYNC)
    {
        const char method_name[] = "CReplUniqStr::CReplUniqStr";
        trace_printf("%s@%d  - Queuing unique string (%d, %d)\n",
                     method_name, __LINE__, nid_, id_ );
    }
}

CReplUniqStr::~CReplUniqStr()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplr", 4);
}

bool CReplUniqStr::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplUniqStr::replicate";
    TRACE_ENTRY;

    if (trace_settings & TRACE_SYNC)
        trace_printf("%s@%d  - Replicating unique string (%d, %d)\n",
                     method_name, __LINE__, nid_, id_);

    // Build message to replicate this key/value to other nodes
    msg->type = InternalType_UniqStr;
    msg->u.uniqstr.nid = nid_;
    msg->u.uniqstr.id = id_;
    strcpy(&msg->u.uniqstr.valueData, dataValue_.c_str());

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

//===========================================================================

CReplOpen::CReplOpen(CProcess *process, COpen *open)
  : process_(process), open_(open)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLC", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( open_def ) + msgAlignment_)
                & ~msgAlignment_;
    if (trace_settings & TRACE_SYNC_DETAIL)
    {
        const char method_name[] = "CReplOpen::CReplOpen";

        trace_printf("%s@%d  - Queuing open process %s (%d, %d), opened "
                     "process %s (%d, %d)\n",
                     method_name, __LINE__, process_->GetName(),
                     process_->GetNid(), process_->GetPid(), open_->Name,
                     open_->Nid, open_->Pid);
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplOpen::~CReplOpen()
{
    const char method_name[] = "CReplOpen::~CReplOpen";
    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplc", 4);
}

bool CReplOpen::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplOpen::replicate";
    TRACE_ENTRY;

    if (process_->GetState() != State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
            trace_printf("%s@%d - Replicating open process %s "
                         "(%d, %d), opened process %s (%d, %d)\n",
                         method_name, __LINE__, process_->GetName(),
                         process_->GetNid(), process_->GetPid(), open_->Name,
                         open_->Nid, open_->Pid);

        // Build message to replicate this open to other nodes
        msg->type = InternalType_Open;
        msg->u.open.nid = process_->GetNid();
        msg->u.open.pid = process_->GetPid();
        msg->u.open.verifier = process_->GetVerifier();
        msg->u.open.opened_nid = open_->Nid;
        msg->u.open.opened_pid = open_->Pid;
        msg->u.open.opened_verifier = open_->Verifier;

        // Advance sync buffer pointer
        Nodes->AddMsg( msg, replSize() );
    }
    else
    {
        if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
            trace_printf("%s@%d - Not replicating open process %s "
                         "(%d, %d), opened process %s (%d, %d) since "
                         "process has stopped.\n",
                         method_name, __LINE__, process_->GetName(),
                         process_->GetNid(), process_->GetPid(), open_->Name,
                         open_->Nid, open_->Pid);
    }

    TRACE_EXIT;

    return true;
}

CReplEvent::CReplEvent( int event_id
                      , int length
                      , char *data
                      , int targetNid
                      , int targetPid
                      , Verifier_t targetVerifier)
           : event_id_(event_id)
           , length_(length)
           , targetNid_(targetNid)
           , targetPid_(targetPid)
           , targetVerifier_(targetVerifier)
           , bigData_(NULL)

{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLE", 4);

    if (length_ <= SMALL_DATA_SIZE)
    {
        memmove(data_, data, length_);
    }
    else
    {
        bigData_ = ::new char[length_];
        memmove(bigData_, data, length_);
    }

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( event_def ) + length_ + msgAlignment_)
                & ~msgAlignment_;

}

CReplEvent::~CReplEvent()
{
    if (length_ > SMALL_DATA_SIZE)
    {
        ::delete[] bigData_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rple", 4);
}

void * CReplEvent::operator new(size_t size)
{
    if (trace_settings & TRACE_SYNC)
    {
        const char method_name[] = "CReplEvent::operator new";
        trace_printf("%s@%d  - Allocating %d bytes\n",
                     method_name, __LINE__, (int) size);
    }

    void * p = ::new char[size];

    return p;
}

void CReplEvent::operator delete(void *deadObject, size_t size)
{
    if (trace_settings & TRACE_SYNC)
    {
        const char method_name[] = "CCReplEvent::operator delete";
        trace_printf("%s@%d  - Deleting %d bytes\n",
                     method_name, __LINE__, (int) size);
    }

    ::delete [] ((char *) deadObject);
}

bool CReplEvent::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplEvent::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d - Event id=%d for process (%d, %d:%d)\n", method_name, __LINE__, event_id_, targetNid_, targetPid_, targetVerifier_);

    // Build message to replicate this event to other nodes
    msg->type = InternalType_Event;
    msg->u.event.nid = targetNid_;
    msg->u.event.pid = targetPid_;
    msg->u.event.verifier = targetVerifier_;
    msg->u.event.event_id = event_id_;
    msg->u.event.length = length_;
    memset(&msg->u.event.data, 0, MAX_SYNC_DATA);
    if (length_ <= SMALL_DATA_SIZE)
    {
        memmove(&msg->u.event.data, data_, length_);
    }
    else
    {
        memmove(&msg->u.event.data, bigData_, length_);
    }

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplProcess::CReplProcess(CProcess *process) : process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLF", 4);

    nameLen_ = strlen(process_->GetName()) + 1;
    infileLen_ = strlen(process->infile()) + 1;
    outfileLen_ = strlen(process->outfile()) + 1;
    argvLen_ = process->userArgvLen();
    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof( process_def ) + nameLen_ + infileLen_
              + outfileLen_ + argvLen_ + msgAlignment_) & ~msgAlignment_ ;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplProcess::CReplProcess";
        trace_printf("%s@%d  - Queuing replicating pre-clone for new "
                     "process %s %s\n", method_name, __LINE__,
                     process_->GetName(), (process_->IsBackup()?" Backup":""));
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplProcess::~CReplProcess()
{
    const char method_name[] = "CReplProcess::~CReplProcess";

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplf", 4);
}

bool CReplProcess::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplProcess::replicate";
    TRACE_ENTRY;

    if (process_->GetState() != State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
            trace_printf("%s@%d - Replicating pre-clone for new process %s %s\n", method_name, __LINE__, process_->GetName(), (process_->IsBackup()?" Backup":""));

        // Build message to replicate this process to other nodes
        msg->type = InternalType_Process;
        msg->u.process.nid = process_->GetNid();
        msg->u.process.pid = process_->GetPid();
#ifdef NAMESERVER_PROCESS
        msg->u.process.verifier = process_->GetVerifier();
#endif
        msg->u.process.type = process_->GetType();
        msg->u.process.priority = process_->GetPriority();
        msg->u.process.backup = process_->IsBackup();
        msg->u.process.unhooked = process_->IsUnhooked();
        msg->u.process.tag = process_;
        msg->u.process.parent_nid = process_->GetParentNid();
        msg->u.process.parent_pid = process_->GetParentPid();
        msg->u.process.parent_verifier = process_->GetParentVerifier();
        msg->u.process.pair_parent_nid = process_->GetPairParentNid();
        msg->u.process.pair_parent_pid = process_->GetPairParentPid();
        msg->u.process.pair_parent_verifier = process_->GetPairParentVerifier();
#ifndef NAMESERVER_PROCESS
        msg->u.process.pathStrId =  process_->pathStrId();
        msg->u.process.ldpathStrId = process_->ldPathStrId();
        msg->u.process.programStrId = process_->programStrId();
#endif
        msg->u.process.argc = process_->argc();

        char * stringData = & msg->u.process.stringData;

        // Copy the process name
        msg->u.process.nameLen = nameLen_;
        memcpy(stringData, process_->GetName(), nameLen_ );
        stringData += nameLen_;

        // Copy the standard in file name
        msg->u.process.infileLen = infileLen_;
        memcpy(stringData, process_->infile(), infileLen_);
        stringData += infileLen_;

        // Copy the standard out file name
        msg->u.process.outfileLen = outfileLen_;
        memcpy(stringData, process_->outfile(), outfileLen_ );
        stringData += outfileLen_;

#ifdef NAMESERVER_PROCESS
        // Copy the path
        msg->u.process.pathLen = pathLen_;
        memcpy(stringData, process_->path(), pathLen_ );
        stringData += pathLen_;

        // Copy the ldpath
        msg->u.process.ldpathLen = ldpathLen_;
        memcpy(stringData, process_->ldpath(), ldpathLen_ );
        stringData += ldpathLen_;

        // Copy the program
        msg->u.process.programLen = programLen_;
        memcpy(stringData, process_->program(), programLen_ );
        stringData += programLen_;
#endif
        // Copy the program argument strings
        msg->u.process.argvLen =  argvLen_;
        memcpy(stringData, process_->userArgv(), argvLen_);

        // temp trace
        if (trace_settings & TRACE_PROCESS)
        {
            trace_printf( "%s@%d - replSize_=%d\n"
                          "        msg->u.process.name=%s, strlen(name)=%d\n"
                          "        msg->u.process.infile=%s, strlen(infile)=%d\n"
                          "        msg->u.process.outfile=%s, strlen(outfile)=%d\n"
#ifdef NAMESERVER_PROCESS
                          "        msg->u.process.path=%s, strlen(path)=%d\n"
                          "        msg->u.process.ldpath=%s, strlen(ldpath)=%d\n"
                          "        msg->u.process.program=%s, strlen(program)=%d\n"
#else
                          "        msg->u.process.programStrId=(%d,%d)\n"
                          "        msg->u.process.pathStrId=(%d,%d)\n"
                          "        msg->u.process.ldPathStrId=(%d,%d)\n"
#endif
                          "        msg->u.process.argc=%d, strlen(total argv)=%d, args=[%.*s]\n"
                        , method_name, __LINE__, replSize_
                        , &msg->u.process.stringData, nameLen_
                        , &msg->u.process.stringData+nameLen_, infileLen_
                        , &msg->u.process.stringData+nameLen_+infileLen_, outfileLen_
#ifdef NAMESERVER_PROCESS
                        , &msg->u.process.stringData+nameLen_+infileLen_+outfileLen_, pathLen_
                        , &msg->u.process.stringData+nameLen_+infileLen_+outfileLen_+pathLen_, ldpathLen_
                        , &msg->u.process.stringData+nameLen_+infileLen_+outfileLen_+pathLen_+ldpathLen_, programLen_
#else
                        , msg->u.process.programStrId.nid
                        , msg->u.process.programStrId.id
                        , msg->u.process.pathStrId.nid
                        , msg->u.process.pathStrId.id
                        , msg->u.process.ldpathStrId.nid
                        , msg->u.process.ldpathStrId.id
#endif
                        , msg->u.process.argc
#ifdef NAMESERVER_PROCESS
                        , argvLen_, argvLen_, &msg->u.clone.stringData+nameLen_+infileLen_+outfileLen_+pathLen_+ldpathLen_+programLen_);
#else
                        , argvLen_, argvLen_, &msg->u.clone.stringData+nameLen_+infileLen_+outfileLen_);
#endif
        }

        // Advance sync buffer pointer
        Nodes->AddMsg( msg, replSize() );
    }
    else
    {
        if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
            trace_printf("%s@%d - Not replicating pre-clone for new process %s %s since process has stopped.\n", method_name, __LINE__, process_->GetName(), (process_->IsBackup()?" Backup":""));
    }

    TRACE_EXIT;

    return true;
}

CReplProcInit::CReplProcInit( CProcess *process
                            , void *tag
                            , int result
                            , int parentNid)
              : result_(result)
              , nid_(0)
              , pid_(0)
              , verifier_(-1)
              , state_(State_Unknown)
              , parentNid_(parentNid)
              , tag_(tag)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLG", 4);


    if ( process != NULL )
    {
        nid_ = process->GetNid();
        pid_ = process->GetPid();
        verifier_ = process->GetVerifier();
        strcpy(name_, process->GetName());
        state_ = process->GetState();
    }
    else
    {
        name_[0] = 0;
    }

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( process_init_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplProcInit::CReplProcInit";
        trace_printf("%s@%d  - Queuing replicating proc init new "
                     "process %s, tag %p\n", method_name, __LINE__,
                     name_, tag_);
    }

}

CReplProcInit::~CReplProcInit()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplg", 4);
}

bool CReplProcInit::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplProcInit::replicate";
    TRACE_ENTRY;


    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d - Replicating proc init new process %s, result=%d\n",
                     method_name, __LINE__, name_, result_);

    // Build message to replicate process initialization data other nodes
    msg->type = InternalType_ProcessInit;
    msg->u.processInit.nid = nid_;
    msg->u.processInit.pid = pid_;
    msg->u.processInit.verifier = verifier_;
    strcpy (msg->u.processInit.name, name_);
    msg->u.processInit.state = state_;
    msg->u.processInit.result = result_;
    msg->u.processInit.tag = tag_;
    msg->u.processInit.origNid = parentNid_;

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplClone::CReplClone(CProcess *process) : process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLH", 4);

    nameLen_ = strlen(process_->GetName()) + 1;
    portLen_ = strlen(process_->GetPort()) + 1;
    infileLen_ = strlen(process->infile()) + 1;
    outfileLen_ = strlen(process->outfile()) + 1;
    argvLen_ = process->userArgvLen();
#ifdef NAMESERVER_PROCESS
    pathLen_ = strlen(process_->path()) + 1;
    ldpathLen_ = strlen(process_->ldpath()) + 1;
    programLen_ = strlen(process_->program()) + 1;
#endif
    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
#ifdef NAMESERVER_PROCESS
    replSize_ = (MSG_HDR_SIZE + sizeof( clone_def ) + nameLen_ + portLen_
                 + infileLen_ + outfileLen_ + argvLen_
                 + pathLen_ + ldpathLen_ + programLen_ + msgAlignment_
                 ) & ~msgAlignment_;
#else
    replSize_ = (MSG_HDR_SIZE + sizeof( clone_def ) + nameLen_ + portLen_
                 + infileLen_ + outfileLen_ + argvLen_ + msgAlignment_
                 ) & ~msgAlignment_;
#endif

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplClone::CReplClone";
        trace_printf("%s@%d  - Queuing replicate process %s (%d, %d:%d), port=%s\n",
                     method_name, __LINE__, process_->GetName(), process_->GetNid(),
                     process_->GetPid(), process_->GetVerifier(), process_->GetPort());
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplClone::~CReplClone()
{
    const char method_name[] = "CReplClone::~CReplClone";

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplh", 4);
}


bool CReplClone::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplClone::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d - Replicating process %s (%d, %d:%d) %s\n",
                     method_name, __LINE__,
                     process_->GetName(), process_->GetNid(), process_->GetPid(),
                     process_->GetVerifier(), (process_->IsBackup()?" Backup":""));

    // Build message to replicate this process to other nodes
    msg->type = InternalType_Clone;
    msg->u.clone.nid = process_->GetNid();
    msg->u.clone.type = process_->GetType();
    msg->u.clone.priority = process_->GetPriority();
    msg->u.clone.backup = process_->IsBackup();
    msg->u.clone.unhooked = process_->IsUnhooked();
#ifndef NAMESERVER_PROCESS
    msg->u.clone.pathStrId = process_->pathStrId();
    msg->u.clone.ldpathStrId = process_->ldPathStrId();
    msg->u.clone.programStrId = process_->programStrId();
#endif
    msg->u.clone.os_pid = process_->GetPid();
    msg->u.clone.verifier = process_->GetVerifier();
    msg->u.clone.prior_pid = process_->GetPriorPid ();
    process_->SetPriorPid ( 0 );
    msg->u.clone.parent_nid = process_->GetParentNid();
    msg->u.clone.parent_pid = process_->GetParentPid();
    msg->u.clone.parent_verifier = process_->GetParentVerifier();
    msg->u.clone.persistent = process_->IsPersistent();
    msg->u.clone.persistent_retries = process_->GetPersistentRetries();
    msg->u.clone.event_messages = process_->IsEventMessages();
    msg->u.clone.system_messages = process_->IsSystemMessages();
#ifdef NAMESERVER_PROCESS
    msg->u.clone.origPNidNs = process_->GetOrigPNidNs();
#endif
    msg->u.clone.argc = process_->argc();
    msg->u.clone.creation_time = process_->GetCreationTime();

    char * stringData = & msg->u.clone.stringData;

    // Copy the process name
    msg->u.clone.nameLen = nameLen_;
    memcpy(stringData, process_->GetName(),  nameLen_ );
    stringData += nameLen_;

    // Copy the port
    msg->u.clone.portLen = portLen_;
    memcpy(stringData, process_->GetPort(),  portLen_ );
    stringData += portLen_;

    // Copy the standard in file name
    msg->u.clone.infileLen = infileLen_;
    memcpy(stringData, process_->infile(), infileLen_);
    stringData += infileLen_;

    // Copy the standard out file name
    msg->u.clone.outfileLen = outfileLen_;
    memcpy(stringData, process_->outfile(),  outfileLen_ );
    stringData += outfileLen_;

#ifdef NAMESERVER_PROCESS
    // Copy the path
    msg->u.clone.pathLen = pathLen_;
    memcpy(stringData, process_->path(),  pathLen_ );
    stringData += pathLen_;

    // Copy the ldpath
    msg->u.clone.ldpathLen = ldpathLen_;
    memcpy(stringData, process_->ldpath(),  ldpathLen_ );
    stringData += ldpathLen_;

    // Copy the program
    msg->u.clone.programLen = programLen_;
    memcpy(stringData, process_->program(),  programLen_ );
    stringData += programLen_;
#endif

    // Copy the program argument strings
    msg->u.clone.argvLen =  argvLen_;
    memcpy(stringData, process_->userArgv(), argvLen_);

    // temp trace
#ifndef NAMESERVER_PROCESS
    if (trace_settings & TRACE_PROCESS)
    {
        trace_printf( "%s@%d - replSize_=%d\n"
                      "        msg->u.clone.name=%s, strlen(name)=%d\n"
                      "        msg->u.clone.port=%s, strlen(port)=%d\n"
                      "        msg->u.clone.infile=%s, strlen(infile)=%d\n"
                      "        msg->u.clone.outfile=%s, strlen(outfile)=%d\n"
                      "        msg->u.clone.programStrId=(%d,%d)\n"
                      "        msg->u.clone.pathStrId=(%d,%d)\n"
                      "        msg->u.clone.ldPathStrId=(%d,%d)\n"
                      "        msg->u.clone.argc=%d, strlen(total argv)=%d, args=[%.*s]\n"
                    , method_name, __LINE__, replSize_
                    , &msg->u.clone.stringData, nameLen_
                    , &msg->u.clone.stringData+nameLen_, portLen_
                    , &msg->u.clone.stringData+nameLen_+portLen_, infileLen_
                    , &msg->u.clone.stringData+nameLen_+portLen_+infileLen_, outfileLen_
                    , msg->u.clone.programStrId.nid
                    , msg->u.clone.programStrId.id
                    , msg->u.clone.pathStrId.nid
                    , msg->u.clone.pathStrId.id
                    , msg->u.clone.ldpathStrId.nid
                    , msg->u.clone.ldpathStrId.id
                    , msg->u.clone.argc
                    , argvLen_, argvLen_, &msg->u.clone.stringData+nameLen_+portLen_+infileLen_+outfileLen_);
    }
#else
    if (trace_settings & TRACE_PROCESS)
    {
        trace_printf( "%s@%d - replSize_=%d\n"
                      "        msg->u.clone.name=%s, strlen(name)=%d\n"
                      "        msg->u.clone.port=%s, strlen(port)=%d\n"
                      "        msg->u.clone.infile=%s, strlen(infile)=%d\n"
                      "        msg->u.clone.outfile=%s, strlen(outfile)=%d\n"
                      "        msg->u.clone.path=%s, strlen(path)=%d\n"
                      "        msg->u.clone.ldpath=%s, strlen(ldpath)=%d\n"
                      "        msg->u.clone.program=%s, strlen(program)=%d\n"
                      "        msg->u.clone.argc=%d, strlen(total argv)=%d, args=[%.*s]\n"
                    , method_name, __LINE__, replSize_
                    , &msg->u.clone.stringData, nameLen_
                    , &msg->u.clone.stringData+nameLen_, portLen_
                    , &msg->u.clone.stringData+nameLen_+portLen_, infileLen_
                    , &msg->u.clone.stringData+nameLen_+portLen_+infileLen_, outfileLen_
                    , &msg->u.clone.stringData+nameLen_+portLen_+infileLen_+outfileLen_, pathLen_
                    , &msg->u.clone.stringData+nameLen_+portLen_+infileLen_+outfileLen_+pathLen_, ldpathLen_
                    , &msg->u.clone.stringData+nameLen_+portLen_+infileLen_+outfileLen_+pathLen_+ldpathLen_, programLen_
                    , msg->u.clone.argc
                    , argvLen_, argvLen_, &msg->u.clone.stringData+nameLen_+portLen_+infileLen_+outfileLen_+pathLen_+ldpathLen_+programLen_);
    }
#endif
    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}


CReplExit::CReplExit( int nid
                    , int pid
                    , Verifier_t verifier
                    , const char *name
                    , bool abended)
          : nid_(nid)
          , pid_(pid)
          , verifier_(verifier)
          , abended_(abended)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLI", 4);

    strcpy(name_, name);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( exit_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplExit::CReplExit";
        trace_printf("%s@%d  - Queuing replicating process exit %s (%d, %d:%d),"
                     " abended=%d\n",
                     method_name, __LINE__, name_, nid_, pid_, verifier_, abended_);
    }
}

CReplExit::~CReplExit()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rpli", 4);
}


bool CReplExit::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplExit::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d" " - Replicating process exit %s (%d, %d:%d),"
                     " abended=%d\n", method_name, __LINE__,
                     name_, nid_, pid_, verifier_, abended_);

    // Build message to replicate this process exit to other nodes
    msg->type = InternalType_Exit;
    msg->u.exit.nid = nid_;
    msg->u.exit.pid = pid_;
    msg->u.exit.verifier = verifier_;
    strcpy(msg->u.exit.name, name_);
    msg->u.exit.abended = abended_;

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplExitNs::CReplExitNs( int nid
                        , int pid
                        , Verifier_t verifier
                        , const char *name
                        , bool abended
                        , struct message_def *msg
                        , int  sockFd
                        , int  origPNid )
            : nid_(nid)
            , pid_(pid)
            , verifier_(verifier)
            , abended_(abended)
            , msg_(msg)
            , sockFd_(sockFd)
            , origPNid_(origPNid)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLJ", 4);

    strcpy(name_, name);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( exit_ns_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplExitNs::CReplExitNs";
        trace_printf( "%s@%d  - Queuing replicating process exit %s (%d, %d:%d),"
                      " abended=%d, msg=%p, sockFd=%d, origPNid=%d\n"
                    , method_name, __LINE__
                    , name_, nid_, pid_, verifier_, abended_
                    , msg_, sockFd_, origPNid_ );
    }
}

CReplExitNs::~CReplExitNs()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplj", 4);
}


bool CReplExitNs::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplExitNs::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d" " - Replicating process exit %s (%d, %d:%d),"
                     " abended=%d\n", method_name, __LINE__,
                     name_, nid_, pid_, verifier_, abended_);

    // Build message to replicate this process exit to other nodes
    msg->type = InternalType_Exit;
    msg->u.exit_ns.nid = nid_;
    msg->u.exit_ns.pid = pid_;
    msg->u.exit_ns.verifier = verifier_;
    strcpy(msg->u.exit_ns.name, name_);
    msg->u.exit_ns.abended = abended_;
    msg->u.exit_ns.msg = msg_;
    msg->u.exit_ns.sockFd = sockFd_;
    msg->u.exit_ns.origPNid = origPNid_;

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}


CReplKill::CReplKill( int nid
                    , int pid
                    , Verifier_t verifier
                    , bool abort) 
          : nid_(nid)
          , pid_(pid)
          , verifier_(verifier)
          , abort_(abort)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLU", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( kill_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplKill::CReplKill";
        trace_printf( "%s@%d  - Queuing replicating process kill (%d, %d:%d)\n"
                    , method_name, __LINE__, nid, pid, verifier );
    }
}

CReplKill::~CReplKill()
{
    const char method_name[] = "CReplKill::~CReplKill";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
        trace_printf("%s@%d - Kill replication for process (%d, %d:%d)\n",
                     method_name, __LINE__, nid_, pid_, verifier_ );

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplu", 4);
}

bool CReplKill::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplKill::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf( "%s@%d" " - Replicating process kill (%d, %d:%d)\n"
                    , method_name, __LINE__, nid_, pid_, verifier_ );

    // build message to replicate this process kill to other nodes
    msg->type = InternalType_Kill;
    msg->u.kill.nid = nid_;
    msg->u.kill.pid = pid_;
    msg->u.kill.verifier = verifier_;
    msg->u.kill.persistent_abort = abort_;

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

#ifndef NAMESERVER_PROCESS
CReplDevice::CReplDevice(CLogicalDevice *ldev) : ldev_(ldev)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLK", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( device_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplDevice::CReplDevice";
        trace_printf("%s@%d  - Queuing replicating device %s, mounted=%d\n",
                     method_name, __LINE__, ldev_->name(),
                     ldev_->Primary->Mounted);
    }
}

CReplDevice::~CReplDevice()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplk", 4);
}


bool CReplDevice::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplDevice::replicate";
    TRACE_ENTRY;

    bool copied = false;
    CProcess *process;

    Nodes->GetNode( (char *) ldev_->name(), &process );
    if (process && process->IsStartupCompleted())
    {
        if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
            trace_printf("%s@%d" " - Replicating device %s, mounted=%d\n", method_name, __LINE__, ldev_->name(), ldev_->Primary->Mounted);

        msg->type = InternalType_Device;
        strcpy (msg->u.device.ldev_name,ldev_->name());
        msg->u.device.primary_mounted = ldev_->Primary->Mounted;
        if ( ldev_->Mirror )
        {
            msg->u.device.mirror_mounted = ldev_->Mirror->Mounted;
        }
        else
        {
            msg->u.device.mirror_mounted = false;
        }

        // Advance sync buffer pointer
        Nodes->AddMsg( msg, replSize() );

        copied = true;
    }
    else if ( !process )
    {
        // process is gone so indicate that its been replicated
        copied = true;
    }
    else
    {
        if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
            trace_printf("%s@%d" " - Process startup not completed so deferring replicating device %s\n", method_name, __LINE__, ldev_->name());
    }

    TRACE_EXIT;

    return copied;
}
#endif

#ifndef NAMESERVER_PROCESS
CReplDump::CReplDump(CProcess *process) : process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLL", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( dump_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplDump::CReplDump";
        trace_printf( "%s@%d" " - Queuing dump pending (%d,%d:%d) "
                      "dumper (%d,%d:%d)\n"
                    , method_name, __LINE__
                    , process_->GetNid()
                    , process_->GetPid()
                    , process_->GetVerifier()
                    , process_->GetDumperNid()
                    , process_->GetDumperPid()
                    , process_->GetDumperVerifier() );
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplDump::~CReplDump()
{
    const char method_name[] = "CReplDump::~CReplDump";

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rpll", 4);
}

bool CReplDump::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplDump::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
    {
        trace_printf("%s@%d" " - Replicating dump pending target (%d, %d) "
                     "dumper (%d, %d)\n",
                     method_name, __LINE__,
                     process_->GetNid(), process_->GetPid(),
                     process_->GetDumperNid(), process_->GetDumperPid());
    }

    msg->type = InternalType_Dump;
    msg->u.dump.nid = process_->GetNid();
    msg->u.dump.pid = process_->GetPid();
    msg->u.dump.verifier = process_->GetVerifier();
    msg->u.dump.dumper_nid = process_->GetDumperNid();
    msg->u.dump.dumper_pid = process_->GetDumperPid();
    msg->u.dump.dumper_verifier = process_->GetDumperVerifier();
    strcpy(msg->u.dump.core_file, process_->GetDumpFile());

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}
#endif

#ifndef NAMESERVER_PROCESS
CReplDumpComplete::CReplDumpComplete(CProcess *process) : process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLM", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( dump_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplDumpComplete::CReplDumpComplete";
        trace_printf( "%s@%d  - Queuing dump complete target (%d,%d:%d) "
                      "dumper (%d,%d:%d)\n"
                    , method_name, __LINE__
                    , process_->GetNid()
                    , process_->GetPid()
                    , process_->GetVerifier()
                    , process_->GetDumperNid()
                    , process_->GetDumperPid()
                    , process_->GetDumperVerifier());
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplDumpComplete::~CReplDumpComplete()
{
    const char method_name[] = "CReplDumpComplete::~CReplDumpComplete";

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplm", 4);
}

bool CReplDumpComplete::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplDumpComplete::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d" " - Replicating dump complete target (%d,%d:%d) "
                      "dumper (%d,%d:%d)\n"
                    , method_name, __LINE__
                    , process_->GetNid()
                    , process_->GetPid()
                    , process_->GetVerifier()
                    , process_->GetDumperNid()
                    , process_->GetDumperPid()
                    , process_->GetDumperVerifier() );
    }

    msg->type = InternalType_DumpComplete;
    msg->u.dump.nid = process_->GetNid();
    msg->u.dump.pid = process_->GetPid();
    msg->u.dump.verifier = process_->GetVerifier();
    msg->u.dump.dumper_nid = process_->GetDumperNid();
    msg->u.dump.dumper_pid = process_->GetDumperPid();
    msg->u.dump.dumper_verifier = process_->GetDumperVerifier();
    msg->u.dump.status = process_->GetDumpStatus();
    strcpy(msg->u.dump.core_file, process_->GetDumpFile());

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}
#endif

CReplShutdown::CReplShutdown(int level) : level_(level)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLD", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( shutdown_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplShutdown::CReplShutdown";
        trace_printf("%s@%d  - Queuing shutdown, level=%d\n",
                     method_name, __LINE__, level_);
    }
}

CReplShutdown::~CReplShutdown()
{
    const char method_name[] = "CReplShutdown::~CReplShutdown";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
        trace_printf("%s@%d - Shutdown replication for level=%d\n", method_name, __LINE__, level_ );

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rpld", 4);
}

bool CReplShutdown::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplShutdown::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d" " - Replicating shutdown, level=%d\n", method_name, __LINE__, level_);

    // build message to replicate this process kill to other nodes
    msg->type = InternalType_Shutdown;
    msg->u.shutdown.level = level_;

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplNameServerAdd::CReplNameServerAdd(CNameServerConfig *config, CProcess *process) 
            : config_(config)
            , process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RpLA", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( nameserver_add_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
    {
        const char method_name[] = "CReplNameServerAdd::CReplNameServerAdd";
        trace_printf( "%s@%d  - Queuing NameServer add replication: node-name=%s\n"
                    , method_name, __LINE__
                    , config_->GetName()
                    );
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplNameServerAdd::~CReplNameServerAdd()
{
    const char method_name[] = "CReplNameServerAdd::~CReplNameServerAdd";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf( "%s@%d - NameServer add replication for node-name=%s\n"
                    , method_name, __LINE__
                    , config_->GetName()
                    );

    delete config_;

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rPla", 4);
}

bool CReplNameServerAdd::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplNameServerAdd::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf( "%s@%d  - Replicating node add (%s): node-name=%s\n"
                    , method_name, __LINE__
                    , process_->GetName()
                    , config_->GetName()
                    );

    // build message to replicate this node add to other nodes
    msg->type = InternalType_NameServerAdd;
    msg->u.nameserver_add.req_nid = process_->GetNid();
    msg->u.nameserver_add.req_pid = process_->GetPid();
    msg->u.nameserver_add.req_verifier = process_->GetVerifier();
    STRCPY( msg->u.nameserver_add.node_name, config_->GetName() );

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplNameServerDelete::CReplNameServerDelete(CNameServerConfig *config, CProcess *process) 
               : config_(config)
               , process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RpLB", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( nameserver_delete_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
    {
        const char method_name[] = "CReplNameServerDelete::CReplNameServerDelete";
        trace_printf( "%s@%d  - Queuing NameServer delete replication: node-name=%s\n"
                    , method_name, __LINE__
                    , config_->GetName()
                    );
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplNameServerDelete::~CReplNameServerDelete()
{
    const char method_name[] = "CReplNameServerDelete::~CReplNameServerDelete";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf( "%s@%d - NameServer delete replication for node-name=%s\n"
                    , method_name, __LINE__
                    , config_->GetName()
                    );

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rPlb", 4);
}

bool CReplNameServerDelete::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplNameServerDelete::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf( "%s@%d  - Replicating nameserver delete (%s): node-name=%s\n"
                    , method_name, __LINE__
                    , process_->GetName()
                    , config_->GetName()
                    );

    // build message to replicate this node delete to other nodes
    msg->type = InternalType_NameServerDelete;
    msg->u.nameserver_delete.req_nid = process_->GetNid();
    msg->u.nameserver_delete.req_pid = process_->GetPid();
    msg->u.nameserver_delete.req_verifier = process_->GetVerifier();
    strcpy(msg->u.nameserver_delete.node_name, config_->GetName());

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplNodeAdd::CReplNodeAdd(CLNodeConfig *lnodeConfig, CProcess *process) 
            : lnodeConfig_(lnodeConfig)
            , process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLW", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( node_add_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
    {
        const char method_name[] = "CReplNodeAdd::CReplNodeAdd";
        trace_printf( "%s@%d  - Queuing node add replication: node-name=%s, "
                      "first_core=%d, last_core=%d, processors=%d, roles=%d\n"
                    , method_name, __LINE__
                    , lnodeConfig_->GetName()
                    , lnodeConfig_->GetFirstCore()
                    , lnodeConfig_->GetLastCore()
                    , lnodeConfig_->GetProcessors()
                    , lnodeConfig_->GetZoneType()
                    );
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplNodeAdd::~CReplNodeAdd()
{
    const char method_name[] = "CReplNodeAdd::~CReplNodeAdd";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf( "%s@%d - Node add replication for nid=%d, node-name=%s\n"
                    , method_name, __LINE__
                    , lnodeConfig_->GetNid()
                    , lnodeConfig_->GetName()
                    );

    CPNodeConfig *pnodeConfig = lnodeConfig_->GetPNodeConfig();
    delete pnodeConfig;
    delete lnodeConfig_;

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplw", 4);
}

bool CReplNodeAdd::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplNodeAdd::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf( "%s@%d  - Replicating node add (%s): node-name=%s, "
                      "first_core=%d, last_core=%d, processors=%d, roles=%d\n"
                    , method_name, __LINE__
                    , process_->GetName()
                    , lnodeConfig_->GetName()
                    , lnodeConfig_->GetFirstCore()
                    , lnodeConfig_->GetLastCore()
                    , lnodeConfig_->GetProcessors()
                    , lnodeConfig_->GetZoneType()
                    );

    // build message to replicate this node add to other nodes
    msg->type = InternalType_NodeAdd;
    msg->u.node_add.req_nid = process_->GetNid();
    msg->u.node_add.req_pid = process_->GetPid();
    msg->u.node_add.req_verifier = process_->GetVerifier();
    STRCPY( msg->u.node_add.node_name, lnodeConfig_->GetName() );
    msg->u.node_add.first_core = lnodeConfig_->GetFirstCore();
    msg->u.node_add.last_core  = lnodeConfig_->GetLastCore();
    msg->u.node_add.processors = lnodeConfig_->GetProcessors();
    msg->u.node_add.roles      = lnodeConfig_->GetZoneType();

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplNodeDelete::CReplNodeDelete(CPNodeConfig *pnodeConfig, CProcess *process) 
               : pnodeConfig_(pnodeConfig)
               , process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLB", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( node_delete_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
    {
        const char method_name[] = "CReplNodeDelete::CReplNodeDelete";
        trace_printf( "%s@%d  - Queuing node delete replication: pnid=%d, node-name=%s\n"
                    , method_name, __LINE__
                    , pnodeConfig_->GetPNid()
                    , pnodeConfig_->GetName()
                    );
    }

    // Increment reference count for process object
    process_->incrReplRef();
}

CReplNodeDelete::~CReplNodeDelete()
{
    const char method_name[] = "CReplNodeDelete::~CReplNodeDelete";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf( "%s@%d - Node delete replication for pnid=%d, node-name=%s\n"
                    , method_name, __LINE__
                    , pnodeConfig_->GetPNid()
                    , pnodeConfig_->GetName()
                    );

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplb", 4);
}

bool CReplNodeDelete::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplNodeDelete::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf( "%s@%d  - Replicating node delete (%s): pnid=%d, node-name=%s\n"
                    , method_name, __LINE__
                    , process_->GetName()
                    , pnodeConfig_->GetPNid()
                    , pnodeConfig_->GetName()
                    );

    // build message to replicate this node delete to other nodes
    msg->type = InternalType_NodeDelete;
    msg->u.node_delete.req_nid = process_->GetNid();
    msg->u.node_delete.req_pid = process_->GetPid();
    msg->u.node_delete.req_verifier = process_->GetVerifier();
    msg->u.node_delete.pnid = pnodeConfig_->GetPNid();

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplNodeDown::CReplNodeDown(int pnid) : pnid_(pnid)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLN", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( down_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplNodeDown::CReplNodeDown";
        trace_printf("%s@%d  - Queuing node down, pnid=%d\n",
                     method_name, __LINE__, pnid_);
    }
}

CReplNodeDown::~CReplNodeDown()
{
    const char method_name[] = "CReplNodeDown::~CReplNodeDown";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
        trace_printf("%s@%d - Node down replication for pnid=%d\n", method_name, __LINE__, pnid_ );

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rpln", 4);
}

bool CReplNodeDown::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplNodeDown::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d" " - Replicating node down, pnid=%d\n", method_name, __LINE__, pnid_);

    // build message to replicate this process kill to other nodes
    msg->type = InternalType_Down;
    msg->u.down.pnid = pnid_;

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

CReplNodeName::CReplNodeName( const char *current_name
                            , const char *new_name
                            , CProcess *process)
               : process_(process)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLZ", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( node_name_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplNodeName::CReplNodeName";
        trace_printf("%s@%d  - Changing node name, old name =%s, new name = %s\n",
                     method_name, __LINE__, current_name, new_name);
    }
    
    current_name_ = current_name;
    new_name_ = new_name;
    // Increment reference count for process object
    process_->incrReplRef();
}

CReplNodeName::~CReplNodeName()
{
    const char method_name[] = "CReplNodeName::~CReplNodeName";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
        trace_printf("%s@%d - Node name change replication\n", method_name, __LINE__ );

    // Decrement reference count for process object.  Then, if reference
    // count is zero and process object has been removed from list of
    // processes, delete it.
    if (process_->decrReplRef() == 0 && process_->GetState() == State_Unlinked)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process_->GetName(), process_->GetNid(), process_->GetPid() );
        delete process_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplz", 4);
}

bool CReplNodeName::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplNodeName::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d" " - Changing node name (%s to %s)\n", method_name, __LINE__, current_name_.c_str(), new_name_.c_str());

    // build message to replicate this process kill to other nodes
    msg->type = InternalType_NodeName;
    msg->u.node_name.req_nid = process_->GetNid();
    msg->u.node_name.req_pid = process_->GetPid();
    msg->u.node_name.req_verifier = process_->GetVerifier();
    strcpy (msg->u.node_name.new_name, new_name_.c_str());
    strcpy (msg->u.node_name.current_name, current_name_.c_str());

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

#ifdef EXCHANGE_CPU_SCHEDULING_DATA
CReplSchedData::CReplSchedData()
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLT", 4);

    if (trace_settings & TRACE_SYNC_DETAIL)
    {
        const char method_name[] = "CReplSchedData::CReplSchedData";
        trace_printf("%s@%d - Queueing replication data request\n",
                     method_name, __LINE__);
    }

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( scheddata_def ) + msgAlignment_)
                & ~msgAlignment_;
}

CReplSchedData::~CReplSchedData()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplt", 4);
}

bool CReplSchedData::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplSchedData::replicate";
    TRACE_ENTRY;


    if ( MyNode->GetSchedulingData() )
    {
        if (trace_settings & TRACE_SYNC_DETAIL)
            trace_printf("%s@%d - Replicating scheduling data\n",
                         method_name, __LINE__);

        // Build message to replicate scheduling data to other nodes
        msg->type = InternalType_SchedData;
        // load our node's current scheduling data
        msg->u.scheddata.PNid = MyPNID;
        msg->u.scheddata.processors = MyNode->GetNumCores();
        msg->u.scheddata.memory_free = MyNode->GetFreeMemory();
        msg->u.scheddata.swap_free = MyNode->GetFreeSwap();
        msg->u.scheddata.cache_free = MyNode->GetFreeCache();
        msg->u.scheddata.memory_total = MyNode->GetMemTotal();
        msg->u.scheddata.memory_active = MyNode->GetMemActive();
        msg->u.scheddata.memory_inactive = MyNode->GetMemInactive();
        msg->u.scheddata.memory_dirty = MyNode->GetMemDirty();
        msg->u.scheddata.memory_writeback = MyNode->GetMemWriteback();
        msg->u.scheddata.memory_VMallocUsed = MyNode->GetMemVMallocUsed();
        msg->u.scheddata.btime = MyNode->GetBTime();

        CLNode *lnode;
        lnode = MyNode->GetFirstLNode();
        int i = 0;

        for ( ; lnode; lnode = lnode->GetNext() )
        {
            msg->u.scheddata.proc_stats[i].cpu_user = lnode->GetCpuUser();
            msg->u.scheddata.proc_stats[i].cpu_nice = lnode->GetCpuNice();
            msg->u.scheddata.proc_stats[i].cpu_system = lnode->GetCpuSystem();
            msg->u.scheddata.proc_stats[i].cpu_idle = lnode->GetCpuIdle();
            msg->u.scheddata.proc_stats[i].cpu_iowait = lnode->GetCpuIowait();
            msg->u.scheddata.proc_stats[i].cpu_irq = lnode->GetCpuIrq();
            msg->u.scheddata.proc_stats[i].cpu_soft_irq = lnode->GetCpuSoftIrq();
            ++i;
        }

        // Advance sync buffer pointer
        Nodes->AddMsg( msg, replSize() );
    }

    TRACE_EXIT;

    return true;
}
#endif


CReplNodeUp::CReplNodeUp(int pnid) : pnid_(pnid)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLO", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof ( up_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
    {
        const char method_name[] = "CReplNodeUp::CReplNodeUp";
        trace_printf("%s@%d  - Queuing node up, pnid=%d\n",
                     method_name, __LINE__, pnid_);
    }
}

CReplNodeUp::~CReplNodeUp()
{
    const char method_name[] = "CReplNodeUp::~CReplNodeUp";

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL))
        trace_printf("%s@%d - Node up replication for pnid=%d\n", method_name, __LINE__, pnid_ );

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplo", 4);
}


bool CReplNodeUp::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplNodeUp::replicate";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf("%s@%d" " - Replicating node up, pnid=%d\n", method_name, __LINE__, pnid_);

    // build message to replicate this process kill to other nodes
    msg->type = InternalType_Up;
    msg->u.up.pnid = pnid_;

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}

#ifndef NAMESERVER_PROCESS
CReplStdioData::CReplStdioData(int nid, int pid, StdIoType type, ssize_t count,
                               char *data)
    : nid_(nid), pid_(pid), type_(type), count_(count)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLP", 4);

    if ( (size_t) count_ > MAX_SYNC_DATA)
    {   // Not expected to occur but guard against buffer overrun
        count_ = MAX_SYNC_DATA;
    }

    data_ = new char[count];
    if ( data_ != NULL )
    {
        memcpy(data_, data, count_);
    }

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof( ioData_t ) - MAX_SYNC_DATA + count_
                 + msgAlignment_) & ~msgAlignment_;

    if (trace_settings & TRACE_SYNC_DETAIL)
    {
        const char method_name[] = "CReplStdioData::CReplStdioData";
        trace_printf("%s@%d  - Queueing stdout data for (%d, %d), %d bytes\n",
                     method_name, __LINE__, nid_, pid_, (int) count_);
    }
}

CReplStdioData::~CReplStdioData()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplp", 4);

    delete [] data_;
}

bool CReplStdioData::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplStdioData::replicate";
    TRACE_ENTRY;

    if (trace_settings & TRACE_SYNC)
        trace_printf("%s@%d  - Replicating output data for (%d, %d), "
                     "length=%d\n", method_name, __LINE__, nid_, pid_,
                     (int) count_);

    msg->type = InternalType_IoData;
    msg->u.iodata.nid = nid_;
    msg->u.iodata.pid = pid_;
    msg->u.iodata.ioType = type_;
    msg->u.iodata.length = count_;
    memcpy(&msg->u.iodata.data, data_, count_);

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}
#endif

#ifndef NAMESERVER_PROCESS
CReplStdinReq::CReplStdinReq(int nid, int pid, StdinReqType type,
                             int supplierNid, int supplierPid) 
    : nid_(nid), pid_(pid), type_(type), supplierNid_(supplierNid),
      supplierPid_(supplierPid)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RPLQ", 4);

    // Compute message size (adjust if needed to conform to
    // internal_msg_def structure alignment).
    replSize_ = (MSG_HDR_SIZE + sizeof( stdin_req_def ) + msgAlignment_)
                & ~msgAlignment_;

    if (trace_settings & TRACE_SYNC_DETAIL)
    {
        const char method_name[] = "CReplStdinReq::CReplStdinReq";
        trace_printf("%s@%d queueing stdin request from (%d,%d), type=%d,"
                     " for supplier (%d, %d)\n",
                     method_name, __LINE__, nid_, pid_, type_, supplierNid_,
                     supplierPid_);

    }
}

CReplStdinReq::~CReplStdinReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rplq", 4);
}

bool CReplStdinReq::replicate(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplStdinReq::replicate";
    TRACE_ENTRY;

    // build message to replicate this request to other nodes
    msg->type = InternalType_StdinReq;
    msg->u.stdin_req.nid = nid_;
    msg->u.stdin_req.pid = pid_;
    msg->u.stdin_req.reqType = type_;
    msg->u.stdin_req.supplier_nid = supplierNid_;
    msg->u.stdin_req.supplier_pid = supplierPid_;

    if (trace_settings & TRACE_SYNC)
    {
        trace_printf("%s@%d forwarding stdin request from (%d, %d)"
                     ", type=%d, for supplier (%d, %d)\n",
                     method_name, __LINE__, nid_, pid_,
                     type_, supplierNid_, supplierPid_);
    }

    // Advance sync buffer pointer
    Nodes->AddMsg( msg, replSize() );

    TRACE_EXIT;

    return true;
}
#endif

CReplicate::CReplicate(): 
    maxListSize_(0), syncClusterData_ (false)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REPL", 4);
}

CReplicate::~CReplicate()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "repl", 4);
}

// Return true if need to replicate data to other cluster nodes or
// if need data from other nodes.  Increment counter indicating why
// replication occurred.
bool CReplicate::haveSyncData()
{
    bool result = false; 

    replListLock_.lock();
    if (!replList_.empty())
    {
        result = true;
    }
    replListLock_.unlock();

    if (syncClusterData_)
    {
        result = true;
        syncClusterData_ = false;
    }

    return result;
}


// Go through the replication list and copy data items to the sync
// buffer.   Do the entire list unless we run out of sync buffer space.
void CReplicate::FillSyncBuffer(struct internal_msg_def *&msg)
{
    const char method_name[] = "CReplicate::FillSyncBuffer";
    CReplObj *replItem;
    int itemSize;

    typedef list<CReplObj *> delReplList_t;
    delReplList_t delReplList;

    replListLock_.lock();

    replList_t::iterator it;

    it = replList_.begin();

    while (msg != NULL && it != replList_.end())
    {
        // Get item from list
        replItem = *it;

        // bugcatcher, temp call
        replItem->validateObj();

        // Get item's replication size
        itemSize = replItem->replSize();

        if ( Nodes->SpaceAvail ( itemSize ) )
        {  // There is space available for this item in the sync buffer

            // Copy data to be replicated into sync buffer
            if (replItem->replicate (msg))
            {
                // Take item out of list, "it" will point to following item
                it = replList_.erase (it);

                // Add the item to the list of items to be deleted
                delReplList.push_back( replItem );
 
                // Record statistics (sonar counters)
                if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                   MonStats->ObjsReplicatedIncr();
            }
            else
            {   // Could not replicate so advance to next item
                ++it;
            }
        }
        else
        {
            // SyncBuffer is full 

            // temp trace
            if (trace_settings & TRACE_SYNC)
                trace_printf("%s@%d - Sync buffer is full\n", method_name,
                             __LINE__);

            // Indicate to caller that sync buffer is full
            msg = NULL;
        }
    }


    replListLock_.unlock();

    // Now delete the replication objects whose data has been copied
    // to the sync buffer.  We do this outside the "replListLock"
    // region since additional locks may need to be acquired during
    // the deletion and we must avoid deadlock.
    delReplList_t::iterator it2 = delReplList.begin();

    while (it2 != delReplList.end())
    {
        // Get item from list
        replItem = *it2;

        // Take item out of list, "it2" will point to following item
        it2 = delReplList.erase (it2);

        delete replItem;
    }
}


void CReplicate::addItem(CReplObj *replItem)
{
    // Add item to replicate list

    replListLock_.lock();

    
    replList_.push_back (replItem);

    int listSize = replList_.size();
    if (listSize > maxListSize_)
        maxListSize_ = listSize;

    replListLock_.unlock();
}

void CReplicate::stats()
{
    const char method_name[] = "CReplicate::stats";

    if (trace_settings & (TRACE_SYNC | TRACE_STATS))
        trace_printf("%s@%d maxListSize=%d\n", method_name, __LINE__,
                     maxListSize_);

}

