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

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         sqlci_yacc.y
 * Description:  Parser for mxci commands.
 *
 *
 * Created:      7/10/95
 * Modified:     $ $Date: 2006/11/01 01:44:52 $ (GMT)
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


%{
#include "Platform.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "ComAnsiNamePart.h"
#include "ParserMsg.h"			// StoreSyntaxError(...ComDiagsArea&...)
#include "Sqlci.h"
#include "str.h"
#include "nawstring.h"
#define   SQLCIPARSEGLOBALS__INITIALIZE
#include "SqlciParseGlobals.h"
#include "HeapLog.h"
#include "charinfo.h"
#include "conversionHex.h"
#include "ComDistribution.h"
#include "ComObjectName.h"
#include "ComSchemaName.h"

  extern char **environ;
#define ENVIRON environ
#define PUTENV putenv

extern NAHeap sqlci_Heap;

  extern "C" {int yylex (void); }

extern ComDiagsArea sqlci_DA;
extern char *sqlcitext;

static char *identifier_name_internal;
static char *identifier_name_internal2;
static int pos_internal;
static int pos_internal2;
static int pos_internal3;
static char *get_stats_str;

//extern "C" { void yyerror(const char *sb); };


static void sqlcierror(const char *)		// (const char *errtext)
{
  // Re-initialize the following variables which are used in sqlci_lex.l
  SqlciParse_SyntaxErrorCleanup = 0;
  SqlciParse_IdentifierExpected = 0;

  if (!SqlciParse_HelpCmd)
    {
      // So syntax errmsg caret points properly, when input is an extra
      // dangling #else		or	#endif --
      //   emit   ^    		not	  ^
      if (SqlciParse_InputPos <= 2)
        if (*SqlciParse_InputStr == '#')
	  SqlciParse_InputPos = 0;
      SqlciError (SQLCI_SYNTAX_ERROR, (ErrorParam *) 0);
      StoreSyntaxError(SqlciParse_InputStr, SqlciParse_InputPos, sqlci_DA, 0);
    }
} // sqlcierror

enum EQuotedStringWithoutPrefixFlag { eNOT_QUOTED_STRING_WITHOUT_CHARSET_PREFIX = 0,
                                      eQUOTED_STRING_WITHOUT_CHARSET_PREFIX = 1 };

static short trimValueText(int/*offset*/ &i,
			   int/*length*/ &len,
			   int/*bool*/   &quoted,
                           CharInfo::CharSet /*charset code*/ &cs,
                           int/* bool*/ &inhexdecimal,
			   int /*bool*/    quoting_allowed = -1,
			   int/*bool*/	  use_i_as_is = 0,
			   int/*bool*/	  cs_allowed = 0,
                           EQuotedStringWithoutPrefixFlag * pQuotedStrWithoutPrefixFlag = NULL
                           , int/*bool*/ keepValueTextAsIs = 0/*false*/
                          )
{
  // Process the value text:
  // remove the trailing semicolon and any squotes (')
  // but leave in any dquotes (").
  //
  // Returns TRUE if syntax error (error in quoted string, basically --
  // which should be already caught by class InputStmt; see its logic
  // around SQLCI_INPUT_MISSING_QUOTE).

  if (pQuotedStrWithoutPrefixFlag != NULL)
    *pQuotedStrWithoutPrefixFlag = eNOT_QUOTED_STRING_WITHOUT_CHARSET_PREFIX;

  quoted = 0;
  cs =  CharInfo::UnknownCharSet;
  inhexdecimal = 0;

  if (!use_i_as_is)
    {
      i = SqlciParse_InputPos - 1;

      // Now the i'th character is a separator char that Lex found,
      // separating the Define name from its value.
      // The following test allows
      //	SET ENVVAR nam;
      //	SET ENVVAR nam ;
      //	SET ENVVAR nam val;
      //	SET ENVVAR nam 'val';
      //	SET ENVVAR nam "val";
      //	SET ENVVAR nam'val';	-- on the analogy of Ansi syntax which
      //	SET ENVVAR nam"val";	-- allows SELECT * FROM tbl"corrName";
      //	SET ENVVAR nam :val;	-- value is  :val
      //	SET ENVVAR nam =val;	-- value is  =val
      // but not
      //	SET ENVVAR nam:val;
      //	SET ENVVAR nam=val;
      //
      if (SqlciParse_OriginalStr[i] != ';'    &&
          SqlciParse_OriginalStr[i] != '\''   &&
	  SqlciParse_OriginalStr[i] != '\"'   &&
	  !isspace(SqlciParse_OriginalStr[i]) &&
	  !isspace(SqlciParse_OriginalStr[i-1]))
	return -1;				// error: space separator needed
    }

  // skip leading blanks
  while (isspace(SqlciParse_OriginalStr[i])) i++;

  // remember the start position of '_' in case we need to go back to position i
  int i_orig = i; 
  NAString origValueText(&SqlciParse_OriginalStr[i_orig]); // for the keepValueTextAsIs case

  // determine if the value is has a valid character set prefix
  if ( cs_allowed && SqlciParse_OriginalStr[i] == '_' )
  {
     int k = i+1;
     char c;
     while ((c=SqlciParse_OriginalStr[k])) {
       if ( c == '\''|| c == ' ' ) // quoted or hexadecimal value
       {
         SqlciParse_OriginalStr[k] = '\0'; // temp. null terminate the prefix
         NAString csPrefix(&SqlciParse_OriginalStr[i+1]);
         SqlciParse_OriginalStr[k] = c; // restore

         csPrefix.toUpper();
         cs = CharInfo::getCharSetEnum(csPrefix); // check the prefix
         if ( CharInfo::isCharSetFullySupported(cs) ) {
            i = k; // mark the start of the string after the prefix
         }  
         break;
       }
       k++;
     }

     // A string in hexadecimal format begins with ' x' or ' X'.
     if ( SqlciParse_OriginalStr[i] == ' ' && 
          SqlciParse_OriginalStr[i+1] &&
          toupper(SqlciParse_OriginalStr[i+1]) == 'X' ) {
        inhexdecimal = 1; // mark the hexadecimal format.
        i += 2;           // skip ' x'. Suppose to point at '. 
     }
  } else {
     // no cs name prefix. Check if it is a hexadecimal string.
     if ( toupper(SqlciParse_OriginalStr[i]) == 'X' &&
          SqlciParse_OriginalStr[i+1] == '\''
        ) {
        inhexdecimal = 1; // mark the hexadecimal format.
        i += 1;           // skip 'x'. Suppose to point at '. 
     }
  }

  char delimQuote = '\'';
  if (SqlciParse_OriginalStr[i] == '\'' ||	// quoted value
      SqlciParse_OriginalStr[i] == '"')
    {
      delimQuote = SqlciParse_OriginalStr[i];
      quoted = -1;

      // Remove the leading single quote as procedure comments indicate. 
      NAString temptrail(&SqlciParse_OriginalStr[i+1]);
      strcpy(&SqlciParse_OriginalStr[i], temptrail);
    } else
       i = i_orig; // need to abadone the charset name assumption.
    
  SqlciParse_InputPos = i;			// for sqlcierror message
  if (quoted && !quoting_allowed) return -1;	// error

  int j = strlen(SqlciParse_OriginalStr) - 1;

  // skip any blanks between end of SqlciParse_OriginalStr and the semicolon.
  while (isspace(SqlciParse_OriginalStr[j])) j--;

  j--;						// skip semicolon

  while (j >= i &&				// skip trailing blanks
	 isspace(SqlciParse_OriginalStr[j])) j--;

  if (j < i && quoted) return -1;		// error: unmatched begin quote

  if (SqlciParse_OriginalStr[j] == delimQuote)
    {
      if (!quoted) return -1;			// error: unmatched end quote
      quoted = +1;
      j--;
    }
  if (quoted < 0) return -1;			// error: unmatched begin quote

  if (j >= i)
    if (quoted)
      {
	// if nonempty string within quotes, convert any embedded pair of quotes
	// (i.e. a quoted quote) into one quote, by left shifting
	//
	// **this modifies SqlciParse_OriginalStr (which points to the same
	// **buffer as SqlciParse_InputStr)!

	//#ifdef NA_WINNT
   SqlciParse_OriginalStr[j+1] = '\0';	// mark end of value text
   //#else
   // Remove the trailing single quote, but save the semi-colon on the TANDEM
   // platform.
   //   SqlciParse_OriginalStr[j+1] = '\;';	// replace quote with a semi-colon
   //   SqlciParse_OriginalStr[j+2] = '\0';	// mark end of string
   //#endif // NA_WINNT

	for (j = i; SqlciParse_OriginalStr[j]; j++)
	  if (SqlciParse_OriginalStr[j] == delimQuote)
	    {
	      if (SqlciParse_OriginalStr[j+1] != delimQuote)
		return -1;			// error: unmatched embedded q
                
              NAString temptrail(&SqlciParse_OriginalStr[j+1]);
 
	      strcpy(&SqlciParse_OriginalStr[j], temptrail);
	    }
	  j--;
      }
    else if (quoting_allowed)
      {
        // if nonempty nonquoted string -- but quoting allowed --
	// then any embedded quotes or blanks are an error
	for (int k = i; k <= j; k++)
	  if (SqlciParse_OriginalStr[k] == delimQuote ||
	      isspace(SqlciParse_OriginalStr[k]))
	    {
	      SqlciParse_InputPos = j;		// for sqlcierror message
	      return -1;			// error
	    }
      }
    //else embedded quotes allowed and passed thru unchanged

  if (keepValueTextAsIs)
  {
    TrimNAStringSpace(origValueText, 0/*false*/ /*leading?*/, 1/*true*/ /*trailing?*/);
    if (origValueText.data()[origValueText.length()-1] == ';')
      origValueText.remove(origValueText.length()-1);
    TrimNAStringSpace(origValueText, 0/*false*/ /*leading?*/, 1/*true*/ /*trailing?*/);
    strcpy(&SqlciParse_OriginalStr[i_orig], origValueText.data());
    len = origValueText.length();
  }
  else
    len = j - i + 1;
  if (len < 0) len = 0;
 
  if ( quoted ) {
    if ( cs == CharInfo::UnknownCharSet ) {
      if (pQuotedStrWithoutPrefixFlag != NULL)
        *pQuotedStrWithoutPrefixFlag = eQUOTED_STRING_WITHOUT_CHARSET_PREFIX;
      cs = SqlciEnvGlobal->getTerminalCharset();
    }

  } else 
    inhexdecimal = 0; // reset the hex indicator if the value is not quoted.

  // Consumed entire input (need this for pre-YYACCEPT test in sqlci_cmd).
  // Of course, a subsequent call to this routine on the same string will
  // have to use the use_i_as_is parameter and pass in a valid i ...
  //
  SqlciParse_InputPos = strlen(SqlciParse_OriginalStr) + 1;

  return 0;					// no error

} // trimValueText

static char * FCString (const char *idString, int isFC)
{
  char * cmd_str = new char[strlen(SqlciParse_OriginalStr)];
  cmd_str[0]='\0';

  // Skip leading blanks
  int i = 0;
  while (isspace(SqlciParse_OriginalStr[i]))
    i++;

  const char *s;  
  if (isFC)
    // i+2 - to skip the two characters of 'fc'
    s = strstr(&SqlciParse_OriginalStr[i+2], idString);
  else 
    s = strstr(&SqlciParse_OriginalStr[i], idString);
  
  if (s)
  {
    for ( ; *s; s++)
    {
  	  if (*s != ';')
      {
	     strncat (cmd_str, s, 1);
      }
    }
  }

  return cmd_str;
}

#define REPOSITION_SqlciParse_InputPos \
  	  { SqlciParse_InputPos = strlen(SqlciParse_InputStr); }

%}
%token NOSQL
%token CONTAINSQL
%token READSQL
%token MODIFYSQL
%token RESETVIOLATION
%token CHECKVIOLATION
%token NoAutoXact
%token DESCRIBEToken
%token CREATECONTEXT
%token CURRENTCONTEXT
%token SWITCHCONTEXT
%token DELETECONTEXT
%token RESETCONTEXT
%token ADDtoken
%token ALLtoken
%token BACKSLASH
%token BRIEF
%token DETAIL
%token CANCEL
%token CLASStoken
%token CLEANUP
%token CLEAR
%token COMMA
%token COMMANDStoken
%token CONTROL
%token OSIM
%token COPY
%token CPU_VALUE
%token ERRORtoken 
%token DEFAULTtoken
%token DEFAULT_CHARSETtoken
%token DISPLAY
%token DISPLAY_STATISTICS
%token DISPLAY_QUERYID
%token DISPLAY_EXPLAIN
%token DISPLAY_QC
%token DISPLAY_QC_ENTRIES
%token DISPLAY_QC_ENTRIES_NOTIME
%token DISPLAY_USE_OF
%token EDIT
%token MXCI_TOK_ENV
%token EXIT
%token EXPLAIN
%token FIRST
%token FC
%token FILEtoken
%token FILEINFO
%token FILENAMES
%token FILES
%token GENERATEtoken
%token MERGEtoken
%token METADATAtoken
%token REPLICATEtoken
%token GETtoken
%token GETSTATISTICStoken
%token HISTORY
%token HYPHEN
%token INFER_CHARSET
%token INFOtoken
%token INtoken
%token INFILE
%token SHOWSTATS
%token SHOWTRANSACTION
%token INVOKE
%token LISTCOUNT
%token LISTMODULES
%token LOADtoken
%token EXTRACTtoken
%token LOCK
%token LOCKINGtoken
%token LPAREN
%token MAPtoken
%token MIGRATE
%token MODE
%token MODIFY
%token MODIFYV
%token MSCKtoken
%token NEXT
%token NOEtoken
%token OBEY
%token OBJECTtoken
%token OFtoken
%token OFF
%token ON
%token OPTIONStoken
%token ISO_MAPPING
%token OUTFILE
%token PARAM
%token PARSERFLAGS
%token SQ_LINUX_PATTERN
%token PATTERN_AS_IS
%token PID_VALUE
%token PROCEDUREtoken
%token PURGEDATA
%token POPULATE
%token VALIDATEtoken
%token QUIESCE
%token REFRESH
%token REPEAT
%token REPORT
%token RESET
%token RESULT
%token RPAREN
%token SETtoken
%token SETENV
%token SHOW
%token SHOWCONTROL
%token SHOWDDL
%token SHOWPLAN
%token SHOWSHAPE
%token SHOWSET
%token SQL
%token SQLINFO
%token SQLNAMES
%token STATEMENTtoken
%token STOREtoken
%token SYNTAX
%token SYSTEMtoken
%token EXAMPLE
%token STATISTICS
%token SET_TABLEtoken   
%token SET_TRANSACTIONtoken
%token TERMINAL_CHARSET
%token TRANSFORM
%token TRUNCATE
%token WITH 
%token UNLOCK
%token UPD_STATS
%token UPD_HIST_STATS
%token USERtoken
%token USING
%token TABLE
%token VALUES
%token VALIDATE
%token VERBOSE
%token VERIFY
%token VERSIONtoken
%token WAITtoken
%token DEFINEtoken ENVVARtoken PREPARED SESSIONtoken
%token LOG LS CD SH SHELL
%token SELECTtoken UPDATEtoken INSERTtoken DELETEtoken UPSERTtoken
%token ROWSETtoken REPOSITORYtoken
%token CREATE ALTER DROP PREPAREtoken EXECUTEtoken FROM
%token DECLAREtoken OPENtoken CLOSEtoken FETCHtoken DEALLOCtoken 
%token CURSORtoken FORtoken CPU PID QID ACTIVEtoken ACCUMULATEDtoken
%token PERTABLEtoken PROGRESStoken 
%token TOK_BEGIN COMMIT ROLLBACK WORK
%token SQLCI_CMD SQL_STMT UTIL_STMT EXIT_STMT ERROR_STMT SHELL_CMD
%token IDENTIFIER NUMBER PARAM_NAME PATTERN_NAME FILENAME
%token QUOTED_STRING
%token DQUOTED_STRING
%token GRANTtoken REVOKEtoken
%token REGISTER UNREGISTER
%token GIVE 
%token INITIALIZE REINITIALIZE
%token CATALOG SCHEMA
%token HIVEtoken
%token PROCESStoken
%token IF WHILE
%token MPLOC
%token NAMETYPE
%token SIGNAL
%token UIDtoken
%token CURSORWITH
%token WITHOUT
%token HOLD
%token INTERNAL  
%token MVLOG
%token UNLOAD   

