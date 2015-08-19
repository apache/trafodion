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
 *****************************************************************************
 *
 * File:         EHException.C
 * Description:  methods for class EHExceptionHandler
 *
 *
 * Created:      5/19/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "EHCommonDefs.h"

#include "NAError.h"

#include <setjmp.h>
#include <string.h>
#ifndef EH_USE_GLOB_NEW
#include <stdlib.h>
#endif

#include "EHExceptionTypeEnum.h"
#include "EHExceptionTypeNode.h"
#include "EHException.h"

#include "EHCallBack.h"

// -----------------------------------------------------------------------
// methods for class EHExceptionHandler
// -----------------------------------------------------------------------
EHExceptionHandler::EHExceptionHandler()
{
  pEHCallBack_=0;
}

//
// virtual destructor
//

EHExceptionHandler::~EHExceptionHandler()
{
}

//int CompEHCounter::counter_ = 0;

//
// mutators
//
void
EHExceptionHandler::registerThrowCallBack(EHCallBack *pEHCallBack)
{
  pEHCallBack_ = pEHCallBack;
}

void
EHExceptionHandler::endTryBlock()
{
  if (getExceptionType() NEQ EH_NORMAL)
  {
    return;  // does nothing, the method throwException() already did the work
  }

  //
  // exits the try block normally.
  //

  isCaught_ = FALSE;

  EHExceptionJmpBufNode * pJmpBufNode = exceptionJmpBufStack_.pop();

  EH_ASSERT(pJmpBufNode NEQ NULL);

  //
  // pExceptionTypeNode points to a list of exceptions that can be
  // caught by the catch block(s) associating with the try block.
  //
  EHExceptionTypeNode * pExceptionTypeNode
    = pJmpBufNode->getExceptionTypeList();

  //
  // deallocates the list of exceptions associating with the try
  // block.  It is no longer needed.
  //
  while (pExceptionTypeNode NEQ NULL)
  {
    pJmpBufNode->setExceptionTypeList(pExceptionTypeNode->getNextNode());
#ifdef EH_USE_GLOB_NEW
    delete pExceptionTypeNode;
#else
    free(pExceptionTypeNode);
#endif
    pExceptionTypeNode = pJmpBufNode->getExceptionTypeList();
  }

  //
  // no longer needs the jmpbuf for this try block.
  //
#ifdef EH_USE_GLOB_NEW
  delete pJmpBufNode;
#else
  free(pJmpBufNode);
#endif
}

EHBoolean
EHExceptionHandler::catchException(EHExceptionTypeEnum exceptionType)
{
  if (getExceptionType() EQU EH_NORMAL)
  {
    return FALSE;
  }

  if (isCaught_)
  {
    //
    // A previous catch block (also associating with the current
    // try block) has caught this exception.  The current catch
    // block should not catch the exception
    //
    return FALSE;
  }
  if (getExceptionType() EQU exceptionType OR
      exceptionType EQU EH_ALL_EXCEPTIONS)
  {
    return (isCaught_ = TRUE);
  }
  return FALSE;
}

void
EHExceptionHandler::setExceptionType(Int32 exceptionType)
{
  EH_ASSERT(exceptionType >= EH_NORMAL AND
            exceptionType < EH_LAST_EXCEPTION_TYPE_ENUM);
  exceptionType_ = (EHExceptionTypeEnum)exceptionType;
}

void
EHExceptionHandler::registerException(EHExceptionTypeEnum exceptionType)
{
#ifdef EH_USE_GLOB_NEW
  EHExceptionTypeNode * pExceptionTypeNode =
    new EHExceptionTypeNode(exceptionType);
  if (pExceptionTypeNode EQU NULL)
  {
    EH_ABORT("operator new could not allocate the needed space");
  }
#else
  EHExceptionTypeNode * pExceptionTypeNode =
    (EHExceptionTypeNode *)malloc(sizeof(EHExceptionTypeNode));
  if (pExceptionTypeNode EQU NULL)
  {
    EH_ABORT("malloc() could not allocate the needed space");
  }
  //
  // Note that the constructor of class EHExceptionTypeNode
  // was not invoked.
  //
  pExceptionTypeNode->setExceptionType(exceptionType);
#endif
  pExceptionTypeNode->setNextNode(pExceptionTypeList_);
  pExceptionTypeList_ = pExceptionTypeNode;
}


// define try block

