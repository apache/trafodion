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

#include "common/evl_sqlog_eventnum.h"
#include "wrapper/intConsWrap.h"
#include "../wrapper/qpidwrapper.h"
#include "../wrapper/config.h"

// -------------------------------------------------------------------------
//
// ConsumerWrapper::ConsumerWrapper : constructor, creates session
//
// -------------------------------------------------------------------------
ConsumerWrapper::ConsumerWrapper() 
{
	connection_closed_ = false;
    subscriptions_ = NULL;
    initialized_ = recoverymode_ = false;
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
    subscriptions_ = NULL;
    initialized_ = recoverymode_ = false;
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
    closeAMQPConnection();
	// why does this cause an exception??
    if (subscriptions_)
        delete subscriptions_;
    if (config_)
        delete (spConfig *)config_;
    if (qpidWrapper_)
        delete (qpidWrapper *)qpidWrapper_;   
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
    return ((qpidWrapper *)qpidWrapper_)->closeQpidConnection();
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

	if (subscriptions_)
    {
        delete subscriptions_;
        subscriptions_ = NULL;
    }
	
	if (!recoverymode_)
		queue_keys_.clear();

    error = ((qpidWrapper *)qpidWrapper_)->createQpidConnection(((spConfig*)config_)->get_iPAddress(),  ((spConfig*)config_)->get_portNumber(),
		                                                         user, password, mode);
  
    // if the connection succeeded, get a session and set up the subscription manager
    if (!error)
    {
        client::Session* session = ((qpidWrapper *)qpidWrapper_)->get_session();
        if (session)
           subscriptions_ = new sub_wrapper(session);
        else
        {
           // an error being returned will alert the user that this method failed, hence
           // a failed connection in that this whole operation did not complete, 
           // so even if we successfully created the connection, 
           // close it to allow them to retry the full operation.
           closeAMQPConnection();
           error = SP_CONFIG_NOT_AVAILABLE;  // TODO CONNECTION_CLOSED
        }
    }
    
    // if connection failed in the constructor, consider us initialited 
    // if the connection succeeded here
    if ((!error) && (!initialized_))
	{
       initialized_ = true;
	}


    return error;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::declareQueue - declare a queue, and remember the queue name
//
// -------------------------------------------------------------------------
int ConsumerWrapper::declareQueue(std::string queue, bool exclusive)
{
    bool done    = false;
    int  error   = SP_SUCCESS;
    int  retries = 0;

    // in case the user passed in bad data and that is why we couldn't connect,
    // we don't want to keep retrying, so bounce it back up to them.
    if (!initialized_)
       return SP_CONNECTION_CLOSED;

    while ((!done) && (retries++ < MAX_RETRIES))
    {
        error = ((qpidWrapper *)qpidWrapper_)->declareQueue(queue, exclusive);
        if (!error)
        {
           // remember this as an active queue
           queue_keys_.push_back(QUEUES(queue, exclusive));
           done = true;
        } 
        else 
        {
           return error;
        }
    }

    return error;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::bindQueue - bind a routing key to a queue
//
// -------------------------------------------------------------------------
int ConsumerWrapper::bindQueue(std::string queue, std::string exchange, std::string routing_key)
{
    bool done    = false;
    int  error   = SP_SUCCESS;
    int  retries = 0;

    // in case the user passed in bad data and that is why we couldn't connect,
    // we don't want to keep retrying, so bounce it back up to them.
    if (!initialized_)
       return SP_CONNECTION_CLOSED;

    while ((!done) && (retries++ < MAX_RETRIES))
    {
        error = ((qpidWrapper *)qpidWrapper_)->bindQueue(queue, exchange, routing_key);
        if (!error)
        {
           // add exchange and routing_key to the queue's list
           error = addToQueue(queue, exchange, routing_key);
           if (error)
           {
               closeAMQPConnection();
               error = SP_CONNECTION_CLOSED;
           }
           done = true;     
        }
        else 
        {
           done = true; 
        }
    }

    return error;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::subscribeQueue - subscribe
//
// -------------------------------------------------------------------------
int ConsumerWrapper::subscribeQueue(std::string queue)
{
    int error = SP_SUCCESS;
    int retries = 0;
    bool done = false;

    // in case the user passed in bad data and that is why we couldn't connect,
    // we don't want to keep retrying, so bounce it back up to them.
    if (!initialized_)
       return SP_CONNECTION_CLOSED;

    // subscribe to the queue using the subscription manager.
    while ((!done) && (retries++ < MAX_RETRIES))
    {
        try
        {
           subscriptions_->subscribe(*this, queue);
           done = true;
        } catch (const std::exception &ex)
        {
            closeAMQPConnection();
            error = SP_CONNECTION_CLOSED;
			done = true;
        }        
    }

    return error;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::listen - listen for messages, thread blocking!!!
//
// -------------------------------------------------------------------------
int ConsumerWrapper::listen() 
{
    bool done = false;
    int error = SP_SUCCESS;
    int random = 0;

    // in case the user passed in bad data and that is why we couldn't connect,
    // we don't want to keep retrying, so bounce it back up to them.
    if (!initialized_)
       return SP_CONNECTION_CLOSED;

    // Receive messages
    while (!done)
    {
        try
        {
            subscriptions_->run();
            done = true;  // will it get here?
        }
        catch (const std::exception& ex)
        {
			if (connection_closed_) // by user
				done = true;
			else
			{
                 random = (rand()%(1+MAX_RAND))*100000; //sleep between 100 and 500 ms)
#ifndef WIN32
                 usleep(random);
#else
		         random = random/1000;  //micro to milli
		         Sleep(random);
#endif
                 closeAMQPConnection();

                 // if we can't recover, exit
                 error = recoverSession();
                 if (error)
                    done = true;
			}
        }
    }

    return error;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::recoverSession - create a new connection and rebuild
//                                   the queues
//
// -------------------------------------------------------------------------
int ConsumerWrapper::recoverSession()
{
    recoverymode_ = true;
    int done = false;
    int error = SP_SUCCESS;
    int random;
    int retries = 0;
  
    // Cannot recover SSL connections.
    if (!strcmp(((spConfig*)config_)->get_mode(), "ssl")) return SP_CONNECTION_CLOSED;

    while ((!done) && (retries++ < MAX_RETRIES))
    {
        random = (rand()%(1+MAX_RAND))*10000; //sleep between 10 and 50 ms)
#ifndef WIN32
        usleep(random);
#else
		random = random/1000;  //micro to milli
	    Sleep(random);
#endif
		// We don't store username, password, mode cannot recover.
    error = createAMQPConnection(((spConfig*)config_)->get_iPAddress(),
	                             ((spConfig*)config_)->get_portNumber(), NULL, NULL, "tcp");
		if (!error)
			done = true;
    }

    if (!error)
    {
        list<QUEUES>::iterator iter1 = queue_keys_.begin();
        // walk through the iterators of queues
        while(iter1 != queue_keys_.end()) 
        {
            std::string name = iter1->queue_name;
            bool exclusive = iter1->queue_exclusive;
            error = ((qpidWrapper *)qpidWrapper_)->declareQueue(name, exclusive);
            // now walk the list of routing keys associated with that queue
            if (!error)
            {
               list<LIST_ELEM>::iterator iter2 = iter1->list.begin();
               while(iter2 != iter1->list.end())
               {
                   error = ((qpidWrapper *)qpidWrapper_)->bindQueue(iter1->queue_name, iter2->exchange_name,
                                                  iter2->binding_key);
                   if (error)
                      break;
                   iter2++;
               }
            }
            // if we got through everyone ok, then subscribe the queue
            if (!error)
                error = subscribeQueue(iter1->queue_name);

            if (error)
               break;
            iter1++;
        }
    }  

    recoverymode_ = false;

    // just a little cleanup if we can't recover
    if (error)
    {
        closeAMQPConnection();
    }

    return error;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::session_name - get name of session
//
// -------------------------------------------------------------------------
const char * ConsumerWrapper::session_name()
{
   qpid::client::Session *session = ((qpidWrapper *)qpidWrapper_)->get_session();
   if (session)
   {
      std::string name = session->getId().getName();
      return name.c_str();
   }
   else 
      return NULL;
}

// -------------------------------------------------------------------------
//
// ConsumerWrapper::addToQueue - add a routing key to the queue's list
//
// -------------------------------------------------------------------------
int ConsumerWrapper::addToQueue(std::string &queue, std::string &exchange, std::string &key)
{
    bool done = false;
    list<QUEUES>::iterator iter = queue_keys_.begin();

    // first find the queue in our list, then add the info to it. 
    while((iter != queue_keys_.end()) & (!done)) 
    {
        if (iter->queue_name == queue)
        {
           iter->list.push_back(LIST_ELEM(exchange, key));
           done = true;
        }
        else
           iter++;
    }

    if (done == false)
    {
        queue_keys_.clear();
        return SP_BAD_PARAM;
    }

    return SP_SUCCESS;
}

bool ConsumerWrapper::get_queue_name(const char *subscription_key, char *queue_name, unsigned int queue_length)
{
	bool success = false;
	list<QUEUES>::iterator iter1 = queue_keys_.begin();

	while(iter1 != queue_keys_.end()) 
    {
       list<LIST_ELEM>::iterator iter2 = iter1->list.begin();
       while(iter2 != iter1->list.end())
       {
		   std::string key = iter2->binding_key;
		   if (strcmp(subscription_key, key.c_str()) == 0)
		   {
			   if (iter1->queue_name.length() < queue_length)
			   {
				   strcpy (queue_name, iter1->queue_name.c_str());
				   success = true;
				   break;
			   }
		   }
           iter2++;
        }
       iter1++;
    }
	return success;
}

