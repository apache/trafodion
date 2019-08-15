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

#ifndef MONSONAR_H_
#define MONSONAR_H_

#ifdef USE_SONAR
#include "sonar/sonarobject.h"
#include "sonar/sonarcounterdata.h"
#include "sonar/sonarcounters.h"
#include "sonar/client/monitor/monitorsonarcounters.h"
#else
typedef unsigned short  WORD;
typedef unsigned int    DWORD;
typedef unsigned long   SONARState_t;       // Sonar State type

extern SONARState_t gv_sonar_state;

const SONARState_t  SONAR_ENABLED           = 0x80000000U;
const SONARState_t  SONAR_MONITOR_ENABLED   = 0x00000010U;

inline bool sonar_verify_state(SONARState_t state)
{
     return ((gv_sonar_state & state) == state);
}
#endif

#include <stdio.h>

class CMonStats
{
 public:
    CMonStats();
    virtual ~CMonStats();

    virtual void displayStats();

    virtual inline void BarrierWaitIncr()
    {
    }

    virtual inline void BarrierWaitDecr()
    {
    }

    virtual inline void RequestServiceTimeIncr()
    {
    }

    virtual inline void RequestServiceTimeDecr()
    {
    }

    virtual inline void MonitorBusyIncr()
    {
    }

    virtual inline void MonitorBusyDecr()
    {
    }

    virtual inline void getMonitorBusy(WORD &)
    {
     }

    virtual inline void req_type_dump_Incr()
    {
        req_type_dump++;
    }

    virtual inline void req_type_event_Incr()
    {
        req_type_event++;
    }

    virtual inline void req_type_exit_Incr()
    {
        req_type_exit++;
    }

    virtual inline void req_type_get_Incr()
    {
        req_type_get++;
    }

    virtual inline void req_type_kill_Incr()
    {
        req_type_kill++;
    }

    virtual inline void req_type_mount_Incr()
    {
        req_type_mount++;
    }

    virtual inline void req_type_newprocess_Incr()
    {
        req_type_newprocess++;
    }

    virtual inline void req_type_nameserverstop_Incr()
    {
        req_type_nameserverstop++;
    }

    virtual inline void req_type_nameserverinfo_Incr()
    {
        req_type_nameserverinfo++;
    }

    virtual inline void req_type_nameserverstart_Incr()
    {
        req_type_nameserverstart++;
    }

    virtual inline void req_type_nodedown_Incr()
    {
        req_type_nodedown++;
    }

    virtual inline void req_type_nodeinfo_Incr()
    {
        req_type_nodeinfo++;
    }

    virtual inline void req_type_nodeup_Incr()
    {
        req_type_nodeup++;
    }

    virtual inline void req_type_notify_Incr()
    {
        req_type_notify++;
    }

    virtual inline void req_type_open_Incr()
    {
        req_type_open++;
    }

    virtual inline void req_type_processinfo_Incr()
    {
        req_type_processinfo++;
    }

    virtual inline void req_type_processinfocont_Incr()
    {
        req_type_processinfocont++;
    }

    virtual inline void req_type_set_Incr()
    {
        req_type_set++;
    }

    virtual inline void req_type_shutdown_Incr()
    {
        req_type_shutdown++;
    }

    virtual inline void req_type_startup_Incr()
    {
        req_type_startup++;
    }

    virtual inline void req_type_tmleader_Incr()
    {
        req_type_tmleader++;
    }

    virtual inline void req_type_zoneinfo_Incr()
    {
        req_type_zoneinfo++;
    }

    virtual inline void msg_type_close_Incr()
    {
        msg_type_close++;
    }

    virtual inline void msg_type_unsolicited_Incr()
    {
        msg_type_unsolicited++;
    }

    virtual inline void msg_type_invalid_Incr()
    {
        msg_type_invalid++;
    }

    virtual inline void req_sync_Incr()
    {
        req_sync++;
    }

