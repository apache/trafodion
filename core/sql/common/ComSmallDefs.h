/* -*-C++-*- */
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
#ifndef COMSMALLDEFS_H
#define COMSMALLDEFS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComSmallDefs.h
 * Description:  Small definitions are declared here that are used throughout
 *               the SQL/ARK product.
 *
 *
 * Created:      10/27/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */


#include <iosfwd>
using namespace std;

#include <stdio.h>
#include "NAAssert.h"   // required after including a RogueWave file!
#include "NABoolean.h"
#include "NAString.h"
#include "Int64.h"
#include "dfs2rec.h"

#ifndef NULL
#define NULL 0
#endif

// A better, fuller Int64 implementation has been developed in Int64
// replace the old ComSInt64 here.  Both will be replaced by long long
// when we have a modern C compiler
#define ComSInt64 Int64

typedef unsigned short          ComUInt16;
typedef          short          ComSInt16;
typedef         UInt32          ComUInt32;
typedef          Int32          ComSInt32;

// ++ MV
#define MAX_COMSINT32 2147483647
#define MAX_NEG_COMSINT32 -2147483648
#define UINT32_MAX_LENGTH 12
// -- MV

// max precision supported by system hardware. Signed NUMERICs upto this
// range are declared as INT64 (largeint).
#define MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION 18

//Unsigned NUMERICs upto this range are supported by hardware.
#define MAX_HARDWARE_SUPPORTED_UNSIGNED_NUMERIC_PRECISION 20

typedef NAString                ComString;

typedef ComSInt64               ComTimestamp;

typedef Int32			ComUserID;

typedef NABoolean               ComBoolean;

// user and role definitions have been moved to NAUserId.h

// Defaults for system attributes
#define SMD_LOCATION  "$SYSTEM"
#define SMD_VERSION   "1000"

#define COM_VOLATILE_SCHEMA_PREFIX "VOLATILE_SCHEMA_"

// prefix of temp tables for common subexpressions
#define COM_CSE_TABLE_PREFIX "CSE_TEMP_"

// 'reserved' tables in public_access_schema for sql internal use
#define COM_PUBLIC_ACCESS_SCHEMA "PUBLIC_ACCESS_SCHEMA"

#define HIVE_SYSTEM_CATALOG          "HIVE"
#define HIVE_SYSTEM_CATALOG_LC       "hive"
#define HIVE_SYSTEM_SCHEMA           "HIVE"
#define HIVE_SYSTEM_SCHEMA_LC        "hive"

// default schema name to be passed to hive methods at runtime
#define HIVE_DEFAULT_SCHEMA_EXE      "default"

#define HIVE_STATS_CATALOG           "TRAFODION"
#define HIVE_STATS_SCHEMA            "\"_HIVESTATS_\""
#define HIVE_STATS_SCHEMA_NO_QUOTES  "_HIVESTATS_"
#define HIVE_EXT_SCHEMA_PREFIX       "_HV_"
#define HBASE_EXT_MAP_SCHEMA         "_HB_MAP_"

#define HBASE_SYSTEM_CATALOG          "HBASE"
#define HBASE_SYSTEM_SCHEMA           "HBASE"
#define HBASE_CELL_SCHEMA             "_CELL_"
#define HBASE_ROW_SCHEMA              "_ROW_"
#define HBASE_MAP_SCHEMA              "_MAP_"
#define HBASE_HIST_NAME               "SB_HISTOGRAMS"
#define HBASE_HISTINT_NAME            "SB_HISTOGRAM_INTERVALS"
#define HBASE_PERS_SAMP_NAME          "SB_PERSISTENT_SAMPLES"
#define HBASE_HIST_PK                    "SB_HISTOGRAMS_PK"
#define HBASE_HISTINT_PK               "SB_HISTOGRAM_INTERVALS_PK"
#define HBASE_PERS_SAMP_PK            "SB_PERSISTENT_SAMPLES_PK"
#define HBASE_EXT_SCHEMA_PREFIX       "_HB_"

#define HBASE_STATS_CATALOG          "TRAFODION"
#define HBASE_STATS_SCHEMA           "\"_HBASESTATS_\""
#define HBASE_STATS_SCHEMA_NO_QUOTES "_HBASESTATS_"

// default null format for data in hive files.
#define HIVE_DEFAULT_NULL_STRING             "\\N"

#define TRAFODION_SYSCAT_LIT              "TRAFODION"
#define SEABASE_SYSTEM_SCHEMA           "SEABASE"
#define SEABASE_OLD_PRIVMGR_SCHEMA         "PRIVMGR_MD"
#define SEABASE_PRIVMGR_SCHEMA         "_PRIVMGR_MD_"
#define SEABASE_UDF_SCHEMA             "_UDF_"
#define TRAF_SAMPLE_PREFIX             "TRAF_SAMPLE_"  // prefix for a sample table used by update stats
#define LOB_MD_PREFIX                  "LOBMD_"
#define LOB_DESC_CHUNK_PREFIX          "LOBDescChunks_"
#define LOB_DESC_HANDLE_PREFIX         "LOBDescHandle_"
#define SEABASE_DEFAULT_COL_FAMILY "#1"

// reserved names for seabase metadata where SQL table information is kept
// there are places in the code that assume metadata schema and table 
// names are less than 100 characters.  If you create a name that is 
// bigger than this, be sure to changes places like GET cidd
#define SEABASE_MD_SCHEMA     "_MD_"
#define SEABASE_COLUMNS         "COLUMNS"
#define SEABASE_DEFAULTS        "DEFAULTS"
#define SEABASE_INDEXES          "INDEXES"
#define SEABASE_KEYS                "KEYS"
#define SEABASE_LIBRARIES         "LIBRARIES"
#define SEABASE_LIBRARIES_USAGE   "LIBRARIES_USAGE"
#define SEABASE_OBJECTS          "OBJECTS"
#define SEABASE_OBJECTS_UNIQ_IDX "OBJECTS_UNIQ_IDX"
#define SEABASE_OBJECTUID      "OBJECTUID"
#define SEABASE_REF_CONSTRAINTS "REF_CONSTRAINTS"
#define SEABASE_ROUTINES          "ROUTINES"
#define SEABASE_SEQ_GEN          "SEQ_GEN"
#define SEABASE_TABLES            "TABLES"
#define SEABASE_TABLE_CONSTRAINTS "TABLE_CONSTRAINTS"
#define SEABASE_TABLE_CONSTRAINTS_IDX "TABLE_CONSTRAINTS_IDX"
#define SEABASE_TEXT            "TEXT"
#define SEABASE_UNIQUE_REF_CONSTR_USAGE "UNIQUE_REF_CONSTR_USAGE"
#define SEABASE_VIEWS              "VIEWS"
#define SEABASE_VIEWS_USAGE  "VIEWS_USAGE"
#define SEABASE_VERSIONS        "VERSIONS"
#define SEABASE_AUTHS            "AUTHS"
#define SEABASE_VALIDATE_SPJ      "VALIDATEROUTINE"
#define SEABASE_VALIDATE_LIBRARY  "UDR_LIBRARY"

#define SEABASE_SCHEMA_OBJECTNAME "__SCHEMA__" 

#define SEABASE_SEQ_GEN  "SEQ_GEN"

// DTM log files are created in this schema. It is a reserved schema.
#define SEABASE_DTM_SCHEMA     "_DTM_"

// Trafodion statistics repository reserved schema
#define SEABASE_REPOS_SCHEMA  "_REPOS_"
#define REPOS_METRIC_QUERY_AGGR_TABLE "METRIC_QUERY_AGGR_TABLE"
#define REPOS_METRIC_QUERY_TABLE "METRIC_QUERY_TABLE"
#define REPOS_METRIC_SESSION_TABLE "METRIC_SESSION_TABLE"
#define REPOS_METRIC_TEXT_TABLE  "METRIC_TEXT_TABLE"

#define SEABASE_REGRESS_DEFAULT_SCHEMA "SCH"

// Trafodion system library and procedures reserved schema
// Procedures are defined in CmpSeabaseDDLroutine.h
#define SEABASE_LIBMGR_SCHEMA "_LIBMGR_"
#define SEABASE_LIBMGR_LIBRARY "DB__LIBMGRNAME"
#define SEABASE_LIBMGR_LIBRARY_CPP "DB__LIBMGR_LIB_CPP"

// reserved column names for traf internal system usage
#define TRAF_SALT_COLNAME "_SALT_"
#define TRAF_DIVISION_COLNAME_PREFIX "_DIVISION_"
#define TRAF_SYSKEY_COLNAME "SYSKEY"

// length of explain_plan column in metric_query_table.
// explain_plan greater than this length are chunked and store in multiple
// rows in metric_text_table
// Note: This symbol is used in the DDL for the Repository tables.
// If you change it, consider whether the Repository tables will need
// an upgrade. See file sqlcomp/CmpSeabaseDDLrepos.h.
#define REPOS_MAX_EXPLAIN_PLAN_LEN 1000000
#define REPOS_MAX_EXPLAIN_PLAN_LEN_STR "1000000"

/******    *****/
enum ComActivationTime { COM_UNKNOWN_TIME, COM_BEFORE , COM_AFTER };
#define COM_BEFORE_LIT         "B "
#define COM_AFTER_LIT          "A "
#define COM_UNKNOWN_TIME_LIT "  "

enum ComOperation{COM_UNKNOWN_IUD, COM_INSERT, COM_DELETE, COM_UPDATE,
                  COM_SELECT, COM_ROUTINE};
#define COM_INSERT_LIT       "I "
#define COM_DELETE_LIT       "D "
#define COM_UPDATE_LIT       "U "
#define COM_SELECT_LIT       "S "
#define COM_ROUTINE_LIT      "R "
#define COM_UNKNOWN_IUD_LIT  "  "

enum ComGranularity { COM_UNKNOWN_GRANULARITY, COM_ROW, COM_STATEMENT };
#define COM_ROW_LIT                  "R "
#define COM_STATEMENT_LIT            "S "
#define COM_UNKNOWN_GRANULARITY_LIT  "  "

enum ComYesNo { COM_YES, COM_NO, COM_NULL };
#define COM_YES_LIT       "Y "
#define COM_NO_LIT        "N "
#define COM_NULL_LIT      "  "


//----------------------------------------------------------------------------
//++ MVS
enum ComMVType {  COM_MJV
                 ,COM_MAV
                 ,COM_MAJV
                 ,COM_MV_OTHER
                 ,COM_MV_UNKNOWN
        };

#define COM_MJV_LIT         "J "
#define COM_MAV_LIT         "A "
#define COM_MAJV_LIT        "X "
#define COM_MV_OTHER_LIT    "O "
#define COM_MV_UNKNOWN_LIT  "  "


enum ComMVStatus {  COM_MVSTATUS_INITIALIZED
                   ,COM_MVSTATUS_NOT_INITIALIZED
                   ,COM_MVSTATUS_NO_INITIALIZATION
                   ,COM_MVSTATUS_UNAVAILABLE
                   ,COM_MVSTATUS_UNKNOWN
                 };

#define COM_MVSTATUS_INITIALIZED_LIT        "Y "
#define COM_MVSTATUS_NOT_INITIALIZED_LIT    "N "
#define COM_MVSTATUS_NO_INITIALIZATION_LIT  "I "
#define COM_MVSTATUS_UNAVAILABLE_LIT        "U "
#define COM_MVSTATUS_UNKNOWN_LIT            "  "


enum ComMvAuditType {  COM_MV_AUDIT
                      ,COM_MV_NO_AUDIT
                      ,COM_MV_NO_AUDIT_ON_REFRESH
                      ,COM_MV_AUDIT_UNKNOWN
                    };

#define COM_MV_AUDIT_LIT                "A "
#define COM_MV_NO_AUDIT_LIT             "N "
#define COM_MV_NO_AUDIT_ON_REFRESH_LIT  "R "
#define COM_MV_AUDIT_UNKNOWN_LIT        "  "

enum ComMVAttribute { COM_MVATTRIBUTE_UNKNOWN }; // For future use

#define COM_MV_ATTRIBUTE_UNKNOWN_LIT  "    "


enum ComMVRefreshType {  COM_ON_STATEMENT
                        ,COM_ON_REQUEST
                        ,COM_RECOMPUTE
                        ,COM_BY_USER
                        ,COM_UNKNOWN_RTYPE
                      };

#define COM_ON_STATEMENT_LIT    "S "
#define COM_ON_REQUEST_LIT      "R "
#define COM_RECOMPUTE_LIT       "C "
#define COM_BY_USER_LIT         "U "
#define COM_UNKNOWN_RTYPE_LIT   "  "


enum ComMVSUsedTableAttribute {  COM_NO_ATTRIBUTE
                                ,COM_IGNORE_CHANGES
                                ,COM_INSERT_ONLY
                              };

#define COM_NO_ATTRIBUTE_LIT    "NO"
#define COM_IGNORE_CHANGES_LIT  "IC"
#define COM_INSERT_ONLY_LIT     "IO"


enum ComMVSUsageType {  COM_USER_SPECIFIED
                       ,COM_DIRECT_USAGE
                       ,COM_EXPANDED_USAGE
                       ,COM_UNKNOWN_USAGE
                     };

#define COM_USER_SPECIFIED_LIT  "U "
#define COM_DIRECT_USAGE_LIT    "D "
#define COM_EXPANDED_USAGE_LIT  "E "
#define COM_UNKNOWN_USAGE_LIT   "  "


enum ComLeftJoinTableType {  COM_NO_LEFT_JOIN
                            ,COM_LEFT_INNER
                            ,COM_LEFT_OUTER
                            ,COM_LEFT_JOIN_UNKNOWN
                          };

