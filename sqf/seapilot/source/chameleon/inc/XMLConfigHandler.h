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

#ifndef _XML_CONFIG_HANDLER_
#define _XML_CONFIG_HANDLER_

#include <string>

class XMLConfigHandler;
class XMLConfigError;

#define DECLARE_ACTION_MAP  static _Action actionMap_[]; \
    virtual _Action * getActionMap_() { return actionMap_; }

#define BEGIN_ACTION_MAP(HDLR) _Action HDLR::actionMap_[] = {

#define ADD_ACTION( HDLR, key, start, end ) { std::string(key), (_HANDLES)&HDLR::start, (_HANDLEE)&HDLR::end},
#define ADD_START_ACTION(HDLR, key, start) { std::string(key), (_HANDLES)&HDLR::start, NULL},
#define ADD_END_ACTION(HDLR, key, end) { std::string(key), NULL, (_HANDLEE)&HDLR::end},

#define END_ACTION_MAP {"", NULL, NULL} };

//! inner usage
typedef void (XMLConfigHandler:: *_HANDLES)(std::string const&, std::string const&);
typedef void (XMLConfigHandler:: *_HANDLEE) (std::string const&);

struct _Action
{
    std::string key;
    _HANDLES start;
    _HANDLEE end;
};

//! base class to do configs
class XMLConfigHandler
{
    public:

        //! \brief called when a config is started and not registered in the action map
        //! \param key      config key
        //! \param value    config value
        virtual void handleStart(std::string const&key, std::string const&value);

        //! \brief called when a config is not in the action map and it's end
        //! \param key      config key
        virtual void handleEnd(std::string const&key);

        //! \brief handle config errors a configurer meets
        virtual void handleError( XMLConfigError const& error );

        //! \brief test if configging is stopped
        bool isStopped( ) { return stopped_; }

        //! \brief stop configuration, that is, stops running of XMLConfigurer
        void stop() { stopped_ = true; }

        XMLConfigHandler( ): stopped_(false) { }

        DECLARE_ACTION_MAP

    protected:

        bool stopped_;
};

//! When XMLConfigurer meets with some error/failure in the config media,
//! a XMLConfigError is filed into user-defined XMLConfigHandler
class XMLConfigError
{

    public:
        enum
        {
            WARNING = 1,
            ERROR = 2,
            FATALERROR = 3
        };
        
        //XMLConfigError( int level, char const*file, int line, int column,  char const* msg):level_(level), file_(file), line_(line), column_(column), msg_(msg){}

        XMLConfigError(int level, std::string const& file, int line, int column, std::string const& msg):level_(level),file_(file),line_(line),column_(column), msg_(msg){}

        //! \brief get the error level
        int getLevel() const { return level_; }

        //! \brief get xml file name
        std::string getFile() const { return file_; }

        //! \brief get the line number 
        int getLine() const { return line_; }

        //! \brief get the column number
        int getColumn() const { return column_; }

        //! \brief get the error message
        std::string getMessage() const { return msg_; }

    private:
        int level_;
        std::string file_;
        int line_;
        int column_;
        std::string msg_;
};

#endif

