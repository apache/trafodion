/**********************************************************************
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
********************************************************************/
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#include <new>
#include <stdio.h>
#include <list>
#include <string>

#include "msgdef.h"
#include "montrace.h"
#include "monlogging.h"
#include "pnode.h"
#include "zookeeper/zookeeper.h"
#include "zclient.h"
#include "zootest.h"

using namespace std;

bool debugFlag = true;

bool IsAgentMode = false;
bool IsMaster = false;
bool IsRealCluster = true;
bool ZClientEnabled = true;
char Node_name[MAX_PROCESSOR_NAME] = {'\0'};
char MyPNidStr[8];
int MyPNID = -1;
int MyNid = -1;
int MyPid = -1;
int InstanceId = -1;
extern CNodeContainer *Nodes;

CZClient    *ZClient = NULL;
CMonLog     *MonLog =  NULL;

void HandleMyNodeExpiration( void )
{
    const char method_name[] = "HandleMyNodeExpiration";
    TRACE_ENTRY;
    printf( "%s@%d ZSession expired!\n", method_name, __LINE__ );
    ZClient->StopMonitoring();
    ZClient->ShutdownWork();
    printf( "%s@%d zootest exiting!\n", method_name, __LINE__ );
    TRACE_EXIT;
    exit( 1  );
}

void HandleNodeChange( const char *nodeName )
{
    const char method_name[] = "HandleNodeChange";
    TRACE_ENTRY;
    printf( "%s@%d Node %s znode changed!\n"
          , method_name, __LINE__, nodeName );
    TRACE_EXIT;
}

void HandleNodeConfigurationChange( void )
{
    const char method_name[] = "HandleNodeConfigurationChange";
    TRACE_ENTRY;
    printf( "%s@%d Node configuration changed!\n"
          , method_name, __LINE__ );
    TRACE_EXIT;
}

void HandleNodeCreated( const char *nodeName )
{
    const char method_name[] = "HandleNodeCreated";
    TRACE_ENTRY;
    printf( "%s@%d Node %s znode created!\n"
          , method_name, __LINE__, nodeName );
    TRACE_EXIT;
}

void HandleNodeError( const char *nodeName )
{
    const char method_name[] = "HandleNodeError";
    TRACE_ENTRY;
    printf( "%s@%d Node %s ERROR child!\n"
          , method_name, __LINE__, nodeName );
    TRACE_EXIT;
}

void HandleNodeExpiration( const char *nodeName )
{
    const char method_name[] = "HandleNodeExpiration";
    TRACE_ENTRY;
    printf( "%s@%d Node %s znode deleted!\n"
          , method_name, __LINE__, nodeName );
    TRACE_EXIT;
}

void CreateZookeeperClient( void )
{
    const char method_name[] = "CreateZookeeperClient";
    TRACE_ENTRY;

    if ( ZClientEnabled )
    {
        string       hostName;
        string       instanceId;
        string       trafodionRootZNode;
        stringstream ss;
        string       zkQuorumHosts;
        stringstream zkQuorumPort;
        char *env;
        char  hostsStr[MAX_PROCESSOR_NAME*3] = { 0 };
        char *tkn = NULL;

        int zport;
        env = getenv("ZOOKEEPER_PORT");
        if ( env && isdigit(*env) )
        {
            zport = atoi(env);
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s], Zookeeper quorum port is not defined!\n"
                    , method_name);
            mon_log_write(MON_ZOOCLIENT_MAIN_3, SQ_LOG_CRIT, buf);

            ZClientEnabled = false;
            TRACE_EXIT;
            return;
        }
        
        env = getenv("ZOOKEEPER_NODES");
        if ( env )
        {
            zkQuorumHosts = env;
            if ( zkQuorumHosts.length() == 0 )
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf),
                         "[%s], Zookeeper quorum hosts are not defined!\n"
                        , method_name);
                mon_log_write(MON_ZOOCLIENT_MAIN_4, SQ_LOG_CRIT, buf);

                ZClientEnabled = false;
                TRACE_EXIT;
                return;
            }
            
            strcpy( hostsStr, zkQuorumHosts.c_str() );
            zkQuorumPort.str( "" );
            
            tkn = strtok( hostsStr, "," );
            do
            {
                if ( tkn != NULL )
                {
                    hostName = tkn;
                    zkQuorumPort << hostName.c_str()
                                 << ":" 
                                 << zport;
                }
                tkn = strtok( NULL, "," );
                if ( tkn != NULL )
                {
                    zkQuorumPort << ",";
                }
                
            }
            while( tkn != NULL );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d zkQuorumPort=%s\n"
                            , method_name, __LINE__
                            , zkQuorumPort.str().c_str() );
            }
        }
    
        env = getenv("TRAF_ROOT_ZNODE");
        if ( env )
        {
            ss.str( "" );
            ss << env;
            trafodionRootZNode = ss.str();
        }
        else
        {
            ss.str( "" );
            ss << "/trafodion";
            trafodionRootZNode = ss.str();

            char la_buf[MON_STRING_BUF_SIZE];
            sprintf( la_buf
                   , "[%s], Environment variable TRAF_ROOT_ZNODE is undefined, defaulting trafodionRootZNode=%s\n"
                   , method_name, trafodionRootZNode.c_str() );
        }

        env = getenv("TRAF_INSTANCE_ID");
        if ( env && isdigit(*env) )
        {
            InstanceId = atoi(env);
            ss.str( "" );
            ss << "/" << InstanceId;
            instanceId = ss.str();
        }
        else
        {
            InstanceId = 1;
            ss.str( "" );
            ss << "/" << InstanceId;
            instanceId = ss.str();

            char la_buf[MON_STRING_BUF_SIZE];
            sprintf( la_buf
                   , "[%s], Environment variable TRAF_INSTANCE_ID is undefined, defaulting instanceId=%s\n"
                   , method_name, instanceId.c_str() );
            printf( "%s", la_buf);
        }

        ZClient = new CZClient( zkQuorumPort.str().c_str()
                              , trafodionRootZNode.c_str()
                              , instanceId.c_str() );
        if ( ZClient == NULL )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s], Failed to allocate ZClient object!\n"
                    , method_name);
            mon_log_write(MON_ZOOCLIENT_MAIN_5, SQ_LOG_CRIT, buf);
            abort();
        }
    }

    TRACE_EXIT;
}

