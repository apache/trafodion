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

#include "common/sp_errors.h"
#include "common/evl_sqlog_eventnum.h"
#include "wrapper/intConsWrap.h"

// -------------------------------------------------------------------------
//
// ConsumerWrapper::ConsumerWrapper : constructor, creates session
//
// -------------------------------------------------------------------------
ConsumerWrapper::ConsumerWrapper() 
{
    connection_closed_ = false;
    initialized_ =  false;
    config_ = new spConfig();
    qpidWrapper_ = new qpidWrapper();

   if ((!config_)  ||  (!qpidWrapper_))
   {
       initialized_ = false;
       return;
    }
 
    int error = createAMQPConnection(NULL, -1, NULL, NULL, "tcp");

    if (error)
       initialized_ = false;
    else
       initialized_ = true;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::ConsumerWrapper : constructor, creates session
//
// -------------------------------------------------------------------------
ConsumerWrapper::ConsumerWrapper(const char *ipAddress, int portNumber, 
								 const char *user, const char *password, const char *mode) 
{
    int error = SP_SUCCESS;

    connection_closed_ = false;
    initialized_ = false;
    config_ = new spConfig();
    qpidWrapper_ = new qpidWrapper();

    if ((!config_)  ||  (!qpidWrapper_))
    {
       initialized_ = false;
       return;
    }

    if ((ipAddress) && (portNumber > 0))
        error = createAMQPConnection (ipAddress, portNumber, user, password, mode);
    else
        error = SP_BAD_PARAM;

    if (error)
       initialized_ = false;
    else
       initialized_ = true;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::~ConsumerWrapper : destructor
//
// -------------------------------------------------------------------------
ConsumerWrapper::~ConsumerWrapper()
{
   if(!connection_closed_)    closeAMQPConnection();
	// why does this cause an exception??
    
    if (config_)
        delete (spConfig *)config_;
    if (qpidWrapper_)
        delete qpidWrapper_;   
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::closeAMQPConnection - close connection
//
// -------------------------------------------------------------------------
int  ConsumerWrapper::closeAMQPConnection()
{ 
  
    // not connected if we aren't initialized, its a no-op
    if ((!initialized_) || (!((spConfig*)config_)->get_activeConfig()) ||
       (((spConfig*)config_)->get_spDisabled()))
       return SP_SUCCESS;
	connection_closed_ = true;
    return qpidWrapper_->closeQpidConnection();
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::createAMQPConnection - create connection using given ip/port
//
// -------------------------------------------------------------------------
int  ConsumerWrapper::createAMQPConnection(const char *ipAddress, int portNumber, 
										   const char *user, const char *password, const char *mode)
{
    int configStatus = SP_SUCCESS; 
    int error = SP_SUCCESS;

    connection_closed_ = false;
    if (ipAddress != NULL)
        ((spConfig*)config_)->set_iPAddress (ipAddress);
    if (portNumber > 0)
        ((spConfig*)config_)->set_portNumber(portNumber);
    if (mode != NULL)
        ((spConfig*)config_)->set_mode      (mode);

    // get configuration if we don't already have it
    if (!((spConfig*)config_)->get_activeConfig())
    {
        configStatus = ((spConfig*)config_)->getConfig();
        if ((configStatus != SP_SUCCESS) && (configStatus != SP_DISABLED))
            return SP_CONFIG_NOT_AVAILABLE;

    }

    if (((spConfig*)config_)->get_spDisabled())
       return SP_SUCCESS;

    error = qpidWrapper_->createQpidConnection(((spConfig*)config_)->get_iPAddress(),  ((spConfig*)config_)->get_portNumber(),
		                                                         user, password, mode);
  
    // if connection failed in the constructor, consider us initialited 
    // if the connection succeeded here
    if ((!error) && (!initialized_))
	{
       initialized_ = true;
	}
    return error;
}
//To create a consumer ,don't need to call del
int ConsumerWrapper::createConsumer(std::string exchange, std::string routing_key)
{
	int  ret   = SP_SUCCESS;

    // in case the user passed in bad data and that is why we couldn't connect,
    // we don't want to keep retrying, so bounce it back up to them.
    if (!initialized_)
       return SP_CONNECTION_CLOSED;
    ret=qpidWrapper_->createConsumer(exchange,routing_key);
    return ret;
}
int ConsumerWrapper::deleteConsumer(std::string exchange, std::string routing_key)
{
	int  ret   = SP_SUCCESS;
	ret=qpidWrapper_->deleteConsumer(exchange,routing_key);     
	return ret;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::bindQueue - Remain bindQueue here for compatibility with the old qpid::client APIs
// Suggest new consumer call CreateConsumer method directly and dont need to call declareQueue and subscribeQueue
//
// -------------------------------------------------------------------------
int ConsumerWrapper::bindQueue(std::string queue, std::string exchange, std::string routing_key)
{
    return createConsumer(exchange,routing_key);
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::listen - listen for messages, thread blocking!!!
//
// -------------------------------------------------------------------------
int ConsumerWrapper::listen() 
{
    int ret = SP_SUCCESS;   
    qpid::messaging::Message message;  
    string content,routingkey;

    // in case the user passed in bad data and that is why we couldn't connect,
    // we don't want to keep retrying, so bounce it back up to them.
    if (!initialized_)
       return SP_CONNECTION_CLOSED;

    // Receive messages
   while (qpidWrapper_->retrieveNextMessage(message)) 
    {
   		qpidWrapper_->getMessageData(message,content,routingkey);   
	 	this->received(content,routingkey);		
   }; 
    return ret;
        }

//add this method to hide message.getContent();message.getSubject();
void ConsumerWrapper::getMessageData(qpid::messaging::Message& message,string & content,string &routingkey)
{
	 qpidWrapper_->getMessageData(message, content,routingkey);
	 return;
    }


