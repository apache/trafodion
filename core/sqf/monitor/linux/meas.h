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

#ifndef MEAS_H_
#define MEAS_H_

class CMeas 
{
public:
    CMeas( void );
    virtual ~CMeas( void );

    void addSockAllGatherRcvdBytes( int nr );
    void addSockAllGatherSentBytes( int ns );
    void addSockRcvdBytes( int nr );
    void addSockSentBytes( int ns );

    void addSockNsRcvdBytes( int nr );
    void addSockNsSentBytes( int ns );

    void addSockPtpRcvdBytes( int nr );
    void addSockPtpSentBytes( int ns );

private:
    typedef struct M
    {
        long rcvdBytes_;
        long rcvdBytesCurr_;
        long sentBytes_;
        long sentBytesCurr_;
        long sent_;
        long sentCurr_;
    } Meas;

    void measAddSentBytes( Meas *meas, int ns );
    void measAddRcvdBytes( Meas *meas, int nr );
    void measClear( Meas *meas );
    void traceMeas( void );
    void traceMeasCumul( int elapsedSec, int elapsedUsec, Meas *meas, const char *mtype );
    void traceMeasCurr( long elapsed, Meas *meas, const char *mtype );
    void traceMeasFmt( char *buf, long count );
    void traceMeasFmtRate( char *buf, long elapsed, long count );

    Meas sock_;
    Meas sockAllGather_;
    Meas sockNs_;
    Meas sockPtp_;
    int  traceTimeElapsedUsec_;
    long traceTimeSecCurr_;
    int  traceTimeUsecCurr_;
};

#endif // MEAS_H_
