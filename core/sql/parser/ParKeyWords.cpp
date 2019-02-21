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
/* -*-C++-*-
*******************************************************************************
*
* File:         ParKeyWords.cpp
* Description:  All keywords including those defined in ANSI 5.2,
*               Used by ulexer
*******************************************************************************
*/

#include "ComASSERT.h"
#include "ParKeyWords.h"
#include "str.h"
#include <fstream>

// ANSI SQL 99 reserved words added as COMPAQ_ reserved words if they were
// not already present in the table below.  Unreserved words were not
// considered.  Words already present, if not reserved, remain unreserved.
// No potentially reserved words in SQL 99.  None were added as compound
// key words.
// 001103 EJF

// Important notes for developers:
//
// If you are adding a keyword to Trafodion, you need to do the following:
//
// 1. Add it to the keyword table below. Use the appropriate flags in the
//    ParKeyWord object for your new keyword. (Note: Today the RESWORD_,
//    NONRESWORD_ and NONRESTOKEN_ flags are almost purely for documentation;
//    the real determination of whether a keyword is reserved or not depends
//    on how it is used in sqlparser.y. See point 3 below. Nevertheless,
//    it is useful documentation, and it might be used in the future, so
//    do try to get this right.)
// 2. Add a token definition for it to the parser (sqlparser.y). Add
//    and/or change any productions as needed to use your new token.
// 3. If your new keyword is *not* a reserved word (which is the usual case),
//    add it to the appropriate non-reserved word production in the parser.
//    That will allow the keyword to be used as a regular identifier in
//    contexts where keyword behavior is not intended. This avoids regressing
//    customer applications that might already be using that keyword as a 
//    regular identifier. See sqlparser.y, productions nonreserved_word and
//    nonreserved_func_word.
// 4. If your new keyword *is* a reserved word (this case should be rare),
//    add your keyword to the list of reserved words in ComResWords::resWords_
//    in common/ComResWords.cpp. This is important so that when an internal
//    representation of your keyword is used as an identifier, it will get
//    double quotes when converted to an external identifier. Failing to do
//    this will cause errors in some statements if your new keyword is used
//    as a delimited identifier.
// 5. If your new keyword commonly has a parenthesis after it, and it is
//    most natural for the parenthesis to be placed right after the keyword
//    (as opposed to having a space between keyword and paranthesis), then
//    add it to the keywords table in function tokIsFuncOrParenKeyword in
//    common/NAString.cpp. An example of this case is a new function name.
//    If my new function is XYZ, it looks more natural to generate XYZ(1)
//    rather than XYZ (1) in generated SQL text. An example of a case where
//    you would *not* want to do this might be adding a new logical operator
//    such as XOR. (A OR B) XOR (C OR D) looks more natural than 
//    (A OR B) XOR(C OR D). This is really an issue of esthetics rather than
//    correctness so this is not as critical as the previous four points.
// 6. Add test cases to regress/core/TEST037 of your new keyword.

