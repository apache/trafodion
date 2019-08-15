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

#include <stddef.h>
#include <stdio.h>
#include <zlib.h>
#include "reqqueue.h"
#include "monitor.h"
#include "monlogging.h"
#include "mlio.h"
#include "montrace.h"
#include "monsonar.h"
#include "clusterconf.h"
#include "nameserverconfig.h"
#include "lock.h"
#include "lnode.h"
#include "pnode.h"
#include "replicate.h"
#include "internal.h"
#include "healthcheck.h"
#ifndef NAMESERVER_PROCESS
#include "redirector.h"
#include "nameserver.h"
#include "ptpclient.h"
#include "zclient.h"
#endif

extern int MyPNID;
extern bool Emulate_Down;
extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
extern CNode *MyNode;
extern CMonStats *MonStats;
extern CLock MemModLock;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;
extern CReqQueue ReqQueue;
extern CConfigContainer *Config;
extern CHealthCheck HealthCheck;
#ifdef NAMESERVER_PROCESS
extern char *ErrorMsg (int error_code);
#else
extern CRedirector Redirector;
extern bool NameServerEnabled;
extern CPtpClient *PtpClient;
extern CProcess *NameServerProcess;
extern CNameServer *NameServer;
extern CNameServerConfigContainer *NameServerConfig;
extern CZClient* ZClient;
#endif

extern int req_type_startup;

extern bool IAmIntegrating;
extern bool IAmIntegrated;
extern bool IsRealCluster;
extern bool IsAgentMode;
extern bool IsMaster;
extern bool ZClientEnabled;

extern CommType_t CommType;
extern bool IsRealCluster;

CReqResource::CReqResource()
{
}

CReqResource::~CReqResource()
{
}

CReqResourceProc::CReqResourceProc( int nid
                                  , int pid
                                  , const char *name
                                  , Verifier_t verifier )
                 : nid_(nid)
                 , pid_(pid)
                 , verifier_(verifier)
                 , processName_(name)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RESA", 4);
}

CReqResourceProc::~CReqResourceProc()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "resa", 4);
}

CReqResource::ResourceStatus_t CReqResourceProc::acquireResource( long requestId )
{
    const char method_name[] = "CReqResourceProc::acquireResource";
    TRACE_ENTRY;

    CProcess *process = NULL;
    if ( nid_ == -1 || pid_ == -1 )
    { // find by name (check node state, don't check process state, not backup)
        process = Nodes->GetProcess( processName_.c_str()
                                   , verifier_, true, false, false );
    }
    else
    { // find by nid (check node state, don't check process state, backup is Ok)
        process = Nodes->GetProcess( nid_
                                   , pid_
                                   , verifier_, true, false, true );
    }

    if ( process == NULL )
    {  // Process no longer exists
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf("%s@%d request #%ld, process (%d, %d) no longer "
                         "exists\n",
                         method_name, __LINE__, requestId, nid_, pid_);
        }

        TRACE_EXIT;
        return UnAvailable;
    }

    if ( process->GetState() != State_Up)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf("%s@%d request #%ld could not acquire ownership of "
                         "process %s (%d, %d), process state not State_Up\n",
                         method_name, __LINE__, requestId, process->GetName(),
                         process->GetNid(), process->GetPid());
        }

        TRACE_EXIT;
        return NotUp;
    }

    if ( !process->isOwned() )
    {
        process->setOwner( requestId );
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d request #%ld obtained ownership of process "
                         "%s (%d, %d)\n",
                         method_name, __LINE__, requestId, process->GetName(),
                         process->GetNid(), process->GetPid());
        TRACE_EXIT;
        return Acquired;
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d request #%ld could not acquire ownership of "
                         "process %s (%d, %d), resource busy.\n",
                         method_name, __LINE__, requestId, process->GetName(),
                         process->GetNid(), process->GetPid());

        TRACE_EXIT;
        return Busy;
    }
}

void CReqResourceProc::releaseResource()
{
    const char method_name[] = "CReqResourceProc::releaseResource";
    TRACE_ENTRY;

    CProcess *process = Nodes->GetProcess( nid_, pid_ );

    if ( process )
    {
        // Indicate that the process object is no longer owned.
        process->resetOwned();

        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d released ownership of process %s (%d, %d)\n",
                         method_name, __LINE__, process->GetName(),
                         process->GetNid(), process->GetPid());
    }

    TRACE_EXIT;
}

CProcess* CReqResourceProc::getProcess()
{
    return( Nodes->GetProcess( nid_, pid_ ) );
}

CReqResourceConfig::CReqResourceConfig(CConfigGroup *config): config_(config)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RESB", 4);
}

CReqResourceConfig::~CReqResourceConfig()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "resb", 4);
}


CReqResource::ResourceStatus_t CReqResourceConfig::acquireResource( long  )
{
    return UnAvailable;
}

void CReqResourceConfig::releaseResource()
{
}

CRequest::CRequest(): concurrent_(false), id_(0), numResources_(0)
{
    clock_gettime(CLOCK_REALTIME, &reqArrival_);
    reqStart_.tv_sec = 0;
    reqStart_.tv_nsec = 0;
    execTimeMax_ = CReqQueue::REQ_MAX_RESPONSIVE + MyNode->GetWDTKeepAliveTimerValue();
    priority_ = Normal;
}

CRequest::~CRequest()
{
    const char method_name[] = "CRequest::~CRequest";
    TRACE_ENTRY;

    for (int i=0; i<numResources_; i++)
    {
        delete resources_[i];
    }

    if ( trace_settings & TRACE_REQUEST )
        trace_printf("%s@%d - request #%ld\n", method_name, __LINE__, id_);
    TRACE_EXIT;
}

void CRequest::addResource(CReqResource * resource)
{
    if (numResources_ < MAX_RESOURCES)
    {
        resources_[numResources_] = resource;
        ++numResources_;
    }
}



CRequest::ReqStatus_t CRequest::okToExecute()
{
    const char method_name[] = "CRequest::okToExecute";
    TRACE_ENTRY;

    ReqStatus_t reqStatus = OkToExec;

    // do any request preparation
    if (!prepare())
    {   // Some problem discovered during preparation.
        // Error reply has been sent to requester by prepare().
        TRACE_EXIT;

        return Failed;
    }

    bool ownershipFailure;
    if ( !isExclusive()
      && !takeOwnership( ownershipFailure ) )
    {   // Can't get ownership of needed objects

        if (ownershipFailure)
        {   // Not possible to get required ownership to complete
            // the request so request fails.

            if ( trace_settings & TRACE_REQUEST_DETAIL )
                trace_printf("%s@%d ownership failure for request "
                             "#%ld, replying with error\n",
                             method_name, __LINE__, getId());

            // Send failure response
            errorReply( MPI_ERR_NAME );

            reqStatus = Failed;
        }
        else
        {
            // Check for request failure due to timeout
            struct timespec now;
            struct timespec tDiff;
            clock_gettime(CLOCK_REALTIME, &now);

            CReqQueue::timeDiff ( reqArrival_, now, tDiff );

            if ( tDiff.tv_sec > CReqQueue::REQ_MAX_DEFER)
            {   // timed out
                populateRequestString();
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf), "[%s] request #%ld timed out "
                         "attempting to gain ownership of needed resources "
                         "(%s)\n", method_name, getId(),
                         requestString_.c_str());
                mon_log_write(MON_REQQUEUE_REQUEST_1, SQ_LOG_ERR, buf);

                // Send failure response
                errorReply( MPI_ERR_OTHER );

                reqStatus = Failed;
            }
            else
            {   // Could not get ownership, will wait until later
                if ( trace_settings & TRACE_REQUEST )
                    trace_printf("%s@%d cannot take ownership for request "
                                 "#%ld\n", method_name, __LINE__, getId());

                reqStatus = WaitToExec;
            }
        }
    }
    else
    {
        clock_gettime(CLOCK_REALTIME, &reqStart_);
    }


    TRACE_EXIT;

    return reqStatus;
}


void CRequest::evalReqPerformance( void )
{
    const char method_name[] = "CRequest::evalReqPerformance";

    // Log info if request took a long time
    struct timespec curTime;
    clock_gettime(CLOCK_REALTIME, &curTime);

    struct timespec queuedTime;
    struct timespec performTime;
    struct timespec totalTime;

    CReqQueue::timeDiff ( reqArrival_, reqStart_, queuedTime );
    CReqQueue::timeDiff ( reqStart_,   curTime,   performTime );
    CReqQueue::timeDiff ( reqArrival_, curTime,   totalTime );

    // temp trace
    if ( trace_settings & TRACE_REQUEST )
    {
        trace_printf("%s@%d request #%ld, arrival-to-start=%ld.%06ld, "
                     "start-to-complete=%ld.%06ld, total=%ld.%06ld\n",
                     method_name, __LINE__, getId(),
                     queuedTime.tv_sec, queuedTime.tv_nsec / 1000,
                     performTime.tv_sec, performTime.tv_nsec / 1000,
                     totalTime.tv_sec, totalTime.tv_nsec / 1000);
    }
    // end trace

    if ( performTime.tv_sec > CReqQueue::REQ_MAX_PERFORM )
    {
        char buf[MON_STRING_BUF_SIZE];
        populateRequestString();
        sprintf(buf, "[%s], Lengthy request: perform "
                "time=%ld.%06ld, total time=%ld.%06ld {%s}\n", method_name,
                performTime.tv_sec, performTime.tv_nsec,
                totalTime.tv_sec, totalTime.tv_nsec, requestString());
        mon_log_write(MON_REQ_EVALREQ_PERFORMANCE_1, SQ_LOG_ERR, buf);
    }
}

#ifndef NAMESERVER_PROCESS
// Sending reply to request from local io client
void CRequest::lioreply(struct message_def *msg, int Pid, int *error)
{
    const char method_name[] = "CRequest::lioreply";
    TRACE_ENTRY;

    if (msg->noreply) // tell client to release buffer
    {
        if (trace_settings & (TRACE_MLIO_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d request type = %d\n", method_name, __LINE__,
                         msg->u.request.type);
        if (msg->u.request.type != ReqType_NewProcess &&
            msg->u.request.type != ReqType_Dump &&
            msg->u.request.type != ReqType_Close)
        {
            if (trace_settings & (TRACE_MLIO_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d" " sending readysend ctl msg=%p, pid=%d, "
                             "idx=%d\n", method_name, __LINE__, msg, Pid,
                             ((SharedMsgDef*)msg)->trailer.index );

            SQ_theLocalIOToClient->sendCtlMsg ( Pid,
                                                MC_ReadySend,
                                                ((SharedMsgDef*)msg)->
                                                trailer.index,
                                                error
                                              );
        }
        else
        {
            if (trace_settings & (TRACE_MLIO_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d waiting for cluster generated reply\n",
                             method_name, __LINE__);
        }
    }
    else // tell client they have message.
    {
        if (trace_settings & TRACE_PROCESS)
            //        if (trace_settings & (TRACE_MLIO_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d sending reply ctl msg=%p, pid=%d, idx=%d\n",
                         method_name, __LINE__, msg, Pid,
                         ((SharedMsgDef*)msg)->trailer.index );

        SQ_theLocalIOToClient->sendCtlMsg ( Pid,
                                            MC_SReady,
                                            ((SharedMsgDef*)msg)->trailer.index,
                                            error
                                          );
    }

    TRACE_EXIT;
}
#endif

void CExternalReq::validateObj( void )
{
    if ((strncmp((const char *)&eyecatcher_, "RQE", 3) !=0 ) &&
        (strncmp((const char *)&eyecatcher_, "RqE", 3) !=0 ))
    {  // Not a valid object
        abort();
    }
}

void CExternalReq::errorReply( int rc )
{
    msg_->u.reply.type = ReplyType_Generic;
    msg_->u.reply.u.generic.nid = -1;
    msg_->u.reply.u.generic.pid = -1;
    msg_->u.reply.u.generic.verifier = -1;
    msg_->u.reply.u.generic.process_name[0] = '\0';
    msg_->u.reply.u.generic.return_code = rc;

#ifdef NAMESERVER_PROCESS
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        const char method_name[] = "CExternalReq::errorReply";
        trace_printf( "%s@%d rc=%d\n", method_name, __LINE__, rc );
    }
    monreply(msg_, sockFd_);
#else
    // Send reply to requester
    lioreply(msg_, pid_);
#endif
}



// Attempt to take ownership of all resources needed for this request.
bool CExternalReq::takeOwnership( bool & ownershipFailure )
{
    const char method_name[] = "CExternalReq::takeOwnership";
    bool haveOwnership = true;
    CReqResource::ResourceStatus_t resourceStatus;
    ownershipFailure = false;

    TRACE_ENTRY;

    if ( (trace_settings & TRACE_REQUEST) && ( numResources_ != 0 ) )
    {
        trace_printf("%s@%d request #%ld, # resources=%d\n", method_name,
                     __LINE__, id_, numResources_);
    }

    for (int i=0; i<numResources_; i++)
    {
        resourceStatus = resources_[i]->acquireResource( id_ );
        if ( resourceStatus != CReqResource::Acquired )
        {   // Could not obtain the resource, release already obtained
            // resources.
            for (int j=0; j<i; j++)
            {
                resources_[j]->releaseResource();
            }
            haveOwnership = false;

            if ( resourceStatus == CReqResource::UnAvailable )
            {
                ownershipFailure = true;
            }
            break;
        }
    }

    TRACE_EXIT;
    return haveOwnership;
}

// Release any resources acquired for executing the request.
void CExternalReq::giveupOwnership()
{
    const char method_name[] = "CExternalReq::giveupOwnership";
    TRACE_ENTRY;

    if ( (trace_settings & TRACE_REQUEST) && ( numResources_ != 0 ) )
    {
        trace_printf("%s@%d request #%ld, # resources=%d\n", method_name,
                     __LINE__, id_, numResources_);
    }

    for (int i=0; i<numResources_; i++)
    {
        resources_[i]->releaseResource();
    }
    TRACE_EXIT;
}

CExtNullReq::CExtNullReq (reqQueueMsg_t msgType, int nid, int pid, int sockFd,
                          struct message_def *msg )
    : CExternalReq(msgType, nid, pid, sockFd, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQE_", 4);
}

CExtNullReq::~CExtNullReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqe_", 4);
}

CInternalReq::CInternalReq()
    : seqNum_(0),
      reviveFlag_(0) // will be set by requests that are needed to perform revive
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REQI", 4);
}

CInternalReq::~CInternalReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "reqi", 4);
}

void CInternalReq::validateObj( void )
{
    if ((strncmp((const char *)&eyecatcher_, "RQI", 3) !=0 ) &&
        (strncmp((const char *)&eyecatcher_, "RqI", 3) !=0 ))
    {  // Not a valid object
        abort();
    }
}

void CInternalReq::giveupOwnership()
{
}

void CInternalReq::populateRequestString( void )
{
}

void CInternalReq::performRequest()
{
    const char method_name[] = "CInternalReq::performRequest";
    TRACE_ENTRY;

    // Trace info about request
    if (trace_settings & TRACE_REQUEST )
        trace_printf("%s@%d \n", method_name, __LINE__);

    TRACE_EXIT;
}

void CInternalReq::errorReply( int  )
{
}

#ifndef NAMESERVER_PROCESS
CIntCloneProcReq::CIntCloneProcReq( bool backup, bool unhooked, bool eventMessages, bool systemMessages, int nid, PROCESSTYPE type, int priority, int parentNid, int parentPid, int parentVerifier, int osPid, int verifier, pid_t priorPid, int persistentRetries, int  argc, struct timespec creationTime, strId_t pathStrId, strId_t ldpathStrId, strId_t programStrId, int nameLen, int portLen, int infileLen, int outfileLen, int argvLen, const char * stringData, int origPNidNs)
    : CInternalReq(),
      backup_( backup ),
      unhooked_( unhooked ),
      eventMessages_ ( eventMessages ),
      systemMessages_( systemMessages ),
      nid_ ( nid ),
      type_( type ),
      priority_( priority ),
      parentNid_( parentNid ),
      parentPid_( parentPid ),
      parentVerifier_( parentVerifier ),
      osPid_( osPid ),
      verifier_( verifier ),
      priorPid_( priorPid ),
      persistentRetries_ ( persistentRetries ),
      argc_( argc ),
      pathStrId_ ( pathStrId ),
      ldpathStrId_ ( ldpathStrId ),
      programStrId_ ( programStrId ),
      nameLen_ ( nameLen ),
      portLen_ ( portLen ),
      infileLen_ ( infileLen ),
      outfileLen_ ( outfileLen ),
      argvLen_ ( argvLen ),
      origPNidNs_ ( origPNidNs )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIL", 4);

    creationTime_.tv_sec = creationTime.tv_sec;
    creationTime_.tv_nsec = creationTime.tv_nsec;

    int stringDataLen = nameLen_ + portLen_+ infileLen_ + outfileLen_
                      + argvLen_;
    stringData_ = new char [stringDataLen];
    memcpy ( stringData_, stringData, stringDataLen );
}

void CIntCloneProcReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (name=%s/nid=%d/pid=%d) parent(nid=%d/pid=%d)"
                   , CReqQueue::intReqType[InternalType_Clone]
                   , getId(),
             &stringData_[0], // process name
             nid_, osPid_, parentNid_, parentPid_ );
    requestString_.assign( strBuf );
}

CIntCloneProcReq::~CIntCloneProcReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqil", 4);

    delete [] stringData_;
}