#define COM_NO_LEFT_JOIN_LIT        "N "
#define COM_LEFT_INNER_LIT          "I "
#define COM_LEFT_OUTER_LIT          "O "
#define COM_LEFT_JOIN_UNKNOWN_LIT   "  "



enum ComMVColType {  COM_MVCOL_GROUPBY
                    ,COM_MVCOL_CONST
                    ,COM_MVCOL_AGGREGATE
                    ,COM_MVCOL_DUPLICATE
                    ,COM_MVCOL_OTHER
                    ,COM_MVCOL_FUNCTION
                    ,COM_MVCOL_BASECOL
                    ,COM_MVCOL_REDUNDANT
                    ,COM_MVCOL_COMPLEX
                    ,COM_MVCOL_UNKNOWN
                  };

#define   COM_MVCOL_GROUPBY_LIT   "GRP "
#define   COM_MVCOL_CONST_LIT     "CNS "
#define   COM_MVCOL_AGGREGATE_LIT "AGG "
#define   COM_MVCOL_DUPLICATE_LIT "DUP "
#define   COM_MVCOL_REDUNDANT_LIT "RED "
#define   COM_MVCOL_COMPLEX_LIT   "CPX "
#define   COM_MVCOL_OTHER_LIT     "OTH "
#define   COM_MVCOL_FUNCTION_LIT  "FUN "
#define   COM_MVCOL_BASECOL_LIT   "BAS "
#define   COM_MVCOL_UNKNOWN_LIT   "    "

// For the operator types (the types (ITM_...) are taken from OperTypeEnum)
#define   COM_COUNT_LIT          "CNT "
#define   COM_COUNT_NONULL_LIT   "CTN "
#define   COM_SUM_LIT            "SUM "
#define   COM_AVG_LIT            "AVG "
#define   COM_MIN_LIT            "MIN "
#define   COM_MAX_LIT            "MAX "
#define   COM_VARIANCE_LIT       "VAR "
#define   COM_STDDEV_LIT         "STD "
#define   COM_BASECOL_LIT        "BCL "
#define   COM_UNKNOWN_AGG_LIT    "    "


enum ComMVIncRefStatus { COM_REF_STAT_UNKNOWN
                        ,COM_REF_STAT_OK
                        ,COM_REF_STAT_RECOMPUTE_REQUIRED
                        ,COM_REF_STAT_LOCK_REQUIRED
                       };

#define COM_REF_STAT_OK_LIT                 "OK"
#define COM_REF_STAT_RECOMPUTE_REQUIRED_LIT "RR"
#define COM_REF_STAT_LOCK_REQUIRED_LIT      "LR"
#define COM_REF_STAT_UNKNOWN_LIT            "  "


enum ComRangeLogType {  COM_NO_RANGELOG
                       ,COM_MANUAL_RANGELOG
                       ,COM_AUTO_RANGELOG
                       ,COM_MIXED_RANGELOG
                       ,COM_RANGELOG_UNKNOWN
                     };

#define COM_NO_RANGELOG_LIT         "N "
#define COM_MANUAL_RANGELOG_LIT     "M "
#define COM_AUTO_RANGELOG_LIT       "A "
#define COM_MIXED_RANGELOG_LIT      "X "
#define COM_RANGELOG_UNKNOWN_LIT    "  "


enum ComMvsAllowed {  COM_NO_MVS_ALLOWED
                     ,COM_ALL_MVS_ALLOWED
                     ,COM_ON_STATEMENT_MVS_ALLOWED
                     ,COM_ON_REQUEST_MVS_ALLOWED
                     ,COM_RECOMPUTE_MVS_ALLOWED
                     ,COM_MVS_ALLOWED_UNKNOWN
                   };

#define COM_NO_MVS_ALLOWED_LIT              "N "
#define COM_ALL_MVS_ALLOWED_LIT             "A "
#define COM_ON_STATEMENT_MVS_ALLOWED_LIT    "S "
#define COM_ON_REQUEST_MVS_ALLOWED_LIT      "R "
#define COM_RECOMPUTE_MVS_ALLOWED_LIT       "C "
#define COM_MVS_ALLOWED_UNKNOWN_LIT         "  "


// This enum marks the type of table that should be created by
// /catman/CatExecCreateTable  .  I guess that TransientObject
// could also be an option here.
enum ComTableType {  COM_REGULAR_TABLE
                    ,COM_TRIGTEMP_TABLE
                    ,COM_IUD_LOG_TABLE
                    ,COM_RANGE_LOG_TABLE
                    ,COM_MVS_UMD
                    ,COM_MV_TABLE
                    ,COM_EXCEPTION_TABLE
                    ,COM_INDEX_TABLE // added for parallel create/drop
                    ,COM_GHOST_REGULAR_TABLE
                    ,COM_GHOST_MV_TABLE
                    ,COM_GHOST_INDEX_TABLE
                    ,COM_GHOST_IUD_LOG_TABLE
                    ,COM_SG_TABLE
                    ,COM_SCHEMA_LABEL_TABLE
                  };

#define COM_REGULAR_TABLE_LIT       "RT"
#define COM_TRIGTEMP_TABLE_LIT      "TT"
#define COM_IUD_LOG_TABLE_LIT       "IU"
#define COM_RANGE_LOG_TABLE_LIT     "RL"
#define COM_MVS_UMD_LIT             "UM"
#define COM_MV_TABLE_LIT            "MV"
#define COM_EXCEPTION_TABLE_LIT     "ET"
#define COM_INDEX_TABLE_LIT         "IT" // added for parallel create/drop
#define COM_GHOST_REGULAR_TABLE_LIT "GR"
#define COM_GHOST_MV_TABLE_LIT      "GM"
#define COM_GHOST_INDEX_TABLE_LIT   "GI"
#define COM_GHOST_IUD_LOG_TABLE_LIT "GU"
#define COM_SG_TABLE_LIT            "SG"
#define COM_SCHEMA_LABEL_TABLE_LIT  "SL"


#define EPOCH_INITIAL_VALUE 100

// JulianTimestamp time of UNIX "epoch", 00:00:00 Jan 1, 1970
const Int64 COM_EPOCH_TIMESTAMP=210866760000000000LL;

// enums used to specify the MV REWRITE PUBLISH operations
// and SYSTEM DEFAULTS propagation
enum ComPublishMVOperationType { COM_PUBLISH_MV_CREATE,
                                 COM_PUBLISH_MV_CREATE_AND_REFRESH,
                                 COM_PUBLISH_MV_DROP,
                                 COM_PUBLISH_MV_REFRESH,
                                 COM_PUBLISH_MV_REFRESH_RECOMPUTE,
                                 COM_PUBLISH_MV_RENAME,
                                 COM_PUBLISH_MV_ALTER_IGNORE_CHANGES,
                                 COM_PUBLISH_MV_TOUCH,
                                 COM_PUBLISH_MV_REPUBLISH,
                                 COM_PUBLISH_MV_DEFAULT,
                                 COM_PUBLISH_MV_UNKNOWN
                               };



#define COM_PUBLISH_MV_CREATE_LIT                       "CT"
#define COM_PUBLISH_MV_CREATE_AND_REFRESH_LIT           "CR"
#define COM_PUBLISH_MV_DROP_LIT                         "DP"
#define COM_PUBLISH_MV_REFRESH_LIT                      "RF"
#define COM_PUBLISH_MV_REFRESH_RECOMPUTE_LIT            "RR"
#define COM_PUBLISH_MV_RENAME_LIT                       "MR"
#define COM_PUBLISH_MV_ALTER_IGNORE_CHANGES_LIT         "AI"
#define COM_PUBLISH_MV_REPUBLISH_LIT                    "RP"
#define COM_PUBLISH_MV_TOUCH_LIT                        "MT"
#define COM_PUBLISH_MV_UNKNOWN_LIT                      "  "


//-- MVS
//----------------------------------------------------------------------------
// -- Histograms

enum ComHistReasonType {  COM_HIST_NOT_CREATED
                         ,COM_HIST_MANUAL
                         ,COM_HIST_INITIAL
                         ,COM_HIST_AUTO_REGEN_NEEDED
                       };

#define COM_HIST_NOT_CREATED_LIT         " "
#define COM_HIST_MANUAL_LIT              "M"
#define COM_HIST_INITIAL_LIT             "I"
#define COM_HIST_AUTO_REGEN_NEEDED_LIT   "N"


//-- Histograms
//---------------------------------------------------------------------------
//----------------------------------------------------------------------------

// NOTE: the following literals get generated into plans, don't change them
// (adding new ones is ok)
//
// COM_SQL_MP_NAME, COM_SQL_MP_USER_TABLE_NAME, COM_SQL_MP_SYSTEM_TABLE_NAME,
// COM_SQL_MP_INDEX_NAME, COM_SQL_MP_PVIEW_NAME, COM_SQL_MP_SVIEW_NAME and
// the corresponding literals ie. COM_SQL_MP_NAME_LIT,
// COM_SQL_MP_USER_TABLE_NAME_LIT, ... are not actually used even though
// w:\smdio\CmUtil.cpp refers to these variables.
//
// In Release 2, MP alias resides in the table name space.

// COM_UDR_NAME is not used as name space for procedure.  Valid name space
// value is COM_TABLE_NAME, TA.  However, COM_UDR_NAME is referenced by
// sqlparser.y, CmpDescribe.cpp for showddl procedure work.  It is also
// referenced by to properly report error in case users has not fixed up their
// database and still have UR name space value in the Objects table for
// procedures/routines.

enum ComAnsiNameSpace { COM_UNKNOWN_NAME              =  0
                      , COM_CONSTRAINT_NAME           =  1
                      , COM_INDEX_NAME                =  2
                      , COM_MODULE_NAME               =  3
                      , COM_TABLE_NAME                =  4
                      , COM_SQL_MP_NAME               =  5
                      , COM_LOCK_NAME                 =  6
                      , COM_SQL_MP_USER_TABLE_NAME    =  7
                      , COM_SQL_MP_SYSTEM_TABLE_NAME  =  8
                      , COM_SQL_MP_INDEX_NAME         =  9
                      , COM_SQL_MP_PVIEW_NAME         = 10
                      , COM_SQL_MP_SVIEW_NAME         = 11
                      , COM_TRIGTEMP_TABLE_NAME       = 12
                      , COM_TRIGGER_NAME              = 13
                      , COM_IUD_LOG_TABLE_NAME        = 14
                      , COM_RANGE_LOG_TABLE_NAME      = 15
                      , COM_MVRG_NAME                 = 16
                      , COM_UDR_NAME                  = 17
                      , COM_LOB_TABLE_NAME            = 18
                      , COM_SCHEMA_LABEL_NAME         = 19
                      , COM_EXCEPTION_TABLE_NAME      = 20
                      , COM_GHOST_TABLE_NAME          = 21
                      , COM_GHOST_INDEX_NAME          = 22
                      , COM_GHOST_IUD_LOG_TABLE_NAME  = 23
                      , COM_SEQUENCE_GENERATOR_NAME   = 24
                      , COM_UDF_NAME                  = 25
                      , COM_UUDF_ACTION_NAME          = 26
                      , COM_LIBRARY_NAME              = 27
                      };

#define COM_NAME_LIT_LEN                        2
#define COM_UNKNOWN_NAME_LIT                    "  "
#define COM_CONSTRAINT_NAME_LIT                 "CN"
#define COM_INDEX_NAME_LIT                      "IX"
#define COM_IUD_LOG_TABLE_NAME_LIT              "IL"  // MV
#define COM_RANGE_LOG_TABLE_NAME_LIT            "RL"
#define COM_MVRG_NAME_LIT                       "RG"   // OZ
#define COM_TRIGGER_NAME_LIT                    "TR"
#define COM_MODULE_NAME_LIT                     "MD"
#define COM_TABLE_NAME_LIT                      "TA"
#define COM_TRIGTEMP_TABLE_NAME_LIT             "TT"
#define COM_UDR_NAME_LIT                        "UR"
#define COM_LOB_TABLE_NAME_LIT                  "LO"
#define COM_SCHEMA_LABEL_NAME_LIT               "SL"
#define COM_EXCEPTION_TABLE_NAME_LIT            "EX"
#define COM_GHOST_TABLE_NAME_LIT                "GT"
#define COM_GHOST_INDEX_NAME_LIT                "GI"
#define COM_GHOST_IUD_LOG_TABLE_NAME_LIT        "GG"
#define COM_SEQUENCE_GENERATOR_NAME_LIT         "SG"
#define COM_UDF_NAME_LIT                        "UF"
#define COM_UUDF_ACTION_NAME_LIT                "AC"
#define COM_LIBRARY_NAME_LIT                    "LB"

// These are not used even though smdio\CmUtil.cpp references them.
// They are put here as place holders.
#define COM_SQL_MP_NAME_LIT                     "  "
#define COM_SQL_MP_USER_TABLE_NAME_LIT          "  "
#define COM_SQL_MP_SYSTEM_TABLE_NAME_LIT        "  "
#define COM_SQL_MP_INDEX_NAME_LIT               "  "
#define COM_SQL_MP_PVIEW_NAME_LIT               "  "
#define COM_SQL_MP_SVIEW_NAME_LIT               "  "
#define COM_LOCK_NAME_LIT                       "LK"

enum ComClusteringScheme { COM_UNKNOWN_CLUSTERING
                         , COM_KEY_SEQ_CLUSTERING
                         , COM_ENTRY_SEQ_CLUSTERING
                         , COM_RELATIVE_CLUSTERING
                         };

#define COM_CLUSTERING_LIT_LEN              2
#define COM_UNKNOWN_CLUSTERING_LIT          "  "
#define COM_KEY_SEQ_CLUSTERING_LIT          "KS"
#define COM_ENTRY_SEQ_CLUSTERING_LIT        "ES"

