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

#include <errno.h>

#include "seabed/labelmaps.h"
#include "seabed/labels.h"
#include "seabed/ms.h"

#include "logaggr.h"
#include "mondef.h"
#include "msi.h"
#include "msix.h"
#include "timeri.h"
#include "transport.h"
#include "utracex.h"

static const char *ga_fs_fsdialect_type_labels[] = {
    "FS_FS",
    SB_LABEL_END
};

static const char *ga_fs_fsreq_type_labels[] = {
    "OPEN",
    "OPEN_REPLY",
    "READ",
    "READ_REPLY",
    "WRITE",
    "WRITE_REPLY",
    "WRITEREAD",
    "WRITEREAD_REPLY",
    "CLOSE",
    "SIMPLE_REPLY",
    SB_LABEL_END
};

static const char *ga_ms_errno_type_labels[] = {
    "EPERM",
    "ENOENT",
    "ESRCH",
    "EINTR",
    "EIO",
    "ENXIO",
    "E2BIG",
    "ENOEXEC",
    "EBADF",
    "ECHILD",
    "EAGAIN",
    "ENOMEM",
    "EACCES",
    "EFAULT",
    "ENOTBLK",
    "EBUSY",
    "EEXIST",
    "EXDEV",
    "ENODEV",
    "ENOTDIR",
    "EISDIR",
    "EINVAL",
    "ENFILE",
    "EMFILE",
    "ENOTTY",
    "ETXTBSY",
    "EFBIG",
    "ENOSPC",
    "ESPIPE",
    "EROFS",
    "EMLINK",
    "EPIPE",
    "EDOM",
    "ERANGE",
    "EDEADLK",
    "ENAMETOOLONG",
    "ENOLCK",
    "ENOSYS",
    "ENOTEMPTY",
    "ELOOP",
    "EUNUSED41",
    "ENOMSG",
    "EIDRM",
    "ECHRNG",
    "EL2NSYNC",
    "EL3HLT",
    "EL3RST",
    "ELNRNG",
    "EUNATCH",
    "ENOCSI",
    "EL2HLT",
    "EBADE",
    "EBADR",
    "EXFULL",
    "ENOANO",
    "EBADRQC",
    "EBADSLT",
    "EUNUSED58",
    "EBFONT",
    "ENOSTR",
    "ENODATA",
    "ETIME",
    "ENOSR",
    "ENONET",
    "ENOPKG",
    "EREMOTE",
    "ENOLINK",
    "EADV",
    "ESRMNT",
    "ECOMM",
    "EPROTO",
    "EMULTIHOP",
    "EDOTDOT",
    "EBADMSG",
    "EOVERFLOW",
    "ENOTUNIQ",
    "EBADFD",
    "EREMCHG",
    "ELIBACC",
    "ELIBBAD",
    "ELIBSCN",
    "ELIBMAX",
    "ELIBEXEC",
    "EILSEQ",
    "ERESTART",
    "ESTRPIPE",
    "EUSERS",
    "ENOTSOCK",
    "EDESTADDRREQ",
    "EMSGSIZE",
    "EPROTOTYPE",
    "ENOPROTOOPT",
    "EPROTONOSUPPORT",
    "ESOCKTNOSUPPORT",
    "EOPNOTSUPP",
    "EPFNOSUPPORT",
    "EAFNOSUPPORT",
    "EADDRINUSE",
    "EADDRNOTAVAIL",
    "ENETDOWN",
    "ENETUNREACH",
    "ENETRESET",
    "ECONNABORTED",
    "ECONNRESET",
    "ENOBUFS",
    "EISCONN",
    "ENOTCONN",
    "ESHUTDOWN",
    "ETOOMANYREFS",
    "ETIMEDOUT",
    "ECONNREFUSED",
    "EHOSTDOWN",
    "EHOSTUNREACH",
    "EALREADY",
    "EINPROGRESS",
    "ESTALE",
    "EUCLEAN",
    "ENOTNAM",
    "ENAVAIL",
    "EISNAM",
    "EREMOTEIO",
    "EDQUOT",
    "ENOMEDIUM",
    "EMEDIUMTYPE",
    "ECANCELED",
    "ENOKEY",
    "EKEYEXPIRED",
    "EKEYREVOKED",
    "EKEYREJECTED",
    "EOWNERDEAD",
    "ENOTRECOVERABLE",
    "ERFKILL",
    SB_LABEL_END
};

static const char *ga_ms_mon_config_type_labels[] = {
    "MS_Mon_ConfigType_Undefined",
    "MS_Mon_ConfigType_Cluster",
    "MS_Mon_ConfigType_Node",
    "MS_Mon_ConfigType_Process",
    SB_LABEL_END
};

static const char *ga_ms_mon_config_type_short_labels[] = {
    "Undefined",
    "Cluster",
    "Node",
    "Process",
    SB_LABEL_END
};

static const char *ga_ms_mon_device_state_labels[] = {
    "MS_Mon_State_UnMounted",
    "MS_Mon_State_Mounted",
    SB_LABEL_END
};

static const char *ga_ms_mon_device_state_short_labels[] = {
    "UnMounted",
    "Mounted",
    SB_LABEL_END
};

static const char *ga_ms_mon_msg_type_labels[] = {
    "MS_MsgType_Change",
    "MS_MsgType_Close",
    "MS_MsgType_Event",
    "MS_MsgType_NodeAdded",
    "MS_MsgType_NodeChanged",
    "MS_MsgType_NodeDeleted",
    "MS_MsgType_NodeDown",
    "MS_MsgType_NodeJoining",
    "MS_MsgType_NodeQuiesce",
    "MS_MsgType_NodeUp",
    "MS_MsgType_Open",
    "MS_MsgType_ProcessCreated",
    "MS_MsgType_ProcessDeath",
    "MS_MsgType_ReintegrationError",
    "MS_MsgType_Service",
    "MS_MsgType_Shutdown",
    "MS_MsgType_SpareUp",
    SB_LABEL_END
};

