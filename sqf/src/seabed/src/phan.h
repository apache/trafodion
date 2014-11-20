//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef __SB_PHAN_H_
#define __SB_PHAN_H_

#include "seabed/int/types.h" // SB_Int64_Type

enum { SB_PHANDLE_NAME_SIZE = 32 };
typedef struct SB_Phandle {
                          //       +-------------------+-------------------+
    unsigned iv_flags:4;  //  0:00 |  type  |   vers   |  len   |   nl     |
    unsigned iv_type:4;   //       +--------+----------+--------+----------+
    char iv_vers;         //  1:04 |                  name                 |
    char iv_len;          //       +                                       +
    char iv_name_len;     //  2:08 |                                       |
    char ia_name[         //       +                                       +
    SB_PHANDLE_NAME_SIZE  //  3:12 |                                       |
    ];                    //       +                                       +
                          //  4:16 |                                       |
                          //       +                                       +
                          //  5:20 |                                       |
                          //       +                                       +
                          //  6:24 |                                       |
                          //       +                                       +
                          //  7:28 |                                       |
                          //       +                                       +
                          //  8:32 |                                       |
                          //       +---------------------------------------+
    int iv_nid;           //  9:36 |                  nid                  |
                          //       +---------------------------------------+
    int iv_pid;           // 10:40 |                  pid                  |
                          //       +---------------------------------------+
    int iv_verifier;      // 11:44 |                verifier               |
                          //       +---------------------------------------+
                          // 12:48 |                  rsv                  |
    int ia_rsv2[3];       //       +                                       +
                          // 13:52 |                                       |
                          //       +                                       +
                          // 14:56 |                                       |
                          //       +---------------------------------------+
    int iv_oid;           // 15:60 |                  oid                  |
                          //       +---------------------------------------+
} SB_Phandle;
enum { SB_PHANDLE_LL_SIZE = sizeof(SB_Phandle_Type)/sizeof(SB_Int64_Type) };
enum { SB_PHANDLE_VERS    = 1 };

enum { PH_INVALID = 0 }; // Invalid phandle
enum { PH_UNAMED = 1 };  // Unnamed post D00 process
enum { PH_NAMED = 2 };   // Named post D00 process
enum { PH_OUNAMED = 3 }; // Unnamed pre D00 process
enum { PH_ONAMED = 4 };  // Named pre D00 process
enum { PH_LDEV = 5 };    // Logical device process
enum { PH_NULL = 15 };   // "null" Phandles are all ones

#endif // !__SB_PHAN_H_
