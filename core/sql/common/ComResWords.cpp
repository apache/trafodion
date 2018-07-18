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
* File:         ComResWords.cpp
* Description:  All reserved words including those defined in ANSI 5.2,
*               Used by NAString (isSqlReservedWord)
* Created:      2000-07-21
*******************************************************************************
*/

#include "ComASSERT.h"
#include "ComResWords.h"
#include "str.h"

// The reserved word table:

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!! ALL entries must be uppercase and in ascending sorted order!!!!!!!!

#define ComResWord(rwd, flags) {rwd, flags}

const ComResWord ComResWords::resWords_[] = {
  ComResWord("ABSOLUTE",         ANS_|NONRESWORD_),
  // ABSOLUTE is a NONRESWORD_ for now (97/11/14) because
  // MicroStrategy is heavily using ABSOLUTE. Since we are not using
  // it yet, it is temporarily removed from the reserved word list
  // until MicroStrategy removes ABSOLUTE. It is too much work for
  // them to do that now.
  ComResWord("ACTION",           ANS_|RESWORD_),
  ComResWord("ADD",              ANS_|RESWORD_),
  ComResWord("ADMIN",            COMPAQ_|RESWORD_),
  ComResWord("AFTER",            COMPAQ_|RESWORD_),
  ComResWord("AGGREGATE",        COMPAQ_|RESWORD_),
  ComResWord("ALIAS",            POTANS_|RESWORD_),
  ComResWord("ALL",              ANS_|RESWORD_),
  ComResWord("ALLOCATE",         ANS_|RESWORD_),
  ComResWord("ALTER",            ANS_|RESWORD_),
  ComResWord("AND",              ANS_|RESWORD_),
  ComResWord("ANY",              ANS_|RESWORD_),
  ComResWord("ARE",              ANS_|RESWORD_),
  ComResWord("ARRAY",            COMPAQ_|RESWORD_),
  ComResWord("AS",               ANS_|RESWORD_),
  ComResWord("ASC",              ANS_|RESWORD_),
  ComResWord("ASSERTION",        ANS_|RESWORD_),
  ComResWord("ASYNC",            POTANS_|RESWORD_),
  ComResWord("AT",               ANS_|RESWORD_),
  ComResWord("AUTHORIZATION",    ANS_|RESWORD_),
  ComResWord("AVG",              ANS_|RESWORD_),
  ComResWord("BEFORE",           POTANS_|RESWORD_),
  ComResWord("BEGIN",            ANS_|RESWORD_),
  ComResWord("BETWEEN",          ANS_|RESWORD_),
  ComResWord("BINARY",           COMPAQ_|RESWORD_),
  ComResWord("BIT",              ANS_|RESWORD_),
  ComResWord("BIT_LENGTH",       ANS_|RESWORD_),
  ComResWord("BLOB",             COMPAQ_|RESWORD_),
  ComResWord("BOOLEAN",          POTANS_|RESWORD_),
  ComResWord("BOTH",             ANS_|RESWORD_),
  ComResWord("BREADTH",          POTANS_|RESWORD_),
  ComResWord("BY",               ANS_|RESWORD_),
  ComResWord("CALL",             POTANS_|RESWORD_),
  ComResWord("CASCADE",          ANS_|RESWORD_),
  ComResWord("CASCADED",         ANS_|RESWORD_),
  ComResWord("CASE",             ANS_|RESWORD_),
  ComResWord("CAST",             ANS_|RESWORD_),
  ComResWord("CATALOG",          ANS_|RESWORD_),
  ComResWord("CHAR",             ANS_|RESWORD_),
  ComResWord("CHARACTER",        ANS_|RESWORD_),
  ComResWord("CHARACTER_LENGTH", ANS_|RESWORD_),
  ComResWord("CHAR_LENGTH",      ANS_|RESWORD_),
  ComResWord("CHECK",            ANS_|RESWORD_),
  ComResWord("CLASS",            COMPAQ_|RESWORD_),
  ComResWord("CLOB",             COMPAQ_|RESWORD_),
  ComResWord("CLOSE",            ANS_|RESWORD_),
  ComResWord("COALESCE",         ANS_|RESWORD_),
  ComResWord("COLLATE",          ANS_|RESWORD_),
  ComResWord("COLLATION",        ANS_|RESWORD_),
  ComResWord("COLUMN",           ANS_|RESWORD_),
  ComResWord("COMMIT",           ANS_|RESWORD_),
  ComResWord("COMPLETION",       POTANS_|RESWORD_),
  ComResWord("CONNECT",          ANS_|RESWORD_),
  //  ComResWord("CONNECTION",       ANS_|RESWORD_),
  ComResWord("CONSTRAINT",       ANS_|RESWORD_),
  ComResWord("CONSTRAINTS",      ANS_|RESWORD_),
  ComResWord("CONSTRUCTOR",      COMPAQ_|RESWORD_),
  ComResWord("CONTINUE",         ANS_|RESWORD_),
  ComResWord("CONVERT",          ANS_|RESWORD_),
  ComResWord("CORRESPONDING",    ANS_|RESWORD_),
  ComResWord("COUNT",            ANS_|RESWORD_),
  ComResWord("CREATE",           ANS_|RESWORD_),
  ComResWord("CROSS",            ANS_|RESWORD_),
  ComResWord("CUBE",             COMPAQ_|RESWORD_),
  ComResWord("CURRENT",          ANS_|RESWORD_),
  ComResWord("CURRENT_DATE",     ANS_|RESWORD_),
  ComResWord("CURRENT_PATH",     COMPAQ_|RESWORD_),
  ComResWord("CURRENT_TIME",     ANS_|RESWORD_),
  ComResWord("CURRENT_TIMESTAMP",ANS_|RESWORD_),
  ComResWord("CURRENT_USER",     ANS_|RESWORD_),
  ComResWord("CURSOR",           ANS_|RESWORD_),
  ComResWord("CYCLE",            POTANS_|RESWORD_),
  ComResWord("DATA",             POTANS_|NONRESWORD_),
  // The word DATA is used in several places in mxcmp (ISP and RFORK),
  // and is in SqlParser.y's "nonreserved_word" list, so we have not
  // made it reserved even though it is in the potential ANSI reserved
  // word list.

  ComResWord("DATE",             ANS_|RESWORD_),
  ComResWord("DATETIME",         COMPAQ_|RESWORD_),
  ComResWord("DAY",              ANS_|RESWORD_),
  ComResWord("DEALLOCATE",       ANS_|RESWORD_),
  ComResWord("DEC",              ANS_|RESWORD_),
  ComResWord("DECIMAL",          ANS_|RESWORD_),
  ComResWord("DECLARE",          ANS_|RESWORD_),
  ComResWord("DEFAULT",          ANS_|RESWORD_),
  ComResWord("DEFERRABLE",       ANS_|RESWORD_),
  ComResWord("DEFERRED",         ANS_|RESWORD_),
  ComResWord("DELETE",           ANS_|RESWORD_),
  ComResWord("DEPTH",            POTANS_|RESWORD_),
  ComResWord("DEREF",            COMPAQ_|RESWORD_),
  ComResWord("DESC",             ANS_|RESWORD_),
  ComResWord("DESCRIBE",         ANS_|RESWORD_),
  ComResWord("DESCRIPTOR",       ANS_|RESWORD_),
  ComResWord("DESTROY",          COMPAQ_|RESWORD_),
  ComResWord("DESTRUCTOR",       COMPAQ_|RESWORD_),
  ComResWord("DETERMINISTIC",    COMPAQ_|RESWORD_),
  ComResWord("DIAGNOSTICS",      ANS_|RESWORD_),
  ComResWord("DICTIONARY",       POTANS_|RESWORD_),
  ComResWord("DISCONNECT",       ANS_|RESWORD_),
  ComResWord("DISTINCT",         ANS_|RESWORD_),
  ComResWord("DOMAIN",           ANS_|RESWORD_),
  ComResWord("DOUBLE",           ANS_|RESWORD_),
  ComResWord("DROP",             ANS_|RESWORD_),
  ComResWord("DYNAMIC",          COMPAQ_|RESWORD_),
  ComResWord("EACH",             POTANS_|RESWORD_),
  ComResWord("ELSE",             ANS_|RESWORD_),
  ComResWord("ELSEIF",           POTANS_|RESWORD_),
  ComResWord("END",              ANS_|RESWORD_),
  ComResWord("END-EXEC",         ANS_|RESWORD_),
  ComResWord("EQUALS",           POTANS_|RESWORD_),
  ComResWord("ESCAPE",           ANS_|RESWORD_),
  ComResWord("EXCEPT",           ANS_|RESWORD_),
  ComResWord("EXCEPTION",        ANS_|RESWORD_),
  ComResWord("EXEC",             ANS_|RESWORD_),
  ComResWord("EXECUTE",          ANS_|RESWORD_),
  ComResWord("EXISTS",           ANS_|RESWORD_),
  ComResWord("EXTERNAL",         ANS_|RESWORD_),
  ComResWord("EXTRACT",          ANS_|RESWORD_),
  ComResWord("FALSE",            ANS_|RESWORD_),
  ComResWord("FETCH",            ANS_|RESWORD_),
  ComResWord("FIRST",            ANS_|RESWORD_),
  ComResWord("FLOAT",            ANS_|RESWORD_),
  ComResWord("FOR",              ANS_|RESWORD_),
  ComResWord("FOREIGN",          ANS_|RESWORD_),
  ComResWord("FOUND",            ANS_|RESWORD_),
  ComResWord("FRACTION",         COMPAQ_|RESWORD_),
  ComResWord("FREE",             COMPAQ_|RESWORD_),
  ComResWord("FROM",             ANS_|RESWORD_),
  ComResWord("FULL",             ANS_|RESWORD_),
  ComResWord("FUNCTION",         COMPAQ_|RESWORD_),
  ComResWord("GENERAL",          POTANS_|RESWORD_),
  ComResWord("GET",              ANS_|RESWORD_),
  ComResWord("GLOBAL",           ANS_|RESWORD_),
  ComResWord("GO",               ANS_|RESWORD_),
  ComResWord("GOTO",             ANS_|RESWORD_),
  ComResWord("GRANT",            ANS_|RESWORD_),
  ComResWord("GROUP",            ANS_|RESWORD_),
  ComResWord("GROUPING",         COMPAQ_|RESWORD_),
  ComResWord("HAVING",           ANS_|RESWORD_),
  ComResWord("HOST",             COMPAQ_|RESWORD_),
  ComResWord("HOUR",             ANS_|RESWORD_),
  ComResWord("IDENTITY",         ANS_|RESWORD_),
  ComResWord("IF",               POTANS_|RESWORD_),
  ComResWord("IGNORE",           POTANS_|RESWORD_),
  ComResWord("IMMEDIATE",        ANS_|RESWORD_),
  ComResWord("IN",               ANS_|RESWORD_),
  ComResWord("INDICATOR",        ANS_|RESWORD_),
  ComResWord("INITIALLY",        ANS_|RESWORD_),
  ComResWord("INNER",            ANS_|RESWORD_),
  ComResWord("INOUT",            COMPAQ_|RESWORD_),
  ComResWord("INPUT",            ANS_|RESWORD_),
  ComResWord("INSENSITIVE",      ANS_|RESWORD_),
  ComResWord("INSERT",           ANS_|RESWORD_),
  ComResWord("INT",              ANS_|RESWORD_),
  ComResWord("INTEGER",          ANS_|RESWORD_),
  ComResWord("INTERSECT",        ANS_|RESWORD_),
  ComResWord("INTERVAL",         ANS_|RESWORD_),
  ComResWord("INTO",             ANS_|RESWORD_),
  ComResWord("IS",               ANS_|RESWORD_),
  ComResWord("ISOLATION",        ANS_|RESWORD_),
  ComResWord("ITERATE",          COMPAQ_|RESWORD_),
  ComResWord("JOIN",             ANS_|RESWORD_),
  ComResWord("KEY",              ANS_|RESWORD_),
  ComResWord("LANGUAGE",         ANS_|RESWORD_),
  ComResWord("LARGE",            COMPAQ_|RESWORD_),
  ComResWord("LAST",             ANS_|RESWORD_),
  ComResWord("LATERAL",          COMPAQ_|RESWORD_),
  ComResWord("LEADING",          ANS_|RESWORD_),
  ComResWord("LEAVE",            POTANS_|RESWORD_),
  ComResWord("LEFT",             ANS_|RESWORD_),
  ComResWord("LESS",             POTANS_|RESWORD_),
  ComResWord("LEVEL",            ANS_|RESWORD_),
  ComResWord("LIKE",             ANS_|RESWORD_),
  ComResWord("LIMIT",            POTANS_|RESWORD_),
  ComResWord("LOCAL",            ANS_|RESWORD_),
  ComResWord("LOCALTIME",        COMPAQ_|RESWORD_),
  ComResWord("LOCALTIMESTAMP",   COMPAQ_|RESWORD_),
  ComResWord("LOCATOR",          COMPAQ_|RESWORD_),
  ComResWord("LOOP",             POTANS_|RESWORD_),
  ComResWord("LOWER",            ANS_|RESWORD_),
  ComResWord("MATCH",            ANS_|RESWORD_),
  ComResWord("MAX",              ANS_|RESWORD_),
  ComResWord("MIN",              ANS_|RESWORD_),
  ComResWord("MINUTE",           ANS_|RESWORD_),
  ComResWord("MODIFIES",         COMPAQ_|RESWORD_),
  ComResWord("MODIFY",           POTANS_|RESWORD_),
  ComResWord("MODULE",           ANS_|RESWORD_),
  ComResWord("MONTH",            ANS_|RESWORD_),
  ComResWord("NAMES",            ANS_|RESWORD_),
  ComResWord("NATIONAL",         ANS_|RESWORD_),
  ComResWord("NATURAL",          ANS_|RESWORD_),
  ComResWord("NCHAR",            ANS_|RESWORD_),
  ComResWord("NCLOB",            COMPAQ_|RESWORD_),
  ComResWord("NEXT",             ANS_|RESWORD_),
  ComResWord("NO",               ANS_|RESWORD_),
  ComResWord("NONE",             POTANS_|RESWORD_),
  ComResWord("NOT",              ANS_|RESWORD_),
  ComResWord("NULL",             ANS_|RESWORD_),
  ComResWord("NULLIF",           ANS_|RESWORD_),
  ComResWord("NUMERIC",          ANS_|RESWORD_),
  ComResWord("OBJECT",           POTANS_|RESWORD_),
  ComResWord("OCTET_LENGTH",     ANS_|RESWORD_),
  ComResWord("OF",               ANS_|RESWORD_),
  ComResWord("OFF",              POTANS_|RESWORD_),
  ComResWord("ON",               ANS_|RESWORD_),
  ComResWord("ONLY",             ANS_|RESWORD_),
  ComResWord("OPEN",             ANS_|RESWORD_),
  ComResWord("OPERATION",        POTANS_|NONRESWORD_),
  // The word OPERATION is used as a column in the SMD LOCKS table, so
  // we have not made this word reserved even though it is in the
  // potential ANSI reserved word list.

  ComResWord("OPERATORS",        POTANS_|RESWORD_),
  ComResWord("OPTION",           ANS_|RESWORD_),
  ComResWord("OR",               ANS_|RESWORD_),
  ComResWord("ORDER",            ANS_|RESWORD_),
  ComResWord("ORDINALITY",       COMPAQ_|RESWORD_),
  ComResWord("OTHERS",           POTANS_|RESWORD_),
  ComResWord("OUT",              COMPAQ_|RESWORD_),
  ComResWord("OUTER",            ANS_|RESWORD_),
  ComResWord("OUTPUT",           ANS_|RESWORD_),
  ComResWord("OVERLAPS",         ANS_|RESWORD_),
  ComResWord("PAD",              ANS_|RESWORD_),
  ComResWord("PARAMETER",        COMPAQ_|RESWORD_),
  ComResWord("PARAMETERS",       POTANS_|RESWORD_),
  ComResWord("PARTIAL",          ANS_|RESWORD_),
  ComResWord("PATH",             COMPAQ_|NONRESWORD_),
// already being used by CQS ...
  ComResWord("PENDANT",          POTANS_|RESWORD_),
  ComResWord("POSITION",         ANS_|RESWORD_),
  ComResWord("POSTFIX",          COMPAQ_|RESWORD_),
  ComResWord("PRECISION",        ANS_|RESWORD_),
  //  ComResWord("PREFIX",           COMPAQ_|RESWORD_),
  ComResWord("PREORDER",         POTANS_|RESWORD_),
  ComResWord("PREPARE",          ANS_|RESWORD_),
  ComResWord("PRESERVE",         ANS_|RESWORD_),
  ComResWord("PRIMARY",          ANS_|RESWORD_),
  ComResWord("PRIOR",            ANS_|RESWORD_),
  ComResWord("PRIVATE",          POTANS_|RESWORD_),
  ComResWord("PRIVILEGES",       ANS_|RESWORD_),
  ComResWord("PROCEDURE",        ANS_|RESWORD_),
  ComResWord("PROTECTED",        POTANS_|RESWORD_),
  ComResWord("PROTOTYPE",        COMPAQ_|RESWORD_),
  ComResWord("PUBLIC",           ANS_|RESWORD_),
  ComResWord("READ",             ANS_|RESWORD_),
  ComResWord("READS",            COMPAQ_|RESWORD_),
  ComResWord("REAL",             ANS_|RESWORD_),
  ComResWord("RECURSIVE",        POTANS_|RESWORD_),
  ComResWord("REF",              POTANS_|RESWORD_),
  ComResWord("REFERENCES",       ANS_|RESWORD_),
  ComResWord("REFERENCING",      POTANS_|RESWORD_),
  ComResWord("RELATIVE",         ANS_|RESWORD_),
  ComResWord("REPLACE",          POTANS_|RESWORD_),
  ComResWord("RESIGNAL",         POTANS_|RESWORD_),
  ComResWord("RESTRICT",         ANS_|RESWORD_),
  ComResWord("RESULT",           COMPAQ_|RESWORD_),
  ComResWord("RETURN",           POTANS_|RESWORD_),
  ComResWord("RETURNS",          POTANS_|RESWORD_),
  ComResWord("REVOKE",           ANS_|RESWORD_),
  ComResWord("RIGHT",            ANS_|RESWORD_),
  ComResWord("ROLLBACK",         ANS_|RESWORD_),
  ComResWord("ROLLUP",           COMPAQ_|RESWORD_),
  ComResWord("ROUTINE",          POTANS_|RESWORD_),
  ComResWord("ROW",              ANS_|RESWORD_),
  ComResWord("ROWS",             ANS_|RESWORD_),
  ComResWord("SAVEPOINT",        POTANS_|RESWORD_),
  ComResWord("SCHEMA",           ANS_|RESWORD_),
  ComResWord("SCOPE",            COMPAQ_|RESWORD_),
  ComResWord("SCROLL",           ANS_|RESWORD_),
  ComResWord("SEARCH",           POTANS_|RESWORD_),
  ComResWord("SECOND",           ANS_|RESWORD_),
  ComResWord("SECTION",          ANS_|RESWORD_),
  ComResWord("SELECT",           ANS_|RESWORD_),
  ComResWord("SENSITIVE",        POTANS_|RESWORD_),
  ComResWord("SESSION",          ANS_|RESWORD_),
  ComResWord("SESSION_USER",     ANS_|RESWORD_),
  ComResWord("SET",              ANS_|RESWORD_),
  ComResWord("SETS",             COMPAQ_|RESWORD_),
  ComResWord("SIGNAL",           POTANS_|RESWORD_),
  ComResWord("SIMILAR",          POTANS_|RESWORD_),
  ComResWord("SIZE",             ANS_|RESWORD_),
  ComResWord("SMALLINT",         ANS_|RESWORD_),
  ComResWord("SOME",             ANS_|RESWORD_),
  ComResWord("SPECIFIC",         COMPAQ_|RESWORD_),
  ComResWord("SPECIFICTYPE",     COMPAQ_|RESWORD_),
  ComResWord("SQL",              ANS_|RESWORD_),
  ComResWord("SQLCODE",          ANS_|RESWORD_),
  ComResWord("SQLERROR",         ANS_|RESWORD_),
  ComResWord("SQLEXCEPTION",     POTANS_|RESWORD_),
  ComResWord("SQLSTATE",         ANS_|RESWORD_),
  ComResWord("SQLWARNING",       POTANS_|RESWORD_),
  ComResWord("SQL_CHAR",         COMPAQ_|RESWORD_),
  ComResWord("SQL_DATE",         COMPAQ_|RESWORD_),
  ComResWord("SQL_DECIMAL",      COMPAQ_|RESWORD_),
  ComResWord("SQL_DOUBLE",       COMPAQ_|RESWORD_),
  ComResWord("SQL_FLOAT",        COMPAQ_|RESWORD_),
  ComResWord("SQL_INT",          COMPAQ_|RESWORD_),
  ComResWord("SQL_INTEGER",      COMPAQ_|RESWORD_),
  ComResWord("SQL_REAL",         COMPAQ_|RESWORD_),
  ComResWord("SQL_SMALLINT",     COMPAQ_|RESWORD_),
  ComResWord("SQL_TIME",         COMPAQ_|RESWORD_),
  ComResWord("SQL_TIMESTAMP",    COMPAQ_|RESWORD_),
  ComResWord("SQL_VARCHAR",      COMPAQ_|RESWORD_),
  ComResWord("START",            COMPAQ_|NONRESWORD_),
// used in nist618 as column name 
  ComResWord("STATE",            COMPAQ_|NONRESWORD_),
// used in qat tests
  ComResWord("STRUCTURE",        POTANS_|RESWORD_),
  ComResWord("SUBSTRING",        ANS_|RESWORD_),
  ComResWord("SUM",              ANS_|RESWORD_),
  ComResWord("SYSTEM_USER",      ANS_|RESWORD_),
  ComResWord("TABLE",            ANS_|RESWORD_),
  ComResWord("TEMPORARY",        ANS_|RESWORD_),
  ComResWord("TERMINATE",        COMPAQ_|RESWORD_),
  ComResWord("TEST",             POTANS_|RESWORD_),
  ComResWord("THAN",             COMPAQ_|RESWORD_),
  ComResWord("THEN",             ANS_|RESWORD_),
  ComResWord("THERE",            POTANS_|RESWORD_),
  ComResWord("TIME",             ANS_|RESWORD_),
  ComResWord("TIMESTAMP",        ANS_|RESWORD_),
  ComResWord("TIMEZONE_HOUR",    ANS_|RESWORD_),
  ComResWord("TIMEZONE_MINUTE",  ANS_|RESWORD_),
  ComResWord("TO",               ANS_|RESWORD_),
  ComResWord("TRAILING",         ANS_|RESWORD_),
  ComResWord("TRANSACTION",      ANS_|RESWORD_),
  ComResWord("TRANSLATE",        ANS_|RESWORD_),
  ComResWord("TRANSLATION",      ANS_|RESWORD_),
  ComResWord("TRANSPOSE",        COMPAQ_|RESWORD_),
  ComResWord("TREAT",            COMPAQ_|RESWORD_),
  ComResWord("TRIGGER",          POTANS_|RESWORD_),
  ComResWord("TRIM",             ANS_|RESWORD_),
  ComResWord("TRUE",             ANS_|RESWORD_),
  ComResWord("UNDER",            POTANS_|RESWORD_),
  ComResWord("UNION",            ANS_|RESWORD_),
  ComResWord("UNIQUE",           ANS_|RESWORD_),
  ComResWord("UNKNOWN",          ANS_|RESWORD_),
  ComResWord("UNNEST",           COMPAQ_|RESWORD_),
  ComResWord("UPDATE",           ANS_|RESWORD_),
  ComResWord("UPPER",            ANS_|RESWORD_),
  ComResWord("UPSERT",           RESWORD_),
  ComResWord("UPSHIFT",          COMPAQ_|RESWORD_),
  ComResWord("USAGE",            ANS_|RESWORD_),
  ComResWord("USER",             ANS_|RESWORD_),
  ComResWord("USING",            ANS_|RESWORD_),
  ComResWord("VALUES",           ANS_|RESWORD_),
  ComResWord("VARCHAR",          ANS_|RESWORD_),
  ComResWord("VARIABLE",         POTANS_|RESWORD_),
  ComResWord("VARYING",          ANS_|RESWORD_),
  ComResWord("VIEW",             ANS_|RESWORD_),
  ComResWord("VIRTUAL",          POTANS_|RESWORD_),
  ComResWord("VISIBLE",          POTANS_|RESWORD_),
  ComResWord("WAIT",             POTANS_|RESWORD_),
  ComResWord("WHEN",             ANS_|RESWORD_),
  ComResWord("WHENEVER",         ANS_|RESWORD_),
  ComResWord("WHERE",            ANS_|RESWORD_),
  ComResWord("WHILE",            POTANS_|RESWORD_),
  ComResWord("WITH",             ANS_|RESWORD_),
  ComResWord("WITHOUT",          POTANS_|RESWORD_),
  ComResWord("WORK",             ANS_|RESWORD_),
  ComResWord("WRITE",            ANS_|RESWORD_),
  ComResWord("YEAR",             ANS_|RESWORD_),
  ComResWord("ZONE",             ANS_|RESWORD_)
};

