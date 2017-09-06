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
#ifndef NAERROR_H
#define NAERROR_H
/* -*-C++-*-
**************************************************************************
*
* File:         NAError.h
* Description:  New Architecture Error Class
* Created:      01/23/95
* Language:     C++
*
*
**************************************************************************
*/

#include "ErrorCondition.h"
#include "NAExit.h"

// -----------------------------------------------------------------------
// Global defines and functions
// -----------------------------------------------------------------------

void NAError_stub_for_breakpoints();        // in BaseTypes.cpp not NAError!
void NAArkcmpExceptionEpilogue();

#define ARKCMP_EXCEPTION_EPILOGUE(str)      NAArkcmpExceptionEpilogue()
#define EXECUTOR_EXCEPTION_EPILOGUE(str)    NAError_stub_for_breakpoints()
#define SQLCI_EXCEPTION_EPILOGUE(str)       NAError_stub_for_breakpoints()
#define UDRSERV_EXCEPTION_EPILOGUE(str)     NAError_stub_for_breakpoints()
#define MXEXPORTDDL_EXCEPTION_EPILOGUE(str) NAError_stub_for_breakpoints()
#define MXIMPORTDDL_EXCEPTION_EPILOGUE(str) NAError_stub_for_breakpoints()
#define MXPREPAREMAP_EXCEPTION_EPILOGUE(str) NAError_stub_for_breakpoints()

// -----------------------------------------------------------------------
// Classes declared in this file.
// -----------------------------------------------------------------------
class NAError;
class NAErrorParam;
class NAErrorParamArray;
class NAErrorStack;

// ***********************************************************************
// The error code datatype.
// ***********************************************************************
typedef Lng32 NAErrorCode;

// ***********************************************************************
// A wrapper for an error parameter.
// ***********************************************************************
class NAErrorParam
{
public:

  enum NAErrorParamType {
                          NAERROR_PARAM_TYPE_INTEGER,
                  NAERROR_PARAM_TYPE_CHAR_STRING
                        };


  // ---------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------
  NAErrorParam(Lng32 l) : errParamType_(NAERROR_PARAM_TYPE_INTEGER),
                         longValue_(l)
  { }

  NAErrorParam(char * s) : errParamType_(NAERROR_PARAM_TYPE_CHAR_STRING),
                           stringValue_(s)
  { }

  // ---------------------------------------------------------------------
  // Destructor
  // ---------------------------------------------------------------------
  virtual ~NAErrorParam() {};

  // ---------------------------------------------------------------------
  // Ideally, there should be just one method for returning a string for
  // the value contained in an ErrParam. However, the NAError class can
  // also be constructed by the executor, which can be running in an SRL
  // (aka DLL) on Guardian. This imposes a restriction on the use of
  // conversion functions such as sprintf. This is because they require
  // the allocation of global data in the code space of the caller; an
  // SRL cannot contain global data because of the privilege model that
  // is implemented on Guardian. Thus, we cannot use formatting functions
  // such as sprintf and support a single method here.
  //
  // The programming model for a caller who wants to interpret the
  // contents of an NAErrorParam is illustrated by the pseudo code below:
  //     char errorParamValue[BUFFER_SIZE];
  //     char * S = errorParamValue;
  //
  //     switch (NAErrorParamVariable.getNAErrorParamType())
  //       {
  //       case NAERROR_PARAM_TYPE_INTEGER:
  //         sprintf(S,"%d",NAErrorParamVariable.getIntegerNAErrorParam());
  //         break;
  //       case NAERROR_PARAM_TYPE_CHAR_STRING:
  //         sprintf(S,"%s",NAErrorParamVariable.getStringNAErrorParam());
  //         break;
  //       default:
  //         // error case
  //         break;
  //       }
  //
  // ---------------------------------------------------------------------
  NAErrorParamType getNAErrorParamType() const
  { return errParamType_; }

  Lng32 getIntegerNAErrorParam() const
  { return longValue_; }

