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

#ifndef MSGDEF_H_
#define MSGDEF_H_

#include <mpi.h>

// HP_MPI supports both 32bit and 64bit modes

// Compile options
//#define DEBUGGING
#define USE_ONE_SYNC_RECV_BUFFER
#define NO_OPEN_CLOSE_NOTICES
//#define USE_MEMORY_LOADBALANCING

#define VERSION "Monitor v0.12 2-17-2009" 

#define SERVICE_TAG      1
#define INTERNAL_TAG     2
#define NOTICE_TAG       3
#define SYNC_TAG         4
#define CHKPNT_TAG       5
#define EVENT_TAG        6
#define UNSOLICITED_TAG  7
#define REPLY_TAG        8
#define CLOSE_TAG        9
#define WAKE_TAG         10

#define MON_BASE_NICE    0
#define TSE_BASE_NICE    1
#define DTM_BASE_NICE    1
#define APP_BASE_NICE    1

#define BCAST_PID -255

#define WATCHDOG_TICKS   250
#define MAX_ARGS         60
#define MAX_ARG_SIZE     256

#define MAX_CORES        256   // Current logic supports Linux limit of 1024
                               // NOTE: Increase with caution as this number
                               //  is also used to gather local CPU statistics
                               //  and a large number may degrade performance
#define MAX_NODES        256   // This can be higher when needed and will
                               // have performance implications
                               // Increment by 64 to match node state bitmask
#define MAX_LNODES       (MAX_NODES*4)  // The 4 is a per physical node limit
#define MAX_NODE_BITMASK  64   // A 64 bit mask, each bit represent pnid state
                               // 0 = down, 1 = up
#define MAX_NODE_MASKS   (MAX_NODES/MAX_NODE_BITMASK) // Node bit mask array size

#define MAX_FAULT_ZONES  16 
#define MAX_FILE_NAME    256
#define MAX_KEY_NAME     64
#define MAX_KEY_LIST     32
#define MAX_NODE_LIST    64
#define MAX_OPEN_LIST    256
#define MAX_OPEN_CONTEXT 5
#define MAX_PID_VALUE    0x00FFD1C9
#define MAX_PRIMITIVES   1    // SQWatchog (WDG) is last to exit on shutdown
#define MAX_PROC_LIST    256
#define MAX_PROCINFO_LIST 64
#define MAX_PROC_CONTEXT 5
#define MAX_PROCESS_NAME MAX_KEY_NAME
#define MAX_PROCESS_NAME_STR 12 
#define MAX_PROCESS_PATH 256
#define MAX_PROCESSOR_NAME 128
#define MAX_REASON_TEXT  256
#define MAX_ROLEBUF_SIZE 84
#define MAX_SEARCH_PATH  BUFSIZ
#define MAX_SEQ_VALUE    0x7FFFFFFF
#define MAX_STFSD_DATA   32767
#define MAX_SYNC_DATA    4096
#define MAX_SYNC_SIZE    65536
#define MAX_TM_SYNCS     200
#define MAX_TM_HANDLES   0x00FFFFFF
#define MAX_VALUE_SIZE   512
#define MAX_VALUE_SIZE_INT 4096

// Use STRCPY when the size of the source string is variable and unknown.
// Safe strcpy - checks that destination has enough capacity to hold
// source string.  If not, source string is truncated.
#define STRCPY(dest,src) \
{ \
    unsigned int dlen = (unsigned int) (sizeof(dest) - 1); \
    if (strlen(src) <= dlen) \
    { \
        strcpy(dest, src); \
    } \
    else \
    { \
        memcpy(dest, src, dlen); \
        dest[dlen] = '\0'; \
    } \
}

typedef int Verifier_t;                  // Process verifier typedef

typedef struct { int nid; int id; } strId_t;

typedef long long _TM_Native_Type;          // Native Data Type for Transaction ID

typedef union {                             // External Extended Transaction ID
    _TM_Native_Type txid[4];
} _TM_Txid_External;

typedef enum {
    SMS_Undefined=1100,     // Invalid
    SMS_Exit                // Monitor Event to exit the SMService process 
} SMServiceEvent_t;

typedef enum {
    Watchdog_Start=1000,    // Monitor Event to start the Watchdog Process timer
    Watchdog_Refresh,       // Monitor Event to restart the Watchdog Process timer
    Watchdog_Expire,        // Monitor Event to expire the Watchdog Process timer
    Watchdog_Stop,          // Monitor Event to stop the Watchdog Process timer
    Watchdog_Shutdown,      // Monitor Event to shutdown the Watchdog Process timer
    Watchdog_Exit           // Monitor Event to exit the Watchdog process 
} WatchdogEvent_t;

typedef enum {
    PStartD_StartPersist=2000,  // Event to PSD to start persistent processes
                                // that do not require DTM
    PStartD_StartPersistDTM     // Event to PSD to start persistent processes
                                // that require DTM
} PStartDEvent_t;

typedef enum {
    ConfigType_Undefined=0,                 // Invalid
    ConfigType_Cluster,                     // Gobal to cluster configuration data
    ConfigType_Node,                        // Local to node configuration data
    ConfigType_Process                      // Gobal to process configuration data
} ConfigType;

typedef enum {
    CommType_Undefined=0,
    CommType_InfiniBand,
    CommType_Sockets
} CommType_t;