static const char *ga_ms_mon_msg_type_short_labels[] = {
    "Change",
    "Close",
    "Event",
    "NodeAdded",
    "NodeChanged",
    "NodeDeleted",
    "NodeDown",
    "NodeJoining",
    "NodeQuiesce",
    "NodeUp",
    "Open",
    "ProcessCreated",
    "ProcessDeath",
    "ReintegrationError",
    "Service",
    "Shutdown",
    "SpareUp",
    SB_LABEL_END
};

static const char *ga_ms_mon_proc_state_labels[] = {
    "MS_Mon_State_Unknown",
    "MS_Mon_State_Up",
    "MS_Mon_State_Down",
    "MS_Mon_State_Stopped",
    "MS_Mon_State_Shutdown",
    "MS_Mon_State_Unlinked",
    "MS_Mon_State_Initializing",
    "MS_Mon_State_Joining",
    "MS_Mon_State_Merging",
    "MS_Mon_State_Merged",
    "MS_Mon_State_Takeover",
    SB_LABEL_END
};

static const char *ga_ms_mon_proc_state_short_labels[] = {
    "Unknown",
    "Up",
    "Down",
    "Stopped",
    "Shutdown",
    "Unlinked",
    "Initializing",
    "Joining",
    "Merging",
    "Merged",
    "Takeover",
    SB_LABEL_END
};

static const char *ga_ms_mon_process_type_labels[] = {
    "MS_ProcessType_Undefined",
    "MS_ProcessType_TSE",
    "MS_ProcessType_DTM",
    "MS_ProcessType_ASE",
    "MS_ProcessType_Generic",
    "MS_ProcessType_NameServer",
    "MS_ProcessType_Watchdog",
    "MS_ProcessType_AMP",
    "MS_ProcessType_Backout",
    "MS_ProcessType_VolumeRecovery",
    "MS_ProcessType_MXOSRVR",
    "MS_ProcessType_SPX",
    "MS_ProcessType_SSMP",
    "MS_ProcessType_PSD",
    "MS_ProcessType_SMS",
    "MS_ProcessType_TMID",
    "MS_ProcessType_PERSIST",
    SB_LABEL_END
};

static const char *ga_ms_mon_process_type_short_labels[] = {
    "Undefined",
    "TSE",
    "DTM",
    "ASE",
    "Generic",
    "NameServer",
    "Watchdog",
    "AMP",
    "Backout",
    "VolumeRecovery",
    "MXOSRVR",
    "SPX",
    "SSMP",
    "PSD",
    "SMS",
    "TMID",
    "PERSIST",
    SB_LABEL_END
};

static const char *ga_ms_mon_req_type_labels[] = {
    "MS_ReqType_Close",
    "MS_ReqType_DelProcessNs",
    "MS_ReqType_Dump",
    "MS_ReqType_Event",
    "MS_ReqType_Exit",
    "MS_ReqType_Get",
    "MS_ReqType_InstanceId",
    "MS_ReqType_Kill",
    "MS_ReqType_MonStats",
    "MS_ReqType_Mount",
    "MS_ReqType_NameServerAdd",
    "MS_ReqType_NameServerDelete",
    "MS_ReqType_NameServerStart",
    "MS_ReqType_NameServerStop",
    "MS_ReqType_NewProcess",
    "MS_ReqType_NewProcessNs",
    "MS_ReqType_NodeAdd",
    "MS_ReqType_NodeDelete",
    "MS_ReqType_NodeDown",
    "MS_ReqType_NodeInfo",
    "MS_ReqType_NodeName",
    "MS_ReqType_NodeUp",
    "MS_ReqType_Notice",
    "MS_ReqType_Notify",
    "MS_ReqType_Open",
    "MS_ReqType_OpenInfo",
    "MS_ReqType_PersistAdd",
    "MS_ReqType_PersistDelete",
    "MS_ReqType_PNodeInfo",
    "MS_ReqType_ProcessInfo",
    "MS_ReqType_ProcessInfoCont",
    "MS_ReqType_ProcessInfoNs",
    "MS_ReqType_Set",
    "MS_ReqType_Shutdown",
    "MS_ReqType_ShutdownNs",
    "MS_ReqType_Startup",
    "MS_ReqType_TmLeader",
    "MS_ReqType_TmReady",
    "MS_ReqType_ZoneInfo",
    SB_LABEL_END
};

static const char *ga_ms_mon_req_type_short_labels[] = {
    "Close",
    "DelProcessNs",
    "Dump",
    "Event",
    "Exit",
    "Get",
    "InstanceId",
    "Kill",
    "MonStats",
    "Mount",
    "NameServerAdd",
    "NameServerDelete",
    "NameServerStart",
    "NameServerStop",
    "NewProcess",
    "NewProcessNs",
    "NodeAdd",
    "NodeDelete",
    "NodeDown",
    "NodeInfo",
    "NodeName",
    "NodeUp",
    "Notice",
    "Notify",
    "Open",
    "OpenInfo",
    "PersistAdd",
    "PersistDelete",
    "PNodeInfo",
    "ProcessInfo",
    "ProcessInfoCont",
    "ProcessInfoNs",
    "Set",
    "Shutdown",
    "ShutdownNs",
    "Startup",
    "TmLeader",
    "TmReady",
    "ZoneInfo",
    SB_LABEL_END
};

static const char *ga_ms_mon_shutdown_level_labels[] = {
    "MS_Mon_ShutdownLevel_Undefined",
    "MS_Mon_ShutdownLevel_Normal",
    "MS_Mon_ShutdownLevel_Immediate",
    "MS_Mon_ShutdownLevel_Abrupt",
    SB_LABEL_END
};

static const char *ga_ms_mon_shutdown_level_short_labels[] = {
    "Undefined",
    "Normal",
    "Immediate",
    "Abrupt",
    SB_LABEL_END
};

static const char *ga_sb_log_aggr_cap_labels[] = {
    "FD-table new-cap=%d\n",
    "MD-recv-hi hi=%d\n",
    "MD-send-hi hi=%d\n",
    "MD-table new-cap=%d\n",
    "OD-table new-cap=%d\n",
    "STREAM-table new-cap=%d\n",
    "THREAD-table new-cap=%d\n",
    SB_LABEL_END
};

