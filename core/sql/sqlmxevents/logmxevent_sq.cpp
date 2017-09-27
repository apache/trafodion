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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         logmxevent.cpp
 * Description:  Eventlogging functions for SQL
 *
 * Created:      02/05/96
 * Language:     C++
 *
 *
 *
 *
 ****************************************************************************/

#include "NLSConversion.h"
#include "logmxevent.h"
#include "str.h"
#include <stdlib.h>
#include <pthread.h>
#include <rosetta/tal.h>
#include "rosetta/rosgen.h"
#include <limits.h>
#include <stdarg.h>
#include <execinfo.h>
#ifdef _MSC_VER
#undef _MSC_VER
#endif
#include "common/sql.error_event.pb.h"
#include "common/sql.info_event.pb.h"
//#include "wrapper/amqpwrapper.h"
#include "sq_sql_eventids.h"
#include "common/evl_sqlog_eventnum.h"

// #include "sqevlog/evl_sqlog_writer.h"
#include "seabed/fs.h"

// forward declaration
static void check_assert_bug_catcher();
Lng32  sqlToSLSeverity(const char *severity, NABoolean isWarning);

#if 0 /* No longer needed with sqlTextBuf moved to CmpContext */
THREAD_P NAWString* sqlTextBuf = 0;
#endif

NABoolean SQLMXLoggingArea::establishedAMQPConnection_ = FALSE;
pthread_mutex_t SQLMXLoggingArea::loggingMutex_;
bool SQLMXLoggingArea::loggingMutexInitialized_ = false;

void SQLMXLoggingArea::init()
{
  char buffer[80];
  int rc;
  establishedAMQPConnection_= FALSE;
  if (!loggingMutexInitialized_)
  {
    rc = pthread_mutex_init(&loggingMutex_, NULL);
    if (rc == 0)
      loggingMutexInitialized_ = true;
    else
    {
      sprintf(buffer, "SQLMXLoggingArea::init() pthread_mutex_init() rc=%d", rc);
      logSQLMXDebugEvent(buffer, (short)rc);
    }
  }
}

bool SQLMXLoggingArea::lockMutex()
{
  char buffer[80];
  int rc = 0;
  if (loggingMutexInitialized_)
  {
    rc = pthread_mutex_trylock(&loggingMutex_);
    if (rc)
    {
      sprintf(buffer, "SQLMXLoggingArea::lockMutex() pthread_mutex_trylock() rc=%d", rc);
      logSQLMXDebugEvent(buffer, (short)rc, false);
    }
  }
  return rc ? false : true;
}

void SQLMXLoggingArea::unlockMutex()
{
  char buffer[80];
  int rc = 0;
  if (loggingMutexInitialized_)
    rc = pthread_mutex_unlock(&loggingMutex_);
  if (rc)
  {
    sprintf(buffer, "SQLMXLoggingArea::unlockMutex() pthread_mutex_unlock() rc=%d", rc);
    logSQLMXDebugEvent(buffer, (short)rc, false);
  }
}


#if 0 /* No longer needed with sqlTextBuf moved to CmpContext */
// Set the SQL text for later use. If the buffer is not empty, this
// call does nothing.
//
void SQLMXLoggingArea::setSqlText(const NAWString& x)
{
   if ( sqlTextBuf == 0 )
      sqlTextBuf = new NAWString(x);
}
#endif

 SQLMXLoggingArea::~SQLMXLoggingArea() 
{
#ifndef SP_DIS
  closeAMQPConnection();
#endif
  establishedAMQPConnection_ = FALSE;
};

