///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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
#include "internal.h"
#include "monlogging.h"
#include "monsonar.h"
#include "montrace.h"
#include "monitor.h"
#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "mlio.h"

extern bool IsRealCluster;
extern CommType_t CommType;
extern CNodeContainer *Nodes;
extern CNode *MyNode;
extern CMonitor *Monitor;
extern CMonStats *MonStats;
extern bool usingCpuAffinity;
extern bool usingTseCpuAffinity;

void CoreMaskString( char *str, cpu_set_t coreMask, int totalCores )
{
    str[totalCores] = '\0'; // trucate using total cores
    // Least significant on right
    for( int i = (totalCores-1); i >= 0; i--, str++ )
    {
        *str = CPU_ISSET( i, &coreMask ) ? '1' : '0';
    }
}

CLNode::CLNode(CLNodeContainer  *lnodes
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

void CLNode::Down( void )
{
    struct  message_def *msg;
    
    const char method_name[] = "CLNode::Down";
    TRACE_ENTRY;


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
        
        MyNode->Bcast( msg );
        delete msg;
    }

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
                                          struct timespec *creation_time )
{
    CProcess *entry = NULL;
    const char method_name[] = "CLNode::CompleteProcessStartup";
    TRACE_ENTRY;

    entry = GetNode()->CompleteProcessStartup(process_name, 
                                              port,
                                              os_pid, 
                                              event_messages,
                                              system_messages,
                                              creation_time);
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

CLNode *CLNode::Link (CLNode * entry)
{
    const char method_name[] = "CLNode::Link";
    TRACE_ENTRY;

    next_ = entry;
    entry->prev_ = this;

    TRACE_EXIT;
    return entry;
}

void CLNode::PrepareForTransactions( bool activatingSpare )
{
    const char method_name[] = "CLNode::PrepareForTransactions";
    TRACE_ENTRY;

    struct  message_def *msg;

    if ( trace_settings & 
        (TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC | TRACE_INIT) )
    {
        trace_printf( "%s@%d -  %s (nid=%d, state=%d) sending prepare notice to DTM and SPX\n"
                    , method_name, __LINE__
                    , MyNode->GetName(), MyNode->GetPNid(), MyNode->GetState());
    }

    if ( MyNode->GetState() == State_Up )
    {
        CLNode *lnode = MyNode->GetFirstLNode();
        for ( ; lnode; lnode = lnode->GetNext() )
        {
            // Send local DTM processes a node prepare message for each
            // logical node activated by spare node
            CProcess   *process = lnode->GetProcessLByType( ProcessType_DTM );
            if ( process )
            {
                // Record statistics (sonar counters)
                if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                   MonStats->notice_node_up_Incr();
            
                // send node prepare notice to our node's DTM process
                msg = new struct message_def;
                msg->type = MsgType_NodePrepare;
                msg->noreply = true;
                msg->u.request.type = ReqType_Notice;
                msg->u.request.u.prepare.nid = Nid;
                msg->u.request.u.prepare.takeover = activatingSpare ? true : false;
                const char * nodeName = GetNode()->GetName();
                STRCPY(msg->u.request.u.prepare.node_name, nodeName);
                SQ_theLocalIOToClient->putOnNoticeQueue( process->GetPid()
                                                       , process->GetVerifier()
                                                       , msg
                                                       , NULL);
            
                if ( trace_settings & 
                    (TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC | TRACE_INIT) )
                {
                    trace_printf( "%s@%d - Sending node %d (takeover=%d) prepare notice to DTM %s (pid=%d)\n"
                                , method_name, __LINE__
                                , Nid , msg->u.request.u.prepare.takeover
                                , process->GetName(), process->GetPid() );
                                
                }
            }

            // Send local SPX processes a node prepare message for each
            // logical node activated by spare node
            process = lnode->GetProcessLByType( ProcessType_SPX );
            if ( process )
            {
                // Record statistics (sonar counters)
                if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
                   MonStats->notice_node_up_Incr();
            
                // send node prepare notice to our node's DTM process
                msg = new struct message_def;
                msg->type = MsgType_NodePrepare;
                msg->noreply = true;
                msg->u.request.type = ReqType_Notice;
                msg->u.request.u.prepare.nid = Nid;
                msg->u.request.u.prepare.takeover = activatingSpare ? true : false;
                const char * nodeName = GetNode()->GetName();
                STRCPY(msg->u.request.u.prepare.node_name, nodeName);
                SQ_theLocalIOToClient->putOnNoticeQueue( process->GetPid()
                                                       , process->GetVerifier()
                                                       , msg
                                                       , NULL);
            
                if ( trace_settings & 
                    (TRACE_RECOVERY | TRACE_REQUEST | TRACE_INIT) )
                {
                    trace_printf( "%s@%d - Sending node %d prepare notice to SPX %s (pid=%d)\n"
                                , method_name, __LINE__, Nid
                                , process->GetName(), process->GetPid());
                }
            }
        }
    }

    TRACE_EXIT;
}

void CLNode::SendDTMRestarted( void )
{
    const char method_name[] = "CLNode::SendDTMRestarted";
    TRACE_ENTRY;

    struct  message_def *msg;

    if ( trace_settings &
        (TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC | TRACE_INIT) )
    {
        trace_printf( "%s@%d -  %s (pnid=%d, state=%d) sending DTM restarted in nid=%d notice to local DTMs\n"
                    , method_name, __LINE__
                    , MyNode->GetName(), MyNode->GetPNid(), MyNode->GetState(), GetNid() );
    }

    if ( MyNode->GetState() == State_Up )
    {
        CLNode *lnode = MyNode->GetFirstLNode();
        for ( ; lnode; lnode = lnode->GetNext() )
        {
            // Send local DTM processes a DTM restarted message
            CProcess   *process = lnode->GetProcessLByType( ProcessType_DTM );
            if ( process )
            {
                // send node prepare notice to our node's DTM process
                msg = new struct message_def;
                msg->type = MsgType_TmRestarted;
                msg->noreply = true;
                msg->u.request.type = ReqType_Notice;
                msg->u.request.u.tm_restart.nid = Nid;
                msg->u.request.u.tm_restart.pnid = GetNode()->GetPNid();
                const char * nodeName = GetNode()->GetName();
                STRCPY(msg->u.request.u.tm_restart.node_name, nodeName);
                SQ_theLocalIOToClient->putOnNoticeQueue( process->GetPid()
                                                       , process->GetVerifier()
                                                       , msg
                                                       , NULL);

                if ( trace_settings &
                    (TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC | TRACE_INIT) )
                {
                    trace_printf( "%s@%d - Sending nid=%d DTM restarted notice to DTM %s (nid=%d,pid=%d)\n"
                                , method_name, __LINE__
                                , Nid , process->GetName(), process->GetNid(), process->GetPid() );
                }
            }
        }
    }

    TRACE_EXIT;
}

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

void CLNode::Up( void )
{
    struct  message_def *msg;
    char    la_buf[MON_STRING_BUF_SIZE];
    
    const char method_name[] = "CLNode::Up";
    TRACE_ENTRY;

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

    TRACE_EXIT;
}

CLNodeContainer::CLNodeContainer(CNode *node)
                :LastNid(0)
                ,node_(node)
                ,numLNodes_(0)
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
    CLNode *lnode = head_;

