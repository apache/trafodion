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
**********************************************************************/
/* -*-C++-*-                       
 *****************************************************************************
 *
 * File:         InputStmt.C
 * RCS:          $Id: InputStmt.cpp,v 1.2 2007/10/19 16:07:26  Exp $
 * Description:
 *
 * Created:      4/15/95
 * Modified:     $ $Date: 2007/10/19 16:07:26 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


#include "Platform.h"
#include "ComASSERT.h"
#include "InputStmt.h"
#include "ParserMsg.h"
#include "SqlciEnv.h"
#include "SqlciError.h"
#include "SqlciParser.h"
#include "str.h"

#include "ComDiags.h"
extern ComDiagsArea sqlci_DA;


InputStmt::InputStmt(SqlciEnv * the_sqlci_env)
{
  sqlci_env = the_sqlci_env;
  first_fragment = NULL;
  packed_string = command = text = NULL;
  text_pos = command_pos = text_maxlen = 0;
  setInHistoryList(FALSE);

  isIgnoreStmt_ = 0;
  ignoreJustThis_ = FALSE;
  veryFirstLine_ = FALSE;

  blockStmt_ = 0;
  shellCmd_ = 0;
  allowCSinsqlci_ = !!getenv("ALLOW_CS_IN_SQLCI");
}	// ctor

// Copy some context from source, but mostly copy a previously packed string
// into this, and retrofit a single fragment from it.
// This may make long, hard-to-FC stmts, but that's just too bad.
InputStmt::InputStmt(const InputStmt * source, const char * packed)
{
  sqlci_env = source->sqlci_env;		// save environment
  setInHistoryList(FALSE);

  isIgnoreStmt_ = 0;
  ignoreJustThis_ = FALSE;
  veryFirstLine_ = FALSE;

  blockStmt_ = 0;
  shellCmd_ = 0;
  allowCSinsqlci_ = !!getenv("ALLOW_CS_IN_SQLCI");

  size_t len = strlen(packed) + 1;
  first_fragment = new StringFragment();
  first_fragment->fragment = new char[len];
  first_fragment->next = NULL;
  strcpy(first_fragment->fragment, packed);

  packed_string = command = text = NULL;
  text_pos = command_pos = text_maxlen = 0;

  // The packed string likely comes from the FC command, which has chopped up
  // multiple stmts like "abc; def;" into "abc;" and " def;".
  // We'll explicitly repack now to trim initial whitespace.
  pack();
}	// ctor

InputStmt::~InputStmt()
{
  delete [] packed_string;
  if (command)
    delete [] command;
  // delete text;	// text points into memory owned by someone else

  StringFragment * curr_fragment = first_fragment;
  StringFragment * prev_fragment;

  while (curr_fragment)
    {
      prev_fragment = curr_fragment;
      delete [] curr_fragment->fragment;
      curr_fragment = curr_fragment->next;
      delete prev_fragment;
    }
}	// dtor

void InputStmt::operator=(const InputStmt * source)
{
  sqlci_env = source->sqlci_env;

  first_fragment = new StringFragment();
  first_fragment->fragment = new char[strlen(source->first_fragment->fragment) + 1];
  strcpy(first_fragment->fragment, source->first_fragment->fragment);

  first_fragment->next = 0;

  StringFragment * source_fragment = source->first_fragment->next;
  StringFragment * target_fragment;
  StringFragment * cur_fragment = first_fragment;

  while (source_fragment)
    {
      // Create a new fragment from the source
      target_fragment=new StringFragment();
      target_fragment->fragment = new char[strlen(source_fragment->fragment)+ 1];
      strcpy(target_fragment->fragment, source_fragment->fragment);
      target_fragment->next = 0;

      // Link the new fragment in the chain
      cur_fragment->next = target_fragment;

      // ... and set up to add the next
      source_fragment = source_fragment->next;
      cur_fragment = cur_fragment->next;
    }

}	// operator=()


////////////////////////////////////////////////////////
// Macros for input
////////////////////////////////////////////////////////

	    // Make sure that eof on stdin not treated as session <EOF>.
	    // The following is required, probably because of a bug in
	    // Centerline's iostream implementation.  It should automatically
	    // clear error on stdin if sync_with_stdio has been called for
	    // this process.
	    // The cout gets us to a new line past the ^D (or ^Y);
	    // it does not go into the sqlci log file.
#define CLEAR_STDIN_EOF				\
	    {					\
	    cin.clear();		      	\
	    clearerr(stdin);		      	\
	    cout << endl;			\
	    }

volatile char Sqlci_PutbackChar = '\0';		// global for Break handling

////////////////////////////////////////////////////////
// Returns:
//	    1, if string is fine, but no semicolon seen yet
//          0, if string is fine, and semicolon-terminated
//         -1, invalid string
//         -2, eof (^Z or F6 on NT, ^D on Unix, ^Y on NSK)
//	   -20, break (^C)
//         -3, error
//         -4, if an eof immediately follows a valid string
////////////////////////////////////////////////////////
Int32 InputStmt::getLine(char * input_str, FILE * nonstdin, Int32 first_line)
{
  char a = ' ';
  size_t i = 0;
  Int32 retcode = 1;
  Int32 white = -1;
  Int32 skipGetc = 0;
  Int32 quick_eof = 0;  // flag for a tight eof

  size_t normalstart = 0;

  enum InputState
    {
      INITIAL, INITIAL_WHITESPACE, DOUBLE_QUOTE, SINGLE_QUOTE,
      NORMAL, PROBABLY_COMMENT, PROBABLY_INITIAL_COMMENT, CONSUME,
      NORMAL_OR_SHELL, EOL, EOSTMT	// , ESCAPE_CHAR
      };
  InputState state = INITIAL;

  if (sqlci_env->prevErrFlushInput() && !nonstdin)
  	{
   	while (a != '\n')
       {
       	cin.get(a);

       }
      state = EOL;
      sqlci_env->resetPrevErrFlushInput();
      
   }

  while (state != EOL)
    {
      if (!nonstdin)
	{
	  if (Sqlci_PutbackChar)
	    {
	      a = Sqlci_PutbackChar;
	      Sqlci_PutbackChar = '\0';
	    }
	  else
	    {
	      cin.get(a);
	      if (cin.eof())
		{
		    Sleep(50);			// give control to Break thre
		  Sqlci_PutbackChar = '\0';
		  return -2;
		}
	      Sqlci_PutbackChar = '\0';
	      if (!cin)
		return -3;
	    }
	}
      else
	{
     if (sqlci_env->prevErrFlushInput()&& !sqlci_env->eolSeenOnInput())
     {
       while ((a != '\n'&& a != '\t' && a != '-') && !feof(nonstdin))
       {
       	a = getc(nonstdin);
       }
      sqlci_env->resetPrevErrFlushInput();
      if (a == '\t' || a == '-')
      	skipGetc = -1;
     }

     if (!skipGetc)
	  	a = getc(nonstdin);

     skipGetc = 0;
	  // if after a statement, there is no '\n'
	  // between the statement and the eof.
	  if (feof(nonstdin) && state == EOSTMT)
	   {
	     quick_eof = 1;  // set a flag
	     a = '\n';       // replace the eof with a '\n'
	     state = EOL;    // process the statement
	   }
	  else
	  if (feof(nonstdin))
            return -2;
	  else if (ferror(nonstdin))
	    return -3;
	}
      // Error: we're about to overrun the buffer!
      if (i > MAX_FRAGMENT_LEN)
      {
        retcode = SqlciEnv::MAX_FRAGMENT_LEN_OVERFLOW;
        state = CONSUME;
      }

      switch (state)
	{
	case INITIAL_WHITESPACE:
	  // NOTE this must precede the INITIAL case!  Here's why:
	  //
          // If the cur character is whitespace do nothing. Simply break out of
          // the case body, continuing as state INITIAL_WHITESPACE.
	  // However, if it is not a whitespace character, then this state is
	  // actually the INITIAL state. Continue processing as if we were
	  // in the INITIAL state. NOTE the absence of break in the ELSE part
	  // of the IF statement.

	  shellCmd_ = 0;
	  if (a == ' ' || a == '\t')
	    break;
	  else
	    state = INITIAL;		// fall thru to INITIAL, next

	case INITIAL:
	  {
	    switch (a)
	      {
              case ' ':
              case '\t':
		state = INITIAL_WHITESPACE;
		break;

	      case '?':
	      case '#':
		if (first_line)		// ?SECTION, #IGNORE
		  {
		    retcode = 0;
		    state = CONSUME;
		  }
		break;

	      case '-':
		state = PROBABLY_INITIAL_COMMENT;
		break;

	      case '\'':
		state = SINGLE_QUOTE;
		break;

	      case '"':
		state = DOUBLE_QUOTE;
		break;

	      case '\n':
		state = EOL;
		break;

	      case ';':
		if (!inBlockStmt()) {
		  retcode = 0;
		  state = EOSTMT;
		} 
		else {
		  state = NORMAL; normalstart = i;
		}
		break;

              case 's':
	      case 'S':
		if (first_line) {
		  state = NORMAL_OR_SHELL;
		  normalstart = i;
		  break;
		}

		// otherwise fall through to default case

	      default:
		state = NORMAL; normalstart = i;
		break;

	      } // switch (a)
	  } // case INITIAL
	  break;

	case SINGLE_QUOTE:
	  {
	    switch (a)
	      {
	      case '\n':
		retcode = -1;
		state = EOL;
		break;

	      case '\'':
		state = NORMAL; normalstart = i;
		break;

	      }
	  }
	  break;

	case DOUBLE_QUOTE:
	  {
	    switch (a)
	      {
	      case '\n':
		retcode = -1;
		state = EOL;
		break;

	      case '"':
		state = NORMAL; normalstart = i;
		break;
	      }
	  }
	  break;

	case CONSUME:

          if (retcode == SqlciEnv::MAX_FRAGMENT_LEN_OVERFLOW)
          {
            while (a != ';')
            {
              cin.get(a);
            }
            a = ' ';
            cin.get(a);

            if (a != '\n')
              sqlci_env->setEol(0);
            state = EOL;
          }

	  if (a == '\n')
	    {
	      state = EOL;
	    }
	  else if (shellCmd_)
	    {
	      // check whether the shell command is terminated
	      // by a semicolon and adjust retcode
	      if (a == ';')
		retcode = 0;
	      else
		retcode = 1;
	    }
	  break;

	case EOSTMT:
	  {
	    if (a != '\n')
	      {
		sqlci_env->setEol(0);

		if (!nonstdin)
		  {
/////////////// This doesn't work on all platforms (cin.putback fails)
//		    cin.putback(a);
//		    if (!cin)
//		      return -3;
/////////////// So we use this little kludge instead:
		    Sqlci_PutbackChar = a;	// kludge
		  }
		else
		  {
		    if (ungetc(a, nonstdin) == EOF)
		      return -3;
		  }

	      }
	    state = EOL;
	  }
	  break;

	case PROBABLY_COMMENT:
	case PROBABLY_INITIAL_COMMENT:
	  if (a == '-')
	    {
	      if (state == PROBABLY_INITIAL_COMMENT && first_line)
		retcode = 0;
	      state = CONSUME;
	      break;
	    }
	  else
        {
          normalstart = i;
	    state = NORMAL;		// fall thru to NORMAL, next
        }

	case NORMAL:
	case NORMAL_OR_SHELL:
	  {
	    Int32 searchForShCmd = (state == NORMAL_OR_SHELL);
	    switch (a)
	      {
	      case '-':
		state = PROBABLY_COMMENT;
		findBlockStmt(input_str, normalstart, i-1, searchForShCmd);
		break;

	      case '\'':
		state = SINGLE_QUOTE;
		findBlockStmt(input_str, normalstart, i-1, searchForShCmd);
		break;

	      case '"':
		state = DOUBLE_QUOTE;
		findBlockStmt(input_str, normalstart, i-1, searchForShCmd);
		break;

	      case '\n':
		state = EOL;
		findBlockStmt(input_str, normalstart, i-1, searchForShCmd);
		break;

	      case ';':
		findBlockStmt(input_str, normalstart, i-1, searchForShCmd);
                if (!inBlockStmt())
		{
		  state = EOSTMT;
           	  retcode = 0;
                }
                else
                  normalstart = i;
		break;

	      }

	    if (shellCmd_)
	      state = CONSUME; // just consume to end of line for sh command
	  }
	  break;

	case EOL:
    	      // if there is an eof immediately after a statement
	      // change the return code to reflect it
	      if (quick_eof) retcode = -4;
	      break;

	} // switch

      // Change '\n' to terminating null.
      // Check if non-whitespace non-semicolon character.
      // If interactive, then "FC" command is operative, so
      // change tabs to spaces so FC aligns correctly;
      // if not interactive, leave tabs as is for pretty formatting
      // of echoed/logged lines.
      if (state == EOL)
      {
	  if (a == '\n')
	    sqlci_env->setEol(-1);
	  a = '\0';
      }
      else if (a != ' ' && a != '\t' && a != ';')  // as in InputStmt::isEmpty
	white = 0;
      else if (a == '\t' && sqlci_env->isInteractiveNow())
	if (state == INITIAL ||
	    state == INITIAL_WHITESPACE ||
	    state == NORMAL ||
	    state == DOUBLE_QUOTE)		// see NOTE in pack() below!
	  a = ' ';				// so "FC" command works

        input_str[i++] = a;
     // reset 'a' to prevent a loop caused that could be caused when a previous
     // statment in a multi-stmt input line cause an error. In the previous error
     // process, the tabs are maintained.
     a = ' ';
    } // while state != EOL

  // A first line of any number of whitespaces followed by ";" or '\n',
  // or a continuation line of whitespaces with no ";" (only '\n'),
  // is trimmed to the equivalent empty string.
  if (retcode >= 0 && white)
    if (retcode > 0 || first_line)
      input_str[0] = '\0';

  return retcode;

}	// getLine()

Int32 InputStmt::consumeLine(FILE * nonstdin)
{
  //char input_str[MAX_FRAGMENT_LEN+1];
  char * input_str = new char[MAX_FRAGMENT_LEN+1];
  Int32 rc = getLine(input_str, nonstdin, -1/*first line*/);
  delete [] input_str;
  return rc;
}

