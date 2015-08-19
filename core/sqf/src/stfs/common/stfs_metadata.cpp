///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_metadata.cpp
/// \brief   Implements metadata specific classes
///   
///  This file implements the following classes:
///    STFS_ExternalFileMetadata
///    STFS_FragmentFileMetadata
///    STFS_ExternalFileMetadataContainer
///    STFS_ExternalFileOpenerContainer
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

// for stat call.  Move to ext2/fs later!
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "stfs_metadata.h"
#include "stfs_sigill.h"

using namespace STFS;

STFS_ExternalFileMetadataContainer *STFS_ExternalFileMetadataContainer::EfmContainer_ = 0;
STFS_ExternalFileOpenerContainer   *STFS_ExternalFileOpenerContainer::EfoContainer_   = 0;

///////////////////////////////////////////////////////////////////////////////
//
//STFS_ExternalFileMetadata methods 
//
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileMetadata::STFS_ExternalFileMetadata():
  STFS_Root(STFS_Root::EC_ExternalFileMetadata),
  usageCount_(0),
  nodeId_(-1),
  fileAvailable_(true),
  controllingOpenEFH_(0)
{
  externalFileName_[0] = 0;
}

STFS_ExternalFileMetadata::STFS_ExternalFileMetadata(char *pp_ExternalFilename,
						     short pv_NodeId):
  STFS_Root(STFS_Root::EC_ExternalFileMetadata),
  usageCount_(0),
  nodeId_(pv_NodeId),
  fileAvailable_(true),
  controllingOpenEFH_(0)
{
  // Rev: Use a ASSERT/PermAssert()
  ASSERT(strlen(pp_ExternalFilename) < sizeof(externalFileName_));
  strcpy(externalFileName_, pp_ExternalFilename);
}

//dtor
STFS_ExternalFileMetadata::~STFS_ExternalFileMetadata()
{
  const char       *WHERE = "STFS_ExternalFileMetadata::~STFS_ExternalFileMetadata";
  STFS_ScopeTrace   lv_st(WHERE,2);

  //Rev: check for Usage count when called without the file getting closed
  //Set some variable to reflect that a 'close' is being processed
  if (usageCount_ > 0) {
    TRACE_PRINTF4(1, "%s: Being called for %s even for usage count: %d\n",
		  WHERE,
		  externalFileName_,
		  usageCount_);
  }

  if (controllingOpenEFH_) {
    delete controllingOpenEFH_;
    controllingOpenEFH_ = 0;
  }

  bool lv_Done = false;
  ffmVector_Def::iterator it;
  it = ffmVector_.begin();
  while (!lv_Done) {
    if (it == ffmVector_.end()) {
      lv_Done = true;
      continue;
    }
    // the erase() returns the next item 
    STFS_FragmentFileMetadata *lp_Ffm = *it;
    delete lp_Ffm;
    it = ffmVector_.erase(it);
  }
  externalFileName_[0] = 0;

}

bool
STFS_ExternalFileMetadata::IsEyeCatcherValid()
{
  const char       *WHERE = "STFS_ExternalFileMetadata::IsEyeCatcherValid";
  STFS_ScopeTrace   lv_st(WHERE);

  return STFS_Root::IsEyeCatcherValid(STFS_Root::EC_ExternalFileMetadata);
}

///////////////////////////////////////////////////////////////////////////////
///
//          Open
///
/// \brief  Opens the EFM, Creates EFH, Increments Usage Count
///
/// \param pv_IsDaemon: Whether called by the daemon
/// 
/// \retval STFS_ExternalFileHandle*    SUCCESS: Valid Pointer \n
///                                     FAILURE: NULL pointer
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileHandle*
STFS_ExternalFileMetadata::Open(bool pv_IsDaemon)
{
  const char       *WHERE = "STFS_ExternalFileMetadata::Open";
  STFS_ScopeTrace   lv_st(WHERE);

  STFS_ExternalFileHandle *lp_Efh = 0;

  if (pv_IsDaemon && EFHGet()) {
    lp_Efh = EFHGet();
  }
  else {
    lp_Efh = new STFS_ExternalFileHandle(this);
    if (!lp_Efh) {
      return NULL;
    }

    if (pv_IsDaemon) {
      this->EFHSet(lp_Efh);
    }
    else {
      
      //insert EFH in the container
      if (STFS_ExternalFileHandle::InsertIntoContainer(lp_Efh) < 0) {
	return NULL;
      }
    }
  }

  IncrementUsageCount();

  return lp_Efh;
}

