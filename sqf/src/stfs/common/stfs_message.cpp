///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_message.cpp
/// \brief   Implementation of STFS_Message and its derived classes
///   
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
#include "stfs_msgbufv.h"
#include "stfs_util.h"
#include "stfs_sigill.h"

using namespace STFS;

static
bool
CommonPack(Enum_MessageType           pv_MessageType,
           STFS_ExternalFileMetadata *pp_EFM,
           FixedMsg_u                *pp_FixedMsgBuffer,
           size_t                     pv_FixedMsgBufferLen,
           size_t                    *pp_variableBufConsumed)
{

  ///////////////////////////////////////
  ///  Validate Parameters
  ///////////////////////////////////////

  if ((!pp_EFM) ||
      (!pp_FixedMsgBuffer)) {
    //  Why nothing to pack???
    ASSERT (false);
    return false;
  }
  
  ///////////////////////////////////////
  ///  Housekeeping
  ///////////////////////////////////////
 
  /// Get the start of the variable buffer
  char *lp_variableBufStart = (char *) pp_FixedMsgBuffer->charArray 
    + STFS::GetSizeOfFixedMessageBuffer(pv_MessageType);
  size_t lv_totalVariableBufSize = pv_FixedMsgBufferLen - STFS::GetSizeOfFixedMessageBuffer (pv_MessageType);

  char *lp_variableBufCurrLoc = lp_variableBufStart;

  size_t lp_variableBufConsumed = 0;

  ///////////////////////////////////////
  ///Pack the EFM
  ///////////////////////////////////////

  size_t lv_eFMPackedLen;
  bool success = pp_EFM->Pack(lp_variableBufCurrLoc, 
                              lv_totalVariableBufSize,
                              &lv_eFMPackedLen);

  if (!success) {
    // out of memory or a programming problem!
    return false;
  }

  varoffset_t lv_eFMOffset = lp_variableBufConsumed;
  lp_variableBufConsumed += lv_eFMPackedLen;
  lp_variableBufCurrLoc = lp_variableBufStart + lp_variableBufConsumed;


  ///////////////////////////////////////
  /// Set the array with FFM offsets
  ///////////////////////////////////////

  int lv_numFragments = pp_EFM->GetNumFragments();

  varoffset_t lv_fragArrayOffset = lp_variableBufConsumed;

  // Next comes an array of the FFM offsets!  Skip over that
  varoffset_t *lv_fragOffsetArray = (varoffset_t *) lp_variableBufCurrLoc;
  lp_variableBufConsumed += (lv_numFragments * sizeof (varoffset_t));
  lp_variableBufCurrLoc = lp_variableBufStart + lp_variableBufConsumed;

  ///////////////////////////////////////
  ///  Loop over FFMs, packing them
  ///////////////////////////////////////

  STFS_FragmentFileMetadata *lv_localFFM = NULL;
  size_t lv_fFMPackedLen;

  for (int lv_fragIndex = 0; lv_fragIndex < lv_numFragments; lv_fragIndex++) {

    lv_fragOffsetArray[lv_fragIndex] = lp_variableBufConsumed;

    lv_localFFM = pp_EFM->GetFragment(lv_fragIndex);

    if (lv_localFFM == NULL) {
      //HUH?  There was supposed to be a fragment here!
      ASSERT (false);
      //I'll be kind and just return False.  Really, this is probably a SigIll()
      return false;
    }

    success = lv_localFFM->Pack (lp_variableBufCurrLoc, 
                                 lv_totalVariableBufSize,
                                 &lv_fFMPackedLen);

    if (success != true) {
      // packing error!
      return false;
    }

    lp_variableBufConsumed += lv_fFMPackedLen;
    lp_variableBufCurrLoc = lp_variableBufStart + lp_variableBufConsumed;

  }

  ///////////////////////////////////////
  ///  Set metadata info in message
  ///////////////////////////////////////

  size_t lv_totalFFMSize = lp_variableBufConsumed - lv_fragArrayOffset;


  pp_FixedMsgBuffer->headerOnly.SetVariableInfo(true, STFS::GetSizeOfFixedMessageBuffer (pv_MessageType));
  
  switch (pv_MessageType) {
  case MT_CreateFileReply:
    pp_FixedMsgBuffer->createFileReply.SetVarMetadataControl (lv_eFMOffset, 
                                                              lv_numFragments,
                                                              lv_fragArrayOffset, 
                                                              lv_totalFFMSize );

    pp_FixedMsgBuffer->createFileReply.SetVarAreaSize(lp_variableBufConsumed);
    break;

  case MT_OpenFileReply:
    pp_FixedMsgBuffer->openFileReply.SetVarMetadataControl (lv_eFMOffset, 
                                                            lv_numFragments,
                                                            lv_fragArrayOffset, 
                                                            lv_totalFFMSize );

    pp_FixedMsgBuffer->openFileReply.SetVarAreaSize(lp_variableBufConsumed);
    break;

  case MT_CreateFragmentReply:
    pp_FixedMsgBuffer->createFragmentReply.SetVarMetadataControl (lv_eFMOffset, 
								  lv_numFragments,
								  lv_fragArrayOffset, 
								  lv_totalFFMSize );

    pp_FixedMsgBuffer->createFragmentReply.SetVarAreaSize(lp_variableBufConsumed);
    break;

  default:
    // Default shouldn't be used.  Did you forget to implement a case?
    ASSERT (false);
    break;
  }

  *pp_variableBufConsumed = lp_variableBufConsumed;

  ///////////////////////////////////////
  /// Finis!
  ///////////////////////////////////////

  return true;

}

