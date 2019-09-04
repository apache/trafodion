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

///////////////////////////////////////////////////////////////////////////////
//                                                                           
// File:        trafconf.cxx
//                                                                           
// Description: Trafodion configuration display utility program
//                                                                           
///////////////////////////////////////////////////////////////////////////////
using namespace std;

#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "seabed/logalt.h"
#include "tclog.h"
#include "tctrace.h"
#include "clusterconf.h"
#include "nameserverconfig.h"

#define MAX_TOKEN   132

typedef enum {
    TrafConfType_Undefined=0,         // Invalid

    TrafConfType_ClusterId,           // Display Cluster Id: -clusterid
    TrafConfType_InstanceId,          // Display Instance Id: -instanceid

    TrafConfType_NodeName,            // Display node names: -name -short
    TrafConfType_NodeName_w,          // Display node names: -wname -wshort
    TrafConfType_NodeId,              // Display node ids
    TrafConfType_MyNodeName,          // Display local node name
    TrafConfType_MyNodeId,            // Display local node id
    TrafConfType_PhysicalNodeId,      // Display physical node ids
    TrafConfType_ZoneId,              // Display zone ids
    // the above displays values as: "<value-1>  <value-2> ..."

    TrafConfType_NodeConfig,          // Display all nodes configuration attributes
    // node-id=0;node-name=n7;cores=0-1;processors=2;roles=connection,aggregation,storage

    TrafConfType_NodeMax,             // # maximim number of nodes names allowed: -node-max   --node-max
    TrafConfType_NodeIdCount,         // # configured <nid>s:        -nid-count   --nid-count
    TrafConfType_PhysicalNodeIdCount, // # configured <pnid>s:       -pnid-count  --pnid-count
    TrafConfType_SparesCount,         // # configured <spare-pnid>s: -spare-count --spare-count

    TrafConfType_PersistConfig,       // Display all persist configuration keys and attributes

    TrafConfType_PersistConfigKeys,   // Display persist configuration keys
    // PERSIST_PROCESS_KEYS = DTM,TMID,SSCP,SSMP,PSD,WDG,TNS,QMN

    TrafConfType_PersistConfigKey,    // Display persist configuration attributes of a 'key'
    // { <persist-prefix>_PROCESS_NAME    = {$<string><nid-format>} }
    // [ <persist-prefix>_PROCESS_TYPE    = {DTM|PERSIST|PSD|SSMP|TMID|WDG|TNS} ]
    // { <persist-prefix>_PROGRAM_NAME    = {<program-name>} }
    // { <persist-prefix>_REQUIRES_DTM    = {Y|N} } 
    // [ <persist-prefix>_STDOUT          = {<file-name-prefix><nid-format>} ]
    // { <persist-prefix>_PERSIST_RETRIES = {<retries> , <time-window-secs>} }
    // { <persist-prefix>_PERSIST_ZONES   = {<zid-format> [,<zid-format>] . . . }
    TrafConfType_NameServerConfig     // Display nameserver configuration

} TrafConfType_t;

bool DisplayBeginEnd = false;   // Valid only with:
                                //   TrafConfType_NodeConfig
                                //   TrafConfType_PersistConfig
                                //   TrafConfType_NameServerConfig
bool DisplayLabel = false;      // Valid only with:
                                //   TrafConfType_NodeIdCount
                                //   TrafConfType_PhysicalNodeIdCount
                                //   TrafConfType_SparesCount
bool DisplayShortHost = false;  // Valid only with:
                                //   TrafConfType_NodeName
                                //   TrafConfType_NodeName_w

char NodeName[TC_PROCESSOR_NAME_MAX] = { 0 };
char Key[MAX_TOKEN] = { 0 };
TrafConfType_t TrafConfType = TrafConfType_Undefined;
CClusterConfig  ClusterConfig;
CNameServerConfigContainer  NameServerConfig;

//char Node_name[MPI_MAX_PROCESSOR_NAME];
//int MyPNID = -1;
long TcTraceSettings = 0;

const char *FormatNidString( FormatNid_t type );
const char *FormatZidString( FormatZid_t type );

