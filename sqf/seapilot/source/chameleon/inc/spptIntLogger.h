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

#ifndef __SPPA_INT_LOGGER_HEADER__
#define __SPPA_INT_LOGGER_HEADER__
#include <time.h>
#include <vector>
#include <string>
#include <nl_types.h>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include "wrapper/amqpwrapper.h"
#include "wrapper/externroutingkey.h"
#include "common/chameleon.events.pb.h"

#include "common/spptLogger.h"

using namespace std;


/* ====================== INTERNAL USAGE ======================================*/


void logError1(int32_t errNum, const string& v1);
void logError2(int32_t errNum, const string& v1, const string& v2);
void logError3(int32_t errNum, const string& v1, const string& v2 , const string& v3);
void logError4(int32_t errNum, const string& v1, const string& v2 , const string& v3, const string& v4);


class spptError {
public:
    //!constructor
    spptError() {}

    //!constructor
    spptError(int32_t en, int32_t level, const string & file, int32_t line, nl_catd catd);

    //!constructor
    spptError(int32_t en, int32_t level, const string &v1, const string &v2, const string &v3, const string & v4 ,const string & file, int32_t line, nl_catd catd);

    //! level of this error
    int32_t level_;

    //!error number
    int32_t errorNum_;

    //! source file of the error
    string srcFile_;

    //! source file line number of the error
    int32_t line_;

    int32_t t1_;
    string v1_;

    int32_t t2_;
    string v2_;

    int32_t t3_;
    string v3_;

    int32_t t4_;
    string v4_;

    vector<int> types_;
    vector<string> values_;
    vector<string> tokens_;
    int32_t tokenNumber_;
    int32_t tix_;

    //! gencat catalog template file
    nl_catd catd_;

    //!timestamp of the error
    string ts_;

    string outputString_;

    //! output this error
    string printMe(void);


    int32_t findGpbMsgId(string &tag);

    //! replace tokens with values in a string
    void replaceIt(string &str);

    //! level
    int32_t level() { return level_; }

};

class spptLogger {

   public:
       static void createInstance();
       static void deleteInstance();

       static spptLogger * instance(); 
       static void report_error(int code);
       //!destructor
       ~spptLogger();

       void logError ( int32_t level, int32_t error_num, spptErrorVar & vars);
       void logError ( int32_t LEVEL,  int32_t error_num, const string & err_string , const string & src_file, int32_t line_num); 

       static bool spptIsRetryableSQLError(int32_t SQL_Error);
       static bool spptIsRetryableOSError(int32_t Error_Num);

        //! do init
        //! open file
        int32_t init(int32_t level);     

        void setMode(LOG_MODE mode);

        //~ set the current global logging level
        void setLevel(int32_t level) {
               if(level <= SPPA_TRACE && level >= SPPA_ERROR)
                 currentLevel_ = level;
               else
                 currentLevel_ = SPPA_INFO;
            }

        void setRedirector(bool v) { redirector_ = v; }

        void setBrokerIp(const string & ip) { broker_ip_ = ip ; }

        void setBrokerPort(int32_t port) { broker_port_ = port; }

        void setCatalogFile(const string &f) { catalogFile_ = f; }

        void setReconn(bool r) { reconn_ = r; }
     
        void setProgramName(const string &name) { program_name_ = name;}

        void setLogFileNm(const string &f) { logfilename_ = f; }

        void setLogSize(int32_t sz ) { if(sz >=0 ) bufferSize_ =  sz; }
    
        void setInfoParts(int32_t parts, LOG_MODE mode) {
            if( mode == LOG_MODE_STARTUP)
                startup_info_parts_ = parts;
            else if( mode == LOG_MODE_NORMAL)
                normal_info_parts_ = parts; 
            }

        void flush(void);

        int magic_;

    private:
        //!constructor
        spptLogger(); 
 
        int lockFile(int fd, int cmd, int type, off_t offset, int whence, off_t len);
    private:
        static pthread_mutex_t lglock;

        static pthread_mutex_t pInstance_lock_;
        static spptLogger * pInstance_;
        
        LOG_MODE mode_;
        
        int32_t startup_info_parts_;
    
        int32_t normal_info_parts_;

        //! Only used for NORMAL mode to make sure the first log info intert into an empty file. For STARTUP mode, always append log info to logfile.
        bool FIRST_LINE_;

        //! name of the program
        string program_name_;

        //! name of the file
        string logfilename_;

        //! ip address of the Qpid broker
        string broker_ip_;

        //!port number of the Qpid broker
        int32_t broker_port_;

        //! real file name
        string logfilenamePhy_;
      
        //! size of the cache
        int32_t logSize_;

        //! catalog file used to convert error number to text
        string catalogFile_;

        google::protobuf::Message *gpbMsg_;

        int32_t currFilePtr_;

        int32_t currentLevel_;

        //! bytes written into single log file
        int32_t writeBytes_;
    
        int currFd_;

        //! gencat catalog id
        nl_catd catd_;

        //! current linux process ID 
        int32_t pid_;

        bool reconn_;

        //!redirector
        bool redirector_;

        //! log file name counter
        int32_t  currLogCounter_;

        //!the buffer
        vector<spptError> cache_;

        //pointer to current poistion in the cache
        int32_t currBufPtr_;

        int32_t bufferSize_;

        //! write file
        int32_t writeFile(string msg);

        //! output an error
        int32_t sendError(spptError &err);

        int32_t sendPub(spptError &err);
        
        int32_t switchLogfile(void);

        void makeLogFName();
		time_t result_;
        struct tm *nowtm_;

        char timebuf_[TLEN];

        void getCurrTime();

        bool fillGpbMsg();
        

        //! a google protobuf for a chameleon event
        chameleon::events chameEvent_;

        int32_t mapEnumToGpbId(AddErrType_Enum type);

        //! init the message header
        void initGpbMsg();

};

#define SPPA_INIT_LOGFILE     0x1
#define SPPA_INIT_RECONN      0x2
#define SPPA_INIT_BROKER      0x4
#define SPPA_INIT_CATALOG     0x8
#define SPPA_INIT_ALL         SPPA_INIT_LOGFILE | SPPA_INIT_RECONN | SPPA_INIT_BROKER | SPPA_INIT_CATALOG

#endif