static
STFS_ExternalFileMetadata *
CommonUnpack(Enum_MessageType           pv_MessageType,
             FixedMsg_u                *pp_FixedMsgBuffer)
{
  /////////////////////////////////////
  /// Housekeeping
  /////////////////////////////////////

  char *lp_variableBufStart = (char *) pp_FixedMsgBuffer->charArray 
    + STFS::GetSizeOfFixedMessageBuffer (pv_MessageType);

  varoffset_t lp_eFMOffset = 0;
  int lv_numFragments = 0;
  varoffset_t lp_firstFFMOffset = 0;
  size_t lp_totalFFMSize = 0;

  size_t lv_unpackSize = 0;

  // Remember, these offsets are from the start of the variable area, not the
  // start of pp_FixedMsgBuffer
  switch (pv_MessageType) {
  case MT_CreateFileReply:
    pp_FixedMsgBuffer->createFileReply.GetVarMetadataControl(&lp_eFMOffset,
                                                             &lv_numFragments,
                                                             &lp_firstFFMOffset, 
                                                             &lp_totalFFMSize);
    break;
  case MT_OpenFileReply:
    pp_FixedMsgBuffer->openFileReply.GetVarMetadataControl(&lp_eFMOffset,
                                                           &lv_numFragments,
                                                           &lp_firstFFMOffset, 
                                                           &lp_totalFFMSize);
    break;
  case MT_CreateFragmentReply:
    pp_FixedMsgBuffer->createFragmentReply.GetVarMetadataControl(&lp_eFMOffset,
								 &lv_numFragments,
								 &lp_firstFFMOffset, 
								 &lp_totalFFMSize);
    break;
  default:
    // Default case is unused.  Did you miss implementing a case??
    ASSERT (false);
    break;
  }

  ////////////////////////////////////
  ///  Allocate new EFM
  ////////////////////////////////////

  STFS_ExternalFileMetadata *lp_eFM = new STFS_ExternalFileMetadata();

  if (lp_eFM == NULL) {
    // no memory apparently
    return NULL;
  }

  /////////////////////////////////////
  /// Find the EFM in the variable buffer
  /////////////////////////////////////

  char *lp_packedEFMPtr = lp_variableBufStart + lp_eFMOffset;

  /////////////////////////////////////
  /// Unpack the EFM
  /////////////////////////////////////

  lv_unpackSize = lp_firstFFMOffset - lp_eFMOffset;

  bool success =  lp_eFM->Unpack (lp_packedEFMPtr, lv_unpackSize );

  if (success != true ) {
    // unpacking failure
    delete lp_eFM;
    return NULL;
  }

  /////////////////////////////////////
  /// Get the fragment offset array
  /////////////////////////////////////

  varoffset_t *lp_offsetArray = (varoffset_t *)(lp_variableBufStart + lp_firstFFMOffset);

  /////////////////////////////////////
  /// Unpack each fragment
  /////////////////////////////////////

  char *lp_packedFFMBuf = NULL;
  short success_short = -1;

  //  initialize the first fragment's size
  lv_unpackSize = lp_offsetArray[0] - lv_numFragments*sizeof (varoffset_t);

  for (int fragIndex = 0; fragIndex < lv_numFragments; fragIndex++) {

    ///////////////////////////////////////
    ///  Get the address of the packed FFM
    ///////////////////////////////////////

    lp_packedFFMBuf = lp_variableBufStart + lp_offsetArray[fragIndex];
    

    // we need a new FFM for each fragment we unpack, so we allocate them in
    // this method

    STFS_FragmentFileMetadata *lp_fFM = new STFS_FragmentFileMetadata();

    success = lp_fFM->Unpack (lp_packedFFMBuf, lv_unpackSize);

    if (success != true) {
      //unpack failed, time to bail!
      //  Have to delete the current FFM because we haven't linked it to the EFM yet
      delete lp_fFM;
      delete lp_eFM;
      return NULL;
    }


  ////////////////////////////////////////
  /// Link fragment to EFM
  ////////////////////////////////////////

    success_short = lp_eFM->InsertFFM (lp_fFM);
    if (success_short != 0) {
      //insertFFM failed! Time to bail!
      //  Have to delete the current FFM because we haven't linked it to the EFM yet
      delete lp_fFM;
      delete lp_eFM;
      return NULL;
    }

    /////////////////////////////////////
    ///  End of loop
    /////////////////////////////////////
    lv_unpackSize = lp_offsetArray[fragIndex+1] - lp_offsetArray[fragIndex];;
  }

  /////////////////////////////////////
  /// Finis!
  /////////////////////////////////////
  return lp_eFM;

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_Message methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
///
//          STFS_Message Default Constructor
///
/// \brief  Initializes an STFS_Message
///
///  Sets up the initial STFS message structure, setting message type and sources
///  to invalid and the message buffer to NULL.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFS_Message::STFS_Message(void) :
  buf_ (NULL), 
  bufLen_(0), 
  messageType_ (MT_Invalid),
  messagePath_ (MPT_Invalid),
  messageBufInUse_ (false),
  fixedSize_(0),
  variableSize_ (0)
{
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_Message Constructor
///
/// \brief  Initializes an STFS_Message to specified values
///
///  Sets up the initial STFS message structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param       pp_MessageBufPtr          Ptr to the buffer for message
/// \param       pv_MessageType            Type of message
/// \param       pv_MessagePath            Source and Dest. for message
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFS_Message::STFS_Message(Enum_MessageType pv_MessageType,
                           Enum_MessagePathType pv_MessagePath,
                           void *pp_messageBufPtr,
                           size_t pp_bufLen ):
  buf_( (FixedMsg_u *)pp_messageBufPtr),
  bufLen_(pp_bufLen),
  messageType_(pv_MessageType),
  messagePath_ (pv_MessagePath),
  messageBufInUse_ (false),
  fixedSize_(STFS::GetSizeOfFixedMessageBuffer(pv_MessageType)),
  variableSize_ (0)
{
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_Message Destructor
///
/// \brief  Deletes an STFS_Message
///
///  Deallocates an STFS_Message and frees any memory it has locked.
///  Does not check to see if the message has been sent, so might
///  cause problems with Seabed...
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFS_Message::~STFS_Message(void)
{

  if (buf_!= NULL) {

    if (messageBufInUse_) {
      // why are we trying to delete an in-use buffer?
      ASSERT (false);
    }
#if 0
    // This should not be done - the buffer is owned by the caller
    // and it may not have been created with a 'new' (could be on the stack too)
    delete [] buf_;
#endif
  }
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetMessageType
///
/// \brief  Gets the message type in the current message
///
///  
/// \param     void
///
/// \retval    The message type in the current message.  Could be MT_Invalid.
///
///////////////////////////////////////////////////////////////////////////////
Enum_MessageType 
STFS_Message::GetMessageType(void) {

  return messageType_;
}


///////////////////////////////////////////////////////////////////////////////
///
//          GetMessagePath
///
/// \brief  Gets the message path type in the current message
///
///  
/// \param     void
///
/// \retval    The message path type in the current message.  Could be MPT_Invalid.
///
///////////////////////////////////////////////////////////////////////////////
Enum_MessagePathType 
STFS_Message::GetMessagePath(void) {

  return messagePath_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          MessageBufHasActiveIO
///
/// \brief  Check if the message buf is in use for an IO
///
///  
/// \param     void
///
/// \retval    true if there might be an IO outstanding using the buf
///            false otherwise
///
///////////////////////////////////////////////////////////////////////////////

bool STFS_Message::MessageBufHasActiveIO (void) {

  return messageBufInUse_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetMessageBuf
///
/// \brief  Gets a pointer to the message buf
///
///  
/// \param     void
///
/// \retval    MessageBuf pointer.  Could be NULL
///
///////////////////////////////////////////////////////////////////////////////

void * STFS_Message::GetMessageBuf (void) {
  return (void *) buf_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetMessageBufMaxLen
///
/// \brief  Gets the maximum size for the message buf
///
///  This method returns the current allocated size for the message
///  buf.  The actual size of the message that it contains might be
///  smaller; use GetMessageCurrSize to get that value.
///  
/// \param     void
///
/// \retval    Max Currently Allocated Size.  
///
///////////////////////////////////////////////////////////////////////////////

size_t STFS_Message::GetMessageMaxBufSize (void) {
  return bufLen_;
}


///////////////////////////////////////////////////////////////////////////////
///
//          GetMessageCurrSize
///
/// \brief  Gets the current size of the data in the message buf
///
///  This method returns the current size in bytes of the data in the
///  message buffer.  The value is less than or equal to the maximum size.
///  
/// \param     void
///
/// \retval    The currently allocated size.  Might be 0 if buf
///                                           unallocated or empty.
///
///////////////////////////////////////////////////////////////////////////////

size_t STFS_Message::GetMessageCurrSize (void) {
  return (fixedSize_ + variableSize_);
}


///////////////////////////////////////////////////////////////////////////////
///
//          GetMessageFixedSize
///
/// \brief  Gets the current size of the fixed portion of data in buf
///
///  This method returns the current size in bytes of the data in the
///  fixed portion of the message buffer.  The value is less than or
///  equal to the maximum size.  The size of the fixed portion varies
///  based on message type.
///  
/// \param     void
///
/// \retval    The currently allocated size.  Might be 0 if buf
///                                           unallocated or empty.
///
///////////////////////////////////////////////////////////////////////////////

size_t STFS_Message::GetMessageFixedSize (void) {
  return fixedSize_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetMessageVariableSize
///
/// \brief  Gets the current size of the variable portion of data in buf
///
///  This method returns the current size in bytes of the data in the
///  variable portion of the message buffer.  The size is less than or
///  equal to the maximum size minus the fixed size for the message.
///  The size of the variable portion varies based on message type and
///  the contents of the variable section.
///  
/// \param     void
///
/// \retval    The currently allocated size.  Might be 0 if buf
///                                           unallocated or empty.
///
///////////////////////////////////////////////////////////////////////////////

size_t STFS_Message::GetMessageVariableSize (void) {
  return variableSize_;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFS_Message::GetRequesterNodeId
///
/// \brief  Gets the Requester Node Id 
///
/// \retval int
//////////////////////////////////////////////////////////////////////////////
int
STFS_Message::GetRequesterNodeId (void) {

  return buf_->headerOnly.GetRequesterNodeId();
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_Message::GetRequesterPID
///
/// \brief  Gets the Requester PID
///
/// \retval int
//////////////////////////////////////////////////////////////////////////////
int
STFS_Message::GetRequesterPID (void) {

  return buf_->headerOnly.GetRequesterPID();
}

///////////////////////////////////////////////////////////////////////////////
///
//          SetMessageFixedSize
///
/// \brief  Sets the current size of the fixed portion of data in buf
///
///  This method sets the size of the fixed portion of the message
///  buffer.  The size is must be less than or equal to the maximum
///  size currently allocated, or it is not reset.  If the value can't
///  be reset, the message buf must be resized, or the message must be
///  continued.
///
///  The fixed size of a message can't be changed unless if the
///  variable portion's size is non-zero.  Changing the fixed size in
///  this case would result in memory corruption.
/// 
///  No sizes can be reset if the buffer is in use for an I/O.
///  Complete the I/O, clear the flag, and then reset the size.
///  
/// \param     pv_newFixedSize                The new fixed size
///
/// \retval    true:                  The fixed size was reset
/// \retval    false:                 The fixed size is unchanged.
///
///////////////////////////////////////////////////////////////////////////////

bool STFS_Message::SetMessageFixedSize (size_t pv_newFixedSize) {

  if (bufLen_ < pv_newFixedSize) {
    return false;
  }

  if (messageBufInUse_) {
    // why are we trying to reset a buffer that's in use???
    ASSERT (false);
    return false;
  }

  if (variableSize_ > 0 ) {
    // can't shrink or other calcuations will be wrong!  Programming bug
    ASSERT (false);
    return false;
  }

  fixedSize_ = pv_newFixedSize;
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          SetMessageVariableSize
///
/// \brief  Sets the current size of the variable portion of data in buf
///
///  This method sets the size of the variable portion of the message buffer.
///  The size is must be less than or equal to the maximum size currently
///  allocated less the fixed size for the current message type, or it is not
///  reset.  If the value can't be reset, the message buf must be resized, or
///  the message must be continued.
///
///  No sizes can be reset if the buffer is in use for an I/O.
///  Complete the I/O, clear the flag, and then reset the size.
///  
/// \param     pv_newFixedSize                The new fixed size
///
/// \retval    true:                  The fixed size was reset
/// \retval    false:                 The fixed size is unchanged.
///
///////////////////////////////////////////////////////////////////////////////

bool STFS_Message::SetMessageBufVariableSize (size_t pv_newVariableSize) {

  if (STFS_Message::bufLen_ < pv_newVariableSize) {
    return false;
  }

  if (messageBufInUse_) {
    // why are we trying to reset a buffer that's in use???
    ASSERT (false);
    return false;
  }

  if (pv_newVariableSize > bufLen_ - fixedSize_  ) {
    // not enough room in the buffer
    return false;
  }

  variableSize_ = pv_newVariableSize;
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          SetMessageType
///
/// \brief  Sets the current type of the message
///
///  This method resets the type of the current message.  It doesn't
///  change or deallocate either the fixed or variable
///  portion of the message buffer.  These must be reset
///  independently. 
///
///  
/// \param     pv_newMessageType      The new fixed size
///
/// \retval    true:                  The message type was reset
/// \retval    false:                 The message type is unchanged.
///
///////////////////////////////////////////////////////////////////////////////

bool STFS_Message::SetMessageType ( Enum_MessageType pv_newMessageType) {

  if (messageBufInUse_) {
    // why are we trying to reset a message that's in use???
    ASSERT (false);
    return false;
  }
  messageType_ = pv_newMessageType;
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          SetMessagePath
///
/// \brief  Sets the current message path
///
///  This method resets the type of the current message.
///
///  
/// \param     pv_newMessagePath      The new fixed size
///
/// \retval    true:                  The message type was reset
/// \retval    false:                 The message type is unchanged.
///
///////////////////////////////////////////////////////////////////////////////

bool STFS_Message::SetMessagePath ( Enum_MessagePathType pv_newMessagePath) {

  if (messageBufInUse_) {
    // why are we trying to reset a message that's in use???
    ASSERT (false);
    return false;
  }

  messagePath_ = pv_newMessagePath;
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          SetMessageBufIOInProgress
///
/// \brief  Sets the flag indicating whether the message buf has an
///         I/O outstanding.
///
///  This method sets the flag indicating whether the message buffer
///  is in use for an outstanding I/O.  This is especially useful when
///  non blocking I/O is used.  The buffer should not be deleted or
///  reused while there's a current I/O outstanding
///
///  
/// \param     pv_IOInProgress        The new setting
///
/// \retval    true:                  The setting was successful
/// \retval    false:                 The setting was unsuccessful and
///                                   the value is unchanged.
///
///////////////////////////////////////////////////////////////////////////////

bool STFS_Message::SetMessageBufIOInProgress ( bool pv_IOInProgress) {

  messageBufInUse_ = pv_IOInProgress;
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///
//          SetMessageBuf
///
/// \brief  Default method to allocate the buffer containing the
///         actual message.
///
///         Sets the message buffer pointer to the user-specified buffer. If
///         there was previously a buffer there, this call overwrites it.  It's
///         up to the caller to deallocate the buffer...
///
///         If pv_PopulateHeader is set to true then this method populates a
///         few members of the STFS_Message class.
///
/// \param     void
///
/// \retval    Returns a pointer to the buffer.
///
///////////////////////////////////////////////////////////////////////////////
void
STFS_Message::SetMessageBuf  (void * pp_buffer, 
                              size_t pv_bufferLen, 
                              bool pv_PopulateHeader) {

  if ( ( pv_bufferLen > 0) && ( pp_buffer != NULL) ) {

    buf_ = (FixedMsg_u *) pp_buffer;
    bufLen_ = pv_bufferLen;
  }
  else {

    // we didn't get both a valid buffer and a valid buffer length.  Set 'em to
    // null and leave...

    ASSERT (false);
    buf_ = NULL;
    bufLen_ = 0;
  }

  if (buf_ && pv_PopulateHeader) {

    buf_->headerOnly.Validate();   /// Make sure we've got a valid message here
    messageType_ = buf_->headerOnly.GetMessageType();
    messagePath_ = buf_->headerOnly.GetMessagePath();
    bool lv_hasVarSection = false;
    varoffset_t lv_varSectionOffset = 0;

    fixedSize_ = bufLen_;
    variableSize_ = 0;

    buf_->headerOnly.GetVariableInfo (&lv_hasVarSection, 
                                      &lv_varSectionOffset);

    if (lv_hasVarSection == true) {
      variableSize_ = bufLen_ - lv_varSectionOffset;
      fixedSize_ = lv_varSectionOffset;
    }

  }
  else {
#ifdef DEBUG
  if (buf_ != NULL) {
    memset (buf_, STFS_GARBAGE_CHAR, pv_bufferLen);
  }
#endif
  }

}

///////////////////////////////////////////////////////////////////////////////
///
//          AppendToVariableSection
///
/// \brief  Default method to append the supplied data to the buffer's
///         variable section 
///
///  This method appends the supplied new data to the message buffer's
///  variable section.  It increments the message buf's variable size
///  by the amount of data appended.
///  
///  The number of bytes appended might be less than the number of
///  bytes passed.  If there isn't room in the variable section for
///  all the appended data, then the buffer is filled and the number
///  of bytes copied is returned.  The rest of the data must be sent
///  in a subsequent message.
///
/// \param     pp_newData             Pointer to the new data.  May
///                                   contain embedded zeroes.
/// \param     pv_newDataLen          Number of bytes to be copied
///
/// \retval    Returns the number of bytes actually copied. If less than
///            pv_newDataLen, then newData was larger than space
///            available.  If 0, then an error occurred.
///
///////////////////////////////////////////////////////////////////////////////

size_t STFS_Message::AppendToVariableSection  (char * pp_newData, 
                                               size_t pv_newDataLen) {

  size_t lv_bytesToCopy = pv_newDataLen;

  //////////////////////////
  // Check for room to copy
  //////////////////////////

  if ( bufLen_ < fixedSize_ + variableSize_ + pv_newDataLen ) {
    lv_bytesToCopy = bufLen_ - variableSize_ - fixedSize_;
  }

  //////////////////////////
  // Do we have a buffer to copy to?
  //////////////////////////

  if (buf_ == NULL) {
    STFS_SigIll (); // Something really bad happened:  buf_ not allocated
                    // allocated, but have a valid bufLen_!
    return 0;
  }

  //////////////////////////
  //  Copy the data
  //////////////////////////


  char *lv_copyLocation = &( buf_->charArray[fixedSize_+variableSize_] );

  // can't use strcpy() because the message could have embedded nulls
  memcpy (lv_copyLocation, pp_newData, lv_bytesToCopy );


  //////////////////////////
  // Bump variableSize_ 
  //////////////////////////

  variableSize_ = variableSize_ + lv_bytesToCopy;


  /////////////////////////
  // Finis!
  /////////////////////////
  return lv_bytesToCopy;
}


///////////////////////////////////////////////////////////////////////////////
///
//          ResetMessageBufContents
///
/// \brief  Clears the current message buffer.
///
///  This method resets the message buffer contents.  It does not
///  deallocate the message buffer or zero its complete contents.
///  However, it does set the initial byte to null, just to be safe.
///
///  In debug mode, it resets the values to STFS_GARBAGE_CHAR
///
/// \param     void
///
/// \retval    true:                  The message was reset
/// \retval    false:                 The message type was not reset
///                                    (programming error)
///
///////////////////////////////////////////////////////////////////////////////

bool STFS_Message::ResetMessageBufContents (void) {

  fixedSize_ = 0;
  variableSize_ = 0;

  if (messageBufInUse_) {
    // why are we trying to reset a message that's in use???
    ASSERT (false);
    return false;
  }

  if (buf_ != NULL) {
    buf_->charArray[0] = '\0';
  }

#ifdef DEBUG
  // for debugging we'll make the whole buffer garbage..."
  if (buf_ != NULL) {
    memset ( buf_, STFS_GARBAGE_CHAR, bufLen_);
  }
#endif

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_CreateFileRequest methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileRequest Default Constructor
///
/// \brief  Initializes an STFSMessage_CreateFileRequest to defaultvalues
///
///  Sets up the initial CreateFileRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_CreateFileRequest::STFSMessage_CreateFileRequest()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileRequest Constructor
///
/// \brief  Initializes an STFSMessage_CreateFileRequest to with
///         supplied template.
///
///  Sets up the initial CreateFileRequest structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pp_template     Filename template for the file to be created
/// \param       pv_isMkstemp    Is this an mkstemp or a straight create?
/// \param       pp_msgBufPtr    Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen       Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_CreateFileRequest::STFSMessage_CreateFileRequest(char *pp_template,
                                                             bool pv_isMkstemp,
                                                             int  pv_RequesterNodeId,
                                                             int  pv_RequesterPID,
                                                             void *pp_MsgBufPtr, 
                                                             size_t pv_bufLen
                                                             )
  : STFS_Message( STFS::MT_CreateFile, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  // the create message doesn't have a buffer, we can't initialize it.

  // Rev: We could set error in the STFS_Session.
  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->createFileReq.Init (pp_template, pv_isMkstemp, pv_RequesterNodeId, pv_RequesterPID);

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_CreateFile) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileRequest Destructor
///
/// \brief  Destructor for Create File Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_CreateFileRequest::~STFSMessage_CreateFileRequest() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->createFileReq.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileRequest::SetMessageBuffer
///
/// \brief  Sets the message pointer for the buffer to be sent
///
///  This method sets the message buffer location for the Create File Request
///  itself.  Callers are responsible for allocating and deallocating it after
///  all message processing is completed, but should not manipulate data within
///  the buffer.  The buffer must be at least the size returned by the
///  GetMessageBufMinSize method; it can be as much larger as desired.
///
///  If the message buffer location is NULL, then the message can't be
///  constructed or sent.  Use this method to reset it.  
///
/// \param    pp_msgBufPtr          pointer to the message buffer location
/// \param    pp_BufLen          size of the buffer
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
void 
STFSMessage_CreateFileRequest::SetMessageBuffer ( void * pp_msgBufPtr, 
                                                  size_t pv_bufLen) {

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }

  STFS_Message::SetMessageBuf (pp_msgBufPtr, pv_bufLen);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileRequest::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File request buffer
///
///  This method returns the minimum size for a CreateFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_CreateFileRequest::GetMessageBufMinSize (void) {

  // we don't have any variable section for a create file request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_CreateFile));
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileRequest::GetTemplate
///
/// \brief  Gets the file template 
///
/// \param    pp_pp_tgtTemplate  pointer to the template buffer
/// \param    pv_templateMaxSize max size of the template
///
/// \retval   void
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_CreateFileRequest::GetTemplate (char  *pp_tgtTemplate, 
                                            size_t pv_templateMaxSize) {

  return buf_->createFileReq.GetTemplate(pp_tgtTemplate,
                                         pv_templateMaxSize);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileRequest::GetIsMkstemp
///
/// \brief  Gets the IsMkstemp flag 
///
/// \retval bool
//////////////////////////////////////////////////////////////////////////////
bool
STFSMessage_CreateFileRequest::GetIsMkstemp (void) {

  return buf_->createFileReq.GetIsMkstemp();
}



///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_CreateFileReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply Default Constructor
///
/// \brief  Initializes an STFSMessage_CreateFileReqply to default values
///
///  Sets up the initial CreateFileRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_CreateFileReply::STFSMessage_CreateFileReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply Default Constructor
///
/// \brief  Initializes an STFSMessage_CreateFileReply to default values
///
///  Sets up the initial CreateFileRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param       pp_template               The updated file name template
/// \param       pp_OpenId                 The open identifier for this reply
/// \param       pp_msgBuf                 Buffer for the message
/// \param       pp_msgBufLen              Size of the buffer allocated.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_CreateFileReply
        ::STFSMessage_CreateFileReply(char *pp_template,
                                      STFS_OpenIdentifier *pp_OpenID,
                                      void *pp_MsgBufPtr, 
                                      size_t pv_bufLen
                                                             )
  : STFS_Message( STFS::MT_CreateFileReply, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  // the create message doesn't have a buffer, we can't initialize it.

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  buf_->createFileReply.Init (pp_template, pp_OpenID);

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_CreateFileReply));
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply Destructor
///
/// \brief  Destructor for Create File Reply
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_CreateFileReply::~STFSMessage_CreateFileReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->createFileReply.Release();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply::SetMessageBuffer
///
/// \brief  Sets the message pointer for the buffer to be sent
///
///  This method sets the message buffer location for the Create File reply
///  itself.  Callers are responsible for allocating and deallocating it after
///  all message processing is completed, but should not manipulate data within
///  the buffer.  The buffer must be at least the size returned by the
///  GetMsgBufMinSize method; it can be as much larger as desired.
///
///  If the message buffer location is NULL, then the message can't be
///  constructed or sent.  Use this method to reset it.  
///
/// \param    pp_msgBufPtr          pointer to the message buffer location
/// \param    pp_bufLen             the length of the buffer
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
void 
STFSMessage_CreateFileReply::SetMessageBuffer ( void * pp_msgBufPtr, 
                                                size_t pv_bufLen) {

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  STFS_Message::SetMessageBuf (pp_msgBufPtr, pv_bufLen);

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File reply buffer
///
///  This method returns the minimum size for a CreateFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
///  \param        void
///  \retval       void
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_CreateFileReply::GetMessageBufMinSize (void) {
  return (STFS::GetSizeOfFixedMessageBuffer(MT_CreateFileReply) 
                 + STFSMSGBUFV_VARIABLE_EFMMAX );
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply::GetTemplate
///
/// \brief  Retrieves the template from the message buffer
///
///  This method extracts the file name template from the reply.  The
///  message buffer must already be set to something other than NULL
///
///  \param     pp_tgtTemplate      Pointer to the target string location
///  \param     pp_templateMaxSize  Max size to copy.  If smaller than
///                                 STFS_NAME_MAX then the template will be
///                                 truncated 
///
///  \retval    void
/// 
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_CreateFileReply::GetTemplate (char * pp_tgtTemplate, 
                                          size_t pv_templateMaxSize) {

  return (buf_->createFileReply.GetTemplate(pp_tgtTemplate, pv_templateMaxSize));

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply::GetOpenIdentifier
///
/// \brief  Retrieves the open identifier from the message buffer
///
///  This method extracts the open identifier from the reply.  The message
///  buffer must already be set to something other than NULL
///
///  \param   void
///
///  \retval  The open identifier
/// 
//////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier
STFSMessage_CreateFileReply::GetOpenIdentifier (void) {

  return ( buf_->createFileReply.GetOpenIdentifier() );

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply::SetTemplate
///
/// \brief  Sets the template in a create file reply. 
///
///  This method sets the file name template to be sent back in a reply.  The
///  message buffer must already be set to something other than NULL
///
///  \param   pp_template  A string containing the new file name
///
///  \retval  void
/// 
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_CreateFileReply::SetTemplate ( char *pp_template ) {

  buf_->createFileReply.SetTemplate (pp_template);

}



///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply::SetOpenIdentifier
///
/// \brief  Sets the template in a create file reply. 
///
///  This method sets the file name template to be sent back in a reply.  The
///  message buffer must already be set to something other than NULL
///
///  \param   pp_openID    The new value for the open identivier
///
///  \retval  void
/// 
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_CreateFileReply::SetOpenIdentifier ( STFS_OpenIdentifier *pp_openID) {

  buf_->createFileReply.SetOpenIdentifier (pp_openID);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply::Pack
///
/// \brief  Packs the variable portion of a message for a Create File Reply
///
///  This method packs the EFM into the message buffer.  After a successful
///  conclusion to this method, the reply can be sent back to the library.
///
///  As part of the packing operation, the variable area of the message is
///  updated to include the EFM and all the FFMS in packed format.
///
///  \param   EFM      The EFM to be packed, including pointers to all FFMs
///
///  \retval  TRUE if packing succeeded, False otherwise.  If packing didn't
///            succeed, then the variable area offsets indicate no data present.
/// 
///
//////////////////////////////////////////////////////////////////////////////
bool
STFSMessage_CreateFileReply::Pack(STFS_ExternalFileMetadata *pp_EFM) {

  size_t lp_variableBufConsumed = 0;
  bool   lv_Stat = true;

  lv_Stat =  CommonPack(this->GetMessageType(),
                        pp_EFM,
                        buf_,
                        bufLen_,
                        &lp_variableBufConsumed
                        );

  if (lv_Stat) {
    SetMessageBufVariableSize(lp_variableBufConsumed);
  }

  return lv_Stat;
#if 0
  ///////////////////////////////////////
  ///  Validate Parameters
  ///////////////////////////////////////

  if (pp_EFM == NULL) {
    //  Why nothing to pack???
    ASSERT (false);
    return false;
  }

  ///////////////////////////////////////
  ///  Housekeeping
  ///////////////////////////////////////
 
  /// Get the start of the variable buffer
  char *lp_variableBufStart = (char *) buf_->charArray 
                                  + FixedMessageSizeTable[MT_CreateFileReply];
  size_t lv_totalVariableBufSize = bufLen_ - FixedMessageSizeTable [MT_CreateFileReply];

  char *lp_variableBufCurrLoc = lp_variableBufStart;

  size_t lp_variableBufConsumed = 0;

  ///////////////////////////////////////
  ///Pack the EFM
  ///////////////////////////////////////

  size_t lv_eFMPackedLen;
  bool success = pp_EFM->Pack(lp_variableBufCurrLoc, 
                              lv_totalVariableBufSize,
                              &lv_eFMPackedLen);

  if (!success) {
    // out of memory or a programming problem!
    return false;
  }

  varoffset_t lv_eFMOffset = lp_variableBufConsumed;
  lp_variableBufConsumed += lv_eFMPackedLen;
  lp_variableBufCurrLoc = lp_variableBufStart + lp_variableBufConsumed;


  ///////////////////////////////////////
  /// Set the array with FFM offsets
  ///////////////////////////////////////

  int lv_numFragments = pp_EFM->GetNumFragments();

  varoffset_t lv_fragArrayOffset = lp_variableBufConsumed;

  // Next comes an array of the FFM offsets!  Skip over that
  varoffset_t *lv_fragOffsetArray = (varoffset_t *) lp_variableBufCurrLoc;
  lp_variableBufConsumed += (lv_numFragments * sizeof (varoffset_t));
  lp_variableBufCurrLoc = lp_variableBufStart + lp_variableBufConsumed;

  ///////////////////////////////////////
  ///  Loop over FFMs, packing them
  ///////////////////////////////////////

  STFS_FragmentFileMetadata *lv_localFFM = NULL;
  size_t lv_fFMPackedLen;

  for (int lv_fragIndex = 0; lv_fragIndex < lv_numFragments; lv_fragIndex++) {

    lv_fragOffsetArray[lv_fragIndex] = lp_variableBufConsumed;

    lv_localFFM = pp_EFM->GetFragment(lv_fragIndex);

    if (lv_localFFM == NULL) {
      //HUH?  There was supposed to be a fragment here!
      ASSERT (false);
      //I'll be kind and just return False.  Really, this is probably a SigIll()
      return false;
    }

    success = lv_localFFM->Pack (lp_variableBufCurrLoc, 
                                 lv_totalVariableBufSize,
                                 &lv_fFMPackedLen);

    if (success != true) {
      // packing error!
      return false;
    }

    lp_variableBufConsumed += lv_fFMPackedLen;
    lp_variableBufCurrLoc = lp_variableBufStart + lp_variableBufConsumed;

  }

  ///////////////////////////////////////
  ///  Set metadata info in message
  ///////////////////////////////////////

  size_t lv_totalFFMSize = lp_variableBufConsumed - lv_fragArrayOffset;

  buf_->createFileReply.SetVarMetadataControl (lv_eFMOffset, lv_numFragments,
                                               lv_fragArrayOffset, 
                                               lv_totalFFMSize );

  buf_->headerOnly.SetVariableInfo(true, FixedMessageSizeTable [MT_CreateFileReply]);
  buf_->createFileReply.SetVarAreaSize(lp_variableBufConsumed);

  SetMessageBufVariableSize ( lp_variableBufConsumed );

  ///////////////////////////////////////
  /// Finis!
  ///////////////////////////////////////

  return true;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFileReply::Unpack
///
/// \brief  Unpacks the variable portion of a message for a Create File Reply
///
///  This method unpacks the EFM into locally allocated FFMs and EFMs.  Then it
///  returns a pointer to these.  It's the caller's responsibility to deallocate
///  them when finished.
///
///  The message is unpacked from buf_.  Use the set method to set the buffer
///  without initializing it.  This method does not unpack the fixed portion of
///  the message.
///
///  The variable portition of the message is unchanged as the result of this
///  call.  The same message can be unpacked multiple times; a new EFM is
///  allocated at each call.
///
///  \param   void
///
///  \retval  Pointer to the newly allocated EFM, with valid pointers to FFMs as
///               appropriate.  NULL if not successful.
/// 
///
//////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileMetadata *
STFSMessage_CreateFileReply::Unpack(void) {


  ////////////////////////////////////
  ///  Parameter checking
  ////////////////////////////////////
  if (GetMessageType() != MT_CreateFileReply) {
    // who called us wanting to unpack something other than a CreateFileReply??
    ASSERT (false);
    return NULL;
  }

  STFS_Message::SetMessageBufVariableSize(buf_->createFileReply.GetVarAreaSize()); 
  if ( GetMessageVariableSize() == 0 ) {
    //weirdness:  We don't have anything to unpack!
    ASSERT (false);
    return NULL;
  }

  STFS_ExternalFileMetadata *lp_eFM = CommonUnpack(GetMessageType(),
                                                   buf_);

#if 0
  /////////////////////////////////////
  /// Housekeeping
  /////////////////////////////////////

  char *lp_variableBufStart = (char *) buf_->charArray 
                                 + FixedMessageSizeTable [MT_CreateFileReply];

  varoffset_t lp_eFMOffset = 0;
  int lv_numFragments = 0;
  varoffset_t lp_firstFFMOffset = 0;
  size_t lp_totalFFMSize = 0;

  size_t lv_unpackSize = 0;

  // Remember, these offsets are from the start of the variable area, not the
  // start of buf_

  buf_->createFileReply.GetVarMetadataControl (&lp_eFMOffset, &lv_numFragments,
                                               &lp_firstFFMOffset, 
                                               &lp_totalFFMSize);


  ////////////////////////////////////
  ///  Allocate new EFM
  ////////////////////////////////////

  STFS_ExternalFileMetadata *lp_eFM = new STFS_ExternalFileMetadata();

  if (lp_eFM == NULL) {
    // no memory apparently
    return NULL;
  }

  /////////////////////////////////////
  /// Find the EFM in the variable buffer
  /////////////////////////////////////

  char *lp_packedEFMPtr = lp_variableBufStart + lp_eFMOffset;

  /////////////////////////////////////
  /// Unpack the EFM
  /////////////////////////////////////

  lv_unpackSize = lp_firstFFMOffset - lp_eFMOffset;

  bool success =  lp_eFM->Unpack (lp_packedEFMPtr, lv_unpackSize );

  if (success != true ) {
    // unpacking failure
    delete lp_eFM;
    return NULL;
  }

  /////////////////////////////////////
  /// Get the fragment offset array
  /////////////////////////////////////

  varoffset_t *lp_offsetArray = (varoffset_t *)(lp_variableBufStart + lp_firstFFMOffset);

  /////////////////////////////////////
  /// Unpack each fragment
  /////////////////////////////////////

  char *lp_packedFFMBuf = NULL;
  short success_short = -1;

  //  initialize the first fragment's size
  lv_unpackSize = lp_offsetArray[0] - lv_numFragments*sizeof (varoffset_t);

  for (int fragIndex = 0; fragIndex < lv_numFragments; fragIndex++) {

    ///////////////////////////////////////
    ///  Get the address of the packed FFM
    ///////////////////////////////////////

    lp_packedFFMBuf = lp_variableBufStart + lp_offsetArray[fragIndex];
    

    // we need a new FFM for each fragment we unpack, so we allocate them in
    // this method

    STFS_FragmentFileMetadata *lp_fFM = new STFS_FragmentFileMetadata();

    success = lp_fFM->Unpack (lp_packedFFMBuf, lv_unpackSize);

    if (success != true) {
      //unpack failed, time to bail!
      //  Have to delete the current FFM because we haven't linked it to the EFM yet
      delete lp_fFM;
      delete lp_eFM;
      return NULL;
    }


  ////////////////////////////////////////
  /// Link fragment to EFM
  ////////////////////////////////////////

    success_short = lp_eFM->InsertFFM (lp_fFM);
    if (success_short != 0) {
      //insertFFM failed! Time to bail!
      //  Have to delete the current FFM because we haven't linked it to the EFM yet
      delete lp_fFM;
      delete lp_eFM;
      return NULL;
    }

    /////////////////////////////////////
    ///  End of loop
    /////////////////////////////////////
    lv_unpackSize = lp_offsetArray[fragIndex+1] - lp_offsetArray[fragIndex];;
  }
#endif

  /////////////////////////////////////
  /// Finis!
  /////////////////////////////////////
  return lp_eFM;

}

//****************************************************************************
///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_CreateFragmentRequest methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentRequest Default Constructor
///
/// \brief  Initializes an STFSMessage_CreateFragmentRequest to defaultvalues
///
///  Sets up the initial CreateFragmentRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_CreateFragmentRequest::STFSMessage_CreateFragmentRequest()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentRequest Constructor
///
/// \brief  Initializes an STFSMessage_CreateFragmentRequest with
///         supplied information.
///
///  Sets up the initial CreateFragmentRequest structure, using the
///  supplied information.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pp_openID          Pointer to the open identifier for this frag
/// \param       pp_msgBufPtr       Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen          Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_CreateFragmentRequest
                     ::STFSMessage_CreateFragmentRequest(STFS_OpenIdentifier *pp_openID,
							 int  pv_RequesterNodeId,
							 int  pv_RequesterPID,
							 void *pp_MsgBufPtr, 
							 size_t pv_bufLen
							 )
  : STFS_Message( STFS::MT_CreateFragment, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  // the create message doesn't have a buffer, we can't initialize it.

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->createFragmentReq.Init (pp_openID,pv_RequesterNodeId, pv_RequesterPID);

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_CreateFragment) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentRequest Destructor
///
/// \brief  Destructor for Create File Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_CreateFragmentRequest::~STFSMessage_CreateFragmentRequest() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->createFragmentReq.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentRequest::SetMessageBuffer
///
/// \brief  Sets the message pointer for the buffer to be sent
///
///  This method sets the message buffer location for the Create File Request
///  itself.  Callers are responsible for allocating and deallocating it after
///  all message processing is completed, but should not manipulate data within
///  the buffer.  The buffer must be at least the size returned by the
///  GetMessageBufMinSize method; it can be as much larger as desired.
///
///  If the message buffer location is NULL, then the message can't be
///  constructed or sent.  Use this method to reset it.  
///
/// \param    pp_msgBufPtr          pointer to the message buffer location
/// \param    pp_BufLen          size of the buffer
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
void 
STFSMessage_CreateFragmentRequest::SetMessageBuffer ( void * pp_msgBufPtr, 
                                                  size_t pv_bufLen) {

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }

  STFS_Message::SetMessageBuf (pp_msgBufPtr, pv_bufLen);

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentRequest::SetOpenIdentifier
///
/// \brief  Sets the template in a create fragment request. 
///
///  This method sets the file name template to be sent in a request.  The
///  message buffer must already be set to something other than NULL
///
///  \param   pp_openID    The new value for the open identivier
///
///  \retval  void
/// 
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_CreateFragmentRequest::SetOpenIdentifier ( STFS_OpenIdentifier *pp_openID) {

  buf_->createFragmentReq.SetOpenIdentifier (pp_openID);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentRequest::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File request buffer
///
///  This method returns the minimum size for a CreateFragmentReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_CreateFragmentRequest::GetMessageBufMinSize (void) {

  // we don't have any variable section for a create fragment request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_CreateFragment));
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentRequest::GetOpenID
///
/// \brief  Gets the Openid flag from the message
///
/// \retval STFS_OpenIdentifier
//////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier
STFSMessage_CreateFragmentRequest::GetOpenID (void) {

  return ( buf_->createFragmentReq.GetOpenIdentifier() );

}



///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_CreateFragmentReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentReply Default Constructor
///
/// \brief  Initializes an STFSMessage_CreateFragmentReqply to default values
///
///  Sets up the initial CreateFragmentRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_CreateFragmentReply::STFSMessage_CreateFragmentReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentReply Default Constructor
///
/// \brief  Initializes an STFSMessage_CreateFragmentReply to default values
///
///  Sets up the initial CreateFragmentRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param       pp_msgBuf                 Buffer for the message
/// \param       pp_msgBufLen              Size of the buffer allocated.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_CreateFragmentReply
        ::STFSMessage_CreateFragmentReply( void *pp_MsgBufPtr, 
					   size_t pv_bufLen )
  : STFS_Message( STFS::MT_CreateFragmentReply, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  // the create message doesn't have a buffer, we can't initialize it.

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  buf_->createFragmentReply.Init ();

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_CreateFragmentReply));
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentReply Destructor
///
/// \brief  Destructor for Create File Reply
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_CreateFragmentReply::~STFSMessage_CreateFragmentReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->createFragmentReply.Release();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentReply::SetMessageBuffer
///
/// \brief  Sets the message pointer for the buffer to be sent
///
///  This method sets the message buffer location for the Create File reply
///  itself.  Callers are responsible for allocating and deallocating it after
///  all message processing is completed, but should not manipulate data within
///  the buffer.  The buffer must be at least the size returned by the
///  GetMsgBufMinSize method; it can be as much larger as desired.
///
///  If the message buffer location is NULL, then the message can't be
///  constructed or sent.  Use this method to reset it.  
///
/// \param    pp_msgBufPtr          pointer to the message buffer location
/// \param    pp_bufLen             the length of the buffer
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
void 
STFSMessage_CreateFragmentReply::SetMessageBuffer ( void * pp_msgBufPtr, 
                                                size_t pv_bufLen) {

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  STFS_Message::SetMessageBuf (pp_msgBufPtr, pv_bufLen);

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File reply buffer
///
///  This method returns the minimum size for a CreateFragmentReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
///  \param        void
///  \retval       void
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_CreateFragmentReply::GetMessageBufMinSize (void) {
  return (STFS::GetSizeOfFixedMessageBuffer(MT_CreateFragmentReply) 
                 + STFSMSGBUFV_VARIABLE_EFMMAX );
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentReply::Pack
///
/// \brief  Packs the variable portion of a message for a Create Fragment Reply
///
///  This method packs the EFM into the message buffer.  After a successful
///  conclusion to this method, the reply can be sent back to the library.
///
///  As part of the packing operation, the variable area of the message is
///  updated to include the EFM and all the FFMS in packed format.
///
///  \param   EFM      The EFM to be packed, including pointers to all FFMs
///
///  \retval  TRUE if packing succeeded, False otherwise.  If packing didn't
///            succeed, then the variable area offsets indicate no data present.
/// 
///
//////////////////////////////////////////////////////////////////////////////
bool
STFSMessage_CreateFragmentReply::Pack(STFS_ExternalFileMetadata *pp_EFM) {

  size_t lp_variableBufConsumed = 0;
  bool   lv_Stat = true;

  lv_Stat =  CommonPack(this->GetMessageType(),
                        pp_EFM,
                        buf_,
                        bufLen_,
                        &lp_variableBufConsumed
                        );

  if (lv_Stat) {
    SetMessageBufVariableSize(lp_variableBufConsumed);
  }

  return lv_Stat;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CreateFragmentReply::Unpack
///
/// \brief  Unpacks the variable portion of a message for a Create FragmentReply
///
///  This method unpacks the EFM into locally allocated FFMs and EFMs.  Then it
///  returns a pointer to these.  It's the caller's responsibility to deallocate
///  them when finished.
///
///  The message is unpacked from buf_.  Use the set method to set the buffer
///  without initializing it.  This method does not unpack the fixed portion of
///  the message.
///
///  The variable portition of the message is unchanged as the result of this
///  call.  The same message can be unpacked multiple times; a new EFM is
///  allocated at each call.
///
///  \param   void
///
///  \retval  Pointer to the newly allocated EFM, with valid pointers to FFMs as
///               appropriate.  NULL if not successful.
/// 
///
//////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileMetadata *
STFSMessage_CreateFragmentReply::Unpack(void) {


  ////////////////////////////////////
  ///  Parameter checking
  ////////////////////////////////////
  if (GetMessageType() != MT_CreateFragmentReply) {
    // who called us wanting to unpack something other than a CreateFragmentReply??
    ASSERT (false);
    return NULL;
  }

  ////////////////////////////////////
  ///  Common unpack
  ////////////////////////////////////

  STFS_Message::SetMessageBufVariableSize(buf_->createFragmentReply.GetVarAreaSize()); 
  if ( GetMessageVariableSize() == 0 ) {
    //weirdness:  We don't have anything to unpack!
    ASSERT (false);
    return NULL;
  }

  STFS_ExternalFileMetadata *lp_eFM = CommonUnpack(GetMessageType(),
                                                   buf_);



  /////////////////////////////////////
  /// Finis!
  /////////////////////////////////////
  return lp_eFM;

}


///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_OpenFileRequest methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileRequest Default Constructor
///
/// \brief  Initializes an STFSMessage_OpenFileRequest to defaultvalues
///
///  Sets up the initial OpenFileRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_OpenFileRequest::STFSMessage_OpenFileRequest()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileRequest Constructor
///
/// \brief  Initializes an STFSMessage_OpenFileRequest to with
///         supplied filename.
///
///  Sets up the initial OpenFileRequest structure, using the
///  supplied filename.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pp_FileName     Name of the file to be opend
/// \param       pv_OpenFlag     Open Flags
/// \param       pp_msgBufPtr    Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen       Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_OpenFileRequest::STFSMessage_OpenFileRequest(char  *pp_FileName,
                                                         int    pv_OpenFlag,
                                                         int    pv_RequesterNodeId,
                                                         int    pv_RequesterPID,
                                                         void  *pp_MsgBufPtr, 
                                                         size_t pv_bufLen
                                                         )
  : STFS_Message( STFS::MT_OpenFile, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  // the open message doesn't have a buffer, we can't initialize it.
  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->openFileReq.Init (pp_FileName,
                          pv_OpenFlag, 
                          pv_RequesterNodeId,
                          pv_RequesterPID);

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_OpenFile) );
  SetMessageBufVariableSize (0);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileRequest Destructor
///
/// \brief  Destructor for Open File Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_OpenFileRequest::~STFSMessage_OpenFileRequest() 
{

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->openFileReq.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileRequest::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Open File request buffer
///
///  This method returns the minimum size for a OpenFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_OpenFileRequest::GetMessageBufMinSize (void) 
{
  // we don't have any variable section for a open file request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_OpenFile));
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileRequest::GetFileName
///
/// \brief  Gets the file filename 
///
/// \param    pp_FileName  pointer to the filename buffer
/// \param    pv_FileNameMaxSize max size of the filename
///
/// \retval   void
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_OpenFileRequest::GetFileName (char  *pp_FileName, 
                                          size_t pv_FileNameMaxSize) 
{

  return buf_->openFileReq.GetFileName(pp_FileName,
                                       pv_FileNameMaxSize);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileRequest::GetOpenFlag
///
/// \brief  Gets the Open Flag
///
/// \retval   Open Flag
///
//////////////////////////////////////////////////////////////////////////////
int
STFSMessage_OpenFileRequest::GetOpenFlag ( void )
{

  return buf_->openFileReq.GetOpenFlag();
}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_OpenFileReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply Default Constructor
///
/// \brief  Initializes an STFSMessage_OpenFileReqply to default values
///
///  Sets up the initial OpenFileRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_OpenFileReply::STFSMessage_OpenFileReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply Default Constructor
///
/// \brief  Initializes an STFSMessage_OpenFileReply to default values
///
///  Sets up the initial OpenFileRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param       pp_Filename               The updated file name filename
/// \param       pp_OpenId                 The open identifier for this reply
/// \param       pp_msgBuf                 Buffer for the message
/// \param       pp_msgBufLen              Size of the buffer allocated.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_OpenFileReply
        ::STFSMessage_OpenFileReply(char                *pp_FileName,
                                    STFS_OpenIdentifier *pp_OpenID,
                                    void                *pp_MsgBufPtr, 
                                    size_t               pv_bufLen)
  : STFS_Message( STFS::MT_OpenFileReply, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  // the open message doesn't have a buffer, we can't initialize it.

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  buf_->openFileReply.Init (pp_FileName, pp_OpenID);

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_OpenFileReply));
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply Destructor
///
/// \brief  Destructor for Open File Reply
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_OpenFileReply::~STFSMessage_OpenFileReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->openFileReply.Release();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply::SetMessageBuffer
///
/// \brief  Sets the message pointer for the buffer to be sent
///
///  This method sets the message buffer location for the Open File reply
///  itself.  Callers are responsible for allocating and deallocating it after
///  all message processing is completed, but should not manipulate data within
///  the buffer.  The buffer must be at least the size returned by the
///  GetMsgBufMinSize method; it can be as much larger as desired.
///
///  If the message buffer location is NULL, then the message can't be
///  constructed or sent.  Use this method to reset it.  
///
/// \param    pp_msgBufPtr          pointer to the message buffer location
/// \param    pp_bufLen             the length of the buffer
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
void 
STFSMessage_OpenFileReply::SetMessageBuffer ( void * pp_msgBufPtr, 
                                              size_t pv_bufLen) {

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  STFS_Message::SetMessageBuf (pp_msgBufPtr, pv_bufLen);

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Open File reply buffer
///
///  This method returns the minimum size for a OpenFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
///  \param        void
///  \retval       void
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_OpenFileReply::GetMessageBufMinSize (void) {

  return (STFS::GetSizeOfFixedMessageBuffer(MT_OpenFileReply) 
                 + STFSMSGBUFV_VARIABLE_EFMMAX );
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply::GetFilename
///
/// \brief  Retrieves the filename from the message buffer
///
///  This method extracts the file name filename from the reply.  The
///  message buffer must already be set to something other than NULL
///
///  \param     pp_tgtFilename      Pointer to the target string location
///  \param     pp_filenameMaxSize  Max size to copy.  If smaller than
///                                 STFS_NAME_MAX then the filename will be
///                                 truncated 
///
///  \retval    void
/// 
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_OpenFileReply::GetFileName (char  *pp_FileName, 
                                        size_t pv_FileNameMaxsize) {

  return (buf_->openFileReply.GetFileName(pp_FileName, pv_FileNameMaxsize));

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply::GetOpenIdentifier
///
/// \brief  Retrieves the open identifier from the message buffer
///
///  This method extracts the open identifier from the reply.  The message
///  buffer must already be set to something other than NULL
///
///  \param   void
///
///  \retval  The open identifier
/// 
//////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier
STFSMessage_OpenFileReply::GetOpenIdentifier (void) {

  return ( buf_->openFileReply.GetOpenIdentifier() );

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply::Pack
///
/// \brief  Packs the variable portion of a message for a Open File Reply
///
///  This method packs the EFM into the message buffer.  After a successful
///  conclusion to this method, the reply can be sent back to the library.
///
///  As part of the packing operation, the variable area of the message is
///  updated to include the EFM and all the FFMS in packed format.
///
///  \param   EFM      The EFM to be packed, including pointers to all FFMs
///
///  \retval  TRUE if packing succeeded, False otherwise.  If packing didn't
///            succeed, then the variable area offsets indicate no data present.
/// 
///
//////////////////////////////////////////////////////////////////////////////
bool
STFSMessage_OpenFileReply::Pack(STFS_ExternalFileMetadata *pp_EFM) {

  size_t lp_variableBufConsumed = 0;
  bool   lv_Stat = true;

  lv_Stat =  CommonPack(this->GetMessageType(),
                        pp_EFM,
                        buf_,
                        bufLen_,
                        &lp_variableBufConsumed
                        );

  if (lv_Stat) {
    SetMessageBufVariableSize(lp_variableBufConsumed);
  }

  return lv_Stat;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenFileReply::Unpack
///
/// \brief  Unpacks the variable portion of a message for a Open File Reply
///
///  This method unpacks the EFM into locally allocated FFMs and EFMs.  Then it
///  returns a pointer to these.  It's the caller's responsibility to deallocate
///  them when finished.
///
///  The message is unpacked from buf_.  Use the set method to set the buffer
///  without initializing it.  This method does not unpack the fixed portion of
///  the message.
///
///  The variable portition of the message is unchanged as the result of this
///  call.  The same message can be unpacked multiple times; a new EFM is
///  allocated at each call.
///
///  \param   void
///
///  \retval  Pointer to the newly allocated EFM, with valid pointers to FFMs as
///               appropriate.  NULL if not successful.
/// 
///
//////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileMetadata *
STFSMessage_OpenFileReply::Unpack(void) {


  ////////////////////////////////////
  ///  Parameter checking
  ////////////////////////////////////
  if (GetMessageType() != MT_OpenFileReply) {
    // who called us wanting to unpack something other than a OpenFileReply??
    ASSERT (false);
    return NULL;
  }

  STFS_Message::SetMessageBufVariableSize(buf_->openFileReply.GetVarAreaSize()); 
  if ( GetMessageVariableSize() == 0 ) {
    //weirdness:  We don't have anything to unpack!
    ASSERT (false);
    return NULL;
  }

  STFS_ExternalFileMetadata *lp_eFM = CommonUnpack(GetMessageType(),
                                                   buf_);

  /////////////////////////////////////
  /// Finis!
  /////////////////////////////////////
  return lp_eFM;

}

//****************************************************************
///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_ErrorReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply Default Constructor
///
/// \brief  Initializes an STFSMessage_ErrorReply to default values
///
///  Sets up the initial ErrorReply structure, based on specified
///  message types.  Buffers are not allocated here.  
///
///  THIS METHOD DOES NOT PACK THE VARIABLE AREA.  DO THAT EXPLICITLY!
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_ErrorReply::STFSMessage_ErrorReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply Parameterized Constructor
///
/// \brief  Initializes an STFSMessage_CreateFileReply to specified values
///
///  Sets up the initial ErrorReply structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param       pp_Error                  The errors to report
/// \param       pp_msgBuf                 Buffer for the message
/// \param       pp_msgBufLen              Size of the buffer allocated.
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_ErrorReply::STFSMessage_ErrorReply(STFS_Error * pp_Error,
                                               void *pp_MsgBufPtr, 
                                               size_t pv_bufLen
                                                             )
  : STFS_Message( STFS::MT_ErrorReply, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  // the create message doesn't have a buffer, we can't initialize it.

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  buf_->errorReply.Init (pp_Error);
  SetMessageFixedSize (sizeof (STFSMsgBuf_ErrorReply) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply Destructor
///
/// \brief  Destructor for an Error Reply
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_ErrorReply::~STFSMessage_ErrorReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->errorReply.Release();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply::SetMessageBuffer
///
/// \brief  Sets the message pointer for the buffer to be sent
///
///  This method sets the message buffer location for the error reply
///  itself.  Callers are responsible for allocating and deallocating it after
///  all message processing is completed, but should not manipulate data within
///  the buffer.  The buffer must be at least the size returned by the
///  GetMsgBufMinSize method; it can be as much larger as desired.
///
///  If the message buffer location is NULL, then the message can't be
///  constructed or sent.  Use this method to reset it.  
///
/// \param    pp_msgBufPtr          pointer to the message buffer location
/// \param    pp_bufLen             the length of the buffer
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
void 
STFSMessage_ErrorReply::SetMessageBuffer ( void * pp_msgBufPtr, 
                                           size_t pv_bufLen) {

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }

  STFS_Message::SetMessageBuf (pp_msgBufPtr, pv_bufLen);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for an error reply buffer
///
///  This method returns the minimum size for a error reply buffer, including
///  both fixed and variable areas.  The buffer in which this message is
///  constructed must be at least this size.
///
///  \param        void
///  \retval       void
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_ErrorReply::GetMessageBufMinSize (void) {

  return (STFS::GetSizeOfFixedMessageBuffer (MT_ErrorReply)
                 + STFSMSGBUFV_VARIABLE_ERRMAX );
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply::SetNumErrors
///
/// \brief  Sets the number of errors in an error reply
///
///  This method directly sets the number of errors sent back in a reply.  The
///  message buffer must already be set to something other than NULL
///
///  \param   pv_numErrors    The new value for the number of errors
///
///  \retval  void
/// 
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_ErrorReply::SetNumErrors ( int pv_numErrors) {

  buf_->errorReply.SetNumErrors (pv_numErrors);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply::SetReportedError
///
/// \brief  Sets the Reported Error in an error reply
///
///  This method directly sets the reported error sent back in a reply.  The
///  message buffer must already be set to something other than NULL
///
///  \param   pv_reportedError    The new value for the reported error
///
///  \retval  void
/// 
///
//////////////////////////////////////////////////////////////////////////////
void
STFSMessage_ErrorReply::SetReportedError ( int pv_reportedError) {

  buf_->errorReply.SetReportedError (pv_reportedError);

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply::GetNumErrors
///
/// \brief  Gets the number of errors in an error reply
///
///  This method retrieves the number of errors sent back in a reply.  The
///  message buffer must already be set to something other than NULL
///
///  \param   void
///
///  \retval  pv_numErrors    The number of errors
/// 
///
//////////////////////////////////////////////////////////////////////////////
int
STFSMessage_ErrorReply::GetNumErrors ( void) {

  return ( buf_->errorReply.GetNumErrors () );

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply::GetReportedError
///
/// \brief  Gets the Reported Error from an error reply
///
///  This method retrieves the reported error sent back in a reply.  The
///  message buffer must already be set to something other than NULL
///
///  \param   void
///
///  \retval  pv_reportedError    The reported error
/// 
///
//////////////////////////////////////////////////////////////////////////////
int
STFSMessage_ErrorReply::GetReportedError (void) {

  return ( buf_->errorReply.GetReportedError () );

}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply::Pack
///
/// \brief  Packs the variable portion of a message for a Create File Reply
///
///  This method packs the error structure into the message buffer.  After a
///  successful conclusion to this method, the reply can be sent back to the
///  library.
///
///
///  \param   ErrStruct     The error structure
///
///  \retval  TRUE if packing succeeded, False otherwise.  If packing didn't
///            succeed, then the variable area offsets indicate no data present.
/// 
///
//////////////////////////////////////////////////////////////////////////////
bool
STFSMessage_ErrorReply::Pack(STFS_Error *pp_Error) {

  //////////////////////////////////
  /// Validate Parameters
  //////////////////////////////////

  if (pp_Error == NULL) {
    //  Why nothing to pack???
    ASSERT (false);
    return false;
  }

  //////////////////////////////////
  ///  Housekeeping
  //////////////////////////////////

  // Get the start of the variable buffer

  char *lp_variableBufStart = (char *) buf_->charArray
                                 + STFS::GetSizeOfFixedMessageBuffer (MT_ErrorReply);

  char *lp_variableBufCurrentLoc = lp_variableBufStart;
  size_t lv_variableBufConsumed = 0;
  size_t lv_packingSpaceAvail = bufLen_ - lv_variableBufConsumed;

  size_t lv_packedErrorLen = 0;

  ///////////////////////////////////
  /// Pack the error structure
  ///////////////////////////////////

  bool lv_success = pp_Error->Pack (lp_variableBufCurrentLoc, 
                                    lv_packingSpaceAvail,
                                    &lv_packedErrorLen);

  if (lv_success != true) {
    return false;
  }

  lp_variableBufCurrentLoc += lv_packedErrorLen;
  lv_variableBufConsumed += lv_packedErrorLen;

  //////////////////////////////////
  ///  Set message variable info
  //////////////////////////////////
  
  STFS_Message::SetMessageBufVariableSize(lv_variableBufConsumed);

  //////////////////////////////////
  /// Finis!
  //////////////////////////////////

  return true;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_ErrorReply::Unpack
///
/// \brief  Unpacks the variable portion of a message for an Error Reply
///
///  This method unpacks the STFS_Error structure into a locally allocated copy
///  of the structure. Then it returns a pointer to this new structure.  It's
///  the caller's responsibility to deallocate them when finished.
///
///  The variable portition of the message is unchanged as the result of this
///  call.  The same message can be unpacked multiple times; a new error
///  structure is allocated at each call.
///
///  Every request can receive an error reply, so be prepared to get one at any
///  time!
///
///  \param   void
///
///  \retval  Pointer to the newly allocated STFS_Error structure. NULL if not
///                            successful. 
/// 
///
//////////////////////////////////////////////////////////////////////////////
STFS_Error *
STFSMessage_ErrorReply::Unpack(void) {


  //////////////////////////////////
  ///  Housekeeping
  //////////////////////////////////

  // Get the start of the variable buffer

  char *lp_variableBufStart = (char *) buf_->charArray
                                 + STFS::GetSizeOfFixedMessageBuffer (MT_ErrorReply);

  char *lp_variableBufCurrentLoc = lp_variableBufStart;

  ///////////////////////////////////
  /// Allocate a new error structure
  ///////////////////////////////////

  STFS_Error *lp_unpackedError = new STFS_Error();

  if (lp_unpackedError == NULL) {
    // no memory apparently
    return NULL;
  }

  ///////////////////////////////////
  ///  Find the error structure in the variable buf
  ///////////////////////////////////

  char *lp_packedErrStruct = lp_variableBufCurrentLoc;

  // right now, the packed error is the only thing in the variable
  // buffer.  That could change, so we'll use variables from the
  // start.

  size_t lp_packedErrStructLen = STFS_Message::GetMessageVariableSize();


  ///////////////////////////////////
  /// Unpack the error structure
  ///////////////////////////////////

  bool lv_success =  lp_unpackedError->Unpack(lp_packedErrStruct,
                                              lp_packedErrStructLen);

  if (lv_success != true) {
    // have to delete the unpacked error structure
    delete lp_unpackedError;
    return NULL;

  }

  return lp_unpackedError;

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_CloseFileRequest methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileRequest Default Constructor
///
/// \brief  Initializes an STFSMessage_CloseFileRequest to defaultvalues
///
///  Sets up the initial CloseFileRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_CloseFileRequest::STFSMessage_CloseFileRequest()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileRequest Constructor
///
/// \brief  Initializes an STFSMessage_CloseFileRequest to with
///         supplied template.
///
///  Sets up the initial CloseFileRequest structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pp_OpenId       Pointer to OpenId
/// \param       pp_msgBufPtr    Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen       Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_CloseFileRequest::STFSMessage_CloseFileRequest(STFS_OpenIdentifier *pp_OpenId,
                                                           int   pv_requesterNodeId,
                                                           int   pv_requesterPID, 
                                                           void *pp_MsgBufPtr, 
                                                           size_t pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_CloseFile, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->closeFileReq.Init (pp_OpenId,
                           pv_requesterNodeId,
                           pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_CloseFile) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileRequest Destructor
///
/// \brief  Destructor for Create File Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_CloseFileRequest::~STFSMessage_CloseFileRequest() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->closeFileReq.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileRequest::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File request buffer
///
///  This method returns the minimum size for a CreateFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_CloseFileRequest::GetMessageBufMinSize (void) {

  // we don't have any variable section for a Close file request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_CloseFile));
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileRequest::GetTemplate
///
/// \brief  Gets the file template 
///
/// \retval   void
///
//////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier *
STFSMessage_CloseFileRequest::GetOpenIdentifier( void ) {

  return buf_->closeFileReq.GetOpenIdentifier();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_CloseFileReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileReply Default Constructor
///
/// \brief  Initializes an STFSMessage_CloseFileReply to defaultvalues
///
///  Sets up the initial CloseFileReply structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_CloseFileReply::STFSMessage_CloseFileReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileReply Constructor
///
/// \brief  Initializes an STFSMessage_CloseFileReply to with
///         supplied template.
///
///  Sets up the initial CloseFileReply structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pp_OpenId       Pointer to OpenId
/// \param       pp_msgBufPtr    Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen       Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_CloseFileReply::STFSMessage_CloseFileReply(STFS_OpenIdentifier *pp_OpenId,
                                                           int   pv_requesterNodeId,
                                                           int   pv_requesterPID, 
                                                           void *pp_MsgBufPtr, 
                                                           size_t pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_CloseFile, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->closeFileReply.Init (pp_OpenId,
                           pv_requesterNodeId,
                           pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_CloseFile) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileReply Destructor
///
/// \brief  Destructor for Create File Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_CloseFileReply::~STFSMessage_CloseFileReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->closeFileReply.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File request buffer
///
///  This method returns the minimum size for a CreateFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_CloseFileReply::GetMessageBufMinSize (void) {

  // we don't have any variable section for a Close file reply
  return (STFS::GetSizeOfFixedMessageBuffer(MT_CloseFile));
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_CloseFileReply::GetTemplate
///
/// \brief  Gets the file template 
///
/// \retval   void
///
//////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier *
STFSMessage_CloseFileReply::GetOpenIdentifier( void ) {

  return buf_->closeFileReply.GetOpenIdentifier();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_UnlinkFileRequest methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileRequest Default Constructor
///
/// \brief  Initializes an STFSMessage_UnlinkFileRequest to defaultvalues
///
///  Sets up the initial UnlinkFileRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_UnlinkFileRequest::STFSMessage_UnlinkFileRequest()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileRequest Constructor
///
/// \brief  Initializes an STFSMessage_UnlinkFileRequest to with
///         supplied template.
///
///  Sets up the initial UnlinkFileRequest structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pp_OpenId       Pointer to OpenId
/// \param       pp_msgBufPtr    Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen       Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_UnlinkFileRequest::STFSMessage_UnlinkFileRequest(const char *pp_fileName,
                                                             int         pv_requesterNodeId,
                                                             int         pv_requesterPID, 
                                                             void       *pp_MsgBufPtr, 
                                                             size_t      pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_UnlinkFile, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->unlinkFileReq.Init (pp_fileName,
                           pv_requesterNodeId,
                           pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_UnlinkFile) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileRequest Destructor
///
/// \brief  Destructor for Create File Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_UnlinkFileRequest::~STFSMessage_UnlinkFileRequest() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->unlinkFileReq.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileRequest::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File request buffer
///
///  This method returns the minimum size for a CreateFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_UnlinkFileRequest::GetMessageBufMinSize (void) {

  // we don't have any variable section for a Unlink file request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_UnlinkFile));
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileRequest::FileName
///
/// \brief  Gets the file name
///
/// \retval char * 
///
//////////////////////////////////////////////////////////////////////////////
char *
STFSMessage_UnlinkFileRequest::GetFileName ( void ) {

  return buf_->unlinkFileReq.GetFileName();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_UnlinkFileReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileReply Default Constructor
///
/// \brief  Initializes an STFSMessage_UnlinkFileReply to defaultvalues
///
///  Sets up the initial UnlinkFileReply structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_UnlinkFileReply::STFSMessage_UnlinkFileReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileReply Constructor
///
/// \brief  Initializes an STFSMessage_UnlinkFileReply to with
///         supplied template.
///
///  Sets up the initial UnlinkFileReply structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pp_OpenId       Pointer to OpenId
/// \param       pp_msgBufPtr    Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen       Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_UnlinkFileReply::STFSMessage_UnlinkFileReply(const char *pp_fileName,
                                                         int   pv_requesterNodeId,
                                                         int   pv_requesterPID, 
                                                         void *pp_MsgBufPtr, 
                                                         size_t pv_bufLen
                                                         )
  : STFS_Message( STFS::MT_UnlinkFile, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->unlinkFileReply.Init (pp_fileName,
                              pv_requesterNodeId,
                              pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_UnlinkFile) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileReply Destructor
///
/// \brief  Destructor for Create File Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_UnlinkFileReply::~STFSMessage_UnlinkFileReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->unlinkFileReply.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File request buffer
///
///  This method returns the minimum size for a CreateFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_UnlinkFileReply::GetMessageBufMinSize (void) {

  // we don't have any variable section for a Unlink file reply
  return (STFS::GetSizeOfFixedMessageBuffer(MT_UnlinkFile));
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_UnlinkFileReply::GetFileName
///
/// \brief  Gets the file name
///
/// \retval char *  
///
//////////////////////////////////////////////////////////////////////////////
char*
STFSMessage_UnlinkFileReply::GetFileName ( void ) {

  return buf_->unlinkFileReply.GetFileName();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_OpenersRequest methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////
//TODO: Update Comments
///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersRequest Default Constructor
///
/// \brief  Initializes an STFSMessage_OpenersRequest to defaultvalues
///
///  Sets up the initial OpenersRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_OpenersRequest::STFSMessage_OpenersRequest()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersRequest Constructor
///
/// \brief  Initializes an STFSMessage_OpenersRequest to with
///         supplied template.
///
///  Sets up the initial OpenersRequest structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pv_Nid             Node ID pointer
/// \param       pp_Path            Pointer to Path Name
/// \param       pp_OpenersSet      Pointer to structure STFS_OpenersSet
/// \param       pp_requesterNodeId Node ID of the requester
/// \param       pp_requesterPID    Process ID of the requester
/// \param       pp_msgBufPtr       Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen          Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_OpenersRequest::STFSMessage_OpenersRequest(stfs_nodeid_t           pv_Nid,
                                                       char                   *pp_Path,
                                                       struct STFS_OpenersSet *pp_OpenersSet,
                                                       int                     pv_requesterNodeId,
                                                       int                     pv_requesterPID, 
                                                       void                   *pp_MsgBufPtr, 
                                                       size_t                  pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_Openers, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->openersReq.Init (pv_Nid,
                         pp_Path,
                         pp_OpenersSet,
                         pv_requesterNodeId,
                         pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_Openers) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersRequest Destructor
///
/// \brief  Destructor for Openers Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_OpenersRequest::~STFSMessage_OpenersRequest() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->openersReq.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersRequest::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Openers request buffer
///
///  This method returns the minimum size for a OpenersRequest buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_OpenersRequest::GetMessageBufMinSize (void) {

  // we don't have any variable section for a Openers request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_Openers));
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersRequest::GetNid
///
/// \brief  Gets the Node ID passed in from caller
///
/// \retval stfs_nodeid_t
///
//////////////////////////////////////////////////////////////////////////////
stfs_nodeid_t
STFSMessage_OpenersRequest::GetNid( void ) {

  return buf_->openersReq.GetNid();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersRequest::GetPath
///
/// \brief  Gets the path and filename
///
/// \retval char * 
///
//////////////////////////////////////////////////////////////////////////////
char *
STFSMessage_OpenersRequest::GetPath( void ) {

  return buf_->openersReq.GetPath();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersRequest::GetOpenersSet
///
/// \brief  Gets the Set of openers
///
/// \retval STFS_OpenersSet *
///
//////////////////////////////////////////////////////////////////////////////
STFS_OpenersSet 
STFSMessage_OpenersRequest::GetOpenersSet( void ) {

  return buf_->openersReq.GetOpenersSet();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_OpenersReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersReply Default Constructor
///
/// \brief  Initializes an STFSMessage_OpenersReply to defaultvalues
///
///  Sets up the initial OpenersReply structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_OpenersReply::STFSMessage_OpenersReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersReply Constructor
///
/// \brief  Initializes an STFSMessage_OpenersReply to with
///         supplied template.
///
///  Sets up the initial OpenersReply structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pv_Nid             Node ID 
/// \param       pp_Path            Pointer to Path Name
/// \param       pp_OpenersSet      Pointer to structure STFS_OpenersSet
/// \param       pp_requesterNodeId Node ID of the requester
/// \param       pp_requesterPID    Process ID of the requester
/// \param       pp_msgBufPtr    Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen       Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_OpenersReply::STFSMessage_OpenersReply(stfs_nodeid_t           pv_Nid,
                                                   char                   *pp_Path,
                                                   struct STFS_OpenersSet *pp_OpenersSet,
                                                   int                     pv_requesterNodeId,
                                                   int                     pv_requesterPID, 
                                                   void                   *pp_MsgBufPtr, 
                                                   size_t                  pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_Openers, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->openersReply.Init (pv_Nid,
                           pp_Path,
                           pp_OpenersSet,
                           pv_requesterNodeId,
                           pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_Openers) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersReply Destructor
///
/// \brief  Destructor for Openers Reply
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_OpenersReply::~STFSMessage_OpenersReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->openersReply.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Openers Reply buffer
///
///  This method returns the minimum size for a OpenersReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_OpenersReply::GetMessageBufMinSize (void) {

  // we don't have any variable section for a Openers reply
  return (STFS::GetSizeOfFixedMessageBuffer(MT_Openers));
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersReply::GetNid
///
/// \brief  Gets the Node ID passed in from caller
///
/// \retval stfs_nodeid_t
///
//////////////////////////////////////////////////////////////////////////////
stfs_nodeid_t
STFSMessage_OpenersReply::GetNid( void ) {

  return buf_->openersReply.GetNid();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersReply::GetPath
///
/// \brief  Gets the path and filename
///
/// \retval char * 
///
//////////////////////////////////////////////////////////////////////////////
char *
STFSMessage_OpenersReply::GetPath( void ) {

  return buf_->openersReply.GetPath();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_OpenersReply::GetOpenersSet
///
/// \brief  Gets the Set of openers
///
/// \retval STFS_OpenersSet *
///
//////////////////////////////////////////////////////////////////////////////
STFS_OpenersSet 
STFSMessage_OpenersReply::GetOpenersSet( void ) {

  return buf_->openersReply.GetOpenersSet();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_FOpenersRequest methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersRequest Default Constructor
///
/// \brief  Initializes an STFSMessage_FOpenersRequest to defaultvalues
///
///  Sets up the initial FOpenersRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_FOpenersRequest::STFSMessage_FOpenersRequest()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersRequest Constructor
///
/// \brief  Initializes an STFSMessage_FOpenersRequest to with
///         supplied template.
///
///  Sets up the initial FOpenersRequest structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pv_Fhandle          fhandle of file to obtain information for
/// \param       pp_FOpenersSet      Pointer to struct STFS_OpenersSet
/// \param       pp_requesterNodeId Node ID of the requester
/// \param       pp_requesterPID    Process ID of the requester
/// \param       pp_msgBufPtr        Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen           Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_FOpenersRequest::STFSMessage_FOpenersRequest(stfs_fhndl_t            pv_Fhandle, 
                                                         struct STFS_OpenersSet *pp_FOpenersSet,
                                                         int                     pv_requesterNodeId,
                                                         int                     pv_requesterPID, 
                                                         void                   *pp_MsgBufPtr, 
                                                         size_t                  pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_FOpeners, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->fopenersReq.Init (pv_Fhandle,
                          pp_FOpenersSet,
                          pv_requesterNodeId,
                          pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_FOpeners) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersRequest Destructor
///
/// \brief  Destructor for FOpeners Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_FOpenersRequest::~STFSMessage_FOpenersRequest() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->fopenersReq.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersRequest::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a FOpeners request buffer
///
///  This method returns the minimum size for a FOpeners request buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_FOpenersRequest::GetMessageBufMinSize (void) {

  // we don't have any variable section for a FOpeners request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_FOpeners));
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersRequest::GetFhandle
///
/// \brief  Gets the Fhandle passed in from caller
///
/// \retval stfs_fhndl_t
///
//////////////////////////////////////////////////////////////////////////////
stfs_fhndl_t
STFSMessage_FOpenersRequest::GetFhandle( void ) {

  return buf_->fopenersReq.GetFhandle();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersRequest::GetPath
///
/// \brief  Gets the path and filename
///
/// \retval char * 
///
//////////////////////////////////////////////////////////////////////////////
char *
STFSMessage_FOpenersRequest::GetPath( void ) {

  return buf_->fopenersReq.GetPath();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersRequest::GetFOpenersSet
///
/// \brief  Gets the Set of fopeners
///
/// \retval STFS_FOpenersSet *
///
//////////////////////////////////////////////////////////////////////////////
STFS_OpenersSet 
STFSMessage_FOpenersRequest::GetFOpenersSet( void ) {

  return buf_->fopenersReq.GetFOpenersSet();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_FOpenersReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersReply Default Constructor
///
/// \brief  Initializes an STFSMessage_FOpenersReply to defaultvalues
///
///  Sets up the initial FOpenersReply structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_FOpenersReply::STFSMessage_FOpenersReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersReply Constructor
///
/// \brief  Initializes an STFSMessage_FOpenersReply to with
///         supplied template.
///
///  Sets up the initial FOpenersReply structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pv_Fhandle         fhandle of file to obtain information for
/// \param       pp_FOpenersSet     Pointer to struct STFS_OpenersSet
/// \param       pp_requesterNodeId Node ID of the requester
/// \param       pp_requesterPID    Process ID of the requester
/// \param       pp_msgBufPtr       Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen          Buffer Length
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_FOpenersReply::STFSMessage_FOpenersReply(stfs_fhndl_t           pv_Fhandle,
                                                   struct STFS_OpenersSet  *pp_FOpenersSet,
                                                   int                      pv_requesterNodeId,
                                                   int                      pv_requesterPID, 
                                                   void                    *pp_MsgBufPtr, 
                                                   size_t                   pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_FOpeners, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->fopenersReply.Init (pv_Fhandle,
                            pp_FOpenersSet,
                            pv_requesterNodeId,
                            pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_FOpeners) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersReply Destructor
///
/// \brief  Destructor for FOpeners Reply
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_FOpenersReply::~STFSMessage_FOpenersReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->fopenersReply.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a FOpeners Reply buffer
///
///  This method returns the minimum size for a FOpenersReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_FOpenersReply::GetMessageBufMinSize (void) {

  // we don't have any variable section for a FOpeners Reply  
  return (STFS::GetSizeOfFixedMessageBuffer(MT_FOpeners));
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersRequest::GetFhandle
///
/// \brief  Gets the Fhandle passed in from caller
///
/// \retval stfs_fhndl_t
///
//////////////////////////////////////////////////////////////////////////////
stfs_fhndl_t
STFSMessage_FOpenersReply::GetFhandle( void ) {

  return buf_->fopenersReply.GetFhandle();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersReply::GetPath
///
/// \brief  Gets the path and filename
///
/// \retval char * 
///
//////////////////////////////////////////////////////////////////////////////
char *
STFSMessage_FOpenersReply::GetPath( void ) {

  return buf_->fopenersReply.GetPath();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_FOpenersReply::GetFOpenersSet
///
/// \brief  Gets the Set of fopeners
///
/// \retval STFS_FOpenersSet *
///
//////////////////////////////////////////////////////////////////////////////
STFS_OpenersSet  
STFSMessage_FOpenersReply::GetFOpenersSet( void ) {

  return buf_->fopenersReply.GetFOpenersSet();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_StatRequest methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatRequest Default Constructor
///
/// \brief  Initializes an STFSMessage_StatRequest to defaultvalues
///
///  Sets up the initial StatRequest structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_StatRequest::STFSMessage_StatRequest()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatRequest Constructor
///
/// \brief  Initializes an STFSMessage_StatRequest to with
///         supplied template.
///
///  Sets up the initial StatRequest structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pv_Nid             Node ID pointer
/// \param       pp_Path            Pointer to Path Name
/// \param       pp_StatSet         Pointer to structure STFS_StatSet
/// \param       pv_Mask            Mask for stat method
/// \param       pp_requesterNodeId Node ID of the requester
/// \param       pp_requesterPID    Process ID of the requester
/// \param       pp_msgBufPtr       Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen          Buffer Length
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_StatRequest::STFSMessage_StatRequest(stfs_nodeid_t           pv_Nid,
                                                 char                   *pp_Path,
                                                 struct STFS_StatSet    *pp_StatSet,
                                                 stfs_statmask_t         pv_Mask, 
                                                 int                     pv_requesterNodeId,
                                                 int                     pv_requesterPID, 
                                                 void                   *pp_MsgBufPtr, 
                                                 size_t                  pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_Stat, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->statReq.Init (pv_Nid,
                      pp_Path,
                      pp_StatSet,
                      pv_Mask,
                      pv_requesterNodeId,
                      pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_Stat) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatRequest Destructor
///
/// \brief  Destructor for Create File Request
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_StatRequest::~STFSMessage_StatRequest() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->statReq.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatRequest::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Create File request buffer
///
///  This method returns the minimum size for a CreateFileReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_StatRequest::GetMessageBufMinSize (void) {

  // we don't have any variable section for a Stat request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_Stat));
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatRequest::GetNid
///
/// \brief  Gets the Node ID passed in from caller
///
/// \retval stfs_nodeid_t
///
//////////////////////////////////////////////////////////////////////////////
stfs_nodeid_t
STFSMessage_StatRequest::GetNid( void ) {

  return buf_->statReq.GetNid();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatRequest::GetPath
///
/// \brief  Gets the path and filename
///
/// \retval char * 
///
//////////////////////////////////////////////////////////////////////////////
char *
STFSMessage_StatRequest::GetPath( void ) {

  return buf_->statReq.GetPath();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatRequest::GetStatSet
///
/// \brief  Gets the Set of stat
///
/// \retval STFS_StatSet 
///
//////////////////////////////////////////////////////////////////////////////
STFS_StatSet 
STFSMessage_StatRequest::GetStatSet( void ) {

  return buf_->statReq.GetStatSet();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatRequest::GetMask
///
/// \brief  Gets the Mask of stat
///
/// \retval stfs_statmask_t *
///
//////////////////////////////////////////////////////////////////////////////
stfs_statmask_t
STFSMessage_StatRequest::GetMask( void ) {

  return buf_->statReq.GetMask();

}

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_StatReply methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatReply Default Constructor
///
/// \brief  Initializes an STFSMessage_StatReply to defaultvalues
///
///  Sets up the initial StatReply structure, based on specified
///  message types.  Buffers are not allocated here.
///
/// \param     void
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////

STFSMessage_StatReply::STFSMessage_StatReply()
  : STFS_Message(MT_Invalid, MPT_Invalid, NULL, 0) {

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatReply Constructor
///
/// \brief  Initializes an STFSMessage_StatReply to with
///         supplied template.
///
///  Sets up the initial StatReply structure, using the
///  supplied template.  After the constructor, the message is ready
///  to set the inUse flag and to send.
///
/// \param       pv_Nid             Node ID 
/// \param       pp_Path            Pointer to Path Name
/// \param       pp_OpenersSet      Pointer to structure STFS_OpenersSet
/// \param       pv_Mask            Mask
/// \param       pp_requesterNodeId Node ID of the requester
/// \param       pp_requesterPID    Process ID of the requester
/// \param       pp_msgBufPtr       Pointer to msgbug allocated/managed by caller
/// \param       pp_bufLen          Pointer to msgbug allocated/managed by caller
///
/// \retval    void
///
///////////////////////////////////////////////////////////////////////////////
STFSMessage_StatReply::STFSMessage_StatReply(stfs_nodeid_t           pv_Nid,
                                             char                   *pp_Path,
                                             struct STFS_StatSet    *pp_StatSet,
                                             stfs_statmask_t         pv_Mask, 
                                             int                     pv_requesterNodeId,
                                             int                     pv_requesterPID, 
                                             void                   *pp_MsgBufPtr, 
                                             size_t                  pv_bufLen
                                                           )
  : STFS_Message( STFS::MT_Stat, STFS::MPT_LibToDaemon,
                  pp_MsgBufPtr, pv_bufLen)
{

  if (pp_MsgBufPtr == NULL) {
    // what are we initializing here??
    ASSERT (false);
    STFS_SigIll();
  }

  if (pv_bufLen < GetMessageBufMinSize() ) {
    // message buf is too small?  Caller parameter error!
    ASSERT (false);
    STFS_SigIll();
  }
  buf_->statReply.Init (pv_Nid,
                        pp_Path,
                        pp_StatSet,
                        pv_Mask,
                        pv_requesterNodeId,
                        pv_requesterPID); 

  SetMessageFixedSize (STFS::GetSizeOfFixedMessageBuffer(MT_Stat) );
  SetMessageBufVariableSize (0);
                                                 
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatReply Destructor
///
/// \brief  Destructor for Stat Reply 
///
/// \param    void
///
/// \retval   void
///
////////////////////////////////////////////////////////////////////////////////
STFSMessage_StatReply::~STFSMessage_StatReply() {

  // Nothing to do here at the moment.  User is responsible for allocating the
  // buffer, so they need to deallocate it too.

  buf_->statReply.Release();


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatReply::GetMessageBufMinSize
///
/// \brief  Gets the minimum size for a Stat Reply buffer
///
///  This method returns the minimum size for a StatReply buffer,
///  including both fixed and variable areas.  The buffer in which this message
///  is constructed must be at least this size.
///
//////////////////////////////////////////////////////////////////////////////
size_t
STFSMessage_StatReply::GetMessageBufMinSize (void) {

  // we don't have any variable section for a Stat request
  return (STFS::GetSizeOfFixedMessageBuffer(MT_Stat));
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatReply::GetNid
///
/// \brief  Gets the Node ID passed in from caller
///
/// \retval stfs_nodeid_t
///
//////////////////////////////////////////////////////////////////////////////
stfs_nodeid_t
STFSMessage_StatReply::GetNid( void ) {

  return buf_->statReply.GetNid();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatReply::GetPath
///
/// \brief  Gets the path and filename
///
/// \retval char * 
///
//////////////////////////////////////////////////////////////////////////////
char *
STFSMessage_StatReply::GetPath( void ) {

  return buf_->statReply.GetPath();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatReply::GetStatSet
///
/// \brief  Gets the Set of stat
///
/// \retval STFS_StatSet *
///
//////////////////////////////////////////////////////////////////////////////
STFS_StatSet 
STFSMessage_StatReply::GetStatSet( void ) {

  return buf_->statReply.GetStatSet();

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSMessage_StatReply::GetMask
///
/// \brief  Gets the Mask of stat
///
/// \retval stfs_statmask_t *
///
//////////////////////////////////////////////////////////////////////////////
stfs_statmask_t
STFSMessage_StatReply::GetMask( void ) {

  return buf_->statReq.GetMask();

}