#if 0 /* No longer needed with sqlTextBuf moved to CmpContext */
//
// clear up the SQL text so that next setSqlText() call can have effect.
//
void SQLMXLoggingArea::resetSqlText()
{
   delete sqlTextBuf;
   sqlTextBuf = 0;
}
#endif


 Int32 SQLMXLoggingArea::logSQLMXEventForError( ULng32 sqlcode, 
                                        const char* experienceLevel,
                                        const char* severityLevel,
                                        const char* eventTarget,
					const char *msgTxt,
					const char* sqlId,
					const Lng32 Int0,
					const Lng32 Int1,
					const Lng32 Int2,
					const Lng32 Int3,
					const Lng32 Int4,
				        const char *String0,
					const char * String1,
					const char * String2,
					const char * String3,
					const char * String4,
					const char * serverName,
					const char * connectionName,
					const char * constraintCatalog,
					const char * constraintSchema,
					const char * constraintName,
					const char * triggerCatalog,
					const char * triggerSchema,
					const char *triggerName,
					const char *catalogName,
					const char *schemaName,
					const char *tableName,
					const char *columnName,
					const Int64 currTransid,
					const Lng32 rowNumber,
					const Lng32 platformCode,
					NABoolean isWarning	
					)
{
  Int32 rc = 0;
  // sealog logging of sql error events
  // declare a event stack variable and populate
#ifndef SP_DIS
  bool lockedMutex = lockMutex();

  sql::error_event sql_error_event;
  Int32 qpidNodePort = atoi(getenv("QPID_NODE_PORT"));

  common::event_header * eventHeader = sql_error_event.mutable_header();
  common::info_header * infoHeader = eventHeader->mutable_header();  


  if (!SQLMXLoggingArea::establishedAMQPConnection())
    {
      rc = createAMQPConnection(NULL,-1);
      if (rc)
      {
        if (lockedMutex)
          unlockMutex();
	return rc;
      }
      establishedAMQPConnection_ = TRUE;
    }
 

  char eventidStr[10]="        ";
  Lng32 eventidLen = 0;
  str_sprintf(eventidStr,"10%d%06d",SQEVL_SQL,sqlcode);
  str_strip_blanks(eventidStr,eventidLen);
  Lng32 eventIdVal = (Lng32)str_atoi(eventidStr,eventidLen);
 
  sql_error_event.mutable_header()->set_event_id(eventIdVal);
  sql_error_event.mutable_header()->set_event_severity(sqlToSLSeverity(severityLevel, isWarning));

 
  sql_error_event.set_sqlcode(sqlcode);
  if (sqlId)
    sql_error_event.set_sqlid(sqlId);
  else
    {
      SB_Phandle_Type myphandle;
      XPROCESSHANDLE_GETMINE_(&myphandle);
      char charProcHandle[200];
      char myprocname[30];
      Int32 mycpu,mypin,mynodenumber=0;
      short myproclength = 0;
      XPROCESSHANDLE_DECOMPOSE_(&myphandle, &mycpu, &mypin, &mynodenumber,NULL,100, NULL, myprocname,100, &myproclength); 
      myprocname[myproclength] = '\0';
      str_sprintf(charProcHandle,"%d,%d,%d,%s",mycpu,mypin,mynodenumber,myprocname);
      sql_error_event.set_sqlid(charProcHandle);
    }
  sql_error_event.set_message_text(msgTxt);
  sql_error_event.set_err_experience_level(experienceLevel);
  sql_error_event.set_err_target(eventTarget);
  sql_error_event.set_int0(Int0);
  sql_error_event.set_int1(Int1);
  sql_error_event.set_int2(Int2);
  sql_error_event.set_int3(Int3);
  sql_error_event.set_int4(Int4);

  if (String0)
    sql_error_event.set_string0(String0);
  if (String1)
    sql_error_event.set_string1(String1);
  if (String2)
    sql_error_event.set_string2(String2);
  if (String3)
    sql_error_event.set_string3(String3);
  if (String4)
    sql_error_event.set_string4(String4);
  if (serverName)
    sql_error_event.set_server_name(serverName);
  if (connectionName)
    sql_error_event.set_connection_name(connectionName);
  if (constraintCatalog)
    sql_error_event.set_constraint_catalog(constraintCatalog);
  if (constraintSchema)
    sql_error_event.set_constraint_schema(constraintSchema);
  if (constraintName)
    sql_error_event.set_constraint_name(constraintName);
  if (triggerCatalog)
    sql_error_event.set_trigger_catalog(triggerCatalog);
  if (triggerSchema)
    sql_error_event.set_trigger_schema(triggerSchema);
  if (triggerName)
    sql_error_event.set_trigger_name(triggerName);
  
  if (catalogName)
    sql_error_event.set_catalog_name(catalogName);
  if (schemaName)
    sql_error_event.set_schema_name(schemaName);
  if (tableName)
    sql_error_event.set_table_name(tableName);
  if (columnName)
    sql_error_event.set_column_name(columnName);
  
  sql_error_event.set_current_transid(currTransid);
  sql_error_event.set_row_number(rowNumber);  
  sql_error_event.set_platform_error_code(platformCode);

  rc =  initAMQPInfoHeader(infoHeader, SQEVL_SQL);
  if (rc)
    {
      closeAMQPConnection();
      establishedAMQPConnection_ = FALSE;
      if (lockedMutex)
        unlockMutex();
      return rc;
    }
 
  AMQPRoutingKey routingKey(SP_EVENT, SP_SQLPACKAGE,  SP_INSTANCE, 
                              SP_PUBLIC,  SP_GPBPROTOCOL,  "error_event"); 
  try { 
    rc = sendAMQPMessage(true, sql_error_event.SerializeAsString(), 
                         SP_CONTENT_TYPE_APP, routingKey);
    if (rc) throw 1;
  } catch (...) {
    closeAMQPConnection(); 
    establishedAMQPConnection_ = FALSE;
    if (!rc) rc = SP_SEND_FAILED;
    if (lockedMutex)
       unlockMutex();
    return rc;
  }

  if (lockedMutex)
    unlockMutex();
#else
  rc = 0;
#endif

  return rc;

}


