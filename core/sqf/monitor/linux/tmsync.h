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

#ifndef TMSYNC_H_
#define TMSYNC_H_

#include <semaphore.h>

#include "lock.h"
#include "msgdef.h"
#include "cluster.h"
#include "process.h"

class CTmSyncReq
{
 private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    int    Nid;
    int    Tag;
    int    Handle;
    int    Length;
    char  *Data;
    bool   Unsolicited; // request is from another node
    bool   Replicated;  // this request has been replicated across nodes
    bool   Completed;   // unsolicited request has accepted by TM

    CTmSyncReq( int nid, int handle, char *data, int length, int tag, bool unsolicited );
    ~CTmSyncReq( void );
    void DeLink( CTmSyncReq **head, CTmSyncReq **tail );
    CTmSyncReq *GetNext( void );
    CTmSyncReq *Link( CTmSyncReq *entry );


protected:
private:
    CTmSyncReq *Next;
    CTmSyncReq *Prev;
};

class CTmSync_Container : public CCluster
{
public:
    int TmSyncReplies;     // # of unsolicited replies received for current block
    int ReqsInBlock;       // # of requests currently being processed
    int TmSyncReplyCode;   // last unsuccessful unsolicited reply code for block
    int PendingSlaveTmSyncCount; // number of unsolicited messages sent to TMs for slave sync

    CTmSync_Container( void );
    virtual ~CTmSync_Container( void );

    void CancelUnsolicitedMessages( CProcess *tmProcess );
    int  GetHandle( void );         // return the next handle id to use
    inline bool GetPendingSlaveTmSync( void ) { return( PendingSlaveTmSync ); }
    int  GetTotalSlaveTmSyncCount( void )
    {
        CAutoLock lock(TmSyncReplyLock.getLocker());
        return( TotalSlaveTmSyncCount );
    }
    inline bool IsAbortPendingTmSync( void ) { return( AbortPendingTmSync ); }
    void ProcessTmSyncReply( struct message_def * msg );
    int  CoordinateTmDataBlock( struct sync_def *block );
    CTmSyncReq *FindTmSyncReq( int handle );
    CTmSyncReq *Q_TmSync( int nid, int handle, char *data, int len, int tag, bool unsolicited );
    void ReQueue_TmSync( bool master ); // return packed TmSyncReqQ entries to the queue
    inline void SetAbortPendingTmSync( void ) { AbortPendingTmSync = true; }
    void TmSync( void );            // Coordinate TmSync across nodes using Im'Alive()
    void TmSyncAbortPending( void );   // Abort all local unprocessed TmSyncs
    bool TmSyncPending( void );

protected:
    void CommitTmDataBlock( int return_code );
    void IncrTmSyncReplies( void )
    {
        TmSyncReplyLock.lock();
        TmSyncReplies++;
        TmSyncReplyLock.unlock();
    }
    int DecrTmSyncReplies( void )
    {
        TmSyncReplyLock.lock();
        TmSyncReplies--;
        TmSyncReplyLock.unlock();
        return( TmSyncReplies );
    }
    int  GetPendingSlaveTmSyncCount( void )
    {
        CAutoLock lock(TmSyncReplyLock.getLocker());
        return( PendingSlaveTmSyncCount );
    }
    int  GetReqsInBlock( void )
    {
        return( ReqsInBlock );
    }
    int  GetTmSyncReplies( void )
    {
        CAutoLock lock(TmSyncReplyLock.getLocker());
        return( TmSyncReplies );
    }
    void UpdateTmSyncState( int return_code );
    void UnsolicitedComplete( struct message_def *msg );
    void UnsolicitedCompleteDone( void );
    void UnsolicitedCompleteWait( void );

private:
    sem_t       UnsolicitedWaitSem; // Unsolicited message completion semaphore
    CLock       TmSyncReplyLock;    // number of unsolicited messages sent to TMs for slave sync
    CTmSyncReq *Head;               // Pointer to 1st TmSync request needing resolution
    CTmSyncReq *Tail;               // Pointer to last TmSync request needing resolution
    int         HandleSeq;
    bool        PendingSlaveTmSync; // true when we need to send unsolicited messages to TM for slave sync
    int         TotalSlaveTmSyncCount; // number of unsolicited messages to be sent to TMs for slave sync
                                       // depending on the number of non-master TM in physical node
    bool        AbortPendingTmSync; // set true when node down or activating spare node is triggered

    void EndTmSync( MSGTYPE type );
    void EndPendingTmSync( struct sync_def *sync );
    void ExchangeTmSyncState( void );
    struct sync_def *PackSyncData( void );        // return sync_def for current TmSyncReqQ entries
    void SendUnsolicitedMessages( void );
    void UnPackSyncData( struct sync_def *sync ); // process sync_def received from another node
};

#endif /*TMSYNC_H_*/
