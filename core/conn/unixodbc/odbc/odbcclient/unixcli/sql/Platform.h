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

#ifdef _WIN32
#define NA_WINNT
#endif




/* -----------------------------------------------------------------------
 * define one of NA_MSVC, NA_C89
 * -----------------------------------------------------------------------
*/

#ifdef _MSC_VER
#define NA_MSVC
#endif

/*  c89 is the only Tandem C++ compiler that we allow                      */

/* -----------------------------------------------------------------------  */
/* define one of NA_MIPS, NA_IX86 and a define for the endianness           */
/* -----------------------------------------------------------------------  */

#ifdef _M_IX86
#define NA_IX86
#ifndef	NA_LITTLE_ENDIAN
#define NA_LITTLE_ENDIAN
#endif
#endif

/* these days MIPS big-endian is the only Tandem platform                   */


/* -----------------------------------------------------------------------  */
/* define some DEBUG and TRACE things                                       */
/* -----------------------------------------------------------------------  */



/* -----------------------------------------------------------------------  */
/* The Windows GUI can only be used if we are running on NT                 */
/* -----------------------------------------------------------------------  */


/* -----------------------------------------------------------------------  */
/* Turn on simulated FS labels feature usage for the catman.                */
/* -----------------------------------------------------------------------  */

/* -----------------------------------------------------------------------  */
/* Designate using the real ARKFS                                           */
/* -----------------------------------------------------------------------  */

/* -----------------------------------------------------------------------  */
/* Designate if real CatMan integration has happened (or still using sqlcat */
/* simulator).                                                              */
/* -----------------------------------------------------------------------  */

/* -----------------------------------------------------------------------  */
/* Designate if we're switched over to FLEX/BISON                           */
/* -----------------------------------------------------------------------  */

/* -----------------------------------------------------------------------  */
/* Designate that calls to the SQL CLI shadow process service DLL           */
/* (tdm_sqlclisp.dll) should be generated on NT                             */
/* -----------------------------------------------------------------------  */
//  The shadow code is no longer used to this flag will be disabled
//#if defined( NA_WINNT ) && !defined( __EID )
//#ifndef NA_SHADOWCALLS
//#define NA_SHADOWCALLS
//#endif
//#endif

/* -----------------------------------------------------------------------  */
/* On the OSS and Unix platforms, build with the HYBRID interface.          */
/* -----------------------------------------------------------------------  */

/* -----------------------------------------------------------------------  */
/* On the OSS Platform, some code in ComSysUtils must be turned on          */
/* to provide the gettimeofday() system call, since OSS currently           */
/* doesn't support gettimeofday().                                          */
/* -----------------------------------------------------------------------  */

/* -----------------------------------------------------------------------  */
/* Set the flavor of Guardian IPC that is used                              */
/* -----------------------------------------------------------------------  */



/* -----------------------------------------------------------------------  */
/* Set a define if we have a PRIV segment for the CLI data and don't        */
/* allow any global variables in the CLI library (or SRL).                  */
/* -----------------------------------------------------------------------  */


/* -----------------------------------------------------------------------  */
/* Tandem Platforms require data alignment                                  */
/* -----------------------------------------------------------------------  */

/* -----------------------------------------------------------------------  */
/* The Tandem compiler doesn't understand a friend declaration in a         */
/* template that references another template.                               */
/* -----------------------------------------------------------------------  */


/* -----------------------------------------------------------------------  */
/* Pre-ANSI C++ compilers allowed iostreams to be assigned to each other.   */
/* This behavior is not allowed by the ANSI C++ iostream library.           */
/* -----------------------------------------------------------------------  */
#if defined (NA_NSK) || (defined(_MSC_VER) && (_MSC_VER < 1300))
#define NA_ASSIGNABLE_IOSTREAMS
#endif


/* -----------------------------------------------------------------------  */
/* The c89 compiler forces you to distinguish pre/post operators (++/--).   */
/* That is very C++ (nice).  Centerline doesn't.                            */
/* -----------------------------------------------------------------------  */

/* ----------------------------------------------------------------------- */
/* C89 and MSVC perform template instantiation at compile time,            */
/* so make sure they see the template implementation files                 */
/* ----------------------------------------------------------------------- */

#if defined( NA_C89 ) || defined( NA_MSVC )
/* the following defines cause the implementation files for templates to
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
/* the following define should be used in the template instantiation file
 * to create an empty object file, if the define is set
 */
#ifndef NO_TEMPLATE_INSTANTIATION_FILE
#define NO_TEMPLATE_INSTANTIATION_FILE
#endif
#endif

/* -----------------------------------------------------------------------
 * The Tandem compiler uses Tandem floating point representation, all
 * other compilers use IEEE floating point representation.
 * Changing to use IEEE format floating point representation on NSK.
 *           3/24/2002
 * ----------------------------------------------------------------------
 */
#define NA_IEEE_FLOAT

/* -------------------------------------------------------------
 * Any procedure called from DP2 must be resident. So, all
 * executor/expression procs that are moved to EID (Executor
 * In Dp2) are made resident. Also, define _resident attribute
 * to be empty if we are not building mxindp2.
 * -------------------------------------------------------------
 */
#define NA_EIDPROC
#define _resident

/* --------------------------------------------------------------------------
   If an include file contains a method that has the
   _priv attribute, then that object becomes priv. We do not want that to
   happen if we are not building the priv srl. Define _priv to be empty
   in that case.
*/


#define NA_MAX_PATH  _POSIX_PATH_MAX


#define NA_WIDE_CHARACTER




#if !defined(NDEBUG) && !defined(NA_NO_C_RUNTIME)
#endif


        typedef long TInt32;
	typedef long long TInt64;

// The 1989 ANSI C++ Standard specifies const types can be used within
// a header file (rather than the enum hack).  VC++ 7.1 and NSK c89 
// version3 both support this use of const types within header files.

// ANSI C++ casts (e.g. static_cast) are not supported by NSK c89 version2.
// ANSI C++ casts are supported by the VC++ 5.0 and later compiler
// and by NSK c89 version3.
#if !defined(NA_C89_VERSION2)
#define NA_HAS_ANSI_CPP_CASTS
#endif

#endif /* PLATFORM_H  */