#define COM_MAXIMUM_NUMBER_OF_COLUMNS       20000
#define COM_DIV_EXPR_BASE_TEXT_SUBID        20000

#define COM_MAXIMUM_LENGTH_OF_COMMENT       1000

enum ComColumnClass { COM_UNKNOWN_CLASS
                    , COM_SYSTEM_COLUMN
                    , COM_USER_COLUMN
                    , COM_ADDED_USER_COLUMN
                    , COM_MV_SYSTEM_ADDED_COLUMN
                    , COM_ALTERED_USER_COLUMN
                    };

#define COM_UNKNOWN_CLASS_LIT               "  "
#define COM_SYSTEM_COLUMN_LIT               "S "
#define COM_USER_COLUMN_LIT                 "U "
#define COM_ADDED_USER_COLUMN_LIT           "A "
#define COM_MV_SYSTEM_ADDED_COLUMN_LIT      "M "
#define COM_ALTERED_USER_COLUMN_LIT         "C "

/* This enum will be saved as integer in metadata tables
 * If you change it, that will affect the existing values
 * Make sure to add new values at the end
 */
enum ComColumnDefaultClass { COM_CURRENT_DEFAULT = 0
                           , COM_NO_DEFAULT = 1
                           , COM_NULL_DEFAULT = 2
                           , COM_USER_DEFINED_DEFAULT = 3
                           , COM_USER_FUNCTION_DEFAULT = 4
                           , COM_IDENTITY_GENERATED_BY_DEFAULT = 5
                           , COM_IDENTITY_GENERATED_ALWAYS =6
                           , COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT = 7
                           , COM_ALWAYS_DEFAULT_COMPUTED_COLUMN_DEFAULT = 8
                           , COM_UUID_DEFAULT = 9 
                           , COM_CURRENT_UT_DEFAULT = 10
                           , COM_FUNCTION_DEFINED_DEFAULT = 11
                           };

#define COM_CURRENT_DEFAULT_LIT                     "CD"
#define COM_CURRENT_UT_DEFAULT_LIT                  "UT"
#define COM_FUNCTION_DEFINED_DEFAULT_LIT            "FD"
#define COM_NO_DEFAULT_LIT                          "  "
#define COM_NULL_DEFAULT_LIT                        "ND"
#define COM_USER_DEFINED_DEFAULT_LIT                "UD"
#define COM_USER_FUNCTION_DEFAULT_LIT               "UF"
#define COM_UUID_DEFAULT_LIT                        "UI"
#define COM_IDENTITY_GENERATED_BY_DEFAULT_LIT       "ID"
#define COM_IDENTITY_GENERATED_ALWAYS_LIT           "IA"
#define COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT_LIT "AC"
#define COM_ALWAYS_DEFAULT_COMPUTED_COLUMN_DEFAULT_LIT "AD"

/* This enum will be saved as integer in metadata tables
 * If you change it, that will affect the existing values
 * Make sure to add new values at the end
 */
enum ComParamDefaultClass { COM_CURRENT_PARAM_DEFAULT        = COM_CURRENT_DEFAULT
                          , COM_NO_PARAM_DEFAULT 
                                                             = COM_NO_DEFAULT
                          , COM_NULL_PARAM_DEFAULT           = COM_NULL_DEFAULT
                          , COM_USER_DEFINED_PARAM_DEFAULT   = COM_USER_DEFINED_DEFAULT
                          , COM_USER_FUNCTION_PARAM_DEFAULT  = COM_USER_FUNCTION_DEFAULT
                            // IDENTITY GENERATED BY DEFAULT not applicable
                            // IDENTITY GENERATED ALWAYS     not applicable
                          , COM_ALWAYS_COMPUTE_COMPUTED_PARAM_DEFAULT // for future internal use only
                                                             = COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT
                          , COM_ALWAYS_DEFAULT_COMPUTED_PARAM_DEFAULT // for future internal use only
                                                             = COM_ALWAYS_DEFAULT_COMPUTED_COLUMN_DEFAULT
                          , COM_UUID_PARAM_DEAULT            = COM_UUID_DEFAULT
                          , COM_CURRENT_UT_PARAM_DEFAULT     = COM_CURRENT_UT_DEFAULT
                          , COM_FUNCTION_DEFINED_PARAM_DEFAULT
                                                             = COM_FUNCTION_DEFINED_DEFAULT
                          };

#define COM_NO_PARAM_DEFAULT_LIT                      "  "
#define COM_CURRENT_PARAM_DEFAULT_LIT                 "CD"
#define COM_CURRENT_UT_PARAM_DEFAULT_LIT              "UT"
#define COM_NULL_PARAM_DEFAULT_LIT                    "ND"
#define COM_USER_DEFINED_PARAM_DEFAULT_LIT            "UD"
#define COM_USER_FUNCTION_PARAM_DEFAULT_LIT           "UF"
#define COM_ALWAYS_COMPUTE_COMPUTED_PARAM_DEFAULT_LIT "AC"
#define COM_ALWAYS_DEFAULT_COMPUTED_PARAM_DEFAULT_LIT "AD"
#define COM_FUNCTION_DEFINED_PARAM_DEFAULT_LIT        "FD"

// Represents the kind of string value stored in TEXT table.  Note
// that changing existing values will require an UPGRADE of the
// metadata.
enum ComTextType {COM_VIEW_TEXT = 0,
                  COM_CHECK_CONSTR_TEXT = 1,
                  COM_HBASE_OPTIONS_TEXT = 2,
                  COM_TABLE_COMMENT_TEXT = 3,
                  COM_COMPUTED_COL_TEXT = 4,
                  COM_HBASE_COL_FAMILY_TEXT = 5,
                  COM_HBASE_SPLIT_TEXT = 6,
                  COM_STORED_DESC_TEXT = 7,
                  COM_VIEW_REF_COLS_TEXT = 8,
                  COM_OBJECT_COMMENT_TEXT = COM_TABLE_COMMENT_TEXT,
                  COM_COLUMN_COMMENT_TEXT = 12
};

enum ComColumnDirection { COM_UNKNOWN_DIRECTION
                        , COM_INPUT_COLUMN
                        , COM_OUTPUT_COLUMN
                        , COM_INOUT_COLUMN
                        };

#define COM_UNKNOWN_DIRECTION_LIT        "  "
#define COM_INPUT_COLUMN_LIT             "I "
#define COM_OUTPUT_COLUMN_LIT            "O "
#define COM_INOUT_COLUMN_LIT             "N "

enum ComParamDirection { COM_UNKNOWN_PARAM_DIRECTION = COM_UNKNOWN_DIRECTION
                       , COM_INPUT_PARAM             = COM_INPUT_COLUMN
                       , COM_OUTPUT_PARAM            = COM_OUTPUT_COLUMN
                       , COM_INOUT_PARAM             = COM_INOUT_COLUMN
                       };

#define COM_UNKNOWN_PARAM_DIRECTION_LIT  "  "
#define COM_INPUT_PARAM_LIT              "I "
#define COM_OUTPUT_PARAM_LIT             "O "
#define COM_INOUT_PARAM_LIT              "N "

enum ComStoreByDetails { COM_STOREBY_DETAILS_UNKNOWN
                       , COM_STOREBY_DETAILS_V1
                       , COM_STOREBY_DETAILS_V2
                       };

#define COM_STOREBY_DETAILS_UNKNOWN_LIT  "  "
#define COM_STOREBY_DETAILS_V1_LIT       "V1"
#define COM_STOREBY_DETAILS_V2_LIT       "V2"

// The enum ComCompressionType  are used in 
// sqlutils/mxtool/replicate_schema_ddl.java
// Sync the changes with the java file also

enum ComCompressionType { COM_NO_COMPRESSION
                        , COM_SOFTWARE_COMPRESSION
                        , COM_HARDWARE_COMPRESSION
                        , COM_UNKNOWN_COMPRESSION
                        , COM_SYSTEM_COMPRESSION
                        , COM_SOURCE_COMPRESSION 
                        };

#define  COM_UNKNOWN_COMPRESSION_LIT            "  "
#define  COM_NO_COMPRESSION_LIT                 "N "
#define  COM_HARDWARE_COMPRESSION_LIT           "H "
#define  COM_SOFTWARE_COMPRESSION_LIT           "S "

enum ComColumnOrdering { COM_UNKNOWN_ORDER
                       , COM_ASCENDING_ORDER
                       , COM_DESCENDING_ORDER
                       };

#define COM_UNKNOWN_ORDER_LIT             "  "
#define COM_ASCENDING_ORDER_LIT           "A "
#define COM_DESCENDING_ORDER_LIT          "D "

enum ComRoutineParamType { COM_NORMAL_PARAM_TYPE
                         , COM_SAS_PUT_FORMAT_NAME_PARAM_TYPE
                         , COM_SAS_PUT_LOCALE_ID_PARAM_TYPE
                         , COM_SAS_SCORE_TABLE_NAME_PARAM_TYPE
                         };

#define COM_NORMAL_PARAM_TYPE_LIT                    "  "
#define COM_SAS_PUT_FORMAT_NAME_PARAM_TYPE_LIT       "SF"
#define COM_SAS_PUT_LOCALE_ID_PARAM_TYPE_LIT         "SL"
#define COM_SAS_SCORE_TABLE_NAME_PARAM_TYPE_LIT      "SM"

enum ComConstraintType { COM_UNKNOWN_CONSTRAINT
                       , COM_CHECK_CONSTRAINT
                       , COM_FOREIGN_KEY_CONSTRAINT
                       , COM_PRIMARY_KEY_CONSTRAINT
                       , COM_UNIQUE_CONSTRAINT
                       };

#define COM_CHECK_CONSTRAINT_LIT                  "C "
#define COM_FOREIGN_KEY_CONSTRAINT_LIT            "F "
#define COM_PRIMARY_KEY_CONSTRAINT_LIT            "P "
#define COM_UNIQUE_CONSTRAINT_LIT                 "U "
#define COM_UNKNOWN_CONSTRAINT_LIT                "  "

enum ComObjectClass { COM_CLASS_USER_METADATA
                    , COM_CLASS_MV_UMD
                    , COM_CLASS_USER_TABLE
                    , COM_CLASS_SYSTEM_METADATA
                    , COM_CLASS_UNKNOWN
                    , COM_CLASS_SYSTEM_TABLE
                    };

#define COM_CLASS_LIT_LEN                          2
#define COM_CLASS_USER_METADATA_LIT                "UM"
#define COM_CLASS_MV_UMD_LIT                       "MU"
#define COM_CLASS_USER_TABLE_LIT                   "UT"
#define COM_CLASS_SYSTEM_METADATA_LIT              "SM"
#define COM_CLASS_SYSTEM_TABLE_LIT                 "ST"
#define COM_CLASS_UNKNOWN_LIT                      "  "


enum ComComponentPrivilegeClass { COM_INTERNAL_COMPONENT_PRIVILEGE  =0
                            , COM_EXTERNAL_COMPONENT_PRIVILEGE
                            };

#define COM_INTERNAL_COMPONENT_PRIVILEGE_LIT "IN"
#define COM_EXTERNAL_COMPONENT_PRIVILEGE_LIT "EX"

// Values are identical to 'enum rec_datetime_field' in dfs2rec.h
enum ComDateTimeStartEnd { COM_DTSE_UNKNOWN = 0
                         , COM_DTSE_YEAR    = 1
                         , COM_DTSE_MONTH
                         , COM_DTSE_DAY
                         , COM_DTSE_HOUR
                         , COM_DTSE_MINUTE
                         , COM_DTSE_SECOND
                         , COM_DTSE_FRACTION    // used only in MP, not ARK!
                         };

enum ComCreateViewBehavior { COM_CREATE_VIEW_BEHAVIOR
                     , COM_CREATE_OR_REPLACE_VIEW_BEHAVIOR
                     , COM_CREATE_OR_REPLACE_VIEW_CASCADE_BEHAVIOR
		     , COM_CREATE_SYSTEM_VIEW_BEHAVIOR
                     };

enum ComDropBehavior { COM_UNKNOWN_DROP_BEHAVIOR
                     , COM_CASCADE_DROP_BEHAVIOR
                     , COM_RESTRICT_DROP_BEHAVIOR
		     , COM_NO_CHECK_DROP_BEHAVIOR
		     , COM_CASCADE_INVALIDATE_DEPENDENT_BEHAVIOR
                     };

enum ComDropType     { COM_DROP_SINGLE
                     , COM_DROP_ALL
                     };

enum ComLibraryPathType  { COM_LIBRARY_PATH_FULL = 2
                         , COM_LIBRARY_PATH_PARTIAL
                         };

enum ComRegisterBehavior { COM_UNKNOWN_REGISTER_BEHAVIOR
                         , COM_CASCADE_REGISTER_BEHAVIOR
                         , COM_RESTRICT_REGISTER_BEHAVIOR
                         };

enum ComUnregisterBehavior { COM_UNKNOWN_UNREGISTER_BEHAVIOR
                           , COM_CASCADE_UNREGISTER_BEHAVIOR
                           , COM_RESTRICT_UNREGISTER_BEHAVIOR
                           };

