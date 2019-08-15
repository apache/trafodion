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
// Message-system module
//
#ifndef __SB_MS_H_
#define __SB_MS_H_

#include <stddef.h>     // size_t

#include <time.h>       // timespec

#include "int/conv.h"
#include "int/diag.h"
#include "int/exp.h"
#include "int/opts.h"
#include "int/types.h"

#include "excep.h"
#include "mslimits.h"

// SRE flags
enum {
    BSRE_PROCDEAD = 32,
    BSRE_MON      = 8,
    BSRE_REMM     = 4,
    BSRE_REMIDM   = 2,
    BSRE_SECUREM  = 1
};
enum {
    XSRE_PROCDEAD = 32,
    XSRE_MON      = 8,
    XSRE_REMM     = 4,
    XSRE_REMIDM   = 2,
    XSRE_SECUREM  = 1
};

// listen options
enum {
    BLISTEN_TEST_ILIMREQM    = 0x80, // test incoming lim requests
    BLISTEN_ALLOW_ILIMREQM   = 0x40, // allow incoming lim requests
    BLISTEN_TEST_IREQM       = 0x20, // test incoming requests
    BLISTEN_ALLOW_ABANDONM   = 0x10, // allow cancellation notifications
    BLISTEN_ALLOW_LDONEM     = 0x4,  // allow LDONE notifications
    BLISTEN_ALLOW_TPOPM      = 0x2,  // allow timer-pop notifications
    BLISTEN_ALLOW_IREQM      = 0x1   // allow normal incoming requests
};
enum {
    BLISTEN_DEFAULTM         = 0x17  // allow everything (not test/not lim)
};
enum {
    XLISTEN_TEST_ILIMREQM    = 0x80, // test incoming lim requests
    XLISTEN_ALLOW_ILIMREQM   = 0x40, // allow incoming lim requests
    XLISTEN_TEST_IREQM       = 0x20, // test incoming requests
    XLISTEN_ALLOW_ABANDONM   = 0x10, // allow cancellation notifications
    XLISTEN_ALLOW_LDONEM     = 0x4,  // allow LDONE notifications
    XLISTEN_ALLOW_TPOPM      = 0x2,  // allow timer-pop notifications
    XLISTEN_ALLOW_IREQM      = 0x1   // allow normal incoming requests
};
enum {
    XLISTEN_DEFAULTM         = 0x17  // allow everything (not test/not lim)
};

// link options
enum {
    BMSG_LINK_MSINTERCEPTOR  = 0x800, // msinterceptor (should only be set by interceptor user)
    //                         0x400, // unused (legacy-MSG_LINK_BIG)
    BMSG_LINK_FSREQ          = 0x200, // fsreq (should only be set by fs)
    BMSG_LINK_FSDONEQ        = 0x010, // fs DONE queueing
    BMSG_LINK_LDONEQ         = 0x008, // LDONE queueing
    BMSG_LINK_FSDONETSQ      = 0x004  // fs TS DONE queueing
};
enum {
    XMSG_LINK_FSREQ          = 0x200, // fsreq (should only be set by fs)
    XMSG_LINK_FSDONEQ        = 0x010, // fs DONE queueing
    XMSG_LINK_LDONEQ         = 0x008, // LDONE queueing
    XMSG_LINK_FSDONETSQ      = 0x004  // fs TS DONE queueing
};

// listen return types
enum {
    BSRETYPE_NOWORK  = 0,           // no work found
    BSRETYPE_IREQ    = 1,           // incoming request to this process
    BSRETYPE_TPOP    = 2,           // timer expiration
    BSRETYPE_LDONE   = 3,           // LDONE completion
    BSRETYPE_ABANDON = 4            // ABANDON
};
enum {
    XSRETYPE_NOWORK  = 0,           // no work found
    XSRETYPE_IREQ    = 1,           // incoming request to this process
    XSRETYPE_TPOP    = 2,           // timer expiration
    XSRETYPE_LDONE   = 3,           // LDONE completion
    XSRETYPE_ABANDON = 4            // ABANDON
};
typedef unsigned short ushort;

typedef enum {
    TMLIB_FUN_REG_TX        = 1,
    TMLIB_FUN_CLEAR_TX      = 2,
    TMLIB_FUN_REINSTATE_TX  = 3,
    TMLIB_FUN_GET_TX        = 4
} MS_Mon_Tmlib_Fun_Type;
typedef SB_Transid_Type  MS_Mon_Transid_Type;
typedef SB_Transseq_Type MS_Mon_Transseq_Type;
typedef int (*MS_Mon_Tmlib_Cb_Type)(MS_Mon_Tmlib_Fun_Type  fun,
                                    MS_Mon_Transid_Type    transid_in,
                                    MS_Mon_Transid_Type   *transid_out);
typedef int (*MS_Mon_Tmlib2_Cb_Type)(MS_Mon_Tmlib_Fun_Type  fun,
                                     MS_Mon_Transid_Type    transid_in,
                                     MS_Mon_Transid_Type   *transid_out,
                                     MS_Mon_Transseq_Type   startid_in,
                                     MS_Mon_Transseq_Type  *startid_out);
typedef int (*MS_Mon_TmSync_Cb_Type)(void *data, int len, int handle);
typedef void (*MS_Mon_Trace_Cb_Type)(const char *key, const char *value);

typedef void *(*MS_Buf_Alloc_Cb_Type)(size_t len);
typedef void (*MS_Buf_Free_Cb_Type)(void *buf);
typedef int  (*MS_Lim_Queue_Cb_Type)(int    msg_ptype,
                                     short *msg_ctrl,
                                     int    msg_ctrl_len,
                                     char  *msg_data,
                                     int    msg_data_len);

// if these are updated, check msg_mon_init
typedef enum {
    MS_Mon_State_UnMounted = 0,
    MS_Mon_State_Mounted
} MS_MON_DEVICE_STATE;
typedef enum {
    MS_Mon_JoiningPhase_Unknown = 0,
    MS_Mon_JoiningPhase_1,
    MS_Mon_JoiningPhase_2,
    MS_Mon_JoiningPhase_3,
    MS_Mon_JoiningPhase_Invalid
} MS_MON_JOINING_PHASE;
typedef enum {
    MS_Mon_State_Unknown = 0,
    MS_Mon_State_Up,
    MS_Mon_State_Down,
    MS_Mon_State_Stopped,
    MS_Mon_State_Shutdown,
    MS_Mon_State_Unlinked,
    MS_Mon_State_Initializing,
    MS_Mon_State_Joining,
    MS_Mon_State_Merging,
    MS_Mon_State_Merged,
    MS_Mon_State_Takeover
} MS_MON_PROC_STATE;
typedef enum {
    MS_Mon_ShutdownLevel_Undefined = -1,
    MS_Mon_ShutdownLevel_Normal    = 0,
    MS_Mon_ShutdownLevel_Immediate,
    MS_Mon_ShutdownLevel_Abrupt
} MS_MON_ShutdownLevel;
typedef enum {
    MS_Mon_ZoneType_Undefined   = 0x0000,   // No zone type defined
    MS_Mon_ZoneType_Edge        = 0x0001,   // Zone of service only nodes
    MS_Mon_ZoneType_Aggregation = 0x0002,   // Zone of compute only nodes
    MS_Mon_ZoneType_Storage     = 0x0004,   // Zone of storage only nodes
    MS_Mon_ZoneType_Excluded    = 0x0010,   // Excluded cores
    MS_Mon_ZoneType_Any = ( MS_Mon_ZoneType_Edge |
                            MS_Mon_ZoneType_Aggregation |
                            MS_Mon_ZoneType_Storage ),
    MS_Mon_ZoneType_Frontend = ( MS_Mon_ZoneType_Edge |
                                 MS_Mon_ZoneType_Aggregation ),
    MS_Mon_ZoneType_Backend = ( MS_Mon_ZoneType_Aggregation |
                                MS_Mon_ZoneType_Storage )
} MS_MON_ZoneType;
typedef struct MS_Mon_Monitor_Stats_Type {
    int acquired_max;
    int avail_min;
    int buf_misses;
} MS_Mon_Monitor_Stats_Type;
typedef struct MS_Mon_Open_Info_Type {
    int num_opens;
    struct {
        int  nid;
        int  pid;
        char process_name[MS_MON_MAX_PROCESS_NAME];
        char stale;
    } opens[MS_MON_MAX_OPEN_LIST];
    int truncated; // return_code - non-zero if list is 'truncated'
} MS_Mon_Open_Info_Type;
// this struct is not in monitor
typedef struct MS_Mon_Open_Info_Max_Type {
    int  nid;
    int  pid;
    char process_name[MS_MON_MAX_PROCESS_NAME];
    char stale;
} MS_Mon_Open_Info_Max_Type;
typedef struct MS_Mon_Node_Info_Entry_Type {
    int               nid;
    MS_MON_PROC_STATE state;
    MS_MON_ZoneType   type;
    int               processors;
    int               process_count;
    int               pnid;
    MS_MON_PROC_STATE pstate;
    bool              spare_node;
    unsigned int      memory_total;
    unsigned int      memory_free;
    unsigned int      swap_free;
    unsigned int      cache_free;
    unsigned int      memory_active;
    unsigned int      memory_inactive;
    unsigned int      memory_dirty;
    unsigned int      memory_writeback;
    unsigned int      memory_vm_alloc_used;
    long long         cpu_user;
    long long         cpu_nice;
    long long         cpu_system;
    long long         cpu_idle;
    long long         cpu_iowait;
    long long         cpu_irq;
    long long         cpu_soft_irq;
    int               btime;
    char              node_name[MS_MON_MAX_PROCESSOR_NAME];
    int               _fill;
} MS_Mon_Node_Info_Entry_Type;

