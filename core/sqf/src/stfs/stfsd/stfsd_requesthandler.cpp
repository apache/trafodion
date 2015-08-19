///////////////////////////////////////////////////////////////////////////////
// 
///  \file    stfsd_requesthandler.cpp
///  \brief   STFS Daemon Request Handler Code
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
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#include "stfs/stfslib.h"
#include "seabed/trace.h"

#include "stfs_metadata.h"
#include "stfs_defs.h"
#include "stfs_util.h"
#include "stfs_fragment.h"
#include "stfs_file.h"
#include "stfs_message.h"
#include "stfsd.h"
#include "stfs_session.h"
#include "stfsd_requesthandler.h"

using namespace STFS;

namespace STFS {

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_RequestHandler
///
/// \brief  This is the gateway to request handling by the STFSd. It 
///         validates & checks the request buffer passed in and dispatches
///         it to the appropriate request handler
///
/// \param  int   pv_ReqLength
/// \param  char *pp_ReqBuffer
/// \param  int  *pv_RspLength
/// \param  char *pp_RspBuffer
/// 
///////////////////////////////////////////////////////////////////////////////
int STFSd_RequestHandler(int    pv_ReqLength,
                         char  *pp_ReqBuffer,
                         int   *pv_RspLength,
                         char  *pp_RspBuffer)
{
  const char       *WHERE = "int STFSd_RequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  if (pv_ReqLength <= 0) {
    return -1;
  }

  // Just check if it is a Ping request
  if (strncmp(pp_ReqBuffer, "Ping", 4) == 0) {
    pp_RspBuffer = pp_ReqBuffer;
    strcpy(pp_RspBuffer, "Pong");
    *pv_RspLength = 4;
    return 0;
  }

  STFS_Message *lp_Request  = new STFS_Message();
  lp_Request->SetMessageBuf(pp_ReqBuffer, pv_ReqLength, true);
  
  Enum_MessageType lv_MessageType = lp_Request->GetMessageType();

