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

#include "monsonar.h"
#include "monlogging.h"
#include "montrace.h"

CMonStats::CMonStats()
   : req_type_dump (0),
     req_type_event (0),
     req_type_exit (0),
     req_type_get (0),
     req_type_kill (0),
     req_type_mount (0),
     req_type_newprocess (0),
     req_type_nodedown (0),
     req_type_nodeinfo (0),
     req_type_nodeup (0),
     req_type_notify (0),
     req_type_open (0),
     req_type_processinfo (0),
     req_type_processinfocont (0),
     req_type_set (0),
     req_type_shutdown (0),
     req_type_startup (0),
     req_type_tmleader (0),
     req_type_zoneinfo (0),
     msg_type_close (0),
     msg_type_unsolicited (0),
     msg_type_invalid (0),
     req_sync (0),
     req_attach (0),
     monitor_notice_death (0),
     monitor_notice_node_up (0),
     monitor_notice_node_down (0),
     monitor_notice_registry_change (0),
     monitor_objs_replicated (0),
     monitor_localio_buffermisses (0),
     monitor_localio_buffersmax_ (0),
     monitor_localio_buffersmin_ (0),
     deathNoticeBcast_ (0),
     deathNoticeBcastPidsMax_ (0),
     deathNoticeBcastPids_ (0),
     totSyncRecvCount_ (0),
     totSyncRecvs_ (0),
     totSyncSendCount_ (0),
     maxSyncSendCount_ (0),
     totSyncSends_ (0),
     stdinRemoteDataRepl_ (0),
     stdioDataRepl_ (0),
     reqqueue_ (0)
{
}

CMonStats::~CMonStats()
{
}

void CMonStats::displayStats ()
{
    const char method_name[] = "CMonStats::displayStats";

    if (trace_settings & TRACE_STATS)
    {
        trace_printf("%s@%d- Monitor Stats: req_type_dump=%llu\n",
                     method_name, __LINE__, req_type_dump);
        trace_printf("%s@%d- Monitor Stats: req_type_event=%llu\n",
                     method_name, __LINE__, req_type_event);
        trace_printf("%s@%d- Monitor Stats: req_type_exit=%llu\n",
                     method_name, __LINE__, req_type_exit);
        trace_printf("%s@%d- Monitor Stats: req_type_get=%llu\n",
                     method_name, __LINE__, req_type_get);
        trace_printf("%s@%d- Monitor Stats: req_type_kill=%llu\n",
                     method_name, __LINE__, req_type_kill);
        trace_printf("%s@%d- Monitor Stats: req_type_mount=%llu\n",
                     method_name, __LINE__, req_type_mount);
        trace_printf("%s@%d- Monitor Stats: req_type_newprocess=%llu\n", 
                     method_name, __LINE__, req_type_newprocess);
        trace_printf("%s@%d- Monitor Stats: req_type_nodedown=%llu\n",
                     method_name, __LINE__, req_type_nodedown);
        trace_printf("%s@%d- Monitor Stats: req_type_nodeinfo=%llu\n",
                     method_name, __LINE__, req_type_nodeinfo);
        trace_printf("%s@%d- Monitor Stats: req_type_nodeup=%llu\n",
                     method_name, __LINE__, req_type_nodeup);
        trace_printf("%s@%d- Monitor Stats: req_type_notify=%llu\n",
                     method_name, __LINE__, req_type_notify);
        trace_printf("%s@%d- Monitor Stats: req_type_open=%llu\n",
                     method_name, __LINE__, req_type_open);
        trace_printf("%s@%d- Monitor Stats: req_type_processinfo=%llu\n",
                     method_name, __LINE__, req_type_processinfo);
        trace_printf("%s@%d- Monitor Stats: req_type_processinfocont=%llu\n",
                     method_name, __LINE__, req_type_processinfocont);
        trace_printf("%s@%d- Monitor Stats: req_type_set=%llu\n",
                     method_name, __LINE__, req_type_set);
        trace_printf("%s@%d- Monitor Stats: req_type_shutdown=%llu\n",
                     method_name, __LINE__, req_type_shutdown);
        trace_printf("%s@%d- Monitor Stats: req_type_startup=%llu\n",
                     method_name, __LINE__, req_type_startup);
        trace_printf("%s@%d- Monitor Stats: req_type_tmleader=%llu\n",
                     method_name, __LINE__, req_type_tmleader);
        trace_printf("%s@%d- Monitor Stats: req_type_zoneinfo=%llu\n",
                     method_name, __LINE__, req_type_zoneinfo);
        trace_printf("%s@%d- Monitor Stats: msg_type_close=%llu\n",
                     method_name, __LINE__, msg_type_close);
        trace_printf("%s@%d- Monitor Stats: msg_type_unsolicited=%llu\n",
                     method_name, __LINE__, msg_type_unsolicited);
        trace_printf("%s@%d- Monitor Stats: msg_type_invalid=%llu\n",
                     method_name, __LINE__, msg_type_invalid);
        trace_printf("%s@%d- Monitor Stats: req_sync=%llu\n",
                     method_name, __LINE__, req_sync);
        trace_printf("%s@%d- Monitor Stats: req_attach=%llu\n",
                     method_name, __LINE__, req_attach);
        trace_printf("%s@%d- Monitor Stats: death notices=%llu\n",
                     method_name, __LINE__, monitor_notice_death);
        trace_printf("%s@%d- Monitor Stats: broadcast death notices=%llu, "
                     "average broadcast pids per notice=%f, "
                     "max broadcast pids=%llu\n",
                     method_name, __LINE__, deathNoticeBcast_,
                     ((deathNoticeBcast_ == 0) ? 0.0 :
                      (deathNoticeBcastPids_ / deathNoticeBcast_)),
                     deathNoticeBcastPidsMax_ );
        trace_printf("%s@%d- Monitor Stats: node up notices=%llu\n",
                     method_name, __LINE__, monitor_notice_node_up);
        trace_printf("%s@%d- Monitor Stats: node down notices=%llu\n",
                     method_name, __LINE__, monitor_notice_node_down);
        trace_printf("%s@%d- Monitor Stats: registry change notices=%llu\n",
                     method_name, __LINE__, monitor_notice_registry_change);
        trace_printf("%s@%d- Monitor Stats: objects replicated=%llu\n",
                     method_name, __LINE__, monitor_objs_replicated);
        trace_printf("%s@%d- Monitor Stats: lio buffer misses=%llu\n",
                     method_name, __LINE__, monitor_localio_buffermisses);
        trace_printf("%s@%d- Monitor Stats: lio buffer max monitor used=%d\n",
                     method_name, __LINE__, monitor_localio_buffersmax_);
        trace_printf("%s@%d- Monitor Stats: lio buffer minimum available=%d\n",
                     method_name, __LINE__, monitor_localio_buffersmin_);

        unsigned long long avgSyncRecv = 0;
        if ( totSyncRecvs_ != 0 )
            avgSyncRecv = totSyncRecvCount_ / totSyncRecvs_;
        trace_printf("%s@%d- Monitor Stats: average sync receive=%llu\n",
                     method_name, __LINE__, avgSyncRecv);

        unsigned long long avgSyncSend = 0;
        if ( totSyncSends_ != 0 )
            avgSyncSend = totSyncSendCount_ / totSyncSends_;
        trace_printf("%s@%d- Monitor Stats: total sends with data=%d, "
                     "average send data size=%llu, max send data size=%llu\n",
                     method_name, __LINE__, totSyncSends_, avgSyncSend,
                     maxSyncSendCount_);

        trace_printf("%s@%d- Monitor Stats: repl stdin data=%llu\n",
                     method_name, __LINE__, stdinRemoteDataRepl_);
        trace_printf("%s@%d- Monitor Stats: repl stdio data=%llu\n",
                     method_name, __LINE__, stdioDataRepl_);
    }
}


