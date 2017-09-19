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
 * File:         SqlciStmts.C
 * RCS:          $Id: SqlciStmts.cpp,v 1.8 1998/07/20 07:27:53  Exp $
 * Description:  
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1998/07/20 07:27:53 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

#include <stdlib.h>
#include <ctype.h>

#include "ComASSERT.h"
#include "ComDiags.h"
#include "SqlciStmts.h"
#include "SqlciNode.h"
#include "SqlciCmd.h"
#include "SqlciError.h"
#include "SqlciParser.h"
#include "InputStmt.h"
#include "str.h"

extern ComDiagsArea sqlci_DA;

SqlciStmts::SqlciStmts(Lng32 max_entries_)
{
  last_stmt_num = 0;
  
  max_entries = max_entries_;
  
  First = new StmtEntry();
  
  StmtEntry * curr = NULL;
  StmtEntry * prev = First;
   
  for (short i = 1; i < max_entries; i++)
    {
      curr = new StmtEntry();
      
      prev->setNext(curr);
      curr->setPrev(prev);
      
      prev = curr;
    }
  
  if(curr != NULL)
  {
    curr->setNext(First);
    First->setPrev(curr);
    Last = First;
  }
}

SqlciStmts::~SqlciStmts()
{
  StmtEntry * curr = First;
  StmtEntry * next = NULL;
 
  for (short i = 1; i < max_entries; i++)
    {
      next = curr->getNext();
      delete curr;
      curr = next;
    }

  if(next != NULL)
    delete next;
}

void SqlciStmts::add(InputStmt * input_stmt_)
{
  if (!input_stmt_->isEmpty())
    {
      // input_stmt is not a comment string.
      // Go ahead and add it to the the history list.
      // Otherwise, just ignore it.

      Last->set(++last_stmt_num, input_stmt_);	// deletes prev InputStmt
      input_stmt_->setInHistoryList(TRUE);	// mark the stmt just added
  
      Last = Last->getNext();
  
      if (Last == First)
	First = First->getNext();
    }
}

InputStmt * SqlciStmts::get(Lng32 stmt_num_) const
{
  if (First == Last) // empty, no statements added yet
    return 0;
  
  if ((stmt_num_ < First->getStmtNum()) ||
      (stmt_num_ > Last->getPrev()->getStmtNum()))
    return 0;
  
  StmtEntry * curr = First;
  
  while ((curr != Last) &&
	 (curr->getStmtNum() != stmt_num_))
    curr = curr->getNext();
  
  if (curr != Last)
    return curr->getInputStmt();
  else
    return 0;
  
}

InputStmt * SqlciStmts::get(char * input_string_) const
{
  if (First == Last) // empty, no statements added yet
    return 0;
  
  size_t input_len = strlen(input_string_);

  StmtEntry * curr = Last;

  while (curr != First)
    {
      char * curr_string = curr->getPrev()->getInputStmt()->getPackedString();
      
      if (input_len <= strlen(curr_string))
	if (strncmp(input_string_, curr_string, input_len) == 0)
	  return curr->getPrev()->getInputStmt();
	else
	  {
	    // input_string_ is a single SQLCI stmt keyword, so we don't have
	    // to worry about quoted literals and delimited identifiers now
	    // while we do a quick case-insensitive comparison
	    size_t i = 0;
	    for (i = 0; i < input_len; i++)
	      if (toupper(input_string_[i]) != toupper(curr_string[i]))
		break;
	    if (i == input_len)
	      return curr->getPrev()->getInputStmt();
	  }
      
      curr = curr->getPrev();
    }
  
  return 0; // not found
}

// get last statement
InputStmt * SqlciStmts::get() const
{
  if (First == Last) // empty, no statements added yet
    return 0;
 
  return Last->getPrev()->getInputStmt();
}

// remove most recent stmt (FC or !) from history list,
// but do not delete it.
// (see StmtEntry::disconnect below)
void SqlciStmts::remove()
{
  if (First == Last) // empty, no statements added yet
    return;

  Last = Last->getPrev();
  last_stmt_num--;
  
  InputStmt * input_stmt_ = Last->getInputStmt();
  ComASSERT(input_stmt_);
  input_stmt_->setInHistoryList(FALSE);  // out of list, for future delete
  Last->disconnect();
}


void SqlciStmts::display(Lng32 num_stmts_) const
{
  if (num_stmts_ >= max_entries)
    num_stmts_ = max_entries - 1;
  
  StmtEntry * curr = Last;
  
  Lng32 i = 0;
  
  while ((curr != First) && (i < num_stmts_))
    {
      curr = curr->getPrev();
      i++;
    }
  
  while (curr != Last)
    {
      curr->getInputStmt()->display(curr->getStmtNum());
      curr = curr->getNext();
    }
  
}


