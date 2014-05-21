// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

#include "PTPAConfigHandler.h"
#include "common/evl_sqlog_eventnum.h"
#include "spptLogger.h"

namespace {
    const char* _publicationMap = "/publicationConfiguration/publicationMap";
    const char* _amqpRoutingKey = "/publicationConfiguration/publicationMap/amqpRoutingKey";
    const char* _repoTableName = "/publicationConfiguration/publicationMap/repositoryTableName";
}

BEGIN_ACTION_MAP(PTPAConfigHandler)
    ADD_ACTION(PTPAConfigHandler, _publicationMap, sPublicationMap, ePublicationMap)
    ADD_START_ACTION(PTPAConfigHandler, _amqpRoutingKey, sPublicationMap)
    ADD_START_ACTION(PTPAConfigHandler, _repoTableName, sPublicationMap)
END_ACTION_MAP

PTPAConfigHandler::~PTPAConfigHandler()
{
    routingKeysExternal_.clear();
    configMap_.clear();
}

void PTPAConfigHandler::sPublicationMap(string const& key, string const& value)
{
    if(key.compare(_publicationMap) == 0)
    {
        amqpRoutingKey_.clear();
        repoTableName_.clear();
    }
    else if(key.compare(_amqpRoutingKey) ==0)
    {
        amqpRoutingKey_ = value;
    }
    else if(key.compare(_repoTableName) == 0) 
    {
        string::size_type pos = value.find_last_of('.');
        if (pos != string::npos)
            repoTableName_ = value.substr(pos + 1);
    }
    return ;
}

void PTPAConfigHandler::ePublicationMap(string const& key)
{
    if(amqpRoutingKey_.size() > 0 && repoTableName_.size() > 0)
    {
        routingKeysExternal_.push_back(amqpRoutingKey_);
        configMap_.insert(make_pair(amqpRoutingKey_, repoTableName_));
    }
    else 
    {
    }
    return ;
}

void PTPAConfigHandler::handleStart(std::string const&key, std::string const&value)
{
    errVar_.reset();
    errVar_.setErrorString(std::string("Invalid ").append(key));
    logError(PTPA_XML_PARSING_EXCEPTION,errVar_);
    error_ = true;
    stop();
}

void PTPAConfigHandler::handleEnd(std::string const&key)
{
    errVar_.reset();
    errVar_.setErrorString(std::string("Invalid ").append(key));
    logError(PTPA_XML_PARSING_EXCEPTION,errVar_);
    error_ = true;
    stop();
}

void PTPAConfigHandler::handleError(XMLConfigError const& error )
{
    if(error.getLevel() == XMLConfigError::FATALERROR)
    {
        errVar_.reset();
        errVar_.setErrorString(error.getMessage());
        logError(PTPA_XML_PARSING_EXCEPTION,errVar_);
        error_ = true;
        stop();
    }
}

const std::vector<std::string> & PTPAConfigHandler::getRoutingKeys() const
{
    return routingKeysExternal_;
}

const std::map<std::string, std::string> & PTPAConfigHandler::getConfigMap() const
{
    return configMap_;
}