#ifdef USE_SONAR
CMonSonarStats::CMonSonarStats()
{
    monCounter monCounterList[] =
        { {MONITOR_REQTYPE_ATTACH_CTR, &req_type_attach},
          {MONITOR_REQTYPE_DUMP_CTR, &req_type_dump},
          {MONITOR_REQTYPE_EVENT_CTR, &req_type_event},
          {MONITOR_REQTYPE_EXIT_CTR, &req_type_exit},
          {MONITOR_REQTYPE_GET_CTR, &req_type_get},
          {MONITOR_REQTYPE_KILL_CTR, &req_type_kill},
          {MONITOR_REQTYPE_MOUNT_CTR, &req_type_mount},
          {MONITOR_REQTYPE_NEWPROCESS_CTR, &req_type_newprocess},
          {MONITOR_REQTYPE_NODEDOWN_CTR, &req_type_nodedown},
          {MONITOR_REQTYPE_NODEINFO_CTR, &req_type_nodeinfo},
          {MONITOR_REQTYPE_NODEUP_CTR, &req_type_nodeup},
          {MONITOR_REQTYPE_NOTIFY_CTR, &req_type_notify},
          {MONITOR_REQTYPE_OPEN_CTR, &req_type_open},
          {MONITOR_REQTYPE_PROCESSINFO_CTR, &req_type_processinfo},
          {MONITOR_REQTYPE_PROCESSINFOCONT_CTR, &req_type_processinfocont},
          {MONITOR_REQTYPE_SET_CTR, &req_type_set},
          {MONITOR_REQTYPE_SHUTDOWN_CTR, &req_type_shutdown},
          {MONITOR_REQTYPE_STARTUP_CTR, &req_type_startup},
          {MONITOR_REQTYPE_TMLEADER_CTR, &req_type_tmleader},
          {MONITOR_REQTYPE_ZONEINFO_CTR, &req_type_zoneinfo},
          {MONITOR_SYNC_CYCLES_CTR, &req_sync},

          {MONITOR_NOTICE_DEATH_CTR, &monitor_notice_death},
          {MONITOR_NOTICE_NODE_UP_CTR, &monitor_notice_node_up},
          {MONITOR_NOTICE_NODE_DOWN_CTR, &monitor_notice_node_down},
          {MONITOR_NOTICE_REGISTRY_CHANGE_CTR, &monitor_notice_registry_change},

          {MONITOR_PROCESS_OBJECTS_CTR, &monitor_process_objects},
          {MONITOR_NOTICE_OBJECTS_CTR, &monitor_notice_objects},
          {MONITOR_OPEN_OBJECTS_CTR, &monitor_open_objects},

          {MONITOR_OBJS_REPLICATED_CTR, &monitor_objs_replicated},

          {MONITOR_BARRIER_WAIT_TIME_CTR, &monitor_barrier_wait_time},
          {MONITOR_SERVICE_TIME_CTR, &monitor_service_time},
          {MONITOR_BUSY_CTR, &monitor_busy},

          {MONITOR_LOCALIO_BUFFERS_CTR, &monitor_localio_buffers},
          {MONITOR_LOCALIO_BUFFERSMAX_CTR, &monitor_localio_buffersmax},
          {MONITOR_LOCALIO_BUFFERMISSES_CTR, &monitor_localio_buffermisses},

          {MONITOR_LOCALIO_MESSAGEBYTES_CTR, &monitor_localio_messagebytes},

          {MONITOR_REQUEST_QUEUE_CTR, &monitor_reqqueue}
/* Monitor counters not yet in use
          {FILE_OPEN_SERVICE_TIME_CTR, &file_open_service_time},
          {PROCESS_CREATE_SERVICE_TIME_CTR, &process_create_service_time},
          {MONITOR_MPI_SEND_MESSAGES_SENT_CTR, &mpi_send_messages_sent},
          {MONITOR_MPI_SEND_BYTES_SENT_CTR, &mpi_send_bytes_sent},
          {MONITOR_MPI_IRECV_MESSAGES_RECEIVED_CTR, &mpi_irecv_messages_received},
          {MONITOR_MPI_IRECV_BYTES_RECEIVED_CTR, &mpi_irecv_bytes_received},
          {MONITOR_MPI_SENDRECV_MESSAGES_SENT_CTR, &mpi_sendrecv_messages_sent},
          {MONITOR_MPI_SENDRECV_BYTES_SENT_CTR, &mpi_sendrecv_bytes_sent},
          {MONITOR_MPI_SENDRECV_MESSAGES_RECEIVED_CTR, &mpi_sendrecv_messages_received},
          {MONITOR_MPI_SENDRECV_BYTES_RECEIVED_CTR, &mpi_sendrecv_bytes_received},
          {MONITOR_LOCALIO_BUFFERSFREE_CTR, &monitor_localio_buffersfree},
          {MONITOR_LOCALIO_MESSAGES_CTR, &monitor_localio_messages},
*/
        };
    char errbuf[MON_STRING_BUF_SIZE];

    char nodeName[25];
    gethostname(nodeName, sizeof(nodeName));

    // Initialize Sonar
    sonar =  new SONARObject( SONAR_COUNTER_VERSION );
    DWORD result = sonar->instantiate( "Monitor", nodeName,
                                       SONAR_DEFAULT_TIMEOUT );
    if( result != INFO_SONAR_SUCCESS )
    {
        sprintf(errbuf,
                "[CMonSonar::CMonSonar], Sonar instantiate error=%d\n", result);
        mon_log_write(MON_SONAR_INIT_1, SQ_LOG_ERR, errbuf);
    }
    else
    {
        sonar->setInstanceInfo( SONARObject::ProcessType_Monitor );
        sonar->setInstancePid( getpid() );

        // Register the Sonar counters used by the monitor
        int numCounters = sizeof(monCounterList)/sizeof(monCounter);

        for (int i=0; i<numCounters; i++)
        {
            result = sonar->registerCounter( monCounterList[i].counterId,
                                             monCounterList[i].pCounter );
            if (result != INFO_SONAR_SUCCESS )
            {
                sprintf(errbuf,
                        "[CMonSonar::CMonSonar], Sonar registerCounter error=%d, counterId=%d\n",
                        result, monCounterList[i].counterId);

                mon_log_write(MON_SONAR_INIT_2, SQ_LOG_ERR, errbuf);

            }
        }
        sonar_state_init();
    }
}

CMonSonarStats::~CMonSonarStats()
{
}

void CMonSonarStats::displayStats ()
{
}

#endif

// used for sonar dynamic on/off
SONARState_t gv_sonar_state;

