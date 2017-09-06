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
#ifndef SQLCIENV_H
#define SQLCIENV_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciEnv.h
 * RCS:          $Id: SqlciEnv.h,v 1.17 1998/09/07 21:50:03  Exp $
 * Description:  
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1998/09/07 21:50:03 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"
#include "SqlCliDllDefines.h"


#include <limits.h>
#include <iostream>
#include <stdio.h>
#include "SqlciList_templ.h"
#include "SqlciStmts.h"
#include "ComASSERT.h"
#include "ComDiags.h"
#include "Define.h"
#include "Param.h"
#include "Prepare.h"
#include "CSInterface.h"

// forward references
class SqlciStats;
class SqlciRWEnv;
class SqlciRWInterfaceExecutor;
class SqlciCSEnv;
class SqlciCSInterfaceExecutor;
class ComSchemaName;
class ComAnsiNamePart;

#pragma nowarn(1506)   // warning elimination 
class Logfile {

private:

  char * name;
  FILE * logfile_stream;
  ULng32 flags_;
  
  enum Flags
  {
    VERBOSE_ = 0x0001,
    NO_LOG = 0x0002,
    NO_DISPLAY = 0x0004
  };

public:

  enum open_mode {CLEAR_, APPEND_};
  Logfile();
  ~Logfile();
  void  Open(char * name_, open_mode mode);
  void  Reopen();
  void  Close();
  void  Close_(); // close withouth delete file name
  short Write(const char *, Lng32);
  short WriteAll(const char *, Lng32);
  short WriteAll(const char *, Lng32, Int32);
  short WriteAll(const char *);
  short WriteAll(const WCHAR *, Lng32);
  short WriteAllWithoutEOL(const char *);
  short IsOpen();
  char *Logname() 		{ return name; }
  FILE* GetLogfile() 		{ return logfile_stream; }
  NABoolean isVerbose() { return flags_ & VERBOSE_; };
  void setVerbose(NABoolean v)
  { (v ? flags_ |= VERBOSE_ : flags_ &= ~VERBOSE_);};


  NABoolean noLog() { return flags_ & NO_LOG; };
  void setNoLog(NABoolean v)
  { (v ? flags_ |= NO_LOG : flags_ &= ~NO_LOG);};

  NABoolean noDisplay() { return flags_ & NO_DISPLAY; };
  void setNoDisplay(NABoolean v);
};
#pragma warn(1506)  // warning elimination 


// SqlciRWEnv class.  
class SqlciRWEnv {

private:			
	NABoolean       rwSelect_;
	void *		rwEnv_; // Pointer to ReportWriter's env.  This is a handle to their env.
	SqlciRWInterfaceExecutor * rwExe_;
	NAHeap		*heap_;

public:
	SqlciRWEnv();
	~SqlciRWEnv();
	NABoolean           isSelectInProgress()		{ return rwSelect_; }
	void                setSelectInProgress(NABoolean val)  { rwSelect_ = val; }
	void*               &rwEnv()				{ return rwEnv_; }
	SqlciRWInterfaceExecutor *rwExe()			{ return rwExe_; }   
	NAHeap                   *rwHeap()			{ return heap_;  }
};


// SqlciCSEnv class.
class SqlciCSEnv {

private:
  void * csEnv_; // Pointer to their MACL environment.  This is a handle to their env.
  SqlciCSInterfaceExecutor *csExe_;

public:
  SqlciCSEnv();
  ~SqlciCSEnv();
  void* &csEnv()                      { return csEnv_; }
  SqlciCSInterfaceExecutor *csExe()   { return csExe_; }

};


class SqlciEnv
{
private:
  short               ole_server;	    // -1 if being used via OLE on NT.
  short               eol_seen_on_input;    // 0 if multiple stmts on input line
  short		      prev_err_flush_input; // -1 when a previous statement
  					    // on a multiple statements request
                                            // failed. Statements following
                                            // need to be flushed.
  Int16               interactive_session;  // -1 if input from terminal device
  short		      obey_file;	    // -1 if in an obey file
  Logfile             *logfile;
  SqlciStmts          *sqlci_stmts;
  SqlciList<PrepStmt> *prepared_stmts;
  SqlciList<Param>    *param_list;
  SqlciList<Param>    *pattern_list;
  SqlciList<Define>   *define_list;
  SqlciList<Envvar>   *envvar_list;
  SqlciList<CursorStmt> *cursorList_;
  SqlciStats          *sqlci_stats;
  SqlciRWEnv          *report_env; //Pointer to ReportWriter's Environement.
  SqlciCSEnv          *cs_env;  // Pointer to MACL's Environment.
  ULng32	      list_count;