// Values are identical to '#define REC_xxx value' in dfs2rec.h
enum ComFSDataType { COM_UNKNOWN_FSDT              = -1
                   , COM_FCHAR_FSDT                = REC_BYTE_F_ASCII
                   , COM_FCHAR_DBL_FSDT            = REC_BYTE_F_DOUBLE
                   , COM_VCHAR_FSDT                = REC_BYTE_V_ASCII
                   , COM_VCHAR_DBL_FSDT            = REC_BYTE_V_DOUBLE
                   , COM_VCHAR_LONG_FSDT           = REC_BYTE_V_ASCII_LONG
                   , COM_SIGNED_BIN8_FSDT          = REC_BIN8_SIGNED
                   , COM_UNSIGNED_BIN8_FSDT        = REC_BIN8_UNSIGNED
                   , COM_SIGNED_BIN16_FSDT         = REC_BIN16_SIGNED
                   , COM_UNSIGNED_BIN16_FSDT       = REC_BIN16_UNSIGNED
                   , COM_SIGNED_BIN32_FSDT         = REC_BIN32_SIGNED
                   , COM_UNSIGNED_BIN32_FSDT       = REC_BIN32_UNSIGNED
                   , COM_SIGNED_BIN64_FSDT         = REC_BIN64_SIGNED
                   , COM_UNSIGNED_BPINT_FSDT       = REC_BPINT_UNSIGNED
                   , COM_FLOAT32_FSDT              = REC_FLOAT32
                   , COM_FLOAT64_FSDT              = REC_FLOAT64
                   , COM_UNSIGNED_DECIMAL_FSDT     = REC_DECIMAL_UNSIGNED
                   , COM_SIGNED_DECIMAL_FSDT       = REC_DECIMAL_LSE
                   , COM_SIGNED_NUM_BIG_FSDT       = REC_NUM_BIG_SIGNED
                   , COM_UNSIGNED_NUM_BIG_FSDT     = REC_NUM_BIG_UNSIGNED

		   , COM_BLOB                      = REC_BLOB
		   , COM_CLOB                      = REC_CLOB

                   , COM_BOOLEAN                   = REC_BOOLEAN

                   , COM_DATETIME_FSDT             = REC_DATETIME
                   , COM_INTERVAL_MIN_FSDT         = REC_MIN_INTERVAL
                   , COM_INTERVAL_YEAR_YEAR_FSDT   = REC_INT_YEAR
                   , COM_INTERVAL_MON_MON_FSDT     = REC_INT_MONTH
                   , COM_INTERVAL_YEAR_MON_FSDT    = REC_INT_YEAR_MONTH
                   , COM_INTERVAL_DAY_DAY_FSDT     = REC_INT_DAY
                   , COM_INTERVAL_HOUR_HOUR_FSDT   = REC_INT_HOUR
                   , COM_INTERVAL_DAY_HOUR_FSDT    = REC_INT_DAY_HOUR
                   , COM_INTERVAL_MIN_MIN_FSDT     = REC_INT_MINUTE
                   , COM_INTERVAL_HOUR_MIN_FSDT    = REC_INT_HOUR_MINUTE
                   , COM_INTERVAL_DAY_MIN_FSDT     = REC_INT_DAY_MINUTE
                   , COM_INTERVAL_SEC_SEC_FSDT     = REC_INT_SECOND
                   , COM_INTERVAL_MIN_SEC_FSDT     = REC_INT_MINUTE_SECOND
                   , COM_INTERVAL_HOUR_SEC_FSDT    = REC_INT_HOUR_SECOND
                   , COM_INTERVAL_DAY_SEC_FSDT     = REC_INT_DAY_SECOND
                   , COM_INTERVAL_MAX_FSDT         = REC_MAX_INTERVAL
                   , COM_LAST_FSDT                              // last value
                   };

// TBD: clean these grantee/grantor types 
enum ComGranteeType { COM_UNKNOWN_GRANTEE_TYPE
                    , COM_PUBLIC_GRANTEE
                    , COM_USER_GRANTEE
// Can get rid of schema owner and possibly "any"
                    , COM_SCHEMA_OWNER_GRANTEE
                    , COM_ANY_GRANTEE
                    };
// There is no corresponding *_LIT value for COM_ANY_GRANTEE as it is
// only used in matching; it is never stored in metadata.
#define COM_UNKNOWN_GRANTEE_TYPE_LIT               "  "
#define COM_PUBLIC_GRANTEE_LIT                     "P "
#define COM_USER_GRANTEE_LIT                       "U "
// Can get rid of schema owner
#define COM_SCHEMA_OWNER_GRANTEE_LIT               "O "

enum ComIdClass { COM_UNKNOWN_ID_CLASS
                , COM_ROLE_CLASS
                , COM_USER_CLASS
                };

#define COM_UNKNOWN_ID_CLASS_LIT       "  "
#define COM_ROLE_CLASS_LIT             "R "
#define COM_USER_CLASS_LIT             "U "

enum ComIdStatus { COM_UNKNOWN_ID_STATUS
                 , COM_AVAILABLE_ID_STATUS
                 , COM_USED_ID_STATUS
                 , COM_PROTECTED_ID_STATUS
                 };

#define COM_UNKNOWN_ID_STATUS_LIT       "  "
#define COM_AVAILABLE_ID_STATUS_LIT     "A "
#define COM_USED_ID_STATUS_LIT          "U "
#define COM_PROTECTED_ID_STATUS_LIT     "P "

/* TBD: can remove
enum ComGranteeClass { COM_UNKNOWN_GRANTEE_CLASS
                     , COM_ROLE_CLASS
                     , COM_LDAP_GROUP_CLASS
                     , COM_USER_CLASS
                     };

#define COM_UNKNOWN_GRANTEE_CLASS_LIT          "  "
#define COM_ROLE_CLASS_LIT                     "R "
#define COM_LDAP_GROUP_CLASS_LIT               "G "
#define COM_USER_CLASS_LIT                     "U "

enum ComGrantorClass { COM_UNKNOWN_GRANTOR_CLASS
                     , COM_ROLE_CLASS
                     , COM_LDAP_GROUP_CLASS
                     , COM_USER_CLASS
                     };

#define COM_UNKNOWN_GRANTOR_CLASS_LIT          "  "
*/

enum ComGrantorType { COM_UNKNOWN_GRANTOR_TYPE
                    , COM_SYSTEM_GRANTOR
                    , COM_USER_GRANTOR
// can get rid of these next 2: owner and any
                    , COM_SCHEMA_OWNER_GRANTOR
                    , COM_ANY_GRANTOR
                    };

// There is no corresponding *_LIT value for COM_ANY_GRANTOR as it is
// only used in matching; it is never stored in metadata.
#define COM_UNKNOWN_GRANTOR_TYPE_LIT               "  "
#define COM_SYSTEM_GRANTOR_LIT                     "S "
#define COM_USER_GRANTOR_LIT                       "U "
// can get rid of schema owner
#define COM_SCHEMA_OWNER_GRANTOR_LIT               "O "

enum ComRoleIdStatus { COM_UNKNOWN_STATUS
                     , COM_AVAILABLE_STATUS
                     , COM_USED_STATUS
                     };

#define COM_UNKNOWN_STATUS_LIT               "  "
#define COM_AVAILABLE_STATUS_LIT             "A "
#define COM_USED_STATUS_LIT                  "U "

enum ComLevels { COM_UNKNOWN_LEVEL
               , COM_CASCADED_LEVEL
               , COM_LOCAL_LEVEL
               };

// For any new object type, add a define for the corresponding string literal
// below, and a case in comObjectTypeLit().
enum ComObjectType { COM_UNKNOWN_OBJECT
                   , COM_BASE_TABLE_OBJECT
                   , COM_CHECK_CONSTRAINT_OBJECT
                   , COM_INDEX_OBJECT
                   , COM_LIBRARY_OBJECT
                   , COM_LOCK_OBJECT
                   , COM_MODULE_OBJECT
                   , COM_NOT_NULL_CONSTRAINT_OBJECT
                   , COM_PRIMARY_KEY_CONSTRAINT_OBJECT
                   , COM_REFERENTIAL_CONSTRAINT_OBJECT
                   , COM_STORED_PROCEDURE_OBJECT
                   , COM_UNIQUE_CONSTRAINT_OBJECT
                   , COM_USER_DEFINED_ROUTINE_OBJECT
                   , COM_VIEW_OBJECT
                   , COM_MV_OBJECT   // MV
                   , COM_MVRG_OBJECT    // OZ
                   , COM_TRIGGER_OBJECT
                   , COM_LOB_TABLE_OBJECT
                   , COM_TRIGGER_TABLE_OBJECT
                   , COM_SYNONYM_OBJECT
                   , COM_PRIVATE_SCHEMA_OBJECT
                   , COM_SHARED_SCHEMA_OBJECT
                   , COM_EXCEPTION_TABLE_OBJECT
                   , COM_SEQUENCE_GENERATOR_OBJECT
                   };
                   
// OBJECT_LIT values are also used by COM_AUD* defines below
#define COM_OBJECT_LIT_LEN                      2
#define COM_UNKNOWN_OBJECT_LIT                  "  "
#define COM_BASE_TABLE_OBJECT_LIT               "BT"
#define COM_CHECK_CONSTRAINT_OBJECT_LIT         "CC"
#define COM_INDEX_OBJECT_LIT                    "IX"
#define COM_LIBRARY_OBJECT_LIT                  "LB"
#define COM_LOCK_OBJECT_LIT                     "LK"
#define COM_MODULE_OBJECT_LIT                   "MD"
#define COM_NOT_NULL_CONSTRAINT_OBJECT_LIT      "NN"
#define COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT   "PK"
#define COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT   "RC"
#define COM_STORED_PROCEDURE_OBJECT_LIT         "SP"
#define COM_UNIQUE_CONSTRAINT_OBJECT_LIT        "UC"
#define COM_USER_DEFINED_ROUTINE_OBJECT_LIT     "UR"
#define COM_VIEW_OBJECT_LIT                     "VI"
#define COM_MV_OBJECT_LIT                       "MV"
#define COM_MVRG_OBJECT_LIT                     "RG"  // OZ
#define COM_TRIGGER_OBJECT_LIT                  "TR"
#define COM_LOB_TABLE_OBJECT_LIT                "LT"
#define COM_TRIGGER_TABLE_OBJECT_LIT            "TT"
#define COM_SYNONYM_OBJECT_LIT                  "SY"
#define COM_SHARED_SCHEMA_OBJECT_LIT            "SS"
#define COM_PRIVATE_SCHEMA_OBJECT_LIT           "PS"
#define COM_EXCEPTION_TABLE_OBJECT_LIT          "EX"
#define COM_SEQUENCE_GENERATOR_OBJECT_LIT       "SG"

// This enum has a similar one, SG_IE_TYPE, in parser/ElemDDLSGOptions.h
// Should keep them in sync.
enum ComSequenceGeneratorType { COM_UNKNOWN_SG
                              , COM_INTERNAL_SG
                              , COM_EXTERNAL_SG
                              , COM_INTERNAL_COMPUTED_SG
                              };

#define COM_UNKNOWN_SG_LIT  "  "
#define COM_INTERNAL_SG_LIT "I "
#define COM_EXTERNAL_SG_LIT "E "
#define COM_INTERNAL_COMPUTED_SG_LIT "C "


enum ComODBCDataType { COM_UNKNOWN_ODT
                     , COM_CHARACTER_ODT
                     , COM_VARCHAR_ODT
                     , COM_LONG_VARCHAR_ODT
                     , COM_NUMERIC_SIGNED_ODT
                     , COM_NUMERIC_UNSIGNED_ODT
                     , COM_TINYINT_SIGNED_ODT
                     , COM_TINYINT_UNSIGNED_ODT
                     , COM_SMALLINT_SIGNED_ODT
                     , COM_SMALLINT_UNSIGNED_ODT
                     , COM_INTEGER_SIGNED_ODT
                     , COM_INTEGER_UNSIGNED_ODT
                     , COM_LARGEINT_SIGNED_ODT
                     , COM_LARGEINT_UNSIGNED_ODT
                     , COM_BIGINT_SIGNED_ODT
                     , COM_FLOAT_ODT
                     , COM_REAL_ODT
                     , COM_DOUBLE_ODT
                     , COM_DECIMAL_SIGNED_ODT
                     , COM_DECIMAL_UNSIGNED_ODT
                     , COM_LARGE_DECIMAL_SIGNED_ODT
                     , COM_LARGE_DECIMAL_UNSIGNED_ODT
		     , COM_BLOB_ODT
		     , COM_CLOB_ODT
		     , COM_BOOLEAN_ODT
                     , COM_DATETIME_ODT
                     , COM_TIMESTAMP_ODT
                     , COM_DATE_ODT
                     , COM_TIME_ODT
                     , COM_INTERVAL_ODT
                     };

#define COM_UNKNOWN_ODT_LIT                    "                  "
#define COM_CHARACTER_ODT_LIT                  "CHARACTER         "
#define COM_VARCHAR_ODT_LIT                    "VARCHAR           "
#define COM_LONG_VARCHAR_ODT_LIT               "LONG VARCHAR      "
#define COM_NUMERIC_SIGNED_ODT_LIT             "SIGNED NUMERIC    "
#define COM_NUMERIC_UNSIGNED_ODT_LIT           "UNSIGNED NUMERIC  "
#define COM_TINYINT_SIGNED_ODT_LIT             "SIGNED TINYINT    "
#define COM_TINYINT_UNSIGNED_ODT_LIT           "UNSIGNED TINYINT  "
#define COM_SMALLINT_SIGNED_ODT_LIT            "SIGNED SMALLINT   "
#define COM_SMALLINT_UNSIGNED_ODT_LIT          "UNSIGNED SMALLINT "
#define COM_INTEGER_SIGNED_ODT_LIT             "SIGNED INTEGER    "
#define COM_INTEGER_UNSIGNED_ODT_LIT           "UNSIGNED INTEGER  "
#define COM_LARGEINT_SIGNED_ODT_LIT            "SIGNED LARGEINT   "
#define COM_LARGEINT_UNSIGNED_ODT_LIT          "UNSIGNED LARGEINT "
#define COM_BIGINT_SIGNED_ODT_LIT              "SIGNED BIGINT     "
#define COM_FLOAT_ODT_LIT                      "FLOAT             "
#define COM_REAL_ODT_LIT                       "REAL              "
#define COM_DOUBLE_ODT_LIT                     "DOUBLE            "
#define COM_DECIMAL_SIGNED_ODT_LIT             "SIGNED DECIMAL    "
#define COM_DECIMAL_UNSIGNED_ODT_LIT           "UNSIGNED DECIMAL  "
#define COM_DATETIME_ODT_LIT                   "DATETIME          "
#define COM_TIMESTAMP_ODT_LIT                  "TIMESTAMP         "
#define COM_DATE_ODT_LIT                       "DATE              "
#define COM_TIME_ODT_LIT                       "TIME              "
#define COM_INTERVAL_ODT_LIT                   "INTERVAL          "
#define COM_BLOB_ODT_LIT                       "BLOB              "
#define COM_CLOB_ODT_LIT                       "CLOB              "
#define COM_BOOLEAN_ODT_LIT                    "BOOLEAN           "

