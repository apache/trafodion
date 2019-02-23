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

#include "BaseTypes.h"

#include <string.h>             // strstr
#include "str.h"                // str_itoa
#include "NAString.h"           // TrimNAStringSpace
#include "nawstring.h"


#include "ComASSERT.h"
#include "OperTypeEnum.h"
#include "CmpSeabaseDDLutil.h"
#include "NLSConversion.h"
#include "CmpCommon.h"

//
// Versioning Light: Changed translation functions to generally use the
// literalToEnum and enumToLiteral general machinery (common/ComDistribution.cpp).
// Translation arrays should be specified in expected order of likelyhood.
// See common/ComSmallDefs for actual enum and literal values
//
// Since all functions will look the same, here is a handy #define to define one pair of them.
// Define a translation array (see below for examples, then invoke the define as follows:
//    defXLateFuncs(<literal-to-enum-func>, <enum-to-literal-func>, <enum-type>, <translation array>);

// define the enum-to-literal function
#define defXLateE2L(E2L,eType,array) void E2L (const eType e, char * l) \
{ NABoolean found; \
  enumToLiteral (array, occurs(array), e, l, found); \
  ComASSERT(found); }

// define the literal-to-enum function
#define defXLateL2E(L2E,eType,array) eType L2E(const char * l) \
{ NABoolean found; \
  eType result = (eType) literalToEnum (array, occurs(array), l, found); \
  ComASSERT(found); \
  return result; }

// Define both
#define defXLateFuncs(L2E,E2L,eType,array) defXLateL2E(L2E,eType,array);defXLateE2L(E2L,eType,array)

//
//----------------------------------------------------------------------------
// ComObjectClass translation
//
const literalAndEnumStruct ObjectClassXlateArray [] =
{
  {COM_CLASS_USER_TABLE, COM_CLASS_USER_TABLE_LIT},
  {COM_CLASS_SYSTEM_METADATA, COM_CLASS_SYSTEM_METADATA_LIT},
  {COM_CLASS_USER_METADATA, COM_CLASS_USER_METADATA_LIT},
  {COM_CLASS_MV_UMD, COM_CLASS_MV_UMD_LIT},
  {COM_CLASS_UNKNOWN, COM_CLASS_UNKNOWN_LIT},
  {COM_CLASS_SYSTEM_TABLE, COM_CLASS_SYSTEM_TABLE_LIT}
};

defXLateFuncs(CmGetComObjectClassAsObjectClass, CmGetComObjectClassAsLit, ComObjectClass, ObjectClassXlateArray);

const literalAndEnumStruct ComponentPrivilegeClassXlateArray [] =
{
  {COM_INTERNAL_COMPONENT_PRIVILEGE, COM_INTERNAL_COMPONENT_PRIVILEGE_LIT},
  {COM_EXTERNAL_COMPONENT_PRIVILEGE, COM_EXTERNAL_COMPONENT_PRIVILEGE_LIT}
};

defXLateFuncs(CmGetComComponentPrivilegeClassAsComponentPrivilegeClass, CmGetComComponentPrivilegeClassAsLit, ComComponentPrivilegeClass, ComponentPrivilegeClassXlateArray);

//
//----------------------------------------------------------------------------
// ComClusteringScheme translation
//
const literalAndEnumStruct ClusteringSchemeXlateArray [] =
{
  {COM_KEY_SEQ_CLUSTERING, COM_KEY_SEQ_CLUSTERING_LIT},
  {COM_ENTRY_SEQ_CLUSTERING, COM_ENTRY_SEQ_CLUSTERING_LIT},
  {COM_UNKNOWN_CLUSTERING, COM_UNKNOWN_CLUSTERING_LIT},
};

defXLateFuncs(CmGetComClusterAsClusterScheme, CmGetComClusterAsLit, ComClusteringScheme, ClusteringSchemeXlateArray);


//
//----------------------------------------------------------------------------
// ComPartitioningScheme translation
//
const literalAndEnumStruct PartitioningSchemeXlateArray [] =
{
  {COM_HASH_V2_PARTITIONING, COM_HASH_V2_PARTITIONING_LIT},
  {COM_NO_PARTITIONING, COM_NO_PARTITIONING_LIT},
  {COM_RANGE_PARTITIONING, COM_RANGE_PARTITIONING_LIT},
  {COM_SYSTEM_PARTITIONING, COM_SYSTEM_PARTITIONING_LIT},
  {COM_HASH_V1_PARTITIONING, COM_HASH_V1_PARTITIONING_LIT},
  {COM_ROUND_ROBIN_PARTITIONING, COM_ROUND_ROBIN_PARTITIONING_LIT},
  {COM_UNSPECIFIED_PARTITIONING, COM_UNSPECIFIED_PARTITIONING_LIT},
  {COM_UNKNOWN_PARTITIONING, COM_UNKNOWN_PARTITIONING_LIT}
};

defXLateFuncs(CmGetComPartAsPartitionScheme, CmGetComPartAsLit, ComPartitioningScheme, PartitioningSchemeXlateArray);

//
//----------------------------------------------------------------------------
// ComAccessPathType translation
//
const literalAndEnumStruct AccessPathTypeXlateArray [] =
{
  {COM_BASE_TABLE_TYPE, COM_BASE_TABLE_TYPE_LIT},
  {COM_INDEX_TYPE, COM_INDEX_TYPE_LIT},
  {COM_LOB_TABLE_TYPE, COM_LOB_TABLE_TYPE_LIT},
  {COM_UNKNOWN_ACCESS_PATH_TYPE, COM_UNKNOWN_ACCESS_PATH_TYPE_LIT}
};

defXLateFuncs(CmGetComAccPathTypeAsAccessPathType, CmGetComAccPathTypeAsLit, ComAccessPathType, AccessPathTypeXlateArray);

//
//----------------------------------------------------------------------------
// ComRowFormat translation
//
const literalAndEnumStruct RowFormatXlateArray [] =
{
  {COM_UNKNOWN_FORMAT_TYPE, COM_UNKNOWN_FORMAT_LIT},
  {COM_ALIGNED_FORMAT_TYPE, COM_ALIGNED_FORMAT_LIT},
  {COM_HBASE_FORMAT_TYPE, COM_HBASE_FORMAT_TYPE_LIT}
  {COM_HBASE_STR_FORMAT_TYPE, COM_HBASE_STR_FORMAT_TYPE_LIT}
};