    virtual inline void req_attach_Incr()
    {
        req_attach++;
    }

    virtual inline void notice_death_Incr()
    {
        monitor_notice_death++;
    }

    virtual inline void notice_node_up_Incr()
    {
        monitor_notice_node_up++;
    }

    virtual inline void notice_node_down_Incr()
    {
        monitor_notice_node_down++;
    }

    virtual inline void notice_registry_change_Incr()
    {
        monitor_notice_registry_change++;
    }

    virtual inline void NumProcsIncr()
    {
    }

    virtual inline void NumProcsDecr()
    {
    }

    virtual inline void NumOpensIncr()
    {
    }

    virtual inline void NumOpensDecr()
    {
    }

    virtual inline void NumNoticesIncr()
    {
    }

    virtual inline void NumNoticesDecr()
    {
    }

    virtual inline void ObjsReplicatedIncr()
    {
        monitor_objs_replicated++;
    }

    virtual inline void LocalIOBuffersIncr()
    {
    }

    virtual inline void LocalIOBuffersDecr()
    {
    }

    virtual inline void LocalIOBuffersMaxSet(DWORD value)
    {
        // Keep track of maximum buffers in use by the monitor
        monitor_localio_buffersmax_ = value;
    }

    virtual inline void LocalIOBuffersMin(int value)
    {
        // Keep trace of minimum available local io buffers
        monitor_localio_buffersmin_ = value;
    }

    virtual inline void LocalIOBufferMissesIncr()
    {
        monitor_localio_buffermisses++;
    }

    virtual inline void localio_messagebytes(unsigned long )
    {
    }


    virtual inline void syncRecvCount(size_t recvCount)
    {
        totSyncRecvCount_ += recvCount;
        ++totSyncRecvs_;
    }

    virtual inline void syncSendCount(size_t sendCount)
    {
        if ( sendCount != 0 )
        {
            totSyncSendCount_ += sendCount;
            ++totSyncSends_;

            if (sendCount > maxSyncSendCount_)
            {
                maxSyncSendCount_ = sendCount;
            }
        }
    }

    virtual inline void StdinRemoteDataReplIncr()
    {
        ++stdinRemoteDataRepl_;
    }

    virtual inline void StdioDataReplIncr()
    {
        ++stdioDataRepl_;
    }


    virtual inline void deathNoticeBcastIncr(unsigned int pids)
    {
        ++deathNoticeBcast_;

        deathNoticeBcastPids_ += pids;

        if (pids > deathNoticeBcastPidsMax_)
            deathNoticeBcastPidsMax_ = pids;
    }


    virtual inline void ReqQueueIncr()
    {
        ++reqqueue_;
    }

    virtual inline void ReqQueueDecr()
    {
        --reqqueue_;
    }

 private:
    unsigned long long req_type_dump;
    unsigned long long req_type_event;
    unsigned long long req_type_exit;
    unsigned long long req_type_get;
    unsigned long long req_type_kill;
    unsigned long long req_type_mount;
    unsigned long long req_type_newprocess;
    unsigned long long req_type_nameserverinfo;
    unsigned long long req_type_nameserverstart;
    unsigned long long req_type_nameserverstop;
    unsigned long long req_type_nodedown;
    unsigned long long req_type_nodeinfo;
    unsigned long long req_type_nodeup;
    unsigned long long req_type_notify;
    unsigned long long req_type_open;
    unsigned long long req_type_processinfo;
    unsigned long long req_type_processinfocont;
    unsigned long long req_type_set;
    unsigned long long req_type_shutdown;
    unsigned long long req_type_startup;
    unsigned long long req_type_tmleader;
    unsigned long long req_type_zoneinfo;

    unsigned long long msg_type_close;
    unsigned long long msg_type_unsolicited;
    unsigned long long msg_type_invalid;

    unsigned long long req_sync;
    unsigned long long req_attach;

