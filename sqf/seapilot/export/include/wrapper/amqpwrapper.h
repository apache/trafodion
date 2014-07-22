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

#ifndef __AMQPWRAPPER_H
#define __AMQPWRAPPER_H

#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Session.h>
#include <qpid/messaging/Message.h>

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "routingkey.h"

using namespace qpid;
using namespace qpid::messaging;
using std::stringstream;
using std::string;

// ------------------------------------------------------------------------
//
// public qpid wrapper header file
// Purpose : expose a small set of functionality for users to use qpid
//           to send messages
//
// -------------------------------------------------------------------------
SP_DLL_EXPORT int   closeAMQPConnection();
SP_DLL_EXPORT int   createAMQPConnection(const char *ipAddress = NULL, int portNumber = -1,
                                         const char *user=NULL, const char *password=NULL, 
                                         const char *mode="tcp");
SP_DLL_EXPORT int   initAMQPHeader (void *header, int componentId);
SP_DLL_EXPORT int   initAMQPHeaderReflecting (void *header, int componentId, const void *field);
SP_DLL_EXPORT int   initAMQPInfoHeader (void *header, int componentId);
SP_DLL_EXPORT int   initAMQPInfoHeaderReflecting (void *header, int componentId, const void *field);
SP_DLL_EXPORT int   setAMQPHeaderSequenceNumber (void *header);
SP_DLL_EXPORT int   setAMQPInfoHeaderSequenceNumber (void *header);
SP_DLL_EXPORT int   sendAMQPMessage(bool retry, const std::string& messageText, 
                      const std::string& ContentType,
                      AMQPRoutingKey &routingKey,
                      bool async = false,
                      void* protoMessage = NULL);
SP_DLL_EXPORT int   syncAMQPMessages();

#endif