typedef enum 
{
    State_UnMounted =0,                     // Device not mounted
    State_Mounted                           // Device mounted
} DEVICESTATE;

typedef enum {
    State_Unknown=0,                        // Invalid
    State_Up,                               // Object is available for use
    State_Down,                             // Object has failed and is unavailable
    State_Stopped,                          // Object has terminated and is unavailable
    State_Shutdown,                         // Object is in the process of terminating
    State_Unlinked,
    State_Initializing,                     // Object not yet ready for use
    State_Joining,                          // Node is ready for Join Phase
    State_Merging,                          // Node is merged to MPI collective
    State_Merged,                           // Node merge with MPI collective is in use by cluster
    State_Takeover                          // Node is in takeover state
} STATE;

typedef enum {
    Dump_Unknown=0,                         // Invalid
    Dump_Ready,                             // ProcessObject can be dumped
    Dump_Pending,                           // ProcessObject dump requested
    Dump_InProgress,                        // ProcessObject dump in process
    Dump_Complete                           // ProcessObject dump complete
} DUMPSTATE;
typedef enum {
    Dump_Success=0,                         // Success
    Dump_Failed                             // Failed
} DUMPSTATUS;

// Node Re-integration phases
typedef enum {
    JoiningPhase_Unknown=0,                 // Invalid
    JoiningPhase_1,                         // Join initiated
    JoiningPhase_2,                         // Join in progress
    JoiningPhase_3,                         // Join completing
    JoiningPhase_Invalid                    // Invalid
} JOINING_PHASE;

typedef enum {
    RoleType_Undefined=0,       // Maps to ZoneType_Any
    RoleType_Maintenance,       // Maps to ZoneType_Edge, Frontend or Any
    RoleType_Operation,         // Maps to ZoneType_Edge, Frontend or Any
    RoleType_Connection,        // Maps to ZoneType_Edge, Frontend or Any
    RoleType_Loader,            // Maps to ZoneType_Edge, Frontend or Any
    RoleType_Aggregation,       // Maps to ZoneType_Aggregation, Backend or Any
    RoleType_Storage            // Maps to ZoneType_Storage, Backend or Any
} RoleType;

typedef enum {
    ZoneType_Undefined = 0x0000,            // No zone type defined
    ZoneType_Edge = 0x0001,                 // Zone of service only nodes
    ZoneType_Aggregation = 0x0002,          // Zone of compute only nodes
    ZoneType_Storage = 0x0004,              // Zone of storage only nodes
    ZoneType_Excluded = 0x0010,             // Excluded cores
    ZoneType_Any = ( ZoneType_Edge | ZoneType_Aggregation | ZoneType_Storage ),
    ZoneType_Frontend = ( ZoneType_Edge | ZoneType_Aggregation ),
    ZoneType_Backend = ( ZoneType_Aggregation | ZoneType_Storage )
} ZoneType;

// Service Request types
// note: other data structures depend on the ordering of the REQTYPE elements.
//       if the ordering changes corresponding changes must be made to 
//       SQ_LocalIOToClient::serviceRequestSize and CReqQueue::svcReqType.
typedef enum {
    ReqType_Close=1,                        // process closing server request
    ReqType_Dump,                           // dump process
    ReqType_Event,                          // send target processes an Event notice
    ReqType_Exit,                           // process is exiting
    ReqType_Get,                            // Retrieve information from the registry
    ReqType_Kill,                           // stop and cleanup the identified process
    ReqType_Mount,                          // mount device associated with process    
    ReqType_NewProcess,                     // process is request server to be spawned
    ReqType_NodeDown,                       // request to take down the identified node
    ReqType_NodeInfo,                       // node information request 
    ReqType_NodeUp,                         // request to bring up the identified node
    ReqType_Notice,                         // this is a informational message only
    ReqType_Notify,                         // register process to receive death notifications
    ReqType_Open,                           // process opening server request
    ReqType_OpenInfo,                       // request open information for process
    ReqType_PNodeInfo,                      // physical node information request 
    ReqType_ProcessInfo,                    // process information request
    ReqType_ProcessInfoCont,                // process information request (continuation)
    ReqType_Set,                            // Add configuration infomation to the registry 
    ReqType_Shutdown,                       // request cluster shutdown
    ReqType_Startup,                        // process startup notification
    ReqType_Stfsd,                          // process stfsd request
    ReqType_TmLeader,                       // request to become the TM leader
    ReqType_TmReady,                        // request to indicate TM ready for transactions
    ReqType_TmSync,                         // request to sync data across all TM's in cluster
    ReqType_TransInfo,                      // request transaction enlistment information
    ReqType_MonStats,                       // get monitor statistics
    ReqType_ZoneInfo,                       // zone information request 
    ReqType_NodeName,                       // change node name request

    ReqType_Invalid                         // marks the end of the request
                                            // types, add any new request types 
                                            // before this one
} REQTYPE;

