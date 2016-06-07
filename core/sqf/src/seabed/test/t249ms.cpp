//------------------------------------------------------------------
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif
int main(int argc, char *argv[]) {
    const char *text;

    argc = argc; // touch
    argv = argv; // touch

    // enums are out of range
    text = msg_mon_text_get_mon_device_state(MS_Mon_State_UnMounted);
    assert(strcmp(text, "MS_Mon_State_UnMounted") == 0);
    text = msg_mon_text_get_mon_device_state(MS_Mon_State_Mounted);
    assert(strcmp(text, "MS_Mon_State_Mounted") == 0);
    text = msg_mon_text_get_mon_device_state((MS_MON_DEVICE_STATE) -2);
    assert(strcmp(text, "MS_Mon_State_<unknown>") == 0);

    text = msg_mon_text_get_mon_proc_state(MS_Mon_State_Unknown);
    assert(strcmp(text, "MS_Mon_State_Unknown") == 0);
    text = msg_mon_text_get_mon_proc_state(MS_Mon_State_Up);
    assert(strcmp(text, "MS_Mon_State_Up") == 0);
    text = msg_mon_text_get_mon_proc_state(MS_Mon_State_Down);
    assert(strcmp(text, "MS_Mon_State_Down") == 0);
    text = msg_mon_text_get_mon_proc_state(MS_Mon_State_Stopped);
    assert(strcmp(text, "MS_Mon_State_Stopped") == 0);
    text = msg_mon_text_get_mon_proc_state(MS_Mon_State_Shutdown);
    assert(strcmp(text, "MS_Mon_State_Shutdown") == 0);
    text = msg_mon_text_get_mon_proc_state((MS_MON_PROC_STATE) -2);
    assert(strcmp(text, "MS_Mon_State_<unknown>") == 0);

    text = msg_mon_text_get_mon_shutdown_level(MS_Mon_ShutdownLevel_Undefined);
    assert(strcmp(text, "MS_Mon_ShutdownLevel_Undefined") == 0);
    text = msg_mon_text_get_mon_shutdown_level(MS_Mon_ShutdownLevel_Normal);
    assert(strcmp(text, "MS_Mon_ShutdownLevel_Normal") == 0);
    text = msg_mon_text_get_mon_shutdown_level(MS_Mon_ShutdownLevel_Immediate);
    assert(strcmp(text, "MS_Mon_ShutdownLevel_Immediate") == 0);
    text = msg_mon_text_get_mon_shutdown_level(MS_Mon_ShutdownLevel_Abrupt);
    assert(strcmp(text, "MS_Mon_ShutdownLevel_Abrupt") == 0);
    text = msg_mon_text_get_mon_shutdown_level((MS_MON_ShutdownLevel) -2);
    assert(strcmp(text, "MS_Mon_ShutdownLevel_<unknown>") == 0);

    text = msg_mon_text_get_mon_config_type(MS_Mon_ConfigType_Undefined);
    assert(strcmp(text, "MS_Mon_ConfigType_Undefined") == 0);
    text = msg_mon_text_get_mon_config_type(MS_Mon_ConfigType_Cluster);
    assert(strcmp(text, "MS_Mon_ConfigType_Cluster") == 0);
    text = msg_mon_text_get_mon_config_type(MS_Mon_ConfigType_Node);
    assert(strcmp(text, "MS_Mon_ConfigType_Node") == 0);
    text = msg_mon_text_get_mon_config_type(MS_Mon_ConfigType_Process);
    assert(strcmp(text, "MS_Mon_ConfigType_Process") == 0);
    text = msg_mon_text_get_mon_config_type((MS_Mon_ConfigType) -2);
    assert(strcmp(text, "MS_Mon_ConfigType_<unknown>") == 0);

    text = msg_mon_text_get_mon_msg_type(MS_MsgType_Change);
    assert(strcmp(text, "MS_MsgType_Change") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_Close);
    assert(strcmp(text, "MS_MsgType_Close") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_Event);
    assert(strcmp(text, "MS_MsgType_Event") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_NodeAdded);
    assert(strcmp(text, "MS_MsgType_NodeAdded") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_NodeDeleted);
    assert(strcmp(text, "MS_MsgType_NodeDeleted") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_NodeDown);
    assert(strcmp(text, "MS_MsgType_NodeDown") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_NodeJoining);
    assert(strcmp(text, "MS_MsgType_NodeJoining") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_NodePrepare);
    assert(strcmp(text, "MS_MsgType_NodePrepare") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_NodeQuiesce);
    assert(strcmp(text, "MS_MsgType_NodeQuiesce") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_NodeUp);
    assert(strcmp(text, "MS_MsgType_NodeUp") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_Open);
    assert(strcmp(text, "MS_MsgType_Open") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_ProcessCreated);
    assert(strcmp(text, "MS_MsgType_ProcessCreated") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_ProcessDeath);
    assert(strcmp(text, "MS_MsgType_ProcessDeath") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_ReintegrationError);
    assert(strcmp(text, "MS_MsgType_ReintegrationError") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_Service);
    assert(strcmp(text, "MS_MsgType_Service") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_Shutdown);
    assert(strcmp(text, "MS_MsgType_Shutdown") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_SpareUp);
    assert(strcmp(text, "MS_MsgType_SpareUp") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_TmRestarted);
    assert(strcmp(text, "MS_MsgType_TmRestarted") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_TmSyncAbort);
    assert(strcmp(text, "MS_MsgType_TmSyncAbort") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_TmSyncCommit);
    assert(strcmp(text, "MS_MsgType_TmSyncCommit") == 0);
    text = msg_mon_text_get_mon_msg_type(MS_MsgType_UnsolicitedMessage);
    assert(strcmp(text, "MS_MsgType_UnsolicitedMessage") == 0);
    text = msg_mon_text_get_mon_msg_type((MS_Mon_MSGTYPE) -2);
    assert(strcmp(text, "MS_MsgType_<unknown>") == 0);

    text = msg_mon_text_get_mon_req_type(MS_ReqType_Close);
    assert(strcmp(text, "MS_ReqType_Close") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Dump);
    assert(strcmp(text, "MS_ReqType_Dump") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Event);
    assert(strcmp(text, "MS_ReqType_Event") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Exit);
    assert(strcmp(text, "MS_ReqType_Exit") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Get);
    assert(strcmp(text, "MS_ReqType_Get") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Kill);
    assert(strcmp(text, "MS_ReqType_Kill") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_MonStats);
    assert(strcmp(text, "MS_ReqType_MonStats") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Mount);
    assert(strcmp(text, "MS_ReqType_Mount") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_NewProcess);
    assert(strcmp(text, "MS_ReqType_NewProcess") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_NodeAdd);
    assert(strcmp(text, "MS_ReqType_NodeAdd") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_NodeDelete);
    assert(strcmp(text, "MS_ReqType_NodeDelete") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_NodeDown);
    assert(strcmp(text, "MS_ReqType_NodeDown") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_NodeInfo);
    assert(strcmp(text, "MS_ReqType_NodeInfo") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_NodeName);
    assert(strcmp(text, "MS_ReqType_NodeName") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_NodeUp);
    assert(strcmp(text, "MS_ReqType_NodeUp") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Notice);
    assert(strcmp(text, "MS_ReqType_Notice") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Notify);
    assert(strcmp(text, "MS_ReqType_Notify") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Open);
    assert(strcmp(text, "MS_ReqType_Open") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_OpenInfo);
    assert(strcmp(text, "MS_ReqType_OpenInfo") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_PersistAdd);
    assert(strcmp(text, "MS_ReqType_PersistAdd") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_PersistDelete);
    assert(strcmp(text, "MS_ReqType_PersistDelete") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_PNodeInfo);
    assert(strcmp(text, "MS_ReqType_PNodeInfo") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_ProcessInfo);
    assert(strcmp(text, "MS_ReqType_ProcessInfo") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_ProcessInfoCont);
    assert(strcmp(text, "MS_ReqType_ProcessInfoCont") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Set);
    assert(strcmp(text, "MS_ReqType_Set") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Shutdown);
    assert(strcmp(text, "MS_ReqType_Shutdown") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Startup);
    assert(strcmp(text, "MS_ReqType_Startup") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_Stfsd);
    assert(strcmp(text, "MS_ReqType_Stfsd") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_TmLeader);
    assert(strcmp(text, "MS_ReqType_TmLeader") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_TmReady);
    assert(strcmp(text, "MS_ReqType_TmReady") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_TmSeqNum);
    assert(strcmp(text, "MS_ReqType_TmSeqNum") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_TmSync);
    assert(strcmp(text, "MS_ReqType_TmSync") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_TransInfo);
    assert(strcmp(text, "MS_ReqType_TransInfo") == 0);
    text = msg_mon_text_get_mon_req_type(MS_ReqType_ZoneInfo);
    assert(strcmp(text, "MS_ReqType_ZoneInfo") == 0);
    text = msg_mon_text_get_mon_req_type((MS_Mon_REQTYPE) -2);
    assert(strcmp(text, "MS_ReqType_<unknown>") == 0);

    text = msg_mon_text_get_mon_process_type(MS_ProcessType_Undefined);
    assert(strcmp(text, "MS_ProcessType_Undefined") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_TSE);
    assert(strcmp(text, "MS_ProcessType_TSE") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_DTM);
    assert(strcmp(text, "MS_ProcessType_DTM") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_ASE);
    assert(strcmp(text, "MS_ProcessType_ASE") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_Generic);
    assert(strcmp(text, "MS_ProcessType_Generic") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_Watchdog);
    assert(strcmp(text, "MS_ProcessType_Watchdog") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_AMP);
    assert(strcmp(text, "MS_ProcessType_AMP") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_Backout);
    assert(strcmp(text, "MS_ProcessType_Backout") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_VolumeRecovery);
    assert(strcmp(text, "MS_ProcessType_VolumeRecovery") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_MXOSRVR);
    assert(strcmp(text, "MS_ProcessType_MXOSRVR") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_SPX);
    assert(strcmp(text, "MS_ProcessType_SPX") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_SSMP);
    assert(strcmp(text, "MS_ProcessType_SSMP") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_PSD);
    assert(strcmp(text, "MS_ProcessType_PSD") == 0);
    text = msg_mon_text_get_mon_process_type(MS_ProcessType_SMS);
    assert(strcmp(text, "MS_ProcessType_SMS") == 0);
    text = msg_mon_text_get_mon_process_type((MS_Mon_PROCESSTYPE) -2);
    assert(strcmp(text, "MS_ProcessType_<unknown>") == 0);

    text = msg_mon_text_get_mon_zone_type(MS_Mon_ZoneType_Undefined);
    assert(strcmp(text, "MS_Mon_ZoneType_Undefined") == 0);
    text = msg_mon_text_get_mon_zone_type(MS_Mon_ZoneType_Any);
    assert(strcmp(text, "MS_Mon_ZoneType_Any") == 0);
    text = msg_mon_text_get_mon_zone_type(MS_Mon_ZoneType_Frontend);
    assert(strcmp(text, "MS_Mon_ZoneType_Frontend") == 0);
    text = msg_mon_text_get_mon_zone_type(MS_Mon_ZoneType_Backend);
    assert(strcmp(text, "MS_Mon_ZoneType_Backend") == 0);
    text = msg_mon_text_get_mon_zone_type((MS_MON_ZoneType) -2);
    assert(strcmp(text, "MS_Mon_ZoneType_<unknown>") == 0);

    return 0;
}
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wconversion"
#endif
