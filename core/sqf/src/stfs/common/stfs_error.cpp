///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_error.cpp
/// \brief   Implementation of STFS error reporting for both lib and daemon, 
///   
/// This file contains the STFS error implementation.  It includes
/// STFS_Error, STFSError_ReportableError, and STFSError_Context
/// classes.
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
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// Rev: Maybe we directly use the STFS trace macros
#include "seabed/trace.h"

#include "stfs_error.h"
#include "stfs_sigill.h"
using namespace STFS;


/////////////////////////////////////////////////////////////////////////////
//  STFS_Error class methods
/////////////////////////////////////////////////////////////////////////////


//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFS_Error Constructor
///
/// \brief  Initializes an STFS_Error
///
///  Sets up the initial STFS error structure, setting condition
///  counts and extraction offsets to zero.  An initial error contains
///  no reportable errors.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFS_Error::STFS_Error()
  :
  STFS_Root (STFS_Root::EC_Error),
  currentExtractOffset_ (0),
  highCondCount_ (0),
  modCondCount_ (0),
  lowCondCount_ (0)
{
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_Error Destructor
///
/// \brief  Frees an STFS_Error
///
///   Disposes of an STFS_Error object.  As a side effect, it deletes
///   all reportable errors and any associated context buffers
/// 
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFS_Error::~STFS_Error(void) {

  // Release All Conditions verifies that we've got a valid eyecatcher

  ReleaseAllConditions ();
}


///////////////////////////////////////////////////////////////////////////////
///
//          PreserveCondition
///
/// \brief  Preserve an error/exceptional condition 
///
/// Preserves the reported condition in the STFS_Error stack, within
/// the scope of a single STFS API procedure or single daemon
/// operation.  Condition is stored by significance and order
/// received.  Additional error and textual context buffer supported
/// as well. The error numbers and context buffer information is
/// copied to a new location so that the context buffer does not need
/// to remain in scope in the calling client.
///
/// \param pv_CondSig        Condition Significance
/// \param pv_errNo          The error number to report
/// \param pv_additionalErr  An additional error number if needed
/// \param pp_context        Context text buffer, suitable for spewage.
/// \param pv_contextLen     Size of the buffer to spew
/// 
/// \retval int    TRUE:    The condition was preserved     \n
///                FAILURE: The condition was not preserved, most likely
///                           because no memory was available
///
///////////////////////////////////////////////////////////////////////////////
bool STFS_Error::PreserveCondition ( Enum_ConditionSignificance pv_CondSig,
                                     int pv_errNo,
                                     int pv_additionalErr,
                                     char *pp_context,
                                     size_t pv_contextLen
                                     )
{
  ///////////////////////////////////////////
  // validate eye catcher
  ///////////////////////////////////////////

  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
    return false; // Housekeeping, since SigIll doesn't return...
  }

  ///////////////////////////////////////////
  // validate parameters
  ///////////////////////////////////////////

  if (pv_CondSig >=CondSig_Last){
    ASSERT (false); // programming error, ignore in the wild
    return false;
  }


  ///////////////////////////////////////////
  // allocate new vector entry and initialize
  ///////////////////////////////////////////

  // we need to allocate a persistent context buffer that gets freed
  // when the error structure is released.  Caller can do what they
  // want with the buffer they passed after we return.

  STFSError_Context *lp_persistentContext
            = new STFSError_Context (pp_context, pv_contextLen);
  //Rev: check allocation failure

  STFSError_ReportableError *lp_localErrEntry
    = new STFSError_ReportableError (pv_CondSig, pv_errNo, pv_additionalErr,
                                     lp_persistentContext);

  ///////////////////////////////////////////
  // insert where approprate
  ///////////////////////////////////////////

  bool lv_errorInserted = InsertReportableError (lp_localErrEntry);


  ///////////////////////////////////////////
  // verify no memory corruption
  ///////////////////////////////////////////

  // validate eye catcher to make sure we didn't break anything

  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
    return false; // Housekeeping, since SigIll doesn't return...
  }


  ///////////////////////////////////////////
  //exit
  ///////////////////////////////////////////

  if ( lv_errorInserted == false) {
    // couldn't insert!  Need to clean up...

    delete lp_localErrEntry;

    // Rev: Free lp_persistentContext

  }

  return lv_errorInserted;

}