typedef struct MS_Mon_Node_Info_Type {
    int num_nodes;
    int num_physical_nodes;
    int num_spares;
    int num_available_spares;
    int num_returned;
    struct {
        int               nid;
        MS_MON_PROC_STATE state;
        MS_MON_ZoneType   type;
        int               processors;
        int               process_count;
        int               pnid;
        MS_MON_PROC_STATE pstate;
        bool              spare_node;
        unsigned int      memory_total;
        unsigned int      memory_free;
        unsigned int      swap_free;
        unsigned int      cache_free;
        unsigned int      memory_active;
        unsigned int      memory_inactive;
        unsigned int      memory_dirty;
        unsigned int      memory_writeback;
        unsigned int      memory_vm_alloc_used;
        long long         cpu_user;
        long long         cpu_nice;
        long long         cpu_system;
        long long         cpu_idle;
        long long         cpu_iowait;
        long long         cpu_irq;
        long long         cpu_soft_irq;
        unsigned int      btime;
        char              node_name[MS_MON_MAX_PROCESSOR_NAME];
        int               _fill;
    } node[MS_MON_MAX_NODE_LIST];
    int _fill_1;
    int _fill_2;
    int _fill_3;
    bool _fill_4;
} MS_Mon_Node_Info_Type;
typedef struct MS_Mon_Process_Info_Type {
    int               nid;
    int               pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type     verifier;
#endif
    int               type;
    char              process_name[MS_MON_MAX_PROCESS_NAME];
    int               os_pid;
    int               priority;
    int               parent_nid;
    int               parent_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type     parent_verifier;
#endif
    char              parent_name[MS_MON_MAX_PROCESS_NAME];
    char              program[MS_MON_MAX_PROCESS_PATH];
    MS_MON_PROC_STATE state;
    char              event_messages;
    char              system_messages;
    char              paired;
    char              pending_delete;
    char              pending_replication;
    char              waiting_startup;
    char              opened;
    char              backup;
    char              unhooked;
    struct timespec   creation_time;
} MS_Mon_Process_Info_Type;
typedef struct MS_Mon_Zone_Info_Entry_Type {
    int                  nid;    // Logical Node's ID 
    int                  zid;    // Logical Node's Zone ID 
    int                  pnid;   // Node's Physical ID 
    MS_MON_PROC_STATE    pstate; // Physical Node's state
                                 // (i.e. UP, DOWN, STOPPING)
    char                 node_name[MS_MON_MAX_PROCESSOR_NAME]; // Node's name
} MS_Mon_Zone_Info_Entry_Type;
typedef struct MS_Mon_Zone_Info_Type {
    int num_nodes;                   // Number of logical nodes in the cluster
    int num_returned;                // Number of node entries returned
    struct
    {
        int                  nid;    // Logical Node's ID 
        int                  zid;    // Logical Node's Zone ID 
        int                  pnid;   // Node's Physical ID 
        MS_MON_PROC_STATE    pstate; // Physical Node's state
                                     // (i.e. UP, DOWN, STOPPING)
        char                 node_name[MS_MON_MAX_PROCESSOR_NAME]; // Node's name
    } node[MS_MON_MAX_NODE_LIST];
    int _fill_1;
    int _fill_2;
    int _fill_3;
    bool _fill_4;
} MS_Mon_Zone_Info_Type;
typedef enum {
    MS_Mon_ConfigType_Undefined = 0,
    MS_Mon_ConfigType_Cluster,
    MS_Mon_ConfigType_Node,
    MS_Mon_ConfigType_Process
} MS_Mon_ConfigType;
typedef struct MS_Mon_Reg_Get_Type {
    MS_Mon_ConfigType type;
    char              group[MS_MON_MAX_KEY_NAME];
    int               num_keys;
    int               num_returned;
    struct {
        char              key[MS_MON_MAX_KEY_NAME];
        char              value[MS_MON_MAX_VALUE_SIZE];
    } list[MS_MON_MAX_KEY_LIST];
} MS_Mon_Reg_Get_Type;
typedef struct MS_Mon_Trans_Info_Type {
    int num_processes; // contains actual number of procs below
    struct {
        int                 nid;
        int                 pid;
        MS_Mon_Transid_Type trans_id;
    } procs[MS_MON_MAX_PROC_LIST];
    int truncated; // return_code - non-zero if list is 'truncated'
} MS_Mon_Trans_Info_Type;
typedef struct MS_Mon_Zone_Info {
    int nid;                                // node id of requesting process
    int pid;                                // process id of requesting process
    int target_nid;                         // get zone information via node id (-1 for all)
    int target_zid;                         // get zone information via zone id (-1 for all)
    int last_nid;                           // Last Logical Node ID returned
    int last_pnid;                          // Last Physical Node ID returned
    bool _fill_1;                           
} MS_Mon_Zone_Info;
typedef struct MS_Mon_ClusterInstanceId {
    int nid;                                // node id of requesting process
    int pid;                                // process id of requesting process
} MS_Mon_ClusterInstanceId;

