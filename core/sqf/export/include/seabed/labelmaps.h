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

//
// Label-maps module
//
#ifndef __SB_LABELMAPS_H_
#define __SB_LABELMAPS_H_

#include "int/exp.h"

//
// Get label for errno
//
SB_Export const char *SB_get_label_errno(int value);


//
// Get label for MS_Mon_ConfigType (MS_Mon_ConfigType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_configtype(int value);

//
// Get (short) label for MS_Mon_ConfigType (MS_Mon_ConfigType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_configtype_short(int value);

//
// Get label for MS_MON_DEVICE_STATE (MS_Mon_State_<value>)
//
SB_Export const char *SB_get_label_ms_mon_device_state(int value);

//
// Get (short) label for MS_MON_DEVICE_STATE (MS_Mon_State_<value>)
//
SB_Export const char *SB_get_label_ms_mon_device_state_short(int value);

//
// Get label for MS_Mon_MSGTYPE (MS_MsgType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_msgtype(int value);

//
// Get (short) label for MS_Mon_MSGTYPE (MS_MsgType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_msgtype_short(int value);

//
// Get label for MS_MON_PROC_STATE (MS_Mon_State_<value)
//
SB_Export const char *SB_get_label_ms_mon_proc_state(int value);

//
// Get (short) label for MS_MON_PROC_STATE (MS_Mon_State_<value)
//
SB_Export const char *SB_get_label_ms_mon_proc_state_short(int value);

//
// Get label for MS_Mon_PROCESSTYPE (MS_ProcessType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_processtype(int value);

//
// Get (short) label for MS_Mon_PROCESSTYPE (MS_ProcessType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_processtype_short(int value);

//
// Get label for MS_Mon_REQTYPE (MS_ReqType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_reqtype(int value);

//
// Get (short) label for MS_Mon_REQTYPE (MS_ReqType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_reqtype_short(int value);

//
// Get label for MS_MON_ShutdownLevel (MS_Mon_ShutdownLevel_<value>)
//
SB_Export const char *SB_get_label_ms_mon_shutdownlevel(int value);

//
// Get (short) label for MS_MON_ShutdownLevel (MS_Mon_ShutdownLevel_<value>)
//
SB_Export const char *SB_get_label_ms_mon_shutdownlevel_short(int value);


#endif // !__SB_LABELMAPS_H_

