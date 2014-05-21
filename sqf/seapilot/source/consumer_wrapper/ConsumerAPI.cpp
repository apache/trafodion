// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef WIN32 // TRW
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef WIN32
#include "common/common.info_header.pb.h"
#else
#include "win/common.info_header.pb.h"
#endif

#include "wrapper/intConsWrap.h"
#include "wrapper/ConsumerAPI.h"
#include "../wrapper/qpidwrapper.h"
#include "common/evl_sqlog_eventnum.h"

ConsumerListener::ConsumerListener()
{
//  Base constructor does it all
}

ConsumerListener::ConsumerListener(const char *ipAddress, int portNumber, 
								   const char *user, const char *password, const char *mode) :
  ConsumerWrapper (ipAddress, portNumber, user, password, mode)
{
//  Base constructor does it all
}

ConsumerListener::~ConsumerListener()
{
//  Base desstructor does it all
}

int  ConsumerListener::closeAMQPConnection()
{
    return ConsumerWrapper::closeAMQPConnection();
}

int  ConsumerListener::createAMQPConnection(const char *ipAddress, int portNumber, 
											const char *user, const char *password, const char *mode)
{
    return ConsumerWrapper::createAMQPConnection(ipAddress,  portNumber, user, password, mode);
}

int ConsumerListener::declareQueue(std::string queue, bool exclusive)
{
    return ConsumerWrapper::declareQueue(queue, exclusive);
}

int ConsumerListener::bindQueue(std::string queue, std::string exchange, std::string routing_key)
{

    return  ConsumerWrapper::bindQueue(queue, exchange, routing_key);
}

int ConsumerListener::subscribeQueue(std::string queue)
{

    return ConsumerWrapper::subscribeQueue(queue);
}
int ConsumerListener::listen() 
{
    return ConsumerWrapper::listen();
}

const char * ConsumerListener::get_session_name()
{
   return ConsumerWrapper::session_name();
}

bool ConsumerListener::get_queue_name(const char *subscription_key, char *queue_name, unsigned int queue_length)
{
	return ConsumerWrapper::get_queue_name (subscription_key, queue_name, queue_length);
}