extern const char *PersistProcessTypeString( TcProcessType_t type );

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: TcLogWrite()
//
// Description:     Display event log message on terminal
//
// Usage:           Invoked to display error message
//
///////////////////////////////////////////////////////////////////////////////
int TcLogWrite(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
{
    
    pv_event_type = pv_event_type;
    pv_severity = pv_severity;
    int lv_err = 0;

    printf("%s", pp_string );

    return lv_err;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayUsage()
//
// Description:     Display usage help
//
// Usage:           Invoked by main() when no option, -? option, -h option,
//                  or invalid options are used
//
///////////////////////////////////////////////////////////////////////////////
void DisplayUsage( void )
{
    fprintf( stderr, 
"\nUsage: trafconf { -? | -h | -cid | -iid | -name | -short | -wname | -wshort | \\\n"
"                  -myname | -mynid | -nameserver | -ns | -node | -persist | \\\n"
"                  -node-max | -nid-count | -pnid-count | -spares-count | \\\n"
"                  --cid | --iid | --name| --short | --wname | --wshort | \\\n"
"                  --myname | --mynid | --nameserver | --ns | --node | --persist | \\\n"
"                  --node-max | --nid-count | --pnid-count | --spares-count  }\n"
"\n   Where:\n"
"     -?                Displays usage.\n"
"     -h                Displays usage.\n\n"

"     -cid              Displays cluster id.\n"
"     -iid              Displays instance id.\n"

"     -name             Displays all node names in configuration.\n"
"                        - Name is as stored in configuration, which could be in short host name or FQDN form.\n"
"     -short            Displays all node names in configuration in short host name form.\n"
"     -wname            Displays all node names in configuration prefixed with '-w'\n"
"                        - Name is as stored in configuration, which could be in short host name or FQDN form.\n"
"     -wshort           Displays all node names in configuration short host name form prefixed with '-w'.\n\n"

"     -myname           Displays local node name in configuration.\n"
"                        - Name is as stored in configuration, which could be in short host name or FQDN form.\n"
"     -mynid            Displays local node-id in configuration.\n\n"

"     -nameserver -ns   Displays nameserver configuration (without begin/end brackets).\n"
"     -node             Displays node configuration (without begin/end brackets).\n"
"     -persist          Displays persist configuration (without begin/end brackets).\n\n"

"     -node-max         Displays maximum number of node allowed in configuration.\n"
"     -nid-count        Displays count of node-id(s) in the configuration.\n"
"     -pnid-count       Displays count of physical-node-id(s) in the configuration.\n"
"     -spares-count     Displays count of spare physical-node-id(s) in the configuration.\n\n"

"     --cid             Displays cluster id (prefixed with 'Cluster Id:')\n"
"     --iid             Displays instance id (prefixed with 'Instance Id:')\n"

"     --myname          Displays local node name in configuration (prefixed with 'Node Name:').\n"
"     --mynid           Displays local node-id in configuration (prefixed with 'Node Id:').\n\n"

"     --nameserver --ns Displays nameserver configuration (with begin/end brackets).\n"
"     --node            Displays node configuration (with begin/end brackets).\n"
"     --persist         Displays persist configuration (with begin/end brackets).\n\n"   

"     --node-max        Displays maximum number of node allowed in configuration (prefixed with 'Maximum Nodes:').\n"
"     --nid-count       Displays count of node-id(s) in the configuration (prefixed with 'Node Ids:').\n"
"     --pnid-count      Displays count of physical-node-id(s) in the configuration (prefixed with 'Physical Node Ids:').\n"
"     --spares-count    Displays count of spare physical-node-id(s) in the configuration (prefixed with 'Spare Node Ids:').\n\n"
           );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: RoleTypeString()
//
///////////////////////////////////////////////////////////////////////////////
const char *RoleTypeString( TcZoneType_t type )
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

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayId()
//
///////////////////////////////////////////////////////////////////////////////
int DisplayId( void )
{
    int rc   = 0;

    switch (TrafConfType)
    {
        case TrafConfType_ClusterId:
            if ( DisplayLabel )
            {
                printf( "Cluster Id: " );
            }
            printf("%d", ClusterConfig.GetClusterId() );
            break;
        case TrafConfType_InstanceId:
            if ( DisplayLabel )
            {
                printf( "Instance Id: " );
            }
            printf("%d", ClusterConfig.GetInstanceId() );
            break;
        default:
            printf( "Invalid configuration type!\n" );
            rc = -1;
    }
    if ( DisplayLabel )
    {
        printf( "\n" );
    }
    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayNodeAttributes()
//
///////////////////////////////////////////////////////////////////////////////
void DisplayNodeAttributes( CLNodeConfig *lnodeConfig )
{

    char coresString[MAX_TOKEN];

    if ( lnodeConfig )
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
        printf( "node-id=%d;node-name=%s;"
                "cores=%s;processors=%d;roles=%s\n"
              , lnodeConfig->GetNid()
              , lnodeConfig->GetFqdn()
              , coresString
              , lnodeConfig->GetProcessors()
              , RoleTypeString( lnodeConfig->GetZoneType() )
              );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: NodeNameStr()
//
///////////////////////////////////////////////////////////////////////////////
const char *NodeNameStr( const char *name )
{
    char *ptr = (char *)name;

    while ( ptr && *ptr  && DisplayShortHost )
    {
        if ( *ptr == '.' && DisplayShortHost )
        {
            *ptr = '\0';
            break;
        }
        ptr++;
    }

    return(name);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayNodeName()
//
///////////////////////////////////////////////////////////////////////////////
void DisplayNodeName( CLNodeConfig *lnodeConfig, bool dashW )
{
    if ( lnodeConfig )
    {
        if ( dashW )
        {
            printf( "-w " );
        }
        printf( "%s ", NodeNameStr(lnodeConfig->GetFqdn()) );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayNameServerConfig()
//
///////////////////////////////////////////////////////////////////////////////
int DisplayNameServerConfig( void )
{
    int rc = 0;
    bool prev = false;
    CNameServerConfig *config;

    if ( DisplayBeginEnd && TrafConfType == TrafConfType_NameServerConfig )
    {
        printf( "BEGIN name-server\n\n" );
    }
    else if ( TrafConfType == TrafConfType_NameServerConfig )
    {
        printf( "\n" );
    }
    
    if (NameServerConfig.GetCount() > 0)
    {
        config = NameServerConfig.GetFirstConfig();
        printf( "nodes=" );
        for ( ; config; config = config->GetNext() )
        {
            const char *configNodeName = config->GetName();
            if (prev)
                printf( "," );
            printf( "%s", NodeNameStr(configNodeName) );
            prev = true;
        }
    }

    if ( DisplayBeginEnd && TrafConfType == TrafConfType_NameServerConfig )
    {
        printf( "\n\nEND name-server\n" );
    }
    else if ( TrafConfType == TrafConfType_NameServerConfig )
    {
        printf( "\n\n" );
    }

    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayNodesConfig()
//
///////////////////////////////////////////////////////////////////////////////
int DisplayNodeConfig( char *nodeName )
{
    int rc   = -1;
    CLNodeConfig *lnodeConfig;

    lnodeConfig = ClusterConfig.GetFirstLNodeConfig();

    if ( DisplayBeginEnd && TrafConfType == TrafConfType_NodeConfig )
    {
        printf( "BEGIN NODE\n\n" );
    }
    else if ( TrafConfType == TrafConfType_NodeConfig )
    {
        printf( "\n" );
    }
    
    for ( ; lnodeConfig; lnodeConfig = lnodeConfig->GetNext() )
    {
        if ( lnodeConfig )
        {
            if ( *nodeName == '\0' 
             || (strcmp( nodeName, lnodeConfig->GetName()) == 0))
            {
                switch (TrafConfType)
                {
                    case TrafConfType_NodeConfig:
                        DisplayNodeAttributes( lnodeConfig );
                        break;
                    case TrafConfType_NodeName:
                        DisplayNodeName( lnodeConfig, false );
                        break;
                    case TrafConfType_NodeName_w:
                        DisplayNodeName( lnodeConfig, true );
                        break;
                    default:
                        printf( "Invalid configuration type!\n" );
                }
                rc   = 0;
            }
            if (*nodeName != '\0' 
             && strcmp( nodeName, lnodeConfig->GetName()) == 0)
            {
                break;
            }
        }
    }

    if ( DisplayBeginEnd && TrafConfType == TrafConfType_NodeConfig )
    {
        printf( "\nEND NODE\n" );
    }
    else if ( TrafConfType == TrafConfType_NodeConfig )
    {
        printf( "\n" );
    }

    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayConfigCounts()
//
///////////////////////////////////////////////////////////////////////////////
int DisplayConfigCounts( void )
{
    int rc   = 0;

    switch (TrafConfType)
    {
        case TrafConfType_NodeMax:
            if ( DisplayLabel )
            {
                printf( "Maximum Nodes: " );
            }
            printf("%d", ClusterConfig.GetPNodesConfigMax() );
            break;
        case TrafConfType_NodeIdCount:
            if ( DisplayLabel )
            {
                printf( "Node Ids: " );
            }
            printf("%d", ClusterConfig.GetLNodesCount() );
            break;
        case TrafConfType_PhysicalNodeIdCount:
            if ( DisplayLabel )
            {
                printf( "Physical Node Ids: " );
            }
            printf("%d", ClusterConfig.GetPNodesCount() );
            break;
        case TrafConfType_SparesCount:
            if ( DisplayLabel )
            {
                printf( "Spare Node Ids: " );
            }
            printf("%d", ClusterConfig.GetSNodesCount() );
            break;
        default:
            printf( "Invalid configuration type!\n" );
            rc = -1;
    }
    if ( DisplayLabel )
    {
        printf( "\n" );
    }
    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayMyNode()
//
///////////////////////////////////////////////////////////////////////////////
int DisplayMyNode( void )
{
    int rc   = 0;
    char name[TC_PROCESSOR_NAME_MAX]; // hostname
    CPNodeConfig * pnodeConfig = NULL;
    CLNodeConfig * lnodeConfig = NULL;

    gethostname(name, TC_PROCESSOR_NAME_MAX);
    char *tmpptr = name;
    while ( *tmpptr )
    {
        *tmpptr = (char)tolower( *tmpptr );
        tmpptr++;
    }

    pnodeConfig = ClusterConfig.GetPNodeConfig( name );
    if (pnodeConfig == NULL)
    {
        printf( "Local host %s is not in Trafodion Configuration!\n"
              , name );
        return(-1);
    }

    lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
    if (lnodeConfig == NULL)
    {
        printf( "Logical node for local host %s is not in Trafodion Configuration!\n"
              , name );
        return(-1);
    }

    switch (TrafConfType)
    {
        case TrafConfType_MyNodeName:
            if ( DisplayLabel )
            {
                printf( "Node Name: " );
            }
            printf("%s", pnodeConfig->GetFqdn() );
            break;
        case TrafConfType_MyNodeId:
            if ( DisplayLabel )
            {
                printf( "Node Id: " );
            }
            printf("%d", lnodeConfig->GetNid() );
            break;
        default:
            printf( "Invalid configuration type!\n" );
            rc = -1;
    }
    if ( DisplayLabel )
    {
        printf( "\n" );
    }
    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayPersistKeys()
//
///////////////////////////////////////////////////////////////////////////////
int DisplayPersistKeys( void )
{
    int rc   = -1;
    char persist_config_str[MAX_TOKEN];
    CPersistConfig *persistConfig;

    persistConfig = ClusterConfig.GetFirstPersistConfig();
    if (persistConfig)
    {
        snprintf( persist_config_str, sizeof(persist_config_str)
                , "%s = ", PERSIST_PROCESS_KEYS );
        for ( ; persistConfig; persistConfig = persistConfig->GetNext() )
        {
            strcat( persist_config_str, persistConfig->GetPersistPrefix() );
            if ( persistConfig->GetNext() )
            {
                strcat( persist_config_str, "," );
            }
            rc   = 0;
        }
        printf ("%s\n\n", persist_config_str);
    }
    else
    {
        printf ("Configuration keys for persistent process do not exist\n");
    }

    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: DisplayPersistConfig()
//
///////////////////////////////////////////////////////////////////////////////
int DisplayPersistConfig( char *key )
{
    int rc   = -1;
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

    if ( DisplayBeginEnd )
    {
        printf( "BEGIN PERSIST\n\n" );
    }
    else
    {
        printf( "\n" );
    }

    if (*key == '\0')
    {
        DisplayPersistKeys();
    }
    
    persistConfig = ClusterConfig.GetFirstPersistConfig();
    if (persistConfig)
    {
        for ( ; persistConfig; persistConfig = persistConfig->GetNext() )
        {
            if (*key == '\0' ||
                 strcasecmp( key, persistConfig->GetPersistPrefix()) == 0)
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
                        , strlen(persistConfig->GetProgramArgs())
                            ?persistConfig->GetProgramArgs():""
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
                if (strcasecmp( key, persistConfig->GetPersistPrefix()) == 0)
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
                rc   = 0;
            }
        }
    }
    if (!foundConfig)
    {
        printf ("Persistent process configuration does not exist\n");
    }

    if ( DisplayBeginEnd )
    {
        printf( "\nEND PERSIST\n" );
    }
    else
    {
        printf( "\n" );
    }

    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: ProcessTrafConfig()
//
///////////////////////////////////////////////////////////////////////////////
int ProcessTrafConfig( void )
{
    int rc   = -1;

    switch (TrafConfType)
    {
        case TrafConfType_ClusterId:
        case TrafConfType_InstanceId:
            rc = DisplayId();
            break;
        case TrafConfType_NodeConfig:
        case TrafConfType_NodeName:
        case TrafConfType_NodeName_w:
            rc = DisplayNodeConfig( NodeName );
            break;
        case TrafConfType_NodeMax:
        case TrafConfType_NodeIdCount:
        case TrafConfType_PhysicalNodeIdCount:
        case TrafConfType_SparesCount:
            rc = DisplayConfigCounts();
            break;
        case TrafConfType_PersistConfig:
        case TrafConfType_PersistConfigKeys:
        case TrafConfType_PersistConfigKey:
            rc = DisplayPersistConfig( Key );
            break;
        case TrafConfType_NameServerConfig:
            rc = DisplayNameServerConfig( );
            break;
        case TrafConfType_MyNodeName:
        case TrafConfType_MyNodeId:
            rc = DisplayMyNode( );
            break;
        case TrafConfType_NodeId:
        case TrafConfType_PhysicalNodeId:
        case TrafConfType_ZoneId:
        default:
            printf( "Not implemented, yet!\n" );
    }

    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: Program Main
//
///////////////////////////////////////////////////////////////////////////////
int main( int argc, char *argv[] )
{
    int  error = 0;

    if ( argc == 1 )
    {
        DisplayUsage();
        return 0;
    }

    // Get required runtime options
    for ( int argx = 1; argx < argc; argx++ )
    {
        if ( strcmp( argv [argx], "-?" ) == 0 )
        {
            DisplayUsage();
            return 0;
        }
        else if ( strcasecmp( argv [argx], "-h" ) == 0 )
        {
            DisplayUsage();
            return 0;
        }
        else if ( strcasecmp( argv [argx], "-cid" ) == 0 )
        {
            TrafConfType = TrafConfType_ClusterId;
        }
        else if ( strcasecmp( argv [argx], "-iid" ) == 0 )
        {
            TrafConfType = TrafConfType_InstanceId;
        }
        else if ( strcasecmp( argv [argx], "-name" ) == 0 )
        {
            TrafConfType = TrafConfType_NodeName;
        }
        else if ( strcasecmp( argv [argx], "-wname" ) == 0 )
        {
            TrafConfType = TrafConfType_NodeName_w;
        }
        else if ( strcasecmp( argv [argx], "-short" ) == 0 )
        {
            DisplayShortHost = true;
            TrafConfType = TrafConfType_NodeName;
        }
        else if ( strcasecmp( argv [argx], "-wshort" ) == 0 )
        {
            DisplayShortHost = true;
            TrafConfType = TrafConfType_NodeName_w;
        }
        else if ( strcasecmp( argv [argx], "-myname" ) == 0 )
        {
            TrafConfType = TrafConfType_MyNodeName;
        }
        else if ( strcasecmp( argv [argx], "-mynid" ) == 0 )
        {
            TrafConfType = TrafConfType_MyNodeId;
        }
        else if ( ( strcasecmp( argv [argx], "-nameserver" ) == 0 ) ||
                  ( strcasecmp( argv [argx], "-ns" ) == 0 ) )
        {
            TrafConfType = TrafConfType_NameServerConfig;
        }
        else if ( ( strcasecmp( argv [argx], "--nameserver" ) == 0 ) ||
                  ( strcasecmp( argv [argx], "--ns" ) == 0 ) )
        {
            DisplayBeginEnd = true;
            TrafConfType = TrafConfType_NameServerConfig;
        }
        else if ( strcasecmp( argv [argx], "-node" ) == 0 )
        {
            TrafConfType = TrafConfType_NodeConfig;
        }
        else if ( strcasecmp( argv [argx], "-node-max" ) == 0 )
        {
            TrafConfType = TrafConfType_NodeMax;
        }
        else if ( strcasecmp( argv [argx], "-nid-count" ) == 0 )
        {
            TrafConfType = TrafConfType_NodeIdCount;
        }
        else if ( strcasecmp( argv [argx], "-pnid-count" ) == 0 )
        {
            TrafConfType = TrafConfType_PhysicalNodeIdCount;
        }
        else if ( strcasecmp( argv [argx], "-spares-count" ) == 0 )
        {
            TrafConfType = TrafConfType_SparesCount;
        }
        else if ( strcasecmp( argv [argx], "-persist" ) == 0 )
        {
            TrafConfType = TrafConfType_PersistConfig;
        }
        else if ( strcasecmp( argv [argx], "--cid" ) == 0 )
        {
            TrafConfType = TrafConfType_ClusterId;
            DisplayLabel = true;
        }
        else if ( strcasecmp( argv [argx], "--iid" ) == 0 )
        {
            TrafConfType = TrafConfType_InstanceId;
            DisplayLabel = true;
        }
        else if ( strcasecmp( argv [argx], "--myname" ) == 0 )
        {
            TrafConfType = TrafConfType_MyNodeName;
            DisplayLabel = true;
        }
        else if ( strcasecmp( argv [argx], "--mynid" ) == 0 )
        {
            TrafConfType = TrafConfType_MyNodeId;
            DisplayLabel = true;
        }
        else if ( strcasecmp( argv [argx], "--node" ) == 0 )
        {
            DisplayBeginEnd = true;
            TrafConfType = TrafConfType_NodeConfig;
        }
        else if ( strcasecmp( argv [argx], "--persist" ) == 0 )
        {
            DisplayBeginEnd = true;
            TrafConfType = TrafConfType_PersistConfig;
        }
        else if ( strcasecmp( argv [argx], "--node-max" ) == 0 )
        {
            TrafConfType = TrafConfType_NodeMax;
            DisplayLabel = true;
        }
        else if ( strcasecmp( argv [argx], "--nid-count" ) == 0 )
        {
            DisplayLabel = true;
            TrafConfType = TrafConfType_NodeIdCount;
        }
        else if ( strcasecmp( argv [argx], "--pnid-count" ) == 0 )
        {
            DisplayLabel = true;
            TrafConfType = TrafConfType_PhysicalNodeIdCount;
        }
        else if ( strcasecmp( argv [argx], "--spares-count" ) == 0 )
        {
            DisplayLabel = true;
            TrafConfType = TrafConfType_SparesCount;
        }
        else
        {
            DisplayUsage();
            return 0;
        }
    }

    char *env;
    bool traceEnabled = false;
    env = getenv("TC_TRACE_ENABLE");
    if ( env && *env == '1' )
    {
        traceEnabled = true;
    }

    if ( !ClusterConfig.Initialize( traceEnabled, NULL ) )
    {
        printf( "Failed to initialize Trafodion Configuration!\n" );
        exit( EXIT_FAILURE );
    }
    else
    {
        if ( !ClusterConfig.LoadConfig() )
        {
            printf( "Failed to load Trafodion Configuration!\n" );
            exit( EXIT_FAILURE );
        }
        if ( !NameServerConfig.LoadConfig() )
        {
            printf( "Failed to load nameserver Configuration!\n" );
            exit( EXIT_FAILURE );
        }
    }

    error = ProcessTrafConfig();
    if ( error )
    {
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}
