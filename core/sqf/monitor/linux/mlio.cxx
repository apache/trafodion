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

#include <sys/time.h>
#include <sys/resource.h>
#include <iostream>
#include <exception>
#include <fstream>
#include <string>
#include <map>

using namespace std;

#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/un.h>
#include <signal.h>
#include "monlogging.h"
#include "monsonar.h"
#include "montrace.h"
#include "localio.h"
#include "monitor.h"
#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "mlio.h"
#include "robsem.h"

#include "reqqueue.h"
extern CReqQueue ReqQueue;

extern int MyPNID;
extern CNode *MyNode;
extern char MyCommPort[];
extern CommType_t CommType;
extern CMonitor *Monitor;
extern sigset_t SigSet;
extern CMonStats *MonStats;
// Seabed disconnect semaphore
extern RobSem * sbDiscSem;

extern char *ErrorMsg( int error_code );

// The serviceRequestSize array holds the size of the request messages for
// use by getSizeOfRequest.  The ordering is important and must match
// that of the REQTYPE enum in msgdef.h.
const int SQ_LocalIOToClient::serviceRequestSize[] = {
   0, // unused, request types start at 1
   sizeof(REQTYPE) + sizeof( Close_def ),           // ReqType_Close
   sizeof(REQTYPE) + sizeof( DelProcessNs_def ),    // ReqType_DelProcessNs
   sizeof(REQTYPE) + sizeof( Dump_def ),            // ReqType_Dump
   sizeof(REQTYPE) + sizeof( Event_Notice_def ),    // ReqType_Event
   sizeof(REQTYPE) + sizeof( Exit_def ),            // ReqType_Exit
   sizeof(REQTYPE) + sizeof( Get_def ),             // ReqType_Get
   sizeof(REQTYPE) + sizeof( InstanceId_def ),      // ReqType_InstanceId
   sizeof(REQTYPE) + sizeof( Kill_def ),            // ReqType_Kill
   sizeof(REQTYPE) + sizeof( MonStats_def ),        // ReqType_MonStats
   sizeof(REQTYPE) + sizeof( Mount_def ),           // ReqType_Mount
   sizeof(REQTYPE) + sizeof( NameServerAdd_def ),   // ReqType_NameServerAdd
   sizeof(REQTYPE) + sizeof( NameServerDelete_def ),// ReqType_NameServerDelete
   sizeof(REQTYPE) + sizeof( NameServerStart_def ), // ReqType_NameServerStart
   sizeof(REQTYPE) + sizeof( NameServerStop_def ),  // ReqType_NameServerStop
   sizeof(REQTYPE) + sizeof( NewProcess_def ),      // ReqType_NewProcess
   sizeof(REQTYPE) + sizeof( NewProcessNs_def ),    // ReqType_NewProcessNs
   sizeof(REQTYPE) + sizeof( NodeAdd_def ),         // ReqType_NodeAdd
   sizeof(REQTYPE) + sizeof( NodeDelete_def ),      // ReqType_NodeDelete
   sizeof(REQTYPE) + sizeof( NodeDown_def ),        // ReqType_NodeDown
   sizeof(REQTYPE) + sizeof( NodeInfo_def ),        // ReqType_NodeInfo
   sizeof(REQTYPE) + sizeof( NodeName_def ),        // ReqType_NodeName
   sizeof(REQTYPE) + sizeof( NodeUp_def ),          // ReqType_NodeUp
   0,                                               // ReqType_Notice -- not an actual request
   sizeof(REQTYPE) + sizeof( Notify_def ),          // ReqType_Notify
   sizeof(REQTYPE) + sizeof( Open_def ),            // ReqType_Open
   sizeof(REQTYPE) + sizeof( OpenInfo_def ),        // ReqType_OpenInfo
   0,                                               // ReqType_PersistAdd (TODO)
   0,                                               // ReqType_PersistDelete (TODO)
   sizeof(REQTYPE) + sizeof( PNodeInfo_def ),       // ReqType_PNodeInfo
   sizeof(REQTYPE) + sizeof( ProcessInfo_def ),     // ReqType_ProcessInfo
   sizeof(REQTYPE) + sizeof( ProcessInfoCont_def ), // ReqType_ProcessInfoCont
   sizeof(REQTYPE) + sizeof( ProcessInfo_def ),     // ReqType_ProcessInfoNs
   sizeof(REQTYPE) + sizeof( Set_def ),             // ReqType_Set
   sizeof(REQTYPE) + sizeof( Shutdown_def ),        // ReqType_Shutdown
   sizeof(REQTYPE) + sizeof( ShutdownNs_def ),      // ReqType_ShutdownNs
   sizeof(REQTYPE) + sizeof( Startup_def ),         // ReqType_Startup
   sizeof(REQTYPE) + sizeof( TmLeader_def ),        // ReqType_TmLeader
   sizeof(REQTYPE) + sizeof( TmReady_def ),         // ReqType_TmReady
   sizeof(REQTYPE) + sizeof( ZoneInfo_def )         // ReqType_ZoneInfo
};

// The serviceReplySize array holds the size of the request messages for
// use by getSizeOfMsg.  The ordering is important and must match
// that of the REPLYTYPE enum in msgdef.h.
const int SQ_LocalIOToClient::serviceReplySize[] = {
   sizeof(REPLYTYPE) + sizeof( Generic_reply_def ),     // ReplyType_Generic
   sizeof(REPLYTYPE) + sizeof( DelProcessNs_reply_def ),// ReplyType_DelProcessNs
   sizeof(REPLYTYPE) + sizeof( Dump_reply_def ),        // ReplyType_Dump
   sizeof(REPLYTYPE) + sizeof( Get_reply_def ),         // ReplyType_Get
   sizeof(REPLYTYPE) + sizeof( InstanceId_reply_def ),  // ReplyType_InstanceId
   sizeof(REPLYTYPE) + sizeof( MonStats_reply_def ),    // ReplyType_MonStats
   sizeof(REPLYTYPE) + sizeof( Mount_reply_def ),       // ReplyType_Mount
   sizeof(REPLYTYPE) + sizeof( NewProcess_reply_def ),  // ReplyType_NewProcess
   sizeof(REPLYTYPE) + sizeof( NewProcessNs_reply_def ),// ReplyType_NewProcessNs
   sizeof(REPLYTYPE) + sizeof( NodeInfo_reply_def ),    // ReplyType_NodeInfo
   sizeof(REPLYTYPE) + sizeof( NodeName_reply_def ),    // ReplyType_NodeName
   sizeof(REPLYTYPE) + sizeof( Open_reply_def ),        // ReplyType_Open
   sizeof(REPLYTYPE) + sizeof( OpenInfo_reply_def ),    // ReplyType_OpenInfo
   sizeof(REPLYTYPE) + sizeof( PNodeInfo_reply_def ),   // ReplyType_PNodeInfo
   sizeof(REPLYTYPE) + sizeof( ProcessInfo_reply_def ), // ReplyType_ProcessInfo
   sizeof(REPLYTYPE) + sizeof( ProcessInfoNs_reply_def ),// ReplyType_ProcessInfoNs
   sizeof(REPLYTYPE) + sizeof( Startup_reply_def ),     // ReplyType_Startup
   sizeof(REPLYTYPE) + sizeof( ZoneInfo_reply_def )     // ReplyType_ZoneInfo
};


// The requestSize array holds the size of the messages for use by
// getSizeOfRequest.  The ordering is important and must match that of
// the MSGTYPE enum in msgdef.h.
const int SQ_LocalIOToClient::requestSize[] = {
   0, // unused, message types start at 1
   sizeof(REQTYPE) + sizeof( Change_def ),            // MsgType_Change
   sizeof(REQTYPE) + sizeof( Close_def ),             // MsgType_Close
   sizeof(REQTYPE) + sizeof( Event_Notice_def ),      // MsgType_Event
   sizeof(REQTYPE) + sizeof( NodeAdded_def ),         // MsgType_NodeAdded
   sizeof(REQTYPE) + sizeof( NodeChanged_def ),       // MsgType_NodeChanged
   sizeof(REQTYPE) + sizeof( NodeDeleted_def ),       // MsgType_NodeDeleted
   sizeof(REQTYPE) + sizeof( NodeDown_def ),          // MsgType_NodeDown
   sizeof(REQTYPE) + sizeof( NodeJoining_def ),       // MsgType_NodeJoining
   sizeof(REQTYPE) + sizeof( NodeQuiesce_def ),       // MsgType_NodeQuiesce
   sizeof(REQTYPE) + sizeof( NodeUp_def ),            // MsgType_NodeUp
   sizeof(REQTYPE) + sizeof( Open_def ),              // MsgType_Open
   sizeof(REQTYPE) + sizeof( NewProcess_Notice_def ), // MsgType_ProcessCreated
   sizeof(REQTYPE) + sizeof( ProcessDeath_def ),      // MsgType_ProcessDeath
   sizeof(REQTYPE) + sizeof( NodeReInt_def ),         // MsgType_ReintegrationError
   0,                                                 // MsgType_Service
   sizeof(REQTYPE) + sizeof( Shutdown_def ),          // MsgType_Shutdown
   sizeof(REQTYPE) + sizeof( SpareUp_def )            // MsgType_SpareUp

};

// this is the definition of the localio object for the monitor
// in most cases a check to see if this is not NULL is made before
// any of the localio functions are executed from the monitor code
SQ_LocalIOToClient *SQ_theLocalIOToClient = NULL;


const char *
SQ_LocalIOToClient::getTypeStr(int type)
{
    const char *type_str;
    switch (type)
    {
    case MC_ReadySend:
        type_str = "ReadySend";
        break;
    case MC_NoticeReady:
        type_str = "NoticeReady";
        break;
    case MC_SReady:
        type_str = "SReady";
        break;
    case MC_AttachStartup:
        type_str = "AttachStartup";
        break;
    case MC_NoticeClear:
        type_str = "NoticeClear";
        break;
    default:
        type_str = "<unknown>";
        break;
    }
    return type_str;
}