defXLateFuncs(CmGetComRowFormatAsRowFormat, CmGetRowFormatTypeAsLit, ComRowFormat, RowFormatXlateArray);

//
// ---------------------------------------------------------------------------
// ComCompressionType translation
//
const literalAndEnumStruct CompressionTypeXlateArray [] =
{
   {COM_UNKNOWN_COMPRESSION, COM_UNKNOWN_COMPRESSION_LIT},
   {COM_NO_COMPRESSION, COM_NO_COMPRESSION_LIT},
   {COM_HARDWARE_COMPRESSION, COM_HARDWARE_COMPRESSION_LIT},
   {COM_SOFTWARE_COMPRESSION, COM_SOFTWARE_COMPRESSION_LIT}
};

defXLateFuncs(CmGetComCompressionTypeAsCompressionType, CmGetCompressionTypeAsLit, ComCompressionType, CompressionTypeXlateArray);

//----------------------------------------------------------------------------
// ComInsertMode translation
//
const literalAndEnumStruct InsertModeXlateArray [] =
{
  {COM_REGULAR_TABLE_INSERT_MODE, COM_REGULAR_TABLE_INSERT_MODE_LIT},
  {COM_SET_TABLE_INSERT_MODE, COM_SET_TABLE_INSERT_MODE_LIT},
  {COM_MULTISET_TABLE_INSERT_MODE, COM_MULTISET_TABLE_INSERT_MODE_LIT},
  {COM_UNKNOWN_TABLE_INSERT_MODE, COM_UNKNOWN_TABLE_INSERT_MODE_LIT}
};

defXLateFuncs(CmGetComInsertModeAsInsertMode, CmGetInsertModeAsLit, ComInsertMode, InsertModeXlateArray);

//----------------------------------------------------------------------------
// ComColumnOrdering translation
//
const literalAndEnumStruct ColumnOrderingXlateArray [] =
{
  {COM_ASCENDING_ORDER, COM_ASCENDING_ORDER_LIT},
  {COM_DESCENDING_ORDER, COM_DESCENDING_ORDER_LIT},
  {COM_UNKNOWN_ORDER, COM_UNKNOWN_ORDER_LIT}
};

defXLateFuncs(CmGetComOrderAsComColumnOrder, CmGetComOrderAsLit, ComColumnOrdering, ColumnOrderingXlateArray);

//----------------------------------------------------------------------------
// ComColumnDirection translation
//
const literalAndEnumStruct ColumnDirectionXlateArray [] =
{
  {COM_INPUT_COLUMN, COM_INPUT_COLUMN_LIT},
  {COM_OUTPUT_COLUMN, COM_OUTPUT_COLUMN_LIT},
  {COM_INOUT_COLUMN, COM_INOUT_COLUMN_LIT},
  {COM_UNKNOWN_DIRECTION, COM_UNKNOWN_DIRECTION_LIT}
};

defXLateFuncs(CmGetComDirectionAsComColumnDirection, CmGetComDirectionAsLit, ComColumnDirection, ColumnDirectionXlateArray);

//----------------------------------------------------------------------------
// ComParamDirection translation
//
const literalAndEnumStruct ParamDirectionXlateArray [] =
{
  {COM_INPUT_PARAM, COM_INPUT_PARAM_LIT},
  {COM_OUTPUT_PARAM, COM_OUTPUT_PARAM_LIT},
  {COM_INOUT_PARAM, COM_INOUT_PARAM_LIT},
  {COM_UNKNOWN_PARAM_DIRECTION, COM_UNKNOWN_PARAM_DIRECTION_LIT}
};

defXLateFuncs(CmGetComDirectionAsComParamDirection, CmGetComParamDirectionAsLit, ComParamDirection, ParamDirectionXlateArray);

//----------------------------------------------------------------------------
// ComColumnClass translation
//
const literalAndEnumStruct ColumnClassXlateArray [] =
{
  {COM_USER_COLUMN, COM_USER_COLUMN_LIT},
  {COM_SYSTEM_COLUMN, COM_SYSTEM_COLUMN_LIT},
  {COM_ADDED_USER_COLUMN, COM_ADDED_USER_COLUMN_LIT},
  {COM_MV_SYSTEM_ADDED_COLUMN, COM_MV_SYSTEM_ADDED_COLUMN_LIT},
  {COM_UNKNOWN_CLASS, COM_UNKNOWN_CLASS_LIT}
};

defXLateFuncs(CmGetComClassAsComColumnClass, CmGetComClassAsLit, ComColumnClass, ColumnClassXlateArray);

//----------------------------------------------------------------------------
// ComStoreByDetails translation
//
const literalAndEnumStruct StoreByDetailsXlateArray [] =
{
  {COM_STOREBY_DETAILS_V1, COM_STOREBY_DETAILS_V1_LIT},
  {COM_STOREBY_DETAILS_V2, COM_STOREBY_DETAILS_V2_LIT},
  {COM_STOREBY_DETAILS_UNKNOWN, COM_STOREBY_DETAILS_UNKNOWN_LIT}
};

defXLateFuncs(CmGetStoreByDetailsAsComStoreByDetails, CmGetComStoreByDetailsAsLit, ComStoreByDetails, StoreByDetailsXlateArray);