////////////////////////////////////////////////////////
// Returns:
//          0, if string is fine, and semicolon-terminated
//         -1, invalid string
//         -2, eof (^Z or F6 on NT, ^D on Unix, ^Y on NSK)
//	   -20, non-first-line interactive eof (used by FixCommand::process)
//		OR break (^C)
//         -99, error
////////////////////////////////////////////////////////
Int32 InputStmt::readStmt(FILE * nonstdin, Int32 suppress_blank_line_output)
{
  //  char input_str[MAX_FRAGMENT_LEN+1];
  char * input_str = new char[MAX_FRAGMENT_LEN+1];
  Int32 prompt = sqlci_env->isInteractiveNow();
  Int32 skip_first_fragment = 0;
  Int32 error = 0;
  StringFragment * prev_fragment = NULL;
  Int32 done = 0;
  //char buffer[256];
  //SYSTEMTIME sysTime;

  // If called by FixCommand, with a partially completed statement,
  // blip past the get-first-line do-loop.
  if (first_fragment)
    {
      skip_first_fragment = 1;
      delete packed_string;
      packed_string = NULL;
    }
  else
    {
      // Get the first line, looping until we get a nonblank line
      do
	{
	  Int32 eolSeenOrig = sqlci_env->eolSeenOnInput();

	  if ((prompt && eolSeenOrig) || (sqlci_env->prevErrFlushInput() && !nonstdin))
	  {

	    cout << ">>";
	    //cout << ">>" << endl;
	  }

	  error = getLine(input_str, nonstdin, -1/*first line*/);
          sqlci_env->resetPrevErrFlushInput();
    // Let NSK ESC be equivalent of NT ^Z or F6, i.e. EOF
    // (at least over a Telnet connection,
    // although unfortunately via OutsideView emulator).
    const char ESC = 27;	// ASCII ESCAPE character

	  switch (error)
	    {
	    case  1 :  // no ";" seen yet, keep looking
	      {
		
		// if this is an 'fc' or 'e' or 'exit' command, 
		// don't require a terminating semicolon (';').
		if ((! sqlci_env->isInteractiveNow()) ||
		    (input_str[0] == '\0'))
		  break;
		
		// skip leading blanks
	 UInt32 len = strlen(input_str);
	 UInt32 i = 0;
		while ((i < len) && (input_str[i] == ' '))
		  {
		    i++;
		  }
		
		// 'i' cannot be equal to 'len', as that will make it a blank
		// line and we will not be in this 'case'.
		// if input string doesn't start with an 'e'(xit) or an 'f'(c),
		// break. It cannot be an exit or fc command.
		if ((input_str[i] != 'f') &&
		    (input_str[i] != 'F') &&
		    (input_str[i] != 'e') &&
		    (input_str[i] != 'E'))
		  break;
		
		// find the next blank or end of line, whichever comes first.
	 UInt32 k = i + 1;
		while ((k < len) && (input_str[k] != ' '))
		  k++;
		
	 UInt32 cmdLen = k - i;
		char cmd[10];

		// 4 is len of 'exit', non-semicolon terminating commands cant
		// be more than 4 chars in length.
		if (cmdLen > 4) 
		  break;

		// copy upshifted 'cmdLen' chars into 'cmd' buffer.
	 UInt32 j = 0;
                for (j = 0; j < cmdLen; j++, i++)
		  cmd[j] = (char)toupper(input_str[i]);

		cmd[j] = 0;

		if ((strcmp(cmd, "FC") == 0) ||
		    (strcmp(cmd, "E") == 0) ||
		    (strcmp(cmd, "EXIT") == 0))
		  {
		    // found a non-semicolon terminating command. 
		    // If an 'E' is seen, change it to 'EXIT'. Sqlci
		    // parser only recognizes an 'EXIT'.
		    if (strcmp(cmd, "E") == 0)
		      strcpy(input_str, "EXIT");

		    // Append a semicolon to this input.
		    strcat(input_str, ";");
		    error = 0;
		  }
	      }
	    break;
	  
	    case  0 :  // normal input string
	    case -4 :  // statement followed immediately with eof
	      break;

	    case -1 :  // bad string -- unmatched quote
	      // call syntaxErrorOnMissingQuote() at end, after all frags saved
	      break;

	    case -2 :  // eof (^Z) on first line of stmt (i.e. before the line)
	      if (prompt) cout << endl;
	      sqlci_env->setEol(-1);	// Reset to initial eol state
	      goto return_error; //	      return error;

	    case -20 : // break (^C) on first line of stmt
	      if (prompt) CLEAR_STDIN_EOF;
	      sqlci_env->setEol(-1);	// Reset to initial eol state
	      //	      return error;
	      goto return_error; //	      return error;

	    case -3 : // error while reading input / obey file.
            {
      	      ErrorParam *p1 = new ErrorParam (errno);
	      SqlciError (SQLCI_ERROR_READING_FILE,
			  p1,
			  (ErrorParam *) 0);
	      delete p1;
	      //return -99;
	      error = -99;
	      goto return_error; //	      return error;
	    }

            case SqlciEnv::MAX_FRAGMENT_LEN_OVERFLOW :
              SqlciError (SQLCI_FRAGMENT_LEN_REACHED,
			  (ErrorParam *) 0);
	      //              return error;
	      goto return_error; //	      return error;

	    default :  // this case should never be reached
	    {
	      SqlciError (SQLCI_INTERNAL_ERROR, (ErrorParam *) 0);
	      // return error;
	      goto return_error; //	      return error;
	    }
	  }

	  // Log an all-blank line here simply as a prompt;
	  // also, if the input is not from terminal or
	  // input is from an obey file, display the blank line on stdout
	  // (logging and echoing of non-blank lines is done separately, in
	  // SqlciEnv::executeCommands, Obey::process,
	  // Repeat::process, FixCommand::process).
	  // Except don't log trailing blanks in a multi-stmt line, e.g.
	  // log "abc;def; " as "abc;" and "def;" but not the final "".
	  if (input_str[0] == '\0' && !suppress_blank_line_output)
	    if (eolSeenOrig || eolSeenOrig == sqlci_env->eolSeenOnInput())
	      {
		if (!prompt)
		  cout << ">>" << endl;

		if (sqlci_env->get_logfile()->IsOpen())
		  sqlci_env->get_logfile()->Write (">>", 2);
	      }
	}
      while (input_str[0] == '\0');	// ignore any number of whitespaces
					// followed by ";" or '\n'
    }

  do
    {
      if (skip_first_fragment)
	{
	  // We already have (from FC) the first fragment(s).
	  // No need to append an empty one and cause a blank line in history.
	  skip_first_fragment = 0;
	  prev_fragment = first_fragment;
	  while (prev_fragment->next)
	    prev_fragment = prev_fragment->next;
	  error = 1;			// getLine code for no semicolon seen
	}
      else
	{
	  StringFragment * new_fragment = new StringFragment();

	  new_fragment->fragment = new char[strlen(input_str) + 1];
	  strcpy(new_fragment->fragment, input_str);
	  new_fragment->next = 0;

	  if (!first_fragment)
	    first_fragment = new_fragment;
	  else
	    prev_fragment->next = new_fragment;

	  prev_fragment = new_fragment;
	}

      if (error == 1)	// no ";" seen yet, keep looking
	{
          if (prompt)
	     cout << "+>";

	  error = getLine(input_str, nonstdin, 0/*not first line*/);
	  // cerr << "##getLine:	" << error << " " << cin.eof() << " " << prompt << endl;

	  switch (error)
	    {

	    case  0 :  // normal input string
	    case  1 :  // no ";" seen yet, keep looking
		case -4 :  // end of file
	      break;

	    case -1 :  // bad string -- unmatched quote
	      // call syntaxErrorOnMissingQuote() at end, after all frags saved
	      break;

	    case -2 : // end of file
	      // Reset to initial eol state
	      sqlci_env->setEol(-1);

	      // On a non-first line, EOF only means get back to the main
	      // prompt without processing this command.  It does not mean exit
	      // from sqlci.  So now make sure that this condition not treated
	      // as session EOF, by clearing stdin and by returning a different
	      // return code, signifying a "special" EOF (one to be logged and
	      // historied, but not executed).
	      if (prompt)
		{
		  CLEAR_STDIN_EOF;
		  error *= 10;		// -20
		}
	      //              return error;
	      goto return_error; //	      return error;

	    case -3 : // error while reading input / obey file.
	    {
      	      ErrorParam *p1 = new ErrorParam (errno);
	      SqlciError (SQLCI_ERROR_READING_FILE,
			  p1,
			  (ErrorParam *) 0);
	      delete p1;
	      error = -99; //return -99;
	      goto return_error; //	      return error;
	    }

            case SqlciEnv::MAX_FRAGMENT_LEN_OVERFLOW :
              SqlciError (SQLCI_FRAGMENT_LEN_REACHED,
			  (ErrorParam *) 0);
	      //              return error;
	      goto return_error; //	      return error;

	    default :  // this case should never be reached
	      SqlciError (SQLCI_INTERNAL_ERROR, (ErrorParam *) 0);
	      //	      return error;
	      goto return_error; //	      return error;
	  }
	}
      else
	done = -1;
    }
  while (!done);

  if (error == -1)				// unmatched quote
    syntaxErrorOnMissingQuote();

return_error:
  if (input_str)
    delete [] input_str;
  return error;

}	// readStmt()