void CIntCloneProcReq::performRequest()
{
    const char method_name[] = "CIntCloneProcReq::performRequest";
    TRACE_ENTRY;

    CProcess *process;
    CProcess *parent;

    // Trace info about request
    if (trace_settings & TRACE_REQUEST )
        trace_printf("%s@%d \n", method_name, __LINE__);

    if ( priorPid_ != 0 )
    {  // The "clone" represents a restarted persistent process
        process = Nodes->GetProcess( nid_, priorPid_, false );
        if (process)
        {
            parent = Nodes->GetProcess( process->GetParentNid(),
                                        process->GetParentPid() );
            // Handle prior process termination
            process->Exit( parent );

            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST
                                  | TRACE_PROCESS))
            {
                trace_printf("%s@%d - For restarted persistent process "
                             " altering (%d, %d) to (%d, %d)\n",
                             method_name, __LINE__,
                             nid_,
                             priorPid_,
                             nid_,
                             osPid_ );
            }

            CNode * node = Nodes->GetLNode (process->GetNid())->GetNode();
            node->DelFromPidMap ( process );
            process->SetVerifier(verifier_);
            process->SetParentVerifier(parentVerifier_);
            process->CompleteProcessStartup (&stringData_[nameLen_],
                                             osPid_,
                                             eventMessages_,
                                             systemMessages_,
                                             false,
                                             &creationTime_,
                                             origPNidNs_);
            node->AddToNameMap ( process );
            node->AddToPidMap ( osPid_, process );
        }
        else
        {  // Unexpectedly could not find process object
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d - Unexpectedly could not locate "
                             " restarted persistent process (%d , %d)\n",
                             method_name, __LINE__,
                             nid_,
                             priorPid_);
            }
        }
    }
    else
    {
        // check to see if we pre-cloned this process for startup in a remote node
        process = Nodes->GetLNode (nid_)->
            CompleteProcessStartup(&stringData_[0], // process name
                                   &stringData_[nameLen_],  // port,
                                   osPid_,
                                   eventMessages_,
                                   systemMessages_,
                                   &creationTime_,
                                   origPNidNs_);
        if (process)
        {
            if (trace_settings & TRACE_SYNC)
            {
                trace_printf("%s@%d - Created process %s (%d, %d), started "
                             "on port %s\n", method_name, __LINE__,
                             process->GetName(), process->GetNid(),
                             process->GetPid(), process->GetPort());
            }

            process->SetVerifier(verifier_);
            process->SetParentVerifier(parentVerifier_);

            // Send reply or notice to parent process if necessary.
            int result = ( process->GetPid() != -1 ) ? 0 : MPI_ERR_SPAWN;
            parent = CProcessContainer::ParentNewProcReply( process, result );
            if ( result )
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf),
                         "[%s], Process %s (%d, %d) did not "
                         "startup successfully.\n",
                         method_name, process->GetName(), process->GetNid(),
                         process->GetPid());
                mon_log_write(MON_INTREQ_CLONEPROC_1, SQ_LOG_ERR, buf);
            }

            if (parent && process->IsBackup())
            {
                if (trace_settings & TRACE_SYNC)
                    trace_printf("%s@%d - For backup process (%d, %d), for"
                                 " parent (%d, %d) setting parent's Parent"
                                 "_Nid/Parent_Pid=(%d, %d).\n",
                                 method_name, __LINE__,  process->GetNid(),
                                 process->GetPid(), parent->GetNid(), parent->GetPid(),
                                 process->GetNid(), process->GetPid());

                parent->SetParentNid ( process->GetNid() );
                parent->SetParentPid ( process->GetPid() );
                parent->SetParent( process );
            }

            // There might be a request waiting for the process creation to
            // complete so have worker check pending request queue.
            ReqQueue.nudgeWorker();
        }
        else
        {
            // This is a new clone process that needs to be created
            // mirroring another node
            CNode * node = Nodes->GetLNode (nid_)->GetNode();
            CProcess * process;
            process = node->CloneProcess (nid_,
                                type_,
                                priority_,
                                backup_,
                                unhooked_,
                                &stringData_[0], // process name
                                &stringData_[nameLen_],  // port
                                osPid_,
                                verifier_,
                                parentNid_,
                                parentPid_,
                                parentVerifier_,
                                eventMessages_,
                                systemMessages_,
                                pathStrId_,
                                ldpathStrId_,
                                programStrId_,
                                &stringData_[nameLen_ + portLen_],  // infile
                                &stringData_[nameLen_ + portLen_ + infileLen_], // outfile
                                &creationTime_,
                                origPNidNs_);
            if ( process )
            {
                process->userArgs ( argc_, argvLen_,
                                    &stringData_[nameLen_ + portLen_
                                                 +infileLen_ + outfileLen_] );
            }
        }
    }

    TRACE_EXIT;
}
#endif


#ifdef NAMESERVER_PROCESS
CIntCloneProcNsReq::CIntCloneProcNsReq( bool backup
                                      , bool unhooked
                                      , bool eventMessages
                                      , bool systemMessages
                                      , int nid
                                      , PROCESSTYPE type
                                      , int priority
                                      , int parentNid
                                      , int parentPid
                                      , int parentVerifier
                                      , int osPid
                                      , int verifier
                                      , pid_t priorPid
                                      , int persistentRetries
                                      , int  argc
                                      , struct timespec creationTime
                                      , int pathLen
                                      , int ldpathLen
                                      , int programLen
//                                      , strId_t pathStrId
//                                      , strId_t ldpathStrId
//                                      , strId_t programStrId
                                      , int nameLen
                                      , int portLen
                                      , int infileLen
                                      , int outfileLen
                                      , int argvLen
                                      , const char * stringData
                                      , int origPNidNs)
                   : CInternalReq()
                   , backup_( backup )
                   , unhooked_( unhooked )
                   , eventMessages_( eventMessages )
                   , systemMessages_( systemMessages )
                   , nid_( nid )
                   , type_( type )
                   , priority_( priority )
                   , parentNid_( parentNid )
                   , parentPid_( parentPid )
                   , parentVerifier_( parentVerifier )
                   , osPid_( osPid )
                   , verifier_( verifier )
                   , priorPid_( priorPid )
                   , persistentRetries_( persistentRetries )
                   , argc_( argc )
                   , pathLen_( pathLen )
                   , ldpathLen_( ldpathLen )
                   , programLen_( programLen )
//                   , pathStrId_( pathStrId )
//                   , ldpathStrId_( ldpathStrId )
//                   , programStrId_( programStrId )
                   , nameLen_( nameLen )
                   , portLen_( portLen )
                   , infileLen_( infileLen )
                   , outfileLen_( outfileLen )
                   , argvLen_( argvLen )
                   , origPNidNs_( origPNidNs )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIL", 4);

    creationTime_.tv_sec = creationTime.tv_sec;
    creationTime_.tv_nsec = creationTime.tv_nsec;

    int stringDataLen = nameLen_ + portLen_+ infileLen_ + outfileLen_
                      + pathLen_ + ldpathLen_ + programLen_
                      + argvLen_;
    stringData_ = new char [stringDataLen];
    memcpy ( stringData_, stringData, stringDataLen );
}

void CIntCloneProcNsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (name=%s/nid=%d/pid=%d) parent(nid=%d/pid=%d)"
                   , CReqQueue::intReqType[InternalType_Clone]
                   , getId(),
             &stringData_[0], // process name
             nid_, osPid_, parentNid_, parentPid_ );
    requestString_.assign( strBuf );
}

CIntCloneProcNsReq::~CIntCloneProcNsReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqil", 4);

    delete [] stringData_;
}

void CIntCloneProcNsReq::performRequest()
{
    const char method_name[] = "CIntCloneProcNsReq::performRequest";
    TRACE_ENTRY;

    CProcess *process;

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        trace_printf("%s@%d \n", method_name, __LINE__);

    // check to see if we pre-cloned this process for startup in a remote node
    process = Nodes->GetLNode (nid_)->
        CompleteProcessStartup(&stringData_[0], // process name
                               &stringData_[nameLen_],  // port,
                               osPid_,
                               eventMessages_,
                               systemMessages_,
                               &creationTime_,
                               origPNidNs_);
    if (process)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf("%s@%d - Registered process %s (%d, %d:%d), "
                         "port=%s, origPNidNs=%d, MyPNID=%d\n", method_name, __LINE__,
                         process->GetName(), process->GetNid(),
                         process->GetPid(), process->GetVerifier(),
                         process->GetPort(), process->GetOrigPNidNs(), MyPNID );
        }

        process->SetVerifier(verifier_);
        process->SetParentVerifier(parentVerifier_);

        // Send reply to originating monitor
        if ( MyPNID == process->GetOrigPNidNs() )
        {
            struct message_def *msg = process->GetMonContext();
            msg->u.reply.type = ReplyType_NewProcessNs;
            msg->noreply = false;
            msg->reply_tag = process->GetReplyTag();
            msg->u.reply.u.new_process_ns.nid = process->GetNid();
            msg->u.reply.u.new_process_ns.pid = process->GetPid();
            msg->u.reply.u.new_process_ns.verifier = process->GetReplyTag();
            strncpy(msg->u.reply.u.new_process_ns.process_name, process->GetName(), MAX_PROCESS_NAME);
            msg->u.reply.u.new_process_ns.return_code = MPI_SUCCESS;

            int sockFd = process->GetMonSockFd();
            // Send reply to requester
            monreply( msg, sockFd );
        }
        
        // There might be a request waiting for the process creation to
        // complete so have worker check pending request queue.
        ReqQueue.nudgeWorker();
    }
    else
    {
        // This is a new clone process that needs to be created 
        // mirroring another node
        CNode * node = Nodes->GetLNode(nid_)->GetNode();
        CProcess * process;
        process = node->CloneProcess( nid_,
                            type_,
                            priority_,
                            backup_,
                            unhooked_,
                            &stringData_[0], // process name
                            &stringData_[nameLen_],  // port
                            osPid_,
                            verifier_,
                            parentNid_,
                            parentPid_,
                            parentVerifier_,
                            eventMessages_,
                            systemMessages_,
//                            pathStrId_,
//                            ldpathStrId_,
//                            programStrId_,
                            &stringData_[nameLen_ + portLen_ + infileLen_ + outfileLen_],  // path
                            &stringData_[nameLen_ + portLen_ + infileLen_ + outfileLen_ + pathLen_],  // ldpath
                            &stringData_[nameLen_ + portLen_ + infileLen_ + outfileLen_ + pathLen_ + ldpathLen_],  // program
                            &stringData_[nameLen_ + portLen_],  // infile
                            &stringData_[nameLen_ + portLen_ + infileLen_], // outfile
                            &creationTime_,
                            origPNidNs_);
        if ( process )
        {
            process->userArgs( argc_
                             , argvLen_
                             , &stringData_[ nameLen_ 
                                          + portLen_
                                          + infileLen_ 
                                          + outfileLen_
                                          + pathLen_ 
                                          + ldpathLen_
                                          + programLen_] );
        }
    }

    TRACE_EXIT;
}
#endif


#ifndef NAMESERVER_PROCESS
CIntDeviceReq::CIntDeviceReq( char *ldevName )
    : CInternalReq()
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQID", 4);

    STRCPY( ldevName_, ldevName );
}

CIntDeviceReq::~CIntDeviceReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqid", 4);
}

void CIntDeviceReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (ldevname=%s)"
                   , CReqQueue::intReqType[InternalType_Device]
                   , getId(), ldevName_ );
    requestString_.assign( strBuf );
}

void CIntDeviceReq::performRequest()
{
    const char method_name[] = "CIntDeviceReq::performRequest";
    TRACE_ENTRY;

    // Trace info about request
    if (trace_settings & TRACE_REQUEST )
        trace_printf("%s@%d device=%s\n", method_name, __LINE__, ldevName_);

    Monitor->DoDeviceReq( ldevName_ );

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntDumpCompleteReq::CIntDumpCompleteReq()
           : CInternalReq()
           , nid_(0)
           , pid_(0)
           , verifier_(-1)
           , dumperNid_(0)
           , dumperPid_(0)
           , dumperVerifier_(-1)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqIC", 4);

    coreFile_[0] = '\0';
}

CIntDumpCompleteReq::~CIntDumpCompleteReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQic", 4);
}

void CIntDumpCompleteReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (nid=%d/pid=%d/verifier=%d)"
                   , CReqQueue::intReqType[InternalType_DumpComplete]
                   , getId(), nid_, pid_, verifier_ );
    requestString_.assign( strBuf );
}

void CIntDumpCompleteReq::prepRequest( struct dump_def *dumpDef )
{
    const char method_name[] = "CIntDumpCompleteReq::prepRequest";
    TRACE_ENTRY;

    nid_ = dumpDef->nid;
    pid_ = dumpDef->pid;
    verifier_ = dumpDef->verifier;
    dumperNid_ = dumpDef->dumper_nid;
    dumperPid_ = dumpDef->dumper_pid;
    dumperVerifier_ = dumpDef->dumper_verifier;
    strcpy( coreFile_, dumpDef->core_file );
    status_ = dumpDef->status;

    TRACE_EXIT;
}

void CIntDumpCompleteReq::performRequest()
{
    const char method_name[] = "CIntDumpCompleteReq::performRequest";
    TRACE_ENTRY;

    CProcess* process = NULL;
    CLNode*   lnode;

    lnode = Nodes->GetLNode( nid_ );
    if ( lnode )
    {
        process = lnode->GetProcessL( pid_ );

        if (process)
        {
            int verifier = verifier_;
            if ( (verifier == -1) || (verifier == process->GetVerifier()) )
            {
                process->DumpEnd( status_, coreFile_ );
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Can't find process nid=%d, "
                          "pid=%d, verifier=%d for dump complete target.\n"
                        , method_name
                        , nid_
                        , pid_
                        , verifier_ );
                mon_log_write(MON_INTREQ_DUMPCOMPLETE_1, SQ_LOG_ERR, buf);
            }
        }
        else
        {
            // Dump completion handled in CProcess::Exit()
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Can't find process nid=%d, "
                      "pid=%d for dump complete target.\n"
                    , method_name
                    , nid_
                    , pid_ );
            mon_log_write(MON_INTREQ_DUMPCOMPLETE_2, SQ_LOG_ERR, buf);
        }
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntDumpReq::CIntDumpReq()
           : CInternalReq()
           , nid_(0)
           , pid_(0)
           , verifier_(-1)
           , dumperNid_(0)
           , dumperPid_(0)
           , dumperVerifier_(-1)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqID", 4);

    coreFile_[0] = '\0';
}

CIntDumpReq::~CIntDumpReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQid", 4);
}

void CIntDumpReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (nid=%d/pid=%d/verifier=%d)"
                   , CReqQueue::intReqType[InternalType_Dump]
                   , getId(), nid_, pid_, verifier_ );
    requestString_.assign( strBuf );
}

void CIntDumpReq::prepRequest( struct dump_def *dumpDef )
{
    const char method_name[] = "CIntDumpReq::prepRequest";
    TRACE_ENTRY;

    nid_ = dumpDef->nid;
    pid_ = dumpDef->pid;
    verifier_ = dumpDef->verifier;
    dumperNid_ = dumpDef->dumper_nid;
    dumperPid_ = dumpDef->dumper_pid;
    dumperVerifier_ = dumpDef->dumper_verifier;
    strcpy( coreFile_, dumpDef->core_file );

    TRACE_EXIT;
}

void CIntDumpReq::performRequest()
{
    const char method_name[] = "CIntDumpReq::performRequest";
    TRACE_ENTRY;

    CProcess* process = NULL;
    CLNode*   lnode;

    lnode = Nodes->GetLNode( nid_ );
    if ( lnode )
    {
        process = lnode->GetProcessL( pid_ );

        if (process)
        {
            int verifier = verifier_;
            if ( (verifier == -1) || (verifier == process->GetVerifier()) )
            {
                process->DumpBegin( dumperNid_
                                  , dumperPid_
                                  , dumperVerifier_
                                  , coreFile_ );
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Can't find process nid=%d, "
                          "pid=%d, verifier=%d for dump target.\n"
                        , method_name
                        , nid_
                        , pid_
                        , verifier_ );
                mon_log_write(MON_INTREQ_DUMP_1, SQ_LOG_ERR, buf);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Can't find process nid=%d, "
                      "pid=%d for dump target.\n"
                    , method_name
                    , nid_
                    , pid_ );
            mon_log_write(MON_INTREQ_DUMP_2, SQ_LOG_ERR, buf);
        }
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntEventReq::CIntEventReq()
            : CInternalReq()
            , eventId_(0)
            , length_(0)
            , targetNid_(-1)
            , targetPid_(-1)
            , targetVerifier_(-1)
            , bigData_(NULL)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqIE", 4);

    data_[0] = '\0';
}

CIntEventReq::~CIntEventReq()
{
    if (length_ > SMALL_DATA_SIZE)
    {
        ::delete[] bigData_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQiE", 4);
}

void * CIntEventReq::operator new(size_t size)
{
    if (trace_settings & TRACE_SYNC)
    {
        const char method_name[] = "CIntEventReq::operator new";
        trace_printf("%s@%d  - Allocating %d bytes\n",
                     method_name, __LINE__, (int) size);
    }

    void * p = ::new char[size];

    return p;
}

void CIntEventReq::operator delete(void *deadObject, size_t size)
{
    if (trace_settings & TRACE_SYNC)
    {
        const char method_name[] = "CIntEventReq::operator delete";
        trace_printf("%s@%d  - Deleting %d bytes\n",
                     method_name, __LINE__, (int) size);
    }

    ::delete [] ((char *) deadObject);
}

void CIntEventReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf
           , "IntReq(%s) req #=%ld eventId=%d, target(nid=%d/pid=%d/verifier=%d), "
             "data length=%d"
           , CReqQueue::intReqType[InternalType_Event]
           , getId(), eventId_, targetNid_, targetPid_, targetVerifier_, length_ );
    requestString_.assign( strBuf );
}

void CIntEventReq::prepRequest( struct event_def *eventDef )
{
    const char method_name[] = "CIntEventReq::prepRequest";
    TRACE_ENTRY;

    eventId_ = eventDef->event_id;
    length_ = eventDef->length;
    targetNid_ = eventDef->nid;
    targetPid_ = eventDef->pid;
    targetVerifier_ = eventDef->verifier;

    if (length_ <= SMALL_DATA_SIZE)
    {
        memmove(data_, &eventDef->data, length_);
    }
    else
    {
        bigData_ = ::new char[length_];
        memmove(bigData_, &eventDef->data, length_);
    }

    TRACE_EXIT;
}

void CIntEventReq::performRequest()
{
    const char method_name[] = "CIntEventReq::performRequest";
    TRACE_ENTRY;

    CProcess * process = NULL;
    CLNode *   lnode;
    char *     eventData = NULL;

    if ( MyNode->IsMyNode( eventId_ ) )
    {
        if (trace_settings & TRACE_SYNC)
        {
            trace_printf( "%s@%d - processing event %d for (%d, %d:%d), data length=%d\n"
                        , method_name, __LINE__
                        , eventId_
                        , targetNid_
                        , targetPid_
                        , targetVerifier_
                        , length_ );
        }

        lnode = Nodes->GetLNode( targetNid_ );
        if (lnode)
        {
            process = lnode->GetProcessL( targetPid_ );

            if (process)
            {
                int verifier = targetVerifier_;
                if ( (verifier == -1) || (verifier == process->GetVerifier()) )
                {
                    eventData = (length_ <= SMALL_DATA_SIZE)?data_:bigData_;
                    process->GenerateEvent( eventId_
                                          , length_
                                          , eventData);
                }
                else
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], Can't find process nid=%d, "
                              "pid=%d, verifier=%d for event=%d\n"
                            , method_name
                            , targetNid_
                            , targetPid_
                            , targetVerifier_
                            , length_ );
                    mon_log_write(MON_INTREQ_EVENT_1, SQ_LOG_ERR, buf);
                }
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Can't find process nid=%d, "
                          "pid=%d for processing event.\n"
                        , method_name
                        , targetNid_
                        , targetPid_ );
                mon_log_write(MON_INTREQ_EVENT_2, SQ_LOG_ERR, buf);
            }
        }
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntExitReq::CIntExitReq( )
            : CInternalReq()
            , nid_(0)
            , pid_(0)
            , verifier_(-1)
            , abended_(false)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIE", 4);

    name_[0] = '\0';
}

CIntExitReq::~CIntExitReq( )
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqie", 4);
}

void CIntExitReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (name=%s/nid=%d/pid=%d/verifier=%d)"
                   , CReqQueue::intReqType[InternalType_Exit]
                   , getId(), name_, nid_, pid_, verifier_ );
    requestString_.assign( strBuf );
}

void CIntExitReq::prepRequest( struct exit_def *exitDef )
{
    const char method_name[] = "CIntExitReq::prepRequest";
    TRACE_ENTRY;

    nid_ = exitDef->nid;
    pid_ = exitDef->pid;
    verifier_ = exitDef->verifier;
    strcpy( name_, exitDef->name );
    abended_ = exitDef->abended;

    TRACE_EXIT;
}

