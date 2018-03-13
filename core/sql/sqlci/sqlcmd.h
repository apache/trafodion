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

#ifndef SQLCMD_H
#define SQLCMD_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlCmd.h
 * RCS:          $Id: SqlCmd.h,v 1.13 1998/09/07 21:49:58  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1998/09/07 21:49:58 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "NAType.h"             // for DEFAULT_CHARACTER_LENGTH
#include "SqlciNode.h"
#include "SqlciEnv.h"
#include "ComDistribution.h"

// Revision 1.6.8.1  1998/04/17 16:27:46
// reaching nchar milestone
//
extern void HandleCLIErrorInit();
extern void HandleCLIError(Lng32 &err, SqlciEnv *sqlci_env, 
			   NABoolean displayErr = TRUE,
			   NABoolean * isEOD = NULL,
                           Int32 prepcode = 0, NABoolean getWarningsWithEOF = FALSE);
extern void HandleCLIError(SQLSTMT_ID *stmt, Lng32 &err, SqlciEnv *sqlci_env, 
			   NABoolean displayErr = TRUE,
			   NABoolean * isEOD = NULL,
                           Int32 prepcode = 0);

void handleLocalError(ComDiagsArea *diags, SqlciEnv *sqlci_env);
Int64 getRowsAffected(SQLSTMT_ID *stmt);
Int64 getDiagsCondCount(SQLSTMT_ID *stmt);
// for unnamed parameters
#define MAX_NUM_UNNAMED_PARAMS  128
#define MAX_LEN_UNNAMED_PARAM 300

class Execute;

class SqlCmd : public SqlciNode {
public:
  enum sql_cmd_type {
    DML_TYPE, PREPARE_TYPE,  EXECUTE_TYPE, CURSOR_TYPE,
    GOAWAY_TYPE, DESCRIBE_TYPE, QUERYCACHE_TYPE,
    USAGE_TYPE, QUIESCE_TYPE, STORE_EXPLAIN_TYPE

  };
private:
  sql_cmd_type cmd_type;
  char *sql_stmt;
  Int32 sql_stmt_oct_length;

public:
  SqlCmd(const sql_cmd_type cmd_type_, const char * argument_);
  ~SqlCmd();
  char * get_sql_stmt() {return sql_stmt;}
  inline Int32 get_sql_stmt_oct_length() {return sql_stmt_oct_length;};

  static short do_prepare(SqlciEnv *, PrepStmt *, 
			  char * sqlStmt,
			  NABoolean resetLastExecStmt = TRUE,
			  Lng32 rsIndex = 0,
                          Int32 *prepcode = NULL,
                          Lng32 *statisticsType = NULL);

  static short updateRepos(SqlciEnv * sqlci_env, SQLSTMT_ID * stmt, char * queryId);

  static short do_execute(SqlciEnv *, PrepStmt *, 
			  Int32 numUnnamedParams = 0,
			  char ** unnamedParamArray = NULL,
			  CharInfo::CharSet* unnamedParamCharSetArray = NULL,
                                          Int32 prepcode = 0);
  //////////////////////////////////////////////////////////
  // made the change to add static in front of the function
  // declaration! Reason - Beacause we will be accessing that 
  // method from outside the files from where it was decalred.  In
  // that case you have to make it static to get to it otherwise
  // it wont link.
  ////////////////////////////////////////////////////////////

  static short doExec(SqlciEnv *, SQLSTMT_ID *, PrepStmt *, 
		      Int32 numUnnamedParams = 0,
		      char ** unnamedParamArray = NULL,
                      CharInfo::CharSet* unnamedParamCharSetArray = NULL,
		      NABoolean handleError = TRUE);
  static short doDescribeInput(SqlciEnv *, SQLSTMT_ID *, PrepStmt *, 
			       Lng32 num_input_entries,
			       Int32 numUnnamedParams = 0,
			       char ** unnamedParamArray = NULL,
			       CharInfo::CharSet* unnamedParamCharSetArray = NULL);
  static short doFetch(SqlciEnv *, SQLSTMT_ID * stmt, 
		       PrepStmt * prep_stmt, 
		       NABoolean firstFetch = FALSE,
		       NABoolean handleError = TRUE,
                                  Int32 prepcode = 0);
  static short doClearExecFetchClose(SqlciEnv *, SQLSTMT_ID *, PrepStmt *, 
				     Int32 numUnnamedParams = 0,
				     char ** unnamedParamArray = NULL,
				     CharInfo::CharSet* unnamedParamCharSetArray = NULL,
				     NABoolean handleError = TRUE);
  static short getHeadingInfo(SqlciEnv * sqlci_env,
			      PrepStmt * prep_stmt,
			      char * headingRow,
			      char * underline);
  
  static short displayHeading(SqlciEnv * sqlci_env,
			      PrepStmt * prep_stmt);
  static Lng32 displayRow(SqlciEnv * sqlci_env, 
			  PrepStmt * prep_stmt);

  static void addOutputInfoToPrepStmt(SqlciEnv *sqlci_env,
                                      PrepStmt *prep_stmt);

  static short deallocate(SqlciEnv * sqlci_env, PrepStmt * prep_stmt);

  static short executeQuery(const char *query, SqlciEnv *sqlci_env);

  static short setEnviron(SqlciEnv *sqlci_env, Lng32 propagate);

  static short showShape(SqlciEnv *sqlci_env, const char *query);

