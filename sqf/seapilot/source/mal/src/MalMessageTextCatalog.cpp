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

#include "MalMessageTextParameters.h"
#include "MalMessageTextCatalog.h"

#include <ctype.h>  // to get isalpha, isdigit

using namespace Trinity;

// Constructor
// \param [in] messageCatalogName is the name of the message catalog
MalMessageTextCatalog::MalMessageTextCatalog(const std::string & messageCatalogName) 
: messageCatalogDescriptor_(nl_catd(-1))
{
    // open the message catalog (returns -1 if open fails)
    // Note: using 0 for the second argument causes catgets to fail for
    // unknown reasons in the Universal Consumer; using 1 seems to work
    // for equally unknown reasons
    messageCatalogDescriptor_ = catopen(messageCatalogName.c_str(),1);

#ifndef MAL_NO_PTHREADS
    // initialize mutex needed for catgets access
    pthread_mutex_init(&messageCatalogMutex_, NULL);
#endif
}

// Destructor
MalMessageTextCatalog::~MalMessageTextCatalog(void)
{
    // close the message catalog if it is open
    if (nl_catd(-1) != messageCatalogDescriptor_)
    {
        catclose(messageCatalogDescriptor_);
        messageCatalogDescriptor_ = nl_catd(-1);
    }

#ifndef MAL_NO_PTHREADS
    // destroy mutex needed for catgets access
    pthread_mutex_destroy(&messageCatalogMutex_);
#endif
}

// obtains the message text for a given message number, and performs
// parameter substitution into it
// \param [in] messageNumber is the message number
// \param [in] params is an object that given a parameter name returns the 
// parameter value in string form
// \return the text with parameters substituted
std::string MalMessageTextCatalog::getMessageText(int32_t messageNumber,MalMessageTextParameters * params)
{
    // try to obtain message text from the catalog (using the default text
    // if we fail)

    const char * defaultMessage = "Message text unavailable.";

    if (params)
    {
        defaultMessage = params->defaultMessage();
    }

    const char * message = defaultMessage;
    if (nl_catd(-1) != messageCatalogDescriptor_)
    {
#ifndef MAL_NO_PTHREADS
        // catgets is not thread-safe, so in multi-threaded programs we must guard
        // access with a lock
        pthread_mutex_lock(&messageCatalogMutex__);
#endif

        message = catgets(messageCatalogDescriptor_, 
                          1 /* set number */, 
                          messageNumber, 
                          defaultMessage);  

#ifndef MAL_NO_PTHREADS
        pthread_mutex_unlock(&messageCatalogMutex__);
#endif        
    }

    // if we have params, perform parameter substitution on the message text

    std::string result;
    if (params)
    {
        // parameter syntax: $~<parameter name>, where <parameter name> is a letter followed
        // by any mixture of letters, digits, underscores and interior dots (we don't allow
        // a dot to be the last character)
  
        // whether parameter names are case-sensitive or not is determined by the
        // MalMessageTextParameters object

        const char * currentSegment = message;
        const char * next = message;

        while (*next)
        {
            if (('$' == *next) && ('~' == next[1]) && (isalpha(next[2])))
            {
               // append previous verbatim segment to result (if there is one)
               if (next > currentSegment)
               {
                   result.append(currentSegment,next-currentSegment);
               }

               // extract the parameter name
               size_t identifierCount = 1;  // already know that identifier[0] is an alpha
               const char * identifier = next+2;  // step over $~
               while ((isalnum(identifier[identifierCount])) || 
                      ('_' == identifier[identifierCount]) ||
                      ('.' == identifier[identifierCount]))
               {
                   identifierCount++;
               }

               // don't allow a dot to be the last character
               while ((identifierCount > 0) && ('.' == identifier[identifierCount-1]))
               {
                   identifierCount--;
               }

               std::string parameterName;
               parameterName.append(identifier,identifierCount);

               // obtain the substitution value for it and append to result
               std::string parameterValue = params->getParameterValue(parameterName);

               if (parameterValue.size() > 0)
               {
                   result += parameterValue;
               }

               // advance next and currentSegment pointers
               next = identifier + identifierCount;
               currentSegment = next;
            }
            else
            {
                next++;  // step over verbatim character
            }
        }

        // append any remaining verbatim segment to result
        if (next > currentSegment)
        {
            result.append(currentSegment,next-currentSegment);
        }
    }
    else
    {
        // no params object supplied; just return message without substitutions
        result = message;
    }

    return result;
}
