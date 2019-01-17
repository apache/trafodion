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

#include "nstype.h"

using namespace std;

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "nscommacceptmon.h"
#include "monlogging.h"
#include "montrace.h"
#include "monitor.h"

extern CCommAcceptMon CommAcceptMon;
extern CMonitor *Monitor;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern int MyPNID;
extern char MyMon2NsPort[MPI_MAX_PORT_NAME];
extern char *ErrorMsg (int error_code);
extern const char *StateString( STATE state);
extern CommType_t CommType;
extern CReqQueue ReqQueue;

static void *mon2nsProcess(void *arg);

CCommAcceptMon::CCommAcceptMon()
           : accepting_(false)
           , shutdown_(false)
           , thread_id_(0)
           , process_thread_id_(0)
{
    const char method_name[] = "CCommAcceptMon::CCommAcceptMon";
    TRACE_ENTRY;


    TRACE_EXIT;
}

CCommAcceptMon::~CCommAcceptMon()
{
    const char method_name[] = "CCommAcceptMon::~CCommAcceptMon";
    TRACE_ENTRY;

    TRACE_EXIT;
}

void CCommAcceptMon::monReqDeleteProcess( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqDeleteProcess";
    TRACE_ENTRY;

    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request delete-process data.\n"
                      "        msg.del_process_ns.nid=%d\n"
                      "        msg.del_process_ns.pid=%d\n"
                      "        msg.del_process_ns.verifier=%d\n"
                      "        msg.del_process_ns.process_name=%s\n"
                      "        msg.del_process_ns.target_nid=%d\n"
                      "        msg.del_process_ns.target_pid=%d\n"
                      "        msg.del_process_ns.target_verifier=%d\n"
                      "        msg.del_process_ns.target_process_name=%s\n"
                      "        msg.del_process_ns.target_abended=%d\n"
                    , method_name, __LINE__
                    , msg->u.request.u.del_process_ns.nid
                    , msg->u.request.u.del_process_ns.pid
                    , msg->u.request.u.del_process_ns.verifier
                    , msg->u.request.u.del_process_ns.process_name
                    , msg->u.request.u.del_process_ns.target_nid
                    , msg->u.request.u.del_process_ns.target_pid
                    , msg->u.request.u.del_process_ns.target_verifier
                    , msg->u.request.u.del_process_ns.target_process_name
                    , msg->u.request.u.del_process_ns.target_abended
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    int nid = msg->u.request.u.del_process_ns.nid;
    int pid = msg->u.request.u.del_process_ns.pid;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, nid, pid, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::monReqExec( CExternalReq * request )
{
    const char method_name[] = "CCommAcceptMon::monReqExec";
    TRACE_ENTRY;

    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST_DETAIL ) )
    {
        request->populateRequestString();
        trace_printf("%s@%d request = %s\n", method_name, __LINE__, request->requestString());
    }
    request->validateObj();
    request->performRequest();
    delete request;

    TRACE_EXIT;
}

