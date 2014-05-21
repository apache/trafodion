// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef _MAL_ERROR_
#define _MAL_ERROR_

#include <string>
#include <stdint.h>

#include "common/evl_sqlog_eventnum.h"  // to get ErrorCode enum
#include "sqevlog/evl_sqlog_writer.h"   // for SQ_LOG_ERR etc. constants used in addError()
#include "MalTrinityException.h"
#include "MalTrinityErrorStack.h"

enum { MAL_ERROR_MAX_PARAMETERS = 5 };

namespace Trinity{

using namespace std;

// forward declarations
class MalTrinityException;
class MalTrinityErrorStack;

//! The class MalError contains an enum ErrorCode and four strings
class MalError
{
public:
    //! constructor for MalError sets the error number and initializes all 4 strings to empty quotes
    MalError(ErrorCode err, int32_t severity);

    //! constructor for MalError sets the error number and string[0] initializes remaining 3 strings to empty quotes
    MalError(ErrorCode err, int32_t severity, string errStr1, int32_t errStr1Type);

    //! constructor for MalError sets the error number and string[0],string[1] initializes remaining 2 strings to empty quotes
    MalError(ErrorCode err, int32_t severity,
                            string errStr1, int32_t errStr1Type,
                            string errStr2, int32_t errStr2Type);

    //! constructor for MalError sets the error number and string[0],string[1],string[2] initializes remaining string to empty quotes
    MalError(ErrorCode err, int32_t severity,
                            string errStr1, int32_t errStr1Type,
                            string errStr2, int32_t errStr2Type,
                            string errStr3, int32_t errStr3Type);

    //! constructor for MalError sets the error number and string[0],string[1],string[2],string[3]
    MalError(ErrorCode err, int32_t severity,
                            string errStr1, int32_t errStr1Type,
                            string errStr2, int32_t errStr2Type,
                            string errStr3, int32_t errStr3Type,
                            string errStr4, int32_t errStr4Type);

    //! constructor for MalError sets the error number and string[0],string[1],string[2],string[3],string[4]
    MalError(ErrorCode err, int32_t severity,
                            string errStr1, int32_t errStr1Type,
                            string errStr2, int32_t errStr2Type,
                            string errStr3, int32_t errStr3Type,
                            string errStr4, int32_t errStr4Type,
                            string errStr5, int32_t errStr5Type);

    //! Method to set the error number
    //! \param [in] err an enum of error numbers
    //! \return void
    void setErrorNumber(ErrorCode err);

    //! Method to get the error number
    //! \return error number
    ErrorCode getErrorNumber();

    //! Method to get severity
    int32_t getSeverity();

    //! Method sets an arbitrary string [0-3] depending on parameter passed
    //! \param [in] errStr a string
    //! \param [in] errStrType is the token type of the string
    //! \param [in] strNumber an unsigned 8bit integer
    //! \return void
    void setErrorStr(string errStr, int32_t errStrType, uint16_t strNumber);

    //! Method that returns an arbitrary string [0-3] depending on parameter passed
    //! \param[in] strNumber unsigned 16 bit integer
    //! \return a string
    string getErrorStr(uint16_t strNumber);

    //! Method that returns the token type for a string [0-3] depending on parameter passed
    //! \param[in] strNumber unsigned 16 bit integer
    //! \return a string
    int32_t getErrorStrType(uint16_t strNumber);

    //! destructor for MallError
    ~MalError(void);

    //! Concatenates all error strings into a single character array of null-terminated strings.
    //! \param[out] errorStrings a sequence of null-terminated strings containing the error strings
    //! \param[in] bufferLength the length of \a errorStrings
    //! \param[out] bytesNeeded the number of bytes needed to return all of the information
    //! \param[out] numberOfStrings the number of strings returned
    //! If \a bytesNeeded is greater than \a bufferLength, then not all information could be returned
    void getErrorStrings(char *errorStrings, uint32_t bufferLength, uint32_t *bytesNeeded,
                               uint32_t *numberOfStrings);


private:
    //! enum ErrorCode defined in common header
    ErrorCode errorNumber_;

    //! severity (as defined by SQ_LOG_* defines in sqevlog/evl_sqlog_writer.h)
    int32_t severity_;

    //! token type information corresponding to strings in errorStr_
    int32_t errorStrType_[MAL_ERROR_MAX_PARAMETERS];

    //! string array of four optional error parameters
    string errorStr_[MAL_ERROR_MAX_PARAMETERS];
};
}

#endif // _MAL_ERROR_

