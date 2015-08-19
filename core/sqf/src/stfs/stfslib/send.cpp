///////////////////////////////////////////////////////////////////////////////
// 
/// \file    send.cpp
/// \brief   High-level message encapsulation routines
///   
/// This file contains the driver code for messages sent to other STFS processes.
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


#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <iostream>


#include "stfs/stfslib.h"

#include "stfs_metadata.h"
#include "stfs_defs.h"
#include "stfs_util.h"
#include "stfsd.h"
#include "stfs_message.h"

namespace STFS {


  ///////////////////////////////////////////////////////////////////////////////
  ///
  //         SendToSTFSd_Close()
  ///
  /// \brief Packages a STFSMessage_CloseFileRequest message, dispatches it to 
  ///        to the STFSd, processes the response message
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_close(  STFS_OpenIdentifier        *pp_OpenId,
                      int                         pv_OpenerNodeId,
                      int                         pv_OpenerPID)
  {
    const char       *WHERE = "SendToSTFSd_close";
    STFS_ScopeTrace   lv_st(WHERE,2);

    STFSMessage_CloseFileRequest *lp_Cfr = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    lp_Cfr = new STFSMessage_CloseFileRequest( pp_OpenId,
                                               pv_OpenerNodeId,
                                               pv_OpenerPID,
                                               lp_ReqMsgBuffer,
                                               sizeof(lp_ReqMsgBuffer));
    lp_Cfr->SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lp_Cfr->GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
    lp_Cfr->SetMessageBufIOInProgress(false);
  
    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);

    Enum_MessageType            lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_CloseFileReply *lp_CFRep = 0;
    STFS_OpenIdentifier        *lp_OpenId;

    switch (lv_MessageType) {
    case MT_CloseFileReply:
      lp_CFRep = (STFSMessage_CloseFileReply *) lp_Response;
      lp_OpenId = lp_CFRep->GetOpenIdentifier();
      TRACE_PRINTF2(3, "Open identifier in reply: %ld\n", lp_OpenId->openIdentifier);
      break;
    default:
      break;
    }

    delete lp_Response;
  
    return lv_Stat;
  }

  ///////////////////////////////////////////////////////////////////////////////
  ///
  //         SendToSTFSd_unlink()
  ///
  /// \brief Packages a STFSMessage_UnlinkFileRequest message, dispatches it to 
  ///        to the STFSd, processes the response message
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_unlink( const char                 *pp_Path,
                      int                         pv_OpenerNodeId,
                      int                         pv_OpenerPID)
  {
    const char       *WHERE = "SendToSTFSd_unlink";
    STFS_ScopeTrace   lv_st(WHERE,2);

    STFSMessage_UnlinkFileRequest *lp_Cfr = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    lp_Cfr = new STFSMessage_UnlinkFileRequest(pp_Path,
                                               pv_OpenerNodeId,
                                               pv_OpenerPID,
                                               lp_ReqMsgBuffer,
                                               sizeof(lp_ReqMsgBuffer));
    lp_Cfr->SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lp_Cfr->GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
    lp_Cfr->SetMessageBufIOInProgress(false);
  
    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);

    Enum_MessageType            lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_UnlinkFileReply *lp_CFRep = 0;
    char                        *lp_RspFileName = 0;

    switch (lv_MessageType) {
    case MT_UnlinkFileReply:
      lp_CFRep = (STFSMessage_UnlinkFileReply *) lp_Response;
      lp_RspFileName = lp_CFRep->GetFileName();
      TRACE_PRINTF2(3, "File Name in reply: %s\n", lp_RspFileName);
      break;
    default:
      break;
    }
  
    return lv_Stat;
  }


  ///////////////////////////////////////////////////////////////////////////////
  ///
  //         SendToSTFSd_mkstemp()
  ///
  /// \brief Packages a STFSMessage_CreateFileRequest message, dispatches it to 
  ///        to the STFSd, processes the response message
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_mkstemp(char                       *pp_Ctemplate,
		      int                         pv_OpenerNodeId,
		      int                         pv_OpenerPID,
		      STFS_ExternalFileMetadata *&pp_Efm,
		      STFS_OpenIdentifier       *&pp_OpenId)
  {
    const char       *WHERE = "SendToSTFSd_mkstemp";
    STFS_ScopeTrace   lv_st(WHERE,2);

    //STFSMessage_CreateFileRequest *lp_Cfr = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    STFSMessage_CreateFileRequest lv_Cfr(pp_Ctemplate,
                                         true,
                                         pv_OpenerNodeId,
                                         pv_OpenerPID,
                                         lp_ReqMsgBuffer,
                                         sizeof(lp_ReqMsgBuffer));
    // Rev: Refactor Start (Have this all in a single method)
    lv_Cfr.SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lv_Cfr.GetMessageBuf();