void CCommAcceptMon::monReqNameServerStop( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqNameServerStop";
    TRACE_ENTRY;

    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request down-nameserver data.\n"
                      "        msg.nameserver_stop.nid=%d\n"
                      "        msg.nameserver_stop.pid=%d\n"
                      "        msg.nameserver_stop.node_name=%s\n"
                    , method_name, __LINE__
                    , msg->u.request.u.nameserver_stop.nid
                    , msg->u.request.u.nameserver_stop.pid
                    , msg->u.request.u.nameserver_stop.node_name
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    int nid = msg->u.request.u.nameserver_stop.nid;
    int pid = msg->u.request.u.nameserver_stop.pid;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, nid, pid, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::monReqNodeDown( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqNodeDown";
    TRACE_ENTRY;

    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor node-down request.\n"
                      "        msg.down.nid=%d\n"
                      "        msg.down.node_name=%s\n"
                      "        msg.down.takeover=%d\n"
                      "        msg.down.reason=%s\n"
                    , method_name, __LINE__
                    , msg->u.request.u.down.nid
                    , msg->u.request.u.down.node_name
                    , msg->u.request.u.down.takeover
                    , msg->u.request.u.down.reason
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    int nid = msg->u.request.u.down.nid;
    int pid = -1;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, nid, pid, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::monReqProcessInfo( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqProcessInfo";
    TRACE_ENTRY;

    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request process-info data.\n"
                      "        msg.info.nid=%d\n"
                      "        msg.info.pid=%d\n"
                      "        msg.info.verifier=%d\n"
                      "        msg.info.target_nid=%d\n"
                      "        msg.info.target_pid=%d\n"
                      "        msg.info.target_verifier=%d\n"
                      "        msg.info.target_process_name=%s\n"
                      "        msg.info.target_process_pattern=%s\n"
                      "        msg.info.type=%d\n"
                    , method_name, __LINE__
                    , msg->u.request.u.process_info.nid
                    , msg->u.request.u.process_info.pid
                    , msg->u.request.u.process_info.verifier
                    , msg->u.request.u.process_info.target_nid
                    , msg->u.request.u.process_info.target_pid
                    , msg->u.request.u.process_info.target_verifier
                    , msg->u.request.u.process_info.target_process_name
                    , msg->u.request.u.process_info.target_process_pattern
                    , msg->u.request.u.process_info.type
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    int nid = msg->u.request.u.process_info.nid;
    int pid = msg->u.request.u.process_info.pid;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, nid, pid, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::monReqProcessInfoCont( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqProcessInfoCont";
    TRACE_ENTRY;

    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request process-info-cont data.\n"
                      "        msg.info_cont.nid=%d\n"
                      "        msg.info_cont.pid=%d\n"
                      "        msg.info_cont.context[0].nid=%d\n"
                      "        msg.info_cont.context[0].pid=%d\n"
                      "        msg.info_cont.context[1].nid=%d\n"
                      "        msg.info_cont.context[1].pid=%d\n"
                      "        msg.info_cont.context[2].nid=%d\n"
                      "        msg.info_cont.context[2].pid=%d\n"
                      "        msg.info_cont.context[3].nid=%d\n"
                      "        msg.info_cont.context[3].pid=%d\n"
                      "        msg.info_cont.context[4].nid=%d\n"
                      "        msg.info_cont.context[5].pid=%d\n"
                      "        msg.info_cont.type=%d\n"
                      "        msg.info_cont.allNodes=%d\n"
                    , method_name, __LINE__
                    , msg->u.request.u.process_info_cont.nid
                    , msg->u.request.u.process_info_cont.pid
                    , msg->u.request.u.process_info_cont.context[0].nid
                    , msg->u.request.u.process_info_cont.context[0].pid
                    , msg->u.request.u.process_info_cont.context[1].nid
                    , msg->u.request.u.process_info_cont.context[1].pid
                    , msg->u.request.u.process_info_cont.context[2].nid
                    , msg->u.request.u.process_info_cont.context[2].pid
                    , msg->u.request.u.process_info_cont.context[3].nid
                    , msg->u.request.u.process_info_cont.context[3].pid
                    , msg->u.request.u.process_info_cont.context[4].nid
                    , msg->u.request.u.process_info_cont.context[4].pid
                    , msg->u.request.u.process_info_cont.type
                    , msg->u.request.u.process_info_cont.allNodes
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    int nid = msg->u.request.u.process_info_cont.nid;
    int pid = msg->u.request.u.process_info_cont.pid;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, nid, pid, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::monReqProcessInfoNs( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqProcessInfoNs";
    TRACE_ENTRY;

    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request process-info-ns data.\n"
                      "        msg.info.nid=%d\n"
                      "        msg.info.pid=%d\n"
                      "        msg.info.verifier=%d\n"
                      "        msg.info.target_nid=%d\n"
                      "        msg.info.target_pid=%d\n"
                      "        msg.info.target_verifier=%d\n"
                      "        msg.info.target_process_name=%s\n"
                      "        msg.info.target_process_pattern=%s\n"
                      "        msg.info.type=%d\n"
                    , method_name, __LINE__
                    , msg->u.request.u.process_info.nid
                    , msg->u.request.u.process_info.pid
                    , msg->u.request.u.process_info.verifier
                    , msg->u.request.u.process_info.target_nid
                    , msg->u.request.u.process_info.target_pid
                    , msg->u.request.u.process_info.target_verifier
                    , msg->u.request.u.process_info.target_process_name
                    , msg->u.request.u.process_info.target_process_pattern
                    , msg->u.request.u.process_info.type
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    int nid = msg->u.request.u.process_info.nid;
    int pid = msg->u.request.u.process_info.pid;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, nid, pid, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::monReqNewProcess( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqNewProcess";
    TRACE_ENTRY;
    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request new-process data.\n"
                      "        msg.new_process_ns.nid=%d\n"
                      "        msg.new_process_ns.pid=%d\n"
                      "        msg.new_process_ns.verifier=%d\n"
                      "        msg.new_process_ns.process_name=%s\n"
                      "        msg.new_process_ns.type=%d\n"
                      "        msg.new_process_ns.parent_nid=%d\n"
                      "        msg.new_process_ns.parent_pid=%d\n"
                      "        msg.new_process_ns.parent_verifier=%d\n"
                      "        msg.new_process_ns.pair_parent_nid=%d\n"
                      "        msg.new_process_ns.pair_parent_pid=%d\n"
                      "        msg.new_process_ns.pair_parent_verifier=%d\n"
                      "        msg.new_process_ns.priority=%d\n"
                      "        msg.new_process_ns.backup=%d\n"
                      "        msg.new_process_ns.unhooked=%d\n"
                      "        msg.new_process_ns.event_messages=%d\n"
                      "        msg.new_process_ns.system_messages=%d\n"
                      "        msg.new_process_ns.path=%s\n"
                      "        msg.new_process_ns.ldpath=%s\n"
                      "        msg.new_process_ns.program=%s\n"
                      "        msg.new_process_ns.port=%s\n"
                      "        msg.new_process_ns.infile=%s\n"
                      "        msg.new_process_ns.outfile=%s\n"
                      "        msg.new_process_ns.creation_time=%ld(secs):%ld(nsecs)\n"
                    , method_name, __LINE__
                    , msg->u.request.u.new_process_ns.nid
                    , msg->u.request.u.new_process_ns.pid
                    , msg->u.request.u.new_process_ns.verifier
                    , msg->u.request.u.new_process_ns.process_name
                    , msg->u.request.u.new_process_ns.type
                    , msg->u.request.u.new_process_ns.parent_nid
                    , msg->u.request.u.new_process_ns.parent_pid
                    , msg->u.request.u.new_process_ns.parent_verifier
                    , msg->u.request.u.new_process_ns.pair_parent_nid
                    , msg->u.request.u.new_process_ns.pair_parent_pid
                    , msg->u.request.u.new_process_ns.pair_parent_verifier
                    , msg->u.request.u.new_process_ns.priority
                    , msg->u.request.u.new_process_ns.backup
                    , msg->u.request.u.new_process_ns.unhooked
                    , msg->u.request.u.new_process_ns.event_messages
                    , msg->u.request.u.new_process_ns.system_messages
                    , msg->u.request.u.new_process_ns.path
                    , msg->u.request.u.new_process_ns.ldpath
                    , msg->u.request.u.new_process_ns.program
                    , msg->u.request.u.new_process_ns.port_name
                    , msg->u.request.u.new_process_ns.infile
                    , msg->u.request.u.new_process_ns.outfile
                    , msg->u.request.u.new_process_ns.creation_time.tv_sec
                    , msg->u.request.u.new_process_ns.creation_time.tv_nsec
                    );
        trace_printf("%s@%d - msg.new_process_ns.argc=%d\n"
                    , method_name, __LINE__
                    , msg->u.request.u.new_process_ns.argc );
        for (int i=0; i < msg->u.request.u.new_process_ns.argc; i++)
        {
            trace_printf("%s@%d - msg.new_process_ns.argv[%d]=%s\n"
                        , method_name, __LINE__
                        , i, msg->u.request.u.new_process_ns.argv[i]);
        }
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    int nid = msg->u.request.u.new_process_ns.nid;
    int pid = msg->u.request.u.new_process_ns.pid;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, nid, pid, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::monReqShutdown( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqShutdown";
    TRACE_ENTRY;

    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request shutdown data.\n"
                      "        msg.shutdown.nid=%d\n"
                      "        msg.shutdown.pid=%d\n"
                      "        msg.shutdown.level=%d\n"
                    , method_name, __LINE__
                    , msg->u.request.u.shutdown_ns.nid
                    , msg->u.request.u.shutdown_ns.pid
                    , msg->u.request.u.shutdown_ns.level
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    int nid = msg->u.request.u.shutdown_ns.nid;
    int pid = msg->u.request.u.shutdown_ns.pid;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, nid, pid, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::monReqUnknown( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqUnknown";
    TRACE_ENTRY;
    if ( trace_settings & ( TRACE_NS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request UNKNOWN data.\n"
                    , method_name, __LINE__
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    msgType = CExternalReq::NonStartupMsg;
    // Place new request on request queue
    ReqQueue.enqueueReq(msgType, -1, -1, sockFd, msg);

    TRACE_EXIT;
}

void CCommAcceptMon::processMonReqs( int sockFd )
{
    const char method_name[] = "CCommAcceptMon::processMonReqs";
    TRACE_ENTRY;

    int rc;
    nodeId_t nodeId;
    struct message_def msg;

    if ( trace_settings & ( TRACE_NS ) )
    {
        trace_printf( "%s@%d - Accepted connection sock=%d\n"
                    , method_name, __LINE__, sockFd );
    }

    // Get info about connecting monitor
    rc = Monitor->ReceiveSock( (char *) &nodeId
                             , sizeof(nodeId_t)
                             , sockFd
                             , method_name );
    if ( rc )
    {   // Handle error
        close( sockFd );
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                 "monitor: %s.\n", method_name, ErrorMsg(rc));
        mon_log_write(NS_COMMACCEPT_PROCESSMONREQS_1, SQ_LOG_ERR, buf);
        return;
    }

    if ( trace_settings & ( TRACE_NS ) )
    {
        trace_printf( "%s@%d - Accepted connection from pnid=%d\n"
                      "        nodeId.nodeName=%s\n"
                      "        nodeId.commPort=%s\n"
                      "        nodeId.syncPort=%s\n"
                      "        nodeId.creatorPNid=%d\n"
                      "        nodeId.creator=%d\n"
                      "        nodeId.creatorShellPid=%d\n"
                      "        nodeId.creatorShellVerifier=%d\n"
                      "        nodeId.ping=%d\n"
                    , method_name, __LINE__
                    , nodeId.pnid
                    , nodeId.nodeName
                    , nodeId.commPort
                    , nodeId.syncPort
                    , nodeId.creatorPNid
                    , nodeId.creator
                    , nodeId.creatorShellPid
                    , nodeId.creatorShellVerifier
                    , nodeId.ping );
    }

    CNode  *node;
    node = Nodes->GetNode( nodeId.pnid );
    if ( node != NULL )
    {
        if ( node->GetState() != State_Up )
        {
            if ( trace_settings & ( TRACE_NS ) )
            {
                trace_printf( "%s@%d - Bringing node up, node=%s, pnid=%d\n"
                            , method_name, __LINE__
                            , node->GetName(), node->GetPNid() );
            }
            rc = Monitor->HardNodeUpNs( node->GetPNid() );
            if ( rc )
            {   // Handle error
                close( sockFd );
                return;
            }
        }
    }
    else
    {   // Handle error
        close( sockFd );
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], invalid physical node id, "
                 "pnid: %d\n", method_name, nodeId.pnid );
        mon_log_write(NS_COMMACCEPT_PROCESSMONREQS_2, SQ_LOG_ERR, buf);
        return;
    }

    strcpy(nodeId.nodeName, MyNode->GetName());
    strcpy(nodeId.commPort, MyNode->GetCommPort());
    strcpy(nodeId.syncPort, MyNode->GetSyncPort());
    nodeId.pnid = MyNode->GetPNid();
    nodeId.creatorPNid = -1;
    nodeId.creatorShellPid = -1;
    nodeId.creatorShellVerifier = -1;
    nodeId.creator = false;
    nodeId.ping = false;
    nodeId.nsPid = getpid();
    int pnidNs = processMonReqsGetBestNs();
    nodeId.nsPNid = pnidNs;

    if ( trace_settings & ( TRACE_NS ) )
    {
        trace_printf( "%s@%d - Sending nodeId back\n"
                      "        nodeId.nodeName=%s\n"
                      "        nodeId.commPort=%s\n"
                      "        nodeId.syncPort=%s\n"
                      "        nodeId.pnid=%d\n"
                      "        nodeId.nsPid=%d\n"
                      "        nodeId.nsPNid=%d\n"
                    , method_name, __LINE__
                    , nodeId.nodeName
                    , nodeId.commPort
                    , nodeId.syncPort
                    , nodeId.pnid
                    , nodeId.nsPid
                    , nodeId.nsPNid );
    }

    // return Send info to connecting monitor
    rc = Monitor->SendSock( (char *) &nodeId
                          , sizeof(nodeId_t)
                          , sockFd
                          , method_name );
    if ( rc )
    {   // Handle error
        close( sockFd );
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], unable to send node id from new "
                 "monitor: %s.\n", method_name, ErrorMsg(rc));
        mon_log_write(NS_COMMACCEPT_PROCESSMONREQS_3, SQ_LOG_ERR, buf);
        return;
    }

    while ( true )
    {
        // Get monitor request (hdr)
        int size;
        rc = Monitor->ReceiveSock( (char *) &size, sizeof(size), sockFd, method_name );
        if ( rc )
        {   // Handle error
            close( sockFd );
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to obtain message size from "
                     "monitor: %s.\n", method_name, ErrorMsg(rc));
            mon_log_write(NS_COMMACCEPT_PROCESSMONREQS_4, SQ_LOG_ERR, buf);
            return;
        }

        rc = Monitor->ReceiveSock( (char *) &msg, size, sockFd, method_name );
        if ( rc )
        {   // Handle error
            close( sockFd );
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to obtain message from "
                     "monitor: %s.\n", method_name, ErrorMsg(rc));
            mon_log_write(NS_COMMACCEPT_PROCESSMONREQS_5, SQ_LOG_ERR, buf);
            return;
        }
        if ( trace_settings & ( TRACE_NS ) )
        {
            const char *mtype = "?";
            if ( msg.type == MsgType_Service )
                mtype = "Service";
            const char *rtype;
            switch (msg.u.request.type)
            {
            case ReqType_DelProcessNs:
                rtype = "DelProcessNs";
                break;
            case ReqType_ProcessInfo:
                rtype = "ProcessInfo";
                break;
            case ReqType_ProcessInfoCont:
                rtype = "ProcessInfoCont";
                break;
            case ReqType_ProcessInfoNs:
                rtype = "ProcessInfoNs";
                break;
            case ReqType_NameServerStop:
                rtype = "NameServerStop";
                break;
            case ReqType_NewProcessNs:
                rtype = "NewProcessNs";
                break;
            case ReqType_ShutdownNs:
                rtype = "ShutdownNs";
                break;
            default:
                rtype = "?";
            }
                
            trace_printf( "%s@%d - Received monitor request, sock=%d.\n"
                          "        msg.type=%d(%s)\n"
                          "        msg.noreply=%d\n"
                          "        msg.reply_tag=%d\n"
                          "        msg.u.request.type=%d(%s)\n"
                        , method_name, __LINE__
                        , sockFd
                        , msg.type
                        , mtype
                        , msg.noreply
                        , msg.reply_tag
                        , msg.u.request.type
                        , rtype
                        );
        }
        switch (msg.u.request.type)
        {
        case ReqType_DelProcessNs:
            monReqDeleteProcess(&msg, sockFd);
            break;

        case ReqType_NameServerStop:
            monReqNameServerStop(&msg, sockFd);
            break;

        case ReqType_NodeDown:
            monReqNodeDown(&msg, sockFd);
            break;

        case ReqType_ProcessInfo:
            monReqProcessInfo(&msg, sockFd);
            break;

        case ReqType_ProcessInfoCont:
            monReqProcessInfoCont(&msg, sockFd);
            break;

        case ReqType_ProcessInfoNs:
            monReqProcessInfoNs(&msg, sockFd);
            break;

        case ReqType_NewProcessNs:
            monReqNewProcess(&msg, sockFd);
            break;

        case ReqType_ShutdownNs:
            monReqShutdown(&msg, sockFd);
            break;

        default:
            monReqUnknown(&msg, sockFd);
            break;
        }
    }

    TRACE_EXIT;
}

int CCommAcceptMon::processMonReqsGetBestNs( void )
{
    const char method_name[] = "CCommAcceptMon::processMonReqsGetBestNs";
    TRACE_ENTRY;
    int pnid;

    int myCount = MyNode->GetMonConnCount();
    int minCount = Monitor->GetMinMonConnCount();
    if ( myCount <= (minCount + HEURISTIC_COUNT) )
    {
        pnid = -1;
    }
    else
    {
        pnid = Monitor->GetMinMonConnPnid();
    }
    if ( trace_settings & ( TRACE_NS ) )
    {
        trace_printf("%s@%d - myCount=%d, minCount=%d, pnid=%d\n"
                    ,method_name, __LINE__, myCount, myCount, pnid);
    }

    TRACE_EXIT;
    return pnid;
}

void CCommAcceptMon::processNewSock( int joinFd )
{
    const char method_name[] = "CCommAcceptMon::processNewSock";
    TRACE_ENTRY;

    int rc;

    mem_log_write(CMonLog::MON_NSCONNTONEWMON_1);

    // need to create context in case back-to-back accept is too fast
    Context *ctx = new Context();
    ctx->this_ = this;
    ctx->pendingFd_ = joinFd;
    rc = pthread_create(&process_thread_id_, NULL, mon2nsProcess, ctx);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], mon2nsProcess thread create error=%d\n",
                 method_name, rc);
        mon_log_write(NS_COMMACCEPT_PROCESSNEWSOCK_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

void CCommAcceptMon::commAcceptor()
{
    const char method_name[] = "CCommAcceptMon::commAcceptor";
    TRACE_ENTRY;

    switch( CommType )
    {
        case CommType_Sockets:
            commAcceptorSock();
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    TRACE_EXIT;
    pthread_exit(0);
}

// commAcceptor thread main processing loop.  Keep an accept
// request outstanding.  After accepting a connection process it.
void CCommAcceptMon::commAcceptorSock()
{
    const char method_name[] = "CCommAcceptMon::commAcceptorSock";
    TRACE_ENTRY;

    int joinFd = -1;

    if ( trace_settings & ( TRACE_NS ) )
    {
        trace_printf("%s@%d thread %lx starting\n", method_name,
                     __LINE__, thread_id_);
    }

    while ( true )
    {
        if ( isAccepting() )
        {
            if ( trace_settings & ( TRACE_NS ) )
            {
                trace_printf("%s@%d - Posting accept\n", method_name, __LINE__);
            }

            mem_log_write(CMonLog::MON_NSCONNTONEWMON_2);
            joinFd = Monitor->AcceptMon2NsSock();
        }
        else
        {
            if ( trace_settings & ( TRACE_NS) )
            {
                trace_printf("%s@%d - Waiting to post accept\n", method_name, __LINE__);
            }

            CLock::lock();
            CLock::wait();
            CLock::unlock();
            if ( !shutdown_ )
            {
                continue; // Ok to accept another connection
            }
        }

        if ( shutdown_ )
        {   // We are being notified to exit.
            break;
        }

        if ( joinFd < 0 )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], cannot accept new monitor: %s.\n",
                     method_name, strerror(errno));
            mon_log_write(NS_COMMACCEPT_COMMACCEPTORSOCK_1, SQ_LOG_ERR, buf);

        }
        else
        {
            processNewSock( joinFd );
        }
    }

    if ( !(joinFd < 0) ) close( joinFd );

    if ( trace_settings & ( TRACE_NS ) )
        trace_printf("%s@%d thread %lx exiting\n", method_name,
                     __LINE__, pthread_self());

    TRACE_EXIT;
}

