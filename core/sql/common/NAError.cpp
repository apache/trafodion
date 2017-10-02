/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
**************************************************************************
*
* File:         NAError.C
* Description:  New Architecture Error Handler
* Created:      01/24/95
* Language:     C++
*
*
*
**************************************************************************
*/

#include "NAError.h"
#include <iostream>

// -----------------------------------------------------------------------
// NAErrorParamArray
// -----------------------------------------------------------------------
NAErrorParamArray::NAErrorParamArray
                    (Lng32 numParams,
		     NAErrorParam * errParam1,NAErrorParam * errParam2,
		     NAErrorParam * errParam3,NAErrorParam * errParam4,
		     NAErrorParam * errParam5,NAErrorParam * errParam6,
		     NAErrorParam * errParam7,NAErrorParam * errParam8,
		     NAErrorParam * errParam9,NAErrorParam * errParam10) 
                : numParams_(numParams)
{
  array_ = new NAErrorParamArrayElement[numParams];
  NAErrorParam * errParamPtr;
  for (Lng32 index = 0; index < numParams; index ++)
    {
           if (index == 0) errParamPtr = errParam1;
      else if (index == 1) errParamPtr = errParam2;
      else if (index == 2) errParamPtr = errParam3;
      else if (index == 3) errParamPtr = errParam4;
      else if (index == 4) errParamPtr = errParam5; 
      else if (index == 5) errParamPtr = errParam6;
      else if (index == 6) errParamPtr = errParam7; 
      else if (index == 7) errParamPtr = errParam8;
      else if (index == 8) errParamPtr = errParam9;
      else if (index == 9) errParamPtr = errParam10;
      
      array_[index].errParam_ = errParamPtr;
    }
} // NAErrorParamArray::NAErrorParamArray()

NAErrorParamArray::~NAErrorParamArray (void)
{
   for (Int32 index=0; index<entries(); index++)
   {
      if (array_[index].errParam_ != 0) delete array_[index].errParam_;
   };

   delete array_;
};

// -----------------------------------------------------------------------
// Constructor 
// -----------------------------------------------------------------------
NAError::NAError(const NAErrorCode errCode,
		 const NAErrorType errType,
		 const NASubsys subsys,
		 NAErrorParamArray * errParams,
		 char * procName,
		 const ULng32 lineNumber,
		 const ULng32 offset
		 ) 
       : errCode_(errCode), errType_(errType), subsysId_(subsys),
	 procName_(procName),
         lineNumber_(lineNumber), offset_(offset), next_(0)
{
  errParams_ = errParams;
}
  
NAError::~NAError()
{
  if (errParams_)
    delete errParams_;
  if (procName_ )
    delete procName_;
} // NAError::~NAError()

NAErrorStack::~NAErrorStack()
{
  NAErrorStack::clear();
} // NAErrorStack::~NAErrorStack()

void NAErrorStack::clear()
{
  NAError * errorPtr = errEntry_;
  NAError * nextPtr;
  
  while (errorPtr)
    {
      nextPtr = errorPtr->getNext();
      delete errorPtr;
      errorPtr = nextPtr;
    }

  errEntry_ = 0;
  nextEntry_ = errEntry_;
  numEntries_ = 0;
} // NAErrorStack::clear

void NAErrorStack::addErrEntry(NAError * errPtr)
{
  if (numEntries_ <= maxEntries_)
    {
      errPtr->setNext(errEntry_);
      errEntry_ = errPtr;
      numEntries_++;
    }
  else
    ; // I believe SQL2 wants us to issue an error here.
} // NAErrorStack::addErrEntry()

NAError * NAErrorStack::getNext()
{
  if (iterEntries_ <= numEntries_)
    {
      iterEntries_++;
      NAError * ne = nextEntry_;
      if (nextEntry_ != 0)
	nextEntry_ = nextEntry_->getNext(); // init with -> to follower
  
      return ne;
    }
  else
    return 0;
} // NAErrorStack::getNext()

NAError * NAErrorStack::getFirst()
{
  nextEntry_ = errEntry_;
  iterEntries_ = 0;
  return getNext();
} // NAErrorStack::getFirst()


