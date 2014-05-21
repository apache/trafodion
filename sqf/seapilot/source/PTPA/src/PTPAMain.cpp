// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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

///////////////////////////////////////////////////////////////////////////
// We don't need to support HA for PTPA part. There are two HA scenarios
// tHat PTPA should support.  One is that proxy should restart PTPA process 
// when PTPA is killed. For this part, we had done it already.  The other 
// is that when one node is down, others should take charge of all the work 
// of this node.  A set of PTPAs are subscribing to the same ECB. They have 
// same subscribe-broker-port and subscirbe-broker-ip.  Use "pdsh -a ps -ef 
// |grep PTPA" to check.  As all the nodes have started their own PTPA 
// process, it's no need to restart it on other nodes. 
///////////////////////////////////////////////////////////////////////////

#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <sys/stat.h>
#include <XMLConfigurer.h>

#include "seabed/fserr.h"
#include "vers/versbin.h"
#include "seabed/ms.h"
#include "common/spptLogger.h"
#include "MalMessageTextCatalog.h"

#include "PTPAEventListener.h"
#include "PTPAConfigHandler.h"

VERS_BIN(sp_PTPA)
DEFINE_COMP_DOVERS(sp_PTPA)


void cleanup()
{
    spptLogFlush();
    spptFinalizeLogger();
    msg_mon_process_shutdown( );
    return ;
}

int main(int argc, char** argv)
{
    using namespace boost;
    using namespace Trinity;
    using namespace std;

    CALL_COMP_DOVERS(sp_PTPA, argc, argv);
    spptInitLogger(argv[0]);
    spptSetLogMode(LOG_MODE_STARTUP);
    
    program_options::options_description desc( "Allowed options" );
    try
    {
        char dummy[1] = { '\0' };
        int error = msg_init_attach( &argc, &argv, 1, dummy );
        if (error != XZFIL_ERR_OK)
        {
            cout << "ERROR: msg_init_attach():" << error << endl;
            logDeath(argv[0], "msg_init_attach() fails");
            return 1;
        }

        char processName[MS_MON_MAX_PROCESS_NAME + 1];
        msg_mon_process_startup( true );
        msg_mon_enable_mon_messages( 1 );
        msg_debug_hook( "PTPA.hook", "PTPA.hook" );

        error = msg_mon_get_my_info( NULL,
                                     NULL,
                                     processName,
                                     MS_MON_MAX_PROCESS_NAME,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL );
        if (error)
        {
            processName[0] = '\0';
        }

        // Set up default log file.
        string sproot = string(getenv("MY_SPROOT"));
        string logfile = sproot + "/logs/PTPA";
        string protoSrc = sproot + "/export/publications";
  
        PTPAArguments args;
        args.subBrokerPort = 0;
        args.pubBrokerPort = 0;
        args.verbose = false;
        desc.add_options( )
                ("help", "print this message")
                ("subscribe-broker-ip", program_options::value<string>(&args.subBrokerIP)->default_value( "127.0.0.1" ), "IP address that PTPA receives messages from")
                ("subscribe-broker-port", program_options::value<int>(&args.subBrokerPort)->default_value( 5672 ), "Port that PTPA receives messages from")
                ("publish-broker-ip", program_options::value<string>(&args.pubBrokerIP)->default_value("127.0.0.1"), "IP address that PTPA sends text messages to")
                ("publish-broker-port", program_options::value<int>(&args.pubBrokerPort)->default_value(5672), "Port that PTPA sends text messages to")
                ("log-file", program_options::value<string>(&args.logFile)->default_value(logfile.data()), "Absolute path and name of log file.")
                ("config-file", program_options::value<string>(&args.configFile), "Configuration file for routingkeys")
                ("text-catalog", program_options::value<string>(&args.textCatalog), "Catalog file")
                ("proto-src", program_options::value<string>(&args.protoSrc)->default_value(protoSrc.data()), "Directory containing *.proto files")
                ("queue-name", program_options::value<string>(&args.queueName)->default_value("PTPA.queue"), "Queue name")
                ("verbose", "Verbose mode to display informative messages");

        int noGuessStyle = program_options::command_line_style::default_style ^ 
                           program_options::command_line_style::allow_guessing;
        program_options::variables_map vm;
        program_options::store( program_options::parse_command_line( argc, argv, desc, noGuessStyle), vm);
        program_options::notify( vm);

        if (argc <= 1)
        {
            cout << desc;
            cleanup();
            return 0;
        }
        if (vm.count( "help" ))
        {
            cout << "Usage: PTPA [options]\n";
            cout << desc;
            cleanup();
            return 0;
        }
        if (!vm.count( "config-file" ))
        {
            cout << "Missing required parameter --config-file." <<endl;
            cout << "Usage: PTPA [options]\n";
            cout << desc;
            logDeath(argv[0], "Missing required parameter --config-file");
            cleanup();
            return 0;
        }

        if (vm.count( "verbose" ))
            args.verbose = true;
        else 
            args.verbose = false;

        //setup logger
        if (args.textCatalog == "" )
            args.textCatalog = sproot + "/export/conf/seaquest.cat";
        spptSetLogCatalog( args.textCatalog);

        spptSetLogMode(LOG_MODE_BRIEF);  // The PTPA logs incoming publications as log entries.
        spptSetLogFile(args.logFile);
        spptSetLogRedirect(args.verbose);

        if (args.verbose == true )
            spptSetLogLevel( SPPA_TRACE);
        
        PTPAConfigHandler configHandler;
        XMLConfigurer xc(&configHandler, args.configFile);
        xc.run();
        if (configHandler.hasError())
        {
            cout <<"Error during XML parsing." <<endl;
            cleanup();
            return 1;
        }

        vector<string> routingkeyVector = configHandler.getRoutingKeys();
        map<string, string> configMap = configHandler.getConfigMap();
        
        string exchange_name = "amq.topic";

        MalMessageTextCatalog messageCatalog(args.textCatalog);
        PTPAEventListener listener(args.subBrokerIP, args.subBrokerPort, args.pubBrokerIP, args.pubBrokerPort,
                                   messageCatalog, args.protoSrc, configMap, args.verbose);

        // prepare the incoming data queues
        int ret = listener.prepareQueue(args.queueName, exchange_name, routingkeyVector);
        if (ret != 0)
        {
            cleanup();
            return 1;
        }

        // Listen and receive messages
        listener.listen();
    }
    catch (const program_options::error & error)
    {
        cout << error.what( ) << endl;
        cout << desc;
        logDeath(argv[0], error.what());
        cleanup();
        return 2;
    }
    catch (const std::exception& error)
    {
        cout << error.what( ) << endl;
        logDeath(argv[0], error.what());
        cleanup();
        return 2;
    }
    catch (...)
    {
        cout << "Unknown exception." << endl;
        logDeath(argv[0], "Unknown exception.");
        cleanup();
        return 2;
    }
    cleanup();
    return 0;
}
