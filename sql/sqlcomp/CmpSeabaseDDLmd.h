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
#define COL_MAX_ATTRIBUTE_LEN 3

#define MD_COL_DATATYPE REC_BYTE_V_ASCII

#define MD_COL_CHARSET SQLCHARSETCODE_UTF8

#define TEXTLEN 10000

struct QString {
public:
  const char * str;
};

///////////////////////////////////////////////////////////////////////////////
// *** current version ***
// Current metadata tables definition for Metadata Version 2.3
//  (Major version = 2, Minor version = 3)
///////////////////////////////////////////////////////////////////////////////

static const QString seabaseAuthsDDL[] =
{
  {" create table "SEABASE_AUTHS" "},
  {" ( "},
  {"  auth_id int unsigned not null not serialized, "},
  {"  auth_db_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"  auth_ext_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"  auth_type char(2) not null not serialized, "},
  {"  auth_creator int unsigned not null not serialized, "},
  {"  auth_is_valid char(2) not null not serialized, "},
  {"  auth_redef_time largeint not null not serialized, "},
  {"  auth_create_time largeint not null not serialized "},
  {" ) "},
  {" primary key (auth_id) "},
  {" ; "}
};

static const QString seabaseColumnsDDL[] =
{
  {" create table "SEABASE_COLUMNS" "},
  {" ( "},
  {"   object_uid largeint not null not serialized, "},
  {"   column_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   column_number int not null not serialized, "},
  {"   column_class char(2) not null not serialized, "},
  {"   fs_data_type int not null not serialized, "},
  {"   column_size int not null not serialized, "},
  {"   column_precision int not null not serialized, "},
  {"   column_scale int not null not serialized, "},
  {"   datetime_start_field int not null not serialized, "},
  {"   datetime_end_field int not null not serialized, "},
  {"   is_upshifted char(2) not null not serialized, "},
  {"   column_flags int unsigned not null not serialized, "},
  {"   nullable int not null not serialized, "},
  {"   character_set char(40) not null not serialized, "},
  {"   default_class int not null not serialized, "},
  {"   default_value varchar(512) character set ucs2 not null not serialized, "},
  {"   column_heading varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   hbase_col_family varchar(40) not null not serialized, "},
  {"   hbase_col_qualifier varchar(40) not null not serialized, "},
  {"   direction char(2) not null not serialized, "},
  {"   is_optional char(2) not null not serialized "},
  {" ) "},
  {" primary key (object_uid, column_name) "},
  {" ; "}
};

static const QString seabaseDefaultsDDL[] =
{
  {" create table "SEABASE_DEFAULTS" "},
  {" ( "},
  {"   attribute varchar(100 bytes) character set utf8 not null not serialized, "},
  {"   attr_value varchar(1000 bytes) character set utf8 not null not serialized, "},
  {"   attr_comment varchar(1000 bytes) character set utf8 not null not serialized "},
  {" ) "},
  {" primary key (attribute) "},
  {" ; "}
};

static const QString seabaseKeysDDL[] =
{
  {" create table "SEABASE_KEYS" "},
  {" ( "},
  {"   object_uid largeint not null not serialized, "},
  {"   column_name varchar( 256 bytes ) character set utf8 not null not serialized, "},
  {"   keyseq_number int not null not serialized, "},
  {"   column_number int not null not serialized, "},
  {"   ordering int not null not serialized, "},
  {"   nonkeycol int not null not serialized "},
  {" ) "},
  {" primary key (object_uid, keyseq_number) "},
  {" ; "}
};

static const QString seabaseIndexesDDL[] =
{
  {" create table "SEABASE_INDEXES" "},
  {" ( "},
  {"   base_table_uid largeint not null not serialized, "},
  {"   keytag int not null not serialized, "},
  {"   is_unique int not null not serialized, "},
  {"   key_colcount int not null not serialized, "},
  {"   nonkey_colcount int not null not serialized, "},
  {"   is_explicit int not null not serialized, "},
  {"   index_uid largeint not null not serialized "},
  {" ) "},
  {" primary key (base_table_uid, index_uid) "},
  {" ; "}
};

