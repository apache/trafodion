/* -*-C++-*-
 *****************************************************************************
 *
 * File:         InputStmt.h
 * RCS:          $Id: InputStmt.h,v 1.5.10.2 1998/09/10 19:38:22  Exp $
 * Description:  
 *               
 * Created:      1/10/95
 * Modified:     $ $Date: 1998/09/10 19:38:22 $ (GMT)
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

#ifndef INPUTSTMT_H
#define INPUTSTMT_H

#include "SqlciEnv.h"

// Globals for Break handling.
// The enum values must be negative,
// i.e. invalid for a char value gotten from stdin,
// and also != -1, i.e. != EOF value from stdio.h.
extern volatile Int32 breakReceived;
extern volatile char Sqlci_PutbackChar;
enum { LOOK_FOR_BREAK = -15, FOUND_A_BREAK = -16};

class InputStmt {

private:
  struct StringFragment {
    char * fragment;
    StringFragment * next;
  };
  StringFragment * first_fragment;
  char * packed_string;
  Int32 isIgnoreStmt_;
  NABoolean ignoreJustThis_;
  NABoolean veryFirstLine_;

  Int32 isInHistoryList_;
  Int32 blockStmt_;
  Int32 shellCmd_;
  NABoolean allowCSinsqlci_;

  SqlciEnv * sqlci_env;

  // The rest of these are used only by fix() and fix_string() and such
  char * command;
  char * text;
  size_t text_maxlen;
  size_t text_pos;
  size_t command_pos;
  
public:
  InputStmt(SqlciEnv * the_sqlci_env);
  InputStmt(const InputStmt * source, const char * packed);
  ~InputStmt();
  void operator=(const InputStmt * source);

  // 64bit Project: Add Distinguish_arg to resolve ambiguous definition error
  // by C++ compiler (because noPrompt and stmt_num_ have the same underlying type.)
  void display(UInt16 Distinguish_arg, NABoolean noPrompt = FALSE) const;

  void display(Lng32 stmt_num_, NABoolean noPrompt = FALSE) const;
  Int32 fix(Int32 append_only = 0);
  Int32 isEmpty(const char *str = NULL);
  Int32 isIgnoreStmt(const char *str = NULL, NABoolean *ignoreJustThis = NULL);
  NABoolean ignoreJustThis() const		{ return ignoreJustThis_; }
  Int32 isInHistoryList() const			{ return isInHistoryList_; }
  void setInHistoryList(Int32 boolean)		{ isInHistoryList_ = boolean; }
  void setVeryFirstLine()		{ veryFirstLine_ = TRUE; }
  Int32 sectionMatches(const char * section = NULL);
  void syntaxErrorOnMissingQuote(char *str = NULL);
  void syntaxErrorOnEof(const char *str = NULL);
  char * findEnd(char * s, size_t &quote_seen_pos);
  void logStmt(NABoolean noPrompt = FALSE) const;
  Int32 readStmt(FILE * non_stdin_file = NULL, Int32 suppress_blank_line_output =0);
  Int32 consumeLine(FILE * non_stdin_file = NULL);

  inline char * getPackedString()
  {
    if (!packed_string)
      pack();
    return packed_string;
  };

  // This enum is used only privately, but appears in function result type
  // for two functions below and c89 therefore requires it to be public
  enum Option
    {
      INSERT_O, DELETE_O, REPLACE_O, EXPLICIT_REPLACE_O, 
      ADVANCE_O, ABORT_O, END_O, DONE_O, AGAIN_O, EMPTY_O
    };

private:
  // a line in a text file on Seaquest platform can be very long
  // Allocate 1 Mbytes for now...
  enum {MAX_FRAGMENT_LEN = 256 * 4096};
  Option fix_string(const char *in_data, char *fixed_data, size_t max_datalen);
  size_t getCommandLen() const;
  Int32 getLine(char * input_str, FILE * non_stdin_file, Int32 first_line);
  Option nextOption();
  Int32 pack();
  void processInsert();
  void processReplace();
  void processDelete();
  inline char * getFirstFragment() const
  {
    if (first_fragment)
      return first_fragment->fragment;
    else
      return 0;
  };

  NABoolean inBlockStmt() { return allowCSinsqlci_ && blockStmt_ > 0; }
  void findBlockStmt(char *s, size_t xbeg, size_t xend,
		     NABoolean searchForShellCmd);
};

#endif
