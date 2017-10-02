/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
*****************************************************************************
*
* File:         UdrStreams.cpp
* Description:  Server-side message stream classes for UDRs
* Created:      06/10/2001
* Language:     C++
*
*
*****************************************************************************
*/

#include "UdrStreams.h"
#include "udrglobals.h"
#include "udrdefs.h"
#include "udrutil.h"
#include "sql_buffer.h"
#include "UdrDebug.h"
#include "spinfo.h"
#include "udrdecs.h"

#include "rosetta/rosgen.h"
#include "nsk/nskprocess.h"
#include "zsysc.h"

extern void processAnInvokeMessage(UdrGlobals *UdrGlob,
                                   UdrServerDataStream &msgStream,
                                   UdrDataBuffer &request);

extern void processAnRSFetchMessage(UdrGlobals *udrGlob,
                                    UdrServerDataStream &msgStream,
				    UdrHandle udrHandle,
				    RSHandle rsHandle,
                                    UdrDataBuffer *request);

extern void processAnRSContinueMessage(UdrGlobals *udrGlob,
                                       UdrServerDataStream &msgStream,
                                       UdrRSContinueMsg &request);

void UdrServerDataStream::actOnReceive(IpcConnection *conn)
{
  const char *moduleName = "UdrServerDataStream::actOnReceive()";

  IpcMessageObjType t;
  NABoolean somethingArrived = FALSE;

  //
  // A note about control flow in this method. It was originally
  // written under the assumption that only one message arrives at a
  // time. It would call getNextReceiveMsg() only once, then process
  // incoming objects, then return. It turns out this creates a memory
  // leak. Internally within the stream, the incoming message buffer
  // never moved from the "receive" list to the "in use" list and
  // never got cleaned up. Now this method calls getNextReceiveMsg()
  // until it returns FALSE. The result is that after the incoming
  // message has been processed, the next getNextReceiveMsg() call
  // will move the request buffer to the "in use" list where it later
  // gets cleaned up by a call to cleanupBuffers() or
  // releaseBuffers().
  //

  while (getNextReceiveMsg(t))
  {
    somethingArrived = TRUE;

    while (getNextObjType(t))
    {
      switch (t)
      {
        case UDR_MSG_DATA_HEADER:
        {
          // Do nothing for now except extract the object from the stream
          UdrDataHeader *h = new (receiveMsgObj()) UdrDataHeader(this);
          
        } // case UDR_MSG_DATA_HEADER
        break;
        
        case UDR_MSG_CONTINUE_REQUEST:
        {
          // Extract the object from the stream and call SPInfo's work()
          UdrContinueMsg *m = new (receiveMsgObj()) UdrContinueMsg(this);
          
          switch (spinfo_->getParamStyle())
            {
            case COM_STYLE_SQLROW_TM:
            case COM_STYLE_CPP_OBJ:
            case COM_STYLE_JAVA_OBJ:
              spinfo_->workTM();
              break;

            default:
              spinfo_->work();
              break;
            }
          
        } // case UDR_MSG_CONTINUE_REQUEST
        break;
        
        case UDR_MSG_DATA_REQUEST:
        {
          UdrDataBuffer *request = new (receiveMsgObj()) UdrDataBuffer(this);
          
          if (udrGlob_->verbose_ &&
              udrGlob_->traceLevel_ >= TRACE_DETAILS &&
              udrGlob_->showMain_)
          {
            ServerDebug( "");
            ServerDebug("[UdrServ (%s)] Invoke Request has %ld tupps.",
                        moduleName,
                        request->getSqlBuffer()->getTotalTuppDescs());
            ServerDebug("[UdrServ (%s)] Invoke Request SQL Buffer",
                        moduleName);
            displaySqlBuffer(request->getSqlBuffer(),
                             (Lng32) request->getSqlBufferLength());
          }
          
          udrGlob_->numReqInvokeSP_++;
          
          // Let the SPInfo process the request
	  spinfo_->setCurrentRequest(request);
	  spinfo_->work();

        } // case UDR_MSG_DATA_REQUEST
        break;

        case UDR_MSG_TMUDF_DATA_HEADER:
	{
	  // Extract the object. UDR Handle and RS Handle are
	  // interesting things in this message.
          UdrTmudfDataHeader *h = new (receiveMsgObj()) UdrTmudfDataHeader(this);
	  UdrHandle udrHandle = h->getHandle();

          NABoolean anyMore = getNextObjType(t);
          UDR_ASSERT(anyMore, "DATA_REQUEST must follow TMUDF DATA_HEADER");
          UDR_ASSERT(t == UDR_MSG_DATA_REQUEST,
                     "DATA_REQUEST must follow TMUDF DATA_HEADER");

          UdrDataBuffer *request = new (receiveMsgObj()) 
                                    UdrDataBuffer(this,
                                                  FALSE); //Avoid unpack.

          // Let the SPInfo process the request
	  spinfo_->setCurrentRequest(request);
	  spinfo_->workTM();
	}
        break;

        case UDR_MSG_RS_DATA_HEADER:
	{
	  // Extract the object. UDR Handle and RS Handle are
	  // interesting things in this message.
          UdrRSDataHeader *h = new (receiveMsgObj()) UdrRSDataHeader(this);
	  UdrHandle udrHandle = h->getHandle();
	  RSHandle rsHandle = h->getRSHandle();

          NABoolean anyMore = getNextObjType(t);
          UDR_ASSERT(anyMore, "DATA_REQUEST must follow RS_DATA_HEADER");
          UDR_ASSERT(t == UDR_MSG_DATA_REQUEST,
                     "DATA_REQUEST must follow RS_DATA_HEADER");

          UdrDataBuffer *request = new (receiveMsgObj()) UdrDataBuffer(this);

	  udrGlob_->numReqRSFetch_++;
	  processAnRSFetchMessage(udrGlob_, *this, udrHandle,
                                  rsHandle, request);

          // We need the request buffer in a state where the stream
          // knows it can be freed. We call SqlBuffer::bufferFull() to
          // accomplish this even though the method name is a bit
          // misleading.
          SqlBuffer *sqlBuf = request->getSqlBuffer();
          UDR_ASSERT(sqlBuf,
                     "UDR request buffer is corrupt or contains no data");
          sqlBuf->bufferFull();
	}
        break;

        case UDR_MSG_RS_CONTINUE:
	{
	  UdrRSContinueMsg *rsContinue =
            new (receiveMsgObj()) UdrRSContinueMsg(this);

	  udrGlob_->numReqRSContinue_++;
	  processAnRSContinueMessage(udrGlob_, *this, *rsContinue);
	}
	break;

        default:
        {
          UDR_ASSERT(FALSE,
                     "Unknown message type arrived on UDR data stream");
        } // default
        break;
        
      } // switch (t)
      
    } // while (getNextObjType(t))
  } // while (getNextReceiveMsg(t))

  // Make sure all reply buffers have been given to the connection. If
  // the only objects that arrived during this callback were continue
  // requests, then this action allows reply buffers associated with
  // those continue requests to propagate out of the stream and onto
  // the connection.
  //
  // If numOfInputBuffers() is > 0 then we do not need to do anything
  // at this point. This callback will be invoked again when the
  // incoming message is complete and ready to be processed.
  Lng32 numInputBuffers = numOfInputBuffers();
  if (somethingArrived && numInputBuffers == 0 &&
      spinfo_->getCurrentRequest() == NULL)
  {
    responseDone();

    if (udrGlob_->verbose_ &&
        udrGlob_->traceLevel_ >= TRACE_IPMS &&
        udrGlob_->showMain_)
    {
      ServerDebug("[UdrServ (%s)] All messages marked as replied", moduleName);
    }

    // Cleanup any unpacked message buffers containing only objects
    // that are no longer in use, as determined by the virtual method
    // IpcMessageObj::msgObjIsFree()
    releaseBuffers();  // Do final garbage collection
  }
  
} // UdrServerDataStream::actOnReceive()

