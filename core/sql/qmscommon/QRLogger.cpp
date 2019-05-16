// **********************************************************************
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
// **********************************************************************

#include <log4cxx/fileappender.h>
#include <log4cxx/rollingfileappender.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/level.h>

#include <QRLogger.h>

#include "ComDiags.h"
#include "logmxevent.h"
#include "NLSConversion.h"
#include "Globals.h"

#include "PortProcessCalls.h"
#include "seabed/ms.h"
#include "seabed/fserr.h"

BOOL gv_QRLoggerInitialized_ = FALSE;
QRLogger *gv_QRLoggerInstance_ = NULL;

using namespace log4cxx;
using namespace log4cxx::helpers;

// SQL categories
std::string CAT_SQL                           = "SQL";
std::string CAT_SQL_EXE                       = "SQL.EXE";
std::string CAT_SQL_COMP                      = "SQL.COMP";
std::string CAT_SQL_ESP                       = "SQL.ESP";
std::string CAT_SQL_LOB                       = "SQL.LOB";
std::string CAT_SQL_SSMP                      = "SQL.SSMP";
std::string CAT_SQL_SSCP                      = "SQL.SSCP";
std::string CAT_SQL_UDR                       = "SQL.UDR";
std::string CAT_SQL_PRIVMGR                   = "SQL.PRIVMGR";
std::string CAT_SQL_USTAT                     = "SQL.USTAT";
// hdfs
std::string CAT_SQL_HDFS_JNI_TOP              =  "SQL.HDFS.JniTop";
std::string CAT_SQL_HDFS_SEQ_FILE_READER      =  "SQL.HDFS.SeqFileReader";
std::string CAT_SQL_HDFS_SEQ_FILE_WRITER      =  "SQL.HDFS.SeqFileWriter";
std::string CAT_SQL_HDFS_ORC_FILE_READER      =  "SQL.HDFS.OrcFileReader";
std::string CAT_SQL_HBASE                     =  "SQL.HBase";
std::string CAT_SQL_HDFS                      =  "SQL.HDFS";

// these categories are currently not used 
std::string CAT_SQL_QMP                       = "SQL.Qmp";
std::string CAT_SQL_QMM                       = "SQL.Qmm";
std::string CAT_SQL_COMP_QR_DESC_GEN          = "SQL.COMP"; // ".DescGen";
std::string CAT_SQL_COMP_QR_HANDLER           = "SQL.COMP"; // ".QRHandler";
std::string CAT_SQL_COMP_QR_COMMON            = "SQL.COMP"; // ".QRCommon";
std::string CAT_SQL_COMP_QR_IPC               = "SQL.COMP"; // ".QRCommon.IPC";
std::string CAT_SQL_COMP_MV_REFRESH           = "SQL.COMP"; // ".MV.REFRESH";
std::string CAT_SQL_COMP_MVCAND               = "SQL.COMP"; // ".MVCandidates";
std::string CAT_SQL_MEMORY                    = "SQL.COMP"; // ".Memory";
std::string CAT_SQL_COMP_RANGE                = "SQL.COMP"; // ".Range";
std::string CAT_QR_TRACER                     = "QRCommon.Tracer";
std::string CAT_SQL_QMS                       = "SQL.Qms";
std::string CAT_SQL_QMS_MAIN                  = "SQL.Qms.Main";
std::string CAT_SQL_QMS_INIT                  = "SQL.Qms.Init";
std::string CAT_SQL_MVMEMO_JOINGRAPH          = "SQL.Qms.MvmemoJoingraph";
std::string CAT_SQL_MVMEMO_STATS              = "SQL.Qms.MvmemoStats";
std::string CAT_SQL_QMS_GRP_LATTCE_INDX       = "SQL.Qms.LatticeIndex";
std::string CAT_SQL_QMS_MATCHTST_MVDETAILS    = "SQL.Qms.MatchTest";
std::string CAT_SQL_QMS_XML                   = "SQL.Qms.XML";
std::string CAT_SQL_COMP_XML                  = "SQL.COMP"; // ".XML";

// **************************************************************************
// **************************************************************************
QRLogger::QRLogger()
  : CommonLogger()
   ,module_(QRL_NONE)
   ,processInfo_("")
{
}

// **************************************************************************
// **************************************************************************
QRLogger& QRLogger::instance()
{
  if (gv_QRLoggerInstance_ == NULL)
     gv_QRLoggerInstance_ = new QRLogger();
  return *gv_QRLoggerInstance_;
}