///////////////////////////////////////////////////////
// class StmtEntry
///////////////////////////////////////////////////////
StmtEntry::StmtEntry()
{
  stmt_num = 0;
  input_stmt = 0;
  next = 0;
  prev = 0;
}

StmtEntry::~StmtEntry()
{
  delete input_stmt;	// delete regardless of stmt's "inHistoryList" setting
}

void StmtEntry::disconnect()
{
  // disconnect the just-inserted FC/! command from history list
  // for later deleting in main SqlciEnv interpreter loop
  // (must wait to delete it till returned there because main loop
  // retains a pointer to it and we don't want to leave any dangling pointers
  // which would happen if we deleted the stmt now)
  ComASSERT(input_stmt && !input_stmt->isInHistoryList());
  input_stmt = 0;
}

void StmtEntry::set(Lng32 stmt_num_, InputStmt * input_stmt_)
{
  stmt_num = stmt_num_;
  // overwrite the n'th previous stmt in the circular history list
  delete input_stmt;	// delete regardless of stmt's "inHistoryList" setting
  input_stmt = input_stmt_;
}


///////////////////////////////////////////////////////
// SqlciStmts-oriented "process" methods
///////////////////////////////////////////////////////

short History::process(SqlciEnv * sqlci_env)
{
  if (!get_argument()) // display last 10 commands -- default.
    sqlci_env->getSqlciStmts()->display(10);
  else
    sqlci_env->getSqlciStmts()->display(atoi(get_argument()));  
  return 0;
}


short ListCount::process(SqlciEnv * sqlci_env)
{
  if (!get_argument())
  {
    sqlci_env->setListCount();
  }
  else
  {
    char *tmp;
    ULng32 val = strtoul(get_argument(), &tmp, 10/*decimal base*/);
    if (tmp == get_argument())
      { /* ##?emit an errmsg, illegal value? */ }
    else
      sqlci_env->setListCount(val);
  }
  return 0;
}


short Verbose::process(SqlciEnv * sqlci_env)
{
  switch (type_)
    {
    case SET_ON:
      sqlci_env->get_logfile()->setVerbose(1);
      break;

    case SET_OFF:
      sqlci_env->get_logfile()->setVerbose(0);
      break;
    }

  return 0;
}


short FCRepeat::process(SqlciEnv * sqlci_env)
{
  Int32 retval = 0;

  if (sqlci_env->isOleServer())
  {
	SqlciError (SQLCI_CMD_NOT_SUPPORTED,
				(ErrorParam *) 0 );
	return 0;
  }

  // ignore if "!" is in an obey file, or stdin redirected from a file
  // (should we display an informative error message?)
  if (!sqlci_env->isInteractiveNow())
    return retval;

  InputStmt * input_stmt = 0;

  // Don't add the repeat cmd to the sqlci stmts list. 
  sqlci_env->getSqlciStmts()->remove();

  if (cmd != 0) // Character string was provided...
    { 
      input_stmt = sqlci_env->getSqlciStmts()->get(cmd);
    }
  else
    {
      if (!cmd_num)
	input_stmt = sqlci_env->getSqlciStmts()->get();
      else
	{
	  if (neg_num) cmd_num = (sqlci_env->getSqlciStmts()->last_stmt()  
				  - cmd_num) + 1;

	  input_stmt = sqlci_env->getSqlciStmts()->get(cmd_num);
	}
    }

  if (input_stmt)
    {

      // Log the statement
      input_stmt->logStmt();

      // Add the repeated command to the stmt list
      // Make a separate copy of the cmd to be repeated; even tho it will not
      // change, a non-separate copy (i.e. 2 pointers pointing at same entity)
      // will free same memory twice in ~SqlciStmts (delete InputStmt).
      InputStmt *new_input_stmt = new InputStmt(sqlci_env);
      *new_input_stmt = input_stmt;
      sqlci_env->getSqlciStmts()->add(new_input_stmt);

      input_stmt->display((UInt16)0); //64bit project: add dummy arg - prevent C++ error
      
      SqlciNode * sqlci_node = 0;
      
      sqlci_parser(input_stmt->getPackedString(),
		   input_stmt->getPackedString(),
		   &sqlci_node, 
		   sqlci_env);
      
      if (sqlci_node)
	{
	  retval = sqlci_node->process(sqlci_env);
	  delete sqlci_node;
	}
    }
  else
    {
      SqlciError(SQLCI_NO_STMT_MATCH, (ErrorParam *) 0);
    }
  
  return retval;
}


