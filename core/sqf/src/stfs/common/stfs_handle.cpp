///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_handle.cpp
/// \brief   Implementation of STFS_ExternalFileHandle, 
///          STFS_FragmentFileHandle, and STFS_ExternalFileHandleContainer
///   
/// This file contains the STFS handle implementation.  
/// It includes STFS_ExternalFileHandle, STFS_FragmentFileHandle, 
/// and STFS_ExternalFileHandleContainer.
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

#include "seabed/trace.h"
#include "stfs_sigill.h"
#include "stfs_handle.h"

using namespace STFS;

STFS_ExternalFileHandleContainer*  STFS_ExternalFileHandleContainer::EfhContainer_ = 0;

///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_ExternalFileHandle methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileHandle::STFS_ExternalFileHandle():
  STFS_Root(STFS_Root::EC_ExternalFileHandle),
  efm_(0),
  currentOffset_(0),
  fileEOF_(0),
  openFlags_(0),
  currentFFH_(0)
{
  fragForcing_.forceFragmentCreation = false;
  fragForcing_.maxCounter = 0;
  fragForcing_.writesSinceLastFragCreate = 0;
  fragForcing_.partialWrites=false;
}

STFS_ExternalFileHandle::STFS_ExternalFileHandle(STFS_ExternalFileMetadata *pp_Efm):
  STFS_Root(STFS_Root::EC_ExternalFileHandle),
  efm_(pp_Efm),
  currentOffset_(0),
  fileEOF_(0),
  openFlags_(0),
  currentFFH_(0)
{

  char *lp_pointer = NULL;
  char lp_envVar[40];

  // Check to see if we need to force fragmentation

  fragForcing_.forceFragmentCreation = false;
  fragForcing_.maxCounter = 0;
  fragForcing_.writesSinceLastFragCreate = 0;
  fragForcing_.partialWrites=false;

  strcpy (lp_envVar, EV_FORCEFRAGCREATION);

  lp_pointer = getenv (lp_envVar);

  if (lp_pointer != NULL) {
    int lv_numWrites = atoi (lp_pointer);

    if (lv_numWrites > 0) {

      fragForcing_.forceFragmentCreation = true;
      fragForcing_.maxCounter = lv_numWrites;
      fragForcing_.writesSinceLastFragCreate=0;

      //  The partial writes envvar only applies if we're already forcing
      //  fragment creation.

      strcpy (lp_envVar, EV_FRAGPARTIALWRITE);

      lp_pointer = getenv (lp_envVar);

      if (lp_pointer != NULL) {
	// the envvar exists, but we need to see if it's set to 1
	int lv_partialWritesVal = atoi (lp_pointer);

	if (lv_partialWritesVal == 1) {
	  fragForcing_.partialWrites = true;
	}
      }
    }
  }
}

//dtor
STFS_ExternalFileHandle::~STFS_ExternalFileHandle()
{
  size_t lv_NumFragments = GetNumFragmentsInEFH();
  for (int i = 0; i < (int) lv_NumFragments; i++) {
    STFS_FragmentFileHandle *lp_Ffh = GetFragment(i);
    if (lp_Ffh) {
      delete lp_Ffh;
      lp_Ffh = 0;
    }
  }
  
  // Cleaning up the ffh vector
  bool lv_Done = false;
  ffhVector_Def::iterator it;
  it = ffhVector_.begin();
  while (!lv_Done) {
    if (it == ffhVector_.end()) {
      lv_Done = true;
      continue;
    }
    // the erase() returns the next item 
    it = ffhVector_.erase(it);
  }

}

bool
STFS_ExternalFileHandle::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid(STFS_Root::EC_ExternalFileHandle);
}

