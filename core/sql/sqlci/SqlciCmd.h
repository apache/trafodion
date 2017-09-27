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

#ifndef SQLCICMD_H
#define SQLCICMD_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciCmd.h
 * Description:
 *
 * Created:      4/15/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "SqlciNode.h"
#include "SqlciEnv.h"


class SqlciCmd : public SqlciNode {
public:
  enum sqlci_cmd_type {
    ENV_TYPE, ERROR_TYPE, EXIT_TYPE,
    FC_TYPE,
    HELP_TYPE, HISTORY_TYPE,
    LISTCOUNT_TYPE, VERBOSE_TYPE, PARSERFLAGS_TYPE,
    LOG_TYPE,
    OBEY_TYPE,
    REPEAT_TYPE,
    SETENVVAR_TYPE, SETPARAM_TYPE, SETPATTERN_TYPE,
    SET_TERMINAL_CHARSET_TYPE,
    SHOW_TYPE, STATISTICS_TYPE,
    SHAPE_TYPE, WAIT_TYPE,
    MODE_TYPE, QUERYID_TYPE,
    SET_ISO_MAPPING_TYPE,
    SET_DEFAULT_CHARSET_TYPE,
    SET_INFER_CHARSET_TYPE
  };

private:
  sqlci_cmd_type cmd_type;
  char *argument;
  Lng32 arglen;
  Int32  numeric_arg;

public:
  SqlciCmd(const sqlci_cmd_type cmd_type_);
  SqlciCmd(const sqlci_cmd_type cmd_type_, char *, Lng32);
  SqlciCmd(const sqlci_cmd_type cmd_type_, NAWchar *, Lng32);
  SqlciCmd(const sqlci_cmd_type cmd_type_, Int32);
  ~SqlciCmd();
  inline char * get_argument(char * dummy_arg = 0){return argument;};
  inline Lng32   get_arglen(){return arglen;};
};

class Shape : public SqlciCmd {
private:
  NABoolean type_;
  char * infile_;
  char * outfile_;
public:
  Shape(NABoolean, char *, char *);
  ~Shape(){};
  short process(SqlciEnv * sqlci_env);
  short processNextStmt(SqlciEnv * sqlci_env, FILE * fStream);
};

class Statistics : public SqlciCmd {
public:
  enum StatsCmdType {SET_ON, SET_OFF};
private:
  StatsCmdType type_;
  char *       statsOptions_;
public:
  Statistics(char *, Lng32 arglen_, StatsCmdType,
	     char * statsOptions);
  ~Statistics();
  short process(SqlciEnv * sqlci_env);

  char * getStatsOptions() { return statsOptions_; }
};

class QueryId : public SqlciCmd {
public:
  QueryId(char * argument_, Lng32 arglen_,
          NABoolean isSet, char * qidVal);
  ~QueryId();
  short process(SqlciEnv * sqlci_env);
 private:
  NABoolean isSet_;
  char * qidVal_;
};

class FixCommand : public SqlciCmd {
  Lng32 cmd_num;
  char *cmd;
  short neg_num;
public:
  FixCommand(char *, Lng32);
  FixCommand(Int32, short);
  ~FixCommand(){};
  short process(SqlciEnv * sqlci_env);
};

class FCRepeat : public SqlciCmd {
  Lng32 cmd_num;
  char *cmd;
  short neg_num;
public:
  FCRepeat(char *, Lng32);
  FCRepeat(Int32, short);
  ~FCRepeat(){};
  short process(SqlciEnv * sqlci_env);
};

class Obey : public SqlciCmd {
  char * section_name;
public:
  Obey(char *, Lng32 arglen_, char * section_name_);
  ~Obey(){};
  short process(SqlciEnv * sqlci_env);
};

class Log : public SqlciCmd {
public:
  enum log_type {CLEAR_, APPEND_, STOP_};
private:
  log_type type;
  Int32 commandsOnly_;
public:
  Log(char *, Lng32 arglen_, log_type type_, Int32 commands_only);
  ~Log(){};
  short process(SqlciEnv * sqlci_env);
};

