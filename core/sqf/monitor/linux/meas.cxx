// ///////////////////////////////////////////////////////////////////////////////
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

#include <iostream>

using namespace std;

#include <memory.h>
#include <stdio.h>
#include <sys/time.h>

#include "montrace.h"
#include "meas.h"

CMeas::CMeas( void )
      :traceTimeElapsedUsec_(0)
{
    memset( &sock_, 0, sizeof( sock_ ) );
    memset( &sockAllGather_, 0, sizeof( sockAllGather_ ) );
    memset( &sockNs_, 0, sizeof( sockNs_ ) );
    memset( &sockPtp_, 0, sizeof( sockPtp_ ) );
    struct timeval traceTime;
    gettimeofday( &traceTime, NULL );
    traceTimeSecCurr_ = traceTime.tv_sec;
    traceTimeUsecCurr_ = traceTime.tv_usec;
}

CMeas::~CMeas (void)
{
}

void CMeas::addSockAllGatherRcvdBytes( int nr )
{
    measAddRcvdBytes( &sockAllGather_, nr );
}

void CMeas::addSockAllGatherSentBytes( int ns )
{
    measAddSentBytes( &sockAllGather_, ns );
}

void CMeas::addSockNsRcvdBytes( int nr )
{
    measAddRcvdBytes( &sockNs_, nr );
}

void CMeas::addSockNsSentBytes( int ns )
{
    measAddSentBytes( &sockNs_, ns );
}

void CMeas::addSockPtpRcvdBytes( int nr )
{
    measAddRcvdBytes( &sockPtp_, nr );
}

void CMeas::addSockPtpSentBytes( int ns )
{
    measAddSentBytes( &sockPtp_, ns );
}

void CMeas::addSockRcvdBytes( int nr )
{
    measAddRcvdBytes( &sock_, nr );
}

void CMeas::addSockSentBytes( int ns )
{
    measAddSentBytes( &sock_, ns );
}

void CMeas::measAddRcvdBytes( Meas *meas, int nr )
{
    meas->rcvdBytes_ += nr;
    meas->rcvdBytesCurr_ += nr;
}

void CMeas::measAddSentBytes( Meas *meas, int ns )
{
    meas->sentBytes_ += ns;
    meas->sentBytesCurr_ += ns;
    meas->sent_++;
    meas->sentCurr_++;
    if (trace_settings & TRACE_MEAS)
        traceMeas();
}

void CMeas::measClear( Meas *meas )
{
    meas->sentBytesCurr_ = 0;
    meas->rcvdBytesCurr_ = 0;
    meas->sentCurr_ = 0;
}

void CMeas::traceMeas( void )
{
    enum { MEAS_TIME = 10000000 };

    struct timeval traceTime;
    gettimeofday( &traceTime, NULL );
    long elapsed = traceTime.tv_sec * 1000000 + traceTime.tv_usec -
      traceTimeSecCurr_ * 1000000 - traceTimeUsecCurr_;
    if ( elapsed >= MEAS_TIME )
    {
        traceTimeElapsedUsec_ += elapsed;
        int elapsedSec = traceTimeElapsedUsec_ / 1000000;
        int elapsedUsec = traceTimeElapsedUsec_ - elapsedSec * 1000000;
        traceMeasCumul( elapsedSec, elapsedUsec, &sockAllGather_, "sockAG" );
        traceMeasCumul( elapsedSec, elapsedUsec, &sock_, "sock" );
        traceMeasCumul( elapsedSec, elapsedUsec, &sockNs_, "sockNs" );
        traceMeasCumul( elapsedSec, elapsedUsec, &sockPtp_, "sockPtp" );
        traceMeasCurr( elapsed, &sockAllGather_, "sockAG" );
        traceMeasCurr( elapsed, &sock_, "sock" );
        traceMeasCurr( elapsed, &sockNs_, "sockNs" );
        traceMeasCurr( elapsed, &sockPtp_, "sockPtp" );
        traceTimeSecCurr_ = traceTime.tv_sec;
        traceTimeUsecCurr_ = traceTime.tv_usec;
        measClear( &sockAllGather_ );
        measClear( &sock_ );
        measClear( &sockNs_ );
        measClear( &sockPtp_ );
    }
}