////////////////////////////////////////////////////////
// Returns 0 if no error, nonzero if mismatched quotes.
// Always sets packed_string to a non-NULL string
// (it may be the empty string, however).
////////////////////////////////////////////////////////
Int32 InputStmt::pack()
{
  StringFragment * curr_fragment = first_fragment;
  size_t packed_string_len = 0;
  size_t i = 0;
  char quote_seen = 0;
  size_t j;

  while (curr_fragment)
    {
      packed_string_len += strlen(curr_fragment->fragment);
      packed_string_len += 1;	// for a blank space between continuing lines
      curr_fragment = curr_fragment->next;
    }
  delete packed_string;
  packed_string = new char[packed_string_len + 1];

  if (packed_string_len)
    {
	// If an obey file is encoded in UTF8 encoding, the first 3 characters must be ignored.
	if(veryFirstLine_)
	{
	  veryFirstLine_ = FALSE;
	  char c0 = first_fragment->fragment[0];
      char c1 = first_fragment->fragment[1];
      char c2 = first_fragment->fragment[2];
	  if(c0 == (char)0xef )
	   if(c1 == (char)0xbb )
	    if (c2 == (char)0xbf)
		{
		 first_fragment->fragment[0] = ' ';
    	 first_fragment->fragment[1] = ' ';
         first_fragment->fragment[2] = ' ';
		}
	}
      /* now pack it*/

      curr_fragment = first_fragment;
      Int32 ignore = 0;
      Int32 first = -1;

      // Save info in private data members isIgnoreStmt_ + ignoreJustThis_
      isIgnoreStmt_ = isIgnoreStmt(curr_fragment->fragment, &ignoreJustThis_);

      if (isIgnoreStmt_ || ignoreJustThis_)
	{
	  // first statement is an ?ignore
	  size_t fraglen = strlen(curr_fragment->fragment);
	  for (j = 0; j < fraglen; j++)
	    {
	      packed_string[i++] = curr_fragment->fragment[j];
	    }
	}
      else
	{
	  while (curr_fragment)
	    {
	      Int32 skip_it = 0;

	      // For subsequent frags, use local vars for isIgnoreStmt stuff!
	      NABoolean ignoreJustThis;
	      Int32 isIgnore =
		isIgnoreStmt(curr_fragment->fragment, &ignoreJustThis);
	      if (isIgnore)
		ignore = NOT ignore;

	      if (!ignore && !isIgnore && !ignoreJustThis)
		{
		  j = 0;
		  size_t fraglen = strlen(curr_fragment->fragment);
		  if (first)
		    {
		      // Get past initial whitespace so that strcmp in
		      // history buffer lookup will find a match
		      // ("!cmd" command calling SqlciStmts::get).
		      // Also get past initial semicolons which user can
		      // input via FC cmd.
		      for ( ; j < fraglen; j++)
			{
			  char c = curr_fragment->fragment[j];
			  if (c != ' ' && c != '\t' && c != ';')
			    break;
			  else if (sqlci_env->isInteractiveNow())
			    curr_fragment->fragment[j] = ' ';
			}
		      if (j < fraglen)
			first = 0;
		    }
		  for ( ; j < fraglen; j++)
		    {
		      switch (curr_fragment->fragment[j])
			{
			case '\'':
			case '"':
			  if (!skip_it)
			    {
			      // Remember which type of quote (' or ") was
			      // seen first, and only reset when its matching
			      // quote is seen.
			      if (!quote_seen)
				quote_seen = curr_fragment->fragment[j];
			      else if (quote_seen == curr_fragment->fragment[j])
				quote_seen = 0;
			    }
			  break;

			case '&':
			  if (!skip_it && !quote_seen)
			    {
			      /* do something. later. */
			    }
			  break;

			case '-':
			  if (!quote_seen)
			    {
			      // comment, skip till eol
			      if (curr_fragment->fragment[j+1] == '-')
				skip_it = -1;
			    }

			  break;

			default:
			  break;
			}

		      if (!skip_it)
			{
			  // Always convert unquoted tabs to spaces
			  // in the packed string, so parser syntax error
			  // messages will be aligned properly
			  // (cf. getLine, we converted only if interactive,
			  // for the FC command).
			  //
			  char a = curr_fragment->fragment[j];
			  /*****
			  if (a == '\t' && !quote_seen)
			    a = ' ';
			  packed_string[i++] = a;
			  ******
				The above test is commented out because
				some Common code disallows tabs in
				delimited identifiers (dquotes (")),
				which is correct per Ansi
				(although parts of SqlParser.y try to allow it,
				but unfortunately tries inconsistently).
				So let's just convert them all (within dquotes)
				to spaces, so FC and syntax align is better yet.
				NOTE: we do this conversion for DOUBLE_QUOTE
				in getLine above too.
			  *****/
			  if (a == '\t' && quote_seen != '\'')
			    {
			      a = ' ';
			      if (sqlci_env->isInteractiveNow())
				curr_fragment->fragment[j] = ' ';
			    }
			  packed_string[i++] = a;
			}

		    }

		  if (quote_seen)    // unmatched quote on this line (this frag)
		    {
			  // Commented out: do not break!
			  //   instead pack all the frags
			  //   whether good or not, so FC can repair them.
			  //	 break;
		     // If unterminated quoting, convert any tabs on right of
		     // the quote to spaces (for FC command alignment -- which
		     // works on frags, not the packed string, so don't bother
		     // converting the latter).
		     char * frag = curr_fragment->fragment;
		     for (j = strlen(frag); j>0 && frag[j] != quote_seen; j--)
		       if (frag[j] == '\t')
			 frag[j] = ' ';
		    }
		}

	      curr_fragment = curr_fragment->next;
	      if (curr_fragment && i)
		packed_string[i++] = ' '; // insert a blank separator.

	    }
	}
    }

  packed_string[i] = '\0';
  if (isEmpty())
    packed_string[0] = '\0';
    // Or, delete the string and allocate a new char[1] for it, containing '\0'
    // (don't just delete it and set it to NULL; that would force a re-pack
    // when getPackedString was next called).

  return quote_seen;

}	// pack()


