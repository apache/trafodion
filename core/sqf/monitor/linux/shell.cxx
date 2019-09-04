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

using namespace std;

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mpi.h>
#include <unistd.h>
#include <curses.h>
#include <semaphore.h>
#include <term.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <netdb.h>

#include "msgdef.h"
#include "props.h"
#include "localio.h"
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "clusterconf.h"
#include "nameserverconfig.h"
#include "seabed/trace.h"
#include "montrace.h"
#include "cmsh.h"
#include "lock.h"

#include "SCMVersHelp.h"

DEFINE_EXTERN_COMP_DOVERS(shell)
DEFINE_EXTERN_COMP_GETVERS(shell)

long trace_settings = 0;
int traceFileFb = 0;
bool traceOpen = false;
char traceFileName[MAX_PROCESS_PATH];
char mpirunOutFileName[MAX_PROCESS_PATH];
char mpirunErrFileName[MAX_PROCESS_PATH];
#define TRACE_SHELL_CMD         0x00001

#define MAX_TOKEN   132
#define MAX_BUFFER  512
#define MAX_CMDLINE 256
#define MAX_DEATH_SAVE 10

char *MyName;
char LDpath[MAX_SEARCH_PATH];
char Path[MAX_SEARCH_PATH];
char Wdir[MAX_SEARCH_PATH];
char prompt[MAX_PROCESS_NAME];
int VirtualNodes = 0;
int VirtualNid = -1;
int NumNodes = 0;
int NumLNodes = 0;
int CurNodes = 0;
int NumDown = 0;
int PNodesConfigMax = 0;
int LNodesConfigMax = 0;
bool Debug = false;
int  Measure = 0;
bool Attached = false;
bool BreakCmd = false;
bool Started = false;
bool ShutdownClean = false;
bool DTMexists = false;
bool Slave = false;
bool NodeState[MAX_NODES];
bool MpiInitialized = false;
bool SpareNodeColdStandby = true;
bool ElasticityEnabled = true;
bool NameServerEnabled = false;
bool QuietShell = false;
bool NodeAddUseFqdn = true;

AgentType_t AgentType = AgentType_Undefined;

int   lastDeathNid[MAX_DEATH_SAVE] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
int   lastDeathPid[MAX_DEATH_SAVE] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
bool  waitDeathPending = false;
int   waitDeathNid;
int   waitDeathPid;
CLock waitDeathLock;

bool  nodePending = false;
char  nodePendingName[MPI_MAX_PROCESSOR_NAME];
int   nodePendingNid;
int   nodePendingPnid;
CLock nodePendingLock;

CClusterConfig ClusterConfig; // Configuration objects
CNameServerConfigContainer NameServerConfig; // nameserver Configuration objects

char PNode[MAX_NODES][MPI_MAX_PROCESSOR_NAME];
char Node[MAX_NODES][MPI_MAX_PROCESSOR_NAME];
char MyNode[MPI_MAX_PROCESSOR_NAME];
char MyPort[MPI_MAX_PORT_NAME] = {0};;
int MyRank = -1;
int MyPNid = -1;
int MyNid = -1;
int MyPid = -1;
Verifier_t  MyVerifier = -1;
int LastNid = -1;               // Node id of last process started
int LastPid = -1;               // Process id of last process started
int MonitorNid = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
int gv_ms_su_pid = -1;          // Local IO pid to make compatible w/ Seabed
SB_Verif_Type gv_ms_su_verif = -1; // Local IO verifier to make compatible w/ Seabed
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // monitor port
struct message_def *msg;

static const char EnvNotStarted[] = "[%s] Environment has not been started!\n";

extern const char *ProcessTypeString( PROCESSTYPE type );
extern const char *PersistProcessTypeString( PROCESSTYPE type );

// forwards

char *cd_cmd (char *cmd_tail, char *wdir);
void  get_server_death (int nid, int pid);
int   get_lnodes_count( int nid );
bool  get_node_state( int nid, char *node_name, int &pnid, STATE &state, bool &integrating );
char *get_token (char *cmd, char *token, char *delimiter, int maxlen=MAX_TOKEN,
                 bool isEqDelim=true, bool isDashDelim=false);
int   get_pnid_by_nid( int nid );
bool  get_more_proc_info(PROCESSTYPE process_type, bool allNodes);
bool  get_zone_state( int &nid, int &zid , char *node_name, int &pnid, STATE &state );
void  interrupt_handler(int signal, siginfo_t *info, void *);
bool  isNumeric( char * str );
bool  load_configuration( void );
void  nameserver_add_cmd( char *cmd );
void  nameserver_config_cmd( char *cmd );
void  nameserver_delete_cmd( char *cmd );
void  nameserver_name_cmd( char *cmd );
void  nameserver_start( char *node_name );
void  nameserver_start_cmd( char *cmd );
void  nameserver_stop( char *node_name );
void  nameserver_stop_cmd ( char *cmd );
void  node_add_cmd( char *cmd, char delimiter );
void  node_config_cmd( char *cmd );
void  node_delete_cmd( char *cmd );
void  node_down( int nid, char *reason );
void  node_down_cmd ( char *cmd );
void  node_info_cmd( char *cmd );
void  node_name_cmd( char *cmd );
int   node_up( int nid, bool nowait=false );
void  node_up_cmd( char *cmd, char delimiter );
char *normalize_case (char *token);
void  normalize_slashes (char *token);
void  persist_config_cmd( char *cmd, bool keysOnly );
void  persist_exec_cmd( char *cmd );
void  persist_info_cmd( char *cmd );
void  persist_kill_cmd( char *cmd );
bool  persist_process_kill( CPersistConfig *persistConfig );
bool  persist_process_start( CPersistConfig *persistConfig );
char *remove_white_space (char *cmd);
void  remove_trailing_white_space (char *buf);
void  send_set_req( ConfigType type, char *name, char *key, char *value );
void  show_proc_info( void );
bool  start_monitor( char *cmd_tail, bool warmstart, bool reintegrate );
int   start_process (int *nid, PROCESSTYPE type, char *name, bool debug, int priority, bool nowait, char *infile, char *outfile, char *cmd_tail);
char *time_string( void );
void  write_startup_log( char *msg );

char *ErrorMsg (int error_code);
void  SetCallbacks ( void );

PhysicalNodeNameMap_t PhysicalNodeMap;

const char *FormatNidString( FormatNid_t type );
const char *FormatZidString( FormatZid_t type );

// classes

class MonCwd
{
public:
    MonCwd();
    ~MonCwd();

private:
    bool cwdChanged_;
    char savedDir_[MAX_SEARCH_PATH];

};

MonCwd::MonCwd(): cwdChanged_(false)
{
    const char method_name[] = "MonCwd::MonCwd";

    char *env = getenv("SQ_SHELL_NOCWD");
    if ( env )
    {
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s] Monitor working directory disabled\n",
                         method_name, __LINE__, MyName);
    }
    else
    {
        env = getenv("TRAF_HOME");
        if ( env )
        {
            string monWdir;

            monWdir = env;
            monWdir.append("/sql/scripts" );
            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf("%s@%d [%s] Monitor working directory: %s\n",
                             method_name, __LINE__, MyName, monWdir.c_str());


            if ( getcwd( savedDir_, sizeof( savedDir_ )) != NULL )
            {
                chdir( monWdir.c_str() );
                cwdChanged_ = true;
                if ( trace_settings & TRACE_SHELL_CMD )
                {
                    trace_printf("%s@%d [%s] Monitor working directory: %s\n",
                                 method_name, __LINE__, MyName, monWdir.c_str());
                    trace_printf("%s@%d [%s] Saved Monitor working directory: %s\n",
                                 method_name, __LINE__, MyName, savedDir_);
                }
            }

        }
    }
}

MonCwd::~MonCwd()
{
    const char method_name[] = "MonCwd::~MonCwd";

    if (  cwdChanged_ )
    {   // restore working directory
        chdir( savedDir_ );
        if ( trace_settings & TRACE_SHELL_CMD )
        {
            trace_printf("%s@%d [%s] Restored saved working directory: %s\n",
                         method_name, __LINE__, MyName, savedDir_);
        }
    }
}

// functions

const char *RoleTypeString( ZoneType type )
{
    const char *str;

    switch( type )
    {
        case ZoneType_Edge:
            str = "connection";
            break;
        case ZoneType_Excluded:
            str = "excluded";
            break;
        case ZoneType_Aggregation:
            str = "aggregation";
            break;
        case ZoneType_Storage:
            str = "storage";
            break;
        case ZoneType_Frontend:
            str = "connection,aggregation";
            break;
        case ZoneType_Backend:
            str = "aggregation,storage";
            break;
        case ZoneType_Any:
            str = "connection,aggregation,storage";
            break;
        default:
            str = "Undefined";
            break;
    }

    return( str );
}

const char *StateString( STATE state )
{
    const char *str;

    switch( state )
    {
    case State_Up:
        str = "Up";
        break;
    case State_Down:
        str = "Down";
        break;
    case State_Stopped:
        str = "Stopped";
        break;
    case State_Shutdown:
        str = "Shutdown";
        break;
    case State_Initializing:
        str = "Initializing";
        break;
    case State_Merging:
        str = "Merging";
        break;
    case State_Merged:
        str = "Merged";
        break;
    case State_Joining:
        str = "Joining";
        break;
    case State_Takeover:
        str = "Takeover";
        break;
    default:
        str = "Unknown";
    }

    return( str );
}

const char *ZoneTypeString( ZoneType type )
{
    const char *str;

    switch( type )
    {
        case ZoneType_Edge:
            str = "Edge";
            break;
        case ZoneType_Excluded:
            str = "Excluded";
            break;
        case ZoneType_Aggregation:
            str = "Aggregation";
            break;
        case ZoneType_Storage:
            str = "Storage";
            break;
        case ZoneType_Frontend:
            str = "Frontend";
            break;
        case ZoneType_Backend:
            str = "Backend";
            break;
        case ZoneType_Any:
            str = "Any";
            break;
        default:
            str = "Undefined";
            break;
    }

    return( str );
}

char *ErrorMsg (int error_code)
{
    int rc;
    int length;
    static char buffer[MPI_MAX_ERROR_STRING];

    rc = MPI_Error_string (error_code, buffer, &length);
    if (rc != MPI_SUCCESS)
    {
        sprintf(buffer,"MPI_Error_string: Invalid error code (%d)\n", error_code);
        length = strlen(buffer);
    }
    buffer[length] = '\0';

    return buffer;
}

bool check_environment( void )
{
    bool  rs = true;
    bool  isNameServerEnabled = false;
    bool  isAgentModeEnabled = false;
    char* env;
    char  msgString[MAX_BUFFER] = { 0 };
    int   val = 0;

    env = getenv("SQ_MON_RUN_MODE");
    if ( env && (strcmp(env, "AGENT") == 0) )
    {
        isAgentModeEnabled = true;
    }

    if (isAgentModeEnabled)
    {
        env = getenv("MONITOR_COMM_PORT");
        if ( env )
        {
            val = atoi(env);
            if ( val <= 0)
            {
                sprintf( msgString, "[%s] Warning: MONITOR_COMM_PORT value is invalid (%s)!", MyName, env );
                write_startup_log( msgString );
                printf("%s\n", msgString );
            }
        }
    
        env = getenv("MONITOR_SYNC_PORT");
        if ( env )
        {
            val = atoi(env);
            if ( val <= 0)
            {
                sprintf( msgString, "[%s] Warning: MONITOR_SYNC_PORT value is invalid (%s)!", MyName, env );
                write_startup_log( msgString );
                printf("%s\n", msgString );
            }
        }
    }

    env = getenv("SQ_NAMESERVER_ENABLED");
    if ( env )
    {
        val = atoi(env);
        isNameServerEnabled = (val != 0) ? true : false;
    }
    
    if (isNameServerEnabled)
    {
        env = getenv("NS_COMM_PORT");
        if ( env )
        {
            val = atoi(env);
            if ( val <= 0)
            {
                sprintf( msgString, "[%s] Error: Name Server is enabled and NS_COMM_PORT value is invalid (%s)! Set NS_COMM_PORT environment variable and try again.", MyName, env );
                write_startup_log( msgString );
                printf("%s\n", msgString );
                rs = false;
            }
        }
        else
        {
            sprintf( msgString, "[%s] Error: Name Server is enabled and NS_COMM_PORT is undefined! Set NS_COMM_PORT environment variable and try again.", MyName );
            write_startup_log( msgString );
            printf("%s\n", msgString );
            rs = false;
        }

        env = getenv("NS_SYNC_PORT");
        if ( env )
        {
            val = atoi(env);
            if ( val <= 0)
            {
                sprintf( msgString, "[%s] Error: Name Server is enabled and NS_SYNC_PORT value is invalid (%s)! Set NS_SYNC_PORT environment variable and try again.", MyName, env );
                write_startup_log( msgString );
                printf("%s\n", msgString );
                rs = false;
            }
        }
        else
        {
            sprintf( msgString, "[%s] Error: Name Server is enabled and NS_SYNC_PORT is undefined! Set NS_SYNC_PORT environment variable and try again.", MyName );
            write_startup_log( msgString );
            printf("%s\n", msgString );
            rs = false;
        }

        env = getenv("NS_M2N_COMM_PORT");
        if ( env )
        {
            val = atoi(env);
            if ( val <= 0)
            {
                sprintf( msgString, "[%s] Error: Name Server is enabled and NS_M2N_COMM_PORT value is invalid (%s)! Set NS_M2N_COMM_PORT environment variable and try again.", MyName, env );
                write_startup_log( msgString );
                printf("%s\n", msgString );
                rs = false;
            }
        }
        else
        {
            sprintf( msgString, "[%s] Error: Name Server is enabled and NS_M2N_COMM_PORT is undefined! Set NS_M2N_COMM_PORT environment variable and try again.", MyName );
            write_startup_log( msgString );
            printf("%s\n", msgString );
            rs = false;
        }

        env = getenv("MON2MON_COMM_PORT");
        if ( env )
        {
            val = atoi(env);
            if ( val <= 0)
            {
                sprintf( msgString, "[%s] Error: Name Server is enabled and MON2MON_COMM_PORT value is invalid (%s)! Set MON2MON_COMM_PORT environment variable and try again.", MyName, env );
                write_startup_log( msgString );
                printf("%s\n", msgString );
                rs = false;
            }
        }
        else
        {
            sprintf( msgString, "[%s] Error: Name Server is enabled and MON2MON_COMM_PORT is undefined! Set MON2MON_COMM_PORT environment variable and try again.", MyName );
            write_startup_log( msgString );
            printf("%s\n", msgString );
            rs = false;
        }
    }

    return(rs);
}

bool init_pnode_map( void )
{
    CPNodeConfig   *pnodeConfig;
    CPhysicalNode  *physicalNode;
    pair<PhysicalNodeNameMap_t::iterator, bool> pnmit;

    // Make sure it's empty
    if ( !PhysicalNodeMap.empty() )
    {
        PhysicalNodeMap.clear();
    }

    pnodeConfig = ClusterConfig.GetFirstPNodeConfig();
    for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
    {
        // Set initial state of all physical nodes in a real cluster to StateDown
        // update_cluster_state() will set operational state of physical node
        NodeState_t nodeState = StateDown;
        physicalNode = new CPhysicalNode( pnodeConfig->GetFqdn(), nodeState );
        if ( physicalNode )
        {
            pnmit = PhysicalNodeMap.insert( PhysicalNodeNameMap_t::value_type
                                            ( physicalNode->GetName(), physicalNode ));
            if (pnmit.second == false)
            {   // Already had an entry with the given key value.
                printf( "[%s] Error: Internal error while loading physical node map, node name exists, node name=%s\n", MyName, pnodeConfig->GetName() );
                return( false );
            }
        }
        else
        {
            printf( "[%s] Error: Internal error while creating physical node map, failed memory allocation\n", MyName );
            return( false );
        }
    }

    return( true );
}

bool get_pnode_state( const char *name, NodeState_t &state )
{
    CPhysicalNode  *physicalNode;
    PhysicalNodeNameMap_t::iterator it;

    if ( strlen(name) == 0 )
    {
        state = StateDown;
        return( false );
    }

    char pnodename[MPI_MAX_PROCESSOR_NAME];
    strncpy(pnodename, name, MPI_MAX_PROCESSOR_NAME);
    pnodename[MPI_MAX_PROCESSOR_NAME-1] = '\0';

    // Look up name
    it = PhysicalNodeMap.find( pnodename );

    if (it != PhysicalNodeMap.end())
    {
        physicalNode = it->second;
        state = physicalNode->GetState();
        return( true );
    }

    printf( "[%s] Error: Internal error while looking up physical node map, node name does not exist, node name=%s\n", MyName, pnodename );
    return( false );
}

bool set_pnode_state( const char *name, NodeState_t &state )
{
    CPhysicalNode  *physicalNode;
    PhysicalNodeNameMap_t::iterator it;

    char pnodename[MPI_MAX_PROCESSOR_NAME];
    strncpy(pnodename, name, MPI_MAX_PROCESSOR_NAME);
    pnodename[MPI_MAX_PROCESSOR_NAME-1] = '\0';

    // Look up name
    it = PhysicalNodeMap.find( pnodename );

    if (it != PhysicalNodeMap.end())
    {
        physicalNode = it->second;
        physicalNode->SetState( state );
        return( true );
    }

    printf( "[%s] Error: Internal error while looking up physical node map, node name does not exist, node name=%s\n", MyName, pnodename );
    return( false );
}

bool update_cluster_state( bool displayState, bool checkSpareColdStandby = true )
{
    int rc, rc2;
    CCmsh cmshcmd( "sqnodestatus" );

    // save, close and restore stdin when executing ssh command
    // because ssh, by design, would consume contents of stdin.
    int savedStdIn = dup(STDIN_FILENO);
    if ( savedStdIn == -1 )
    {
        fprintf(stderr, "[%s] Error: dup() failed for STDIN_FILENO: %s (%d)\n", MyName, strerror(errno), errno );
        exit(1);
    }
    close(STDIN_FILENO);

    rc = cmshcmd.GetClusterState( PhysicalNodeMap );
    rc2 = dup2(savedStdIn, STDIN_FILENO);
    if ( rc2 == -1 )
    {
        fprintf(stderr, "[%s] Error: dup2() failed for STDIN_FILENO: %s (%d)\n", MyName, strerror(errno), errno );
        exit(1);
    }
    close(savedStdIn);

    if ( rc == -1 )
    {
        return( false );
    }

    NumDown = 0;

    NodeState_t nodeState;
    CPNodeConfig *pnodeConfig = ClusterConfig.GetFirstPNodeConfig();
    if ( pnodeConfig )
    {
        while ( pnodeConfig )
        {
            if ( get_pnode_state( PNode[pnodeConfig->GetPNid()], nodeState ) )
            {
                if ( nodeState == StateUp )
                {
                    if ( checkSpareColdStandby && SpareNodeColdStandby )
                    {
                        if ( pnodeConfig  && pnodeConfig->IsSpareNode() )
                        {
                            ++NumDown;
                            NodeState[pnodeConfig->GetPNid()] = false;
                            nodeState = StateDown;
                            set_pnode_state( PNode[pnodeConfig->GetPNid()], nodeState );
                        }
                        else
                        {
                            NodeState[pnodeConfig->GetPNid()] = true;
                        }
                    }
                    else
                    {
                        NodeState[pnodeConfig->GetPNid()] = true;
                    }
                }
                else
                {
                    NodeState[pnodeConfig->GetPNid()] = false;
                    ++NumDown;
                    if ( displayState )
                    {
                        fprintf(stderr, "[%s] Warning: Node %s is in a down state and is not currently available\n", MyName, PNode[pnodeConfig->GetPNid()] );
                    }
                }
            }
            pnodeConfig = pnodeConfig->GetNext();
        }
    }

    return( true );
}

bool update_node_state( char *nodeName, bool checkSpareColdStandby = true )
{
    if ( strlen(nodeName) == 0 )
    {
        return( false );
    }

    int rc, rc2;
    char pnodename[MPI_MAX_PROCESSOR_NAME];
    CPhysicalNode  *physicalNode;
    PhysicalNodeNameMap_t::iterator it;
    CCmsh cmshcmd( "sqnodestatus" );

    strncpy(pnodename, nodeName, MPI_MAX_PROCESSOR_NAME);
    pnodename[MPI_MAX_PROCESSOR_NAME-1] = '\0';

    // Look up name
    it = PhysicalNodeMap.find( pnodename );

    if (it != PhysicalNodeMap.end())
    {
        physicalNode = it->second;
    }
    else
    {
        printf( "[%s] Error: Internal error while looking up physical node map, node name does not exist, node name=%s\n", MyName, pnodename );
        return( false );
    }

    // save, close and restore stdin when executing ssh command
    // because ssh, by design, would consume contents of stdin.
    int savedStdIn = dup(STDIN_FILENO);
    if ( savedStdIn == -1 )
    {
        fprintf(stderr, "[%s] Error: dup() failed for STDIN_FILENO: %s (%d)\n", MyName, strerror(errno), errno );
        exit(1);
    }
    close(STDIN_FILENO);

    rc = cmshcmd.GetNodeState( nodeName, physicalNode );
    rc2 = dup2(savedStdIn, STDIN_FILENO);
    if ( rc2 == -1 )
    {
        fprintf(stderr, "[%s] Error: dup2() failed for STDIN_FILENO: %s (%d)\n", MyName, strerror(errno), errno );
        exit(1);
    }
    close(savedStdIn);

    if ( rc == -1 )
    {
        return( false );
    }

    NodeState_t nodeState;
    CPNodeConfig *pnodeConfig = ClusterConfig.GetPNodeConfig( nodeName );
    if ( pnodeConfig )
    {
        if ( get_pnode_state( PNode[pnodeConfig->GetPNid()], nodeState ) )
        {
            if ( nodeState == StateUp )
            {
                if ( checkSpareColdStandby && SpareNodeColdStandby )
                {
                    if ( pnodeConfig  && pnodeConfig->IsSpareNode() )
                    {
                        ++NumDown;
                        NodeState[pnodeConfig->GetPNid()] = false;
                        nodeState = StateDown;
                        set_pnode_state( PNode[pnodeConfig->GetPNid()], nodeState );
                    }
                    else
                    {
                        NodeState[pnodeConfig->GetPNid()] = true;
                    }
                }
                else
                {
                    NodeState[pnodeConfig->GetPNid()] = true;
                }
            }
            else
            {
                NodeState[pnodeConfig->GetPNid()] = false;
                ++NumDown;
            }
        }
    }
    else
    {
        printf( "[%s] Physical node configuration does not exist, node name=%s\n", MyName, nodeName );
        return( false );
    }

    return( true );
}

int mon_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
{
    pv_event_type = pv_event_type;
    pv_severity = pv_severity;
    int lv_err = 0;

    fprintf(stderr,"[%s] %s", MyName, pp_string );

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        trace_printf("Evlog event: type=%d, severity=%d, text: %s",
                     pv_event_type, pv_severity, pp_string);
    }

    return lv_err;
}


// A shell tracing facility provides a way of troubleshooting operational
// problems.
//
// Trace output:
//    By default trace output appears in a file named "shell.trace.<pid>"
//    where <pid> is the process id of the shell.   The file is created
//    in the directory given in environment variable $MPI_TMPDIR which
//    is typically the "tmp" directory of the Seaquest environment.
//
//    By setting the SHELL_TRACE_FILE environment variable one can
//    direct trace output to other locations.   Setting SHELL_TRACE_FILE
//    to STDOUT or STDERR directs the trace output to the shell's standard
//    output or standard error files respectively.   Setting SHELL_TRACE_FILE
//    to any other value directs the trace output to a file of the specified
//    name.
//
// Trace buffer size:
//    The default trace buffer size is sufficient in most cases.  The
//    trace buffer is an in memory region where trace output is
//    accumulated before being written to a file or device. To alter
//    the trace buffer size set the environment variable
//    SHELL_TRACE_FILE_FB to the desired value.
//
// Shell tracing options.
//
// Specifying trace options via environment variables.  Prior to starting
// the shell the following environment variables may be set to a non-zero
// value.  Each option controls a different trace area:
//    SHELL_TRACE_CMD          enables tracing for shell commands
//    SHELL_TRACE_INIT         enables tracing for initialization processing
//    SHELL_TRACE_LIO          enables tracing for local io
//    SHELL_TRACE_ENTRY_EXIT   enables tracing for function entry/exit
//
// Specifying trace options via command line argument.  A trace command
// line argument "-t <number>" can be used to enable tracing.  This is an
// alternative to using environment variables.  The specified number
// is a bit mask that enables tracing for different trace areas.  The
// bit mask values and the enabled trace areas are:
//    1  enables tracing for shell commands
//    2  enables tracing for initialization processing
//    4  enables tracing for local io
//    8  enables tracing for function entry/exit
// Multiple trace areas may be enabled by summing the values for the
// individual trace areas.  For example the value 5 enables tracing for
// shell commands and for local io.
//
// Specifying trace options dynamically.  Tracing can be enabled and
// disabled dynamically by using the shell's "trace" command.  This
// command takes a single numeric argument which specifies the trace
// areas of interest.  The number has the same meaning as for the "-t"
// command line argument described above.  Using zero as the numeric
// value disables tracing.
//

void TraceOpen ( void )
{
    // Initialize tracing
    trace_init(traceFileName,
               false,  // don't append pid to file name
               (char *)"shell",  // prefix
               false);
    if (traceFileFb > 0)
    {
        trace_set_mem(traceFileFb);
    }
    traceOpen = true;
}

void TraceUpdate ( int flags )
{
    if ( flags & 1 )
    {
        trace_settings |= TRACE_SHELL_CMD;
    }
    if ( flags & 2 )
    {
        trace_settings |= TRACE_INIT;
        trace_settings |= TRACE_TRAFCONFIG;
    }
    if ( flags & 4 )
    {
        trace_settings |= TRACE_MLIO;
        Local_IO_To_Monitor::cv_trace = true;
        Local_IO_To_Monitor::cp_trace_cb = trace_where_vprintf;
    }
    if ( flags & 8 )
    {
        trace_settings |= TRACE_ENTRY_EXIT;
    }
}

void TraceInit( int & argc, char **&argv )
{
    // Determine trace file name
    const char *tmpDir;
    tmpDir = getenv( "TRAF_LOG" );

    const char *envVar;
    envVar = getenv("SHELL_TRACE_FILE");
    if (envVar != NULL)
    {
        if ((strcmp(envVar, "STDOUT") == 0)
          || strcmp(envVar, "STDERR") == 0)
        {
            strcpy( traceFileName, envVar);
        }
        else if (tmpDir)
        {
            sprintf( traceFileName, "%s/%s.%d", tmpDir, envVar, getpid() );
        }
        else
        {
            sprintf( traceFileName, "./%s.%d", envVar, getpid() );
        }

    }
    else
    {   // No trace file name specified, use default name
        if (tmpDir)
        {
            sprintf( traceFileName, "%s/shell.trace.%d", tmpDir, getpid() );
        }
        else
        {
            sprintf( traceFileName, "./shell.trace.%d", getpid() );
        }
    }

    // Get trace settings from environment variables
    trace_settings = 0;
    envVar = getenv("SHELL_TRACE_CMD");
    if (envVar && atoi (envVar) != 0 )
    {
        trace_settings |= TRACE_SHELL_CMD;
    }

    envVar = getenv("SHELL_TRACE_INIT");
    if (envVar && atoi (envVar) != 0 )
    {
        trace_settings |= TRACE_INIT;
        trace_settings |= TRACE_TRAFCONFIG;
    }

    envVar = getenv("SHELL_TRACE_LIO");
    if (envVar && atoi (envVar) != 0 )
    {
        trace_settings |= TRACE_MLIO;
        Local_IO_To_Monitor::cv_trace = true;
        Local_IO_To_Monitor::cp_trace_cb = trace_where_vprintf;
    }

    envVar = getenv("SHELL_TRACE_ENTRY_EXIT");
    if (envVar && atoi (envVar) != 0 )
    {
        trace_settings |= TRACE_ENTRY_EXIT;
    }

    // Get environment variable value for trace buffer size if specified
    envVar = getenv("SHELL_TRACE_FILE_FB");
    if (envVar)
    {
        traceFileFb = atoi ( envVar );
    }

    // Check for trace flags specified on the command line.
    int flags;
    for (int i = 0; i < argc; i++)
    {
        if ( strcmp ( argv[i], "-t" ) == 0 && (i != argc-1) )
        {   // Trace flag setting specified on command line.
            flags = strtol(argv[i+1], NULL, 0);
            TraceUpdate ( flags );

            // Remove the trace flag arguments from the list of command
            // line arguments.
            for (int j=i, k=i+2; k < argc; j++, k++)
            {
                //printf ("setting argv[%d] = argv[%d]\n", j, k);
                argv[j] = argv[k];
            }
            argc -= 2;
        }
    }

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        printf("traceFileName=%s, trace_settings=%lX, traceFileFb=%d\n",
               traceFileName, trace_settings, traceFileFb);
    }

    // Initialize tracing if any trace flags set
    if ( trace_settings )
    {
        TraceOpen();
    }
}

void VirtualNidInit( int & argc, char **&argv )
{
    // Check for trace flags specified on the command line.
    for (int i = 0; i < argc; i++)
    {
        if ( strcmp ( argv[i], "-nid" ) == 0 && (i != argc-1) )
        {   // <nid> setting specified on command line.
            VirtualNid = atoi ( argv[i+1] );

            // Remove the virtual nid arguments from the list of command
            // line arguments.
            for (int j=i, k=i+2; k < argc; j++, k++)
            {
                //printf ("setting argv[%d] = argv[%d]\n", j, k);
                argv[j] = argv[k];
            }
            argc -= 2;
        }
    }

    if (VirtualNid != -1)
    {
        printf( "Using VirtualNid=%d\n", VirtualNid );
    }
}

void RedirectFd(int orig_fd, char *fifo_name)
{
    int rdir_fd;
    const char method_name[] = "RedirectFd";

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] opening (%s) for writing, "
                     "fd=%d remapped to this pipe/file\n",
                     method_name, __LINE__, MyName, fifo_name, orig_fd);

    // Open for writing.
    rdir_fd = open( fifo_name, O_CREAT | O_TRUNC | O_WRONLY,
                    S_IRUSR | S_IWUSR );
    if (rdir_fd == -1)
    {
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s], fifo open(%s) error, %s.\n",
                         method_name, __LINE__, MyName, fifo_name,
                         strerror(errno));
        return;
    }

    // Remap file descriptor to desired file descriptor.
    // Close unneeded file descriptor.
    if (close(orig_fd))
    {
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s], close(%d) error, %s.\n",
                         method_name, __LINE__, MyName, orig_fd,
                         strerror(errno));
    }

    if ( dup2(rdir_fd, orig_fd) == -1)
    {
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s], dup2(%d, %d) error, %s.\n",
                         method_name, __LINE__, MyName, rdir_fd, orig_fd,
                         strerror(errno));
    }

    if (close(rdir_fd))
    {
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s], close(%d) error, %s.\n",
                         method_name, __LINE__, MyName, rdir_fd,
                         strerror(errno) );
    }
}