static const char *ga_sb_md_type_labels[] = {
    "INV",
    "WR",
    "REPLY",
    "ABANDON",
    "ABANDON_ACK",
    "CLOSE",
    "CLOSE_ACK",
    "OPEN",
    "OPEN_ACK",
    "REPLY_NACK",
    SB_LABEL_END
};

static const char *ga_sb_msmon_reply_type_labels[] = {
    "Generic",
    "DeleteNs",
    "Dump",
    "Get",
    "InstanceId",
    "MonStats",
    "Mount",
    "NewProcess",
    "NewProcessNs",
    "NodeInfo",
    "NodeName",
    "Open",
    "OpenInfo",
    "PNodeInfo",
    "ProcessInfo",
    "ProcessInfoNs",
    "Startup",
    "ZoneInfo",
    SB_LABEL_END
};

static const char *ga_sb_stream_md_state_labels[] = {
    "<unknown>",
    "WR_SENDING",
    "WR_SEND_FIN",
    "WR_FIN",
    "REPLY_SENDING",
    "REPLY_SEND_FIN",
    "REPLY_FIN",
    "ABANDON_SENDING",
    "ABANDON_FIN",
    "ABANDON_ACK_SENDING",
    "ABANDON_ACK_FIN",
    "CLOSE_SENDING",
    "CLOSE_FIN",
    "CLOSE_ACK_SENDING",
    "CLOSE_ACK_FIN",
    "OPEN_SENDING",
    "OPEN_FIN",
    "OPEN_ACK_SENDING",
    "OPEN_ACK_FIN",
    "REPLY_NACK_SENDING",
    "RSVD_MD",
    "RCVD_CLOSE",
    "RCVD_FSREQ",
    "RCVD_MSREQ",
    "RCVD_IC",
    "RCVD_MON_CLOSE",
    "RCVD_MON_MSG",
    "RCVD_MON_OPEN",
    "RCVD_OPEN",
    "SEND_INLINE_OPEN",
    "SEND_INLINE_OPEN_SELF",
    "ZERO_MD",
    "MSG_FS_NOWAIT_OPEN",
    "MSG_FS_SMSG_CLOSE",
    "MSG_LINK",
    "MSG_LOW_LOOP_CLOSE",
    "MSG_LOW_LOOP_OPEN",
    "MSG_LOW_LOOP_WR",
    "MSG_LOW_ABANDON",
    "MSG_LOW_CAN_ACK",
    "MSG_LOW_CLOSE_ACK",
    "MSG_LOW_CONN_ACK",
    "MSG_LOW_OPEN_ACK",
    "MSG_LOW_REPLY_NACK",
    "MSG_MON_MSG",
    "MSG_MON_OPEN",
    "MSG_REOPEN_FAIL",
    "MSG_SEND_SELF",
    "MSG_START_STREAM",
    "MSG_TIMER",
    "SM_INT_MSG",
    "CONN_SENDING",
    "CONN_ACK_SENDING",
    "CONN_FIN",
    SB_LABEL_END
};

static const char *ga_sb_utrace_api_mon_msgtype_labels[] = {
    "MsgType_Change",
    "MsgType_Close",
    "MsgType_Event",
    "MsgType_NodeAdded",
    "MsgType_NodeChanged",
    "MsgType_NodeDeleted",
    "MsgType_NodeDown",
    "MsgType_NodeJoining",
    "MsgType_NodeQuiesce",
    "MsgType_NodeUp",
    "MsgType_Open",
    "MsgType_ProcessCreated",
    "MsgType_ProcessDeath",
    "MsgType_ReintegrationError",
    "MsgType_Service",
    "MsgType_Shutdown",
    "MsgType_SpareUp",
    "MsgType_Invalid",
    SB_LABEL_END
};

static const char *ga_sb_utrace_api_mon_reqtype_labels[] = {
    "ReqType_Close",
    "ReqType_DelProcessNs",
    "ReqType_Dump",
    "ReqType_Event",
    "ReqType_Exit",
    "ReqType_Get",
    "ReqType_InstanceId",
    "ReqType_Kill",
    "ReqType_MonStats",
    "ReqType_Mount",
    "ReqType_NameServerAdd",
    "ReqType_NameServerDelete",
    "ReqType_NameServerStart",
    "ReqType_NameServerStop",
    "ReqType_NewProcess",
    "ReqType_NewProcessNs",
    "ReqType_NodeAdd",
    "ReqType_NodeDelete",
    "ReqType_NodeDown",
    "ReqType_NodeInfo",
    "ReqType_NodeName",
    "ReqType_NodeUp",
    "ReqType_Notice",
    "ReqType_Notify",
    "ReqType_Open",
    "ReqType_OpenInfo",
    "ReqType_PersistAdd",
    "ReqType_PersistDelete",
    "ReqType_PNodeInfo",
    "ReqType_ProcessInfo",
    "ReqType_ProcessInfoCont",
    "ReqType_ProcessInfoNs",
    "ReqType_Set",
    "ReqType_Shutdown",
    "ReqType_ShutdownNs",
    "ReqType_Startup",
    "ReqType_TmLeader",
    "ReqType_TmReady",
    "ReqType_ZoneInfo",
    SB_LABEL_END
};