///////////////////////////////////////////////////////////////////////////////
///
//          ReleaseAllConditions
///
/// \brief  Clears all the reportable errors from an STFS_Error variable
///
/// This method resets the STFS_Error class so that it no longer contains
///   any conditions. Any allocated context buffers are also deleted.
///
/// \param void              No parameters
///
/// \retval void             Nothing returned
///
///////////////////////////////////////////////////////////////////////////////
void STFS_Error::ReleaseAllConditions (void)
{

  ///////////////////////////////////////////
  // Check our data structures
  ///////////////////////////////////////////

  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }


  ///////////////////////////////////////////
  // push exception handler for STL calls
  ///////////////////////////////////////////

  try {

    // nothing to get rid of if the error vector is empty

    if (!errVector_.empty() ) {

      ///////////////////////////////////////////
      // loop over the reportable errors while there are any remaining
      ///////////////////////////////////////////

      errStructVector_Def::iterator repErrElt = errVector_.begin();

      do {      // while not at the end of the vector

        STFSError_ReportableError *lp_localErrEntry 
          = (STFSError_ReportableError *) *repErrElt;
        

        if (!lp_localErrEntry->IsEyeCatcherValid()) {
          // Memory corruption!
          STFS_SigIll();
        }

        Enum_ConditionSignificance lv_repErrSig;
        int    lv_repErrNo;
        int    lv_repAddlErrNo;
        STFSError_Context  *lp_repContextBuf;
        
        bool success = lp_localErrEntry->GetErrInfo (&lv_repErrSig, &lv_repErrNo,
                                                     &lv_repAddlErrNo,
                                                     &lp_repContextBuf);
        
        if (!success) {
          // there should be an error here!  Die!
          STFS_SigIll();
        }
        
        // deallocate its context buffer
        
        if (lp_repContextBuf != NULL){
          delete lp_repContextBuf;
        }

        // erase this one from the vector

        repErrElt = errVector_.erase (repErrElt);
        
        // decrement significance counters since this error is gone
        DecrementSignificanceCounter ( lv_repErrSig );

        // Free the memory
        delete lp_localErrEntry;

      } //do
      // Rev: could be (!(repErrElt == errVector_.end()))
      while ( repErrElt < errVector_.end() ) ;

    }  // if !errVector_.empty ()

  } // try


  ///////////////////////////////////////////
  // catch any exceptions
  ///////////////////////////////////////////
  catch (...) {
    STFS_SigIll();
  }


  ///////////////////////////////////////////
  // reset significance counters 
  ///////////////////////////////////////////


  // In debug, we can also assert they're already 0, but it's too
  // important to get wrong if they're not, so we'll reset them 
  // anyway

  ASSERT (highCondCount_ == 0);
  ASSERT (modCondCount_ == 0);
  ASSERT (lowCondCount_ == 0);


  highCondCount_ = 0;
  modCondCount_  = 0;
  lowCondCount_  = 0;

  ///////////////////////////////////////////
  // reset extract offest
  ///////////////////////////////////////////

  currentExtractOffset_ = 0;

}


///////////////////////////////////////////////////////////////////////////////
///
//          ExtractCondition
///
/// \brief  Iterator that returns a single reported error
///
/// The function is an iterator over the reportable errors associated
/// with this error variable.  It can be called repeatedly until it
/// returns FALSE, indicating that there are no more errors to be
/// returned.  The position within the error buffers is maintaned by
/// the class; callers have no control over its value.
///
/// \param pp_CondSig        The significance of the reported error
/// \param pp_errNo          The error number for the reported error
/// \param pp_additionErr    Any additional error no for the reported error
/// \param pp_context        User buffer to copy any context info into. Null is OK.
/// \param pv_contextLenMax  Max size that can be copied into the context buf
/// \param pv_contextLenOut  Actual number of bytes copied into the context buf
///
/// \retval true             Indicates a valid error is returned
/// \retval false            Indicates that there are no more valid errors 
///                          to return.
///
///////////////////////////////////////////////////////////////////////////////

bool STFS_Error::ExtractCondition ( Enum_ConditionSignificance *pv_CondSig,
                                    int *pv_errNo,
                                    int *pv_additionErr,
                                    char *pp_context,
                                    size_t pv_contextLenMax,
                                    size_t *pv_contextLenOut ) {

  ///////////////////////////////////////////
  // verify all structures/parameters valid
  ///////////////////////////////////////////

  if (!IsEyeCatcherValid() == true) {
    // memory corruption!
    STFS_SigIll();
    return false;
  }

  if (pv_contextLenMax>0 && pp_context == NULL ) {
    // Rev: Make it ASSERT
    // Parameters in error
    STFS_SigIll();
    return false;
  }

  ///////////////////////////////////////////
  //try block for vector exceptions
  ///////////////////////////////////////////

  try {

    // Make sure we're not beyond the end of the list

    if ((int) errVector_.size() <= (currentExtractOffset_) ) {
      // we're at/past the end of the vector, nothing to return
      return false;
    }

    ///////////////////////////////////////////
    // Get the current error
    ///////////////////////////////////////////

    errStructVector_Def::iterator repErrElt;

    repErrElt = errVector_.begin() + currentExtractOffset_;
    //Rev: Assert if repErrElt.end()

    STFSError_ReportableError *lp_localErrEntry
      = (STFSError_ReportableError *) *repErrElt;

    // We need pointers for our copy of the context info that we'll copy into the
    // user's buffer

    STFSError_Context *lp_storedContext;

    ///////////////////////////////////////////
    // Get the user parameters
    ///////////////////////////////////////////

    bool success = lp_localErrEntry->GetErrInfo (pv_CondSig, pv_errNo, pv_additionErr, 
                                                 &lp_storedContext);
  
    if (!success) {
      // there ought to be a reportable error here!  Die!
      STFS_SigIll();
      return false;
    }

    ///////////////////////////////////////////
    // copy the context if needed
    ///////////////////////////////////////////

    if (pv_contextLenOut != NULL &&pp_context != NULL) {
      success = lp_storedContext->GetContext (pp_context, pv_contextLenMax,
                                              pv_contextLenOut);

      if (!success) {
        // we didn't copy any context, set the len to 0.
        *pv_contextLenOut = 0;
      }
    }

  }

  ///////////////////////////////////////////
  // Catch any exceptions thrown
  ///////////////////////////////////////////

  catch (...) {
    STFS_SigIll();
    return false ;
  }


  ///////////////////////////////////////////
  // Bump the current error extraction significance
  ///////////////////////////////////////////
  currentExtractOffset_++;

  ///////////////////////////////////////////
  // Finis!
  ///////////////////////////////////////////

  return true;
}


///////////////////////////////////////////////////////////////////////////////
///
//          ResetExtractPosition
///
/// \brief  Resets the extract position to the first (highest) error
///
/// This method resets the position in the collection of the
/// reportable errors to the first (highest) error.  No other
/// positions are currently supported.
///
/// \param void              No parameters
///
/// \retval void             Nothing returned
///
///////////////////////////////////////////////////////////////////////////////