// The maximum length of all the reserved words.  This is used to be
// able to allocate a buffer large enough to hold any word.  This
// value is checked (once) at runtime to make sure it is still valid.
//
#define MAX_RESWORD_LENGTH 25

// Copy Constructor for ComResWords
// (no data to copy)
//
ComResWords::ComResWords(const ComResWords &other, NAMemory * h)
{
}


// ComResWords::wordCompare() ===========================================
// comparison function for reserved word Table binary search
// returns: negative if val1 < val2 (table entry)
//          zero if val1 == val2
//          positive if val1 > val2
// =====================================================================
//
Int32 ComResWords::wordCompare(const void *val1, const void *val2)
{
  // Cast the (void *) pointers to the (ComResWord *) which they must
  // be.
  //
  ComResWord *val = (ComResWord *)val1;
  ComResWord *entry = (ComResWord *)val2;

  // If either entry is NULL or has a NULL word, return 0 (equal).
  //
  ComASSERT(val && val->getResWord() && entry && entry->getResWord());

  // case-sensitive comparison
  // (all words are uppercase)
  //
  Int32 len1 = str_len(val->getResWord());
  Int32 len2 = str_len(entry->getResWord());

  Int32 maxlen = (len1 > len2) ? len1 : len2;
 
  return str_cmp(val->getResWord(), entry->getResWord(), maxlen);
}