static const QString seabaseLibrariesDDL[] =
{
  {" create table "SEABASE_LIBRARIES" "},
  {" ( "},
  {"   library_uid largeint not null not serialized, "},
  {"   library_filename varchar(512) not null not serialized, "},
  {"   version int not null not serialized "},
  {" ) "},
  {" primary key (library_uid) "},
  {" ; "}
};

static const QString seabaseLibrariesUsageDDL[] =
{
  {" create table "SEABASE_LIBRARIES_USAGE" "},
  {" ( "},
  {"   using_library_uid largeint not null not serialized, "},
  {"   used_udr_uid largeint not null not serialized "},
  {" ) "},
  {" primary key (using_library_uid, used_udr_uid) "},
  {" ; "}
};

static const QString seabaseObjectsDDL[] =
{
  {" create table "SEABASE_OBJECTS" "},
  {" ( "},
  {"   catalog_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   schema_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   object_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   object_type char(2) not null not serialized, "},
  {"   object_uid largeint not null not serialized, "},
  {"   create_time largeint not null not serialized, "},
  {"   redef_time largeint not null not serialized, "},
  {"   valid_def char(2) not null not serialized, "},
  {"   object_owner int not null not serialized "},
  {" ) "},
  {" primary key (catalog_name, schema_name, object_name, object_type) "},
  {" ; "}
};

static const QString seabaseObjectsUniqIdxIndexDDL[] =
{
  {" create unique index "SEABASE_OBJECTS_UNIQ_IDX" on "TRAFODION_SYSCAT_LIT".\""SEABASE_MD_SCHEMA"\"."SEABASE_OBJECTS" "},
  {" ( "},
  {"   object_uid "},
  {" ) "},
  {" ; "}
};

static const QString seabaseObjectsUniqIdxDDL[] =
{
  {" create table "SEABASE_OBJECTS_UNIQ_IDX" "},
  {" ( "},
  {"   \"OBJECT_UID@\" largeint not null not serialized, "},
  {"   catalog_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   schema_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   object_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   object_type char(2) not null not serialized "},
  {" ) "},
  {" primary key (\"OBJECT_UID@\") "},
  {" ; "}
};

static const QString seabaseRefConstraintsDDL[] =
{
  {" create table "SEABASE_REF_CONSTRAINTS" "},
  {" ( "},
  {"   ref_constraint_uid largeint not null not serialized, "},
  {"   unique_constraint_uid largeint not null not serialized, "},
  {"   match_option char(2) not null not serialized, "},
  {"   update_rule char(2) not null not serialized, "},
  {"   delete_rule char(2) not null not serialized "},
  {" ) "},
  {" primary key (ref_constraint_uid, unique_constraint_uid) "},
  {" ; "}
};

static const QString seabaseRoutinesDDL[] =
{
  {" create table "SEABASE_ROUTINES" "},
  {" ( "},
  {"   udr_uid largeint not null not serialized, "},
  {"   udr_type char(2) not  null, "},
  {"   language_type char(2) not null not serialized, "},
  {"   deterministic_bool char(2) not null not serialized, "},
  {"   sql_access char(2) not null not serialized, "},
  {"   call_on_null char(2) not null not serialized, "},
  {"   isolate_bool char(2) not null not serialized, "},
  {"   param_style char(2) not null not serialized, "},
  {"   transaction_attributes char(2) not null not serialized, "},
  {"   max_results int not null not serialized, "},
  {"   state_area_size int not null not serialized, "},
  {"   external_name varchar(1024 bytes) character set utf8 not null not serialized, "},
  {"   parallelism char(2) not null not serialized, "},
  {"   user_version varchar(32) not null not serialized, "},
  {"   external_security char(2) not null not serialized, "},
  {"   execution_mode char(2) not null not serialized, "},
  {"   library_uid largeint not null not serialized, "},
  {"   signature varchar(8192 bytes) character set utf8 not null not serialized "},
  {" ) "},
  {" primary key (udr_uid) "},
  {" ; "}
};

static const QString seabaseTablesDDL[] =
{
  {" create table "SEABASE_TABLES" "},
  {" ( "},
  {"   table_uid largeint not null not serialized, "},
  {"   is_audited char(2) not null not serialized, "},
  {"   hbase_create_options varchar(6000) not null not serialized "},
  {" ) "},
  {" primary key (table_uid) "},
  {" ; "}
};

