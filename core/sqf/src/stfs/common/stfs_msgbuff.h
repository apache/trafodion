#ifndef STFS_MSGBUFF_H
#define STFS_MSGBUFF_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_msgbuff.h
///  \brief   STFSMsgBuf_* template classes
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
#include "stfs_root.h"
#include "stfs_error.h"
#include "stfs_message.h"
#include "stfs_metadata.h"
#include "stfs_util.h"
#include "seabed/thread.h"

namespace STFS {


  /////////////////////////////////////////////////////////////////////////////
  //
  //  Adding a message?  Here's what you need to do:
  //    1)  Add a class to STFS_Message for the container
  //    2)  Add a message type here
  //    3)  Add a class and methods for the new message.
  //    4)  Add the new class to the common fixed message union at the bottom of
  //        the file 
  //    5)  Add any variable processing needed to STSF_msgbufv
  //    6)  Add the callers for the container class
  //    7)  Compile/Test/Ship/Party!
  //
  //  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  //
  //   Classes that are members of unions can't have explicit constructors or
  //   destructors.  Each message method has an Init and Release member that
  //   does the same thing but MUST BE CALLED EXPLICITLY.  Make sure that these
  //   get called appropriately.  If your new message adds another control
  //   structure, make sure it gets correctly initialized and freed as well!!!
  //
  //  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  //
  ////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////
    ///  The STFS message type enum
    ///  
    ///   WARNING WARNING: To make life safer for all of us, please only add new
    ///   message type literals before MT_Unknown
    //////////////////////////////////

  enum Enum_MessageType {
    MT_Invalid = 0,
    MT_CreateFile,
    MT_CreateFileReply,
    MT_CreateFragment,
    MT_CreateFragmentReply,
    MT_OpenFile,
    MT_OpenFileReply,
    MT_CloseFile,
    MT_CloseFileReply,
    MT_UnlinkFile,
    MT_UnlinkFileReply,
    MT_ErrorReply,
    MT_GetFileMetadata,
    MT_GetFileMetadataReply,
    MT_Openers,
    MT_OpenersReply,
    MT_FOpeners,
    MT_FOpenersReply,
    MT_Stat,
    MT_StatReply,
    MT_GetEFMReply,
    MT_GetEFM,
    /// WARNING WARNING : new message types go immediately before this line
    MT_Unknown
  };

