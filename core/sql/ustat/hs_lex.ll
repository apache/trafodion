%a 60000
%o 30000
%p 30000
%e 30000
%n 30000

%{
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_lex.l
 * Description:  Scanner rules for UPDATE STATISTICS.
 *
 *
 * Created:      03/25/96
 * Language:     C++
 *
 *
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
 *
 *

 *****************************************************************************
 */
#include "NLSConversion.h"
#include "ParserMsg.h"
#include "hs_globals.h"
#include "hs_parser.h"
#include "hs_yacc.h"
//#define yylval ystatlval
#define yyerror ystaterror
#define YYPARSE_PARAM yyscan_t scanner
#define YYLEX_PARAM scanner
#include <errno.h>
#pragma nowarn(1506)  //warning elimination

extern "C" int isatty(int);

int yyback(int *p, int m);
int yylook(void);
THREAD_P HSLogMan *LM;

extern THREAD_P char *hs_input;
static int hs_cursor = 0;

THREAD_P NAString* ius_where_condition_text = NULL;

#undef YY_INPUT
#define YY_INPUT(buffer,result,maxsize)\
{\
        buffer[0] = hs_input[hs_cursor++];\
        if ( !buffer[0] )\
                result = YY_NULL;\
        else\
                result=1;\
}
#undef unput
#define unput(c) HsUnput(c)

// LCOV_EXCL_START :cnu
// put back a previously read input character that wasn't needed
void HsUnput(char c)
{
 hs_cursor--;
}
// LCOV_EXCL_STOP


// LCOV_EXCL_START :cnu
// handle EOF in the input stream and let lex continue
// with the next input file
extern "C"
{
int yywrap(void*)
{
  return 0;
}
// LCOV_EXCL_STOP

void yyerror(const char *errText)
{
  HSGlobalsClass *hs_globals = GetHSContext();
  if (hs_globals == NULL)
    // LCOV_EXCL_START :rfi -- hsglobals created before ustat stmt parsed
    return;
    // LCOV_EXCL_STOP

  if (hs_globals->parserError == HSGlobalsClass::ERROR_SYNTAX)
    {
      hs_globals->diagsArea << DgSqlCode(-15001);
      StoreSyntaxError(hs_input, hs_cursor, hs_globals->diagsArea);
    }
}
};

void HSFuncResetLexer(yyscan_t scanner)
{
  hs_cursor = 0;
  LM = HSLogMan::Instance();
  ius_where_condition_text = NULL;
  ystatrestart(0, scanner);
}
%}

%option reentrant 
%option bison-bridge


%x CMNT
%x IUS_WHERE

DIGIT                   [0-9]
NUMBER                  ({DIGIT}+)
DECIMAL                 ([\+-]?\.{NUMBER}|[\+-]?{NUMBER}\.{NUMBER}|[\+-]?{NUMBER}\.)
INTEGER_NUMBER          ([\+-]?{NUMBER})
APPROX_NUMBER           (({INTEGER_NUMBER}|{DECIMAL})[eE]{INTEGER_NUMBER}|{DECIMAL})

