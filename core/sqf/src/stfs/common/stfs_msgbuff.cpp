///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_msgbuff.cpp
/// \brief   Implementation of STFS interprocess fixed size message structures
///   
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

#include "seabed/trace.h"

#include "stfs_defs.h"
#include "stfs_message.h"
#include "stfs_msgbuff.h"
#include "stfs_util.h"
#include "stfs_sigill.h"

using namespace STFS;

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_CommonHeader class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::Init Default Constructor
///
/// \brief  Initializes the common header to defaults
///
///  Sets up the message header that it common to all STFS message requests and
///  replies.  The default constructure sets the version to the current default
///  and sets other values to invalid.  Use the parameterized constructor to set
///  these values in a single call, or the various set-methods to set them after
///  calling the default constructor.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::Init() {
  msgHeader_.msgType = MT_Invalid;
  strcpy (msgHeader_.eyeCatcher, STFS_MSGBUFEYECATCHER);
  msgHeader_.msgVersion = STFS_MSGTEMPLATEVERSION;
  msgHeader_.msgPath =MPT_Invalid;
  msgHeader_.requesterNodeId_ = -1;
  msgHeader_.requesterPID_ = -1;
  msgHeader_.hasVarSection = false;
  msgHeader_.varSectionBaseOffset = 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::Init Parameterized Init