  /////////////////////////////////
  /// Message path tells where we're from and where we're going.
  /////////////////////////////////
  enum Enum_MessagePathType {
    MPT_Invalid = 0,
    MPT_LibToDaemon,
    MPT_DaemonToDaemon
  };

#define STFS_MSGTEMPLATEVERSION 1
#define STFS_MSGBUFEYECATCHER "STF"



  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_CommonHeader
  /// 
  /// \brief  The common message header for all messages
  ///
  /// This class encapsulates the STFS message header that leads off
  /// every STFS message buffer.
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_CommonHeader {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( Enum_MessageType pv_messageType,
		Enum_MessagePathType pv_messagePathType,
		int pv_requesterNodeID,
		int pv_requesterPID );

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////
    void SetMessageType (Enum_MessageType pv_MessageType);
    void SetMessageVersion (int pv_MessageVersion);
    void SetMessagePath (Enum_MessagePathType pv_MessagePath);
    void SetRequesterNodeId( int pv_NodeId );
    void SetRequesterPID ( int pv_PID );

    void SetVariableInfo (bool pv_HasVarInfo, varoffset_t pv_varOffsetBase);

    /////////////////////////
    /// Get methods
    /////////////////////////

    Enum_MessageType GetMessageType (void);
    int GetMessageVersion (void);
    Enum_MessagePathType GetMessagePath (void);
    int  GetRequesterNodeId( void );
    int  GetRequesterPID ( void );

    void GetVariableInfo (bool *pv_hasVariableInfo, varoffset_t *pv_varOffsetBase);
    bool EyeCatcherIsValid (void);
    void Validate (void);

  private:

    typedef struct {
      Enum_MessageType      msgType;       // Must be first!
      char                  eyeCatcher[4]; // STF, always!
      int   msgVersion;
      Enum_MessagePathType  msgPath;
      int                   requesterNodeId_;           // Node Id of the requester
      int                   requesterPID_;              // PID of the requester
      bool                  hasVarSection;
      varoffset_t           varSectionBaseOffset;      // offset from beginning
                                                       // of whole msgbuf to the
                                                       // start of the variable
						       // section.
    } CommmonHeader_Def;

    CommmonHeader_Def msgHeader_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_MetadataContainer
  /// 
  /// \brief  Fixed-size package for sending metadata across processes
  ///
  /// This class encapsulates the information needed to pack metadata.  The
  /// MetadataContainer is the fixed-length overhead control structure sent in
  /// any message that includes the file and fragment metdata (EFM and FFMs).
  /// The actual EFM and FFM data is in the variable-length area.  We might need
  /// to move it to STFS_Metadata...
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_MetadataContainer {
  public:
    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( varoffset_t pv_EFMOffset, 
		int pv_numFragments,
		varoffset_t pv_FFMOffset, 
		size_t pv_totalFFMSize);

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    void       SetEFMOffset (varoffset_t pv_EFMOffset);
    void       SetNumFragments (int pv_numFragments);
    void       SetFFMOffset (varoffset_t pv_FFMOfset);
    void       SetTotalFFMSize (size_t pv_totalFFMSize);


    /////////////////////////
    /// Get methods
    /////////////////////////

    varoffset_t   GetEFMOffset (void);
    int           GetNumFragments (void);
    varoffset_t   GetFFMOffset (void);
    size_t        GetTotalFFMSize (void);

  private:

  ////////////////////////////////////////////
  ///  Metadata Encapsulation
  ////////////////////////////////////////////

    typedef struct {
      varoffset_t      EFMOffset;            // The variable area offset to the
                                             // beginning of the new EFM.
      int              numFragments;         // Number of fragments for this
					     // file.
      varoffset_t      FFMOffset;            // The variable area offset to the
					     // begining of the FIRST FFM info.
      size_t           totalFFMSize;         // Size of all FFM info, including
					     // any padding for alignment.
  } STFSMsg_MetadataPackage_Def;



    STFSMsg_MetadataPackage_Def metadataPackage_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_ErrorReply
  /// 
  /// \brief  The error message reply 
  ///
  /// This class encapsulates an error-containing reply from one STFS_* process
  /// to another.  It can be sent in response to any request message and
  /// indicates that processing on the request could not complete due to one or
  /// more error conditions.  These conditions are contained in the variable
  /// area of the message.
  ///
  /// The specifics of precisely how much processing was completed in the server
  /// process and whether that work has been undone depends on the semantics of
  /// the original request.
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_ErrorReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init(STFS_Error *pp_Error);


    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    void SetNumErrors(int pv_numberOfErrors);
    void SetReportedError (int pv_reportedError);
    void SetOffsetToErrorVectorOffsets (varoffset_t pv_EVOffset);
    void SetOffsetToFirstPackedErrorStruct (varoffset_t pv_firstPacked);

    /////////////////////////
    /// Get methods
    /////////////////////////

    int  GetNumErrors(void);
    int  GetReportedError (void);
    varoffset_t GetOffsetToErrorVectorOffsets (void);
    varoffset_t GetOffsetToFirstPackedErrorStruct (void);


  private:

  typedef struct {
    STFSMsgBuf_CommonHeader      msgHeader;
    
    int                          numErrors;
    int                          reportedError;
    varoffset_t                  offsetToErrorVectorOffsets;
    varoffset_t                  offsetToFirstPackedErrorStruct;
  } ErrorReplyFixed_Def;

    ErrorReplyFixed_Def errorReply_;

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_CloseFileReq Message
  /// 
  /// \brief  The close file request 
  ///
  /// This class encapsulates the close file request that closes an
  /// externally-visible STFS file.  It can travel Lib->D, or D->D in
  /// the case of a remote file close.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_CloseFileReq {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( STFS_OpenIdentifier *pp_OpenId,
                int                  pv_requesterNodeId,
	        int                  pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    STFS_OpenIdentifier *GetOpenIdentifier ( void );

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader msgHeader;
      STFS_OpenIdentifier     openIdentifier_; 

  } CloseFileReqFixed_Def;


    CloseFileReqFixed_Def  closeFileReq_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_CloseFileReply Message
  /// 
  /// \brief  The close message reply 
  ///
  /// This class encapsulates the close file reply that creates an
  /// externally-visible STFS file.  It can travel Lib->D, or D->D in
  /// the case of a remote file close.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_CloseFileReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( STFS_OpenIdentifier *pp_OpenId,
                int                  pv_requesterNodeId,
	        int                  pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    STFS_OpenIdentifier *GetOpenIdentifier ( void );

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader msgHeader;
      STFS_OpenIdentifier     openIdentifier_; 

  } CloseFileReplyFixed_Def;


