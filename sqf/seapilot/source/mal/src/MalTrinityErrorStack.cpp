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
#include "MalTrinityErrorStack.h"
#include "sqevlog/evl_sqlog_writer.h"   // for SQ_LOG_ERR etc. constants
#include <cstring>

using namespace Trinity;


//extern MalEnvironment & getMalEnv();

MalTrinityErrorStack::MalTrinityErrorStack(void) : errorCount_(0)
{

}
void MalTrinityErrorStack::addError(ErrorCode errorNumber,int32_t severity)
{
    MalError *error = new MalError(errorNumber,severity);
    errorContainer.push_back(error);
    if (severity <= (int32_t)SQ_LOG_ERR)  // the lower the number the more severe 
    {
        errorCount_++;
    }
}
void MalTrinityErrorStack::addError(ErrorCode errorNumber,int32_t severity,
                                    const string& errStr1,int32_t errStr1Type)
{
    MalError *error = new MalError(errorNumber,severity,errStr1,errStr1Type);
    errorContainer.push_back(error);
    if (severity <= (int32_t)SQ_LOG_ERR)  // the lower the number the more severe 
    {
        errorCount_++;
    }
}
void MalTrinityErrorStack::addError(ErrorCode errorNumber,int32_t severity,
                                    const string& errStr1,int32_t errStr1Type,
                                    const string& errStr2,int32_t errStr2Type)
{
    MalError *error = new MalError(errorNumber,severity,
                                               errStr1,errStr1Type,
                                               errStr2,errStr2Type);
    errorContainer.push_back(error);
    if (severity <= (int32_t)SQ_LOG_ERR)  // the lower the number the more severe 
    {
        errorCount_++;
    }
}
void MalTrinityErrorStack::addError(ErrorCode errorNumber,int32_t severity,
                                    const string& errStr1,int32_t errStr1Type,
                                    const string& errStr2,int32_t errStr2Type,
                                    const string& errStr3,int32_t errStr3Type)
{
    MalError *error = new MalError(errorNumber,severity,
                                               errStr1,errStr1Type,
                                               errStr2,errStr2Type,
                                               errStr3,errStr3Type);
    errorContainer.push_back(error);
    if (severity <= (int32_t)SQ_LOG_ERR)  // the lower the number the more severe 
    {
        errorCount_++;
    }
}
void MalTrinityErrorStack::addError(ErrorCode errorNumber,int32_t severity,
                                    const string& errStr1,int32_t errStr1Type,
                                    const string& errStr2,int32_t errStr2Type,
                                    const string& errStr3,int32_t errStr3Type,
                                    const string& errStr4,int32_t errStr4Type)
{
    MalError *error = new MalError(errorNumber,severity,
                                               errStr1,errStr1Type,
                                               errStr2,errStr2Type,
                                               errStr3,errStr3Type,
                                               errStr4,errStr4Type);
    errorContainer.push_back(error);
    if (severity <= (int32_t)SQ_LOG_ERR)  // the lower the number the more severe 
    {
        errorCount_++;
    }
}

void MalTrinityErrorStack::addError(ErrorCode errorNumber,int32_t severity,
                                    const string& errStr1,int32_t errStr1Type,
                                    const string& errStr2,int32_t errStr2Type,
                                    const string& errStr3,int32_t errStr3Type,
                                    const string& errStr4,int32_t errStr4Type,
                                    const string& errStr5,int32_t errStr5Type)
{
    MalError *error = new MalError(errorNumber,severity,
                                               errStr1,errStr1Type,
                                               errStr2,errStr2Type,
                                               errStr3,errStr3Type,
                                               errStr4,errStr4Type,
                                               errStr5,errStr5Type);
    errorContainer.push_back(error);
    if (severity <= (int32_t)SQ_LOG_ERR)  // the lower the number the more severe
    {
        errorCount_++;
    }
}

void MalTrinityErrorStack::removeError(uint16_t entry)
{
    if (entry < itemCount())
    {
        vector<MalError *>::iterator entryIterator;
        entryIterator = errorContainer.begin() + entry;
        MalError * toBeDeleted = *entryIterator;
        if (toBeDeleted->getSeverity() <= (int32_t)SQ_LOG_ERR)
        {
            errorCount_--;
        }
        delete (*entryIterator);
        errorContainer.erase(entryIterator);
    }
}

uint16_t MalTrinityErrorStack::itemCount() const
{
    return (uint16_t)errorContainer.size();
}

uint16_t MalTrinityErrorStack::errorCount() const
{
    return errorCount_;
}

void MalTrinityErrorStack::clear()
{
    for(size_t i=0;i<errorContainer.size();i++)
    {
        delete errorContainer.at(i);
    }
    errorContainer.clear();
    errorCount_ = 0;
}
ErrorCode MalTrinityErrorStack::getErrorCode(uint16_t entry) const
{
    if (entry < itemCount())
    {
        return errorContainer.at(entry)->getErrorNumber();
    }
    else
    {
        return MAL_ERR_GETTING_ERRCODE;
    }
}
ErrorCode MalTrinityErrorStack::getErrorCodeTop()
{
    if (itemCount() > 0)
    {
        return (errorContainer.back())->getErrorNumber();
    }
    return MAL_ERR_GETTING_ERRCODE;
}
ErrorCode MalTrinityErrorStack::popErrorCodeTop()
{
    ErrorCode rc = MAL_ERR_GETTING_ERRCODE;
    if (itemCount() > 0)
    {
       rc =  (errorContainer.back())->getErrorNumber();
       if (errorContainer.back()->getSeverity() <= SQ_LOG_ERR)
       {
           errorCount_--;
       }
       delete errorContainer.at(errorContainer.size()-1);
       errorContainer.pop_back();
    }
    return rc;
}