    unsigned long long monitor_notice_death;
    unsigned long long monitor_notice_node_up;
    unsigned long long monitor_notice_node_down;
    unsigned long long monitor_notice_registry_change;
    unsigned long long monitor_objs_replicated;
    unsigned long long monitor_localio_buffermisses;
    int monitor_localio_buffersmax_;
    int monitor_localio_buffersmin_;

    // Number of process death notices using broadcast
    unsigned long long deathNoticeBcast_;
    // Maximum number processes notified for a single process death notice
    unsigned long long deathNoticeBcastPidsMax_;
    // Count of processes notified via death notice broadcast
    unsigned long long deathNoticeBcastPids_;

    unsigned long long totSyncRecvCount_;
    int totSyncRecvs_;

    unsigned long long totSyncSendCount_;
    unsigned long long maxSyncSendCount_;
    int totSyncSends_;

    unsigned long long stdinRemoteDataRepl_;
    unsigned long long stdioDataRepl_;

    unsigned long long reqqueue_;
};

#ifdef USE_SONAR
class CMonSonarStats: public CMonStats
{
 public:
    CMonSonarStats();
    virtual ~CMonSonarStats();

    // Sonar statistics are displayed via the Sonar facility (external
    // to the monitor.)
    virtual void displayStats();

    virtual inline void BarrierWaitIncr()
    {
        monitor_barrier_wait_time.increment();
    }

    virtual inline void BarrierWaitDecr()
    {
       monitor_barrier_wait_time.decrement();
    }

    virtual inline void RequestServiceTimeIncr()
    {
        monitor_service_time.increment();
    }

    virtual inline void RequestServiceTimeDecr()
    {
       monitor_service_time.decrement();
    }

    virtual inline void MonitorBusyIncr()
    {
        monitor_busy.increment();
    }

    virtual inline void MonitorBusyDecr()
    {
        monitor_busy.decrement();
    }

    virtual inline void getMonitorBusy(WORD &state)
    {
        LONGLONG value;
        monitor_busy.getData(state, value);
    }

    virtual inline void req_attach_Incr()
    {
        req_type_attach.increment();
    }

    virtual inline void req_type_dump_Incr()
    {
        req_type_dump.increment();
    }

    virtual inline void req_type_event_Incr()
    {
        req_type_event.increment();
    }

    virtual inline void req_type_exit_Incr()
    {
        req_type_exit.increment();
    }

    virtual inline void req_type_get_Incr()
    {
        req_type_get.increment();
    }

    virtual inline void req_type_kill_Incr()
    {
        req_type_kill.increment();
    }

    virtual inline void req_type_mount_Incr()
    {
        req_type_mount.increment();
    }

    virtual inline void req_type_newprocess_Incr()
    {
        req_type_newprocess.increment();
    }

    virtual inline void req_type_nodedown_Incr()
    {
        req_type_nodedown.increment();
    }

    virtual inline void req_type_nodeinfo_Incr()
    {
        req_type_nodeinfo.increment();
    }

    virtual inline void req_type_nodeup_Incr()
    {
        req_type_nodeup.increment();
    }

    virtual inline void req_type_notify_Incr()
    {
        req_type_notify.increment();
    }

    virtual inline void req_type_open_Incr()
    {
        req_type_open.increment();
    }

    virtual inline void req_type_processinfo_Incr()
    {
        req_type_processinfo.increment();
    }

    virtual inline void req_type_processinfocont_Incr()
    {
        req_type_processinfocont.increment();
    }

    virtual inline void req_type_set_Incr()
    {
        req_type_set.increment();
    }

    virtual inline void req_type_shutdown_Incr()
    {
        req_type_shutdown.increment();
    }

    virtual inline void req_type_startup_Incr()
    {
        req_type_startup.increment();
    }

    virtual inline void req_type_tmleader_Incr()
    {
        req_type_tmleader.increment();
    }

    virtual inline void req_type_zoneinfo_Incr()
    {
        req_type_zoneinfo.increment();
    }