  CharInfo::CharSet terminal_charset_;
  CharInfo::CharSet iso_mapping_charset_;
  CharInfo::CharSet default_charset_;
  NABoolean         infer_charset_;

  Lng32		       specialError_;	// special sqlCode in HandleCLIError
  typedef void (*SpecialHandler)(SqlciEnv *, Lng32, const char *, const char *);
  SpecialHandler       specialHandler_;
  ComSchemaName *      defaultCatAndSch_;

  NABoolean            showShape_; // if TRUE, show CONTROL SHAPE before
                                   // compiling the query

  NABoolean            logCommands_; // if TRUE, log commands only.
  NABoolean            constructorFlag_; // Have a flag to let the constructor
                                         // know whether to call MACL and RW constructors.
  NABoolean            deallocateStmt_; // for deallocatin statement in case of a Break key is hit.

  char *               defaultCatalog_;
  char *               defaultSchema_;
  unsigned char        defaultSubvol_[40];

  // see DML::process for details about this field.
  ULng32        lastDmlStmtStatsType_;

  // last statement that was executed.
  // Used to retrieve stats. See SqlciStats.cpp for details.
  PrepStmt *           lastExecutedStmt_;

  // stats stmt to retrieve pertable or accumulated stats.
  // Prepared once at sqlci startup time.
  PrepStmt *           statsStmt_;

  // last prepared stmt.
  SQLSTMT_ID * lastAllocatedStmt_;

  NABoolean doneWithPrologue_;
  NABoolean noBanner_;

  // could be DDL, DML, CONTROL, ALL
  char * prepareOnly_;
  // could be DDL, DML, CONTROL, ALL
  char * executeOnly_;

  NAString userNameFromCommandLine_;

public:
  enum {MAX_LISTCOUNT = UINT_MAX };
  enum {MAX_FRAGMENT_LEN_OVERFLOW = 900};
  enum ModeType { SQL_, REPORT_, MXCS_, DISPLAY_ }; // Modes in which MXCI can exist.

  ModeType mode;

   // Add a new flag to SqlciEnv constructor to handle calls to MACL and RW constructors.
  SqlciEnv(short serv_type = 0, NABoolean macl_rw_flag = TRUE); //If serv_type = -1, then we are a OLE server
  ~SqlciEnv();
  
  short                isOleServer()            { return ole_server; }
  short                eolSeenOnInput()   	{ return eol_seen_on_input; }
  short		       prevErrFlushInput()      { return prev_err_flush_input; }
  short                inObeyFile()       	{ return obey_file; }
  short                isInteractiveSession()	{ return interactive_session; }
  short                isInteractiveNow()	{ return interactive_session &&
						  !obey_file;
						}
  void		       setEol(short i)    	{ eol_seen_on_input = i; }
  void		       setObey(short i)   	{ obey_file = i; }
  void		       setPrevErrFlushInput()   { prev_err_flush_input = -1; }
  void                 resetPrevErrFlushInput() { prev_err_flush_input = 0; }
  void                 setDeallocateStmt()      { deallocateStmt_ = TRUE; }
  void                 resetDeallocateStmt()    { deallocateStmt_ = FALSE; }
  void                 setLastAllcatedStmt(SQLSTMT_ID *stmt)    { lastAllocatedStmt_ = stmt; };
  SQLSTMT_ID           *getLastAllocatedStmt(){return lastAllocatedStmt_; };
  NABoolean            getDeallocateStmt()      { return deallocateStmt_; }
  CharInfo::CharSet    getTerminalCharset()const{ return terminal_charset_; }
  void                 setTerminalCharset(CharInfo::CharSet cs)
                                                { terminal_charset_ = cs; }
  CharInfo::CharSet    getIsoMappingCharset() const
                                                { return iso_mapping_charset_; }
  void                 setIsoMappingCharset(CharInfo::CharSet cs)
                                                { iso_mapping_charset_ = cs; }
  CharInfo::CharSet    retrieveIsoMappingCharsetViaShowControlDefault();
  CharInfo::CharSet    getDefaultCharset()const { return default_charset_; }
  void                 setDefaultCharset(CharInfo::CharSet cs)
                                                { default_charset_ = cs; }
  CharInfo::CharSet    retrieveDefaultCharsetViaShowControlDefault();
  NABoolean            getInferCharset() const  { return infer_charset_; }
  void                 setInferCharset(NABoolean setting)
                                                { infer_charset_ = setting; }
  NABoolean           retrieveInferCharsetViaShowControlDefault();
  Logfile             *get_logfile()      	{ return logfile; }
  SqlciStmts          *getSqlciStmts()    	{ return sqlci_stmts; }
  SqlciList<PrepStmt> *get_prep_stmts()   	{ return prepared_stmts; }
  SqlciList<Param>    *get_paramlist()    	{ return param_list; }
  SqlciList<Param>    *get_patternlist()        { return pattern_list; }
  SqlciList<Define>   *get_definelist()   	{ return define_list; }
  SqlciList<Envvar>   *get_envvarlist()   	{ return envvar_list; }
  SqlciList<CursorStmt> *getCursorList()        { return cursorList_; }
  SqlciStats          *getStats()         	{ return sqlci_stats; }
  SqlciRWEnv          *sqlciRWEnv()	        { return report_env; }
  SqlciCSEnv          *sqlciCSEnv()             { return cs_env; }
  NABoolean           isReportWriterMode()      { if (mode == REPORT_) return TRUE; else return FALSE;};
  void                setMode(ModeType mode_)   { mode = mode_; }
  NABoolean           isMXCSMode() { if (mode == MXCS_) return TRUE; else return FALSE;};
  void                showMode(ModeType mode_) ;
  ModeType            getMode()                 { return mode;}
  void                setListCount(ULng32 num = MAX_LISTCOUNT) { list_count = num; }
  ULng32	      getListCount()            { return list_count; }
  Lng32		      specialError()		{ return specialError_; }
  SpecialHandler      specialHandler()		{ return specialHandler_; }
  void		      resetSpecialError()	{ setSpecialError(0, NULL); }
  void		      setSpecialError(Lng32 err, SpecialHandler func)
  {
    // * If err is 0, special error handling is disabled
    // * If err is -1, console error messages are suppressed
    // * Otherwise err is a sqlcode and func is a function to be called
    //   when that sqlcode is encountered

    // If err is not 0 or -1, make sure func is valid
    ComASSERT((err == 0) || (err == -1) || func);

    specialError_   = err;
    specialHandler_ = func;
  }
  void                getDefaultCatAndSch (ComAnsiNamePart & defaultCat, ComAnsiNamePart & defaultSch);
  ComSchemaName &     defaultCatAndSch (void) { return *defaultCatAndSch_; };

