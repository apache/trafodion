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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <iostream>
#include "/usr/include/linux/watchdog.h"
#include "redirector.h"

using namespace std;
#include "msgdef.h"
#include "internal.h"
#include "monlogging.h"
#include "monsonar.h"
#include "montrace.h"
#include "monitor.h"
#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "mlio.h"
#include "nameserver.h"

extern bool IsRealCluster;
extern CommType_t CommType;
extern CNodeContainer *Nodes;
extern CNode *MyNode;
extern CMonitor *Monitor;
extern CMonStats *MonStats;
extern bool usingCpuAffinity;
extern bool usingTseCpuAffinity;
#ifndef NAMESERVER_PROCESS
extern CNameServer *NameServer;
extern bool NameServerEnabled;
#endif

void CoreMaskString( char *str, cpu_set_t coreMask, int totalCores )
{
    str[totalCores] = '\0'; // trucate using total cores
    // Least significant on right
    for( int i = (totalCores-1); i >= 0; i--, str++ )
    {
        *str = CPU_ISSET( i, &coreMask ) ? '1' : '0';
    }
}

const char *RoleTypeString( ZoneType type )
{
    const char *str;

    switch( type )
    {
        case ZoneType_Edge:
            str = "connection";
            break;
        case ZoneType_Excluded:
            str = "excluded";
            break;
        case ZoneType_Aggregation:
            str = "aggregation";
            break;
        case ZoneType_Storage:
            str = "storage";
            break;
        case ZoneType_Frontend:
            str = "connection,aggregation";
            break;
        case ZoneType_Backend:
            str = "aggregation,storage";
            break;
        case ZoneType_Any:
            str = "connection,aggregation,storage";
            break;
        default:
            str = "Undefined";
            break;
    }

    return( str );
}

CLNode::CLNode( CLNodeContainer *lnodes
              , int              nid
              , cpu_set_t       &coreMask 
              , int              processors
              , ZoneType         zoneType
              )
       :CProcessContainer()
       ,Nid(nid)
       ,CoreMask(coreMask)
       ,NumProcessors(processors)
       ,NodeZoneType(zoneType)
       ,ChangeState( false )
       ,tseCnt_(0)
       ,tseBackupCnt_(0)
       ,lnodes_(lnodes)
       ,cpuUser_(0)
       ,cpuNice_(0)
       ,cpuSystem_(0)
       ,cpuIdle_(0)
       ,cpuIowait_(0)
       ,cpuIrq_(0)
       ,cpuSoftIrq_(0)
       ,numCores_(0)
       ,firstCore_(-1)
       ,lastTseCoreAssigned_(-1)
       ,lastBackupTseCoreAssigned_(-1)
       ,next_(NULL)
       ,prev_(NULL)
       ,nextP_(NULL)
       ,prevP_(NULL)
       ,SSMProc(NULL)
{
    const char method_name[] = "CLNode::CLNode";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "LNOD", 4);

    // Set  numCores_ firstCore_ based on coreMask.  These are used
    // when retrieving processor statistics.
    for (int i = 0; i < MAX_CORES; i++ )
    {
        if ( CPU_ISSET( i, &coreMask ) )
        {
            if (firstCore_ == -1)
            {
                firstCore_ = i;
            }
            ++numCores_;
        }
    }

    if (firstCore_ == -1)
    {   // Unexpectedly, mask does not indicate any processors for this
        // logical node.  Set default values.
        firstCore_ = 0;
        numCores_ = 1;
    }

    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
    {   // Display pidMap location, useful for understanding later trace output
        trace_printf("%s@%d Nid %d, pidMap_ (%p)\n",
                     method_name, __LINE__, Nid, GetPidMap());
    }

    TRACE_EXIT;
}

CLNode::~CLNode (void)
{
    const char method_name[] = "CLNode::~CLNode";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d nid=%d\n", method_name, __LINE__, Nid );
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "lnod", 4);

    TRACE_EXIT;
}

void CLNode::DeLink (CLNode **head, CLNode **tail)
{
    const char method_name[] = "CLNode::DeLink";
    TRACE_ENTRY;

    if (*head == this)
        *head = next_;
    if (*tail == this)
        *tail = prev_;
    if (prev_)
        prev_->next_ = next_;
    if (next_)
        next_->prev_ = prev_;

    TRACE_EXIT;
}

void CLNode::DeLinkP(CLNode **head, CLNode **tail)
{
    const char method_name[] = "CLNode::DeLinkP";
    TRACE_ENTRY;

    if (*head == this)
        *head = nextP_;
    if (*tail == this)
        *tail = prevP_;
    if (prevP_)
        prevP_->nextP_ = nextP_;
    if (nextP_)
        nextP_->prevP_ = prevP_;

    TRACE_EXIT;
}

