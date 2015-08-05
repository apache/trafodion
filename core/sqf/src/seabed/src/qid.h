//------------------------------------------------------------------
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
// Queue ids
//

#ifndef __SB_QID_H_
#define __SB_QID_H_

enum {
    QID_NONE              = -1,
    QID_OPEN_THREAD       = 1,
    QID_MPI_HELPER_THREAD = 2,
    QID_FS_COMP           = 3,
    QID_ABANDON_COMP      = 4,
    QID_HOLD              = 5,
    QID_LDONE_COMP        = 6,
    QID_EVENT_MGR         = 7,
    QID_RCV               = 8,
    QID_TPOP_COMP         = 9,
    QID_MESSENGER_THREAD  = 10,
    QID_REPLY_PB          = 11,
    QID_REQ_MAP           = 12, // not a queue
    QID_REP_MAP           = 13, // not a queue
    QID_STREAM_CLOSE      = 14,
    QID_STREAM_DEL        = 15,
    QID_STREAM_DEL_TEMP   = 16,
    QID_TIME_MAP          = 17,
    QID_SM_HELPER_THREAD  = 18,
    QID_FS_TS_COMP        = 19,
    QID_LAST
};

#endif // !__SB_QID_H_
