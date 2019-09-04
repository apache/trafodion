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
#include <stdlib.h>
#include <unistd.h>


#include "SCMVersHelp.h"
#include "clio.h"
#include "monlogging.h"
#include "msgdef.h"
#include "seabed/trace.h"
#include "montrace.h"
#include "clusterconf.h"
#include "pstartd.h"


const char *MyName;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connection port - not used

long trace_settings = 0;
bool IsRealCluster = true;
int MyPNID = -1;
int MyZid = -1;
int MyNid = -1;
int MyPid = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
int gv_ms_su_pid = -1;          // Local IO pid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
bool tracing = false;
bool shuttingDown = false;

CClusterConfig ClusterConfig; // Trafodion Configuration objects
CMonUtil monUtil;
CPStartD *pStartD;
CMonLog *MonLog = NULL;

DEFINE_EXTERN_COMP_DOVERS(pstartd)
DEFINE_EXTERN_COMP_PRINTVERS(pstartd)

const char *ProcessTypeString( PROCESSTYPE type );

const char *MessageTypeString( MSGTYPE type )
{
    const char *str = NULL;
    
    switch( type )
    {
        case MsgType_Change:
            str = "MsgType_Change";
            break;
        case MsgType_Close:
            str = "MsgType_Close";
            break;
        case MsgType_Event:
            str = "MsgType_Event";
            break;
        case MsgType_NodeAdded:
            str = "MsgType_NodeAdded";
            break;
        case MsgType_NodeChanged:
            str = "MsgType_NodeChanged";
            break;
        case MsgType_NodeDeleted:
            str = "MsgType_NodeDeleted";
            break;
        case MsgType_NodeDown:
            str = "MsgType_NodeDown";
            break;
        case MsgType_NodeJoining:
            str = "MsgType_NodeJoining";
            break;
        case MsgType_NodeQuiesce:
            str = "MsgType_NodeQuiesce";
            break;
        case MsgType_NodeUp:
            str = "MsgType_NodeUp";
            break;
        case MsgType_Open:
            str = "MsgType_Open";
            break;
        case MsgType_ProcessCreated:
            str = "MsgType_ProcessCreated";
            break;
        case MsgType_ProcessDeath:
            str = "MsgType_ProcessDeath";
            break;
        case MsgType_ReintegrationError:
            str = "MsgType_ReintegrationError";
            break;
        case MsgType_Service:
            str = "MsgType_Service";
            break;
        case MsgType_Shutdown:
            str = "MsgType_Shutdown";
            break;
        case MsgType_SpareUp:
            str = "MsgType_SpareUp";
            break;
        default:
            str = "MsgType - Undefined";
            break;
    }
    return( str );
}

void InitLocalIO( void )
{
    gp_local_mon_io = new Local_IO_To_Monitor( -1 );
}

void localIONoticeCallback(struct message_def *recv_msg, int )
{
    const char method_name[] = "localIONoticeCallback";
    char buf[MON_STRING_BUF_SIZE];

    CAutoLock autoLock(pStartD->getLocker());

    if ( tracing )
    {
        trace_printf( "%s@%d Received notice: %s\n",
                      method_name, __LINE__,
                      MessageTypeString( recv_msg->type ) );
    }
    
    switch ( recv_msg->type )
    {
    case MsgType_NodeDown:
    case MsgType_NodeQuiesce:
    case MsgType_NodeJoining:
        if ( tracing )
        {
            trace_printf( "%s@%d CB Notice: Type=%d\n",
                          method_name, __LINE__, recv_msg->type);
        }
        break;

    case MsgType_Shutdown:
        if ( tracing )
        {
            trace_printf( "%s@%d Shutdown notice received\n",
                          method_name, __LINE__);
        }
        shuttingDown = true;

        snprintf( buf, sizeof(buf), "Received 'Shutdown' event.\n");
        monproc_log_write( LIO_NOTICE_CALLBACK_1, SQ_LOG_INFO, buf );

        CShutdownReq * reqShutdown;
        reqShutdown = new CShutdownReq();
        pStartD->enqueueReq( reqShutdown );
        pStartD->CLock::wakeOne();
        break;

    case MsgType_NodeUp:
        if ( tracing )
        {
            trace_printf( "%s@%d Node up notice: Nid=%d, name=%s\n",
                          method_name, __LINE__, recv_msg->u.request.u.up.nid,
                          recv_msg->u.request.u.up.node_name);
        }
        break;

    case MsgType_ProcessDeath:
        if ( tracing )
        {
            trace_printf( "%s@%d Process death notice for %s (%d, %d), "
                          "aborted=%d\n",
                          method_name, __LINE__,
                          recv_msg->u.request.u.death.process_name,
                          recv_msg->u.request.u.death.nid,
                          recv_msg->u.request.u.death.pid,
                          recv_msg->u.request.u.death.aborted);
        }
        break;

    default:
        snprintf( buf, sizeof(buf),
                  "[%s], Unexpected callback notice, type=%d\n",
                  method_name, recv_msg->type );
        monproc_log_write( LIO_NOTICE_CALLBACK_2, SQ_LOG_ERR, buf );

    }
}

