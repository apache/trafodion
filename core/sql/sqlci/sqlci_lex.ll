/********************************************************************  
//
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
********************************************************************/

%a 60000
%o 30000
%p 30000
%e 30000
%n 30000
%{
/* -*-C++-*-
 *****************************************************************************
 *
 * File:          sqlci_lex.l

 * Description:  
 *               
 *               
 * Created:      4/15/95

 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComGuardianFileNameParts.h"
#include "Platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "SqlciCmd.h"
#include "str.h"
#include "Sqlci.h"
#define yylval sqlcilval
#define yyerror sqlcierror
#include "sqlci_yacc.h"
#include "SqlciParseGlobals.h"

static int token_to_return;

#define return_IDENT_or_TOKEN(token_id, set_ident)  \
{\
 if (SqlciParse_IdentifierExpected)\
   {\
     if (yylval.stringval_type)\
       delete [] yylval.stringval_type;\
     yylval.stringval_type = new char[strlen(yytext)+1];\
     strcpy(yylval.stringval_type, yytext);\
     token_to_return = IDENTIFIER; \
   }\
 else token_to_return = token_id; \
 if (SqlciParse_SyntaxErrorCleanup) \
   SqlciParse_IdentifierExpected = -1; \
 else \
   SqlciParse_IdentifierExpected = set_ident;\
/*  cout << "token = " << token_id << endl;*/  \
 return (token_to_return);\
}
#ifndef NA_LINUX
#ifdef	NA_WINNT
extern "C" int isatty(int);
#else
extern "C" { int yylex(void); }
#endif
#else
extern "C" { int yylex(void); }
#endif

# undef YY_INPUT
#define YY_INPUT(buffer,result,maxsize)\
{\
    buffer[0] = SqlciParse_InputStr[SqlciParse_InputPos++];\
    if ( !buffer[0] )\
        result = YY_NULL;\
    else\
        result=1;\
}

#undef unput
#define unput(c) SqlciUnput(c)

#define yylval sqlcilval
#define yyerror sqlcierror

// put back a previously read input character that wasn't needed
void SqlciUnput(char c)
{
 SqlciParse_InputPos--;
}

// handle EOF in the input stream and let lex continue
// with the next input file
extern "C"   
  int yywrap(void)
    {
      return 0;
    }

%}

D			[0-9]
A			[a-zA-Z]
G                       ({A}({A}|{D})*)

%x FNAME
%s LOGFNAME
B			[ \t\n]+
%%