int32_t MalTrinityErrorStack::getSeverity(uint16_t entry) const
{
    if (entry < itemCount())
    {
        return errorContainer.at(entry)->getSeverity();
    }
    else
    {
        return -1;
    }
}
int32_t MalTrinityErrorStack::getSeverityTop()
{
    if (itemCount() > 0)
    {
        return (errorContainer.back())->getSeverity();
    }
    return -1;
}





string MalTrinityErrorStack::getErrorStr(uint16_t entry,uint16_t strNumber) const
{
    if (entry < itemCount())
    {
        return errorContainer.at(entry)->getErrorStr(strNumber);
    }
    else
    {
        return "";
    }
}
int32_t MalTrinityErrorStack::getErrorStrType(uint16_t entry,uint16_t strNumber) const
{
    int32_t result = 0;  // assume absent
    if (entry < itemCount())
    {
        result = errorContainer.at(entry)->getErrorStrType(strNumber);
    }
    return result;
}

void MalTrinityErrorStack::setErrorCode(uint16_t entry,ErrorCode err)
{
    if (entry < itemCount())
    {
        errorContainer.at(entry)->setErrorNumber(err);
    }
    else
    {
        //do nothing for now but may return errorcode
    }
}
void MalTrinityErrorStack::setErrorStr(uint16_t entry,
   string errStr,int32_t errStrType,uint16_t strNumber)
{
    if (entry < itemCount())
    {
        errorContainer.at(entry)->setErrorStr(errStr,errStrType,strNumber);
    }
    else
    {
        //do nothing now but may return errorcode
    }
}
MalTrinityErrorStack& MalTrinityErrorStack::operator=(const MalTrinityErrorStack &mtes)
{
    // check for self-assignment by comparing the address of the
    // implicit object and the parameter
    if(this == &mtes)
    {
         return *this;
    }

    //check to see if existing stack contains items and then delete them
    for( uint16_t i=0;i<this->itemCount();i++)
    {
        delete this->errorContainer.at(i);
    }
    this->errorContainer.clear();

    //copy items over to stack
    for( uint16_t i=0;i<mtes.itemCount();i++)
    {
        this->addError(mtes.getErrorCode(i),
                       mtes.getSeverity(i),
                       mtes.getErrorStr(i,0),
                       mtes.getErrorStrType(i,0),
                       mtes.getErrorStr(i,1),
                       mtes.getErrorStrType(i,1),
                       mtes.getErrorStr(i,2),
                       mtes.getErrorStrType(i,2),
                       mtes.getErrorStr(i,3),
                       mtes.getErrorStrType(i,3));
    }
    return (*this);
}

MalTrinityErrorStack::MalTrinityErrorStack(const MalTrinityErrorStack &mtes) : errorCount_(0)
{
    for( uint16_t i=0;i<mtes.itemCount();i++)
    {
        this->addError(mtes.getErrorCode(i),
                       mtes.getSeverity(i),
                       mtes.getErrorStr(i,0),
                       mtes.getErrorStrType(i,0),
                       mtes.getErrorStr(i,1),
                       mtes.getErrorStrType(i,1),
                       mtes.getErrorStr(i,2),
                       mtes.getErrorStrType(i,2),
                       mtes.getErrorStr(i,3),
                       mtes.getErrorStrType(i,3));
    }
}

MalTrinityErrorStack::~MalTrinityErrorStack(void)
{
    for( uint16_t i=0;i<errorContainer.size();i++)
    {
        delete errorContainer.at(i);
    }
    errorContainer.clear();
}

void MalTrinityErrorStack::addErrors(const MalTrinityErrorStack &errStack)
{
    for( uint16_t i=0;i<errStack.itemCount();i++)
    {
        addError(errStack.getErrorCode(i),
                 errStack.getSeverity(i),
                 errStack.getErrorStr(i,0),
                 errStack.getErrorStrType(i,0),
                 errStack.getErrorStr(i,1),
                 errStack.getErrorStrType(i,1),
                 errStack.getErrorStr(i,2),
                 errStack.getErrorStrType(i,2),
                 errStack.getErrorStr(i,3),
                 errStack.getErrorStrType(i,3));
    }
}

bool MalTrinityErrorStack::allErrorsAre(ErrorCode errorCode)
{
    bool rc = true;  // assume every error (if any) in the stack has this error code

    for (uint16_t i = 0; (rc) && (i < itemCount()); i++)
    {
        rc = (errorCode == getErrorCode(i));
    }
    return rc;
}

ErrorCode MalTrinityErrorStack::getErrorDetail(uint16_t entry, ErrorCode *errorNumber, char *errorStrings,
                                               uint32_t bufferLength, uint32_t *bytesNeeded, uint32_t *numberOfStrings)
{
    ErrorCode retval = MAL_ARRAY_OUT_OF_BOUNDS;
    *errorNumber = (ErrorCode)0;
    *bytesNeeded = 0;
    *numberOfStrings = 0;    

    if ((entry + 1) <= itemCount())
    {
        retval = (ErrorCode)0;
        *errorNumber = getErrorCode(entry);
        errorContainer.at(entry)->getErrorStrings(errorStrings,
            bufferLength, 
            bytesNeeded, 
            numberOfStrings);
    }
    return retval;
}


void MalTrinityErrorStack::printStackContents()
{
	for (uint16_t i = 0; i < (uint16_t)errorContainer.size(); i++)
	{
	     cout << getErrorCode(i) << " strings are ";
	     for(int j = 0; j < 4; j++)
	     {
		    string errorString = getErrorStr(i,j);
		    if(errorString.size() > 0)
		    {
	         	cout << errorString << endl;
		    }
		    else
    		{
    		    cout << "no value" << endl;
		    }
	     }
	}
}