bool attach( int nid, char *name, char *program )
{
    bool attached = false;
    int count;
    MPI_Status status;
    struct message_def *saved_msg = msg;
    const char method_name[] = "attach";

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        if (gp_local_mon_io)
        {
            trace_printf("%s@%d [%s] locio: %p, nid: %d:%d, initted: %d\n",
                         method_name, __LINE__, MyName,
                         (void *)gp_local_mon_io, MyNid, nid,
                         gp_local_mon_io->iv_initted);
        }
        else
        {
            trace_printf("%s@%d [%s] locio: %p, nid: %d:%d\n",
                         method_name, __LINE__, MyName,
                         (void *)gp_local_mon_io, MyNid, nid);
        }
    }
    if (gp_local_mon_io)
    {
        if (gp_local_mon_io->init_comm())
        {
            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf("%s@%d [%s] new comm must send startup\n",
                             method_name, __LINE__, MyName);

            if (gp_local_mon_io->iv_monitor_down)
                return false;
        }
        else
        {
            if (gp_local_mon_io->iv_monitor_down)
                return false;
            MonitorNid = nid;
            msg = saved_msg;
            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf("%s@%d [%s] monitor already attached on node %d\n",
                             method_name, __LINE__, MyName, nid );
            return true;
        }
    }

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] Attaching to monitor on nid=%d\n",
                     method_name, __LINE__, MyName, nid);

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return attached;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;                         // attach needs reply
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Startup;
    STRCPY (msg->u.request.u.startup.port_name, MyPort);
    STRCPY (msg->u.request.u.startup.program, program);
    msg->u.request.u.startup.os_pid = getpid ();
    msg->u.request.u.startup.verifier = MyVerifier;
    msg->u.request.u.startup.event_messages = true;
    msg->u.request.u.startup.system_messages = true;
    if ( strcmp(name, "SHELL") == 0 )
    {
        msg->u.request.u.startup.nid = -1;        // signals attach
        msg->u.request.u.startup.pid = -1;        // signals attach
        msg->u.request.u.startup.paired = false;
        msg->u.request.u.startup.process_name[0] = '\0';
        if (gp_local_mon_io)
        {
            ((SharedMsgDef *)msg)->trailer.attaching = true;
        }
    }
    else
    {
        msg->u.request.u.startup.nid = MyNid;
        msg->u.request.u.startup.pid = MyPid;
        msg->u.request.u.startup.paired = true;
        STRCPY(msg->u.request.u.startup.process_name, name);
    }
    msg->u.request.u.startup.startup_size = sizeof(msg->u.request.u.startup);

    gp_local_mon_io->send_recv( msg );
    status.MPI_TAG = msg->reply_tag;
    count = sizeof( *msg );

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Startup))
        {
            if (msg->u.reply.u.startup_info.return_code == MPI_SUCCESS)
            {
                if ( strcmp(name, "SHELL") == 0 )
                {
                    MyNid = msg->u.reply.u.startup_info.nid;
                }
                MyPid = msg->u.reply.u.startup_info.pid;
                strcpy (MyName, msg->u.reply.u.startup_info.process_name);
                gv_ms_su_pid = MyPid;
                gv_ms_su_verif = MyVerifier = msg->u.reply.u.startup_info.verifier;
                if (gp_local_mon_io)
                {
                    gp_local_mon_io->iv_pid = MyPid;
                    gp_local_mon_io->iv_verifier = MyVerifier;
                }
                if ( MonitorNid == -1 )
                {
                    // set the monitor to the first connection monitor process
                    MonitorNid = nid;
                }
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf("%s@%d [%s] Attached to local monitor nid="
                                 "%d\n", method_name, __LINE__, MyName, MyNid );
                attached = true;
                // Connect to monitor via pipes and remap stderr
                RedirectFd(2, msg->u.reply.u.startup_info.fifo_stderr);
            }
            else if (msg->u.reply.u.startup_info.return_code == MPI_ERR_NO_MEM)
            {
                printf("[%s] Monitor rejected attach, physical node is a spare node\n", MyName);
                NodeState[nid]=false;
            }
            else
            {
                printf("[%s] Monitor rejected attach, error=%s\n",
                        MyName, ErrorMsg(msg->u.reply.u.startup_info.return_code));
                NodeState[nid]=false;
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for Process "
                   "attach message, msg->type=%d, expected=%d, reply.type=%d,"
                   " expected=%d\n",
                   MyName, msg->type, msg->u.reply.type,
                   msg->type,  MsgType_Service,
                   msg->u.reply.type,  ReplyType_Startup);
        }
    }
    else
    {
        printf ("[%s] Process attach reply invalid.\n", MyName);
    }

    if (gp_local_mon_io)
        gp_local_mon_io->release_msg(msg);
    msg = saved_msg;
    return attached;
}

const char *join_phase_string( JOINING_PHASE phase )
{
    const char *str;

    switch( phase )
    {
    case JoiningPhase_1:
        str = "join phase starting.";
        break;
    case JoiningPhase_2:
        str = "join phase in progress.";
        break;
    case JoiningPhase_3:
        str = "join phase completed.";
        break;
    default:
        str = "join phase unknown!";
    }

    return( str );
}

void nodePendingComplete()
{
    nodePendingLock.lock();
    nodePendingLock.wakeOne();
    nodePendingLock.unlock();
}

void waitDeathComplete()
{
    waitDeathLock.lock();
    waitDeathPending = false;
    waitDeathLock.wakeOne();
    waitDeathLock.unlock();
}

void recv_notice_msg(struct message_def *recv_msg, int )
{
    switch (recv_msg->type )
    {
    case MsgType_Change:
        if (! QuietShell) printf ("[%s] %s - Configuration Change Notice for Group: %s Key: %s Value: %s\n", 
                                  MyName, time_string(),
                                  recv_msg->u.request.u.change.group,
                                  recv_msg->u.request.u.change.key,
                                  recv_msg->u.request.u.change.value);
        break;

    case MsgType_Event:
        if (! QuietShell) printf("[%s] %s - Event %d received\n",
                                 MyName, time_string(), recv_msg->u.request.u.event_notice.event_id);
        break;
    
    case MsgType_NodeAdded:
        if (! QuietShell) printf ("[%s] %s - Node %d (%s) ADDED to configuration\n", 
                                  MyName, time_string(), recv_msg->u.request.u.node_added.nid,
                                  recv_msg->u.request.u.node_added.node_name);
        if ( !load_configuration() )
        {
            exit (1);
        }
        if ( !init_pnode_map() )
        {
            exit(1);
        }
        if ( nodePending )
        {
            if ( strcmp( nodePendingName, recv_msg->u.request.u.node_added.node_name) == 0 )
            {
                nodePendingComplete();
            }
        }
        break;

    case MsgType_NodeChanged:
        if (! QuietShell) printf ("[%s] %s - Node %d (%s) CHANGED in configuration\n", 
                                  MyName, time_string(), recv_msg->u.request.u.node_changed.nid,
                                  recv_msg->u.request.u.node_changed.node_name);
        if ( !load_configuration() )
        {
            exit (1);
        }
        if ( !init_pnode_map() )
        {
            exit(1);
        }
        if ( nodePending )
        {
            if ( strcmp( nodePendingName, recv_msg->u.request.u.node_changed.node_name) == 0 )
            {
                nodePendingComplete();
            }
        }
        break;

    case MsgType_NodeDeleted:
        if (! QuietShell) printf ("[%s] %s - Node %d (%s) DELETED from configuration\n", 
                                  MyName, time_string(), recv_msg->u.request.u.node_deleted.nid,
                                  recv_msg->u.request.u.node_deleted.node_name);
        if ( !load_configuration() )
        {
            exit (1);
        }
        if ( !init_pnode_map() )
        {
            exit(1);
        }
        if ( nodePending )
        {
            if ( strcmp( nodePendingName, recv_msg->u.request.u.node_deleted.node_name) == 0 )
            {
                nodePendingComplete();
            }
        }
        break;

    case MsgType_NodeDown:
        if (! QuietShell) printf ("[%s] %s - Node %d (%s) is DOWN\n", 
                                  MyName, time_string(), recv_msg->u.request.u.down.nid,
                                  recv_msg->u.request.u.down.node_name );
        NodeState[recv_msg->u.request.u.down.nid] = false;

        if ( nodePending )
        {
            if ( VirtualNodes )
            {
                if (recv_msg->u.request.u.down.nid == nodePendingNid)
                {
                    nodePendingComplete();
                }
            }
            else
            {
                if ( strcmp( nodePendingName, recv_msg->u.request.u.down.node_name) == 0 )
                {
                    nodePendingComplete();
                }
            }
        }
        if ( waitDeathPending )
        {
            if ( recv_msg->u.request.u.down.nid == waitDeathNid )
            {   // Node is down so process is down too.
                waitDeathComplete();
            }
        }

        break;


    case MsgType_NodeJoining:
        if (! QuietShell) printf ("[%s] %s - Node %s %s\n"
                                  , MyName
                                  , time_string()
                                  , recv_msg->u.request.u.joining.node_name 
                                  , join_phase_string(recv_msg->u.request.u.joining.phase) );
        break;

    case MsgType_NodeQuiesce:
        if (! QuietShell) printf ("[%s] %s - Node %d (%s) is QUIESCEd\n", 
                                  MyName, time_string(), msg->u.request.u.quiesce.nid,
                                  msg->u.request.u.quiesce.node_name );
        NodeState[msg->u.request.u.quiesce.nid] = false;
        if ( waitDeathPending )
        {
            if ( msg->u.request.u.quiesce.nid == waitDeathNid )
            {   // Node is quiesced so process is down too.
                waitDeathComplete();
            }
        }
        break;

    case MsgType_NodeUp:
        if (! QuietShell) printf ("[%s] %s - Node %d (%s) is UP\n", 
                                  MyName, time_string(), recv_msg->u.request.u.up.nid,
                                  recv_msg->u.request.u.up.node_name);
        NodeState[recv_msg->u.request.u.down.nid] = true;        
        if ( nodePending )
        {
            if ( strcmp( nodePendingName, recv_msg->u.request.u.up.node_name) == 0 )
            {
                nodePendingComplete();
            }
        }
        break;

    case MsgType_ProcessCreated:
        if ( recv_msg->u.request.u.process_created.return_code == MPI_SUCCESS )
        {
            if (! QuietShell) printf ("[%s] %s - Process %s successfully created. Nid=%d, Pid=%d\n",
                                      MyName, time_string(), recv_msg->u.request.u.process_created.process_name,
                                      recv_msg->u.request.u.process_created.nid,
                                      recv_msg->u.request.u.process_created.pid);
        }
        else
        {
            if (! QuietShell) printf ("[%s] %s - Process %s NOT created. Nid=%d, Pid=%d\n",
                                      MyName, time_string(), recv_msg->u.request.u.process_created.process_name,
                                      recv_msg->u.request.u.process_created.nid,
                                      recv_msg->u.request.u.process_created.pid);
        }
        break;

    case MsgType_ProcessDeath:
        if ( recv_msg->u.request.u.death.aborted )
        {
            if (! QuietShell) printf ("[%s] %s - Process %s abnormally terminated. Nid=%d, Pid=%d\n",
                                      MyName, time_string(), recv_msg->u.request.u.death.process_name, 
                                      recv_msg->u.request.u.death.nid,
                                      recv_msg->u.request.u.death.pid);
        }
        else
        {
            if (! QuietShell) printf ("[%s] %s - Process %s terminated normally. Nid=%d, Pid=%d\n", 
                                      MyName, time_string(), recv_msg->u.request.u.death.process_name, 
                                      recv_msg->u.request.u.death.nid,
                                      recv_msg->u.request.u.death.pid);
        }
        for ( int dinx = 0; dinx < MAX_DEATH_SAVE; dinx++ )
        {
            if ( lastDeathNid[dinx] == -1 )
            {
                lastDeathNid[dinx] = recv_msg->u.request.u.death.nid;
                lastDeathPid[dinx] = recv_msg->u.request.u.death.pid;
                break;
            }
        }
        if ( waitDeathPending )
        {
            if ( recv_msg->u.request.u.death.nid == waitDeathNid
              && recv_msg->u.request.u.death.pid == waitDeathPid )
            {
                waitDeathComplete();
            }
        }
        break;

    case MsgType_SpareUp:
        if (! QuietShell) printf ("[%s] %s - Node %s is Spare Node and available\n"
                                  , MyName
                                  , time_string()
                                  , recv_msg->u.request.u.spare_up.node_name );
        nodePendingComplete();
        break;

    case MsgType_Shutdown:
        if (! QuietShell) printf("[%s] %s - Shutdown notice, level=%d received\n",
                                 MyName, time_string(), recv_msg->u.request.u.shutdown.level);
        nodePendingComplete();
        break;
    case MsgType_ReintegrationError:
        if (! QuietShell) printf ("[%s] %s - %s\n"
                                  , MyName
                                  , time_string()
                                  , recv_msg->u.request.u.reintegrate.msg );
        nodePendingComplete();
        break;

    default:
        if (! QuietShell) printf("[%s] %s - Unexpected notice type(%d) received\n",
                                 MyName, time_string(), recv_msg->type);

    }

    if (! QuietShell) printf( "%s", prompt );
    fflush( stdout );
}

void interrupt_handler(int signal, siginfo_t *info, void *)
{
    const char method_name[] = "interrupt_handler";

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d - signal=%d, code=%d, status=%d, pid=%d\n",
                            method_name, __LINE__, signal, info->si_code,
                            info->si_status, info->si_pid);

    BreakCmd = true;
    int rc = gp_local_mon_io->signal_cv( EINTR );
    if ( rc != 0 )
    {
        printf ("[%s] Failed signaling notice completion!\n", MyName );
    }

    if (nodePending)
    {
        nodePendingComplete();
    }
}

bool is_spare_node( char *node_name )
{
    bool rs = false; // Assume it's not a spare node
    CPNodeConfig *pNodeConfig;

    // Any spare nodes in configuration?
    if ( ClusterConfig.GetSNodesCount() )
    {
        pNodeConfig =
            ClusterConfig.GetPNodeConfig( ClusterConfig.GetPNid( node_name ) );
        if ( pNodeConfig )
        {
            rs = pNodeConfig->IsSpareNode();
        }
    }

    return( rs );
}

bool is_environment_up( void )
{
    char tempCStatFileName[PATH_MAX];
    char tempMonStatFileName[PATH_MAX];
    char tempString[PATH_MAX] = { 0 };
    int count = 0;
    int tempCStatFd;
    int tempMonStatFd;
    bool up = false;

    char *envCheck = getenv( "SQ_MON_ENV_CHECK_DISABLE" );
    if (envCheck)
    {
        // Don't check for running environment, just say it's down
        printf ("[%s] Environment check is disabled!\n", MyName );
        return( false );
    }


    CUtility getStat( "cstat" );
    CUtility getMonStat( "grep monitor" );
    CUtility getMonProcCount( "cat" );

    char *tmpDir = getenv( "MPI_TMPDIR" );
    if (tmpDir)
    {
        sprintf( tempCStatFileName, "%s/tempCStat_XXXXXX", tmpDir );
        sprintf( tempMonStatFileName, "%s/tempMonStat_XXXXXX", tmpDir );
    }
    else
    {
        sprintf( tempCStatFileName, "./tempCStat_XXXXXX" );
        sprintf( tempMonStatFileName, "./tempMonStat_XXXXXX" );
    }

    // stdout, swap stdout with tempCStatFile
    int savedStdOut = dup(STDOUT_FILENO);
    close(STDOUT_FILENO);
    tempCStatFd = mkstemp( tempCStatFileName );
    if ( tempCStatFd == -1 )
    {
        printf ("[%s] Error= Can't create '%s' temporary file! (%s)\n", MyName, tempCStatFileName, strerror(errno) );
        dup2(savedStdOut, STDOUT_FILENO);
        close(savedStdOut);

        // Assume environment is up until environmental problem causing this
        // error is resolved
        return( true );
    }

    // stdin, save (cstat uses ssh so we need to save stdin)
    int savedStdIn = dup(STDIN_FILENO);
    close(STDIN_FILENO);
    int newStdinFd = open("/dev/null", O_RDWR);
    if (newStdinFd == -1 || newStdinFd != 0 )
    {
        printf ("[%s] Error, can't open temporary stdin, fd=%d (%s)\n",
                MyName, newStdinFd, strerror(errno) );

        dup2(savedStdOut, STDOUT_FILENO);
        close(savedStdOut);

        // Assume environment is up until environmental problem causing this
        // error is resolved
        return( true );
    }

    // get environment process status
    int rc = getStat.ExecuteCommand( tempString );
    if ( rc )
    {
        printf ("[%s] Error= Can't execute 'cstat' command!\n",MyName);
        if ( savedStdIn != -1 )
        {
            dup2(savedStdIn, STDIN_FILENO);
            close(savedStdIn);
        }
        if ( savedStdOut != -1 )
        {
            dup2(savedStdOut, STDOUT_FILENO);
            close(savedStdOut);
        }
        close(tempCStatFd);
        unlink( tempCStatFileName );

        // Assume environment is up until environmental problem causing this
        // error is resolved
        return( true );
    }

    // save stdin close, and restore stdin
    dup2(savedStdIn, STDIN_FILENO);
    close(savedStdIn);

    // stdout, swap tempCStat with tempMonStat
    close(tempCStatFd);
    tempMonStatFd = mkstemp( tempMonStatFileName );
    if ( tempMonStatFd == -1 )
    {
        printf ("[%s] Error= Can't create '%s' temporary file! (%s)\n", MyName, tempCStatFileName, strerror(errno) );
        dup2(savedStdOut, STDOUT_FILENO);
        close(savedStdOut);
        unlink( tempCStatFileName );

        // Assume environment is up until environmental problem causing this
        // error is resolved
        return( true );
    }


    // get the monitor process status in the environment
    sprintf( tempString, "%s", tempCStatFileName );
    rc = getMonStat.ExecuteCommand( tempString );
    if ( rc != 0 && rc != 1 )
    {
        printf ("[%s] Error= 'grep' command failed!\n",MyName);
        close(tempMonStatFd);
        dup2(savedStdOut, STDOUT_FILENO);
        close(savedStdOut);
        unlink( tempCStatFileName );
        unlink( tempMonStatFileName );

        // Assume environment is up until environmental problem causing this
        // error is resolved
        return( true );
    }

    // stdout, swap tempMonStat with stdout
    close(tempMonStatFd);
    dup2(savedStdOut, STDOUT_FILENO);
    close(savedStdOut);

    rc = getMonProcCount.ExecuteCommand( tempMonStatFileName );
    if ( rc )
    {
        printf ("[%s] Error= Can't execute 'cat' command!\n",MyName);
        unlink( tempCStatFileName );
        unlink( tempMonStatFileName );

        // Assume environment is up until environmental problem causing this
        // error is resolved
        return( true );
    }

    if ( getMonProcCount.HaveOutput() )
    {
#if 1
        count = getMonProcCount.GetOutputLineCount();
#else
        getMonProcCount.GetOutput( buffer, sizeof(buffer) );
        count = atoi( buffer );
#endif
        if ( count > 0 )
        {
            up = true;
        }
    }

    unlink( tempCStatFileName );
    unlink( tempMonStatFileName );

    //printf ("[%s] %d monitor processes are running!\n",MyName, count);

    return( up );
}

bool is_node_up( int nid )
{
    char node_name[MAX_TOKEN] = { 0 };
    int lv_nid = nid;
    int pnid;
    int zid = -1;
    STATE state;

    if ( !get_zone_state( lv_nid, zid, node_name, pnid, state ) )
    {
        return( false );
    }

    return( (state == State_Up) ? true : false );
}

void exit_process (void)
{
    int count;
    MPI_Status status;
    const char method_name[] = "exit_process";

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] sending exit process message.\n",
                     method_name, __LINE__, MyName);

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Exit;
    msg->u.request.u.exit.nid = MyNid;
    msg->u.request.u.exit.pid = MyPid;
    msg->u.request.u.exit.verifier = MyVerifier;
    msg->u.request.u.exit.process_name[0] = 0;

    gp_local_mon_io->send_recv( msg );
    if (gp_local_mon_io->iv_shutdown)
    {
        gp_local_mon_io->release_msg(msg);
        return;
    }
    status.MPI_TAG = msg->reply_tag;
    count = sizeof( *msg );

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf("%s@%d [%s] exited process successfully. "
                                 "error=%s\n", method_name, __LINE__, MyName,
                                 ErrorMsg(msg->u.reply.u.generic.return_code));
            }
            else
            {
                printf ("[%s] Exit process failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.generic.return_code));
            }
        }
        else
        {
            printf ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
                MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Exit process reply invalid, msg tag is %d, count= %d. \n", MyName, status.MPI_TAG, count);
    }
    if (gp_local_mon_io)
        gp_local_mon_io->release_msg(msg);
}

bool env_replace (char *cmd)
{
    int new_len = 0;
    int len;
    char *pb;
    char *pe;
    char *env;
    char temp[MAX_BUFFER];
    char var[MAX_BUFFER];

    while (*cmd)
    {
        if ((cmd[0] == '$') && (cmd[1] == '{'))
        {
            pb = &cmd[2];
            pe = pb;
            while ((*pe) && (*pe != '}'))
            {
                pe++;
            }
            if (*pe == '\0')
            {
                printf
                    ("[%s] Macro terminator '}' for ${%s} is missing - no expansion\n",
                     MyName,pb);
                break;
            }
            len = (int) (pe - pb);
            STRCPY (temp, &pe[1]);
            if ( ( size_t ) len > ( sizeof ( var ) - 1 ) )
            {   // Not expected to occur but guard against buffer overrun
                len = sizeof ( var ) - 1;
            }
            memcpy (var, pb, len);
            var[len] = '\0';
            env = getenv (var);
            if (env != NULL)
            {
                len = (int) strlen (env);
                if ((new_len + strlen (temp) + len + 1) > MAX_BUFFER)
                {
                    printf ("[%s] Replacement buffer > MAX_BUFFER\n",
                            MyName);
                    break;
                }
                strcpy (cmd, env);
            }
            else
            {
                len = 0;
            }
            strcpy (&cmd[len], temp);
        }
        else
        {
            cmd++;
            new_len++;
        }
    }

    return false;
}

char *find_delimiter(char *cmd, bool isEqDelim, bool isDashDelim)
{
    char *ptr = cmd;

    while (ptr
       && *ptr
       && *ptr != ' '
       && *ptr != ','
       && *ptr != ';'
       && *ptr != '{'
       && *ptr != '}'
       && *ptr != ':')
    {
        if (*ptr == '-' && isDashDelim)
        {
            break;
        }
        if (*ptr == '=' && isEqDelim)
        {
            break;
        }
        ptr++;
    }

    return ptr;
}

char *find_end_of_token (char *cmd, int maxlen, bool isEqDelim, bool isDashDelim)
{
    int length = 0;
    char *ptr = cmd;

    while (ptr
       && *ptr
       && *ptr != ' '
       && *ptr != ','
       && *ptr != '{'
       && *ptr != '}'
       && *ptr != ':')
    {
        if (*ptr == '-' && isDashDelim)
        {
            break;
        }
        if (*ptr == '=' && isEqDelim)
        {
            break;
        }
        length++;
        ptr++;
        if (length == maxlen)
        {
            break;
        }
    }

    return ptr;
}

bool find_DTM(void)
{
    int count;
    bool ret = false;
    MPI_Status status;
    PROCESSTYPE process_type = ProcessType_DTM;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return ret;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfo;
    msg->u.request.u.process_info.nid = MyNid;
    msg->u.request.u.process_info.pid = MyPid;
    msg->u.request.u.process_info.verifier = MyVerifier;
    msg->u.request.u.process_info.process_name[0] = 0;
    msg->u.request.u.process_info.target_nid = -1;
    msg->u.request.u.process_info.target_pid = -1;
    msg->u.request.u.process_info.target_verifier = -1;
    msg->u.request.u.process_info.target_process_name[0] = 0;
    msg->u.request.u.process_info.target_process_pattern[0] = 0;
    msg->u.request.u.process_info.type = process_type;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {

        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_ProcessInfo))
        {
            if (msg->u.reply.u.process_info.return_code == MPI_SUCCESS)
            {
                if (msg->u.reply.u.process_info.num_processes > 0)
                    ret = true;
            }
            else
            {
                printf ("[%s] ProcessInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.process_info.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessInfo message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] ProcessInfo reply message invalid, msg tag is %d, count= %d. \n", MyName, status.MPI_TAG, count);
    }

    gp_local_mon_io->release_msg(msg);
    return ret;
}

bool find_process( char *process_name )
{
    int count;
    bool ret = false;
    MPI_Status status;
    PROCESSTYPE process_type = ProcessType_Undefined;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return ret;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfo;
    msg->u.request.u.process_info.nid = MyNid;
    msg->u.request.u.process_info.pid = MyPid;
    msg->u.request.u.process_info.verifier = MyVerifier;
    msg->u.request.u.process_info.process_name[0] = 0;
    msg->u.request.u.process_info.target_nid = -1;
    msg->u.request.u.process_info.target_pid = -1;
    msg->u.request.u.process_info.target_verifier = -1;
    msg->u.request.u.process_info.target_process_name[0] = 0;
    msg->u.request.u.process_info.target_process_pattern[0] = 0;
    STRCPY (msg->u.request.u.process_info.target_process_name, process_name);
    msg->u.request.u.process_info.type = process_type;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {

        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_ProcessInfo))
        {
            if (msg->u.reply.u.process_info.return_code == MPI_SUCCESS)
            {
                if (msg->u.reply.u.process_info.num_processes > 0)
                {
                    ret = true;
                }
            }
            else
            {
                printf ("[%s] ProcessInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.process_info.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessInfo message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] ProcessInfo reply message invalid, msg tag is %d, count= %d. \n", MyName, status.MPI_TAG, count);
    }

    gp_local_mon_io->release_msg(msg);
    return ret;
}

void get_event (int event_id)
{
    bool done = false;
    int count;
    MPI_Status status;
    const char method_name[] = "get_event";

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] waiting for event %d message.\n",
                     method_name, __LINE__, MyName, event_id);
    do
    {
        gp_local_mon_io->wait_for_event( &msg );
        assert( msg );
        status.MPI_TAG = msg->reply_tag;
        count = sizeof( *msg );

        if ((status.MPI_TAG == EVENT_TAG           ) &&
            (count == sizeof (struct message_def)  ) &&
            (msg->u.request.type == ReqType_Notice ) &&
            (msg->type == MsgType_Event            )   )
        {
          printf("[%s] Event %d (%s) received\n",
                MyName,
                msg->u.request.u.event_notice.event_id,
                msg->u.request.u.event_notice.data);
            if (event_id == -1
             || event_id == msg->u.request.u.event_notice.event_id)
            {
                done = true;
            }
            else
            {
                sleep(1);
            }
        }
        else
        {
            printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                 MyName, msg->type, msg->noreply, msg->u.request.type);
            done = true;
        }

        gp_local_mon_io->release_msg( msg );
    }
    while (!done);
}

int get_node_name_by_nid( int nid, char *node_name )
{
    int pnid;

    CPNodeConfig   *pnodeConfig;
    CLNodeConfig   *lnodeConfig;

    lnodeConfig = ClusterConfig.GetLNodeConfig( nid );
    if ( !lnodeConfig )
    {
        return( -1 );
    }
    pnodeConfig = lnodeConfig->GetPNodeConfig();
    if ( !pnodeConfig )
    {
        return( -1 );
    }
    strcpy( node_name, pnodeConfig->GetFqdn() );

    return( 0 );
}

bool get_nameserver_by_node_name( char *node_name )
{
    CNameServerConfig *config;

    config = NameServerConfig.GetFirstConfig();
    for ( ; config; config = config->GetNext() )
    {
        if ( CPNodeConfigContainer::hostnamecmp( node_name, config->GetName() ) == 0 )
        {
            return true;
        }
    }

    return false;
}

void get_persist_process_attributes( CPersistConfig *persistConfig
                                   , int             nid
                                   , PROCESSTYPE    &processType
                                   , char           *processName
                                   , int            &programArgc
                                   , char           *programArgs
                                   , char           *outfile
                                   , char           *persistRetries
                                   , char           *persistZones
                                   )
{
    const char method_name[] = "get_persist_process_attributes";
    char zoneStr[MAX_PERSIST_VALUE_STR];

    processType = persistConfig->GetProcessType();

    switch (persistConfig->GetZoneZidFormat())
    {
    case Zid_ALL:
        sprintf( zoneStr, "%d (ALL)", -1 );
        strcat( persistZones, zoneStr );
        break;
    case Zid_RELATIVE:
        sprintf( zoneStr, "%d", nid );
        strcpy( persistZones, zoneStr );
        break;
    default:
        printf ("[%s] Invalid persist zone zid format!\n", MyName);
        break;
    }

    if ( nid == -1 )
    {
        sprintf( processName, "%s"
               , persistConfig->GetProcessNamePrefix() );
        sprintf( outfile, "%s"
               , persistConfig->GetStdoutPrefix() );
    }
    else
    {
        sprintf( processName, "%s%d"
               , persistConfig->GetProcessNamePrefix()
               , nid );
        sprintf( outfile, "%s%d"
               , persistConfig->GetStdoutPrefix()
               , nid );
    }

    programArgc = persistConfig->GetProgramArgc();
    if (programArgc)
    {
        sprintf( programArgs, "%s"
               , persistConfig->GetProgramArgs() );
    }

    sprintf( persistRetries, "%d,%d"
           , persistConfig->GetPersistRetries()
           , persistConfig->GetPersistWindow() );

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d Persist process Nid=%d, "
                      "processName=%s, type=%s, stdout=%s, "
                      "persistRetries=%s, persistZones=%s\n"
                    , method_name, __LINE__
                    , nid, processName
                    , ProcessTypeString(persistConfig->GetProcessType())
                    , outfile
                    , persistRetries
                    , persistZones );
}

int get_pnid_by_nid( int nid )
{
    int pnid;

    CPNodeConfig   *pnodeConfig;
    CLNodeConfig   *lnodeConfig;

    lnodeConfig = ClusterConfig.GetLNodeConfig( nid );
    if ( !lnodeConfig )
    {
        return( -1 );
    }
    pnodeConfig = lnodeConfig->GetPNodeConfig();
    if ( !pnodeConfig )
    {
        return( -1 );
    }
    pnid = pnodeConfig->GetPNid();

    return( pnid );
}

int get_pnid_by_node_name( char *node_name )
{
    CPNodeConfig   *pnodeConfig;

    pnodeConfig = ClusterConfig.GetFirstPNodeConfig();
    for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
    {
        if ( CPNodeConfigContainer::hostnamecmp( node_name, pnodeConfig->GetName() ) == 0 )
        {
            return( pnodeConfig->GetPNid() );
        }
    }

    return( -1 );
}

bool get_node_state( int nid, char *node_name, int &pnid, STATE &state, bool &integrating )
{
    bool rs = false;
    int  count;
    MPI_Status status;

    if ( nid > -1 )
    {
        pnid = get_pnid_by_nid( nid );
        if ( pnid == -1 )
        {
            printf( "[%s] Invalid node, nid=%d\n", MyName, nid );
            return( rs );
        }
    }
    else if ( node_name )
    {
        pnid = get_pnid_by_node_name( node_name );
        if ( pnid == -1 )
        {
            printf( "[%s] Invalid node=%s\n", MyName, node_name );
            return( rs );
        }
    }
    else
    {
        printf( "[%s] Internal error - invalid node=%s, nid=%d\n", MyName, node_name, nid );
        return( rs );
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf("[%s] Unable to acquire message buffer.\n", MyName);
        return( rs );
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_PNodeInfo;
    msg->u.request.u.pnode_info.nid = MyNid;
    msg->u.request.u.pnode_info.pid = MyPid;
    msg->u.request.u.pnode_info.target_pnid = (nid > -1) ? pnid : -1;
    if ( node_name )
    {
        STRCPY( msg->u.request.u.pnode_info.target_name, node_name );
    }
    msg->u.request.u.pnode_info.continuation = false;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_PNodeInfo))
        {
            if (msg->u.reply.u.pnode_info.return_code == MPI_SUCCESS)
            {
                integrating = msg->u.reply.u.pnode_info.integrating;
                if (msg->u.reply.u.pnode_info.num_returned == 1)
                {
                    state = msg->u.reply.u.pnode_info.node[0].pstate;
                    rs = true;
                }
            }
            else
            {
                printf ("[%s] NodeInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.pnode_info.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for NodeInfo message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] NodeInfo reply message invalid\n", MyName);
    }

    gp_local_mon_io->release_msg(msg);

    return( rs );
}

void get_proc_info( int nid
                  , int pid
                  , char *process_name
                  , PROCESSTYPE process_type
                  , bool pattern
                  , bool displayHeader = true )
{
    int count;
    MPI_Status status;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfo;
    msg->u.request.u.process_info.nid = MyNid;
    msg->u.request.u.process_info.pid = MyPid;
    msg->u.request.u.process_info.verifier = MyVerifier;
    msg->u.request.u.process_info.process_name[0] = 0;
    msg->u.request.u.process_info.target_nid = nid;
    msg->u.request.u.process_info.target_pid = pid;
    msg->u.request.u.process_info.target_verifier = -1;
    msg->u.request.u.process_info.target_process_name[0] = 0;
    msg->u.request.u.process_info.target_process_pattern[0] = 0;
    if (pattern)
    {
        STRCPY (msg->u.request.u.process_info.target_process_pattern, process_name);
    }
    else
    {
        STRCPY (msg->u.request.u.process_info.target_process_name, process_name);
    }
    msg->u.request.u.process_info.type = process_type;

    bool fail = gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ( fail )
    {
        printf("[%s] ProcessInfo message send failed\n", MyName);
    }
    else if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_ProcessInfo))
        {
            if (msg->u.reply.u.process_info.return_code == MPI_SUCCESS)
            {
                if (displayHeader)
                {
                    printf("[%s] NID,PID(os)  PRI TYPE STATES  NAME         PARENT       PROGRAM\n",MyName);
                    printf("[%s] ------------ --- ---- ------- ------------ ------------ ---------------\n",MyName);
                }

                show_proc_info();
                // If there is more process info that will fit into
                // one reply message, make more requests to get all of
                // the data.
                bool allNodes = (nid == -1) && (pid == -1);
                while (msg->u.reply.u.process_info.more_data)
                {
                    if (get_more_proc_info(process_type, allNodes))
                        show_proc_info();
                    else // Error in getting more data
                        break;
                }
            }
            else
            {
                printf ("[%s] ProcessInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.process_info.return_code));
            }
        }
        else if ((msg->type == MsgType_Service) &&
                 (msg->u.reply.type == ReplyType_Generic))
        {
            printf ("ps failed, rc=%d\n", msg->u.reply.u.generic.return_code);
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessInfo message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] ProcessInfo reply message invalid, msg tag is %d, count= %d. \n", MyName, status.MPI_TAG, count);
    }

    gp_local_mon_io->release_msg(msg);
}