static const char *ga_sb_utrace_api_op_labels[] = {
    "FS_AWAITIOX",
    "FS_AWAITIOX_comp",
    "FS_AWAITIOX_comp_open",
    "FS_AWAITIOXTS",
    "FS_BUF_READUPDATEX",
    "FS_CANCEL",
    "FS_CANCELREQ",
    "FS_CLOSE",
    "fs_comp_io_write",
    "fs_comp_io_writeread",
    "fs_comp_reply_close_noopen",
    "fs_comp_reply_close_nosm",
    "fs_comp_reply_err",
    "fs_comp_reply_nosm",
    "fs_comp_reply_open",
    "fs_comp_reply_open_nosm",
    "fs_comp_sm_change",
    "fs_comp_sm_close",
    "fs_comp_sm_cpudown",
    "fs_comp_sm_cpuup",
    "fs_comp_sm_open",
    "fs_comp_sm_procdeath",
    "fs_comp_sm_shutdown",
    "fs_comp_sm_unknown",
    "fs_comp_tpop",
    "fs_exit",
    "FS_OPEN",
    "fs_open_md",
    "fs_open_thread_start",
    "fs_open_thread_stop",
    "FS_READX",
    "FS_READUPDATEX",
    "FS_REPLYX",
    "FS_WRITEX",
    "FS_WRITEREADX",
    "ms_exit",
    "ms_locio_acquire_msg",
    "ms_locio_release_msg",
    "ms_locio_send",
    "ms_locio_send_recv",
    "ms_locio_wait_for_event",
    "ms_mon_msg",
    "MS_MSG_ABANDON",
    "MS_MSG_BREAK",
    "MS_MSG_LINK",
    "MS_MSG_LISTEN_abandon",
    "MS_MSG_LISTEN_ireq",
    "MS_MSG_LISTEN_ireq_ctrl",
    "MS_MSG_LISTEN_ireq_data",
    "MS_MSG_LISTEN_ldone",
    "MS_MSG_LISTEN_tpop",
    "MS_MSG_REPLY",
    "ms_req_done",
    "ms_send_md",
    "MSG_INIT",
    "MSG_INIT_ATTACH",
    "MSG_MON_CLOSE_PROCESS",
    "MSG_MON_DEREGISTER_DEATH_NOTIFICATION",
    "MSG_MON_DEREGISTER_DEATH_NOTIFICATION2",
    "MSG_MON_DEREGISTER_DEATH_NOTIFICATION3",
    "MSG_MON_DEREGISTER_DEATH_NOTIFICATION_NAME",
    "MSG_MON_DEREGISTER_DEATH_NOTIFICATION_NAME2",
    "MSG_MON_DUMP_PROCESS_ID",
    "MSG_MON_DUMP_PROCESS_ID2",
    "MSG_MON_DUMP_PROCESS_NAME",
    "MSG_MON_ENABLE_MON_MESSAGES",
    "MSG_MON_EVENT_SEND",
    "MSG_MON_EVENT_SEND_NAME",
    "MSG_MON_EVENT_WAIT",
    "MSG_MON_EVENT_WAIT2",
    "MSG_MON_GET_NODE_INFO",
    "MSG_MON_GET_NODE_INFO2",
    "MSG_MON_GET_NODE_INFO_ALL",
    "MSG_MON_GET_NODE_INFO_DETAIL",
    "MSG_MON_GET_OPEN_INFO",
    "MSG_MON_GET_OPEN_INFO_MAX",
    "MSG_MON_GET_PROCESS_INFO",
    "MSG_MON_GET_PROCESS_INFO2",
    "MSG_MON_GET_PROCESS_INFO_DETAIL",
    "MSG_MON_GET_PROCESS_INFO_TYPE",
    "MSG_MON_GET_PROCESS_NAME",
    "MSG_MON_GET_PROCESS_NAME2",
    "MSG_MON_GET_TM_SEQ",
    "MSG_MON_GET_TRANS_INFO_PROCESS",
    "MSG_MON_GET_TRANS_INFO_TRANSID",
    "MSG_MON_GET_ZONE_INFO",
    "MSG_MON_GET_ZONE_INFO_DETAIL",
    "MSG_MON_GET_INSTANCE_ID",
    "MSG_MON_MOUNT_DEVICE,",
    "MSG_MON_MOUNT_DEVICE2",
    "MSG_MON_NODE_DOWN",
    "MSG_MON_NODE_DOWN2",
    "MSG_MON_NODE_UP",
    "MSG_MON_OPEN_PROCESS",
    "MSG_MON_OPEN_PROCESS_BACKUP",
    "MSG_MON_OPEN_PROCESS_IC",
    "MSG_MON_OPEN_PROCESS_NOWAIT_CB",
    "msg_mon_open_process_oid",
    "MSG_MON_OPEN_PROCESS_SELF",
    "MSG_MON_OPEN_PROCESS_SELF_IC",
    "MSG_MON_PROCESS_CLOSE",
    "MSG_MON_PROCESS_STARTUP",
    "MSG_MON_PROCESS_STARTUP2",
    "MSG_MON_PROCESS_STARTUP3",
    "MSG_MON_PROCESS_STARTUP4",
    "MSG_MON_PROCESS_SHUTDOWN",
    "MSG_MON_PROCESS_SHUTDOWN_FAST",
    "MSG_MON_PROCESS_SHUTDOWN_NOW",
    "MSG_MON_REG_GET",
    "MSG_MON_REG_SET",
    "MSG_MON_REGISTER_DEATH_NOTIFICATION",
    "MSG_MON_REGISTER_DEATH_NOTIFICATION2",
    "MSG_MON_REGISTER_DEATH_NOTIFICATION3",
    "MSG_MON_REGISTER_DEATH_NOTIFICATION4",
    "MSG_MON_REGISTER_DEATH_NOTIFICATION_NAME",
    "MSG_MON_REGISTER_DEATH_NOTIFICATION_NAME2",
    "MSG_MON_REOPEN_PROCESS",
    "MSG_MON_SHUTDOWN",
    "MSG_MON_START_PROCESS",
    "MSG_MON_START_PROCESS2",
    "MSG_MON_START_PROCESS_NOWAIT",
    "MSG_MON_START_PROCESS_NOWAIT2",
    "MSG_MON_START_PROCESS_NOWAIT_CB",
    "MSG_MON_START_PROCESS_NOWAIT_CB2",
    "MSG_MON_STOP_PROCESS",
    "MSG_MON_STOP_PROCESS2",
    "MSG_MON_STFSD_SEND",
    "MSG_MON_TM_LEADER_SET",
    "MSG_MON_TM_READY",
    "MSG_MON_TMSYNC_ISSUE",
    "MSG_MON_TMSYNC_REGISTER",
    "MSG_MON_TRANS_DELIST",
    "MSG_MON_TRANS_END",
    "MSG_MON_TRANS_ENLIST",
    "MS_CONTROLMESSAGESYSTEM",
    "MS_MESSAGESYSTEMINFO",
    "thread_exit",
    "thread_exit_disp1",
    "thread_exit_disp2",
    "thread_exit_name",
    "thread_start",
    "thread_start_disp1",
    "thread_start_disp2",
    "thread_start_err",
    "thread_start_name",
    SB_LABEL_END
};

