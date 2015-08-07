#ifndef STFS_MESSAGE_H
#define STFS_MESSAGE_H

///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfs_message.h
///  \brief   STFS_Message class
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
#include "seabed/thread.h"
#include "stfs_msgbuff.h"

namespace STFS {

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// A brief word on message structure and usage
  ///
  /// The buffer in an STFS_Message structure has 3 basic components:
  ///     **************************
  ///     *                        *
  ///     *     Common Header      *    Every message has this.
  ///     *                        *    Fixed size for all messages.
  ///     *                        *
  ///     *------------------------*
  ///     *                        *
  ///     *                        *
  ///     *                        *     Every message has this too!
  ///     *          Type          *     Size varies by message type
  ///     *        Specific        *
  ///     *         fixed          *     Pointers resolved to offsets
  ///     *          size          *     in variable length area
  ///     *          data          *
  ///     *                        *
  ///     *                        *
  ///     *------------------------*
  ///     *                        *
  ///     *                        *
  ///     *                        *
  ///     *          Type          *
  ///     *        Specific        *
  ///     *        Variable        *     Not all messages have this.  Only
  ///     *         Length         *     needed if fixed type-specific 
  ///     *          Data          *     section has variable length
  ///     *                        *     components, or if message is sending
  ///     *                        *     large amounts of data (read/write)
  ///     *                        *
  ///     *                        *
  ///     *                        *
  ///     **************************
  ///
  ///  These routines don't send messages, they just prep them for sending. The
  ///  caller is reponsible for allocating the message buf and passing it in
  ///  either at constuctor time or using a set method.  The buffer is assumed
  ///  to be of STFS_MAX_MESSAGEBUFSIZE.
  ///
  ///  For reads and writes, we'll need to carefully set the fixed area so that
  ///  we don't have to copy user buffers around by default.
  /// 
  ///  What we refer to as packing and unpacking is simply filling in
  ///  the fixed-length areas and resolving any pointers into offsets
  ///  to the same data in the variable data area.  Every message
  ///  defined here needs an appropriate pack method and a
  ///  corresponding unpack method.  Of course, if there's no variable
  ///  length data, the pack and unpack methods are trivially easy.
  ///
  ///  The message templates (both fixed and variable) are private and
  ///  subject to change.  The higher-level interfaces to them are
  ///  through the STFSMessage_* classes.  Do not call the methods
  ///  directly; use the STFSMessage_ interfaces!
  ///
  ///////////////////////////////////////////////////////////////////////////////


#define STFS_MAX_MESSAGEBUFSIZE 65535


  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFS_Message
  /// 
  /// \brief  The base class for messages between STFS processes
  ///
  /// This class serves as the base class for all messages between
  /// STFS processes, covering STFSLib->STFSd and STFSd->STFSd
  /// messages.
  ///
  /// It includes a single eye catcher for all messages; this
  /// eyecatcher is used for authentication purposes but not on a
  /// per-message basis.  In addition, it includes message types for
  /// all messages, and virtual pack and unpack methods
  ///
  /// THis serves as the base class for all STFS messages and
  /// replies.  When adding a new message request/reply, be sure to
  /// add the new message type to the MT_* enum type and to create a
  /// class for the message.
  ///
  ///////////////////////////////////////////////////////////////////////////////

  class STFS_Message {
  public:


    ///////////////////
    /// Constructors
    //////////////////

    STFS_Message(void);

    STFS_Message(Enum_MessageType pv_MessageType,
                 Enum_MessagePathType pv_MessagePathType,
                 void * pp_msgBufPtr, 
                 size_t bufLen
                 );

    ///////////////////
    /// Destructor
    //////////////////
    virtual ~STFS_Message(void);


    /////////////////
    /// Get Methods
    /////////////////

    Enum_MessageType GetMessageType(void);              // Gets message type
    Enum_MessagePathType GetMessagePath(void);          // Gets message path (sender)
    bool MessageBufHasActiveIO (void);                  // Is msg buf used for
                                                        // outstanding I/O?