// this method is used to send real time signals to the seabed/shell
// processes.  TODO: I had problems trying to remove osPID in this file
// It works this way so I kept it.  osPID really should not be necessary.
int
SQ_LocalIOToClient::sendCtlMsg( 
  int osPID, 
  MonitorCtlType type, // see localio.h for these values
  int data, 
  int *error
)
{
  sigval value;
  const char method_name[] = "SQ_LocalIOToClient::sendCtlMsg";

  value.sival_int = (0xfffff & data) << 8 | (0xff & type);

  if (trace_settings & TRACE_MLIO_DETAIL)
  {
      SharedMsgDef *shm = (SharedMsgDef *)(clientBuffers+sizeof(SharedMemHdr)+
                                           (data*sizeof(SharedMsgDef)));
      trace_printf("%s@%d" " sending ctl msg: type=%d (%s), data=%d, value=%x, pid=%d, idx=%d, monitor pid=%d, msg type=%d\n", method_name, __LINE__, type, getTypeStr(type), data, value.sival_int, osPID, data, ((SharedMemHdr*)clientBuffers)->mPid, shm->msg.type);
  }

  int err = 0;
  int rc;
  do
  {
    errno = 0;
    rc = sigqueue( osPID, SQ_LIO_SIGNAL_REQUEST_REPLY, value );
    if ( rc != 0 )
    {
        err = errno;
        if (err == EAGAIN)
        {
            if (trace_settings & TRACE_MLIO_DETAIL)
               trace_printf("%s@%d" "retrying sigqueue(), errno = EAGAIN" "\n", method_name, __LINE__);
        }
        else
        {
            if ( error != NULL )
            {
                *error = err;
            }
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error= Can't queue signal to pid=%d! - errno=%d (%s)\n", method_name, osPID, err, strerror(err));
            mon_log_write(MON_MLIO_SENDCTL_1, SQ_LOG_ERR, la_buf);
        }
        assert(rc == 0 || err == ESRCH  || err == EPERM || err == EAGAIN);
    }
  } while( err == EAGAIN );
  
  if (trace_settings & TRACE_MLIO_DETAIL)
     trace_printf("%s@%d" " - Exit, rc=%d" "\n", method_name, __LINE__, rc);

  return(rc);
}

// signal the pendingNoticeThread that there is work to do
void SQ_LocalIOToClient::nudgeNotifier ( void )
{
    pendingNoticesLock_.lock();

    noticeSignaled = true;
    pendingNoticesLock_.wakeOne();

    pendingNoticesLock_.unlock();
}


// This routine puts the notice on a list to be sent by the
// pendingNoticeThread.
void 
SQ_LocalIOToClient::putOnNoticeQueue( 
  int                osPID, 
  Verifier_t         verifier,
  struct message_def *notice,
  bcastPids_t        *bcastPids
)
{
    const char method_name[] = "SQ_LocalIOToClient::putOnNoticeQueue";
    TRACE_ENTRY;
    int size = 0;

    if (trace_settings & (TRACE_TMSYNC | TRACE_PROCESS_DETAIL | TRACE_NOTICE | TRACE_NOTICE_DETAIL))
        trace_printf( "%s@%d queueing notice for pid: %d\n",
                      method_name, __LINE__, osPID );

    PendingNotice pn;
    pn.msg = notice;
    pn.pid = osPID;
    pn.verifier = verifier;
    pn.bcastPids = bcastPids;

    // Protect access to pending notice data structure during modification.
    // This structure is accessed from multiple threads.
    pendingNoticesLock_.lock();

    switch ( notice->type )
    {
        case MsgType_Event:
            switch ( notice->u.request.u.event_notice.event_id )
            {
                case Watchdog_Expire:
                case Watchdog_Refresh:
                case Watchdog_Start:
                case Watchdog_Stop:
                    // Add new notice to the front of the list
                    pendingNotices_.push_front( pn );
                    break;
                default:
                    // Add new notice to the back of the list
                    pendingNotices_.push_back( pn );
                    break;
            }
            break;
        case MsgType_NodeQuiesce:
            // Add new notice to the front of the list
            pendingNotices_.push_front( pn );
            break;
        default:
            // Add new notice to the back of the list
            pendingNotices_.push_back( pn );
            break;
    }

    // signal the pendingNoticeThread that there is work to do
    noticeSignaled = true;
    pendingNoticesLock_.wakeOne();

    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_RECOVERY | TRACE_TMSYNC | TRACE_PROCESS_DETAIL | TRACE_NOTICE_DETAIL))
    {
        size = pendingNotices_.size(); // used in tracing below
    }

    // Release data structure protection.
    pendingNoticesLock_.unlock();
    
    if (trace_settings & (TRACE_TMSYNC | TRACE_PROCESS_DETAIL | TRACE_NOTICE_DETAIL))
        trace_printf( "%s@%d waking pendingNoticeThread to dispose of %d notices\n", method_name, __LINE__, size );

    TRACE_EXIT;
}

void SQ_LocalIOToClient::manageNotice ( int msgIndex, CNoticeMsg * noticeMsg )
{
    const char method_name[] = "SQ_LocalIOToClient::manageNotice";
    TRACE_ENTRY;

    pair<noticeMap_t::iterator, bool> ret1;

    noticeMapLock_.lock();
    ret1 = noticeMap_.insert ( noticeMap_t::value_type ( msgIndex, noticeMsg ));
    noticeMapLock_.unlock();

    if (ret1.second == false)
    {   // Unexpectedly, already had an entry with the given key value.
        if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
        {
            trace_printf("%s@%d noticeMap already contained buffer, idx=%d\n",
                         method_name, __LINE__, msgIndex );
        }

        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], noticeMap already contained buffer, idx=%d\n", 
                method_name, msgIndex);
        mon_log_write(MON_MLIO_MANAGE_NOTICE_1, SQ_LOG_ERR, buf);
    }
    else if (trace_settings & TRACE_NOTICE_DETAIL)
    {
        trace_printf("%s@%d added buffer idx=%d, noticeMap now has %d entries\n",
                     method_name, __LINE__, msgIndex, (int) noticeMap_.size());
    }

    TRACE_EXIT;
}

void SQ_LocalIOToClient::noticeCompleted ( int msgIndex )
{
    const char method_name[] = "SQ_LocalIOToClient::noticeCompleted";
    TRACE_ENTRY;

    CNoticeMsg *noticeMsg = NULL;
    noticeMap_t::iterator it;

    noticeMapLock_.lock();

    it = noticeMap_.find ( msgIndex );
    if (it != noticeMap_.end())
    {   // Cleanup entry found for the buffer in the notice map
        noticeMsg = it->second;
        noticeMap_.erase ( it );
    }

    noticeMapLock_.unlock();

    if ( noticeMsg )
    {
        delete noticeMsg;
    }

    TRACE_EXIT;
}

void SQ_LocalIOToClient::sendNotice(SharedMsgDef *msg, PendingNotice &pn)
                         throw( int, std::exception )
{
    const char method_name[] = "SQ_LocalIOToClient::sendNotice";
    TRACE_ENTRY;

    unsigned int       msgSize;
    int                rc;

    msgSize = getSizeOfMsg( pn.msg );
    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->localio_messagebytes(msgSize);

    if ( msgSize > sizeof ( message_def ) )
    {   // Not expected to occur but guard against client buffer overrun
        msgSize = sizeof ( message_def );
    }

    // copy the notice to the client buffer
    memcpy( msg, pn.msg, msgSize );

    if ( pn.pid == BCAST_PID )
    {   // Broadcast notice

        // Send the notice to each pid in the set of os pids created in
        // CProcess::Bcast().
        bcastPids_t *bcastPids = pn.bcastPids;
        if ( bcastPids )
        {
            // Verify that each process in the broadcast list is still
            // alive.  Remove any that are not alive from the
            // broadcast list.
            pidVerifier_t pv;
            for (bcastPids_t::const_iterator it = bcastPids->begin();
                 it != bcastPids->end();)
            {
                pv.pnv = *it;
                ++it;

                if (kill((pid_t)pv.pv.pid,0) == -1 && errno == ESRCH) 
                {
                    if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                        trace_printf( "%s@%d No process associated with "
                                      "notice, decrementing shared buffer, "
                                      "pid=%d:%d, idx=%d\n"
                                    , method_name, __LINE__
                                    , pv.pv.pid
                                    , pv.pv.verifier
                                    , msg->trailer.index );
                    // Target process no longer exists so remove it from
                    // the list of pids receiving this notice.
                    bcastPids->erase ( pv.pnv );
                }
            }

            if ( !bcastPids->empty() )
            {

                if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                    trace_printf( "%s@%d broadcasting notice to %d processes"
                                  " using local io buffer=%d\n"
                                , method_name, __LINE__
                                , (int)bcastPids->size()
                                , msg->trailer.index);

                bcastPids_t bcastPidsCopy;
                // Use a copy of the broadcast pid list to send control
                // messages to the processes.   Those processes may start
                // responding with NoticeClear responses before we have
                // finished broadcasting to all and the NoticeClear could
                // modify the broadcast list.
                try 
                {
                    bcastPidsCopy = *bcastPids;
                }
                catch( std::exception &stlException )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    sprintf(buf, "[%s], STL exception on set copy (bcastPidsCopy): %s!\n", method_name, stlException.what());
                    mon_log_write(MON_MLIO_SEND_NOTICE_2, SQ_LOG_ERR, buf);
                    throw stlException;
                }
                catch( ... )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    sprintf(buf, "[%s], Unknown exception on set copy (bcastPidsCopy)!\n", method_name);
                    mon_log_write(MON_MLIO_SEND_NOTICE_2, SQ_LOG_ERR, buf);
                    throw 99;
                }

                // Keep track of this notice and retain the set of
                // process ids that will receive the broadcast.  This
                // set is used to manage the client buffer reference
                // count when "notice clear" replies arrive from the
                // broadcast clients.
                CNoticeMsg *       noticeMsg;
                int msgIndex = msg->trailer.index;

                if (trace_settings & TRACE_NOTICE_DETAIL)
                {
                    trace_printf( "%s@%d Creating notice message: index=%d, pid=%d, verifier=%d\n"
                                , method_name, __LINE__
                                , msgIndex
                                , pn.pid
                                , pn.verifier );
                }

                noticeMsg = new CNoticeMsg( msgIndex
                                          , msg
                                          , pn.pid
                                          , pn.verifier
                                          , bcastPids );
                manageNotice ( msgIndex, noticeMsg );

                // Notify all processes in the broadcast pid list that
                // the notice is available.
                for (bcastPids_t::const_iterator it = bcastPidsCopy.begin();
                     it != bcastPidsCopy.end();)
                {
                    pv.pnv = *it;
                    ++it;

                    if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                        trace_printf( "%s@%d Sending notice to pid=%d:%d, t=%d\n",
                                      method_name, __LINE__, pv.pv.pid, pv.pv.verifier, msg->msg.type );

                    // send a notice ready control message to the client
                    rc = sendCtlMsg( pv.pv.pid, MC_NoticeReady, msg->trailer.index );
                    if ( rc )
                    {
                        if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                            trace_printf( "%s@%d failed broadcast send to target process for notice, pid=%d:%d\n",
                                          method_name, __LINE__, pv.pv.pid, pv.pv.verifier );
                        // Could not send the notice to the target process
                        // so remove it from the list of pids receiving
                        // the notice.
                        decrNoticeMsgRef ( msgIndex, pv.pv.pid, pv.pv.verifier );
                    }
                }
            }
            else
            { // The broadcast message has no viable clients.
                delete bcastPids;
                // Return shared buffer to free pool
                releaseMsg ( msg, true);
            }
        }
        else
        {   // Unexpectedly, there is no pointer to set of process ids
            if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                trace_printf( "%s@%d Bcast pid list not found\n",
                              method_name, __LINE__ );

            // Return shared buffer to free pool
            releaseMsg ( msg, true);
            
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], Bcast pid list not found\n", method_name);
            mon_log_write(MON_MLIO_SEND_NOTICE_1, SQ_LOG_ERR, buf);
        }
    }
    else // Send notice to a single client.
    {
        // check if the process exists
        if (kill((pid_t)pn.pid,0) == -1 && errno == ESRCH) 
        {   // Process does not exist
            if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                trace_printf( "%s@%d no target process for notice, pid=%d\n",
                              method_name, __LINE__, pn.pid);
            // Could not send the notice to the target process
            noticeCompleted ( msg->trailer.index );
            // Return shared buffer to free pool
            releaseMsg ( msg, true);
        }
        else
        {
            // Add a notice message object to the set of those
            // being managed by the monitor.
            CNoticeMsg *       noticeMsg;

            if (trace_settings & TRACE_NOTICE_DETAIL)
            {
                trace_printf( "%s@%d Creating notice message: index=%d, pid=%d, verifier=%d\n"
                            , method_name, __LINE__
                            , msg->trailer.index
                            , pn.pid
                            , pn.verifier );
            }

            noticeMsg = new CNoticeMsg( msg->trailer.index
                                      , msg
                                      , pn.pid
                                      , pn.verifier
                                      , NULL );
            manageNotice ( msg->trailer.index, noticeMsg );
 
            if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
            {
                trace_printf( "%s@%d Sending notice to process %d:%d, t=%d\n",
                              method_name, __LINE__, pn.pid, pn.verifier, msg->msg.type );
            }

            rc = sendCtlMsg( pn.pid, MC_NoticeReady, msg->trailer.index );
            if ( rc )
            {   
                if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                    trace_printf( "%s@%d Failed sending notice to process %d:%d\n",
                                  method_name, __LINE__, pn.pid, pn.verifier);
                // Could not send the notice to the target process
                noticeCompleted ( msg->trailer.index );
                // Return shared buffer to free pool
                releaseMsg ( msg, true);
            }
        }
    }

    // Free the message image allocated by notice originator.
    delete pn.msg;

    TRACE_EXIT;
}