static const char *ga_timer_kind_labels[] = {
    "CB",
    "COMPQ",
    SB_LABEL_END
};

static const char *ga_timer_list_labels[] = {
    "NONE",
    "TIMER",
    "COMP",
    SB_LABEL_END
};

enum {
    SB_LABEL_LIMIT_FS_FSDIALECT_TYPE_LO = DIALECT_FS_FS,
    SB_LABEL_LIMIT_FS_FSDIALECT_TYPE_HI = DIALECT_FS_FS
};
SB_Label_Map gv_fs_fsdialect_type_label_map = {
    SB_LABEL_LIMIT_FS_FSDIALECT_TYPE_LO,
    SB_LABEL_LIMIT_FS_FSDIALECT_TYPE_HI,
    "<unknown>", ga_fs_fsdialect_type_labels
};

enum {
    SB_LABEL_LIMIT_FS_FSREQ_TYPE_LO = FS_FS_OPEN,
    SB_LABEL_LIMIT_FS_FSREQ_TYPE_HI = FS_FS_SIMPLE_REPLY
};
SB_Label_Map gv_fs_fsreq_type_label_map = {
    SB_LABEL_LIMIT_FS_FSREQ_TYPE_LO,
    SB_LABEL_LIMIT_FS_FSREQ_TYPE_HI,
    "<unknown>", ga_fs_fsreq_type_labels
};

enum {
    MS_LABEL_LIMIT_ERRNO_TYPE_LO = EPERM,
    MS_LABEL_LIMIT_ERRNO_TYPE_HI = ERFKILL
};
SB_Label_Map gv_ms_errno_type_label_map = {
    MS_LABEL_LIMIT_ERRNO_TYPE_LO,
    MS_LABEL_LIMIT_ERRNO_TYPE_HI,
    "<unknown>",
    ga_ms_errno_type_labels
};

enum {
    MS_LABEL_LIMIT_MON_CONFIG_TYPE_LO = MS_Mon_ConfigType_Undefined,
    MS_LABEL_LIMIT_MON_CONFIG_TYPE_HI = MS_Mon_ConfigType_Process
};
static SB_Label_Map gv_ms_mon_config_type_label_map = {
    MS_LABEL_LIMIT_MON_CONFIG_TYPE_LO,
    MS_LABEL_LIMIT_MON_CONFIG_TYPE_HI,
    "MS_Mon_ConfigType_<unknown>",
    ga_ms_mon_config_type_labels
};
static SB_Label_Map gv_ms_mon_config_type_short_label_map = {
    MS_LABEL_LIMIT_MON_CONFIG_TYPE_LO,
    MS_LABEL_LIMIT_MON_CONFIG_TYPE_HI,
    "<unknown>",
    ga_ms_mon_config_type_short_labels
};

enum {
    MS_LABEL_LIMIT_MON_DEVICE_STATE_LO = MS_Mon_State_UnMounted,
    MS_LABEL_LIMIT_MON_DEVICE_STATE_HI = MS_Mon_State_Mounted
};
SB_Label_Map gv_ms_mon_device_state_label_map = {
    MS_LABEL_LIMIT_MON_DEVICE_STATE_LO,
    MS_LABEL_LIMIT_MON_DEVICE_STATE_HI,
    "MS_Mon_State_<unknown>",
    ga_ms_mon_device_state_labels
};
static SB_Label_Map gv_ms_mon_device_state_short_label_map = {
    MS_LABEL_LIMIT_MON_DEVICE_STATE_LO,
    MS_LABEL_LIMIT_MON_DEVICE_STATE_HI,
    "<unknown>",
    ga_ms_mon_device_state_short_labels
};

enum {
    MS_LABEL_LIMIT_MON_MSG_TYPE_LO = MS_MsgType_Change,
    MS_LABEL_LIMIT_MON_MSG_TYPE_HI = MsgType_SpareUp
};
SB_Label_Map gv_ms_mon_msg_type_label_map = {
    MS_LABEL_LIMIT_MON_MSG_TYPE_LO,
    MS_LABEL_LIMIT_MON_MSG_TYPE_HI,
    "MS_MsgType_<unknown>",
    ga_ms_mon_msg_type_labels
};
static SB_Label_Map gv_ms_mon_msg_type_short_label_map = {
    MS_LABEL_LIMIT_MON_MSG_TYPE_LO,
    MS_LABEL_LIMIT_MON_MSG_TYPE_HI,
    "<unknown>",
    ga_ms_mon_msg_type_short_labels
};

enum {
    MS_LABEL_LIMIT_MON_PROC_STATE_LO = MS_Mon_State_Unknown,
    MS_LABEL_LIMIT_MON_PROC_STATE_HI = MS_Mon_State_Takeover
};
SB_Label_Map gv_ms_mon_proc_state_label_map = {
    MS_LABEL_LIMIT_MON_PROC_STATE_LO,
    MS_LABEL_LIMIT_MON_PROC_STATE_HI,
    "MS_Mon_State_<unknown>",
    ga_ms_mon_proc_state_labels
};
static SB_Label_Map gv_ms_mon_proc_state_short_label_map = {
    MS_LABEL_LIMIT_MON_PROC_STATE_LO,
    MS_LABEL_LIMIT_MON_PROC_STATE_HI,
    "<unknown>",
    ga_ms_mon_proc_state_short_labels
};

enum {
    MS_LABEL_LIMIT_MON_PROCESS_TYPE_LO = MS_ProcessType_Undefined,
    MS_LABEL_LIMIT_MON_PROCESS_TYPE_HI = MS_ProcessType_PERSIST
};
SB_Label_Map gv_ms_mon_process_type_label_map = {
    MS_LABEL_LIMIT_MON_PROCESS_TYPE_LO,
    MS_LABEL_LIMIT_MON_PROCESS_TYPE_HI,
    "MS_ProcessType_<unknown>",
    ga_ms_mon_process_type_labels
};
static SB_Label_Map gv_ms_mon_process_type_short_label_map = {
    MS_LABEL_LIMIT_MON_PROCESS_TYPE_LO,
    MS_LABEL_LIMIT_MON_PROCESS_TYPE_HI,
    "<unknown>",
    ga_ms_mon_process_type_short_labels
};

