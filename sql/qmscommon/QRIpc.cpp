// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
// **********************************************************************

#include "Platform.h"
#include "QRIpc.h"

#include "ComRtUtils.h"

// Used to track allocations of message streams and objects, for logging.
THREAD_P Int32 QRMessageStream::allocCount_ = 0;
THREAD_P Int32 QRMessageObj::allocCount_    = 0;

// NOTE: This definition must remain in sync with the definition of
//       QR::QRMessageTypeEnum.
const char* const QRMessageObj::MessageTypeNames[] =
  {
    "UNSPECIFIED_QR_MESSAGE"

    // Requests
    ,"INITIALIZE_REQUEST"
    ,"ALLOCATE_REQUEST"
    ,"PUBLISH_REQUEST"
    ,"MATCH_REQUEST"
    ,"CHECK_REQUEST"
    ,"CLEANUP_REQUEST"
    ,"DEFAULTS_REQUEST"

    // These are used only for command-line QMS
    ,"COMMENT_REQUEST"
    ,"ERROR_REQUEST"

    // Responses.
    ,"STATUS_RESPONSE"
    ,"ALLOCATE_RESPONSE"
    ,"MATCH_RESPONSE"
    ,"CHECK_RESPONSE"
  };

void QRMessageStream::setOtherEnd()
{
  static const Int32 MAX_LEN = 300;
  char other[MAX_LEN];
  const IpcSetOfConnections& connections = getRecipients();
  CollIndex elemInx = NULL_COLL_INDEX;
  // for (CollIndex elemInx = 0; setToNext(elemInx); elemInx++)
  if (connections.setToNext(elemInx))
    {
      connections.element(elemInx)->getOtherEnd().toAscii(other, MAX_LEN);
      otherEnd_ = other;
    }
  else
    otherEnd_ = "<no connection>";
}

void QRMessageStream::actOnSend(IpcConnection *connection)
{
      MessageStateEnum state = IpcMessageStream::getState();
      if (otherEnd_.length() == 0)
        setOtherEnd();

      const char *msgType =
              QRMessageObj::getRequestTypeName((QRMessageTypeEnum)getType());

      char connText[PROCESSNAME_STRING_LEN];
      connection->getOtherEnd().toAscii(connText, PROCESSNAME_STRING_LEN);
      if (state == ERROR_STATE)
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
        "Error sending message of type %s from %s to %s",
        msgType, thisEnd_.data(), connText);
    else
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
        "Message of type %s was sent from %s to %s",
        msgType, thisEnd_.data(), connText); //otherEnd_.data())
}

void QRMessageStream::actOnReceive(IpcConnection *connection)
{
      MessageStateEnum state = IpcMessageStream::getState();
      if (otherEnd_.length() == 0)
        setOtherEnd();

      const char *msgType = QRMessageObj::getRequestTypeName((QRMessageTypeEnum)getType());
      char connText[PROCESSNAME_STRING_LEN];
      connection->getOtherEnd().toAscii(connText, PROCESSNAME_STRING_LEN);

      if (state == ERROR_STATE)
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
        "Error in %s receiving message of type %s from %s",
        thisEnd_.data(), msgType, connText);
//                    thisEnd_.data(), msgType, otherEnd_.data())
      else
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
        "Message of type %s was received from %s by %s ",
        msgType, connText, thisEnd_.data());
//                    msgType, otherEnd_.data(), thisEnd_.data())
    }

void QRMessageStream::respond(QRMessageObj* responseObj)
{
  QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG, "Reached QRMessageStream::respond()");

  if (!responseObj)
    responseObj = new QRStatusMessageObj(QR::InternalError);

  clearAllObjects();
  setType(responseObj->getType()); // so correct msg type logged
  *this << *responseObj;
  send(FALSE);
  responseObj->decrRefCount();
}