//
// Note the MS_Mon_MSGTYPE, MS_Mon_REQTYPE, MS_Mon_PROCESSTYPE, and MS_Mon_Msg
// are near clones from msgdef.h
//
typedef enum {
    MS_MsgType_Change = 1,
    MS_MsgType_Close,
    MS_MsgType_Event,
    MS_MsgType_NodeAdded,
    MS_MsgType_NodeChanged,
    MS_MsgType_NodeDeleted,
    MS_MsgType_NodeDown,
    MS_MsgType_NodeJoining,
    MS_MsgType_NodeQuiesce,
    MS_MsgType_NodeUp,
    MS_MsgType_Open,
    MS_MsgType_ProcessCreated,
    MS_MsgType_ProcessDeath,
    MS_MsgType_ReintegrationError,
    MS_MsgType_Service,
    MS_MsgType_Shutdown,
    MS_MsgType_SpareUp
} MS_Mon_MSGTYPE;
typedef enum {
    MS_ReqType_Close = 1,
    MS_ReqType_DeleteNs,
    MS_ReqType_Dump,
    MS_ReqType_Event,
    MS_ReqType_Exit,
    MS_ReqType_Get,
    MS_ReqType_InstanceId,
    MS_ReqType_Kill,
    MS_ReqType_MonStats,
    MS_ReqType_Mount,
    MS_ReqType_NameServerAdd,
    MS_ReqType_NameServerDelete,
    MS_ReqType_NameServerStart,
    MS_ReqType_NameServerStop,
    MS_ReqType_NewProcess,
    MS_ReqType_NewProcessNs,
    MS_ReqType_NodeAdd,
    MS_ReqType_NodeDelete,
    MS_ReqType_NodeDown,
    MS_ReqType_NodeInfo,
    MS_ReqType_NodeName,
    MS_ReqType_NodeUp,
    MS_ReqType_Notice,
    MS_ReqType_Notify,
    MS_ReqType_Open,
    MS_ReqType_OpenInfo,
    MS_ReqType_PersistAdd,
    MS_ReqType_PersistDelete,
    MS_ReqType_PNodeInfo,
    MS_ReqType_ProcessInfo,
    MS_ReqType_ProcessInfoCont,
    MS_ReqType_ProcessInfoNs,
    MS_ReqType_Set,
    MS_ReqType_Shutdown,
    MS_ReqType_ShutdownNs,
    MS_ReqType_Startup,
    MS_ReqType_TmLeader,
    MS_ReqType_TmReady,
    MS_ReqType_ZoneInfo
} MS_Mon_REQTYPE;
typedef enum {
    MS_ProcessType_Undefined = 0,
    MS_ProcessType_TSE,
    MS_ProcessType_DTM,
    MS_ProcessType_ASE,
    MS_ProcessType_Generic,
    MS_ProcessType_NameServer,
    MS_ProcessType_Watchdog,
    MS_ProcessType_AMP,
    MS_ProcessType_Backout,
    MS_ProcessType_VolumeRecovery,
    MS_ProcessType_MXOSRVR,
    MS_ProcessType_SPX,
    MS_ProcessType_SSMP,
    MS_ProcessType_PSD,
    MS_ProcessType_SMS,
    MS_ProcessType_TMID,
    MS_ProcessType_PERSIST
} MS_Mon_PROCESSTYPE;
struct MS_Mon_Change_def {
    MS_Mon_ConfigType type;
    char              group[MS_MON_MAX_KEY_NAME];
    char              key[MS_MON_MAX_KEY_NAME];
    char              value[MS_MON_MAX_VALUE_SIZE];
};
struct MS_Mon_Close_def {
    int           nid;
    int           pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type verifier;
#endif
    char          process_name[MS_MON_MAX_PROCESS_NAME];
    int           aborted;
    int           mon;
};
struct MS_Mon_NewProcess_Notice_def {
    int             nid;
    int             pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type   verifier;
#endif
    long long       tag;
    char            port[MS_MON_MAX_PORT_NAME];
    char            process_name[MS_MON_MAX_PROCESS_NAME];
    int             ferr; // return_code;
};

struct MS_Mon_NodeAdded_def
{
    int  nid;
    int  zid;
    char node_name[MS_MON_MAX_PROCESSOR_NAME];
};

struct MS_Mon_NodeChanged_def
{
    int  nid;
    int  zid;
    int  pnid;
    char node_name[MS_MON_MAX_PROCESSOR_NAME];
    int  first_core;
    int  last_core;
    int  processors;
    int  roles;
};

struct MS_Mon_NodeDeleted_def
{
    int  nid;
    int  zid;
    char node_name[MS_MON_MAX_PROCESSOR_NAME];
};

struct MS_Mon_NodeDown_def {
    int  nid;
    char node_name[MS_MON_MAX_PROCESSOR_NAME];
    int  takeover;
    char reason[MS_MON_MAX_REASON_TEXT];
};
struct MS_Mon_NodeJoining_def {
    int                  pnid;
    char                 node_name[MS_MON_MAX_PROCESSOR_NAME];
    MS_MON_JOINING_PHASE phase;
};
struct MS_Mon_NodeQuiesce_def {
    int  nid;
    char node_name[MS_MON_MAX_PROCESSOR_NAME];
};
struct MS_Mon_NodeUp_def {
    int  nid;
    char node_name[MS_MON_MAX_PROCESSOR_NAME];
    int  takeover;
};
struct MS_Mon_Open_def {
    int           nid;
    int           pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type verifier;
    char          process_name[MS_MON_MAX_PROCESS_NAME];
    int           target_nid;
    int           target_pid;
    SB_Verif_Type target_verifier;
#endif
    char          target_process_name[MS_MON_MAX_PROCESS_NAME];
    int           death_notification;
};
struct MS_Mon_ProcessDeath_def {
    int                 nid;
    int                 pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type       verifier;
#endif
    MS_Mon_Transid_Type transid;
    int                 aborted;
    char                process_name[MS_MON_MAX_PROCESS_NAME];
    MS_Mon_PROCESSTYPE  type;
};
struct MS_Mon_Shutdown_def {
    int                  nid;
    int                  pid;
    MS_MON_ShutdownLevel level;
};
struct MS_Mon_SpareUp_def {
    int  pnid;
    char node_name[MS_MON_MAX_PROCESSOR_NAME];
};
// TODO: make less kludgy
#if __WORDSIZE == 64
enum { _MS_REQ_FILL = 3 };
#else
enum { _MS_REQ_FILL = 1 };
#endif
typedef struct MS_Mon_Msg {
    MS_Mon_MSGTYPE type;
    char           _fill1[_MS_REQ_FILL]; // message_def.noreply
    int            _fill2;               // message_def.reply_tag
#if __WORDSIZE == 64
    int            _fill3;               // pad
#endif
    MS_Mon_REQTYPE reqtype;
    union {
        struct MS_Mon_Change_def              change;
        struct MS_Mon_Close_def               close;
        struct MS_Mon_ProcessDeath_def        death;
        struct MS_Mon_NodeDown_def            down;
        struct MS_Mon_NodeJoining_def         joining;
        struct MS_Mon_Open_def                open;
        struct MS_Mon_NewProcess_Notice_def   process_created;
        struct MS_Mon_NodeQuiesce_def         quiesce;
        struct MS_Mon_Shutdown_def            shutdown;
        struct MS_Mon_SpareUp_def             spare_up;
        struct MS_Mon_NodeUp_def              up;
        struct MS_Mon_NodeAdded_def           added;
        struct MS_Mon_NodeChanged_def         changed;
        struct MS_Mon_NodeDeleted_def         deleted;
    } u;
} MS_Mon_Msg;
typedef struct MS_Mon_Open_Comp_Type {
    int                ferr;    // error?
    const char        *name;     // name of opened process
    int                oid;      // open-id of opened process
    SB_Phandle_Type    phandle;  // phandle of opened process
    long long          tag;      // returned tag of open
} MS_Mon_Open_Comp_Type;
typedef void (*MS_Mon_Open_Process_Cb_Type)(struct MS_Mon_Open_Comp_Type *);
typedef void (*MS_Mon_Start_Process_Cb_Type)(SB_Phandle_Type *, struct MS_Mon_NewProcess_Notice_def *);

//
// add-on emulation functions
//
enum {
    MS_BUF_OPTION_COPY = 0x00000001,
    MS_BUF_OPTION_ALL  = 0x00000001
};
SB_Export short msg_buf_options(int options)
SB_DIAG_UNUSED;
SB_Export short msg_buf_read_ctrl(int     msgid,
                                  short **reqctrl,
                                  int    *bytecount,
                                  int     clear)
SB_DIAG_UNUSED;
SB_Export short msg_buf_read_data(int    msgid,
                                  char **buffer,
                                  int   *bytecount,
                                  int    clear)
SB_DIAG_UNUSED;
SB_Export short msg_buf_register(MS_Buf_Alloc_Cb_Type callback_alloc,
                                 MS_Buf_Free_Cb_Type  callback_free)
SB_DIAG_UNUSED;

//
// When you call this function, it will wait for 'fname' to exist.
// In the meantime, one can (debug) attach to the process
// and when the debugger is setup, one can create 'fname'
// and this function will exit.
//
// If 'fname' already exists, then this function will almost
// immediately exit.
//
SB_Export void msg_debug_hook(const char *who,
                              const char *fname);

//
// Call this to enable lim queue
//
SB_Export void msg_enable_lim_queue(MS_Lim_Queue_Cb_Type cb);

//
// Call this to enable open cleanup
//
SB_Export int msg_enable_open_cleanup();

//
// Call this to set priority queue
//
SB_Export void msg_enable_priority_queue();

//
// Call this to enable process-death indications on recv queue
//
SB_Export void msg_enable_recv_queue_proc_death();

//
// Called by SQL
//
SB_Export SB_Phandle_Type *msg_get_phandle(char *pname)
SB_DIAG_UNUSED;
SB_Export SB_Phandle_Type *msg_get_phandle(char *pname, int *fserr)
SB_DIAG_UNUSED;