void CCommAcceptMon::shutdownWork(void)
{
    const char method_name[] = "CCommAcceptMon::shutdownWork";
    TRACE_ENTRY;

    // Set flag that tells the commAcceptor thread to exit
    shutdown_ = true;
    Monitor->ConnectToMon2NsCommSelf();
    CLock::wakeOne();

    if ( trace_settings & ( TRACE_NS ) )
        trace_printf("%s@%d waiting for mon2nsAcceptMon thread %lx to exit.\n",
                     method_name, __LINE__, thread_id_);

    // Wait for commAcceptor thread to exit
    pthread_join(thread_id_, NULL);

    TRACE_EXIT;
}

// Initialize mon2nsAcceptor thread
static void *mon2nsAcceptMon(void *arg)
{
    const char method_name[] = "mon2nsAcceptMon";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CommAcceptMon object
    CCommAcceptMon *cao = (CCommAcceptMon *) arg;

    // Mask all allowed signals
    sigset_t  mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], pthread_sigmask error=%d\n",
                 method_name, rc);
        mon_log_write(NS_COMMACCEPT_MON2NSACCEPTMON_1, SQ_LOG_ERR, buf);
    }

    // Enter thread processing loop
    cao->commAcceptor();

    TRACE_EXIT;
    return NULL;
}

