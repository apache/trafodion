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
//
 * File:         CatErrorCodes.h
 * Description:  Catalog manager error codes.
 *
 * Created:      02. Feb. 2004  (VO: Separated it from CatError.h)
 * Language:     C++
 *
**********************************************************************/
#ifndef CMP_DDL_CAT_ERROR_CODES_H
#define CMP_DDL_CAT_ERROR_CODES_H

enum CatErrorCode { CAT_FIRST_ERROR = 1000
                  , CAT_SYNTAX_ERROR = CAT_FIRST_ERROR            //1000
                  , CAT_INTERNAL_EXCEPTION_ERROR                  = 1001
                  , CAT_CATALOG_DOES_NOT_EXIST_ERROR              = 1002
                  , CAT_SCHEMA_DOES_NOT_EXIST_ERROR               = 1003
                  , CAT_TABLE_DOES_NOT_EXIST_ERROR                = 1004
                  , CAT_CONSTRAINT_DOES_NOT_EXIST_ERROR           = 1005
                  // unused                                       = 1006
                  , CAT_WGO_NOT_ALLOWED                           = 1007
                  , CAT_AUTHID_DOES_NOT_EXIST_ERROR               = 1008
                  , CAT_COLUMN_DOES_NOT_EXIST_ERROR               = 1009
                  , CAT_UNSUPPORTED_COMMAND_ERROR                 = 1010
                  , CAT_ONLY_ONE_GRANTEE_ALLOWED                  = 1011
                  , CAT_PRIVILEGE_NOT_GRANTED                     = 1012
                  , CAT_NOT_ALL_PRIVILEGES_GRANTED                = 1013
                  // unused                                       = 1014
                  , CAT_NOT_ALL_PRIVILEGES_REVOKED                = 1015
                  , CAT_REDUNDANT_COLUMN_REF_PK                   = 1016
                  , CAT_NOT_AUTHORIZED                            = 1017
                  , CAT_GRANT_NOT_FOUND                           = 1018
                  // unused                                       = 1019
                  , CAT_SMD_PRIVS_CANNOT_BE_CHANGED               = 1020
                  , CAT_INITIALIZE_SQL_ALREADY_DONE               = 1021
                  , CAT_SCHEMA_ALREADY_EXISTS                     = 1022
                  // unused                                       = 1023
                  // unused                                       = 1024
                  , CAT_DEPENDENT_OBJECTS_EXIST                   = 1025
                  // unused                                       = 1026
                  , CAT_COLUMN_PRIVILEGE_NOT_ALLOWED              = 1027
                  , CAT_SCHEMA_IS_NOT_EMPTY                       = 1028
                  , CAT_UNABLE_TO_CREATE_OBJECT                   = 1029
                  , CAT_HBASE_NAME_TOO_LONG                       = 1030
                  , CAT_UNABLE_TO_DROP_OBJECT                     = 1031
                  // unused                                       = 1033
                  , CAT_UNABLE_TO_RETRIEVE_PRIVS                  = 1034
                  , CAT_CATALOG_ALREADY_EXISTS                    = 1035
                  , CAT_CIRCULAR_PRIVS                            = 1036
                  , CAT_DEPENDENT_PRIV_EXISTS                     = 1037
                  // unused                                       = 1038
                  , CAT_PRIVILEGE_NOT_REVOKED                     = 1039
                  , CAT_SMD_CANNOT_BE_ALTERED                     = 1040
                  , CAT_PRIMARY_KEY_ALREADY_DEFINED               = 1041
                  , CAT_COLUMN_IN_CONSTRAINT_MUST_BE_NOT_NULL     = 1042
                  , CAT_CONSTRAINT_ALREADY_EXISTS                 = 1043
                  , CAT_REFERENCED_CONSTRAINT_DOES_NOT_EXIST      = 1044
                  , CAT_UNIQUE_CONSTRAINT_IS_DEFERRABLE           = 1045
                  , CAT_REFERENCING_AND_REFERENCED_COLUMNS_INCORRECT
                  , CAT_DEPENDENT_VIEW_EXISTS                     = 1047
                  , CAT_ONLY_SUPPORTING_RESTRICT_DROP_BEHAVIOR    = 1048
                  , CAT_CANNOT_DROP_NON_DROPPABLE_CONSTRAINT      = 1049
                  , CAT_DROP_FAILS_FOREIGN_KEY_EXISTS             = 1050
                  , CAT_INSUFFICIENT_PRIV_ON_OBJECT               = 1051
                  , CAT_CONSTRAINT_DOES_NOT_BELONG_TO_TABLE       = 1052
                  , CAT_UNIQUE_INDEX_LOAD_FAILED_WITH_DUPLICATE_ROWS = 1053
                  // unused                                       = 1054
                  , CAT_TABLE_ALREADY_EXISTS                      = 1055
                  // unused                                       = 1056
                  // unused                                       = 1057
                  , CAT_HISTOGRAM_TABLE_NOT_CREATED               = 1058
                  , CAT_DEPENDENT_CONSTRAINT_EXISTS               = 1059
                  // unused                                       = 1060
                  // unused                                       = 1061
                  , CAT_USER_CANNOT_DROP_SMD_SCHEMA               = 1062
                  // unused                                       = 1063
                  // unused                                       = 1064
                  // unused                                       = 1065
                  // unused                                       = 1066
                  // unused                                       = 1067
                  , CAT_AUTH_COMPLETED_WITH_WARNINGS              = 1068
                  , CAT_UNABLE_TO_DROP_SCHEMA                     = 1069
                  , CAT_CREATE_OBJECT_ERROR                       = 1070
                  , CAT_HIVE_VIEW_USAGE_UNAVAILABLE               = 1071
                  // unused                                       = 1072
                  , CAT_ATTEMPT_CLEANUP_SCHEMA                    = 1073
                  // unused                                       = 1074
                  // unused                                       = 1075
                  // unused                                       = 1076
                  // unused                                       = 1077
                  // unused                                       = 1078
                  // unused                                       = 1079
                  , CAT_DUPLICATE_COLUMNS                         = 1080
                  , CAT_CLI_LOAD_INDEX                            = 1081
                  , CAT_CLI_VALIDATE_CONSTRAINT                   = 1082
                  , CAT_CONSTRAINT_DATA_EXISTS                    = 1083
                  , CAT_ILLEGAL_DEFAULT_VALUE_FORMAT              = 1084
                  // unused                                       = 1085
                  // unused                                       = 1086
                  // unused                                       = 1087
                  // unused                                       = 1088
                  , CAT_SYSKEY_NOT_LAST_COLUMN                    = 1089
                  , CAT_SELF_REFERENCING_CONSTRAINT               = 1090
                  // unused                                       = 1091
                  // unused                                       = 1092
                  // unused                                       = 1093
                  // unused                                       = 1094
                  // unused                                       = 1095
                  // unused                                       = 1096
                  , CAT_PARTITION_KEY_NOT_EXIST                   = 1097
                  , CAT_PARTITION_KEY_DUPLICATED                  = 1098
                  , CAT_VIEW_COLUMN_UNNAMED                       = 1099
		  , CAT_LOB_COLUMN_ALTER                          = 1100
                  , CAT_SMDIO_UPDATE_ERROR                        = 1101
                  , CAT_SMDIO_INSERT_ERROR                        = 1102
                  , CAT_SMDIO_DELETE_ERROR                        = 1103
                  , CAT_DEFAULT_VALUE_TOO_LONG                    = 1104
                  , CAT_LIKE_CANNOT_CONTAIN_BOTH_STORE_BY_AND_HORIZONTAL_PARTITION
                                                                  = 1105
                  , CAT_PARTITION_KEY_OF_OBJ_NOT_EXIST            = 1106
                  , CAT_ERRORS_ENCOUNTERED_IN_PARSING_PHASE       = 1107
                  , CAT_NUM_OF_VIEW_COLS_NOT_MATCHED              = 1108
                  , CAT_WITH_CK_OPT_IN_NOT_UPDATABLE_VIEW         = 1109
                  , CAT_KLUDGE_TO_PRINT_TRACE_MESSAGE             = 1110
                  , CAT_ERROR_IN_STARTING_TRANSACTION             = 1111
                  , CAT_SYSKEY_ONLY_INDEXED_COLUMNS_LIST          = 1112
                  , CAT_UNABLE_TO_ALTER_VIEW_CASCADE              = 1113
                  , CAT_UNABLE_TO_CREATE_METADATA                 = 1114
                  , CAT_UNABLE_TO_CREATE_LABEL                    = 1115
                  , CAT_RANGE_PARTITION_KEY_ERROR                 = 1116
                  , CAT_CANNOT_DROP_LAST_PARTN                    = 1117
                  , CAT_CREATE_TABLE_NOT_ALLOWED_IN_SMD           = 1118
                  , CAT_USER_CANNOT_DROP_SMD_TABLE                = 1119
                  , CAT_INVALID_OPERATION                         = 1121
                  , CAT_TOO_MANY_KEY_VALUES                       = 1122
                  , CAT_INCORRECT_KEY_VALUES                      = 1123
                  , CAT_WRONG_METADATA_VERSION                    = 1124
                  , CAT_BAD_API_VERSION                           = 1125
                  , CAT_UNABLE_TO_UPGRADE_DOWNGRADE               = 1126
                  , CAT_NOT_A_BASE_TABLE                          = 1127
                  , CAT_WRONG_API_REQUEST                         = 1128
                  , CAT_OPERATION_NOT_ALLOWED_ON_VP_TABLE         = 1129
                  , CAT_DEFAULT_REQUIRED                          = 1130
                  , CAT_EXTERNAL_TABLE_EXISTS                     = 1131
                  , CAT_CANNOT_BE_DEFAULT_NULL_AND_NOT_NULL       = 1132
                  , CAT_ONLY_SUPER_CAN_DO_THIS                    = 1133
                  , CAT_LIST_OF_LOCKS_EXIST                       = 1134
                  , CAT_CLUSTERING_KEY_COL_MUST_BE_NOT_NULL_NOT_DROP
                  , CAT_ADDED_PK_CANNOT_BE_NOT_DROPPABLE          = 1136
                  , CAT_WRONG_INDEX_STATUS_REQUEST                = 1137
                  , CAT_WRONG_INPUT_PARAMETER                     = 1138
                  , CAT_SYSKEY_COL_NOT_ALLOWED_IN_CK_CNSTRNT      = 1139
                  , CAT_REC_LEN_TOO_LARGE                         = 1140
                  , CAT_UNABLE_TO_ACCESS_LABEL                    = 1141
                  , CAT_NO_CONSTRAINTS_FOR_NON_AUDITED            = 1142
                  , CAT_DATA_NOT_MEET_RI_CONSTRAINT_CRITERIA      = 1143
                  , CAT_MISSING_QUOTE_IN_CHAR_FIRSTKEY            = 1144
                  , CAT_RESERVED_METADATA_CATALOG_NAME            = 1145
                  , CAT_CANNOT_ALTER_WRONG_TYPE                   = 1146
                  , CAT_SYSTEM_COL_NOT_ALLOWED_IN_UNIQUE_CNSTRNT  = 1147
                  , CAT_SYSTEM_COL_NOT_ALLOWED_IN_RI_CNSTRNT      = 1148
                  , CAT_COLUMN_DOES_NOT_EXIST_IN_TABLE            = 1149
                  , CAT_COULD_NOT_GET_VOLUME_NAMES_FOR_POS        = 1150
                  , CAT_POS_WAS_NOT_APPLIED                       = 1151
                  , CAT_PARTITION_OFFLINE                         = 1152
                  , CAT_COLSZ_NOTSET_IN_KEY                       = 1153
                  , CAT_POS_TABLE_SIZE_TOO_BIG                    = 1154
                  , CAT_NOT_A_SYNONYM                             = 1155
                  , CAT_INCORRECT_OBJECT_TYPE                     = 1156
                  , CAT_ALTERING_TO_SAME_VALUE                    = 1157
                  , CAT_OBJECT_ALREADY_EXISTS                     = 1158
                  , CAT_SYNONYM_DOES_NOT_EXIST                    = 1159
                  , CAT_INVALID_STORE_BY                          = 1160
                  , CAT_SYS_COL_NOT_ALLOWED_IN_PARTITIONBY_CLAUSE = 1161
                  , CAT_ID_ALREADY_OWNS_OBJECT                    = 1162
                  , CAT_NOT_ALLOWED_TO_CHANGE_OWNER               = 1163
                  , CAT_SEND_CONTROLS_FAILED                      = 1164
                  , CAT_PARALLEL_OP_FAILED                        = 1165
                  , CAT_DROP_FAILED_WITH_CLEANUP                  = 1166
                  , CAT_OWNER_MUST_BE_SUPER_OR_SERVICES           = 1167
                  , CAT_INVALID_OBJECT_UID                        = 1168
                  , CAT_MISSING_SCHEMA_SECURITY                   = 1169
                  , CAT_POS_INVALID_NUM_DISK_POOLS                = 1170
                  , CAT_FETCH_DISK_SIZE_ERROR                     = 1171
                  , CAT_CANNOT_SPECIFY_DDL_AT_OBJECT_LEVEL        = 1172
                  , CAT_NO_VOLUMES_FOR_VOLATILE_TABLES            = 1173
                  , CAT_INVALID_COLUMN_DATATYPE                   = 1174
                  , CAT_NO_IDENTITY_COLUMN_FOR_TABLE              = 1175
                  , CAT_POS_MTS_SIZE_BIGGER_THAN_AMTS_SIZE        = 1176
                  , CAT_UNKNOWN_TABLE_TYPE                        = 1177
                  , CAT_INVALID_CATALOG_UID                       = 1178
                  , CAT_INVALID_OBJECT_INFO                       = 1179
                  //
                  // 1150-1180  Queuing and publish/subscribe errors
                  //
                  , CAT_EXTERNAL_NAME_MISMATCH                    = 1180
                  , CAT_EXTERNAL_SCHEMA_NAME_TOO_LONG             = 1181
                  , CAT_RFORK_SQL_ERROR                           = 1182
                  , CAT_METADATA_SQL_ERROR                        = 1183
                  , CAT_INSUFFICIENT_PRIV_ON_COLUMN               = 1184
                  , CAT_LOCATION_INVALID_OR_MISSING               = 1185
                  , CAT_INCOMPATIBLE_DATA_TYPE_IN_DEFAULT_CLAUSE  = 1186
                  , CAT_RESERVED_METADATA_SCHEMA_NAME             = 1187
                  , CAT_RI_CIRCULAR_DEPENDENCY                    = 1188
                  , CAT_VIEW_NAME_VALID                           = 1189

