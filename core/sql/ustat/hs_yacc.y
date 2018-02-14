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
%{
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_yacc.y
 * Description:  Parser rules for UPDATE STATISTICS.
 *
 *
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#define HS_FILE "hs_yacc"

#include <math.h>

#   include <alloca.h>

#include "CompException.h"
#include "hs_globals.h"
#include "hs_parser.h"


extern "C" { void yyerror(void* scanner, const char *sb); };

union YYSTYPE;

extern THREAD_P HSGlobalsClass *hs_globals_y;

extern int yylex(YYSTYPE * lvalp, void* scanner);
//extern int yylex();

#define scanner scanner

%}

%lex-param {void * scanner}
%parse-param {void * scanner}
%define api.pure full

%union
{
  Int64           tokval;
  double          tokdbl;
  NAString       *stringval;
  HSColumnStruct *colinfo;
  HSColSet       *colset;
} 

%start statement
%token <stringval> REGULAR_IDENTIFIER DELIMITED_IDENTIFIER GUARDIAN_TABLE_NAME DEFINE_NAME
%token <tokval> INTEGER_NUMBER
%token <tokdbl> APPROX_NUMBER

%type  <colinfo> column_name
%type  <colset> multi_column_list
%type  <stringval> identifier
%type  <tokval> int_number
%type  <tokdbl> real_number

%token  SHOWSTATS DETAIL UPDATE STATISTICS FOR LOG TABLE
%token  ON TOK_OFF EVERY COLUMN KEY TO CLEAR VIEWONLY GENERATE INTERVALS
%token  TOK_SET ROWCOUNT SAMPLE ROWS RANDOM PERIODIC TOK_PERCENT CLUSTERS BLOCKS OF
%token  EXISTING COLUMNS NECESSARY CREATE REMOVE ALL WITH SKEWED VALUES 
%token  INCREMENTAL WHERE WHERE_CONDITION PERSISTENT NO
%token  SYSTEM
%%

/*
   NOTE: Any syntax change affecting SHOWSTATS, including changes to on_clause,
         must be duplicated in parser/sqlparser.y. A preliminary parse of the
         SHOWSTATS statement is performed there, and if it does not contain the
         same syntax it will fail to parse there and never get here.
*/

statement   :  UPDATE STATISTICS { hs_globals_y->isUpdatestatsStmt = TRUE; } FOR table_kind table_identifier histogram_options
                   {
                      hs_globals_y->isUpdatestatsStmt = FALSE;
                   }
              | UPDATE STATISTICS LOG ON
                   {
                     HSLogMan *LM = HSLogMan::Instance();
                     LM->SetLogSetting(HSLogMan::ON);
                     hs_globals_y->optFlags |= LOG_OPT;
                   }
              | UPDATE STATISTICS LOG TOK_OFF
                   {
                     HSLogMan *LM = HSLogMan::Instance();
                     LM->SetLogSetting(HSLogMan::OFF);
                     hs_globals_y->optFlags |= LOG_OPT;
                   }
              | UPDATE STATISTICS LOG SYSTEM
                   {
                     HSLogMan *LM = HSLogMan::Instance();
                     LM->SetLogSetting(HSLogMan::SYSTEM);
                     hs_globals_y->optFlags |= LOG_OPT;
                   }
              | UPDATE STATISTICS LOG CLEAR
                   {
                     HSLogMan *LM = HSLogMan::Instance();
                     LM->ClearLog();
                     hs_globals_y->optFlags |= LOG_OPT;
                   }
              | SHOWSTATS FOR table_kind table_identifier showstats_options
	           {
                     hs_globals_y->optFlags |= SHOWSTATS_OPT;
                   }  
;

column_name :  identifier
                   {

                     HSColumnStruct *hist_pt = new(STMTHEAP) HSColumnStruct;

                     *hist_pt->colname = $1->data();

                     $$ = hist_pt;
                   }
;

table_kind :
              LOG TABLE
                  {
                    hs_globals_y->nameSpace = COM_IUD_LOG_TABLE_NAME;
                  }
              | TABLE
                  {
                    hs_globals_y->nameSpace = COM_TABLE_NAME;
                  }
;

