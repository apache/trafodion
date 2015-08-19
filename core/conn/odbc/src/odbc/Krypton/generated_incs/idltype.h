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
/*
 * idltype.h
 *
 * Andrew Schofield, Wednesday, the 6th of October 1995
 *
 * $Id: idltype.h 1.10 2000/02/10 12:28:28 ASchofield Exp $
 *
 * Header file containing 'C' type definitions corresponding to
 * IDL types and macros
 *
 * The declarations cascade for portability reasons, e.g.
 * IDL_char is used to define IDL_string and IDL_octet is used
 * to define IDL_boolean.
 */
#ifndef IDLTYPE_H
#define IDLTYPE_H

/*
 * Figure out the compilation enviroment 
 * Let's start with TRU64 and alpha
 */
#if defined __alpha && !defined __vms
#define IDL_LP_64
#endif

/*
 * Solaris sparc(v9) 64bit enviroment
 */
#if defined __sun  && defined _LP64 
#define IDL_LP_64
#endif
/*
 * HPUX HPPA 64bit enviroment
 */
#if defined __hpux  && defined __LP64__
#define IDL_LP_64
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Version
 */
#define IDL_TYPE_H_VERSION 19971225

/*
 * One byte types
 */
typedef unsigned char IDL_octet;
#define IDL_octet_min		((IDL_octet) 0)
#define IDL_octet_max		((IDL_octet) 255)

typedef char IDL_char;
#define IDL_char_min		((IDL_char) -128)
#define IDL_char_max		((IDL_char)  127)

typedef IDL_octet IDL_boolean;
#define IDL_TRUE  		((IDL_boolean) 1)
#define IDL_FALSE 		((IDL_boolean) 0)

/*
 * Two byte integers
 */
typedef short IDL_short;
#define IDL_short_min	       	((IDL_short) -32768)
#define IDL_short_max		((IDL_short)  32767)

typedef unsigned short IDL_unsigned_short;
#define IDL_unsigned_short_min	((IDL_unsigned_short) 0)
#define IDL_unsigned_short_max	((IDL_unsigned_short) 65535)

/*
 * Four byte integers
 */
#if defined(IDL_LP_64)
    typedef int IDL_long;
    typedef unsigned int IDL_unsigned_long;
#else
#ifdef _WIN32
    typedef long IDL_long;
    typedef unsigned long IDL_unsigned_long;
#else
    typedef int IDL_long;
    typedef unsigned int IDL_unsigned_long;
#endif
#endif

/*
 * for 64bit StmtHandle
 */

typedef long Long;

/* Some compilers don't deal well with large negative numbers */
#define IDL_long_min	       	((IDL_long) -2147483647 - 1)
#define IDL_long_max		((IDL_long)  2147483647)

#define IDL_unsigned_long_min	((IDL_unsigned_long) 0)
#ifdef _MSC_VER
#define IDL_unsigned_long_max	((IDL_unsigned_long) 4294967295)
#else
#define IDL_unsigned_long_max	((IDL_unsigned_long) 4294967295UL)
#endif

typedef IDL_long IDL_enum;
#define IDL_enum_min	  	((IDL_enum) 0)
#define IDL_enum_max		((IDL_enum) IDL_long_max)

typedef IDL_long IDL_union_discriminator;

/*
 * Eight byte integers
 */
#if defined(_MSC_VER) || defined(__vms)
typedef __int64 IDL_long_long;
#else
#ifndef __cplusplus
typedef long long IDL_long_long;
#else
#if defined(__USLC__) /* UNIXWARE */ || defined(__hpux) \
  || (defined(__nonstopux) && !defined(_LONGLONG)) /* NSUXB */
typedef union IDL_long_long_tag
{
  IDL_long	v_long[2];	     /* must be first, see IDL_long_long_max */
  double 	v_double; 			   /* force 8 byte alignment */
  char   	v_char[8];
  IDL_short	v_short[4];
} IDL_long_long;
#define IDL_long_long_faked
#else
typedef long long IDL_long_long;
#endif /* __USLC__ || __hpux */
#endif /* __cplusplus */
#endif /* _MSC_VER */

#if defined(_MSC_VER) || defined(__vms)  
typedef unsigned __int64 IDL_unsigned_long_long;
#else
typedef unsigned long long IDL_unsigned_long_long;
#endif


#ifdef IDL_long_long_faked
/* These declarations only work as initializers */
#define IDL_long_long_min	{ IDL_long_min, IDL_long_min }
/* Approximate max, since can't assume byte order */
#define IDL_long_long_max	{ IDL_long_max, IDL_long_max }

#define IDL_unsigned_long_long_min  	{ 0, 0 }
#define IDL_unsigned_long_long_max 	IDL_long_long_max

#else

#if defined(_MSC_VER) || defined(__vms) || defined(__nonstopux) || defined(__alpha) || defined(__SunOS_5_7)
#define IDL_long_long_min	((IDL_long_long) -9223372036854775807 - 1)
#define IDL_long_long_max	((IDL_long_long)  9223372036854775807)
#else
#define IDL_long_long_min	((IDL_long_long) -9223372036854775807LL - 1)
#define IDL_long_long_max	((IDL_long_long)  9223372036854775807LL)
#endif

#define IDL_unsigned_long_long_min  	((IDL_unsigned_long_long) 0)
#define IDL_unsigned_long_long_max	\
                                   ((IDL_unsigned_long_long) IDL_long_long_max)
#endif

/*
 * Real numbers
 */
typedef float IDL_float;
typedef double IDL_double;

/*
 * Pointers
 */
#if defined(IDL_LP_64)
#define IDL_PTR_SIZE 8
#endif

#ifndef IDL_PTR_SIZE
#define IDL_PTR_SIZE 4
#endif

/*
 * Defines to pad pointers to a fixed size (8 bytes)
 */
#if IDL_PTR_SIZE == 8
#define IDL_PTR_PAD(name, count)
#else
#define IDL_PTR_PAD(name, count)  char name##pad_[8 - IDL_PTR_SIZE] [count];
#endif

/*
 * Any type
 */
typedef struct {
  void *_type;
  IDL_PTR_PAD(_type, 1)
  void *_value;
  IDL_PTR_PAD(_value, 1)
} IDL_any;

/*
 * Unbounded string
 */
typedef IDL_char *IDL_string;

/*
 * Object reference
 */
typedef char *IDL_Object;

/*
 * Void type
 */
typedef void IDL_void;

/*
 * TypeCode
 */
typedef void *IDL_TypeCode;

/*
 * Principal
 */
typedef struct IDL_Principal_seq_ {
  IDL_unsigned_long _length;
  char pad_to_offset_8_[4];
  IDL_octet *_buffer;
  IDL_PTR_PAD(_buffer, 1)
} IDL_Principal;

#ifdef __cplusplus
}
#endif
#endif /* IDLTYPE_H */