///////////////////////////////////////////////////////////////////////////////
///
//          InsertIntoContainer
///
/// \brief  Inserts the EFM* into the container
///
/// \param STFS_ExternalFileMetadata * : Pointer to the EFM
/// 
/// \retval int    SUCCESS: 0 \n
///                FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_ExternalFileMetadata::InsertIntoContainer(STFS_ExternalFileMetadata *pp_Efm)
{
  const char       *WHERE = "STFS_ExternalFileMetadata::InsertIntoContainer";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_RetCode = 0;

  if (!pp_Efm) {
    // TBD Error handling
    return -1;
  }

  STFS_ExternalFileMetadataContainer *lp_EfmContainer = STFS_ExternalFileMetadata::GetContainer();
  if (!lp_EfmContainer) {
    //TBD Error handling  
    TRACE_PRINTF2(1, "%s\n", "Null EFM Container");
    return -1;
  }
      
  char *lp_ExternalFileName = pp_Efm->ExternalFileNameGet();
  char *lp_Key = new char[strlen(lp_ExternalFileName) + 1];
  if (!lp_Key) {
    // TBD Error handling
    TRACE_PRINTF2(1, "%s\n", "Error allocating space for Key");
    return -1;
  }
  strcpy(lp_Key, lp_ExternalFileName);

  lv_RetCode = lp_EfmContainer->Insert(lp_Key, pp_Efm);
  if (lv_RetCode < 0) {
    // TBD Error handling
    return -1;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          DeleteFromContainer
///
/// \brief  Deletes the EFM* from the container
///
/// \param STFS_ExternalFileMetadata * : Pointer to the EFM
/// 
/// \retval int    SUCCESS: 0        \n
///                FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_ExternalFileMetadata::DeleteFromContainer(STFS_ExternalFileMetadata *&pp_Efm)
{
  const char       *WHERE = "STFS_ExternalFileMetadata::DeleteFromContainer";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_RetCode = 0;

  if (!pp_Efm) {
    // TBD Error handling
    return -1;
  }

  if (pp_Efm->UsageCountGet() > 0) {
    return 0;
  }

  STFS_ExternalFileMetadataContainer *lp_EfmContainer = STFS_ExternalFileMetadata::GetContainer();
  if (!lp_EfmContainer) {
    TRACE_PRINTF3(1, "%s: %s\n", WHERE, "Null EFM Container");
    return -1;
  }
      
  lv_RetCode = lp_EfmContainer->Delete(pp_Efm->ExternalFileNameGet());
  if (lv_RetCode < 0) {
    // TBD Error handling
    return -1;
  }

  delete pp_Efm;
  pp_Efm = 0;

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetFromContainer
///
/// \brief  Gets the EFM* from the container
///
/// \param char *pp_Key: External File Name of the EFM to be searched for
/// 
/// \retval STFS_ExternalFileMetadata*    SUCCESS: Valid Pointer \n
///                                       FAILURE: NULL pointer
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileMetadata*
STFS_ExternalFileMetadata::GetFromContainer(char *pp_Key)
{
  const char       *WHERE = "STFS_ExternalFileMetadata::GetFromContainer";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!pp_Key || !pp_Key[0]) {
    return NULL;
  }

  STFS_ExternalFileMetadataContainer *lp_EfmContainer = STFS_ExternalFileMetadata::GetContainer();
  if (!lp_EfmContainer) {
    TRACE_PRINTF3(1, "%s: %s\n", WHERE, "Null EFM Container");
    return NULL;
  }

  STFS_ExternalFileMetadata *lp_Efm = lp_EfmContainer->Get(pp_Key);
  if (!lp_Efm) {
    return NULL;
  }

  if (!lp_Efm->IsEyeCatcherValid()) {
    STFS_util::SoftwareFailureHandler(WHERE);
    return NULL;
  }

  return lp_Efm;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Close
///
/// \brief  Closes the EFM 
///
/// \param pv_IsDaemon: Whether called by the daemon
/// 
/// \retval int    SUCCESS: 0             \n
///                FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_ExternalFileMetadata::Close(bool pv_IsDaemon)
{
  const char       *WHERE = "STFS_ExternalFileMetadata::Close";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_RetCode = 0;

  DecrementUsageCount();

  if (UsageCountGet() <= 0) {
    if (pv_IsDaemon) {

      if (controllingOpenEFH_) {
	lv_RetCode = controllingOpenEFH_->Close();
	if (lv_RetCode < 0) {
	  // TBD Error handling
	  return -1;
	}
      }

      lv_RetCode = Unlink();
      if (lv_RetCode < 0) {
	// TBD Error handling
	return -1;
      }
    }
  }

  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Unlink
///
/// \brief  Performs Unlink operation on the EFM
///
/// \retval int    SUCCESS: 0             \n
///                FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_ExternalFileMetadata::Unlink()
{
  const char       *WHERE = "STFS_ExternalFileMetadata::Unlink";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_RetCode = 0;

  // File is NOT available for a new open any longer
  // (Current handles would continue to work (if any))
  fileAvailable_ = false;

  if (usageCount_ > 0) {
    return lv_RetCode;
  }


  if (controllingOpenEFH_) {

    lv_RetCode = controllingOpenEFH_->Unlink();
    if (lv_RetCode < 0) {
      // TBD Error handling
      return -1;
    }
  }
  
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileMetadata::FileIsLocallyOwned()
///
/// \brief  Determines whether this file is owned on the current node.
///
///         This method compares the current node to the node referenced in the
///         file name.
///
/// \retval TRUE:           If the current node is the node in the file name.
///         FALSE:          The file's node is different than the current node
///                         or the filename is not yet initialized.
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileMetadata::FileIsLocallyOwned(void) {

  int lv_myNode = STFS_util::GetMyNodeId();

  bool lv_retVal = ( ( nodeId_ != -1) && (nodeId_ = lv_myNode) );

  return lv_retVal;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileMetadata::Pack()
///
/// \brief  Creates a buffer containing the EFM fixed for shipping to other
///         processes 
///
///         This method takes a buffer and size as input parameters and attempts
///         to package the EFM itself for transport.  Packing an EFM means
///         compacting the file name and copying other information into to the
///         buffer. 
///
///         The packed EFM does not include the FFMs associated with it; those
///         must be packed separately.  It also doesn't include the EFH; those
///         must be re-materialized in the other process.
///         
///         After a successful pack, pv_packedLen contains the actual size in
///         bytes of the packed buffer, including any internal padding bytes
///         used to force variable alignment.
///
/// \retval TRUE:           If the pack succeeded.
///         FALSE:          The pack did no succeed. The contents of pp_buf are
///                         unknown. 
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileMetadata::Pack (char *pp_buf, 
				 size_t pv_bufLen, 
				 size_t *pv_packedLen) {

  ////////////////////////////////
  ///  Validate params, EFM
  ////////////////////////////////

  if (pp_buf == NULL || pv_bufLen == 0) {
    // why call without a buffer?
    ASSERT (false);
    return false;
  }

  if (pv_packedLen == NULL) {
    // no place to put the packed length!
    ASSERT (false);
    return false;
  }

  if (!IsEyeCatcherValid() ) {
    ASSERT (false);
    STFS_SigIll();
    return false;
  }

  ////////////////////////////////
  ///  Start Copy Process
  ////////////////////////////////

  char *lp_copyPtr = pp_buf;
  size_t lv_currSpaceUsed = 0;

  ////////////////////////////////
  ///  Pack the Root Entities
  ////////////////////////////////
  size_t lv_rootBytes = STFS_Root::Pack(lp_copyPtr, (pv_bufLen - lv_currSpaceUsed) );
  if  (lv_rootBytes > 0 ) {

    lp_copyPtr += lv_rootBytes;
    lv_currSpaceUsed += lv_rootBytes;
  }
  else {
    //eye catchers weren't copied in!
    ASSERT (false);
    return false;
  }

  ////////////////////////////////
  ///  Leave space for file name
  ////////////////////////////////

  // we can't actually pack the file name 'til we know where it's going to go.
  // So we skip over the offset field for it.

  char *lp_fileNameOffsetPtr = NULL;

  if (lv_currSpaceUsed + sizeof (varoffset_t) < pv_bufLen) {
    lp_fileNameOffsetPtr = lp_copyPtr;
    lv_currSpaceUsed += sizeof (varoffset_t);
    lp_copyPtr += sizeof (varoffset_t);
  }
  else {
    return false;
  }

  ///////////////////////////////
  /// Set usage count
  ///////////////////////////////

  //  Usage count is really associated with the local process.  So we set it to
  //  0 for transport.  Theoretically, we could leave it out, but someday we
  //  might want to send it, so here it is....

  if (lv_currSpaceUsed + sizeof (usageCount_) < pv_bufLen ) {
    *lp_copyPtr = (short) 0;
    lv_currSpaceUsed += sizeof (usageCount_);
    lp_copyPtr += sizeof (usageCount_);
  }
  else {
    return false;
  }

  ///////////////////////////////
  ///  Copy in nodeID_
  ///////////////////////////////

  if (lv_currSpaceUsed + sizeof (nodeId_) < pv_bufLen) {

    memcpy (lp_copyPtr, &nodeId_, sizeof (nodeId_));
    lv_currSpaceUsed += sizeof (nodeId_);
    lp_copyPtr += sizeof (nodeId_);
  }
  else {
    return false;
  }


  // we don't copy in the FFMs right now.

  ///////////////////////////////
  ///  Copy in File Available
  ///////////////////////////////

  if (lv_currSpaceUsed + sizeof (fileAvailable_) < pv_bufLen) {
    memcpy (lp_copyPtr, &fileAvailable_, sizeof (fileAvailable_));    
    lv_currSpaceUsed += sizeof (fileAvailable_);
    lp_copyPtr += sizeof (fileAvailable_);
  }
  else {
    return false;
  }

  // we haven't gotten lost in the buffer, have we??
  ASSERT ((int) lv_currSpaceUsed == lp_copyPtr - pp_buf) ;

  ////////////////////////////////
  /// Compress the filename
  ////////////////////////////////


  /// Now we know how much space the fixed portion of the message occupies.  We
  /// can set up the filename, which is the only variable part of an EFM.

  // We don't know whether we've got the whole buffer or a subset of it.  So the
  // file name offset is set relative to the start of the EFM, not the start of
  // the buffer.


  memcpy (lp_fileNameOffsetPtr,&lv_currSpaceUsed, sizeof (lv_currSpaceUsed));

  size_t lv_filenameSpaceAvail = pv_bufLen - lv_currSpaceUsed;
    
  size_t lv_fileNameCompressedSize 
         = STFS_util::CompressString (externalFileName_,
				      lp_copyPtr, /* location for name */
				      lv_filenameSpaceAvail );
  
  if (lv_fileNameCompressedSize > 0) {
    lv_currSpaceUsed += lv_fileNameCompressedSize;

    // for safety, we'll bump the copy pointer too.  We're done copying now, but
    // if something gets added later, at least the copy pointer will be correct.
    lp_copyPtr += lv_fileNameCompressedSize;

  }
  else {
    return false;
  }
                                    

  ////////////////////////////////
  ///  Set the total length return param
  ////////////////////////////////

  *pv_packedLen = lv_currSpaceUsed;

  ////////////////////////////////
  /// Finis!
  ////////////////////////////////

  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileMetadata::Unpack()
///
/// \brief Takes a buffer that begins with a packed EFM and unpacks it into the
///        class variables, overwriting any existing values
///
///         This method takes a buffer and size as input parameters and attempts
///         extract an EFM from it.  This method should be used with an EFM
///         created using the default constructor because unpack overwrites
///         everything. 
///
///         This method only extracts an EFM.  It doesn't look for any FFMs.  It
///         does, however, set the num fragments.
///         
///         The buffer is unchanged as a result of an unpack.  The same buffer
///         may be unpacked multiple times.
///
///         Right now, we don't pack any of the STFS_Root stuff (i.e., the eye
///         catcher).  Perhaps we should as a sanity check.
///
/// \retval TRUE:           If the unpack succeeded.
///         FALSE:          The unpack did not succeed. The state of the class is unknown
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileMetadata::Unpack (char *pp_buf, 
				   size_t pv_bufLen) {

  ////////////////////////////////
  ///  Validate params, EFM
  ////////////////////////////////

  if (pp_buf == NULL || pv_bufLen == 0) {
    // why call without a buffer?
    ASSERT (false);
    return false;
  }

  if (!IsEyeCatcherValid() ) {
    ASSERT (false);
    STFS_SigIll();
    return false;
  }

  ////////////////////////////////
  ///  Start Copy Process
  ////////////////////////////////

  char *lp_copyPtr = pp_buf;

  ////////////////////////////////
  ///  Get the root information
  ////////////////////////////////

  size_t lv_rootBytes = STFS_Root::Unpack(lp_copyPtr);

  if ( lv_rootBytes > 0) {
    lp_copyPtr += lv_rootBytes;
  }
  else {
    return false;
  }


  ////////////////////////////////
  ///  Extract the filename
  ////////////////////////////////

  // get the offset for the actual file name

  varoffset_t lv_fileNameOffset = 0;
  memcpy (&lv_fileNameOffset, lp_copyPtr, sizeof (varoffset_t));
  lp_copyPtr += sizeof (varoffset_t);

  STFS_util::UncompressString (pp_buf+lv_fileNameOffset,
			       STFS_ExternalFileMetadata::externalFileName_,
			       STFS_NAME_MAX);


  ///////////////////////////////
  /// Set usage count
  ///////////////////////////////

  //  Usage count is really associated with the local process.  So we set it to
  //  0 for transport.  Theoretically, we could leave it out, but someday we
  //  might want to send it, so here it is....


  usageCount_ = (short) *lp_copyPtr;
  lp_copyPtr += sizeof (usageCount_);

  ///////////////////////////////
  ///  Copy in nodeID_
  ///////////////////////////////

  memcpy (&nodeId_, lp_copyPtr, sizeof (nodeId_));
  lp_copyPtr += sizeof (nodeId_);


  // we don't copy in the FFMs right now.

  ///////////////////////////////
  ///  Copy in File Available
  ///////////////////////////////

  memcpy (&fileAvailable_,lp_copyPtr, sizeof (fileAvailable_));    
  lp_copyPtr += sizeof (fileAvailable_);

  ////////////////////////////////
  /// Finis!
  ////////////////////////////////

  return true;
}



///////////////////////////////////////////////////////////////////////////////
///
//          CreateFFM
///
/// \brief  Creates the FFM with the proper parameters and performs the 
///         necessary linkage.
///
/// \param char *pp_FragmentFilename:   physical file name of the fragment file
/// \param long  pv_FragmentFileOffset: absolute File offset for this FFM
/// 
/// \retval STFS_FragmentFileMetadata*    SUCCESS: Valid Pointer \n
///                                       FAILURE: NULL pointer
///
///////////////////////////////////////////////////////////////////////////////
STFS_FragmentFileMetadata *
STFS_ExternalFileMetadata::CreateFFM(char *pp_FragmentFilename,
				     char *pp_StfsDirectory,
				     long  pv_FragmentFileOffset)
{
  const char       *WHERE = "STFS_ExternalFileMetadata::CreateFFM";
  STFS_ScopeTrace   lv_st(WHERE,2);


  if ((!pp_FragmentFilename) || 
      (!pp_StfsDirectory)) {
    return NULL;
  }

  STFS_FragmentFileMetadata *lp_Ffm = new STFS_FragmentFileMetadata(pp_FragmentFilename,
								    STFS_util::GetMyNodeId(),
								    pp_StfsDirectory,
								    GetNumFragments(),
								    pv_FragmentFileOffset);

  if (!lp_Ffm) {
    return NULL;
  }

  lp_Ffm->EFMSet(this);
  InsertFFM(lp_Ffm);

  return lp_Ffm;
}

///////////////////////////////////////////////////////////////////////////////
///
//          InsertFFM
///
/// \brief  Inserts the FFM in this object's FFM container
///
/// \param STFS_FragmentFileMetadata *pp_Ffm
/// 
/// \retval int    SUCCESS: 0             \n
///                FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileMetadata::InsertFFM(STFS_FragmentFileMetadata *pp_Ffm)
{
  const char       *WHERE = "STFS_ExternalFileMetadata::InsertFFM";
  STFS_ScopeTrace   lv_st(WHERE,2);

  Lock();
  try {
    ffmVector_.push_back(pp_Ffm);
  }
  catch (...) {
    Unlock();
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }
  
  Unlock();
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          CopyAndAppendFFM
///
/// \brief  Takes an FFM and copies it into the current EFM
///
///  Useful for moving FFMs from message EFMs to the official EFM without
///  breaking anything.  Copy constructor alone isn't good enough because we
///  also need to do an insert...
///
/// \param STFS_FragmentFileMetadata *pp_Ffm
/// 
/// \retval int    SUCCESS: 0             \n
///                FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileMetadata::CopyAndAppendFFM(STFS_FragmentFileMetadata *pp_Ffm)
{
  const char       *WHERE = "STFS_ExternalFileMetadata::CopyAndAppendFFM";
  STFS_ScopeTrace   lv_st(WHERE,2);


  // Parameter checking and housekeeping

  if (pp_Ffm == NULL) {
    // no puck, no hockey.
    ASSERT (false);
    return -1;
  }
  STFS_FragmentFileMetadata *lp_Ffm 
    = new STFS_FragmentFileMetadata (pp_Ffm->NameGet(),
				     pp_Ffm->NodeIdGet(),
				     pp_Ffm->StfsDirectoryGet(),
				     pp_Ffm->FragmentNumberGet(),
				     pp_Ffm->OffsetGet() );

  if (lp_Ffm == NULL) {
    // Memory allocation error
    // TBD:  Need to handle better!
    ASSERT (false);
    return -1;
  }


  // Now put it in the real vector

  Lock();
  try {
    ffmVector_.push_back(lp_Ffm);
  }
  catch (...) {
    Unlock();
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }
  
  Unlock();
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetFragment
///
/// \brief  Returns the FFM at the provided index 
///
/// \param int pv_Index: Index(zero based) at which the desired FFM is located
/// 
/// \retval STFS_FragmentFileMetadata*    SUCCESS: Valid Pointer \n
///                                       FAILURE: NULL pointer
///
///////////////////////////////////////////////////////////////////////////////
STFS_FragmentFileMetadata *
STFS_ExternalFileMetadata::GetFragment(int pv_Index) 
{
  const char       *WHERE = "STFS_ExternalFileMetadata::GetFragment";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (pv_Index >= (int) ffmVector_.size()) {
    return (STFS_FragmentFileMetadata *) 0;
  }
  
  return ffmVector_[pv_Index];
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetFullEOF
///
/// \brief  Gets the EOF for an STFS file, including the sizes of all fragments
///
/// \param void
/// 
/// \retval The complete size of the file
///                FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
ssize_t STFS_ExternalFileMetadata::GetFullEOF() {

  // To calculate the EOF, we rely on the initial offset for the last fragment,
  // plus the EOF of that fragment

  ///  find the FFM for the last fragment

  STFS_FragmentFileMetadata *lp_lastFFM = ffmVector_.back();

  struct stat lv_stat;
  int lv_retVal = stat (lp_lastFFM->NameGet(), &lv_stat);
  if (lv_retVal == 0) {
    size_t lv_fragStart = lp_lastFFM->OffsetGet();

    return (lv_fragStart + lv_stat.st_size);
  }
  else{

    return -1;
  }

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileMetadata::GetContainer() 
///
/// \brief  Returns the STFS_ExternalFileMetadataContainer
///
/// \param  void
/// 
/// \retval STFS_ExternalFileMetadataContainer *
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileMetadataContainer*
STFS_ExternalFileMetadata::GetContainer() {
  const char       *WHERE = "STFS_ExternalFileMetadata::GetContainer";
  STFS_ScopeTrace   lv_st(WHERE,2);

  STFS_ExternalFileMetadataContainer *lp_EfmContainer = STFS_ExternalFileMetadataContainer::GetInstance();

  if (!lp_EfmContainer->IsEyeCatcherValid()) {
    STFS_util::SoftwareFailureHandler(WHERE);
    return NULL;
  }

  return lp_EfmContainer;

}

// Rev: Move the implementation of FFM to a diff file
///////////////////////////////////////////////////////////////////////////////
//
// STFS_FragmentFileMetadata methods
//
///////////////////////////////////////////////////////////////////////////////
STFS_FragmentFileMetadata::STFS_FragmentFileMetadata():
  STFS_Root(STFS_Root::EC_FragmentFileMetadata),
  fragmentNumber_(0),
  offset_(0),
  nodeId_(0),
  efm_(0)
{
  const char       *WHERE = "STFS_FragmentFileMetadata::STFS_FragmentFileMetadata - default";
  STFS_ScopeTrace   lv_st(WHERE,2);

  filename_[0] = 0;
}

STFS_FragmentFileMetadata::STFS_FragmentFileMetadata(char *pp_Filename,
						     short pv_NodeId,
						     char *pp_StfsDirectory,
						     int   pv_Number, 
						     int   pv_Offset):
  STFS_Root(STFS_Root::EC_FragmentFileMetadata),
  fragmentNumber_(pv_Number),
  offset_(pv_Offset),
  nodeId_(pv_NodeId),
  efm_(0)
{
  const char       *WHERE = "STFS_FragmentFileMetadata::STFS_FragmentFileMetadata - ctor2";
  STFS_ScopeTrace   lv_st(WHERE,2);

  // Rev: Use Perm Assert methodology
  ASSERT(strlen(pp_Filename) <= STFS_NAME_MAX);
  ASSERT(strlen(pp_StfsDirectory) <= STFS_NAME_MAX);
   
  strcpy(filename_, pp_Filename);
  strcpy(stfsDirectory_, pp_StfsDirectory);
}

STFS_FragmentFileMetadata::~STFS_FragmentFileMetadata()
{
  const char       *WHERE = "STFS_FragmentFileMetadata::~STFS_FragmentFileMetadata";
  STFS_ScopeTrace   lv_st(WHERE,2);

  fragmentNumber_ = 0;
  filename_[0] = 0;
  offset_ = 0;
  nodeId_ = 0;
  efm_ = 0;
}

bool
STFS_FragmentFileMetadata::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid(STFS_Root::EC_FragmentFileMetadata);
}

bool 
STFS_FragmentFileMetadata::IsLocal() const
{
  // Rev: Could think of some optimization here 
  // (store the bool in the class while creation/unpacking)
  int lv_MyNodeId = STFS_util::GetMyNodeId();

  return (lv_MyNodeId == this->nodeId_);
}

void
STFS_FragmentFileMetadata::FragmentNumberSet(int pv_Number)
{
  fragmentNumber_ = pv_Number;
}

void
STFS_FragmentFileMetadata::NameSet(char *pp_Filename)
{
  if (!pp_Filename) {
    return;
  }

  // Rev: Use a Perm Assert here
  ASSERT(strlen(pp_Filename) <= STFS_NAME_MAX);
   
  strcpy(filename_, pp_Filename);
}

void
STFS_FragmentFileMetadata::OffsetSet(long pv_Offset)
{
  // Rev: Some validation here
  // Rev: Add eyecatchers for the setters/getters
  offset_ = pv_Offset;
}

void
STFS_FragmentFileMetadata::NodeIdSet(short pv_NodeId)
{
  nodeId_ = pv_NodeId;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_FragmentFileMetadata::Pack()
///
/// \brief  Creates a buffer containing the FFM fixed for shipping to other
///         processes 
///
///         This method takes a buffer and size as input parameters and attempts
///         to package the FFM itself for transport.  Packing an FFM means
///         compacting the file names and copying other information into to the
///         buffer. 
///
///         After a successful pack, pv_packedLen contains the actual size in
///         bytes of the packed buffer, including any internal padding bytes
///         used to force variable alignment.
///
///         Right now, we don't pack any of the STFS_Root stuff (i.e., the eye
///         catcher).  Perhaps we should as a sanity check.
///
/// \retval TRUE:           If the pack succeeded.
///         FALSE:          The pack did no succeed. The contents of pp_buf are
///                         unknown. 
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_FragmentFileMetadata::Pack (char *pp_buf, 
				 size_t pv_bufLen, 
				 size_t *pv_packedLen) {

  ////////////////////////////////////////////
  /// Validate Parameters
  ////////////////////////////////////////////

  if (pp_buf == NULL || pv_bufLen == 0) {
    // why call without a buffer?
    ASSERT (false);
    return false;
  }

  if (pv_packedLen == NULL) {
    // no place to put the packed length!
    ASSERT (false);
    return false;
  }

  // Rev: Could have a CheckEyeCatcher(Debug) / Perm_EyeCatcher (Release code)
  if (!IsEyeCatcherValid() ) {
    ASSERT (false);
    STFS_SigIll();
    return false;
  }

  ////////////////////////////////////////////
  ///  Initial housekeeping
  ////////////////////////////////////////////

  char *lp_copyPtr = pp_buf;
  size_t lv_currSpaceUsed = 0;

  ////////////////////////////////
  ///  Pack the Root Entities
  ////////////////////////////////
  size_t lv_rootBytes = STFS_Root::Pack(lp_copyPtr, (pv_bufLen - lv_currSpaceUsed) );
  if  (lv_rootBytes > 0 ) {

    lp_copyPtr += lv_rootBytes;
    lv_currSpaceUsed += lv_rootBytes;
  }
  else {
    //eye catchers weren't copied in!
    ASSERT (false);
    return false;
  }

  ////////////////////////////////////////////
  /// Skip over fragment file name
  ////////////////////////////////////////////

  // we can't actually pack the fragment name until we know where it's going to
  // go.  So we leave space for it now.

  char *lp_fragNameOffsetPtr = NULL;

  if (lv_currSpaceUsed + sizeof (varoffset_t) < pv_bufLen){

    lp_fragNameOffsetPtr = lp_copyPtr;
    lv_currSpaceUsed += sizeof (varoffset_t);
    lp_copyPtr += sizeof (varoffset_t);

  }
  else {
    return false;
  }


  ////////////////////////////////////////////
  /// Copy fragment number
  ////////////////////////////////////////////

  if (lv_currSpaceUsed + sizeof (fragmentNumber_) < pv_bufLen) {
    memcpy (lp_copyPtr, &fragmentNumber_, sizeof (fragmentNumber_));
    lv_currSpaceUsed += sizeof (fragmentNumber_);
    lp_copyPtr += sizeof (fragmentNumber_);
  }
  else {
    return false;
  }

  ////////////////////////////////////////////
  /// Copy fragment first offset
  ////////////////////////////////////////////

  if (lv_currSpaceUsed + sizeof (offset_) < pv_bufLen) {
    memcpy (lp_copyPtr, &offset_, sizeof (offset_) );
    lv_currSpaceUsed += sizeof (offset_);
    lp_copyPtr += sizeof (offset_);
  }
  else {
    return false;
  }

  ////////////////////////////////////////////
  /// Copy Node ID
  ////////////////////////////////////////////

  if (lv_currSpaceUsed + sizeof (nodeId_) < pv_bufLen) {

    memcpy (lp_copyPtr, &nodeId_, sizeof (nodeId_));
    lv_currSpaceUsed += sizeof (nodeId_);
    lp_copyPtr += sizeof (nodeId_);
  }
  else {
    return false;
  }

  ////////////////////////////////////////////
  ///  Skip over space for STFS directory
  ////////////////////////////////////////////

  // we can't actually pack the directory now until we know where it's going to
  // go.  So we just leave space for it now.

  char *lp_directoryOffsetPtr = NULL;

  if (lv_currSpaceUsed + sizeof (varoffset_t) < pv_bufLen) {

    lp_directoryOffsetPtr = lp_copyPtr;
    lv_currSpaceUsed +=sizeof (varoffset_t);
    lp_copyPtr += sizeof (varoffset_t);
  }
  else {
    return false;
  }

  ////////////////////////////////////////////
  ///  Compress the fragment file name
  ////////////////////////////////////////////
 
  // first copy the offset into the saved location
  memcpy (lp_fragNameOffsetPtr, &lv_currSpaceUsed, sizeof (varoffset_t));

  size_t lv_fragNameSpaceAvail = pv_bufLen - lv_currSpaceUsed;

  size_t lv_fragCompressedSize = 
       STFS_util::CompressString (filename_, lp_copyPtr, lv_fragNameSpaceAvail);

  if (lv_fragCompressedSize > 0) {
    lv_currSpaceUsed += lv_fragCompressedSize;
    lp_copyPtr += lv_fragCompressedSize;
  }
  else {
    return false;
  }

  ////////////////////////////////////////////
  /// Compress the STFS directory name
  ////////////////////////////////////////////

  // first copy the offset into the saved location
  memcpy (lp_directoryOffsetPtr, &lv_currSpaceUsed, sizeof (varoffset_t));

  // removed it for warning elimination (unused variable)
  size_t lv_directorySpaceAvail = pv_bufLen - lv_currSpaceUsed;

  // Rev: use lv_directorySpaceAvail
  size_t lv_directoryCompressedSize = 
       STFS_util::CompressString (stfsDirectory_, lp_copyPtr, lv_directorySpaceAvail);

  if (lv_directoryCompressedSize > 0) {
    lv_currSpaceUsed += lv_directoryCompressedSize;
    lp_copyPtr += lv_directoryCompressedSize;
  }
  else {
    return false;
  }
   

  ////////////////////////////////////////////
  ///  Get total bytes written
  ////////////////////////////////////////////
  *pv_packedLen = lv_currSpaceUsed;

  ////////////////////////////////////////////
  /// Finis!
  ////////////////////////////////////////////

  return true;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_FragmentFileMetadata::Unpack()
///
/// \brief Takes a buffer that begins with a packed FFM and unpacks it into the
///        class variables, overwriting any existing values
///
///         This method takes a buffer and size as input parameters and attempts
///         extract an FFM from it.  This method should be used with an EFM
///         created using the default constructor because unpack overwrites
///         everything. 
///
///         The buffer is unchanged as a result of an unpack.  The same buffer
///         may be unpacked multiple times.
///
/// \retval TRUE:           If the unpack succeeded.
///         FALSE:          The unpack didn't succeed. The state of the class is unknown
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_FragmentFileMetadata::Unpack (char *pp_buf, 
				   size_t pv_bufLen) {

  ////////////////////////////////
  ///  Validate params, FFM
  ////////////////////////////////

  if (pp_buf == NULL || pv_bufLen == 0) {
    // why call without a buffer?
    ASSERT (false);
    return false;
  }

  if (!IsEyeCatcherValid() ) {
    ASSERT (false);
    STFS_SigIll();
    return false;
  }

  ////////////////////////////////
  /// Housekeeping
  ////////////////////////////////

  char *lp_copyPtr = pp_buf;

  ////////////////////////////////
  ///  Get the root information
  ////////////////////////////////

  size_t lv_rootBytes = STFS_Root::Unpack(lp_copyPtr);

  if ( lv_rootBytes > 0) {
    lp_copyPtr += lv_rootBytes;
  }
  else {
    return false;
  }

  ////////////////////////////////
  /// Extract the frag file name
  ////////////////////////////////

  // get the offset for the actual fragment file name

  varoffset_t lv_fragOffset = 0;
  memcpy ( &lv_fragOffset, lp_copyPtr, sizeof (varoffset_t));

  lp_copyPtr += sizeof (varoffset_t);

  if (STFS_util::UncompressString (pp_buf+lv_fragOffset,
				   STFS_FragmentFileMetadata::filename_,
				   STFS_NAME_MAX) == 0) {
    return false;
  }

  ////////////////////////////////
  /// Extract the fragment number
  ////////////////////////////////

  memcpy (&fragmentNumber_, lp_copyPtr, sizeof (fragmentNumber_) );
  lp_copyPtr += sizeof (fragmentNumber_);

  ////////////////////////////////
  /// Extract the offset
  ////////////////////////////////

  memcpy (&offset_, lp_copyPtr, sizeof (offset_) );
  lp_copyPtr += sizeof (offset_);

  ////////////////////////////////
  /// Extract the nodeID
  ////////////////////////////////
  memcpy (&nodeId_, lp_copyPtr, sizeof (nodeId_) );
  lp_copyPtr += sizeof (nodeId_);

  ////////////////////////////////
  /// Extract the stfs directory
  ////////////////////////////////
  varoffset_t lv_directoryOffset = 0;
  memcpy ( &lv_directoryOffset, lp_copyPtr, sizeof (varoffset_t));

  lp_copyPtr += sizeof (varoffset_t);

  // Rev: Check the returned val
  if (STFS_util::UncompressString (pp_buf+lv_directoryOffset,
				   STFS_FragmentFileMetadata::stfsDirectory_,
				   STFS_NAME_MAX) == 0) {
    return false;
  }


  ////////////////////////////////
  /// Finis!
  ////////////////////////////////
  return true;

}

void
STFS_FragmentFileMetadata::EFMSet(STFS_ExternalFileMetadata *pp_efm)
{
  // Rev: It could set the objects efm_ to null if the parameter is null
  if(!pp_efm) {
    return;
  }
  
  // Rev: Check the eye catcher of pp_efm

  this->efm_ = pp_efm;
}

////////////////////////////////////////////////////
//
// STFS_CharacterPointerComparator implementation
//
////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///
//          STFS_CharacterPointerComparator::operator()
///
/// \brief  To compare two STFS_CharacterPointerComparator objects 
///
/// \param  pp_Lhs
/// \param  pp_Rhs
/// 
/// \retval bool   returns a 'true' when pp_Lhs < pp_Rhs
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_CharacterPointerComparator::operator() (char *pp_Lhs, 
					     char *pp_Rhs) const 
{
  int lv_Ret = strcmp(pp_Lhs, pp_Rhs);
  if (lv_Ret < 0) {
    return true;
  }
  return false;
}

////////////////////////////////////////////////////
//
// STFS_ExternalFileMetadataContainer method implementation
//
////////////////////////////////////////////////////
bool
STFS_ExternalFileMetadataContainer::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid(STFS_Root::EC_ExternalFileMetadataContainer);
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetInstance
///
/// \brief  Returns an instance of STFS_ExternalFileMetadataContainer
///
///         returns an instance of the STFS_ExternalFileMetadataContainer
///         (creates the container if necessary)
///
/// \retval STFS_ExternalFileMetadataContainer* SUCCESS: Valid Pointer \n
///                                             FAILURE: NULL pointer
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileMetadataContainer*
STFS_ExternalFileMetadataContainer::GetInstance() 
{
  if (STFS_ExternalFileMetadataContainer::EfmContainer_) {
    return STFS_ExternalFileMetadataContainer::EfmContainer_;
  }

  STFS_ExternalFileMetadataContainer::EfmContainer_ = new STFS_ExternalFileMetadataContainer();

  if (!STFS_ExternalFileMetadataContainer::EfmContainer_) {
    return NULL;
  }
    
  return STFS_ExternalFileMetadataContainer::EfmContainer_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Insert
///
/// \brief  Inserts an EFM in the EFM Container for the given key
///
/// \param  char                       *pp_Key (Key)
/// \param  STFS_ExternalFileMetadata  *pp_Efm (Value)
/// 
/// \retval int    SUCCESS: 0             \n
///                FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileMetadataContainer::Insert(char                       *pp_Key,
					   STFS_ExternalFileMetadata  *pp_Efm) 
{
  const char       *WHERE = "STFS_ExternalFileMetadataContainer::Insert";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  if (!pp_Key || !pp_Efm) {
    return -1;
  }

  TRACE_PRINTF2(3,"Size before insert: %ld\n", Size());

  Lock();
  try {
    efmMap_[pp_Key] = pp_Efm;
  }
  catch (...) {
    Unlock();
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }

  Unlock();

  TRACE_PRINTF2(3,"Size after insert: %ld\n", Size());

  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Delete
///
/// \brief  Delete the EFM from the EFM Container for the given key
///
/// \param  char                       *pp_Key (Key)
/// 
/// \retval int    SUCCESS: 0             \n
///                FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileMetadataContainer::Delete(char  *pp_Key)
{
  const char       *WHERE = "STFS_ExternalFileMetadataContainer::Delete";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  if (!pp_Key) {
    return -1;
  }

  TRACE_PRINTF2(3,"Size before erase: %ld\n", Size());

  Lock();
  try {
    STFS_ExternalFileMetadataContainer::efmMap_Def::iterator it;
    it = efmMap_.find(pp_Key);
    if (it != efmMap_.end()) {
      char *lp_Key = (*it).first;
      if (efmMap_.erase(pp_Key) <= 0) {
	lv_RetCode = -1;
      }
      delete [] lp_Key;
    }
  }
  catch (...) {
    Unlock();
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }

  Unlock();

  TRACE_PRINTF2(3,"Size after erase: %ld\n", Size());
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Get
///
/// \brief  Returns the EFM from the EFM Container for the given key
///
/// \param  char                       *pp_Key (Key)
/// 
/// \retval STFS_ExternalFileMetadata*  SUCCESS: Valid Pointer \n
///                                     FAILURE: NULL pointer
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileMetadata*
STFS_ExternalFileMetadataContainer::Get(char *pp_Key)
{
  const char       *WHERE = "STFS_ExternalFileMetadataContainer::Get";
  STFS_ScopeTrace   lv_st(WHERE,3);

  if (!pp_Key) {
    return NULL;
  }

  STFS_ExternalFileMetadataContainer::efmMap_Def::iterator it;
  it = efmMap_.find(pp_Key);
  if (it == efmMap_.end()) {
    return NULL;
  }
    
  STFS_ExternalFileMetadata* lp_Efm = (*it).second;

  return lp_Efm;
}

STFS_ExternalFileMetadata*
STFS_ExternalFileMetadataContainer::Geti(long pv_Index)
{
  if (pv_Index < 0) {
    return NULL;
  }

  efmMap_Def::iterator it;
  STFS_ExternalFileMetadata *lp_Efm = NULL;
  
  long i = 0;
  for(it = efmMap_.begin(); it != efmMap_.end(); it++)
  {
    if(i == pv_Index) {
      lp_Efm = (*it).second;
      break;
    }
    i++;
  }

  //If index is not found, lp_Efm will be NULL
  return lp_Efm;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Size
///
/// \brief  Returns the number of items(EFMs) in the container
///
/// \retval int    SUCCESS: number of EFMs in the container
///                FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
long
STFS_ExternalFileMetadataContainer::Size() 
{
  return efmMap_.size();
}

///////////////////////////////////////////////////////////////////////////////
///
//          Walk
///
/// \brief  Traverses all the entries in the EFM container and trace_prints 
///         some information (For debugging purpose)
///
/// \retval void
///
///////////////////////////////////////////////////////////////////////////////
void
STFS_ExternalFileMetadataContainer::Walk()
{
  STFS_ExternalFileMetadataContainer::efmMap_Def::iterator it;

  //debugging begin
  for (it = efmMap_.begin();
       it != efmMap_.end();
       it++) {
    char *lp_Key = (*it).first;
    STFS_ExternalFileMetadata* lp_Efm = (*it).second;
    if (lp_Key) {
      TRACE_PRINTF2(3, "Key: %s\n", 
		    (*it).first);
    }
    if (lp_Efm) {
      TRACE_PRINTF2(3, "EFM: %s\n", 
		    lp_Efm->ExternalFileNameGet());
    }
  }
  //debugging end
}

///////////////////////////////////////////////////////////////////////////////
///
//          Cleanup
///
/// \brief  Traverses all the entries in the EFM container
///         and invokes the Close() method on each EFM
///
///
///////////////////////////////////////////////////////////////////////////////
void
STFS_ExternalFileMetadataContainer::Cleanup()
{
  const char       *WHERE = "STFS_ExternalFileMetadataContainer::Cleanup";
  STFS_ScopeTrace   lv_st(WHERE,2);

  STFS_ExternalFileMetadataContainer::efmMap_Def::iterator it;

  TRACE_PRINTF2(1,"Size of the container: %ld\n",Size());

  for (it = efmMap_.begin();
       it != efmMap_.end();
       it++) {
    STFS_ExternalFileMetadata* lp_Efm = (*it).second;
    if (lp_Efm) {
      lp_Efm->Close(true);
    }
  }
}

STFS_ExternalFileMetadataContainer::STFS_ExternalFileMetadataContainer():
  STFS_Root(EC_ExternalFileMetadataContainer)
{}


///////////////////////////////////////////////////////////////////////////////
//
// STFS_ExternalFileOpener implementation
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpener::operator==
///
/// \brief  To compare two STFS_ExternalFileOpener objects (*this and the 
///         other STFS_ExternalFileOpener object)
///
/// \param  ptr to this, reference to 'other'
/// 
/// \retval bool   true ('*this' and 'other' have the same content
///                false
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileOpener::operator==(const STFS_ExternalFileOpener& other) const
{
  if (this->sqOpenerNodeId_ != other.sqOpenerNodeId_) {
    return false;
  }

  if (this->sqOpenerPID_ != other.sqOpenerPID_) {
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpener::operator()
///
/// \brief  To compare two STFS_ExternalFileOpener objects 
///
/// \param  pp_Lhs
/// \param  pp_Rhs
/// 
/// \retval bool   returns a 'true' when pp_Lhs < pp_Rhs
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileOpener::operator() (STFS_ExternalFileOpener *pp_Lhs, 
				     STFS_ExternalFileOpener *pp_Rhs) const
{
  if (pp_Lhs->sqOpenerNodeId_ < pp_Rhs->sqOpenerNodeId_) {
    return true;
  }
      
  if (pp_Lhs->sqOpenerPID_ < pp_Rhs->sqOpenerPID_) {
    return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////
//
// STFS_OpenIdentifierComparator implementation
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_OpenIdentifierComparator::operator()
///
/// \brief  To compare two STFS_OpenIdentifier objects 
///
/// \param  pp_Lhs
/// \param  pp_Rhs
/// 
/// \retval bool   returns a 'true' when pp_Lhs < pp_Rhs
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_OpenIdentifierComparator::operator() (STFS_OpenIdentifier *pp_Lhs, 
					   STFS_OpenIdentifier *pp_Rhs) const 
{
  if (!pp_Lhs) {
    return true;
  }

  if (!pp_Rhs) {
    return false;
  }

  if (pp_Lhs->sqOwningDaemonNodeId == pp_Rhs->sqOwningDaemonNodeId) {
    if (pp_Lhs->openIdentifier < pp_Rhs->openIdentifier) {
      return true;
    }
    else {
      return false;
    }
  }

  if (pp_Lhs->sqOwningDaemonNodeId < pp_Rhs->sqOwningDaemonNodeId) {
    return true;
  }

  return false;
}


///////////////////////////////////////////////////////////////////////////////
//
// STFS_ExternalFileOpenerContainer method implementation
//
///////////////////////////////////////////////////////////////////////////////

bool
STFS_ExternalFileOpenerContainer::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid(STFS_Root::EC_ExternalFileOpenerContainer);
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::GetInstance
///
/// \brief  returns an instance of the STFS_ExternalFileOpenerContainer
///         (creates the container if necessary)
///          
/// \param  void
/// 
/// \retval STFS_ExternalFileOpenerContainer* 
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileOpenerContainer*
STFS_ExternalFileOpenerContainer::GetInstance() 
{
  if (STFS_ExternalFileOpenerContainer::EfoContainer_) {
    return STFS_ExternalFileOpenerContainer::EfoContainer_;
  }

  STFS_ExternalFileOpenerContainer::EfoContainer_ = new STFS_ExternalFileOpenerContainer();

  if (!STFS_ExternalFileOpenerContainer::EfoContainer_) {
    return NULL;
  }
    
  return STFS_ExternalFileOpenerContainer::EfoContainer_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::InsertEfo
///
/// \brief  Maintains the efoMap_. 
///
///         Inserts a new key(STFS File Name) if not present. 
///         Adds into the value.
///
///         Map Key: STFS File Name
///         Map Val: Vector of STFS_ExternalFileOpener
///         Used to keep track of all the openers of a particular STFS file
///          
/// \param  STFS_ExternalFileOpenerInfo  *
/// 
/// \retval short   0 if successful.
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileOpenerContainer::InsertEfo(STFS_ExternalFileOpenerInfo  *pp_Efoi) 
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::InsertEfo";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  if (!pp_Efoi) {
    return -1;
  }

  TRACE_PRINTF2(3,"Size before insert: %ud\n", efoMap_.size());

  try {
    efoMap_Def::iterator lv_EfoIt;
    STFS_ExternalFileMetadata *lv_Efm = pp_Efoi->efm_;
    char *lp_ExternalFileName = lv_Efm->ExternalFileNameGet();

    lv_EfoIt = efoMap_.find(lp_ExternalFileName);
    efoVector_Def *lp_EfoVector = 0;
    if (lv_EfoIt != efoMap_.end()) {
      lp_EfoVector = (*lv_EfoIt).second;
    }
    else {
      lp_EfoVector = new efoVector_Def;
      efoMap_[lp_ExternalFileName] = lp_EfoVector;
    }
    STFS_ExternalFileOpener *lp_Efo = &(pp_Efoi->efo_);
    lp_EfoVector->push_back(lp_Efo);
  }
  catch (...) {
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }

  TRACE_PRINTF2(3,"Size after insert: %ud\n", efoMap_.size());
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::InsertEFOpenIdVector
///
/// \brief  Maintains the efOpenIdVectorMap_
///
///         Inserts a new key if not present. 
///         Adds into the value.
///
///         Map Key: STFS_ExternalFileOpener 
///         Map Val: Vector of STFS_OpenIdentifier
///         Used to keep track of all the Open Ids of an opener process
///          
/// \param  STFS_ExternalFileOpenerInfo  *
/// \param  STFS_OpenIdentifier *
/// 
/// \retval short   0 if successful.
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileOpenerContainer::InsertEFOpenIdVector(STFS_ExternalFileOpenerInfo  *pp_Efoi,
						       STFS_OpenIdentifier          *pp_OpenId) 
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::InsertEFOpenIdVector";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  if (!pp_Efoi) {
    return -1;
  }

  TRACE_PRINTF2(3,"Size before insert: %ud\n", efOpenIdVectorMap_.size());

  try {
    efOpenIdVectorMap_Def::iterator lv_EfOpenIdVectorMap_It;
    STFS_ExternalFileOpener   *lp_ExternalFileOpener ;
    
    lp_ExternalFileOpener = &pp_Efoi->efo_;
    
    lv_EfOpenIdVectorMap_It = efOpenIdVectorMap_.find(lp_ExternalFileOpener);
    efOpenIdVector_Def *lp_EfOpenIdVector = 0;
    if (lv_EfOpenIdVectorMap_It != efOpenIdVectorMap_.end()) {
      lp_EfOpenIdVector = (*lv_EfOpenIdVectorMap_It).second;
    }
    else {
      lp_EfOpenIdVector = new efOpenIdVector_Def;
      lp_ExternalFileOpener = new STFS_ExternalFileOpener();
      memcpy(lp_ExternalFileOpener, &pp_Efoi->efo_, sizeof(STFS_ExternalFileOpener));
      efOpenIdVectorMap_[lp_ExternalFileOpener] = lp_EfOpenIdVector;
    }
    lp_EfOpenIdVector->push_back(*pp_OpenId);
  }
  catch (...) {
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }

  TRACE_PRINTF2(3,"Size after insert: %ud\n", efOpenIdVectorMap_.size());
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Insert
///
/// \brief  Maintains the efoiMap_
///         Also calls the private methods InsertEfo() & InsertEFOpenIdVector()
///
///         Inserts a new key if not present. 
///
///         Map Key: STFS_OpenIdentifier *
///         Map Val: STFS_ExternalFileOpenerInfo *
///         Used to get the OpenerInfo for a given Open Id
///          
/// \param  STFS_OpenIdentifier *
/// \param  STFS_ExternalFileOpenerInfo  *
/// 
/// \retval short   0 if successful.
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileOpenerContainer::Insert(STFS_OpenIdentifier          *pp_OpenId,
					 STFS_ExternalFileOpenerInfo  *pp_Efo) 
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::Insert";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  if (!pp_OpenId || !pp_Efo) {
    return -1;
  }

  TRACE_PRINTF2(3,"Size before insert: %ld\n", Size());

  Lock();
  try {
    efoiMap_[pp_OpenId] = pp_Efo;

    lv_RetCode = InsertEfo(pp_Efo);
    if (lv_RetCode < 0) {
      TRACE_PRINTF2(1,"Error %d from InsertEfo\n", lv_RetCode);
      return lv_RetCode;
    }

    lv_RetCode= InsertEFOpenIdVector(pp_Efo, pp_OpenId);
    if (lv_RetCode < 0) {
      TRACE_PRINTF2(1,"Error %d from InsertEfOpenIdVector\n", lv_RetCode);
      return lv_RetCode;
    }

  }
  catch (...) {
    Unlock();
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }

  Unlock();

  TRACE_PRINTF2(3,"Size after insert: %ld\n", Size());
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::DeleteEfo
///
/// \brief  Maintains the efoMap_. 
///
///         Delete the entry from the map with the specified key if
///         the asscoiated vector is empty. 
///         Deletes an entry from the associated vector.
///
///         Map Key: STFS File Name
///         Map Val: Vector of STFS_ExternalFileOpener
///         Used to keep track of all the openers of a particular STFS file
///          
/// \param  STFS_ExternalFileOpenerInfo  *
/// 
/// \retval short   0 if successful.
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileOpenerContainer::DeleteEfo(STFS_ExternalFileOpenerInfo *pp_Efoi)
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::DeleteEfo";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  if (!pp_Efoi) {
    return 0;
  }

  TRACE_PRINTF2(3,"Size before erase: %ud\n", efoMap_.size());

  try {

    efoMap_Def::iterator lv_EfoIt;
    STFS_ExternalFileMetadata *lv_Efm = pp_Efoi->efm_;
    char *lp_ExternalFileName = lv_Efm->ExternalFileNameGet();
    lv_EfoIt = efoMap_.find(lp_ExternalFileName);

    if (lv_EfoIt == efoMap_.end()) {
      TRACE_PRINTF2(1, "Could not find %s in efoMap\n", lp_ExternalFileName);
      return -1;
    }

    efoVector_Def *lp_EfoVector = (*lv_EfoIt).second;
    
    if (! lp_EfoVector) {
      TRACE_PRINTF2(1, "Could not find the opener vector for %s in efoMap\n", lp_ExternalFileName);
      return -1;
    }

    bool lv_Done = false;
    efoVector_Def::iterator lp_EfoVectorIt = lp_EfoVector->begin();
    while (!lv_Done) {
      if (lp_EfoVectorIt == lp_EfoVector->end()) {
	lv_Done = true;
	continue;
      }
      STFS_ExternalFileOpener *lp_Efo = *lp_EfoVectorIt;
      if (*lp_Efo == pp_Efoi->efo_) {
	lp_EfoVector->erase(lp_EfoVectorIt);
	lv_Done = true;
	continue;
      }
      lp_EfoVectorIt++;
    }

    if (lp_EfoVector->size() <= 0) {
      delete lp_EfoVector;
      efoMap_.erase(lv_EfoIt);
    }
  }
  catch (...) {
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }
 
  TRACE_PRINTF2(3,"Size after erase: %ud\n", efoMap_.size());
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::DeleteEFOpenIdVector
///
/// \brief  Maintains the efOpenIdVectorMap_
///
///         Deletes the entry from the map with the specified key if 
///         the asscoiated vector is empty.
///         Deletes an entry from the associated vector.
///
///         Map Key: STFS_ExternalFileOpener *
///         Map Val: Vector of STFS_OpenIdentifier
///         Used to keep track of all the Open Ids of an opener process
///          
/// \param  STFS_ExternalFileOpenerInfo  *
/// \param  STFS_OpenIdentifier *
/// 
/// \retval short   0 if successful.
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileOpenerContainer::DeleteEFOpenIdVector(STFS_ExternalFileOpenerInfo *pp_Efoi,
						       STFS_OpenIdentifier         *pp_OpenId)
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::DeleteEFOpenIdVector";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  if (!pp_Efoi) {
    return 0;
  }

  TRACE_PRINTF2(3,"Size before erase: %ud\n", efOpenIdVectorMap_.size());

  try {

    efOpenIdVectorMap_Def::iterator  lv_EfOpenIdVectorMap_It;
    STFS_ExternalFileOpener         *lp_ExternalFileOpener ;
    
    lp_ExternalFileOpener = &pp_Efoi->efo_;

    lv_EfOpenIdVectorMap_It = efOpenIdVectorMap_.find(lp_ExternalFileOpener);

    if (lv_EfOpenIdVectorMap_It == efOpenIdVectorMap_.end()) {
      TRACE_PRINTF3(3, 
		    "Could not find %d,%d in efOpenIdVectorMap\n",
		    lp_ExternalFileOpener->sqOpenerNodeId_,
		    lp_ExternalFileOpener->sqOpenerPID_);
		    
      return -1;
    }

    efOpenIdVector_Def *lp_EfOpenIdVector = (*lv_EfOpenIdVectorMap_It).second;
    
    if (! lp_EfOpenIdVector) {
      TRACE_PRINTF3(1, "Could not find the open id vector for %d,%d in efOpenIdVectorMap\n", 
		    lp_ExternalFileOpener->sqOpenerNodeId_,
		    lp_ExternalFileOpener->sqOpenerPID_);
      return -1;
    }

    bool lv_Done = false;
    efOpenIdVector_Def::iterator lv_EfOpenIdVector_It = lp_EfOpenIdVector->begin();
    while (!lv_Done) {
      if (lv_EfOpenIdVector_It == lp_EfOpenIdVector->end()) {
	lv_Done = true;
	continue;
      }
      STFS_OpenIdentifier lv_OpenId = *lv_EfOpenIdVector_It;
      if (lv_OpenId == *pp_OpenId) {
	lp_EfOpenIdVector->erase(lv_EfOpenIdVector_It);
	lv_Done = true;
	continue;
      }
      lv_EfOpenIdVector_It++;
    }

    if (lp_EfOpenIdVector->size() <= 0) {
      delete lp_EfOpenIdVector;
      delete (*lv_EfOpenIdVectorMap_It).first;
      efOpenIdVectorMap_.erase(lv_EfOpenIdVectorMap_It);
    }
  }
  catch (...) {
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }
 
  TRACE_PRINTF2(3,"Size after erase: %ud\n", efOpenIdVectorMap_.size());
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Delete
///
/// \brief  Maintains the efoiMap_
///         Also calls the private methods DeleteEfo() & DeleteEFOpenIdVector()
///
///         Deletes the entry in the map with the specified key
///
///         Map Key: STFS_OpenIdentifier *
///         Map Val: STFS_ExternalFileOpenerInfo *
///         Used to get the OpenerInfo for a given Open Id
///          
/// \param  STFS_OpenIdentifier *
/// 
/// \retval short   0 if successful.
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileOpenerContainer::Delete(STFS_OpenIdentifier *pp_OpenId)
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::Delete";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  if (!pp_OpenId) {
    return 0;
  }

  TRACE_PRINTF2(3,"Size before erase: %ld\n", Size());

  Lock();
  try {
    efoiMap_Def::iterator it;
    it = efoiMap_.find(pp_OpenId);
    if (it != efoiMap_.end()) {
      STFS_ExternalFileOpenerInfo *lp_Efoi = (*it).second;
      STFS_OpenIdentifier *lp_OpenId = (*it).first;
      if (lp_OpenId) {
	delete lp_OpenId;
      }
      
      if (lp_Efoi) {
	lv_RetCode = DeleteEfo(lp_Efoi);
	if (lv_RetCode < 0) {
	  TRACE_PRINTF2(1,"Error %d from DeleteEfo\n", lv_RetCode);
	  goto exit;
	}

	lv_RetCode= DeleteEFOpenIdVector(lp_Efoi, pp_OpenId);
	if (lv_RetCode < 0) {
	  TRACE_PRINTF2(1,"Error %d from DeleteEFOpenIdVector\n", lv_RetCode);
	  goto exit;
	}

	delete lp_Efoi;
      }
      efoiMap_.erase(it);
    }
  }
  catch (...) {
    Unlock();
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }
 
  TRACE_PRINTF2(3,"Size after erase: %ld\n", Size());
 exit:
  Unlock();
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Get
///
/// \brief  Gets the STFS_ExternalFileOpenerInfo* for the 
///         given Open Id
///          
/// \param  STFS_OpenIdentifier *
/// 
/// \retval STFS_ExternalFileOpenerInfo*
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileOpenerInfo*
STFS_ExternalFileOpenerContainer::Get(STFS_OpenIdentifier *pp_OpenId)
{
  if (!pp_OpenId) {
    return NULL;
  }

  efoiMap_Def::iterator         it;
  STFS_ExternalFileOpenerInfo *lp_Efo = 0;

  it = efoiMap_.find(pp_OpenId);
  if (it != efoiMap_.end()) {
    lp_Efo = (*it).second;
  }

  return lp_Efo;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Geti
///
/// \brief  Gets the STFS_ExternalFileOpenerInfo* at the specified index
///          
/// \param  long pv_Index
/// 
/// \retval STFS_ExternalFileOpenerInfo*
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileOpenerInfo*
STFS_ExternalFileOpenerContainer::Geti(long pv_Index)
{
  if (pv_Index < 0) {
    return NULL;
  }

  efoiMap_Def::iterator it;
  STFS_ExternalFileOpenerInfo *lp_Efo = 0;
  
  long i = 0;
  for(it = efoiMap_.begin(); it != efoiMap_.end(); it++)
  {
    if(i == pv_Index) {
      lp_Efo = (*it).second;
      break;
    }
    i++;
  }

  return lp_Efo;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Get
///
/// \brief  Gets the STFS_ExternalFileOpener* at the given index from the 
///         vector of ExternalFileOpener* for the specified file 
///          
/// \param  char *      pp_ExternalFileName
/// \param  int         index
/// 
/// \retval STFS_ExternalFileOpener*
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileOpener *
STFS_ExternalFileOpenerContainer::Get(char *pp_ExternalFileName,
				      int   pv_Index)
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::Get";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (pv_Index  < 0) {
    TRACE_PRINTF1(1, "Negative value provided for the index into the vector\n");
    return NULL;
  }

  efoVector_Def *lp_EfoVector = GetEfoVector(pp_ExternalFileName);
  if (!lp_EfoVector) {
    return NULL;
  }

  if (pv_Index >= (int) lp_EfoVector->size()) {
    TRACE_PRINTF3(1, "Provided index: %d > number of items in the vector: %ud\n", 
		  pv_Index,
		  lp_EfoVector->size());
    return NULL;
  }

  return (*lp_EfoVector)[pv_Index];


}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Get
///
/// \brief  Gets the STFS_OpenIdentifier* at the given index from the 
///         vector of STFS_OpenIdentifier* for the specified opener process
///          
/// \param  STFS_ExternalFileOpener *pp_EFO
/// \param  int                     pv_index
/// 
/// \retval STFS_OpenIdentifier*
///
///////////////////////////////////////////////////////////////////////////////
STFS_OpenIdentifier*
STFS_ExternalFileOpenerContainer::Get(STFS_ExternalFileOpener *pp_EFO,
				      int                      pv_Index)
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::Get(STFS_ExternalFileOpener *, index)";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (pv_Index  < 0) {
    TRACE_PRINTF1(1, "Negative value provided for the index into the vector\n");
    return NULL;
  }

  efOpenIdVector_Def* lp_EfOpenIdVector = GetOpenIdVector(pp_EFO);
  if (!lp_EfOpenIdVector) {
    return NULL;
  }

  if (pv_Index >= (int) lp_EfOpenIdVector->size()) {
    TRACE_PRINTF3(1, "Provided index: %d > number of items in the vector: %ud\n", 
		  pv_Index,
		  lp_EfOpenIdVector->size());
    return NULL;
  }

  return &((*lp_EfOpenIdVector)[pv_Index]);

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Size
///
/// \brief  Returns the number of openers for a particular STFS file
///          
/// \param  char *      Name of the STFS File
/// 
/// \retval long 
///
///////////////////////////////////////////////////////////////////////////////
long
STFS_ExternalFileOpenerContainer::Size(char* pp_ExternalFileName) 
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::Size(char *)";
  STFS_ScopeTrace   lv_st(WHERE,2);


  efoVector_Def *lp_EfoVector = GetEfoVector(pp_ExternalFileName);
  if (!lp_EfoVector) {
    return 0;
  }

  return lp_EfoVector->size();
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Size
///
/// \brief  Returns the number of open ids
///          
/// \param  void
/// 
/// \retval long 
///
///////////////////////////////////////////////////////////////////////////////
long
STFS_ExternalFileOpenerContainer::Size() 
{
  return efoiMap_.size();
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::GetEfoVector
///
/// \brief  returns the vector(of openers) for the given STFS file
///
/// \param  char *pp_ExternalFileName
///
/// \retval efoVector_Def*
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileOpenerContainer::efoVector_Def* 
STFS_ExternalFileOpenerContainer::GetEfoVector(char *pp_ExternalFileName)
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::GetVector";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!pp_ExternalFileName || !pp_ExternalFileName[0]) {
    TRACE_PRINTF1(1,"Null external file name provided as parameter\n");
    return NULL;
  }

  efoMap_Def::iterator lv_EfoIt;

  lv_EfoIt = efoMap_.find(pp_ExternalFileName);
  if (lv_EfoIt == efoMap_.end()) {
    TRACE_PRINTF2(3,"%s not found in efoMap\n", 
		  pp_ExternalFileName);
    return NULL;
  }
  
  efoVector_Def *lp_EfoVector = (*lv_EfoIt).second;

  return lp_EfoVector;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::GetOpenIdVector
///
/// \brief  returns the vector(of open ids) for the given opener
///
/// \param  STFS_ExternalFileOpener *pp_EFO
///
/// \retval efOpenIdVector_Def*
///
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileOpenerContainer::efOpenIdVector_Def*
STFS_ExternalFileOpenerContainer::GetOpenIdVector(STFS_ExternalFileOpener *pp_EFO)
{
  const char       *WHERE = "STFS_ExternalFileOpenerContainer::GetOpenIdVector";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!pp_EFO) {
    TRACE_PRINTF1(1,"Null pp_EFO provided as parameter\n");
    return NULL;
  }

  efOpenIdVectorMap_Def::iterator lv_EfOpenIdVectorMap_It;

  lv_EfOpenIdVectorMap_It = efOpenIdVectorMap_.find(pp_EFO);
  if (lv_EfOpenIdVectorMap_It == efOpenIdVectorMap_.end()) {
    TRACE_PRINTF3(3,"%d,%d not found in efOpenIdVectorMap\n", 
		  pp_EFO->sqOpenerNodeId_,
		  pp_EFO->sqOpenerPID_);
    return NULL;
  }
  
  efOpenIdVector_Def *lp_EfOpenIdVector = (*lv_EfOpenIdVectorMap_It).second;

  return lp_EfOpenIdVector;

}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileOpenerContainer::Walk
///
/// \brief  Traverses all the entries in the EFO container and TRACE_PRINTS 
///         some information (For debugging purpose)
///
///
///////////////////////////////////////////////////////////////////////////////
void
STFS_ExternalFileOpenerContainer::Walk()
{
  STFS_ExternalFileOpenerContainer::efoiMap_Def::iterator it;

  for (it = efoiMap_.begin();
       it != efoiMap_.end();
       it++) {
    STFS_OpenIdentifier         *lp_OpenId = (*it).first;
    STFS_ExternalFileOpenerInfo *lp_Efo = (*it).second;
    if (lp_OpenId) {
      TRACE_PRINTF3(3, "Key: %d,%ld\n", 
		    lp_OpenId->sqOwningDaemonNodeId, 
		    lp_OpenId->openIdentifier);
    }
    if (lp_Efo) {
      TRACE_PRINTF3(3, "EFO: %d,%d\n", 
		    lp_Efo->efo_.sqOpenerNodeId_, 
		    lp_Efo->efo_.sqOpenerPID_);
    }
  }
}

STFS_ExternalFileOpenerContainer::STFS_ExternalFileOpenerContainer():
  STFS_Root(EC_ExternalFileOpenerContainer)
{}

#ifdef MAIN
int main()
{
  char fileName[STFS_SIZE_MAX];
  STFS_FragmentFileMetadata *ffm = new STFS_FragmentFileMetadata("testingName");
   
  fileName = ffm->NameGet();

  return 0;
}

#endif