/*
 *
 * The znode hierarchy is as follows:
 *    /trafodion/<instance-name>/cluster
 *    /trafodion/<instance-name>/cluster/<node-name-1>
 *    /trafodion/<instance-name>/cluster/<node-name-2>
 * Each monitor will create an ephemeral node using its node name (hostname)
 * followed by its <pnid>.
 * The monitor processes will watch the cluster parent znode changes.
 * When a change in the cluster znode occurs they will check the state of
 * each child. A missing child znode will is assumed to be a down node.
 *
 */
int main( int argc, char *argv[] )
{
    const char method_name[] = "main";
    TRACE_ENTRY;

    char *env;
    char  MyName[MAX_PROCESSOR_NAME];

    trace_settings |= TRACE_INIT;

    int   count = 1;
    while ( count < argc )
    {
        if ( strcmp( argv[count], "-pnid" ) == 0 )
        {
            if ( ++count < argc )
            {
                MyPNID=atoi( argv[count] );
            }
        }
        count++;
    }


    sigset_t newset, oldset;
    sigemptyset( &newset );
    sigaddset( &newset,SIGQUIT );
    sigaddset( &newset,SIGTERM );
    sigprocmask( SIG_BLOCK,&newset,&oldset );

    gethostname(Node_name, MAX_PROCESSOR_NAME);

    sprintf( MyName,"zooclient" );
    MyPid = getpid();

    MonLog = new CMonLog( "log4cxx.monitor.wdg.config", "ZOO", "alt.wdg", MyPNID, MyNid, MyPid, MyName  );

    int rc;
    env = getenv("SQ_MON_ZCLIENT_ENABLED");
    if ( env )
    {
        if ( env && isdigit(*env) )
        {
            if ( strcmp(env,"0")==0 )
            {
                ZClientEnabled = false;
            }
        }
    }

    if ( ZClientEnabled )
    {
        CreateZookeeperClient();

        sleep( 3 );  // Wait for the other zclients to register

        rc = ZClient->StartWork();
        if (rc != 0)
        {
            TRACE_EXIT;
            exit( 1 );
        }
        
        ZClient->StartMonitoring();
    
        unsigned int sleepTime = 10; // 10 seconds
        env = getenv("MON_INIT_SLEEP");
        if ( env && isdigit(*env) )
        {
            sleepTime = atoi(env);
        }
        sleep( sleepTime );  // Til' quitting time!
    
        ZClient->StopMonitoring();
        
        sleep( 1 );
    
        // Stop the Process Monitor thread
        rc = ZClient->ShutdownWork();
        if (rc != 0)
        {
            TRACE_EXIT;
            exit( 1 );
        }
    }
    else
    {
        printf( "%s@%d ZClient is disabled, exiting!\n"
              , method_name, __LINE__ );
    }

    printf( "%s@%d zootest exiting!\n"
          , method_name, __LINE__ );

    TRACE_EXIT;
    exit( 0 );
}
