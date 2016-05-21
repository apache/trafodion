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

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "robsem.h"

RobSem::RobSem( int shmid, int flags, void * sharedSeg, int semMax )
    : shmid_(shmid), shmFlags_(flags), semMax_(semMax),
      sharedSeg_((sharedSeg_t *) sharedSeg)
      
{
    pid_ = getpid();
}

RobSem::~RobSem()
{
    if ( shmFlags_ & IPC_CREAT )
    {
        sem_destroy( &sharedSeg_->sem );

        // Mark shared segment to be destroyed after last
        // process detaches.
        struct shmid_ds shmbuf;
        shmctl( shmid_, IPC_RMID, &shmbuf );
    }

    // Detach from shared segment
    shmdt( sharedSeg_ );
}

unsigned int RobSem::getSegKey ( unsigned int applId, unsigned int segId,
                                 int nid)
{
    unsigned int segKey;

    char *envp = getenv("SQ_VIRTUAL_NODES");
    if (envp != NULL) 
    {
        segKey = applId + (nid << 16) + segId;
    }
    else
    {
        segKey = applId + segId;
    }

    return segKey;
}

int RobSem::create_sem( unsigned int sharedSegKey,
                        int flags,
                        RobSem *& sem,
                        unsigned int     value)
{
    RobSem * robSem;
    int shmid;
    int rc = -1;
    int pidMax;

    // Get configured maximum number of process ids
    char buf[20];
    FILE *pidMaxFile;
    pidMaxFile = fopen("/proc/sys/kernel/pid_max", "r");
    fgets(buf, sizeof(buf), pidMaxFile);
    if (pidMaxFile) fclose(pidMaxFile);
    errno = 0;
    pidMax = (int) strtol(buf, NULL, 10);
    if ( errno != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf),
                 "[%s], Error obtaining /proc/sys/kernel/pid_max: %s (%d)\n",
                 "RobSem::create_sem", strerror(errno), errno);
        log_error( MON_ROBSEM_2, SQ_LOG_CRIT, buf);

        assert (false);
    }

    size_t size = sizeof(sharedSeg_t) + ( pidMax * sizeof(unsigned short int));

    // Allocate shared memory area for semaphore management
    shmid = shmget( sharedSegKey,
                    size,
                    SHM_NORESERVE | flags | SHM_PERMISSIONS );
    if ( shmid != -1 )
    {
        void *sharedSeg;

        // Attach the shared memory segment
        sharedSeg = shmat( shmid, NULL, 0 );

        if ( sharedSeg == (void *) -1 )
        {
            rc = errno;

            char buf[256];
            snprintf(buf, sizeof(buf),
                     "[%s], Failed attaching shared memory: %s (%d).\n",
                     "RobSem::create_sem", strerror(errno), errno);
            log_error( MON_ROBSEM_1, SQ_LOG_ERR, buf );

            sharedSeg = NULL;
        }
        else
        {
            robSem = new RobSem( shmid, flags, sharedSeg, value );

            if ( flags & IPC_CREAT )
            {
                robSem->init((int) pidMax);
            }

            sem = robSem;

            rc = 0;
        }
    }
    else
    {
        rc = errno;
    }

    return rc;
}

int RobSem::destroy_sem( RobSem *sem )
{
    delete sem;

    return 0;
}

int RobSem::init( int pidMax )
{
    // Initialize semaphore that controls access to users list
    if (sem_init( &sharedSeg_->sem, 1, semMax_) != 0 )
    {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "[%s], Failed initializing semaphore: %s (%d).\n",
                 "RobSem::init", strerror(errno), errno); 
        log_error( MON_ROBSEM_3, SQ_LOG_ERR, buf );
    }
    else
    {
        // Initialize array used to hold process ids
        for (int i=0; i < pidMax; ++i)
        {
            sharedSeg_->users[i] = 0;
        }
    }

    return 0;
}


int RobSem::post()
{
    // Remove process as user of the semaphore
    __sync_sub_and_fetch (&sharedSeg_->users[pid_], 1);

    int rc = 0;
    if (sem_post( &sharedSeg_->sem ) == -1)
    {
        rc = errno;
    }

    return rc;
}


int RobSem::post_all( pid_t user )
{
    unsigned short int useCount;

    useCount = sharedSeg_->users[user];
    sharedSeg_->users[user] = 0;

    while ( useCount != 0 )
    {
        // Remove specified process as user of the semaphore
        if ( sem_post( &sharedSeg_->sem ) == -1)
        {  // Unexpectedly have invalid semaphore)
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s] sem_post error: %s (%d)\n", "RobSem::post_all",
                     strerror(errno), errno);
            log_error( MON_ROBSEM_4, SQ_LOG_CRIT, buf);
            
            break;
        }

        --useCount;
    }

    return 0;
}


int RobSem::trywait()
{
    int rc;

    do
    {
        rc = sem_trywait( &sharedSeg_->sem );
    } while ( rc == -1 && errno == EINTR );

    if ( rc == 0 )
    {   // Add process as user of the semaphore
        __sync_add_and_fetch (&sharedSeg_->users[pid_], 1);
    }
    else
    {
        rc = errno;
    }

    return rc;
}

int RobSem::value( int * value )
{
    int rc = 0;

    if (sem_getvalue( &sharedSeg_->sem, value ) == -1)
    {
        rc = errno;
    }

    return rc;
}

int RobSem::wait()
{
    int rc;

    do
    {
        rc = sem_wait( &sharedSeg_->sem );
    } while ( rc == -1 && errno == EINTR );

    if ( rc == 0 )
    {   // Add process as user of the semaphore
        __sync_add_and_fetch (&sharedSeg_->users[pid_], 1);
    }
    else
    {
        rc = errno;
    }

    return rc;
}

int RobSem::wait_timed( int sec, int us )
{
    int              rc;
    struct timespec  ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += sec;
    ts.tv_nsec += (us * SB_NS_PER_US);
    if (ts.tv_nsec >= SB_NS_PER_SEC) {
        ts.tv_nsec -= SB_NS_PER_SEC;
        ts.tv_sec++;
    }

    do
    {
        rc = sem_timedwait( &sharedSeg_->sem, &ts);
    } while ( rc == -1 && errno == EINTR );

    if ( rc == 0 )
    {   // Add process as user of the semaphore
        __sync_add_and_fetch (&sharedSeg_->users[pid_], 1);
    }
    else
    {
        rc = errno;
    }

    return rc;
}
