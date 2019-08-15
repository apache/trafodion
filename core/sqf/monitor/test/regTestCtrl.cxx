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

#include <stdio.h>
#include <stdlib.h>

#include <set>

using namespace std;

#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;
bool tracing = false;

const char *MyName;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect

typedef struct {
    ConfigType type;
    const char * group;
    const char *key;
    const char * value;
    int noticeRcvdCount;
} RegVal_t;

size_t numRegVals;
RegVal_t regVals[]
   = {{ConfigType_Cluster, "", "ckey0", "cluster0", 0},
      {ConfigType_Cluster, "", "ckey1", "cluster1", 0},
      {ConfigType_Cluster, "", "ckey2", "cluster2", 0},
      {ConfigType_Cluster, "", "ckey3", "cluster3", 0},
      {ConfigType_Cluster, "", "ckey4", "cluster4", 0},
      {ConfigType_Cluster, "", "ckey5", "cluster5", 0},
      {ConfigType_Cluster, "", "ckey6", "cluster6", 0},
      {ConfigType_Cluster, "", "ckey7", "cluster7", 0},
      {ConfigType_Cluster, "", "ckey8", "cluster8", 0},
      {ConfigType_Cluster, "", "ckey9", "cluster9", 0},
      {ConfigType_Cluster, "", "ckey10", "cluster10", 0},
      {ConfigType_Cluster, "", "ckey11", "cluster11", 0},
      {ConfigType_Cluster, "", "ckey12", "cluster12", 0},
      {ConfigType_Cluster, "", "ckey13", "cluster13", 0},
      {ConfigType_Cluster, "", "ckey14", "cluster14", 0},
      {ConfigType_Cluster, "", "ckey15", "cluster15", 0},
      {ConfigType_Cluster, "", "ckey16", "cluster16", 0},
      {ConfigType_Cluster, "", "ckey17", "cluster17", 0},
      {ConfigType_Cluster, "", "ckey18", "cluster18", 0},
      {ConfigType_Cluster, "", "ckey19", "cluster19", 0},
      {ConfigType_Cluster, "", "ckey20", "cluster20", 0},
      {ConfigType_Cluster, "", "ckey21", "cluster21", 0},
      {ConfigType_Cluster, "", "ckey22", "cluster22", 0},
      {ConfigType_Cluster, "", "ckey23", "cluster23", 0},
      {ConfigType_Cluster, "", "ckey24", "cluster24", 0},
      {ConfigType_Cluster, "", "ckey25", "cluster25", 0},
      {ConfigType_Cluster, "", "ckey26", "cluster26", 0},
      {ConfigType_Cluster, "", "ckey27", "cluster27", 0},
      {ConfigType_Cluster, "", "ckey28", "cluster28", 0},
      {ConfigType_Cluster, "", "ckey29", "cluster29", 0},
      {ConfigType_Cluster, "", "ckey30", "cluster30", 0},
      {ConfigType_Cluster, "", "ckey31", "cluster31", 0},
      {ConfigType_Cluster, "", "ckey32", "cluster32", 0},
      {ConfigType_Cluster, "", "ckey33", "cluster33", 0},
      {ConfigType_Cluster, "", "ckey34", "cluster34", 0},
      {ConfigType_Cluster, "", "ckey35", "cluster35", 0},
      {ConfigType_Cluster, "", "ckey36", "cluster36", 0},
      {ConfigType_Cluster, "", "ckey37", "cluster37", 0},
      {ConfigType_Cluster, "", "ckey38", "cluster38", 0},
      {ConfigType_Cluster, "", "ckey39", "cluster39", 0},
      {ConfigType_Cluster, "", "ckey40", "cluster40", 0},
      {ConfigType_Node, "NODE0", "nkey1", "node0-1", 0},
      {ConfigType_Node, "NODE0", "nkey2", "node0-2", 0},
      {ConfigType_Node, "NODE0", "nkey3", "node0-3", 0},
      {ConfigType_Node, "NODE1", "nkey1", "node1-1", 0},
      {ConfigType_Node, "NODE1", "nkey2", "node1-2", 0},
      {ConfigType_Node, "NODE1", "nkey3", "node1-3", 0},
      {ConfigType_Process, "$abc", "key1", "abc-1", 0},
      {ConfigType_Process, "$abc", "key2", "abc-2", 0},
      {ConfigType_Process, "$CTRLR", "CTRLR-key1", "ctrlr-1", 0},
      {ConfigType_Process, "$CTRLR", "CTRLR-key2", "ctrlr-2", 0},
};