//----------------------------------------------------------------------------
// ComODBCDataType translation
//
const literalAndEnumStruct ODBCDataTypeXlateArray [] =
{
  {COM_CHARACTER_ODT, COM_CHARACTER_ODT_LIT},
  {COM_VARCHAR_ODT, COM_VARCHAR_ODT_LIT},
  {COM_LONG_VARCHAR_ODT, COM_LONG_VARCHAR_ODT_LIT},
  {COM_NUMERIC_SIGNED_ODT, COM_NUMERIC_SIGNED_ODT_LIT},
  {COM_NUMERIC_UNSIGNED_ODT, COM_NUMERIC_UNSIGNED_ODT_LIT},
  {COM_SMALLINT_SIGNED_ODT, COM_SMALLINT_SIGNED_ODT_LIT},
  {COM_SMALLINT_UNSIGNED_ODT, COM_SMALLINT_UNSIGNED_ODT_LIT},
  {COM_INTEGER_SIGNED_ODT, COM_INTEGER_SIGNED_ODT_LIT},
  {COM_INTEGER_UNSIGNED_ODT, COM_INTEGER_UNSIGNED_ODT_LIT},
  {COM_LARGEINT_SIGNED_ODT, COM_LARGEINT_SIGNED_ODT_LIT},
  {COM_BIGINT_SIGNED_ODT, COM_BIGINT_SIGNED_ODT_LIT},
  {COM_FLOAT_ODT, COM_FLOAT_ODT_LIT},
  {COM_REAL_ODT, COM_REAL_ODT_LIT},
  {COM_DOUBLE_ODT, COM_DOUBLE_ODT_LIT},
  {COM_DECIMAL_SIGNED_ODT, COM_DECIMAL_SIGNED_ODT_LIT},
  {COM_DECIMAL_UNSIGNED_ODT, COM_DECIMAL_UNSIGNED_ODT_LIT},
  {COM_BLOB_SDT, COM_BLOB_ODT_LIT},
  {COM_CLOB_SDT, COM_CLOB_ODT_LIT},
  {COM_BOOLEAN_SDT, COM_BOOLEAN_ODT_LIT},
  {COM_BINARY_SDT, COM_BINARY_ODT_LIT},
  {COM_VARBINARY_SDT, COM_VARBINARY_ODT_LIT},
  {COM_DATETIME_ODT, COM_DATETIME_ODT_LIT},
  {COM_TIMESTAMP_ODT, COM_TIMESTAMP_ODT_LIT},
  {COM_DATE_ODT, COM_DATE_ODT_LIT},
  {COM_TIME_ODT, COM_TIME_ODT_LIT},
  {COM_INTERVAL_ODT, COM_INTERVAL_ODT_LIT},
  {COM_UNKNOWN_ODT, COM_UNKNOWN_ODT_LIT}
};

defXLateFuncs(CmGetODBCTypeAsComODBCDataType, CmGetComODBCTypeAsLit, ComODBCDataType, ODBCDataTypeXlateArray);

//----------------------------------------------------------------------------
// ComSQLDataType translation
//
const literalAndEnumStruct SQLDataTypeXlateArray [] =
{
  {COM_CHARACTER_SDT, COM_CHARACTER_SDT_LIT},
  {COM_VARCHAR_SDT, COM_VARCHAR_SDT_LIT},
  {COM_LONG_VARCHAR_SDT, COM_LONG_VARCHAR_SDT_LIT},
  {COM_NUMERIC_SIGNED_SDT, COM_NUMERIC_SIGNED_SDT_LIT},
  {COM_NUMERIC_UNSIGNED_SDT, COM_NUMERIC_UNSIGNED_SDT_LIT},
  {COM_SMALLINT_SIGNED_SDT, COM_SMALLINT_SIGNED_SDT_LIT},
  {COM_SMALLINT_UNSIGNED_SDT, COM_SMALLINT_UNSIGNED_SDT_LIT},
  {COM_INTEGER_SIGNED_SDT, COM_INTEGER_SIGNED_SDT_LIT},
  {COM_INTEGER_UNSIGNED_SDT, COM_INTEGER_UNSIGNED_SDT_LIT},
  {COM_LARGEINT_SIGNED_SDT, COM_LARGEINT_SIGNED_SDT_LIT},
  {COM_BPINT_UNSIGNED_SDT, COM_BPINT_UNSIGNED_SDT_LIT},
  {COM_FLOAT_SDT, COM_FLOAT_SDT_LIT},
  {COM_REAL_SDT, COM_REAL_SDT_LIT},
  {COM_DOUBLE_SDT, COM_DOUBLE_SDT_LIT},
  {COM_DECIMAL_SIGNED_SDT, COM_DECIMAL_SIGNED_SDT_LIT},
  {COM_DECIMAL_UNSIGNED_SDT, COM_DECIMAL_UNSIGNED_SDT_LIT},
  {COM_LARGE_DECIMAL_SIGNED_SDT, COM_DECIMAL_SIGNED_SDT_LIT},  // one-way conversion enum->literal
  {COM_BLOB_SDT, COM_BLOB_SDT_LIT},
  {COM_CLOB_SDT, COM_CLOB_SDT_LIT},
  {COM_BOOLEAN_SDT, COM_BOOLEAN_SDT_LIT},
  {COM_BINARY_SDT, COM_BINARY_SDT_LIT},
  {COM_VARBINARY_SDT, COM_VARBINARY_SDT_LIT},
  {COM_DATETIME_SDT, COM_DATETIME_SDT_LIT},
  {COM_TIMESTAMP_SDT, COM_TIMESTAMP_SDT_LIT},
  {COM_DATE_SDT, COM_DATE_SDT_LIT},
  {COM_TIME_SDT, COM_TIME_SDT_LIT},
  {COM_INTERVAL_SDT, COM_INTERVAL_SDT_LIT},
  {COM_UNKNOWN_SDT, COM_UNKNOWN_SDT_LIT}
};

defXLateFuncs(CmGetComSQLTypeAsComSQLDataType, CmGetComSQLTypeAsLit, ComSQLDataType, SQLDataTypeXlateArray);

//----------------------------------------------------------------------------
// ComColumnDefaultClass translation
//
const literalAndEnumStruct ColumnDefaultClassXlateArray [] =
{
  {COM_NULL_DEFAULT, COM_NULL_DEFAULT_LIT},
  {COM_NO_DEFAULT, COM_NO_DEFAULT_LIT},
  {COM_USER_DEFINED_DEFAULT, COM_USER_DEFINED_DEFAULT_LIT},
  {COM_USER_FUNCTION_DEFAULT, COM_USER_FUNCTION_DEFAULT_LIT},
  {COM_CURRENT_DEFAULT, COM_CURRENT_DEFAULT_LIT},
  {COM_CURRENT_UT_DEFAULT, COM_CURRENT_UT_DEFAULT_LIT},
  {COM_UUID_DEFAULT, COM_UUID_DEFAULT_LIT},
  {COM_FUNCTION_DEFINED_DEFAULT, COM_FUNCTION_DEFINED_DEFAULT_LIT},
  {COM_IDENTITY_GENERATED_BY_DEFAULT, COM_IDENTITY_GENERATED_BY_DEFAULT_LIT},
  {COM_IDENTITY_GENERATED_ALWAYS, COM_IDENTITY_GENERATED_ALWAYS_LIT},
  {COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT, COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT_LIT},
  {COM_ALWAYS_DEFAULT_COMPUTED_COLUMN_DEFAULT, COM_ALWAYS_DEFAULT_COMPUTED_COLUMN_DEFAULT_LIT}
};

defXLateFuncs(CmGetComColDefaultAsColDefault, CmGetComColDefaultAsLit, ComColumnDefaultClass, ColumnDefaultClassXlateArray);

