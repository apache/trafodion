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

//
// Text Protocol Adapter
//
// Reads publication data from a named pipe and feeds it to the qpid broker.
//

#include <signal.h>

#include <iostream>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "common/spptLogger.h"
#include "common/evl_sqlog_eventnum.h"
#include "common/sqEnv.h"

#include "TextProtocolAdapter.h"
#include "vers/versbin.h"

using namespace std;
using namespace boost;
namespace po=boost::program_options;
namespace fs=boost::filesystem;

const char *MY_SQROOT = "MY_SQROOT";
const char *QPID_NODE_PORT = "QPID_NODE_PORT";

const int32_t ERR_OK = 0;
const int32_t ERR_INVALID_OPT = 1;
const int32_t ERR_INITLOGGER = 2;
const int32_t ERR_CONFIG = 3;
const int32_t ERR_SEABED = 4;

static void sig_catch(int signum)
{
   cout << "Signal " << signum << " caught" << endl;
   if (SIGINT == signum || SIGTERM == signum) {
      cout << "terminating..." << endl;
      exit(1);
   }
}

TextProtocolAdapter tpa;
                           
VERS_BIN(sp_textprotocoladapter)
DEFINE_COMP_DOVERS(sp_textprotocoladapter)
int32_t main(int argc, char** argv) 
{
   CALL_COMP_DOVERS(sp_textprotocoladapter, argc, argv);
   po::options_description desc("Allowed options", SPPA_LONG_LINE);
   try
   {
      // init seabed message service
      SqEnv env(&argc, &argv, "TPA");

      signal(SIGTERM, sig_catch);
      signal(SIGINT, sig_catch);
      atexit(spptFinalizeLogger);
      spptInitLogger(argv[0]);
      spptSetLogMode(LOG_MODE_STARTUP);
      

      // listenFifo defaults to $MY_SPROOT directory
      const char *pSqRoot = getenv(MY_SQROOT);

      if (pSqRoot == NULL) {
         pSqRoot = "";
      }
      string strDefFifo = str(format("%1%/seapilot/amqp-tpa") % pSqRoot);
      string strDefProtoSrc = str(format("%1%/seapilot/export/publications") % pSqRoot);
      string strDefLogFile = str(boost::format("%1%/seapilot/logs/TPA.log") % pSqRoot);

      char *pNlbPort = getenv(QPID_NODE_PORT);
      uint32_t pDefaultNlbPort = 5672;
      if ( pNlbPort ){
         pDefaultNlbPort = atoi(pNlbPort);
      }

      // ------------Set up program options ----------------------
      string host;
      uint32_t port;
      string listenFifo;
      string strLogFile;
      string strTextLog;
      uint32_t timeout;
 
      desc.add_options()
           ("help,h", "produce this help message")
           ("listen-fifo", po::value<string>(&listenFifo)->default_value(strDefFifo), "listening fifo")
           ("log-file", po::value<string>(&strLogFile)->default_value(strDefLogFile), "log file")
           ("publish-broker-ip", po::value<string>(&host)->default_value("127.0.0.1"), "publish broker IP address")
           ("publish-broker-port", po::value<uint32_t>(&port)->default_value(pDefaultNlbPort), "publish broker port number")
           ("proto-src", po::value<string>()->default_value(strDefProtoSrc), "root directory of protocol source") 
           ("text-catalog", po::value<string>(&strTextLog)->default_value("seaquest.cat")
                       , "specify the location of catalog file")
           ("timeout", po::value<uint32_t>(&timeout)->default_value(30), "time to sleep after retry")
           ("verbose", "if set, dumps debug messages to screen")
       ;

      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, desc), vm);
      po::notify(vm);
   
      if (vm.count("help")) {
         cout << "Usage: textprotocoladapter [options]\n";
         cout << desc << endl;

         return ERR_OK;
      }

      if (ends_with(strLogFile, ".log")) {
         replace_last(strLogFile,".log", "");
      }

      fs::path pTextLog(strTextLog);
      if ( !exists(pTextLog) ) {
         if (strTextLog[0] != '/') {
            strTextLog = str(boost::format("%1%/seapilot/export/conf/") % pSqRoot)
                           + strTextLog;
         }
      }

      spptSetLogFile(strLogFile);
      int32_t rc = spptSetLogger(host, port,
                                 strTextLog,
                                 vm.count("verbose"));

      if (rc) {
         cout <<  "Error when init logger" << endl;
         logInfo("Error when init logger. Program exit.\n");
         return ERR_INITLOGGER;
      }

      if (vm.count("verbose")) {
         spptSetLogLevel(SPPA_TRACE);
      }
      spptSetLogMode(LOG_MODE_NORMAL);
      logInfo("TPA Start");

      tpa.init(timeout, listenFifo, vm["proto-src"].as<string>());

      int retCode = tpa.doit(host, port);

      spptLogFlush();
      return retCode;
   }
   catch (SB_Fatal_Excep &e)
   {
      cout<< "Error: " << e.what()<<endl;
      return ERR_SEABED;
   }
   catch ( const program_options::error & error )
   {
      cout << error.what( ) << endl;
      cout << desc << endl;

      return ERR_INVALID_OPT;
   }
}