void CIntExitReq::performRequest()
{
    const char method_name[] = "CIntExitReq::performRequest";
    TRACE_ENTRY;

    bool displayErrorMsg = false;
    CProcess *process = NULL;
    CLNode  *lnode;

    lnode = Nodes->GetLNode( nid_ );
    if ( lnode )
    {
        process = lnode->GetNode()->GetProcess( pid_ );

        if ( ! process )
        {
            // Could not locate process by process id.  If the exit
            // occurred due to an early process termination on another
            // node we won't have the process id.  Try the look up by
            // name instead.
            process = lnode->GetNode()->GetProcess( name_, false );
        }
    }

    if ( process )
    {
        if ( (verifier_ != -1) && (verifier_ != process->GetVerifier()) )
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
               trace_printf("%s@%d - Exit %s (%d, %d:%d) failed -- verifier mismatch (%d)\n",
                            method_name, __LINE__,
                            name_,
                            nid_,
                            pid_,
                            verifier_,
                            process->GetVerifier());
            }
        }
        else
        {
            lnode->GetNode()->DelFromNameMap ( process );
            lnode->GetNode()->DelFromPidMap ( process );
            lnode->GetNode()->Exit_Process (process, abended_, -1);
        }
    }
    else
    {
        if (lnode)
        {
            displayErrorMsg = true;
        }
        if (displayErrorMsg)
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], Can't find process %s (%d, %d) for processing "
                    "exit.\n", method_name, name_, nid_, pid_);
            mon_log_write(MON_INTREQ_EXIT_1, SQ_LOG_ERR, buf); 
        }
    }

    TRACE_EXIT;
}
#else
CIntExitNsReq::CIntExitNsReq( )
              : CInternalReq()
              , nid_(0)
              , pid_(0)
              , verifier_(-1)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIE", 4);

    name_[0] = '\0';
}

CIntExitNsReq::~CIntExitNsReq( )
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqie", 4);
}

void CIntExitNsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (name=%s/nid=%d/pid=%d/verifier=%d)"
                     "(msg=%p, sockFd=%d, origPNid=%d)"
                   , CReqQueue::intReqType[InternalType_Exit]
                   , getId(), name_, nid_, pid_, verifier_
                   , msg_, sockFd_, origPNid_ );
    requestString_.assign( strBuf );
}

void CIntExitNsReq::prepRequest( struct exit_ns_def *exitDef )
{
    const char method_name[] = "CIntExitNsReq::prepRequest";
    TRACE_ENTRY;

    nid_ = exitDef->nid;
    pid_ = exitDef->pid;
    verifier_ = exitDef->verifier;
    strcpy( name_, exitDef->name );
    abended_ = exitDef->abended;
    msg_ = exitDef->msg;
    sockFd_ = exitDef->sockFd;
    origPNid_ = exitDef->origPNid;

    TRACE_EXIT;
}

void CIntExitNsReq::performRequest()
{
    const char method_name[] = "CIntExitReq::performRequest";
    TRACE_ENTRY;

    CProcess *process = NULL;
    CLNode  *lnode;

    // Check if this name server is handling monitor request
    // from CExtDelProcessNsReq::performRequest()
    if (origPNid_ == MyPNID)
    {
        msg_->noreply = false;
        msg_->u.reply.type = ReplyType_DelProcessNs;
        msg_->u.reply.u.del_process_ns.nid = nid_;
        msg_->u.reply.u.del_process_ns.pid = pid_;
        msg_->u.reply.u.del_process_ns.verifier = verifier_;
        strncpy(msg_->u.reply.u.del_process_ns.process_name, name_, MAX_PROCESS_NAME);
        msg_->u.reply.u.del_process_ns.return_code = MPI_SUCCESS;

        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
           trace_printf("%s@%d - Replying to monitor sockFd_=%d, msg_=%p, deleted %s (%d, %d:%d)\n",
                        method_name, __LINE__,
                        sockFd_,
                        msg_,
                        name_,
                        nid_,
                        pid_,
                        verifier_ );
        }

        // Send reply to requesting monitor
        monreply( msg_, sockFd_ );
        return;
    }

    lnode = Nodes->GetLNode( nid_ );
    if ( lnode )
    {
        process = lnode->GetNode()->GetProcess( pid_ );

        if ( ! process )
        {
            // Could not locate process by process id.  If the exit
            // occurred due to an early process termination on another
            // node we won't have the process id.  Try the look up by
            // name instead.
            process = lnode->GetNode()->GetProcess( name_, false );
        }
    }

    if ( process )
    {
        if ( (verifier_ != -1) && (verifier_ != process->GetVerifier()) )
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
               trace_printf("%s@%d - Exit %s (%d, %d:%d) failed -- verifier mismatch (%d)\n",
                            method_name, __LINE__,
                            name_,
                            nid_,
                            pid_,
                            verifier_,
                            process->GetVerifier());
            }
        }
        else
        {
            lnode->GetNode()->Exit_Process( process, abended_, -1 );
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], Can't find process %s (%d, %d) for processing "
                "exit.\n", method_name, name_, nid_, pid_);
        mon_log_write(MON_CLUSTER_HANDLEOTHERNODE_5, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntIoDataReq::CIntIoDataReq( ioData_t *ioData )
             : CInternalReq()
             , nid_( ioData->nid )
             , pid_( ioData->pid )
             , verifier_( ioData->verifier )
             , ioType_( ioData->ioType )
             , length_( ioData->length )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqIK", 4);
    memcpy(data_, ioData->data, (length_<=MAX_SYNC_DATA)?length_:MAX_SYNC_DATA);
}

CIntIoDataReq::~CIntIoDataReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQik", 4);
}

void CIntIoDataReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (nid=%d/pid=%d/verifier=%d), type=%d, length=%d"
                   , CReqQueue::intReqType[InternalType_IoData]
                   , getId(), nid_, pid_, verifier_, ioType_, length_ );
    requestString_.assign( strBuf );
}

void CIntIoDataReq::performRequest()
{
    const char method_name[] = "CIntIoDataReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS))
    {
        trace_printf( "%s@%d" " - IO data "
                      "to (%d,%d:%d), count=%d\n(%s)"
                    , method_name, __LINE__
                    , nid_
                    , pid_
                    , verifier_
                    , length_
                    , length_?data_:"\n" );
    }
    if ( MyNode->IsMyNode( nid_ ) )
    {
        if (trace_settings & (TRACE_SYNC | TRACE_REDIRECTION))
            trace_printf( "%s@%d - processing IO Data for (%d, %d:%d)\n"
                        , method_name, __LINE__
                        , nid_, pid_, verifier_ );

        CLNode  *lnode;
        lnode = Nodes->GetLNode( nid_ );
        if ( lnode )
        {
            CProcess *process;
            process = lnode->GetProcessL( pid_ );
            if (process)
            {
                int fd;
                if (ioType_ == STDIN_DATA)
                {
                    fd = process->FdStdin();
                }
                else
                {
                    fd = process->FdStdout();
                }
                Redirector.disposeIoData( fd, length_, data_ );
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Can't find process nid"
                          "=%d, pid=%d for processing IO Data.\n"
                        , method_name, nid_, pid_ );
                mon_log_write(MON_REQ_IODATA_1, SQ_LOG_ERR, buf);
            }
        }
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntKillReq::CIntKillReq( struct kill_def *killDef )
            : CInternalReq()
            , nid_( killDef->nid )
            , pid_( killDef->pid )
            , verifier_( killDef->verifier )
            , abort_( killDef->persistent_abort )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIK", 4);
}

CIntKillReq::~CIntKillReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqik", 4);
}

void CIntKillReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (nid=%d/pid=%d/verifier=%d), abort=%d"
                   , CReqQueue::intReqType[InternalType_Kill]
                   , getId(), nid_, pid_, verifier_, abort_ );
    requestString_.assign( strBuf );
}

void CIntKillReq::performRequest()
{
    const char method_name[] = "CIntKillReq::performRequest";
    TRACE_ENTRY;

    CProcess *process = Nodes->GetProcess( nid_, pid_, false );

    if (process)
    {
        if ( (verifier_ != -1) && (verifier_ != process->GetVerifier()) )
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
               trace_printf("%s@%d - Kill (%d, %d:%d) failed -- verifier mismatch (%d)\n",
                            method_name, __LINE__,
                            nid_,
                            pid_,
                            verifier_,
                            process->GetVerifier());
            }
        }
        else
        {
            CLNode   *lnode = Nodes->GetLNode( nid_ );
            if ( lnode && process->GetPid() == pid_ )
            {

                // Remove mapping of name to process object.
                lnode->GetNode()->DelFromNameMap ( process );
            }
            else if (trace_settings & TRACE_PROCESS)
            {
                trace_printf("%s@%d - Leaving %s (%d, %d) in namemap, killed "
                             "(%d, %d:%d)\n", method_name, __LINE__,
                             process->GetName(), process->GetNid(),
                             process->GetPid(), nid_, pid_, verifier_ );
            }

            process->SetAbort( abort_ );

            if ( !process->IsClone() )
            {
                // Indicate thate process is down and abended
                lnode->GetNode()->SetProcessState( process, State_Down, true );

                // Save the pid/verifier to cleanup LIO buffers after SIGCHLD
                SQ_theLocalIOToClient->addToVerifierMap( process->GetPid()
                                                       , process->GetVerifier() );

                // Kill the process, will get child death signal later
                kill( pid_, Monitor->GetProcTermSig() );
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d - Completed kill(%d) for (%d, %d:%d)\n",
                                 method_name, __LINE__, Monitor->GetProcTermSig(),
                                 nid_, pid_, verifier_);
            }
            else
            {   // Actual process is on another node.
                if (trace_settings & TRACE_PROCESS_DETAIL)
                    trace_printf("%s@%d - Ingoring kill for clone process %s "
                                 "(%d, %d:%d), state=%d\n", method_name, __LINE__,
                                 process->GetName(), nid_, pid_, verifier_,
                                 process->GetState() );
            }

            CProcess *parent = Nodes->GetProcess( process->GetParentNid(),
                                                  process->GetParentPid() );
            process->Switch(parent); // switch process pair roles if needed
        }
    }
    else
    {
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Killing process but can't find process "
                         "(%d, %d:%d)\n", method_name, __LINE__,
                         nid_, pid_, verifier_ );
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntNewProcReq::CIntNewProcReq( int nid
                              , PROCESSTYPE type
                              , int priority
                              , int backup
                              , int parentNid
                              , int parentPid
                              , Verifier_t parentVerifier
                              , int pairParentNid
                              , int pairParentPid
                              , Verifier_t pairParentVerifier
                              , int argc
                              , bool unhooked
                              , void *reqTag
                              , strId_t pathStrId
                              , strId_t ldpathStrId
                              , strId_t programStrId
                              , int nameLen
                              , int infileLen
                              , int outfileLen
                              , int argvLen
                              , const char * stringData )
    : CInternalReq(),
      nid_ ( nid ),
      type_( type ),
      priority_( priority ),
      backup_( backup ),
      parentNid_( parentNid ),
      parentPid_( parentPid ),
      parentVerifier_( parentVerifier ),
      pairParentNid_( pairParentNid ),
      pairParentPid_( pairParentPid ),
      pairParentVerifier_( pairParentVerifier ),
      argc_( argc ),
      unhooked_( unhooked ),
      reqTag_ ( reqTag ),
      pathStrId_ ( pathStrId ),
      ldpathStrId_ ( ldpathStrId ),
      programStrId_ ( programStrId ),
      nameLen_ ( nameLen ),
      infileLen_ ( infileLen ),
      outfileLen_ ( outfileLen ),
      argvLen_ ( argvLen )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIN", 4);

    int stringDataLen = nameLen_ + infileLen_ + outfileLen_ + argvLen_;
    stringData_ = new char [stringDataLen];
    memcpy ( stringData_, stringData, stringDataLen );
}

CIntNewProcReq::~CIntNewProcReq( )
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqin", 4);

    delete [] stringData_;
}

void CIntNewProcReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (name=%s/nid=%d) parent(nid=%d/pid=%d)"
                   , CReqQueue::intReqType[InternalType_Process]
                   , getId(),
                   stringData_,
                   nid_, parentNid_, parentPid_ );
    requestString_.assign( strBuf );
}

void CIntNewProcReq::performRequest()
{
    const char method_name[] = "CIntNewProcReq::performRequest";
    TRACE_ENTRY;

    CProcess *parentProcess = NULL;
    int result = 0;

    if (trace_settings & TRACE_SYNC)
    {
        trace_printf("%s@%d - processing new process %s (%d, tbd) %s "
                     "(tag %p)\n", method_name, __LINE__,
                     stringData_,
                     nid_, (backup_?" Backup":""), reqTag_);
    }

    if (parentNid_ == -1)
    {
        parentProcess = NULL;
    }
    else
    {
        parentProcess = Nodes->GetProcess( parentNid_, parentPid_ );
        if ( parentProcess )
        {
            if ( (parentVerifier_ == -1) ||
                 (parentVerifier_ == parentProcess->GetVerifier()) )
            {
                if ( backup_ &&
                    (parentProcess->GetPairParentNid() == -1 &&
                     parentProcess->GetPairParentPid() == -1))
                {
                    parentProcess->SetPairParentNid( pairParentNid_ );
                    parentProcess->SetPairParentPid( pairParentPid_ );
                    parentProcess->SetPairParentVerifier( pairParentVerifier_ );
                }
            }
        }
        else
        {
            if (NameServerEnabled)
            {
                if (parentNid_ != -1 && parentPid_ != -1)
                {
                    if (trace_settings & TRACE_REQUEST)
                        trace_printf( "%s@%d" " - Getting parent process from Name Server (%d,%d:%d)\n"
                                    , method_name, __LINE__
                                    , parentNid_
                                    , parentPid_
                                    , parentVerifier_ );
                    parentProcess = Nodes->CloneProcessNs( parentNid_
                                                         , parentPid_
                                                         , parentVerifier_ );
                }
            }
        }
    }

    if (parentProcess || unhooked_ )
    {
        CLNode *lnode = Nodes->GetLNode(nid_);

        if ( lnode &&
            (lnode->GetState() == State_Up ||
             lnode->GetState() == State_Shutdown ) )
        {   // Create the CProcess object and store the various
            // process parameters.
            // cause strings to be forwarded
            string path;
            string ldpath;
            string program;
            Config->strIdToString( pathStrId_, path );
            Config->strIdToString( ldpathStrId_, ldpath );
            Config->strIdToString( programStrId_, program );
            CProcess *newProcess ;
            newProcess = lnode->GetNode()->
                CreateProcess ( parentProcess,
                                nid_,
                                type_,
                                0,
                                priority_,
                                backup_,
                                unhooked_,
                                &stringData_[0], // process name
                                pathStrId_,
                                ldpathStrId_,
                                programStrId_,
                                &stringData_[nameLen_],  // infile
                                &stringData_[nameLen_ + infileLen_], // outfile
                                reqTag_,
                                result);
            if ( newProcess != NULL )
            {
                newProcess->userArgs ( argc_, argvLen_,
                                       &stringData_[nameLen_ + infileLen_
                                                    + outfileLen_] );

                // Create the new process (fork/exec)
                if (newProcess->Create(newProcess->GetParent(), reqTag_, result))
                {
                    MyNode->AddToNameMap( newProcess );
                    MyNode->AddToPidMap( newProcess->GetPid(),  newProcess );

                    if (!NameServerEnabled)
                    {
                        // Successfully forked process.  Replicate actual process
                        // id and process name.
                        CReplProcInit *repl
                            = new CReplProcInit(newProcess, reqTag_, 0, parentNid_);
                        Replicator.addItem(repl);
                    }
                }
                else
                {
                    MyNode->DeleteFromList ( newProcess );
                    newProcess = NULL;
                }
            }

            if (  newProcess == NULL )
            {
                // Process creation failure, relay error code to node
                // that requested process creation.
                if (!NameServerEnabled)
                {
                    CReplProcInit *repl = new CReplProcInit(newProcess, reqTag_,
                                                            result, parentNid_);
                    Replicator.addItem(repl);
                }
            }
        }
    }
    else if ( parentProcess == NULL )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], Can't find parent process nid=%d, pid=%d "
                "for process create.\n", method_name,
                parentNid_, parentPid_ );
        mon_log_write(MON_INTREQ_NEWPROC_2, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}
#endif

#ifdef NAMESERVER_PROCESS
CIntNewProcNsReq::CIntNewProcNsReq( int nid
                                  , int pid
                                  , Verifier_t verifier
                                  , PROCESSTYPE type
                                  , int priority
                                  , int backup
                                  , int parentNid
                                  , int parentPid
                                  , Verifier_t parentVerifier
                                  , int pairParentNid
                                  , int pairParentPid
                                  , Verifier_t pairParentVerifier
                                  , int argc
                                  , bool unhooked
                                  , void* reqTag
                                  , int pathLen
                                  , int ldpathLen
                                  , int programLen
                                  , int nameLen
                                  , int infileLen
                                  , int outfileLen
                                  , int argvLen
                                  , const char* stringData )
                : CInternalReq()
                , nid_( nid )
                , pid_( pid )
                , verifier_( verifier )
                , type_( type )
                , priority_( priority )
                , backup_( backup )
                , parentNid_( parentNid )
                , parentPid_( parentPid )
                , parentVerifier_( parentVerifier )
                , pairParentNid_( pairParentNid )
                , pairParentPid_( pairParentPid )
                , pairParentVerifier_( pairParentVerifier )
                , argc_( argc )
                , unhooked_( unhooked )
                , reqTag_( reqTag )
                , pathLen_( pathLen )
                , ldpathLen_( ldpathLen )
                , programLen_( programLen )
                , nameLen_( nameLen )
                , infileLen_( infileLen )
                , outfileLen_( outfileLen )
                , argvLen_( argvLen )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqIN", 4);

    int stringDataLen = pathLen_ 
                      + ldpathLen_
                      + programLen_
                      + nameLen_
                      + infileLen_
                      + outfileLen_
                      + argvLen_;
    stringData_ = new char [stringDataLen];
    memcpy ( stringData_, stringData, stringDataLen );
}

CIntNewProcNsReq::~CIntNewProcNsReq( )
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQin", 4);

    delete [] stringData_;
}

void CIntNewProcNsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (name=%s/nid=%d) parent(nid=%d/pid=%d)"
                   , CReqQueue::intReqType[InternalType_Process]
                   , getId(),
                   stringData_,
                   nid_, parentNid_, parentPid_ );
    requestString_.assign( strBuf );
}

void CIntNewProcNsReq::performRequest()
{
    const char method_name[] = "CIntNewProcNsReq::performRequest";
    TRACE_ENTRY;

    CProcess *parentProcess = NULL;
    int result = 0;

    if (trace_settings & TRACE_SYNC)
    {
        trace_printf("%s@%d - processing new process %s (%d, tbd) %s "
                     "(tag %p)\n", method_name, __LINE__,
                     stringData_,
                     nid_, (backup_?" Backup":""), reqTag_);
    }

    if (parentNid_ == -1)
    {
        parentProcess = NULL;
    }
    else
    {
        parentProcess = Nodes->GetProcess( parentNid_, parentPid_ );
        if ( parentProcess )
        {
            if ( (parentVerifier_ == -1) ||
                 (parentVerifier_ == parentProcess->GetVerifier()) )
            {
                if ( backup_ &&
                    (parentProcess->GetPairParentNid() == -1 &&
                     parentProcess->GetPairParentPid() == -1))
                {
                    parentProcess->SetPairParentNid( pairParentNid_ );
                    parentProcess->SetPairParentPid( pairParentPid_ );
                    parentProcess->SetPairParentVerifier( pairParentVerifier_ );
                }
            }
        }
    }

    if (parentProcess || unhooked_ )
    {
        CProcess *newProcess = NULL;
        CLNode *lnode = Nodes->GetLNode(nid_);

        if ( lnode &&
            (lnode->GetState() == State_Up ||
             lnode->GetState() == State_Shutdown ) )
        {   // Create the CProcess object and store the various
            // process parameters.
            newProcess = lnode->GetNode()->
                CreateProcess ( parentProcess,
                                nid_,
                                pid_,
                                verifier_,
                                false,
                                false,
                                type_,
                                0,
                                priority_,
                                backup_,
                                unhooked_,
                                &stringData_[0], // process name
                                &stringData_[nameLen_ + infileLen_ + outfileLen_],  // path
                                &stringData_[nameLen_ + infileLen_ + outfileLen_ + pathLen_],  // ldpath
                                &stringData_[nameLen_ + infileLen_ + outfileLen_ + pathLen_ + ldpathLen_],  // program
                                &stringData_[nameLen_],  // infile
                                &stringData_[nameLen_ + infileLen_], // outfile
                                reqTag_,
                                result);
        }
        if (  newProcess == NULL )
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf( buf, "[%s], Can't create process %s (%d,%d:%d)\n"
                   , method_name, &stringData_[0],nid_, pid_, verifier_ );
            mon_log_write(MON_INTREQ_NEWPROC_1, SQ_LOG_ERR, buf);
        }
    }
    else if ( parentProcess == NULL )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], Can't find parent process nid=%d, pid=%d "
                "for process create.\n", method_name,
                parentNid_, parentPid_ );
        mon_log_write(MON_INTREQ_NEWPROC_3, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntNotifyReq::CIntNotifyReq( struct notify_def *notifyDef ) 
             : CInternalReq()
             , nid_( notifyDef->nid )
             , pid_( notifyDef->pid )
             , verifier_( notifyDef->verifier )
             , canceled_( notifyDef->canceled )
             , targetNid_( notifyDef->target_nid )
             , targetPid_( notifyDef->target_pid )
             , targetVerifier_( notifyDef->target_verifier )
             , transId_( notifyDef->trans_id )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQI1", 4);
}