// **************************************************************************
// construct a string with the node number and PIN of the ancestor 
// executor process and the string ".log". This will be used as the suffix
// of the log name. So if the log name specified by the user in the 
// configuration file is "sql_events", the actual log name will be
// something like "sql_events_0_12345.log"
// **************************************************************************
void getMyTopAncestor(char ancsNidPid[])
{
  short result = 0;

  NAProcessHandle myPhandle;
  myPhandle.getmine();
  myPhandle.decompose();
  char processName[MS_MON_MAX_PROCESS_NAME + 1];
  char ppName[MS_MON_MAX_PROCESS_NAME + 1];

  memset(processName, 0, sizeof(processName));
  memcpy(processName, myPhandle.getPhandleString(), myPhandle.getPhandleStringLen());

  memset(ppName, 0, sizeof(ppName));

  MS_Mon_Process_Info_Type procInfo;
  Int32 rc = 0;

  Int32 ancsNid = 0;
  Int32 ancsPid = 0;
  do
  {
     rc = msg_mon_get_process_info_detail(processName, &procInfo);
     ancsNid = procInfo.nid;
     ancsPid = procInfo.pid;
     strcpy(ppName, procInfo.parent_name);
     strcpy(processName, ppName);
  } while ((rc == XZFIL_ERR_OK) && (procInfo.parent_nid != -1) && (procInfo.parent_pid != -1));

  snprintf (ancsNidPid, 5+2*sizeof(Int32), "_%d_%d.log", ancsNid, ancsPid);
}
// **************************************************************************
// construct a string with the node number of the 
//  process and the string ".log". This will be used as the suffix
// of the log name forthis process . So if the log name specified by the 
// user in the 
// configuration file is "sql_events", the actual log name will be
// something like "sql_events_<nid>.log"
// **************************************************************************
void getMyNidSuffix(char stringNidSuffix[])
{
  short result = 0;

  NAProcessHandle myPhandle;
  myPhandle.getmine();
  myPhandle.decompose();
  
  Int32 myNid = myPhandle.getCpu();

  snprintf (stringNidSuffix, 5+sizeof(Int32), "_%d.log", myNid);
}
// **************************************************************************
// Call the superclass to configure log4cxx from the config file.
// If the configuration file is not found, perform default initialization:
// Create an appender, layout, and categories.
// Attaches layout to the appender and appender to categories.
// **************************************************************************
NABoolean QRLogger::initLog4cxx(const char* configFileName)
{
  NAString logFileName;

  if (gv_QRLoggerInitialized_)
     return TRUE;
 
  // get the log directory
  logFileName = "";

  // gets the top ancestor process name that will be used to name the file appender log
  char logFileSuffix [100]="";
  static bool singleSqlLogFile = (getenv("TRAF_MULTIPLE_SQL_LOG_FILE") == NULL);
  switch (module_)
  {
    case QRL_NONE:
    case QRL_MXCMP:
    case QRL_ESP:
    case QRL_MXEXE:
    case QRL_UDR:
      if (singleSqlLogFile) 
         getMyNidSuffix(logFileSuffix);
      else 
         getMyTopAncestor(logFileSuffix);
      break;
    case QRL_LOB:
      getMyNidSuffix(logFileSuffix);
      break; 
    case QRL_SSMP:
    case QRL_SSCP:
      getMyNidSuffix(logFileSuffix);
      break;
    default:
      break;
  }

  
  if (CommonLogger::initLog4cxx(configFileName, logFileSuffix))
  {
    introduceSelf();
    gv_QRLoggerInitialized_ = TRUE;
    return TRUE;
  }
  return FALSE;
}


std::string &QRLogger::getMyDefaultCat()
{
  switch (module_)
    {
    case QRL_MXCMP:
      return CAT_SQL_COMP;
      break;
    case QRL_MXEXE:
      return CAT_SQL_EXE;
      break;
    case QRL_ESP:
      return CAT_SQL_ESP;
      break;
    case QRL_LOB:
      return CAT_SQL_LOB;
      break;
    case QRL_SSMP:
      return CAT_SQL_SSMP;
      break;
    case QRL_SSCP:
      return CAT_SQL_SSCP;
      break;
    case QRL_UDR:
      return CAT_SQL_UDR;
      break;
    default:
      return CAT_SQL;   
    }

}

const char*  QRLogger::getMyProcessInfo()
{
  char procInfo[300];
  SB_Phandle_Type myphandle;
  XPROCESSHANDLE_GETMINE_(&myphandle);
  char charProcHandle[200];
  char myprocname[30];
  Int32 mycpu,mypin,mynodenumber=0;
  short myproclength = 0;
  XPROCESSHANDLE_DECOMPOSE_(&myphandle, &mycpu, &mypin, &mynodenumber,NULL,100, NULL, myprocname,100, &myproclength);
  myprocname[myproclength] = '\0';

  snprintf(procInfo, 300, "Node Number: %d, CPU: %d, PIN: %d, Process Name: %s", mycpu,mycpu, mypin, myprocname);

  processInfo_ = procInfo;
  return processInfo_.data();
}

