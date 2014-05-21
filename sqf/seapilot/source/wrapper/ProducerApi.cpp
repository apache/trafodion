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
#include <errno.h>

#include "qpidwrapper.h"
#include "wrapper/amqpwrapper.h"
#include "common/sp_errors.h"
#include "common/evl_sqlog_eventnum.h"
#include "ProducerWrapper.h"

ProducerWrapper gv_ProducerWrapper;

/***************************************************************************
/ This file implementes the C style user API.  We will eventually turn this
/ into a class when time permits and we want to force users to change their
/ usage
*****************************************************************************/

// ----------------------------------------------------------------------
//
// createConnection
// Purpose - call wrapper to establish a connection
//
// ----------------------------------------------------------------------
SP_DLL_EXPORT int closeAMQPConnection ()
{
    return gv_ProducerWrapper.closeAMQPConnection();
}
// ----------------------------------------------------------------------
//
// createConnection
// Purpose - call wrapper to establish a connection
//
// ----------------------------------------------------------------------
SP_DLL_EXPORT int createAMQPConnection(const char *ipAddress, int portNumber,
                                       const char *user, const char *password, const char *mode)
{
  return gv_ProducerWrapper.createAMQPConnection(ipAddress, portNumber, user, password, mode);
}

// -----------------------------------------------------------------------
//
// initQpidHeader
// Purpose : Call wrapper to initialize the Header fields
//
// -----------------------------------------------------------------------
SP_DLL_EXPORT int initAMQPHeader (void *header, int componentId)
{
    return gv_ProducerWrapper.initAMQPHeader(header, componentId, NULL);
}

// -----------------------------------------------------------------------
//
// initQpidHeader, reflection flavor
// Purpose : Call wrapper to initialize the Header fields
//
// -----------------------------------------------------------------------
SP_DLL_EXPORT int initAMQPHeaderReflecting (void *header, int componentId, const void *field)
{
    return gv_ProducerWrapper.initAMQPHeader(header, componentId, field);
}
// ----------------------------------------------------------------------
//
// initAMQPInfoHeader
// Purpose : init Info and Qpid headers
//
// ---------------------------------------------------------------------
SP_DLL_EXPORT int initAMQPInfoHeader (void *header, int componentId)
{
    return gv_ProducerWrapper.initAMQPInfoHeader(header, componentId, NULL);
}

// -----------------------------------------------------------------------
//
// initAMQPInfoHeader, reflection flavor
// Purpose : init Info and Qpid headers
//
// -----------------------------------------------------------------------
SP_DLL_EXPORT int initAMQPInfoHeaderReflecting (void *header, int componentId, const void *field)
{
    return gv_ProducerWrapper.initAMQPInfoHeader(header, componentId, field);
}


// -----------------------------------------------------------------------
//
// setAMQPHeaderSequenceNumber
// Purpose : Call wrapper to set sequence number in a (reused) header
//
// -----------------------------------------------------------------------
SP_DLL_EXPORT int setAMQPHeaderSequenceNumber (void *header)
{
    return gv_ProducerWrapper.setAMQPHeaderSequenceNumber(header);
}

// ----------------------------------------------------------------------
//
// initAMQPInfoHeader
// Purpose : Set sequence number in (reused) Info and Qpid headers
//
// ---------------------------------------------------------------------
SP_DLL_EXPORT int setAMQPInfoHeaderSequenceNumber (void *header)
{
    return gv_ProducerWrapper.setAMQPInfoHeaderSequenceNumber(header);
}

// -----------------------------------------------------------------------
//
// sendAMQPMessageExchange
// Purpose : Call wrapper to send a message
//
// -----------------------------------------------------------------------
SP_DLL_EXPORT int sendAMQPMessageExchange(bool retry, const std::string& messageText, const std::string& contentType,
                    AMQPRoutingKey& routingKey, std::string &exchange, bool async, void* protoMessage)
{
    int retcode = SP_SUCCESS;        //  assume success
#ifndef MONITOR_PROCESS
    retcode =  gv_ProducerWrapper.sendAMQPMessage( retry, messageText, contentType,
                                           routingKey, exchange, async);
#endif
    return retcode;
}


// -----------------------------------------------------------------------
//
// sendQpidMessage
// Purpose : Call wrapper to send a message
//
// -----------------------------------------------------------------------
SP_DLL_EXPORT int sendAMQPMessage(bool retry, const std::string& messageText, const std::string& contentType,
                    AMQPRoutingKey& routingKey, bool async, void* protoMessage)
{
    int retcode = SP_SUCCESS;        //  assume success
#ifndef MONITOR_PROCESS
    std::string exchange = "amq.topic";
    retcode =  gv_ProducerWrapper.sendAMQPMessage( retry, messageText, contentType,
                                           routingKey, exchange, async);
#endif
    return retcode;
}

// ----------------------------------------------------------------------
//
// syncQpidMessages
// Purpose : sync async messages
//
// -----------------------------------------------------------------------
SP_DLL_EXPORT int syncAMQPMessages()
{
    return gv_ProducerWrapper.syncAMQPMessages();
}


