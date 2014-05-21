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

#ifndef _MAL_MESSAGE_TEXT_CATALOG_
#define _MAL_MESSAGE_TEXT_CATALOG_

#include <string>
#include <nl_types.h>
#include <stdint.h>

#ifndef MAL_NO_PTHREADS
#include <pthread.h>
#endif


namespace Trinity {

    // forward reference
    class MalMessageTextParameters;

    //! The class MalMessageCatalog provides message catalog and message
    //! text parameter substitution services.
    class MalMessageTextCatalog
    {
    public:

        //! Constructor
        //! \param [in] messageCatalogName is the name of the message catalog
        MalMessageTextCatalog(const std::string & messageCatalogName);

        //! Destructor
        ~MalMessageTextCatalog(void);

        //! obtains the message text for a given message number, and performs
        //! parameter substitution into it
        //! \param [in] messageNumber is the message number
        //! \param [in] params is an object that given a parameter name returns the 
        //! parameter value in string form
        //! \return the text with parameters substituted
        std::string getMessageText(int32_t messageNumber,MalMessageTextParameters * params);

    private:

        //! catalog descriptor used by Linux catopen/catgets/catclose APIs
        nl_catd messageCatalogDescriptor_;

#ifndef MAL_NO_PTHREADS
        //! mutex used to access message catalog (note that catgets API is not
        //! thread safe, so such a mutex is required in a multi-threaded program
        //! when multiple threads need access)
        pthread_mutex_t messageCatalogMutex_;
#endif

    };

}

#endif  // _MAL_MESSAGE_TEXT_CATALOG_
