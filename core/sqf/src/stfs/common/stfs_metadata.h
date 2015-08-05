#ifndef _STFS_FFM_H
#define _STFS_FFM_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_metadata.h
///  \brief   Defines metadata specific classes 
///    
///  This file defines the following classes:
///    STFS_ExternalFileMetadata
///    STFS_FragmentFileMetadata
///    STFS_ExternalFileMetadataContainer
///    STFS_ExternalFileOpenerContainer
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
#include "stfs_handle.h"
#include "stfs_util.h"

#include <vector>
#include <map>

namespace STFS {

  class STFS_FragmentFileMetadata;
  class STFS_ExternalFileMetadataContainer;

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_ExternalFileMetadata
  ///
  /// \brief  External file metadata class
  /// 
  /// This class holds the metadata for an External File created by the STFSd.
  /// An object of this class is owned by the STFS daemon that creates it.
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_ExternalFileMetadata : public STFS_Root {
  public:
    STFS_ExternalFileMetadata();
    
    STFS_ExternalFileMetadata(char *pp_InFileName,
			      short pv_NodeId);

    virtual ~STFS_ExternalFileMetadata();

    virtual bool IsEyeCatcherValid();

    virtual STFS_ExternalFileHandle* Open(bool pv_IsControllingOpen);

    virtual int Close(bool pv_IsDaemon);

    virtual int Unlink();

    static int InsertIntoContainer(STFS_ExternalFileMetadata *pp_Efm);
    
    static int DeleteFromContainer(STFS_ExternalFileMetadata *&pp_Efm);

    static STFS_ExternalFileMetadata* GetFromContainer(char *pp_Key);
 
    //Getters
    char* ExternalFileNameGet() {
      return externalFileName_;
    }
      
    short UsageCountGet() {
      return usageCount_;
    }

    int NodeIdGet() {
      return nodeId_;
    }

    bool FileAvailableGet() {
      return fileAvailable_;
    }

    STFS_ExternalFileHandle* EFHGet() const {
      return controllingOpenEFH_;
    }

    bool FileIsLocallyOwned(void);

    //Setters
    void FileAvailableSet(bool pv_FileAvailable) {
      Lock();
      fileAvailable_ = pv_FileAvailable;
      Unlock();
    }

    void IncrementUsageCount() 
    {
      Lock();
      usageCount_++;
      Unlock();
    }

    void DecrementUsageCount() 
    {
      if (usageCount_ > 0) {
        Lock();
	usageCount_--;
        Unlock();
      }
    }

    void EFHSet(STFS_ExternalFileHandle* pp_Efh) {
      Lock();
      controllingOpenEFH_ = pp_Efh;
      Unlock();
    }

    //other methods
    STFS_FragmentFileMetadata* CreateFFM(char *pp_FragmentFilename,
					 char *pp_StfsDirectory,
					 long  pv_FragmentFileOffset);

    short CopyAndAppendFFM (STFS_FragmentFileMetadata *pp_efmToCopy);
    short InsertFFM(STFS_FragmentFileMetadata *pp_FFM);

    size_t GetNumFragments() {return ffmVector_.size();}

    ssize_t GetFullEOF(void);

    STFS_FragmentFileMetadata *GetFragment(int pv_Index);

    // Note: This could instantiate different types of container
    // (derived from STFS_ExternalFileMetadataContainer)
    // based on whether the code is being executed in the library or
    // the daemon.
    static STFS_ExternalFileMetadataContainer* GetContainer();

    // Pack and unpack methods for sending across process boundaries
    bool Pack (char *pp_buf, size_t pv_bufLen, size_t *pv_packedLen );
    bool Unpack (char *pp_buf, size_t pv_bufLen);

  private:  

    char                                             externalFileName_[STFS_NAME_MAX];
    short                                            usageCount_;
    int                                              nodeId_;
    typedef std::vector<STFS_FragmentFileMetadata *> ffmVector_Def;
    ffmVector_Def                                    ffmVector_;
    bool                                             fileAvailable_; // Set to false to dis-allow
                                                                     // any more opens
    STFS_ExternalFileHandle                         *controllingOpenEFH_;
  };

