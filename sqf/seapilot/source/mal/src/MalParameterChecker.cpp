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

#include "MalParameterChecker.h"

#include <limits>
#include <sstream>

#include <string.h>
#include <ctype.h>


using namespace Trinity;

// Constructor

MalParameterChecker::MalParameterChecker(void) 
{
    // find the maximum unsigned integer value and save as a string
    std::stringstream converter;
    converter << std::numeric_limits<unsigned int>::max();
    maxUnsignedInt_ = converter.str();
}


// Destructor

MalParameterChecker::~MalParameterChecker(void)
{
    // nothing to do
}


const char * MalParameterChecker::canConvertToUnsignedInt(int argc, 
    char** argv, 
    const char * parameterName)
{
    const char * rc = 0;  // assume the parameter is absent (in which case it is good)

    // search for the desired parameter in the argv array

    int i = 0;
    const char * parameterValue = 0;
    size_t parameterNameLength = strlen(parameterName);
    while ((i < argc) && (0 == parameterValue))
    {
        if (strncmp(argv[i],parameterName,parameterNameLength) == 0)
        {
            // might have found it; if followed by '=' or null we have it
            if (argv[i][parameterNameLength] == '=')
            {
                parameterValue = argv[i] + parameterNameLength + 1;
            }
            else if ((argv[i][parameterNameLength] == '\0') && (i + 1 < argc))
            {
                parameterValue = argv[i+1];  // there is a space between parm name and its value
            }
        }
        i++;
    }

    if (parameterValue)
    {
        // parameter is present, see if it can be converted
        if (!canConvertStringToUnsignedInt(parameterValue))
        {
            rc = parameterValue;  // return a pointer to the bad string
        }
    }

    return rc;            
}

bool MalParameterChecker::canConvertStringToUnsignedInt(const char * str)
{   
    bool rc = false;  // assume the string cannot be converted
    const char * next = str;

    // verify that the string is all digits, with a possible leading '+' sign

    if (*next == '+')
    {
        next++;
    }

    const char * numberStart = next;  // remember where digits start
    
    while (isdigit(*next))
    {
        next++;
    }

    if (*next == '\0')
    {
        // the string is all digits, with possible leading '+'
  
        // next verify that the string is a value that fits in an unsigned int

        // skip any leading zeroes

        while (*numberStart == '0')
        {
            numberStart++;
        }

        if (strlen(numberStart) < maxUnsignedInt_.size())
        {
            // the number has fewer digits than the maximum unsigned integer,
            // therefore it will fit
            rc = true; 
        }
        else if ((strlen(numberStart) == maxUnsignedInt_.size()) && 
                 (strcmp(numberStart,maxUnsignedInt_.c_str()) <= 0))
        {
            // the number has the same number of digits as the maximum 
            // unsigned integer and is less than or equal to it, so it will
            // fit
            rc = true; 
        }
    } 
     
    return rc;  
}

