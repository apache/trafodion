// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef _MAL_PARAMETER_CHECKER_
#define _MAL_PARAMETER_CHECKER_

#include <string>

namespace Trinity {

    //! The class MalParameterChecker provides parameter checking services. It
    //! was written to compensate for undesireable changes introduced in Boost 1.47;
    //! for example Boost now allows negative numbers to be assigned to unsigned
    //! integers.
    class MalParameterChecker
    {
    public:

        //! Constructor
        MalParameterChecker(void);

        //! Destructor
        ~MalParameterChecker(void);

        //! locates the value of the desired parameter in argv and checks that
        //! it can be converted to an unsigned int
        //! \return null if so (or if parameter is absent), a pointer to the
        //! (bad) parameter value if not
        const char * canConvertToUnsignedInt(int argc, 
           char** argv, const char * parameterName);

    private:

        //! determines if the input string can be converted to an unsigned int
        //! \return true if so false if not
        bool canConvertStringToUnsignedInt(const char * str);

        //! keeps a string containing the string representation of the maximum
        //! unsigned integer value
        std::string maxUnsignedInt_;
    };

}

#endif  // _MAL_PARAMETER_CHECKER_