//----------------------------------------------------------------------------
// ComParamDefaultClass translation
//
const literalAndEnumStruct ParamDefaultClassXlateArray [] =
{
  {COM_NO_PARAM_DEFAULT, COM_NO_PARAM_DEFAULT_LIT},  // the default
  {COM_CURRENT_PARAM_DEFAULT, COM_CURRENT_PARAM_DEFAULT_LIT},              // e.g. DEFAULT CURRENT DATE
  {COM_CURRENT_UT_PARAM_DEFAULT, COM_CURRENT_UT_PARAM_DEFAULT_LIT},              // e.g. DEFAULT CURRENT DATE
  {COM_UUID_PARAM_DEFAULT, COM_UUID_PARAM_DEFAULT_LIT},              // e.g. DEFAULT CURRENT DATE
  {COM_FUNCTION_DEFINED_PARAM_DEFAULT, COM_FUNCTION_DEFINED_PARAM_DEFAULT_LIT},              // e.g. DEFAULT CURRENT DATE
  {COM_NULL_PARAM_DEFAULT, COM_NULL_PARAM_DEFAULT_LIT},                    // i.e. DEFAULT NULL
  {COM_USER_DEFINED_PARAM_DEFAULT, COM_USER_DEFINED_PARAM_DEFAULT_LIT},    // e.g. DEFAULT 'a-string-literal'
  {COM_USER_FUNCTION_PARAM_DEFAULT, COM_USER_FUNCTION_PARAM_DEFAULT_LIT},  // e.g. DEFAULT CURRENT USER
  {COM_ALWAYS_COMPUTE_COMPUTED_PARAM_DEFAULT, COM_ALWAYS_COMPUTE_COMPUTED_PARAM_DEFAULT_LIT}, // for future internal use only
  {COM_ALWAYS_DEFAULT_COMPUTED_PARAM_DEFAULT, COM_ALWAYS_DEFAULT_COMPUTED_PARAM_DEFAULT_LIT}  // for future internal use only
};

defXLateFuncs(CmGetParamDefaultClassAsComParamDefaultClass,
              CmGetComParamDefaultClassAsLit,
              ComParamDefaultClass,
              ParamDefaultClassXlateArray);

//----------------------------------------------------------------------------
// ComGrantorType translation
//
const literalAndEnumStruct GrantorTypeXlateArray [] =
{
  {COM_SYSTEM_GRANTOR, COM_SYSTEM_GRANTOR_LIT},
  {COM_USER_GRANTOR, COM_USER_GRANTOR_LIT},
  {COM_SCHEMA_OWNER_GRANTOR, COM_SCHEMA_OWNER_GRANTOR_LIT},
  {COM_UNKNOWN_GRANTOR_TYPE, COM_UNKNOWN_GRANTOR_TYPE_LIT}
};

defXLateFuncs(CmGetComGrantorAsGrantorType, CmGetComGrantorAsLit, ComGrantorType, GrantorTypeXlateArray);

//----------------------------------------------------------------------------
// ComGranteeType translation
//
const literalAndEnumStruct GranteeTypeXlateArray [] =
{
  {COM_PUBLIC_GRANTEE, COM_PUBLIC_GRANTEE_LIT},
  {COM_USER_GRANTEE, COM_USER_GRANTEE_LIT},
  {COM_SCHEMA_OWNER_GRANTEE, COM_SCHEMA_OWNER_GRANTEE_LIT},
  {COM_UNKNOWN_GRANTEE_TYPE, COM_UNKNOWN_GRANTEE_TYPE_LIT}
};

defXLateFuncs(CmGetComGranteeAsGranteeType, CmGetComGranteeAsLit, ComGranteeType, GranteeTypeXlateArray);

//----------------------------------------------------------------------------
// ComAutoRebindOption translation
//
const literalAndEnumStruct AutoRebindOptionXlateArray [] =
{
  // no translations, we don't support MODULE objects anyway
  {COM_UNKNOWN_RBND_OPTION, COM_UNKNOWN_RBND_OPTION_LIT}
};

defXLateFuncs(CmGetComRebindAsAutoRebindOption, CmGetComRebindAsLit, ComAutoRebindOption, AutoRebindOptionXlateArray);

//----------------------------------------------------------------------------
// ComRCMatchOption translation
//
const literalAndEnumStruct RCMatchOptionXlateArray [] =
{
  {COM_FULL_MATCH_OPTION, COM_FULL_MATCH_OPTION_LIT},
  {COM_NONE_MATCH_OPTION, COM_NONE_MATCH_OPTION_LIT},
  {COM_PARTIAL_MATCH_OPTION, COM_PARTIAL_MATCH_OPTION_LIT},
  {COM_UNKNOWN_MATCH_OPTION, COM_UNKNOWN_MATCH_OPTION_LIT}
};

defXLateFuncs(CmGetComRCMatchAsRCMatch, CmGetComRCMatchAsLit, ComRCMatchOption, RCMatchOptionXlateArray);

//----------------------------------------------------------------------------
// ComRCUpdateRule translation
//
const literalAndEnumStruct RCUpdateRuleXlateArray [] =
{
  {COM_CASCADE_UPDATE_RULE, COM_CASCADE_UPDATE_RULE_LIT},
  {COM_NO_ACTION_UPDATE_RULE, COM_NO_ACTION_UPDATE_RULE_LIT},
  {COM_SET_DEFAULT_UPDATE_RULE, COM_SET_DEFAULT_UPDATE_RULE_LIT},
  {COM_SET_NULL_UPDATE_RULE, COM_SET_NULL_UPDATE_RULE_LIT},
  {COM_RESTRICT_UPDATE_RULE, COM_RESTRICT_UPDATE_RULE_LIT},
  {COM_UNKNOWN_UPDATE_RULE, COM_UNKNOWN_UPDATE_RULE_LIT}
};

defXLateFuncs(CmGetComRCUpdateAsRCUpdate, CmGetComRCUpdateAsLit, ComRCUpdateRule, RCUpdateRuleXlateArray);

