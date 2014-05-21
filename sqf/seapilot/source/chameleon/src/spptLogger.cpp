// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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

#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <nl_types.h>
#include <ctype.h>
#include <sstream>
#include <fstream>

#include "wrapper/amqpwrapper.h"
#include "wrapper/externroutingkey.h"

#include "common/sp_defines.h"  // for AMQP wrapper constants
#include "common/sp_util.h"
#include "sqevlog/evl_sqlog_writer.h"   // for SQEVL_SEABRIDGE constant

#include "spptIntLogger.h"
#include "common/chameleon.events.pb.h"

using namespace std;

static seapilot::Stdout sp_cout;

pthread_mutex_t spptLogger::lglock = PTHREAD_MUTEX_INITIALIZER;

spptLogger * spptLogger::pInstance_ = 0;
pthread_mutex_t spptLogger::pInstance_lock_ = PTHREAD_MUTEX_INITIALIZER;

void logTrace(const string &msg)
{
  spptLogger::instance()->logError(SPPA_TRACE, 0, msg, __FILE__, __LINE__);
}
void logInfo(const string &msg)
{
  spptLogger::instance()->logError(SPPA_INFO, 0, msg, __FILE__, __LINE__);
}

void logError(int32_t errNum, spptErrorVar &var)
{
  spptLogger::instance()->logError(SPPA_ERROR, errNum, var); 
}

void spptErrorVar::reset()
{
  addErrorList_.clear(); 
}

bool spptErrorVar::isEmpty()
{
  if(addErrorList_.size() > 0)
    return true;
  else
    return false;
}


