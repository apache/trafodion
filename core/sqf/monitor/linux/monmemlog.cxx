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
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "monlogging.h"
#include "msgdef.h"

#include "SCMVersHelp.h"

DEFINE_EXTERN_COMP_DOVERS(monmemlog)

//forward declarations
void displayHelp();
memLogHeader_t* attachMemLogSegment(int monpid);
void doWork(memLogHeader_t *memLogHeader, int eventStart, int eventEnd, bool waited);
void printEntry(memLogEntry_t *entry, int eventStart, int eventEnd);

const char *StateString( int state)
{
    const char *str;
    
    switch( state )
    {
        case State_Unknown:
            str = "State_Unknown";
            break;
        case State_Up:
            str = "State_Up";
            break;
        case State_Down:
            str = "State_Down";
            break;
        case State_Stopped:
            str = "State_Stopped";
            break;
        case State_Shutdown:
            str = "State_Shutdown";
            break;
        case State_Unlinked:
            str = "State_Unlinked";
            break;
        case State_Merging:
            str = "State_Merging";
            break;
        case State_Merged:
            str = "State_Merged";
            break;
        case State_Joining:
            str = "State_Joining";
            break;
        case State_Initializing:
            str = "State_Initializing";
            break;
        default:
            str = "State - Undefined";
            break;
    }

    return( str );
}

// *************************************************************************************
// This utility attaches to monitor's shared segment that has the in-memory log data.
// It displays the events for a given event id or for a range of event ids.
// With waited option, user can monitor the events continuously as they are created.
// Usage:
// monmemlog <monitor's pid> <wait/nowait> [ <eventid> | [startEventID endEventID] ]  
//
int main(int argc, char *argv[])
{
    int monPid = 0;
    int eventStart = 0;
    int eventEnd = 0;
    bool waited = false;
    memLogHeader_t *memLogHeader = NULL;

    CALL_COMP_DOVERS(monmemlog, argc, argv);

    if (argc < 3)
    {
        displayHelp();
        return(0);
    }
    
    monPid = atoi(argv[1]);

    if (strcmp(argv[2], "wait") == 0)
    {
        waited = true;
    } 
    else if (strcmp(argv[2], "nowait") == 0)
    {
        waited = false;
    }
    else
    {
        displayHelp();
        return(0);
    }

    if (argc >=4)
    {
        eventStart = atoi(argv[3]);
    }
    if (argc >=5)
    {
        eventEnd = atoi(argv[4]);
    }

    memLogHeader = attachMemLogSegment(monPid);

    if (memLogHeader != (memLogHeader_t *) -1)
    {
        doWork(memLogHeader, eventStart, eventEnd, waited);
    }

    return(0);
}

void displayHelp()
{
    printf("Syntax: \n monmemlog <monitor's pid> <wait/nowait> [ eventID | [startEventID endEventID] ]\n");
}

// attach to the monitor's shared memory segment. 
// The first attempt gets the max number of entries from the header. 
// The second attempt attaches for the full size, depending upon the max number of entries. 
memLogHeader_t* attachMemLogSegment(int monpid)
{
    int memLogID;
    memLogHeader_t *memLogAddr;
    int memLogEntries;

    memLogID = shmget( MEM_LOG_KEY(monpid), sizeof(memLogHeader_t), 0400 /* user can read */ );

    if (memLogID == -1)
    {
        printf("Error: Unable to get Monitor's Memory Log Shared Segment ID. Err = %d(%s). Exiting.\n",
                errno, strerror(errno));
        return (memLogHeader_t *)-1;
    }

    memLogAddr = (memLogHeader_t *)shmat( memLogID, NULL, 0 );

    if (memLogAddr == (memLogHeader_t *)-1)
    {
        printf("Error: Unable to attach to Monitor's Memory Log Shared Segment. Err = %d(%s). Exiting.\n", 
                errno, strerror(errno));
        return (memLogHeader_t *)-1;
    } 

    memLogEntries = memLogAddr->entries_;

    memLogID = shmget( MEM_LOG_KEY(monpid), MEM_LOG_SIZE(memLogEntries), 0400 /* user can read */ );

    if (memLogID == -1)
    {
        printf("Error: Unable to get Monitor's Memory Log Shared Segment ID. Err = %d(%s). Exiting.\n",
                errno, strerror(errno));
        return (memLogHeader_t *)-1;
    }
     
    memLogAddr = (memLogHeader_t *)shmat( memLogID, NULL, 0 );

    if (memLogAddr == (memLogHeader_t *)-1)
    {
        printf("Error: Unable to attach to Monitor's Memory Log Shared Segment. Err = %d(%s). Exiting.\n", 
                errno, strerror(errno));
        return (memLogHeader_t *)-1;
    } 

    return memLogAddr;
}