table_identifier : identifier
                   {
                     if (AddTableName(ANSI_TABLE, $1->data()))
                       {
                         hs_globals_y->parserError =
                           HSGlobalsClass::ERROR_SEMANTICS;
                         return -1;
                       }
                   }
            |  identifier '.' identifier
                   {
                     if (AddTableName(ANSI_TABLE, $3->data(), $1->data()))
                       {
                          hs_globals_y->parserError =
                           HSGlobalsClass::ERROR_SEMANTICS;
                         return -1;
                       }
                   }
            |  identifier '.' identifier '.' identifier
                   {
                     if (AddTableName(ANSI_TABLE, $5->data(), $3->data(), $1->data()))
                       {
                         hs_globals_y->parserError =
                           HSGlobalsClass::ERROR_SEMANTICS;
                         return -1;
                       }
                   }
;

/* In addition to the token types for regular and delimited identifiers, the
   'identifier' nonterminal can expand to a number of keywords that are used
   with the UPDATE STATISTICS syntax. This makes it possible to use as identifiers
   certain tokens that are identified by the lexer as keywords. Otherwise these
   keywords would have to be made reserved words.
*/
identifier :   REGULAR_IDENTIFIER
                   {
                     $$ = $1;
                   }
            |  DELIMITED_IDENTIFIER
                   {
                     $$ = $1;
                   }
            |  ALL
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "ALL";
                     $$ = buf;
                   }
            |  BLOCKS
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "BLOCKS";
                     $$ = buf;
                   }
            |  CLEAR
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "CLEAR";
                     $$ = buf;
                   }
            |  CLUSTERS
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "CLUSTERS";
                     $$ = buf;
                   }
            |  COLUMNS
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "COLUMNS";
                     $$ = buf;
                   }
            |  CREATE
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "CREATE";
                     $$ = buf;
                   }
            |  DETAIL
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "DETAIL";
                     $$ = buf;
                   }
            |  EVERY
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "EVERY";
                     $$ = buf;
                   }
            |  EXISTING
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "EXISTING";
                     $$ = buf;
                   }
            |  GENERATE
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "GENERATE";
                     $$ = buf;
                   }
            |  INCREMENTAL
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "INCREMENTAL";
                     $$ = buf;
                   }
            |  INTERVALS
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "INTERVALS";
                     $$ = buf;
                   }
            |  LOG
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "LOG";
                     $$ = buf;
                   }
            |  NECESSARY
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "NECESSARY";
                     $$ = buf;
                   }
            |  PERIODIC
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "PERIODIC";
                     $$ = buf;
                   }
            |  RANDOM
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "RANDOM";
                     $$ = buf;
                   }
            |  ROWCOUNT
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "ROWCOUNT";
                     $$ = buf;
                   }
            |  REMOVE
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "REMOVE";
                     $$ = buf;
                   }
            |  PERSISTENT
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "PERSISTENT";
                     $$ = buf;
                   }
            |  SAMPLE
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "SAMPLE";
                     $$ = buf;
                   }
            |  SHOWSTATS
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "SHOWSTATS";
                     $$ = buf;
                   }
            |  STATISTICS
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "STATISTICS";
                     $$ = buf;
                   }
            |  SYSTEM
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "SYSTEM";
                     $$ = buf;
                   }
            |  TOK_PERCENT
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "PERCENT";
                     $$ = buf;
                   }
            |  VIEWONLY
                   {
                     NAString *buf = new(STMTHEAP) NAString(STMTHEAP);
                     *buf = "VIEWONLY";
                     $$ = buf;
                   }
;

showstats_options :   on_clause
                    | on_clause DETAIL
                   {
                     hs_globals_y->optFlags |= DETAIL_OPT;
                   }
;