enum ComAccessPathType  { COM_UNKNOWN_ACCESS_PATH_TYPE
                        , COM_BASE_TABLE_TYPE
                        , COM_INDEX_TYPE
                        , COM_LOB_TABLE_TYPE
                        };

#define COM_ACCESS_PATH_TYPE_LIT_LEN                  2
#define COM_UNKNOWN_ACCESS_PATH_TYPE_LIT              "  "
#define COM_BASE_TABLE_TYPE_LIT                       "BT"
#define COM_INDEX_TYPE_LIT                            "IX"
#define COM_LOB_TABLE_TYPE_LIT                        "LT"


enum ComPartitioningScheme { COM_UNSPECIFIED_PARTITIONING
                           , COM_NO_PARTITIONING
                           , COM_SINGLE_PARTITIONING = COM_NO_PARTITIONING
                           , COM_RANGE_PARTITIONING
                           , COM_SYSTEM_PARTITIONING
                           , COM_ROUND_ROBIN_PARTITIONING
                           , COM_HASH_V1_PARTITIONING
                           , COM_HASH_V2_PARTITIONING
                           , COM_UNKNOWN_PARTITIONING
                           };

#define COM_PARTITIONING_LIT_LEN             2
#define COM_UNSPECIFIED_PARTITIONING_LIT     "  "
#define COM_NO_PARTITIONING_LIT              "N "
#define COM_SINGLE_PARTITIONING_LIT          COM_NO_PARTITIONING_LIT
#define COM_RANGE_PARTITIONING_LIT           "RP"
#define COM_SYSTEM_PARTITIONING_LIT          "SP"
#define COM_ROUND_ROBIN_PARTITIONING_LIT     "RR"
#define COM_HASH_V1_PARTITIONING_LIT         "HP"
#define COM_HASH_V2_PARTITIONING_LIT         "H2"
#define COM_UNKNOWN_PARTITIONING_LIT         COM_UNSPECIFIED_PARTITIONING_LIT


// aligned_format:   All columns are stored in an internal row aligned format,
//                   and the whole row is stored in one hbase cell.
// hbase_format:     each col is stored as one hbase cell with data in native
//                   format (for ex: INT datatype is stored as 4-byte integer)
// hbase_str_format: same as hbase_format except data is stored in displayable
//                   string format(for ex: 100 is stored as string '100')
enum ComRowFormat {  COM_UNKNOWN_FORMAT_TYPE
                     , COM_ALIGNED_FORMAT_TYPE
                     , COM_HBASE_FORMAT_TYPE
                     , COM_HBASE_STR_FORMAT_TYPE
};

#define COM_ROWFORMAT_LIT_LEN                2
#define COM_UNKNOWN_FORMAT_LIT               "  "
#define COM_ALIGNED_FORMAT_LIT               "AF"
#define COM_HBASE_FORMAT_LIT                 "HF"
#define COM_HBASE_STR_FORMAT_LIT             "HS"

// table load action: regular, SET or MULTISET.
// Regular:  will error out if duplicate key is inserted.
// SET:      will silently ignore duplicate rows on insert.
//           Error will not be returned.
// MULTISET: will allow duplicate rows by adding a SYSKEY.
//           This option not currently supported.
enum ComInsertMode  { COM_UNKNOWN_TABLE_INSERT_MODE,
                      COM_REGULAR_TABLE_INSERT_MODE    // reject dups
                    , COM_SET_TABLE_INSERT_MODE        // discard/ignore dups
                    , COM_MULTISET_TABLE_INSERT_MODE   // allow dups
                    };
#define COM_INSERT_MODE_LIT_LEN                           2
#define COM_UNKNOWN_TABLE_INSERT_MODE_LIT                 "  "
#define COM_REGULAR_TABLE_INSERT_MODE_LIT                 "RD"
#define COM_SET_TABLE_INSERT_MODE_LIT                     "DD"
#define COM_MULTISET_TABLE_INSERT_MODE_LIT                "AD"

// The order of ComPrivilegeType matters.  As new privileges begin to be used
// they should be added after privileges that are already used.  This is so we
// can have code that can check if a privilege is higher than what a certain
// schema version can understand.
enum ComPrivilegeType { COM_UNKNOWN_PRIVILEGE = 0   // must always be first
// Privileges used in schema v2000
                      , COM_SELECT_PRIVILEGE
                      , COM_INSERT_PRIVILEGE
                      , COM_DELETE_PRIVILEGE
                      , COM_UPDATE_PRIVILEGE
                      , COM_USAGE_PRIVILEGE
                      , COM_REFERENCE_PRIVILEGE
                      , COM_EXECUTE_PRIVILEGE
                      , COM_DATABASE_ADMINISTRATOR_PRIVILEGE
// Privileges used in schema v2300
                      , COM_CREATE_PRIVILEGE
                      , COM_CREATE_TABLE_PRIVILEGE
                      , COM_CREATE_VIEW_PRIVILEGE
                      , COM_ALTER_PRIVILEGE
                      , COM_ALTER_TABLE_PRIVILEGE
// Privileges implemented but not externalized
                      , COM_CREATE_LIBRARY_PRIVILEGE
                      , COM_CREATE_MV_PRIVILEGE
                      , COM_CREATE_PROCEDURE_PRIVILEGE
                      , COM_CREATE_SYNONYM_PRIVILEGE
                      , COM_CREATE_TRIGGER_PRIVILEGE
                      , COM_CREATE_MV_GROUP_PRIVILEGE
                      , COM_ALTER_LIBRARY_PRIVILEGE
                      , COM_ALTER_MV_PRIVILEGE
                      , COM_ALTER_SYNONYM_PRIVILEGE
                      , COM_ALTER_MV_GROUP_PRIVILEGE
                      , COM_ALTER_TRIGGER_PRIVILEGE
                      , COM_ALTER_VIEW_PRIVILEGE
                      , COM_DROP_PRIVILEGE
                      , COM_DROP_TABLE_PRIVILEGE
                      , COM_DROP_LIBRARY_PRIVILEGE
                      , COM_DROP_MV_PRIVILEGE
                      , COM_DROP_MV_GROUP_PRIVILEGE
                      , COM_DROP_PROCEDURE_PRIVILEGE
                      , COM_DROP_SYNONYM_PRIVILEGE
                      , COM_DROP_TRIGGER_PRIVILEGE
                      , COM_DROP_VIEW_PRIVILEGE
// Privileges used in schema v2500
                      , COM_ALTER_ROUTINE_PRIVILEGE
                      , COM_ALTER_ROUTINE_ACTION_PRIVILEGE
                      , COM_CREATE_ROUTINE_PRIVILEGE
                      , COM_CREATE_ROUTINE_ACTION_PRIVILEGE
                      , COM_DROP_ROUTINE_PRIVILEGE
                      , COM_DROP_ROUTINE_ACTION_PRIVILEGE
// Privileges defined for Business Continuity
                      , COM_BUSINESS_CONTINUITY_PRIVILEGE
                      , COM_BACKUP_PRIVILEGE
                      , COM_RESTORE_PRIVILEGE
                      , COM_ARCHIVE_PRIVILEGE
// The Privileges below are not yet used; the ones that become used should
// be moved to before those that remain unused.
                      , COM_TRIGGER_PRIVILEGE
                      , COM_MAINTAIN_PRIVILEGE
                      , COM_REFRESH_PRIVILEGE
                      , COM_REORG_PRIVILEGE
                      , COM_UPDATE_STATS_PRIVILEGE
                      };

#define COM_PRIVILEGE_LIT_LEN               2
#define COM_UNKNOWN_PRIVILEGE_LIT           "  "
#define COM_SELECT_PRIVILEGE_LIT            "S "
#define COM_INSERT_PRIVILEGE_LIT            "I "
#define COM_DELETE_PRIVILEGE_LIT            "D "
#define COM_UPDATE_PRIVILEGE_LIT            "U "
#define COM_USAGE_PRIVILEGE_LIT             "Y "
#define COM_REFERENCE_PRIVILEGE_LIT         "R "
#define COM_EXECUTE_PRIVILEGE_LIT           "E "
#define COM_ALTER_TABLE_PRIVILEGE_LIT       "AB"
#define COM_ALTER_LIBRARY_PRIVILEGE_LIT     "AL"
#define COM_ALTER_MV_PRIVILEGE_LIT          "AM"
#define COM_ALTER_SYNONYM_PRIVILEGE_LIT     "AS"
#define COM_ALTER_VIEW_PRIVILEGE_LIT        "AV"
#define COM_TRIGGER_PRIVILEGE_LIT           "T "
#define COM_MAINTAIN_PRIVILEGE_LIT          "M "
#define COM_REFRESH_PRIVILEGE_LIT           "RF"
#define COM_REORG_PRIVILEGE_LIT             "RO"
#define COM_UPDATE_STATS_PRIVILEGE_LIT      "US"

// ADDITIONAL PRIVILEGES LITs FOR SCHEMA PRIVILEGES TABLE

#define COM_DATABASE_ADMINISTRATOR_PRIVILEGE_LIT   "AD"
#define COM_ALTER_PRIVILEGE_LIT                    "A "
#define COM_ALTER_TRIGGER_PRIVILEGE_LIT            "AT"
#define COM_ALTER_MV_GROUP_PRIVILEGE_LIT           "AG"
#define COM_ALTER_ROUTINE_PRIVILEGE_LIT            "AR"
#define COM_ALTER_ROUTINE_ACTION_PRIVILEGE_LIT     "AA"
#define COM_CREATE_PRIVILEGE_LIT                   "C "
#define COM_CREATE_TABLE_PRIVILEGE_LIT             "CB"
#define COM_CREATE_LIBRARY_PRIVILEGE_LIT           "CL"
#define COM_CREATE_MV_PRIVILEGE_LIT                "CM"
#define COM_CREATE_PROCEDURE_PRIVILEGE_LIT         "CP"
#define COM_CREATE_SYNONYM_PRIVILEGE_LIT           "CS"
#define COM_CREATE_TRIGGER_PRIVILEGE_LIT           "CT"
#define COM_CREATE_VIEW_PRIVILEGE_LIT              "CV"
#define COM_CREATE_MV_GROUP_PRIVILEGE_LIT          "CG"
#define COM_CREATE_ROUTINE_PRIVILEGE_LIT           "CR"
#define COM_CREATE_ROUTINE_ACTION_PRIVILEGE_LIT    "CA"
#define COM_DROP_PRIVILEGE_LIT                     "DR"
#define COM_DROP_TABLE_PRIVILEGE_LIT               "DB"
#define COM_DROP_LIBRARY_PRIVILEGE_LIT             "DL"
#define COM_DROP_MV_PRIVILEGE_LIT                  "DM"
#define COM_DROP_MV_GROUP_PRIVILEGE_LIT            "DG"
#define COM_DROP_PROCEDURE_PRIVILEGE_LIT           "DP"
#define COM_DROP_ROUTINE_PRIVILEGE_LIT             "DD"
#define COM_DROP_ROUTINE_ACTION_PRIVILEGE_LIT      "DA"
#define COM_DROP_SYNONYM_PRIVILEGE_LIT             "DS"
#define COM_DROP_TRIGGER_PRIVILEGE_LIT             "DT"
#define COM_DROP_VIEW_PRIVILEGE_LIT                "DV"
#define COM_BUSINESS_CONTINUITY_PRIVILEGE_LIT      "BC"
#define COM_BACKUP_PRIVILEGE_LIT                   "BA"
#define COM_RESTORE_PRIVILEGE_LIT                  "RS"
#define COM_ARCHIVE_PRIVILEGE_LIT                  "AC"

// Values for Query Invalidation
enum ComQIActionType { COM_QI_INVALID_ACTIONTYPE = 0
                     , COM_QI_USER_GRANT_ROLE
                     , COM_QI_ROLE_GRANT_ROLE
                     , COM_QI_OBJECT_SELECT
                     , COM_QI_OBJECT_INSERT
                     , COM_QI_OBJECT_DELETE
                     , COM_QI_OBJECT_UPDATE
                     , COM_QI_OBJECT_USAGE
                     , COM_QI_OBJECT_REFERENCES
                     , COM_QI_SCHEMA_SELECT
                     , COM_QI_SCHEMA_INSERT
                     , COM_QI_SCHEMA_DELETE
                     , COM_QI_SCHEMA_UPDATE
                     , COM_QI_SCHEMA_USAGE
                     , COM_QI_SCHEMA_REFERENCES
                     , COM_QI_OBJECT_EXECUTE
                     , COM_QI_SCHEMA_EXECUTE
                     , COM_QI_USER_GRANT_SPECIAL_ROLE
                     , COM_QI_OBJECT_REDEF
                     , COM_QI_STATS_UPDATED
                     , COM_QI_GRANT_ROLE
                     , COM_QI_COLUMN_SELECT
                     , COM_QI_COLUMN_INSERT
                     , COM_QI_COLUMN_UPDATE
                     , COM_QI_COLUMN_REFERENCES
                     } ;

