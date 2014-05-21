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

  // column information of the new table
  Lng32 newColInfoSize;
  const ComTdbVirtTableColumnInfo * newColInfo;

  // column information of the old table.
  // if NULL, then old and new structure are the same. In this case, data could
  // be copied as is using an "insert into tgt select * from src;" stmt.
  Lng32 oldColInfoSize;
  const ComTdbVirtTableColumnInfo * oldColInfo;

  // key information of the new table
  Lng32 newKeyInfoSize;
  const ComTdbVirtTableKeyInfo * newKeyInfo;

  // key information of the old table
  Lng32 oldKeyInfoSize;
  const ComTdbVirtTableKeyInfo * oldKeyInfo;

  // index information of the new table
  Lng32 newIndexInfoSize;
  const ComTdbVirtTableIndexInfo * newIndexInfo;

  // key information of the new index
  Lng32 newIndexKeyInfoSize;
  const ComTdbVirtTableKeyInfo * newIndexKeyInfo;

  // non key col information of the new index
  Lng32 newIndexNonKeyInfoSize;
  const ComTdbVirtTableKeyInfo * newIndexNonKeyInfo;

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

//////////////////////////////////////////////////////////////
// This section should reflect the upgrade steps needed to go from
// source to current version.
// Modify it as needed.
// Currently it is set to upgrade from V21(source major version 2, minor version 1)
// to V22.
//////////////////////////////////////////////////////////////
static const MDTableInfo allMDtablesInfo[] = {
  {SEABASE_AUTHS, NULL,
   sizeof(seabaseMDAuthsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDAuthsColInfo,
   0, NULL,
   sizeof(seabaseMDAuthsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDAuthsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_COLUMNS, SEABASE_COLUMNS_OLD_MD,
   sizeof(seabaseMDColumnsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDColumnsColInfo,
   0, NULL,
   sizeof(seabaseMDColumnsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDColumnsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   NULL,
   NULL,
   NULL, 
   FALSE, FALSE, FALSE,
   FALSE, FALSE},

  {SEABASE_DEFAULTS, SEABASE_DEFAULTS_OLD_MD,
   sizeof(seabaseMDDefaultsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDDefaultsColInfo,
   0, NULL,
   sizeof(seabaseMDDefaultsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDDefaultsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_INDEXES, SEABASE_INDEXES_OLD_MD,
   sizeof(seabaseMDIndexesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDIndexesColInfo,
   0, NULL,
   sizeof(seabaseMDIndexesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDIndexesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   NULL,
   NULL, 
   NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS, SEABASE_OBJECTS_OLD_MD,
   sizeof(seabaseMDObjectsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDObjectsColInfo,
   sizeof(seabaseOldMDv21ObjectsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseOldMDv21ObjectsColInfo,
   sizeof(seabaseMDObjectsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsKeyInfo,
   0, NULL,
   // new index info
   0, NULL,

   // new index key info
   0, NULL, 
   // new index nonkey info
   0, NULL,
   NULL,
  "catalog_name, schema_name, case when schema_name = '_MD_' then object_name || '_OLD_MD' else object_name end, object_type, object_uid, create_time, redef_time, valid_def, "SUPER_USER_LIT" ",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS_UNIQ_IDX, SEABASE_OBJECTS_UNIQ_IDX_OLD_MD,
   sizeof(seabaseMDObjectsUniqIdxColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDObjectsUniqIdxColInfo,
   0, NULL,
   sizeof(seabaseMDObjectsUniqIdxOnlyKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsUniqIdxOnlyKeyInfo,
   0, NULL,
   0, NULL,
   0, NULL,
   0, NULL,
   NULL,
   NULL,
   NULL, FALSE, FALSE, FALSE, FALSE, TRUE},

  {SEABASE_KEYS, SEABASE_KEYS_OLD_MD,
   sizeof(seabaseMDKeysColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDKeysColInfo,
   0, NULL,
   sizeof(seabaseMDKeysKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDKeysKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES, SEABASE_LIBRARIES_OLD_MD,
   sizeof(seabaseMDLibrariesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDLibrariesColInfo,
   0, NULL,
   sizeof(seabaseMDLibrariesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDLibrariesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES_USAGE, SEABASE_LIBRARIES_USAGE_OLD_MD,
   sizeof(seabaseMDLibrariesUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDLibrariesUsageColInfo,
   0, NULL,
   sizeof(seabaseMDLibrariesUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDLibrariesUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_REF_CONSTRAINTS, SEABASE_REF_CONSTRAINTS_OLD_MD,
   sizeof(seabaseMDRefConstraintsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDRefConstraintsColInfo,
   0, NULL,
   sizeof(seabaseMDRefConstraintsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDRefConstraintsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_ROUTINES, SEABASE_ROUTINES_OLD_MD,
   sizeof(seabaseMDRoutinesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDRoutinesColInfo,
   0, NULL,
   sizeof(seabaseMDRoutinesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDRoutinesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLES, SEABASE_TABLES_OLD_MD,
   sizeof(seabaseMDTablesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTablesColInfo,
   0, NULL,
   sizeof(seabaseMDTablesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTablesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLE_CONSTRAINTS, SEABASE_TABLE_CONSTRAINTS_OLD_MD,
   sizeof(seabaseMDTableConstraintsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTableConstraintsColInfo,
   0, NULL,
   sizeof(seabaseMDTableConstraintsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTableConstraintsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TEXT, SEABASE_TEXT_OLD_MD,
   sizeof(seabaseMDTextColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTextColInfo,
   0, NULL,
   sizeof(seabaseMDTextKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTextKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_UNIQUE_REF_CONSTR_USAGE, SEABASE_UNIQUE_REF_CONSTR_USAGE_OLD_MD,
   sizeof(seabaseMDUniqueRefConstrUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDUniqueRefConstrUsageColInfo,
   0, NULL,
   sizeof(seabaseMDUniqueRefConstrUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDUniqueRefConstrUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS, SEABASE_VIEWS_OLD_MD,
   sizeof(seabaseMDViewsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDViewsColInfo,
   0, NULL,
   sizeof(seabaseMDViewsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDViewsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL,
   NULL,
   NULL,
   FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS_USAGE, SEABASE_VIEWS_USAGE_OLD_MD,
   sizeof(seabaseMDViewsUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDViewsUsageColInfo,
   0, NULL,
   sizeof(seabaseMDViewsUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDViewsUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VERSIONS, SEABASE_VERSIONS_OLD_MD,
   sizeof(seabaseMDVersionsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDVersionsColInfo,
   0, NULL,
   sizeof(seabaseMDVersionsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDVersionsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE}
};

//////////////////////////////////////////////////////////////
// This struct is set up for V21 to V22 upgrade.
//////////////////////////////////////////////////////////////
static const MDTableInfo allMDv21tov22TablesInfo[] = {
  {SEABASE_AUTHS, NULL,
   sizeof(seabaseMDAuthsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDAuthsColInfo,
   0, NULL,
   sizeof(seabaseMDAuthsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDAuthsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_COLUMNS, SEABASE_COLUMNS_OLD_MD,
   sizeof(seabaseMDColumnsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDColumnsColInfo,
   0, NULL,
   sizeof(seabaseMDColumnsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDColumnsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_DEFAULTS, SEABASE_DEFAULTS_OLD_MD,
   sizeof(seabaseMDDefaultsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDDefaultsColInfo,
   0, NULL,
   sizeof(seabaseMDDefaultsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDDefaultsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_INDEXES, SEABASE_INDEXES_OLD_MD,
   sizeof(seabaseMDIndexesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDIndexesColInfo,
   0, NULL,
   sizeof(seabaseMDIndexesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDIndexesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS, SEABASE_OBJECTS_OLD_MD,
   sizeof(seabaseMDObjectsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDObjectsColInfo,
   sizeof(seabaseOldMDv21ObjectsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseOldMDv21ObjectsColInfo,
   sizeof(seabaseMDObjectsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsKeyInfo,
   0, NULL,
   // new index info
   0, NULL,

   // new index key info
   0, NULL, 
   // new index nonkey info
   0, NULL,
   NULL,
    "catalog_name, schema_name, case when schema_name = '_MD_' then object_name || '_OLD_MD' else object_name end, object_type, object_uid, create_time, redef_time, valid_def, "SUPER_USER_LIT" ",
    NULL, TRUE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS_UNIQ_IDX, NULL,
   sizeof(seabaseMDObjectsUniqIdxColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDObjectsUniqIdxColInfo,
   0, NULL,
   sizeof(seabaseMDObjectsUniqIdxOnlyKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsUniqIdxOnlyKeyInfo,
   0, NULL,
   0, NULL,
   0, NULL,
   0, NULL,
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_KEYS, SEABASE_KEYS_OLD_MD,
   sizeof(seabaseMDKeysColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDKeysColInfo,
   0, NULL,
   sizeof(seabaseMDKeysKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDKeysKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES, SEABASE_LIBRARIES_OLD_MD,
   sizeof(seabaseMDLibrariesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDLibrariesColInfo,
   0, NULL,
   sizeof(seabaseMDLibrariesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDLibrariesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES_USAGE, SEABASE_LIBRARIES_USAGE_OLD_MD,
   sizeof(seabaseMDLibrariesUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDLibrariesUsageColInfo,
   0, NULL,
   sizeof(seabaseMDLibrariesUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDLibrariesUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_REF_CONSTRAINTS, SEABASE_REF_CONSTRAINTS_OLD_MD,
   sizeof(seabaseMDRefConstraintsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDRefConstraintsColInfo,
   0, NULL,
   sizeof(seabaseMDRefConstraintsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDRefConstraintsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_ROUTINES, SEABASE_ROUTINES_OLD_MD,
   sizeof(seabaseMDRoutinesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDRoutinesColInfo,
   0, NULL,
   sizeof(seabaseMDRoutinesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDRoutinesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLES, SEABASE_TABLES_OLD_MD,
   sizeof(seabaseMDTablesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTablesColInfo,
   0, NULL,
   sizeof(seabaseMDTablesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTablesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLE_CONSTRAINTS, SEABASE_TABLE_CONSTRAINTS_OLD_MD,
   sizeof(seabaseMDTableConstraintsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTableConstraintsColInfo,
   0, NULL,
   sizeof(seabaseMDTableConstraintsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTableConstraintsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TEXT, SEABASE_TEXT_OLD_MD,
   sizeof(seabaseMDTextColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTextColInfo,
   0, NULL,
   sizeof(seabaseMDTextKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTextKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_UNIQUE_REF_CONSTR_USAGE, SEABASE_UNIQUE_REF_CONSTR_USAGE_OLD_MD,
   sizeof(seabaseMDUniqueRefConstrUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDUniqueRefConstrUsageColInfo,
   0, NULL,
   sizeof(seabaseMDUniqueRefConstrUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDUniqueRefConstrUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS, SEABASE_VIEWS_OLD_MD,
   sizeof(seabaseMDViewsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDViewsColInfo,
   0, NULL,
   sizeof(seabaseMDViewsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDViewsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL,
   NULL,
   NULL, 
   FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS_USAGE, SEABASE_VIEWS_USAGE_OLD_MD,
   sizeof(seabaseMDViewsUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDViewsUsageColInfo,
   0, NULL,
   sizeof(seabaseMDViewsUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDViewsUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VERSIONS, SEABASE_VERSIONS_OLD_MD,
   sizeof(seabaseMDVersionsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDVersionsColInfo,
   0, NULL,
   sizeof(seabaseMDVersionsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDVersionsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE}
};

//////////////////////////////////////////////////////////////
// This struct is set up for V11 to V21 upgrade.
//////////////////////////////////////////////////////////////
static const MDTableInfo allMDv11tov21TablesInfo[] = {
  {SEABASE_COLUMNS, SEABASE_COLUMNS_OLD_MD,
   sizeof(seabaseMDColumnsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDColumnsColInfo,
   sizeof(seabaseOldMDv11ColumnsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseOldMDv11ColumnsColInfo,
   sizeof(seabaseMDColumnsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDColumnsKeyInfo,
   sizeof(seabaseMDColumnsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDColumnsKeyInfo,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   NULL,
   "object_uid, column_name, column_number, column_class, fs_data_type, column_size, column_precision, column_scale, datetime_start_field, datetime_end_field, is_upshifted, column_flags, nullable, character_set, default_class, default_value, column_heading, hbase_col_family, hbase_col_qualifier, ' ', 'N'",
   NULL,
   TRUE /* added cols*/, FALSE,
   FALSE, FALSE,
   FALSE},

  {SEABASE_DEFAULTS, SEABASE_DEFAULTS_OLD_MD,
   sizeof(seabaseMDDefaultsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDDefaultsColInfo,
   0, NULL,
   sizeof(seabaseMDDefaultsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDDefaultsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_INDEXES, SEABASE_INDEXES_OLD_MD,
   sizeof(seabaseMDIndexesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDIndexesColInfo,
   sizeof(seabaseOldMDv11IndexesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseOldMDv11IndexesColInfo,
   sizeof(seabaseMDIndexesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDIndexesKeyInfo,
   sizeof(seabaseOldMDv11IndexesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseOldMDv11IndexesKeyInfo,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index non key info
   "BASE_TABLE_UID, KEYTAG, IS_UNIQUE, KEY_COLCOUNT, NONKEY_COLCOUNT, IS_EXPLICIT, INDEX_UID",
   "BASE_TABLE_UID, KEYTAG, IS_UNIQUE, KEY_COLCOUNT, NONKEY_COLCOUNT, 1, (select object_uid from "TRAFODION_SYSCAT_LIT".""\""SEABASE_MD_SCHEMA"\"""."SEABASE_OBJECTS_OLD_MD" O where o.catalog_name = src.catalog_name and o.schema_name = src.schema_name and o.object_name = src.index_name and o.object_type = 'IX')",
   NULL, TRUE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS, SEABASE_OBJECTS_OLD_MD,
   sizeof(seabaseMDObjectsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDObjectsColInfo,
   0, NULL,
   sizeof(seabaseMDObjectsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsKeyInfo,
   sizeof(seabaseOldMDv11ObjectsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseOldMDv11ObjectsKeyInfo,
   // new index info
   sizeof(seabaseMDObjectsUniqIdxIndexInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsUniqIdxIndexInfo,

   // new index key info
   sizeof(seabaseMDObjectsUniqIdxKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsUniqIdxKeyInfo,
   // new index nonkey info
   sizeof(seabaseMDObjectsUniqIdxNonKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsUniqIdxNonKeyInfo,
   NULL,
   "catalog_name, schema_name, case when schema_name = '_MD_' then object_name || '_OLD_MD' else object_name end, object_type, object_uid, create_time, redef_time, valid_def",
   NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_OBJECTS_UNIQ_IDX, NULL,
   sizeof(seabaseMDObjectsUniqIdxColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDObjectsUniqIdxColInfo,
   0, NULL,
   sizeof(seabaseMDObjectsUniqIdxOnlyKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDObjectsUniqIdxOnlyKeyInfo,
   0, NULL,
   0, NULL,
   0, NULL,
   0, NULL,
   NULL,
   NULL,
   NULL, FALSE, FALSE, TRUE, FALSE, TRUE},

  {SEABASE_OBJECTUID, SEABASE_OBJECTUID_OLD_MD,
   0, NULL,
   0, NULL,
   0, NULL,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL,
   NULL,
   NULL, FALSE, FALSE, FALSE, TRUE, FALSE},

  {SEABASE_KEYS, SEABASE_KEYS_OLD_MD,
   sizeof(seabaseMDKeysColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDKeysColInfo,
   0, NULL,
   sizeof(seabaseMDKeysKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDKeysKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_LIBRARIES, NULL,
   sizeof(seabaseMDLibrariesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDLibrariesColInfo,
   0, NULL,
   sizeof(seabaseMDLibrariesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDLibrariesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_LIBRARIES_USAGE, NULL,
   sizeof(seabaseMDLibrariesUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDLibrariesUsageColInfo,
   0, NULL,
   sizeof(seabaseMDLibrariesUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDLibrariesUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_REF_CONSTRAINTS, NULL,
   sizeof(seabaseMDRefConstraintsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDRefConstraintsColInfo,
   0, NULL,
   sizeof(seabaseMDRefConstraintsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDRefConstraintsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_ROUTINES, NULL,
   sizeof(seabaseMDRoutinesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDRoutinesColInfo,
   0, NULL,
   sizeof(seabaseMDRoutinesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDRoutinesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_TABLES, SEABASE_TABLES_OLD_MD,
   sizeof(seabaseMDTablesColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTablesColInfo,
   0, NULL,
   sizeof(seabaseMDTablesKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTablesKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_TABLE_CONSTRAINTS, NULL,
   sizeof(seabaseMDTableConstraintsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTableConstraintsColInfo,
   0, NULL,
   sizeof(seabaseMDTableConstraintsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTableConstraintsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_TEXT, NULL,
   sizeof(seabaseMDTextColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDTextColInfo,
   0, NULL,
   sizeof(seabaseMDTextKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDTextKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_UNIQUE_REF_CONSTR_USAGE, NULL,
   sizeof(seabaseMDUniqueRefConstrUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDUniqueRefConstrUsageColInfo,
   0, NULL,
   sizeof(seabaseMDUniqueRefConstrUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDUniqueRefConstrUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, TRUE, FALSE, FALSE},

  {SEABASE_VIEWS, SEABASE_VIEWS_OLD_MD,
   sizeof(seabaseMDViewsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDViewsColInfo,
   sizeof(seabaseOldMDv11ViewsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseOldMDv11ViewsColInfo,
   sizeof(seabaseMDViewsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDViewsKeyInfo,
   sizeof(seabaseMDViewsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDViewsKeyInfo,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   "VIEW_UID, CHECK_OPTION, IS_UPDATABLE, IS_INSERTABLE",
   "VIEW_UID, CHECK_OPTION, IS_UPDATABLE, IS_INSERTABLE",
   NULL,
   FALSE, TRUE, FALSE, FALSE, FALSE},

  {SEABASE_VIEWS_USAGE, SEABASE_VIEWS_USAGE_OLD_MD,
   sizeof(seabaseMDViewsUsageColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDViewsUsageColInfo,
   0, NULL,
   sizeof(seabaseMDViewsUsageKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDViewsUsageKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE},

  {SEABASE_VERSIONS, SEABASE_VERSIONS_OLD_MD,
   sizeof(seabaseMDVersionsColInfo)/sizeof(ComTdbVirtTableColumnInfo), seabaseMDVersionsColInfo,
   0, NULL,
   sizeof(seabaseMDVersionsKeyInfo)/sizeof(ComTdbVirtTableKeyInfo), seabaseMDVersionsKeyInfo,
   0, NULL,
   0, NULL, // new index info
   0, NULL, // new index key info
   0, NULL, // new index nonkey col info
   NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE}
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

  static NABoolean getMDtableInfo(const NAString &objName,
				  Lng32 &colInfoSize,
				  const ComTdbVirtTableColumnInfo* &colInfo,
				  Lng32 &keyInfoSize,
				  const ComTdbVirtTableKeyInfo* &keyInfo,
				  Lng32 &indexInfoSize,
				  const ComTdbVirtTableIndexInfo* &indexInfo,
				  Lng32 &indexKeyInfoSize,
				  const ComTdbVirtTableKeyInfo * &indexKeyInfo,
				  const char * objType);
};

#endif