{B} ;
<FNAME>{B} ;
<LOGFNAME>{B} ;
[Nn][Oo][Ss][Qq][Ll]		      return_IDENT_or_TOKEN(NOSQL, 0);
[Cc][Oo][Nn][Tt][Aa][Ii][Nn][Ss][Qq][Ll]  return_IDENT_or_TOKEN(CONTAINSQL, 0);
[Rr][Ee][Aa][Dd][Ss][Qq][Ll]	      return_IDENT_or_TOKEN(READSQL, 0);
[Mm][Oo][Dd][Ii][Ff][Yy][Ss][Qq][Ll]  return_IDENT_or_TOKEN(MODIFYSQL, 0);
[Rr][Ee][Ss][Ee][Tt][Vv][Ii][Oo][Ll][Aa][Tt][Ii][Oo][Nn]  return_IDENT_or_TOKEN(RESETVIOLATION, 0);
[Cc][Hh][Ee][Cc][Kk][Vv][Ii][Oo][Ll][Aa][Tt][Ii][Oo][Nn]  return_IDENT_or_TOKEN(CHECKVIOLATION, 0);
[Nn][Oo][Aa][Uu][Tt][Oo][Xx][Aa][Cc][Tt]		return_IDENT_or_TOKEN(NoAutoXact, 0);
[Dd][Ee][Ss][Cc][Rr][Ii][Bb][Ee]			return_IDENT_or_TOKEN(DESCRIBEToken, 0);
[Cc][Rr][Ee][Aa][Tt][Ee][Cc][Oo][Nn][Tt][Ee][Xx][Tt]	return_IDENT_or_TOKEN(CREATECONTEXT, 0);
[Cc][Uu][Rr][Rr][Ee][Nn][Tt][Cc][Oo][Nn][Tt][Ee][Xx][Tt] return_IDENT_or_TOKEN(CURRENTCONTEXT, 0);
[Ss][Ww][Ii][Tt][Cc][Hh][Cc][Oo][Nn][Tt][Ee][Xx][Tt]	return_IDENT_or_TOKEN(SWITCHCONTEXT, 0);
[Dd][Ee][Ll][Ee][Tt][Ee][Cc][Oo][Nn][Tt][Ee][Xx][Tt]    return_IDENT_or_TOKEN(DELETECONTEXT, 0);
[Rr][Ee][Ss][Ee][Tt][Cc][[Oo][Nn][Tt][Ee][Xx][Tt]	return_IDENT_or_TOKEN(RESETCONTEXT, 0);
[Aa][Dd][Dd]                           return_IDENT_or_TOKEN( ADDtoken, 0);
[Aa][Ll][Ll]                           return_IDENT_or_TOKEN(ALLtoken, 0);
[Bb][Ee][Gg][Ii][Nn]                   return_IDENT_or_TOKEN(TOK_BEGIN, 0);
[Bb][Rr][Ii][Ee][Ff]                   return_IDENT_or_TOKEN(BRIEF, 0);
[Bb][Tt]                               return_IDENT_or_TOKEN(TOK_BEGIN, 0);
[Dd][Ee][Tt][Aa][Ii][Ll]               return_IDENT_or_TOKEN(DETAIL, 0);
[Cc][Aa][Ll][Ll]		       return_IDENT_or_TOKEN (CALLToken, 0);
[Cc][Ll][Ee][Aa][Nn][Uu][Pp]           return_IDENT_or_TOKEN(CLEANUP, 0);
[Cc][Ll][Aa][Ss][Ss]                   return_IDENT_or_TOKEN(CLASStoken, 0);
[Cc][Ll][Oo][Ss][Ee]                   return_IDENT_or_TOKEN(CLOSEtoken, 0);
[Cc][Oo][Mm][Mm][Aa][Nn][Dd]           return_IDENT_or_TOKEN(COMMANDStoken,0);
[Cc][Oo][Mm][Mm][Aa][Nn][Dd][Ss]       return_IDENT_or_TOKEN(COMMANDStoken,0);
[Cc][Oo][Mm][Mm][Ii][Tt]               return_IDENT_or_TOKEN(COMMIT, 0);
[Cc][Oo][Mm][Mm][Ee][Nn][Tt]       return_IDENT_or_TOKEN(COMMENTtoken, 0);
[Cc][Oo][Nn][Tt][Rr][Oo][Ll]	       return_IDENT_or_TOKEN(CONTROL, 0);
[Cc][Oo][Nn][Tt][Rr][Oo][Ll][Ss]       return_IDENT_or_TOKEN(CONTROL, 0);
[Cc][Qq][Dd]	                       return_IDENT_or_TOKEN(CONTROL, 0);
[Oo][Ss][Ii][Mm]                       return_IDENT_or_TOKEN(OSIM, 0);
[Cc][Uu][Rr][Ss][Oo][Rr]               return_IDENT_or_TOKEN(CURSORtoken, 0);
[Cc][Uu][Rr][Ss][Oo][Rr][ ]+[Ww][Ii][Tt][Hh] return_IDENT_or_TOKEN(CURSORWITH, 0);
[Dd][Ee][Ff][Aa][Uu][Ll][Tt]           return_IDENT_or_TOKEN(DEFAULTtoken, 0);
[Dd][Ee][Aa][Ll][Ll][Oo][Cc]           return_IDENT_or_TOKEN(DEALLOCtoken, 0);
[Dd][Ee][Cc][Ll][Aa][Rr][Ee]           return_IDENT_or_TOKEN(DECLAREtoken, 0);
[Dd][Ee][Ff][Ii][Nn][Ee]               return_IDENT_or_TOKEN(DEFINEtoken, 0);
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]           return_IDENT_or_TOKEN(DISPLAY, 0);
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]{B}[Ss][Tt][Aa][Tt][Ii][Ss][Tt][Ii][Cc][Ss]           return_IDENT_or_TOKEN(DISPLAY_STATISTICS, 0);
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]{B}[Qq][Ii][Dd]   return_IDENT_or_TOKEN(DISPLAY_QUERYID,0);
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]_[Ee][Xx][Pp][Ll][Aa][Ii][Nn]          return_IDENT_or_TOKEN(DISPLAY_EXPLAIN, 0);
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]_[Qq][Cc]  return_IDENT_or_TOKEN(DISPLAY_QC,0);
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]_[Qq][Cc]_[Ee][Nn][Tt][Rr][Ii][Ee][Ss] return_IDENT_or_TOKEN(DISPLAY_QC_ENTRIES,0);
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]_[Qq][Cc]_[Ee][Nn][Tt][Rr][Ii][Ee][Ss]_[Nn][Oo][Tt][Ii][Mm][Ee] return_IDENT_or_TOKEN(DISPLAY_QC_ENTRIES_NOTIME,0);
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]{B}[Uu][Ss][Ee]{B}[Oo][Ff]          return_IDENT_or_TOKEN(DISPLAY_USE_OF, 0);
[Ee][Nn][Vv]  			       return_IDENT_or_TOKEN(MXCI_TOK_ENV, 0);
[Ee][Nn][Vv][Vv][Aa][Rr]  	       return_IDENT_or_TOKEN(ENVVARtoken, 0);
[Ee][Tt]                               return_IDENT_or_TOKEN(COMMIT, 0);
[Ee][Xx][Pp][Ll][Aa][Ii][Nn]           return_IDENT_or_TOKEN(EXPLAIN, 0);
[Ee][Xx][Tt][Rr][Aa][Cc][Tt] 	       return_IDENT_or_TOKEN(EXTRACTtoken, 0);
[Ff][Ee][Tt][Cc][Hh]                   return_IDENT_or_TOKEN(FETCHtoken, 0);
[Ff][Ii][Ll][Ee]                       { 
                                         BEGIN(FNAME);
                     //SqlciParse_IdentifierExpected = 0;
                                         return_IDENT_or_TOKEN(FILEtoken, 0);
                                       }