void QRLogger::introduceSelf ()
{
  char msg[300];
  std::string &myCat = getMyDefaultCat(); 
  // save previous default priority
  log4cxx::LevelPtr defaultPriority = log4cxx::Logger::getLogger(myCat)->getLevel();
  
  NAString procInfo = getMyProcessInfo();

  // change default priority to INFO to be able to printout some informational messages
  log4cxx::LoggerPtr myLogger(log4cxx::Logger::getLogger(myCat));
  myLogger->setLevel(log4cxx::Level::getInfo());
 
  switch (module_)
    {
    case QRL_NONE:
    case QRL_MXCMP:
      snprintf (msg, 200, "%s,,, A compiler process is launched.", procInfo.data());
      break;
 
    case QRL_ESP:
      snprintf (msg, 300, "%s,,, An ESP process is launched.", procInfo.data());
      break;
 
    case QRL_MXEXE:
      snprintf (msg, 300, "%s,,, An executor process is launched.", procInfo.data());
      break;
    case QRL_LOB:
      snprintf (msg, 300, "%s,,, A lobserver  process is launched.", procInfo.data());
      break;
    case QRL_SSMP:
      snprintf (msg, 300, "%s,,, An ssmp  process is launched.", procInfo.data());
      break;
    case QRL_SSCP:
      snprintf (msg, 300, "%s,,, An sscp  process is launched.", procInfo.data());
      break;
   case QRL_UDR:
      snprintf (msg, 300, "%s,,, A udrserver  process is launched.", procInfo.data());
      break;
    }

   LOG4CXX_INFO(myLogger,msg);

   // restore default priority
   myLogger->setLevel(defaultPriority);
}


// **************************************************************************
// Log an exception-type error message:
// First log "Error thrown at line <line> in file <filename>:"
// Then log the actual error message, which is passed with a variable 
// number of parameters. The error message is epected to be a single line 
// of text not exceeding 2K.
// This error message can be logged as either ERROR or FATAL type.
// Both lines of text are sent to the log file
// MVQR failures that prevent only rewrite, and not execution of the original
// form of the query, cause an informational rather than an error notification
// to be entered in the system log.
// **************************************************************************
void QRLogger::logError(const char* file, 
                            Int32       line, 
                            std::string &cat,
                            logLevel    level,
                            const char* logMsgTemplate...)
{
  #define BUFFER_SIZE 2048

  char buffer[BUFFER_SIZE]; // This method expects a single line to text.
  NAString logData = QRLogger::instance().getMyProcessInfo();

  if (level == LL_MVQR_FAIL)
    snprintf(buffer, sizeof(buffer), "%s,,, Attempted MVQR operation (rewrite of query or publication of MV) failed at line %d in file %s:", logData.data(), line, file);
  else
    snprintf(buffer, sizeof(buffer), "%s,,, Error thrown at line %d in file %s:", logData.data(), line, file);

  va_list args;
  va_start(args, logMsgTemplate);

  if (level == LL_FATAL)
  {
    LOG4CXX_FATAL(log4cxx::Logger::getLogger(cat), buffer);
  }
  else
  {
    LOG4CXX_ERROR(log4cxx::Logger::getLogger(cat), buffer);
  }

  // format actual error message
  vsnprintf(buffer, BUFFER_SIZE, logMsgTemplate, args);

  NAString logData2 = QRLogger::instance().getMyProcessInfo();
  logData2 += ",,, ";
  logData2 += buffer;

  // log actual error msg to logfile.
  if (level == LL_FATAL)
  {
    LOG4CXX_FATAL(log4cxx::Logger::getLogger(cat), logData2.data());
  }
  else
  {
    LOG4CXX_ERROR(log4cxx::Logger::getLogger(cat), logData2.data());
  }

  va_end(args);
}