Lng32  sqlToSLSeverity(const char *severity,NABoolean isWarning)
{
  if (isWarning)
    return SQ_LOG_WARNING; 
  if (str_cmp(severity,"CRTCL",str_len(severity)) == 0)
    return SQ_LOG_CRIT;
  else if (str_cmp(severity,"MAJOR",str_len(severity)) == 0)
    return SQ_LOG_ERR;
  else if (str_cmp(severity,"MINOR",str_len(severity)) == 0)
    return SQ_LOG_ERR;
  else if (str_cmp(severity,"INFRM",str_len(severity)) == 0 )
    return SQ_LOG_INFO;
  else
    return -1;
}
void SQLMXLoggingArea::logCompNQCretryEvent(char *stmt)
{
  const char m[]="Statement was compiled as if query plan caching were off: ";
  Int32 mLen = sizeof(m);
  Int32 sLen = str_len(stmt);
  char msg[8192];
  str_cpy_all(msg, m, mLen);
  str_cpy_all(msg+mLen, stmt, MINOF(sLen, 8192-mLen));
  logSQLMXEventForError(SQEV_CMP_NQC_RETRY_OCCURED, "ADVANCED", "INFRM", 
                        "LOGONLY", msg); 
}
void SQLMXLoggingArea::logExecRtInfo(const char *fileName,
                                     ULng32 lineNo,
                                     const char *msg, Lng32 explainSeqNum)
{
  bool lockedMutex = lockMutex();
  short rc = 0;
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters
  sevent.setFileName((char *)fileName);
  sevent.setLineNumber(lineNo);
  sevent.setMessageText((char *)msg);
  sevent.setExplainSeqNum(explainSeqNum);
  // set the event ifd and severity and send the event
  sevent.sendEvent(SQEV_SQL_EXEC_RT_INFO, SQ_LOG_INFO);
  // close the connection.
  sevent.closeConnection();
  if (lockedMutex)
    unlockMutex();
}
static void writeStackTrace(char *s, int bufLen)
{
  const int safetyMargin = 256;
  int len = sprintf(s, "Process Stack Trace:\n");

  // This is a quick and dirty implementation for Linux. It is easy to
  // get the program counters for the stack trace, but is difficult to
  // look up the function name, line, and file number based off of the
  // program counter. For simplicity, this code just calls addr2line to
  // look up the information. This could be changed in the future if an
  // easy to use API becomes available.
  void *bt[20];
  size_t size = backtrace(bt, 20);

  pid_t myPID = getpid();

  // Write each level of the stack except for the top frame and the
  // bottom two frames, which aren't important here.
  Int32 i = 1;
  while (i < size - 2)
  {
    char buffer[128];  // Used for command-line + addr2line output.
    char addrBuf[sizeof(void *)*2 + 4];

    sprintf(buffer, "/usr/bin/addr2line -e /proc/%d/exe -f -C ", myPID);

    Int32 j;

    // Run addr2line on 5 addresses at a time.
    for (j = i; j < i+5 && j < size-2; j++)
    {
      sprintf(addrBuf, " %p", bt[j]);
      strcat(buffer, addrBuf);
    }

    FILE *cmdFP = popen(buffer, "r");
    if (cmdFP == NULL)
    {
      if (len+safetyMargin < bufLen)
        len += sprintf(s, "Error %d while popen() of %s\n", errno, buffer);
      break;
    }
    else
    {
      for (j = i; j < i+5 && j < size-2; j++)
      {
        // Read from the addr2line output
        fgets(buffer, sizeof(buffer), cmdFP);

        // Replace newline with null character
        size_t len = strlen(buffer);
        if (buffer[len-1] == '\n')
          buffer[len-1] = '\0';

        if (len+safetyMargin < bufLen)
          len += sprintf(s, "%p: %s()\n", bt[j], buffer); 
        fgets(buffer, sizeof(buffer), cmdFP);
        if (len+safetyMargin < bufLen)
          len += sprintf(s, "            %s", buffer); 
      }
      fclose(cmdFP);
    }
    i = j;
  }
  sprintf(s, "\n");
}
void SQLMXLoggingArea::logErr97Event(int rc)
{
#if 0
// to be completed, need event id 516 and proper template in sqf/seapilot/
// source/event_templates/sql.info_event.template.
  const int LEN=8192;
  if (rc == 97) {
    char msg[LEN];
    writeStackTrace(msg, LEN);
    logSQLMXEventForError(SQLMX_ERR97_OCCURED, "ADVANCED", "INFRM", 
			  "LOGONLY", msg);
  }
#endif
}
void SQLMXLoggingArea::logSQLMXPredefinedEvent(
  ULng32 eventId,
  SQLMXLoggingArea::Category category)
{
 
  bool lockedMutex = lockMutex();
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters 
  // set the event id and severity and send the event
  sevent.sendEvent(eventId, SQ_LOG_INFO);
  // close the connection.
  // sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
 
}
void SQLMXLoggingArea::logSQLMXDebugEvent( const char *msg, short errorcode, bool lock)
{
  
  bool lockedMutex = lock ? lockMutex() : false;
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters

  sevent.setMessageText((char *)msg);
  sevent.setError1((Lng32)errorcode);
  // set the event id and severity and send the event
  sevent.sendEvent(SQEV_SQL_DEBUG_EVENT, SQ_LOG_DEBUG);
  // close the connection.
  sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
}

