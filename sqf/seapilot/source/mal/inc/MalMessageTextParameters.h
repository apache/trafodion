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

#ifndef _MAL_MESSAGE_TEXT_PARAMETERS_
#define _MAL_MESSAGE_TEXT_PARAMETERS_

#include <string>
#include <nl_types.h>
#include <stdint.h>

namespace Trinity {

    //! The class MalMessageTextParameters is an abstract base class
    //! that provides default message text and wraps the interface to 
    //! obtain text substitution parameters
    class MalMessageTextParameters
    {
    public:

        //! Constructor
        MalMessageTextParameters(void);

        //! Destructor
        virtual ~MalMessageTextParameters(void);

        //! obtains default message text in case message text cannot be
        //! be found in the catalog
        //! \return the default message text
        virtual const char * defaultMessage(void) = 0;

        //! gets the parameter value associated with a given parameter name
        //! \param [in] parameterName is the name of the parmeter to find a value for
        //! \return the value associted with the parameter if found, an empty string otherwise
        virtual std::string getParameterValue(std::string & parameterName) = 0;

    private:

    };

}

#endif  // _MAL_MESSAGE_TEXT_PARAMETERS_