#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
    // Rev: Check if there is an Error Message right at the spot here. 
  
    lv_Cfr.SetMessageBufIOInProgress(false);

    if (lv_Stat != 0) {
      return (lv_Stat);
    }
    // Rev: Refactor End
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);
    Enum_MessageType lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_CreateFileReply   *lp_CFRep = 0;
    STFS_OpenIdentifier            lv_OpenId;
    STFS_ExternalFileMetadata     *lp_UnpackedEfm = 0;
    STFS_ExternalFileMetadata     *lp_ExistingEfm = 0;
    STFSMessage_ErrorReply        *lp_ErrorRep = 0;

    switch (lv_MessageType) {
    case MT_CreateFileReply:
      lp_CFRep = (STFSMessage_CreateFileReply *) lp_Response;
      lp_CFRep->GetTemplate(pp_Ctemplate, STFS_NAME_MAX);
      lv_OpenId = lp_CFRep->GetOpenIdentifier();
      pp_OpenId = new STFS_OpenIdentifier();
      memcpy(pp_OpenId, &lv_OpenId, sizeof(STFS_OpenIdentifier));
      lp_UnpackedEfm = lp_CFRep->Unpack();

      if (!lp_UnpackedEfm) {
	// Rev: Check if errno is set in the Session object, else set the errno
	//      Error handling to be done in Unpack()
	//      Method/macro to check for errors
	// Rev: Also Assert lp_UnpackedEfm otherwise.
        return -1;
      }

      lp_ExistingEfm = STFS_ExternalFileMetadata::GetFromContainer(pp_Ctemplate);
#ifdef SQ_STFSD
      if (lp_ExistingEfm) {
        TRACE_PRINTF2(1, "Error: An EFM already exists in the EFM container for %s\n",
                      pp_Ctemplate);
	// Rev: set errno = EEXISTS
        return -1;
      }
      //insert EFM in a global map
      lv_Stat = STFS_ExternalFileMetadata::InsertIntoContainer(lp_UnpackedEfm);
      if (lv_Stat < 0) {
	// Rev: Check if errno is set in the Session object, else set the errno
	// TBD : cleanup
        return -1;
      }
      pp_Efm = lp_UnpackedEfm;
#else
      // In a single process, the EFM container is common between the STFSLib & STFSd
      // and hence at this point there 'should' be an EFM in the container for the 
      // given template.
      if (lp_ExistingEfm) {
        delete lp_UnpackedEfm;
        pp_Efm = lp_ExistingEfm;
      }
      else {
        return -1;
      }
#endif

      break;

    case MT_ErrorReply:
      // Rev: The refactored call to STFSd_send should take care of this case
      lp_ErrorRep = (STFSMessage_ErrorReply *) lp_Response;
      return lp_ErrorRep->GetReportedError();
      break;

    default:
      // Rev: Assert here
      // Rev: set errno to EBADMSG
      break;
    }
  
    return lv_Stat;
  }


  ///////////////////////////////////////////////////////////////////////////////
  ///
  //         SendToSTFSd_open()
  ///
  /// \brief Packages a STFSMessage_OpenFileRequest message, dispatches it to 
  ///        to the STFSd, processes the response message
  ///
  ///
  ///////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_open(char                       *pp_FileName,
                   int                         pv_OpenFlag,
                   int                         pv_OpenerNodeId,
                   int                         pv_OpenerPID,
                   STFS_ExternalFileMetadata *&pp_Efm,
                   STFS_FragmentFileMetadata *&pp_Ffm,
                   STFS_OpenIdentifier       *&pp_OpenId)
  {
    const char       *WHERE = "SendToSTFSd_open";
    STFS_ScopeTrace   lv_st(WHERE,2);

    STFSMessage_OpenFileRequest *lp_Ofr = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    lp_Ofr = new STFSMessage_OpenFileRequest(pp_FileName,
                                             pv_OpenFlag,
                                             pv_OpenerNodeId,
                                             pv_OpenerPID,
                                             lp_ReqMsgBuffer,
                                             sizeof(lp_ReqMsgBuffer));
    lp_Ofr->SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lp_Ofr->GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif

    lp_Ofr->SetMessageBufIOInProgress(false);
    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);

    Enum_MessageType           lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_OpenFileReply *lp_OFRep = 0;
    STFS_OpenIdentifier        lv_OpenId;
    STFS_ExternalFileMetadata *lp_UnpackedEfm = 0;
    STFS_ExternalFileMetadata *lp_ExistingEfm = 0;

    switch (lv_MessageType) {
    case MT_OpenFileReply:
      lp_OFRep = (STFSMessage_OpenFileReply *) lp_Response;
      lv_OpenId = lp_OFRep->GetOpenIdentifier();
      pp_OpenId = new STFS_OpenIdentifier();
      memcpy(pp_OpenId, &lv_OpenId, sizeof(STFS_OpenIdentifier));
      lp_UnpackedEfm = lp_OFRep->Unpack();
      if (!lp_UnpackedEfm) {
        //TBD Error
        return -1;
      }

      lp_ExistingEfm = STFS_ExternalFileMetadata::GetFromContainer(pp_FileName);
      if (lp_ExistingEfm) {
#if 0
        // TBD Update an existing EFM
        lv_Stat = STFS_ExternalFileMetadata::DeleteFromContainer(lp_ExistingEfm);
        if (lv_Stat < 0) {
          //TBD Error cleanup
          return -1;
        }
#endif
        pp_Efm = lp_ExistingEfm;
        delete lp_UnpackedEfm;
      }
      else {
        lv_Stat = STFS_ExternalFileMetadata::InsertIntoContainer(lp_UnpackedEfm);
        if (lv_Stat < 0) {
          //TBD cleanup   
          return -1;
        }
        pp_Efm = lp_UnpackedEfm;
      }
      pp_Ffm = pp_Efm->GetFragment(0);

      break;

    case MT_ErrorReply:
      //TBD
      break;

    default:
      break;
    }
  
    return lv_Stat;
  }