// log an ABORT event
void
SQLMXLoggingArea::logSQLMXAbortEvent(const char* file, Int32 line, const char* msg)
{
  bool lockedMutex = lockMutex();
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters
  sevent.setFileName((char *)file);
  sevent.setLineNumber(line);
  sevent.setMessageText((char *)msg);
 
  // set the event id and severity and send the event
  sevent.sendEvent(SQEV_SQL_ABORT, SQ_LOG_ERR);
  // close the connection.
  sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
 
}

// log an ASSERTION FAILURE event
void
SQLMXLoggingArea::logSQLMXAssertionFailureEvent(const char* file, Int32 line, const char* msg, const char* condition, const Lng32* tid)
{
  bool lockedMutex = lockMutex();
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters
  sevent.setFileName((char *)file);
  sevent.setLineNumber(line);
  sevent.setMessageText((char *)msg);
  if (tid)
    sevent.setInt0(*tid);
  if (condition)
    sevent.setString0((char *)condition);
 
  // set the event id and severity and send the event
  sevent.sendEvent(SQEV_SQL_ASSERTION_FAILURE, SQ_LOG_ERR);
  // close the connection.
  sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
  // logSQLMXEvent(SQLMX_ASSERTION_FAILURE, file, line, msg);
}

void SQLMXLoggingArea::logPOSInfoEvent(const char *msg)
{
  bool lockedMutex = lockMutex();
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters
  sevent.setMessageText((char *)msg);
 
  // set the event id and severity and send the event
  sevent.sendEvent(SQEV_SQL_POS_INFO, SQ_LOG_INFO);
  // close the connection.
  sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
       
}
void SQLMXLoggingArea::logPOSErrorEvent(const Lng32 errorCode,
                                        const char *msg1,
                                        const char *msg2,
                                        const char *msg3)
{
  bool lockedMutex = lockMutex();
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters
  sevent.setError1(errorCode);
  sevent.setString0((char *)msg1);
  sevent.setString1((char *)msg2);
  sevent.setString2((char *)msg3);
 
  // set the event id and severity and send the event
  if (errorCode == 1150)
    sevent.sendEvent(SQEV_SQL_POS_ERROR, SQ_LOG_ERR);
  else if (errorCode ==1154)
    sevent.sendEvent(SQEV_SQL_POS_CREATE_ERROR,SQ_LOG_ERR);
  // close the connection.
  sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
	
}