// Initialize mon2nsProcess thread
static void *mon2nsProcess(void *arg)
{
    const char method_name[] = "mon2nsProcess";
    TRACE_ENTRY;

    // Parameter passed to the thread is an context
    CCommAcceptMon::Context *ctx = (CCommAcceptMon::Context *) arg;
    CCommAcceptMon *cao = ctx->this_;

    // Mask all allowed signals
    sigset_t  mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], pthread_sigmask error=%d\n",
                 method_name, rc);
        mon_log_write(NS_COMMACCEPT_MON2NSPROCESS_1, SQ_LOG_ERR, buf);
    }

    MyNode->AddMonConnCount(1);

    // Enter thread processing loop
    cao->processMonReqs(ctx->pendingFd_);
    delete ctx;

    MyNode->AddMonConnCount(-1);

    TRACE_EXIT;
    return NULL;
}


// Create a commAcceptorMon thread
void CCommAcceptMon::start()
{
    const char method_name[] = "CCommAcceptMon::start";
    TRACE_ENTRY;

    int rc = pthread_create(&thread_id_, NULL, mon2nsAcceptMon, this);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], thread create error=%d\n",
                 method_name, rc);
        mon_log_write(NS_COMMACCEPT_START_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

void CCommAcceptMon::startAccepting( void )
{
    const char method_name[] = "CCommAcceptMon::startAccepting";
    TRACE_ENTRY;

    CAutoLock lock( getLocker( ) );

    if ( !accepting_ )
    {
        accepting_ = true;
        if ( trace_settings & ( TRACE_NS ) )
        {
            trace_printf( "%s@%d - Enabling accepting_=%d\n"
                        , method_name, __LINE__, accepting_ );
        }
        CLock::wakeOne();
    }

    TRACE_EXIT;
}

void CCommAcceptMon::stopAccepting( void )
{
    const char method_name[] = "CCommAcceptMon::stopAccepting";
    TRACE_ENTRY;

    CAutoLock lock( getLocker( ) );

    if ( accepting_ )
    {
        accepting_ = false;
        if ( trace_settings & ( TRACE_NS ) )
        {
            trace_printf( "%s@%d - Disabling accepting_=%d\n"
                        , method_name, __LINE__, accepting_ );
        }
        CLock::wakeOne();
    }

    TRACE_EXIT;
}
