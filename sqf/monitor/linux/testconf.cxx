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

///////////////////////////////////////////////////////////////////////////////
//                                                                           
//                   Seaquest Foundation
//                                                                           
///////////////////////////////////////////////////////////////////////////////
//                                                                           
// File:        testconf.cxx
//                                                                           
// Description: Cluster configuration classes test program
//                                                                           
// Classes:     None
//                                                                           
// Structures:  None
//                                                                           
// Functions:   None
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
#include "sqevlog/evl_sqlog_writer.h"
#include "clusterconf.h"

static long LoopMax             = 1;
static bool ClusterConfigTest   = false;
char Node_name[MPI_MAX_PROCESSOR_NAME];
int MyPNID = -1;
long trace_settings = 0;

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
// Usage:           Invoked by main() when no option, -? option, or 
//                  wrong options are used
//
///////////////////////////////////////////////////////////////////////////////
void DisplayUsage( void )
{
    fprintf (stderr, "\nUsage: testconf { <pm> } { -c  } [ { -l  <count> }\n"
                     "   Where:\n"
                     "          -c       - ClusterConfig test.\n"
                     "          -l       - Loops test for <count>.\n\n");
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CoreMaskString()
//
// Usage:           Invoked by TestClusterConfig()
//
///////////////////////////////////////////////////////////////////////////////
void CoreMaskString( char *str, cpu_set_t coreMask )
{
    for (int i = 0; i < 16; i++, str++ )
    {
        *str = CPU_ISSET( i, &coreMask ) ? '1' : '0';
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CoreMaskString()
//
// Usage:           Invoked by TestClusterConfig()
//
///////////////////////////////////////////////////////////////////////////////
void SpareListString( char *str, int sparePNids[], int spareCount )
{
    int len = 0;
    
    for (int i = 0; i < spareCount; i++, str+= len )
    {
        len = sprintf(str, "%d ", sparePNids[i] );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: ZoneTypeString()
//
// Usage:           Invoked by TestClusterConfig()
//
///////////////////////////////////////////////////////////////////////////////
void ZoneTypeString( char *str, ZoneType type )
{
    switch( type )
    {
        case ZoneType_Edge:
            sprintf(str, "%s", "Edge" );
            break;
        case ZoneType_Excluded:
            sprintf(str, "%s", "Excluded" );
            break;
        case ZoneType_Aggregation:
            sprintf(str, "%s", "Aggregation" );
            break;
        case ZoneType_Storage:
            sprintf(str, "%s", "Storage" );
            break;
        case ZoneType_Frontend:
            sprintf(str, "%s", "Frontend" );
            break;
        case ZoneType_Backend:
            sprintf(str, "%s", "Backend" );
            break;
        case ZoneType_Any:
            sprintf(str, "%s", "Any" );
            break;
        default:
            sprintf(str, "%s", "Undefined" );
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: TestClusterConfig()
//
// Usage:           Invoked by main()
//
///////////////////////////////////////////////////////////////////////////////
int TestClusterConfig( void )
{
    int rc   = -1;
    int pnodesCount;
    int lnodesCount;
    int sparesCount;
    int sparesInSetCount;
    int spareListCount;
    int sparePNids[MAX_NODES];
    char coreMaskStr[17] = {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','\0'};
    char zoneTypeStr[30] = {'\0'};
    char spareListStr[80] = {'\0'};
    CClusterConfig  clusterConfig;
    CPNodeConfig   *pnodeConfig;
    CLNodeConfig   *lnodeConfig;
    cpu_set_t       coreMask;

    printf( "BEGIN 'cluster.conf' Test\n" );
    
    gethostname(Node_name, MPI_MAX_PROCESSOR_NAME);
    if ( clusterConfig.Initialize() )
    {
        if ( clusterConfig.LoadConfig() )
        {
            pnodesCount = clusterConfig.GetPNodesCount();
            printf( "\nphysical nodes=%d\n", pnodesCount );

            // Print physical nodes array
            printf( "physical nodes array test\n" );
            for (int i = 0; i < pnodesCount; i++ )
            {
                pnodeConfig = clusterConfig.GetPNodeConfig( i );
                if ( pnodeConfig )
                {
                    coreMask = pnodeConfig->GetExcludedCoreMask();
                    CoreMaskString( coreMaskStr, coreMask );
                    spareListCount = pnodeConfig->GetSpareList( sparePNids );
                    if ( spareListCount )
                    {
                        SpareListString( spareListStr, sparePNids, spareListCount );
                    }
                    printf( "pnid=%d, nodename=%s, excluded=%s%s %s\n"
                          , pnodeConfig->GetPNid()
                          , pnodeConfig->GetName()
                          , coreMaskStr
                          , pnodeConfig->IsSpareNode() ? ", SPARE :" : " "
                          , pnodeConfig->IsSpareNode() ? spareListStr : " "
                          );
                }
            }
            // Print physical nodes list
            printf( "physical nodes list test\n" );
            pnodeConfig = clusterConfig.GetFirstPNodeConfig();
            for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
            {
                if ( pnodeConfig )
                {
                    coreMask = pnodeConfig->GetExcludedCoreMask();
                    CoreMaskString( coreMaskStr, coreMask );
                    spareListCount = pnodeConfig->GetSpareList( sparePNids );
                    if ( spareListCount )
                    {
                        SpareListString( spareListStr, sparePNids, spareListCount );
                    }
                    printf( "pnid=%d, nodename=%s, excluded=%s%s %s\n"
                          , pnodeConfig->GetPNid()
                          , pnodeConfig->GetName()
                          , coreMaskStr
                          , pnodeConfig->IsSpareNode() ? ", SPARE :" : " "
                          , pnodeConfig->IsSpareNode() ? spareListStr : " "
                          );
                }
            }

            lnodesCount = clusterConfig.GetLNodesCount();
            printf( "\nlogical nodes=%d\n", lnodesCount );

            // Print logical nodes array
            printf( "logical nodes array test\n" );
            for (int i = 0; i < lnodesCount; i++ )
            {
                lnodeConfig = clusterConfig.GetLNodeConfig( i );
                if ( lnodeConfig )
                {
                    
                    coreMask = lnodeConfig->GetCoreMask();
                    CoreMaskString( coreMaskStr, coreMask );
                    ZoneTypeString( zoneTypeStr, lnodeConfig->GetZoneType() );
                    printf( "pnid=%d, nid=%d, processors=%d, coremask=%s, zone=%s\n"
                          , lnodeConfig->GetPNid()
                          , lnodeConfig->GetNid()
                          , lnodeConfig->GetProcessors()
                          , coreMaskStr
                          , zoneTypeStr
                          );
                }
            }
            // Print logical nodes list
            printf( "logical nodes list test\n" );
            lnodeConfig = clusterConfig.GetFirstLNodeConfig();
            for ( ; lnodeConfig; lnodeConfig = lnodeConfig->GetNext() )
            {
                if ( lnodeConfig )
                {
                    coreMask = lnodeConfig->GetCoreMask();
                    CoreMaskString( coreMaskStr, coreMask );
                    ZoneTypeString( zoneTypeStr, lnodeConfig->GetZoneType() );
                    printf( "pnid=%d, nid=%d, processors=%d, coremask=%s, zone=%s\n"
                          , lnodeConfig->GetPNid()
                          , lnodeConfig->GetNid()
                          , lnodeConfig->GetProcessors()
                          , coreMaskStr
                          , zoneTypeStr
                          );
                }
            }

            printf( "\nphysical nodes=%d\n", pnodesCount );

            // Print physical nodes array
            printf( "logical nodes in physical nodes test\n" );
            for (int i = 0; i < pnodesCount; i++ )
            {
                pnodeConfig = clusterConfig.GetPNodeConfig( i );
                if ( pnodeConfig )
                {

                    printf( "pnid=%d, logical nodes=%d :\n"
                          , pnodeConfig->GetPNid()
                          , pnodeConfig->GetLNodesCount()
                          );
                    lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
                    for ( ; lnodeConfig; lnodeConfig = lnodeConfig->GetNextP() )
                    {
                        if ( lnodeConfig )
                        {
                            coreMask = lnodeConfig->GetCoreMask();
                            CoreMaskString( coreMaskStr, coreMask );
                            ZoneTypeString( zoneTypeStr, lnodeConfig->GetZoneType() );
                            printf( " nid=%d, processors=%d, coremask=%s, zone=%s\n"
                                  , lnodeConfig->GetNid()
                                  , lnodeConfig->GetProcessors()
                                  , coreMaskStr
                                  , zoneTypeStr
                                  );
                        }
                    }
                }
            }

            sparesCount = clusterConfig.GetSNodesCount();
            // Print spare nodes sets
            printf( "\nspares nodes=%d\n", sparesCount );
            printf( "spare node sets test\n" );

            CPNodeConfig *spareNodeConfig;
            PNodesConfigList_t *spareNodesConfigList =
                                clusterConfig.GetSpareNodesConfigList();
            PNodesConfigList_t::iterator itSn;
            for ( itSn = spareNodesConfigList->begin(); 
                  itSn != spareNodesConfigList->end(); 
                  itSn++ ) 
            {
                spareNodeConfig = *itSn;
                PNodesConfigList_t spareNodesConfigSet;
                clusterConfig.GetSpareNodesConfigSet( spareNodeConfig->GetName()
                                                    , spareNodesConfigSet );
                sparesInSetCount = spareNodesConfigSet.size();
                printf( " configured spare node=%s, nodes in spare set count=%d\n"
                      , spareNodeConfig->GetName()
                      , sparesInSetCount
                      );
                PNodesConfigList_t::iterator itSnSet;
                for ( itSnSet = spareNodesConfigSet.begin(); 
                      itSnSet != spareNodesConfigSet.end(); 
                      itSnSet++ ) 
                {
                    spareNodeConfig = *itSnSet;
                    printf( "   spare set member node=%s\n"
                          , spareNodeConfig->GetName()
                          );
                }
            }

            rc = 0;
        }
        else
        {
            printf( "FAILED 'cluster.conf' Load\n" );
        }
    }
    else
    {
        printf( "FAILED 'cluster.conf' Initialize\n" );
    }

    printf( "\nEND 'cluster.conf' Test\n" );
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: Program Main
//
///////////////////////////////////////////////////////////////////////////////
int main( int argc, char *argv[] )
{
    int error = 0;

    if ( argc == 1 )
    {
        DisplayUsage();
        return 0;
    }

    // Get required runtime options
    for ( int argx = 1; argx < argc; argx++ )
    {
        if ( strcasecmp( argv [argx], "-c" ) == 0 )
        {
            ClusterConfigTest = true;
        }
        else
        {
            DisplayUsage();
            return 0;
        }
    }
    
    // Get optional runtime options
    for ( int argx = 1; argx < argc; argx++ )
    {
        if ( strcasecmp( argv [argx], "-l" ) == 0 && argx + 1 < argc )
        {
            long count = atol(argv [++argx]);
            LoopMax = (count < 1) ? 1 : count;
        }
    }

    printf( "BEGIN Unit Test\n\n" );
    
    if ( ClusterConfigTest )
    {
        error = TestClusterConfig( );
        if ( error )
        {
            exit( EXIT_FAILURE );
        }
    }

    printf( "\nEND Unit Test\n" );
    exit( EXIT_SUCCESS );
}