void localIOEventCallback(struct message_def *recv_msg, int )
{
    const char method_name[] = "localIOEventCallback";

    recv_msg->u.request.u.event_notice.data
        [recv_msg->u.request.u.event_notice.length] = '\0';
    if ( tracing )
    {
        trace_printf( "%s@%d CB Event: Type=%d, id=%d(%s), data length=%d, data=%s\n",
                      method_name,  __LINE__, recv_msg->type,
                      recv_msg->u.request.u.event_notice.event_id,
                      recv_msg->u.request.u.event_notice.event_id
                        == PStartD_StartPersist 
                        ? "PStartD_StartPersist"
                        : "PStartD_StartPersistDTM",
                      recv_msg->u.request.u.event_notice.length,
                      recv_msg->u.request.u.event_notice.data);
    }

    int eventId = recv_msg->u.request.u.event_notice.event_id;
    errno = 0;
    int nid = strtol(recv_msg->u.request.u.event_notice.data, NULL, 10);
    if ( errno != 0 )
    {   // Could not convert node id
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf),
                  "[%s], Unable to convert event data [%s] to node id\n",
                  method_name, recv_msg->u.request.u.event_notice.data );
        monproc_log_write( LIO_EVENT_CALLBACK_1, SQ_LOG_ERR, buf );

        return;
    }

    CAutoLock autoLock(pStartD->getLocker());
    CNodeUpReq * req = NULL;

    if ( eventId == PStartD_StartPersist )
    {
        req = new CNodeUpReq(nid, (char *) "", false);

    }
    else if ( eventId == PStartD_StartPersistDTM )
    {
        req = new CNodeUpReq(nid, (char *) "", true);
    }
    else
    {
        // Unexpected event
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf),
                  "[%s], Unexpected event id=%d (ignored)\n",
                  method_name, eventId );
        monproc_log_write( LIO_EVENT_CALLBACK_2, SQ_LOG_ERR, buf );
    }

    if ( req != NULL )
    {   // enqueue event for processing
        pStartD->enqueueReq( req );
        pStartD->CLock::wakeOne();
    }
}

// ignore these, prevent leak
void localIORecvCallback(struct message_def *recv_msg, int )
{
    const char method_name[] = "localIORecvCallback";

    if ( tracing )
    {
        trace_printf( "%s@%d CB Recv: Type=%d\n",
                      method_name,  __LINE__, recv_msg->type);
    }
}

CMonUtil::CMonUtil(): nid_(-1), pid_(-1), verifier_(-1), trace_(false)
{
    processName_[0] = '\0';
    port_[0] = '\0';
}

CMonUtil::~CMonUtil()
{
}

char * CMonUtil::MPIErrMsg ( int code )
{
    int length;
    static char buffer[MPI_MAX_ERROR_STRING];

    if (MPI_Error_string (code, buffer, &length) != MPI_SUCCESS)
    {
        snprintf(buffer, sizeof(buffer),
                 "MPI_Error_string: Invalid error code (%d)\n", code);
        length = strlen(buffer);
    }
    buffer[length] = '\0';

    return buffer;
}