  switch (lv_MessageType) {
  case MT_CreateFile:
    lv_Stat = STFSd_CreateFileRequestHandler(lp_Request,
                                             pv_RspLength,
                                             pp_RspBuffer);
    if (lv_Stat != 0) {
      return lv_Stat;
    }
    break;
  case MT_FOpeners:
    lv_Stat = STFSd_FOpenersRequestHandler(lp_Request,
                                            pv_RspLength,
                                           pp_RspBuffer);
    if (lv_Stat != 0) {
      return lv_Stat;
    }
    break;
  case MT_Stat:
    lv_Stat = STFSd_StatRequestHandler( lp_Request,
                                        pv_RspLength,
                                        pp_RspBuffer);
    if (lv_Stat != 0) {
      return lv_Stat;
    }
    break;
  case MT_Openers:
    lv_Stat = STFSd_OpenersRequestHandler( lp_Request,
                                           pv_RspLength,
                                           pp_RspBuffer);
    if (lv_Stat != 0) {
      return lv_Stat;
    }
    break;
  case MT_OpenFile:
    lv_Stat = STFSd_OpenFileRequestHandler(lp_Request,
                                           pv_RspLength,
                                           pp_RspBuffer);
    if (lv_Stat != 0) {
      return lv_Stat;
    }
    break;
  case MT_CloseFile:
    lv_Stat = STFSd_CloseFileRequestHandler(lp_Request,
                                             pv_RspLength,
                                             pp_RspBuffer);
    if (lv_Stat != 0) {
      return lv_Stat;
    }
    break;
  case MT_UnlinkFile:
    lv_Stat = STFSd_UnlinkFileRequestHandler(lp_Request,
                                             pv_RspLength,
                                             pp_RspBuffer);
    if (lv_Stat != 0) {
      return lv_Stat;
    }
    break;
  case MT_CreateFragment:
    lv_Stat = STFSd_CreateFragmentRequestHandler(lp_Request,
                                             pv_RspLength,
                                             pp_RspBuffer);
    if (lv_Stat != 0) {
      return lv_Stat;
    }
    break;
  default:
    // unrecognized request:  Missed something on implementation?
    ASSERT (false);
    break;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_CreateFileRequestHandler
///
/// \brief  Processes a create file request:
///         - Downcasts the input message to STFSMessage_CreateFileRequest *
///         - Extracts the required parameters
///         - Calls STFSd_mkstemp
///         - Packages a reply into the pp_RspBuffer
///
/// \param[in]   STFS_Message *lp_Request
/// \param[out]  int          *pv_RspLength
/// \param[out]  char         *pp_RspBuffer
/// 
///////////////////////////////////////////////////////////////////////////////
int STFSd_CreateFileRequestHandler(STFS_Message *lp_Request,
                                   int          *pv_RspLength,
                                   char         *pp_RspBuffer)
{
  const char       *WHERE = "int STFSd_CreateFileRequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  if ((!lp_Request) ||
      (!pv_RspLength) ||
      (!pp_RspBuffer)) {
    return -1;
  }

  STFSMessage_CreateFileRequest *lp_CFReq = 0;
  STFSMessage_CreateFileReply   *lp_CFRep = 0;

  char lp_FileTemplate[STFS_NAME_MAX];
  STFS_ExternalFileMetadata *lp_Efm = 0;
  STFS_OpenIdentifier       *lp_OpenId = 0;

  lp_CFReq = (STFSMessage_CreateFileRequest *) lp_Request;
  lp_CFReq->GetTemplate(lp_FileTemplate, STFS_NAME_MAX-1);
  if (lp_CFReq->GetIsMkstemp()) {
    lv_Stat = STFSd_mkstemp(lp_FileTemplate,
                            lp_CFReq->GetRequesterNodeId(),
                            lp_CFReq->GetRequesterPID(),
                            lp_Efm,
                            lp_OpenId);
  }
  else {
    // Straight create request isn't yet supported
    ASSERT (false);
  }

  if (lv_Stat != 0) {
    // TBD Create and Error Reply
    STFS_Error lv_Error;
    lv_Error.PreserveCondition(CondSig_ReplaceHighest, 
                               (errno != 0 ? errno:lv_Stat), 0, NULL, 0);
    STFSMessage_ErrorReply lv_ErrorReplyMsg(&lv_Error,
                                            pp_RspBuffer,
                                            *pv_RspLength);
    
    return lv_Stat;
  }

  lp_CFRep = new STFSMessage_CreateFileReply(lp_FileTemplate,
                                             lp_OpenId,
                                             pp_RspBuffer,
                                             *pv_RspLength);

  if (!lp_CFRep) {
    // TBD Cleanup
    return -1;
  }
  
  if (! lp_CFRep->Pack(lp_Efm)) {
    // TBD Cleanup
    return -1;
  }

  int lv_Size = lp_CFRep->GetMessageCurrSize();
  TRACE_PRINTF2(3, "Reply Message Size: %d\n", lv_Size);

  return lv_Stat;
}


//**********************************************************
///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_CreateFragmentRequestHandler
///
/// \brief  Processes a create fragment request:
///         - Downcasts the input message to STFSMessage_CreateFragmentRequest *
///         - Extracts the required parameters
///         - Calls STFSd_createFragment
///         - Packages a reply into the pp_RspBuffer
///
/// \param[in]   STFS_Message *lp_Request
/// \param[out]  int          *pv_RspLength
/// \param[out]  char         *pp_RspBuffer
/// 
///////////////////////////////////////////////////////////////////////////////
int STFSd_CreateFragmentRequestHandler(STFS_Message *lp_Request,
                                       int          *pv_RspLength,
                                       char         *pp_RspBuffer)
{

  const char       *WHERE = "int STFSd_CreateFragmentRequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  /////////////////////////////////
  ///  Check parameters
  /////////////////////////////////

  if ((!lp_Request) ||
      (!pv_RspLength) ||
      (!pp_RspBuffer)) {
    return -1;
  }

  /////////////////////////////////
  ///  Housekeeping and setup
  /////////////////////////////////

  STFSMessage_CreateFragmentRequest *lp_CFReq = 0;
  STFSMessage_CreateFragmentReply   *lp_CFRep = 0;

  STFS_ExternalFileMetadata *lp_Efm = NULL;
  STFS_OpenIdentifier       lp_OpenId;

  /////////////////////////////////
  /// Validate CreateFrag Request and extract
  /////////////////////////////////

  lp_CFReq = (STFSMessage_CreateFragmentRequest *) lp_Request;
  lp_OpenId = lp_CFReq->GetOpenID();

  /////////////////////////////////
  /// Verify that this STSFd is the owner or fragment should be created here
  /////////////////////////////////

  //  TBD:  If not the owner, send request to owner

  /////////////////////////////////
  /// Find the EFM for the file
  /////////////////////////////////

  STFS_ExternalFileOpenerContainer *lp_EfoContainer = STFS_ExternalFileOpenerContainer::GetInstance();
  if (!lp_EfoContainer) {
    TRACE_PRINTF2(1,"%s\n", "Null EFO Container");

    // TBD:  Error handling/reply
    ASSERT (false);
    return -1;
  }

  /////////////////////////////////
  /// Find the EFH for this open
  /////////////////////////////////

  //  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  //
  //  If open not found, then we've got a rogue create fragment (if STFS file is
  //  on local node), or the first fragment for this file on this node.  This
  //  implementation presumes rogue, but when we implement remote fragments,
  //  we'll have to change this logic!  (And remove this comment)
  //
  //  See below where we reserve the name, another operation only for remote
  //  fragments 
  //
  //  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING


  STFS_ExternalFileOpenerInfo *lp_Efoi = lp_EfoContainer->Get(&lp_OpenId);
  if (!lp_Efoi) {
    TRACE_PRINTF3(1,
                  "Open Id: %d,%ld not found in the EFO Container\n",
                  lp_OpenId.sqOwningDaemonNodeId,
                  lp_OpenId.openIdentifier
                  );

    // TBD:  Error handling/reply
    ASSERT (false);
    return -1;
  }

  lp_Efm = lp_Efoi->efm_;
  if (!lp_Efm) {
    TRACE_PRINTF1(1,"Null EFM Found in the lp_Efoi Entry\n");

    // TBD:  Error handling/reply
    ASSERT (false);
    return -1;
  }

  /////////////////////////////////
  /// Reserve the directory, and create the EFM if needed
  /////////////////////////////////

  //TBD:  This is only for remote fragments

  /////////////////////////////////
  /// Calculate the start offset for the new fragment
  /////////////////////////////////

  //  Theoretically, we could rely on a value that the library passes to us.
  //  Theoretically, however, might not be correct.  We can take the initial
  //  offset of the previous fragment and adjust it by the EOF to get the real
  //  initial offset.  The library can sanity check this value in the returned
  //  FFM using assertions.

  ///////////////////////////////////////////////////////////////////////////
  //
  //  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  //
  //  This code relies on the assumption that there is a single opener with
  //  write access, and only that opener can create new fragment.  From that, it
  //  presumes that the current EOF+1 is the starting offset of the new
  //  fragment.  If we ever allow more than a single write-opener per file, this
  //  code will not work!
  //
  //  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  //
  ///////////////////////////////////////////////////////////////////////////

  long lv_fragOffset = lp_Efm->GetFullEOF();


  /////////////////////////////////
  /// Create the fragment
  /////////////////////////////////

  lv_Stat = STFSd_createFragment (lp_Efm,
                                  lp_CFReq->GetRequesterNodeId(),
                                  lp_CFReq->GetRequesterPID(),
                                  lv_fragOffset);



  if (lv_Stat != 0) {
    // Create an Error 
    STFS_Error lv_Error;
    lv_Error.PreserveCondition(CondSig_ReplaceHighest, 
                               (errno != 0 ? errno:lv_Stat), 0, NULL, 0);

    STFSMessage_ErrorReply lv_ErrorReplyMsg(&lv_Error,
                                            pp_RspBuffer,
                                            *pv_RspLength);
    
    return lv_Stat;
  }

  /////////////////////////////////
  /// Open the fragment
  /////////////////////////////////

  // Tbd -- linking it to an existing open and updating position info...  Not
  // essential until we support remote fragments

  /////////////////////////////////
  /// Construct the reply
  /////////////////////////////////

  lp_CFRep = new STFSMessage_CreateFragmentReply( pp_RspBuffer,
                                                 *pv_RspLength);

  if (!lp_CFRep) {
    // TBD Cleanup
    return -1;
  }
  
  if (! lp_CFRep->Pack(lp_Efm)) {
    // TBD Cleanup
    return -1;
  }

  int lv_Size = lp_CFRep->GetMessageCurrSize();
  TRACE_PRINTF2(3, "Reply Message Size: %d\n", lv_Size);

  /////////////////////////////////
/// Finis!
  /////////////////////////////////

  return lv_Stat;
}

//*************************************************

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_OpenFileRequestHandler
///
/// \brief  Processes a close file request:
///         - Downcasts the input message to STFSMessage_OpenFileRequest *
///         - Extracts the required parameters
///         - Calls STFSd_close
///         - Packages a reply into the pp_RspBuffer
///
/// \param[in]   STFS_Message *lp_Request
/// \param[out]  int          *pv_RspLength
/// \param[out]  char         *pp_RspBuffer
/// 
///////////////////////////////////////////////////////////////////////////////
int STFSd_OpenFileRequestHandler(STFS_Message *lp_Request,
                                 int          *pv_RspLength,
                                 char         *pp_RspBuffer)
{
  const char       *WHERE = "int STFSd_OpenFileRequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  if ((!lp_Request) ||
      (!pv_RspLength) ||
      (!pp_RspBuffer)) {
    return -1;
  }