//
// Called by SQL
//
SB_Export SB_Phandle_Type *msg_get_phandle_no_open(char *pname)
SB_DIAG_UNUSED;

//
// Get value from environment (only work after msg_init!)
//
SB_Export void        msg_getenv_bool(const char *key, bool *val);
SB_Export void        msg_getenv_int(const char *key, int *val);
SB_Export const char *msg_getenv_str(const char *key)
SB_DIAG_UNUSED;

//
// Call this to initialize - like calling MPI_Init
// Note: This WILL change the args, so that it looks like start from a prog
//
SB_Export int msg_init(int    *argc,
                       char ***argv)
SB_THROWS_FATAL SB_DIAG_UNUSED;

//
// Call this to initialize - like calling MPI_Init
// Note: args will NOT be changed (as in msg_init).
//
SB_Export int msg_init_attach(int    *argc,
                              char ***argv,
                              int     forkexec,
                              char   *name)
SB_THROWS_FATAL SB_DIAG_UNUSED;

//
// Call this to initialize - like calling MPI_Init
// Note: args will NOT be changed (as in msg_init).
// Will not display error message to stderr.
//
SB_Export int msg_init_attach_no_msg(int    *argc,
                                     char ***argv,
                                     int     forkexec,
                                     char   *name)
SB_THROWS_FATAL SB_DIAG_UNUSED;

//
// Call this to for non-SQ trace initialize
//
SB_Export void msg_init_trace();

//
// Call this to close process
//
// phandle: phandle of process
//
SB_Export int msg_mon_close_process(SB_Phandle_Type *phandle)
SB_DIAG_UNUSED;

#ifdef SQ_PHANDLE_VERIFIER
//
// Call this to create process name with seq #
//
SB_Export int msg_mon_create_name_seq(char          *name,
                                      SB_Verif_Type  verifier,
                                      char          *name_seq,
                                      int            name_seq_len)
SB_DIAG_UNUSED;
#endif

//
// Call this to deregister for death notifications
//
// target_nid/target_pid may be -1 to deregister all associated with target_transid.
//
SB_Export int msg_mon_deregister_death_notification(int                 target_nid,
                                                    int                 target_pid,
                                                    MS_Mon_Transid_Type target_transid)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

//
// Call this to deregister for death notifications (for notify_nid/notify_pid)
//
// target_nid/target_pid may be -1 to deregister all associated with target_transid.
//
SB_Export int msg_mon_deregister_death_notification2(int                 notify_nid,
                                                     int                 notify_pid,
                                                     int                 target_nid,
                                                     int                 target_pid,
                                                     MS_Mon_Transid_Type target_transid)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

#ifdef SQ_PHANDLE_VERIFIER
//
// Call this to deregister for death notifications (for notify_nid/notify_pid/notify_verifier)
//
// target_nid/target_pid may be -1 to deregister all associated with target_transid.
//
SB_Export int msg_mon_deregister_death_notification3(int                 notify_nid,
                                                     int                 notify_pid,
                                                     SB_Verif_Type       notify_verifier,
                                                     int                 target_nid,
                                                     int                 target_pid,
                                                     SB_Verif_Type       target_verifier,
                                                     MS_Mon_Transid_Type target_transid)
SB_DIAG_UNUSED;

//
// Call this to deregister for death notifications
//
// target_name may not be NULL!
//
SB_Export int msg_mon_deregister_death_notification_name(const char          *target_name,
                                                         MS_Mon_Transid_Type  target_transid)
SB_DIAG_UNUSED;

//
// Call this to deregister for death notifications (for notify_name)
//
// target_name may not be NULL!
//
SB_Export int msg_mon_deregister_death_notification_name2(const char          *notify_name,
                                                          const char          *target_name,
                                                          MS_Mon_Transid_Type  target_transid)
SB_DIAG_UNUSED;
#endif

//
// Call this to dump process by id
//
SB_Export int msg_mon_dump_process_id(const char *path,
                                      int         nid,
                                      int         pid,
                                      char       *core_file)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

//
// Call this to dump process by name
//
SB_Export int msg_mon_dump_process_name(const char *path,
                                        const char *name,
                                        char       *core_file)
SB_DIAG_UNUSED;

//
// Call this to enable monitor messages
//
SB_Export void msg_mon_enable_mon_messages(int enable_messages);

//
// Call this to send an event.
//
// nid/pid can be specified as -1.
// process_type must be a value in the enumeration MS_Mon_PROCESSTYPE.
// If event_len is 0, event_data will be ignored;
// otherwise, event_data will be expected to be non-NULL.
// The event data max size is MS_MON_MAX_SYNC_DATA.
//
SB_Export int msg_mon_event_send(int   nid,
                                 int   pid,
                                 int   process_type,
                                 int   event_id,
                                 int   event_len,
                                 char *event_data)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

#ifdef SQ_PHANDLE_VERIFIER
//
// Call this to send an event.
//
// name may not be NULL!
// To send to all nodes or processes in a node, use msg_mon_event_send
//
// process_type must be a value in the enumeration MS_Mon_PROCESSTYPE.
// If event_len is 0, event_data will be ignored;
// otherwise, event_data will be expected to be non-NULL.
// The event data max size is MS_MON_MAX_SYNC_DATA.
//
SB_Export int msg_mon_event_send_name(const char *name,
                                      int         process_type,
                                      int         event_id,
                                      int         event_len,
                                      char       *event_data)
SB_DIAG_UNUSED;
#endif

//
// Call this to wait for the event to occur.
// Note that this is a blocking call and that
// seabed will do nothing but wait for the event to occur.
//
// If event_len is not NULL, then the event data length will be returned.
// If event_data is not NULL, then the event data will be returned.
//
SB_Export int msg_mon_event_wait(int   event_id,
                                 int  *event_len,
                                 char *event_data)
SB_DIAG_UNUSED;

//
// Similar to msg_mon_event_wait() but will return the an event-id.
//
SB_Export int msg_mon_event_wait2(int  *event_id,
                                  int  *event_len,
                                  char *event_data)
SB_DIAG_UNUSED;

//
// Call this to get cluster id and instance id
//
SB_Export int msg_mon_get_instance_id(int *cluster_id,
                                      int *instance_id)
SB_DIAG_UNUSED;

//
// Call this to get monitor stats
//
SB_Export int msg_mon_get_monitor_stats(MS_Mon_Monitor_Stats_Type *stats)
SB_DIAG_UNUSED;

//
// Call this to get my info
//
SB_Export int msg_mon_get_my_info(int  *mon_nid,        // mon node-id
                                  int  *mon_pid,        // mon process-id
                                  char *mon_name,       // mon name
                                  int   mon_name_len,   // mon name-len
                                  int  *mon_ptype,      // mon process-type
                                  int  *mon_zid,        // mon zone-id
                                  int  *os_pid,         // os process-id
                                  long *os_tid)         // os thread-id
SB_DIAG_UNUSED;

//
// Call this to get my info
//
SB_Export int msg_mon_get_my_info2(int  *mon_nid,        // mon node-id
                                   int  *mon_pid,        // mon process-id
                                   char *mon_name,       // mon name
                                   int   mon_name_len,   // mon name-len
                                   int  *mon_ptype,      // mon process-type
                                   int  *mon_zid,        // mon zone-id
                                   int  *os_pid,         // os process-id
                                   long *os_tid,         // os thread-id
                                   int  *compid)         // component-id
SB_DIAG_UNUSED;

//
// Call this to get my info
//
SB_Export int msg_mon_get_my_info3(int  *mon_nid,        // mon node-id
                                   int  *mon_pid,        // mon process-id
                                   char *mon_name,       // mon name
                                   int   mon_name_len,   // mon name-len
                                   int  *mon_ptype,      // mon process-type
                                   int  *mon_zid,        // mon zone-id
                                   int  *os_pid,         // os process-id
                                   long *os_tid,         // os thread-id
                                   int  *compid,         // component-id
                                   int  *pnid)           // physical node-id
SB_DIAG_UNUSED;