bool CMonUtil::requestGet ( ConfigType type,
                            const char *group,
                            const char *key,
                            bool resumeFlag,
                            struct Get_reply_def *& regData
                            )
{
    /*  ReqType_Get arguments:
      type: ConfigType_Cluster, ConfigType_Node, or ConfigType_Process
      next: false if start from beginning, true if start from key
      group: name of group, if NULL and type=ConfigNode assume local node
      key: name of the item to be returned, empty string for all in group
    */

    const char method_name[] = "CMonUtil::requestGet";

    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s], Unable to acquire message buffer\n",
                  method_name );
        monproc_log_write( MONUTIL_REQUEST_GET_1, SQ_LOG_ERR, buf );

        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Get;
    msg->u.request.u.get.nid = nid_;
    msg->u.request.u.get.pid = pid_;
    msg->u.request.u.get.verifier = verifier_;
    msg->u.request.u.get.process_name[0] = 0;
    msg->u.request.u.get.type = type;
    msg->u.request.u.get.next = resumeFlag;
    STRCPY(msg->u.request.u.get.group, group);
    STRCPY(msg->u.request.u.get.key, key);

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Get))
        {
            regData = (Get_reply_def*) malloc(sizeof(Get_reply_def));
            regData->type = msg->u.reply.u.get.type;
            strcpy(regData->group, msg->u.reply.u.get.group);
            regData->num_keys = msg->u.reply.u.get.num_keys;
            regData->num_returned = msg->u.reply.u.get.num_returned;
            for (int i=0; i<msg->u.reply.u.get.num_returned; i++)
            {
                strcpy(regData->list[i].key, msg->u.reply.u.get.list[i].key);
                strcpy(regData->list[i].value, msg->u.reply.u.get.list[i].value);
            }
            result = true;
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf),
                  "[%s] Get reply message invalid.  Reply tag=%d, count=%d "
                  "(expected %d)\n", method_name, msg->reply_tag,
                  count, (int) sizeof (struct message_def) );
        monproc_log_write( MONUTIL_REQUEST_GET_2, SQ_LOG_ERR, buf );

    }

    gp_local_mon_io->release_msg(msg);

    return result;
}


void CMonUtil::requestExit ( void )
{
    const char method_name[] = "CMonUtil::requestExit";

    int count;
    MPI_Status status;
    struct message_def *msg;

    if ( trace_ )
    {
        trace_printf ("%s@%d sending exit process message.\n",
                      method_name, __LINE__);
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s], Unable to acquire message buffer\n",
                  method_name );
        monproc_log_write( MONUTIL_REQUEST_EXIT_1, SQ_LOG_ERR, buf );

        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Exit;
    msg->u.request.u.exit.nid = nid_;
    msg->u.request.u.exit.pid = pid_;
    msg->u.request.u.exit.verifier = verifier_;
    msg->u.request.u.exit.process_name[0] = 0;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code != MPI_SUCCESS)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf),
                          "[%s], exit process failed, rc=%d (%s)\n",
                          method_name, msg->u.reply.u.generic.return_code,
                          MPIErrMsg(msg->u.reply.u.generic.return_code));
                monproc_log_write( MONUTIL_REQUEST_EXIT_2, SQ_LOG_ERR, buf );
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s], Invalid MsgType(%d)/ReplyType(%d) for Exit "
                      "message\n", method_name, msg->type, msg->u.reply.type );
            monproc_log_write( MONUTIL_REQUEST_EXIT_3, SQ_LOG_ERR, buf );
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf),
                  "[%s] exit process reply message invalid.  Reply tag=%d, "
                  "count=%d (expected %d)\n",
                  method_name, msg->reply_tag,
                  count, (int) sizeof (struct message_def) );
        monproc_log_write( MONUTIL_REQUEST_EXIT_4, SQ_LOG_ERR, buf );
    }

    gp_local_mon_io->release_msg(msg);
}