void SQ_LocalIOToClient::handleSSMPNotices()
{
    const char method_name[] = "SQ_LocalIOToClient::handleSSMPNotices";
    TRACE_ENTRY;

    // Check all logical nodes in this physical node.   Each logical
    // node may have an assocated SSMP process and that process may
    // have notices waiting to be delivered to it.
    CProcess * ssmProc;
    CLNode *lnode = MyNode->GetFirstLNode();
    struct message_def *notice;
    SharedMsgDef *msg;
    for ( ; lnode ; lnode = lnode->GetNextP() )
    {
        ssmProc = lnode->GetSSMProc();
        if ( ssmProc )
        {
            // Deliver any notices queued for the SSM process.
            while ( (notice = ssmProc->GetDeathNotice()) != NULL )
            {
                if ( ssmProc->GetState() == State_Up )
                {
                    PendingNotice pn;
                    pn.msg = notice;
                    pn.pid = ssmProc->GetPid();
                    pn.verifier = ssmProc->GetVerifier();
                    pn.bcastPids = NULL;

                    // try to acquire a client buffer
                    msg = (SharedMsgDef *) acquireMsg( pn.pid, pn.verifier );
                    if (msg)
                    {
                        // deliver notice
                        sendNotice (msg, pn);
                    }
                    else
                    {   // Could not get a client buffer.  Requeue the notice.
                        ssmProc->PutDeathNotice( notice );

                        if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                            trace_printf( "%s@%d buffer not available for "
                                          "sending notice\n",
                                          method_name, __LINE__);
                        // Could not get a client buffer.
                        // Stop processing notices for now.  
                        // pendingNoticeThread will be reawakened once a 
                        // client buffer has been freed.
                        break;
                    }
                }
                else
                {   // Requeue the notice.
                    ssmProc->PutDeathNotice( notice );

                    if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                    {
                        trace_printf( "%s@%d SSMP process for logical node %d "
                                      "is not UP.  Deferring notices delivery.\n",
                                      method_name, __LINE__, lnode->GetNid());
                    }
                    break;
                }

                // If ssmProc exited in the middle of this loop, checking it again would
                // narrow the synchronization window.
                ssmProc = lnode->GetSSMProc();
                if ( !ssmProc )
                {
                    break;
                }
            }
        }
    }

    TRACE_EXIT;
}

// This method is called from the pending notice thread to handle the
// notices that have been placed on the pending notice list
void SQ_LocalIOToClient::processNotices() throw()
{
    const char method_name[] = "SQ_LocalIOToClient::processNotices";
    TRACE_ENTRY;

    SharedMsgDef *msg;
    PendingNotice pn;

    handleSSMPNotices();

    while (!pendingNotices_.empty())
    {
        // Protect access to notice map data structures during modification.
        // These structures are accessed from multiple threads.
        pendingNoticesLock_.lock();

        pn = pendingNotices_.front( );

        // try to acquire a client buffer
        msg = (SharedMsgDef *) acquireMsg( pn.pid, pn.verifier );
        if (msg)
        {
            pendingNotices_.pop_front( );

            // Release data structure protection.
            pendingNoticesLock_.unlock();

            try 
            {
                // Send the notice to the client
                sendNotice (msg, pn);
            }
            catch( std::exception &stlException )
            {
                if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                    trace_printf( "%s@%d sendNotice STL exception caught!\n",
                                  method_name, __LINE__);
                // Return shared buffer to free pool
                releaseMsg ( msg, true);
                pendingNoticesLock_.lock();
                pendingNotices_.push_front( pn );
                pendingNoticesLock_.unlock();
            }
            catch( int &intException )
            {
                if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                    trace_printf( "%s@%d sendNotice %d exception caught!\n",
                                  method_name, __LINE__, intException );
                // Return shared buffer to free pool
                releaseMsg ( msg, true);
                pendingNoticesLock_.lock();
                pendingNotices_.push_front( pn );
                pendingNoticesLock_.unlock();
            }
        }
        else
        {
            // Release data structure protection.
            pendingNoticesLock_.unlock();

            if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
                trace_printf( "%s@%d buffer not available for sending notice\n",
                              method_name, __LINE__);
            // Could not get a client buffer.
            // Stop processing notices for now.  pendingNoticeThread will be
            // reawakened once a client buffer has been freed.
            break;
        }
    }

    TRACE_EXIT;
}

// this method is called from the processThread when a rt signal arrives.
void 
SQ_LocalIOToClient::processLocalIO( siginfo_t *siginfo )
{
  struct message_def *msg = NULL;
  SharedMsgDef *cmsg = NULL;
  int cidx = -1;

  const char method_name[] = "SQ_LocalIOToClient::processLocalIO";
  TRACE_ENTRY;

  // get the control message in the lowest order byte of the payload
  MonitorCtlType type = (MonitorCtlType)(siginfo->si_int & 0xff);

  // get the index for the client buffer by shifting out the low order byte
  cidx = ((siginfo->si_int >> 8) & 0xfffff);
  cmsg = ((SharedMsgDef *)(clientBuffers+sizeof(SharedMemHdr)+
                           (cidx*sizeof(SharedMsgDef))));
  cmsg->trailer.index = cidx;
  msg = &cmsg->msg;
  if (trace_settings & TRACE_MLIO_DETAIL)
  {
      const char *type_str = getTypeStr(type);
      trace_printf("%s@%d" " got ctl msg: type=%d(%s), value=0x%x, pid=%d, idx=%d, trailer.idx=%d\n", method_name, __LINE__, type, type_str, siginfo->si_int, siginfo->si_pid, cidx, cmsg->trailer.index);
  }

  // Record statistics (sonar counters)
  long msgSize = getSizeOfRequest( msg );
  if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
     MonStats->localio_messagebytes(msgSize);

  switch( type )
  {
    case MC_NoticeClear:
      if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
        trace_printf( "%s@%d" " Got Notice Clear from client, si_pid=%d, buf pid=%d, buf verifier=%d, idx=%d, cidx=%d\n"
                    , method_name, __LINE__
                    , siginfo->si_pid
                    , cmsg->trailer.OSPid
                    , cmsg->trailer.verifier
                    , cmsg->trailer.index
                    , cidx);

      if (!decrNoticeMsgRef( cmsg->trailer.index
                           , siginfo->si_pid
                           , cmsg->trailer.verifier ))
      {
          if (trace_settings & (TRACE_NOTICE | TRACE_NOTICE_DETAIL))
          { // Unexpectedly, could not find notice associated with the buffer
              trace_printf("%s@%d Got notice clear but could not locate "
                           "associated notice, idx=%d\n", method_name,
                           __LINE__, cmsg->trailer.index);
          }
      }
      break;
      

    case MC_SReady:
    case MC_AttachStartup:
    {
      int pid = cmsg->trailer.OSPid;
      Verifier_t verifier = cmsg->trailer.verifier;

      if (trace_settings & (TRACE_MLIO_DETAIL | TRACE_REQUEST_DETAIL))
      {
          trace_printf( "%s@%d Got %s from client, pid=%d, verifier=%d, si_pid=%d\n"
                      , method_name, __LINE__
                      , (type == MC_SReady ? "message" : "Attach Startup")
                      , pid
                      , verifier
                      , siginfo->si_pid);
      }
      assert( msg );

      if ( MyNode->IsSpareNode() || 
          (MyNode->GetState() != State_Up && MyNode->GetState() != State_Shutdown) )
      {  // Reject request for any of the above reasons 
          if (trace_settings & (TRACE_MLIO | TRACE_REQUEST))
          {
              trace_printf("%s@%d Spare node rejecting request\n",
                           method_name, __LINE__);
          }

          if (type == MC_AttachStartup)
          {
              pid = siginfo->si_pid;
              msg->u.reply.type = ReplyType_Startup;
              msg->u.reply.u.startup_info.nid = -1;
              msg->u.reply.u.startup_info.pid = -1;
              msg->u.reply.u.startup_info.verifier = -1;
              msg->u.reply.u.startup_info.process_name[0] = '\0';
              msg->u.reply.u.startup_info.return_code = MPI_ERR_OP;
          }
          else
          {
              msg->u.reply.type = ReplyType_Generic;
              msg->u.reply.u.generic.nid = -1;
              msg->u.reply.u.generic.pid = -1;
              msg->u.reply.u.generic.verifier = -1;
              msg->u.reply.u.generic.process_name[0] = '\0';
              msg->u.reply.u.generic.return_code = MPI_ERR_OP;
          }

          sendCtlMsg( pid, MC_SReady, ((SharedMsgDef *)msg)->trailer.index );
      }
      else
      {
          CExternalReq::reqQueueMsg_t msgType;

          // Gather request specific information
          if (type == MC_AttachStartup)
          {
              pid = siginfo->si_pid;
              msgType = CExternalReq::AttachStartupMsg;
          }
          else if (msg->type == MsgType_Service && 
                   msg->u.request.type == ReqType_Startup )
          {
              msgType = CExternalReq::StartupMsg;
          }
          else
          {
              msgType = CExternalReq::NonStartupMsg;
          }

          // Place new request on request queue
          ReqQueue.enqueueReq(msgType, -1, pid, -1, msg);
      }

      break;
    }


    default:
      if (trace_settings & TRACE_MLIO)
         trace_printf("%s@%d" " Unknown MonitorCtlType: "  "%d" "\n", method_name, __LINE__, type);
      break;
  }

  TRACE_EXIT;
}