///////////////////////////////////////////////////////////////////////////////
///
//          Read
///
/// \brief  Read from a file using efh
///
/// Reads the number of bytes specified by count
///
/// \param pp_Buf   Buffer where the read contents are returned
/// \param pv_Count Value between zerio and SSIZE_MAX
/// 
/// \retval int    SUCCESS: The number of bytes read is returned     \n
///                FAILURE: -1 is returned and errno is set.
///
///////////////////////////////////////////////////////////////////////////////
long 
STFS_ExternalFileHandle::Read(char  *pp_Buf,
                              size_t pv_Count)
{
  const char       *WHERE = "STFS_ExternalFileHandle::Read";
  STFS_ScopeTrace   lv_st(WHERE,2);

  long  lv_FragmentOffset = 0;
  ssize_t lv_Ret = 0;

  DEBUG_CHECK_EYECATCHER(this,-1);

  if (((this->openFlags_ & O_RDWR) != O_RDWR) &&
      ((this->openFlags_ & O_RDONLY) != O_RDONLY)) {
    errno = EACCES;
    SetError (true, errno, 0, NULL, 0);
    return -1;
  }

  // find the appropriate FFH
  if (!currentFFH_) {
    if (EvalCurrentFFH(currentOffset_, lv_FragmentOffset) < 0) {
      errno = EBADF;
      SetError (true, errno, 0, NULL, 0);
      return -1;
    }
  }
  
  // Check if ffh is open, else open it
  if (!currentFFH_->IsOpen()) {
    int lv_OpenRet = currentFFH_->Open(this->openFlags_);
    if (lv_OpenRet < 0) {
      return lv_OpenRet;
    }
  }
    
  // perform the read
  lv_Ret = currentFFH_->Read(pp_Buf, pv_Count);
  if (lv_Ret < 0) {
    return lv_Ret;
  }

  currentOffset_ += lv_Ret;
   
  return lv_Ret;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Write
///
/// \brief  Write to a file using efh
///
/// Writes the number of bytes in pp_Data specified by count to a file
///
/// \param  pp_Data  Buffer with the write contents 
/// \param  pv_Count value between zero and SSIZE_MAX
/// 
/// \retval short SUCCESS: 0 or return code for create();        \n
///               FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
ssize_t
STFS_ExternalFileHandle::Write(const void *pp_Data,
                               long        pv_Count)
{
  const char       *WHERE = "STFS_ExternalFileHandle::Write";
  STFS_ScopeTrace   lv_st(WHERE,2);



  ///  Initial Setup and Housekeeping


  long lv_count = pv_Count;

  DEBUG_CHECK_EYECATCHER(this,-1);


  ///   Check security and access permissions


  if (((this->openFlags_ & O_RDWR) != O_RDWR) &&
      ((this->openFlags_ & O_WRONLY) != O_WRONLY)) {
    errno = EACCES;
    SetError (true, errno, 0, NULL, 0);
    return -1;
  }

  ///  Check overall STFS File Size

  //  Rev. signed/unsigned comparison
  if ( (unsigned long)(currentOffset_+ pv_Count) > STFS_SSIZE_MAX ) {
    errno = ENOMEM;
    SetError (true, errno, 0, NULL, 0);
    return -1;
  }

  ///  TBD:  Incorporate any EOF-relative positioning

  // Assert for now that no one as specified O_Append
  // Remove this assertion when we've got the real code
  ASSERT ( (this->openFlags_ & O_APPEND) != O_APPEND);


  ///  Position to the current open fragment


  long lv_FragmentOffset = 0;

  if (!currentFFH_) {
    if (EvalCurrentFFH(currentOffset_, lv_FragmentOffset) < 0) {
      return -1;
    }
  }
  

  // Open Fragment if needed


  if (!currentFFH_->IsOpen()) {
    int lv_OpenRet = currentFFH_->Open(this->openFlags_);
    if (lv_OpenRet < 0) {
      return lv_OpenRet;
    }
  }
  

  /// Force Fragment Creation if needed


  // See if we're forcing fragment creation and it's time to force a
  // fragment. We only play games with fragments if we're in the library.
  // Debugging in the daemon will require a different mechanism...

  if (STFS_util::executingInDaemon_ == false) {
    if (fragForcing_.forceFragmentCreation == true ) {

      fragForcing_.writesSinceLastFragCreate++;
      if (fragForcing_.writesSinceLastFragCreate > fragForcing_.maxCounter) {

	// time to pretend that we've got an error here!  Reset our counter so
	// we don't go through this again...

	fragForcing_.writesSinceLastFragCreate = 0;

	errno = ENOSPC;
	return -1;
      }
      else if ( (fragForcing_.writesSinceLastFragCreate == fragForcing_.maxCounter) &&
		(fragForcing_.partialWrites == true) ) {

	// break up this I/O into a smaller chunk and just write that.
	// Assuming all is going well, the other part of the buffer will be
	// written at the next call, which will blow up on the "are we over the
	// max?" test.  This simulates a real file-full condition.  

	lv_count  = pv_Count * (rand()/RAND_MAX);
      }
    }
  }

  /// Do initial write

  ssize_t lv_WriteRet = currentFFH_->Write(pp_Data, lv_count);

  /// Handle basic write errors
  if (lv_WriteRet < 0) {

    return lv_WriteRet;
  }

  /// Track our current position
  currentOffset_ += lv_WriteRet;

  /// return the count written
  return lv_WriteRet;

}

///////////////////////////////////////////////////////////////////////////////
///
//          Lseek
///
///  \brief Reposition read/write file offset for efh
///
///  Reposition read/write file offset for efh
///
/// \param  pv_Offset  byte offset count based on whence
/// \param  pv_Whence  values are as follow: \n
///                         SEEK_SET - The offset is set to offset bytes. \n
///                         SEEK_CUR - The offset is set to its current
///                                    location plus offset bytes. \n
///                         SEEK_END - The offset is set to the size of the 
///                                    file plus offset bytes. 
/// 
/// \retval off_t SUCCESS: The resulting offset location from beginning
///                        of current fragment file.\n
///               FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
off_t 
STFS_ExternalFileHandle::Lseek(off_t pv_Offset,
                               int   pv_Whence)
{
  const char       *WHERE = "STFS_ExternalFileHandle::Lseek";
  STFS_ScopeTrace   lv_st(WHERE,2);

  // find the appropriate FFH
  long lv_FragmentOffset = 0;

  // Make sure our picture of FFHs matches the FFM's picture

  if (NewFFHsNeeded() == true) {
    int lv_stat = OpenNewFragments();
    if (lv_stat < 0) {
      // memory allocation error!  tbd
      ASSERT (false);
      return -1;
    }
  }

  if (EvalCurrentFFH(pv_Offset, lv_FragmentOffset, pv_Whence) < 0) {
    return -1;
  }
  
  // Check if ffh is open, else open it
  if (!currentFFH_->IsOpen()) {
    int lv_OpenRet = currentFFH_->Open(this->openFlags_);
    if (lv_OpenRet < 0) {
      return lv_OpenRet;
    }
  }
  
  // perform the absolute lseek
  ssize_t lv_Ret = currentFFH_->Lseek(lv_FragmentOffset, SEEK_SET);
  if (lv_Ret < 0) {
    return lv_Ret;
  }

  currentOffset_ = currentFFH_->FfmGet()->OffsetGet() + lv_Ret;

  return lv_Ret;
  
}


///////////////////////////////////////////////////////////////////////////////
///
//          Close
///
///  \brief Closes the EFH
///
/// \retval int SUCCES:   0
///             FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_ExternalFileHandle::Close()
{
  const char       *WHERE = "STFS_ExternalFileHandle::Close";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Ret = 0;

  // Rev: Check the usage model here - its usefulness etc.
  // Close all the fragment handles
  for (short i = 0;
       i < (int) ffhVector_.size();
       i++) {
    lv_Ret = ffhVector_[i]->Close();
    if (lv_Ret < 0) {
      return lv_Ret;
    }
  }

  return lv_Ret;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Unlink
///
///  \brief Unlinks the EFH
///
/// \retval int SUCCES:   0
///             FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_ExternalFileHandle::Unlink()
{
  const char       *WHERE = "STFS_ExternalFileHandle::Unlink";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Ret = 0;

  for (short i = 0;
       i < (int) ffhVector_.size();
       i++) {
    STFS_FragmentFileMetadata* lp_Ffm = ffhVector_[i]->FfmGet();

    lv_Ret = ffhVector_[i]->Close();

    if (lv_Ret < 0) {
      // TBD:  Any error handling??
      TRACE_PRINTF3(1,"%s: Error %d while closing the fragment\n",
		    WHERE,
		    errno);
      return lv_Ret;
    }


    lv_Ret = ffhVector_[i]->Unlink();
    if (lv_Ret < 0) {
      TRACE_PRINTF3(1,"%s: Error %d while unlinking the fragment\n",
		    WHERE,
		    errno);
      return lv_Ret;
    }

    // TBD  - each fragment could be on a separate directory
    //        so need to delete all such directories
    char lv_DirName[STFS_DIRNAME_MAX];
    sprintf(lv_DirName, "%s/%s", 
            lp_Ffm->StfsDirectoryGet(),
            efm_->ExternalFileNameGet());

    //Rev: see what we can do about the failure in deleting the directory. 
    // ??Put it in a undeleted directory container - to be tried again later??
    //

    //  There might be more than one fragment in the directory

    lv_Ret = rmdir(lv_DirName);
    if (lv_Ret < 0 && errno != ENOTEMPTY) {
      TRACE_PRINTF5(1,"%s: Error %d(%s) while unlinking the directory: %s\n",
		    WHERE,
		    errno,
		    sys_errlist[errno],
		    lv_DirName);
      return lv_Ret;
    }
    
  }
  
  return lv_Ret;
}

///////////////////////////////////////////////////////////////////////////////
///
//          InsertFFH
///
/// \brief  Inserts passed in FFH into FFH container
///
/// Inserts passed in FFH into FFH container
///
/// \param  pp_FFH Pointer to FFH that is to be inserted
/// 
/// \retval off_t SUCCESS:  0
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileHandle::InsertFFH(STFS_FragmentFileHandle *pp_Ffh)
{
  const char       *WHERE = "STFS_ExternalFileHandle::InsertFFH";
  STFS_ScopeTrace   lv_st(WHERE,2);

  Lock();
  try {
    ffhVector_.push_back(pp_Ffh);
  }
  catch (...) {
    Unlock();
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }

  Unlock();
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//  GetFD()
////////////////////////////////////////////////////////////////////////////////
int
STFS_ExternalFileHandle::GetFD()
{
  int lv_Fd = 0;
  STFS_FragmentFileHandle *lp_Ffh = this->CurrentFfhGet();
  if (!lp_Ffh) {
    return -1;
  }
  STFS_fs *lp_Fs = lp_Ffh->FsGet();
  if (!lp_Fs) { 
    return -1;
  }
  lv_Fd = lp_Fs->FdGet();

  // TODO: Add a check that lv_Fd is less than the max number of fragments when 
  // that value is decided.

  return lv_Fd;
}

////////////////////////////////////////////////////////////////////////////////
//  GetNumFragments()
/// Returns the number of FFH's from an EFH's perspective
////////////////////////////////////////////////////////////////////////////////
size_t 
STFS_ExternalFileHandle::GetNumFragmentsInEFH() 
{
  // Rev: Could add another 'ValidateFragments() methods that validates this
  //      the number of FFHs v/s the number of FFMs in the EFM
  return ffhVector_.size();
}

////////////////////////////////////////////////////////////////////////////////
//  GetFragment()
////////////////////////////////////////////////////////////////////////////////

//Rev: Add a comment if the index base is 0 or 1
STFS_FragmentFileHandle *
STFS_ExternalFileHandle::GetFragment(int pv_Index)
{
  const char       *WHERE = "STFS_ExternalFileHandle::GetFragment";
  STFS_ScopeTrace   lv_st(WHERE,3);

  // Rev: Check for a negative value also
  if (pv_Index >= (int) ffhVector_.size()) {
    return (STFS_FragmentFileHandle *) 0;
  }

  return ffhVector_[pv_Index];
}

////////////////////////////////////////////////////////////////////////////////
//  InsertIntoContainer()
////////////////////////////////////////////////////////////////////////////////
int 
STFS_ExternalFileHandle::InsertIntoContainer(STFS_ExternalFileHandle *pp_Efh)
{
  const char       *WHERE = "STFS_ExternalFileHandle::InsertIntoContainer";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_RetCode = 0;

  //Rev: Add an ASSERT
  if (!pp_Efh) {
    return -1;
  }

  STFS_ExternalFileHandleContainer *lp_EfhContainer = STFS_ExternalFileHandle::GetContainer();
  if (!lp_EfhContainer) {
    //Rev: Should be a Software Failure
    TRACE_PRINTF2(1, "%s\n", "Null EFH Container");
    return -1;
  }
      
  lv_RetCode = lp_EfhContainer->Insert(pp_Efh);
  if (lv_RetCode < 0) {
    // Rev:The Container Delete() method should set the appropriate errno
    // as that is the entity that has the knowledge

    // Rev: Have a macro/static method to SetErrorNum() that will ultimately be returned back to the
    // STFS API caller.
    return -1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//  DeleteFromContainer()
////////////////////////////////////////////////////////////////////////////////
int 
STFS_ExternalFileHandle::DeleteFromContainer(STFS_ExternalFileHandle *&pp_Efh)
{
  const char       *WHERE = "STFS_ExternalFileHandle::DeleteFromContainer";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_RetCode = 0;

  //Rev: Add an ASSERT
  if (!pp_Efh) {
    return -1;
  }

  STFS_ExternalFileHandleContainer *lp_EfhContainer = STFS_ExternalFileHandle::GetContainer();
  if (!lp_EfhContainer) {
    //Rev: Should be a Software Failure
    TRACE_PRINTF2(1, "%s\n", 
		  "Null EFH Container");
    return -1;
  }
      
  lv_RetCode = lp_EfhContainer->Delete(pp_Efh);
  if (lv_RetCode < 0) {
    // Rev:The Container Delete() method should set the appropriate errno
    // as that is the entity that has the knowledge
    return -1;
  }

  delete pp_Efh;
  pp_Efh = 0;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//  GetExternalFileHandle()
////////////////////////////////////////////////////////////////////////////////
STFS_ExternalFileHandle*
STFS_ExternalFileHandle::GetExternalFileHandle(stfs_fhndl_t pv_Fhandle)
{
  const char       *WHERE = "STFS_ExternalFileHandle::GetExternalFileHandle";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (pv_Fhandle == 0) {
    // Use the SetErrorNum API
    errno = EBADF;
    return NULL;
  }

  STFS_ExternalFileHandle *lp_Efh = (STFS_ExternalFileHandle *) pv_Fhandle;

  // Rev: The EHHExists() should be moved to this class
  bool lv_Found = STFS_util::EFHExists(lp_Efh);
  if (!lv_Found) {
    errno = EBADF;
    return NULL;
  }

  if (!lp_Efh->IsEyeCatcherValid()) {
    STFS_util::SoftwareFailureHandler(WHERE);
    return NULL;
  }

  return lp_Efh;
}

////////////////////////////////////////////////////////////////////////////////
//  EfmGet()
////////////////////////////////////////////////////////////////////////////////

STFS_ExternalFileMetadata* 
STFS_ExternalFileHandle::EfmGet() const
{ 
  const char       *WHERE = "STFS_ExternalFileHandle::EfmGet";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!efm_->IsEyeCatcherValid()) {
    STFS_util::SoftwareFailureHandler("STFS_ExternalFileHandle::EfmGet");
    return NULL;
  }
      
  return efm_;
}

////////////////////////////////////////////////////////////////////////////////
//  GetContainer()
////////////////////////////////////////////////////////////////////////////////

STFS_ExternalFileHandleContainer* 
STFS_ExternalFileHandle::GetContainer() 
{
  return STFS_ExternalFileHandleContainer::GetInstance();
}

///////////////////////////////////////////////////////////////////////////////
///
//          EvalCurrentFFH
///
/// \brief  Finds the current FFH
///
/// By using the file offset, This function will return the offset of the 
/// current fragment to be used and will set the current fragment value.
///
/// \param  pv_FileOffset      Overall file offset
/// \param  pv_FragmentOffset  Sets the offset of the current fragment
/// \param  pv_Whence  values are as follow: \n
///                         SEEK_SET - The offset is set to offset bytes. \n
///                         SEEK_CUR - The offset is set to its current
///                                    location plus offset bytes.        \n
///                         SEEK_END - The offset is set to the size of the 
///                                    file plus offset bytes. 
/// 
/// \retval off_t           SUCCESS:  0                                   \n
///                         ERROR:   -1
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileHandle::EvalCurrentFFH(long  pv_FileOffset,
                                        long &pv_FragmentOffset,
                                        int   pv_Whence)
{
  const char       *WHERE = "STFS_ExternalFileHandle::EvalCurrentFFH";
  STFS_ScopeTrace   lv_st(WHERE,2);

  // find the appropriate FFH
  long lv_FileOffset = 0;
  switch (pv_Whence) {
  case SEEK_SET:
    lv_FileOffset = pv_FileOffset;
    break;

  case SEEK_CUR:
    // Rev: Check whether we need to validate lv_FileOffset (going beyond STFS_SIZE_MAX)
    lv_FileOffset = currentOffset_ + pv_FileOffset;
    break;

  case SEEK_END:
    // Rev: Assert (false);
    break;

  default:
    // What new seek mode were you looking for?
    ASSERT (false); 
    return -1;
    break;
  }

  STFS_FragmentFileMetadata *lp_Ffm = 0;
  STFS_FragmentFileHandle *lp_Ffh = 0;
  for (int i = (int) efm_->GetNumFragments() - 1; i >= 0; i--) {
    lp_Ffm = efm_->GetFragment(i);

    if (lp_Ffm->OffsetGet() <= lv_FileOffset) {//TBD Sprint 1

      // found the correct fragment, get the physical offset into it
      pv_FragmentOffset = lv_FileOffset - lp_Ffm->OffsetGet();

      // Set up the FFH
      lp_Ffh = this->GetFragment(i);

      if (!lp_Ffh) {

	// need to reconcile opens versus knowns

	if (NewFFHsNeeded()== true) {

	  int lv_Stat = OpenNewFragments();
	  if ( lv_Stat < 0 ) {
	    return -1;
	  }

	  else {
	    lp_Ffh = this->GetFragment(i);
	  }
	}
      }
      break;
    }

  }

  if (lp_Ffh == NULL) {
    return -1;
  }
  currentFFH_ = lp_Ffh;
  
  return 0;
}


///////////////////////////////////////////////////////////////////////////////
///
//          STFS_ExternalFileHandle::GetExternalFilename()
///
/// \brief  Returns the file name of the associated EFM
///
/// This method returns the file name on success.
/// 
/// \retval    SUCCESS: Returns file name
///            ERROR:   NULL
///
///////////////////////////////////////////////////////////////////////////////
char *
STFS_ExternalFileHandle :: GetExternalFilename()
{
    STFS_ExternalFileMetadata *lp_Efm = this->EfmGet();
    if(!lp_Efm) {
      //TODO: Error Handling
      return NULL;
    }

    return lp_Efm->ExternalFileNameGet();
}

///////////////////////////////////////////////////////////////////////////////
///
//          HasError
///
/// \brief  Check if there are any error conditions associated with this open.
///
/// This method reports whether or not there are any error conditions
/// associated with this EFH.
/// 
/// \retval    true:  One or more error conditions were found
///            false: No error conditions are stored in this EFH
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileHandle :: HasError() {

  // love means never missing a chance to check for memory corruption
  // Rev: Could make IsEyeCatcherValid() much more efficient
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
    return false; // Housekeeping, since SigIll doesn't return...
  }

  return ( (openError_.GetNumberOfReportableErrors () ) > 0 );
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetError
///
/// \brief  Retrieves an error condition, if any, associated with this open
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
STFS_ExternalFileHandle :: GetError ( int *pp_errNo,
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

  bool status = openError_.ExtractCondition ( NULL, // condition significance
                                              pp_errNo, pp_additionErr, pp_context,
                                              pv_contextLenMax, pp_contextLenOut);

  return status;

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
STFS_ExternalFileHandle::SetError ( bool pv_errorIsHighest,
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

  bool status = openError_.PreserveCondition (lv_sig, pv_errNo, pv_addlErr,
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
void STFS_ExternalFileHandle :: ResetErrors () {

  // love means never missing a chance to check for memory corruption
  DEBUG_CHECK_EYECATCHER_VOID(this);

  openError_.ReleaseAllConditions ();
}

///////////////////////////////////////////////////////////////////////////////
///
//          ForcingFragmentCreation
///
/// \brief  Checks to see if this I/Os for EFH is arbitrarily forcing new
///         fragments to be created.
/// 
/// \retval    True if fragments are arbitrarily created
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileHandle::ForcingFragmentCreation() {

  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }

  return fragForcing_.forceFragmentCreation;

}

///////////////////////////////////////////////////////////////////////////////
///
//          SetCurrentFragmentBasedOnOffset
///
/// \brief  Sets the current fragment based on the current I/O offset
///
/// Might be a no-op if current fragment is already correct.  The
/// current fragment is not necessarily physically opened, but there
/// should already be an FFH for it.  Use OpenNewFragments first to
/// ensure that there are corresponding FFHs for any newly created
/// fragments. 
///
/// Positioning is relative to the current offset, so SEEK_SET is
/// always used as positioning specifier.
///
/// \retval TRUE       if the call changed the current fragment
/// \retval FALSE      otherwise
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileHandle::SetCurrentFragmentBasedOnCurrOffset(void) {


  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }

  long lv_FragmentOffset = 0;

  STFS_FragmentFileHandle *lv_savedCurrFFH = currentFFH_;

  short lv_Ret = EvalCurrentFFH(currentOffset_, lv_FragmentOffset, SEEK_SET);

  ASSERT (lv_Ret == 0);  // Perm assert?  Better error handling?  The
			 // only error here would be a bug where we
			 // didn't open the fragments first...

  return (lv_savedCurrFFH != currentFFH_);

}

///////////////////////////////////////////////////////////////////////////////
///
//          SeekToCurrFragmentStart()
///
///  \brief  Seeks to the beginnning of a fragment
///
///  This method unconditionally moves to the first data byte of the
///  current STFS fragment.  Used for doing sequential I/Os that span
///  fragments.
///
///  \retval  0     Fine
///  \retval <0     Error occurred
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_ExternalFileHandle::SeekToCurrFragmentStart() {

  int lv_Ret = currentFFH_->Lseek(0,SEEK_SET);
  if (lv_Ret < 0) {
    return lv_Ret;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          CurrentFFHIsLast
///
/// \brief Checks to see if the current FFH points to the last FFH
///        currently available to this EFH
///
/// Keep in mind that the last FFH in the vector might not be the
/// actual last fragment.  If the EFH is for a read-only open, then
/// there might be more FFMs in the associated EFM.  Further, if this
/// process isn't the one that owns write access, the owning STFSd
/// might have created additional fragments.
///
/// \retval true       if this is the last FFH in the ffhVector_
///         false      if there are more FFH's in the ffhVector_
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileHandle::CurrentFFHIsLast (void) {

  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }

  return (currentFFH_ == ffhVector_.back());
}

///////////////////////////////////////////////////////////////////////////////
///
//          NewFFHsNeeded
///
/// \brief Checks to see if new FFHs are required because the EFM
///        knows about fragments created since the FFH list was last
///        refreshed. 
/// \retval true       if there are more FFMs than FFHs
///         false      if the number of FFMs matches the number of FFMs
///
///////////////////////////////////////////////////////////////////////////////
bool 
STFS_ExternalFileHandle::NewFFHsNeeded(void) {
  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }

  STFS_ExternalFileMetadata *lv_efm = EfmGet();

  int lv_efmNumFragments = lv_efm->GetNumFragments();
  int lv_efhNumFragments = ffhVector_.size();

  ASSERT (lv_efmNumFragments >= lv_efhNumFragments);
  return (lv_efhNumFragments < lv_efmNumFragments);

}

///////////////////////////////////////////////////////////////////////////////
///
//          GetLastFFMStartOffset()
///
/// \brief  Retrieve the starting offset from the last FFM
///
///////////////////////////////////////////////////////////////////////////////
size_t 
STFS_ExternalFileHandle::GetLastFFMStartOffset() {

  // this could be shortened/inlined into a single call sequence
  // without the local variables.  Right now, it's broken out to allow
  // for easy debugging...


  STFS_ExternalFileMetadata *lp_Efm = EfmGet();
  ASSERT (lp_Efm != NULL);

  int lv_numFrags = lp_Efm->GetNumFragments();

  STFS_FragmentFileMetadata *lp_lastFFM = lp_Efm->GetFragment (lv_numFrags-1);
  ASSERT (lp_lastFFM != 0);

  size_t lv_retVal = lp_lastFFM->OffsetGet();

  return lv_retVal;

}
///////////////////////////////////////////////////////////////////////////////
///
//          OpenNewFragments
///
/// \brief  Does Logical fragment opens for any fragments that are not
///         currently open.
///
/// This method checks to see if the number of fragments in the
/// metadata matches the number of fragments in the EFH's FFH vector.
/// If they match, then the method returns.  If they don't match, then
/// the missing fragments are logically opened (i.e, FFHs are created
/// and placed in the vector).  They're not physically opened until
/// they're actually used.
///
/// \param  void
///
/// \retval  <0 indicates an error in opening one or more fragments
///                (usually space errors)
/// \retval   0 indicates success
///////////////////////////////////////////////////////////////////////////////
int
STFS_ExternalFileHandle::OpenNewFragments(void) {

  ////////////////////////////////////
  /// Housekeeping and parameter checks
  ////////////////////////////////////

  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }

  ////////////////////////////////////
  ///  Get the EFM
  ////////////////////////////////////
  STFS_ExternalFileMetadata *lv_efm = EfmGet();

  if (lv_efm == NULL) {
    return -1;
  }

  ////////////////////////////////////
  /// Compare EFM number of fragments to EFH number of fragments
  ////////////////////////////////////

  int lv_efmNumFragments = lv_efm->GetNumFragments();
  int lv_efhNumFragments = ffhVector_.size();

  if (lv_efhNumFragments < lv_efmNumFragments ) {

    ////////////////////////////////////
    /// Loop over missing fragments, opening them
    ////////////////////////////////////

    //fragments are always opened sequentially.  There are no "holes"
    //where we've opened fragment 1 and then fragment 3, but skipped
    //logically opening fragment 2.  So we'll start with the next
    //fragment to be opened and go up to the number of fragments the
    //EFM thinks we should have

    for (short i = lv_efhNumFragments; i < lv_efmNumFragments; i++) {

      ////////////////////////////////////
      /// Get the fragment
      ////////////////////////////////////

      STFS_FragmentFileMetadata *lv_ffm = lv_efm->GetFragment (i);


      ////////////////////////////////////
      /// Allocate the FFH
      ////////////////////////////////////

      STFS_FragmentFileHandle *lv_ffh = new STFS_FragmentFileHandle(lv_ffm);

      if (lv_ffm == NULL) {
	// Memory allocation error!
	return -1;
      }

      ////////////////////////////////////
      /// Open the fragment
      ////////////////////////////////////

      int lv_ret = lv_ffh->Open(this->openFlags_);
      if (lv_ret < 0) {
	// FFH open error
	return -1;
      }

      ////////////////////////////////////
      /// Insert the fragment into the vector
      ////////////////////////////////////

      lv_ret = InsertFFH (lv_ffh);
      if (lv_ret <0) {
	/// Insert error
	return -1;
      }

      ////////////////////////////////////
      ///End of loop over unopened fragments
      ////////////////////////////////////

    } // for loop

  }// If numFFHs < numFFMs

  ////////////////////////////////////
  /// Finis!
  ////////////////////////////////////

  return 0;
}


///////////////////////////////////////////////////////////////////////////////
///
//          IsCurrFragmentLocal
///
/// \brief  Checks to see if the current ffh is local to the node
/// 
/// \retval  1:    Current ffh resides on the local node
///          0:    Current ffh does not reside on the local node
///         -1:    Error has occurred
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_ExternalFileHandle::IsCurrFragmentLocal() {

  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }

  long lv_FragmentOffset = 0;
  // find the appropriate FFH
  if (!currentFFH_) {
    if (EvalCurrentFFH(currentOffset_, lv_FragmentOffset) < 0) {
      errno = EBADF;
      SetError (true, errno, 0, NULL, 0);
      return -1;
    }
  }
  
  if(currentFFH_->IsLocal()) {
    return 1;
  }
  
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          ForcingPartialWrite
///
/// \brief  Checks to see if this I/Os for EFH should be split arbitrarily when
///           forcing extra fragment creation.  
/// 
/// \retval  True if fragments are arbitrarily split (caller picks split location)
/// \retval  False if not forcing fragment creation, or if extra fragments are
///            to be created only on write boundaries.
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileHandle::ForcePartialWrite() {

  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }

  // the only way partial writes should be true is if we're alreadying forcing
  // fragment creation.  Verify that here as an assertion, because it won't
  // matter when we get around to doing writes. Catch bugs early, catch bugs often.

  ASSERT ( ( fragForcing_.partialWrites == false )      || 
	   ( fragForcing_.forceFragmentCreation == true )   );

  return fragForcing_.partialWrites;

}

///////////////////////////////////////////////////////////////////////////////
///
//          CheckForNewFragRequired()
///
/// \brief  Checks to see if the number of writes equals the max pecified in the 
///            user's envvar.   Should not be called if not forcign fragment
///            creation.  Does not reset the number of writes!
///
/// 
/// \retval  True if the number of writes equals the max
/// \retval  false if not forcing fragment creation, or if the number of writes
///             is less than the max.
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileHandle::CheckForNewFragRequired() {

  // love means never missing a chance to check for memory corruption
  // Rev: Maybe we dont need to check eye catcher if this method is provate.
  // Or we could just do ASSERT(IsEyeCatcherValid())
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }

  if (  fragForcing_.forceFragmentCreation == true ) {
    return ( fragForcing_.maxCounter >= fragForcing_.writesSinceLastFragCreate);
  }
  else {
    return false;
  }

}

///////////////////////////////////////////////////////////////////////////////
///
//          ResetForceNewFragCounter
///
/// \brief  Resets the force fragment write counter to 0.
/// 
/// \retval  void
///
///////////////////////////////////////////////////////////////////////////////
void
STFS_ExternalFileHandle::ResetForceNewFragCounter() {

  // love means never missing a chance to check for memory corruption
  if (!IsEyeCatcherValid() == true) {
    STFS_SigIll();
  }
  
 fragForcing_.writesSinceLastFragCreate = 0;

}


///////////////////////////////////////////////////////////////////////////////
//   ----------------
//   STFS_FragmentFileHandle methods
//   ----------------
///////////////////////////////////////////////////////////////////////////////
STFS_FragmentFileHandle::STFS_FragmentFileHandle():
  STFS_Root(STFS_Root::EC_FragmentFileHandle)
{
  fs_ = new STFS_ext2();
  ffm_ = 0;
}

STFS_FragmentFileHandle::STFS_FragmentFileHandle(STFS_FragmentFileMetadata *pp_Ffm):
  STFS_Root(STFS_Root::EC_FragmentFileHandle)
{
  const char       *WHERE = "STFS_FragmentFileHandle::STFS_FragmentFileHandle ctor 2";
  STFS_ScopeTrace   lv_st(WHERE,2);

  fs_ = new STFS_ext2(pp_Ffm->NameGet());
  ffm_ = pp_Ffm;
}

STFS_FragmentFileHandle::~STFS_FragmentFileHandle()
{
  delete fs_;
}

bool
STFS_FragmentFileHandle::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid(STFS_Root::EC_FragmentFileHandle);
}

///////////////////////////////////////////////////////////////////////////////
///
//          IsOpen
///
/// \brief  Evaluates whether fragment file is open
///
/// \retval bool    True if file is open, False if not open.
///
///////////////////////////////////////////////////////////////////////////////
bool 
STFS_FragmentFileHandle::IsOpen() const 
{
  if (!fs_) {
    return false;
  }

  return fs_->IsOpen();
}

///////////////////////////////////////////////////////////////////////////////
///
//              Open
///
/// \brief      Opens fragment file
///
/// \param[in]  pv_oflag    File status flags
/// 
/// \retval off_t           SUCCESS:  0                                   \n
///                         ERROR:   -1 or errno
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_FragmentFileHandle::Open(int pv_OFlag)
{
  const char       *WHERE = "STFS_FragmentFileHandle::Open";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!fs_) {
    return -1;
  }

  if (this->IsOpen()) {
    return 0;
  }

  return fs_->open(pv_OFlag);
}

///////////////////////////////////////////////////////////////////////////////
///
//              Read
///
/// \brief      Reads from a fragment file
///
/// \param[in,out]  *pp_Buf    read buffer
///  \param[in]      pv_count  value between zero and SSIZE_MAX.
/// 
///  \retval      int       SUCCESS: The number of bytes read is returned, the 
///                                  file position is advanced by this number.\n
///                                  Zero indicates end of file\n\n
///                         ERROR:   -1 is returned and errno is set. 
///
///////////////////////////////////////////////////////////////////////////////
long 
STFS_FragmentFileHandle::Read(char   *pp_Buf, 
                              size_t  pv_Count)
{
  const char       *WHERE = "STFS_FragmentFileHandle::Read";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!pp_Buf) {
    return -1;
  }

  if (!fs_) {
    return -1;
  }

  if (!fs_->IsOpen()) {
    return -1;
  }

  return fs_->read(pp_Buf, pv_Count);
}