void CLNode::Added( void )
{
    const char method_name[] = "CLNode::Added";
    TRACE_ENTRY;

#ifndef NAMESERVER_PROCESS
    struct  message_def *msg;
    if ( MyNode->GetState() == State_Up )
    {
        // send node added message to local node's processes
        msg = new struct message_def;
        msg->type = MsgType_NodeAdded;
        msg->noreply = true;
        msg->u.request.type = ReqType_Notice;
        msg->u.request.u.node_added.nid = Nid;
        msg->u.request.u.node_added.zid = GetNode()->GetZone();
        const char *nodeName = GetNode()->GetName();
        if (IsRealCluster)
        {
            nodeName = GetNode()->GetFqdn();
            STRCPY(msg->u.request.u.node_added.node_name, nodeName);
        }
        else
        {
            sprintf(msg->u.request.u.node_added.node_name,"%s:%d", nodeName, Nid);
        }
        
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf( "%s@%d - Broadcasting node added nid=%d, zid=%d, name=(%s)\n"
                        , method_name, __LINE__
                        , msg->u.request.u.node_added.nid
                        , msg->u.request.u.node_added.zid
                        , msg->u.request.u.node_added.node_name );
        }

        MyNode->Bcast( msg );
        delete msg;
    }
#endif

    TRACE_EXIT;
}

void CLNode::Changed( CLNodeConfig *lnodeConfig )
{
    const char method_name[] = "CLNode::Changed";
    TRACE_ENTRY;

#ifdef NAMESERVER_PROCESS
    lnodeConfig = lnodeConfig; // touch
#else
    struct  message_def *msg;
    if ( MyNode->GetState() == State_Up )
    {
        // send node changed message to local node's processes
        msg = new struct message_def;
        msg->type = MsgType_NodeChanged;
        msg->noreply = true;
        msg->u.request.type = ReqType_Notice;
        msg->u.request.u.node_changed.nid = Nid;
        msg->u.request.u.node_changed.pnid = GetNode()->GetPNid();
        msg->u.request.u.node_changed.zid = GetNode()->GetZone();
        msg->u.request.u.node_changed.first_core = lnodeConfig->GetFirstCore();
        msg->u.request.u.node_changed.last_core  = lnodeConfig->GetLastCore();
        msg->u.request.u.node_changed.processors = lnodeConfig->GetProcessors();
        msg->u.request.u.node_changed.roles      = static_cast<int>(lnodeConfig->GetZoneType());
        const char *nodeName = GetNode()->GetName();
        if (IsRealCluster)
        {
            nodeName = GetNode()->GetFqdn();
            STRCPY(msg->u.request.u.node_changed.node_name, nodeName);
        }
        else
        {
            sprintf(msg->u.request.u.node_changed.node_name,"%s:%d", nodeName, Nid);
        }
        
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf( "%s@%d - Broadcasting node changed nid=%d, zid=%d, "
                          "pnid=%d, name=(%s), cores=%d:%d, "
                          "processors=%d, roles=(%s)\n"
                        , method_name, __LINE__
                        , msg->u.request.u.node_changed.nid
                        , msg->u.request.u.node_changed.zid
                        , msg->u.request.u.node_changed.pnid
                        , msg->u.request.u.node_changed.node_name
                        , msg->u.request.u.node_changed.first_core
                        , msg->u.request.u.node_changed.last_core
                        , msg->u.request.u.node_changed.processors
                        , RoleTypeString(static_cast<ZoneType>(msg->u.request.u.node_changed.roles)) );
        }

        MyNode->Bcast( msg );
        delete msg;
    }
#endif

    TRACE_EXIT;
}

void CLNode::Deleted( void )
{
    const char method_name[] = "CLNode::Deleted";
    TRACE_ENTRY;

#ifndef NAMESERVER_PROCESS
    struct  message_def *msg;
    if ( MyNode->GetState() == State_Up )
    {
        // send node added message to local node's processes
        msg = new struct message_def;
        msg->type = MsgType_NodeDeleted;
        msg->noreply = true;
        msg->u.request.type = ReqType_Notice;
        msg->u.request.u.node_deleted.nid = Nid;
        msg->u.request.u.node_deleted.zid = GetNode()->GetZone();
        const char *nodeName = GetNode()->GetName();
        if (IsRealCluster)
        {
            nodeName = GetNode()->GetFqdn();
            STRCPY(msg->u.request.u.node_deleted.node_name, nodeName);
        }
        else
        {
            sprintf(msg->u.request.u.node_deleted.node_name,"%s:%d", nodeName, Nid);
        }
        
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf( "%s@%d - Broadcasting node deleted nid=%d, zid=%d, name=(%s)\n"
                        , method_name, __LINE__
                        , msg->u.request.u.node_deleted.nid
                        , msg->u.request.u.node_deleted.zid
                        , msg->u.request.u.node_deleted.node_name );
        }

        MyNode->Bcast( msg );
        delete msg;
    }
#endif

    TRACE_EXIT;
}