// The request process thread is not used for concurrent requests.  Only the
// serial request thread is used.

// This is the serial request process thread.  There is only 1 of these.
// A specific realtime signal is used by client processes to route request
// to this thread for processing in order of arrival since real time signals
// are queued until delivered.
void *
serialRequestThread( void * )
{
    sigset_t          sig_set;
    siginfo_t         sig_info;
    int               sig;
    const char method_name[] = "serialRequestThread";

    // Setup signal handling
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGCHLD);
    sigaddset(&sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
    int rc = pthread_sigmask(SIG_BLOCK, &sig_set, NULL);
    if( rc )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't set blocking signal mask for thread! - errno=%d (%s)\n", method_name, rc, strerror(rc));
        mon_log_write(MON_MLIO_SERIALREQ_TH_1, SQ_LOG_CRIT, la_buf);

        mon_failure_exit();
    }

    // Record statistics (sonar counters): monitor is busy
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->MonitorBusyIncr();

    // Setup signal handling
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
    sigaddset(&sig_set, SIGUSR1);

    if (trace_settings & TRACE_MLIO)
       trace_printf("%s@%d" " Thread started, shutdown=%d\n", method_name, __LINE__, SQ_theLocalIOToClient->isShutdown());
       
    // until there is a monitor shutdown
    while (!SQ_theLocalIOToClient->isShutdown()) 
    {
        // Record statistics (sonar counters): monitor is NOT busy
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->MonitorBusyDecr();

        // Wait for a request to arrive or for shutdown to be signaled.
        sig = sigwaitinfo ( &sig_set, &sig_info );

        // Record statistics (sonar counters): monitor is busy
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->MonitorBusyIncr();

        if (sig == -1)
        {   // sigwaitinfo returned an error
            int err = errno;
            if (err != EAGAIN)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], Error= Can't wait for signal! - errno=%d (%s)\n", method_name, err, strerror(err));
                mon_log_write(MON_MLIO_SERIALREQ_TH_2, SQ_LOG_ERR, la_buf);
            }
            assert(err == EAGAIN || err == EINTR);
        }
        else if (!SQ_theLocalIOToClient->isShutdown() && sig != SIGUSR1 )
        {
            if (trace_settings & TRACE_MLIO_DETAIL)
                trace_printf("%s@%d got signal: %d, OSpid: %d, uid: %d, "
                             "val: %x\n", method_name, __LINE__, sig,
                             sig_info.si_pid, sig_info.si_uid, sig_info.si_int);

            SQ_theLocalIOToClient->processLocalIO( &sig_info );
        }
    }


    // Record statistics (sonar counters): monitor is NOT busy
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->MonitorBusyDecr();

    if (trace_settings & TRACE_MLIO)
       trace_printf("%s@%d" " EXIT thread" "\n", method_name, __LINE__);
    pthread_exit((void *) errno); // cast
    return (void *) errno; // cast
}

// This is the pending notice thread used to send unsent notices to the client.
void *
pendingNoticeThread( void * )
{
  sigset_t sig_set;
  const char method_name[] = "pendingNoticeThread";

  // Setup signal handling
  sigemptyset(&sig_set);
  sigaddset(&sig_set, SIGCHLD);
  sigaddset(&sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
  int rc = pthread_sigmask(SIG_BLOCK, &sig_set, NULL);
  if( rc )
  {
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Error= Can't set blocking signal mask for thread! - errno=%d (%s)\n", method_name, rc, strerror(rc));
      mon_log_write(MON_MLIO_PENDING_TH_1, SQ_LOG_CRIT, la_buf);

      mon_failure_exit();
  }

  sigemptyset(&sig_set);
  sigaddset(&sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);

  if (trace_settings & TRACE_NOTICE)
     trace_printf("%s@%d" " Thread started, shutdown=%d\n", method_name, __LINE__, SQ_theLocalIOToClient->isShutdown());
     
  while (!SQ_theLocalIOToClient->isShutdown())
  {
    SQ_theLocalIOToClient->processNotices();

    // Wait until signalled that there is more work to do or that
    // we should shutdown.
    SQ_theLocalIOToClient->waitForNoticeWork();

  }
  if (trace_settings & TRACE_NOTICE)
     trace_printf("%s@%d" " EXIT thread" "\n", method_name, __LINE__);
  pthread_exit( (void *)errno );
  return (void *)errno;
}

void *
lioBufCleanupThread( void * )
{
    const char method_name[] = "lioBufCleanupThread";

    if (trace_settings & TRACE_MLIO)
       trace_printf("%s@%d Thread started\n", method_name, __LINE__);

    // Mask all allowed signals except SIGUSR1 and SIGPROF
    sigset_t              mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler

    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], pthread_sigmask error=%d\n", method_name, rc);
        mon_log_write(MON_MLIO_PROCESS_TH_1, SQ_LOG_ERR, buf);
    }

    sigset_t          sig_set;
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGUSR1);

    // until there is a monitor shutdown
    while (!SQ_theLocalIOToClient->isShutdown()) 
    {
        if ( SQ_theLocalIOToClient->noDeadPids() )
        {
            // Wait until signaled.
            sigwaitinfo ( &sig_set, NULL );
        }

        SQ_theLocalIOToClient->recycleProcessBufs();
    }       

    if (trace_settings & TRACE_MLIO)
       trace_printf("%s@%d EXIT thread\n", method_name, __LINE__);
    pthread_exit((void *) errno);
    return (void *) errno;
}


