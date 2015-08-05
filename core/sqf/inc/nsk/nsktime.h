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
/*++

Module name:

NSKtime.h

Abstract:

   This module defines the types and constants that are used by the
     Tandem NSK time suite.

Revision History:

--*/

#ifndef _NSK_time_
#define _NSK_time_

#include "seaquest/sqtypes.h"
#include "nsk/nskmem.h"

#define DLLIMPORT extern "C" __declspec( dllimport )

#define TLE_ENTRIES_INITIAL    0x040
#define TLE_ENTRIES_MAX        0x100

#define MICROSECONDS_IN_A_MILLISECOND    1000


// defines for NSK Time List Element (TLE) template

typedef class _NSK_TLE: public NSK_CB {
public:

    int                tletoval;    // timeout value in 10ms units

    _int64        dueTime;    // the TIME_SINCE_COLDLOAD timestamp
                        // when this tle should pop up

    SHORT    tleid;    // id of this TLE

    SHORT    tleparm1;    // user param 1

    int_32            tleparm2;    // user param 2

    SHORT    tletype;    // type of this tle

    BOOL    tlequeued;    // is this tle been queued?

    NSK_PORT_HANDLE    tleowner_phandle;        // owner's NSK port handle.

} NSK_TLE, *PNSK_TLE;


// function prototype

DllImport
int    NSK64BitGreaterAndEqualTo (_int64 val1, _int64 val2);

DllImport
DWORD NSKTmfTlepoppedSetup ( void (*pfunc)(int) );

DllImport
void NSKTimerPopup (PNSK_TLE ptle);

extern
void TMF_TLEPOPPED_STUB_ (int);

// prototypes from Stan's TIME package

DLLIMPORT int DayOfMonthToJDN (long ordinal,
                               long dayOfWeek,
                               long year,
                               long month,
                               long * jdn
                              );

DLLIMPORT _int64 FastJulianTimestamp (short   type,
                                      short * tuid,
                                      short * error,
                                      short   node
                                     );


#endif
