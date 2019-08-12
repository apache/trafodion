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
#ifndef HSCONST_H
#define HSCONST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_const.h
 * Description:  Global constants.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include <float.h>
#include "BaseTypes.h"
#include "ComSmallDefs.h"
#include "EncodedValue.h"
                                              /*==============================*/
                                              /*     HISTOGRAM CONSTANTS      */
                                              /*==============================*/
const Lng32   HS_MAX_INTERVAL_COUNT = 10000;
const Lng32   HS_MAX_BOUNDARY_LEN = 500;
const Lng32   HS_MAX_UCS_BOUNDARY_CHAR = HS_MAX_BOUNDARY_LEN / 2;
const Lng32   HS_SAMP_PCNT_UPSCALE = 10000;
const Lng32   SAMPLE_THRESHOLD = 100000000;
const Lng32   MIN_SAMPLE_SIZE  = 40000;
const Lng32   MAX_MC_COLUMNS = 1000;

//if you modify the MAX_ROWSET value, you must manually modify the ROWSET
//declarations in SQLHIST.MDF. If you forget, you will receive
//ERROR[30008] Rowset index is out of range.
const Lng32   MAX_ROWSET       = HS_MAX_INTERVAL_COUNT + 1;
const Lng32   MIN_BLOCK_SIZE   = 512 ;
const Lng32   MB_IN_BYTES      = 1048576;
const Lng32   VARCHAR_LEN_FIELD_IN_BYTES = 2;
const Lng32   TIMESTAMP_CHAR_LEN = 19;
const Lng32   MAX_IUS_WHERE_LEN = 512;  // keep this in synch with declared len of
                                        //   persistent_samples.ius_search_condition


                                              /*==============================*/
                                              /*        OBJECT RELATED        */
                                              /*==============================*/
enum hs_table_type { UNKNOWN_TYPE = 0,
                     GUARDIAN_TABLE,
                     ANSI_TABLE
                   };
#define HS_EXTENT_SIZE_MX      " EXTENT(8192, 16384) MAXEXTENTS 768 "
#define HS_EXTENT_SIZE_MP_FMT2 " EXTENT(8192, 16384) MAXEXTENTS 959 "
#define HS_EXTENT_SIZE_MP_FMT1 " EXTENT(8192, 1024) MAXEXTENTS 959 "
                                              /*==============================*/
                                              /*       SYNTAX / PARSING       */
                                              /*==============================*/
                                                   /*===   BASIC OPTIONS   ===*/
#define CLEAR_OPT         0x00000001               /* CLEAR                   */
#define VIEWONLY_OPT      0x00000002               /* VIEWONLY (unpublished)  */
#define INTERVAL_OPT      0x00000004               /* GENERATE x INTERVALS    */
#define EVERYCOL_OPT      0x00000008               /* ON EVERY COLUMN         */
#define ROWCOUNT_OPT      0x00000010               /* SET ROWCOUNT c          */
#define EVERYKEY_OPT      0x00000020               /* ON EVERY KEY            */
#define LOG_OPT           0x00000040               /* LOG (unpublished)       */
#define EXISTING_OPT      0x00000080               /* ON EXISTING COLUMNS     */
#define NECESSARY_OPT     0x00000100               /* ON NECESSARY COLUMNS    */
#define REG_GROUP_OPT     0x00000200               /* ON col,...              */
#define CREATE_SAMPLE_OPT 0x00000400               /* CREATE SAMPLE           */
#define REMOVE_SAMPLE_OPT 0x00000800               /* REMOVE SAMPLE           */

                                                   /*=== SAMPLING OPTIONS  ===*/
#define SAMPLE_BASIC_0    0x00001000               /* SAMPLE                  */
#define SAMPLE_BASIC_1    0x00002000               /* SAMPLE+ROWS             */
#define SAMPLE_RAND_1     0x00004000               /* SAMPLE RANDOM           */
#define SAMPLE_RAND_2     0x00008000               /* SAMPLE RANDOM+CLUSTERS  */
#define SAMPLE_PERIODIC   0x00010000               /* SAMPLE PERIODIC         */
#define SAMPLE_REQUESTED  0x0001F000               /* 'OR' sampling options   */
#define SAMPLE_ALL        0x00020000               /* For REMOVE SAMPLE       */
#define NO_SAMPLE         0x00200000               /* NO SAMPLE (explicit)    */

                                                   /*===    IUS OPTIONS    ===*/
#define IUS_OPT           0x00040000               /* INCREMENTAL UPDATE STATS*/
#define IUS_PERSIST       0x00080000               /* CREATE PERSISTENT SAMPLE*/

                                                   /*=== SHOWSTATS OPTIONS ===*/
#define SHOWSTATS_OPT     0x00100000               /* SHOWSTATS COMMAND       */
//  note 0x00200000 taken above
#define DETAIL_OPT        0x00000001               /* DETAILS                 */



                                              /*==============================*/
                                              /*        ERROR CODES           */
                                              /*==============================*/
const Lng32 HS_EOF = 100;
const Lng32 HS_WARNING = 4;
const Lng32 HS_PKEY_FLOAT_ERROR = 1120;

#define HS_QUERY_ERROR  "Process_Query"
#define HS_DDL_ERROR    "Process_DDL"
const Lng32 HS_MAX_MSGTOK_LEN = 1024;
enum USTAT_ERROR_CODES {UERR_SYNTAX_ERROR                    = 15001,
                        UERR_ERROR_LINE_NUMBER               = 1001,
                        UERR_OBJECT_INACCESSIBLE             = 4082,

