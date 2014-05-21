// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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

/*
 ********************************************************************************
 * 
 * File:         CmpSeabaseDDLmd.h
 * Description:  This file describes metadata tables used by Trafodion as 
 *               ComTdbVirtTableColumnInfo and ComTdbVirtTableKeyInfo structures 
 *               These structures are defined in comexe/ComTdb.h
 *                
 *               The first section contains the current metadata definition for
 *               this release.
 *
 *               The remaining sections contain metadata definitions for prior
 *               versions.
 *
 *               Version definitions are defined in CmpSeabaseDDL.h and are
 *               persistent in the VERSIONS metadata table.
 *
 * *****************************************************************************
 */

#ifndef _CMP_SEABASE_MD_H_
#define _CMP_SEABASE_MD_H_

#define COL_MAX_CATALOG_LEN 256
#define COL_MAX_SCHEMA_LEN 256
#define COL_MAX_TABLE_LEN 256
#define COL_MAX_COLUMN_LEN 256
#define COL_MAX_EXT_LEN 1024
#define COL_MAX_LIB_LEN 512

#define MD_COL_DATATYPE REC_BYTE_V_ASCII

#define MD_COL_CHARSET SQLCHARSETCODE_UTF8

#define TEXTLEN 10000


///////////////////////////////////////////////////////////////////////////////
// *** current version ***
// Current metadata tables definition for Metadata Version 2.2
//  (Major version = 2, Minor version = 2)
///////////////////////////////////////////////////////////////////////////////

static const ComTdbVirtTableColumnInfo seabaseMDColumnsColInfo[] =
  {                                                                                     
    { "OBJECT_UID",             0, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_NAME",         1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_COLUMN_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_NUMBER",    2, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_CLASS",        3, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "FS_DATA_TYPE",          4, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "COLUMN_SIZE",           5, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "COLUMN_PRECISION", 6, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "COLUMN_SCALE",        7, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "DATETIME_START_FIELD", 8, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,   4,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"9", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "DATETIME_END_FIELD", 9, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,   4,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"10", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_UPSHIFTED",           10, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"11", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "COLUMN_FLAGS",        11, COM_USER_COLUMN_LIT, REC_BIN32_UNSIGNED, 4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"12", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "NULLABLE",                 12, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"13", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "CHARACTER_SET",      13, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     40,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"14", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "DEFAULT_CLASS",       14, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"15", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "DEFAULT_VALUE",     15, COM_USER_COLUMN_LIT, REC_BYTE_V_DOUBLE,    1024,   FALSE, SQLCHARSETCODE_UCS2, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"16", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_HEADING",         16, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_COLUMN_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"17", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "HBASE_COL_FAMILY", 17, COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII,      40,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"18", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "HBASE_COL_QUALIFIER", 18, COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII, 40,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"19", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "DIRECTION",           19, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"20", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "IS_OPTIONAL",           20, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"21", COM_UNKNOWN_DIRECTION_LIT, 0} 
  };

static const ComTdbVirtTableKeyInfo seabaseMDColumnsKeyInfo[] =
  {
    // columnname     keyseqnumber            tablecolnumber       ordering
    {    "OBJECT_UID",                1,                                0,            0  , 0 },
    {    "COLUMN_NAME",            2,                                1,            0  , 0 }
  };


static const ComTdbVirtTableColumnInfo seabaseMDObjectsColInfo[] =
  {                                                                                     
    { "CATALOG_NAME",  0, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_CATALOG_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "SCHEMA_NAME",    1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_SCHEMA_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "OBJECT_NAME",     2,  COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_TABLE_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "OBJECT_TYPE",     3,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "OBJECT_UID",       4,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CREATE_TIME",     5,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "REDEF_TIME",      6,    COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "VALID_DEF",         7,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8", COM_UNKNOWN_DIRECTION_LIT, 0},
   { "OBJECT_OWNER",    8, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"9", COM_UNKNOWN_DIRECTION_LIT, 0},
  };

static const ComTdbVirtTableKeyInfo seabaseMDObjectsKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "CATALOG_NAME",           1,                                0,            0  , 0},
    {    "SCHEMA_NAME",            2,                                 1,            0  , 0},
    {    "OBJECT_NAME",             3,                                 2,            0  , 0},
    {    "OBJECT_TYPE",              4,                                 3,            0  , 0}
  };

