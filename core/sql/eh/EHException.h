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
#ifndef EHEXCEPTION_H
#define EHEXCEPTION_H
/* -*-C++-*-
******************************************************************************
*
* File:         EHException.h
* Description:  Exception handling support (without stack unwinding support)
*
*
* Created:      5/16/95
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "NABasicObject.h"
#include "EHCommonDefs.h"
#include "EHExceptionTypeEnum.h"
#include "EHJmpBufNode.h"
#include "EHJmpBufStack.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class EHExceptionHandler;
class EHCallBack;
// macro EH_END_TRY
// macro EH_REGISTER
// macro EH_TRY
// macro EH_CATCH
// macro EH_THROW
// macro EH_RESET

// -----------------------------------------------------------------------
//
//       Descriptions of This Exception Handling Package
//     ---------------------------------------------------
//
// This exception handling package provides temporary support for
// exception handling (until the Tandem Native Mode C++ compiler
// provides the C++ exception handling support).  This exception
// handling package cut back the runtime stack by calling the C
// runtime function longjmp.  The destructors of the cut-back
// automatic instances will not be invoked.
//
// The header file EHExceptionTypeEnum.h contains the list of
// enumerated constants for various kinds of exceptions.  If the
// user needs to handle a new kind of exception that is not
// described in the list, the user will need to update this file
// to add a new enumerated constant to describe the new exception.
// The new enumerated constant must be have the EH_ prefix, and
// it must be placed between the existing enumerated constants
// EH_NORMAL and EH_LAST_EXCEPTION_TYPE_ENUM.
//
// The macro EH_TRY is corresponding to the C++ keyword try.  It
// should be used in place of the try keyword in a try block.
//
// The macro EH_CATCH should be used in place of the catch
// keyword in a catch block.  Unlike the parameter of the C++
// catch clause, the parameter of the macro EH_CATCH must be an
// enumerated constant (defined in the file EHExceptionTypeEnum.h).
//
// Since this exception handling package does not have any C++
// compiler support, the package requires that the user specify
// the kinds of exceptions to be caught by the catch blocks
// associating with a try block.  The macro EH_REGISTER is
// used for this purpose.  It must be placed before the try block,
// and it only accepts one parameter (an enumerated constant
// describing the kind of exception to be caught).  If there are
// more than one catch blocks assocating with a try block, more
// than one EH_REGISTER will need to be invoked to register the
// exceptions associating with the try block.  The expanded
// (invoked) macro EH_REGISTER must placed right before the
// expanded macro EH_TRY.  It is recommended that the catch blocks
// be placed right after the try block (as required by the C++
// syntax).
//
// The user also needs to place the macro EH_END_TRY right after
// the try block, before any catch blocks.  If the code in the
// try block exits the try block, the user needs to place the
// macro EH_END_TRY right befor the exit point.  This is necessary
// since this exception handling package does not any any C++
// compiler support.  The macro EH_END_TRY should not be terminated
// by any semicolon.
//
// The macro EH_RESET should be used in a catch block if the exception is
// not going to be thrown.  This resets the exception type to normal so
// other exception blocks work correctly.  This fixes a problem encountered
// while testing a bug fix for genesis case:  10-020808-8324
//
// Example:
//
//      EH_REGISTER(EH_ALL_EXCEPTIONS);
//      EH_REGISTER(EH_ARITHMETIC_OVERFLOW);
//      EH_REGISTER(EH_OUT_OF_RANGE);
//      EH_TRY
//      {
//        // code in try block
//
//        if (wantsToExitThisRoutineIsTrue)
//        {
//          EH_END_TRY;
//          return;  // Exits try block
//        }
//
//        // other code in try block
//      }
//      EH_END_TRY
//      EH_CATCH(EH_ARITHMETIC_OVERFLOW)
//      {
//        // code in catch block to handle
//        // arithmetic overflow condition
//      }
//      EH_CATCH(EH_OUT_OF_RANGE)
//      {
//        // code in catch block to handle
//        // out of range condition
//      }
//      EH_CATCH(EH_ALL_EXCEPTIONS)
//      {
//        // catches the other remaining exceptions
//      }
//
// The enumerated constant EH_ALL_EXCEPTIONS simulates the ellipsis
// in the catch statement.  EH_ALL_EXCEPTIONS may be used to catch
// any errors, and it must be registered just like any other exceptions.
// The EH_ALL_EXCEPTIONS catch block should be placed last.
//
//
// The macro EH_THROW should be used in place of the C++ throw
// keyword.  Unlike the syntax of the C++ try statement, this macro
// requires one parameter (an enumerated constant describing the
// the kind of exception being thrown).  For example:
//
//      EH_THROW(EH_OUT_OF_RANGE);
//
// The macro EH_THROW needs to be terminated by a semicolon.
//
// This exception handling package does not issue any error messages
// when two catch blocks (associating with the same try block) catch
// the same exception.  When that exception is thrown, the first catch
// block will catch the exception and the other catch block is skipped.
//
// The files test1.C, test2.C, and test3.C in this directory illustrate
// how the macros with the EH_ prefix can be used.
//
//
//              Instructions on How to Use This Package
//            -------------------------------------------
//
// This exception handling package are included in the library
// file libehtool.a in the directory containing the file
// EHException.h (this file).  To use the macros provided in this
// package, the user's source file need to include the header
// file EHException.h (this file).  Since the header file EHException.h
// includes other header files in the same directory, the user needs
// to specify the compiler flag -I<ehdir> in the compilation.  <ehdir>
// represents the name of the directory containing the package.
// Besides specifying the include (-I) flag, the user also needs to
// specify the library flags -L<ehdir> and -lehtool to bind the routines
// and methods provided by this package to the user's target object.
// For example:
//
//      CC -o test1.o -c -I/designs/newarc/ark/main-thread/eh test1.C
//      CC -o test1 test1.o -L/designs/newarc/ark/main-thread/eh -lehtool
//
// where /designs/newarc/ark/main-thread/eh is the directory
// containing this exception handling package.
//
// The file makefile.test in the same directory illustrates the
// steps in the make file used to build the tests test1.C and
// test2.C to test the exception handling package.
//
// -----------------------------------------------------------------------


// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class NABasicObject;

// -----------------------------------------------------------------------
// Macro definitions
// -----------------------------------------------------------------------
/*
#define EH_END_TRY \
        EHExceptionHandle.endTryBlock();

#define EH_REGISTER(exceptionType) \
        EHExceptionHandle.registerException(exceptionType)

#define EH_TRY \
	EHExceptionHandle.setjmpStatus = \
		setjmp(EHExceptionHandle.environment.jmpBuf); \
        if (EHExceptionHandle.defineTryBlock() EQU EH_NORMAL)

#define EH_CATCH(exceptionType) \
        if (EHExceptionHandle.catchException(exceptionType))

#define EH_THROW(exceptionType) \
	EHExceptionHandle.throwException(exceptionType)

#define EH_REGISTER_THROW_CALL_BACK(pEHCallBack) \
        EHExceptionHandle.registerThrowCallBack(pEHCallBack)
#define EH_RESET \
        EHExceptionHandle.setExceptionType(EH_NORMAL)
*/
#define EH_END_TRY !!!EH_END_TRY_RETIRED_DO_NOT_USE_IT
#define EH_REGISTER(exceptionType) !!!EH_REGISTER_RETIRED_DO_NOT_USE_IT
#define EH_TRY !!!EH_TRY_RETIRED_DO_NOT_USE_IT
#define EH_CATCH(exceptionType) !!!EH_CATCH_RETIRED_DO_NOT_USE_IT
#define EH_THROW(exceptionType) !!!EH_THROW_RETIRED_DO_NOT_USE_IT
#define EH_REGISTER_THROW_CALL_BACK(pEHCallBack) !!!EH_REGISTER_THROW_CALL_BACK_RETIRED_DO_NOT_USE_IT
#define EH_RESET !!!EH_RESET_RETIRED_DO_NOT_USE_IT

