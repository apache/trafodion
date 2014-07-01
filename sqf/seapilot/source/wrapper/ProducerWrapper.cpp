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
#ifndef WIN32
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "common/common.info_header.pb.h"
#else
#include "win/common.info_header.pb.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qpidwrapper.h"
#include "wrapper/amqpwrapper.h"
#include "common/evl_sqlog_eventnum.h"
#include "ProducerWrapper.h"


// public methods

ProducerWrapper::ProducerWrapper()
{
    pthread_mutex_init(&mutex, NULL);
    info_retries_ = 0;
}

ProducerWrapper::~ProducerWrapper()
{
    pthread_mutex_destroy(&mutex);
    // don't need to close connection, the qpidWrapper destructor does that.
}

// ----------------------------------------------------------------------
//
// createConnection
// Purpose - call wrapper to establish a connection
//
// ----------------------------------------------------------------------
int ProducerWrapper::closeAMQPConnection ()
{
    // get configuration if we don't already have it
    int error = getSystemConfig();

    if (error == SP_DISABLED)
       return SP_SUCCESS;

    if (!error)  
       error = qpidWrapper_.closeQpidConnection();

    return error;
}
// ----------------------------------------------------------------------
//
// createConnection
// Purpose - call wrapper to establish a connection
//
// ----------------------------------------------------------------------
int ProducerWrapper::createAMQPConnection(const char *ipAddress, int portNumber,
                                         const char *user, const char *password, 
                                         const char *mode)
{
    if (ipAddress != NULL)
        config_.set_iPAddress(ipAddress);
    if (portNumber != -1)
        config_.set_portNumber(portNumber);
    if (mode != NULL)
        config_.set_mode(mode);

    // get configuration if we don't already have it
    int error = getSystemConfig();  

    if (error == SP_DISABLED)
       return SP_SUCCESS;

    if (!error)  
			error =  qpidWrapper_.createQpidConnection(config_.get_iPAddress(),  config_.get_portNumber(),
                                                 user, password, mode);

    return error;
}

// -----------------------------------------------------------------------
//
// initQpidHeader
// Purpose : Call wrapper to initialize the Header fields
//
// -----------------------------------------------------------------------
int ProducerWrapper::initAMQPHeader (void *header, int componentId, const void *field)
{
    int error;

    if ((header == NULL) || (componentId < 0))
        return SP_BAD_PARAM;

    // get configuration if we don't already have it
    error = getSystemConfig();  

    if (error == SP_DISABLED)
       return SP_SUCCESS;

    if (error) 
       return error;

#ifndef MONITOR_PROCESS
#ifdef SEAQUEST_PROCESS

    if (((config_.get_pnodeId() == -1) || (config_.get_nodeId() == -1)) && (info_retries_++ < MAX_CONFIG_RETRIES))
    {
       config_.getSeaquestConfig();
    }
#endif
#endif

    std::string ipaddr = config_.get_sIpaddress();
    unsigned int host_id = config_.get_host_id();
    return qpidWrapper_.initQpidHeader(header, componentId, config_.get_instanceId(), ipaddr, host_id, field
#ifdef SEAQUEST_PROCESS
                                       , config_.get_processName(), config_.get_nodeId(), 
                                         config_.get_pnodeId()
#endif
);
}

// ----------------------------------------------------------------------
//
// initAMQPInfoHeader
// Purpose : init Info and Qpid headers
//
// ---------------------------------------------------------------------
int ProducerWrapper::initAMQPInfoHeader (void *header, int componentId, const void *field)
{
    int error;

    if ((header == NULL) || (componentId < 0))
        return SP_BAD_PARAM;

    // get configuration if we don't already have it
   error = getSystemConfig();  

    if (error == SP_DISABLED)
       return SP_SUCCESS;
    else if (error)
       return error; 

    std::string ipaddr = config_.get_sIpaddress();
    unsigned int host_id = config_.get_host_id();
    return qpidWrapper_.initInfoHeader(header, componentId, config_.get_instanceId(), ipaddr, host_id, field
#ifdef SEAQUEST_PROCESS
                                       , config_.get_processName(), config_.get_nodeId(),
                                         config_.get_pnodeId()
#endif
);
}

// -----------------------------------------------------------------------
//
// setAMQPHeaderSequenceNumber
// Purpose : Call wrapper to set sequence number in a (reused) header
//
// -----------------------------------------------------------------------
int ProducerWrapper::setAMQPHeaderSequenceNumber (void *header)
{
    int error;

    if (header == NULL)
        return SP_BAD_PARAM;

    // get configuration if we don't already have it
    error = getSystemConfig();  

    if (error == SP_DISABLED)
       return SP_SUCCESS;

    if (!error)   
       error = qpidWrapper_.setQpidHeaderSequenceNumber(header);

    return error;
}

// ----------------------------------------------------------------------
//
// initAMQPInfoHeader
// Purpose : Set sequence number in (reused) Info and Qpid headers
//
// ---------------------------------------------------------------------
int ProducerWrapper::setAMQPInfoHeaderSequenceNumber (void *header)
{
    int error;

    if (header == NULL)
        return SP_BAD_PARAM;

    // get configuration if we don't already have it
    error = getSystemConfig();  

    if (error == SP_DISABLED)
       return SP_SUCCESS;

    if (!error)   
      error = qpidWrapper_.setInfoHeaderSequenceNumber(header);

    return error;
}


// -----------------------------------------------------------------------
//
// sendQpidMessage
// Purpose : Call wrapper to send a message
//
// -----------------------------------------------------------------------
int ProducerWrapper::sendAMQPMessage(bool retry, const std::string& messageText, 
                     const std::string& contentType, AMQPRoutingKey& routingKey,
                     std::string& exchange, bool async)
{
    int error = getSystemConfig();

    if (error == SP_DISABLED)
       return SP_SUCCESS;  
    else if (error)
       return error;

#ifndef MONITOR_PROCESS
       error =  qpidWrapper_.sendMessage( retry, messageText, contentType,
                                           routingKey, exchange, async);
#endif

    return error;
}

// ----------------------------------------------------------------------
//
// syncQpidMessages
// Purpose : sync async messages
//
// -----------------------------------------------------------------------
int ProducerWrapper::syncAMQPMessages()
{
    // get configuration if we don't already have it
    int error = getSystemConfig();

    if (error == SP_DISABLED)
       return SP_SUCCESS;
  
    if (!error)
        error = qpidWrapper_.syncMessages();

    return error;
}

int ProducerWrapper::getSystemConfig()
{
    // get configuration if we don't already have it
    if (!config_.isActiveConfig())
    {
        pthread_mutex_lock( &mutex );
        int configStatus = config_.getConfig();
        pthread_mutex_unlock( &mutex );

        if (configStatus != SP_SUCCESS)
           return SP_CONFIG_NOT_AVAILABLE;

        qpidWrapper_.set_iPAddress (config_.get_iPAddress());
        qpidWrapper_.set_portNumber(config_.get_portNumber());
    }
    
    if (config_.get_spDisabled())
        return SP_DISABLED;
        
    return SP_SUCCESS;
}