// This returns a StateDown in state if any node in the spare set is down,
// except the node_name passed in.
// Used with cold standby spare node activation where there must be at least
// one logical nodel in the configured spare in a down state. Otherwise,
// a spare node could be brought up and not activated and therefore become
// a hot standby.
bool get_spare_set_state( char *node_name, STATE &spare_set_state )
{
    const char method_name[] = "get_spare_set_state";

    bool rs = false;  // Assume failure
    bool integrating = false;
    int  pnid;
    STATE state = spare_set_state = State_Up;
    CPNodeConfig *spareNodeConfig;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Getting spare set for node=%s\n"
                     , method_name, __LINE__, MyName, node_name );

    // Any spare nodes in configuration?
    if ( ClusterConfig.GetSNodesCount() )
    {
        // Get the spare set that corresponds to node_name, if it exists
        PNodesConfigList_t spareNodesConfigSet;
        ClusterConfig.GetSpareNodesConfigSet( node_name
                                            , spareNodesConfigSet );
        if (spareNodesConfigSet.size())
        {
            // Check each node in the spare set against the current operational node state
            PNodesConfigList_t::iterator itSnSet;
            for ( itSnSet = spareNodesConfigSet.begin();
                  itSnSet != spareNodesConfigSet.end();
                  itSnSet++ )
            {
                spareNodeConfig = *itSnSet;

                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf( "%s@%d [%s] Spare set member node=%s\n"
                                , method_name, __LINE__, MyName
                                , spareNodeConfig->GetName() );

                if ( CPNodeConfigContainer::hostnamecmp( spareNodeConfig->GetName(), node_name ) == 0 )
                {
                    if ( trace_settings & TRACE_SHELL_CMD )
                        trace_printf( "%s@%d [%s] Skipping member node=%s\n"
                                    , method_name, __LINE__, MyName
                                    , spareNodeConfig->GetName() );
                    continue;
                }

                // Check monitors state of the target node
                if ( !get_node_state( -1
                                    , (char *)spareNodeConfig->GetName()
                                    , pnid
                                    , state
                                    , integrating ) )
                {
                    // Something went wrong, error was already displayed
                    break;
                }
                else
                {
                    if ( trace_settings & TRACE_SHELL_CMD )
                        trace_printf( "%s@%d [%s] Member node=%s, state=%s\n"
                                    , method_name, __LINE__, MyName
                                    , spareNodeConfig->GetName()
                                    , StateString(state) );
                    if (state == State_Down)
                    {
                        // Found one member in the set down, so we are done
                        spare_set_state = State_Down;
                        break;
                    }
                }
            }
            rs = true;
        }
    }
    else
    {
        // No spare nodes to check, so assume node_name is down
        spare_set_state = State_Down;
        rs = true;
    }

    return( rs );
}

bool get_zone_state( int &nid, int &zid , char *node_name, int &pnid, STATE &state )
{
    bool rs = false;
    int  count;
    MPI_Status status;
    struct message_def *saved_msg = msg;

    if ( nid < 0 && zid < 0 )
    {
        printf( "[%s] Invalid operation, nid=%d, zid=%d\n", MyName, nid, zid );
        return( rs );
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf("[%s] Unable to acquire message buffer.\n", MyName);
        return( rs );
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ZoneInfo;
    msg->u.request.u.zone_info.nid = MyNid;
    msg->u.request.u.zone_info.pid = MyPid;
    msg->u.request.u.zone_info.target_nid = nid;
    msg->u.request.u.zone_info.target_zid = zid;
    msg->u.request.u.zone_info.continuation = false;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_ZoneInfo))
        {
            if (msg->u.reply.u.zone_info.return_code == MPI_SUCCESS)
            {
                if (msg->u.reply.u.zone_info.num_returned == 1)
                {
                    nid = msg->u.reply.u.zone_info.node[0].nid;
                    zid = msg->u.reply.u.zone_info.node[0].zid;
                    pnid = msg->u.reply.u.zone_info.node[0].pnid;
                    state = msg->u.reply.u.zone_info.node[0].pstate;
                    if ( node_name )
                    {
                        strcpy( node_name, msg->u.reply.u.zone_info.node[0].node_name );
                    }
                    rs = true;
                }
            }
            else
            {
                printf ("[%s] ZoneInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.zone_info.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ZoneInfo message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] ZoneInfo reply message invalid\n", MyName);
    }

    gp_local_mon_io->release_msg(msg);
    msg = saved_msg;

    return( rs );
}

bool getpid( char *name, int &nid, int &pid )
{
    bool notfound = true;
    int count;
    MPI_Status status;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return notfound;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfo;
    msg->u.request.u.process_info.nid = MyNid;
    msg->u.request.u.process_info.pid = MyPid;
    msg->u.request.u.process_info.verifier = MyVerifier;
    msg->u.request.u.process_info.process_name[0] = 0;
    msg->u.request.u.process_info.target_nid = nid;
    msg->u.request.u.process_info.target_pid = pid;
    msg->u.request.u.process_info.target_verifier = -1;
    if (strlen(name) >= MAX_PROCESS_NAME)
    {
        name[MAX_PROCESS_NAME-1] = '\0';
    }
    STRCPY (msg->u.request.u.process_info.target_process_name, name);
    msg->u.request.u.process_info.type = ProcessType_Undefined;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service              ) &&
            (msg->u.reply.type == ReplyType_ProcessInfo)   )
        {
            if (msg->u.reply.u.process_info.num_processes == 1)
            {
                nid = msg->u.reply.u.process_info.process[0].nid,
                pid = msg->u.reply.u.process_info.process[0].pid,
                notfound = false;
            }
            else if (msg->u.reply.u.process_info.return_code != 0)
            {
                printf ("[%s] ProcessInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.process_info.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessInfo message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] ProcessInfo reply message invalid, msg tag is %d, count= %d. \n", MyName, status.MPI_TAG, count);
    }

    gp_local_mon_io->release_msg(msg);
    return notfound;
}

void cancel_notice( _TM_Txid_External trans_id )
{
    int count;
    MPI_Status status;
    const char method_name[] = "cancel_notice";

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Notify;
    msg->u.request.u.notify.nid = MyNid;
    msg->u.request.u.notify.pid = MyPid;
    msg->u.request.u.notify.verifier = MyVerifier;
    msg->u.request.u.notify.process_name[0] = 0;
    msg->u.request.u.notify.cancel = 1;
    msg->u.request.u.notify.target_nid = -1;
    msg->u.request.u.notify.target_pid = -1;
    msg->u.request.u.notify.target_verifier = -1;
    msg->u.request.u.notify.trans_id = trans_id;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf("%s@%d [%s] Cancel Notify successfully. "
                                 "Nid=%d, Pid=%d\n", method_name, __LINE__,
                                 MyName, msg->u.reply.u.generic.nid,
                                 msg->u.reply.u.generic.pid );
            }
            else
            {
                printf ("Cancel Notify failed, rc=%d\n",
                        msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf
                ("Invalid MsgType(%d)/ReplyType(%d) for Cancel Notify message\n",
                 msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("Cancel Notify process reply message invalid\n");
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
}

int copy_config_db( char *node_name )
{
    // copy sqconfig.db
    char msgString[MAX_BUFFER] = { 0 };
    char cmd[256];
    int  error = 0;

    sprintf(cmd, "pdcp -p -w %s %s/sqconfig.db %s/.", node_name,
              getenv("TRAF_VAR"), getenv("TRAF_VAR") );

    error = system(cmd);

    if (error != 0)
    {
        sprintf( msgString, "[%s] Unable to copy sqconfig.db to node %s.\n"
                          , MyName, node_name);
        write_startup_log( msgString );
        printf( "Unable to copy sqconfig.db to node %s.\n", node_name);
        return( -1 );
    }

    return( 0 );
}

void request_notice( int nid,int pid,  _TM_Txid_External trans_id )
{
    int count;
    MPI_Status status;
    const char method_name[] = "request_notice";

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Notify;
    msg->u.request.u.notify.nid = MyNid;
    msg->u.request.u.notify.pid = MyPid;
    msg->u.request.u.notify.verifier = MyVerifier;
    msg->u.request.u.notify.process_name[0] = 0;
    msg->u.request.u.notify.cancel = 0;
    msg->u.request.u.notify.target_nid = nid;
    msg->u.request.u.notify.target_pid = pid;
    msg->u.request.u.notify.target_verifier = -1;
    msg->u.request.u.notify.trans_id = trans_id;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf("%s@%d [%s] Notify successfully. Nid=%d, "
                                 "Pid=%d \n", method_name, __LINE__, MyName,
                                 msg->u.reply.u.generic.nid,
                                 msg->u.reply.u.generic.pid);
            }
            else
            {
                printf ("Notify failed, rc=%d\n",
                        msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf
                (" Invalid MsgType(%d)/ReplyType(%d) for Notify message\n",
                  msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("Notify process reply message invalid\n");
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
}

_TM_Txid_External null_trans( void )
{
    _TM_Txid_External trans1;

    trans1.txid[0] = 0LL;
    trans1.txid[1] = 0LL;
    trans1.txid[2] = 0LL;
    trans1.txid[3] = 0LL;

    return trans1;
}

int get_lnodes_count( int nid )
{
    CLNodeConfig   *lnodeConfig;
    CPNodeConfig   *pnodeConfig;

    lnodeConfig = ClusterConfig.GetLNodeConfig( nid );
    if ( lnodeConfig )
    {
        pnodeConfig = lnodeConfig->GetPNodeConfig();
        if ( pnodeConfig )
        {
            return( pnodeConfig->GetLNodesCount() );
        }
        else
        {
            printf( "[%s] Error: Internal error in physical node configuration\n",MyName);
        }
    }
    else
    {
        printf( "[%s] Node id %d does not exist in configuration\n",MyName, nid);
    }

    return( -1 );
}

int get_first_nid( char *node_name )
{
    CLNodeConfig   *lnodeConfig;
    CPNodeConfig   *pnodeConfig;

    pnodeConfig = ClusterConfig.GetFirstPNodeConfig();
    for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
    {
        if ( CPNodeConfigContainer::hostnamecmp( node_name, pnodeConfig->GetName() ) == 0 )
        {
            lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
            if ( lnodeConfig )
            {
                return( lnodeConfig->GetNid() );
            }
            break;
        }
    }

    return( -1 );
}

int get_fqdn_by_name( char *nodeName, char *fqdn )
{
    int rc;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;      // Allow IPv4 only 
    hints.ai_socktype = 0;          // Any socktype
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          // Any protocol
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
 
    // getaddrinfo() returns a list of address structures.
    rc = getaddrinfo( nodeName, NULL, &hints, &result);
    if (rc != 0) 
    {
        fprintf( stderr
               , "Could not resolve host address, getaddrinfo(%s): %s\n"
               , nodeName, gai_strerror(rc) );
        return( -1 );
    }

    socklen_t saLen;
    struct sockaddr *sa;
    char hbuf[NI_MAXHOST];

    for (rp = result; rp != NULL; rp = rp->ai_next) 
    {
        sa = rp->ai_addr;
        saLen = rp->ai_addrlen;
        rc = getnameinfo(sa, saLen, hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD);
        if (rc != 0)
        {
            fprintf( stderr
                   , "Could not resolve hostname, getnameinfo(%s, NI_NAMEREQD): %s\n"
                   , nodeName, gai_strerror(rc) );
            continue;
        }
        else
        {
            break; // Good one, we're done!
        }
    }

    freeaddrinfo(result);   // No longer needed

    if (rp == NULL)
    {
        return( -1 );
    }

    strcpy( fqdn, hbuf );

    return( 0 );
}

int get_node_name( char *node_name, char *short_node_name )
{
    CPNodeConfig   *pnodeConfig;

    pnodeConfig = ClusterConfig.GetFirstPNodeConfig();
    for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
    {
        if ( CPNodeConfigContainer::hostnamecmp( node_name, pnodeConfig->GetName() ) == 0 )
        {
            if (short_node_name)
            {
                strcpy( short_node_name, pnodeConfig->GetName() );
            }
            return( 0 );
        }
    }

    return( -1 );
}

int get_short_node_name( char *node_name, char *short_node_name )
{
    if ( !node_name ) return( -1 );
    if ( !short_node_name ) return( -1 );

    char str1[1024];
    memset( str1, 0, 1024 );

    char *str1_dot = strchr( (char *) node_name, '.' );
    if ( str1_dot )
    { // Found '.', copy up to one char before '.'
        memcpy( str1, node_name, str1_dot - node_name );
    }
    else
    { // Copy entire string
        strcpy( str1, node_name );
    }

    strcpy( short_node_name, str1 );

    return( 0 );
}

bool get_more_proc_info(PROCESSTYPE process_type, bool allNodes)
{
    bool replyOk = false;
    MPI_Status status;
    int count;

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfoCont;
    msg->u.request.u.process_info_cont.nid = MyNid;
    msg->u.request.u.process_info_cont.pid = MyPid;
    msg->u.request.u.process_info_cont.type = process_type;
    msg->u.request.u.process_info_cont.allNodes = allNodes;

    for (int i=0, contexti = (MAX_PROCINFO_LIST - 1);
         i<MAX_PROC_CONTEXT; i++, contexti--)
    {
        msg->u.request.u.process_info_cont.context[i].nid =
            msg->u.reply.u.process_info.process[contexti].nid;
        msg->u.request.u.process_info_cont.context[i].pid =
            msg->u.reply.u.process_info.process[contexti].pid;
    }

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;


    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_ProcessInfo))
        {
            if (msg->u.reply.u.process_info.return_code == MPI_SUCCESS)
            {
                replyOk = true;
            }
            else
            {
                printf ("[%s] ProcessInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.process_info.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessInfo message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] ProcessInfo reply message invalid, msg tag is %d, count= %d. \n", MyName, status.MPI_TAG, count);
    }

    return replyOk;

}

int get_pnid( int nid )
{
    int count;
    int pnid = -1;
    MPI_Status status;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return pnid;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NodeInfo;
    msg->u.request.u.node_info.nid = MyNid;
    msg->u.request.u.node_info.pid = MyPid;
    msg->u.request.u.node_info.target_nid = nid;
    msg->u.request.u.node_info.continuation = false;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_NodeInfo))
        {
            if (msg->u.reply.u.node_info.return_code == MPI_SUCCESS)
            {
                if (msg->u.reply.u.node_info.num_returned)
                {
                    pnid = msg->u.reply.u.node_info.node[0].pnid;
                }
            }
        }
    }
    gp_local_mon_io->release_msg(msg);

    return( pnid );
}


void get_server_death (int nid, int pid)
{
    const char method_name[] = "get_server_death";
    _TM_Txid_External transid;

    if (Slave == true)
    {
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s] register death notice from nid=%d, "
                         "pid=%d.\n",
                         method_name, __LINE__, MyName, nid, pid);
        transid = null_trans();
        request_notice(nid, pid, transid);
    }

    for ( int dinx = 0; dinx < MAX_DEATH_SAVE; dinx++ )
    {
        if ( lastDeathNid[dinx] == nid
          && lastDeathPid[dinx] == pid )
        {
            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf("%s@%d [%s] death message already received from nid=%d, "
                             "pid=%d.\n", method_name, __LINE__, MyName, nid, pid);

            lastDeathNid[dinx] = -1;
            lastDeathPid[dinx] = -1;

            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf("%s@%d [%s] Exiting wait for process death\n",
                             method_name, __LINE__, MyName);

            return;
        }
    }

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] waiting for death message from nid=%d, "
                     "pid=%d.\n", method_name, __LINE__, MyName, nid, pid);

    waitDeathNid = nid;
    waitDeathPid = pid;
    waitDeathPending = true;

    waitDeathLock.lock();
    waitDeathLock.wait();
    waitDeathLock.unlock();

    if (Slave == true)
    {
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s] cancel death notice from nid=%d, pid=%d.\n",
                         method_name, __LINE__, MyName, nid, pid);
        cancel_notice(transid);
    }
    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] Exiting wait for process death\n",
                     method_name, __LINE__, MyName);

}

char *get_token (char *cmd, char *token, char *delimiter,
                 int maxlen, bool isEqDelim, bool isDashDelim)
{
    char *ptr = remove_white_space (cmd);
    char *end = find_end_of_token (ptr,maxlen,isEqDelim,isDashDelim);

    *delimiter = *end;
    if (*end)
    {
        *end = '\0';
        strcpy (token, ptr);
        *end = *delimiter;
        end = find_delimiter(end, isEqDelim,isDashDelim);
        *delimiter = *end;
        ptr = remove_white_space (end);
        if (*ptr == '{'
         || *ptr == '}'
         || *ptr == ':'
         || (*ptr == '-' && isDashDelim)
         || (*ptr == '=' && isEqDelim))
        {
            *delimiter = *ptr;
            ptr++;
            ptr = remove_white_space (ptr);
        }
    }
    else
    {
        strcpy (token, ptr);
        ptr = end;
    }

    return ptr;
}

bool isNumeric( char * str )
{
    bool isNum = false;
    char *ptr = str;
    int len = strlen( str );

    if ( len )
    {
        isNum = true;
        for ( ; *ptr ; ptr++ )
        {
            if ( ! isdigit(*ptr) )
            {
                isNum = false;
                break;
            }
        }
    }

    return( isNum );
}

void kill_process(int nid, int pid, char *name, bool abort)
{
    int count;
    MPI_Status status;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Kill;
    msg->u.request.u.kill.nid = MyNid;
    msg->u.request.u.kill.pid = MyPid;
    msg->u.request.u.kill.verifier = MyVerifier;
    msg->u.request.u.kill.process_name[0] = 0;
    msg->u.request.u.kill.target_nid = nid;
    msg->u.request.u.kill.target_pid = pid;
    msg->u.request.u.kill.target_verifier = -1;
    msg->u.request.u.kill.persistent_abort = abort;
    msg->u.request.u.kill.target_process_name[0] = 0;
    if ( name[0] == '$' )
    {
        STRCPY (msg->u.request.u.kill.target_process_name, name);
    }

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code != MPI_SUCCESS)
            {
                printf ("[%s] Kill failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.generic.return_code));
            }
            else
            {
                printf ("[%s] Process %s (%d,%d) killed\n", MyName,
                    name, nid, pid );
            }
        }
        else
        {
            printf ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Kill message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Kill reply message invalid\n", MyName);
    }

    gp_local_mon_io->release_msg(msg);
}

void listZoneInfo( int nid, int zid )
{
    int i;
    int count;
    MPI_Status status;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    bool needBanner = true;
    bool getMoreInfo = false;
    do
    {
        msg->type = MsgType_Service;
        msg->noreply = false;
        msg->reply_tag = REPLY_TAG;
        msg->u.request.type = ReqType_ZoneInfo;
        msg->u.request.u.zone_info.nid = MyNid;
        msg->u.request.u.zone_info.pid = MyPid;
        msg->u.request.u.zone_info.target_nid = nid;
        msg->u.request.u.zone_info.target_zid = zid;
        if ( !getMoreInfo )
        {
            msg->u.request.u.zone_info.continuation = false;
        }

        gp_local_mon_io->send_recv( msg );
        count = sizeof( *msg );
        status.MPI_TAG = msg->reply_tag;

        getMoreInfo = false;
        if ((status.MPI_TAG == REPLY_TAG) &&
            (count == sizeof (struct message_def)))
        {
            if ((msg->type == MsgType_Service) &&
                (msg->u.reply.type == ReplyType_ZoneInfo))
            {
                if (msg->u.reply.u.zone_info.return_code == MPI_SUCCESS)
                {
                    if (msg->u.reply.u.zone_info.num_returned)
                    {
                        if (needBanner)
                        {
                            printf ("[%s] ZID PNID State    Name\n", MyName);
                            printf ("[%s] --- ---- -------- --------\n", MyName);
                            needBanner = false;
                        }

                        for (i=0; i < msg->u.reply.u.zone_info.num_returned; i++)
                        {
                            if ( msg->u.reply.u.zone_info.node[i].nid != -1 )
                            {
                                // Display zone node info
                                //      "[%s] ZID PNID State    Name\n", MyName);
                                //      "[%s] --- ---- -------- --------\n", MyName);
                                printf ("[%s] %3.3d  %3.3d %-8s %s\n",
                                    MyName,
                                    msg->u.reply.u.zone_info.node[i].zid,
                                    msg->u.reply.u.zone_info.node[i].pnid,
                                    StateString( msg->u.reply.u.zone_info.node[i].pstate ),
                                    msg->u.reply.u.zone_info.node[i].node_name );
                            }
                        }
                        getMoreInfo = (msg->u.reply.u.zone_info.num_returned == MAX_NODE_LIST);
                        if (getMoreInfo)
                        {  // Since we got the maximum number of node info
                           // entries there may be additional node info
                           // entries to retrieve.  Populate the request.
                           msg->u.request.u.zone_info.last_nid =
                              msg->u.reply.u.zone_info.last_nid;
                           msg->u.request.u.zone_info.last_pnid =
                              msg->u.reply.u.zone_info.last_pnid;
                           msg->u.request.u.zone_info.continuation = true;
                        }
                    }
                }
                else
                {
                    printf ("[%s] ZoneInfo failed, error=%s\n", MyName,
                        ErrorMsg(msg->u.reply.u.zone_info.return_code));
                }
            }
            else
            {
                printf
                    ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ZoneInfo message\n",
                     MyName, msg->type, msg->u.reply.type);
            }
        }
        else
        {
            printf ("[%s] ZoneInfo reply message invalid\n", MyName);
        }
    }
    while (getMoreInfo);

    gp_local_mon_io->release_msg(msg);
}



bool load_configuration( void )
{
    char mynode[MPI_MAX_PROCESSOR_NAME];
    int i;
    CLNodeConfig   *lnodeConfig;
    CPNodeConfig   *pnodeConfig;

    // Read initialization file
    gethostname(mynode, MPI_MAX_PROCESSOR_NAME);
    NumDown=0;
    NumNodes=0;
    NumLNodes=0;
    for (i=0; i < MAX_NODES; i++)
    {
        Node[i][0] = '\0';
        PNode[i][0] = '\0';
        NodeState[i] = false;
    }

    if ( ClusterConfig.IsConfigReady() )
    {
        // It was previously loaded, remove the current configuration
        ClusterConfig.Clear();
    }
    NameServerConfig.Clear();
    bool traceEnabled = (trace_settings & TRACE_TRAFCONFIG) ? true : false;
    if ( ClusterConfig.Initialize( traceEnabled, traceFileName ) )
    {
        if ( ! ClusterConfig.LoadConfig() )
        {
            printf("[%s] Error: Failed to load cluster configuration.\n", MyName);
            return false;
        }
        if ( ! NameServerConfig.LoadConfig() )
        {
            printf("[%s] Error: Failed to load nameserver configuration.\n", MyName);
            return false;
        }
        NumLNodes = ClusterConfig.GetLNodesCount();
        NumNodes  = ClusterConfig.GetPNodesCount();
        PNodesConfigMax = ClusterConfig.GetPNodesConfigMax();
        LNodesConfigMax = ClusterConfig.GetLNodesConfigMax();
        lnodeConfig = ClusterConfig.GetFirstLNodeConfig();
        if ( lnodeConfig )
        {
            while ( lnodeConfig )
            {
                pnodeConfig = lnodeConfig->GetPNodeConfig();
                if ( pnodeConfig )
                {
                    if (!VirtualNodes)
                    {
                        strcpy( Node[pnodeConfig->GetPNid()], pnodeConfig->GetName() );
                    }
                    if ( !NodeState[pnodeConfig->GetPNid()] )
                    {
                        NodeState[pnodeConfig->GetPNid()] = true;
                    }
                }
                else
                {
                    printf( "[%s] Error: Internal error while loading physical node configuration\n",MyName);
                }
                lnodeConfig = lnodeConfig->GetNext();
            }
        }
        else
        {
            printf( "[%s] Error: No logical nodes in configuration\n",MyName);
        }
        pnodeConfig = ClusterConfig.GetFirstPNodeConfig();
        if ( pnodeConfig )
        {
            while ( pnodeConfig )
            {
                if (!VirtualNodes)
                {
                    strcpy( PNode[pnodeConfig->GetPNid()], pnodeConfig->GetFqdn() );
                }
                else
                {
                    gethostname(PNode[pnodeConfig->GetPNid()], MPI_MAX_PROCESSOR_NAME);
                }
                pnodeConfig = pnodeConfig->GetNext();
            }
        }
        else
        {
            printf( "[%s] Error: No physical nodes in configuration\n",MyName);
        }
    }
    else
    {
        printf( "[%s] Error: Could not initialize Trafodion Configuration database\n",MyName);
        return false;
    }

    return true;
}

char *normalize_case (char *token)
{
    char *ptr = token;

    while (*ptr)
    {
        *ptr = tolower (*ptr);
        if (*ptr == '\n') *ptr = '\0';
        ptr++;
    }

    return token;
}

void normalize_slashes (char *token)
{
    char *ptr;

    for (ptr = token; *ptr; ptr++)
    {
        if (*ptr == '\\')
        {
            *ptr = '/';
        }
    }
}

void nameserver_add( char *node_name )
{
    const char method_name[] = "nameserver_add";

    int pnid;
    int count;
    char msgString[MAX_BUFFER] = { 0 };
    MPI_Status status;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Adding nameserver:\n"
                      "   node_name  = %s\n"
                    , method_name, __LINE__, MyName
                    , node_name );

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending nameserver add message.\n",
                      method_name, __LINE__, MyName);

    if(VirtualNodes)
        pnid = get_pnid_by_nid( atoi(node_name) );
    else
        pnid = get_pnid_by_node_name( node_name );
    if ( pnid == -1 )
    {
        sprintf( msgString, "[%s] Node %s is not configured!", MyName, node_name );
        write_startup_log( msgString );
        printf ("%s\n", msgString );
        return;
    }

    assert(gp_local_mon_io);
    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        sprintf( msgString, "[%s] Unable to acquire message buffer.", MyName);
        write_startup_log( msgString );
        printf( "%s\n", msgString );
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NameServerAdd;
    msg->u.request.u.nameserver_add.nid = MyNid;
    msg->u.request.u.nameserver_add.pid = MyPid;
    STRCPY( msg->u.request.u.nameserver_add.node_name, node_name );

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Sending nameserver add\n "
                    , method_name, __LINE__, MyName );

    gp_local_mon_io->send_recv( msg );
    if (gp_local_mon_io->iv_shutdown)
    {
        gp_local_mon_io->release_msg(msg);
        return;
    }
    status.MPI_TAG = msg->reply_tag;
    count = sizeof( *msg );

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                if ( !load_configuration() )
                {
                    exit (1);
                }
            }
            else
            {
                if (msg->u.reply.u.generic.return_code == MPI_ERR_IO)
                {
                    printf( "[%s] Nameserver add failed, could not accessing configuration database\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NAME)
                {
                    printf( "[%s] Nameserver add failed, node %s already exists in nameserver configuration\n"
                          , MyName, node_name );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_OP)
                {
                    printf( "[%s] Nameserver add failed, number of nodes limit exceeded\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NO_MEM)
                {
                    printf( "[%s] Nameserver add failed with memory allocation error, check monitor log for details\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_INTERN)
                {
                    printf( "[%s] Nameserver add failed, check monitor log for error details\n"
                          , MyName );
                }
                else
                {
                    printf( "[%s] Nameserver add failed, error=%s\n"
                          , MyName, ErrorMsg(msg->u.reply.u.generic.return_code));
                }
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n"
                  , MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf( "[%s] Node add reply invalid, msg tag is %d, count= %d. \n"
              , MyName, status.MPI_TAG, count);
    }

    if (gp_local_mon_io)
        gp_local_mon_io->release_msg(msg);
}

void nameserver_config( )
{
    const char method_name[] = "nameserver_config";
    bool prev = false;

    CNameServerConfig *nameServerConfig;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] nameserver Configuration\n"
                    , method_name, __LINE__, MyName );

    printf( "node-name=" );
    nameServerConfig = NameServerConfig.GetFirstConfig();
    for ( ; nameServerConfig; nameServerConfig = nameServerConfig->GetNext() )
    {
        if ( prev )
            printf( "," );
        prev = true;
        printf( "%s", nameServerConfig->GetName() );
    }
    printf( "\n" );
}

void nameserver_delete( char *node_name )
{
    const char method_name[] = "nameserver_delete";

    char msgString[MAX_BUFFER] = { 0 };
    int count;
    MPI_Status status;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Deleting nameserver:\n"
                      "   node_name  = %s\n"
                    , method_name, __LINE__, MyName
                    , node_name );

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending nameserver delete message.\n",
                      method_name, __LINE__, MyName);

    assert(gp_local_mon_io);
    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        sprintf( msgString, "[%s] Unable to acquire message buffer.", MyName);
        write_startup_log( msgString );
        printf( "%s\n", msgString );
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NameServerDelete;
    msg->u.request.u.nameserver_delete.nid = MyNid;
    msg->u.request.u.nameserver_delete.pid = MyPid;
    STRCPY( msg->u.request.u.nameserver_delete.node_name, node_name );

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Sending nameserver delete\n "
                    , method_name, __LINE__, MyName );

    gp_local_mon_io->send_recv( msg );
    if (gp_local_mon_io->iv_shutdown)
    {
        gp_local_mon_io->release_msg(msg);
        return;
    }
    status.MPI_TAG = msg->reply_tag;
    count = sizeof( *msg );

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                if ( !load_configuration() )
                {
                    exit (1);
                }
            }
            else
            {
                if (msg->u.reply.u.generic.return_code == MPI_ERR_IO)
                {
                    printf( "[%s] NameServer deleted failed, could not access configuration database\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NAME)
                {
                    printf( "[%s] NameServer deleted failed, node does not exist in configuration in monitor\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NO_MEM)
                {
                    printf( "[%s] NameServer deleted failed, could not process request in monitor\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_INTERN)
                {
                    printf( "[%s] NameServer deleted failed, could not re-establish cluster configuration in monitor\n"
                          , MyName );
                }
                else
                {
                    printf( "[%s] NameServer deleted failed, error=%s\n"
                          , MyName, ErrorMsg(msg->u.reply.u.generic.return_code));
                }
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n"
                  , MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf( "[%s] NameServer deleted reply invalid, msg tag is %d, count= %d. \n"
              , MyName, status.MPI_TAG, count);
    }

    if (gp_local_mon_io)
        gp_local_mon_io->release_msg(msg);
}

void nameserver_info( )
{
    char process_name[1];

    process_name[0] = '\0';
    get_proc_info( -1
                 , -1
                 , process_name
                 , ProcessType_NameServer
                 , false );
}

void nameserver_stop( char *node_name )
{
    const char method_name[] = "nameserver_stop";
    char msgString[MAX_BUFFER] = { 0 };
    int count;
    MPI_Status status;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] sending nameserver stop message.\n",
                     method_name, __LINE__, MyName);

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        sprintf( msgString, "[%s] Unable to acquire message buffer.\n", MyName);
        write_startup_log( msgString );
        printf( "%s\n", msgString );
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NameServerStop;
    msg->u.request.u.nameserver_stop.nid = MyNid;
    msg->u.request.u.nameserver_stop.pid = MyPid;
    STRCPY(msg->u.request.u.nameserver_stop.node_name, node_name);

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code != MPI_SUCCESS)
            {
                printf ("[%s] NameServer Stop failed, error=%s\n", MyName,
                ErrorMsg(msg->u.reply.u.generic.return_code));
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for NameServer Stop message\n"
                  , MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] NameServer Stop reply message invalid\n", MyName);
    }

    gp_local_mon_io->release_msg(msg);
}

void nameserver_start( char *node_name )
{
    const char method_name[] = "nameserver_start";
    int count;
    int rc = -1;
    char msgString[MAX_BUFFER] = { 0 };
    MPI_Status status;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending NameServer Start message.\n",
                      method_name, __LINE__, MyName);

    assert(gp_local_mon_io);
    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        sprintf( msgString, "[%s] Unable to acquire message buffer.", MyName);
        write_startup_log( msgString );
        printf( "%s\n", msgString );
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NameServerStart;
    msg->u.request.u.nameserver_start.nid = MyNid;
    msg->u.request.u.nameserver_start.pid = MyPid;
    STRCPY( msg->u.request.u.nameserver_start.node_name, node_name );
    gp_local_mon_io->send_recv( msg );

    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ( "%s@%d [%s] reply from monitor with tag: %d, count: "
                       "%d.\n", method_name, __LINE__, MyName,
                       status.MPI_TAG, count);

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            rc = msg->u.reply.u.generic.return_code;
            if (rc == MPI_SUCCESS)
            {
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf (
                         "%s@%d [%s] Started process successfully. Nid=%d, "
                         "Pid=%d, Process_name=%s, Verifier=%d,rtn=%d\n",
                         method_name, __LINE__,
                         MyName,
                         msg->u.reply.u.generic.nid,
                         msg->u.reply.u.generic.pid,
                         msg->u.reply.u.generic.process_name,
                         msg->u.reply.u.generic.verifier,
                         rc );
            }
            else
            {
                printf ("[%s] NameServer Start failed, error=%d(%s)\n", MyName,
                        rc, ErrorMsg(rc));
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for NameServer Start message\n",
                   MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] NameServer Start reply message invalid\n", MyName);
    }
    gp_local_mon_io->release_msg( msg );
}