// Reply types
// note: other data structures depend on the ordering of the REPLYTYPE elements.
//       if the ordering changes corresponding changes must be made to 
//       SQ_LocalIOToClient::serviceReplySize.
typedef enum {
    ReplyType_Generic=100,                  // general reply across message types
    ReplyType_Dump,                         // reply with dump info
    ReplyType_Get,                          // reply with configuration key/value pairs
    ReplyType_NewProcess,                   // reply with new process information
    ReplyType_NodeInfo,                     // reply with info on list of nodes
    ReplyType_PNodeInfo,                    // reply with info on list of physical nodes
    ReplyType_ProcessInfo,                  // reply with info on list of processes
    ReplyType_Open,                         // reply with open server information
    ReplyType_OpenInfo,                     // reply with list of opens for a process
    ReplyType_TmSync,                       // reply from unsolicited TmSync message
    ReplyType_TransInfo,                    // reply with transaction enlistment process list
    ReplyType_Stfsd,                        // reply with stfsd info
    ReplyType_Startup,                      // reply with startup info
    ReplyType_Mount,                        // reply with mount info
    ReplyType_MonStats,                     // reply with monitor statistics
    ReplyType_ZoneInfo,                     // reply with info on list of zones
    ReplyType_NodeName,                     // reply with results


    ReplyType_Invalid                       // marks the end of the reply types,
                                            // add any new reply types before
                                            // this one
} REPLYTYPE;

// Request types
// note: other data structures depend on the ordering of the MSGTYPE elements.
//       if the ordering changes corresponding changes must be made to 
//       SQ_LocalIOToClient::requestSize
typedef enum {
    MsgType_Change=1,                       // registry information has changed notification
    MsgType_Close,                          // process close notification
    MsgType_Event,                          // generic event notification
    MsgType_NodeDown,                       // node is down notification
    MsgType_NodeJoining,                    // node is joining notification
    MsgType_NodePrepare,                    // node prepare notification
    MsgType_NodeQuiesce,                    // node quiesce notification (always followed by node down)
    MsgType_NodeUp,                         // node is up notification
    MsgType_Open,                           // process open notification
    MsgType_ProcessCreated,                 // process creation completed notification
    MsgType_ProcessDeath,                   // process death notification
    MsgType_Service,                        // request a service from the monitor
    MsgType_SpareUp,                        // spare node is up notification
    MsgType_Shutdown,                       // system shutdown notification
    MsgType_TmRestarted,                    // DTM process restarted notification
    MsgType_TmSyncAbort,                    // request to abort TM sync data previously received
    MsgType_TmSyncCommit,                   // request to commit previously received TM sync data
    MsgType_UnsolicitedMessage,             // Outgoing monitor msg expecting a reply 
    MsgType_ReintegrationError,             // Problem during node reintegration

    MsgType_Invalid                         // marks the end of the message
                                            // types, add any new message types 
                                            // before this one
} MSGTYPE;

typedef enum {
    ProcessType_Undefined=0,                // No process type as been defined
    ProcessType_TSE,                        // Identifies a Table Storage Engine (DP2)
    ProcessType_DTM,                        // Identifies a Distributed Transaction Monitor process
    ProcessType_ASE,                        // Identifies a Audit Storage Engine (ADP)
    ProcessType_Generic,                    // Identifies a generic process
    ProcessType_Watchdog,                   // Identifies the monitor's watchdog processes
    ProcessType_AMP,                        // Identifies a AMP process
    ProcessType_Backout,                    // Identifies a Backout process
    ProcessType_VolumeRecovery,             // Identifies a Volume Recovery process
    ProcessType_MXOSRVR,                    // Identifies a MXOSRVR process
    ProcessType_SPX,                        // Identifies a SeaPilot ProXy process
    ProcessType_SSMP,                       // Identifies a SQL Statistics Merge Process (SSMP)
    ProcessType_PSD,                        // Identifies the monitor's process start daemon processes
    ProcessType_SMS,                        // Identifies a SeaMonster Service process

    ProcessType_Invalid                     // marks the end of the process
                                            // types, add any new process
                                            // types before this one
} PROCESSTYPE;

typedef enum {
    ShutdownLevel_Undefined=-1,
    ShutdownLevel_Normal=0,                 // Wait for all transactions and processes to end
    ShutdownLevel_Immediate,                // Abort transactions and wait for processes to end
    ShutdownLevel_Abrupt                    // just kill all processes and stop
} ShutdownLevel;

struct Change_def
{
    ConfigType type;                        // type of group that has changed 
    char group[MAX_KEY_NAME];               // name of the group that has changed 
    char key[MAX_KEY_NAME];                 // name of the configured item that has changed
    char value[MAX_VALUE_SIZE];             // value currently assigned to the name
};

struct Close_def
{
    int  nid;                              // requesting process's node id
    int  pid;                              // requesting process's id
    Verifier_t verifier;                   // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];   // requesting process's name
    int  aborted;                          // Non-zero if close because of process abort
    int  mon;                              // Non-zero if monitor close
};

struct Dump_def
{
    int  nid;                              // requesting process's node id
    int  pid;                              // requesting process's id
    Verifier_t verifier;                   // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];   // requesting process's name
    char path[MAX_PROCESS_PATH];           // path for dump-file
    int  target_nid;                       // Nid of process to dump ( -1 if using name )
    int  target_pid;                       // Pid of process to dump ( -1 if using name )
    Verifier_t target_verifier;            // Verifier of process to dump ( -1 if using name )
    char target_process_name[MAX_PROCESS_NAME];   // Name of process to dump
};

struct Dump_reply_def
{
    int nid;                                // target process's node id
    int pid;                                // target process's process id
    Verifier_t verifier;                    // target process's verifier
    char process_name[MAX_PROCESS_NAME];    // target process's name
    char core_file[MAX_PROCESS_PATH];       // name of core-file
    int return_code;                        // error returned to sender
};

