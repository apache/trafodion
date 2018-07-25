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

#include <iostream>

using namespace std;

#include <signal.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>

#include "monlogging.h"
#include "montrace.h"
#include "monitor.h"
#include "lock.h"
#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "tmsync.h"
#include "mlio.h"
#include "reqqueue.h"
#include "nameserver.h"

extern bool NameServerEnabled;
extern int trace_level;
extern int MyPNID;
extern sigset_t SigSet;
extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
extern CNode *MyNode;
extern CReqQueue ReqQueue;

CTmSyncReq::CTmSyncReq( int nid, int handle, char *data, int length, int tag, bool unsolicited )
            :Nid( nid ),
             Tag( tag ),
             Handle( handle ),
             Length( length ),
             Unsolicited( unsolicited ),
             Replicated( false ),
             Completed( false ),
             Next( NULL ),
             Prev( NULL )
{
    const char method_name[] = "CTmSyncReq::CTmSyncReq";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "TSYN", 4);

    Data = new char [Length+1];
    memmove( Data, data, Length );
    if (trace_settings & TRACE_TMSYNC)
       trace_printf("%s@%d" " - Create " "%s"  "request handle=" "%d"  "\n", method_name, __LINE__, (Unsolicited?"unsolicited ":""), Handle);
    TRACE_EXIT;
}

CTmSyncReq::~CTmSyncReq( void )
{
    const char method_name[] = "CTmSyncReq::~CTmSyncReq";
    TRACE_ENTRY;
    delete [] Data;
    if (trace_settings & TRACE_TMSYNC)
       trace_printf("%s@%d" " - Delete " "%s"  "request (%p) handle=" "%d"  "\n", method_name, __LINE__, (Unsolicited?"unsolicited ":""), this, Handle);

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "tsyn", 4);

    TRACE_EXIT;
}

void CTmSyncReq::DeLink (CTmSyncReq ** head, CTmSyncReq ** tail)
{
    const char method_name[] = "CTmSyncReq::DeLink";
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

CTmSyncReq *CTmSyncReq::GetNext (void)
{
    const char method_name[] = "CTmSyncReq::GetNext";
    TRACE_ENTRY;
    TRACE_EXIT;
    return Next;
}

CTmSyncReq *CTmSyncReq::Link (CTmSyncReq * entry)
{
    const char method_name[] = "CTmSyncReq::Link";
    TRACE_ENTRY;
    Next = entry;
    entry->Prev = this;

    TRACE_EXIT;
    return entry;
}

CTmSync_Container::CTmSync_Container(void)
                  :CCluster(),
                   TmSyncReplies( 0 ),
                   ReqsInBlock( 0 ),
                   TmSyncReplyCode( MPI_SUCCESS ),
                   PendingSlaveTmSyncCount( 0 ),
                   Head( NULL ),
                   Tail( NULL ),
                   HandleSeq(MyPNID*MAX_TM_HANDLES),
                   PendingSlaveTmSync( false ),
                   TotalSlaveTmSyncCount( false ),
                   AbortPendingTmSync( false )
{
    const char method_name[] = "CTmSync_Container::CTmSync_Container";
    TRACE_ENTRY;

    int rc = sem_init(&UnsolicitedWaitSem, 0, 0);
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Can't create unnamed semaphore! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_TMSYNC_INIT_1, SQ_LOG_ERR, la_buf);

        abort();
    }

    TRACE_EXIT;
}

CTmSync_Container::~CTmSync_Container(void)
{
    CTmSyncReq *req = Head;

    const char method_name[] = "CTmSync_Container::~CTmSync_Container";
    TRACE_ENTRY;

    while (req)
    {
        req->DeLink (&Head, &Tail);
        delete req;
        req = Head;
    }

    int rc = sem_destroy( &UnsolicitedWaitSem );
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Can't destroy unnamed semaphore! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_TMSYNC_DEST_1, SQ_LOG_ERR, la_buf);

        abort();
    }

    TRACE_EXIT;
}

void CTmSync_Container::UpdateTmSyncState( int return_code )
{
    char                la_buf[MON_STRING_BUF_SIZE];
 
    const char method_name[] = "CTmSync_Container::UpdateTmSyncState";
    TRACE_ENTRY;
    
    if ( MyNode->GetTmSyncState() != SyncState_Abort )
    {
        if (( MyNode->GetTmSyncState() == SyncState_Start    ) ||
            ( MyNode->GetTmSyncState() == SyncState_Continue ) ||
            ( MyNode->GetTmSyncState() == SyncState_Commit   )   )
        {
            if ( return_code )
            {
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                   trace_printf("%s@%d" " - TmSync aborted" "\n", method_name, __LINE__);
                MyNode->SetTmSyncState( SyncState_Abort );
            }
            else
            {
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                   trace_printf("%s@%d" " - TmSync commited" "\n", method_name, __LINE__);
                MyNode->SetTmSyncState( SyncState_Commit );
            }
        }
        else
        {
            sprintf(la_buf, "[%s], Invalid SyncState (%d)! \n", method_name, MyNode->GetTmSyncState());
            mon_log_write(MON_TMSYNC_UPDATE_STATE_1, SQ_LOG_ERR, la_buf); 
            MyNode->SetTmSyncState( SyncState_Abort );
        }
        if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
           trace_printf("%s@%d" " - Physical Node " "%d"  " TmSyncState updated (" "%d" ")" "\n", method_name, __LINE__, MyPNID, MyNode->GetTmSyncState());
    }    

    TRACE_EXIT;
}