  STFSMessage_OpenFileRequest *lp_CFReq = 0;
  STFSMessage_OpenFileReply   *lp_CFRep = 0;

  char lp_FileName[STFS_NAME_MAX];
  STFS_ExternalFileMetadata *lp_Efm = 0;
  STFS_FragmentFileMetadata *lp_Ffm = 0;
  STFS_OpenIdentifier       *lp_OpenId = 0;

  lp_CFReq = (STFSMessage_OpenFileRequest *) lp_Request;
  lp_CFReq->GetFileName(lp_FileName, STFS_NAME_MAX-1);
  lv_Stat = STFSd_open(lp_FileName,
                       lp_CFReq->GetOpenFlag(),
                       lp_CFReq->GetRequesterNodeId(),
                       lp_CFReq->GetRequesterPID(),
                       lp_Efm,
                       lp_Ffm,
                       lp_OpenId);

  if (lv_Stat != 0) {
    // TBD Open and Error Reply
    return lv_Stat;
  }

  lp_CFRep = new STFSMessage_OpenFileReply(lp_FileName,
                                           lp_OpenId,
                                           pp_RspBuffer,
                                           *pv_RspLength);

  if (!lp_CFRep) {
    // TBD Cleanup
    return -1;
  }
  
  if (! lp_CFRep->Pack(lp_Efm)) {
    // TBD Cleanup
    return -1;
  }