%token CALLToken
%token COMMENTtoken

%union {
	 enum ComRoutineSQLAccess sql_access_mode_type;
	 SqlCliCmd * sqlcli_cmd_type;
         SqlciCmd * sqlci_cmd_type;
         SqlciNode * sql_cmd_type;
	 ShellCmd * shell_cmd_type;
         int intval_type;
	 char * stringval_type;
	 dml_type dml_stmt_type;
	 Reset::reset_type reset_stmt_type_;
	 Show::show_type show_stmt_type_;
         SqlciCursorInfo *cursor_info_type_;
       }

%type <sql_access_mode_type> sql_access_mode;
%type <sqlcli_cmd_type> sqlcli_cmd
%type <sqlci_cmd_type> sqlci_cmd
%type <sql_cmd_type> sql_cmd
%type <shell_cmd_type> shell_cmd
%type <stringval_type> IDENTIFIER
%type <stringval_type> PARAM_NAME
%type <stringval_type> PATTERN_NAME
%type <stringval_type> PID_VALUE
%type <stringval_type> CPU_VALUE
%type <stringval_type> QUOTED_STRING
%type <stringval_type> DQUOTED_STRING
%type <stringval_type> NUMBER
%type <stringval_type> FILENAME
%type <dml_stmt_type> dml_type
%type <dml_stmt_type> dml_simple_table_type
%type <reset_stmt_type_> reset_type_;
%type <show_stmt_type_> show_type_;
%type <stringval_type> section_name_;
%type <stringval_type> HYPHEN;
%type <intval_type>    commands_only;
%type <intval_type>    stats_active_clause;
%type <intval_type>    stats_merge_clause;
%type <stringval_type> explain_options;
%type <cursor_info_type_> cursor_info;
%type <intval_type> optional_rs_index;
%type <stringval_type> qid_identifier;

