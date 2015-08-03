#ifndef ARKCMP_PARSER_DEFS_H
#define ARKCMP_PARSER_DEFS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         arkcmp_parser_defs.h
 * RCS:          $Id: arkcmp_parser_defs.h,v 1.2 1997/04/23 00:31:39 Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $Date: 1997/04/23 00:31:39 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
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

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: arkcmp_parser_defs.h,v $
// Revision 1.2  1997/04/23 00:31:39
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:25:20
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.5.4.1  1997/04/10 18:33:47
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:39:46
// These are the source files from SourceSafe.
//
// 
// 8     1/22/97 11:04p
// Merged UNIX and NT versions.
// 
// 6     1/14/97 4:55a
// Merged UNIX and  NT versions.
// 
// 4     12/04/96 11:20a
// Revision 1.5  1996/11/21 03:18:27
// Moved "#define yyerror(txt) arkcmperror(txt)" from arkcmp_yacc.y.
//
// Revision 1.4  1996/05/31 21:35:57
// no change
//
// Revision 1.3  1996/04/05 20:19:10
// Included the standard banner with RCS strings.
// 

#define yylex arkcmplex
#define yyact arkcmpact
#define yyback arkcmpback
#define yybgin arkcmpbgin
#define yychar arkcmpchar
#define yychk arkcmpchk
#define yycrank arkcmpcrank
#define yycvtok arkcmpcvtok
#define yydebug arkcmpdebug
#define yydef arkcmpdef
#define yyerrflag arkcmperrflag
#define yyerror(txt) arkcmperror(txt)
#define yyestate arkcmpestate
#define yyexca arkcmpexca
#define yyextra arkcmpextra
#define yyfind arkcmpfind
#define yyfnd arkcmpfnd
#define yyin arkcmpin
#define yyinput arkcmpinput
#define yyleng arkcmpleng
#define yylineno arkcmplineno
#define yylook arkcmplook
#define yylsp arkcmplsp
#define yylval arkcmplval
#define yymatch arkcmpmatch
#define yymorfg arkcmpmorfg
#define yynerrs arkcmpnerrs
#define yyolsp arkcmpolsp
#define yyout arkcmpout
#define yyoutput arkcmpoutput
#define yypact arkcmppact
#define yyparse arkcmpparse
#define yypgo arkcmppgo
#define yyprevious arkcmpprevious
#define yyps arkcmpps
#define yypv arkcmppv
#define yyr1 arkcmpr1
#define yyr2 arkcmpr2
#define yyreds arkcmpreds
#define yys arkcmps
#define yysptr arkcmpsptr
#define yystate arkcmpstate
#define yysvec arkcmpsvec
#define yytchar arkcmptchar
#define yytmp arkcmptmp
#define yytoks arkcmptoks
#define yyunput arkcmpunput
#define yyv arkcmpv
#define yyval arkcmpval
#define yyvstop arkcmpvstop
#define yytop arkcmptop
#define yywrap arkcmpwrap

#define YYSTYPE ARKCMPSTYPE
#define yy_switch_to_buffer arkcmp_switch_to_buffer

#define yy_yyv arkcmp_yyv
#define yyrestart arkcmprestart
#define yy_create_buffer arkcmp_create_buffer
#define yy_delete_buffer arkcmp_delete_buffer
#define yy_init_buffer arkcmp_init_buffer
#define yy_load_buffer_state arkcmp_load_buffer_state

#define yy_flush_buffer arkcmp_flush_buffer
#define yy_scan_buffer arkcmp_scan_buffer
#define yy_scan_bytes arkcmp_scan_bytes
#define yy_scan_string arkcmp_scan_string
#define yytext arkcmp_yytext

#endif /* ARKCMP_PARSER_DEFS_H */
