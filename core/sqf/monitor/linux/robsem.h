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

#ifndef ROBSEM_H_
#define ROBSEM_H_

#include <semaphore.h>

class RobSem
{
 public:
    static int   create_sem(unsigned int sharedSegKey,
                            int flags,
                            RobSem *& sem,
                            unsigned int value);

    static int   destroy_sem(RobSem *sem);

    static unsigned int getSegKey( unsigned int applId,
                                   unsigned int segId,
                                   int nid);

    int          post();
    int          post_all( pid_t user );
    int          trywait();
    int          value(int *value);
    int          wait();
    int          wait_timed(int sec, int usec);

    static void log_error ( int event, int severity, char * buf );

 private:
    enum { SHM_PERMISSIONS = 0640 };
    enum { SB_NS_PER_SEC   = 1000000000,
           SB_NS_PER_US    = 1000
         };

    RobSem(){}
    RobSem(int shmid, int flags, void * sharedSeg, int semMax);
    ~RobSem();

    typedef struct { sem_t sem;
                     unsigned short int users[1];
                   } sharedSeg_t;

    int init( int pidMax );

    int shmid_;
    int shmFlags_; 
    int semMax_;
    pid_t pid_;
    sharedSeg_t * sharedSeg_;
};

#endif // ROBSEM_H_