void node_add( char *node_name, int first_core, int last_core, int processors, int roles )
{
    const char method_name[] = "node_add";

    int pnid;
    int count;
    char msgString[MAX_BUFFER] = { 0 };
    MPI_Status status;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Adding node:\n"
                      "   node_name  = %s\n"
                      "   first_core = %d\n"
                      "   last_core  = %d\n"
                      "   processors = %d\n"
                      "   roles      = %s\n"
                    , method_name, __LINE__, MyName
                    , node_name, first_core, last_core, processors
                    , RoleTypeString( static_cast<ZoneType>(roles) ));

    // Check that node_name is not in the configuration
    pnid = get_pnid_by_node_name( node_name );
    if ( pnid != -1 )
    {
        sprintf( msgString, "[%s] Node %s already exists in configuration", MyName, node_name );
        write_startup_log( msgString );
        printf ("%s\n", msgString );
        return;
    }

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending node add message.\n",
                      method_name, __LINE__, MyName);

    assert(gp_local_mon_io);
    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        sprintf( msgString, "[%s] Unable to acquire message buffer.", MyName);
        write_startup_log( msgString );
        printf ("%s\n", msgString );
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NodeAdd;
    msg->u.request.u.node_add.nid = MyNid;
    msg->u.request.u.node_add.pid = MyPid;
    STRCPY( msg->u.request.u.node_add.node_name, node_name );
    msg->u.request.u.node_add.first_core = first_core;
    msg->u.request.u.node_add.last_core  = last_core;
    msg->u.request.u.node_add.processors = processors;
    msg->u.request.u.node_add.roles      = roles;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Sending node add\n "
                    , method_name, __LINE__, MyName );

    gp_local_mon_io->send_recv( msg );
    if (gp_local_mon_io->iv_shutdown)
    {
        gp_local_mon_io->release_msg(msg);
        return;
    }
    status.MPI_TAG = msg->reply_tag;
    count = sizeof( *msg );

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code != MPI_SUCCESS)
            {
                if (msg->u.reply.u.generic.return_code == MPI_ERR_IO)
                {
                    printf( "[%s] Node add failed, could not accessing configuration database\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NAME)
                {
                    printf( "[%s] Node add failed, node %s already exists in cluster configuration\n"
                          , MyName, node_name );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_OP)
                {
                    printf( "[%s] Node add failed, number of nodes limit exceeded\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NO_MEM)
                {
                    printf( "[%s] Node add failed with memory allocation error, check monitor log for details\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_INTERN)
                {
                    printf( "[%s] Node add failed, check monitor log for error details\n"
                          , MyName );
                }
                else
                {
                    printf( "[%s] Node add failed, error=%s\n"
                          , MyName, ErrorMsg(msg->u.reply.u.generic.return_code));
                }
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n"
                  , MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf( "[%s] Node add reply invalid, msg tag is %d, count= %d. \n"
              , MyName, status.MPI_TAG, count);
    }

    if (gp_local_mon_io)
        gp_local_mon_io->release_msg(msg);
}

void node_change_name(char *current_name, char *new_name)
{
    const char method_name[] = "node_change_name";

    bool integrating = false;
    int pnid = -1;
    int count;
    char msgString[MAX_BUFFER] = { 0 };
    MPI_Status status;

    pnid = get_pnid_by_node_name( current_name );
    if ( pnid == -1 )
    {
        sprintf( msgString, "[%s] Node %s is not configured!", MyName, current_name );
        write_startup_log( msgString );
        printf ("%s\n", msgString );
        return;
    }

    STATE state = State_Unknown;
    // Check monitors state of the target node
    if ( !get_node_state( -1, current_name, pnid, state, integrating ) )
    {
        sprintf( msgString, "[%s] Monitor does not have state information on pnid=%d (%s)!"
                          , MyName, pnid, current_name);
        write_startup_log( msgString );
        printf ("%s\n", msgString );
    }
    if ( integrating )
    {
        sprintf( msgString, "[%s] Node up in progress! Try again when node re-integration completes.", MyName);
        write_startup_log( msgString );
        printf ("%s\n", msgString );
        return;
    }
    if ( state != State_Unknown )
    {
        if ( state != State_Down )
        {
            sprintf( msgString, "[%s] Node %s is not in down state! (state=%s)", MyName, current_name, StateString(state) );
            write_startup_log( msgString );
            printf ("%s\n", msgString );
            return;
        }
    }

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending node delete message.\n",
                      method_name, __LINE__, MyName);

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        trace_printf ("%s@%d [%s] sending node change name message.\n",
                      method_name, __LINE__, MyName);
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NodeName;
    msg->u.request.u.nodename.nid = MyNid;
    msg->u.request.u.nodename.pid = MyPid;
    strcpy (msg->u.request.u.nodename.new_name, new_name);
    strcpy (msg->u.request.u.nodename.current_name, current_name);

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.node_info.return_code != MPI_SUCCESS)
            {
                if (msg->u.reply.u.generic.return_code == MPI_ERR_IO)
                {
                    printf( "[%s] Node name failed accessing configuration database\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NAME)
                {
                    printf( "[%s] Node name failed, node %s does not exist in configuration database\n"
                          , MyName, current_name );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NO_MEM)
                {
                    printf( "[%s] Node name failed with memory allocation error, check monitor log for details\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_INTERN)
                {
                    printf( "[%s] Node name failed, check monitor log for error details\n"
                          , MyName );
                }
                else
                {
                    printf( "[%s] Node name change failed, error=%s\n"
                          , MyName, ErrorMsg(msg->u.reply.u.generic.return_code));
                }
            }
        }
        else
        {
            printf ("[%s] Invalid Message/Reply type for Node Name Change request.\n", MyName);
        }
    }
    else
    {
        printf( "[%s] Node name reply invalid, msg tag is %d, count= %d. \n"
              , MyName, status.MPI_TAG, count);
    }
}

void node_config( int nid, char *node_name )
{
    const char method_name[] = "node_config";

    char coresString[MAX_TOKEN];
    CLNodeConfig   *lnodeConfig;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Configuration for node:\n"
                      "   nid        = %d\n"
                      "   node_name  = %s\n"
                    , method_name, __LINE__, MyName
                    , nid, node_name );

    lnodeConfig = ClusterConfig.GetFirstLNodeConfig();
    for ( ; lnodeConfig; lnodeConfig = lnodeConfig->GetNext() )
    {
        if ( lnodeConfig )
        {
            if ((nid == -1 && *node_name == '\0' )
             || (nid != -1 && nid == lnodeConfig->GetNid())
             || (strcmp( node_name, lnodeConfig->GetName()) == 0))
            {
                if (lnodeConfig->GetLastCore() == -1)
                {
                    snprintf( coresString, sizeof(coresString)
                            , "%d", lnodeConfig->GetFirstCore() );
                }
                else
                {
                    snprintf( coresString, sizeof(coresString)
                            , "%d-%d"
                            , lnodeConfig->GetFirstCore()
                            , lnodeConfig->GetLastCore() );
                }
                printf( "node-id=%d, node-name=%s, "
                        "cores=%s, processors=%d, roles=%s\n"
                      , lnodeConfig->GetNid()
                      , lnodeConfig->GetFqdn()
                      , coresString
                      , lnodeConfig->GetProcessors()
                      , RoleTypeString( lnodeConfig->GetZoneType() )
                      );
            }
            if (nid != -1 && nid == lnodeConfig->GetNid())
            {
                break;
            }
            if (*node_name != '\0'
             && strcmp( node_name, lnodeConfig->GetName()) == 0)
            {
                break;
            }
        }
    }
}

void node_delete( int nid, char *node_name )
{
    const char method_name[] = "node_delete";

    bool integrating = false;
    char msgString[MAX_BUFFER] = { 0 };
    int pnid = -1;
    int count;
    MPI_Status status;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Deleting node:\n"
                      "   nid        = %d\n"
                      "   node_name  = %s\n"
                    , method_name, __LINE__, MyName
                    , nid, node_name );

    // Check that node_name exists in the configuration
    if (nid == -1)
    {
        pnid = get_pnid_by_node_name( node_name );
        if ( pnid == -1 )
        {
            sprintf( msgString, "[%s] Node %s is not configured!", MyName, node_name );
            write_startup_log( msgString );
            printf ("%s\n", msgString );
            return;
        }
    }
    else
    {
        pnid = get_pnid_by_nid( nid );
        if ( pnid == -1 )
        {
            sprintf( msgString, "[%s] Node %d is not configured!", MyName, nid );
            write_startup_log( msgString );
            printf ("%s\n", msgString );
            return;
        }
    }

    STATE state = State_Unknown;
    // Check monitors state of the target node
    if ( !get_node_state( nid, node_name, pnid, state, integrating ) )
    {
        sprintf( msgString, "[%s] Monitor does not have state information on nid=%d (%s)!"
                          , MyName, nid, node_name);
        write_startup_log( msgString );
        printf ("%s\n", msgString );
    }
    if ( integrating )
    {
        sprintf( msgString, "[%s] Node up in progress! Try again when node re-integration completes.", MyName);
        write_startup_log( msgString );
        printf ("%s\n", msgString );
        return;
    }
    if ( state != State_Unknown )
    {
        if ( state != State_Down )
        {
            sprintf( msgString, "[%s] Node %s is not in down state! (state=%s)", MyName, node_name, StateString(state) );
            write_startup_log( msgString );
            printf ("%s\n", msgString );
            return;
        }
    }

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending node delete message.\n",
                      method_name, __LINE__, MyName);

    assert(gp_local_mon_io);
    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        sprintf( msgString, "[%s] Unable to acquire message buffer.", MyName);
        write_startup_log( msgString );
        printf ("%s\n", msgString );
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NodeDelete;
    msg->u.request.u.node_delete.nid = MyNid;
    msg->u.request.u.node_delete.pid = MyPid;
    msg->u.request.u.node_delete.target_pnid = pnid;
    STRCPY( msg->u.request.u.node_delete.target_node_name, node_name );

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Sending node delete\n "
                    , method_name, __LINE__, MyName );

    gp_local_mon_io->send_recv( msg );
    if (gp_local_mon_io->iv_shutdown)
    {
        gp_local_mon_io->release_msg(msg);
        return;
    }
    status.MPI_TAG = msg->reply_tag;
    count = sizeof( *msg );

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code != MPI_SUCCESS)
            {
                if (msg->u.reply.u.generic.return_code == MPI_ERR_IO)
                {
                    printf( "[%s] Node deleted failed, could not access configuration database\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NAME)
                {
                    printf( "[%s] Node deleted failed, node does not exist in configuration in monitor\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_NO_MEM)
                {
                    printf( "[%s] Node deleted failed, could not process request in monitor\n"
                          , MyName );
                }
                else if (msg->u.reply.u.generic.return_code == MPI_ERR_INTERN)
                {
                    printf( "[%s] Node deleted failed, could not re-establish cluster configuration in monitor\n"
                          , MyName );
                }
                else
                {
                    printf( "[%s] Node deleted failed, error=%s\n"
                          , MyName, ErrorMsg(msg->u.reply.u.generic.return_code));
                }
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n"
                  , MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf( "[%s] Node deleted reply invalid, msg tag is %d, count= %d. \n"
              , MyName, status.MPI_TAG, count);
    }

    if (gp_local_mon_io)
        gp_local_mon_io->release_msg(msg);
}

void node_down( int nid, char *reason )
{
    const char method_name[] = "node_down";
    int pnid = -1;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] sending down node message.\n",
                     method_name, __LINE__, MyName);

    if ( get_node_name_by_nid( nid, node_name ) != 0 )
    {
        sprintf( msgString, "[%s] Invalid node id!\n", MyName);
        write_startup_log( msgString );
        printf ("[%s] Invalid node id!\n", MyName);
        return;
    }
    
    pnid = get_pnid_by_nid( nid );

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        sprintf( msgString, "[%s] Unable to acquire message buffer.\n", MyName);
        write_startup_log( msgString );
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = true;
    msg->u.request.type = ReqType_NodeDown;
    msg->u.request.u.down.nid = nid;
    STRCPY(msg->u.request.u.down.node_name, node_name);
    STRCPY(msg->u.request.u.down.reason, reason);

    gp_local_mon_io->send( msg );

    struct sigaction int_act, old_act;
    int_act.sa_sigaction = interrupt_handler;
    sigemptyset(&int_act.sa_mask);
    sigaddset (&int_act.sa_mask, SIGINT);
    int_act.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &int_act, &old_act);

    nodePending = true;
    nodePendingNid = nid;
    nodePendingPnid = pnid;
    STRCPY(nodePendingName, node_name);

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] Waiting for node down notice, node_name=%s\n",
                      method_name, __LINE__, MyName, node_name );

    nodePendingLock.lock();
    nodePendingLock.wait();
    nodePendingLock.unlock();

    sigaction (SIGINT, &old_act, NULL);

    nodePending = false;
    NodeState[nid] = false;
}

void node_info( int nid )
{
    int i;
    int count;
    bool display_pnode = false;
    int  last_nid = 0;
    MPI_Status status;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    bool needBanner = true;
    bool getMoreInfo = false;
    do
    {
        msg->type = MsgType_Service;
        msg->noreply = false;
        msg->reply_tag = REPLY_TAG;
        msg->u.request.type = ReqType_NodeInfo;
        msg->u.request.u.node_info.nid = MyNid;
        msg->u.request.u.node_info.pid = MyPid;
        msg->u.request.u.node_info.target_nid = nid;
        if ( !getMoreInfo )
        {
            msg->u.request.u.node_info.continuation = false;
        }

        gp_local_mon_io->send_recv( msg );
        count = sizeof( *msg );
        status.MPI_TAG = msg->reply_tag;

        getMoreInfo = false;
        if ((status.MPI_TAG == REPLY_TAG) &&
            (count == sizeof (struct message_def)))
        {
            if ((msg->type == MsgType_Service) &&
                (msg->u.reply.type == ReplyType_NodeInfo))
            {
                if (msg->u.reply.u.node_info.return_code == MPI_SUCCESS)
                {
                    if (msg->u.reply.u.node_info.num_returned)
                    {
                        if (needBanner)
                        {
                            printf ("[%s] Logical Nodes    = %d\n",MyName,msg->u.reply.u.node_info.num_nodes);
                            printf ("[%s] Physical Nodes   = %d\n",MyName,msg->u.reply.u.node_info.num_pnodes);
                            printf ("[%s] Spare Nodes      = %d\n",MyName,msg->u.reply.u.node_info.num_spares);
                            printf ("[%s] Available Spares = %d\n",MyName,msg->u.reply.u.node_info.num_available_spares);
                            printf ("[%s] NID Type        State    Processors   #Procs\n", MyName);
                            printf ("[%s]     PNID        State        #Cores  MemFree SwapFree CacheFree Name\n", MyName);
                            printf ("[%s] --- ----------- -------- ---------- -------- -------- --------- --------\n", MyName);
                            needBanner = false;
                        }

                        for (i=0; i < msg->u.reply.u.node_info.num_returned; i++)
                        {
                            if ( last_nid != -1 )
                            {
                                if ( (msg->u.reply.u.node_info.node[i].pnid !=
                                      msg->u.reply.u.node_info.node[i+1].pnid) ||
                                      i == (msg->u.reply.u.node_info.num_returned - 1) )
                                {
                                    display_pnode = true;
                                }
                            }
                            else
                            {
                                display_pnode = true;
                            }
                            if ( msg->u.reply.u.node_info.node[i].nid != -1 )
                            {
                                // Display logical node info
                                last_nid = msg->u.reply.u.node_info.node[i].nid;
                                switch( msg->u.reply.u.node_info.node[i].state )
                                {
                                case State_Up:
                                    NodeState[i]=true;
                                    break;
                                case State_Down:
                                    NodeState[i]=false;
                                    break;
                                default:
                                    NodeState[i]=false;
                                }
                                if ( msg->u.reply.u.node_info.node[i].state == State_Down )
                                {
                                     //      "[%s] NID Type      State                                                   \n"
                                     //      "[%s] --- ----------- -------- ---------- -------- -------- --------- --------\n"
                                     printf ("[%s] %3.3d %-11s %-8s\n",
                                         MyName,
                                         msg->u.reply.u.node_info.node[i].nid,
                                         ZoneTypeString( msg->u.reply.u.node_info.node[i].type ),
                                         StateString( msg->u.reply.u.node_info.node[i].state ) );
                                }
                                else
                                {
                                    //      "[%s] NID Type      State    Processors   #Procs\n");
                                    //      "[%s] --- ----------- -------- ---------- -------- -------- --------- --------\n"
                                    printf ("[%s] %3.3d %-11s %-8s   %8d %8d\n",
                                        MyName,
                                        msg->u.reply.u.node_info.node[i].nid,
                                        ZoneTypeString( msg->u.reply.u.node_info.node[i].type ),
                                        StateString( msg->u.reply.u.node_info.node[i].state ),
                                        msg->u.reply.u.node_info.node[i].processors,
                                        msg->u.reply.u.node_info.node[i].process_count );
                                }
                            }
                            if ( display_pnode )
                            {
                                // Display physical node info
                                display_pnode = false;
                                StateString( msg->u.reply.u.node_info.node[i].pstate );
                                if ( msg->u.reply.u.node_info.node[i].pstate == State_Down )
                                {
                                     //      "[%s]     PNID      State                                                   \n"
                                     //      "[%s] --- ----------- -------- ---------- -------- -------- --------- --------\n"
                                     printf ("[%s]     %3.3d         %-8s                                        %s\n",
                                         MyName,
                                         msg->u.reply.u.node_info.node[i].pnid,
                                         StateString( msg->u.reply.u.node_info.node[i].pstate ),
                                         msg->u.reply.u.node_info.node[i].node_name );
                                }
                                else
                                {
                                    //      "[%s]     PNID      State        #Cores  MemFree SwapFree CacheFree Name    \n"
                                    //      "[%s] --- ----------- -------- ---------- -------- -------- --------- --------\n"
                                    printf ("[%s]     %3.3d         %-8s         %2d %8d %8d  %8d %s\n",
                                        MyName,
                                        msg->u.reply.u.node_info.node[i].pnid,
                                        msg->u.reply.u.node_info.node[i].spare_node?"Spare":StateString( msg->u.reply.u.node_info.node[i].pstate ),
                                        msg->u.reply.u.node_info.node[i].cores,
                                        msg->u.reply.u.node_info.node[i].memory_free,
                                        msg->u.reply.u.node_info.node[i].swap_free,
                                        msg->u.reply.u.node_info.node[i].cache_free,
                                        msg->u.reply.u.node_info.node[i].node_name );
                                }
                            }
                        }
                        getMoreInfo = (msg->u.reply.u.node_info.num_returned == MAX_NODE_LIST);
                        if (getMoreInfo)
                        {  // Since we got the maximum number of node info
                           // entries there may be additional node info
                           // entries to retrieve.  Populate the request.
                           msg->u.request.u.node_info.last_nid =
                              msg->u.reply.u.node_info.last_nid;
                           msg->u.request.u.node_info.last_pnid =
                              msg->u.reply.u.node_info.last_pnid;
                           msg->u.request.u.node_info.continuation = true;
                        }
                    }
                }
                else
                {
                    printf ("[%s] NodeInfo failed, error=%s\n", MyName,
                        ErrorMsg(msg->u.reply.u.node_info.return_code));
                }
            }
            else
            {
                printf
                    ("[%s] Invalid MsgType(%d)/ReplyType(%d) for NodeInfo message\n",
                     MyName, msg->type, msg->u.reply.type);
            }
        }
        else
        {
            printf ("[%s] NodeInfo reply message invalid\n", MyName);
        }
    }
    while (getMoreInfo);

    gp_local_mon_io->release_msg(msg);
}

int node_up( int nid, bool nowait )
{
    const char method_name[] = "node_up";
    bool integrating = false;
    int pnid = -1;
    int rc = -1;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };

    // If this is a real cluster
    if ( !VirtualNodes )
    {
        if ( get_node_name_by_nid( nid, node_name ) != 0 )
        {
            sprintf( msgString, "[%s] Invalid node id!\n", MyName);
            write_startup_log( msgString );
            printf ("[%s] Invalid node id!\n", MyName);
            return( rc ) ;
        }
        
        // Get current physical state of all nodes
        if ( !update_node_state( node_name, false ) )
        {
            return( rc ) ;
        }
        NodeState_t nodeState;
        // Check physical state of the target node
        if ( get_pnode_state( node_name, nodeState ) )
        {
            if ( nodeState != StateUp )
            {
                sprintf( msgString, "[%s] Node %s is not available.", MyName, node_name);
                write_startup_log( msgString );
                printf("[%s] Node %s is not available.\n", MyName, node_name);
                return( rc ) ;
            }
        }
        STATE state;
        // Check monitors state of the target node
        if ( !get_node_state( nid, node_name, pnid, state, integrating ) )
        {
            return( rc ) ;
        }
        if ( integrating )
        {
            sprintf( msgString, "[%s] Node up in progress! Try again when node re-integration completes.", MyName);
            write_startup_log( msgString );
            printf ("[%s] Node up in progress! Try again when node re-integration completes.\n", MyName );
            return( rc ) ;
        }
        if ( state != State_Down )
        {
            sprintf( msgString, "[%s] Node %s is not in down state! (state=%s)", MyName, node_name, StateString(state) );
            write_startup_log( msgString );
            printf ("[%s] Node %s is not in down state! (state=%s)\n", MyName, node_name, StateString(state) );
            return( rc ) ;
        }

        if ( SpareNodeColdStandby )
        {
            // Check monitors state of all logical nodes of the spare set.
            // There must be at least one logical node down that is associated.
            // with the spare set or the command is rejected (cold standby).
            // Otherwise, the spare node comes up and it becomes a hot standby
            // which is not currenly supported.
            STATE spare_set_state = State_Up;
            if ( !get_spare_set_state( node_name, spare_set_state ) )
            {
                return( rc ) ;
            }
            if ( spare_set_state != State_Down )
            {
                sprintf( msgString, "[%s] No nodes in the down state for spare node activation on %s!", MyName, node_name );
                write_startup_log( msgString );
                printf ("[%s] No nodes in the down state for spare node activation on %s!\n", MyName, node_name );
                return( rc ) ;
            }
        }

        // remove shared segment on the node
        char cmd[256];
        sprintf(cmd, "pdsh -w %s \"sqipcrm %s >> $TRAF_LOG/node_up_%s.log\"", node_name, node_name, node_name);
        system(cmd);

        // Start a monitor process on the node
        if ( start_monitor( node_name, false, true ) )
        {
            sprintf( msgString, "[%s] Unable to start monitor process in node %s.", MyName, node_name );
            write_startup_log( msgString );
            printf ("[%s] Unable to start monitor process in node %s.\n", MyName, node_name );
            return( rc ) ;
        }
    }
    else
    {
        STATE state;
        if ( !get_node_state( nid, NULL, pnid, state, integrating ) )
        {
            return( rc ) ;
        }
        if ( state != State_Down )
        {
            sprintf( msgString, "[%s] Node is not in down state! (nid=%d, state=%s)", MyName, nid, StateString(state) );
            write_startup_log( msgString );
            printf ("[%s] Node is not in down state! (nid=%d, state=%s)\n", MyName, nid, StateString(state) );
            return( rc ) ;
        }
    }
    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending up node message.\n",
                      method_name, __LINE__, MyName);

    assert(gp_local_mon_io);
    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        sprintf( msgString, "[%s] Unable to acquire message buffer.", MyName);
        write_startup_log( msgString );
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return( rc ) ;
    }

    msg->type = MsgType_Service;
    msg->u.request.type = ReqType_NodeUp;
    msg->u.request.u.up.nid = nid;

    // If this is a real cluster
    if ( !VirtualNodes )
    {
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf( "%s@%d [%s] %s node up successful, rtn=%d\n ",
                 method_name, __LINE__, MyName, node_name,
                 msg->u.reply.u.generic.return_code );

        sprintf( msgString, "[%s] Node %s is merging to existing cluster.",
               MyName, node_name);
        write_startup_log( msgString );
        printf ("[%s] %s - Node %s is merging to existing cluster.\n",
                 MyName, time_string(), node_name );

        if ( ! nowait )
        {
            struct sigaction int_act, old_act;
            int_act.sa_sigaction = interrupt_handler;
            sigemptyset(&int_act.sa_mask);
            sigaddset (&int_act.sa_mask, SIGINT);
            int_act.sa_flags = SA_SIGINFO;
            sigaction (SIGINT, &int_act, &old_act);

            nodePending = true;
            nodePendingPnid = pnid;
            STRCPY(nodePendingName, node_name);

            nodePendingLock.lock();
            nodePendingLock.wait();
            nodePendingLock.unlock();

            sigaction (SIGINT, &old_act, NULL);

            nodePending = false;
        }
        rc = MPI_SUCCESS;
    }
    else
    {
        msg->noreply = true;
        STRCPY( msg->u.request.u.up.node_name, Node[nid] );
        gp_local_mon_io->send( msg );
        rc = 0;
    }

    gp_local_mon_io->release_msg( msg );

    if ( nid != -1 && get_pnid( nid ) == MyPNid )
    {
        // this process is no longer valid in the monitor and needs to be restarted.
        sleep(1);
        exit(0);
    }

    return( rc ) ;
}

void persist_config( char *prefix )
{
    bool foundConfig = false;
    char persist_config_buf[TC_PERSIST_VALUE_MAX*2];
    char process_name_str[TC_PERSIST_VALUE_MAX];
    char process_type_str[TC_PERSIST_VALUE_MAX];
    char program_name_str[TC_PERSIST_VALUE_MAX];
    char program_args_str[TC_PERSIST_VALUE_MAX];
    char requires_dtm_str[TC_PERSIST_VALUE_MAX];
    char stdout_str[TC_PERSIST_VALUE_MAX];
    char persist_retries_str[TC_PERSIST_VALUE_MAX];
    char persist_zones_str[TC_PERSIST_VALUE_MAX];
    CPersistConfig *persistConfig;

    persistConfig = ClusterConfig.GetFirstPersistConfig();
    if (persistConfig)
    {
        for ( ; persistConfig; persistConfig = persistConfig->GetNext() )
        {
            if (*prefix == '\0' ||
                 strcasecmp( prefix, persistConfig->GetPersistPrefix()) == 0)
            {
                foundConfig = true;
                snprintf( process_name_str, sizeof(process_name_str)
                        , "%s_%s    = %s%s"
                        , persistConfig->GetPersistPrefix()
                        , PERSIST_PROCESS_NAME_KEY
                        , persistConfig->GetProcessNamePrefix()
                        , persistConfig->GetProcessNameFormat()
                        );
                snprintf( process_type_str, sizeof(process_type_str)
                        , "%s_%s    = %s"
                        , persistConfig->GetPersistPrefix()
                        , PERSIST_PROCESS_TYPE_KEY
                        , PersistProcessTypeString(persistConfig->GetProcessType())
                        );
                snprintf( program_name_str, sizeof(program_name_str)
                        , "%s_%s    = %s"
                        , persistConfig->GetPersistPrefix()
                        , PERSIST_PROGRAM_NAME_KEY
                        , persistConfig->GetProgramName()
                        );
                snprintf( program_args_str, sizeof(program_args_str)
                        , "%s_%s    = %s"
                        , persistConfig->GetPersistPrefix()
                        , PERSIST_PROGRAM_ARGS_KEY
                        , persistConfig->GetProgramArgs()
                        );
                snprintf( requires_dtm_str, sizeof(requires_dtm_str)
                        , "%s_%s    = %s"
                        , persistConfig->GetPersistPrefix()
                        , PERSIST_REQUIRES_DTM
                        , persistConfig->GetRequiresDTM()?"Y":"N"
                        );
                snprintf( stdout_str, sizeof(stdout_str)
                        , "%s_%s          = %s%s"
                        , persistConfig->GetPersistPrefix()
                        , PERSIST_STDOUT_KEY
                        , persistConfig->GetStdoutPrefix()
                        , persistConfig->GetStdoutFormat()
                        );
                snprintf( persist_retries_str, sizeof(persist_retries_str)
                        , "%s_%s = %d,%d"
                        , persistConfig->GetPersistPrefix()
                        , PERSIST_RETRIES_KEY
                        , persistConfig->GetPersistRetries()
                        , persistConfig->GetPersistWindow()
                        );
                snprintf( persist_zones_str, sizeof(persist_zones_str)
                        , "%s_%s   = %s"
                        , persistConfig->GetPersistPrefix()
                        , PERSIST_ZONES_KEY
                        , persistConfig->GetZoneFormat()
                        );
                snprintf( persist_config_buf, sizeof(persist_config_buf)
                        , "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n"
                        , process_name_str
                        , process_type_str
                        , program_name_str
                        , program_args_str
                        , requires_dtm_str
                        , stdout_str
                        , persist_retries_str
                        , persist_zones_str
                        );
                if (strcasecmp( prefix, persistConfig->GetPersistPrefix()) == 0)
                {
                    printf ("%s", persist_config_buf);
                    break;
                }
                if (persistConfig->GetNext())
                {
                    printf ("%s\n", persist_config_buf);
                }
                else
                {
                    printf ("%s", persist_config_buf);
                }
            }
        }
    }
    if (!foundConfig)
    {
        printf("[%s] %s - Persistent process configuration does not exist\n"
              , MyName, time_string() );
    }
}

void persist_config_keys( void )
{
    char persist_config_str[MAX_TOKEN];
    CPersistConfig *persistConfig;

    persistConfig = ClusterConfig.GetFirstPersistConfig();
    if (persistConfig)
    {
        snprintf( persist_config_str, sizeof(persist_config_str)
                , "%s=", PERSIST_PROCESS_KEYS );
        for ( ; persistConfig; persistConfig = persistConfig->GetNext() )
        {
            strcat( persist_config_str, persistConfig->GetPersistPrefix() );
            if ( persistConfig->GetNext() )
            {
                strcat( persist_config_str, "," );
            }
        }
        printf ("%s\n", persist_config_str);
    }
    else
    {
        printf ("[%s] Configuration keys for persistent process do not exist\n", MyName);
    }
}

void persist_info( char *prefix )
{
    bool foundConfig = false;
    bool usePattern = false;
    char process_name_str[MAX_TOKEN];
    CPersistConfig *persistConfig;

    persistConfig = ClusterConfig.GetFirstPersistConfig();
    if (persistConfig)
    {
        for ( ; persistConfig; persistConfig = persistConfig->GetNext() )
        {
            if (*prefix == '\0' ||
                 strcasecmp( prefix, persistConfig->GetPersistPrefix()) == 0)
            {
                if (strlen(persistConfig->GetProcessNameFormat()) != 0)
                {
                    // Build pattern string
                    usePattern = true;
                    snprintf( process_name_str, sizeof(process_name_str)
                            , "\%s[0-9]+"
                            , persistConfig->GetProcessNamePrefix()
                            );
                }
                else
                {
                    usePattern = false;
                    snprintf( process_name_str, sizeof(process_name_str)
                            , "\%s"
                            , persistConfig->GetProcessNamePrefix()
                            );
                }
                //printf ("[%s] Persist process info for: %s\n", MyName, prefix);
                get_proc_info( -1
                             , -1
                             , process_name_str
                             , ProcessType_Undefined
                             , usePattern
                             , foundConfig ? false : true
                             );
                foundConfig = true;
                if (strcasecmp( prefix, persistConfig->GetPersistPrefix()) == 0)
                {
                    break;
                }
            }
        }
    }
    if (!foundConfig)
    {
        printf("[%s] %s - Persistent process configuration does not exist\n"
              , MyName, time_string() );
    }
}

bool persist_process_kill( CPersistConfig *persistConfig )
{
    const char method_name[] = "persist_process_kill";
    bool integrating = false;
    bool rs = false;
    char processName[MAX_PROCESS_NAME];
    char outfile[MAX_PROCESS_PATH];
    char persistRetries[MAX_PERSIST_VALUE_STR];
    char persistZones[MAX_VALUE_SIZE_INT];
    char programArgs[MAX_VALUE_SIZE_INT];
    int programArgc = 0;
    int nid;
    int pnid;
    int lnodesCount = 0;
    PROCESSTYPE process_type;
    STATE nodeState;

    nid = -1;
    processName[0] = '\0';       // The monitor will assign name if null
    outfile[0] = '\0';          // The monitor's default outfile is used
    process_type = ProcessType_Undefined;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d Persist process prefix=%s, name format=%s\n "
                    , method_name, __LINE__
                    , persistConfig->GetPersistPrefix()
                    , FormatNidString(persistConfig->GetProcessNameNidFormat()) );

    switch (persistConfig->GetProcessNameNidFormat())
    {
    case Nid_ALL:
        for (int i = 0; i < LNodesConfigMax && lnodesCount < NumLNodes; i++)
        {
            // Check monitors state of the target node
            rs = get_node_state( i, NULL, pnid, nodeState, integrating );
            lnodesCount++;
            if ( rs == false || nodeState != State_Up || integrating )
            {
                continue;
            }
            get_persist_process_attributes( persistConfig
                                          , i
                                          , process_type
                                          , processName
                                          , programArgc
                                          , programArgs
                                          , outfile
                                          , persistRetries
                                          , persistZones );
            if ( !find_process( processName ) )
            {
                printf( "[%s] %s - Persistent process %s does not exist\n"
                      , MyName, time_string()
                      , processName );
                continue;
            }
            kill_process( -1, -1, processName, true );
        }
        break;

    case Nid_RELATIVE:
        nid = 0;
        get_persist_process_attributes( persistConfig
                                      , nid
                                      , process_type
                                      , processName
                                      , programArgc
                                      , programArgs
                                      , outfile
                                      , persistRetries
                                      , persistZones );
        if ( !find_process( processName ) )
        {
            printf( "[%s] %s - Persistent process %s does not exist\n"
                  , MyName, time_string()
                  , processName );
            break;
        }
        kill_process( -1, -1, processName, true );
        break;
    case Nid_Undefined:
        nid = 0;
        get_persist_process_attributes( persistConfig
                                      , -1
                                      , process_type
                                      , processName
                                      , programArgc
                                      , programArgs
                                      , outfile
                                      , persistRetries
                                      , persistZones );
        if ( !find_process( processName ) )
        {
            printf( "[%s] %s - Persistent process %s does not exist\n"
                  , MyName, time_string()
                  , processName );
            break;
        }
        kill_process( -1, -1, processName, true );
        break;
    default:
        return(false);
    }
    return(true);
}