// ComResWords::isSqlReservedWord() ================================
// Determine if the given word is a reserved identifier.
// =================================================================
//
NABoolean
ComResWords::isSqlReservedWord(const char *word,
			       UInt32 ifSetFlags)
{
  char uword[MAX_RESWORD_LENGTH];

  NABoolean lookup = TRUE;
  UInt32 i = 0; 
  while (word[i]) {
    if(i >= (MAX_RESWORD_LENGTH - 1)) {
      lookup = FALSE;
      break;
    }
    // Upper case all words since all the words in the resword table
    // are also uppercase.
    //
    uword[i] = TOUPPER(word[i]);
    i++;
  }
  uword[i] = '\0';

  if (lookup) {
    // search for uppercase word in the reserved word table
    //
    ComResWord resWord = { uword, 0 };
    //resWord.setResWord(uword, 0);
    ComResWord *entry = searchResWordTbl(&resWord);

    return (entry && entry->isReserved(ifSetFlags));
  } else {
    return FALSE;
  }
}

ComResWord * ComResWords::binarySearch(ComResWord *val)
{
   Int32 lower = 0;
   size_t numEntries = sizeof(resWords_)/sizeof(ComResWord);

   Int32 upper = numEntries -1;
   short result = 0;
   while ( lower <= upper ) 
   {
    Int32 middle = (lower+upper) >> 1;
    result = wordCompare(&(resWords_[middle]), val);

    if ( result == 0 )
       return (ComResWord*)(&(resWords_[middle]));

    if ( result < 0 )
      lower = middle + 1;
    else
      upper = middle - 1;
    }
 return val; 
}