class History : public SqlciCmd {
public:
  History(char *, Lng32 arglen_);
  short process(SqlciEnv * sqlci_env);
};

class ListCount : public SqlciCmd {
public:
    ListCount(char *, Lng32);
    short process(SqlciEnv * sqlci_env);
};

class Mode :  public SqlciCmd {
public:
    enum ModeType { SQL_};
    Mode(ModeType type, NABoolean value);
    ~Mode(){};
    short process(SqlciEnv * sqlci_env);
private:
    ModeType type;
    NABoolean value;
    short process_sql(SqlciEnv * sqlci_env);
    short process_display(SqlciEnv * sqlci_env);
};

class Verbose : public SqlciCmd {
public:
  enum VerboseCmdType { SET_ON, SET_OFF };

private:
  VerboseCmdType type_;

public:
  Verbose(char *, Lng32 arglen_, VerboseCmdType);
  ~Verbose(){};
  short process(SqlciEnv * sqlci_env);
};

class ParserFlags : public SqlciCmd {

public:
  enum ParserFlagsOperation { DO_SET, DO_RESET };

private:
  Lng32 param;
  ParserFlagsOperation opType;

public:
  ParserFlags(ParserFlagsOperation, Int32 param_);
  ~ParserFlags(){};
  short process(SqlciEnv * sqlci_env);
};

class SetTerminalCharset : public SqlciCmd {

private:

public:
  SetTerminalCharset(char* new_cs_name) :
  SqlciCmd(SET_TERMINAL_CHARSET_TYPE, new_cs_name, strlen(new_cs_name)) {};
  ~SetTerminalCharset(){};
  short process(SqlciEnv * sqlci_env);
};

class SetIsoMapping : public SqlciCmd {

private:

public:
  SetIsoMapping(char* new_cs_name) :
  SqlciCmd(SET_ISO_MAPPING_TYPE, new_cs_name, strlen(new_cs_name)) {};
  ~SetIsoMapping(){};
  short process(SqlciEnv * sqlci_env);
};

class SetDefaultCharset : public SqlciCmd {

private:

public:
  SetDefaultCharset(char* new_cs_name) :
  SqlciCmd(SET_DEFAULT_CHARSET_TYPE, new_cs_name, strlen(new_cs_name)) {};
  ~SetDefaultCharset(){};
  short process(SqlciEnv * sqlci_env);
};

class SetInferCharset : public SqlciCmd {

private:

public:
  SetInferCharset(char* new_boolean_setting) :
  SqlciCmd(SET_INFER_CHARSET_TYPE, new_boolean_setting, strlen(new_boolean_setting)) {};
  ~SetInferCharset(){};
  short process(SqlciEnv * sqlci_env);
};

class Error : public SqlciCmd {
public:
  enum error_type {BRIEF_, DETAIL_, ENVCMD_};
private:
  error_type type;
public:
  Error(char *, Lng32 arglen_, error_type type_);
  ~Error(){};
  short process(SqlciEnv * sqlci_env);
};

class Help : public SqlciCmd {
public:
  enum help_type {SYNTAX_, EXAMPLE_, DETAIL_};
private:
  help_type type;
public:
  Help(char *, Lng32, help_type);
  ~Help(){};
  short process(SqlciEnv * sqlci_env);
};

class Env : public SqlciCmd {
public:
  Env(char *, Lng32 arglen_);
  short process(SqlciEnv * sqlci_env);
};

class Exit : public SqlciCmd {
public:
  Exit(char *, Lng32 arglen_);
  short process(SqlciEnv * sqlci_env);
};

class Reset : public SqlciCmd {
public:
  enum reset_type {PARAM_, PATTERN_, PREPARED_, CONTROL_};
private:
  reset_type type;
  short reset_control(SqlciEnv * sqlci_env);
  short reset_param(SqlciEnv * sqlci_env);
  short reset_pattern(SqlciEnv * sqlci_env);
  short reset_prepared(SqlciEnv * sqlci_env);
public:
  Reset(reset_type type, char * argument_, Lng32 arglen_);
  Reset(reset_type type);
  ~Reset();
  short process(SqlciEnv * sqlci_env);
};