// events that correspond to informational messages from CommonLogger or one of
// its subclasses
void SQLMXLoggingArea::logCommonLoggerInfoEvent(ULng32 eventId,
                                                const char *msg)
{ 
  SqlSealogEvent sevent;
  sevent.openConnection();
  sevent.setExperienceLevel("ADVANCED");
  sevent.setTarget("LOGONLY");
  sevent.setMessageText((char*)msg);
  sevent.sendEvent(eventId, SQ_LOG_INFO);
  sevent.closeConnection(); 
}   
    
// events that correspond to error messages from CommonLogger or one of its
// subclasses
void SQLMXLoggingArea::logCommonLoggerErrorEvent(ULng32 eventId,
                                                 const char *msg)
{ 
  SqlSealogEvent sevent;
  sevent.openConnection();
  sevent.setExperienceLevel("ADVANCED");
  sevent.setTarget("DBADMIN");
  sevent.setMessageText((char*)msg);
  sevent.sendEvent(eventId, SQ_LOG_ERR);
  sevent.closeConnection(); 
}

// events that correspond to fatal error messages from CommonLogger or one of
// its subclasses
void SQLMXLoggingArea::logCommonLoggerFailureEvent(ULng32 eventId,
                                                   const char *msg)
{
  SqlSealogEvent sevent;
  sevent.openConnection();
  sevent.setExperienceLevel("ADVANCED");
  sevent.setTarget("DBADMIN");
  sevent.setMessageText((char*)msg);
  sevent.sendEvent(eventId, SQ_LOG_ERR);
  sevent.closeConnection(); 
}

void SQLMXLoggingArea::logMVQRInfoEvent(const char *msg)
{ 
  logSQLMXEventForError(SQLMX_MVQR_INFO, "ADVANCED", "INFRM", "LOGONLY", msg);
}   
    
// events that correspond to error messages in an MVQR process (qms, qmm, qmp)
void SQLMXLoggingArea::logMVQRErrorEvent(const char *msg)
{ 
  logSQLMXEventForError(SQLMX_MVQR_ERROR, "ADVANCED", "MAJOR", "DBADMIN", msg);
} 

// events that correspond to fatal error messages in an MVQR process (qms, qmm, qmp)
void SQLMXLoggingArea::logMVQRFailureEvent(const char *msg)
{
  logSQLMXEventForError(SQLMX_MVQR_FAILURE, "ADVANCED", "MAJOR", "DBADMIN", msg);
}

void SQLMXLoggingArea::logMVRefreshInfoEvent(const char *msg)
{
  bool lockedMutex = lockMutex();
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters
  
  sevent.setMessageText((char *)msg);
 
  // set the event id and severity and send the event
  sevent.sendEvent(SQEV_MVREFRESH_INFO, SQ_LOG_INFO);
  // close the connection.
  sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
  	
}

void SQLMXLoggingArea::logMVRefreshErrorEvent(const char *msg)
{
  bool lockedMutex = lockMutex();
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters

  sevent.setMessageText((char *)msg);
 
  // set the event id and severity and send the event
  sevent.sendEvent(SQEV_MVREFRESH_ERROR, SQ_LOG_ERR);
  // close the connection.
  sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
       
}