////////////////////////////////////////////////////////////////////////////////
///
//       SendToSTFSd_createFrag
//
/// \brief Sends create fragment request to STFSd
///
/// \retval int SUCCESS:  0
///             FAILURE: -1
////////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_createFrag ( STFS_ExternalFileMetadata *&pp_Efm,
			   int                         pv_OpenerNodeId,
			   int                         pv_OpenerPID,
			   STFS_OpenIdentifier       *pp_OpenId) {

    const char       *WHERE = "SendToSTFSd_createFrag";
    STFS_ScopeTrace   lv_st(WHERE,2);

    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    STFSMessage_CreateFragmentRequest lv_Cfr(pp_OpenId,
					     pv_OpenerNodeId,
					     pv_OpenerPID,
					     lp_ReqMsgBuffer,
					     sizeof(lp_ReqMsgBuffer));
    lv_Cfr.SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lv_Cfr.GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
  
    lv_Cfr.SetMessageBufIOInProgress(false);

    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);
    Enum_MessageType lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_CreateFragmentReply   *lp_CFRep = 0;
    STFS_ExternalFileMetadata     *lp_UnpackedEfm = 0;
    STFSMessage_ErrorReply        *lp_ErrorRep = 0;

    switch (lv_MessageType) {
    case MT_CreateFragmentReply:
      lp_CFRep = (STFSMessage_CreateFragmentReply *) lp_Response;
      lp_UnpackedEfm = lp_CFRep->Unpack();

      if (!lp_UnpackedEfm) {
        //TBD Error
        return -1;
      }

      // update the existing EFM to include the new FFMs

#ifdef SQ_STFSD
      // Verify that the number of fragments changed, and link in the new
      // fragments
      if (pp_Efm->GetNumFragments()<lp_UnpackedEfm->GetNumFragments()) {

	// new fragments found, hook them onto the main EFM

	for (unsigned int i = pp_Efm->GetNumFragments(); 
	     i < lp_UnpackedEfm->GetNumFragments(); 
	     i++) {

	  //copy the FFM

	  lv_Stat = pp_Efm->CopyAndAppendFFM (lp_UnpackedEfm->GetFragment (i));
	  if (lv_Stat <0) {
	    // Huh?  Memory error, probably.
	    return -1;
	  }
	}
      } // if we have fragments to append
      else {

	// This is a create fragment reply, darnit.  We should have create at
	// least one fragment!  

	ASSERT (false);

	//Oh well, everything's consistent, so we could
	// just mush on in the non-debug case....
      }
#else
      // In a single process, the EFM container is common between the STFSLib & STFSd
      // and hence at this point there 'should' be an EFM in the container for the 
      // given template, and it should be up to date WRT the number of
      // fragments.  So theoretically, we shouldn't get here except when
      // tortured into submission via the debugger or other coercive means.
      
      ASSERT (pp_Efm->GetNumFragments()<=lp_UnpackedEfm->GetNumFragments() );
#endif

      break;

    case MT_ErrorReply:
      lp_ErrorRep = (STFSMessage_ErrorReply *) lp_Response;
      return lp_ErrorRep->GetReportedError();
      break;

    default:
      // Unexpected reply!
      ASSERT (false);
      return -1;
      break;
    }
  
    return lv_Stat;
  }