///////////////////////////////////////////////////////////////////////////////
///
//              Stat 
///
/// \brief      Gathers statistical information on a file
///
/// \param[in]  *pp_Buf     struct stat, holds statistical info
/// 
/// \retval off_t           SUCCESS:  0                                   \n
///                         ERROR:   -1 is returned and errno is set
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_FragmentFileHandle::Stat(struct stat *pp_Buf)
{
  const char       *WHERE = "STFS_FragmentFileHandle::Stat";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!pp_Buf) {
    return -1;
  }

  return fs_->stat(pp_Buf);
}


///////////////////////////////////////////////////////////////////////////////
///
//              Write
///
/// \brief      Writes to a fragment file handle
///
///  \param[in]   *pp_Data  buffer with the write contents
///  \param[in]    pv_Count value between zero and SSIZE_MAX
/// 
///  \retval      ssize_t   SUCCESS: Returns the number of bytes written\n
///                                  A Zero indicates nothing was written\n\n
///                         ERROR:   -1 is returned and errno is set
///
///////////////////////////////////////////////////////////////////////////////
long 
STFS_FragmentFileHandle::Write(const void *pp_Data, 
                               long        pv_Count)
{
  const char       *WHERE = "STFS_FragmentFileHandle::Write";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!pp_Data) {
    return -1;
  }

  if (!fs_) {
    return -1;
  }

  if (!fs_->IsOpen()) {
    return -1;
  }

  return fs_->write(pp_Data, pv_Count);
}

