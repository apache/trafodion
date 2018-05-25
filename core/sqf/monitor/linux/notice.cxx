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

#include <stdlib.h>
#include <iostream>

using namespace std;

#include <string.h>

#include "monsonar.h"
#include "montrace.h"
#include "monitor.h"
#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "notice.h"
#include "mlio.h"
#include "replicate.h"
#include "ptpclient.h"
#include "nameserver.h"

extern int trace_level;
extern CMonitor *Monitor;
extern int NameServerEnabled;
extern CNameServer *NameServer;
extern CPtpClient *PtpClient;
extern CNode *MyNode;

extern CNodeContainer *Nodes;
extern CMonStats *MonStats;
extern int MyPNID;
extern CReplicate Replicator;

extern _TM_Txid_External invalid_trans( void );
extern _TM_Txid_External null_trans( void );
extern bool isEqual( _TM_Txid_External trans1, _TM_Txid_External trans2 );
extern bool isNull( _TM_Txid_External transid );
extern bool isInvalid( _TM_Txid_External transid );

CNotice::CNotice( int nid
                , int pid
                , Verifier_t verifier
                , const char *name
                , _TM_Txid_External trans_id
                , CProcess *process )
        : Nid(nid)
        , Pid(pid)
        , verifier_(verifier)
        , name_(name)
        , TransID (trans_id)
        , canceled_(false)
        , Process(process)
        , Next(NULL)
        , Prev(NULL)
{
    const char method_name[] = "CNotice::CNotice";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "NTCE", 4);

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        trace_printf( "%s@%d - On process death of %s (%d, %d:%d) notify "
                      "%s (%d, %d:%d), trans_id=%lld.%lld.%lld.%lld\n"
                    , method_name, __LINE__
                    , process->GetName()
                    , process->GetNid()
                    , process->GetPid()
                    , process->GetVerifier()
                    , name_.c_str()
                    , nid
                    , pid
                    , verifier_
                    , TransID.txid[0]
                    , TransID.txid[1]
                    , TransID.txid[2]
                    , TransID.txid[3]);

    Monitor->IncNoticeCount();

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->NumNoticesIncr();

    TRACE_EXIT;
}

CNotice::~CNotice( void )
{
    const char method_name[] = "CNotice::~CNotice";
    TRACE_ENTRY;
    Monitor->DecrNoticeCount();

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->NumNoticesDecr();

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "ntce", 4);

    TRACE_EXIT;
}

void CNotice::Cancel( void )
{
    const char method_name[] = "CNotice::Cancel";
    TRACE_ENTRY;

    canceled_ = true;
    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
       trace_printf( "%s@%d - Process %s (%d, %d:%d) transid=%lld.%lld.%lld.%lld "
                     "Cancelled notice for %s:%d\n"
                   , method_name, __LINE__
                   , name_.c_str()
                   , Nid
                   , Pid
                   , verifier_
                   , TransID.txid[0]
                   , TransID.txid[1]
                   , TransID.txid[2]
                   , TransID.txid[3]
                   , Process->GetName()
                   , Process->GetVerifier());

    TRACE_EXIT;
}

void CNotice::DeLink (CNotice ** head, CNotice ** tail)
{
    const char method_name[] = "CNotice::DeLink";
    TRACE_ENTRY;

    if (*head == this)
        *head = Next;
    if (*tail == this)
        *tail = Prev;
    if (Prev)
        Prev->Next = Next;
    if (Next)
        Next->Prev = Prev;

    TRACE_EXIT;
}

CNotice *CNotice::GetNext( void )
{
    return Next;
}