#define COM_QI_INVALID_ACTIONTYPE_LIT  "  "
#define COM_QI_USER_GRANT_ROLE_LIT     "UR"
#define COM_QI_ROLE_GRANT_ROLE_LIT     "RR"
#define COM_QI_OBJECT_SELECT_LIT       "OS"
#define COM_QI_OBJECT_INSERT_LIT       "OI"
#define COM_QI_OBJECT_DELETE_LIT       "OD"
#define COM_QI_OBJECT_UPDATE_LIT       "OU"
#define COM_QI_OBJECT_USAGE_LIT        "OG"
#define COM_QI_OBJECT_REFERENCES_LIT   "OF"
#define COM_QI_SCHEMA_SELECT_LIT       "SS"
#define COM_QI_SCHEMA_INSERT_LIT       "SI"
#define COM_QI_SCHEMA_DELETE_LIT       "SD"
#define COM_QI_SCHEMA_UPDATE_LIT       "SU"
#define COM_QI_SCHEMA_USAGE_LIT        "SG"
#define COM_QI_SCHEMA_REFERENCES_LIT   "SF"
#define COM_QI_OBJECT_EXECUTE_LIT      "OE"
#define COM_QI_SCHEMA_EXECUTE_LIT      "SE"
#define COM_QI_USER_GRANT_SPECIAL_ROLE_LIT     "UZ"
#define COM_QI_OBJECT_REDEF_LIT        "OR"
#define COM_QI_STATS_UPDATED_LIT       "US"
#define COM_QI_GRANT_ROLE_LIT          "GG"
#define COM_QI_COLUMN_SELECT_LIT       "CS"
#define COM_QI_COLUMN_INSERT_LIT       "CI"
#define COM_QI_COLUMN_UPDATE_LIT       "CU"
#define COM_QI_COLUMN_REFERENCES_LIT   "CF"




enum ComRCDeleteRule { COM_UNKNOWN_DELETE_RULE
                     , COM_CASCADE_DELETE_RULE
                     , COM_NO_ACTION_DELETE_RULE
                     , COM_SET_DEFAULT_DELETE_RULE
                     , COM_SET_NULL_DELETE_RULE
                     , COM_RESTRICT_DELETE_RULE
                     };

#define COM_UNKNOWN_DELETE_RULE_LIT               "  "
#define COM_CASCADE_DELETE_RULE_LIT               "CA"
#define COM_NO_ACTION_DELETE_RULE_LIT             "NA"
#define COM_SET_DEFAULT_DELETE_RULE_LIT           "SD"
#define COM_SET_NULL_DELETE_RULE_LIT              "SN"
#define COM_RESTRICT_DELETE_RULE_LIT              "RE"

enum ComRCMatchOption { COM_UNKNOWN_MATCH_OPTION
                      , COM_FULL_MATCH_OPTION
                      , COM_NONE_MATCH_OPTION
                      , COM_PARTIAL_MATCH_OPTION
                      };

#define COM_UNKNOWN_MATCH_OPTION_LIT               "  "
#define COM_FULL_MATCH_OPTION_LIT                  "F "
#define COM_NONE_MATCH_OPTION_LIT                  "N "
#define COM_PARTIAL_MATCH_OPTION_LIT               "P "

enum ComRCUpdateRule { COM_UNKNOWN_UPDATE_RULE
                     , COM_CASCADE_UPDATE_RULE
                     , COM_NO_ACTION_UPDATE_RULE
                     , COM_SET_DEFAULT_UPDATE_RULE
                     , COM_SET_NULL_UPDATE_RULE
                     , COM_RESTRICT_UPDATE_RULE
                     };

#define COM_UNKNOWN_UPDATE_RULE_LIT                 "  "
#define COM_CASCADE_UPDATE_RULE_LIT                 "CA"
#define COM_NO_ACTION_UPDATE_RULE_LIT               "NA"
#define COM_SET_DEFAULT_UPDATE_RULE_LIT             "SD"
#define COM_SET_NULL_UPDATE_RULE_LIT                "SN"
#define COM_RESTRICT_UPDATE_RULE_LIT                "RE"

enum ComRoutineLanguage { COM_UNKNOWN_ROUTINE_LANGUAGE
                        , COM_LANGUAGE_JAVA
                        , COM_LANGUAGE_C
                        , COM_LANGUAGE_CPP
                        , COM_LANGUAGE_SQL
                        };

#define COM_UNKNOWN_ROUTINE_LANGUAGE_LIT            "  "
#define COM_LANGUAGE_JAVA_LIT                       "J "
#define COM_LANGUAGE_C_LIT                          "C "
#define COM_LANGUAGE_CPP_LIT                        "C+"
#define COM_LANGUAGE_SQL_LIT                        "S "

// Parameter passing styles for stored procedures and user-defined
// functions.
enum ComRoutineParamStyle { COM_UNKNOWN_ROUTINE_PARAM_STYLE
                          , COM_STYLE_GENERAL
                          , COM_STYLE_JAVA_CALL
                          , COM_STYLE_JAVA_OBJ
                          , COM_STYLE_SQL
                          , COM_STYLE_SQLROW
                          , COM_STYLE_SQLROW_TM
                          , COM_STYLE_CPP_OBJ
                          };

#define COM_UNKNOWN_ROUTINE_PARAM_STYLE_LIT         "  "
#define COM_STYLE_GENERAL_LIT                       "G "
#define COM_STYLE_JAVA_CALL_LIT                     "J "
#define COM_STYLE_JAVA_OBJ_LIT                      "JO"
#define COM_STYLE_SQL_LIT                           "S "
#define COM_STYLE_SQLROW_LIT                        "SR"
#define COM_STYLE_SQLROW_TM_LIT                     "TM"
#define COM_STYLE_CPP_OBJ_LIT                       "C+"

#define COM_UNKNOWN_ROUTINE_PARAM_STYLE_VERSION       0
#define COM_ROUTINE_PARAM_STYLE_VERSION_1             1

// Routine parallelism attribute
enum ComRoutineParallelism { COM_ROUTINE_NO_PARALLELISM
                           , COM_ROUTINE_ANY_PARALLELISM
                           };

#define COM_ROUTINE_NO_PARALLELISM_LIT              "NO"
#define COM_ROUTINE_ANY_PARALLELISM_LIT             "AP"

// Routine security attribute for definer/invoker rights
enum ComRoutineExternalSecurity { COM_ROUTINE_EXTERNAL_SECURITY_INVOKER // the default
                                , COM_ROUTINE_EXTERNAL_SECURITY_DEFINER
                                , COM_ROUTINE_EXTERNAL_SECURITY_IMPLEMENTATION_DEFINED
                                };

#define COM_ROUTINE_EXTERNAL_SECURITY_INVOKER_LIT                "I "
#define COM_ROUTINE_EXTERNAL_SECURITY_DEFINER_LIT                "D "
#define COM_ROUTINE_EXTERNAL_SECURITY_IMPLEMENTATION_DEFINED_LIT "X "

// Routine pass through input value type: either TEXT or BINARY
enum ComRoutinePassThroughInputType { COM_ROUTINE_PASS_THROUGH_INPUT_TEXT_TYPE
                                    , COM_ROUTINE_PASS_THROUGH_INPUT_BINARY_TYPE
                                    };

#define COM_ROUTINE_PASS_THROUGH_INPUT_TEXT_TYPE_LIT             "TEXT"
#define COM_ROUTINE_PASS_THROUGH_INPUT_BINARY_TYPE_LIT           "BINARY"

// Do not change the following order of definitions
// because they are used for indexing into an array.
enum ComRoutinePTIAttrKind { COM_ROUTINE_PTI_ATTR_KIND_PASS_THROUGH_INPUT_TYPE
                           , COM_ROUTINE_PTI_ATTR_KIND_VALUE_ENCODING_VERSION
                           , COM_ROUTINE_PTI_ATTR_KIND_UNKNOWN
};

enum ComRoutinePTIAttrValueKind { COM_ROUTINE_PTI_ATTR_VALUE_KIND_WORD
                                , COM_ROUTINE_PTI_ATTR_VALUE_KIND_NUMBER
                                , COM_ROUTINE_PTI_ATTR_VALUE_KIND_UNKNOWN
};

#define COM_ROUTINE_PTI_ATTR_NAME_TYPE_LIT                       "TYPE="
#define COM_ROUTINE_PTI_ATTR_NAME_ENCODED_LIT                    "ENCODED="

enum ComUdrParamFlags
{
  UDR_PARAM_IN           = 0x0001,  // For IN and INOUT parameters
  UDR_PARAM_OUT          = 0x0002,  // For OUT and INOUT parameters
  UDR_PARAM_NULLABLE     = 0x0004,  // The parameter type is nullable
  UDR_PARAM_LM_OBJ_TYPE  = 0x0008   // If set the Language Manager will
                                    // map the SQL type to an external type
                                    // using its "object mapping" instead of
                                    // the default mapping. This allows, for
                                    // example, the SQL INTEGER type to map
                                    // to either int or java.lang.Integer in
                                    // Java.
};

enum ComUdrFlags
{
  UDR_ISOLATE            = 0x0001,  // Cannot run in priv mode
  UDR_CALL_ON_NULL       = 0x0002,  // OK to invoke if an input is NULL
  UDR_EXTRA_CALL         = 0x0004,  // UDR expects a cleanup call
  UDR_FINAL_CALL         = UDR_EXTRA_CALL,  // FINAL CALL and EXTRA CALL
                                            // mean the same thing. EXTRA
                                            // CALL is deprecated.
  UDR_DETERMINISTIC      = 0x0008,  // Always same outputs given same inputs
  UDR_LM_NOLOAD          = 0x0010,  // Don't load Language Manager at startup
  UDR_RESETSTATS         = 0x0020,  // Reset UDR Server statistics
  UDR_RESULT_SET         = 0x0040,  // Differentiate TDB/TCB for result sets
  UDR_TMUDF              = 0x0080  // This is a table mapping udf
};

enum ComRoutineSQLAccess { COM_UNKNOWN_ROUTINE_SQL_ACCESS
                         , COM_NO_SQL
                         , COM_CONTAINS_SQL
                         , COM_READS_SQL
                         , COM_MODIFIES_SQL
                         };

#define COM_UNKNOWN_ROUTINE_SQL_ACCESS_LIT          "  "
#define COM_NO_SQL_LIT                              "N "
#define COM_CONTAINS_SQL_LIT                        "C "
#define COM_READS_SQL_LIT                           "R "
#define COM_MODIFIES_SQL_LIT                        "M "

enum ComRoutineTransactionAttributes { COM_UNKNOWN_ROUTINE_TRANSACTION_ATTRIBUTE
                                     , COM_NO_TRANSACTION_REQUIRED
                                     , COM_TRANSACTION_REQUIRED
                                     };

#define COM_UNKNOWN_ROUTINE_TRANSACTION_ATTRIBUTE_LIT "  "
#define COM_NO_TRANSACTION_REQUIRED_LIT               "NO"
#define COM_TRANSACTION_REQUIRED_LIT                  "RQ"

enum ComRoutineType { COM_UNKNOWN_ROUTINE_TYPE
                    , COM_PROCEDURE_TYPE
                    , COM_SCALAR_UDF_TYPE
                    , COM_TABLE_UDF_TYPE
                    , COM_UNIVERSAL_UDF_TYPE
                    , COM_ACTION_UDF_TYPE
                    };

#define COM_UNKNOWN_ROUTINE_TYPE_LIT                "  "
#define COM_PROCEDURE_TYPE_LIT                      "P "
#define COM_SCALAR_UDF_TYPE_LIT                     "F "
#define COM_TABLE_UDF_TYPE_LIT                      "T "
#define COM_UNIVERSAL_UDF_TYPE_LIT                  "U "
#define COM_ACTION_UDF_TYPE_LIT                     "AC"

// Routine execution mode
enum ComRoutineExecutionMode { COM_ROUTINE_FAST_EXECUTION
                             , COM_ROUTINE_SAFE_EXECUTION
                             };

#define COM_ROUTINE_FAST_EXECUTION_LIT              "FA"
#define COM_ROUTINE_SAFE_EXECUTION_LIT              "SF"

enum ComUdfOptimizationHintKind { COM_UDF_INITIAL_CPU_COST
                                , COM_UDF_INITIAL_IO_COST
                                , COM_UDF_INITIAL_MESSAGE_COST
                                , COM_UDF_NORMAL_CPU_COST
                                , COM_UDF_NORMAL_IO_COST
                                , COM_UDF_NORMAL_MESSAGE_COST
                                , COM_UDF_NUMBER_OF_UNIQUE_OUTPUT_VALUES
                                };

enum ComUudfParamKind { COM_UUDF_PARAM_OMITTED
                      , COM_UUDF_PARAM_ACTION
                      , COM_UUDF_PARAM_SAS_FORMAT
                      , COM_UUDF_PARAM_SAS_LOCALE
                      , COM_UUDF_PARAM_SAS_MODEL_INPUT_TABLE
                      };

#define  COM_UUDF_PARAM_OMITTED_LIT                 ""
#define  COM_UUDF_PARAM_ACTION_LIT                  "ACTION"
#define  COM_UUDF_PARAM_SAS_FORMAT_LIT              "SAS_FORMAT"
#define  COM_UUDF_PARAM_SAS_LOCALE_LIT              "SAS_LOCALE"
#define  COM_UUDF_PARAM_SAS_MODEL_INPUT_TABLE_LIT   "SAS_MODEL_INPUT_TABLE"

