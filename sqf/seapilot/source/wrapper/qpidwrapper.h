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
#ifndef __QPIDWRAPPER_H
#define __QPIDWRAPPER_H

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/types/Variant.h>

#ifndef WIN32 
#include <syslog.h>
#else
#include "../wrapper_win/pthread_win.h"
#endif


#define MAX_RETRIES   3
#define MAX_WAIT      1000000
#define MAX_RAND      5
#define MAX_SP_BUFFER 256


#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/text_format.h>

#include "wrapper/routingkey.h"

#ifdef SEAQUEST_PROCESS
#include "seabed/ms.h"
#endif

using namespace std;
using namespace qpid;
using namespace qpid::messaging;
using namespace qpid::types;

const string address_option(";{create: always, node:{type:topic}}");
const qpid::messaging::Duration RECEIVER_TIMEOUT=qpid::messaging::Duration::FOREVER;
const uint32_t CAPACITY=5000;   //Currently force it to default value :5000.

// --------------------------------------------------------------------------------------
//
// Wrapper class to send/receive qpid messages 
//
// -------------------------------------------------------------------------------------
class qpidWrapper
{
    public:

    qpidWrapper();
    ~qpidWrapper();

    // shared stuff
    int  closeQpidConnection();
    int  createQpidConnection(const char *ipAddress, int portNumber, 
                              const char *user=NULL, const char *password=NULL, 
                              const char *mode="tcp");
   
    // producer stuff
    void initHeaderFields (void *header, int comonentId, int pid = -1, uint tid = 0, time_t time=-1);
    int  initQpidHeader (void *header, int componentId, unsigned int instanceId, std::string& ipaddr, 
                        unsigned int host_id, const void *field
#ifdef SEAQUEST_PROCESS
                         ,  char *processName, int nodeId, int pnodeId
#endif  
);  
    int  initInfoHeader (void *header, int componentId, unsigned int instanceId, std::string& ipaddr, 
                         unsigned int host_id, const void *field
#ifdef SEAQUEST_PROCESS
                         ,  char *processName, int nodeId, int pnodeId
#endif 
);
    int  setQpidHeaderSequenceNumber (void *header);
    int  setInfoHeaderSequenceNumber (void *header);
    int  sendMessage(bool retry, const std::string& messageText, 
                     const std::string& contentType,
                     AMQPRoutingKey& routingKey, std::string &exchange, bool async = false);
    int syncMessages();

    //consumer stuff
    //helper methods
    void set_iPAddress(const char *ip)          { if (ip)   strcpy(iPAddress_, ip);}
    void set_portNumber(int pn) {portNumber_ = pn;}
    int createConsumer(std::string exchange,std::string routingkey,uint32_t capacity=CAPACITY);
    int  deleteConsumer(std::string exchange,std::string routingkey);
    //This method can work for multiple Consumers/receivers.
    bool retrieveNextMessage(qpid::messaging::Message &message,qpid::messaging::Duration timeout=RECEIVER_TIMEOUT);
    void getMessageData(qpid::messaging::Message& message,string & content,string &routingkey);
    
    private:

    pthread_mutex_t mutex;

    // connection related
    bool                      activeConnection;
    bool                      activeSession;
    bool                      activeSender;
    qpid::messaging::Connection connection;
    qpid::messaging::Session    session;
    qpid::messaging::Sender  sender;
    qpid::messaging::Receiver receiver_;
    Variant::Map receiver_map_;
    	
    unsigned long       sequenceNumber_;
    
    char                      iPAddress_[MAX_SP_BUFFER];
    int                       portNumber_;
 
    //private helper methods
    int createConnection(bool retry, const char *ipAddress = NULL, int portNumber = -1,
		                 const char *user = NULL, const char *password = NULL, const char *mode = "tcp");
    int createSession ();
    int createProducer(std::string &exchange);
    string createAddress(std::string exchange,std::string routingkey);
    void acknowledge(qpid::messaging::Message &message);
    void acknowledge();
};

#endif