CNotice *CNotice::GetNotice( int nid
                           , int pid
                           , Verifier_t verifier
                           , _TM_Txid_External trans_id )
{
    CNotice *entry;
    
    const char method_name[] = "CNotice::GetNotice";
    TRACE_ENTRY;

    for( entry=this; entry; entry=entry->Next )
    {
        if ( (nid == entry->Nid) &&
             (pid == entry->Pid) &&
             (verifier == entry->verifier_) )
        {
            if ( !isNull( trans_id ) )
            {
                if ( isEqual( trans_id, entry->TransID ) )
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }    
    }
    if (entry)
    {
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Found Notice: notify (%d, %d), monitored process %s (%d, %d)\n", method_name, __LINE__, nid, pid, Process->GetName(), Process->GetNid(), Process->GetPid());
    }

    TRACE_EXIT;

    return entry;
}

CNotice *CNotice::Link (CNotice * entry)
{
    const char method_name[] = "CNotice::Link";
    TRACE_ENTRY;
    Next = entry;
    entry->Prev = this;

    TRACE_EXIT;
    return entry;
}

void CNotice::Notify( SQ_LocalIOToClient::bcastPids_t *bcastPids )
{
    CProcess *notify;
    
    const char method_name[] = "CNotice::Notify";
    TRACE_ENTRY;

    if ( canceled_ )
    {
        if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf( "%s@%d - Process death notice for process %s (%d, %d:%d) "
                          "was canceled so not delivered to process %s (%d, %d:%d)\n"
                        , method_name, __LINE__
                        , Process->GetName()
                        , Process->GetNid()
                        , Process->GetPid()
                        , Process->GetVerifier()
                        , name_.c_str()
                        , Nid
                        , Pid
                        , verifier_);
    }
    else
    {
        // find by nid (check node state, check process state, backup is Ok)
        notify = Nodes->GetProcess( Nid
                                  , Pid
                                  , verifier_
                                  , true, true, true );
        if( notify )
        {
            if (!notify->IsClone())
            {
                if (Process->GetType() == ProcessType_DTM &&
                    notify->GetType()  == ProcessType_DTM)
                {
                    // Do nothing, DTM death message is delivered in CProcess::Exit()
                }
                else if (notify->IsSystemMessages() )
                {
                    // Add this process id to the list.
                    SQ_LocalIOToClient::pidVerifier_t pv;
                    pv.pv.pid = notify->GetPid();
                    pv.pv.verifier = notify->GetVerifier();
                    bcastPids->insert( pv.pnv );


                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d - Sending Death message of"
                                      " %s (%d, %d:%d) to %s (%d, %d:%d)\n"
                                    , method_name, __LINE__
                                    , Process->GetName()
                                    , Process->GetNid()
                                    , Process->GetPid()
                                    , Process->GetVerifier()
                                    , notify->GetName()
                                    , notify->GetNid()
                                    , notify->GetPid()
                                    , notify->GetVerifier());
                    }
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d - Death message of %s (%d, %d:%d)" 
                                      " not wanted by %s (%d, %d:%d)\n"
                                    , method_name, __LINE__
                                    , Process->GetName()
                                    , Process->GetNid()
                                    , Process->GetPid()
                                    , Process->GetVerifier()
                                    , notify->GetName()
                                    , notify->GetNid()
                                    , notify->GetPid()
                                    , notify->GetVerifier());
                    }
                }
            }
            else
            {
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                {
                    trace_printf( "%s@%d - Death message of %s (%d, %d:%d)" 
                                  " not processed for clone %s (%d, %d:%d)\n"
                                , method_name, __LINE__
                                , Process->GetName()
                                , Process->GetNid()
                                , Process->GetPid()
                                , Process->GetVerifier()
                                , notify->GetName()
                                , notify->GetNid()
                                , notify->GetPid()
                                , notify->GetVerifier());
                }
            }
        }
        else
        {
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf( "%s@%d - Can't find process %s (%d, %d:%d)\n"
                            , method_name, __LINE__
                            , name_.c_str()
                            , Nid
                            , Pid
                            , verifier_ );
            }
        }
    }
    TRACE_EXIT;
}

void CNotice::NotifyAll( void )
{
    CNotice *entry;
    
    const char method_name[] = "CNotice::NotifyAll";
    TRACE_ENTRY;

    SQ_LocalIOToClient::bcastPids_t *bcastPids
        = new SQ_LocalIOToClient::bcastPids_t;

    // build the broadcast list of processes to notify
    for( entry=this; entry; entry=entry->Next )
    {
        entry->Notify( bcastPids );
    }

    if ( !bcastPids->empty() )
    {
        struct message_def *msg;

        msg = Process->DeathMessage();

        if (bcastPids->size() == 1)
        {
            SQ_LocalIOToClient::bcastPids_t::const_iterator it = bcastPids->begin();
            SQ_LocalIOToClient::pidVerifier_t targetPv;
            targetPv.pnv = *it;
            SQ_theLocalIOToClient->putOnNoticeQueue( targetPv.pv.pid
                                                   , targetPv.pv.verifier
                                                   , msg
                                                   , NULL);
            delete bcastPids;
        }
        else
        {
            if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
               MonStats->deathNoticeBcastIncr(bcastPids->size());

            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d - Sending broadcast process death notice "
                             "to %d processes\n",
                             method_name, __LINE__, (int)bcastPids->size());
            }

            SQ_theLocalIOToClient->putOnNoticeQueue( BCAST_PID
                                                   , -1
                                                   , msg
                                                   , bcastPids);
        }
    }
    else
    {
        delete bcastPids;
    }

    TRACE_EXIT;
}

