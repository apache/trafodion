///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>
#include <semaphore.h>
#include <unistd.h>

#include "msgdef.h"

char MyPort[MPI_MAX_PORT_NAME];
char MyName[MAX_PROCESS_NAME];
int MyRank = -1;
int MyZid = -1;
int MyNid = -1;
int MyPid = -1;
MPI_Comm Monitor;
struct message_def *msg;

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

void process_startup (int argc, char *argv[])
{
    int i;
    int rc;

    printf ("[%s] processing startup.\n", argv[5]);
    fflush (stdout);

    printf ("[%s] - argc=%d", argv[5], argc);
    for(i=0; i<argc; i++)
    {
        printf (", argv[%d]=%s",i,argv[i]);
    }
    printf ("\n");
    fflush(stdout);

    strcpy (MyName, argv[5]);
    MPI_Open_port (MPI_INFO_NULL, MyPort);

#ifdef OFED_MUTEX
    // free monitor.sem semaphore
    printf ("[%s] Opening mutex\n",MyName);
    fflush(stdout);
    char sem_name[MAX_PROCESS_PATH];
    sprintf(sem_name,"/monitor.sem2.%s",getenv("USER"));
    sem_t *mutex = sem_open(sem_name,0,0644,0);
    if(mutex == SEM_FAILED)
    {
        printf ("[%s] Can't access %s semaphore\n", MyName, sem_name);
        sem_close(mutex);
        abort();
    }
    printf ("[%s] Putting mutex\n",MyName);
    fflush(stdout);
    sem_post(mutex);
    sem_close(mutex);
#endif

    rc = MPI_Comm_connect (argv[6], MPI_INFO_NULL, 0, MPI_COMM_SELF, &Monitor);
    if (rc != MPI_SUCCESS)
    {
        printf ("[%s] failed to connected to monitor. rc = (%d)\n", argv[5], rc);
        fflush (stdout);
    }
    if (argc < 10)
    {
        printf
            ("Error: Invalid startup arguments, argc=%d, argv[0]=%s, argv[1]=%s, argv[2]=%s, argv[3]=%s, argv[4]=%s, argv[5]=%s, argv[6]=%s, argv[7]=%s, argv[8]=%s, argv[9]=%s\n",
             argc, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]);
        exit (1);
    }
    else
    {
        MyZid = atoi (argv[8]);
        msg->type = MsgType_Service;
        msg->noreply = true;
        msg->u.request.type = ReqType_Startup;
        msg->u.request.u.startup.nid = MyNid = atoi (argv[3]);
        msg->u.request.u.startup.pid = MyPid = atoi (argv[4]);
        strcpy (msg->u.request.u.startup.process_name, argv[5]);
        strcpy (msg->u.request.u.startup.port_name, MyPort);
        msg->u.request.u.startup.os_pid = getpid ();
        msg->u.request.u.startup.event_messages = true;
        msg->u.request.u.startup.system_messages = true;
        msg->u.request.u.startup.verifier = true;
        msg->u.request.u.startup.startup_size = sizeof(msg->u.request.u.startup);
        printf ("[%s] sending startup reply to monitor.\n", argv[5]);
        fflush (stdout);
        MPI_Send (msg, sizeof (struct message_def), MPI_CHAR, 0, SERVICE_TAG,
                  Monitor);
        printf ("[%s] Startup completed", argv[5]);
        if (argc > 9)
        {
            for (i = 10; i < argc; i++)
            {
                printf (", argv[%d]=%s", i, argv[i]);
            }
        }
        printf ("\n");
        fflush (stdout);
    }
}

void flush_incoming_msgs( void )
{
    int count;
    int rc;
    int complete = 0;
    bool done = false;
    MPI_Status status;

    printf ("[%s] flush incoming event & notices.\n", MyName);
    fflush (stdout);
    do
    {
        rc = MPI_Iprobe( MPI_ANY_SOURCE, MPI_ANY_TAG, Monitor, &complete, &status );
        if (rc == MPI_SUCCESS && complete)
        {
            rc = MPI_Recv (msg, sizeof (struct message_def), MPI_CHAR, MPI_ANY_SOURCE,
                            MPI_ANY_TAG, Monitor, &status );
            if (rc != MPI_SUCCESS)
            {
                printf("[%s] MPI_Recv failure on Monitor, error=%s\n", MyName, ErrorMsg(rc));
            }
        }
        else
        {
            done = true;
        }

        if (complete)
        {
            MPI_Get_count (&status, MPI_CHAR, &count);
            if (((status.MPI_TAG == NOTICE_TAG) ||
                 (status.MPI_TAG == EVENT_TAG )      ) &&
                (count == sizeof (struct message_def))   )
            {
                if (msg->u.request.type == ReqType_Notice)
                {
                    switch (msg->type)
                    {
                    case MsgType_ProcessDeath:
                        if ( msg->u.request.u.death.aborted )
                        {
                            printf ("[%s] Process %s abnormally terminated. Nid=%d, Pid=%d\n",
                                MyName, msg->u.request.u.death.process_name, 
                                msg->u.request.u.death.nid, msg->u.request.u.death.pid);
                        }
                        else
                        {
                            printf ("[%s] Process %s terminated normally. Nid=%d, Pid=%d\n", 
                                MyName, msg->u.request.u.death.process_name, 
                                msg->u.request.u.death.nid, msg->u.request.u.death.pid);
                        }
                        break;

                    case MsgType_NodeDown:
                        printf ("[%s] Node %d (%s) is DOWN\n", 
                            MyName, msg->u.request.u.down.nid, msg->u.request.u.down.node_name );
                        break;

                    case MsgType_NodeUp:
                        printf ("[%s] Node %d (%s) is UP\n", 
                            MyName, msg->u.request.u.up.nid, msg->u.request.u.up.node_name);
                        break;    

                    case MsgType_Change:
                        printf ("[%s] Configuration Change Notice for Group: %s Key: %s\n", 
                            MyName, 
                            msg->u.request.u.change.group,
                            msg->u.request.u.change.key);
                        break;
                    case MsgType_Open:
                    case MsgType_Close:
                        printf ("[%s] Open/Close process notification\n", MyName);
                        break;
                    
                    case MsgType_Event:
                        printf("[%s] Event %d received\n",
                            MyName, msg->u.request.u.event_notice.event_id);
                        break;

                    case MsgType_Shutdown:
                        printf("[%s] Shutdown notice, level=%d received\n",
                            MyName, msg->u.request.u.shutdown.level);
                        break;
                        
                    default:
                        printf("[%s] Invalid Notice Type(%d) for flush message\n",
                            MyName, msg->type);
                    }
                }    
                else
                {
                    printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                        MyName, msg->type, msg->noreply, msg->u.request.type);
                }
            }
            else
            {
                printf ("[%s] Failed to flush messages\n", MyName);
                done = true;
            }
            fflush (stdout);
        }    
    }
    while (!done);
}

