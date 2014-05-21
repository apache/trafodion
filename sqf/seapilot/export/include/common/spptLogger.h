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

#ifndef __SPPA_LOGGER_HEADER__
#define __SPPA_LOGGER_HEADER__
#include <time.h>
#include <vector>
#include <string>
#include <nl_types.h>

#include <qpid/client/Session.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include "wrapper/amqpwrapper.h"
#include "wrapper/externroutingkey.h"
#include "common/chameleon.events.pb.h"

using namespace std;

#define SPPA_MAX_BUFFER_SIZE 1 
#define SPPA_MAX_LOGFILE_SZ_BYTE 1024*1024*10   //10M
#define SPPA_MAX_LOGFILE_COUNTER 10  //10 files
#define TLEN 128

#define SPPA_ERROR 1
#define SPPA_INFO 2
#define SPPA_TRACE 3

#define SPPA_LONG_LINE  256  // for help message, the default 80 is not big enough
#define SPPA_MAX_MSG_SIZE 4096

#define CHAME_MAGIC_NUM 3579

#define SPPA_LOG_FILE_NAME "spStartup"

/* =================== EXTERNAL INTERFACES ================================*/
//token type
enum AddErrType_Enum {
  ERROR_NUMBER,
  ERROR_STRING,
  FILE_NAME,
  FILE_LINE_NUMBER,
  FILE_COLUMN_NUMBER,
  PUBLICATION_COLUMN,
  PUBLICATION_FIELD_NAME,
  PUBLICATION_NAME,
  PROTO_DATA_TYPE,
  ROUTING_KEY,
  REPOSITORY_COLUMN,
  REPOSITORY_TABLE_NAME,
  SQL_ERROR_STRING,
  SQL_ERROR_CODE,
  FS_ERROR_CODE,
  NODE_ID,
  NODE_NAME,
  PROCESS_ID,
  PROCESS_NAME
  // whatever other types we come up with
};

struct spptAddError {
  AddErrType_Enum     errorType_;
  string              value_;
};

class spptErrorVar {
  public:
    spptErrorVar(){}
    ~spptErrorVar(){}
    void reset();
    bool isEmpty();
    void setFileName(const string &str);
    void setErrorNumber(int32_t num);
    void setErrorString(const string &str);
    void setFileLineNumber(int32_t num);
    void setFileColumnNumber(int32_t num);
    void setPublicationColumn(const string &str);
    void setPublicationColumnFullName(const string &str);
    void setPublicationName(const string &str);
    void setDataType(int32_t datatype);
    void setRouteKey(const string &str);
    void setRepositoryColumn(const string &str);
    void setRepositoryTable(const string &str);
    void setSqlErrorString(const string &str);
    void setSqlErrorCode(int32_t err);
    void setFsErrorCode(int32_t err);
    void setNodeId(int32_t nid);
    void setNodeName(const string &str);
    void setProcessId(int32_t pid);
    void setProcessName(const string &nm);
    vector<spptAddError> & getErrorList(){ return addErrorList_; }
  private:
    vector <spptAddError> addErrorList_; // vector
};

//! For STARTUP mode, log info from different programs will be recorded into one startup log file.
//! For NORMAL mode, log info from different programs will be recorded into program specified file. 
enum LOG_MODE
{
    LOG_MODE_STARTUP,
    LOG_MODE_NORMAL,
    LOG_MODE_BRIEF
};

enum LOG_INFO_PARTS
{
    PROGRAM_NAME_PARTS = 0x0001,
    LOG_FILENAME_PARTS = 0x0002,
    TIMESTAMP_PARTS = 0x0004,
    MAIN_INFO_PARTS = 0x0008,
    THREAD_ID_PARTS = 0x0010
};


//! log a trace
void logTrace(const string &msg);

//! log an info
void logInfo(const string &msg);

//! log en error
void logError(int32_t errNum, spptErrorVar &var);

//! log death message of SP processes
//! this log function differs from other log* functions because
//! it's a standalone function and it logs directly to process.death.log
void logDeath(const string& processName, const string& deathMsg);


//! init the logger
//! logfilename  -- name which will be part of the log file name on disk

//! ip/port      -- ip/port of the Qpid broker
//! catalog      -- where to find the template file
//! redirect     -- true if you want to print the message to STDOUT
int32_t spptInitLogger(const string &programName = "", const string &logfilename="", const string &ip="default", int32_t port=-1, const string &catalog = "../conf/seaquest.cat", bool redirect = false, int32_t csize = SPPA_MAX_BUFFER_SIZE);

int32_t spptSetLogCatalog(const string &file);
int32_t spptSetLogBroker(const string &ip, int32_t port);
int32_t spptSetLogger(const string &ip, int32_t port, const string &catalog = "", bool redirect = false, int32_t csize = SPPA_MAX_BUFFER_SIZE );
void spptSetLogMode(LOG_MODE mode);
void spptSetProgramName(const string &name);
void spptSetLogFile(const string &logfilename);
void spptSetLogRedirect(bool r);
void spptSetLogLevel(int32_t level);
void spptSetBufferSize(int32_t cs);
//! See LOG_INFO_PARTS for optional parts. Note that MAIN_INFO_PARTS must be selected, or there will be no log information.  
void spptSetInfoParts(int32_t parts, LOG_MODE mode);
void spptFinalizeLogger();
void spptLogFlush();
bool spptIsRetryableSQLError(int32_t SQL_Error);
bool spptIsRetryableOSError(int32_t Error_Num);


#endif

