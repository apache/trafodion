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
#include <mpi.h>

#include "msgdef.h"
#include "montrace.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "clusterconf.h"

#define MAX_TOKEN   132

typedef enum {
    TrafConfType_Undefined=0,         // Invalid
    TrafConfType_NodeName,            // Display node names
    TrafConfType_NodeName_w,          // Display node names
    TrafConfType_NodeId,              // Display node ids
    TrafConfType_PhysicalNodeId,      // Display physical node ids
    TrafConfType_ZoneId,              // Display zone ids
    // the above displays values as: "<value-1>  <value-2> ..."
    TrafConfType_NodeConfig,          // Display all nodes configuration attributes
    // node-id=0;node-name=n7;cores=0-1;processors=2;roles=connection,aggregation,storage

    TrafConfType_PersistConfig,       // Display all persist configuration keys and attributes

    TrafConfType_PersistConfigKeys,   // Display persist configuration keys
    // PERSIST_PROCESS_KEYS = DTM,TMID,SSCP,SSMP,PSD,WDG,QMN

    TrafConfType_PersistConfigKey     // Display persist configuration attributes of a 'key'
    // { <persist-prefix>_PROCESS_NAME    = {$<string><nid-format>} }
    // [ <persist-prefix>_PROCESS_TYPE    = {DTM|PERSIST|PSD|SSMP|TMID|WDG} ]
    // { <persist-prefix>_PROGRAM_NAME    = {<program-name>} }
    // { <persist-prefix>_REQUIRES_DTM    = {Y|N} } 
    // [ <persist-prefix>_STDOUT          = {<file-name-prefix><nid-format>} ]
    // { <persist-prefix>_PERSIST_RETRIES = {<retries> , <time-window-secs>} }
    // { <persist-prefix>_PERSIST_ZONES   = {<zid-format> [,<zid-format>] . . . }

} TrafConfType_t;

bool DisplayBeginEnd = false;   // Valid only with:
                                //   TrafConfType_NodeConfig
                                //   TrafConfType_PersistConfig
bool DisplayShortHost = false;  // Valid only with:
                                //   TrafConfType_NodeName
                                //   TrafConfType_NodeName_w

char NodeName[MPI_MAX_PROCESSOR_NAME] = { 0 };
char Key[MAX_TOKEN] = { 0 };
TrafConfType_t TrafConfType = TrafConfType_Undefined;
CClusterConfig  ClusterConfig;

//char Node_name[MPI_MAX_PROCESSOR_NAME];
//int MyPNID = -1;
long trace_settings = 0;

const char *FormatNidString( FormatNid_t type );
const char *FormatZidString( FormatZid_t type );

extern const char *PersistProcessTypeString( PROCESSTYPE type );

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: mon_log_write()
//
// Description:     Display event log message on terminal
//
// Usage:           Invoked to display error message
//
///////////////////////////////////////////////////////////////////////////////
int mon_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
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
"\nUsage: trafconf { -? | -h | -name | -nameshort | -wname | -wnameshort | -node |  -persist }\n"
"\n   Where:\n"
"          -?           Displays usage.\n"
"          -h           Displays usage.\n"
"          -name        Displays all node names in configuration.\n"
"                        - Name is as stored in configuration, which could be in short host name or FQDN form.\n"
"          -short       Displays all node names in configuration in short host name form.\n"
"          -wname       Displays all node names in configuration prefixed with '-w'\n"
"                        - Name is as stored in configuration, which could be in short host name or FQDN form.\n"
"          -wshort      Displays all node names in configuration short host name form prefixed with '-w'.\n"
"          -node        Displays node configuration (without begin/end brackets).\n"
"          -persist     Displays persist configuration (without begin/end brackets).\n\n"
"          --node       Displays node configuration (with begin/end brackets).\n"
"          --persist    Displays persist configuration (with begin/end brackets).\n\n"
           );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: RoleTypeString()
//
///////////////////////////////////////////////////////////////////////////////
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
              , lnodeConfig->GetName()
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
        printf( "%s ", NodeNameStr(lnodeConfig->GetName()) );
    }
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
        case TrafConfType_NodeConfig:
        case TrafConfType_NodeName:
        case TrafConfType_NodeName_w:
            rc = DisplayNodeConfig( NodeName );
            break;
        case TrafConfType_PersistConfig:
        case TrafConfType_PersistConfigKeys:
        case TrafConfType_PersistConfigKey:
            rc = DisplayPersistConfig( Key );
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
        else if ( strcasecmp( argv [argx], "-node" ) == 0 )
        {
            TrafConfType = TrafConfType_NodeConfig;
        }
        else if ( strcasecmp( argv [argx], "-persist" ) == 0 )
        {
            TrafConfType = TrafConfType_PersistConfig;
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
        else
        {
            DisplayUsage();
            return 0;
        }
    }

    char *env;
    env = getenv("TC_TRACE_ENABLE");
    if ( env && *env == '1' )
    {
        trace_settings |= TRACE_TRAFCONFIG;
    }

    if ( !ClusterConfig.Initialize() )
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
    }

    error = ProcessTrafConfig();
    if ( error )
    {
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}
