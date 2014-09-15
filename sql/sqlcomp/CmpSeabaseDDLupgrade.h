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

#ifndef _CMP_SEABASE_MD_UPGD_H_
#define _CMP_SEABASE_MD_UPGD_H_

#include "CmpSeabaseDDLmd.h"

// structure containing information on old and new MD tables
// and how the new MD tables will be updated.
struct MDTableInfo
{
  // name of the new MD table.
  // if NULL, then this table was dropped.
  const char * newName;

  // name of the corresponding old MD table.
  // if NULL, then the new MD table is an added table.
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
};

struct MDDescsInfo
{
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
// source to current version.
// Modify it as needed.
// Currently it is set to upgrade from V22(source major version 2, minor version 2)
// to V23.
//////////////////////////////////////////////////////////////
static const MDTableInfo allMDtablesInfo[] = {
  {SEABASE_AUTHS, SEABASE_AUTHS_OLD_MD,
   seabaseAuthsDDL, sizeof(seabaseAuthsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_COLUMNS, SEABASE_COLUMNS_OLD_MD,
   seabaseColumnsDDL, sizeof(seabaseColumnsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_DEFAULTS, SEABASE_DEFAULTS_OLD_MD,
   seabaseDefaultsDDL, sizeof(seabaseDefaultsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_INDEXES, SEABASE_INDEXES_OLD_MD,
   seabaseIndexesDDL, sizeof(seabaseIndexesDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_KEYS, SEABASE_KEYS_OLD_MD,
   seabaseKeysDDL, sizeof(seabaseKeysDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES, SEABASE_LIBRARIES_OLD_MD,
   seabaseLibrariesDDL, sizeof(seabaseLibrariesDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES_USAGE, SEABASE_LIBRARIES_USAGE_OLD_MD,
   seabaseLibrariesUsageDDL, sizeof(seabaseLibrariesUsageDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS, SEABASE_OBJECTS_OLD_MD,
   seabaseObjectsDDL, sizeof(seabaseObjectsDDL),
   NULL, 0,
   seabaseObjectsUniqIdxIndexDDL, sizeof(seabaseObjectsUniqIdxIndexDDL),

   NULL,
  "catalog_name, schema_name, case when schema_name = '_MD_' then object_name || '_OLD_MD' else object_name end, object_type, object_uid, create_time, redef_time, valid_def, object_owner",
   NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS_UNIQ_IDX, SEABASE_OBJECTS_UNIQ_IDX_OLD_MD,
   seabaseObjectsUniqIdxDDL, sizeof(seabaseObjectsUniqIdxDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, TRUE},

  {SEABASE_REF_CONSTRAINTS, SEABASE_REF_CONSTRAINTS_OLD_MD,
   seabaseRefConstraintsDDL, sizeof(seabaseRefConstraintsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_ROUTINES, SEABASE_ROUTINES_OLD_MD,
   seabaseRoutinesDDL, sizeof(seabaseRoutinesDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLES, SEABASE_TABLES_OLD_MD,
   seabaseTablesDDL, sizeof(seabaseTablesDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLE_CONSTRAINTS, SEABASE_TABLE_CONSTRAINTS_OLD_MD,
   seabaseTableConstraintsDDL, sizeof(seabaseTableConstraintsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TEXT, SEABASE_TEXT_OLD_MD,
   seabaseTextDDL, sizeof(seabaseTextDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_UNIQUE_REF_CONSTR_USAGE, SEABASE_UNIQUE_REF_CONSTR_USAGE_OLD_MD,
   seabaseUniqueRefConstrUsageDDL, sizeof(seabaseUniqueRefConstrUsageDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VERSIONS, SEABASE_VERSIONS_OLD_MD,
   seabaseVersionsDDL, sizeof(seabaseVersionsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS, SEABASE_VIEWS_OLD_MD,
   seabaseViewsDDL, sizeof(seabaseViewsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS_USAGE, SEABASE_VIEWS_USAGE_OLD_MD,
   seabaseViewsUsageDDL, sizeof(seabaseViewsUsageDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

};


//////////////////////////////////////////////////////////////
// This struct is set up for V11 to V21 upgrade.
// keep_this_around_for_examples_of_upgrade_struct_change
//////////////////////////////////////////////////////////////
static const MDTableInfo allMDv11tov21TablesInfo[] = {

  // added columns
  {SEABASE_COLUMNS, SEABASE_COLUMNS_OLD_MD,
   seabaseColumnsDDL, sizeof(seabaseColumnsDDL),
   seabaseOldMDv11ColumnsDDL, sizeof(seabaseOldMDv11ColumnsDDL),
   NULL, 0,
   NULL,
   "object_uid, column_name, column_number, column_class, fs_data_type, column_size, column_precision, column_scale, datetime_start_field, datetime_end_field, is_upshifted, column_flags, nullable, character_set, default_class, default_value, column_heading, hbase_col_family, hbase_col_qualifier, ' ', 'N'",
   NULL,
   TRUE /* added cols*/, FALSE, FALSE, FALSE, FALSE},

  // no change
  {SEABASE_DEFAULTS, SEABASE_DEFAULTS_OLD_MD,
   seabaseDefaultsDDL, sizeof(seabaseDefaultsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  // added columns
  {SEABASE_INDEXES, SEABASE_INDEXES_OLD_MD,
   seabaseIndexesDDL, sizeof(seabaseIndexesDDL),
   seabaseOldMDv11IndexesDDL, sizeof(seabaseOldMDv11IndexesDDL),
   NULL, 0,
   "BASE_TABLE_UID, KEYTAG, IS_UNIQUE, KEY_COLCOUNT, NONKEY_COLCOUNT, IS_EXPLICIT, INDEX_UID",
   "BASE_TABLE_UID, KEYTAG, IS_UNIQUE, KEY_COLCOUNT, NONKEY_COLCOUNT, 1, (select object_uid from "TRAFODION_SYSCAT_LIT".""\""SEABASE_MD_SCHEMA"\"""."SEABASE_OBJECTS_OLD_MD" O where o.catalog_name = src.catalog_name and o.schema_name = src.schema_name and o.object_name = src.index_name and o.object_type = 'IX')",
   NULL, 
   TRUE, FALSE, FALSE, FALSE, FALSE},

  // no change
  {SEABASE_KEYS, SEABASE_KEYS_OLD_MD,
   seabaseKeysDDL, sizeof(seabaseKeysDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  // new table added
  {SEABASE_LIBRARIES, NULL,
   seabaseLibrariesDDL, sizeof(seabaseLibrariesDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  // new table added
  {SEABASE_LIBRARIES_USAGE, NULL,
   seabaseLibrariesUsageDDL, sizeof(seabaseLibrariesUsageDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  // primary key changed. Unique index added. Reflected in the new struct
  {SEABASE_OBJECTS, SEABASE_OBJECTS_OLD_MD,
   seabaseObjectsDDL, sizeof(seabaseObjectsDDL),
   NULL, 0,
   seabaseObjectsUniqIdxIndexDDL, sizeof(seabaseObjectsUniqIdxIndexDDL),
   NULL,
   "catalog_name, schema_name, case when schema_name = '_MD_' then object_name || '_OLD_MD' else object_name end, object_type, object_uid, create_time, redef_time, valid_def",
   NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  // new index added
  {SEABASE_OBJECTS_UNIQ_IDX, NULL,
   seabaseObjectsUniqIdxDDL, sizeof(seabaseObjectsUniqIdxDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, TRUE},

  // this table was removed in v21
  {SEABASE_OBJECTUID, SEABASE_OBJECTUID_OLD_MD,
   NULL, 0,
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, TRUE, FALSE},

  // new table added
  {SEABASE_REF_CONSTRAINTS, NULL,
   seabaseRefConstraintsDDL, sizeof(seabaseRefConstraintsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  // new table added
  {SEABASE_ROUTINES, NULL,
   seabaseRoutinesDDL, sizeof(seabaseRoutinesDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  // no change
  {SEABASE_TABLES, SEABASE_TABLES_OLD_MD,
   seabaseTablesDDL, sizeof(seabaseTablesDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  // new table added
  {SEABASE_TABLE_CONSTRAINTS, NULL,
   seabaseTablesDDL, sizeof(seabaseTablesDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  // new table added
  {SEABASE_TEXT, NULL,
   seabaseTextDDL, sizeof(seabaseTextDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  // new table added
  {SEABASE_UNIQUE_REF_CONSTR_USAGE, NULL,
   seabaseUniqueRefConstrUsageDDL, sizeof(seabaseUniqueRefConstrUsageDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  // no change
  {SEABASE_VERSIONS, SEABASE_VERSIONS_OLD_MD,
   seabaseVersionsDDL, sizeof(seabaseVersionsDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  // columns dropped
  {SEABASE_VIEWS, SEABASE_VIEWS_OLD_MD,
   seabaseViewsDDL, sizeof(seabaseViewsDDL),
   seabaseOldMDv11ViewsDDL, sizeof(seabaseOldMDv11ViewsDDL),
   NULL, 0,
   "VIEW_UID, CHECK_OPTION, IS_UPDATABLE, IS_INSERTABLE",
   "VIEW_UID, CHECK_OPTION, IS_UPDATABLE, IS_INSERTABLE",
   NULL,
   FALSE, TRUE, FALSE, FALSE, FALSE},

  // no change
  {SEABASE_VIEWS_USAGE, SEABASE_VIEWS_USAGE_OLD_MD,
   seabaseViewsUsageDDL, sizeof(seabaseViewsUsageDDL),
   NULL, 0,
   NULL, 0,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

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

  CmpSeabaseMDupgrade()
    : CmpSeabaseDDL(NULL)
    {};

  NABoolean isOldMDtable(const NAString &objName);

  short dropMDtables(ExpHbaseInterface *ehi, NABoolean oldTbls,
		     NABoolean useOldNameForNewTables = FALSE);

  short restoreOldMDtables(ExpHbaseInterface *ehi);

  short executeSeabaseMDupgrade(CmpMDupgradeInfo &mdi,
				NAString &currCatName, NAString &currSchName);

};

#endif

