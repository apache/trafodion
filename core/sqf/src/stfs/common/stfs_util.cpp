///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_util.cpp
///  \brief   Implementation of STFS_util Methods
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

#include "stfs_util.h"
#include "stfs_sigill.h"

#include "seabed/ms.h"
#include "seabed/fs.h"

using namespace STFS;

char  STFS_util::currentDir_[512];
int   STFS_util::currentNodeId_ = -1;
long  STFS_util::currentOpenId_ = 0;
short STFS_util::initDone_ = 0;
int   STFS_util::myPID_ = -1;
short STFS_util::traceLevel_ = 0;
bool  STFS_util::executingInDaemon_ = false;
bool  STFS_util::seaquestFilesystemInitialized_ = false;

static int SQInit();

// Rev: Move to the EFH class
///////////////////////////////////////////////////////////////////////////////
///
//          EFHExists
///
/// \brief  Checks if Efh exists in container
///
/// Checks if Efh exists in container.  If exists, will return true, 
/// otherwise will return false.
///
/// \param pp_Efh   EFH to search for
/// 
/// \retval bool    If EFH exists in container:         True
///                 If EFH does not exist in container: False
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_util::EFHExists(STFS_ExternalFileHandle *pp_Efh)
{
  const char     *WHERE = "STFS_util::EFHExists";
  STFS_ScopeTrace lv_st(WHERE,2);

  if (!pp_Efh) {
    return false;
  }

  bool lv_found = false;
  STFS_ExternalFileHandleContainer *lp_efh_container = STFS_ExternalFileHandle::GetContainer();
  if (!lp_efh_container) {
    // Rev: Use tracing & call Software Failure Handler
    std::cout << "Null EFH Container" << std::endl;
    //cleanup
    return false;
  }

  lv_found = lp_efh_container->Exists(pp_Efh);

  return lv_found;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GenerateExternalFilename
///
/// \brief  Generates STFS suffix and appends to passed in filename
///
/// Generates STFS suffix and appends it to user-specified filename. 
/// Checks whether filename passed in is too long. 
///
/// \param[in,out]  pp_FileName 
/// 
/// \retval int     SUCCESS: 0 
///                 Failure: -1
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_util::GenerateExternalFilename(char *pp_FileName) 
{
  const char     *WHERE = "STFS_util::GenerateExternalFilename";
  STFS_ScopeTrace lv_st(WHERE,2);

  struct tm     *lp_tx;
  struct tm      lv_tx;
  struct timeval lv_t;
  int            lv_ms;
  int            lv_us;

  // Rev: Remove this
  STFS_util::Init();

  //Rev: do this as the first thing in this function
  if (!pp_FileName) { 
    return -1;
  }

  // Rev check the return val
  gettimeofday(&lv_t, NULL);
  // Rev check the return val
  lp_tx = localtime_r(&lv_t.tv_sec, &lv_tx);
  lv_ms = (int) lv_t.tv_usec / 1000; 
  lv_us = (int) lv_t.tv_usec - lv_ms * 1000;

  // Rev: Move it before we call time functions
  if (strlen(pp_FileName) > (STFS_NAME_MAX - STFS_NAME_SUFFIX_MAX)) {
    return -1;
  }

  char stfs_suffix[STFS_NAME_SUFFIX_MAX];
  memset(stfs_suffix, 0, STFS_NAME_SUFFIX_MAX);
  
  sprintf(stfs_suffix, "_stfs_%04d_%04d%02d%02d_%02d%02d%02d_%03d%03d", 
          STFS_util::GetMyNodeId(),
          (lp_tx->tm_year)+1900,           
          (lp_tx->tm_mon)+1, 
          lp_tx->tm_mday, 
          (lp_tx->tm_hour)%24, 
          lp_tx->tm_min, 
          lp_tx->tm_sec,
          lv_ms,
          lv_us);
  strcat(pp_FileName, stfs_suffix);

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetMountPointName
///
/// \brief  Returns a mount point name for the current node
///
/// \param  pp_mountPointName   String with the name of the mountpoint
/// \param  pp_StorageLocation  The location for the required mountpoint.  Might
///                             be null if default local storage location is to
///                             be used.
///
/// \retval TRUE if there was a mountpoint returned
///         FALSE if no mountpoint available. pp_mountPointName unchanged.
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_util::GetMountPointName (char *pp_MountpointName, 
			      char *pp_StorageLocation) {

  // Compiler appeasement to ensure that we reference pp_StorageLocation even
  // though we don't need it quite yet.  We'll need it later when we support
  // external storage.

  if (pp_StorageLocation == NULL) {
    // use default local storage node
    ASSERT (true);
  }

  // Use currentDir_ with a front slash appended for now.  Later, we'll need to
  // make this more dynamic (based on the node id (e.g.) & storage loc)

  sprintf (pp_MountpointName, "%s/", STFS_util::currentDir_);

  return true;
}


///////////////////////////////////////////////////////////////////////////////
///
//          GetLocalNodeStorageLocation
///
/// \brief  Returns the directory storage location
///
/// \retval char *  Directory storage location 
///
///////////////////////////////////////////////////////////////////////////////
char *
STFS_util::GetLocalNodeStorageLocation()
{
  const char     *WHERE = "STFS_util::GetLocalNodeStorageLocation";
  STFS_ScopeTrace lv_st(WHERE,2);

  // Rev: Remove this now
  STFS_util::Init();

  // Rev: Use a directory based on the node id (e.g. stfsl<node id>)
  //TBD - For Sprint 1, just return the current directory
  return STFS_util::currentDir_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetMyNodeId
///
/// \brief  Gets the node ID for the current object
///
/// \retval int  Node ID
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_util::GetMyNodeId() 
{
  const char     *WHERE = "STFS_util::GetMyNodeId";
  STFS_ScopeTrace lv_st(WHERE,2);

  if (STFS_util::currentNodeId_ == -1) {
    if (STFS_util::seaquestFilesystemInitialized_) {
      msg_mon_get_process_info((char *) "",
                               &STFS_util::currentNodeId_,
                               &STFS_util::myPID_);
    }
    else {
      STFS_util::currentNodeId_ = 0;
    }
  }
  
  return STFS_util::currentNodeId_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetMyPID
///
/// \brief  Gets the PID for the current object
///
/// \retval int  PID
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_util::GetMyPID() 
{
  const char     *WHERE = "STFS_util::GetMyPID";
  STFS_ScopeTrace lv_st(WHERE,2);

  if (STFS_util::myPID_ == -1) {
    if (STFS_util::seaquestFilesystemInitialized_) {
      msg_mon_get_process_info((char *) "",
                               &STFS_util::currentNodeId_,
                               &STFS_util::myPID_);
    }
    else {
      // TBD - is this ok as monitor's PID
      STFS_util::myPID_ = 0;
    }
  }
  
  return STFS_util::myPID_;
}

///////////////////////////////////////////////////////////////////////////////
///
//          GetOpenId
///
/// \brief  Gets the OpenID for the current object
///
/// \retval int  
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_util::GetOpenId(STFS_OpenIdentifier& pv_OpenId)
{
  // Rev: Assuming that STFS_util::GetMyPID() has been called
  pv_OpenId.sqOwningDaemonNodeId = currentNodeId_;
  // Rev: There could be a problem in a 'longevity' test and
  // if a file at a low openid number is still open 
  currentOpenId_++;
  pv_OpenId.openIdentifier = currentOpenId_;

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          Init
///
/// \brief  Initializes STFS
///
/// Initializes STFS by cleaning up storage directory, Getting Node ID.
/// Setting trace level.
///
///////////////////////////////////////////////////////////////////////////////
void
STFS_util::Init(bool pv_ExecutingInDaemon)
{
  const char     *WHERE = "STFS_util::Init";
  STFS_ScopeTrace lv_st(WHERE,2);

  if (STFS_util::initDone_ != 0) {
    return;
  }

  // seed our random number generator

  srand (time (NULL));

  STFS_util::executingInDaemon_ = pv_ExecutingInDaemon;

  if (!STFS_util::executingInDaemon_) {
    SQInit();
  }

  if (STFS_util::GetMyNodeId() < 0) {
    //TBD evlog error message
    exit(1);
  }

  if (getcwd(STFS_util::currentDir_,
             sizeof(STFS_util::currentDir_)) == NULL) {
    //TBD evlog error message
    exit(2);
  }

  char *lp_EnvTraceLevel = getenv("STFS_TRACE");
  short lv_EnvTraceLevel = 0;
  if (lp_EnvTraceLevel) {
    lv_EnvTraceLevel = (short) atoi(lp_EnvTraceLevel);
  }
  if (lv_EnvTraceLevel <= 0) {
    STFS_util::traceLevel_ = 0;
  }
  else {
    STFS_util::traceLevel_ = lv_EnvTraceLevel;
  }

  if (STFS_util::traceLevel_ > 0) {
    if (! trace_get_fd()) { // check if there is already a trace file 

      char *lp_TraceFileName = getenv("STFS_TRACE_FILE");
      char lv_TraceFileName[512];

      if (lp_TraceFileName && lp_TraceFileName[0]) {
	memset(lv_TraceFileName, 0, sizeof(lv_TraceFileName));
	strncpy(lv_TraceFileName, lp_TraceFileName, sizeof(lv_TraceFileName) - 1);
      }
      else {
	strcpy(lv_TraceFileName, "stfs");
	if (STFS_util::executingInDaemon_) {
	  strcat(lv_TraceFileName, "d");
	}
	strcat(lv_TraceFileName, "_trace");
      }

      trace_init(lv_TraceFileName,
		 true, 
		 (char *) "stfs", 
		 false);
    }
  }

#ifdef SQ_STFSD
  if (STFS_util::executingInDaemon_) {
#endif
    if(STFS_util::CreateDir() < 0) {
      //Rev: generate an evlog error message
      exit(3);
    }

    char lv_buf[256];
    sprintf(lv_buf, "STFS Initialized on Node: %d\n", STFS_util::GetMyNodeId());
    TRACE_PRINTF1(1, lv_buf);
    STFS_util::STFS_evlog(STFS_EVENT_STARTUP,
			  SQ_LOG_INFO,
			  (char *) &lv_buf);
#ifdef SQ_STFSD
  }
#endif
  
  atexit(STFSd_exit);

  initDone_ = 1;
  return;
}

///////////////////////////////////////////////////////////////////////////////
///
//          SoftwareFailureHandler
///
/// \brief  Method to handle Software Failure
///
/// \param  pp_Where  Function that experienced a software failure
/// 
///////////////////////////////////////////////////////////////////////////////
void 
STFS_util::SoftwareFailureHandler(const char * pp_Where)
{
  const char     *WHERE = "STFS_util::SoftwareFailureHandler";
  STFS_ScopeTrace lv_st(WHERE,2);

  //Rev: This shouldn't have to be called in here
  STFS_util::Init();

  // Rev: Have some variation if called from STFSLib space (using executingInDaemon_) 
  TRACE_PRINTF2(1,"Software Failure Handler called by: %s\n", pp_Where);

  char lv_buf[256];
  // Rev: Put pp_Where in the lv_buf
  sprintf(lv_buf, "STFS Software Failure on Node: %d\n", STFS_util::GetMyNodeId());
  STFS_util::STFS_evlog(STFS_EVENT_SOFTWARE_FAILURE,
			SQ_LOG_ERR,
			(char *) &lv_buf);
  
  if (!STFS_util::executingInDaemon_) {
    abort();
  }
}

// Rev: Move it to the EFM class (and give it a different name)
///////////////////////////////////////////////////////////////////////////////
///
//          ValidateEFM
///
/// \brief  Validates whether EFM exists
///
/// \param  lp_efm  EFM 
/// 
/// \retval bool    If valid, returns true.
///                 If not valid, returns false.
///
///////////////////////////////////////////////////////////////////////////////
bool
STFS_util::ValidateEFM(STFS_ExternalFileMetadata *lp_efm)
{
  const char     *WHERE = "STFS_util::ValidateEFM";
  STFS_ScopeTrace lv_st(WHERE,2);

  if (!lp_efm) {
    return false;
  }
  
  size_t lv_num_fragments = lp_efm->GetNumFragments();

  TRACE_PRINTF3(2,"EFM: %s, num_ffm: %ud\n",
               lp_efm->ExternalFileNameGet(),
               lv_num_fragments);
  
  if (lv_num_fragments > 0) {
    for (short i=0; i < (int) lv_num_fragments; i++) {
      STFS_FragmentFileMetadata *lp_ffm = lp_efm->GetFragment(i);
      if (lp_ffm) {
        TRACE_PRINTF3(2,"ffm %d: %s\n",
                     i,
                     lp_ffm->NameGet());
      }
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
///
//          CompressString
///
/// \brief  Takes an expanded string and compresses it
///
///  This method takes a string that can be up to STFS_NAME_MAX bytes and
///  stores it in a format that allows it to be shorter.  This format is a
///  2-byte length indicator, followed by the string itself, including a null
///  terminator. The length indicator includes the null.  It does not include
///  any padding necessary to round to a word boundary.
///
///  At a successful end, pp_tgtLocation looks like this:
///     nnsssssss...ss0
///     
///
///
/// \param  *pp_ExpandedString      The full string to be compressed
/// \param  *pp_tgtLocation         Where to put the compressed name
/// \param  *pp_tgtMaxSize          Max size of the target name
/// 
/// \retval Size of the compressed name + length indicator
///                 If not valid, returns 0
///
///////////////////////////////////////////////////////////////////////////////
size_t
STFS_util::CompressString(char *pp_expandedString,
                          char *pp_tgtLocation,
                          size_t pv_tgtMaxSize )
{
  const char       *WHERE = "STFS_util::CompressString";
  STFS_ScopeTrace   lv_st(WHERE,2);


  // Method internal to STFS; Check params by asserts
  ASSERT ((pp_expandedString != NULL) &&
          (pp_tgtLocation != NULL)      &&
          (pv_tgtMaxSize > 0)&&
          (strlen (pp_expandedString) < 32767)
           );

  char *lv_copyLocation = pp_tgtLocation;
  short lv_stringLength = 0;
  size_t lv_numBytesCopied = 0;

  lv_stringLength = (short) strlen (pp_expandedString);
  if (pv_tgtMaxSize <= (lv_stringLength + sizeof (short))) {
    TRACE_PRINTF3(1,
		  "Insufficient space for the compressed string.Strlen: %d, Space: %d",
		  lv_stringLength,
		  (int) pv_tgtMaxSize);
    return 0;
  }

  memcpy (lv_copyLocation,  &lv_stringLength, sizeof (short));

  lv_numBytesCopied += sizeof (short);
  lv_copyLocation += sizeof (short);

  strcpy ((char *) lv_copyLocation, pp_expandedString);

  lv_numBytesCopied += lv_stringLength + 1; //Add 1 to take care of the null terminator

  return (lv_numBytesCopied);

}


//**************************
///////////////////////////////////////////////////////////////////////////////
///
//          UncompressString
///
/// \brief  Takes a compressed string and uncompresses it
///
///  This method takes a string that s in compressed format and expands it up
///  the specified size.
///     
///
///
/// \param  *pp_compString             The compressed filename
/// \param  *pp_tgtExpandedString      The full file name after expansion
/// \param  *pp_tgtMaxSize             Max size of the target name
/// 
/// \retval     Size of the expanded string
///             0 if string unexpanded
///
///////////////////////////////////////////////////////////////////////////////
size_t
STFS_util::UncompressString  (char *pp_compString,
                              char *pp_tgtExpandedString,
                              size_t pv_tgtMaxSize )
{
  const char       *WHERE = "STFS_util::UncompressString";
  STFS_ScopeTrace   lv_st(WHERE,2);

  // Method internal to STFS; Check params by asserts
  ASSERT ((pp_tgtExpandedString != NULL) &&
          (pp_compString != NULL)      &&
          (pv_tgtMaxSize > 0)
           );

  short lv_stringLenFromCompString = 0;
  char *lv_copyLocation = pp_compString;

  memcpy (&lv_stringLenFromCompString, lv_copyLocation, sizeof (short));


  lv_copyLocation += sizeof (short);

  if (lv_stringLenFromCompString > (short) pv_tgtMaxSize) {
    return 0;
  }


  if ( ( (short) strlen ( lv_copyLocation) ) != lv_stringLenFromCompString) {
    // something's wrong with the message or compression!
    // Rev: Just return 0 here (let the caller decide what to do).
    TRACE_PRINTF4(1, "%s: Error: Mismatch between the stored length:%d and the actual strlen:%d \n",
		  WHERE,
		  lv_stringLenFromCompString,
		  (short) strlen(pp_tgtExpandedString));
    return 0;
  }

  strcpy (pp_tgtExpandedString, lv_copyLocation );

  return lv_stringLenFromCompString;

}


///////////////////////////////////////////////////////////////////////////////
///
//          CreateDir
///
/// \brief  Creates STFS storage directory 
///
/// Checks whether the directory already exists and removes contents if true.
/// '/stfsl<node ID>' will be created in current directory. 
///
/// \retval int     SUCCESS: 0 
///                 Failure: -1
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_util::CreateDir()
{
  const char     *WHERE = "STFS_util::CreateDir";
  STFS_ScopeTrace lv_st(WHERE,2);

  char  lv_ErrorMessage[512];
  int   lv_Ret = 0;
  DIR *lp_DirHandle = 0;
  char *lp_Directory = new char[strlen(STFS_util::currentDir_) + 
                                strlen("/stfslxxx") + 1];

  if (!lp_Directory) {
    return -1;
  }
  
  if (sprintf(lp_Directory,"%s/stfsl%03d", STFS_util::currentDir_,
      STFS_util::GetMyNodeId()) < 0) {  
    lv_Ret = -1;
    goto exit;
  } 

  lp_DirHandle = opendir(lp_Directory);
  if(lp_DirHandle != NULL) 
  {
    //Directory exists, Remove...
    if(RemoveDir(lp_Directory) <0) {
      lv_Ret = -1;
      goto exit;
    }
  } 

  lv_Ret = mkdir(lp_Directory, S_IRUSR | S_IWUSR | S_IXUSR);
  if (lv_Ret < 0) {
    if (errno != EEXIST) {
      perror(lv_ErrorMessage);
      TRACE_PRINTF2(1, "%s\n", lv_ErrorMessage);
      lv_Ret = -1;
      goto exit;
    }
    else {
      lv_Ret = 0;
    }
  }

  strcpy(STFS_util::currentDir_, lp_Directory);

 exit:
  delete [] lp_Directory;
  if (lp_DirHandle) {
    closedir(lp_DirHandle);
  }
  return lv_Ret;
}

///////////////////////////////////////////////////////////////////////////////
///
//          RemoveDir 
///
/// \brief  Removes directory contents
///
/// Removes directory contents including files, directories, and links. 
///
/// \param  pp_DirName  Directory name
/// 
/// \retval int     SUCCESS: 0 
///                 Failure: -1
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_util::RemoveDir(char* pp_DirName)
{
  const char     *WHERE = "STFS_util::RemoveDir";
  STFS_ScopeTrace lv_st(WHERE,2);

  int lv_Ret = 0;

  enum Enum_FileType {
    Directory   = 4, 
    RegularFile = 8, 
    Link        = 10
  };

  if(pp_DirName == NULL) {
    return -1;
  }

  DIR *lp_Directory;
  struct dirent *lp_Dirent;
  
  lp_Directory = opendir(pp_DirName);
  if(lp_Directory == NULL) {
    return -1;
  }

  while((lp_Dirent=readdir(lp_Directory))!=NULL)
  {
    if((strcmp(lp_Dirent->d_name,".")!=0) && (strcmp(lp_Dirent->d_name,"..")!=0))
    {
      char lp_ToDelete[STFS_DIRNAME_MAX];
      lp_ToDelete[0] = 0;
      
      if(sprintf(lp_ToDelete, "%s/%s", pp_DirName, lp_Dirent->d_name) < 0) {
        lv_Ret = -1;
        goto exit;
      }

      if(lp_Dirent->d_type == RegularFile) {
        if(unlink(lp_ToDelete) < 0) {
          lv_Ret = -1;
          goto exit;
        }
      }
      else if(lp_Dirent->d_type == Directory) {
         if(rmdir(lp_ToDelete) < 0) {
           //If rmdir fails, Directory has contents. Delete contents. 
           STFS_util::RemoveDir(lp_ToDelete);
           //Retry rmdir after contents are deleted
           if(rmdir(lp_ToDelete) < 0) {
             lv_Ret = -1;
             goto exit;
           }
         }
         else {
           //Directory is empty
           continue; 
         }
      } else if (lp_Dirent->d_type == Link) {
        if(unlink(lp_ToDelete) < 0) {
          lv_Ret = -1;
          goto exit;
        }
      }
    }
  }

 exit:
  closedir(lp_Directory);
  return lv_Ret;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_util::STFS_openers()
///
/// \brief  Gets the openers of STFS files
///
/// Obtains opener information for STFS files.  Returns information about an 
/// STFS file's openers, that is, the processes which have a particular STFS 
/// file open. Wildcards are not supported.   
///
/// \param  pv_nid       Node ID
/// \param  pp_path      Set to the path of the file whose opener info is returned
/// \param  pp_previndex  Index of openers
/// \param  pv_openernid  Node ID of the opener process
/// \param  pv_openerpid  Process ID of opener process
/// \param  pp_openername Process name of opener process
/// 
/// \retval int     Opener information is returned: 0
///                 No more openers:                1
///                 Failure:                       -1
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_util::STFS_openers(stfs_nodeid_t     pv_nid,      
                        char             *pp_path,    
                        long             *pp_previndex,   
                        stfs_nodeid_t    &pv_openernid,  
                        pid_t            &pv_openerpid, 
                        char             *pp_openername)
{
  const char     *WHERE = "STFS_util::STFS_openers";
  STFS_ScopeTrace lv_st(WHERE,2);

  if((!pp_previndex) || (*pp_previndex < 0)) {
    errno = EINVAL;
    return -1;
  }
  
  if(pv_nid < 0) {

    if(pp_path == NULL)
    {
      errno = EINVAL;
      return -1;
    }
  }
  else {
    //TODO: Check Node Validity
  }

  //Get openercontainer instance
  STFS_ExternalFileOpenerContainer *lp_EfoContainer = STFS_ExternalFileOpenerContainer::GetInstance();
  if (!lp_EfoContainer) {
    TRACE_PRINTF1(1,"Error while allocating STFS_OpenIdentifier\n");
    // TBD Cleanup
    return -1;
  }  

  //previndex is specfied.  Get next position after previndex
  long lv_CurrIndex = *pp_previndex + 1;
    
 
  if(pv_nid <0) {
    //filename specified
    if(lv_CurrIndex > lp_EfoContainer->Size(pp_path)) {
      //Requested index is past the container size. 
      return 1;
    }

    STFS_ExternalFileOpener *lp_EfOpener = lp_EfoContainer->Get(pp_path, (lv_CurrIndex-1));
    if (!lp_EfOpener) {
      TRACE_PRINTF1(1,"Error while getting STFS_ExternalFileOpener\n");
      // TBD Cleanup
      return -1;
    }
    pv_openernid = lp_EfOpener->sqOpenerNodeId_;
    pv_openerpid = lp_EfOpener->sqOpenerPID_;
  }
  else
  {
    if(lv_CurrIndex > lp_EfoContainer->Size()) {
      //Requested index is past the container size. 
      return 1;
    }

    STFS_ExternalFileOpenerInfo *lp_EfOpenerInfo =
    lp_EfoContainer->Geti(lv_CurrIndex-1);
    if (!lp_EfOpenerInfo) {
      TRACE_PRINTF1(1,"Error while getting STFS_ExternalFileOpenerInfo\n");
      // TBD Cleanup
      return -1;
    }
  
    STFS_ExternalFileMetadata *lp_EFM = lp_EfOpenerInfo->efm_;
    if (!lp_EFM ) {
      TRACE_PRINTF1(1,"Error while allocating STFS_ExternalFileMetadata\n");
      // TBD Cleanup
      return -1;
    }
       
    pp_path = lp_EFM->ExternalFileNameGet();
    pv_openernid = lp_EfOpenerInfo->efo_.sqOpenerNodeId_;
    pv_openerpid = lp_EfOpenerInfo->efo_.sqOpenerPID_;
  }

 *pp_previndex = lv_CurrIndex;
  strcpy(pp_openername, "TBD");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_util::STFS_fopeners()
///
/// \brief  Gets the openers for the specified Fhandle
///
/// Obtains opener information for STFS files.  Specifies Fhandle of file.
/// Returns information about an STFS file's openers, that is, the processes 
/// which have a particular STFS file open. Wildcards are not supported.   
///
/// \param  pv_Fhansle    File Handle of STFS file to obtain information returned 
///                          from STFS_open or STFS_mkstemp
/// \param  pp_previndex  Index of openers
/// \param  pv_openernid  Node ID of the opener process
/// \param  pv_openerpid  Process ID of opener process
/// \param  pp_openername Process name of opener process
/// \param  pp_OpenPath   Pathname of the STFS file
/// 
/// \retval int     Opener information is returned: 0
///                 No more openers:                1
///                 Failure:                       -1
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_util::STFS_fopeners( stfs_fhndl_t     pv_Fhandle,      
                          long            *pp_PrevIndex,    
                          stfs_nodeid_t   &pv_OpenerNid,  
                          pid_t           &pv_OpenerPid,  
                          char            *pp_OpenerName, 
                          char            *pp_OpenPath)
{
  const char     *WHERE = "STFS_util::STFS_fopeners";
  STFS_ScopeTrace lv_st(WHERE,2);
 
  if(*pp_PrevIndex < 0) {
    return -1;
  }
  if(pv_Fhandle < 0) {
    return -1;
  }

  STFS_ExternalFileHandle *lp_EFH = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
  if(lp_EFH == NULL)
  {
    TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileHandle\n");
    return -1;
  }

  STFS_ExternalFileMetadata *lp_EFM = lp_EFH->EfmGet();
  if(lp_EFM == NULL)
  {
    TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileMetadata\n");
    return -1;
  }

  //Get openercontainer instance
  STFS_ExternalFileOpenerContainer *lp_EfoContainer = STFS_ExternalFileOpenerContainer::GetInstance();
  if (!lp_EfoContainer) {
    TRACE_PRINTF1(1,"Error while allocating STFS_OpenIdentifier\n");
    // TBD Cleanup
    return -1;
  }  

  char lp_ExternalFileName[STFS_NAME_MAX];
  strcpy(lp_ExternalFileName, lp_EFM->ExternalFileNameGet());
  if(lp_ExternalFileName[0] == 0)
  {
    return -1;
  }
   
  *pp_PrevIndex += 1;
  
  if((*pp_PrevIndex) >= lp_EfoContainer->Size()+1) {
    *pp_PrevIndex -= 1;
    //Requested index is past the container size. 
    return 1;
  }

  STFS_ExternalFileOpener *lp_EfOpener = lp_EfoContainer->Get(lp_ExternalFileName, (*pp_PrevIndex-1));
    if (!lp_EfOpener) {
      TRACE_PRINTF1(1,"Error while allocating STFS_ExternalFileOpener\n");
      // TBD Cleanup
      return -1;
    }
   
  pv_OpenerNid = lp_EfOpener->sqOpenerNodeId_;
  pv_OpenerPid = lp_EfOpener->sqOpenerPID_;
  strcpy(pp_OpenerName, "TBD");
  strcpy(pp_OpenPath, lp_ExternalFileName);

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_util::STFS_stat()
///
/// \brief  Obtains information about an STFS file
///
/// Returns information about an STFS file. Information about the STFS file
/// is obtained and written in the structure buf.
///
/// \param  pv_Nid       Node ID of local node.
/// \param  pp_Path      If nid is -1, the pathname value of the STFS file 
///                      returned from STFS_openers.  Else value is ignored on 
///                      input and consecutive invocations will return pathname 
///                      of STFS open files in the local node.
/// \param  pv_PrevIndex Identifies last open information returned
/// \param  pv_Mask      A stfs_statmask_t bit field mask in <stfslib.h>
/// \param  pp_Buf       A stfs_stat structure as defined in <stfslib.h> 
/// 
/// \retval int     SUCCESS: 0
///                 FAILURE:-1
///
///////////////////////////////////////////////////////////////////////////////
int 
STFS_util::STFS_stat(stfs_nodeid_t     pv_Nid,
                     char             *pp_Path,
                     long             *pp_PrevIndex,
                     stfs_statmask_t   pv_Mask,
                     struct stfs_stat *pp_Buf) 
{
  const char     *WHERE = "STFS_util::STFS_stat";
  STFS_ScopeTrace lv_st(WHERE,2);

  if((pv_Nid < 0) && (!pp_Path)) {
    errno = EINVAL;
    return -1;
  }
  else {
    //TODO: Check Node Validity
  }

  //Get container instance
  STFS_ExternalFileMetadataContainer *lp_EFMContainer = 
    STFS_ExternalFileMetadata::GetContainer();
  if(lp_EFMContainer == NULL) {
    TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileMetadataContainer\n");
    return -1;
  }

  STFS_ExternalFileMetadata *lp_EFM = new STFS_ExternalFileMetadata();
  if(pv_Nid < 0) 
  {
    lp_EFM  = lp_EFMContainer->Get(pp_Path);
    if(lp_EFM == NULL) {
      TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileMetadata\n");
      return -1;
    }
  }  
  else
  {
    *pp_PrevIndex += 1;

    if((*pp_PrevIndex) >= (lp_EFMContainer->Size() + 1)) {
      return 1;
    }

    lp_EFM = lp_EFMContainer->Geti(*pp_PrevIndex-1);
    if(lp_EFM == NULL) {
      TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileMetadataContainer\n");
      return -1;
    }
    
    ASSERT(lp_EFM->ExternalFileNameGet());
    strcpy(pp_Path, lp_EFM->ExternalFileNameGet());
  }
  
  STFS_ExternalFileHandle *lp_EFH = lp_EFM->EFHGet();
  if(lp_EFH == NULL)
  {
    TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileHandle\n");
    return -1;
  }
 
  ////////////////////////////////////////////////////
  //  Logic for Multiple fragments:
  ////////////////////////////////////////////////////
  uid_t lv_Uid = 0;
  gid_t lv_Gid = 0;
  off_t lv_Size = 0;
  time_t lv_Atime = 0;
  time_t lv_Mtime = 0;
  time_t lv_Ctime = 0;
  struct stat *lp_StatBuf = new struct stat();

  for(unsigned int i = 0; 
      i < lp_EFH->GetNumFragmentsInEFH();
      i++)
  {
    STFS_FragmentFileHandle *lp_FFH = lp_EFH->GetFragment(i); 
    if(!lp_FFH) {
      TRACE_PRINTF1(1,"Error while Getting FFH\n");
      return -1; 
    }
   
    if (lp_FFH->Stat(lp_StatBuf) < 0) {
      TRACE_PRINTF1(2,"Error while Getting Stat for FFH\n");
      return -1;
    } 

    lv_Size += lp_StatBuf->st_size;
    
    if(lp_StatBuf->st_atime > lv_Atime) {
      lv_Atime = lp_StatBuf->st_atime;
    }
    if(lp_StatBuf->st_mtime > lv_Mtime) {
      lv_Mtime = lp_StatBuf->st_mtime;
    }
    if(lp_StatBuf->st_ctime > lv_Ctime) {
      lv_Ctime = lp_StatBuf->st_ctime;
    }
  }
  
  lv_Uid = lp_StatBuf->st_uid;
  lv_Gid = lp_StatBuf->st_gid;
  
  //Get the fragments in the vector list for the EFM
  //Get the stat data for the first fragment
  
  //Uid Gid just get from first fragment
  //if size, then add the size 
  //if more fragments then get the time data and compare to get latest data

  //Bitmask work
  if((pv_Mask & S_STFS_NID) == S_STFS_NID) {
    pp_Buf->nid = lp_EFM->NodeIdGet();
  } 
  if((pv_Mask & S_STFS_MODE) == S_STFS_MODE) {
    pp_Buf->mode = lp_EFH->OpenFlagsGet();
  }
  if((pv_Mask & S_STFS_OPENS) == S_STFS_OPENS) {
    pp_Buf->opens = lp_EFM->UsageCountGet();
  }
  if((pv_Mask & S_STFS_NFRAG) == S_STFS_NFRAG) {
    pp_Buf->nfrag = lp_EFM->GetNumFragments();
  }
  if((pv_Mask & S_STFS_UID) == S_STFS_UID) {
    pp_Buf->uid = lv_Uid ;
  }
  if((pv_Mask & S_STFS_GID) == S_STFS_GID) {
    pp_Buf->gid = lv_Gid;
  }
  if((pv_Mask & S_STFS_SIZE) == S_STFS_SIZE) {
    pp_Buf->size = lv_Size;
  }
  if((pv_Mask & S_STFS_ATIME) == S_STFS_ATIME) {
    pp_Buf->atime = lv_Atime;
  }
  if((pv_Mask & S_STFS_MTIME) == S_STFS_MTIME) {
    pp_Buf->mtime = lv_Mtime;
  }
  if((pv_Mask & S_STFS_CTIME) == S_STFS_CTIME) {
    pp_Buf->ctime = lv_Ctime;
  }
  
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFS_util::STFS_fstat()
///
/// \brief  Obtains information about an STFS file
///
/// Returns information about an STFS file. Information about the STFS file
/// is obtained and written in the structure buf.
///
/// \param  pv_Fhandle   File handle returned from STFS_open or STFS_mkstemp 
/// \param  pp_Path      If nid is -1, the pathname value of the STFS file 
///                      returned from STFS_openers.  Else value is ignored on 
///                      input and consecutive invocations will return pathname 
///                      of STFS open files in the local node.
/// \param  pv_PrevIndex Identifies last open information returned
/// \param  pv_Mask      A stfs_statmask_t bit field mask in <stfslib.h>
/// \param  pp_Buf       A stfs_stat structure as defined in <stfslib.h> 
/// 
/// \retval int     SUCCESS: 0
///                 FAILURE:-1
///
///////////////////////////////////////////////////////////////////////////////
int
STFS_util::STFS_fstat(stfs_fhndl_t      pv_Fhandle,
                      stfs_statmask_t   pv_Mask,
                      struct stfs_stat *pp_Buf)
{
  const char     *WHERE = "STFS_util::STFS_fstat";
  STFS_ScopeTrace lv_st(WHERE,2);

  STFS_ExternalFileHandle *lp_EFH = STFS_ExternalFileHandle::GetExternalFileHandle(pv_Fhandle);
  if(lp_EFH == NULL)
  {
    TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileHandle\n");
    return -1;
  }

  STFS_ExternalFileMetadata *lp_EFM = lp_EFH->EfmGet();
  if(lp_EFM == NULL)
  {
    TRACE_PRINTF1(1,"Error while Getting STFS_ExternalFileMetadata\n");
    return -1;
  }
 
  //Bitmask work
  if((pv_Mask & S_STFS_NID) == S_STFS_NID) {
    pp_Buf->nid = lp_EFM->NodeIdGet();
  } 
  if((pv_Mask & S_STFS_MODE) == S_STFS_MODE) {
    pp_Buf->mode = lp_EFH->OpenFlagsGet();
  }
  if((pv_Mask & S_STFS_OPENS) == S_STFS_OPENS) {
    pp_Buf->opens = lp_EFM->UsageCountGet();
  }
  if((pv_Mask & S_STFS_NFRAG) == S_STFS_NFRAG) {
    pp_Buf->nfrag = lp_EFM->GetNumFragments();
  }
  if((pv_Mask & S_STFS_UID) == S_STFS_UID) {
    //pp_Buf->uid = ;
  //    set the appropriate value in the struct 
  }
  if((pv_Mask & S_STFS_GID) == S_STFS_GID) {
    //pp_Buf->gid = ;
  //    set the appropriate value in the struct 
  }
  if((pv_Mask & S_STFS_SIZE) == S_STFS_SIZE) {
    pp_Buf->size = lp_EFH->FileEOFGet();
  }
  if((pv_Mask & S_STFS_ATIME) == S_STFS_ATIME) {
    //pp_Buf->atime = ;
  //    set the appropriate value in the struct 
  }
  if((pv_Mask & S_STFS_MTIME) == S_STFS_MTIME) {
    //pp_Buf->mtime = ;
  //    set the appropriate value in the struct 
  }
  if((pv_Mask & S_STFS_CTIME) == S_STFS_CTIME) {
    //pp_Buf->ctime = ;
  //    set the appropriate value in the struct 
  }
  
  return 0;

}

// Pings the STFS Daemon. 
int
STFS_util::STFS_ping()
{
  const char       *WHERE = "STFS_util::STFS_ping";
  STFS_ScopeTrace   lv_st(WHERE,2);

#ifdef SQ_STFSD
  int lv_Stat = 0;
  char lp_MsgBuffer[512];
  char lp_RspBuffer[512];
  int  lv_RspLen = 0;

  STFS_util::Init(false);

  memset(lp_MsgBuffer, 0, sizeof(lp_MsgBuffer));
  memset(lp_RspBuffer, 0, sizeof(lp_RspBuffer));
  strcpy(lp_MsgBuffer, "Ping");

  lv_Stat = STFS_util::SendToSTFSd(lp_MsgBuffer,
                                   strlen(lp_MsgBuffer),
                                   lp_RspBuffer,
                                   &lv_RspLen);

  if (lv_Stat == 0) {
    if ((lv_RspLen == 4) &&
      (strcmp(lp_RspBuffer, "Pong") == 0)) {
      return 0;
    }
  }

  return 1;
#else
  return 0;
#endif
}

#ifdef SQ_STFSD
int
STFS_util::SendToSTFSd(void *pp_ReqBuffer,
                       int   pv_ReqLen,
                       void *pp_RspBuffer,
                       int  *pp_RspLen)
{
  const char       *WHERE = "SendToSTFSd";
  STFS_ScopeTrace   lv_st(WHERE,2);
  static int        lv_Tag = 1;

  int               lv_Stat = 0;

  lv_Stat = msg_mon_stfsd_send(pp_ReqBuffer,
                               pv_ReqLen,
                               pp_RspBuffer,
                               pp_RspLen,
                               lv_Tag++);

  return lv_Stat;
}
#endif

int
STFS_util::STFS_evlog(int                     pv_EventType,
		      posix_sqlog_severity_t  pv_Severity,
		      char                   *pp_EvlBuf)
{
  const char       *WHERE = "STFS_util::STFS_evlog";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int  lv_Err;
  char la_Ebuf[STFS_util::STFS_EVLOG_BUF_SIZE];

  lv_Err = evl_sqlog_init(la_Ebuf, STFS_util::STFS_EVLOG_BUF_SIZE);
  if (lv_Err) {
    return lv_Err;
  }

  lv_Err = evl_sqlog_add_token(la_Ebuf, TY_STRING, pp_EvlBuf);
  if (!lv_Err) {
    lv_Err = evl_sqlog_write((posix_sqlog_facility_t)SQ_LOG_SEAQUEST, 
			     pv_EventType,
			     pv_Severity,
			     la_Ebuf);
  }

  return lv_Err;
}


///////////////////////////////////////////////////////////////////////////////
// STFS_ScopeTrace implementation methods
///////////////////////////////////////////////////////////////////////////////

STFS_ScopeTrace::STFS_ScopeTrace(const char *pp_ScopeName, short pv_TraceLevel):
  scopeName_(pp_ScopeName),
  scopeTraceLevel_(pv_TraceLevel)
{
  if (STFS_util::traceLevel_ >= scopeTraceLevel_) {
    trace_where_printf(scopeName_,
                       "ENTER\n");
  }
}

STFS_ScopeTrace::~STFS_ScopeTrace() 
{
  if (STFS_util::traceLevel_ >= scopeTraceLevel_) {
    trace_where_printf(scopeName_,
                       "EXIT\n");
  }
}

#ifdef SQ_STFSD
static
void
SQFileSystemClose()
{
  const char       *WHERE = "SQFileSystemClose";
  STFS_ScopeTrace   lv_st(WHERE,2);

  static bool sv_MpiCloseCalled = false;

  if (sv_MpiCloseCalled) {
    return;
  }
  sv_MpiCloseCalled = true;
  file_mon_process_shutdown();
}

static 
int
SQFileSystemSetup (int* argc, char** argv[] )
{
  const char       *WHERE = "SQFileSystemSetup ";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Retcode = 0;

  static bool sv_MpiSetupCalled = false;

  if (sv_MpiSetupCalled) {
    return 0;
  }

  sv_MpiSetupCalled = true;

  file_init_attach(argc,argv,1,(char *) "");
  lv_Retcode = file_mon_process_startup(1);

  if (lv_Retcode == 0) {
    msg_debug_hook("STFS", "stfs.hook");
  }
  STFS_util::seaquestFilesystemInitialized_ = true;

  return lv_Retcode;
}
#endif

static 
int
SQInit()
{
  const char       *WHERE = "SQInit";
  STFS_ScopeTrace   lv_st(WHERE,2);

#ifdef SQ_STFSD
  static bool lv_SbInitialized = false;

  if (lv_SbInitialized) {
    return 0;
  }

  lv_SbInitialized = true;
    
  int     lv_Argc = 0;
  char  **lp_Argv = 0;
  char    lp_ProcFileName[128];
  FILE   *lp_ProcFile = 0;
  char    lp_Buf[2048];
  short   lv_Index = 0;
  int     lv_Char = 0;

  lp_Argv = (char **) malloc(100 * sizeof(char *));
  sprintf(lp_ProcFileName, "/proc/%d/cmdline",getpid());
  lp_ProcFile = fopen(lp_ProcFileName, "r");

  lp_Buf[0] = 0;  lv_Index = 0;
  while ((lv_Char = fgetc(lp_ProcFile)) != EOF) {
    lp_Buf[lv_Index++] = (char ) lv_Char; 
    ASSERT(lv_Index < (int) sizeof(lp_Buf));
    if (lv_Char == 0) {
      if (lv_Argc == 0) {
        // check if we are the monitor
        char *lp_LastSlash = strrchr(lp_Buf, '/');
        if (lp_LastSlash) {
          lp_LastSlash++;
          if (strcmp(lp_LastSlash, "monitor") == 0) {
            STFS_util::executingInDaemon_ = true;
            break;
          }
        }
      }
      lp_Argv[lv_Argc] = (char *) malloc ((lv_Index + 1) * sizeof(char));
      strcpy(lp_Argv[lv_Argc++], lp_Buf);
      lv_Index = 0;
      lp_Buf[0] = 0;
    }
  }

  fclose(lp_ProcFile);

  if (STFS_util::executingInDaemon_) {
    return 0;
  }

  try
    {
      int lv_Retcode = SQFileSystemSetup(&lv_Argc, &lp_Argv);
      if (lv_Retcode == 0)
        atexit(SQFileSystemClose);
    }
  catch (...)
    {
      TRACE_PRINTF1(1, "Not running in a SQ environment");
      goto exit;
    }

 exit:
  for (int i = 0; i < lv_Argc; i++) {
    free(lp_Argv[i]);
  }
  free(lp_Argv);
#endif
  return 0;
}