%start statement
%%
statement :	sqlcli_cmd
		{

		   SqlciParseTree = (SqlciNode *) $1;
		   YYACCEPT;

		}

	|	sqlci_cmd
		{
			// Accept if we're at/past the end of the cmd
			// or if only blanks and semicolons remain;
			// otherwise, syntax error.
			//
			unsigned pos = SqlciParse_InputPos-1;
			if (pos < strlen(SqlciParse_InputStr))
			  {
			    const char *s = &SqlciParse_InputStr[pos];
			    for ( ; *s; s++)
			      if (!isspace(*s) && *s != ';')
				{sqlcierror("");YYERROR;}
			  }



		   	SqlciParseTree = (SqlciNode *) $1;
			YYACCEPT;
		}

	|	sql_cmd
		{
		   	SqlciParseTree = (SqlciNode *) $1;
			YYACCEPT;
		}

	|	shell_cmd
		{
		   	SqlciParseTree = (SqlciNode *) $1;
			YYACCEPT;
		}
;

sqlcli_cmd :	CHECKVIOLATION
		{
		  $$ = new CheckViolation();
		}
	    |	RESETVIOLATION sql_access_mode
		{
		  $$ = new ResetViolation($2);
		}
	    |	CREATECONTEXT
		{
		  $$ = new CreateContext();
		}
	    |	CREATECONTEXT NoAutoXact
		{
		  $$ = new CreateContext(TRUE/* noAutoXact */);
 		}
            |   CURRENTCONTEXT
                {
		  $$ = new CurrentContext();
		}
	    |	SWITCHCONTEXT NUMBER
		{
		  $$ = new SwitchContext(atoi($2));
		}

	    |	DELETECONTEXT NUMBER
		{
		  $$ = new DeleteContext(atoi($2));
		}
	    |	RESETCONTEXT NUMBER
		{
		  $$ = new ResetContext(atoi($2));
		}
;

sql_access_mode :   NOSQL
		    {
		      $$ = COM_NO_SQL;
		    }
		|   CONTAINSQL
		    {
		      $$ = COM_CONTAINS_SQL;
		    }
		|   READSQL
		    {
		      $$ = COM_READS_SQL;
		    }
		|   MODIFYSQL
		    {
		      $$ = COM_MODIFIES_SQL;
		    }
;