enum ComStoreOption { COM_UNKNOWN_STORE_OPTION
                    , COM_ENTRY_ORDER_STORE_OPTION
                    , COM_KEY_COLUMN_LIST_STORE_OPTION
                    , COM_NONDROPPABLE_PK_STORE_OPTION
                    , COM_RELATIVE_ORDER_STORE_OPTION
                    };

// Exception table enums
enum ComExceptionTableType { COM_UNKNOWN_EXCEPTION_TABLE_TYPE
                           , COM_VALIDATE_EXCEPTION_TABLE_TYPE
                           };

#define COM_UNKNOWN_EXCEPTION_TABLE_TYPE_LIT        "  "
#define COM_VALIDATE_EXCEPTION_TABLE_TYPE_LIT       "VD"

// [Distribution] Literals & defines for replication rules
enum ComReplicationRule { COM_UNKNOWN_REPLICATION,
                          COM_AUTOMATIC_REPLICATION,
                          COM_MANUAL_REPLICATION};

#define COM_UNKNOWN_REPLICATION_LIT                "  "
#define COM_AUTOMATIC_REPLICATION_LIT              "A "
#define COM_MANUAL_REPLICATION_LIT                 "M "

// [Distribution/Versioning] Literals & defines for future schema level operations
enum ComSchemaOperation { COM_NO_SCHEMA_OPERATION,
                          COM_UPGRADE_OPERATION,
                          COM_DOWNGRADE_OPERATION,
                          COM_VOLATILE_SCHEMA_OPERATION,
                          COM_DISABLE_CREATE_OPERATION,
                          COM_UNKNOWN_SCHEMA_OPERATION};

#define COM_NO_SCHEMA_OPERATION_LIT                "  "
#define COM_UPGRADE_OPERATION_LIT                  "UG"
#define COM_DOWNGRADE_OPERATION_LIT                "DG"
#define COM_VOLATILE_SCHEMA_OPERATION_LIT          "VS"
#define COM_DISABLE_CREATE_OPERATION_LIT           "DC"
#define COM_UNKNOWN_SCHEMA_OPERATION_LIT           "??"

//Schema Type
enum ComSchemaType { COM_USER_TYPE,
                     COM_PUBLIC_TYPE,
                     COM_SYSTEM_TYPE};

#define COM_USER_TYPE_LIT       "U"
#define COM_PUBLIC_TYPE_LIT     "P"
#define COM_SYSTEM_TYPE_LIT     "S"

//Schema Class
enum ComSchemaClass { COM_SCHEMA_CLASS_UNKNOWN = 2,
                      COM_SCHEMA_CLASS_PRIVATE = 3,
                      COM_SCHEMA_CLASS_SHARED = 4,
                      COM_SCHEMA_CLASS_DEFAULT = 5
                    };

enum ComSQLDataType { COM_UNKNOWN_SDT
                    , COM_CHARACTER_SDT
                    , COM_VARCHAR_SDT
                    , COM_LONG_VARCHAR_SDT
                    , COM_BPINT_UNSIGNED_SDT
                    , COM_NUMERIC_SIGNED_SDT
                    , COM_NUMERIC_UNSIGNED_SDT
                    , COM_TINYINT_SIGNED_SDT
                    , COM_TINYINT_UNSIGNED_SDT
                    , COM_SMALLINT_SIGNED_SDT
                    , COM_SMALLINT_UNSIGNED_SDT
                    , COM_INTEGER_SIGNED_SDT
                    , COM_INTEGER_UNSIGNED_SDT
                    , COM_LARGEINT_SIGNED_SDT
                    , COM_LARGEINT_UNSIGNED_SDT
                    , COM_FLOAT_SDT
                    , COM_REAL_SDT
                    , COM_DOUBLE_SDT
                    , COM_DECIMAL_SIGNED_SDT
                    , COM_DECIMAL_UNSIGNED_SDT
                    , COM_LARGE_DECIMAL_SIGNED_SDT
                    , COM_LARGE_DECIMAL_UNSIGNED_SDT
		    , COM_BLOB_SDT
		    , COM_CLOB_SDT
		    , COM_BOOLEAN_SDT
                    , COM_DATETIME_SDT
                    , COM_TIMESTAMP_SDT
                    , COM_DATE_SDT
                    , COM_TIME_SDT
                    , COM_INTERVAL_SDT
                    };

#define COM_UNKNOWN_SDT_LIT                    "                  "
#define COM_CHARACTER_SDT_LIT                  "CHARACTER         "
#define COM_VARCHAR_SDT_LIT                    "VARCHAR           "
#define COM_LONG_VARCHAR_SDT_LIT               "LONG VARCHAR      "
#define COM_NUMERIC_SIGNED_SDT_LIT             "SIGNED NUMERIC    "
#define COM_NUMERIC_UNSIGNED_SDT_LIT           "UNSIGNED NUMERIC  "
#define COM_TINYINT_SIGNED_SDT_LIT             "SIGNED TINYINT   "
#define COM_TINYINT_UNSIGNED_SDT_LIT           "UNSIGNED TINYINT "
#define COM_SMALLINT_SIGNED_SDT_LIT            "SIGNED SMALLINT   "
#define COM_SMALLINT_UNSIGNED_SDT_LIT          "UNSIGNED SMALLINT "
#define COM_INTEGER_SIGNED_SDT_LIT             "SIGNED INTEGER    "
#define COM_INTEGER_UNSIGNED_SDT_LIT           "UNSIGNED INTEGER  "
#define COM_BPINT_UNSIGNED_SDT_LIT             "UNSIGNED BP INT   "
#define COM_LARGEINT_SIGNED_SDT_LIT            "SIGNED LARGEINT   "
#define COM_LARGEINT_UNSIGNED_SDT_LIT          "UNSIGNED LARGEINT "
#define COM_FLOAT_SDT_LIT                      "FLOAT             "
#define COM_REAL_SDT_LIT                       "REAL              "
#define COM_DOUBLE_SDT_LIT                     "DOUBLE            "
#define COM_DECIMAL_SIGNED_SDT_LIT             "SIGNED DECIMAL    "
#define COM_DECIMAL_UNSIGNED_SDT_LIT           "UNSIGNED DECIMAL  "
#define COM_DATETIME_SDT_LIT                   "DATETIME          "
#define COM_TIMESTAMP_SDT_LIT                  "TIMESTAMP         "
#define COM_DATE_SDT_LIT                       "DATE              "
#define COM_TIME_SDT_LIT                       "TIME              "
#define COM_INTERVAL_SDT_LIT                   "INTERVAL          "
#define COM_BLOB_SDT_LIT                       "BLOB              "
#define COM_CLOB_SDT_LIT                       "CLOB              "
#define COM_BOOLEAN_SDT_LIT                    "BOOLEAN           "
#define COM_CHAR_BINARY_SDT_LIT                     "BINARY            "
#define COM_CHAR_VARBINARY_SDT_LIT                  "VARBINARY         "

enum ComViewCheckOption { COM_UNKNOWN_CHECK_OPTION
                        , COM_CASCADE_CHECK_OPTION
                        , COM_LOCAL_CHECK_OPTION
                        , COM_NONE_CHECK_OPTION
                        };

#define COM_UNKNOWN_CHECK_OPTION_LIT           "  "
#define COM_CASCADE_CHECK_OPTION_LIT           "C "
#define COM_LOCAL_CHECK_OPTION_LIT             "L "
#define COM_NONE_CHECK_OPTION_LIT              "N "

// Added in v2500
enum ComViewType { COM_UNKNOWN_VIEW_TYPE
                 , COM_USER_VIEW_TYPE
                 , COM_SYSTEM_VIEW_TYPE
                 };

#define COM_UNKNOWN_VIEW_TYPE_LIT              "  "
#define COM_USER_VIEW_TYPE_LIT                 "UV"
#define COM_SYSTEM_VIEW_TYPE_LIT               "SV"

enum ComAutoRebindOption { COM_UNKNOWN_RBND_OPTION
                         , COM_CUR_DEFAULTS_CUR_DEFINES_RBND_OPTION
                         , COM_STO_DEFAULTS_STO_DEFINES_RBND_OPTION
                         , COM_CUR_DEFAULTS_STO_DEFINES_RBND_OPTION
                         , COM_STO_DEFAULTS_CUR_DEFINES_RBND_OPTION
                         , COM_NONE_RBND_OPTION
                         };

#define COM_UNKNOWN_RBND_OPTION_LIT                       "  "
#define COM_CUR_DEFAULTS_CUR_DEFINES_RBND_OPTION_LIT      "CC"
#define COM_STO_DEFAULTS_STO_DEFINES_RBND_OPTION_LIT      "SS"
#define COM_CUR_DEFAULTS_STO_DEFINES_RBND_OPTION_LIT      "CS"
#define COM_STO_DEFAULTS_CUR_DEFINES_RBND_OPTION_LIT      "SC"
#define COM_NONE_RBND_OPTION_LIT                          "OF"

enum ComPartnStatus { COM_UNKNOWN_PARTN_STATUS
                    , COM_PARTN_AVAILABLE
                    , COM_PARTN_OFFLINE
                    , COM_PARTN_CORRUPT
                    , COM_PARTN_PHANTOM};

#define COM_UNKNOWN_PARTN_STATUS_LIT       "  "
#define COM_PARTN_AVAILABLE_LIT            "AV"
#define COM_PARTN_OFFLINE_LIT              "UO"
#define COM_PARTN_CORRUPT_LIT              "UC"
#define COM_PARTN_PHANTOM_LIT              "PH"

enum ComUtilOperation { COM_UNKNOWN_UTIL
                      , COM_UTIL_BACKUP
                      , COM_UTIL_DROP_LABEL
                      , COM_UTIL_DUP
                      , COM_UTIL_EXPORT
                      , COM_UTIL_IMPORT
                      , COM_UTIL_MODIFY_TABLE
                      , COM_UTIL_MODIFY_INDEX
                      , COM_UTIL_POPULATE_INDEX
                      , COM_UTIL_PURGEDATA
                      , COM_UTIL_RECOVER
                      , COM_UTIL_RESTORE
                      , COM_UTIL_UPDATE_STATISTICS
                      , COM_UTIL_REFRESH
                      , COM_UTIL_UPDATE_PARTITION_METADATA
                      , COM_UTIL_ALL_METADATA_UPGRADE
                      , COM_UTIL_ALL_METADATA_DOWNGRADE
                      , COM_UTIL_TRANSFORM
                      , COM_UTIL_VALIDATE
                      };

#define COM_UNKNOWN_UTIL_LIT            "  "
#define COM_UTIL_BACKUP_LIT             "BK"
#define COM_UTIL_DROP_LABEL_LIT         "DL"
#define COM_UTIL_DUP_LIT                "DP"
#define COM_UTIL_EXPORT_LIT             "EX"
#define COM_UTIL_IMPORT_LIT             "IM"
#define COM_UTIL_MODIFY_TABLE_LIT       "MT"
#define COM_UTIL_MODIFY_INDEX_LIT       "MI"
#define COM_UTIL_POPULATE_INDEX_LIT     "PI"
#define COM_UTIL_PURGEDATA_LIT          "PD"
#define COM_UTIL_RECOVER_LIT            "RC"
#define COM_UTIL_RESTORE_LIT            "RS"
#define COM_UTIL_UPDATE_STATISTICS_LIT  "US"
#define COM_UTIL_REFRESH_LIT            "RF"
#define COM_UTIL_TRANSFORM_LIT          "TR"
#define COM_UTIL_UPDATE_PARTITION_METADATA_LIT "UP"
#define COM_UTIL_ALL_METADATA_UPGRADE_LIT  "MU"
#define COM_UTIL_ALL_METADATA_DOWNGRADE_LIT  "MD"
#define COM_UTIL_VALIDATE_LIT              "VA"


enum ComDdlStatus { COM_UNKNOWN_DDL_STATUS
                  , COM_NO_DDL_IN_PROGRESS
                  , COM_ROW_HIDING
                  , COM_KEY_RANGE_CHECKING };

#define COM_UNKNOWN_DDL_STATUS_LIT      "  "
#define COM_NO_DDL_IN_PROGRESS_LIT      "N "
#define COM_ROW_HIDING_LIT              "RH"
#define COM_KEY_RANGE_CHECKING_LIT      "KR"

// Support for the NOT DROPPABLE and INSERT_ONLY attributes
enum ComTableFeature  { COM_UNKNOWN_TABLE_FEATURE
                      , COM_DROPPABLE
                      , COM_DROPPABLE_INSERT_ONLY
                      , COM_NOT_DROPPABLE
                      , COM_NOT_DROPPABLE_INSERT_ONLY };

#define COM_UNKNOWN_TABLE_FEATURE_LIT        "  "
#define COM_DROPPABLE_LIT                    "Y "
#define COM_DROPPABLE_INSERT_ONLY_LIT        "YI"
#define COM_NOT_DROPPABLE_LIT                "N "
#define COM_NOT_DROPPABLE_INSERT_ONLY_LIT    "NI"


enum ComDiskFileFormat { UNKNOWN = 0,
             SQLMP   = 100,       // MP
             SQLARK  = 200,       // simulator
             SQLMX   = 300        // real MX
                       };

enum ComLobsStorage {
    Lob_Invalid_Storage = 0,
    Lob_Local_File = 1,
    Lob_HDFS_File = 2,
    Lob_HBASE_Table = 3,
    Lob_Cloud_File = 4,
    Lob_Seaquest_Table = 5,
    Lob_MySQL_Table = 6,
    Lob_InMemory_File = 7,
    Lob_External_HDFS_File = 8,
    Lob_External_HBASE_Table = 9,
    Lob_External_Local_File = 10,
    Lob_Empty = 11
};

// for the TRANSFORM utility - equivalent definitions in catapirequest.h
enum ComTransformDependent { COM_TR_DROP_DEPENDENT
                           , COM_TR_CASCADE_DEPENDENT
                           , COM_TR_RECREATE_DEPENDENT
                           , COM_TR_KEEP_DEPENDENT };

