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
                  , CAT_WARN_USED_AUTHID                          = 1006
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
                  , CAT_REG_UNREG_OBJECTS                         = 1026
                  , CAT_COLUMN_PRIVILEGE_NOT_ALLOWED              = 1027
                  , CAT_SCHEMA_IS_NOT_EMPTY                       = 1028
                  , CAT_UNABLE_TO_CREATE_OBJECT                   = 1029
                  , CAT_HBASE_NAME_TOO_LONG                       = 1030
                  , CAT_UNABLE_TO_DROP_OBJECT                     = 1031
                  , CAT_UNABLE_TO_RETRIEVE_COMMENTS               = 1033
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
                  , CAT_IS_NOT_CORRECT_AUTHID                     = 1086
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
                  // unused                                       = 1101
                  // unused                                       = 1102
                  // unused                                       = 1103
                  // unused                                       = 1104
                  // unused                                       = 1105
                  // unused                                       = 1106
                  // unused                                       = 1107
                  , CAT_NUM_OF_VIEW_COLS_NOT_MATCHED              = 1108
                  , CAT_WITH_CK_OPT_IN_NOT_UPDATABLE_VIEW         = 1109
                  , CAT_KLUDGE_TO_PRINT_TRACE_MESSAGE             = 1110
                  // unused                                       = 1111
                  , CAT_SYSKEY_ONLY_INDEXED_COLUMNS_LIST          = 1112
                  // unused                                       = 1113
                  , CAT_UNABLE_TO_CREATE_METADATA                 = 1114
                  , CAT_UNABLE_TO_CREATE_LABEL                    = 1115
                  , CAT_RANGE_PARTITION_KEY_ERROR                 = 1116
                  , CAT_CANNOT_DROP_LAST_PARTN                    = 1117
                  , CAT_CREATE_TABLE_NOT_ALLOWED_IN_SMD           = 1118
                  , CAT_USER_CANNOT_DROP_SMD_TABLE                = 1119
                  , CAT_INVALID_OPERATION                         = 1121
                  , CAT_TOO_MANY_KEY_VALUES                       = 1122
                  , CAT_INCORRECT_KEY_VALUES                      = 1123
                  // unused                                       = 1124
                  // unused                                       = 1125
                  // unused                                       = 1126
                  , CAT_NOT_A_BASE_TABLE                          = 1127
                  // unused                                       = 1128
                  // unused                                       = 1129
                  , CAT_DEFAULT_REQUIRED                          = 1130
                  // unused                                       = 1131
                  , CAT_CANNOT_BE_DEFAULT_NULL_AND_NOT_NULL       = 1132
                  , CAT_ONLY_SUPER_CAN_DO_THIS                    = 1133
                  // unused                                       = 1134
                  , CAT_CLUSTERING_KEY_COL_MUST_BE_NOT_NULL_NOT_DROP = 1135
                  , CAT_ADDED_PK_CANNOT_BE_NOT_DROPPABLE          = 1136
                  // unused                                       = 1137
                  // unused                                       = 1138
                  , CAT_SYSKEY_COL_NOT_ALLOWED_IN_CK_CNSTRNT      = 1139
                  , CAT_REC_LEN_TOO_LARGE                         = 1140
                  , CAT_ROWKEY_LEN_TOO_LARGE                      = 1141
                  // unused                                       = 1142
                  , CAT_DATA_NOT_MEET_RI_CONSTRAINT_CRITERIA      = 1143
                  , CAT_MISSING_QUOTE_IN_CHAR_FIRSTKEY            = 1144
                  // unused                                       = 1145
                  , CAT_CANNOT_ALTER_WRONG_TYPE                   = 1146
                  , CAT_SYSTEM_COL_NOT_ALLOWED_IN_UNIQUE_CNSTRNT  = 1147
                  , CAT_SYSTEM_COL_NOT_ALLOWED_IN_RI_CNSTRNT      = 1148
                  // unused                                       = 1149
                  // unused                                       = 1150
                  // unused                                       = 1151
                  // unused                                       = 1152
                  // unused                                       = 1153
                  // unused                                       = 1154
                  , CAT_NOT_A_SYNONYM                             = 1155
                  , CAT_INCORRECT_OBJECT_TYPE                     = 1156
                  , CAT_ALTERING_TO_SAME_VALUE                    = 1157
                  // unused                                       = 1158
                  // unused                                       = 1159
                  // unused                                       = 1160
                  // unused                                       = 1161
                  // unused                                       = 1162
                  // unused                                       = 1163
                  // unused                                       = 1164
                  // unused                                       = 1165
                  // unused                                       = 1166
                  // unused                                       = 1167
                  // unused                                       = 1168
                  // unused                                       = 1169
                  // unused                                       = 1170
                  // unused                                       = 1171
                  // unused                                       = 1172
                  // unused                                       = 1173
                  , CAT_INVALID_COLUMN_DATATYPE                   = 1174
                  // unused                                       = 1175
                  // unused                                       = 1176
                  // unused                                       = 1177
                  // unused                                       = 1178
                  // unused                                       = 1179
                  , CAT_EXTERNAL_NAME_MISMATCH                    = 1180
                  , CAT_EXTERNAL_SCHEMA_NAME_TOO_LONG             = 1181
                  // unused                                       = 1182
                  // unused                                       = 1183
                  // unused                                       = 1184
                  // unused                                       = 1185
                  , CAT_INCOMPATIBLE_DATA_TYPE_IN_DEFAULT_CLAUSE  = 1186
                  , CAT_RESERVED_METADATA_SCHEMA_NAME             = 1187
                  , CAT_RI_CIRCULAR_DEPENDENCY                    = 1188
                  // unused                                       = 1189

                  , CAT_INVALID_COMPONENT_PRIVILEGE               = 1194
                  , CAT_INVALID_NUM_OF_SALT_PARTNS                = 1196
                  , CAT_INVALID_SALTED_UNIQUE_IDX                 = 1201
                  , CAT_INVALID_SALT_LIKE_CLAUSE                  = 1202
                  , CAT_INVALID_HBASE_OPTIONS_CLAUSE              = 1203
                  , CAT_CODE_MUST_CONTAIN_2_NONBLANKS             = 1220
                  , CAT_COMPONENT_NOT_SYSTEM                      = 1221
                  , CAT_AUTHORIZATION_NOT_ENABLED                 = 1222
                  , CAT_CANT_GRANT_TO_SELF_OR_ROOT                = 1223
                  // unused                                       = 1224
                  // unused                                       = 1225
                  // unused                                       = 1226
                  , CAT_NO_UNREG_USER_HAS_PRIVS                   = 1227
                  , CAT_ROLE_HAS_PRIVS_NO_DROP                    = 1228
                  , CAT_OPTION_NOT_SUPPORTED                      = 1229
                  , CAT_BY_CLAUSE_IN_PRIVATE_SCHEMA               = 1230
                  , CAT_UNABLE_TO_CREATE_ROUTINE                  = 1231
                  // unused                                       = 1232
                  // unused                                       = 1233
                  // unused                                       = 1235
                  // unused                                       = 1236
                  , CAT_NON_ISO88591_RANGE_PARTITION_COLUMN       = 1240
                  // unused                                       = 1244
                  // unused                                       = 1245
                  // unused                                       = 1246
                  // unused                                       = 1248
                  // unused                                       = 1249
                  // unused                                       = 1250
                  , CAT_SCALE_OF_DEFAULT_VALUE_ADJUSTED           = 1251
                  // unused                                       = 1252
                  , CAT_DUPLICATE_UNIQUE_CONSTRAINT_ON_SAME_COL   = 1254
                  // unused                                       = 1261
                  // unused                                       = 1262
                  // unused                                       = 1263
                  , CAT_DUPLICATE_PRIVILEGES                      = 1264
                  // unused                                       = 1265
                  , CAT_ONLY_EXECUTE_PRIVILEGE_IS_ALLOWED_FOR_ROUTINE = 1266
                  , CAT_PRIVILEGE_NOT_ALLOWED_FOR_THIS_OBJECT_TYPE = 1267
                  , CAT_DUPLICATE_COLUMN_NAMES                    = 1268
                  , CAT_RESERVED_COLUMN_NAME                      = 1269
                  // unused                                       = 1270
                  // unused                                       = 1271
                  // unused                                       = 1272
                  // unused                                       = 1273
                  // unused                                       = 1274
                  // unused                                       = 1275
                  // unused                                       = 1277
                  // unused                                       = 1278
                  , CAT_VOLATILE_OPERATION_ON_REGULAR_OBJECT      = 1279
                  , CAT_REGULAR_OPERATION_ON_VOLATILE_OBJECT      = 1280
		  , CAT_LOB_COLUMN_IN_VOLATILE_TABLE              = 1282
                  // unused                                       = 1284
                  // unused                                       = 1285
                  // unused                                       = 1287
                  // unused                                       = 1288
                  , CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA   = 1289
                  // unused                                       = 1292
                  // unused                                       = 1293
                  // unused                                       = 1294
                  // unused                                       = 1295
                  // unused                                       = 1296
                  // unused                                       = 1297
                  , CAT_UNABLE_TO_ALTER_SCHEMA                    = 1298

                  // unused                                       = 1301
                  // unused                                       = 1302

                  // unused                                       = 1304
                  // unused                                       = 1305
                  // unused                                       = 1306
                  // unused                                       = 1307
                  // unused                                       = 1309
                  // unused                                       = 1310

                  // unused                                       = 1311
                  // unused                                       = 1312
                  , CAT_NOT_ENFORCED_RI_CONSTRAINT_WARNING        = 1313

                  // unused                                       = 1314
                  // unused                                       = 1315
                  // unused                                       = 1316
                  // unused                                       = 1317
                  // unused                                       = 1318
                  // unused                                       = 1319
                  // unused                                       = 1320
                  // unused                                       = 1321
                  // unused                                       = 1322
                  , CAT_UNABLE_TO_GRANT_PRIVILEGES                = 1323
                  // unused                                       = 1324
                  // unused                                       = 1325
                  // unused                                       = 1326
                  // unused                                       = 1327
                  , CAT_INVALID_PRIV_FOR_OBJECT                   = 1328
                  // unused                                       = 1329
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
                  // unused                                       = 1341
                  // unused                                       = 1342
                  , CAT_NO_UNREG_USER_OWNS_OBJECT                 = 1343
                  // unused                                       = 1344
                  // unused                                       = 1345
                  // unused                                       = 1346
                  , CAT_NO_UNREG_USER_OWNS_ROLES                  = 1347
                  , CAT_ROLE_IS_GRANTED_NO_DROP                   = 1348
                  , CAT_NO_UNREG_USER_GRANTED_ROLES               = 1349
                  // unused                                       = 1350
                  , CAT_DUPLICATE_ROLES_IN_LIST                   = 1351
                  , CAT_DUPLICATE_USERS_IN_LIST                   = 1352
                  // unused                                       = 1353
                  // unused                                       = 1354
                  , CAT_NO_GRANT_ROLE_TO_PUBLIC_OR_SYSTEM         = 1355
                  , CAT_COMPONENT_PRIVILEGE_CODE_EXISTS           = 1356
                  , CAT_COMPONENT_PRIVILEGE_NAME_EXISTS           = 1357
                  // unused                                       = 1358
                  , CAT_INVALID_PRIVILEGE_FOR_GRANT_OR_REVOKE     = 1359
                  // unused                                       = 1360
                  , CAT_LIBRARY_DOES_NOT_EXIST                    = 1361
                  // unused                                       = 1362
                  // unused                                       = 1363
                  , CAT_DEPENDENT_ROLE_PRIVILEGES_EXIST           = 1364
                  // unused                                       = 1365
                  , CAT_DEPENDENT_ROUTINES_EXIST                  = 1366
                  // unused                                       = 1367
                  // unused                                       = 1368
                  // unused                                       = 1369
                  , CAT_INVALID_CHARS_IN_AUTH_NAME                = 1370
                  // unused                                       = 1371
                  // unused                                       = 1372
                  // unused                                       = 1373
                  // unused                                       = 1374
                  // unused                                       = 1375
                  // unused                                       = 1376
                  // unused                                       = 1377
                  // unused                                       = 1378
                  // unused                                       = 1379
                  // unused                                       = 1380
                  // unused                                       = 1381
                  , CAT_JAR_NOT_FOUND                             = 1382
                  // unused                                       = 1383
                  // unused                                       = 1384
                  // unused                                       = 1385
                  , CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION        = 1389
                  , CAT_TRAFODION_OBJECT_EXISTS                   = 1390

                  , TRAF_ALREADY_INITIALIZED                      = 1392
                  , TRAF_NOT_INITIALIZED                          = 1393
                  , TRAF_HBASE_ACCESS_ERROR                       = 1398

                  // unused                                       = 1400
                  , CAT_UNABLE_TO_CONVERT_COLUMN_DEFAULT_VALUE_TO_CHARSET = 1401
                  , CAT_UNIQUE_INDEX_COLS_NOT_IN_DIV_EXPRS        = 1402
                  , CAT_ALTER_NOT_ALLOWED_IN_SMD                  = 1403

                  // unused                                       = 1500
                  // unused                                       = 1502
                  // unused                                       = 1503
                  // unused                                       = 1504
                  // unused                                       = 1505
                  // unused                                       = 1506

                  // IDENTITY Column (Surrogate Key) related Errors.
                  , CAT_IDENTITY_COLUMN_DATATYPE_MISMATCH         = 1510
                  , CAT_IDENTITY_COLUMN_ONE_PER_TABLE             = 1511
                  // unused                                       = 1512
                  // unused                                       = 1513
                  , CAT_IDENTITY_COLUMN_NO_ALTER_TABLE            = 1514
                  // unused                                       = 1515

                  // unused                                       = 1517
                  // unused                                       = 1518
                  // unused                                       = 1519
                  // unused                                       = 1521
                  // unused                                       = 1522
                  // unused                                       = 1523
                  // unused                                       = 1524
                  // unused                                       = 1525
                  // unused                                       = 1526
                  // unused                                       = 1527
                  // unused                                       = 1528

                  // unused                                       = 1529
                  // unused                                       = 1530

                  , CAT_NO_POPULATE_VOLATILE_INDEX                = 1540
                  , CAT_LOB_COL_CANNOT_BE_INDEX_OR_KEY            = 1541

                  // Sequence Generator errors
                  , CAT_SG_MAXVALUE_MINVALUE_ERROR                = 1570
                  , CAT_SG_INCREMENT_BY_ZERO_ERROR                = 1571
                  , CAT_SG_NEGATIVE_ERROR                         = 1572
                  , CAT_SG_STARTWITH_MINVALUE_MAXVALUE_ERROR      = 1573
                  // unused                                       = 1574
                  , CAT_SG_INCREMENT_BY_MAXVALUE_ERROR            = 1575
                  , CAT_SG_MAXIMUM_DATATYPE_ERROR                 = 1576

                  // Sequence Generator alter table errors
                  , CAT_SG_ALTER_NOT_IDENTITY_COLUMN_ERROR        = 1590
                  // unused                                       = 1591
                  , CAT_SG_ALTER_UNSUPPORTED_OPTION_ERROR         = 1592
                  // unused                                       = 1593
                  // unused                                       = 1594
                  // unused                                       = 1595
                  // unused                                       = 1596
                  // unused                                       = 1597
                  // unused                                       = 1598
                  // unused                                       = 1599
                  // unused                                       = 1600
                  // unused                                       = 1601

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
