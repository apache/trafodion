// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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

#include "StaticLocking.h"

// This stores the mutex 
static MUTEX_TYPE* mutex_buf;

void SecurityInitialize(void) throw (SecurityException)
{
	setupMutex();
}

static void setupMutex(void)
{
   mutex_buf = (MUTEX_TYPE*) malloc(sizeof(MUTEX_TYPE));

   if(!mutex_buf)
      throw SecurityException(MEMORY_ALLOC_FAILED, NULL);
   MUTEX_SETUP(*mutex_buf);

}

void getMutex(void)
{
   if (!mutex_buf)
      throw SecurityException(MUTEX_NOT_EXIST, NULL);
   MUTEX_LOCK(mutex_buf);
}      

void releaseMutex(void)
{
   MUTEX_UNLOCK(mutex_buf);    
}

void cleanupMutex(void)
{
#ifdef _WINDOWS
   if (MUTEX_CLEANUP(*mutex_buf) == 0) // ReleaseMutex returns 0 if failed
#else
   if (MUTEX_CLEANUP(mutex_buf) != 0)  // pthread_mutex_destroy returns 0 if successful
#endif
		throw SecurityException(MUTEX_RELEASE_FAILED, NULL);
   free(mutex_buf);
   mutex_buf = NULL;
}