bool CMonUtil::requestNewProcess (int nid, PROCESSTYPE type,
                                  const char *processName,
                                  const char *progName, const char *inFile,
                                  const char *outFile,
                                  int progArgc, const char *progArgs,
//                                  int argBegin[], int argLen[],
                                  int& newNid, int& newPid,
                                  char *newProcName)
{
    const char method_name[] = "CMonUtil::requestNewProcess";

    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( trace_ )
    {
        trace_printf ("%s@%d starting process %s on node=%d.\n",
                      method_name, __LINE__, processName, nid);
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s], Unable to acquire message buffer\n",
                  method_name );
        monproc_log_write( MONUTIL_REQUEST_NEWPROC_1, SQ_LOG_ERR, buf );

        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NewProcess;
    msg->u.request.u.new_process.nid = nid;
    msg->u.request.u.new_process.type = type;
    msg->u.request.u.new_process.debug = 0;
    msg->u.request.u.new_process.priority = 0;
    msg->u.request.u.new_process.backup = 0;
    msg->u.request.u.new_process.unhooked = true; // keep child alive if this process aborts
    msg->u.request.u.new_process.nowait = false;
    msg->u.request.u.new_process.tag = 0;
    strcpy (msg->u.request.u.new_process.process_name, processName);
    strcpy (msg->u.request.u.new_process.path, getenv ("PATH"));
    strcpy (msg->u.request.u.new_process.ldpath, getenv ("LD_LIBRARY_PATH"));
    strcpy (msg->u.request.u.new_process.program, progName);
    STRCPY (msg->u.request.u.new_process.infile, inFile);
    STRCPY (msg->u.request.u.new_process.outfile, outFile);
    msg->u.request.u.new_process.argc = progArgc;

    char *token, *programArgs = NULL;
    int argi = 0;
    static const char *delim = " ";

    programArgs = new char [MAX_ARG_SIZE*MAX_ARGS];
    memset(programArgs, 0, MAX_ARG_SIZE*MAX_ARGS);
    memcpy(programArgs, progArgs, strlen(progArgs)+1);
    token = strtok( programArgs, delim );
    while (token != NULL)
    {
        if ( trace_ )
        {
            trace_printf( "%s@%d Setting progArgc=%d, argv[%d]=%s\n"
                        , method_name, __LINE__, progArgc, argi, token);
        }
        strncpy( msg->u.request.u.new_process.argv[argi]
               , token
               , strlen(token));
        argi++;
        token = strtok( NULL, delim );
    }

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_NewProcess))
        {
            if (msg->u.reply.u.new_process.return_code == MPI_SUCCESS)
            {
                if ( trace_ )
                {
                    trace_printf
                        ("%s@%d started process successfully. Nid=%d, "
                         "Pid=%d, Process_name=%s, rtn=%d\n",
                         method_name, __LINE__, msg->u.reply.u.new_process.nid,
                         msg->u.reply.u.new_process.pid,
                         msg->u.reply.u.new_process.process_name,
                         msg->u.reply.u.new_process.return_code);
                }
                result = true;
                newNid = msg->u.reply.u.new_process.nid;
                newPid = msg->u.reply.u.new_process.pid;
                strcpy(newProcName, msg->u.reply.u.new_process.process_name);
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf),
                          "[%s], new process failed to spawn, rc=%d (%s)\n",
                          method_name, msg->u.reply.u.new_process.return_code,
                          MPIErrMsg(msg->u.reply.u.new_process.return_code) );
                monproc_log_write( MONUTIL_REQUEST_NEWPROC_2, SQ_LOG_ERR, buf );
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s], Invalid MsgType(%d)/ReplyType(%d) for Exit "
                      "message\n", method_name, msg->type, msg->u.reply.type );
            monproc_log_write( MONUTIL_REQUEST_NEWPROC_3, SQ_LOG_ERR, buf );
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] new process reply message invalid."
                  "  Reply tag=%d, count=%d (expected %d)\n",
                  method_name, msg->reply_tag,
                  count, (int) sizeof (struct message_def));
        monproc_log_write( MONUTIL_REQUEST_NEWPROC_4, SQ_LOG_ERR, buf );
    }

    gp_local_mon_io->release_msg(msg);

    return result;

}


bool CMonUtil::requestProcInfo( const char *processName, int &nid, int &pid )
{
    const char method_name[] = "CMonUtil::requestProcInfo";

    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s], Unable to acquire message buffer\n",
                  method_name );
        monproc_log_write( MONUTIL_REQUEST_PROCINFO_1, SQ_LOG_ERR, buf );

        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfo;
    msg->u.request.u.process_info.nid = nid_;
    msg->u.request.u.process_info.pid = pid_;
    msg->u.request.u.process_info.verifier = verifier_;
    msg->u.request.u.process_info.process_name[0] = 0;
    msg->u.request.u.process_info.target_nid = -1;
    msg->u.request.u.process_info.target_pid = -1;
    msg->u.request.u.process_info.target_verifier = -1;
    strcpy(msg->u.request.u.process_info.target_process_name, processName);
    msg->u.request.u.process_info.type = ProcessType_Undefined;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ( (status.MPI_TAG == REPLY_TAG) &&
         (count == sizeof (struct message_def)) )
    {
        if ( (msg->type == MsgType_Service) &&
             (msg->u.reply.type == ReplyType_ProcessInfo) )
        {
            if ( msg->u.reply.u.process_info.return_code == MPI_SUCCESS )
            {
                if ( msg->u.reply.u.process_info.num_processes == 1 )
                {
                    if ( trace_ )
                    {
                        trace_printf ( "%s@%d Got process status for %s "
                                       "(%d, %d), state=%s\n",
                                       method_name, __LINE__,
                             msg->u.reply.u.process_info.process[0].process_name,
                             msg->u.reply.u.process_info.process[0].nid,
                             msg->u.reply.u.process_info.process[0].pid,
                             (msg->u.reply.u.process_info.process[0].state == State_Up) ? "Up" : "not Up");
                    }
                    nid = msg->u.reply.u.process_info.process[0].nid;
                    pid = msg->u.reply.u.process_info.process[0].pid;
                    result = true;
                }
                else
                {
                    if ( trace_ )
                    {
                        trace_printf( "%s@%d - process info for %s returned data for %d processes\n"
                                , method_name, __LINE__
                                , processName
                                , msg->u.reply.u.process_info.num_processes);
                    }
                }
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf),
                          "[%s] ProcessInfo failed, rc=%d (%s)\n",
                          method_name, msg->u.reply.u.process_info.return_code,
                          MPIErrMsg(msg->u.reply.u.process_info.return_code));
                monproc_log_write( MONUTIL_REQUEST_PROCINFO_2, SQ_LOG_ERR, buf );
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s], Invalid MsgType(%d)/ReplyType(%d) for "
                      "ProcessInfo\n", method_name, msg->type,
                      msg->u.reply.type );
            monproc_log_write( MONUTIL_REQUEST_PROCINFO_3, SQ_LOG_ERR, buf );

        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf),
                  "[%s] ProcessInfo reply message invalid.  Reply tag=%d, "
                  "count=%d (expected %d)\n",
                  method_name,  msg->reply_tag,
                  count, (int) sizeof (struct message_def) );
        monproc_log_write( MONUTIL_REQUEST_PROCINFO_4, SQ_LOG_ERR, buf );
    }

    gp_local_mon_io->release_msg(msg);
    msg = NULL;

    return result;
}