typedef set<string> configChangeNotice_t;
configChangeNotice_t configChanges;


void recv_change_msg(struct message_def *recv_msg, int )
{
    if ( recv_msg->type == MsgType_Change )
    {
        if ( tracing )
        {
            printf("[%s] Got Change notice: type=%d, group=%s, key=%s, "
                   "value=%s\n",
                   MyName, recv_msg->u.request.u.change.type,
                   recv_msg->u.request.u.change.group,
                   recv_msg->u.request.u.change.key,
                   recv_msg->u.request.u.change.value);
        }

        // Save "key" for configuration change notice so can validate later.
        configChanges.insert ( recv_msg->u.request.u.change.key );

    }
    else
    {
        printf("[%s] unexpected change notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
    }
}

void recv_notice_msg(struct message_def *recv_msg, int )
{
    {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
    }
}

void cleanup ()
{
    // tell monitor we are exiting
    util.requestExit ( );

    if ( tracing ) printf ("[%s] calling Finalize!\n", MyName);

    XMPI_Close_port( util.getPort() );
    MPI_Finalize ();
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
}

bool setRegValues()
{
    bool result = true;

    if ( tracing )
    {
        printf("[%s] Setting %d configuration values.\n", MyName, (int) numRegVals);
    }
    for (size_t i=0; i<numRegVals; i++)
    {
        if ( tracing )
        {
            printf("[%s] Setting type=%d, group=%s, key=%s, value=%s\n",
                   MyName, regVals[i].type, regVals[i].group,
                   regVals[i].key, regVals[i].value);
        }
        if (!util.requestSet ( regVals[i].type, regVals[i].group,
                               regVals[i].key, regVals[i].value ))
        {
            result = false;
            break;
        }
    }

    return result;
}

bool getRegValues()
{
    bool result = true;

    if ( tracing )
    {
        printf("[%s] Getting %d configuration values\n", MyName, (int) numRegVals);
    }
    struct Get_reply_def * regData;
    for (size_t i=0; i<numRegVals; i++)
    {
        util.requestGet ( regVals[i].type, regVals[i].group, regVals[i].key,
                          false, regData );
        if ( tracing )
        {
            printf("[%s] Got reg value: type=%d, group=%s, num_keys=%d, "
                   "num_returned=%d, key=%s, value=%s\n", MyName,
                   regData->type, regData->group, regData->num_keys,
                   regData->num_returned, regData->list[0].key,
                   regData->list[0].value);
        }
        if ( regData->num_returned == 1 )
        {
//            markRegValue( regData->list[0].key, regData->list[0].value );

        }
        else
        {
            printf("[%s] *** Error *** Failed to retrieve value for key=%s, "
                   "type=%d, group=%s\n",
                   MyName, regVals[i].key, regVals[i].type, regVals[i].group);
            result = false;
        }

        free(regData);
    }

    return result;
}

// Purpose: Verify that we got configuration change notices for all
//   configuration values for which we expected them.
//   Should get all cluster wide configuration changes, all changes
//   for our node, and changes for our process.
bool checkChangeNotices()
{
    bool result = true;
    configChangeNotice_t::const_iterator it;
    char upKey[MAX_KEY_NAME];
    char *ptr;

    for (size_t i=0; i<numRegVals; i++)
    {
        if (   regVals[i].type == ConfigType_Cluster
            || regVals[i].type == ConfigType_Node
            || ( regVals[i].type == ConfigType_Process
                 && strcasecmp(regVals[i].group, MyName) == 0))
        {
            // Get upper case key
            strcpy(upKey, regVals[i].key );
            ptr = upKey;
            while(*ptr)
            {
                *ptr = toupper (*ptr);
                ptr++;
            }

            it = configChanges.find ( upKey );
            if ( it != configChanges.end() )
            {  // Found the configuration change
                if ( tracing )
                {
                    printf("[%s] Got configuration change notice for key=%s\n",
                           MyName,  regVals[i].key);
                }
            }
            else
            {
                printf("[%s] *** Error *** No configuration change notice "
                       "received for key=%s\n",
                       MyName, regVals[i].key);
                result = false;
            }
        }
        else
        {
            if ( tracing )
            {
                printf("[%s] Not expecting configuration change notice "
                       "for type=%d, group=%s\n", MyName, regVals[i].type,
                       regVals[i].group);
            }
        }
    }

    return result;
}


// Purpose: validates that ReqType_Get works as expected when
//          asked to get all configuration values for the given type and group
bool getAllRegValues( ConfigType type, const char *group)
{
    bool result = true;
    struct Get_reply_def * regData;

    bool gotClusterVal[numRegVals];
    for (size_t i=0; i<numRegVals; i++)
    {
        gotClusterVal[i] = false;
    }

    if ( tracing )
    {
        printf("[%s] Getting all cluster configuration values for type=%d, "
               "group=%s\n", MyName, type, group);
    }

    // Retrieve data of the specified type
    bool moreRegData = false;
    int numReturned = 0;
    char nextKey[MAX_KEY_NAME];
    nextKey[0] = '\0';
    do
    {
        util.requestGet ( type, group, nextKey, moreRegData, regData );
        if ( tracing )
        {
            printf("[%s] Got reg value: type=%d, group=%s, num_keys=%d, "
                   "num_returned=%d\n", MyName, regData->type, regData->group,
                   regData->num_keys, regData->num_returned);
        }

        for (int i=0; i<regData->num_returned; i++)
        {
            numReturned++;
            if ( tracing )
            {
                printf("[%s] Got reg value:  key=%s, value=%s\n", MyName,
                       regData->list[i].key, regData->list[i].value);
            }
            for (size_t j=0; j<numRegVals; j++)
            {
                if ( regVals[j].type == type
                 && (strcasecmp(regVals[j].group, group) == 0)
                 && (strcasecmp( regData->list[i].key, regVals[j].key ) == 0))
                {
                    gotClusterVal[j] = true;
                    if ( tracing )
                    {
                        printf("[%s] Marking key[%d]=%s as retrieved\n",
                               MyName, (int)j, regVals[j].key);
                    }
                }
            }
        }
        if ( (moreRegData = (numReturned < regData->num_keys)))
        {
            strcpy ( nextKey, regData->list[regData->num_returned-1].key);
            if ( tracing )
            {
                printf("[%s] Getting more reg values following %s\n", MyName,
                       nextKey);
            }
        }
        free(regData);

    }
    while (moreRegData);

    // Verify that got the expected values
    for (size_t j=0; j<numRegVals; j++)
    {
        if ( regVals[j].type == type
             && strcmp(regVals[j].group, group) == 0
             && !gotClusterVal[j] )
        {
            printf("[%s] *** Error *** Did not retrieve key %s "
                   "(type=%d, group=%s)\n",
                   MyName, regVals[j].key, regVals[j].type, regVals[j].group);
            result = false;
        }
    }

    return result;
}


int main (int argc, char *argv[])
{
    bool testSuccess = true;

    if ( tracing ) util.setTrace (tracing);

    util.processArgs (argc, argv);
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    // Set trace flag in case command line argument specified tracing.
    tracing = util.getTrace ();

    // Set local io callback function for "open", "change" and
    // "process created".
    gp_local_mon_io->set_cb(recv_change_msg, "recv");

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb(recv_notice_msg, "notice");

    util.requestStartup ();

    numRegVals = sizeof(regVals) / sizeof(RegVal_t);

    if ( testSuccess == true )
    {
        // Verify that we have the needed configuration
        testSuccess = util.validateNodeCount(3);
    }

    if ( testSuccess == true )
    {
        // Set configuration values from our table of keys and values.
        testSuccess = setRegValues();
    }

    if ( testSuccess == true )
    {
        // Now retrieve the configuration values based on our table.
        // Verify that we can retrieve each one.
        testSuccess = getRegValues();
    }

    if ( testSuccess == true )
    {
        testSuccess = checkChangeNotices();
    }

    if ( testSuccess == true )
    {
        // Verify that can get all registry values for the cluster
        testSuccess = getAllRegValues(ConfigType_Cluster, "");
    }

    if ( testSuccess == true )
    {
        // Verify that can get all registry values for the "NODE0"
        testSuccess = getAllRegValues(ConfigType_Node, "NODE0");
    }

    if ( testSuccess == true )
    {
        // Verify that can get all registry values for the "NODE1"
        testSuccess = getAllRegValues(ConfigType_Node, "NODE1");
    }

    if ( testSuccess == true )
    {
        // Verify that can get all registry values for the local node
        testSuccess = getAllRegValues(ConfigType_Node, "");
    }

    if ( testSuccess == true )
    {
        // Verify that can get all registry values for the process "$abc"
        testSuccess = getAllRegValues(ConfigType_Process, "$abc");
    }

    if ( testSuccess == true )
    {
        // Verify that can get all registry values for the process "$CTRLR"
        testSuccess = getAllRegValues(ConfigType_Process, "$CTRLR");
    }

    printf("Registry Test:\t\t\t%s\n", (testSuccess) ? "PASSED" : "FAILED");

    cleanup();
    exit (0);
}