histogram_options : CLEAR
                      {
                        hs_globals_y->optFlags |= CLEAR_OPT;
                      }
                 |  CREATE SAMPLE random_clause
                      {
                        if (hs_globals_y->optFlags & SAMPLE_RAND_2)
                          {
                            HSFuncMergeDiags(-UERR_IUS_WRONG_RANDOM);
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        hs_globals_y->optFlags |= CREATE_SAMPLE_OPT;

                        // read the column groups so we can truncate columns if needed
                        hs_globals_y->optFlags |= EVERYCOL_OPT;
                        if (Lng32 retcode = AddEveryColumn())
                          HSHandleError(retcode);
                      }
                 |  REMOVE SAMPLE
                      {
                        hs_globals_y->optFlags |= REMOVE_SAMPLE_OPT;
                      }
                 |  on_clause_wrapper
                 |  on_clause CLEAR
                      {
                        hs_globals_y->optFlags |= CLEAR_OPT;
                      }
                 |  on_clause_wrapper interval_clause
                 |  on_clause_wrapper incremental_clause
                 |  on_clause_wrapper sample_clause
                 |  on_clause_wrapper interval_clause sample_clause
                 |  on_clause_wrapper sample_clause interval_clause
                 |  on_clause_wrapper with_skewed_values_clause
                 |  VIEWONLY
                    {
                      hs_globals_y->optFlags |= VIEWONLY_OPT;
                    }
                 | /* empty */
;


on_clause :         ON predefined_groups
                 |  ON predefined_groups ',' regular_group_list
                 |  ON regular_group_list
;

on_clause_wrapper : on_clause
                    {
                      // If neither "on every key" nor "on every column" (which includes
                      // every key) is specified, and the table is salted, we check to see
                      // if there are any groups (SC or MC) that constitute a (possibly
                      // improper) prefix of the primary key minus its initial "_SALT_" column.
                      // If so, we add a group consisting of that group prefixed by "_SALT_".
                      // Also, if the column(s) of an MC group constitute a leading prefix of
                      // the primary key including "_SALT_", we make sure the group is added
                      // with the columns in index order.
                      if (!(hs_globals_y->optFlags & (EVERYKEY_OPT | EVERYCOL_OPT)) &&
                          hs_globals_y->objDef->getColNum("_SALT_", FALSE) >= 0)
                        {
                          AddSaltToIndexPrefixes();
                        }
                    }
;

incremental_clause :   INCREMENTAL WHERE WHERE_CONDITION
                { 
                       HSLogMan *LM = HSLogMan::Instance();
                       if (LM->LogNeeded() )
                         LM->Log("incremental clause identified");
                           
                       if (CmpCommon::getDefault(USTAT_INCREMENTAL_UPDATE_STATISTICS) == DF_OFF) {
                         HSFuncMergeDiags(-UERR_IUS_IS_DISABLED);
                       }

                      if (hs_globals_y->optFlags & (REG_GROUP_OPT | EVERYCOL_OPT | EVERYKEY_OPT ))
                        HSFuncMergeDiags(-UERR_WRONG_ON_CLAUSE_FOR_IUS, "INCREMENTAL");
                      else
                        {
                          // This check is here to make sure we cover all the possible
                          // ON clause alternatives
                          if (!(hs_globals_y->optFlags & (EXISTING_OPT | NECESSARY_OPT)))
                             HSFuncMergeDiags(-UERR_WRONG_ON_CLAUSE_FOR_IUS, "INCREMENTAL");
                          else
                             hs_globals_y->optFlags |= IUS_OPT;
                        }
                }
;



interval_clause :   GENERATE int_number INTERVALS
                      {
                        if (($2 <= 0) || ($2 > HS_MAX_INTERVAL_COUNT))
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "INTERVALS",
                                             "an integer between 1 and 10000");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }
                        hs_globals_y->intCount = int64ToInt32($2);
                        hs_globals_y->optFlags |= INTERVAL_OPT;
                      }
;

sample_clause : sample_clause_init sample_clause_body

              | NO SAMPLE
                 {
                   // explicit NO SAMPLE, to override error 
                   // UERR_YOU_WILL_LIKELY_BE_SORRY on large tables
                   hs_globals_y->optFlags |= NO_SAMPLE;
                 }
;

sample_clause_init : { 
                       // Need this in case NECESSARY clause caused sampling parameters
                       // to be set. If a sampling clause is present, we give priority
                       // to the parameters specified by the user.
                       hs_globals_y->optFlags &= ~SAMPLE_REQUESTED; 
                       hs_globals_y->sampleValue1 = 0;
                       hs_globals_y->sampleValue2 = 0;

                       HSLogMan *LM = HSLogMan::Instance();
                       if (LM->LogNeeded() && hs_globals_y->optFlags & NECESSARY_OPT)
                         // Preserve the text used in this log message; it is searched for
                         // by certain ustat regression tests.
                         LM->Log("Sampling for NECESSARY: overridden by user-specified SAMPLE clause");
                     }
;

