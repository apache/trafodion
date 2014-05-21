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

#include "PTPAEventListener.h"

#include <string>
#include <qpid/client/Message.h>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>

#include "common/spptLogger.h"
#include "sp_util.h"
#include "wrapper/routingkey.h"
#include "wrapper/externroutingkey.h"

using namespace qpid;
using namespace google;
using namespace std;
using namespace Trinity;

static seapilot::Stdout sp_cout;

PTPAEventListener::PTPAEventListener(const string &ip, int port, 
                                     const string &pubIp, int pubPort,
                                     Trinity::MalMessageTextCatalog & messageCatalog,
                                     const string &protoSrc,
                                     map<string, string> &configMap,
                                     bool verbose):
    ConsumerListener(ip.c_str(), port),
    eventHandler_(pubIp, pubPort, messageCatalog, configMap),
    importer_(&sourceTree_, NULL),
    messageFactory_(importer_.pool()),
    verbose_(verbose)
{
    sourceTree_.MapPath("", protoSrc);
}

PTPAEventListener::~PTPAEventListener()
{
}

void PTPAEventListener::received(client::Message& message)
{
    processMsg(message);
}

bool PTPAEventListener::processMsg(const client::Message &message)
{
    string routingKey = message.getDeliveryProperties().getRoutingKey();
    if (verbose_)
        sp_cout << "Received event with routing key: " << routingKey << endl;

    AMQPRoutingKey wrapperRoutingKey(routingKey);
    string publicationName = wrapperRoutingKey.GetAsMessageName();
    string protoFile = wrapperRoutingKey.GetAsProtofileName();
    const protobuf::FileDescriptor *fileDes = importer_.Import(protoFile);
    if (fileDes == 0)
    {
        string errString;
        errString = string("Proto file ") + protoFile + string(" doesn't exist.");
        spptErrorVar errVar;
        errVar.setErrorString(errString);
        logError(PTPA_PROTOBUF_ERROR, errVar);
        return false;
    }
    
    const protobuf::Descriptor *desc = importer_.pool()->FindMessageTypeByName(publicationName);
    const protobuf::Message *prototype = messageFactory_.GetPrototype(desc);
    protobuf::Message *msg = prototype->New();
    if(!msg->ParseFromString(message.getData()))
    {
        spptErrorVar errVar;
        errVar.setErrorString("Proto buffer parse from string error!");
        logError(PTPA_PROTOBUF_ERROR, errVar);
        delete msg;
        msg = NULL;
        return false;
    }
    bool ret = eventHandler_.processMsg(msg, routingKey);
    delete msg;
    msg = NULL;
    return ret;
}

int PTPAEventListener::prepareQueue(const string &queue, const string &exchange, vector<string> &routing_keys)
{
    int ret = SP_SUCCESS;
    spptErrorVar errVar;

    // prepare the incoming data queues
    ret = declareQueue(queue);
    if (ret != SP_SUCCESS)
    {
        char tembuf[128];
        memset(tembuf, 128,0);
        sprintf(tembuf,"Function declareQueue return error %d",ret);
        errVar.reset();   
        errVar.setErrorString(tembuf);
        logError(PTPA_QPID_ERROR, errVar);   
        return ret;
    }
 
    for(vector<string>::size_type i = 0; i < routing_keys.size(); i++)
    {
        AMQPRoutingKey routingKey;
        ExtRoutingKey_ToInternal(routing_keys[i], &routingKey);

        sp_cout << "Subscribing to routing key: " << routingKey.GetAsString() << endl;
        ret = bindQueue(queue, exchange, routingKey.GetAsString());
        if (ret != SP_SUCCESS)
        {
            char tembuf[128];
            memset(tembuf, 128,0);
            sprintf(tembuf,"Function bindQueue return error %d",ret);
            errVar.reset();
            errVar.setErrorString(tembuf);
            logError(PTPA_QPID_ERROR, errVar);
            return ret;
        }
    }

    // ------- Subscribe to messages on the queues we are interested in -------
    ret = subscribeQueue(queue);
    if (ret != SP_SUCCESS)
    {
        char tembuf[128];
        memset(tembuf, 128,0);
        sprintf(tembuf,"Function subscribeQueue return error %d",ret);
        errVar.reset();
        errVar.setErrorString(tembuf);
        logError(PTPA_QPID_ERROR, errVar);
        return ret;
    }
    return ret;
}

