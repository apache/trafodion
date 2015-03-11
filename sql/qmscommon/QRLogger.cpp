// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
// **********************************************************************

#include <QRLogger.h>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/Configurator.hh>

#include "ComDiags.h"
#include "logmxevent.h"
#include "NLSConversion.h"

#include "PortProcessCalls.h"
#include "seabed/ms.h"
#include "seabed/fserr.h"

// SQL categories
const char CAT_SQL[]                           = "SQL";
const char CAT_SQL_EXE[]                       = "SQL.EXE";
const char CAT_SQL_COMP[]                      = "SQL.COMP";
const char CAT_SQL_ESP[]                       = "SQL.ESP";
const char CAT_SQL_LOB[]                       = "SQL.LOB";
const char CAT_SQL_SSMP[]                      = "SQL.SSMP";
const char CAT_SQL_SSCP[]                      = "SQL.SSCP";

// hdfs
const char CAT_SQL_HDFS_JNI_TOP[]              =  "SQL.HDFS.JniTop";
const char CAT_SQL_HDFS_SEQ_FILE_READER[]      =  "SQL.HDFS.SeqFileReader";
const char CAT_SQL_HDFS_SEQ_FILE_WRITER[]      =  "SQL.HDFS.SeqFileWriter";
const char CAT_SQL_HDFS_ORC_FILE_READER[]      =  "SQL.HDFS.OrcFileReader";
const char CAT_SQL_HBASE[]                     =  "SQL.HBase";

// these categories are currently not used 
const char CAT_SQL_QMP[]                       = "SQL.Qmp";
const char CAT_SQL_QMM[]                       = "SQL.Qmm";
const char CAT_SQL_COMP_QR_DESC_GEN[]          = "SQL.Comp.DescGen";
const char CAT_SQL_COMP_QR_HANDLER[]           = "SQL.Comp.QRHandler";
const char CAT_SQL_COMP_QR_COMMON[]            = "SQL.COMP.QRCommon";
const char CAT_SQL_COMP_QR_IPC[]               = "SQL.COMP.QRCommon.IPC";
const char CAT_SQL_COMP_MV_REFRESH[]           = "SQL.COMP.MV.REFRESH";
const char CAT_SQL_COMP_MVCAND[]               = "SQL.Comp.MVCandidates";
const char CAT_SQL_MEMORY[]                    = "SQL.Memory";
const char CAT_SQL_COMP_RANGE[]                = "SQL.COMP.Range";
const char CAT_QR_TRACER[]                     = "QRCommon.Tracer";
const char CAT_SQL_QMS[]                       = "SQL.Qms";
const char CAT_SQL_QMS_MAIN[]                  = "SQL.Qms.Main";
const char CAT_SQL_QMS_INIT[]                  = "SQL.Qms.Init";
const char CAT_SQL_MVMEMO_JOINGRAPH[]          = "SQL.Qms.MvmemoJoingraph";
const char CAT_SQL_MVMEMO_STATS[]              = "SQL.Qms.MvmemoStats";
const char CAT_SQL_QMS_GRP_LATTCE_INDX[]       = "SQL.Qms.LatticeIndex";
const char CAT_SQL_QMS_MATCHTST_MVDETAILS[]    = "SQL.Qms.MatchTest";
const char CAT_SQL_QMS_XML[]                   = "SQL.Qms.XML";
const char CAT_SQL_COMP_XML[]                  = "SQL.Comp.XML";

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
  static QRLogger onlyInstance_;
  return onlyInstance_;
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
  
  char processName[MS_MON_MAX_PROCESS_NAME + 1];
  

  memset(processName, 0, sizeof(processName));
  memcpy(processName, myPhandle.getPhandleString(), myPhandle.getPhandleStringLen());
  memset(processName, 0, sizeof(processName));
  memcpy(processName, myPhandle.getPhandleString(), myPhandle.getPhandleStringLen());

 

  MS_Mon_Process_Info_Type procInfo;
  Int32 rc = 0;

  Int32 myNid = 0;
  
  do
  {
     rc = msg_mon_get_process_info_detail(processName, &procInfo);
     myNid = procInfo.nid;
   
  } while ((rc == XZFIL_ERR_OK) && (procInfo.parent_nid != -1) && (procInfo.parent_pid != -1));

  snprintf (stringNidSuffix, 5+sizeof(Int32), "_%d.log", myNid);
}
// **************************************************************************
// Call the superclass to configure log4cpp from the config file.
// If the configuration file is not found, perform default initialization:
// Create an appender, layout, and categories.
// Attaches layout to the appender and appender to categories.
// **************************************************************************
NABoolean QRLogger::initLog4cpp(const char* configFileName)
{
  NAString logFileName;

  // get the log directory
  logFileName = "";

  // gets the top ancestor process name that will be used to name the file appender log
  char logFileSuffix [100]="";
  
  switch (module_)
    {
    case QRL_NONE:
    case QRL_MXCMP:
    case QRL_ESP:
    case QRL_MXEXE:
      getMyTopAncestor(logFileSuffix);
      break;
    case QRL_LOB:
    case QRL_SSMP:
    case QRL_SSCP:
      getMyNidSuffix(logFileSuffix);
      break;
    default:
      break;
    }

  
  if (CommonLogger::initLog4cpp(configFileName, logFileSuffix))
  {
    introduceSelf();
    return TRUE;
  }

  logFileName += "sql_events";
  logFileName += logFileSuffix;
  
  fileAppender_ = new log4cpp::RollingFileAppender("FileAppender", logFileName.data());

  log4cpp::PatternLayout *fileLayout = new log4cpp::PatternLayout();
  fileLayout->setConversionPattern("%d, %p, %c, %m%n");
  fileAppender_->setLayout(fileLayout);

  // Top level categories
  initCategory(CAT_SQL_QMP, log4cpp::Priority::ERROR);
  initCategory(CAT_SQL_QMM, log4cpp::Priority::ERROR);
  initCategory(CAT_SQL_COMP_QR_COMMON, log4cpp::Priority::ERROR);
  initCategory(CAT_SQL_QMS, log4cpp::Priority::ERROR);

  initCategory(CAT_SQL, log4cpp::Priority::ERROR);
  switch (module_)
  {
    case QRL_NONE:
    case QRL_MXCMP:
      initCategory(CAT_SQL_COMP, log4cpp::Priority::ERROR);
      log4cpp::Category::getInstance(CAT_SQL_COMP).setPriority(log4cpp::Priority::INFO);
      log4cpp::Category::getInstance(CAT_SQL_COMP).setAdditivity(false);
      break;

    case QRL_ESP:
      initCategory(CAT_SQL_ESP, log4cpp::Priority::ERROR);
      log4cpp::Category::getInstance(CAT_SQL_ESP).setPriority(log4cpp::Priority::INFO);
      log4cpp::Category::getInstance(CAT_SQL_ESP).setAdditivity(false);
      break;

    case QRL_MXEXE:
      initCategory(CAT_SQL_EXE, log4cpp::Priority::ERROR);
      log4cpp::Category::getInstance(CAT_SQL_EXE).setPriority(log4cpp::Priority::INFO);
      log4cpp::Category::getInstance(CAT_SQL_EXE).setAdditivity(false);
      break;
      
  }

  introduceSelf();

  // initialize sub categories here - they were removed since they are not currently being used

  return FALSE;
}