ALPHA                   [a-zA-Z]
ALPHANUM                [a-zA-Z0-9]
R_IDENTIFIER            ({ALPHA}({ALPHA}|{ALPHANUM}|_)*)
DEFINE_ATTRIBUTE        ((=){ALPHA}({ALPHA}|{ALPHANUM}|_|-)*)
G                       ({ALPHA}({ALPHA}|{ALPHANUM})*)
VOLUME                  ((\\{G}\.\${G})|(\${G}))
G_IDENTIFIER            ({G}|(\"{G}\"))
GUARDIAN_NAME           ({VOLUME}\.{G_IDENTIFIER}\.{G_IDENTIFIER})

D_IDENTIFIER            (\"([^\"]|"\"\"")*\")
STRING                  (\'([^\']|"\'\'")*\')
BLANK_SPACE             [ \t\r\n]+
END_SYNTAX              [;\0]
SPECIAL                 [^0-9a-zA-Z \t\n\;]

CMNT_START              "/*"
%%

[Ww][Hh][Ee][Rr][Ee]    {
                          if (ius_where_condition_text == NULL)
                             ius_where_condition_text = new(CTXTHEAP) NAString("");
                          (*ius_where_condition_text).resize(0);
                          BEGIN IUS_WHERE;
                          return(WHERE);
                        }

[Ii][Nn][Cc][Rr][Ee][Mm][Ee][Nn][Tt][Aa][Ll]    {
                          return(INCREMENTAL);
                        }

[Uu][Pp][Dd][Aa][Tt][Ee] {
                          return(UPDATE);
                        }

[Pp][Ee][Rr][Ss][Ii][Ss][Tt][Ee][Nn][Tt] {
                          return(PERSISTENT);
                        }
                        
[Ss][Tt][Aa][Tt][Ii][Ss][Tt][Ii][Cc][Ss] {
                          return(STATISTICS);
                        }

[Ff][Oo][Rr]            {
                          return(FOR);
                        }

[Ll][Oo][Gg]            {
                          /* ++MV */
                          return(LOG);
                        }
[Tt][Aa][Bb][Ll][Ee]    {
                          return(TABLE);
                        }

[Oo][Nn]                {
                          return(ON);
                        }

[Oo][Ff][Ff]            {
                          return(TOK_OFF);
                        }

[Ee][Vv][Ee][Rr][Yy]    {
                          return(EVERY);
                        }

[Cc][Oo][Ll][Uu][Mm][Nn] {
                          return(COLUMN);
                        }

[Kk][Ee][Yy]            {
                          return(KEY);
                        }

[Tt][Oo]                {
                          return(TO);
                        }

[Cc][Ll][Ee][Aa][Rr]    {
                          return(CLEAR);
                        }

[Vv][Ii][Ee][Ww][Oo][Nn][Ll][Yy] {
                          return(VIEWONLY);
                        }

[Gg][Ee][Nn][Ee][Rr][Aa][Tt][Ee] {
                          return(GENERATE);
                        }

[Ii][Nn][Tt][Ee][Rr][Vv][Aa][Ll][Ss] {
                          return(INTERVALS);
                        }

[Ss][Ee][Tt]            {
                          return(TOK_SET);
                        }

[Rr][Oo][Ww][Cc][Oo][Uu][Nn][Tt] {
                          return(ROWCOUNT);
                        }

[Ss][Aa][Mm][Pp][Ll][Ee] {
                          return(SAMPLE);
                        }

[Rr][Oo][Ww][Ss]        {
                          return(ROWS);
                        }

[Rr][Aa][Nn][Dd][Oo][Mm] {
                          return(RANDOM);
                        }

[Pp][Ee][Rr][Ii][Oo][Dd][Ii][Cc] {
                          return(PERIODIC);
                        }

[Pp][Ee][Rr][Cc][Ee][Nn][Tt] {
                          return(TOK_PERCENT);
                        }

[Cc][Ll][Uu][Ss][Tt][Ee][Rr][Ss] {
                          return(CLUSTERS);
                        }

[Bb][Ll][Oo][Cc][Kk][Ss] {
                          return(BLOCKS);
                        }

[Ss][Hh][Oo][Ww][Ss][Tt][Aa][Tt][Ss] {
                          return(SHOWSTATS);
                        }

[Dd][Ee][Tt][Aa][Ii][Ll] {
                          return(DETAIL);
                        }

[Oo][Ff]                {
                          return(OF);
                        }

[Ee][Xx][Ii][Ss][Tt][Ii][Nn][Gg] {
                          return(EXISTING);
                        }

[Cc][Oo][Ll][Uu][Mm][Nn][Ss] {
                          return(COLUMNS);
                        }

[Nn][Ee][Cc][Ee][Ss][Ss][Aa][Rr][Yy] {
                          return(NECESSARY);
                        }

[Cc][Rr][Ee][Aa][Tt][Ee] {
                          return(CREATE);
                        }

[Rr][Ee][Mm][Oo][Vv][Ee] {
                          return(REMOVE);
                        }

[Aa][Ll][Ll]            {
                          return(ALL);
                        }
                        
[Ww][Ii][Tt][Hh]        {
                          return(WITH);
                        }

[Ss][Kk][Ee][Ww][Ee][Dd]        {
                          return(SKEWED);
                        }

[Vv][Aa][Ll][Uu][Ee][Ss] {
                          return(VALUES);
                        }

[Nn][Oo]                {
                          return(NO);
                        }

{R_IDENTIFIER}          {
                          yylval->stringval = new(STMTHEAP) NAString(STMTHEAP);
                          *yylval->stringval = yytext;
                          yylval->stringval->toUpper();
                          return(REGULAR_IDENTIFIER);
                        }

{DEFINE_ATTRIBUTE}      {
                          // LCOV_EXCL_START :nsk
                          yylval->stringval = new(STMTHEAP) NAString(STMTHEAP);
                          *yylval->stringval = yytext;
                          yylval->stringval->toUpper();
                          return(DEFINE_NAME);
                          // LCOV_EXCL_STOP
                        }

{D_IDENTIFIER}          {
                          long retcode = 0;

                          yylval->stringval = new(STMTHEAP) NAString(STMTHEAP);
                         *yylval->stringval = yytext;
                          retcode = ToInternalIdentifier(*yylval->stringval, FALSE);
                          if (retcode != 0 && LM->LogNeeded())
                            {
                              sprintf(LM->msg, "***[ERROR %ld] INVALID DELIMITED IDENTIFIER: %s", retcode, yytext);
                              LM->Log(LM->msg);
                            }
                          
                         return(DELIMITED_IDENTIFIER);
                        }

{GUARDIAN_NAME}         {
                          // LCOV_EXCL_START :nsk
                          yylval->stringval = new(STMTHEAP) NAString(STMTHEAP);
                          *yylval->stringval = yytext;
                          yylval->stringval->toUpper();
                          return(GUARDIAN_TABLE_NAME);
                          // LCOV_EXCL_STOP
                        }

{INTEGER_NUMBER}        {
                          errno  = 0;
                          double dval = strtod(yytext,(char**)NULL);
                          Int64 maxLongVal = LLONG_MAX;
                          double maxLongValAsDouble = (double)maxLongVal;
                          if (errno || dval > maxLongValAsDouble)
                            {
                              yylval->tokval = -1;
                              if (LM->LogNeeded())
                                {
                                  sprintf(LM->msg, "***[ERROR] INVALID INTEGER: %s", yytext);
                                  LM->Log(LM->msg);
                                }
                            }
                          else
                            {
                              yylval->tokval = (Int64)dval;
                            }

                          return(INTEGER_NUMBER);
                        }

{APPROX_NUMBER}         {
                          errno  = 0;
                          yylval->tokdbl = strtod(yytext,(char**)NULL);
                          Int64 maxLongVal = LLONG_MAX;
                          double maxLongValAsDouble = (double)maxLongVal;
                          if (errno || yylval->tokdbl > maxLongValAsDouble)
                            {
                              yylval->tokdbl = -1;
                              if (LM->LogNeeded())
                                {
                                  sprintf(LM->msg, "***[ERROR] INVALID FLOAT: %s", yytext);
                                  LM->Log(LM->msg);
                                }
                            }

                          return(APPROX_NUMBER);
                        }

{SPECIAL}               {
                          return((int)(yylval->tokval = (int) yytext[0]));
                        }

{BLANK_SPACE}           {
                          /* no action required - ignore blank space */
                        }

{END_SYNTAX}            {
                          return((int)(yylval->tokval = 0));
                        }

{CMNT_START}            BEGIN CMNT;


<CMNT>.                 |
<CMNT>\n                ;
<CMNT>"*/"              BEGIN INITIAL;


<IUS_WHERE>{END_SYNTAX}    {
                             // The input position is always 2 characters ahead
                             // of the last character of the last token scanned,
                             // so we must do 2 unputs. The argument to that
                             // function is required, but ignored, so it doesn't
                             // matter what we pass.
                             unput(';'); unput(';');
                             BEGIN INITIAL;
                             return(WHERE_CONDITION);
                           }

<IUS_WHERE>{D_IDENTIFIER}  |
<IUS_WHERE>{STRING}        |
<IUS_WHERE>.               |
<IUS_WHERE>\n              {
                             (*ius_where_condition_text).append(yytext);
                           }

%%

void init_scanner(void* &scanner)
{
   yylex_init(&scanner);
}

void destroy_scanner(void* &scanner)
{
   yylex_destroy(scanner);
}


// LCOV_EXCL_START
#pragma warn(1506)  //warning elimination
// LCOV_EXCL_STOP
#pragma nowarn(262)  //warning elimination

