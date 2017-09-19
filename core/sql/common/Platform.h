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

#ifdef __gnu_linux__
#endif   /* __gnu_linux__ */


/* GNU Linux: G++/GCC major version 3 */
#if __GNUC__ >= 3
#define NA_GCC
#endif

#ifdef _MSC_VER
#define NA_MSVC
#endif


/* -----------------------------------------------------------------------  */
/* define one of NA_MIPS, NA_IX86 and a define for the endianness           */
/* -----------------------------------------------------------------------  */

#if defined(_M_IX86) || defined(__i386__)
#define NA_IX86
#ifndef	NA_LITTLE_ENDIAN
#define NA_LITTLE_ENDIAN
#endif
#endif


/*
// On Linux, either NA_BIG_ENDIAN or NA_LITTLE_ENDIAN may have already
// been set because some other target may have been defined.  The following
// should set it correctly on Linux.
*/
#if !defined(USE_EMBEDDED_SQL_DEFINITIONS)
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#undef NA_BIG_ENDIAN
#define NA_LITTLE_ENDIAN
#else
#undef NA_LITTLE_ENDIAN
#define NA_BIG_ENDIAN
#endif
#endif /* NA_LINUX */


/* -----------------------------------------------------------------------  */
/* Designate use of static or dynamic queries for Update Statistics.        */
/* -----------------------------------------------------------------------  */
#undef NA_USTAT_USE_STATIC

/* -----------------------------------------------------------------------  */
/* Designate using the real ARKFS                                           */
/* -----------------------------------------------------------------------  */
#ifndef NA_ARKFS
#define NA_ARKFS
#endif

/* -----------------------------------------------------------------------  */
/* Designate if real CatMan integration has happened (or still using sqlcat */
/* simulator).                                                              */
/* -----------------------------------------------------------------------  */
#define NA_CATMAN_SIM

/* -----------------------------------------------------------------------  */
/* Designate if we're switched over to FLEX/BISON                           */
/* -----------------------------------------------------------------------  */
#define NA_FLEXBUILD

/* -----------------------------------------------------------------------  */
/* Set the flavor of Guardian IPC that is used                              */
/* -----------------------------------------------------------------------  */


#ifndef NA_NO_GUARDIAN_IPC /* allow override from dev environment*/
#define NA_GUARDIAN_IPC
#endif

#if defined( NA_C89 ) || defined( NA_MSVC )
/* ----------------------------------------------------------------------- */
/* C89 and MSVC perform template instantiation at compile time,            */
/* so make sure they see the template implementation files                 */
/* ----------------------------------------------------------------------- */

/* The following defines cause the implementation files for templates to
 * be sourced in at compile time, so the compiler actually can perform
 * template instantiation at compile time
 */
#ifndef NA_MSVC
/* RogueWave file rw/compiler.h recognizes this automatically for
   Microsoft Visual C++, no need to set here */
#ifndef	RW_COMPILE_INSTANTIATE
#define RW_COMPILE_INSTANTIATE
#endif
#endif

#ifndef	NA_COMPILE_INSTANTIATE
#define NA_COMPILE_INSTANTIATE
#endif
/* The following define should be used in the template instantiation file
 * to create an empty object file, if the define is set
 */
#ifndef NO_TEMPLATE_INSTANTIATION_FILE
#define NO_TEMPLATE_INSTANTIATION_FILE
#endif
#endif

#define NA_IEEE_FLOAT

#define NA_EIDPROC
#define SQLEXP_LIB_FUNC
#define SQLEXPORT_LIB_FUNC

/* Use ANSI standard namespace for .NET and new GCC compilers. */
#if (_MSC_VER >= 1300) || (__GNUC__ >= 3)
#define NA_STD_NAMESPACE
#endif

#ifdef NA_64BIT
  /* dg64 - need NA_MAX_PATH */
  #define NA_MAX_PATH  PATH_MAX
#else
#define NA_MAX_PATH  _MAX_PATH
#endif


