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

#include <qpid/client/Connection.h>
#include <qpid/client/Session.h>
#include <qpid/client/AsyncSession.h>
#include <qpid/client/Message.h>
#include <qpid/client/QueueOptions.h>

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

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

using namespace qpid;
using std::stringstream;
using std::string;


// --------------------------------------------------------------------------------------
//
// Wrapper class to send qpid messages
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
    qpid::client::Session* get_session() {return session;} 
   
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
    int bindQueue(std::string queue, std::string exchange, std::string &bindingKey);
    int declareQueue(std::string queue, bool exclusive, int message_depth=0);

    //helper methods
    void set_iPAddress(const char *ip) 
          { if (ip) 
               strcpy(iPAddress, ip);}
    void set_portNumber(int pn) {portNumber = pn;}
    void set_mode(const char *md)
          { if (md)
               strcpy(mode, md);}

    private:

    pthread_mutex_t mutex;

    // connection related
    bool                      activeConnection;
    bool                      activeSession;
    qpid::client::Connection* connection;
    qpid::client::Session*    session;
    unsigned long             sequenceNumber_;
    
    char                      iPAddress[20];
    int                       portNumber;
    char                      mode[20];

 
    //private helper methods
    int createConnection(bool retry, bool allocateMem, const char *ipAddress = NULL, int portNumber = -1,
		                 const char *user = NULL, const char *password = NULL, const char *mode = "tcp");
    int createSession ();
};

#endif