  char * getStringNAErrorParam() const
  { return stringValue_; }

protected:
  NAErrorParamType  errParamType_;
  Lng32            longValue_;
  char *          stringValue_;

}; // class NAErrorParam


// ***********************************************************************
// NAErrorParamArray
// A wrapper for supplying a single package of N NAErrorParam objects.
// This wrapper is designed to be a readonly object. No mutator functions
// are supported on it.
// ***********************************************************************
class NAErrorParamArray
{
public:

  // ---------------------------------------------------------------------
  // The first argument, numParams, ends up deciding how many of the
  // following parameters are relevant.
  // ---------------------------------------------------------------------
  NAErrorParamArray(Lng32 numParams = 0,
            NAErrorParam * errParam0 = 0,
            NAErrorParam * errParam1 = 0,
            NAErrorParam * errParam2 = 0,
            NAErrorParam * errParam3 = 0,
            NAErrorParam * errParam4 = 0,
            NAErrorParam * errParam5 = 0,
            NAErrorParam * errParam6 = 0,
            NAErrorParam * errParam7 = 0,
            NAErrorParam * errParam8 = 0,
            NAErrorParam * errParam9 = 0
           );

  virtual ~NAErrorParamArray();

  // ---------------------------------------------------------------------
  // How many NAErrorParams are contained in the NAErrorParamArray?
  // ---------------------------------------------------------------------
  Lng32 entries() const { return numParams_; }

  // ---------------------------------------------------------------------
  // Accessor function.
  // ---------------------------------------------------------------------
  NAErrorParam * getNAErrorParam(Lng32 paramNo) const
  {
    if (  (paramNo >= 0) && (paramNo < numParams_ ) )
      return array_[paramNo].errParam_;
    else
      return 0;
  }

  // ---------------------------------------------------------------------
  // NO MUTATOR FUNCTIONS.
  // ---------------------------------------------------------------------

private:
  // ---------------------------------------------------------------------
  // A private declaration
  // ---------------------------------------------------------------------
  struct NAErrorParamArrayElement
    {
      NAErrorParam * errParam_;
    }; // struct NAErrorParamArrayElement

private:
  Lng32                     numParams_;
  NAErrorParamArrayElement * array_;
}; // class NAErrorParamArray

// ***********************************************************************
// An error issued by an SQL subsystem.
// ***********************************************************************
class NAError
{
public:

  // ---------------------------------------------------------------------
  // A classification for the type of error that is issued.
  // ---------------------------------------------------------------------
  enum NAErrorType {
                     NAERROR_NONE,
                     NAERROR_WARNING,
             NAERROR_SYNTAX,
             NAERROR_SQL_SEMANTICS,
             NAERROR_EXPR_SEMANTICS,
             NAERROR_INTERNAL,
             NAERROR_EXCEPTION
           };

  // ---------------------------------------------------------------------
  // An id for the subsystem that issues the error.
  // ---------------------------------------------------------------------
  enum NASubsys {
                  NA_CATMAN = 1000,
                  NA_COMPMAIN = 2000,
                  NA_PARSER = 3000,
                  NA_BINDER = 4000,
          NA_NORMALIZER = 5000,
          NA_OPTIMIZER = 6000,
          NA_CODEGEN = 7000,
          NA_EXPRGEN = 8000,
          NA_EXECUTOR = 9000,
          NA_EXPREVAL = 10000,
          NA_FILESYSTEM = 11000,
          NA_OS = 12000,
                  NA_SQLC = 13000,
                  NA_UNKNOWN = 14000
            };

  // ---------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------
  NAError(const NAErrorCode errCode = 0,
          const NAErrorType errType = NAERROR_NONE,
      const NASubsys subsys = NA_UNKNOWN,
      NAErrorParamArray * errParams = 0,
      char * procName = 0,
      const ULng32 lineNumber = 0,
      const ULng32 offset = 0
         );

  // ---------------------------------------------------------------------
  // Destructor
  // ---------------------------------------------------------------------
  ~NAError();