  static char * replacePattern(SqlciEnv * sqlci_env, char * inStr);

  static void clearCLIDiagnostics();

  static short cleanupAfterError(Lng32 retcode,
                                 SqlciEnv * sqlci_env,
                                 SQLSTMT_ID * stmt,
                                 SQLDESC_ID *sql_src,
                                 SQLDESC_ID *output_desc,
                                 SQLDESC_ID *input_desc,
                                 NABoolean resetLastExecStmt);
};

class DML : public SqlCmd {
private:
  char * this_stmt_name;
  dml_type type;

  // $$$$ SPJ RS THROWAWAY
  Lng32 rsIndex_;

public:
  DML(const char * argument_, dml_type type_, const char * stmt_name_ = NULL);
  ~DML();
  short process(SqlciEnv * sqlci_env);

  // $$$$ SPJ RS THROWAWAY
  void setResultSetIndex(Lng32 i) { rsIndex_ = i; }
  Lng32 getResultSetIndex() const { return rsIndex_; }

};

class Prepare : public SqlCmd {
  char * this_stmt_name;
  dml_type type;
public:
  Prepare(char * stmt_name_, char * argument_, dml_type type_);
  ~Prepare();
  short process(SqlciEnv * sqlci_env);
};

class DescribeStmt : public SqlCmd {
  char * stmtName_;
public:
  DescribeStmt(char * stmtName, char *argument);
  ~DescribeStmt();
  short process(SqlciEnv * sqlciEnv);
  short displayEntries(SqlciEnv *sqlci_env,
                       SQLDESC_ID *desc, Lng32 numEntries,
                       Logfile *log);
};

class Execute : public SqlCmd {
private:
  char * using_params[MAX_NUM_UNNAMED_PARAMS];
  CharInfo::CharSet using_param_charsets[MAX_NUM_UNNAMED_PARAMS];
  short num_params;
  char * this_stmt_name;
public:
  Execute(char * stmt_name_, char * argument_, short flag = 0, SqlciEnv * sqlci_env = NULL);
  ~Execute();
  short process(SqlciEnv * sqlci_env);
  static Lng32 storeParams(char * argument_, short &num_params,
                           char * using_params[], CharInfo::CharSet[] = NULL,
                           SqlciEnv * sqlci_env = NULL);
  short getNumParams() const                    { return num_params; }
  char ** getUnnamedParamArray()                 { return using_params; }
  CharInfo::CharSet* getUnnamedParamCharSetArray()  { return using_param_charsets; }
  char* getUnnamedParamValue(short num)         { return using_params[num]; }
  CharInfo::CharSet getUnnamedParamCharSet(short num)         { return using_param_charsets[num]; }
};

class Cursor : public SqlCmd {
public:
  enum CursorOperation{DECLARE, OPEN, FETCH, CLOSE, DEALLOC};
  Cursor(char * cursorName, CursorOperation operation, 
         Int16 internalPrepare, char * argument,
	 NABoolean internalCursor = FALSE);
  ~Cursor();
  short process(SqlciEnv * sqlci_env);
  // QSTUFF
  inline void setHoldable(NABoolean t) { isHoldable_ = t; }
  inline NABoolean isHoldable() { return isHoldable_; }
  // QSTUFF

  short declareC(SqlciEnv * sqlci_env, 
		 char * donemsg, Lng32 &retcode);
  short declareCursorStmt(SqlciEnv *sqlci_env, Lng32 &retcode);
  short declareCursorStmtForRS(SqlciEnv *sqlci_env, Lng32 &retcode);

  short open(SqlciEnv * sqlci_env, char * donemsg, Lng32 &retcode);

  short fetch(SqlciEnv * sqlci_env, NABoolean doDisplayRow,
		char * donemsg, Lng32 &retcode);

  short close(SqlciEnv * sqlci_env, char * donemsg, Lng32 &retcode);

  short dealloc(SqlciEnv * sqlci_env, char * donemsg, Lng32 &retcode);

  void cleanupCursorStmt(SqlciEnv *sqlci_env, CursorStmt *c);

  void setResultSetIndex(Lng32 i) { resultSetIndex_ = i; }
  Lng32 getResultSetIndex() const { return resultSetIndex_; }
private:
  char * cursorName_;
  CursorOperation operation_;
  Int16 internalPrepare_; // if -1, then argument is a SQL statement that
                          // has to be prepared. Otherwise, argument is the
                          // name of a prepared statement.
  // QSTUFF
  // indicates holdable cursor
  NABoolean isHoldable_;
  // QSTUFF

  NABoolean internalCursor_;
  Lng32 resultSetIndex_;
};

// This class is used by the query cache virtual interface to execute
// the commands "display_qc" and "display_qc_entries" which will show
// some important fields of the SQL/MX query cache.
class QueryCacheSt : public SqlCmd {
public:
  QueryCacheSt(short option);
  virtual ~QueryCacheSt() {};
  short process(SqlciEnv * sqlci_env);

private:

     short option_;
};

class StoreExplain : public SqlCmd {
public:
  StoreExplain(char *argument)
    : SqlCmd(STORE_EXPLAIN_TYPE, argument)
    {}
  ~StoreExplain()
    {}
  short process(SqlciEnv * sqlciEnv);
};

class Quiesce : public SqlCmd
{
public:
  Quiesce();
  virtual ~Quiesce();
  short process(SqlciEnv *sqlci_env);
};

#endif