enum ComPrivilegeChecks { COM_PRIV_CHECK_PASS = 0,
                          COM_PRIV_NO_CHECK   = 1,
                          COM_PRIV_CHECK_FAIL = 2 };
        
// for serialization of primary keys
enum ComPkeySerialization
  { COM_SER_NOT_SPECIFIED =  0,    // not specified, will be set based on
                                   // the table type
    COM_SERIALIZED        =  1,    // pkey is serialized
    COM_NOT_SERIALIZED    =  2     // pkey is not serialized
  };
                  
// DDL Operation literals

#define COM_OP_NONE_LIT                            "  "
#define COM_OP_ALTER_CATALOG_LIT                   "AC"
#define COM_OP_ALTER_INDEX_LIT                     "AI"
#define COM_OP_ALTER_LIBRARY_LIT                   "AL"
#define COM_OP_ALTER_MV_LIT                        "AM"
#define COM_OP_ALTER_MV_GROUP_LIT                  "AG"
#define COM_OP_ALTER_PROCEDURE_LIT                 "AP"
#define COM_OP_ALTER_ROUTINE_LIT                   "AR"
#define COM_OP_ALTER_ROUTINE_ACTION_LIT            "AA"
#define COM_OP_ALTER_TABLE_LIT                     "AB"
#define COM_OP_ALTER_TRIGGER_LIT                   "AT"
#define COM_OP_ALTER_VIEW_LIT                      "AV"
#define COM_OP_CREATE_CATALOG_LIT                  "CC"
#define COM_OP_CREATE_COMPONENT_LIT                "CN"
#define COM_OP_CREATE_COMPONENT_PRIVILEGE_LIT      "CO"
#define COM_OP_CREATE_INDEX_LIT                    "CI"
#define COM_OP_CREATE_LIBRARY_LIT                  "CL"
#define COM_OP_CREATE_MV_LIT                       "CM"
#define COM_OP_CREATE_MV_GROUP_LIT                 "CG"
#define COM_OP_CREATE_PROCEDURE_LIT                "CP"
#define COM_OP_CREATE_ROLE_LIT                     "CE"
#define COM_OP_CREATE_ROUTINE_LIT                  "CR"
#define COM_OP_CREATE_ROUTINE_ACTION_LIT           "CA"
#define COM_OP_CREATE_SCHEMA_LIT                   "CH"
#define COM_OP_CREATE_TABLE_LIT                    "CB"
#define COM_OP_CREATE_TRIGGER_LIT                  "CT"
#define COM_OP_CREATE_VIEW_LIT                     "CV"
#define COM_OP_DROP_CATALOG_LIT                    "DC"
#define COM_OP_DROP_COMPONENT_LIT                  "DN"
#define COM_OP_DROP_COMPONENT_PRIVILEGE_LIT        "DO"
#define COM_OP_DROP_INDEX_LIT                      "DI"
#define COM_OP_DROP_LIBRARY_LIT                    "DL"
#define COM_OP_DROP_MV_LIT                         "DM"
#define COM_OP_DROP_MV_GROUP_LIT                   "DG"
#define COM_OP_DROP_PROCEDURE_LIT                  "DP"
#define COM_OP_DROP_ROLE_LIT                       "DE"
#define COM_OP_DROP_ROUTINE_LIT                    "DR"
#define COM_OP_DROP_ROUTINE_ACTION_LIT             "DA"
#define COM_OP_DROP_SCHEMA_LIT                     "DH"
#define COM_OP_DROP_TABLE_LIT                      "DB"
#define COM_OP_DROP_TRIGGER_LIT                    "DT"
#define COM_OP_DROP_VIEW_LIT                       "DV"
#define COM_OP_REGISTER_COMPONENT_LIT              "RC"
#define COM_OP_UNREGISTER_COMPONENT_LIT            "UC"

// DDL Suboperation literals
#define COM_SUBOP_NONE_LIT                          "  "
#define COM_SUBOP_ADD_COLUMN_LIT                    "A "
#define COM_SUBOP_ADD_CHECK_CONSTRAINT_LIT          "AC"
#define COM_SUBOP_ADD_INFORMATION_LIT               "AD"
#define COM_SUBOP_ADD_RI_CONSTRAINT_LIT             "AF"
#define COM_SUBOP_ADD_PK_CONSTRAINT_LIT             "AP"
#define COM_SUBOP_ALTER_ATTRIBUTE_LIT               "AT"
#define COM_SUBOP_ADD_UNIQUE_CONSTRAINT_LIT         "AU"
#define COM_SUBOP_ALTER_COLUMN_SG_OPTION_LIT        "CG"
#define COM_SUBOP_ALTER_COLUMN_RECALIBRATE_LIT      "CR"
#define COM_SUBOP_CASCADE_LIT                       "CS"
#define COM_SUBOP_CASCADE_INVALIDATE_LIT            "CI"
#define COM_SUBOP_COMPILE_LIT                       "CM"
#define COM_SUBOP_DROP_CHECK_CONSTRAINT_LIT         "DC"
#define COM_SUBOP_DROP_RI_CONSTRAINT_LIT            "DF"
#define COM_SUBOP_DISABLE_LIT                       "DI"
#define COM_SUBOP_DROP_PK_CONSTRAINT_LIT            "DP"
#define COM_SUBOP_DROP_UNIQUE_CONSTRAINT_LIT        "DU"
#define COM_SUBOP_ENABLE_LIT                        "EN"
#define COM_SUBOP_REMOVE_INFORMATION_LIT            "RM"
#define COM_SUBOP_RENAME_LIT                        "RN"
#define COM_SUBOP_RESTRICT_LIT                      "RS"
#define COM_SUBOP_UNIQUE_LIT                        "UQ"

// Audit object literals
#define COM_AUD_UNKNOWN_OBJECT_LIT                  "  "
#define COM_AUD_BASE_TABLE_OBJECT_LIT               COM_BASE_TABLE_OBJECT_LIT
#define COM_AUD_CATALOG_OBJECT_LIT                  "CA"
#define COM_AUD_COMPONENT_OBJECT_LIT                "CO"
#define COM_AUD_COMPONENT_PRIVILEGE_OBJECT_LIT      "CP"
#define COM_AUD_INDEX_OBJECT_LIT                    COM_INDEX_OBJECT_LIT
#define COM_AUD_LIBRARY_OBJECT_LIT                  COM_LIBRARY_OBJECT_LIT
#define COM_AUD_MV_OBJECT_LIT                       COM_MV_OBJECT_LIT
#define COM_AUD_MVRG_OBJECT_LIT                     COM_MVRG_OBJECT_LIT
#define COM_AUD_ROLE_OBJECT_LIT                     "RO"
#define COM_AUD_ROUTINE_ACTION_OBJECT_LIT           "RA"
#define COM_AUD_SCHEMA_OBJECT_LIT                   "SC"
#define COM_AUD_STORED_PROCEDURE_OBJECT_LIT         COM_STORED_PROCEDURE_OBJECT_LIT
#define COM_AUD_TRIGGER_OBJECT_LIT                  COM_TRIGGER_OBJECT_LIT
#define COM_AUD_USER_DEFINED_ROUTINE_OBJECT_LIT     COM_USER_DEFINED_ROUTINE_OBJECT_LIT
#define COM_AUD_VIEW_OBJECT_LIT                     COM_VIEW_OBJECT_LIT

// for parsing and implementing authentication option for Register User
enum ComAuthenticationType{ 
   COM_UNKNOWN_AUTH,
   COM_PRIMARY_AUTH,
   COM_SECONDARY_AUTH,
   COM_DEFAULT_AUTH
};
// Security Audit Logging Object Changes Operations Literals
#define COM_DBS_OBJ_CHGS_CREATE_TABLE_LIT                       "CT"

// Security Audit Logging Object-specific Object Subtypes
#define COM_DBS_OBJ_CHGS_NO_SUBTYPE_LIT                         "NS"

// Security Audit Logging User Management Operations Literals
#define COM_DBS_USER_MGMT_ALTER_USER_LIT                        "AU"
#define COM_DBS_USER_MGMT_ALTER_USER_SET_IMMUTABLE_LIT          "SI"
#define COM_DBS_USER_MGMT_ALTER_USER_RESET_IMMUTABLE_LIT        "RI"
#define COM_DBS_USER_MGMT_ALTER_USER_SET_LOGON_ROLE_LIT         "LR"
#define COM_DBS_USER_MGMT_ALTER_USER_SET_EXTERNAL_NAME_LIT      "EN"
#define COM_DBS_USER_MGMT_ALTER_USER_SET_IS_VALID_USER_LIT      "IV" // online
#define COM_DBS_USER_MGMT_ALTER_USER_SET_IS_NOT_VALID_USER_LIT  "NV" // offline
#define COM_DBS_USER_MGMT_ALTER_USER_SET_AUTH_LIT               "AT"
#define COM_DBS_USER_MGMT_ALTER_USER_SET_UNKNOWN_SUB_OP_LIT     "UN"
#define COM_DBS_USER_MGMT_REGISTER_USER_LIT                     "RU"
#define COM_DBS_USER_MGMT_UNREGISTER_USER_LIT                   "UR"
#define COM_DBS_USER_MGMT_REGISTER_NEO_ROLE_LIT                 "NR"
#define COM_DBS_USER_MGMT_CREATE_ROLE_LIT                       "CR"
#define COM_DBS_USER_MGMT_DROP_ROLE_LIT                         "DR"
#define COM_DBS_USER_MGMT_GRANT_ROLE_LIT                        "GR"
#define COM_DBS_USER_MGMT_REVOKE_ROLE_LIT                       "RR"
#define COM_DBS_USER_MGMT_WITH_GRANT_LIT                        "WG"
#define COM_DBS_USER_MGMT_WITHOUT_GRANT_LIT                     "NG"

// Boolean Literals
#define COM_DBS_SUCCEED_LIT    "S"
#define COM_DBS_FAIL_LIT       "F"
#define COM_DBS_YES_LIT        "Y"
#define COM_DBS_NO_LIT         "N"

// values used during ORC file writes if not specified as part
// of table creation.
#define ORC_DEFAULT_STRIPE_SIZE         67108864
#define ORC_DEFAULT_ROW_INDEX_STRIDE    10000
#define ORC_DEFAULT_COMPRESSION         "ZLIB"
#define ORC_DEFAULT_BLOOM_FILTER_FPP    0.05

// values used during Parquet file writes if not specified as part
// of table creation.
#define PARQUET_DEFAULT_BLOCK_SIZE      134217728
#define PARQUET_DEFAULT_PAGE_SIZE       1048576
#define PARQUET_DEFAULT_COMPRESSION     "UNCOMPRESSED"
#define PARQUET_DEFAULT_DICTIONARY_PAGE_SIZE 1048576


// used with removeNATable for QI support
enum ComQiScope 
  {
    REMOVE_FROM_ALL_USERS = 100,
    REMOVE_MINE_ONLY
  };


//
// (Maximum) size of TEXT.TEXT metadata column in bytes (for NSK) or NAWchars (for SeaQuest)
//

#ifndef COM_TEXT__TEXT__MD_COL_MAX_SIZE
#define COM_TEXT__TEXT__MD_COL_MAX_SIZE 3000
#endif // ! defined(COM_TEXT__TEXT__MD_COL_MAX_SIZE)

// max size 16Mb
#define MAX_CHAR_COL_LENGTH_IN_BYTES 16777216
#define MAX_CHAR_COL_LENGTH_IN_BYTES_STR "16777216"

enum BMOQuotaRatio 
 {
    NO_RATIO = -1,
    MIN_QUOTA = 0,
    MAX_QUOTA = 1 
 };

//
// Definition of class ComUID
//

class ComUID
{
  friend void   print_ComUID_with_text(FILE *fp, char *text, ComUID value);
  friend ComUID read_ComUID_with_text(FILE *fp, char *text);
  friend ostream & operator<<(ostream &s, const ComUID &uid);

 public:
  // constructors
  ComUID()
  {
    data = Int64(0L);
  }

 ComUID(const Int64 num)
  {
    data = num;
  }

  // ---------------------------------------------------------------------
  // Compare.
  // ---------------------------------------------------------------------
  short operator==(const ComUID& value) const
  {
    return (data == value.data);
  }

  short operator!=(const ComUID& value) const
  {
    return (data != value.data);
  }

  short operator<(const ComUID& value) const
  {
    return (data < value.data);
  }

  short operator<=(const ComUID& value) const
  {
    return (data <= value.data);
  }

  short operator>(const ComUID& value) const
  {
    return (data > value.data);
  }

  short operator>=(const ComUID& value) const
  {

    return (data >= value.data);
  }


  void  make_UID();

  ComBoolean is_valid_UID() const
  {
    if (this->data == Int64(Lng32(0)))
      return FALSE;
    else
      return TRUE;
  } ;

  Int64 get_value() const
  {
    return data;
  } ;

  Int64 getKey() const
  {
    return data;
  } ;



  Int64 castToInt64()
  {
    return data;
  }

  inline const Int64 castToInt64() const
  {
    return data;
  }

  void convertTo19BytesFixedWidthStringWithZeroesPrefix (ComString &out) const;

  protected:
    Int64 data;

};

Int64  ComSmallDef_local_GetTimeStamp();

void   print_ComUID_with_text(FILE *fp, char *text, ComUID value);
ComUID read_ComUID_with_text(FILE *fp, char *text);
ostream & operator<<(ostream &s, const ComUID &uid);

const char* comObjectTypeLit(ComObjectType objType);
const char* comObjectTypeName(ComObjectType objType);

#endif // COMSMALLDEFS_H