// This temporary function is used for logging QVP messages only. More extensive
// changes to the CommonLogger hierarchy will be made when the QI integration
// stream is merged into the mainline. For now, we avoid making changes that
// affect MVQR to minimize merge difficulties.
void QRLogger::logQVP(ULng32 eventId,
                          std::string &cat,
                          logLevel    level,
                          const char* logMsgTemplate...)
{
  // Don't do anything if this message will be ignored anyway.
  log4cxx::LoggerPtr myLogger = log4cxx::Logger::getLogger(cat);
  if ( myLogger ) 
  {
    log4cxx::LevelPtr myLevel = myLogger->getLevel();
    log4cxx::LevelPtr paramLevel = log4cxx::Level::toLevel(level);
    if ( myLevel && paramLevel ) 
    {
        if ( myLevel == log4cxx::Level::getOff() )
          return;

        int_32 configuredLevel = myLevel->toInt();
        int_32 requestedLevel = paramLevel->toInt();

        // If the configured logging level is greater (more restrictive) than
        // the requested level, don't log.  
        if ( configuredLevel > requestedLevel)
          return;
    }
  }else return;

  va_list args;
  va_start(args, logMsgTemplate);

  char* buffer = buildMsgBuffer(cat, level, logMsgTemplate, args);
  log1(cat, level, buffer, eventId);

  va_end(args);
}


void QRLogger::logDiags(ComDiagsArea* diagsArea, std::string &cat)
{
  const NAWchar* diagMsg;
  NAString* diagStr;
  Int32 i;
  ComCondition* condition;
  
  log4cxx::LoggerPtr myLogger = log4cxx::Logger::getLogger(cat);
  if ( myLogger ) 
  {
    log4cxx::LevelPtr myLevel = myLogger->getLevel();
    if ( myLevel ) 
    {
      // If configured Level is the same or less restrictive than WARN (30000)
      // than report the warning
      if ( myLevel != log4cxx::Level::getOff() && myLevel->toInt() <= LL_WARN )
      {
        for (i=1; i<=diagsArea->getNumber(DgSqlCode::WARNING_); i++)
        {
          condition = diagsArea->getWarningEntry(i);
          diagMsg = condition->getMessageText();
          diagStr = unicodeToChar(diagMsg, NAWstrlen(diagMsg),
                                          CharInfo::ISO88591, NULL, TRUE);
          log(cat, LL_WARN, condition->getSQLCODE(), "",
            "  warn condition %d: %s", i, diagStr->data());
          delete diagStr;
        }
      }
    }
  }
}

// **************************************************************************
// **************************************************************************
CommonTracer::CommonTracer(const char*   fnName,
                           CommonLogger& logger,
                           std::string &logCategory,
                           TraceLevel    level,
                           const char*   file,
                           Int32         line)
  : fnName_(fnName),
    level_(level),
    file_(file ? file : ""),
    line_(line),
    logger_(logger),
    category_(logCategory)
{
  if (level_ == TL_all)
  {
    if (file_.length() == 0)
      logger_.log(category_, LL_DEBUG,
                  "Entering %s", fnName_.data());
    else
      logger_.log(category_, LL_DEBUG,
                  "Entering %s (file %s, line %d)",
                  fnName_.data(), file_.data(), line_);
  }
}

// **************************************************************************
// **************************************************************************
CommonTracer::~CommonTracer()
{
  NAString logMsg;

  if (level_ >= TL_exceptionOnly && std::uncaught_exception())
    logMsg.append("Exiting %s with uncaught exception");
  else if (level_ == TL_all)
    logMsg.append("Exiting %s");
  else
    return;

  if (file_.length() > 0)
  {
    logMsg.append("(file %s, line %d)");
    logger_.log(category_, LL_DEBUG,
                logMsg, fnName_.data(), file_.data(), line_);
  }
  else
  {
    logger_.log(category_, LL_DEBUG, logMsg, fnName_.data());
  }
}

void QRLogger::log(const std::string &cat,
                   logLevel    level,
                   int         sqlCode,
                   const char  *queryId,
                   const char  *logMsgTemplate...)
{

  log4cxx::LoggerPtr myLogger = log4cxx::Logger::getLogger(cat);
  if ( myLogger ) 
  {
    log4cxx::LevelPtr myLevel = myLogger->getLevel();
    log4cxx::LevelPtr paramLevel = log4cxx::Level::toLevel(level);
    if ( myLevel && paramLevel ) 
    {
        if ( myLevel == log4cxx::Level::getOff() )
          return;

        int_32 configuredLevel = myLevel->toInt();
        int_32 requestedLevel = paramLevel->toInt();

        // If the configured logging level is greater (more restrictive) than
        // the requested level, don't log.  
        if ( configuredLevel > requestedLevel)
          return;
    }
  } else return;
  
  va_list args ;
  va_start(args, logMsgTemplate);

  char* buffer = buildMsgBuffer(cat, level, logMsgTemplate, args);
  NAString logData = QRLogger::instance().getMyProcessInfo();
  char sqlCodeBuf[30];
  snprintf(sqlCodeBuf, sizeof(sqlCodeBuf), "%d", sqlCode);
  logData += ",";
  if (sqlCode != 0)
    {
      logData += " SQLCODE: ";
      logData += sqlCodeBuf;
    }
  logData += ",";
  if (queryId != NULL && queryId[0] != '\0')
    {
      logData += " QID: ";
      logData += queryId;
    }
  logData += ", ";
  logData += buffer;
  log1(cat, level, logData.data());
  va_end(args);
}