//////////////////////////////////////////
// Returns -1 if stmt (or passed string) contains only zero or more whitespaces
// and optionally a semicolon (" ", "	", ";", "  ;", " ;  ", etc).
//////////////////////////////////////////
Int32 InputStmt::isEmpty(const char *s)
{
  Int32 empty = -1;

  if (!s)
    s = getPackedString();

  for ( ; *s; s++)
    if (*s != ' ' && *s != '\t' && *s != ';')
      {
	empty = 0;
	break;
      }
  return empty;
}	// isEmpty()

//////////////////////////////////////////
// Return:
//	- pointer to one past the semicolon terminating the passed string
//	  and quote_seen_pos always zero
// 	- or NULL if no terminating semicolon exists, in which case
//	  quote_seen_pos is nonzero position in the string if additionally
//	  no terminating quote was found (thus quote_seen_pos is ONE-based,
//	  not zero-based -- see syntaxErrorOnMissingQuote!).
//////////////////////////////////////////
char * InputStmt::findEnd(char * s, size_t &quote_seen_pos)
{
  char *orig = s;
  char quote_seen;
  for (quote_seen=quote_seen_pos=0; ; s++)
    switch (*s)
      {
      case '\'':
      case '"':
	if (!quote_seen)
	  {
	    quote_seen = *s;
	    quote_seen_pos = s - orig + 1;	// +1: one-based position!
	  }
	else if (quote_seen == *s)
	  quote_seen = quote_seen_pos = 0;
	break;
      case ';':
	if (!quote_seen)
	  return ++s;
	break;
      case '\0':
	return NULL;
      }
}	// findEnd