///
/// \brief  Initializes the common header to specified values.
///
///  Sets up the message header that it common to all STFS message requests and
///  replies using the values that the user specifies and default values for
///  version and eyecatcher.
///
/// \param     pv_messageType              The message type for this message
/// \param     pv_messagePathType          The message's source and destination.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::Init (Enum_MessageType pv_messageType,
                               Enum_MessagePathType pv_messagePathType,
                               int pv_requesterNodeId, 
                               int pv_requesterPID){
  msgHeader_.msgType = pv_messageType;
  strcpy (msgHeader_.eyeCatcher, STFS_MSGBUFEYECATCHER);
  msgHeader_.msgVersion = STFS_MSGTEMPLATEVERSION;
  msgHeader_.msgPath = pv_messagePathType;
  msgHeader_.requesterNodeId_ = pv_requesterNodeId;
  msgHeader_.requesterPID_ = pv_requesterPID;
  msgHeader_.hasVarSection = false;
  msgHeader_.varSectionBaseOffset = 0;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::Release Destructor
///
/// \brief  Frees an STFSMsgPath_CommonHeader
///
///   Clears and deletes the common header.  No memory is allocated here, so
///   there's not really much to do.  But if the eyecatcher isn't valid, we'll
///   die a horrible death anyway....
/// 
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::Release (void) {

  Validate();
  // nothing to free, we're done now...
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::SetMessageType
///
/// \brief  Sets the message type in the common header
///
///   Sets the common header's message type.  If the eyecatcher isn't valid, it
///   dies a horrible death.
/// 
/// \param     pv_MessageType           The new message type
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::SetMessageType (Enum_MessageType pv_MessageType) {

  Validate();
  msgHeader_.msgType = pv_MessageType;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::SetMessageVersion
///
/// \brief  Sets the message version in the common header
///
///   Sets the common header's message version.  If the eyecatcher isn't valid, it
///   dies a horrible death.
/// 
/// \param     pv_MessageVersion        The new message Version
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::SetMessageVersion (int pv_MessageVersion) {

  Validate();

  msgHeader_.msgVersion = pv_MessageVersion;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::SetMessagePath
///
/// \brief  Sets the message path in the common header
///
///   Sets the common header's message path.  If the eyecatcher isn't valid, it
///   dies a horrible death.
/// 
/// \param     pv_MessagePath           The new message path
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::SetMessagePath (Enum_MessagePathType pv_MessagePath) {

  Validate();

  msgHeader_.msgPath = pv_MessagePath;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::SetRequesterNodeId
///
/// \brief  Sets the Requester Node Id 
///

/// \param     pv_NodeId    
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader:: SetRequesterNodeId ( int pv_NodeId ) {

  Validate();

  msgHeader_.requesterNodeId_ = pv_NodeId;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::SetRequesterPID
///
/// \brief  Sets the Requester PID for a create file request
///
/// \param     pv_PID
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader:: SetRequesterPID ( int pv_PID ) {

  Validate();

  msgHeader_.requesterPID_ = pv_PID;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::SetVariableInfo
///
/// \brief  Sets information about the variable section from the message header.
///
///   Sets the common header's variable information.  This method should be used
///   in conjunction with the appropriate STFSMsgBufV_* methods to initialize
///   the variable part of the message buffer.
///
///   If the eyecatcher isn't valid, it dies a horrible death.
/// 
///
/// \param      pv_hasVariableInfo   If true, then this message has a variable
///                                  section actively allocated.  
/// \param      pv_varOffsetBase     The offset from the beginning of the
///                                  message buffer to to the start of the
///                                  variable offset section. 
///
/// \retval      void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::SetVariableInfo (bool pv_hasVariableInfo,
                                          varoffset_t pv_varOffsetBase)
 {

  Validate();

  msgHeader_.hasVarSection = pv_hasVariableInfo;

  if (pv_varOffsetBase < 0) {
    /// how did we get a negative offset???
    STFS_SigIll();
  }

  msgHeader_.varSectionBaseOffset = pv_varOffsetBase;
}



///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::GetMessageType
///
/// \brief  Gets the message type in the common header
///
///   Gets the common header's message type.  If the eyecatcher isn't valid, it
///   dies a horrible death.
/// 
/// \param      void
///
/// \retval     pv_MessageType           The message type
///
///////////////////////////////////////////////////////////////////////////////
Enum_MessageType
STFSMsgBuf_CommonHeader::GetMessageType () {

  Validate();

  return (msgHeader_.msgType);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::GetMessageVersion
///
/// \brief  Gets the message version from the common header
///
///   Gets the common header's message version.  If the eyecatcher isn't valid, it
///   dies a horrible death.
/// 
/// \param      void
///
/// \retval     pv_MessageVersion        The message Version
///
///////////////////////////////////////////////////////////////////////////////
int
STFSMsgBuf_CommonHeader::GetMessageVersion () {

  Validate();

  return( msgHeader_.msgVersion );
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::GetMessagePath
///
/// \brief  Gets the message path from the common header
///
///   Gets the common header's message path.  If the eyecatcher isn't valid, it
///   dies a horrible death.
/// 
/// \param      void
///
/// \retval     pv_MessagePath           The message path
///
///////////////////////////////////////////////////////////////////////////////
Enum_MessagePathType
STFSMsgBuf_CommonHeader::GetMessagePath () {

  Validate();
  return (msgHeader_.msgPath);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::GetRequesterNodeId
///
/// \brief  Gets the Node Id of the requester process
///
/// \param     void 
///
/// \retval    pv_NodeId
///
///////////////////////////////////////////////////////////////////////////////
int
STFSMsgBuf_CommonHeader:: GetRequesterNodeId (void) {

  Validate();

  return (msgHeader_.requesterNodeId_);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::GetRequesterPID
///
/// \brief  Gets the PID of the requester process
///
/// \param     void 
///
/// \retval    pv_PID
///
///////////////////////////////////////////////////////////////////////////////
int
STFSMsgBuf_CommonHeader:: GetRequesterPID (void) {

  Validate();

  return (msgHeader_.requesterPID_);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::GetVariableInfo
///
/// \brief  Gets information about the variable section from the message header.
///
///   Gets the common header's variable information.  If the eyecatcher isn't
///   valid, it dies a horrible death.
/// 
///
/// \param      pv_hasVariableInfo   If true, then this message has a variable
///                                  section actively allocated.  Pass NULL if
///                                  not interested in this value
/// \param      pv_varOffsetBase     The offset from the beginning of the
///                                  message buffer to to the start of the
///                                  variable offset section. Pass NULL if not
///                                  interested in this value
///
/// \retval      void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::GetVariableInfo (bool *pv_hasVariableInfo,
                                          varoffset_t *pv_varOffsetBase)
 {

  Validate();

  if (pv_hasVariableInfo != NULL) {
    *pv_hasVariableInfo = msgHeader_.hasVarSection;
  }

  if (pv_varOffsetBase != NULL) {
    *pv_varOffsetBase = msgHeader_.varSectionBaseOffset;
  }
}



///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::EyeCatcherIsValid
///
/// \brief  Checks that the eyecatcher is the expected value
///
///   Checks the value in the msgHeader_ eyecatcher to determine if it is the
///   expected value. 
/// 
/// \param      void
///
/// \retval     true if eyecatcher is valid, false otherwise.
///
///////////////////////////////////////////////////////////////////////////////
bool
STFSMsgBuf_CommonHeader::EyeCatcherIsValid () {

  return (strcmp (msgHeader_.eyeCatcher, STFS_MSGBUFEYECATCHER) == 0);
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CommonHeader::Validate
///
/// \brief  Verifies that the common message header is valid, aborting if not
///
///   This method checks to verify that the message buffer is valid.  If it is
///   not valid, the method calls STFS_SigIll.  This method should not be used
///   to detect recoverable errors.  Instead, use the EyeCatcherIsValid method
///   if continued processing is considered normal.
/// 
/// \param      void
///
/// \retval     true if eyecatcher is valid, false otherwise.
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CommonHeader::Validate () {

  if ( !EyeCatcherIsValid() == true) {
    ASSERT (false);
    STFS_SigIll();
  }
}


//***********************
// Private methods
//***********************




/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_MetadataContainer class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::Init Default Constructor
///
/// \brief  Initializes the MetadataContainer  to defaults
///
///  Sets up the metadata container to default values.  The MetadataContainer is
///  the fixed-length overhead control structure sent in any message that
///  includes the file and fragment metdata (EFM and FFMs).  The actual EFM and
///  FFM data is in the variable-length area.  The default constructure sets the
///  values to null, as if there were no metadata in the variable length area.
///  Use the parameterized constructor to set these values in a single call, or
///  the various set methods to set them after calling the default constructor.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_MetadataContainer::Init() {
  metadataPackage_.EFMOffset = 0;  
  metadataPackage_.numFragments = 0;
  metadataPackage_.FFMOffset = 0;
  metadataPackage_.totalFFMSize = 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::Init Parameterized Constructor
///
/// \brief  Initializes the MetadataContainer to specified values
///
///  Sets up the metdata header to specified values, plus other defaults as
///  needed.  The parameterized constructure sets the values as specified, but
///  does nothing to the variable-length area.  
///
/// \param    pv_EFMOffset          Offset to the start of the EFM
/// \param    pv_numFragments       Number of FFMs in this message
/// \param    pv_FFMOffset          Offset to the first FFM
/// \param    pv_totalFFMSize       Total number of bytes of FFM data
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_MetadataContainer::Init (varoffset_t pv_EFMOffset,
                                    int pv_numFragments,
                                    varoffset_t pv_FFMOffset, 
                                    size_t pv_totalFFMSize) {

  /////////////////////
  /// Set EFM Offset
  /////////////////////

  if (pv_EFMOffset > 0) {
    metadataPackage_.EFMOffset = pv_EFMOffset;  
  }
  else{
    metadataPackage_.EFMOffset = pv_EFMOffset;  
  }

  /////////////////////
  /// Set num fragments
  /////////////////////

  if (pv_numFragments > 0) {
    metadataPackage_.numFragments = pv_numFragments;
  }
  else {
    metadataPackage_.numFragments = 0;
  }


  /////////////////////
  /// Set ffm offset
  /////////////////////

  if (pv_FFMOffset > 0) {
    metadataPackage_.FFMOffset = pv_FFMOffset;
  }
  else {
    metadataPackage_.FFMOffset = 0;
  }

  /////////////////////
  /// Set num fragments
  /////////////////////

  if (pv_totalFFMSize > 0) {
    metadataPackage_.totalFFMSize = pv_totalFFMSize;
  }
  else {
  metadataPackage_.totalFFMSize = 0;
  }
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::Release Destructor 
///
/// \brief  Frees an STFSMsgPath_MetadataContainer
///
///   Clears and deletes the metadata container.  Doesn't do anything with data
///   in the variable length area.
/// 
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_MetadataContainer::Release (void) {
  // nothing to free, we're done now...
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::SetEFMOffset
///
/// \brief  Sets the current EFMOffset to the specified value
///
///   This method sets the current EFMOffset to the value specified.  It
///   overwrites any previous value.  It does not do anything in the
///   variable-length area.  It's up to the caller to ensure that the EFM offset
///   is maintained consistently with the variable length area.
/// 
/// \param     pv_EFMOffset          The new value for EFMOffset
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_MetadataContainer::SetEFMOffset (varoffset_t pv_EFMOffset) {
  if (pv_EFMOffset < 0) {
    // what are we doing with a negative offset???
    STFS_SigIll();
  }

  metadataPackage_.EFMOffset = pv_EFMOffset;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::SetNumFragments
///
/// \brief  Sets the current number of fragments to the specified value
///
///   This method sets the current number of fragments to the value specified.
///   It overwrites any previous value.  It does not do anything in the
///   variable-length area.  It's up to the caller to ensure that the number of
///   fragments is maintained consistently with the variable length area.
/// 
/// \param     pv_numFragments          The new value for the number of fragments
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_MetadataContainer::SetNumFragments (varoffset_t pv_numFragments) {
  if (pv_numFragments < 0) {
    // what are we doing with a negative number of fragments???
    STFS_SigIll();
  }

  metadataPackage_.numFragments = pv_numFragments;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::SetFFMOffset
///
/// \brief  Sets the current FFMOffset to the specified value
///
///   This method sets the current FFMOffset to the value specified.  It
///   overwrites any previous value.  It does not do anything in the
///   variable-length area.  It's up to the caller to ensure that the FFM offset
///   is maintained consistently with the variable length area.
///
///   The FFM offset is the offset from the start of the variable length area to
///   the start of the first FFM.
/// 
/// \param     pv_FFMOffset          The new value for FFMOffset
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_MetadataContainer::SetFFMOffset (varoffset_t pv_FFMOffset) {
  if (pv_FFMOffset < 0) {
    // what are we doing with a negative offset???
    STFS_SigIll();
  }

  metadataPackage_.FFMOffset = pv_FFMOffset;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::SetTotalFFMSize
///
/// \brief  Sets the current total FFM size to the specified value
///
///   This method sets the current total FFM size to the value specified.
///   It overwrites any previous value.  It does not do anything in the
///   variable-length area.  It's up to the caller to ensure that the total FFM
///   size is
///   maintained consistently with the variable length area.
///
///   The total FFM size is the size in bytes occupied by all FFMs, including
///   any padding space to force optimal data alignment for an individual FFM.
/// 
/// \param     pv_totalFFMSize       The new value for the total FFM size
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_MetadataContainer::SetTotalFFMSize (size_t pv_totalFFMSize) {

  // pv_totalFFMSize is unsigned, no need to make sure > 0...

  metadataPackage_.totalFFMSize = pv_totalFFMSize;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::GetEFMOffset
///
/// \brief  Gets the current EFMOffset from the Metadata Container
///
/// \param     void 
///
/// \retval    pv_EFMOffset        The EFM offset
///
///////////////////////////////////////////////////////////////////////////////
varoffset_t
STFSMsgBuf_MetadataContainer::GetEFMOffset (void) {

  return ( metadataPackage_.EFMOffset );
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::GetNumFragments
///
/// \brief  Gets the current number of fragments
/// 
/// \param     void
///
/// \retval    pv_numFragments          The number of fragments
///
///////////////////////////////////////////////////////////////////////////////
varoffset_t
STFSMsgBuf_MetadataContainer::GetNumFragments (void) {

  return ( metadataPackage_.numFragments );

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::GetFFMOffset
///
/// \brief  Gets the current FFMOffset from the metadata container
///
/// \param      void
///
/// \retval     pv_FFMOffset          The current FFMOffset
///
///////////////////////////////////////////////////////////////////////////////
varoffset_t
STFSMsgBuf_MetadataContainer::GetFFMOffset (void) {

  return ( metadataPackage_.FFMOffset );

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_MetadataContainer::GetTotalFFMSize
///
/// \brief  Gets the current total FFM size
///
/// \param      void
///
/// \retval     pv_totalFFMSize       The current total FFM size
///
///////////////////////////////////////////////////////////////////////////////
size_t
STFSMsgBuf_MetadataContainer::GetTotalFFMSize (void) {

  return (metadataPackage_.totalFFMSize);

}

//**********************
// Private methods
//**********************


/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_ErrorReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::Init Default Constructor
///
/// \brief  Initializes an error reply message buffer to defaults
///
///  Sets up an error reply message buffer fixed length area to defaults.  It
///  doesn't do anything to the variable-length area for an error reply message
///  buffer.  
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_ErrorReply::Init() {

  errorReply_.msgHeader.Init (MT_ErrorReply, MPT_LibToDaemon, -1, -1);

  // we'll need to do something here eventually when we support daemon-to-daemon
  // communications -- need to change the MPT...

  errorReply_.reportedError = 0;
  errorReply_.numErrors = 0;
  errorReply_.offsetToErrorVectorOffsets = 0;
  errorReply_.offsetToFirstPackedErrorStruct = 0;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::Init Parameterized Constructor
///
/// \brief  Initializes an error reply message buffer to the supplied error
///
///  Sets up an error reply message buffer fixed length area to report the
///  supplied error.  It doesn't do anything to the variable-length area for an
///  error reply message buffer.  It's up to the caller to ensure that the
///  variable area is set commensurate with fixed length portions.
///
/// \param     pp_Error            The error to be reported
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_ErrorReply::Init(STFS_Error *pp_Error) {

  errorReply_.msgHeader.Init (MT_ErrorReply, MPT_LibToDaemon, -1, -1);

  if (STFS_util::executingInDaemon_ == true) {
    errorReply_.msgHeader.SetMessagePath (MPT_DaemonToDaemon);
  }

  errorReply_.reportedError = pp_Error->GetHighestReportedErrNo();
  errorReply_.numErrors = pp_Error->GetNumberOfReportableErrors();
  errorReply_.offsetToErrorVectorOffsets = 0;
  errorReply_.offsetToFirstPackedErrorStruct = 0;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::Release Destructor
///
/// \brief  Destructor for ErrorReply File Request fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_ErrorReply::Release() {

  errorReply_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::SetNumErrors
///
/// \brief  Sets the current number of errors to the specified value.
///
///   This method sets the current number of errors to the value specified.  It
///   overwrites any previous value.  It does not do anything in the
///   variable-length area.
///
/// 
/// \param     pv_NumberOfErrors     The new number of errors
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_ErrorReply::SetNumErrors (int pv_numberOfErrors) {

  errorReply_.msgHeader.Validate();

  if (pv_numberOfErrors < 0) {
    // what are we doing with a negative number of errors?
    STFS_SigIll();
  }

  errorReply_.numErrors = pv_numberOfErrors;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::SetReportedError
///
/// \brief  Sets the reported error  to the specified value.
///
///   This method sets the reported error to the value specified.  It
///   overwrites any previous value.  It does not do anything in the
///   variable-length area.
///
/// 
/// \param     pv_reportedError     The new reported error
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_ErrorReply::SetReportedError (int pv_reportedError) {

  errorReply_.msgHeader.Validate();
  errorReply_.reportedError = pv_reportedError;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::SetOffsetToErrorVectorOffsets
///
/// \brief  Sets the offest to the start of the Error Vector offset to the
///         specified value. 
///
///   This method sets the current offset to the Error Vector Offsets in the
///   variable portion of the messag to the value specified.  It overwrites any
///   previous value.  It does not do anything in the variable-length area.
///   It's up to the caller to ensure that the Offsets Error Vector Offsets
///   array is set up correctly and at the offset specified by this call.
///
/// 
/// \param     pv_EVOffset     The new offset to the ErrorVectorOffsets
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_ErrorReply::SetOffsetToErrorVectorOffsets (varoffset_t  pv_EVOffset) {
  if (pv_EVOffset < 0) {
    // what are we doing with a negative offset?
    STFS_SigIll();
  }

  errorReply_.msgHeader.Validate();

  errorReply_.offsetToErrorVectorOffsets = pv_EVOffset;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::SetOffsetToFirstPackedErrorStruct
///
/// \brief  Sets the offset to the first packed error structure in the variable
///         area.
///
///   This method sets the offset to the first packed error structure in the
///   variable area to the value specified.  It overwrites any previous value.
///   It does not do anything in the variable-length area.  It's up to the
///   caller to ensure that the variable area is maintained consistently.
///
/// 
/// \param     pv_firstPacked     The new offset to the first packed error
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_ErrorReply::
    SetOffsetToFirstPackedErrorStruct (varoffset_t pv_firstPacked) {

  errorReply_.msgHeader.Validate();

  if (pv_firstPacked < 0) {
    // what are we doing with a negative offset???
    STFS_SigIll();
  }

  errorReply_.offsetToFirstPackedErrorStruct = pv_firstPacked;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::GetNumErrors
///
/// \brief  Gets the current number of errors from the message
///
/// \param     void
///
/// \retval    pv_NumberOfErrors     Number of errors
///
///////////////////////////////////////////////////////////////////////////////
int
STFSMsgBuf_ErrorReply::GetNumErrors (void) {

  errorReply_.msgHeader.Validate();

  return ( errorReply_.numErrors );
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::GetReportedError
///
/// \brief  Gets the reported error from the message
///
/// \param     void
///
/// \retval    pv_reportedError     The reported error
///
///////////////////////////////////////////////////////////////////////////////
int
STFSMsgBuf_ErrorReply::GetReportedError (void) {

  errorReply_.msgHeader.Validate();

  return (errorReply_.reportedError);
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::GetOffsetToErrorVectorOffsets
///
/// \brief  Gets the offset to the start of the Error Vector Offset Array
///
/// \param     void
///
/// \retval    pv_EVOffset     The offset to the ErrorVectorOffsets
///
///////////////////////////////////////////////////////////////////////////////
varoffset_t
STFSMsgBuf_ErrorReply::GetOffsetToErrorVectorOffsets (void) {

  errorReply_.msgHeader.Validate();

  return (errorReply_.offsetToErrorVectorOffsets);
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_ErrorReply::GetOffsetToFirstPackedErrorStruct
///
/// \brief  Gets the offset to the first packed error structure in the variable
///         area.
///
/// \param     void
///
/// \retval    pv_firstPacked     The offset to the first packed error
///
///////////////////////////////////////////////////////////////////////////////
varoffset_t
STFSMsgBuf_ErrorReply::GetOffsetToFirstPackedErrorStruct (void) {

  errorReply_.msgHeader.Validate();

  return (  errorReply_.offsetToFirstPackedErrorStruct);

}


/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_CreateFileReq class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReq::Init Default Constructor
///
/// \brief  Initializes a create file request fixed message buffer to defaults
///
///  Sets up a create file request message buffer fixed length area to
///  defaults (no template name, mkstemp request).  There is no
///  variable area for a CreateFile Request, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReq::Init() {

  createFileReq_.msgHeader.Init (MT_CreateFile,MPT_LibToDaemon,-1, -1);

  createFileReq_.template_[0]= '\0';
  createFileReq_.isMkstemp = true;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReq::Init Parameterized Constructor
///
/// \brief  Initializes a create file request fixed message buffer to defaults
///
///  Sets up a create file request message buffer fixed length area to specified
///  values (no template name, mkstemp request).  There is no variable area for
///  a CreateFile Request, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReq::Init(char *pp_template, 
                               bool  pp_isMkstemp,
                               int   pv_requesterNodeId,
                               int   pv_requesterPID) {

  // LibToDaemon is only choice for MPT until we support creating an STFS FILE,
  // not fragment, on another node.  For now, STFS files can only be created
  // local to the library that's creating them.

  createFileReq_.msgHeader.Init (MT_CreateFile,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
  createFileReq_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  // We'll let STFSd decide whether the name is really too long. Here we just
  // decide whether or not it fits in the message.

  if (strlen (pp_template) > STFS_NAME_MAX-1 ){
    // string too long in constructor!
    ASSERT (false);
  }

  strncpy (createFileReq_.template_, pp_template, STFS_NAME_MAX-1);
  createFileReq_.template_[STFS_NAME_MAX-1] = '\0';

  createFileReq_.isMkstemp = pp_isMkstemp;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReq::Release Destructor
///
/// \brief  Destructor for Create File Request fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReq::Release() {

  createFileReq_.msgHeader.Release();

  // nothing to free, we're done now...

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReq::SetTemplate
///
/// \brief  Sets the file name template for the create file request
///
///   This method sets the template in the message buffer to the specified
///   value, overwriting any name already in there.  It asserts that the string
///   passed is shorter than STFS_MAX_NAMELEN-1 bytes, and copies only that
///   amount of data.  The name is not otherwise checked; any validation is done
///   at higher levels or in STFSd.
/// 
/// \param     pp_template     The file name template
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReq:: SetTemplate ( char * pp_template ) {

  createFileReq_.msgHeader.Validate();

  if (strlen (pp_template) > STFS_NAME_MAX-1 ){
    // string too long in constructor!
    ASSERT (false);
  }

  strncpy (createFileReq_.template_, pp_template, STFS_NAME_MAX-1);
  createFileReq_.template_[STFS_NAME_MAX-1] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReq::SetIsMkstemp
///
/// \brief  Sets the flag indicating an mkstemp request for a create file request
///
///   This method sets the flag indicating whether the create request is for a
///   normal create or an mkstemp request.  In the latter case, the file name is
///   extended with a uniquifier at return.
/// 
/// \param     pv_isMkstemp     The boolean flag.  True -> mkstemp request.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReq:: SetIsMkstemp ( bool pv_isMkstemp ) {

  createFileReq_.msgHeader.Validate();

  createFileReq_.isMkstemp = pv_isMkstemp;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReq::GetTemplate
///
/// \brief  Gets the file name template for the create file request
///
///   This method retrieves the file name template from a create file request
///   and copies it into the user-supplied buffer.
/// 
/// \param     pp_tgtTemplate     The file name template
/// \param     pv_templateMaxSize Max number of bytes for the template
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReq:: GetTemplate ( char * pp_tgtTemplate,
                                         size_t pv_templateMaxSize ) {

  createFileReq_.msgHeader.Validate();

  if (strlen (createFileReq_.template_) > pv_templateMaxSize-1 ){
    strncpy (pp_tgtTemplate, createFileReq_.template_, pv_templateMaxSize-1);
    pp_tgtTemplate[pv_templateMaxSize-1] = '\0';
  }
  else {
    strcpy (pp_tgtTemplate, createFileReq_.template_);
  }
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReq::GetIsMkstemp
///
/// \brief  Gets the value of the flag indicating an mkstemp request 
///
///   This method retrieves the flag indicating whether the create request is for a
///   normal create or an mkstemp request.  In the latter case, the file name is
///   extended with a uniquifier at return.
/// 
/// \param     void 
///
/// \retval    pv_isMkstemp     The boolean flag.  True -> mkstemp request.
///
///////////////////////////////////////////////////////////////////////////////
bool
STFSMsgBuf_CreateFileReq:: GetIsMkstemp (void) {

  createFileReq_.msgHeader.Validate();

  return (createFileReq_.isMkstemp);
}


/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_CreateFileReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::Init Default Constructor
///
/// \brief  Initializes a create file reply fixed message buffer to defaults
///
///  Sets up a create file reply message buffer fixed length area to defaults.
///  A normal file create reply includes a variable area that has the EFM and
///  FFMs in it.  This area is not initialized as part of the constructor.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply::Init() {

  createFileReply_.msgHeader.Init (MT_CreateFileReply,MPT_LibToDaemon,-1,-1);
  createFileReply_.msgHeader.SetVariableInfo(false, // No variable section yet
                                             0);    // current var section offset

  createFileReply_.template_[0]= '\0';
  memset ( &createFileReply_.openIdentifier, -1, sizeof (STFS_OpenIdentifier) );
  createFileReply_.varAreaSize = 0;

  // use default constructor for the metadata info
  createFileReply_.metadataInfo.Init();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::Init Parameterized Constructor
///
/// \brief  Initializes a create file reply fixed message buffer to values
/// specified as parameters, plus defaults
///
///  Sets up a create file reply message buffer fixed length area to defaults.
///  A normal file create reply includes a variable area that has the EFM and
///  FFMs in it.  This area is not initialized as part of the constructor.
///
/// \param    pp_template     the file name template for the reply
/// \param    pp_openId       pointer to an openID for the message
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply ::Init(char *pp_template,
                                  STFS_OpenIdentifier *pp_openID) {

  createFileReply_.msgHeader.Init (MT_CreateFileReply, MPT_LibToDaemon,-1,-1);
  createFileReply_.msgHeader.SetVariableInfo (true, 
                                              STFS::GetSizeOfFixedMessageBuffer(MT_CreateFileReply));


  if ( strlen ( pp_template) > STFS_NAME_MAX ) {
    // Hey wait, that name came from STFS!  What'd we do wrong?
    ASSERT (false);
    STFS_SigIll();
  }

  strncpy (createFileReply_.template_, pp_template, STFS_NAME_MAX);

  ASSERT (pp_openID != NULL);
  if (pp_openID != NULL) {
    //createFileReply_.openIdentifier =  *pp_openID;
    memcpy(&createFileReply_.openIdentifier, pp_openID, sizeof (STFS_OpenIdentifier) );
  }
  else {
    // shouldn't reach here, but just in case...
    memset ( &createFileReply_.openIdentifier, -1, sizeof (STFS_OpenIdentifier) );
  }

  createFileReply_.varAreaSize = 0;

  // still use default constructor for the metadata info
  createFileReply_.metadataInfo.Init();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::Release Destructor
///
/// \brief  Destructor for Create File Reply
///
///  This is the destructor for the Create File Reply fixed buffer.  It doesn't
///  do anything.  Why is that noteworthy?  Well, in particular, it doesn't do
///  anything to clean up the variable area either.  They're not related.  But
///  after this destructor is called, the area is good as gone, because we've
///  cleaned up the metadata info.  Caveat Emptor.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply::Release() {

  createFileReply_.msgHeader.Release();
  // nothing to free, we're done now...

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::GetTemplate
///
/// \brief  Gets the file name template for the create file reply
///
///   This method retrieves the file name template from a create file reply
///   and copies it into the user-supplied buffer.
/// 
/// \param     pp_tgtTemplate     The file name template
/// \param     pv_templateMaxSize Max number of bytes for the template
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply::GetTemplate ( char * pp_tgtTemplate,
                                         size_t pv_templateMaxSize ) {

  createFileReply_.msgHeader.Validate();

  if (strlen (createFileReply_.template_) > pv_templateMaxSize-1 ){
    strncpy (pp_tgtTemplate, createFileReply_.template_, pv_templateMaxSize-1);
    pp_tgtTemplate[pv_templateMaxSize-1] = '\0';
  }
  else {
    strcpy (pp_tgtTemplate, createFileReply_.template_);
  }
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::GetOpenIdentifier
///
/// \brief  Gets the open identifier from the create file reply
///
/// \param     void
///
/// \retval    Returns the open identifier from the message. All bytes set to -1
///            if uninitialized.
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier
STFSMsgBuf_CreateFileReply::GetOpenIdentifier ( void ) {

  createFileReply_.msgHeader.Validate();

  return createFileReply_.openIdentifier;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::GetVarAreaSize
///
/// \brief  Gets the size of the variable area from the create file reply
///
/// \param     void
///
/// \retval    Returns the variable area size from the message.
///
///////////////////////////////////////////////////////////////////////////////
size_t
STFSMsgBuf_CreateFileReply::GetVarAreaSize ( void ) {

  createFileReply_.msgHeader.Validate();

  return (createFileReply_.varAreaSize);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::GetVarMetadataControl
///
/// \brief  Extracts the metdata control information from the message
///
/// \param     pp_EFMOffset        offset from beginning of msg to the EFM
/// \param     pp_numFragments     number of fragments/ffms in this message
/// \param     pp_firstFFMOffset   offset from beginning of msg to the first FFM
/// \param     pp_totalFFMSize     Size of all FFMs.
///
/// \retval    Returns the variable area size from the message.
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply
    ::GetVarMetadataControl (varoffset_t *pp_EFMOffset, 
                             int *pp_numFragments, 
                             varoffset_t *pp_firstFFMOffset, 
                             size_t *pp_totalFFMSize  ) {

  createFileReply_.msgHeader.Validate();

  if (pp_EFMOffset != NULL) {
    *pp_EFMOffset = createFileReply_.metadataInfo.GetEFMOffset();
  }

  if (pp_numFragments != NULL) {
    *pp_numFragments = createFileReply_.metadataInfo.GetNumFragments();
  }

  if (pp_firstFFMOffset != NULL) {
    *pp_firstFFMOffset = createFileReply_.metadataInfo.GetFFMOffset();
  }

  if (pp_totalFFMSize != NULL) {
    *pp_totalFFMSize = createFileReply_.metadataInfo.GetTotalFFMSize();
  }

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::SetTemplate
///
/// \brief  Sets the file name template for the create file reply
///
///   This method puts the supplied  file name template into a create file reply
///   overwriting any existing value there.
/// 
/// \param     pp_template     The file name template
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply::SetTemplate ( char * pp_template ) {

  createFileReply_.msgHeader.Validate();

  if (strlen (pp_template) > STFS_NAME_MAX-1 ){
    ASSERT (false);
  }

  strncpy (createFileReply_.template_, pp_template, STFS_NAME_MAX-1);
  createFileReply_.template_[STFS_NAME_MAX-1] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::SetOpenIdentifier
///
/// \brief  Sets the open identifier in the create file reply
///
///  This method sets the open identifer in a create file reply, overwriting any
///  existing open identifier.
///
/// \param     pp_openID      The address of the open ID to copy
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply::SetOpenIdentifier(STFS_OpenIdentifier *pp_openID ) {

  createFileReply_.msgHeader.Validate();

  if (pp_openID != NULL) {
    createFileReply_.openIdentifier = *pp_openID;
  }
  else {

    // huh?  Don't pass NULL here!

    ASSERT (false);

    // Just because we're paranoid doesn't mean they aren't out to get us.  So
    // we'll make it an invalid openid, just for safety
    memset ( &createFileReply_.openIdentifier, -1, sizeof (STFS_OpenIdentifier) );
  }

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::SetVarAreaSize
///
/// \brief  Sets the size of the variable area in the create file reply
///
/// \param     pv_varAreaSize       New variable area size
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply::SetVarAreaSize ( size_t pv_varAreaSize ) {

  createFileReply_.msgHeader.Validate();

  createFileReply_.varAreaSize = pv_varAreaSize;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFileReply::SetVarMetadataControl
///
/// \brief  Sets the metdata control information in the message
///
///  Sets the metadata control inforamtion in a create file reply.  Any existing
///  values are overwritten.  All values must be specified.
///
/// \param     pv_EFMOffset        offset from beginning of msg to the EFM
/// \param     pv_numFragments     number of fragments/ffms in this message
/// \param     pv_firstFFMOffset   offset from beginning of msg to the first FFM
/// \param     pv_totalFFMSize     Size of all FFMs.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFileReply
    ::SetVarMetadataControl (varoffset_t    pv_EFMOffset, 
                             int            pv_numFragments, 
                             varoffset_t    pv_firstFFMOffset, 
                             size_t         pv_totalFFMSize  ) {

  createFileReply_.msgHeader.Validate();

  createFileReply_.metadataInfo.Init (pv_EFMOffset,
                                      pv_numFragments,
                                      pv_firstFFMOffset,
                                      pv_totalFFMSize);
}


/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_CreateFragmentReq class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReq::Init Default Constructor
///
/// \brief  Initializes a create fragment request fixed message buffer to defaults
///
///  Sets up a create fragment request message buffer fixed length
///  area to defaults.  There's no variable area on a CreateFragmentReq, so it's
///  just a matter of setting up the headers to defaults and the template and
///  open identifier to null values.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReq::Init() {

  createFragmentReq_.msgHeader.Init (MT_CreateFragment,MPT_LibToDaemon,-1,-1);
  createFragmentReq_.msgHeader.SetVariableInfo(false, // No variable section yet
                                             0);    // current var section offset

  memset ( &createFragmentReq_.openIdentifier, -1, sizeof (STFS_OpenIdentifier) );

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReq::Init Parameterized Constructor
///
/// \brief  Initializes a create fragment fixed message buffer to values
/// specified as parameters, plus defaults
///
///  Sets up a create fragment request  message buffer fixed length area to the
///  values specified as parameters.  There is no variable area on a
///  CreateFragmentReq, so after this method has executed, the message is ready
///  to send.
///
/// \param    pp_openId       pointer to an openID for the message
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReq::Init(STFS_OpenIdentifier *pp_openID,
                                   int                  pv_requesterNodeId,
                                   int                  pv_requesterPID) {

  createFragmentReq_.msgHeader.Init (MT_CreateFragment, MPT_LibToDaemon,
                                     pv_requesterNodeId,pv_requesterPID);

  ASSERT (pp_openID != NULL);
  if (pp_openID != NULL) {
    memcpy(&createFragmentReq_.openIdentifier, pp_openID, sizeof (STFS_OpenIdentifier) );
  }
  else {
    // shouldn't reach here, but just in case...
    memset ( &createFragmentReq_.openIdentifier, -1, sizeof (STFS_OpenIdentifier) );
  }

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReq::Release Destructor
///
/// \brief  Destructor for Create Fragment Request
///
///  This is the destructor for the Create Fragment Request fixed buffer.  It doesn't
///  do anything.  There's no variable length buffer here, so no need to worry
///  about it getting "lost."
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReq::Release() {

  createFragmentReq_.msgHeader.Release();
  // nothing to free, we're done now...

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReq::GetOpenIdentifier
///
/// \brief  Gets the open identifier from the create fragment request
///
/// \param     void
///
/// \retval    Returns the open identifier from the message. All bytes set to -1
///            if uninitialized.
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier
STFSMsgBuf_CreateFragmentReq::GetOpenIdentifier ( void ) {

  createFragmentReq_.msgHeader.Validate();

  return createFragmentReq_.openIdentifier;

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReq::SetOpenIdentifier
///
/// \brief  Sets the open identifier in the create fragment request
///
///  This method sets the open identifer in a create fragment request, overwriting any
///  existing open identifier.
///
/// \param     pp_openID      The address of the open ID to copy
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReq::SetOpenIdentifier(STFS_OpenIdentifier *pp_openID ) {

  createFragmentReq_.msgHeader.Validate();

  if (pp_openID != NULL) {
    createFragmentReq_.openIdentifier = *pp_openID;
  }
  else {

    // huh?  Don't pass NULL here!

    ASSERT (false);

    // Just because we're paranoid doesn't mean they aren't out to get us.  So
    // we'll make it an invalid openid, just for safety
    memset ( &createFragmentReq_.openIdentifier, -1, sizeof (STFS_OpenIdentifier) );
  }

}



//*****************************************************************************
/////////////////////////////////////////////////////////////////////////////
///  STFSMsgBuf_CreateFragmentReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReply::Init Default Constructor
///
/// \brief  Initializes a create fragment reply fixed message buffer to defaults
///
///  Sets up a create fragment reply message buffer fixed length area to
///  defaults.  A normal create fragment reply includes a variable area that has
///  the EFM and FFMs in it.  This area is not initialized as part of the
///  constructor.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReply::Init() {

  createFragmentReply_.msgHeader.Init (MT_CreateFragmentReply,MPT_LibToDaemon,-1,-1);
  createFragmentReply_.msgHeader.SetVariableInfo(false, // No variable section yet
                                                 0);    // current var section offset

  createFragmentReply_.varAreaSize = 0;

  // use default constructor for the metadata info
  createFragmentReply_.metadataInfo.Init();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReply::Release Destructor
///
/// \brief  Destructor for Create File Reply
///
///  This is the destructor for the Create Fragment Reply fixed buffer.  It
///  doesn't do anything.  Why is that noteworthy?  Well, in particular, it
///  doesn't do anything to clean up the variable area either.  They're not
///  related.  But after this destructor is called, the area is good as gone,
///  because we've cleaned up the metadata info.  Caveat Emptor.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReply::Release() {

  createFragmentReply_.msgHeader.Release();
  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReply::GetVarAreaSize
///
/// \brief  Gets the size of the variable area from the create fragment reply
///
/// \param     void
///
/// \retval    Returns the variable area size from the message.
///
///////////////////////////////////////////////////////////////////////////////
size_t
STFSMsgBuf_CreateFragmentReply::GetVarAreaSize ( void ) {

  createFragmentReply_.msgHeader.Validate();

  return (createFragmentReply_.varAreaSize);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReply::GetVarMetadataControl
///
/// \brief  Extracts the metdata control information from the message
///
/// \param     pp_EFMOffset        offset from beginning of msg to the EFM
/// \param     pp_numFragments     number of fragments/ffms in this message
/// \param     pp_firstFFMOffset   offset from beginning of msg to the first FFM
/// \param     pp_totalFFMSize     Size of all FFMs.
///
/// \retval    Returns the variable area size from the message.
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReply
    ::GetVarMetadataControl (varoffset_t *pp_EFMOffset, 
                             int *pp_numFragments, 
                             varoffset_t *pp_firstFFMOffset, 
                             size_t *pp_totalFFMSize  ) {

  createFragmentReply_.msgHeader.Validate();

  if (pp_EFMOffset != NULL) {
    *pp_EFMOffset = createFragmentReply_.metadataInfo.GetEFMOffset();
  }

  if (pp_numFragments != NULL) {
    *pp_numFragments = createFragmentReply_.metadataInfo.GetNumFragments();
  }

  if (pp_firstFFMOffset != NULL) {
    *pp_firstFFMOffset = createFragmentReply_.metadataInfo.GetFFMOffset();
  }

  if (pp_totalFFMSize != NULL) {
    *pp_totalFFMSize = createFragmentReply_.metadataInfo.GetTotalFFMSize();
  }

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReply::SetVarAreaSize
///
/// \brief  Sets the size of the variable area in the create fragment reply
///
/// \param     pv_varAreaSize       New variable area size
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReply::SetVarAreaSize ( size_t pv_varAreaSize ) {

  createFragmentReply_.msgHeader.Validate();

  createFragmentReply_.varAreaSize = pv_varAreaSize;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CreateFragmentReply::SetVarMetadataControl
///
/// \brief  Sets the metdata control information in the message
///
///  Sets the metadata control inforamtion in a create file reply.  Any existing
///  values are overwritten.  All values must be specified.
///
/// \param     pv_EFMOffset        offset from beginning of msg to the EFM
/// \param     pv_numFragments     number of fragments/ffms in this message
/// \param     pv_firstFFMOffset   offset from beginning of msg to the first FFM
/// \param     pv_totalFFMSize     Size of all FFMs.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CreateFragmentReply
    ::SetVarMetadataControl (varoffset_t    pv_EFMOffset, 
                             int            pv_numFragments, 
                             varoffset_t    pv_firstFFMOffset, 
                             size_t         pv_totalFFMSize  ) {

  createFragmentReply_.msgHeader.Validate();

  createFragmentReply_.metadataInfo.Init (pv_EFMOffset,
                                      pv_numFragments,
                                      pv_firstFFMOffset,
                                      pv_totalFFMSize);
}


/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_CloseFileReq class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CloseFileReq::Init Default Constructor
///
/// \brief  Initializes a create file request fixed message buffer to defaults
///
///  Sets up a close file request message buffer fixed length area to
///  defaults. There is no
///  variable area for a CloseFile Request, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CloseFileReq::Init() {

  closeFileReq_.msgHeader.Init (MT_CloseFile,MPT_LibToDaemon,-1, -1);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CloseFileReq::Init Parameterized Constructor
///
/// \brief  Initializes a close file request fixed message buffer to defaults
///
///  Sets up a close file request message buffer fixed length area to specified
///  values.  There is no variable area for
///  a CloseFile Request, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CloseFileReq::Init(STFS_OpenIdentifier *pp_OpenId,
                               int   pv_requesterNodeId,
                               int   pv_requesterPID) {

  closeFileReq_.msgHeader.Init (MT_CloseFile,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  closeFileReq_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  if (!pp_OpenId) {
    ASSERT (false);
  }

  memcpy(&closeFileReq_.openIdentifier_, pp_OpenId, sizeof(STFS_OpenIdentifier));

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CloseFileReq::Release Destructor
///
/// \brief  Destructor for Close File Request fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CloseFileReq::Release() {

  closeFileReq_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CloseFileReq::GetOpenIdentifier
///
/// \brief  Gets the OpenID
///
/// \retval  STFS_OpenIdentifier *
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier *
STFSMsgBuf_CloseFileReq:: GetOpenIdentifier ( void ) {

  closeFileReq_.msgHeader.Validate();

  return &closeFileReq_.openIdentifier_;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_CloseFileReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CloseFileReply::Init Default Constructor
///
/// \brief  Initializes a close file request fixed message buffer to defaults
///
///  Sets up a close file request message buffer fixed length area to
///  defaults.  There is no
///  variable area for a CloseFile Reply, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CloseFileReply::Init() {

  closeFileReply_.msgHeader.Init (MT_CloseFileReply,MPT_LibToDaemon,-1, -1);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CloseFileReply::Init Parameterized Constructor
///
/// \brief  Initializes a close file reply fixed message buffer to defaults
///
///  Sets up a close file request message buffer fixed length area to specified
///  values.  There is no variable area for
///  a CloseFile Reply, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CloseFileReply::Init(STFS_OpenIdentifier *pp_OpenId,
                               int   pv_requesterNodeId,
                               int   pv_requesterPID) {

  // LibToDaemon is only choice for MPT until we support creating an STFS FILE,
  // not fragment, on another node.  For now, STFS files can only be created
  // local to the library that's creating them.

  closeFileReply_.msgHeader.Init (MT_CloseFileReply,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  closeFileReply_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  if (!pp_OpenId) {
    ASSERT (false);
  }

  memcpy(&closeFileReply_.openIdentifier_, pp_OpenId, sizeof(STFS_OpenIdentifier));

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CloseFileReply::Release Destructor
///
/// \brief  Destructor for Close File Request fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_CloseFileReply::Release() {

  closeFileReply_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_CloseFileReply::GetOpenIdentifier
///
/// \brief  Gets the OpenID
///
/// \retval  STFS_OpenIdentifier *
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier *
STFSMsgBuf_CloseFileReply:: GetOpenIdentifier ( void ) {

  closeFileReply_.msgHeader.Validate();

  return &closeFileReply_.openIdentifier_;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_OpenersReq class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReq::Init Default Constructor
///
/// \brief  Initializes a openers request fixed message buffer to defaults
///
///  Sets up an openers request message buffer fixed length area to
///  defaults.  There is no
///  variable area for an Openers Request, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenersReq::Init() {

  openersReq_.msgHeader.Init (MT_Openers,MPT_LibToDaemon,-1, -1);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReq::Init Parameterized Constructor
///
/// \brief  Initializes an Openers request fixed message buffer to defaults
///
///  Sets up an Openers request message buffer fixed length area to specified
///  values.  There is no variable area for
///  an Openers Request, so that area is left defaulted to empty.
///
/// \param     pv_Nid             Node ID
/// \param     pp_Path            Path and Filename
/// \param     pp_OpenersSet      Pointer to OpenersSet struct
/// \param     pp_requesterNodeId Requester's Node ID
/// \param     pv_requesterPID    Requester's Process ID
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenersReq::Init(stfs_nodeid_t        pv_Nid,
                            char                *pp_Path,
                            STFS_OpenersSet     *pp_OpenersSet,
                            int                  pv_requesterNodeId,
                            int                  pv_requesterPID) {

  // LibToDaemon is only choice for MPT until we support creating an STFS FILE,
  // not fragment, on another node.  For now, STFS files can only be created
  // local to the library that's creating them.

  openersReq_.msgHeader.Init (MT_Openers,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  openersReq_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  if (!pp_Path) {
    ASSERT (false);
  }

  openersReq_.Nid = pv_Nid;
  strcpy(openersReq_.Path, pp_Path);
  memcpy(&openersReq_.OpenersSet, pp_OpenersSet, sizeof(STFS_OpenersSet));
  

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReq::Release Destructor
///
/// \brief  Destructor for Close File Request fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenersReq::Release() {

  openersReq_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReq::GetNid
///
/// \brief  Gets the Node ID
///
/// \retval stfs_nodeid_t
///
///////////////////////////////////////////////////////////////////////////////
stfs_nodeid_t
STFSMsgBuf_OpenersReq::GetNid( void ) {

  openersReq_.msgHeader.Validate();

  return openersReq_.Nid;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReq::GetPath
///
/// \brief  Gets the File Path
///
/// \retval char *
///
///////////////////////////////////////////////////////////////////////////////
char *
STFSMsgBuf_OpenersReq::GetPath( void ) {

  openersReq_.msgHeader.Validate();

  return openersReq_.Path;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReq::GetOpenersSet
///
/// \brief  Gets the Set of Openers
///
/// \retval STFS_OpenersSet
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenersSet 
STFSMsgBuf_OpenersReq::GetOpenersSet( void ) {

  openersReq_.msgHeader.Validate();

  return openersReq_.OpenersSet;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_OpenersReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReply::Init Default Constructor
///
/// \brief  Initializes an Openers Reply fixed message buffer to defaults
///
///  Sets up an Openers Reply message buffer fixed length area to
///  defaults.  There is no
///  variable area for an Openers Reply, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenersReply::Init() {

  openersReply_.msgHeader.Init (MT_OpenersReply,MPT_LibToDaemon,-1, -1);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReply::Init Parameterized Constructor
///
/// \brief  Initializes an Openers Reply fixed message buffer to defaults
///
///  Sets up an Openers Reply message buffer fixed length area to specified
///  values.  There is no variable area for
///  an Openers Reply, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenersReply::Init(stfs_nodeid_t        pv_Nid,
                              char                *pp_Path,
                              STFS_OpenersSet     *pp_OpenersSet,
                              int                  pv_requesterNodeId,
                              int                  pv_requesterPID) {

  openersReply_.msgHeader.Init (MT_Openers,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  openersReply_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  if (!pp_Path) {
    ASSERT (false);
  }

  openersReply_.Nid = pv_Nid;
  strcpy(openersReply_.Path, pp_Path);
  memcpy(&openersReply_.OpenersSet, pp_OpenersSet, sizeof(STFS_OpenersSet));
  

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReply::Release Destructor
///
/// \brief  Destructor for Openers Reply fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenersReply::Release() {

  openersReply_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReply::GetNid
///
/// \brief  Gets the Node ID
///
/// \retval stfs_nodeid_t
///
///////////////////////////////////////////////////////////////////////////////
stfs_nodeid_t
STFSMsgBuf_OpenersReply::GetNid( void ) {

  openersReply_.msgHeader.Validate();

  return openersReply_.Nid;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReply::GetPath
///
/// \brief  Gets the File Path
///
/// \retval char *
///
///////////////////////////////////////////////////////////////////////////////
char *
STFSMsgBuf_OpenersReply::GetPath( void ) {

  openersReply_.msgHeader.Validate();

  return openersReply_.Path;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenersReply::GetOpenersSet
///
/// \brief  Gets the Set of Openers
///
/// \retval STFS_OpenersSet
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenersSet  
STFSMsgBuf_OpenersReply::GetOpenersSet( void ) {

  openersReply_.msgHeader.Validate();

  return openersReply_.OpenersSet;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_StatReq class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReq::Init Default Constructor
///
/// \brief  Initializes a Stat request fixed message buffer to defaults
///
///  Sets up a Stat request message buffer fixed length area to
///  defaults.  There is no
///  variable area for a Stat Request, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_StatReq::Init() {

  statReq_.msgHeader.Init (MT_Stat,MPT_LibToDaemon,-1, -1);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReq::Init Parameterized Constructor
///
/// \brief  Initializes a Stat request fixed message buffer to defaults
///
///  Sets up a Stat request message buffer fixed length area to specified
///  values.  There is no variable area for
///  a Stat Request, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_StatReq::Init(stfs_nodeid_t        pv_Nid,
                         char                *pp_Path,
                         STFS_StatSet        *pp_StatSet,
                         stfs_statmask_t      pv_Mask,
                         int                  pv_requesterNodeId,
                         int                  pv_requesterPID) {

  statReq_.msgHeader.Init (MT_Stat,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  statReq_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  if (!pp_Path) {
    ASSERT (false);
  }

  statReq_.Nid = pv_Nid;
  strcpy(statReq_.Path, pp_Path);
  memcpy(&statReq_.StatSet, pp_StatSet, sizeof(STFS_StatSet));
  statReq_.StatMask = pv_Mask;

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReq::Release Destructor
///
/// \brief  Destructor for Stat Request fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_StatReq::Release() {

  statReq_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReq::GetNid
///
/// \brief  Gets the Node ID
///
/// \retval stfs_nodeid_t
///
///////////////////////////////////////////////////////////////////////////////
stfs_nodeid_t
STFSMsgBuf_StatReq::GetNid( void ) {

  statReq_.msgHeader.Validate();

  return statReq_.Nid;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReq::GetPath
///
/// \brief  Gets the File Path
///
/// \retval char *
///
///////////////////////////////////////////////////////////////////////////////
char *
STFSMsgBuf_StatReq::GetPath( void ) {

  statReq_.msgHeader.Validate();

  return statReq_.Path;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReq::GetStatSet
///
/// \brief  Gets the Set of Stat
///
/// \retval STFS_StatSet
///
///////////////////////////////////////////////////////////////////////////////
STFS_StatSet 
STFSMsgBuf_StatReq::GetStatSet( void ) {

  statReq_.msgHeader.Validate();

  return statReq_.StatSet;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReq::GetMask
///
/// \brief  Gets the Mask of Stat
///
/// \retval stfs_statmask_t
///
///////////////////////////////////////////////////////////////////////////////
stfs_statmask_t 
STFSMsgBuf_StatReq::GetMask ( void ) {

  statReq_.msgHeader.Validate();

  return statReq_.StatMask;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_StatReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReply::Init Default Constructor
///
/// \brief  Initializes a Stat Reply fixed message buffer to defaults
///
///  Sets up a Stat reply message buffer fixed length area to
///  defaults.  There is no
///  variable area for a Stat Reply, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_StatReply::Init() {

  statReply_.msgHeader.Init (MT_StatReply,MPT_LibToDaemon,-1, -1);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReply::Init Parameterized Constructor
///
/// \brief  Initializes a Stat Reply message buffer to defaults
///
///  Sets up a Stat Reply message buffer fixed length area to specified
///  values. There is no variable area for
///  a Stat Reply, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_StatReply::Init(stfs_nodeid_t        pv_Nid,
                           char                *pp_Path,
                           struct STFS_StatSet *pp_StatSet,
                           stfs_statmask_t      pv_Mask,
                           int                  pv_requesterNodeId,
                           int                  pv_requesterPID) {

  statReply_.msgHeader.Init (MT_Stat,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  statReply_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  if (!pp_Path) {
    ASSERT (false);
  }

  statReply_.Nid = pv_Nid;
  strcpy(statReply_.Path, pp_Path);
  memcpy(&statReply_.StatSet, pp_StatSet, sizeof(STFS_StatSet));
  statReply_.StatMask = pv_Mask;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReply::Release Destructor
///
/// \brief  Destructor for Stat Reply fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_StatReply::Release() {

  statReply_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReply::GetNid
///
/// \brief  Gets the Node ID
///
/// \retval stfs_nodeid_t
///
///////////////////////////////////////////////////////////////////////////////
stfs_nodeid_t
STFSMsgBuf_StatReply::GetNid( void ) {

  statReply_.msgHeader.Validate();

  return statReply_.Nid;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReply::GetPath
///
/// \brief  Gets the File Path
///
/// \retval char *
///
///////////////////////////////////////////////////////////////////////////////
char *
STFSMsgBuf_StatReply::GetPath( void ) {

  statReply_.msgHeader.Validate();

  return statReply_.Path;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReply::GetStatSet
///
/// \brief  Gets the Set of Stat
///
/// \retval STFS_StatSet
///
///////////////////////////////////////////////////////////////////////////////
STFS_StatSet  
STFSMsgBuf_StatReply::GetStatSet( void ) {

  statReply_.msgHeader.Validate();

  return statReply_.StatSet;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_StatReply::GetMask
///
/// \brief  Gets the Mask of Stat
///
/// \retval stfs_statmask_t
///
///////////////////////////////////////////////////////////////////////////////
stfs_statmask_t 
STFSMsgBuf_StatReply::GetMask ( void ) {

  statReply_.msgHeader.Validate();

  return statReply_.StatMask;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_FOpenersReq class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReq::Init Default Constructor
///
/// \brief  Initializes an FOpeners request fixed message buffer to defaults
///
///  Sets up an FOpeners request message buffer fixed length area to
///  defaults.  There is no
///  variable area for an FOpeners Request, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_FOpenersReq::Init() {

  fopenersReq_.msgHeader.Init (MT_FOpeners,MPT_LibToDaemon,-1, -1);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReq::Init Parameterized Constructor
///
/// \brief  Initializes an  FOpeners request fixed message buffer to defaults
///
///  Sets up an FOpeners request message buffer fixed length area to specified
///  values (no template name, mkstemp request).  There is no variable area for
///  a FOpeners Request, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_FOpenersReq::Init(stfs_fhndl_t        pv_Fhandle,
                             STFS_OpenersSet    *pp_FOpenersSet,
                             int                 pv_requesterNodeId,
                             int                 pv_requesterPID) {


  fopenersReq_.msgHeader.Init (MT_FOpeners,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  fopenersReq_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  fopenersReq_.fhandle = pv_Fhandle;
  memcpy(&fopenersReq_.fOpenersSet, pp_FOpenersSet, sizeof(STFS_OpenersSet));
  

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReq::Release Destructor
///
/// \brief  Destructor for FOpeners Request fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_FOpenersReq::Release() {

  fopenersReq_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReq::GetFhandle
///
/// \brief  Gets the File Handle
///
/// \retval stfs_fhndl_t
///
///////////////////////////////////////////////////////////////////////////////
stfs_fhndl_t
STFSMsgBuf_FOpenersReq::GetFhandle( void ) {

  fopenersReq_.msgHeader.Validate();

  return fopenersReq_.fhandle;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReq::GetPath
///
/// \brief  Gets the File Path
///
/// \retval char *
///
///////////////////////////////////////////////////////////////////////////////
char *
STFSMsgBuf_FOpenersReq::GetPath( void ) {

  fopenersReq_.msgHeader.Validate();

  return fopenersReq_.path;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReq::GetFOpenersSet
///
/// \brief  Gets the Set of FOpeners
///
/// \retval STFS_FOpenersSet
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenersSet  
STFSMsgBuf_FOpenersReq::GetFOpenersSet( void ) {

  fopenersReq_.msgHeader.Validate();

  return fopenersReq_.fOpenersSet;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_FOpenersReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReply::Init Default Constructor
///
/// \brief  Initializes a FOpeners reply fixed message buffer to defaults
///
///  Sets up a FOpeners reply message buffer fixed length area to
///  defaults (no template name, mkstemp request).  There is no
///  variable area for a FOpeners reply, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_FOpenersReply::Init() {

  fopenersReply_.msgHeader.Init (MT_FOpenersReply,MPT_LibToDaemon,-1, -1);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReply::Init Parameterized Constructor
///
/// \brief  Initializes a FOpeners reply fixed message buffer to defaults
///
///  Sets up a FOpeners reply message buffer fixed length area to specified
///  values (no template name, mkstemp request).  There is no variable area for
///  a FOpeners reply, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_FOpenersReply::Init(stfs_fhndl_t        pv_Fhandle,
                              STFS_OpenersSet     *pp_FOpenersSet,
                              int                  pv_requesterNodeId,
                              int                  pv_requesterPID) {


  fopenersReply_.msgHeader.Init (MT_FOpeners,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  fopenersReply_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  fopenersReply_.fhandle = pv_Fhandle;
  memcpy(&fopenersReply_.fOpenersSet, pp_FOpenersSet, sizeof(STFS_OpenersSet));
  

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReply::Release Destructor
///
/// \brief  Destructor for FOpeners Reply fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_FOpenersReply::Release() {

  fopenersReply_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReply::GetFhandle
///
/// \brief  Gets the File Handle
///
/// \retval stfs_fhndl_t
///
///////////////////////////////////////////////////////////////////////////////
stfs_fhndl_t
STFSMsgBuf_FOpenersReply::GetFhandle( void ) {

  fopenersReply_.msgHeader.Validate();

  return fopenersReply_.fhandle;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReply::GetPath
///
/// \brief  Gets the File Path
///
/// \retval char *
///
///////////////////////////////////////////////////////////////////////////////
char *
STFSMsgBuf_FOpenersReply::GetPath( void ) {

  fopenersReply_.msgHeader.Validate();

  return fopenersReply_.path;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_FOpenersReply::GetFOpenersSet
///
/// \brief  Gets the Set of FOpeners
///
/// \retval STFS_FOpenersSet
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenersSet  
STFSMsgBuf_FOpenersReply::GetFOpenersSet( void ) {

  fopenersReply_.msgHeader.Validate();

  return fopenersReply_.fOpenersSet;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_OpenFileReq class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReq::Init Default Constructor
///
/// \brief  Initializes a open file request fixed message buffer to defaults
///
///  Sets up a open file request message buffer fixed length area to
///  defaults (no file name).  There is no variable area for an
///  OpenFile Request, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReq::Init() {

  openFileReq_.msgHeader.Init (MT_OpenFile,MPT_LibToDaemon,-1, -1);

  openFileReq_.fileName_[0]= '\0';
  openFileReq_.openFlag_ = 0;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReq::Init Parameterized Constructor
///
/// \brief  Initializes a open file request fixed message buffer to defaults
///
///  Sets up a open file request message buffer fixed length area to specified
///  values (no File name).  There is no variable area for
///  a OpenFile Request, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReq::Init(char *pp_FileName,
                             bool  pv_OpenFlag,
                             int   pv_requesterNodeId,
                             int   pv_requesterPID) {

  // LibToDaemon is only choice for MPT until we support creating an STFS FILE,
  // not fragment, on another node.  For now, STFS files can only be opend
  // local to the library that's creating them.

  openFileReq_.msgHeader.Init (MT_OpenFile,
                               MPT_LibToDaemon, 
                               pv_requesterNodeId,
                               pv_requesterPID
                               );
  openFileReq_.msgHeader.SetVariableInfo (false, // No variable section on open request
                                          0);

  // We'll let STFSd decide whether the name is really too long. Here we just
  // decide whether or not it fits in the message.

  if (strlen (pp_FileName) > STFS_NAME_MAX-1 ){
    // string too long in constructor!
    ASSERT (false);
  }

  strncpy (openFileReq_.fileName_, pp_FileName, STFS_NAME_MAX-1);
  openFileReq_.fileName_[STFS_NAME_MAX-1] = '\0';

  openFileReq_.openFlag_ = pv_OpenFlag;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReq::Release Destructor
///
/// \brief  Destructor for Open File Request fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReq::Release() {

  openFileReq_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReq::GetFileName
///
/// \brief  Gets the file name for the open file request
///
///   This method retrieves the file name from a open file request
///   and copies it into the user-supplied buffer.
/// 
/// \param     pp_FileName     The file name template
/// \param     pv_FileNameMaxSize Max number of bytes for the template
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReq::GetFileName ( char * pp_FileName,
                                      size_t pv_FileNameMaxSize ) 
{

  openFileReq_.msgHeader.Validate();

  if (strlen (openFileReq_.fileName_) > pv_FileNameMaxSize-1 ){
    strncpy (pp_FileName, openFileReq_.fileName_, pv_FileNameMaxSize-1);
    pp_FileName[pv_FileNameMaxSize-1] = '\0';
  }
  else {
    strcpy (pp_FileName, openFileReq_.fileName_);
  }
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReq::GetOpenFlag
///
/// \brief  Gets the open flag 
///
///   This method retrieves the open flag supplied in the request
/// 
/// \param     void 
///
/// \retval    Open Flag (int)
///
///////////////////////////////////////////////////////////////////////////////
int
STFSMsgBuf_OpenFileReq:: GetOpenFlag (void) {

  openFileReq_.msgHeader.Validate();

  return (openFileReq_.openFlag_);
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_OpenFileReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::Init Default Constructor
///
/// \brief  Initializes a open file reply fixed message buffer to defaults
///
///  Sets up a open file reply message buffer fixed length area to defaults.
///  A normal file open reply includes a variable area that has the EFM and
///  FFMs in it.  This area is not initialized as part of the constructor.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReply::Init() {

  openFileReply_.msgHeader.Init (MT_OpenFileReply,MPT_LibToDaemon,-1,-1);
  openFileReply_.msgHeader.SetVariableInfo(false, // No variable section yet
                                             0);    // current var section offset

  openFileReply_.fileName_[0]= '\0';
  memset ( &openFileReply_.openIdentifier, -1, sizeof (STFS_OpenIdentifier) );
  openFileReply_.varAreaSize = 0;

  // use default constructor for the metadata info
  openFileReply_.metadataInfo.Init();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::Init Parameterized Constructor
///
/// \brief  Initializes a open file reply fixed message buffer to values
/// specified as parameters, plus defaults
///
///  Sets up a open file reply message buffer fixed length area to defaults.
///  A normal file open reply includes a variable area that has the EFM and
///  FFMs in it.  This area is not initialized as part of the constructor.
///
/// \param    pp_template     the file name template for the reply
/// \param    pp_openId       pointer to an openID for the message
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReply ::Init(char *pp_FileName,
                                STFS_OpenIdentifier *pp_openID) 
{
  openFileReply_.msgHeader.Init (MT_OpenFileReply, MPT_LibToDaemon,-1,-1);
  openFileReply_.msgHeader.SetVariableInfo (true, 
                                            STFS::GetSizeOfFixedMessageBuffer(MT_OpenFileReply));


  if ( strlen (pp_FileName) > STFS_NAME_MAX ) {
    // Hey wait, that name came from STFS!  What'd we do wrong?
    ASSERT (false);
    STFS_SigIll();
  }

  strncpy (openFileReply_.fileName_, pp_FileName, STFS_NAME_MAX);

  ASSERT (pp_openID != NULL);
  if (pp_openID != NULL) {
    //openFileReply_.openIdentifier =  *pp_openID;
    memcpy(&openFileReply_.openIdentifier, pp_openID, sizeof (STFS_OpenIdentifier) );
  }
  else {
    // shouldn't reach here, but just in case...
    memset ( &openFileReply_.openIdentifier, -1, sizeof (STFS_OpenIdentifier) );
  }

  openFileReply_.varAreaSize = 0;

  // still use default constructor for the metadata info
  openFileReply_.metadataInfo.Init();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::Release Destructor
///
/// \brief  Destructor for Open File Reply
///
///  This is the destructor for the Open File Reply fixed buffer.  It doesn't
///  do anything.  Why is that noteworthy?  Well, in particular, it doesn't do
///  anything to clean up the variable area either.  They're not related.  But
///  after this destructor is called, the area is good as gone, because we've
///  cleaned up the metadata info.  Caveat Emptor.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReply::Release() {

  openFileReply_.msgHeader.Release();
  // nothing to free, we're done now...

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::GetFileName
///
/// \brief  Gets the file name for the open file reply
///
///   This method retrieves the file name from a open file reply
///   and copies it into the user-supplied buffer.
/// 
/// \param     pp_tgtTemplate     The file name template
/// \param     pv_templateMaxSize Max number of bytes for the template
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReply::GetFileName ( char * pp_FileName,
                                        size_t pv_FileNameMaxSize ) 
{

  openFileReply_.msgHeader.Validate();

  if (strlen (openFileReply_.fileName_) > pv_FileNameMaxSize-1 ){
    strncpy (pp_FileName, openFileReply_.fileName_, pv_FileNameMaxSize-1);
    pp_FileName[pv_FileNameMaxSize-1] = '\0';
  }
  else {
    strcpy (pp_FileName, openFileReply_.fileName_);
  }
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::GetOpenIdentifier
///
/// \brief  Gets the open identifier from the open file reply
///
/// \param     void
///
/// \retval    Returns the open identifier from the message. All bytes set to -1
///            if uninitialized.
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier
STFSMsgBuf_OpenFileReply::GetOpenIdentifier ( void ) {

  openFileReply_.msgHeader.Validate();

  return openFileReply_.openIdentifier;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::GetVarAreaSize
///
/// \brief  Gets the size of the variable area from the open file reply
///
/// \param     void
///
/// \retval    Returns the variable area size from the message.
///
///////////////////////////////////////////////////////////////////////////////
size_t
STFSMsgBuf_OpenFileReply::GetVarAreaSize ( void ) {

  openFileReply_.msgHeader.Validate();

  return (openFileReply_.varAreaSize);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::GetVarMetadataControl
///
/// \brief  Extracts the metdata control information from the message
///
/// \param     pp_EFMOffset        offset from beginning of msg to the EFM
/// \param     pp_numFragments     number of fragments/ffms in this message
/// \param     pp_firstFFMOffset   offset from beginning of msg to the first FFM
/// \param     pp_totalFFMSize     Size of all FFMs.
///
/// \retval    Returns the variable area size from the message.
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReply
    ::GetVarMetadataControl (varoffset_t *pp_EFMOffset, 
                             int *pp_numFragments, 
                             varoffset_t *pp_firstFFMOffset, 
                             size_t *pp_totalFFMSize  ) {

  openFileReply_.msgHeader.Validate();

  if (pp_EFMOffset != NULL) {
    *pp_EFMOffset = openFileReply_.metadataInfo.GetEFMOffset();
  }

  if (pp_numFragments != NULL) {
    *pp_numFragments = openFileReply_.metadataInfo.GetNumFragments();
  }

  if (pp_firstFFMOffset != NULL) {
    *pp_firstFFMOffset = openFileReply_.metadataInfo.GetFFMOffset();
  }

  if (pp_totalFFMSize != NULL) {
    *pp_totalFFMSize = openFileReply_.metadataInfo.GetTotalFFMSize();
  }

}


//////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::SetVarAreaSize
///
/// \brief  Sets the size of the variable area in the open file reply
///
/// \param     pv_varAreaSize       New variable area size
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReply::SetVarAreaSize ( size_t pv_varAreaSize ) {

  openFileReply_.msgHeader.Validate();

  openFileReply_.varAreaSize = pv_varAreaSize;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_OpenFileReply::SetVarMetadataControl
///
/// \brief  Sets the metdata control information in the message
///
///  Sets the metadata control inforamtion in a open file reply.  Any existing
///  values are overwritten.  All values must be specified.
///
/// \param     pv_EFMOffset        offset from beginning of msg to the EFM
/// \param     pv_numFragments     number of fragments/ffms in this message
/// \param     pv_firstFFMOffset   offset from beginning of msg to the first FFM
/// \param     pv_totalFFMSize     Size of all FFMs.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_OpenFileReply
    ::SetVarMetadataControl (varoffset_t    pv_EFMOffset, 
                             int            pv_numFragments, 
                             varoffset_t    pv_firstFFMOffset, 
                             size_t         pv_totalFFMSize  ) {

  openFileReply_.msgHeader.Validate();

  openFileReply_.metadataInfo.Init (pv_EFMOffset,
                                    pv_numFragments,
                                    pv_firstFFMOffset,
                                    pv_totalFFMSize);
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_UnlinkFileReq class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_UnlinkFileReq::Init Default Constructor
///
/// \brief  Initializes an UnlinkFileReq request fixed message buffer to defaults
///
///  Sets up a UnlinkFileReq request message buffer fixed length area to
///  defaults (no template name, mkstemp request).  There is no
///  variable area for a UnlinkFile Req, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_UnlinkFileReq::Init() {

  unlinkFileReq_.msgHeader.Init (MT_UnlinkFile,MPT_LibToDaemon,-1, -1);
  unlinkFileReq_.fileName_[0] = '\0';
  
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_UnlinkFileReq::Init Parameterized Constructor
///
/// \brief  Initializes an UnlinkFileReq fixed message buffer to defaults
///
///  Sets up a UnlinkFileReq message buffer fixed length area to specified
///  values (no template name, mkstemp request).  There is no variable area for
///  a UnlinkFileReq, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_UnlinkFileReq::Init(const char *pp_fileName,
                               int         pv_requesterNodeId,
                               int         pv_requesterPID) {

  // LibToDaemon is only choice for MPT until we support creating an STFS FILE,
  // not fragment, on another node.  For now, STFS files can only be created
  // local to the library that's creating them.

  unlinkFileReq_.msgHeader.Init (MT_UnlinkFile,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  unlinkFileReq_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  if (strlen (pp_fileName) > STFS_NAME_MAX-1 ){
    // string too long in constructor!
    ASSERT (false);
  }

  strncpy (unlinkFileReq_.fileName_, pp_fileName, STFS_NAME_MAX-1);
  unlinkFileReq_.fileName_[STFS_NAME_MAX-1] = '\0';


}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_UnlinkFileReq::Release Destructor
///
/// \brief  Destructor for UnlinkFileReq fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_UnlinkFileReq::Release() {

  unlinkFileReq_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_UnlinkFileReq::GetFileName
///
/// \brief  Gets the file name
///
/// \retval char *
///
///////////////////////////////////////////////////////////////////////////////
char *
STFSMsgBuf_UnlinkFileReq:: GetFileName ( void ) {

  unlinkFileReq_.msgHeader.Validate();

  return unlinkFileReq_.fileName_;
}

/////////////////////////////////////////////////////////////////////////////
/// STFSMsgBuf_UnlinkFileReply class methods
/////////////////////////////////////////////////////////////////////////////

//***********************
// Public methods
//***********************


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_UnlinkFileReply::Init Default Constructor
///
/// \brief  Initializes an UnlinkFileReply fixed message buffer to defaults
///
///  Sets up an UnlinkFileReply message buffer fixed length area to
///  defaults (no template name, mkstemp request).  There is no
///  variable area for a UnlinkFileReply, so that area is ignored.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_UnlinkFileReply::Init() {

  unlinkFileReply_.msgHeader.Init (MT_UnlinkFileReply,MPT_LibToDaemon,-1, -1);

  unlinkFileReply_.fileName_[0] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_UnlinkFileReply::Init Parameterized Constructor
///
/// \brief  Initializes a UnlinkFileReply fixed message buffer to defaults
///
///  Sets up an UnlinkFileReply message buffer fixed length area to specified
///  values (no template name, mkstemp request).  There is no variable area for
///  a UnlinkFileReply, so that area is left defaulted to empty.
///
/// \param     
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_UnlinkFileReply::Init(const char *pp_fileName,
                                 int         pv_requesterNodeId,
                                 int         pv_requesterPID) {

  // LibToDaemon is only choice for MPT until we support creating an STFS FILE,
  // not fragment, on another node.  For now, STFS files can only be created
  // local to the library that's creating them.

  unlinkFileReply_.msgHeader.Init (MT_UnlinkFileReply,MPT_LibToDaemon, 
                                 pv_requesterNodeId,pv_requesterPID
                                 );
                                 
  unlinkFileReply_.msgHeader.SetVariableInfo (false, // No variable section on create request
                                           0);

  if (strlen (pp_fileName) > STFS_NAME_MAX-1 ){
    // string too long in constructor!
    ASSERT (false);
  }

  strncpy (unlinkFileReply_.fileName_, pp_fileName, STFS_NAME_MAX-1);
  unlinkFileReply_.fileName_[STFS_NAME_MAX-1] = '\0';
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_UnlinkFileReply::Release Destructor
///
/// \brief  Destructor for UnlinkFileReply fixed message buf
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
void
STFSMsgBuf_UnlinkFileReply::Release() {

  unlinkFileReply_.msgHeader.Release();

  // nothing to free, we're done now...

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMsgBuf_UnlinkFileReply::FileName
///
/// \brief  Gets the file name
///
/// \retval char * 
///
///////////////////////////////////////////////////////////////////////////////
char *
STFSMsgBuf_UnlinkFileReply:: GetFileName ( void ) {

  unlinkFileReply_.msgHeader.Validate();

  return unlinkFileReply_.fileName_;
}

    
namespace STFS {

  ///////////////////////////////////////////////////////////////////////////////
  ///
  //          GetSizeOfFixedMessageBuffer
  ///
  /// \brief  Returns the size of the Fixed Message Buffer for the given 
  ///         message type
  ///
  /// \param  pv_MessageType - the message type of the fixed message buffer
  ///
  /// \retval size_t (returns 0 if the input message type is outside 
  ///                 the range of known message types)
  ///
  ///////////////////////////////////////////////////////////////////////////////
  size_t 
  GetSizeOfFixedMessageBuffer(Enum_MessageType pv_MessageType)
  {
    // Update this array when a class associated with 
    // a new message type is added
    static size_t FixedMessageSizeTable[] = {
        0,                                     // MT_Invalid
        sizeof (STFSMsgBuf_CreateFileReq),     // MT_CreateFile
        sizeof (STFSMsgBuf_CreateFileReply),   // MT_CreateFileReply
        sizeof (STFSMsgBuf_CreateFragmentReq), // MT_CreateFragment
        sizeof (STFSMsgBuf_CreateFragmentReply), // MT_CreateFragmentReply
        sizeof (STFSMsgBuf_OpenFileReq),         // MT_OpenFile
        sizeof (STFSMsgBuf_OpenFileReply),       // MT_OpenFileReply
        sizeof (STFSMsgBuf_CloseFileReq),        // MT_CloseFile
        sizeof (STFSMsgBuf_CloseFileReply),      // MT_CloseFileReply
        sizeof (STFSMsgBuf_UnlinkFileReq),       // MT_UnlinkFile
        sizeof (STFSMsgBuf_UnlinkFileReply),     // MT_UnlinkFileReply
        sizeof (STFSMsgBuf_ErrorReply),          // MT_ErrorReply,
        0,                                       // MT_GetFileMetadata,
        0,                                 // MT_GetFileMetadataReply,
        sizeof (STFSMsgBuf_OpenersReq),    // MT_Openers
        sizeof (STFSMsgBuf_OpenersReply),  // MT_OpenersReply
        sizeof (STFSMsgBuf_FOpenersReq),   // MT_FOpeners
        sizeof (STFSMsgBuf_FOpenersReply), // MT_FOpenersReply
        sizeof (STFSMsgBuf_StatReq),       // MT_Stat
        sizeof (STFSMsgBuf_StatReply),     // MT_StatReply
        0,                                 // MT_GetEFMReply
        0,                                 // MT_GetEFM

        /// new message types go immediately before this line
        0,                      // MT_Unknown
    };

    if ((pv_MessageType <= MT_Invalid) ||
        (pv_MessageType >= MT_Unknown)) {
      return 0;
    }
      
    return FixedMessageSizeTable[pv_MessageType];
  }
}
