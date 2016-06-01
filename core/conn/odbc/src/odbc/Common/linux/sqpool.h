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
// Header file for sqpool.
#ifndef SQPOOL_H_
#define SQPOOL_H_

//O_CREAT is defined in fcntl.h, but having trouble including it.
//#include <fcntl.h>
#define O_CREAT               0100


short POOL_DEFINE_( long *pool, int poolsize);
//					 [ short alignment ], [ short priv-only ] );

long POOL_GETSPACE_( long *pool, int blocksize, short *error );

short POOL_PUTSPACE_( long *pool, long *block);


void POOL_CLOSE_();

/*
void * POOL_DEFINE_( void *pool, int poolsize);
//					 [ short alignment ], [ short priv-only ] );

void * POOL_GETSPACE_( void *pool, int blocksize, short *error );

short POOL_PUTSPACE_( void *pool, void *block);
*/

short POOL_CHECK_(void *pool);

void POOL_TEST_( void * membase, int length);

int msg_mon_get_my_info(int  *mon_nid,        // mon node-id
                                  int  *mon_pid,        // mon process-id
                                  char *mon_name,       // mon name
                                  int   mon_name_len,   // mon name-len
                                  int  *mon_ptype,      // mon process-type
                                  int  *mon_zid,        // mon zone-id
                                  int  *os_pid,         // os process-id
                                  long *os_tid);         // os thread-id

#endif //SQPOOL_H_