void STFS_Error :: ResetExtractPosition (void) {

  ///////////////////////////////////////////
  // Check out our data structures
  ///////////////////////////////////////////

  if ( !(IsEyeCatcherValid()) == true) {
    // Memory Corruption!
    STFS_SigIll();
  }

  ///////////////////////////////////////////
  // Reset the counter
  ///////////////////////////////////////////

  currentExtractOffset_ = 0;

  ///////////////////////////////////////////
  // Finis!
  ///////////////////////////////////////////

}

///////////////////////////////////////////////////////////////////////////////
///
//          GetNumberOfReportableErrors
///
/// \brief  Returns the number of reportable errors for this instance
///
/// This method determines the total number of reportable errors
/// stacked in this instance.
///
/// \param void              No parameters
///
/// \retval >=1              Returns the count of errors, 
/// \retval 0                If no errors or a problem occurred retrieving the
///                             count
///
///////////////////////////////////////////////////////////////////////////////

size_t STFS_Error::GetNumberOfReportableErrors (void) {

  ///////////////////////////////////////////
  // check for validity
  ///////////////////////////////////////////

  if ( !(IsEyeCatcherValid()) == true) {
    // Memory Corruption!
    STFS_SigIll();
    return 0;
  }

  ///////////////////////////////////////////
  // get the count of errors
  ///////////////////////////////////////////
  // Rev: Have a try/catch block (ASSERT in catcher)
  return (errVector_.size());

}

///////////////////////////////////////////////////////////////////////////////
///
//          GetHighestReportedErrNo
///
/// \brief  Returns the error number from the most-significant error
///         in the vector
///
/// This method looks at the error no for the first entry in the error
/// vector, i.e, the most significant error.  It does not reset the
/// extract position.
///
/// \param void              No parameters
///
/// \retval error number     Returns the error number from the
///                          vector.  Will be 0 if error vector
///                          contains no errors. 
///
///////////////////////////////////////////////////////////////////////////////

int STFS_Error::GetHighestReportedErrNo (void) {

  ///////////////////////////////////////////
  // check for validity
  ///////////////////////////////////////////

  if ( !(IsEyeCatcherValid()) == true) {
    // Memory Corruption!
    STFS_SigIll();
    return 0;
  }

  ///////////////////////////////////////////
  // Check to see if we have any errors
  ///////////////////////////////////////////


  // Rev: Have a try/catch block surrounding vector.front() also
  if (errVector_.size() == 0 ) {
    return 0;
  }

  ///////////////////////////////////////////
  // get the error number from entry 0 on the v
  ///////////////////////////////////////////

  STFSError_ReportableError *lp_localErrEntry
    = (STFSError_ReportableError * )errVector_.front();

  int lv_localErrNo = 0;

  bool success = lp_localErrEntry->GetErrInfo (NULL, &lv_localErrNo, NULL, NULL);

  if (!success) {
    // there ought to be a reportable error here since the size isn't 0
    STFS_SigIll();
  }

  return (lv_localErrNo);

}