enum {
    MS_LABEL_LIMIT_MON_REQ_TYPE_LO = MS_ReqType_Close,
    MS_LABEL_LIMIT_MON_REQ_TYPE_HI = MS_ReqType_ZoneInfo
};
SB_Label_Map gv_ms_mon_req_type_label_map = {
    MS_LABEL_LIMIT_MON_REQ_TYPE_LO,
    MS_LABEL_LIMIT_MON_REQ_TYPE_HI,
    "MS_ReqType_<unknown>",
    ga_ms_mon_req_type_labels
};
static SB_Label_Map gv_ms_mon_req_type_short_label_map = {
    MS_LABEL_LIMIT_MON_REQ_TYPE_LO,
    MS_LABEL_LIMIT_MON_REQ_TYPE_HI,
    "<unknown>",
    ga_ms_mon_req_type_short_labels
};

enum {
    MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_LO = MS_Mon_ShutdownLevel_Undefined,
    MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_HI = MS_Mon_ShutdownLevel_Abrupt
};
SB_Label_Map gv_ms_mon_shutdown_level_label_map = {
    MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_LO,
    MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_HI,
    "MS_Mon_ShutdownLevel_<unknown>",
    ga_ms_mon_shutdown_level_labels
};
static SB_Label_Map gv_ms_mon_shutdown_level_short_label_map = {
    MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_LO,
    MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_HI,
    "<unknown>",
    ga_ms_mon_shutdown_level_short_labels
};

enum {
    SB_LABEL_LIMIT_LOG_AGGR_CAP_LO = SB_LOG_AGGR_CAP_AFIRST + 1,
    SB_LABEL_LIMIT_LOG_AGGR_CAP_HI = SB_LOG_AGGR_CAP_ZLAST - 1
};
SB_Label_Map gv_sb_log_aggr_cap_label_map = {
    SB_LABEL_LIMIT_LOG_AGGR_CAP_LO,
    SB_LABEL_LIMIT_LOG_AGGR_CAP_HI,
    "<unknown>-table new-cap=%d\n",
    ga_sb_log_aggr_cap_labels
};

enum {
    SB_LABEL_LIMIT_MD_TYPE_LO = MS_PMH_TYPE_INV,
    SB_LABEL_LIMIT_MD_TYPE_HI = MS_PMH_TYPE_REPLY_NACK
};
SB_Label_Map gv_sb_md_type_label_map = {
    SB_LABEL_LIMIT_MD_TYPE_LO,
    SB_LABEL_LIMIT_MD_TYPE_HI,
    "<unknown>", ga_sb_md_type_labels
};

enum {
    SB_LABEL_LIMIT_MSMON_REPLY_TYPE_LO = ReplyType_Generic,
    SB_LABEL_LIMIT_MSMON_REPLY_TYPE_HI = ReplyType_ZoneInfo
};
SB_Label_Map gv_sb_msmon_reply_type_label_map = {
    SB_LABEL_LIMIT_MSMON_REPLY_TYPE_LO,
    SB_LABEL_LIMIT_MSMON_REPLY_TYPE_HI,
    "<unknown>", ga_sb_msmon_reply_type_labels
};

enum {
    SB_LABEL_LIMIT_STREAM_MD_STATE_LO = MD_STATE_AFIRST,
    SB_LABEL_LIMIT_STREAM_MD_STATE_HI = MD_STATE_ZLAST - 1
};
SB_Label_Map gv_sb_stream_md_state_label_map = {
    SB_LABEL_LIMIT_STREAM_MD_STATE_LO,
    SB_LABEL_LIMIT_STREAM_MD_STATE_HI,
    "<unknown>", ga_sb_stream_md_state_labels
};

enum {
    SB_LABEL_LIMIT_UTRACE_API_MON_MSGTYPE_LO = MsgType_Change,
    SB_LABEL_LIMIT_UTRACE_API_MON_MSGTYPE_HI = MsgType_Invalid
};
SB_Label_Map gv_sb_utrace_api_mon_msgtype_label_map = {
    SB_LABEL_LIMIT_UTRACE_API_MON_MSGTYPE_LO,
    SB_LABEL_LIMIT_UTRACE_API_MON_MSGTYPE_HI,
    "<unknown>",
    ga_sb_utrace_api_mon_msgtype_labels
};

enum {
    SB_LABEL_LIMIT_UTRACE_API_MON_REQTYPE_LO = ReqType_Close,
    SB_LABEL_LIMIT_UTRACE_API_MON_REQTYPE_HI = ReqType_ZoneInfo
};
SB_Label_Map gv_sb_utrace_api_mon_reqtype_label_map = {
    SB_LABEL_LIMIT_UTRACE_API_MON_REQTYPE_LO,
    SB_LABEL_LIMIT_UTRACE_API_MON_REQTYPE_HI,
    "<unknown>",
    ga_sb_utrace_api_mon_reqtype_labels
};

enum {
    SB_LABEL_LIMIT_UTRACE_API_OP_LO = SB_UTRACE_API_OP_AFIRST + 1,
    SB_LABEL_LIMIT_UTRACE_API_OP_HI = SB_UTRACE_API_OP_ZLAST - 1
};
SB_Label_Map gv_sb_utrace_api_op_label_map = {
    SB_LABEL_LIMIT_UTRACE_API_OP_LO,
    SB_LABEL_LIMIT_UTRACE_API_OP_HI,
    "<unknown>",
    ga_sb_utrace_api_op_labels
};

enum {
    SB_LABEL_LIMIT_TIMER_KIND_LO = TIMER_TLE_KIND_CB,
    SB_LABEL_LIMIT_TIMER_KIND_HI = TIMER_TLE_KIND_COMPQ
};
SB_Label_Map gv_timer_kind_label_map = {
    SB_LABEL_LIMIT_TIMER_KIND_LO,
    SB_LABEL_LIMIT_TIMER_KIND_HI,
    "<unknown>",
    ga_timer_kind_labels
};