  int lv_Size = lp_CFRep->GetMessageCurrSize();
  TRACE_PRINTF2(3, "Reply Message Size: %d\n", lv_Size);

  return lv_Stat;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_CloseFileRequestHandler
///
/// \brief  Processes a close file request:
///         - Downcasts the input message to STFSMessage_CloseFileRequest *
///         - Extracts the required parameters
///         - Calls STFSd_close
///         - Packages a reply into the pp_RspBuffer
///
/// \param[in]   STFS_Message *lp_Request
/// \param[out]  int          *pv_RspLength
/// \param[out]  char         *pp_RspBuffer
/// 
///////////////////////////////////////////////////////////////////////////////
int STFSd_CloseFileRequestHandler(STFS_Message *lp_Request,
                                   int          *pv_RspLength,
                                   char         *pp_RspBuffer)
{
  const char       *WHERE = "int STFSd_CloseFileRequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  if ((!lp_Request) ||
      (!pv_RspLength) ||
      (!pp_RspBuffer)) {
    return -1;
  }

  STFSMessage_CloseFileRequest *lp_CFReq = 0;
  STFSMessage_CloseFileReply   *lp_CFRep = 0;

  lp_CFReq = (STFSMessage_CloseFileRequest *) lp_Request;

  lv_Stat = STFSd_close( lp_CFReq->GetOpenIdentifier());
  if (lv_Stat != 0) {
    // TBD Create and Error Reply
    return lv_Stat;
  }

  lp_CFRep = new STFSMessage_CloseFileReply( lp_CFReq->GetOpenIdentifier(),
                                             lp_CFReq->GetRequesterNodeId(),
                                             lp_CFReq->GetRequesterPID(),
                                             pp_RspBuffer,
                                            *pv_RspLength);

  if (!lp_CFRep) {
    // TBD Cleanup
    return -1;
  }
  
  int lv_Size = lp_CFRep->GetMessageCurrSize();
  TRACE_PRINTF2(3, "Reply Message Size: %d\n", lv_Size);

