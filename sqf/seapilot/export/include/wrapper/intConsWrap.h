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

#ifndef __CONSUMERWRAPPER_H
#define __CONSUMERWRAPPER_H

#ifndef WIN32
#include <syslog.h>
#endif


#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/text_format.h>
#include <qpid/messaging/Message.h>
#include "../wrapper/qpidwrapper.h"
#include "../wrapper/config.h"


#ifdef SEAQUEST_PROCESS
#include "seabed/ms.h"
#endif

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#ifndef WIN32
#include "wrapper/routingkey.h"
#else
#include "routingkey.h"
#endif

using namespace qpid;
using namespace std;

#define MAX_RETRIES   3
#define MAX_WAIT      1000000
#define MAX_RAND      5
#define MAX_SP_BUFFER 256


// ---------------------------------------------------------------------------
//
// Internal Wrapper class to send qpid messages
//
// ---------------------------------------------------------------------------
class SP_DLL_EXPORT ConsumerWrapper 

{
    public:

    ConsumerWrapper();
    ConsumerWrapper(const char *ipAddress, int portNumber, 
                    const char *user=NULL, const char *password=NULL, 
                    const char *mode="tcp");
    ~ConsumerWrapper();

    // entry points
   
    int          closeAMQPConnection();
    int          createAMQPConnection(const char *ipAddress, int portNumber, 
                                      const char *user=NULL, 
                                      const char *password=NULL, 
                                      const char *mode="tcp");
    int          createConsumer(std::string exchange, std::string routing_key);
    int          deleteConsumer(std::string exchange, std::string routing_key);

    int          bindQueue(std::string queue, std::string exchange, 
                           std::string routing_key);
    int          listen() ;
    virtual void received(std::string & content ,std::string & routingkey) = 0;

    void         getMessageData(qpid::messaging::Message& message,
                                string& content,string& routingkey);
    int          declareQueue(std::string queue, bool exclusive = false){ return SP_SUCCESS;}
    int          subscribeQueue(std::string queue){ return SP_SUCCESS;} 

private:
    void        *config_;
    bool         initialized_;
    qpidWrapper *qpidWrapper_;
    bool         connection_closed_;

};

#endif