void CMonUtil::requestStartup ( )
{
    const char method_name[] = "CMonUtil::requestStartup";

    struct message_def *msg;

    gp_local_mon_io->iv_pid = pid_;
    gp_local_mon_io->iv_verifier = verifier_;
    gp_local_mon_io->init_comm();

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s], Unable to acquire message buffer\n",
                  method_name );
        monproc_log_write( MONUTIL_REQUEST_STARTUP_1, SQ_LOG_ERR, buf );

        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = true;
    msg->u.request.type = ReqType_Startup;
    msg->u.request.u.startup.nid = nid_;
    msg->u.request.u.startup.pid = pid_;
    msg->u.request.u.startup.paired = false;
    strcpy (msg->u.request.u.startup.process_name, processName_);
    strcpy (msg->u.request.u.startup.port_name, port_);
    msg->u.request.u.startup.os_pid = getpid ();
    msg->u.request.u.startup.event_messages = true;
    msg->u.request.u.startup.system_messages = true;
    msg->u.request.u.startup.verifier = gv_ms_su_verif;
    msg->u.request.u.startup.startup_size = sizeof(msg->u.request.u.startup);

    if ( trace_ )
    {
        trace_printf ("%s@%d sending startup reply to monitor.\n",
                      method_name, __LINE__);
    }

    gp_local_mon_io->send( msg );
}


void CMonUtil::processArgs( int argc, char *argv[] )
{
    const char method_name[] = "CMonUtil::processArgs";

    // enable tracing if tracing was enabled via TraceInit
    if ( tracing ) trace_ = true;

    if ( trace_ )
    {
        trace_printf( "%s@%d CMonUtil::processArgs processing arguments.\n",
                      method_name, __LINE__ );
    }

    if (argc < 11)
    {
        printf("Error: Invalid startup arguments, argc=%d, argv[0]=%s, "
               "argv[1]=%s, argv[2]=%s, argv[3]=%s, argv[4]=%s, argv[5]=%s, "
               "argv[6]=%s, argv[7]=%s, argv[8]=%s, argv[9]=%s, argv[10]=%s, "
               "argv[11]=%s\n"
               , argc, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]
               , argv[6], argv[7], argv[8], argv[9], argv[10], argv[11]);
        exit (1);
    }

    if ( trace_ )
    {   // Trace program arguments
        char argInfo[500];
        int len = snprintf(argInfo, sizeof(argInfo), "argc=%d", argc);
        int n;
        for (int i=0; i<argc; i++)
        {
            n = snprintf (&argInfo[len], sizeof(argInfo)-len,
                          ", argv[%d]=%s", i, argv[i]);
            if ( n >= (int) (sizeof(argInfo)-len) ) break;
            len += n;
        }
        trace_printf( "%s@%d - %s\n", method_name, __LINE__, argInfo);
    }

    pnid_ = atoi(argv[2]);
    nid_ = atoi(argv[3]);
    pid_ = atoi(argv[4]);
    zid_ = atoi(argv[8]);
    gv_ms_su_verif  = verifier_ = atoi(argv[9]);

    strncpy( processName_, argv[5], sizeof(processName_) );
    processName_[sizeof(processName_)-1] = '\0';
}

CNodeUpReq::CNodeUpReq(int nid, char nodeName[], bool requiresDTM)
          : nid_(nid)
          , requiresDTM_(requiresDTM)
    { 
        strncpy(nodeName_, nodeName, sizeof(nodeName_));
        nodeName_[sizeof(nodeName_)-1] = '\0';

        CLNodeConfig *lnodeConfig = ClusterConfig.GetLNodeConfig( nid );
        zid_ = lnodeConfig->GetZid();
    }