void CMeas::traceMeasFmt( char *buf, long count )
{
    if ( count < 1000 )
        sprintf( buf, "%ld", count );
    else if ( count < 1000000 )
    {
        long kCount = count / 1000;
        long kCount10 = count / 100;
        sprintf( buf, "%ld.%01ldK", kCount, kCount10 - kCount * 10);
    }
    else if ( count < 1000000000 )
    {
        long mCount = count / 1000000;
        long mCount10 = count / 100000;
        sprintf( buf, "%ld.%01ldM", mCount, mCount10 - mCount * 10);
    }
    else
    {
        long mCount = count / 1000000000;
        long mCount10 = count / 100000000;
        sprintf( buf, "%ld.%01ldG", mCount, mCount10 - mCount * 10);
    }
}

void CMeas::traceMeasFmtRate( char *buf, long elapsed, long count )
{
    long bps = count * 1000000 / elapsed;
    if ( bps < 1000 )
        sprintf( buf, "%ldB/s", bps );
    else if ( bps < 1000000 )
    {
        long kCount = bps / 1000;
        long kCount10 = bps / 100;
        sprintf( buf, "%ld.%01ldKB/s", kCount, kCount10 - kCount * 10);
    }
    else if ( bps < 1000000000 )
    {
        long mCount = bps / 1000000;
        long mCount10 = bps / 100000;
        sprintf( buf, "%ld.%01ldMB/s", mCount, mCount10 - mCount * 10);
    }
    else
    {
        long mCount = bps / 1000000000;
        long mCount10 = bps / 100000000;
        sprintf( buf, "%ld.%01ldGB/s", mCount, mCount10 - mCount * 10);
    }
}

void CMeas::traceMeasCumul( int elapsedSec, int elapsedUsec, Meas *meas, const char *mtype )
{
    const char method_name[] = "CMeas::traceMeasCumul";
    char tSent[20];
    char tRcvd[20];
    traceMeasFmt( tSent, meas->sentBytes_ );
    traceMeasFmt( tRcvd, meas->rcvdBytes_ );
    trace_printf( "%s@%d - elapsed=%d.%06d, CUMUL-%s sent=%ld(%s), rcvd=%ld(%s)\n"
                , method_name, __LINE__
                , elapsedSec
                , elapsedUsec
                , mtype
                , meas->sentBytes_
                , tSent
                , meas->rcvdBytes_
                , tRcvd
                );
}

void CMeas::traceMeasCurr( long elapsed, Meas *meas, const char *mtype )
{
    const char method_name[] = "CMeas::traceMeasCurr";
    char tSentCurr[20];
    char tRcvdCurr[20];
    char tSentRate[20];
    char tRcvdRate[20];
    traceMeasFmt( tSentCurr, meas->sentBytesCurr_ );
    traceMeasFmt( tRcvdCurr, meas->rcvdBytesCurr_ );
    traceMeasFmtRate( tSentRate, elapsed, meas->sentBytesCurr_ );
    traceMeasFmtRate( tRcvdRate, elapsed, meas->rcvdBytesCurr_ );
    int elapsedCurrSec = elapsed / 1000000;
    int elapsedCurrUsec = elapsed - elapsedCurrSec * 1000000;
    trace_printf( "%s@%d - elapsed=%d.%06d, CUR-%s sent=%ld(%s-%s), rcvd=%ld(%s-%s)\n"
                , method_name, __LINE__
                , elapsedCurrSec
                , elapsedCurrUsec
                , mtype
                , meas->sentBytesCurr_
                , tSentCurr
                , tSentRate
                , meas->rcvdBytesCurr_
                , tRcvdCurr
                , tRcvdRate
                );
}