///////////////////////////////////////////////////////////////////////////////
///
//          Pack
///
/// \brief Translates the error into a format that can be sent across
///         process boudaries
///
///  This method packs the error structure into the provided buffer.
///  After a successful conclusion to this method, the error structure
///  can be sent across process boundaries and unpacked by the
///  recipient.
///
///  A single error reply can contain multiple errors, each of which might
///  include a variable-length context field.  So the variable portion
///  of the message contains an array of offsets to the beginning of each error
///  structure.  This is similar to FFM packing.
///
///
/// \param   pp_buf          Pointer to the buf to put the err struct in
/// \param   pv_bufLen       Maximum length of the buffer for packing
/// \param   pv_packedLen    The length of the error structure.
///
/// \retval true if the entire error structure could be packed, false otherwise.
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_Error::Pack ( char *pp_buf, size_t pv_bufLen, size_t *pv_packedLen) {

  ///////////////////////////////////////////
  // validate eye catcher
  ///////////////////////////////////////////

  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
    return false; // Housekeeping, since SigIll doesn't return...
  }

  //////////////////////////////////
  /// Validate Parameters
  //////////////////////////////////

  // we're in the bowels of code that we control, so any missing
  // parameters are programming errors, since they were all set up way
  // above us and checked on the way down...

  ASSERT (pp_buf != NULL && pv_bufLen != 0 && pv_packedLen != NULL);

  //////////////////////////////////
  ///  Housekeeping
  //////////////////////////////////

  char  *lp_copyPtr = pp_buf;

  size_t lv_copyBufConsumed = 0;
  size_t lv_copySpaceAvail = pv_bufLen - lv_copyBufConsumed;

  size_t lv_packedErrorLen = 0;


  //////////////////////////////////
  ///  Get number of errors to report
  //////////////////////////////////

  size_t lv_numErrors = GetNumberOfReportableErrors();

  if (lv_numErrors == 0) {
    //  HUH?  An error reply with no errors???  Well alrighty then!
    ASSERT (false);

    //  Theoretically, we could pretend to pack it and return true.  But for the
    //  first implementation, honesty is the best policy...
    return false;
  }

  //////////////////////////////////
  ///  Put the number of packed errors into the message
  //////////////////////////////////
  if (lv_copySpaceAvail > sizeof (lv_numErrors)) {
    memcpy (lp_copyPtr, &lv_numErrors, sizeof (lv_numErrors) );

    lp_copyPtr += sizeof (lv_numErrors);
    lv_copySpaceAvail  -= sizeof (lv_numErrors);
    lv_copyBufConsumed += sizeof (lv_numErrors);
    lv_packedErrorLen  += sizeof (lv_numErrors);
  }
  else {
    // no room to pack
    return false;
  }



  //////////////////////////////////
  ///  Set up space for the error table offset struct
  //////////////////////////////////

  // the error Offset array pointer
  varoffset_t *lp_errorOffsetArray = (varoffset_t *)lp_copyPtr;

  //  Current position in the buffer skips over the offset array

  lv_copyBufConsumed += (lv_numErrors * sizeof (varoffset_t));
  lp_copyPtr += (lv_numErrors * sizeof (varoffset_t) );


  //////////////////////////////////
  ///  Set up the loop variables
  //////////////////////////////////

  // Rev: try/catch
  // Rev: Remove the errVector_.begin();
  errStructVector_Def::iterator repErrElt = errVector_.begin();

  //////////////////////////////////
  ///  Loop over the errors
  //////////////////////////////////
  for (int lv_errCounter = 0; lv_errCounter < (int) lv_numErrors; lv_errCounter++)  {


    repErrElt = errVector_.begin() + lv_errCounter;
        
    STFSError_ReportableError *lp_localErrEntry 
      = (STFSError_ReportableError *) *repErrElt;

    if (lp_localErrEntry == NULL) {
      ASSERT (false);
      STFS_SigIll();
    }
        

    if (!lp_localErrEntry->IsEyeCatcherValid()) {
      // Memory corruption!
      STFS_SigIll();
    }

    //////////////////////////////////
    ///  Put the offset into the offset array
    //////////////////////////////////

    lp_errorOffsetArray[lv_errCounter] = (varoffset_t) (lp_copyPtr) -(varoffset_t) pp_buf;


#ifdef DEBUG
    //  A nice little sanity check: is the length of the previous
    //  offset entry - this one equal to the packed errorLen?  We miss
    //  checking the very last entry, but we'll cope...

    if (lv_errCounter>0) {
      ASSERT  (lp_errorOffsetArray[lv_errCounter] 
	       - lp_errorOffsetArray[lv_errCounter-1] == (int) lv_packedErrorLen);
    }
#endif    

    lv_packedErrorLen = 0;

    //////////////////////////////////
    /// Get the error condition
    //////////////////////////////////
    
    Enum_ConditionSignificance lv_repErrSig;
    int    lv_repErrNo;
    int    lv_repAddlErrNo;
    STFSError_Context  *lp_repContextBuf;

    bool success = lp_localErrEntry->GetErrInfo (&lv_repErrSig, &lv_repErrNo,
                                                 &lv_repAddlErrNo,
                                                 &lp_repContextBuf);
    
    if (!success) {
      // there should be an error here!  Die!
      STFS_SigIll();
    }
        
    //////////////////////////////////
    ///  Pack the condition significance
    //////////////////////////////////

    if (lv_copySpaceAvail > sizeof (lv_repErrSig)) {
      memcpy (lp_copyPtr, &lv_repErrSig, sizeof (lv_repErrSig) );
      lp_copyPtr += sizeof (lv_repErrSig);
      lv_copySpaceAvail  -= sizeof(lv_repErrSig);
      lv_copyBufConsumed += sizeof (lv_repErrSig);
      lv_packedErrorLen  += sizeof (lv_repErrSig);
    }
    else {
      // no room to pack
      return false;
    }

    //////////////////////////////////
    ///  Pack the error number
    //////////////////////////////////

    if (lv_copySpaceAvail > sizeof (lv_repErrNo)) {
      memcpy (lp_copyPtr, &lv_repErrNo, sizeof (lv_repErrNo) );

      lp_copyPtr += sizeof (lv_repErrNo);
      lv_copySpaceAvail  -= sizeof(lv_repErrNo);
      lv_copyBufConsumed += sizeof (lv_repErrNo);
      lv_packedErrorLen  += sizeof (lv_repErrNo);
    }
    else {
      // no room to pack
      return false;
    }


    //////////////////////////////////
    ///  Pack the additional error number
    //////////////////////////////////

    if (lv_copySpaceAvail > sizeof (lv_repAddlErrNo)) {
      memcpy (lp_copyPtr, &lv_repAddlErrNo, sizeof (lv_repAddlErrNo) );
      lp_copyPtr += sizeof (lv_repAddlErrNo);
      lv_copySpaceAvail  -= sizeof(lv_repAddlErrNo);
      lv_copyBufConsumed += sizeof (lv_repAddlErrNo);
      lv_packedErrorLen  += sizeof (lv_repAddlErrNo);
    }
    else {
      // no room to pack
      return false;
    }
    //////////////////////////////////
    ///  Pack the context buffer string
    //////////////////////////////////

    
    //  Packing the context is a variable length string...  We don't
    //  have a pointer to it, so we just use GetContext

    size_t lv_contextLen = lp_repContextBuf->GetContextLen();

    // Rev: Use a define/enum
    if (lv_contextLen > 32677) {
      // Context too long!  How could that be??
      ASSERT (false);
      return false;
    }

    short lv_contextLenS = (short) lv_contextLen;
    // Rev: could use a  sizeof(lv_contextLenS)
    size_t lv_contextPackedLen = lv_contextLen + sizeof (varoffset_t);

    if (lv_copySpaceAvail > lv_contextPackedLen) {

      // put in the length indicator
      memcpy (lp_copyPtr, &lv_contextLenS, sizeof (short));
      lp_copyPtr += sizeof (short);
      lv_copySpaceAvail  -= sizeof(short);
      lv_copyBufConsumed += sizeof (short);
      lv_packedErrorLen  += sizeof (short);

      // now the actual context itself

      lp_repContextBuf->GetContext (lp_copyPtr, lv_copySpaceAvail, &lv_contextLen);
     
      //  sanity check!
      ASSERT (lv_contextLen == size_t (lv_contextLenS) );

      lp_copyPtr += lv_contextLen;
      lv_copySpaceAvail  -= lv_contextLen;
      lv_copyBufConsumed += lv_contextLen;
      lv_packedErrorLen  += lv_contextLen;
    }
    else {
      // no room to pack
      return false;
    }

    //////////////////////////////////
    ///  Bottom of loop
    //////////////////////////////////

  }// for loop


  //////////////////////////////////
  ///  Set return variables
  //////////////////////////////////

  *pv_packedLen = lv_copyBufConsumed;

  //////////////////////////////////
  /// Finis!
  //////////////////////////////////


  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Unpack