struct Event_def
{
    int  nid;                  // requesting process's node id
    int  pid;                  // requesting process id
    Verifier_t verifier;       // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];   // requesting process's name
    int  target_nid;           // Node id of processes to receive event (-1 for all all nodes)
    int  target_pid;           // Process id of processes to receive event (-1 for all in node)
    Verifier_t target_verifier; // Process verifier of processes to receive event (-1 for all in node)
    char target_process_name[MAX_PROCESS_NAME];    // Name of target process
    PROCESSTYPE type;          // Process type of processes to receive event 
                               //   (ProcessType_Undefined for not type filtering)
    int  event_id;             // Non-Zero user defined event id to be pass in event notice
    int  length;               // The length in bytes used the data buffer
    char data[MAX_SYNC_DATA];  // Data buffer to be sent to selected processes
};

struct Event_Notice_def
{
    int  event_id;             // Non-Zero user defined event id to be pass in event notice
    int  length;               // The length in bytes used the data buffer
    char data[MAX_SYNC_DATA];  // Data buffer to be sent to selected processes
};

struct Exit_def
{                                               
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    Verifier_t verifier;                    // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];    // requesting process's name
};

struct Generic_reply_def
{
    int nid;                                // target process's node id
    int pid;                                // target process's process id
    Verifier_t verifier;                    // target process's verifier
    char process_name[MAX_PROCESS_NAME];    // target process's name
    int return_code;                        // error returned to sender
};

struct Get_def
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    Verifier_t verifier;                    // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];    // requesting process's name
    ConfigType type;                        // type of group being requested
    bool next;                              // if true, get key list starting after key 
                                            // else start new list
    char group[MAX_KEY_NAME];               // name of group, if NULL and type=ConfigNode assume local node 
    char key[MAX_KEY_NAME];                 // name of the item to be returned, Null for all in group
};

struct Get_reply_def
{
    ConfigType type;                        // type of group being returned 
    char group[MAX_KEY_NAME];               // name of the group of items returned 
    int num_keys;                           // Number of keys in request set
    int num_returned;                       // Number of keys returned
    struct
    {
        char key[MAX_KEY_NAME];             // name of the configured item
        char value[MAX_VALUE_SIZE];         // value currently assigned to the name
    } list[MAX_KEY_LIST];
};

struct Kill_def
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    Verifier_t verifier;                    // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];    // requesting process's name
    int  target_nid;                        // Node id of processes to kill (-1 for all)
    int  target_pid;                        // Process id of process to kill (-1 for all)
    Verifier_t target_verifier;             // Process verifier of processes to kill (-1 if process name only)
    char target_process_name[MAX_PROCESS_NAME];    // Name of process to kill
    bool persistent_abort;                  // when true, persistent process is not restarted
                                            // otherwise, it is ignored
};

struct MonStats_def
{
    int placeholder;
};

struct MonStats_reply_def
{
    int acquiredMax;
    int availMin;
    int bufMisses;
};

struct Mount_def                            // to mount device associated with process
{                                               
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    Verifier_t verifier;                    // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];    // requesting process's name
};

struct Mount_reply_def
{
    DEVICESTATE  primary_state;             // State of primary device
    DEVICESTATE  mirror_state;              // State of mirror device
    int          return_code;               // mpi error code of spawn operation
}; 

struct NewProcess_def
{
    int  nid;                               // zone or node id if TM type to start process on, 
                                            // if -1 then will assign node
    PROCESSTYPE type;                       // Identifies the process handling catagory
    int  priority;                          // Linux system priority
    int  debug;                             // if non-zero, starts processing using GDB
    int  backup;                            // if non-zero, starts process as backup
    bool unhooked;                          // if hooked, parent process dies will trigger child process exits
    bool nowait;                            // reply after local node initiates new process and send notice on completion
    long long tag;                          // user defined tag to be sent in completion notice
    char path[MAX_SEARCH_PATH];             // process's object lookup path to program
    char ldpath[MAX_SEARCH_PATH];           // process's library load path for program
    char program[MAX_PROCESS_PATH];         // full path to object file
    char process_name[MAX_PROCESS_NAME];    // NSK process name
    int  argc;                              // number of additional command line argument
    char argv[MAX_ARGS][MAX_ARG_SIZE];      // array of additional command line arguments
    char infile[MAX_PROCESS_PATH];          // if null then use monitor's infile
    char outfile[MAX_PROCESS_PATH];         // if null then use monitor's outfile
    int  fill1;                             // filler to fill out struct
};

struct NewProcess_reply_def
{
    int  nid;                               // node id of started process
    int  pid;                               // internal process id of started process
    Verifier_t verifier;                    // Process verifier
    char process_name[MAX_PROCESS_NAME];    // NSK process names assigned to started process
    int  return_code;                       // mpi error code of spawn operation
};

struct NewProcess_Notice_def 
{
    int  nid;                               // node id of started process
    int  pid;                               // internal process id of started process
    Verifier_t verifier;                    // Process verifier
    long long tag;                          // user tag sent with original request
    char port[MPI_MAX_PORT_NAME];           // mpi port to started process
    char process_name[MAX_PROCESS_NAME];    // NSK process names assigned to started process
    int  return_code;                       // mpi error code of spawn operation
};