sample_clause_body : SAMPLE
                      {
                        hs_globals_y->optFlags |= SAMPLE_BASIC_0;
                      }
                 |  SAMPLE rowcount_clause
                      {
                        hs_globals_y->optFlags |= SAMPLE_BASIC_0;
                      }
                 |  SAMPLE int_number ROWS
                      {
                        if ($2 < 0)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "SAMPLE ROWS",
                                             "an integer greater than or equal to 0 and within limits");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }
                        hs_globals_y->optFlags |= SAMPLE_BASIC_1;
                        hs_globals_y->sampleValue1 = $2;
                      }
                 |  SAMPLE int_number ROWS rowcount_clause
                      {
                        if ($2 < 0)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "SAMPLE ROWS",
                                             "an integer greater than or equal to 0 and within limits");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        if ($2 > hs_globals_y->actualRowCount)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "SAMPLE ROWS",
                                             "less than or equal to ROWCOUNT");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        hs_globals_y->optFlags |= SAMPLE_BASIC_1;
                        hs_globals_y->sampleValue1 = $2;
                      }
                 |  SAMPLE random_clause
                 |  SAMPLE random_clause PERSISTENT
                      {
                        HSLogMan *LM = HSLogMan::Instance();
                        if (LM->LogNeeded() )
                          LM->Log("Creation of persistent sample table for IUS requested");

                        // Formerly there was code here to limit PERSISTENT to 
                        // ON EXISTING COLUMNS and ON NECESSARY COLUMNS (EXISTING_OPT and
                        // NECESSARY_OPT in the optFlags). But there doesn't seem to be
                        // a compelling reason for this limitation. The persistent sample
                        // table will have all the columns of the base table regardless
                        // of the ON clause.

                        if (hs_globals_y->optFlags & SAMPLE_RAND_2)
                          HSFuncMergeDiags(-UERR_IUS_WRONG_RANDOM);
                        else
                          hs_globals_y->optFlags |= IUS_PERSIST;
                      }
                 |  SAMPLE random_clause rowcount_clause
                 |  SAMPLE periodic_clause
                 |  SAMPLE periodic_clause rowcount_clause
;

rowcount_clause :   TOK_SET ROWCOUNT int_number
                      {
                        if ($3 < 0)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "ROWCOUNT",
                                             "an integer greater than or equal to 0 and within limits");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }
                        hs_globals_y->actualRowCount = $3;
                        hs_globals_y->optFlags |= ROWCOUNT_OPT;
                      }
;

random_clause :     RANDOM real_number TOK_PERCENT
                      {
                        if ($2 < 0 || $2 > 100)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "PERCENT",
                                             "a value between 0 and 100");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        // Check cqd that may request converting random sampling to 
                        // periodic, to make test results deterministic.
                        if (CmpCommon::getDefault(USTAT_IUS_USE_PERIODIC_SAMPLING) == DF_ON)
                          {
                            // Use periodic 1 every n rows, where n is determined by
                            // the specified percentage.
                            hs_globals_y->optFlags |= SAMPLE_PERIODIC;
                            hs_globals_y->sampleValue1 = 1;
                            hs_globals_y->sampleValue2 = (Int64)(1000000 / ($2 * 10000) + .5);
                            
                            HSLogMan* LM = HSLogMan::Instance();
                            if (LM->LogNeeded())
                              {
                                sprintf(LM->msg,
                                        "PERIODIC (%d every %d) substituted for RANDOM (%f%%) sampling",
                                        (Int32)hs_globals_y->sampleValue1,
                                        (Int32)hs_globals_y->sampleValue2,
                                        $2);
                                LM->Log(LM->msg);
                              }
                          }
                        else
                          {
                            hs_globals_y->optFlags |= SAMPLE_RAND_1;
                            hs_globals_y->sampleValue1 = (Int64)($2 * 10000);
                          }
                      }
                 |  RANDOM real_number TOK_PERCENT CLUSTERS OF int_number BLOCKS
                      {
                        if ($2 < 0 || $2 > 100)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "PERCENT",
                                             "a value between 0 and 100");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        if ($6 < 0)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "BLOCKS",
                                             "an integer greater than or equal to 0 and within limits");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        hs_globals_y->optFlags |= SAMPLE_RAND_2;
                        hs_globals_y->sampleValue1 = (Int64)($2 * 10000);
                        hs_globals_y->sampleValue2 = $6;
                      }
;

periodic_clause :   PERIODIC int_number ROWS EVERY int_number ROWS
                      {
                        if ($2 < 0)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "SAMPLE SIZE",
                                             "an integer greater than or equal to 0 and within limits");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        if ($5 < 0)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "SAMPLE PERIOD",
                                             "an integer greater than or equal to 0 and within limits");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        if ($2 > $5)
                          {
                            HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                             "SAMPLE SIZE",
                                             "less than or equal to SAMPLE PERIOD");
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }

                        hs_globals_y->optFlags |= SAMPLE_PERIODIC;
                        hs_globals_y->sampleValue1 = $2;
                        hs_globals_y->sampleValue2 = $5;
                      }
;

with_skewed_values_clause : WITH SKEWED VALUES
                            {
                              HSColGroupStruct *mgroup = hs_globals_y->multiGroup;
                              while(mgroup)
                              {
                                mgroup->skewedValuesCollected = TRUE;
                                mgroup = mgroup->next;
                              }
                            }
