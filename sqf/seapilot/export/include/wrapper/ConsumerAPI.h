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

#ifndef __CONSUMERAPI_H
#define __CONSUMERAPI_H

#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Session.h>

#include "intConsWrap.h"

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

// --------------------------------------------------------------------------------------
//
// Wrapper class for clients to extend.  The only method that the client should implement
// is the received method below.  
//
// -------------------------------------------------------------------------------------
class SP_DLL_EXPORT ConsumerListener :  public ConsumerWrapper
{
    public:

    ConsumerListener();
    ConsumerListener(const char *ipAddress, int portNumber, 
		             const char *user=NULL, const char *password=NULL, const char *mode="tcp");
    ~ConsumerListener();

    // entry points
    int  bindQueue(std::string queue, std::string exchange,
                   std::string routing_key);
    int  deleteConsumer(std::string exchange, std::string routing_key);
    int  closeAMQPConnection();
    int  createAMQPConnection(const char *ipAddress, int portNumber, 
		                      const char *user=NULL, const char *password=NULL, const char *mode="tcp");
    int  declareQueue(std::string queue, bool exclusive=false); 
    int  listen() ;
    int  subscribeQueue(std::string queue);

    // client must implement the below method
    //change the parameters to hide qpid::messaging::message
    virtual void  received(std::string & content ,std::string &routingkey) = 0;
    void getMessageData(qpid::messaging::Message& message,string & content,string &routingkey);
};

#endif
