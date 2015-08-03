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

#include "sqtypes.h"

#define DLLEXPORT
#define TMFLIBAPI

#define Boolean       short

TMFLIBAPI int_16 ATTACHTMF()
{ return 0; }
TMFLIBAPI int_16 ATTACHTMFSEGMENT
  (

   int_16  TmfSegmentNumber) /* Input: TmfSegment which the caller process*/
/*        claims affinity to.*/
{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_ABORTTRANS_
  (

   int_16  *TcbRef,     /* IN: The transaction to abort.*/

   int_16   AbortError) /* IN: The error indicating why the transaction is being*/
                        /*   aborted.*/

{ return 0; }

/*dif2perl manually apply patch 0 djw*/
TMFLIBAPI int_16/*prv*/TMFLIBFS_ABORTTRENT_
  (
   int_16 *TcbRef_) /* IN: The transaction to abort.*/

{ return 0; }
/*dif2perl end patch 0 djw*/

TMFLIBAPI void /*prv*/TMFLIBFS_ATTACHTMF_()
   /*
   //------------------------------------------------------------------------------
   //
   // This function is called by the File System when the Trent array in the TFile
   // of a user process is allocated.  It is assumed that when the File System
   // invokes this function, the user process does not have any outstanding
   // transactions.  If the attach fails, the previous attachment is maintained or
   // NSKService will be brought down on this NT node.
   //
   //------------------------------------------------------------------------------
   */

{ return; }
TMFLIBAPI void/*prv*/TMFLIBFS_ATTACHTRANS_
  (/*
   //------------------------------------------------------------------------------
   //
   //   Used by the File System to tell TMF it is interested in the
   //   outcome of a transaction.
   //
   //   Used by 'BEGINTRANSACTION', 'SETSYNCINFO', and 'TmfLibFs_CheckOpenTFile'.
   //   'SETSYNCINFO' uses it for 'BEGINTRANSACTION' checkpoints.
   //
   //   A TRENT entry for the transaction must exist while a transaction
   //   is attached.  The TRENT entry will be updated with the transaction
   //   result error (Disposition) in an interrupt environment (i.e.
   //   BUSRECIEVE of a TMF packet) when the transaction goes Phase 1.
   //
   //   This routine can be called under MUTEX if necessary.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef,     /* IN: The transaction to attach. */

   int_16 *Found,      /* OUT: Returns 'True' if a TCB for the transaction was */
                       /*   Found. */

   int_16 *TransError) /* OUT: The transaction result error is returned. */
                       /*   Only valid when 'Found' returns 'True'.  If */
                       /*   'FEOK' is returned, the transaction has been */
                       /*   attached. */

{ return; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_BEGINTRANS_
  (/*
   //------------------------------------------------------------------------------
   //
   //   This procedure is called by BEGINTRANSACTION to begin a transaction in
   //   the local system.    TmfLib's data structures must
   //   be fully reloaded in this cpu (TmfLibSegment_IsMyRlodComplete_ must
   //   be True).
   //
   //   Returns 'FEOK' if successful.
   //
   //   Failure errors include:
   //
   //       'FETMFNOTRUNNING' --
   //           TMF is not started.
   //
   //       'FEBEGINTRDISABLED' --
   //           Beginning transactions has been disabled.
   //
   //       'FENOMORETCBS' --
   //           No more transaction control blocks
   //           available.
   //
   //------------------------------------------------------------------------------
   */
/*
// dif2perl 1 update BEGIN
// dif2perl 1 delete BEGIN
// dif2perl 1 (delete):
// dif2perl 1 (delete):    int_16  *TcbRef,                // OUT: The transaction identifier of the new
// dif2perl 1 (delete):                                    //   transaction.
// dif2perl 1 (delete):
// dif2perl 1 (delete):    int_16   RetryOnResourceErrors) // IN: Specify 'True' to automatically retry the
// dif2perl 1 delete END
// dif2perl 1 insert BEGIN
*/

   int_16  *TcbRef,  /* OUT: The transaction identifier of the new */
                                   /*   transaction. */

   int_16   RetryOnResourceErrors,
                     /* IN: Specify 'True' to automatically retry the */
/*
// dif2perl 1 insert END
// dif2perl 1 update END
*/
                                   /*
                                   //   begin operation if it fails due to lack of
                                   //   resources (e.g. "out of TCBS").  Before
                                   //   retrying, this routine will ask Guardian 90 if
                                   //   there is a queued STOP on pending on the
                                   //   process and will quit if there is.
                                   //
                                   //   Multi-threaded requestors specify 'False'
                                   //   since they can't wait and single-threaded
                                   //   requestors specify 'True' to be resilient.
                                   */
/*dif2perl patch 2 djw*/
	DblInt   Timeout) /* IN: Time remaining in seconds, for the transaction to */
                     /*     abort. */
/*dif2perl patch 2 djw*/

{ return 0; }
TMFLIBAPI void/*prv*/TMFLIBFS_CHECKOPENTFILE_
  (int_16 *TFilePtr)

{ return; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_CANCEL_RECV_
  (/*
   //----------------------------------------------------------------------------
   //
   //dif2perl patch 3 djw
   // This procedure is No-op.
   //dif2perl end patch 3 djw
   //----------------------------------------------------------------------------
   */

   int_32  FileTag)

{ return 0; }
TMFLIBAPI void /*prv*/TMFLIBFS_CLEANUPATTACH_
  (/*
   //----------------------------------------------------------------------------
   //
   // This function is called by the FsCleanUp process on NT.  The FsCleanUp
   // process retrieves the TmfSegmentNumber from the PCB of the terminated
   // and calls this function to perform an attach before calling
   // TMFLIBFS_ABORTALLTRANSACTIONS_.
   //
   //------------------------------------------------------------------------------
   */

   int_16  TmfSegmentNumber)  /*IN: The number of the TmfSegment used previously by */
                              /*    the terminated process for TmfSegment affinity */

{ return; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_CLOSE_
  (/*
   //----------------------------------------------------------------------------
   //dif2perl patch 4 begin djw
   // This procedure is called by the File System when an RM file is being
   // closed due to a user issuing a CLOSE or a process STOP.
   //
   // This procedure initiates the close processing of the RM file by calling
   // the TmfLibRm_Close_ procedure.
   //
   // Any error returned from the TmfLibRm_Close_ procedure is returned to
   // the caller. An error from this procedure indicates that there is an
   // inconsistency in the interface between the TMF Library and File System.
   //
   // Other possible error is:
   //
   //     FEBADPARMVALUE    - Invalid FileTag parameter supplied.
   //
   // The calling process MUST BE unstoppable when this procedure is called.
   //
   // This procedure MUST NOT BE called under MUTEX.
   //
   //dif2perl end patch 4 djw
   //----------------------------------------------------------------------------
   */

   int_32 *FileTag) /* IN/OUT: File tag associated with the RM file to be closed. */
                    /*         If there are no errors, on the output, FileTag */
                    /*         will be set to a NIL address. */

{ return 0; }
/*dif2perl patch 5*/
TMFLIBAPI int_16/*prv*/TMFLIBFS_DELETETRENT_
  (/*
   //------------------------------------------------------------------------------
   //
   //   Requests a TRENT to be deleted.
   //
   //   This procedure MUST BE called under MUTEX.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef_) /* IN: The transaction to abort. */

{ return 0; }

TMFLIBAPI void/*prv*/TMFLIBFS_DETACHTRANS_
  (/*
   //------------------------------------------------------------------------------
   //
   //   Used to terminate the File System's obligation for a transaction,
   //   ignoring the Disposition.
   //
   //   This procedure can be called under MUTEX.
   //
   //   No error is returned --> This procedure is a no-op if the transaction
   //   has gone away.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef) /* IN: The transaction to detach. */

{ return; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_DONE_SIGNAL_
  (/*
   //----------------------------------------------------------------------------
   //dif2perl patch 6 djw
   // This procedure is called by the File System when it is done with the
   // copying of the signal data from the TmfLibrary data area to the user data
   // area. A pointer to signal data and its size is returned to the File System
   // in the TmfLibFs_Listen_Signal_ call.
   //
   // The File System calls the TmfLibFs_Listen_Signal_ procedure to get
   // the signal data and its size. After that, it copies the signal data from
   // the TmfLibrary data area to the user data area and calls the
   // TmfLibFs_Done_Signal_ procedure to indicate to the TmfLibrary to get
   // rid of its resources related to the signal.
   //
   // This procedure calls TmfLibRm_Done_Signal_ to indicate to the
   // TmfLibRm module, that the resources allocated to the signal can be
   // disposed of.
   //
   // Any error returned from the TmfLibRm_Done_Siganl_ procedure is returned to
   // the caller.
   //
   // Possible error is:
   //
   //     FEBADPARMVALUE    - Invalid SignalTag parameter supplied.
   //
   // The calling process MUST BE unstoppable when this procedure is called.
   //
   // This procedure CAN BE called under MUTEX.
   //
   //----------------------------------------------------------------------------
   */

   int_32 *SignalTag) /* IN/OUT: The tag associated with the signal. If there is no */
                      /*         error, on the output, SignalTag will be set to  a */
                      /*         NIL address. */

   /*dif2perl patch 6 end*/
{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_ENDTRANS_
  (/*
   //------------------------------------------------------------------------------
   //
   //   Requests a transaction begun by this process be committed, but
   //   does not wait for it to finish.  TmfLib's data structures must
   //   be fully reloaded in this cpu (TmfLibSegment_IsMyRlodComplete_ must
   //   be True).
   //
   //   Returns 'FEOK' if the transaction is being finished (or already
   //   finished).
   //
   //   The only other error that can be returned is 'FETRANSNOWAITOUT'.
   //   It is returned when 'PendingCount' (from the TRENT entry) is
   //   greater than 0 and the transaction is still active.
   //
   //------------------------------------------------------------------------------
   */

   int_16  *TcbRef,       /* IN: The transaction to commit. */

   int_32   PendingCount) /* IN: The 'PendingCount' field from the TRENT. */

{ return 0; }
/*dif2perl patch 7*/
TMFLIBAPI void/*prv*/TMFLIBFS_GETBEGINTAG_
  (/*
   //------------------------------------------------------------------------------
   //
   //   This procedure computes the File System TAG, given a TcbRef.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef_,  /* IN: The TcbRef of the transaction. */

   int_32 *BeginTag) /* OUT: The File System TAG associated with the */
                     /*      transaction. */

{ return; }
/*dif2perl patch 7 end*/

TMFLIBAPI int_16/*prv*/TMFLIBFS_GETCACHEDREMOTETCBREF_
  (/*
   //------------------------------------------------------------------------------
   //
   //   The File System gets the TCBREF for a Remote System cached by the
   //   TMF Library with this routine.
   //
   //   If the TMF Library does not have the TCBREF cached, the caller
   //   must send a message to the TMP by calling TmfLibFs_PrepareToGoRemote_.
   //
   //   Returns 'FEOK' if the transaction is still valid.
   //
   //   Failure errors include:
   //
   //       'FEINVTRANSID' --
   //           Transaction has aborted and been cleaned up.
   //
   //       'FETMFNOTRUNNING' --
   //           A TMF shutdown occurred on the local system.
   //
   //------------------------------------------------------------------------------
   */

   int_16  *TcbRef,             /* IN: The transaction the File System is working on. */

   int_32   SysNum,             /* IN: The remote system to participate in the */
                                /*   transaction. */

   int_16  *MessageRequired,    /* OUT: Returns 'True' if the TCBREF was not cached */
                                /*   and a message must be sent to the TMP using the */
                                /*   'TmfLibFs_SendPrepareToGoRemote_' routine. */
                                /*   Otherwise, the Remote TCBREF is returned in */
                                /*   in the 'TcbRefForRemoteSys' parameter. */
                                /* */
                                /*   This parameter is valid if the function return */
                                /*   is 'FEOK'. */

   int_16  *BufferLen,          /* OUT: If MessageRequired returns True, then this is */
                                /*      the size of the Buffer, in bytes, that will be */
                                /*      needed by the TmfLibFs_SendPrepareToGoRemote_ */
                                /*      routine. */

   int_16  *TcbRefForRemoteSys) /* OUT: The TCBREF to send to the Remote System. */
                                /* */
                                /*   This parameter is valid if the function return */
                                /*   is 'FEOK' and 'MessageRequired' returns 'False'. */

{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_GETTRANSID_
  (/*
   //------------------------------------------------------------------------------
   //
   //   Returns the transaction id given a TCBREF.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef,  /* IN: The TCBREF. */

   int_16 *TransId) /* OUT: If 'Error' is 'FEOK', the transaction id. */

{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_LISTEN_SIGNAL_
  (/*
   //----------------------------------------------------------------------------
   //
   // This procedure is called by the File System to READ a signal related to
   // an RM file. The FileTag parameter identifies the RM file for which the
   // READ is requested. If the FileTag parameter supplied is a Nil^Addr, then a
   // signal related to any of the RM files opened by the process is returned.
   //
   // This procedure calls the TmfLibRm_Listen_Signal_ procedure in order to
   // return a signal to the caller.
   //
   // Any error returned from the TmfLibRm_Listen_Signal_ procedure is returned
   // to the caller.
   //
   // Possible errors are:
   //
   //     FEBADPARMVALUE - Invalid parameters supplied.
   //
   //     FENOTFOUND     - No signal is available currently.
   //
   // The calling process MUST BE unstoppable when this procedure is called.
   //
   // This procedure CAN BE called under MUTEX.
   //
   //----------------------------------------------------------------------------
   */

   int_32   FileTag,
   /* IN: The file tag associated with the RM file. */

   int_16  *FileNum,
   /* OUT: The RM file number associated with the signal. */

   int_32  *Buffer,
   /* OUT: The absolute address of the signal buffer. */

   int_16  *Size,
   /* OUT: The size of the signal buffer in bytes. */

   int_32  *SignalTag)

/* OUT: The signal tag associated with the signal. This tag */
/*      should be passed in the TmfLibFs_Done_Signal_ procedure. */
{ return 0; }
TMFLIBAPI void/*prv*/TMFLIBFS_NOTIFYAPPLIC_
  (int_16   *TcbPtr,  /*IN: The Tcb for the transaction. */

   Boolean   IsCrash) /*IN: True if this procedure is being called during Crash */
                      /*    processing.  False if not.  If True, then the application */
                      /*    will be told FEABORTEDTRANSID or FETRANSABORTEDMAYBE, */
                      /*    depending on the values of CompletedPh1EpochNum and */
                      /*    AbortFlags. */

/*
//----------------------------------------------------------------------------
// This procedure notifies the transaction beginner (or the beginner's
// backup or the resumer ) of the Disposition of the transaction.
//----------------------------------------------------------------------------
*/

{ return; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_PREPARETOGOREMOTE_
  (/*
   //------------------------------------------------------------------------------
   //
   //   Prepares a transaction so the File System can send it to another
   //   system.
   //
   //   TMF verifies the system is running TMF and is on a compatible
   //   release, etc.  The system is added to the list of system
   //   participating in the transaction.
   //
   //   This call may suspend this process for network i/o.
   //
   //   Returns 'FEOK' if the transaction can be sent.
   //
   //   Failure errors include:
   //
   //       'FEINVTransId' --
   //           Transaction has aborted and been cleaned up.
   //
   //       'FETMFNOTRUNNING' --
   //           TMF is not running on the remote system, or a TMF shutdown
   //           occurred on the local system.
   //
   //------------------------------------------------------------------------------
   */

   int_16  *TcbRef,             /* IN: The transaction the File System is working on. */

   int_32   SysNum,             /* IN: The remote system to participate in the */
                                /*   transaction. */

   int_16  *TcbRefForRemoteSys) /* OUT: The TCBREF to put in the message being */
                                /*  sent to the remote system. */

{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_PREPARETOGOREMOTEDONE_
  (/*
   //------------------------------------------------------------------------------
   //
   //   The File System completes a nowait message that was sent to TMP to
   //   get the TCBREF for a Remote System with this routine.
   //
   //   This routine handles retries.  This routine passes back the
   //   message id for the retried message.
   //
   //   The function return is 'FEOK' when the message was returned a
   //   Remote TCBREF and the transaction is still active.  Otherwise, the
   //   error can be relected back to the user and the buffer can be
   //   deallocated.
   //
   //------------------------------------------------------------------------------
   */

   int_32 *MsgId,                     /* IN OUT: The message id of the message sent. */
                                      /* */
                                      /*   If the function return is 'FEOK' and 'RETRIED' returns */
                                      /*   'True', the new message id is returned here. */

   int_16 *Buffer,                    /* IN: The same buffer allocated by the File System */
                                      /*   and specified earlier to the */
                                      /*   'TMFLIBFS_SENDPREPARETOGOREMOTE_' routine. */

   int_16 *Retried,                   /* OUT: Returns 'True' if the message had to be retried. */

   int_16 *TcbRef_For_Remote_Sys_Ptr) /* OUT: The TCBREF to send to the Remote System. */
                                      /* */
                                      /*   This parameter is valid if the function return */
                                      /*   is 'FEOK' and 'RETRIED' returns 'False'. */

{ return 0; }
TMFLIBAPI void/*prv*/TMFLIBFS_PROCESS_EXIT_
  (/*
   //----------------------------------------------------------------------------
   //
   // This procedure is called by the File System as part of STOP processing of
   // of the process.
   //
   // This procedure initiates the cleanup of the library data structures
   // related to the process by calling the TmfLibRm_Process_Exit_ procedure.
   //
   // This procdure does not return any error.
   //
   // If a process does not do any operations related to RM files, the process
   // tag stored in the PFS will be a Nil^Addr. In such as case, it is
   // allowed to call this procedure with a Nil^Addr.
   //
   // This procedure CAN BE called under MUTEX.
   //
   //----------------------------------------------------------------------------
   */

   int_32 *ProcessTag)

/* IN/OUT: The process tag associated with the calling process. */
/*         On the output, ProcessTag will be set to a NIL */
/*         address. */
{ return; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_RECV_SIGNAL_
  (/*
   //----------------------------------------------------------------------------
   //
   // This procedure is a No-op.
   //
   //----------------------------------------------------------------------------
   */

   int_32  FileTag)

{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_RESUMETRANSTEST_
  (/*
   //------------------------------------------------------------------------------
   //
   //   Returns 'True' if a transaction is still active and the transaction
   //   beginner's CPU is still up...
   //
   //   This procedure can be called under MUTEX.
   //
   //   *** Matches old behavior...
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRefPtr) /* IN: The TCBREF of the transaction to test. */

{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_SENDPREPARETOGOREMOTE_
  (/*
   //------------------------------------------------------------------------------
   //
   //   The File System sends a nowait message to TMP to get the TCBREF
   //   for a Remote System with this routine.
   //
   //   This routine passes back the message id for the message sent.
   //
   //   When the message completes, the 'TMFLIBFS_PREPARETOGOREMOTEDONE_'
   //   routine is called with the message id and buffer.
   //
   //   The function return is 'FEOK' when the message was successfully
   //   sent.  Otherwise, the error can be relected back to the user and
   //   the buffer can be deallocated.
   //
   //------------------------------------------------------------------------------
   */

   int_16  *TcbRef_Ptr,
                   /* IN: The transaction the File System is working on. */

   int_32   Sys_Num,
                   /* IN: The remote system to participate in the */
                   /*   transaction. */

   int_16  *Buffer,/* IN OUT: The buffer allocated by the caller.  It will be */
                   /*   used for private data, the request control, reply */
                   /*   control, etc. */

   int_16   Buffer_Len,
                   /* IN: The byte length of 'BUFFER'. The length is */
                   /*   the value returned by the */
                   /*   'TmfLibFs_GetCachedRemoteTcbRef_' routine. */

   int_32   Msg_Linker_Tag,
                   /* IN: The user defined tag for the 'MSG_LINK_'. */

   int_16   Msg_Link_Options,
                   /* IN: The options mask for 'MSG_LINK_'. */

   int_32  *MsgId) /* OUT: The message id of the message sent. */

{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_SENDRESPONSE_
  (/*
   //------------------------------------------------------------------------------
   //
   //   The File System calls this procedure before replying to ANY transactional
   //   request.
   //
   //   If there are EXPORT operations pending when this procedure is called,
   //   the process will be suspended waiting on LTMF. The process will be
   //   woken up when there are no more EXPORT operations pending.
   //
   //   The calling process MUST BE unstoppable when this procedure is called.
   //
   //   This procedure MUST NOT be called under MUTEX.
   //
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef_)

/* IN: The TcbRef associated with the transactional */
/*     request, for which the File System wants to reply. */
{ return 0; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_STATUSTRANS_
  (/*
   //------------------------------------------------------------------------------
   //
   //   Returns the Status of a transaction.
   //
   //   Returns 'FEOK' if successful.
   //
   //   Failure errors include:
   //
   //       'FETMFNOTRUNNING' --
   //           TMF is not started.
   //
   //       'FEINVTransId' --
   //           The transaction has committed or aborted, and
   //           been cleared from the system. Or, the
   //           transaction id is bogus.
   //
   //------------------------------------------------------------------------------
   */

   int_16   *TransIdOrTcbRef,
                     /* IN: The TransId or TcbRef of the transaction to get */
                     /* the Status for. */

   Boolean   IsTransId,
                     /* IN: True if TransIdOrTcbRef should be interpreted as */
                     /*     a TransId.  False if it is a TcbRef. */
   int_16   *Status) /* OUT: If 'Error' is 'FEOK', one of the */
                     /*   '*^TransStatus' literals is */
                     /*   returned. */
                     /* */

{ return 0; }
TMFLIBAPI void/*prv*/TMFLIBFS_TRANSLEAVINGPROCESS_
  (/*
   //------------------------------------------------------------------------------
   //
   //   File System folks: Please call this routine.
   //
   //   Well, perhaps more explanation is needed.  You must call this
   //   routine before a transaction leaves a process for the first time.
   //   This includes I/Os, checkpoints, 'GETSYNCINFO' calls, etc.
   //
   //   This call is cheap, so it does not hurt to call every time a
   //   transaction leaves the process.  It does mean the File System can
   //   not worry about calling this routine when it retries an I/O (resending
   //   a previously formatted message with the TCBREF), etc.
   //
   //   We are buffering our "begin PIO packet"s to the TMP CPUs and need
   //   this routine to make sure they have been sent before the
   //   transaction leaves the process.
   //
   //   This routine does not return an error.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef) /* IN: The transaction the File System is working on. */

{ return; }
TMFLIBAPI int_16/*prv*/TMFLIBFS_TRENTXFROMTAG_
  (int_16  *TFilePtr,
   int_32   Tag,
   int_16   IgnoreTrState)

{ return 0; }
/* dif2perl 13 insert BEGIN */
TMFLIBAPI int_16/*prv*/TMFLIBFS_WAITFORJOINACK_
  (/*
   //------------------------------------------------------------------------------
   //
   //   This procedure waits for the acknowledgment for the Join PIO packet.
   //
   //   If there are JOIN operations pending when this procedure is called,
   //   the process will be suspended waiting on LTMF. The process will be
   //   woken up when there are no more JOIN operations pending.
   //
   //   The calling process MUST BE unstoppable when this procedure is called.
   //
   //   This procedure MUST NOT be called under MUTEX.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef_) /* IN: The TcbRef associated with the joining transaction. */

{ return 0; }
/* dif2perl 13 insert END */
TMFLIBAPI int_16/*prv*/TMFLIBFS_WAITFORSUSPENDACK_
  (/*
   //------------------------------------------------------------------------------
   //
   //   This procedure waits for all the acknowledgments for the SUSPEND PIO packets.
   //
   //   If there are SUSPEND operations pending when this procedure is called,
   //   the process will delay for 10 milliseconds and check if the SUSPEND
   //   operations have completed. This will continue till all the SUSPEND
   //	operations have completed.
   //
   //   The calling process MUST BE unstoppable when this procedure is called.
   //
   //   This procedure MUST NOT be called under MUTEX.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef_) /* IN: The TcbRef associated with the suspended transaction. */

{ return 0; }
void TMFLIBFS_WAITFORRELOAD_
() /*
//----------------------------------------------------------------------------
// This procedure will suspend the calling process until this CPU's reload is
// complete.
//
// Must NOT be called under MUTEX.
//----------------------------------------------------------------------------
*/

{ return; }
/* dif2perl 14 insert BEGIN */
TMFLIBAPI int_16/*prv*/TMFLIBFS_WAITFORRESUMERESULT_
  (/*
   //------------------------------------------------------------------------------
   //
   //   This procedure waits for the result of a Resume operation. When a
   //   process tries to resume a transaction, a ResumeTx PIO packet is sent
   //   to the broadcast owner CPU. After validating this request, the broadcast
   //   CPU sends back a ResumeTxResult PIO packet giving the result of the
   //   resume operation.
   //
   //   This routine returns the error associated with the Resume operation.
   //
   //------------------------------------------------------------------------------
   */

   int_16 *TcbRef) /* IN: The transaction the File System is working on. */

{ return 0; }
/* dif2perl 14 insert END */

   /*EXPORT*//*TMFLIBAPI int_16 TmfLibFs_SendAUDITMSGTOCAT_(*/
          /*int_16*/ /*namelen*//*, int_16*/ /*audit*//*);*/

TMFLIBAPI int_16 TMFLIBFS_SENDAUDITMSGTOCAT_(unsigned char * param1,int_16 param2,int_16 param3){ return 0; }

int_16 TMFSTATEINFO() /* */
/* This procedure returns the state of TMF, according to the TCT in this CPU. */
/* */

{ return 0; }

TMFLIBAPI void TMFLIBFS_ABORTALLTRANSACTIONS_ (int_16 *TFilePtr) { return; }
/*SWE - This definition moved here from flbfsz.h.*/

TMFLIBAPI void Tmf_Mutex_On(){ return; }
TMFLIBAPI void Tmf_Mutex_Off(){ return; }