void CLNode::Down( void )
{
    const char method_name[] = "CLNode::Down";
    TRACE_ENTRY;

#ifndef NAMESERVER_PROCESS
    struct  message_def *msg;
    if ( MyNode->GetState() == State_Up )
    {
        // Record statistics (sonar counters)
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->notice_node_down_Incr();
    
        // send node down message to local node's processes
        msg = new struct message_def;
        msg->type = MsgType_NodeDown;
        msg->noreply = true;
        msg->u.request.type = ReqType_Notice;
        msg->u.request.u.down.nid = Nid;
        msg->u.request.u.down.takeover = GetNode()->IsActivatingSpare();
#ifdef USE_SEQUENCE_NUM
        msg->u.request.u.down.seqnum = Monitor->GetTimeSeqNum();
#endif
        const char * nodeName = GetNode()->GetName();
        if (IsRealCluster)
        {
            nodeName = GetNode()->GetFqdn();
            STRCPY(msg->u.request.u.down.node_name, nodeName);
        }
        else
        {
            sprintf(msg->u.request.u.down.node_name,"%s:%d", nodeName, Nid);
        }
        
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Broadcasting Node Down nid=%d, name=(%s), takeover=%d\n"
                        , method_name, __LINE__, GetNid()
                        , GetNode()->GetName(), msg->u.request.u.down.takeover );
        }
#ifndef NAMESERVER_PROCESS
        if ( NameServerEnabled )
        {
            NameServer->ProcessNodeDown( Nid, msg->u.request.u.down.node_name );
        }
#endif
        MyNode->Bcast( msg );
        delete msg;
    }
#endif

    TRACE_EXIT;
}

CProcess *CLNode::GetProcessL(int pid)
{
    CProcess *entry;
    const char method_name[] = "CLNode::GetProcessL";
    TRACE_ENTRY;

    // Temporary trace
    if (trace_settings & TRACE_PROCESS_DETAIL)
        trace_printf("%s@%d - pid %d\n",
                     method_name, __LINE__, pid);

    entry = GetNode()->GetProcess(pid);

    TRACE_EXIT;
    return entry;
}

CProcess *CLNode::GetProcessL(char *name, bool checkstate)
{
    CProcess *entry = NULL;
    const char method_name[] = "CLNode::GetProcessL";
    TRACE_ENTRY;

    // Temporary trace
    if (trace_settings & TRACE_PROCESS_DETAIL)
        trace_printf("%s@%d - name=%s, checkstate=%d\n",
                     method_name, __LINE__, name, checkstate);

    entry = GetNode()->GetProcess(name, checkstate);

    TRACE_EXIT;
    return entry;
}

CProcess *CLNode::GetProcessL( int pid
                             , Verifier_t verifier
                             , bool checkstate )
{
    CProcess *entry = NULL;
    const char method_name[] = "CLNode::GetProcessL(pid,verifier)";
    TRACE_ENTRY;

    // Temporary trace
    if (trace_settings & TRACE_PROCESS_DETAIL)
        trace_printf("%s@%d - nid, %d, pid=%d, verifier=%d, checkstate=%d\n",
                     method_name, __LINE__, GetNid(), pid, verifier, checkstate);

    entry = GetNode()->GetProcess( pid, verifier, checkstate );

    // Temporary trace
    if (trace_settings & TRACE_PROCESS_DETAIL)
        trace_printf("%s@%d - entry=%p, pid=%d, Name=%s\n",
                     method_name, __LINE__, entry, pid,
                     ((entry != NULL) ? entry->GetName(): ""));

    TRACE_EXIT;
    return entry;
}

CProcess *CLNode::GetProcessL( const char *name
                             , Verifier_t verifier
                             , bool checkstate )
{
    CProcess *entry = NULL;
    const char method_name[] = "CLNode::GetProcessL(name,verifier)";
    TRACE_ENTRY;

    // Temporary trace
    if (trace_settings & TRACE_PROCESS_DETAIL)
        trace_printf("%s@%d - name=%s, verifier=%d, checkstate=%d\n",
                      method_name, __LINE__, name, verifier, checkstate);

    entry = GetNode()->GetProcess( name, verifier, checkstate );

    // Temporary trace
    if (trace_settings & TRACE_PROCESS_DETAIL)
        trace_printf("%s@%d - entry=%p, Name=%s\n",
                     method_name, __LINE__, entry,
                     ((entry != NULL) ? entry->GetName(): ""));

    TRACE_EXIT;
    return entry;
}

CProcess *CLNode::CompleteProcessStartup( char *process_name, 
                                          char *port, 
                                          int os_pid, 
                                          bool event_messages,
                                          bool system_messages,
                                          struct timespec *creation_time,
                                          int origPNidNs )
{
    CProcess *entry = NULL;
    const char method_name[] = "CLNode::CompleteProcessStartup";
    TRACE_ENTRY;

    entry = GetNode()->CompleteProcessStartup(process_name, 
                                              port,
                                              os_pid, 
                                              event_messages,
                                              system_messages,
                                              creation_time,
                                              origPNidNs);
    TRACE_EXIT;
    return entry;
}