    void * GetMessageBuf (void);                        // Gets ptr to buf
    size_t GetMessageMaxBufSize (void);                 // current message buf
                                                        // max size
    size_t GetMessageCurrSize (void);                   // Gets current size of msg
    size_t GetMessageFixedSize (void);                  // Current fixed size
    size_t GetMessageVariableSize (void);               // Current variable size
    int    GetRequesterNodeId( void );
    int    GetRequesterPID ( void );


    /////////////////
    /// Set Methods
    /////////////////

    bool SetMessageFixedSize (size_t pv_newFixedSize);
    bool SetMessageType (Enum_MessageType pv_newMessageType);
    bool SetMessagePath (Enum_MessagePathType pv_newMessagePath);
    bool SetMessageBufIOInProgress (bool pv_IOInProgress);
    bool ResetMessageBufContents (void);

    void SetMessageBuf (void * pp_buffer,
                        size_t pv_bufferLen, 
                        bool pv_PopulateHeader = false);

    virtual size_t AppendToVariableSection (char *pp_newData,
                                            size_t pv_newDataLen);

                                                    // variable length buffer

  protected:

    bool SetMessageBufVariableSize (size_t pv_VariableSize);

    FixedMsg_u          *buf_;                      // The actual message
    size_t               bufLen_;                   // How big in total?

  private:

    Enum_MessageType     messageType_;              // What message do we have here?
    Enum_MessagePathType messagePath_;              // Who sent it?
    bool                 messageBufInUse_;          // Indicates outstanding I/O
    size_t               fixedSize_;                // How big is fixed portion?
    size_t               variableSize_;             // How big is variable
                                                    // portion right now?
  };

  ////////////////////////////////////////////////////////////////////////////
  /// Container classes for each message type.
  ///  these describe the message while hiding the contents of the buffer
  ///
  ///  For buffer contents/layout, see stfs_msgbuf modules.  But Shhhhh!
  ///  They're private!  Don't tell anyone where they are!
  ///
  ////////////////////////////////////////////////////////////////////////////


  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_CreateFileRequest
  /// 
  /// \brief  The container for a message to create a file
  ///
  /// This class encapsulates the messages required to create an STFS file
  /// through either the STFS_create() or STFS_mkstemp() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_CreateFileRequest : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_CreateFileRequest();
    STFSMessage_CreateFileRequest (char  *pp_template, 
                                   bool   pv_isMkstemp,
                                   int    pv_RequesterNodeId,
                                   int    pv_RequesterPID,
                                   void  *pp_msgbuf, 
                                   size_t pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_CreateFileRequest();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    size_t GetMessageBufMinSize();
    void   GetTemplate(char * pp_tgtTemplate, size_t pv_templateMaxSize);
    bool   GetIsMkstemp();

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

    void SetMessageBuffer( void *pp_MsgBufPtr, size_t pv_bufLen );


  private:

  };



  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_CreateFileReply
  /// 
  /// \brief  The container for a message containing a create file reply
  ///
  /// This class encapsulates the positive reply to a create file request
  /// through either the STFS_create() or STFS_mkstemp() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_CreateFileReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_CreateFileReply();
    STFSMessage_CreateFileReply (char *pp_template, 
                                 STFS_OpenIdentifier *pp_openID,
                                 void * pp_buf,
                                 size_t pv_bufLen);


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_CreateFileReply();

    ///////////////////////////
    /// Set methods
    ///////////////////////////

    void SetMessageBuffer(void * pp_buf, size_t pv_bufLen);
    void SetTemplate ( char *pp_template);
    void SetOpenIdentifier (STFS_OpenIdentifier *pp_openID);

    bool Pack (STFS_ExternalFileMetadata *pp_EFM);

    ///////////////////////////
    /// Get methods
    ///////////////////////////
    size_t GetMessageBufMinSize();
    void   GetTemplate ( char *pp_tgtTemplate, size_t pv_templateMaxSize );
    STFS_OpenIdentifier GetOpenIdentifier (void);