void CTmSync_Container::CommitTmDataBlock( int return_code )
{
    SyncState           state;
    char                la_buf[MON_STRING_BUF_SIZE];
 
    const char method_name[] = "CTmSync_Container::CommitTmDataBlock";
    TRACE_ENTRY;
    
    UpdateTmSyncState( return_code );

    // Loop here until the TM Sync has completed
    while (1)
    {
        if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
           trace_printf("%s@%d" " - Getting all nodes TmSyncState\n", method_name, __LINE__);

        ExchangeTmSyncState( false );
        state = Nodes->GetTmState( SyncState_Commit );
        if (( state == SyncState_Abort ) ||
            ( state == SyncState_Null  )   )
        {
            if ( state == SyncState_Null )
            {
                sprintf(la_buf, "[%s], Early termination! \n", method_name);
                mon_log_write(MON_TMSYNC_COMMITTMDATA_1, SQ_LOG_ERR, la_buf);
            }
            if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
               trace_printf("%s@%d" " - TmSyncAbort Sent" "\n", method_name, __LINE__);
            EndTmSync( MsgType_TmSyncAbort );
            if ( AbortPendingTmSync )
            {
                Monitor->TmSyncAbortPending();
            }
            break;
        }
        else if ( state == SyncState_Commit )
        {
            if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
               trace_printf("%s@%d" " - TmSyncCommit Sent" "\n", method_name, __LINE__);
            EndTmSync( MsgType_TmSyncCommit );
            break;
        }
        //usleep (5000);
    }    
    // End the TM sync processing cycle for my node.
    MyNode->SetTmSyncState( SyncState_Null );
    MyNode->SetTmSyncNid( -1 );
    ExchangeTmSyncState( false );
    if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
       trace_printf("%s@%d" " - Physical Node " "%d"  " TmSyncState updated (" "%d" ")" "\n", method_name, __LINE__, MyPNID, MyNode->GetTmSyncState());
    
    TRACE_EXIT;
}