// the monitor localio constructor
SQ_LocalIOToClient::SQ_LocalIOToClient(int nid)
                   :
                   shutdown(false),
                   noticeSignaled(false),
                   Nid(nid),
                   nidBase(0),
                   clientBuffers(NULL),
                   qid(0),
                   acquiredBufferCount(0),
                   acquiredBufferCountMax(0),
                   availableBufferCountMin(0),
                   missedBufferCount(0),
                   sharedBuffersMax(SQ_LIO_MAX_BUFFERS),
                   serialRequestTid_(0),
                   pendingNoticesTid_(0),
                   lioBufCleanupTid_(0),
                   sharedSegKeyBase(0)
                   , deadPidsHead_(0)
                   , deadPidsTail_(0)
                   , deadPidsMax_(0)
                   , deadPidsOverflow_(0)
                   , almostDeadPidsHead_(0)
                   , almostDeadPidsTail_(0)
                   , almostDeadPidsTotal_(0)
                   , almostDeadPidsHandled_(0)
                   , almostDeadPidsDeferred_(0)
                   , almostDeadPidsError_(0)
{
  char *ptr;
  int   nodes;
  int   rc;
  const char method_name[] = "SQ_LocalIOToClient::SQ_LocalIOToClient";

  // Add eyecatcher sequence as a debugging aid
  memcpy(&eyecatcher_, "MLIO", 4);

  nextNoticeCheck_.tv_sec = 0;
  nextNoticeCheck_.tv_nsec = 0;

  
  ptr = getenv("SQ_VIRTUAL_NODES");
  if ( ptr )
  {
    nidBase = Nid;
    nodes = atoi( ptr );
    if (trace_settings & TRACE_INIT)
       trace_printf("%s@%d virtual nodes=%d\n", method_name, __LINE__, nodes);
    if ( nodes <= 0 )
      nodes = 1;
  } 
  else
  {
    // It's a real cluster
    nidBase = 0;
    nodes = Monitor->GetConfigPNodesCount();
  }

  ptr = getenv( "SQ_LIO_MAX_BUFFERS" );
  if (ptr) 
  {
      int nb = atoi( ptr );
      if (nb > 0)
      {
          sharedBuffersMax = nb;

          if (trace_settings & (TRACE_INIT | TRACE_MLIO))
          {
              trace_printf("%s@%d Allocating %d shared buffers as specified "
                           "by SQ_LIO_MAX_BUFFERS environment variable.\n",
                           method_name, __LINE__, sharedBuffersMax);
          }
      }
  }

  // Compute the shared segment base address that will be used by local
  // io.  This monitor's MPI port number is the primary component of the
  // base address.  If we are using virtual nodes the virtual node id
  // augments to the MPI port number.
  unsigned long int myPortNum;
  errno = 0;
  char *pPort;
  switch( CommType )
  {
      case CommType_InfiniBand:
          pPort = strstr(MyCommPort, "$port#");
          if (pPort) pPort += 5;
          break;
      case CommType_Sockets:
          pPort = strchr(MyCommPort, ':');
          break;
      default:
          // Programmer bonehead!
          abort();
  }
  if (pPort == NULL)
  {
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Monitor port does not contain ':' (%s)\n",
              method_name, MyCommPort);
      mon_log_write(MON_MLIO_INIT_2, SQ_LOG_ERR, la_buf);

      mon_failure_exit();
  }

  myPortNum = strtoul(&pPort[1], NULL, 10);
  if (errno != 0)
  {
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Cannot convert MyCommPort - errno=%d (%s)\n",
              method_name, errno, strerror(errno));
      mon_log_write(MON_MLIO_INIT_3, SQ_LOG_ERR, la_buf);

      mon_failure_exit();
  }
  sharedSegKeyBase = (key_t) ((nidBase << 28) + (myPortNum & 0xFFFF));
  if (myPortNum > 65535)
  {
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], MyCommPort value exceeds 16 bits\n", method_name);
      mon_log_write(MON_MLIO_INIT_4, SQ_LOG_ERR, la_buf);

      mon_failure_exit();
  }
  
  availableBufferCountMin = sharedBuffersMax;

  // Retain values for the sizes of shared memory structures.   May be useful
  // for debugging.
  sharedMemHdrSize_ = sizeof( SharedMemHdr );
  sharedBufferSize_ = sizeof( SharedMsgDef );

  if (trace_settings & TRACE_INIT)
     trace_printf("%s@%d nodes=%d, nid=%d, nidBase=%d, sharedSegKey=0x%x\n", method_name, __LINE__, nodes, Nid, nidBase, sharedSegKeyBase + 0x10000);

  int shsize = (int) ((sharedBuffersMax * sizeof( SharedMsgDef )) + 
                sizeof( SharedMemHdr ));

  if (trace_settings & TRACE_INIT)
     trace_printf("%s@%d number of buffers=%d, shared mem header size=%u, shared buffer size=%u, shared size=%d\n", method_name, __LINE__,
                  sharedBuffersMax, (int)sizeof(SharedMemHdr), (int)sizeof(SharedMsgDef), shsize);

  long enableHugePages;
  int lv_shmFlag = SHM_NORESERVE | IPC_CREAT | IPC_EXCL | SQ_LIO_SHM_PERMISSIONS;
  char *envShmHugePages = getenv("SQ_ENABLE_HUGEPAGES");
  if (envShmHugePages != NULL)
  {
     enableHugePages = (long) atoi(envShmHugePages);
     if (enableHugePages > 0)
     {
     // Round it 2 MB - Huge page size
     // Possible get the HugePageSize and adjust it accordingly
        shsize = (shsize + (2*1024*1024)) >> 22  << 22;
        if (shsize == 0)
            shsize = 2*1024*1024;
        lv_shmFlag =  lv_shmFlag | SHM_HUGETLB;
     }
  }

  cmid = shmget( sharedSegKeyBase + 0x10000, shsize, lv_shmFlag);
  if (cmid == -1)
  {
      if (trace_settings & TRACE_INIT)
      {
          int err = errno;
          trace_printf( "%s@%d" " failed shmget(%d), errno=%d (%s)\n"
                      , method_name, __LINE__
                      , (shsize), err, strerror(err) );
      }
      if ( errno == EEXIST)
      {
          // and try getting it with a smaller size
          int shsizesm =  sizeof( SharedMemHdr );
          cmid = shmget( sharedSegKeyBase + 0x10000,
                         shsizesm,
                         SHM_NORESERVE | IPC_CREAT | SQ_LIO_SHM_PERMISSIONS );
          if (cmid != -1)
          {
              if (trace_settings & TRACE_INIT)
                 trace_printf("%s@%d" " removing existing shared segment shmctl("  "%d" ")" "\n", method_name, __LINE__, cmid);
              int rc = shmctl( cmid, IPC_RMID, NULL );
              if ( rc  != -1)
              {
                  // now try to recreate it with the original size
                  cmid = shmget( sharedSegKeyBase + 0x10000, shsize,
                                 SHM_NORESERVE | IPC_CREAT | SQ_LIO_SHM_PERMISSIONS );
                  if (cmid == -1)
                  {
                      int err = errno;
                      char la_buf[MON_STRING_BUF_SIZE];
                      sprintf(la_buf, "[%s], Error= Can't access shared memory segment! - errno=%d (%s)\n", method_name, err, strerror(err));
                      mon_log_write(MON_MLIO_INIT_5, SQ_LOG_CRIT, la_buf);

                      mon_failure_exit();
                  }
              }
              else
              {
                  int err = errno;
                  char la_buf[MON_STRING_BUF_SIZE];
                  sprintf(la_buf, "[%s], Error= Can't remove existing shared memory segment! - errno=%d (%s)\n", method_name, err, strerror(err));
                  mon_log_write(MON_MLIO_INIT_6, SQ_LOG_CRIT, la_buf);

                  mon_failure_exit();
              }
          }
          else
          {
              int err = errno;
              char la_buf[MON_STRING_BUF_SIZE];
              sprintf(la_buf, "[%s], Error= Can't access shared memory segment! - errno=%d (%s)\n", method_name, err, strerror(err));
              mon_log_write(MON_MLIO_INIT_7, SQ_LOG_CRIT, la_buf);

              mon_failure_exit();
          }
      }
      else
      {
          int err = errno;
          char la_buf[MON_STRING_BUF_SIZE];
          sprintf(la_buf, "[%s], Error= Can't access shared memory segment! - errno=%d (%s)\n", method_name, err, strerror(err));
          mon_log_write(MON_MLIO_INIT_8, SQ_LOG_CRIT, la_buf);

          mon_failure_exit();
      }
  }
  
  if (trace_settings & TRACE_INIT)
     trace_printf("%s@%d" " shared-memory-id=%d, size=%d, key=0x%x\n", method_name, __LINE__, cmid, shsize, sharedSegKeyBase + 0x10000);
  // Attach share segment to process' address space
  clientBuffers = (char *)shmat( cmid, NULL, 0 );
  if (clientBuffers == (void *)-1)
  {
      int err = errno;
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Error= Can't map shared memory segment address! - errno=%d (%s)\n", method_name, err, strerror(err));
      mon_log_write(MON_MLIO_INIT_9, SQ_LOG_CRIT, la_buf);

      mon_failure_exit();
  }

  memset( clientBuffers, 0, shsize );

  if (trace_settings & TRACE_INIT)
      trace_printf("%s@%d local io buffers start at=%p, number of buffers=%d, "
                   "shared mem header size=%d, shared buffer size=%d\n",
                   method_name, __LINE__, (void *)clientBuffers,
                   sharedBuffersMax,
                   (int)sharedMemHdrSize_, (int)sharedBufferSize_);

  ((SharedMemHdr*)clientBuffers)->mPid = getpid();
  ((SharedMemHdr*)clientBuffers)->wdtEnabler = 0;
  ((SharedMemHdr*)clientBuffers)->lastMonRefresh = 0;
  if (trace_settings & TRACE_INIT)
     trace_printf("%s@%d mpid=%d\n", method_name, __LINE__, ((SharedMemHdr*)clientBuffers)->mPid);

  qid = msgget( sharedSegKeyBase + 0x10000, IPC_CREAT | SQ_LIO_MSQ_PERMISSIONS );
  if (qid == -1)
  {
    int err = errno;
    char la_buf[MON_STRING_BUF_SIZE];
    sprintf(la_buf, "[%s], Error= Can't access message queue! - errno=%d (%s)\n", method_name, err, strerror(err));
    mon_log_write(MON_MLIO_INIT_10, SQ_LOG_CRIT, la_buf);

    mon_failure_exit();
  }

  errno = 0;
  ssize_t ret = 0;
  ClientBufferInfo cbi;
  // drain queue
  while (ret != -1)
  {
    ret = msgrcv( qid, 
                  (void *)&cbi,
                  sizeof( cbi.index ),
                  0,
                  IPC_NOWAIT
                );
    if (ret == -1  && errno != ENOMSG)
    {
      int err = errno;
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Error= Can't drain message queue! - errno=%d (%s)\n", method_name, err, strerror(err));
      mon_log_write(MON_MLIO_INIT_11, SQ_LOG_CRIT, la_buf);

      mon_failure_exit();
    }
  }
  // populate client buffer relative index location
  for (int n=0; n<sharedBuffersMax; n++)
  {
    cbi.index = n;
    cbi.mtype = SQ_LIO_NORMAL_MSG;
    rc = msgsnd( qid, &cbi, sizeof( cbi.index ), 0 );
    if (rc == -1)
    {
      int err = errno;
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Error= Can't load message queue! - errno=%d (%s)\n", method_name, err, strerror(err));
      mon_log_write(MON_MLIO_INIT_12, SQ_LOG_CRIT, la_buf);

      mon_failure_exit();
    }
  }

  rc = shmctl( cmid, SHM_LOCK, NULL );
  if (rc == -1)
  {
    int err = errno;
    char la_buf[MON_STRING_BUF_SIZE];
    sprintf(la_buf, "[%s], Error= Can't lock shared memory segment! - errno=%d (%s)\n", method_name, err, strerror(err));
    mon_log_write(MON_MLIO_INIT_13, SQ_LOG_CRIT, la_buf);

    mon_failure_exit();
  }


  if (trace_settings & TRACE_INIT)
     trace_printf("%s@%d" " - Created shared memory (cmid=%d) and message queue (qid=%d)\n", method_name, __LINE__, cmid, qid );
}