void exit_process (void)
{
    int count;
    MPI_Status status;

    printf ("[%s] sending exit process message.\n", MyName);
    fflush (stdout);
    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Exit;
    msg->u.request.u.exit.nid = MyNid;
    msg->u.request.u.exit.pid = MyPid;
    MPI_Sendrecv (msg, sizeof (struct message_def), MPI_CHAR, 0, SERVICE_TAG,
                  msg, sizeof (struct message_def), MPI_CHAR, 0, REPLY_TAG,
                  Monitor, &status);
    MPI_Get_count (&status, MPI_CHAR, &count);
    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                printf ("[%s] exited process successfully. rc=%d\n",
                        MyName, msg->u.reply.u.generic.return_code);
            }
            else
            {
                printf ("[%s] exit process failed, rc=%d\n", MyName,
                        msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
        flush_incoming_msgs();
        MPI_Comm_disconnect(&Monitor);
    }
    else
    {
        printf ("[%s] exit process reply invalid.\n", MyName);
    }
    fflush (stdout);
}

int main (int argc, char *argv[])
{
    int i;
    char key[MAX_KEY_NAME];
    char value[MAX_VALUE_SIZE];
    char *env;

    msg = new struct message_def;
    // Setup HP_MPI software license
    int mpi_key = 413675219; //413675218 to display banner
    MPI_Initialized(&mpi_key);

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &MyRank);
    process_startup (argc, argv);

    // see if environment variables are set
    // cluster keys
    printf("[%s] - Testing cluster environment values.\n",MyName);
    for(i=0; i<40; i++)
    {
        sprintf(key,"CKEY%d",i);
        if ( i==2 )
        {
            sprintf(value,"cluster%da",i);
        }
        else
        {
            sprintf(value,"cluster%d",i);
        }
        env = getenv(key);
        if (env)
        {
            if (strcmp(env,value) != 0)
            {
                printf("[%s] - Key: '%s', Value: '%s' doesn't match Env: '%s'\n", 
                    MyName, key, value, env);
            }
        }
        else
        {
            printf("[%s] - Key: '%s', NOT in Env\n", MyName, key);
        }
    }

    // node keys
    printf("[%s] - Testing node environment values.\n",MyName);
    for(i=1; i<4; i++)
    {
        sprintf(key,"NKEY%d",i);
        if ( i==2 )
        {
            sprintf(value,"node%d-%da",MyNid,i);
        }
        else
        {
            sprintf(value,"node%d-%d",MyNid,i);
        }
        env = getenv(key);
        if (env)
        {
            if (strcmp(env,value) != 0)
            {
                printf("[%s] - Key: '%s', Value: '%s' doesn't match Env: '%s'\n", 
                    MyName, key, value, env);
            }
        }
        else
        {
            printf("[%s] - Key: '%s', NOT in Env\n", MyName, key);
        }
    }

    // process keys
    printf("[%s] - Testing process environment values.\n",MyName);
    for(i=1; i<4; i++)
    {
        sprintf(key,"KEY%d",i);
        if ( i==2 )
        {
            sprintf(value,"abc-%da",i);
        }
        else
        {
            sprintf(value,"abc-%d",i);
        }
        env = getenv(key);
        if (env)
        {
            if (strcmp(env,value) != 0)
            {
                printf("[%s] - Key: '%s', Value: '%s' doesn't match Env: '%s'\n", 
                    MyName, key, value, env);
            }
        }
        else
        {
            printf("[%s] - Key: '%s', NOT in Env\n", MyName, key);
        }
    }

    // exit my process
    exit_process ();

    printf ("[%s] calling Finalize!\n", MyName);
    fflush (stdout);
    MPI_Close_port(MyPort);
    MPI_Finalize ();
    delete msg;
    exit (0);
}