bool persist_process_start( CPersistConfig *persistConfig )
{
    const char method_name[] = "persist_process_start";
    bool integrating = false;
    bool rs = false;
    bool debug = false;
    bool nowait = false;
    char processName[MAX_PROCESS_NAME];
    char programArgs[MAX_VALUE_SIZE_INT];
    char programNameAndArgs[MAX_PROCESS_PATH+MAX_VALUE_SIZE_INT];
    char infile[MAX_PROCESS_PATH];
    char outfile[MAX_PROCESS_PATH];
    char outpath[MAX_PROCESS_PATH];
    char persistRetries[MAX_PERSIST_VALUE_STR];
    char persistZones[MAX_VALUE_SIZE_INT];
    int nid;
    int pid;
    int pnid;
    int programArgc = 0;
    int lnodesCount = 0;
    PROCESSTYPE process_type;
    int priority;
    STATE nodeState;

    nid = -1;
    priority = 0;
    processName[0] = '\0';       // The monitor will assign name if null
    programArgs[0] = '\0';
    infile[0] = '\0';           // The monitor's default infile is used
    outfile[0] = '\0';          // The monitor's default outfile is used
    process_type = ProcessType_Undefined;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d Persist process prefix=%s, name format=%s\n "
                    , method_name, __LINE__
                    , persistConfig->GetPersistPrefix()
                    , FormatNidString(persistConfig->GetProcessNameNidFormat()) );

    switch (persistConfig->GetProcessNameNidFormat())
    {
    case Nid_ALL:
        for (int i = 0; i < LNodesConfigMax && lnodesCount < NumLNodes; i++)
        {
            // Check monitors state of the target node
            rs = get_node_state( i, NULL, pnid, nodeState, integrating );
            lnodesCount++;
            if ( rs == false || nodeState != State_Up || integrating )
            {
                continue;
            }
            get_persist_process_attributes( persistConfig
                                          , i
                                          , process_type
                                          , processName
                                          , programArgc
                                          , programArgs
                                          , outfile
                                          , persistRetries
                                          , persistZones );
            if ( find_process( processName ) )
            {
                printf( "[%s] %s - Persistent process %s already exists\n"
                      , MyName, time_string()
                      , processName );
                continue;
            }
            if (programArgc)
            {
                sprintf( programNameAndArgs, "%s %s"
                       , persistConfig->GetProgramName()
                       , programArgs );
            }
            else
            {
                sprintf( programNameAndArgs, "%s"
                       , persistConfig->GetProgramName() );
            }
            snprintf(outpath, MAX_FILE_NAME, "%s/%s", getenv("TRAF_LOG"), outfile);
            pid = start_process( &i
                               , process_type
                               , processName
                               , debug
                               , priority
                               , nowait
                               , infile
                               , outpath
                               , programNameAndArgs );
                               //, (char *)persistConfig->GetProgramName() );
            if (pid > 0)
            {
                printf( "[%s] %s - Persistent process %s created\n"
                      , MyName, time_string()
                      , processName );
                if (process_type == ProcessType_DTM)
                {
                    DTMexists = true;
                }
            }
            else
            {
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf("%s@%d [%s] persistexec failed!\n",
                                 method_name, __LINE__, MyName);
            }
        }
        break;

    case Nid_RELATIVE:
        nid = 0;
        get_persist_process_attributes( persistConfig
                                      , nid
                                      , process_type
                                      , processName
                                      , programArgc
                                      , programArgs
                                      , outfile
                                      , persistRetries
                                      , persistZones );
        if ( find_process( processName ) )
        {
            printf( "[%s] %s - Persistent process %s already exists\n"
                  , MyName, time_string()
                  , processName );
            break;
        }
        if (programArgc)
        {
            sprintf( programNameAndArgs, "%s %s"
                   , persistConfig->GetProgramName()
                   , programArgs );
        }
        else
        {
            sprintf( programNameAndArgs, "%s"
                   , persistConfig->GetProgramName() );
        }
        snprintf(outpath, MAX_FILE_NAME, "%s/%s", getenv("TRAF_LOG"), outfile);
        pid = start_process( &nid
                           , process_type
                           , processName
                           , debug
                           , priority
                           , nowait
                           , infile
                           , outpath
                           , programNameAndArgs );
        if (pid > 0)
        {
            printf( "[%s] %s - Persistent process %s created\n"
                  , MyName, time_string()
                  , processName );
            if (process_type == ProcessType_DTM)
            {
                DTMexists = true;
            }
        }
        else
        {
            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf("%s@%d [%s] persistexec failed!\n",
                             method_name, __LINE__, MyName);
        }
        break;
    case Nid_Undefined:
        get_persist_process_attributes( persistConfig
                                      , -1
                                      , process_type
                                      , processName
                                      , programArgc
                                      , programArgs
                                      , outfile
                                      , persistRetries
                                      , persistZones );
        if ( find_process( processName ) )
        {
            printf( "[%s] %s - Persistent process %s already exists\n"
                  , MyName, time_string()
                  , processName );
            break;
        }
        if (programArgc)
        {
            sprintf( programNameAndArgs, "%s %s"
                   , persistConfig->GetProgramName()
                   , programArgs );
        }
        else
        {
            sprintf( programNameAndArgs, "%s"
                   , persistConfig->GetProgramName() );
        }
        // Find the first up nid
        for ( nid = 0; nid < NumLNodes; nid++ )
        {
            if (is_node_up( nid ))
            {
                break;
            }
        }
        snprintf(outpath, MAX_FILE_NAME, "%s/%s", getenv("TRAF_LOG"), outfile);
        pid = start_process( &nid
                           , process_type
                           , processName
                           , debug
                           , priority
                           , nowait
                           , infile
                           , outpath
                           , programNameAndArgs );
        if (pid > 0)
        {
            printf( "[%s] %s - Persistent process %s created\n"
                  , MyName, time_string()
                  , processName );
            if (process_type == ProcessType_DTM)
            {
                DTMexists = true;
            }
        }
        else
        {
            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf("%s@%d [%s] persist exec failed!\n"
                             , method_name, __LINE__, MyName);
        }
        break;
    default:
        return(false);
    }
    return(true);
}

void process_startup (int nid,char *port)
{
    struct message_def *saved_msg = msg;
    const char method_name[] = "process_startup";

    port = port;  //warning
    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] process_startup, nid: %d, MyNid: %d, lio: "
                      "%p\n", method_name, __LINE__, MyName, nid, MyNid,
                      (void *)gp_local_mon_io );

    gp_local_mon_io->iv_pid = MyPid;
    gp_local_mon_io->init_comm();

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing startup.\n", method_name,
                      __LINE__, MyName);


    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = true;
    msg->u.request.type = ReqType_Startup;
    msg->u.request.u.startup.nid = MyNid;
    msg->u.request.u.startup.pid = MyPid;
    msg->u.request.u.startup.verifier = MyVerifier;
    msg->u.request.u.startup.paired = false;
    STRCPY (msg->u.request.u.startup.process_name, MyName);
    STRCPY (msg->u.request.u.startup.port_name, MyPort);
    msg->u.request.u.startup.os_pid = getpid ();
    msg->u.request.u.startup.event_messages = true;
    msg->u.request.u.startup.system_messages = true;
    msg->u.request.u.startup.startup_size = sizeof(msg->u.request.u.startup);
    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending startup reply to monitor.\n",
                      method_name, __LINE__, MyName);
    gp_local_mon_io->send( msg );

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] Startup completed\n", method_name,
                      __LINE__, MyName);
    msg = saved_msg;
}

// Keep string location in sync with PROCESSTYPE typedef in msgdef.h
const char * processTypeStr [] = {"???", "TSE", "DTM", "ASE", "GEN", "NS", "WDG", "AMP", "BO", "VR", "CS", "SPX", "SSMP", "PSD", "SMS", "TMID", "PERS"};

void show_proc_info( void )
{
    int i;
    int j;
    char filename[MAX_PROCESS_PATH];

    for (i = 0; i < msg->u.reply.u.process_info.num_processes; i++)
    {
        // find just the filename in the program path
        filename[0] ='\0';
        for (j = strlen (msg->u.reply.u.process_info.process[i].program); j >= 0; j--)
        {
            if (msg->u.reply.u.process_info.process[i].program[j] == '/')
            {
                msg->u.reply.u.process_info.process[i].program[j] = '\0';
                strcpy (filename, &msg->u.reply.u.process_info.process[i].program[j + 1]);
                break;
            }
            else if ( j == 0 )
            {
                strcpy (filename, msg->u.reply.u.process_info.process[i].program );
            }
        }
        printf("[%s] %3.3d,%8.8d ",
               MyName,
               msg->u.reply.u.process_info.process[i].nid,
               msg->u.reply.u.process_info.process[i].pid
            );
        if (msg->u.reply.u.process_info.process[i].type >= ProcessType_Invalid)
        {
            msg->u.reply.u.process_info.process[i].type
                = ProcessType_Undefined;
        }
        printf("%3.3d %-4s %c%c%c%c%c%c%c %-12s %-12s %-15s\n",
               msg->u.reply.u.process_info.process[i].priority,
               processTypeStr[msg->u.reply.u.process_info.process[i].type],
               (msg->u.reply.u.process_info.process[i].event_messages?'E':'-'),
               (msg->u.reply.u.process_info.process[i].system_messages?'S':'-'),
               (msg->u.reply.u.process_info.process[i].pending_replication?'R':'-'),
               (msg->u.reply.u.process_info.process[i].pending_delete?'D':'-'),
               (msg->u.reply.u.process_info.process[i].state==State_Up?'A':'U'),
               (msg->u.reply.u.process_info.process[i].opened?'O':'-'),
               (msg->u.reply.u.process_info.process[i].paired?'P':
                (msg->u.reply.u.process_info.process[i].backup?'B':'-')
                   ),
               msg->u.reply.u.process_info.process[i].process_name,
               (msg->u.reply.u.process_info.process[i].parent_nid==-1?
                "NONE":msg->u.reply.u.process_info.process[i].parent_name
                   ),
               filename
            );
    }
}

//==========================
//==========================
//=================================================================

char *remove_white_space (char *cmd)
{
    char *ptr = cmd;

    while (ptr && *ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == ','))
    {
        ptr++;
    }

    return ptr;
}

void remove_trailing_white_space (char *buf)
{
    int i = strlen(buf);
    while (i >= 1 && buf[i-1] == ' ')
    {
        --i;
    }
    buf[i] = '\0';
}

void send_set_req( ConfigType type, char *name, char *key, char *value )
{
    int count;
    MPI_Status status;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    // send set request to monitor
    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Set;
    msg->u.request.u.set.nid = MyNid;
    msg->u.request.u.set.pid = MyPid;
    msg->u.request.u.set.verifier = MyVerifier;
    msg->u.request.u.set.process_name[0] = 0;
    msg->u.request.u.set.type = type;
    STRCPY(msg->u.request.u.set.group,name);
    STRCPY(msg->u.request.u.set.key,key);
    STRCPY(msg->u.request.u.set.value,value);

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code != MPI_SUCCESS)
            {
                printf ("[%s] Set failed, error=%s\n", MyName,
                ErrorMsg(msg->u.reply.u.generic.return_code));
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for Set message\n"
                  , MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Set reply message invalid\n", MyName);
    }

    gp_local_mon_io->release_msg(msg);
}

void shutdown (ShutdownLevel level)
{
    int count;
    MPI_Status status;
    const char method_name[] = "shutdown";

    if (find_DTM())
    {
        DTMexists = true;
    }
    else
    {
        DTMexists = false;
    }

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] sending shutdown message.\n", method_name,
                      __LINE__, MyName);

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Shutdown;
    msg->u.request.u.shutdown.nid = MyNid;
    msg->u.request.u.shutdown.pid = MyPid;
    msg->u.request.u.shutdown.level = level;

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                struct message_def *saved_msg = msg;
                exit_process();
                msg = saved_msg;
                Started = false;
                if (gp_local_mon_io)
                {
                    delete gp_local_mon_io;
                    gp_local_mon_io = NULL;
                }
                ShutdownClean = true;
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf ("%s@%d [%s] Shutdown successful.\n",
                                  method_name, __LINE__, MyName);
           }
            else
            {
                printf ("[%s] Shutdown failed, error=%s\n", MyName,
                     ErrorMsg(msg->u.reply.u.generic.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Shutdown message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Shutdown reply invalid.\n", MyName);
    }
    if (gp_local_mon_io)
        gp_local_mon_io->release_msg(msg);
}

int start_process (int *nid, PROCESSTYPE type, char *name, bool debug, int priority,
                   bool , char *infile, char *outfile, char *cmd_tail)
{
    int count;
    char delimiter;
    char token[MAX_ARG_SIZE];
    char program[MAX_PROCESS_PATH];
    char path[MAX_SEARCH_PATH];
    char ldpath[MAX_SEARCH_PATH];
    MPI_Status status;
    const char method_name[] = "start_process";

    LastNid = LastPid = -1;
    if (debug && *nid != 0)
    {
        if (*nid == -1)
        {
            *nid = 0;
        }
        else
        {
            printf ("[%s] Debug mode supported only for nid=0\n", MyName);
            return LastPid;
        }
    }


    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return LastPid;
    }

    cmd_tail = get_token (cmd_tail, token, &delimiter);
    strcpy (program, Wdir);
    cd_cmd (token, program);

    count = 0;
    while (*cmd_tail && count < MAX_ARGS)
    {
        cmd_tail = get_token (cmd_tail, token, &delimiter, (MAX_ARG_SIZE - 1),
                              false /* equal is not a delim */);
        strncpy (msg->u.request.u.new_process.argv[count], token,
                 MAX_ARG_SIZE - 1);
        msg->u.request.u.new_process.argv[count][MAX_ARG_SIZE - 1] = '\0';
        count++;
    }
    msg->u.request.u.new_process.argc = count;
    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf( "%s@%d [%s] starting process %s.\n"
                    , method_name, __LINE__, MyName, name );
    if ( trace_settings & TRACE_SHELL_CMD )
    {
        trace_printf("%s@%d - Program='%s' argc=%d\n"
                    , method_name, __LINE__
                    , program, count);
        int i = 0;
        while (i < count)
        {
            trace_printf("%s@%d - argv[%d]=%s\n"
                        , method_name, __LINE__
                        , i, msg->u.request.u.new_process.argv[i]);
            i++;
        }
    }
    strcpy( path, Path );
    if (strlen (path) + strlen (Wdir) + 1 > MAX_SEARCH_PATH - 1)
    {
        strcpy (path, Wdir);
        printf ("[%s] Warning: Path set to current working director\n",MyName);
    }
    else
    {
        strncat (path, ":", MAX_SEARCH_PATH-1);
        strncat (path, Wdir, MAX_SEARCH_PATH-1);
    }

    strcpy( ldpath, LDpath );
    if (strlen (ldpath) + strlen (Wdir) + 1 > MAX_SEARCH_PATH - 1)
    {
        strcpy (ldpath, Wdir);
        printf ("[%s] Warning: LD_LIBRARY_PATH set to current working director\n",MyName);
    }
    else
    {
        strncat (ldpath, ":", MAX_SEARCH_PATH-1);
        strncat (ldpath, Wdir, MAX_SEARCH_PATH-1);
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NewProcess;
    msg->u.request.u.new_process.nid = *nid;
    msg->u.request.u.new_process.type = type;
    msg->u.request.u.new_process.priority = priority;
    msg->u.request.u.new_process.backup = 0;
    msg->u.request.u.new_process.unhooked = true;
//    msg->u.request.u.new_process.nowait = nowait;
    msg->u.request.u.new_process.nowait = false;
    msg->u.request.u.new_process.tag = 0;
    msg->u.request.u.new_process.debug = (debug ? 1 : 0);
    STRCPY (msg->u.request.u.new_process.process_name, name);
    STRCPY (msg->u.request.u.new_process.path, path);
    STRCPY (msg->u.request.u.new_process.ldpath, ldpath);
    STRCPY (msg->u.request.u.new_process.program, program);
    STRCPY (msg->u.request.u.new_process.infile, infile);
    STRCPY (msg->u.request.u.new_process.outfile, outfile);
    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] send req to monitor with tag: %d, count: "
                      "%d.\n", method_name, __LINE__,MyName, REPLY_TAG, count);

    gp_local_mon_io->send_recv(msg);
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ( "%s@%d [%s] reply from monitor with tag: %d, count: "
                       "%d.\n", method_name, __LINE__, MyName,
                       status.MPI_TAG, count);

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_NewProcess))
        {
            if (msg->u.reply.u.new_process.return_code == MPI_SUCCESS)
            {
                LastNid = msg->u.reply.u.new_process.nid;
                LastPid = msg->u.reply.u.new_process.pid;
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf (
                         "%s@%d [%s] Started process successfully. Nid=%d, "
                         "Pid=%d, Process_name=%s, Verifier=%d,rtn=%d\n",
                         method_name, __LINE__,
                         MyName, msg->u.reply.u.new_process.nid,
                         msg->u.reply.u.new_process.pid,
                         msg->u.reply.u.new_process.process_name,
                         msg->u.reply.u.new_process.verifier,
                         msg->u.reply.u.new_process.return_code);
                *nid = msg->u.reply.u.new_process.nid;
            }
            else
            {
                printf ("[%s] NewProcess failed to spawn, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.new_process.return_code));
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for NewProcess message\n",
                   MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] NewProcess reply message invalid\n", MyName);
    }

    gp_local_mon_io->release_msg(msg);
    return LastPid;
}

bool start_monitor( char *cmd_tail, bool warmstart, bool reintegrate )
{
    bool status = false;

    int i;
    int rc = MPI_SUCCESS;
    int idx;
    char *argv[100];
    char token[MAX_TOKEN];
    char delimiter;
    char *env;
    char home[MAX_PROCESS_PATH];
    char nodelist[MAX_NODES*MPI_MAX_PROCESSOR_NAME];
    char np[6];
    char path[MAX_SEARCH_PATH];
    char sqroot[MAX_PROCESS_PATH];
    char sqvar[MAX_PROCESS_PATH];
    pid_t os_pid;
    const char method_name[] = "start_monitor";
    // Set working directory for monitor if needed
    MonCwd monCwd;

    // Ensure that we are on a node that is part of the configuration
    char mynode[MPI_MAX_PROCESSOR_NAME];
    gethostname(mynode, MPI_MAX_PROCESSOR_NAME);

    //JIRA: TRAFODION-1854, hostname contains upper case letters
    //change all char in mynode to lowercase
    //since cluster config strings will all be in lowercase, see CTokenizer::NormalizeCase()
    if(!VirtualNodes)  // for VirtualNodes, it use same gethostname, so do not tolower
    {
        char *tmpptr = mynode;
        while ( *tmpptr )
        {
            *tmpptr = tolower( *tmpptr );
            tmpptr++;
        }
    }
#if 1
    bool nodeInConfig = false;
    for ( i = 0; i < ClusterConfig.GetPNodesConfigMax(); i++ )
    {
        if ( CPNodeConfigContainer::hostnamecmp( mynode, PNode[i]) == 0 )
        {
            nodeInConfig = true;
            break;
        }
    }
    if ( !nodeInConfig )
    {
        printf ("[%s] Cannot start monitor from node '%s' since it is not member of the cluster configuration or 'hostname' string does not match configuration string.\n", MyName, mynode);
        printf ("[%s] Configuration node names:\n", MyName);
        for ( i = 0; i < NumNodes; i++ )
        {
            printf ("[%s]    '%s'\n", MyName, PNode[i]);
        }
        return true;
    }
#else
    // TODO: Need to evaluate the proper way to handle JIRA:TRAFODION-1854
    //       with the real elasticity implementation
    CPNodeConfig *pnodeConfig = ClusterConfig.GetPNodeConfig( mynode );
    if ( !pnodeConfig )
    {
        printf ("[%s] Cannot start monitor from node '%s' since it is not member of the cluster configuration or 'hostname' string does not match configuration string.\n", MyName, mynode);
        printf ("[%s] Configuration node names:\n", MyName);
        CPNodeConfig *pnodeConfig = ClusterConfig.GetFirstPNodeConfig();
        while (pnodeConfig)
        {
            printf ("[%s]    '%s'\n", MyName, pnodeConfig->GetName());
            pnodeConfig = pnodeConfig->GetNext();
        }
        return true;
    }
#endif

    if (gp_local_mon_io)
    {   // Ensure the monitor port name file does not exist.  We use the
        // file name's existence to indicate whether or not the monitor
        // has started.
        if ( !reintegrate )
        {
            // We are re-integrating another monitor, so don't remove this
            // monitor's port file as it will have to initiate re-integration
            unlink(gp_local_mon_io->mon_port_fname());
        }
    }

    // setup arguments to process
    idx=0;
    argv[idx] = (char *) "mpirun";
    argv[idx+1] = (char *) "-disable-auto-cleanup";
    idx++;
    argv[idx+1] = (char *) "-demux";
    argv[idx+2] = (char *) "select";
    idx+=2;
    argv[idx+1] = (char *) "-env";
    argv[idx+2] = (char *) "SQ_IC";
    argv[idx+3] = (char *) "TCP";
    idx+=3;
    argv[idx+1] = (char *) "-env";
    argv[idx+2] = (char *) "MPI_ERROR_LEVEL";
    argv[idx+3] = (char *) "2";
    idx+=3;
    if (Measure == 1)
    {
        argv[idx+1] = (char *) "-env";
        argv[idx+2] = (char *) "SQ_MEASURE";
        argv[idx+3] = (char *) "1";
        idx+=3;
    }
    else if (Measure == 2)
    {
        argv[idx+1] = (char *) "-env";
        argv[idx+2] = (char *) "SQ_MEASURE";
        argv[idx+3] = (char *) "2";
        idx+=3;
    }
    env=getenv("SQ_PIDMAP");
    if (env && *env == '1')
    {
        argv[idx+1] = (char *) "-env";
        argv[idx+2] = (char *) "SQ_PIDMAP";
        argv[idx+3] = (char *) "1";
        idx+=3;
    }
    env=getenv("SQ_SEAMONSTER");
    if (env && *env == '1')
    {
        argv[idx+1] = (char *) "-env";
        argv[idx+2] = (char *) "SQ_SEAMONSTER";
        argv[idx+3] = (char *) "1";
        idx+=3;
    }
    if (NameServerEnabled)
    {
        argv[idx+1] = (char *) "-env";
        argv[idx+2] = (char *) "SQ_NAMESERVER_ENABLED";
        argv[idx+3] = (char *) "1";
        idx+=3;
    }
    argv[idx+1] = (char *) "-env";
    argv[idx+2] = (char *) "MPI_TMPDIR";
    env = getenv ("HOME");
    strcpy (home, (env?env:""));
    env = getenv ("MPI_TMPDIR");
    strcpy (path, (env?env:home));
    argv[idx+3] = path;
    idx+=3;
    argv[idx+1] = (char *) "-env";
    argv[idx+2] = (char *) "TRAF_HOME";
    env=getenv("TRAF_HOME");
    strcpy (sqroot, (env?env:""));
    argv[idx+3] = sqroot;
    idx+=3;

    //    argv[idx+1] = (char *) "-e";
    //    argv[idx+2] = (char *) "MPI_MALLOCLIB=1";
    //    idx+=2;
    // add env from properties file
    // do it here so that variables can be overwritten
    char **xvals = NULL;
    MON_Props xprops(true);
    strcpy (sqvar, getenv("TRAF_CONF"));
    char *envfile = new char [strlen(sqvar)+20];
    strcpy(envfile, sqvar);
    strcat(envfile, "/monitor.env");
    xprops.load(envfile);
    delete [] envfile;
    MON_Smap_Enum xenum(&xprops);
    int xsize = xprops.size();
    int xinx;
    if (xsize > 0) {
        printf("[%s] - Warning using monitor.env\n",MyName);
        xvals = new char*[2*xsize];
        xinx = 0;
        while (xenum.more())
        {
            char *xkey = xenum.next();
            const char *xvalue = xprops.get(xkey);
            xvals[xinx] = new char[strlen(xkey)+1];
            xvals[xinx+1] = new char[strlen(xvalue)+1];
            strcpy(xvals[xinx], xkey);
            strcpy(xvals[xinx+1], xvalue);
            argv[idx+1] = (char *) "-env";
            argv[idx+2] = xvals[xinx];
            argv[idx+3] = xvals[xinx+1];
            idx+=3;
            xinx+=2;
        }
    }

    // Refresh the current cluster state
    if ( !VirtualNodes )
    {
        NodeState_t nodeState;
        if ( reintegrate )
        {
            strcpy( nodelist, cmd_tail );
        }
        else
        {
            if ( !update_cluster_state( true ) )
            {
                if (xvals != NULL)
                {
                   for (xinx = 0; xinx < xsize; xinx++)
                       delete [] xvals[xinx];
                   delete [] xvals;
                }
                return true;
            }
            bool copiedToList =false;
            for(i=0,nodelist[0]='\0'; i < ClusterConfig.GetPNodesConfigMax(); i++)
            {
                if (copiedToList)
                {
                    copiedToList =false;
                    strcat(nodelist, ",");
                }
                if ( strlen(PNode[i]) == 0 ) continue;
                if ( get_pnode_state( PNode[i], nodeState ) )
                {
                    if ( nodeState == StateUp )
                    {
                        copiedToList =true;
                        strcat(nodelist,PNode[i]);
                    }
                }
            }
        }
    }
    CurNodes = NumNodes - NumDown;
    argv[idx+1] = (char *) "-np";
    if ( reintegrate )
    {
        // Re-integrating one monitor process only
        sprintf( np,"%d",1 );
    }
    else
    {
        sprintf(np,"%d",CurNodes);
    }
    argv[idx+2] = np;
    idx+=2;
    if (!VirtualNodes) {
        argv[idx+1] = (char *) "-hosts";
        argv[idx+2] = nodelist;
        idx+=2;
    }

    char fname[MAX_PROCESS_PATH];
    // find absoute path to monitor from path
    env=getenv("PATH");
    char monpath[PATH_MAX+1];
    strncpy(monpath, env, PATH_MAX);
    monpath[PATH_MAX] = '\0';
    char *monbeg = monpath;
    char *monend;
    struct stat monstatbuf;
    int monstaterr;
    do
    {
        monend = strchr(monbeg, ':');
        if (monend != NULL)
            *monend = '\0';
        if (*monbeg == '/')
            sprintf(fname, "%s/monitor", monbeg);
        else // transform relative-to-absolute
            sprintf(fname, "%s/%s/monitor", getenv("PWD"), monbeg);

        monstaterr = lstat(fname, &monstatbuf);
        if (monstaterr == 0)
        {
            if ( S_ISREG(monstatbuf.st_mode) && !access(fname, X_OK))
            {   // Should be allowed to execute this program
                break;
            }
        }
        monbeg = &monend[1];
    } while (monend != NULL);

    if (monend == NULL)
    {
        if (xvals != NULL)
        {
            for (xinx = 0; xinx < xsize; xinx++)
                delete [] xvals[xinx];
            delete [] xvals;
        }
        // log error and quit
        printf ("[%s] Cannot find valid monitor executable in PATH.\n", MyName);
        return true;
    }

    argv[idx+1]=fname;
    idx++;

    if (warmstart)
    {
        argv[idx+1] = (char *) "WARM";
    }
    else
    {
        argv[idx+1] = (char *) "COLD";
    }
    idx++;
    if ( reintegrate )
    {
        // NEW monitor argv1 argv2 argv3 argv4 argv5:
        // monitor -integrate <creator-monitor-port> <creator-shell-pid> <creator_shell_verifier>
        argv[idx+1] = (char *) "-integrate";
        char mon_port[MPI_MAX_PORT_NAME];
        strcpy( mon_port, gp_local_mon_io->mon_port() );
        argv[idx+2] = mon_port;
        char creator_shell_pid[MPI_MAX_PORT_NAME];
        sprintf( creator_shell_pid, "%d", MyPid );
        argv[idx+3] = creator_shell_pid;
        char creator_shell_verifier[MPI_MAX_PORT_NAME];
        sprintf( creator_shell_verifier, "%d", MyVerifier );
        argv[idx+4] = creator_shell_verifier;
        idx+=4;
        argv[idx+1] = NULL;
    }
    else
    {
        cmd_tail = get_token (cmd_tail, token, &delimiter);
        if ( isdigit(*token) )
        {
            argv[idx+1] = token;
            argv[idx+2] = NULL;
        }
        else if (strcmp (normalize_case (token), "trace") == 0)
        {
            argv[idx+1] = (char *) "TRACEFILE";
            if (isdigit (*cmd_tail))
            {
                argv[idx+2] = cmd_tail;
                argv[idx+3] = NULL;
            }
            else
            {
                argv[idx+2] = NULL;
            }
        }
        else
        {
            argv[idx+1] = NULL;
        }
    }

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        trace_printf("%s@%d" " - Program='" "%s" "' argc=" "%d" "\n", method_name, __LINE__, argv[0], idx);
        i = 1;
        while (argv[i] != NULL)
        {
            trace_printf("%s@%d" " - argv[" "%d" "]="  "%s" "\n", method_name, __LINE__, i, argv[i]);
            i++;
        }
    }

    os_pid = fork ();
    if (os_pid)
    {
        // I'm the parent
        if (os_pid == -1)
        {
            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf ("%s@%d [%s] Monitor fork() failed, errno=%d\n",
                         method_name, __LINE__, MyName, errno);
            rc = MPI_ERR_SPAWN;
        }
        else
        {
            rc = MPI_SUCCESS;
        }

        // Wait for mpirun to finish, get status
        int mpirunStatus;
        pid_t child;
        bool done = false;
        do
        {
            child = waitpid(os_pid, &mpirunStatus, WNOHANG);
            if (child == os_pid)
            {
                // mpirun completed
                if (WIFEXITED(mpirunStatus))
                {
                    // If mpirun exit status is not zero mpirun had an
                    // error trying to start the monitor.  Set "failure"
                    // status this start_monitor function.
                    status = WEXITSTATUS(mpirunStatus) != 0;
                    if (status)
                    {
                        rc = MPI_ERR_SPAWN;
                    }
                }
                else
                {
                    printf("[%s]Unexpected mpirun status=%d\n", MyName,
                           mpirunStatus);
                }
                done = true;
            }
            else if (child == 0)
            {  // mpirun has not yet changed state, delay.
                // do not wait for mpirun to complete when using mpich!
                done = true;
            }
            else
            {
                if ( trace_settings & TRACE_SHELL_CMD )
                {
                    if (child == -1)
                    {
                        trace_printf("[%s] waiting for mpirun: %s (%d)\n",
                                     MyName, strerror(errno), errno);
                    }
                    else
                    {
                        trace_printf("[%s] waiting for mpirun pid=%d but got "
                                     "pid=%d\n", MyName, os_pid, child);
                    }
                }
                done = true;
            }
        } while (not done);
    }
    else
    {
        // I'm the child process
        close(0);
        // remap mpirun stdout and stderr
        RedirectFd(1, mpirunOutFileName);
        RedirectFd(2, mpirunErrFileName);

        if (execvp ("mpirun", argv) == -1)
        {
            printf("[%s] - Error trying to execvp 'mpirun' with program=monitor: %s (%d)\n",MyName, strerror(errno), errno);
        }
    }
    if (xvals != NULL)
    {
       for (xinx = 0; xinx < xsize; xinx++)
           delete [] xvals[xinx];
       delete [] xvals;
    }

    if (rc == MPI_SUCCESS && !reintegrate )
    {
        strcpy(MyName,"SHELL");

        gv_ms_su_nid = MyNid;
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf ("%s@%d [%s] Local IO nid = %d\n",
                          method_name, __LINE__, MyName, MyNid);

        gp_local_mon_io = new Local_IO_To_Monitor( -1 );
        assert (gp_local_mon_io);
        SetCallbacks();

        // We always need to attach to our monitor first
        int portFileStatus;

        gp_local_mon_io->iv_monitor_down = true;
        gp_local_mon_io->iv_pid = -1;
        if ( gp_local_mon_io->wait_for_monitor( portFileStatus ) )
        {
            sleep(1);
            Attached = attach( MyNid, MyName, (char *) "shell");
            if ( Attached )
            {
                gp_local_mon_io->iv_pid = MyPid;

                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf ("%s@%d [%s] Monitor started successfully\n",
                                  method_name, __LINE__, MyName);
            }
            else
            {
                printf("[%s] Unable to communicate with monitor\n",MyName);
                status = true;
            }
        }
        else
        {
            if ( portFileStatus == ENOENT )
            {
                printf("[%s] Unable to communicate with monitor because "
                       "monitor port file %s is missing.\n", MyName,
                       gp_local_mon_io->mon_port_fname() );
            }
            else
            {
                printf("[%s] Unable to communicate with monitor because "
                       "monitor port file %s is not accessible (%s).\n",
                       MyName, gp_local_mon_io->mon_port_fname(),
                       strerror( portFileStatus ));
            }
            status = true;
        }
    }
    else
    {
        if ( !reintegrate )
        {
            status = true;
        }
    }

    return status;
}