                  , CAT_DROP_LABEL_ERROR_FELABELBAD               = 1194
                  , CAT_INVALID_SYSTEM_NAME                       = 1196
                  , CAT_INVALID_SALTED_UNIQUE_IDX                 = 1201
                  , CAT_INVALID_SALT_LIKE_CLAUSE                  = 1202
                  , CAT_INVALID_HBASE_OPTIONS_CLAUSE              = 1203
                  , CAT_CODE_MUST_CONTAIN_2_NONBLANKS             = 1220
                  , CAT_COMPONENT_NOT_SYSTEM                      = 1221
                  , CAT_AUTHORIZATION_NOT_ENABLED                 = 1222
                  , CAT_CANT_GRANT_TO_SELF_OR_ROOT                = 1223
                  , CAT_INVALID_TYPE_FOR_PARAM                    = 1224
                  , CAT_MIXED_PRIVILEGES                          = 1225
                  , CAT_NO_PRIVILEGES_SPECIFIED                   = 1226
                  , CAT_NO_UNREG_USER_HAS_PRIVS                   = 1227
                  , CAT_ROLE_HAS_PRIVS_NO_DROP                    = 1228
                  , CAT_OPTION_NOT_SUPPORTED                      = 1229
                  , CAT_BY_CLAUSE_IN_PRIVATE_SCHEMA               = 1230
                  , CAT_UNABLE_TO_CREATE_ROUTINE                  = 1231
                  , CAT_UNABLE_TO_SAVE_DDL                        = 1232
                  , CAT_CREATE_SCHEMA_IN_SYSCAT_IS_PROHIBITED     = 1233
                  , CAT_LABEL_FAILED_DUE_TO_EXTENT_OR_MAXEXTENT   = 1235
                  , CAT_IMPROPER_SCHEMA_NAME                      = 1236
                  , CAT_NON_ISO88591_RANGE_PARTITION_COLUMN       = 1240
                  , CAT_INIT_AUTHORIZATION_FAILED                 = 1244
                  , CAT_FIRST_KEY_VALUE_INVALID                   = 1245
                  , CAT_FIRST_KEY_VALUE_INCONSISTENT              = 1246
                  , CAT_PARTITION_NAME_ALREADY_EXISTS             = 1248
                  , CAT_DDL_OPERATION_IN_PROGRESS                 = 1250
                  , CAT_SCALE_OF_DEFAULT_VALUE_ADJUSTED           = 1251
                  , CAT_INVALID_INDEX_DATA                        = 1252
                  , CAT_DUPLICATE_UNIQUE_CONSTRAINT_ON_SAME_COL   = 1254
                  , CAT_UNABLE_TO_SET_UDR_OPTIONS                 = 1261
                  , CAT_SCHEMA_IN_TRANSITION                      = 1262
                  , CAT_RESERVED_UMD_PREFIX                       = 1263
                  , CAT_DUPLICATE_PRIVILEGES                      = 1264
                  , CAT_DUPLICATE_GRANTEES                        = 1265
                  , CAT_ONLY_EXECUTE_PRIVILEGE_IS_ALLOWED_FOR_ROUTINE = 1266
                  , CAT_PRIVILEGE_NOT_ALLOWED_FOR_THIS_OBJECT_TYPE = 1267
                  , CAT_DUPLICATE_COLUMN_NAMES                    = 1268
                  , CAT_LABEL_ALLOCATE_FAILED_DUE_TO_FS_ERROR     = 1270
                  , CAT_LABEL_ALLOCATE_FAILED_GREATER_THAN_MAXEXTENTS = 1271
                  , CAT_NOT_LICENSED_FOR_SQLMX_DDL                = 1272
                  , CAT_MAXEXT_LESS_THAN_ALLOCATED_EXTENTS        = 1273
                  , CAT_WARNING_MAXEXT_RESET                      = 1274
                  , CAT_CANNOT_DROP_DUE_TO_UNIQUE_CONSTRAINT      = 1275
                  , CAT_UNRECOGNIZED_PARTITIONING_SCHEME          = 1277
                  , CAT_ALL_SCHEMAS_OP_IN_PROGRESS                = 1278
                  , CAT_VOLATILE_OPERATION_ON_REGULAR_OBJECT      = 1279
                  , CAT_REGULAR_OPERATION_ON_VOLATILE_OBJECT      = 1280
		  , CAT_LOB_COLUMN_IN_VOLATILE_TABLE              = 1282
                  , CAT_NOT_DROPPABLE_TABLE                       = 1284
                  , CAT_NOT_DROPPABLE_SCHEMA                      = 1285
                  , CAT_ONLY_SUPER_ID_CAN_INITIALIZE_SECURITY     = 1287
                  , CAT_UNABLE_TO_INITIALIZE_SECURITY             = 1288
                  , CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA   = 1289
                  , CAT_DELETE_FROM_TRANSLATION_TABLE_FAILED      = 1292
                  , CAT_WRONG_SCHEMA_VERSION                      = 1293
                  , CAT_WRONG_ISO_MAPPING                         = 1294
                  , CAT_COLUMN_WRONG_DEFAULT_TYPE                 = 1295
                  , CAT_COLUMN_MISMATCHED_DEFAULT_TYPES           = 1296
                  , CAT_VOLATILE_SCHEMA_PRESENT                   = 1297
                  , CAT_UNABLE_TO_ALTER_SCHEMA                    = 1298