void CNotice::NotifyRemote( void )
{
    const char method_name[] = "CNotice::NotifyRemote";
    TRACE_ENTRY;

    int      targetNid = -1;
    CNotice *entry = NULL;
    NidQueue_t *nidQueue = new NidQueue_t;

    // build the nid queue of nodes to notify
    for( entry=this; entry; entry=entry->Next )
    {
        entry->NotifyNid( nidQueue );
    }

    while ( !nidQueue->empty() )
    {
        targetNid = nidQueue->front();
        CLNode *targetLNode = Nodes->GetLNode( targetNid );
    
        int rc = -1;
        // Send the process exit to the target node
        rc = PtpClient->ProcessExit( Process
                                   , targetNid
                                   , targetLNode->GetNode()->GetName() );
        if (rc)
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] - Can't send process exit "
                      "for process %s (%d, %d) "
                      "to target node %s, nid=%d\n"
                    , method_name
                    , Process->GetName()
                    , Process->GetNid()
                    , Process->GetPid()
                    , targetLNode->GetNode()->GetName()
                    , targetLNode->GetNid() );
            mon_log_write(NOTICE_NOTIFYREMOTE_1, SQ_LOG_ERR, la_buf);
        }
        nidQueue->pop();
    }

    delete nidQueue;

    TRACE_EXIT;
}

void CNotice::NotifyNid( NidQueue_t *nidQueue )
{
    const char method_name[] = "CNotice::NotifyNid";
    TRACE_ENTRY;

    CProcess *remoteProcess;
    
    if ( canceled_ )
    {
        if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf( "%s@%d - Process death notice for process %s (%d, %d:%d) "
                          "was canceled so not delivered to process %s (%d, %d:%d)\n"
                        , method_name, __LINE__
                        , Process->GetName()
                        , Process->GetNid()
                        , Process->GetPid()
                        , Process->GetVerifier()
                        , name_.c_str()
                        , Nid
                        , Pid
                        , verifier_);
    }
    else
    {
        if ( !MyNode->IsMyNode(Nid) )
        {
            // find by nid (check node state, check process state, backup is Ok)
            remoteProcess = Nodes->GetProcess( Nid
                                             , Pid
                                             , verifier_
                                             , true, true, true );
            if( remoteProcess )
            {
                if (remoteProcess->IsSystemMessages() )
                {
                    // Add this process' nid to the queue
                    nidQueue->push( Nid);
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        CLNode *lnode = Nodes->GetLNode( Nid );
                        trace_printf( "%s@%d - Sending exit message of"
                                      " %s (%d, %d:%d) to %s (nid=%d)\n"
                                    , method_name, __LINE__
                                    , Process->GetName()
                                    , Process->GetNid()
                                    , Process->GetPid()
                                    , Process->GetVerifier()
                                    , lnode?lnode->GetNode()->GetName():""
                                    , lnode?lnode->GetNode()->GetPNid():Nid);
                    }
                }
                else
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d - Death message of %s (%d, %d:%d)" 
                                      " not wanted by %s (%d, %d:%d)\n"
                                    , method_name, __LINE__
                                    , Process->GetName()
                                    , Process->GetNid()
                                    , Process->GetPid()
                                    , Process->GetVerifier()
                                    , remoteProcess->GetName()
                                    , remoteProcess->GetNid()
                                    , remoteProcess->GetPid()
                                    , remoteProcess->GetVerifier() );
                    }
                }
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
               {
                   trace_printf( "%s@%d - Can't find process %s (%d, %d:%d)\n"
                               , method_name, __LINE__
                               , name_.c_str()
                               , Nid
                               , Pid
                               , verifier_ );
               }
            }
        }
        else
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf( "%s@%d - Not processed for local Process %s (%d, %d:%d)\n"
                            , method_name, __LINE__
                            , Process->GetName()
                            , Process->GetNid()
                            , Process->GetPid()
                            , Process->GetVerifier() );
            }
        }
    }
    TRACE_EXIT;
}