static const QString seabaseTableConstraintsDDL[] =
{
  {" create table "SEABASE_TABLE_CONSTRAINTS" "},
  {" ( "},
  {"   table_uid largeint not null not serialized, "},
  {"   constraint_uid largeint not null not serialized, "},
  {"   constraint_type char(2) not null not serialized, "},
  {"   col_count int not null not serialized, "},
  {"   index_uid largeint not null not serialized "},
  {" ) "},
  {" primary key (table_uid, constraint_uid, constraint_type) "},
  {" ; "}
};

static const QString seabaseTextDDL[] =
{
  {" create table "SEABASE_TEXT" "},
  {" ( "},
  {"   object_uid largeint not null not serialized, "},
  {"   seq_num int not null not serialized, "},
  {"   text varchar(10000 bytes) character set utf8 not null not serialized "},
  {" ) "},
  {" primary key (object_uid, seq_num) "},
  {" ; "}
};

static const QString seabaseUniqueRefConstrUsageDDL[] =
{
  {" create table "SEABASE_UNIQUE_REF_CONSTR_USAGE" "},
  {" ( "},
  {"   unique_constraint_uid largeint not null not serialized, "},
  {"   foreign_constraint_uid largeint not null not serialized "},
  {" ) "},
  {" primary key (unique_constraint_uid, foreign_constraint_uid) "},
  {" ; "}
};

static const QString seabaseVersionsDDL[] =
{
  {" create table "SEABASE_VERSIONS" "},
  {" ( "},
  {"   version_type char(50 bytes) character set utf8 not null not serialized, "},
  {"   major_version largeint not null not serialized, "},
  {"   minor_version largeint not null not serialized, "},
  {"   init_time largeint not null not serialized, "},
  {"   comment varchar(1000 bytes) character set utf8 not null not serialized "},
  {" ) "},
  {" primary key (version_type) "},
  {" ; "}
};

static const QString seabaseViewsDDL[] =
{
  {" create table "SEABASE_VIEWS" "},
  {" ( "},
  {"   view_uid largeint not null not serialized, "},
  {"   check_option char(2) not null not serialized, "},
  {"   is_updatable int not null not serialized, "},
  {"   is_insertable int not null not serialized "},
  {" ) "},
  {" primary key (view_uid) "},
  {" ; "}
};