//////////////////////////////////////
void InputStmt::syntaxErrorOnMissingQuote(char * str)
{
  if (!str)
    str = getPackedString();

  SqlciError (SQLCI_INPUT_MISSING_QUOTE, (ErrorParam *) 0);
  size_t quote_pos = 0;
  findEnd(str, quote_pos);

  // subtract one from quote_pos to convert 1-based offset to 0-based
  StoreSyntaxError(str, --quote_pos, sqlci_DA);
  sqlci_parser_syntax_error_cleanup(NULL,sqlci_env);
}	// syntaxErrorOnMissingQuote

//////////////////////////////////////
// Append a bogus string ("!" is an illegal SQL_TEXT character)
// to a copy of the unterminated string
// so the parser is guaranteed to exit with a syntax error message.
// Note that since this is to be called only on EOF,
// we assume this InputStmt will not be saved on the history list
// so there's no need to append <bangeof> to its fragment list and repack it.
//////////////////////////////////////
void InputStmt::syntaxErrorOnEof(const char * str)
{
  // We must embed a space in obeybangeof, else an unterminated OBEY cmd
  // will hang forever (because of the sqlci lexer's special <FNAME> state,
  // called by sqlci_parser_syntax_error_cleanup).
  #define bangeof " !EOF!"
  #define obeybangeof " !EOF EOF!"
  if (!str)
    str = getPackedString();
  char * err;

  err = new char[strlen(str) + strlen(bangeof) + 2];	// +2: \n, \0
  strcpy(err,"\n");
  strcat(err,str);
  strcat(err,bangeof);

  ErrorParam *p1 = new ErrorParam (err);
  SqlciError (SQLCI_INPUT_PREMATURE_EOF,
  	      p1,
	      (ErrorParam *) 0
	     );
  delete p1;
  delete [] err;
}	// syntaxErrorOnEof

void InputStmt::display(UInt16 Distinguish_arg, NABoolean noPrompt) const  // 64-bit
{
  display((Lng32) 0, noPrompt);  // 64-bit
}

void InputStmt::display(Lng32 stmt_num_, NABoolean noPrompt) const
{
  if (!first_fragment) return;

  char   pfxbuf[20] = "";
  Int32  pfxlen;

  if (!noPrompt)
    {
      if (stmt_num_ <= 0)
	  cout << ">>";
      else
	{
	  sprintf(pfxbuf, "%d> %n", stmt_num_, &pfxlen);
	  cout << pfxbuf;
	  memset(pfxbuf, ' ', pfxlen);
	  pfxbuf[pfxlen] = '\0';
	}
    }

  cout << first_fragment->fragment;

  StringFragment * fragment = first_fragment->next;
  while (fragment)
    {
      cout << endl;

      if (stmt_num_ <= 0)
	cout << "+>";
      else
	cout << pfxbuf;

      cout << fragment->fragment;

      fragment = fragment->next;
    }
  cout << endl;
}	// display()