  void updateDefaultCatAndSch();

  NABoolean doneWithPrologue() { return doneWithPrologue_; }
  void setDoneWithPrologue(NABoolean dwp)
  {
    doneWithPrologue_ = dwp;
  }

  NABoolean noBanner() { return noBanner_; }
  void setNoBanner(NABoolean nb)
  {
    noBanner_ = nb;
  }
  
  ComDiagsArea	      &diagsArea();
    
  void run();
  void runWithInputString(char * input_string);
  void run(char * in_filename, char * input_string = NULL);
 
  void autoCommit();
  void readonlyCursors();
  void pertableStatistics();
  void generateExplain();
  void datatypeSupport();
  void sqlmxRegress();

  void welcomeMessage();
  void displayDiagnostics();

  Int32 executeCommands(InputStmt *& input_stmt);

  short statusTransaction(Int64 * transid = 0);

  NABoolean &showShape() { return showShape_; };

  NABoolean &logCommands() { return logCommands_; };

  char  *     &defaultCatalog() { return defaultCatalog_; };
  char  *     &defaultSchema() { return defaultSchema_; };
  unsigned char  *     defaultSubvol() { return defaultSubvol_; };

  ULng32 &lastDmlStmtStatsType() { return lastDmlStmtStatsType_; };
  PrepStmt* &lastExecutedStmt() { return lastExecutedStmt_; };
  PrepStmt* &statsStmt() { return statsStmt_; };

  char * getPrepareOnly() { return prepareOnly_;}
  char * getExecuteOnly() { return executeOnly_;}

  void setPrepareOnly(char * po);
  void setExecuteOnly(char * eo);

  // Get/set the command-line user name
  const NAString &getUserNameFromCommandLine()
  { return userNameFromCommandLine_; }
  void setUserNameFromCommandLine(const char *s);

  // Retrieve database user information from CLI
  Int32 getExternalUserName(NAString &username);
  Int32 getDatabaseUserID(Int32 &uid);
  Int32 getDatabaseUserName(NAString &username);

  Int32 getAuthState(bool &authenticationEnabled,
                     bool &authorizationEnabled,
                     bool &authorizationReady,
                     bool &auditingEnabled);

  // Interact with CLI to establish user identity. For Linux only.
  void setUserIdentityInCLI();

};

//BOOL _stdcall ControlSignalHandler(DWORD dwCtrlType);
BOOL WINAPI CtrlHandler(DWORD dwCtrlType);


#endif