// This initialization routine is used to start the various threads.
int
SQ_LocalIOToClient::initWorker()
{
  pthread_t mSelf = pthread_self();
  int ret;
  const char method_name[] = "SQ_LocalIOToClient::initWorker";
  TRACE_ENTRY;

  if (trace_settings & TRACE_INIT)
     trace_printf("%s@%d" " self: " "%lx" "\n", method_name, __LINE__, mSelf);

// The request threads are not used for concurrent requests.  Only the
// serial request thread is used.
  
  // Create the serial request thread
  ret = pthread_create(&serialRequestTid_, NULL, serialRequestThread, NULL);
  if (ret != 0)
  {
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Error= Can't create thread! - errno=%d (%s)\n", method_name, ret, strerror(ret));
      mon_log_write(MON_MLIO_INIT_WORKER_2, SQ_LOG_ERR, la_buf);
      return(ret);
  }
  if (trace_settings & TRACE_INIT)
      trace_printf("%s@%d" " serialRequestThread created, threadId=%lx" "\n", method_name, __LINE__, serialRequestTid_);

  // Create the pending notice thread
  ret = pthread_create(&pendingNoticesTid_, NULL, pendingNoticeThread, NULL);
  if (ret != 0)
  {
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Error= Can't create thread! - errno=%d (%s)\n", method_name, ret, strerror(ret));
      mon_log_write(MON_MLIO_INIT_WORKER_3, SQ_LOG_ERR, la_buf);
      return(ret);
  }
  if (trace_settings & TRACE_INIT)
      trace_printf("%s@%d" " pendingNoticeThread created, threadId=%lx" "\n", method_name, __LINE__, pendingNoticesTid_);

  // Create the local io buffer cleanup thread
  ret = pthread_create(&lioBufCleanupTid_, NULL, lioBufCleanupThread, NULL);
  if (ret != 0)
  {
      char la_buf[MON_STRING_BUF_SIZE];
      sprintf(la_buf, "[%s], Error= Can't create thread! - errno=%d (%s)\n", method_name, ret, strerror(ret));
      mon_log_write(MON_MLIO_INIT_WORKER_2, SQ_LOG_ERR, la_buf);
      return(ret);
  }

  TRACE_EXIT;

  return ret;
}

// the monitor localio destructor
SQ_LocalIOToClient::~SQ_LocalIOToClient()
{
    const char method_name[] = "SQ_LocalIOToClient::~SQ_LocalIOToClient";
    TRACE_ENTRY;

    int rc;
    
    if (trace_settings & TRACE_INIT)
       trace_printf("%s@%d" " removing shared memory (cmid=%d) and message queue (qid=%d)\n", method_name, __LINE__, cmid, qid );

    // remove shared memory
    rc = shmctl( cmid, IPC_RMID, NULL );
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't remove shared memory segment! - errno=%d (%s)\n", method_name, err, strerror(err));
        mon_log_write(MON_MLIO_DESTRUCT_1, SQ_LOG_ERR, la_buf);
    }

    rc = msgctl( qid, IPC_RMID, NULL );
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't remove message queue! - errno=%d (%s)\n", method_name, err, strerror(err));
        mon_log_write(MON_MLIO_DESTRUCT_2, SQ_LOG_ERR, la_buf);
    }
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "mlio", 4);

    TRACE_EXIT;
}


void SQ_LocalIOToClient::msgQueueStats( void )
{
    struct msqid_ds  mds;
    int err = 0;
    int ret;
    const char method_name[] = "SQ_LocalIOToClient::msgQueueStats";
    TRACE_ENTRY;

    ret = msgctl(qid, IPC_STAT, &mds);
    if (ret)
    {
        err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't get message queue stats! - errno=%d (%s)\n", method_name, errno, strerror(errno));
        mon_log_write(MON_MLIO_QSTATS_1, SQ_LOG_ERR, la_buf);
        mds.msg_qnum = -1;
    }
    trace_printf("%s@%d (qid=%d) shared buffers available=%ld, acquired=%d, "
                 "ret=%d, errno=%d\n", method_name, __LINE__, qid,
                 mds.msg_qnum, acquiredBufferCount, ret, err);
    TRACE_EXIT;
}

// get a client buffer from the available pool
struct message_def *
SQ_LocalIOToClient::acquireMsg( int pid, Verifier_t verifier )
{
    bool done = false;
    struct message_def *msg = NULL;
    struct msqid_ds  mds;
    int ret;
    ClientBufferInfo cbi;

    const char method_name[] = "SQ_LocalIOToClient::acquireMsg";
    TRACE_ENTRY;

    if (acquiredBufferCount < SQ_LIO_MONITOR_ACQUIRE_MAX)
    {
        while (!done)
        {
            ret = (int)msgrcv( qid
                             , (void *)&cbi
                             , sizeof(cbi.index)
                             , SQ_LIO_NORMAL_MSG
                             , IPC_NOWAIT );
            if (ret == sizeof(cbi.index))
            {
                SharedMsgDef *shm;
                shm = (SharedMsgDef *)(clientBuffers+sizeof(SharedMemHdr)
                                                   +(cbi.index*sizeof(SharedMsgDef)));
                memset( &shm->trailer, 0, sizeof( shm->trailer ) );
                shm->trailer.index = cbi.index;
                shm->trailer.OSPid = pid;
                shm->trailer.verifier = verifier;
                shm->trailer.bufInUse = getpid();
                clock_gettime(CLOCK_REALTIME, &shm->trailer.timestamp);
                msg = &shm->msg;
                // Increment acquiredBufferCount.  Use atomic operation due
                // to multi-threaded access.
                __sync_fetch_and_add( &acquiredBufferCount, 1 );
    
                // Record statistics (sonar counters)
                if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                   MonStats->LocalIOBuffersIncr();
                if (acquiredBufferCount > acquiredBufferCountMax) {
                    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                       MonStats->LocalIOBuffersMaxSet(acquiredBufferCount);
                }
    
                acquiredBufferCountMax = acquiredBufferCount > acquiredBufferCountMax ? acquiredBufferCount : acquiredBufferCountMax;
                if (trace_settings & TRACE_MLIO_DETAIL)
                  trace_printf("%s@%d" " dequeued shared buffer, idx="  "%d" "\n", method_name, __LINE__, cbi.index);
                done = true;
            }
            else if (ret == -1  && errno != ENOMSG)
            {  // msgrcv error
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], msgrcv error %s (%d)\n", method_name, strerror(errno), errno);
                mon_log_write(MON_MLIO_ACQUIRE_MSG_1, SQ_LOG_ERR, la_buf);
                done = true;
            }
            if (!done)
            {
                usleep(10000); // sleep 10ms and try again
                if (trace_settings & TRACE_MLIO)
                {
                    trace_printf( "%s@%d" " No message buffer!\n"
                                , method_name, __LINE__);
                }
            }
        }
    }
    else
    {
        // Record statistics (sonar counter)
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->LocalIOBufferMissesIncr();

        missedBufferCount++;
        if (trace_settings & TRACE_MLIO)
            trace_printf("%s@%d" " failed getting buffer from shared pool, misses=%d\n", method_name, __LINE__, missedBufferCount);
    }
    
    ret = msgctl(qid, IPC_STAT, &mds);
    if (ret)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't get message queue stats! - errno=%d (%s)\n", method_name, err, strerror(err));
        mon_log_write(MON_MLIO_ACQUIRE_MSG_2, SQ_LOG_ERR, la_buf);
    }

    availableBufferCountMin  = (int) mds.msg_qnum < availableBufferCountMin ? (int)mds.msg_qnum : availableBufferCountMin;

    if (trace_settings & TRACE_MLIO_DETAIL)
    {
        trace_printf("%s@%d" " (qid=%d) shared buffers available=" "%ld" ", acquired=" "%d"  ", ret=" "%d" ", errno=" "%d" "\n", method_name, __LINE__, qid, mds.msg_qnum, acquiredBufferCount, ret, errno);
    }
      
    TRACE_EXIT;
    return msg;
}

bool SQ_LocalIOToClient::decrNoticeMsgRef ( int bufIndex, int pid, Verifier_t verifier )
{
    const char method_name[] = "SQ_LocalIOToClient::decrRef";
    TRACE_ENTRY;

    bool foundNotice = false;
    bool noticeReleased = false;

    CNoticeMsg *noticeMsg = NULL;
    noticeMap_t::iterator it;
 
    noticeMapLock_.lock();

    it = noticeMap_.find ( bufIndex );
    
    if (it != noticeMap_.end())
    {   // Found entry for the buffer in the notice map
        noticeMsg = it->second;

        // bugcatcher temp call
        noticeMsg->validateObj();

        // Indicate that the given bufIndex holds a monitor notice
        foundNotice = true;

        if ( noticeMsg->clientDone( pid, verifier ) == 0 )
        {   // Remove notice from the map
            noticeMap_.erase( it );

            if (trace_settings & TRACE_NOTICE_DETAIL)
            {
                trace_printf( "%s@%d Removing notice message map entry: "
                              "index=%d, pid=%d, verifier=%d\n"
                            , method_name, __LINE__
                            , bufIndex
                            , pid 
                            , verifier );
            }
            noticeReleased = true;
        }
    }

    noticeMapLock_.unlock();

    if ( noticeReleased )
    {
        if (trace_settings & TRACE_NOTICE)
        {
            trace_printf("%s@%d Releasing notice buffer, idx=%d\n",
                         method_name, __LINE__, bufIndex);
        }

        // Delete the notice object
        delete noticeMsg;

        // Buffer was released, signal the pendingNoticeThread
        // since it might have been waiting for a buffer to
        // become available.
        pendingNoticesLock_.lock();
        pendingNoticesLock_.wakeOne();
        pendingNoticesLock_.unlock();
    }

    TRACE_EXIT;

    return foundNotice;
}

// release a message based on a pid to the available client buffer pool.
// This routine is called from the child death signal handler to cleanup
// any orphaned client buffers
void
SQ_LocalIOToClient::releaseMsg( pid_t pid, Verifier_t verifier )
{
    SharedMsgDef *shm;
    ClientBufferInfo cbi;

    const char method_name[] = "SQ_LocalIOToClient::releaseMsg";
    TRACE_ENTRY;
      
    if (trace_settings & TRACE_MLIO_DETAIL)
    {
        trace_printf("%s@%d releasing shared buffers for pid=%d, verifier=%d\n",
                     method_name, __LINE__, pid, verifier);
      
        msgQueueStats();
    }
    
    cbi.index = 0;
    cbi.mtype = SQ_LIO_NORMAL_MSG;
    while (cbi.index < sharedBuffersMax)
    {
        shm = (SharedMsgDef *)(clientBuffers+sizeof(SharedMemHdr)
                                           +(cbi.index*sizeof(SharedMsgDef)));
        if ( shm->trailer.bufInUse != 0 )
        {   // Buffer is in use
            if ( decrNoticeMsgRef ( cbi.index, pid, verifier ) )
            {   // Buffer is a monitor generated notice.  decrNoticeMsgRef
                // took appropriate actions to indicate notice no longer
                // used by "pid".
            }
            else if ( shm->trailer.OSPid == pid && 
                      shm->trailer.verifier == verifier )
            {   // The buffer was client allocated and the client
                // is a dead process.  Release the shared buffer.
                releaseMsg ( shm, false );
            }
            else
            {
                if (kill( (pid_t)shm->trailer.bufInUse, 0 ) == -1
                    && errno == ESRCH) 
                {   // Owning process no longer exists, release the buffer
                    releaseMsg ( shm, false );
                }
                else if (trace_settings & TRACE_MLIO_DETAIL)
                {
                    trace_printf("%s@%d shared buffer idx=%d in use by pid=%d\n",
                                 method_name, __LINE__, shm->trailer.index,
                                 shm->trailer.bufInUse);
                }
            }
        }
        cbi.index++;
    }

    if (trace_settings & TRACE_MLIO_DETAIL)
    {
        msgQueueStats();

        trace_printf("%s@%d monitor is managing %d shared buffers for "
                     "notices\n", method_name, __LINE__, (int) noticeMap_.size());
    }

    TRACE_EXIT;
}