CNode *CLNode::GetNode( void )
{
    const char method_name[] = "CLNode::GetNode";
    TRACE_ENTRY;

    TRACE_EXIT;
    return( lnodes_->GetNode() );
}

STATE CLNode::GetState( void ) 
{ 
    CNode *node = lnodes_->GetNode();
    const char method_name[] = "CLNode::GetState";
    TRACE_ENTRY;

    TRACE_EXIT;
    return( node ? node->GetState() : State_Unknown ); 
}

int CLNode::GetZone( void )
{ 
    const char method_name[] = "CLNode::GetZone";
    TRACE_ENTRY;

    TRACE_EXIT;
    return( lnodes_->GetNode()->GetZone() ); 
}

bool CLNode::IsKillingNode( void )
{
    const char method_name[] = "CLNode::IsKillingNode";
    TRACE_ENTRY;

    TRACE_EXIT;
    return( lnodes_->GetNode()->IsKillingNode() );
}

CLNode *CLNode::LinkAfter( CLNode * &tail, CLNode * entry )
{
    const char method_name[] = "CLNode::LinkAfter";
    TRACE_ENTRY;

    entry->prev_ = this;
    if (next_ == NULL)
    {
        entry->next_ = NULL;
        tail = entry;
    }
    else
    {
        entry->next_ = next_;
        next_->prev_ = entry;
    }
    next_ = entry;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Linked logical node object "
                      "tail=%d\n"
                      "\t\tthis: prev=%d, this=%d, next=%d\n"
                      "\t\tentry: prev=%d, entry=%d, next=%d\n"
                    , method_name, __LINE__
                    , tail->GetNid()
                    , prev_?prev_->GetNid():-1
                    , GetNid()
                    , next_?next_->GetNid():-1
                    , entry->prev_?entry->prev_->GetNid():-1
                    , entry->GetNid()
                    , entry->next_?entry->next_->GetNid():-1 );
    }
    
    TRACE_EXIT;
    return entry;
}

CLNode *CLNode::LinkBefore( CLNode * &head, CLNode * entry )
{
    const char method_name[] = "CLNode::LinkBefore";
    TRACE_ENTRY;

    entry->next_ = this;
    if (prev_ == NULL)
    {
        entry->prev_ = NULL;
        head = entry;
    }
    else
    {
        entry->prev_ = prev_;
        prev_->next_ = entry;
    }
    prev_ = entry;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Linked logical node object "
                      "head=%d\n"
                      "\t\tthis: prev=%d, this=%d, next=%d\n"
                      "\t\tentry: prev=%d, entry=%d, next=%d\n"
                    , method_name, __LINE__
                    , head->GetNid()
                    , prev_?prev_->GetNid():-1
                    , GetNid()
                    , next_?next_->GetNid():-1
                    , entry->prev_?entry->prev_->GetNid():-1
                    , entry->GetNid()
                    , entry->next_?entry->next_->GetNid():-1 );
    }
    
    TRACE_EXIT;
    return entry;
}

CLNode *CLNode::LinkAfterP( CLNode * &tail, CLNode * entry )
{
    const char method_name[] = "CLNode::LinkAfterP";
    TRACE_ENTRY;

    entry->prevP_ = this;
    if (nextP_ == NULL)
    {
        entry->nextP_ = NULL;
        tail = entry;
    }
    else
    {
        entry->nextP_ = nextP_;
        nextP_->prevP_ = entry;
    }
    nextP_ = entry;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Linked logical node object "
                      "tail=%d\n"
                      "\t\tthis: prev=%d, this=%d, next=%d\n"
                      "\t\tentry: prev=%d, entry=%d, next=%d\n"
                    , method_name, __LINE__
                    , tail->GetNid()
                    , prevP_?prevP_->GetNid():-1
                    , GetNid()
                    , nextP_?nextP_->GetNid():-1
                    , entry->prevP_?entry->prevP_->GetNid():-1
                    , entry->GetNid()
                    , entry->nextP_?entry->nextP_->GetNid():-1 );
    }
    
    TRACE_EXIT;
    return entry;
}

CLNode *CLNode::LinkBeforeP( CLNode * &head, CLNode * entry )
{
    const char method_name[] = "CLNode::LinkBeforeP";
    TRACE_ENTRY;

    entry->nextP_ = this;
    if (prevP_ == NULL)
    {
        entry->prevP_ = NULL;
        head = entry;
    }
    else
    {
        entry->prevP_ = prevP_;
        prevP_->nextP_ = entry;
    }
    prevP_ = entry;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Linked logical node object "
                      "head=%d\n"
                      "\t\tthis: prev=%d, this=%d, next=%d\n"
                      "\t\tentry: prev=%d, entry=%d, next=%d\n"
                    , method_name, __LINE__
                    , head->GetNid()
                    , prevP_?prevP_->GetNid():-1
                    , GetNid()
                    , nextP_?nextP_->GetNid():-1
                    , entry->prevP_?entry->prevP_->GetNid():-1
                    , entry->GetNid()
                    , entry->nextP_?entry->nextP_->GetNid():-1 );
    }
    
    TRACE_EXIT;
    return entry;
}