                  // Restrict and No Action referential action Messages.
                  , CAT_REF_CONSTRAINT_NO_ACTION_NOT_SUPPORTED    = 1301
                  , CAT_REF_CONSTRAINT_NO_ACTION_LIKE_RESTRICT    = 1302

                  // Anchor file access error
                  , CAT_ANCHOR_FILE_ERROR                         = 1304

                  // Schema subvol error/warnings
                  , CAT_DUP_SCHEMA_SUBVOL_SPECIFIED               = 1305
                  , CAT_DUP_SCHEMA_SUBVOL_GENERATED               = 1306
                  , CAT_METADATA_SCHEMA_SUBVOL                    = 1307
                  , CAT_INVALID_OBJECT_TYPE                       = 1309
                  , CAT_TRIGGER_UNSUPPORTED_IN_COMPOUNDCREATE     = 1310

                  , CAT_UNABLE_TO_XXX_CONSTRAINT_DUE_TO_ERRORS    = 1311
                  , CAT_UNABLE_TO_XXX_DUE_TO_ERRORS               = 1312
                  , CAT_NOT_ENFORCED_RI_CONSTRAINT_WARNING        = 1313

                  // Alter catalog, disable creates
                  , CAT_CREATE_OPERATION_DISABLED                 = 1314
                  , CAT_NO_DEFINITION_SCHEMA                      = 1315