// release a message back to the available client buffer pool
void
SQ_LocalIOToClient::releaseMsg( SharedMsgDef *shm, bool monitorOwned )
{
    ClientBufferInfo cbi;
    int ret;

    const char method_name[] = "SQ_LocalIOToClient::releaseMsg";
    TRACE_ENTRY;

    if ( shm->trailer.index != -1 )
    {
        cbi.index = shm->trailer.index;
        cbi.mtype = SQ_LIO_NORMAL_MSG;
    
        if (trace_settings & TRACE_MLIO_DETAIL)
        {
            msgQueueStats();
          
            trace_printf("%s@%d releasing shared buffer, idx=%d\n",
                         method_name, __LINE__, shm->trailer.index);
        }

        // Reset fields in message trailer and indicate that the shared
        // buffer is not in use.
        memset( (void*)&shm->trailer, 0, sizeof(shm->trailer) );
        shm->trailer.index = -1;
        shm->trailer.OSPid = getpid(); // Identify releaser - monitor thread

        if (monitorOwned)
        {
            // Decrement acquiredBufferCount.  Use atomic operation due
            // to multi-threaded access.
            __sync_fetch_and_add( &acquiredBufferCount, -1 );

            if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
               MonStats->LocalIOBuffersDecr();
        }

        // Queue the shared buffer on the free list
        ret = msgsnd( qid, &cbi, sizeof( cbi.index ), 0);
        if (ret == -1)
        {
            int err = errno;
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error= Can't return buffer through message queue! - errno=%d (%s)\n", method_name, err, strerror(err));
            mon_log_write(MON_MLIO_RELEASE_MSG_1, SQ_LOG_CRIT, la_buf);
        }
    
        if (trace_settings & TRACE_MLIO_DETAIL)
        {
            msgQueueStats();
        }
    }
      
    TRACE_EXIT;
}


void SQ_LocalIOToClient::handleAlmostDeadPid( pid_t pid )
{
    // Add pid to circular queue of pids for which child death signal
    // was received.
    almostDeadPidsLock_.lock();
    int oldhead = almostDeadPidsHead_;
    ++almostDeadPidsHead_;
    if (almostDeadPidsHead_ >= MAX_ALMOST_DEAD_PIDS) almostDeadPidsHead_ = 0;
    if (almostDeadPidsHead_ == almostDeadPidsTail_)
    {
        // overflow, reset to previous position.
        almostDeadPidsHead_ = oldhead;
    }
    else
    {   // insert into queue
        almostDeadPids_[almostDeadPidsHead_] = pid;
    }
    almostDeadPidsLock_.unlock();
}

void SQ_LocalIOToClient::examineAlmostDeadPids()
{
    pid_t pid;
    pid_t pidRet;

    while ( true )
    {
        // Get next pid (if any) from the queue
        almostDeadPidsLock_.lock();
        if ( almostDeadPidsHead_ == almostDeadPidsTail_ )
        {   // No more dead pids
            almostDeadPidsLock_.unlock();
            break;
        }
        // Temp counter during development
        ++almostDeadPidsTotal_;

        ++almostDeadPidsTail_;
        if (almostDeadPidsTail_ >= MAX_DEAD_PIDS) almostDeadPidsTail_ = 0;
        pid = almostDeadPids_[almostDeadPidsTail_];
        almostDeadPidsLock_.unlock();

        pidRet = waitpid(pid, NULL, WNOHANG);
        if ( pidRet > 0 )
        {   // The pid's state has changed, so can proceed with exit processing
            handleDeadPid(pid);

            // Temp counter during development
            ++almostDeadPidsHandled_;
        }
        else if ( pidRet == 0 )
        {   // Pid has not yet changed state
            // TBD, requeue?

            // Temp counter during development
            ++almostDeadPidsDeferred_;
        }
        else
        {   // Should only get here if "pid" does not exist or is not a
            // child of this process.  In either case we are finished with
            // this "pid"

            // Temp counter during development
            ++almostDeadPidsError_;
        }
    }
}

// Add to list of pids that need to be examined for unreleased message buffers.
void SQ_LocalIOToClient::handleDeadPid( pid_t pid )
{
    // Add pid to circular queue of pids for which child death signal
    // was received.
    deadPidsLock_.lock();
    int oldhead = deadPidsHead_;
    ++deadPidsHead_;
    if (deadPidsHead_ >= MAX_DEAD_PIDS) deadPidsHead_ = 0;
    if (deadPidsHead_ == deadPidsTail_)
    {
        // overflow, reset to previous position.
        // [note: we don't expect to get overflow since the size of
        // the deadPids_ array should be much larger than needed.  If
        // overflow does occur the lioCleanupThread will log an error
        // to report an undersized deadPids_ array.]
        deadPidsHead_ = oldhead;
        deadPidsOverflow_ = true;
    }
    else
    {   // insert into queue
        deadPids_[deadPidsHead_] = pid;
    }
    deadPidsLock_.unlock();

   // Wake up local io buffer cleanup thread so it can invoke recycleProcessBufs
    pthread_kill(lioBufCleanupTid_, SIGUSR1);
}

// Examine info queued by child_death_signal_handler2 regarding
// processes that have terminated.  For each process on the
// list, see if there is still a process object.  If so, free any
// local io buffers the process owns.
void
SQ_LocalIOToClient::recycleProcessBufs( void )
{
    const char method_name[] = "SQ_LocalIOToClient::recycleProcessBufs";
    TRACE_ENTRY;

    pid_t nextPid;

    examineAlmostDeadPids();

    while ( true )
    {
        // Get next pid (if any) from the queue
        deadPidsLock_.lock();
        if ( deadPidsHead_ == deadPidsTail_ )
        {   // No more dead pids
            deadPidsLock_.unlock();
            break;
        }
        
        // Keep track of highest number of queued pids
        int count;
        if ( deadPidsHead_ >= deadPidsTail_)
            count = deadPidsHead_ - deadPidsTail_;
        else count = (MAX_DEAD_PIDS - deadPidsTail_) + deadPidsHead_ + 1;
        if ( count > deadPidsMax_ ) deadPidsMax_ = count;

        ++deadPidsTail_;
        if (deadPidsTail_ >= MAX_DEAD_PIDS) deadPidsTail_ = 0;
        nextPid = deadPids_[deadPidsTail_];
        deadPidsLock_.unlock();

        if ( deadPidsOverflow_ )
        {   // handleDeadPid unexpectedly got overflow.   This indicates
            // an incorrectly sized deadPids_ array.
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], lost child death event.  Current "
                    "array size (%d) is too small\n", method_name,
                    MAX_DEAD_PIDS);
            mon_log_write(MON_MLIO_DEADPID_OVERFLOW, SQ_LOG_CRIT, buf);

            deadPidsOverflow_ = false;
        }

        if (trace_settings & (TRACE_PROCESS | TRACE_MLIO))
        {
            trace_printf("%s@%d handling dead process %d\n",
                         method_name, __LINE__, nextPid);
        }

        // Release any Seabed disconnect semaphores held by the process
        if ( sbDiscSem != NULL )
            sbDiscSem->post_all( nextPid );

        // Indicate that we have seen a child death signal for this process
        MyNode->PidHangupClear ( nextPid );

        // Queue request for processing by worker thread
        ReqQueue.enqueueChildDeathReq ( nextPid );

        // Get verifier from dead process verifier map
        Verifier_t verifier = getVerifier( nextPid );
        releaseMsg(nextPid, verifier);
        delFromVerifierMap( nextPid );
    }

    // Periodically check to see if any notice buffers have expired.
    struct timespec    now;
    clock_gettime(CLOCK_REALTIME, &now);

    if ( now.tv_sec > nextNoticeCheck_.tv_sec )
    {
        if (trace_settings & TRACE_MLIO)
        {
            trace_printf("%s@%d checking for timed-out notice buffers\n",
                         method_name, __LINE__);
        }

        // Check for process ids for which we have received a broken pipe
        // indication but have not yet processed a child death signal.
        MyNode->PidHangupCheck ( now.tv_sec );

        // Check for timed-out notice buffers
        CNoticeMsg * noticeMsg;
        noticeMap_t::iterator it;
        noticeMapLock_.lock();
        it = noticeMap_.begin();

        bool noticeReleased = false;

        while (it != noticeMap_.end())
        {
            noticeMsg = it->second;

            // bugcatcher, temp call
            noticeMsg->validateObj();

            if ( (now.tv_sec - noticeMsg->tsSecs() )
                 > CNoticeMsg::NOTICE_BUF_TIME_LIMIT )
            {
                noticeMap_.erase ( it );

                noticeReleased = true;

                // Stop searching list to avoid holding lock too long
                break;
            }

            ++it;
        }
        noticeMapLock_.unlock();

        if ( noticeReleased )
        {
            if (trace_settings & TRACE_MLIO)
            {
                trace_printf("%s@%d Releasing timed-out notice buffer, "
                             "idx=%d\n", method_name, __LINE__,
                             noticeMsg->getIndex());
            }

            delete noticeMsg;
        }
        
        // Check again in one minute.
        nextNoticeCheck_.tv_sec = now.tv_sec + 60;
    }

    TRACE_EXIT;
}

// Get the size of the message.  there is an equivalent routine on the
// seabed/shell side.  This routine is used so that only the data needed
// to be copied, is copied.
int
SQ_LocalIOToClient::getSizeOfMsg( struct message_def *myMsg )
{
    int len;
    // Must use the first structure in union to get correct offset
    int preamble = (long)&myMsg->u.request.u.shutdown - (long)myMsg;

    if (myMsg->type == MsgType_Service) 
    {
        len = preamble + serviceReplySize[myMsg->u.reply.type-ReplyType_Generic];
    }
    else
    {
        len = preamble + requestSize[myMsg->type];
    }
    return len;
}

