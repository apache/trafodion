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

#include <qpid/client/Connection.h>
#include <qpid/client/Session.h>
#include <qpid/client/AsyncSession.h>
#include <qpid/client/Message.h>
#include <qpid/client/MessageListener.h>
#include <qpid/client/SubscriptionManager.h>

#ifndef WIN32
#include <syslog.h>
#endif


#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/text_format.h>

#ifdef SEAQUEST_PROCESS
#include "seabed/ms.h"
#endif

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <list>

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

class sub_wrapper
{
public:
	sub_wrapper(qpid::client::Session* session) : subscriptions(*session){};
	~sub_wrapper(){};

	void run() {subscriptions.run();}
	void subscribe(client::MessageListener &listener, const std::string &queue) {subscriptions.subscribe(listener, queue);}

	client::SubscriptionManager subscriptions;
};

// class will hold an exchange/binding_key pairs
class LIST_ELEM
{
    public:
    LIST_ELEM(std::string &exchange, std::string &key)
              {exchange_name = exchange; binding_key = key;}
    ~LIST_ELEM(){}
    std::string exchange_name;
    std::string binding_key;
};

// class hold hold a queue name with a list of exchange/binding_key pairs
class QUEUES
{
    public:
    QUEUES(std::string &name, bool exclusive) 
       {queue_name = name; 
        queue_exclusive=exclusive;}
    ~QUEUES() {list.clear();}

    std::string queue_name;
    bool queue_exclusive;
    std::list<LIST_ELEM> list;
};

// --------------------------------------------------------------------------------------
//
// Internal Wrapper class to send qpid messages
//
// -------------------------------------------------------------------------------------
class SP_DLL_EXPORT ConsumerWrapper : public client::MessageListener

{
    public:

    ConsumerWrapper();
    ConsumerWrapper(const char *ipAddress, int portNumber, 
		            const char *user=NULL, const char *password=NULL, const char *mode="tcp");
    ~ConsumerWrapper();

    // entry points
    int           bindQueue(std::string queue, std::string exchange,
                            std::string routing_key);
    int           closeAMQPConnection();
    int           createAMQPConnection(const char *ipAddress, int portNumber, 
		                               const char *user=NULL, const char *password=NULL, const char *mode="tcp");
    int           declareQueue(std::string queue, bool exclusive = false); 
    const char *  session_name();
    int           listen() ;
    virtual void  received(client::Message& message) = 0;
    int           subscribeQueue(std::string queue);
	bool          get_queue_name(const char *subscription_key, char *queue_name, unsigned int queue_length);

    private:
    int          addToQueue(std::string &queue, std::string &exchange, std::string &key);
    int          recoverSession();

private:
    void *                       config_;
    bool                         initialized_;
    void *                       qpidWrapper_;
    bool                         recoverymode_;
    sub_wrapper                  *subscriptions_;
	bool                         connection_closed_;
    std::list <QUEUES>           queue_keys_;
};

#endif