void InputStmt::logStmt(NABoolean noPrompt) const
{
  if (!sqlci_env->get_logfile()->IsOpen()) return;

  StringFragment * fragment = first_fragment;
  if (!first_fragment) return;

  while (fragment)
    {
      char * log_str = new char[2 /* to display sqlci prompt */ +
				strlen(fragment->fragment)];

      size_t start = 0;
      if (!noPrompt)
	{
	  start = 2;
	  if (fragment == first_fragment)
	    strncpy(log_str, ">>", 2);
	  else
	    strncpy(log_str, "+>", 2);
	}

      size_t fraglen = strlen(fragment->fragment);
      strncpy(&log_str[start], fragment->fragment, fraglen);

      sqlci_env->get_logfile()->Write(log_str, start + fraglen);

      delete [] log_str;

      fragment = fragment->next;
    }
}	// logStmt()

InputStmt::Option InputStmt::nextOption()
{
  Option option;

  switch (command[command_pos])
    {
    case 'i':
    case 'I':
      option = INSERT_O;
      command_pos += 1;
      break;

    case 'd':
    case 'D':
      option = DELETE_O;
      command_pos += 1;
      break;

    case 'r':
    case 'R':
      option = EXPLICIT_REPLACE_O;
      command_pos += 1;
      break;

    case ' ':
      option = ADVANCE_O;
      command_pos += 1;
      break;

    case '/':
      {
	if (command[command_pos+1] == '/')
	  {
	    if ((command_pos == 0) && (command[command_pos+2] == 0))
	      option = ABORT_O;
	    else
	      {
		option = END_O;
		command_pos += 2;
	      }
	  }
	else
	  {
	    option = REPLACE_O;
	  }
      }
      break;

    case 0:
      {
	if (command_pos == 0)
	  option = DONE_O;
	else
	  option = AGAIN_O;
      }
      break;

    default:
      option = REPLACE_O;
      break;
    }

  return option;
}	// nextOption()

size_t InputStmt::getCommandLen() const
{
  Int32 done = 0;
  size_t i = 0;

  while (!done)
    {
      if (command[command_pos + i] == 0)
	done = -1;
      else
	if ((command[command_pos+i] == '/') &&
	    (command[command_pos+i+1] == '/'))
	  done = -1;
	else
	  i++;
    }
  return i;
}	// getCommandLen()

void InputStmt::processInsert()
{
  size_t command_len = getCommandLen();

  if ((command_len > 0) && (text_maxlen > 0))
    {
      size_t j = text_maxlen - 1;
      while (j >= text_pos)
	{
	  text[j+command_len] = text[j];
	  if (j-- == 0) break;
	}

      str_cpy_all(&text[text_pos], &command[command_pos], command_len);
      command_pos += command_len;

      text_pos += 2 * command_len + 1;
      text_maxlen += command_len;

      if (text_pos > text_maxlen)
	text_maxlen = text_pos;
    }
}	// processInsert()

void InputStmt::processReplace()
{
  size_t command_len = getCommandLen();

  if (command_len > 0)
    {
      str_cpy_all(&text[text_pos], &command[command_pos], command_len);
      command_pos += command_len;

      text_pos += command_len;

      if (text_pos > text_maxlen)
	text_maxlen = text_pos;
    }
}	// processReplace()

void InputStmt::processDelete()
{
  size_t j = text_pos;

  if (j < text_maxlen)
    {
      while (j < text_maxlen)
	{
	  text[j] = text[j+1];
	  j++;
	}

      text_maxlen -= 1;
    }
}	// processDelete()

//////////////////////////////////////////////////////
// Member "text" is used to communicate with the above processXxx methods;
// the resulting string is returned to caller in parameter "fixed_data",
// not in "text" ("text" not used elsewhere).
// (Member "command" and the "pos/len" members -- same for them.)
//////////////////////////////////////////////////////
InputStmt::Option InputStmt::fix_string(const char * in_data,
					char * fixed_data,
					size_t data_maxlen)
{
  command = new char[data_maxlen + 1];
  command_pos = 0;
  text_pos = 0;

  text = fixed_data;
  text_maxlen = strlen(in_data);
  str_pad(text, data_maxlen, ' ');
  str_cpy_all(text, in_data, strlen(in_data));

  char c;
  Option option;

  cout << ">>" << in_data << endl;
  cout << "..";

  if (cin.peek() != '\n')
     cin.get(command, data_maxlen, '\n');
  else
     command[0] = '\0';
 
  if (cin.eof())
    {
      // Abort the "FC", not the whole SQLCI session
      CLEAR_STDIN_EOF;
      option = ABORT_O;
    }
  else
    {
      // consume the eol ('\n')
      cin.get(c);
      option = EMPTY_O;
    }

  while ((option != DONE_O) && (option != ABORT_O))
    {
      option = nextOption();

      switch (option)
	{
	case INSERT_O:
	  processInsert();
	  break;

	case REPLACE_O:
	  processReplace();
	  break;

	case EXPLICIT_REPLACE_O:
	  processReplace();
	  text_pos++;
	  break;

	case DELETE_O:
	  processDelete();
	  break;

	case ADVANCE_O:
	  text_pos += 1;
	  break;

	case END_O:
	  text_pos += 2;
	  break;

	case ABORT_O:
	  strncpy(text, in_data, strlen(in_data));
	  break;

	case DONE_O:
	  text[text_maxlen] = 0;
	  break;

	case AGAIN_O:
	  text[text_maxlen] = 0;
	  cout << ">>" << text << endl;
	  text[text_maxlen] = ' ';

	  cout << "..";
          if (cin.peek() != '\n')
             cin.get(command, data_maxlen, '\n');
          else
             command[0] = '\0';
	  if (cin.eof())
	    {
	      CLEAR_STDIN_EOF;
	      option = ABORT_O;
	    }
	  else
	    {
	      // consume the eol ('\n')
	      cin.get(c);
	      command_pos = 0;
	      text_pos = 0;
	    }
	  break;

	default:
	  break;

	}
    }

  delete [] command;
  command = 0;
  // delete text;    // text points to fixed_data, memory owned by caller!
  text = 0;

  return option;
}	// fix_string()