struct NodeDown_def
{
    int  nid;
    char node_name[MPI_MAX_PROCESSOR_NAME];
    int  takeover;                          // true if Spare Node activation
#ifdef USE_SEQUENCE_NUM
    long long seqnum;                       // sequence number
#endif
    char reason[MAX_REASON_TEXT];           // text describing reason for down node
};

struct NodeInfo_def
{
    int nid;                                // node id of requesting process
    int pid;                                // process id of requesting process
    int target_nid;                         // get information on node id (-1 for all)
    int last_nid;                           // Last Logical Node ID returned
    int last_pnid;                          // Last Physical Node ID returned
    bool continuation;                      // true if continuation of earlier request
};

struct NodeInfo_reply_def
{
    int num_nodes;                          // Number of logical nodes in the cluster
    int num_pnodes;                         // Number of physical nodes in the cluster
    int num_spares;                         // Number of spare nodes in the cluster
    int num_available_spares;               // Number of currenly available spare nodes in the cluster
    int num_returned;                       // Number of nodes returned
    struct
    {
        int      nid;                       // Node's ID 
        STATE    state;                     // Node's state (i.e. UP, DOWN, STOPPING)
        ZoneType type;                      // Node's fault zone type (service,compute,storge,any,frontend,backend)
        int      processors;                // Number of processors in logical node
        int      process_count;             // Number of processes in executing on the node
        int      pnid;                      // Node's Physical ID 
        STATE    pstate;                    // Physical Node's state (i.e. UP, DOWN, STOPPING)
        bool     spare_node;                // True when physical node is spare
        unsigned int memory_total;          // Node's total memory
        unsigned int memory_free;           // Node's current free memory
        unsigned int swap_free;             // Node's current free swap
        unsigned int cache_free;            // Node's current free buffer/cache
        unsigned int memory_active;         // Node's memory in active use
        unsigned int memory_inactive;       // Node's memory available for reclamation
        unsigned int memory_dirty;          // Node's memory waiting to be written to disk
        unsigned int memory_writeback;      // Node's memory being written to disk
        unsigned int memory_VMallocUsed;    // Node's amount of used virtual memory
        long long    cpu_user;              // Time in user mode
        long long    cpu_nice;              // Time in user mode, low priority
        long long    cpu_system;            // Time in system mode
        long long    cpu_idle;              // Time in idle task
        long long    cpu_iowait;            // Time waiting for i/o
        long long    cpu_irq;               // Time in hardware long longerrupt
        long long    cpu_soft_irq;          // Time in software interrupt
        unsigned int btime;                 // Boot time (secs since 1/1/1970)
        char     node_name[MPI_MAX_PROCESSOR_NAME]; // Node's name
        int      cores;                     // Number of processors in physical node
    } node[MAX_NODE_LIST];
    int return_code;                        // error returned to sender
    int last_nid;                           // Last Logical Node ID returned
    int last_pnid;                          // Last Physical Node ID returned
    bool continuation;                      // true if continuation of earlier request
};

struct NodeName_def
{
    int nid;                                    // node id of requesting process
    int pid;                                    // process id of requesting process
    char new_name[MPI_MAX_PROCESSOR_NAME];      // get information on node id (-1 for all)
    char current_name[MPI_MAX_PROCESSOR_NAME];  // current name of node (validation)
};

struct NodeName_reply_def
{
    int return_code;                        // error returned to sender
};

struct NodeJoining_def
{
    int  pnid;
    char node_name[MPI_MAX_PROCESSOR_NAME];
    JOINING_PHASE  phase;
};

struct PNodeInfo_def
{
    int  nid;                               // node id of requesting process
    int  pid;                               // process id of requesting process
    int  target_pnid;                       // get information on pnode id (-1 for all when node_name is null string)
    char target_name[MPI_MAX_PROCESSOR_NAME]; // get information on pnode by node name
    int  last_pnid;                         // Last Physical Node ID returned
    bool continuation;                      // true if continuation of earlier request
};

struct PNodeInfo_reply_def
{
    int num_nodes;                          // Number of logical nodes in the cluster
    int num_pnodes;                         // Number of physical nodes in the cluster
    int num_spares;                         // Number of spare nodes in the cluster
    int num_available_spares;               // Number of currenly available spare nodes in the cluster
    int num_returned;                       // Number of nodes returned
    struct
    {
        int      pnid;                      // Node's Physical ID 
        char     node_name[MPI_MAX_PROCESSOR_NAME]; // Node's name
        STATE    pstate;                    // Physical Node's state (i.e. UP, DOWN, STOPPING)
        int      lnode_count;               // Number of logical nodes
        int      process_count;             // Number of processes in executing on the node
        bool     spare_node;                // True when physical node is spare
        unsigned int memory_total;          // Node's total memory
        unsigned int memory_free;           // Node's current free memory
        unsigned int swap_free;             // Node's current free swap
        unsigned int cache_free;            // Node's current free buffer/cache
        unsigned int memory_active;         // Node's memory in active use
        unsigned int memory_inactive;       // Node's memory available for reclamation
        unsigned int memory_dirty;          // Node's memory waiting to be written to disk
        unsigned int memory_writeback;      // Node's memory being written to disk
        unsigned int memory_VMallocUsed;    // Node's amount of used virtual memory
        unsigned int btime;                 // Boot time (secs since 1/1/1970)
        int      cores;                     // Number of processors in physical node
    } node[MAX_NODE_LIST];
    int return_code;                        // error returned to sender
    int last_pnid;                          // Last Physical Node ID returned
    bool integrating;                       // true if re-integration in progress in local monitor
    bool continuation;                      // true if continuation of earlier request
};