// **************************************************************************
// **************************************************************************
void QRLogger::initCategory(const char* cat, log4cpp::Priority::PriorityLevel defaultPriority)
{
  log4cpp::Category& catObj = log4cpp::Category::getInstance(cat);
  catObj.setAppender(fileAppender_);
  catObj.setPriority(defaultPriority);
}


const char* QRLogger::getMyDefaultCat()
{
  const char* myCat;
  switch (module_)
    {
    case QRL_MXCMP:
      myCat = CAT_SQL_COMP;
      break;
    case QRL_MXEXE:
      myCat = CAT_SQL_EXE;
      break;
    case QRL_ESP:
      myCat = CAT_SQL_ESP;
      break;
    case QRL_LOB:
      myCat = CAT_SQL_LOB;
      break;
    case QRL_SSMP:
      myCat = CAT_SQL_SSMP;
      break;
    case QRL_SSCP:
      myCat = CAT_SQL_SSCP;
      break;

    default:
      myCat = CAT_SQL;   
    }

  return myCat;
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

  snprintf(procInfo, 300, "Node Number: %d, CPU: %d, PIN: %d, Process Name: %s", mynodenumber,mycpu, mypin, myprocname);

  processInfo_ = procInfo;
  return processInfo_.data();
}

void QRLogger::introduceSelf ()
{
  char msg[300];
  const char* myCat = getMyDefaultCat(); 
  // save previous default priority
  log4cpp::Priority::PriorityLevel defaultPriority = (log4cpp::Priority::PriorityLevel) log4cpp::Category::getInstance(myCat).getPriority();

  NAString procInfo = getMyProcessInfo();

  // change default priority to INFO to be able to printout some infomational messages
  log4cpp::Category::getInstance(myCat).setPriority(log4cpp::Priority::INFO);
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
    }

   log4cpp::Category::getInstance(myCat).info(msg);

   // restore default priority
   log4cpp::Category::getInstance(myCat).setPriority(defaultPriority);
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
// LCOV_EXCL_START :rfi
void QRLogger::logError(const char* file, 
                            Int32       line, 
                            const char* cat,
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
    log4cpp::Category::getInstance(cat).fatal(buffer);
  else
    log4cpp::Category::getInstance(cat).error(buffer);

  // format actual error message
  vsnprintf(buffer, BUFFER_SIZE, logMsgTemplate, args);

  NAString logData2 = QRLogger::instance().getMyProcessInfo();
  logData2 += ",,, ";
  logData2 += buffer;

  // log actual error msg to logfile.
  if (level == LL_FATAL)
    log4cpp::Category::getInstance(cat).fatal(logData2.data());
  else
    log4cpp::Category::getInstance(cat).error(logData2.data());

  va_end(args);
}
// LCOV_EXCL_STOP