enum {
    SB_LABEL_LIMIT_TIMER_LIST_LO = LIST_NONE,
    SB_LABEL_LIMIT_TIMER_LIST_HI = LIST_COMP
};
SB_Label_Map gv_timer_list_label_map = {
    SB_LABEL_LIMIT_TIMER_LIST_LO,
    SB_LABEL_LIMIT_TIMER_LIST_HI,
    "<unknown>",
    ga_timer_list_labels
};

#define SB_LABEL_CHK(name, low, high) \
SB_util_static_assert((sizeof(name)/sizeof(const char *)) == (high - low + 2));

//
// statically check that labels length matches map lengths
//
void sb_label_map_init() {
    SB_LABEL_CHK(ga_fs_fsdialect_type_labels,
                 SB_LABEL_LIMIT_FS_FSDIALECT_TYPE_LO,
                 SB_LABEL_LIMIT_FS_FSDIALECT_TYPE_HI)
    SB_LABEL_CHK(ga_fs_fsreq_type_labels,
                 SB_LABEL_LIMIT_FS_FSREQ_TYPE_LO,
                 SB_LABEL_LIMIT_FS_FSREQ_TYPE_HI)
    SB_LABEL_CHK(ga_ms_errno_type_labels,
                 MS_LABEL_LIMIT_ERRNO_TYPE_LO,
                 MS_LABEL_LIMIT_ERRNO_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_config_type_labels,
                 MS_LABEL_LIMIT_MON_CONFIG_TYPE_LO,
                 MS_LABEL_LIMIT_MON_CONFIG_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_config_type_short_labels,
                 MS_LABEL_LIMIT_MON_CONFIG_TYPE_LO,
                 MS_LABEL_LIMIT_MON_CONFIG_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_device_state_labels,
                 MS_LABEL_LIMIT_MON_DEVICE_STATE_LO,
                 MS_LABEL_LIMIT_MON_DEVICE_STATE_HI)
    SB_LABEL_CHK(ga_ms_mon_device_state_short_labels,
                 MS_LABEL_LIMIT_MON_DEVICE_STATE_LO,
                 MS_LABEL_LIMIT_MON_DEVICE_STATE_HI)
    SB_LABEL_CHK(ga_ms_mon_msg_type_labels,
                 MS_LABEL_LIMIT_MON_MSG_TYPE_LO,
                 MS_LABEL_LIMIT_MON_MSG_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_msg_type_short_labels,
                 MS_LABEL_LIMIT_MON_MSG_TYPE_LO,
                 MS_LABEL_LIMIT_MON_MSG_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_req_type_labels,
                 MS_LABEL_LIMIT_MON_REQ_TYPE_LO,
                 MS_LABEL_LIMIT_MON_REQ_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_req_type_short_labels,
                 MS_LABEL_LIMIT_MON_REQ_TYPE_LO,
                 MS_LABEL_LIMIT_MON_REQ_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_proc_state_labels,
                 MS_LABEL_LIMIT_MON_PROC_STATE_LO,
                 MS_LABEL_LIMIT_MON_PROC_STATE_HI)
    SB_LABEL_CHK(ga_ms_mon_proc_state_short_labels,
                 MS_LABEL_LIMIT_MON_PROC_STATE_LO,
                 MS_LABEL_LIMIT_MON_PROC_STATE_HI)
    SB_LABEL_CHK(ga_ms_mon_process_type_labels,
                 MS_LABEL_LIMIT_MON_PROCESS_TYPE_LO,
                 MS_LABEL_LIMIT_MON_PROCESS_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_process_type_short_labels,
                 MS_LABEL_LIMIT_MON_PROCESS_TYPE_LO,
                 MS_LABEL_LIMIT_MON_PROCESS_TYPE_HI)
    SB_LABEL_CHK(ga_ms_mon_shutdown_level_labels,
                 MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_LO,
                 MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_HI)
    SB_LABEL_CHK(ga_ms_mon_shutdown_level_short_labels,
                 MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_LO,
                 MS_LABEL_LIMIT_MON_SHUTDOWN_LEVEL_HI)
    SB_LABEL_CHK(ga_sb_log_aggr_cap_labels,
                 SB_LABEL_LIMIT_LOG_AGGR_CAP_LO,
                 SB_LABEL_LIMIT_LOG_AGGR_CAP_HI)
    SB_LABEL_CHK(ga_sb_md_type_labels,
                 SB_LABEL_LIMIT_MD_TYPE_LO,
                 SB_LABEL_LIMIT_MD_TYPE_HI)
    SB_LABEL_CHK(ga_sb_msmon_reply_type_labels,
                 SB_LABEL_LIMIT_MSMON_REPLY_TYPE_LO,
                 SB_LABEL_LIMIT_MSMON_REPLY_TYPE_HI)
    SB_LABEL_CHK(ga_sb_stream_md_state_labels,
                 SB_LABEL_LIMIT_STREAM_MD_STATE_LO,
                 SB_LABEL_LIMIT_STREAM_MD_STATE_HI)
    SB_LABEL_CHK(ga_sb_utrace_api_mon_msgtype_labels,
                 SB_LABEL_LIMIT_UTRACE_API_MON_MSGTYPE_LO,
                 SB_LABEL_LIMIT_UTRACE_API_MON_MSGTYPE_HI)
    SB_LABEL_CHK(ga_sb_utrace_api_mon_reqtype_labels,
                 SB_LABEL_LIMIT_UTRACE_API_MON_REQTYPE_LO,
                 SB_LABEL_LIMIT_UTRACE_API_MON_REQTYPE_HI)
    SB_LABEL_CHK(ga_sb_utrace_api_op_labels,
                 SB_LABEL_LIMIT_UTRACE_API_OP_LO,
                 SB_LABEL_LIMIT_UTRACE_API_OP_HI)
    SB_LABEL_CHK(ga_timer_kind_labels,
                 SB_LABEL_LIMIT_TIMER_KIND_LO,
                 SB_LABEL_LIMIT_TIMER_KIND_HI)
    SB_LABEL_CHK(ga_timer_list_labels,
                 SB_LABEL_LIMIT_TIMER_LIST_LO,
                 SB_LABEL_LIMIT_TIMER_LIST_HI)
}

//
// Get label for errno
//
SB_Export const char *SB_get_label_errno(int pv_value) {
    return SB_get_label(&gv_ms_errno_type_label_map, pv_value);
}