#ifdef SQ_PHANDLE_VERIFIER
SB_Export int msg_mon_get_my_info4(int           *mon_nid,        // mon node-id
                                   int           *mon_pid,        // mon process-id
                                   char          *mon_name,       // mon name
                                   int            mon_name_len,   // mon name-len
                                   int           *mon_ptype,      // mon process-type
                                   int           *mon_zid,        // mon zone-id
                                   int           *os_pid,         // os process-id
                                   long          *os_tid,         // os thread-id
                                   int           *compid,         // component-id
                                   int           *pnid,           // physical node-id
                                   SB_Verif_Type *mon_verifier)   // mon process-verifier
SB_DIAG_UNUSED;
#endif

//
// Call this to get process name
//
SB_Export int msg_mon_get_my_process_name(char *name,
                                          int   len)
SB_DIAG_UNUSED;

//
// Call this to get segid
//
SB_Export int msg_mon_get_my_segid(int *segid)
SB_DIAG_UNUSED;

//
// Call this to get node info
//
// count: is number of node info array entries returned
// max:   is size of the info array
// info:  is array of info's
// if count would be greater than max, an error is returned
//
SB_Export int msg_mon_get_node_info(int                         *count,
                                    int                          max,
                                    MS_Mon_Node_Info_Entry_Type *info)
SB_DIAG_UNUSED;

//
// Call this to get node info
//
// count:                   is number of node info array entries returned
// max:                     is size of the info array
// info:                    is array of info's
// node_count:              is node-count
// pnode_count:             is pnode-count
// spares_count:            is spares-count
// available_spares_count:  is available-spares-count
// if count would be greater than max, an error is returned
//
SB_Export int msg_mon_get_node_info2(int                         *count,
                                     int                          max,
                                     MS_Mon_Node_Info_Entry_Type *info,
                                     int                         *node_count,
                                     int                         *pnode_count,
                                     int                         *spares_count,
                                     int                         *available_spares_count)
SB_DIAG_UNUSED;

//
// Call this to get node info for all nodes
//
// info is returned
//
SB_Export int msg_mon_get_node_info_all(MS_Mon_Node_Info_Type *info)
SB_DIAG_UNUSED;

//
// Call this to get node info
//
// nid:  to get info on
// info: is returned
//
SB_Export int msg_mon_get_node_info_detail(int                    nid,
                                           MS_Mon_Node_Info_Type *info)
SB_DIAG_UNUSED;

//
// Call this to get open info for a nid/pid or name.
//
// nid:    nid get info on (-1 if using name)
// pid:    pid get info on (-1 if using name)
// name:   name to get info on
// opened: true (opened) or false (opens)
// info:   returned info
//
// deprecated
//
SB_Export int msg_mon_get_open_info(int                    nid,
                                    int                    pid,
                                    char                  *name,
                                    int                    opened,
                                    MS_Mon_Open_Info_Type *info)
SB_DIAG_DEPRECATED;

//
// Call this to get open info for a nid/pid or name (max specified).
//
// nid:    nid get info on (-1 if using name)
// pid:    pid get info on (-1 if using name)
// name:   name to get info on
// opened: true (opened) or false (opens)
// count:  returned count of opens
// max:    max number of opens returned
// info:   returned info
//
// deprecated
//
SB_Export int msg_mon_get_open_info_max(int                        nid,
                                        int                        pid,
                                        char                      *name,
                                        int                        opened,
                                        int                       *count,
                                        int                        max,
                                        MS_Mon_Open_Info_Max_Type *info)
SB_DIAG_DEPRECATED;

//
// Call this to get process info
//
// name: process name to get info on
// nid:  is returned
// pid:  is returned
//
SB_Export int msg_mon_get_process_info(char *name,
                                       int  *nid,
                                       int  *pid)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

#ifdef SQ_PHANDLE_VERIFIER
//
// Call this to get process info
//
// name:     process name to get info on
// nid:      is returned
// pid:      is returned
// verifier: is returned
//
SB_Export int msg_mon_get_process_info2(char          *name,
                                        int           *nid,
                                        int           *pid,
                                        SB_Verif_Type *verifier)
SB_DIAG_UNUSED;
#endif

//
// Call this to get process info
//
// name: process name to get info on
// info: is returned
//
SB_Export int msg_mon_get_process_info_detail(char                     *name,
                                              MS_Mon_Process_Info_Type *info)
SB_DIAG_UNUSED;

//
// Call this to get process info for a process-type
//
// type:  process type to get info on
// count: is number of process's returned
// max:   is size of the info array
// info:  is array of info's
// if count would be greater than max, an error is returned
//
SB_Export int msg_mon_get_process_info_type(int                       ptype,
                                            int                      *count,
                                            int                       max,
                                            MS_Mon_Process_Info_Type *info)
SB_DIAG_UNUSED;

//
// Call this to get process name
//
// nid:  is nid
// pid:  is pid
// name: is returned name
//
SB_Export int msg_mon_get_process_name(int   nid,
                                       int   pid,
                                       char *name)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

#ifdef SQ_PHANDLE_VERIFIER
//
// Call this to get process name
//
// nid:      is nid
// pid:      is pid
// verifier: is verifier
// name:     is returned name
//
SB_Export int msg_mon_get_process_name2(int            nid,
                                        int            pid,
                                        SB_Verif_Type  verifier,
                                        char          *name)
SB_DIAG_UNUSED;
#endif

//
// Call this to get trans info for process
//
SB_Export int msg_mon_get_trans_info_process(char                   *name,
                                             MS_Mon_Trans_Info_Type *info)
SB_DIAG_UNUSED;

//
// Call this to get trans info for transid
//
SB_Export int msg_mon_get_trans_info_transid(MS_Mon_Transid_Type     transid,
                                             MS_Mon_Trans_Info_Type *info)
SB_DIAG_UNUSED;

//
// Call this to get zone info
//
// count: is number of zone info array entries returned
// max:   is size of the info array
// info:  is array of info's
// if count would be greater than max, an error is returned
//
SB_Export int msg_mon_get_zone_info(int                         *count,
                                    int                          max,
                                    MS_Mon_Zone_Info_Entry_Type *info)
SB_DIAG_UNUSED;

//
// Call this to get zone info
//
// nid:  to get info on a node id
// zid:  to get info on a zone id
// info: is returned
//
SB_Export int msg_mon_get_zone_info_detail(int                    nid,
                                           int                    zid, 
                                           MS_Mon_Zone_Info_Type *info)
SB_DIAG_UNUSED;

//
// Call this to mount device.
//
SB_Export int msg_mon_mount_device()
SB_DIAG_UNUSED;

SB_Export int msg_mon_mount_device2(MS_MON_DEVICE_STATE *primary,
                                    MS_MON_DEVICE_STATE *mirror)
SB_DIAG_UNUSED;

//
// Call this to down node.
//
SB_Export int msg_mon_node_down(int nid)
SB_DIAG_UNUSED;

#ifdef SQ_PHANDLE_VERIFIER
//
// Call this to down node (with reason).
//
SB_Export int msg_mon_node_down2(int nid, const char *reason)
SB_DIAG_UNUSED;
#endif

//
// Call this to up node.
//
SB_Export int msg_mon_node_up(int nid)
SB_DIAG_UNUSED;

//
// Call this to open process
//
SB_Export int msg_mon_open_process(char            *name,
                                   SB_Phandle_Type *phandle,
                                   int             *oid)
SB_DIAG_UNUSED;

//
// Call this to open process (backup)
//
SB_Export int msg_mon_open_process_backup(char            *name,
                                          SB_Phandle_Type *phandle,
                                          int             *oid)
SB_DIAG_UNUSED;

//
// Call this to open process (interceptor)
//
SB_Export int msg_mon_open_process_ic(char            *name,
                                      SB_Phandle_Type *phandle,
                                      int             *oid)
SB_DIAG_UNUSED;

//
// Call this to open process (nowait with callback)
//
SB_Export int msg_mon_open_process_nowait_cb(char                        *name,
                                             SB_Phandle_Type             *phandle,
                                             MS_Mon_Open_Process_Cb_Type  callback,
                                             long long                    tag,
                                             int                         *done,
                                             int                         *oid)
SB_DIAG_UNUSED;

//
// Call this to open process (self)
//
SB_Export int msg_mon_open_process_self(SB_Phandle_Type *phandle,
                                        int             *oid)