                  // Publish/unpublish errors
                  , CAT_PUBLISH_NOT_SYSTEM_VIEW                   = 1316
                  , CAT_PUBLISH_NO_DBA_USER_DEFINED               = 1317
                  , CAT_PUBLISH_SYNONYM_NAME_TOO_LONG             = 1318
                  , CAT_PUBLISH_MISMATCH_COLUMN_TABLE_PRIVS       = 1319
                  , CAT_PUBLISH_MISMATCH_COLUMN_COLUMN_PRIVS      = 1320
                  , CAT_PUBLISH_NO_PUBLIC_SCHEMA                  = 1321
                  , CAT_PUBLISH_VIEW_NOT_REFERENCING_OBJECT       = 1322
                  , CAT_UNABLE_TO_GRANT_PRIVILEGES                = 1323
                  , CAT_NO_SCHEMA_WGO_ALLOWED                     = 1325

                  , CAT_REGULAR_OPERATION_ON_INMEMORY_OBJECT      = 1326
                  , CAT_INMEMORY_OPERATION_ON_REGULAR_OBJECT      = 1327
                  , CAT_INVALID_PRIV_FOR_OBJECT                   = 1328
                  , CAT_UNABLE_TO_CREATE_METADATA_VIEWS           = 1329
                  , CAT_ROLE_IS_GRANTED_NO_REVOKE                 = 1330
                  , CAT_LDAP_USER_NOT_FOUND                       = 1331
                  , CAT_LDAP_COMM_ERROR                           = 1332
                  , CAT_USER_NOT_EXIST                            = 1333
                  , CAT_AUTHID_ALREADY_EXISTS                     = 1334
                  , CAT_LDAP_USER_ALREADY_EXISTS                  = 1335
                  , CAT_DB_ROOT_MISSING                           = 1336
                  , CAT_AUTH_NAME_RESERVED                        = 1337
                  , CAT_ROLE_NOT_EXIST                            = 1338
                  , CAT_IS_NOT_A_ROLE                             = 1339
                  , CAT_IS_NOT_A_USER                             = 1340
                  , CAT_NO_UNREG_USER_OWNS_CATALOG                = 1341
                  , CAT_NO_UNREG_USER_OWNS_SCHEMA                 = 1342
                  , CAT_NO_UNREG_USER_OWNS_OBJECT                 = 1343
                  , CAT_NO_UNREG_USER_HAS_SCHEMA_PRIVS            = 1344
                  , CAT_NO_UNREG_USER_HAS_TABLE_PRIVS             = 1345
                  , CAT_NO_UNREG_USER_HAS_COLUMN_PRIVS            = 1346
                  , CAT_NO_UNREG_USER_OWNS_ROLES                  = 1347
                  , CAT_ROLE_IS_GRANTED_NO_DROP                   = 1348
                  , CAT_NO_UNREG_USER_GRANTED_ROLES               = 1349
                  , CAT_ROLE_NOT_GRANTED_TO_USER                  = 1350
                  , CAT_DUPLICATE_ROLES_IN_LIST                   = 1351
                  , CAT_DUPLICATE_USERS_IN_LIST                   = 1352
                  , CAT_NO_ROLE_WGO_ALLOWED                       = 1353
                  , CAT_NO_ROLE_SCHEMA_GRANT_ALLOWED              = 1354
                  , CAT_NO_GRANT_ROLE_TO_PUBLIC_OR_SYSTEM         = 1355
                  , CAT_COMPONENT_PRIVILEGE_CODE_EXISTS           = 1356
                  , CAT_COMPONENT_PRIVILEGE_NAME_EXISTS           = 1357
                  , CAT_COMPONENT_PRIVILEGE_NOT_FOUND             = 1358
                  , CAT_INVALID_PRIVILEGE_FOR_GRANT_OR_REVOKE     = 1359
                  , CAT_DEPENDENT_COMPONENT_PRIVILEGES_EXIST      = 1360
                  , CAT_LIBRARY_DOES_NOT_EXIST                    = 1361
                  , CAT_NOT_A_LIBRARY                             = 1362
                  , CAT_LIBRARY_EXISTS                            = 1363
                  , CAT_DEPENDENT_ROLE_PRIVILEGES_EXIST           = 1364
                  , CAT_COULDNT_LOCK_PARTICIPATING_AUTH_ID        = 1365
                  , CAT_DEPENDENT_ROUTINES_EXIST                  = 1366
                  , CAT_ROUTINE_USES_LIBRARY                      = 1367
                  , CAT_LIBRARY_MUST_BE_IN_SAME_CATALOG           = 1368
                  , CAT_ONLY_UPDATE_OR_USAGE_PRIV_FOR_LIBRARY     = 1369
                  , CAT_INVALID_CHARS_IN_AUTH_NAME                = 1370
                  , CAT_ONLY_SELECT_OR_INSERT_FOR_TABLE           = 1371
                  , CAT_ALTER_TABLE_INSERT_ONLY_FAILED            = 1372
                  , CAT_AUDIT_NOT_A_VALID_BOOL_VALUE              = 1373
                  , CAT_AUDIT_NOT_A_VALID_LOG_TYPE                = 1374
                  , CAT_AUDIT_INPUT_TOO_LARGE                     = 1375
                  , CAT_AUDIT_INVALID_COLUMN_NUMBER               = 1376
                  , CAT_AUDIT_ALTER_CONFIG_FAILED                 = 1377
                  , CAT_AUDIT_REFRESH_RANGE                       = 1378
                  , CAT_AUDIT_AGING_RANGE                         = 1379
                  , CAT_AUDIT_THRESHOLD_RANGE                     = 1380
                  , CAT_AUDIT_COLUMN_VALUE_COUNT_MISMATCH         = 1381
                  , CAT_JAR_NOT_FOUND                             = 1382
                  , CAT_DISABLE_AUTHNAME_CHANGES                  = 1383
                  , CAT_POS_UNEQUABLE_DISK_POOL_DEFINED           = 1384
                  , CAT_POS_DISK_POOL_MAPPING_FAILED              = 1385
                  , CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION        = 1389
                  , CAT_TRAFODION_OBJECT_EXISTS                   = 1390

