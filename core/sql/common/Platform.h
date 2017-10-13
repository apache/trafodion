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
#ifndef PLATFORM_H
#define PLATFORM_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Platform.h
 * Description:  Platform, operating system, and compiler-dependent settings
 *
 * Created:      9/8/95
 * Modified:	 08/08/2006
 * Language:     C (works with both C and C++ compilers)
 *
 *
 *****************************************************************************
 */

/*
// On Linux, either NA_BIG_ENDIAN or NA_LITTLE_ENDIAN may have already
// been set because some other target may have been defined.  The following
// should set it correctly on Linux.
*/
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#undef NA_BIG_ENDIAN
#define NA_LITTLE_ENDIAN
#else
#undef NA_LITTLE_ENDIAN
#define NA_BIG_ENDIAN
#endif

/* -----------------------------------------------------------------------  */
/* Set the flavor of Guardian IPC that is used                              */
/* -----------------------------------------------------------------------  */
#define NA_GUARDIAN_IPC

/* ----------------------------------------------------------------------- */
/* MSVC perform template instantiation at compile time,                    */
/* so make sure they see the template implementation files                 */
/* ----------------------------------------------------------------------- */
#define NA_COMPILE_INSTANTIATE

#define NA_MAX_PATH  PATH_MAX

/* For declare thread private variables (have to be POD types) */
#define THREAD_P __thread

namespace std {}
using namespace std;

/* For process thread id, it is long on Linux currently */
typedef long ThreadId;

/*
 ---------------------------------------------------------------------
 Used where variable size matters
 ---------------------------------------------------------------------
*/
typedef char            Int8;
typedef unsigned char   UInt8;
typedef unsigned char   UChar;
typedef short           Int16;
typedef unsigned short  UInt16;

typedef int             Int32;
typedef unsigned int    UInt32;

typedef float           Float32;
typedef double          Float64;

typedef long Int64;
typedef unsigned long UInt64;

/* Linux with the gcc compiler */
typedef int TInt32;
typedef long long int TInt64;

/*
 format strings
*/
#define PFLL  "%ld"
#define PFLLX "%lx"
#define PF64  "%ld"
#define PF64X "%lx"
#define PFSZ  "%lu"
#define PFSZX "%lx"

/*
 additional format strings.
 PFV64 and PFLV64 for variable width field and left pad 0s
 PFP64 added for variable precision.
*/
#define PFV64  "%*ld"
#define PFLV64  "%0*ld"
#define PFP64  "%.*ld"

/* Lng32 to replace "long" or "signed long" */
/* and some will remain Int32 and others would become Int64 when done */
typedef int             Lng32;

/* ULng32 to replace "unsigned long" or "unsigned long int" */
/* and some will remain UInt32 and others would become UInt64 when done */
typedef unsigned int    ULng32;

/* These types are used for variables that must store integers sometime */
/* and pointers other time. Could have given a better name */
typedef  long Long;
typedef  unsigned long ULong;

#endif /* PLATFORM_H  */