void CNodeUpReq::performRequest()
{
    const char method_name[] = "CNodeUpReq::performRequest";

    char buf[MON_STRING_BUF_SIZE];
    snprintf( buf, sizeof(buf)
            , "Received 'Node Up' event for node %d, "
              "requires DTM flag=%d\n"
            , nid_, requiresDTM_);
    monproc_log_write( MONUTIL_PERFORM_REQUEST_1, SQ_LOG_INFO, buf );

    if ( zid_ == MyZid )
    {
        if ( tracing )
        {
            trace_printf("%s@%d invoking startProcs(%d, %d)\n",
                         method_name, __LINE__, nid_, requiresDTM_);
        }
        pStartD->startProcs( requiresDTM_ );
    }
    else
    {
        if ( tracing )
        {
            trace_printf( "%s@%d Ignoring node up for for node %s, nid=%d, "
                          ", zid=%d, MyZid=%d\n"
                        , method_name, __LINE__
                        , nodeName_, nid_, zid_, MyZid );
        }
    }
}

CPStartD::CPStartD()
{
}

CPStartD::~CPStartD()
{
}

CRequest * CPStartD::getReq ( )
{
    CRequest *req = NULL;
    CAutoLock autoLock(getLocker());

    if (workQ_.size())
    {
        req = workQ_.front();
        workQ_.pop_front();
    }

    return req;
}

int CPStartD::getReqCount( )
{
    CAutoLock autoLock(getLocker());
    return(workQ_.size());
}

void CPStartD::enqueueReq(CRequest * req)
{
    CAutoLock autoLock( getLocker() );
    workQ_.push_back ( req );
}