                  , CAT_SHOWDDL_UNABLE_TO_CONVERT_COLUMN_DEFAULT_VALUE  = 1400
                  , CAT_UNABLE_TO_CONVERT_COLUMN_DEFAULT_VALUE_TO_CHARSET = 1401
                  , CAT_UNIQUE_INDEX_COLS_NOT_IN_DIV_EXPRS        = 1402
                  , CAT_ALTER_NOT_ALLOWED_IN_SMD                  = 1403

                  // Detectable metadata inconsistencies
                  , CAT_CATSYS_CATREF_MISMATCH                    = 1500
                  , CAT_OBJECTS_REPLICAS_MISMATCH                 = 1502
                  , CAT_OBJECTS_PARTITIONS_MISMATCH               = 1503
                  , CAT_NO_OBJECTS_ROW_FOR_SMD                    = 1504
                  , CAT_CATSYS_MISMATCH_ON_AUTOREF_NODE           = 1505
                  , CAT_SCHEMA_MISMATCH_ON_AUTOREF_NODE           = 1506

                  // IDENTITY Column (Surrogate Key) related Errors.
                  , CAT_IDENTITY_COLUMN_DATATYPE_MISMATCH         = 1510
                  , CAT_IDENTITY_COLUMN_ONE_PER_TABLE             = 1511
                  , CAT_IDENTITY_COLUMN_NOT_NULL_NOT_DROPPABLE    = 1512
                  , CAT_IDENTITY_COLUMN_HASH_PARTITIONED_ONLY     = 1513
                  , CAT_IDENTITY_COLUMN_NO_ALTER_TABLE            = 1514
                  , CAT_IDENTITY_COLUMN_2                         = 1515

                  , CAT_CANNOT_DISABLE_NOT_DROPPABLE_CONSTRAINT   = 1517
                  , CAT_CANNOT_ENABLE_CONSTRAINT                  = 1518
                  , CAT_CANNOT_DISABLE_FK_CONSTRAINT              = 1519
                  , CAT_SCHEMA_MISMATCH_ON_EXCEPTION_TABLE        = 1521
                  , CAT_NOT_AN_EXCEPTION_TABLE                    = 1522
                  , CAT_EXCEPTION_TABLE_DOES_NOT_EXIST            = 1523
                  , CAT_NO_EXCEPTIONS_ON_METADATA                 = 1524
                  , CAT_NO_EXCEPTIONS_ON_EXCEPTIONS               = 1525
                  , CAT_EXCEPTION_NOT_IN_METADATA_TABLE           = 1526
                  , CAT_NO_KEY_ON_BASE_TABLE                      = 1527
                  , CAT_NO_EXCEPTION_TABLE_FOR_BASE_TABLE         = 1528