    STFS_ExternalFileMetadata *Unpack ();


  }; // Create File Reply


  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_CreateFragmentRequest
  /// 
  /// \brief  The container for a message to create a fragment of an STFS file.
  ///
  /// This class encapsulates the messages required to add a fragment to an
  /// existing STFS file.  This request is not sent for the initial fragment
  /// which is created automatically as part of a FileCreateRequest.  It is sent
  /// for all subsequent messages.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_CreateFragmentRequest : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_CreateFragmentRequest();
    STFSMessage_CreateFragmentRequest (STFS_OpenIdentifier *pp_openID,
				       int    pv_RequesterNodeId,
				       int    pv_RequesterPID,
				       void  *pp_msgbuf, 
				       size_t pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_CreateFragmentRequest();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    STFS_OpenIdentifier GetOpenID (void);
    size_t GetMessageBufMinSize();

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

    void SetMessageBuffer( void *pp_MsgBufPtr, size_t pv_bufLen );
    void SetOpenIdentifier (STFS_OpenIdentifier *pp_openID);

  private:

  };



  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_CreateFragmentReply
  /// 
  /// \brief  The container for a message containing a create fragment reply
  ///
  /// This class encapsulates the positive reply to a create fragment request
  /// through either the STFS_create() or STFS_mkstemp() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_CreateFragmentReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_CreateFragmentReply();
    STFSMessage_CreateFragmentReply ( void * pp_buf,
				      size_t pv_bufLen);


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_CreateFragmentReply();

    ///////////////////////////
    /// Set methods
    ///////////////////////////

    void SetMessageBuffer(void * pp_buf, size_t pv_bufLen);

    bool Pack (STFS_ExternalFileMetadata *pp_EFM);

    ///////////////////////////
    /// Get methods
    ///////////////////////////
    size_t GetMessageBufMinSize();

    STFS_ExternalFileMetadata *Unpack ();


  }; // Create Fragment Reply


  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_ErrorReply
  /// 
  /// \brief  The container for a message containing an error reply
  ///
  /// This class encapsulates the negative reply for all STFS interprocess
  /// messages. 
  ///
  ///  Every request can receive an error reply, so be prepared to get one at
  ///  any time!
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_ErrorReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_ErrorReply();
    STFSMessage_ErrorReply (STFS_Error *pp_Error,
                            void * pp_buf,
                            size_t pv_bufLen);


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_ErrorReply();

    ///////////////////////////
    /// Set methods
    ///////////////////////////

    void SetMessageBuffer(void * pp_buf, size_t pv_bufLen);
    void SetTemplate ( char *pp_template);
    void SetNumErrors (int pv_numErrors);
    void SetReportedError (int pv_reportedError);

    bool Pack (STFS_Error *pp_Error);

    ///////////////////////////
    /// Get methods
    ///////////////////////////
    size_t GetMessageBufMinSize();
    int    GetNumErrors (void );
    int    GetReportedError (void);

    STFS_Error *Unpack ();

  }; // Error Reply


  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_OpenFileRequest
  /// 
  /// \brief  The container for a message to open a file
  ///
  /// This class encapsulates the messages required to open an STFS file
  /// through the STFS_open() library interface.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_OpenFileRequest : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_OpenFileRequest();
    STFSMessage_OpenFileRequest (char  *pp_FileName, 
                                 int    pv_OpenFlag,
                                 int    pv_RequesterNodeId,
                                 int    pv_RequesterPID,
                                 void  *pp_msgbuf, 
                                 size_t pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_OpenFileRequest();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    size_t GetMessageBufMinSize();
    void   GetFileName(char * pp_FileName, size_t pv_FileNameMaxSize);
    int    GetOpenFlag( void );

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...

    ///////////////////////////
    /// SetMethods
    ///////////////////////////

    void SetMessageBuffer( void *pp_MsgBufPtr, size_t pv_bufLen );


  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_OpenFileReply
  /// 
  /// \brief  The container for a message containing a open file reply
  ///
  /// This class encapsulates the positive reply to a open file request
  /// through the STFS_open() library interface.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_OpenFileReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_OpenFileReply();
    STFSMessage_OpenFileReply (char                *pp_FileName, 
                               STFS_OpenIdentifier *pp_openID,
                               void                *pp_buf,
                               size_t               pv_bufLen);


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_OpenFileReply();

    ///////////////////////////
    /// Set methods
    ///////////////////////////

    void SetMessageBuffer(void * pp_buf, size_t pv_bufLen);

    bool Pack (STFS_ExternalFileMetadata *pp_EFM);

    ///////////////////////////
    /// Get methods
    ///////////////////////////
    size_t GetMessageBufMinSize();
    void   GetFileName ( char *pp_FileName, size_t pv_FileNameMaxSize );
    STFS_OpenIdentifier GetOpenIdentifier (void);

    STFS_ExternalFileMetadata *Unpack ();


  }; // Open File Reply

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_CloseFileRequest
  /// 
  /// \brief  The container for a message to close a file
  ///
  /// This class encapsulates the messages required to close an STFS file
  /// through STFS_close() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_CloseFileRequest : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_CloseFileRequest();
    STFSMessage_CloseFileRequest (STFS_OpenIdentifier *pp_OpenId,
                                  int                  pv_requesterNodeId,
                                  int                  pv_requesterPID, 
                                  void                *pp_msgbuf, 
                                  size_t               pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_CloseFileRequest();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    size_t GetMessageBufMinSize();
    STFS_OpenIdentifier * GetOpenIdentifier( void );

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_CloseFileReply
  /// 
  /// \brief  The container for a message to close a file
  ///
  /// This class encapsulates the messages required to close an STFS file
  /// through STFS_close() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_CloseFileReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_CloseFileReply();
    STFSMessage_CloseFileReply (STFS_OpenIdentifier *pp_OpenId,
                                int                  pv_requesterNodeId,
                                int                  pv_requesterPID, 
                                void                *pp_msgbuf, 
                                size_t               pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_CloseFileReply();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    size_t GetMessageBufMinSize();
    STFS_OpenIdentifier * GetOpenIdentifier( void );

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_UnlinkFileRequest
  /// 
  /// \brief  The container for a message to unlink a file
  ///
  /// This class encapsulates the messages required to unlink an STFS file
  /// through STFS_unlink() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_UnlinkFileRequest : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_UnlinkFileRequest();
    STFSMessage_UnlinkFileRequest(const char          *pp_fileName,
                                  int                  pv_requesterNodeId,
                                  int                  pv_requesterPID, 
                                  void                *pp_msgbuf, 
                                  size_t               pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_UnlinkFileRequest();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    size_t GetMessageBufMinSize();
    char *GetFileName ( void );

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_UnlinkFileReply
  /// 
  /// \brief  The container for a message to unlink a file
  ///
  /// This class encapsulates the messages required to unlink an STFS file
  /// through STFS_unlink() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_UnlinkFileReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_UnlinkFileReply();
    STFSMessage_UnlinkFileReply(const char          *pp_fileName,
                                int                  pv_requesterNodeId,
                                int                  pv_requesterPID, 
                                void                *pp_msgbuf, 
                                size_t               pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_UnlinkFileReply();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    size_t GetMessageBufMinSize();
    char* GetFileName ( void );

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_FOpenersRequest
  /// 
  /// \brief  The container for a message to close a file
  ///
  /// This class encapsulates the messages required to close an STFS file
  /// through STFS_close() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_FOpenersRequest : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_FOpenersRequest();
    STFSMessage_FOpenersRequest (stfs_fhndl_t                pv_Fhandle,
                                 struct STFS_OpenersSet     *pp_FOpenersSet,
                                 int                         pv_requesterNodeId,
                                 int                         pv_requesterPID, 
                                 void                       *pp_msgbuf, 
                                 size_t                      pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_FOpenersRequest();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////
    stfs_fhndl_t GetFhandle();
    char *GetPath();
    STFS_OpenersSet GetFOpenersSet();
    size_t GetMessageBufMinSize();

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_FOpenersReply
  /// 
  /// \brief  The container for a message for FOpeners information
  ///
  /// This class encapsulates the messages required obtain FOpeners information 
  /// through STFS_fopeners().
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_FOpenersReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_FOpenersReply();
    STFSMessage_FOpenersReply (stfs_fhndl_t                pv_Fhandle, 
                               struct STFS_OpenersSet     *pp_FOpenersSet,
                               int                         pv_requesterNodeId,
                               int                         pv_requesterPID, 
                               void                       *pp_msgbuf, 
                               size_t                      pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_FOpenersReply();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    stfs_fhndl_t GetFhandle();
    char *GetPath();
    struct STFS_OpenersSet GetFOpenersSet();
    size_t GetMessageBufMinSize();

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_StatRequest
  /// 
  /// \brief  The container for a message to obtain file statistics
  ///
  /// This class encapsulates the messages required to obtain file statistics
  /// through STFS_stat() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////
  class STFSMessage_StatRequest : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_StatRequest();
    STFSMessage_StatRequest (stfs_nodeid_t           pv_Nid,
                             char                   *pp_Path,
                             struct STFS_StatSet    *pp_StatSet,
                             stfs_statmask_t         pv_Mask, 
                             int                     pv_requesterNodeId,
                             int                     pv_requesterPID, 
                             void                   *pp_msgbuf, 
                             size_t                  pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_StatRequest();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////
    stfs_nodeid_t   GetNid();
    char           *GetPath();
    STFS_StatSet    GetStatSet();
    stfs_statmask_t GetMask();
    size_t          GetMessageBufMinSize();

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_StatReply
  /// 
  /// \brief  The container for a message for Stat information
  ///
  /// This class encapsulates the messages required obtain Stat information 
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_StatReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_StatReply();
    STFSMessage_StatReply (stfs_nodeid_t           pv_Nid,
                           char                   *pp_Path,
                           struct STFS_StatSet    *pp_StatSet,
                           stfs_statmask_t         pv_Mask, 
                           int                     pv_requesterNodeId,
                           int                     pv_requesterPID, 
                           void                   *pp_msgbuf, 
                           size_t                  pv_bufLen );

    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_StatReply();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    stfs_nodeid_t        GetNid();
    char                *GetPath();
    struct STFS_StatSet  GetStatSet();
    stfs_statmask_t      GetMask();
    size_t               GetMessageBufMinSize();

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_OpenersRequest
  /// 
  /// \brief  The container for a message to obtain openers information
  ///
  /// This class encapsulates the messages required to obtain openers information
  /// through STFS_openers() library interfaces.
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_OpenersRequest : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_OpenersRequest();
    STFSMessage_OpenersRequest (stfs_nodeid_t               pv_Nid,
                                char                       *pp_Path,
                                struct STFS_OpenersSet     *pp_OpenersSet,
                                int                         pv_requesterNodeId,
                                int                         pv_requesterPID, 
                                void                       *pp_msgbuf, 
                                size_t                      pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_OpenersRequest();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////
    stfs_nodeid_t GetNid();
    char *GetPath();
    STFS_OpenersSet GetOpenersSet();
    size_t GetMessageBufMinSize();

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

  ///////////////////////////////////////////////////////////////////////////////
  ///
  /// \class STFSMessage_OpenersReply
  /// 
  /// \brief  The container for a message for Openers information
  ///
  /// This class encapsulates the messages required obtain Openers information 
  /// through STFS_openers().
  ///
  ///////////////////////////////////////////////////////////////////////////////


  class STFSMessage_OpenersReply : public STFS_Message 
  {
  public:

    ///////////////////////////
    /// Constructors
    ///////////////////////////

    STFSMessage_OpenersReply();
    STFSMessage_OpenersReply (stfs_nodeid_t               pv_Nid,
                              char                       *pp_Path,
                              struct STFS_OpenersSet     *pp_OpenersSet,
                              int                         pv_requesterNodeId,
                              int                         pv_requesterPID, 
                              void                       *pp_msgbuf, 
                              size_t                      pv_bufLen );


    ///////////////////////////
    /// Destructors
    ///////////////////////////

    ~STFSMessage_OpenersReply();

    ///////////////////////////
    /// GetMethods
    ///////////////////////////

    stfs_nodeid_t GetNid();
    char *GetPath();
    struct STFS_OpenersSet GetOpenersSet();
    size_t GetMessageBufMinSize();

    // No pack method needed here at this point, since there's nothing in a
    // variable area.  This message is all header...


    ///////////////////////////
    /// SetMethods
    ///////////////////////////

  private:

  };

} //namespace STFS
#endif //STFS_MESSAGE_H