///////////////////////////////////////////////////////////////////////////////
///
//         SendToSTFSd_read()
///
/// \brief Sends Nid, Path, and OpenersSet data to STFSd for Openers function
///
/// \retval long SUCCESS:  0
///              FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_read()
#if 0
                       stfs_fhndl_t                pv_Fhandle,      
                       struct STFS_OpenersSet     *pp_OpenersSet,
                       int                         pv_OpenerNodeId,
                       int                         pv_OpenerPID)
#endif
  {
    const char       *WHERE = "SendToSTFSd_read";
    STFS_ScopeTrace   lv_st(WHERE,2);

#if 0
    STFSMessage_FOpenersRequest *lp_OpR = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    //To avoid error, remove later...
    if(!pp_OpenersSet) {
      return -1;
    }
  
    lp_OpR = new STFSMessage_FOpenersRequest(pv_Fhandle,
                                             pp_OpenersSet,
                                             pv_OpenerNodeId,
                                             pv_OpenerPID,
                                             lp_ReqMsgBuffer,
                                             sizeof(lp_ReqMsgBuffer));
    lp_OpR->SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lp_OpR->GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
  
    lp_OpR->SetMessageBufIOInProgress(false);

    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);
    Enum_MessageType lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_FOpenersReply   *lp_FOpenersRep = 0;
    STFS_OpenersSet lv_FOpenersCopy;

    switch (lv_MessageType) {
    case MT_FOpeners:
      lp_FOpenersRep = (STFSMessage_FOpenersReply *) lp_Response;
      lv_FOpenersCopy = lp_FOpenersRep->GetFOpenersSet();
      memcpy(pp_OpenersSet, &lv_FOpenersCopy, sizeof(STFS_OpenersSet));
      break;

    default:
      break;
    }
  
    return lv_Stat;
#endif 
    return 0;
  }

