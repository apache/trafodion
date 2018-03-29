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

#ifndef NOTICE_H_
#define NOTICE_H_
#ifndef NAMESERVER_PROCESS

#include <queue>
#include "mlio.h"

typedef queue<int> NidQueue_t;  // (nid)

class CProcess;

class CNotice
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    int  Nid;                  // node id of requesting process
    int  Pid;                  // process id of requesting process
    Verifier_t verifier_;      // verifier of requesting process
    string name_;
    _TM_Txid_External TransID; // Associated Transaction ID, zero is not associated.
       
    CNotice( int nid
           , int pid
           , Verifier_t verifier
           , const char *name
           , _TM_Txid_External trans_id
           , CProcess *process );
    ~CNotice( void );

    void Cancel( void );
    void DeLink( CNotice **head, CNotice **tail );
    CNotice *GetNext( void );
    CNotice *GetNotice( int nid
                      , int pid
                      , Verifier_t verifier
                      , _TM_Txid_External trans_id );
    CNotice *Link( CNotice *entry );
    void NotifyAll( void );
    void NotifyRemote( void );
    bool isCanceled ( void ) { return canceled_; }

protected:
private:
    void Notify( SQ_LocalIOToClient::bcastPids_t *bcastPids );
    void NotifyNid( NidQueue_t *nidQueue );
    bool canceled_;             // true if notice request has been canceled
    CProcess *Process;         // process that is being monitored
    CNotice *Next;
    CNotice *Prev;
};

#endif
#endif /*NOTICE_H_*/
