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
**********************************************************************/

#ifndef WIDECHAR_H
#define WIDECHAR_H 1

#include "NAWinNT.h"

struct _scanfbuf {
        NAWchar *_ptr;
        UInt32   _cnt;
        };

typedef struct _scanfbuf SCANBUF;

#define _r _cnt
#define _p _ptr

Int32 na_swscanf(const NAWchar *str, NAWchar const *fmt, ...);

struct _sprintf_buf {
        NAWchar *_ptr;
         Int32   _cnt;
        };

typedef struct _sprintf_buf SPRINTF_BUF;

#define _w _cnt

Int32
na_wsprintf(NAWchar *str, NAWchar const *fmt, ...);

#if (defined(NA_C89) || defined(NA_WINNT)) 
typedef UInt64  u_quad_t;
typedef Int64 quad_t;
#endif

typedef unsigned short u_short;
typedef UInt32   u_int;

#endif
