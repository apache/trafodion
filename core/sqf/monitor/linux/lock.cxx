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
#include <assert.h>
#include <signal.h>
#include <pthread.h>

#ifdef USE_MON_LOGGING
    #include "montrace.h"
    #include "monlogging.h"
#else
    #include "montrace.h"
#endif
#include "lock.h"

CLock::CLock( void ) 
      : bellRang_( false )
      , visitors_( 0 )
{
    const char method_name[] = "CLock::CLock";
    TRACE_ENTRY;

    pthread_mutexattr_t mutexAttr;
    
    // Setup the thread related condition variables and mutex.
    // The mutex is set to enable error checking and any deadlock detection.
    
    int rc = pthread_mutexattr_init( &mutexAttr );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't initialize mutex attributes! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_INIT_1, SQ_LOG_CRIT, la_buf);
        abort();
    }
#endif
    
    rc = pthread_mutexattr_settype( &mutexAttr, PTHREAD_MUTEX_RECURSIVE );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't set mutex attributes! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_INIT_2, SQ_LOG_CRIT, la_buf);
        abort();
    }
#endif
    
    rc = pthread_mutex_init( &mutex_, &mutexAttr );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't initialize mutex! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_INIT_3, SQ_LOG_CRIT, la_buf);
        abort();
    }
#endif
    
    rc = pthread_cond_init( &workBell_, NULL );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't initialize condition variable! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_INIT_4, SQ_LOG_CRIT, la_buf);
        abort();
    }
#endif
    
    TRACE_EXIT;
}

// This function must be called without the mutex held.
CLock::~CLock( void ) 
{
    const char method_name[] = "CLock::~CLock";
    TRACE_ENTRY;

    int rc;
    // Clear all the variables.
    rc = pthread_mutex_destroy( &mutex_ );
#ifdef xx_USE_MON_LOGGING // disable this since it always returns EINVAL
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't destroy mutex! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_DEST_1, SQ_LOG_ERR, la_buf);
    }
#endif
    
    
    rc = pthread_cond_destroy ( &workBell_ );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't destroy condition variable! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_DEST_2, SQ_LOG_ERR, la_buf);
    }
#endif
    
    TRACE_EXIT;
}

void
CLock::lock( void ) 
{
    const char method_name[] = "CLock::lock";
    TRACE_ENTRY;

    // lock the mutex. Deadlock detection is turned on.
    int rc  = pthread_mutex_lock( &mutex_ );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't lock mutex! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_LOCK_1, SQ_LOG_ERR, la_buf);
    }
#endif

    TRACE_EXIT;
}


void
CLock::unlock( void ) 
{
    const char method_name[] = "CLock::unlock";
    TRACE_ENTRY;

    // unlock the mutex
    int rc = pthread_mutex_unlock( &mutex_ );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't unlock mutex! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_UNLOCK_2, SQ_LOG_ERR, la_buf);
    }
#endif

    TRACE_EXIT;
}

void
CLock::wakeOne( void ) 
{
    const char method_name[] = "CLock::wakeOne";
    TRACE_ENTRY;

    int rc;
    
    if ( visitors_ == 0 )
    {
        bellRang_ = true;
    }
    rc = pthread_cond_signal( &workBell_ );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't destroy condition variable! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_WAKEONE_1, SQ_LOG_ERR, la_buf);
    }
#endif

    TRACE_EXIT;
}

void
CLock::wakeAll( void ) 
{
    const char method_name[] = "CLock::wakeAll";
    TRACE_ENTRY;

    int rc;

    if ( visitors_ == 0 )
    {
        bellRang_ = true;
    }
    rc = pthread_cond_broadcast( &workBell_ );
    assert( rc == 0 );
#ifdef USE_MON_LOGGING
    if (rc)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't broadcast signal! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_WAKEALL_1, SQ_LOG_ERR, la_buf);
    }
#endif

    TRACE_EXIT;
}

int
CLock::timedWait( struct timespec *ts ) 
{
    const char method_name[] = "CLock::timedWait";
    TRACE_ENTRY;

    int rc = 0;

    if ( bellRang_ )
    {
        bellRang_ = false;
    }
    else
    {
        visitors_++;
        rc = pthread_cond_timedwait( &workBell_, &mutex_, ts );
        assert( rc == 0 || rc == ETIMEDOUT );
        visitors_--;
#ifdef USE_MON_LOGGING
        if (rc && rc != ETIMEDOUT)
        {
            int err = errno;
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error= Can't wait on signal! - errno=%d (%s)\n", method_name, err, strerror(errno));
            mon_log_write(MON_LOCK_TMWAIT_1, SQ_LOG_ERR, la_buf);
        }
#endif
    }

    TRACE_EXIT;
    return( rc );
}

void
CLock::wait( void ) 
{
    const char method_name[] = "CLock::wait";
    TRACE_ENTRY;

    int rc;

    if ( bellRang_ )
    {
        bellRang_ = false;
    }
    else
    {
        visitors_++;
        rc = pthread_cond_wait( &workBell_, &mutex_ );
        assert( rc == 0 );
        visitors_--;
#ifdef USE_MON_LOGGING
        if (rc)
        {
            int err = errno;
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error= Can't wait on signal! - errno=%d (%s)\n", method_name, err, strerror(errno));
            mon_log_write(MON_LOCK_WAIT_1, SQ_LOG_ERR, la_buf);
        }
#endif
    }

    TRACE_EXIT;
}

bool
CLock::tryLock( void ) 
{
    const char method_name[] = "CLock::tryLock";
    TRACE_ENTRY;

    int rc = pthread_mutex_trylock( &mutex_ );
    
    assert( rc == 0 || rc == EBUSY );
#ifdef USE_MON_LOGGING
    if (rc && rc != EBUSY)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't try locking mutex! - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LOCK_TRY_1, SQ_LOG_ERR, la_buf);
    }
#endif
    
    if ( rc == 0 )
        unlock();
        
    TRACE_EXIT;
    return ((rc == 0) ? true : false);
}