SB_DIAG_UNUSED;

//
// Call this to open process (self-interceptor)
//
SB_Export int msg_mon_open_process_self_ic(SB_Phandle_Type *phandle,
                                           int             *oid)
SB_DIAG_UNUSED;

//
// Call this to handle process close
//
SB_Export int msg_mon_process_close()
SB_DIAG_UNUSED;

//
// Call this to handle process startup
//
// sysmsgs: want system messages?
//
SB_Export int msg_mon_process_startup(int sysmsgs)
SB_THROWS_FATAL SB_DIAG_UNUSED;

//
// Call this to handle process startup
//
// sysmsgs: want system messages?
// eventmsgs: want event messages?
//
SB_Export int msg_mon_process_startup2(int sysmsgs, int eventmsgs)
SB_THROWS_FATAL SB_DIAG_UNUSED;

//
// Call this to handle process startup
//
// sysmsgs: want system messages?
// pipeio: want pipe io?
//
SB_Export int msg_mon_process_startup3(int sysmsgs, int pipeio, bool remap_stderr=true)
SB_THROWS_FATAL SB_DIAG_UNUSED;

//
// Call this to handle process startup
//
// sysmsgs: want system messages?
// pipeio: want pipe io?
// altsig: alternate sig?
//
SB_Export int msg_mon_process_startup4(int sysmsgs, int pipeio, int altsig)
SB_THROWS_FATAL SB_DIAG_UNUSED;

//
// Call this to process shutdown
//
SB_Export int msg_mon_process_shutdown()
SB_DIAG_UNUSED;

//
// Call this to process shutdown (fast)
//
SB_Export int msg_mon_process_shutdown_fast();

//
// Call this to process shutdown (now)
//
SB_Export void msg_mon_process_shutdown_now();

//
// Call this to get registry
//
SB_Export int msg_mon_reg_get(MS_Mon_ConfigType    config_type,
                              int                  next,
                              char                *group,
                              char                *key,
                              MS_Mon_Reg_Get_Type *info)
SB_DIAG_UNUSED;

//
// Call this to set registry
//
SB_Export int msg_mon_reg_set(MS_Mon_ConfigType   config_type,
                              char               *group,
                              char               *key,
                              char               *value)
SB_DIAG_UNUSED;

//
// Call this to register for death notifications
//
SB_Export int msg_mon_register_death_notification(int target_nid,
                                                  int target_pid)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

//
// Call this to register for death notifications (to notify_nid/notify_pid)
//
SB_Export int msg_mon_register_death_notification2(int notify_nid,
                                                   int notify_pid,
                                                   int target_nid,
                                                   int target_pid)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

//
// Call this to register for death notifications
//
SB_Export int msg_mon_register_death_notification3(int target_nid,
                                                   int target_pid)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

#ifdef SQ_PHANDLE_VERIFIER
//
// Call this to register for death notifications (internal use)
//
SB_Export int msg_mon_register_death_notification4(int           target_nid,
                                                   int           target_pid,
                                                   SB_Verif_Type target_verifier)
SB_DIAG_UNUSED;

//
// Call this to register for death notifications
//
// target_name may not be NULL!
//
SB_Export int msg_mon_register_death_notification_name(const char *target_name)
SB_DIAG_UNUSED;

//
// Call this to register for death notifications (to notify_name)
//
// notify_name may not be NULL!
// target_name may not be NULL!
//
SB_Export int msg_mon_register_death_notification_name2(const char *notify_name,
                                                        const char *target_name)
SB_DIAG_UNUSED;
#endif

//
// Call this to re-open process (not normally called by application)
//
SB_Export int msg_mon_reopen_process(SB_Phandle_Type *phandle)
SB_DIAG_UNUSED;

//
// Call this to shutdown
//
SB_Export int msg_mon_shutdown(int level)
SB_DIAG_UNUSED;

//
// Call this to start a process
//
// prog:     name of program
// name:     name of process (e.g. $serv) you want to call it
// ret_name: returned name of process (if not NULL)
// argc:     argc for process
// argv:     argv for process
// phandle:  phandle of server (output)
// open:     open it after it's started?
// oid:      if open then open id is returned here
// ptype:    type of process
// priority: priority
// debug:    debug
// backup:   backup
// nid:      if nid is not-NULL, specified nid and nid is returned here
// pid:      returned pid
// infile:   input file
// outfile:  output file
// verifier: verifier
//
SB_Export int msg_mon_start_process(char             *prog,
                                    char             *name,
                                    char             *ret_name,
                                    int               argc,
                                    char            **argv,
                                    SB_Phandle_Type  *phandle,
                                    int               open,
                                    int              *oid,
                                    int               ptype,
                                    int               priority,
                                    int               debug,
                                    int               backup,
                                    int              *nid,
                                    int              *pid,
                                    char             *infile,
                                    char             *outfile
#ifdef SQ_PHANDLE_VERIFIER
                                   ,SB_Verif_Type    *verifier = NULL
#endif
                                   )
SB_DIAG_UNUSED;

//
// Call this to start a process
// (same as msg_mon_start_process plus unhooked)
//
// unhooked:  if false, parent process death will trigger child exits
//
SB_Export int msg_mon_start_process2(char             *prog,
                                     char             *name,
                                     char             *ret_name,
                                     int               argc,
                                     char            **argv,
                                     SB_Phandle_Type  *phandle,
                                     int               open,
                                     int              *oid,
                                     int               ptype,
                                     int               priority,
                                     int               debug,
                                     int               backup,
                                     int              *nid,
                                     int              *pid,
                                     char             *infile,
                                     char             *outfile,
                                     int               unhooked
#ifdef SQ_PHANDLE_VERIFIER
                                    ,SB_Verif_Type    *verifier = NULL
#endif
                                    )
SB_DIAG_UNUSED;

//
// Call this to start a process (nowait)
//
// prog:     name of program
// name:     name of process (e.g. $serv) you want to call it
// argc:     argc for process
// argv:     argv for process
// ptype:    type of process
// priority: priority
// debug:    debug
// backup:   backup
// tag:      tag
// nid:      nid
// infile:   input file
// outfile:  output file
// verifier: verifier
//
SB_Export int msg_mon_start_process_nowait(char             *prog,
                                           char             *name,
                                           char             *ret_name,
                                           int               argc,
                                           char            **argv,
                                           SB_Phandle_Type  *phandle,
                                           int               ptype,
                                           int               priority,
                                           int               debug,
                                           int               backup,
                                           long long         tag,
                                           int              *nid,
                                           int              *pid,
                                           char             *infile,
                                           char             *outfile
#ifdef SQ_PHANDLE_VERIFIER
                                          ,SB_Verif_Type    *verifier = NULL
#endif
                                          )
SB_DIAG_UNUSED;

//
// Call this to start a process (nowait)
// (same as msg_mon_start_process_nowait plus unhooked)
//
SB_Export int msg_mon_start_process_nowait2(char             *prog,
                                            char             *name,
                                            char             *ret_name,
                                            int               argc,
                                            char            **argv,
                                            SB_Phandle_Type  *phandle,
                                            int               ptype,
                                            int               priority,
                                            int               debug,
                                            int               backup,
                                            long long         tag,
                                            int              *nid,
                                            int              *pid,
                                            char             *infile,
                                            char             *outfile,
                                            int               unhooked
#ifdef SQ_PHANDLE_VERIFIER
                                           ,SB_Verif_Type    *verifier = NULL
#endif
                                           )
SB_DIAG_UNUSED;

//
// Call this to start a process (nowait-callback)
//
// callback: callback
// prog:     name of program
// name:     name of process (e.g. $serv) you want to call it
// argc:     argc for process
// argv:     argv for process
// ptype:    type of process
// priority: priority
// debug:    debug
// backup:   backup
// tag:      tag
// nid:      nid
// infile:   input file
// outfile:  output file
// verifier: verifier
//
SB_Export int msg_mon_start_process_nowait_cb(MS_Mon_Start_Process_Cb_Type  callback,
                                              char                         *prog,
                                              char                         *name,
                                              char                         *ret_name,
                                              int                           argc,
                                              char                        **argv,
                                              int                           ptype,
                                              int                           priority,
                                              int                           debug,
                                              int                           backup,
                                              long long                     tag,
                                              int                          *nid,
                                              int                          *pid,
                                              char                         *infile,
                                              char                         *outfile
#ifdef SQ_PHANDLE_VERIFIER
                                             ,SB_Verif_Type                 *verifier = NULL
#endif
                                             )
