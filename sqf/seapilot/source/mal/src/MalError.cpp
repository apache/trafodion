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
#include "MalError.h"
#include <cstring>

using namespace Trinity;

//extern MalEnvironment & getMalEnv();

MalError::MalError(ErrorCode err,int32_t severity)
{
    severity_ = severity;
    errorStr_[0] = "";
    errorStr_[1] = "";
    errorStr_[2] = "";
    errorStr_[3] = "";
    errorStrType_[0] = 0;
    errorStrType_[1] = 0;
    errorStrType_[2] = 0;
    errorStrType_[3] = 0;
    setErrorNumber (err);  // debugging aid
}
MalError::MalError(ErrorCode err,int32_t severity,string errStr1,int32_t errStr1Type)
{
    severity_ = severity;
    errorStr_[0] = errStr1;
    errorStr_[1] = "";
    errorStr_[2] = "";
    errorStr_[3] = "";
    errorStrType_[0] = errStr1Type;
    errorStrType_[1] = 0;
    errorStrType_[2] = 0;
    errorStrType_[3] = 0;
    setErrorNumber (err);  // debugging aid
}
MalError::MalError(ErrorCode err,int32_t severity,
                                 string errStr1,int32_t errStr1Type,
                                 string errStr2,int32_t errStr2Type)
{
    severity_ = severity;
    errorStr_[0] = errStr1;
    errorStr_[1] = errStr2;
    errorStr_[2] = "";
    errorStr_[3] = "";
    errorStrType_[0] = errStr1Type;
    errorStrType_[1] = errStr2Type;
    errorStrType_[2] = 0;
    errorStrType_[3] = 0;
    setErrorNumber (err);  // debugging aid
}
MalError::MalError(ErrorCode err,int32_t severity,
                                 string errStr1,int32_t errStr1Type,
                                 string errStr2,int32_t errStr2Type,
                                 string errStr3,int32_t errStr3Type)
{
    severity_ = severity;
    errorStr_[0] = errStr1;
    errorStr_[1] = errStr2;
    errorStr_[2] = errStr3;
    errorStr_[3] = "";
    errorStrType_[0] = errStr1Type;
    errorStrType_[1] = errStr2Type;
    errorStrType_[2] = errStr3Type;
    errorStrType_[3] = 0;
    setErrorNumber (err);
}
MalError::MalError(ErrorCode err,int32_t severity,
                                 string errStr1,int32_t errStr1Type,
                                 string errStr2,int32_t errStr2Type,
                                 string errStr3,int32_t errStr3Type,
                                 string errStr4,int32_t errStr4Type)
{
    severity_ = severity;
    errorStr_[0] = errStr1;
    errorStr_[1] = errStr2;
    errorStr_[2] = errStr3;
    errorStr_[3] = errStr4;
    errorStrType_[0] = errStr1Type;
    errorStrType_[1] = errStr2Type;
    errorStrType_[2] = errStr3Type;
    errorStrType_[3] = errStr4Type;
    setErrorNumber (err);
}
MalError::MalError(ErrorCode err,int32_t severity,
                                 string errStr1,int32_t errStr1Type,
                                 string errStr2,int32_t errStr2Type,
                                 string errStr3,int32_t errStr3Type,
                                 string errStr4,int32_t errStr4Type,
                                 string errStr5,int32_t errStr5Type)
{
    severity_ = severity;
    errorStr_[0] = errStr1;
    errorStr_[1] = errStr2;
    errorStr_[2] = errStr3;
    errorStr_[3] = errStr4;
    errorStr_[4] = errStr5;
    errorStrType_[0] = errStr1Type;
    errorStrType_[1] = errStr2Type;
    errorStrType_[2] = errStr3Type;
    errorStrType_[3] = errStr4Type;
    errorStrType_[4] = errStr5Type;
    setErrorNumber (err);
}
void MalError::setErrorStr(string errStr, int32_t errStrType, uint16_t strNumber)
{
    // for array index comparisons, also rids us of compiler warnings
    uint16_t low_index = 0;
    uint16_t high_index = MAL_ERROR_MAX_PARAMETERS - 1;

    if(strNumber >= low_index && strNumber <= high_index)
    {
        errorStr_[strNumber] = errStr;
        errorStrType_[strNumber] = errStrType;
    }
    else
    {
        //do nothing now may return errorcode
    }
}
string MalError::getErrorStr(uint16_t strNumber)
{
    // for array index comparisons, also rids us of compiler warnings
    uint16_t low_index = 0;
    uint16_t high_index = MAL_ERROR_MAX_PARAMETERS - 1;

    if(strNumber >= low_index && strNumber <= high_index)
    {
        return errorStr_[strNumber];
    }
    else
    {
        return "";
        //do nothing now may return errorcode
    }
}

int32_t MalError::getErrorStrType(uint16_t strNumber)
{
    // for array index comparisons, also rids us of compiler warnings
    uint16_t low_index = 0;
    uint16_t high_index = MAL_ERROR_MAX_PARAMETERS - 1;
    int32_t result = 0;  // assume we don't find it

    if(strNumber >= low_index && strNumber <= high_index)
    {
        result = errorStrType_[strNumber];
    }
    return result;
}

void MalError::setErrorNumber(ErrorCode err)
{

    errorNumber_ = err;
}

ErrorCode MalError::getErrorNumber()
{
    return(errorNumber_);
}

int32_t MalError::getSeverity()
{
    return(severity_);
}

MalError::~MalError(void)
{

}

void MalError::getErrorStrings(char *errorStrings, uint32_t bufferLength, uint32_t *bytesNeeded, 
                               uint32_t *numberOfStrings)
{
    uint16_t low_index = 0;
    uint16_t high_index = MAL_ERROR_MAX_PARAMETERS - 1;
    *numberOfStrings = 0;
    *bytesNeeded = 0;

    for (uint16_t i =low_index; i <= high_index; i++)
    {
        uint32_t bytesAdded;
        size_t stringSize = errorStr_[i].size();
        const char *stringPtr = errorStr_[i].c_str();

        (*bytesNeeded) += stringSize + 1;
        if (*bytesNeeded <= bufferLength)
        {
            strcpy(errorStrings, stringPtr);
            (*numberOfStrings)++;
            bytesAdded = static_cast<uint32_t>(stringSize+1);
            errorStrings[bytesAdded-1] = 0;
            errorStrings += bytesAdded;
        }
    }
}
