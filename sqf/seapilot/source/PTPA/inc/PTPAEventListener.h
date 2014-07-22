// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef PTPA_EVENT_LISTENER_H_
#define PTPA_EVENT_LISTENER_H_

#include <string>
#include <ConsumerAPI.h>

#include "PTPATokenizedEventHandler.h"

struct PTPAArguments
{
    std::string    subBrokerIP;
    int            subBrokerPort;
    std::string    pubBrokerIP;
    int            pubBrokerPort;
    std::string    configFile;
    std::string    textCatalog;
    std::string    protoSrc;
    std::string    logging;
    std::string    logFile;
    std::string    queueName;
    bool           verbose;
};

//! This class listen the event from subscribed broker and process it.
class PTPAEventListener: public ConsumerListener
{
public:
    //! Constructor
    //! \param [in] ip - The ip to subscribe.
    //! \param [in] port - The port to subscribe.
    //! \param [in] pubIp - The ip to publish text event.
    //! \param [in] pubPort - The port to publish text event.
    //! \param [in] messageCatalog - The message catalog wrapper.
    //! \param [in] protoSrc - Directory containing *.proto files.
    //! \param [in] configMap - The routingkey to table name mapping read from xml.
    PTPAEventListener(const std::string &ip, int port,
                      const std::string &pubIp, int pubPort,
                      Trinity::MalMessageTextCatalog & messageCatalog,
                      const std::string &protoSrc,
                      std::map<std::string, std::string> &configMap,
                      bool verbose);

    //! Deconstructor
    virtual ~PTPAEventListener();

public:
    //! This function receives the event from qpid broker.
    //! \param [in] message - The message received.
    virtual void received(std::string & content ,std::string &routingkey);

    //! Creates and binds to an exclusive queue.
    //! \param [in] queue - Declared queue name.
    //! \param [in] exchange - Exchange name.
    //! \param [in] routing_keys - The routing keys for each message.
    //! return 0 if succeed.
    int prepareQueue(const std::string &queue, const std::string &exchange, std::vector<std::string> &routing_keys);

private:
    //! Translate the tokenized message to text message.
    // \param [in] message - Message received.
    // \return true if the translation suceed, false if not.
    bool processMsg(std::string & content ,std::string &routingkey);
    
private:
    PTPATokenizedEventHandler eventHandler_;

    google::protobuf::compiler::DiskSourceTree sourceTree_;
    google::protobuf::compiler::Importer importer_;
    google::protobuf::DynamicMessageFactory messageFactory_;

    bool verbose_;
};

#endif