[Gg][Ee][Nn][Ee][Rr][Aa][Tt][Ee]       return_IDENT_or_TOKEN(GENERATEtoken, 0);
[Gg][Ee][Tt]                           return_IDENT_or_TOKEN(GETtoken, 0);
[Gg][Ee][Tt]{B}[Ss][Tt][Aa][Tt][Ii][Ss][Tt][Ii][Cc][Ss]    return_IDENT_or_TOKEN(GETSTATISTICStoken, 0);
[Hh][Ii][Vv][Ee]                       return_IDENT_or_TOKEN (HIVEtoken, 0); 
[Ii][Nn]                       return_IDENT_or_TOKEN (INtoken, 0); 
[Ii][Nn][Ff][Oo]                       return_IDENT_or_TOKEN (INFOtoken, 0); 
[Ii][Nn][Ff][Ii][Ll][Ee]               {
                     BEGIN FNAME; 
                     return_IDENT_or_TOKEN (INFILE, 0); 
                       }
[Ll][Oo][Aa][Dd] 		       return_IDENT_or_TOKEN(LOADtoken, 0);
[Mm][Ee][Rr][Gg][Ee]                   return_IDENT_or_TOKEN(MERGEtoken, 0);
[Mm][Ee][Tt][Aa][Dd][Aa][Tt][Aa]                   return_IDENT_or_TOKEN(METADATAtoken, 0);
[Mm][Aa][Pp]                           return_IDENT_or_TOKEN(MAPtoken, 0);
[Mm][Oo][Dd][Ii][Ff][Yy]               return_IDENT_or_TOKEN(MODIFY, 0);
[Mm][Ss][Cc][Kk] 		       return_IDENT_or_TOKEN(MSCKtoken, 0);
[Oo][Bb][Ee][Yy]  		       { 
                     BEGIN FNAME; 
                                         return_IDENT_or_TOKEN (OBEY, 0); 
                                       }
[Oo][Bb][Jj][Ee][Cc][Tt]               return_IDENT_or_TOKEN(OBJECTtoken, 0);
[Oo][Nn]                               return_IDENT_or_TOKEN(ON, 0);
[Oo][Ff]                               return_IDENT_or_TOKEN(OFtoken, 0);
[Oo][Ff][Ff]                           return_IDENT_or_TOKEN(OFF, 0);
[Oo][Uu][Tt][Ff][Ii][Ll][Ee]           {
                     BEGIN FNAME; 
                     return_IDENT_or_TOKEN (OUTFILE, 0); 
                       }