    CloseFileReplyFixed_Def  closeFileReply_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_CreateFileReq Message
  /// 
  /// \brief  The create message request unveiled
  ///
  /// This class encapsulates the create file request that creates an
  /// externally-visible STFS file.  It can travel Lib->D, or D->D in
  /// the case of a remote file create.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_CreateFileReq {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( char *pp_template,
		bool  pp_isMkstemp,
		int   pv_requesterNodeId,
		int   pv_requesterPID);

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    void SetTemplate ( char* pp_template );
    void SetIsMkstemp ( bool pv_isMkstemp );


    /////////////////////////
    /// Get methods
    /////////////////////////

    void GetTemplate ( char * pp_tgtTemplate, size_t pv_templateMaxSize );
    bool GetIsMkstemp ( void );


  private:

    typedef struct {
      STFSMsgBuf_CommonHeader msgHeader;
      
      char                  template_[STFS_NAME_MAX];   // Null-terminated
							// string
      bool                  isMkstemp;                  // If true, then caller
							// is mkstemp and
							// mkstemp rules apply.
  } CreateFileReqFixed_Def;


    CreateFileReqFixed_Def  createFileReq_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_CreateFileReply
  /// 
  /// \brief  The create message reply 
  ///
  /// This class encapsulates the reply to the create file message.
  /// externally-visible STFS file.  It can travel D->Lib, or D->D in
  /// the case of a remote file create.
  ///
  ///  This message contains the completed file name (changed from the request
  ///  if the request call was STFS_mkstemp, unchanged otherwise, and the
  ///  complete EFM and FFM information.
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_CreateFileReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init (char * pp_template, 
	       STFS_OpenIdentifier *pp_openId);

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////
    void SetTemplate (char *pp_template);

    void SetOpenIdentifier (STFS_OpenIdentifier *pp_openID);

    void SetVarAreaSize (size_t pv_varAreaSize);

    void SetVarMetadataControl (varoffset_t pv_EFMOffset, 
				int pv_numFragments, 
				varoffset_t pv_firstFFMOffset, 
				size_t pv_totalFFMSize);


    /////////////////////////
    /// Get methods
    /////////////////////////
    void GetTemplate ( char * pp_tgtTemplate, size_t pv_templateMaxSize );

    STFS_OpenIdentifier GetOpenIdentifier ();

    size_t GetVarAreaSize ();

    void GetVarMetadataControl (varoffset_t *pp_EFMOffset, 
				int *pp_numFragments, 
				varoffset_t *pp_firstFFMOffset, 
				size_t *pp_totalFFMSize);

  private:

    typedef struct {
      STFSMsgBuf_CommonHeader msgHeader;
      char                    template_[STFS_NAME_MAX];// Null-terminated
                                                       // string
      STFS_OpenIdentifier     openIdentifier;          // The open identifier
						       // for this open.
      size_t                  varAreaSize;             // Total size in bytes of
						       // the variable area.
      STFSMsgBuf_MetadataContainer  metadataInfo;
                                                       // Container for EFM, FFMs

  } CreateFileReplyFixed_Def;

    CreateFileReplyFixed_Def  createFileReply_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_CreateFragmentReq Message
  /// 
  /// \brief  The create fragment message request unveiled
  ///
  /// This class encapsulates the create fragment request that creates
  /// a fragment for an externally-visible STFS file.  It can travel
  /// Lib->D, or D->D in the case of a fragment that's created remotely from the
  /// STFS file, or when a remote library attempts to create a new fragment.
  ///
  ///  This message has no variable-length area at this time.  
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_CreateFragmentReq {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init (STFS_OpenIdentifier *pv_openIdentifier,
	       int                  pv_requesterNodeId,
	       int                  pv_requesterPID);

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    void SetOpenIdentifier (STFS_OpenIdentifier *pp_openID);

    /////////////////////////
    ///  Get methods
    /////////////////////////

    STFS_OpenIdentifier GetOpenIdentifier ();


  protected:
  private: 

    typedef struct {
      STFSMsgBuf_CommonHeader msgHeader;
                                                       // string
      STFS_OpenIdentifier     openIdentifier;          // The open identifier
						       // for this open.

    } CreateFragReqFixed_Def;

    CreateFragReqFixed_Def  createFragmentReq_;

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_CreateFragmentReply
  /// 
  /// \brief  The create fragment reply 
  ///
  /// This class encapsulates the reply to the create fragment message, which
  /// creates a single fragment for an STFS file.  It can travel D->Lib, or D->D
  /// in the case of a remote fragment create.
  ///
  ///  This reply is composed of the fixed informtation for an updated EFM and
  ///  FFM information, which includes the new fragment.  The actual EFM and
  ///  FFMs are packed in the variable portion of the message.  
  ///
  ///  If an error occurs, then an error reply is sent instead.
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_CreateFragmentReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    // No parameterized constructor; this message is all variable-lenght related
    // so the higher-level pack() picks up what we need

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    void SetVarAreaSize (size_t pv_varAreaSize);

    void SetVarMetadataControl (varoffset_t pv_EFMOffset, 
				int pv_numFragments, 
				varoffset_t pv_firstFFMOffset, 
				size_t pv_totalFFMSize);


    /////////////////////////
    /// Get methods
    /////////////////////////
    size_t GetVarAreaSize ();

    void GetVarMetadataControl (varoffset_t *pp_EFMOffset, 
				int *pp_numFragments, 
				varoffset_t *pp_firstFFMOffset, 
				size_t *pp_totalFFMSize);

  private:

    typedef struct {
      STFSMsgBuf_CommonHeader msgHeader;

      size_t                  varAreaSize;             // Total size in bytes of
						       // the variable area.
      STFSMsgBuf_MetadataContainer  metadataInfo;
                                                       // Container for EFM, FFMs

  } CreateFragmentReplyFixed_Def;

    CreateFragmentReplyFixed_Def  createFragmentReply_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_FOpenersReq 
  /// 
  /// \brief FOpeners Request Message
  ///
  /// This class encapsulates the FOpeners request that obtains information from an 
  /// externally-visible STFS file.  It can travel Lib->D, or D->D in
  /// the case of a remote file.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_FOpenersReq {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( stfs_fhndl_t         pv_Fhandle,
                STFS_OpenersSet     *pp_OpenersSet,
                int                  pv_requesterNodeId,
	        int                  pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    stfs_fhndl_t      GetFhandle();
    char *            GetPath();
    STFS_OpenersSet   GetFOpenersSet();

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader msgHeader;
      stfs_fhndl_t            fhandle; 
      char                    path[STFS_NAME_MAX];
      STFS_OpenersSet         fOpenersSet;

  } FOpenersReqFixed_Def;


    FOpenersReqFixed_Def  fopenersReq_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_FOpenersReply Message
  /// 
  /// \brief FOpeners Reply Message
  ///
  /// This class encapsulates the FOpeners request that obtains information from an 
  /// externally-visible STFS file.  It can travel Lib->D, or D->D in
  /// the case of a remote file.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_FOpenersReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( stfs_fhndl_t        pv_Fhandle,
                STFS_OpenersSet    *pp_OpenersSet,
                int                 pv_requesterNodeId,
	        int                 pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    stfs_fhndl_t      GetFhandle();
    char *            GetPath();
    STFS_OpenersSet   GetFOpenersSet();

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader msgHeader;
      stfs_fhndl_t            fhandle; 
      char                    path[STFS_NAME_MAX];
      STFS_OpenersSet         fOpenersSet;
      

  } FOpenersReplyFixed_Def;


    FOpenersReplyFixed_Def  fopenersReply_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_OpenersReq Message
  /// 
  /// \brief Openers Request Message
  ///
  /// This class encapsulates the openers information request.
  /// It can travel Lib->D, or D->D.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_OpenersReq {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( stfs_nodeid_t        pv_Nid,
                char                *pp_Path,
                STFS_OpenersSet     *pp_OpenersSet,
                int                  pv_requesterNodeId,
	        int                  pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    stfs_nodeid_t    GetNid();
    char *           GetPath();
    STFS_OpenersSet  GetOpenersSet();

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader msgHeader;
      stfs_nodeid_t           Nid;
      char                    Path[STFS_NAME_MAX];
      STFS_OpenersSet         OpenersSet;

  } OpenersReqFixed_Def;


    OpenersReqFixed_Def  openersReq_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_OpenersReply Message
  /// 
  /// \brief  Openers Message reply
  ///
  /// This class encapsulates the Openers reply.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_OpenersReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( stfs_nodeid_t        pv_Nid,
                char                *pp_Path,
                STFS_OpenersSet     *pp_OpenersSet,
                int                  pv_requesterNodeId,
	        int                  pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    stfs_nodeid_t    GetNid();
    char *           GetPath();
    STFS_OpenersSet  GetOpenersSet();

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader msgHeader;
      stfs_nodeid_t           Nid;
      char                    Path[STFS_NAME_MAX];
      STFS_OpenersSet         OpenersSet;
      

  } OpenersReplyFixed_Def;


    OpenersReplyFixed_Def  openersReply_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_StatReq Message
  /// 
  /// \brief Stat Message Request 
  ///
  /// This class encapsulates the Stat request.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_StatReq {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( stfs_nodeid_t         pv_Nid,
                char                 *pp_Path,
                struct STFS_StatSet  *pp_StatSet,
                stfs_statmask_t       pv_Mask, 
                int                   pv_requesterNodeId,
	        int                   pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    stfs_nodeid_t    GetNid();
    char *           GetPath();
    STFS_StatSet     GetStatSet();
    stfs_statmask_t  GetMask();
  private:

    typedef struct {

      STFSMsgBuf_CommonHeader msgHeader;
      stfs_nodeid_t           Nid;
      char                    Path[STFS_NAME_MAX];
      STFS_StatSet            StatSet;
      stfs_statmask_t         StatMask;

  } StatReqFixed_Def;


    StatReqFixed_Def  statReq_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_StatReply Message
  /// 
  /// \brief Stat Reply Message
  ///
  ///  This class encapsulates the Stat Reply Message.
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_StatReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( stfs_nodeid_t        pv_Nid,
                char                *pp_Path,
                struct STFS_StatSet *pp_StatSet,
                stfs_statmask_t      pv_Mask, 
                int                  pv_requesterNodeId,
	        int                  pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    stfs_nodeid_t    GetNid();
    char *           GetPath();
    STFS_StatSet     GetStatSet();
    stfs_statmask_t  GetMask();

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader msgHeader;
      stfs_nodeid_t           Nid;
      char                    Path[STFS_NAME_MAX];
      STFS_StatSet            StatSet;
      stfs_statmask_t         StatMask;
      

  } StatReplyFixed_Def;


    StatReplyFixed_Def  statReply_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_OpenFileReq Message
  /// 
  /// \brief  Open file request message
  ///
  ///  This class encapsulates the open file request that opens an
  ///  externally-visible STFS file.  
  ///
  ///  This message has no variable-length area at this time.  When we
  ///  add security checking and authentication for STFS files, that
  ///  may change. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_OpenFileReq {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( char *pp_FileName,
		bool  pv_OpenFlag,
		int   pv_requesterNodeId,
		int   pv_requesterPID);

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    /// Get methods
    /////////////////////////

    void GetFileName ( char * pp_FileName, size_t pv_FileNameMaxSize );
    int  GetOpenFlag ( void );


  private:

    typedef struct {
      STFSMsgBuf_CommonHeader msgHeader;
      
      char                    fileName_[STFS_NAME_MAX];   // Null-terminated
				                          // string
      int                     openFlag_;
  } OpenFileReqFixed_Def;


    OpenFileReqFixed_Def  openFileReq_;
  };


  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_OpenFileReply
  /// 
  /// \brief Open Message reply 
  ///
  /// This class encapsulates the reply to the open file message.
  ///
  ///  This message contains the completed file name (changed from the request
  ///  if the request call was STFS_mkstemp, unchanged otherwise, and the
  ///  complete EFM and FFM information.
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_OpenFileReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init (char                *pp_FileName, 
	       STFS_OpenIdentifier *pp_openId);

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////
    void SetVarAreaSize (size_t pv_varAreaSize);

    void SetVarMetadataControl (varoffset_t pv_EFMOffset, 
				int         pv_numFragments, 
				varoffset_t pv_firstFFMOffset, 
				size_t      pv_totalFFMSize);


    /////////////////////////
    /// Get methods
    /////////////////////////
    void GetFileName ( char * pp_FileName, size_t pv_FileNameMaxSize );

    STFS_OpenIdentifier GetOpenIdentifier ();

    size_t GetVarAreaSize ();

    void GetVarMetadataControl (varoffset_t *pp_EFMOffset, 
				int         *pp_numFragments, 
				varoffset_t *pp_firstFFMOffset, 
				size_t      *pp_totalFFMSize);

  private:

    typedef struct {
      STFSMsgBuf_CommonHeader msgHeader;
      char                    fileName_[STFS_NAME_MAX];// Null-terminated
                                                       // string
      STFS_OpenIdentifier     openIdentifier;          // The open identifier
						       // for this open.
      size_t                  varAreaSize;             // Total size in bytes of
						       // the variable area.
      STFSMsgBuf_MetadataContainer  metadataInfo;
                                                       // Container for EFM, FFMs

  } OpenFileReplyFixed_Def;

    OpenFileReplyFixed_Def  openFileReply_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_UnlinkFileReq Message
  /// 
  /// \brief Unlink File Request Message 
  ///
  /// This class encapsulates the unlink file request that unlinks an
  /// externally-visible STFS file. 
  ///
  ///  This message has no variable-length area at this time. 
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_UnlinkFileReq {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( const char          *pp_fileName,
                int                  pv_requesterNodeId,
	        int                  pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    char *GetFileName ( void );

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader  msgHeader;
      char                     fileName_[STFS_NAME_MAX]; 

    } UnlinkFileReqFixed_Def;


    UnlinkFileReqFixed_Def  unlinkFileReq_;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMsgBuf_UnlinkFileReply Message
  /// 
  /// \brief Unlink File Reply Message
  ///
  ///  This message has no variable-length area at this time.  
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFSMsgBuf_UnlinkFileReply {
  public:

    /////////////////////////
    /// Constructors
    /////////////////////////

    void Init();
    void Init ( const char          *pp_fileName,
                int                  pv_requesterNodeId,
	        int                  pv_requesterPID); 

    /////////////////////////
    /// Destructors
    /////////////////////////

    void Release();

    /////////////////////////
    ///  Set methods
    /////////////////////////

    /////////////////////////
    /// Get methods
    /////////////////////////

    char *GetFileName ( void );

  private:

    typedef struct {

      STFSMsgBuf_CommonHeader  msgHeader;
      char                     fileName_[STFS_NAME_MAX]; 

     } UnlinkFileReplyFixed_Def;


    UnlinkFileReplyFixed_Def  unlinkFileReply_;
  };

  // Here's the actual message union that's used in STFS_Msg to overlay the
  // buffer
  //
  // Please keep this at the bottom so it's easy to find.

  union FixedMsg_u {
    char                              charArray[1];
    STFSMsgBuf_CommonHeader           headerOnly;
    STFSMsgBuf_ErrorReply             errorReply;
    STFSMsgBuf_CreateFileReq          createFileReq;
    STFSMsgBuf_CreateFileReply        createFileReply;
    STFSMsgBuf_CreateFragmentReq      createFragmentReq;
    STFSMsgBuf_CreateFragmentReply    createFragmentReply;
    STFSMsgBuf_CloseFileReq           closeFileReq;
    STFSMsgBuf_CloseFileReply         closeFileReply;
    STFSMsgBuf_OpenFileReq            openFileReq;
    STFSMsgBuf_OpenFileReply          openFileReply;
    STFSMsgBuf_UnlinkFileReq          unlinkFileReq;
    STFSMsgBuf_UnlinkFileReply        unlinkFileReply;
    STFSMsgBuf_OpenersReq             openersReq;
    STFSMsgBuf_OpenersReply           openersReply;
    STFSMsgBuf_FOpenersReq            fopenersReq;
    STFSMsgBuf_FOpenersReply          fopenersReply;
    STFSMsgBuf_StatReq                statReq;
    STFSMsgBuf_StatReply              statReply;
  };

  // Update the array in the following method when a class 
  // associated with a new message type is added
  size_t GetSizeOfFixedMessageBuffer(Enum_MessageType pv_MessageType);

} // namespace
#endif // ifdef STFS_MSGBUFF_H