//////////////////////////////////////////
// return value: 0, execute new statement.
//               -20, abort it.
//		other error codes from readStmt()
//////////////////////////////////////////
Int32 InputStmt::fix(Int32 append_only)
{
  Int32 retval = 0;

  if (!append_only)
    {
      Option option;

      StringFragment * fragment = first_fragment;

      while (fragment)
	{
	  char * new_fragment = new char[MAX_FRAGMENT_LEN + 1];
	  option = fix_string(fragment->fragment,new_fragment,MAX_FRAGMENT_LEN);
	  if (option == ABORT_O)
	    return -20;	  // caller is responsible for deleting this InputStmt

	  delete [] fragment->fragment;
	  fragment->fragment = new_fragment;
	  fragment = fragment->next;
	}
    }
  else
    {
      retval = readStmt(NULL/*interactive only, so input is stdin*/);
    }

  // Pack (or repack) the new stmt.
  pack();

  return retval;
}	// fix()

//////////////////////////////////////////
// -1, if input contains a "?section secname" statement
//     and secname matches section_name. Or if no section
//     name is specified in which case this is a wildcard
//     section match.
//  0, otherwise.
//////////////////////////////////////////

  // This will allow "?SECTION ABC" or "?SECTION ABC;" or "?SECTION ABC+FOO"
  // (the ";" and the "+FOO" will be ignored -- section ABC will match).
  // Likewise, either of "?SECTION" or "SECTION;" or "SECTION ;" is allowed.
  // This will disallow "?SECTIONA" or "?SECTION1" or "?IGNOREB" or "?IGNORE_2".
  //
  // The rationale is that "?xxx" directives are not lexed/parsed by Sqlci,
  // but "obey file(sect)" is, and "sect" allows only identifier characters.
  #define IS_SECTION_NAME_CHAR(a)	(isalnum(a) || a == '_')

Int32 InputStmt::sectionMatches(const char * section_name)
{
  const char * str = getPackedString();	// any tabs have become spaces!
					// (so no need to test below)

  // Although "?SECTION" does not cause any ambiguities with a sqlci PARAM
  // named "?SECTION", here we allow "#SECTION" for a symmetry of sorts with
  // "#IGNORE" and "#IFDEF".
  if (*str != '?' && *str != '#')	// ?SECTION *or* #SECTION
    return 0;
  if (strlen(str) < 8)
    return 0;

  char tmp[10];
  size_t i = 0;
  for (i=0; i < 8; i++)
    tmp[i] = toupper(str[i]);
  tmp[i] = '\0';
  tmp[0] = '?';				// #SECTION -> ?SECTION
  if (strncmp(tmp, "?SECTION", 8) != 0)
    return 0;

  if (IS_SECTION_NAME_CHAR(str[8]))
    return 0;

  // We have so far matched "?SECTION"
  if (!section_name) 			// wildcard match
    return -1;

  for (i = 8; str[i] == ' '; i++) ;	// now skip whitespace to section name

  size_t s = 0;
  for (s = 0; IS_SECTION_NAME_CHAR(str[i]) && section_name[s]; i++, s++)
    if (toupper(str[i]) != toupper(section_name[s]))
      return 0;

  if (IS_SECTION_NAME_CHAR(str[i]) || section_name[s])
    return 0;

  return -1;
}	// sectionMatches

static NABoolean isIfdefStmtTransition(NAString &nsUpTrim,  // toUpper+trimmed
				       NABoolean *ignoreJustThis)
{
  enum { IFDEF_, IFNDEF_, ELSE_, ENDIF_ };
  Int32 which;

  char *str = (char *)nsUpTrim.data();
  char *s, c;
  for (s = str; *s && !isspace((unsigned char)*s); s++) ;		// find end of 1st tok   // For VS2003
  c = *s;
  *s = '\0';
  if      (strcmp(str, "IFDEF")  == 0) which = IFDEF_;
  else if (strcmp(str, "IFNDEF") == 0) which = IFNDEF_;
  else if (strcmp(str, "ELSE")   == 0) which = ELSE_;
  else if (strcmp(str, "ENDIF")  == 0) which = ENDIF_;
  else return FALSE;

  if (ignoreJustThis)
    *ignoreJustThis = TRUE;

  static const size_t MAX_IFDEF_NESTING_LEVEL = 20;
  static       size_t level = 0;
  static NABoolean    execute[MAX_IFDEF_NESTING_LEVEL+1];
  execute[0] = TRUE;

  if (which == IFDEF_ || which == IFNDEF_) {
    if (level >= MAX_IFDEF_NESTING_LEVEL) {
      ++level;
      return FALSE;
    }
    else if (!execute[level])
      execute[++level] = FALSE;
    else {
      *s = c;
      for ( ; isspace((unsigned char)*s) || *s == '$' || *s == '='; s++) ;  // beg of 2nd tok  //  For VS2003
      const char *env  = *s ? getenv(s) : NULL;
      NABoolean isdef  = (env && *env && *env != '0');
      NABoolean ifdef  = (which == IFDEF_);
      execute[++level] = (isdef == ifdef);
    }
  }
  else if (which == ELSE_) {
    if (level <= MAX_IFDEF_NESTING_LEVEL && level > 0)
      if (execute[level-1]) {
	execute[level] = NOT execute[level];
	return TRUE;
      }
  }
  else {
    if (level <= MAX_IFDEF_NESTING_LEVEL && level > 0) {
      NABoolean transition = (execute[level] != execute[level-1]);
      --level;
      return (transition);
    }
  }

  if (level <= MAX_IFDEF_NESTING_LEVEL && level > 0)
    return (execute[level] != execute[level-1]);

  if (level == 0 && ignoreJustThis)
    *ignoreJustThis = FALSE;
  return FALSE;
}	// isIfdefStmtTransition