#ifndef NAMESERVER_PROCESS
void CLNode::SetAffinity( pid_t pid, PROCESSTYPE type )
{
    int rc = 0;
    cpu_set_t mask;
    CProcess *process = NULL;

    int tse_1st = 0;        // affinity mask to put tse in cpu 0
    int tse_2nd = 1;        // 2nd tse in cpu 1
    int tse_3rd = 2;        // 3rd in cpu 2
    int tse_4th = 3;        // 4th in cpu 3

    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CLNode::SetAffinity";
    TRACE_ENTRY;

    process = GetProcessL( pid );
    if ( usingTseCpuAffinity && type == ProcessType_TSE
      && process != NULL && (!process->IsBackup()) )
    {
        switch ( tseCnt_ )
        {

        case 0:
            tseCnt_++;
            CPU_ZERO( &mask );
            CPU_SET( tse_1st, &mask );
            break;

        case 1:
            tseCnt_++;
            CPU_ZERO( &mask );
            CPU_SET( tse_2nd, &mask );
            break;

        case 2:
            tseCnt_++;
            CPU_ZERO( &mask );
            CPU_SET( tse_3rd, &mask );
            break;

        case 3:
            tseCnt_=0;
            CPU_ZERO( &mask );
            CPU_SET( tse_4th, &mask );
            break;


        } //end of switch

        rc = sched_setaffinity( pid, sizeof(cpu_set_t), &mask );

    } // end of type == tse
    else
    {
        // Let it float
        CPU_ZERO( &mask );
        for ( int i=0; i<lnodes_->GetNode()->GetNumCores(); i++ )
        {
            CPU_SET( i, &mask );
        }
        rc = sched_setaffinity( pid, sizeof(cpu_set_t), &mask );
    }

    if ( rc )
    {
        sprintf( la_buf, "[CLNode::SetAffinity], Can't set processor affinity.\n" );
        mon_log_write( MON_LNODE_SETAFFINITY_1, SQ_LOG_ERR, la_buf );
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CLNode::SetAffinity( CProcess *process )
{
    int rc = 0;
    char coreMaskStr[MAX_CORES+1];
    cpu_set_t mask;
    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CLNode::SetAffinity";
    TRACE_ENTRY;

    if ( usingTseCpuAffinity && process->GetType( ) == ProcessType_TSE )
    {
        // round-robin the TSE affinity within the logical node mask
        if ( !process->IsBackup() )
        {
            if ( lastTseCoreAssigned_ == -1 )
            {
                // always start with the first core and count it
                lastTseCoreAssigned_ = firstCore_;
                ++tseCnt_;
            }
            else
            {
                // check for wrap around time
                if ( tseCnt_ == numCores_ )
                {
                    tseCnt_ =  1;
                    lastTseCoreAssigned_ = firstCore_;
                }
                else
                {
                    ++tseCnt_;
                    ++lastTseCoreAssigned_;
                }
            }
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d - TSE process %s%s (%d, %d) affinity set to core=%d, tseCount=%d, numCores=%d." "\n"
                             , method_name, __LINE__, process->GetName(), process->IsBackup() ? "-B" : "-P"
                             , process->GetNid(), process->GetPid(), lastTseCoreAssigned_, tseCnt_, numCores_ );
            
            }
            CPU_ZERO( &mask );
            CPU_SET( lastTseCoreAssigned_, &mask );
        }
        else
        {
            if ( lastBackupTseCoreAssigned_ == -1 )
            {
                // always start with the first core and count it
                lastBackupTseCoreAssigned_ = firstCore_;
                ++tseBackupCnt_;
            }
            else
            {
                // check for wrap around time
                if ( tseBackupCnt_ == numCores_ )
                {
                    tseBackupCnt_ =  1;
                    lastBackupTseCoreAssigned_ = firstCore_;
                }
                else
                {
                    ++tseBackupCnt_;
                    ++lastBackupTseCoreAssigned_;
                }
            }
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d - TSE process %s%s (%d, %d) affinity set to core=%d, tseCount=%d, numCores=%d." "\n"
                             , method_name, __LINE__, process->GetName(), process->IsBackup() ? "-B" : "-P"
                             , process->GetNid(), process->GetPid(), lastBackupTseCoreAssigned_, tseBackupCnt_, numCores_ );
            
            }
            CPU_ZERO( &mask );
            CPU_SET( lastBackupTseCoreAssigned_, &mask );
        }

        rc = sched_setaffinity( process->GetPid(), sizeof(cpu_set_t), &mask );
    }
    else if ( CommType == CommType_InfiniBand &&
              process->GetType( ) == ProcessType_Generic &&
              process->isCmpOrEsp( ) )
    {
        CPU_ZERO( &mask );
        short lv_corenum = 0;
        do
        {
            CPU_SET( lv_corenum, &mask );
            lv_corenum++;
        }
        while ( lv_corenum < (GetNode( )->GetNumCores( ) - 1) );
        if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
        {
            CoreMaskString( coreMaskStr, mask, GetNode( )->GetNumCores( ) );
            trace_printf( "%s@%d - Generic process %s (%d, %d), cores=%d, affinity set to mask=%s" "\n"
                , method_name, __LINE__, process->GetName( )
                , process->GetNid( ), process->GetPid( ), GetNode( )->GetNumCores( ), coreMaskStr );

        }
        rc = sched_setaffinity( process->GetPid( ), sizeof(cpu_set_t), &mask );
    }
    else if ( usingCpuAffinity )
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            CoreMaskString( coreMaskStr, CoreMask, GetNode()->GetNumCores() );
            trace_printf("%s@%d - process %s (%d, %d), cores=%d, affinity set to mask=%s" "\n"
                         , method_name, __LINE__, process->GetName()
                         , process->GetNid(), process->GetPid(), GetNode()->GetNumCores(), coreMaskStr );
            
        }
        // Use the configured cores
        rc = sched_setaffinity( process->GetPid(), sizeof(cpu_set_t), &CoreMask );
    }
    else
    {
        // Let it float in the physical node
        mask = GetNode( )->GetAffinityMask( );
        if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
        {
            CoreMaskString( coreMaskStr, mask, GetNode( )->GetNumCores( ) );
            trace_printf( "%s@%d - process %s (%d, %d), cores=%d, affinity set to mask=%s" "\n"
                , method_name, __LINE__, process->GetName( )
                , process->GetNid( ), process->GetPid( ), GetNode( )->GetNumCores( ), coreMaskStr );
        }
        rc = sched_setaffinity( process->GetPid( ), sizeof(cpu_set_t), &mask );
    }

    if ( rc )
    {
        sprintf( la_buf, "[CLNode::SetAffinity], Can't set processor affinity.\n" );
        mon_log_write( MON_LNODE_SETAFFINITY_2, SQ_LOG_ERR, la_buf );
    }

    TRACE_EXIT;
}
#endif