                  , CAT_SG_NOT_IN_METADATA                        = 1529
                  , CAT_LDAP_DEFAULTCONFIG_INSERT_ERROR           = 1530

                  , CAT_NO_POPULATE_VOLATILE_INDEX                = 1540
                  , CAT_LOB_COL_CANNOT_BE_INDEX_OR_KEY            = 1541

                  // Sequence Generator errors
                  , CAT_SG_MAXVALUE_MINVALUE_ERROR                = 1570
                  , CAT_SG_INCREMENT_BY_ZERO_ERROR                = 1571
                  , CAT_SG_NEGATIVE_ERROR                         = 1572
                  , CAT_SG_STARTWITH_MINVALUE_MAXVALUE_ERROR      = 1573
                  , CAT_SG_CYCLE_NOT_SUPPORTED_ERROR              = 1574
                  , CAT_SG_INCREMENT_BY_MAXVALUE_ERROR            = 1575
                  , CAT_SG_MAXIMUM_DATATYPE_ERROR                 = 1576

                  // Collation for Catlog & Schema
                  , CAT_CATALOG_COLLATION_NOT_SUPPORTED           = 1580
                  , CAT_SCHEMA_COLLATION_NOT_SUPPORTED            = 1581

                  // Sequence Generator alter table errors
                  , CAT_SG_ALTER_NOT_IDENTITY_COLUMN_ERROR        = 1590
                  , CAT_SG_ALTER_MAXVALUE_NOT_GT_ERROR            = 1591
                  , CAT_SG_ALTER_UNSUPPORTED_OPTION_ERROR         = 1592
                  , CAT_SG_ALTER_TOO_MANY_OPTIONS_ERROR           = 1593
                  , CAT_SG_ALTER_CURRENT_VALUE_ERROR              = 1594
                  , CAT_SG_ALTER_NO_MAXVALUE_ERROR                = 1595
                  , CAT_SG_ALTER_RECALIBRATION_ERROR              = 1596
                  , CAT_SG_ALTER_RECALIBRATION_MAXIMUM_ERROR      = 1597
                  , CAT_SG_ALTER_RECALIBRATION_CURRENT_ERROR      = 1598
                  , CAT_SG_ALTER_RECALIBRATION_SPECIFIED_ERROR    = 1599
                  , CAT_SG_ALTER_RECALIBRATION_LOCKING_ERROR      = 1600
                  , CAT_SG_ALTER_RECALIBRATION_NO_SELECT_ERROR    = 1601

                  // UDF related errors
                  , CAT_NOT_UUDF_OBJECT                           = 1700
                  , CAT_TOO_MANY_PASS_THRU_INPUTS                 = 1701
                  , CAT_ONLY_STRING_LITERAL                       = 1702
                  , CAT_BINARY_ONLY_OPTION_WITH_UCS2              = 1703
                  , CAT_BINARY_ONLY_OPTION_WITHIN_VALUE_FROM_FILE_CLAUSE = 1704
                  , CAT_UNABLE_TO_OPEN_FILE                       = 1705
                  , CAT_UNABLE_TO_READ_FILE                       = 1706
                  , CAT_RA_ALREADY_EXISTS_UUDF                    = 1707
                  , CAT_EXCEEDS_NUMBER_OF_OUTPUT_VALUES           = 1708
                  , CAT_UNABLE_TO_DROP_UUDF_BEING_USED_BY_RA      = 1709
                  , CAT_PASS_THRU_INPUT_WRONG_POSITION_SPECIFIED  = 1710
                  , CAT_POSITION_SPECIFIED_EXCEEDS_NUMBER_OF_PASS_THRU_INPUTS = 1711
                  , CAT_SPECIFIED_POSITION_APPEARS_MULTIPLE_TIMES = 1712
                  , CAT_MISSING_UUDF_FUNCTION_NAME_CLAUSE         = 1713
                  , CAT_INVALID_ROUTINE_ACTION_NAME               = 1714
                  , CAT_UNABLE_TO_START_TRANSACTION               = 1715
                  , CAT_SQL_STYLE_PARAMETER_EXCEEDS_LIMIT         = 1716
                  , CAT_PASS_THRU_BINARY_INPUT_CANNOT_BE_EMPTY    = 1717
                  , CAT_BINARY_TYPE_FILE_EMPTY                    = 1718

                  // Method validation failures
                  , CAT_CLASS_NOT_FOUND                           = 11205
                  , CAT_CLASS_DEFINITION_NOT_FOUND                = 11206
                  , CAT_MULTIPLE_COMPATIBLE_METHODS               = 11230
                  , CAT_METHOD_NOT_PUBLIC                         = 11231
                  , CAT_METHOD_NOT_STATIC                         = 11232
                  , CAT_METHOD_NOT_VOID                           = 11233
                  , CAT_METHOD_NOT_FOUND                          = 11234
                  , CAT_NO_COMPATIBLE_METHODS                     = 11239

                  , CAT_ID_TOO_LONG                               = 3118
                  , CAT_DUPLICATE_CLAUSES                         = 3183
                  , CAT_STORE_BY_PK_NO_PK                         = 3188
                  , CAT_LONG_VARCHAR_BELOW_MIN                    = 3215
                  , CAT_LONG_WVARCHAR_BELOW_MIN                   = 3216
                  , CAT_POS_INVALID_DISK_POOL                     = 3417

                  // Binder like error
                  , CAT_DUPLICATE_COLUMN                          = 4022