///////////////////////////////////////////////////////////////////////////////
///
//         SendToSTFSd_fopeners()
///
/// \brief Sends Nid, Path, and OpenersSet data to STFSd for Openers function
///
/// \retval long SUCCESS:  0
///              FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_fopeners(stfs_fhndl_t                pv_Fhandle,      
                       struct STFS_OpenersSet     *pp_OpenersSet,
                       int                         pv_OpenerNodeId,
                       int                         pv_OpenerPID)
  {
    const char       *WHERE = "SendToSTFSd_fopeners";
    STFS_ScopeTrace   lv_st(WHERE,2);

    STFSMessage_FOpenersRequest *lp_OpR = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    //To avoid error, remove later...
    if(!pp_OpenersSet) {
      return -1;
    }
  
    lp_OpR = new STFSMessage_FOpenersRequest(pv_Fhandle,
                                             pp_OpenersSet,
                                             pv_OpenerNodeId,
                                             pv_OpenerPID,
                                             lp_ReqMsgBuffer,
                                             sizeof(lp_ReqMsgBuffer));
    lp_OpR->SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lp_OpR->GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
  
    lp_OpR->SetMessageBufIOInProgress(false);

    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);
    Enum_MessageType lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_FOpenersReply   *lp_FOpenersRep = 0;
    STFS_OpenersSet lv_FOpenersCopy;

    switch (lv_MessageType) {
    case MT_FOpeners:
      lp_FOpenersRep = (STFSMessage_FOpenersReply *) lp_Response;
      lv_FOpenersCopy = lp_FOpenersRep->GetFOpenersSet();
      memcpy(pp_OpenersSet, &lv_FOpenersCopy, sizeof(STFS_OpenersSet));
      break;

    default:
      break;
    }
  
    return lv_Stat;
  }

///////////////////////////////////////////////////////////////////////////////
///
//         SendToSTFSd_Openers()
///
/// \brief Sends Nid, Path, and OpenersSet data to STFSd for Openers function
///
/// \retval long SUCCESS: Count of the Openers Set
///              FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_openers(stfs_nodeid_t               pv_Nid,
                      char                       *pp_Path,
                      struct STFS_OpenersSet     *pp_OpenersSet,
                      int                         pv_OpenerNodeId,
                      int                         pv_OpenerPID)
  {
    const char       *WHERE = "SendToSTFSd_openers";
    STFS_ScopeTrace   lv_st(WHERE,2);

    STFSMessage_OpenersRequest *lp_OpR = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    lp_OpR = new STFSMessage_OpenersRequest(pv_Nid,
                                            pp_Path,
                                            pp_OpenersSet,
                                            pv_OpenerNodeId,
                                            pv_OpenerPID,
                                            lp_ReqMsgBuffer,
                                            sizeof(lp_ReqMsgBuffer));
    lp_OpR->SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lp_OpR->GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
  
    lp_OpR->SetMessageBufIOInProgress(false);

    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);
    Enum_MessageType lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_OpenersReply   *lp_OpenersRep = 0;
    STFS_OpenersSet lv_OpenersCopy;

    switch (lv_MessageType) {
    case MT_Openers:
      lp_OpenersRep = (STFSMessage_OpenersReply *) lp_Response;
      pv_Nid = lp_OpenersRep->GetNid();
      strcpy(pp_Path, lp_OpenersRep->GetPath());
      lv_OpenersCopy = lp_OpenersRep->GetOpenersSet();
      memcpy(pp_OpenersSet, &lv_OpenersCopy, sizeof(STFS_OpenersSet));
      break;

    default:
      break;
    }
  
    return lv_Stat;
  }