CIntNotifyReq::~CIntNotifyReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqi1", 4);
}

void CIntNotifyReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld notify(nid=%d/pid=%d/verifier=%d) death target(nid=%d/pid=%d/verifier=%d), canceled=%d, transId=%lld.%lld.%lld.%lld"
                   , CReqQueue::intReqType[InternalType_Notify]
                   , getId()
                   , nid_
                   , pid_
                   , verifier_
                   , targetNid_
                   , targetPid_
                   , targetVerifier_
                   , canceled_
                   , transId_.txid[0]
                   , transId_.txid[1]
                   , transId_.txid[2]
                   , transId_.txid[3] );
    requestString_.assign( strBuf );
}

void CIntNotifyReq::performRequest()
{
    const char method_name[] = "CIntNotifyReq::performRequest";
    TRACE_ENTRY;

    //CLNode *node;
    //CLNode *targetNode;
    CProcess *sourceProcess = NULL;
    CProcess *targetProcess = NULL;

    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf( "%s@%d - Process death notification, canceled=%d\n"
                    , method_name, __LINE__
                    , canceled_ );
    }

    
    if (trace_settings & TRACE_REQUEST)
    {
        trace_printf( "%s@%d" " - Finding targetProcess (%d,%d:%d)\n"
                    , method_name, __LINE__
                    , nid_
                    , pid_
                    , verifier_ );
    }

    // find by nid,pid (check node state, don't check process state, backup is Ok)
    sourceProcess = Nodes->GetProcess( nid_
                                     , pid_
                                     , verifier_
                                     , true, false, true );
    if ( sourceProcess )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf( "%s@%d - Found sourceProcess %s (%d,%d:%d), clone=%d\n"
                        , method_name, __LINE__
                        , sourceProcess->GetName()
                        , sourceProcess->GetNid()
                        , sourceProcess->GetPid()
                        , sourceProcess->GetVerifier()
                        , sourceProcess->IsClone() );
        }
    }
    else
    {
        if (!NameServerEnabled)
        {
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf("%s@%d - Can't find sourceProcess\n", method_name, __LINE__);
            }
        }
        else
        { // Name Server find by nid,pid:verifier
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d" " - Getting sourceProcess from Name Server (%d,%d:%d)\n"
                            , method_name, __LINE__
                            , nid_
                            , pid_
                            , verifier_ );
            }
            sourceProcess = Nodes->CloneProcessNs( nid_
                                                 , pid_
                                                 , verifier_ );
            if (sourceProcess)
            {
                if (trace_settings & TRACE_REQUEST)
                    trace_printf( "%s@%d - Found sourceProcess %s (%d,%d:%d), clone=%d\n"
                                , method_name, __LINE__
                                , sourceProcess->GetName()
                                , sourceProcess->GetNid()
                                , sourceProcess->GetPid()
                                , sourceProcess->GetVerifier()
                                , sourceProcess->IsClone() );
            }
            else
            {
                trace_printf( "%s@%d" " - Can't find sourceProcess (%d,%d:%d)\n"
                            , method_name, __LINE__
                            , nid_
                            , pid_
                            , verifier_ );
            }
        }
    }
    
    if ( sourceProcess )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf( "%s@%d" " - Finding targetProcess (%d,%d:%d)\n"
                        , method_name, __LINE__
                        , targetNid_
                        , targetPid_
                        , targetVerifier_ );
        }
    
        // find by nid,pid (check node state, don't check process state, backup is Ok)
        targetProcess = Nodes->GetProcess( targetNid_
                                         , targetPid_
                                         , targetVerifier_
                                         , true, false, true );
        if ( targetProcess )
        {
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d - Found targetProcess %s (%d,%d:%d), clone=%d\n"
                            , method_name, __LINE__
                            , targetProcess->GetName()
                            , targetProcess->GetNid()
                            , targetProcess->GetPid()
                            , targetProcess->GetVerifier()
                            , targetProcess->IsClone() );
            }
        }
        else
        {
            if (!NameServerEnabled)
            {
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf("%s@%d - Can't find targetProcess\n", method_name, __LINE__);
                }
            }
            else
            { // Name Server find by nid,pid:verifier
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf( "%s@%d" " - Getting targetProcess from Name Server (%d,%d:%d)\n"
                                , method_name, __LINE__
                                , targetNid_
                                , targetPid_
                                , targetVerifier_ );
                }
                targetProcess = Nodes->CloneProcessNs( targetNid_
                                                     , targetPid_
                                                     , targetVerifier_ );
                if (targetProcess)
                {
                    if (trace_settings & TRACE_REQUEST)
                        trace_printf( "%s@%d - Found targetProcess %s (%d,%d:%d), clone=%d\n"
                                    , method_name, __LINE__
                                    , targetProcess->GetName()
                                    , targetProcess->GetNid()
                                    , targetProcess->GetPid()
                                    , targetProcess->GetVerifier()
                                    , targetProcess->IsClone() );
                }
                else
                {
                    trace_printf( "%s@%d" " - Can't find targetProcess (%d,%d:%d)\n"
                                , method_name, __LINE__
                                , targetNid_
                                , targetPid_
                                , targetVerifier_ );
                }
            }
        }
        
        if ( targetProcess )
        {
            if (canceled_)
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                {
                    trace_printf( "%s@%d - Process (%d, %d:%d) deleting death "
                                  "notice interest for %s (%d, %d:%d), "
                                  "trans_id=%lld.%lld.%lld.%lld\n"
                                , method_name, __LINE__
                                , nid_
                                , pid_
                                , verifier_
                                , targetProcess->GetName()
                                , targetNid_
                                , targetPid_
                                , targetVerifier_
                                , transId_.txid[0]
                                , transId_.txid[1]
                                , transId_.txid[2]
                                , transId_.txid[3] );
                }
            
                // Remove death notice registration
                targetProcess->CancelDeathNotification( nid_
                                                      , pid_
                                                      , verifier_
                                                      , transId_ );
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                {
                    trace_printf("%s@%d - Process (%d, %d:%d) registering interest "
                                 "in death of process %s (%d, %d:%d), "
                                 "trans_id=%lld.%lld.%lld.%lld\n"
                                , method_name, __LINE__
                                , nid_
                                , pid_
                                , verifier_
                                , targetProcess->GetName()
                                , targetNid_
                                , targetPid_
                                , targetVerifier_
                                , transId_.txid[0]
                                , transId_.txid[1]
                                , transId_.txid[2]
                                , transId_.txid[3] );
                }
                // Register interest with the target process 
                sourceProcess->procExitReg( targetProcess, transId_);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf( buf
                   , "[%s], Can't find target process nid=%d, pid=%d:%d for "
                     "processing process death notification.\n"
                   , method_name
                   , targetNid_
                   , targetPid_
                   , targetVerifier_ );
            mon_log_write(MON_INTREQ_NOTIFY_1, SQ_LOG_INFO, buf);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf( buf
               , "[%s], Can't find sourceProcess nid=%d, pid=%d:%d for "
                 "processing process death notification.\n"
               , method_name
               , nid_
               , pid_
               , verifier_ );
        mon_log_write(MON_INTREQ_NOTIFY_3, SQ_LOG_INFO, buf);
    }
    
    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntOpenReq::CIntOpenReq( struct open_def *openDef ) 
            : CInternalReq()
            , openerNid_( openDef->nid )
            , openerPid_( openDef->pid )
            , openerVerifier_( openDef->verifier )
            , openedNid_( openDef->opened_nid )
            , openedPid_( openDef->opened_pid )
            , openedVerifier_( openDef->opened_verifier )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIO", 4);
}

CIntOpenReq::~CIntOpenReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqio", 4);
}

void CIntOpenReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld opener(nid=%d/pid=%d/verifier=%d) opened(nid=%d/pid=%d/verifier=%d)"
                   , CReqQueue::intReqType[InternalType_Open]
                   , getId()
                   , openerNid_
                   , openerPid_
                   , openerVerifier_
                   , openedNid_
                   , openedPid_
                   , openedVerifier_ );
    requestString_.assign( strBuf );
}

void CIntOpenReq::performRequest()
{
    const char method_name[] = "CIntOpenReq::performRequest";
    TRACE_ENTRY;

    CProcess *process = Nodes->GetProcess( openedNid_, openedPid_ );

    if (process)
    {
        if ( (openedVerifier_ != -1) && (openedVerifier_ != process->GetVerifier()) )
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
               trace_printf("%s@%d - Open (%d, %d:%d) failed -- verifier mismatch (%d)\n",
                            method_name, __LINE__,
                            openedNid_,
                            openedPid_,
                            openedVerifier_,
                            process->GetVerifier());
            }
        }
        else
        {
            Nodes->GetLNode (openerNid_)->
                    Open_Process (openerNid_,
                                  openerPid_,
                                  openerVerifier_,
                                  0, // notification will be handle independently
                                  process);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], Can't find process nid=%d, pid=%d for "
                "processing open.\n", method_name, openedNid_, openedPid_ );
        mon_log_write(MON_INTREQ_OPEN_1, SQ_LOG_ERR, buf); 
    }

    TRACE_EXIT;
}
#endif

CIntProcInitReq::CIntProcInitReq( struct process_init_def *procInitDef )
                : CInternalReq()
                , nid_( procInitDef->nid )
                , pid_( procInitDef->pid )
                , verifier_( procInitDef->verifier )
                , state_( procInitDef->state )
                , result_( procInitDef->result )
                , process_( (CProcess *) procInitDef->tag )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQII", 4);

    STRCPY(name_, procInitDef->name);
}

CIntProcInitReq::~CIntProcInitReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqii", 4);
}

void CIntProcInitReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (name=%s/nid=%d/pid=%d,verifier=%d)"
                   , CReqQueue::intReqType[InternalType_ProcessInit]
                   , getId(), name_, nid_, pid_, verifier_ );
    requestString_.assign( strBuf );
}

void CIntProcInitReq::performRequest()
{
    const char method_name[] = "CIntProcInitReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
        trace_printf( "%s@%d - processing process init %s (%d, %d), result=%d, tag=%p\n"
                    , method_name, __LINE__
                    , name_, nid_, pid_, result_, static_cast<void*>(process_) );

    if ( result_ != 0 )
    {  // Was unable to create the process, send response to requester
        if ( process_ )
        {
#ifndef NAMESERVER_PROCESS
            // this will send response to to the requester and remove the process object
            MyNode->Exit_Process(process_, true, process_->GetNid());
#endif
        }
    }
    else if ( process_ )
    {
        // Update process state information
        process_->SetPid ( pid_ );
        process_->SetVerifier ( verifier_ );
        process_->SetState ( state_ );
        process_->SetName ( name_ );

        // Add to pid and name maps
        Nodes->GetLNode( process_->GetNid() )->GetNode()->AddToPidMap(process_->GetPid(), process_);
        Nodes->GetLNode( process_->GetNid() )->GetNode()->AddToNameMap(process_);

        CProcess* parent;

        if (process_->IsBackup())
        {
            parent = Nodes->GetProcess(process_->GetParentNid(),
                                       process_->GetParentPid(), false);
            if (parent)
            {   // Set link from primary process object to
                // this backup process object.
                if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
                {
                    trace_printf( "%s@%d - For backup process %s (%d,%d:%d)"
                                  ", for parent %s (%d,%d:%d) setting "
                                  "parent's Parent_Nid/Parent_Pid="
                                  "(%d,%d).\n"
                                , method_name, __LINE__
                                , process_->GetName()
                                , process_->GetNid()
                                , process_->GetPid()
                                , process_->GetVerifier()
                                , parent->GetName()
                                , parent->GetNid()
                                , parent->GetPid()
                                , parent->GetVerifier()
                                , process_->GetNid()
                                , process_->GetPid());
                }
                parent->SetParentNid ( process_->GetNid() );
                parent->SetParentPid ( process_->GetPid() );
            }
        }
#ifndef NAMESERVER_PROCESS
        if (NameServerEnabled)
        {
            if (process_->IsUnhooked())
            {
                if ( process_->GetParentNid() != -1 && process_->GetParentPid() != -1 )
                {
                    parent = Nodes->GetProcess(process_->GetParentNid(),
                                               process_->GetParentPid(), false);
                    if (parent && !parent->IsClone())
                    {   // Parent process object keeps track of child processes
                        // created. Needed when parent process exits to clean up
                        // parent clone process object in remote nodes.
                        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL
                                              | TRACE_PROCESS_DETAIL))
                            trace_printf( "%s@%d - Adding unhooked child process %s (%d,%d:%d) to "
                                          "parent %s (%d,%d:%d)\n"
                                        , method_name, __LINE__
                                        , process_->GetName()
                                        , process_->GetNid()
                                        , process_->GetPid()
                                        , process_->GetVerifier()
                                        , parent->GetName()
                                        , parent->GetNid()
                                        , parent->GetPid()
                                        , parent->GetVerifier() );
            
                        parent->childUnHookedAdd( process_->GetNid()
                                                , process_->GetPid() );
                    }
                }
            }
        }
#endif
    }

    TRACE_EXIT;
}

CIntSetReq::CIntSetReq( ConfigType type, const char *group, const char *key,
                        const char *value)
    : CInternalReq(),
      type_( type )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIS", 4);

    STRCPY( group_, group );
    STRCPY( key_, key );
    STRCPY( value_, value );
}

CIntSetReq::~CIntSetReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqis", 4);
}

void CIntSetReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (group=%s/key=%s)"
                   , CReqQueue::intReqType[InternalType_Set]
                   , getId(), group_, key_ );
    requestString_.assign( strBuf );
}

void CIntSetReq::performRequest()
{
    const char method_name[] = "CIntSetReq::performRequest";
    TRACE_ENTRY;

    Config->Set( group_, type_, key_, value_, true );

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
CIntStdInReq::CIntStdInReq( struct stdin_req_def *stdin_req )
             : CInternalReq()
             , nid_( stdin_req->nid )
             , pid_( stdin_req->pid )
             , verifier_( stdin_req->verifier )
             , reqType_( stdin_req->reqType )
             , supplierNid_( stdin_req->supplier_nid )
             , supplierPid_( stdin_req->supplier_pid )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqIS", 4);
}

CIntStdInReq::~CIntStdInReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQis", 4);
}

void CIntStdInReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (nid=%d/pid=%d/verifier=%d), "
                     "type=%d, supplier (%d,%d)"
                   , CReqQueue::intReqType[InternalType_StdinReq]
                   , getId(), nid_, pid_, verifier_, reqType_
                   , supplierNid_, supplierPid_ );
    requestString_.assign( strBuf );
}

void CIntStdInReq::performRequest()
{
    const char method_name[] = "CIntStdInReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS))
    {
        trace_printf("%s@%d - stdin request from (%d,%d:%d)"
                     ", type=%d, for supplier (%d,%d)\n"
                    , method_name, __LINE__
                    , nid_
                    , pid_
                    , verifier_
                    , reqType_
                    , supplierNid_
                    , supplierPid_ );
    }

    if ( !MyNode->IsMyNode( supplierNid_ ) )
    {
        return;
    }

    CLNode  *lnode;
    lnode = Nodes->GetLNode( nid_ );
    if ( lnode == NULL )
    {
        return;
    }

    CProcess *process;
    process = lnode->GetProcessL( pid_ );
    if (process)
    {
        if (reqType_ == STDIN_REQ_DATA)
        {
            // Set up to forward stdin data to requester.
            // Save file descriptor associated with stdin
            // so can find the redirector object later.
            CProcess *supProcess;
            lnode = Nodes->GetLNode( supplierNid_ );
            if ( lnode )
            {
                supProcess = lnode->GetProcessL ( supplierPid_ );
                if (supProcess)
                {
                    int fd;
                    fd = Redirector.stdinRemote( supProcess->infile()
                                               , supplierNid_
                                               , supplierPid_ );
                    process->FdStdin(fd);
                }
                else
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf), 
                              "[%s], Can't find supplier process "
                              "nid=%d, pid=%d for stdin data request.\n"
                            , method_name
                            , supplierNid_
                            , supplierPid_);
                    mon_log_write(MON_REQ_STDIN_1, SQ_LOG_ERR, buf);
                }
            }
        }
        else if (reqType_ == STDIN_FLOW_OFF)
        {
            Redirector.stdinOff(process->FdStdin());
        }
        else if (reqType_ == STDIN_FLOW_ON)
        {
            Redirector.stdinOn(process->FdStdin());
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Can't find process nid=%d, "
                  "pid=%d for stdin data request.\n"
                , method_name
                , nid_
                , pid_ );
        mon_log_write(MON_REQ_STDIN_2, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}
#endif

CIntUniqStrReq::CIntUniqStrReq( int nid, int id, const char *value )
    : CInternalReq(), nid_(nid), id_(id)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIF", 4);

    STRCPY( value_, value );
}

CIntUniqStrReq::~CIntUniqStrReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqif", 4);
}

void CIntUniqStrReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (nid=%d/id=%d)"
                   , CReqQueue::intReqType[InternalType_UniqStr]
                   , getId(), nid_, id_ );
    requestString_.assign( strBuf );
}

void CIntUniqStrReq::performRequest()
{
    const char method_name[] = "CIntUniqStrReq::performRequest";
    TRACE_ENTRY;

    Config->addUniqueString( nid_, id_, value_ );

    TRACE_EXIT;
}


CIntChildDeathReq::CIntChildDeathReq( pid_t pid )
    : CInternalReq(), pid_(pid), process_(NULL)
{
    const char method_name[] = "CIntChildDeathReq::CIntChildDeathReq";

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIC", 4);

    process_ = MyNode->GetProcess( pid );

    if (!process_)
    {
        if (trace_settings & TRACE_PROCESS)
        {
           trace_printf("%s@%d Process %d not found so unable to set "
                        "process state.\n", method_name, __LINE__, pid);
        }
    }
}

CIntChildDeathReq::~CIntChildDeathReq( )
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqic", 4);
}

void CIntChildDeathReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(ChildDeath) req #=%ld (pid=%d)"
                   , getId(), pid_ );
    requestString_.assign( strBuf );
}