                  //
                  // Trigger-related Catalog Manager errors
                  //
                  , CAT_TRIGGER_TABLE_NAME_TOO_LONG               = 11004
                  , CAT_TRIGGER_DOES_NOT_EXIST_ERROR              = 11030
                  , CAT_TRIGGER_NOT_IN_METADATA_TABLE             = 11031
                  , CAT_TRIGGER_COLUMNS_FOR_NON_UPDATE            = 11032
                  , CAT_UNABLE_TO_DROP_OBJECT_BEING_USED_BY_TRIGGERS = 11033
                  , CAT_TRIGGERS_TEMPORARY_TABLE_EXISTED          = 11034
                  , CAT_NO_TRIGGERS_ON_METADATA                   = 11035
                  , CAT_NO_TRIGGERS_DEFINED_ON_TABLE              = 11036
                  , CAT_NO_TRIGGER_ON_VIEWS                       = 11037
                  , CAT_TRIGGER_ONLY_ON_TABLES                    = 11038
                  , CAT_NO_TRIGGER_ON_VERTICAL_PARTITIONS         = 11039
                  , CAT_NO_TRIGGER_PK_TOO_LONG                    = 11040
                  , CAT_NO_TEMP_BAD_DEFAULT_PARTITIONS            = 11041
                  , CAT_TRIGGER_ALREADY_EXISTS                    = 11042
                  , CAT_NO_TRIGGERS_ON_COMPUTED_COLS              = 11052

          //----------------------------------------------------------
          //++ MV
          , CAT_MV_NON_INCREMENTAL_REFRESH                = 12001
          , CAT_MV_HAVE_TRIGGERS_NO_LOGS_ON_SAME_TABLE    = 12003
          , CAT_MV_HAS_LOG_NO_TRIGGERS_ON_SAME_TABLE      = 12004
          , CAT_MV_NOT_IN_METADATA_TABLE                  = 12005
          , CAT_MV_NO_SELECT_COLUMNS_FOR_INCREMENTAL      = 12006
          , CAT_MV_VERTICAL_PARTITION_NOT_ALLOWED         = 12007
          , CAT_MV_CREATE_TABLE_FAILED                    = 12008
          , CAT_UNABLE_TO_DROP_OBJECT_BEING_USED_BY_MVS   = 12009
          , CAT_MV_USED_TABLE_MUST_BE_AUDITED             = 12010
          , CAT_MV_CANNOT_ALTER_IGNORECHANGES_ON_STATEMENT_RECOMPUTE_MV = 12011
          , CAT_MV_ALTER_TABLE_FAILED                     = 12012
          , CAT_MV_INITIALIZATION_FAILED                  = 12013
          , CAT_MV_LOCKING_OF_USED_OBJECT_FAILED          = 12014
          , CAT_LOG_CREATION_FAILED                       = 12015
          , CAT_NO_UNIQUE_INDEX_ON_MV                     = 12016
          , CAT_NO_MV_INCR_USING_NON_INCR                 = 12017
          , CAT_DUPLICATED_TABLE_IN_CHANGES_CLAUSE        = 12020
          , CAT_CHANGES_CONTAINS_ONLY_FROM_CLAUSE_BASE_TABLES = 12021
          , CAT_MV_CANNOT_BE_BUILD_ON_TOP_OF_THESE_MVS    = 12022
          , CAT_INITIALIZED_MV_CONFIGURATION_ERROR        = 12023
          , CAT_INITIALIZED_MV_ALLOWD_ONLY_ON_INIT_MVS    = 12024
          , CAT_CHANGES_TABLES_NOT_IN_FROM_CLAUSE         = 12025
          , CAT_ALL_FROM_CLAUSE_IS_IGNORE_CHANGES         = 12026
          , CAT_MVS_NOT_ALLOWED_ON_META_DATA              = 12027
          , CAT_GRANT_UPDATE_NOT_ALLOWED_FOR_MV           = 12028
          , CAT_MV_ATTRIBUTE_SPECIFIED_FOR_BASE_TABLE     = 12029
          , CAT_IGNORE_CHANGES_ONLY_FOR_ON_REQUEST_MVS    = 12032
          , CAT_INCOMPATIBLE_MV_REFRESH_TYPE              = 12033
          , CAT_MV_NOT_ALLOWED_ON_SOME_OF_USED_OBJECTS    = 12034
          , CAT_ALL_USED_OBJECTS_MUST_BE_AUDITED          = 12035
          , CAT_ON_STATMENT_MUST_BE_AUDITED               = 12036
          , CAT_MINMAX_ON_STATEMENT_NOT_ONLY_ON_INSERTLOG_BT = 12037
          , CAT_GRANT_REFERENCES_NOT_ALLOWED_FOR_MV       = 12038
          , CAT_UNDERLYING_TABLE_RECORD_LENGTH_TOO_LARGE  = 12067
          , CAT_TABLE_ALTER_FAILED_RECORD_LENGTH_TOO_LARGE  = 12068
          , CAT_UNDERLYING_TABLES_CANNOT_BE_VOLATILE      = 12069

