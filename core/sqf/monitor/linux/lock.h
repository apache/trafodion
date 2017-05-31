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

#ifndef LOCK_H_
#define LOCK_H_

#include <pthread.h>
#include <errno.h>
#include <assert.h>

class CAutoLock
{
public:
    CAutoLock(pthread_mutex_t *mtx)
        : mtx_(mtx)
    {
        assert( pthread_mutex_lock( mtx_ ) == 0 );
    }

    ~CAutoLock()
    {
        assert( pthread_mutex_unlock( mtx_ ) == 0 );
    }

private:
    pthread_mutex_t *mtx_;
};

class CLock 
{
public:
    CLock( void );
    ~CLock( void );
    
    void    lock( void );
    void    unlock( void );
    bool    tryLock( void );

    // Note: These "wake" and "wait" methods should be called with
    // the associated mutex locked.
    void    wakeOne( void );
    void    wakeAll( void );
    
    void    wait( void );
    int     timedWait( struct timespec *ts );

    inline pthread_mutex_t* getLocker() { return &mutex_; }
    
private:
    bool                bellRang_;  // flag set when no visitors are waiting
    int                 visitors_;  // number waiting

    // Mutex to control access to the queue.
    pthread_mutex_t     mutex_;
    pthread_cond_t      workBell_;
    
};

#endif /*LOCK_H_*/


