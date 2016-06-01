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

#ifndef _CMP_SEABASE_MD_UPGD_H_
#define _CMP_SEABASE_MD_UPGD_H_

#include "CmpSeabaseDDLmd.h"

/*******************************************************************************************
Steps needed to upgrade metadata are listed below.

In this example:
  Upgrade is being done from v23 to v30.
  old MD major version = 2, old MD minor version = 3
  new MD major version = 3, new MD minor version = 0
  Metadata table COLUMNS is being changed.

CmpSeabaseDDLmd.h
   -- move current definition of metadata table that is being changed 
     to START_OLD_MD_v23 section. 
   -- Change name to *_OLD_MD.  (Ex: SEABASE_COLUMNS_OLD_MD)
   -- Change definition name to seabaseOldMDv23* (Ex: seabaseOldMDv23ColumnsDDL)
   -- modify current definition to reflect new definition(Ex: seabaseColumnsDDL)

CmpSeabaseDDLupgrade.h
-- in struct allMDupgradeInfo,
   modify entry for  the table whose definition is being changed
       -- add old defn, insert/select col list, added/dropped col flag, etc.
  -- see struct MDUpgradeInfo for details on what fields need to be modified.

CmpSeabaseDDL.h
-- modify enum METADATA_MAJOR/MINOR, OLD_MAJOR/MINOR versions

CmpSeabaseDDLcommon.cpp and other files:
-- modify insert/select statements that access the changed metadata table.

CmpSeabaseDDLupgrade.cpp
-- In method CmpSeabaseMDupgrade::executeSeabaseMDupgrade
  -- modify CUSTOMIZE_NEW_MD case if something specified need to be done.
    Ex: update new sql_data_type field in COLUMNS table

***************************************************************************************/

// structure containing information on old and new MD tables
// and how the new MD tables will be updated.
struct MDUpgradeInfo
{
  // name of the new MD table.
  const char * newName;

  // name of the corresponding old MD table.
  const char * oldName;  // name of the old

  // ddl stmt corresponding to the current ddl.
  const QString *newDDL;
  Lng32 sizeOfnewDDL;

  // ddl stmt corresponding to the old ddl which is being upgraded.
  // If null, then old/new ddl are the same.
  const QString *oldDDL;
  Lng32 sizeOfoldDDL;

  // ddl stmt corresponding to index on this table, if one exists
  const QString *indexDDL;
  Lng32 sizeOfIndexDDL;

  const NABoolean upgradeNeeded;

  // if new and old col info is different, then data need to be copied using
  // explicit column names in insert and select part of the query.
  // insert into tgt (insertedCols) select selectedCols from src;
  const char * insertedCols;
  const char * selectedCols;

  // any where predicate to be applied to the select part of the query
  const char * wherePred;

  const NABoolean addedCols;
  const NABoolean droppedCols;

  const NABoolean addedTable;
  const NABoolean droppedTable;

  const NABoolean isIndex;

  // if true, indicates this objects exists in metadata only. There is no corresponding
  // object in hbase.
  const NABoolean mdOnly;
};

struct MDDescsInfo
{
  ComTdbVirtTableTableInfo * tableInfo;

  Lng32 numNewCols;
  ComTdbVirtTableColumnInfo * newColInfo;
  Lng32 numNewKeys;
  ComTdbVirtTableKeyInfo       * newKeyInfo;

  Lng32 numOldCols;
  ComTdbVirtTableColumnInfo * oldColInfo;
  Lng32 numOldKeys;
  ComTdbVirtTableKeyInfo       * oldKeyInfo;

  Lng32 numIndexes;
  ComTdbVirtTableIndexInfo * indexInfo;

};