/* BBZ -- used only in /sqlci/sqlci_lex.ll */
#define NA_EXTERN_C_LINKAGE

#define NA_WIDE_CHARACTER

#if !defined(NDEBUG)
    #define NA_DEBUG_C_RUNTIME
#endif

     /* Linux with the gcc compiler */
     typedef int TInt32;
     typedef long long int TInt64;


/*
// ANSI C++ casts (e.g. static_cast) are not supported by NSK c89 version2.
// ANSI C++ casts are supported by the VC++ 5.0 and later compiler
// and by NSK c89 version3.
*/
#if !defined(NA_C89_VERSION2) && !defined(_EMBEDDED)
#define NA_HAS_ANSI_CPP_CASTS
#endif


//
// -------------------------------------------------------------------------
// Set to enable conditional compilation of SeaQuest Unicode code for Linux
// -------------------------------------------------------------------------
//
#define NA_SQ_UNI

/*
// -------------------------------------------------------------------------
// Set to enable conditional compilation of SeaQuest Multi-Temperate Data
// (MTD) code using the metadata column ACCESS_PATH_COLS.DIVISION_KEY_SEQ_NUM
// that is available in the SeaQuest SQL software only.
// -------------------------------------------------------------------------
*/
#define NA_SQ_SMD_DIV_COL

/*
//--------------------------------------------------------------------------
// Hybrid Super Cluster (HSC) definitions
//--------------------------------------------------------------------------
*/

#if defined(_HSC) || defined(HSC)
#define NA_HSC
#endif


#if defined(NA_STD_NAMESPACE) && !defined(USE_EMBEDDED_SQL_DEFINITIONS)
/* If using an ANSI C++ compiler, then go ahead and use the std namespace. */
namespace std {}
using namespace std;
#endif

/* For process thread id, it is long on Linux currently */
typedef long ThreadId;

/* For declare thread private variables (have to be POD types) */
#define THREAD_P __thread

/*
// ---------------------------------------------------------------------
// Used where variable size matters
// Moved from NAVersionedObject.h and Int64.h
// ---------------------------------------------------------------------
*/
typedef char            Int8;
typedef unsigned char   UInt8;
typedef unsigned char   UChar;
typedef short           Int16;
typedef unsigned short  UInt16;
#if !defined(USE_EMBEDDED_SQL_DEFINITIONS)
typedef int             Int32;
typedef unsigned int    UInt32;
#endif
typedef float           Float32;
typedef double          Float64;

#if   defined( __linux__ ) && defined( NA_64BIT )

  typedef long Int64;
  typedef unsigned long UInt64;

#elif (defined( NA_WINNT )) && !defined(USE_EMBEDDED_SQL_DEFINITIONS )     /* NT_PORT SK 08/13/96 */

  typedef _int64 Int64;
  typedef unsigned __int64 UInt64;

#endif  /* NA_C89 */

/*
// format strings, in case not defined (should be seabed/int/types.h)
*/
#ifndef PFSZ
#ifdef NA_64BIT
 #define PFLL  "%ld"
 #define PFLLX "%lx"
 #define PF64  "%ld"
 #define PF64X "%lx"
 #define PFSZ  "%lu"
 #define PFSZX "%lx"
#else
 #define PFLL  "%lld"
 #define PFLLX "%llx"
 #define PF64  "%lld"
 #define PF64X "%llx"
 #define PFSZ  "%u"
 #define PFSZX "%x"
#endif // NA_64BIT
#endif // PFSZ

/*
// additional format strings used only in SQL code
// PFV64 and PFLV64 for variable width field and left pad 0s
// PFP64 added for variable precision.
*/
#ifdef NA_64BIT
 #define PFV64  "%*ld"
 #define PFLV64  "%0*ld"
#define PFP64  "%.*ld"
#else
 #define PFV64  "%*lld"
 #define PFLV64  "%0*lld"
 #define PFP64  "%.*lld"
#endif // NA_64BIT

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

//@ZXrngspec

#endif /* PLATFORM_H  */