///////////////////////////////////////////////////////////////////////////////
///
//              Lseek
///
/// \brief      Reposition read/write file offset
///
/// \param[in]  pv_Offset   byte offset count based on whence
/// \param[in]  pv_Whence   SEEK_SET, SEEK_CUR, or SEEK_END
/// 
/// \retval off_t           SUCCESS: The resulting offset location from the 
///                                  beginning of the file.\n\n
///                         ERROR:   -1 is returned and errno is set.
///
///////////////////////////////////////////////////////////////////////////////
off_t 
STFS_FragmentFileHandle::Lseek(off_t pv_Offset, 
                               int   pv_Whence)
{
  const char       *WHERE = "STFS_FragmentFileHandle::Lseek";
  STFS_ScopeTrace   lv_st(WHERE,2);

  //check if pv_Offset is larger than supported size
  if ((unsigned long) pv_Offset > STFS_SSIZE_MAX) {
    return -1;
  }

  if (!fs_) {
    return -1;
  }

  if (!fs_->IsOpen()) {
    return -1;
  }

  return fs_->lseek(pv_Offset, pv_Whence);
}

///////////////////////////////////////////////////////////////////////////////
///
//          Close
///
///  \brief Closes the FFH
///
/// \retval int SUCCES:   0
///             FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_FragmentFileHandle::Close()
{
  const char       *WHERE = "STFS_FragmentFileHandle::Close";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!fs_) {
    return 0;
  }

  if (!fs_->IsOpen()) {
    return 0;
  }

  return fs_->close();
}