SB_DIAG_UNUSED;

//
// Call this to start a process (nowait-callback)
// (same as msg_mon_start_process_nowait_cb plus unhooked)
//
SB_Export int msg_mon_start_process_nowait_cb2(MS_Mon_Start_Process_Cb_Type  callback,
                                               char                         *prog,
                                               char                         *name,
                                               char                         *ret_name,
                                               int                           argc,
                                               char                        **argv,
                                               int                           ptype,
                                               int                           priority,
                                               int                           debug,
                                               int                           backup,
                                               long long                     tag,
                                               int                          *nid,
                                               int                          *pid,
                                               char                         *infile,
                                               char                         *outfile,
                                               int                           unhooked
#ifdef SQ_PHANDLE_VERIFIER
                                              ,SB_Verif_Type                 *verifier = NULL
#endif
                                              )
SB_DIAG_UNUSED;

//
// Call this to stop process
//
SB_Export int msg_mon_stop_process(char *name,
                                   int   nid,
                                   int   pid)
SB_DIAG_UNUSED; // TODO: ->SB_DIAG_DEPRECATED

#ifdef SQ_PHANDLE_VERIFIER
//
// Call this to stop process
//
// target_name may not be NULL!
//
SB_Export int msg_mon_stop_process_name(const char *name)
SB_DIAG_UNUSED;
#endif

#ifdef SQ_STFSD
//
// Call this to send data to the stfsd
//
// The data max size is MS_MON_MAX_STFSD_DATA.
// The len specifies the length of the data.
//
SB_Export int msg_mon_stfsd_send(void *data,
                                 int   len,
                                 void *rsp_data,
                                 int  *output_len,
                                 int   tag)
SB_DIAG_UNUSED;
#endif

//
// Call this to get text for config type
//
SB_Export const char *msg_mon_text_get_mon_config_type(MS_Mon_ConfigType ct)
SB_DIAG_UNUSED;

//
// Call this to get text for device state
//
SB_Export const char *msg_mon_text_get_mon_device_state(MS_MON_DEVICE_STATE state)
SB_DIAG_UNUSED;

//
// Call this to get text for msg type
//
SB_Export const char *msg_mon_text_get_mon_msg_type(MS_Mon_MSGTYPE mt)
SB_DIAG_UNUSED;

//
// Call this to get text for proc state
//
SB_Export const char *msg_mon_text_get_mon_proc_state(MS_MON_PROC_STATE state)
SB_DIAG_UNUSED;

//
// Call this to get text for process type
//
SB_Export const char *msg_mon_text_get_mon_process_type(MS_Mon_PROCESSTYPE pt)
SB_DIAG_UNUSED;

//
// Call this to get text for req type
//
SB_Export const char *msg_mon_text_get_mon_req_type(MS_Mon_REQTYPE rt)
SB_DIAG_UNUSED;

//
// Call this to get text for shutdown level
//
SB_Export const char *msg_mon_text_get_mon_shutdown_level(MS_MON_ShutdownLevel sl)
SB_DIAG_UNUSED;

//
// Call this to get text for zone type
//
SB_Export const char *msg_mon_text_get_mon_zone_type(MS_MON_ZoneType zt)
SB_DIAG_UNUSED;

//
// Call this to set TM leader
//
SB_Export int msg_mon_tm_leader_set(int *nid, int *pid, char *name)
SB_DIAG_UNUSED;

//
// Call this to set TM ready for transactions
//
SB_Export int msg_mon_tm_ready(void)
SB_DIAG_UNUSED;

//
// Call this to issue tmsync.
//
// The sync data max size is MS_MON_MAX_SYNC_DATA.
// The len specifies the length of the data.
// The handle is returned to cordinate with commit or abort notice
// The tag will be returned at abort/commit time.
// Returns: zero, for ok; nozero, otherwise.
//
SB_Export int msg_mon_tmsync_issue(void *data,
                                   int   len,
                                   int  *handle,
                                   int   tag)
SB_DIAG_UNUSED;

//
// Call this to register tmsync.
//
// The callback will be called when a tmsync message arrives.
// The callback is called with the sync data (max size MS_MON_MAX_SYNC_DATA).
// The callback should return zero to accept, return non-zero to reject.
//
SB_Export int msg_mon_tmsync_register(MS_Mon_TmSync_Cb_Type callback)
SB_DIAG_UNUSED;

//
// Call this to register for trace-change callback.
//
// The callback will be called when a trace change message arrives.
//
SB_Export int msg_mon_trace_register_change(MS_Mon_Trace_Cb_Type callback)
SB_DIAG_UNUSED;

//
// Call this to delist trans
//
SB_Export int msg_mon_trans_delist(int                 tm_nid,
                                   int                 tm_pid,
                                   MS_Mon_Transid_Type transid)
SB_DIAG_UNUSED;

//
// Call this to end trans
//
SB_Export int msg_mon_trans_end(int                 tm_nid,
                                int                 tm_pid,
                                MS_Mon_Transid_Type transid)
SB_DIAG_UNUSED;

//
// Call this to enlist trans
//
SB_Export int msg_mon_trans_enlist(int                 tm_nid,
                                   int                 tm_pid,
                                   MS_Mon_Transid_Type transid)
SB_DIAG_UNUSED;

//
// Call this to register tmlib.
//
// The callback will be called at various times during tx processing.
//
SB_Export int msg_mon_trans_register_tmlib(MS_Mon_Tmlib_Cb_Type callback)
SB_DIAG_UNUSED;

//
// Call this to register tmlib2.
//
// The callback will be called at various times during tx processing.
// The user can only use either msg_mon_trans_register_tmlib OR
// msg_mon_trans_register_tmlib2.
//
SB_Export int msg_mon_trans_register_tmlib2(MS_Mon_Tmlib2_Cb_Type callback)
SB_DIAG_UNUSED;

//
// Called by SQL
//
SB_Export short msg_set_phandle(char            *pname,
                                SB_Phandle_Type *phandle)
SB_DIAG_UNUSED;

//
// Call this to disable assert (test)
//
SB_Export int msg_test_assert_disable()
SB_DIAG_UNUSED;

//
// Call this to enable assert (test)
//
SB_Export void msg_test_assert_enable(int state);

//
// For testing
//
SB_Export void msg_test_disable_wait(int disable_wait);

//
// Call this to enable client-only (test)
//
SB_Export int msg_test_enable_client_only()
SB_DIAG_UNUSED;

//
// For testing
//
SB_Export int msg_test_init(int    *argc,
                            char ***argv,
                            int     mpi_init)
SB_DIAG_UNUSED;

//
// Call this to close openers (test)
//
SB_Export void msg_test_openers_close();

//
// Call this to delete openers (test)
//
SB_Export void msg_test_openers_del();

//
// For testing
//
SB_Export void msg_test_set_md_count(int count);

//
// emulation functions
//
SB_Export short BMSG_ABANDON_(int              msgid)
SB_DIAG_UNUSED;
SB_Export short BMSG_AWAIT_(int                msgid,
                            int                tov)
SB_DIAG_UNUSED;
SB_Export short BMSG_BREAK_(int                msgid,
                            short             *results,
                            SB_Phandle_Type   *phandle)
SB_DIAG_UNUSED;
SB_Export void  BMSG_BREAK2_(int                msgid,
                             short             *results,
                             SB_Phandle_Type   *phandle);
SB_Export short BMSG_GETREQINFO_(int           itemcode,
                                 int           msgid,
                                 int          *item)
SB_DIAG_UNUSED;
SB_Export void  BMSG_HOLD_(int                 msgid);
SB_Export short BMSG_ISCANCELED_(int           msgid)
SB_DIAG_UNUSED;
SB_Export short BMSG_ISDONE_(int               msgid)
SB_DIAG_UNUSED;
SB_Export short BMSG_LINK_(SB_Phandle_Type    *phandle,
                           int                *msgid,
                           short              *reqctrl,
                           int                 reqctrlsize,
                           short              *replyctrl,
                           int                 replyctrlmax,
                           char               *reqdata,
                           int                 reqdatasize,
                           char               *replydata,
                           int                 replydatamax,
                           long                linkertag,
                           short               pri,
                           short               xmitclass,
                           short               linkopts)