char *timer ( bool start )
{
    static char timestr[18] = "dd:hh:mm:ss.xxxxx";
    static double base_time;
    int days, hours, mins;
    double seconds;

    if ( start )
    {
        base_time = MPI_Wtime ();
    }
    else
    {
        seconds = MPI_Wtime () - base_time;
        mins = (int) (seconds / 60.0);
        seconds = seconds - (mins * 60.0);
        hours = mins / 60;
        mins = mins - (hours * 60);
        days = hours / 24;
        hours = hours - (days * 24);

        sprintf (timestr, "%2.2d:%2.2d:%2.2d:%7.5f", days, hours, mins, seconds);
    }
    return timestr;
}

char *time_string( void )
{
    static char timestr[22] = "mm/dd/yyyy - hh:mm:ss";
    time_t mytime = time(NULL);
    struct tm *tmp = localtime( &mytime );

    strftime( timestr, sizeof(timestr), "%m/%d/%Y-%H:%M:%S", tmp );
    return( timestr ) ;
}

void write_startup_log( char *msg )
{
    char fname[PATH_MAX];
    char msgString[MAX_BUFFER] = { 0 };

    char *tmpDir = getenv( "TRAF_LOG" );
    if ( tmpDir )
    {
        snprintf( fname, sizeof(fname), "%s/mon_startup.log", tmpDir );
    }
    else
    {
        snprintf( fname, sizeof(fname), "./startup.log" );
    }

    int processMapFd = open( fname
                           , O_WRONLY | O_APPEND | O_CREAT
                           , S_IRUSR | S_IWUSR );
    if ( processMapFd == -1 )
    {  // File open error
        printf( "[%s] Error= Can't open %s, %s (%d).\n"
              , MyName, fname, strerror(errno), errno);
        return;
    }

    static char timestr[30] = "Thu May 31 15:04:00 PDT 2012:";
    time_t mytime = time(NULL);
    struct tm *tmp = localtime( &mytime );
    strftime( timestr, sizeof(timestr), "%a %b %e %T %Z %Y:", tmp );

    sprintf( msgString, "%s %s\n",timestr, msg );
    write( processMapFd, msgString, strlen(msgString));
    close( processMapFd );
}

char *cd_cmd (char *cmd_tail, char *wdir)
{
    int i;
    int length = (int) strlen (cmd_tail);
    char *ptr;

    normalize_slashes (cmd_tail);

    if (wdir != Wdir)
    {
        strcpy (wdir, Wdir);
    }

    if (length == 2 && cmd_tail[0] == '.' && cmd_tail[1] == '.')
    {
        for (i = (int) strlen (wdir); i >= 0; i--)
        {
            if (wdir[i] == '/')
            {
                wdir[i] = '\0';
                strcat (wdir, &cmd_tail[2]);
                break;
            }
        }
    }
    else if (length > 2 && cmd_tail[0] == '.' && cmd_tail[1] == '.'
             && cmd_tail[2] == '/')
    {
        ptr = cmd_tail;
        i = (int) strlen (wdir);
        while (ptr[0] == '.' && ptr[1] == '.')
        {
            for (; i >= 0; i--)
            {
                if (wdir[i] == '/')
                {
                    wdir[i] = '\0';
                    ptr = &ptr[2];
                    break;
                }
            }
            if (ptr[0] == '/' && ptr[1] == '.')
            {
                ptr++;
            }
        }
        if (ptr[0] == '\0' || ptr[0] == '/')
        {
            strcat (wdir, ptr);
        }
        else
        {
            printf ("[%s] Invalid wdir syntax!\n", MyName);
        }
    }
    else if (cmd_tail[0] == '/')
    {
        strcpy (wdir, cmd_tail);
    }
    else if (length > 1 && cmd_tail[0] == '.' && cmd_tail[1] == '/')
    {
        if (wdir[strlen (wdir) - 1] != '/')
        {
            strcat (wdir, "/");
        }
        strcat (wdir, &cmd_tail[2]);
    }
    else if (cmd_tail[0] == '.')
    {
        if (length != 1)
        {
            printf ("[%s] Invalid wdir syntax!\n", MyName);
        }
    }
    else if (length == 2 && cmd_tail[1] == ':')
    {
        strcpy (wdir, cmd_tail);
        strcat (wdir, "/");
    }
    else if (length > 1 && cmd_tail[1] == ':' && cmd_tail[2] == '/')
    {
        strcpy (wdir, cmd_tail);
    }
    else if (length > 2 && cmd_tail[1] == '/' && cmd_tail[2] == '/')
    {
        strcpy (wdir, cmd_tail);
    }
    else
    {
        if (wdir[strlen (wdir) - 1] != '/')
        {
            strcat (wdir, "/");
        }
        strcat (wdir, cmd_tail);
    }

    return wdir;
}

void dump_cmd (char *cmd_tail, char delimiter)
{
    int count;
    char *dir;
    char name[MAX_PROCESS_NAME] = {0};
    int nid;
    char path[MAX_PROCESS_PATH] = {0};
    char path2[MAX_PROCESS_PATH] = {0};
    int pid;
    MPI_Status status;
    char token[MAX_TOKEN];
    const char method_name[] = "dump_cmd";

    if (delimiter == '{')
    {
        delimiter = ' ';
        while (*cmd_tail && delimiter != '}')
        {
            cmd_tail = get_token(cmd_tail, token, &delimiter);
            normalize_case(token);
            if (strcmp(token, "path") == 0)
            {
                cmd_tail = get_token(cmd_tail, path2, &delimiter, MAX_PROCESS_PATH-1);
                dir = path2;
            }
            else
            {
                printf("[%s] Invalid dump options syntax!\n", MyName);
                return;
            }
        }
        if (*cmd_tail == '\0')
        {
            printf("[%s] Invalid dump syntax!\n", MyName);
            return;
        }
        else if (delimiter != '}')
        {
            printf("[%s] Invalid dump syntax - expecting '}'!\n", MyName);
            return;
        }
    } else
    {
        dir = getenv("SQ_SNAPSHOT_DIR");
        if (dir == NULL)
            dir = getenv("TRAF_LOG");
    }
    // convert to absolute path
    if (dir[0] == '/')
        strcpy(path, dir);
    else
        sprintf(path, "%s/%s", getenv("TRAF_LOG"), dir);

    if (*cmd_tail)
    {
        if (isdigit(*cmd_tail))
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            if (delimiter == ',')
            {
                nid = atoi(token);
                get_token(cmd_tail, token, &delimiter);
                pid = atoi(token);
            }
            else
            {
                printf("[%s] Invalid process Nid,Pid!\n", MyName);
                return;
            }
        } else
        {
            nid = pid = -1;
            get_token(cmd_tail, name, &delimiter, MAX_PROCESS_NAME-1);
        }
    } else
    {
        printf("[%s] No process identified!\n", MyName);
        return;
    }

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf("%s@%d [%s] sending dump message.\n",
                     method_name, __LINE__, MyName);

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Dump;
    msg->u.request.u.dump.nid = MyNid;
    msg->u.request.u.dump.pid = MyPid;
    msg->u.request.u.dump.verifier = MyVerifier;
    msg->u.request.u.dump.process_name[0] = 0;
    STRCPY(msg->u.request.u.dump.path, path);
    msg->u.request.u.dump.target_nid = nid;
    msg->u.request.u.dump.target_pid = pid;
    msg->u.request.u.dump.target_verifier = -1;
    STRCPY(msg->u.request.u.dump.target_process_name, name);

    gp_local_mon_io->send_recv( msg );
    if (gp_local_mon_io->iv_shutdown)
    {
        gp_local_mon_io->release_msg(msg);
        return;
    }
    status.MPI_TAG = msg->reply_tag;
    count = sizeof( *msg );

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof(struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Dump))
        {
            if (msg->u.reply.u.dump.return_code == MPI_SUCCESS)
            {
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf("%s@%d [%s] dumped process successfully. "
                                 "error=%s\n", method_name, __LINE__, MyName,
                                 ErrorMsg(msg->u.reply.u.dump.return_code));
                printf("dump file created@ %s\n",
                       msg->u.reply.u.dump.core_file);
            }
            else
            {
                printf("[%s] Dump process failed, error=%s\n", MyName,
                       ErrorMsg(msg->u.reply.u.dump.return_code));
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for Dump message\n",
                   MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf("[%s] Dump process reply invalid, msg tag is %d, count= %d. \n",
               MyName, status.MPI_TAG, count);
    }
    if (gp_local_mon_io)
        gp_local_mon_io->release_msg(msg);
}

void event_cmd (char *cmd_tail, char delimiter)
{
    int count;
    int nid;
    int pid;
    int event_id;
    char token[MAX_TOKEN];
    MPI_Status status;
    PROCESSTYPE process_type = ProcessType_Undefined;
    const char method_name[] = "event_cmd";

    // parse options
    if (delimiter == '{')
    {
        delimiter = ' ';
        while (*cmd_tail && delimiter != '}')
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "dtm") == 0)
            {
                process_type = ProcessType_DTM;
            }
            else if (strcmp (token, "tse") == 0)
            {
                process_type = ProcessType_TSE;
            }
            else if (strcmp (token, "ase") == 0)
            {
                process_type = ProcessType_ASE;
            }
            else if (strcmp (token, "amp") == 0)
            {
                process_type = ProcessType_AMP;
            }
            else if (strcmp (token, "bo") == 0)
            {
                process_type = ProcessType_Backout;
            }
            else if (strcmp (token, "vr") == 0)
            {
                process_type = ProcessType_VolumeRecovery;
            }
            else if (strcmp (token, "cs") == 0)
            {
                process_type = ProcessType_MXOSRVR;
            }
            else if (strcmp (token, "sms") == 0)
            {
                process_type = ProcessType_SMS;
            }
            else if (strcmp (token, "spx") == 0)
            {
                process_type = ProcessType_SPX;
            }
            else if (strcmp (token, "ssmp") == 0)
            {
                process_type = ProcessType_SSMP;
            }
            else if (strcmp (token, "tmid") == 0)
            {
                process_type = ProcessType_TMID;
            }
            else
            {
                printf ("[%s] Invalid process type!\n",MyName);
                return;
            }
        }
        if (delimiter != '}')
        {
            printf ("[%s] Invalid event option syntax!\n", MyName);
            return;
        }
    }

    // check if we have a event_id
    if (isdigit (*cmd_tail))
    {
        cmd_tail = get_token (cmd_tail, token, &delimiter);
        event_id = atoi (token);
    }
    else
    {
        printf ("[%s] Invalid event id!\n", MyName);
        return;
    }

    // check if we have a <nid,pid>
    if (delimiter == ' ')
    {
        cmd_tail = get_token (cmd_tail, token, &delimiter);
        if (delimiter == ',')
        {
            nid = atoi (token);
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            pid = atoi (token);
        }
        else
        {
            printf ("[%s] Invalid process Nid,Pid!\n", MyName);
            return;
        }
    }
    else if (*cmd_tail)
    {
        printf ("[%s] Invalid process Nid,Pid!\n", MyName);
        return;
    }
    else
    {
        nid = pid = -1;
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Event;
    msg->u.request.u.event.nid = MyNid;
    msg->u.request.u.event.pid = MyPid;
    msg->u.request.u.event.verifier = MyVerifier;
    msg->u.request.u.event.process_name[0] = 0;
    msg->u.request.u.event.target_nid = nid;
    msg->u.request.u.event.target_pid = pid;
    msg->u.request.u.event.target_verifier = -1;
    msg->u.request.u.event.target_process_name[0] = 0;
    msg->u.request.u.event.type = process_type;
    msg->u.request.u.event.event_id = event_id;
    if (*cmd_tail)
    {
        STRCPY(msg->u.request.u.event.data, cmd_tail);
    }
    else
    {
        msg->u.request.u.event.data[0] = '\0';
    }
    msg->u.request.u.event.length = strlen(msg->u.request.u.event.data);

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                if ( trace_settings & TRACE_SHELL_CMD )
                    trace_printf("%s@%d [%s] Event sent successfully.\n",
                                 method_name, __LINE__, MyName );
            }
            else
            {
                printf ("[%s] Event failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.generic.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Event message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Event reply message invalid\n", MyName);
    }

    gp_local_mon_io->release_msg(msg);
}

void exec_cmd (char *cmd, char delimiter)
{
    bool debug;
    bool nowait;
    bool startup_process;
    char *cmd_tail = cmd;
    char token[MAX_TOKEN];
    char name[MAX_PROCESS_NAME];
    char infile[MAX_PROCESS_PATH];
    char outfile[MAX_PROCESS_PATH];
    int nid;
    int pid;
    PROCESSTYPE process_type;
    int priority;
    const char method_name[] = "exec_cmd";

    // setup defaults
    name[0] = '\0';             // The monitor will assign name if null
    nid = -1;
    process_type = ProcessType_Generic;
    nowait = false;
    debug = false;
    priority = 0;
    infile[0] = '\0';           // The monitor's default infile is used
    outfile[0] = '\0';          // The monitor's default outfile is used
    if (*cmd && delimiter == '{')
    {
        startup_process = false;
    }
    else
    {
        startup_process = true;
    }

    // parse options
    if (delimiter == '{')
    {
        delimiter = ' ';
        while (*cmd_tail && delimiter != '}')
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "in") == 0)
            {
                cmd_tail = get_token (cmd_tail, infile, &delimiter, MAX_PROCESS_PATH-1);
            }
            else if (strcmp (token, "name") == 0)
            {
                cmd_tail = get_token (cmd_tail, name, &delimiter, MAX_PROCESS_NAME-1);
            }
            else if (strcmp (token, "nid") == 0)
            {
                cmd_tail = get_token (cmd_tail, token, &delimiter);
                nid = atoi (token);
            }
            else if (strcmp (token, "nowait") == 0)
            {
                nowait = true;
            }
            else if (strcmp (token, "out") == 0)
            {
                cmd_tail = get_token (cmd_tail, outfile, &delimiter, MAX_PROCESS_PATH-1);
            }
            else if (strcmp (token, "pri") == 0)
            {
                cmd_tail = get_token (cmd_tail, token, &delimiter);
                priority = atoi (token);
            }
            else if (strcmp (token, "debug") == 0)
            {
                debug = true;
            }
            else if (strcmp (token, "type") == 0)
            {
                cmd_tail = get_token (cmd_tail, token, &delimiter);
                normalize_case (token);
                if (strcmp (token, "dtm") == 0)
                {
                    process_type = ProcessType_DTM;
                }
                else if (strcmp (token, "tse") == 0)
                {
                    process_type = ProcessType_TSE;
                }
                else if (strcmp (token, "ase") == 0)
                {
                    process_type = ProcessType_ASE;
                }
                else if (strcmp (token, "amp") == 0)
                {
                    process_type = ProcessType_AMP;
                }
                else if (strcmp (token, "bo") == 0)
                {
                    process_type = ProcessType_Backout;
                }
                else if (strcmp (token, "vr") == 0)
                {
                    process_type = ProcessType_VolumeRecovery;
                }
                else if (strcmp (token, "cs") == 0)
                {
                    process_type = ProcessType_MXOSRVR;
                }
                else if (strcmp (token, "sms") == 0)
                {
                    process_type = ProcessType_SMS;
                }
                else if (strcmp (token, "spx") == 0)
                {
                    process_type = ProcessType_SPX;
                }
                else if (strcmp (token, "ssmp") == 0)
                {
                    process_type = ProcessType_SSMP;
                }
                else if (strcmp (token, "tmid") == 0)
                {
                    process_type = ProcessType_TMID;
                }
                else
                {
                    printf ("[%s] Invalid process type!\n",MyName);
                    delimiter = ' ';
                    break;
                }
            }
            else
            {
                printf ("[%s] Invalid exec options syntax!\n", MyName);
                delimiter = ' ';
                break;
            }
        }
        if (*cmd_tail == '\0')
        {
            printf ("[%s] Invalid exec syntax!\n", MyName);
        }
        else if (delimiter == '}')
        {
            startup_process = true;
        }
    }

    // start process
    if (startup_process)
    {
        pid = start_process ( &nid,
                              process_type,
                              name,
                              debug,
                              priority,
                              nowait,
                              infile,
                              outfile,
                              cmd_tail );
        if (pid > 0)
        {
            if (!nowait && !debug)
            {
                get_server_death (nid, pid);
                LastNid = LastPid = -1;
            }
            if (process_type == ProcessType_DTM)
            {
                DTMexists = true;
            }
        }
        else
        {
            if ( trace_settings & TRACE_SHELL_CMD )
                trace_printf("%s@%d [%s] Exec failed!\n",
                             method_name, __LINE__, MyName);
        }
    }
}

void help_cmd (void)
{
    const char method_name[] = "help_cmd";

    printf ("[%s] usage: shell {[-a|-i] [<scriptfile>]} | {-c <command>}\n", MyName);
    printf ("[%s] - commands:\n", MyName);
    printf ("[%s] -- Command line environment variable replacement: ${<var_name>}\n", MyName);
    printf ("[%s] -- ! comment statement\n", MyName);
    printf ("[%s] -- cd <path>\n", MyName);
    if ( Debug && (trace_settings & TRACE_SHELL_CMD) )
        trace_printf ("%s@%d [%s] -- debug\n", method_name, __LINE__, MyName);
    printf ("[%s] -- delay <seconds>\n", MyName);
    printf ("[%s] -- down <nid>|<node-name> [, <reason-string>]\n", MyName);
    printf ("[%s] -- dump [{path <pathname>}] <process name> | <nid,pid>\n", MyName);
    printf ("[%s] -- echo [<string>]\n", MyName);
    printf ("[%s] -- event [{DTM|CS}] <event_id> [<nid,pid> [ event-data] ]\n", MyName);
    printf ("[%s] -- exec [{[debug][nowait][pri <value>][name <process name>]\n", MyName);
    printf ("[%s]           [nid <zone or node number>][type {CS|DTM|SSMP}]\n", MyName);
    printf ("[%s] --        [in <file>|#default][out <file>|#default]}] path [[<args>]...]\n", MyName);
    printf ("[%s] -- exit [!]\n", MyName);
    printf ("[%s] -- help\n", MyName);
    printf ("[%s] -- kill [{abort}] <process name> | <nid,pid>\n", MyName);
    printf ("[%s] -- ldpath [<directory>[,<directory>]...]\n", MyName);
    printf ("[%s] -- ls [{[detail]}] [<path>]\n", MyName);
    printf ("[%s] -- measure | measure_cpu\n", MyName);
    printf ("[%s] -- monstats\n", MyName);
    printf ("[%s] -- nameserver add node-name <node-name>\n", MyName);
    printf ("[%s] -- nameserver config\n", MyName);
    printf ("[%s] -- nameserver delete <node-name>\n", MyName);
    printf ("[%s] -- nameserver info\n", MyName);
    printf ("[%s] -- nameserver start <node-name>\n", MyName);
    printf ("[%s] -- nameserver stop <node-name>\n", MyName);
    printf ("[%s] -- node add node-name <node-name>,\n", MyName);
    printf ("[%s] --          cores {<first-core>} [ - <last-core>}],\n", MyName);
    printf ("[%s] --          processors {<processor-count>},\n", MyName);
    printf ("[%s] --          roles {connection|aggregation|storage}\n", MyName);
    printf ("[%s] -- node config [<nid>|<node-name>]\n", MyName);
    printf ("[%s] -- node delete <node-name>\n", MyName);
    printf ("[%s] -- node down <nid>|<node-name> [, <reason-string>]\n", MyName);
    printf ("[%s] -- node info [<nid>]\n", MyName);
    printf ("[%s] -- node name <old-node-name> <new-node-name>\n", MyName);
    printf ("[%s] -- node up <name>\n", MyName);
    printf ("[%s] -- path [<directory>[,<directory>]...]\n", MyName);
    printf ("[%s] -- persist config [{keys}|<persist-process-prefix>]\n", MyName);
    printf ("[%s] -- persist exec <persist-process-prefix>\n", MyName);
    printf ("[%s] -- persist info [<persist-process-prefix>]\n", MyName);
    printf ("[%s] -- persist kill <persist-process-prefix>\n", MyName);
    printf ("[%s] -- ps [{CS|DTM|GEN|PSD|SMS|SSMP|WDG|NS}] [<nid>|<process_name>|<nid,pid>]\n", MyName);
    printf ("[%s] -- pwd\n", MyName);
    printf ("[%s] -- quit\n", MyName);
    printf ("[%s] -- scanbufs\n", MyName);
    printf ("[%s] -- set [{[nid <number>]|[process <name>]}] key=<value string>\n", MyName);
    printf ("[%s] -- show [{[nid <number>]|[process <name>]}] [key]\n", MyName);
    printf ("[%s] -- shutdown [[immediate]|[abrupt]|[!]]\n", MyName);
    printf ("[%s] -- startup [trace] [<trace level>]\n", MyName);
    printf ("[%s] -- suspend [<event_id>]\n", MyName);
    printf ("[%s] -- time <shell command>\n", MyName);
    printf ("[%s] -- trace <number>\n", MyName);
    printf ("[%s] -- up <name>\n", MyName);
    printf ("[%s] -- wait [<process name> | <nid,pid>]\n", MyName);
    printf ("[%s] -- warmstart [trace] [<trace level>]\n", MyName);
    printf ("[%s] -- zone [nid <nid>|zid <zid>]\n", MyName);
}

void kill_cmd( char *cmd_tail, char delimiter )
{
    int nid;
    int pid;
    char token[MAX_TOKEN];
    bool abort = false;
    bool found = false;
    bool isValidNidPid = false;
    char * token_next;
    char * token_end;
    long number;
    char process_name[MAX_PROCESS_NAME];

    if (delimiter == '{')
    {
        while (*cmd_tail && delimiter != '}')
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "abort") == 0 && !found)
            {
                abort = true;
                found = true;
            }
            else
            {
                printf ("[%s] Invalid kill option syntax!\n", MyName);
                return;
            }
        }
    }

    // check if we have a process <name> or <nid,pid>
    if (*cmd_tail)
    {
        // Try interpreting next token as a number
        token_next = get_token (cmd_tail, token, &delimiter);
        number = strtol(token, &token_end, 10);
        if (token != token_end && *token_end == 0)
        {   // Entire first token is a number, assume user specified nid,pid
            nid = number;
            if (delimiter == ',')
            {   // Try interpreting next token as a number
                get_token (token_next, token, &delimiter);
                number = strtol(token, &token_end, 10);
                if (token_next != token_end && *token_end == 0)
                {   // Have a valid nid and pid
                    pid = number;
                    isValidNidPid = true;
                }
            }

            token[0] = '\0';
            if (!isValidNidPid || getpid(token, nid, pid))
            {
                printf ("[%s] No such process %s\n", MyName, cmd_tail);
                return;
            }
        }
        else
        {
            if ( token[0] == '$' )
            {
                nid = pid = -1;
            }
            else
            {
                printf ("[%s] Invalid process name!\n", MyName);
                return;
            }
        }
    }
    else
    {
        printf ("[%s] No process identified!\n", MyName);
        return;
    }

    process_name[0] = 0;
    if ( token[0] == '$' )
    {
        if (strlen(cmd_tail) >= MAX_PROCESS_NAME)
        {
            cmd_tail[MAX_PROCESS_NAME-1] = '\0';
        }
        remove_trailing_white_space(cmd_tail);
        STRCPY (process_name, cmd_tail);
    }

    kill_process( nid, pid, process_name, abort );
}

void ls_cmd (char *cmd_tail, char delimiter)
{
    int size = (int) (strlen (cmd_tail) + strlen (Wdir) + 8);
    char *cmd = new char[size];
    char *wdir = new char[size];
    char token[MAX_TOKEN];
    const char method_name[] = "ls_cmd";

    strcpy (cmd, "ls ");
    if (delimiter == '{')
    {
        delimiter = ' ';
        while (*cmd_tail && delimiter != '}')
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "detail") == 0)
            {
                strcpy (cmd, "ls -l ");
            }
            else
            {
                printf ("[%s] Invalid ls options syntax!\n", MyName);
            }
        }
    }

    if (*cmd_tail)
    {
        strcat (cmd, cd_cmd (cmd_tail, wdir));
    }
    else
    {
        strcat (cmd, Wdir);
    }

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] - calling system(%s)\n", method_name, __LINE__, MyName, cmd);
    system (cmd);
    delete [] cmd;
    delete [] wdir;
}

void monstats_cmd (char *)
{
    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_MonStats;

    gp_local_mon_io->send_recv( msg );

    if ((msg->type == MsgType_Service) &&
        (msg->u.reply.type == ReplyType_MonStats))
    {   // Display statistics
        printf("For node %d:\n", MyNid);
        printf("%5d Buffer pool size\n",
               gp_local_mon_io->getSharedBufferCount());
        printf("%5d Current number of free buffers in buffer pool\n",
               (gp_local_mon_io->getCurrentBufferCount()+1));
        printf("%5d Lowest number of free buffers in buffer pool\n",
               msg->u.reply.u.mon_info.availMin);
        printf("%5d Highest number of buffers ever in use by monitor\n",
               msg->u.reply.u.mon_info.acquiredMax);
        printf("%5d Number of times monitor could not obtain a buffer\n",
               msg->u.reply.u.mon_info.bufMisses);
    }
    else
    {
        printf
            ("[%s] Invalid MsgType(%d)/ReplyType(%d) for MonStats message\n",
             MyName, msg->type, msg->u.reply.type);
    }

    gp_local_mon_io->release_msg ( msg );
}

void nameserver_cmd (char *cmd_tail)
{
    char token[MAX_TOKEN];
    char delimiter;
    char *cmd = cmd_tail;
    char msgString[MAX_BUFFER] = { 0 };

    cmd = get_token (cmd, token, &delimiter);
    if (strcmp( token, "add" ) == 0)
    {
        if (Started)
        {
            if (ElasticityEnabled)
            {
                if (NameServerConfig.GetCount() <
                    NameServerConfig.GetConfigMax())
                {
                    // node-name=<node-name>,
                    nameserver_add_cmd( cmd );
                }
                else
                {
                    sprintf( msgString, "[%s] Nameserver add is not allowed, node count (%d) would exceed configuration limit (%d)"
                                      , MyName
                                      , NameServerConfig.GetCount()
                                      , NameServerConfig.GetConfigMax());
                    write_startup_log( msgString );
                    printf( "%s\n", msgString );
                }
            }
            else
            {
                sprintf( msgString, "[%s] Nameserver add is not enabled, to enable export SQ_ELASTICY_ENABLED=1",MyName);
                write_startup_log( msgString );
                printf( "%s\n", msgString );
            }
        }
        else
        {
            printf( EnvNotStarted, MyName );
        }
    }
    else if (strcmp( token, "config" ) == 0)
    {
        // [ <nid> | <node-name> ]
        nameserver_config_cmd( cmd );
    }
    else if (strcmp( token, "delete" ) == 0)
    {
        if (Started) // Should delete be ok with the instance down?
        {
            if (ElasticityEnabled)
            {
                // <node-name>
                nameserver_delete_cmd( cmd );
            }
            else
            {
                sprintf( msgString, "[%s] Nameserver delete is not enabled, to enable export SQ_ELASTICY_ENABLED=1",MyName);
                write_startup_log( msgString );
                printf( "%s\n", msgString );
            }
        }
        else
        {
            printf( EnvNotStarted, MyName );
        }
    }
    else if (strcmp (token, "info") == 0)
    {
        if (Started)
            nameserver_info( );
        else
        {
            printf( EnvNotStarted, MyName );
        }
    }
    else if (strcmp( token, "start" ) == 0)
    {
        if (Started)
        {
            // <node-name>
            nameserver_start_cmd( cmd );
        }
        else
        {
            printf( EnvNotStarted, MyName );
        }
    }
    else if (strcmp( token, "stop" ) == 0)
    {
        if (Started)
        {
            // <node-name>
            nameserver_stop_cmd( cmd );
        }
        else
        {
            printf( EnvNotStarted, MyName );
        }
    }
    else
    {
        printf( "[%s] Invalid nameserver syntax!\n", MyName );
    }
}

void nameserver_add_cmd( char *cmd )
{
    const char method_name[] = "nameserver_add_cmd";

    bool process_cmd = false;
    char *cmd_tail = cmd;
    char delimiter;
    char name[MPI_MAX_PROCESSOR_NAME] = { 0 };
    char token[MAX_TOKEN] = { 0 };
    char msgString[MAX_BUFFER] = { 0 };

    // setup defaults
    name[0] = '\0';

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing node add command.\n",
                      method_name, __LINE__, MyName);

    // parse options
    // node-name=<node-name>
    if (*cmd_tail != '\0')
    {
        while (*cmd_tail)
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter, MAX_TOKEN-1);
            normalize_case (token);
            if (strcmp( token, "node-name" ) == 0)
            {
                cmd_tail = get_token( cmd_tail, name, &delimiter, MPI_MAX_PROCESSOR_NAME-1, false );
            }
            else
            {
                sprintf( msgString, "[%s] Invalid nameserver add options syntax!",MyName);
                write_startup_log( msgString );
                printf( "%s\n", msgString );
                break;
            }
        }

        // Check for required values
        if (name[0] != 0)
        {
            process_cmd = true;
        }
    }
    else
    {
        sprintf( msgString, "[%s] Invalid nameserver add options syntax!",MyName);
        write_startup_log( msgString );
        printf( "%s\n", msgString );
    }

    if ( process_cmd )
    {
        nameserver_add( name );
    }
    else
    {
        sprintf( msgString, "[%s] Invalid nameserver add options syntax!",MyName);
        write_startup_log( msgString );
        printf( "%s\n", msgString );
    }
}

void nameserver_config_cmd( char *cmd )
{
    const char method_name[] = "nameserver_config_cmd";

    char *cmd_tail = cmd;
    char msgString[MAX_BUFFER] = { 0 };

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing nameserver config command.\n",
                      method_name, __LINE__, MyName);

    if (*cmd_tail != '\0')
    {
        sprintf( msgString, "[%s] Invalid nameserver config options syntax!",MyName);
        write_startup_log( msgString );
        printf("%s\n", msgString );
        return;
    }

    nameserver_config();
}

void nameserver_delete_cmd( char *cmd )
{
    const char method_name[] = "nameserver_delete_cmd";

    char *cmd_tail = cmd;
    char delimiter;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };
    char token[MAX_TOKEN] = { 0 };

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing nameserver delete command.\n",
                      method_name, __LINE__, MyName);

    if (*cmd_tail != '\0')
    {
        // <node-name>
        cmd_tail = get_token( cmd_tail, token, &delimiter );
        STRCPY(node_name, token);
        snprintf( msgString, sizeof(msgString)
                , "[%s] Executing nameserver delete. (node_name=%s)"
                , MyName, node_name );
        write_startup_log( msgString );
    }
    else
    {
        sprintf( msgString, "[%s] Invalid nameserver delete options syntax!",MyName);
        write_startup_log( msgString );
        printf("%s\n", msgString );
        return;
    }

    nameserver_delete( node_name );
}

void nameserver_start_cmd( char *cmd )
{
    const char method_name[] = "nameserver_start_cmd";

    char *cmd_tail = cmd;
    char delimiter;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };
    char token[MAX_TOKEN] = { 0 };

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing nameserver start command.\n",
                      method_name, __LINE__, MyName);

    if (*cmd_tail != '\0')
    {
        cmd_tail = get_token( cmd_tail, token, &delimiter );
        STRCPY(node_name, token);
        if ( !get_nameserver_by_node_name( node_name ) )
        {
            sprintf( msgString, "[%s] Node %s is not configured!"
                   , MyName, node_name);
            write_startup_log( msgString );
            printf ("%s\n", msgString);
            return;
        }
        if ( VirtualNodes )
        {
            nameserver_start( node_name );
        }
        else
        {
            if ( ClusterConfig.GetStorageType() == TCDBSQLITE)
            {
                if ( copy_config_db( node_name ) == 0 )
                {
                    nameserver_start( node_name );
                }
            }
        }
    }
    else
    {
        sprintf( msgString, "[%s] Invalid NameServer Up syntax!",MyName);
        write_startup_log( msgString );
        printf( "%s\n", msgString );
    }
}

