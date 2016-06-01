/**********************************************************************
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
********************************************************************/
/**************************************************************************
**************************************************************************/
#ifndef ODBCPING
#define ODBCPING

#ifndef HEART_BEAT_TICK 
#define HEART_BEAT_TICK 120
#endif

#ifndef HEART_BEAT_MICRO_TICK
#define HEART_BEAT_MICRO_TICK 250000
#endif

#ifndef PING_TIMEOUT
#define PING_TIMEOUT 1000
#endif

#ifndef PING_SLEEP
#define PING_SLEEP 1000
#endif

#define WIN32_LEAN_AND_MEAN 
#ifdef _AFXDLL
#include <winsock2.h>
#endif

typedef unsigned long   IPAddr;     // An IP address.
struct ip_option_information {
    unsigned char      Ttl;             // Time To Live
    unsigned char      Tos;             // Type Of Service
    unsigned char      Flags;           // IP header flags
    unsigned char      OptionsSize;     // Size in bytes of options data
    unsigned char FAR *OptionsData;     // Pointer to options data
}; /* ip_option_information */

struct icmp_echo_reply {
    IPAddr                         Address;         // Replying address
    unsigned long                  Status;          // Reply IP_STATUS
    unsigned long                  RoundTripTime;   // RTT in milliseconds
    unsigned short                 DataSize;        // Reply data size in bytes
    unsigned short                 Reserved;        // Reserved for system use
    void FAR                      *Data;            // Pointer to the reply data
    struct ip_option_information   Options;         // Reply options
}; /* icmp_echo_reply */

typedef struct ip_option_information IP_OPTION_INFORMATION,
                                     FAR *PIP_OPTION_INFORMATION;
typedef struct icmp_echo_reply ICMP_ECHO_REPLY,
                               FAR *PICMP_ECHO_REPLY;

extern "C" HANDLE WINAPI IcmpCreateFile( VOID );

extern "C" BOOL WINAPI IcmpCloseHandle( HANDLE  IcmpHandle );

extern "C" DWORD WINAPI IcmpSendEcho(
    HANDLE                   IcmpHandle,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    );

long ping( char* ObjRef );

#endif