  return lv_Stat;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_FOpenersRequestHandler
///
/// \brief  Processes an openers request:
///         - Downcasts the input message to STFSMessage_FOpenersRequest *
///         - Extracts the required parameters
///         - Calls STFSd_openers
///         - Packages a reply into the pp_RspBuffer
///
/// \param[in]   STFS_Message *lp_Request
/// \param[out]  int          *pv_RspLength
/// \param[out]  char         *pp_RspBuffer
/// 
/// \retval      int  SUCCESS: 0
///                   FAILURE: !0
///
///////////////////////////////////////////////////////////////////////////////
int STFSd_FOpenersRequestHandler( STFS_Message *lp_Request,
                                  int          *pv_RspLength,
                                  char         *pp_RspBuffer)
{
  const char       *WHERE = "int STFSd_FOpenersRequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  if ((!lp_Request) ||
      (!pv_RspLength) ||
      (!pp_RspBuffer)) {
    return -1;
  }

  STFSMessage_FOpenersRequest *lp_FOpReq = 0;
  STFSMessage_FOpenersReply   *lp_FOpRep = 0;

  lp_FOpReq = (STFSMessage_FOpenersRequest *) lp_Request;
 
  STFS_OpenersSet lv_FOpenersSet = lp_FOpReq->GetFOpenersSet();

  //The parameters are input/outputs so this will have to be done differently.
  //Create separate variables for them and use them for the parameters.
  lv_Stat = STFSd_fopeners( lp_FOpReq->GetFhandle(),
                           &lv_FOpenersSet);
  if (lv_Stat < 0) {
    // TBD Create an Error Reply
    return lv_Stat;
  }

  lp_FOpRep = new STFSMessage_FOpenersReply( lp_FOpReq->GetFhandle(),
                                            &lv_FOpenersSet,
                                             lp_FOpReq->GetRequesterNodeId(),
                                             lp_FOpReq->GetRequesterPID(),
                                             pp_RspBuffer,
                                            *pv_RspLength);
  if (!lp_FOpRep) {
    // TBD Cleanup
    return -1;
  }
  
  int lv_Size = lp_FOpRep->GetMessageCurrSize();
  TRACE_PRINTF2(3, "Reply Message Size: %d\n", lv_Size);

  return lv_Stat;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_StatRequestHandler
///
/// \brief  Processes an openers request:
///         - Downcasts the input message to STFSMessage_StatRequest *
///         - Extracts the required parameters
///         - Calls STFSd_openers
///         - Packages a reply into the pp_RspBuffer
///
/// \param[in]   STFS_Message *lp_Request
/// \param[out]  int          *pv_RspLength
/// \param[out]  char         *pp_RspBuffer
/// 
/// \retval      int  SUCCESS: 0
///                   FAILURE: !0
///
///////////////////////////////////////////////////////////////////////////////
int STFSd_StatRequestHandler( STFS_Message *lp_Request,
                                 int          *pv_RspLength,
                                 char         *pp_RspBuffer)
{
  const char       *WHERE = "int STFSd_StatRequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  if ((!lp_Request) ||
      (!pv_RspLength) ||
      (!pp_RspBuffer)) {
    return -1;
  }

  STFSMessage_StatRequest *lp_StatReq = 0;
  STFSMessage_StatReply   *lp_StatRep = 0;

  lp_StatReq = (STFSMessage_StatRequest *) lp_Request;
 
  char *lp_Path = new char[STFS_PATH_MAX];

  STFS_StatSet lv_StatSet = lp_StatReq->GetStatSet();

  strcpy(lp_Path, lp_StatReq->GetPath());
  //The parameters are input/outputs so this will have to be done differently.
  //Create separate variables for them and use them for the parameters.
  lv_Stat = STFSd_stat( lp_StatReq->GetNid(),
                        lp_Path,
                       &lv_StatSet,
                        lp_StatReq->GetMask());
  if (lv_Stat < 0) {
    // TBD Create an Error Reply
    return lv_Stat;
  }

  lp_StatRep = new STFSMessage_StatReply( lp_StatReq->GetNid(),
                                           lp_Path,
                                          &lv_StatSet,
                                           lp_StatReq->GetMask(),
                                           lp_StatReq->GetRequesterNodeId(),
                                           lp_StatReq->GetRequesterPID(),
                                           pp_RspBuffer,
                                          *pv_RspLength);

  if (!lp_StatRep) {
    // TBD Cleanup
    return -1;
  }
  
  int lv_Size = lp_StatRep->GetMessageCurrSize();
  TRACE_PRINTF2(3, "Reply Message Size: %d\n", lv_Size);