void nameserver_stop_cmd( char *cmd )
{
    const char method_name[] = "nameserver_stop_cmd";

    char *cmd_tail = cmd;
    char delimiter;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };
    char token[MAX_TOKEN] = { 0 };

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing nameserver stop command.\n",
                      method_name, __LINE__, MyName);

    if (*cmd_tail != '\0')
    {
        // <node-name>
        cmd_tail = get_token( cmd_tail, token, &delimiter );
        STRCPY(node_name, token);
        if ( !get_nameserver_by_node_name( node_name ) )
        {
            sprintf( msgString, "[%s] Node %s is not configured!"
                   , MyName, node_name);
            write_startup_log( msgString );
            printf ("%s\n", msgString);
            return;
        }
        snprintf( msgString, sizeof(msgString)
                , "[%s] Executing nameserver stop. (node_name=%s)"
                , MyName, node_name );
        write_startup_log( msgString );
        printf( "%s\n", msgString );

        nameserver_stop( node_name );
    }
    else
    {
        sprintf( msgString, "[%s] Invalid NameServer Stop syntax!",MyName);
        write_startup_log( msgString );
        printf( "%s\n", msgString );
    }
}

void node_cmd (char *cmd_tail)
{
    int nid;
    int pnid;
    char token[MAX_TOKEN];
    char delimiter;
    char *cmd = cmd_tail;
    char msgString[MAX_BUFFER] = { 0 };

    if (*cmd_tail == '\0')
    {
        // TODO: To be deprecated (use 'node config' instead)
        // printf ("[%s] Invalid nodes syntax!\n", MyName);
        if ( ClusterConfig.IsConfigReady() )
        {
            CLNodeConfig   *lnodeConfig;
            CPNodeConfig   *pnodeConfig;

            // Print logical nodes array
            lnodeConfig = ClusterConfig.GetFirstLNodeConfig();
            if ( lnodeConfig )
            {
                while ( lnodeConfig )
                {
                    pnodeConfig = lnodeConfig->GetPNodeConfig();
                    if ( pnodeConfig )
                    {
                        printf( "[%s] Node[%d]=%s, %s\n"
                              , MyName
                              , lnodeConfig->GetNid()
                              , pnodeConfig->GetName()
                              , ZoneTypeString( lnodeConfig->GetZoneType() )
                              );
                    }
                    else
                    {
                        printf("[%s] Fatal Error: Logical node has no physical node in configuration\n",MyName);
                        exit(1);
                    }
                    lnodeConfig = lnodeConfig->GetNext();
                }
            }
        }
    }
    else
    {
        cmd = get_token (cmd, token, &delimiter);
        if (strcmp( token, "add" ) == 0)
        {
            if (Started)
            {
                if ( VirtualNodes )
                {
                    sprintf( msgString, "[%s] Node add is not available with Virtual Nodes!",MyName);
                    write_startup_log( msgString );
                    printf ("%s\n", msgString);
                }
                else
                {
                    if (ElasticityEnabled)
                    {
                        if (ClusterConfig.GetPNodesCount() <
                            ClusterConfig.GetPNodesConfigMax())
                        {
                            // node-name=<node-name>,
                            // cores=<first-core>[-<last-core>],
                            // processors=<processor-count>,
                            // roles=connection|aggregation|storage
                            node_add_cmd( cmd, delimiter );
                        }
                        else
                        {
                            sprintf( msgString, "[%s] Node add is not allowed, node count (%d) would exceed configuration limit (%d)"
                                              , MyName
                                              , ClusterConfig.GetPNodesCount()
                                              , ClusterConfig.GetPNodesConfigMax());
                            write_startup_log( msgString );
                            printf ("%s\n", msgString);
                        }
                    }
                    else
                    {
                        sprintf( msgString, "[%s] Node add is not enabled, to enable export SQ_ELASTICY_ENABLED=1",MyName);
                        write_startup_log( msgString );
                        printf ("%s\n", msgString);
                    }
                }
            }
            else
            {
                printf( EnvNotStarted, MyName );
            }
        }
        else if (strcmp( token, "config" ) == 0)
        {
            // [ <nid> | <node-name> ]
            node_config_cmd( cmd );
        }
        else if (strcmp( token, "delete" ) == 0)
        {
            if (Started) // Should delete be ok with the instance down?
            {
                if ( VirtualNodes )
                {
                    sprintf( msgString, "[%s] Node delete is not available with Virtual Nodes!",MyName);
                    write_startup_log( msgString );
                    printf ("%s\n", msgString);
                }
                else
                {
                    if (ElasticityEnabled)
                    {
                        // <nid> | <node-name>
                        node_delete_cmd( cmd );
                    }
                    else
                    {
                        sprintf( msgString, "[%s] Node delete is not enabled, to enable export SQ_ELASTICY_ENABLED=1",MyName);
                        write_startup_log( msgString );
                        printf ("%s\n", msgString);
                    }
                }
            }
            else
            {
                printf( EnvNotStarted, MyName );
            }
        }
        else if (strcmp( token, "down" ) == 0)
        {
            if (Started)
            {
                // <nid> [, <reason-string>]
                node_down_cmd( cmd );
            }
            else
            {
                printf( EnvNotStarted, MyName );
            }
        }
        else if (strcmp (token, "info") == 0)
        {
            if (Started)
            {
                // [ <nid> ]
                if ( *cmd )
                {
                    node_info_cmd( cmd );
                    CurNodes = NumLNodes-NumDown;
                }
                else
                {
                    // display all nodes
                    node_info( -1 );
                    CurNodes = NumLNodes-NumDown;
                }
            }
            else
            {
                printf (EnvNotStarted, MyName);
            }
        }
        else if (strcmp (token, "name") == 0)
        {
            if (Started) // Should delete be ok with the instance down?
            {
                if ( VirtualNodes )
                {
                    sprintf( msgString, "[%s] Node name is not available with Virtual Nodes!",MyName);
                    write_startup_log( msgString );
                    printf ("%s\n", msgString);
                }
                else
                {
                    if (ElasticityEnabled)
                    {
                        // <old-node-name> <new-node-name>
                        node_name_cmd(cmd);
                    }
                    else
                    {
                        sprintf( msgString, "[%s] Node name is not enabled, to enable export SQ_ELASTICY_ENABLED=1",MyName);
                        write_startup_log( msgString );
                        printf ("%s\n", msgString);
                    }
                }
            }
            else
            {
                printf( EnvNotStarted, MyName );
            }
        }
        else if (strcmp( token, "up" ) == 0)
        {
            if (Started)
            {
                // <name>
                node_up_cmd( cmd, delimiter );
            }
            else
            {
                printf( EnvNotStarted, MyName );
            }
        }
        else
        {
            printf ("[%s] Invalid nodes syntax!\n", MyName);
        }
    }
}

void node_add_cmd( char *cmd, char delimiter )
{
    const char method_name[] = "node_add_cmd";

    bool process_cmd = false;
    char *cmd_tail = cmd;
    char name[MPI_MAX_PROCESSOR_NAME] = { 0 };
    char node_name[MPI_MAX_PROCESSOR_NAME] = { 0 };
    char fqdn_name[MPI_MAX_PROCESSOR_NAME] = { 0 };
    char token[MAX_TOKEN] = { 0 };
    int  first_core, last_core, processor_count, roles;
    char msgString[MAX_BUFFER] = { 0 };

    // setup defaults
    name[0] = '\0';
    first_core = -1;
    last_core = -1;
    processor_count = 1;
    roles = 0;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing node add command.\n",
                      method_name, __LINE__, MyName);

    // parse options
    // { node-name=<node-name>,
    //   cores=<first-core>[-<last-core>],
    //   processors=<processor-count>,
    //   roles=connection|aggregation|storage }
    if (*cmd_tail != '\0')
    {
        delimiter = ';';
        while (*cmd_tail)
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter, MAX_TOKEN-1);
            normalize_case (token);
            if (strcmp( token, "node-name" ) == 0)
            {
                cmd_tail = get_token( cmd_tail, name, &delimiter, MPI_MAX_PROCESSOR_NAME-1, false );
                //printf ("[%s] node-name=%s, delimeter=%c\n", MyName, name, delimiter);
                if (NodeAddUseFqdn)
                {
                    if(get_fqdn_by_name( name, fqdn_name ) == -1)
                    {
                        fprintf( stderr
                               , "Fully Qualified Domain Name not available for hostname %s\n"
                               , name );
                        return;
                    }
                    else
                    {
                        strncpy( node_name, fqdn_name, sizeof(node_name) );
                    }
                }
                else
                {
                    strncpy( node_name, name, sizeof(node_name) );
                }
            }
            else if (strcmp( token, "cores" ) == 0)
            {
                delimiter = '-';
                cmd_tail = get_token (cmd_tail, token, &delimiter, MAX_TOKEN-1,false,true);
                first_core = atoi (token);
                //printf ("[%s] first_core=%d, delimeter=%c\n", MyName, first_core, delimiter);
                if (delimiter == '-')
                {
                    cmd_tail = get_token (cmd_tail, token, &delimiter);
                    last_core = atoi (token);
                    //printf ("[%s] last_core=%d, delimeter=%c\n", MyName, last_core, delimiter);
                }
            }
            else if (strcmp( token, "processors" ) == 0)
            {
                delimiter = '=';
                cmd_tail = get_token (cmd_tail, token, &delimiter);
                processor_count = atoi (token);
                //printf ("[%s] processor_count=%d, delimeter=%c\n", MyName, processor_count, delimiter);
            }
            else if (strcmp( token, "roles" ) == 0)
            {
                while (*cmd_tail && delimiter != '}')
                {
                    delimiter = '=';
                    cmd_tail = get_token (cmd_tail, token, &delimiter);
                    normalize_case (token);
                    if (strcmp( token, "connection" ) == 0)
                    {
                        roles += RoleType_Connection;
                        //printf ("[%s] roles=0x%x (%s), delimeter=%c\n", MyName, roles, token, delimiter);
                    }
                    else if (strcmp( token, "aggregation" ) == 0)
                    {
                        roles += RoleType_Aggregation;
                        //printf ("[%s] roles=0x%x (%s), delimeter=%c\n", MyName, roles, token, delimiter);
                    }
                    else if (strcmp( token, "storage" ) == 0)
                    {
                        roles += RoleType_Storage;
                        //printf ("[%s] roles=0x%x (%s), delimeter=%c\n", MyName, roles, token, delimiter);
                    }
                }
            }
            else
            {
                sprintf( msgString, "[%s] Invalid node add options syntax!",MyName);
                write_startup_log( msgString );
                printf ("[%s] Invalid node add options syntax!\n", MyName);
                delimiter = ' ';
                break;
            }
        }

        // Check for required values (currently all but last_core are required)
        if (node_name[0] != 0
         && first_core != -1
         && processor_count != -1
         && roles != 0)
        {
            process_cmd = true;
        }
    }
    else
    {
        sprintf( msgString, "[%s] Invalid node add options syntax!",MyName);
        write_startup_log( msgString );
        printf ("[%s] Invalid node add options syntax!\n", MyName);
    }

    if ( process_cmd )
    {
        node_add( node_name, first_core, last_core, processor_count, roles );
    }
    else
    {
        sprintf( msgString, "[%s] Invalid node add options syntax!",MyName);
        write_startup_log( msgString );
        printf ("[%s] Invalid node add options syntax!\n", MyName);
    }
}

void node_config_cmd( char *cmd )
{
    const char method_name[] = "node_config_cmd";

    char *cmd_tail = cmd;
    char delim;
    char token[MAX_TOKEN] = { 0 };
    char short_node_name[MAX_TOKEN] = { 0 };
    int nid = -1;
    int pnid = -1;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing node config command.\n",
                      method_name, __LINE__, MyName);

    // [ <nid> | <node-name> ]
    if (*cmd_tail != '\0')
    {
        // <nid> | <node-name>
        cmd_tail = get_token( cmd_tail, token, &delim );
        if ( isNumeric( token ) )
        {
            nid = atoi (token);
            pnid = get_pnid_by_nid( nid );
            if ( pnid == -1 )
            {
                printf( "[%s] Node id %d does not exist in configuration!\n"
                      , MyName, nid );
                return;
            }
        }
        else
        {
            if ( get_node_name( token, short_node_name ) != 0 )
            {
                printf( "[%s] Node %s does not exist in configuration!\n"
                      , MyName, token );
                return;
            }
        }
    }

    node_config( nid, short_node_name );
}

void node_delete_cmd( char *cmd )
{
    const char method_name[] = "node_delete_cmd";

    char *cmd_tail = cmd;
    char delim;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };
    char token[MAX_TOKEN] = { 0 };
    int nid = -1;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing node delete command.\n",
                      method_name, __LINE__, MyName);

    if (*cmd_tail != '\0')
    {
        // <node-name>
        cmd_tail = get_token( cmd_tail, token, &delim );
        if ( isNumeric( token ) )
        {
            sprintf( msgString, "[%s] Invalid node delete options syntax! (expecting <node-name>)",MyName);
            write_startup_log( msgString );
            printf ("%s\n", msgString);
            return;
        }
        else
        {
            if ( get_node_name( token, NULL ) != 0 )
            {
                sprintf( msgString, "[%s] Node %s does not exist in configuration!"
                       , MyName, token);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
                return;
            }
            STRCPY(node_name, token);
            snprintf( msgString, sizeof(msgString)
                    , "[%s] Executing node delete. (node_name=%s)"
                    , MyName, node_name );
            write_startup_log( msgString );
            printf ("%s\n", msgString);
        }
    }
    else
    {
        sprintf( msgString, "[%s] Invalid node delete options syntax!",MyName);
        write_startup_log( msgString );
        printf ("%s\n", msgString);
        return;
    }

    node_delete( nid, node_name );
}

void node_down_cmd( char *cmd )
{
    const char method_name[] = "node_down_cmd";

    int numLNodes = -1;
    int nid;
    int pnid;
    char *cmd_tail = cmd;
    char delim;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };
    char token[MAX_TOKEN] = { 0 };

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing node down command.\n",
                      method_name, __LINE__, MyName);

    // <node-name> | <nid> [, <reason-string>]
    cmd_tail = get_token( cmd_tail, token, &delim );
    if ( isNumeric( token ) )
    {
        if (cmd_tail[0] != 0)
        {
            snprintf( msgString, sizeof(msgString)
                    , "[%s] Executing node down. (nid=%s) \"%s\""
                    , MyName, token, cmd_tail );
        }
        else
        {
            snprintf( msgString, sizeof(msgString)
                    , "[%s] Executing node down. (nid=%s)"
                    , MyName, token );
        }
        write_startup_log( msgString );
        printf ("%s\n", msgString);

        nid = atoi (token);
        pnid = get_pnid_by_nid( nid );
        if ( pnid == -1 )
        {
            sprintf( msgString, "[%s] Node id %d does not exist in configuration!"
                   , MyName, nid);
            write_startup_log( msgString );
            printf ("%s\n", msgString);
            return;
        }
    }
    else
    {
        if (cmd_tail[0] != 0)
        {
            snprintf( msgString, sizeof(msgString)
                    , "[%s] Executing node down. (node_name=%s) \"%s\""
                    , MyName, token, cmd_tail );
        }
        else
        {
            snprintf( msgString, sizeof(msgString)
                    , "[%s] Executing node down. (node_name=%s)"
                    , MyName, token );
        }
        write_startup_log( msgString );
        printf ("%s\n", msgString);

        if ( get_node_name( token, NULL ) != 0 ) 
        {
            sprintf( msgString, "[%s] Node %s does not exist in configuration!"
                   , MyName, token);
            write_startup_log( msgString );
            printf ("%s\n", msgString);
            return;
        }
        STRCPY(node_name, token);
        nid = get_first_nid( node_name );
    }

    numLNodes = get_lnodes_count( nid );
    if ( numLNodes == -1 )
    {
        return;
    }

    int zid = -1;
    STATE state;

    if ( !get_zone_state( nid, zid, node_name, pnid, state ) )
    {
        return;
    }
    if ( state == State_Down )
    {
        sprintf( msgString, "[%s] Node is already down! (nid=%d, state=%s)\n", MyName, nid, StateString(state) );
        write_startup_log( msgString );
        printf ("%s\n", msgString);
        return;
    }
    else
    {
        if ( numLNodes > 1 )
        {
            if ( strcmp(cmd_tail, "!") )
            {
                sprintf( msgString, "[%s] Multiple logical nodes in physical node. Use <nid> '!' to down all logical nodes in physical node\n", MyName);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
                return;
            }
        }
    }

    node_down( nid, cmd_tail );

    NodeState[nid] = false;
}

void node_info_cmd( char *cmd )
{
    const char method_name[] = "node_info_cmd";

    char *cmd_tail = cmd;
    char delim;
    char token[MAX_TOKEN];
    int  i;
    int  nid;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing node info command.\n",
                      method_name, __LINE__, MyName);

    if ( VirtualNodes )
    {
        get_token( cmd_tail, token, &delim );
        if ( isNumeric( token ) )
        {
            i = atoi (token);
            if ( (i < 0) || (i > (CurNodes - 1)) )
            {
                sprintf( msgString, "[%s] Invalid node id!",MyName);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
            }
            else
            {
                // 1:1 mapping of virtual logical to physical nodes
                node_info( i );
            }
        }
        else
        {
            sprintf( msgString, "[%s] Invalid node id!",MyName);
            write_startup_log( msgString );
            printf ("%s\n", msgString);
        }
    }
    else
    {
        get_token( cmd_tail, token, &delim );
        if ( isNumeric( token ) )
        {
            nid = atoi (token);
            if ( get_node_name_by_nid( nid, node_name ) != 0 )
            {
                sprintf( msgString, "[%s] Invalid node id!\n", MyName);
                write_startup_log( msgString );
                printf ("[%s] Invalid node id!\n", MyName);
                return;
            }
        }
        else
        {
            if ( get_node_name( token, NULL ) != 0 ) 
            {
                sprintf( msgString, "[%s] Node %s does not exist in configuration!"
                       , MyName, token);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
                return;
            }
            STRCPY(node_name, token);
            nid = get_first_nid( node_name );
        }
        node_info( nid );
    }
}

void node_name_cmd( char *cmd )
{
    const char method_name[] = "node_name_cmd";

    char *cmd_tail = cmd;
    char delim;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };
    char new_node_name[MAX_TOKEN] = { 0 };
    char token[MAX_TOKEN] = { 0 };
    int nid = -1;

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing node name command.\n",
                      method_name, __LINE__, MyName);

    // <old-node-name> <new-node-name>
    if (*cmd_tail != '\0')
    {
        // <nid> | <node-name>
        cmd_tail = get_token( cmd_tail, token, &delim );
        if ( isNumeric( token ) )
        {
            nid = atoi (token);
            if (nid < 0 || nid > LNodesConfigMax - 1)
            {
                sprintf( msgString, "[%s] Invalid old node name!",MyName);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
               return;
            }
            snprintf( msgString, sizeof(msgString)
                    , "[%s] Executing node delete. (nid=%s)"
                    , MyName, token );
            write_startup_log( msgString );
        }
        else
        {
            STRCPY(node_name, token);
            if ( get_node_name( node_name, NULL ) != 0 )
            {
                sprintf( msgString, "[%s] Node %s is not configured!"
                       , MyName, node_name);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
                return;
            }
            if (*cmd_tail == '\0')
            {
                sprintf( msgString, "[%s] Invalid new node name!",MyName);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
                return;
            }
            STRCPY(new_node_name, cmd_tail);
            if (strcmp(node_name, new_node_name) == 0)
            {
                sprintf( msgString, "[%s] Invalid node name options syntax! <old-node-name> and <new-node-name> are the same.",MyName);
                write_startup_log( msgString );
                printf ("%s\n", msgString );
                return;
            }
            if ( get_node_name( new_node_name, NULL ) == 0 )
            {
                sprintf( msgString, "[%s] Node %s is already configured!"
                       , MyName, new_node_name);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
                return;
            }
            snprintf( msgString, sizeof(msgString)
                    , "[%s] Executing node name change. "
                      "(node_name=%s, new_node_name=%s)"
                    , MyName, node_name, new_node_name );
            write_startup_log( msgString );
        }
    }
    else
    {
        sprintf( msgString, "[%s] Invalid node name options syntax!",MyName);
        write_startup_log( msgString );
        printf ("%s\n", msgString );
        return;
    }

    node_change_name( node_name, new_node_name );
}

void node_up_cmd( char *cmd, char delimiter )
{
    const char method_name[] = "node_up_cmd";

    bool nowait = false;
    bool process_cmd;
    char *cmd_tail = cmd;
    char delim;
    char token[MAX_TOKEN];
    int  i;
    int  nid;
    char msgString[MAX_BUFFER] = { 0 };
    char node_name[MAX_TOKEN] = { 0 };

    if ( trace_settings & TRACE_SHELL_CMD )
        trace_printf ("%s@%d [%s] processing up node command.\n",
                      method_name, __LINE__, MyName);

    if (AgentType == AgentType_CM)
    {
        sprintf( msgString
               , "[%s] Command 'node up' is not supported in Cloudera Manager "
                 "installations! (You must use the node role start or restart action)"
               , MyName );
        write_startup_log( msgString );
        printf ("%s\n", msgString);
        return;
    }

    if (*cmd && delimiter == '{')
    {
        process_cmd = false;
    }
    else
    {
        process_cmd = true;
    }

    // parse options
    if (delimiter == '{')
    {
        delimiter = ' ';
        while (*cmd_tail && delimiter != '}')
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "nowait") == 0)
            {
                nowait = true;
            }
            else
            {
                sprintf( msgString, "[%s] Invalid up options syntax!",MyName);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
                delimiter = ' ';
                break;
            }
        }

        if (*cmd_tail == '\0')
        {
            sprintf( msgString, "[%s] Invalid up syntax!",MyName);
            write_startup_log( msgString );
            printf ("%s\n", msgString);
        }
        else if (delimiter == '}')
        {
            process_cmd = true;
        }
    }

    if ( process_cmd )
    {
        if ( VirtualNodes )
        {
            sprintf( msgString, "[%s] Executing node up. (nid=%s)"
                   , MyName, cmd_tail);
            write_startup_log( msgString );
            printf ("%s\n", msgString);

            get_token( cmd_tail, token, &delim );
            if ( isNumeric( token ) )
            {
                i = atoi (token);
                if ( (i < 0) || (i > (CurNodes - 1)) )
                {
                    sprintf( msgString, "[%s] Invalid node id!",MyName);
                    write_startup_log( msgString );
                    printf ("%s\n", msgString);
                }
                else
                {
                    // 1:1 mapping of virtual logical to physical nodes
                    node_up( i );
                }
            }
            else
            {
                sprintf( msgString, "[%s] Invalid node id!",MyName);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
            }
        }
        else
        {
            sprintf( msgString, "[%s] Executing node up. (node=%s)"
                   , MyName, cmd_tail);
            write_startup_log( msgString );
            printf ("%s\n", msgString);

            get_token( cmd_tail, token, &delim );
            if ( isNumeric( token ) )
            {
                sprintf( msgString, "[%s] Invalid node name (%s)!"
                       , MyName, token);
                write_startup_log( msgString );
                printf ("%s\n", msgString);
                return;
            }
            else
            {
                if ( get_node_name( token, NULL ) == 0 ) 
                {
                    if ( ClusterConfig.GetStorageType() == TCDBSQLITE)
                    {
                        if ( copy_config_db( cmd_tail ) != 0 )
                        {
                            return;
                        }
                    }
                }
                else
                {
                    sprintf( msgString, "[%s] Node %s does not exist in configuration!"
                           , MyName, token);
                    write_startup_log( msgString );
                    printf ("%s\n", msgString);
                    return;
                }
            }
            STRCPY(node_name, token);
            nid = get_first_nid( node_name );
            node_up( nid, nowait );
        }
    }
}

bool path_cmd (char *current_path, char *cmd_tail)
{
    char token[MAX_TOKEN];
    char delimiter;
    char wdir[MAX_SEARCH_PATH];
    char *path;

    if (*cmd_tail == '\0')
    {
        printf ("[%s] Path=%s\n", MyName, current_path);
        return false;
    }
    else
    {
        current_path[0] = '\0';
        while (*cmd_tail)
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            path = cd_cmd (token, wdir);
            if (*Path)
            {
                strcat (current_path, ":");
                strcat (current_path, path);
            }
            else
            {
                strcpy (current_path, path);
            }
        }
        return true;
    }
}

void persist_cmd (char *cmd)
{
    bool persistKeys = false;
    char *cmd_tail = cmd;
    char token[MAX_TOKEN];
    char delimiter;

    if (*cmd_tail == '\0')
    {
        printf( "[%s] Invalid persist syntax!\n", MyName );
    }
    else
    {
        cmd_tail = get_token (cmd_tail, token, &delimiter);
        normalize_case(token);
        if (strcmp (token, "config") == 0)
        {
            if (delimiter == '{')
            {
                if (*cmd_tail)
                {
                    cmd_tail = get_token (cmd_tail, token, &delimiter);
                    normalize_case(token);
                    if (strcmp (token, "keys") == 0)
                    {
                        persistKeys = true;
                    }
                    else
                    {
                        printf ("[%s] Invalid persist config options!\n", MyName);
                        return;
                    }
                }
                if (delimiter != '}')
                {
                    printf ("[%s] Invalid persist config syntax, looking for '}'\n", MyName);
                    return;
                }
            }
            persist_config_cmd( cmd_tail, persistKeys );
        }
        else if (strcmp( token, "exec" ) == 0)
        {
            if (Started)
            {
                if (delimiter == '{')
                {
                    printf( "[%s] Invalid persist info syntax!\n", MyName );
                    return;
                }
                persist_exec_cmd( cmd_tail );
            }
            else
            {
                printf( EnvNotStarted, MyName );
            }
        }
        else if (strcmp( token, "info" ) == 0)
        {
            if (Started)
            {
                if (delimiter == '{')
                {
                    printf( "[%s] Invalid persist info syntax!\n", MyName );
                    return;
                }
                persist_info_cmd( cmd_tail );
            }
            else
            {
                printf( EnvNotStarted, MyName );
            }
        }
        else if (strcmp( token, "kill" ) == 0)
        {
            if (Started)
            {
                if (delimiter == '{')
                {
                    printf( "[%s] Invalid persist kill syntax!\n", MyName );
                    return;
                }
                persist_kill_cmd( cmd_tail );
            }
            else
            {
                printf( EnvNotStarted, MyName );
            }
        }
        else
        {
            printf( "[%s] Invalid persist syntax!\n", MyName );
        }
    }
}

void persist_config_cmd( char *cmd, bool keysOnly )
{
    const char method_name[] = "persist_config_cmd";
    char *cmd_tail = cmd;
    char delimiter;
    char nullString[2] = {0,0};
    char token[MAX_TOKEN];

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        trace_printf( "%s@%d Persist process prefix=%s, keysOnly=%d\n "
                    , method_name, __LINE__
                    , cmd
                    , keysOnly );
    }

    if (ClusterConfig.IsConfigReady())
    {
        if (*cmd_tail == '\0')
        {
            if (keysOnly)
            {
                persist_config_keys();
            }
            else
            {
                // Get all persist process config
                persist_config( nullString );
            }
        }
        else
        {
            // Parse cmd to get persist-process-prefix
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case(token);
            if (*token != '\0')
            {
                // Get persist process config for persist-process-prefix
                persist_config( token );
            }
        }
    }
}

