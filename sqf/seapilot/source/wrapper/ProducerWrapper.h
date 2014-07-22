// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef __PRODUCERWRAPPER_H
#define __PRODUCERWRAPPER_H

#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Session.h>
#include <qpid/messaging/Message.h>

#ifndef WIN32 //TRW
#include <syslog.h>
#endif

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/text_format.h>

#include "wrapper/routingkey.h"
#include "qpidwrapper.h"
#include "config.h"

#ifdef SEAQUEST_PROCESS
#include "seabed/ms.h"
#endif

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

using namespace qpid;
using namespace qpid::messaging;
using std::stringstream;
using std::string;

#define MAX_RETRIES   3
#define MAX_WAIT      1000000
#define MAX_RAND      5
#define MAX_SP_BUFFER 256

// --------------------------------------------------------------------------------------
//
// Wrapper class to send qpid messages
//
// -------------------------------------------------------------------------------------
class ProducerWrapper
{
    public:

    ProducerWrapper();
    ~ProducerWrapper();

    // entry points
    int  closeAMQPConnection();
    int  createAMQPConnection(const char *ipAddress, int portNumber,
                                         const char *user=NULL, const char *password=NULL, 
                                         const char *mode="tcp");
    void initHeaderFields (void *header, int comonentId, int pid = -1, uint tid = 0, time_t time=-1);
    int  initAMQPHeader (void *header, int componentId, const void *field);
    int  initAMQPInfoHeader (void *header, int componentId, const void *field);
    int  setAMQPHeaderSequenceNumber (void *header);
    int  setAMQPInfoHeaderSequenceNumber (void *header);
    int  sendAMQPMessage(bool retry, const std::string& messageText, 
                     const std::string& contentType,
                     AMQPRoutingKey& routingKey, std::string& exchange, bool async = false);
    int  syncAMQPMessages();

    private:

    int  getSystemConfig();

    spConfig        config_;
    pthread_mutex_t mutex;
    qpidWrapper     qpidWrapper_;
    int             info_retries_;

};

#endif
