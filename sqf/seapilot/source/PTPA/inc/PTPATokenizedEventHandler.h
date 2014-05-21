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

#ifndef PTPA_TOKENIZED_EVENT_HANDLER_H_
#define PTPA_TOKENIZED_EVENT_HANDLER_H_

#include "MalTrinityErrorStack.h"
#include "PTPAMessageTextParameters.h"
#include "MalMessageTextCatalog.h"
#include "common.text_event.pb.h" 
#include "wrapper/routingkey.h"
#include "common/spptLogger.h"

// Forward References

namespace google
{
    namespace protobuf
    {
        class Message;
        class Descriptor;
    }
};


//! The class PTPATokenizedEventHandler is a specialization that processes
//! a tokenized event and creating and republishing a corresponding text event.
class PTPATokenizedEventHandler
{
public: 
    //! Constructor
    //!
    //! \param [in] ip - The ip to publish the text event.
    //! \param [in] port - The port to publish the text event.
    //! \param [in] catalog - The message catalog wrapper.
    //! \param [in] configMap - The routingkey to table name map read from xml file.
    PTPATokenizedEventHandler(const std::string &ip, int port,
                          Trinity::MalMessageTextCatalog & catalog,
                          std::map<std::string, std::string> &configMap);

    //! Destructor
    ~PTPATokenizedEventHandler(void);

    //! \brief processes a received message
    //!
    //! \param [in] m - a pointer to the message.
    //! \param [in] routingkey - The routingkey of the coming tokenized event.
    //! \return true if the translation suceed, false if not.
    bool processMsg(const google::protobuf::Message *m, const std::string &routingKey);

private:        
    //! message catalog wrapper
    Trinity::MalMessageTextCatalog & messageCatalog_;

    //! parameters object for substituting parameters into message text
    PTPAMessageTextParameters parameters_;

    //! content type needed by sendAMQPMessage
    std::string contentType_;

    //! routing key object needed by sendAMQPMessage
    AMQPRoutingKey outboundRoutingKey_;

    //! protocol buffer for republishing event as a text event
    common::text_event textEvent_;

    //! tokenized event routingkey and repositorytable map
    std::map<std::string, std::string> repositoryTableMap_;

    //! error variable for log error
    spptErrorVar errVar_;
};

#endif