// display entries from the shared segment.
void doWork(memLogHeader_t *memLogHeader, int eventStart, int eventEnd, bool waited)
{
    memLogEntry_t *memLogBase = (memLogEntry_t *)(memLogHeader + 1);

    int currEntry = memLogHeader->currEntry_;

    // if the log buffer has cycled, print old ones first
    if (memLogHeader->cycles_ > 0) 
    {
        for (int i=currEntry; i<memLogHeader->entries_; i++)
        {
            printEntry((memLogEntry_t *)&memLogBase[i], eventStart, eventEnd);
        }
    }

    for (int i=0; i<currEntry; i++)
    {
        printEntry((memLogEntry_t *)&memLogBase[i], eventStart, eventEnd);
    }

    if (waited)
    {
        while(true)
        {
            int currEntrySaved = currEntry;

            sleep(1);

            currEntry = memLogHeader->currEntry_;

            while (currEntrySaved != currEntry)
            {
                printEntry((memLogEntry_t *)&memLogBase[currEntrySaved], eventStart, eventEnd);
                ++currEntrySaved;
                if (currEntrySaved > memLogHeader->entries_)
                {
                    currEntrySaved = 0;
                }
            }
        }
    }
}

// format the entry and print it to the terminal.
// if no format is specified, print raw values. 
// filter the entries based on given start and end event values. 
void printEntry(memLogEntry_t *entry, int eventStart, int eventEnd)
{
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64], timebuf[64];

    if (eventStart && eventEnd)
    {
        if (entry->eventType_ < eventStart || entry->eventType_ > eventEnd)
        {
            return;
        }
    }
    else if (eventStart && !eventEnd)
    {
        if (entry->eventType_ != eventStart)
        {
            return;
        }
    }

    nowtime = entry->ts_.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    snprintf(timebuf, sizeof timebuf, "%s.%06d", tmbuf, (int)(entry->ts_.tv_nsec/1000));
    printf("%s ", timebuf);

    switch(entry->eventType_)
    {
        case MON_CLUSTER_RESPONSIVE_1:
        {
            printf("Sync thread not responsive, mpi time exceeded %d secs, stuck in ", entry->value1_);
            switch (entry->value2_)
            {
                case 4: printf("Comm Dup");
                        break;
                case 2: printf("Allgather");
                        break;
                case 1: printf("Barrier");
                        break;
            }
            printf("\n");
            break;
        } 
        case MON_CLUSTER_RESPONSIVE_2:
        {
            printf("Sync thread not responsive, non-mpi time exceeded %d secs.\n", entry->value1_);
            break;
        }
        case MON_CLUSTER_RESPONSIVE_3:
        {
            printf("Sync thread is slow, Barriers per sec = %d. Minimum required = %d\n", entry->value1_, entry->value2_);
            break;
        }
        case MON_HEALTHCHECK_WAKEUP_2:
        {
            printf("Healthcheck thread late wakeup by %d secs.\n", entry->value1_);
            break;
        }
        case MON_HEALTHCHECK_TEVENT_1:
        { 
            printf("Request worker thread not responsive.\n");
            break;
        }
        case MON_HEALTHCHECK_TEVENT_2:
        {
            printf("Sync thread not responsive.\n");
            break;
        }
        case MON_HEALTHCHECK_WAKEUP_1:
        {
            printf("Healthcheck thread took %d seconds since last wakeup.\n", entry->value1_);
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_1:
        {
            printf("Snapshot request.\n");
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_2:
        {
            printf("Join Comm is Null, aborting snapshot req.\n");
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_3:
        {
            printf("Copied config.db. Error = %d.\n", entry->value1_);
            break;
        }
       case MON_REQQUEUE_SNAPSHOT_4:
        {
            printf("Estimated Proc size = %d. Sparenode size = %d\n", entry->value1_, entry->value2_);
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_5:
        {
            printf("Unable to malloc(%d) snapshot buffer.\n", entry->value1_);
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_6:
        {
            printf("Packed. Nodemap count = %d, Proc count = %d.\n", entry->value1_, entry->value2_);
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_7:
        {
            printf("Packed fullsize =  %d.\n", entry->value1_);
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_8:
        {
            printf("Unable to malloc compression buffer.\n");
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_9:
        {
            printf("Compression error = %d. compressed size = %d\n", entry->value1_, entry->value2_);
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_10:
        {
            printf("Populated and compressed snapshot buffer.\n");
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_11:
        {
            printf("Sent header. Error = %d.\n", entry->value1_);
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_12:
        {
            printf("Sent data of size %d bytes. Error = %d.\n", entry->value1_, entry->value2_);
            break;
        }
        case MON_REQQUEUE_SNAPSHOT_13:
        {
            printf("Snapshot request completed.\n");
            break;
        }
        case MON_REQQUEUE_REVIVE_1:
        {
            printf("Revive request.\n");
            break;
        }
        case MON_REQQUEUE_REVIVE_2:
        {
            printf("Received header. Error = %d.\n", entry->value1_);
            break;
        }
        case MON_REQQUEUE_REVIVE_3:
        {
            printf("Received data. Error = %d.\n", entry->value1_);
            break;
        }
        case MON_REQQUEUE_REVIVE_4:
        {
            printf("Data uncompressed. Starting to unpack.\n");
            break;
        }
        case MON_REQQUEUE_REVIVE_5:
        {
            printf("Data unpacked. Starting to process revive queue.\n");
            break;
        }
        case MON_REQQUEUE_REVIVE_6:
        {
            printf("Revive request complete.\n");
            break;
        }
        case MON_CLUSTER_MERGETONEWMON_1:
        {
            printf("Calling Intercomm_create\n");
            break;
        }
        case MON_CLUSTER_MERGETONEWMON_2:
        {
            printf("Intercomm_create error = %d.\n", entry->value1_);
            break;
        }
        case MON_CLUSTER_MERGETONEWMON_3:
        {
            printf("Intercomm_merge error = %d.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_REINTEGRATE_1:
        {
            printf("Node %d connecting to creator monitor.\n", entry->value1_);
            break;
        }
        // case CMonLog::MON_REINTEGRATE_2:
        // {
        //     printf("Node %d connected to creator monitor.\n", entry->value1_);
        //     break;
        // }
        case CMonLog::MON_REINTEGRATE_3:
        {
            printf("Node %d obtaining cluster port info.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_REINTEGRATE_4:
        {
            printf("Node %d sending node id to creator monitor.\n",
                   entry->value1_);
            break;
        }
        case CMonLog::MON_REINTEGRATE_5:
        {
            printf("Node %d connecting to node %d monitor.\n",
                   entry->value1_,  entry->value2_);

            break;
        }
        case CMonLog::MON_REINTEGRATE_6:
        {
            printf("Node %d sending node id to node %d monitor.\n",
                   entry->value1_,  entry->value2_);
            break;
        }
        case CMonLog::MON_REINTEGRATE_7:
        {
            printf("Node %d now connected to other monitors.\n",
                   entry->value1_);
            break;
        }
        case CMonLog::MON_REINTEGRATE_8:
        {
            printf("Node %d ready to participate in allgather.\n",
                   entry->value1_);
            break;
        }
        case CMonLog::MON_REINTEGRATE_9:
        {
            printf("Node %d aborted due to initialization error %d.\n",
                   entry->value1_, entry->value2_);
            break;
        }
        case CMonLog::MON_CONNTONEWMON_1:
        {
            printf("Posting MPI accept.\n");
            break;
        }
        case CMonLog::MON_CONNTONEWMON_2:
        {
            printf("Completed MPI accept.  Receiving node id from new "
                   "monitor.\n");
            break;
        }
        case CMonLog::MON_CONNTONEWMON_3:
        {
            printf("Sending port and node info for existing monitors to "
                   "node %d.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_CONNTONEWMON_4:
        {
            printf("Connected to node %d monitor.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_CONNTONEWMON_5:
        {
            printf("Waiting for node %d initialization.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_CONNTONEWMON_6:
        {
            printf("Node %d initialized, state=%d.\n", entry->value1_,
                   entry->value2_);
            break;
        }
        case CMonLog::MON_CONNTONEWMON_7:
        {
            printf("Node up for node %d failed, timed-out.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_UPDATE_CLUSTER_1:
        {
            printf("Connection to node %d is gone.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_UPDATE_CLUSTER_2:
        {
            printf("Detected seq num mismatch.  Scheduling node down for "
                   "node %d.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_UPDATE_CLUSTER_3:
        {
            printf("Node %d is down.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_UPDATE_CLUSTER_4:
        {
            printf("Node %d is going down due to inconsistent cluster view of "
                   "node %d.\n", entry->value1_, entry->value2_);
            break;
        }
        case CMonLog::MON_NODE_1:
        {
            printf("Node %d state: %s.\n", entry->value1_,
                   StateString(entry->value2_));
            break;
        }
        case CMonLog::MON_UPDATE_CLUSTER_5:
        {
            printf("Node %d changing state from down to up.\n", entry->value1_);
            break;
        }
        case CMonLog::MON_UPDATE_CLUSTER_6:
        {
            printf("Node %d changing state from merging to up.\n",
                   entry->value1_);
            break;
        }
        case CMonLog::MON_NSCONNTONEWMON_1:
        {
            printf("Completed socket accept.  Receiving node id from new "
                   "monitor.\n");
            break;
        }
        case CMonLog::MON_NSCONNTONEWMON_2:
        {
            printf("Posting socket accept.\n");
            break;
        }
        default:
        {
            printf("%d %d %d\n", entry->eventType_, entry->value1_, entry->value2_);
            break;
        }
    }
}