sqlci_cmd :	MODE SQL
		  {
			$$ = new Mode (Mode::SQL_ , TRUE);
		  }	
		|   OBEY FILENAME section_name_
                  { 
			$$ = new Obey($2, strlen($2), $3);
		  }
		|	MXCI_TOK_ENV
		  { 
			$$ = new Env(0,0);
		  }
                |   USERtoken IDENTIFIER
                  {
                    char userName[strlen($2)+1];
                    for (size_t i=0; i < strlen($2); i++)
                      {
                        userName[i] = toupper($2[i]);
                      }
                    userName[strlen($2)] = 0;
                    $$ = new ChangeUser(userName, strlen(userName));
                  }
		|	REPEAT
          	  {
		    // "!" command, a la SQL/MP aRepeat (0,0);
			$$ = new FCRepeat (0,(short)0);
		  }
		|	REPEAT REPEAT
          	  {
		    // "!!" command, a la Unix csh
		    $$ = new FCRepeat (0,(short)0);
		  }
        |   REPEAT NUMBER
          {
		    $$ = new FCRepeat (atoi($2), 0);
		  }
        |   REPEAT HYPHEN NUMBER
          {
		    $$ = new FCRepeat (atoi($3), -1);
		  }
        |   REPEAT IDENTIFIER
          {
		   char *fcString = FCString ($2, 0);
               	   $$ = new FCRepeat(fcString, (long)strlen(fcString));
                   REPOSITION_SqlciParse_InputPos;
          }
        |   REPEAT LPAREN NUMBER RPAREN
          {
		    $$ = new FCRepeat (atoi($3), 0);
	  }
        |   FC
          {
		    $$ = new FixCommand(0, (short)0);
	  }
	|   FC NUMBER
          {
		    $$ = new FixCommand(atoi($2), 0);
	  }
	|  FC HYPHEN NUMBER
          {
		    $$ = new FixCommand(atoi($3), -1);
	  }
	| FC IDENTIFIER
          {
		   char *fcString = FCString ($2, 1);
	           $$ = new FixCommand(fcString, (long)strlen(fcString));
	           REPOSITION_SqlciParse_InputPos;
          }
		| FC LPAREN NUMBER RPAREN
          {
		   $$ = new FixCommand(atoi($3), 0);
          }
		| LOG IDENTIFIER commands_only
         {
		   identifier_name_internal = new char[strlen($2)+1];
		   strcpy(identifier_name_internal, $2);
		 }
		 CLEAR
         { 
			$$ = new Log(identifier_name_internal, strlen(identifier_name_internal), Log::CLEAR_, $3);
		 }
	    |	LOG IDENTIFIER commands_only 
		 { 
			$$ = new Log($2, strlen($2), Log::APPEND_, $3);
		 }
    
		|	LOG FILENAME commands_only
         {
		   identifier_name_internal = new char[strlen($2)+1];
		   strcpy(identifier_name_internal, $2);
		 }
		 CLEAR
           {
	   $$ = new Log(identifier_name_internal, strlen(identifier_name_internal), Log::CLEAR_, $3);
		 }

		|	LOG FILENAME commands_only

		 { 
			$$ = new Log($2, strlen($2), Log::APPEND_, $3);
		 }

		|	LOG
		  { 
			$$ = new Log(0,0, Log::STOP_, 0);
		  }

		|	HISTORY
		  { 
			$$ = new History(0,0);   
		  }

		|	HISTORY NUMBER
		  { 
			$$ = new History($2,strlen($2));
		  }

		|	EXIT
                  { 
			$$ = new Exit(0,0);
		  }
        |       ERRORtoken NUMBER
                  { 
		  $$ = new Error($2, strlen($2), Error::DETAIL_);
		  }
        |       ERRORtoken NUMBER COMMA BRIEF
                  { $$ = new Error($2, strlen($2), Error::BRIEF_);
		  }
        |       ERRORtoken NUMBER COMMA DETAIL
                  { $$ = new Error($2, strlen($2), Error::DETAIL_);
		  }

	|	SETtoken PARAM PARAM_NAME
		  {
		    int i, len, quoted;
                    CharInfo::CharSet cs;
                    int inhexadecimal;
                    EQuotedStringWithoutPrefixFlag quotedStrWithoutPrefixFlag;

		    if (trimValueText(i, len, quoted, cs, inhexadecimal, -1, 0, -1,
                                      &quotedStrWithoutPrefixFlag)) {sqlcierror("");YYERROR;}
		    //if (len <= 0 && !quoted) 	       {sqlcierror("");YYERROR;}

		    // see if input was of form "set param ?p null;".
		    // If yes, this param is a null value.
		    short null_param_val = 0;
		    if (len == 4 && !quoted)
		      {
			char buf[4];
			for (int k = 0; k < 4; k++)
			  buf[k] = toupper(SqlciParse_OriginalStr[i+k]);

			null_param_val = (strncmp(buf, "NULL", 4) == 0);
		      }

		    if (null_param_val)
		      $$ = new SetParam($3, strlen($3), (char*)0, -1);
		    else {
                      char* pvalue = &SqlciParse_OriginalStr[i];

                      if ( inhexadecimal ) {

                         NAWString pvalue_in_wchar(CharInfo::ISO88591, pvalue);

                         void* result = NULL;
                         enum hex_conversion_code code = 
                           verifyAndConvertHex(pvalue_in_wchar, pvalue_in_wchar.length(), 
                                               L'\'', cs, &sqlci_Heap, result);

                         switch ( code ) {

                           case SINGLE_BYTE:
                           {
                             NAString* conv_pvalue = (NAString*)result;

		             $$ = new SetParam($3, strlen($3), 
                                               (char*)(conv_pvalue->data()), conv_pvalue->length(), cs);
                             if (quotedStrWithoutPrefixFlag == eQUOTED_STRING_WITHOUT_CHARSET_PREFIX)
                             {
                               NAWString wstr(SqlciEnvGlobal->getTerminalCharset(),
                                              conv_pvalue->data(), conv_pvalue->length());
                               SetParam * pSetParam = (SetParam *)$$;
                               pSetParam->setUTF16ParamStrLit(wstr.data(), wstr.length());
                               pSetParam->setQuotedStrWithoutPrefixFlag(TRUE);
                               pSetParam->setTermCharSet(SqlciEnvGlobal->getTerminalCharset());
                             }
// NADELETE() is not a const func while this routine is. Mask out the warning
                             NADELETE(conv_pvalue, NAString, &sqlci_Heap);
                           } 
                           break;

                           case DOUBLE_BYTE:
                           {
                             NAWString* conv_pvalue = (NAWString*)result;
		             $$ = new SetParam($3, strlen($3), 
                                               (NAWchar*)(conv_pvalue->data()), conv_pvalue->length(), cs);
// NADELETE() is not a const func while this routine is. Mask out the warning
                             NADELETE(conv_pvalue, NAWString, &sqlci_Heap);
                           } 
                           break;

                           default:
                             sqlcierror("");YYERROR;
                         }
                      } else {

                        switch (cs) {
                           case CharInfo::UNICODE:
                            {
                               NAWString wstr(SqlciEnvGlobal->getTerminalCharset(), pvalue, len); 
		               $$ = new SetParam($3, strlen($3), (NAWchar*)wstr.data(), wstr.length(), cs);
                            }
                            break;

                           case CharInfo::KANJI_MP:
                           case CharInfo::KSC5601_MP:

                            if ( len % 2 != 0 )
                              { 
                                // restore the closing two single quotes and ';' removed 
                                // inside the trimValueText() call. The string starts 
                                // at pvalue. We guarantee to have enough space here 
                                // because trimValueText() will bail out if no single 
                                // quotes present.

                                // We need to restore the closing single quotes and ';'
                                // so that in the second attempt of sqlci cmd parsing,
                                // the error can be caught again. Otherwise, _kanji'a' 
                                // becomes _kanjia after first parsing attempt. That
                                // string is legal and is different than the one actually
                                // typed. 

                                // It is difficult to remove the KLRUGE of 2nd parsing
                                // attempt in sqlciparser.cpp, func sqlci_parser().
                                // A lot of regressions will fail if we do so.
                                int k = strlen(pvalue);

                                for ( int x=k; x>=1; x-- ) {
                                  pvalue[x] = pvalue[x-1];
                                }
                                *pvalue = '\'';
                                *(pvalue+k+1) = '\'';
                                *(pvalue+k+2) = ';';
                                *(pvalue+k+3) = '\0';

                                sqlcierror("");YYERROR; 
                              }

		            $$ = new SetParam($3, strlen($3), (NAWchar*)pvalue, len/2, cs);
                            break;

                            // case CharInfo::ISO88591:
                            // case CharInfo::UTF8:
                            // case CharInfo::SJIS:
                            // case CharInfo::GB2312:
                            // case CharInfo::GBK:
                            // case CharInfo::...:
                           default:
                            {
                              SetParam * pSetParam = NULL;
                              if (quotedStrWithoutPrefixFlag == eQUOTED_STRING_WITHOUT_CHARSET_PREFIX)
                              {
                                // SqlciEnvGlobal->getInferCharset() == TRUE || FALSE
                                pSetParam = new SetParam($3, strlen($3), pvalue, len, cs);
                                NAWString wstr(cs, pvalue, len);
                                pSetParam->setUTF16ParamStrLit(wstr.data(), wstr.length());
                                pSetParam->setQuotedStrWithoutPrefixFlag(TRUE);
                                pSetParam->setTermCharSet(SqlciEnvGlobal->getTerminalCharset());
                              }
                              else
                                pSetParam = new SetParam($3, strlen($3), pvalue, len, cs);
                              $$ = pSetParam;
                            }
                            break;
                            
                        }
                      }
                    }
		  }

	|	SETtoken SQ_LINUX_PATTERN PATTERN_NAME
		  {
		    int i, len, quoted;
                    CharInfo::CharSet cs;
                    int inhexadecimal;
		    if (trimValueText(i, len, quoted, cs, inhexadecimal)) {sqlcierror("");YYERROR;}
		    $$ = new SetPattern($3, strlen($3),
					&SqlciParse_OriginalStr[i], len);
		  }

	|	SETtoken PATTERN_AS_IS PATTERN_NAME
		  {
		    int i, len, quoted;
		    CharInfo::CharSet cs;
		    int inhexadecimal;
		    EQuotedStringWithoutPrefixFlag quotedStrWithoutPrefixFlag;
		    if ( trimValueText( i, len, quoted, cs, inhexadecimal
		                      , -1/*true*/ // quoting_allowed?
		                      , 0/*false*/ // use_i(1st_param)_as_is?
		                      , -1/*true*/ // cs_allowed?
		                      , &quotedStrWithoutPrefixFlag
		                      , -1/*true*/ // keepValueTextAsIs
		                      ) )
		    {sqlcierror("");YYERROR;}
		    $$ = new SetPattern($3, strlen($3),
					&SqlciParse_OriginalStr[i], len);
		  }

        |       SETtoken SHOWSHAPE ON
                  {
		    $$ = new Shape(TRUE, NULL, NULL);
		  }

        |       SETtoken SHOWSHAPE OFF
                  {
		    $$ = new Shape(FALSE, NULL, NULL);
		  }

        |       SETtoken SHOWSHAPE INFILE FILENAME 
                  {
		    identifier_name_internal = new char[strlen($4)+1];
		    strcpy(identifier_name_internal, $4);
		  }
                OUTFILE FILENAME
                  {
		    $$ = new Shape(FALSE, identifier_name_internal, $7);
                  }

	|       SETtoken STATISTICS OFF
                  {
		    $$ = new Statistics(0,0,Statistics::SET_OFF, NULL);
		  }

        |       SETtoken STATISTICS ON
                  {
		    // display statistics in old format
		    $$ = new Statistics(NULL, 0, Statistics::SET_ON,
					(char *)"of");
             	  }

        |       SETtoken STATISTICS PERTABLEtoken
                  {
		    $$ = new Statistics(NULL, 0, Statistics::SET_ON,
					(char *)"PERTABLE");
		  }

        |       SETtoken STATISTICS PROGRESStoken
                  {
		    $$ = new Statistics(NULL, 0, Statistics::SET_ON,
					(char *)"PROGRESS");
		  }

        |       SETtoken STATISTICS DEFAULTtoken
                  {
		    $$ = new Statistics(NULL, 0, Statistics::SET_ON,
					(char *)"DEFAULT");
		  }

        |       SETtoken STATISTICS ALLtoken
                  {
		    $$ = new Statistics(NULL, 0, Statistics::SET_ON,
					(char *)"ALL");
		  }


        |       SETtoken STATISTICS ON COMMA GETtoken STATISTICS
                  {
		    // display stats in new format
		    $$ = new Statistics(NULL, 0, Statistics::SET_ON, NULL);
		  }

        |       SETtoken STATISTICS ON COMMA GETtoken STATISTICS COMMA OPTIONStoken QUOTED_STRING
                  {
		    // display stats in new format based on options in
		    // quoted_string
		    $$ = new Statistics(NULL, 0, Statistics::SET_ON,
					$9);
                  }

        |       SETtoken LISTCOUNT NUMBER
                  {
                    $$ = new ListCount($3, strlen($3));
                  }

        |       SETtoken LISTCOUNT
		  {
		    if ((sqlcitext) && (sqlcitext[0] != ';'))
		      {sqlcierror("");YYERROR;}

		    $$ = new ListCount(0, 0);
		  }

	|	SETtoken VERBOSE ON
		{
		    $$ = new Verbose(0, 0, Verbose::SET_ON);
		}

	|	SETtoken VERBOSE OFF
		{
		    $$ = new Verbose(0, 0, Verbose::SET_OFF);
		}

	|	SETtoken TERMINAL_CHARSET IDENTIFIER
		{
		    $$ = new SetTerminalCharset($3);
		}
	|	SETtoken ISO_MAPPING IDENTIFIER
		{
		    $$ = new SetIsoMapping($3);
		}

	|	SETtoken DEFAULT_CHARSETtoken IDENTIFIER
		{
		    $$ = new SetDefaultCharset($3);
		}

	|	SETtoken INFER_CHARSET NUMBER
		{
		    $$ = new SetInferCharset($3);
		}

	|	SETtoken INFER_CHARSET IDENTIFIER
		{
		    $$ = new SetInferCharset($3);
		}

	|	SETtoken INFER_CHARSET ON
		{
		    $$ = new SetInferCharset((char *)"TRUE");
		}

	|	SETtoken INFER_CHARSET OFF
		{
		    $$ = new SetInferCharset((char *)"FALSE");
		}

        |      SETtoken QID IDENTIFIER
                {
                  // save qid val.
                   identifier_name_internal = new char[strlen($3)+1]; 
                   str_cpy_convert(identifier_name_internal, $3, strlen($3), TRUE); 
                   identifier_name_internal[strlen($3)] = 0;
                }
              FORtoken 
                {
                  pos_internal = SqlciParse_InputPos;
                }
              IDENTIFIER
                {
                  // remove trailing ";"
                  char * id = &SqlciParse_OriginalStr[pos_internal];
                  Lng32 i = strlen(&SqlciParse_OriginalStr[pos_internal]) -1;
                  while ((i > 0) && (id[i] != ';'))
                    i--;
                  id[i] = 0;

                  char * id2 = new char[strlen(id)+1];
                  str_cpy_convert(id2, id, strlen(id), TRUE);
                  id2[strlen(id)] = 0;
                  $$ = new QueryId (id2, strlen(id2),
                                    TRUE, identifier_name_internal);
                }

	|	RESET PARAM PARAM_NAME
		  {
		    $$ = new Reset(Reset::PARAM_, $3, strlen($3));
		  }
	|	RESET SQ_LINUX_PATTERN PATTERN_NAME
		  {
		    $$ = new Reset(Reset::PATTERN_, $3, strlen($3));
		  }
	|	RESET reset_type_
		  {
		    if ((sqlcitext) && (sqlcitext[0] != ';'))
		      {sqlcierror("");YYERROR;}

		    $$ = new Reset($2);
		  }

	|	SHOW show_type_
		  {
		    $$ = new Show($2, TRUE);
		  }
    |       DISPLAY_QUERYID FORtoken IDENTIFIER
                  {
                    identifier_name_internal = new char[strlen($3)+1]; 
                    str_cpy_convert(identifier_name_internal, $3, strlen($3), TRUE); 
                    identifier_name_internal[strlen($3)] = 0;
                    $$ = new QueryId (identifier_name_internal,strlen(identifier_name_internal),
                                      FALSE, NULL);
                  }
    |       DISPLAY_QUERYID
                  {
                    $$ = new QueryId (0,0, FALSE, NULL);
                  }


        |       SHOW PREPARED
		  {
		    if ((sqlcitext) && (sqlcitext[0] != ';'))
		      {sqlcierror("");YYERROR;}

		    $$ = new Show(Show::PREPARED_, TRUE);
		  }

        |       SHOW PREPARED ALLtoken
		  {
		    $$ = new Show(Show::PREPARED_, TRUE);
		  }

        |       WAITtoken
                  {
                    $$ = new Wait(NULL, 0);
                  }

