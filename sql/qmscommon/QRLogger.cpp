// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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

const char CAT_QMP[]                  = "Qmp";
const char CAT_QMM[]                  = "Qmm";
const char CAT_COMPILER[]             = "Comp";
const char CAT_QR_DESC_GEN[]          = "Comp.DescGen";
const char CAT_QR_HANDLER[]           = "Comp.QRHandler";
const char CAT_MVCAND[]               = "Comp.MVCandidates";
const char CAT_QR_COMMON[]            = "QRCommon";
const char CAT_QR_IPC[]               = "QRCommon.IPC";
const char CAT_MEMORY[]               = "QRCommon.Memory";
const char CAT_RANGE[]                = "QRCommon.Range";
const char CAT_QR_TRACER[]            = "QRCommon.Tracer";
const char CAT_QMS[]                  = "Qms";
const char CAT_QMS_MAIN[]             = "Qms.Main";
const char CAT_QMS_INIT[]             = "Qms.Init";
const char CAT_MVMEMO_JOINGRAPH[]     = "Qms.MvmemoJoingraph";
const char CAT_MVMEMO_STATS[]         = "Qms.MvmemoStats";
const char CAT_GRP_LATTCE_INDX[]      = "Qms.LatticeIndex";
const char CAT_MATCHTST_MVDETAILS[]   = "Qms.MatchTest";
const char CAT_QMS_XML[]              = "Qms.XML";
const char CAT_CMP_XML[]              = "Comp.XML";

// **************************************************************************
// **************************************************************************
QRLogger::QRLogger()
  : CommonLogger()
   ,module_(QRL_NONE)
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
// Call the superclass to configure log4cpp from the config file.
// If the configuration file is not found, perform default initialization:
// Create an appender, layout, and categories.
// Attaches layout to the appender and appender to categories.
// **************************************************************************
NABoolean QRLogger::initLog4cpp(const char* configFileName)
{
  if (CommonLogger::initLog4cpp(configFileName))
    return TRUE;

  NAString logFileName;

  // get the log directory
  logFileName = logFolder_;

  switch (module_)
  {
    case QRL_NONE:
    case QRL_MXCMP:
      logFileName += "mvqr.log";
      break;

    case QRL_QMS:
      logFileName += "qms.log";
      break;

    case QRL_QMP:
      logFileName += "qmp.log";
      break;

    case QRL_QMM:
      logFileName += "qmm.log";
      break;
  }

  fileAppender_ = new log4cpp::RollingFileAppender("FileAppender", logFileName.data());

  log4cpp::PatternLayout *fileLayout = new log4cpp::PatternLayout();
  fileLayout->setConversionPattern("%d, %p, %c, %m%n");
  fileAppender_->setLayout(fileLayout);

  // Top level categories
  initCategory(CAT_QMP, log4cpp::Priority::ERROR);
  initCategory(CAT_QMM, log4cpp::Priority::ERROR);
  initCategory(CAT_QR_COMMON, log4cpp::Priority::ERROR);
  initCategory(CAT_QMS, log4cpp::Priority::ERROR);
  initCategory(CAT_COMPILER, log4cpp::Priority::ERROR);

  // Sub-categories.
  //initCategory(CAT_QR_DESC_GEN, log4cpp::Priority::ERROR);
  //initCategory(CAT_QR_HANDLER, log4cpp::Priority::ERROR);
  //initCategory(CAT_QR_IPC, log4cpp::Priority::ERROR);
  //initCategory(CAT_MEMORY, log4cpp::Priority::ERROR);
  //initCategory(CAT_RANGE, log4cpp::Priority::ERROR);
  //initCategory(CAT_MVCAND, log4cpp::Priority::ERROR);
  //initCategory(CAT_QMS_MAIN, log4cpp::Priority::ERROR);
  //initCategory(CAT_QMS_INIT, log4cpp::Priority::ERROR);
  //initCategory(CAT_MVMEMO_JOINGRAPH, log4cpp::Priority::ERROR);
  //initCategory(CAT_GRP_LATTCE_INDX, log4cpp::Priority::ERROR);
  //initCategory(CAT_MATCHTST_MVDETAILS, log4cpp::Priority::ERROR);
  //initCategory(CAT_XML, log4cpp::Priority::ERROR);

  // Change priority just long enough to emit "no config file" message as INFO,
  // so we don't confuse user with a message categorized as ERROR.
  log4cpp::Category::getInstance(CAT_QR_COMMON).setPriority(log4cpp::Priority::INFO);
  log4cpp::Category::getInstance(CAT_QR_COMMON).info("No config file present, using default logging level."); 
  log4cpp::Category::getInstance(CAT_QR_COMMON).setPriority(log4cpp::Priority::ERROR);
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

