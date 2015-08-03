#ifndef STFS_HANDLE_H
#define STFS_HANDLE_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_handle.h
///  \brief   Defines the following classes:
///              STFS_FragmentFileHandle,
///              STFS_ExternalFileHandle, 
///          and STFS_ExternalFileHandleContainer
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

#include "stfs/stfslib.h"
#include "stfs_defs.h"
#include "stfs_root.h"
#include "stfs_ext2.h"
#include "stfs_metadata.h"
#include "stfs_error.h"

#include <vector>

namespace STFS {
  class STFS_ExternalFileMetadata;
  class STFS_FragmentFileMetadata;
  class STFS_FragmentFileHandle;
  class STFS_ExternalFileHandle;
  class STFS_ExternalFileHandleContainer;

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_ExternalFileHandle
  ///
  /// \brief  External file handle class
  ///
  /// This class will hold the variables and methods to create and manipulate 
  /// the efh.  Methods are also included that operate on the file that is 
  /// referred to by the efh. 
  /// \n
  /// This is the master table for all external files opened by a single STFS 
  /// client, with one entry for each open.  This table serves as the master 
  /// control point for STFSlib accesses, containing pointers to the external 
  /// file metadata and to file descriptor control information for the individual 
  /// fragments that form an STFS file.  These fragment file descriptors contain 
  /// the information needed to do file I/O operations against the file. 
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_ExternalFileHandle : public STFS_Root
  {
  public:
    STFS_ExternalFileHandle();
    STFS_ExternalFileHandle(STFS_ExternalFileMetadata *pp_Efm);

    virtual ~STFS_ExternalFileHandle();

    virtual bool IsEyeCatcherValid();
 
    long Read(char   *pp_Buf, 
              size_t  pv_Count);
  
    ssize_t Write(const void *pp_Data,
                  long        pv_Count);
  
    off_t Lseek(off_t pv_Offset, 
                int   pv_Whence);

    int  Close();

    int  Unlink();

    short InsertFFH(STFS_FragmentFileHandle *pp_Ffh);

    static STFS_ExternalFileHandleContainer *GetContainer();

    static int InsertIntoContainer(STFS_ExternalFileHandle *pp_Efh);
    
    static int DeleteFromContainer(STFS_ExternalFileHandle *&pp_Efh);

    static STFS_ExternalFileHandle* GetExternalFileHandle(stfs_fhndl_t pv_Fhandle);
 
    int GetFD();

    size_t GetNumFragmentsInEFH();

    STFS_FragmentFileHandle *GetFragment(int pv_Index);

    //Getters/Setters
    
    STFS_ExternalFileMetadata* EfmGet() const;

    long CurrentOffsetGet() const{
      return currentOffset_;
    }

    STFS_FragmentFileHandle* CurrentFfhGet() const{
      return currentFFH_;
    }

    long FileEOFGet() const {
      return fileEOF_;
    }

    long OpenFlagsGet() const {
      return openFlags_;
    }

    STFS_OpenIdentifier& OpenIdentifierGet() {
      return openIdentifier_;
    }

    char *GetExternalFilename();

    bool GetError ( int *pp_errNo,
                    int *pp_additionErr,
                    char *pp_context,
                    size_t pv_contextLenMax,
                    size_t *pp_contextLenOut
                    );

    bool HasError ();

    void CurrentOffsetSet(long pv_CurrentOffset) {
      Lock();
      currentOffset_ = pv_CurrentOffset;
      Unlock();
    }

    void FileEOFSet(long pv_FileEOF) {
      Lock();
      fileEOF_ = pv_FileEOF;
      Unlock();
    }

    void OpenFlagsSet(long pv_OpenFlags) {
      Lock();
      openFlags_ = pv_OpenFlags;
      Unlock();
    }

    void OpenIdentifierSet(STFS_OpenIdentifier& pv_OpenIdentifier) {
      Lock();
      openIdentifier_.sqOwningDaemonNodeId = pv_OpenIdentifier.sqOwningDaemonNodeId;
      openIdentifier_.openIdentifier = pv_OpenIdentifier.openIdentifier;
      Unlock();
    }

    bool SetError ( bool pv_errorIsHighest,
                    int pv_errNo,
                    int pv_addlErr,
                    char *pp_contextBuf,
                    size_t pv_contextBufLen);

    void ResetErrors ();


    // Methods for supporting more than one fragment

    bool SetCurrentFragmentBasedOnCurrOffset(void);
    int  SeekToCurrFragmentStart();
    bool CurrentFFHIsLast(void);
    bool NewFFHsNeeded(void);
    int OpenNewFragments(void);
    size_t GetLastFFMStartOffset();
    int IsCurrFragmentLocal(void);

    // Variables for forcing fragment creation via environment variables
    // These flags are set on a per-open basis (not per-process!), but only
    // count STFS_write() calls that pass basic parameter tests (i.e., that
    // result in an actual I/O).  This code is only used in STFSLib; STFSd never
    // arbitrarily decides to create a new fragment.
    //
    // In a future world, we could surround all of this code by #ifdef DEBUG,
    // but for now I'll mainline it.  We can decide later if we need this in
    // release code (for QA purposes, if nothing else).
    //
    //
    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
    //
    // This adds to the path length for a write, which is a VERY BAD THING.
    // That's why we might consider removing it with compiler toggles.  I know,
    // I know, CPU cycles are cheap, but that doesn't mean we should go out of
    // our way to waste 'em...
    //
    //
    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING


    // the actual envvar names.  We check these and set the appropriate EFH
    // booleans as part of EFH construction.

#define EV_FORCEFRAGCREATION "SQ_NUM_WRITES_PER_FRAG"
#define EV_FRAGPARTIALWRITE "SQ_WRITE_PARTIAL_FRAG"

    bool ForcingFragmentCreation();
    bool ForcePartialWrite();
    bool CheckForNewFragRequired();
    void ResetForceNewFragCounter();

  private:
    short EvalCurrentFFH(long  pv_FileOffset,
                         long &pv_FragmentOffset,
                         int   pv_Whence = SEEK_SET);

    typedef struct {
      bool  forceFragmentCreation;
      bool  partialWrites;
      int   maxCounter;
      int   writesSinceLastFragCreate;
    } STFS_FragCreation_def;
  
    STFS_ExternalFileMetadata                         *efm_;
    long                                               currentOffset_;
    long                                               fileEOF_;
    long                                               openFlags_;
    typedef std::vector<STFS_FragmentFileHandle *>     ffhVector_Def;
    ffhVector_Def                                      ffhVector_;

    /// Need to make sure that currentFFH_ is set correctly 
    /// (e.g STFS_write, STFS_seek etc.). 
    /// Especially need to set it properly in an error condition.
    STFS_FragmentFileHandle                           *currentFFH_; 
    STFS_OpenIdentifier                                openIdentifier_;
    STFS_Error                                         openError_;
    STFS_FragCreation_def                              fragForcing_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_FragmentFileHandle
  ///
  /// \brief  fragment file handle class
  ///
  /// This class will hold the variables and methods to create and manipulate 
  /// the ffh.  Methods are also included that operate on the file that is 
  /// referred to by the ffh. 
  /// \n
  /// The fragment file handles for an STFS external file handle are stored as 
  /// a list, in the order in which the metadata is returned from STFSd.  There 
  /// is one entry for each fragment file that might be accessed via this 
  /// ExternalFileHandle entry.
  /// \n  
  /// The FragmentFileHandle (FFH) list is a list of the fragment file handles 
  /// for a given external file name currently known to the STFSlib in the client 
  /// process.  Each FragmentFileHandleEntry represents a single potential open of 
  /// an internal file, associated with a single STFSlib ExternalFileHandle.  
  /// The number of entries on the FragmentFileHandle list varies depending on the 
  /// number of fragments comprising an external file.  A FragmentFileHandleEntry 
  /// is added when a new fragment is detected while doing I/O. 
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_FragmentFileHandle : public STFS_Root { 
  public:
    STFS_FragmentFileHandle();
    STFS_FragmentFileHandle(STFS_FragmentFileMetadata *pp_Ffm);

    ~STFS_FragmentFileHandle();

    virtual bool IsEyeCatcherValid();

    bool IsLocal();
 
    bool IsOpen() const;

    int Close();

    off_t Lseek(off_t pv_Offset, 
                int   pv_Whence);

    int Open(int pv_OFlag);

    long Read(char   *pp_Buf, 
              size_t  pv_Count);

    int Stat(struct stat *pp_Buf);

    int Unlink();

    long Write(const void *pp_Data, 
               long        pv_Count);

    // Does a chdir to the directory where the fragment file is
    int ChdirToFragmentDir();
   
    //Getters/Setters
    STFS_fs* FsGet() const {
      return (STFS_fs *)fs_;
    }

    STFS_FragmentFileMetadata* FfmGet() const {
      return (STFS_FragmentFileMetadata *)ffm_;
    }

    void FsSet(STFS_fs *pp_Fs) {
      Lock();
      if (fs_) {
        delete fs_;
      }
      fs_ = pp_Fs;
      Unlock();
    }
 
    void FfmSet(STFS_FragmentFileMetadata *pp_Ffm) {
      Lock();
      ffm_ = pp_Ffm;
      Unlock();
    }

  private:
    STFS_fs                      *fs_;
    STFS_FragmentFileMetadata    *ffm_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_ExternalFileHandleContainer 
  ///
  /// \brief  Container for external file handles
  ///
  /// This class contains methods to create and manipulate the container for
  /// External File Handles(EFH).  This container will be used when more than one
  /// EFH is being used for an External File Metadata.
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_ExternalFileHandleContainer : public STFS_Root {
  public:
    virtual bool IsEyeCatcherValid();
 
    static STFS_ExternalFileHandleContainer* EfhContainer_;

    static STFS_ExternalFileHandleContainer* GetInstance();

    short Insert(STFS_ExternalFileHandle *pp_Efh);
    short Delete(STFS_ExternalFileHandle *pp_Efh);
    bool Exists(STFS_ExternalFileHandle *pp_Efh);
    long Size();

  private:
    //To enforce a singleton container
    STFS_ExternalFileHandleContainer();

    typedef std::vector<STFS_ExternalFileHandle *>   efhVector_Def;
    efhVector_Def                                    efhVector_;
  };
}
#endif //STFS_HANDLE_H