void CLNode::Up( void )
{
    const char method_name[] = "CLNode::Up";
    TRACE_ENTRY;

#ifndef NAMESERVER_PROCESS
    struct  message_def *msg;
    char    la_buf[MON_STRING_BUF_SIZE];
    sprintf(la_buf, "[CLNode::Up], Node %d (%s) is up.\n", GetNid(), GetNode()->GetName());
    mon_log_write(MON_LNODE_MARKUP, SQ_LOG_INFO, la_buf); 

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->notice_node_up_Incr();

    // send node up message to our node's processes
    msg = new struct message_def;
    msg->type = MsgType_NodeUp;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    msg->u.request.u.up.nid = Nid;
    msg->u.request.u.up.takeover = GetNode()->IsActivatingSpare();
#ifdef USE_SEQUENCE_NUM
    msg->u.request.u.up.seqnum = Monitor->GetTimeSeqNum();
#endif
    const char * nodeName = GetNode()->GetName();
    if (IsRealCluster)
    {
        nodeName = GetNode()->GetFqdn();
        STRCPY(msg->u.request.u.up.node_name, nodeName);
    }
    else
    {
        sprintf(msg->u.request.u.up.node_name,"%s:%d", nodeName, Nid);
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Broadcasting Node Up nid=%d, name=(%s), takeover=%d\n"
                    , method_name, __LINE__, GetNid()
                    , GetNode()->GetName(), msg->u.request.u.up.takeover );
    }
    
    MyNode->Bcast( msg );
    delete msg;
#endif

    TRACE_EXIT;
}

CLNodeContainer::CLNodeContainer(CNode *node)
                :LNode(NULL)
                ,LastNid(0)
                ,lnodesCount_(0)
                ,indexToNid_(NULL)
                ,node_(node)
                ,head_(NULL)
                ,tail_(NULL)
{
    const char method_name[] = "CLNodeContainer::CLNodeContainer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "LCTR", 4);

    TRACE_EXIT;
}

CLNodeContainer::~CLNodeContainer (void)
{
    const char method_name[] = "CLNodeContainer::~CLNodeContainer";
    TRACE_ENTRY;

    if (node_ == NULL)
    {   // In the main logical nodes container
        CLNode *lnode = head_;
        while (head_)
        {
            lnode->DeLink(&head_, &tail_);
            delete lnode;
    
            lnode = head_;
        }
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "lctr", 4);

    TRACE_EXIT;
}