// This temporary function is used for logging QVP messages only. More extensive
// changes to the CommonLogger hierarchy will be made when the QI integration
// stream is merged into the mainline. For now, we avoid making changes that
// affect MVQR to minimize merge difficulties.
void QRLogger::logQVP(ULng32 eventId,
                          const char* cat,
                          logLevel    level,
                          const char* logMsgTemplate...)
{
  // Don't do anything if this message will be ignored anyway.
  log4cpp::Category &myCat = log4cpp::Category::getInstance(cat);
  if (myCat.getPriority() < level)
    return;

  va_list args;
  va_start(args, logMsgTemplate);

  char* buffer = buildMsgBuffer(cat, level, logMsgTemplate, args);
  log1(cat, level, buffer, eventId);
  delete [] buffer;

  va_end(args);
}


void QRLogger::logDiags(ComDiagsArea* diagsArea, const char* cat)
{
  const NAWchar* diagMsg;
  NAString* diagStr;
  Int32 i;
  ComCondition* condition;
  log4cpp::Category& myCat = log4cpp::Category::getInstance(cat);

  if (myCat.getPriority() >= LL_ERROR)
    for (i=1; i<=diagsArea->getNumber(DgSqlCode::ERROR_); i++)
      {
        condition = diagsArea->getErrorEntry(i);
        diagMsg = condition->getMessageText();
        diagStr = unicodeToChar(diagMsg, NAWstrlen(diagMsg),
                                          CharInfo::ISO88591, NULL, TRUE);
        log(cat, LL_ERROR, condition->getSQLCODE(), "",
          "  err condition %d: %s", i, diagStr->data());
        delete diagStr;
      }

  if (myCat.getPriority() >= LL_WARN)
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

// **************************************************************************
// **************************************************************************
CommonTracer::CommonTracer(const char*   fnName,
                           CommonLogger& logger,
                           const char*   logCategory,
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
    // LCOV_EXCL_START :cnu
    if (file_.length() == 0)
      logger_.log(category_, LL_DEBUG,
                  "Entering %s", fnName_.data());
    else
      logger_.log(category_, LL_DEBUG,
                  "Entering %s (file %s, line %d)",
                  fnName_.data(), file_.data(), line_);
    // LCOV_EXCL_STOP
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
    logMsg.append("Exiting %s");  // LCOV_EXCL_LINE :cnu
  else
    return;

  if (file_.length() > 0)
  {
    // LCOV_EXCL_START :cnu
    logMsg.append("(file %s, line %d)");
    logger_.log(category_, LL_DEBUG,
                logMsg, fnName_.data(), file_.data(), line_);
    // LCOV_EXCL_STOP
  }
  else
  {
    logger_.log(category_, LL_DEBUG, logMsg, fnName_.data());
  }
}


// **************************************************************************
// The generic message logging method for any message type and length.
// adds information about the current process
// **************************************************************************
void QRLogger::log(const char  *cat,
                   logLevel    level,
                   const char  *logMsgTemplate...)
{
  log4cpp::Category &myCat = log4cpp::Category::getInstance(cat);
  if (myCat.getPriority() < level)
    return;

  va_list args ;
  va_start(args, logMsgTemplate);

  char* buffer = buildMsgBuffer(cat, level, logMsgTemplate, args);
  NAString logData = QRLogger::instance().getMyProcessInfo();
  // SQLCode and Query id are not provided in this flavor
  logData += ",,,";
  logData += buffer;
  log1(cat, level, logData.data());
  delete [] buffer;

  va_end(args);
}


void QRLogger::log(const char  *cat,
                   logLevel    level,
                   int         sqlCode,
                   const char  *queryId,
                   const char  *logMsgTemplate...)
{
  log4cpp::Category &myCat = log4cpp::Category::getInstance(cat);
  if (myCat.getPriority() < level)
    return;

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
  delete [] buffer;

  va_end(args);
}