EHExceptionTypeEnum
EHExceptionHandler::defineTryBlock()
{
  // this->setjmpStatus and this->environment
  // were set by setjmp() (for more information
  // refer to the definition of macro EH_TRY)

  setExceptionType(setjmpStatus);

  if (getExceptionType() EQU EH_NORMAL)
  {
    if (pExceptionTypeList_ EQU NULL)
    {
      EH_ABORT("You have not registered any exceptions");
    }
#ifdef EH_USE_GLOB_NEW
    EHExceptionJmpBufNode * pJmpBufNode = new EHExceptionJmpBufNode;
    if (pJmpBufNode EQU NULL)
    {
      EH_ABORT("operator new could not allocate the needed space");
    }
#else
    EHExceptionJmpBufNode * pJmpBufNode =
      (EHExceptionJmpBufNode *)malloc(sizeof(EHExceptionJmpBufNode));
    if (pJmpBufNode EQU NULL)
    {
      EH_ABORT("malloc() could not allocate the needed space");
    }
    //
    // Note that the constructor of class EHExceptionJmpBufNode
    // was not invoked
    //
    pJmpBufNode->setLink(NULL);
#endif
    pJmpBufNode->setEnv(environment);
    pJmpBufNode->setExceptionTypeList(pExceptionTypeList_);
    exceptionJmpBufStack_.push(pJmpBufNode);
    pExceptionTypeList_ = NULL;
  }
  return getExceptionType();
}

// throw

void
EHExceptionHandler::throwException(EHExceptionTypeEnum exceptionType)
{
  EHExceptionJmpBufNode::env envStruct;
  EHBoolean foundException = FALSE;
  EHExceptionTypeNode * pExceptionTypeNode = NULL;
  EHExceptionJmpBufNode * pJmpBufNode = exceptionJmpBufStack_.pop();

  //
  // The following flag helps to make sure that no more than one catch
  // block (of all catch blocks associating with a try block) catches
  // an exception.  This flag is cleared whenever an exception is thrown.
  // It will be set by the first catch block that catches the exception.
  //
  isCaught_ = FALSE;

  // Record the state at the time of exception; we do this at the time
  // exception is thrown, so if an exception is thrown more than once
  // we record this as many times. User errors (as indicated by value
  // EH_PROCESSING_EXCEPTION) are not to be propagated.
  // Genesis case: 10-010523-0766

  if (( exceptionType != EH_PROCESSING_EXCEPTION ) && pEHCallBack_)
    pEHCallBack_->doFFDC();

  //
  // Searches the nested try blocks, starting with the innermost try
  // block, to find a catch block (associating with the current try
  // block) that can catch the exception.
  //
  // pJmpBufNode points to the control block describing the (current)
  // innermost try block.
  //
  while (pJmpBufNode NEQ NULL)
  {
    //
    // pExceptionTypeNode points to a list of exceptions that can be
    // caught by the catch block(s) associating with the innermost
    // try block
    //
    pExceptionTypeNode = pJmpBufNode->getExceptionTypeList();

    //
    // Finds out if there exists a catch block associating with the
    // the (current) innermost try block that can catch the exception.
    // The list is deallocated after the search since it is no longer
    // needed.
    //
    while (pExceptionTypeNode NEQ NULL)
    {
      if (pExceptionTypeNode->getExceptionType() EQU exceptionType OR
          pExceptionTypeNode->getExceptionType() EQU EH_ALL_EXCEPTIONS)
      {
        foundException = TRUE;
      }
      pJmpBufNode->setExceptionTypeList(pExceptionTypeNode->getNextNode());
#if EH_USE_GLOB_NEW
      delete pExceptionTypeNode;
#else
      free(pExceptionTypeNode);
#endif
      pExceptionTypeNode = pJmpBufNode->getExceptionTypeList();
    }
    //
    // If the thrown exception is found,
    // cut back the runtime stack now.
    //
    if (foundException)
    {
      memcpy ((void *)&envStruct,
              (void *)&pJmpBufNode->environment,
              sizeof(envStruct));
#if EH_USE_GLOB_NEW
      delete pJmpBufNode;
#else
      free(pJmpBufNode);
#endif

      if(pEHCallBack_)
	pEHCallBack_->dumpDiags();

      longjmp(envStruct.jmpBuf, exceptionType);
    }
    //
    // Has not found a catch block that can catch the exception yet.
    // Tries the catch block(s) associating with the next outer try
    // block.  Deallocates the control block describing the current
    // try block because the control block is no longer needed.
    //
#if EH_USE_GLOB_NEW
    delete pJmpBufNode;
#else
    free(pJmpBufNode);
#endif
    pJmpBufNode = exceptionJmpBufStack_.pop();
  }

  EH_ABORT("Could not find any matching exception handler");
}