bool CPStartD::loadConfiguration( void )
{
    if ( ClusterConfig.IsConfigReady() )
    {
        // It was previously loaded, remove the old configuration
        ClusterConfig.Clear();
    }

    if ( ClusterConfig.Initialize() )
    {
        if ( ! ClusterConfig.LoadConfig() )
        {
            printf("[%s], Failed to load cluster configuration.\n", MyName);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf( "[%s] Warning: No cluster.conf found\n",MyName);
        exit(EXIT_FAILURE);
    }

    return true;
}

void CPStartD::startProcess( CPersistConfig *persistConfig )
{
    const char method_name[] = "CPStartD::startProcess";

    PROCESSTYPE procType = ProcessType_Undefined;
    int progArgc = 0;
    string procName;
    string progArgs;
    string progStdout;
    string progProgram;
    char newProcName[MAX_PROCESS_PATH];
    bool result;
    int newNid;
    int newPid;
//    int argBegin[MAX_ARGS];
//    int argLen[MAX_ARGS];

    procName = persistConfig->GetProcessName( MyNid );
    procType = persistConfig->GetProcessType();
    progStdout = persistConfig->GetStdoutFile( MyNid );
    progProgram = persistConfig->GetProgramName();
    progArgs = persistConfig->GetProgramArgs();
    progArgc = persistConfig->GetProgramArgc();

    if ( tracing )
    {
        trace_printf( "%s@%d Will start process: nid=%d, type=%s, name=%s, "
                      "prog=%s, stdout=%s, argc=%d, args=%s\n"
                    , method_name, __LINE__, MyNid
                    , ProcessTypeString(procType), procName.c_str()
                    , progProgram.c_str(), progStdout.c_str()
                    , progArgc, progArgs.c_str());
    }

    char buf[MON_STRING_BUF_SIZE];
    snprintf( buf, sizeof(buf)
            , "Starting process %s on nid=%d, program=%s, type=%s\n"
            , procName.c_str(), MyNid, progProgram.c_str()
            , ProcessTypeString(procType));
    monproc_log_write( PSTARTD_START_PROCESS_1, SQ_LOG_INFO, buf );

    result = monUtil.requestNewProcess( MyNid
                                      , procType
                                      , procName.c_str()
                                      , progProgram.c_str()
                                      , ""
                                      , progStdout.c_str()
                                      , progArgc
                                      , progArgs.c_str()
                                    //  , argBegin
                                    //  , argLen
                                      , newNid
                                      , newPid
                                      , newProcName);
    if ( tracing )
    {
        trace_printf("%s@%d requestNewProcess returned: %d\n",
                     method_name, __LINE__, result);
    }
}

void CPStartD::startProcs ( bool requiresDTM )
{
    const char method_name[] = "CPStartD::startProcs";

    /*
     1. use persist configuration objects
     2. for each persistent configuration object:
        a) determine if the process type is candidate for restart in this nid
        b) ask monitor if process is currently running in this nid
        c) if not running, start it
    */

    bool foundConfig = false;
    CPersistConfig *persistConfig;
    string procName = "";
    
    persistConfig = ClusterConfig.GetFirstPersistConfig();
    if (persistConfig)
    {
        for ( ; persistConfig; persistConfig = persistConfig->GetNext() )
        {
            if ( tracing )
            {
                trace_printf( "%s@%d Persist Prefix =%s\n"
                              "\t\tProcess Name    = %s\n"
                              "\t\tProcess Type    = %s\n"
                              "\t\tProgram Name    = %s\n"
                              "\t\tSTDOUT          = %s\n"
                              "\t\tRequires DTM    = %s\n"
                              "\t\tPersist Retries = %d\n"
                              "\t\tPersist Window  = %d\n"
                              "\t\tPersist Zones   = %s\n"
                              "\t\tProgram Args    = %s\n"
                              "\t\tProgram Argc    = %d\n"
                            , method_name, __LINE__
                            , persistConfig->GetPersistPrefix()
                            , persistConfig->GetProcessName( MyNid )
                            , ProcessTypeString(persistConfig->GetProcessType())
                            , persistConfig->GetProgramName()
                            , persistConfig->GetStdoutFile( MyNid )
                            , persistConfig->GetRequiresDTM() ? "Y" : "N"
                            , persistConfig->GetPersistRetries()
                            , persistConfig->GetPersistWindow()
                            , persistConfig->GetZoneFormat()
                            , persistConfig->GetProgramArgs()
                            , persistConfig->GetProgramArgc() );
            }
    
            switch (persistConfig->GetProcessType())
            {
            case ProcessType_TMID:
            case ProcessType_PERSIST:
            case ProcessType_SSMP:
                if ( persistConfig->GetRequiresDTM() && !requiresDTM )
                {
                    if ( tracing )
                    {
                        trace_printf("%s@%d Persist type %s NOT targeted for restart DTM not ready\n",
                                     method_name, __LINE__, persistConfig->GetPersistPrefix() );
                    }
                    break;
                }
                else if ( !persistConfig->GetRequiresDTM() && requiresDTM )
                {
                    if ( tracing )
                    {
                        trace_printf("%s@%d Persist type %s NOT targeted for restart DTM ready\n",
                                     method_name, __LINE__, persistConfig->GetPersistPrefix() );
                    }
                    break;
                }

                procName = persistConfig->GetProcessName( MyNid );
        
                if ( tracing )
                {
                    trace_printf( "%s@%d Persist %s process type %s targeted for restart\n"
                                , method_name, __LINE__
                                , persistConfig->GetPersistPrefix()
                                , ProcessTypeString(persistConfig->GetProcessType()) );
                }
                
                if (procName.length() != 0)
                {
                    int procNid = -1;
                    int procPid = -1;
        
                    if (persistConfig->IsZoneMatch( MyZid ))
                    {
                        if ( ! monUtil.requestProcInfo( procName.c_str()
                                                      , procNid
                                                      , procPid))
                        {   // Process does not exist, so start it
                            startProcess( persistConfig );
                        }
                        else
                        {
                            if ( procNid != -1)
                            {
                                char buf[MON_STRING_BUF_SIZE];
                                snprintf( buf, sizeof(buf), "Not starting process %s "
                                          "because it is already running\n",
                                          procName.c_str());
                                monproc_log_write( PSTARTD_STARTPROCS_1, SQ_LOG_INFO, buf );
            
                                if ( tracing )
                                {
                                    trace_printf( "%s@%d %s"
                                                , method_name, __LINE__, buf);
                                }
                            }
                        }
                    }
                    else
                    {
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                , "[%s] Zone does not match for persist type %s"
                                  ", zone format=%s, MyZid=%d\n"
                                , method_name
                                , persistConfig->GetPersistPrefix()
                                , persistConfig->GetZoneFormat()
                                , MyZid );
                        monproc_log_write( PSTARTD_STARTPROCS_2, SQ_LOG_CRIT, buf );
                    }
                }
                break;
            case ProcessType_DTM:
            case ProcessType_PSD:
            case ProcessType_NameServer:
            case ProcessType_Watchdog:
            default:
                // Skip these, they are managed by the monitor
                if ( tracing )
                {
                    trace_printf("%s@%d Persist type %s NOT targeted for restart\n",
                                 method_name, __LINE__, persistConfig->GetPersistPrefix() );
                }
                break;
            }
        }
    }
    if (!foundConfig)
    {
        printf ("[%s] Persistent process configuration does not exist\n", MyName);
    }
}