void SQLMXLoggingArea::logCliReclaimSpaceEvent(Lng32 freeSize, Lng32 totalSize,
					       Lng32 totalContexts, Lng32 totalStatements)
{

  bool lockedMutex = lockMutex();
  SqlSealogEvent sevent;
  // Open a  new connection 
  sevent.openConnection();
  // set the required parameters
  sevent.setInt0(freeSize);
  sevent.setInt1(totalSize);
  sevent.setInt2(totalContexts);
  sevent.setInt3(totalStatements);
 
  // set the event id and severity and send the event
  sevent.sendEvent(SQEV_SQL_CLI_RECLAIM_OCCURED, SQ_LOG_INFO);
  // close the connection.
  sevent.closeConnection(); 
  if (lockedMutex)
    unlockMutex();
 
}

Int16 SqlSealogEvent::openConnection()
{
#ifdef SP_DIS
  return 0;
#else
  if (SQLMXLoggingArea::establishedAMQPConnection())
    return 0;
  Int32 qpidNodePort = atoi(getenv("QPID_NODE_PORT"));

  common::event_header * eventHeader = sqlInfoEvent_.mutable_header();
  common::info_header * infoHeader = eventHeader->mutable_header();  

  Int32 rc = createAMQPConnection("127.0.0.1",qpidNodePort);
  if (rc)
    //add trace log 
    return rc;
  SQLMXLoggingArea::establishedAMQPConnection_ = TRUE;
  return rc;
#endif
 
}

//set methods
void SqlSealogEvent::setQueryId(char *queryId)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_query_id(queryId ? queryId:"(not available)");
#endif
}

void SqlSealogEvent::setMessageText(char *messageText)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_message_text(messageText ? messageText: "(not available)");
#endif
}

void SqlSealogEvent::setExperienceLevel(const char *el)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_experience_level(el?el:"ADVANCED");
#endif
}
void SqlSealogEvent::setTarget(const char *target)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_target(target?target:"LOGONLY");
#endif
}
void SqlSealogEvent::setFileName(char *fn)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_file_name(fn?fn:"(not available)");
#endif
}
void SqlSealogEvent::setLineNumber(Lng32 ln)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_line_number((ln>0)?ln:0);
#endif
}
void SqlSealogEvent::setExplainSeqNum(Lng32 esn)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_explain_seq_num(esn);
#endif
}
void SqlSealogEvent::setError1(Lng32 e1)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_error1(e1);
#endif
}
void SqlSealogEvent::setError2(Lng32 e2)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_error2(e2);
#endif
}
void SqlSealogEvent::setError3(Lng32 e3)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_error3(e3);
#endif
}
void SqlSealogEvent::setInt0(Lng32 i0)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_int0(i0);
#endif
}
void SqlSealogEvent::setInt1(Lng32 i1)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_int1(i1);
#endif
}
void SqlSealogEvent::setInt2(Lng32 i2)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_int2(i2);
#endif
}
void SqlSealogEvent::setInt3(Lng32 i3)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_int3(i3);
#endif
}
void SqlSealogEvent::setInt4(Lng32 i4)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_int4(i4);
#endif
}
void SqlSealogEvent::setString0(char *string0)
{ 
#ifndef SP_DIS
  sqlInfoEvent_.set_string0(string0 ? string0 : "");
#endif
}
void SqlSealogEvent::setString1(char *string1)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_string1(string1?string1:"");
#endif
}
void SqlSealogEvent::setString2(char *string2)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_string2(string2 ? string2 : "");
#endif
}
void SqlSealogEvent::setString3(char *string3)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_string3(string3 ? string3 : "");
#endif
}
void SqlSealogEvent::setString4(char *string4)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_string4(string4 ? string4 : "");
#endif
}
void SqlSealogEvent::setInt64_0(Int64 i64_0)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_int64_0(i64_0);
#endif
}
void SqlSealogEvent::setInt64_1(Int64 i64_1)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_int64_1(i64_1);
#endif
}
void SqlSealogEvent::setInt64_2(Int64 i64_2)
{
#ifndef SP_DIS
  sqlInfoEvent_.set_int64_2(i64_2);
#endif
}
  