int
SQ_LocalIOToClient::getSizeOfRequest( struct message_def *myMsg )
{
    int len;
    // Must use the first structure in union to get correct offset
    int preamble = (long)&myMsg->u.request.u.shutdown - (long)myMsg;

    if (myMsg->type == MsgType_Service) 
    {
        len = preamble + serviceRequestSize[myMsg->u.request.type];
    }
    else
    {
        len = preamble + requestSize[myMsg->type];
    }
    return len;
}

void 
SQ_LocalIOToClient::shutdownWork(void)
{
    const char method_name[] = "SQ_LocalIOToClient::shutdownWork";
    TRACE_ENTRY;

    pendingNoticesLock_.lock();
    if (trace_settings & TRACE_MLIO)
      trace_printf("%s@%d" " shutting down!" "\n", method_name, __LINE__);
    shutdown = true;   
    // signal the pendingNoticeThread so it can exit
    pendingNoticesLock_.wakeOne();
    pendingNoticesLock_.unlock();
    
    int rc;
    // Signal the serial request thread so it will wake up and exit
    if ((rc = pthread_kill(serialRequestTid_, SIGUSR1)) != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], pthread_kill error=%d\n", method_name, rc);
        mon_log_write(MON_MLIO_SHUTDOWN_1, SQ_LOG_ERR, buf);
    }
    else
    {
        if (trace_settings & TRACE_INIT)
            trace_printf("%s@%d waiting for serial request thread=%lx to "
                         "exit.\n",
                         method_name, __LINE__, serialRequestTid_);
        // Wait for serial request thread to exit
        pthread_join(serialRequestTid_, NULL);
    }

    // Signal the lioBufCleanup thread so it will wake up and exit
    if ((rc = pthread_kill(lioBufCleanupTid_, SIGUSR1)) != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], pthread_kill error=%d\n", method_name, rc);
        mon_log_write(MON_MLIO_SHUTDOWN_1, SQ_LOG_ERR, buf);
    }
    else
    {
        if (trace_settings & TRACE_INIT)
            trace_printf("%s@%d waiting for lioBufCleanup thread=%lx to "
                         "exit.\n",
                         method_name, __LINE__, lioBufCleanupTid_);
        // Wait for local io buffer cleanup thread to exit
        pthread_join(lioBufCleanupTid_, NULL);
    }

    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d waiting for pending notices request thread=%lx to "
                     "exit.\n",
                     method_name, __LINE__, pendingNoticesTid_);
    // Wait for redirector thread to exit
    pthread_join(pendingNoticesTid_, NULL);

    
    TRACE_EXIT;
}


void SQ_LocalIOToClient::waitForNoticeWork( void )
{
    const char method_name[] = "SQ_LocalIOToClient::waitForNoticeWork";
    TRACE_ENTRY;

    pendingNoticesLock_.lock( );
    if ( !noticeSignaled )
    {
        pendingNoticesLock_.wait( );
    }
    noticeSignaled = false;
    pendingNoticesLock_.unlock( );

    TRACE_EXIT;
}

void SQ_LocalIOToClient::addToVerifierMap(int pid, Verifier_t verifier)
{
    const char method_name[] = "SQ_LocalIOToClient::addToVerifierMap";
    TRACE_ENTRY;

    pair<verifierMap_t::iterator, bool> ret;

    if (pid != -1)
    {
        // temp trace
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf( "%s@%d inserting into verifierMap_ %p: (%d:%d)\n"
                        , method_name, __LINE__
                        , &verifierMap_
                        , pid
                        , verifier );
        }

        verifierMapLock_.lock();
        ret = verifierMap_.insert( verifierMap_t::value_type ( pid, verifier ));
        verifierMapLock_.unlock();
        if (ret.second == false)
        {   // Already had an entry with the given key value
            if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d verifier map already contained pid=%d\n",
                             method_name, __LINE__, pid);
            }
        }

        // temp trace
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf( "%s@%d verifierMap_ (%p) now has %d entries\n"
                        , method_name, __LINE__
                        , &verifierMap_, (int)verifierMap_.size());
        }

    }

    TRACE_EXIT;
}

void SQ_LocalIOToClient::delFromVerifierMap( int pid )
{
    const char method_name[] = "SQ_LocalIOToClient::delFromVerifierMap";
    TRACE_ENTRY;

    verifierMapLock_.lock();
    int count = verifierMap_.erase ( pid );
    verifierMapLock_.unlock();

    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
    {
        if (count != 0)
        {
            trace_printf( "%s@%d removed pid=%d - verifierMap_ (%p) now has %d entries\n"
                        , method_name, __LINE__
                        , pid, &verifierMap_, (int)verifierMap_.size());
        }
    }

    TRACE_EXIT;
}

Verifier_t SQ_LocalIOToClient::getVerifier( int pid )
{
    const char method_name[] = "SQ_LocalIOToClient::getVerifier(pid)";
    TRACE_ENTRY;

    verifierMap_t::iterator it;
    Verifier_t verifier = -1;

    verifierMapLock_.lock();
    it = verifierMap_.find(pid);
    if (it != verifierMap_.end())
    {
        verifier = it->second;
    }
    verifierMapLock_.unlock();

    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf("%s@%d - pidmap_ (%p) pid=%d, verifier=%d\n",
                     method_name, __LINE__, &verifierMap_, pid, verifier );
    }

    TRACE_EXIT;
    return verifier;
}

CNoticeMsg::CNoticeMsg( int bufIndex
                      , SharedMsgDef *buf
                      , pid_t pid
                      , Verifier_t verifier
                      , SQ_LocalIOToClient::bcastPids_t *bcastPids )
           : bufIndex_( bufIndex )
           , buf_( buf )
           , pid_( pid )
           , verifier_( verifier )
           , bcastPids_( bcastPids )
{
    const char method_name[] = "CNoticeMsg::CNoticeMsg";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "NMSG", 4);

    clock_gettime(CLOCK_REALTIME, &timestamp_);
    
    if (trace_settings & TRACE_NOTICE_DETAIL)
    {
        trace_printf( "%s@%d New notice message: index=%d, pid=%d, verifier=%d\n"
                    , method_name, __LINE__
                    , bufIndex_ 
                    , pid_ 
                    , verifier_ );
    }

    TRACE_EXIT;
}

// CNoticeMsg is used to manage a message sent from the monitor to a
// client.  Once all clients have replied the shared memory buffer can
// be released.
CNoticeMsg::~CNoticeMsg ( )
{
    const char method_name[] = "CNoticeMsg::~CNoticeMsg";
    TRACE_ENTRY;

    char buf[MON_STRING_BUF_SIZE];

    if (trace_settings & TRACE_NOTICE_DETAIL)
    {
        trace_printf( "%s@%d Deleting notice message: index=%d, pid=%d, verifier=%d\n"
                    , method_name, __LINE__
                    , bufIndex_ 
                    , pid_ 
                    , verifier_ );
    }

    // Log information about processes that did not respond to the notice
    bcastPidsLock_.lock();
    if ( pid_ == BCAST_PID )
    {
        if ( !bcastPids_->empty() )
        {  // One or more processes did not respond to the notice
            SQ_LocalIOToClient::pidVerifier_t pv;
            for (SQ_LocalIOToClient::bcastPids_t::const_iterator
                     it = bcastPids_->begin(); it != bcastPids_->end();)
            {
                pv.pnv = *it;
                ++it;

                if ( kill( (pid_t)pv.pv.pid, 0 ) == 0 )
                {   // Process exists but did not respond to the notice
                    sprintf(buf, "[%s], Process %d:%d did not respond to notice"
                                 ": index=%d, pid=%d, verifier=%d\n"
                               , method_name
                               , pv.pv.pid
                               , pv.pv.verifier
                               , bufIndex_ 
                               , pid_ 
                               , verifier_ );
                    mon_log_write(MON_MLIO_NOTICE_DEST_1, SQ_LOG_ERR, buf);
                }
            }
        }
    }
    else if ( pid_ != 0 )
    {   
        if ( kill( (pid_t)pid_, 0 ) == 0 )
        {   // Process exists but did not respond to the notice
            sprintf(buf, "[%s], Process %d did not respond to notice\n", 
                    method_name, pid_ );
            mon_log_write(MON_MLIO_NOTICE_DEST_2, SQ_LOG_ERR, buf);
        }
    }

    if (bcastPids_ != NULL)
        delete bcastPids_;

    bcastPidsLock_.unlock();

    SQ_theLocalIOToClient->releaseMsg ( buf_, true );

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "nmsg", 4);

    TRACE_EXIT;
}

int CNoticeMsg::clientDone ( pid_t pid, Verifier_t verifier )
{
    const char method_name[] = "SQ_LocalIOToClient::clientDone";
    TRACE_ENTRY;
    int refCount = -1;
    CProcess *client = NULL;

    if ( pid_ == BCAST_PID )
    {
        client = MyNode->GetProcess( pid );
    }

    if (trace_settings & TRACE_NOTICE_DETAIL)
    {
        trace_printf( "%s@%d Client (%d:%d) done with notice message: index=%d, pid=%d, verifier=%d\n"
                    , method_name, __LINE__
                    , pid
                    , client ? client->GetVerifier() : verifier
                    , bufIndex_ 
                    , pid_ 
                    , verifier_ );
    }

    if ( pid_ == BCAST_PID )
    {   // The buffer was used for a broadcast message so remove the
        // pid from the broadcast list.
        bcastPidsLock_.lock();
        SQ_LocalIOToClient::pidVerifier_t pv;
        pv.pv.pid = pid;
        pv.pv.verifier = client ? client->GetVerifier() : verifier;
        bcastPids_->erase( pv.pnv );
        refCount = bcastPids_->size();
        bcastPidsLock_.unlock();
    }
    else if ( pid_ == pid && verifier_ == verifier )
    {
        pid_ = 0;
        refCount = 0;
    }


    TRACE_EXIT;

    return refCount;
}

void CNoticeMsg::validateObj( void )
{
    if (strncmp((const char *)&eyecatcher_, "NMSG", 4) !=0 )
    {  // Not a valid object
        abort();
    }
}