void CPStartD::waitForEvent( void )
{
    CAutoLock autoLock(getLocker());
    wait();
}

//
// To enable tracing, add the following to sql/scripts/mon.env file:
// PSD_TRACE=1
//
void TraceInit( int & argc, char **& argv )
{
    char traceFileName[MAX_PROCESS_PATH];
    const char * currentDir = ".";

    // enable tracing if trace flag supplied
    for (int i=0; i<argc; i++)
    {
        if ( strcmp(argv[i], "-t") == 0 ) tracing = true;
    }

    // Determine trace file name
    const char *tmpDir = getenv( "TRAF_LOG" );
    snprintf( traceFileName, sizeof(traceFileName),
              "%s/pstartd.trace.%d", ((tmpDir != NULL) ? tmpDir : currentDir),
              getpid() );

    const char *envVar;
    envVar = getenv("PSD_TRACE_FILE");
    if (envVar != NULL &&
        ( strcmp(envVar, "STDOUT") == 0 ||  strcmp(envVar, "STDERR") == 0))
    {
        // Trace output goes to STDOUT or STDERR
        strcpy( traceFileName, envVar);
    }

    // Get trace settings from environment variables (typically set in mon.env)
    envVar = getenv("PSD_TRACE");
    if (envVar) tracing = true;

    // Get environment variable value for trace buffer size if specified
    envVar = getenv("PSD_TRACE_FILE_FB");
    int  traceFileFb = 0;
    if (envVar)
    {
        traceFileFb = atoi ( envVar );
    }

    // Initialize tracing if any trace flags set
    if ( tracing )
    {
        // Trace any errors that are logged
        trace_settings |= TRACE_EVLOG_MSG;

        // Initialize tracing
        trace_init(traceFileName,
                   false,  // don't append pid to file name
                   NULL,  // prefix
                   false);
        if (traceFileFb > 0)
        {
            trace_set_mem(traceFileFb);
        }
    }
}

int main (int argc, char *argv[])
{
    const char method_name[] = "main";

    bool done = false;
    unsigned int initSleepTime = 1; // 1 second

    CALL_COMP_DOVERS(pstartd, argc, argv);
    CALL_COMP_PRINTVERS(pstartd)

    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    // Initialize MPI environment
    MPI_Init (&argc, &argv);

    TraceInit ( argc, argv );

    // Mask all allowed signals except SIGPROF
    sigset_t    mask;
    sigfillset( &mask);
    sigdelset( &mask, SIGPROF ); // allows profiling such as google profiler

    int rc = pthread_sigmask( SIG_SETMASK, &mask, NULL );
    if ( rc != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf( buf, "[%s - main], pthread_sigmask error=%d\n", MyName, rc );
        monproc_log_write( PSTARTD_MAIN_1, SQ_LOG_ERR, buf );
    }

    // This process does not use MPI.  But unless MPI is initialized
    // printf does to route output correctly.

    monUtil.processArgs (argc, argv);
    MyName = monUtil.getProcName();
    gv_ms_su_nid = MyPNID = monUtil.getPNid();
    MyZid = monUtil.getZid();
    MyNid = monUtil.getNid();
    MyPid = monUtil.getPid();

    MonLog = new CMonLog( "log4cxx.monitor.psd.config", "PSD", "alt.pstartd", MyPNID, MyNid, MyPid, MyName );

    pStartD = new CPStartD;

    if ( !pStartD->loadConfiguration() )
    {
        trace_printf("%s@%d Exiting!\n",  method_name, __LINE__);
        exit (1);
    }

    InitLocalIO( );

    gp_local_mon_io->set_cb(localIONoticeCallback, "notice");
    gp_local_mon_io->set_cb(localIOEventCallback, "event");
    gp_local_mon_io->set_cb(localIORecvCallback, "recv");

    monUtil.requestStartup ();

    // Debugging aid, set # seconds in mon.env
    char *env = getenv("PSD_INIT_SLEEP"); 
    if ( env && isdigit(*env) )
    {
        initSleepTime = atoi(env);
    }
    sleep( initSleepTime );

    CRequest *req;

    do
    {
        if ( tracing )
        {
            trace_printf("%s@%d waiting for event\n", method_name, __LINE__);
        }

        pStartD->waitForEvent();
        do
        {
            if ( tracing )
            {
                trace_printf("%s@%d Got event\n",  method_name, __LINE__);
            }
            req = pStartD->getReq();
            if (req)
            {
                req->performRequest();
                delete req;
            }
        }
        while( pStartD->getReqCount() );
    }
    while ( !done && !shuttingDown );

    trace_printf("%s@%d Exiting!\n",  method_name, __LINE__);

    monUtil.requestExit ();
}