class SetParam : public SqlciCmd {
  char * param_name;
  Lng32 namelen;
  CharInfo::CharSet cs;
  NABoolean inSingleByteForm_;
  NABoolean isQuotedStrWithoutCharSetPrefix_;  // set to TRUE in w:/sqlci/sqlci_yacc.y
                                               // if the parameter value is a string
                                               // literal (i.e., quoted string) AND
                                               // the string literal does not have a
                                               // string literal character set prefix;
                                               // otherwise, this data member is set
                                               // to FALSE.
  NAWchar * m_convUTF16ParamStrLit;  // When isQuotedStrWithoutCharSetPrefix_ is TRUE,
                                     // this data member points to the UTF16 string
                                     // literal equivalent to the specified quoted
                                     // string parameter; otherwise, this data member
                                     // is set to NULL.
  CharInfo::CharSet m_termCS;  // When isQuotedStrWithoutCharSetPrefix_ is TRUE, this
                               // data member contains the TERMINAL_CHARSET CQD
                               // setting at the time the SET PARAM command was
                               // executed; otherwise, this data member is set to
                               // CharInfo:UnknownCharSet.

public:
  // if arglen_ passed in is -1, then set param to null value.
  SetParam(char *, Lng32, char *, Lng32 arglen_, CharInfo::CharSet cs = CharInfo::UnknownCharSet);
  SetParam(char *, Lng32, NAWchar *, Lng32 arglen_, CharInfo::CharSet cs = CharInfo::UnknownCharSet);
  SetParam(char *, Lng32);
  ~SetParam();
  short process(SqlciEnv * sqlci_env);

  CharInfo::CharSet getCharSet() { return cs; };
  NABoolean isInSingleByteForm() { return inSingleByteForm_; };
  NAWchar * getUTF16ParamStrLit() { return m_convUTF16ParamStrLit; }
  void setUTF16ParamStrLit(const NAWchar * utf16Str, size_t ucs2StrLen);
  CharInfo::CharSet getTermCharSet() const { return m_termCS; }
  void setTermCharSet(CharInfo::CharSet termCS) { m_termCS = termCS; }
  NABoolean isQuotedStrWithoutCharSetPrefix() const { return isQuotedStrWithoutCharSetPrefix_; }
  void setQuotedStrWithoutPrefixFlag(NABoolean value) { isQuotedStrWithoutCharSetPrefix_ = value; }
};

class SetPattern : public SqlciCmd {
  char * pattern_name;
  Lng32 namelen;
public:
  SetPattern(char *, Lng32, char *, Lng32 arglen_);
  SetPattern(char *, Lng32);
  ~SetPattern();

  // this method defined in Param.cpp
  short process(SqlciEnv * sqlci_env);
};

class Show : public SqlciCmd {
public:
  enum show_type {CURSOR_, PARAM_, PATTERN_,
		  PREPARED_, CONTROL_,
		  SESSION_, VERSION_};
private:
  show_type type;

  // show values if set by shell/tacl before invoking mxci.
  // Currently used to show defines only.
  NABoolean allValues_;

  short show_control(SqlciEnv * sqlci_env);
  short show_cursor(SqlciEnv * sqlci_env);
  short show_param(SqlciEnv * sqlci_env);
  short show_pattern(SqlciEnv * sqlci_env);
  short show_prepared(SqlciEnv * sqlci_env);
  short show_session(SqlciEnv * sqlci_env);
  short show_version(SqlciEnv * sqlci_env);
public:
  Show(show_type type, NABoolean allValues);
  ~Show();
  short process(SqlciEnv * sqlci_env);
};

class Wait : public SqlciCmd {
public:
  Wait(char *, Lng32);
  ~Wait(){};
  short process(SqlciEnv * sqlci_env);
};

#endif
