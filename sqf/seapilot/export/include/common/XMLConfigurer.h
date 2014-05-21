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

#ifndef _XML_CONFIGURER_
#define _XML_CONFIGURER_

#include <string>
#include "XMLConfigHandler.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
using namespace xercesc;

//! Base class for any configurers
class XMLConfigurer
{
    public:

        //! \brief used to add/re-define actions
        template<typename SF, typename EF>
        void registerAction(std::string const& key, SF start = NULL, EF end = NULL)
        {
            addNode(key, (_HANDLES)start, (_HANDLEE)end );
        }

        //! \brief constructor
        //! \param cb    a user-defined config handler
        //! \param file  xml config file
        XMLConfigurer(XMLConfigHandler *cb, std::string const& file);

        ~XMLConfigurer();

        //! \brief start XML configuration
        //! \return 0 on success; 1 on failure
        virtual int run( );


    protected:

        //! node structure in the configuration tree
        struct ConfigNode
        {
            _HANDLES start;
            _HANDLEE end;
            std::string key;
            ConfigNode *left;
            ConfigNode *right;
            ConfigNode(): start(NULL), end(NULL), left(NULL), right(NULL){}
        };

        //! return the node whose key equals to the given parameter
        ConfigNode* findNode(std::string const& key );

        //! entry method to add callbacks
        void addNode(std::string const& key, _HANDLES start, _HANDLEE end );

        void callStart( ConfigNode* node, std::string const& value );
        void callEnd( ConfigNode* node );
        void callStart(std::string const& key, std::string const& value );
        void callEnd(std::string const& key);

    private:
        void delNode(ConfigNode *node);

        //! inner class to catch exceptions from dom parser
        class XMLErrorHandler: public ErrorHandler
    {
        public:
            XMLErrorHandler(XMLConfigurer* xc): xc_(xc){}
            void warning(SAXParseException const& ex);
            void error(SAXParseException const& ex);
            void fatalError(SAXParseException const& ex);
            void resetErrors();
        private:
            void handleError(int level, SAXParseException const& ex);
            XMLConfigurer *xc_;
    };
    friend class XMLErrorHandler;

        bool buildNextNode();

        bool initialized_;
        void initialize();

        std::string file_;
        std::string currentKey_;
        std::string currentValue_;

        XercesDOMParser *domParser_;
        DOMNode * root_;
        DOMNode * current_;
        int currentState_;

        XMLErrorHandler xeh_;

        XMLConfigHandler *xch_;

        ConfigNode *head_;
};

#endif
