///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_session.cpp
/// \brief   Implementation of STFS_Session
///   
/// This file contains the STFS session implementation.  
//
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
///////////////////////////////////////////////////////////////////////////////

#include <errno.h>

#include "stfs/stfslib.h"
#include "stfs_defs.h"

#include "stfs_sigill.h"
#include "stfs_session.h"

using namespace STFS;

STFS_Session * STFS_Session::session_ = 0;

bool
STFS_Session::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid(STFS_Root::EC_Session);
}
 
STFS_Session::STFS_Session() : STFS_Root(EC_Session)
{}

STFS_Session::~STFS_Session() 
{}
///////////////////////////////////////////////////////////////////////////////
///
//          GetSession
///
/// \brief  Returns an instance of STFS_Session
///
///         returns an instance of the STFS_Session
///         (creates the session if necessary)
///
/// \retval STFS_Session * SUCCESS: Valid Pointer \n
///                        FAILURE: NULL pointer
///
///////////////////////////////////////////////////////////////////////////////
STFS_Session *
STFS_Session::GetSession() 
{
  if (STFS_Session::session_) {
    return STFS_Session::session_;
  }

  STFS_Session::session_ = new STFS_Session();

  if (!STFS_Session::session_) {
    return NULL;
  }
    
  return STFS_Session::session_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetError
///
/// \brief  Retrieves an error condition, if any, associated with this session
///
/// This method returns the error information that has been saved to
/// provide additional information on the error condition.  Retrieving
/// error information is an iterative process, since one error might
/// have several subordinate or follow-on errors associated with
/// it. Each successive call to this method returns a new condition.
/// Each condition is independent and must have an error number.  In
/// addition, each condition might also have an addiitonal error
/// number, and/or a context buffer and context length.  Callers
/// indicate which of these parameters should be returned as indicated
/// in the parameter descriptions below.
///
/// The boolean return value associated with this method indicates
/// whether there might be more errors.
///
/// \param  pv_errNo               The error number for this condition.
/// \param  pv_addlErr             Any secondary error associated with
///                                   this condition.  Pass NULL if
///                                   this value should not be returned.
/// \param  pp_contextBuf          A text array buf, suitable for
///                                   printing, that represents any
///                                   context that could be printed
///                                   for this condition.  If NULL,
///                                   then no context is returned.
/// \param  pv_contextBufLenMax    The max length of the context buffer.
///                                   Must be 0 if no context buffer.
/// \param  pv_contextBufLenMax    The actual length of the context
///                                   buffer returned.  Pass NULL if
///                                   no context is desired.  If 0,
///                                   then no context was returned.
/// 
/// \retval off_t           SUCCESS: true:  An error condition was found
///                         ERROR:   false: No more error conditions
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_Session::GetError ( int *pp_errNo,
                         int *pp_additionErr,
                         char *pp_context,
                         size_t pv_contextLenMax,
                         size_t *pp_contextLenOut ) {

  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
    return false; // Housekeeping, since SigIll doesn't return...
  }

  // NB:  Right now, we swallow significance so clients of this
  // module don't need to include stfs_error.h.  They don't care about
  // the significance anyway, since all efh errors are high.  If we need
  // to, we can add a significance parameter.

  bool status = sessionError_.ExtractCondition ( NULL, // condition significance
                                              pp_errNo, pp_additionErr, pp_context,
                                              pv_contextLenMax, pp_contextLenOut);

  return status;

}

///////////////////////////////////////////////////////////////////////////////
///
//          HasError
///
/// \brief  Check if there are any error conditions associated
///
/// This method reports whether or not there are any error conditions
/// associated with this session.
/// 
/// \retval    true:  One or more error conditions were found
///            false: No error conditions are stored in this session
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_Session:: HasError() {

  // love means never missing a chance to check for memory corruption
  // Rev: Could make IsEyeCatcherValid() much more efficient
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
    return false; // Housekeeping, since SigIll doesn't return...
  }

  return ( (sessionError_.GetNumberOfReportableErrors () ) > 0 );
}

///////////////////////////////////////////////////////////////////////////////
///
//          SetError
///
/// \brief  Sets an error associated with this open
///
/// Sets an error condition associated with this open.  Error might be
/// from this EFH module, or might be set from another area as a result
/// of actions taken while processing this open (for example, a
/// communications error during an I/O).
///
/// Multiple errors can be stored in the same error vector.  Errors
/// are stored in a stack, with the first error stored being the one
/// that is reported.  Error numbers and additional error numbers are
/// not restricted to being POSIX E* errors, but care must be taken so
/// that the top error returned to STFS lib clients is the topmost
/// error stored in the error stack.
///
/// The errorIsHighest parameter can be used to put the current error
/// to the beginning of the stack, meaning it is the currently
/// reported error.  Care should be taken to ensure that this facility
/// is not overused: in general, errors should be reported in the
/// order in which they were received.
/// 
///
/// \param  pv_errorIsHighest      If true, then this error should go
///                                at the beginning of the error stack
///                                If false, then this error is added
///                                as the last error.  
/// \param  pv_errNo               The error number for this condition.
/// \param  pv_addlErr             Any secondary error associated with
///                                   this condition
/// \param  pp_contextBuf          A text array buf, suitable for
///                                   printing, that represents any
///                                   context that could be printed
///                                   for this condition.  If NULL,
///                                   then no context supplied.
/// \param  pv_contextBufLen       The length of the context buffer.
///                                   Must be 0 if no context buffer.
/// 
/// \retval off_t           SUCCESS: true
///                         ERROR:   false
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_Session::SetError ( bool pv_errorIsHighest,
                                    int pv_errNo, int pv_addlErr,
                                    char *pp_contextBuf, 
                                    size_t pv_contextBufLen) {

  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
    return false; // Housekeeping, since SigIll doesn't return...
  }

  Enum_ConditionSignificance lv_sig 
    = pv_errorIsHighest == true ? CondSig_ReplaceHighest : CondSig_Highest;

  bool status = sessionError_.PreserveCondition (lv_sig, pv_errNo, pv_addlErr,
                                              pp_contextBuf, pv_contextBufLen);


  return status;
}

///////////////////////////////////////////////////////////////////////////////
///
//          ResetErrors
///
/// \brief  Clear all errors from this EFH's error vector
///
/// This method resets the EFH's error condition list, freeing up any
/// stored error context buffers stored.  When it completes, there is
/// no record of any previous errors associated with this open.
/// 
/// \retval    NONE
///
///////////////////////////////////////////////////////////////////////////////
void 
STFS_Session:: ResetErrors () {

  // love means never missing a chance to check for memory corruption
  DEBUG_CHECK_EYECATCHER_VOID(this);

  sessionError_.ReleaseAllConditions ();
}