void persist_exec_cmd( char *cmd )
{
    const char method_name[] = "persist_exec_cmd";
    char *cmd_tail = cmd;
    char delimiter;
    char token[MAX_TOKEN];
    CPersistConfig *persistConfig;

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        trace_printf( "%s@%d Persist process prefix=%s\n "
                    , method_name, __LINE__
                    , cmd );
    }

    if (*cmd_tail == '\0')
    {
        printf ("[%s] Invalid persist exec syntax, looking for '<persist-process-prefix>'\n", MyName);
        return;
    }

    if (find_DTM())
    {
        DTMexists = true;
    }
    else
    {
        DTMexists = false;
    }

    if (ClusterConfig.IsConfigReady())
    {
        // Parse cmd to get persist-process-prefix
        get_token (cmd_tail, token, &delimiter);
        if (*token != '\0')
        {
            // Get persist process configuration
            persistConfig = ClusterConfig.GetPersistConfig( token );
            if (persistConfig)
            {
                if (persistConfig->GetProcessType() == ProcessType_Watchdog)
                {
                    printf ("[%s] Persist process exec of a WDG process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetProcessType() == ProcessType_PSD)
                {
                    printf ("[%s] Persist process exec of a PSD process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetProcessType() == ProcessType_SMS)
                {
                    printf ("[%s] Persist process exec of a SMS process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetProcessType() == ProcessType_DTM)
                {
                    printf ("[%s] Persist process exec of a DTM process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetRequiresDTM())
                {
                    if (DTMexists)
                    {
                        persist_process_start( persistConfig );
                    }
                    else
                    {
                        printf ("[%s] Persist process '%s' requires DTM and DTM does not exist\n", MyName, token);
                    }
                }
                else
                {
                    persist_process_start( persistConfig );
                }
            }
            else
            {
                printf ("[%s] Configuration for persist process prefix '%s' does not exist\n", MyName, token);
            }
        }
        else
        {
            printf ("[%s] Invalid persist exec syntax, looking for '<persist-process-prefix>'\n", MyName);
        }
    }
}

void persist_info_cmd( char *cmd )
{
    const char method_name[] = "persist_info_cmd";
    char *cmd_tail = cmd;
    char delimiter;
    char nullString[2] = {0,0};
    char token[MAX_TOKEN];

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        trace_printf( "%s@%d Persist process prefix=%s\n "
                    , method_name, __LINE__
                    , cmd );
    }

    if (*cmd_tail == '\0')
    {
        // Get all persist process info
        persist_info( nullString );
    }
    else
    {
        // Parse cmd to get persist-process-prefix
        cmd_tail = get_token (cmd_tail, token, &delimiter);
        normalize_case(token);
        if (*token != '\0')
        {
            // Get persist process info for persist-process-prefix
            persist_info( token );
        }
    }
}

void persist_kill_cmd( char *cmd )
{
    const char method_name[] = "persist_kill_cmd";
    char *cmd_tail = cmd;
    char delimiter;
    char token[MAX_TOKEN];
    CPersistConfig *persistConfig;

    if ( trace_settings & TRACE_SHELL_CMD )
    {
        trace_printf( "%s@%d Persist process prefix=%s\n "
                    , method_name, __LINE__
                    , cmd );
    }

    if (*cmd_tail == '\0')
    {
        printf ("[%s] Invalid persist kill syntax, looking for '<persist-process-prefix>'\n", MyName);
        return;
    }

    if (ClusterConfig.IsConfigReady())
    {
        // Parse cmd to get persist-process-prefix
        get_token (cmd_tail, token, &delimiter);
        if (*token != '\0')
        {
            // Get persist process configuration
            persistConfig = ClusterConfig.GetPersistConfig( token );
            if (persistConfig)
            {
                if (persistConfig->GetProcessType() == ProcessType_Watchdog)
                {
                    printf ("[%s] Persist process kill of a WDG process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetProcessType() == ProcessType_NameServer)
                {
                    printf ("[%s] Persist process kill of a TNS process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetProcessType() == ProcessType_PSD)
                {
                    printf ("[%s] Persist process kill of a PSD process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetProcessType() == ProcessType_SMS)
                {
                    printf ("[%s] Persist process kill of a SMS process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetProcessType() == ProcessType_DTM)
                {
                    printf ("[%s] Persist process kill of a DTM process type is not allowed!\n", MyName);
                }
                else if (persistConfig->GetProcessType() == ProcessType_TMID)
                {
                    printf ("[%s] Persist process kill of a TMID process type is not allowed!\n", MyName);
                }
                else
                {
                    persist_process_kill( persistConfig );
                }
            }
            else
            {
                printf ("[%s] Configuration for persist process prefix '%s' does not exist\n", MyName, token);
            }
        }
        else
        {
            printf ("[%s] Invalid persist kill syntax, looking for '<persist-process-prefix>'\n", MyName);
        }
    }
}

void ps_cmd (char *cmd_tail, char delimiter)
{
    int nid;
    int pid;
    int pnid;
    char process_name[MAX_PROCESS_NAME];
    char token[MAX_TOKEN];
    PROCESSTYPE process_type = ProcessType_Undefined;

    // parse options
    if (delimiter == '{')
    {
        delimiter = ' ';
        while (*cmd_tail && delimiter != '}')
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "cs") == 0)
            {
                process_type = ProcessType_MXOSRVR;
            }
            else if (strcmp (token, "dtm") == 0)
            {
                process_type = ProcessType_DTM;
            }
            else if (strcmp (token, "gen") == 0)
            {
                process_type = ProcessType_Generic;
            }
            else if (strcmp (token, "psd") == 0)
            {
                process_type = ProcessType_PSD;
            }
            else if (strcmp (token, "sms") == 0)
            {
                process_type = ProcessType_SMS;
            }
            //else if (strcmp (token, "spx") == 0)
            //{
            //    process_type = ProcessType_SPX;
            //}
            else if (strcmp (token, "ssmp") == 0)
            {
                process_type = ProcessType_SSMP;
            }
            else if (strcmp (token, "tmid") == 0)
            {
                process_type = ProcessType_TMID;
            }
            else if (strcmp (token, "wdg") == 0)
            {
                process_type = ProcessType_Watchdog;
            }
            else if (strcmp (token, "ns") == 0)
            {
                process_type = ProcessType_NameServer;
            }
            else
            {
                printf ("[%s] Invalid process type!\n",MyName);
                return;
            }
        }
        if (delimiter != '}')
        {
            printf ("[%s] Invalid ps option syntax!\n", MyName);
            return;
        }
    }

    // check if we have a process <name> or <nid> or <nid,pid>
    if (isdigit (*cmd_tail))
    {
        cmd_tail = get_token (cmd_tail, token, &delimiter);
        if (delimiter == ',')
        {
            nid = atoi (token);
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            pid = atoi (token);
        }
        else
        {
            nid = atoi (token);
            pid = -1;
            //printf ("[%s] Invalid process Nid,Pid!\n", MyName);
            //return;
        }
        pnid = get_pnid_by_nid( nid );
        if ( pnid == -1 )
        {
            printf( "[%s] Invalid node, nid=%d\n", MyName, nid );
            return;
        }
    }
    else
    {
        nid = pid = -1;
    }

    if (strlen(cmd_tail) >= MAX_PROCESS_NAME)
    {
        cmd_tail[MAX_PROCESS_NAME-1] = '\0';
    }
    remove_trailing_white_space(cmd_tail);
    STRCPY (process_name, cmd_tail);

    get_proc_info( nid
                 , pid
                 , process_name
                 , process_type
                 , false );
}

void set_cmd (char *cmd, char delimiter)
{
    int nid = MonitorNid;
    char *cmd_tail = cmd;
    char token[MAX_TOKEN];
    char name[MAX_PROCESS_NAME];
    ConfigType type;

    // setup defaults
    type = ConfigType_Cluster;
    name[0] = '\0';             // The monitor will assign name if null
    // parse options
    if (delimiter == '{')
    {
        if (*cmd_tail)
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "process") == 0)
            {
                cmd_tail = get_token (cmd_tail, name, &delimiter, MAX_PROCESS_NAME-1);
                type = ConfigType_Process;
            }
            else if (strcmp (token, "nid") == 0)
            {
                cmd_tail = get_token (cmd_tail, token, &delimiter);
                nid = atoi (token);
                if (nid < 0 || nid > CurNodes - 1)
                {
                    printf ("[%s] Invalid node id!\n", MyName);
                    return;
                }
                type = ConfigType_Node;
                sprintf(name,"NODE%d",nid);
            }
            else
            {
                printf ("[%s] Invalid set options!\n", MyName);
                return;
            }
        }
        if (delimiter != '}')
        {
            printf ("[%s] Invalid set syntax, looking for '}'\n", MyName);
            return;
        }
    }

    if (*cmd_tail == '\0')
    {
        printf ("[%s] Invalid set syntax, no key\n", MyName);
        return;
    }

    // find key=token and set value=cmd_tail
    cmd_tail = get_token (cmd_tail, token, &delimiter, MAX_KEY_NAME-1);

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {   // Could not acquire a message buffer
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return;
    }

    // send set request to monitor
    send_set_req( type, name, token, cmd_tail );
}

void show_cmd (char *cmd, char delimiter)
{
    int i;
    int count;
    int nid = MonitorNid;
    int num_returned = 0;
    bool next=false;
    MPI_Status status;
    char *cmd_tail = cmd;
    char token[MAX_TOKEN];
    char name[MAX_PROCESS_NAME];
    ConfigType type;

    // setup defaults
    type = ConfigType_Cluster;
    name[0] = '\0';             // The monitor will assign name if null

    // parse options
    if (delimiter == '{')
    {
        if (*cmd_tail)
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "process") == 0)
            {
                cmd_tail = get_token (cmd_tail, name, &delimiter, MAX_PROCESS_NAME-1);
                type = ConfigType_Process;
            }
            else if (strcmp (token, "nid") == 0)
            {
                cmd_tail = get_token (cmd_tail, token, &delimiter);
                nid = atoi (token);
                if (nid < 0 || nid > CurNodes - 1)
                {
                    printf ("[%s] Invalid node id!\n", MyName);
                    return;
                }
                type = ConfigType_Node;
                sprintf(name,"NODE%d",nid);
            }
            else
            {
                printf ("[%s] Invalid set options!\n", MyName);
                return;
            }
        }
        if (delimiter != '}')
        {
            printf ("[%s] Invalid set syntax, looking for '}'\n", MyName);
            return;
        }
    }

    if (*cmd_tail != '\0')
    {
        // key is the next token
        get_token (cmd_tail, token, &delimiter, MAX_KEY_NAME-1);
    }
    else
    {
        token[0] = '\0';
    }

    do
    {
         if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
        {   // Could not acquire a message buffer
            printf ("[%s] Unable to acquire message buffer.\n", MyName);
            return;
        }

        // send get request to monitor
        msg->type = MsgType_Service;
        msg->noreply = false;
        msg->reply_tag = REPLY_TAG;
        msg->u.request.type = ReqType_Get;
        msg->u.request.u.get.nid = MyNid;
        msg->u.request.u.get.pid = MyPid;
        msg->u.request.u.get.verifier = MyVerifier;
        msg->u.request.u.get.process_name[0] = 0;
        msg->u.request.u.get.type = type;
        msg->u.request.u.get.next = next;
        STRCPY(msg->u.request.u.get.group,name);
        STRCPY(msg->u.request.u.get.key,token);

        gp_local_mon_io->send_recv( msg );
        count = sizeof( *msg );
        status.MPI_TAG = msg->reply_tag;

        if ((status.MPI_TAG == REPLY_TAG) &&
            (count == sizeof (struct message_def)))
        {
            if ((msg->type == MsgType_Service) &&
                (msg->u.reply.type == ReplyType_Get))
            {
                if ( num_returned == 0 )
                {
                    switch (msg->u.reply.u.get.type)
                    {
                    case ConfigType_Cluster:
                        printf("[%s] Configuration Global Group: %s\n",
                            MyName,msg->u.reply.u.get.group);
                        break;
                    case ConfigType_Node:
                        printf("[%s] Configuration Local Group: %s\n",
                            MyName,msg->u.reply.u.get.group);
                        break;
                    case ConfigType_Process:
                        printf("[%s] Configuration Global Group: PROCESS %s\n",
                            MyName,msg->u.reply.u.get.group);
                        break;
                    default:
                        printf("[%s] Configuration Group: UNKNOWN\n",MyName);
                    }
                }
                for(i=0; i<msg->u.reply.u.get.num_returned; i++)
                {
                    num_returned++;
                    printf("[%s] - %d: %-*s = %s\n",
                        MyName,
                        num_returned,
                        MAX_KEY_NAME,
                        msg->u.reply.u.get.list[i].key,
                        msg->u.reply.u.get.list[i].value);
                }
                if ( (next = (num_returned < msg->u.reply.u.get.num_keys) ) )
                {
                    strcpy(token,msg->u.reply.u.get.list[i-1].key);
                }
            }
            else
            {
                printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for Get message\n",
                     MyName, msg->type, msg->u.reply.type);
            }
        }
        else
        {
            printf ("[%s] Get reply message invalid\n", MyName);
        }

        gp_local_mon_io->release_msg(msg);
    }
    while (next);
}

void suspend_cmd (char *cmd_tail)
{
    int event_id;
    char delimiter;
    char token[MAX_TOKEN];

    if (*cmd_tail)
    {
        get_token (cmd_tail, token, &delimiter);
        if (isdigit (*token))
        {
            event_id = atoi (token);
            get_event (event_id);
        }
        else
        {
            printf ("[%s] Invalid event id syntax!\n", MyName);
        }
    }
    else
    {
        get_event (-1);
    }
}

void wait_cmd (char *cmd_tail)
{
    int nid;
    int pid;
    char delimiter;
    char token[MAX_TOKEN];

    if (*cmd_tail)
    {
        cmd_tail = get_token (cmd_tail, token, &delimiter);
        if (delimiter == ',' && isdigit (*token))
        {
            nid = atoi (token);
            get_token (cmd_tail, token, &delimiter);
            pid = atoi (token);

            // Verify that the given node id/process id pair is valid
            token[0] = '\0';
            if ( getpid(token, nid, pid) )
            {  // No such process
                printf ("[%s] Process %d,%d not found\n", MyName, nid, pid);
            }
            else
            {   // Wait for the specified process to die
                get_server_death (nid, pid);
            }
        }
        else
        {
            // Convert the process name to a node id/process id pair
            nid = pid = -1;
            if ( getpid(token, nid, pid) )
            {
                printf ("[%s] Process %s not found\n", MyName,token);
            }
            else
            {   // Wait for the specified process to die
                get_server_death (nid, pid);
            }
        }
    }
    else
    {
        if (LastNid == -1 || LastPid == -1)
        {
            printf ("[%s] No last nid,pid defined\n", MyName);
        }
        else
        {
            token[0] = '\0';
            if ( getpid(token, LastNid, LastPid) )
            {  // No such process
                printf ("[%s] Process %d,%d not found\n", MyName, LastNid,
                        LastPid);
            }
            else
            {
                printf ("[%s] Waiting for process %3.3d,%3.3d death notice\n",
                        MyName, LastNid, LastPid);
                get_server_death (LastNid, LastPid);
            }
            LastNid = LastPid = -1;
        }
    }
}

void zone_cmd (char *cmd_tail)
{
    int nid, zid;
    char delimiter;
    char token[MAX_TOKEN];

    if ( !Started )
    {
        printf (EnvNotStarted, MyName);
        return;
    }

    if (*cmd_tail != '\0')
    {
        if (*cmd_tail)
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            if (strcmp (token, "nid") == 0)
            {
                if ( *cmd_tail )
                {
                    get_token (cmd_tail, token, &delimiter);
                    normalize_case (token);
                    nid = atoi(token);
                    if ((!isNumeric(token)) || (nid >= LNodesConfigMax) || (nid < 0))
                    {
                        printf ("[%s] Invalid nid\n", MyName);
                    }
                    else
                    {
                        listZoneInfo(nid, -1);
                    }
                }
                else
                {
                    printf ("[%s] Invalid zone option syntax!\n", MyName);
                    return;
                }
            }
            else if (strcmp (token, "zid") == 0)
            {
                if ( *cmd_tail )
                {
                    cmd_tail = get_token (cmd_tail, token, &delimiter);
                    normalize_case (token);
                    zid = atoi(token);
                    if ((!isNumeric(token)) || (zid >= LNodesConfigMax) || (zid < 0))
                    {
                        printf ("[%s] Invalid zid\n", MyName);
                    }
                    else
                    {
                        listZoneInfo(-1, zid);
                    }
                }
                else
                {
                    printf ("[%s] Invalid zone option syntax!\n", MyName);
                    return;
                }
            }
            else
            {
                printf ("[%s] Invalid zone option!\n",MyName);
                return;
            }
        }
        CurNodes = NumLNodes-NumDown;
    }
    else
    {
        // display all nodes
        listZoneInfo( -1, -1 );
    }
}

bool process_command( char *token, char *cmd_tail, char delimiter )
{
    bool done=false;
    char msgString[MAX_BUFFER] = { 0 };
    struct message_def *saved_msg = msg;

    // process command
    if (strcmp (token, "cd") == 0)
    {
        cd_cmd (cmd_tail, Wdir);
    }
#ifdef DEBUGGING
    else if (strcmp (token, "debug") == 0)
    {
        printf("MPI_WTIME_IS_GLOBAL=%d\n",MPI_WTIME_IS_GLOBAL);
        Debug = true;
    }
#endif
    else if (strcmp (token, "delay") == 0)
    {
        sleep (atoi (cmd_tail));
    }
    else if (strcmp (token, "down") == 0)
    {
        if (Started)
        {
            node_down_cmd( cmd_tail );
        }
        else
        {
            sprintf( msgString, "[%s] Environment has not been started!",MyName);
            write_startup_log( msgString );
            printf( EnvNotStarted, MyName );
        }
    }
    else if (strcmp (token, "dump") == 0)
    {
        if (Started)
        {
            dump_cmd (cmd_tail, delimiter);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "echo") == 0)
    {
        printf ("[%s] %s\n", MyName, cmd_tail);
    }
    else if (strcmp (token, "event") == 0)
    {
        if (Started)
        {
            event_cmd (cmd_tail, delimiter);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "exec") == 0)
    {
        if (Started)
        {
            exec_cmd (cmd_tail, delimiter);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (( strcmp (token, "exit") == 0) ||
             ( strcmp (token, "quit") == 0)   )
    {
        done = true;
    }
    else if (strcmp (token, "help") == 0)
    {
        help_cmd ();
    }
    else if (strcmp (token, "kill") == 0)
    {
        if (Started)
        {
            kill_cmd( cmd_tail, delimiter );
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "ldpath") == 0)
    {
        if ( path_cmd (LDpath,cmd_tail) )
        {
            setenv("LD_LIBRARY_PATH", LDpath, 1);
        }
    }
    else if (strcmp (token, "ls") == 0)
    {
        ls_cmd (cmd_tail, delimiter);
    }
    else if (strcmp (token, "measure") == 0)
    {
        if (Started)
        {
            printf ("[%s] Command must be issued before the environment is started!\n", MyName);
        }
        else
        {
            Measure = 1;
            setenv("SQ_PIDMAP", "1", 1);
        }
    }
    else if (strcmp (token, "measure_cpu") == 0)
    {
        if (Started)
        {
            printf ("[%s] Command must be issued before the environment is started!\n", MyName);
        }
        else
        {
            Measure = 2;
            setenv("SQ_PIDMAP", "1", 1);
        }
    }
    else if (strcmp (token, "nameserver") == 0)
    {
        nameserver_cmd (cmd_tail);
    }
    else if (strcmp (token, "node") == 0)
    {
        node_cmd (cmd_tail);
    }
    else if (strcmp (token, "path") == 0)
    {
        if ( path_cmd (Path, cmd_tail) )
        {
            setenv("PATH", Path, 1);
        }
    }
    else if (strcmp (token, "persist") == 0)
    {
        persist_cmd (cmd_tail);
    }
    else if (strcmp (token, "ps") == 0)
    {
        if (Started)
        {
            ps_cmd (cmd_tail, delimiter);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "pwd") == 0)
    {
        printf ("[%s] Wdir=%s\n", MyName, Wdir);
    }
    else if (strcmp (token, "set") == 0)
    {
        if (Started)
        {
            set_cmd (cmd_tail, delimiter);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "show") == 0)
    {
        if (Started)
        {
            show_cmd (cmd_tail, delimiter);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "shutdown") == 0)
    {
        if (Started)
        {
            normalize_case (cmd_tail);
            if ( *cmd_tail == '\0' )
            {
                sprintf( msgString, "[%s] Executing shutdown.",MyName);
                write_startup_log( msgString );
                shutdown (ShutdownLevel_Normal);
            }
            else if (strcmp(cmd_tail,"immediate")==0)
            {
                sprintf( msgString, "[%s] Executing shutdown immediate.",MyName);
                write_startup_log( msgString );
                shutdown (ShutdownLevel_Immediate);
            }
            else if (strcmp(cmd_tail,"abrupt")==0)
            {
                sprintf( msgString, "[%s] Executing shutdown abrupt.",MyName);
                write_startup_log( msgString );
                shutdown (ShutdownLevel_Abrupt);
            }
            else if (*cmd_tail == '!')
            {
                sprintf( msgString, "[%s] Executing shutdown abrupt.",MyName);
                write_startup_log( msgString );
                shutdown (ShutdownLevel_Abrupt);
            }
            else
            {
                sprintf( msgString, "[%s] Executing shutdown.",MyName);
                write_startup_log( msgString );
                sprintf( msgString, "[%s] Invalid shutdown option!",MyName);
                write_startup_log( msgString );
                printf ("[%s] Invalid shutdown option!\n", MyName);
            }
        }
        else
        {
            sprintf( msgString, "[%s] Executing shutdown.",MyName);
            write_startup_log( msgString );
            sprintf( msgString, "[%s] Environment has not been started!",MyName);
            write_startup_log( msgString );
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "startup") == 0)
    {
        sprintf( msgString, "[%s] Executing environment startup.",MyName);
        write_startup_log( msgString );
        if ( is_environment_up() )
        {
            sprintf( msgString, "[%s] Environment already started!",MyName);
            write_startup_log( msgString );
            printf ("[%s] Environment already started!\n", MyName);
        }
        else if ( is_spare_node( MyNode ) )
        {
            sprintf( msgString, "[%s] Current node (%s) is a configured spare node! Must use non-spare node to startup environment.", MyName, MyNode);
            write_startup_log( msgString );
            printf ("[%s] Current node (%s) is a configured spare node! Must use non-spare node to startup environment.\n", MyName, MyNode);
        }
        else if ( check_environment() )
        {
            if ( start_monitor( cmd_tail,false, false ) )
            {
                sprintf( msgString, "[%s] Failed to start environment!",MyName);
                write_startup_log( msgString );
                printf ("[%s] Failed to start environment!\n",MyName);
                Started = false;
            }
            else
            {
                Started = true;
                ShutdownClean = false;
            }
        }
    }
    else if (strcmp (token, "suspend") == 0)
    {
        if (Started)
        {
            suspend_cmd (cmd_tail);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "trace") == 0)
    {
        if ( strlen(cmd_tail) != 0)
        {
            // Turn off any current trace flags
            trace_settings = 0;
            Local_IO_To_Monitor::cv_trace = false;

            // Get new trace flags and enable tracing if needed
            long int flags = strtol(cmd_tail, NULL, 0);
            if ( flags != 0 && traceOpen == false )
            {
                TraceOpen();
            }
            TraceUpdate ( flags );
        }
    }
    else if (strcmp (token, "up") == 0)
    {
        if (Started)
        {
            node_up_cmd( cmd_tail, delimiter );
        }
        else
        {
            sprintf( msgString, "[%s] Environment has not been started!",MyName);
            write_startup_log( msgString );
            printf( EnvNotStarted, MyName );
        }
    }
    else if (strcmp (token, "wait") == 0)
    {
        if (Started)
        {
            wait_cmd (cmd_tail);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "warmstart") == 0)
    {
        sprintf( msgString, "[%s] Executing environment warmstart.",MyName);
        write_startup_log( msgString );
        if ( is_environment_up() )
        {
            sprintf( msgString, "[%s] Environment already started!",MyName);
            write_startup_log( msgString );
            printf ("[%s] Environment already started!\n", MyName);
        }
        else
        {
            if (ShutdownClean)
            {
                if ( start_monitor( cmd_tail, true, false ) )
                {
                    sprintf( msgString, "[%s] Failed to warmstart environment!",MyName);
                    write_startup_log( msgString );
                    printf ("[%s] Failed to warmstart environment!\n",MyName);
                    Started = false;
                }
                else
                {
                    Started = true;
                    ShutdownClean = false;
                }
            }
            else
            {
                sprintf( msgString, "[%s] Environment was not shutdown cleanly, can't warm start!",MyName);
                write_startup_log( msgString );
                printf ("[%s] Environment was not shutdown cleanly, can't warm start!\n",MyName);
            }
        }
    }
    else if (strcmp (token, "scanbufs") == 0)
    {
        if (gp_local_mon_io)
            gp_local_mon_io->scan_liobufs ();
    }
    else if (strcmp (token, "monstats") == 0)
    {
        if (Started)
        {
            monstats_cmd (cmd_tail);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else if (strcmp (token, "zone") == 0)
    {
        if (Started)
        {
            zone_cmd (cmd_tail);
        }
        else
        {
            printf (EnvNotStarted, MyName);
        }
    }
    else
    {
        if (*token != '\0')
        {
            printf ("[%s] Invalid command!\n", MyName);
        }
    }

    msg = saved_msg;
    return done;
}

void SetCallbacks ( void )
{
    if ( gp_local_mon_io )
    {
        gp_local_mon_io->set_cb(recv_notice_msg, "notice");
        gp_local_mon_io->set_cb(recv_notice_msg, "event");
        gp_local_mon_io->set_cb(recv_notice_msg, "recv");
        gp_local_mon_io->set_cb(recv_notice_msg, "unsol");
    }
}

void InitLocalIO( void )
{
    const char method_name[] = "InitLocalIO";

    if ( MyPNid == -1 )
    {
        CPNodeConfig   *pnodeConfig;
        CLNodeConfig   *lnodeConfig;

        if ( VirtualNodes )
        {
            lnodeConfig = ClusterConfig.GetLNodeConfig( MyNid );
            pnodeConfig = lnodeConfig->GetPNodeConfig();
        }
        else
        {
            pnodeConfig = ClusterConfig.GetPNodeConfig( MyNode );
            if ( !pnodeConfig )
            {
                printf( "[%s] Node %s is not in the configuration!\n"
                      , MyName, MyNode);
                exit(1);
            }
        }
        gv_ms_su_nid = MyPNid = pnodeConfig->GetPNid();
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf ("%s@%d [%s] Local IO pnid = %d\n", method_name,
                          __LINE__, MyName, MyPNid);
    }

    gp_local_mon_io = new Local_IO_To_Monitor( -1 );
    SetCallbacks();
}

void MpirunInit( void )
{
    // Determine trace file name
    const char *tmpDir;
    tmpDir = getenv( "TRAF_LOG" );

    if (tmpDir)
    {
        sprintf( mpirunOutFileName, "%s/monitor.mpirun.out", tmpDir );
        sprintf( mpirunErrFileName, "%s/monitor.mpirun.err", tmpDir );
    }
    else
    {
        sprintf( mpirunOutFileName, "./monitor.mpirun.out" );
        sprintf( mpirunErrFileName, "./monitor.mpirun.err" );
    }

    if ( trace_settings & TRACE_SHELL_CMD)
    {
        printf( "mpirunOutFileName=%s\n", mpirunOutFileName );
        printf( "mpirunErrFileName=%s\n", mpirunErrFileName );
    }
}

void OrderlyShutdown()
{
    if ( Started && Attached )
    {
        struct message_def *saved_msg = msg;
        exit_process();
        msg = saved_msg;
    }
    delete msg;

    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
}

int main (int argc, char *argv[])
{
    bool done = false;
    bool exec_one_command = false;
    bool tty = true;
    char delimiter;
    char *env;
    char *input_file;
    char *cmd_buffer;
    char token[MAX_TOKEN];
    char cmd_string[MAX_CMDLINE];
    char *cmd_tail = NULL;
    int i;
    bool alloc_new = false;
    const char method_name[] = "main";

    CALL_COMP_DOVERS(shell, argc, argv);

    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    // Initialize MPI environment
    MPI_Init (&argc, &argv);

    // Initialize trace settings
    TraceInit ( argc, argv );

    // Initialize virtual <nid> from command line args
    VirtualNidInit( argc, argv );
    
    MyName = new char [MAX_PROCESS_PATH];
    // setup defaults
    strcpy (MyName, "SHELL");
    cmd_buffer = getenv("SQ_MEASURE");
    if (cmd_buffer && *cmd_buffer == '1')
    {
        Measure = 1;
        setenv("MPI_INSTR", "shell", 1);
        setenv("SQ_PIDMAP", "1", 1);
    }
    if (cmd_buffer && *cmd_buffer == '2')
    {
        Measure = 2;
        setenv("MPI_INSTR", "shell.cpu:cpu", 1);
        setenv("SQ_PIDMAP", "1", 1);
    }

    // Check if we are using virtual nodes ... if so, set MyNid
    cmd_buffer = getenv("SQ_VIRTUAL_NODES");
    if (cmd_buffer && isdigit(cmd_buffer[0]))
    {
        VirtualNodes = atoi(cmd_buffer);
        if (VirtualNodes > 8) VirtualNodes = 8;
    }
    else
    {
        VirtualNodes = 0;
    }
    cmd_buffer = getenv("SQ_VIRTUAL_NID");
    if ( VirtualNodes && cmd_buffer )
    {
        MyNid = atoi(cmd_buffer);
    }
    else
    {
        MyNid = 0;
    }

    if ( VirtualNodes && VirtualNid != -1)
    {
        // Override NyNid with the command line nid value
        MyNid = VirtualNid;
    }

    env = getenv("TRAF_AGENT");
    if ( env != NULL && strcmp(env, "CM") == 0 )
    {
        AgentType = AgentType_CM;
    }
    else if ( env != NULL && strcmp(env, "Ambari") == 0 )
    {
        AgentType = AgentType_Ambari;
    }
    else
    {
        AgentType = AgentType_MPI;
    }

    msg = new struct message_def;

    // Load default node information
    if (VirtualNodes)
    {
        for(i=0; i<VirtualNodes; i++)
        {
            NodeState[i] = true;
            gethostname(Node[i], MPI_MAX_PROCESSOR_NAME);
        }
        NumDown = 0;
        if ( !load_configuration() )
        {
            exit (1);
        }
        CurNodes = NumNodes - NumDown;
    }
    else
    {
        gethostname(MyNode, MPI_MAX_PROCESSOR_NAME);
        if ( !load_configuration() )
        {
            exit (1);
        }
        CurNodes = NumNodes - NumDown;
        if (CurNodes == 0 )
        {
            gethostname(Node[0], MPI_MAX_PROCESSOR_NAME);
            NodeState[0] = true;
            NumDown = 0;
            NumNodes = CurNodes = 1;
        }
    }

    // Initialize mpirun std file settings
    MpirunInit();

    env = getenv("SQ_ELASTICY_ENABLED");
    if ( env && isdigit(*env) )
    {
        if ( strcmp(env,"0") == 0 )
        {
            ElasticityEnabled = false;
        }
    }

    env = getenv("SQ_NAMESERVER_ENABLED");
    if ( env && isdigit(*env) )
    {
        int val = atoi(env);
        NameServerEnabled = (val != 0) ? true : false;
    }

    env = getenv("SQ_QUIET_SHELL");
    if ( env && isdigit(*env) )
    {
        if ( strcmp(env,"1") == 0 )
        {
          QuietShell = true;
        }
    }

    env = getenv("SQ_NODE_ADD_USE_FQDN");
    if ( env && isdigit(*env) )
    {
        if ( strcmp( env, "0" ) == 0 )
        {
            NodeAddUseFqdn = false;
        }
        else
        {
            NodeAddUseFqdn = true;
        }
    }

    if ( !VirtualNodes )
    {
        env = getenv("SQ_COLD_STANDBY_SPARE");
        if ( env )
        {
            if ( strcmp(env,"0")==0 )
            {
                SpareNodeColdStandby = false;
            }
        }

        if ( !init_pnode_map() )
        {
            exit(1);
        }
    }

    switch (argc)
    {
    case 1:
        // defaults to interactive Master shell
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s] Startup case 1 - master shell\n",
                         method_name, __LINE__, MyName);

        tty = (isatty (fileno (stdin)) ? true : false);
        break;

    case 2:
        // Started as Master shell
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s] Startup case 2 - master shell\n",
                         method_name, __LINE__, MyName);
        if (strcmp (argv[1], "-i") == 0)
        {
            tty = true;
        }
        else if (strcmp (argv[1], "-a") == 0)
        {
            if ( ! gp_local_mon_io )
            {
                InitLocalIO();
            }
            // Started as child attached interactive shell
            if ( (Attached = attach(MyNid,MyName,(char *) "shell")) == true)
            {
                Started = true;
                Slave = true;
            }
            if ( !Started )
            {
                printf("[%s] Can't attach to monitor, aborting\n",MyName);
                if ( gp_local_mon_io )
                {
                    delete gp_local_mon_io;
                }

                exit(1);
            }
            tty = (isatty (fileno (stdin)) ? true : false);
        }
        else
        {
            tty = false;
            if (freopen (argv[1], "r", stdin) == NULL)
            {
                printf ("[%s] Can't open input file '%s'.\n",
                        MyName,
                        argv[1]);
                done = true;
            }
        }
        break;

    case 3:
        // Started as child attached batch shell with:
        // <command>
        sprintf( cmd_string, "%s", argv[2] );
    case 4:
        // <command> <token1>
        if ( argc == 4)
        {
            sprintf( cmd_string, "%s %s", argv[2], argv[3] );
        }
    case 5:
        // <command> <token1> <token2>
        if ( argc == 5)
        {
            sprintf( cmd_string, "%s %s %s", argv[2], argv[3], argv[4] );
        }
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s] Startup case %d - child attached batch "
                         "shell, command: %s\n",  method_name, __LINE__, MyName
                         , argc, cmd_string);
        if ( ! gp_local_mon_io )
        {
            InitLocalIO();
        }
        if ( (Attached = attach(MyNid,MyName,(char *) "shell")) == true )
        {
            Started = true;
        }
        tty = false;
        if (strcmp (argv[1], "-c") == 0)
        {
            normalize_case (cmd_string);
            cmd_tail = get_token (cmd_string, token, &delimiter);
            exec_one_command = true;
        }
        else
        {
            input_file = argv[2];
            if (freopen (input_file, "r", stdin) == NULL)
            {
                printf ("[%s] Can't open input file '%s'.\n", MyName, input_file);
                done = true;
            }
        }
        break;

    case 11:
    case 12:
    case 13:
        // Started as child shell
        if ( trace_settings & TRACE_SHELL_CMD )
            trace_printf("%s@%d [%s] Startup case 11/12/13 - child shell\n",
                         method_name, __LINE__, MyName);
        Started = true;
        MonitorNid = MyNid = atoi (argv[3]);
        if ( ! gp_local_mon_io )
        {
            InitLocalIO();
        }
        MyPid = atoi (argv[4]);
        gv_ms_su_verif  = MyVerifier = atoi(argv[9]);
        strcpy (MyName, argv[5]);
        tty = (isatty (fileno (stdin)) ? true : false);
        process_startup (MyNid, argv[6]);
        Attached = true;
        if (argv[11] && argv[12] && strcmp (argv[11], "-c") == 0)
        {
            cmd_tail = get_token (argv[12], token, &delimiter);
            normalize_case (token);
            exec_one_command = true;
            tty = false;
            break;
        }
        else if (argv[11] && argv[11][0] != '-')
        {
            tty = false;
            input_file = argv[11];
            if (freopen (input_file, "r", stdin) == NULL)
            {
                printf ("[%s] Can't open input file '%s'.\n", MyName, input_file);
                done = true;
            }
            break;
        }

    default:
        printf ("[%s] Shell/%s\n", MyName, CALL_COMP_GETVERS(shell));
        printf ("[%s] Invalid startup arguments\n", MyName);
        for (i = 0; i < argc; i++)
        {
            printf ("[%s] - argv[%d]='%s'\n", MyName, i, argv[i]);
        }
        printf ("[%s] Usage: shell {[-a|-i] [<scriptfile>]} | {-c <command>}\n",MyName);
        OrderlyShutdown();

        exit (1);
    }

    printf ("[%s] Shell/%s\n", MyName, CALL_COMP_GETVERS(shell));

    if ( Started && MonitorNid == -1 )
    {
        printf ("[%s] Not attach to any monitor processes, aborting\n", MyName);
        OrderlyShutdown();

        exit (1);
    }

    // Setup default path and working directory from current environment
    strncpy (Wdir, getenv ("PWD"), MAX_SEARCH_PATH);
    Wdir[MAX_SEARCH_PATH - 1] = '\0';
    strncpy (Path, getenv ("PATH"), MAX_SEARCH_PATH);
    Path[MAX_SEARCH_PATH - 1] = '\0';
    strncpy (LDpath, getenv ("LD_LIBRARY_PATH"), MAX_SEARCH_PATH);
    LDpath[MAX_SEARCH_PATH - 1] = '\0';

    if (exec_one_command)
    {
        // it's just a single command that needs to be executed.
        printf("[%s] %%%s %c%s\n", MyName, token, delimiter?delimiter:' ', cmd_tail );
        process_command ( token, cmd_tail, delimiter );
        done = true;
    }

    while (!done)
    {
        if (tty)
        {
            sprintf (prompt, "[%s] %%", MyName);
        }
        else
        {
            prompt[0] = '\0';
        }
        do
        {
            cmd_buffer = readline (prompt);
            if (cmd_buffer == NULL)
            {
                alloc_new = true;
                printf ("exit\n");
                cmd_buffer = new char[5];
                strcpy (cmd_buffer, "exit");
            }
            else if (*cmd_buffer && tty && *remove_white_space (cmd_buffer) != '!')
            {
                add_history (cmd_buffer);
            }
            else if (env_replace (cmd_buffer))
            {
                // never gets here
            }
            else if (!tty && *remove_white_space (cmd_buffer) != '!')
            {
                if (strncmp (cmd_buffer, "echo", 4) != 0)
                {
                    printf ("[%s] %%%s\n", MyName, cmd_buffer);
                }
            }
            cmd_tail = get_token (cmd_buffer, token, &delimiter);
            normalize_case (token);
        }
        while (token[0] == '!');
        if ( strcmp( token, "time" ) == 0 )
        {
            cmd_tail = get_token (cmd_tail, token, &delimiter);
            normalize_case (token);
            timer(true);
            done = process_command( token, cmd_tail, delimiter );
            printf("[%s] Execution time = %s\n", MyName, timer(false) );
        }
        else
        {
            done = process_command( token, cmd_tail, delimiter );
        }
        if (alloc_new)
           delete [] cmd_buffer;
        else
           free (cmd_buffer);
      //  delete [] cmd_buffer;
    }

    OrderlyShutdown();
    return(0);
}