;

commands_only :
               COMMANDStoken   { $$ = 1; }
             | empty { $$ = 0; }
;

empty :
;

section_name_:
	        LPAREN IDENTIFIER RPAREN  {$$ = $2;}
     	|	                          {$$ = 0;}
;

reset_type_ :
                PARAM   	{$$ = Reset::PARAM_;}
;

show_type_ :
                CURSORtoken     {$$ = Show::CURSOR_;}
        |       PARAM    	{$$ = Show::PARAM_;}
        |       SQ_LINUX_PATTERN  	{$$ = Show::PATTERN_;}
        |       SESSIONtoken  	{$$ = Show::SESSION_;}
        |       VERSIONtoken  	{$$ = Show::VERSION_;}
;


stats_active_clause :
               ACTIVEtoken NUMBER       { $$ = atoi($2); }
             | ALLtoken                 { $$ = 0; }
             | empty                    { $$ = 1; }
;

stats_merge_clause :
               ACCUMULATEDtoken       { $$ = SQLCLI_ACCUMULATED_STATS; }
             | PERTABLEtoken          { $$ = SQLCLI_PERTABLE_STATS; }
             | PROGRESStoken          { $$ = SQLCLI_PROGRESS_STATS; }
             | DEFAULTtoken           { $$ = SQLCLI_SAME_STATS; }
             | empty                  { $$ = SQLCLI_DEFAULT_STATS; }
;

qid_identifier :             
               IDENTIFIER              
               { 
                // Convert IDENTIFIER to uppercase
                identifier_name_internal2 = new char[strlen($1)+1];
		str_cpy_convert(identifier_name_internal2, $1, strlen($1), TRUE); 
                identifier_name_internal2[strlen($1)] = '\0';
                $$ = identifier_name_internal2;
              } 
            | DQUOTED_STRING          
            { 
                // keep Double Quoted String as is                
                identifier_name_internal2 = new char[strlen($1)+1];
                strcpy(identifier_name_internal2, $1);
                $$ = identifier_name_internal2;
            } 
;