#if (defined(NA_GUARDIAN_IPC))
void
UdrGuaControlConnection::actOnSystemMessage(
  short                  messageNum,
  IpcMessageBufferPtr    sysMsg,
  IpcMessageObjSize      sysMsgLen,
  short                  clientFileNumber,
  const GuaProcessHandle &clientPhandle,
  GuaConnectionToClient  *connection)
{
  const char * moduleName = "UdrGuaControlConnection::actOnSystemMessage";

  switch (messageNum)
  {
    case ZSYS_VAL_SMSG_OPEN:
      {
        if (udrGlob_->verbose_ &&
            udrGlob_->traceLevel_ >= TRACE_IPMS &&
            udrGlob_->showMain_)
        {
          ServerDebug(
           "[UdrServ (%s)] A new connection %p is opened to the Server.",
           moduleName,
           connection);
        }

        // Create a new message stream. Link the connection.
        // Then call receive on it to bring it into receiving mode.
        UdrServerControlStream *newControlStream =
          new (udrGlob_->getIpcHeap())
            UdrServerControlStream(udrGlob_->getIpcEnvironment(),
                                   udrGlob_,
                                   UDR_STREAM_SERVER_CONTROL,
                                   UdrServerControlStreamVersionNumber);
        newControlStream->addRecipient(connection);
        newControlStream->receive(FALSE);
      }
      break;

    default:
      // do nothing for all other kinds of system messages
      break;
  } // switch

  // The parent class already handles the job of closing all connections
  // who lost their client process by failed processes, failed CPUs and
  // failed systems or networks. Check here that we die if all our
  // requestors go away, but don't die if the first system message is
  // something other than an OPEN message.
  if (getNumRequestors() == 0 AND initialized_)
  {
    NAExit(0);
  }
  else if (NOT initialized_ AND getNumRequestors() > 0)
  {
    // the first requestor came in
    initialized_ = TRUE;
  }

} // UdrGuaControlConnection::actOnSystemMessage()
#endif