//----------------------------------------------------------------------------
// ComRCDeleteRule translation
//
const literalAndEnumStruct RCDeleteRuleXlateArray [] =
{
  {COM_CASCADE_DELETE_RULE, COM_CASCADE_DELETE_RULE_LIT},
  {COM_NO_ACTION_DELETE_RULE, COM_NO_ACTION_DELETE_RULE_LIT},
  {COM_SET_DEFAULT_DELETE_RULE, COM_SET_DEFAULT_DELETE_RULE_LIT},
  {COM_SET_NULL_DELETE_RULE, COM_SET_NULL_DELETE_RULE_LIT},
  {COM_RESTRICT_DELETE_RULE, COM_RESTRICT_DELETE_RULE_LIT},
  {COM_UNKNOWN_DELETE_RULE, COM_UNKNOWN_DELETE_RULE_LIT}
};

defXLateFuncs(CmGetComRCDeleteAsRCDelete, CmGetComRCDeleteAsLit, ComRCDeleteRule, RCDeleteRuleXlateArray);

//----------------------------------------------------------------------------
// ComConstraintType translation
//
const literalAndEnumStruct ConstraintTypeXlateArray [] =
{
  {COM_CHECK_CONSTRAINT, COM_CHECK_CONSTRAINT_LIT},
  {COM_PRIMARY_KEY_CONSTRAINT, COM_PRIMARY_KEY_CONSTRAINT_LIT},
  {COM_UNIQUE_CONSTRAINT, COM_UNIQUE_CONSTRAINT_LIT},
  {COM_FOREIGN_KEY_CONSTRAINT, COM_FOREIGN_KEY_CONSTRAINT_LIT},
  {COM_UNKNOWN_CONSTRAINT, COM_UNKNOWN_CONSTRAINT_LIT}
};

defXLateFuncs(CmGetComConstrTypeAsConstrType, CmGetComConstrTypeAsLit, ComConstraintType, ConstraintTypeXlateArray);

//----------------------------------------------------------------------------
// ComViewCheckOption translation
//
const literalAndEnumStruct ViewCheckOptionXlateArray [] =
{
  {COM_CASCADE_CHECK_OPTION, COM_CASCADE_CHECK_OPTION_LIT},
  {COM_LOCAL_CHECK_OPTION, COM_LOCAL_CHECK_OPTION_LIT},
  {COM_NONE_CHECK_OPTION, COM_NONE_CHECK_OPTION_LIT},
  {COM_UNKNOWN_CHECK_OPTION, COM_UNKNOWN_CHECK_OPTION_LIT}
};

defXLateFuncs(CmGetComViewCheckAsViewCheckOption, CmGetComViewCheckAsLit, ComViewCheckOption, ViewCheckOptionXlateArray);

//----------------------------------------------------------------------------
// ComViewType translation -- added in v2500
//
const literalAndEnumStruct ViewTypeXlateArray [] =
{
  {COM_USER_VIEW_TYPE, COM_USER_VIEW_TYPE_LIT},
  {COM_SYSTEM_VIEW_TYPE, COM_SYSTEM_VIEW_TYPE_LIT},
  {COM_UNKNOWN_VIEW_TYPE, COM_UNKNOWN_VIEW_TYPE_LIT}
};

defXLateFuncs(CmGetComViewTypeAsViewType, CmGetComViewTypeAsLit, ComViewType, ViewTypeXlateArray);

//----------------------------------------------------------------------------
// ComPartnStatus translation
//
const literalAndEnumStruct PartnStatusXlateArray [] =
{
  {COM_PARTN_AVAILABLE, COM_PARTN_AVAILABLE_LIT},
  {COM_PARTN_OFFLINE, COM_PARTN_OFFLINE_LIT},
  {COM_PARTN_PHANTOM, COM_PARTN_PHANTOM_LIT},
  {COM_PARTN_CORRUPT, COM_PARTN_CORRUPT_LIT},
  {COM_UNKNOWN_PARTN_STATUS, COM_UNKNOWN_PARTN_STATUS_LIT}
};

defXLateFuncs(CmGetComPartnStatusAsPartnStatus, CmGetComPartnStatusAsLit, ComPartnStatus, PartnStatusXlateArray);

//----------------------------------------------------------------------------
// ComDdlStatus translation
//
const literalAndEnumStruct DdlStatusXlateArray [] =
{
  {COM_NO_DDL_IN_PROGRESS, COM_NO_DDL_IN_PROGRESS_LIT},
  {COM_ROW_HIDING, COM_ROW_HIDING_LIT},
  {COM_KEY_RANGE_CHECKING, COM_KEY_RANGE_CHECKING_LIT},
  {COM_UNKNOWN_DDL_STATUS, COM_UNKNOWN_DDL_STATUS_LIT}
};

defXLateFuncs(CmGetComDdlStatusAsDdlStatus, CmGetComDdlStatusAsLit, ComDdlStatus, DdlStatusXlateArray);

//----------------------------------------------------------------------------
// ComTableRestrictionAttr translation
//
const literalAndEnumStruct TableFeatureXlateArray [] =
{
  {COM_DROPPABLE, COM_DROPPABLE_LIT},
  {COM_DROPPABLE_INSERT_ONLY, COM_DROPPABLE_INSERT_ONLY_LIT},
  {COM_NOT_DROPPABLE, COM_NOT_DROPPABLE_LIT},
  {COM_NOT_DROPPABLE_INSERT_ONLY, COM_NOT_DROPPABLE_INSERT_ONLY_LIT},
  {COM_UNKNOWN_TABLE_FEATURE, COM_UNKNOWN_TABLE_FEATURE_LIT}
};

defXLateFuncs(CmGetTableFeature, CmGetTableFeatureAsLit, ComTableFeature, TableFeatureXlateArray);


//----------------------------------------------------------------------------
// ComBoolean translation
//
const literalAndEnumStruct BooleanXlateArray [] =
{
  {TRUE, COM_YES_LIT},
  {FALSE, COM_NO_LIT}
};

defXLateFuncs( YN_TRUEFALSE, CmGetComBooleanAsLit, ComBoolean, BooleanXlateArray);

//----------------------------------------------------------------------------
// ComActivationTime translation
//
const literalAndEnumStruct ActivationTimeXlateArray [] =
{
  {COM_BEFORE, COM_BEFORE_LIT},
  {COM_AFTER, COM_AFTER_LIT},
  {COM_UNKNOWN_TIME, COM_UNKNOWN_TIME_LIT}
};

defXLateFuncs( CmGetComActivationTime, CmGetComActivationTimeAsLit, ComActivationTime, ActivationTimeXlateArray);