//////////////////////////////////////////////////////////////
// This section should reflect the upgrade steps needed to go from
// previous to current version.
// Modify it as needed.
// Currently it is set to upgrade from V30(source major version 3, minor version 0)
// to V31.
//////////////////////////////////////////////////////////////
static const MDUpgradeInfo allMDupgradeInfo[] = {
  {SEABASE_AUTHS, SEABASE_AUTHS_OLD_MD,
   seabaseAuthsDDL, sizeof(seabaseAuthsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_COLUMNS, SEABASE_COLUMNS_OLD_MD,
   seabaseColumnsDDL, sizeof(seabaseColumnsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_DEFAULTS, SEABASE_DEFAULTS_OLD_MD,
   seabaseDefaultsDDL, sizeof(seabaseDefaultsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_INDEXES, SEABASE_INDEXES_OLD_MD,
   seabaseIndexesDDL, sizeof(seabaseIndexesDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_KEYS, SEABASE_KEYS_OLD_MD,
   seabaseKeysDDL, sizeof(seabaseKeysDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES, SEABASE_LIBRARIES_OLD_MD,
   seabaseLibrariesDDL, sizeof(seabaseLibrariesDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES_USAGE, SEABASE_LIBRARIES_USAGE_OLD_MD,
   seabaseLibrariesUsageDDL, sizeof(seabaseLibrariesUsageDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS, SEABASE_OBJECTS_OLD_MD,
   seabaseObjectsDDL, sizeof(seabaseObjectsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS_UNIQ_IDX, SEABASE_OBJECTS_UNIQ_IDX_OLD_MD,
   seabaseObjectsUniqIdxDDL, sizeof(seabaseObjectsUniqIdxDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE},

  {SEABASE_REF_CONSTRAINTS, SEABASE_REF_CONSTRAINTS_OLD_MD,
   seabaseRefConstraintsDDL, sizeof(seabaseRefConstraintsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_ROUTINES, SEABASE_ROUTINES_OLD_MD,
   seabaseRoutinesDDL, sizeof(seabaseRoutinesDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_SEQ_GEN, SEABASE_SEQ_GEN_OLD_MD,
   seabaseSeqGenDDL, sizeof(seabaseSeqGenDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLES, SEABASE_TABLES_OLD_MD,
   seabaseTablesDDL, sizeof(seabaseTablesDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLE_CONSTRAINTS, SEABASE_TABLE_CONSTRAINTS_OLD_MD,
   seabaseTableConstraintsDDL, sizeof(seabaseTableConstraintsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TEXT, SEABASE_TEXT_OLD_MD,
   seabaseTextDDL, sizeof(seabaseTextDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_UNIQUE_REF_CONSTR_USAGE, SEABASE_UNIQUE_REF_CONSTR_USAGE_OLD_MD,
   seabaseUniqueRefConstrUsageDDL, sizeof(seabaseUniqueRefConstrUsageDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VERSIONS, SEABASE_VERSIONS_OLD_MD,
   seabaseVersionsDDL, sizeof(seabaseVersionsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS, SEABASE_VIEWS_OLD_MD,
   seabaseViewsDDL, sizeof(seabaseViewsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS_USAGE, SEABASE_VIEWS_USAGE_OLD_MD,
   seabaseViewsUsageDDL, sizeof(seabaseViewsUsageDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VALIDATE_SPJ, SEABASE_VALIDATE_SPJ_OLD_MD,
   NULL, 0,
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VALIDATE_LIBRARY, SEABASE_VALIDATE_LIBRARY_OLD_MD,
   NULL, 0,
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE}

};

//////////////////////////////////////////////////////////////
// This struct is set up for V23 to V30 upgrade.
//////////////////////////////////////////////////////////////
static const MDUpgradeInfo allMDv23tov30TablesInfo[] = {
  {SEABASE_AUTHS, SEABASE_AUTHS_OLD_MD,
   seabaseAuthsDDL, sizeof(seabaseAuthsDDL),
   seabaseOldMDv23AuthsDDL, sizeof(seabaseOldMDv23AuthsDDL),
   NULL, 0,
   TRUE,
   "auth_id, auth_db_name, auth_ext_name, auth_type, auth_creator, auth_is_valid, auth_redef_time, auth_create_time, flags",
   "auth_id, auth_db_name, auth_ext_name, auth_type, auth_creator, auth_is_valid, auth_redef_time, auth_create_time, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_COLUMNS, SEABASE_COLUMNS_OLD_MD,
   seabaseColumnsDDL, sizeof(seabaseColumnsDDL),
   seabaseOldMDv23ColumnsDDL, sizeof(seabaseOldMDv23ColumnsDDL),
   NULL, 0,
   TRUE,
   "object_uid, column_name, column_number, column_class, fs_data_type, sql_data_type, column_size, column_precision, column_scale, datetime_start_field, datetime_end_field, is_upshifted, column_flags, nullable, character_set, default_class, default_value, column_heading, hbase_col_family, hbase_col_qualifier, direction, is_optional, flags",
   "object_uid, column_name, column_number, column_class, fs_data_type, '', column_size, column_precision, column_scale, datetime_start_field, datetime_end_field, is_upshifted, column_flags, nullable, character_set, default_class, default_value, column_heading, hbase_col_family, hbase_col_qualifier, direction, is_optional, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_DEFAULTS, SEABASE_DEFAULTS_OLD_MD,
   seabaseDefaultsDDL, sizeof(seabaseDefaultsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_INDEXES, SEABASE_INDEXES_OLD_MD,
   seabaseIndexesDDL, sizeof(seabaseIndexesDDL),
   seabaseOldMDv23IndexesDDL, sizeof(seabaseOldMDv23IndexesDDL),
   NULL, 0,
   TRUE,
   "base_table_uid, keytag, is_unique, key_colcount, nonkey_colcount, is_explicit, index_uid, flags",
   "base_table_uid, keytag, is_unique, key_colcount, nonkey_colcount, is_explicit, index_uid, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_KEYS, SEABASE_KEYS_OLD_MD,
   seabaseKeysDDL, sizeof(seabaseKeysDDL),
   seabaseOldMDv23KeysDDL, sizeof(seabaseOldMDv23KeysDDL),
   NULL, 0,
   TRUE,
   "object_uid, column_name, keyseq_number, column_number, ordering, nonkeycol, flags",
   "object_uid, column_name, keyseq_number, column_number, ordering, nonkeycol, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES, SEABASE_LIBRARIES_OLD_MD,
   seabaseLibrariesDDL, sizeof(seabaseLibrariesDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES_USAGE, SEABASE_LIBRARIES_USAGE_OLD_MD,
   seabaseLibrariesUsageDDL, sizeof(seabaseLibrariesUsageDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS, SEABASE_OBJECTS_OLD_MD,
   seabaseObjectsDDL, sizeof(seabaseObjectsDDL),
   seabaseOldMDv23ObjectsDDL, sizeof(seabaseOldMDv23ObjectsDDL),
   NULL, 0,
   TRUE,
   "catalog_name, schema_name, object_name, object_type, object_uid, create_time, redef_time, valid_def, droppable, object_owner, schema_owner, flags",
   "catalog_name, schema_name, case when schema_name = '_MD_' then object_name || '_OLD_MD' else object_name end, object_type, object_uid, create_time, redef_time, valid_def, 'N', object_owner, "SUPER_USER_LIT", 0 ",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS_UNIQ_IDX, SEABASE_OBJECTS_UNIQ_IDX_OLD_MD,
   seabaseObjectsUniqIdxDDL, sizeof(seabaseObjectsUniqIdxDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE},

  {SEABASE_REF_CONSTRAINTS, SEABASE_REF_CONSTRAINTS_OLD_MD,
   seabaseRefConstraintsDDL, sizeof(seabaseRefConstraintsDDL),
   seabaseOldMDv23RefConstraintsDDL, sizeof(seabaseOldMDv23RefConstraintsDDL),
   NULL, 0,
   TRUE,
   "ref_constraint_uid, unique_constraint_uid, match_option, update_rule, delete_rule, flags",
   "ref_constraint_uid, unique_constraint_uid, match_option, update_rule, delete_rule, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_ROUTINES, SEABASE_ROUTINES_OLD_MD,
   seabaseRoutinesDDL, sizeof(seabaseRoutinesDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

 {SEABASE_SEQ_GEN, SEABASE_SEQ_GEN_OLD_MD,
   seabaseSeqGenDDL, sizeof(seabaseSeqGenDDL),
   seabaseOldMDv23SeqGenDDL, sizeof(seabaseOldMDv23SeqGenDDL),
   NULL, 0,
  TRUE,
   "seq_type, seq_uid, fs_data_type, start_value, increment, max_value, min_value, cycle_option, cache_size, next_value, num_calls, redef_ts, upd_ts, flags",
   "seq_type, seq_uid, fs_data_type, start_value, increment, max_value, min_value, cycle_option, cache_size, next_value, num_calls, 0, 0, 0",
  NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLES, SEABASE_TABLES_OLD_MD,
   seabaseTablesDDL, sizeof(seabaseTablesDDL),
   seabaseOldMDv23TablesDDL, sizeof(seabaseOldMDv23TablesDDL),
   NULL, 0,
   TRUE,
   "table_uid, row_format, is_audited, row_data_length, row_total_length, key_length, num_salt_partns, flags",
   "table_uid, 'HF', is_audited, -1, -1, -1, 0, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLE_CONSTRAINTS, SEABASE_TABLE_CONSTRAINTS_OLD_MD,
   seabaseTableConstraintsDDL, sizeof(seabaseTableConstraintsDDL),
   seabaseOldMDv23TableConstraintsDDL, sizeof(seabaseOldMDv23TableConstraintsDDL),
   NULL, 0,
   TRUE,
   "table_uid, constraint_uid, constraint_type, disabled, droppable, is_deferrable, enforced, validated, last_validated, col_count, index_uid, flags",
   "table_uid, constraint_uid, constraint_type, 'N', 'N', 'N', 'Y', 'Y', juliantimestamp(current_timestamp), col_count, index_uid, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TEXT, SEABASE_TEXT_OLD_MD,
   seabaseTextDDL, sizeof(seabaseTextDDL),
   seabaseOldMDv23TextDDL, sizeof(seabaseOldMDv23TextDDL),
   NULL, 0,
   TRUE,
   "text_uid, text_type, sub_id, seq_num, flags, text",
   "object_uid, 0, 0, seq_num, 0, text",
   NULL, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_UNIQUE_REF_CONSTR_USAGE, SEABASE_UNIQUE_REF_CONSTR_USAGE_OLD_MD,
   seabaseUniqueRefConstrUsageDDL, sizeof(seabaseUniqueRefConstrUsageDDL),
   seabaseOldMDv23UniqueRefConstrUsageDDL, sizeof(seabaseOldMDv23UniqueRefConstrUsageDDL),
   NULL, 0,
   TRUE,
   "unique_constraint_uid, foreign_constraint_uid, flags",
   "unique_constraint_uid, foreign_constraint_uid, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VERSIONS, SEABASE_VERSIONS_OLD_MD,
   seabaseVersionsDDL, sizeof(seabaseVersionsDDL),
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS, SEABASE_VIEWS_OLD_MD,
   seabaseViewsDDL, sizeof(seabaseViewsDDL),
   seabaseOldMDv23ViewsDDL, sizeof(seabaseOldMDv23ViewsDDL),
   NULL, 0,
   TRUE,
   "view_uid, check_option, is_updatable, is_insertable, flags",
   "view_uid, check_option, is_updatable, is_insertable, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS_USAGE, SEABASE_VIEWS_USAGE_OLD_MD,
   seabaseViewsUsageDDL, sizeof(seabaseViewsUsageDDL),
   seabaseOldMDv23ViewsUsageDDL, sizeof(seabaseOldMDv23ViewsUsageDDL),
   NULL, 0,
   TRUE,
   "using_view_uid, used_object_uid, used_object_type, flags",
   "using_view_uid, used_object_uid, used_object_type, 0",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VALIDATE_SPJ, SEABASE_VALIDATE_SPJ_OLD_MD,
   NULL, 0,
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VALIDATE_LIBRARY, SEABASE_VALIDATE_LIBRARY_OLD_MD,
   NULL, 0,
   NULL, 0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE}

};


class CmpSeabaseMDupgrade : public CmpSeabaseDDL
{
 public:

  enum Steps {
    UPGRADE_START              = 0,
    VERSION_CHECK,
    OLD_MD_DROP_PRE,
    CURR_MD_BACKUP,
    CURR_MD_DROP,
    INITIALIZE_TRAF,
    COPY_MD_TABLES_PROLOGUE,
    COPY_MD_TABLES,
    COPY_MD_TABLES_EPILOGUE,
    CUSTOMIZE_NEW_MD,
    VALIDATE_DATA_COPY,
    OLD_TABLES_MD_DELETE,
    OLD_MD_TABLES_HBASE_DELETE,
    UPDATE_MD_VIEWS,
    UPGRADE_PRIV_MGR,
    UPGRADE_REPOS,
    UPDATE_VERSION,
    OLD_MD_DROP_POST,
    METADATA_UPGRADED,
    UPGRADE_DONE,
    UPGRADE_FAILED,
    UPGRADE_FAILED_RESTORE_OLD_MD,
    UPGRADE_FAILED_DROP_OLD_MD,
    GET_MD_VERSION,
    GET_SW_VERSION,
    DONE_RETURN
  } ;

  CmpSeabaseMDupgrade(NAHeap *heap)
    : CmpSeabaseDDL(heap)
    {};

  NABoolean isOldMDtable(const NAString &objName);

  NABoolean isMDUpgradeNeeded();
  NABoolean isViewsUpgradeNeeded();
  NABoolean isReposUpgradeNeeded();
  NABoolean isPrivsUpgradeNeeded();
  NABoolean isUpgradeNeeded() 
  { return (isMDUpgradeNeeded() || 
            isViewsUpgradeNeeded() ||
            isReposUpgradeNeeded() ||
            isPrivsUpgradeNeeded());
  }

  short dropMDtables(ExpHbaseInterface *ehi, NABoolean oldTbls,
		     NABoolean useOldNameForNewTables = FALSE);

  short dropReposTables(ExpHbaseInterface *ehi, NABoolean oldTbls);

  short restoreOldMDtables(ExpHbaseInterface *ehi);

  short upgradePrivMgr(ExeCliInterface *cliInterface, 
                       NABoolean ddlXns,
                       NAString &privMgrDoneMsg);

  short customizeNewMD(CmpDDLwithStatusInfo *mdui, ExeCliInterface &cliInterface);
  short customizeNewMDv23tov30(CmpDDLwithStatusInfo *mdui,
                               ExeCliInterface &cliInterface);

  short executeSeabaseMDupgrade(CmpDDLwithStatusInfo *mdui,
                                NABoolean ddlXns,
				NAString &currCatName, NAString &currSchName);

};

#endif