static const ComTdbVirtTableIndexInfo seabaseMDObjectsUniqIdxIndexInfo[] =
  {                                                                                     
    { TRAFODION_SYSCAT_LIT".""\""SEABASE_MD_SCHEMA"\"""."SEABASE_OBJECTS,  TRAFODION_SYSCAT_LIT".""\""SEABASE_MD_SCHEMA"\"""."SEABASE_OBJECTS_UNIQ_IDX, 1, 1, 1, 1, 4, 0, 0 }
  };
 
static const ComTdbVirtTableColumnInfo seabaseMDObjectsUniqIdxColInfo[] =
  {       
    { "OBJECT_UID@",       0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"@1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CATALOG_NAME",  1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_CATALOG_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"@2", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "SCHEMA_NAME",    2, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_SCHEMA_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"@3", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "OBJECT_NAME",     3,  COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_TABLE_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"@4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "OBJECT_TYPE",     4,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"@5", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDObjectsUniqIdxKeyInfo[] =
  {
    // columnname        keyseqnumber            tablecolnumber       ordering  nonkeycol  hbaseCF            hbaseCQ
    {    "OBJECT_UID@",                      1,                                4,                 0,   0,             SEABASE_DEFAULT_COL_FAMILY, "@1"}
  };

static const ComTdbVirtTableKeyInfo seabaseMDObjectsUniqIdxNonKeyInfo[] =
  {
    // columnname        keyseqnumber            tablecolnumber       ordering  nonkeycol     hbaseCF            hbaseCQ
    {    "CATALOG_NAME",                  2,                                 0,                0  ,              1,  SEABASE_DEFAULT_COL_FAMILY,  "@2"},
    {    "SCHEMA_NAME",                    3,                                 1,                0  ,             1,   SEABASE_DEFAULT_COL_FAMILY, "@3"},
    {    "OBJECT_NAME",                     4,                                 2,                0  ,             1,   SEABASE_DEFAULT_COL_FAMILY, "@4"},
    {    "OBJECT_TYPE",                      5,                                 3,                0  ,             1,   SEABASE_DEFAULT_COL_FAMILY, "@5"}
  };

static const ComTdbVirtTableKeyInfo seabaseMDObjectsUniqIdxOnlyKeyInfo[] =
  {
    // columnname        keyseqnumber            tablecolnumber       ordering  nonkeycol
    {    "OBJECT_UID@",                      1,                                0,                 0,   0,   SEABASE_DEFAULT_COL_FAMILY, "@1"}
  };

static const ComTdbVirtTableColumnInfo seabaseMDKeysColInfo[] =
  {                                                                                     
    { "OBJECT_UID",             0, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_NAME",         1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_COLUMN_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "KEYSEQ_NUMBER",     2, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_NUMBER",    3, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "ORDERING",                4, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "NONKEYCOL",             5, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6" , COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDKeysKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "OBJECT_UID",                1,                                0,            0  , 0 },
    {    "KEYSEQ_NUMBER",        2,                                 2,            0  , 0 }
  };

static const ComTdbVirtTableColumnInfo seabaseMDIndexesColInfo[] =
  {                                                                                     
    { "BASE_TABLE_UID",      0, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "KEYTAG",                   1, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_UNIQUE",               2, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "KEY_COLCOUNT",       3, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "NONKEY_COLCOUNT", 4, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8" , COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_EXPLICIT",             5, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"9", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "INDEX_UID",      6, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"10", COM_UNKNOWN_DIRECTION_LIT, 0},
  };

static const ComTdbVirtTableKeyInfo seabaseMDIndexesKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "BASE_TABLE_UID",         1,                               0,             0  , 0},
    {    "INDEX_UID",                   2,                               6,             0  , 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDTablesColInfo[] =
  {                                                                                     
    { "TABLE_UID",                        0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_AUDITED",                       1,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "HBASE_CREATE_OPTIONS",  2,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII,     6000, FALSE , SQLCHARSETCODE_UTF8, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDTablesKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber            ordering
    {    "TABLE_UID",                    1,                                0,            0  , 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDViewsColInfo[] =
  {                                                                                     
    { "VIEW_UID",            0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CHECK_OPTION",  1,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_UPDATABLE",    2, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_INSERTABLE",   3, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
  };

static const ComTdbVirtTableKeyInfo seabaseMDViewsKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "VIEW_UID",         1,                               0,                     0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDViewsUsageColInfo[] =
  {                                                                                     
    { "USING_VIEW_UID",            0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "USED_OBJECT_UID",         1,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "USED_OBJECT_TYPE",     2,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDViewsUsageKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "USING_VIEW_UID",         1,                               0,                     0, 0},
    {    "USED_VIEW_UID",          2,                               1,                     0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDDefaultsColInfo[] =
  {   
    { "ATTRIBUTE",         0, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,  100, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "ATTR_VALUE",       1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,  1000, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "ATTR_COMMENT",  2, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,  1000, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDDefaultsKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "ATTRIBUTE",         1,                               0,                     0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDVersionsColInfo[] =
  {   
    { "VERSION_TYPE",         0, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,  50, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "MAJOR_VERSION",         1,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "MINOR_VERSION",         2,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "INIT_TIME",                  3,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COMMENT",                 4, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,  1000, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDVersionsKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "VERSION_TYPE",         1,                               0,                     0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDTableConstraintsColInfo[] =
  {                                                                                     
    { "TABLE_UID",            0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CONSTRAINT_UID",            1,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CONSTRAINT_TYPE",  2,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COL_COUNT",       3, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "INDEX_UID",            4,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDTableConstraintsKeyInfo[] =
  {
    // columnname          keyseqnumber            tablecolnumber       ordering
    {    "TABLE_UID",                   1,                               0,                     0, 0},
    {    "CONSTRAINT_UID",         2,                               1,                     0, 0},
    {    "CONSTRAINT_TYPE",       3,                               2,                     0, 0}

  };

static const ComTdbVirtTableColumnInfo seabaseMDTextColInfo[] =
  {                                                                                     
    { "OBJECT_UID",            0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "SEQ_NUM",                 1, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "TEXT",                       2,   COM_USER_COLUMN_LIT, MD_COL_DATATYPE,   TEXTLEN,     FALSE, MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDTextKeyInfo[] =
  {
    // columnname          keyseqnumber            tablecolnumber       ordering
    {    "OBJECT_UID",                   1,                               0,                     0, 0},
    {    "SEQ_NUM",                        2,                               1,                     0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDRefConstraintsColInfo[] =
  {                                                                                     
    { "REF_CONSTRAINT_UID",            0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "UNIQUE_CONSTRAINT_UID",            1,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "MATCH_OPTION",  2,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
   { "UPDATE_RULE",  3,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
   { "DELETE_RULE",  4,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDRefConstraintsKeyInfo[] =
  {
    // columnname                      keyseqnumber            tablecolnumber       ordering
    {    "REF_CONSTRAINT_UID",                   1,                               0,                 0, 0},
    {    "UNIQUE_CONSTRAINT_UID",             2,                               1,                 0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDUniqueRefConstrUsageColInfo[] =
  {                                                                                     
    { "UNIQUE_CONSTRAINT_UID",            0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "FOREIGN_CONSTRAINT_UID",            1,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDUniqueRefConstrUsageKeyInfo[] =
  {
    // columnname                      keyseqnumber            tablecolnumber       ordering
    {  "UNIQUE_CONSTRAINT_UID",                   1,                               0,                 0, 0},
    {  "FOREIGN_CONSTRAINT_UID",                 2,                               1,                 0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDRoutinesColInfo[] =
  {
    { "UDR_UID",             0, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "UDR_TYPE",            1, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "LANGUAGE_TYPE",       2, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "DETERMINISTIC_BOOL",  3, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "SQL_ACCESS",          4, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CALL_ON_NULL",        5, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "ISOLATE_BOOL",        6, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "PARAM_STYLE",         7, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "TRANSACTION_ATTRIBUTES", 8, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"9", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "MAX_RESULTS",         9, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,   4,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"10", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "STATE_AREA_SIZE",    10, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,   4,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"11", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "EXTERNAL_NAME",      11, COM_USER_COLUMN_LIT, MD_COL_DATATYPE, COL_MAX_EXT_LEN,   FALSE, MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"12", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "PARALLELISM",        12, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,    2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"13", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "USER_VERSION",       13, COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII,   32,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"14", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "EXTERNAL_SECURITY",  14, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,    2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"15", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "EXECUTION_MODE",     15, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,    2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"16", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "LIBRARY_UID",        16, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"17", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "SIGNATURE",         17,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII,    8192, FALSE , SQLCHARSETCODE_UTF8, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"18", COM_UNKNOWN_DIRECTION_LIT, 0}

  };

static const ComTdbVirtTableKeyInfo seabaseMDRoutinesKeyInfo[] =
  {
    //  columnname      keyseqnumber                      tablecolnumber       ordering
    {    "UDR_UID",         1,                               0,                     0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDLibrariesColInfo[] =
  {
    { "LIBRARY_UID",             0, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "LIBRARY_FILENAME",        1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE, COL_MAX_LIB_LEN,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "VERSION",                 2, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,   4,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseMDLibrariesKeyInfo[] =
  {
    //  columnname      keyseqnumber                      tablecolnumber       ordering
    { "LIBRARY_UID",         1,                               0,                     0, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDLibrariesUsageColInfo[] =
  {
    { "USING_LIBRARY_UID",       0, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "USED_UDR_UID",            1, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
  };

static const ComTdbVirtTableKeyInfo seabaseMDLibrariesUsageKeyInfo[] =
  {
    //  columnname      keyseqnumber                      tablecolnumber       ordering
    {    "USING_LIBRARY_UID",    1,                               0,                     0, 0},
    //  columnname      keyseqnumber                      tablecolnumber       ordering
    {    "USED_UDR_UID",         2,                               1,                     0, 0}
  };

static const ComTdbVirtTableRoutineInfo seabaseMDValidateRoutineInfo =
  {
    "VALIDATEROUTINE", COM_PROCEDURE_TYPE_LIT,  COM_LANGUAGE_JAVA_LIT, 1, COM_NO_SQL_LIT, 0, 0, COM_STYLE_JAVA_LIT, COM_NO_TRANSACTION_REQUIRED_LIT, 0, 0, "org.trafodion.sql.udr.LmUtility.validateMethod", COM_ROUTINE_NO_PARALLELISM_LIT, " ", COM_ROUTINE_EXTERNAL_SECURITY_INVOKER_LIT, COM_ROUTINE_SAFE_EXECUTION_LIT, " ", 1, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;III[Ljava/lang/String;[I[Ljava/lang/String;)V"   
  };

static const ComTdbVirtTableColumnInfo seabaseMDValidateRoutineColInfo[] =
  {        
    { "CLASSNAME", 0,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII, 256, FALSE , SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_INPUT_COLUMN_LIT, 0},
    { "METHODNAME", 1,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII, 256, FALSE , SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_INPUT_COLUMN_LIT, 0},
    { "EXTERNALPATH", 2,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII, 256, FALSE , SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_INPUT_COLUMN_LIT, 0},
    { "SIGNATURE", 3,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII, 8192, FALSE , SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_INPUT_COLUMN_LIT, 0},
    { "NUMPARAM", 4,  COM_USER_COLUMN_LIT, REC_BIN32_SIGNED, 4, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_INPUT_COLUMN_LIT, 0},
    { "MAXRESULTSETS", 5,  COM_USER_COLUMN_LIT, REC_BIN32_SIGNED, 4, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_INPUT_COLUMN_LIT, 0},
    { "OPTIONALSIG", 6,  COM_USER_COLUMN_LIT, REC_BIN32_SIGNED, 4, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_INPUT_COLUMN_LIT, 0},
     { "RETSIG", 7,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII, 8192, FALSE , SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8", COM_OUTPUT_COLUMN_LIT, 0},
    { "ERRCODE", 8,  COM_USER_COLUMN_LIT, REC_BIN32_SIGNED, 4, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"9", COM_OUTPUT_COLUMN_LIT, 0},
     { "ERRDETAIL", 9,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII, 8192, FALSE , SQLCHARSETCODE_ISO88591, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"10", COM_OUTPUT_COLUMN_LIT, 0}
  };

static const ComTdbVirtTableColumnInfo seabaseMDAuthsColInfo[] =
  {
    { "AUTH_ID",         0, COM_USER_COLUMN_LIT, REC_BIN32_UNSIGNED,  4, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "AUTH_DB_NAME",         1,   COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_TABLE_LEN,     FALSE, MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "AUTH_EXT_NAME",         2,   COM_USER_COLUMN_LIT, MD_COL_DATATYPE,   COL_MAX_TABLE_LEN,     FALSE, MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "AUTH_TYPE",                  3,   COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "AUTH_CREATOR",                  4,   COM_USER_COLUMN_LIT, REC_BIN32_UNSIGNED,   4,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "AUTH_IS_VALID",                 5, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,  2, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "AUTH_REDEF_TIME",      6,    COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "AUTH_CREATE_TIME",     7,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8", COM_UNKNOWN_DIRECTION_LIT, 0}
  };
static const ComTdbVirtTableKeyInfo seabaseMDAuthsKeyInfo[] =
  {
    {    "AUTH_ID",         1,                               0,                     0, 0}
  };


/////////////////////////////////////////////////////////////////////
//
// Information about changed old metadata tables from which upgrade
// is being done to the current version.
// These definitions have changed in the current version of code.
// 
// Old definitions have the form (for ex for COLUMNS table):
//            seabaseOldMDv??ColumnsColInfo[]
// v?? is the old version.
//
// When definitions change, make new entries between
// START_OLD_MD_v?? and END_OLD_MD_v??.
// Do not remove older entries. We want to keep them around for
// historical purpose.
//
// Change entries in allMDtablesInfo[] struct in CmpSeabaseDDLupgrade.h
// to reflect the 'old' metadata.
//
//////////////////////////////////////////////////////////////////////
#define OLD_MD_EXTENSION "_OLD_MD"

#define SEABASE_COLUMNS_OLD_MD           SEABASE_COLUMNS"_OLD_MD"
#define SEABASE_DEFAULTS_OLD_MD          SEABASE_DEFAULTS"_OLD_MD"
#define SEABASE_INDEXES_OLD_MD            SEABASE_INDEXES"_OLD_MD"
#define SEABASE_KEYS_OLD_MD                 SEABASE_KEYS"_OLD_MD"
#define SEABASE_LIBRARIES_OLD_MD         SEABASE_LIBRARIES"_OLD_MD"
#define SEABASE_LIBRARIES_USAGE_OLD_MD  SEABASE_LIBRARIES_USAGE"_OLD_MD"
#define SEABASE_OBJECTS_OLD_MD           SEABASE_OBJECTS"_OLD_MD"
#define SEABASE_OBJECTS_UNIQ_IDX_OLD_MD           SEABASE_OBJECTS_UNIQ_IDX"_OLD_MD"
#define SEABASE_OBJECTUID_OLD_MD       SEABASE_OBJECTUID"_OLD_MD"
#define SEABASE_REF_CONSTRAINTS_OLD_MD SEABASE_REF_CONSTRAINTS"_OLD_MD"
#define SEABASE_ROUTINES_OLD_MD          SEABASE_ROUTINES"_OLD_MD"
#define SEABASE_TABLES_OLD_MD              SEABASE_TABLES"_OLD_MD"
#define SEABASE_TABLE_CONSTRAINTS_OLD_MD SEABASE_TABLE_CONSTRAINTS"_OLD_MD"
#define SEABASE_TEXT_OLD_MD                 SEABASE_TEXT"_OLD_MD"
#define SEABASE_UNIQUE_REF_CONSTR_USAGE_OLD_MD SEABASE_UNIQUE_REF_CONSTR_USAGE"_OLD_MD"
#define SEABASE_VIEWS_OLD_MD                SEABASE_VIEWS"_OLD_MD"
#define SEABASE_VIEWS_USAGE_OLD_MD    SEABASE_VIEWS_USAGE"_OLD_MD"
#define SEABASE_VERSIONS_OLD_MD          SEABASE_VERSIONS"_OLD_MD"

////////////////////////////////////////////////////////////////////////
//// START_OLD_MD_v21: 
////        OLD metadata Version 2.1  (Major version = 2, Minor version = 1).
//////////////////////////////////////////////////////////////////////////
static const ComTdbVirtTableColumnInfo seabaseOldMDv21ObjectsColInfo[] =
  {
    { "CATALOG_NAME",  0, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_CATALOG_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "SCHEMA_NAME",    1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_SCHEMA_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "OBJECT_NAME",     2,  COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_TABLE_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "OBJECT_TYPE",     3,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2, FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "OBJECT_UID",       4,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CREATE_TIME",     5,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "REDEF_TIME",      6,    COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "VALID_DEF",         7,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,   2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

////////////////////////////////////////////////////////////////////////
//// END_OLD_MD_v21: 
////        OLD metadata Version 2.1  (Major version = 2, Minor version = 1).
//////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
// START_OLD_MD_v11: 
//        OLD metadata Version 1.1  (Major version = 1, Minor version = 1).
////////////////////////////////////////////////////////////////////////
static const ComTdbVirtTableColumnInfo seabaseOldMDv11ColumnsColInfo[] =
  {                                                                                     
    { "OBJECT_UID",             0, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_NAME",         1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_COLUMN_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_NUMBER",    2, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_CLASS",        3, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "FS_DATA_TYPE",          4, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "COLUMN_SIZE",           5, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "COLUMN_PRECISION", 6, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "COLUMN_SCALE",        7, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,    4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "DATETIME_START_FIELD", 8, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,   4,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"9", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "DATETIME_END_FIELD", 9, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,   4,   FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"10", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_UPSHIFTED",           10, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"11", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "COLUMN_FLAGS",        11, COM_USER_COLUMN_LIT, REC_BIN32_UNSIGNED, 4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"12", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "NULLABLE",                 12, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"13", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "CHARACTER_SET",      13, COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     40,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"14", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "DEFAULT_CLASS",       14, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"15", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "DEFAULT_VALUE",     15, COM_USER_COLUMN_LIT, REC_BYTE_V_DOUBLE,    1024,   FALSE, SQLCHARSETCODE_UCS2, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"16", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "COLUMN_HEADING",         16, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_COLUMN_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"17", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "HBASE_COL_FAMILY", 17, COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII,      40,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"18", COM_UNKNOWN_DIRECTION_LIT, 0}, 
    { "HBASE_COL_QUALIFIER", 18, COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII, 40,  FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"19", COM_UNKNOWN_DIRECTION_LIT, 0},
  };

static const ComTdbVirtTableColumnInfo seabaseOldMDv11IndexesColInfo[] =
  {                                                                                     
    { "BASE_TABLE_UID",      0, COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CATALOG_NAME",       1, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_CATALOG_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "SCHEMA_NAME",         2, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_SCHEMA_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},  
    { "INDEX_NAME",            3, COM_USER_COLUMN_LIT, MD_COL_DATATYPE,    COL_MAX_TABLE_LEN, FALSE , MD_COL_CHARSET, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "KEYTAG",                   4, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_UNIQUE",               5, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"6", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "KEY_COLCOUNT",       6, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"7", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "NONKEY_COLCOUNT", 7, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"8", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

static const ComTdbVirtTableKeyInfo seabaseOldMDv11IndexesKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "BASE_TABLE_UID",         1,                               0,             0  , 0},
    {    "CATALOG_NAME",          2,                                1,             0  , 0},
    {    "SCHEMA_NAME",            3,                                2,             0  , 0},
    {    "INDEX_NAME",               4,                                3,             0  , 0}
  };

static const ComTdbVirtTableKeyInfo seabaseOldMDv11ObjectsKeyInfo[] =
  {
    // columnname keyseqnumber            tablecolnumber       ordering
    {    "CATALOG_NAME",           1,                                0,            0  , 0},
    {    "SCHEMA_NAME",            2,                                 1,            0  , 0},
    {    "OBJECT_NAME",             3,                                 2,            0  , 0}
  };

static const ComTdbVirtTableColumnInfo seabaseOldMDv11ViewsColInfo[] =
  {                                                                                     
    { "VIEW_UID",            0,   COM_USER_COLUMN_LIT, REC_BIN64_SIGNED,   8,     FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"1", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "CHECK_OPTION",  1,  COM_USER_COLUMN_LIT, REC_BYTE_F_ASCII,     2,     FALSE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"2", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_UPDATABLE",    2, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"3", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "IS_INSERTABLE",   3, COM_USER_COLUMN_LIT, REC_BIN32_SIGNED,     4,    FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"4", COM_UNKNOWN_DIRECTION_LIT, 0},
    { "VIEW_TEXT",         4,  COM_USER_COLUMN_LIT, REC_BYTE_V_ASCII,     6000, FALSE , SQLCHARSETCODE_UTF8, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",SEABASE_DEFAULT_COL_FAMILY,"5", COM_UNKNOWN_DIRECTION_LIT, 0}
  };

////////////////////////////////////////////////////////////////////////
// END_OLD_MD_v11: 
//        OLD metadata Version 1.1  (Major version = 1, Minor version = 1).
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// Metadata views.
////////////////////////////////////////////////////////////////////////
struct QString {
public:
  const char * str;
};

#define TRAF_COLUMNS_VIEW "COLUMNS_VIEW"
#define TRAF_INDEXES_VIEW "INDEXES_VIEW"
#define TRAF_KEYS_VIEW "KEYS_VIEW"
#define TRAF_REF_CONSTRAINTS_VIEW "REF_CONSTRAINTS_VIEW"
#define TRAF_TABLES_VIEW "TABLES_VIEW"
#define TRAF_VIEWS_VIEW "VIEWS_VIEW"

static const QString createTrafColumnsViewQuery[] =
{
  {" create view %s.\"%s\"."TRAF_COLUMNS_VIEW" as "},
  {" select O.catalog_name, O.schema_name, O.object_name table_name, "},
  {"           C.column_name, C.column_number, "},
  {"           cast( case "},
  {"           when C.fs_data_type = 130 then '"COM_SMALLINT_SIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 131 then '"COM_SMALLINT_UNSIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 132 then '"COM_INTEGER_SIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 133 then '"COM_INTEGER_UNSIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 134 then '"COM_LARGEINT_SIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 135 then '"COM_SMALLINT_UNSIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 140 then '"COM_REAL_SDT_LIT"' "},
  {"           when C.fs_data_type = 141 then '"COM_DOUBLE_SDT_LIT"' "},
  {"           when C.fs_data_type = 150 then '"COM_DECIMAL_UNSIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 151 then '"COM_DECIMAL_SIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 155 then '"COM_NUMERIC_UNSIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 156 then '"COM_NUMERIC_SIGNED_SDT_LIT"' "},
  {"           when C.fs_data_type = 0     then '"COM_CHARACTER_SDT_LIT"' "},
  {"           when C.fs_data_type = 2     then '"COM_CHARACTER_SDT_LIT"' "},
  {"           when C.fs_data_type = 70    then '"COM_LONG_VARCHAR_SDT_LIT"' "},
  {"           when C.fs_data_type = 64    then '"COM_VARCHAR_SDT_LIT"' "},
  {"           when C.fs_data_type = 66    then '"COM_VARCHAR_SDT_LIT"' "},
  {"           when C.fs_data_type = 100   then '"COM_VARCHAR_SDT_LIT"' "},
  {"           when C.fs_data_type = 101   then '"COM_VARCHAR_SDT_LIT"' "},
  {"           when C.fs_data_type = 192 then '"COM_DATETIME_SDT_LIT"' "},
  {"           when C.fs_data_type >= 196 and C.fs_data_type <= 207 then '"COM_INTERVAL_SDT_LIT"' "},
  {"           else 'UNKNOWN' end as char(24)) sql_data_type,                              "},
  {"           C.fs_data_type, C.column_size, C.column_precision, C.column_scale, "},
  {"           C.nullable, C.character_set, "},
  {"           C.datetime_start_field, C.datetime_end_field, "},
  {"           case "},
  {"           when C.fs_data_type = 192 and C.datetime_start_field = 1 and C.datetime_end_field = 3 then 1 "},
  {"           when C.fs_data_type = 192 and C.datetime_start_field = 1 and C.datetime_end_field = 6 then 3 "},
  {"           when C.fs_data_type = 192 and C.datetime_start_field = 4 and C.datetime_end_field = 6 then 2 "},
  {"           else 0 "},
  {"           end dt_code, "},
  {"           cast (case "},
  {"           when C.fs_data_type = 192 and C.datetime_end_field = 6 "},
  {"             then '(' || trim(cast(C.column_scale as varchar(2))) || ')'  else ' ' "},
  {"           end as char(28)) datetime_qualifier, "},
  {"           translate(trim(C.default_value) using ucs2toutf8) default_value  "},
  {"   from %s.\"%s\".\"%s\" O, "},
  {"   %s.\"%s\".\"%s\" C "},
  {"  where O.catalog_name = '%s' "},
  {"        and O.schema_name != '%s' "},
  {"        and O.object_uid = C.object_uid "},
  {"        and O.object_type = '%s' "},
  {"  for read uncommitted access "},
  {"  order by 1,2,3,4 "},
  {"  ; "}
};

static const QString createTrafIndexesViewQuery[] =
{
  {" create view %s.\"%s\"."TRAF_INDEXES_VIEW" as "},
  {" select O.catalog_name, O.schema_name, O.object_name index_name, "},
  {"           O2.object_name table_name, "},
  {"           I.is_unique, I.key_colcount, I.nonkey_colcount "},
  {"   from %s.\"%s\".\"%s\" I, "},
  {"           %s.\"%s\".\"%s\" O, "},
  {"           %s.\"%s\".\"%s\" O2 "},
  {"  where O.catalog_name = '%s' "},
  {"        and O.schema_name != '%s' "},
  {"        and I.index_uid = O.object_uid "},
  {"        and I.base_table_uid = O2.object_uid "},
  {"  for read uncommitted access "},
  {"  order by 1,2,3 "},
  {"  ; "}
};

static const QString createTrafKeysViewQuery[] =
{
  {" create view %s.\"%s\"."TRAF_KEYS_VIEW" as "},
  {" select O.catalog_name, O.schema_name, O.object_name table_name, "},
  {"           O2.object_name constraint_name, "},
  {"           cast(case "},
  {"            when T.constraint_type = 'P' then 'PRIMARY KEY' "},
  {"            when T.constraint_type = 'F' then 'FOREIGN KEY' "},
  {"            when T.constraint_type = 'U' then 'UNIQUE KEY' "},
  {"             else ' ' end as varchar(22)) constraint_type, "},
  {"           K.column_name, K.column_number, "},
  {"           K.keyseq_number ordinal_position, K.ordering "},
  {"   from %s.\"%s\".\"%s\" T, "},
  {"           %s.\"%s\".\"%s\" O, "},
  {"           %s.\"%s\".\"%s\" O2, "},
  {"           %s.\"%s\".\"%s\" K "},
  {"  where O.catalog_name = '%s' "},
  {"        and O.schema_name != '%s' "},
  {"        and T.table_uid = O.object_uid "},
  {"        and T.constraint_uid = O2.object_uid "},
  {"        and T.constraint_uid = K.object_uid "},
  {"  for read uncommitted access "},
  {"  order by 1,2 "},
  {"  ; "}
};

static const QString createTrafRefConstraintsViewQuery[] =
{
  {" create view %s.\"%s\"."TRAF_REF_CONSTRAINTS_VIEW" as "},
  {" select O2.catalog_name, O2.schema_name, O2.object_name table_name, "},
  {"           O.object_name constraint_name, "},
  {"           O3.schema_name unique_constraint_schema_name, "},
  {"           O3.object_name unique_constraint_name   "},
  {"   from %s.\"%s\".\"%s\" R, "},
  {"   %s.\"%s\".\"%s\" O, "},
  {"   %s.\"%s\".\"%s\" O2, "},
  {"   %s.\"%s\".\"%s\" O3, "},
  {"   %s.\"%s\".\"%s\" T "},
  {"  where O.catalog_name = '%s' "},
  {"        and O.schema_name != '%s' "},
  {"        and R.ref_constraint_uid = O.object_uid "},
  {"        and R.unique_constraint_uid = O3.object_uid "},
  {"        and R.ref_constraint_uid = T.constraint_uid "},
  {"        and T.table_uid = O2.object_uid "},
  {"  for read uncommitted access "},
  {"  order by O2.catalog_name, O2.schema_name, constraint_name "},
  {"  ; "}
};

static const QString createTrafTablesViewQuery[] =
{
  {" create view %s.\"%s\"."TRAF_TABLES_VIEW" as "},
  {" select catalog_name, schema_name, object_name table_name "},
  {"   from %s.\"%s\".\"%s\" O "},
  {"  where O.catalog_name = '%s' "},
  {"        and O.schema_name != '%s' "},
  {"        and O.object_type = '%s' "},
  {"  for read uncommitted access "},
  {"  order by 1,2,3 "},
  {"  ; "}
};

static const QString createTrafViewsViewQuery[] =
{
  {" create view %s.\"%s\"."TRAF_VIEWS_VIEW" as "},
  {" select O.catalog_name, O.schema_name, O.object_name view_name, "},
  {"           V.check_option, V.is_updatable, V.is_insertable "},
  {"   from %s.\"%s\".\"%s\" O, "},
  {"   %s.\"%s\".\"%s\" V "},
  {"  where O.catalog_name = '%s' "},
  {"        and O.schema_name != '%s' "},
  {"        and O.object_uid = V.view_uid "},
  {"        and O.object_type = '%s' "},
  {"  for read uncommitted access "},
  {"  order by 1,2,3,6 "},
  {"  ; "}
};

struct MDViewInfo
{
  const char * viewName;

  const QString *viewDefnQuery;

  Lng32 sizeOfDefnArr;
};
 
static const MDViewInfo allMDviewsInfo[] = {
  {
    TRAF_COLUMNS_VIEW,
    createTrafColumnsViewQuery,
    sizeof(createTrafColumnsViewQuery)
  },
  {
    TRAF_INDEXES_VIEW,
    createTrafIndexesViewQuery,
    sizeof(createTrafIndexesViewQuery)
  },
  {
    TRAF_KEYS_VIEW,
    createTrafKeysViewQuery,
    sizeof(createTrafKeysViewQuery)
  },
  {
    TRAF_REF_CONSTRAINTS_VIEW,
    createTrafRefConstraintsViewQuery,
    sizeof(createTrafRefConstraintsViewQuery)
  },
  {
    TRAF_TABLES_VIEW,
    createTrafTablesViewQuery,
    sizeof(createTrafTablesViewQuery)
  },
  {
    TRAF_VIEWS_VIEW,
    createTrafViewsViewQuery,
    sizeof(createTrafViewsViewQuery)
  },

};

#endif
