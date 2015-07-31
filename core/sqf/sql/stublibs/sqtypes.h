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

#ifndef _SQTYPES_H
#define _SQTYPES_H

#include <stdlib.h>

#define DLLEXPORT
#define SECLIBAPI

typedef unsigned char unsigned_char;

typedef short         int_16;

typedef int           _int_32;
typedef _int_32       int_32;
typedef unsigned long _unsigned_32;
typedef _unsigned_32  unsigned_32;

#ifdef NA_64BIT
typedef long          _int_64;
#else
typedef long long     _int_64;
#endif

typedef _int_64       _int64;
typedef _int_64       int_64;

typedef int_64        fixed_0;

typedef double        DblInt;
typedef unsigned long DWORD;

typedef char *           LPCSTR;
typedef const wchar_t *  LPCWSTR;

typedef void *        LPVOID;

typedef short         BOOL;

#define FALSE         0

#endif