// The keyword table:
//
ParKeyWord ParKeyWords::keyWords_[] = {
  ParKeyWord("ABORT",              TOK_ABORT,       NONRESTOKEN_),
  ParKeyWord("ABS",                TOK_ABS,         NONRESTOKEN_),
  ParKeyWord("ABSOLUTE",           IDENTIFIER,      ANS_|NONRESWORD_),
  // ABSOLUTE is a NONRESWORD_ for now (97/11/14) because
  // MicroStrategy is heavily using ABSOLUTE. Since we are not using
  // it yet, it is temporarily removed from the reserved word list
  // until MicroStrategy removes ABSOLUTE. It is too much work for
  // them to do that now.

  ParKeyWord("ACCESS",             TOK_ACCESS,      SECOND_|THIRD_|NONRESTOKEN_),
  ParKeyWord("ACCUMULATED",        TOK_ACCUMULATED, NONRESTOKEN_),
  ParKeyWord("ACOS",               TOK_ACOS,        NONRESTOKEN_),
  ParKeyWord("ACTION",             TOK_ACTION,      ANS_|RESWORD_),
  ParKeyWord("ACTIVATE",           TOK_ACTIVATE,      NONRESTOKEN_),
  ParKeyWord("ACTIVE",             TOK_ACTIVE,      NONRESTOKEN_),
  ParKeyWord("ADD",                TOK_ADD,         ANS_|RESWORD_),
  ParKeyWord("ADD_MONTHS",         TOK_ADD_MONTHS,  NONRESTOKEN_),
  ParKeyWord("ADMIN",              TOK_ADMIN,       COMPAQ_|RESWORD_),
  ParKeyWord("AES_ENCRYPT",        TOK_AES_ENCRYPT, NONRESTOKEN_),
  ParKeyWord("AES_DECRYPT",        TOK_AES_DECRYPT, NONRESTOKEN_),
  ParKeyWord("AFTER",              TOK_AFTER,       ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("AGGREGATE",          TOK_AGGREGATE,   COMPAQ_|RESWORD_),
  ParKeyWord("ALIAS",              TOK_ALIAS,       POTANS_|RESWORD_),
  ParKeyWord("ALIGNED",            TOK_ALIGNED,     NONRESTOKEN_),
  ParKeyWord("ALIGNED_LENGTH",     TOK_ALIGNED_LENGTH,     NONRESTOKEN_),
  ParKeyWord("ALL",                TOK_ALL,         ANS_|RESWORD_),
  ParKeyWord("ALL_DDL",            TOK_ALL_DDL,     NONRESTOKEN_),
  ParKeyWord("ALL_DML",            TOK_ALL_DML,     NONRESTOKEN_),
  ParKeyWord("ALL_UTILITY",        TOK_ALL_UTILITY, NONRESTOKEN_),
  ParKeyWord("ALLOCATE",           TOK_ALLOCATE,    ANS_|RESWORD_),
  ParKeyWord("ALLOW",              TOK_ALLOW,       NONRESTOKEN_),
  ParKeyWord("ALLOWED",            TOK_ALLOWED,     FLAGSNONE_),
  ParKeyWord("ALWAYS",             TOK_ALWAYS,      NONRESTOKEN_),
  ParKeyWord("ALTER",              TOK_ALTER,       ANS_|RESWORD_),
  ParKeyWord("ALTER_LIBRARY",      TOK_ALTER_LIBRARY,	NONRESTOKEN_),
  ParKeyWord("ALTER_MV",           TOK_ALTER_MV,	NONRESTOKEN_),
  ParKeyWord("ALTER_MV_GROUP",     TOK_ALTER_MV_GROUP,	NONRESTOKEN_),
  ParKeyWord("ALTER_ROUTINE",      TOK_ALTER_ROUTINE, NONRESTOKEN_),
  ParKeyWord("ALTER_ROUTINE_ACTION", TOK_ALTER_ROUTINE_ACTION, NONRESTOKEN_),
  ParKeyWord("ALTER_SYNONYM",      TOK_ALTER_SYNONYM,	NONRESTOKEN_),
  ParKeyWord("ALTER_TABLE",        TOK_ALTER_TABLE,	NONRESTOKEN_),
  ParKeyWord("ALTER_TRIGGER",      TOK_ALTER_TRIGGER,	NONRESTOKEN_),
  ParKeyWord("ALTER_VIEW",         TOK_ALTER_VIEW,	NONRESTOKEN_),
  ParKeyWord("AND",                TOK_AND,         ANS_|RESWORD_),
  ParKeyWord("ANSIVARCHAR",        TOK_ANSIVARCHAR, NONRESTOKEN_),
  ParKeyWord("ANY",                TOK_ANY,         ANS_|RESWORD_),
  ParKeyWord("APPEND",             TOK_APPEND,      NONRESTOKEN_),
  ParKeyWord("AQR",                TOK_AQR,         NONRESTOKEN_),
  ParKeyWord("AREA",               TOK_AREA,        NONRESTOKEN_),
  ParKeyWord("ARRAY",              TOK_ARRAY,       COMPAQ_|RESWORD_),
  ParKeyWord("ARE",                TOK_ARE,         ANS_|RESWORD_),
  ParKeyWord("AS",                 TOK_AS,          ANS_|RESWORD_),
  ParKeyWord("ASC",                TOK_ASC,         ANS_|RESWORD_),
  ParKeyWord("ASCENDING",          TOK_ASCENDING,   NONRESTOKEN_),
  ParKeyWord("ASCII",              TOK_ASCII,       NONRESTOKEN_),
  ParKeyWord("ASIN",               TOK_ASIN,        NONRESTOKEN_),
  ParKeyWord("ASSERTION",          TOK_ASSERTION,   ANS_|RESWORD_),
  ParKeyWord("ASYNC",              IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("AT",                 IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("ATAN",               TOK_ATAN,        NONRESTOKEN_),
  ParKeyWord("ATAN2",              TOK_ATAN2,       NONRESTOKEN_),
  ParKeyWord("ATOMIC",             TOK_ATOMIC,      NONRESTOKEN_),
  ParKeyWord("ATTRIBUTE",          TOK_ATTRIBUTE,   NONRESTOKEN_),
  ParKeyWord("ATTRIBUTES",         TOK_ATTRIBUTES,  NONRESTOKEN_),
  ParKeyWord("AUDIT",              TOK_AUDIT,       NONRESTOKEN_),
  ParKeyWord("AUDITCOMPRESS",      TOK_AUDITCOMPRESS, NONRESTOKEN_),
  ParKeyWord("AUTHENTICATION",     TOK_AUTHENTICATION, NONRESTOKEN_),
  ParKeyWord("AUDITONREFRESH",     TOK_AUDITONREFRESH, FLAGSNONE_),
  ParKeyWord("AUTHNAME",           TOK_AUTHNAME,    NONRESTOKEN_),
  ParKeyWord("AUTHORIZATION",      TOK_AUTHORIZATION, ANS_|RESWORD_),
  ParKeyWord("AUTHTYPE",           TOK_AUTHTYPE,    NONRESTOKEN_),
  ParKeyWord("AUTOABORT",          TOK_AUTOABORT,   NONRESTOKEN_),
  ParKeyWord("AUTOBEGIN",          TOK_AUTOBEGIN,   NONRESTOKEN_),
  ParKeyWord("AUTOCOMMIT",         TOK_AUTOCOMMIT,  NONRESTOKEN_),
  ParKeyWord("AUTOMATIC",          TOK_AUTOMATIC, FLAGSNONE_),
  ParKeyWord("AVERAGE_STREAM_WAIT", TOK_AVERAGE_STREAM_WAIT, NONRESTOKEN_),
  ParKeyWord("AVG",                TOK_AVG,         ANS_|RESWORD_),
  ParKeyWord("BACKUP",             TOK_BACKUP,      NONRESTOKEN_),
  ParKeyWord("BALANCE",            TOK_BALANCE,     NONRESTOKEN_),
  ParKeyWord("BEFORE",             TOK_BEFORE,      ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("BEGIN",              TOK_BEGIN,       ANS_|RESWORD_),
  ParKeyWord("BETWEEN",            TOK_BETWEEN,     SECOND_|ANS_|RESWORD_),
  ParKeyWord("BIGINT",             TOK_BIGINT,      NONRESTOKEN_),
  ParKeyWord("BINARY",             TOK_BINARY,      COMPAQ_|RESWORD_),
  ParKeyWord("BIT",                TOK_BIT,         ANS_|RESWORD_),
  ParKeyWord("BITAND",             TOK_BITAND,      NONRESTOKEN_),
  ParKeyWord("BITEXTRACT",         TOK_BITEXTRACT,  NONRESTOKEN_),
  ParKeyWord("BITNOT",             TOK_BITNOT,      NONRESTOKEN_),
  ParKeyWord("BITOR",              TOK_BITOR,       NONRESTOKEN_),
  ParKeyWord("BITXOR",             TOK_BITXOR,      NONRESTOKEN_),
  ParKeyWord("BIT_LENGTH",         IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("BLOB",               TOK_BLOB,        COMPAQ_|RESWORD_),
  ParKeyWord("BLOCKS",             TOK_BLOCKS,      NONRESTOKEN_),
  ParKeyWord("BLOCKSIZE",          TOK_BLOCKSIZE,   NONRESTOKEN_),
  ParKeyWord("BOOLEAN",            TOK_BOOLEAN,     RESWORD_),
  ParKeyWord("BOTH",               TOK_BOTH,        ANS_|RESWORD_),
  ParKeyWord("BREADTH",            IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("BRIEF",              TOK_BRIEF,       NONRESTOKEN_),
  ParKeyWord("BT",                 TOK_BT,          NONRESTOKEN_),
  ParKeyWord("BUFFER",             TOK_BUFFER,      NONRESTOKEN_),
  ParKeyWord("BUFFERED",           TOK_BUFFERED,    NONRESTOKEN_),
  ParKeyWord("BUFFERTOLOB",        TOK_BUFFERTOLOB, NONRESTOKEN_),
  ParKeyWord("BY",                 TOK_BY,          SECOND_|ANS_|RESWORD_),
  ParKeyWord("BYTE",               TOK_BYTE,        NONRESTOKEN_),
  ParKeyWord("BYTEINT",            TOK_BYTEINT,     NONRESTOKEN_),
  ParKeyWord("BYTES",              TOK_BYTES,       NONRESTOKEN_),
  ParKeyWord("C",                  TOK_C,           NONRESTOKEN_),
  ParKeyWord("CPP",                TOK_CPP,         NONRESTOKEN_),
  ParKeyWord("CACHE",              TOK_CACHE,       NONRESTOKEN_),
  ParKeyWord("CALL",               TOK_CALL,        FIRST_|POTANS_|RESWORD_),
  ParKeyWord("CANCEL",             TOK_CANCEL,      NONRESTOKEN_),
  ParKeyWord("CAPTURE",             TOK_CAPTURE,      NONRESTOKEN_),
  ParKeyWord("CARDINALITY",        TOK_CARDINALITY, NONRESTOKEN_),
  ParKeyWord("CASCADE",            TOK_CASCADE,     ANS_|RESWORD_),
  ParKeyWord("CASCADED",           TOK_CASCADED,    ANS_|RESWORD_),
  ParKeyWord("CASE",               TOK_CASE,        ANS_|RESWORD_),
  ParKeyWord("CASESPECIFIC",       TOK_CASESPECIFIC, SECOND_|NONRESTOKEN_),  
  ParKeyWord("CAST",               TOK_CAST,        ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("CATALOG",            TOK_CATALOG,     ANS_|RESWORD_),
  ParKeyWord("CATALOGS",           TOK_CATALOGS,    NONRESTOKEN_),
  ParKeyWord("CATALOG_NAME",       TOK_CATALOG_NAME, NONRESTOKEN_),
  ParKeyWord("CATCHUP",            TOK_CATCHUP,     FLAGSNONE_),
  ParKeyWord("CEIL",               TOK_CEIL,        NONRESTOKEN_),
  ParKeyWord("CEILING",            TOK_CEILING,     NONRESTOKEN_),
  ParKeyWord("CENTURY",            TOK_CENTURY,     NONRESTOKEN_),
  ParKeyWord("CHANGED",            TOK_CHANGED,     NONRESTOKEN_),
  ParKeyWord("CHANGES",            TOK_CHANGES,     FLAGSNONE_),
  ParKeyWord("CHAR",               TOK_CHAR,        ANS_|RESWORD_),
  ParKeyWord("CHARS",              TOK_CHARS,       NONRESTOKEN_),
  ParKeyWord("CHR",                TOK_CHR,         NONRESTOKEN_),
  ParKeyWord("CHARACTER",          TOK_CHARACTER,   ANS_|RESWORD_),
  ParKeyWord("CHARACTERS",         TOK_CHARACTERS,  NONRESTOKEN_),
  ParKeyWord("CHARACTER_LENGTH",   TOK_CHAR_LENGTH, ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("CHARACTER_SET_CATALOG", TOK_CHARACTER_SET_CATALOG, NONRESTOKEN_),
  ParKeyWord("CHARACTER_SET_NAME", TOK_CHARACTER_SET_NAME, NONRESTOKEN_),
  ParKeyWord("CHARACTER_SET_SCHEMA", TOK_CHARACTER_SET_SCHEMA, NONRESTOKEN_),
  ParKeyWord("CHAR_LENGTH",        TOK_CHAR_LENGTH, ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("CHECK",              TOK_CHECK,       ANS_|RESWORD_),
  ParKeyWord("CHECKSUM",           TOK_CHECKSUM,    NONRESTOKEN_),
  ParKeyWord("CLASS",              TOK_CLASS,       COMPAQ_|RESWORD_),
  ParKeyWord("CLASS_ORIGIN",       TOK_CLASS_ORIGIN, NONRESTOKEN_),
  ParKeyWord("CLEAN",              TOK_CLEAN,     NONRESTOKEN_),
  ParKeyWord("CLEANUP",            TOK_CLEANUP,     FIRST_ | NONRESTOKEN_),
  ParKeyWord("CLEAR",              TOK_CLEAR,       NONRESTOKEN_),
  ParKeyWord("CLEARONPURGE",       TOK_CLEARONPURGE, NONRESTOKEN_),
  ParKeyWord("CLOB",               TOK_CLOB,        COMPAQ_|RESWORD_),
  ParKeyWord("CLOSE",              TOK_CLOSE,       ANS_|RESWORD_),
  ParKeyWord("CLUSTER",            TOK_CLUSTER,     NONRESTOKEN_),
  ParKeyWord("CLUSTERING",         TOK_CLUSTERING,    NONRESTOKEN_),
  ParKeyWord("CLUSTERS",           TOK_CLUSTERS,    NONRESTOKEN_),
  ParKeyWord("COMPILE",            TOK_COMPILE,     NONRESTOKEN_),
  ParKeyWord("CMP",                TOK_ARKCMP,      NONRESTOKEN_),
  ParKeyWord("COALESCE",           TOK_COALESCE,    ANS_|RESWORD_),
  ParKeyWord("CODE_VALUE",         TOK_CODE_VALUE,  NONRESTOKEN_),
  ParKeyWord("COLLATE",            TOK_COLLATE,     ANS_|RESWORD_),
  ParKeyWord("COLLATION",          TOK_COLLATION,   ANS_|RESWORD_),
  ParKeyWord("COLLATION_CATALOG",  TOK_COLLATION_CATALOG, NONRESTOKEN_),
  ParKeyWord("COLLATION_NAME",     TOK_COLLATION_NAME, NONRESTOKEN_),
  ParKeyWord("COLLATION_SCHEMA",   TOK_COLLATION_SCHEMA, NONRESTOKEN_),
  ParKeyWord("COLUMN",             TOK_COLUMN,      ANS_|RESWORD_),
  ParKeyWord("COLUMN_CREATE",        TOK_COLUMN_CREATE, NONRESTOKEN_),
  ParKeyWord("COLUMN_DISPLAY",        TOK_COLUMN_DISPLAY, NONRESTOKEN_),
  ParKeyWord("COLUMN_LOOKUP",        TOK_COLUMN_LOOKUP, NONRESTOKEN_),
  ParKeyWord("COLUMN_NAME",        TOK_COLUMN_NAME, NONRESTOKEN_),
  ParKeyWord("COLUMN_NUMBER",      TOK_COLUMN_NUMBER, NONRESTOKEN_),
  ParKeyWord("COLUMNS",            TOK_COLUMNS, FLAGSNONE_),
  ParKeyWord("COMMAND_FUNCTION",   TOK_COMMAND_FUNCTION, NONRESTOKEN_),
  ParKeyWord("COMMANDS",           TOK_COMMANDS, NONRESTOKEN_),
  ParKeyWord("COMMENT",            TOK_COMMENT, NONRESTOKEN_),
  ParKeyWord("COMMIT",             TOK_COMMIT,      SECOND_|ANS_|RESWORD_),
  ParKeyWord("COMMITTED",          TOK_COMMITTED,   NONRESTOKEN_),
  ParKeyWord("COMP",               TOK_COMP,        NONRESTOKEN_),
  ParKeyWord("COMPACT",            TOK_COMPACT,     NONRESTOKEN_),
  ParKeyWord("COMPARE",            TOK_COMPARE,        NONRESTOKEN_),
  ParKeyWord("COMPLETE",           TOK_COMPLETE,   NONRESTOKEN_),
  ParKeyWord("COMPLETION",         IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("COMPONENT",          TOK_COMPONENT,   NONRESTOKEN_|SECOND_),
  ParKeyWord("COMPONENTS",         TOK_COMPONENTS,  NONRESTOKEN_),
  ParKeyWord("COMPRESS",           TOK_COMPRESS,    NONRESTOKEN_),
  ParKeyWord("COMPRESSION",        TOK_COMPRESSION, NONRESTOKEN_),
  ParKeyWord("COMPRESSED",         TOK_COMPRESSED,  NONRESTOKEN_),
  ParKeyWord("CONCAT",             TOK_CONCAT,      NONRESTOKEN_),
  ParKeyWord("CONCURRENCY",        TOK_CONCURRENCY, NONRESTOKEN_),
  ParKeyWord("CONCURRENT",         TOK_CONCURRENT,  NONRESTOKEN_),
  ParKeyWord("CONDITION_NUMBER",   TOK_CONDITION_NUMBER, NONRESTOKEN_),
  ParKeyWord("CONFIG",             TOK_CONFIG,      NONRESTOKEN_),
  ParKeyWord("CONFLICT",           TOK_CONFLICT,    SECOND_|NONRESTOKEN_),
  ParKeyWord("CONNECT",            IDENTIFIER,      ANS_|RESWORD_),
  //  ParKeyWord("CONNECTION",         IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("CONNECTION_NAME",    TOK_CONNECTION_NAME, NONRESTOKEN_),
  ParKeyWord("CONSTRAINT",         TOK_CONSTRAINT,  ANS_|RESWORD_),
  ParKeyWord("CONSTRAINTS",        TOK_CONSTRAINTS, ANS_|RESWORD_),
  ParKeyWord("CONSTRAINT_CATALOG", TOK_CONSTRAINT_CATALOG, NONRESTOKEN_),
  ParKeyWord("CONSTRAINT_NAME",    TOK_CONSTRAINT_NAME, NONRESTOKEN_),
  ParKeyWord("CONSTRAINT_SCHEMA",  TOK_CONSTRAINT_SCHEMA, NONRESTOKEN_),
  ParKeyWord("CONSTRUCTOR",        TOK_CONSTRUCTOR, COMPAQ_|RESWORD_),
  ParKeyWord("CONTAINS",           TOK_CONTAINS,    NONRESTOKEN_),
  ParKeyWord("CONTINUE",           TOK_CONTINUE,    ANS_|RESWORD_),
  ParKeyWord("CONTROL",            TOK_CONTROL,     NONRESTOKEN_),
  ParKeyWord("CONVERT",            TOK_CONVERT,     ANS_|RESWORD_),
  ParKeyWord("CONVERTFROMHEX",     TOK_CONVERTFROMHEX, NONRESTOKEN_),
  ParKeyWord("CONVERTTIMESTAMP",   TOK_CONVERTTIMESTAMP, NONRESTOKEN_),
  ParKeyWord("CONVERTTOBITS",      TOK_CONVERTTOBITS, NONRESTOKEN_),
  ParKeyWord("CONVERTTOHEX",       TOK_CONVERTTOHEX, NONRESTOKEN_),
  ParKeyWord("CONVERTTOHX_INTN",   TOK_CONVERTTOHX_INTN, NONRESTOKEN_),
  ParKeyWord("CORRESPONDING",      TOK_CORRESPONDING, ANS_|RESWORD_),
  ParKeyWord("COS",                TOK_COS,         NONRESTOKEN_),
  ParKeyWord("COSH",               TOK_COSH,        NONRESTOKEN_),
  ParKeyWord("COST",               TOK_COST,        NONRESTOKEN_),
  ParKeyWord("COUNT",              TOK_COUNT,       ANS_|RESWORD_),
  ParKeyWord("COPY",               TOK_COPY,        NONRESTOKEN_),
  ParKeyWord("CPU",                TOK_CPU,         NONRESTOKEN_),
  ParKeyWord("CQD",                TOK_CQD,         NONRESTOKEN_),
  ParKeyWord("CRC32",              TOK_CRC32,       NONRESTOKEN_),
  ParKeyWord("CREATE",             TOK_CREATE,      ANS_|RESWORD_),
  ParKeyWord("CREATE_LIBRARY",     TOK_CREATE_LIBRARY,   NONRESTOKEN_),
  ParKeyWord("CREATE_MV",          TOK_CREATE_MV,   NONRESTOKEN_),
  ParKeyWord("CREATE_MV_GROUP",    TOK_CREATE_MV_GROUP,   NONRESTOKEN_),
  ParKeyWord("CREATE_PROCEDURE",   TOK_CREATE_PROCEDURE, NONRESTOKEN_),
  ParKeyWord("CREATE_ROUTINE",     TOK_CREATE_ROUTINE, NONRESTOKEN_),
  ParKeyWord("CREATE_ROUTINE_ACTION", TOK_CREATE_ROUTINE_ACTION, NONRESTOKEN_),
  ParKeyWord("CREATE_TABLE",       TOK_CREATE_TABLE, NONRESTOKEN_),
  ParKeyWord("CREATE_TRIGGER",     TOK_CREATE_TRIGGER, NONRESTOKEN_),
  ParKeyWord("CREATE_SYNONYM",     TOK_CREATE_SYNONYM, NONRESTOKEN_),
  ParKeyWord("CREATE_VIEW",        TOK_CREATE_VIEW, NONRESTOKEN_),
  ParKeyWord("CROSS",              TOK_CROSS,       ANS_|RESWORD_),
  ParKeyWord("CUBE",               TOK_CUBE,        COMPAQ_|RESWORD_),
  ParKeyWord("CURDATE",            TOK_CURDATE,     NONRESTOKEN_),
  ParKeyWord("CURRENT",            TOK_CURRENT,     ANS_|RESWORD_),
  ParKeyWord("CURRENT_DATE",       TOK_CURRENT_DATE, ANS_|RESWORD_),
  ParKeyWord("CURRENT_PATH",       TOK_CURRENT_PATH, COMPAQ_|RESWORD_),
  ParKeyWord("CURRENT_RUNNING",    TOK_CURRENT_RUNNING,     NONRESTOKEN_),
  ParKeyWord("CURRENT_TIME",       TOK_CURRENT_TIME, ANS_|RESWORD_),
  ParKeyWord("CURRENT_TIMESTAMP",  TOK_CURRENT_TIMESTAMP, ANS_|RESWORD_),
  ParKeyWord("CURRENT_TIMESTAMP_RUNNING",    TOK_CURRENT_TIMESTAMP_RUNNING,     NONRESTOKEN_),
  ParKeyWord("CURRENT_TIMESTAMP_UTC",    TOK_CURRENT_TIMESTAMP_UTC,     NONRESTOKEN_),
  ParKeyWord("CURRENT_TIME_UTC",   TOK_CURRENT_TIME_UTC,     NONRESTOKEN_),
  ParKeyWord("CURRENT_USER",       TOK_CURRENT_USER, ANS_|RESWORD_),
  ParKeyWord("CURRNT_USR_INTN",    TOK_CURRNT_USR_INTN, RESWORD_),
  ParKeyWord("CURSOR",             TOK_CURSOR,      FIRST_|ANS_|RESWORD_),
  ParKeyWord("CURSOR_NAME",        TOK_CURSOR_NAME, NONRESTOKEN_),
  ParKeyWord("CURTIME",            TOK_CURTIME,     NONRESTOKEN_),
  ParKeyWord("CYCLE",              TOK_CYCLE,       ANS_|RESWORD_),
  ParKeyWord("D",                  TOK_D,           NONRESTOKEN_),
  ParKeyWord("DATA",               TOK_DATA,        POTANS_|NONRESTOKEN_|NONRESWORD_),
  // The word DATA is used in several places in mxcmp (ISP and RFORK),
  // and is in SqlParser.y's "nonreserved_word" list, so we have not
  // made it reserved even though it is in the potential ANSI reserved
  // word list.

  ParKeyWord("DATABASE",           TOK_DATABASE,    NONRESTOKEN_),
  ParKeyWord("DATA_OFFSET",        TOK_DATA_OFFSET, NONRESTOKEN_),
  ParKeyWord("DATE",               TOK_DATE,        FIRST_|ANS_|RESWORD_),
  ParKeyWord("DATEADD",            TOK_DATEADD,     NONRESTOKEN_),
  ParKeyWord("DATE_ADD",           TOK_DATE_ADD,    NONRESTOKEN_),
  ParKeyWord("DATE_PART",          TOK_DATE_PART,   NONRESTOKEN_),
  ParKeyWord("DATE_TRUNC",         TOK_DATE_TRUNC,  NONRESTOKEN_),
  ParKeyWord("DATEDIFF",           TOK_DATEDIFF,    NONRESTOKEN_),
  ParKeyWord("DATE_SUB",           TOK_DATE_SUB,    NONRESTOKEN_),
  ParKeyWord("DATEFORMAT",         TOK_DATEFORMAT,  NONRESTOKEN_),
  ParKeyWord("DATETIME",           TOK_DATETIME,    COMPAQ_|RESWORD_),
  ParKeyWord("DATETIME_CODE",      TOK_DATETIME_CODE, NONRESTOKEN_),
  ParKeyWord("DAY",                TOK_DAY,         ANS_|RESWORD_),
  ParKeyWord("DAYNAME",            TOK_DAYNAME,     NONRESTOKEN_),
  ParKeyWord("DAYOFMONTH",         TOK_DAYOFMONTH,  NONRESTOKEN_),
  ParKeyWord("DAYOFWEEK",          TOK_DAYOFWEEK,   NONRESTOKEN_),
  ParKeyWord("DAYOFYEAR",          TOK_DAYOFYEAR,   NONRESTOKEN_),
  ParKeyWord("DB",                 TOK_DATABASE,    NONRESTOKEN_),
  ParKeyWord("DBA",		   TOK_DBA,	    NONRESTOKEN_),
  ParKeyWord("DCOMPRESS",          TOK_DCOMPRESS,   NONRESTOKEN_),
  ParKeyWord("DDL",                TOK_DDL,         NONRESTOKEN_),
  ParKeyWord("DE",                 TOK_DE,          FLAGSNONE_),
  ParKeyWord("DEALLOCATE",         TOK_DEALLOCATE,  ANS_|RESWORD_),
  ParKeyWord("DEBUG",              TOK_DEBUG,       NONRESTOKEN_),
  ParKeyWord("DEC",                TOK_DECIMAL,     ANS_|RESWORD_),
  ParKeyWord("DECIMAL",            TOK_DECIMAL,     ANS_|RESWORD_),
  ParKeyWord("DECADE",             TOK_DECADE,      NONRESTOKEN_),
  ParKeyWord("DECODE",             TOK_DECODE,      NONRESTOKEN_),
  ParKeyWord("DECLARE",            TOK_DECLARE,     ANS_|RESWORD_),
  ParKeyWord("DECODE_BASE64",      TOK_DECODE_BASE64,  NONRESTOKEN_),
  ParKeyWord("DEFAULT",            TOK_DEFAULT,     SECOND_|ANS_|RESWORD_),
  ParKeyWord("DEFAULTS",           TOK_DEFAULTS,    NONRESTOKEN_),
  ParKeyWord("DEFERRABLE",         IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("DEFERRED",           IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("DEFINER",            TOK_DEFINER,     NONRESTOKEN_),
  ParKeyWord("DEFINITION",         TOK_DEFINITION,  NONRESTOKEN_),
  ParKeyWord("DEGREES",            TOK_DEGREES,     NONRESTOKEN_),
  ParKeyWord("DELAY",              TOK_DELAY,       NONRESTOKEN_),
  ParKeyWord("DELETE",             TOK_DELETE,      ANS_|RESWORD_),
  ParKeyWord("DELIMITER",          TOK_DELIMITER,   NONRESTOKEN_),
  ParKeyWord("DEPENDENT", 	   TOK_DEPENDENT,   NONRESTOKEN_),
  ParKeyWord("DENSE_RANK",         TOK_D_RANK,      NONRESTOKEN_),
  ParKeyWord("DEPTH",              IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("DEREF",              TOK_DEREF,       COMPAQ_|RESWORD_),
  ParKeyWord("DESC",               TOK_DESC,        ANS_|RESWORD_),
  ParKeyWord("DESCENDING",         TOK_DESCENDING,  NONRESTOKEN_),
  ParKeyWord("DESCRIBE",           TOK_DESCRIBE,    ANS_|RESWORD_),
  ParKeyWord("DESCRIPTOR",         TOK_DESCRIPTOR,  ANS_|RESWORD_),
  ParKeyWord("DESTROY",            TOK_DESTROY,     COMPAQ_|RESWORD_),
  ParKeyWord("DESTRUCTOR",         TOK_DESTRUCTOR,  COMPAQ_|RESWORD_),
  ParKeyWord("DETAIL",             TOK_DETAIL,      NONRESTOKEN_),
  ParKeyWord("DETAILS",            TOK_DETAILS,      NONRESTOKEN_),
  ParKeyWord("DETERMINISTIC",      TOK_DETERMINISTIC, COMPAQ_|RESWORD_),
  ParKeyWord("DIAGNOSTICS",        TOK_DIAGNOSTICS, ANS_|RESWORD_),
  ParKeyWord("DICTIONARY",         IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("DIFF1",              TOK_DIFF1,       NONRESTOKEN_),
  ParKeyWord("DIFF2",              TOK_DIFF2,       NONRESTOKEN_),
  ParKeyWord("DIRECTEDBY",         TOK_DIRECTEDBY,  FLAGSNONE_),
  ParKeyWord("DISABLE",            TOK_DISABLE,     NONRESTOKEN_),
  ParKeyWord("DISCONNECT",         IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("DISK",		   TOK_DISK,        NONRESTOKEN_),
  ParKeyWord("DISPLAY",            TOK_DISPLAY,     NONRESTOKEN_),
  ParKeyWord("DISTINCT",           TOK_DISTINCT,    ANS_|RESWORD_),
  ParKeyWord("DIVISION",           TOK_DIVISION,    NONRESTOKEN_),
  ParKeyWord("DO",                 TOK_DO,          NONRESTOKEN_),
  ParKeyWord("DOMAIN",             IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("DOUBLE",             TOK_DOUBLE,      ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("DOUBLE_IEEE",        TOK_DOUBLE_IEEE, NONRESTOKEN_),
  ParKeyWord("DOW",                TOK_DOW,         NONRESTOKEN_),
  ParKeyWord("DOY",                TOK_DOY,         NONRESTOKEN_),
  ParKeyWord("DROP",               TOK_DROP,        ANS_|RESWORD_),
  ParKeyWord("DROPPABLE",          TOK_DROPPABLE,   SECOND_|NONRESTOKEN_),
  ParKeyWord("DROP_LIBRARY",       TOK_DROP_LIBRARY,	  NONRESTOKEN_),
  ParKeyWord("DROP_MV",            TOK_DROP_MV,		  NONRESTOKEN_),
  ParKeyWord("DROP_MV_GROUP",      TOK_DROP_MV_GROUP,	  NONRESTOKEN_),
  ParKeyWord("DROP_PROCEDURE",     TOK_DROP_PROCEDURE,	  NONRESTOKEN_),
  ParKeyWord("DROP_ROUTINE",       TOK_DROP_ROUTINE, NONRESTOKEN_),
  ParKeyWord("DROP_ROUTINE_ACTION", TOK_DROP_ROUTINE_ACTION, NONRESTOKEN_),
  ParKeyWord("DROP_SYNONYM",       TOK_DROP_SYNONYM,	  NONRESTOKEN_),
  ParKeyWord("DROP_TABLE",         TOK_DROP_TABLE,	  NONRESTOKEN_),
  ParKeyWord("DROP_TRIGGER",       TOK_DROP_TRIGGER,	  NONRESTOKEN_),
  ParKeyWord("DROP_VIEW",	   TOK_DROP_VIEW,	  NONRESTOKEN_),
  ParKeyWord("DSLACK",             TOK_DSLACK,      NONRESTOKEN_),
  ParKeyWord("DUAL",               TOK_DUAL,        NONRESTOKEN_),  
  ParKeyWord("DUPLICATE",          TOK_DUPLICATE,        NONRESTOKEN_),
  ParKeyWord("DYNAMIC",            TOK_DYNAMIC,     COMPAQ_|RESWORD_),
  ParKeyWord("DYNAMIC_FUNCTION",   TOK_DYNAMIC_FUNCTION, NONRESTOKEN_),
  ParKeyWord("EACH",               TOK_EACH,        ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("EID",                TOK_EID,         NONRESTOKEN_),  
  ParKeyWord("ELSE",               TOK_ELSE,        ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("ELSEIF",             TOK_ELSEIF,      POTANS_|RESWORD_),
  ParKeyWord("ENABLE",             TOK_ENABLE,      NONRESTOKEN_),
  ParKeyWord("ENCODE_BASE64",      TOK_ENCODE_BASE64,  NONRESTOKEN_),
  ParKeyWord("ENCODE_KEY",         TOK_ENCODE_KEY,  NONRESTOKEN_),
  ParKeyWord("END",                TOK_END,         ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("END-EXEC",           IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("ENFORCED",           TOK_ENFORCED,    SECOND_|NONRESTOKEN_),
  ParKeyWord("ENFORCERS",          TOK_ENFORCERS,   NONRESTOKEN_),
  ParKeyWord("ENTERPRISE",         TOK_ENTERPRISE,  NONRESTOKEN_),
  ParKeyWord("ENTRIES",            TOK_ENTRIES,     NONRESTOKEN_),
  ParKeyWord("ENTRY",              TOK_ENTRY,       NONRESTOKEN_),
  ParKeyWord("ENVVAR",             TOK_ENVVAR,      NONRESTOKEN_),
  ParKeyWord("ENVVARS",            TOK_ENVVARS,     NONRESTOKEN_),
  ParKeyWord("EOF",                TOK_EOF,         NONRESTOKEN_),
  ParKeyWord("EPOCH",              TOK_EPOCH,       NONRESTOKEN_),
  ParKeyWord("EQUALS",             IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("ERROR",              TOK_ERROR,       NONRESTOKEN_),
  ParKeyWord("ESCAPE",             TOK_ESCAPE,      ANS_|RESWORD_),
  ParKeyWord("ET",                 TOK_ET,          NONRESTOKEN_),
  ParKeyWord("EUROPEAN",           TOK_EUROPEAN,    NONRESTOKEN_),
  ParKeyWord("EVERY",              TOK_EVERY,       NONRESTOKEN_),
  ParKeyWord("EXCEPT",             TOK_EXCEPT,      ANS_|RESWORD_),
  ParKeyWord("EXCEPTION",          TOK_EXCEPTION,   ANS_|RESWORD_),
  ParKeyWord("EXCEPTION_TABLE",    TOK_EXCEPTION_TABLE,     NONRESTOKEN_),
  ParKeyWord("EXCHANGE",           TOK_EXCHANGE,    NONRESTOKEN_),
  ParKeyWord("EXCHANGE_AND_SORT",  TOK_EXCHANGE_AND_SORT, NONRESTOKEN_),
  ParKeyWord("EXCLUSIVE",          TOK_EXCLUSIVE,   NONRESTOKEN_),
  ParKeyWord("EXEC",               IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("EXECUTE",            TOK_EXECUTE,     ANS_|RESWORD_),
  ParKeyWord("EXECUTION",          TOK_EXECUTION,   NONRESTOKEN_),
  ParKeyWord("EXISTING",           TOK_EXISTING,    NONRESTOKEN_),
  ParKeyWord("EXISTS",             TOK_EXISTS,      ANS_|RESWORD_),
  ParKeyWord("EXIT",               TOK_EXIT,        NONRESTOKEN_),
  ParKeyWord("EXP",                TOK_EXP,         NONRESTOKEN_),
  ParKeyWord("EXPLAIN",            TOK_EXPLAIN,     NONRESTOKEN_),
  ParKeyWord("EXTEND",             TOK_EXTEND,      NONRESTOKEN_),
  ParKeyWord("EXTENT",             TOK_EXTENT,      NONRESTOKEN_),
  ParKeyWord("EXTERNAL",           TOK_EXTERNAL,    ANS_|RESWORD_),
  ParKeyWord("EXTERNALTOLOB",      TOK_EXTERNALTOLOB,   NONRESTOKEN_),
  ParKeyWord("EXTERNALTOSTRING",      TOK_EXTERNALTOSTRING,   NONRESTOKEN_),
  ParKeyWord("EXTRACT",            TOK_EXTRACT,     ANS_|RESWORD_),
  ParKeyWord("EXTRACT_SOURCE",     TOK_EXTRACT_SOURCE, NONRESTOKEN_),
  ParKeyWord("EXTRACT_TARGET",     TOK_EXTRACT_TARGET, NONRESTOKEN_),
  ParKeyWord("FALLBACK",           TOK_FALLBACK,    NONRESTOKEN_),
  ParKeyWord("FALSE",              TOK_FALSE,       ANS_|RESWORD_),
  ParKeyWord("FAMILY",               TOK_FAMILY,        NONRESTOKEN_),
  ParKeyWord("FAST",               TOK_FAST,        NONRESTOKEN_),
  ParKeyWord("FEATURE_VERSION_INFO",TOK_FEATURE_VERSION_INFO, NONRESTOKEN_),
  ParKeyWord("FETCH",              TOK_FETCH,       ANS_|RESWORD_),
  ParKeyWord("FILE",               TOK_FILE,        NONRESTOKEN_),
  ParKeyWord("FILETOLOB",          TOK_FILETOLOB,   NONRESTOKEN_),
  ParKeyWord("FILETOEXTERNAL",          TOK_FILETOEXTERNAL,   NONRESTOKEN_),
  ParKeyWord("FINAL",              TOK_FINAL,       NONRESTOKEN_),
  ParKeyWord("FIRST",              TOK_FIRST,       SECOND_|ANS_|RESWORD_),
  ParKeyWord("FIRSTDAYOFYEAR",     TOK_FIRSTDAYOFYEAR, NONRESTOKEN_),
  ParKeyWord("FIRST_FSCODE",       TOK_FIRST_FSCODE, NONRESTOKEN_),
  ParKeyWord("FIXED",              TOK_FIXED,        NONRESTOKEN_),
  ParKeyWord("FLOAT",              TOK_FLOAT,       ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("FLOAT_IEEE",         TOK_FLOAT_IEEE,  NONRESTOKEN_),
  ParKeyWord("FLOOR",              TOK_FLOOR,       NONRESTOKEN_),
  ParKeyWord("FN",                 TOK_FN,          NONRESTOKEN_),
  ParKeyWord("FOLLOWING",          TOK_FOLLOWING,   NONRESTOKEN_),
  ParKeyWord("FOR",                TOK_FOR,         FIRST_|ANS_|RESWORD_),
  ParKeyWord("FORCE",              TOK_FORCE,       NONRESTOKEN_),
  ParKeyWord("FOREIGN",            TOK_FOREIGN,     ANS_|RESWORD_),
  ParKeyWord("FORMAT",             TOK_FORMAT,      NONRESTOKEN_),
  ParKeyWord("FOUND",              TOK_FOUND,       ANS_|RESWORD_),
  ParKeyWord("FRACTION",           TOK_FRACTION,    COMPAQ_|RESWORD_),
  ParKeyWord("FREE",               TOK_FREE,        COMPAQ_|RESWORD_),
  ParKeyWord("FREESPACE",          TOK_FREESPACE,   NONRESTOKEN_),  
  ParKeyWord("FROM",               TOK_FROM,        ANS_|RESWORD_),
  ParKeyWord("FROM_HEX",           TOK_FROM_HEX,    NONRESTOKEN_),
  ParKeyWord("FULL",               TOK_FULL,        ANS_|RESWORD_),
  ParKeyWord("FUNCTION",           TOK_FUNCTION,    COMPAQ_|RESWORD_),
  ParKeyWord("FUNCTIONS",          TOK_FUNCTIONS,   NONRESTOKEN_),
  ParKeyWord("G",                  TOK_G,           NONRESTOKEN_),
  ParKeyWord("GENERAL",            IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("GENERATE",           TOK_GENERATE,    NONRESTOKEN_),
  ParKeyWord("GENERATED",          TOK_GENERATED,    NONRESTOKEN_),
  ParKeyWord("GET",                TOK_GET,         ANS_|RESWORD_),
  ParKeyWord("GHOST",              TOK_GHOST,       NONRESTOKEN_),
  ParKeyWord("GIVE",               TOK_GIVE,        NONRESTOKEN_),
  ParKeyWord("GLOBAL",             TOK_GLOBAL,      ANS_|RESWORD_),
  ParKeyWord("GO",                 TOK_GO,          FIRST_|ANS_|RESWORD_),
  ParKeyWord("GOTO",               TOK_GOTO,        FIRST_|ANS_|RESWORD_),
  ParKeyWord("GRANT",              TOK_GRANT,       ANS_|RESWORD_),
  ParKeyWord("GRANTEES",           TOK_GRANTEES,    NONRESTOKEN_),
  ParKeyWord("GRANTED",            TOK_GRANTED,     NONRESTOKEN_),
  ParKeyWord("GREATEST",                  TOK_GREATEST,           NONRESTOKEN_),
  ParKeyWord("GROUP",              TOK_GROUP,       ANS_|RESWORD_),
  ParKeyWord("GROUP_CONCAT",       TOK_GROUP_CONCAT,   NONRESTOKEN_),
  ParKeyWord("GROUPING",           TOK_GROUPING,    COMPAQ_|RESWORD_),
  ParKeyWord("GROUPING_ID",        TOK_GROUPING_ID, NONRESTOKEN_),
  ParKeyWord("GZIP",               TOK_GZIP,         NONRESTOKEN_),
  ParKeyWord("HARDWARE",           TOK_HARDWARE,    NONRESTOKEN_),
  ParKeyWord("HASH",               TOK_HASH,        NONRESTOKEN_),
  ParKeyWord("HASH2",              TOK_HASH2,        NONRESTOKEN_),
  ParKeyWord("HASHPARTFUNC",       TOK_HASHPARTFUNC, NONRESTOKEN_),
  ParKeyWord("HASH2PARTFUNC",      TOK_HASH2PARTFUNC, NONRESTOKEN_),
  ParKeyWord("HAVING",             TOK_HAVING,      ANS_|RESWORD_),
  ParKeyWord("HBASE",              TOK_HBASE,      NONRESTOKEN_),
  ParKeyWord("HBASE_OPTIONS",  TOK_HBASE_OPTIONS,      NONRESTOKEN_),
  ParKeyWord("HBASE_TIMESTAMP",  TOK_HBASE_TIMESTAMP,      NONRESTOKEN_),
  ParKeyWord("HBASE_VERSION",  TOK_HBASE_VERSION,      NONRESTOKEN_),
  ParKeyWord("HBMAP_TABLE",    TOK_HBMAP_TABLE, NONRESTOKEN_),
  ParKeyWord("HEADER",             TOK_HEADER,      NONRESTOKEN_),
  ParKeyWord("HEADING",            TOK_HEADING,     NONRESTOKEN_),
  ParKeyWord("HEADINGS",           TOK_HEADINGS,    NONRESTOKEN_),
  ParKeyWord("HEX",                TOK_HEX,         NONRESTOKEN_),
  ParKeyWord("HEXADECIMAL",        TOK_HEXADECIMAL, NONRESTOKEN_),
  ParKeyWord("HIGH_VALUE",         TOK_HIGH_VALUE,  NONRESTOKEN_),
  ParKeyWord("HIVE",               TOK_HIVE,  NONRESTOKEN_),
  ParKeyWord("HIVEMD",                    TOK_HIVEMD,  NONRESTOKEN_),
  ParKeyWord("HOLD",               TOK_HOLD,        THIRD_|NONRESTOKEN_),
  ParKeyWord("HORIZONTAL",         TOK_HORIZONTAL,  NONRESTOKEN_),
  ParKeyWord("HOST",               TOK_HOST,        COMPAQ_|RESWORD_),
  ParKeyWord("HOUR",               TOK_HOUR,        ANS_|RESWORD_),
  ParKeyWord("HOURS",              TOK_HOURS,       NONRESTOKEN_),
  ParKeyWord("ICOMPRESS",          TOK_ICOMPRESS,   NONRESTOKEN_),
  ParKeyWord("IDENTITY",           TOK_IDENTITY,    ANS_|RESWORD_),
  ParKeyWord("IF",                 TOK_IF,          POTANS_|RESWORD_),
  ParKeyWord("IFNULL",             TOK_NVL,         NONRESTOKEN_),
  ParKeyWord("IGNORE",             TOK_IGNORE,      POTANS_|RESWORD_),
  ParKeyWord("IGNORETRIGGERS",     TOK_IGNORETRIGGERS, FLAGSNONE_),
  ParKeyWord("IGNORE_TRIGGER",     TOK_IGNORE_TRIGGER, NONRESTOKEN_),
  ParKeyWord("IMMEDIATE",          TOK_IMMEDIATE,   ANS_|RESWORD_),
  ParKeyWord("IMMUTABLE",          TOK_IMMUTABLE,   NONRESTOKEN_),
  ParKeyWord("IMPLICIT",           TOK_IMPLICIT,    NONRESTOKEN_),
  ParKeyWord("IN",                 TOK_IN,          SECOND_|ANS_|RESWORD_),
  ParKeyWord("INCLUSIVE",          TOK_INCLUSIVE,   NONRESTOKEN_),
  ParKeyWord("INCREMENT",          TOK_INCREMENT,    NONRESTOKEN_),
  ParKeyWord("INCREMENTAL",        TOK_INCREMENTAL,  NONRESTOKEN_),
  ParKeyWord("INDEX",              TOK_INDEX,       SECOND_|NONRESTOKEN_),
  ParKeyWord("INDEXES",            TOK_INDEXES,     NONRESTOKEN_),
  ParKeyWord("INDEX_TABLE",        TOK_INDEX_TABLE, NONRESTOKEN_),
  ParKeyWord("INDICATOR",          TOK_INDICATOR,   ANS_|RESWORD_),
  ParKeyWord("INDICATOR_DATA",     TOK_INDICATOR_DATA, NONRESTOKEN_),
  ParKeyWord("INDICATOR_POINTER",  TOK_INDICATOR_POINTER, NONRESTOKEN_),
  ParKeyWord("INDICATOR_TYPE",     TOK_INDICATOR_TYPE, NONRESTOKEN_),
  ParKeyWord("INET_ATON",          TOK_INET_ATON, NONRESTOKEN_),
  ParKeyWord("INET_NTOA",          TOK_INET_NTOA, NONRESTOKEN_),
  ParKeyWord("INITIAL",            TOK_INITIAL,       NONRESTOKEN_),
  ParKeyWord("INITIALIZATION",     TOK_INITIALIZATION, FLAGSNONE_),
  ParKeyWord("INITIALIZE",         TOK_INITIALIZE,  FIRST_|NONRESTOKEN_),
  ParKeyWord("INITIALIZED",        TOK_INITIALIZED, FLAGSNONE_),
  ParKeyWord("INITIALLY",          IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("INGEST",             TOK_INGEST,      NONRESTOKEN_),
  ParKeyWord("INNER",              TOK_INNER,       ANS_|RESWORD_),
  ParKeyWord("INOUT",              TOK_INOUT,       COMPAQ_|RESWORD_),
  ParKeyWord("INPUT",              TOK_INPUT,       ANS_|RESWORD_),
  ParKeyWord("INPUTS",             TOK_INPUTS,      NONRESTOKEN_),
  ParKeyWord("INSENSITIVE",        IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("INSERT",             TOK_INSERT,      ANS_|RESWORD_),
  ParKeyWord("INSERT_ONLY",        TOK_INSERT_ONLY, NONRESTOKEN_),
  ParKeyWord("INS",                TOK_INS,         NONRESTOKEN_),
  ParKeyWord("INSERTLOG",          TOK_INSERTLOG,   FLAGSNONE_),
  ParKeyWord("INSTR",              TOK_INSTR,       NONRESTOKEN_),
  ParKeyWord("INT",                TOK_INTEGER,     ANS_|RESWORD_),
  ParKeyWord("INTEGER",            TOK_INTEGER,     ANS_|RESWORD_),
  ParKeyWord("INTERNAL",           TOK_INTERNAL,    FLAGSNONE_),
  ParKeyWord("INTERSECT",          TOK_INTERSECT,   ANS_|RESWORD_),
  ParKeyWord("INTERVAL",           TOK_INTERVAL,    ANS_|RESWORD_),
  ParKeyWord("INTERVALS",          TOK_INTERVALS,   NONRESTOKEN_),
  ParKeyWord("INTO",               TOK_INTO,        ANS_|RESWORD_),
  ParKeyWord("INVOKE",             TOK_INVOKE,      NONRESTOKEN_),
  ParKeyWord("INVOKER",            TOK_INVOKER,     NONRESTOKEN_),
  ParKeyWord("IO",                 TOK_IO,          NONRESTOKEN_),
  ParKeyWord("IS",                 TOK_IS,          ANS_|RESWORD_),
  ParKeyWord("ISLACK",             TOK_ISLACK,      NONRESTOKEN_),
  ParKeyWord("ISNULL",             TOK_ISNULL,      NONRESTOKEN_),
  ParKeyWord("ISOLATE",            TOK_ISOLATE,     NONRESTOKEN_),
  ParKeyWord("ISOLATION",          TOK_ISOLATION,   ANS_|RESWORD_),
  ParKeyWord("IS_IPV4",            TOK_ISIPV4,      NONRESTOKEN_),
  ParKeyWord("IS_IPV6",            TOK_ISIPV6,      NONRESTOKEN_),
  ParKeyWord("ITERATE",            TOK_ITERATE,     COMPAQ_|RESWORD_),
  ParKeyWord("IUDLOG",             TOK_IUDLOG,      FLAGSNONE_),
  ParKeyWord("IUD_LOG_TABLE",      TOK_IUD_LOG_TABLE, FLAGSNONE_),
  ParKeyWord("INVALID", 	   TOK_INVALID,    NONRESTOKEN_),
  ParKeyWord("INVALIDATE", 	   TOK_INVALIDATE, NONRESTOKEN_),
  ParKeyWord("JAVA",               TOK_JAVA,        NONRESTOKEN_),
  ParKeyWord("JOIN",               TOK_JOIN,        SECOND_|ANS_|RESWORD_),
  ParKeyWord("JOURNAL",            TOK_JOURNAL,     NONRESTOKEN_), 
  ParKeyWord("JSON_OBJECT_FIELD_TEXT",  TOK_JSONOBJECTFIELDTEXT, NONRESTOKEN_),
  ParKeyWord("JULIANTIMESTAMP",    TOK_JULIANTIMESTAMP, NONRESTOKEN_),
  ParKeyWord("K",                  TOK_K,           NONRESTOKEN_),
  ParKeyWord("KEY",                TOK_KEY,         ANS_|RESWORD_),
  ParKeyWord("KEY_RANGE_COMPARE",  TOK_KEY_RANGE_COMPARE, NONRESTOKEN_),
  ParKeyWord("LABEL",              TOK_LABEL,       NONRESTOKEN_),
  ParKeyWord("LABEL_ALTER",        TOK_LABEL_ALTER,        NONRESTOKEN_),
  ParKeyWord("LABEL_CREATE",       TOK_LABEL_CREATE,       NONRESTOKEN_),
  ParKeyWord("LABEL_DROP",         TOK_LABEL_DROP,         NONRESTOKEN_),
  ParKeyWord("LABEL_PURGEDATA",    TOK_LABEL_PURGEDATA,    NONRESTOKEN_),
  ParKeyWord("LAG",               TOK_LAG,        ANS_|RESWORD_),  
  ParKeyWord("LANGUAGE",           TOK_LANGUAGE,    ANS_|RESWORD_),
  ParKeyWord("LARGE",              TOK_LARGE,       COMPAQ_|RESWORD_),
  ParKeyWord("LARGEINT",           TOK_LARGEINT,    NONRESTOKEN_),
  ParKeyWord("LAST",               TOK_LAST,        ANS_|RESWORD_),
  ParKeyWord("LASTNOTNULL",        TOK_LASTNOTNULL, NONRESTOKEN_),
  ParKeyWord("LASTSYSKEY",         TOK_LASTSYSKEY,  NONRESTOKEN_),
  ParKeyWord("LAST_DAY",           TOK_LAST_DAY,    NONRESTOKEN_),
  ParKeyWord("LAST_FSCODE",        TOK_LAST_FSCODE, NONRESTOKEN_),
  ParKeyWord("LAST_SYSKEY",        TOK_LAST_SYSKEY, NONRESTOKEN_),
  ParKeyWord("LATERAL",            TOK_LATERAL,     COMPAQ_|RESWORD_),
  ParKeyWord("LCASE",              TOK_LCASE,       NONRESTOKEN_),
  ParKeyWord("LEAD",               TOK_LEAD,        ANS_|RESWORD_),
  ParKeyWord("LEADING",            TOK_LEADING,     ANS_|RESWORD_),
  ParKeyWord("LEADING_PRECISION",  TOK_LEADING_PRECISION, NONRESTOKEN_),
  ParKeyWord("LEAST",                  TOK_LEAST,           NONRESTOKEN_),
  ParKeyWord("LEAVE",              IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("LEFT",               TOK_LEFT,        ANS_|RESWORD_),
  ParKeyWord("LENGTH",             TOK_LENGTH,      NONRESTOKEN_),
  ParKeyWord("LESS",               IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("LEVEL",              TOK_LEVEL,       ANS_|RESWORD_),
  ParKeyWord("LEVELS",             TOK_LEVELS,      NONRESTOKEN_),
  ParKeyWord("LIBRARY",            TOK_LIBRARY,     NONRESTOKEN_|SECOND_),
  ParKeyWord("LIBRARIES",          TOK_LIBRARIES,   NONRESTOKEN_),
  ParKeyWord("LIKE",               TOK_LIKE,        ANS_|RESWORD_),
  ParKeyWord("LIMIT",              TOK_LIMIT,      POTANS_|RESWORD_),
  ParKeyWord("LINE_NUMBER",        TOK_LINE_NUMBER, NONRESTOKEN_),
  ParKeyWord("LOAD",               TOK_LOAD,        NONRESTOKEN_|SECOND_),
  ParKeyWord("LOADTOLOB",          TOK_LOADTOLOB,   NONRESTOKEN_),
  ParKeyWord("LOAD_ID",            TOK_LOAD_ID,     NONRESTOKEN_),
  ParKeyWord("LOB",                TOK_LOB,         NONRESTOKEN_|SECOND_),
  ParKeyWord("LOBLENGTH",          TOK_LOBLENGTH,   NONRESTOKEN_),
  ParKeyWord("LOBTOBUFFER",        TOK_LOBTOBUFFER, NONRESTOKEN_),
  ParKeyWord("LOBTOFILE",          TOK_LOBTOFILE, NONRESTOKEN_),
  ParKeyWord("LOBTOSTRING",        TOK_LOBTOSTRING, NONRESTOKEN_),
  ParKeyWord("EMPTY_BLOB",         TOK_EMPTY_BLOB, NONRESTOKEN_),
  ParKeyWord("EMPTY_CLOB",         TOK_EMPTY_CLOB, NONRESTOKEN_),
  ParKeyWord("LOCAL",              TOK_LOCAL,       ANS_|RESWORD_),
  ParKeyWord("LOCALTIME",          TOK_LOCALTIME,   COMPAQ_|RESWORD_),
  ParKeyWord("LOCALTIMESTAMP",     TOK_LOCALTIMESTAMP, COMPAQ_|RESWORD_),
  ParKeyWord("LOCATE",             TOK_LOCATE,      NONRESTOKEN_),
  ParKeyWord("LOCATION",           TOK_LOCATION,    NONRESTOKEN_),
  ParKeyWord("LOCATOR",            TOK_LOCATOR,     COMPAQ_|RESWORD_),
  ParKeyWord("LOCK",               TOK_LOCK,        FIRST_|NONRESTOKEN_),
  ParKeyWord("LOCKING",            TOK_LOCKING,     NONRESTOKEN_),
  ParKeyWord("LOCKONREFRESH",      TOK_LOCKONREFRESH, FLAGSNONE_),
  ParKeyWord("LOG",                TOK_LOG,         NONRESTOKEN_),
  ParKeyWord("LOG10",              TOK_LOG10,       NONRESTOKEN_),
  ParKeyWord("LOG2",               TOK_LOG2,       NONRESTOKEN_),
  ParKeyWord("LOGGABLE",           TOK_LOGGABLE,    FLAGSNONE_),
  ParKeyWord("LOGON",              TOK_LOGON,       NONRESTOKEN_),
  ParKeyWord("LONG",               TOK_LONG,        NONRESTOKEN_),
  ParKeyWord("LONGWVARCHAR",       TOK_LONGWVARCHAR,NONRESTOKEN_),
  ParKeyWord("LOOP",               IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("LOWER",              TOK_LOWER,       ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("LOW_VALUE",          TOK_LOW_VALUE,   NONRESTOKEN_),
  ParKeyWord("LPAD",               TOK_LPAD,        NONRESTOKEN_),
  ParKeyWord("LSDEC",              TOK_LSDECIMAL,   NONRESTOKEN_),
  ParKeyWord("LSDECIMAL",          TOK_LSDECIMAL,   NONRESTOKEN_),
  ParKeyWord("LTRIM",              TOK_LTRIM,       NONRESTOKEN_),
  ParKeyWord("LZO",                TOK_LZO,         NONRESTOKEN_),
  ParKeyWord("M",                  TOK_M,           NONRESTOKEN_),
  ParKeyWord("MAINTAIN",           TOK_MAINTAIN,    SECOND_|COMPAQ_|RESWORD_),
  ParKeyWord("MANAGEMENT",         TOK_MANAGEMENT,  NONRESTOKEN_),
  ParKeyWord("MANUAL",             TOK_MANUAL,      FLAGSNONE_),
  ParKeyWord("MAP",                TOK_MAP,         NONRESTOKEN_),
  ParKeyWord("MASTER",             TOK_MASTER,      NONRESTOKEN_),
  ParKeyWord("MATCH",              TOK_MATCH,       ANS_|RESWORD_),
  ParKeyWord("MATCHED",            TOK_MATCHED,     ANS_|RESWORD_),
  ParKeyWord("MATERIALIZED",       TOK_MATERIALIZED,FLAGSNONE_),
  ParKeyWord("MAX",                TOK_MAX,         ANS_|RESWORD_),
  ParKeyWord("MAXEXTENTS",         TOK_MAXEXTENTS,  NONRESTOKEN_),
  ParKeyWord("MAXIMUM",            TOK_MAX,         NONRESTOKEN_),
  ParKeyWord("MAXRUNTIME",         TOK_MAXRUNTIME,  SECOND_|NONRESTOKEN_),
  ParKeyWord("MAXVALUE",           TOK_MAXVALUE,    NONRESTOKEN_),
  ParKeyWord("MD5",                TOK_MD5,         NONRESTOKEN_),
  ParKeyWord("MEMORY",             TOK_MEMORY,      NONRESTOKEN_),
  ParKeyWord("MERGE",              TOK_MERGE,       ANS_|RESWORD_),
  ParKeyWord("MESSAGE",            TOK_MESSAGE,     NONRESTOKEN_),
  ParKeyWord("MESSAGE_LENGTH",     TOK_MESSAGE_LEN, NONRESTOKEN_),
  ParKeyWord("MESSAGE_OCTET_LENGTH", TOK_MESSAGE_OCTET_LEN, NONRESTOKEN_),
  ParKeyWord("MESSAGE_TEXT",       TOK_MESSAGE_TEXT, NONRESTOKEN_),
  ParKeyWord("METADATA",       TOK_METADATA, SECOND_ | NONRESTOKEN_),
  ParKeyWord("MIN",                TOK_MIN,         ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("MINIMAL",            TOK_MINIMAL,     NONRESTOKEN_),
  ParKeyWord("MINUTE",             TOK_MINUTE,      ANS_|RESWORD_),
  ParKeyWord("MINUTES",            TOK_MINUTES,     NONRESTOKEN_),
  ParKeyWord("MINVALUE",           TOK_MINVALUE,   NONRESTOKEN_),
  ParKeyWord("MIXED",              TOK_MIXED,       FLAGSNONE_),
  ParKeyWord("MOD",                TOK_MOD,         NONRESTOKEN_),
  ParKeyWord("MODE",               TOK_MODE,        NONRESTOKEN_),
  ParKeyWord("MODIFIES",           TOK_MODIFIES,    COMPAQ_|RESWORD_),
  ParKeyWord("MODIFY",             IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("MODULE",             TOK_MODULE,      ANS_|RESWORD_),
  ParKeyWord("MODULES",            TOK_MODULES,     NONRESTOKEN_),
  ParKeyWord("MONTH",              TOK_MONTH,       ANS_|RESWORD_),
  ParKeyWord("MONTHNAME",          TOK_MONTHNAME,   NONRESTOKEN_),
  ParKeyWord("MONTHS_BETWEEN",     TOK_MONTHS_BETWEEN,   NONRESTOKEN_),
  ParKeyWord("MORE",               TOK_MORE,        NONRESTOKEN_),
  ParKeyWord("MOVE",               TOK_MOVE,        NONRESTOKEN_),
  ParKeyWord("MOVEMENT",           TOK_MOVEMENT,    NONRESTOKEN_),
  ParKeyWord("MOVINGAVG",          TOK_MAVG,        NONRESTOKEN_),
  ParKeyWord("MOVINGCOUNT",        TOK_MCOUNT,      NONRESTOKEN_),
  ParKeyWord("MOVINGMAX",          TOK_MMAX,        NONRESTOKEN_),
  ParKeyWord("MOVINGMIN",          TOK_MMIN,        NONRESTOKEN_),
  ParKeyWord("MOVINGRANK",         TOK_MRANK,       NONRESTOKEN_),
  ParKeyWord("MSCK",               TOK_MSCK,        NONRESTOKEN_),
  ParKeyWord("MOVINGSTDDEV",       TOK_MSTDDEV,     NONRESTOKEN_),
  ParKeyWord("MOVINGSUM",          TOK_MSUM,        NONRESTOKEN_),
  ParKeyWord("MOVINGVARIANCE",     TOK_MVARIANCE,   NONRESTOKEN_),
  ParKeyWord("MTS",                TOK_MTS,         NONRESTOKEN_),
  ParKeyWord("MULTI",              TOK_MULTI,       NONRESTOKEN_),
  ParKeyWord("MULTIDELTA",         TOK_MULTIDELTA,  FLAGSNONE_),
  ParKeyWord("MULTISET",           TOK_MULTISET,    NONRESTOKEN_),
  ParKeyWord("MV",                 TOK_MV,          FLAGSNONE_),
  ParKeyWord("MVATTRIBUTE",        TOK_MVATTRIBUTE, FLAGSNONE_),
  ParKeyWord("MVATTRIBUTES",       TOK_MVATTRIBUTES,FLAGSNONE_),
  ParKeyWord("MVGROUP",            TOK_MVGROUP,     FLAGSNONE_),
  ParKeyWord("MVGROUPS",           TOK_MVGROUPS,    FLAGSNONE_),
  ParKeyWord("MVLOG",              TOK_MVLOG,       FLAGSNONE_),
  ParKeyWord("MVS",                TOK_MVS,         FLAGSNONE_),
  ParKeyWord("MVSTATUS",           TOK_MVSTATUS,    FLAGSNONE_),
  ParKeyWord("MVSUMD",             TOK_MVS_UMD,     FLAGSNONE_),
  ParKeyWord("MVUID",              TOK_MVUID,       FLAGSNONE_),
  ParKeyWord("MV_TABLE",           TOK_MV_TABLE,    FLAGSNONE_),
  ParKeyWord("NAME",               TOK_NAME,        NONRESTOKEN_),
  ParKeyWord("NAMED",              TOK_NAMED,       NONRESTOKEN_),
  ParKeyWord("NAMES",              TOK_NAMES,       ANS_|RESWORD_),
  ParKeyWord("NAMESPACE",          TOK_NAMESPACE,   NONRESTOKEN_),
  ParKeyWord("NAMETYPE",           TOK_NAMETYPE,    NONRESTOKEN_),
  ParKeyWord("NATIONAL",           TOK_NATIONAL,    ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("NATIVE",             TOK_NATIVE,      NONRESTOKEN_),
  ParKeyWord("NATURAL",            TOK_NATURAL,     ANS_|RESWORD_),
  ParKeyWord("NCHAR",              TOK_NCHAR,       ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("NCLOB",              TOK_NCLOB,       COMPAQ_|RESWORD_),
  ParKeyWord("NECESSARY",          TOK_NECESSARY,   NONRESTOKEN_),
  ParKeyWord("NEEDED",             TOK_NEEDED,      NONRESTOKEN_),
  ParKeyWord("NEW",                TOK_NEW,         ANS_|RESWORD_|NONRESTOKEN_|ALLOWOLDNEW_),
  ParKeyWord("NEXT",               TOK_NEXT,        ANS_|RESWORD_),
  ParKeyWord("NEXT_DAY",           TOK_NEXT_DAY,    NONRESTOKEN_),
  ParKeyWord("NO",                 TOK_NO,          FIRST_|ANS_|RESWORD_),
  ParKeyWord("NODELETE",           TOK_NODELETE,    FLAGSNONE_),
  ParKeyWord("NODES",              TOK_NODES,       NONRESTOKEN_),
  ParKeyWord("NOLOG",              TOK_NOLOG,       NONRESTOKEN_),
  ParKeyWord("NOMVLOG",            TOK_NOMVLOG,     FLAGSNONE_),
  ParKeyWord("NONE",               TOK_NONE,        ANS_|RESWORD_),
  ParKeyWord("NORMAL",             TOK_NORMAL,      NONRESTOKEN_),
  ParKeyWord("NOT",                TOK_NOT,         FIRST_|ANS_|RESWORD_),
  ParKeyWord("NOW",                TOK_NOW,         NONRESTOKEN_),
  ParKeyWord("NSK_CODE",           TOK_NSK_CODE,    NONRESTOKEN_),
  ParKeyWord("NULL",               TOK_NULL,        ANS_|RESWORD_),
  ParKeyWord("NULLABLE",           TOK_NULLABLE,    NONRESTOKEN_),
  ParKeyWord("NULLIF",             TOK_NULLIF,      ANS_|RESWORD_),
  ParKeyWord("NULLIFZERO",         TOK_NULLIFZERO,  NONRESTOKEN_),
  ParKeyWord("NULL_IND_OFFSET",    TOK_NULL_IND_OFFSET, NONRESTOKEN_),
  ParKeyWord("NULL_STRING",        TOK_NULL_STRING, NONRESTOKEN_),
  ParKeyWord("NUMBER",             TOK_NUMBER,      NONRESTOKEN_),
  ParKeyWord("NUMERIC",            TOK_NUMERIC,     ANS_|RESWORD_),
  ParKeyWord("NUM_OF_RANGES",      TOK_NUM_OF_RANGES, FLAGSNONE_),
  ParKeyWord("NVL",                TOK_NVL,         NONRESTOKEN_),
  ParKeyWord("OBJECT",            TOK_OBJECT,     NONRESTOKEN_),
  ParKeyWord("OBJECTS",            TOK_OBJECTS,     NONRESTOKEN_),
  ParKeyWord("OBSOLETE",           TOK_OBSOLETE,    NONRESTOKEN_),
  ParKeyWord("OCTET_LENGTH",       TOK_OCTET_LENGTH, ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("OF",                 TOK_OF,          ANS_|RESWORD_),
  ParKeyWord("OFF",                TOK_OFF,         POTANS_|RESWORD_),
  ParKeyWord("OFFLINE",            TOK_OFFLINE,     NONRESTOKEN_),
  ParKeyWord("OFFSET",             TOK_OFFSET,      NONRESTOKEN_),
  ParKeyWord("OJ",                 TOK_OJ,          NONRESTOKEN_),
  ParKeyWord("OLD",                TOK_OLD,         ANS_|RESWORD_|NONRESTOKEN_|ALLOWOLDNEW_),
  ParKeyWord("ON",                 TOK_ON,          FIRST_|ANS_|RESWORD_),
  ParKeyWord("ONLINE",             TOK_ONLINE,      NONRESTOKEN_),
  ParKeyWord("ONLY",               TOK_ONLY,        THIRD_|ANS_|RESWORD_),
  ParKeyWord("OPCODE",             TOK_OPCODE,      NONRESTOKEN_),
  ParKeyWord("OPEN",               TOK_OPEN,        ANS_|RESWORD_),
  ParKeyWord("OPENBLOWNAWAY",      TOK_OPENBLOWNAWAY, FLAGSNONE_),
  ParKeyWord("OPERATION",          IDENTIFIER,      POTANS_|NONRESWORD_),
  // The word OPERATION is used as a column in the SMD LOCKS table, so
  // we have not made this word reserved even though it is in the
  // potential ANSI reserved word list.

  ParKeyWord("OPERATORS",          IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("OPTION",             TOK_OPTION,      ANS_|RESWORD_),
  ParKeyWord("OPTIONS",            TOK_OPTIONS,     COMPAQ_|RESWORD_),
  ParKeyWord("OR",                 TOK_OR,          ANS_|RESWORD_),
  ParKeyWord("ORDER",              TOK_ORDER,       ANS_|RESWORD_),
  ParKeyWord("ORDERED",            TOK_ORDERED,     NONRESTOKEN_),
  ParKeyWord("ORDINALITY",         TOK_ORDINALITY,  COMPAQ_|RESWORD_),
  ParKeyWord("OSIM",                 TOK_OSIM,    NONRESTOKEN_),
  ParKeyWord("OS_USERID",          TOK_OS_USERID,   NONRESTOKEN_),
  ParKeyWord("OTHERS",             IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("OUT",                TOK_OUT,         COMPAQ_|RESWORD_),
  ParKeyWord("OUTER",              TOK_OUTER,       ANS_|RESWORD_),
  ParKeyWord("OUTPUT",             TOK_OUTPUT,      ANS_|RESWORD_),
  ParKeyWord("OVER",               TOK_OVER,        NONRESTOKEN_),
  ParKeyWord("OVERLAPS",           TOK_OVERLAPS,    ANS_|RESWORD_),
  ParKeyWord("OVERLAY",            TOK_OVERLAY,     NONRESTOKEN_),
  ParKeyWord("OVERRIDE",           TOK_OVERRIDE,    NONRESTOKEN_),
  ParKeyWord("OVERWRITE",          TOK_OVERWRITE,    NONRESTOKEN_),
  ParKeyWord("PACKED",             TOK_PACKED,      NONRESTOKEN_),
  ParKeyWord("PAD",                IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("PAGE",               TOK_PAGE,        NONRESTOKEN_),
  ParKeyWord("PAGES",              TOK_PAGES,       NONRESTOKEN_),
  ParKeyWord("PARALLEL",           TOK_PARALLEL,    NONRESTOKEN_),
  ParKeyWord("PARALLELISM",        TOK_PARALLELISM, NONRESTOKEN_),
  ParKeyWord("PARAMETER",          TOK_PARAMETER,   COMPAQ_|RESWORD_),
  ParKeyWord("PARAMETERS",         IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("PARAMETER_INDEX",    TOK_PARAMETER_INDEX,NONRESTOKEN_),
  ParKeyWord("PARAMETER_MODE",     TOK_PARAMETER_MODE,NONRESTOKEN_),
  ParKeyWord("PARAMETER_ORDINAL_POSITION",TOK_PARAMETER_ORDINAL_POSITION,
                                                    NONRESTOKEN_),
  ParKeyWord("PARENT",             TOK_PARENT,      NONRESTOKEN_),
  ParKeyWord("PARSERFLAGS",        TOK_PARSERFLAGS, NONRESTOKEN_),
  ParKeyWord("PARTIAL",            TOK_PARTIAL,     ANS_|RESWORD_),
  ParKeyWord("PARTITION",          TOK_PARTITION,   FIRST_|SECOND_|NONRESTOKEN_),
  ParKeyWord("PARTITIONING",       TOK_PARTITIONING, NONRESTOKEN_),
  ParKeyWord("PARTITIONS",         TOK_PARTITIONS,  SECOND_|NONRESTOKEN_),
  ParKeyWord("PASS",               TOK_PASS,        NONRESTOKEN_),
  ParKeyWord("PATH",               TOK_PATH,        COMPAQ_|NONRESWORD_),
// PATH is already used in CQS ...
  ParKeyWord("PENDANT",            IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("PERCENT",            TOK_PERCENT,     NONRESTOKEN_),
  ParKeyWord("PERFORM",            TOK_PERFORM,     FIRST_|NONRESTOKEN_),
  ParKeyWord("PERIODIC",           TOK_PERIODIC,    SECOND_|NONRESTOKEN_),
  ParKeyWord("PERTABLE",           TOK_PERTABLE,    NONRESTOKEN_),
  ParKeyWord("PHASE",              TOK_PHASE,       FLAGSNONE_),
  ParKeyWord("PI",                 TOK_PI,          NONRESTOKEN_),
  ParKeyWord("PIC",                TOK_PICTURE,     FIRST_),
  ParKeyWord("PICTURE",            TOK_PICTURE,     FIRST_),
  ParKeyWord("PIPELINE",           TOK_PIPELINE,    FLAGSNONE_),
  ParKeyWord("PID",                TOK_PID,         NONRESTOKEN_),
  ParKeyWord("PIVOT_GROUP",        TOK_PIVOT_GROUP, NONRESTOKEN_),
  ParKeyWord("PIVOT",              TOK_PIVOT,       NONRESTOKEN_),
  ParKeyWord("PLACING",            TOK_PLACING,     NONRESTOKEN_),
  ParKeyWord("POOL",               TOK_POOL,        NONRESTOKEN_),
  ParKeyWord("POPULATE",           TOK_POPULATE,    NONRESTOKEN_),
  ParKeyWord("POSITION",           TOK_POSITION,    ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("POS",                TOK_POS,         NONRESTOKEN_),
  ParKeyWord("POSTFIX",            TOK_POSTFIX,     COMPAQ_|RESWORD_),
  ParKeyWord("POWER",              TOK_POWER,       NONRESTOKEN_),
  ParKeyWord("PRECEDING",          TOK_PRECEDING,   NONRESTOKEN_),
  ParKeyWord("PRECISION",          TOK_PRECISION,   ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("PREFER_FOR_SCAN_KEY", TOK_PREFER_FOR_SCAN_KEY, NONRESTOKEN_),
  //  ParKeyWord("PREFIX",             TOK_PREFIX,      COMPAQ_|RESWORD_),
  ParKeyWord("PREORDER",           IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("PREPARE",            TOK_PREPARE,     ANS_|RESWORD_),
  ParKeyWord("PRESERVE",           TOK_PRESERVE,    ANS_|RESWORD_),
  ParKeyWord("PRIMARY",            TOK_PRIMARY,     FIRST_|ANS_|RESWORD_),
  ParKeyWord("PRIOR",              IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("PRIORITY",           TOK_PRIORITY,    NONRESTOKEN_),
  ParKeyWord("PRIORITY_DELTA",     TOK_PRIORITY_DELTA,    NONRESTOKEN_),
  ParKeyWord("PRIVATE",            TOK_PRIVATE,     NONRESTOKEN_),
  ParKeyWord("PRIVILEGE",          TOK_PRIVILEGE,   NONRESWORD_),
  ParKeyWord("PRIVILEGES",         TOK_PRIVILEGES,  ANS_|RESWORD_),
  ParKeyWord("PROCEDURE",          TOK_PROCEDURE,   ANS_|RESWORD_),
  ParKeyWord("PROCEDURES",         TOK_PROCEDURES,  NONRESTOKEN_),
  ParKeyWord("PROCESS",            TOK_PROCESS,     NONRESTOKEN_),
  ParKeyWord("PROGRESS",           TOK_PROGRESS,    NONRESTOKEN_),
  ParKeyWord("PROMPT",             TOK_PROMPT,      NONRESTOKEN_),
  ParKeyWord("PROTECTED",          TOK_PROTECTED,   POTANS_|RESWORD_),
  ParKeyWord("PROTECTION",         TOK_PROTECTION,  NONRESTOKEN_),  
  ParKeyWord("PROTOTYPE",          TOK_PROTOTYPE,   COMPAQ_|RESWORD_),
  ParKeyWord("PUBLIC",             TOK_PUBLIC,      ANS_|RESWORD_),
  ParKeyWord("PURGEDATA",          TOK_PURGEDATA,   NONRESTOKEN_),
  ParKeyWord("QID",                TOK_QID,         NONRESTOKEN_),
  ParKeyWord("QID_INTERNAL",       TOK_QID_INTERNAL,  NONRESTOKEN_),
  ParKeyWord("QUARTER",            TOK_QUARTER,     NONRESTOKEN_),
  ParKeyWord("QUERY",              TOK_QUERY,       NONRESTOKEN_),
  ParKeyWord("QUERYCACHE",         TOK_QUERY_CACHE, NONRESTOKEN_),
  ParKeyWord("HYBRIDQUERYCACHE",         TOK_HYBRID_QUERY_CACHE, NONRESTOKEN_),
  ParKeyWord("HYBRIDQUERYCACHEENTRIES",     TOK_HYBRID_QUERY_CACHE_ENTRIES, NONRESTOKEN_),
  ParKeyWord("QUERYCACHEENTRIES",  TOK_QUERY_CACHE_ENTRIES, NONRESTOKEN_),
  ParKeyWord("CATMANCACHE",        TOK_CATMAN_CACHE, NONRESTOKEN_),
  ParKeyWord("NATABLECACHE",       TOK_NATABLE_CACHE, NONRESTOKEN_),
  ParKeyWord("NATABLECACHEENTRIES", TOK_NATABLE_CACHE_ENTRIES, NONRESTOKEN_),
  ParKeyWord("NAROUTINECACHE",     TOK_NAROUTINE_CACHE, NONRESTOKEN_),
  ParKeyWord("NAROUTINEACTIONCACHE",TOK_NAROUTINE_ACTION_CACHE, NONRESTOKEN_),
  ParKeyWord("QUALIFY",            TOK_QUALIFY,     CONDITIONAL_RES_ | RESWORD_),
  ParKeyWord("QUERYID_EXTRACT",    TOK_QUERYID_EXTRACT, NONRESTOKEN_),
  ParKeyWord("RADIANS",            TOK_RADIANS,     NONRESTOKEN_),
  ParKeyWord("RAND",               TOK_RAND,        NONRESTOKEN_),
  ParKeyWord("RANDOM",             TOK_RANDOM,      SECOND_|NONRESTOKEN_),
  ParKeyWord("RANGE",              TOK_RANGE,       NONRESTOKEN_),
  ParKeyWord("RANGE_N",            TOK_RANGE_N,     NONRESTOKEN_),
  ParKeyWord("RANGELOG",           TOK_RANGELOG,    FLAGSNONE_),
  ParKeyWord("RANGE_LOG_TABLE",    TOK_RANGE_LOG_TABLE, FLAGSNONE_),
  ParKeyWord("RANK",               TOK_RRANK,       NONRESTOKEN_),
  ParKeyWord("RATE",               TOK_RATE,        NONRESTOKEN_),
  ParKeyWord("READ",               TOK_READ,        SECOND_|ANS_|RESWORD_),
  ParKeyWord("READS",              TOK_READS,       COMPAQ_|RESWORD_),
  ParKeyWord("REAL",               TOK_REAL,        ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("REAL_IEEE",          TOK_REAL_IEEE,   NONRESTOKEN_),
  ParKeyWord("REBUILD",            TOK_REBUILD,     NONRESTOKEN_),
  ParKeyWord("RECOMPUTE",          TOK_RECOMPUTE,   FLAGSNONE_),
  ParKeyWord("RECORD_SEPARATOR",   TOK_RECORD_SEPARATOR, NONRESTOKEN_),
  ParKeyWord("RECOVER",            TOK_RECOVER,     NONRESTOKEN_),
  ParKeyWord("RECOVERY",           TOK_RECOVERY,    NONRESTOKEN_),
  ParKeyWord("RECURSIVE",          TOK_RECURSIVE,      POTANS_|RESWORD_),
  ParKeyWord("REDEFTIME",          TOK_REDEFTIME,   FLAGSNONE_),
  ParKeyWord("REF",                IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("REFERENCES",         TOK_REFERENCES,  ANS_|RESWORD_),
  ParKeyWord("REFERENCING",        TOK_REFERENCING, ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("REFRESH",            TOK_REFRESH,     FLAGSNONE_),
  ParKeyWord("REGEXP",             TOK_REGEXP,      NONRESTOKEN_),
  ParKeyWord("REGION",             TOK_REGION,      NONRESTOKEN_),
  ParKeyWord("REGISTER",           TOK_REGISTER,    NONRESTOKEN_),
  ParKeyWord("REGISTERED",         TOK_REGISTERED,  NONRESTOKEN_),
  ParKeyWord("REINITIALIZE",       TOK_REINITIALIZE, FIRST_|NONRESTOKEN_),
  ParKeyWord("RELATED",            TOK_RELATED,     NONRESTOKEN_),
  ParKeyWord("RELATEDNESS",        TOK_RELATEDNESS, NONRESTOKEN_),
  ParKeyWord("RELATIVE",           IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("RELOAD",             TOK_RELOAD,      NONRESTOKEN_),
  ParKeyWord("REMOTE",             TOK_REMOTE,      NONRESTOKEN_),
  ParKeyWord("REMOVE",             TOK_REMOVE,      FLAGSNONE_),
  ParKeyWord("RENAME",             TOK_RENAME,      NONRESTOKEN_),
  ParKeyWord("REPAIR",             TOK_REPAIR,      NONRESTOKEN_),
  ParKeyWord("REPEAT",             TOK_REPEAT,      NONRESTOKEN_),
  ParKeyWord("REPEATABLE",         TOK_REPEATABLE,  FIRST_|SECOND_|NONRESTOKEN_),
  ParKeyWord("REPLACE",            TOK_REPLACE,     POTANS_|RESWORD_),
  ParKeyWord("REPLICATE",          TOK_REPLICATE,   FIRST_|NONRESTOKEN_),
  ParKeyWord("REPOSITORY",         TOK_REPOSITORY,  NONRESTOKEN_),
  ParKeyWord("REQUEST",            TOK_REQUEST,     FLAGSNONE_),
  ParKeyWord("REQUIRED",           TOK_REQUIRED,    NONRESTOKEN_),
  ParKeyWord("RESET",              TOK_RESET,       NONRESTOKEN_),
  ParKeyWord("RESIGNAL",           IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("RESTORE",            TOK_RESTORE,     NONRESTOKEN_),
  ParKeyWord("RESTRICT",           TOK_RESTRICT,    ANS_|RESWORD_),
  ParKeyWord("RESULT",             TOK_RESULT,      COMPAQ_|RESWORD_),
  ParKeyWord("RESUME",             TOK_RESUME,      NONRESTOKEN_),
  ParKeyWord("RETRIES",            TOK_RETRIES,     NONRESTOKEN_),
  ParKeyWord("RETURN",             TOK_RETURN,      POTANS_|RESWORD_),
  ParKeyWord("RETURNED_LENGTH",    TOK_RETURNED_LENGTH, NONRESTOKEN_),
  ParKeyWord("RETURNED_OCTET_LENGTH", TOK_RETURNED_OCTET_LENGTH, NONRESTOKEN_),
  ParKeyWord("RETURNED_SQLSTATE",  TOK_RETURNED_SQLSTATE, NONRESTOKEN_),
  ParKeyWord("RETURNS",            TOK_RETURNS,     POTANS_|RESWORD_),
  ParKeyWord("REVERSE",            TOK_REVERSE,     NONRESTOKEN_),
  ParKeyWord("REVOKE",             TOK_REVOKE,      ANS_|RESWORD_),
  ParKeyWord("REWRITE",            TOK_REWRITE,     FLAGSNONE_),
  ParKeyWord("RIGHT",              TOK_RIGHT,       ANS_|RESWORD_),
  ParKeyWord("RMS",                TOK_RMS,         NONRESTOKEN_),
  ParKeyWord("ROLE",               TOK_ROLE,        NONRESTOKEN_|SECOND_), 
  ParKeyWord("ROLES",              TOK_ROLES,       NONRESTOKEN_),
  ParKeyWord("ROLLBACK",           TOK_ROLLBACK,    ANS_|RESWORD_),
  ParKeyWord("ROLLUP",             TOK_ROLLUP,      COMPAQ_|RESWORD_),
  ParKeyWord("ROUND",              TOK_ROUND,       NONRESTOKEN_),
  ParKeyWord("ROUNDROBINPARTFUNC", TOK_RRPARTFUNC,  NONRESTOKEN_),
  ParKeyWord("ROUTINE",            IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("ROW",                TOK_ROW,         ANS_|RESWORD_|SECOND_),
  ParKeyWord("ROWNUM",                TOK_ROWNUM,         NONRESTOKEN_),
  ParKeyWord("ROWS",               TOK_ROWS,        ANS_|RESWORD_),
  ParKeyWord("ROWSET",             TOK_ROWSET,      NONRESTOKEN_),
  ParKeyWord("ROWSET_IND_LAYOUT_SIZE",  TOK_ROWSET_IND_LAYOUT_SIZE, NONRESTOKEN_),
  ParKeyWord("ROWSET_SIZE",        TOK_ROWSET_SIZE, NONRESTOKEN_),
  ParKeyWord("ROWSET_VAR_LAYOUT_SIZE",  TOK_ROWSET_VAR_LAYOUT_SIZE, NONRESTOKEN_),
  ParKeyWord("ROWS_COVERED",       TOK_ROWS_COVERED,  FLAGSNONE_),
  ParKeyWord("ROWS_DELETED",       TOK_ROWS_DELETED,  FLAGSNONE_),
  ParKeyWord("ROWS_INSERTED",      TOK_ROWS_INSERTED, FLAGSNONE_),
  ParKeyWord("ROWS_UPDATED",       TOK_ROWS_UPDATED,  FLAGSNONE_),
  ParKeyWord("ROWWISE",            TOK_ROWWISE,     NONRESTOKEN_),
  ParKeyWord("ROW_COUNT",          TOK_ROW_COUNT,   NONRESTOKEN_),
  ParKeyWord("ROW_NUMBER",         TOK_ROW_NUMBER,  NONRESTOKEN_),
  ParKeyWord("RPAD",               TOK_RPAD,        NONRESTOKEN_),
  ParKeyWord("RTRIM",              TOK_RTRIM,       NONRESTOKEN_),
  ParKeyWord("RUN",                TOK_RUN,         NONRESTOKEN_),
  ParKeyWord("RUNNINGAVG",         TOK_RAVG,        NONRESTOKEN_),
  ParKeyWord("RUNNINGCOUNT",       TOK_RCOUNT,      NONRESTOKEN_),
  ParKeyWord("RUNNINGMAX",         TOK_RMAX,        NONRESTOKEN_),
  ParKeyWord("RUNNINGMIN",         TOK_RMIN,        NONRESTOKEN_),
  ParKeyWord("RUNNINGRANK",        TOK_RRANK,       NONRESTOKEN_),
  ParKeyWord("RUNNINGSTDDEV",      TOK_RSTDDEV,     NONRESTOKEN_),
  ParKeyWord("RUNNINGSUM",         TOK_RSUM,        NONRESTOKEN_),
  ParKeyWord("RUNNINGVARIANCE",    TOK_RVARIANCE,   NONRESTOKEN_),
  ParKeyWord("SAFE",               TOK_SAFE,        NONRESTOKEN_),
  ParKeyWord("SALT",               TOK_SALT,        NONRESTOKEN_),
  ParKeyWord("SAS_FORMAT",         TOK_SAS_FORMAT,  NONRESTOKEN_),
  ParKeyWord("SAS_LOCALE",         TOK_SAS_LOCALE,  NONRESTOKEN_),
  ParKeyWord("SAS_MODEL_INPUT_TABLE", TOK_SAS_MODEL_INPUT_TABLE,  NONRESTOKEN_),
  ParKeyWord("SAMPLE",             TOK_SAMPLE,      FIRST_|NONRESTOKEN_),
  ParKeyWord("SAVEPOINT",          IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("SCALAR",             TOK_SCALAR,      NONRESTOKEN_),
  ParKeyWord("SCALE",              TOK_SCALE,       NONRESTOKEN_),
  ParKeyWord("SCAN",               TOK_SCAN,       NONRESTOKEN_),
  ParKeyWord("SCHEMA",             TOK_SCHEMA,      ANS_|RESWORD_),
  ParKeyWord("SCHEMAS",            TOK_SCHEMAS,     NONRESTOKEN_),
  ParKeyWord("SCHEMA_NAME",        TOK_SCHEMA_NAME, NONRESTOKEN_),
  ParKeyWord("SCROLL",             IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("SCOPE",              TOK_SCOPE,       COMPAQ_|RESWORD_),
  ParKeyWord("SERIALIZED",             TOK_SERIALIZED,      NONRESTOKEN_),
  ParKeyWord("SEARCH",             IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("STRING_SEARCH",      TOK_STRING_SEARCH,        NONRESTOKEN_),
  ParKeyWord("SECOND",             TOK_SECOND,      ANS_|RESWORD_),
  ParKeyWord("SECONDS",            TOK_SECONDS,     NONRESTOKEN_),
  ParKeyWord("SECTION",            TOK_SECTION,     ANS_|RESWORD_),
  ParKeyWord("SECURITY",           TOK_SECURITY,    NONRESTOKEN_),  
  ParKeyWord("SEL",                TOK_SEL,         NONRESTOKEN_),
  ParKeyWord("SELECT",             TOK_SELECT,      ANS_|RESWORD_),
  ParKeyWord("SELECTIVITY",        TOK_SELECTIVITY, NONRESTOKEN_),
  ParKeyWord("SENSITIVE",          IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("SEPARATE",           TOK_SEPARATE,    NONRESTOKEN_),
  ParKeyWord("SEPARATOR",          TOK_SEPARATOR,    NONRESTOKEN_),
  ParKeyWord("SEQNUM",           TOK_SEQNUM,    NONRESTOKEN_),
  ParKeyWord("SEQUENCE",           TOK_SEQUENCE,    FIRST_|NONRESTOKEN_|SECOND_),
  ParKeyWord("SEQUENCES",           TOK_SEQUENCES,    NONRESTOKEN_),
  ParKeyWord("SERIALIZABLE",       TOK_SERIALIZABLE, FIRST_|SECOND_|NONRESTOKEN_),
  ParKeyWord("STRINGTOLOB",        TOK_STRINGTOLOB,   NONRESTOKEN_),
  ParKeyWord("STRINGTOEXTERNAL",        TOK_STRINGTOEXTERNAL,   NONRESTOKEN_),
  ParKeyWord("SERVER_NAME",        TOK_SERVER_NAME, NONRESTOKEN_),
  ParKeyWord("SESSION",            TOK_SESSION,     ANS_|RESWORD_),
  ParKeyWord("SESSIONS",           TOK_SESSIONS,    NONRESTOKEN_), 
  ParKeyWord("SESSION_USER",       TOK_SESSION_USER, ANS_|RESWORD_),
  ParKeyWord("SESSN_USR_INTN",     TOK_SESSN_USR_INTN, RESWORD_),
  ParKeyWord("SET",                TOK_SET,         ANS_|RESWORD_),
  ParKeyWord("SETS",               TOK_SETS,        COMPAQ_|RESWORD_),
  ParKeyWord("SG_TABLE",           TOK_SG_TABLE,    NONRESTOKEN_),
  ParKeyWord("SHA",                TOK_SHA,         NONRESTOKEN_),
  ParKeyWord("SHA1",               TOK_SHA1,        NONRESTOKEN_),
  ParKeyWord("SHA2",               TOK_SHA2,        NONRESTOKEN_),
  ParKeyWord("SHAPE",              TOK_SHAPE,       NONRESTOKEN_),
  ParKeyWord("SHARE",              TOK_SHARE,       NONRESTOKEN_),
  ParKeyWord("SHARED",             TOK_SHARED,      NONRESTOKEN_),
  ParKeyWord("SHOW",               TOK_SHOW,        NONRESTOKEN_),
  ParKeyWord("SHOWCONTROL",        TOK_SHOWCONTROL, NONRESTOKEN_),
  ParKeyWord("SHOWDDL",            TOK_SHOWDDL,     NONRESTOKEN_|FIRST_),
  ParKeyWord("SHOWPLAN",           TOK_SHOWPLAN,    NONRESTOKEN_),
  ParKeyWord("SHOWSET",            TOK_SHOWSET,     NONRESTOKEN_),
  ParKeyWord("SHOWSHAPE",          TOK_SHOWSHAPE,   NONRESTOKEN_),
  ParKeyWord("SHOWSTATS",          TOK_SHOWSTATS,   NONRESTOKEN_),
  ParKeyWord("SHOWTRANSACTION",    TOK_SHOWTRANSACTION, NONRESTOKEN_),
  ParKeyWord("SIGN",               TOK_SIGN,        NONRESTOKEN_),
  ParKeyWord("SIGNAL",             TOK_SIGNAL,      COMPAQ_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("SIGNED",             TOK_SIGNED,      NONRESTOKEN_),
  ParKeyWord("SIMILAR",            IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("SIMULATE",        TOK_SIMULATE,    NONRESTOKEN_),
  ParKeyWord("SIN",                TOK_SIN,         NONRESTOKEN_),
  ParKeyWord("SINCE",              TOK_SINCE,       NONRESTOKEN_),
  ParKeyWord("SINGLEDELTA",        TOK_SINGLEDELTA, FLAGSNONE_),
  ParKeyWord("SINH",               TOK_SINH,        NONRESTOKEN_),
  ParKeyWord("SIZE",               TOK_SIZE,        ANS_|RESWORD_),
  ParKeyWord("SKIP",               TOK_SKIP,        FIRST_|SECOND_|NONRESTOKEN_),
  ParKeyWord("SLACK",              TOK_SLACK,        NONRESTOKEN_),
  ParKeyWord("SLEEP",              TOK_SLEEP,        NONRESTOKEN_),
  ParKeyWord("SMALLINT",           TOK_SMALLINT,    ANS_|RESWORD_),
  ParKeyWord("SNAPSHOT",           TOK_SNAPSHOT,    NONRESTOKEN_),
  ParKeyWord("SOFTWARE",           TOK_SOFTWARE,    NONRESTOKEN_),
  ParKeyWord("SOME",               TOK_SOME,        ANS_|RESWORD_),
  ParKeyWord("SOUNDEX",            TOK_SOUNDEX,     NONRESTOKEN_),
  ParKeyWord("SORT",               TOK_SORT,        NONRESTOKEN_),
  ParKeyWord("SORT_KEY",           TOK_SORT_KEY,    NONRESTOKEN_),
  ParKeyWord("SOURCE",             TOK_SOURCE,      NONRESTOKEN_),
  ParKeyWord("SOURCE_FILE",        TOK_SOURCE_FILE, NONRESTOKEN_),
  ParKeyWord("SPACE",              TOK_SPACE,       NONRESTOKEN_),
  ParKeyWord("SPECIFIC",           TOK_SPECIFIC,    COMPAQ_|RESWORD_),
  ParKeyWord("SPECIFICTYPE",       TOK_SPECIFICTYPE, COMPAQ_|RESWORD_),
  ParKeyWord("SPLIT_PART",         TOK_SPLIT_PART, NONRESTOKEN_),
  ParKeyWord("SP_RESULT_SET",      TOK_SP_RESULT_SET, NONRESTOKEN_),
  ParKeyWord("SQL",                TOK_SQL,         ANS_|RESWORD_|SECOND_),
  ParKeyWord("SQLCODE",            TOK_SQLCODE,     ANS_|RESWORD_),
  ParKeyWord("SQLERROR",           TOK_SQLERROR,    ANS_|RESWORD_),
  ParKeyWord("SQLEXCEPTION",       IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("SQLROW",             TOK_SQLROW,      NONRESTOKEN_),
  ParKeyWord("SQLSTATE",           TOK_SQLSTATE,    ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("SQLWARNING",         IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("SQL_BIGINT",         TOK_LARGEINT,    NONRESTOKEN_),
  ParKeyWord("SQL_CHAR",           TOK_CHARACTER,   COMPAQ_|RESWORD_),
  ParKeyWord("SQL_DATE",           TOK_DATE,        COMPAQ_|RESWORD_),
  ParKeyWord("SQL_DECIMAL",        TOK_DECIMAL,     COMPAQ_|RESWORD_),
  ParKeyWord("SQL_DOUBLE",         TOK_SQL_DOUBLE,  COMPAQ_|RESWORD_),
  ParKeyWord("SQL_FLOAT",          TOK_FLOAT,       COMPAQ_|RESWORD_),
  ParKeyWord("SQL_INT",            TOK_INTEGER,     COMPAQ_|RESWORD_),
  ParKeyWord("SQL_INTEGER",        TOK_INTEGER,     COMPAQ_|RESWORD_),
  ParKeyWord("SQL_REAL",           TOK_REAL,        COMPAQ_|RESWORD_),
  ParKeyWord("SQL_SMALLINT",       TOK_SMALLINT,    COMPAQ_|RESWORD_),
  ParKeyWord("SQL_TIME",           TOK_TIME,        COMPAQ_|RESWORD_),
  ParKeyWord("SQL_TIMESTAMP",      TOK_TIMESTAMP,   COMPAQ_|RESWORD_),
  ParKeyWord("SQL_VARCHAR",        TOK_VARCHAR,     COMPAQ_|RESWORD_),
  ParKeyWord("SQL_WARNING",        TOK_SQL_WARNING, NONRESTOKEN_),
  ParKeyWord("SQRT",               TOK_SQRT,        NONRESTOKEN_),
  ParKeyWord("START",              TOK_START,       COMPAQ_|NONRESWORD_),
// used in nist618 test
  ParKeyWord("STATE",              TOK_STATE,       COMPAQ_|NONRESWORD_),
// used in QAT tests
  ParKeyWord("STATEMENT",          TOK_STATEMENT,   NONRESTOKEN_),
  ParKeyWord("STATIC",             TOK_STATIC,      NONRESTOKEN_),
  ParKeyWord("STATISTICS",         TOK_STATISTICS,  NONRESTOKEN_),
  ParKeyWord("STATS",              TOK_STATS,       NONRESTOKEN_),
  ParKeyWord("STATUS",             TOK_STATUS,      NONRESTOKEN_),
  ParKeyWord("STDDEV",             TOK_STDDEV,      NONRESTOKEN_),
  ParKeyWord("STOP",               TOK_STOP,        NONRESTOKEN_),
  ParKeyWord("STORAGE",            TOK_STORAGE,     NONRESTOKEN_),
  ParKeyWord("STORE",              TOK_STORE,       NONRESTOKEN_),
  ParKeyWord("STORED",             TOK_STORED,      NONRESTOKEN_),
  ParKeyWord("STREAM",             TOK_STREAM,      NONRESTOKEN_),
  ParKeyWord("STRUCTURE",          IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("STUFF",              TOK_STUFF,       NONRESTOKEN_),
  ParKeyWord("STYLE",              TOK_STYLE,       NONRESTOKEN_),
  ParKeyWord("SUBCLASS_ORIGIN",    TOK_SUBCLASS_ORIGIN, NONRESTOKEN_),
  ParKeyWord("SUBSTR",             TOK_SUBSTRING,   NONRESTOKEN_),
  ParKeyWord("SUBSTRING",          TOK_SUBSTRING,   ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("SUBSYSTEM_ID",       TOK_SUBSYSTEM_ID, NONRESTOKEN_),
  ParKeyWord("SUFFIX",             TOK_SUFFIX,       NONRESTOKEN_),
  ParKeyWord("SUM",                TOK_SUM,         ANS_|RESWORD_),
  ParKeyWord("SUMMARY",            TOK_SUMMARY,     NONRESTOKEN_),
  ParKeyWord("SUSPEND",            TOK_SUSPEND,     NONRESTOKEN_),
  ParKeyWord("SYNONYM",            TOK_SYNONYM,     POTANS_|RESWORD_),
  ParKeyWord("SYNONYMS",           TOK_SYNONYMS,    NONRESTOKEN_),
  ParKeyWord("SYSDATE",            TOK_SYSDATE,     NONRESTOKEN_),
  ParKeyWord("SYS_GUID",           TOK_SYS_GUID,     NONRESTOKEN_),
  ParKeyWord("SYSTEM",             TOK_SYSTEM,      NONRESTOKEN_),
  ParKeyWord("SYSTIMESTAMP",            TOK_SYSTIMESTAMP,     NONRESTOKEN_),
  ParKeyWord("SYSTEM_USER",        IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("T",                  TOK_T,           NONRESTOKEN_),
  ParKeyWord("TAG",                TOK_TAG,         NONRESTOKEN_),
  ParKeyWord("TABLE",              TOK_TABLE,       ANS_|RESWORD_),
  ParKeyWord("TABLES",             TOK_TABLES,      FLAGSNONE_),
  ParKeyWord("TABLESPACE",             TOK_TABLESPACE,      NONRESTOKEN_),
  ParKeyWord("TABLE_MAPPING",      TOK_TABLE_MAPPING, NONRESTOKEN_),
  ParKeyWord("TABLE_NAME",         TOK_TABLE_NAME,  NONRESTOKEN_),
  ParKeyWord("TAN",                TOK_TAN,         NONRESTOKEN_),
  ParKeyWord("TANH",               TOK_TANH,        NONRESTOKEN_),
  ParKeyWord("TARGET",             TOK_TARGET,      NONRESTOKEN_),
  ParKeyWord("TDMISP",             TOK_INTERNALSP,  NONRESTOKEN_),
  ParKeyWord("TEMPORARY",          TOK_TEMPORARY,   ANS_|RESWORD_),
  ParKeyWord("TEMP_TABLE",         TOK_TEMP_TABLE,     NONRESTOKEN_),
  ParKeyWord("TERMINATE",          TOK_TERMINATE,   COMPAQ_|RESWORD_),
  ParKeyWord("TEST",               IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("TEXT",               TOK_TEXT,        NONRESTOKEN_),
  ParKeyWord("THAN",               TOK_THAN,        COMPAQ_|RESWORD_),
  ParKeyWord("THEN",               TOK_THEN,        ANS_|RESWORD_),
  ParKeyWord("THERE",              IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("THIS",               TOK_THIS,        NONRESTOKEN_),
  ParKeyWord("THROUGH",            TOK_THROUGH,     NONRESTOKEN_),
  ParKeyWord("TIME",               TOK_TIME,        FIRST_|ANS_|RESWORD_),
  ParKeyWord("TIMEOUT",            TOK_TIMEOUT,     NONRESTOKEN_),
  ParKeyWord("TIMESTAMP",          TOK_TIMESTAMP,   ANS_|RESWORD_),
  ParKeyWord("TIMESTAMPADD",       TOK_TIMESTAMPADD,NONRESTOKEN_),
  ParKeyWord("TIMESTAMPDIFF",      TOK_TIMESTAMPDIFF,NONRESTOKEN_),
  ParKeyWord("TIMEZONE_HOUR",      IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("TIMEZONE_MINUTE",    IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("TINYINT",            TOK_TINYINT,     NONRESTOKEN_),
  ParKeyWord("TITLE",              TOK_TITLE,       NONRESTOKEN_),
  ParKeyWord("TO",                 TOK_TO,          ANS_|RESWORD_),
  ParKeyWord("TO_BINARY",          TOK_TO_BINARY,   NONRESTOKEN_),
  ParKeyWord("TO_CHAR",            TOK_TO_CHAR,     NONRESTOKEN_),
  ParKeyWord("TO_DATE",            TOK_TO_DATE,     NONRESTOKEN_),
  ParKeyWord("TO_HEX",             TOK_TO_HEX,      NONRESTOKEN_),
  ParKeyWord("TO_NUMBER",          TOK_TO_NUMBER,   NONRESTOKEN_),
  ParKeyWord("TO_TIME",            TOK_TO_TIME,   NONRESTOKEN_),
  ParKeyWord("TO_TIMESTAMP",       TOK_TO_TIMESTAMP,   NONRESTOKEN_),
  ParKeyWord("TOKENSTR",           TOK_TOKENSTR,    NONRESTOKEN_),
  ParKeyWord("TRAFODION",          TOK_TRAFODION,    NONRESTOKEN_),
  ParKeyWord("TRAILING",           TOK_TRAILING,    ANS_|RESWORD_),
  ParKeyWord("TRANSACTION",        TOK_TRANSACTION, ANS_|RESWORD_),
  ParKeyWord("TRANSFORM",          TOK_TRANSFORM,   NONRESTOKEN_),
  ParKeyWord("TRANSLATE",          TOK_TRANSLATE,   ANS_|RESWORD_),
  ParKeyWord("TRANSLATION",        IDENTIFIER,      ANS_|RESWORD_),
  ParKeyWord("TRANSPOSE",          TOK_TRANSPOSE,   COMPAQ_|RESWORD_),
  ParKeyWord("TREAT",              TOK_TREAT,       COMPAQ_|RESWORD_),
  ParKeyWord("TRIGGER",            TOK_TRIGGER,     ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("TRIGGERS",           TOK_TRIGGERS,    NONRESTOKEN_),
  ParKeyWord("TRIGGER_CATALOG",    TOK_TRIGGER_CATALOG, NONRESTOKEN_),
  ParKeyWord("TRIGGER_NAME",       TOK_TRIGGER_NAME, NONRESTOKEN_),
  ParKeyWord("TRIGGER_SCHEMA",     TOK_TRIGGER_SCHEMA, NONRESTOKEN_),
  ParKeyWord("TRIM",               TOK_TRIM,        ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("TRUE",               TOK_TRUE,        ANS_|RESWORD_),
  ParKeyWord("TRUNC",              TOK_TRUNC,    NONRESTOKEN_),
  ParKeyWord("TRUNCATE",           TOK_TRUNCATE,    NONRESTOKEN_),
  ParKeyWord("TS",                 TOK_TS,          NONRESTOKEN_),
  ParKeyWord("TYPE",               TOK_TYPE,        NONRESTOKEN_),
  ParKeyWord("TYPECAST",               TOK_TYPECAST,        NONRESTOKEN_),
  ParKeyWord("TYPES",              TOK_TYPES,       NONRESTOKEN_),
  ParKeyWord("TYPE_ANSI",          TOK_TYPE_ANSI,   NONRESTOKEN_),
  ParKeyWord("TYPE_FS",            TOK_TYPE_FS,     NONRESTOKEN_),
  ParKeyWord("UCASE",              TOK_UCASE,       NONRESTOKEN_),
  ParKeyWord("UDF",                TOK_UDF,         NONRESTOKEN_),
  ParKeyWord("UID",		   TOK_UID,	    NONRESTOKEN_),
  ParKeyWord("UNAVAILABLE",        TOK_UNAVAILABLE, FLAGSNONE_),
  ParKeyWord("UNBOUNDED",          TOK_UNBOUNDED,   NONRESTOKEN_),
  ParKeyWord("UNCOMMITTED",        TOK_UNCOMMITTED, NONRESTOKEN_),
  ParKeyWord("UNDER",              IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("UNHEX",              TOK_UNHEX,       NONRESTOKEN_),
  ParKeyWord("UNION",              TOK_UNION,       FIRST_|ANS_|RESWORD_),
  ParKeyWord("UNIQUE",             TOK_UNIQUE,      ANS_|RESWORD_),
  ParKeyWord("UNIQUE_ID",             TOK_UNIQUE_ID,      NONRESTOKEN_),
  ParKeyWord("UNIVERSAL",          TOK_UNIVERSAL,   NONRESTOKEN_),
  ParKeyWord("UNIX_TIMESTAMP",     TOK_UNIX_TIMESTAMP,   NONRESTOKEN_),
  ParKeyWord("UNKNOWN",            TOK_UNKNOWN,     ANS_|RESWORD_),
  ParKeyWord("UNLOAD",             TOK_UNLOAD,      NONRESTOKEN_),
  ParKeyWord("UNLOCK",             TOK_UNLOCK,      NONRESTOKEN_),
  ParKeyWord("UNNAMED",            TOK_UNNAMED,     NONRESTOKEN_),
  ParKeyWord("UNNEST",             TOK_UNNEST,      COMPAQ_|RESWORD_),
  ParKeyWord("UNREGISTER",         TOK_UNREGISTER,  NONRESTOKEN_),
  ParKeyWord("UNSIGNED",           TOK_UNSIGNED,    NONRESTOKEN_),
  ParKeyWord("UPDATE",             TOK_UPDATE,      FIRST_|ANS_|RESWORD_),
  ParKeyWord("UPD",                TOK_UPD,         NONRESTOKEN_),
  ParKeyWord("UPDATE_STATS",       TOK_UPDATE_STATS,	  NONRESTOKEN_),
  ParKeyWord("UPGRADE",                TOK_UPGRADE,         NONRESTOKEN_),
  ParKeyWord("UPPER",              TOK_UPPER,       ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("UPPERCASE",          TOK_UPPERCASE,   NONRESTOKEN_),
  ParKeyWord("UPSERT",              TOK_UPSERT,       NONRESTOKEN_),
  ParKeyWord("UPSHIFT",            TOK_UPSHIFT,     COMPAQ_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("USA",                TOK_USA,         NONRESTOKEN_),
  ParKeyWord("USAGE",              TOK_USAGE,       ANS_|RESWORD_),
  ParKeyWord("USE",                TOK_USE,         FLAGSNONE_),
  ParKeyWord("USER",               TOK_USER,        ANS_|RESWORD_|SECOND_),
  ParKeyWord("USERNAME_INT_TO_EXT",TOK_USERNAMEINTTOEXT,  NONRESTOKEN_),
  ParKeyWord("USERS",              TOK_USERS,       NONRESTOKEN_),
  ParKeyWord("USING",              TOK_USING,       ANS_|RESWORD_),
  ParKeyWord("UUID",                TOK_UUID,         NONRESTOKEN_),
  ParKeyWord("UUID_SHORT",          TOK_UUID_SHORT,	NONRESTOKEN_),
  ParKeyWord("VALIDATE",	   TOK_VALIDATE,    NONRESTOKEN_),
  ParKeyWord("VALUE",              TOK_VALUE,       NONRESTOKEN_),
  ParKeyWord("VALUES",             TOK_VALUES,      ANS_|RESWORD_),
  ParKeyWord("VARBINARY",          TOK_VARBINARY,   NONRESTOKEN_),
  ParKeyWord("VARCHAR",            TOK_VARCHAR,     ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("VARCHAR2",            TOK_VARCHAR,     ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("VARIABLE",           IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("VARIABLE_DATA",      TOK_VARIABLE_DATA, NONRESTOKEN_),
  ParKeyWord("VARIABLE_POINTER",   TOK_VARIABLE_POINTER, NONRESTOKEN_),
  ParKeyWord("VARIANCE",           TOK_VARIANCE,    NONRESTOKEN_),
  ParKeyWord("VARWCHAR",           TOK_VARWCHAR,    NONRESTOKEN_),
  ParKeyWord("VARYING",            TOK_VARYING,     ANS_|RESWORD_|NONRESTOKEN_),
  ParKeyWord("VERIFY",             TOK_VERIFY,      NONRESTOKEN_),
  ParKeyWord("VERSION",            TOK_VERSION,     NONRESTOKEN_),
  ParKeyWord("VERSIONS",            TOK_VERSIONS,     NONRESTOKEN_),
  ParKeyWord("VERSION_INFO",       TOK_VERSION_INFO, NONRESTOKEN_),
  ParKeyWord("VIEW",               TOK_VIEW,        ANS_|RESWORD_),
  ParKeyWord("VIEWS",              TOK_VIEWS,       NONRESTOKEN_),
  ParKeyWord("VIRTUAL",            IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("VISIBLE",            IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("VOLATILE",           TOK_VOLATILE,    NONRESTOKEN_),
  ParKeyWord("VPROC",              TOK_VPROC,       NONRESTOKEN_),
  ParKeyWord("VSBB",               TOK_VSBB,        NONRESTOKEN_),
  ParKeyWord("WAIT",               IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("WAITED",             TOK_WAITED,      NONRESTOKEN_),
  ParKeyWord("WAITEDIO",           TOK_WAITEDIO,      NONRESTOKEN_),
  ParKeyWord("WCHAR",              TOK_WCHAR,       NONRESTOKEN_),
  ParKeyWord("WEEK",               TOK_WEEK,        NONRESTOKEN_),
  ParKeyWord("WHEN",               TOK_WHEN,        ANS_|RESWORD_),
  ParKeyWord("WHENEVER",           TOK_WHENEVER,    ANS_|RESWORD_),
  ParKeyWord("WHERE",              TOK_WHERE,       ANS_|RESWORD_),
  ParKeyWord("WHILE",              IDENTIFIER,      POTANS_|RESWORD_),
  ParKeyWord("WITH",               TOK_WITH,        SECOND_|ANS_|RESWORD_),
  ParKeyWord("WITHOUT",            TOK_WITHOUT,     SECOND_|POTANS_|RESWORD_),
  ParKeyWord("WORK",               TOK_WORK,        ANS_|RESWORD_),
  ParKeyWord("WOM",                TOK_WOM,         NONRESTOKEN_),
  ParKeyWord("WRITE",              TOK_WRITE,       ANS_|RESWORD_),
  ParKeyWord("XMLAGG",               TOK_XMLAGG,        NONRESTOKEN_),
  ParKeyWord("XMLELEMENT",               TOK_XMLELEMENT,        NONRESTOKEN_),
  ParKeyWord("YEAR",               TOK_YEAR,        ANS_|RESWORD_),
  ParKeyWord("ZEROIFNULL",         TOK_ZEROIFNULL,  NONRESTOKEN_),
  ParKeyWord("ZONE",               IDENTIFIER,      ANS_|RESWORD_)
};

// The maximum length of all the keywords.  This is used to be able to
// allocate a buffer large enough to hold any keyword.  This value is
// checked (once) at runtime to make sure it is still valid.
//
#define MAX_KEYWORD_LENGTH 30

// The static keyword for an IDENTIFIER.  Used when a given keyword
// was not found in the keyword table.
//
ParKeyWord ParKeyWords::identWord_("IDENT", IDENTIFIER, FLAGSNONE_);

// The number of entries in the keyword table.
// set once by initKeyWordTable().
//
size_t ParKeyWords::numEntries_(0);

// The maximum length of all the keywords.
// set once by initKeyWordTable().
//
size_t ParKeyWords::maxKeyWordLength_(0);

// Indicates if the keyword table has been sorted.
// FALSE - not sorted
// TRUE - sorted
//
// Initialized to FALSE (not sorted) and set to TRUE (sorted)
// by initKeyWordTable()
//
NABoolean ParKeyWords::keyWordTableSorted_(FALSE);

// Copy Constructor for ParKeyWord
//
ParKeyWord::ParKeyWord(const ParKeyWord &other, NAMemory * h)
  : keyword_(other.keyword_),
    tokenCode_(other.tokenCode_),
    flags_(other.flags_)
{}

// Basic constructor for a ParKeyWord.
//
ParKeyWord::ParKeyWord(const char *kwd,
                       Int32 code,
                       UInt32 flags,
                       NAMemory * h)
  : keyword_(kwd),
    tokenCode_(code),
    flags_(flags)
{
}

// Constructor for ParKeyWords.
//
ParKeyWords::ParKeyWords(NAMemory * h)
{
  initKeyWordTable();
}

// Copy Constructor for ParKeyWords
// (no data to copy)
//
ParKeyWords::ParKeyWords(const ParKeyWords &other, NAMemory * h)
{
  initKeyWordTable();
}

// ParKeyWord::useAsIdentifierInNonSpecialMode() =====================
// Do we use this word only as a reserved word in special modes 
// (otherwise as an identifier)
// ===================================================================
//
NABoolean 
ParKeyWord::useAsIdentifierInNonSpecialMode() const 
{    
  NABoolean useAsIdentifier = FALSE;

  if( !( SqlParser_CurrentParser->modeSpecial1() 
         || 
         CmpCommon::getDefault(COMP_BOOL_200) == DF_ON ) )
  {
    switch(tokenCode_)
    {
    case TOK_QUALIFY:
      useAsIdentifier = TRUE;
      break;
    default: 
      /* default is already FALSE */
      break;
    };  
  }
  return useAsIdentifier;
}

// ParKeyWords::initKeyWordTable() ====================================
// Initialize the key word table.  This will load in any changes to
// the table and sort it if these things have not already been done.
// ====================================================================
//
void
ParKeyWords::initKeyWordTable()
{
  // make sure keyword table is sorted and all static data has been
  // initialized. Do this only once.
  //
  if (!keyWordTableSorted_) {
    keyWordTableSorted_ = TRUE;

    // The number of entries in the keyword table.
    // (needed by qsort and bsearch).
    //
    numEntries_ = sizeof(keyWords_)/sizeof(ParKeyWord);

    // sort the keyword table.
    //
    qsort(keyWords_, numEntries_, sizeof(ParKeyWord), keyCompare);

    // Determine the max length of the keywords.
    // And ensure that all the words are in uppercase.
    //

    for(UInt32 i = 0; i < numEntries_; i++) {

      const char *kword = keyWords_[i].getKeyWord();
      UInt32 len = str_len(kword);

      if (maxKeyWordLength_ < len) {
         maxKeyWordLength_ = len;
      }

      for (UInt32 l = 0; l < len; l++) {
        ComASSERT(isUpper8859_1(kword[l]) || !isAlpha8859_1(kword[l]));
      }

    }

    // Add one for the NULL char.
    //
    maxKeyWordLength_++;

    // Make sure that the equivalent #define is still valid.
    //
    ComASSERT(maxKeyWordLength_ <= MAX_KEYWORD_LENGTH);

  }
} // ParKeyWords::initKeyWordTable()

// ParKeyWords::keyCompare() ===========================================
// key comparison function for keywordTable binary search
// returns: negative if keyval < datum (table entry)
//          zero if keyval == datum
//          positive if keyval > datum
// =====================================================================
//
Int32 ParKeyWords::keyCompare(const void *keyval, const void *datum)
{
  // Cast the (void *) pointers to the (ParKeyWord *) which they must
  // be.
  //
  ParKeyWord *kval = (ParKeyWord *)keyval;
  ParKeyWord *entry = (ParKeyWord *)datum;

  // If either entry is NULL or has a NULL keyword, return 0 (equal).
  //
  ComASSERT(kval && kval->getKeyWord() && entry && entry->getKeyWord());

  // case-sensitive comparison
  // (all words are uppercase)
  //
  Int32 len1 = str_len(kval->getKeyWord());
  Int32 len2 = str_len(entry->getKeyWord());

  Int32 ok = 0;

  if ( len1 == len2 )
    return  str_cmp(kval->getKeyWord(), entry->getKeyWord(), len1);
  else
    ok = strcmp(kval->getKeyWord(), entry->getKeyWord());
 
  if ( ok > 0 ) return 1;
  if ( ok < 0 ) return -1;
  
  return 0;
}

// normalizeKeyWord() ================================================
// static helper routine used by lookupKeyWord.  Used to narrow a wide
// string to a narrow string and convert to uppercase .  Note that all
// keywords can be represented as narrow words.
//
// wWord (IN) - a pointer to the WIDE word to be narrowed.
//
// word (IN) - a pointer to an allocated buffer used to hold the
//             narrow word.
//
// size (IN) - the size of the buffer (word).
//
// Returns boolean indicating if the word was successfully normalized.
// It will be unsuccessful if the word contains characters that can
// not be represented as a narrow character or if the size of the
// buffer is not large enough to hold the normalized word.
// ===================================================================
//
static
NABoolean normalizeKeyWord(NAWchar *wWord, char *word, UInt32 size)
{

  UInt32 i = 0;
  while (wWord[i]) {

    // If the Word is bigger than the buffer return FALSE.
    //
    if (i >= size - 1) {
      return FALSE;
    }

    // Convert the wide character to a regular character.
    //
    word[i] = (char)wWord[i];

    // Convert it back to the wide char.  If it is not the same,
    // then data was lost in the conversion.
    //
    if (((NAWchar)word[i] ) != wWord[i]) {
      return FALSE;
    }

    // Upper case all words since all the words in the keyword table
    // are also uppercase.
    //
    word[i] = TOUPPER(word[i]);

    i++;
  }
  word[i] = '\0';

  return TRUE;
}

// ParKeyWords::lookupKeyWord() ======================================
// Look up the narrowed version of the given word in the keyword
// table.  If found return a const pointer to the ParKeyWord
// object. If the word is too large, or cannot be narrowed, or is not
// found in the keyword table, return a const pointer to the static
// identWord_.
// ===================================================================
//
const
ParKeyWord *ParKeyWords::lookupKeyWord(NAWchar *id)
{

  // Buffer to hold the narrowed version of the given word.
  //
  char word[MAX_KEYWORD_LENGTH];

  // Narrow the given word.  If this is successful, search for the
  // word in the keyword table
  //
  if(normalizeKeyWord(id, word, maxKeyWordLength_)) {

    // search for word in the keyword table
    //
    ParKeyWord key(word, 0, 0);
    const ParKeyWord *entry = searchKeyWordTbl(&key);

    // If the word was found, return a const pointer to the ParKeyWord
    // entry.
    //
    if (entry &&
        (!entry->isConditionallyReserved() ||
         !entry->useAsIdentifierInNonSpecialMode())) {      
      return entry;
    }
  }

  // If 'id' could not be narrowed or it could not be found in the
  // keyword table, then it must be an identifier.  Return a pointer
  // to the static identifier entry.
  //
  return &identWord_;
}