///
/// \brief Translates a buffer containing a packed error structure
///            back into an STFS_Error class object
///
///  This method takes a buffer containing an error structure with one
///  or more error conditions and unpacks it into an error structure,
///  rebuilding the error vector as needed.
///
///  A single error reply can contain multiple errors, each of which might
///  include a variable-length context field.  So the variable portion
///  of the message contains an array of offsets to the beginning of each error
///  structure that needs to be deconstructed into the actual errors
///  on the vector.  This is similar to FFM unpacking.
///
///
/// \param   pp_buf          Pointer to the buf to with the err struct in
/// \param   pv_bufLen       Length of that buffer
///
/// \retval True if unpack was successful, fals otherwise
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_Error::Unpack (char *pp_buf, size_t pv_bufLen) {

  ///////////////////////////////////////////
  // validate eye catcher
  ///////////////////////////////////////////

  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
    return false; // Housekeeping, since SigIll doesn't return...
  }

  //////////////////////////////
  ///  Parameter validation
  //////////////////////////////

  if (pp_buf == NULL || pv_bufLen == 0) {
    // we need a buf with contents to unpack!
    ASSERT (false);
    return false;
  }

  //////////////////////////////
  ///  Housekeeping variables
  //////////////////////////////


  char *lp_variableBufStart = pp_buf;
  char *lp_copyPtr = pp_buf;


  //////////////////////////////
  // Extract the number of errors are stored in the structure
  //////////////////////////////

  size_t lv_numErrors = 0;
  memcpy (&lv_numErrors, lp_copyPtr, sizeof (lv_numErrors));
  lp_copyPtr += sizeof (lv_numErrors);

  // I'm not comfortable with this level of validation, but it's better than
  // nothing.... 

  if (lv_numErrors > STFSERROR_MAXERRS) {
    STFS_SigIll();
    return false;
  }

  if (lv_numErrors == 0) {
    // no errors here to unpack!
    return false;
  }

  //////////////////////////////
  ///  find the offsetArray
  //////////////////////////////

  varoffset_t *lp_offsetArray = (varoffset_t *) lp_copyPtr;

  //////////////////////////////
  /// loop over the errors in the buffer
  //////////////////////////////

  char *lp_packedErrCopyLoc = NULL;
  int lv_mainError = 0;
  Enum_ConditionSignificance lv_condSig = CondSig_Last;
  int lv_addlError = 0;
  short lv_contextLen;
  char *lp_contextPtr;

  // Rev: Verify that lp_packedErrCopyLoc is within (pp_buf + pv_bufLen)
  for (int lv_errNum = 0; lv_errNum < (int) lv_numErrors; lv_errNum++) {

    //////////////////////////////
    /// Translate Array offset to packed error structure
    //////////////////////////////

    lp_packedErrCopyLoc = lp_variableBufStart + lp_offsetArray[lv_errNum];

    //////////////////////////////
    /// Extract the condition significance
    //////////////////////////////

    memcpy (&lv_condSig, lp_packedErrCopyLoc, sizeof (lv_condSig));
    lp_packedErrCopyLoc += sizeof (lv_condSig);

    //////////////////////////////
    /// Extract the error number
    //////////////////////////////

    memcpy (&lv_mainError, lp_packedErrCopyLoc, sizeof (lv_mainError));
    lp_packedErrCopyLoc += sizeof (lv_mainError);

    //////////////////////////////
    ///  Extract the additional error number
    //////////////////////////////

    memcpy (&lv_addlError, lp_packedErrCopyLoc, sizeof (lv_addlError));
    lp_packedErrCopyLoc += sizeof (lv_addlError);
            
    //////////////////////////////
    ///  Extract the context information
    //////////////////////////////

    memcpy (&lv_contextLen, lp_packedErrCopyLoc, sizeof (short));
    lp_packedErrCopyLoc += sizeof (short);

    if (lv_contextLen > 0) {
      lp_contextPtr = lp_packedErrCopyLoc;
    }
    else {
      lp_contextPtr = NULL;
    }

    //////////////////////////////
    /// Build the error struct control information
    //////////////////////////////
    bool lv_success = PreserveCondition ( lv_condSig, lv_mainError, lv_addlError,
                                          lp_contextPtr, lv_contextLen);

    if (lv_success != true) {
      // Rev: Maybe we dont call ReleaseAllConditions()
      ReleaseAllConditions();
      return false;
    }

  //////////////////////////////
  /// End loop
  //////////////////////////////

    // bump our copy loc in case we need it after this
    lp_copyPtr = lp_packedErrCopyLoc + lv_contextLen;

  }  // For {lv_errNum = 0....

  //////////////////////////////
  ///    Finis!
  //////////////////////////////

  return true;
}


