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

#include <assert.h>
#include <pthread.h>
#include "seaquest/sqtypes.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/unistd.h>

typedef  int  BOOL ;

#define usPerSecond      1000000
#define nsPerMicrosecond 1000
#define TRUE             1

BOOL QueryPerfCounter( LargeInt * pPerfCount )
{
   struct timespec currTime ;
   clock_gettime( CLOCK_MONOTONIC, &currTime ) ;

   pPerfCount->QuadPart = (long long ) (currTime.tv_sec) * usPerSecond
                          + currTime.tv_nsec / nsPerMicrosecond ;
   return TRUE ;

}

int MessageBoxA(void*, char const* text, char const* process, int type) {
    fprintf(stderr, "MESSAGE-BOX: text=%s, process=%s, type=%d\n",
            text, process, (int)type);
    return 0;
}

void DebugBreak() {
  abort();
}


typedef  unsigned int ( * PTHRD_START_ROUTINE )( void * pThrdParameter ) ;

HANDLE CreateNewThread( PTHRD_START_ROUTINE  pStartAddress,
                        void * pParameter )
{
    pthread_attr_t      thrdAttrs   ;
    pthread_attr_init( &thrdAttrs ) ;

    pthread_t *pNewTid = new pthread_t ;

    typedef void *(*Func) (void *) ;

    int status = pthread_create( pNewTid, &thrdAttrs,
                                (Func) pStartAddress,
                                 pParameter ) ;
    if ( status != 0 ) {
        // Create failed -- should we handle errors ?
    }
    return (HANDLE) pNewTid ; // cast as needed
}

unsigned int GetCurrThreadId( void )
{
    pid_t  threadID = syscall( __NR_gettid ) ;
    return threadID ;
}



void InitializeCriticalSection( CRITICAL_SECTION * pCriticalSection )
{
    assert( sizeof( pthread_mutex_t ) <= sizeof( CRITICAL_SECTION ) );

    pthread_mutexattr_t mutexAttr;
    int status = pthread_mutexattr_init( &mutexAttr );
    assert(status == 0);

    // On windows, Critical Sections are recursive, so just in case ...
    status = pthread_mutexattr_settype( &mutexAttr, PTHREAD_MUTEX_RECURSIVE );
    assert(status == 0);

    pthread_mutex_t *pMutex = (pthread_mutex_t *) pCriticalSection; // cast as mutex*
    status = pthread_mutex_init( pMutex, &mutexAttr );
    assert(status == 0);
}

void EnterCriticalSection( CRITICAL_SECTION * pCriticalSection )
{
    pthread_mutex_t *pMutex = (pthread_mutex_t *) pCriticalSection; // cast as mutex*
    int rtnv = pthread_mutex_lock( pMutex );
    assert(rtnv == 0);
}

BOOL TryEnterCriticalSection( CRITICAL_SECTION * pCriticalSection )
{
    pthread_mutex_t *pMutex = (pthread_mutex_t *) pCriticalSection; // cast as mutex*
    int status = pthread_mutex_trylock( pMutex );
    // On Windows, return TRUE if entered or Crit. Sec. already owned by thread
    // On Unix, status is 0 if lock is acquired.
    return status ? FALSE : TRUE ;
}

void LeaveCriticalSection( CRITICAL_SECTION * pCriticalSection )
{
    pthread_mutex_t *pMutex = (pthread_mutex_t *) pCriticalSection; // cast as mutex*
    int status = pthread_mutex_unlock( (pthread_mutex_t *) pMutex );
    assert(status == 0);
}

void DeleteCriticalSection( CRITICAL_SECTION * pCriticalSection )
{
    pthread_mutex_destroy( (pthread_mutex_t *) pCriticalSection );
}

void Sleep( DWORD milliSecs )
{
    struct timespec  ntime ;
    ntime.tv_sec  =   milliSecs / 1000 ;
    ntime.tv_nsec = ( milliSecs % 1000 ) * 1000000 ;
    nanosleep( &ntime, NULL ) ;
}

DWORD SleepEx( DWORD milliSecs , BOOL alertableFlag )
{
      Sleep( milliSecs ) ;
      return 0 ;
}

BOOL GetSystemTimeAdjustment( DWORD * pTimeAdjustment
                            , DWORD * pTimeIncrement
                            , BOOL  * pTimeAdjustmentDisabled )
{
    *pTimeIncrement  = 0  ;
    *pTimeAdjustment = 10 ;
    *pTimeAdjustmentDisabled = TRUE ;
    return TRUE ;
}

void GetSystemTimeAsFileTime( FILETIME * pSystemTimeAsFileTime )
{
    struct timeval currTime;
    gettimeofday( &currTime, NULL );
    //
    // currTime has 2 components: tv_sec  (in seconds)
    //                            tv_usec (in microseconds)
    //
    // Our output has 2 DWORDS: which together have the time
    // in 100 nanosecond increments.
    //
    _int64 currSecs = currTime.tv_sec ;

#define Ns100_PER_MicroSec  10
#define Ns100_PER_SEC       10000000LL
#define ADJ_SEC 11644473600LL // Diff between mic epoch 1/1/1601 and 1/1/1970

    currSecs += ADJ_SEC ;

    _int64 num100ns = (_int64) currSecs * Ns100_PER_SEC +
                      (_int64) currTime.tv_usec * Ns100_PER_MicroSec ;

    pSystemTimeAsFileTime->dwHighDateTime = num100ns >> 32;
    pSystemTimeAsFileTime->dwLowDateTime  = (DWORD) num100ns;
}

static bool       globalTimeZoneSet                = FALSE ;
static int        globalTimeZoneOffsetInMinutes    = 0 ;
static _int64     globalTimeZoneOffsetIn100nsIncrs = 0 ;
static struct tm  globalTimeZoneTime ;

static void setGlobalTimeZoneInfo()
{
   struct timeval tmpTimeval ;

   globalTimeZoneSet = true;
   gettimeofday( &tmpTimeval , NULL ) ;
   localtime_r( &tmpTimeval.tv_sec, &globalTimeZoneTime ) ;
   globalTimeZoneOffsetInMinutes    = globalTimeZoneTime.tm_gmtoff/60 ; // On Linux, convert to minutes
   globalTimeZoneOffsetIn100nsIncrs = (_int64) globalTimeZoneTime.tm_gmtoff * Ns100_PER_SEC ;
}

DWORD GetTimeZoneInformation( TIME_ZONE_INFO * pTimeZoneInfo )
{
    if ( ! globalTimeZoneSet ) setGlobalTimeZoneInfo() ;

    pTimeZoneInfo->Bias = globalTimeZoneOffsetInMinutes ;
    return TIME_ZONE_ID_STANDARD ;
}