  return lv_Stat;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_OpenersRequestHandler
///
/// \brief  Processes an openers request:
///         - Downcasts the input message to STFSMessage_OpenersRequest *
///         - Extracts the required parameters
///         - Calls STFSd_openers
///         - Packages a reply into the pp_RspBuffer
///
/// \param[in]   STFS_Message *lp_Request
/// \param[out]  int          *pv_RspLength

/// \param[out]  char         *pp_RspBuffer
/// 
/// \retval      int  SUCCESS: 0
///                   FAILURE: !0
///
///////////////////////////////////////////////////////////////////////////////
int STFSd_OpenersRequestHandler( STFS_Message *lp_Request,
                                 int          *pv_RspLength,
                                 char         *pp_RspBuffer)
{
  const char       *WHERE = "int STFSd_OpenersRequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  if ((!lp_Request) ||
      (!pv_RspLength) ||
      (!pp_RspBuffer)) {
    return -1;
  }

  STFSMessage_OpenersRequest *lp_OpReq = 0;
  STFSMessage_OpenersReply   *lp_OpRep = 0;

  lp_OpReq = (STFSMessage_OpenersRequest *) lp_Request;
 
  char *lp_Path = new char[STFS_PATH_MAX];

  STFS_OpenersSet lv_OpenersSet = lp_OpReq->GetOpenersSet();

  strcpy(lp_Path, lp_OpReq->GetPath());
  //The parameters are input/outputs so this will have to be done differently.
  //Create separate variables for them and use them for the parameters.
  lv_Stat = STFSd_openers( lp_OpReq->GetNid(),
                           lp_Path,
                          &lv_OpenersSet);
  if (lv_Stat < 0) {
    // TBD Create an Error Reply
    return lv_Stat;
  }

  lp_OpRep = new STFSMessage_OpenersReply( lp_OpReq->GetNid(),
                                           lp_Path,
                                          &lv_OpenersSet,
                                           lp_OpReq->GetRequesterNodeId(),
                                           lp_OpReq->GetRequesterPID(),
                                           pp_RspBuffer,
                                          *pv_RspLength);

  if (!lp_OpRep) {
    // TBD Cleanup
    return -1;
  }
  
  int lv_Size = lp_OpRep->GetMessageCurrSize();
  TRACE_PRINTF2(3, "Reply Message Size: %d\n", lv_Size);

  return lv_Stat;
}

///////////////////////////////////////////////////////////////////////////////
///
//          STFSd_UnlinkFileRequestHandler
///
/// \brief  Processes an unlink request:
///         - Downcasts the input message to STFSMessage_UnlinkFileRequest *
///         - Extracts the required parameters
///         - Calls STFSd_unlink
///         - Packages a reply into the pp_RspBuffer
///
/// \param[in]   STFS_Message *lp_Request
/// \param[out]  int          *pv_RspLength
/// \param[out]  char         *pp_RspBuffer
/// 
///////////////////////////////////////////////////////////////////////////////
int STFSd_UnlinkFileRequestHandler(STFS_Message *lp_Request,
                                   int          *pv_RspLength,
                                   char         *pp_RspBuffer)
{
  const char       *WHERE = "int STFSd_UnlinkFileRequestHandler";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Stat = 0;

  if ((!lp_Request) ||
      (!pv_RspLength) ||
      (!pp_RspBuffer)) {
    return -1;
  }

  STFSMessage_UnlinkFileRequest *lp_CFReq = 0;
  STFSMessage_UnlinkFileReply   *lp_CFRep = 0;

  lp_CFReq = (STFSMessage_UnlinkFileRequest *) lp_Request;

  lv_Stat = STFSd_unlink( lp_CFReq->GetFileName());
  if (lv_Stat != 0) {
    // TBD Create and Error Reply
    return lv_Stat;
  }

  lp_CFRep = new STFSMessage_UnlinkFileReply( lp_CFReq->GetFileName(),
                                              lp_CFReq->GetRequesterNodeId(),
                                              lp_CFReq->GetRequesterPID(),
                                              pp_RspBuffer,
                                              *pv_RspLength);

  if (!lp_CFRep) {
    // TBD Cleanup
    return -1;
  }
  
  int lv_Size = lp_CFRep->GetMessageCurrSize();
  TRACE_PRINTF2(3, "Reply Message Size: %d\n", lv_Size);

  return lv_Stat;
}

} //namespace STFS