///////////////////////////////////////////////////////////////////////////////
///
//          Unlink
///
///  \brief Unlinks the FFH
///
/// \retval int SUCCES:   0
///             FAILURE: -1 
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_FragmentFileHandle::Unlink()
{
  const char       *WHERE = "STFS_FragmentFileHandle::Unlink";
  STFS_ScopeTrace   lv_st(WHERE,2);

  if (!fs_) {
    return 0;
  }

  if (fs_->IsOpen()) {
    return -1;
  }

  return fs_->unlink();
}

///////////////////////////////////////////////////////////////////////////////
///
//                  IsLocal
///
/// \brief          Used to determine whether fragment file is on local node
///
/// \retval bool    True if fragment file is on local node, False if not.        
///
///////////////////////////////////////////////////////////////////////////////
bool 
STFS_FragmentFileHandle::IsLocal()
{
  int lv_MyNodeId = STFS_util::GetMyNodeId();
  int lv_FfmNodeId = ffm_->NodeIdGet();

  return (lv_MyNodeId == lv_FfmNodeId);
}

///////////////////////////////////////////////////////////////////////////////
///
//                  ChdirToFragmentDir
///
/// \brief          Changes the current dir of the current process to the 
///                 fragment's directory
///
/// \retval int    SUCCESS: 0                              \n
///                FAILURE: -1 is returned and errno is set.
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_FragmentFileHandle::ChdirToFragmentDir()
{
  const char       *WHERE = "STFS_FragmentFileHandle::ChdirToFragmentDir";
  STFS_ScopeTrace   lv_st(WHERE,3);

  int lv_Ret = 0;

  if (STFS_util::GetMyNodeId() != ffm_->NodeIdGet()) {
    return -1;
  }

  STFS_ExternalFileMetadata *lp_Efm = ffm_->EFMGet();

  lv_Ret = chdir(ffm_->StfsDirectoryGet());
  if (lv_Ret < 0) {
    errno = errno;
    return -1;
  }

  lv_Ret = chdir(lp_Efm->ExternalFileNameGet());
  if (lv_Ret < 0) {
    errno = errno;
    return -1;
  }

  return lv_Ret;
}