void spptErrorVar::setNodeName(const string &str)
{
  spptAddError ae;
  ae.errorType_=NODE_NAME;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setProcessName(const string &str)
{
  spptAddError ae;
  ae.errorType_= PROCESS_NAME;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setFileName(const string &str)
{
  spptAddError ae;
  ae.errorType_=FILE_NAME;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setNodeId(int32_t num)
{
  spptAddError ae;
  ae.errorType_= NODE_ID;
  char str[128];
  sprintf(str,"%d",num);
  ae.value_ = str;
  addErrorList_.push_back(ae);
}


void spptErrorVar::setProcessId(int32_t num)
{
  spptAddError ae;
  ae.errorType_= PROCESS_ID;
  char str[128];
  sprintf(str,"%d",num);
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setErrorNumber(int32_t num)
{
  spptAddError ae;
  ae.errorType_= ERROR_NUMBER;
  char str[128];
  sprintf(str,"%d",num);
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setErrorString(const string &str)
{
  spptAddError ae;
  ae.errorType_= ERROR_STRING;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setFileLineNumber(int32_t num)
{
  spptAddError ae;
  ae.errorType_= FILE_LINE_NUMBER;
  char str[128];
  sprintf(str,"%d",num);

  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setFileColumnNumber(int32_t num){
  spptAddError ae;
  ae.errorType_= FILE_COLUMN_NUMBER;
  char str[128];
  sprintf(str,"%d",num);
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setPublicationColumn(const string &str)
{
  spptAddError ae;
  ae.errorType_= PUBLICATION_COLUMN;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setPublicationColumnFullName(const string &str){
  spptAddError ae;
  ae.errorType_= PUBLICATION_FIELD_NAME;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setPublicationName(const string &str){
  spptAddError ae;
  ae.errorType_= PUBLICATION_NAME;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setDataType(int32_t datatype)
{
  spptAddError ae;
  ae.errorType_= PROTO_DATA_TYPE;
  char tmp[128];
  sprintf(tmp,"%d",datatype);
  ae.value_ = tmp;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setRouteKey(const string &str)
{
  spptAddError ae;
  ae.errorType_= ROUTING_KEY;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setRepositoryColumn(const string &str)
{
  spptAddError ae;
  ae.errorType_= REPOSITORY_COLUMN;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setRepositoryTable(const string &str)
{
  spptAddError ae;
  ae.errorType_= REPOSITORY_TABLE_NAME;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setSqlErrorString(const string &str)
{
  spptAddError ae;
  ae.errorType_= SQL_ERROR_STRING;
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setSqlErrorCode(int32_t err)
{
  spptAddError ae;
  ae.errorType_= SQL_ERROR_CODE;
  char str[128];
  sprintf(str,"%d",err);
  ae.value_ = str;
  addErrorList_.push_back(ae);
}

void spptErrorVar::setFsErrorCode(int32_t err)
{
  spptAddError ae;
  ae.errorType_= FS_ERROR_CODE;
  char tmp[128];
  sprintf(tmp,"%d",err);
  ae.value_ = tmp;
  addErrorList_.push_back(ae);
}
 
int32_t spptInitLogger(const string &programName, const string &logfilename, const string &ip, int32_t port, const string &catalog, bool redirect, int32_t csize)
{
  spptLogger::createInstance();
  int ret = 0;
  spptLogger::instance()->setProgramName(programName);
  spptLogger::instance()->setBrokerIp(ip);
  spptLogger::instance()->setLogFileNm(logfilename);
  spptLogger::instance()->setBrokerPort(port);
  spptLogger::instance()->setCatalogFile(catalog);
  spptLogger::instance()->setRedirector(redirect);
  spptLogger::instance()->setLogSize(csize);
  ret = spptLogger::instance()->init(SPPA_INIT_ALL);

  return ret;
}

int32_t spptSetLogger(const string &ip, int32_t port, const string &catalog, bool redirect, int32_t csize)
{
  int32_t ret=0;
  spptLogger::instance()->setBrokerIp(ip);
  spptLogger::instance()->setBrokerPort(port);
  spptLogger::instance()->setCatalogFile(catalog);
  spptLogger::instance()->setRedirector( redirect);
  spptLogger::instance()->setLogSize(csize);
  ret = spptLogger::instance()->init(SPPA_INIT_BROKER | SPPA_INIT_CATALOG);
  return ret;
}

void spptSetLogMode(LOG_MODE mode)
{
  spptLogger::instance()->setMode(mode);
}

void spptSetLogLevel(int32_t level)
{
  spptLogger::instance()->setLevel(level);
}

void spptSetBufferSize(int32_t cs)
{
  spptLogger::instance()->setLogSize(cs);
}

void spptSetInfoParts(int32_t parts, LOG_MODE mode)
{
  spptLogger::instance()->setInfoParts(parts, mode);
}

void spptFinalizeLogger()
{
  spptLogger::deleteInstance();
}

void spptSetProgramName(const string &name)
{
    spptLogger::instance()->setProgramName(name);
    return ;
}

void spptSetLogFile(const string &logfilename)
{
  spptLogger::instance()->setLogFileNm(logfilename);
  spptLogger::instance()->init(SPPA_INIT_LOGFILE);
  return ;
}

int32_t spptSetLogCatalog(const string &file)
{
  int32_t ret = 0;
  spptLogger::instance()->setCatalogFile(file);
  ret = spptLogger::instance()->init(SPPA_INIT_CATALOG);
  return ret;

}

int32_t spptSetLogBroker(const string &ip, int32_t port)
{
  int32_t ret = 0;
  spptLogger::instance()->setBrokerIp(ip);
  spptLogger::instance()->setBrokerPort(port);
  ret = spptLogger::instance()->init(SPPA_INIT_BROKER);
  return ret;

}

void spptSetLogRedirect(bool r)
{
  spptLogger::instance()->setRedirector(r);
  return ;
}
 
void spptLogFlush(void)
{
  spptLogger::instance()->flush();
}


/*====================================INTERNAL=============================================*/
/*====================================usage only=============================================*/


bool spptIsRetryableOSError(int32_t Error_Num)
{
  return false;
}

bool spptIsRetryableSQLError(int32_t SQL_Error)
{
  return false;
}

void spptLogger::createInstance()
{
    report_error(pthread_mutex_lock(&pInstance_lock_));
    if( pInstance_ == 0) 
    {
        try
        {
            pInstance_ = new spptLogger();
        }
        catch(std::exception &error)
        {
            sp_cout << error.what( ) << std::endl;
            exit(1);
        }
    }
    report_error(pthread_mutex_unlock(&pInstance_lock_));
}

void spptLogger::deleteInstance()
{
    report_error(pthread_mutex_lock(&pInstance_lock_));
    if( pInstance_ != 0) 
    {
        delete pInstance_;
        pInstance_ = 0;
    }
    report_error(pthread_mutex_unlock(&pInstance_lock_));
}

spptLogger * spptLogger::instance()
{
    assert(pInstance_ != 0);
    return pInstance_;
}

void spptLogger::report_error(int code)
{
    if( code !=0)
    {
        std::cout <<strerror(errno) <<endl;
        abort();
    }
}

//constructor
spptLogger::spptLogger()
    :mode_(LOG_MODE_NORMAL), program_name_(""), logfilename_(""), redirector_(false)
{
  currFilePtr_ = 0;
  writeBytes_ = 0;
  currFd_ = 0;
  currentLevel_ = SPPA_INFO;
  currLogCounter_ = 1;
  pid_ = 0;
  magic_ = CHAME_MAGIC_NUM;
  reconn_ = false;
  catd_ = nl_catd(-1);
  bufferSize_= SPPA_MAX_BUFFER_SIZE;
  startup_info_parts_ = PROGRAM_NAME_PARTS | TIMESTAMP_PARTS | MAIN_INFO_PARTS;
  normal_info_parts_ = TIMESTAMP_PARTS | THREAD_ID_PARTS |  MAIN_INFO_PARTS;
  FIRST_LINE_ = true;
  broker_port_ = 0;
  currBufPtr_ = 0;
  logSize_ = 1;
  memset(timebuf_, 0, TLEN);
}

spptLogger::~spptLogger()
{  
  if(reconn_)
  {
    flush();
    closeAMQPConnection();
  }
  
  if(currFd_!=0 && currFd_ != -1 )
    close(currFd_);

  if (nl_catd(-1) != catd_)
  {
      catclose(catd_);
      catd_ = nl_catd(-1);
  }
}

int32_t spptLogger::switchLogfile()
{
  int32_t ret = 0;
  close(currFd_);
  currFd_ = -1;
  
  if( currLogCounter_ == SPPA_MAX_LOGFILE_COUNTER )
  {
    currLogCounter_ = 1;  //rewind the file counter
  }
  else
  {
    currLogCounter_++;
  }

  makeLogFName();
 
  remove(logfilenamePhy_.c_str());
  return ret;
}


void spptLogger::makeLogFName()
{
    //Create the startup.log file if user didn't specify log filename. Useful during application arguments parsing.
    if( logfilename_.empty())
    {
        logfilenamePhy_ = getenv("MY_SQROOT");
        if( logfilenamePhy_.empty())
            logfilenamePhy_ += "../../logs/";
        else 
            logfilenamePhy_ += "/seapilot/logs/";
        stringstream ss;
        ss <<SPPA_LOG_FILE_NAME <<"." <<currLogCounter_ <<".log";
        logfilenamePhy_ += ss.str();
    }
    else
    {
        logfilenamePhy_ = ""; //reset
        stringstream ss;
        if( mode_ == LOG_MODE_BRIEF)
          ss << logfilename_ << "." <<currLogCounter_ << ".log";
        else
          ss << logfilename_ << "." << pid_ << "." <<currLogCounter_ << ".log";
        logfilenamePhy_ = ss.str();
        FIRST_LINE_ = true;
    }
}


int32_t spptLogger::init(int32_t level)
{
  int32_t ret = 0;
  //open the file
  pid_ = getpid();

  if(level & SPPA_INIT_LOGFILE)
    makeLogFName();

  //open the catalog file
  if(level & SPPA_INIT_CATALOG)
  {
    if(catd_ != nl_catd(-1))
        catclose(catd_);
    catd_ = catopen(catalogFile_.c_str(), 1);
    if(catd_ == nl_catd(-1))
        ret = -1;
  }

  //connect to broker
  if( level & SPPA_INIT_BROKER)
  {
    if(broker_port_ !=-1)
    {
        sp_cout <<"spptLogger: Attempt to establish qpid connection to " <<broker_ip_.c_str() <<":" <<broker_port_ <<". (This may take several minutes.)" <<endl;
        int error = createAMQPConnection(broker_ip_.c_str(),broker_port_);
        if(error == SP_SUCCESS)
            sp_cout <<"spptLogger: qpid connection succeeded." <<endl;
        else 
        {
            sp_cout <<"spptLogger: qpid connection failed! Error: " << error << endl;
            ret = -1;
        }

        reconn_ = true;
    }
    else
    {
        reconn_ = false;
        ret = -1;
    }
  }
  return ret;
}

void spptLogger::setMode(LOG_MODE mode)
{
  mode_ = mode;
  close(currFd_);
  currFd_ = -1;
  return ;
}

void spptLogger::logError ( int32_t LEVEL,  int32_t error_num, spptErrorVar &var)
{
  if( mode_ == LOG_MODE_STARTUP)
  {
    if( startup_info_parts_ & MAIN_INFO_PARTS)
    {
      spptError aErr(error_num,LEVEL,"",0, catd_);
      if( startup_info_parts_ & TIMESTAMP_PARTS)
      {
        getCurrTime();
        aErr.ts_ = timebuf_;
      }
     
      vector<spptAddError> vars = var.getErrorList();
      int sizeOfIt = vars.size();
      for(int ix=0 ; ix < sizeOfIt; ix++)
      {
        aErr.values_.push_back(vars[ix].value_);
        aErr.types_.push_back(vars[ix].errorType_);
      }

      currBufPtr_++;
      report_error(pthread_mutex_lock(&lglock));
      cache_.push_back(aErr);
      report_error(pthread_mutex_unlock(&lglock));
    }
  }
  else if( mode_ == LOG_MODE_NORMAL)
  {
    if( normal_info_parts_ & MAIN_INFO_PARTS)
    {
      spptError aErr(error_num,LEVEL,"",0, catd_);
      if( normal_info_parts_ & TIMESTAMP_PARTS)
      {
        getCurrTime();
        aErr.ts_ = timebuf_;
      }

      vector<spptAddError> vars = var.getErrorList();
      int sizeOfIt = vars.size();
      for(int ix=0 ; ix < sizeOfIt; ix++)
      {
        aErr.values_.push_back(vars[ix].value_);
        aErr.types_.push_back(vars[ix].errorType_);
      }

      currBufPtr_++;
      report_error(pthread_mutex_lock(&lglock));
      cache_.push_back(aErr);
      report_error(pthread_mutex_unlock(&lglock));
    }
  }
  else if( mode_ == LOG_MODE_BRIEF)
  {
    sp_cout << "logError1 - LOG_MODE_BRIEF." << endl;
    if( normal_info_parts_ & MAIN_INFO_PARTS)
    {
      spptError aErr(error_num,LEVEL,"",0, catd_);
      if( normal_info_parts_ & TIMESTAMP_PARTS)
      {
        getCurrTime();
        aErr.ts_ = timebuf_;
      }

      vector<spptAddError> vars = var.getErrorList();
      int sizeOfIt = vars.size();
      for(int ix=0 ; ix < sizeOfIt; ix++)
      {
        aErr.values_.push_back(vars[ix].value_);
        aErr.types_.push_back(vars[ix].errorType_);
      }

      currBufPtr_++;
      report_error(pthread_mutex_lock(&lglock));
      cache_.push_back(aErr);
      report_error(pthread_mutex_unlock(&lglock));
    }
  }

  if(currBufPtr_ >= bufferSize_)
  {
    currBufPtr_ = 0;
    flush();
  }
}

int32_t  spptLogger::mapEnumToGpbId(AddErrType_Enum type)
{
  //fix me later
  //we brutally map the enum to GPB id, maybe there will be better solution here
  int32_t gpbId = -1;

  if(type == ERROR_NUMBER) 
    gpbId = -1 ;//todo
  else if( type == ERROR_STRING )
    gpbId = chameleon::events::kErrorStringFieldNumber ;
  else if( type == FILE_NAME)
    gpbId = chameleon::events::kFileNameFieldNumber;
  else if( type == FILE_LINE_NUMBER )
    gpbId = chameleon::events::kFileLineNumberFieldNumber; 
  else if( type == FILE_COLUMN_NUMBER)
    gpbId = chameleon::events::kFileColumnNumberFieldNumber;
  else if( type == PUBLICATION_COLUMN)
    gpbId = chameleon::events::kPublicationColumnFieldNumber;
  else if( type == PUBLICATION_FIELD_NAME)
    gpbId = chameleon::events::kPublicationColumnFullNameFieldNumber; 
  else if( type == PUBLICATION_NAME)
    gpbId = chameleon::events::kPublicationNameFieldNumber;
  else if( type == PROTO_DATA_TYPE)
    gpbId = chameleon::events::kProtoDataTypeFieldNumber; 
  else if( type == ROUTING_KEY)
    gpbId = chameleon::events::kRoutingKeyFieldNumber; 
  else if( type == REPOSITORY_COLUMN)
    gpbId = chameleon::events::kRepositoryColumnFieldNumber; 
  else if( type == REPOSITORY_TABLE_NAME)
    gpbId = chameleon::events::kRepositoryTableFieldNumber; 
  else if( type == SQL_ERROR_STRING)
    gpbId = chameleon::events::kSqlErrorTextFieldNumber; 
  else if( type == SQL_ERROR_CODE)
    gpbId = chameleon::events::kSqlCodeFieldNumber; 
  else
    gpbId = -1;
  return gpbId; 
}

void spptLogger::getCurrTime(void)
{  
  result_= time(NULL);
  nowtm_= localtime(&result_);
  bzero(timebuf_,TLEN);
  strftime(timebuf_, TLEN, "[%D %T]", nowtm_); 
}

void spptLogger::logError ( int32_t LEVEL,  int32_t error_num, const string& v1, const string & src_file, int line_num)
{
  if( mode_ == LOG_MODE_STARTUP)
  {
    if( startup_info_parts_ & MAIN_INFO_PARTS)
    {
      spptError aErr(error_num,LEVEL,v1,"","","",src_file,line_num, catd_);
      if( startup_info_parts_ & TIMESTAMP_PARTS)
      {
        getCurrTime();
        aErr.ts_ = timebuf_;        
      }
      
      currBufPtr_++;
      report_error(pthread_mutex_lock(&lglock));
      cache_.push_back(aErr);
      report_error(pthread_mutex_unlock(&lglock));
    } 
  }
  else if( mode_ == LOG_MODE_NORMAL)
  {
    if( normal_info_parts_ & MAIN_INFO_PARTS) 
    {
      spptError aErr(error_num,LEVEL,v1,"","","",src_file,line_num, catd_);
      if( normal_info_parts_ & TIMESTAMP_PARTS)
      {
        getCurrTime();
        aErr.ts_ = timebuf_;
      }

      currBufPtr_++;
      report_error(pthread_mutex_lock(&lglock));
      cache_.push_back(aErr);
      report_error(pthread_mutex_unlock(&lglock));
    }
  }
  else if( mode_ == LOG_MODE_BRIEF)
  {
    sp_cout << "logError2 - LOG_MODE_BRIEF." << endl;
    if( normal_info_parts_ & MAIN_INFO_PARTS)
    {
      spptError aErr(error_num,LEVEL,v1,"","","",src_file,line_num, catd_);
      if( normal_info_parts_ & TIMESTAMP_PARTS)
      {
        getCurrTime();
        aErr.ts_ = timebuf_;
      }

      currBufPtr_++;
      report_error(pthread_mutex_lock(&lglock));
      cache_.push_back(aErr);
      report_error(pthread_mutex_unlock(&lglock));
    }
  }

  if(currBufPtr_ >= bufferSize_)
  {
    currBufPtr_ = 0;
    flush();
  }

}

bool spptLogger::spptIsRetryableSQLError(int SQL_Error)
{
//todo
  bool ret = true;
  return ret;
}

bool spptLogger::spptIsRetryableOSError(int Error_Num)
{
  bool ret = true;
  return ret;
}

bool spptLogger::fillGpbMsg()
{
  bool ret = true;
  return ret;
}


int32_t spptLogger::writeFile(string msg)
{
  int32_t ret = 0;
#if 0
  if(redirector_ == true) //both COUT and log file
  {
    sp_cout << msg;
  }
#endif

  if(writeBytes_ >= SPPA_MAX_LOGFILE_SZ_BYTE )
  {
    //todo: create a new file?
    switchLogfile();
    writeBytes_ = 0;
  }

  if(currFd_ == 0 || currFd_ == -1)
  {
    if(FIRST_LINE_ && mode_ == LOG_MODE_NORMAL)
    {
      currFd_ = open(logfilenamePhy_.c_str(),O_CREAT|O_TRUNC|O_RDWR,0660);
      FIRST_LINE_ = false;
    }
    else
      currFd_ = open(logfilenamePhy_.c_str(),O_CREAT|O_RDWR,0660);

    if(currFd_ == -1)
    {
      perror("open file");
      ret = -1;
    }
  }

  lockFile(currFd_, F_SETLKW, F_WRLCK, 0, SEEK_SET, 0);
  lseek(currFd_,0,SEEK_END); 
  int32_t b = write(currFd_, const_cast<char *>(msg.c_str()), msg.size());
  if(b< 0 )
  {
    return  -1;
  }
  lockFile(currFd_, F_SETLKW, F_UNLCK, 0, SEEK_SET, 0);

  writeBytes_+= msg.size(); 
  return ret;  
}

int32_t spptLogger::sendError(spptError &err)
{
    string message;
    stringstream tmpBuf;
    if( mode_ == LOG_MODE_STARTUP)
    {
        if( startup_info_parts_ & PROGRAM_NAME_PARTS)
            message += program_name_ + "\t"; 
        if( startup_info_parts_ & THREAD_ID_PARTS)
        {
            tmpBuf << pthread_self();
            message += tmpBuf.str() + "\t"; 
        }
        if( startup_info_parts_ & LOG_FILENAME_PARTS)   
            message += logfilename_ + "\t";
    }
    else if( mode_ == LOG_MODE_NORMAL)
    {
        if( normal_info_parts_ & PROGRAM_NAME_PARTS) 
            message += program_name_ + "\t";
        if( normal_info_parts_ & THREAD_ID_PARTS)
        {
            tmpBuf << pthread_self();
            message += tmpBuf.str() + "\t"; 
        }
        if( normal_info_parts_ & LOG_FILENAME_PARTS)
            message += logfilename_ + "\t";
    }
    else if( mode_ == LOG_MODE_BRIEF)
    {
        // No prefix.
    }

    int32_t ret = 0;  
    switch(err.level())
    {
    case SPPA_ERROR:
    {
        //Record error logs at any log level.
        writeFile(message + err.printMe());
        sendPub(err); 
        break;
    }
    case SPPA_INFO:
    {
        //Record info logs when log level is TRACE or INFO.
        if(currentLevel_ ==  SPPA_TRACE || currentLevel_ ==  SPPA_INFO)
        {
            writeFile(message + err.printMe());
        }
        break;
    }
    case SPPA_TRACE:
    {
        //Record trace logs only when log level is TRACE.
        if(currentLevel_ == SPPA_TRACE)  // trace
        {
            writeFile(message + err.printMe());
        }
        break;
    }
    default:
        break;
    }
    return ret;
}

void spptLogger::initGpbMsg()
{

  common::event_header * eventHeader = chameEvent_.mutable_header();
  common::info_header * infoHeader = eventHeader->mutable_header();
  common::qpid_header * qpidHeader; 
  initAMQPInfoHeader(infoHeader,SQEVL_SEABRIDGE /* component ID */);
  eventHeader->set_event_severity(SQ_LOG_ERR); 
  setAMQPInfoHeaderSequenceNumber(infoHeader); // update sequence number in info and qpid headers

  qpidHeader = infoHeader->mutable_header();

  chameEvent_.clear_error_number();
  chameEvent_.clear_error_string();
  chameEvent_.clear_file_line_number();
  chameEvent_.clear_file_column_number();
  chameEvent_.clear_file_name();
  chameEvent_.clear_publication_column();
  chameEvent_.clear_publication_column_full_name();
  chameEvent_.clear_publication_name();
  chameEvent_.clear_proto_data_type();
  chameEvent_.clear_publication_column2();
  chameEvent_.clear_routing_key();
  chameEvent_.clear_repository_column();
  chameEvent_.clear_repository_table();
  chameEvent_.clear_sql_error_text();
  chameEvent_.clear_sql_code();
  chameEvent_.clear_sq_fs_error_number();
  chameEvent_.clear_node_id();
  chameEvent_.clear_node_name();
  chameEvent_.clear_process_id();
  chameEvent_.clear_process_name();

}

int32_t spptLogger::sendPub(spptError &err)
{
  int32_t ret = 0;
  initGpbMsg();

  common::event_header * eventHeader = chameEvent_.mutable_header();
  common::info_header * infoHeader = eventHeader->mutable_header();
  common::qpid_header * qpidHeader; 
  qpidHeader = infoHeader->mutable_header();
  eventHeader->set_event_id(err.errorNum_);  


  int msgId = 0;
  int tokenNm = err.types_.size();
  for(int ix=0; ix < tokenNm ; ix++)
  {
    //string tokenStr = err.tokens_[ix];
    msgId = err.types_[ix];

    switch(msgId)
    {
      //case chameleon::events::kErrorStringFieldNumber:
      case ERROR_STRING:
      {
        chameEvent_.set_error_string(err.values_[ix]);
      }
      break; 
      case FILE_LINE_NUMBER:
      //case chameleon::events::kFileLineNumberFieldNumber:
      {
        int32_t lineNumber = atoi(err.values_[ix].c_str());
        chameEvent_.set_file_line_number(lineNumber);
      }
      break;
      case FILE_COLUMN_NUMBER: 
      //case chameleon::events::kFileColumnNumberFieldNumber:
      {
        int32_t columnNumber = atoi(err.values_[ix].c_str());
        chameEvent_.set_file_column_number(columnNumber);
      }
      break;
      case FILE_NAME: 
      //case chameleon::events::kFileNameFieldNumber:
      {
        chameEvent_.set_file_name(err.values_[ix]);
      }
      break;
      case PUBLICATION_COLUMN:
      //case chameleon::events::kPublicationColumnFieldNumber:
      {
        chameEvent_.set_publication_column(err.values_[ix]);
      }
      break; 
      case PUBLICATION_FIELD_NAME:
      //case chameleon::events::kPublicationColumnFullNameFieldNumber:
      {
        chameEvent_.set_publication_column_full_name(err.values_[ix]);
      }
      break; 
      case PUBLICATION_NAME:
      //case chameleon::events::kPublicationNameFieldNumber:
      {
        chameEvent_.set_publication_name(err.values_[ix]);
      }
      break; 
      case PROTO_DATA_TYPE:
      //case chameleon::events::kProtoDataTypeFieldNumber:
      {
        int32_t protoDataType = atoi(err.values_[ix].c_str());
        chameEvent_.set_proto_data_type(protoDataType);
      }
      break; 
      //case ROUTING_KEY:
      //case chameleon::events::kPublicationColumn2FieldNumber:
      //{
      //  chameEvent_.set_publication_column2(err.values_[ix]);
      //}
      case  ROUTING_KEY:
      //case chameleon::events::kRoutingKeyFieldNumber:
      {
        chameEvent_.set_routing_key(err.values_[ix]);
      }
      break; 
      case REPOSITORY_COLUMN:
      //case chameleon::events::kRepositoryColumnFieldNumber:
      {
        chameEvent_.set_repository_column(err.values_[ix]);
      }
      break; 
      case REPOSITORY_TABLE_NAME:
      //case chameleon::events::kRepositoryTableFieldNumber:
      {
        chameEvent_.set_repository_table(err.values_[ix]);
      }
      break; 
      case SQL_ERROR_STRING:
      //case chameleon::events::kSqlErrorTextFieldNumber:
      {
        chameEvent_.set_sql_error_text(err.values_[ix]);
      }
      break; 
      case FS_ERROR_CODE:
      //case chameleon::events::kSqlCodeFieldNumber:
      {
        int32_t sqlCode = atoi(err.values_[ix].c_str());
        chameEvent_.set_sql_code(sqlCode);
      }
      break; 
      case NODE_ID:
      //case chameleon::events::kSqlCodeFieldNumber:
      {
        int32_t sqlCode = atoi(err.values_[ix].c_str());
        chameEvent_.set_node_id(sqlCode);
      }
      break; 
      case NODE_NAME:
      //case chameleon::events::kSqlCodeFieldNumber:
      {
        chameEvent_.set_sql_error_text(err.values_[ix]);
      }
      break; 
      case PROCESS_ID:
      //case chameleon::events::kSqlCodeFieldNumber:
      {
        int32_t sqlCode = atoi(err.values_[ix].c_str());
        chameEvent_.set_process_id(sqlCode);
      }
      break; 
      case PROCESS_NAME:
      //case chameleon::events::kSqlCodeFieldNumber:
      {
        chameEvent_.set_sql_error_text(err.values_[ix]);
      }
      break; 
      default:
      break;  // ignore unknown tokens}
    }//end of switch 
  }//end of for 

  //publication is ready now
  string message;
  if(chameEvent_.SerializeToString(&message) == true)
  {
      string contentType_= "application/x-protobuf";
      AMQPRoutingKey routingKey_(SP_EVENT /* publication type */, 
            SP_CHAMPACKAGE /* package name */, 
            SP_INSTANCE /* scope */, 
            SP_PUBLIC /* security */, 
	    SP_GPBPROTOCOL /* protocol */,
            "events" /* publication name */);

      int errorCode = sendAMQPMessage(false, message,
                    contentType_,
                    routingKey_,
                    true /* send synchronously */,
 	            &chameEvent_);

      if (SP_SUCCESS != errorCode)
      { 
          // if we could not send it, $$$ perhaps write to syslog?
          // $$$ temp for testing (note that it does no good to put it into
          // the error stack; we'd just loop creating new errors!) 
          sp_cout << "Tried to write error " << err.errorNum_ << " to Qpid but failed." << endl;
      }
  }

  return ret;
}


void spptLogger::flush()
{
  //loop through the cache to flush
  //for trace and info, cout them and to the logfile
  //for error, also need to send publication...
  report_error(pthread_mutex_lock(&lglock));
  int s = cache_.size();
  for (int ix = 0; ix < s; ix++)
  {
    sendError(cache_[ix]);
  }

  cache_.clear();
  report_error(pthread_mutex_unlock(&lglock));
}
 
int spptLogger::lockFile(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
    flock fileLock;
    fileLock.l_type = type;
    fileLock.l_start = offset;
    fileLock.l_whence = whence;
    fileLock.l_len = len;
    
    return (fcntl(fd, cmd, fileLock));
}
    

spptError::spptError(int32_t error_num,int32_t level, const string & src_file, int line_num, nl_catd catd)
:level_(level),errorNum_(error_num),line_(line_num),catd_(catd)
{
  srcFile_ = src_file;
}

spptError::spptError(int32_t en, int32_t level, const string &v1,const string &v2,const string & v3, const string & v4, const string & file, int32_t line, nl_catd catd)
:level_(level),errorNum_(en),line_(line),v1_(v1),v2_(v2),v3_(v3),v4_(v4),catd_(catd)
{
  srcFile_ = file;
  values_.push_back(v1);
  values_.push_back(v2);
  values_.push_back(v3);
  values_.push_back(v4);
}

void spptError::replaceIt(string &msg)
{
  //find $
  //fix me

  char buf[SPPA_MAX_MSG_SIZE];
  int i = 0, len = 0;
  bool paramStart = false;
  string tokenNm;
  string result;

  memset(buf, 0, SPPA_MAX_MSG_SIZE);
  if(msg.size() > SPPA_MAX_MSG_SIZE) return; //todo

  strcpy(buf , msg.c_str());
  len = strlen(buf);

  tokens_.clear();
  tix_ = 0;
  
  for(i = 0; i < len; i++)
  {
    if( buf[i] == '$') //start a param
    {
      paramStart = true;
    }
    else
    {
      if(paramStart == true)
      {
        if( isalpha(buf[i]) == 0 && buf[i] != '_' &&buf[i] != '~' ) //endof token
        {
          tokens_.push_back(tokenNm);
          paramStart = false;
          int indx = 0, y = 0;
          indx = findGpbMsgId(tokenNm);
          int s = types_.size();
          for(y = 0; y<s; y++)
          {
            if(indx == types_[y] )   break;
          }
          if(y < s)
            result += values_[y];
          else
            result += "unknown";
          result += buf[i];
          tokenNm="";
          tix_++;
        }
        else
        {
          tokenNm+=buf[i];
        }
      }
      else
      {
        result += buf[i];
      }
    }
  }
  if(paramStart == true)
  {
    tokens_.push_back(tokenNm);
    result += values_[tix_];
  }
  msg = result;
}

#if 1
int32_t spptError::findGpbMsgId(string &tag)
{

  if(tag == "~error_number")
    return ERROR_NUMBER;
  else if( tag == "~error_string")
    return ERROR_STRING;
  else if( tag == "~file_line_number")
    return FILE_LINE_NUMBER;
  else if( tag == "~file_column_number")
    return FILE_COLUMN_NUMBER;
  else if( tag == "~file_name")
    return FILE_NAME;
  else if( tag == "~publication_column")
    return PUBLICATION_COLUMN;
  else if( tag == "~publication_column_full_name")
    return PUBLICATION_FIELD_NAME;
  else if( tag == "~publication_name")
    return PUBLICATION_NAME;
  else if( tag == "~proto_data_type")
    return PROTO_DATA_TYPE;
  else if( tag == "~routing_key")
    return ROUTING_KEY;
  else if( tag == "~repository_column")
    return REPOSITORY_COLUMN;
  else if( tag == "~repository_table")
    return REPOSITORY_TABLE_NAME;
  else if( tag == "~sql_error_text")
    return SQL_ERROR_STRING;
  else if( tag == "~sql_code")
    return SQL_ERROR_CODE;
  else if( tag == "~sq_fs_error_number")
    return FS_ERROR_CODE;
  else if( tag == "~node_id")
    return NODE_ID;
  else if( tag == "~node_name")
    return NODE_NAME;
  else if( tag == "~process_id")
    return PROCESS_ID;
  else if( tag == "~process_name")
    return PROCESS_NAME;
  else
    return -1;
}
#endif

string spptError::printMe(void)
{
  const char * defaultMessage = "Message text unavailable.";
  const char * message = defaultMessage;

  outputString_ = ""; //clear the string
  //Component   Timestamp                 filename    line       Text
  outputString_ += ts_;

  if( !outputString_.empty())
    outputString_ += '\t';

  if(level_ == SPPA_ERROR) //we need to get the text from gencat
  {
    //todo get error text and replace the varaibles
    if(nl_catd(-1) != catd_)
    {
      message = catgets(catd_, 1, errorNum_, defaultMessage);
      string tmpStr = message;
      replaceIt(tmpStr);
      outputString_ += tmpStr;
    }
    else
    {
      outputString_ += defaultMessage;
    }
  }
  else
  {
    outputString_ += v1_;
  }

  outputString_ += '\n';
  
  return outputString_;
}

void logDeath(const string& pName, const string& deathMsg)
{
    // the log would be $MY_SQROOT/seapilot/logs/process.death.log
    string logfile = getenv("MY_SQROOT");
    if( logfile.empty() )
        logfile = "../../logs/process.death.log";
    else
        logfile += "/seapilot/logs/process.death.log";
    ofstream fout(logfile.c_str(), ios_base::app | ios_base::out );

    // construct death log in this format:
    // [date time] [process name] [death message]
    string msg("[");
    time_t curtime;
    time(&curtime);
    msg += ctime(&curtime);
    msg.erase(msg.size()-1); //remove the last newline character
    msg += "]\t[";
    msg += pName;
    msg += "]\t[";
    msg += deathMsg;
    msg += ']';

    if(fout.good())
        fout << msg << endl;
    fout.close();
}