void CIntChildDeathReq::performRequest()
{
    const char method_name[] = "CIntChildDeathReq::performRequest";

    // process could have been deleted by a previous child death
    // request. This could happen if child death signal did not arrive
    // in hangupTime + PROCESS_DEATH_MARGIN. So get process ptr again.
    process_ = MyNode->GetProcess( pid_ );

    if ( process_ != NULL)
    {
        if (trace_settings & TRACE_PROCESS)
        {
            trace_printf( "%s@%d Processing child death "
                          "of process %s (%d, %d:%d)\n"
                         , method_name, __LINE__
                         , process_->GetName()
                         , process_->GetNid()
                         , process_->GetPid()
                         , process_->GetVerifier() );
        }
#ifndef NAMESERVER_PROCESS
        if ( NameServerEnabled && process_ != NameServerProcess)
        {
            int rc = NameServer->ProcessDelete(process_); // in reqQueue thread (CIntChildDeathReq)
            if (rc)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s] - Process delete request to Name Server failed"
                          "for child process %s (%d, %d:%d)\n"
                        , method_name
                        , process_->GetName()
                        , process_->GetNid()
                        , process_->GetPid()
                        , process_->GetVerifier() );
                mon_log_write(MON_INTREQ_CHILDDEATH_1, SQ_LOG_ERR, la_buf);
            }
        }
#endif
        MyNode->DelFromNameMap ( process_ );
        MyNode->DelFromPidMap ( process_ );

        // if state is still Up, then process has not called exit.
        bool abended = (process_->GetState() == State_Up);
        MyNode->SetProcessState(process_, State_Stopped, abended);

    }
}

CIntAttachedDeathReq::CIntAttachedDeathReq( pid_t pid )
    : CInternalReq(), pid_(pid)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIA", 4);
}

CIntAttachedDeathReq::~CIntAttachedDeathReq( )
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqia", 4);
}

void CIntAttachedDeathReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(AttachedDeath) req #=%ld (pid=%d)"
                   , getId(), pid_ );
    requestString_.assign( strBuf );
}

void CIntAttachedDeathReq::performRequest()
{
    CProcess * process = MyNode->GetProcess ( pid_ );

    if (process)
    {
        MyNode->DelFromNameMap ( process );
        MyNode->DelFromPidMap ( process );

        MyNode->SetProcessState(process, State_Stopped, true); // abend
    }
}

CIntShutdownReq::CIntShutdownReq( int level )
    : CInternalReq(),
      level_ ( level )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIH", 4);
}

CIntShutdownReq::~CIntShutdownReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqih", 4);
}

void CIntShutdownReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (level=%d)"
                   , CReqQueue::intReqType[InternalType_Shutdown]
                   , getId(), level_ );
    requestString_.assign( strBuf );
}

void CIntShutdownReq::performRequest()
{
    const char method_name[] = "CIntShutdownReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf("%s@%d - Shutdown request, level=%d\n",
                     method_name, __LINE__, level_);

    MyNode->SetShutdownLevel( (ShutdownLevel) level_ );

    // only abrupt case is supported through this mechanism at present.
    // modify this assert as more shutdown levels are supported here.
    assert(level_ == ShutdownLevel_Abrupt);

    if( !getenv("SQ_VIRTUAL_NODES") )
    {
        // Execute shutdown via the Watchdog process
        HealthCheck.setState(MON_SHUT_DOWN);
        // wait forever if not a spare node
        // spare node will go through clean shutdown
        if ( !MyNode->IsSpareNode() )
        {
            for (;;)
                sleep(10000);
        }
    }
    else
    {
        // Stop all processes
#ifndef NAMESERVER_PROCESS
        Monitor->HardNodeDown( MyPNID );
        MyNode->EmptyQuiescingPids();
#else
        Monitor->HardNodeDownNs( MyPNID );
#endif
        // now stop the Watchdog process
        HealthCheck.setState(MON_NODE_DOWN);
    }

    TRACE_EXIT;
}

CIntNameServerAddReq::CIntNameServerAddReq( int req_nid
                                          , int req_pid
                                          , Verifier_t req_verifier
                                          , char *nodeName
                                          )
              : CInternalReq()
              , req_nid_(req_nid)
              , req_pid_(req_pid)
              , req_verifier_(req_verifier)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqIA", 4);
    STRCPY( nodeName_, nodeName );
}

CIntNameServerAddReq::~CIntNameServerAddReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQia", 4);
}

void CIntNameServerAddReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    snprintf( strBuf, sizeof(strBuf),
              "IntReq(%s) req #=%ld "
              "(node_name=%s)"
            , CReqQueue::intReqType[InternalType_NodeAdd]
            , getId()
            , nodeName_ );
    requestString_.assign( strBuf );
}

void CIntNameServerAddReq::performRequest()
{
    const char method_name[] = "CIntNameServerAddReq::performRequest";
    TRACE_ENTRY;

    int rc = MPI_SUCCESS;
    CProcess *requester = NULL;

    if (trace_settings & (TRACE_SYNC | TRACE_PROCESS))
    {
        trace_printf("%s@%d - NameServer add request (%s), "
                     "node_name=%s\n"
                    , method_name, __LINE__
                    , requester ? requester->GetName() : ""
                    , nodeName_);
    }

    CNameServerConfigContainer *nameServerConfig = Nodes->GetNameServerConfig();

    // Insert nameserver in configuration database
    if ( nameServerConfig->SaveConfig( nodeName_ ) )
    {
    }
    else
    {
        rc = MPI_ERR_IO;
    }

    requester = Nodes->GetProcess( req_nid_
                                 , req_pid_
                                 , req_verifier_ );
    if (requester)
    {
        // Reply to requester
        requester->CompleteRequest( rc );
    }

    TRACE_EXIT;
}

CIntNameServerDeleteReq::CIntNameServerDeleteReq( int req_nid
                                                , int req_pid
                                                , Verifier_t req_verifier
                                                , const char *nodeName )
                 : CInternalReq()
                 , req_nid_(req_nid)
                 , req_pid_(req_pid)
                 , req_verifier_(req_verifier)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqIB", 4);
    STRCPY( nodeName_, nodeName );
}

CIntNameServerDeleteReq::~CIntNameServerDeleteReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQib", 4);
}

void CIntNameServerDeleteReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    snprintf( strBuf, sizeof(strBuf),
              "IntReq(%s) req #=%ld "
              "(node=%s)"
            , CReqQueue::intReqType[InternalType_NameServerDelete]
            , getId()
            , nodeName_ );
    requestString_.assign( strBuf );
}

void CIntNameServerDeleteReq::performRequest()
{
    const char method_name[] = "CIntNameServerDeleteReq::performRequest";
    TRACE_ENTRY;

    int rc = MPI_SUCCESS;
    CProcess *requester = NULL;

    requester = Nodes->GetProcess( req_nid_
                                 , req_pid_
                                 , req_verifier_ );

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf( "%s@%d - NameServer delete request (%s), node=%s\n"
                    , method_name, __LINE__
                    , requester ? requester->GetName() : ""
                    , nodeName_ );

    CNameServerConfigContainer *nameServerConfig = Nodes->GetNameServerConfig();
    CNameServerConfig *config = nameServerConfig->GetConfig( nodeName_ );
    if ( config )
    {
        if ( !nameServerConfig->DeleteConfig( config ) )
            rc = MPI_ERR_IO;
    }
    else
    {
        rc = MPI_ERR_IO;
    }

    requester = Nodes->GetProcess( req_nid_
                                 , req_pid_
                                 , req_verifier_ );
    if (requester)
    {
        // Reply to requester
        requester->CompleteRequest( rc );
    }

    TRACE_EXIT;
}

CIntNodeNameReq::CIntNodeNameReq( int req_nid
                                , int req_pid
                                , Verifier_t req_verifier
                                , const char *current_name
                                , const char *new_name  )
                : CInternalReq()
                , req_nid_(req_nid)
                , req_pid_(req_pid)
                , req_verifier_(req_verifier)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIZ", 4);
    new_name_ = new_name;
    current_name_=current_name;
}

CIntNodeNameReq::~CIntNodeNameReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqiz", 4);
}

void CIntNodeNameReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld"
                   , CReqQueue::intReqType[InternalType_NodeName]
                   , getId() );
    requestString_.assign( strBuf );
}

void CIntNodeNameReq::performRequest()
{
    const char method_name[] = "CIntNodeNameReq::performRequest";
    TRACE_ENTRY;

    int rc = MPI_SUCCESS;
    char current_n[TC_PROCESSOR_NAME_MAX];
    char new_n[TC_PROCESSOR_NAME_MAX];
    char new_domain[TC_PROCESSOR_NAME_MAX];
    CPNodeConfig   *pnodeConfig = NULL;
    CProcess *requester = NULL;

    strcpy( current_n, current_name_.c_str() );
    strcpy( new_n, new_name_.c_str() );

    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    if (clusterConfig)
    {
        // Check for existence of node name in the configuration
        pnodeConfig = clusterConfig->GetPNodeConfig( current_n );
        if (pnodeConfig)
        {
            char short_node_name[TC_PROCESSOR_NAME_MAX];
            char str1[TC_PROCESSOR_NAME_MAX];
            char *tmpptr = NULL;

            tmpptr = new_n;
            while ( *tmpptr )
            {
                *tmpptr = (char)tolower( *tmpptr );
                tmpptr++;
            }
        
            if (IsRealCluster)
            {
                // Extract the domain portion of the name if any
                memset( str1, 0, TC_PROCESSOR_NAME_MAX );
                memset( short_node_name, 0, TC_PROCESSOR_NAME_MAX );
                strcpy( str1, new_n );
    
                char *str1_dot = strchr( (char *) str1, '.' );
                if ( str1_dot )
                {
                    memcpy( short_node_name, str1, str1_dot - str1 );
                    // copy the domain portion and skip the '.'
                    strcpy( new_domain, str1_dot+1 );
                }
                else
                {
                    strcpy(short_node_name, str1 );
                    new_domain[0] = 0;
                }
    
                strcpy(new_n, short_node_name);
    
            }
    
            if (trace_settings & TRACE_PROCESS)
            {
                trace_printf( "%s@%d name=%s, domain=%s\n"
                              , method_name, __LINE__
                              , new_n
                              , new_domain );
            }
    
            // Update the node name in the configuration database
            if (clusterConfig->UpdatePNodeConfig( pnodeConfig->GetPNid()
                                                , new_n
                                                , new_domain
                                                , pnodeConfig->GetExcludedFirstCore()
                                                , pnodeConfig->GetExcludedLastCore() ))
            {
                // lock sync thread since we are making a change the monitor's
                // operational view of the cluster
                if ( !Emulate_Down )
                {
                    Monitor->EnterSyncCycle();
                }

                // Change node name in monitor's view of cluster
                CNode    *node = Nodes->GetNode(current_n);
                if (node)
                {
                    node->SetName( new_n );
                    Nodes->ChangedNode( node );
                }
                else
                {
                    char buf[MON_STRING_BUF_SIZE];
                    sprintf( buf
                           , "[%s], Failed to retrieve node object for node %s!\n"
                           ,  method_name, current_n);
                    mon_log_write(MON_INTREQ_NODE_NAME_1, SQ_LOG_ERR, buf);

                    rc = MPI_ERR_INTERN;
                }

                // unlock sync thread
                if ( !Emulate_Down )
                {
                    Monitor->ExitSyncCycle();
                }
            }
            else
            {
                rc = MPI_ERR_IO;
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf( buf
                   , "[%s], Failed to retrieve pnodeConfig object for node %s!\n"
                   ,  method_name, current_n);
            mon_log_write(MON_INTREQ_NODE_NAME_2, SQ_LOG_ERR, buf);

            rc = MPI_ERR_INTERN;
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], Failed to retrieve ClusterConfig object!\n",
                method_name);
        mon_log_write(MON_INTREQ_NODE_NAME_3, SQ_LOG_ERR, buf);

        rc = MPI_ERR_INTERN;
    }

    requester = Nodes->GetProcess( req_nid_
                                 , req_pid_
                                 , req_verifier_ );
    if (requester)
    {
        // Reply to requester
        requester->CompleteRequest( rc );
    }


    TRACE_EXIT;
}


#ifndef NAMESERVER_PROCESS
CIntNodeAddReq::CIntNodeAddReq( int req_nid
                              , int req_pid
                              , Verifier_t req_verifier
                              , char *nodeName
                              , int  firstCore
                              , int  lastCore
                              , int  processors
                              , int  roles
                              )
              : CInternalReq()
              , req_nid_(req_nid)
              , req_pid_(req_pid)
              , req_verifier_(req_verifier)
              , first_core_(firstCore)
              , last_core_(lastCore)
              , processors_(processors)
              , roles_(roles)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIJ", 4);
    STRCPY( nodeName_, nodeName );
}

CIntNodeAddReq::~CIntNodeAddReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqij", 4);
}

void CIntNodeAddReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    snprintf( strBuf, sizeof(strBuf),
              "IntReq(%s) req #=%ld "
              "(node_name=%s/first_core=%d/last_core=%d/processors=%d/roles=%d)"
            , CReqQueue::intReqType[InternalType_NodeAdd]
            , getId()
            , nodeName_
            , first_core_
            , last_core_
            , processors_
            , roles_ );
    requestString_.assign( strBuf );
}

void CIntNodeAddReq::performRequest()
{
    const char method_name[] = "CIntNodeAddReq::performRequest";
    TRACE_ENTRY;

    int nid;
    int pnid;
    int rc = MPI_SUCCESS;
    char node_name[TC_PROCESSOR_NAME_MAX];
    char domain_name[TC_PROCESSOR_NAME_MAX];
    CProcess *requester = NULL;

    char short_node_name[TC_PROCESSOR_NAME_MAX];
    char str1[TC_PROCESSOR_NAME_MAX];
    char *tmpptr = NULL;

    tmpptr = nodeName_;
    while ( *tmpptr )
    {
        *tmpptr = (char)tolower( *tmpptr );
        tmpptr++;
    }

    if (IsRealCluster)
    {
        // Extract the domain portion of the name if any
        memset( str1, 0, TC_PROCESSOR_NAME_MAX );
        memset( short_node_name, 0, TC_PROCESSOR_NAME_MAX );
        strcpy( str1, nodeName_ );

        char *str1_dot = strchr( (char *) str1, '.' );
        if ( str1_dot )
        {
            memcpy( short_node_name, str1, str1_dot - str1 );
            // copy the domain portion and skip the '.'
            strcpy( domain_name, str1_dot+1 );
        }
        else
        {
            strcpy(short_node_name, str1 );
            domain_name[0] = 0;
        }

        strcpy(node_name, short_node_name);

    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY| TRACE_REQUEST | TRACE_REQUEST_DETAIL))
    {
        trace_printf("%s@%d - Node add request (%s), "
                     "node_name=%s, domain_name=%s, "
                     "first_core=%d, last_core=%d, "
                     "processors=%d, roles=%d\n"
                    , method_name, __LINE__
                    , requester ? requester->GetName() : ""
                    , node_name
                    , domain_name
                    , first_core_
                    , last_core_
                    , processors_
                    , roles_ );
    }

    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();

    // Get the next pnid and nid available for assignment
    nid  = clusterConfig->GetNextNid();
    pnid = clusterConfig->GetNextPNid();

    // Insert node in configuration database and
    // add to configuration object in monitor
    if (clusterConfig->SaveNodeConfig( node_name
                                     , domain_name
                                     , nid
                                     , pnid
                                     , first_core_
                                     , last_core_
                                     , processors_
                                     , -1 // excludedFirstCore
                                     , -1 // excludedLastCore
                                     , roles_ ))
    {
        ZClient->ConfiguredZNodeCreate( node_name);

        // lock sync thread since we are making a change the monitor's
        // operational view of the cluster
        if ( !Emulate_Down )
        {
            Monitor->EnterSyncCycle();
        }

        // Add node to monitor's view of cluster and broadcast node added notice
        if (!Monitor->ReinitializeConfigCluster( true, pnid ))
        {
            rc = MPI_ERR_INTERN;
        }

        // unlock sync thread
        if ( !Emulate_Down )
        {
            Monitor->ExitSyncCycle();
        }
    }
    else
    {
        rc = MPI_ERR_IO;
    }

    requester = Nodes->GetProcess( req_nid_
                                 , req_pid_
                                 , req_verifier_ );
    if (requester)
    {
        // Reply to requester
        requester->CompleteRequest( rc );
    }

    TRACE_EXIT;
}

CIntNodeDeleteReq::CIntNodeDeleteReq( int req_nid
                                    , int req_pid
                                    , Verifier_t req_verifier
                                    , int pnid )
                 : CInternalReq()
                 , req_nid_(req_nid)
                 , req_pid_(req_pid)
                 , req_verifier_(req_verifier)
                 , pnid_(pnid)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIT", 4);
}

CIntNodeDeleteReq::~CIntNodeDeleteReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqit", 4);
}

void CIntNodeDeleteReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    snprintf( strBuf, sizeof(strBuf),
              "IntReq(%s) req #=%ld "
              "(pnid=%d)"
            , CReqQueue::intReqType[InternalType_NodeDelete]
            , getId()
            , pnid_ );
    requestString_.assign( strBuf );
}

void CIntNodeDeleteReq::performRequest()
{
    const char method_name[] = "CIntNodeDeleteReq::performRequest";
    TRACE_ENTRY;

    int rc = MPI_SUCCESS;
    CProcess *requester = NULL;

    requester = Nodes->GetProcess( req_nid_
                                 , req_pid_
                                 , req_verifier_ );

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf( "%s@%d - Node delete request (%s), pnid=%d\n"
                    , method_name, __LINE__
                    , requester ? requester->GetName() : ""
                    , pnid_ );

    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    CPNodeConfig   *pnodeConfig =  clusterConfig->GetPNodeConfig( pnid_ ) ;
    if (clusterConfig->DeleteNodeConfig( pnid_ ))
    {
        ZClient->ConfiguredZNodeDelete( pnodeConfig->GetName() );

        // lock sync thread
        if ( !Emulate_Down )
        {
            Monitor->EnterSyncCycle();
        }

        // Reload the static configuration and broadcast node deleted notice
        if (!Monitor->ReinitializeConfigCluster( false, pnid_ ))
        {
            rc = MPI_ERR_INTERN;
        }

        // unlock sync thread
        if ( !Emulate_Down )
        {
            Monitor->ExitSyncCycle();
        }
    }
    else
    {
        rc = MPI_ERR_IO;
    }

    requester = Nodes->GetProcess( req_nid_
                                 , req_pid_
                                 , req_verifier_ );
    if (requester)
    {
        // Reply to requester
        requester->CompleteRequest( rc );
    }

    TRACE_EXIT;
}
#endif

CIntDownReq::CIntDownReq( int pnid )
    : CInternalReq(),
      pnid_ ( pnid )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIP", 4);

    if ( pnid == MyPNID )
    {
        SetReviveFlag(1); // allow this request to be processed during revive
    }
}

CIntDownReq::~CIntDownReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqip", 4);
}

void CIntDownReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (pnid=%d)"
                   , CReqQueue::intReqType[InternalType_Down]
                   , getId(), pnid_ );
    requestString_.assign( strBuf );
}