CLNode *CLNodeContainer::AddLNode( CLNodeConfig *lnodeConfig, CNode *node )
{
    const char method_name[] = "CLNodeContainer::AddLNode";
    TRACE_ENTRY;

    assert( lnodeConfig != NULL );

    CLNode *lnode = new CLNode( node->GetLNodeContainer()
                              , lnodeConfig->GetNid()
                              , lnodeConfig->GetCoreMask()
                              , lnodeConfig->GetProcessors()
                              , lnodeConfig->GetZoneType()
                              );
    assert( lnode != NULL );
    
    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d - Adding logical node object "
                      "(nid=%d) to lnodes container, "
                      "lnodesCount=%d\n"
                    , method_name, __LINE__
                    , lnode->GetNid()
                    , lnodesCount_ );
    }

    lnodesCount_++;
    if (head_ == NULL)
    {
        head_ = tail_ = lnode;
    }
    else
    {
        // add to list in nid sort order
        if (lnode->GetNid() < head_->GetNid())
        { // link new lnode to the begining
            head_->LinkBefore( head_, lnode );
        }
        else if (lnode->GetNid() > tail_->GetNid())
        { // link new lnode to the end
            tail_->LinkAfter( tail_, lnode );
        }
        else
        {
            CLNode *entry = head_;
            CLNode *prevEntry = NULL;
            while (entry)
            { // walk the list
                if (lnode->GetNid() > entry->GetNid())
                { // new lnode is greater than current list entry
                    prevEntry = entry;
                    entry = prevEntry->GetNext();
                }
                else
                { // new lnode is less than current list entry
                    prevEntry->LinkAfter( tail_, lnode );
                    entry = NULL;
                }
            }
        }
    }

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d - Added logical node object "
                      "(nid=%d) to lnodes container, "
                      "lnodesCount=%d\n"
                    , method_name, __LINE__
                    , lnode->GetNid()
                    , lnodesCount_ );
    }

    TRACE_EXIT;
    return lnode;
}

void CLNodeContainer::AddLNodeP( CLNode *lnode )
{
    const char method_name[] = "CLNodeContainer::AddLNodeP";
    TRACE_ENTRY;

    if (!node_)
    {
        // Must only be called from physical node's logical node container
        abort(); 
    }

    assert( lnode != NULL );
    
    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d - Adding logical node object "
                      "(nid=%d) to (pnid=%d) lnodes container, "
                      "lnodesCount=%d\n"
                    , method_name, __LINE__
                    , lnode->GetNid()
                    , lnode->GetLNodeContainer()->GetNode()->GetPNid()
                    , lnodesCount_ );
    }

    lnodesCount_++;
    if (head_ == NULL)
    {
        head_ = tail_ = lnode;
    }
    else
    {
        // add to list in nid sort order
        if (lnode->GetNid() < head_->GetNid())
        { // link new lnode to the begining
            head_->LinkBeforeP( head_, lnode );
        }
        else if (lnode->GetNid() > tail_->GetNid())
        { // link new lnode to the end
            tail_->LinkAfterP( tail_, lnode );
        }
        else
        {
            CLNode *entry = head_;
            CLNode *prevEntry = NULL;
            while (entry)
            { // walk the list
                if (lnode->GetNid() > entry->GetNid())
                { // new lnode is greater than current list entry
                    prevEntry = entry;
                    entry = prevEntry->GetNext();
                }
                else
                { // new lnode is less than current list entry
                    prevEntry->LinkAfterP( tail_, lnode );
                    entry = NULL;
                }
            }
        }
    }

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d - Added logical node object "
                      "(nid=%d) to (pnid=%d) lnodes container, "
                      "lnodesCount=%d\n"
                    , method_name, __LINE__
                    , lnode->GetNid()
                    , lnode->GetLNodeContainer()->GetNode()->GetPNid()
                    , lnodesCount_ );
    }

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
void CLNodeContainer::CancelDeathNotification( int nid
                                             , int pid
                                             , int verifier
                                             , _TM_Txid_External trans_id )
{
    CLNode *lnode;
    
    const char method_name[] = "CLNodeContainer::CancelDeathNotification";
    TRACE_ENTRY;

    for ( lnode=head_; lnode; lnode=lnode->GetNextP() )
    {
        lnode->CancelDeathNotification( nid, pid, verifier, trans_id);
    }

    TRACE_EXIT;
}
#endif
   
void CLNodeContainer::CheckForPendingCreates ( CProcess *process )
{
    process = process;
    //TODO
    abort();
}