void
UdrServerControlStream::actOnReceive(IpcConnection *conn)
{
  const char *moduleName = "UdrServerControlStream::actOnReceive";

  NABoolean doTrace = (udrGlob_->verbose_ &&
                       udrGlob_->traceLevel_ >= TRACE_IPMS &&
                       udrGlob_->showMain_) ? TRUE : FALSE;
  if (doTrace)
  {
    ServerDebug("[UdrServ (%s)]", moduleName);
    ServerDebug("        A new message arrived on connection %p.", conn);
  }

  // Create a new stream and give the message to that stream
  UdrServerReplyStream *newReplyStream =
    new (udrGlob_->getUdrHeap()) UdrServerReplyStream(
      udrGlob_->getIpcEnvironment(),
      udrGlob_,
      UDR_STREAM_SERVER_REPLY,
      UdrServerReplyStreamVersionNumber);

  giveMessageTo(*newReplyStream, conn);

  // Add the stream to Udr Globals list of streams
  udrGlob_->addReplyStream(newReplyStream);


  if (doTrace)
  {
    ServerDebug("        The new message is scheduled for processing.");
    ServerDebug("        The connection %p will be placed on receive mode.",
                 conn);
    ServerDebug("[UdrServ (%s)] Done", moduleName);
  }

  // We have retrieved the message from stream, make it ready for
  // next message.
  receive(FALSE);

} // UdrServerControlStream::actOnReceive()


void UdrServerDataStream::activateCurrentMsgTransaction()
{
  udrGlob_->getControlConnection()->setUserTransReplyTag(getReplyTag());
  return;
}

void UdrServerReplyStream::routeMessage(UdrServerDataStream &other)
{
  // Store reply tag in data stream before routing message to it.
  other.setReplyTag(getReplyTag());
  giveReceiveMsgTo(other);
}

void UdrServerReplyStream::activateCurrentMsgTransaction()
{
  udrGlob_->getControlConnection()->setUserTransReplyTag(getReplyTag());
  return;
}
