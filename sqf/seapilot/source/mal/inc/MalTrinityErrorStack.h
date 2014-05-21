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

#ifndef _MAL_TRINITY_ERRORSTACK_
#define _MAL_TRINITY_ERRORSTACK_

#include <string>
#include <vector>
#include <stdint.h>
#include "MalError.h"

namespace Trinity{

using namespace std;
//! forward declaration
class MalError;

//! The class MalTrinityErrorStack holds a list of errors
//! MalTrinityErrorStack allocates new errors.
class MalTrinityErrorStack
{
public:
    //! constructor for MalTrinityErrorStack
    MalTrinityErrorStack(void);
    //! destructor for MalTrinityError
    ~MalTrinityErrorStack(void);
    //! copy constructor for MalTrinityErrorStack
    //! param [in] mtes MalTrinityErrorStack
    MalTrinityErrorStack(const MalTrinityErrorStack &mtes);
    //! overloaded = operator for MalTrinityErrorStack
    //! param [in] mtes const reference to MalTrinityErrorStack
    //! returns reference to MalTrinityErrorStack
    MalTrinityErrorStack& operator =(const MalTrinityErrorStack &mtes);
    //! creates a new MalError stores it in a list
    //! \param [in] errorNumber is an enum of error numbers
    //! \param [in] severity (e.g. SQ_LOG_ERR, SQ_LOG_WARNING, etc.)
    void addError(ErrorCode errorNumber, int32_t severity);
    //! creates a new MalError stores it in a list
    //! \param [in] errorNumber is an enum of error numbers
    //! \param [in] severity (e.g. SQ_LOG_ERR, SQ_LOG_WARNING, etc.)
    //! \param [in] errStr1 is a string
    //! \param [in] errStr1Type is the token type of errStr1
    void addError(ErrorCode errorNumber, int32_t severity,
                                        const string& errStr1,int32_t errStr1Type);
    //! creates a new MalError stores it in a list
    //! \param [in] errorNumber is an enum of error numbers
    //! \param [in] severity (e.g. SQ_LOG_ERR, SQ_LOG_WARNING, etc.)
    //! \param [in] errStr1 is a string
    //! \param [in] errStr1Type is the token type of errStr1
    //! \param [in] errStr2 is a string
    //! \param [in] errStr2Type is the token type of errStr2
    void addError(ErrorCode errorNumber, int32_t severity,
                                        const string& errStr1,int32_t errStr1Type,
                                        const string& errStr2,int32_t errStr2Type);
    //! creates a new MalError stores it in a list
    //! \param [in] errorNumber is an enum of error numbers
    //! \param [in] severity (e.g. SQ_LOG_ERR, SQ_LOG_WARNING, etc.)
    //! \param [in] errStr1 is a string
    //! \param [in] errStr1Type is the token type of errStr1
    //! \param [in] errStr2 is a string
    //! \param [in] errStr2Type is the token type of errStr2
    //! \param [in] errStr3 is a string
    //! \param [in] errStr3Type is the token type of errStr3
    void addError(ErrorCode errorNumber, int32_t severity,
                                        const string& errStr1,int32_t errStr1Type,
                                        const string& errStr2,int32_t errStr2Type,
                                        const string& errStr3,int32_t errStr3Type);
    //! creates a new MalError stores it in a list
    //! \param [in] errorNumber is an enum of error numbers
    //! \param [in] severity (e.g. SQ_LOG_ERR, SQ_LOG_WARNING, etc.)
    //! \param [in] errStr1 is a string
    //! \param [in] errStr1Type is the token type of errStr1
    //! \param [in] errStr2 is a string
    //! \param [in] errStr2Type is the token type of errStr2
    //! \param [in] errStr3 is a string
    //! \param [in] errStr3Type is the token type of errStr3
    //! \param [in] errStr4 is a string
    //! \param [in] errStr4Type is the token type of errStr4
    void addError(ErrorCode errorNumber, int32_t severity,
                                        const string& errStr1,int32_t errStr1Type,
                                        const string& errStr2,int32_t errStr2Type,
                                        const string& errStr3,int32_t errStr3Type,
                                        const string& errStr4,int32_t errStr4Type);
    //! creates a new MalError stores it in a list
    //! \param [in] errorNumber is an enum of error numbers
    //! \param [in] severity (e.g. SQ_LOG_ERR, SQ_LOG_WARNING, etc.)
    //! \param [in] errStr1 is a string
    //! \param [in] errStr1Type is the token type of errStr1
    //! \param [in] errStr2 is a string
    //! \param [in] errStr2Type is the token type of errStr2
    //! \param [in] errStr3 is a string
    //! \param [in] errStr3Type is the token type of errStr3
    //! \param [in] errStr4 is a string
    //! \param [in] errStr4Type is the token type of errStr4
    //! \param [in] errStr5 is a string
    //! \param [in] errStr5Type is the token type of errStr5
    void addError(ErrorCode errorNumber, int32_t severity,
                                        const string& errStr1,int32_t errStr1Type,
                                        const string& errStr2,int32_t errStr2Type,
                                        const string& errStr3,int32_t errStr3Type,
                                        const string& errStr4,int32_t errStr4Type,
                                        const string& errStr5,int32_t errStr5Type);
    //! removes an arbitrary error from the list specified by parameter passed
    //! \param [in] entry is an unsigned 16 bit integer
    void removeError(uint16_t entry);
    //! sets the error number of any arbitrary error specified by parameter
    //! \param [in] entry is an unsigned 16 bit integer
    //! \param [in] err is an ErrorCode
    void setErrorCode(uint16_t entry,ErrorCode err);
    //! sets the string of any arbitrary error specified by parameter
    //! \param [in] entry is an unsigned 16 bit integer
    //! \param [in] errStr is a string
    //! \param [in] errStrType is the token type of errStr
    //! \param [in] strNumber is an unsigned 16 bit integer
    void setErrorStr(uint16_t entry,string errStr,int32_t errStrType,uint16_t strNumber);
    //! returns the number of items of severity SQ_LOG_ERR or greater stored in the list
    //! \return an unsigned 16 bit integer
    uint16_t errorCount() const;
    //! returns the number of items stored in the list
    //! \return an unsigned 16 bit integer
    uint16_t itemCount() const;
    //! clears the entire stack and deletes all errors
    void clear();
    //! returns the error code of any arbitrary error specified by parameters
    //! \param [in] entry is an unsigned 16 bit integer
    //! \return an ErrorCode
    ErrorCode getErrorCode(uint16_t entry) const;
    //! returns the error code of the last item added to the list
    //! return an ErrorCode
    ErrorCode getErrorCodeTop();
    //! removes the top error code, if one exists
    //! \return the popped error code if it existed, or ERR_GETTING_ERRCODE
    ErrorCode popErrorCodeTop();
    //! returns the severity of any arbitrary error specified by parameters
    //! \param [in] entry is an unsigned 16 bit integer
    //! \return an ErrorCode
    int32_t getSeverity(uint16_t entry) const;
    //! returns the severity of the last item added to the list
    //! return an ErrorCode
    int32_t getSeverityTop();
    //! returns a string from any arbitrary error in the list
    //! \param [in] entry is an unsigned 16 bit integer
    //! \param [in] strNumber is an unsigned 16 bit integer
    string getErrorStr(uint16_t entry,uint16_t strNumber) const;
    //! returns the token type of a string from any arbitrary error in the list
    //! \param [in] entry is an unsigned 16 bit integer
    //! \param [in] strNumber is an unsigned 16 bit integer
    int32_t getErrorStrType(uint16_t entry,uint16_t strNumber) const;
    //! adds the errors inside an error stack to the current stack
    //! \param [in] errStack is a stack of errors
    //! \return void
    void addErrors(const MalTrinityErrorStack &errStack);
    //! checks to see if all errors in the MalTrinityErrorStack are one
    //! particular error code
    //! \param [in] errorCode is the error code to check for
    //! \return true if all errors in the stack are that error code (or if the stack is empty),
    //! false if there exists an error code in the stack different from the input error code
    bool allErrorsAre(ErrorCode errorCode);
    //! Concatenates all error strings for a given entry into a single character array of 
    //! null-terminated strings. Also returns the error code.
    //! \param[in] entry the number of the error stack entry to be returned
    //! \param[out] errorNumber a pointer to an ErrorCode to be populated with the error code
    //! \param[out] errorStrings a sequence of null-terminated strings containing the error detail
    //! \param[in] bufferLength the length of \a errorStrings 
    //! \param[out] bytesNeeded the number of bytes needed to return all of the information
    //! \param[out] numberOfStrings the number of strings returned in \a errorStrings
    //! \return OK if successful, else truncation warning and \a bytesNeeded is updated, or AOOB
    //! meaning there is no such entry in the error stack.
    ErrorCode getErrorDetail(uint16_t entry, ErrorCode *errorNumber, char *errorStrings, uint32_t bufferLength, 
        uint32_t *bytesNeeded, uint32_t *numberOfStrings);

    //! Prints out the error stack (used primarily for debugging)
    void printStackContents();
    
private:

    //! count of items in errorContainer that have severity SQ_LOG_ERR or greater
    uint16_t errorCount_;

    //! vector of *MalErrors to store all errors
    vector<MalError *> errorContainer;
};
}

#endif // _MAL_TRINITY_ERRORSTACK_