struct NodePrepare_def
{
    int  nid;
    char node_name[MPI_MAX_PROCESSOR_NAME];
    int  takeover;                          // true if Spare Node activation
};

struct NodeQuiesce_def
{
    int  nid;
    char node_name[MPI_MAX_PROCESSOR_NAME];
};

struct NodeReInt_def 
{
    char msg[200];
};

struct NodeUp_def
{
    int  nid;
    char node_name[MPI_MAX_PROCESSOR_NAME];
    int  takeover;                          // true if Spare Node activation
#ifdef USE_SEQUENCE_NUM
    long long seqnum;                       // sequence number
#endif
};

struct Notify_def                           // Register for death notification
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    Verifier_t verifier;                    // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];    // requesting process's name
    int  cancel;                            // Zero to set notice else cancel current notice
    int  target_nid;                        // Nid of process to be monitored, if -1 cancel all associated with trans_id
    int  target_pid;                        // Pid of process to be monitored, if -1 cancel all associated with trans_id
    Verifier_t target_verifier;             // Verifier of process to be monitored, if -1 cancel all associated with trans_id
    char target_process_name[MAX_PROCESS_NAME];    // monitored process's name
    int  fill1;                             // filler to trans_id
    _TM_Txid_External trans_id;             // associated transaction id, zero for none.
};                                          // note: cancel=0, target_nid or target_pid = -1 is invalid

struct Open_def
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    Verifier_t verifier;                    // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];    // requesting process's name (if notification)
    int  target_nid;                        // Process to open node id
    int  target_pid;                        // Process to open id
    Verifier_t target_verifier;             // Process to open verifier
    char target_process_name[MAX_PROCESS_NAME]; // Process to open name
    int  death_notification;                // true if death notice needed on process failure
};

struct Open_reply_def
{
    int  nid;                               // opened process's node id 
    int  pid;                               // opened process's process id
    Verifier_t verifier;                    // opened process's process verifier
    char process_name[MAX_PROCESS_NAME];    // opened process's name
    char port[MPI_MAX_PORT_NAME];           // opened process's mpi port used in MPI_Comm_connect
    PROCESSTYPE type;                       // opened process's process type
    int  return_code;                       // mpi error code
};

struct Close_reply_def
{
    int  nid;                               // Close requesting process's node id
    int  pid;                               // Close requesting process's process id
    Verifier_t verifier;                    // Close requesting process's verifier
    char process_name[MAX_PROCESS_NAME];    // Close requesting process's name
    int  return_code;                       // mpi error code
};

struct OpenInfo_def
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    int  target_nid;                        // Nid of process to list opens ( -1 if using name )
    int  target_pid;                        // Pid of process to list opens ( -1 if using name )
    char process_name[MAX_PROCESS_NAME];    // Name of process to list opens
    bool who_opened;                        // false for list of processes opened by this process
                                            // true for list of who has this process opened
    bool continuation;                      // true if continuation of earlier request
    struct                                  // last few process ids returned
    {                                       //    previously
        int  nid;                           // node id
        int  pid;                           // process id
    } context[MAX_OPEN_CONTEXT];
};

struct OpenInfo_reply_def
{
    int  num_opens;                         // number of opens returned
    struct 
    {
        int  nid;                           // opened process's node id
        int  pid;                           // opened process's process id
        char process_name[MAX_PROCESS_NAME];// opened process's name
        bool stale;                         // true if open to terminated process
    } opens[MAX_OPEN_LIST];
    int  return_code;                       // error returned to sender
};

struct ProcessDeath_def
{
    int nid;                                // dead process's node id
    int pid;                                // dead process's process id
    Verifier_t verifier;                    // dead process's verifier
    _TM_Txid_External trans_id;
    int aborted;                            // Non-zero indicates abnormal termination
    char process_name[MAX_PROCESS_NAME];    // dead process's name
    PROCESSTYPE type;                       // Process type
#ifdef USE_SEQUENCE_NUM
    long long seqnum;                       // sequence number
#endif
};

struct ProcessInfo_def
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    Verifier_t verifier;                    // requesting process' verifier
    char process_name[MAX_PROCESS_NAME];    // requesting process' name
    int  target_nid;                        // Node id of processes for status request (-1 for all)
    int  target_pid;                        // Process id of process for status request (-1 for all)
    Verifier_t target_verifier;             // Verifier of process for status request (-1 for if not used)
    char target_process_name[MAX_PROCESS_NAME]; // Name of process for status request (NULL if not used)
    PROCESSTYPE type;                       // Return only processes of this type (ProcessType_Undefined for all)
};

