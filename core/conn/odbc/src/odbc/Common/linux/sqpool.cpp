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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <new>
#include <semaphore.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
//#include <sys/stat.h>

#include "sqmem.h"
#include "sqpool.h"

#define MAX_SEMNAME_LEN		32
// Globals
sem_t *sqPool_sem;
//const char sqPool_semName[] = "sqPoolSem";
char sqPool_semName[MAX_SEMNAME_LEN];
bool sqPool_firstCall = true;


//-----------------------------------------------------------------------------
// poolinitialize
// Purpose : Initialize pool semaphore.
// This is an internal routine called by the pool management interface APIs
// to open the internal pool management semaphore.
//-----------------------------------------------------------------------------
void poolinitialize()
{
	int nid;
   // Only execute this function the first time it's called.
   if (!sqPool_firstCall)
      return;
   sqPool_firstCall = false;

   msg_mon_get_my_info(&nid,NULL,NULL,0,NULL,NULL,NULL,NULL);

   bzero( sqPool_semName, MAX_SEMNAME_LEN );
   uid_t uid = getuid();
   sprintf( sqPool_semName, "/SQPOOL_SEM_%d_%03d", uid, nid );

   sqPool_sem = sem_open(sqPool_semName, O_CREAT, 0644, 1);
   if (sqPool_sem == SEM_FAILED)
   {
      printf("**sem_open failed to create semaphore %s\n", sqPool_semName);
      sem_unlink(sqPool_semName);
      exit(-1);
   }

   // Test semaphore, it seems to get into a bad state sometimes
   if (sem_trywait(sqPool_sem)==0)
      // Sucess
      sem_post(sqPool_sem);
   else
   {
      // Failed to get semaphore - something is wrong
      sem_close(sqPool_sem);
      sem_unlink(sqPool_semName);
      printf("**Semaphore %s is in a bad state, cleaning up and trying again.\n", sqPool_semName);
      sqPool_sem = sem_open(sqPool_semName, O_CREAT, 0644, 1);
      if (sqPool_sem == SEM_FAILED)
      {
         printf("**sem_open failed to create semaphore %s\n", sqPool_semName);
         sem_unlink(sqPool_semName);
         exit(-1);
      }
      if (sem_trywait(sqPool_sem)==0)
         // Sucess
         sem_post(sqPool_sem);
      else
      {
         // Failed to get semaphore on second attempt - give up.
         sem_close(sqPool_sem);
         sem_unlink(sqPool_semName);
         printf("**Semaphore %s is unrecoverable, terminating.\n", sqPool_semName);
         exit(-1);
      }
   }
} //poolinitialize


//-----------------------------------------------------------------------------
// POOL_CLOSE_
// Purpose : Release pool management semaphore.
// This function has no NSK equivalent.  Not sure whether it's needed.
//-----------------------------------------------------------------------------
void POOL_CLOSE_()
{
   sem_close(sqPool_sem);
   sem_unlink(sqPool_semName);
}


//-----------------------------------------------------------------------------
// POOL_DEFINE_
// Purpose : Define a pool.
// pool     Input.   Starting address of pool.
// poolsize Input.   Size of pool in bytes.
// Returns addres of pool header.
//-----------------------------------------------------------------------------
short POOL_DEFINE_( long *pool, int poolsize)
//					 [ short alignment ], [ short priv-only ] )
{
	short error = NO_ERROR;
   poolinitialize();

   sem_wait(sqPool_sem);
   void *phdr = PoolDefine((void *)pool, poolsize, &error);
   sem_post(sqPool_sem);
   if (error != NO_ERROR)
   {
      printf("**PoolDefine returned error %d.\n", error);
      return error;
   }

   return error;
}


//-----------------------------------------------------------------------------
// POOL_GETSPACE_
// Purpose : Allocate a block from the specified pool.
// pool      Input.  Address of pool header from which block will be allocated.
// blocksize Input.  Size of block to be allocated in bytes.
// error     Output. Error returned by PoolGetSpace.
// Returns address of block allocated.
//-----------------------------------------------------------------------------
long POOL_GETSPACE_( long *pool, int blocksize, short *error )
{
	*error = NO_ERROR;
   poolinitialize();

   sem_wait(sqPool_sem);
   void *block = (void *) PoolGetSpace((PPOOL_HEADER) pool, blocksize, error);
   sem_post(sqPool_sem);
   return (long)block;
}


//-----------------------------------------------------------------------------
// POOL_PUTSPACE_
// Purpose : Return a block to the pool.
// pool  Input.   Address of pool header.
// block Input.   Address of block being returned to pool.
// Returns error from PoolPutSpace.
//-----------------------------------------------------------------------------
short POOL_PUTSPACE_( long *pool, long *block)
{
	short error = NO_ERROR;
   poolinitialize();

   sem_wait(sqPool_sem);
   error = PoolPutSpace((PPOOL_HEADER) pool, (void *)block);
   sem_post(sqPool_sem);
   return error;
}


//-----------------------------------------------------------------------------
// POOL_CHECK_
// Pupose : Validate pool.
// Not implemented yet.
//-----------------------------------------------------------------------------
short POOL_CHECK_(void *pool)
{
   poolinitialize();

   sem_wait(sqPool_sem);
   short error = PoolCheck((PPOOL_HEADER) pool);
   sem_post(sqPool_sem);
   return error;
}


//-----------------------------------------------------------------------------
// POOL_TEST_
// Purpose : Test pool management routines.
//-----------------------------------------------------------------------------
void POOL_TEST_(void * membase, int length)
{
   poolinitialize();

   sem_wait(sqPool_sem);
   TestPool(membase, length);
   sem_post(sqPool_sem);
}