int CTmSync_Container::CoordinateTmDataBlock ( struct sync_def *sync )
{
    const char method_name[] = "CTmSync_Container::CoordinateTmDataBlock";
    TRACE_ENTRY;
    if ( MyNode->GetState() == State_Down )
    {
        // For Virtual nodes: if we are down ... 
        // just return and continue processing like normal
        return MPI_SUCCESS;
    }

    if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
    {
        CNode *node = Nodes->GetNode( sync->pnid );
        if ( node )
        {
            trace_printf("%s@%d" " - Node %s (pnid=%d) TmSync initiated (sync nid=%d, sync state=%d)\n", method_name, __LINE__, node->GetName(), sync->pnid, node->GetTmSyncNid(), node->GetTmSyncState());
        }
    }
    if ( sync->pnid == MyPNID )
    {
//        if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
//        {
//           trace_printf("%s@%d" " - Node %s (pnid=%d) TmSync initiated (sync nid=%d, sync state=%d)\n", method_name, __LINE__, MyNode->GetName(), MyPNID, MyNode->GetTmSyncNid(), MyNode->GetTmSyncState());
//        }
        if ( MyNode->GetTmSyncNid() == -1)
        {
            // Our physical node requested the TM sync
            if ( Nodes->GetTmState( SyncState_Null ) != SyncState_Null)
            {
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                   trace_printf("%s@%d" " - Tm Sync already pending" "\n", method_name, __LINE__);
                return MPI_ERR_PENDING;
            }
            else
            {
                MyNode->SetTmSyncState( SyncState_Start );
                MyNode->SetTmSyncNid( sync->syncnid );
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                {
                   trace_printf("%s@%d" " - Master TmSync started" "\n", method_name, __LINE__);
                   trace_printf("%s@%d" " - Physical Node %d TmSyncState updated (nid=%d, state=%d)\n", method_name, __LINE__, MyPNID, MyNode->GetTmSyncNid(), MyNode->GetTmSyncState());
                }
                syncCycle_.lock();
                exchangeTmSyncData( sync, false );
                syncCycle_.unlock();
                ExchangeTmSyncState( false );
                if (( Monitor->tmSyncPNid_ == MyPNID                           ) &&
                    ( Nodes->GetTmState( SyncState_Start ) == SyncState_Start )   )
                {
                    // send unsolicited messages to other TMs in
                    // local physical node and wait for them to reply
                    if ( MyNode->GetLNodesCount() > 1 )
                    {
                        if ( PendingSlaveTmSync )
                        {
                            SendUnsolicitedMessages();
                            while (1)
                            {
                                if ( GetTmSyncReplies() == GetTotalSlaveTmSyncCount() )
                                {
                                    break;
                                }
                                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                                    trace_printf("%s@%d" " - Master waiting for Local Unsolicited TmSync reply, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );
                                    UnsolicitedCompleteWait();
                            }    
                        }
                    }
                         
                    // send reply to our TM for the sync request
                    if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                       trace_printf("%s@%d" " - Master TmSync reply" "\n", method_name, __LINE__);
                    CommitTmDataBlock(MPI_SUCCESS);
                    return MPI_SUCCESS;
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                       trace_printf("%s@%d" " - Tm Sync failed to start, tmSyncPNid_=%d, MyPNID=%d, " "TmSyncState=%d, expecting=%d\n", method_name, __LINE__, tmSyncPNid_, MyPNID, Nodes->GetTmState( SyncState_Start ), SyncState_Start);
                    if (MyNode->GetTmSyncState() == SyncState_Start)
                    {
                        MyNode->SetTmSyncState( SyncState_Null );
                        if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                           trace_printf("%s@%d" " - Physical Node " "%d"  " TmSyncState updated (" "%d" ")" "\n", method_name, __LINE__, MyPNID, MyNode->GetTmSyncState());
                    }       
                    return MPI_ERR_PENDING;
                }
            }
        }
        else
        {
            // Another logical node in my physical node requested a TM sync
            if ( MyNode->GetTmSyncState() == SyncState_Start )
            {
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                {
                   trace_printf("%s@%d" " - Slave TmSync started on local Physical Node "  "%d" "\n", method_name, __LINE__, sync->pnid);
                   trace_printf("%s@%d" " - Physical Node " "%d"  " TmSyncState updated (" "%d" ")" "\n", method_name, __LINE__, MyPNID, MyNode->GetTmSyncState());
                }
                UnPackSyncData(sync);
            }
        }
    }
    else
    {
        // some other physical node requested a TM sync.
        if ( MyNode->GetTmSyncState() == SyncState_Null )
        {
            // Send sync data to our node's TM
            MyNode->SetTmSyncState( SyncState_Continue );
            if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
            {
               trace_printf("%s@%d" " - Slave TmSync started on Physical Node "  "%d" "\n", method_name, __LINE__, sync->pnid);
               trace_printf("%s@%d" " - Physical Node " "%d"  " TmSyncState updated (" "%d" ")" "\n", method_name, __LINE__, MyPNID, MyNode->GetTmSyncState());
            }
            UnPackSyncData(sync);
            ExchangeTmSyncState( true );
        }
        else
        {
            MyNode->SetTmSyncState( SyncState_Abort );
            if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
            {
               trace_printf("%s@%d" " - Collision with another TM trying to sync" "\n", method_name, __LINE__);
               trace_printf("%s@%d" " - Physical Node " "%d"  " TmSyncState updated (" "%d" ")" "\n", method_name, __LINE__, MyPNID, MyNode->GetTmSyncState());
            }
        }    
    }

    TRACE_EXIT;
    return 0;
}

void CTmSync_Container::EndTmSync( MSGTYPE type )
{
    CProcess           *mytm;
    CTmSyncReq         *req = Head;
    CTmSyncReq         *next;
    CLNode             *lnode;
    struct message_def *msg = NULL;
    struct message_def *notice;
    int                 count = 0;
    int                 orig_count = 0;
    char                la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CTmSync_Container::EndTmSync";
    TRACE_ENTRY;

    // send a commit or abort notice to my node's TM
    msg = new struct message_def;
    msg->type = type;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    while (req)
    {
        next = req->GetNext();
        if ( !req->Unsolicited )
        {
            // my node
            if (trace_settings & TRACE_TMSYNC)
            {
                trace_printf("%s@%d - Original request (%p) nid=%d, handle=%d, tag=%d, unsol=%d, comp=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, req->Unsolicited, req->Completed);
            }
            msg->u.request.u.tm_sync_notice.orig_tag[orig_count] = req->Tag;
            msg->u.request.u.tm_sync_notice.orig_handle[orig_count] = req->Handle;
            orig_count++;
        }
        if (( req->Replicated                 ) &&
            ( req->Completed                  )   )
        {
            if (trace_settings & TRACE_TMSYNC)
            {
                trace_printf("%s@%d - Request (%p) nid=%d, handle=%d, tag=%d, unsol=%d, comp=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, req->Unsolicited, req->Completed);
            }
            if ( tmSyncPNid_ == MyPNID )
            {
                if ( MyNode->GetLNodesCount() > 1 )
                {
                    if ( req->Unsolicited )
                    {
                        msg->u.request.u.tm_sync_notice.nid[count] = req->Nid;
                        msg->u.request.u.tm_sync_notice.handle[count] = req->Handle;
                        count++;
                        if (trace_settings & TRACE_TMSYNC)
                        {
                            trace_printf("%s@%d - Count Unsolicited request (%p) nid=%d, handle=%d, tag=%d, count=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, count);
                        }
                    }
                }
                else
                {
                    msg->u.request.u.tm_sync_notice.nid[count] = req->Nid;
                    msg->u.request.u.tm_sync_notice.handle[count] = req->Handle;
                    count++;
                    if (trace_settings & TRACE_TMSYNC)
                    {
                        trace_printf("%s@%d - Count request (%p) nid=%d, handle=%d, tag=%d, count=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, count);
                    }
                }
            }
            else
            {
                msg->u.request.u.tm_sync_notice.nid[count] = req->Nid;
                msg->u.request.u.tm_sync_notice.handle[count] = req->Handle;
                count++;
                if (trace_settings & TRACE_TMSYNC)
                {
                    trace_printf("%s@%d - Count request (%p) nid=%d, handle=%d, tag=%d, count=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, count);
                }
            }
            req->DeLink(&Head,&Tail);
            delete req;
        }
        req = next;
    }
    msg->u.request.u.tm_sync_notice.count = count;
    msg->u.request.u.tm_sync_notice.orig_count = orig_count;

    lnode = MyNode->GetFirstLNode();
    for ( ; lnode  ; lnode = lnode->GetNextP() )
    {
        if ( lnode->GetState() == State_Up )
        {
            // the logical node is up and 
            // is not the requesting logical node
            mytm = lnode->GetProcessLByType( ProcessType_DTM );
            if ( !mytm )
            {
                sprintf(la_buf, "[%s], Can't find TM in node=%d\n", method_name, lnode->GetNid());
                mon_log_write(MON_TMSYNC_END, SQ_LOG_ERR, la_buf); 
    
                ReqQueue.enqueueDownReq(MyPNID);
            }
            else
            {
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                {
                    trace_printf("%s@%d - Sending Notice to TM=%s (%d,%d)\n", method_name, __LINE__, mytm->GetName(), mytm->GetNid(), mytm->GetPid());
                    trace_printf("        %s %d handles\n", (type == MsgType_TmSyncCommit?" - Commit ":" - Aborted "), count);
                }
                notice = new struct message_def;
                memmove( notice, msg, sizeof(message_def) );

                SQ_theLocalIOToClient->putOnNoticeQueue( mytm->GetPid()
                                                       , mytm->GetVerifier()
                                                       , notice
                                                       , NULL);
            }
        }
    }

    
    if ( msg )
    {
        delete msg;
    }

    // Initialize Unsolicited message counters
    TmSyncReplies = 0;
    TmSyncReplyCode = MPI_SUCCESS;
    ReqsInBlock = 0;
    PendingSlaveTmSyncCount = 0;
    TotalSlaveTmSyncCount = 0;
    TRACE_EXIT;
}

void CTmSync_Container::EndPendingTmSync( struct sync_def *sync )
{
    CProcess           *mytm;
    CTmSyncReq         *req = Head;
    CTmSyncReq         *next;
    CLNode             *lnode;
    struct message_def *msg = NULL;
    struct message_def *notice;
    int                 count = 0;
    int                 orig_count = 0;
    char                la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CTmSync_Container::EndPendingTmSync";
    TRACE_ENTRY;

    // send a abort notice to my node's TM
    msg = new struct message_def;
    msg->type = MsgType_TmSyncAbort;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    while (req)
    {
        next = req->GetNext();
        if ( !req->Unsolicited )
        {
            // my node
            if (trace_settings & TRACE_TMSYNC)
            {
                trace_printf("%s@%d - Original request (%p) nid=%d, handle=%d, tag=%d, unsol=%d, comp=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, req->Unsolicited, req->Completed);
            }
            msg->u.request.u.tm_sync_notice.orig_tag[orig_count] = req->Tag;
            msg->u.request.u.tm_sync_notice.orig_handle[orig_count] = req->Handle;
            orig_count++;
        }
        if (( sync->syncnid == req->Nid ) &&
            ( req->Replicated           ) &&
            ( !req->Unsolicited         ) &&
            ( req->Completed            )   )
        {
            if (trace_settings & TRACE_TMSYNC)
            {
                trace_printf("%s@%d - Request (%p) nid=%d, handle=%d, tag=%d, unsol=%d, comp=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, req->Unsolicited, req->Completed);
            }
            msg->u.request.u.tm_sync_notice.nid[count] = req->Nid;
            msg->u.request.u.tm_sync_notice.handle[count] = req->Handle;
            count++;
            if (trace_settings & TRACE_TMSYNC)
            {
                trace_printf("%s@%d - Count request (%p) nid=%d, handle=%d, tag=%d, count=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, count);
            }
            req->DeLink(&Head,&Tail);
            delete req;
        }
        req = next;
    }
    
    if ( count )
    {
        msg->u.request.u.tm_sync_notice.count = count;
        msg->u.request.u.tm_sync_notice.orig_count = orig_count;

        lnode = MyNode->GetFirstLNode();
        for ( ; lnode  ; lnode = lnode->GetNextP() )
        {
            if ( lnode->GetNid() == sync->syncnid &&
                 lnode->GetState() == State_Up )
            {
                // the logical node is up and 
                // is not the requesting logical node
                mytm = lnode->GetProcessLByType( ProcessType_DTM );
                if ( !mytm )
                {
                    sprintf(la_buf, "[%s], Can't find TM in node=%d\n", method_name, lnode->GetNid());
                    mon_log_write(MON_TMSYNC_END_PENDING, SQ_LOG_ERR, la_buf); 
    
                    ReqQueue.enqueueDownReq(MyPNID);
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                    {
                        trace_printf("%s@%d - Sending Notice to TM=%s (%d,%d)\n", method_name, __LINE__, mytm->GetName(), mytm->GetNid(), mytm->GetPid());
                        trace_printf("      - Aborted %d handles\n", count);
                    }
                    notice = new struct message_def;
                    memmove( notice, msg, sizeof(message_def) );
                    SQ_theLocalIOToClient->putOnNoticeQueue( mytm->GetPid()
                                                           , mytm->GetVerifier()
                                                           , notice
                                                           , NULL);
                }
            }
        }
    }

    
    if ( msg )
    {
        delete msg;
    }

    TRACE_EXIT;
}

void CTmSync_Container::ProcessTmSyncReply( struct message_def * msg )
{
    const char method_name[] = "CTmSync_Container::ProcessTmSyncReply";
    TRACE_ENTRY;

    CTmSyncReq *tmsync_req;

    if (trace_settings & (TRACE_REQUEST | TRACE_TMSYNC))
        trace_printf("%s@%d - Unsolicited TmSync Reply\n",
                     method_name, __LINE__);
    tmsync_req = FindTmSyncReq( msg->u.reply.u.unsolicited_tm_sync.handle );
    if (tmsync_req)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_TMSYNC))
            trace_printf("%s@%d - Unsolicited TmSync reply, handle=%d\n",
                         method_name, __LINE__, tmsync_req->Handle);
        if (msg->u.reply.u.unsolicited_tm_sync.return_code == MPI_SUCCESS)
        {
            TmSyncReplyCode |= msg->u.reply.u.unsolicited_tm_sync.return_code;
            tmsync_req->Completed = true;
            UnsolicitedComplete( msg );
            if ( tmSyncPNid_ == MyPNID )
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_TMSYNC))
                    trace_printf("%s@%d - Local Unsolicited TmSync reply, handle="
                                 "%d\n", method_name, __LINE__,
                                 tmsync_req->Handle);
                if ( GetTmSyncReplies() == GetTotalSlaveTmSyncCount() )
                {
                    UpdateTmSyncState( TmSyncReplyCode );
                    UnsolicitedCompleteDone();
                }
            }
            else
            {
                if ( GetTmSyncReplies() == GetTotalSlaveTmSyncCount() )
                {
                    CommitTmDataBlock(TmSyncReplyCode);
                }
            }
        }
        else
        { // The Seabed callback has not been registered, try again
            if (trace_settings & (TRACE_REQUEST | TRACE_TMSYNC))
                trace_printf("%s@%d - Retrying Local Unsolicited TmSync, handle="
                             "%d\n", method_name, __LINE__,
                             tmsync_req->Handle);
            PendingSlaveTmSyncCount--;
            tmsync_req->Completed = false;
            SendUnsolicitedMessages();
        }
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_TMSYNC))
            trace_printf("%s@%d" " - Can't find TmSync request, handle="  "%d" "\n", method_name, __LINE__, msg->u.reply.u.unsolicited_tm_sync.handle);
    }
    msg->noreply = true;

    if (trace_settings & TRACE_TMSYNC)
       trace_printf("%s@%d" " - Unsolicited TmSync notices, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );

    TRACE_EXIT;
}

void CTmSync_Container::ExchangeTmSyncState( bool bumpSync )
{
    struct sync_def sync;

    const char method_name[] = "CTmSync_Container::ExchangeTmSyncState";
    TRACE_ENTRY;

    sync.type = SyncType_TmSyncState;
    sync.pnid = MyPNID;
    sync.syncnid = MyNode->GetTmSyncNid();
    sync.state = MyNode->GetTmSyncState();
    sync.count = 0;
    sync.length = 0;
    syncCycle_.lock();
    exchangeTmSyncData( &sync, bumpSync );
    syncCycle_.unlock();

    TRACE_EXIT;
}

CTmSyncReq *CTmSync_Container::FindTmSyncReq( int handle )
{
    CTmSyncReq *req = Head;

    const char method_name[] = "CTmSync_Container::FindTmSyncReq";
    TRACE_ENTRY;
    while (req)
    {
        if( req->Handle == handle && req->Unsolicited)
        {
            if (trace_settings & TRACE_TMSYNC)
            {
                trace_printf("%s@%d - request (%p) nid=%d, handle=%d, tag=%d, unsol=%d, comp=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, req->Unsolicited, req->Completed);
            }
            break;
        }
        req = req->GetNext();
    }
    TRACE_EXIT;

    return req;
}

int CTmSync_Container::GetHandle( void )
{
    const char method_name[] = "CTmSync_Container::GetHandle";
    TRACE_ENTRY;
    if ( HandleSeq >= (MyPNID+1)*MAX_TM_HANDLES ) 
    {
        HandleSeq = MyPNID*MAX_TM_HANDLES;
    }
    TRACE_EXIT;
    return ++HandleSeq;
}

struct sync_def *CTmSync_Container::PackSyncData( void )
{
    char *ptr;
    CTmSyncReq *req = Head;
    struct sync_def *sync;

    const char method_name[] = "CTmSync_Container::PackSyncData";
    TRACE_ENTRY;

    sync = new struct sync_def;
    sync->type = SyncType_TmData;
    sync->pnid = MyPNID;
    sync->syncnid = -1;
    sync->state = SyncState_Start;
    sync->count = 0;
    sync->length = 0;
    ptr = sync->data;
    while ( req )
    {
        if (( !req->Unsolicited                                         ) &&
            ( !req->Replicated                                          ) &&
            (  sync->length+req->Length+(sizeof(int)*3) < MAX_SYNC_DATA ) && 
            (  sync->count                              < MAX_TM_SYNCS  )   )
        {
            if ( sync->syncnid == -1 )
            {
                // The first pending request's logical node wins
                sync->syncnid = req->Nid;
            }
            if ( sync->syncnid == req->Nid )
            {
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                   trace_printf("%s@%d" " - Packing TmSync request, nid=%d, handle="  "%d" "\n", method_name, __LINE__, req->Nid, req->Handle);
                // load request
                sync->count++;
                *((int*)ptr) = req->Nid;
                ptr += sizeof(int);
                *((int*)ptr) = req->Handle;
                ptr += sizeof(int);
                *((int*)ptr) = req->Length;
                ptr += sizeof(int);
                memmove( ptr, req->Data, req->Length );
                ptr += req->Length;
                req->Replicated = true;
                sync->length += ((sizeof(int)*3)+req->Length);
            }
            else
            {
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                   trace_printf("%s@%d" " - NOT Packing TmSync request, nid=%d, handle="  "%d" "\n", method_name, __LINE__, req->Nid, req->Handle);
            }
        }
        req = req->GetNext();
    }
    TRACE_EXIT;

    return sync;
}

void CTmSync_Container::ReQueue_TmSync( bool master )
{
    int collisionNid = -1;
    MyNode->SetTmSyncNid( -1 );
    CTmSyncReq *req = Head;
    CTmSyncReq *next;

    const char method_name[] = "CTmSync_Container::ReQueue_TmSync";
    TRACE_ENTRY; 
    while (req)
    {
        if (trace_settings & TRACE_TMSYNC)
        {
            trace_printf("%s@%d - request (%p) nid=%d, handle=%d, tag=%d, unsol=%d, comp=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, req->Unsolicited, req->Completed);
        }
        next = req->GetNext();
        if ( req->Replicated )
        {
            if ( master && !req->Unsolicited )
            {
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                   trace_printf("%s@%d" " - Collision, resetting handle="  "%d" "\n", method_name, __LINE__, req->Handle);
                req->Replicated = false;
                collisionNid = req->Nid;
            }
            if ( master && req->Unsolicited && req->Nid == collisionNid ) 
            { 
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                   trace_printf("%s@%d" " - Collision, deleting unsolicited handle="  "%d" "\n", method_name, __LINE__, req->Handle);
                req->DeLink(&Head,&Tail);
                delete req;
            }
            if ( !master && req->Unsolicited ) 
            { 
                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                   trace_printf("%s@%d" " - Collision, deleting unsolicited handle="  "%d" "\n", method_name, __LINE__, req->Handle);
                req->DeLink(&Head,&Tail);
                delete req;
            }
        }
        req = next;
    }
    TRACE_EXIT; 
}

CTmSyncReq *CTmSync_Container::Q_TmSync(int nid, int handle, char *data, int len, int tag, bool unsolicited)
{
    CTmSyncReq *req = NULL;
    char        la_buf[MON_STRING_BUF_SIZE];
    
    const char method_name[] = "CTmSync_Container::Q_TmSync";
    TRACE_ENTRY;

    if ( len > MAX_SYNC_DATA )
    {
        sprintf(la_buf, "[%s], Tm Sync length greater than max, len=%d. \n", method_name, len);
        mon_log_write(MON_TMSYNC_Q_TMSYNC, SQ_LOG_ERR, la_buf);
    }
    else
    {
        req = new CTmSyncReq(nid, handle, data, len, tag, unsolicited);
        if (Head == NULL)
        {
            Head = Tail = req;
        }
        else
        {
            Tail = Tail->Link (req);
        }
    }

    TRACE_EXIT;
    return req;
}

void CTmSync_Container::SendUnsolicitedMessages (void)
{
    int                 numTMs = 0;
    CTmSyncReq         *req = Head;
    CProcess           *mytm;
    CProcess           *tm = NULL;
    CLNode             *lnode;
    struct message_def *msg = NULL;
    struct message_def *notice;
    char                la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CTmSync_Container::SendUnsolicitedMessages";
    TRACE_ENTRY;

    while (req)
    {
        if (trace_settings & TRACE_TMSYNC)
        {
            trace_printf("%s@%d - request (%p) nid=%d, handle=%d, tag=%d, unsol=%d, comp=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, req->Unsolicited, req->Completed);
        }
        if ( req->Unsolicited && !req->Completed )
        { 
            if (!tm || req->Nid != tm->GetNid())
            { 
                // Get the TM that initiated the sync request
                tm = LNode[req->Nid]->GetProcessLByType( ProcessType_DTM );
            }
            if (!tm && NameServerEnabled)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                {
                    trace_printf( "%s@%d - Getting process from Name Server, nid=%d, type=ProcessType_DTM\n"
                                , method_name, __LINE__, req->Nid );
                }
            
                tm = Nodes->GetProcessLByTypeNs( req->Nid, ProcessType_DTM );
            }
            if ( tm )
            {
                // send all TmSync requests data to the local TM processes
                msg = new struct message_def;
                msg->type = MsgType_UnsolicitedMessage;
                msg->noreply = false;
                msg->u.request.type = ReqType_TmSync;
                msg->u.request.u.unsolicited_tm_sync.nid = tm->GetNid();
                msg->u.request.u.unsolicited_tm_sync.pid = tm->GetPid();
                msg->u.request.u.unsolicited_tm_sync.handle = req->Handle;
                memmove( msg->u.request.u.unsolicited_tm_sync.data, req->Data, req->Length );
                msg->u.request.u.unsolicited_tm_sync.length = req->Length;

                // count the number of candidate TMs
                if ( numTMs == 0 )
                {
                    lnode = MyNode->GetFirstLNode();
                    for ( ; lnode  ; lnode = lnode->GetNextP() )
                    {
                        if ( lnode->GetNid() != req->Nid )
                        {
                            numTMs++;
                        }
                    }
                    // calculate the total number of TmSync request 
                    TotalSlaveTmSyncCount = ReqsInBlock * numTMs;
                }

                if (trace_settings & TRACE_TMSYNC)
                   trace_printf("%s@%d" " - Unsolicited TmSync notices, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );
                   
                lnode = MyNode->GetFirstLNode();
                for ( ; lnode  ; lnode = lnode->GetNextP() )
                {
                    if ( lnode->GetState() == State_Up && 
                         lnode->GetNid()   != req->Nid      )
                    {
                        // the logical node is up and 
                        // is not the requesting logical node
                        mytm = lnode->GetProcessLByType( ProcessType_DTM );
                        if ( !mytm )
                        {
                            sprintf(la_buf, "[%s], Can't find TM in node=%d\n", method_name, lnode->GetNid());
                            mon_log_write(MON_TMSYNC_SEND_UNSOLICITED, SQ_LOG_ERR, la_buf); 
                
                            ReqQueue.enqueueDownReq(MyPNID);
                        }
                        else
                        {
                            if (trace_settings & TRACE_TMSYNC)
                            {
                                trace_printf("%s@%d - Sending Unsolicited TmSync to TM=%s (%d,%d)\n", method_name, __LINE__, mytm->GetName(), mytm->GetNid(), mytm->GetPid());
                                trace_printf("        from TM=%s (%d,%d)\n", tm->GetName(), tm->GetNid(), tm->GetPid());
                                trace_printf("        tag=%d\n", req->Tag);
                                trace_printf("        handle=%d\n", req->Handle);
                            }
                            notice = new struct message_def;
                            memmove( notice, msg, sizeof(message_def) );
                            SQ_theLocalIOToClient->putOnNoticeQueue( mytm->GetPid()
                                                                   , mytm->GetVerifier()
                                                                   , notice
                                                                   , NULL);
                            mytm->IncrUnsolTmSyncCount();
                            PendingSlaveTmSyncCount++;
                        }
                    }
                }
                if ( msg )
                {
                    delete msg;
                    msg = NULL;
                }
                if (NameServerEnabled)
                {
                    if (!MyNode->IsMyNode( tm->GetNid() )
                      && (req->GetNext() && req->GetNext()->Nid != tm->GetNid() ) )
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                        {
                            trace_printf( "%s@%d - Deleting clone process %s, (%d,%d:%d)\n"
                                        , method_name, __LINE__
                                        , tm->GetName()
                                        , tm->GetNid()
                                        , tm->GetPid()
                                        , tm->GetVerifier() );
                        }
                        Nodes->DeleteCloneProcess( tm );
                        tm = NULL;
                    }
                
                }
            }
            else
            {
                sprintf(la_buf, "[%s], Can't find requesting TM for nid= %d.\n", method_name, req->Nid);
                mon_log_write(MON_TMSYNC_UNPACKSYNCDATA, SQ_LOG_ERR, la_buf); 
                CNode *node = LNode[req->Nid]->GetNode();
                ReqQueue.enqueueDownReq( node->GetPNid() );
            }
        }
        req = req->GetNext();
    }

    if (trace_settings & TRACE_TMSYNC)
       trace_printf("%s@%d" " - Unsolicited TmSync notices, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );
       
    PendingSlaveTmSync = false;

    TRACE_EXIT;
}

void CTmSync_Container::TmSync( void )
{
    int              rc;
    struct sync_def *block;

    const char method_name[] = "CTmSync_Container::TmSync";
    TRACE_ENTRY;

    block = PackSyncData();
    if ( block->count )
    {
        if (trace_settings & TRACE_TMSYNC)
           trace_printf("%s@%d" " - Processing TmSync request" "\n", method_name, __LINE__);
        rc = CoordinateTmDataBlock( block );
        if ( rc != MPI_SUCCESS )
        {
            ReQueue_TmSync (true);
            if (trace_settings & TRACE_TMSYNC)
               trace_printf("%s@%d" " - Collision, no requests processed" "\n", method_name, __LINE__);
        }
    }

    if (trace_settings & TRACE_TMSYNC)
       trace_printf("%s@%d" " - PendingTmSync=%d, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, PendingSlaveTmSync, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );

    delete block;

    TRACE_EXIT;
}

void CTmSync_Container::TmSyncAbortPending( void )
{
    bool             notDone = true;
    struct sync_def *block = NULL;

    const char method_name[] = "CTmSync_Container::TmSyncAbortPending";
    TRACE_ENTRY;

    do
    {
        block = PackSyncData();
        if ( block->count )
        {
            if (trace_settings & TRACE_TMSYNC)
               trace_printf("%s@%d" " - Aborting pending TmSync\n", method_name, __LINE__);
            EndPendingTmSync( block );
        }
        else
        {
            notDone = false;
        }

        if (trace_settings & TRACE_TMSYNC)
           trace_printf("%s@%d" " - PendingTmSync=%d, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, PendingSlaveTmSync, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );

        delete block;
    }
    while( notDone );

    AbortPendingTmSync = false;

    TRACE_EXIT;
}

bool CTmSync_Container::TmSyncPending( void )
{
    bool rc = false;
    bool ready = true;
    int pnid = 0;
    CNode *node;
    CTmSyncReq *req;

    const char method_name[] = "CTmSync_Container::TmSyncPending";
    TRACE_ENTRY;
    if ( MyNode->GetState() == State_Down )
    {
        // For Virtual nodes: if we are down ... 
        // just return and continue processing like normal
        return false;
    }
    if (trace_settings & TRACE_TMSYNC)
       trace_printf("%s@%d" " - PendingTmSync=%d, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, PendingSlaveTmSync, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );

    if (( MyNode->GetTmSyncState() == SyncState_Abort ) &&
        ( tmSyncPNid_ != MyPNID ) &&
        ( GetTmSyncReplies() == GetTotalSlaveTmSyncCount() )   )
    {
        CommitTmDataBlock( MPI_ERR_UNKNOWN );
    }
    else
    {
        if ( ! MyNode->IsSpareNode() && MyNode->GetPhase() != Phase_Ready )
        {
            MyNode->CheckActivationPhase();
        }
        if ( Nodes->GetTmState( SyncState_Suspended ) == SyncState_Suspended )
        {
            if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
               trace_printf("%s@%d" " - TmSync suspended" "\n", method_name, __LINE__);
            return false;
        }
        if (trace_settings & TRACE_TMSYNC)
           trace_printf("%s@%d" " - PendingTmSync=%d, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, PendingSlaveTmSync, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );
        if ( PendingSlaveTmSync )
        {
            if (trace_settings & TRACE_TMSYNC)
               trace_printf("%s@%d" " - Pending TmSync, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );
            SendUnsolicitedMessages();
            return false;
        }
        if ( TmSyncReplies != GetTotalSlaveTmSyncCount() )
        {
            if (trace_settings & TRACE_TMSYNC)
               trace_printf("%s@%d" " - TmSync waiting for TmSync replies, total=%d, replies=%d, pending=%d\n", method_name, __LINE__, GetTotalSlaveTmSyncCount(), GetTmSyncReplies(), GetPendingSlaveTmSyncCount() );
            return false;
        }
    }

    // if no one is trying to tmsync and we have something ... then go for it
    node = Nodes->GetNode(pnid);
    while ( node )
    {
        if (( node->GetState() == State_Up && ! node->IsSpareNode() ) &&
            ( node->GetTmSyncState() != SyncState_Null )   )
        {
            if (trace_settings & TRACE_TMSYNC)
               trace_printf("%s@%d" " - TmSync needed, but not ready. TmSyncState("  "%d" ")" "\n", method_name, __LINE__, node->GetTmSyncState());
            ready = false;
            break;
        }
        node = Nodes->GetNode(++pnid);
    }

    if ( ready )
    {
        req = Head;
        while ( req )
        {
            if ( !req->Replicated )
            {
                if (trace_settings & TRACE_TMSYNC)
                   trace_printf("%s@%d" " - TmSync needed" "\n", method_name, __LINE__);
                rc = true;
                break;
            }
            req = req->GetNext();
        }
    }

    TRACE_EXIT;
    return rc;
}

void CTmSync_Container::UnPackSyncData(struct sync_def *sync)
{
    char               *ptr;
    int                 nid;
    int                 handle;
    int                 length;
    CTmSyncReq         *req;

    const char method_name[] = "CTmSync_Container::UnPackSyncData";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
       trace_printf("%s@%d" " - UnPacking TmSync request(s)" "\n", method_name, __LINE__);

    // Initialize Unsolicited message counters
    TmSyncReplies = 0;
    TotalSlaveTmSyncCount = 0;
    TmSyncReplyCode = MPI_SUCCESS;
    ReqsInBlock = sync->count;

    // send all TmSync request data to TM process
    ptr = sync->data;
    while (sync->count)
    {
        // load request
        nid = *((int*)ptr);
        ptr += sizeof(int);
        handle = *((int*)ptr);
        ptr += sizeof(int);
        length = *((int*)ptr);
        ptr += sizeof(int);

        // save another nodes TmSync request in our nodes' request queue,
        // but mark it as having been replicated
        req = Q_TmSync(nid, handle, ptr, length, -1, true);
        req->Replicated = true;
        PendingSlaveTmSync = true;

        ptr += length;
        sync->count--;
    }
    TRACE_EXIT;
}

void CTmSync_Container::CancelUnsolicitedMessages( CProcess *tmProcess )
{
    CTmSyncReq *req = Head;
    const char method_name[] = "CTmSync_Container::CancelUnsolicitedMessages";
    TRACE_ENTRY;

    while (req)
    {
        if (trace_settings & TRACE_TMSYNC)
        {
            trace_printf("%s@%d - request (%p) nid=%d, handle=%d, tag=%d, unsol=%d, comp=%d\n", method_name, __LINE__, req, req->Nid, req->Handle, req->Tag, req->Unsolicited, req->Completed);
        }
        if ( req->Unsolicited && !req->Completed )
        {
            // Count it as a completed reply
            IncrTmSyncReplies();
            tmProcess->DecrUnsolTmSyncCount();
            if ( trace_settings & TRACE_TMSYNC )
            {
                trace_printf("%s@%d - Canceling Unsolicited TmSync to TM=%s (%d,%d)\n", method_name, __LINE__, tmProcess->GetName(), tmProcess->GetNid(), tmProcess->GetPid());
                trace_printf("        tag=%d\n", req->Tag);
                trace_printf("        handle=%d\n", req->Handle);
                trace_printf("%s@%d - TM=%s (%d,%d), TmSync count=%d  \n", method_name, __LINE__, tmProcess->GetName(), tmProcess->GetNid(), tmProcess->GetPid(), tmProcess->GetUnsolTmSyncCount());
            }

            int rc = sem_post( &UnsolicitedWaitSem );
            if ( rc && errno != EINTR)
            {
                int err = errno;
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], Can't post on unnamed semaphore! - errno=%d (%s)\n", method_name, err, strerror(errno));
                mon_log_write(MON_TMSYNC_CANCEL_UNSOL_MESSAGE_1, SQ_LOG_ERR, la_buf);
            }
        
            break;
        }
        req = req->GetNext();
    }

    TRACE_EXIT;
}

void CTmSync_Container::UnsolicitedComplete( struct message_def *msg )
{
    const char method_name[] = "CTmSync_Container::UnsolicitedComplete";
    TRACE_ENTRY;

    CLNode *lnode = MyNode->GetLNode( msg->u.reply.u.unsolicited_tm_sync.nid );
    assert( lnode );

    CProcess *mytm = lnode->GetProcessLByType( ProcessType_DTM );
    if ( !mytm )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Can't find TM in node nid=%d\n", method_name, lnode->GetNid());
        mon_log_write(MON_TMSYNC_UNSOL_COMPLETE_1, SQ_LOG_ERR, la_buf); 

        ReqQueue.enqueueDownReq(MyPNID);
    }
    else
    {
        IncrTmSyncReplies();
        mytm->DecrUnsolTmSyncCount();
        if ( trace_settings & TRACE_TMSYNC )
        {
            trace_printf("%s@%d - TmSync reply from TM=%s (%d,%d), TmSync count=%d  \n", method_name, __LINE__, mytm->GetName(), mytm->GetNid(), mytm->GetPid(), mytm->GetUnsolTmSyncCount());
        }
    }

    TRACE_EXIT;
}

void CTmSync_Container::UnsolicitedCompleteDone( void )
{
    const char method_name[] = "CTmSync_Container::UnsolicitedCompleteDone";
    TRACE_ENTRY;

    int rc = sem_post( &UnsolicitedWaitSem );
    if ( rc && errno != EINTR)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Can't post on unnamed semaphore! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_TMSYNC_UNSOL_COMPLETE_2, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
}

void CTmSync_Container::UnsolicitedCompleteWait( void )
{
    const char method_name[] = "CTmSync_Container::UnsolicitedCompleteWait";
    TRACE_ENTRY;

    int rc;
    bool waitSomeMore = true;

    do
    {
        rc = sem_wait( &UnsolicitedWaitSem );
        if ( rc )
        {
            if ( errno != EINTR )
            {
                int err = errno;
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], Can't wait on unnamed semaphore! - errno=%d (%s)\n", method_name, err, strerror(errno));
                mon_log_write(MON_TMSYNC_UNSOL_COMPLETE_WAIT_1, SQ_LOG_ERR, la_buf);
                waitSomeMore = false;
            }
        }
        else
        {
            waitSomeMore = false;
        }
    }
    while( waitSomeMore );

    TRACE_EXIT;
}