    const char method_name[] = "CLNodeContainer::~CLNodeContainer";
    TRACE_ENTRY;

    while (head_)
    {
        lnode->DeLink (&head_, &tail_);
        delete lnode;

        lnode = head_;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "lctr", 4);

    TRACE_EXIT;
}

CLNode *CLNodeContainer::AddLNode( CLNodeConfig   *lnodeConfig )
{
    const char method_name[] = "CLNodeContainer::AddLNode";
    TRACE_ENTRY;

    assert( lnodeConfig != NULL );

    CLNode *lnode = new CLNode( this
                              , lnodeConfig->GetNid()
                              , lnodeConfig->GetCoreMask()
                              , lnodeConfig->GetProcessors()
                              , lnodeConfig->GetZoneType()
                              );
    assert( lnode != NULL );
    
    numLNodes_++;
    if (head_ == NULL)
    {
        head_ = tail_ = lnode;
    }
    else
    {
        tail_ = tail_->Link (lnode);
    }

    TRACE_EXIT;
    return lnode;
}

void CLNodeContainer::CancelDeathNotification( int nid
                                             , int pid
                                             , int verifier
                                             , _TM_Txid_External trans_id )
{
    CLNode *lnode;
    
    const char method_name[] = "CLNodeContainer::CancelDeathNotification";
    TRACE_ENTRY;

    for ( lnode=head_; lnode; lnode=lnode->GetNext() )
    {
        lnode->CancelDeathNotification( nid, pid, verifier, trans_id);
    }

    TRACE_EXIT;
}
   
void CLNodeContainer::CheckForPendingCreates ( CProcess *process )
{
    process = process;
    //TODO
    abort();
}


CLNode *CLNodeContainer::GetLNode(int nid)
{
    CLNode *lnode;

    const char method_name[] = "CLNodeContainer::GetLNode";
    TRACE_ENTRY;

    
    if( nid >= 0 && nid < Nodes->NumberLNodes )
    {
        lnode = Nodes->LNode[nid];
    }
    else
    {
        lnode = NULL;
    }

    TRACE_EXIT;
    return lnode;
}

CLNode *CLNodeContainer::GetLNode (char *process_name, CProcess ** process, bool  checkstate)
{
    CLNode *lnode = head_;
    CProcess *p_process;

    const char method_name[] = "CLNodeContainer::GetLNode";
    TRACE_ENTRY;
    while (lnode)
    {
        *process = lnode->GetProcessL(process_name, checkstate);
        if (*process)
        { 
            p_process = *process;
            if (!p_process->IsBackup())
                break;
        }
        lnode = lnode->GetNext ();
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
    
    for ( lnode = head_; lnode; lnode = lnode->GetNext() )
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

bool CLNodeContainer::IsShutdownActive (void)
{
    bool status = false;
    CLNode *lnode = head_;

    const char method_name[] = "CLNodeContainer::IsShutdownActive";
    TRACE_ENTRY;
    
    while (lnode)
    {
        if (( node_->GetState() == State_Shutdown ) ||
            ( node_->GetState() == State_Stopped  )   )
        {
            status = true;
        }
        lnode = lnode->GetNext ();
    }

    TRACE_EXIT;

    return status;
}

int CLNodeContainer::ProcessCount( void )
{
    int count = 0;
    CLNode *lnode = head_;

    const char method_name[] = "CLNodeContainer::ProcessCount";
    TRACE_ENTRY;

    while (lnode)
    {
        if ( node_->GetState() == State_Up || node_->GetState() == State_Shutdown )
        {
            count += lnode->GetNumProcs();
        }
        lnode = lnode->GetNext ();
    }

    if (trace_settings & TRACE_ENTRY_EXIT)
       trace_printf("%s@%d" " - Count=" "%d" ", Exit" "\n", method_name, __LINE__, count);
    TRACE_EXIT;

    return (count<=0?0:count);
}