SB_DIAG_UNUSED;
SB_Export short BMSG_LINK2_(SB_Phandle_Type    *phandle,
                            int                *msgid,
                            short              *reqctrl,
                            int                 reqctrlsize,
                            short              *replyctrl,
                            int                 replyctrlmax,
                            char               *reqdata,
                            int                 reqdatasize,
                            char               *replydata,
                            int                 replydatamax,
                            long                linkertag,
                            short               pri,
                            short               xmitclass,
                            short               linkopts)
SB_DIAG_UNUSED;
SB_Export short BMSG_LISTEN_(short            *sre,
                             short             listenopts,
                             long              listenertag)
SB_DIAG_UNUSED;
SB_Export short BMSG_READCTRL_(int             msgid,
                               short          *reqctrl,
                               int             bytecount)
SB_DIAG_UNUSED;
SB_Export short BMSG_READDATA_(int             msgid,
                               char           *buffer,
                               int             bytecount)
SB_DIAG_UNUSED;
SB_Export void  BMSG_RELEASEALLHELD_();
SB_Export void  BMSG_REPLY_(int                msgid,
                            short             *replyctrl,
                            int                replyctrlsize,
                            char              *replydata,
                            int                replydatasize,
                            short              errorclass,
                            SB_Phandle_Type   *newphandle);
SB_Export short BMSG_SETTAG_(int               msgid,
                             long              tag)
SB_DIAG_UNUSED;

SB_Export short XMSG_ABANDON_(int              msgid)
SB_DIAG_UNUSED;
SB_Export short XMSG_AWAIT_(int                msgid,
                            int                tov)
SB_DIAG_UNUSED;
SB_Export short XMSG_BREAK_(int                msgid,
                            short             *results,
                            SB_Phandle_Type   *phandle)
SB_DIAG_UNUSED;
SB_Export void  XMSG_BREAK2_(int                msgid,
                             short             *results,
                             SB_Phandle_Type   *phandle);
SB_Export short XMSG_GETREQINFO_(int           itemcode,
                                 int           msgid,
                                 int          *item)
SB_DIAG_UNUSED;
SB_Export void  XMSG_HOLD_(int                 msgid);
SB_Export short XMSG_ISCANCELED_(int           msgid)
SB_DIAG_UNUSED;
SB_Export short XMSG_ISDONE_(int               msgid)
SB_DIAG_UNUSED;
SB_Export short XMSG_LINK_(SB_Phandle_Type    *phandle,
                           int                *msgid,
                           short              *reqctrl,
                           ushort              reqctrlsize,
                           short              *replyctrl,
                           ushort              replyctrlmax,
                           char               *reqdata,
                           ushort              reqdatasize,
                           char               *replydata,
                           ushort              replydatamax,
                           long                linkertag,
                           short               pri,
                           short               xmitclass,
                           short               linkopts)
SB_DIAG_UNUSED;
SB_Export short XMSG_LINK2_(SB_Phandle_Type    *phandle,
                            int                *msgid,
                            short              *reqctrl,
                            ushort              reqctrlsize,
                            short              *replyctrl,
                            ushort              replyctrlmax,
                            char               *reqdata,
                            ushort              reqdatasize,
                            char               *replydata,
                            ushort              replydatamax,
                            long                linkertag,
                            short               pri,
                            short               xmitclass,
                            short               linkopts)
SB_DIAG_UNUSED;
SB_Export short XMSG_LISTEN_(short            *sre,
                             short             listenopts,
                             long              listenertag)
SB_DIAG_UNUSED;
SB_Export short XMSG_READCTRL_(int             msgid,
                               short          *reqctrl,
                               ushort          bytecount)
SB_DIAG_UNUSED;
SB_Export short XMSG_READDATA_(int             msgid,
                               char           *buffer,
                               ushort          bytecount)
SB_DIAG_UNUSED;
SB_Export void  XMSG_RELEASEALLHELD_();
SB_Export void  XMSG_REPLY_(int                msgid,
                            short             *replyctrl,
                            ushort             replyctrlsize,
                            char              *replydata,
                            ushort             replydatasize,
                            short              errorclass,
                            SB_Phandle_Type   *newphandle);
SB_Export short XMSG_SETTAG_(int               msgid,
                             long              tag)
SB_DIAG_UNUSED;

SB_Export short XCONTROLMESSAGESYSTEM(short    actioncode,
                                      short    value)
SB_DIAG_UNUSED;

SB_Export short XMESSAGESYSTEMINFO(short       itemcode,
                                   short      *value)
SB_DIAG_UNUSED;


typedef struct MS_Result_Type {
    unsigned int   rr_ctrlsize;
    unsigned int   rr_datasize;
    unsigned       rrerr_mserrb:1;        //.<09> = err set by msgsys or net
    unsigned       rrerr_datareceivedb:1; //.<10> = reply data may hv bn rcvd
    unsigned       rrerr_updatedestb:1;   //.<11> = phandle has been updated
    unsigned       rrerr_countitb:1;      //.<12> = should be counted
    unsigned       rrerr_startedb:1;      //.<13> = may have been acted on
    unsigned       rrerr_retryableb:1;    //.<14> = retryable path error
    unsigned       rrerr_errorb:1;
    unsigned       _filler:25;
} MS_Result_Type;

typedef struct BMS_SRE {
    int    sre_msgId;
    int    sre_flags;
    int    sre_pri;
    int    sre_reqCtrlSize;
    int    sre_reqDataSize;
    int    sre_replyCtrlMax;
    int    sre_replyDataMax;
} BMS_SRE;

typedef struct MS_SRE {
    int    sre_msgId;
    ushort sre_flags;
    ushort sre_pri;
    ushort sre_reqCtrlSize;
    ushort sre_reqDataSize;
    ushort sre_replyCtrlMax;
    ushort sre_replyDataMax;
} MS_SRE;

typedef struct MS_SRE_TPOP {
    int         sre_tleId;
    int         sre_tleTOVal;
    int         sre_tleType;
    short       sre_tleParm1;
    long        sre_tleParm2;
} MS_SRE_TPOP;
typedef MS_SRE_TPOP BMS_SRE_TPOP;

typedef struct MS_SRE_LDONE {
    int         sre_msgId;
    SB_Tag_Type sre_linkTag;
} MS_SRE_LDONE;
typedef MS_SRE_LDONE BMS_SRE_LDONE;

typedef struct MS_SRE_ABANDON {
    int         sre_msgId;
    SB_Tag_Type sre_servTag;
} MS_SRE_ABANDON;
typedef MS_SRE_ABANDON BMS_SRE_ABANDON;

// item codes for XMSG_GETREQINFO
enum {
    MSGINFO_NID   = 1,
    MSGINFO_PID   = 2,
    MSGINFO_PTYPE = 3
};

// max limits for XCONTROLMESSAGESYSTEM
enum {
    XMAX_SETTABLE_RECVLIMIT     = 4095,
    XMAX_SETTABLE_SENDLIMIT     = 4095
};
enum {
    XMAX_SETTABLE_RECVLIMIT_H   = 16383,
    XMAX_SETTABLE_SENDLIMIT_H   = 16383
};
enum {
    XMAX_SETTABLE_RECVLIMIT_TM  = 25700,
    XMAX_SETTABLE_SENDLIMIT_TM  = 25700
};

// item codes for XCONTROLMESSAGESYSTEM
enum {
    XCTLMSGSYS_SETRECVLIMIT     = 0,
    XCTLMSGSYS_SETSENDLIMIT     = 1
};

// item codes for XMESSAGESYSTEMINFO
enum {
    XMSGSYSINFO_RECVLIMIT       = 0,
    XMSGSYSINFO_SENDLIMIT       = 1,
    XMSGSYSINFO_RECVUSE         = 4,
    XMSGSYSINFO_SENDUSE         = 5,
    XMSGSYSINFO_RECVSIZE        = 100,
    XMSGSYSINFO_RECVLIMSIZE     = 101
};

#endif // !__SB_MS_H_
