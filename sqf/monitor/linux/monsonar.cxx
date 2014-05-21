///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
     req_type_tmseqnum (0),
     req_type_tmsync (0),
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
        trace_printf("%s@%d- Monitor Stats: req_type_tmseqnum=%llu\n",
                     method_name, __LINE__, req_type_tmseqnum);
        trace_printf("%s@%d- Monitor Stats: req_type_tmsync=%llu\n",
                     method_name, __LINE__, req_type_tmsync);
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