//***********************
// Private Methods
//***********************


///*************************************
bool 
STFS_Error::InsertReportableError (STFSError_ReportableError *pp_newErrEntry) {

  //////////////////////////////////////////
  // Check params and structures
  //////////////////////////////////////////



  if (pp_newErrEntry == NULL) {
    STFS_SigIll();
    return false;
  }

  if (pp_newErrEntry->IsEyeCatcherValid() != true) {
    STFS_SigIll();
    return false;
  }

  //////////////////////////////////////////
  // figure out where to insert the new entry
  //////////////////////////////////////////

  try {

    errStructVector_Def::iterator lv_locationToInsert = errVector_.begin();
    
    switch (pp_newErrEntry->GetCondSig()) {

    case CondSig_ReplaceHighest: {
      // no need to bump lv_locationToInsert because this one goes at the head
      break;
    }
 
    case CondSig_Highest: {
      lv_locationToInsert = lv_locationToInsert + highCondCount_;
      break;
    }

    case CondSig_Moderate: {
      // skip over the highs and any mods
      lv_locationToInsert = lv_locationToInsert + highCondCount_ + modCondCount_;
      break;
    }

    case CondSig_Low: {
      lv_locationToInsert = lv_locationToInsert
                                + highCondCount_      // skip over highs
                                + modCondCount_      // skip over mods
                                + lowCondCount_;      // skip over existing lows
      break;
    }

    case CondSig_Last:
    default: {
      STFS_SigIll();
      return false;
      
    }
    }
    /////////////////////
    // insert into vector
    /////////////////////


    errVector_.insert (lv_locationToInsert, 
                       pp_newErrEntry);

    // bump the counter if we succeeded
    
    IncrementSignificanceCounter (pp_newErrEntry->GetCondSig() );
  
  }
  catch (...) {
    return false;
  }

  ////////////////////////
  // return success
  ////////////////////////


  return true;
}


///*************************************
void STFS_Error::IncrementSignificanceCounter (Enum_ConditionSignificance pv_condSig) {

  int *localCounter = FindConditionSignificanceCounter (pv_condSig);
  (*localCounter)++;
}


///*************************************
void STFS_Error::DecrementSignificanceCounter (Enum_ConditionSignificance pv_condSig) {

  int *localCounter = FindConditionSignificanceCounter (pv_condSig);
  (*localCounter)--;
}

///*************************************
int * STFS_Error ::FindConditionSignificanceCounter (Enum_ConditionSignificance pv_condSig) {

  switch (pv_condSig) {

  case CondSig_ReplaceHighest:
  case CondSig_Highest: {
    return &highCondCount_;
  }

  case CondSig_Moderate: {
    return &modCondCount_;
  }

  case CondSig_Low: {
    return &lowCondCount_;
  }

  case CondSig_Last:
  default: {
    STFS_SigIll();
    return NULL;  // completeness' sake
  }

  } // switch

}


///**************************************
bool STFS_Error::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid (STFS_Root::EC_Error);
}


/////////////////////////////////////////////////////////////////////////////
//  STFSError_ReportableError class methods
/////////////////////////////////////////////////////////////////////////////


//***********************
// Public methods
//***********************