void CIntDownReq::performRequest()
{
    const char method_name[] = "CIntDownReq::performRequest";
    TRACE_ENTRY;

    const char *tp = getenv( "MON_TP017_NODE_DOWN" );           \
    if ((tp != NULL) && (MyPNID == 6))
    {
        if (trace_settings & TRACE_REQUEST)
            trace_printf("%s@%d - Node down test point, pnid=%d delaying...\n",
                         method_name, __LINE__, MyPNID);

        while ( true )
        {
            sleep(1);
        }
    }

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf("%s@%d - Node down request, pnid=%d\n",
                     method_name, __LINE__, pnid_);
#ifndef NAMESERVER_PROCESS
    Monitor->HardNodeDown( pnid_ );
#else
    Monitor->HardNodeDownNs( pnid_ );
#endif

    TRACE_EXIT;
}

CIntUpReq::CIntUpReq( int pnid, char *node_name, int merge_lead )
    : CInternalReq(),
      nodeName_ ( node_name?node_name:"" ),
      mergeLead_ ( merge_lead ),
      pnid_ ( pnid )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIQ", 4);

    SetReviveFlag(1); // allow this request to be processed during revive
}

CIntUpReq::~CIntUpReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqiq", 4);
}

void CIntUpReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (pnid=%d)"
                   , CReqQueue::intReqType[InternalType_Up]
                   , getId(), pnid_ );
    requestString_.assign( strBuf );
}

void CIntUpReq::performRequest()
{
    const char method_name[] = "CIntUpReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf("%s@%d - Node up request, pnid=%d, name=%s\n",
                     method_name, __LINE__, pnid_, nodeName_.c_str());
    Monitor->HardNodeUp( pnid_, (char *) nodeName_.c_str() );

    TRACE_EXIT;
}

CIntActivateSpareReq::CIntActivateSpareReq(CNode *spareNode, CNode *downNode, bool checkHealth)
    : CInternalReq(),
    spareNode_(spareNode),
    downNode_(downNode),
    checkHealth_(checkHealth)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIM", 4);
}

CIntActivateSpareReq::~CIntActivateSpareReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqim", 4);
}

void CIntActivateSpareReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "Activate Spare (%s) req #=%ld"
                   , CReqQueue::intReqType[InternalType_ActivateSpare]
                   , getId());
    requestString_.assign( strBuf );
}

void CIntActivateSpareReq::performRequest()
{
    const char method_name[] = "CIntActivateSpareReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d - Activating spare pnid=%d, spareNode_=%p, "
                      "downNode_=%p\n"
                    , method_name, __LINE__
                    , spareNode_->GetPNid() , spareNode_, downNode_ );
    }

    if ( downNode_ == NULL )
    {
        Monitor->NodeReady(spareNode_);
    }
    else
    {
        spareNode_->ResetSpareNode();
        Nodes->RemoveFromSpareNodesList( spareNode_ );
        Monitor->ActivateSpare(spareNode_, downNode_, checkHealth_);
    }

    TRACE_EXIT;
    return;
}

CIntReviveReq::CIntReviveReq()
    : CInternalReq()
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIR", 4);

    SetReviveFlag(1); // allow this request to be processed during revive
}

CIntReviveReq::~CIntReviveReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqir", 4);
}

void CIntReviveReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "Revive(%s) req #=%ld"
                   , CReqQueue::intReqType[InternalType_Revive]
                   , getId());
    requestString_.assign( strBuf );
}

void CIntReviveReq::performRequest()
{
    const char method_name[] = "CIntReviveReq::performRequest";
    TRACE_ENTRY;

    int error;

    Monitor->EnterSyncCycle();

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Revive request\n", method_name, __LINE__);

    mem_log_write(MON_REQQUEUE_REVIVE_1);

    CCluster::snapShotHeader_t header;

    switch( CommType )
    {
        case CommType_InfiniBand:
            error = Monitor->ReceiveMPI( (char *)&header
                                       , sizeof(header)
                                       , 0
                                       , MON_XCHNG_HEADER
                                       , Monitor->getJoinComm());
            break;
        case CommType_Sockets:
            error = Monitor->ReceiveSock( (char *)&header
                                         , sizeof(header)
                                         , Monitor->getJoinSock()
                                         , method_name );
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    mem_log_write(MON_REQQUEUE_REVIVE_2, error);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Msg Received - header. Error = %d\n", method_name, __LINE__, error);

    if (error)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unable to receive header. Exiting.", method_name, __LINE__);

        TRACE_EXIT;
        return;
    }

    if (header.compressedSize_ == -1)
    {   // creator monitor ran into compression error, abort.
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Creator monitor compression error. Exiting.", method_name, __LINE__);

        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "Creator monitor had compression error. Aborting node reintegration.\n");
        mon_log_write(MON_REQQUEUE_REVIVE_2, SQ_LOG_CRIT, buf);

        // exit call below runs desctructors. Stop healthcheck thread so that its lock can be destructed.
        HealthCheck.shutdownWork();

        TRACE_EXIT;
        exit(0); // this will cause other monitors to disconnect from the new monitor.
    }

    char *compBuf = (char *) malloc ( header.compressedSize_ );

    if (!compBuf)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unable to allocate buffer of size = %ld\n",
                          method_name, __LINE__, header.compressedSize_);
        TRACE_EXIT;
        return;
    }

    switch( CommType )
    {
        case CommType_InfiniBand:
            error = Monitor->ReceiveMPI( compBuf
                                       , header.compressedSize_
                                       , 0
                                       , MON_XCHNG_DATA
                                       , Monitor->getJoinComm());
            break;
        case CommType_Sockets:
            error = Monitor->ReceiveSock( compBuf
                                        , header.compressedSize_
                                        , Monitor->getJoinSock()
                                        , method_name );
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    mem_log_write(MON_REQQUEUE_REVIVE_3, error);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Msg Received - data. Error = %d\n", method_name, __LINE__, error);

    if (error)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unable to receive data. Exiting.", method_name, __LINE__);

        free( compBuf );

        TRACE_EXIT;
        return;
    }

    char *buf = (char *) malloc ( header.fullSize_ );

    if (!buf)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unable to allocate buffer of size = %ld\n",
                          method_name, __LINE__, header.fullSize_);

        free( compBuf );

        TRACE_EXIT;
        return;
    }

    unsigned long bufLen = header.fullSize_;

    error = uncompress((Bytef *)buf, &bufLen, (Bytef *)compBuf, header.compressedSize_);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - compSize = %ld, fullsize = %ld, uncompress error = %d\n",
                     method_name, __LINE__, header.compressedSize_, header.fullSize_, error);

    free( compBuf ); // don't need anymore. Will work on uncompressed buffer from this point.

    char *buffer = buf;

#ifndef NAMESERVER_PROCESS
    // unpack the current TM leader
    Monitor->SetTmLeader( header.tmLeader_ );

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf( "%s@%d - TM leader (%d) unpacked\n", method_name, __LINE__
                    , Monitor->GetTmLeader() );
#endif

    mem_log_write(MON_REQQUEUE_REVIVE_4);

    Config->Init();

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Config Cluster group unpacked\n", method_name, __LINE__);

    Nodes->UnpackSpareNodesList( (intBuffPtr_t&)buffer, header.spareNodesCount_);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Spare Nodes List unpacked\n", method_name, __LINE__);

    Nodes->UnpackNodeMappings( (intBuffPtr_t&)buffer, header.nodeMapCount_ );

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Node mappings unpacked\n", method_name, __LINE__);

    Nodes->UnpackZids( (intBuffPtr_t&)buffer );

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Node zids unpacked\n", method_name, __LINE__);

    Config->UnpackRegistry(buffer, (header.clusterRegistryCount_ + header.processRegistryCount_));

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Registry unpacked\n", method_name, __LINE__);
       
    Config->UnpackUniqueStrings(buffer, header.uniqueStringCount_);
    
    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Unique Strings unpacked\n", method_name, __LINE__);
       
    // unpack process objects and create clones
    Monitor->UnpackProcObjs(buffer, header.procCount_);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Process Objects unpacked\n", method_name, __LINE__);

    mem_log_write(MON_REQQUEUE_REVIVE_5);

    // process the requests that were deferred to the revive side queue.
    ReqQueue.processReviveRequests(header.seqNum_);

    mem_log_write(MON_REQQUEUE_REVIVE_6);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Queued revive requests processed\n", method_name, __LINE__);

    free( buf );

    // done with joining.
    // we are in the new monitor, and this will drive the state change
    MyNode->SetChangeState( true );

    Monitor->ExitSyncCycle();

    TRACE_EXIT;

    return;
}

CIntSnapshotReq::CIntSnapshotReq( unsigned long long seqNum )
    : CInternalReq(),
      seqNum_( seqNum )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIG", 4);
}

CIntSnapshotReq::~CIntSnapshotReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqig", 4);
}

void CIntSnapshotReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "Snapshot(%s) req #=%ld"
                   , CReqQueue::intReqType[InternalType_Snapshot]
                   , getId());
    requestString_.assign( strBuf );
}

void CIntSnapshotReq::performRequest()
{
    const char method_name[] = "CIntSnapshotReq::performRequest";
    TRACE_ENTRY;

    char *snapshotBuf = NULL;
    char *compBuf = NULL;
    unsigned long size = 0;
    unsigned long compSize = 0;
    int z_result = 0;
    struct timespec startTime, snapShotTime, compressTime;
    int error = 0;

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Snapshot request\n", method_name, __LINE__);

    mem_log_write(MON_REQQUEUE_SNAPSHOT_1);

    // abort this request if join communication is not setup
    switch( CommType )
    {
        case CommType_InfiniBand:
            if (Monitor->getJoinComm() == MPI_COMM_NULL)
            {
                mem_log_write(MON_REQQUEUE_SNAPSHOT_2);

                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], Join communicator is null, aborting snapshot req.\n", method_name);
                mon_log_write(MON_REQQUEUE_SNAPSHOT_2, SQ_LOG_ERR, buf);

                TRACE_EXIT;
                return;
            }
            break;
        case CommType_Sockets:
            if (Monitor->getJoinSock() == -1)
            {
                mem_log_write(MON_REQQUEUE_SNAPSHOT_2);

                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], Join socket is -1, aborting snapshot req.\n", method_name);
                mon_log_write(MON_REQQUEUE_SNAPSHOT_2, SQ_LOG_ERR, buf);

                TRACE_EXIT;
                return;
            }
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    // estimate size of snapshot buffer
    // about 500 bytes per process, 2 times total
    int procSize = Nodes->ProcessCount() * 2 * 500;
    int idsSize = Nodes->GetSNodesCount() * sizeof(int); // spare pnids
    idsSize += (Nodes->GetPNodesCount() + Nodes->GetLNodesCount()) * sizeof(int); // pnid/nid map
    idsSize += Nodes->GetPNodesCount() * 2 * sizeof(int);    // pnid/zid
    idsSize += Nodes->GetLNodesCount() * sizeof(int);    // nids
    idsSize += Config->getUniqueStringsSize();
    idsSize += Config->getRegistrySize();

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Snapshot sizes, procSize = %d, idsSize = %d\n",
                      method_name, __LINE__, procSize, idsSize);

    mem_log_write(MON_REQQUEUE_SNAPSHOT_4, procSize, idsSize);

    snapshotBuf = (char *) malloc (procSize + idsSize);

    if (!snapshotBuf)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unable to allocate snapshot buffer, size = %ld\n",
                 method_name, __LINE__, size);

        mem_log_write(MON_REQQUEUE_SNAPSHOT_5);

        TRACE_EXIT;
        return;
    }

    clock_gettime(CLOCK_REALTIME, &startTime);

    memset( snapshotBuf, 0, (procSize + idsSize) );
    char *buf = snapshotBuf;

    CCluster::snapShotHeader_t header;

#ifndef NAMESERVER_PROCESS
    // pack the current TM leader
    header.tmLeader_ = Monitor->GetTmLeader();
    
    // pack license verifiers
    for (int index = 0; index < 3; index++)
    {
      header.verifiers_[index] = -1;
    }
#endif

    // pack spareNodes pnids
    header.spareNodesCount_ = Nodes->PackSpareNodesList( (intBuffPtr_t&)buf );

    // pack logical-to-physical nid mappings
    header.nodeMapCount_ = Nodes->PackNodeMappings( (intBuffPtr_t&)buf );

    Nodes->PackZids( (intBuffPtr_t&)buf );
    
    header.clusterRegistryCount_ =  Config->PackRegistry(buf, ConfigType_Cluster);
    header.processRegistryCount_ =  Config->PackRegistry(buf, ConfigType_Process);
    header.uniqueStringCount_   =  Config->PackUniqueStrings(buf);
    
    // pack process objects
    header.procCount_ = Monitor->PackProcObjs(buf);

    mem_log_write(MON_REQQUEUE_SNAPSHOT_6, header.nodeMapCount_, header.procCount_);

    header.fullSize_ = buf - snapshotBuf;

    mem_log_write(MON_REQQUEUE_SNAPSHOT_7, header.fullSize_);

    // the seq num in the request reflects the state of the cluster at this point.
    // do not use the current seq num as it may have advanced.
    header.seqNum_ = seqNum_; // copy the one stored in this request

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - procCount = %ld, fullSize = %ld\n",
             method_name, __LINE__, header.procCount_, header.fullSize_);

    clock_gettime(CLOCK_REALTIME, &snapShotTime);

    // compress call requires the compression buffer to be little more than the input buffer.
    compSize = compressBound(header.fullSize_);

    compBuf = (char *)malloc(compSize);

    if (!compBuf)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unable to allocate compression buffer, size = %ld\n",
                 method_name, __LINE__, compSize);

        free( snapshotBuf );

        mem_log_write(MON_REQQUEUE_SNAPSHOT_8);

        TRACE_EXIT;
        return;
    }

    memset( compBuf, 0, compSize ); // TODO: WHY?
    z_result = compress((Bytef *)compBuf, (unsigned long *)&compSize,
                        (Bytef *)snapshotBuf, header.fullSize_);

    mem_log_write(MON_REQQUEUE_SNAPSHOT_9, z_result, compSize);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - compression result = %d, orig size = %ld, comp size = %ld\n",
                     method_name, __LINE__, z_result, header.fullSize_, compSize);

    if (z_result != Z_OK)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "Snapshot buffer compression error = %d, aborting node reintegration.\n", z_result);
        mon_log_write (MON_REQQUEUE_SNAPSHOT_14, SQ_LOG_CRIT, buf);

        // send msg to new monitor so that it can exit
        header.compressedSize_ = -1;
        switch( CommType )
        {
            case CommType_InfiniBand:
                error = Monitor->SendMPI( (char *)&header
                                        , sizeof(header)
                                        , 0
                                        , MON_XCHNG_HEADER
                                        , Monitor->getJoinComm());
                break;
            case CommType_Sockets:
                error = Monitor->SendSock( (char *)&header
                                         , sizeof(header)
                                         , Monitor->getJoinSock()
                                         , method_name );
                break;
            default:
                // Programmer bonehead!
                abort();
        }
        if (error) {
            sprintf(buf, "Unable to send exit msg to new monitor, error = %d\n", error);
            mon_log_write(MON_REQQUEUE_SNAPSHOT_15, SQ_LOG_CRIT, buf);
        }

        sprintf(buf, "Node reintegration aborted due to buffer compression error.");
#ifndef NAMESERVER_PROCESS
        SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                               , MyNode->GetCreatorVerifier()
                                               , Monitor->ReIntegErrorMessage( buf )
                                               , NULL );
#endif
        TRACE_EXIT;
        return;
    }
    header.compressedSize_ = compSize;
    header.compressedSize_ = compSize;
    clock_gettime(CLOCK_REALTIME, &compressTime);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - snapshot times, start = %ld.%ld, end = %ld.%ld compressed = %ld.%ld\n",
                    method_name, __LINE__, startTime.tv_sec, startTime.tv_nsec,
                    snapShotTime.tv_sec, snapShotTime.tv_nsec, compressTime.tv_sec, compressTime.tv_nsec);

    free( snapshotBuf ); // don't need anymore. Will work on compressed buffer from this point.

    mem_log_write(MON_REQQUEUE_SNAPSHOT_10);

    switch( CommType )
    {
        case CommType_InfiniBand:
            error = Monitor->SendMPI( (char *)&header
                                    , sizeof(header)
                                    , 0
                                    , MON_XCHNG_HEADER
                                    , Monitor->getJoinComm());
            break;
        case CommType_Sockets:
            error = Monitor->SendSock( (char *)&header
                                     , sizeof(header)
                                     , Monitor->getJoinSock()
                                     , method_name );
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    mem_log_write(MON_REQQUEUE_SNAPSHOT_11, error);

    if (error)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unable to send header. Exiting.", method_name, __LINE__);

        free( compBuf );

        TRACE_EXIT;
        return;
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Msg Sent - header. Error = %d\n", method_name, __LINE__, error);

    switch( CommType )
    {
        case CommType_InfiniBand:
            error = Monitor->SendMPI( compBuf
                                    , header.compressedSize_
                                    , 0
                                    , MON_XCHNG_DATA
                                    , Monitor->getJoinComm());
            break;
        case CommType_Sockets:
            error = Monitor->SendSock( compBuf
                                     , header.compressedSize_
                                     , Monitor->getJoinSock()
                                     , method_name );
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    mem_log_write(MON_REQQUEUE_SNAPSHOT_12, header.compressedSize_, error);

    if (error)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unable to send data. Exiting.", method_name, __LINE__);

        free( compBuf );

        TRACE_EXIT;
        return;
    }

    if (trace_settings & TRACE_REQUEST)
        trace_printf("%s@%d - Msg Sent - data. Error = %d\n", method_name, __LINE__, error);

    free( compBuf );

    mem_log_write(MON_REQQUEUE_SNAPSHOT_13);

    TRACE_EXIT;
    return;
}

CQuiesceReq::CQuiesceReq( )
    : CInternalReq()
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIU", 4);
}

CQuiesceReq::~CQuiesceReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqiu", 4);
}

void CQuiesceReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "Quiece(%s) req #=%ld"
                   , CReqQueue::intReqType[InternalType_Quiesce]
                   , getId());
    requestString_.assign( strBuf );
}

void CQuiesceReq::performRequest()
{
    const char method_name[] = "CQuiesceReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf("%s@%d - Quiece request\n",
                     method_name, __LINE__);

    if ( Monitor->isMonSyncResponsive() )
    {
        Monitor->CompleteSyncCycle(); // let other nodes know that we are in quiesce state
    }

#ifndef NAMESERVER_PROCESS
    MyNode->SendQuiescingNotices();
#endif

    char buf[MON_STRING_BUF_SIZE];
    sprintf(buf, "[%s], Quiesce notices sent.\n", method_name);
    mon_log_write(MON_REQQUEUE_QUIESCE_1, SQ_LOG_WARNING, buf);

    // if nothing in exit list, schedule a node down.
    // if not, node down will be scheduled when exit list becomes empty.
#ifndef NAMESERVER_PROCESS
    if (MyNode->getNumQuiesceExitPids() == 0)
    {
#endif
        if (trace_settings & (TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
            trace_printf("%s@%d - Scheduling node down\n", method_name, __LINE__);
        HealthCheck.setState(MON_SCHED_NODE_DOWN);
#ifndef NAMESERVER_PROCESS
    }
#endif

    TRACE_EXIT;
}

CPostQuiesceReq::CPostQuiesceReq( )
    : CInternalReq()
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIB", 4);
}

CPostQuiesceReq::~CPostQuiesceReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqib", 4);
}

void CPostQuiesceReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "PostQuiece(%s) req #=%ld"
                   , CReqQueue::intReqType[InternalType_PostQuiece]
                   , getId());
    requestString_.assign( strBuf );
}