short FixCommand::process(SqlciEnv * sqlci_env)
{
  Int32 retval = 0;

  if (sqlci_env->isOleServer())
  {
	SqlciError (SQLCI_CMD_NOT_SUPPORTED,
				(ErrorParam *) 0 );
	return 0;
  }

  // ignore if FC is in an obey file, or stdin redirected from a file
  // (should we display an informative error message?)
  if (!sqlci_env->isInteractiveNow())
    return retval;

  InputStmt *input_stmt = 0, *stmt = 0;

  // Don't add the FC cmd to the sqlci stmts list. 
  sqlci_env->getSqlciStmts()->remove();

  if (cmd != 0) // Character string was provided...
    { 
      input_stmt = sqlci_env->getSqlciStmts()->get(cmd);
    }
  else
    {
      if (!cmd_num)
	input_stmt = sqlci_env->getSqlciStmts()->get();
      else
	{
	  if (neg_num) cmd_num = (sqlci_env->getSqlciStmts()->last_stmt()  
				  - cmd_num) + 1;
	  
	  input_stmt = sqlci_env->getSqlciStmts()->get(cmd_num);
	}
    }

  if (input_stmt)
    {
      enum { DUNNO, YES, NO };
      Int32 is_single_stmt = DUNNO;
      InputStmt * fc_input_stmt = new InputStmt(sqlci_env);
      *fc_input_stmt = input_stmt;

      // Prompt user to fix the input stmt.
      // Fix() value is 0 if we are to execute (and log and history) new stmt,
      // -20 if user "aborted" the FC via an EOF or "//" at the prompt
      // (we must emulate this behavior at the -20 section later on!)
      //
      if (fc_input_stmt->fix() != -20)
	{
	  if (!fc_input_stmt->isEmpty())
	    {
	      // Clear any syntax errors thrown by InputStmt::fix();
	      // we'll get to 'em one at a time in this loop
	      sqlci_DA.clear();

	      // Looping, process one or more commands ("a;b;c;")
	      // on the FC input line.
	      char * packedStr = fc_input_stmt->getPackedString();
	      do
		{
		  size_t quotePos;		// unterminated quote seen?
		  char packedEndC = '\0';
		  char * packedEnd = fc_input_stmt->findEnd(packedStr, quotePos);
		  if (!quotePos)
		    {
		      // No unterminated quote

		      if (is_single_stmt == DUNNO)
			is_single_stmt = 
			  (!packedEnd || fc_input_stmt->isEmpty(packedEnd)) ?
			  YES : NO;
			
		      if (packedEnd)		// semicolon seen
			{
			  packedEndC = *packedEnd;
			  *packedEnd = '\0';
			  if (is_single_stmt == YES && packedEndC)
			    is_single_stmt = NO;
			}

		      if (!fc_input_stmt->isEmpty(packedStr))
			{
			  if (is_single_stmt == YES)
			    stmt = fc_input_stmt;
			  else
			    stmt = new InputStmt(fc_input_stmt, packedStr);

			  Int32 read_error = 0;

			  // Unterminated stmt (no ";" seen),
			  // so prompt for the rest of it.
			  // If user enters EOF in the appended lines,
			  // then log it, but don't history it or execute it.
			  if (!packedEnd)
			    {
			      read_error = stmt->fix(-1/*append_only*/);
	      		      packedStr = stmt->getPackedString();
			    }

			  stmt->logStmt();
			  if (read_error != -20)       // see "abort" note above
			    sqlci_env->getSqlciStmts()->add(stmt);

			  if (!read_error)
			    {
			      SqlciNode * sqlci_node = 0;
			      sqlci_parser(packedStr, packedStr, &sqlci_node, sqlci_env);
			      if (sqlci_node)
				{
				  retval = sqlci_node->process(sqlci_env);
				  delete sqlci_node;
				}
			    }

			  sqlci_env->displayDiagnostics();
			  sqlci_DA.clear();
			  if (retval == -1)		// EXIT command
			    break;
			}

		      if (packedEnd)
			{
			  *packedEnd = packedEndC;
			  packedStr = packedEnd;
			}
		      else
			break;		// terminate the unterminated stmt
		    }
		  else
		    {
		      // If unterminated quote, log it and history it and
		      // display error message and exit the loop;
		      // if just trailing blanks, it's no error, exit the loop.
		      if (!fc_input_stmt->isEmpty(packedStr))
			{
			  if (is_single_stmt == DUNNO)
			    {
			      is_single_stmt = YES;
			      stmt = fc_input_stmt;
			    }
			  else
			    stmt = new InputStmt(fc_input_stmt, packedStr);
			  stmt->logStmt();
			  sqlci_env->getSqlciStmts()->add(stmt);
			  fc_input_stmt->syntaxErrorOnMissingQuote(packedStr);
			}
		      break;
		    }
		}
	      while (*packedStr);
	    }
	}
      if (is_single_stmt != YES)
	delete fc_input_stmt;
    }
  else
    {
      SqlciError(SQLCI_NO_STMT_MATCH, (ErrorParam *) 0);
    }

  return retval;
} // end FixCommand::process