//////////////////////////////////////////
// Checks for ?ignore, ?ifMX, ?ifNSKRel1
// and	      #ignore, #ifMX, #ifNSKRel1,
// which are equivalent, but the ? syntax allows a grammar ambiguity:
//	SET PARAM ?ifmx 'abc';
//	SELECT * FROM T WHERE
//	?ifmx
//	= columnA;
// The ? syntax we continue to support until 100 to 200 regress
// tests+expected files (!) can be edited to change to the unambiguous # syntax.
//
// Notice that the new #ifdef, #ifndef, #else, #endif syntax
// supports ONLY the unambiguous form --
// ?ifdef will ALWAYS be interpreted as a sqlci PARAM.
//
// Function returns:
// -1, if input is ?IGNORE, or
//	  input is ?ifMX && we're on NSK Rel 1, or
//	  input is ?ifNSKRel1 && we're on either {NT or NSK Rel 2 or later}
//  0, otherwise -- in which case
//	  "ignoreJustThis" returns TRUE if and only if
//	  input is any of ?IGNORE or ?ifMX or ?ifNSKRel1.
//
// The caller is responsible for ignoring all input between stmts on which
// this function returns -1.
//////////////////////////////////////////
Int32 InputStmt::isIgnoreStmt(const char *str, NABoolean *ignoreJustThis)
{
  if (!str) {
    ComASSERT(packed_string);
    if (ignoreJustThis)
      *ignoreJustThis = ignoreJustThis_;
    return isIgnoreStmt_;
  }

  if (ignoreJustThis)
    *ignoreJustThis = FALSE;

  if (*str != '?' && *str != '#')
    return 0;

  // These directives can only be used when running regressions.  Regressions
  // can be run by setting the SQLMX_REGRESS environment variable.
  // If we are not running regressions, then execute all statements
  //  char *ev = getenv("SQLMX_REGRESS");
  //  if (!ev)
  //    return 0;

  NAString ns(&str[1]);
  TrimNAStringSpace(ns);
  ns.toUpper();

  NABoolean ignoreAll    = FALSE;
  NABoolean ignoreOnMX   = FALSE;
  NABoolean ignoreOnMP   = FALSE;
  NABoolean ignoreOnNT   = FALSE;
  NABoolean ignoreOnLINUX = FALSE;
  NABoolean ignoreOnNSK  = FALSE;

  // #ifNT -> include the input stmt on NT or LINUX  platform
  // #ifLINUX -> include the input stme on LINUX platform
  // #ifNSK -> include the input stme on NSK platform

  if      (ns == "IFMX")   ignoreOnMP  = TRUE;
  else if (ns == "IFMP")   ignoreOnMX  = TRUE;
  else if (ns == "IFNT")   ignoreOnNSK = TRUE; 
  else if (ns == "IFNTNOTLINUX") { ignoreOnNSK = TRUE; ignoreOnLINUX = TRUE; }
  else if (ns == "IFNSK")  { ignoreOnNT  = TRUE; ignoreOnLINUX = TRUE; }
  else if (ns == "IFLINUX")  { ignoreOnNSK = TRUE; ignoreOnNT = TRUE; }
  else if (ns == "IGNORE") ignoreAll   = TRUE;
  else if (*str != '#')                 // Only the #xxxx form is acceptable
    return 0;                           // for all future keywords we add!
  else if (isIfdefStmtTransition(ns, ignoreJustThis))
    return -1;
  else
    return 0;

  if (ignoreJustThis)
    *ignoreJustThis = TRUE;

// We are on an NSK system
  if (ignoreOnLINUX || ignoreOnMX)
    ignoreAll = TRUE;

  return (ignoreAll ? -1 : 0);
}       // isIgnoreStmt

//////////////////////////////////////////
//  Block statement handler. 
//////////////////////////////////////////

void InputStmt::findBlockStmt
(char     *s,			// (IN) : a "NORMAL" string
 size_t    xbeg, 		// (IN) : beginning index of NORMAL string in s
 size_t    xend,		// (IN) : ending index of NORMAL string in s
 NABoolean searchForShellCmd)	// (IN) : recognize shell commands
  // requires: s[xbeg..xend] is a "NORMAL" string, ie, a string of characters
  //                         read by getLine from the NORMAL state
  // modifies: blockStmt_, shellCmd_
  // effects : scans s for compound statements and updates blockStmt_ to
  //           record the number of unterminated compound statements.
  // NB: scan must avoid the pitfalls of genesis case 10-990419-8752, viz:
  //       create table t(
  //         if int, while int
  //       );
  //     must not cause sqlci to hang. Furthermore, single-line compound
  //     statements like
  //       begin insert into t values(1,2); end;
  //     must not cause false syntax errors in arkcmp (due to a premature
  //     truncation of the statement at the first ";").
  // We recognize only the following compound statements:
  //   begin ... end;
  //   case ... end [case];
  //   if ... then ... end if;
  // "begin ... end" is a compound statement but "begin work" is not.
  // "begin work" cannot straddle a line.
  // "end if" cannot straddle a line.
  // We must also handle the following compound statement correctly
  //   begin;
  //     delete from t;
  //     insert into t
  //       select * from t sample first
  //          balance when a < 10 then 5 rows
  //                  when a >=10 then 5 rows
  //          END sort by a;
  //   end;
  // Specifically, the END of a data-mining sample first balance expr
  // must not cause a false syntax error due to a premature end of the
  // compound statment. To do this, we match "balance when" with its
  // "end". But, in doing so, we must not be tripped by "balance when" in
  //   select case balance when 1 then 10 else 7 from t;
  // (Isn't parsing fun?)
  // Other compound statements that may be supported in the future include
  //   loop ... end loop;
  //   while ... end while;
  //   repeat ... until ... end repeat;
  //   for ... end for;
{
  // do nothing if compound statements are not allowed in sqlci
  if (!allowCSinsqlci_) return; 

  static const char* tokenDelimiter=" \t\n\r\f\v(),+-*/;<>=|[]";
  if (xbeg < xend) {
    size_t len = xend-xbeg+1;
    char *p, *tempStr = new char[len+1];
    if (tempStr) {
      strncpy(tempStr, &s[xbeg], len);
      tempStr[len] = 0;
      p = strtok(tempStr, tokenDelimiter);
      while (p) {
	if (shellCmd_) {
	  // don't search for block commands inside a shell command
	}
	else if (searchForShellCmd &&
	    (_stricmp(p, "SH")==0 || _stricmp(p, "SHELL")==0)) {
	  shellCmd_ = 1;
	}
        else if (_stricmp(p, "BEGIN")==0) {
          if ((p=strtok(NULL, tokenDelimiter)) != NULL && 
              _stricmp(p, "WORK")==0) {
            // fall thru & consume "BEGIN WORK" as one statement
          }
          else {
            // consume "BEGIN" as a compound stmt token 
            blockStmt_++;
            continue; // examine next token
          }
        }
        else if (_stricmp(p, "IF")==0)
          blockStmt_++;
        else if (_stricmp(p, "CASE")==0) {
          blockStmt_++;
          // consume any "BALANCE WHEN" here as ordinary tokens
          if ((p=strtok(NULL, tokenDelimiter)) != NULL && 
              _stricmp(p, "BALANCE")==0)
            p = strtok(NULL, tokenDelimiter);
          continue; 
        }
        else if (_stricmp(p, "END")==0) {
          blockStmt_--;
          if ((p=strtok(NULL, tokenDelimiter)) != NULL && 
              _stricmp(p, "IF")==0) {
            // fall thru & consume "END IF" as one token
          }
          else if (p != NULL && _stricmp(p, "CASE")==0) {
            // fall thru & consume "END CASE" as one token
          }
          else {
            // consume "END" as a token & examine next token
            continue;
          }
        }
        else if (_stricmp(p, "BALANCE")==0) {
          if ((p=strtok(NULL, tokenDelimiter)) != NULL && 
              _stricmp(p, "WHEN")==0) {
            // "BALANCE WHEN" starts a new nesting level
            blockStmt_++;
          }
          else {
            // "BALANCE" is an ordinary identifier
            continue; // examine next token
          }
        }

	// the shell command must be the first token
	searchForShellCmd = 0;

	// get the next token in the string
        p = strtok(NULL, tokenDelimiter);
      }
      delete [] tempStr;
    }
  }
}

