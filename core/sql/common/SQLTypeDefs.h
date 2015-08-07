#ifndef SQLTYPEDEFS_H
#define SQLTYPEDEFS_H
/* -*-C++-*-
*************************************************************************
*
* File:         SQLTypeDefs.h
* Description:  Declarations for SQL related constants
*
*               
* Created:      5/20/94
* Language:     C++
*
*
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
*
*
**************************************************************************
*/


// -----------------------------------------------------------------------
// Precision for SQL numeric data types in bytes
// -----------------------------------------------------------------------
#define BINARY8_PRECISION   8
#define BINARY16_PRECISION  16
#define BINARY32_PRECISION  32
#define BINARY64_PRECISION  64
#define SIGN_BIT            1

#define SQL_SMALL_PRECISION   15
#define SQL_USMALL_PRECISION  16
#define SQL_INT_PRECISION     31
#define SQL_UINT_PRECISION    32
#define SQL_LARGE_PRECISION   63
#define SQL_ULARGE_PRECISION  64
#define SQL_REAL_PRECISION    22
#define SQL_FLOAT_PRECISION   54
#define SQL_DOUBLE_PRECISION  54

// -----------------------------------------------------------------------
// Sizes of standard SQL data types in bytes
// -----------------------------------------------------------------------
#define SQL_SMALL_SIZE 2
#define SQL_INT_SIZE 4
#define SQL_LARGE_SIZE 8
#define SQL_REAL_SIZE 4
#define SQL_FLOAT_SIZE 8
#define SQL_DOUBLE_PRECISION_SIZE 8
#define SQL_CHAR_SIZE 1
#define SQL_DBCHAR_SIZE 2

#define SQL_NULL_HDR_SIZE 2
#define SQL_VARCHAR_HDR_SIZE 2
#define SQL_VARCHAR_HDR_SIZE_4 4

// -----------------------------------------------------------------------
// display Sizes of standard SQL data types in bytes
// -----------------------------------------------------------------------
#define SQL_SMALL_DISPLAY_SIZE 6
#define SQL_USMALL_DISPLAY_SIZE 5
#define SQL_INT_DISPLAY_SIZE 11
#define SQL_UINT_DISPLAY_SIZE 10
#define SQL_LARGE_DISPLAY_SIZE 20
#define SQL_REAL_DISPLAY_SIZE 15
#define SQL_REAL_MIN_DISPLAY_SIZE 9
#define SQL_REAL_FRAG_DIGITS 7
#define SQL_FLOAT_DISPLAY_SIZE 25
#define SQL_FLOAT_FRAG_DIGITS 17
#define SQL_DOUBLE_PRECISION_DISPLAY_SIZE 25
#define SQL_DOUBLE_PRECISION_FRAG_DIGITS 17

// -----------------------------------------------------------------------
// display sizes for datetime types in bytes
// -----------------------------------------------------------------------
#define DATE_DISPLAY_SIZE      10
#define TIME_DISPLAY_SIZE       8
#define TIMESTAMP_DISPLAY_SIZE 19

#endif /* SQLTYPEDEFS_H */