sql_cmd :
		dml_type
			{
                          $$ = new DML(SqlciParse_OriginalStr, $1, NULL);
                          REPOSITION_SqlciParse_InputPos;
			}

        |       DECLAREtoken IDENTIFIER 
			{
			  identifier_name_internal = new char[strlen($2)+1];
			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
			}
                CURSORWITH HOLD FORtoken
			{
			  pos_internal = SqlciParse_InputPos;
			}
		       dml_simple_table_type
			{
			  $$ = 
	   		    new Cursor(identifier_name_internal, 
				       Cursor::DECLARE, -1,
				       &SqlciParse_OriginalStr[pos_internal]);
			  REPOSITION_SqlciParse_InputPos;

			  ((Cursor *)$$)->setHoldable(TRUE);
			}

        |       DECLAREtoken IDENTIFIER 
			{
			  identifier_name_internal = new char[strlen($2)+1];
			  str_cpy_convert(identifier_name_internal, $2,
					  (long) strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
			}
                CURSORtoken FORtoken
			{
			  pos_internal = (int) SqlciParse_InputPos;
			}
	        cursor_info
			{
                          SqlciCursorInfo *cinfo = $7;

			  Cursor *c = new
                            Cursor(identifier_name_internal, 
                                   Cursor::DECLARE,
                                   (short) cinfo->queryTextSpecified_,
                                   cinfo->queryTextOrStmtName_);

                          c->setResultSetIndex(cinfo->resultSetIndex_);

                          $$ = c;

                          delete [] identifier_name_internal2;
                          identifier_name_internal2 = NULL;

                          delete cinfo;

			  REPOSITION_SqlciParse_InputPos;
			}

        |       OPENtoken IDENTIFIER
			{
			  identifier_name_internal = new char[strlen($2)+1];
			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
			  $$ = 
			    new Cursor(identifier_name_internal, 
				       Cursor::OPEN, 0, NULL);
			}

        |       FETCHtoken IDENTIFIER
			{
			  identifier_name_internal = new char[strlen($2)+1];
			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
			  $$ = 
			    new Cursor(identifier_name_internal, 
				       Cursor::FETCH, 0, NULL);
			}

        |       CLOSEtoken IDENTIFIER
			{
			  identifier_name_internal = new char[strlen($2)+1];
			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
			  $$ = 
			    new Cursor(identifier_name_internal, 
				       Cursor::CLOSE, 0, NULL);
			}

        |       DEALLOCtoken IDENTIFIER
			{
			  identifier_name_internal = new char[strlen($2)+1];
			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
			  $$ = 
			    new Cursor(identifier_name_internal, 
				       Cursor::DEALLOC, 0, NULL);
			}

		|	PREPAREtoken IDENTIFIER
			{
			  identifier_name_internal = new char[strlen($2)+1];
			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
			}
			FROM
			{
			  pos_internal = SqlciParse_InputPos;
			}
			dml_type
			{
			  $$ = new Prepare(identifier_name_internal,
				     &SqlciParse_OriginalStr[pos_internal], $6);
			  REPOSITION_SqlciParse_InputPos;
			}

		|	DESCRIBEToken IDENTIFIER
			{
			  identifier_name_internal = new char[strlen($2)+1];
			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
			  $$ = new DescribeStmt(identifier_name_internal, identifier_name_internal);
			}

		|	EXECUTEtoken IDENTIFIER
			{
			  // ## This kind of foolishness could be partly done
			  // ## away with, consolidated at least, if most of
			  // ## these "sql_cmd" productions were made
			  // ## "sqlci_cmd" ones (the pre-YYACCEPT logic for
			  // ## the latter would handle this generically).
			  if (sqlcitext[0] && sqlcitext[0] != ';')
			    {sqlcierror("");YYERROR;}

			  identifier_name_internal = new char[strlen($2)+1];

			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;

                          $$ = new Execute(identifier_name_internal, 
                                           identifier_name_internal, 0/*flag*/, SqlciEnvGlobal);
			}

		|	EXECUTEtoken IDENTIFIER
			{
			  identifier_name_internal = new char[strlen($2)+1];

			  str_cpy_convert(identifier_name_internal, $2,
					  strlen($2), TRUE);
			  identifier_name_internal[strlen($2)] = 0;
            }
            USING
            {
              pos_internal = SqlciParse_InputPos;
			  if (SqlciParse_OriginalStr[pos_internal-1] == ',')
			    pos_internal--;
                          $$ = new Execute(identifier_name_internal, 
                                           &SqlciParse_OriginalStr[pos_internal], 1/*flag*/, SqlciEnvGlobal);

            }

	|	DISPLAY_EXPLAIN explain_options IDENTIFIER
			{
			  // returns "EXPLAIN [options 'x'] id;", sets deprecated warning
			  // The +16 allows space to add "options 'm' " when defaulted
			  long newStrLen = strlen(SqlciParse_OriginalStr) + 16;
			  char * newStr = new char[newStrLen+1];
			  if ($2)
			    {
			      strcpy(newStr, "EXPLAIN options '");
			      strcat(newStr, $2);
			      strcat(newStr, "' ");
			    }
			  else
			    {
			      strcpy(newStr, "EXPLAIN options 'm' ");
			    }
			  identifier_name_internal = new char[strlen($3)+1];

			  str_cpy_convert(identifier_name_internal, $3,
					  strlen($3), TRUE);
			  identifier_name_internal[strlen($3)] = 0;
			  strcat(newStr, identifier_name_internal);
			  strcat(newStr, ";");
			  sqlci_DA << DgSqlCode(15039);

			  $$ = new DML(newStr, DML_DESCRIBE_TYPE, NULL);

			  delete identifier_name_internal;
			  delete newStr;
                        }

	|	DISPLAY_EXPLAIN explain_options PROCEDUREtoken 
			{
			  // Not a user-documented feature
			  // returns "EXPLAIN [options 'x'] procedure();", sets deprecated warning
			  // The +16 allows space to add "options 'm' " when defaulted
			  long newStrLen = strlen(SqlciParse_OriginalStr) + 16;
			  char * newStr = new char[newStrLen+1];
			  if ($2)
			    {
			      strcpy(newStr, "EXPLAIN ");
			    }
			  else
			    {
			      strcpy(newStr, "EXPLAIN options 'm' ");
			    }
			  strcat(newStr, 
				 &SqlciParse_OriginalStr[strlen("DISPLAY_EXPLAIN")]);
			  sqlci_DA << DgSqlCode(15039);
			  $$ = new DML(newStr,
				       DML_DESCRIBE_TYPE, NULL);
			  REPOSITION_SqlciParse_InputPos;
			  delete newStr;

                        }

	|	DISPLAY_EXPLAIN	explain_options dml_type
			{
			  // returns "EXPLAIN [options 'x'] SQL-cmd;", sets deprecated warning
			  // The +16 allows space to add "options 'm' " when defaulted
			  long newStrLen = strlen(SqlciParse_OriginalStr) + 16;
			  char * newStr = new char[newStrLen+1];
			  if ($2)
			    {
			      strcpy(newStr, "EXPLAIN ");
			    }
			  else
			    {
			      strcpy(newStr, "EXPLAIN options 'm' ");
			    }
			  strcat(newStr, 
				 &SqlciParse_OriginalStr[strlen("DISPLAY_EXPLAIN")]);
			  sqlci_DA << DgSqlCode(15039);
			  $$ = new DML(newStr,
				       DML_DESCRIBE_TYPE, NULL);
			  REPOSITION_SqlciParse_InputPos;
			  delete newStr;
			}

	| 	DISPLAY_STATISTICS
                  {
		    long newStrLen = 
		      strlen("GET STATISTICS , options 'of';");
		    char * newStr = new char[newStrLen+1];
		    strcpy(newStr, "GET STATISTICS , options 'of';");
		    $$ = new DML(newStr, DML_DESCRIBE_TYPE, NULL);
		    
		    delete newStr;
		  }

	| 	DISPLAY_STATISTICS IDENTIFIER
                  {
		    identifier_name_internal = new char[strlen($2)+1];
		    
		    str_cpy_convert(identifier_name_internal, $2,
				    strlen($2), TRUE);
		    identifier_name_internal[strlen($2)] = 0;

		    long newStrLen = 
		      strlen("GET STATISTICS FOR STATEMENT , options 'of';"); 
		    newStrLen += strlen(identifier_name_internal);
		    char * newStr = new char[newStrLen+1];
		    strcpy(newStr, "GET STATISTICS FOR STATEMENT ");
		    strcat(newStr, identifier_name_internal);
		    strcat(newStr, ", options 'of';");
		    $$ = new DML(newStr, DML_DESCRIBE_TYPE, NULL);
		    
		    delete newStr;
		  }
        |   DISPLAY_STATISTICS FORtoken CPU NUMBER 
                  {
                    pos_internal = atoi($4);
                  }
                  stats_active_clause
                  {
                    pos_internal3 = $6;
                  }
                  stats_merge_clause
                  {
                    char * newStr = new char[200];
                    sprintf(newStr, "GET STATISTICS FOR CPU %d ACTIVE %d", 
                            pos_internal, pos_internal3);
                    if ($8 == SQLCLIDEV_ACCUMULATED_STATS)
                      strcat(newStr, " ACCUMULATED");
                    strcat(newStr, " ;");
                    $$ = new DML(newStr, DML_DESCRIBE_TYPE, NULL);
                    delete newStr;
                  }
        |   DISPLAY_STATISTICS FORtoken CPU CPU_VALUE 
                  {
                    // This is of the form "FOR CPU \DOPEY.1"
                    identifier_name_internal2 = new char[strlen($4)+1];  
                    str_cpy_convert (identifier_name_internal2, $4, strlen ($4), TRUE);
                    identifier_name_internal2[strlen($4)] = 0;
                  }
                  stats_active_clause
                  {
                    pos_internal3 = $6;
                  }
                  stats_merge_clause
                  {
                    
                    long strLen = 100+strlen(identifier_name_internal2);
                    char *newStr = new char[strLen];
                    sprintf( newStr, "GET STATISTICS FOR CPU %s ACTIVE %d",
                        identifier_name_internal2, pos_internal3);
                    if ($8 == SQLCLIDEV_ACCUMULATED_STATS)
                        strcat(newStr, " ACCUMULATED");
                    strcat(newStr, " ;");
                    $$ = new DML(newStr, DML_DESCRIBE_TYPE, NULL);
                    delete identifier_name_internal2;
                    delete newStr;
                  }
      |       DISPLAY_STATISTICS FORtoken PID NUMBER 
                  {
                    pos_internal = atoi ($4);
                  }
                  COMMA NUMBER 
                  {
                    pos_internal2 = atoi ($7);
                  }
                  stats_active_clause 
                  {
                    pos_internal3 = $9;
                    
                  }
                  stats_merge_clause
                  {
                    char *newStr = new char[200];
                    sprintf(newStr, "GET STATISTICS FOR PID %d,%d ACTIVE %d", 
                          pos_internal, pos_internal2, pos_internal3);
                    if ($11 == SQLCLIDEV_ACCUMULATED_STATS)
                        strcat(newStr, " ACCUMULATED");
                    strcat(newStr, " ;");
                    $$ = new DML(newStr, DML_DESCRIBE_TYPE, NULL);
                    delete newStr;
                  }
      |       DISPLAY_STATISTICS FORtoken PID PID_VALUE  
                  {
                    // This is of the form "FOR PID \DOPEY.1,5"
                    identifier_name_internal2 = new char[strlen($4)+1];  
                    str_cpy_convert (identifier_name_internal2, $4, strlen($4), TRUE);
                    identifier_name_internal2[strlen($4)] = 0;
                  }
                  stats_active_clause
                  {
                    pos_internal3 = $6;
                  }
                  stats_merge_clause
                  {
                    long strLen = 100 + strlen(identifier_name_internal2);
                    char *newStr = new char[strLen];
                    sprintf(newStr, "GET STATISTICS FOR PID %s ACTIVE %d", 
                      identifier_name_internal2, pos_internal3);
                    if ($8 == SQLCLIDEV_ACCUMULATED_STATS)
                        strcat(newStr, " ACCUMULATED");
                    strcat(newStr, " ;");
                    $$ = new DML(newStr, DML_DESCRIBE_TYPE, NULL);
                    delete identifier_name_internal2;
                    delete newStr;
                  }
      |       DISPLAY_STATISTICS FORtoken QID qid_identifier 
                  {
                    get_stats_str = new char[strlen($4)+100]; 
                    sprintf(get_stats_str, "GET STATISTICS FOR QID %s", $4);
                  }
                  stats_merge_clause
                  {
                    char *newStr1 = new char[25];
                    if ($6 == SQLCLI_ACCUMULATED_STATS)
                        strcpy(newStr1, " ACCUMULATED");
                    else if ($6 == SQLCLI_PERTABLE_STATS)
                        strcpy(newStr1, " PERTABLE");
                    else if ($6 == SQLCLI_PROGRESS_STATS)
                        strcpy(newStr1, " PROGRESS");
                    else if ($6 == SQLCLI_SAME_STATS)
                        strcpy(newStr1, " DEFAULT");
                    else
                        strcpy(newStr1, "");
				    strcat(get_stats_str, newStr1);
					strcat(get_stats_str, " ;");
					$$ = new DML(get_stats_str, DML_DESCRIBE_TYPE, NULL);
	                
					// delete id name internal2 allocated in qid_identifier
					if (identifier_name_internal2)
					{
					  delete [] identifier_name_internal2;
					  identifier_name_internal2 = NULL;
					}
					delete [] get_stats_str;
					delete [] newStr1;
				  }
	| DISPLAY_QC
	  {
	    $$ = new QueryCacheSt(0);
      }
	| DISPLAY_QC_ENTRIES
	  {
	    $$ = new QueryCacheSt(1);
      }
	| DISPLAY_QC_ENTRIES_NOTIME
	  {
	    $$ = new QueryCacheSt(2);
      }
	|	DISPLAY
			{
			  pos_internal = 7;
			}
		 dml_type
			{
			  $$ = new DML(SqlciParse_OriginalStr, $3,
				       "__SQLCI_DML_DISPLAY__");
			  REPOSITION_SqlciParse_InputPos;
			}
       |        GETtoken 
                        {
			  $$ = new DML(SqlciParse_OriginalStr, DML_DISPLAY_NO_HEADING_TYPE, NULL);
			  REPOSITION_SqlciParse_InputPos;
			}

       |        GETSTATISTICStoken
                        {
			  $$ = new DML(SqlciParse_OriginalStr, DML_DESCRIBE_TYPE, NULL);
			  REPOSITION_SqlciParse_InputPos;
			}

       |        GETtoken UIDtoken
                        {
			  $$ = new DML(SqlciParse_OriginalStr, DML_DESCRIBE_TYPE, NULL);
			  REPOSITION_SqlciParse_InputPos;
			}

		|	SETtoken ENVVARtoken IDENTIFIER
		  {
		    int i, len, quoted;
                    CharInfo::CharSet cs;
                    int inhexadecimal;
		    if (trimValueText(i, len, quoted, cs, inhexadecimal)) {sqlcierror("");YYERROR;}

		    char * evNam = new char[strlen($3)+1];
		    for (size_t k=0; k < strlen($3); k++)
		      {
			evNam[k] = toupper($3[k]);
		      }
		    evNam[strlen($3)] = 0;

		    char * evVal = NULL;
		    if (len <= 0)
		      {
			len = 1;
			evVal = new char[len + 1];
			strcpy(evVal, "1");
		      }
		    else
		      {
			evVal = new char[len+1];
			strncpy(evVal, &SqlciParse_OriginalStr[i], len);
			evVal[len] = 0;
		      }

		    $$ = new DML(SqlciParse_OriginalStr, DML_DESCRIBE_TYPE, NULL);

                    int j = setenv(evNam, evVal, 1); 
		    if (j) 
		      {
			delete [] evNam;
			delete [] evVal;

			cerr << "*** ERROR " << j << " from putenv." << endl;
			sqlcierror("");
			YYERROR;
		      }

		    REPOSITION_SqlciParse_InputPos;

		    Envvar * envvar = 
		      SqlciEnvGlobal->get_envvarlist()->get(evNam);
		    if (envvar)
		      {
			envvar->setValue(evVal);
		      }
		    else
		      {
			envvar = new Envvar(evNam, evVal);
			SqlciEnvGlobal->get_envvarlist()->append(envvar);
		      }
		    delete [] evNam;
		    delete [] evVal;
		  }

	|	RESET ENVVARtoken IDENTIFIER
		  {
		    char * evNam = new char[strlen($3)+1];
		    for (size_t k=0; k < strlen($3); k++)
		      {
			evNam[k] = toupper($3[k]);
		      }
		    evNam[strlen($3)] = 0;

		    $$ = new DML(SqlciParse_OriginalStr, DML_DESCRIBE_TYPE, NULL);
		    
		    long putenvLen = 0;
		    putenvLen += strlen(evNam);
		    putenvLen += strlen("=");
		    char * putenvStr = new char[putenvLen+1];

		    strcpy(putenvStr, evNam);
		    strcat(putenvStr, "=");
		    int j = 0;
		    NABoolean found = FALSE;
		    while ((ENVIRON[j]) && (NOT found))
		      {
			if ((strlen(ENVIRON[j]) >= strlen(putenvStr)) &&
			    (memcmp(ENVIRON[j], putenvStr, strlen(putenvStr)) == 0))
			  {
			    found = TRUE;
			  }
			else
			  j++;
		      }

		    if (found)
		      {
			while (ENVIRON[j])
			  {
			    ENVIRON[j] = ENVIRON[j+1];
			    j++;
			  }
		      }
		    SqlciEnvGlobal->get_envvarlist()->remove(evNam);

		    delete [] evNam;
		  }

	|	SHOW ENVVARtoken
                {
		  long newStrLen = 
		    strlen("GET ENVVARS;");
		  char * newStr = new char[newStrLen+1];
		  strcpy(newStr, "GET ENVVARS;");
		  $$ = new DML(newStr, DML_SHOWSHAPE_TYPE, NULL);
		  
		  delete newStr;
		}
	|	SETtoken PARSERFLAGS NUMBER
		{
		  $$ = new DML(SqlciParse_OriginalStr, DML_DESCRIBE_TYPE, NULL);
		}

	|	RESET PARSERFLAGS NUMBER
		{
		  $$ = new DML(SqlciParse_OriginalStr, DML_DESCRIBE_TYPE, NULL);
		}

	|	RESET PARSERFLAGS
		{
		  $$ = new DML(SqlciParse_OriginalStr, DML_DESCRIBE_TYPE, NULL);
		}

        |       QUIESCE
		        { 
                          $$ = new Quiesce();
                        }

        |       ERRORtoken NUMBER COMMA GETtoken
                  { 
		    long newStrLen = 
		      strlen("GET TEXT FOR ERROR ") + 20;
		    char * newStr = new char[newStrLen+1];
		    str_sprintf(newStr, "GET TEXT FOR ERROR %s", $2);
		    $$ = new DML(newStr, DML_DESCRIBE_TYPE, NULL);
		  }

        |      STOREtoken EXPLAIN FORtoken IDENTIFIER INtoken REPOSITORYtoken
                {
                  identifier_name_internal = new char[strlen($4)+1]; 
                  str_cpy_convert(identifier_name_internal, $4, strlen($4), TRUE); 
                  identifier_name_internal[strlen($4)] = 0;
 
                  $$ = new StoreExplain(identifier_name_internal);
                }


optional_rs_index : empty
                    {
                      $$ = 0;
                    }
                  | RESULT SETtoken NUMBER
                    {
                      $$ = atoi($3);
                    }

cursor_info : dml_simple_table_type
              { 
                char *queryText = &SqlciParse_OriginalStr[pos_internal];
                char *queryTextCopy = new char[strlen(queryText) + 1];
                strcpy(queryTextCopy, queryText);

                SqlciCursorInfo *c = new SqlciCursorInfo();
                c->queryTextSpecified_ = -1;
                c->queryTextOrStmtName_ = queryTextCopy;
                $$ = c;
              }
            | IDENTIFIER
              {
                // Not sure exactly why, but we must make a copy of
                // the IDENTIFIER string now before more tokens are
                // parsed. If we wait until the entire rule is
                // matched, the IDENTIFIER string may not be valid
                // anymore. The copy will be pointed to by a global
                // variable and released later by the rule that
                // contains cursor_info as a subrule.
                identifier_name_internal2 = new char[strlen($1) + 1];
                strcpy(identifier_name_internal2, $1);
              }
              optional_rs_index
              {
                SqlciCursorInfo *c = new SqlciCursorInfo();
                c->queryTextSpecified_ = 0;
                c->queryTextOrStmtName_ = identifier_name_internal2;
                c->resultSetIndex_ = $3;
                $$ = c;
              }

dml_type :
	 	dml_simple_table_type	{}
	|	CONTROL 		{$$ = DML_CONTROL_TYPE;}
	|       OSIM                    {$$ = DML_OSIM_TYPE;}
	|	SETtoken CATALOG	{$$ = DML_CONTROL_TYPE;}
	|	SETtoken SCHEMA		{$$ = DML_CONTROL_TYPE;}
	|	SETtoken HIVEtoken	{$$ = DML_CONTROL_TYPE;}
        |       SETtoken MPLOC          {$$ = DML_CONTROL_TYPE;}
        |       SETtoken NAMETYPE       {$$ = DML_CONTROL_TYPE;}
        |       SETtoken SESSIONtoken   {$$ = DML_CONTROL_TYPE;}
	|	UPDATEtoken  		{$$ = DML_UPDATE_TYPE;}
	| 	INSERTtoken  		{$$ = DML_INSERT_TYPE;}
        |       UPSERTtoken             {$$ = DML_INSERT_TYPE;}
	|	ROWSETtoken  		{$$ = DML_INSERT_TYPE;}
	|	DELETEtoken  		{$$ = DML_DELETE_TYPE;}
	|	MERGEtoken  		{$$ = DML_UPDATE_TYPE;}
        |       CREATE  		{$$ = DML_DDL_TYPE;}
	|	ALTER   		{$$ = DML_DDL_TYPE;}
	|       CLEANUP 		{$$ = DML_DESCRIBE_TYPE;}
	|	DROP    		{$$ = DML_DDL_TYPE;}
	|	UPD_STATS       	{$$ = DML_DDL_TYPE;}
	|	UPD_HIST_STATS  	{$$ = DML_DDL_TYPE;}
	|       METADATAtoken 		{$$ = DML_DDL_TYPE;}
        |       INITIALIZE              {$$ = DML_DESCRIBE_TYPE;}
        |       REINITIALIZE            {$$ = DML_DDL_TYPE;}
        |       GIVE                    {$$ = DML_DDL_TYPE;}
        |       GRANTtoken              {$$ = DML_DDL_TYPE;}
        |       REVOKEtoken             {$$ = DML_DDL_TYPE;}
        |       REGISTER                {$$ = DML_DDL_TYPE;}
        |       UNREGISTER              {$$ = DML_DDL_TYPE;}
	|       SHOWCONTROL 		{$$ = DML_DESCRIBE_TYPE;}
	|       SHOWDDL 		{$$ = DML_DESCRIBE_TYPE;}
	|	SHOWSTATS               {$$ = DML_DESCRIBE_TYPE;}
        |	SHOWTRANSACTION         {$$ = DML_DESCRIBE_TYPE;}
	|	INVOKE  		{$$ = DML_DESCRIBE_TYPE;}
	|	SHOWPLAN  		{$$ = DML_DESCRIBE_TYPE;}
        |       SHOWSHAPE               {$$ = DML_DESCRIBE_TYPE;}
        |       SHOWSET                 {$$ = DML_DESCRIBE_TYPE;}
	|	LOCK    		{$$ = DML_CONTROL_TYPE;}
	|	UNLOCK  		{$$ = DML_CONTROL_TYPE;}
	|	TOK_BEGIN   		{$$ = DML_CONTROL_TYPE;}
	|	COMMIT  		{$$ = DML_CONTROL_TYPE;}
	|	ROLLBACK 		{$$ = DML_CONTROL_TYPE;}
        |	SET_TABLEtoken 	        {$$ = DML_CONTROL_TYPE;}  
	|	SET_TRANSACTIONtoken 	{$$ = DML_CONTROL_TYPE;}
	|	MODIFY			{$$ = DML_DDL_TYPE;}
	|	IF			{$$ = DML_CONTROL_TYPE;}
	|	WHILE			{$$ = DML_CONTROL_TYPE;}
	|	SIGNAL			{$$ = DML_CONTROL_TYPE;}
	|	INTERNAL		{$$ = DML_CONTROL_TYPE;}
	|	MVLOG			{$$ = DML_CONTROL_TYPE;} 
	|	PURGEDATA		{$$ = DML_DDL_TYPE;}
	|	TRUNCATE		{$$ = DML_DDL_TYPE;}
	|	WITH                    {$$ = DML_DDL_TYPE;}
	|	POPULATE		{$$ = DML_DDL_TYPE;}
        |       VALIDATEtoken           {$$ = DML_DDL_TYPE;}
	|	REFRESH		        {$$ = DML_DDL_TYPE;}
	|	TRANSFORM		{$$ = DML_DDL_TYPE;}
	|	CALLToken		{$$ = DML_DDL_TYPE;}
	|	LOADtoken		{$$ = DML_DDL_TYPE;}
	|	MSCKtoken		{$$ = DML_DDL_TYPE;}
	|	EXTRACTtoken		{$$ = DML_DESCRIBE_TYPE;}
	|	REPLICATEtoken		{$$ = DML_DESCRIBE_TYPE;}
	|	GENERATEtoken		{$$ = DML_DESCRIBE_TYPE;}
        |	EXPLAIN		        {$$ = DML_DESCRIBE_TYPE;}
        |       GETtoken                {$$ = DML_DESCRIBE_TYPE;}
 	|       PROCESStoken            {$$ = DML_DDL_TYPE;}
 	|     COMMENTtoken       {$$ = DML_DDL_TYPE;}
;

dml_simple_table_type :
	 	SELECTtoken  			{$$ = DML_SELECT_TYPE;}
	|	TABLE   			{$$ = DML_SELECT_TYPE;}
	|	VALUES  			{$$ = DML_SELECT_TYPE;}
	|	LPAREN dml_simple_table_type  	{$$ = DML_SELECT_TYPE;}
        |       LOCKINGtoken                    {$$ = DML_SELECT_TYPE;}
        |       UNLOAD                          {$$ = DML_UNLOAD_TYPE;}
;



explain_options : empty
            { 
              $$ = 0; 
            } 
        | OPTIONStoken QUOTED_STRING
            { 
              if (strcmp($2, "f") != 0)         // accept only old value
                {
                  sqlci_DA << DgSqlCode(-15517);// say invalid option
                  YYERROR;                      // give up processing
                }
              else
                {
                  $$ = $2;
                } 
            } 
;

shell_cmd :
		CD
			{
			  $$ = new Chdir(SqlciParse_OriginalStr);
			  REPOSITION_SqlciParse_InputPos;
			}
	|	LS
			{
			  $$ = new Ls(SqlciParse_OriginalStr);
			  REPOSITION_SqlciParse_InputPos;
			}
	|	SHELL
			{
			  $$ = new Shell(SqlciParse_OriginalStr);
			  REPOSITION_SqlciParse_InputPos;
			}
;


%%