void CLNodeContainer::DeleteLNode( CLNode *lnode )
{
    const char method_name[] = "CLNodeContainer::DeleteLNode";
    TRACE_ENTRY;

    int nid = lnode->GetNid();

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d Deleting nid=%d)\n", method_name, __LINE__, nid );
    }

    if (node_)
    {
        // Must only be called from main (cluster's) logical node container
        abort(); 
    }

    // delete logical node and remove from logical nodes array
    lnode->DeLink(&head_, &tail_);
    delete lnode;
    lnodesCount_--;

    TRACE_EXIT;
}

CLNode *CLNodeContainer::GetLNode(int nid)
{
    const char method_name[] = "CLNodeContainer::GetLNode";
    TRACE_ENTRY;

    CLNode *lnode = head_;

    while (lnode)
    {
        if ( lnode->GetNid() == nid )
        { 
            break;
        }
        lnode = lnode->GetNext();
    }

    TRACE_EXIT;
    return lnode;
}

CLNode *CLNodeContainer::GetLNode( char *process_name, CProcess **process,
                                  bool checkstate, bool backupOk )
{
    CLNode *lnode = head_;
    CNode *node = lnode ? lnode->GetNode() : NULL;
    CProcess *p_process;
    CLNode *b_lnode = NULL;
    CProcess *b_process = NULL;
    const char method_name[] = "CLNodeContainer::GetLNode";
    TRACE_ENTRY;

    // Initialize return value
    *process = NULL;

    while (node)
    {
        if ( !node->IsSpareNode() && 
             (node->GetState() == State_Up ||
              node->GetState() == State_Shutdown) )
        {
            *process = node->CProcessContainer::GetProcess(process_name, checkstate);
            if (*process)
            { 
                p_process = *process;
                if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    trace_printf("%s@%d - process %s (%d, %d), backup=%d, backupOk=%d\n",
                                 method_name, __LINE__,
                                 p_process->GetName(), p_process->GetNid(),
                                 p_process->GetPid(),  p_process->IsBackup(),
                                 backupOk);
                if (!p_process->IsBackup())
                {
                    lnode = LNode[p_process->GetNid()];
                    break;
                }
                else
                {
                    // Save backup process and lnode
                    b_process = *process;
                    b_lnode = LNode[b_process->GetNid()];
                }
            }
        }
        lnode = lnode->GetNext ();
        node = lnode ? lnode->GetNode() : NULL;
    }

    if ( !*process && backupOk )
    {
        // We did not find the primary and it's ok to return the backup
        *process = b_process;
        lnode = b_lnode;
    }

    TRACE_EXIT;
    return lnode;
}

CLNode *CLNodeContainer::GetLNodeByMap(int index )
{
    const char method_name[] = "CNodeContainer::GetLNodeByMap";
    TRACE_ENTRY;

    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    CLNode *lnode = NULL;
    
    if( index >= 0 && index < clusterConfig->GetLNodesCount() )
    {
        lnode = LNode[indexToNid_[index]];
    }

    TRACE_EXIT;
    return lnode;
}

int CLNodeContainer::GetNidIndex( int nid )
{
    const char method_name[] = "CNodeContainer::GetNidIndex";
    TRACE_ENTRY;

    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();

    for (int i = 0; i <  clusterConfig->GetLNodesCount(); i++ )
    {
        if (indexToNid_[i] == nid)
        {
            return(i);
        }
    }

    TRACE_EXIT;
    return(-1);
}

CLNode *CLNodeContainer::GetLNodeNext( int nid, bool checkstate )
{
    const char method_name[] = "CLNodeContainer::GetLNodeNext";
    TRACE_ENTRY;

    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    CLNode *lnode = NULL;

    for (int i = (nid+1); i <  clusterConfig->GetLNodesCount(); i++ )
    {
        lnode = LNode[i];
        if ( lnode )
        {
            if ( lnode->GetNid() > nid )
            {
                if (checkstate && lnode->GetState() == State_Up)
                {
                    break; // found it
                }
                else
                {
                    break; // found it
                }
            }
        }
    }
    
    if ( lnode == NULL )
    {
        for (int i = 0; i < clusterConfig->GetLNodesCount(); i++ )
        {
            lnode = LNode[i];
            if ( lnode )
            {
                if ( lnode->GetNid() <= nid )
                {
                    if (checkstate && lnode->GetState() == State_Up)
                    {
                        break; // found it
                    }
                    else
                    {
                        break; // found it
                    }
                }
            }
        }
    }

    TRACE_EXIT;
    return lnode;
}

bool CLNodeContainer::IsMyNode( int nid )
{
    bool found = false;
    CLNode *lnode;
    const char method_name[] = "CLNodeContainer::IsMyNode";
    TRACE_ENTRY;
    
    for ( lnode = head_; lnode; lnode = lnode->GetNextP() )
    {
        if ( lnode->Nid == nid )
        {
            found = true;
            break;
        }
    }

    TRACE_EXIT;
    return found;
}

void CLNodeContainer::RemoveLNodeP( CLNode *lnode )
{
    lnode->DeLinkP(&head_, &tail_);
}

