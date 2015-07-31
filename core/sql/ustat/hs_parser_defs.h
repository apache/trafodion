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
#ifndef HSPARSERDEF_H
#define HSPARSERDEF_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_parser_defs.h
 * Description:  Rename yy-functions to avoid name conflicts at binding time.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#define yyact ystatact
#define yyback ystatback
#define yybgin ystatbgin
#define yychar ystatchar
#define yychk ystatchk
#define yycrank ystatcrank
#define yydebug ystatdebug
#define yydef ystatdef
#define yyerrflag ystaterrflag
#define yyerror ystaterror
#define yyestate ystatestate
#define yyexca ystatexca
#define yyextra ystatextra
#define yyfind ystatfind
#define yyfnd ystatfnd
#define yyin ystatin
#define yyinput ystatinput
#define yyleng ystatleng
#define yylex ystatlex
#define yylineno ystatlineno
#define yylook ystatlook
#define yylsp ystatlsp
#define yylval ystatlval
#define yymatch ystatmatch
#define yymorfg ystatmorfg
#define yynerrs ystatnerrs
#define yyolsp ystatolsp
#define yyout ystatout
#define yyoutput ystatoutput
#define yypact ystatpact
#define yyparse ystatparse
#define yypgo ystatpgo
#define yyprevious ystatprevious
#define yyps ystatps
#define yyps_index ystatps_index
#define yypv ystatpv
#define yypvt ystatpvt
#define yyr1 ystatr1
#define yyr2 ystatr2
#define yyreds ystatreds
#define yys ystats
#define yysptr ystatsptr
#define yystate ystatstate
#define yysvec ystatsvec
#define yytext ystattext
#define yytchar ystattchar
#define yytmp ystattmp
#define yytoks ystattoks
#define yyunput ystatunput
#define yyv ystatv
#define yy_yyv ystat_yyv
#define yyrestart ystatrestart
#define yyval ystatval
#define yyvstop ystatvstop
#define yytop ystattop
#define yywrap ystatwrap

// #define YYSTYPE YSTATSTYPE
#define YYSTYPE USTATSTYPE

#define yy_switch_to_buffer ystat_switch_to_buffer

#define yy_create_buffer ystat_create_buffer
#define yy_delete_buffer ystat_delete_buffer
#define yy_init_buffer ystat_init_buffer
#define yy_load_buffer_state ystat_load_buffer_state

#define yy_flush_buffer ystat_flush_buffer
#define yy_scan_buffer ystat_scan_buffer
#define yy_scan_bytes ystat_scan_bytes
#define yy_scan_string ystat_scan_string

#endif /* HSPARSERDEF_H */