// Pass in event id (from sq_sql_eventids.h file
// Pass in severity from sealog header file (foll values) : 
// SQ_LOG_CRIT
// SQ_LOG_ALERT
// SQ_LOG_ERR
// SQ_LOG_INFO
// This method sets the event id and severity and serailizes
// any other event tokens into a string form and sends the buffer
// to sealog

Int16 SqlSealogEvent::sendEvent(Int16 eventId, Lng32 slSeverity)
{
  Int32 rc = 0;
#ifndef SP_DIS
  char eventidStr[10]="        ";
  Lng32 eventidLen = 0;
  str_sprintf(eventidStr,"10%d%06d",SQEVL_SQL,eventId);
  str_strip_blanks(eventidStr,eventidLen);
  Lng32 eventIdVal = (Lng32)str_atoi(eventidStr,eventidLen);
   common::event_header * eventHeader = sqlInfoEvent_.mutable_header();
  common::info_header * infoHeader = eventHeader->mutable_header();  

   rc = initAMQPInfoHeader(infoHeader, SQEVL_SQL);
   if (rc)
     //add trace log
     return rc;
  sqlInfoEvent_.mutable_header()->set_event_id(eventIdVal);
  sqlInfoEvent_.mutable_header()->set_event_severity(slSeverity);
  setExperienceLevel("ADVANCED");
  setTarget("LOGONLY");

  AMQPRoutingKey routingKey(SP_EVENT, SP_SQLPACKAGE,  SP_INSTANCE, 
                              SP_PUBLIC,  SP_GPBPROTOCOL,  "info_event"); 
  try {
    rc = sendAMQPMessage(true, sqlInfoEvent_.SerializeAsString(), SP_CONTENT_TYPE_APP, routingKey);
  } catch(...) {
    rc = -1;
  }
#endif
  return rc;
}

Int16 SqlSealogEvent::closeConnection()
{
#ifdef SP_DIS
  return 0;
#else
 Int32 rc = closeAMQPConnection();
 SQLMXLoggingArea::establishedAMQPConnection_ = FALSE;
  if (rc)
    // add trace log
    return rc;
  return rc;
#endif
}






void SQLMXLoggingArea::logSortDiskInfo(char *diskname, short percentfree, short diskerror)
{
  // 
  //TBD or rewrite needed 
  //** use event id SQEV_SQL_SRT_INFO **
}

static void check_assert_bug_catcher()
{

}

void SQLMXLoggingArea::logUtilErrorsEvent (const char *utilName,
                                         const Int32 numOfErrors,
                                         const Lng32  errorCode,
                                         const char *msg1,
                                         const char *msg2,
                                         const char *msg3,
                                         const char *msg4,
                                         const char *msg5)
{
/* TBD
  tokenClass operation_tok(
    STRING, 
    TKN_UTIL_NAME,
    (void*)utilName
    );
  
  tokenClass errorCode_tok(
    LONGINT32, 
    TKN_PM_SQLCODE,
    (void*)&errorCode
    );

  // There must be atleast one error message(msg1) to emit.
  tokenClass msg1_tok(STRING, 
		      TKN_PM_ERRTEXT,
		      (void*)msg1
		      );
  tokenClass msg2_tok(STRING, 
		      TKN_PM_ERRTEXT,
		      (void*)msg2
		      );
  
  tokenClass msg3_tok(STRING, 
		      TKN_PM_ERRTEXT,
		      (void*)msg3
		      );

  tokenClass msg4_tok(STRING, 
		      TKN_PM_ERRTEXT,
		      (void*)msg4
		      );

  tokenClass msg5_tok(STRING, 
		      TKN_PM_ERRTEXT,
		      (void*)msg5
		      );
  
  switch (numOfErrors)
    {
    case (1):
      logAnMxEvent(SQLMX_UTIL_OP_ERROR,
		   SQLMXLoggingArea::SoftwareFailure,
		   &operation_tok, 
		   &errorCode_tok,
		   &msg1_tok,
		   0
		   );

      break;
      
    case (2):
      logAnMxEvent(SQLMX_UTIL_OP_ERROR,
		   SQLMXLoggingArea::SoftwareFailure,
		   &operation_tok, 
		   &errorCode_tok,
		   &msg1_tok,
		   &msg2_tok,
		   0
		   );
      break;
      
    case (3):
      logAnMxEvent(SQLMX_UTIL_OP_ERROR,
		   SQLMXLoggingArea::SoftwareFailure,
		   &operation_tok, 
		   &errorCode_tok,
		   &msg1_tok,
		   &msg2_tok,
		   &msg3_tok,
		   0
		   );
      
      break;
      
    case (4):
      logAnMxEvent(SQLMX_UTIL_OP_ERROR,
		   SQLMXLoggingArea::SoftwareFailure,
		   &operation_tok, 
		   &errorCode_tok,
		   &msg1_tok,
		   &msg2_tok,
		   &msg3_tok,
		   &msg4_tok,
		   0
		   );
      
      break;

    case (5):
      logAnMxEvent(SQLMX_UTIL_OP_ERROR,
		   SQLMXLoggingArea::SoftwareFailure,
		   &operation_tok, 
		   &errorCode_tok,
		   &msg1_tok,
		   &msg2_tok,
		   &msg3_tok,
		   &msg4_tok,
		   &msg5_tok,
		   0
		   );
      break;

    default:
      // should not get here.
      break;

    }
end TBD */
}