typedef
struct ProcessInfoState
{
    int   nid;                              // process's node id
    int   pid;                              // process's process id
    Verifier_t verifier;                    // process's process verifier
    PROCESSTYPE type;                       // process handling catagory
    char  process_name[MAX_PROCESS_NAME];   // process's Name
    int   os_pid;                           // process's OS based process id
    int   priority;                         // process's OS based priority
    int   parent_nid;                       // process's parent's node id
    int   parent_pid;                       // process's parent's process id
    Verifier_t parent_verifier;             // process's parent's process verifier
    char  parent_name[MAX_PROCESS_NAME];    // process's Name
    char  program[MAX_PROCESS_PATH];        // process's object file name
    STATE state;                            // process's current state
    bool  event_messages;                   // true if receiving system notices
    bool  system_messages;                  // true if receiving system notices
    bool  paired;                           // true if functional copy of like process on another node
    bool  pending_delete;                   // true if process being deleted from cluster
    bool  pending_replication;              // true if process pending replication to cluster
    bool  waiting_startup;                  // true if process has not sent startup message
    bool  opened;                           // true if process is currently opened by another process
    bool  backup;                           // true if process is a backup to the parent process
    bool  unhooked;                         // false if parent process dies will trigger child process exit
    struct timespec creation_time;          // creation time
} ProcessInfoState;

struct ProcessInfo_reply_def
{
    int  num_processes;                     // number of process returned
    ProcessInfoState process[MAX_PROCINFO_LIST];
    int  return_code;                       // error returned to sender
    bool more_data;                         // true if have additional process data
};

struct ProcessInfoCont_def
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    struct                                  // last few process ids returned
    {                                       //    previously
        int  nid;                           // node id
        int  pid;                           // process id
    } context[MAX_PROC_CONTEXT];
    PROCESSTYPE type;                       // Return only processes of this type
    bool allNodes;                          // true if requesting process info on all nodes
};


struct Set_def
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    Verifier_t verifier;                    // requesting process's verifier
    char process_name[MAX_PROCESS_NAME];    // requesting process's name
    ConfigType type;                        // type of group being set
    char group[MAX_KEY_NAME];               // name of group, if NULL and type=ConfigNode assume local node 
    char key[MAX_KEY_NAME];                 // key name of the item being set
    char value[MAX_VALUE_SIZE];             // value of key
};

struct Shutdown_def
{
    int  nid;                               // requesting process's node id
    int  pid;                               // requesting process id
    ShutdownLevel level;                    // 0=normal, 1=fast or 2=crash
};

struct SpareUp_def
{
    int  pnid;
    char node_name[MPI_MAX_PROCESSOR_NAME];
};

struct Startup_def
{
    int   nid;                              // process's node id argv[3] or -1 for attach
    int   pid;                              // process's process id argv[4] or -1 for attach
    int   os_pid;                           // process's native OS process id
    char  process_name[MAX_PROCESS_NAME];   // process's NSK name argv[5] or assign for attach, "" will autogenerate name
    char  port_name[MPI_MAX_PORT_NAME];     // mpi port name from MPI_Open_port
    char  program[MAX_PROCESS_PATH];        // process's object filename
    bool  event_messages;                   // true if want event messages
    bool  system_messages;                  // true if want system messages
    bool  paired;                           // true if should pair with name process
    Verifier_t  verifier;                   // process's verifier
    int   startup_size;                     // size of this struct
};

struct Startup_reply_def
{
    int nid;                                // target process's node id
    int pid;                                // target process's process id
    Verifier_t  verifier;                   // process's verifier
    int   startup_size;                     // size of this struct
    int return_code;                        // error returned to sender
    char process_name[MAX_PROCESS_NAME];    // Name of target process
    char fifo_stdin  [MAX_PROCESS_PATH];
    char fifo_stdout [MAX_PROCESS_PATH];
    char fifo_stderr [MAX_PROCESS_PATH];
};

struct Stfsd_def
{
    int  nid;                               // Requester's node id
    int  pid;                               // Requester's process id
    int  tag;                               // Requester's tag
    int  length;                            // The length in bytes used the data buffer
    char data[MAX_STFSD_DATA];              // The data to be sent 
};

struct Stfsd_reply_def
{
    int  nid;                                // Replying STFSD's node id
    int  pid;                                // Replying STFSD's process id
    int  return_code;                        // If non-zero, error code
    int  length;                             // The length in bytes of the data buffer
    char data[MAX_STFSD_DATA];               // Reply data
};

struct TmLeader_def
{
    int nid;                                // Requesting TM's node id
    int pid;                                // Requesting TM's process id
};

struct TmReady_def
{
    int nid;                                // Requesting TM's node id
    int pid;                                // Requesting TM's process id
};

struct TmRestarted_def
{
    int  nid;                               // Restarted TM's logical node id
    int  pnid;                              // Restarted TM's physical node id
    char node_name[MPI_MAX_PROCESSOR_NAME]; // Restarted TM's physical node name
};

struct TmSync_def
{
    int  nid;                               // Requesting TM's node id
    int  pid;                               // Requesting TM's process id
    int  tag;                               // Requesting TM's tag
    int  length;                            // The length in bytes used the data buffer
    char data[MAX_SYNC_DATA];               // The data to be sent to all TM's in the cluster
};

struct TmSync_reply_def
{
    int nid;                                // Replying TM's node id
    int pid;                                // Replying TM's process id
    int handle;                             // Request associated handle for sync completed notice
    int return_code;                        // If non-zero, TM not excepting sync data
};

struct TmSyncNotice_def
{
    int nid[MAX_TM_SYNCS];                  // Owning TM's node id
    int orig_count;                         // Number of originator tags
    int orig_tag[MAX_TM_SYNCS];             // Originator tag (only valid for orig)
    int orig_handle[MAX_TM_SYNCS];          // Originator handle (only valid for orig)
    int count;                              // Number of handles returned
    int handle[MAX_TM_SYNCS];               // Requests associated handle for sync completed notice
};