  // ---------------------------------------------------------------------
  // Accessor functions.
  // ---------------------------------------------------------------------
  NAErrorCode getErrCode() const { return errCode_; }
  NAErrorType getErrType() const { return errType_; }
  NASubsys  getSubsys() const { return subsysId_; }

  // ---------------------------------------------------------------------
  // Error Parameters.
  // ---------------------------------------------------------------------
  Lng32 getErrParamCount() const
    {
      if (errParams_)
    return errParams_->entries();
      else
    return 0;
    }

  NAErrorParam * getNAErrorParam(Lng32 paramNo) const
  {
    if (errParams_)
      return errParams_->getNAErrorParam(paramNo);
    else
      return 0;
  }

  // ---------------------------------------------------------------------
  // Procedure name and line numbers.
  // ---------------------------------------------------------------------
  char * getProcName() const { return procName_; }

  Lng32  getLineNumber() const { return lineNumber_; }

  // ---------------------------------------------------------------------
  // Method for traversing error list
  // ---------------------------------------------------------------------
  void setNext(NAError * errPtr) { next_ = errPtr; }
  NAError * getNext() const { return next_; }

  // ---------------------------------------------------------------------
  // Linearization and De-linearization functions to copy SQLDiagArea
  // tree to and from contiguous storage.
  // ---------------------------------------------------------------------
  Long pack(void * space);
  Lng32 unpack(Lng32);

private:

  // ---------------------------------------------------------------------
  // Link to its predecessor.
  // ---------------------------------------------------------------------
  NAError * next_;

  // ---------------------------------------------------------------------
  // Error code and type.
  // ---------------------------------------------------------------------
  const NAErrorCode errCode_;     // error code
  const NAErrorType errType_;     // error type
  const NASubsys  subsysId_;    // id of subsystem that issues the error

  // ---------------------------------------------------------------------
  // Error Parameters.
  // ---------------------------------------------------------------------
  NAErrorParamArray * errParams_;

  // ---------------------------------------------------------------------
  // Procedure name and line numbers.
  // ---------------------------------------------------------------------
  const Lng32   lineNumber_;
  char * procName_;
  const Lng32   offset_;

}; // class NAError

// ***********************************************************************
// The error stack
// ***********************************************************************
class NAErrorStack
{
public:

  // ---------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------
  NAErrorStack(Lng32 maxSize)
              : maxEntries_(maxSize), numEntries_(0), errEntry_(0),
        nextEntry_(0), iterEntries_(0)
  {
  }

  // ---------------------------------------------------------------------
  // Destructor
  // ---------------------------------------------------------------------
  ~NAErrorStack();

  // ---------------------------------------------------------------------
  // Method for initializing an error stack - once per statement
  // ---------------------------------------------------------------------
  void clear();

  // ---------------------------------------------------------------------
  // Method for returning the error code for the latest error
  // ---------------------------------------------------------------------
  NAErrorCode getErrCode()
  {
    if (errEntry_ != (NAError *)0)
      return errEntry_->getErrCode();
    else
      return (NAErrorCode)0;
  }

  // ---------------------------------------------------------------------
  // Method for adding a new Error
  // ---------------------------------------------------------------------
  void addErrEntry(NAError * errPtr);

  // ---------------------------------------------------------------------
  // Iterator over all errors
  // ---------------------------------------------------------------------
  NAError * getFirst(); // initialize the iterator
  NAError * getNext();  // behaviour undefined unless getFirst() was called

private:

  // ---------------------------------------------------------------------
  // The error stack
  // ---------------------------------------------------------------------
  Lng32  maxEntries_;       // if prescribed by a SET TRANSACTION
  Lng32  numEntries_;       // number of entries allocated so far
  NAError * errEntry_;     // an array of NAError
  NAError * nextEntry_;    // maintained after an iterator is initialized
  Lng32     iterEntries_;   // for terminating iterations over unused entries

}; // class NAErrorStack


#endif /* NAERROR_H */