//----------------------------------------------------------------------------
// ComOperation translation
//
const literalAndEnumStruct OperationXlateArray [] =
{
  {COM_INSERT, COM_INSERT_LIT},
  {COM_DELETE, COM_DELETE_LIT},
  {COM_UPDATE, COM_UPDATE_LIT},
  {COM_SELECT, COM_SELECT_LIT},
  {COM_ROUTINE, COM_ROUTINE_LIT},
  {COM_UNKNOWN_IUD, COM_UNKNOWN_IUD_LIT}
};

defXLateFuncs( CmGetComOperation, CmGetComOperationAsLit, ComOperation, OperationXlateArray);

//----------------------------------------------------------------------------
// ComGranularity translation
//
const literalAndEnumStruct GranularityXlateArray [] =
{
  {COM_ROW, COM_ROW_LIT},
  {COM_STATEMENT, COM_STATEMENT_LIT},
  {COM_UNKNOWN_GRANULARITY, COM_UNKNOWN_GRANULARITY_LIT}
};

defXLateFuncs( CmGetComGranularity, CmGetComGranularityAsLit, ComGranularity, GranularityXlateArray);

//----------------------------------------------------------------------------
// ComYesNo translation
//
const literalAndEnumStruct YesNoXlateArray [] =
{
  {COM_YES, COM_YES_LIT},
  {COM_NO, COM_NO_LIT},
  {COM_NULL, COM_NULL_LIT}
};

defXLateFuncs( CmGetComYesNo, CmGetComYesNoAsLit, ComYesNo, YesNoXlateArray);