struct UnsolicitedTmSync_def
{
    int  nid;                               // Requesting TM's node id or target TM's node id
    int  pid;                               // Requesting TM's process id or target  TM's process id
    int  handle;                            // Request associated handle for sync completed notice
    int  length;                            // The length in bytes used the data buffer
    char data[MAX_SYNC_DATA];               // The data to be sent to all TM's in the cluster
};

struct TransInfo_def
{ // Deprecated
    int  nid;                               // Requesting process's node id
    int  pid;                               // Requesting process id
    char process_name[MAX_PROCESS_NAME];    // Name of process to list associated transactions
                                            // If NULL then use trans-id to list assocaited process
    _TM_Txid_External trans_id;             // Transaction ID of enlisted processes to list
};

struct TransInfo_reply_def
{ // Deprecated
    int  num_processes;                     // Number of process returned
    struct 
    {
        int  nid;                           // Associated process's node id
        int  pid;                           // Associated process's process id
        _TM_Txid_External trans_id;         // Transiaction ID associated with process
    } procs[MAX_PROC_LIST];
    int  return_code;                       // Error returned to sender
};

struct ZoneInfo_def
{
    int nid;                                // node id of requesting process
    int pid;                                // process id of requesting process
    int target_nid;                         // get zone information via node id (-1 for all)
    int target_zid;                         // get zone information via zone id (-1 for all)
    int last_nid;                           // Last Logical Node ID returned
    int last_pnid;                          // Last Physical Node ID returned
    bool continuation;                      // true if continuation of earlier request
};

struct ZoneInfo_reply_def
{
    int num_nodes;                          // Number of logical nodes in the cluster
    int num_returned;                       // Number of node entries returned
    struct
    {
        int      nid;                       // Logical Node's ID 
        int      zid;                       // Logical Node's Zone ID 
        int      pnid;                      // Node's Physical ID 
        STATE    pstate;                    // Physical Node's state (i.e. UP, DOWN, STOPPING)
        char     node_name[MPI_MAX_PROCESSOR_NAME]; // Node's name
    } node[MAX_NODE_LIST];
    int return_code;                        // error returned to sender
    int last_nid;                           // Last Logical Node ID returned
    int last_pnid;                          // Last Physical Node ID returned
    bool continuation;                      // true if continuation of earlier request
};

struct request_def
{
    REQTYPE type;
    union
    {
        struct Change_def            change;
        struct Close_def             close;
        struct ProcessDeath_def      death;
        struct NodeDown_def          down;
        struct Dump_def              dump;
        struct Event_def             event;
        struct Event_Notice_def      event_notice;
        struct Exit_def              exit;
        struct Get_def               get;
        struct Mount_def             mount;
        struct Kill_def              kill;
        struct NewProcess_def        new_process;
        struct NodeInfo_def          node_info;
        struct Notify_def            notify;
        struct Open_def              open;
        struct OpenInfo_def          open_info;
        struct NewProcess_Notice_def process_created;
        struct ProcessInfo_def       process_info;
        struct ProcessInfoCont_def   process_info_cont;
        struct Set_def               set;
        struct Shutdown_def          shutdown;
        struct Startup_def           startup;
#ifdef SQ_STFSD
        struct Stfsd_def             stfsd;
#endif
        struct TmLeader_def          leader;
        struct TmReady_def           tm_ready;
        struct TmRestarted_def       tm_restart;
        struct TmSync_def            tm_sync;
        struct TmSyncNotice_def      tm_sync_notice;
        struct TransInfo_def         trans_info;
        struct UnsolicitedTmSync_def unsolicited_tm_sync;
        struct NodeUp_def            up;
        struct NodeQuiesce_def       quiesce;
        struct NodePrepare_def       prepare;
        struct ZoneInfo_def          zone_info;
        struct NodeJoining_def       joining;
        struct PNodeInfo_def         pnode_info;
        struct SpareUp_def           spare_up;
        struct NodeReInt_def         reintegrate;
        struct NodeName_def          nodename;
    } u;
};

struct reply_def
{
    REPLYTYPE type;
    union
    {
        struct Dump_reply_def          dump;
        struct Generic_reply_def       generic;
        struct Get_reply_def           get;
        struct Mount_reply_def         mount;
        struct NewProcess_reply_def    new_process;
        struct NodeInfo_reply_def      node_info;
        struct Open_reply_def          open;
        struct OpenInfo_reply_def      open_info;
        struct PNodeInfo_reply_def     pnode_info;
        struct ProcessInfo_reply_def   process_info;
        struct Startup_reply_def       startup_info;
#ifdef SQ_STFSD
        struct Stfsd_reply_def         stfsd;
#endif
        int                            tm_seqnum;
        struct TmSync_reply_def        tm_sync;
        struct TransInfo_reply_def     trans_info;
        struct TmSync_reply_def        unsolicited_tm_sync;
        struct Close_reply_def         close;
        struct MonStats_reply_def      mon_info;
        struct ZoneInfo_reply_def      zone_info;
        struct NodeName_reply_def      nodename;
    } u;
};

struct message_def
{
    MSGTYPE type;
    bool noreply; // true if the receiving process is not to send reply.
    int reply_tag;
    union
    {
        int                 handle;    // used only for TmSync notices
        struct request_def  request;
        struct reply_def    reply;
    } u;
};

#endif /*MSGDEF_H_*/