                        UERR_FIRST_ERROR                     = 9200,
                        UERR_INTERNAL_ERROR                  = 9200,
                        UERR_UNABLE_TO_DROP_OBJECT           = 9201,
                        UERR_DOWN_LEVEL_HISTOGRAMS           = 9202,
                        UERR_COLUMNLIST_NOT_UNIQUE           = 9203,
                        UERR_INVALID_OPTION                  = 9204,
                        UERR_INVALID_OBJECT                  = 9205,
                        UERR_SECURITY_VIOLATION              = 9206,
                        UERR_SAMPLE_SET_IS_ZERO              = 9207,
                        UERR_UNABLE_TO_DESCRIBE_COLUMN_NAMES = 9208,
                        UERR_COLUMN_NAME_DOES_NOT_EXIST      = 9209,
                        UERR_UNSUPPORTED_DATATYPE            = 9210,
                        UERR_WRONG_ON_CLAUSE_FOR_IUS         = 9211,
                        UERR_WARNING_SET_ROWCOUNT            = 9212,
                        UERR_WARNING_FILE_STATISTICS         = 9213,
                        UERR_UNABLE_TO_CREATE_OBJECT         = 9214,
                        UERR_GENERIC_ERROR                   = 9215,
                        UERR_EVERY_COLUMN_NOT_ALLOWED_FOR_LOG  = 9216,
                        UERR_WARNING_NO_EXISTING_HISTOGRAMS  = 9217,
                        UERR_WARNING_NO_OBSOLETE_HISTOGRAMS  = 9218,
                        UERR_IUS_BAD_WHERE_CLAUSE            = 9219,
                        UERR_WARNING_NO_SAMPLE_TABLE_CREATED = 9220,
                        UERR_IUS_NO_PERSISTENT_SAMPLE        = 9221,
                        UERR_WARNING_IUS_TOO_MUCH_RC_CHANGE_INTERVAL = 9222,
                        UERR_WARNING_IUS_TOO_MUCH_RC_CHANGE_TOTAL = 9223,
                        UERR_WARNING_IUS_TOO_MUCH_UEC_CHANGE_INTERVAL= 9224,
                        UERR_WARNING_IUS_TOO_MUCH_UEC_CHANGE_TOTAL = 9225,
                        UERR_WARNING_IUS_ZERO_UEC_INTERVAL   = 9226,
                        UERR_WARNING_IUS_EMPTY_INTERVAL      = 9227,
                        UERR_WARNING_NO_SAMPLE_TABLE         = 9228,
                        UERR_MULTI_COLUMN_LIMIT_EXCEEDED     = 9229,
                        UERR_WARNING_IUS_INSUFFICIENT_MEMORY = 9230,
                        UERR_IUS_WRONG_RANDOM                = 9231,
                        UERR_IUS_IN_PROGRESS                 = 9232,
                        UERR_ALL_LOB_COLUMNS                 = 9233,
                        UERR_IUS_INSERT_NONMFV_OVERFLOW      = 9234,
                        UERR_WARNING_NONEXISTENT_COLUMN      = 9235,
                        UERR_IUS_NO_EXISTING_STATS           = 9236,
                        UERR_IUS_WHERE_CLAUSE                = 9237,
                        UERR_WARNING_FILESTATS_FAILED        = 9238,
                        UERR_NO_VIEWONLY                     = 9239,
                        UERR_NO_ONEVERYKEY                   = 9240,
                        UERR_NO_PRIVILEGE                    = 9241,
                        UERR_CANT_CREATE_HIVE_STATS_SCHEMA   = 9242,
                        UERR_YOU_WILL_LIKELY_BE_SORRY        = 9243,
                        UERR_USER_TRANSACTION                = 9244,
                        UERR_UNEXPECTED_BACKWARDS_DATA       = 9245,
                        UERR_LOB_STATS_NOT_SUPPORTED         = 9246,
                        UERR_VOLATILE_TABLES_NOT_SUPPORTED   = 9247,
                        UERR_FASTSTATS_MEM_ALLOCATION_ERROR  = 9248,
                        UERR_IUS_IS_DISABLED                 = 9249,
                        UERR_WARNING_IUS_NO_LONGER_ALL_NULL  = 9250,
                        UERR_DROP_PERSISTANT_SAMPLE_FIRST    = 9251,
                        UERR_BAD_EST_ROWCOUNT                = 9252,
                        UERR_NO_ERROR                        = 9259,
                        UERR_LAST_ERROR                      = 9259
                       };

// reason of a histogram entry. it is a char value.
enum HS_REASON { 
                  HS_REASON_EMPTY        = ' ',  // it was added because it was needed  
                                                 // but had not been generated
                  HS_REASON_MANUAL       = 'M',  // it was manually generated 
                                                 // by running update statistics
                  HS_REASON_AUTO_INIT    = 'I',  // it is an initial entry generated 
                                                 // by histogram automation 
                  HS_REASON_AUTO_REGEN   = 'N',  // it is a re-generated entry by histogram automation 
                                                 // because it was recently needed by optimizer
                  HS_REASON_SMALL_SAMPLE = 'S',  // it is a histogram generated during compile time
                  HS_REASON_UNKNOWN      = 'X'
                };

const Int32 HS_TIMESTAMP_SIZE = 30;

const Int32 USTAT_CQDS_ALLOWED_FOR_SPAWNED_COMPILERS_MAX_SIZE = 15000;

#endif  /* HSCONST_H */