///////////////////////////////////////////////////////////////////////////////
//  ----------------------------------------
//  STFS_ExternalFileHandleContainer methods
//  ----------------------------------------
///////////////////////////////////////////////////////////////////////////////

//Rev: Move the comment
///////////////////////////////////////////////////////////////////////////////
///
//         GetInstance
///
/// \brief Creates an efh Container
///
///Creates an efh Container if one is not already present. 
/// 
/// \retval STFS_ExternalFileHandleContainer
///                SUCCESS:  Returns a pointer              \n
///                ERROR:    Returns NULL
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileHandleContainer::IsEyeCatcherValid()
{
  return STFS_Root::IsEyeCatcherValid(STFS_Root::EC_ExternalFileHandleContainer);
}

STFS_ExternalFileHandleContainer*
STFS_ExternalFileHandleContainer::GetInstance() 
{
  if (STFS_ExternalFileHandleContainer::EfhContainer_) {
    return STFS_ExternalFileHandleContainer::EfhContainer_;
  }
  else {
    STFS_ExternalFileHandleContainer::EfhContainer_ = new STFS_ExternalFileHandleContainer();
  }

  if (!STFS_ExternalFileHandleContainer::EfhContainer_) {
    // Rev: Do we need to do something here.
    return NULL;
  }
    
  return STFS_ExternalFileHandleContainer::EfhContainer_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Insert
///
/// \brief  Inserts efh into efh Container
///
/// Inserts efh into efh Container
/// 
/// \retval short SUCCESS: 0 or return code for create();
///               FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileHandleContainer::Insert(STFS_ExternalFileHandle *pp_Efh) 
{
  const char       *WHERE = "STFS_ExternalFileHandleContainer::Insert";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = 0;

  // Rev: Add an Assert
  if (!pp_Efh) {
    return -1;
  }

  Lock();
  try {
    efhVector_.push_back(pp_Efh);
  }
  catch (...) {
    Unlock();
    // Rev: Add EV Logging here
    // Rev: Set errno (Use SetErrorNum())
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }
  Unlock();

  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Delete
///
/// \brief  Deletes efh from efh Container
///
/// Deletes efh from the efh Container
/// 
/// \retval short SUCCESS: 0 or return code for create();
///               FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
short
STFS_ExternalFileHandleContainer::Delete(STFS_ExternalFileHandle *pp_Efh) 
{
  const char       *WHERE = "STFS_ExternalFileHandleContainer::Delete";
  STFS_ScopeTrace   lv_st(WHERE,2);

  short lv_RetCode = -1;

  // Rev: Assert
  if (!pp_Efh) {
    return -1;
  }

  // Rev: Blanket other usage of STL with a try/catch block
  Lock();
  try {
    efhVector_Def::iterator it;
    for (it = efhVector_.begin();
         it != efhVector_.end(); 
         it++) {
      if (pp_Efh == *it) {
        efhVector_.erase(it);
        lv_RetCode = 0;
        break;
      }
    }
  }
  catch (...) {
    Unlock();
    TRACE_PRINTF1(1,"In the catch-all exception handler\n");
    return -1;
  }
  
  Unlock();
  return lv_RetCode;
}

///////////////////////////////////////////////////////////////////////////////
///
//         Exists
///
/// \brief Checks that an efh exists
///
/// Checks if the passed in efh exists in the efh Container
/// 
/// \param STFS_ExternalFileHandle Pointer to efh 
/// 
/// \retval bool  If passed in efh exists, returns true             \n
///               If passed in efh does not exist, returns false
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_ExternalFileHandleContainer::Exists(STFS_ExternalFileHandle *pp_Efh) 
{
  const char       *WHERE = "STFS_ExternalFileHandleContainer::Exists";
  STFS_ScopeTrace   lv_st(WHERE,2);

  bool lv_Found = false;

  // Rev: Assert
  if (!pp_Efh) {
    return false;
  }

  // Rev: Blanket with a try/catch
  STFS_ExternalFileHandle *lp_CurrEfh = 0;
  for (short i = 0; i < (int) efhVector_.size(); i++) {
    lp_CurrEfh = efhVector_.at(i);
    if (lp_CurrEfh == pp_Efh) {
      lv_Found = true;
    }
  }

  return lv_Found;
}

///////////////////////////////////////////////////////////////////////////////
///
//         Size
///
/// \brief Returns the size of efhContainer
///
/// Returns the size of efhContainer
/// 
/// \retval long SUCCESS: Size of efhContainer \n
///              FAILURE: 0 if efhContainer does not exist 
///
///////////////////////////////////////////////////////////////////////////////
long
STFS_ExternalFileHandleContainer::Size() 
{
  // Rev: Blanket with a try/catch
  return efhVector_.size();
}

STFS_ExternalFileHandleContainer::STFS_ExternalFileHandleContainer():
  STFS_Root(EC_ExternalFileHandleContainer)
{}