void SQLMXLoggingArea::logUtilOperationStatusEvent(ULng32 eventId,
                                                   const char *utilName,
						   const char *objType,
						   const char *objAnsiName,
                                                   const char *utilStatus)
{
/* TBD
  tokenClass utilName_tok(
    STRING, 
    TKN_UTIL_NAME,
    (void*)utilName
    );
  
  tokenClass objType_tok(
    STRING, 
    TKN_PM_OBJ_TYPE,
    (void*)objType
    );

  tokenClass objAnsiName_tok(
    STRING, 
    TKN_PM_ANSI_NAME,
    (void*)objAnsiName
    );

  tokenClass utilStatus_tok(
    STRING, 
    TKN_UTIL_STATUS,
    (void*)utilStatus
    );

  logAnMxEvent(eventId,
	       SQLMXLoggingArea::Informational,
	       &utilName_tok, 
               &objType_tok,
	       &objAnsiName_tok, 
               &utilStatus_tok,
	       0
	       );
end TBD */
}

void SQLMXLoggingArea::logPMEvent(ULng32 eventId)
{
// TBD
}

// These aren't currently used
#if 0
void SQLMXLoggingArea::logPMOperationStatusEvent(ULng32 eventId,
						const char *operation, 
						  const char *objType,
						  const char *objAnsiName)
{
   
}
 
void SQLMXLoggingArea::logPMDataCopyStatusEvent(ULng32 eventId, 
						TInt64 elapsedTime,
						TInt64 totalCopyCount,
						const char *unitName)
{

}

void SQLMXLoggingArea::logPMEventWithGuardianName(ULng32 eventId,
						  const char *location)
{

}

void SQLMXLoggingArea::logPMEventWithDumpFileName(ULng32 eventId,
						  const char *location)
{

}

void SQLMXLoggingArea::logPMEventWithInterval(ULng32 eventId,
					      TInt64 interval)
{


}


void SQLMXLoggingArea::logPMErrorsEvent (const char *operation,
					 const Int32 numOfErrors,
					 const Lng32  errorCode,
					 const char *msg1,
					 const char *msg2,
					 const char *msg3,
					 const char *msg4,
					 const char *msg5)
{

}

void SQLMXLoggingArea::logPMAudInitEvent(ULng32 eventId,
					 TInt64 interval)
{
 
}

void SQLMXLoggingArea::logPMAudStartEvent(ULng32 eventId,
					  const short audNum)
{
  
}

void SQLMXLoggingArea::logPMAudDoneEvent(ULng32 eventId,
					 const short audNum,
					 TInt64 interval)
{
  

}
#endif // #if 0
