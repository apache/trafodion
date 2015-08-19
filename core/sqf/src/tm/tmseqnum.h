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

#ifndef TMSEQNUM_H_
#define TMSEQNUM_H_

#include <stdlib.h>
#include "dtm/tm.h"
#include "dtm/tmtransid.h"
#include "dtm/tmtransaction.h"
#include "tmlibglobal.h"
#include "tmmap.h"

// --------------------------------------------------------------------------
// CtmSeqNum
// -- Provides TM Library with sequence number blocks 
// --------------------------------------------------------------------------
class CtmSeqNum
{
private:
   #define MAX_SEQNUM (UINT_MAX-1)
   bool iv_firstTime;
   uint32 iv_nextSeqNum;
   uint32 iv_seqNumBlock_start;
   uint32 iv_seqNumBlock_count;
   TM_Mutex iv_mutex;
public: 
    CtmSeqNum() {
       iv_firstTime = true;
       iv_nextSeqNum = -1;
       iv_seqNumBlock_start = -1;
       iv_seqNumBlock_count = -1;
    }

    ~CtmSeqNum();
    void lock() {
       iv_mutex.lock();
    }
    void unlock() {
       iv_mutex.unlock();
    }

//----------------------------------------------------------------------------
// CtmSeqNum::nextSeqNum
// Purpose: Get the next block of available transaction sequence numbers from
// the tm.
//----------------------------------------------------------------------------
    uint32 nextSeqNum() {
       uint32 lv_returnSeqNum = 0;
       TMlibTrace(("TMLIB_TRACE : CtmSeqNum::nextSeqNum ENTRY. Current seqnum %u, start at %u, block size %u.\n",
                  iv_nextSeqNum, iv_seqNumBlock_start, iv_seqNumBlock_count), 2);
       lock();
       //  Get next sequence number block or rollover if at the end of the block or MAX_SEQNUM
       if (iv_firstTime ||
           (iv_nextSeqNum >= iv_seqNumBlock_start + iv_seqNumBlock_count) ||
           (iv_seqNumBlock_start >= MAX_SEQNUM) || (iv_nextSeqNum >= MAX_SEQNUM)) {
          iv_firstTime = false;
          // End of sequence number block or maximum sequence number - get new block
          short lv_err = DTM_GETNEXTSEQNUMBLOCK(iv_seqNumBlock_start, iv_seqNumBlock_count);
          if (lv_err) {
             printf("Sequence number block fetch failed with error %d.\n", lv_err);
             abort();
          }
          else
             iv_nextSeqNum = iv_seqNumBlock_start;
       }
       lv_returnSeqNum = iv_nextSeqNum++;
       unlock();
       TMlibTrace(("TMLIB_TRACE : CtmSeqNum::nextSeqNum EXIT. Returning next seqnum %u, start at %u, block size %u.\n",
          lv_returnSeqNum, iv_seqNumBlock_start, iv_seqNumBlock_count), 2);
       return lv_returnSeqNum;
    }
};


#endif