//
// Get label for MS_Mon_ConfigType (MS_Mon_ConfigType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_configtype(int pv_value) {
    return SB_get_label(&gv_ms_mon_config_type_label_map, pv_value);
}

//
// Get (short) label for MS_Mon_ConfigType (MS_Mon_ConfigType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_configtype_short(int pv_value) {
    return SB_get_label(&gv_ms_mon_config_type_short_label_map, pv_value);
}

//
// Get label for MS_MON_DEVICE_STATE (MS_Mon_State_<value>)
//
SB_Export const char *SB_get_label_ms_mon_device_state(int pv_value) {
    return SB_get_label(&gv_ms_mon_device_state_label_map, pv_value);
}

//
// Get (short) label for MS_MON_DEVICE_STATE (MS_Mon_State_<value>)
//
SB_Export const char *SB_get_label_ms_mon_device_state_short(int pv_value) {
    return SB_get_label(&gv_ms_mon_device_state_short_label_map, pv_value);
}

//
// Get label for MS_Mon_MSGTYPE (MS_MsgType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_msgtype(int pv_value) {
    return SB_get_label(&gv_ms_mon_msg_type_label_map, pv_value);
}

//
// Get (short) label for MS_Mon_MSGTYPE (MS_MsgType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_msgtype_short(int pv_value) {
    return SB_get_label(&gv_ms_mon_msg_type_short_label_map, pv_value);
}

//
// Get label for MS_MON_PROC_STATE (MS_Mon_State_<value)
//
SB_Export const char *SB_get_label_ms_mon_proc_state(int pv_value) {
    return SB_get_label(&gv_ms_mon_proc_state_label_map, pv_value);
}

//
// Get (short) label for MS_MON_PROC_STATE (MS_Mon_State_<value)
//
SB_Export const char *SB_get_label_ms_mon_proc_state_short(int pv_value) {
    return SB_get_label(&gv_ms_mon_proc_state_short_label_map, pv_value);
}

//
// Get label for MS_Mon_PROCESSTYPE (MS_ProcessType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_processtype(int pv_value) {
    return SB_get_label(&gv_ms_mon_process_type_label_map, pv_value);
}

//
// Get (short) label for MS_Mon_PROCESSTYPE (MS_ProcessType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_processtype_short(int pv_value) {
    return SB_get_label(&gv_ms_mon_process_type_short_label_map, pv_value);
}

//
// Get label for MS_Mon_REQTYPE (MS_ReqType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_reqtype(int pv_value) {
    return SB_get_label(&gv_ms_mon_req_type_label_map, pv_value);
}

//
// Get (short) label for MS_Mon_REQTYPE (MS_ReqType_<value>)
//
SB_Export const char *SB_get_label_ms_mon_reqtype_short(int pv_value) {
    return SB_get_label(&gv_ms_mon_req_type_short_label_map, pv_value);
}

//
// Get label for MS_MON_ShutdownLevel (MS_Mon_ShutdownLevel_<value>)
//
SB_Export const char *SB_get_label_ms_mon_shutdownlevel(int pv_value) {
    return SB_get_label(&gv_ms_mon_shutdown_level_label_map, pv_value);
}

//
// Get (short) label for MS_MON_ShutdownLevel (MS_Mon_ShutdownLevel_<value>)
//
SB_Export const char *SB_get_label_ms_mon_shutdownlevel_short(int pv_value) {
    return SB_get_label(&gv_ms_mon_shutdown_level_short_label_map, pv_value);
}

#if 0 // standalone test
int main() {
    const char *lp_label;
    lp_label = SB_get_label_errno(EPERM);
    assert(strcmp(lp_label, "EPERM") == 0);
    lp_label = SB_get_label_ms_mon_configtype(MS_Mon_ConfigType_Undefined);
    assert(strcmp(lp_label, "MS_Mon_ConfigType_Undefined") == 0);
    lp_label = SB_get_label_ms_mon_configtype_short(MS_Mon_ConfigType_Undefined);
    assert(strcmp(lp_label, "Undefined") == 0);
    lp_label = SB_get_label_ms_mon_device_state(MS_Mon_State_UnMounted);
    assert(strcmp(lp_label, "MS_Mon_State_UnMounted") == 0);
    lp_label = SB_get_label_ms_mon_device_state_short(MS_Mon_State_UnMounted);
    assert(strcmp(lp_label, "UnMounted") == 0);
    lp_label = SB_get_label_ms_mon_msgtype(MS_MsgType_Change);
    assert(strcmp(lp_label, "MS_MsgType_Change") == 0);
    lp_label = SB_get_label_ms_mon_msgtype_short(MS_MsgType_Change);
    assert(strcmp(lp_label, "Change") == 0);
    lp_label = SB_get_label_ms_mon_proc_state(MS_Mon_State_Unknown);
    assert(strcmp(lp_label, "MS_Mon_State_Unknown") == 0);
    lp_label = SB_get_label_ms_mon_proc_state_short(MS_Mon_State_Unknown);
    assert(strcmp(lp_label, "Unknown") == 0);
    lp_label = SB_get_label_ms_mon_processtype(MS_ProcessType_Undefined);
    assert(strcmp(lp_label, "MS_ProcessType_Undefined") == 0);
    lp_label = SB_get_label_ms_mon_processtype_short(MS_ProcessType_Undefined);
    assert(strcmp(lp_label, "Undefined") == 0);
    lp_label = SB_get_label_ms_mon_reqtype(MS_ReqType_Close);
    assert(strcmp(lp_label, "MS_ReqType_Close") == 0);
    lp_label = SB_get_label_ms_mon_reqtype_short(MS_ReqType_Close);
    assert(strcmp(lp_label, "Close") == 0);
    lp_label = SB_get_label_ms_mon_shutdownlevel(MS_Mon_ShutdownLevel_Undefined);
    assert(strcmp(lp_label, "MS_Mon_ShutdownLevel_Undefined") == 0);
    lp_label = SB_get_label_ms_mon_shutdownlevel_short(MS_Mon_ShutdownLevel_Undefined);
    assert(strcmp(lp_label, "Undefined") == 0);
    return 0;
}
#endif