//----------------------------------------------------------------------------
// ComMVType translation
//
const literalAndEnumStruct MVTypeXlateArray [] =
{
  {COM_MJV, COM_MJV_LIT},
  {COM_MAV, COM_MAV_LIT},
  {COM_MAJV, COM_MAJV_LIT},
  {COM_MV_OTHER, COM_MV_OTHER_LIT},
  {COM_MV_UNKNOWN, COM_MV_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComMVType, CmGetComMVTypeAsLit, ComMVType, MVTypeXlateArray);

//----------------------------------------------------------------------------
// ComMVStatus translation
//
const literalAndEnumStruct MVStatusXlateArray [] =
{
  {COM_MVSTATUS_INITIALIZED, COM_MVSTATUS_INITIALIZED_LIT},
  {COM_MVSTATUS_NO_INITIALIZATION, COM_MVSTATUS_NO_INITIALIZATION_LIT},
  {COM_MVSTATUS_NOT_INITIALIZED, COM_MVSTATUS_NOT_INITIALIZED_LIT},
  {COM_MVSTATUS_UNAVAILABLE, COM_MVSTATUS_UNAVAILABLE_LIT},
  {COM_MVSTATUS_UNKNOWN, COM_MVSTATUS_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComMVStatus, CmGetComMVStatusAsLit, ComMVStatus, MVStatusXlateArray);

//----------------------------------------------------------------------------
// ComRangeLogType translation
//
const literalAndEnumStruct RangeLogTypeXlateArray [] =
{
  {COM_NO_RANGELOG, COM_NO_RANGELOG_LIT},
  {COM_MANUAL_RANGELOG, COM_MANUAL_RANGELOG_LIT},
  {COM_AUTO_RANGELOG, COM_AUTO_RANGELOG_LIT},
  {COM_MIXED_RANGELOG, COM_MIXED_RANGELOG_LIT},
  {COM_RANGELOG_UNKNOWN, COM_RANGELOG_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComRangeLogType, CmGetComRangeLogTypeAsLit, ComRangeLogType, RangeLogTypeXlateArray);

//----------------------------------------------------------------------------
// ComMvsAllowed translation
//
const literalAndEnumStruct MvsAllowedXlateArray [] =
{
  {COM_NO_MVS_ALLOWED, COM_NO_MVS_ALLOWED_LIT},
  {COM_ALL_MVS_ALLOWED, COM_ALL_MVS_ALLOWED_LIT},
  {COM_ON_STATEMENT_MVS_ALLOWED, COM_ON_STATEMENT_MVS_ALLOWED_LIT},
  {COM_ON_REQUEST_MVS_ALLOWED, COM_ON_REQUEST_MVS_ALLOWED_LIT},
  {COM_RECOMPUTE_MVS_ALLOWED, COM_RECOMPUTE_MVS_ALLOWED_LIT},
  {COM_MVS_ALLOWED_UNKNOWN, COM_MVS_ALLOWED_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComMvsAllowed, CmGetComMvsAllowedAsLit, ComMvsAllowed, MvsAllowedXlateArray);

//----------------------------------------------------------------------------
// ComMvAuditType translation
//
const literalAndEnumStruct MvAuditTypeXlateArray [] =
{
  {COM_MV_AUDIT, COM_MV_AUDIT_LIT},
  {COM_MV_NO_AUDIT, COM_MV_NO_AUDIT_LIT},
  {COM_MV_NO_AUDIT_ON_REFRESH, COM_MV_NO_AUDIT_ON_REFRESH_LIT},
  {COM_MV_AUDIT_UNKNOWN, COM_MV_AUDIT_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComMvAuditType, CmGetComMvAuditTypeAsLit, ComMvAuditType, MvAuditTypeXlateArray);

//----------------------------------------------------------------------------
// ComMVRefreshType translation
//
const literalAndEnumStruct MVRefreshTypeXlateArray [] =
{
  {COM_ON_STATEMENT, COM_ON_STATEMENT_LIT},
  {COM_ON_REQUEST, COM_ON_REQUEST_LIT},
  {COM_RECOMPUTE, COM_RECOMPUTE_LIT},
  {COM_BY_USER, COM_BY_USER_LIT},
  {COM_UNKNOWN_RTYPE, COM_UNKNOWN_RTYPE_LIT}
};

defXLateFuncs( CmGetComMVRefreshType, CmGetComMVRefreshTypeAsLit, ComMVRefreshType, MVRefreshTypeXlateArray);

//----------------------------------------------------------------------------
// ComLeftJoinTableType translation
//
const literalAndEnumStruct LeftJoinTableTypeXlateArray [] =
{
  {COM_NO_LEFT_JOIN, COM_NO_LEFT_JOIN_LIT},
  {COM_LEFT_INNER, COM_LEFT_INNER_LIT},
  {COM_LEFT_OUTER, COM_LEFT_OUTER_LIT},
  {COM_LEFT_JOIN_UNKNOWN, COM_LEFT_JOIN_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComLeftJoinTableType, CmGetComLeftJoinTableTypeAsLit, ComLeftJoinTableType, LeftJoinTableTypeXlateArray);

//----------------------------------------------------------------------------
// ComMVIncRefStatus translation
//
const literalAndEnumStruct MVIncRefStatusXlateArray [] =
{
  {COM_REF_STAT_OK, COM_REF_STAT_OK_LIT},
  {COM_REF_STAT_RECOMPUTE_REQUIRED, COM_REF_STAT_RECOMPUTE_REQUIRED_LIT},
  {COM_REF_STAT_LOCK_REQUIRED, COM_REF_STAT_LOCK_REQUIRED_LIT},
  {COM_REF_STAT_UNKNOWN, COM_REF_STAT_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComIncRefStatus, CmGetComIncRefStatusAsLit, ComMVIncRefStatus, MVIncRefStatusXlateArray);

//----------------------------------------------------------------------------
// ComMVColType translation
//
const literalAndEnumStruct MVColTypeXlateArray [] =
{
  {COM_MVCOL_GROUPBY, COM_MVCOL_GROUPBY_LIT},
  {COM_MVCOL_CONST, COM_MVCOL_CONST_LIT},
  {COM_MVCOL_AGGREGATE, COM_MVCOL_AGGREGATE_LIT},
  {COM_MVCOL_DUPLICATE, COM_MVCOL_DUPLICATE_LIT},
  {COM_MVCOL_OTHER, COM_MVCOL_OTHER_LIT},
  {COM_MVCOL_FUNCTION, COM_MVCOL_FUNCTION_LIT},
  {COM_MVCOL_BASECOL, COM_MVCOL_BASECOL_LIT},
  {COM_MVCOL_REDUNDANT, COM_MVCOL_REDUNDANT_LIT},
  {COM_MVCOL_COMPLEX, COM_MVCOL_COMPLEX_LIT},
  {COM_MVCOL_UNKNOWN, COM_MVCOL_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComMVColType, CmGetComMVColTypeAsLit, ComMVColType, MVColTypeXlateArray);

// NOTE: The following routines map only a __subset__ of OperatorTypeEnum
// into literals
//----------------------------------------------------------------------------
// OperatorTypeEnum translation
//
const literalAndEnumStruct OperatorTypeXlateArray [] =
{
  {ITM_COUNT, COM_COUNT_LIT},
  {ITM_COUNT_NONULL, COM_COUNT_NONULL_LIT},
  {ITM_SUM, COM_SUM_LIT},
  {ITM_AVG, COM_AVG_LIT},
  {ITM_MIN, COM_MIN_LIT},
  {ITM_MAX, COM_MAX_LIT},
  {ITM_VARIANCE, COM_VARIANCE_LIT},
  {ITM_STDDEV, COM_STDDEV_LIT},
  {ITM_BASECOLUMN, COM_BASECOL_LIT}
};

// Cannot use defXLateFuncs because both directions may translate values that don't exist in the
// translation array.

void CmGetMVOperatorTypeAsLit (const OperatorTypeEnum e, char * l)
{
  NABoolean found;
  enumToLiteral (OperatorTypeXlateArray, occurs(OperatorTypeXlateArray), e, l, found);
  if (!found)
    strcpy (l, COM_UNKNOWN_AGG_LIT);
}

OperatorTypeEnum CmGetMVOperatorTypeEnum (const char * l)
{
  NABoolean found;
  OperatorTypeEnum result = (OperatorTypeEnum) literalToEnum (OperatorTypeXlateArray, occurs(OperatorTypeXlateArray), l, found);
  if (!found)
    result = ITM_IS_UNKNOWN;
  return result;
}

//----------------------------------------------------------------------------
// ComMVSUsedTableAttribute translation
//
const literalAndEnumStruct MVSUsedTableAttributeXlateArray [] =
{
  {COM_IGNORE_CHANGES, COM_IGNORE_CHANGES_LIT},
  {COM_INSERT_ONLY, COM_INSERT_ONLY_LIT},
  {COM_NO_ATTRIBUTE, COM_NO_ATTRIBUTE_LIT}
};

defXLateFuncs( CmGetMVSUsedTableAttribute, CmGetMVSUsedTableAttributeAsLit, ComMVSUsedTableAttribute, MVSUsedTableAttributeXlateArray);

//----------------------------------------------------------------------------
// ComMVSUsageType translation
//
const literalAndEnumStruct MVSUsageTypeXlateArray [] =
{
  {COM_USER_SPECIFIED, COM_USER_SPECIFIED_LIT},
  {COM_DIRECT_USAGE, COM_DIRECT_USAGE_LIT},
  {COM_EXPANDED_USAGE, COM_EXPANDED_USAGE_LIT},
  {COM_UNKNOWN_USAGE, COM_UNKNOWN_USAGE_LIT}
};

defXLateFuncs( CmGetMVSUsageType, CmGetMVSUsageTypeAsLit, ComMVSUsageType, MVSUsageTypeXlateArray);

//----------------------------------------------------------------------------
// ComMVAttribute translation
//
const literalAndEnumStruct MVAttributeXlateArray [] =
{
  {COM_MVATTRIBUTE_UNKNOWN, COM_MV_ATTRIBUTE_UNKNOWN_LIT}
};

defXLateFuncs( CmGetComMVAttribute, CmGetComMVAttributeAsLit, ComMVAttribute, MVAttributeXlateArray);

//----------------------------------------------------------------------------
// ComHistReasonType translation
//
const literalAndEnumStruct HistReasonTypeXlateArray [] =
{
  {COM_HIST_MANUAL, COM_HIST_MANUAL_LIT},
  {COM_HIST_INITIAL, COM_HIST_INITIAL_LIT},
  {COM_HIST_AUTO_REGEN_NEEDED, COM_HIST_AUTO_REGEN_NEEDED_LIT},
  {COM_HIST_NOT_CREATED, COM_HIST_NOT_CREATED_LIT}
};

defXLateFuncs( CmGetHistReasonType, CmGetHistReasonTypeAsLit, ComHistReasonType, HistReasonTypeXlateArray);

//----------------------------------------------------------------------------
// ComExceptionTableType translation
//
const literalAndEnumStruct ExceptionTableTypeXlateArray [] =
{
  {COM_VALIDATE_EXCEPTION_TABLE_TYPE, COM_VALIDATE_EXCEPTION_TABLE_TYPE_LIT},
  {COM_UNKNOWN_EXCEPTION_TABLE_TYPE, COM_UNKNOWN_EXCEPTION_TABLE_TYPE_LIT}
};

defXLateFuncs( CmGetExceptionTableType, CmGetExceptionTableTypeAsLit, ComExceptionTableType, ExceptionTableTypeXlateArray);


//----------------------------------------------------------------------------
// ComRoleIdStatus translation
//
const literalAndEnumStruct RoleIdStatusXlateArray [] =
{
  {COM_AVAILABLE_STATUS, COM_AVAILABLE_STATUS_LIT},
  {COM_USED_STATUS, COM_USED_STATUS_LIT},
  {COM_UNKNOWN_STATUS, COM_UNKNOWN_STATUS_LIT}
};

defXLateFuncs(CmGetRoleIdStatus, CmGetRoleIdStatusAsLit, ComRoleIdStatus, RoleIdStatusXlateArray);

//----------------------------------------------------------------------------
// ComSequenceGeneratorType translation
//
const literalAndEnumStruct SequenceGeneratorTypeXlateArray [] =
{
  {COM_INTERNAL_SG, COM_INTERNAL_SG_LIT},
  {COM_EXTERNAL_SG, COM_EXTERNAL_SG_LIT},
  {COM_INTERNAL_COMPUTED_SG, COM_INTERNAL_COMPUTED_SG_LIT},
  {COM_UNKNOWN_SG, COM_UNKNOWN_SG_LIT}
};

defXLateFuncs(CmGetSequenceGeneratorType, CmGetSequenceGeneratorTypeAsLit, ComSequenceGeneratorType, SequenceGeneratorTypeXlateArray);


//----------------------------------------------------------------------------
// ComSchemaType translation
//
const literalAndEnumStruct schemaTypeArray [] =
{
  {COM_USER_TYPE, COM_USER_TYPE_LIT},
  {COM_PUBLIC_TYPE, COM_PUBLIC_TYPE_LIT},
  {COM_SYSTEM_TYPE, COM_SYSTEM_TYPE_LIT}
};

defXLateFuncs( CmGetComSchemaType, CmGetComSchemaTypeLit, ComSchemaType, schemaTypeArray);

//----------------------------------------------------------------------------
// Version 2000 grantor/grantee translation function.
// These will be used for
// - version 1200->2000 translations
// - early version 2000-> real version 2000 translations (fixup for internal-only use)
void CmTranslateV1200ToV2000Privs ( ComGrantorType & grantorType
                                  , ComGranteeType & granteeType
                                  , const ComUserID & grantor
                                  , const ComUserID & objectOwner
                                  , const ComUserID & schemaOwner)
{

  if (grantorType == COM_SYSTEM_GRANTOR && granteeType == COM_USER_GRANTEE)
    granteeType = COM_SCHEMA_OWNER_GRANTEE;

  if (grantor == schemaOwner)
    grantorType = COM_SCHEMA_OWNER_GRANTOR;

}

//----------------------------------------------------------------------------
// Added for Security in SQ 1.0
//----------------------------------------------------------------------------
// Translation for columns that hold ComIdClass
//
const literalAndEnumStruct IdClassXlateArray [] =
{
  {COM_ROLE_CLASS, COM_ROLE_CLASS_LIT},
  {COM_USER_CLASS, COM_USER_CLASS_LIT},
  //{COM_GROUP_CLASS, COM_GROUP_CLASS_LIT},
  {COM_UNKNOWN_ID_CLASS, COM_UNKNOWN_ID_CLASS_LIT}
};

defXLateFuncs(CmGetLitAsIdClass, CmGetComIdClassAsLit, ComIdClass, IdClassXlateArray);

// Translation for ID status in ID_MAPPING table
const literalAndEnumStruct IdStatusXlateArray [] =
{
  {COM_AVAILABLE_ID_STATUS, COM_AVAILABLE_ID_STATUS_LIT},
  {COM_USED_ID_STATUS, COM_USED_ID_STATUS_LIT},
  {COM_PROTECTED_ID_STATUS, COM_PROTECTED_ID_STATUS_LIT},
  {COM_UNKNOWN_ID_STATUS, COM_UNKNOWN_ID_STATUS_LIT}
};

defXLateFuncs(CmGetLitAsIdStatus, CmGetComIdStatusAsLit, ComIdStatus, IdStatusXlateArray);

// -----------------------------------------------------------------------
// Translate ANSI SQL names from Default ANSI SQL Name character set
// to UCS-2 encoding values.  The contents of the outWcs parameter is
// clear and set to the newly computed UCS2 string
// -----------------------------------------------------------------------
void CmAnsiNameToUCS2(const NAString &inMbs, NAWString &outWcs)
{
  outWcs.remove(0); // set to an empty string
  if (inMbs.length() <= 0)
  {
    return;
  }
  NAWString * pTargetNAWString =
    charToUnicode ( (Lng32)ComGetNameInterfaceCharSet() // in - Lng32        strCharSet
                  , inMbs.data()                        // in - const char * str
                  , (Int32)inMbs.length()               // in - Int32        len
                  , (NAMemory *)STMTHEAP                // in - NAMemory *   h
                  );
  ComASSERT(pTargetNAWString != NULL AND pTargetNAWString->length() > 0 AND
             pTargetNAWString->length() <= ComMAX_ANSI_IDENTIFIER_INTERNAL_LEN/*in NAWchars*/);
  outWcs.append(pTargetNAWString->data(), pTargetNAWString->length());
  delete pTargetNAWString;
}

// -----------------------------------------------------------------------
// Translate ANSI SQL names from UCS-2/UTF-16 encoding values to
// the Default ANSI SQL Name character set.
// -----------------------------------------------------------------------
void CmAnsiNameToUTF8(const NAWString &inWcs, NAString &outMbs)
{
  outMbs.remove(0); // set to an empty string
  if (inWcs.length() <= 0)
  {
    return;
  }

  NAString *pConvStr =
    unicodeToChar ( inWcs.data()                        // in - const char * str
                  , (Int32)inWcs.length()               // in - Int32        len
                  , (Lng32)ComGetNameInterfaceCharSet() // in - Lng32        strCharSet
                  , (NAMemory *)STMTHEAP                // in - NAMemory *   h
                  , FALSE                               // in - NABoolean allowInvalidChar
                  );
  if (pConvStr != NULL AND pConvStr->length() > 0)
  {
    outMbs = *pConvStr;
  }
  delete pConvStr;
}

void CatInternalError(char const*, int, char const*, int)
{
  abort();
}