static const QString seabaseViewsUsageDDL[] =
{
  {" create table "SEABASE_VIEWS_USAGE" "},
  {" ( "},
  {"   using_view_uid largeint not null not serialized, "},
  {"   used_object_uid largeint not null not serialized, "},
  {"   used_object_type char(2) not null not serialized "},
  {" ) "},
  {" primary key (using_view_uid, used_object_uid) "},
  {" ; "}
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

static const QString seabaseHistogramsDDL[] =
{
  {" create table "HBASE_HIST_NAME" "},
  {" ( "},
  {"   table_uid largeint not null not serialized, "},
  {"   histogram_id int unsigned not null not serialized, "},
  {"   col_position int not null not serialized, "},
  {"   column_number int not null not serialized, "},
  {"   colcount int not null not serialized, "},
  {"   interval_count smallint not null not serialized, "},
  {"   rowcount largeint not null not serialized, "},
  {"   total_uec largeint not null not serialized, "},
  {"   stats_time timestamp(0) not null not serialized, "},
  {"   low_value varchar(250) character set ucs2 not null not serialized, "},
  {"   high_value varchar(250) character set ucs2 not null not serialized, "},
  {"   read_time timestamp(0) not null not serialized, "},
  {"   read_count smallint not null not serialized, "},
  {"   sample_secs largeint not null not serialized, "},
  {"   col_secs largeint not null not serialized, "},
  {"   sample_percent smallint not null not serialized, "},
  {"   cv double precision not null not serialized, "},
  {"   reason char(1) not null not serialized, "},
  {"   v1 largeint not null not serialized, "},
  {"   v2 largeint not null not serialized, "},
  {"   v3 largeint not null not serialized, "},
  {"   v4 largeint not null not serialized, "},
  {"   v5 varchar(250) character set ucs2 not null not serialized, "},
  {"   v6 varchar(250) character set ucs2 not null not serialized "},
  {" , constraint "HBASE_HIST_PK" primary key "},
  {"     (table_uid, histogram_id, col_position) "},
  {" ) "},
  {" ; "}
};

static const QString seabaseHistogramIntervalsDDL[] =
{
  {" create table "HBASE_HISTINT_NAME" "},
  {" ( "},
  {"   table_uid largeint not null not serialized, "},
  {"   histogram_id int unsigned not null not serialized, "},
  {"   interval_number smallint not null not serialized, "},
  {"   interval_rowcount largeint not null not serialized, "},
  {"   interval_uec largeint not null not serialized, "},
  {"   interval_boundary varchar(250) character set ucs2 not null not serialized, "},
  {"   std_dev_of_freq numeric(12,3) not null not serialized, "},
  {"   v1 largeint not null not serialized, "},
  {"   v2 largeint not null not serialized, "},
  {"   v3 largeint not null not serialized, "},
  {"   v4 largeint not null not serialized, "},
  {"   v5 varchar(250) character set ucs2 not null not serialized, "},
  {"   v6 varchar(250) character set ucs2 not null not serialized"},
  {" , constraint "HBASE_HISTINT_PK" primary key "},
  {"    (table_uid, histogram_id, interval_number) "},
  {" ) "},
  {" ; "}
};

static const QString seabaseSeqGenDDL[] =
{
  {" create table %s.\"%s\"."SEABASE_SEQ_GEN" "},
  {" ( "},
  {"   seq_type char(2) not null not serialized, "},
  {"   seq_uid largeint not null not serialized, "},
  {"   fs_data_type integer not null not serialized, "},

  {"   start_value largeint not null not serialized, "},
  //  {"   start_value numeric(30) not null not serialized, "},

  {"   increment largeint not null not serialized, "},

  {"   max_value largeint not null not serialized, "},
  //  {"   max_value numeric(30) not null not serialized, "},

  {"   min_value largeint not null not serialized, "},
  //  {"   min_value numeric(30) not null not serialized, "},

  {"   cycle_option char(2) not null not serialized, "},
  {"   cache_size largeint not null not serialized, "},

  // next value that seq generator will return
  {"   next_value largeint not null not serialized, "},
  //  {"   curr_value numeric(30) not null not serialized, "},

  // number of seq generator calls accessing this metadata table
  {"   num_calls largeint not null not serialized "},

  {" ) "},
  {" primary key (seq_uid) salt using 8 partitions "},
  {" ; "}
};

/////////////////////////////////////////////////////////////////////
//
// Information about changed old metadata tables from which upgrade
// is being done to the current version.
// These definitions have changed in the current version of code.
// 
// Old definitions have the form (for ex for COLUMNS table):
//            seabaseOldMDv??ColumnsDDL[]
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

#define SEABASE_AUTHS_OLD_MD           SEABASE_AUTHS"_OLD_MD"
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

static const QString seabaseOldMDv21ObjectsDDL[] =
{
  {" create table "SEABASE_OBJECTS_OLD_MD" "},
  {" ( "},
  {"   catalog_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   schema_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   object_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   object_type char(2) not null not serialized, "},
  {"   object_uid largeint not null not serialized, "},
  {"   create_time largeint not null not serialized, "},
  {"   redef_time largeint not null not serialized, "},
  {"   valid_def char(2) not null not serialized "},
  {" ) "},
  {" primary key (catalog_name, schema_name, object_name, object_type) "},
  {" ; "}
};

////////////////////////////////////////////////////////////////////////
//// END_OLD_MD_v21: 
////        OLD metadata Version 2.1  (Major version = 2, Minor version = 1).
//////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// START_OLD_MD_v11: 
//        OLD metadata Version 1.1  (Major version = 1, Minor version = 1).
////////////////////////////////////////////////////////////////////////

static const QString seabaseOldMDv11ColumnsDDL[] =
{
  {" create table "SEABASE_COLUMNS_OLD_MD" "},
  {" ( "},
  {"   object_uid largeint not null not serialized, "},
  {"   column_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   column_number int not null not serialized, "},
  {"   column_class char(2) not null not serialized, "},
  {"   fs_data_type int not null not serialized, "},
  {"   column_size int not null not serialized, "},
  {"   column_precision int not null not serialized, "},
  {"   column_scale int not null not serialized, "},
  {"   datetime_start_field int not null not serialized, "},
  {"   datetime_end_field int not null not serialized, "},
  {"   is_upshifted char(2) not null not serialized, "},
  {"   column_flags int unsigned not null not serialized, "},
  {"   nullable int not null not serialized, "},
  {"   character_set char(40) not null not serialized, "},
  {"   default_class int not null not serialized, "},
  {"   default_value varchar(512) character set ucs2 not null not serialized, "},
  {"   column_heading varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   hbase_col_family varchar(40) not null not serialized, "},
  {"   hbase_col_qualifier varchar(40) not null not serialized "},
  {" ) "},
  {" primary key (object_uid, column_name) "},
  {" ; "}
};

static const QString seabaseOldMDv11IndexesDDL[] =
{
  {" create table "SEABASE_INDEXES_OLD_MD" "},
  {" ( "},
  {"   base_table_uid largeint not null not serialized, "},
  {"   catalog_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   schema_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   index_name varchar(256 bytes) character set utf8 not null not serialized, "},
  {"   keytag int not null not serialized, "},
  {"   is_unique int not null not serialized, "},
  {"   key_colcount int not null not serialized, "},
  {"   nonkey_colcount int not null not serialized "},
  {" ) "},
  {" primary key (base_table_uid, catalog_name, schema_name, index_name) "},
  {" ; "}
};

static const QString seabaseOldMDv11ObjectsDDL[] =
{
  {" create table "SEABASE_OBJECTS_OLD_MD" "},
  {" ( "},
  {"   catalog_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   schema_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   object_name varchar ( 256 bytes ) character set utf8 not null not serialized, "},
  {"   object_type char(2) not null not serialized, "},
  {"   object_uid largeint not null not serialized, "},
  {"   create_time largeint not null not serialized, "},
  {"   redef_time largeint not null not serialized, "},
  {"   valid_def char(2) not null not serialized "},
  {" ) "},
  {" primary key (catalog_name, schema_name, object_name) "},
  {" ; "}
};

static const QString seabaseOldMDv11ViewsDDL[] =
{
  {" create table "SEABASE_VIEWS_OLD_MD" "},
  {" ( "},
  {"   view_uid largeint not null not serialized, "},
  {"   check_option char(2) not null not serialized, "},
  {"   is_updatable int not null not serialized, "},
  {"   is_insertable int not null not serialized, "},
  {"   view_text varchar(6000 bytes) character set utf8 not null not serialized "},
  {" ) "},
  {" primary key (view_uid) "},
  {" ; "}
};

////////////////////////////////////////////////////////////////////////
// END_OLD_MD_v11: 
//        OLD metadata Version 1.1  (Major version = 1, Minor version = 1).
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// Metadata views.
////////////////////////////////////////////////////////////////////////
#define TRAF_COLUMNS_VIEW "COLUMNS_VIEW"
#define TRAF_INDEXES_VIEW "INDEXES_VIEW"
#define TRAF_KEYS_VIEW "KEYS_VIEW"
#define TRAF_REF_CONSTRAINTS_VIEW "REF_CONSTRAINTS_VIEW"
#define TRAF_SEQUENCES_VIEW "SEQUENCES_VIEW"
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

static const QString createTrafSequencesViewQuery[] =
{
  {" create view %s.\"%s\"."TRAF_SEQUENCES_VIEW" as "},
  {" select O.catalog_name, O.schema_name, O.object_name seq_name, "},
  {"           S.start_value, S.increment, S.max_value, S.min_value, "},
  {"           S.cycle_option, case when S.cache_size = 0 then 'N' else 'Y' end cache_option, "},
  {"           S.cache_size, S.next_value, S.num_calls "},
  {"   from %s.\"%s\".\"%s\" O, "},
  {"   %s.\"%s\".\"%s\" S "},
  {"  where O.catalog_name = '%s' "},
  {"        and O.schema_name != '%s' "},
  {"        and O.object_uid = S.seq_uid "},
  {"        and O.object_type = '%s' "},
  {"  for read uncommitted access "},
  {"  order by 1,2,3 "},
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
    TRAF_SEQUENCES_VIEW,
    createTrafSequencesViewQuery,
    sizeof(createTrafSequencesViewQuery)
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