  // Rev: Move the container classes out of here. 
  // Rev: The implementation of the metadata classes could be separated into diff source files.
  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_FragmentFileMetadata
  ///
  /// \brief  Fragment file metadata class
  /// 
  /// This class holds the metadata of a fragment.
  /// An object of this class is owned by the STFS daemon that creates it.
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_FragmentFileMetadata : public STFS_Root { 
  public:
    STFS_FragmentFileMetadata();
    STFS_FragmentFileMetadata(char *pp_FragmentFilename,
			      short pv_NodeId, 
			      char *pp_StfsDirectory,
			      int   pv_FragmentNumber, 
			      int   pv_Offset);

    virtual ~STFS_FragmentFileMetadata();

    virtual bool IsEyeCatcherValid();

    // Rev: Add a comment - Is it Local to the node of the caller
    bool IsLocal() const;

    //Getters
    int FragmentNumberGet() const {
      return fragmentNumber_;
    }

    char* NameGet() const {
      return (char *)filename_;
    }

    int OffsetGet() const {
      return offset_;
    }

    int NodeIdGet() {
      return nodeId_;
    }

    char *StfsDirectoryGet() {
      return stfsDirectory_;
    }

    STFS_ExternalFileMetadata *EFMGet() {
      return efm_;
    }
   
    //Setters
    void FragmentNumberSet(int number);
  
    void NameSet(char *pp_filename);

    void OffsetSet(long offset);

    void NodeIdSet(short nodeId);
    
    void StfsDirectorySet(char *pv_StfsDirectory);
   
    void EFMSet(STFS_ExternalFileMetadata *pp_efm);


    // Pack and unpack methods for sending across process boundaries

    bool Pack (char *pp_buf, size_t pv_bufLen, size_t *pv_packedLen );
    bool Unpack (char *pp_buf, size_t pv_bufLen);

  private:
    char                       filename_[STFS_NAME_MAX];      //File Name
    int                        fragmentNumber_;               //Fragment Num in the set of fragments for the File (zero based)
    long                       offset_;                       //Absolute offset of this fragment's first byte
    int                        nodeId_;                       //Node id where the physical file resides
    char                       stfsDirectory_[STFS_NAME_MAX]; //Whether /stfsl<nid> or /stfse<nid>
    STFS_ExternalFileMetadata *efm_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_CharacterPointerComparator
  ///
  /// \brief  Helper class used to compare two character pointers.
  /// 
  /// This class is used by the STFS_ExternalFileMetadataContainer class 
  /// when creating a map where the 'key' of the map is a 'char *'
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_CharacterPointerComparator {
  public:
    bool operator() (char *pp_lhs, char *pp_rhs) const;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_ExternalFileMetadataContainer
  ///
  /// \brief  Container of External File Metadata
  /// 
  /// This singleton class is a container of the External File Metadata objects.
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_ExternalFileMetadataContainer : public STFS_Root {
  public:
    virtual bool IsEyeCatcherValid();
 
    static STFS_ExternalFileMetadataContainer* EfmContainer_;

    // creates the container if it does not exist
    static STFS_ExternalFileMetadataContainer* GetInstance();

    short Insert(char                        *pp_Key,
		 STFS_ExternalFileMetadata   *pp_Efm);

    short Delete(char                        *pp_Key);

    STFS_ExternalFileMetadata* Get(char *pp_Key);

    STFS_ExternalFileMetadata* Geti(long pv_Index);

    long Size();
  
    void Walk();

    void Cleanup();

  private:
    /* To enforce a singleton container */
    STFS_ExternalFileMetadataContainer();
      
    ~STFS_ExternalFileMetadataContainer() {}

    typedef std::map<char *,STFS_ExternalFileMetadata *,STFS_CharacterPointerComparator>  efmMap_Def;
    STFS_ExternalFileMetadataContainer::efmMap_Def                                        efmMap_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_ExternalFileOpener
  ///
  /// \brief  Structure to store the STFS file opener process info
  /// 
  ///
  ///////////////////////////////////////////////////////////////////////////////
  struct STFS_ExternalFileOpener {
    int                        sqOpenerNodeId_; // Opener's SQ Node ID
    int                        sqOpenerPID_;    // Opener's SQ Process ID

    bool operator==(const STFS_ExternalFileOpener& other) const;

    bool operator() (STFS_ExternalFileOpener *pp_lhs, 
		     STFS_ExternalFileOpener *pp_rhs) const;
  };

  // Opener and the metadata 
  struct STFS_ExternalFileOpenerInfo {
    STFS_ExternalFileOpener    efo_;
    STFS_ExternalFileMetadata *efm_; 
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_OpenIdentifierComparator
  ///
  /// \brief  Helper class - implements the compare() method to compare two 
  ///         STFS_OpenIdentifier pointers
  /// 
  /// This class is used by the STFS_ExternalFileOpenerContainer class 
  /// when creating a map where the 'key' of the map is a 'STFS_OpenIdentifier *'
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_OpenIdentifierComparator {
  public:
    bool operator() (STFS_OpenIdentifier *pp_lhs, 
		     STFS_OpenIdentifier *pp_rhs) const;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class  STFS_ExternalFileOpenerContainer
  ///
  /// \brief  Container of External File Openers
  /// 
  /// This singleton class is a container of the External File Opener objects.
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFS_ExternalFileOpenerContainer : public STFS_Root {
  public:
    virtual bool IsEyeCatcherValid();
 
    static STFS_ExternalFileOpenerContainer* EfoContainer_;

    // creates the container if it does not exist
    static STFS_ExternalFileOpenerContainer* GetInstance();

    short Insert(STFS_OpenIdentifier          *pp_OpenId,
		 STFS_ExternalFileOpenerInfo  *pp_EFO);

    short Delete(STFS_OpenIdentifier *pp_OpenId);

    STFS_ExternalFileOpenerInfo* Get(STFS_OpenIdentifier *pp_OpenerId);

    STFS_ExternalFileOpenerInfo* Geti(long pv_index);

    // returns the opener of the given external file at the specified index
    STFS_ExternalFileOpener *Get(char *pp_ExternalFileName, int pv_Index);

    STFS_OpenIdentifier* Get(STFS_ExternalFileOpener *pp_EFO,
			    int                      pv_Index);

    // returns the number of openers of the given external file 
    long Size(char* pp_ExternalFileName);

    long Size();
  
    void Walk();

  private:
    /* To enforce a singleton container */
    STFS_ExternalFileOpenerContainer();
      
    ~STFS_ExternalFileOpenerContainer() {}

    //called by the public Insert(STFS_OpenIdentifier *, STFS_EFOI *) method
    short InsertEfo(STFS_ExternalFileOpenerInfo *pp_Efoi);

    //called by the public Delete(STFS_OpenIdentifier *) method
    short DeleteEfo(STFS_ExternalFileOpenerInfo *pp_Efoi);

    //called by the public Insert(STFS_OpenIdentifier *, STFS_EFOI *) method
    short InsertEFOpenIdVector(STFS_ExternalFileOpenerInfo  *pp_Efoi,
			       STFS_OpenIdentifier          *pp_OpenId);

    //called by the public Delete(STFS_OpenIdentifier *) method
    short DeleteEFOpenIdVector(STFS_ExternalFileOpenerInfo *pp_Efoi,
			       STFS_OpenIdentifier          *pp_OpenId);

    typedef std::vector<STFS_ExternalFileOpener *> efoVector_Def;
    STFS_ExternalFileOpenerContainer::efoVector_Def* GetEfoVector(char *pp_ExternalFileName);

    // Map Key: Open Id
    // Map Val: ExternalFileOpenerInfo*
    // Used to get the OpenerInfo for a given Open Id
    typedef std::map<
      STFS_OpenIdentifier*,
      STFS_ExternalFileOpenerInfo *,
      STFS_OpenIdentifierComparator>               efoiMap_Def;
    STFS_ExternalFileOpenerContainer::efoiMap_Def  efoiMap_;

    // Map Key: STFS File Name
    // Map Val: Vector of STFS_ExternalFileOpener
    // Used to keep track of all the openers of a particular STFS file
    typedef std::map<
      char *,
      STFS_ExternalFileOpenerContainer::efoVector_Def *,
      STFS_CharacterPointerComparator>             efoMap_Def;
    efoMap_Def                                     efoMap_;

    typedef std::vector<STFS_OpenIdentifier>    efOpenIdVector_Def;
    STFS_ExternalFileOpenerContainer::efOpenIdVector_Def*
      GetOpenIdVector(STFS_ExternalFileOpener *pp_EFO);

    // Map Key: STFS_ExternalFileOpener
    // Map Val: Vector of STFS_OpenIdentifier
    // Used to keep track of all the Open Ids of an opener process
    typedef std::map<
      STFS_ExternalFileOpener *,
      STFS_ExternalFileOpenerContainer::efOpenIdVector_Def *,
      STFS_ExternalFileOpener> efOpenIdVectorMap_Def;
    efOpenIdVectorMap_Def                                     efOpenIdVectorMap_;


  };

} //namespace STFS

#endif //STFS_FFM_H

