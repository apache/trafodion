//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

#ifndef __T284_H_
#define __T284_H_

// request types
typedef enum {
    GID_REQ_PING = 1,
    GID_REQ_ID   = 2,
    GID_REQ_LAST = 3
} GID_REQ_TYPE;

// reply types
typedef enum {
    GID_REP_PING = 100,
    GID_REP_ID   = 101,
    GID_REP_LAST = 102
} GID_REP_TYPE;

// error types
typedef enum {
    GID_ERR_OK    = 0,
    GID_ERR_SHORT = 1,
    GID_ERR_LAST  = 2
} GID_Err_Type;

// reply common
typedef struct GID_Rep_Com_Type {
    GID_Err_Type     error;
} GID_Rep_Com_Type;

// id reply
typedef struct GID_Rep_Id_Type {
    GID_Rep_Com_Type com;
    long             id;
} GID_Rep_Id_Type;

// ping reply
typedef struct GID_Rep_Ping_Type {
    GID_Rep_Com_Type com;
    long             ts_sec;
    long             ts_us;
} GID_Rep_Ping_Type;

// id request
typedef struct GID_Req_Id_Type {
} GID_Req_Id_Type;

// ping request
typedef struct GID_Req_Ping_Type {
} GID_Req_Ping_Type;

// request type
typedef struct GID_Req {
    GID_REQ_TYPE req_type;  // request type
    long         req_tag;   // request tag
    int          req_len;   // size of union type
    union {
        GID_Req_Id_Type   id;
        GID_Req_Ping_Type ping;
    } u;
} GID_Req_Type;

// reply type
typedef struct GID_Rep {
    GID_REP_TYPE rep_type;  // reply type
    long         rep_tag;   // reply tag
    int          rep_len;   // size of union type
    union {
        GID_Rep_Id_Type   id;
        GID_Rep_Ping_Type ping;
    } u;
} GID_Rep_Type;

#endif // !__T284_H_