///////////////////////////////////////////////////////////////////////////////
///
//          STFSError_ReportableError Default Constructor
///
/// \brief  Initializes an STFSError_ReportableError with no error
///
///  Sets up the initial ReportableError structure, setting all error
///  information to 0 and the context buffer to NULL.
///
/// \param     void
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSError_ReportableError::STFSError_ReportableError ()
  :
  STFS_Root (STFS_Root::EC_Error),
  significance_ (CondSig_ReplaceHighest),
  errNo_ (0),
  additionalErr_ (0),
  context_ (NULL){

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSError_ReportableError Constructor
///
/// \brief  Initializes an STFSError_ReportableError with an established error
///
///  Sets up the initial ReportableError structure, using values
///  specified to report the error.
///
/// \param     pv_condSig                  The condition significance
/// \param     pv_errNo                    The error number to report
/// \param     pv_additionalErr            An additional error number if needed
/// \param     pp_context                  Pointer to the context object
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSError_ReportableError
           ::STFSError_ReportableError(Enum_ConditionSignificance pv_condSig, 
                                       int pv_errNo, int pv_additionalErr,
                                       STFSError_Context *pp_context)
             :  STFS_Root (STFS_Root::EC_Error)
 {
  significance_ = pv_condSig;
  errNo_ = pv_errNo;
  additionalErr_ = pv_additionalErr;
  context_ = pp_context;

}
                                                   


///////////////////////////////////////////////////////////////////////////////
///
//          GetErrInfo
///
/// \brief  Extractor that gets the information for an error
///
///  Extracts the information from a reportable error into user
///  variables.  specified to report the error.  If a parameter
///  pointer is null, then its value is not extracted.
///
/// \param     pv_condSig                  The condition significance
/// \param     pv_errNo                    The error number to report
/// \param     pv_additionalErr            An additional error number if needed
/// \param     pp_context                  Pointer to the context 
///
/// \retval    TRUE                        Error info successfully extracted
/// \retval    FALSE                       No error info retrieved because structure
///                                              was invalid.
///
///////////////////////////////////////////////////////////////////////////////

bool STFSError_ReportableError::GetErrInfo (Enum_ConditionSignificance *pv_condSig, 
                                            int *pv_errNo, int *pv_additionalErr, 
                                            STFSError_Context **pp_context) {

  ///////////////////////////////////////////
  // Validate Parameters
  ///////////////////////////////////////////

  if (!IsEyeCatcherValid() == true ) {
    STFS_SigIll();
    return false; 
  }

  ///////////////////////////////////////////
  //copy parameters
  ///////////////////////////////////////////

  if (pv_condSig != NULL) {
    *pv_condSig = significance_;
  }

  if (pv_errNo != NULL) {
    *pv_errNo = errNo_;
  }

  if (pv_additionalErr != NULL) {
    *pv_additionalErr = additionalErr_;
  }

  if ( pp_context != NULL ) {
    *pp_context = context_;
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////
///
//          SetErrInfo
///
/// \brief  Set up the values for an existing error
///
///  Set the error information as specified in the parameters.
///  Overwrites any information already stored in the object.  As a
///  side effect, if there was already a context buffer allocated, it
///  is freed before being overwritten.
///
/// \param     pv_condSig                  The condition significance
/// \param     pv_errNo                    The error number to report
/// \param     pv_additionalErr            An additional error number if needed
/// \param     pp_context                  Pointer to the context 
///
/// \retval    TRUE                        Error info successfully set
/// \retval    FALSE                       No error info set because structure
///                                              or parameters were invalid
///
///////////////////////////////////////////////////////////////////////////////

bool STFSError_ReportableError::SetErrInfo (Enum_ConditionSignificance pv_condSig, 
                                            int pv_errNo, int pv_additionalErr, 
                                            STFSError_Context *pp_context) {

  ///////////////////////////////////////////
  // check memory, parameters
  ///////////////////////////////////////////

  if ( !IsEyeCatcherValid() == true ) {
    STFS_SigIll();
    return false;
  }

  // We validated all parameters in the caller, so parameter errors
  // indicate programming errors or memory corruption or other horrible
  // fates...

  if (pv_condSig >= CondSig_Last) {
    ASSERT (false); // programming error, ignore in the wild
    STFS_SigIll();
    return false;
  }


  ///////////////////////////////////////////
  // set new error values
  ///////////////////////////////////////////

  significance_ = pv_condSig;
  errNo_ = pv_errNo;
  additionalErr_ = pv_additionalErr;

  ///////////////////////////////////////////
  // deal with overwriting a context buffer if needed
  ///////////////////////////////////////////

  // Rev: Check if we consistently 'own' context_
  if (context_->IsEmpty() == false) {

    context_->DeleteContext();
  }


  ///////////////////////////////////////////
  // set up new context buffer
  ///////////////////////////////////////////


  //  *** MEMORY LEAK???? ***  Did we deallocate the previous 
  // context structure?  Check!

  context_ = pp_context;

  ///////////////////////////////////////////
  // finis!
  ///////////////////////////////////////////
  return true;

}


///////////////////////////////////////////////////////////////////////////////
///
//          GetCondSig
///
/// \brief  Gets Only The Condition Significance
///
///    Returns the condition significance from this instance.  Could
///    be INLINE if we forego the memory corruption check.
///
/// \param     void                        No parameters
///
/// \retval    EnumConditionSignificance   Value from this instance
///
///////////////////////////////////////////////////////////////////////////////

Enum_ConditionSignificance STFSError_ReportableError::GetCondSig (void) {

  ///////////////////////////////////////////
  // Check for memory corruption
  ///////////////////////////////////////////

  if (IsEyeCatcherValid() != true) {
    STFS_SigIll();
    return CondSig_Last;
  }

  return significance_;
}


///**************************************
bool STFSError_ReportableError::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid (STFS_Root::EC_Error);
}

//***********************
// Private Methods
//***********************



/////////////////////////////////////////////////////////////////////////////
//  STFSError_Context class methods
/////////////////////////////////////////////////////////////////////////////


//***********************
// Public methods
//***********************

///////////////////////////////////////////////////////////////////////////////
///
//          STFSError_Context Default Constructor
///
/// \brief  Initializes an STFSError_Context buffer
///
///  Sets up an initial, empty STFSError_Context buffer.  No memory is
///  allocated.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSError_Context::STFSError_Context () :
  context_ (NULL),
  contextLen_ (0){

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSError_Context Constructor
///
/// \brief  Initializes an STFSError_Context buffer with specifed context
///
///  Sets up an initial STFSError_Context buffer with the context
///  supplied.  The supplied context is copied to a new buffer managed
///  within the STFSError_Context class.  If we can't get memory, we
///  don't save the context.  We'll probably die shortly anyway.
/// \n
///  The context buffer is presumed to be a single string in 
///  form.  If contextLen is longer than STFSERROR_CONTEXTMAXLEN, it
///  will be truncated.
/// \n
///  This is a utility class, so context information might be lost if
///  parameters are passed incorrectly.  In particular, if 0 is passed
///  for length, then no context is saved and any data in the context
///  buffer is ignored.  If a non-zero lenght is passed but the
///  context buffer pointer is null, then no context is saved.  The
///  latter is flagged by an assert in the debug version
///
/// \param     pp_context                    The context data buffer
/// \param     pv_contextLen                 The context data buffer
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSError_Context::STFSError_Context (char *pp_context, size_t pv_contextLen) {

  ///////////////////////////////////////////
  // validate the parameters
  ///////////////////////////////////////////

  context_ = NULL;
  contextLen_ = 0; 

  if ( (pp_context != NULL) && (pv_contextLen != 0) ){

    if (SetContext ( pp_context, pv_contextLen) == false ) {

      // Upgezorked!  We're out of memory!
      context_ = NULL;
      contextLen_ = 0;
    }
  }

  ///////////////////////////////////////////
  // finis!
  ///////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSError_Context Destructor
///
/// \brief  Disposes of STFSError_Context object
///
///   Deletes an STFSError_Context object.  It deletes the associated
///   context buffer as well.
/// 
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSError_Context::~STFSError_Context () {

  DeleteContext();

}


///////////////////////////////////////////////////////////////////////////////
///
//          SetContext
///
/// \brief  Sets the context to the appropriate value
///
///   Sets the context to the parameters.  Any existing context is
///   overwritten.
/// 
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

bool STFSError_Context::SetContext (char *pp_context, size_t pv_contextLen) {

  ///////////////////////////////////////////
  //delete any existing context
  ///////////////////////////////////////////

  DeleteContext();

  ///////////////////////////////////////////
  // Allocate the context buffer
  ///////////////////////////////////////////

  size_t lv_contextSize = (STFSERROR_CONTEXTLENMAX < pv_contextLen) 
                                    ? STFSERROR_CONTEXTLENMAX : pv_contextLen;

  // we need to allocate a persistent context buffer that gets freed
  // when the context structure is released. If things go south from
  // here and the error isn't inserted, we need to free it,
  // otherwise we've got memory leaks...

  char *lp_persistentContextBuf = new char[lv_contextSize];

  if ( lp_persistentContextBuf != NULL ) {

    // Copy the context buffer
    // Should we check to make sure that strlen is the same as
    // lv_contextLen? Or do memcpy instead?
      
    // Rev: strncpy (lp_persistentContextBuf, pp_context, lv_contextSize -1) & then set the last byte to 0
    // Rev: To be able to use binary data as part of context, change this to a 'memcpy'
    //      and in other places where we are using strcpy  or any string related function call
    strncpy (lp_persistentContextBuf, pp_context, lv_contextSize);

    context_ = lp_persistentContextBuf;
    contextLen_ = lv_contextSize;
  }

  else {
    // No memory! 
    // overkill to set these because DeleteContext did.  But let's be
    // really, really sure they're NULL/0!

    context_ = NULL;
    contextLen_ = 0;

    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetContext
///
/// \brief  Extractor gets a context buffer and size from the object
///
///  GetContext extracts the current context information stored in the
///  object.  The context from the object is copied into the user
///  buffer for its size up to the supplied maximum length (which
///  includes the null terminator).
///
/// \param     pp_tgtContext               The buffer to copy the context to
/// \param     pv_contextLenMax            The maximum context length
/// \param     pv_contextLen               The actual length copied to the buffer
///
/// \retval    TRUE                        Context info successfully extracted
/// \retval    FALSE                       No error info retrieved because context
///                                              was not present or other problem
///
///////////////////////////////////////////////////////////////////////////////

bool STFSError_Context::GetContext (char *pp_tgtContext, 
                                    size_t  pv_contextLenMax, 
                                    size_t *pv_contextLen) {

  ///////////////////////////////////////////
  // validate parameters
  ///////////////////////////////////////////

  // Rev: check on pv_contextLen also
  if ( (pp_tgtContext==NULL) || (pv_contextLenMax == 0)) {
    // parameter problem
    ASSERT (false);
    STFS_SigIll();
    return false;
  }

  ///////////////////////////////////////////
  // copy the context up to the maximum length.  We know the stored
  // length is less than max...
  ///////////////////////////////////////////

  size_t lv_copyLen = (pv_contextLenMax-1 < contextLen_) 
                              ? pv_contextLenMax-1 : contextLen_;

  // Rev: use memcpy
  strncpy (pp_tgtContext, context_, lv_copyLen );

  // Rev: remove
  pp_tgtContext[lv_copyLen] = 0;

  *pv_contextLen = lv_copyLen;

  ///////////////////////////////////////////
  // finis!
  ///////////////////////////////////////////
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetContextLen
///
/// \brief  Extracts the context length from a context buffer
///
/// \param     void
///
/// \retval    context length.  Might be 0 if no context stored
///
///////////////////////////////////////////////////////////////////////////////
size_t
STFSError_Context::GetContextLen (void) {
  return (contextLen_);
}
///////////////////////////////////////////////////////////////////////////////
///
//          DeleteContext
///
/// \brief  Releases the current context buffer
///
///   This method deletes the current context buffer if there is one.
///   If one has not been allocated, this method simply returns.
/// 
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

void STFSError_Context::DeleteContext(void) {

  // Rev: Copy the assert from IsEmpty(void)
  if ( context_ != NULL) {
    delete [] context_;

  }

  context_ = NULL; // Yes, we really mean it!
  contextLen_ = 0;

}

///////////////////////////////////////////////////////////////////////////////
///
//          IsEmpty
///
/// \brief  Determine if the context buffer contains information
///
///   This method determines if the context buffer has no data in it.
/// 
/// \param     void
///
/// \retval    TRUE                 if the context buffer is empty
/// \retval    FALSE                if the context buffer has data in it
///
///////////////////////////////////////////////////////////////////////////////

bool STFSError_Context::IsEmpty (void) {

  // this method does no production checking to verify that context_
  // and contextLen_ are of the same opinion as to whether the buffer
  // is allocated.  That is, there's no assumption that contextLen_ is
  // 0 then context_ is NULL, or vice versa.  Instead, it relies on
  // contextLen_ as the final arbiter.  We manage both, so any
  // out-of-sync indicates a bug on our part.  Future versions could
  // get fancier than the simple assert here.


  ASSERT ( (contextLen_ == 0 && context_ == NULL) ||
           (contextLen_ != 0 && context_ != NULL) );

  return contextLen_ == 0 ? true : false;

}



//***********************
// Private Methods
//***********************