[Oo][Pp][Ee][Nn]                       return_IDENT_or_TOKEN(OPENtoken, 0);
[Oo][Pp][Tt][Ii][Oo][Nn][Ss]           return_IDENT_or_TOKEN(OPTIONStoken, 0);
[Pp][Rr][Ee][Pp][Aa][Rr][Ee][Dd]       return_IDENT_or_TOKEN(PREPARED, 0);
[Pp][Rr][Oo][Cc][Ee][Ss][Ss]           return_IDENT_or_TOKEN(PROCESStoken, 0);
[Ee][Rr][Rr][Oo][Rr]                   return_IDENT_or_TOKEN(ERRORtoken, 0);
[Ee][Xx][Ii][Tt] 		       return_IDENT_or_TOKEN(EXIT, 0);
[Ff][Cc]   		               return_IDENT_or_TOKEN(FC, -1);
[Ff][Oo][Rr]   		               return_IDENT_or_TOKEN(FORtoken, 0);
[Rr][Ee][Pp][Ll][Ii][Cc][Aa][Tt][Ee]    return_IDENT_or_TOKEN(REPLICATEtoken, 0);
[[Cc][Pp][Uu]                           return_IDENT_or_TOKEN(CPU, 0);
[Pp][Ii][Dd]                           return_IDENT_or_TOKEN(PID, 0);
[Qq][Ii][Dd]			       return_IDENT_or_TOKEN(QID, 0);
[Rr][Ee][Pp][Ee][Aa][Tt]               return_IDENT_or_TOKEN(REPEAT, -1);
[Ss][Hh][Oo][Ww][Ss][Tt][Aa][Tt][Ss]   return_IDENT_or_TOKEN(SHOWSTATS, 0);
[Ss][Hh][Oo][Ww][Tt][Rr][Aa][Nn][Ss][Aa][Cc][Tt][Ii][Oo][Nn]   return_IDENT_or_TOKEN(SHOWSTATS, 0);
[Ii][Nn][Vv][Oo][Kk][Ee]               return_IDENT_or_TOKEN(INVOKE, 0);
[Ll][Oo][Gg]            	       {
                                         BEGIN LOGFNAME;
                                         return_IDENT_or_TOKEN(LOG, 0);
                                       }
                                      
[Hh][Ii][Ss][Tt][Oo][Rr][Yy]	       return_IDENT_or_TOKEN(HISTORY, 0);
[Rr][Ee][Ss][Ee][Tt]                   return_IDENT_or_TOKEN(RESET, 0);
[Ss][Ee][Tt]{B}[Tt][Aa][Bb][Ll][Ee] return_IDENT_or_TOKEN(SET_TABLEtoken, 0);
[Ss][Ee][Tt]{B}[Tt][Rr][Aa][Nn][Ss][Aa][Cc][Tt][Ii][Oo][Nn] return_IDENT_or_TOKEN(SET_TRANSACTIONtoken, 0);
[Ss][Ee][Tt]                           return_IDENT_or_TOKEN(SETtoken, 0);
[Ss][Ee][Tt][Ee][Nn][Vv]               return_IDENT_or_TOKEN(SETENV, 0);
[Ll][Ii][Ss][Tt]_[Cc][Oo][Uu][Nn][Tt]  return_IDENT_or_TOKEN(LISTCOUNT, 0);
[Ff][Ii][Rr][Ss][Tt]                   return_IDENT_or_TOKEN(FIRST, 0);
[Nn][Ee][Xx][Tt]                       return_IDENT_or_TOKEN(NEXT, 0);
[Rr][Ee][Pp][Oo][Rr][Tt]               return_IDENT_or_TOKEN(REPORT, 0);
[Ss][Qq][Ll]                           return_IDENT_or_TOKEN(SQL, 0);
[Cc][Aa][Nn][Cc][Ee][ll]               return_IDENT_or_TOKEN(CANCEL, 0);
[Mm][Oo][Dd][Ee]                       return_IDENT_or_TOKEN(MODE,0);
[Ww][Aa][Rr][Nn][Ii][Nn][Gg][Ss]       return_IDENT_or_TOKEN(VERBOSE, 0);
[Pp][Aa][Rr][Aa][Mm]                   return_IDENT_or_TOKEN(PARAM, 0);
[Pp][Aa][Rr][Aa][Mm][Ss]               return_IDENT_or_TOKEN(PARAM, 0);
[Pp][Aa][Tt][Tt][Ee][Rr][Nn]           return_IDENT_or_TOKEN(SQ_LINUX_PATTERN, 0);
[Pp][Aa][Tt][Tt][Ee][Rr][Nn]_[Aa][Ss]_[Ii][Ss]  return_IDENT_or_TOKEN(PATTERN_AS_IS, 0);
[Pp][Rr][Oo][Cc][Ee][Dd][Uu][Rr][Ee]   return_IDENT_or_TOKEN(PROCEDUREtoken, 0);
[Pp][Uu][Rr][Gg][Ee][Dd][Aa][Tt][Aa]   return_IDENT_or_TOKEN(PURGEDATA, 0);
[Pp][Oo][Pp][Uu][Ll][Aa][Tt][Ee]       return_IDENT_or_TOKEN(POPULATE, 0);
[Vv][Aa][Ll][Ii][Dd][Aa][Tt][Ee]       return_IDENT_or_TOKEN(VALIDATEtoken, 0);
[Rr][Ee][Ff][Rr][Ee][Ss][Hh]		   return_IDENT_or_TOKEN(REFRESH, 0);	/* MV - REFRESH utility */
[Rr][Ee][Pp][Oo][Ss][Ii][Tt][Oo][Rr][Yy]    return_IDENT_or_TOKEN(REPOSITORYtoken, 0);
[Rr][Oo][Ww][Ss][Ee][Tt]               return_IDENT_or_TOKEN(ROWSETtoken, 0);
[Tt][Rr][Aa][Nn][Ss][Ff][Oo][Rr][Mm]   return_IDENT_or_TOKEN(TRANSFORM, 0);
[Cc][Ll][Ee][Aa][Rr]                   return_IDENT_or_TOKEN(CLEAR, 0);
[Ss][Hh]                               return_IDENT_or_TOKEN(SHELL, 0);
[Ss][Hh][Ee][Ll][Ll]                   return_IDENT_or_TOKEN(SHELL, 0);
[Ss][Hh][Oo][Ww]                       return_IDENT_or_TOKEN(SHOW, 0);
[Ss][Hh][Oo][Ww][Cc][Oo][Nn][Tt][Rr][Oo][Ll]           return_IDENT_or_TOKEN(SHOWCONTROL, 0);
[Ss][Hh][Oo][Ww][Dd][Dd][Ll]           return_IDENT_or_TOKEN(SHOWDDL, 0);
[Ss][Hh][Oo][Ww][Pp][Ll][Aa][Nn]       return_IDENT_or_TOKEN(SHOWPLAN, 0);
[Ss][Hh][Oo][Ww][Ss][Hh][Aa][Pp][Ee]   return_IDENT_or_TOKEN(SHOWSHAPE, 0);
[Ss][Hh][Oo][Ww][Ss][Ee][Tt]           return_IDENT_or_TOKEN(SHOWSET, 0);
[Ss][Ee][Ss][Ss][Ii][Oo][Nn]           return_IDENT_or_TOKEN(SESSIONtoken, 0);
[Ss][Tt][Oo][Rr][Ee]                   return_IDENT_or_TOKEN(STOREtoken, 0);
[Ss][Yy][Nn][Tt][Aa][Xx]               return_IDENT_or_TOKEN(SYNTAX, 0);
[Ss][Yy][Ss][Tt][Ee][Mm]               return_IDENT_or_TOKEN(SYSTEMtoken, 0);
[Ww][Aa][Ii][Tt]                       return_IDENT_or_TOKEN(WAITtoken, 0);
[Ee][Xx][Aa][Mm][Pp][Ll][Ee]           return_IDENT_or_TOKEN(EXAMPLE, 0);
[Ss][Ee][Ll]	                       return_IDENT_or_TOKEN(SELECTtoken, 0);
[Ss][Ee][Ll][Ee][Cc][Tt]	       return_IDENT_or_TOKEN(SELECTtoken, 0);
[Uu][Pp][Dd][Aa][Tt][Ee]	       return_IDENT_or_TOKEN(UPDATEtoken, 0);
[Dd][Ee][Ll][Ee][Tt][Ee] 	       return_IDENT_or_TOKEN(DELETEtoken, 0);
[Ii][Nn][Ss]	                       return_IDENT_or_TOKEN(INSERTtoken, 0);
[Ii][Nn][Ss][Ee][Rr][Tt]	       return_IDENT_or_TOKEN(INSERTtoken, 0);
[Uu][Pp][Ss][Ee][Rr][Tt]	       return_IDENT_or_TOKEN(UPSERTtoken, 0);
[Tt][Aa][Bb][Ll][Ee] 		       return_IDENT_or_TOKEN(TABLE, 0);
[Vv][Aa][Ll][Uu][Ee][Ss]	       return_IDENT_or_TOKEN(VALUES, 0);
[Vv][Ee][Rr][Ss][Ii][Oo][Nn]           return_IDENT_or_TOKEN(VERSIONtoken, 0);
[Cc][Rr][Ee][Aa][Tt][Ee]	       return_IDENT_or_TOKEN(CREATE, 0);
[Aa][Ll][Tt][Ee][Rr]		       return_IDENT_or_TOKEN(ALTER, 0);
[Dd][Rr][Oo][Pp] 		       return_IDENT_or_TOKEN(DROP, 0);
[Ll][Oo][Cc][Kk]                       return_IDENT_or_TOKEN(LOCK, 0);
[Ll][Oo][Cc][Kk][Ii][Nn][Gg]           return_IDENT_or_TOKEN(LOCKINGtoken, 0);
[Uu][Nn][Ll][Oo][Cc][Kk]               return_IDENT_or_TOKEN(UNLOCK, 0);
[Pp][Rr][Ee][Pp][Aa][Rr][Ee]	       return_IDENT_or_TOKEN(PREPAREtoken, 0);
[Ee][Xx][Ee][Cc][Uu][Tt][Ee]	       return_IDENT_or_TOKEN(EXECUTEtoken, 0);
[Uu][Ss][Ii][Nn][Gg]                   return_IDENT_or_TOKEN(USING, 0);
[Ff][Rr][Oo][Mm]		       return_IDENT_or_TOKEN(FROM, 0);
[Ee][Dd][Ii][Tt] 		       return_IDENT_or_TOKEN(EDIT, 0);
[Ll][Ss]	        	       return_IDENT_or_TOKEN(LS, 0);
[Cc][Dd]   		               return_IDENT_or_TOKEN(CD, 0);
[Rr][Oo][Ll][Ll][Bb][Aa][Cc][Kk]       return_IDENT_or_TOKEN(ROLLBACK, 0);
[Ss][Tt][Aa][Tt][Ii][Ss][Tt][Ii][Cc][Ss]	return_IDENT_or_TOKEN(STATISTICS, 0);
[Ww][Oo][Rr][Kk]                       return_IDENT_or_TOKEN(WORK, 0);
[Uu][Pp][Dd][Aa][Tt][Ee]{B}[Ss][Tt][Aa][Tt][Ii][Ss][Tt][Ii][Cc][Ss]	return_IDENT_or_TOKEN(UPD_STATS, 0);
[Uu][Pp][Dd][Aa][Tt][Ee]{B}[Hh][Ii][Ss][Tt][Oo][Gg][Rr][Aa][Mm]{B}[Ss][Tt][Aa][Tt][Ii][Ss][Tt][Ii][Cc][Ss]	return_IDENT_or_TOKEN(UPD_HIST_STATS, 0);
[Ii][Nn][Ii][Tt][Ii][Aa][Ll][Ii][Zz][Ee] return_IDENT_or_TOKEN(INITIALIZE, 0);
[Rr][Ee][Ii][Nn][Ii][Tt][Ii][Aa][Ll][Ii][Zz][Ee] return_IDENT_or_TOKEN(REINITIALIZE, 0);
[Gg][Rr][Aa][Nn][Tt]                   return_IDENT_or_TOKEN(GIVE, 0);
[Gg][Ii][Vv][Ee]                       return_IDENT_or_TOKEN(GRANTtoken, 0);
[Rr][Ee][Vv][Oo][Kk][Ee]               return_IDENT_or_TOKEN(REVOKEtoken, 0);
[Nn][Aa][Mm][Ee][Tt][Yy][Pp][Ee]       return_IDENT_or_TOKEN(NAMETYPE, 0);
[Mm][Pp][Ll][Oo][Cc]                   return_IDENT_or_TOKEN(MPLOC, 0);
[Cc][Aa][Tt][Aa][Ll][Oo][Gg]	       return_IDENT_or_TOKEN(CATALOG, 0);
[Rr][Ee][Gg][Ii][Ss][Tt][Ee][Rr]       return_IDENT_or_TOKEN(REGISTER, 0);
[Uu][Nn][Rr][Ee][Gg][Ii][Ss][Tt][Ee][Rr] return_IDENT_or_TOKEN(UNREGISTER, 0);
[Ss][Cc][Hh][Ee][Mm][Aa]	       return_IDENT_or_TOKEN(SCHEMA, 0);
[Ss][Tt][Aa][Tt][Ee][Mm][Ee][Nn][Tt]   return_IDENT_or_TOKEN(STATEMENTtoken, 0);
[Ii][Ff]                               return_IDENT_or_TOKEN(IF, 0);
[Uu][Ii][Dd]                           return_IDENT_or_TOKEN(UIDtoken, 0);
[Ww][Hh][Ii][Ll][Ee]                   return_IDENT_or_TOKEN(WHILE, 0);
[Ss][Ii][Gg][Nn][Aa][Ll]	       return_IDENT_or_TOKEN(SIGNAL, 0);
[Ww][Ii][Tt][Hh][Oo][Uu][Tt]           return_IDENT_or_TOKEN(WITHOUT, 0);
[Ww][Ii][Tt][Hh]                       return_IDENT_or_TOKEN(WITH, 0);
[Hh][Oo][Ll][Dd]                       return_IDENT_or_TOKEN(HOLD, 0);
[Pp][Aa][Rr][Ss][Ee][Rr][Ff][Ll][Aa][Gg][Ss] return_IDENT_or_TOKEN(PARSERFLAGS, 0);
[Tt][Ee][Rr][Mm][Ii][Nn][Aa][Ll]_[Cc][Hh][Aa][Rr][Ss][Ee][Tt] return_IDENT_or_TOKEN(TERMINAL_CHARSET, 0);
[Ii][Ss][Oo]_[Mm][Aa][Pp][Pp][Ii][Nn][Gg] return_IDENT_or_TOKEN(ISO_MAPPING, 0);
[Dd][Ee][Ff][Aa][Uu][Ll][Tt]_[Cc][Hh][Aa][Rr][Ss][Ee][Tt] return_IDENT_or_TOKEN(DEFAULT_CHARSETtoken, 0);
[Ii][Nn][Ff][Ee][Rr]_[Cc][Hh][Aa][Rr][Ss][Ee][Tt] return_IDENT_or_TOKEN(INFER_CHARSET, 0);
[Rr][Ee][Ss][Uu][Ll][Tt]               return_IDENT_or_TOKEN(RESULT, 0);
[Qq][Uu][Ii][Ee][Ss][Cc][Ee]           return_IDENT_or_TOKEN(QUIESCE, 0);
[Aa][Cc][Tt][Ii][Vv][Ee]               return_IDENT_or_TOKEN(ACTIVEtoken, 0);
[Aa][Cc][Cc][Uu][Mm][Uu][Ll][Aa][Tt][Ee][Dd] return_IDENT_or_TOKEN(ACCUMULATEDtoken, 0);
[Pp][Ee][Rr][Tt][Aa][Bb][Ll][Ee]       return_IDENT_or_TOKEN(PERTABLEtoken, 0);
[Pp][Rr][Oo][Gg][Rr][Ee][Ss][Ss]       return_IDENT_or_TOKEN(PROGRESStoken, 0);
[;]             {
                  SqlciParse_IdentifierExpected = 0; 
          return_IDENT_or_TOKEN(0, 0);
                };
[Ii][Nn][Tt][Ee][Rr][Nn][Aa][Ll]	return_IDENT_or_TOKEN(INTERNAL, 0);  /* MV OZ_REFRESH */
[Mm][Vv][Ll][Oo][Gg]				return_IDENT_or_TOKEN(MVLOG, 0);
[Uu][Nn][Ll][Oo][Aa][Dd]                return_IDENT_or_TOKEN(UNLOAD, 0); 
[Tt][Rr][Uu][Nn][Cc][Aa][Tt][Ee]        return_IDENT_or_TOKEN(TRUNCATE, 0);
[Cc][Hh][Aa][Nn][Gg][Ee][Uu][Ss][Ee][Rr] return_IDENT_or_TOKEN(USERtoken, 0);

[\*]		{SqlciParse_IdentifierExpected = 0; return(ALLtoken);};
[(]		{SqlciParse_IdentifierExpected = 0; return(LPAREN);};
[)]		{SqlciParse_IdentifierExpected = 0; return(RPAREN);};
[,]             {
                 SqlciParse_IdentifierExpected = 0; 
                 return_IDENT_or_TOKEN(COMMA, 0);
                };
[-]             {SqlciParse_IdentifierExpected = 0; return(HYPHEN);};

<LOGFNAME>[^ \t;()]+          {
                                if (yylval.stringval_type)
				  delete [] yylval.stringval_type;
                                yylval.stringval_type = new char[strlen(yytext)+1];
				strcpy(yylval.stringval_type, yytext);
                                BEGIN 0;
                                SqlciParse_IdentifierExpected = 0;
                                return_IDENT_or_TOKEN(FILENAME, 0);
			   }

[0-9]*		           {  
                                if (yylval.stringval_type)
                  delete [] yylval.stringval_type; 
                yylval.stringval_type = new char[strlen(yytext)+1];
                strcpy(yylval.stringval_type, yytext);
                SqlciParse_IdentifierExpected = 0;
                //		       return(NUMBER);
                return_IDENT_or_TOKEN(NUMBER, 0);
                           }

[?][A-Za-z_][A-Za-z0-9_]*  {
                                if (yylval.stringval_type)
                  delete [] yylval.stringval_type;
                yylval.stringval_type = new char[strlen(yytext)];
                strcpy(yylval.stringval_type, &yytext[1]);
                                SqlciParse_IdentifierExpected = 0;
                return(PARAM_NAME);
               }

[$][$][A-Za-z_][A-Za-z0-9_.]*[$][$] {
                                if (yylval.stringval_type)
                  delete [] yylval.stringval_type;
                yylval.stringval_type = new char[strlen(yytext)];
                strncpy(yylval.stringval_type, &yytext[2], strlen(&yytext[2])-2);
                yylval.stringval_type[strlen(&yytext[2])-2]  = '\0';
                                SqlciParse_IdentifierExpected = 0;
                return(PATTERN_NAME);
                 }

['][A-Za-z0-9_\-.\\$\*^\"/: %]*['] {
                                if (yylval.stringval_type)
                  delete [] yylval.stringval_type;
                yylval.stringval_type = new char[strlen(yytext)];
                strncpy(yylval.stringval_type, &yytext[1], strlen(yytext)-2);
                yylval.stringval_type[strlen(yytext)-2]  = '\0';
                
                                SqlciParse_IdentifierExpected = 0;
                return_IDENT_or_TOKEN(QUOTED_STRING, 0);
                 }
                 
["][A-Za-z0-9_\-.\\$\*^\"/: %]*["] {
                if (yylval.stringval_type)
                   delete [] yylval.stringval_type;
                yylval.stringval_type = new char[strlen(yytext+1)];
                strcpy(yylval.stringval_type, yytext);
                yylval.stringval_type[strlen(yytext)]  = '\0';
                
                SqlciParse_IdentifierExpected = 0;
                return_IDENT_or_TOKEN(DQUOTED_STRING, 0);
                 }

[A-Za-z_][A-Za-z0-9_.]*    {
                yylval.stringval_type = new char[strlen(yytext)+1];
                strcpy(yylval.stringval_type, yytext);
                if (SqlciParse_SyntaxErrorCleanup) 
                  { 
                    SqlciParse_IdentifierExpected = -1;
                    SqlciParse_SyntaxErrorCleanup=0; 
                  }
                else          
                  SqlciParse_IdentifierExpected = 0;
                //cout << "token = " << IDENTIFIER << endl;
                return(IDENTIFIER);
               }

                           
<FNAME>[^ \t;()]+          {
                                if (yylval.stringval_type)
                  delete [] yylval.stringval_type;
                                yylval.stringval_type = new char[strlen(yytext)+1];
                strcpy(yylval.stringval_type, yytext);
                                BEGIN 0;
                                SqlciParse_IdentifierExpected = 0;
                return_IDENT_or_TOKEN(FILENAME, 0);
                        }
\\{G}\.[0-9]*\,[0-9]*      {
                                if (yylval.stringval_type)
	                              delete [] yylval.stringval_type;
                                yylval.stringval_type = new char[strlen(yytext)+1];
				strcpy(yylval.stringval_type, yytext);
                                return(PID_VALUE);
                           }

\\{G}\.[0-9]*              {
                                if (yylval.stringval_type)
	                              delete [] yylval.stringval_type;
                                yylval.stringval_type = new char[strlen(yytext)+1];
				strcpy(yylval.stringval_type, yytext);
                                return(CPU_VALUE);
                           }

\-\-.*$                    {
                /* no action (comment) */
                           }

[!]             {SqlciParse_IdentifierExpected = -1; return(REPEAT);};

. 		{SqlciParse_IdentifierExpected = 0; return(ERROR_STMT);};

%%

void SqlciLexReinit()
{
    sqlcirestart(0);
}