class EHBreakException // EH conversion.
{
public:
  EHBreakException(const char* fileName = NULL, Int32 num = 0)
   : lineNum_(num)
  {
   if ( fileName ) {
     strncpy(fileName_, fileName, sizeof(fileName_));
     fileName_[sizeof(fileName_)-1] = 0;
   } else
     fileName_[0] = 0;
  };

  ~EHBreakException() {};

  const char * getFileName() const        { return fileName_; }
  const UInt32 getLineNum() const          { return lineNum_; }

private:
  char fileName_[512];
  UInt32 lineNum_;

}; // EH conversion.




// -----------------------------------------------------------------------
// class for exception handler - !!! Obsolete class.
// -----------------------------------------------------------------------
class EHExceptionHandler
{
public:

  // ---------------------------------------------------------------------
  // public data members
  // ---------------------------------------------------------------------

  // environment.jmpBuf set by setjmp()
  EHExceptionJmpBufNode::env environment;

  // contains the value returned by setjmp()
  Int32 setjmpStatus;

  // ---------------------------------------------------------------------
  // public methods
  // ---------------------------------------------------------------------
  EHExceptionHandler();
  //
  // virtual destructor
  //

  virtual ~EHExceptionHandler();

  //
  // accessor
  //

  inline EHExceptionTypeEnum getExceptionType() const;

  //
  // mutators
  //

  void endTryBlock();

  // catch an exception

  EHBoolean catchException(EHExceptionTypeEnum exceptionType);

  void setExceptionType(Int32 setjmpStatus);

  // register the type of the exception handlers
  // associating with the following try block

  void registerException(EHExceptionTypeEnum exceptionType);

  // define try block

  EHExceptionTypeEnum defineTryBlock();

  // throw

  void throwException(EHExceptionTypeEnum exceptionType);

  void registerThrowCallBack(EHCallBack *pEHCallBack);
private:

  EHExceptionJmpBufStack exceptionJmpBufStack_;
  EHExceptionTypeEnum exceptionType_;
  EHExceptionTypeNode * pExceptionTypeList_;

  // The following flag helps to make sure that no more than one catch
  // block (of all catch blocks associating with a try block) catches
  // an exception.  This flag is cleared whenever an exception is thrown.

  EHBoolean isCaught_;

  EHCallBack *pEHCallBack_;

};  // class EHExceptionHandler

// -----------------------------------------------------------------------
// definitions of inline methods for class EHExceptionHandler
// -----------------------------------------------------------------------

inline EHExceptionTypeEnum
EHExceptionHandler::getExceptionType() const
{
  return exceptionType_;
}

#endif // EHEXCEPTION_H