;

predefined_groups : EVERY COLUMN
                      {
                        Lng32 retcode = 0;

                        if (COM_IUD_LOG_TABLE_NAME == hs_globals_y->nameSpace)
                          {
                            HSFuncMergeDiags(- UERR_EVERY_COLUMN_NOT_ALLOWED_FOR_LOG);
                            hs_globals_y->parserError = HSGlobalsClass::ERROR_SEMANTICS;
                            return -1;
                          }
                        else
                          {
                            hs_globals_y->optFlags |= EVERYCOL_OPT;
                            if (retcode = AddEveryColumn())
                              HSHandleError(retcode);
                          }
                      }
                 |  EVERY KEY
                      {
                        Lng32 retcode = 0;
                        hs_globals_y->optFlags |= EVERYKEY_OPT;
                        if (retcode = AddKeyGroups())
                          HSHandleError(retcode);
                      }
                 |  EXISTING columns_spec
                      {
                        Lng32 retcode = 0;
                        hs_globals_y->optFlags |= EXISTING_OPT;
                        if (retcode = AddExistingColumns())
                          HSHandleError(retcode);
                      }
                 | NECESSARY columns_spec
                      {
                        // Set flag indicating use of NECESSARY.
                        hs_globals_y->optFlags |= NECESSARY_OPT;
                      }
;

columns_spec :      COLUMN 
                 |  COLUMNS
;

regular_group :     '(' multi_column_list ')'
                      {
                        Lng32 retcode;

                        if (retcode = AddColumnSet(*$2))
                          {
                            delete $2;
                            $2 = NULL;
                            HSHandleError(retcode);
                          }
                      }
                 |  '(' identifier ')'
                      {
                        Lng32 retcode = 0;

                        if (retcode = AddEveryColumn($2->data(), $2->data()))
                          HSHandleError(retcode);
                      }
                 |  identifier
                      {
                        Lng32 retcode = 0;

                        if (retcode = AddEveryColumn($1->data(), $1->data()))
                          HSHandleError(retcode);
                      }
                 |  '(' identifier ')' TO '(' identifier ')'
                      {
                        Lng32 retcode = 0;

                        if (retcode = AddEveryColumn($2->data(), $6->data()))
                          HSHandleError(retcode);
                      }
                 |  '(' identifier ')' TO identifier
                      {
                        Lng32 retcode = 0;

                        if (retcode = AddEveryColumn($2->data(), $5->data()))
                          HSHandleError(retcode);
                      }
                 |  identifier TO '(' identifier ')'
                      {
                        Lng32 retcode = 0;

                        if (retcode = AddEveryColumn($1->data(), $4->data()))
                          HSHandleError(retcode);
                      }
                 |  identifier TO identifier
                      {
                        Lng32 retcode = 0;

                        if (retcode = AddEveryColumn($1->data(), $3->data()))
                          HSHandleError(retcode);
                      }
;

regular_group_list : regular_group
                       {
                         hs_globals_y->optFlags |= REG_GROUP_OPT;
                       }
                 |   regular_group ',' regular_group_list
;

multi_column_list : column_name
                      {
                        $$ = new (STMTHEAP) HSColSet (STMTHEAP);
                        Lng32 colnum = hs_globals_y->objDef->getColNum($1->colname->data());
                        if (colnum < 0)
                          {
                            Lng32 retcode = -1;
                            HSHandleError(retcode);
                          }
                        *$1 = hs_globals_y->objDef->getColInfo(colnum);
                        $1->colnum = colnum;
                        $$->insert(*$1);
                      }
                 |  column_name ',' multi_column_list
                    {
                      Lng32 colnum = hs_globals_y->objDef->getColNum($1->colname->data());
                      if (colnum < 0)
                        {
                          Lng32 retcode = -1;
                          HSHandleError(retcode);
                        }
                      *$1 = hs_globals_y->objDef->getColInfo(colnum);
                      $1->colnum = colnum;
                      $3->insert(*$1);
                      $$ = $3;
                    }
;


int_number :        INTEGER_NUMBER
                      {
                        $$ = $1;
                      }
                 |  APPROX_NUMBER
                      {
                        $$ = (Int64)ceil($1);
                      }
;


real_number :        INTEGER_NUMBER
                      {
                        $$ = convertInt64ToDouble($1);
                      }
                 |  APPROX_NUMBER
                      {
                        $$ = $1;
                      }
;
%%