    virtual inline void req_sync_Incr()
    {
        req_sync.increment();
    }



    virtual inline void notice_death_Incr()
    {
        monitor_notice_death.increment();
    }

    virtual inline void notice_node_up_Incr()
    {
        monitor_notice_node_up.increment();
    }

    virtual inline void notice_node_down_Incr()
    {
        monitor_notice_node_down.increment();
    }

    virtual inline void notice_registry_change_Incr()
    {
        monitor_notice_registry_change.increment();
    }

    virtual inline void NumProcsIncr()
    {
        monitor_process_objects.increment();
    }

    virtual inline void NumProcsDecr()
    {
        monitor_process_objects.decrement();
    }

    virtual inline void NumOpensIncr()
    {
        monitor_open_objects.increment();
    }

    virtual inline void NumOpensDecr()
    {
        monitor_open_objects.decrement();
    }

    virtual inline void NumNoticesIncr()
    {
        monitor_notice_objects.increment();
    }

    virtual inline void NumNoticesDecr()
    {
        monitor_notice_objects.decrement();
    }

    virtual inline void ObjsReplicatedIncr()
    {
        monitor_objs_replicated.increment();
    }

    virtual inline void LocalIOBuffersIncr()
    {
        monitor_localio_buffers.increment();
    }

    virtual inline void LocalIOBuffersDecr()
    {
        monitor_localio_buffers.decrement();
    }

    virtual inline void LocalIOBuffersMaxSet(DWORD val)
    {
        monitor_localio_buffersmax.set(val);
    }

    virtual inline void LocalIOBufferMissesIncr()
    {
        monitor_localio_buffermisses.increment();
    }

    virtual inline void localio_messagebytes(unsigned long count)
    {
        monitor_localio_messagebytes.add(count);
    }

    virtual inline void ReqQueueIncr()
    {
        monitor_reqqueue.increment();
    }

    virtual inline void ReqQueueDecr()
    {
        monitor_reqqueue.decrement();
    }


 private:
    typedef struct {SONARCId_t counterId;
                    SONARCounter * pCounter;} monCounter;

    SONARObject *sonar;

    IncrementingCounter req_type_attach;
    IncrementingCounter req_type_dump;
    IncrementingCounter req_type_event;
    IncrementingCounter req_type_exit;
    IncrementingCounter req_type_get;
    IncrementingCounter req_type_kill;
    IncrementingCounter req_type_mount;
    IncrementingCounter req_type_newprocess;
    IncrementingCounter req_type_nodedown;
    IncrementingCounter req_type_nodeinfo;
    IncrementingCounter req_type_nodeup;
    IncrementingCounter req_type_notify;
    IncrementingCounter req_type_open;
    IncrementingCounter req_type_processinfo;
    IncrementingCounter req_type_processinfocont;
    IncrementingCounter req_type_set;
    IncrementingCounter req_type_shutdown;
    IncrementingCounter req_type_startup;
    IncrementingCounter req_type_tmleader;
    IncrementingCounter req_type_zoneinfo;
    IncrementingCounter req_sync;

    IncrementingCounter monitor_notice_death;
    IncrementingCounter monitor_notice_node_up;
    IncrementingCounter monitor_notice_node_down;
    IncrementingCounter monitor_notice_registry_change;

    IncrementingCounter monitor_process_objects;
    IncrementingCounter monitor_notice_objects;
    IncrementingCounter monitor_open_objects;

    IncrementingCounter monitor_objs_replicated;

    BusyCounter monitor_barrier_wait_time;
    BusyCounter monitor_service_time;
    BusyCounter monitor_busy;

    IncrementingCounter monitor_localio_buffers;
    SnapshotCounter monitor_localio_buffersmax;
    IncrementingCounter monitor_localio_buffermisses;

    Accumulating64Counter monitor_localio_messagebytes;

    QueueCounter monitor_reqqueue;
};
#endif




#endif