///////////////////////////////////////////////////////////////////////////////
///
//         SendToSTFSd_stat()
///
/// \brief Sends Nid, Path, and OpenersSet data to STFSd for Openers function
///
/// \retval long SUCCESS: Count of the Openers Set
///              FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
  int
  SendToSTFSd_stat ( stfs_nodeid_t               pv_Nid,
                     char                       *pp_Path,
                     struct STFS_StatSet        *pp_StatSet,
                     stfs_statmask_t             pv_Mask, 
                     int                         pv_OpenerNodeId,
                     int                         pv_OpenerPID)
  {
    const char       *WHERE = "SendToSTFSd_stat";
    STFS_ScopeTrace   lv_st(WHERE,2);

    STFSMessage_StatRequest *lp_OpR = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

    lp_OpR = new STFSMessage_StatRequest(pv_Nid,
                                         pp_Path,
                                         pp_StatSet,
                                         pv_Mask,
                                         pv_OpenerNodeId,
                                         pv_OpenerPID,
                                         lp_ReqMsgBuffer,
                                         sizeof(lp_ReqMsgBuffer));

    lp_OpR->SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lp_OpR->GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                    &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                  &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
  
    lp_OpR->SetMessageBufIOInProgress(false);

    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);
    Enum_MessageType lv_MessageType = lp_Response->GetMessageType();
    STFSMessage_StatReply *lp_StatRep = 0;
    STFS_StatSet lv_StatCopy;

    switch (lv_MessageType) {
    case MT_Stat:
      lp_StatRep = (STFSMessage_StatReply *) lp_Response;
      strcpy(pp_Path, lp_StatRep->GetPath());
      lv_StatCopy = lp_StatRep->GetStatSet();
      memcpy(pp_StatSet, &lv_StatCopy, sizeof(STFS_StatSet));
      break;

    default:
      break;
    }
  
    return lv_Stat;
  }

///////////////////////////////////////////////////////////////////////////////
///
//         SendToSTFSd_getEFM
///
/// \brief Sends Open ID to daemon and gets latest EFM 
///
/// \retval long SUCCESS:  0
///              FAILURE: -1
///
///////////////////////////////////////////////////////////////////////////////
#if 0
  int
  SendToSTFSd_getEFM(STFS_OpenIdentifier        *pp_OpenId,
                     STFS_ExternalFileMetadata  *pp_Efm,
                     int                         pv_OpenerNodeId,
                     int                         pv_OpenerPID)
{
    const char       *WHERE = "SendToSTFSd_getEFM";
    STFS_ScopeTrace   lv_st(WHERE,2);

    STFSMessage_FOpenersRequest *lp_OpR = 0;
    char lp_ReqMsgBuffer[STFS_MSGBUFFER_MAX];
    char lp_RspMsgBuffer[STFS_MSGBUFFER_MAX];
    int  lv_ReqLen=sizeof(lp_ReqMsgBuffer);
    int  lv_RspLen=sizeof(lp_RspMsgBuffer);
    int  lv_Stat = 0;

  
    lp_OpR = new STFSMessage_GetEFMRequest(pv_Fhandle,
                                           pp_OpenersSet,
                                           pv_OpenerNodeId,
                                           pv_OpenerPID,
                                           lp_ReqMsgBuffer,
                                           sizeof(lp_ReqMsgBuffer));
    lp_OpR->SetMessageBufIOInProgress(true);

    char *lp_BufferToSend = (char *) lp_OpR->GetMessageBuf();
  
#ifdef SQ_STFSD
    lv_Stat = STFS_util::SendToSTFSd(lp_BufferToSend,
                                     lv_ReqLen,
                                     lp_RspMsgBuffer,
                                     &lv_RspLen);
#else
    lv_Stat = STFSd_RequestHandler(lv_ReqLen,
                                   lp_BufferToSend,
                                   &lv_RspLen,
                                   lp_RspMsgBuffer);
#endif
  
    lp_OpR->SetMessageBufIOInProgress(false);

    if (lv_Stat != 0) {
      return (lv_Stat);
    }
  
    STFS_Message *lp_Response = new STFS_Message();
    lp_Response->SetMessageBuf(lp_RspMsgBuffer, lv_RspLen, true);
   // Enum_MessageType lv_MessageType = lp_Response->GetMessageType();
    
    //TODO: Update this
    STFSMessage_FOpenersReply   *lp_FOpenersRep = 0;
    STFS_OpenersSet lv_FOpenersCopy;

    switch (lv_MessageType) {
    case MT_FOpeners:
      lp_FOpenersRep = (STFSMessage_FOpenersReply *) lp_Response;
      lv_FOpenersCopy = lp_FOpenersRep->GetFOpenersSet();
      memcpy(pp_OpenersSet, &lv_FOpenersCopy, sizeof(STFS_OpenersSet));
      break;

    default:
      break;
    }
    return lv_Stat;
  }
#endif
} //namespace STFS