void CPostQuiesceReq::performRequest()
{
    const char method_name[] = "CPostQuiesceReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf("%s@%d - Post Quiece request\n",
                     method_name, __LINE__);

    if( !getenv("SQ_VIRTUAL_NODES") )
    {
        // Execute node fail safe via the Watchdog process
        HealthCheck.setState(MON_NODE_DOWN);
        // wait forever
        for (;;)
            sleep(10000);
    }
    else
    {
        // Stop all processes
#ifndef NAMESERVER_PROCESS
        Monitor->HardNodeDown( MyPNID );
#else
        Monitor->HardNodeDownNs( MyPNID );
#endif
#ifndef NAMESERVER_PROCESS
        MyNode->EmptyQuiescingPids();
#endif
        // now stop the Watchdog process
        HealthCheck.setState(MON_NODE_DOWN);
        // and tell the cluster the node is down
        CReplNodeDown *repl = new CReplNodeDown(MyPNID);
        Replicator.addItem(repl);
    }
    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
CIntCreatePrimitiveReq::CIntCreatePrimitiveReq( int pnid )
                       :CInternalReq()
                       ,pnid_ ( pnid )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIW", 4);
}

CIntCreatePrimitiveReq::~CIntCreatePrimitiveReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqiw", 4);
}

void CIntCreatePrimitiveReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (pnid=%d)"
                   , CReqQueue::intReqType[InternalType_CreatePrimitives]
                   , getId(), pnid_ );
    requestString_.assign( strBuf );
}

void CIntCreatePrimitiveReq::performRequest()
{
    const char method_name[] = "CIntCreatePrimitiveReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf("%s@%d - Create Primitive Processes request, pnid=%d\n",
                     method_name, __LINE__, pnid_);
    if ( pnid_ == MyPNID )
    {
        if ( NameServerEnabled )
        {
            bool startNs = false;
            if ( IsRealCluster )
            {
                CNameServerConfig *config;
                config = NameServerConfig->GetConfig( MyNode->GetName() );
                if ( config )
                    startNs = true;
            }
            else
            {
                startNs = true;
            }
            if ( startNs )
            {
                NameServer->SetLocalHost();
                MyNode->StartNameServerProcess();
            }
        }
        MyNode->StartWatchdogProcess();
        MyNode->StartPStartDProcess();
        char *env = getenv( "MON_DTM_PRIMITIVE_DISABLE" );
        if ( env == NULL || (env && strcmp( env, "0" ) == 0) )
        {
            MyNode->StartDtmProcess();
        }
        env = getenv( "SQ_SEAMONSTER" );
        if ( env && strcmp( env, "1" ) == 0 )
        {
            MyNode->StartSMServiceProcess();
        }
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
CIntTmReadyReq::CIntTmReadyReq( int nid )
               :CInternalReq()
               ,nid_ ( nid )
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQIV", 4);
}

CIntTmReadyReq::~CIntTmReadyReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqiv", 4);
}

void CIntTmReadyReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2];
    sprintf( strBuf, "IntReq(%s) req #=%ld (nid=%d)"
                   , CReqQueue::intReqType[InternalType_TmReady]
                   , getId(), nid_ );
    requestString_.assign( strBuf );
}

void CIntTmReadyReq::performRequest()
{
    const char method_name[] = "CIntTmReadyReq::performRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
        trace_printf("%s@%d - TM ready request, pnid=%d\n",
                     method_name, __LINE__, nid_);

    Monitor->NodeTmReady( nid_ );

    TRACE_EXIT;
}
#endif

//
CReqQueue::CReqQueue(): busyExclusive_(false), busyWorkers_(0), syncDependentRequests_(0), maxQueueSize_(0),
                        maxBusyWorkers_(0), numRequests_(0), execTimeMax_(0)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "REQQ", 4);
    mostRecentStart_.tv_sec = 0;
    mostRecentStart_.tv_nsec = 0;
}

CReqQueue::~CReqQueue()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "reqq", 4);
}


CExternalReq *CReqQueue::prepExternalReq(CExternalReq::reqQueueMsg_t msgType,
                                         int nid, int pid, int sockFd,
                                         struct message_def *msg)
{
    const char method_name[] = "CReqQueue::prepExternalReq";
    TRACE_ENTRY;

    CExternalReq * request = NULL;


