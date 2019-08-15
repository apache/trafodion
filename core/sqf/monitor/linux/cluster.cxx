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

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "localio.h"
#include "mlio.h"
#include "monlogging.h"
#include "monsonar.h"
#include "montrace.h"
#include "redirector.h"
#include "healthcheck.h"
#include "config.h"
#include "device.h"
#include "cluster.h"
#include "monitor.h"

#include "replicate.h"

#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "reqqueue.h"
#include "zclient.h"
#include "commaccept.h"
#include "meas.h"
#ifdef NAMESERVER_PROCESS
#include "nscommacceptmon.h"
#else
#include "nameserver.h"
#include "ptpclient.h"
#endif

extern bool IAmIntegrating;
extern bool IAmIntegrated;
extern bool IsRealCluster;
extern bool IsAgentMode;
extern bool IsMaster;
extern bool IsMPIChild;
extern char MasterMonitorName[MAX_PROCESS_PATH];
extern char Node_name[MPI_MAX_PROCESSOR_NAME];
extern bool ZClientEnabled;
extern char IntegratingMonitorPort[MPI_MAX_PORT_NAME];
extern char MyCommPort[MPI_MAX_PORT_NAME];
extern char MyMPICommPort[MPI_MAX_PORT_NAME];
extern char MySyncPort[MPI_MAX_PORT_NAME];
#ifdef NAMESERVER_PROCESS
extern CCommAcceptMon CommAcceptMon;
extern char MyMon2NsPort[MPI_MAX_PORT_NAME];
#else
extern CProcess *NameServerProcess;
extern CNameServer *NameServer;
extern CPtpClient *PtpClient;
extern bool NameServerEnabled;
extern char MyPtPPort[MPI_MAX_PORT_NAME];
#endif
extern bool SMSIntegrating;
extern int CreatorShellPid;
extern Verifier_t CreatorShellVerifier;
extern CommType_t CommType;

extern int MyPNID;

extern CReqQueue ReqQueue;

extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
extern CConfigContainer *Config;
#ifndef NAMESERVER_PROCESS
extern CDeviceContainer *Devices;
#endif
extern CNode *MyNode;
extern CMonStats *MonStats;
#ifndef NAMESERVER_PROCESS
extern CRedirector Redirector;
#endif
extern CMonLog *MonLog;
extern CHealthCheck HealthCheck;
extern CCommAccept CommAccept;
extern CZClient    *ZClient;
extern CMeas Meas;

extern long next_test_delay;
extern CReplicate Replicator;

extern char *ErrorMsg (int error_code);

extern const char *ProcessTypeString( PROCESSTYPE type );

const char *JoiningPhaseString( JOINING_PHASE phase);
const char *StateString( STATE state);
#ifndef NAMESERVER_PROCESS
const char *SyncStateString( SyncState state);
#endif
const char *EpollEventString( __uint32_t events );
const char *EpollOpString( int op );

const char *NodePhaseString( NodePhase phase )
{
    const char *str;

    switch( phase )
    {
        case Phase_Ready:
            str = "Phase_Ready";
            break;
        case Phase_Activating:
            str = "Phase_Activating";
            break;
        default:
            str = "NodePhase - Undefined";
            break;
    }

    return( str );
}

void CCluster::ActivateSpare( CNode *spareNode, CNode *downNode, bool checkHealth )
{
    const char method_name[] = "CCluster::ActivateSpare";
    TRACE_ENTRY;
    // if not checking health, assume the spare is healthy
    bool spareHealthy = checkHealth ? false : true;
    int tmCount = 0;
    CNode *node;
    CLNode *lnode;

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d - pnid=%d, name=%s (%s) is taking over pnid=%d, name=%s (%s), check health=%d, isIntegrating=%d , integrating pnid=%d\n"
                    , method_name, __LINE__
                    , spareNode->GetPNid(), spareNode->GetName(), StateString(spareNode->GetState())
                    , downNode->GetPNid(), downNode->GetName(), StateString(downNode->GetState())
                    , checkHealth, IsIntegrating(), integratingPNid_ );
    }

    if ( checkHealth )
    {
        // TODO: Execute physical node health check script here
        spareHealthy = true;
        if ( !spareHealthy )
        {
            // and tell the cluster the node is down, since the spare can't takeover
            CReplNodeDown *repl = new CReplNodeDown(downNode->GetPNid());
            Replicator.addItem(repl);
        }
    }

    if ( spareHealthy )
    {
        if ( downNode->GetPNid() != spareNode->GetPNid() )
        {
            // Move down node's logical nodes to spare node
            downNode->MoveLNodes( spareNode );

            spareNode->SetPhase( Phase_Activating );

            Nodes->AddToSpareNodesList( downNode->GetPNid() );

            if ( !IsIntegrating() )
            {
                downNode->SetState( State_Down );

                // Send process death notices
#ifndef NAMESERVER_PROCESS
                spareNode->KillAllDown();
#endif

                // Send node down notice
                lnode = spareNode->GetFirstLNode();
                for ( ; lnode; lnode = lnode->GetNextP() )
                {
                    // Watchdog process clone was removed in KillAllDown
                    lnode->Down();
                }
            }
        }

        // Any DTMs running?
        for ( int i=0; !tmCount && i < Nodes->GetPNodesCount(); i++ )
        {
            node = Nodes->GetNodeByMap( i );
            lnode = node->GetFirstLNode();
            for ( ; lnode; lnode = lnode->GetNextP() )
            {
                CProcess *process = lnode->GetProcessLByType( ProcessType_DTM );
                if ( process  ) tmCount++;
            }
        }

        // Create Watchdog and PSD processes if this node is the activating spare
        if ( spareNode->GetPNid() == MyPNID )
        {
#ifndef NAMESERVER_PROCESS
            Monitor->StartPrimitiveProcesses();
#endif
        }
        else
        {
            // Check for end of joining phase on node re-integration
            if ( spareNode->GetState() == State_Joining )
            {
                spareNode->SetState( State_Up );
            }
            ResetIntegratingPNid();
        }

        if (trace_settings & TRACE_INIT)
        {
            trace_printf( "%s@%d - Spare node activating! pnid=%d, name=(%s)\n"
                        , method_name, __LINE__
                        , spareNode->GetPNid(), spareNode->GetName());
        }
    }

    if ( spareNode->GetPNid() == MyPNID && spareHealthy )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
           trace_printf( "%s@%d" " - Replicating activate spare node pnid=%d, name=%s (%s), spare=%d, down pnid=%d, name=%s (%s), DTM count=%d\n"
                       , method_name, __LINE__
                       , spareNode->GetPNid(), spareNode->GetName(), StateString(spareNode->GetState())
                       , spareNode->IsSpareNode()
                       , downNode->GetPNid(), downNode->GetName(), StateString(downNode->GetState())
                       , tmCount );
        // Let other monitors know is ok to activate this spare node
        CReplActivateSpare *repl = new CReplActivateSpare( MyPNID, downNode->GetPNid() );
        Replicator.addItem(repl);

#ifndef NAMESERVER_PROCESS
        if ( !tmCount )
        {
            // No DTMs in environment so implicitly make ready for transactions
            lnode = MyNode->GetFirstLNode();
            for ( ; lnode; lnode = lnode->GetNextP() )
            {
                ReqQueue.enqueueTmReadyReq( lnode->GetNid() );
            }
        }
#endif
    }

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
void CCluster::NodeTmReady( int nid )
{
    const char method_name[] = "CCluster::NodeTmReady";
    TRACE_ENTRY;

    if ( ! MyNode->IsSpareNode() && MyNode->GetPhase() != Phase_Ready )
    {
        MyNode->CheckActivationPhase();
    }

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d - nid=%d\n", method_name, __LINE__, nid );
    }

    tmReadyCount_++;

    if (trace_settings & (TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
    {
        trace_printf( "%s@%d - TmReady, nid=%d, tm count=%d, LNodesCount=%d\n"
                    , method_name, __LINE__
                    , nid
                    , tmReadyCount_
                    , MyNode->GetLNodesCount() );
    }

    if (IsRealCluster)
    {
        MyNode->StartPStartDPersistentDTM( nid );
    }

    if ( MyNode->GetLNodesCount() == tmReadyCount_ )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Node activated! pnid=%d, name=(%s) \n", method_name, MyNode->GetPNid(), MyNode->GetName());
        mon_log_write(MON_CLUSTER_NODE_TM_READY_2, SQ_LOG_INFO, la_buf);

        // Let other monitors know the node is up
        CReplActivateSpare *repl = new CReplActivateSpare( MyPNID, -1 );
        Replicator.addItem(repl);
    }

    TRACE_EXIT;
}
#endif

void CCluster::NodeReady( CNode *spareNode )
{
    const char method_name[] = "CCluster::NodeReady";
    TRACE_ENTRY;

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d - spare node %s pnid=%d, state=%s\n"
                    , method_name, __LINE__
                    , spareNode->GetName()
                    , spareNode->GetPNid()
                    , StateString(spareNode->GetState()) );
    }

    assert( spareNode->GetState() == State_Up );

    // Send node up notice
    CLNode *lnode = spareNode->GetFirstLNode();
    for ( ; lnode; lnode = lnode->GetNextP() )
    {
        lnode->Up();
    }

    spareNode->SetPhase( Phase_Ready );
    spareNode->SetActivatingSpare( false );
    HealthCheck.triggerTimeToLogHealth();

    TRACE_EXIT;
}

void CCluster::UpdateMonitorPort (const char* newMaster)
{
    const char method_name[] = "CCluster::UpdateMonitorPort";
    TRACE_ENTRY;
    
    char *monitorPort = getenv ("MONITOR_COMM_PORT");
    if ((monitorPort) && (newMaster))
    {
         strcpy( IntegratingMonitorPort, newMaster );
         strcat( IntegratingMonitorPort, ":");
         strcat( IntegratingMonitorPort, monitorPort);

          if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
          {
               trace_printf("%s@%d" " (MasterMonitor) UpdateMonitorPort Updating IntegratingMonitorPort to %s\n",
                             method_name, __LINE__,IntegratingMonitorPort );
          }
    } 
    TRACE_EXIT;
}

// Assign leaders as required
// Current leaders are TM Leader and Monitor Leader
void CCluster::AssignLeaders( int pnid, const char* failedMaster, bool checkProcess )
{
    const char method_name[] = "CCluster::AssignLeaders";
    TRACE_ENTRY;

#ifndef NAMESERVER_PROCESS
    AssignTmLeader ( pnid, checkProcess );
#else
    pnid = pnid;
    checkProcess = checkProcess;
#endif
    AssignMonitorLeader ( failedMaster );

    TRACE_EXIT;
}

// Assign monitor lead in the case of failure
void CCluster::AssignMonitorLeader( const char* failedMaster )
{
    const char method_name[] = "CCluster::AssignMonitorLeader";
    TRACE_ENTRY;
     
    if (!IsAgentMode || !ZClientEnabled)
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d" " - (MasterMonitor) not in AgentMode or zookeeper not enabled, returning\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return;
    }

    int rc = 0;
    CNode *node = NULL;
    CNode *failedMasterNode = Nodes->GetNode( (char *)failedMaster ); 
    int failedMasterPNid = failedMasterNode ? failedMasterNode->GetPNid() : -1;
    
    if (failedMaster == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d" " - (MasterMonitor) failedMaster is NULL, returning\n" , method_name, __LINE__);
        }
        TRACE_EXIT;
        return;
    }

    if (failedMasterNode == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d" " - (MasterMonitor) failedMasterNode is NULL, returning\n" , method_name, __LINE__);
        }
        TRACE_EXIT;
        return;
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d" " - (MasterMonitor) MonitorLeader %s (pnid=%d) failed!\n"
                    , method_name, __LINE__
                    , failedMasterNode->GetName()
                    , failedMasterPNid );
    }

    // delete old master if needed
    const char *masterMonitor = ZClient->MasterWaitForAndReturn(false);
    if (masterMonitor)
    {   
        // If it is the failed master, delete, do not delete anything else because we could delete a new master
        if (strcmp (masterMonitor, failedMaster) == 0)
        {
            ZClient->MasterZNodeDelete( failedMaster );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                 trace_printf( "%s@%d" " - (MasterMonitor) deleting master %s\n"
                              , method_name, __LINE__, masterMonitor );
             }
        }
        // else no worries
    }

    if ((MyNode && 
        (MyNode->IsPendingNodeDown()
      || MyNode->GetState() != State_Up
      || !IAmIntegrated)) )
    {
        // Do not let this monitor participate in choosing the master.  It can wait until an integrated
        // monitor makes a decision.
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
             trace_printf( "%s@%d" " - (MasterMonitor) This Node %s is not up yet or going down and will not participate in master choice!\n"
                   , method_name, __LINE__
                   , MyNode->GetName() );
        }
        
        if (failedMasterPNid != MyPNID)
        {
            // wait until another monitor choose a master
            const char *masterMonitor = ZClient->MasterWaitForAndReturn(true);
            if (masterMonitor)
            {
                UpdateMonitorPort ( masterMonitor );
            }
        }
        TRACE_EXIT;
        return;
    }

    int masterPNid = -1;
    // For all monitors who are up - choose the master using the same logic
    for ( int i = 0; i < GetConfigPNodesCount(); i++)
    {
        masterPNid = indexToPnid_[i];

        if (masterPNid == -1)
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - indexToPnid_[%d]=%d\n"
                            , method_name, __LINE__
                            , i
                            , indexToPnid_[i] );
            }
            continue;
        }

        node = Node[masterPNid];

        // skip this node
        if ( node == NULL )
        {
            continue; 
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Node pnid=%d (%s), state=%s, phase=%s, "
                          "isPendingNodeDown=%d\n"
                        , method_name, __LINE__
                        , node->GetPNid()
                        , node->GetName()
                        , StateString(node->GetState())
                        , NodePhaseString(node->GetPhase())
                        , node->IsPendingNodeDown() );
        }

        if ( node->IsSpareNode() ||
             node->IsPendingNodeDown() ||
             node->GetState() != State_Up ||
             node->GetPhase() != Phase_Ready )
        {
            continue; // skip this node for any of the above reasons
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Master Monitor candidate is %s, pnid=%d\n"
                        , method_name, __LINE__
                        , node->GetName(), node->GetPNid() );
        }

        const char *masterMonitor = ZClient->MasterWaitForAndReturn(false);
    
        //nobody has written it yet
        if (!masterMonitor)
        {
            if (!masterMonitor && node->GetPNid() == MyPNID)
            { // I'm the new master
                rc = ZClient->MasterZNodeCreate( node->GetName() );  
                if ( rc == ZOK )
                {
                    strcpy( MasterMonitorName, node->GetName() );
        
                    char    buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                                      , "[%s], Master Monitor is %s on node %d\n"
                                      , method_name, node->GetName(), node->GetPNid() );
                    mon_log_write(MON_CLUSTER_ASSIGNMONITORLEADER_1, SQ_LOG_INFO, buf);
                }
                else
                {
                     char    buf[MON_STRING_BUF_SIZE];
                     snprintf( buf, sizeof(buf)
                               , "[%s], Unable to create or set watch on master node %s\n"
                               , method_name, node->GetName() );
                     mon_log_write(MON_CLUSTER_ASSIGNMONITORLEADER_2, SQ_LOG_ERR, buf);
                }
            }
            else
            {
                int retries = 0;
                bool found = false;
                while (!found && (retries < ZCLIENT_MASTER_ZNODE_RETRY_COUNT)) 
                {
                    // the current node candidate is not my node
                    // so check for the current candidate to register as the master
                    masterMonitor = ZClient->MasterWaitForAndReturn(false);
                    if (!masterMonitor)
                    { // no master registered
                        if (node->GetState() == State_Down
                         || node->IsPendingNodeDown() )
                        {
                            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                            {
                                trace_printf( "%s@%d (MasterMonitor) Current "
                                              "candidate node %s, state=%s, pendingNodeDown=%d\n"
                                            , method_name, __LINE__
                                            , node->GetName()
                                            , StateString(node->GetState())
                                            , node->IsPendingNodeDown() );
                            }
                            break;
                        }
                        else
                        {
                            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                            {
                                trace_printf( "%s@%d (MasterMonitor) No masterMonitor registered, "
                                              "candidate is %s, pnid=%d, state=%s, pendingNodeDown=%d\n"
                                            , method_name, __LINE__
                                            , node->GetName(), node->GetPNid()
                                            , StateString(node->GetState())
                                            , node->IsPendingNodeDown() );
                            }
                            usleep(1000000); // sleep for a second as to not overwhelm the system   
                            retries++;
                            continue;
                        }
                    }
                    else
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d (MasterMonitor) Registered masterMonitor=%s, "
                                          "candidate node %s, pnid=%d\n"
                                        , method_name, __LINE__
                                        , masterMonitor
                                        , node->GetName(), node->GetPNid() );
                        }
        
                        strcpy( MasterMonitorName, masterMonitor );
        
                        char    buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                          , "[%s], Master Monitor is %s on node %d\n"
                                          , method_name, node->GetName(), node->GetPNid() );
                        mon_log_write(MON_CLUSTER_ASSIGNMONITORLEADER_3, SQ_LOG_INFO, buf);
                        found = true;
                    }
                }
                if (!masterMonitor)
                {
                    // the current node candidate no longer a candidate
                    continue;
                }
            }
        }
        else
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d (MasterMonitor) MasterMonitorName=%s, masterMonitor=%s\n"
                            , method_name, __LINE__
                            , MasterMonitorName
                            , masterMonitor );
            }

            strcpy( MasterMonitorName, masterMonitor );

            char    buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                              , "[%s], Master Monitor is %s on node %d\n"
                              , method_name, node->GetName(), node->GetPNid() );
            mon_log_write(MON_CLUSTER_ASSIGNMONITORLEADER_4, SQ_LOG_INFO, buf);
        }

        break;
    }

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
// Assigns a new TMLeader if given pnid is same as tmLeaderNid_
// TmLeader is a logical node num.
// pnid has gone down, so if that node was previously the TM leader, a new one needs to be chosen.
void CCluster::AssignTmLeader( int pnid, bool checkProcess )
{
    const char method_name[] = "CCluster::AssignTmLeader";
    TRACE_ENTRY;

    int i = 0;
    CNode *node = NULL;
    CProcess *process = NULL;

    int TmLeaderPNid = LNode[tmLeaderNid_]->GetNode()->GetPNid();

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
    {
        trace_printf( "%s@%d - pnid=%d, checkProcess=%d, tmLeaderNid_=%d, TmLeaderPNid=%d\n"
                    , method_name, __LINE__
                    , pnid, checkProcess, tmLeaderNid_, TmLeaderPNid );
    }

    if (TmLeaderPNid != pnid)
    {
        node = LNode[tmLeaderNid_]->GetNode();

        if (checkProcess)
        {
            process = LNode[tmLeaderNid_]->GetProcessLByType( ProcessType_DTM );
            if (process)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                {
                    if (node)
                        trace_printf( "%s@%d - Node pnid=%d (%s), phase=%s, "
                                      "checkProcess=%d\n"
                                    , method_name, __LINE__
                                    , node->GetPNid()
                                    , node->GetName()
                                    , NodePhaseString(node->GetPhase())
                                    , checkProcess );
                }
                return;
            }
            else
            {
                if (NameServerEnabled)
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                    {
                        trace_printf( "%s@%d - Getting process from Name Server, nid=%d, type=%s\n"
                                    , method_name, __LINE__
                                    , tmLeaderNid_, ProcessTypeString(ProcessType_DTM) );
                    }
                
                    process = Nodes->GetProcessLByTypeNs( tmLeaderNid_, ProcessType_DTM );
                    if (process)
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                        {
                            if (node)
                                trace_printf( "%s@%d - Node pnid=%d (%s), phase=%s, "
                                              "checkProcess=%d\n"
                                            , method_name, __LINE__
                                            , node->GetPNid()
                                            , node->GetName()
                                            , NodePhaseString(node->GetPhase())
                                            , checkProcess );
                        }
                        return;
                    }
                }
            }
        }
        else
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
            {
                if (node)
                    trace_printf( "%s@%d - Node pnid=%d (%s), state=%s, phase=%s, "
                                  "checkProcess=%d\n"
                                , method_name, __LINE__
                                , node->GetPNid()
                                , node->GetName()
                                , StateString(node->GetState())
                                , NodePhaseString(node->GetPhase())
                                , checkProcess );
            }
            return;
        }
    }

    node = Node[TmLeaderPNid];

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
    {
        trace_printf( "%s@%d - Node pnid=%d (%s), state=%s, TmLeader failed! "
                      "(tmLeaderNid_=%d, checkProcess=%d)\n"
                    , method_name, __LINE__
                    , node->GetPNid()
                    , node->GetName()
                    , StateString(node->GetState())
                    , tmLeaderNid_
                    , checkProcess );
    }

    for ( int i = 0; i < GetConfigPNodesCount(); i++)
    {
        TmLeaderPNid = indexToPnid_[i];

        if (TmLeaderPNid == indexToPnid_[GetConfigPNodesCount()])
        {
            TmLeaderPNid = indexToPnid_[0]; // restart with first nid
        }

        if (TmLeaderPNid == pnid)
        {
            continue; // this is the node that is going down, skip it
        }

        if (Node[TmLeaderPNid] == NULL)
        {
            continue;
        }

        node = Node[TmLeaderPNid];

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
        {
            trace_printf( "%s@%d - Node pnid=%d (%s), state=%s, phase=%s\n"
                        , method_name, __LINE__
                        , node->GetPNid()
                        , node->GetName()
                        , StateString(node->GetState())
                        , NodePhaseString(node->GetPhase()) );
        }

        if ( node->IsSpareNode() ||
             node->GetState() != State_Up ||
             node->GetPhase() != Phase_Ready )
        {
            continue; // skip this node for any of the above reasons
        }

        tmLeaderNid_ = node->GetFirstLNode()->GetNid();

        if (checkProcess)
        {
            process = LNode[tmLeaderNid_]->GetProcessLByType( ProcessType_DTM );
            if (!process)
            {
                continue; // skip this node no DTM process exists
            }
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
        {
            trace_printf("%s@%d" " - Node "  "%d" " is the new TmLeader." "\n", method_name, __LINE__, tmLeaderNid_);
        }

        break;
    }

    TRACE_EXIT;
}
#endif


CCluster::CCluster (void)
      :NumRanks (-1)
      ,socks_(NULL)
      ,sockPorts_(NULL)
      ,commSock_(-1)
      ,syncPort_(0)
      ,syncSock_(-1)
#ifdef NAMESERVER_PROCESS
      ,mon2nsSock_(-1)
#endif
      ,epollFD_(-1)
      ,epollPingFD_(-1),
      Node (NULL),
      LNode (NULL),
      currentNodes_ (0),
      configPNodesCount_ (-1),
      configPNodesMax_ (-1),
      nodeMap_ (NULL),
#ifndef NAMESERVER_PROCESS
      tmLeaderNid_ (-1),
      tmReadyCount_(0),
#endif
      minRecvCount_(4096),
      recvBuffer_(NULL),
      recvBuffer2_(NULL),
      swpRecCount_(0),
      barrierCount_(0),
      allGatherCount_(0),
      commDupCount_(0),
      barrierCountSaved_(0),
      allGatherCountSaved_(0),
      commDupCountSaved_(0),
      inBarrier_(false),
      inAllGather_(false),
      inCommDup_(false),
      monInitComplete_(false),
      monSyncResponsive_(true),
      integratingPNid_(-1),
      joinComm_(MPI_COMM_NULL),
      joinSock_(-1),
      lastSeqNum_(0),
      lowSeqNum_(0),
      highSeqNum_(0),
      reconnectSeqNum_(0),
      seqNum_(1)
      ,cumulativeSyncDelay_(0)
      ,syncDelayLogEventInterval_(CCluster::SYNC_DELAY_LOGGING_FREQUENCY_DEFAULT)
      ,syncDelayLogEventThreshold_(180)
      ,waitForWatchdogExit_(false)
      ,waitForNameServerExit_(false)
      ,checkSeqNum_(false)
      ,validateNodeDown_(false)
      ,enqueuedDown_(false)
      ,nodeDownDeathNotices_(true)
      ,verifierNum_(0)
#ifdef NAMESERVER_PROCESS
      ,myMonConnCount_(0)
      ,minMonConnCount_(0)
      ,minMonConnPnid_(-1)
#else
      ,clusterProcCount_(0)
#endif
{
    int i;
    const char method_name[] = "CCluster::CCluster";
    TRACE_ENTRY;

    configMaster_ = -1;
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,MPI_ERRORS_RETURN);

    char *env = getenv("SQ_MON_CHECK_SEQNUM");
    if ( env )
    {
        int val = atoi(env);
        if ( val > 0)
        {
            checkSeqNum_ = (val != 0);
        }
    }

    if (trace_settings & TRACE_INIT)
       trace_printf("%s@%d Checking sync sequence numbers is %s\n",
                    method_name, __LINE__,
                    (checkSeqNum_ ? "enabled" : "disabled"));

    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    configPNodesMax_ = clusterConfig->GetPNodesConfigMax();

    // get master from CClusterConfig
    configMaster_ = clusterConfig->GetConfigMaster();

    // Compute minimum "sync cycles" per second.   The minimum is 1/10
    // the expected number, assuming "next_test_delay" cycles per second (where
    // next_test_delay is in microseconds).
    syncMinPerSec_ = 1000000 / next_test_delay / 10;

    agMaxElapsed_.tv_sec = 0;
    agMaxElapsed_.tv_nsec = 0;
    agMinElapsed_.tv_sec = 10000;
    agMinElapsed_.tv_nsec = 0;

    // Allocate structures for monitor point-to-point communications
    //
    //   The current approach is to allocate to a maximum number (MAX_NODES).
    //
    //   The actual number could be based on the number of nodes configured
    //   which is better from a memory allocation perspective. However,
    //   this requires changing to an index-to-pnid map structure to access
    //   physical node objects (CNode) in the array structures and managing
    //   the map as nodes are added and deleted. (an optimization task)
    //
    comms_        = new MPI_Comm[MAX_NODES];
    otherMonRank_ = new int[MAX_NODES];
    socks_        = new int[MAX_NODES];
    sockPorts_    = new int[MAX_NODES];

    for ( int i =0; i < MAX_NODE_MASKS ; i++ )
    {
        upNodes_.upNodes[i] = 0;
    }

    for (i=0; i < MAX_NODES; ++i)
    {
        comms_[i] = MPI_COMM_NULL;
        socks_[i] = -1;
        sockPorts_[i] = -1;
    }

    env = getenv("SQ_MON_NODE_DOWN_VALIDATION");
    if ( env )
    {
        int val = atoi(env);
        if ( val > 0)
        {
            validateNodeDown_ = (val != 0);
        }
    }

    char buf[MON_STRING_BUF_SIZE];
    snprintf(buf, sizeof(buf), "[%s] Validation of node down is %s\n",
             method_name, (validateNodeDown_ ? "enabled" : "disabled"));
    mon_log_write(MON_CLUSTER_CLUSTER_1, SQ_LOG_INFO, buf);

    InitializeConfigCluster();

    for (size_t j=0; j<(sizeof(agElapsed_)/sizeof(int)); ++j)
    {
        agElapsed_[j] = 0;
    }

    char *p = getenv("MON_MIN_RECV_COUNT");
    if ( p )
    {
        long int val = strtoul(p, NULL, 10);
        if (errno != ERANGE)
        {
            minRecvCount_ = val;
        }
    }

    p = getenv("SQ_MON_NODE_DOWN_DEATH_MESSAGES");
    if ( p && atoi(p) == 0)
    {
        nodeDownDeathNotices_ = false;
    }

    // build the node objects & Sync collision assignment arrays
    // these buffers will be used in ShareWithPeers in AllGather
    // operation to get TMSync data as well as Replication data.
    // Allocate the maximum allowed so that we pay the price only once.
    // This wastes a bit of memory but reduces complexity when
    // adding and deleting nodes. Usage is based on GetConfigPNodesMax()
    // the maximum number that can be configured.
    recvBuffer_ = new struct sync_buffer_def[GetConfigPNodesMax()];
    recvBuffer2_ = new struct sync_buffer_def[GetConfigPNodesMax()];
    memset( recvBuffer_, 0, sizeof(sync_buffer_def[GetConfigPNodesMax()]) );
    memset( recvBuffer2_, 0, sizeof(sync_buffer_def[GetConfigPNodesMax()]) );

    char *syncDelayLogEventIntervalC = getenv("SQ_MON_SYNC_DELAY_LOGGING_FREQUENCY");
    if ( syncDelayLogEventIntervalC ) 
    {
        syncDelayLogEventInterval_ = atoi(syncDelayLogEventIntervalC);
    }

    int thresholdPercent = CCluster::SYNC_DELAY_LOGGING_THRESHOLD_MAX;
    float threshold  = 0.5;
    char *syncDelayLogEventThresholdC = getenv("SQ_MON_SYNC_DELAY_LOGGING_THRESHOLD");
    if ( syncDelayLogEventThresholdC ) 
    {
        thresholdPercent = atoi(syncDelayLogEventThresholdC);
        thresholdPercent = (thresholdPercent > 50) 
            ? CCluster::SYNC_DELAY_LOGGING_THRESHOLD_MAX : thresholdPercent;
        threshold  = (thresholdPercent/100.0);
        syncDelayLogEventThreshold_ = (HealthCheck.getSyncTimeout() * threshold);
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d thresholdPercent=%d, threshold=%f, "
                     "syncDelayLogEventThreshold_=%d, syncDelayLogEventInterval_=%d\n"
                    , method_name, __LINE__
                    , thresholdPercent
                    , threshold
                    , syncDelayLogEventThreshold_
                    , syncDelayLogEventInterval_ );
    }

    TRACE_EXIT;
}

CCluster::~CCluster (void)
{
    const char method_name[] = "CCluster::~CCluster";
    TRACE_ENTRY;

    if (epollPingFD_ != -1)
    {
        close( epollPingFD_ );
    }

    if (epollFD_ != -1)
    {
        close( epollFD_ );
    }

    if (commSock_ != -1)
    {
        close( commSock_ );
    }

    if (syncSock_ != -1)
    {
        close( syncSock_ );
    }

    delete [] comms_;
    delete [] otherMonRank_;
    delete [] socks_;
    delete [] sockPorts_;
    if (nodeMap_)
    {
        delete [] nodeMap_;
        nodeMap_ = NULL;
    }

    delete [] recvBuffer2_;
    delete [] recvBuffer_;

    TRACE_EXIT;
}

int CCluster::incrGetVerifierNum()
{
    verifierNum_++;
    if ( verifierNum_ < 0 )
    {
        verifierNum_ = 0;
    }

    return verifierNum_;
}

// For a reintegrated monitor node, following the first sync cycle, obtain the
// current sync cycle sequence number.   And verify that all nodes agree
// on the sequence number.
unsigned long long CCluster::EnsureAndGetSeqNum(cluster_state_def_t nodestate[])
{
    const char method_name[] = "CCluster::EnsureAndGetSeqNum";
    TRACE_ENTRY;

    unsigned long long seqNum = 0;

    for (int i = 0; i < GetConfigPNodesCount(); i++)
    {
        if (trace_settings & TRACE_RECOVERY)
        {
            trace_printf("%s@%d nodestate[%d].seq_num=%lld, seqNum=%lld\n"
                        , method_name, __LINE__
                        , indexToPnid_[i]
                        , nodestate[indexToPnid_[i]].seq_num, seqNum );
        }
        if (nodestate[indexToPnid_[i]].seq_num > 1)
        {
            if (seqNum == 0)
            {
                seqNum = nodestate[indexToPnid_[i]].seq_num;
            }
            else if (nodestate[indexToPnid_[i]].seq_num != seqNum)
            {
                char    buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Sync sequence number mismatch, expecting "
                          "seqNum=%lld, my pnid=%d, nodestate[%d].seq_num=%lld\n"
                        , method_name
                        , seqNum, MyPNID, indexToPnid_[i]
                        , nodestate[indexToPnid_[i]].seq_num);
                mon_log_write(MON_CLUSTER_ENSUREANDGETSEQNUM_1, SQ_LOG_CRIT, buf);
                mon_failure_exit();
            }
        }
        if (trace_settings & TRACE_RECOVERY)
        {
            trace_printf("%s@%d nodestate[%d].seq_num=%lld, seqNum=%lld\n"
                        , method_name, __LINE__
                        , indexToPnid_[i]
                        , nodestate[indexToPnid_[i]].seq_num, seqNum );
        }
    }

    TRACE_EXIT;
    return seqNum;
}


#ifndef NAMESERVER_PROCESS
void CCluster::HardNodeDown (int pnid, bool communicate_state)
{
    char port_fname[MAX_PROCESS_PATH];
    char temp_fname[MAX_PROCESS_PATH];
    CNode  *node;
    CLNode *lnode;
    char    buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CCluster::HardNodeDown";
    TRACE_ENTRY;

    node = Nodes->GetNode(pnid);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
       trace_printf( "%s@%d - pnid=%d, comm_state=%d, state=%s, isInQuiesceState=%d,"
                     " (local pnid=%d, state=%s, isInQuiesceState=%d, "
                     "shutdown level=%d)\n", method_name, __LINE__,
                     pnid, communicate_state, StateString(node->GetState()),
                     node->isInQuiesceState(),
                     MyPNID, StateString(MyNode->GetState()),
                     MyNode->isInQuiesceState(), MyNode->GetShutdownLevel() );

    if (( MyPNID == pnid              ) &&
        ( MyNode->GetState() == State_Down ||
          MyNode->IsKillingNode() ) )
    {
        // we are coming down ... don't process it
        if ( !IsRealCluster && MyNode->isInQuiesceState())
        {
          // in virtual env, this would be called after node quiescing,
          // so continue with mark down processing.
        }
        else
        {
          return;
        }
    }

    if ( (MyNode->GetShutdownLevel() != ShutdownLevel_Undefined) &&
         (pnid != MyPNID) ) // some other node went down while shutdown was in progress
    {
        snprintf(buf, sizeof(buf), "[%s], Node failure during shutdown, down nid = %d\n", method_name, pnid);
        mon_log_write(MON_CLUSTER_MARKDOWN_1, SQ_LOG_ERR, buf);

        if (!waitForWatchdogExit_) // if WDT is not exiting
        {
            // bring down this node because TSE backup processes may not exit
            // if the primary was on the node that went down.
            ReqQueue.enqueueDownReq(MyPNID);
        }
    }

    if ( communicate_state && pnid != MyPNID )
    {
        // just communicate the change and let the real node handle it.
        node->SetChangeState( true );
        return;
    }

    if ( !Emulate_Down )
    {
        if( !IsRealCluster )
        {
            snprintf(port_fname, sizeof(port_fname), "%s/monitor.%d.port.%s",getenv("TRAF_LOG"),pnid,node->GetName());
        }
        else
        {
            // Remove the domain portion of the name if any
            char short_node_name[MPI_MAX_PROCESSOR_NAME];
            char str1[MPI_MAX_PROCESSOR_NAME];
            memset( short_node_name, 0, MPI_MAX_PROCESSOR_NAME );
            memset( str1, 0, MPI_MAX_PROCESSOR_NAME );
            strcpy (str1, node->GetName() );

            char *str1_dot = strchr( (char *) str1, '.' );
            if ( str1_dot )
            {
                memcpy( short_node_name, str1, str1_dot - str1 );
            }
            else
            {
                strcpy (short_node_name, str1 );
            }
            snprintf(port_fname, sizeof(port_fname), "%s/monitor.port.%s",getenv("TRAF_LOG"),short_node_name);
        }
        sprintf(temp_fname, "%s.bak", port_fname);
        remove(temp_fname);
        rename(port_fname, temp_fname);
    }

    if (node->GetState() != State_Down || !node->isInQuiesceState())
    {
        snprintf(buf, sizeof(buf),
                 "[CCluster::HardNodeDown], Node %s (%d) is going down.\n",
                 node->GetName(), node->GetPNid());
        mon_log_write(MON_CLUSTER_MARKDOWN_2, SQ_LOG_CRIT, buf);

        node->SetPendingNodeDown( true );
        node->SetKillingNode( true );
        node->SetPhase( Phase_Undefined );

        if ( MyPNID == pnid &&
             (MyNode->GetState() == State_Up || MyNode->GetState() == State_Shutdown) &&
            !MyNode->isInQuiesceState() )
        {
            STATE state = MyNode->GetState();
            switch ( state )
            {
            case State_Up:
            case State_Shutdown:
                // do node quiescing and let HealthCheck thread know that quiescing has started
                // setting internal state to 'quiesce' will prevent replicating process exits
                // and reject normal shutdown requests in all nodes while we are quiescing.
                if (!waitForWatchdogExit_) // if WDT is not exiting
                {
                    MyNode->setQuiesceState();
                    HealthCheck.setState(MON_NODE_QUIESCE);
                }
                if( IsRealCluster )
                { // Terminate CommAccept thread, remote pings will fail
                    CommAccept.shutdownWork();
                }
                break;
            default: // in all other states
                if ( ! Emulate_Down )
                {
                    // make sure no processes are alive if in the middle of re-integration
                    node->KillAllDown();
                    snprintf(buf, sizeof(buf),
                             "[CCluster::HardNodeDown], Node %s (%d)is down.\n",
                             node->GetName(), node->GetPNid());
                    mon_log_write(MON_CLUSTER_MARKDOWN_3, SQ_LOG_ERR, buf);

                    mon_failure_exit();
                }
            }
        }
        else
        {
            if (node->GetState() != State_Down)
            {
                if ( node->GetPNid() == integratingPNid_ )
                {
                    ResetIntegratingPNid();
                }
                node->KillAllDown();
                node->SetState( State_Down );
                node->SetPendingNodeDown( false );
                // Send node down message to local node's processes
                lnode = node->GetFirstLNode();
                for ( ; lnode; lnode = lnode->GetNextP() )
                {
                    lnode->Down();
                }
                if ( ZClientEnabled )
                {
                    ZClient->RunningZNodeDelete( node->GetName() );
                    ZClient->MasterZNodeDelete( node->GetName() );
                }
            }
        }
    }
    if ( Emulate_Down )
    {
        AssignTmLeader(pnid, false);
    }
    else
    {
        AssignLeaders(pnid, node->GetName(), false);
    }

    HealthCheck.triggerTimeToLogHealth();

    TRACE_EXIT;
}
#endif

#ifdef NAMESERVER_PROCESS
void CCluster::HardNodeDownNs( int pnid )
{
    CNode  *node;
    char    buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CCluster::HardNodeDownNs";
    TRACE_ENTRY;

    node = Nodes->GetNode(pnid);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
       trace_printf( "%s@%d - pnid=%d, state=%s, isInQuiesceState=%d,"
                     " (local pnid=%d, state=%s, isInQuiesceState=%d, "
                     "shutdown level=%d)\n", method_name, __LINE__,
                     pnid, StateString(node->GetState()),
                     node->isInQuiesceState(),
                     MyPNID, StateString(MyNode->GetState()),
                     MyNode->isInQuiesceState(), MyNode->GetShutdownLevel() );

    if (( MyPNID == pnid              ) &&
        ( MyNode->GetState() == State_Down ||
          MyNode->IsKillingNode() ) )
    {
        // we are coming down ... don't process it
        if ( !IsRealCluster && MyNode->isInQuiesceState())
        {
          // in virtual env, this would be called after node quiescing,
          // so continue with mark down processing.
        }
        else
        {
          return;
        }
    }

    if (node->GetState() != State_Down)
    {
        snprintf( buf, sizeof(buf)
                , "[%s], Node %s (%d) is going down.\n"
                 , method_name, node->GetName(), node->GetPNid());
        mon_log_write(MON_CLUSTER_MARKDOWN_4, SQ_LOG_INFO, buf);

        node->SetKillingNode( true );
        node->DeleteAllDown();
        node->SetState( State_Down );

        if ( ZClientEnabled )
        {
            ZClient->RunningZNodeDelete( node->GetName() );
        }
    }

    AssignLeaders(pnid, node->GetName(), false);

    TRACE_EXIT;
}
#endif

int CCluster::CheckSockPeer( int pnid, MPI_Status *stats, peer_t *peer )
{
    const char method_name[] = "CCluster::CheckSockPeer";
    TRACE_ENTRY;

    int err = MPI_SUCCESS;
    CNode *node;

    if( !IsRealCluster )
    { // In virtual cluster, just return success
        TRACE_EXIT;
        return( err );
    }

    // Release the sync lock temporarily to allow request worker thread to
    // process any request that needs the sync lock.
    Monitor->ExitSyncCycle();
    pthread_yield();

    node = Nodes->GetNode( pnid );
    if (node)
    {
        if (node->GetState() != State_Up)
        {
            if (socks_[pnid] != -1)
            { // Peer socket is still active
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - Node %s (%d) is not up, "
                                  "removing old socket from epoll set, "
                                  "socks_[%d]=%d\n"
                                , method_name, __LINE__
                                , node->GetName(), node->GetPNid()
                                , pnid, socks_[pnid] );
                }
                stats[pnid].MPI_ERROR = MPI_ERR_EXITED;
                stats[pnid].count = 0;
                err = MPI_ERR_IN_STATUS;
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                  "stats[%d].MPI_ERROR=%s\n"
                                , method_name, __LINE__
                                , node->GetName(), node->GetPNid()
                                , pnid
                                , ErrorMsg(stats[pnid].MPI_ERROR) );
                }

                --currentNodes_;
                // Clear bit in set of "up nodes"
                upNodes_.upNodes[pnid/MAX_NODE_BITMASK] &= ~(1ull << (pnid%MAX_NODE_BITMASK));
    
                // Remove old socket from epoll set, it may not be there
                struct epoll_event event;
                event.data.fd = socks_[pnid];
                event.events = 0;
                EpollCtlDelete( epollFD_, socks_[pnid], &event );
                shutdown( socks_[pnid], SHUT_RDWR);
                close( socks_[pnid] );
                socks_[pnid] = -1;
            }
        }
        else if ( pnid > MyPNID )
        { // peer is above node is my node, so connect to peer
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Pinging node %s (%d) to see if it's up\n"
                            , method_name, __LINE__
                            , node->GetName(), node->GetPNid() );
            }
            if (PingSockPeer( node, peer->znodeFailedTime ))
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - Node %s (%d) is available\n"
                                , method_name, __LINE__
                                , node->GetName(), node->GetPNid() );
                }
            }
            else
            {
                if (node->GetState() != State_Up)
                {
                    if (socks_[pnid] != -1)
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Node %s (%d) is not up, "
                                          "removing old socket from epoll set, "
                                          "socks_[%d]=%d\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , pnid, socks_[pnid] );
                        }
    
                        --currentNodes_;
                        // Clear bit in set of "up nodes"
                        upNodes_.upNodes[pnid/MAX_NODE_BITMASK] &= ~(1ull << (pnid%MAX_NODE_BITMASK));
            
                        // Remove old socket from epoll set, it may not be there
                        struct epoll_event event;
                        event.data.fd = socks_[pnid];
                        event.events = 0;
                        EpollCtlDelete( epollFD_, socks_[pnid], &event );
                        shutdown( socks_[pnid], SHUT_RDWR);
                        close( socks_[pnid] );
                        socks_[pnid] = -1;
                    }
                    stats[pnid].MPI_ERROR = MPI_ERR_EXITED;
                    stats[pnid].count = 0;
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                      "stats[%d].MPI_ERROR=%s\n"
                                    , method_name, __LINE__
                                    , node->GetName(), node->GetPNid()
                                    , pnid
                                    , ErrorMsg(stats[pnid].MPI_ERROR) );
                    }
                }
                err = MPI_ERR_IN_STATUS;
            }
        }
        else
        { // peer is below my node, accept connection from peer
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Pinging node %s (%d) to see if it's up\n"
                            , method_name, __LINE__
                            , node->GetName(), node->GetPNid() );
            }
            if (PingSockPeer( node, peer->znodeFailedTime ))
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - Node %s (%d) is available\n"
                                , method_name, __LINE__
                                , node->GetName(), node->GetPNid() );
                }
            }
            else
            {
                if (node->GetState() != State_Up)
                {
                    if (socks_[pnid] != -1)
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Node %s (%d) is not up, "
                                          "removing old socket from epoll set, "
                                          "socks_[%d]=%d\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , pnid, socks_[pnid] );
                        }
    
                        --currentNodes_;
                        // Clear bit in set of "up nodes"
                        upNodes_.upNodes[pnid/MAX_NODE_BITMASK] &= ~(1ull << (pnid%MAX_NODE_BITMASK));
            
                        // Remove old socket from epoll set, it may not be there
                        struct epoll_event event;
                        event.data.fd = socks_[pnid];
                        event.events = 0;
                        EpollCtlDelete( epollFD_, socks_[pnid], &event );
                        shutdown( socks_[pnid], SHUT_RDWR);
                        close( socks_[pnid] );
                        socks_[pnid] = -1;
                    }
                    stats[pnid].MPI_ERROR = MPI_ERR_EXITED;
                    stats[pnid].count = 0;
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                      "stats[%d].MPI_ERROR=%s\n"
                                    , method_name, __LINE__
                                    , node->GetName(), node->GetPNid()
                                    , pnid
                                    , ErrorMsg(stats[pnid].MPI_ERROR) );
                    }
                }
                err = MPI_ERR_IN_STATUS;
            }
        }
    
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            for ( int i = 0; i < GetConfigPNodesCount(); i++ )
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d" " - socks_[%d]=%d, "
                                  "stats[%d].MPI_ERROR=%s\n"
                                , method_name, __LINE__
                                , indexToPnid_[i]
                                , socks_[indexToPnid_[i]]
                                , indexToPnid_[i]
                                , ErrorMsg(stats[indexToPnid_[i]].MPI_ERROR) );
                }
            }
            trace_printf( "%s@%d - Returning err=%d(%s)\n"
                        , method_name, __LINE__, err, ErrorMsg(err) );
        }
    }

    Monitor->EnterSyncCycle();

    TRACE_EXIT;
    return( err );
}

bool CCluster::CheckSpareSet( int pnid )
{
    bool activatedSpare = false;
    bool done = false;
    unsigned int ii;
    unsigned int jj;
    CNode *newNode = Nodes->GetNode( pnid );

    const char method_name[] = "CCluster::CheckSpareSet";
    TRACE_ENTRY;

    // Build spare node set
    CNode *spareNode;
    NodesList spareSetList;
    NodesList *spareNodesConfigList = Nodes->GetSpareNodesConfigList();
    NodesList::iterator itSn;
    for ( itSn = spareNodesConfigList->begin();
          itSn != spareNodesConfigList->end() && !done ; itSn++ )
    {
        spareNode = *itSn;
        PNidVector sparePNids = spareNode->GetSparePNids();
        // if the new node is a spare node in the configuration
        if ( newNode->GetPNid() == spareNode->GetPNid() )
        {
            // Add the spare node and each node it is configured to spare to the set
            spareSetList.push_back( spareNode );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf("%s@%d - pnid=%d, name=(%s) is a configured Spare\n", method_name, __LINE__, spareNode->GetPNid(), spareNode->GetName());

            for ( ii = 0; ii < sparePNids.size(); ii++ )
            {
                spareSetList.push_back( Nodes->GetNode(sparePNids[ii]) );

                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    trace_printf("%s@%d - pnid=%d, name=(%s) is in Spare set\n", method_name, __LINE__, Nodes->GetNode(sparePNids[ii])->GetPNid(), Nodes->GetNode(sparePNids[ii])->GetName());
            }
            done = true;
        }
        else
        {
            // Check each pnid it is configured to spare
            for ( jj = 0; jj < sparePNids.size(); jj++ )
            {
                // if the new node is in the spare set of a spare node
                if ( newNode->GetPNid() == sparePNids[jj] )
                {
                    // Add the spare node and each node it is configured to spare to the set
                    spareSetList.push_back( spareNode );

                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        trace_printf("%s@%d - pnid=%d, name=(%s) is a configured Spare\n", method_name, __LINE__, spareNode->GetPNid(), spareNode->GetName());

                    for ( ii = 0; ii < sparePNids.size(); ii++ )
                    {
                        spareSetList.push_back( Nodes->GetNode(sparePNids[ii]) );

                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                            trace_printf("%s@%d - pnid=%d, name=(%s) is in Spare set\n", method_name, __LINE__, Nodes->GetNode(sparePNids[ii])->GetPNid(), Nodes->GetNode(sparePNids[ii])->GetName());
                    }
                    done = true;
                }
            }
        }
    }

    if (newNode && trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - new node pnid=%d, name=(%s), zid=%d\n"
                    , method_name, __LINE__
                    , newNode->GetPNid(), newNode->GetName(), newNode->GetZone());
    }

    // if the newNode still owns the zone
    if ( newNode && newNode->GetZone() != -1 )
    {
        // assume implicit spare node activation
        // (no need to move logical nodes to physical node)
        // since HardNodeUp() already set State_Up,
        // just reset spare node flag and remove from available spare nodes
        newNode->ResetSpareNode();
        Nodes->RemoveFromSpareNodesList( newNode );
        ActivateSpare( newNode, newNode );
        activatedSpare = true;
        TRACE_EXIT;
        return( activatedSpare );
    }

    CLNode  *lnode;
    CNode   *node;
    CNode   *downNode = NULL;

    // Now check the state of each configured logical node in the set for down state
    spareNode = newNode;  // new node (pnid) is the spare to activate
    NodesList::iterator itSs;
    for ( itSs = spareSetList.begin(); itSs != spareSetList.end(); itSs++ )
    {
        node = *itSs;
        if ( node->GetPNid() != pnid )
        {
            // Find the first down node
            if ( !downNode )
            {
                lnode = node->GetFirstLNode();
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    trace_printf( "%s@%d - node nid=%d, pnid=%d(%s), state=%s\n"
                                , method_name, __LINE__, lnode?lnode->GetNid():-1
                                , node->GetPNid(), node->GetName()
                                , StateString( node->GetState() ) );
                if ( lnode && lnode->GetState() == State_Down )
                {
                    downNode = node;
                }
            }
        }
        if ( spareNode && downNode )
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf( "%s@%d - spare node pnid=%d (%s), down node pnid=%d (%s) \n"
                            , method_name, __LINE__
                            , spareNode->GetPNid(), spareNode->GetName()
                            , downNode->GetPNid(), downNode->GetName());
            break;
        }
    }

    if ( spareNode && downNode )
    {
        Nodes->RemoveFromSpareNodesList( spareNode );
        spareNode->ResetSpareNode();
        if ( downNode->GetPNid() != pnid )
        { // the spare node does not own the down logical nodes so activate it
            ActivateSpare( spareNode, downNode );
        }
        activatedSpare = true;
    }

    TRACE_EXIT;
    return( activatedSpare );
}

const char *JoiningPhaseString( JOINING_PHASE phase )
{
    const char *str;

    switch( phase )
    {
        case JoiningPhase_Unknown:
            str = "JoiningPhase_Unknown";
            break;
        case JoiningPhase_1:
            str = "JoiningPhase_1";
            break;
        case JoiningPhase_2:
            str = "JoiningPhase_2";
            break;
        case JoiningPhase_3:
            str = "JoiningPhase_3";
            break;
        default:
            str = "JoiningPhase - Undefined";
            break;
    }

    return( str );
}

struct message_def *CCluster::JoinMessage( const char *node_name, int pnid, JOINING_PHASE phase )
{
    struct message_def *msg;

    const char method_name[] = "CCluster::JoinMessage";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->notice_death_Incr();

    msg = new struct message_def;
    msg->type = MsgType_NodeJoining;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    strcpy( msg->u.request.u.joining.node_name, node_name );
    msg->u.request.u.joining.pnid = pnid;
    msg->u.request.u.joining.phase = phase;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST_DETAIL))
        trace_printf("%s@%d - Joining notice for node %s (pnid=%d, phase=%d)\n",
                     method_name, __LINE__, node_name, pnid, phase );
    TRACE_EXIT;

    return msg;
}

struct message_def *CCluster::SpareUpMessage( const char *node_name, int pnid )
{
    struct message_def *msg;

    const char method_name[] = "CCluster::SpareUpMessage";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->notice_death_Incr();

    msg = new struct message_def;
    msg->type = MsgType_SpareUp;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    strcpy( msg->u.request.u.spare_up.node_name, node_name );
    msg->u.request.u.spare_up.pnid = pnid;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST_DETAIL))
        trace_printf("%s@%d - Spare node up notice for node %s nid=%d\n",
                     method_name, __LINE__, node_name, pnid );
    TRACE_EXIT;

    return msg;
}

struct message_def *CCluster::ReIntegErrorMessage( const char *msgText )
{
    struct message_def *msg;

    const char method_name[] = "CCluster::ReIntegErrorMessage";
    TRACE_ENTRY;

    msg = new struct message_def;
    msg->type = MsgType_ReintegrationError;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    strncpy( msg->u.request.u.reintegrate.msg, msgText,
             sizeof(msg->u.request.u.reintegrate.msg) );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST_DETAIL))
        trace_printf("%s@%d - Reintegrate notice %s\n",
                     method_name, __LINE__, msgText );

    TRACE_EXIT;

    return msg;
}

int CCluster::HardNodeUp( int pnid, char *node_name )
{
    bool    spareNodeActivated = false;
    int     rc = MPI_SUCCESS;
    int     tmCount = 0;
    CNode  *node;
    CLNode *lnode;
    STATE   nodeState;

    const char method_name[] = "CCluster::HardNodeUp";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
       trace_printf( "%s@%d - pnid=%d, name=%s (MyPNID = %d), currentNodes_=%d\n"
                   , method_name, __LINE__, pnid, node_name, MyPNID, currentNodes_ );

    if ( pnid == -1 )
    {
        node = Nodes->GetNode( node_name );
    }
    else
    {
        node = Nodes->GetNode( pnid );
    }

    if ( node == NULL )
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
           trace_printf( "%s@%d" " - Invalid node, pnid=%d, name=%s" "\n"
                       , method_name, __LINE__, pnid, node_name );

        return( MPI_ERR_NAME );
    }

    nodeState = node->GetState();

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
       trace_printf( "%s@%d" " - Node state=%s" "\n"
                   , method_name, __LINE__, StateString( nodeState ) );
    if ( nodeState != State_Up )
    {
        if ( nodeState == State_Down )
        {
            node->SetKillingNode( false );
#ifndef NAMESERVER_PROCESS
            if ( Emulate_Down )
            {
#endif
                // Any DTMs running?
                for ( int i=0; !tmCount && i < Nodes->GetPNodesCount(); i++ )
                {
                    CNode  *tempNode = Nodes->GetNodeByMap( i );
                    lnode = tempNode->GetFirstLNode();
                    for ( ; lnode; lnode = lnode->GetNextP() )
                    {
                        CProcess *process = lnode->GetProcessLByType( ProcessType_DTM );
                        if ( process  ) tmCount++;
                    }
                }
                if ( tmCount )
                {
                    IAmIntegrated = true;
                }
                // We need to remove any old process objects before we restart the node.
                node->CleanUpProcesses();
                node->SetState( State_Up );
                if ( MyPNID == pnid )
                {
                    MyNode->clearQuiesceState();
                    HealthCheck.initializeVars();
                    SMSIntegrating = true;
#ifndef NAMESERVER_PROCESS
                    Monitor->StartPrimitiveProcesses();
#endif
                    // Let other monitors know this node is up
                    CReplNodeUp *repl = new CReplNodeUp(MyPNID);
                    Replicator.addItem(repl);
                }
                else
                {
                    if ( tmCount == 0 )
                    {
                        // Process logical node up
                        lnode = node->GetFirstLNode();
                        for ( ; lnode; lnode = lnode->GetNextP() )
                        {
                            lnode->Up();
                        }
                    }
                }
#ifndef NAMESERVER_PROCESS
            }
            else
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    trace_printf( "%s@%d - Unexpectedly executing HardNodeUp.  Expecting to do accept in commAccept thread\n",
                                  method_name, __LINE__ );

            }
#endif
        }
        else if ( nodeState == State_Merged )
        {
            node->SetKillingNode( false );
            node->SetState( State_Joining );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d" " - New monitor %s, pnid=%d, state=%s" "\n"
                            , method_name, __LINE__, node->GetName(), node->GetPNid(), StateString( node->GetState() ) );
                for ( int i =0; i < Nodes->GetPNodesCount(); i++ )
                {
                    trace_printf( "%s@%d socks_[indexToPnid_[%d]=%d]=%d, sockPorts_[indexToPnid_[%d]=%d]=%d\n"
                                , method_name, __LINE__
                                , i, indexToPnid_[i], socks_[indexToPnid_[i]]
                                , i, indexToPnid_[i], sockPorts_[indexToPnid_[i]] );
                }
            }
            if ( MyNode->IsCreator() )
            {
#ifndef NAMESERVER_PROCESS
                SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                       , MyNode->GetCreatorVerifier()
                                                       , JoinMessage( node->GetName()
                                                                    , node->GetPNid()
                                                                    , JoiningPhase_1 )
                                                       , NULL);
#endif

                // save the current seq num in the snapshot request.
                // this sequence number will match the state of the cluster
                // when this request is processed.
                ReqQueue.enqueueSnapshotReq(seqNum_);
            }
            if ( MyPNID == pnid )
            {
                // request and process revive packet from the creator.
                // when complete, this will call HardNodeUp again.
                ReqQueue.enqueueReviveReq( );
            }
            else
            {
                if ( ZClientEnabled )
                {
                    rc = ZClient->RunningZNodeWatchAdd( node->GetName() );
                    if ( rc != ZOK )
                    {
                        char    buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                , "[%s], Unable to set node watch on %s, pnid%d\n"
                                , method_name, node->GetName(), node->GetPNid() );
                        mon_log_write(MON_CLUSTER_HARDNODEUP_1, SQ_LOG_ERR, buf);
                    }
                }
            }
        }
        else if ( nodeState == State_Joining )
        {
            // The new monitor comes in here first and schedules a node up request on all nodes.
            // All other monitors come here next, including the creator.
            // The new monitor will not come here again because
            // CReplNodeUp is a noop for the one who schedules it.
            node->SetState( State_Up );

            if ( Nodes->GetSNodesCount() == 0 )
            { // Spare nodes not configured so bring up my logical nodes
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                   trace_printf( "%s@%d" " - No spare nodes configured node=%s, pnid=%d, state=%s\n"
                               , method_name, __LINE__, node->GetName(), node->GetPNid()
                               , StateString(node->GetState()) );
                if ( MyPNID == pnid )
                {
                    ActivateSpare( node, node );
                }
            }
            else
            {
                node->SetSpareNode();
                Nodes->AddToSpareNodesList( node->GetPNid() );
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                   trace_printf( "%s@%d" " - Adding to available spares node=%s, pnid=%d\n"
                               , method_name, __LINE__, node->GetName(), node->GetPNid() );
                // Check for a node down in spare set and activate down node if found
                spareNodeActivated = CheckSpareSet( node->GetPNid() );
                if ( spareNodeActivated )
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                       trace_printf( "%s@%d" " - Activated spare node=%s, pnid=%d\n"
                                   , method_name, __LINE__, node->GetName(), node->GetPNid() );
                }
                else
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                       trace_printf( "%s@%d" " - Available spare node=%s, pnid=%d\n"
                                   , method_name, __LINE__, node->GetName(), node->GetPNid() );

                    // Spare node not activated
                    if ( MyNode->IsCreator() )
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                           trace_printf( "%s@%d" " - Sending spare up notice to creator shell(%d) spare node=%s, pnid=%d\n"
                                       , method_name, __LINE__, MyNode->GetCreatorPid(), node->GetName(), node->GetPNid() );
#ifndef NAMESERVER_PROCESS
                        // Tell creator spare node is up
                        SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                               , MyNode->GetCreatorVerifier()
                                                               , SpareUpMessage( node->GetName()
                                                                               , node->GetPNid() )
                                                               , NULL);
#endif
                    }
                }
            }

            if ( MyPNID == pnid )
            {
                // Any DTMs running?
                for ( int i=0; !tmCount && i < Nodes->GetPNodesCount(); i++ )
                {
                    CNode  *tempNode = Nodes->GetNodeByMap( i );
                    lnode = tempNode->GetFirstLNode();
                    for ( ; lnode; lnode = lnode->GetNextP() )
                    {
                        CProcess *process = lnode->GetProcessLByType( ProcessType_DTM );
                        if ( process  ) tmCount++;
                    }
                }
                if ( !tmCount && !spareNodeActivated )
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                       trace_printf( "%s@%d" " - Replicating node up %s, pnid=%d, state=%s, spare=%d, DTM count=%d\n"
                                   , method_name, __LINE__, node->GetName(), node->GetPNid()
                                   , StateString(node->GetState()), node->IsSpareNode(), tmCount );
                    // Let other monitors know this node is up
                    CReplNodeUp *repl = new CReplNodeUp(MyPNID);
                    Replicator.addItem(repl);
                }
            }

            ResetIntegratingPNid();

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
               trace_printf( "%s@%d" " - New monitor %s, pnid=%d, state=%s, spare=%d\n"
                           , method_name, __LINE__, node->GetName(), node->GetPNid()
                           , StateString(node->GetState()), node->IsSpareNode() );
        }
    }

    TRACE_EXIT;
    return( rc );
}

#ifdef NAMESERVER_PROCESS
int CCluster::HardNodeUpNs( int pnid )
{
    int     rc = 0;
    CNode  *node;
    STATE   nodeState;

    const char method_name[] = "CCluster::HardNodeUpNs";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
       trace_printf( "%s@%d - pnid=%d, MyPNID = %d, currentNodes_=%d\n"
                   , method_name, __LINE__, pnid, MyPNID, currentNodes_ );

    node = Nodes->GetNode( pnid );
    if ( node == NULL )
    {
        if ( rc )
        {   // Handle error
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Invalid node, pnid=%d\n"
                    , method_name, pnid );
            mon_log_write(MON_CLUSTER_HARDNODEUPNS_1, SQ_LOG_ERR, buf);
            return( -1 );
        }
    }

    nodeState = node->GetState();

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
       trace_printf( "%s@%d" " - Node state=%s" "\n"
                   , method_name, __LINE__, StateString( nodeState ) );

    if ( nodeState != State_Up )
    {
        if ( nodeState == State_Down )
        {
            node->SetKillingNode( false );
            // We need to remove any old process objects before we restart the node.
            node->CleanUpProcesses();
            node->SetState( State_Up );
            if ( MyPNID != pnid )
            {
                // Let other monitors know this node is up
                CReplNodeUp *repl = new CReplNodeUp(pnid);
                Replicator.addItem(repl);
            }
        }
    }
    else
    {   // Handle error
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Invalid node state, node %s, pnid=%d, state=%s\n"
                , method_name
                , node->GetName()
                , node->GetPNid()
                , StateString( nodeState ) );
        mon_log_write(MON_CLUSTER_HARDNODEUPNS_2, SQ_LOG_ERR, buf);
        return( -1 );
    }

    TRACE_EXIT;
    return( rc );
}
#endif


const char *StateString( STATE state)
{
    const char *str;

    switch( state )
    {
        case State_Unknown:
            str = "State_Unknown";
            break;
        case State_Up:
            str = "State_Up";
            break;
        case State_Down:
            str = "State_Down";
            break;
        case State_Stopped:
            str = "State_Stopped";
            break;
        case State_Shutdown:
            str = "State_Shutdown";
            break;
        case State_Unlinked:
            str = "State_Unlinked";
            break;
        case State_Merging:
            str = "State_Merging";
            break;
        case State_Merged:
            str = "State_Merged";
            break;
        case State_Joining:
            str = "State_Joining";
            break;
        case State_Initializing:
            str = "State_Initializing";
            break;
        default:
            str = "State - Undefined";
            break;
    }

    return( str );
}

const char *SyncStateString( SyncState state)
{
    const char *str;

    switch( state )
    {
        case SyncState_Null:
            str = "SyncState_Null";
            break;
        case SyncState_Start:
            str = "SyncState_Start";
            break;
        case SyncState_Continue:
            str = "SyncState_Continue";
            break;
        case SyncState_Abort:
            str = "SyncState_Abort";
            break;
        case SyncState_Commit:
            str = "SyncState_Commit";
            break;
        case SyncState_Suspended:
            str = "SyncState_Suspended";
            break;
        default:
            str = "SyncState - Undefined";
            break;
    }

    return( str );
}


#ifndef NAMESERVER_PROCESS
void CCluster::DoDeviceReq(char * ldevName)
{
    const char method_name[] = "CCluster::DoDeviceReq";
    TRACE_ENTRY;

    CProcess *process;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
        trace_printf("%s@%d - Internal device request for ldev %s\n",
                     method_name, __LINE__, ldevName);
    Nodes->GetLNode(ldevName, &process);
    if (!process)
    {
        if (trace_settings & TRACE_SYNC)
            trace_printf("%s@%d - Device processing but can't find device %s\n",
                         method_name, __LINE__, ldevName);
    }
    else
    {
        CLogicalDevice *ldev;
        ldev = Devices->GetLogicalDevice( ldevName );
        if ( !ldev )
        {   // The device name is not known on this node
            // we need to clone the device
            ldev = Devices->CloneDevice( process );
        }
        if ( ldev )
        {
            bool rstate = false;
            if ( ldev->Mounted() )
            {
                rstate = ldev->UnMount( false );
                if (!rstate)
                {
                    if (trace_settings & TRACE_REQUEST)
                        trace_printf("%s@%d - Can't unmount device %s for "
                                     "process %s (%d, %d)\n", method_name,
                                     __LINE__, ldev->name(), process->GetName(),
                                     process->GetNid(), process->GetPid());
                }
            }
            if ( rstate )
            {
                rstate = ldev->Mount( process, false );
                if (!rstate)
                {
                    if (trace_settings & TRACE_REQUEST)
                        trace_printf("%s@%d - Can't mount device %s for "
                                     "process %s (%d, %d)\n", method_name,
                                     __LINE__, ldev->name(), process->GetName(),
                                     process->GetNid(), process->GetPid());
                }
                else
                {
                    if (trace_settings & TRACE_REQUEST)
                        trace_printf("%s@%d - Mounted device %s for process "
                                     "%s (%d, %d)\n", method_name, __LINE__,
                                     ldev->name(), process->GetName(),
                                     process->GetNid(), process->GetPid());
                }
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], Can't find ldev %s.\n", method_name,
                    ldevName);
            mon_log_write(MON_CLUSTER_DODEVICEREQ_1, SQ_LOG_ERR, buf);
        }
    }

    TRACE_EXIT;
}
#endif

#ifdef EXCHANGE_CPU_SCHEDULING_DATA
void CCluster::SaveSchedData( struct internal_msg_def *recv_msg )
{
    const char method_name[] = "CCluster::SaveSchedData";
    TRACE_ENTRY;

    int nid = recv_msg->u.scheddata.PNid;
    Node[nid]->SetNumCores( recv_msg->u.scheddata.processors );
    Node[nid]->SetFreeMemory( recv_msg->u.scheddata.memory_free );
    Node[nid]->SetFreeSwap( recv_msg->u.scheddata.swap_free );
    Node[nid]->SetFreeCache( recv_msg->u.scheddata.cache_free );
    Node[nid]->SetMemTotal( recv_msg->u.scheddata.memory_total );
    Node[nid]->SetMemActive( recv_msg->u.scheddata.memory_active );
    Node[nid]->SetMemInactive( recv_msg->u.scheddata.memory_inactive );
    Node[nid]->SetMemDirty( recv_msg->u.scheddata.memory_dirty );
    Node[nid]->SetMemWriteback( recv_msg->u.scheddata.memory_writeback );
    Node[nid]->SetMemVMallocUsed( recv_msg->u.scheddata.memory_VMallocUsed );
    Node[nid]->SetBTime( recv_msg->u.scheddata.btime );

    CLNode *lnode;
    lnode = Node[nid]->GetFirstLNode();
    int i = 0;

    for ( ; lnode; lnode = lnode->GetNextP() )
    {
        lnode->SetCpuUser(recv_msg->u.scheddata.proc_stats[i].cpu_user);
        lnode->SetCpuNice(recv_msg->u.scheddata.proc_stats[i].cpu_nice);
        lnode->SetCpuSystem(recv_msg->u.scheddata.proc_stats[i].cpu_system);
        lnode->SetCpuIdle(recv_msg->u.scheddata.proc_stats[i].cpu_idle);
        lnode->SetCpuIowait(recv_msg->u.scheddata.proc_stats[i].cpu_iowait);
        lnode->SetCpuIrq(recv_msg->u.scheddata.proc_stats[i].cpu_irq);
        lnode->SetCpuSoftIrq(recv_msg->u.scheddata.proc_stats[i].cpu_soft_irq);

        ++i;
    }

    TRACE_EXIT;
}
#endif

void CCluster::HandleOtherNodeMsg (struct internal_msg_def *recv_msg,
                                   int pnid)
{
    const char method_name[] = "CCluster::HandleOtherNodeMsg";
    TRACE_ENTRY;

    CNode *downNode;
    CNode *spareNode;
#ifndef NAMESERVER_PROCESS
    CProcess *process;
    CLNode  *lnode;
#endif

    switch (recv_msg->type)
    {
    case InternalType_Null:
        if (trace_settings & TRACE_SYNC_DETAIL)
            trace_printf("%s@%d - Physical Node pnid=n%d has nothing to "
                         "update. \n", method_name, __LINE__, pnid);
        break;

    case InternalType_ActivateSpare:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal activate spare request, spare pnid=%d, down pnid=%d\n"
                        , method_name, __LINE__
                        , recv_msg->u.activate_spare.spare_pnid
                        , recv_msg->u.activate_spare.down_pnid);

        downNode = NULL;
        if ( recv_msg->u.activate_spare.down_pnid != -1 )
        {
            downNode = Nodes->GetNode( recv_msg->u.activate_spare.down_pnid );
        }
        spareNode = Nodes->GetNode( recv_msg->u.activate_spare.spare_pnid );
        ReqQueue.enqueueActivateSpareReq( spareNode, downNode );
        break;

    case InternalType_NameServerAdd:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Internal NameServer add request for node_name=%s\n"
                        , method_name, __LINE__
                        , recv_msg->u.nameserver_add.node_name );

        // Queue the nameserver add request for processing by a worker thread.
        ReqQueue.enqueueNameServerAddReq( recv_msg->u.nameserver_add.req_nid
                                        , recv_msg->u.nameserver_add.req_pid
                                        , recv_msg->u.nameserver_add.req_verifier
                                        , recv_msg->u.nameserver_add.node_name );
        break;

    case InternalType_NameServerDelete:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Internal NameServer delete request for node=%s\n"
                        , method_name, __LINE__, recv_msg->u.nameserver_delete.node_name);

        // Queue the nameserver delete request for processing by a worker thread.
        ReqQueue.enqueueNameServerDeleteReq( recv_msg->u.nameserver_delete.req_nid
                                           , recv_msg->u.nameserver_delete.req_pid
                                           , recv_msg->u.nameserver_delete.req_verifier
                                           , recv_msg->u.nameserver_delete.node_name );
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_NodeAdd:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Internal node add request for node_name=%s, "
                          "first_core=%d, last_core=%d, "
                          "processors=%d, roles=%d\n"
                        , method_name, __LINE__
                        , recv_msg->u.node_add.node_name
                        , recv_msg->u.node_add.first_core
                        , recv_msg->u.node_add.last_core
                        , recv_msg->u.node_add.processors
                        , recv_msg->u.node_add.roles );

        // Queue the node add request for processing by a worker thread.
        ReqQueue.enqueueNodeAddReq( recv_msg->u.node_add.req_nid
                                  , recv_msg->u.node_add.req_pid
                                  , recv_msg->u.node_add.req_verifier
                                  , recv_msg->u.node_add.node_name
                                  , recv_msg->u.node_add.first_core
                                  , recv_msg->u.node_add.last_core
                                  , recv_msg->u.node_add.processors
                                  , recv_msg->u.node_add.roles );
        break;
#endif

    case InternalType_Clone:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal clone request, process (%d, %d)"
                         " %s\n", method_name, __LINE__,
                         recv_msg->u.clone.nid, recv_msg->u.clone.os_pid,
                         (recv_msg->u.clone.backup?" Backup":""));

        ReqQueue.enqueueCloneReq( &recv_msg->u.clone );
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_Device:
        ReqQueue.enqueueDeviceReq(recv_msg->u.device.ldev_name);
        break;
#endif

    case InternalType_Shutdown:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal shutdown request for level=%d\n", method_name, __LINE__, recv_msg->u.shutdown.level);

        // Queue the shutdown request for processing by a worker thread.
        ReqQueue.enqueueShutdownReq( recv_msg->u.shutdown.level );
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_NodeDelete:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Internal node delete request for pnid=%d\n"
                        , method_name, __LINE__, recv_msg->u.node_delete.pnid);

        // Queue the node delete request for processing by a worker thread.
        ReqQueue.enqueueNodeDeleteReq( recv_msg->u.node_delete.req_nid
                                     , recv_msg->u.node_delete.req_pid
                                     , recv_msg->u.node_delete.req_verifier
                                     , recv_msg->u.node_delete.pnid );
        break;
#endif

    case InternalType_Down:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal down node request for pnid=%d\n", method_name, __LINE__, recv_msg->u.down.pnid);

        // Queue the node down request for processing by a worker thread.
        ReqQueue.enqueueDownReq( recv_msg->u.down.pnid );
        break;

    case InternalType_NodeName:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal node name request (%s to %s)\n", method_name, __LINE__, recv_msg->u.node_name.current_name, recv_msg->u.node_name.new_name);

        // Queue the node name request for processing by a worker thread.
        ReqQueue.enqueueNodeNameReq( recv_msg->u.node_name.req_nid
                                   , recv_msg->u.node_name.req_pid
                                   , recv_msg->u.node_name.req_verifier
                                   , recv_msg->u.node_name.current_name
                                   , recv_msg->u.node_name.new_name );
        break;

    case InternalType_Up:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal up node request for pnid=%d\n", method_name, __LINE__, recv_msg->u.up.pnid);

        // Queue the node up request for processing by a worker thread.
        ReqQueue.enqueueUpReq( recv_msg->u.up.pnid, NULL, -1 );
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_Dump:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal dump request for nid=%d, pid=%d\n",
                         method_name, __LINE__,
                         recv_msg->u.dump.nid, recv_msg->u.dump.pid);
        // Queue the dump request for processing by a worker thread.
        ReqQueue.enqueueDumpReq( &recv_msg->u.dump );
        break;

    case InternalType_DumpComplete:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal dump-complete request for nid=%d, pid=%d\n",
                         method_name, __LINE__,
                         recv_msg->u.dump.nid, recv_msg->u.dump.pid);
        // Queue the dump complete request for processing by a worker thread.
        ReqQueue.enqueueDumpCompleteReq( &recv_msg->u.dump );
        break;
#endif

    case InternalType_Exit:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal exit request for %s (%d, %d)\n", method_name, __LINE__, recv_msg->u.exit.name, recv_msg->u.exit.nid, recv_msg->u.exit.pid);
#ifndef NAMESERVER_PROCESS
        ReqQueue.enqueueExitReq( &recv_msg->u.exit );
#else
        ReqQueue.enqueueExitNsReq( &recv_msg->u.exit_ns );
#endif
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_Event:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal event request\n", method_name, __LINE__);
        // Queue the event request for processing by a worker thread.
        ReqQueue.enqueueEventReq( &recv_msg->u.event );
        break;
#endif

#ifndef NAMESERVER_PROCESS
    case InternalType_IoData:
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_REDIRECTION))
            trace_printf("%s@%d - Internal IO data request\n", method_name, __LINE__);
        if ( MyNode->IsMyNode(recv_msg->u.iodata.nid) )
        {
            if (trace_settings & (TRACE_SYNC | TRACE_REDIRECTION))
                trace_printf("%s@%d - processing IO Data for (%d, %d)\n", method_name, __LINE__, recv_msg->u.iodata.nid, recv_msg->u.iodata.pid);

            lnode = Nodes->GetLNode( recv_msg->u.iodata.nid );
            if ( lnode )
            {
                process = lnode->GetProcessL(recv_msg->u.iodata.pid);

                if (process)
                {
                    int fd;
                    if (recv_msg->u.iodata.ioType == STDIN_DATA)
                    {
                        fd = process->FdStdin();
                    }
                    else
                    {
                        fd = process->FdStdout();
                    }
                    Redirector.disposeIoData(fd,
                                             recv_msg->u.iodata.length,
                                             recv_msg->u.iodata.data);
                }
                else
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf(buf, sizeof(buf), "[%s], Can't find process nid"
                             "=%d, pid=%d for processing IO Data.\n",
                             method_name,
                             recv_msg->u.iodata.nid, recv_msg->u.iodata.pid);
                    mon_log_write(MON_CLUSTER_HANDLEOTHERNODE_7, SQ_LOG_ERR,
                                  buf);
                }
            }
        }
        break;
#endif

#ifndef NAMESERVER_PROCESS
    case InternalType_StdinReq:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal STDIN request\n", method_name, __LINE__);
        if ( !MyNode->IsMyNode(recv_msg->u.stdin_req.supplier_nid) )
        {
            break;

        }

        if (trace_settings & (TRACE_SYNC | TRACE_REDIRECTION))
            trace_printf("%s@%d - stdin request from (%d,%d)"
                         ", type=%d, for supplier (%d, %d)\n",
                         method_name, __LINE__,
                         recv_msg->u.stdin_req.nid,
                         recv_msg->u.stdin_req.pid,
                         recv_msg->u.stdin_req.reqType,
                         recv_msg->u.stdin_req.supplier_nid,
                         recv_msg->u.stdin_req.supplier_pid);

        lnode = Nodes->GetLNode( recv_msg->u.stdin_req.nid );
        if ( lnode == NULL )
        {
            break;
        }
        process = lnode->GetProcessL(recv_msg->u.stdin_req.pid);

        if (process)
        {
            if (recv_msg->u.stdin_req.reqType == STDIN_REQ_DATA)
            {
                // Set up to forward stdin data to requester.
                // Save file descriptor associated with stdin
                // so can find the redirector object later.
                CProcess *supProcess;
                lnode = Nodes->GetLNode( recv_msg->u.stdin_req.supplier_nid );
                if ( lnode )
                {
                    supProcess = lnode->GetProcessL ( recv_msg->u.stdin_req.supplier_pid );
                    if (supProcess)
                    {
                        int fd;
                        fd = Redirector.stdinRemote(supProcess->infile(),
                                                    recv_msg->u.stdin_req.nid,
                                                    recv_msg->u.stdin_req.pid);
                        process->FdStdin(fd);
                    }
                    else
                    {
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf(buf, sizeof(buf), "[%s], Can't find process "
                                 "nid=%d, pid=%d for stdin data request.\n",
                                 method_name,
                                 recv_msg->u.stdin_req.supplier_nid,
                                 recv_msg->u.stdin_req.supplier_pid);
                        mon_log_write(MON_CLUSTER_HANDLEOTHERNODE_8,
                                      SQ_LOG_DEBUG, buf);
                    }
                }
            }
            else if (recv_msg->u.stdin_req.reqType == STDIN_FLOW_OFF)
            {
                Redirector.stdinOff(process->FdStdin());
            }
            else if (recv_msg->u.stdin_req.reqType == STDIN_FLOW_ON)
            {
                Redirector.stdinOn(process->FdStdin());
            }
        }
        else
        {
            if (trace_settings 
               & (TRACE_SYNC | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL | TRACE_EVLOG_MSG))
            {
                trace_printf( "%s@%d - Can't find process nid=%d, "
                              "pid=%d for stdin data request.\n"
                            , method_name, __LINE__
                            , recv_msg->u.stdin_req.nid
                            , recv_msg->u.stdin_req.pid);
            }
        }
        break;
#endif

#ifndef NAMESERVER_PROCESS
    case InternalType_Kill:
        // Queue the kill request for processing by a worker thread.
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal kill request for (%d, %d), abort =%d\n", method_name, __LINE__, recv_msg->u.kill.nid, recv_msg->u.kill.pid, recv_msg->u.kill.persistent_abort);

        ReqQueue.enqueueKillReq( &recv_msg->u.kill );
        break;
#endif

    case InternalType_Process:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal process request\n", method_name, __LINE__);
        if ( MyNode->IsMyNode(recv_msg->u.process.nid) )
        {   // Need to create process on this node.
            // Queue process creation request for handling by worker thread
#ifdef NAMESERVER_PROCESS
            ReqQueue.enqueueNewProcNsReq( &recv_msg->u.process );
#endif
#ifndef NAMESERVER_PROCESS
            ReqQueue.enqueueNewProcReq( &recv_msg->u.process );
#endif
        }
        break;

    case InternalType_ProcessInit:
        if ( MyNode->IsMyNode(recv_msg->u.processInit.origNid) )
        {  // New process request originated on this node
            ReqQueue.enqueueProcInitReq( &recv_msg->u.processInit );
        }
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_Open:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal open request for (%d, %d), opened (%d, %d)\n", method_name, __LINE__, recv_msg->u.open.nid, recv_msg->u.open.pid, recv_msg->u.open.opened_nid, recv_msg->u.open.opened_pid);

        ReqQueue.enqueueOpenReq( &recv_msg->u.open );
        break;
#endif

    case InternalType_SchedData:
#ifdef EXCHANGE_CPU_SCHEDULING_DATA
        SaveSchedData( recv_msg );
#endif
        break;

    case InternalType_Set:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal set request\n", method_name, __LINE__);
        ReqQueue.enqueueSetReq( &recv_msg->u.set );
        break;

    case InternalType_UniqStr:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal unique string request\n", method_name, __LINE__);
        ReqQueue.enqueueUniqStrReq( &recv_msg->u.uniqstr );
        break;

    default:
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], Unknown Internal message received, Physical Node=%d.\n", method_name, pnid);
            mon_log_write(MON_CLUSTER_HANDLEOTHERNODE_11, SQ_LOG_ERR, buf);
        }
    }

    TRACE_EXIT;
}

void CCluster::HandleMyNodeMsg (struct internal_msg_def *recv_msg,
                                int pnid)
{
    const char method_name[] = "CCluster::HandleMyNodeMsg";
    TRACE_ENTRY;

    CNode *downNode;
    CNode *spareNode;
#ifndef NAMESERVER_PROCESS
    CProcess *process;
    CLNode  *lnode;
#endif

    switch (recv_msg->type)
    {

    case InternalType_Null:
        if (trace_settings & TRACE_SYNC_DETAIL)
            trace_printf("%s@%d - Physical Node pnid=n%d has nothing to "
                         "update. \n", method_name, __LINE__, pnid);
        break;

    case InternalType_ActivateSpare:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal activate spare request, spare pnid=%d, down pnid=%d\n"
                        , method_name, __LINE__
                        , recv_msg->u.activate_spare.spare_pnid
                        , recv_msg->u.activate_spare.down_pnid);
        break;

    case InternalType_NameServerAdd:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Internal NameServer add request for node_name=%s\n"
                        , method_name, __LINE__
                        , recv_msg->u.nameserver_add.node_name );

        // Queue the nameserver add request for processing by a worker thread.
        ReqQueue.enqueueNameServerAddReq( recv_msg->u.nameserver_add.req_nid
                                        , recv_msg->u.nameserver_add.req_pid
                                        , recv_msg->u.nameserver_add.req_verifier
                                        , recv_msg->u.nameserver_add.node_name );
        break;

    case InternalType_NameServerDelete:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Internal NameServer delete request for node=%s\n"
                        , method_name, __LINE__, recv_msg->u.nameserver_delete.node_name);

        // Queue the nameserver delete request for processing by a worker thread.
        ReqQueue.enqueueNameServerDeleteReq( recv_msg->u.nameserver_delete.req_nid
                                           , recv_msg->u.nameserver_delete.req_pid
                                           , recv_msg->u.nameserver_delete.req_verifier
                                           , recv_msg->u.nameserver_delete.node_name );
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_NodeAdd:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Internal node add request for node_name=%s, "
                          "first_core=%d, last_core=%d, "
                          "processors=%d, roles=%d\n"
                        , method_name, __LINE__
                        , recv_msg->u.node_add.node_name
                        , recv_msg->u.node_add.first_core
                        , recv_msg->u.node_add.last_core
                        , recv_msg->u.node_add.processors
                        , recv_msg->u.node_add.roles );

        // Queue the node add request for processing by a worker thread.
        ReqQueue.enqueueNodeAddReq( recv_msg->u.node_add.req_nid
                                  , recv_msg->u.node_add.req_pid
                                  , recv_msg->u.node_add.req_verifier
                                  , recv_msg->u.node_add.node_name
                                  , recv_msg->u.node_add.first_core
                                  , recv_msg->u.node_add.last_core
                                  , recv_msg->u.node_add.processors
                                  , recv_msg->u.node_add.roles );
        break;
#endif

    case InternalType_Clone:
#ifndef NAMESERVER_PROCESS
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal clone request, completed replicating process (%d, %d) %s\n", method_name, __LINE__, recv_msg->u.clone.nid, recv_msg->u.clone.os_pid, (recv_msg->u.clone.backup?" Backup":""));
#else        
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal clone request, process (%d, %d)"
                         " %s\n", method_name, __LINE__,
                         recv_msg->u.clone.nid, recv_msg->u.clone.os_pid,
                         (recv_msg->u.clone.backup?" Backup":""));

        ReqQueue.enqueueCloneReq( &recv_msg->u.clone );
#endif
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_Device:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal device request, completed device processing for ldev %s\n", method_name, __LINE__, recv_msg->u.device.ldev_name);
        break;
#endif

    case InternalType_Shutdown:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal shutdown request for level=%d\n", method_name, __LINE__, recv_msg->u.shutdown.level);

        // Queue the shutdown request for processing by a worker thread.
        ReqQueue.enqueueShutdownReq( recv_msg->u.shutdown.level );
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_NodeDelete:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf( "%s@%d - Internal node delete request for pnid=%d\n"
                        , method_name, __LINE__, recv_msg->u.node_delete.pnid);

        // Queue the node delete request for processing by a worker thread.
        ReqQueue.enqueueNodeDeleteReq( recv_msg->u.node_delete.req_nid
                                     , recv_msg->u.node_delete.req_pid
                                     , recv_msg->u.node_delete.req_verifier
                                     , recv_msg->u.node_delete.pnid );
        break;
#endif

    case InternalType_Down:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal down node request for pnid=%d\n", method_name, __LINE__, recv_msg->u.down.pnid);
        break;

    case InternalType_NodeName:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal node name request (%s to %s)\n", method_name, __LINE__, recv_msg->u.node_name.current_name, recv_msg->u.node_name.new_name);

        // Queue the node name request for processing by a worker thread.
        ReqQueue.enqueueNodeNameReq( recv_msg->u.node_name.req_nid
                                   , recv_msg->u.node_name.req_pid
                                   , recv_msg->u.node_name.req_verifier
                                   , recv_msg->u.node_name.current_name
                                   , recv_msg->u.node_name.new_name );
        break;
    case InternalType_Up:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal up node request for pnid=%d\n", method_name, __LINE__, recv_msg->u.up.pnid);
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_Dump:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal dump request for nid=%d, pid=%d\n",
                         method_name, __LINE__,
                         recv_msg->u.dump.nid, recv_msg->u.dump.pid);
        ReqQueue.enqueueDumpReq( &recv_msg->u.dump );
        break;

    case InternalType_DumpComplete:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal dump-complete request for nid=%d, pid=%d\n",
                         method_name, __LINE__,
                         recv_msg->u.dump.nid, recv_msg->u.dump.pid);
        ReqQueue.enqueueDumpCompleteReq( &recv_msg->u.dump );
        break;
#endif

    case InternalType_Exit:
        // Final process exit logic is done in Process_Exit, not here
        // as in the past.
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal exit request for %s (%d, %d)\n", method_name, __LINE__, recv_msg->u.exit_ns.name, recv_msg->u.exit_ns.nid, recv_msg->u.exit_ns.pid);
#ifdef NAMESERVER_PROCESS
        ReqQueue.enqueueExitNsReq( &recv_msg->u.exit_ns );
#endif
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_Event:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal event request\n", method_name, __LINE__);
        break;
#endif

#ifndef NAMESERVER_PROCESS
    case InternalType_IoData:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal IO data request\n", method_name, __LINE__);
        break;
#endif

#ifndef NAMESERVER_PROCESS
    case InternalType_StdinReq:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal STDIN request\n", method_name, __LINE__);
        break;
#endif

#ifndef NAMESERVER_PROCESS
    case InternalType_Kill:
        // Queue the kill request for processing by a worker thread.
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal kill request for (%d, %d), abort =%d\n", method_name, __LINE__, recv_msg->u.kill.nid, recv_msg->u.kill.pid, recv_msg->u.kill.persistent_abort);

        ReqQueue.enqueueKillReq( &recv_msg->u.kill );
        break;
#endif

    case InternalType_Process:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal process request, completed process replication for (%d, %d) %s\n", method_name,  __LINE__, recv_msg->u.process.pid, recv_msg->u.process.nid, (recv_msg->u.process.backup?" Backup":""));
        break;

    case InternalType_ProcessInit:
        // No action needed
        break;

#ifndef NAMESERVER_PROCESS
    case InternalType_Open:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal open request, completed open replication, "
                         "(%d, %d:%d)  opened (%d, %d:%d)\n",
                         method_name, __LINE__,
                         recv_msg->u.open.nid,
                         recv_msg->u.open.pid,
                         recv_msg->u.open.verifier,
                         recv_msg->u.open.opened_nid,
                         recv_msg->u.open.opened_pid,
                         recv_msg->u.open.opened_verifier);
        break;
#endif

    case InternalType_SchedData:
        // No action needed
        break;

    case InternalType_Set:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal set request, completed replicating key %s::%s\n", method_name, __LINE__, recv_msg->u.set.group, recv_msg->u.set.key);
        break;

    case InternalType_UniqStr:
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Internal unique string request, completed replicating (%d, %d)\n", method_name, __LINE__, recv_msg->u.uniqstr.nid, recv_msg->u.uniqstr.id);
        break;

    default:
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], Unknown Internal message received during processing local SyncType for pnid=%d.\n", method_name, pnid);
            mon_log_write(MON_CLUSTER_HANDLEMYNODE_6, SQ_LOG_ERR, buf);
        }

    }

    TRACE_EXIT;
}



bool CCluster::responsive()
{
    const char method_name[] = "CCluster::responsive";
    TRACE_ENTRY;

    static bool logEvent = false;
    int barrierDiff = barrierCount_ - barrierCountSaved_;
    struct timespec currTime;
    static struct timespec nextLogTime;

    // if no difference in barrier count, sync thread is not responsive
    if  ( !barrierDiff && isMonInitComplete() )
    {
        // this proc is called every SYNC_MAX_RESPONSIVE+1 secs
        cumulativeSyncDelay_ += CCluster::SYNC_MAX_RESPONSIVE + 1;

        monSyncResponsive_ = false; // sync thread is no longer responsive

        if ( CommType == CommType_InfiniBand )
        {
            // if sync thread is stuck in mpi call, one of the following checks will be true
            if ( inBarrier_ || inAllGather_ || inCommDup_ )
            {
                mem_log_write( MON_CLUSTER_RESPONSIVE_1
                             , cumulativeSyncDelay_
                             , inCommDup_   ? 4 :
                                inAllGather_ ? 2 : 
                                 /* inBarrier_ */ 1 );
            }
            else // non-mpi took quite long
            {
                mem_log_write(MON_CLUSTER_RESPONSIVE_2, cumulativeSyncDelay_);
            }
        }
        else
        {
            // if sync thread is stuck in mpi call
            if ( inBarrier_ )
            {
                mem_log_write(MON_CLUSTER_RESPONSIVE_1, cumulativeSyncDelay_, 
                              /* inBarrier_ */ 1);
            }
            else // non-mpi took quite long
            {
                mem_log_write(MON_CLUSTER_RESPONSIVE_2, cumulativeSyncDelay_);
            }
            
            if (!logEvent)
            {
                if (cumulativeSyncDelay_ > syncDelayLogEventThreshold_)
                {
                    logEvent = true;
                    clock_gettime(CLOCK_REALTIME, &currTime);
                    nextLogTime = currTime;
                }
            }
            else
            {
                clock_gettime(CLOCK_REALTIME, &currTime);
            }
#if 0
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - logEvent=%d, cumulativeSyncDelay_=%d, "
                              "currTime.tv_sec=%ld(secs), nextLogTime.tv_sec=%ld(secs)\n"
                            , method_name, __LINE__
                            , logEvent
                            , cumulativeSyncDelay_
                            , currTime.tv_sec
                            , nextLogTime.tv_sec );
            }
#endif
            if (logEvent && currTime.tv_sec >= nextLogTime.tv_sec)
            {
                int syncTimeoutCountDown = 
                    (HealthCheck.getSyncTimeout() - cumulativeSyncDelay_);
                
                nextLogTime.tv_sec = currTime.tv_sec + syncDelayLogEventInterval_;

                char buf[MON_STRING_BUF_SIZE];
                sprintf( buf
                       , "[%s], Sync thread not responsive, Allgather() "
                         "IO completion exceeded by %d seconds! "
                         "('Sync Thread Timeout' will occur in approximately %d "
                         "seconds and instance will go down "
                         "if not resolved)\n"
                       , method_name
                       , cumulativeSyncDelay_
                       , (syncTimeoutCountDown > 0) ? syncTimeoutCountDown : 0 );
                mon_log_write(MON_CLUSTER_RESPONSIVE_3, SQ_LOG_CRIT, buf);
            }
        }
    }
    else if (barrierDiff < syncMinPerSec_)
    {
        //logEvent = false;
        mem_log_write(MON_CLUSTER_RESPONSIVE_4, barrierDiff, syncMinPerSec_);
        cumulativeSyncDelay_ = 0;
        monSyncResponsive_ = true; // slow but responsive
    }
    else
    {
        logEvent = false;
        cumulativeSyncDelay_ = 0;
        monSyncResponsive_ = true; // truly responsive
    }

    barrierCountSaved_ = barrierCount_;
    if ( CommType == CommType_InfiniBand )
    {
        allGatherCountSaved_ = allGatherCount_;
        commDupCountSaved_ = commDupCount_;
    }

    TRACE_EXIT;

    return monSyncResponsive_;
}


int CCluster::MPIAllgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm Comm)
{
    const char method_name[] = "CCluster::MPIAllGather";
    TRACE_ENTRY;

    inAllGather_ = true;

    int rc = MPI_Allgather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, Comm);

    inAllGather_ = false;

    allGatherCount_++;

    TRACE_EXIT;
    return rc;
}

bool CCluster::ReinitializeConfigCluster( bool nodeAdded, int pnid )
{
    const char method_name[] = "CCluster::ReinitializeConfigCluster";
    TRACE_ENTRY;

    int     rs = true;
    CNode  *pnode;

    // Update node membership in the cluster

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d - Configured physical nodes count=%d\n"
                    , method_name, __LINE__
                    , GetConfigPNodesCount() );
    }

    if (nodeAdded)
    {
        // Add node to monitor's view of the cluster
        pnode = Nodes->AddNode( pnid );
        if ( !pnode )
        {
            rs = false;
        }
    }
    else
    {
        // Delete node from monitor's view of the cluster
        if ( !Nodes->DeleteNode( pnid ) )
        {
            rs = false;
        }
    }

    if ( rs )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf( "%s@%d - Updating cluster configuration, physical nodes count=%d, rs=%d\n"
                        , method_name, __LINE__
                        , GetConfigPNodesCount(), rs );
        }

        CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
        configPNodesCount_ = clusterConfig->GetPNodesCount();
        Nodes->UpdateCluster();
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d - Configured physical nodes count=%d, rs=%d\n"
                    , method_name, __LINE__
                    , GetConfigPNodesCount(), rs );
    }

    TRACE_EXIT;
    return( rs );
}

void CCluster::InitializeConfigCluster( void )
{
#ifndef NAMESERVER_PROCESS // nameserver is running in agent mode
    int rc;
#endif

    const char method_name[] = "CCluster::InitializeConfigCluster";
    TRACE_ENTRY;

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d (MasterMonitor) Node_name=%s, MyPNID=%d\n"
                    , method_name, __LINE__, Node_name, MyPNID );
    }

    int worldSize = 0;
    MPI_Comm_size (MPI_COMM_WORLD, &worldSize);
#ifdef NAMESERVER_PROCESS
    if ( !IsRealCluster )
    {
        char *nodes = getenv( "SQ_VIRTUAL_NODES" );
        worldSize = atoi(nodes);
        if ( worldSize <= 0 )
        {
            worldSize = 1;
        }
    }
#endif
    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    configPNodesCount_ = clusterConfig->GetPNodesCount();
    int rankToPnid[configPNodesCount_];

#ifdef NAMESERVER_PROCESS
    currentNodes_ = 1;  // non-master Name Servers join set through master Name Server
#else
    if (IAmIntegrating || IsAgentMode)
    {
        currentNodes_ = 1;  // non-master monitors join cluster through master monitor
    }
    else
    {
        currentNodes_ = worldSize;
    }
#endif

    if ( !IsRealCluster )
    {
        // Set virtual cluster size to collective size
        MPI_Comm_size (MPI_COMM_WORLD, &configPNodesCount_);
#ifdef NAMESERVER_PROCESS
        configPNodesCount_ = worldSize;
#endif

        // For virtual cluster set physical node id equal to rank
        for (int i=0; i<worldSize; ++i)
        {
            rankToPnid[i] = i;

            // Set bit indicating node is up
            upNodes_.upNodes[i/MAX_NODE_BITMASK] |= (1ull << (i%MAX_NODE_BITMASK));
        }
    }
    else
    {
        for (int i=0; i<configPNodesCount_; ++i)
        {
            rankToPnid[i] = i;
            rankToPnid[i] = rankToPnid[i];  // make compiler happy
        }
    }

    // Build the monitor's configured view of the cluster
    if ( IsRealCluster )
    {   // Map node name to physical node id
        // (for virtual nodes physical node equals "rank" (previously set))
        if (MyPNID == -1)
        {
            MyPNID = clusterConfig->GetPNid( Node_name );

            if (trace_settings & TRACE_INIT)
            {
                trace_printf( "%s@%d (MasterMonitor) Node_name=%s, MyPNID=%d\n"
                            , method_name, __LINE__, Node_name, MyPNID );
            }

            if (MyPNID == -1)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf), "[%s@%d] Can't find node name=%s in cluster configuration\n",
                         method_name, __LINE__, Node_name );
                mon_log_write(MON_CLUSTER_INITCONFIGCLUSTER_1, SQ_LOG_CRIT, buf);

                mon_failure_exit();
            }
        }
    }

    Nodes->AddNodes( );
    MyNode = Nodes->GetNode(MyPNID);
    Nodes->SetupCluster( &Node, &LNode, &indexToPnid_ );

    if ( CommType == CommType_Sockets )
    {
        InitServerSock();
    }

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d (MasterMonitor) IAmIntegrating=%s,"
                      " IsAgentMode=%s, IsMaster=%s, MasterMonitorName=%s,"
                      " Node_name=%s, MyNode Name=%s, MyPNID=%d\n"
                    , method_name, __LINE__
                    , IAmIntegrating?"TRUE":"FALSE"
                    , IsAgentMode?"TRUE":"FALSE", IsMaster?"TRUE":"FALSE"
                    , MasterMonitorName, Node_name, MyNode->GetName(), MyPNID );
    }

    if (IAmIntegrating || IsAgentMode)
    {
#ifndef NAMESERVER_PROCESS
        int TmLeaderPNid = -1;
        if (IsMaster)
        {
            tmLeaderNid_ = Nodes->GetFirstNid();
            TmLeaderPNid = LNode[tmLeaderNid_]->GetNode()->GetPNid();
        }
#endif
        // Monitors processes in AGENT mode in a real cluster initialize all
        // remote nodes to a down state. The master monitor and the joining
        // monitors will set the joining node state to up as part of the node
        // re-integration processing as monitor processes join the cluster
        // through the master.
        for (int i=0; i < clusterConfig->GetPNodesCount(); i++)
        {
            if (Node[indexToPnid_[i]])
            {
                if (Node[indexToPnid_[i]]->GetPNid() == MyPNID)
                { // Set bit indicating node is up
                    upNodes_.upNodes[indexToPnid_[i]/MAX_NODE_BITMASK] |=
                        (1ull << (indexToPnid_[i]%MAX_NODE_BITMASK));
                }
                else
                { // Set node state to down
                    Node[indexToPnid_[i]]->SetState( State_Down );
#ifndef NAMESERVER_PROCESS
                    if (IsMaster)
                    {
                        if (TmLeaderPNid == indexToPnid_[i])
                        {
                            AssignTmLeader(indexToPnid_[i], false);
                        }
                    }
#endif
                }
            }
        }
    }
#ifndef NAMESERVER_PROCESS // nameserver is running in agent mode
    else
    {
        char *nodeNames = 0;
        if ( IsRealCluster )
        {
            if (trace_settings & TRACE_INIT)
                trace_printf( "%s@%d Collecting port numbers and node names, "
                              "configPNodesCount_=%d, worldSize=%d, pnid=%d (%s:%s)\n"
                              "MyCommPort=%s\nMySyncPort=%s\n"
                             , method_name, __LINE__
                             , GetConfigPNodesCount(), worldSize
                             , MyPNID, MyNode->GetName(), MyNode->GetCommPort()
                             , MyCommPort, MySyncPort );

            bool nodeStatus[GetConfigPNodesCount()];
            for (int i=0; i<GetConfigPNodesCount(); ++i)
            {
                nodeStatus[i] = false;

                if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
                    trace_printf( "%s@%d - nodeStatus[%d]=%d\n"
                                , method_name, __LINE__, i, nodeStatus[i] ) ;
            }

            // Collect comm port info from other monitors
            char *commPortNums = new char[worldSize * MPI_MAX_PORT_NAME];
            rc = MPI_Allgather (MyCommPort, MPI_MAX_PORT_NAME, MPI_CHAR, commPortNums,
                                MPI_MAX_PORT_NAME, MPI_CHAR, MPI_COMM_WORLD);
            if (rc != MPI_SUCCESS)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf), "[%s@%d] MPI_Allgather error=%s\n",
                         method_name, __LINE__, ErrorMsg(rc));
                mon_log_write(MON_CLUSTER_INITCONFIGCLUSTER_2, SQ_LOG_CRIT, buf);

                mon_failure_exit();
            }

            // Collect sync port info from other monitors
            char *syncPortNums = new char[worldSize * MPI_MAX_PORT_NAME];
            rc = MPI_Allgather (MySyncPort, MPI_MAX_PORT_NAME, MPI_CHAR, syncPortNums,
                                MPI_MAX_PORT_NAME, MPI_CHAR, MPI_COMM_WORLD);
            if (rc != MPI_SUCCESS)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf), "[%s@%d] MPI_Allgather error=%s\n",
                         method_name, __LINE__, ErrorMsg(rc));
                mon_log_write(MON_CLUSTER_INITCONFIGCLUSTER_2, SQ_LOG_CRIT, buf);

                mon_failure_exit();
            }

            // Exchange Node Names with collective
            nodeNames = new char[worldSize * MPI_MAX_PROCESSOR_NAME];
            rc = MPI_Allgather (Node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
                                nodeNames, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
                                MPI_COMM_WORLD);
            if (rc != MPI_SUCCESS)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf), "[%s@%d] MPI_Allgather error=%s\n",
                         method_name, __LINE__, ErrorMsg(rc));
                mon_log_write(MON_CLUSTER_INITCONFIGCLUSTER_3, SQ_LOG_CRIT, buf);

                mon_failure_exit();
            }

            // For each node name received get corresponding CNode object and
            // store port number in it.
            char * nodeName;
            CNode * node;
            for (int i = 0; i < worldSize; i++)
            {
                nodeName = &nodeNames[ i * MPI_MAX_PROCESSOR_NAME ];
                node = Nodes->GetNode( nodeName );
                if ( node )
                {
                    node->SetCommPort( &commPortNums[ i * MPI_MAX_PORT_NAME] );
                    node->SetSyncPort( &syncPortNums[ i * MPI_MAX_PORT_NAME] );
                    rankToPnid[i] = node->GetPNid();
                    nodeStatus[rankToPnid[i]] = true;

                    if (trace_settings & TRACE_INIT)
                    {
                        trace_printf( "%s@%d rankToPnid[%d]=%d (%s:%s:%s)"
                                      "(node=%s,commPort=%s,syncPort=%s)\n"
                                    , method_name, __LINE__, i, rankToPnid[i]
                                    , node->GetName()
                                    , node->GetCommPort()
                                    , node->GetSyncPort()
                                    , &nodeNames[ i * MPI_MAX_PROCESSOR_NAME]
                                    , &commPortNums[ i * MPI_MAX_PORT_NAME]
                                    , &syncPortNums[ i * MPI_MAX_PORT_NAME]);
                    }
                }
                else
                {
                    rankToPnid[i] = -1;

                    // Unexpectedly could not map node name to CNode object
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf(buf, sizeof(buf), "[%s@%d] Unable to find node "
                             "object for node %s\n", method_name, __LINE__,
                             nodeName );
                    mon_log_write(MON_CLUSTER_INITCONFIGCLUSTER_4, SQ_LOG_CRIT, buf);
                }
            }
            delete [] commPortNums;
            delete [] syncPortNums;

            tmLeaderNid_ = Nodes->GetFirstNid();
            int TmLeaderPNid = LNode[tmLeaderNid_]->GetNode()->GetPNid();

            // Any nodes not in the initial MPI_COMM_WORLD are down.
            for (int i=0; i<GetConfigPNodesCount(); ++i)
            {
                if ( nodeStatus[indexToPnid_[i]] == false )
                {
                    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
                        trace_printf( "%s@%d - nodeStatus[%d]=%d"
                                      ", indexToPnid_[%d]=%d\n"
                                    , method_name, __LINE__
                                    , i, nodeStatus[i]
                                    , i, indexToPnid_[i] ) ;

                    node = Nodes->GetNode(indexToPnid_[i]);
                    if ( node ) node->SetState( State_Down );
                    // assign new TmLeader if TMLeader node is dead.
                    if (TmLeaderPNid == indexToPnid_[i])
                    {
                        AssignTmLeader(indexToPnid_[i], false);
                    }
                }
                else
                {   // Set bit indicating node is up

                    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
                        trace_printf( "%s@%d - nodeStatus[%d]=%d"
                                      ", indexToPnid_[%d]=%d\n"
                                    , method_name, __LINE__
                                    , i, nodeStatus[i]
                                    , i, indexToPnid_[i] ) ;

                    upNodes_.upNodes[indexToPnid_[i]/MAX_NODE_BITMASK] |=
                        (1ull << (indexToPnid_[i]%MAX_NODE_BITMASK));
                }
            }
        }
        else
        {
            tmLeaderNid_ = 0;
        }

        // Initialize communicators for point-to-point communications
        int myRank;
        MPI_Comm_rank( MPI_COMM_WORLD, &myRank );
        if ( !IsRealCluster )
            myRank = MyPNID;

        InitClusterComm(worldSize, myRank, rankToPnid);
        if ( CommType == CommType_Sockets )
        {
            InitClusterSocks(worldSize, myRank, nodeNames, rankToPnid);
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                for ( int i =0; i < worldSize; i++ )
                {
                    trace_printf( "%s@%d socks_[%d]=%d\n"
                                , method_name, __LINE__
                                , rankToPnid[i], socks_[rankToPnid[i]]);
                }
            }
        }

        if (nodeNames) delete [] nodeNames;
    }
#endif

    if ( CommType == CommType_Sockets )
    {
        // Allgather() cluster sockets are established as remote
        // monitor processes join the cluster
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            for ( int i =0; i < clusterConfig->GetPNodesCount() ; i++ )
            {
                trace_printf( "%s@%d %s (%d), state=%s, socks_[%d]=%d\n"
                            , method_name, __LINE__
                            , Node[indexToPnid_[i]]->GetName()
                            , Node[indexToPnid_[i]]->GetPNid()
                            , StateString(Node[indexToPnid_[i]]->GetState())
                            , indexToPnid_[i], socks_[indexToPnid_[i]]);
            }
        }
    }
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        for ( int i =0; i < MAX_NODE_MASKS ; i++ )
        {
            trace_printf( "%s@%d upNodes set[%d]: %llx\n"
                        , method_name, __LINE__
                        , i, upNodes_.upNodes[i]);
        }
    }

#ifndef NAMESERVER_PROCESS
    // Kill the MPICH hydra_pmi_proxy to prevent it from killing all
    // processes in cluster when mpirun or monitor processes are killed
    if (!IsAgentMode  || (IsAgentMode && IsMPIChild))
    {
        kill( getppid(), SIGKILL );
    }
#endif

    TRACE_EXIT;
}

void CCluster::InitializeConfigCluster( int pnid )
{
    const char method_name[] = "CCluster::InitializeConfigCluster";
    TRACE_ENTRY;

    Nodes->AddLNodes();

    // Set bit indicating node is up
    upNodes_.upNodes[pnid/MAX_NODE_BITMASK] |= 
        (1ull << (pnid%MAX_NODE_BITMASK));

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        for ( int i=0; i < MAX_NODE_MASKS ; i++ )
        {
            trace_printf( "%s@%d upNodes set[%d]: %llx\n"
                        , method_name, __LINE__
                        , i, upNodes_.upNodes[i]);
        }
    }

    // Refresh the pnid and nid maps
    Nodes->UpdateCluster();

    TRACE_EXIT;
}

void CCluster::InitClusterComm(int worldSize, int myRank, int * rankToPnid)
{
    const char method_name[] = "CCluster::InitClusterComm";
    TRACE_ENTRY;

    // Compute an array of "colors" for use with  MPI_Comm_split.
    int *splitColors;
    splitColors = new int[worldSize*worldSize*2];
    int *splitOtherNode;
    splitOtherNode = new int[worldSize*worldSize*2];
    int splitRows = 0;
    for ( int i=0; i<(worldSize*worldSize*2); ++i)
    {
        splitColors[i] = MPI_UNDEFINED;
        splitOtherNode[i] = -1;
    }

    int color = 1;
    bool placed;
    for (int i = 0; i < worldSize; i++)
    {
        for (int j = i+1; j < worldSize; j++)
        {
            // Find a free slot for rank "i" to rank "j"

            placed = false;
            for (int k=0; k<splitRows; ++k)
            {
                if (    splitColors[k*worldSize+i] == MPI_UNDEFINED
                     && splitColors[k*worldSize+j] == MPI_UNDEFINED )
                {
                    splitColors[k*worldSize+i] = color;
                    splitColors[k*worldSize+j] = color;
                    placed = true;

                    if (myRank == i)
                        splitOtherNode[k] = j;
                    else if (myRank == j)
                        splitOtherNode[k] = i;
                    break;
                }
            }
            if (!placed)
            {   // Need to use a new row
                splitColors[splitRows*worldSize+i] = color;
                splitColors[splitRows*worldSize+j] = color;

                if (myRank == i)
                    splitOtherNode[splitRows] = j;
                else if (myRank == j)
                    splitOtherNode[splitRows] = i;

                ++splitRows;
            }

            ++color;
        }
    }

    if (trace_settings & TRACE_INIT)
    {
        trace_printf("%s@%d Created %d splitRows for worldSize=%d, myRank=%d\n",
                     method_name, __LINE__, splitRows, worldSize, myRank);
        string line;
        char fragment[50];
        for (int i=0; i<splitRows; ++i)
        {
            sprintf(fragment, "%s@%d splitColors[%d]=", method_name, __LINE__,
                    i);
            line = fragment;
            for (int j=0; j<worldSize; ++j)
            {
                sprintf(fragment, " %d,", splitColors[i*worldSize+j]);
                line += fragment;
            }
            line += "\n";
            trace_printf(line.c_str());

            trace_printf("%s@%d splitOtherNode[%d]=%d\n", method_name,
                         __LINE__, i, splitOtherNode[i]);
        }
    }

    // Create one communicator for each other rank in MPI_COMM_WORLD
    // This permits point-to-point communication with each rank.
    int myRankInComm;
    MPI_Comm ncomm;
    int nid;

    for (int nSplit=0; nSplit < splitRows; ++nSplit)
    {
        color = splitColors[nSplit*worldSize+myRank];
        MPI_Comm_split(MPI_COMM_WORLD, color, myRank, &ncomm);
        if (ncomm == MPI_COMM_NULL)
        {
            if (splitColors[nSplit*worldSize+myRank] != MPI_UNDEFINED)
            {
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf("%s@%d Rank %d: Unexpected MPI_COMM_NULL from "
                                 "MPI_Comm_split, nSplit=%d\n",
                                 method_name, __LINE__,myRank, nSplit);
                }
            }
        }
        else
        {
            // Set comms_ (communicators) array element for the
            // physical node.
            nid = rankToPnid[splitOtherNode[nSplit]];
            comms_[nid] = ncomm;

            MPI_Comm_rank(ncomm, &myRankInComm);
            otherMonRank_[nid] = (myRankInComm == 0)? 1: 0;

            if (trace_settings & TRACE_INIT)
            {
                trace_printf("%s@%d Rank %d: MPI_Comm_split %d, color=%d, "
                             "comms_[%d] is orig rank #%d, "
                             "otherMonRank_=%d\n",
                             method_name, __LINE__,
                             myRank, nSplit, color,
                             nid, splitOtherNode[nSplit],
                             otherMonRank_[nid]);
            }
        }
    }

    delete [] splitColors;
    delete [] splitOtherNode;

    TRACE_EXIT;
}

void CCluster::HandleReintegrateError( int rc, int err,
                                       int pnid, nodeId_t *nodeInfo,
                                       bool abortIn )
{
    const char method_name[] = "CCluster::HandleReintegrateError";
    TRACE_ENTRY;

    char buf[MON_STRING_BUF_SIZE];

    switch ( err )
    {
    case Reintegrate_Err1:
        snprintf(buf, sizeof(buf), "[%s], can't to connect to creator monitor"
                 " port: %s - Error: %s.\n",
                 method_name, IntegratingMonitorPort, ErrorMsg(rc));
        break;

    case Reintegrate_Err2:
        snprintf(buf, sizeof(buf), "[%s], can't merge intercomm to existing "
                 "MPI collective - Error: %s.\n",
                 method_name, ErrorMsg(rc));

        break;

    case Reintegrate_Err3:
        snprintf(buf, sizeof(buf), "[%s], unable to obtain cluster info "
                 "from creator monitor: %s.\n", method_name, ErrorMsg(rc));
        break;

    case Reintegrate_Err4:
        snprintf(buf, sizeof(buf), "[%s], Failed to send name/port "
                 "to node %d (%s): %s.\n", method_name, pnid,
                 nodeInfo->nodeName, ErrorMsg(rc));
        break;

    case Reintegrate_Err5:
        snprintf(buf, sizeof(buf), "[%s], can't to connect to "
                 " node %d monitor, commPort=%s, syncPort=%s: %s.\n",
                 method_name, pnid, nodeInfo->commPort,
                 nodeInfo->syncPort, ErrorMsg(rc));
        break;

    case Reintegrate_Err6:
        snprintf(buf, sizeof(buf), "[%s], can't merge intercomm "
                 "for node %d: %s.\n", method_name, pnid,
                 ErrorMsg(rc));
        break;

    case Reintegrate_Err7:
        snprintf(buf, sizeof(buf), "[%s], can't disconnect "
                 "intercomm for node %d: %s.\n", method_name, pnid,
                 ErrorMsg(rc));
        break;

    case Reintegrate_Err8:
        snprintf(buf, sizeof(buf), "[%s], Failed to send status to creator "
                 "monitor: %s\n", method_name, ErrorMsg(rc));
        break;

    case Reintegrate_Err9:
        snprintf(buf, sizeof(buf), "[%s], Failed to send name/port "
                 "to creator monitor: %s.\n", method_name, ErrorMsg(rc));
        break;

    case Reintegrate_Err10:
        snprintf(buf, sizeof(buf), "[%s], Monitor initialization failed (could"
                 " not write to port file).  Aborting.\n", method_name);
        break;

    case Reintegrate_Err11:
        snprintf(buf, sizeof(buf), "[%s], Monitor initialization failed (could"
                 " not open port file).  Aborting.\n", method_name);
        break;

    case Reintegrate_Err12:
        snprintf(buf, sizeof(buf), "[%s], Monitor initialization failed (could"
                 " not initialize local io).  Aborting.\n", method_name);
        break;

    case Reintegrate_Err13:
        snprintf(buf, sizeof(buf), "[%s], Monitor initialization failed (could"
                 " not initialize devices).  Aborting.\n", method_name);
        break;

    case Reintegrate_Err14:
        snprintf(buf, sizeof(buf), "[%s] Aborting.\n", method_name);
        break;

    case Reintegrate_Err15:
        snprintf(buf, sizeof(buf), "[%s], no connect acknowledgement "
                 "for node %d: %s.\n", method_name, pnid,
                 ErrorMsg(rc));
        break;

    default:
        snprintf(buf, sizeof(buf), "[%s], Reintegration error: %s\n",
                 method_name, ErrorMsg(rc));
    }

    mon_log_write(MON_CLUSTER_REINTEGRATE_1, SQ_LOG_ERR, buf);

    if ( abortIn )
    {
        abort();
    }

    TRACE_EXIT;
}

void CCluster::SendReIntegrateStatus( STATE nodeState, int initErr )
{
    const char method_name[] = "CCluster::SendReIntegrateStatus";
    TRACE_ENTRY;

    int rc;
    nodeStatus_t nodeStatus;
    nodeStatus.state = nodeState;
    nodeStatus.status = initErr;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Sending reintegrate status: state=%s, error=%d\n"
                    , method_name, __LINE__
                    , StateString(nodeStatus.state)
                    , nodeStatus.status );
    }

    switch( CommType )
    {
        case CommType_InfiniBand:
            rc = Monitor->SendMPI( (char *) &nodeStatus
                                 , sizeof(nodeStatus_t)
                                 , 0
                                 , MON_XCHNG_DATA
                                 , joinComm_ );
            if ( rc )
            {
                HandleReintegrateError( rc, Reintegrate_Err8, -1, NULL );
            }
            break;
        case CommType_Sockets:
            rc = Monitor->SendSock( (char *) &nodeStatus
                                  , sizeof(nodeStatus_t)
                                  , joinSock_
                                  , method_name );
            if ( rc )
            {
                HandleReintegrateError( rc, Reintegrate_Err8, -1, NULL );
            }
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    if ( nodeState != State_Up )
    {  // Initialization error

        mem_log_write(CMonLog::MON_REINTEGRATE_9, MyPNID, initErr);
        HandleReintegrateError( rc, initErr, -1, NULL );
    }

    TRACE_EXIT;
}

bool CCluster::PingSockPeer( CNode *node, struct timespec &peerZnodeFailTime )
{
    const char method_name[] = "CCluster::PingSockPeer";
    TRACE_ENTRY;

    static int sv_connect_wait_timeout = -2;
    static int sv_connect_retry_count = 1;
    if ( sv_connect_wait_timeout == -2 )
    {
        // Use the EPOLL timeout and retry values
        char *lv_connect_wait_timeout_env = getenv( "SQ_MON_EPOLL_WAIT_TIMEOUT" );
        if ( lv_connect_wait_timeout_env )
        {
            // Timeout in seconds
            sv_connect_wait_timeout = atoi( lv_connect_wait_timeout_env );
            char *lv_connect_retry_count_env = getenv( "SQ_MON_EPOLL_RETRY_COUNT" );
            if ( lv_connect_retry_count_env )
            {
                sv_connect_retry_count = atoi( lv_connect_retry_count_env );
            }
            if ( sv_connect_retry_count > 180 )
            {
                sv_connect_retry_count = 180;
            }
        }
        else
        {
            // default to 64 seconds
            sv_connect_wait_timeout = 16;
            sv_connect_retry_count = 4;
        }
    }

    bool createErrorZNode = true;
    int  pingSock = -1;
    struct timespec currentTime;

    if (MyNode->IsPendingNodeDown())
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - MyNode %s (%d) is going down, "
                          "socks_[%d]=%d, state=%s, pendingNodeDown=%d\n"
                        , method_name, __LINE__
                        , MyNode->GetName(), MyNode->GetPNid()
                        , MyNode->GetPNid(), socks_[MyNode->GetPNid()]
                        , StateString(MyNode->GetState())
                        , MyNode->IsPendingNodeDown() );
        }
        return( false );
    }

    char buf[MON_STRING_BUF_SIZE];
    snprintf( buf, sizeof(buf)
            , "[%s@%d] Pinging remote monitor %s, pnid=%d\n"
            , method_name,  __LINE__
            , node->GetName(), node->GetPNid() );

    mon_log_write( MON_PINGSOCKPEER_1, SQ_LOG_INFO, buf );

    // Attempt to connect with remote monitor in one seconds increments
    // to recover as quickly as possible or give up trying
    for (int i = 0; i < (sv_connect_retry_count*sv_connect_wait_timeout); i++ )
    {
        // Disable connect internal retries
        pingSock = Monitor->Connect( node->GetCommPort(), false );
        if ( pingSock < 0 )
        {
            clock_gettime(CLOCK_REALTIME, &currentTime);
            if (node->GetState() != State_Up || node->IsPendingNodeDown())
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - Node %s (%d) is not up, "
                                  "socks_[%d]=%d, state=%s, pendingNodeDown=%d\n"
                                , method_name, __LINE__
                                , node->GetName(), node->GetPNid()
                                , node->GetPNid(), socks_[node->GetPNid()]
                                , StateString(node->GetState())
                                , node->IsPendingNodeDown() );
                }
                break;
            }
            else if (currentTime.tv_sec > peerZnodeFailTime.tv_sec)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s@%d] Connect exceeded session timeout to remote "
                          "monitor %s, pnid=%d, retry=%d, "
                          "currentTime=%ld(secs), peerZnodeFailTime=%ld(secs)\n"
                        , method_name,  __LINE__
                        , node->GetName(), node->GetPNid(), i 
                        , currentTime.tv_sec
                        , peerZnodeFailTime.tv_sec );
                mon_log_write( MON_PINGSOCKPEER_2, SQ_LOG_WARNING, buf );
                if (ZClientEnabled && createErrorZNode)
                {
                    if (node->GetState() == State_Up)
                    {
                        ZClient->ErrorZNodeCreate( node->GetName() );
                    }
                    else
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d Node %s is not up, state=%s\n"
                                        , method_name, __LINE__
                                        , node->GetName()
                                        , StateString(node->GetState()) );
                        }
                    }
                    createErrorZNode = false;
                }
                break;
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s@%d] Retrying connect to remote monitor %s, pnid=%d, retry=%d\n"
                        , method_name,  __LINE__
                        , node->GetName(), node->GetPNid(), (i+1) );
                mon_log_write( MON_PINGSOCKPEER_3, SQ_LOG_INFO, buf );
                sleep( 1 );
            }
        }
        else
        {
            break;
        }
    }
    if ( pingSock < 0 )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Can't connect to remote monitor %s, pnid=%d\n"
                        , method_name, __LINE__
                        , node->GetName(), node->GetPNid() );
        }
        return(false);
    }

    int rc = MPI_SUCCESS;
    nodeId_t nodeInfo;

    nodeInfo.pnid = MyPNID;
    strcpy(nodeInfo.nodeName, MyNode->GetName());
    strcpy(nodeInfo.commPort, MyNode->GetCommPort());
    strcpy(nodeInfo.syncPort, MyNode->GetSyncPort());
    nodeInfo.ping = true;
    nodeInfo.creatorPNid = -1;
    nodeInfo.creator = false;
    nodeInfo.creatorShellPid = -1;
    nodeInfo.creatorShellVerifier = -1;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Sending my nodeInfo.pnid=%d\n"
                      "        nodeInfo.nodeName=%s\n"
                      "        nodeInfo.commPort=%s\n"
                      "        nodeInfo.syncPort=%s\n"
                      "        nodeInfo.creatorPNid=%d\n"
                      "        nodeInfo.creator=%d\n"
                      "        nodeInfo.creatorShellPid=%d\n"
                      "        nodeInfo.creatorShellVerifier=%d\n"
                      "        nodeInfo.ping=%d\n"
                    , method_name, __LINE__
                    , nodeInfo.pnid
                    , nodeInfo.nodeName
                    , nodeInfo.commPort
                    , nodeInfo.syncPort
                    , nodeInfo.creatorPNid
                    , nodeInfo.creator
                    , nodeInfo.creatorShellPid
                    , nodeInfo.creatorShellVerifier
                    , nodeInfo.ping );
    }

    rc = SendSock( (char *) &nodeInfo
                 , sizeof(nodeId_t)
                 , pingSock
                 , method_name );
    if ( rc )
    {
        shutdown( pingSock, SHUT_RDWR);
        close( (int)pingSock );

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Cannot send my node info to node %s: (%s)\n"
                , method_name
                , node?node->GetName():"", ErrorMsg(rc));
        mon_log_write(MON_PINGSOCKPEER_4, SQ_LOG_ERR, buf);
        return(false);
    }
    else
    {
        // Get info about connecting monitor
        rc = ReceiveSock( (char *) &nodeInfo
                        , sizeof(nodeId_t)
                        , pingSock
                        , method_name );
        if ( rc )
        {   // Handle error
            shutdown( pingSock, SHUT_RDWR);
            close( (int)pingSock );

            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], unable to obtain node sync info from remote"
                      "monitor: %s.\n"
                    , method_name, ErrorMsg(rc));
            mon_log_write(MON_PINGSOCKPEER_5, SQ_LOG_ERR, buf);    
            return(false);
        }
        else
        {
            if (ZClientEnabled)
            {
                int zerr;
                if ( ZClient->IsRunningZNodeExpired( node->GetName(), zerr ) )
                {   // Handle znode expiration
                    shutdown( pingSock, SHUT_RDWR);
                    close( (int)pingSock );
        
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], Ping successful, but znode expired on "
                              "node: %s.\n"
                            , method_name, zerror(zerr));
                    mon_log_write(MON_PINGSOCKPEER_6, SQ_LOG_ERR, buf);    
                    return(false);
                }
            }
        
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Received from nodeInfo.pnid=%d\n"
                              "        nodeInfo.nodeName=%s\n"
                              "        nodeInfo.commPort=%s\n"
                              "        nodeInfo.syncPort=%s\n"
                              "        nodeInfo.ping=%d\n"
                            , method_name, __LINE__
                            , nodeInfo.pnid
                            , nodeInfo.nodeName
                            , nodeInfo.commPort
                            , nodeInfo.syncPort
                            , nodeInfo.ping );
            }
        }
    }

    shutdown( pingSock, SHUT_RDWR);
    close( pingSock );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Ping success to remote monitor %s, pnid=%d\n"
                    , method_name, __LINE__
                    , node->GetName(), node->GetPNid() );
    }

    if (ZClientEnabled)
    {
        // Clean up error znodes and where I am their 'only' child
        ZClient->HandleErrorChildZNodesForZNodeChild( Node_name, false );
    }

    TRACE_EXIT;
    return( true );
}

void CCluster::ReIntegrate( int initProblem )
{
    const char method_name[] = "CCluster::ReIntegrate";
    TRACE_ENTRY;

    switch( CommType )
    {
        case CommType_InfiniBand:
            ReIntegrateMPI( initProblem );
            break;
        case CommType_Sockets:
            ReIntegrateSock( initProblem );
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    TRACE_EXIT;
}

void CCluster::ReIntegrateMPI( int initProblem )
{
    const char method_name[] = "CCluster::ReIntegrateMPI";
    TRACE_ENTRY;

    int rc;
    bool haveCreatorComm = false;
    MPI_Comm interComm;
    MPI_Comm intraComm = MPI_COMM_NULL;
    MPI_Comm intraCommCreatorMon = MPI_COMM_NULL;

    nodeId_t myNodeInfo;
    strcpy(myNodeInfo.nodeName, MyNode->GetName());
    strcpy(myNodeInfo.commPort, MyNode->GetCommPort());
    // Set bit indicating my node is up
    upNodes_.upNodes[MyPNID/MAX_NODE_BITMASK] |= (1ull << (MyPNID%MAX_NODE_BITMASK));

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
       trace_printf("%s@%d - Connect to creator monitor (port %s)\n",
                    method_name, __LINE__, IntegratingMonitorPort);

    mem_log_write(CMonLog::MON_REINTEGRATE_1, MyPNID);

    if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
    {
        for ( int i =0; i < MAX_NODE_MASKS ; i++ )
        {
            trace_printf( "%s@%d Integrating node %s (pnid=%d) "
                          "sees set[%d]: %llx\n"
                        , method_name, __LINE__
                        , MyNode->GetName(), MyPNID
                        , i, upNodes_.upNodes[i] );
        }
    }

    TEST_POINT( TP010_NODE_UP );
    // Connect with my creator monitor
    rc = MPI_Comm_connect( IntegratingMonitorPort,
                           MPI_INFO_NULL, 0, MPI_COMM_SELF, &joinComm_ );
    if ( rc )
    {
        HandleReintegrateError( rc, Reintegrate_Err1, -1, NULL );
    }

    MPI_Comm_set_errhandler( joinComm_, MPI_ERRORS_RETURN );

    mem_log_write(CMonLog::MON_REINTEGRATE_4, MyPNID);

    TEST_POINT( TP011_NODE_UP );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d Connected to creator monitor, sending id\n",
                     method_name, __LINE__);
    }

    // Send this node's name and port number so creator monitor
    // knows who we are, and set flag to let creator monitor it is the CREATOR.
    myNodeInfo.creator = true;
    myNodeInfo.creatorShellPid = CreatorShellPid;
    myNodeInfo.creatorShellVerifier = CreatorShellVerifier;
    if ((rc = Monitor->SendMPI((char *) &myNodeInfo, sizeof(nodeId_t), 0,
                            MON_XCHNG_DATA, joinComm_)))
        HandleReintegrateError( rc, Reintegrate_Err9, -1, NULL );

    TEST_POINT( TP012_NODE_UP );

    // Merge the inter-communicators obtained from the connect/accept
    // between this new monitor and the creator monitor.
    if ((rc = MPI_Intercomm_merge( joinComm_, 1, &intraCommCreatorMon )))
        HandleReintegrateError( rc, Reintegrate_Err2, -1, NULL );

    MPI_Comm_set_errhandler( intraCommCreatorMon, MPI_ERRORS_RETURN );

    nodeId_t *nodeInfo = new nodeId_t[GetConfigPNodesCount()];

    mem_log_write(CMonLog::MON_REINTEGRATE_3, MyPNID);

    // Obtain node names & port numbers of existing monitors from
    // the creator monitor.
    if ((rc = Monitor->ReceiveMPI((char *)nodeInfo, sizeof(nodeId_t)*GetConfigPNodesCount(),
                               MPI_ANY_SOURCE, MON_XCHNG_DATA, joinComm_)))
        HandleReintegrateError( rc, Reintegrate_Err3, -1, NULL );

    if ( initProblem )
    {
        // The monitor encountered an initialization error.  Inform
        // the creator monitor that the node is down.  Then exit.
        SendReIntegrateStatus( State_Down, initProblem );
    }

    // Connect to each of the other existing monitors and let them know
    // we are the NEW monitor and reset the creator flag so they know they are
    // not the creator monitor.
    myNodeInfo.creator = false;
    myNodeInfo.creatorShellPid = -1;
    myNodeInfo.creatorShellVerifier = -1;
    for (int i = 0; i < GetConfigPNodesCount(); i++)
    {
        if (strcmp(nodeInfo[i].commPort, IntegratingMonitorPort) == 0)
        {   // Already connected to creator monitor
            comms_[i] = intraCommCreatorMon;
            otherMonRank_[i] = 0;
            ++currentNodes_;

            // Set bit indicating node is up
            upNodes_.upNodes[i/MAX_NODE_BITMASK] |= (1ull << (i%MAX_NODE_BITMASK));

            Node[i]->SetCommPort( IntegratingMonitorPort );
            Node[i]->SetState( State_Up );
            haveCreatorComm = true;
        }
        else if (nodeInfo[i].nodeName[0] != 0
                 && nodeInfo[i].commPort[0] != 0)
        {
            if ( haveCreatorComm && i >= GetConfigPNodesCount()/2)
                // Reintegration failure after connecting to half
                // of existing monitors.
                TEST_POINT( TP016_NODE_UP );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d Attempting connection to node %d (%s), "
                             "port %s\n", method_name, __LINE__, i,
                             nodeInfo[i].nodeName, nodeInfo[i].commPort);
            }

            mem_log_write(CMonLog::MON_REINTEGRATE_5, MyPNID, i);

            TEST_POINT( TP013_NODE_UP );

            // Connect to existing monitor
            if ((rc = MPI_Comm_connect( nodeInfo[i].commPort,
                                        MPI_INFO_NULL, 0, MPI_COMM_SELF,
                                        &interComm )))
            {
                HandleReintegrateError( rc, Reintegrate_Err5, i, &nodeInfo[i] );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }

            MPI_Comm_set_errhandler( interComm, MPI_ERRORS_RETURN );

            TEST_POINT( TP014_NODE_UP );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d Connected to node %d (%s), sending id\n",
                             method_name, __LINE__,i,nodeInfo[i].nodeName);
            }

            // Send this nodes name and port number so other monitor
            // knows who we are.
            if ((rc = Monitor->SendMPI((char *) &myNodeInfo, sizeof(nodeId_t), 0,
                                    MON_XCHNG_DATA, interComm)))
            {
                HandleReintegrateError( rc, Reintegrate_Err4, i, &nodeInfo[i] );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }

            if ((rc = MPI_Intercomm_merge(interComm, 1, &intraComm)))
            {
                HandleReintegrateError( rc, Reintegrate_Err6, i, NULL );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }

            // Get acknowledgement that other monitor is ready to
            // integrate this node.  This is an interlock to avoid a
            // race condition where the creator monitor could signal
            // the monitors in the cluster to integrate the new node
            // before one or more was ready to do the integration.
            int readyFlag;
            if ((rc = Monitor->ReceiveMPI((char *) &readyFlag, sizeof(readyFlag),
                                       MPI_ANY_SOURCE, MON_XCHNG_DATA,
                                       interComm)))
            {
                HandleReintegrateError( rc, Reintegrate_Err15, i, NULL );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }


            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Received ready-flag from node %d (%s)\n",
                              method_name, __LINE__, i,
                             nodeInfo[i].nodeName);
            }

            if ((rc = MPI_Comm_disconnect(&interComm)))
                HandleReintegrateError( rc, Reintegrate_Err7, i, NULL );

            MPI_Comm_set_errhandler(intraComm, MPI_ERRORS_RETURN);

            comms_[i] = intraComm;
            otherMonRank_[i] = 0;
            ++currentNodes_;
            Node[i]->SetSyncPort( nodeInfo[i].syncPort );
            Node[i]->SetState( State_Up );

            // Set bit indicating node is up
            upNodes_.upNodes[i/MAX_NODE_BITMASK] |= (1ull << (i%MAX_NODE_BITMASK));

            mem_log_write(CMonLog::MON_REINTEGRATE_6, MyPNID, i);
        }
        else if ( i != MyPNID)
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d Connection to node %d not attempted, "
                             "no port information.  nodeInfo[%d].port=%s, "
                             "IntegratingMonitorPort=%s\n", method_name,
                             __LINE__, i, i, nodeInfo[i].commPort,
                             IntegratingMonitorPort);
            }
        }
    }

    if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
    {
        for ( int i =0; i < MAX_NODE_MASKS ; i++ )
        {
            trace_printf( "%s@%d Integrating node %s (pnid=%d) "
                          "sees set[%d]: %llx\n"
                        , method_name, __LINE__
                        , MyNode->GetName(), MyPNID
                        , i, upNodes_.upNodes[i] );
        }
    }

    mem_log_write(CMonLog::MON_REINTEGRATE_7, MyPNID);

    TEST_POINT( TP015_NODE_UP );

    // Inform creator monitor that connections are complete and
    // this monitor is ready to participate in "allgather"
    // communications with the other monitors.
    SendReIntegrateStatus( State_Up, 0 );

    mem_log_write(CMonLog::MON_REINTEGRATE_8, MyPNID);

    MyNode->SetState( State_Merged );

    delete[] nodeInfo;

    TRACE_EXIT;
}

void CCluster::ReIntegrateSock( int initProblem )
{
    const char method_name[] = "CCluster::ReIntegrateSock";
    TRACE_ENTRY;

    bool haveCreatorSocket = false;
    int rc;
    int existingCommFd;
    int existingSyncFd;
    char commPort[MPI_MAX_PORT_NAME];
    char syncPort[MPI_MAX_PORT_NAME];
    char *pch1;
    char *pch2;

    // Set bit indicating my node is up
    upNodes_.upNodes[MyPNID/MAX_NODE_BITMASK] |= (1ull << (MyPNID%MAX_NODE_BITMASK));

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
       trace_printf("%s@%d - Connect to creator monitor (port %s)\n",
                    method_name, __LINE__, IntegratingMonitorPort);

    mem_log_write(CMonLog::MON_REINTEGRATE_1, MyPNID);

    if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
    {
        for ( int i =0; i < MAX_NODE_MASKS ; i++ )
        {
            trace_printf( "%s@%d Integrating node %s (pnid=%d) "
                          "sees set[%d]: %llx\n"
                        , method_name, __LINE__
                        , MyNode->GetName(), MyPNID
                        , i, upNodes_.upNodes[i] );
        }
    }

    TEST_POINT( TP010_NODE_UP );

    // Connect with my creator monitor
    bool lv_done = false;
    bool lv_did_not_connect_in_first_attempt = false;
    while ( ! lv_done )
    {
        joinSock_ = Monitor->Connect( IntegratingMonitorPort );
        if ( joinSock_ < 0 )
        {
            if ( IsAgentMode )
            {
                lv_did_not_connect_in_first_attempt = true;
                sleep( 15 );
            }
            else
            {
                HandleReintegrateError( joinSock_, Reintegrate_Err1, -1, NULL );
            }
        }
        else
        {
            if ( lv_did_not_connect_in_first_attempt )
            {
                sleep( 10 );
            }
            lv_done = true;
        }
    }

    mem_log_write(CMonLog::MON_REINTEGRATE_4, MyPNID);

    TEST_POINT( TP011_NODE_UP );

    // Send this node's name and port number so creator monitor
    // knows who we are, and set flag to let creator monitor it is the CREATOR.
    nodeId_t myNodeInfo;
    strcpy(myNodeInfo.nodeName, MyNode->GetName());
    strcpy(myNodeInfo.commPort, MyNode->GetCommPort());
    strcpy(myNodeInfo.syncPort, MyNode->GetSyncPort());
    myNodeInfo.pnid = MyNode->GetPNid();
    myNodeInfo.creatorPNid = -1;
    myNodeInfo.creator = true;
    myNodeInfo.creatorShellPid = CreatorShellPid;
    myNodeInfo.creatorShellVerifier = CreatorShellVerifier;
    myNodeInfo.ping = false;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Connected to creator monitor, sending my info:\n"
                      "        myNodeInfo.pnid=%d (%s)\n"
                      "        myNodeInfo.commPort=%s\n"
                      "        myNodeInfo.syncPort=%s\n"
                      "        myNodeInfo.creator=%d\n"
                      "        myNodeInfo.creatorShellPid=%d:%d\n"
                      "        myNodeInfo.ping=%d\n"
                    , method_name, __LINE__
                    , myNodeInfo.pnid
                    , myNodeInfo.nodeName
                    , myNodeInfo.commPort
                    , myNodeInfo.syncPort
                    , myNodeInfo.creator
                    , myNodeInfo.creatorShellPid
                    , myNodeInfo.creatorShellVerifier
                    , myNodeInfo.ping );
    }

    rc = Monitor->SendSock( (char *) &myNodeInfo
                          , sizeof(nodeId_t)
                          , joinSock_
                          , method_name );
    if ( rc )
    {
        HandleReintegrateError( rc, Reintegrate_Err9, -1, NULL );
    }

    TEST_POINT( TP012_NODE_UP );

    mem_log_write(CMonLog::MON_REINTEGRATE_3, MyPNID);

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d Getting all node info from creator monitor\n",
                     method_name, __LINE__);
    }

    // Obtain node names & port numbers of existing monitors from
    // the creator monitor.
    int pnodeCount = Nodes->GetPNodesCount();
    nodeId_t *nodeInfo;
    size_t nodeInfoSize = (sizeof(nodeId_t) * pnodeCount);
    nodeInfo = (nodeId_t *) new char[nodeInfoSize];
    rc = Monitor->ReceiveSock( (char *)nodeInfo
                             , nodeInfoSize
                             , joinSock_
                             , method_name );
    if ( rc )
    {
        HandleReintegrateError( rc, Reintegrate_Err3, -1, NULL );
    }

    if ( initProblem )
    {
        // The monitor encountered an initialization error.  Inform
        // the creator monitor that the node is down.  Then exit.
        SendReIntegrateStatus( State_Down, initProblem );
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Received port info from creator monitor\n"
                    , method_name, __LINE__);
        for (int i=0; i<pnodeCount; i++)
        {
            trace_printf( "Port info for pnid=%d\n"
                          "        nodeInfo[%d].nodeName=%s\n"
                          "        nodeInfo[%d].commPort=%s\n"
                          "        nodeInfo[%d].syncPort=%s\n"
                          "        nodeInfo[%d].creatorPNid=%d\n"
                        , nodeInfo[i].pnid
                        , i, nodeInfo[i].nodeName
                        , i, nodeInfo[i].commPort
                        , i, nodeInfo[i].syncPort
                        , i, nodeInfo[i].creatorPNid );
        }
    }
    // Connect to each of the other existing monitors and let them know
    // we are the NEW monitor and reset the creator flag so they know they are
    // not the creator monitor.
    myNodeInfo.creator = false;
    myNodeInfo.creatorShellPid = -1;
    myNodeInfo.creatorShellVerifier = -1;
    myNodeInfo.ping = false;
    for (int i=0; i<pnodeCount; i++)
    {
        if ( nodeInfo[i].creatorPNid != -1 &&
             nodeInfo[i].creatorPNid == nodeInfo[i].pnid )
        {
            // Get acknowledgement that creator monitor is ready to
            // integrate this node.
            int creatorpnid = -1;
            rc = Monitor->ReceiveSock( (char *) &creatorpnid
                                     , sizeof(creatorpnid)
                                     , joinSock_
                                     , method_name );
            if ( rc || creatorpnid != nodeInfo[i].creatorPNid )
            {
                HandleReintegrateError( rc, Reintegrate_Err15, i, NULL );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Received ready indication from creator "
                              "node %d nodeInfo[%d].nodeName=%s\n"
                            , method_name, __LINE__
                            , creatorpnid, i , nodeInfo[i].nodeName);
            }

            otherMonRank_[nodeInfo[i].pnid] = 0;
            ++currentNodes_;

            // Store port numbers for the node
            strncpy(commPort, nodeInfo[i].commPort, MPI_MAX_PORT_NAME);
            strncpy(syncPort, nodeInfo[i].syncPort, MPI_MAX_PORT_NAME);

            Node[nodeInfo[i].pnid]->SetCommPort( commPort );
            pch1 = strtok (commPort,":");
            pch1 = strtok (NULL,":");
            Node[nodeInfo[i].pnid]->SetCommSocketPort( atoi(pch1) );
            Node[nodeInfo[i].pnid]->SetSyncPort( syncPort );
            pch2 = strtok (syncPort,":");
            pch2 = strtok (NULL,":");
            Node[nodeInfo[i].pnid]->SetSyncSocketPort( atoi(pch2) );
            sockPorts_[nodeInfo[i].pnid] = Node[nodeInfo[i].pnid]->GetSyncSocketPort();

            Node[nodeInfo[i].pnid]->SetState( State_Up );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Setting node %d (%s), commPort=%s(%d), syncPort=%s(%d)\n"
                            , method_name, __LINE__
                            , Node[nodeInfo[i].pnid]->GetPNid()
                            , Node[nodeInfo[i].pnid]->GetName()
                            , pch1, atoi(pch1)
                            , pch2, atoi(pch2) );
            }

            // Tell creator we are ready to accept its connection
            int mypnid = MyPNID;
            rc = Monitor->SendSock( (char *) &mypnid
                                  , sizeof(mypnid)
                                  , joinSock_
                                  , method_name );
            if ( rc )
            {
                HandleReintegrateError( rc, Reintegrate_Err4, i, &nodeInfo[i] );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }

            // Connect to creator monitor
            existingSyncFd = AcceptSyncSock();
            if ( existingSyncFd < 0 )
            {
                HandleReintegrateError( rc, Reintegrate_Err5, i, &nodeInfo[i] );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }
            socks_[nodeInfo[i].pnid] = existingSyncFd; // ReIntegrateSock
            // Set bit indicating node is up
            upNodes_.upNodes[nodeInfo[i].pnid/MAX_NODE_BITMASK] |=
                (1ull << (nodeInfo[i].pnid%MAX_NODE_BITMASK));

            if (trace_settings & (TRACE_RECOVERY | TRACE_INIT))
            {
                trace_printf( "%s@%d Connected to creator node %d (%s)\n"
                            , method_name, __LINE__
                            , nodeInfo[i].creatorPNid
                            , nodeInfo[i].nodeName );
                trace_printf( "%s@%d socks_[%d]=%d\n"
                            , method_name, __LINE__
                            , nodeInfo[i].pnid, socks_[nodeInfo[i].pnid]);
                for ( int i =0; i < MAX_NODE_MASKS ; i++ )
                {
                    trace_printf( "%s@%d Integrating node %s (pnid=%d) "
                                  "sees set[%d]: %llx\n"
                                , method_name, __LINE__
                                , MyNode->GetName(), MyPNID
                                , i, upNodes_.upNodes[i] );
                }
            }

            haveCreatorSocket = true;
        }
        else if ( nodeInfo[i].nodeName[0] != 0 && nodeInfo[i].commPort[0]  != 0 )
        {
            if ( haveCreatorSocket && i >= pnodeCount/2)
                // Reintegration failure after connecting to half
                // of existing monitors.
                TEST_POINT( TP016_NODE_UP );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d Attempting connection to node %d (%s), "
                             "port %s\n", method_name, __LINE__, nodeInfo[i].pnid,
                             nodeInfo[i].nodeName, nodeInfo[i].commPort);
            }

            mem_log_write(CMonLog::MON_REINTEGRATE_5, MyPNID, i);

            TEST_POINT( TP013_NODE_UP );

            // Connect to existing monitor
            existingCommFd = Monitor->Connect( nodeInfo[i].commPort );
            if ( existingCommFd < 0 )
            {
                HandleReintegrateError( rc, Reintegrate_Err5, i, &nodeInfo[i] );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }

            TEST_POINT( TP014_NODE_UP );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d Connected to node %d (%s), sending my node name\n",
                             method_name, __LINE__,i,nodeInfo[i].nodeName);
            }

            // Send this nodes name and port number so other monitor
            // knows who we are.
            rc = Monitor->SendSock( (char *) &myNodeInfo
                                  , sizeof(nodeId_t)
                                  , existingCommFd
                                  , method_name );
            if ( rc )
            {
                HandleReintegrateError( rc, Reintegrate_Err4, i, &nodeInfo[i] );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }

            // Get acknowledgement that other monitor is ready to
            // integrate this node.  This is an interlock to avoid a
            // race condition where the creator monitor could signal
            // the monitors in the cluster to integrate the new node
            // before one or more was ready to do the integration.
            int remotepnid = -1;
            rc = Monitor->ReceiveSock( (char *) &remotepnid
                                     , sizeof(remotepnid)
                                     , existingCommFd
                                     , method_name );
            if ( rc || remotepnid != nodeInfo[i].pnid )
            {
                HandleReintegrateError( rc, Reintegrate_Err15, i, NULL );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Received ready indication from "
                              "node %d nodeInfo[%d].nodeName=%s\n"
                            , method_name, __LINE__
                            , remotepnid, i , nodeInfo[i].nodeName);
            }

            otherMonRank_[nodeInfo[i].pnid] = 0;
            ++currentNodes_;

            // Store port numbers for the node
            strncpy(commPort, nodeInfo[i].commPort, MPI_MAX_PORT_NAME);
            strncpy(syncPort, nodeInfo[i].syncPort, MPI_MAX_PORT_NAME);

            Node[nodeInfo[i].pnid]->SetCommPort( commPort );
            pch1 = strtok (commPort,":");
            pch1 = strtok (NULL,":");
            Node[nodeInfo[i].pnid]->SetCommSocketPort( atoi(pch1) );

            Node[nodeInfo[i].pnid]->SetSyncPort( syncPort );
            pch2 = strtok (syncPort,":");
            pch2 = strtok (NULL,":");
            Node[nodeInfo[i].pnid]->SetSyncSocketPort( atoi(pch2) );
            sockPorts_[nodeInfo[i].pnid] = Node[nodeInfo[i].pnid]->GetSyncSocketPort();

            Node[nodeInfo[i].pnid]->SetState( State_Up );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Setting node %d (%s), commPort=%s(%d), syncPort=%s(%d)\n"
                            , method_name, __LINE__
                            , Node[nodeInfo[i].pnid]->GetPNid()
                            , Node[nodeInfo[i].pnid]->GetName()
                            , pch1, atoi(pch1)
                            , pch2, atoi(pch2) );
            }

            // Connect to existing monitor
            existingSyncFd = AcceptSyncSock();
            if ( existingSyncFd < 0 )
            {
                HandleReintegrateError( rc, Reintegrate_Err5, i, &nodeInfo[i] );
                SendReIntegrateStatus( State_Down, Reintegrate_Err14 );
            }
            socks_[nodeInfo[i].pnid] = existingSyncFd; // ReIntegrateSock

            // Set bit indicating node is up
            upNodes_.upNodes[nodeInfo[i].pnid/MAX_NODE_BITMASK] |=
            (1ull << (nodeInfo[i].pnid%MAX_NODE_BITMASK));

            if (trace_settings & (TRACE_RECOVERY | TRACE_INIT))
            {
                trace_printf( "%s@%d socks_[%d]=%d\n"
                            , method_name, __LINE__
                            , nodeInfo[i].pnid, socks_[nodeInfo[i].pnid]);
                for ( int i =0; i < MAX_NODE_MASKS ; i++ )
                {
                    trace_printf( "%s@%d Integrating node %s (pnid=%d) "
                                  "sees set[%d]: %llx\n"
                                , method_name, __LINE__
                                , MyNode->GetName(), MyPNID
                                , i, upNodes_.upNodes[i] );
                }
            }

            mem_log_write(CMonLog::MON_REINTEGRATE_6, MyPNID, i);
        }
        else if ( nodeInfo[i].pnid != MyPNID)
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d Connection to node %d not attempted, "
                              "since it's unavailable port information.\n"
                              "nodeInfo[%d].commPort=%s\n"
                              "nodeInfo[%d].syncPort=%s\n"
                              "IntegratingMonitorPort=%s\n"
                            , method_name, __LINE__
                            , nodeInfo[i].pnid
                            , i, nodeInfo[i].commPort
                            , i, nodeInfo[i].syncPort
                            , IntegratingMonitorPort);
            }
        }
    }

    if (trace_settings & (TRACE_RECOVERY | TRACE_INIT))
    {
        for (int i=0; i<pnodeCount; i++)
        {
            if (nodeInfo[i].pnid == -1) continue;
            if (Node[nodeInfo[i].pnid] == NULL) continue;
            trace_printf( "%s@%d - Node info for pnid=%d (%s)\n"
                          "        Node[%d] commPort=%s\n"
                          "        Node[%d] syncPort=%s\n"
                          "        Node[%d] creatorPNid=%d\n"
                        , method_name, __LINE__
                        , Node[nodeInfo[i].pnid]->GetPNid()
                        , Node[nodeInfo[i].pnid]->GetName()
                        , nodeInfo[i].pnid, Node[nodeInfo[i].pnid]->GetCommPort()
                        , nodeInfo[i].pnid, Node[nodeInfo[i].pnid]->GetSyncPort()
                        , nodeInfo[i].pnid, nodeInfo[i].creatorPNid);
        }
        for ( int i =0; i < pnodeCount; i++ )
        {
            if (nodeInfo[i].pnid == -1) continue;
            trace_printf( "%s@%d socks_[%d]=%d, sockPorts_[%d]=%d\n"
                        , method_name, __LINE__
                        , nodeInfo[i].pnid, socks_[nodeInfo[i].pnid]
                        , nodeInfo[i].pnid, sockPorts_[nodeInfo[i].pnid]);
        }
        for ( int i =0; i < MAX_NODE_MASKS ; i++ )
        {
            trace_printf( "%s@%d Integrating node %s (pnid=%d) "
                          "sees set[%d]: %llx\n"
                        , method_name, __LINE__
                        , MyNode->GetName(), MyPNID
                        , i, upNodes_.upNodes[i] );
        }
    }

    mem_log_write(CMonLog::MON_REINTEGRATE_7, MyPNID);

    TEST_POINT( TP015_NODE_UP );

    // Inform creator monitor that connections are complete and
    // this monitor is ready to participate in "allgather"
    // communications with the other monitors.
    SendReIntegrateStatus( State_Up, 0 );

    mem_log_write(CMonLog::MON_REINTEGRATE_8, MyPNID);

    MyNode->SetState( State_Merged );

    delete[] nodeInfo;

    TRACE_EXIT;
}

void CCluster::ResetIntegratingPNid( void )
{
    const char method_name[] = "CCluster::ResetIntegratingPNid";
    TRACE_ENTRY;

    switch( CommType )
    {
        case CommType_InfiniBand:
            if ( joinComm_ != MPI_COMM_NULL )
            {
                MPI_Comm_free( &joinComm_ );
                joinComm_ = MPI_COMM_NULL;
            }
            break;
        case CommType_Sockets:
            if ( joinSock_ != -1 )
            {
                close(joinSock_);
                joinSock_ = -1;
            }
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    if ( MyNode->IsCreator() )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Resetting creator pnid=%d\n",
                          method_name, __LINE__, MyPNID );
        }

        MyNode->SetCreator( false, -1, -1 );
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Resetting integratingPNid_=%d\n",
                      method_name, __LINE__, integratingPNid_ );
    }

    integratingPNid_ = -1;

#ifdef NAMESERVER_PROCESS
    if (!CommAcceptMon.isAccepting())
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Triggering commAcceptorMon thread to begin accepting connections\n",
                          method_name, __LINE__ );
        }

        // Indicate to the commAcceptor thread to begin accepting connections
        CommAcceptMon.startAccepting();
    }
#endif

    if (!CommAccept.isAccepting())
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Triggering commAcceptor thread to begin accepting connections\n",
                          method_name, __LINE__ );
        }

        // Indicate to the commAcceptor thread to begin accepting connections
        CommAccept.startAccepting();
    }

    TRACE_EXIT;
}

void CCluster::SetIntegratingPNid( int pnid )
{
    const char method_name[] = "CCluster::SetIntegratingPNid";
    TRACE_ENTRY;

    integratingPNid_ = pnid;

    TRACE_EXIT;
}

// Save information about a new communicator for a node that is reintegrating
void CCluster::addNewComm(int pnid, int otherRank,  MPI_Comm comm)
{
    const char method_name[] = "CCluster::addNewComm";
    TRACE_ENTRY;

    if (trace_settings & TRACE_RECOVERY)
    {
        trace_printf("%s@%d - saving communicator for pnid %d\n",
                     method_name, __LINE__, pnid);
    }

    // Insert info for new comm into list
    commInfo_t commInfo = {pnid, otherRank, comm, -1, {0, 0}};
    clock_gettime(CLOCK_REALTIME, &commInfo.ts);

    newCommsLock_.lock();
    newComms_.push_back( commInfo );
    newCommsLock_.unlock();

    TRACE_EXIT;
}

// A node is reintegrating.   Add the communicator for the node to the set of
// communicators used by "Allgather".
void CCluster::setNewComm( int pnid )
{
    const char method_name[] = "CCluster::setNewComm";
    TRACE_ENTRY;

    newComms_t::iterator it;
    bool foundComm = false;

    if ( comms_[pnid] != MPI_COMM_NULL )
    {   // Unexpectedly already have a communicator for this node
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s]  Unexpectedly already have a "
                 "communicator for node %d\n", method_name, pnid);
        mon_log_write(MON_CLUSTER_SETNEWCOMM_1, SQ_LOG_ERR, buf);

        MPI_Comm_free( &comms_[pnid] );
        if ( CommType == CommType_Sockets )
        {
            shutdown( socks_[pnid], SHUT_RDWR);
            close( socks_[pnid] );
            socks_[pnid] = -1;
        }
    }

    newCommsLock_.lock();
    for ( it = newComms_.begin(); it != newComms_.end(); )
    {
        if ( it->pnid == pnid )
        {
            if ( comms_[pnid] != MPI_COMM_NULL )
            {   // Found another communicator for the specified node.
                // Disconnect from the previous one.  It must be a
                // stale leftover from a previous reintegration
                // attempt for the node.
                if (trace_settings & TRACE_RECOVERY)
                {
                    trace_printf("%s@%d - discarding stale communicator for "
                                 "pnid %d\n", method_name, __LINE__, pnid);
                }

                MPI_Comm_free( &comms_[pnid] );
                if ( CommType == CommType_Sockets )
                {
                    shutdown( socks_[pnid], SHUT_RDWR);
                    close( socks_[pnid] );
                    socks_[pnid] = -1;
                }
                --currentNodes_;
            }

            if (trace_settings & TRACE_RECOVERY)
            {
                trace_printf("%s@%d - setting new communicator for pnid %d, "
                             "otherRank=%d\n",
                             method_name, __LINE__, it->pnid, it->otherRank);
            }

            comms_[it->pnid] = it->comm;
            otherMonRank_[it->pnid] = it->otherRank;
            ++currentNodes_;
            // Set bit indicating node is up
            upNodes_.upNodes[it->pnid/MAX_NODE_BITMASK] |= (1ull << (it->pnid%MAX_NODE_BITMASK));

            // Delete current list element and advance to next one
            it = newComms_.erase ( it );

            foundComm = true;
        }
        else
        {   // Advance to next list element
            ++it;
        }
    }
    newCommsLock_.unlock();

    if ( !foundComm )
    {  // We have no communicator for the specified node.
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s] Could not find a communicator for "
                 "node %d\n", method_name, pnid);
        mon_log_write(MON_CLUSTER_SETNEWCOMM_2, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

// Save information about a new socket for a node that is reintegrating
void CCluster::addNewSock(int pnid, int otherRank, int sockFd)
{
    const char method_name[] = "CCluster::addNewSock";
    TRACE_ENTRY;

    if (trace_settings & TRACE_RECOVERY)
    {
        trace_printf("%s@%d - saving socket for pnid %d\n",
                     method_name, __LINE__, pnid);
    }

    // Insert info for new comm into list
    commInfo_t commInfo = {pnid, otherRank, MPI_COMM_NULL, sockFd, {0, 0}};
    clock_gettime(CLOCK_REALTIME, &commInfo.ts);

    newCommsLock_.lock();
    newComms_.push_back( commInfo );
    newCommsLock_.unlock();

    TRACE_EXIT;
}

// A node is reintegrating.   Add the socket for the node to the set of
// communicators used by "Allgather".
void CCluster::setNewSock( int pnid )
{
    const char method_name[] = "CCluster::setNewSock";
    TRACE_ENTRY;

    newComms_t::iterator it;
    bool foundSocket = false;

    if ( socks_[pnid] != -1 )
    {   // Unexpectedly already have a communicator for this node
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s]  Unexpectedly already have a "
                 "socket for node %d\n", method_name, pnid);
        mon_log_write(MON_CLUSTER_SETNEWSOCK_1, SQ_LOG_ERR, buf);

        shutdown( socks_[pnid], SHUT_RDWR);
        close( socks_[pnid] );
        socks_[pnid] = -1;
    }

    newCommsLock_.lock();
    for ( it = newComms_.begin(); it != newComms_.end(); )
    {
        if ( it->pnid == pnid )
        {
            if ( socks_[pnid] != -1 )
            {   // Found another socket for the specified node.
                // Disconnect from the previous one.  It must be a
                // stale leftover from a previous reintegration
                // attempt for the node.
                if (trace_settings & TRACE_RECOVERY)
                {
                    trace_printf("%s@%d - discarding stale communicator for "
                                 "pnid %d\n", method_name, __LINE__, pnid);
                }

                shutdown( socks_[pnid], SHUT_RDWR);
                close( socks_[pnid] );
                socks_[pnid] = -1;
                --currentNodes_;
            }

            CNode *node = Nodes->GetNode( it->pnid );
            socks_[it->pnid] = it->socket; // setNewSock
            sockPorts_[it->pnid] = node->GetSyncSocketPort();
            otherMonRank_[it->pnid] = it->otherRank;
            ++currentNodes_;

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Setting new communicator for %d (%s), "
                              "socks_[%d]=%d, sockPorts_[%d]=%d, otherMonRank_[%d]=%d\n"
                            , method_name, __LINE__
                            , node->GetPNid()
                            , node->GetName()
                            , it->pnid, socks_[it->pnid]
                            , it->pnid, sockPorts_[it->pnid]
                            , it->pnid, otherMonRank_[it->pnid] );
            }

            // Set bit indicating node is up
            upNodes_.upNodes[it->pnid/MAX_NODE_BITMASK] |= (1ull << (it->pnid%MAX_NODE_BITMASK));

            // Delete current list element and advance to next one
            it = newComms_.erase ( it );

            foundSocket = true;
        }
        else
        {   // Advance to next list element
            ++it;
        }
    }
    newCommsLock_.unlock();

    if ( !foundSocket )
    {  // We have no communicator for the specified node.
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s] Could not find a socket for "
                 "node %d\n", method_name, pnid);
        mon_log_write(MON_CLUSTER_SETNEWSOCK_2, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

int CCluster::Allgather( int nbytes, void *sbuf, char *rbuf, int tag, MPI_Status *stats )
{
    const char method_name[] = "CCluster::Allgather";
    TRACE_ENTRY;

    int err = 0;

    switch( CommType )
    {
        case CommType_InfiniBand:
            err = AllgatherIB( nbytes, sbuf, rbuf, tag, stats );
            break;
        case CommType_Sockets:
            err = AllgatherSock( nbytes, sbuf, rbuf, tag, stats );
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    TRACE_EXIT;
    return err;
}

int CCluster::AllgatherIB( int nbytes, void *sbuf, char *rbuf, int tag, MPI_Status *stats )
{
    const char method_name[] = "CCluster::AllgatherIB";
    TRACE_ENTRY;

    int e;
    int err = 0;

    MPI_Request r[2*GetConfigPNodesCount()];
    MPI_Status s[2*GetConfigPNodesCount()];
    for ( int i = 0; i < 2*GetConfigPNodesCount(); i++ )
    {
        s[i].MPI_ERROR = MPI_SUCCESS;
        r[i] = MPI_REQUEST_NULL;
    }

    char *cp = rbuf;
    for ( int i = 0; i < GetConfigPNodesCount(); i++ )
    {
        if ( comms_[i] != MPI_COMM_NULL && otherMonRank_[i] != -1 )
        {
            e = MPI_Send_init( sbuf, nbytes, MPI_CHAR, otherMonRank_[i], tag,
                comms_[i], &r[i] );
            if ( e != MPI_SUCCESS )
            {
                MPI_Error_class( e, &err );
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Comunication error with pnid=%d (%s), "
                          "MPI_Send_init() error=%s (%d)\n"
                        , method_name, i, Node[i]->GetName()
                        , ErrorMsg(e), e );
                mon_log_write(MON_CLUSTER_ALLGATHERIB_1, SQ_LOG_ERR, buf);
                goto early_exit;
            }

            e = MPI_Recv_init( cp, CommBufSize, MPI_CHAR, otherMonRank_[i], tag,
                comms_[i], &r[i+GetConfigPNodesCount()] );
            if ( e != MPI_SUCCESS )
            {
                MPI_Error_class( e, &err );
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Comunication error with pnid=%d (%s), "
                          "MPI_Recv_init() error=%s (%d)\n"
                        , method_name, i, Node[i]->GetName()
                        , ErrorMsg(e), e );
                mon_log_write(MON_CLUSTER_ALLGATHERIB_2, SQ_LOG_ERR, buf);
                goto early_exit;
            }
        }
        cp += CommBufSize;
    }
    for ( int i = 0; i < 2*GetConfigPNodesCount(); i++ )
    {
        if ( r[i] == MPI_REQUEST_NULL ) continue;
        e = MPI_Start( &r[i] );
        if ( e != MPI_SUCCESS )
        {
            MPI_Error_class( e, &err );
            char buf[MON_STRING_BUF_SIZE];
            int pnid = (i < GetConfigPNodesCount()) ? i : (i - GetConfigPNodesCount());
            snprintf( buf, sizeof(buf)
                    , "[%s], Comunication error with pnid=%d (%s), "
                      "MPI_Start() error=%s (%d)\n"
                    , method_name, pnid, Node[pnid]->GetName()
                    , ErrorMsg(e), e );
            mon_log_write(MON_CLUSTER_ALLGATHERIB_3, SQ_LOG_ERR, buf);
            goto early_exit;
        }
    }

    inBarrier_ = true;
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->BarrierWaitIncr();

    e = MPI_Waitall( GetConfigPNodesCount()*2, r, s );
    if ( e != MPI_SUCCESS )
    {
        MPI_Error_class( e, &err );
        if ( err != MPI_ERR_IN_STATUS )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s], MPI_Waitall() error=%s (%d)\n"
                    , method_name, ErrorMsg(e), e );
            mon_log_write(MON_CLUSTER_ALLGATHERIB_4, SQ_LOG_ERR, buf);
            inBarrier_ = false;
            goto early_exit;
        }
    }

    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->BarrierWaitDecr();
    inBarrier_ = false;

    for ( int i = 0; i < GetConfigPNodesCount(); i++ )
    {
        stats[i] = s[i+GetConfigPNodesCount()];
    }
    if ( e == MPI_SUCCESS )
    {
        err = MPI_SUCCESS;
        goto early_exit;
    }

    for ( int i = 0; i < GetConfigPNodesCount(); i++ )
    {
        if ( s[i].MPI_ERROR != MPI_SUCCESS &&             // send
             s[i+GetConfigPNodesCount()].MPI_ERROR == MPI_SUCCESS )   // receive
        {
            stats[i].MPI_ERROR = s[i].MPI_ERROR;
        }
    }

early_exit:

    for ( int i = 0; i < 2*GetConfigPNodesCount(); i++ )
    {
        if ( r[i] != MPI_REQUEST_NULL )
        {
            MPI_Request_free( &r[i] );
        }
    }

    barrierCount_++;

    TRACE_EXIT;
    return err;
}

int CCluster::AllgatherSock( int nbytes, void *sbuf, char *rbuf, int tag, MPI_Status *stats )
{
    const char method_name[] = "CCluster::AllgatherSock";
    TRACE_ENTRY;

    bool reconnecting = false;
    static int hdrSize = Nodes->GetSyncHdrSize( );
    int err = MPI_SUCCESS;
    int lastReconnectErr = MPI_ERR_IN_STATUS;
    peer_t p[GetConfigPNodesMax()];
    memset( p, 0, sizeof(p) );
    tag = tag; // make compiler happy
    struct timespec currentTime;
    // Set to twice the ZClient session timeout
    static int sessionTimeout = ZClientEnabled
                                ? (ZClient->SessionTimeoutGet() * 2) : 120;

    int nsent = 0, nrecv = 0;
    for ( int iPeer = 0; iPeer < GetConfigPNodesCount(); iPeer++ )
    {
        peer_t *peer = &p[indexToPnid_[iPeer]];
        stats[indexToPnid_[iPeer]].MPI_ERROR = MPI_SUCCESS;
        stats[indexToPnid_[iPeer]].count = 0;
        if ( indexToPnid_[iPeer] == MyPNID || socks_[indexToPnid_[iPeer]] == -1 )
        {
            peer->p_sending = peer->p_receiving = false;
            nsent++;
            nrecv++;
        }
        else
        {
            peer->p_sending = peer->p_receiving = true;
            peer->p_sent = peer->p_received = 0;
            peer->p_timeout_count = 0;
            peer->p_initial_check = true;
            peer->p_n2recv = -1;
            peer->p_buff = ((char *) rbuf) + (indexToPnid_[iPeer] * CommBufSize);

            struct epoll_event event;
            event.data.fd = socks_[indexToPnid_[iPeer]];
            event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
            EpollCtl( epollFD_, EPOLL_CTL_ADD, socks_[indexToPnid_[iPeer]], &event );
        }
    }

    if (trace_settings & (TRACE_SYNC | TRACE_SYNC_DETAIL))
    {
        for ( int i = 0; i < GetConfigPNodesCount(); i++ )
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                peer_t *peer = &p[indexToPnid_[i]];
                trace_printf( "%s@%d" " - socks_[%d]=%d, "
                              "peer->p_sending=%d, "
                              "peer->p_receiving=%d\n"
                            , method_name, __LINE__
                            , indexToPnid_[i]
                            , socks_[indexToPnid_[i]]
                            , peer->p_sending
                            , peer->p_receiving );
            }
        }
    }

    inBarrier_ = true;
    MonStats->BarrierWaitIncr( );

    static int sv_epoll_wait_timeout = -2;
    static int sv_epoll_retry_count = 1;
    if ( sv_epoll_wait_timeout == -2 )
    {
        char *lv_epoll_wait_timeout_env = getenv( "SQ_MON_EPOLL_WAIT_TIMEOUT" );
        if ( lv_epoll_wait_timeout_env )
        {
            // convert to milliseconds
            sv_epoll_wait_timeout = atoi( lv_epoll_wait_timeout_env ) * 1000;
            char *lv_epoll_retry_count_env = getenv( "SQ_MON_EPOLL_RETRY_COUNT" );
            if ( lv_epoll_retry_count_env )
            {
                sv_epoll_retry_count = atoi( lv_epoll_retry_count_env );
            }
            else
            {
                // default to 64 seconds
                sv_epoll_wait_timeout = 16000;
                sv_epoll_retry_count = 4;
            }
            if ( sv_epoll_retry_count > 180 )
            {
                sv_epoll_retry_count = 180;
            }
        }
        else
        {
            // default to 64 seconds
            sv_epoll_wait_timeout = 16000;
            sv_epoll_retry_count = 4;
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] EPOLL timeout wait_timeout=%d msecs, retry_count=%d\n"
                , method_name
                ,  __LINE__
                , sv_epoll_wait_timeout
                , sv_epoll_retry_count );

        mon_log_write( MON_CLUSTER_ALLGATHERSOCK_1, SQ_LOG_INFO, buf );
    }

    bool resetConnections = false;
    int peerTimedoutCount = 0;

    // do the work
    struct epoll_event events[2*GetConfigPNodesMax() + 1];
    while ( 1 )
    {
reconnected:
        bool checkConnections = false;
        bool doReconnect = false;
        int numPeersTimedout = 0;
        int maxEvents = 2*GetConfigPNodesCount() - nsent - nrecv;
        if ( maxEvents == 0 ) break;
        int nw;
        peer_t *peer;

        while ( 1 )
        {
            nw = epoll_wait( epollFD_, events, maxEvents, sv_epoll_wait_timeout );
            if ( nw >= 0 || errno != EINTR ) break;
        }

        if ( nw == 0 )
        { // Timeout, no fd's ready
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d" " - IO timeout! (seqNum_=%lld)\n"
                            , method_name, __LINE__, seqNum_ );
            }
        
            peerTimedoutCount++;
            clock_gettime(CLOCK_REALTIME, &currentTime);
            for ( int iPeer = 0; iPeer < GetConfigPNodesCount(); iPeer++ )
            { // Check no IO completion on peers
                peer = &p[indexToPnid_[iPeer]];
                if ( (peer->p_receiving) || (peer->p_sending) )
                {
                    if (peer->p_initial_check && !reconnecting)
                    { // Set the session timeout relative to now
                        peer->p_initial_check = false;
                        clock_gettime(CLOCK_REALTIME, &peer->znodeFailedTime);
                        peer->znodeFailedTime.tv_sec += sessionTimeout;
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d" " - Znode Fail Time %ld(secs)\n"
                                        , method_name, __LINE__
                                        , peer->znodeFailedTime.tv_sec);
                        }
                    }

                    numPeersTimedout++;
                    peer->p_timeout_count++;
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d - EPOLL timeout (peer->p_timeout_count=%d) on: %s(%d), "
                                      "socks_[%d]=%d, "
                                      "peer->p_sending=%d, "
                                      "peer->p_receiving=%d\n"
                                    , method_name, __LINE__
                                    , peer->p_timeout_count
                                    , Node[indexToPnid_[iPeer]]->GetName(), indexToPnid_[iPeer]
                                    , indexToPnid_[iPeer]
                                    , socks_[indexToPnid_[iPeer]]
                                    , peer->p_sending
                                    , peer->p_receiving );
                    }

                    if (IsRealCluster && peer->p_timeout_count)
                    {
                        checkConnections = true;
                        if (lastReconnectErr == MPI_SUCCESS
                         && peer->p_timeout_count)
                        {
                            resetConnections = true;
                        }
                    }
                    else
                    {
                        checkConnections = (IsRealCluster) ? true : false;
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d" " - Peer timed out: %s(%d), "
                                          "socks_[%d]=%d, "
                                          "peer->p_timeout_count=%d, "
                                          "peer->znodeFailedTime=%ld(secs), "
                                          "currentTime=%ld(secs)\n"
                                        , method_name, __LINE__
                                        , Node[indexToPnid_[iPeer]]->GetName(), indexToPnid_[iPeer]
                                        , indexToPnid_[iPeer]
                                        , socks_[indexToPnid_[iPeer]]
                                        , peer->p_timeout_count
                                        , peer->znodeFailedTime.tv_sec
                                        , currentTime.tv_sec );
                        }
                    }
                }
            } // Check no IO completion on peers

            if (checkConnections)
            {
                checkConnections = false;
                if (trace_settings & TRACE_RECOVERY)
                {
                    trace_printf( "%s@%d - Initiating AllgatherSockReconnect(), "
                                  "peerTimedoutCount=%d, numPeersTimedout=%d, "
                                  "resetConnections=%d, lastReconnectErr=%d\n"
                                , method_name, __LINE__
                                , peerTimedoutCount
                                , numPeersTimedout
                                , resetConnections
                                , lastReconnectErr );
                }
                // First, check ability to connect to all peers
                // An err returned will mean that connect failed with
                // at least one peer. No err implies that possible network
                // reset occurred and there is probably one dead connection
                // to a peer where no IOs will complete ever, so connections
                // to all peers must be reestablished.
                lastReconnectErr = err = AllgatherSockReconnect( stats, p, resetConnections );
                if (err == MPI_SUCCESS)
                { // Connections to all peers are good
                    if (resetConnections)
                    { // Establish new connections on all peers
                        resetConnections = false;
                        peerTimedoutCount = 0;
                        // Redrive IOs on new peer connections
                        nsent = 0; nrecv = 0;
                        for ( int i = 0; i < GetConfigPNodesCount(); i++ )
                        {
                            peer = &p[indexToPnid_[i]];
                            if ( indexToPnid_[i] == MyPNID || socks_[indexToPnid_[i]] == -1 )
                            { // peer is me or not available
                                peer->p_sending = peer->p_receiving = false;
                                nsent++;
                                nrecv++;
                            }
                            else
                            {
                                peer->p_sending = peer->p_receiving = true;
                                peer->p_sent = peer->p_received = 0;
                                peer->p_timeout_count = 0;
                                peer->p_n2recv = -1;
                                peer->p_buff = ((char *) rbuf) + (indexToPnid_[i] * CommBufSize);
                                struct epoll_event event;
                                event.data.fd = socks_[indexToPnid_[i]];
                                event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
                                EpollCtl( epollFD_, EPOLL_CTL_ADD, socks_[indexToPnid_[i]], &event );
                            }
                        }
                    } // (resetConnections)
                } // (err == MPI_SUCCESS)
                else
                {
                    for ( int i = 0; i < GetConfigPNodesCount(); i++ )
                    {
                        peer = &p[indexToPnid_[i]];
                        if ( indexToPnid_[i] != MyPNID && socks_[indexToPnid_[i]] == -1 )
                        { // peer is me or no longer available
                            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY) &&
                                (peer->p_sending || peer->p_receiving) )
                            {
                                trace_printf( "%s@%d No IO completion on %s(%d):socks_[%d]=%d, "
                                              "peer->p_sending=%d, "
                                              "peer->p_receiving=%d\n"
                                            , method_name, __LINE__
                                            , Node[indexToPnid_[i]]->GetName(), indexToPnid_[i]
                                            , indexToPnid_[i]
                                            , socks_[indexToPnid_[i]]
                                            , peer->p_sending
                                            , peer->p_receiving );
                            }
                            if (peer->p_sending)
                            {
                                nsent++;
                                peer->p_sending = false;
                            }
                            if (peer->p_receiving)
                            {
                                peer->p_receiving = false;
                                nrecv++;
                            }
                        }
                    }
                }
                doReconnect = true;
            } // (checkConnections)

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                for ( int i = 0; i < GetConfigPNodesCount(); i++ )
                {
                    peer = &p[indexToPnid_[i]];
                    trace_printf( "%s@%d doReconnect=%d, %s(%d):socks_[%d]=%d, "
                                  "peer->p_sending=%d, "
                                  "peer->p_receiving=%d\n"
                                , method_name, __LINE__
                                , doReconnect
                                , Node[indexToPnid_[i]]->GetName(), indexToPnid_[i]
                                , indexToPnid_[i]
                                , socks_[indexToPnid_[i]]
                                , peer->p_sending
                                , peer->p_receiving );
                }
            }

            if (doReconnect)
            {
                reconnectSeqNum_ = seqNum_;
                reconnecting = true;
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d" " - Reconnecting! "
                                  "(lastReconnectErr=%d, reconnectSeqNum_=%lld)\n"
                                , method_name, __LINE__
                                , lastReconnectErr
                                , reconnectSeqNum_ );
                }
                goto reconnected;
            }
        }  // ( nw == 0 )

        if ( nw < 0 )
        { // Got an error
            char ebuff[256];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s@%d] epoll_wait(%d, %d) error: %s\n",
                method_name, __LINE__, epollFD_, maxEvents,
                strerror_r( errno, ebuff, 256 ) );
            mon_log_write( MON_CLUSTER_ALLGATHERSOCK_3, SQ_LOG_CRIT, buf );

            mon_failure_exit();
        }

        // Process fd's which are ready to initiate an IO or completed IO
        for ( int iEvent = 0; iEvent < nw; iEvent++ )
        {
            bool stateChange = false;
            int fd = events[iEvent].data.fd;
            int iPeer;
            for ( iPeer = 0; iPeer < GetConfigPNodesCount(); iPeer++ )
            { // Find corresponding peer by matching socket fd
                if ( events[iEvent].data.fd == socks_[indexToPnid_[iPeer]] ) break;
            }
            if ( indexToPnid_[iPeer] < 0 || indexToPnid_[iPeer] >= GetConfigPNodesMax() || indexToPnid_[iPeer] == MyPNID
                || socks_[indexToPnid_[iPeer]] == -1
                || (!p[indexToPnid_[iPeer]].p_sending && !p[indexToPnid_[iPeer]].p_receiving) )
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s@%d] Invalid peer %d, "
                          "peer.p_sending=%d, "
                          "peer.p_receiving=%d\n"
                        , method_name, __LINE__
                        , indexToPnid_[iPeer]
                        , indexToPnid_[iPeer] >= GetConfigPNodesMax()?-1:p[indexToPnid_[iPeer]].p_sending
                        , indexToPnid_[iPeer] >= GetConfigPNodesMax()?-1:p[indexToPnid_[iPeer]].p_receiving );
                mon_log_write( MON_CLUSTER_ALLGATHERSOCK_4, SQ_LOG_CRIT, buf );

                mon_failure_exit();
            }
            peer_t *peer = &p[indexToPnid_[iPeer]];
            if ( (events[iEvent].events & EPOLLERR) ||
                 (events[iEvent].events & EPOLLHUP) ||
                 ( !(events[iEvent].events & (EPOLLIN|EPOLLOUT))) )
            {
                // An error has occurred on this fd, or the socket is not
                // ready for reading nor writing
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s@%d] Error: peer=%s(%d), events[%d].data.fd=%d, event[%d]=%s\n"
                        , method_name, __LINE__
                        , Node[indexToPnid_[iPeer]]->GetName()
                        , indexToPnid_[iPeer]
                        , iEvent
                        , events[iEvent].data.fd
                        , iEvent
                        , EpollEventString(events[iEvent].events) );
                mon_log_write( MON_CLUSTER_ALLGATHERSOCK_5, SQ_LOG_CRIT, buf );

                err = CheckSockPeer( indexToPnid_[iPeer], stats, peer );
                if (err == MPI_SUCCESS)
                {
                    if ( indexToPnid_[iPeer] == MyPNID || socks_[indexToPnid_[iPeer]] == -1 )
                    { // peer is me or not available
                        peer->p_sending = peer->p_receiving = false;
                        nsent++;
                        nrecv++;
                    }
                    else
                    {
                        doReconnect = true;
                    }
                }
                else
                {
                    if (stats[indexToPnid_[iPeer]].MPI_ERROR == MPI_SUCCESS)
                    {
                        doReconnect = true;
                        checkConnections = (IsRealCluster) ? true : false;
                    }
                    else
                    {
                        if (peer->p_sending)
                        {
                            nsent++;
                            peer->p_sending = false;
                        }
                        if (peer->p_receiving)
                        {
                            peer->p_receiving = false;
                            nrecv++;
                        }
                        goto early_exit;
                    }
                }
            }
            if ( peer->p_receiving && events[iEvent].events & EPOLLIN )
            { // Got receive (read) completion
                int eagain_ok = 0;
read_again:
                char *r = &peer->p_buff[peer->p_received];
                int n2get;
                if ( peer->p_received >= hdrSize )
                {
                    n2get = peer->p_n2recv;
                }
                else
                {
                    n2get = hdrSize - peer->p_received;
                }
                int nr;
                while ( 1 )
                {
                    if (trace_settings & TRACE_SYNC)
                    {
                        trace_printf( "%s@%d - EPOLLIN from %s(%d),"
                                      " sending=%d,"
                                      " receiving=%d (%d)"
                                      " sent=%d,"
                                      " received=%d"
                                      " timeout_count=%d,"
                                      " initial_check=%d,"
                                      " n2recv=%d\n"
                                    , method_name, __LINE__
                                    , Node[indexToPnid_[iPeer]]->GetName(), indexToPnid_[iPeer]
                                    , peer->p_sending
                                    , peer->p_receiving, n2get
                                    , peer->p_sent
                                    , peer->p_received
                                    , peer->p_timeout_count
                                    , peer->p_initial_check
                                    , peer->p_n2recv );
                    }
                    nr = recv( fd, r, n2get, 0 );
                    if ( nr > 0 ) Meas.addSockAllGatherRcvdBytes( nr );
                    if ( nr >= 0 || errno == EINTR ) break;
                }
                if ( nr < 0 )
                {
                    if ( nr < 0 && eagain_ok && errno == EAGAIN )
                    {
                        // do nothing
                    }
                    else
                    {
                        // error, down socket
                        int err = errno;
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                , "[%s@%d] recv[%d](%d) error %d (%s)\n"
                                , method_name, __LINE__
                                , indexToPnid_[iPeer], nr , err, strerror(err) );
                        mon_log_write( MON_CLUSTER_ALLGATHERSOCK_6, SQ_LOG_CRIT, buf );
                        peer->p_receiving = false;
                        nrecv++;
                        if ( peer->p_sending )
                        {
                            peer->p_sending = false;
                            nsent++;
                        }
                        stats[indexToPnid_[iPeer]].MPI_ERROR = MPI_ERR_EXITED;
                        err = MPI_ERR_IN_STATUS;
                        stateChange = true;
                    }
                }
                else
                {
                    peer->p_received += nr;
                    if ( peer->p_received < hdrSize )
                    {
                        // do nothing
                    }
                    else
                    {
                        if ( peer->p_received == hdrSize )
                        {
                            // got the complete header, get buffer size
                            struct sync_buffer_def *sb;
                            sb = (struct sync_buffer_def *)peer->p_buff;
                            peer->p_n2recv = sb->msgInfo.msg_offset;
                            if ( peer->p_n2recv )
                            {
                                eagain_ok = 1;
                                goto read_again;
                            }
                        }
                        else
                        {
                            // reading buffer, update counters
                            peer->p_n2recv -= nr;
                        }
                        if ( peer->p_n2recv < 0 )
                        {
                            char buf[MON_STRING_BUF_SIZE];
                            snprintf( buf, sizeof(buf),
                                "[%s@%d] error n2recv %d\n",
                                method_name, __LINE__, peer->p_n2recv );
                            mon_log_write( MON_CLUSTER_ALLGATHERSOCK_7, SQ_LOG_CRIT, buf );

                            mon_failure_exit();
                        }
                        if ( peer->p_n2recv == 0 )
                        {
                            // this buffer is done
                            peer->p_receiving = false;
                            nrecv++;
                            stats[indexToPnid_[iPeer]].count = peer->p_received;
                            if (trace_settings & TRACE_SYNC)
                            {
                                trace_printf( "%s@%d - EPOLLIN from %s(%d),"
                                              " sending=%d,"
                                              " receiving=%d (%d)"
                                              " sent=%d,"
                                              " received=%d"
                                              " timeout_count=%d,"
                                              " initial_check=%d,"
                                              " n2recv=%d\n"
                                            , method_name, __LINE__
                                            , Node[indexToPnid_[iPeer]]->GetName(), indexToPnid_[iPeer]
                                            , peer->p_sending
                                            , peer->p_receiving, n2get
                                            , peer->p_sent
                                            , peer->p_received
                                            , peer->p_timeout_count
                                            , peer->p_initial_check
                                            , peer->p_n2recv );
                            }
                            stateChange = true;
                        }
                    }
                }
            }
            if ( peer->p_sending  && events[iEvent].events & EPOLLOUT )
            { // Got send (write) completion
                char *s = &((char *)sbuf)[peer->p_sent];
                int n2send = nbytes - peer->p_sent;
                int ns;
                while ( 1 )
                {
                    if (trace_settings & TRACE_SYNC)
                    {
                        trace_printf( "%s@%d - EPOLLOUT to %s(%d),"
                                      " sending=%d (%d),"
                                      " receiving=%d"
                                      " sent=%d,"
                                      " received=%d"
                                      " timeout_count=%d,"
                                      " initial_check=%d,"
                                      " n2recv=%d\n"
                                    , method_name, __LINE__
                                    , Node[indexToPnid_[iPeer]]->GetName(), indexToPnid_[iPeer]
                                    , peer->p_sending, n2send
                                    , peer->p_receiving
                                    , peer->p_sent
                                    , peer->p_received
                                    , peer->p_timeout_count
                                    , peer->p_initial_check
                                    , peer->p_n2recv );
                    }
                    ns = send( fd, s, n2send, 0 );
                    if ( ns > 0 ) Meas.addSockAllGatherSentBytes( ns );
                    if ( ns >= 0 || errno != EINTR ) break;
                }
                if ( ns < 0 )
                {
                    // error, down socket
                    int err = errno;
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s@%d] send[%d](%d) error=%d (%s)\n"
                            , method_name, __LINE__
                            , indexToPnid_[iPeer], ns, err, strerror(err) );
                    mon_log_write( MON_CLUSTER_ALLGATHERSOCK_8, SQ_LOG_CRIT, buf );
                    peer->p_sending = false;
                    nsent++;
                    if ( peer->p_receiving )
                    {
                        peer->p_receiving = false;
                        nrecv++;
                    }
                    stats[indexToPnid_[iPeer]].MPI_ERROR = MPI_ERR_EXITED;
                    err = MPI_ERR_IN_STATUS;
                    stateChange = true;
                }
                else
                {
                    peer->p_sent += ns;
                    if ( peer->p_sent == nbytes )
                    {
                        // finished sending to this destination
                        peer->p_sending = false;
                        nsent++;
                        if (trace_settings & TRACE_SYNC)
                        {
                            trace_printf( "%s@%d - EPOLLOUT to %s(%d),"
                                          " sending=%d (%d),"
                                          " receiving=%d"
                                          " sent=%d,"
                                          " received=%d"
                                          " timeout_count=%d,"
                                          " initial_check=%d,"
                                          " n2recv=%d\n"
                                        , method_name, __LINE__
                                        , Node[indexToPnid_[iPeer]]->GetName(), indexToPnid_[iPeer]
                                        , peer->p_sending, n2send
                                        , peer->p_receiving
                                        , peer->p_sent
                                        , peer->p_received
                                        , peer->p_timeout_count
                                        , peer->p_initial_check
                                        , peer->p_n2recv );
                        }
                        stateChange = true;
                    }
                }
            }
early_exit:
            if ( stateChange && (socks_[indexToPnid_[iPeer]] != -1))
            {
                struct epoll_event event;
                event.data.fd = socks_[indexToPnid_[iPeer]];
                int op = 0;
                if ( !peer->p_sending && !peer->p_receiving )
                {
                    op = EPOLL_CTL_DEL;
                    event.events = 0;
                }
                else if ( peer->p_sending )
                {
                    op = EPOLL_CTL_MOD;
                    event.events = EPOLLOUT | EPOLLET | EPOLLRDHUP;
                }
                else if ( peer->p_receiving )
                {
                    op = EPOLL_CTL_MOD;
                    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                }
                if ( op == EPOLL_CTL_DEL || op == EPOLL_CTL_MOD )
                {
                    EpollCtl( epollFD_, op, fd, &event );
                    if (op == EPOLL_CTL_DEL
                     && stats[indexToPnid_[iPeer]].MPI_ERROR == MPI_ERR_EXITED)
                    {
                        CNode *node = Node[indexToPnid_[iPeer]];
                        if (trace_settings & (TRACE_SYNC |TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Node %s (%d) is not available, "
                                          "removing old socket from epoll set, "
                                          "socks_[%d]=%d\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , indexToPnid_[iPeer]
                                        , socks_[indexToPnid_[iPeer]] );

                        }
                        shutdown( socks_[indexToPnid_[iPeer]], SHUT_RDWR);
                        close( socks_[indexToPnid_[iPeer]] );
                        socks_[indexToPnid_[iPeer]] = -1;
                    }
                }
            }
            if (doReconnect)
            {
                reconnectSeqNum_ = seqNum_;
                reconnecting = true;
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d" " - Reconnecting! "
                                  "(lastReconnectErr=%d, reconnectSeqNum_=%lld)\n"
                                , method_name, __LINE__
                                , lastReconnectErr
                                , reconnectSeqNum_ );
                }
            }
        } // for (event)
    } // while ( 1 )

    MonStats->BarrierWaitDecr( );
    inBarrier_ = false;

    barrierCount_++;

    TRACE_EXIT;
    return err;
}

int CCluster::AllgatherSockReconnect( MPI_Status *stats, peer_t *peers, bool resetConnections )
{
    const char method_name[] = "CCluster::AllgatherSockReconnect";
    TRACE_ENTRY;

    int err = MPI_SUCCESS;
    int idst;
    int reconnectSock = -1;
    int zerr = ZOK;
    CNode *node;
    peer_t *peer;

    if( !IsRealCluster )
    { // In virtual cluster, just return success
        TRACE_EXIT;
        return( err );
    }
    
    // Release the sync lock temporarily to allow request worker thread to
    // process any request that needs the sync lock.
    Monitor->ExitSyncCycle();
    pthread_yield();

    if (resetConnections)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] Resetting sync port connections! (resetConnections=%d)\n"
                , method_name, __LINE__, resetConnections );
        mon_log_write( MON_CLUSTER_ALLGATHERSOCKRECONN_2, SQ_LOG_INFO, buf );
    }

    // Loop on each node in the cluster
    for ( int i = 0; i < GetConfigPNodesCount(); i++ )
    {
        // Loop on each adjacent node in the cluster
        for ( int j = i+1; j < GetConfigPNodesCount(); j++ )
        {
            if ( indexToPnid_[i] == MyPNID )
            { // Current indexToPnid_[i] node is my node, so connect to indexToPnid_[j] node

                idst = j;
                node = Nodes->GetNode( indexToPnid_[idst] );
                if (!node) continue;
                if (node->GetState() != State_Up)
                {
                    if (socks_[indexToPnid_[idst]] != -1)
                    { // Peer socket is still active
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Node %s (%d) is not up, "
                                          "removing old socket from epoll set, "
                                          "socks_[%d]=%d\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , indexToPnid_[idst], socks_[indexToPnid_[idst]] );
                        }
                        stats[indexToPnid_[idst]].MPI_ERROR = MPI_ERR_EXITED;
                        stats[indexToPnid_[idst]].count = 0;
                        err = MPI_ERR_IN_STATUS;
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                          "stats[%d].MPI_ERROR=%s\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , indexToPnid_[idst]
                                        , ErrorMsg(stats[indexToPnid_[idst]].MPI_ERROR) );
                        }

                        --currentNodes_;
                        // Clear bit in set of "up nodes"
                        upNodes_.upNodes[indexToPnid_[idst]/MAX_NODE_BITMASK] &= ~(1ull << (indexToPnid_[idst]%MAX_NODE_BITMASK));
            
                        // Remove old socket from epoll set, it may not be there
                        struct epoll_event event;
                        event.data.fd = socks_[indexToPnid_[idst]];
                        event.events = 0;
                        EpollCtlDelete( epollFD_, socks_[indexToPnid_[idst]], &event );
                        shutdown( socks_[indexToPnid_[idst]], SHUT_RDWR);
                        close( socks_[indexToPnid_[idst]] );
                        socks_[indexToPnid_[idst]] = -1;
                    }
                    continue;
                }
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - Pinging node %s (%d) to see if it's up, "
                                  "indexToPnid_[%d]=%d\n"
                                , method_name, __LINE__
                                , node->GetName(), node->GetPNid()
                                , idst, indexToPnid_[idst] );
                }
                peer = &peers[node->GetPNid()];
                if (PingSockPeer( node, peer->znodeFailedTime ))
                {
                    if (resetConnections)
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Connecting to node %s (%d), "
                                          "idst=%d, indexToPnid_[%d]=%d\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , idst, idst
                                        , indexToPnid_[idst] );
                        }
                        reconnectSock = ConnectSockPeer( node, indexToPnid_[idst], resetConnections );
                        if (reconnectSock == -1)
                        {
                            stats[indexToPnid_[idst]].MPI_ERROR = MPI_ERR_EXITED;
                            stats[indexToPnid_[idst]].count = 0;
                            err = MPI_ERR_IN_STATUS;
                            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                            {
                                trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                              "stats[%d].MPI_ERROR=%s\n"
                                            , method_name, __LINE__
                                            , node->GetName(), node->GetPNid()
                                            , indexToPnid_[idst]
                                            , ErrorMsg(stats[indexToPnid_[idst]].MPI_ERROR) );
                            }
                        }
                    }
                }
                else
                {
                    if ((ZClientEnabled && ZClient->IsRunningZNodeExpired( node->GetName(), zerr ))
                     || MyNode->IsPendingNodeDown()
                     || MyNode->GetState() != State_Up
                     || node->GetState()   != State_Up)
                    {
                        if (socks_[indexToPnid_[idst]] != -1)
                        {
                            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                            {
                                trace_printf( "%s@%d - Node %s (%d) is not up, "
                                              "removing old socket from epoll set, "
                                              "socks_[%d]=%d\n, zerr=%d"
                                            , method_name, __LINE__
                                            , node->GetName(), node->GetPNid()
                                            , indexToPnid_[idst], socks_[indexToPnid_[idst]] 
                                            , zerr );
                            }
    
                            --currentNodes_;
                            // Clear bit in set of "up nodes"
                            upNodes_.upNodes[indexToPnid_[idst]/MAX_NODE_BITMASK] &= ~(1ull << (indexToPnid_[idst]%MAX_NODE_BITMASK));
                
                            // Remove old socket from epoll set, it may not be there
                            struct epoll_event event;
                            event.data.fd = socks_[indexToPnid_[idst]];
                            event.events = 0;
                            EpollCtlDelete( epollFD_, socks_[indexToPnid_[idst]], &event );
                            shutdown( socks_[indexToPnid_[idst]], SHUT_RDWR);
                            close( socks_[indexToPnid_[idst]] );
                            socks_[indexToPnid_[idst]] = -1;
                        }
                        reconnectSock = -1;
                        stats[indexToPnid_[idst]].MPI_ERROR = MPI_ERR_EXITED;
                        stats[indexToPnid_[idst]].count = 0;
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                          "stats[%d].MPI_ERROR=%s\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , indexToPnid_[idst]
                                        , ErrorMsg(stats[indexToPnid_[idst]].MPI_ERROR) );
                        }
                    }
                    err = MPI_ERR_IN_STATUS;
                }
            }
            else if ( j == MyPNID )
            { // Current indexToPnid_[j] is my node, accept connection from peer indexToPnid_[i] node

                idst = i;
                node = Nodes->GetNode( indexToPnid_[idst] );
                if (!node) continue;
                if (node->GetState() != State_Up)
                {
                    if (socks_[indexToPnid_[idst]] != -1)
                    { // Peer socket is still active
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Node %s (%d) is not up, "
                                          "removing old socket from epoll set, "
                                          "socks_[%d]=%d\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , indexToPnid_[idst], socks_[indexToPnid_[idst]] );
                        }
                        stats[indexToPnid_[idst]].MPI_ERROR = MPI_ERR_EXITED;
                        stats[indexToPnid_[idst]].count = 0;
                        err = MPI_ERR_IN_STATUS;
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                          "stats[%d].MPI_ERROR=%s\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , indexToPnid_[idst]
                                        , ErrorMsg(stats[indexToPnid_[idst]].MPI_ERROR) );
                        }

                        --currentNodes_;
                        // Clear bit in set of "up nodes"
                        upNodes_.upNodes[indexToPnid_[idst]/MAX_NODE_BITMASK] &= ~(1ull << (indexToPnid_[idst]%MAX_NODE_BITMASK));
            
                        // Remove old socket from epoll set, it may not be there
                        struct epoll_event event;
                        event.data.fd = socks_[indexToPnid_[idst]];
                        event.events = 0;
                        EpollCtlDelete( epollFD_, socks_[indexToPnid_[idst]], &event );
                        shutdown( socks_[indexToPnid_[idst]], SHUT_RDWR);
                        close( socks_[indexToPnid_[idst]] );
                        socks_[indexToPnid_[idst]] = -1;
                    }
                    continue;
                }
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d - Pinging node %s (%d) to see if it's up\n"
                                , method_name, __LINE__
                                , node->GetName(), node->GetPNid() );
                }
                peer = &peers[node->GetPNid()];
                if (PingSockPeer( node, peer->znodeFailedTime ))
                {
                    if (resetConnections)
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Accepting from node %s (%d), "
                                          "idst=%d, indexToPnid_[%d]=%d\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , idst, idst
                                        , indexToPnid_[idst] );
                        }
                        reconnectSock = AcceptSockPeer( stats, resetConnections );
                        if (reconnectSock == -1)
                        {
                            err = MPI_ERR_IN_STATUS;
                        }
                    }
                }
                else
                {
                    if ((ZClientEnabled && ZClient->IsRunningZNodeExpired( node->GetName(), zerr ))
                     || MyNode->IsPendingNodeDown()
                     || MyNode->GetState() != State_Up
                     || node->GetState()   != State_Up)
                    {
                        if (socks_[indexToPnid_[idst]] != -1)
                        {
                            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                            {
                                trace_printf( "%s@%d - Node %s (%d) is not up, "
                                              "removing old socket from epoll set, "
                                              "socks_[%d]=%d, zerr=%d\n"
                                            , method_name, __LINE__
                                            , node->GetName(), node->GetPNid()
                                            , indexToPnid_[idst], socks_[indexToPnid_[idst]]
                                            , zerr );
                            }
    
                            --currentNodes_;
                            // Clear bit in set of "up nodes"
                            upNodes_.upNodes[indexToPnid_[idst]/MAX_NODE_BITMASK] &= ~(1ull << (indexToPnid_[idst]%MAX_NODE_BITMASK));
                
                            // Remove old socket from epoll set, it may not be there
                            struct epoll_event event;
                            event.data.fd = socks_[indexToPnid_[idst]];
                            event.events = 0;
                            EpollCtlDelete( epollFD_, socks_[indexToPnid_[idst]], &event );
                            shutdown( socks_[indexToPnid_[idst]], SHUT_RDWR);
                            close( socks_[indexToPnid_[idst]] );
                            socks_[indexToPnid_[idst]] = -1;
                        }
                        reconnectSock = -1;
                        stats[indexToPnid_[idst]].MPI_ERROR = MPI_ERR_EXITED;
                        stats[indexToPnid_[idst]].count = 0;
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                          "stats[%d].MPI_ERROR=%s\n"
                                        , method_name, __LINE__
                                        , node->GetName(), node->GetPNid()
                                        , indexToPnid_[idst]
                                        , ErrorMsg(stats[indexToPnid_[idst]].MPI_ERROR) );
                        }
                    }
                    err = MPI_ERR_IN_STATUS;
                }
            }
            else
            {
                idst = -1;
            }
            if ( idst >= 0
              && reconnectSock != -1
              && socks_[indexToPnid_[idst]] != -1
              && fcntl( socks_[indexToPnid_[idst]], F_SETFL, O_NONBLOCK ) )
            {
                err = MPI_ERR_AMODE;
                char ebuff[256];
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf), "[%s@%d] fcntl(socks_[%d]=%d,F_SETFL,NONBLOCK) error: %s\n",
                    method_name, __LINE__,indexToPnid_[indexToPnid_[idst]], socks_[indexToPnid_[idst]], strerror_r( errno, ebuff, 256 ) );
                mon_log_write( MON_CLUSTER_ALLGATHERSOCKRECONN_1, SQ_LOG_CRIT, buf );
            }
        }
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        for ( int i = 0; i < GetConfigPNodesCount(); i++ )
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d" " - socks_[%d]=%d, "
                              "stats[%d].MPI_ERROR=%s\n"
                            , method_name, __LINE__
                            , indexToPnid_[i]
                            , socks_[indexToPnid_[i]]
                            , indexToPnid_[i]
                            , ErrorMsg(stats[indexToPnid_[i]].MPI_ERROR) );
            }
        }
        trace_printf( "%s@%d - Returning err=%d(%s)\n"
                    , method_name, __LINE__, err, ErrorMsg(err) );
    }

    Monitor->EnterSyncCycle();

    TRACE_EXIT;
    return( err );
}

int CCluster::AcceptSockPeer( MPI_Status *stats, bool resetConnections )
{
    const char method_name[] = "CCluster::AcceptSockPeer";
    TRACE_ENTRY;

    int rc = MPI_SUCCESS;
    int reconnectSock = -1;
    struct hostent *he;

    // Get my host structure via my node name
    he = gethostbyname( MyNode->GetName() );
    if ( !he )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] gethostbyname(%s) error: %s\n"
                , method_name, __LINE__
                , MyNode->GetName()
                , strerror_r( h_errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_ACCEPTSOCKPEER_1, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
    else
    {
        if (trace_settings & TRACE_RECOVERY)
        {
            trace_printf( "%s@%d Accepting server socket on port=%d\n"
                        , method_name, __LINE__
                        , MyNode->GetSyncSocketPort() );
        }

        // Accept connection from peer
        reconnectSock = AcceptSock( syncSock_ );
        if (reconnectSock < 0)
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s@%d] AcceptSock(%d) failed!\n",
                method_name, __LINE__, syncSock_ );
            mon_log_write( MON_CLUSTER_ACCEPTSOCKPEER_2, SQ_LOG_ERR, buf );
            rc = -1;
        }

        if (rc != -1 && resetConnections)
        {
            if (reconnectSock > -1)
            {
                nodeSyncInfo_t readSyncInfo;
                // Get info about connecting monitor
                rc = ReceiveSock( (char *) &readSyncInfo
                                , sizeof(nodeSyncInfo_t)
                                , reconnectSock
                                , method_name );
                if ( rc )
                {   // Handle error
                    shutdown( reconnectSock, SHUT_RDWR);
                    close( (int)reconnectSock );

                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], unable to obtain node sync infor from remote"
                              "monitor: %s.\n"
                            , method_name, ErrorMsg(rc));
                    mon_log_write( MON_CLUSTER_ACCEPTSOCKPEER_4, SQ_LOG_ERR, buf );
                }
                else
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d - Received remote SyncInfo.pnid=%d, "
                                      "SyncInfo.nodeName=%s, "
                                      "SyncInfo.seqNum=%lld, "
                                      "SyncInfo.reconnectSeqNum=%lld\n"
                                    , method_name, __LINE__
                                    , readSyncInfo.pnid
                                    , readSyncInfo.nodeName
                                    , readSyncInfo.seqNum
                                    , readSyncInfo.reconnectSeqNum );
                    }

                    CNode *acceptedNode = Nodes->GetNode( readSyncInfo.pnid );
                    if (!acceptedNode)
                    {
                        shutdown( reconnectSock, SHUT_RDWR);
                        close( (int)reconnectSock );

                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf), "[%s@%d] AcceptSock(%d) failed!\n",
                            method_name, __LINE__, syncSock_ );
                        mon_log_write( MON_CLUSTER_ACCEPTSOCKPEER_2, SQ_LOG_ERR, buf );
                        return(-1);
                    }

                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s@%d] Resetting remote connection with %s(%d)\n"
                            , method_name, __LINE__
                            , acceptedNode->GetName(), acceptedNode->GetPNid() );
                    mon_log_write( MON_CLUSTER_ACCEPTSOCKPEER_3, SQ_LOG_INFO, buf );

                    nodeSyncInfo_t writeSyncInfo;

                    strcpy(writeSyncInfo.nodeName, MyNode->GetName());
                    writeSyncInfo.pnid = MyPNID;
                    writeSyncInfo.seqNum = seqNum_;
                    writeSyncInfo.reconnectSeqNum = reconnectSeqNum_;
                    rc = SendSock( (char *) &writeSyncInfo
                                 , sizeof(nodeSyncInfo_t)
                                 , reconnectSock
                                 , method_name );
                    if ( rc )
                    {
                        shutdown( reconnectSock, SHUT_RDWR);
                        close( (int)reconnectSock );
    
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                , "[%s], Cannot send sync node info to node %s: (%s)\n"
                                , method_name
                                , acceptedNode->GetName(), ErrorMsg(rc));
                        mon_log_write(MON_CLUSTER_ACCEPTSOCKPEER_5, SQ_LOG_ERR, buf);
                    }
                    else
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Sent my SyncInfo.pnid=%d, "
                                          "SyncInfo.nodeName=%s, "
                                          "SyncInfo.seqNum=%lld, "
                                          "SyncInfo.reconnectSeqNum=%lld\n"
                                        , method_name, __LINE__
                                        , writeSyncInfo.pnid
                                        , writeSyncInfo.nodeName
                                        , writeSyncInfo.seqNum
                                        , writeSyncInfo.reconnectSeqNum );
                        }

                        if (trace_settings & TRACE_RECOVERY)
                        {
                            trace_printf( "%s@%d Server %s(%d) accepted from client %s(%d), old socks_[%d]=%d, new socks_[%d]=%d\n"
                                        , method_name, __LINE__
                                        , MyNode->GetName(), MyPNID
                                        , acceptedNode->GetName(), acceptedNode->GetPNid()
                                        , acceptedNode->GetPNid(), socks_[acceptedNode->GetPNid()]
                                        , acceptedNode->GetPNid(), reconnectSock);
                        }

                        if (socks_[acceptedNode->GetPNid()] != -1)
                        {
                            // Remove old socket from epoll set, it may not be there
                            struct epoll_event event;
                            event.data.fd = socks_[acceptedNode->GetPNid()];
                            event.events = 0;
                            EpollCtlDelete( epollFD_, socks_[acceptedNode->GetPNid()], &event );
                            if (acceptedNode->GetState() != State_Up)
                            {
                                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                                {
                                    trace_printf( "%s@%d - Node %s (%d) is not up, "
                                                  "removing old socket from epoll set, "
                                                  "socks_[%d]=%d\n"
                                                , method_name, __LINE__
                                                , acceptedNode->GetName(), acceptedNode->GetPNid()
                                                , acceptedNode->GetPNid(), socks_[acceptedNode->GetPNid()] );
                                }
                                shutdown( socks_[acceptedNode->GetPNid()], SHUT_RDWR);
                                close( socks_[acceptedNode->GetPNid()] );
                                socks_[acceptedNode->GetPNid()] = -1;
                                stats[acceptedNode->GetPNid()].MPI_ERROR = MPI_ERR_EXITED;
                                stats[acceptedNode->GetPNid()].count = 0;

                                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                                {
                                    trace_printf( "%s@%d - Setting Node %s (%d) status to "
                                                  "stats[%d].MPI_ERROR=%s\n"
                                                , method_name, __LINE__
                                                , acceptedNode->GetName(), acceptedNode->GetPNid()
                                                , acceptedNode->GetPNid()
                                                , ErrorMsg(stats[acceptedNode->GetPNid()].MPI_ERROR) );
                                }
                            }
                            else
                            {
                                socks_[acceptedNode->GetPNid()] = reconnectSock; // AcceptSockPeer
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (reconnectSock > -1)
            {
                shutdown( reconnectSock, SHUT_RDWR);
                close( (int)reconnectSock );
            }
        }
    }


    TRACE_EXIT;
    return rc;
}

int CCluster::ConnectSockPeer( CNode *node, int peer, bool resetConnections )
{
    const char method_name[] = "CCluster::ConnectSockPeer";
    TRACE_ENTRY;

    int rc = MPI_SUCCESS;
    int reconnectSock = -1;
    unsigned char srcaddr[4], dstaddr[4];
    struct hostent *he;

    // Get my host structure via my node name
    he = gethostbyname( MyNode->GetName() );
    if ( !he )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] gethostbyname(%s) error: %s\n"
                , method_name, __LINE__
                , MyNode->GetName()
                , strerror_r( h_errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_CONNECTSOCKPEER_1, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
    else
    {
        // Initialize my source address structure
        memcpy( srcaddr, he->h_addr, 4 );
        // Get peer's host structure via its node name
        he = gethostbyname( node->GetName() );
        if ( !he )
        {
            char ebuff[256];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                "[%s@%d] gethostbyname(%s) error: %s\n",
                method_name, __LINE__, node->GetName(),
                strerror_r( h_errno, ebuff, 256 ) );
            mon_log_write( MON_CLUSTER_CONNECTSOCKPEER_2, SQ_LOG_CRIT, buf );

            mon_failure_exit();
        }
        // Initialize peer's destination address structure
        memcpy( dstaddr, he->h_addr, 4 );

        if (trace_settings & TRACE_RECOVERY)
        {
            trace_printf( "%s@%d Creating client socket: src=%d.%d.%d.%d, "
                          "dst(%s)=%d.%d.%d.%d, dst port=%d\n"
                        , method_name, __LINE__
                        , (int)((unsigned char *)srcaddr)[0]
                        , (int)((unsigned char *)srcaddr)[1]
                        , (int)((unsigned char *)srcaddr)[2]
                        , (int)((unsigned char *)srcaddr)[3]
                        ,  node->GetName()
                        , (int)((unsigned char *)dstaddr)[0]
                        , (int)((unsigned char *)dstaddr)[1]
                        , (int)((unsigned char *)dstaddr)[2]
                        , (int)((unsigned char *)dstaddr)[3]
                        , sockPorts_[peer] );
        }
        // Connect to peer
        reconnectSock = MkCltSock( srcaddr, dstaddr, sockPorts_[peer] );
        if (reconnectSock > -1)
        {
            if (trace_settings & TRACE_RECOVERY)
            {
                trace_printf( "%s@%d Client %s(%d) connected to server %s(%d), old socks_[%d]=%d, new socks_[%d]=%d\n"
                            , method_name, __LINE__
                            , MyNode->GetName(), MyPNID
                            , node->GetName(), node->GetPNid()
                            , peer, socks_[peer]
                            , peer, reconnectSock);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s@%d] MkCltSock() src=%d.%d.%d.%d, "
                      "dst(%s)=%d.%d.%d.%d failed!\n"
                    , method_name, __LINE__
                    , (int)((unsigned char *)srcaddr)[0]
                    , (int)((unsigned char *)srcaddr)[1]
                    , (int)((unsigned char *)srcaddr)[2]
                    , (int)((unsigned char *)srcaddr)[3]
                    ,  node->GetName()
                    , (int)((unsigned char *)dstaddr)[0]
                    , (int)((unsigned char *)dstaddr)[1]
                    , (int)((unsigned char *)dstaddr)[2]
                    , (int)((unsigned char *)dstaddr)[3] );
            mon_log_write( MON_CLUSTER_CONNECTSOCKPEER_3, SQ_LOG_ERR, buf );
            rc = -1;
        }

        if (rc != -1 && resetConnections)
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s@%d] Resetting remote connection with %s(%d)\n"
                    , method_name, __LINE__
                    , node->GetName(), node->GetPNid() );
            mon_log_write( MON_CLUSTER_CONNECTSOCKPEER_4, SQ_LOG_INFO, buf );

            if (socks_[peer] != -1)
            {
                // Remove old socket from epoll set, it may not be there
                struct epoll_event event;
                event.data.fd = socks_[peer];
                event.events = 0;
                EpollCtlDelete( epollFD_, socks_[peer], &event );
                if (node->GetState() != State_Up)
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d - Node %s (%d) is not up, "
                                      "removing old socket from epoll set, "
                                      "socks_[%d]=%d\n"
                                    , method_name, __LINE__
                                    , node->GetName(), node->GetPNid()
                                    , peer, socks_[peer] );
                    }
                    shutdown( socks_[peer], SHUT_RDWR);
                    close( socks_[peer] );
                    socks_[peer] = -1;
                }
            }

            if (reconnectSock > -1)
            {
                nodeSyncInfo_t writeSyncInfo;

                strcpy(writeSyncInfo.nodeName, MyNode->GetName());
                writeSyncInfo.pnid = MyPNID;
                writeSyncInfo.seqNum = seqNum_;
                writeSyncInfo.reconnectSeqNum = reconnectSeqNum_;
                rc = SendSock( (char *) &writeSyncInfo
                             , sizeof(nodeSyncInfo_t)
                             , reconnectSock
                             , method_name );
                if ( rc )
                {
                    shutdown( reconnectSock, SHUT_RDWR);
                    close( (int)reconnectSock );

                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], Cannot send sync node info to node %s: (%s)\n"
                            , method_name
                            , node?node->GetName():"", ErrorMsg(rc));
                    mon_log_write(MON_CLUSTER_CONNECTSOCKPEER_5, SQ_LOG_ERR, buf);    
                }
                else
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d - Sent my SyncInfo.pnid=%d, "
                                      "SyncInfo.nodeName=%s, "
                                      "SyncInfo.seqNum=%lld, "
                                      "SyncInfo.reconnectSeqNum=%lld\n"
                                    , method_name, __LINE__
                                    , writeSyncInfo.pnid
                                    , writeSyncInfo.nodeName
                                    , writeSyncInfo.seqNum
                                    , writeSyncInfo.reconnectSeqNum );
                    }
                    nodeSyncInfo_t readSyncInfo;
                    // Get info about connecting monitor
                    rc = ReceiveSock( (char *) &readSyncInfo
                                    , sizeof(nodeSyncInfo_t)
                                    , reconnectSock
                                    , method_name );
                    if ( rc )
                    {   // Handle error
                        shutdown( reconnectSock, SHUT_RDWR);
                        close( (int)reconnectSock );

                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf)
                                , "[%s], unable to obtain node sync infor from remote"
                                  "monitor: %s.\n"
                                , method_name, ErrorMsg(rc));
                        mon_log_write(MON_CLUSTER_CONNECTSOCKPEER_6, SQ_LOG_ERR, buf);    
                    }
                    else
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        {
                            trace_printf( "%s@%d - Received remote SyncInfo.pnid=%d, "
                                          "SyncInfo.nodeName=%s, "
                                          "SyncInfo.seqNum=%lld, "
                                          "SyncInfo.reconnectSeqNum=%lld\n"
                                        , method_name, __LINE__
                                        , readSyncInfo.pnid
                                        , readSyncInfo.nodeName
                                        , readSyncInfo.seqNum
                                        , readSyncInfo.reconnectSeqNum );
                        }
                        socks_[peer] = reconnectSock; // ConnectSockPeer
                    }
                }
            }
        }
        else
        {
            if (reconnectSock > -1)
            {
                shutdown( reconnectSock, SHUT_RDWR);
                close( (int)reconnectSock );
            }
        }
    }

    TRACE_EXIT;
    return( rc );
}

// When we get a communication error for a point-to-point monitor communicator
// verify that the other nodes in the cluster also lost communications
// with that monitor.  If all nodes concur we consider that monitor
// down.
void CCluster::ValidateClusterState( cluster_state_def_t nodestate[],
                                     bool haveDivergence)
{
    const char method_name[] = "CCluster::ValidateClusterState";

    exitedMons_t::iterator it;
    upNodes_t nodeMask;

    for ( int i =0; i < MAX_NODE_MASKS ; i++ )
    {
        nodeMask.upNodes[i] = 0;
    }

    for ( it = exitedMons_.begin(); it != exitedMons_.end(); )
    {
        if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
        {
            trace_printf("%s@%d checking exited pnid=%d, detecting pnid=%d, seqNum=%lld"
                         " (current seqNum_=%lld)\n", method_name, __LINE__,
                         it->exitedPnid, it->detectingPnid, it->seqNum, seqNum_);
        }

        if ( seqNum_ >= (it->seqNum + 2) )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] Validating exited node %d, "
                      "detected by node %d at seq #%lld "
                      "(current seq # is %lld).\n",
                      method_name, it->exitedPnid, it->detectingPnid,
                      it->seqNum, seqNum_);
            mon_log_write(MON_CLUSTER_VALIDATE_STATE_1, SQ_LOG_ERR, buf);

            int concurringNodes = 0;

            // Check if all active nodes see the node as down.
            nodeMask.upNodes[it->exitedPnid/MAX_NODE_BITMASK] = 1ull << (it->exitedPnid%MAX_NODE_BITMASK);
            string setSeesUp;
            string setSeesDown;
            char nodeX[10];

            // Evaluate each active (up) node in the cluster
            int pnodesCount = 0;
            for (int index = 0;
                 index < GetConfigPNodesMax() && pnodesCount < currentNodes_;
                 ++index)
            {
                if ( nodestate[index].seq_num != 0 )
                {  // There is valid nodestate info from node "index"

                    pnodesCount++;

                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    {
                        trace_printf( "%s@%d down pnid= %d: nodestate[%d].nodeMask.upNodes[%d]=%llx, downNodeMask.upNodes[%d]=%llx\n"
                                    , method_name, __LINE__
                                    , it->exitedPnid
                                    , index, (it->exitedPnid/MAX_NODE_BITMASK)
                                    , nodestate[index].nodeMask.upNodes[it->exitedPnid/MAX_NODE_BITMASK]
                                    , (index/MAX_NODE_BITMASK)
                                    , nodeMask.upNodes[it->exitedPnid/MAX_NODE_BITMASK] );
                    }

                    if ((nodestate[index].nodeMask.upNodes[it->exitedPnid/MAX_NODE_BITMASK] &
                         nodeMask.upNodes[it->exitedPnid/MAX_NODE_BITMASK]) == 0)
                    {  // Node "pnid" sees the node as down

                        // temp trace
                        if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                        {
                            trace_printf("%s@%d node %d concurs that node %d "
                                         "is down\n", method_name, __LINE__,
                                         /*indexToPnid_[index]*/ index, it->exitedPnid);
                        }

                        snprintf(nodeX, sizeof(nodeX), "%d, ", /*indexToPnid_[index]*/ index);
                        setSeesDown.append(nodeX);

                        ++concurringNodes;
                    }
                    else
                    {
                        // temp trace
                        if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                        {
                            trace_printf("%s@%d node %d says node %d is up\n",
                                         method_name, __LINE__, /*indexToPnid_[index]*/ index,
                                         it->exitedPnid);
                        }

                        snprintf(nodeX, sizeof(nodeX), "%d, ", /*indexToPnid_[index]*/ index);
                        setSeesUp.append(nodeX);

                    }
                }
                else
                {
                    // temp trace
                    if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                    {
                        trace_printf("%s@%d ignoring state from node %d\n",
                                     method_name, __LINE__, /*indexToPnid_[index]*/ index);
                    }
                }
            }

            if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
            {
                trace_printf("%s@%d concurringNodes=%d, currentNodes_=%d\n",
                             method_name, __LINE__, concurringNodes, currentNodes_);
            }

            if (concurringNodes == currentNodes_)
            {   // General agreement that node is down, proceed to mark it down

                CNode *downNode = Nodes->GetNode( it->exitedPnid );
                if (downNode && downNode->GetState() != State_Down)
                {
                    // temp trace
                    if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                    {
                        trace_printf("%s@%d proceeding to mark node %d down\n",
                                     method_name, __LINE__, it->exitedPnid);
                    }

                    mem_log_write(CMonLog::MON_UPDATE_CLUSTER_3, it->exitedPnid);

                    HandleDownNode(it->exitedPnid);
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                    {
                        trace_printf("%s@%d Node pnid=%d is already down\n"
                                    , method_name, __LINE__
                                    , it->exitedPnid);
                    }
                }
            }
            else if ( concurringNodes != 0 && !enqueuedDown_ )
            {   // Some monitors say the node is down, others don't.
                // This is not supposed to happen.  Enqueue request to
                // bring this node down.  All monitors will do the same
                // so the cluster will be brought down.
                if (setSeesUp.length() > 2)
                    setSeesUp.erase(setSeesUp.length()-2, 2);
                if (setSeesDown.length() > 2)
                    setSeesDown.erase(setSeesDown.length()-2, 2);
                char buf[MON_STRING_BUF_SIZE*2];
                snprintf( buf, sizeof(buf), "[%s] Lost connection to node "
                          "%d but only %d of %d nodes also lost the "
                          "connection.  See up: %s.  See down: %s.  So node "
                          "%d is going down (at seq #%lld).\n", method_name,
                          it->exitedPnid, concurringNodes, currentNodes_,
                          setSeesUp.c_str(), setSeesDown.c_str(),
                          MyPNID, seqNum_ );
                mon_log_write(MON_CLUSTER_VALIDATE_STATE_2, SQ_LOG_ERR, buf);

                mem_log_write(CMonLog::MON_UPDATE_CLUSTER_4, MyPNID,
                              it->exitedPnid);

                enqueuedDown_ = true;
                ReqQueue.enqueueDownReq(MyPNID);
            }

            if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
            {
                trace_printf("%s@%d removing exited pnid=%d, detecting pnid=%d, seqNum=%lld"
                             " (current seqNum_=%lld)\n", method_name, __LINE__,
                             it->exitedPnid, it->detectingPnid, it->seqNum, seqNum_);
            }
            // Delete current list element and advance to next one
            it = exitedMons_.erase( it );
        }
        else
        {   // Advance to next list element
            ++it;
        }
    }


    if ( haveDivergence )
    {
        for ( int i =0; i < MAX_NODE_MASKS ; i++ )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] Cluster view divergence (at seq #%lld), "
                      "node %d sees set[%d]: %llx\n"
                    , method_name, seqNum_, MyPNID, i
                    , upNodes_.upNodes[i] );
            mon_log_write(MON_CLUSTER_VALIDATE_STATE_3, SQ_LOG_ERR, buf);
        }

        // For each "up node" (from local perspective)
        // go through nodestate for each other node. If any node
        // says the node is down, add an item to the exitedMons_ list
        // for examination during the next sync cycle (by which time
        // all nodes will have had a chance to detect the down monitor.)

        int pnodesCount2 = 0;
        for (int remIndex = 0;
             remIndex < GetConfigPNodesMax() && pnodesCount2 < currentNodes_;
             ++remIndex)
        {
            bool someExited = false;
            // No need to check local monitor's view of the cluster since
            // any down connections are handled directly when detected.
            if (/*indexToPnid_[remIndex]*/remIndex == MyPNID)
            {
                pnodesCount2++;
                continue;
            }

            // No need to check a remote monitor's view when node is down
            CNode *remoteNode = Nodes->GetNode( /*indexToPnid_[remIndex]*/remIndex );
            if ( ! remoteNode )
            {   //  node is not member of cluster
                if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                {
                    trace_printf("%s@%d Skipping non-existing node "
                                 "pnid=%d\n",
                                 method_name, __LINE__,
                                 /*indexToPnid_[remIndex]*/remIndex);
                }
                continue;
            }
            else if (remoteNode->GetState() == State_Down)
            {   //  node is down
                if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                {
                    trace_printf("%s@%d Skipping down node "
                                 "pnid=%d (%s)\n",
                                 method_name, __LINE__,
                                 /*indexToPnid_[remIndex]*/remIndex, remoteNode->GetName());
                }
                continue;
            }
            else
            {
                pnodesCount2++;
            }

            // Check if all active nodes see the node as up.
            nodeMask.upNodes[/*indexToPnid_[remIndex]*/remIndex/MAX_NODE_BITMASK] =
                1ull << (/*indexToPnid_[remIndex]*/remIndex%MAX_NODE_BITMASK);

            if ( upNodes_.upNodes[/*indexToPnid_[remIndex]*/remIndex/MAX_NODE_BITMASK] &
                 nodeMask.upNodes[/*indexToPnid_[remIndex]*/remIndex/MAX_NODE_BITMASK] )
            {  // This remote node sees node pnid as up
                int pnodesCount3 = 0;
                for (int exitedPNid = 0;
                     exitedPNid < GetConfigPNodesMax() && pnodesCount3 < currentNodes_;
                     ++exitedPNid)
                {
                    CNode *exitedNode = Nodes->GetNode( /*indexToPnid_[remIndex]*/exitedPNid );
                    if (  exitedNode &&
                         (/*indexToPnid_[remIndex]*/remIndex != exitedPNid) &&
                         (nodestate[remIndex].seq_num != 0) &&
                         (nodestate[exitedPNid].nodeMask.upNodes[/*indexToPnid_[remIndex]*/remIndex/MAX_NODE_BITMASK] &
                          nodeMask.upNodes[/*indexToPnid_[remIndex]*/remIndex/MAX_NODE_BITMASK]) == 0 )
                    {  // Node remIndex sees exitedPNid as down

                        pnodesCount3++;

                        if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                        {
                            trace_printf("%s@%d Divergence, queueing "
                                         "monExited{%d, %d, %lld}\n",
                                         method_name, __LINE__, exitedPNid, /*indexToPnid_[remIndex]*/remIndex,
                                         seqNum_);
                        }

                        someExited = true;
                        monExited_t monExited = {exitedPNid, /*indexToPnid_[remIndex]*/remIndex, seqNum_};
                        exitedMons_.push_back( monExited );
                    }
                }
            }
            if (someExited)
            {
                // No need to look further for any other
                // monitor's view of node pnid.  When the
                // exitedMons_ element is processed all nodes
                // will be checked for concurrence.
                break;
            }
        }
    }
}

bool CCluster::ValidateSeqNum( cluster_state_def_t nodestate[] )
{
    const char method_name[] = "CCluster::ValidateSeqNum";

    unsigned long long seqNum;
    unsigned long long loSeqNum = seqNum_;
    unsigned long long hiSeqNum = seqNum_;
    unsigned long long seqNumBucket[MAX_NODES];
    int seqNumCount[MAX_NODES];
    int maxBucket = 0;
    bool found;
    int mostCountsIndex;

    if ( GetConfigPNodesCount() ==  1 ) return true;

    // Count occurrences of sequence numbers
    for (int pnid = 0; pnid < GetConfigPNodesMax(); pnid++)
    {
        CNode *node = Nodes->GetNode( pnid );
        if (!node) continue;
        if (node->GetState() != State_Up) continue;

        if ( pnid == MyPNID )
        {
            seqNum = nodestate[pnid].seq_num = seqNum_;
        }
        else
        {
            seqNum = nodestate[pnid].seq_num;
        }

        if (trace_settings & TRACE_SYNC)
        {
            trace_printf( "%s@%d seqNum_=%lld, nodestate[%d].seq_num=%lld\n"
                        , method_name, __LINE__
                        , seqNum_
                        , pnid
                        , nodestate[pnid].seq_num );
        }

        if (seqNum != 0)
        {
            loSeqNum = (seqNum < loSeqNum) ? seqNum : loSeqNum;
            hiSeqNum = (seqNum > hiSeqNum) ? seqNum : hiSeqNum;

            found = false;
            for (int i=0; i<maxBucket; ++i)
            {
                if ( seqNum == seqNumBucket[i] )
                {
                    ++seqNumCount[i];
                    found = true;
                    break;
                }
            }
            if ( ! found )
            {
                seqNumBucket[maxBucket] = seqNum;
                seqNumCount[maxBucket] = 1;
                ++maxBucket;
            }
        }
    }

    if ( maxBucket == 0 )
    {  // Normal case, all nodes have same sequence number
        mostCountsIndex = 0;
    }
    else
    {  // Look for majority sequence number
        int mostCounts = 0;
        mostCountsIndex = 0;
        for (int i=0; i<maxBucket; ++i)
        {
            if ( seqNumCount[i] > mostCounts )
            {
                mostCounts = seqNumCount[i];
                mostCountsIndex = i;
            }
        }
    }

    lowSeqNum_  = loSeqNum;
    highSeqNum_ = hiSeqNum;

    if (trace_settings & TRACE_SYNC)
    {
        if ( lowSeqNum_ != highSeqNum_ )
        {
            trace_printf( "%s@%d Most common seq num=%lld (%d nodes), "
                          "%d buckets, low=%lld, high=%lld, local seq num (%lld) did not match.\n"
                         , method_name, __LINE__
                         , seqNumBucket[mostCountsIndex]
                         , seqNumCount[mostCountsIndex]
                         , maxBucket
                         , lowSeqNum_
                         , highSeqNum_
                         , seqNum_ );
        }
    }

    // Fail if any sequence number does not match
    return( lowSeqNum_ == highSeqNum_ );
}

void CCluster::HandleDownNode( int pnid )
{
    const char method_name[] = "CCluster::HandleDownNode";
    TRACE_ENTRY;

    // Add to dead node name list
    CNode *downNode = Nodes->GetNode( pnid );
    assert(downNode);
    if (downNode->GetState() != State_Down)
    {
        downNode->SetPendingNodeDown(true);
    }
    deadNodeList_.push_back( downNode );

    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d - Added down node to list, pnid=%d, name=(%s)\n", method_name, __LINE__, downNode->GetPNid(), downNode->GetName());

    // Build available list of spare nodes
    CNode *spareNode;
    NodesList *spareNodesList = Nodes->GetSpareNodesList();
    NodesList::iterator itSn;
    for ( itSn = spareNodesList->begin(); itSn != spareNodesList->end() ; itSn++ )
    {
        spareNode = *itSn;
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d - %s (pnid=%d) is in available spare node list, state=%s, spare=%d, rank failure=%d\n"
                        , method_name, __LINE__, spareNode->GetName(), spareNode->GetPNid()
                        , StateString(spareNode->GetState()), spareNode->IsSpareNode(), spareNode->IsRankFailure());
        // if spare node is available
        if ( spareNode->IsSpareNode()    &&
             !spareNode->IsRankFailure() &&
             spareNode->GetState() == State_Up )
        {
            spareNodeVector_.push_back( spareNode );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf("%s@%d - pnid=%d, name=(%s) is available Spare\n", method_name, __LINE__, spareNode->GetPNid(), spareNode->GetName());
        }
    }

    // Activate spare or down node
    NodesList::iterator itDn;
    for ( itDn = deadNodeList_.begin(); itDn != deadNodeList_.end() ; itDn++ )
    {
        downNode = *itDn;
        if ( Emulate_Down )
        {
            ReqQueue.enqueueDownReq( downNode->GetPNid() );
        }
        else
        {
            bool done = false;
            spareNode = NULL;
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf( "%s@%d - spare node vector size=%ld\n"
                            , method_name, __LINE__, spareNodeVector_.size());
            // Find available spare node for current down node
            for ( unsigned int ii = 0; ii < spareNodeVector_.size() && !done ; ii++ )
            {
                PNidVector sparePNids = spareNodeVector_[ii]->GetSparePNids();
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                    trace_printf( "%s@%d - spare pnids vector size=%ld\n"
                                , method_name, __LINE__, sparePNids.size());
                // Check each pnid it is configured to spare
                for ( unsigned int jj = 0; jj < sparePNids.size(); jj++ )
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        trace_printf( "%s@%d - %s (pnid=%d) is in spare node vector[%d], size=%ld\n"
                                    , method_name, __LINE__
                                    , spareNodeVector_[ii]->GetName()
                                    , spareNodeVector_[ii]->GetPNid()
                                    , jj, sparePNids.size());
                    // if this is a spare for the down node
                    if ( spareNodeVector_[ii]->IsSpareNode() &&
                         downNode->GetPNid() == sparePNids[jj] )
                    {
                        // assign it and remove it from the vector
                        spareNode = spareNodeVector_[ii];
                        spareNodeVector_.erase( spareNodeVector_.begin() + ii );
                        done = true;
                        break;
                    }
                }
            }

            if ( spareNode )
            {
                Nodes->RemoveFromSpareNodesList( spareNode );
                downNode->SetState( State_Takeover ); // change state so that pending requests could fail.
                spareNode->SetActivatingSpare( true );
                if ( spareNode->GetPNid() == MyPNID )
                {
                    ReqQueue.enqueueActivateSpareReq( spareNode, downNode, true );
                }
            }
            else
            {
                if ( downNode->IsSpareNode() )
                {
                    Nodes->RemoveFromSpareNodesList( downNode );
                }
                ReqQueue.enqueueDownReq( downNode->GetPNid() );
            }
        }
    }

    spareNodeVector_.clear();
    deadNodeList_.clear();

    TRACE_EXIT;
}

void CCluster::UpdateClusterState( bool &doShutdown,
                                   struct sync_buffer_def * syncBuf,
                                   MPI_Status *status,
                                   int sentChangeNid)
{
    const char method_name[] = "CCluster::UpdateClusterState";
    TRACE_ENTRY;

    struct sync_buffer_def *recvBuf;
#ifndef NAMESERVER_PROCESS
    struct sync_buffer_def *sendBuf = Nodes->GetSyncBuffer();
#endif
    STATE node_state;
    int change_nid;
    cluster_state_def_t nodestate[GetConfigPNodesMax()];
    bool clusterViewDivergence = false;


    // Populate nodestate array using node state info from "allgather"
    // along with local node state.
    for (int index = 0; index < GetConfigPNodesCount(); index++)
    {
        // Only process active nodes
        bool noComm;
        switch( CommType )
        {
            case CommType_InfiniBand:
                noComm = (comms_[indexToPnid_[index]] == MPI_COMM_NULL) ? true : false;
                break;
            case CommType_Sockets:
                noComm = (socks_[indexToPnid_[index]] == -1) ? true : false;
                break;
            default:
                // Programmer bonehead!
                abort();
        }

        if (noComm
         || status[indexToPnid_[index]].MPI_ERROR != MPI_SUCCESS)
        {
            if (trace_settings & (TRACE_RECOVERY | TRACE_INIT))
            {
                if (!noComm)
                {
                    trace_printf( "%s@%d - Communication error from node %d, "
                                  " seq_num=#%lld\n"
                                , method_name, __LINE__, indexToPnid_[index]
                                , seqNum_ );
                }
            }
            // Not an active node, set default values
            nodestate[indexToPnid_[index]].node_state = State_Unknown;
            nodestate[indexToPnid_[index]].change_nid = -1;
            nodestate[indexToPnid_[index]].seq_num     = 0;
            for ( int i =0; i < MAX_NODE_MASKS ; i++ )
            {
                nodestate[indexToPnid_[index]].nodeMask.upNodes[i] = 0;
            }
#ifdef NAMESERVER_PROCESS
            nodestate[indexToPnid_[index]].monConnCount = -1;
#else
            nodestate[indexToPnid_[index]].monProcCount = 0;
#endif

            continue;
        }

        recvBuf = (struct sync_buffer_def *)
            (((char *) syncBuf) + indexToPnid_[index] * CommBufSize);

        if (trace_settings & TRACE_SYNC)
        {
            int nr;
            MPI_Get_count(&status[indexToPnid_[index]], MPI_CHAR, &nr);
            trace_printf("%s@%d - Received %d bytes from node %d, "
                         "seq_num=%lld, message count=%d\n",
                         method_name, __LINE__, nr, indexToPnid_[index],
                         recvBuf->nodeInfo.seq_num,
                         recvBuf->msgInfo.msg_count);
        }

        nodestate[indexToPnid_[index]].node_state  = recvBuf->nodeInfo.node_state;
        nodestate[indexToPnid_[index]].change_nid  = recvBuf->nodeInfo.change_nid;
        nodestate[indexToPnid_[index]].seq_num     = recvBuf->nodeInfo.seq_num;
        nodestate[indexToPnid_[index]].nodeMask    = recvBuf->nodeInfo.nodeMask;
#ifdef NAMESERVER_PROCESS
        nodestate[indexToPnid_[index]].monConnCount = recvBuf->nodeInfo.monConnCount;
#else
        nodestate[indexToPnid_[index]].monProcCount = recvBuf->nodeInfo.monProcCount;
#endif

        for ( int i =0; i < MAX_NODE_MASKS ; i++ )
        {
            if ( nodestate[indexToPnid_[index]].nodeMask.upNodes[i] != upNodes_.upNodes[i] ) 
            {
                if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                {
                    for ( int j =0; j < MAX_NODE_MASKS ; j++ )
                    {
                        trace_printf( "%s@%d - Divergence  (at seq #%lld), node %s "
                                      "(pnid=%d) sees cluster state[%d] %llx, local "
                                      "monitor sees %llx\n"
                                    , method_name, __LINE__
                                    , seqNum_
                                    , Node[indexToPnid_[index]]->GetName()
                                    , indexToPnid_[index]
                                    , j 
                                    , nodestate[indexToPnid_[index]].nodeMask.upNodes[j]
                                    , upNodes_.upNodes[j] );
                    }
                }
                clusterViewDivergence = true;
            }
        }

#ifndef NAMESERVER_PROCESS
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_TMSYNC))
        {
           trace_printf( "%s@%d - Node %s (pnid=%d) TmSyncState=(%d)(%s)\n"
                       , method_name, __LINE__
                       , Node[indexToPnid_[index]]->GetName()
                       , indexToPnid_[index]
                       , recvBuf->nodeInfo.tmSyncState
                       , SyncStateString( recvBuf->nodeInfo.tmSyncState ));
        }
#endif

        // Check if we need to increase my node's shutdown level ...
        // all nodes should be at the highest level selected from any source
        if ( MyNode->GetShutdownLevel() < recvBuf->nodeInfo.sdLevel )
        {
            MyNode->SetShutdownLevel( recvBuf->nodeInfo.sdLevel );
            if (MyNode->GetState() == State_Up)
            {
                MyNode->SetState( State_Shutdown );
            }
            if (trace_settings & (TRACE_REQUEST | TRACE_SYNC))
                trace_printf("%s@%d - Node %s Shutdown Level updated (%d)\n",
                             method_name, __LINE__,
                             Node[indexToPnid_[index]]->GetName(), recvBuf->nodeInfo.sdLevel);
        }

        Node[indexToPnid_[index]]->SetInternalState( recvBuf->nodeInfo.internalState );
        if ( recvBuf->nodeInfo.internalState == State_Ready_To_Exit )
        {   // The node is exiting.  Don't communicate with it any more.
            if (trace_settings & (TRACE_REQUEST | TRACE_SYNC))
                trace_printf("%s@%d - Node %s (%d) ready to exit, setting comm "
                             "to null\n", method_name, __LINE__,
                             Node[indexToPnid_[index]]->GetName(), indexToPnid_[index]);

            switch( CommType )
            {
                case CommType_InfiniBand:
                    MPI_Comm_free( &comms_[indexToPnid_[index]] );
                    break;
                case CommType_Sockets:
                    shutdown( socks_[indexToPnid_[index]], SHUT_RDWR );
                    close( socks_[indexToPnid_[index]] );
                    socks_[indexToPnid_[index]] = -1;
                    break;
                default:
                    // Programmer bonehead!
                    abort();
            }
            Node[indexToPnid_[index]]->SetState( State_Down );
            --currentNodes_;
            // Clear bit in set of "up nodes"
            upNodes_.upNodes[indexToPnid_[index]/MAX_NODE_BITMASK] &= ~(1ull << (indexToPnid_[index]%MAX_NODE_BITMASK));
        }
    }

    if ( (checkSeqNum_ || reconnectSeqNum_ != 0)
      && !ValidateSeqNum( nodestate )
      && !enqueuedDown_ )
    {
        if ( reconnectSeqNum_ == 0 && MyNode->GetState() == State_Up )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s] Sync cycle sequence number (%lld) "
                     "incorrect.  Aborting!\n", method_name, seqNum_);
            mon_log_write(MON_CLUSTER_UPDTCLUSTERSTATE_1, SQ_LOG_CRIT, buf);
            mem_log_write(CMonLog::MON_UPDATE_CLUSTER_2, MyPNID);
            if( IsRealCluster )
            { // Terminate CommAccept thread, remote pings will fail
                CommAccept.shutdownWork();
                if ( ZClientEnabled )
                {
                    ZClient->RunningZNodeDelete( MyNode->GetName() );
                    ZClient->MasterZNodeDelete( MyNode->GetName() );
                }
            }
            mon_failure_exit();
        }
    }

    nodestate[MyPNID].node_state = Node[MyPNID]->GetState();
    nodestate[MyPNID].change_nid = sentChangeNid;
    nodestate[MyPNID].seq_num = seqNum_;
    nodestate[MyPNID].nodeMask = upNodes_;
#ifdef NAMESERVER_PROCESS
    nodestate[MyPNID].monConnCount = Node[MyPNID]->GetMonConnCount();
#else
    nodestate[MyPNID].monProcCount = Node[MyPNID]->GetNumProcs();
#endif

    // Examine status returned from MPI receive requests
    for (int index = 0; index < GetConfigPNodesCount(); index++)
    {
        bool noComm;
        switch( CommType )
        {
            case CommType_InfiniBand:
                noComm = (comms_[indexToPnid_[index]] == MPI_COMM_NULL) ? true : false;
                break;
            case CommType_Sockets:
                noComm = (socks_[indexToPnid_[index]] == -1) ? true : false;
                break;
            default:
                // Programmer bonehead!
                abort();
        }
        if (noComm) continue;

        if (status[indexToPnid_[index]].MPI_ERROR != MPI_SUCCESS)
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s] MPI communications error=%d "
                     "(%s) for node %d (at seq #%lld).\n", method_name,
                     status[indexToPnid_[index]].MPI_ERROR, ErrorMsg(status[indexToPnid_[index]].MPI_ERROR),
                     indexToPnid_[index],  seqNum_);
            mon_log_write(MON_CLUSTER_UPDTCLUSTERSTATE_2, SQ_LOG_ERR, buf); 

            if ( status[indexToPnid_[index]].MPI_ERROR == MPI_ERR_EXITED )
            {   // A monitor has gone away

                mem_log_write(CMonLog::MON_UPDATE_CLUSTER_1, indexToPnid_[index]);

                switch( CommType )
                {
                    case CommType_InfiniBand:
                        MPI_Comm_free( &comms_[indexToPnid_[index]] );
                        break;
                    case CommType_Sockets:
                        shutdown( socks_[indexToPnid_[index]], SHUT_RDWR );
                        close( socks_[indexToPnid_[index]] );
                        socks_[indexToPnid_[index]] = -1;
                        break;
                    default:
                        // Programmer bonehead!
                        abort();
                }
                --currentNodes_;

                // Clear bit in set of "up nodes"
                upNodes_.upNodes[indexToPnid_[index]/MAX_NODE_BITMASK] &= ~(1ull << (indexToPnid_[index]%MAX_NODE_BITMASK));

                // Pretend node is still up until down node processing
                // completes.
                nodestate[indexToPnid_[index]].node_state = State_Unknown;
                nodestate[indexToPnid_[index]].change_nid  = -1;
                nodestate[indexToPnid_[index]].seq_num     = 0;
                for ( int i =0; i < MAX_NODE_MASKS ; i++ )
                {
                    nodestate[indexToPnid_[index]].nodeMask.upNodes[i] = 0;
                }
#ifdef NAMESERVER_PROCESS
                nodestate[indexToPnid_[index]].monConnCount = -1;
#else
                nodestate[indexToPnid_[index]].monProcCount = 0;
#endif

                if ( validateNodeDown_ )
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
                    {
                        trace_printf( "%s@%d Divergence, queueing "
                                      "monExited{%d, %d, %lld}\n"
                                    , method_name, __LINE__
                                    , indexToPnid_[index], MyPNID, seqNum_ );
                    }
                    // Save info for the exited monitor so can confirm
                    // that all monitors have the same view.
                    monExited_t monExited = {indexToPnid_[index], MyPNID, seqNum_};
                    exitedMons_.push_back( monExited );
                }
                else
                {
                    HandleDownNode(indexToPnid_[index]);
                }
            }
        }
    }

    if ( validateNodeDown_ )
        ValidateClusterState( nodestate, clusterViewDivergence );

#ifndef NAMESERVER_PROCESS
    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_TMSYNC))
    {
       trace_printf( "%s@%d - Node %s (pnid=%d) TmSyncState=(%d)(%s)\n"
                   , method_name, __LINE__
                   , MyNode->GetName()
                   , MyPNID
                   , sendBuf->nodeInfo.tmSyncState
                   , SyncStateString( sendBuf->nodeInfo.tmSyncState ));
    }
#endif

    // Update our node states
    for (int index = 0; index < GetConfigPNodesCount(); index++)
    {
        node_state = (STATE)nodestate[indexToPnid_[index]].node_state;
        change_nid = nodestate[indexToPnid_[index]].change_nid;
#if 0
        // Temporary trace - debugging only
        if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
        {
            trace_printf("%s@%d indexToPnid_[%d]=%d, MyPNID=%d,"
                         "mystate=%d(%s), myseqNum_=%lld\n", method_name, __LINE__,
                         index, indexToPnid_[index], MyPNID, MyNode->GetState(), 
                         StateString(MyNode->GetState()), seqNum_ );
        }
#endif
        if ( indexToPnid_[index] == MyPNID && 
             MyNode->GetState() == State_Merged && seqNum_ == 1)
        {   // Initial "allgather" for this re-integrated monitor.

            seqNum_ = EnsureAndGetSeqNum(nodestate);

            if (trace_settings & (TRACE_SYNC | TRACE_RECOVERY | TRACE_INIT))
            {
                trace_printf("%s@%d Completed initial allgather for pnid=%d, "
                             "state=%d(%s), seqNum_=%lld\n", method_name, __LINE__,
                             indexToPnid_[index], MyNode->GetState(), 
                             StateString(MyNode->GetState()), seqNum_ );
            }

            // Queue the node up request for processing by a
            // worker thread.
            ReqQueue.enqueueUpReq( MyPNID, NULL, -1 );
        }

        if ( change_nid == MyPNID )
        {
            if( MyNode->GetState() == State_Down ||
                MyNode->GetState() == State_Merged ||
                MyNode->GetState() == State_Joining )
            {
                if (trace_settings & TRACE_RECOVERY)
                    trace_printf( "%s@%d enqueueing node up, state=%s\n",
                                  method_name, __LINE__,
                                  StateString(MyNode->GetState()) );

                // Queue the node up request for processing by a
                // worker thread.
                ReqQueue.enqueueUpReq( MyPNID, NULL, -1 );
            }
            else
            {   // This node is being "downed"

                if (trace_settings & TRACE_RECOVERY)
                    trace_printf( "%s@%d enqueueing node down, state=%s\n",
                                  method_name, __LINE__,
                                  StateString(MyNode->GetState()) );

                // Queue the node down request for processing by a
                // worker thread.
                ReqQueue.enqueueDownReq( MyPNID );
            }
        }
        else
        {
            // In a real cluster, existing monitors need to merge new
            // monitor.

            CNode *pnode = change_nid != -1 ? Nodes->GetNode( change_nid ) : NULL;
#ifdef NAMESERVER_PROCESS
            if ( change_nid != -1 && pnode )
#else
            if ( ! Emulate_Down && change_nid != -1 && pnode )
#endif
            {
                switch ( pnode->GetState() )
                {
                case State_Down:
                    if (trace_settings & TRACE_RECOVERY)
                        trace_printf( "%s@%d - change_nid=%d, state=%s, "
                                      "queueing up request\n",
                                      method_name, __LINE__ , change_nid,
                                      StateString(pnode->GetState()));

                    mem_log_write(CMonLog::MON_UPDATE_CLUSTER_5, change_nid);

                    // Queue the node up request for processing by a
                    // worker thread.
                    ReqQueue.enqueueUpReq( change_nid,
                                           (char *)pnode->GetName(),
                                           -1 );
                    break;
                case State_Merging:
                    if (trace_settings & TRACE_RECOVERY)
                        trace_printf( "%s@%d - change_nid=%d, state=%s, "
                                      "queueing up request\n",
                                      method_name, __LINE__ , change_nid,
                                      StateString(pnode->GetState()));

                    mem_log_write(CMonLog::MON_UPDATE_CLUSTER_6, change_nid);

                    switch( CommType )
                    {
                        case CommType_InfiniBand:
                            setNewComm(change_nid);
                            break;
                        case CommType_Sockets:
                            setNewSock(change_nid);
                            break;
                        default:
                            // Programmer bonehead!
                            abort();
                    }
                    pnode->SetState( State_Merged );
                    ReqQueue.enqueueUpReq( change_nid,
                                           (char *)pnode->GetName(),
                                           -1 );
                    break;

                case State_Merged:
                case State_Joining:
                default:
                    if (trace_settings & TRACE_RECOVERY)
                        trace_printf( "%s@%d - change_nid=%d, state=%s, "
                                      "no action required.\n",
                                      method_name, __LINE__ , change_nid,
                                      StateString( pnode->GetState() ));
                    break;
                }
            }
        }
        switch ( node_state )
        {
        case State_Up:
        case State_Joining:
        case State_Merged:
        case State_Merging:
        case State_Initializing:
        case State_Unlinked:
        case State_Unknown:
           break;
        case State_Down:
            if (IsRealCluster)
            {
                doShutdown = true;
            }
            break;
        case State_Stopped:
        case State_Shutdown:
            if (trace_settings & TRACE_SYNC_DETAIL)
                trace_printf("%s@%d - Node %d is stopping.\n", method_name, __LINE__, indexToPnid_[index]);
            Node[indexToPnid_[index]]->SetState( (STATE) node_state );
            doShutdown = true;
            break;
        default:
            if (trace_settings & TRACE_SYNC)
                trace_printf("%s@%d - Node %d in unknown state (%d).\n",
                             method_name, __LINE__, indexToPnid_[index], node_state);
        }
    }

#ifdef NAMESERVER_PROCESS
    // Update min monConnCount
    int minConnCount = INT_MAX;
    int minConnPnid = -1;
    for (int index = 0; index < GetConfigPNodesMax(); index++)
    {
        int connCount = nodestate[index].monConnCount;
        if ( ( connCount >= 0 ) && ( connCount < minConnCount ) )
        {
            minConnPnid = index;
            minConnCount = connCount;
        }
    }
    myMonConnCount_ = nodestate[MyPNID].monConnCount;
    minMonConnCount_ = minConnCount;
    minMonConnPnid_ = minConnPnid;
#else
    if (NameServerEnabled)
    {
        clusterProcCount_ = 0;
        for (int index = 0; index < GetConfigPNodesMax(); index++)
        {
            clusterProcCount_ += nodestate[index].monProcCount;
        }
    }
#endif

    TRACE_EXIT;
}

bool CCluster::ProcessClusterData( struct sync_buffer_def * syncBuf,
                                   struct sync_buffer_def * sendBuf )
{
    const char method_name[] = "CCluster::ProcessClusterData";
    TRACE_ENTRY;

    // Using the data returned from Allgather, process replication data
    // from all nodes.
    struct internal_msg_def *msg;
    struct sync_buffer_def *msgBuf;
    bool rs = false;

    for (int i = 0; i < GetConfigPNodesCount(); i++)
    {
        bool noComm;
        switch( CommType )
        {
            case CommType_InfiniBand:
                noComm = (comms_[indexToPnid_[i]] == MPI_COMM_NULL) ? true : false;
                break;
            case CommType_Sockets:
                noComm = (socks_[indexToPnid_[i]] == -1) ? true : false;
                break;
            default:
                // Programmer bonehead!
                abort();
        }
        // Only process active nodes
        if (noComm && indexToPnid_[i] != MyPNID) continue;

        if ( indexToPnid_[i] == MyPNID )
        {   // Get pointer to message sent by this node
            msgBuf = sendBuf;
        }
        else
        {   // Compute pointer to receive buffer element for node "indexToPnid_[i]"
            msgBuf = (struct sync_buffer_def *)
                (((char *) syncBuf) + indexToPnid_[i] * CommBufSize);
        }

        if (trace_settings & TRACE_SYNC)
        {
            trace_printf("%s@%d - Buffer for node %d, swpRecCount_=%d, seq_num=%lld, "
                         "lastSeqNum_=%lld, msg_count=%d, msg_offset=%d\n",
                         method_name, __LINE__, indexToPnid_[i], swpRecCount_,
                         msgBuf->nodeInfo.seq_num,
                         lastSeqNum_,
                         msgBuf->msgInfo.msg_count,
                         msgBuf->msgInfo.msg_offset);
        }

        // if we have already processed buffer, skip it
        if (lastSeqNum_ >= msgBuf->nodeInfo.seq_num)
        {
            if (trace_settings & TRACE_SYNC)
            {
                trace_printf("%s@%d - Already processed buffer for node %d, swpRecCount_=%d, seq_num=%lld, "
                             "lastSeqNum_=%lld, msg_count=%d, msg_offset=%d\n",
                             method_name, __LINE__, indexToPnid_[i], swpRecCount_,
                             msgBuf->nodeInfo.seq_num,
                             lastSeqNum_,
                             msgBuf->msgInfo.msg_count,
                             msgBuf->msgInfo.msg_offset);
            }
            continue;
        }

        if (trace_settings & TRACE_SYNC)
        {
            trace_printf("%s@%d - Processing buffer for node %d, swpRecCount_=%d, seq_num=%lld, "
                         "lastSeqNum_=%lld, msg_count=%d, msg_offset=%d\n",
                         method_name, __LINE__, indexToPnid_[i], swpRecCount_,
                         msgBuf->nodeInfo.seq_num,
                         lastSeqNum_,
                         msgBuf->msgInfo.msg_count,
                         msgBuf->msgInfo.msg_offset);
        }

        // reset msg length to zero to initialize for PopMsg()
        msgBuf->msgInfo.msg_offset = 0;

        if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
        {
            if (msgBuf->msgInfo.msg_count)
            {
                trace_printf( "%s@%d - Handling %d message(s) for node %d (seq_num=%lld)\n"
                             , method_name, __LINE__
                             , msgBuf->msgInfo.msg_count
                             , indexToPnid_[i]
                             , msgBuf->nodeInfo.seq_num );
            }
            else
            {
                trace_printf( "%s@%d - No messages for node %d (seq_num=%lld)\n"
                             , method_name, __LINE__
                             , indexToPnid_[i]
                             , msgBuf->nodeInfo.seq_num );
            }
        }
        do
        {
            // Get the next sync msg for the node
            msg = Nodes->PopMsg( msgBuf );
            if (msg->type == InternalType_Null) break;

            if ( indexToPnid_[i] == MyPNID )
                HandleMyNodeMsg (msg, MyPNID);
            else
                HandleOtherNodeMsg (msg, indexToPnid_[i]);
        }
        while ( true );
    }

    TRACE_EXIT;

    return( rs );
}

bool CCluster::checkIfDone (  )
{
    const char method_name[] = "CCluster::checkIfDone";
    TRACE_ENTRY;

    int nameServerCount = 0;
    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    CNameServerConfigContainer *nameServerConfigContainer = NULL;

#ifdef NAMESERVER_PROCESS

    if (clusterConfig)
    {
        nameServerConfigContainer = Nodes->GetNameServerConfig();
        if (nameServerConfigContainer)
        {
            nameServerCount = nameServerConfigContainer->GetCount();
        }
    }
#else
    int myNameServerCount = 0;
    CNameServerConfig *nameServerConfig = NULL;

    if (NameServerEnabled && clusterConfig)
    {
        nameServerConfigContainer = Nodes->GetNameServerConfig();
        if (nameServerConfigContainer)
        {
            nameServerCount = nameServerConfigContainer->GetCount();
            if (IsRealCluster)
            {
                nameServerConfig = nameServerConfigContainer->GetConfig( Node_name );
                if (nameServerConfig)
                {
                    myNameServerCount = 1;
                }
            }
            else
            {
                if (nameServerCount && MyPNID < nameServerCount)
                {
                    myNameServerCount = 1;
                }
            }
        }
    }
#endif

#ifdef NAMESERVER_PROCESS
    if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC))
        trace_printf("%s@%d - Node %d shutdown level=%d, state=%s.  Process "
                     "count=%d, internal state=%d, currentNodes_=%d, "
                     "local process count=%d, shutdownNameServer=%d, "
                     "nameServerCount=%d\n",
                     method_name, __LINE__, 
                     MyNode->GetPNid(),
                     MyNode->GetShutdownLevel(),
                     StateString(MyNode->GetState()),
                     Nodes->ProcessCount(),
                     MyNode->getInternalState(),
                     currentNodes_, 
                     MyNode->GetNumProcs(),
                     MyNode->IsShutdownNameServer(),
                     nameServerCount );

#else
    if (NameServerEnabled)
    {
        if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC))
            trace_printf("%s@%d - Node %d shutdown level=%d, state=%s.  Cluster process "
                         "count=%d, internal state=%d, currentNodes_=%d, "
                         "local process count=%d\n",
                         method_name, __LINE__, MyNode->GetPNid(),
                         MyNode->GetShutdownLevel(),
                         StateString(MyNode->GetState()),
                         clusterProcCount_,
                         MyNode->getInternalState(),
                         currentNodes_, MyNode->GetNumProcs());
    }
    else
    {
        if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC))
            trace_printf("%s@%d - Node %d shutdown level=%d, state=%s.  Process "
                         "count=%d, internal state=%d, currentNodes_=%d, "
                         "local process count=%d\n",
                         method_name, __LINE__, MyNode->GetPNid(),
                         MyNode->GetShutdownLevel(),
                         StateString(MyNode->GetState()),
                         Nodes->ProcessCount(),
                         MyNode->getInternalState(),
                         currentNodes_, MyNode->GetNumProcs());
    }
#endif            
    // Check if we are also done
    if (( MyNode->GetState() != State_Down    ) &&
        ( MyNode->GetState() != State_Stopped )   )
    {
        if ( MyNode->GetShutdownLevel() != ShutdownLevel_Undefined )
        {
#ifdef NAMESERVER_PROCESS
            if ( (Nodes->ProcessCount() <= nameServerCount )   // only Name Servers alive
                 && (MyNode->GetNumProcs() <= MAX_PRIMITIVES ) // only My Name Server alive
                 && MyNode->IsShutdownNameServer()   // monitor shutdown Name Server received
                 && !MyNode->isInQuiesceState() )    // post-quiescing will
                                                     // expire WDG (cluster)
            {
                if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL | TRACE_SYNC))
                   trace_printf("%s@%d - Name Server signaled to exit.\n", method_name, __LINE__);
                MyNode->SetState( State_Stopped );
                MyNode->SetInternalState(State_Ready_To_Exit);

                // we need to sync one more time so other nodes see our state
                return false;
            }
#else
            if ( NameServerEnabled )
            {

                if ( clusterProcCount_ == 0 )  // all Name Servers exited
                {
                    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL | TRACE_SYNC))
                       trace_printf("%s@%d - Monitor signaled to exit.\n", method_name, __LINE__);
                    MyNode->SetState( State_Stopped );
                    MyNode->SetInternalState(State_Ready_To_Exit);
    
                    // we need to sync one more time so other nodes see our state
                    return false;
                }
                else if ( (clusterProcCount_ <= 
                            (currentNodes_ * (MAX_PRIMITIVES+1)) ) // only WDGs and Name Servers alive
                          && (MyNode->GetNumProcs() <=
                            (MAX_PRIMITIVES+1) )                   // only WDGs and Name Servers alive
                          && !MyNode->isInQuiesceState()    // post-quiescing will
                                                            // expire WDG (cluster)
                          && !waitForWatchdogExit_ )        // WDG not yet exiting
                {
                    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL | TRACE_SYNC))
                       trace_printf("%s@%d - Stopping watchdog process. "
                                    "(process count: cluster=%d, MyNode=%d)\n",
                                    method_name, __LINE__,
                                    Nodes->ProcessCount(), MyNode->ProcessCount());
    
                    waitForWatchdogExit_ = true;
                    // stop the watchdog timer first
                    HealthCheck.setState(MON_STOP_WATCHDOG);
                    // let the watchdog process exit
                    HealthCheck.setState(MON_EXIT_PRIMITIVES);
                }
                else if ( NameServerProcess != NULL
                          && myNameServerCount > 0
                          && (MyNode->GetNumProcs() <= myNameServerCount ) // only My Name Server alive
                          && !MyNode->isInQuiesceState()    // post-quiescing will
                                                            // expire WDG (cluster)
                          && !waitForNameServerExit_ )      // Name Server not yet exiting
                {
                    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL | TRACE_SYNC))
                    {
                        trace_printf("%s@%d - Stopping Name Server process. "
                                     "(process count: cluster=%d, MyNode=%d)\n",
                                     method_name, __LINE__,
                                     Nodes->ProcessCount(), MyNode->ProcessCount());
                    }

                    waitForNameServerExit_ = true;
                    MyNode->SetProcessState( NameServerProcess, State_Down, false );
                    int rc = NameServer->ProcessShutdown();
                    if (rc)
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        snprintf( la_buf, sizeof(la_buf)
                                , "[%s] - Shutdown request to Name Server failed, node going down\n"
                                , method_name );
                        mon_log_write( MON_CLUSTER_CHECKIFDONE_1, SQ_LOG_ERR, la_buf );
                        ReqQueue.enqueueDownReq( MyPNID );
                    }
                }
            }
            else
            {
                if ( Nodes->ProcessCount() == 0 )  // all WDTs exited
                {
                    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL | TRACE_SYNC))
                       trace_printf("%s@%d - Monitor signaled to exit.\n", method_name, __LINE__);
                    MyNode->SetState( State_Stopped );
                    MyNode->SetInternalState(State_Ready_To_Exit);
    
                    // we need to sync one more time so other nodes see our state
                    return false;
                }
                else if ( (Nodes->ProcessCount() <=
                          (currentNodes_*MAX_PRIMITIVES))        // only WDGs alive
                          && !MyNode->isInQuiesceState()    // post-quiescing will
                                                            // expire WDG (cluster)
                          && !waitForWatchdogExit_ )        // WDG not yet exiting
                {
                    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL | TRACE_SYNC))
                       trace_printf("%s@%d - Stopping watchdog process.\n",
                                    method_name, __LINE__);
    
                    waitForWatchdogExit_ = true;
                    // stop the watchdog timer first
                    HealthCheck.setState(MON_STOP_WATCHDOG);
                    // let the watchdog process exit
                    HealthCheck.setState(MON_EXIT_PRIMITIVES);
                    if ( ZClientEnabled )
                    {
                        ZClient->StateSet( CZClient::ZC_SHUTDOWN ); // Disable Zookeeper client
                    }
                }
            }
#endif
        }
    }
    else if ( MyNode->GetShutdownLevel() != ShutdownLevel_Undefined
              && MyNode->GetState() == State_Down
              && MyNode->GetNumProcs() == 0)
    {
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL | TRACE_SYNC))
            trace_printf("%s@%d - No processes remaining, monitor exiting.\n",
                         method_name, __LINE__);

        MyNode->SetState( State_Stopped );
        MyNode->SetInternalState(State_Ready_To_Exit);
        if ((ZClientEnabled) && (ZClient != NULL))
        {
            ZClient->StateSet( CZClient::ZC_SHUTDOWN ); // Disable Zookeeper client
        }
        // we need to sync one more time so other nodes see our state
        return false;
    }

    MyNode->CheckShutdownProcessing();

    TRACE_EXIT;

    return ( MyNode->getInternalState() == State_Ready_To_Exit );
}


// Gather "Allgather" performance statistics
// Given the beginning and ending time of an "Allgather" operation, compute
// the elapsed time and increment the count for the appropriate range
// bucket.

const struct timespec CCluster::agBuckets_[] = {
    {0,         0},  // lowest
    {0,     20000},  // 20 us
    {0,     50000},  // 50 us
    {0,    500000},  // 500 us
    {0,   1000000},  // 1 ms
    {0,  10000000},  // 10 ms
    {0,  25000000},  // 25 ms
    {0,  50000000},  // 50 ms
    {0, 100000000},  // 100 ms
    {0, 500000000}}; // 500 ms
const int CCluster::agBucketsSize_ = sizeof(agBuckets_)/sizeof(timespec);

bool CCluster::agTimeStats(struct timespec & ts_begin,
                           struct timespec & ts_end)
{
    const char method_name[] = "CCluster::agTimeStats";
    bool slowAg = false;

    struct timespec timediff;
    if ( (ts_end.tv_nsec - ts_begin.tv_nsec )  < 0 )
    {
        timediff.tv_sec = ts_end.tv_sec - ts_begin.tv_sec - 1;
        timediff.tv_nsec = 1000000000 + ts_end.tv_nsec - ts_begin.tv_nsec;
    }
    else
    {
        timediff.tv_sec = ts_end.tv_sec - ts_begin.tv_sec;
        timediff.tv_nsec = ts_end.tv_nsec - ts_begin.tv_nsec;
    }

    if ( timediff.tv_sec > agMaxElapsed_.tv_sec
         || (timediff.tv_sec == agMaxElapsed_.tv_sec
             && timediff.tv_nsec > agMaxElapsed_.tv_nsec ))
        // Have a new maximum elapsed time
        agMaxElapsed_ = timediff;

    if ( timediff.tv_sec < agMinElapsed_.tv_sec
         || (timediff.tv_sec == agMinElapsed_.tv_sec
             && timediff.tv_nsec < agMinElapsed_.tv_nsec ))
        // Have a new minimum time
        agMinElapsed_ = timediff;

    for (int i=agBucketsSize_-1; i>=0; --i)
    {
        if (timediff.tv_sec > agBuckets_[i].tv_sec
            || (timediff.tv_sec == agBuckets_[i].tv_sec
                && timediff.tv_nsec > agBuckets_[i].tv_nsec ))
        {
            ++agElapsed_[i];
            if (i >= 7)
            {
                slowAg = true;
                if (trace_settings & TRACE_SYNC)
                {
                    trace_printf("%s@%d slow Allgather=(%ld, %ld) seqNum_=%lld, i=%d\n",
                                 method_name, __LINE__,
                                 timediff.tv_sec, timediff.tv_nsec, seqNum_, i);
                }
            }
            break;
        }
    }

    return slowAg;
}

// Display "Allgather" statistics
void CCluster::stats()
{
    const char method_name[] = "CCluster::stats";

    trace_printf("%s@%d Allgather min elapsed=%ld.%ld\n", method_name, __LINE__,
                 agMinElapsed_.tv_sec, agMinElapsed_.tv_nsec);

    trace_printf("%s@%d Allgather max elapsed=%ld.%ld\n", method_name, __LINE__,
                 agMaxElapsed_.tv_sec, agMaxElapsed_.tv_nsec);

    unsigned long int bucket;
    const char * unit;
    const char * range;
    for (int i=0; i<agBucketsSize_; ++i)
    {
        if ( i == (agBucketsSize_-1))
        {
            bucket = agBuckets_[i].tv_nsec;
            range = ">";
        }
        else
        {
            bucket = agBuckets_[i+1].tv_nsec;
            range = "<=";
        }
        bucket = bucket/1000;
        if (bucket < 1000)
            unit = "usec";
        else
        {
            bucket = bucket / 1000;
            if ( bucket < 1000 )
                unit = "msec";
            else
                unit = "???";
        }
        trace_printf("%s@%d bucket[%d]=%d (%s %ld %s)\n",
                     method_name, __LINE__, i, agElapsed_[i],
                     range, bucket, unit);
    }
}

bool CCluster::exchangeNodeData ( )
{
    const char method_name[] = "CCluster::exchangeNodeData";
    TRACE_ENTRY;

    bool result = false;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_sync_Incr();

    ++swpRecCount_; // recursive count for this function

    bool doShutdown = false;
    bool lastAllgatherWithLastSyncBuffer = false;

    struct internal_msg_def *msg;
    MPI_Status status[GetConfigPNodesMax()];
    int err;
    struct sync_buffer_def *recv_buffer;
    struct sync_buffer_def *send_buffer = Nodes->GetSyncBuffer();
    unsigned long long savedSeqNum = 0;

    // if we are here in a second recursive call that occurred while
    // processing TMSync data, use the second receive buffer
    // else, use the first one.
    if (swpRecCount_ == 1)
    {
      recv_buffer = recvBuffer_;
    }
    else
    {
      // should not be here in more than one recursive call.
      assert(swpRecCount_ == 2);
      recv_buffer = recvBuffer2_;
    }

    // Initialize sync buffer header including node state
    msg = Nodes->InitSyncBuffer( send_buffer, seqNum_, upNodes_ );

    // Initialize recv buffer
    Nodes->InitRecvBuffer( recv_buffer );

    // Fill sync buffer based on queue of replication requests
    Replicator.FillSyncBuffer ( msg );

reconnected:

    if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
        trace_printf( "%s@%d - doing Allgather size=%d, swpRecCount_=%d, "
                      "message count=%d, message seq_num=%lld, "
                      "seqNum_=%lld, lastSeqNum_=%lld, lowSeqNum_=%lld, "
                      "highSeqNum_=%lld, reconnectSeqNum_=%lld\n"
                    , method_name, __LINE__
                    , Nodes->GetSyncSize(send_buffer)
                    , swpRecCount_
                    , send_buffer->msgInfo.msg_count
                    , send_buffer->nodeInfo.seq_num
                    , seqNum_
                    , lastSeqNum_
                    , lowSeqNum_
                    , highSeqNum_
                    , reconnectSeqNum_);

    struct timespec ts_ag_begin;
    clock_gettime(CLOCK_REALTIME, &ts_ag_begin);


    // Exchange info with other nodes
    err = Allgather(Nodes->GetSyncSize(send_buffer), send_buffer, (char *)recv_buffer,
             0 /*seqNum_*/, status );

    struct timespec ts_ag_end;
    clock_gettime(CLOCK_REALTIME, &ts_ag_end);

    if (err != MPI_SUCCESS && err != MPI_ERR_IN_STATUS)
    {
        if (trace_settings & TRACE_SYNC)
        {
            trace_printf("%s@%d - unexpected Allgather error=%s (%d)\n",
                         method_name, __LINE__, ErrorMsg(err), err);
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], Unexpected MPI communications "
                 "error=%s (%d).\n", method_name, ErrorMsg(err), err);
        mon_log_write(MON_CLUSTER_EXCHANGENODEDATA_1, SQ_LOG_ERR, buf);

        // Allgather() failed in a fundamental way, bring this node down
        if ( !enqueuedDown_ )
        {
            enqueuedDown_ = true;
            ReqQueue.enqueueDownReq(MyPNID);
        }
    }
    else
    {
        if (agTimeStats( ts_ag_begin, ts_ag_end))
        {  // Slow cycle, print info
            if ( trace_settings & TRACE_SYNC )
            {
                trace_printf("%s@%d - slow Allgather info: sync size=%d, message count=%d, MyPNID=%d\n",
                             method_name, __LINE__,  Nodes->GetSyncSize(send_buffer),
                             send_buffer->msgInfo.msg_count, MyPNID);
                struct sync_buffer_def *msgBuf;
                int nr;

                for (int i = 0; i < GetConfigPNodesCount(); i++)
                {
                    bool noComm;
                    switch( CommType )
                    {
                        case CommType_InfiniBand:
                            noComm = (comms_[indexToPnid_[i]] == MPI_COMM_NULL) ? true : false;
                            break;
                        case CommType_Sockets:
                            noComm = (socks_[indexToPnid_[i]] == -1) ? true : false;
                            break;
                        default:
                            // Programmer bonehead!
                            abort();
                    }
                    // Only process active nodes
                    if (noComm) continue;

                    msgBuf = (struct sync_buffer_def *)
                        (((char *) recv_buffer) + indexToPnid_[i] * CommBufSize);

                    MPI_Get_count(&status[indexToPnid_[i]], MPI_CHAR, &nr);

                    trace_printf("%s@%d - slow Allgather info, pnid=%d: received bytes=%d, message count=%d, msg_offset=%d\n",
                                 method_name, __LINE__, indexToPnid_[i], nr,
                                 msgBuf->msgInfo.msg_count,
                                 msgBuf->msgInfo.msg_offset);
                }
            }
        }

        UpdateClusterState( doShutdown
                          , recv_buffer
                          , status
                          , send_buffer->nodeInfo.change_nid);

        if ( lastAllgatherWithLastSyncBuffer )
        {
            seqNum_ = savedSeqNum;
            lastAllgatherWithLastSyncBuffer = false;
            send_buffer = Nodes->GetSyncBuffer();

            if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                trace_printf( "%s@%d - Resetting lastAllgatherWithLastSyncBuffer=%d\n"
                            , method_name, __LINE__
                            , lastAllgatherWithLastSyncBuffer);

            goto reconnected;
        }

        if ( reconnectSeqNum_ != 0 )
        {

            if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                trace_printf( "%s@%d - Allgather IO retry, swpRecCount_=%d, "
                              "seqNum_=%lld, lastSeqNum_=%lld, lowSeqNum_=%lld, "
                              "highSeqNum_=%lld, reconnectSeqNum_=%lld\n"
                            , method_name, __LINE__
                            , swpRecCount_
                            , seqNum_
                            , lastSeqNum_
                            , lowSeqNum_
                            , highSeqNum_
                            , reconnectSeqNum_);

            // The Allgather() has executed a reconnect at reconnectSeqNum_.
            // The UpdateClusterState has set the lowSeqNum_and highSeqNum_
            // in the current IO exchange which will indicate whether there is
            // a mismatch in IOs between monitor processes. If there is a mismatch,
            // the lowSeqNum_and highSeqNum_ relative to our current seqNum_
            // will determine how to redrive the exchange of node data.
            if (seqNum_ > lowSeqNum_)
            { // A remote monitor did not receive our last SyncBuffer
                // Redo exchange with the previous SyncBuffer
                send_buffer = Nodes->GetLastSyncBuffer();
                savedSeqNum = seqNum_;
                seqNum_ = lastSeqNum_;
                // Indicate to follow up the next exchange with current SyncBuffer
                lastAllgatherWithLastSyncBuffer = true;
                lowSeqNum_ = highSeqNum_ = reconnectSeqNum_ = 0;

                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                    trace_printf( "%s@%d - Setting lastAllgatherWithLastSyncBuffer=%d\n"
                                , method_name, __LINE__
                                , lastAllgatherWithLastSyncBuffer);

                goto reconnected;
            }
            else if (seqNum_ < highSeqNum_)
            { // The local monitor did not receive the last remote SyncBuffer
                // Redo exchange with the current SyncBuffer
                send_buffer = Nodes->GetSyncBuffer();
                lowSeqNum_ = highSeqNum_ = reconnectSeqNum_ = 0;

                if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
                    trace_printf( "%s@%d - lastAllgatherWithLastSyncBuffer=%d\n"
                                , method_name, __LINE__
                                , lastAllgatherWithLastSyncBuffer);

                goto reconnected;
            }
            lowSeqNum_ = highSeqNum_ = reconnectSeqNum_ = 0;
        }
    }

    if (swpRecCount_ == 1)
    {
        // Save the sync buffer and corresponding sequence number we just processed
        // On reconnect we must resend the last buffer and the current buffer
        // to ensure dropped buffers are processed by all monitor processe in the
        // correct order.
        // Note: ProcessClusterData() modifies the contents of the send buffer
        //       so we must save the buffer prior to processing it
        Nodes->SaveMyLastSyncBuffer();
    }

    ProcessClusterData( recv_buffer, send_buffer );

    if (swpRecCount_ == 1)
    {
        lastSeqNum_ = seqNum_;

        // Increment count of "Allgather" calls.  If wrap-around, start again at 1.
        if ( ++seqNum_ == 0) seqNum_ = 1;
    }

    // Wake up any threads waiting on the completion of a sync cycle
    syncCycle_.wakeAll();

    if (doShutdown) result = checkIfDone( );

    if (trace_settings & (TRACE_SYNC | TRACE_TMSYNC))
        trace_printf( "%s@%d - node data exchange completed, swpRecCount_=%d, "
                      "seqNum_=%lld, lastSeqNum_=%lld, reconnectSeqNum_=%lld\n"
                    , method_name, __LINE__
                    , swpRecCount_
                    , seqNum_
                    , lastSeqNum_
                    , reconnectSeqNum_);

    --swpRecCount_;

    TRACE_EXIT;

    return result;
}

void CCluster::EpollCtl( int efd, int op, int fd, struct epoll_event *event )
{
    const char method_name[] = "CCluster::EpollCtl";
    TRACE_ENTRY;
#if 0
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        int iPeer;
        for ( iPeer = 0; iPeer < GetConfigPNodesCount(); iPeer++ )
        { // Find corresponding peer by matching socket fd
            if ( fd == socks_[indexToPnid_[iPeer]] ) break;
        }
        trace_printf( "%s@%d epoll_ctl( efd=%d,%s, fd=%d(%s), %s )\n"
                    , method_name, __LINE__
                    , efd
                    , EpollOpString(op)
                    , fd, Node[indexToPnid_[iPeer]]->GetName()
                    , EpollEventString(event->events) );
    }
#endif
    int rc = epoll_ctl( efd, op, fd, event );
    if ( rc == -1 )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        int iPeer;
        for ( iPeer = 0; iPeer < GetConfigPNodesCount(); iPeer++ )
        { // Find corresponding peer by matching socket fd
            if ( fd == socks_[indexToPnid_[iPeer]] ) break;
        }
        snprintf( buf, sizeof(buf), "[%s@%d] epoll_ctl(efd=%d,%s, fd=%d(%s), %s) error: %s\n"
                , method_name, __LINE__
                , efd
                , EpollOpString(op)
                , fd, Node[indexToPnid_[iPeer]]->GetName()
                , EpollEventString(event->events)
                , strerror_r( errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_EPOLLCTL_1, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }

    TRACE_EXIT;
    return;
}

void CCluster::EpollCtlDelete( int efd, int fd, struct epoll_event *event )
{
    const char method_name[] = "CCluster::EpollCtlDelete";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        int iPeer;
        for ( iPeer = 0; iPeer < GetConfigPNodesCount(); iPeer++ )
        { // Find corresponding peer by matching socket fd
            if ( fd == socks_[indexToPnid_[iPeer]] ) break;
        }
        trace_printf( "%s@%d epoll_ctl( efd=%d,%s, fd=%d(%s), %s )\n"
                    , method_name, __LINE__
                    , efd
                    , EpollOpString(EPOLL_CTL_DEL)
                    , fd, Node[indexToPnid_[iPeer]]->GetName()
                    , EpollEventString(event->events) );
    }

    // Remove old socket from epoll set, it may not be there
    int rc = epoll_ctl( efd, EPOLL_CTL_DEL, fd, event  );
    if ( rc == -1 )
    {
        int err = errno;
        if (err != ENOENT)
        {
            char ebuff[256];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s@%d] epoll_ctl(efd=%d, %s, fd=%d, %s) error: %s\n"
                    , method_name, __LINE__
                    , efd
                    , EpollOpString(EPOLL_CTL_DEL)
                    , fd
                    , EpollEventString(event->events)
                    , strerror_r( err, ebuff, 256 ) );
            mon_log_write( MON_CLUSTER_EPOLLCTLDELETE_1, SQ_LOG_CRIT, buf );

            mon_failure_exit();
        }
    }

    TRACE_EXIT;
    return;
}

void CCluster::InitClusterSocks( int worldSize, int myRank, char *nodeNames, int *rankToPnid )
{
    const char method_name[] = "CCluster::InitClusterSocks";
    TRACE_ENTRY;

    int serverSyncPort;
    CNode *node;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d worldSize=%d, myRank=%d\n"
                    , method_name, __LINE__
                    , worldSize, myRank);
    }

    // Exchange ports with collective
    serverSyncPort = MyNode->GetSyncSocketPort();
    int rc = MPI_Allgather( &serverSyncPort, 1, MPI_INT,
        sockPorts_, 1, MPI_INT, MPI_COMM_WORLD );
    if ( rc != MPI_SUCCESS )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s@%d] MPI_Allgather error=%s\n",
            method_name, __LINE__, ErrorMsg( rc ) );
        mon_log_write( MON_CLUSTER_INITCLUSTERSOCKS_3, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
#ifdef NAMESERVER_PROCESS
    if ( !IsRealCluster )
    {
        for ( int i = 0; i < worldSize; i++ )
            sockPorts_[i] = syncPort_ + i;
    }
#endif

    char *n, nodeName[MPI_MAX_PROCESSOR_NAME];
    unsigned char srcaddr[4], dstaddr[4];
    struct hostent *he;
    if ( nodeNames )
    {
        n = &nodeNames[myRank*MPI_MAX_PROCESSOR_NAME];
    }
    else
    {
        strcpy( nodeName, "localhost" );
        n = nodeName;
    }
    // Get my host structure via my node name or localhost
    he = gethostbyname( n );
    if ( !he )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s@%d] gethostbyname(%s) error: %s\n",
            method_name, __LINE__, n, strerror_r( h_errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_INITCLUSTERSOCKS_4, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
    // Initialize my source address structure
    memcpy( srcaddr, he->h_addr, 4 );
    int idst;
    // Loop on each node in the cluster
    for ( int i = 0; i < worldSize; i++ )
    {
        // Loop on each adjacent node in the cluster
        for ( int j = i+1; j < worldSize; j++ )
        {
            if ( i == myRank )
            { // Current [i] node is my node, so connect to [j] node
                idst = j;
                if ( nodeNames )
                { // Real cluster
                    n = &nodeNames[j*MPI_MAX_PROCESSOR_NAME];
                    // Get peer's host structure via its node name
                    he = gethostbyname( n );
                    if ( !he )
                    {
                        char ebuff[256];
                        char buf[MON_STRING_BUF_SIZE];
                        snprintf( buf, sizeof(buf),
                            "[%s@%d] gethostbyname(%s) error: %s\n",
                            method_name, __LINE__, n,
                            strerror_r( h_errno, ebuff, 256 ) );
                        mon_log_write( MON_CLUSTER_INITCLUSTERSOCKS_5, SQ_LOG_CRIT, buf );

                        mon_failure_exit();
                    }
                    // Initialize peer's destination address structure
                    memcpy( dstaddr, he->h_addr, 4 );
                    node = Nodes->GetNode( n );
                    if ( node )
                    { // Save peer's port in its node object
                        node->SetSyncSocketPort(sockPorts_[j]);
                    }
                }
                else
                { // Virtual cluster. Same source and destination addresses
                    node = NULL;
                    memcpy( dstaddr, srcaddr, 4 );
                }

                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d Creating client socket: src=%d.%d.%d.%d, dst(%s)=%d.%d.%d.%d, dst port=%d\n"
                                , method_name, __LINE__
                                , (int)((unsigned char *)srcaddr)[0]
                                , (int)((unsigned char *)srcaddr)[1]
                                , (int)((unsigned char *)srcaddr)[2]
                                , (int)((unsigned char *)srcaddr)[3]
                                ,  n
                                , (int)((unsigned char *)dstaddr)[0]
                                , (int)((unsigned char *)dstaddr)[1]
                                , (int)((unsigned char *)dstaddr)[2]
                                , (int)((unsigned char *)dstaddr)[3]
                                , sockPorts_[j] );
                }
                // Connect to peer
                socks_[rankToPnid[j]] = MkCltSock( srcaddr, dstaddr, sockPorts_[j] ); // InitClusterSocks
            }
            else if ( j == myRank )
            { // Current [j] peer my node, accept connection from peer [i] node
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d Accepting server socket: src=%d.%d.%d.%d, port=%d\n"
                                , method_name, __LINE__
                                , (int)((unsigned char *)srcaddr)[0]
                                , (int)((unsigned char *)srcaddr)[1]
                                , (int)((unsigned char *)srcaddr)[2]
                                , (int)((unsigned char *)srcaddr)[3]
                                , serverSyncPort );
                }

                idst = i;
                // Accept connection from peer [i]
                socks_[rankToPnid[i]] = AcceptSock( syncSock_ ); // InitClusterSocks
            }
            else
            {
                idst = -1;
            }
            if ( idst >= 0 && socks_[rankToPnid[idst]] < 0 )
            {
                char buf[MON_STRING_BUF_SIZE];
                if ( idst == i )
                {
                    snprintf( buf, sizeof(buf), "[%s@%d] mkcltsock src=%d.%d.%d.%d dst=%d.%d.%d.%d failed\n",
                        method_name, __LINE__,
                        (int)((unsigned char *)srcaddr)[0],
                        (int)((unsigned char *)srcaddr)[1],
                        (int)((unsigned char *)srcaddr)[2],
                        (int)((unsigned char *)srcaddr)[3],
                        (int)((unsigned char *)dstaddr)[0],
                        (int)((unsigned char *)dstaddr)[1],
                        (int)((unsigned char *)dstaddr)[2],
                        (int)((unsigned char *)dstaddr)[3] );
                }
                else
                {
                    snprintf( buf, sizeof(buf), "[%s@%d] acceptsock(%d) failed\n",
                        method_name, __LINE__, syncSock_ );
                }
                mon_log_write( MON_CLUSTER_INITCLUSTERSOCKS_6, SQ_LOG_CRIT, buf );

                mon_failure_exit();
            }
            if ( idst >= 0 && fcntl( socks_[rankToPnid[idst]], F_SETFL, O_NONBLOCK ) )
            {
                char ebuff[256];
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf), "[%s@%d] fcntl(NONBLOCK) error: %s\n",
                    method_name, __LINE__, strerror_r( errno, ebuff, 256 ) );
                mon_log_write( MON_CLUSTER_INITCLUSTERSOCKS_7, SQ_LOG_CRIT, buf );

                mon_failure_exit();
            }
            MPI_Barrier( MPI_COMM_WORLD );
        }
    }
    TRACE_EXIT;
}

void CCluster::InitServerSock( void )
{
    const char method_name[] = "CCluster::InitServerSock";
    TRACE_ENTRY;
    int serverCommPort = 0;
    int serverSyncPort = 0;
#ifdef NAMESERVER_PROCESS
    int mon2nsPort = 0;
#else
    int ptpPort = 0;
#endif
    int val = 0;

    unsigned char addr[4];
    struct hostent *he;

    he = gethostbyname( Node_name );
    if ( !he )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] gethostbyname(%s) error: %s\n"
                , method_name, __LINE__
                , Node_name, strerror_r( h_errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_INITSERVERSOCK_1, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
    memcpy( addr, he->h_addr, 4 );

#ifdef NAMESERVER_PROCESS
    char *env = getenv ("NS_COMM_PORT");
#else
    char *env = getenv("MONITOR_COMM_PORT");
#endif
    if ( env )
    {
        val = atoi(env);
        if ( val > 0)
        {
            if ( !IsRealCluster )
            {
                val += MyPNID;
            }
            serverCommPort = val;
        }
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d COMM_PORT Node_name=%s, env=%s, serverCommPort=%d, val=%d\n"
                    , method_name, __LINE__
                    , Node_name, env, serverCommPort, val );
    }

    commSock_ = MkSrvSock( &serverCommPort );
    if ( commSock_ < 0 )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
#ifdef NAMESERVER_PROCESS
                , "[%s@%d] MkSrvSock(NS_COMM_PORT=%d) error: %s\n"
#else
                , "[%s@%d] MkSrvSock(MONITOR_COMM_PORT=%d) error: %s\n"
#endif
                , method_name, __LINE__, serverCommPort
                , strerror_r( errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_INITSERVERSOCK_2, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
    else
    {
        snprintf( MyCommPort, sizeof(MyCommPort)
                , "%d.%d.%d.%d:%d"
                , (int)((unsigned char *)addr)[0]
                , (int)((unsigned char *)addr)[1]
                , (int)((unsigned char *)addr)[2]
                , (int)((unsigned char *)addr)[3]
                , serverCommPort );
        MyNode->SetCommSocketPort( serverCommPort );
        MyNode->SetCommPort( MyCommPort );

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d Initialized my comm socket port, "
                          "pnid=%d (%s:%s) (commPort=%s)\n"
                        , method_name, __LINE__
                        , MyPNID, MyNode->GetName(), MyCommPort
                        , MyNode->GetCommPort() );

    }

#ifdef NAMESERVER_PROCESS
    env = getenv("NS_SYNC_PORT");
#else
    env = getenv("MONITOR_SYNC_PORT");
#endif
    if ( env )
    {
        val = atoi(env);
        if ( val > 0)
        {
            if ( !IsRealCluster )
            {
                val += MyPNID;
            }
            syncPort_ = serverSyncPort = val;
        }
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d SYNC_PORT Node_name=%s, env=%s, serverSyncPort=%d, val=%d\n"
                    , method_name, __LINE__
                    , Node_name, env, syncPort_, val );
    }

    syncSock_ = MkSrvSock( &serverSyncPort );
    if ( syncSock_ < 0 )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
#ifdef NAMESERVER_PROCESS
                , "[%s@%d] MkSrvSock(NS_SYNC_PORT=%d) error: %s\n"
#else
                , "[%s@%d] MkSrvSock(MONITOR_SYNC_PORT=%d) error: %s\n"
#endif
                , method_name, __LINE__, serverSyncPort
                , strerror_r( errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_INITSERVERSOCK_3, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
    else
    {
        snprintf( MySyncPort, sizeof(MySyncPort)
                , "%d.%d.%d.%d:%d"
                , (int)((unsigned char *)addr)[0]
                , (int)((unsigned char *)addr)[1]
                , (int)((unsigned char *)addr)[2]
                , (int)((unsigned char *)addr)[3]
                , serverSyncPort );
        MyNode->SetSyncSocketPort( serverSyncPort );
        MyNode->SetSyncPort( MySyncPort );

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d Initialized my sync socket port, "
                          "pnid=%d (%s:%s) (syncPort=%s)\n"
                        , method_name, __LINE__
                        , MyPNID, MyNode->GetName(), MySyncPort
                        , MyNode->GetSyncPort() );
    }

#ifdef NAMESERVER_PROCESS
    env = getenv("NS_M2N_COMM_PORT");
    if ( env )
    {
        val = atoi(env);
        if ( val > 0)
        {
            if ( !IsRealCluster )
            {
                val += MyPNID;
            }
            mon2nsPort = val;
        }
    }

    mon2nsSock_ = MkSrvSock( &mon2nsPort );
    if ( mon2nsSock_ < 0 )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] MkSrvSock(NS_M2N_COMM_PORT=%d) error: %s\n"
                , method_name, __LINE__, mon2nsPort
                , strerror_r( errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_INITSERVERSOCK_4, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
    else
    {
        snprintf( MyMon2NsPort, sizeof(MyMon2NsPort)
                , "%d.%d.%d.%d:%d"
                , (int)((unsigned char *)addr)[0]
                , (int)((unsigned char *)addr)[1]
                , (int)((unsigned char *)addr)[2]
                , (int)((unsigned char *)addr)[3]
                , mon2nsPort );
        MyNode->SetMon2NsPort( MyMon2NsPort );
        MyNode->SetMon2NsSocketPort( mon2nsPort );

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d Initialized my mon2ns comm socket port, "
                          "pnid=%d (%s:%s) (Mon2NsPort=%s, Mon2NsSocketPort=%d)\n"
                        , method_name, __LINE__
                        , MyPNID, MyNode->GetName(), MyMon2NsPort
                        , MyNode->GetMon2NsPort()
                        , MyNode->GetMon2NsSocketPort() );

    }
#else
    if (NameServerEnabled)
    {
        env = getenv("MON2MON_COMM_PORT");
        if ( env )
        {
            val = atoi(env);
            if ( val > 0)
            {
                ptpPort = val;
            }
        }
        else
        {
           char buf[MON_STRING_BUF_SIZE];
           snprintf( buf, sizeof(buf)
                   , "[%s@%d] MON2MON_COMM_PORT environment variable is not set!\n"
                   , method_name, __LINE__ );
           mon_log_write( MON_CLUSTER_INITSERVERSOCK_5, SQ_LOG_CRIT, buf );

           mon_failure_exit();
        }
    
        // For virtual env, add PNid to the port so we can still test without collisions of port numbers
        if (!IsRealCluster)
        {
            ptpPort += MyNode->GetPNid();
        }
    
        ptpSock_ = MkSrvSock( &ptpPort );
        if ( ptpSock_ < 0 )
        {
            char ebuff[MON_STRING_BUF_SIZE];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s@%d] MkSrvSock(MON2MON_COMM_PORT=%d) error: %s\n"
                    , method_name, __LINE__, ptpPort
                    , strerror_r( errno, ebuff, MON_STRING_BUF_SIZE ) );
            mon_log_write( MON_CLUSTER_INITSERVERSOCK_6, SQ_LOG_CRIT, buf );

            mon_failure_exit();
        }
        else
        {
            snprintf( MyPtPPort, sizeof(MyPtPPort)
                    , "%d.%d.%d.%d:%d"
                    , (int)((unsigned char *)addr)[0]
                    , (int)((unsigned char *)addr)[1]
                    , (int)((unsigned char *)addr)[2]
                    , (int)((unsigned char *)addr)[3]
                    , ptpPort );
            MyNode->SetPtPPort( MyPtPPort );
            MyNode->SetPtPSocketPort( ptpPort );
    
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf( "%s@%d Initialized my ptp socket port, "
                              "pnid=%d (%s:%s) (ptpPort=%s)\n"
                            , method_name, __LINE__
                            , MyPNID, MyNode->GetName(), MyPtPPort
                            , MyNode->GetPtPPort() );
    
        }
    }
#endif

    epollFD_ = epoll_create1( EPOLL_CLOEXEC );
    if ( epollFD_ < 0 )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s@%d] epoll_create1(sync) error: %s\n",
            method_name, __LINE__, strerror_r( errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_INITSERVERSOCK_7, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }

    epollPingFD_ = epoll_create1( EPOLL_CLOEXEC );
    if ( epollPingFD_ < 0 )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s@%d] epoll_create1(ping) error: %s\n",
            method_name, __LINE__, strerror_r( errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_INITSERVERSOCK_5, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }

    TRACE_EXIT;
}

int CCluster::AcceptCommSock( void )
{
    const char method_name[] = "CCluster::AcceptCommSock";
    TRACE_ENTRY;

    int csock = AcceptSock( commSock_ );

    TRACE_EXIT;
    return( csock  );
}

int CCluster::AcceptSyncSock( void )
{
    const char method_name[] = "CCluster::AcceptSyncSock";
    TRACE_ENTRY;

    int csock = AcceptSock( syncSock_ );

    TRACE_EXIT;
    return( csock  );
}

#ifndef NAMESERVER_PROCESS
int CCluster::AcceptPtPSock( void )
{
    const char method_name[] = "CCluster::AcceptPtPSock";
    TRACE_ENTRY;

    int csock = AcceptSock( ptpSock_ );

    TRACE_EXIT;
    return( csock  );
}
#endif


int CCluster::AcceptSock( int sock )
{
    const char method_name[] = "CCluster::AcceptSock";
    TRACE_ENTRY;

#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    int csock; // connected socket
    struct sockaddr_in  sockinfo;   // socket address info

    size = sizeof(struct sockaddr *);
    if ( getsockname( sock, (struct sockaddr *) &sockinfo, &size ) )
    {
        char buf[MON_STRING_BUF_SIZE];
        int err = errno;
        snprintf(buf, sizeof(buf), "[%s], getsockname() failed, errno=%d (%s).\n",
                 method_name, err, strerror(err));
        mon_log_write(MON_CLUSTER_ACCEPTSOCK_1, SQ_LOG_ERR, buf);
        return ( -1 );
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        unsigned char *addrp = (unsigned char *) &sockinfo.sin_addr.s_addr;
        trace_printf( "%s@%d - Accepting socket on addr=%d.%d.%d.%d,  port=%d\n"
                    , method_name, __LINE__
                    , addrp[0]
                    , addrp[1]
                    , addrp[2]
                    , addrp[3]
                    , (int) ntohs( sockinfo.sin_port ) );
    }

    while ( ((csock = accept( sock
                            , (struct sockaddr *) 0
                            , (socklen_t *) 0 ) ) < 0) && (errno == EINTR) );

    if ( csock > 0 )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            unsigned char *addrp = (unsigned char *) &sockinfo.sin_addr.s_addr;
            trace_printf( "%s@%d - Accepted socket on addr=%d.%d.%d.%d,  port=%d, sock=%d\n"
                        , method_name, __LINE__
                        , addrp[0]
                        , addrp[1]
                        , addrp[2]
                        , addrp[3]
                        , (int) ntohs( sockinfo.sin_port )
                        , csock );
        }

        int nodelay = 1;
        if ( setsockopt( csock
                       , IPPROTO_TCP
                       , TCP_NODELAY
                       , (char *) &nodelay
                       , sizeof(int) ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            int err = errno;
            snprintf(buf, sizeof(buf), "[%s], setsockopt() failed, errno=%d (%s).\n",
                     method_name, err, strerror(err));
            mon_log_write(MON_CLUSTER_ACCEPTSOCK_2, SQ_LOG_ERR, buf);
            return ( -2 );
        }

        int reuse = 1;
        if ( setsockopt( csock
                       , SOL_SOCKET
                       , SO_REUSEADDR
                       , (char *) &reuse
                       , sizeof(int) ) )
        {
            char buf[MON_STRING_BUF_SIZE];
            int err = errno;
            snprintf(buf, sizeof(buf), "[%s], setsockopt() failed, errno=%d (%s).\n",
                     method_name, err, strerror(err));
            mon_log_write(MON_CLUSTER_ACCEPTSOCK_3, SQ_LOG_ERR, buf);
            return ( -2 );
        }
    }

    TRACE_EXIT;
    return ( csock );
}

int CCluster::Connect( const char *portName, bool doRetries )
{
    const char method_name[] = "CCluster::Connect";
    TRACE_ENTRY;

    int  sock;      // socket
    int  ret;       // returned value
    int  nodelay = 1; // sockopt reuse option
    int  reuse = 1; // sockopt reuse option
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    static int retries = 0;      // # times to retry connect
    int    outer_failures = 0;   // # failed connect loops
    int    connect_failures = 0; // # failed connects
    char   *p;     // getenv results
    struct sockaddr_in  sockinfo; // socket address info
    struct hostent *he;
    char   host[1000];
    const char *colon;
    unsigned int port;

    colon = strstr(portName, ":");
    strcpy(host, portName);
    int len = colon - portName;
    host[len] = '\0';
    port = atoi(&colon[1]);
    size = sizeof(sockinfo);

    if ( !retries )
    {
        p = getenv( "HPMP_CONNECT_RETRIES" );
        if ( p ) retries = atoi( p );
        else retries = 5;
    }

    for ( ;; )
    {
        sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock < 0 )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], socket() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ));
            mon_log_write(MON_CLUSTER_CONNECT_1, SQ_LOG_CRIT, la_buf);

            mon_failure_exit();
        }

        he = gethostbyname( host );
        if ( !he )
        {
            char ebuff[256];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s@%d] gethostbyname(%s) error: %s\n",
                method_name, __LINE__, host, strerror_r( h_errno, ebuff, 256 ) );
            mon_log_write( MON_CLUSTER_CONNECT_2, SQ_LOG_CRIT, buf );

            mon_failure_exit();
        }

        // Connect socket.
        memset( (char *) &sockinfo, 0, size );
        memcpy( (char *) &sockinfo.sin_addr, (char *) he->h_addr, 4 );
        sockinfo.sin_family = AF_INET;
        sockinfo.sin_port = htons( (unsigned short) port );

        // Note the outer loop uses "retries" from HPMP_CONNECT_RETRIES,
        // and has a yield between each retry, since it's more oriented
        // toward failures from network overload and putting a pause
        // between retries.  This inner loop should only iterate when
        // a signal interrupts the local process, so it doesn't pause
        // or use the same HPMP_CONNECT_RETRIES count.
        connect_failures = 0;
        ret = 1;
        while ( ret != 0 && connect_failures <= 10 )
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                if (doRetries)
                {
                    trace_printf( "%s@%d - Connecting to %s, addr=%d.%d.%d.%d, port=%d, connect_failures=%d\n"
                                , method_name, __LINE__
                                , portName
                                , (int)((unsigned char *)he->h_addr)[0]
                                , (int)((unsigned char *)he->h_addr)[1]
                                , (int)((unsigned char *)he->h_addr)[2]
                                , (int)((unsigned char *)he->h_addr)[3]
                                , port
                                , connect_failures );
                }
                else
                {
                    trace_printf( "%s@%d - Connecting to %s, addr=%d.%d.%d.%d, port=%d\n"
                                , method_name, __LINE__
                                , portName
                                , (int)((unsigned char *)he->h_addr)[0]
                                , (int)((unsigned char *)he->h_addr)[1]
                                , (int)((unsigned char *)he->h_addr)[2]
                                , (int)((unsigned char *)he->h_addr)[3]
                                , port );
                }
            }

            ret = connect( sock, (struct sockaddr *) &sockinfo, size );
            if ( ret == 0 ) break;
            if ( errno == EINTR )
            {
                ++connect_failures;
            }
#ifdef NAMESERVER_PROCESS
            else if ( errno == ECONNREFUSED )
            {
                ++connect_failures;
                sleep( 1 );
            }
#endif
            else
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf( la_buf, "[%s], connect(%s) failed! errno=%d (%s)\n"
                       , method_name, portName, err, strerror( err ));
                mon_log_write(MON_CLUSTER_CONNECT_3, SQ_LOG_ERR, la_buf);
                close(sock);
                return ( -1 );
            }
        }

        if ( ret == 0 ) break;

        if (doRetries == false)
        {
            close( sock );
            return( -1 );
        }

        // For large clusters, the connect/accept calls seem to fail occasionally,
        // no doubt do to the large number (1000's) of simultaneous connect packets
        // flooding the network at once.  So, we retry up to HPMP_CONNECT_RETRIES
        // number of times.
        if ( errno != EINTR )
        {
            if ( ++outer_failures > retries )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf, "[%s], connect(%s) exceeded retries! count=%d\n"
                       , method_name, portName, retries);
                mon_log_write(MON_CLUSTER_CONNECT_4, SQ_LOG_ERR, la_buf);
                close( sock );
                return ( -1 );
            }
            struct timespec req, rem;
            req.tv_sec = 0;
            req.tv_nsec = 500000;
            nanosleep( &req, &rem );
        }
        close( sock );
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Connected to %s addr=%d.%d.%d.%d, port=%d, sock=%d\n"
                    , method_name, __LINE__
                    , host
                    , (int)((unsigned char *)he->h_addr)[0]
                    , (int)((unsigned char *)he->h_addr)[1]
                    , (int)((unsigned char *)he->h_addr)[2]
                    , (int)((unsigned char *)he->h_addr)[3]
                    , port
                    , sock );
    }

    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_CONNECT_5, SQ_LOG_ERR, la_buf);
        close( sock );
        return ( -2 );
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_CONNECT_6, SQ_LOG_ERR, la_buf);
        close( sock );
        return ( -2 );
    }

    TRACE_EXIT;
    return ( sock );
}

#ifdef NAMESERVER_PROCESS
void CCluster::ConnectToMon2NsCommSelf( void )
{
    const char method_name[] = "CCluster::ConnectToMon2NsCommSelf";
    TRACE_ENTRY;

    Connect( MyNode->GetMon2NsSocketPort() );

    TRACE_EXIT;
}
#else
void CCluster::ConnectToPtPCommSelf( void )
{
    const char method_name[] = "CCluster::ConnectToPtPCommSelf";
    TRACE_ENTRY;

    Connect( MyNode->GetPtPSocketPort() );

    TRACE_EXIT;
}
#endif

void CCluster::ConnectToSelf( void )
{
    const char method_name[] = "CCluster::ConnectToSelf";
    TRACE_ENTRY;

    Connect( MyNode->GetCommSocketPort() );

    TRACE_EXIT;
}

void CCluster::Connect( int socketPort )
{
    const char method_name[] = "CCluster::Connect";
    TRACE_ENTRY;

    int  sock;     // socket
    int  ret;      // returned value
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    static int retries = 0;       // # times to retry connect
    int     connect_failures = 0; // # failed connects
    char   *p;     // getenv results
    struct sockaddr_in  sockinfo; // socket address info
    struct hostent *he;

    size = sizeof(sockinfo);

    if ( !retries )
    {
        p = getenv( "HPMP_CONNECT_RETRIES" );
        if ( p ) retries = atoi( p );
        else retries = 5;
    }

    sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock < 0 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], socket() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_CONNECTTOSELF_1, SQ_LOG_CRIT, la_buf);

        mon_failure_exit();
    }

    he = gethostbyname( "localhost" );
    if ( !he )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s@%d] gethostbyname(%s) error: %s\n",
            method_name, __LINE__, "localhost", strerror_r( h_errno, ebuff, 256 ) );
        mon_log_write( MON_CLUSTER_CONNECTTOSELF_2, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }

    // Connect socket.
    memset( (char *) &sockinfo, 0, size );
    memcpy( (char *) &sockinfo.sin_addr, (char *) he->h_addr, 4 );
    sockinfo.sin_family = AF_INET;
    sockinfo.sin_port = htons( (unsigned short) socketPort );

    connect_failures = 0;
    ret = 1;
    while ( ret != 0 && connect_failures <= 10 )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Connecting to localhost addr=%d.%d.%d.%d, port=%d, connect_failures=%d\n"
                        , method_name, __LINE__
                        , (int)((unsigned char *)he->h_addr)[0]
                        , (int)((unsigned char *)he->h_addr)[1]
                        , (int)((unsigned char *)he->h_addr)[2]
                        , (int)((unsigned char *)he->h_addr)[3]
                        , socketPort
                        , connect_failures );
        }

        ret = connect( sock, (struct sockaddr *) &sockinfo, size );
        if ( ret == 0 ) break;
        if ( errno == EINTR )
        {
            ++connect_failures;
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], connect() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ));
            mon_log_write(MON_CLUSTER_CONNECTTOSELF_3, SQ_LOG_CRIT, la_buf);

            mon_failure_exit();
        }
    }

    close( sock );

    TRACE_EXIT;
}

int CCluster::MkSrvSock( int *pport )
{
    const char method_name[] = "CCluster::MkSrvSock";
    TRACE_ENTRY;

    int  sock;     // socket
    int  err;      // return code
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t size; // size of socket address
#else
    size_t    size; // size of socket address
#endif
#else
    unsigned int size; // size of socket address
#endif
    struct sockaddr_in  sockinfo;   // socket address info
    sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock < 0 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], socket() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKSRVSOCK_1, SQ_LOG_CRIT, la_buf);
        return ( -1 );
    }

    int    nodelay = 1;   // sockopt nodelay option
    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKSRVSOCK_2, SQ_LOG_ERR, la_buf);
        close( sock );
        return ( -2 );
    }

    int    reuse = 1;   // sockopt reuse option
    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt(SO_REUSEADDR) failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKSRVSOCK_3, SQ_LOG_ERR, la_buf);
        close( sock );
        return ( -1 );
    }

    // Bind socket.
    size = sizeof(sockinfo);
    memset( (char *) &sockinfo, 0, size );
    sockinfo.sin_family = AF_INET;
    sockinfo.sin_addr.s_addr = htonl( INADDR_ANY );
    sockinfo.sin_port = htons( *pport );
    int lv_bind_tries = 0;
    do
    {
        if (lv_bind_tries > 0)
        {
            sleep(5);
        }
        err = bind( sock, (struct sockaddr *) &sockinfo, size );
        sched_yield( );
    } while ( err &&
             (errno == EADDRINUSE) &&
             (++lv_bind_tries < 4) );
    if ( err )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], bind() failed! port=%d, errno=%d (%s)\n"
               , method_name, *pport, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKSRVSOCK_4, SQ_LOG_CRIT, la_buf);
        close( sock );
        return ( -1 );
    }
    if ( pport )
    {
        if ( getsockname( sock, (struct sockaddr *) &sockinfo, &size ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], getsockname() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ));
            mon_log_write(MON_CLUSTER_MKSRVSOCK_5, SQ_LOG_CRIT, la_buf);
            close( sock );
            return ( -1 );
        }

        *pport = (int) ntohs( sockinfo.sin_port );
    }
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        unsigned char *addrp = (unsigned char *) &sockinfo.sin_addr.s_addr;
        trace_printf( "%s@%d listening on addr=%d.%d.%d.%d, port=%d\n"
                    , method_name, __LINE__
                    , addrp[0]
                    , addrp[1]
                    , addrp[2]
                    , addrp[3]
                    , pport?*pport:0);
    }

    int lv_retcode = SetKeepAliveSockOpt( sock );
    if ( lv_retcode != 0 )
    {
        return lv_retcode;
    }

    // Listen
    if ( listen( sock, SOMAXCONN ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], listen() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKSRVSOCK_6, SQ_LOG_CRIT, la_buf);
        close( sock );
        return ( -1 );
    }
    TRACE_EXIT;
    return ( sock );
}

int CCluster::MkCltSock( const char *portName )
{
    const char method_name[] = "CCluster::MkCltSock1";
    TRACE_ENTRY;

    int    sock;        // socket
    int    ret;         // returned value
    int    reuse = 1;   // sockopt reuse option
    int    nodelay = 1; // sockopt nodelay option
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    static int retries = 0;      // # times to retry connect
    int    outer_failures = 0;   // # failed connect loops
    int    connect_failures = 0; // # failed connects
    char   *p;     // getenv results
    struct sockaddr_in  sockinfo;    // socket address info
    struct hostent *he;
    char   host[1000];
    const char *colon;
    unsigned int port;

    colon = strstr(portName, ":");
    strcpy(host, portName);
    int len = colon - portName;
    host[len] = '\0';
    port = atoi(&colon[1]);

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Connecting to %s:%d\n"
                        , method_name, __LINE__
                        , host
                        , port );
        }
    }

    size = sizeof(sockinfo);

    if ( !retries )
    {
        p = getenv( "HPMP_CONNECT_RETRIES" );
        if ( p ) retries = atoi( p );
        else retries = 5;
    }

    for ( ;; )
    {
        sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock < 0 )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s], socket() failed! errno=%d (%s)\n"
                    , method_name, err, strerror( err ));
            mon_log_write(MON_CLUSTER_MKCLTSOCK_1, SQ_LOG_ERR, la_buf);
            return ( -1 );
        }

        he = gethostbyname( host );
        if ( !he )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = h_errno;
            snprintf( la_buf, sizeof(la_buf),
                      "[%s] gethostbyname(%s) failed! errno=%d (%s)\n"
                    , method_name, host, err, strerror( err ));
            mon_log_write(MON_CLUSTER_MKCLTSOCK_2, SQ_LOG_ERR, la_buf);
            close( sock );
            return ( -1 );
        }

        // Connect socket.
        memset( (char *) &sockinfo, 0, size );
        memcpy( (char *) &sockinfo.sin_addr, (char *) he->h_addr, 4 );
        sockinfo.sin_family = AF_INET;
        sockinfo.sin_port = htons( (unsigned short) port );

        // Note the outer loop uses "retries" from HPMP_CONNECT_RETRIES,
        // and has a yield between each retry, since it's more oriented
        // toward failures from network overload and putting a pause
        // between retries.  This inner loop should only iterate when
        // a signal interrupts the local process, so it doesn't pause
        // or use the same HPMP_CONNECT_RETRIES count.
        connect_failures = 0;
        ret = 1;
        while ( ret != 0 && connect_failures <= 10 )
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Connecting to %s addr=%d.%d.%d.%d, port=%d, connect_failures=%d\n"
                            , method_name, __LINE__
                            , host
                            , (int)((unsigned char *)he->h_addr)[0]
                            , (int)((unsigned char *)he->h_addr)[1]
                            , (int)((unsigned char *)he->h_addr)[2]
                            , (int)((unsigned char *)he->h_addr)[3]
                            , port
                            , connect_failures );
            }

            ret = connect( sock, (struct sockaddr *) &sockinfo, size );
            if ( ret == 0 ) break;
            if ( errno == EINTR )
            {
                ++connect_failures;
            }
            else
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf( la_buf, "[%s], connect() failed! errno=%d (%s)\n"
                       , method_name, err, strerror( err ));
                mon_log_write(MON_CLUSTER_MKCLTSOCK_3, SQ_LOG_ERR, la_buf);
                close(sock);
                return ( -1 );
            }
        }

        if ( ret == 0 ) break;

        // For large clusters, the connect/accept calls seem to fail occasionally,
        // no doubt do to the large number (1000's) of simultaneous connect packets
        // flooding the network at once.  So, we retry up to HPMP_CONNECT_RETRIES
        // number of times.
        if ( errno != EINTR )
        {
            if ( ++outer_failures > retries )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf, "[%s], connect() exceeded retries! count=%d\n"
                       , method_name, retries);
                mon_log_write(MON_CLUSTER_MKCLTSOCK_4, SQ_LOG_ERR, la_buf);
                close( sock );
                return ( -1 );
            }
            struct timespec req, rem;
            req.tv_sec = 0;
            req.tv_nsec = 500000;
            nanosleep( &req, &rem );
        }
        close( sock );
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Connected to %s addr=%d.%d.%d.%d, port=%d, sock=%d\n"
                    , method_name, __LINE__
                    , host
                    , (int)((unsigned char *)he->h_addr)[0]
                    , (int)((unsigned char *)he->h_addr)[1]
                    , (int)((unsigned char *)he->h_addr)[2]
                    , (int)((unsigned char *)he->h_addr)[3]
                    , port
                    , sock );
    }

    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKCLTSOCK_5, SQ_LOG_ERR, la_buf);
        close( sock );
        return ( -2 );
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt(SO_REUSEADDR) failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKCLTSOCK_6, SQ_LOG_ERR, la_buf);
        close( sock );
        return ( -2 );
    }

    TRACE_EXIT;
    return ( sock );
}

int CCluster::SetKeepAliveSockOpt( int sock )
{
    const char method_name[] = "CCluster::SetKeepAliveSockOpt";
    TRACE_ENTRY;

    static int sv_keepalive = -1;
    static int sv_keepidle  = 120;
    static int sv_keepintvl = 12;
    static int sv_keepcnt   = 5;

    if ( sv_keepalive == -1 )
    {
        char *lv_keepalive_env = getenv( "SQ_MON_KEEPALIVE" );
        if ( lv_keepalive_env )
        {
            sv_keepalive = atoi( lv_keepalive_env );
        }
        if ( sv_keepalive == 1 )
        {
            char *lv_keepidle_env = getenv( "SQ_MON_KEEPIDLE" );
            if ( lv_keepidle_env )
            {
                sv_keepidle = atoi( lv_keepidle_env );
            }
            char *lv_keepintvl_env = getenv( "SQ_MON_KEEPINTVL" );
            if ( lv_keepintvl_env )
            {
                sv_keepintvl = atoi( lv_keepintvl_env );
            }
            char *lv_keepcnt_env = getenv( "SQ_MON_KEEPCNT" );
            if ( lv_keepcnt_env )
            {
                sv_keepcnt = atoi( lv_keepcnt_env );
            }
        }
    }

    if ( sv_keepalive == 1 )
    {
        if ( setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE, &sv_keepalive, sizeof(int) ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], setsockopt so_keepalive() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ) );
            mon_log_write( MON_CLUSTER_SETKEEPALIVESOCKOPT_1, SQ_LOG_ERR, la_buf );
            close( sock );
            return ( -2 );
        }

        if ( setsockopt( sock, SOL_TCP, TCP_KEEPIDLE, &sv_keepidle, sizeof(int) ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], setsockopt tcp_keepidle() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ) );
            mon_log_write( MON_CLUSTER_SETKEEPALIVESOCKOPT_2, SQ_LOG_ERR, la_buf );
            close( sock );
            return ( -2 );
        }

        if ( setsockopt( sock, SOL_TCP, TCP_KEEPINTVL, &sv_keepintvl, sizeof(int) ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], setsockopt tcp_keepintvl() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ) );
            mon_log_write( MON_CLUSTER_SETKEEPALIVESOCKOPT_3, SQ_LOG_ERR, la_buf );
            close( sock );
            return ( -2 );
        }

        if ( setsockopt( sock, SOL_TCP, TCP_KEEPCNT, &sv_keepcnt, sizeof(int) ) )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf( la_buf, "[%s], setsockopt tcp_keepcnt() failed! errno=%d (%s)\n"
                   , method_name, err, strerror( err ) );
            mon_log_write( MON_CLUSTER_SETKEEPALIVESOCKOPT_4, SQ_LOG_ERR, la_buf );
            close( sock );
            return ( -2 );
        }
    }

    TRACE_EXIT;
    return ( 0 );
}

int CCluster::MkCltSock( unsigned char srcip[4], unsigned char dstip[4], int port )
{
    const char method_name[] = "CCluster::MkCltSock2";
    TRACE_ENTRY;

    int    sock;        // socket
    int    ret;         // returned value
    int    reuse = 1;   // sockopt reuse option
    int    nodelay = 1; // sockopt nodelay option
#if defined(_XOPEN_SOURCE_EXTENDED)
#ifdef __LP64__
    socklen_t  size;    // size of socket address
#else
    size_t   size;      // size of socket address
#endif
#else
    int    size;        // size of socket address
#endif
    static int retries = 0;      // # times to retry connect
    int    outer_failures = 0;   // # failed connect loops
    int    connect_failures = 0; // # failed connects
    char   *p;     // getenv results
    struct sockaddr_in  sockinfo;    // socket address info

    size = sizeof(sockinfo);
    const char * size_srcip = (const char *) srcip;

    if ( !retries )
    {
        p = getenv( "HPMP_CONNECT_RETRIES" );
        if ( p ) retries = atoi( p );
        else retries = 5;
    }

    for ( ;; )
    {
        sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock < 0 ) return ( -1 );

        // Bind local address if specified.
        if ( srcip )
        {
            memset( (char *) &sockinfo, 0, size );
            memcpy( (char *) &sockinfo.sin_addr,
                (unsigned char *) srcip, strlen(size_srcip));

            sockinfo.sin_family = AF_INET;
            sockinfo.sin_port = 0;
            if ( bind( sock, (struct sockaddr *) &sockinfo, size ) )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf( la_buf, "[%s], bind() failed! errno=%d (%s)\n"
                       , method_name, err, strerror( err ));
                mon_log_write(MON_CLUSTER_MKCLTSOCK_7, SQ_LOG_ERR, la_buf);
                close( sock );
                return ( -1 );
            }
        }

        // Connect socket.
        memset( (char *) &sockinfo, 0, size );
        memcpy( (char *) &sockinfo.sin_addr, (char *) dstip, 4 );
        sockinfo.sin_family = AF_INET;
        sockinfo.sin_port = htons( (unsigned short) port );

        // Note the outer loop uses "retries" from HPMP_CONNECT_RETRIES,
        // and has a yield between each retry, since it's more oriented
        // toward failures from network overload and putting a pause
        // between retries.  This inner loop should only iterate when
        // a signal interrupts the local process, so it doesn't pause
        // or use the same HPMP_CONNECT_RETRIES count.
        connect_failures = 0;
        ret = 1;
        while ( ret != 0 && connect_failures <= 10 )
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Connecting to addr=%d.%d.%d.%d, port=%d, connect_failures=%d\n"
                            , method_name, __LINE__
                            , (int)dstip[0]
                            , (int)dstip[1]
                            , (int)dstip[2]
                            , (int)dstip[3]
                            , port
                            , connect_failures );
            }
            ret = connect( sock, (struct sockaddr *) &sockinfo,
                size );
            if ( ret == 0 ) break;
            if ( errno == EINTR )
            {
                ++connect_failures;
            }
#ifdef NAMESERVER_PROCESS
            else if ( errno == ECONNREFUSED )
            {
                ++connect_failures;
                sleep( 1 );
            }
#endif
            else
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf( la_buf, "[%s], connect(%d.%d.%d.%d:%d) failed! errno=%d (%s)\n"
                       , method_name
                       , (int)((unsigned char *)dstip)[0]
                       , (int)((unsigned char *)dstip)[1]
                       , (int)((unsigned char *)dstip)[2]
                       , (int)((unsigned char *)dstip)[3]
                       , port
                       , err, strerror( err ));
                mon_log_write(MON_CLUSTER_MKCLTSOCK_8, SQ_LOG_ERR, la_buf);
                close(sock);
                return ( -1 );
            }
        }

        if ( ret == 0 ) break;

        // For large clusters, the connect/accept calls seem to fail occasionally,
        // no doubt do to the large number (1000's) of simultaneous connect packets
        // flooding the network at once.  So, we retry up to HPMP_CONNECT_RETRIES
        // number of times.
        if ( errno != EINTR )
        {
            if ( ++outer_failures > retries )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf, "[%s], connect() exceeded retries! count=%d\n"
                       , method_name, retries);
                mon_log_write(MON_CLUSTER_MKCLTSOCK_9, SQ_LOG_ERR, la_buf);
                close( sock );
                return ( -1 );
            }
            struct timespec req, rem;
            req.tv_sec = 0;
            req.tv_nsec = 500000;
            nanosleep( &req, &rem );
        }
        close( sock );
    }

    if ( setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKCLTSOCK_10, SQ_LOG_ERR, la_buf);
        close( sock );
        return ( -2 );
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int) ) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf( la_buf, "[%s], setsockopt() failed! errno=%d (%s)\n"
               , method_name, err, strerror( err ));
        mon_log_write(MON_CLUSTER_MKCLTSOCK_11, SQ_LOG_ERR, la_buf);
        close( sock );
        return ( -2 );
    }

    int lv_retcode = SetKeepAliveSockOpt( sock );
    if ( lv_retcode != 0 )
    {
        return lv_retcode;
    }

    TRACE_EXIT;
    return ( sock );
}

int CCluster::ReceiveMPI(char *buf, int size, int source, MonXChngTags tag, MPI_Comm comm)
{
    const char method_name[] = "CCluster::ReceiveMPI";
    TRACE_ENTRY;

    MPI_Request request;
    MPI_Status status;
    int received = 0;

    int error = MPI_Irecv(buf, size, MPI_CHAR, source, tag, comm, &request);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Msg Received. Error = %d\n", method_name, __LINE__, error);

    if (!error)
    {
        while (!received)
        {
            error = MPI_Test(&request, &received, &status);

            if (!error)
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
                    trace_printf("%s@%d - Msg Received Test. Flag = %d\n", method_name, __LINE__, received);
            }
            else
            {
                usleep(10000); // sleep 10ms and try again
            }
         }
    }

    TRACE_EXIT;
    return error;
}

int CCluster::SendMPI(char *buf, int size, int source, MonXChngTags tag, MPI_Comm comm)
{
    const char method_name[] = "CCluster::SendMPI";
    TRACE_ENTRY;

    MPI_Request request;
    MPI_Status status;
    int sent = 0;

    int error = MPI_Isend(buf, size, MPI_CHAR, source, tag, comm, &request);

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Msg Sent. Error = %d\n", method_name, __LINE__, error);

    if (!error)
    {
        while (!sent)
        {
            error = MPI_Test(&request, &sent, &status);

            if (!error)
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
                    trace_printf("%s@%d - Msg Sent Test. Flag = %d\n", method_name, __LINE__, sent);
            }
            else
            {
                usleep(10000); // sleep 10ms and try again
            }
         }
    }

    TRACE_EXIT;
    return error;
}

int CCluster::ReceiveSock(char *buf, int size, int sockFd, const char *desc)
{
    const char method_name[] = "CCluster::ReceiveSock";
    TRACE_ENTRY;

    bool    readAgain = false;
    int     error = 0;
    int     readCount = 0;
    int     received = 0;
    int     sizeCount = size;

    do
    {
        readCount = (int) recv( sockFd
                              , buf
                              , sizeCount
                              , 0 );
        if ( readCount > 0 ) Meas.addSockRcvdBytes( readCount );

        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - recv(%d), sock=%d, readCount=%d, desc=%s\n"
                        , method_name, __LINE__
                        , sizeCount
                        , sockFd
                        , readCount
                        , desc );
        }

        if ( readCount > 0 )
        { // Got data
            received += readCount;
            buf += readCount;
            if ( received == size )
            {
                readAgain = false;
            }
            else
            {
                sizeCount -= readCount;
                readAgain = true;
            }
        }
        else if ( readCount == 0 )
        { // EOF
             error = ENODATA;
             readAgain = false;
        }
        else
        { // Got an error
            if ( errno != EINTR)
            {
                error = errno;
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf, "[%s], recv(), received=%d, sock=%d, error=%d(%s), desc=%s\n"
                       , method_name
                       , received
                       , sockFd
                       , error, strerror(error)
                       , desc );
                mon_log_write(MON_CLUSTER_RECEIVESOCK_1, SQ_LOG_ERR, la_buf);
                readAgain = false;
            }
            else
            {
                readAgain = true;
            }
        }
    }
    while( readAgain );

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - recv(), received=%d, sock=%d, error=%d(%s), desc=%s\n"
                    , method_name, __LINE__
                    , received
                    , sockFd
                    , error, strerror(error)
                    , desc );
    }

    TRACE_EXIT;
    return error;
}

int CCluster::SendSock(char *buf, int size, int sockFd, const char *desc)
{
    const char method_name[] = "CCluster::SendSock";
    TRACE_ENTRY;

    bool    sendAgain = false;
    int     error = 0;
    int     sendCount = 0;
    int     sent = 0;

    do
    {
        sendCount = (int) send( sockFd
                              , buf
                              , size
                              , 0 );
        if ( sendCount > 0 ) Meas.addSockSentBytes( sendCount );

        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - send(), sock=%d, sendCount=%d, desc=%s\n"
                        , method_name, __LINE__
                        , sockFd
                        , sendCount
                        , desc );
        }

        if ( sendCount > 0 )
        { // Sent data
            sent += sendCount;
            if ( sendCount == size )
            {
                 sendAgain = false;
            }
            else
            {
                sendAgain = true;
            }
        }
        else
        { // Got an error
            if ( errno != EINTR)
            {
                error = errno;
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf, "[%s], send(), sent=%d, sock=%d, error=%d(%s), desc=%s\n"
                       , method_name
                       , sent
                       , sockFd
                       , error, strerror(error)
                       , desc );
                mon_log_write(MON_CLUSTER_SENDSOCK_1, SQ_LOG_ERR, la_buf);
                sendAgain = false;
            }
            else
            {
                sendAgain = true;
            }
        }
    }
    while( sendAgain );

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - send(), sent=%d, sock=%d, error=%d(%s), desc=%s\n"
                    , method_name, __LINE__
                    , sent
                    , sockFd
                    , error, strerror(error)
                    , desc );
    }

    TRACE_EXIT;
    return error;
}
