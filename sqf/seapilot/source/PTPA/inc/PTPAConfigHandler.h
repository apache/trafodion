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

#ifndef PTPA_CONFIG_HANDLER_H_
#define PTPA_CONFIG_HANDLER_H_

#include <string>
#include <XMLConfigHandler.h>
#include "common/spptLogger.h"

class PTPAConfigHandler: public XMLConfigHandler
{
public:
    //! Constructor
    PTPAConfigHandler() : error_(false) {}
    
    //! Deconstructor
    virtual ~PTPAConfigHandler();

    //! This function is invoked when publication map start during reading xml.
    //! \param [in] key - xml tag name.
    //! \param [in] value - xml element value.
    void sPublicationMap( std::string const& key, std::string const& value );

    //! This function is invoked when publication map end during reading xml.
    //! \param [in] key - xml tag name.
    void ePublicationMap( std::string const& key );

    //! Error handling function
    //! \param [in] key - xml tag name.
    //! \param [in] value - xml element value.
    virtual void handleStart(std::string const&key, std::string const&value);

    //! Error handling function
    //! \param [in] key - xml tag name.
    virtual void handleEnd(std::string const&key);

    //! Error handling function
    //! \param [in] error - config error during parsing xml.
    virtual void handleError( XMLConfigError const& error );

    //! Get function
    //! return error if failed during parsing xml.
    inline bool hasError() const { return error_; }

    //! Get function
    //! return routing key.
    const std::vector<std::string> & getRoutingKeys() const;

    //! Get function
    //! return routingkey and table name mapping.
    const std::map<std::string, std::string> & getConfigMap() const;

private:
    std::vector<std::string> routingKeysExternal_;
    std::map<std::string, std::string> configMap_;
    std::string amqpRoutingKey_;
    std::string repoTableName_;
    bool error_;

    DECLARE_ACTION_MAP

    spptErrorVar errVar_;
};

#endif