    if (msg && msg->type == MsgType_Service)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d from pid=%d, request=%s, lio msg type=%d\n",
                     method_name, __LINE__, pid,
                     svcReqType[msg->u.request.type], msgType);

        switch (msg->u.request.type)
        {
#ifdef NAMESERVER_PROCESS
        case ReqType_DelProcessNs:
            request = new CExtDelProcessNsReq(msgType, nid, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NameServerStart:
            request = new CExtNameServerStartNsReq(msgType, nid, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NameServerStop:
            request = new CExtNameServerStopNsReq(msgType, nid, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NodeDown:
            request = new CExtNodeDownNsReq(msgType, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NewProcessNs:
            request = new CExtNewProcNsReq(msgType, nid, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_ProcessInfo:
            request = new CExtProcInfoReq(msgType, nid, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_ProcessInfoCont:
            request = new CExtProcInfoContReq(msgType, nid, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_ProcessInfoNs:
            request = new CExtProcInfoNsReq(msgType, nid, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_ShutdownNs:
            request = new CExtShutdownNsReq(msgType, nid, pid, sockFd, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

#else
        case ReqType_Close:
            // No work to do for "close" (obsolete request).  Reply
            // with success.
            request = new CExtNullReq(msgType, nid, pid, -1, msg);
            request->errorReply( MPI_SUCCESS );
            delete request;
            request = NULL;
            break;

        case ReqType_Dump:
            request = new CExtDumpReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_ProcessInfo:
            request = new CExtProcInfoReq(msgType, nid, pid, -1, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_ProcessInfoCont:
            request = new CExtProcInfoContReq(msgType, nid, pid, -1, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NodeInfo:
            request = new CExtNodeInfoReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_PNodeInfo:
            request = new CExtPNodeInfoReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_Set:
            request = new CExtSetReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_Get:
            request = new CExtGetReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_InstanceId:
            request = new CExtInstanceIdReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NewProcess:
            request = new CExtNewProcReq(msgType, nid, pid, -1, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_Kill:
            request = new CExtKillReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_Exit:
            request = new CExtExitReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_Notify:
            request = new CExtNotifyReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_MonStats:
            request = new CExtMonStatsReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_Open:
            request = new CExtOpenReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_Startup:
            if (msgType == CExternalReq::AttachStartupMsg)
            {
                request = new CExtAttachStartupReq ( msgType, pid, msg );
            }
            else
            {
                request = new CExtStartupReq ( msgType, pid, msg );
            }
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        // The following request types are not executed concurrently
        // so ownership does not need to be asserted.
        case ReqType_Event:
            request = new CExtEventReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_Mount:
            request = new CExtMountReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NameServerAdd:
            request = new CExtNameServerAddReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NameServerDelete:
            request = new CExtNameServerDeleteReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NameServerStart:
            request = new CExtNameServerStartReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NameServerStop:
            request = new CExtNameServerStopReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NodeAdd:
            request = new CExtNodeAddReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NodeDelete:
            request = new CExtNodeDeleteReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NodeDown:
            request = new CExtNodeDownReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NodeName:
            request = new CExtNodeNameReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_NodeUp:
            request = new CExtNodeUpReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;
#if 0
        case ReqType_PersistAdd:
            request = new CExtPersistAddReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_PersistDelete:
            request = new CExtPersistDeleteReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;
#endif
        case ReqType_Shutdown:
            request = new CExtShutdownReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_TmLeader:
            request = new CExtTmLeaderReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_TmReady:
            request = new CExtTmReadyReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;
        case ReqType_ZoneInfo:
            request = new CExtZoneInfoReq(msgType, pid, msg);
            request->setConcurrent(reqConcurrent[msg->u.request.type]);
            break;

        case ReqType_OpenInfo:
        case ReqType_Notice:
#endif
        default:
            // Invalid request type
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d invalid request type\n",
                             method_name, __LINE__);
            // Send error reply
            request = new CExtNullReq(msgType, nid, pid, sockFd, msg);
            request->errorReply( MPI_ERR_REQUEST );
            delete request;
            request = NULL;
        }
    }

    else if (msgType == CExternalReq::ShutdownWork)
    {
        request = new CExtNullReq(msgType, nid, pid, sockFd, msg);
        request->setConcurrent(true);
    }
    else
    {
        if ( trace_settings & TRACE_REQUEST )
            trace_printf("%s@%d Unknown message type=%d\n",
                         method_name, __LINE__,
                         ((msg != NULL) ? msg->type : -1));

        // Send error reply
        request = new CExtNullReq(msgType, nid, pid, sockFd, msg);
        request->errorReply( MPI_ERR_REQUEST );
        delete request;
        request = NULL;
    }

    TRACE_EXIT;
    return request;
}

// Enqueue an external request
void CReqQueue::enqueueReq(CExternalReq::reqQueueMsg_t msgType,
                           int nid, int pid, int sockFd,
                           struct message_def *msg)
{
    const char method_name[] = "CReqQueue::enqueueReq (ext)";
    TRACE_ENTRY;

    MemModLock.lock();

    CExternalReq *extReq;
    extReq = prepExternalReq(msgType, nid, pid, sockFd, msg);

    if (extReq)
    {   // Have a valid external request
        reqQueueLock_.lock();

        extReq->setId ( numRequests_ );
        reqQueue_.push_back (extReq);

        if ( trace_settings & TRACE_REQUEST )
            trace_printf("%s@%d queueing request #%ld\n", method_name,
                         __LINE__, numRequests_);

        // Maintain statistics
        ++numRequests_;
        int listSize = reqQueue_.size();
        if (listSize > maxQueueSize_)
            maxQueueSize_ = listSize;

        // Record statistics (sonar counters)
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->ReqQueueIncr();

        // Since there is a new request, possibly wake up a worker thread
        // to work on it.
        if (extReq->isShutdown())
            reqQueueLock_.wakeAll();
        else
            reqQueueLock_.wakeOne();

        reqQueueLock_.unlock();
    }

    MemModLock.unlock();

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueCloneReq ( struct clone_def *cloneDef )
{
    CInternalReq * request;

    request = new CIntCloneProcReq( cloneDef->backup
                                  , cloneDef->unhooked
                                  , cloneDef->event_messages
                                  , cloneDef->system_messages
                                  , cloneDef->nid
                                  , cloneDef->type
                                  , cloneDef->priority
                                  , cloneDef->parent_nid
                                  , cloneDef->parent_pid
                                  , cloneDef->parent_verifier
                                  , cloneDef->os_pid
                                  , cloneDef->verifier
                                  , cloneDef->prior_pid
                                  , cloneDef->persistent_retries
                                  , cloneDef->argc
                                  , cloneDef->creation_time
                                  , cloneDef->pathStrId
                                  , cloneDef->ldpathStrId
                                  , cloneDef->programStrId
                                  , cloneDef->nameLen
                                  , cloneDef->portLen
                                  , cloneDef->infileLen
                                  , cloneDef->outfileLen
                                  , cloneDef->argvLen
                                  , &cloneDef->stringData
                                  , cloneDef->origPNidNs);
    enqueueReq ( request );
}
#endif

#ifdef NAMESERVER_PROCESS
void CReqQueue::enqueueCloneReq ( struct clone_def *cloneDef )
{
    CInternalReq * request;

    request = new CIntCloneProcNsReq( cloneDef->backup
                                    , cloneDef->unhooked
                                    , cloneDef->event_messages
                                    , cloneDef->system_messages
                                    , cloneDef->nid
                                    , cloneDef->type
                                    , cloneDef->priority
                                    , cloneDef->parent_nid
                                    , cloneDef->parent_pid
                                    , cloneDef->parent_verifier
                                    , cloneDef->os_pid
                                    , cloneDef->verifier
                                    , cloneDef->prior_pid
                                    , cloneDef->persistent_retries
                                    , cloneDef->argc
                                    , cloneDef->creation_time
                                    , cloneDef->pathLen
                                    , cloneDef->ldpathLen
                                    , cloneDef->programLen
                                    , cloneDef->nameLen
                                    , cloneDef->portLen
                                    , cloneDef->infileLen
                                    , cloneDef->outfileLen
                                    , cloneDef->argvLen
                                    , &cloneDef->stringData
                                    , cloneDef->origPNidNs);
    enqueueReq ( request );
}
#endif

void CReqQueue::enqueueActivateSpareReq (CNode *spareNode, CNode *downNode, bool checkHealth )
{
    CInternalReq * request;

    request = new CIntActivateSpareReq ( spareNode, downNode, checkHealth );

//    request->setPriority(CRequest::High);

    enqueueReq ( request );
}

void CReqQueue::enqueueReviveReq ()
{
    CInternalReq * request;

    request = new CIntReviveReq ( );

    enqueueReq ( request );
}

void CReqQueue::enqueueSnapshotReq (unsigned long long seqnum)
{
    CInternalReq * request;

    request = new CIntSnapshotReq ( seqnum );

    enqueueReq ( request );
}

void CReqQueue::enqueueQuiesceReq ()
{
    CInternalReq * request;

    request = new CQuiesceReq ( );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}

void CReqQueue::enqueuePostQuiesceReq ()
{
    CInternalReq * request;

    request = new CPostQuiesceReq ( );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueDeviceReq ( char *ldevName )
{
    CInternalReq * request;

    request = new CIntDeviceReq ( ldevName );

    enqueueReq ( request );
}
#endif

void CReqQueue::enqueueNameServerAddReq( int req_nid
                                       , int req_pid
                                       , Verifier_t req_verifier
                                       , char *node_name
                                       )
{
    CInternalReq * request;

    request = new CIntNameServerAddReq( req_nid
                                      , req_pid
                                      , req_verifier
                                      , node_name
                                      );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}

void CReqQueue::enqueueNameServerDeleteReq( int req_nid
                                          , int req_pid
                                          , Verifier_t req_verifier
                                          , char *node_name )
{
    CInternalReq * request;

    request = new CIntNameServerDeleteReq( req_nid
                                         , req_pid
                                         , req_verifier
                                         , node_name );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueNodeAddReq( int req_nid
                                 , int req_pid
                                 , Verifier_t req_verifier
                                 , char *node_name
                                 , int firstCore
                                 , int lastCore
                                 , int processors
                                 , int roles
                                 )
{
    CInternalReq * request;

    request = new CIntNodeAddReq( req_nid
                                , req_pid
                                , req_verifier
                                , node_name
                                , firstCore
                                , lastCore
                                , processors
                                , roles
                                );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}

void CReqQueue::enqueueNodeDeleteReq( int req_nid
                                    , int req_pid
                                    , Verifier_t req_verifier
                                    , int pnid )
{
    CInternalReq * request;

    request = new CIntNodeDeleteReq( req_nid
                                   , req_pid
                                   , req_verifier
                                   , pnid );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}
#endif

void CReqQueue::enqueueDownReq( int pnid )
{
    CInternalReq * request;

    request = new CIntDownReq ( pnid );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}

void CReqQueue::enqueueNodeNameReq( int req_nid
                                  , int req_pid
                                  , Verifier_t req_verifier
                                  , char *current_name
                                  , char *new_name)
{
    CInternalReq * request;

    request = new CIntNodeNameReq( req_nid
                                 , req_pid
                                 , req_verifier
                                 , current_name
                                 , new_name );

    enqueueReq ( request );
}

void CReqQueue::enqueueShutdownReq( int level )
{
    CInternalReq * request;

    request = new CIntShutdownReq ( level );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}

void CReqQueue::enqueueUpReq( int pnid, char *node_name, int merge_lead )
{
    CInternalReq * request;

    request = new CIntUpReq ( pnid, node_name, merge_lead );


    enqueueReq ( request );
}

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueDumpCompleteReq( struct dump_def *dumpDef )
{
    CIntDumpCompleteReq * request;

    request = new CIntDumpCompleteReq();
    request->prepRequest( dumpDef );

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueDumpReq( struct dump_def *dumpDef )
{
    CIntDumpReq * request;

    request = new CIntDumpReq();
    request->prepRequest( dumpDef );

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueEventReq( struct event_def *eventDef )
{
    CIntEventReq * request;

    request = new CIntEventReq();
    request->prepRequest( eventDef );

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueExitReq( struct exit_def *exitDef )
{
    CIntExitReq * request;

    request = new CIntExitReq ( );
    request->prepRequest( exitDef );

    enqueueReq ( request );
}
#else
void CReqQueue::enqueueExitNsReq( struct exit_ns_def *exitDef )
{
    CIntExitNsReq * request;

    request = new CIntExitNsReq ( );
    request->prepRequest( exitDef );

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueIoDataReq( ioData_t *ioData )
{
    CInternalReq * request;

    request = new CIntIoDataReq ( ioData );

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueKillReq( struct kill_def *killDef )
{
    CInternalReq * request;

    request = new CIntKillReq ( killDef );

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueNewProcReq( struct process_def *procDef )
{
    CIntNewProcReq * request;

    request = new CIntNewProcReq( procDef->nid
                                , procDef->type
                                , procDef->priority
                                , procDef->backup
                                , procDef->parent_nid
                                , procDef->parent_pid
                                , procDef->parent_verifier
                                , procDef->pair_parent_nid
                                , procDef->pair_parent_pid
                                , procDef->pair_parent_verifier
                                , procDef->argc
                                , procDef->unhooked
                                , procDef->tag
                                , procDef->pathStrId
                                , procDef->ldpathStrId
                                , procDef->programStrId
                                , procDef->nameLen
                                , procDef->infileLen
                                , procDef->outfileLen
                                , procDef->argvLen
                                , &procDef->stringData );

    enqueueReq ( request );
}
#endif

#ifdef NAMESERVER_PROCESS
void CReqQueue::enqueueNewProcNsReq( struct process_def *procDef )
{
    CIntNewProcNsReq* request;

    request = new CIntNewProcNsReq( procDef->nid
                                , procDef->pid
                                , procDef->verifier
                                , procDef->type
                                , procDef->priority
                                , procDef->backup
                                , procDef->parent_nid
                                , procDef->parent_pid
                                , procDef->parent_verifier
                                , procDef->pair_parent_nid
                                , procDef->pair_parent_pid
                                , procDef->pair_parent_verifier
                                , procDef->argc
                                , procDef->unhooked
                                , procDef->tag
                                , procDef->pathLen
                                , procDef->ldpathLen
                                , procDef->programLen
                                , procDef->nameLen
                                , procDef->infileLen
                                , procDef->outfileLen
                                , procDef->argvLen
                                , &procDef->stringData );

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueNotifyReq( struct notify_def *notifyDef )
{
    CIntNotifyReq * request;

    request = new CIntNotifyReq( notifyDef );

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueOpenReq( struct open_def *openDef )
{
    CIntOpenReq * request;

    request = new CIntOpenReq( openDef );

    enqueueReq ( request );
}
#endif

void CReqQueue::enqueueProcInitReq( struct process_init_def *procInitDef )
{
    CIntProcInitReq * request;

    request = new CIntProcInitReq( procInitDef );

    enqueueReq ( request );
}

void CReqQueue::enqueueSetReq( struct set_def *setDef )
{
    CIntSetReq * request;

    request = new CIntSetReq (setDef->type, setDef->group,
                              setDef->key, &setDef->valueData);

    enqueueReq ( request );
}

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueStdInReq( struct stdin_req_def *stdin_req )
{
    CInternalReq * request;

    request = new CIntStdInReq ( stdin_req );

    enqueueReq ( request );
}
#endif

void CReqQueue::enqueueUniqStrReq( struct uniqstr_def *uniqStrDef )
{
    CIntUniqStrReq * request;

    request = new CIntUniqStrReq (uniqStrDef->nid, uniqStrDef->id,
                                  &uniqStrDef->valueData);

    enqueueReq ( request );
}

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueChildDeathReq( pid_t pid )
{
    CIntChildDeathReq * request;

    request = new CIntChildDeathReq ( pid );
    // queue the request to be handled later by worker thread
    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueAttachedDeathReq( pid_t pid )
{
    CIntAttachedDeathReq * request;

    request = new CIntAttachedDeathReq ( pid );
    // queue the request to be handled later by worker thread
    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueCreatePrimitiveReq( int pnid )
{
    CInternalReq * request;

    request = new CIntCreatePrimitiveReq( pnid );

    request->setPriority(CRequest::High);

    enqueueReq ( request );
}
#endif

#ifndef NAMESERVER_PROCESS
void CReqQueue::enqueueTmReadyReq( int nid )
{
    CInternalReq * request;

    request = new CIntTmReadyReq( nid );

    enqueueReq ( request );
}
#endif

// this function moves the queued requests from revive queue to the main request queue.
// it will skip the requests whose seq num is less than the given one.
void CReqQueue::processReviveRequests(unsigned long long minSeqNum)
{
    const char method_name[] = "CReqQueue::processReviveRequests";
    TRACE_ENTRY;

    reqListInt_t::iterator it;

    while ( true )
    {
        reqReviveQueueLock_.lock();
        it = reqReviveQueue_.begin();

        if (it == reqReviveQueue_.end())
        {
            IAmIntegrating = false; // all subsequent requests will go to regular req queue
            IAmIntegrated = true;
            reqReviveQueueLock_.unlock();
            break;
        }

        CInternalReq *request = *it;
        it = reqReviveQueue_.erase (it); // remove the request from the list
        reqReviveQueueLock_.unlock(); // so that Sync thread can keep adding to the revive list

        unsigned long long reqSeqNum = request->getSeqNum();

        // move requests whose seq num is above the minSeqNum, discard others.
        if (reqSeqNum >= minSeqNum)
        {
            enqueueReq( request, true );

            if ( trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY) )
                 trace_printf("%s@%d Req moved from revive to regular queue. "
                              "Req seq num = %llu, Min seq num = %llu\n",
                              method_name, __LINE__, reqSeqNum, minSeqNum);
        }
        else
        {
            delete request;

            if ( trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY) )
                 trace_printf("%s@%d Req discarded from revive queue. "
                              "Req seq num = %llu, Min seq num = %llu\n",
                              method_name, __LINE__, reqSeqNum, minSeqNum);
        }
    }

    TRACE_EXIT;
}

// this function adds new request to the revive queue instead of the regular req queue.
bool CReqQueue::addToReqReviveQueue( CInternalReq *req )
{
    const char method_name[] = "CReqQueue::addToReqReviveQueue";
    TRACE_ENTRY;

    bool result = false; // request not added yet.

    if ( IAmIntegrating )
    {
        reqReviveQueueLock_.lock();

        if ( IAmIntegrating ) // check again because it could have gotten turned off while waiting on lock.
        {
            reqReviveQueue_.push_back(req);

            int qsize = reqReviveQueue_.size();

            if ( trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY) )
                 trace_printf("%s@%d Req added to revive queue. Revive queue size = %d\n",
                      method_name, __LINE__, qsize);

            if ( qsize > MAX_REVIVE_QUEUE_SIZE )
            {
                // remove a request set from the front of the queue.
                // the set is a group of all requests with the same seq num.
                unsigned long long seqnum = reqReviveQueue_.front()->getSeqNum();
                while (seqnum == reqReviveQueue_.front()->getSeqNum())
                {
                    reqReviveQueue_.pop_front(); // calls destructor

                    if ( trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY) )
                        trace_printf("%s@%d Req removed from revive queue. Revive queue size = %d\n",
                             method_name, __LINE__, qsize);
                }
            }

            result = true;
        }

        reqReviveQueueLock_.unlock();
    }

    TRACE_EXIT;

    return result;
}

void CReqQueue::enqueueReq( CInternalReq *req, bool reviveOper )
{
    const char method_name[] = "CReqQueue::enqueueReq (int)";
    TRACE_ENTRY;

    if ( !reviveOper )
    {
        req->setSeqNum ( Monitor->getSeqNum() );

        if ( IAmIntegrating && !req->GetReviveFlag() )
        {
            if ( addToReqReviveQueue(req) )
            {
                TRACE_EXIT;
                return;
            }
        }
    }

    reqQueueLock_.lock();

    req->setId ( numRequests_ );

    if (req->getPriority() == CRequest::High)
    {
        reqList_t::iterator it;
        it = reqQueue_.begin();

        // find first request not at high priority
        while ( it != reqQueue_.end()
             && (*it)->getPriority() == CRequest::High )
        {
            ++it;
        }

        // insert new request before first non-high priority request
        reqQueue_.insert(it, req);
    }
    else
    {
        reqQueue_.push_back (req);
    }

    if ( trace_settings & TRACE_REQUEST )
        trace_printf("%s@%d queueing request #%ld\n", method_name,
                     __LINE__, numRequests_);

    // Maintain statistics
    ++numRequests_;
    int listSize = reqQueue_.size();
    if (listSize > maxQueueSize_)
        maxQueueSize_ = listSize;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->ReqQueueIncr();

    // Since there is a new request, possibly wake up a worker thread
    // to work on it.
    reqQueueLock_.wakeOne();
    reqQueueLock_.unlock();

    TRACE_EXIT;
}


void CReqQueue::nudgeWorker()
{
    const char method_name[] = "CReqQueue::nudgeWorker";
    TRACE_ENTRY;

    reqQueueLock_.lock();
    reqQueueLock_.wakeOne();
    reqQueueLock_.unlock();

    TRACE_EXIT;
}

void CReqQueue::timeDiff ( struct timespec t1, struct timespec t2,
                           struct timespec &tDiff)
{
    if ( (t2.tv_nsec - t1.tv_nsec )  < 0 )
    {
        tDiff.tv_sec = t2.tv_sec - t1.tv_sec - 1;
        tDiff.tv_nsec = 1000000000 + t2.tv_nsec - t1.tv_nsec;
    }
    else
    {
        tDiff.tv_sec = t2.tv_sec - t1.tv_sec;
        tDiff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
}

bool CReqQueue::responsive(struct timespec &curTime)
{
    const char method_name[] = "CReqQueue::responsive";
    TRACE_ENTRY;

    bool result = true;

    if ( busyWorkers_ > 0 )
    {
        struct timespec reqTime;

        // Get difference between request start and now.
        timeDiff ( mostRecentStart_, curTime, reqTime );

        if  ( reqTime.tv_sec > getExecTimeMax() )
        {   // Request has been executing too long
            if (trace_settings & TRACE_REQUEST)
               trace_printf("%s@%d" " - request not responsive, reqTime.tv_sec=%ld > responsive=%d" "\n"
                           , method_name, __LINE__
                           , reqTime.tv_sec, getExecTimeMax());

            result = false;
        }
    }

    TRACE_EXIT;

    return result;
}

CRequest* CReqQueue::getRequest()
{
    const char method_name[] = "CReqQueue::getRequest";
    TRACE_ENTRY;

    reqQueueLock_.lock();

    reqList_t::iterator it;
    it = reqQueue_.begin();

    CRequest *request = NULL;
    CRequest::ReqStatus_t reqStatus;
    while (request == NULL)
    {
        // If necessary, wait until a currently executing exclusive request
        // finishes.
        workerStatusLock_.lock();
        while (busyExclusive_)
        {
            if ( trace_settings & TRACE_REQUEST )
                trace_printf("%s@%d doing workerStatusLock_.wait() waiting for exclusive request to finish\n", method_name, __LINE__);
            workerStatusLock_.wait();
            if ( trace_settings & TRACE_REQUEST )
                trace_printf("%s@%d completed workerStatusLock_.wait() waiting for exclusive request to finish\n", method_name, __LINE__);
        }
        workerStatusLock_.unlock();

        if (it != reqQueue_.end())
        {
            request = *it;
            if ( trace_settings & TRACE_REQUEST )
                trace_printf("%s@%d examining request\n",
                             method_name, __LINE__);

            // bugcatcher, temp call
            request->validateObj();

            if ( (reqStatus = request->okToExecute( )) != CRequest::OkToExec )
            {
                if ( reqStatus == CRequest::Failed )
                {
                    // Take request out of list
                    it = reqQueue_.erase (it);

                    delete request;

                    // Record statistics (sonar counters)
                    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                       MonStats->ReqQueueDecr();
                }
                else
                {
                    // Take request out of request list.  "it" then
                    // points to next element in the queue.
                    it = reqQueue_.erase (it);

                    // Record statistics (sonar counters)
                    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                       MonStats->ReqQueueDecr();

                    if ( trace_settings & TRACE_REQUEST )
                    {
                        trace_printf("%s@%d request #%ld deferred.\n",
                                     method_name, __LINE__, request->getId());
                    }

                    // Put request on deferred request list
                    reqDeferred_.push_back (request);
                }
                request = NULL;
            }
        }
        else
        {   // Wait for a new request to be queued or a currently
            // executing request to complete.
            if ( trace_settings & TRACE_REQUEST )
                trace_printf("%s@%d waiting for request\n", method_name,
                             __LINE__);
            reqQueueLock_.wait();
            if ( trace_settings & TRACE_REQUEST )
                trace_printf("%s@%d finished waiting for request\n",
                             method_name, __LINE__);

            it = reqQueue_.begin();
        }
    }

    if (request && !request->isShutdown())
    {
        // Take request out of list
        reqQueue_.erase (it);

        // Record statistics (sonar counters)
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->ReqQueueDecr();

        workerStatusLock_.lock();

        if ( request->isExclusive() )
        {   // Request needs to run while no other requests are running.
            // Wait until any currently executing requests finish.
            if ( trace_settings & TRACE_REQUEST )
                trace_printf("%s@%d request is exclusive\n",
                             method_name, __LINE__);
            while ( busyWorkers_ > 0 )
            {
                if ( trace_settings & TRACE_REQUEST )
                    trace_printf("%s@%d exclusive request waiting while "
                                 "current requests complete\n",
                                 method_name, __LINE__);
                workerStatusLock_.wait();
            }
            busyExclusive_ = true;
        }

        ++busyWorkers_;
        if (busyWorkers_ > maxBusyWorkers_)
            maxBusyWorkers_ = busyWorkers_;

        // Store start time of most recent request.   Need this to detect
        // if a request ever gets stuck unexpectedly.
        mostRecentStart_ = request->startTime();

        // Store max request execution time. This is needed to detect if it is a lengthy request or not.
        execTimeMax_ = request->getExecTimeMax();

        if ( trace_settings & TRACE_REQUEST )
            trace_printf("%s@%d will work on %s request #%ld, priority = %d, busyWorkers=%d\n", method_name, __LINE__, (request->isExclusive() ? "exclusive" : ""), request->getId(), (int)request->getPriority(), busyWorkers_);

        if ( trace_settings & TRACE_REQUEST_DETAIL )
        {
            request->populateRequestString();
            trace_printf("%s@%d request = %s\n", method_name, __LINE__, request->requestString());
        }

        workerStatusLock_.unlock();

    }

    reqQueueLock_.unlock();

    TRACE_EXIT;

    return request;
}

void CReqQueue::finishRequest(CRequest *request)
{
    const char method_name[] = "CReqQueue::finishRequest";
    TRACE_ENTRY;

    workerStatusLock_.lock();

    busyExclusive_ = false;

    --busyWorkers_;
    if (busyWorkers_ == 0)
    {
        if ( trace_settings & TRACE_REQUEST )
            trace_printf("%s@%d no busy workers, doing workerStatusLock_.wakeAll()\n", method_name, __LINE__);
        workerStatusLock_.wakeAll();
    }
    else
    {
        if ( trace_settings & TRACE_REQUEST )
            trace_printf("%s@%d busyWorkers now=%d\n", method_name, __LINE__, busyWorkers_);
    }

    workerStatusLock_.unlock();

    reqQueueLock_.lock();

    // Release any object ownership acquired at start of request processing.
    request->giveupOwnership();

    if ( !reqDeferred_.empty() )
    {   // Re-queue deferred requests

        if ( trace_settings & TRACE_REQUEST )
        {
            trace_printf("%s@%d re-queueing %d deferred requests\n",
                         method_name, __LINE__, (int) reqDeferred_.size());
        }

        int numDeferred = reqDeferred_.size();

        reqQueue_.insert ( reqQueue_.begin(), reqDeferred_.begin(),
                           reqDeferred_.end() );
        reqDeferred_.erase ( reqDeferred_.begin(), reqDeferred_.end() );

        // Record statistics (sonar counters)
        for (int i=0; i<numDeferred; ++i)
        {
            if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
               MonStats->ReqQueueIncr();
        }
    }

    if (request->isSyncDependent())
    {
        --syncDependentRequests_;
        if ( trace_settings & TRACE_REQUEST )
            trace_printf("%s@%d sync dependent request,  syncDependentRequests_ now=%d\n", method_name, __LINE__, syncDependentRequests_);
        if (syncDependentRequests_ == 0)
        {
            reqQueueLock_.wakeAll();
        }
    }

    reqQueueLock_.unlock();

    request->evalReqPerformance();

    delete request;

    TRACE_EXIT;
}

void CReqQueue::stats()
{
    const char method_name[] = "CReqQueue::stats";
    TRACE_ENTRY;

    if ( trace_settings & (TRACE_REQUEST | TRACE_STATS) )
        trace_printf("%s@%d numRequests=%ld, maxQueueSize=%d, maxBusyWorkers=%d\n", method_name, __LINE__, numRequests_, maxQueueSize_, maxBusyWorkers_);

    TRACE_EXIT;
}

// Definition of which requests can execute concurrently
const bool CReqQueue::reqConcurrent[] = {
   false,    // unused, request types start at 1
   false,    // ReqType_Close
   false,    // ReqType_DelProcessNs
   true,     // ReqType_Dump
   false,    // ReqType_Event
   false,    // ReqType_Exit
   true,     // ReqType_Get
   false,    // ReqType_Kill
   false,    // ReqType_MonStats
   false,    // ReqType_Mount
   false,    // ReqType_NameServerAdd
   false,    // ReqType_NameServerDelete
   false,    // ReqType_NameServerStart
   false,    // ReqType_NameServerStop
   false,    // ReqType_NewProcess
   false,    // ReqType_NewProcessNs
   false,    // ReqType_NodeAdd
   false,    // ReqType_NodeDelete
   false,    // ReqType_NodeDown
   false,    // ReqType_NodeInfo
   false,    // ReqType_NodeName
   false,    // ReqType_NodeUp
   false,    // ReqType_Notice -- not an actual request
   false,    // ReqType_Notify
   true,     // ReqType_Open
   false,    // ReqType_OpenInfo
   false,    // ReqType_PersistAdd
   false,    // ReqType_PersistDelete
   false,    // ReqType_PNodeInfo
   false,    // ReqType_ProcessInfo
   false,    // ReqType_ProcessInfoCont
   false,    // ReqType_ProcessInfoNs
   false,    // ReqType_Set
   false,    // ReqType_Shutdown
   false,    // ReqType_ShutdownNs
   false,    // ReqType_Startup
   false,    // ReqType_TmLeader
   false,    // ReqType_TmReady
   false,    // ReqType_ZoneInfo
   false     // ReqType_Invalid
};

// Request names used for trace output
const char * CReqQueue::svcReqType[] = {
    "INVALID",          // unused, request types start at 1
    "Close",            // ReqType_Close
    "DelProcessNs",     // ReqType_DelProcessNs
    "Dump",             // ReqType_Dump
    "Event",            // ReqType_Event
    "Exit",             // ReqType_Exit
    "Get",              // ReqType_Get
    "Kill",             // ReqType_Kill
    "MonStats",         // ReqType_MonStats
    "Mount",            // ReqType_Mount
    "NameServerAdd",    // ReqType_NameServerAdd
    "NameServerDelete", // ReqType_NameServerDelete
    "NameServerStart",  // ReqType_NameServerStart
    "NameServerStop",   // ReqType_NameServerStop
    "NewProcess",       // ReqType_NewProcess
    "NewProcessNs",     // ReqType_NewProcessNs
    "NodeAdd",          // ReqType_NodeAdd
    "NodeDelete",       // ReqType_NodeDelete
    "NodeDown",         // ReqType_NodeDown
    "NodeInfo",         // ReqType_NodeInfo
    "NodeName",         // ReqType_NodeName
    "NodeUp",           // ReqType_NodeUp
    "Notice",           // ReqType_Notice -- not an actual request
    "Notify",           // ReqType_Notify
    "Open",             // ReqType_Open
    "OpenInfo",         // ReqType_OpenInfo
    "PersistAdd",       // ReqType_PersistAdd
    "PersistDelete",    // ReqType_PersistDelete
    "PNodeInfo",        // ReqType_PNodeInfo
    "ProcessInfo",      // ReqType_ProcessInfo
    "ProcessInfoCont",  // ReqType_ProcessInfoCont
    "ProcessInfoNs",    // ReqType_ProcessInfoNs
    "Set",              // ReqType_Set
    "Shutdown",         // ReqType_Shutdown
    "ShutdownNs",       // ReqType_ShutdownNs
    "Startup",          // ReqType_Startup
    "TmLeader",         // ReqType_TmLeader
    "TmReady",          // ReqType_TmReady
    "ZoneInfo",         // ReqType_ZoneInfo
    "INVALID"           // ReqType_Invalid
};

// Must match internal.h:InternalType
const char * CReqQueue::intReqType[] = {
      "INVALID"           // InternalType_Null
    , "ActivateSpare"     // InternalType_ActivateSpare
    , "Clone"             // InternalType_Clone
    , "CreatePrimitives"  // InternalType_CreatePrimitives
    , "Device"            // InternalType_Device
    , "Down"              // InternalType_Down
    , "Dump"              // InternalType_Dump
    , "DumpComplete"      // InternalType_DumpComplete
    , "Event"             // InternalType_Event
    , "Exit"              // InternalType_Exit
    , "IoData"            // InternalType_IoData
    , "Kill"              // InternalType_Kill
    , "NameServerAdd"     // InternalType_NameServerAdd
    , "NameServerDelete"  // InternalType_NameServerDelete
    , "NodeAdd"           // InternalType_NodeAdd
    , "NodeAdded"         // InternalType_NodeAdded
    , "NodeDelete"        // InternalType_NodeDelete
    , "NodeDeleted"       // InternalType_NodeDeleted
    , "NodeName"          // InternalType_NodeName
    , "Notify"            // InternalType_Notify
    , "Open"              // InternalType_Open
    , "PersistAdd"        // InternalType_PersistAdd
    , "PersistDelete"     // InternalType_PersistDelete
    , "PostQuiesce"       // InternalType_PostQuiece
    , "Process"           // InternalType_Process
    , "ProcessInit"       // InternalType_ProcessInit
    , "Quiesce"           // InternalType_Quiesce
    , "Revive"            // InternalType_Revive
    , "SchedData"         // InternalType_SchedData
    , "Set"               // InternalType_Set
    , "Shutdown"          // InternalType_Shutdown
    , "Snapshot"          // InternalType_Snapshot
    , "StdinReq"          // InternalType_StdinReq
    , "TMReady"           // InternalType_TmReady
    , "Up"                // InternalType_Up
    , "UniqStr"           // InternalType_UniqStr
    , "INVALID"           // InternalType_Invalid
};