          , CAT_INITIALIZED_ON_CREATE_USING_NO_LOCKONREFRESH      = 12076
          , CAT_USED_OBJECTS_MUST_BE_LOCKONREFRESH_BEFORE_INIT_MV = 12077
          , CAT_MV_ENTRY_SEQ_CLUSTERING_NOT_ALLOWED               = 12078
          , CAT_NON_SELF_MAINTAIN_MV_WITH_USED_NOLOCKONREFRESH    = 12079
          , CAT_AUTO_MIX_RANGELOG_AND_NO_LOCKONREFRESH_CANNOT_COEXIST = 12080
          , CAT_COMMIT_REFRESH_ONLY_FOR_AUDIT_ON_REQUEST_SM       = 12082
          , CAT_BASED_ON_HASH_PARTITIONED_NON_MANUAL_RANGELOG_BT  = 12083
          , CAT_MV_CANNOT_BE_INITIALIZED_ON_CREATION              = 12084
          , CAT_MV_CANNOT_BE_NO_INITIALIZATION                    = 12085
          , CAT_LOG_ALTER_FAILED                                  = 12086
          , CAT_IUD_LOG_NOT_EMPTY                                 = 12087
          , CAT_MV_DEPENDANT_ON_MVS_ALLOWED_ATTRIBUTE             = 12088
          , CAT_CLUSTERING_INDEX_COLUMNS_CANNOT_BE_NOT_LOGGABLE   = 12089
          , CAT_UNABLE_TO_ADD_COLUMN_TO_IUD_LOG                   = 12090
          , MV_DEFINED_ON_NOT_LOGGABLE_COLUMN                     = 12091
          , CAT_GROUP_BY_EXCEED_MAX_KEY_LENGTH                    = 12092
          , CAT_CANNOT_CREATE_MV_WITH_NO_LOCKONREFRESH            = 12093
          , CAT_CANT_CHANGE_AUDIT_ATTR_FOR_ON_REQUEST_MJV_ON_MV   = 12095
          , CAT_MV_LOGS_EXISTED                                   = 12096
          , CAT_MULTI_TRANSACTION_CONTEXT_TABLE_CREATION_FAILED   = 12097
          , CAT_CANNOT_DROP_MULTI_TRANSACTION_CONTEXT_TABLE       = 12098
          , CAT_COMMIT_REFRESH_EACH_WAS_SET_TO_ZERO               = 12099
          , CAT_IGNORE_CHANGES_OBJECT_IS_NO_LOCKONREFRESH         = 12100
          , CAT_UNDERLYING_TABLES_CANNOT_BE_VERTICAL_PARTITIONED  = 12101
          , CAT_MANUAL_MIX_RANGE_NOT_ALLOWED_FOR_MULTI_DELTA_MAJV = 12102
          , CAT_RECOMPUTE_MV_CANNOT_BE_NO_INITIALIZATION          = 12103
          , CAT_PART_KEY_IS_NOT_PREFIX_OF_CI_NO_RANGE_LOG_ALLOWED = 12105
          , CAT_INDEX_WAS_CREATED_NOT_POPULATED                   = 12106
          , CAT_MAV_STORE_BY_COLS_MUST_BE_IN_GROUP_BY_COLS        = 12107
          , CAT_NO_STORE_BY_CLAUSE                                = 12108
          , CAT_THE_STORE_BY_SHOULD_INCLUDE_THE_FOLLOWING_COLS    = 12109
          , CAT_MV_WAS_DROPPED                                    = 12110
          , CAT_NO_AUDIT_MV_NOT_SUPPORTED                         = 12111
          , CAT_SECONDARY_INDEX_WAS_CREATED_AUTOMATICALLY         = 12112
          , CAT_ON_STATEMENT_MAV_ARE_CURRENTLY_NOT_SUPPORTED      = 12113
          , CAT_THERE_ARE_ON_STATEMENT_MVS_ON_THE_OBJECT          = 12114
          , CAT_MV_CANNOT_ALTER_IGNORE_CHANGE_CLAUSE              = 12115
          , CAT_MV_WITH_COMMIT_EACH_CANNOT_ALTER_IC_CLAUSE        = 12116
          , CAT_CLI_MV_REWRITE_PUBLISH_TABLE                      = 12117
          , CAT_MJV_CLUSTERING_ON_NON_BASE_COLUMN                 = 12119
          , CAT_MVQR_NOT_SUPPORTED_FOR_MV_QUERY = 12120

          // MVGROUPS
          , CAT_MV_REFRESH_GROUP_DOES_NOT_EXIST_ERROR             = 12201
          , CAT_MV_DOES_NOT_EXIST                                 = 12202
          , CAT_MV_ALREADY_EXIST_IN_GROUP                         = 12203
          , CAT_MV_OR_GROUP_DOES_NOT_EXIST_ERROR                  = 12204
          , CAT_MV_NOT_IN_GROUP                                   = 12206
          , CAT_MV_GROUPS_INCONSISTENCY                           = 12207
          , CAT_MV_GROUP_CANNOT_CONTAIN_ON_STATEMENT_MVS          = 12208
          , CAT_SOME_MVS_USED_BY_THIS_MV_NEED_TO_BE_IN_GROUP_AS_WELL = 12209
          , CAT_DEPENDING_MVS_ARE_IN_GROUP                        = 12210
          , CAT_MVGROUP_ALREADY_EXISTS                            = 12211
          , CAT_MV_AND_DEPENDENTS_WERE_REMOVED_FROM_GROUP         = 12212
          , CAT_MV_WAS_NOT_ADDED_TO_GROUP                         = 12213
          , CAT_MV_GROUP_CANNOT_CONTAIN_UMV_MVS                   = 12214

          // Reasons MVs are not incremental
          // see also CAT_MV_NON_INCREMENTAL_REFRESH above for default reason
          , CAT_MVNI_ON_VIEW                        = 12320
          , CAT_MVNI_NO_COLUMNS                     = 12321
          , CAT_MVNI_TABLE_REUSED                   = 12322
          , CAT_MVNI_NON_REPEATABLE_EXPRESSION      = 12323
          , CAT_MVNI_MULTIPLE_GROUP_BY_CLAUSES      = 12324
          , CAT_MVNI_HAVING_CLAUSE                  = 12325
          , CAT_MVNI_JOIN_GRAPH_NOT_CONNECTED       = 12326
          , CAT_MVNI_COLUMN_NOT_IN_GROUPING_COLUMNS = 12327
          , CAT_MVNI_NON_EQUAL_JOIN_PREDICATE       = 12328
          , CAT_MVNI_MULTIPLE_COLUMN_JOIN_PREDICATE = 12329
          , CAT_MVNI_DISTINCT                       = 12331
          , CAT_MVNI_AGGREGATE_NOT_TOP_LEVEL        = 12334
          // End reasons MVs are not incremental

          , CAT_NO_ALTER_LOGGABLE_WHEN_AUTO_MAINTENANCE_ON = 12330

          //-- MV
          //----------------------------------------------------------

          , CAT_LAST_ERROR                                        = 1999
                  };

#endif // CAT_ERROR_CODES_H