void QRLogger::log(const std::string &cat,
                   logLevel    level,
                   const char  *logMsgTemplate...)
{
  log4cxx::LoggerPtr myLogger = log4cxx::Logger::getLogger(cat);
  if ( myLogger ) 
  {
    log4cxx::LevelPtr myLevel = myLogger->getLevel();
    log4cxx::LevelPtr paramLevel = log4cxx::Level::toLevel(level);
    if ( myLevel && paramLevel ) 
    {
        if ( myLevel == log4cxx::Level::getOff() )
          return;

        int_32 configuredLevel = myLevel->toInt();
        int_32 requestedLevel = paramLevel->toInt();
  
        // If the configured logging level is greater (more restrictive) than
        // the requested level, don't log. 
        if ( configuredLevel > requestedLevel)
          return;
    }
  }else return;

  va_list args ;
  va_start(args, logMsgTemplate);

  char* buffer = buildMsgBuffer(cat, level, logMsgTemplate, args);
  NAString logData = QRLogger::instance().getMyProcessInfo();
  // SQLCode and Query id are not provided in this flavor
  logData += ",,,";
  logData += buffer;
  log1(cat, level, logData.data());

  va_end(args);
}

NABoolean QRLogger::initLog4cxx(ExecutableModule module)
{
   NABoolean retcode;
   static bool singleSqlLogFile = (getenv("TRAF_MULTIPLE_SQL_LOG_FILE") == NULL);
   QRLogger::instance().setModule(module);
   if (singleSqlLogFile)
      retcode =  QRLogger::instance().initLog4cxx("log4cxx.trafodion.sql.config");
   else
      retcode =  QRLogger::instance().initLog4cxx("log4cxx.trafodion.masterexe.config");
   return retcode;
}


NABoolean QRLogger::startLogFile(const std::string &cat, const char * logFileName)
{
  NABoolean retcode = TRUE;  // assume success
  try 
    {
      log4cxx::LoggerPtr logger(Logger::getLogger(cat));
      logger->setAdditivity(false);  // make this logger non-additive

      log4cxx::PatternLayout * layout = new PatternLayout("%d, %p, %c, %m%n");
      log4cxx::LogString fileName(logFileName);
      log4cxx::FileAppenderPtr fap(new FileAppender(layout,fileName,false /* no append */));
      
      logger->addAppender(fap);
    }
  catch (...)
    {
      retcode = FALSE;
    }

  return retcode;
}

NABoolean QRLogger::stopLogFile(const std::string &cat)
{
  NABoolean retcode = TRUE;  // assume success
  try 
    {
      log4cxx::LoggerPtr logger(Logger::getLogger(cat));
      logger->removeAllAppenders();
    }
  catch (...)
    {
      retcode = FALSE;
    }

  return retcode;
}

NABoolean QRLogger::getRootLogDirectory(const std::string &cat, std::string &out)
{
  NABoolean retcode = TRUE;  // assume success
   
  out.clear();

  // strip off all but the first qualifier of the category name

  size_t firstDot = cat.find_first_of('.');
  std::string firstQualifier;
  if (firstDot == std::string::npos)
    firstQualifier = cat;  // no dot, use the whole thing
  else
    firstQualifier = cat.substr(0,firstDot);

  try 
    {
      log4cxx::LoggerPtr logger(Logger::getLogger(firstQualifier));
      log4cxx::AppenderList appenderList = logger->getAllAppenders();
      for (size_t i = 0; i < appenderList.size(); i++)
        {
          log4cxx::AppenderPtr appender = appenderList[i];
          log4cxx::LogString appenderName = appender->getName();
          log4cxx::Appender * appenderP = appender;
          log4cxx::FileAppender * fileAppender = dynamic_cast<FileAppender *>(appenderP);
          if (fileAppender)
            {
              log4cxx::LogString logFileName = fileAppender->getFile();
              out = logFileName.data();
              size_t lastSlash = out.find_last_of('/');
              if (lastSlash != std::string::npos)
                out = out.substr(0,lastSlash);
              return TRUE;
            }
        }
    }
  catch (...)
    {
      retcode = FALSE;
    }

  return retcode;
}


