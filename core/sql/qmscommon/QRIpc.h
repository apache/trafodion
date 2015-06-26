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

#ifndef _QRIPC_H_
#define _QRIPC_H_

#include "Ipc.h"
#include "QRLogger.h"
#include "QRMessage.h"
#include "XMLUtil.h"
#include "NABoolean.h"
#include <stdlib.h>
#include <string.h>

/**
 * \file 
 * Contains the IPC specialization classes used for QMS messaging.
 */

using namespace QR;

// Classes defined in this file.
class QRMessageObj;
class   QRXmlMessageObj;
class   QRStatusMessageObj;
class QRMessageStream;

#define MSGBUFFERMAX  (128*1024)
#define QR_MSG_STREAM_VERSION 1
#define QR_XML_MSG_VERSION 1
#define QR_STATUS_MSG_VERSION 1

/**
 * Abstract base class for MVQR message object types.
 */
class QRMessageObj : public IpcMessageObj
{
  public:
    virtual ~QRMessageObj()
      {
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
          "---QRMessageObj dealloc: allocCount_ = %d", --allocCount_);
      }

    /**
     * Returns this object's request type.
     * @return Request type of this object.
     */
    QRMessageTypeEnum getRequestType() const
      {
        return requestType_;
      }

    static THREAD_P Int32 allocCount_;

    static const char* getRequestTypeName(QRMessageTypeEnum type) 
      {
        static char invalid[] = "Invalid message type";
        if (type < (Int32) IPC_MSG_QR_FIRST || type > (Int32) IPC_MSG_QR_LAST)
          return invalid;
        else
          return MessageTypeNames[type - IPC_MSG_QR_FIRST];
      }

    /**
     * Sets this object's request type.
     * @param requestType Request type for this object.
     */
    void setRequestType(QRMessageTypeEnum requestType)
      {
        requestType_ = requestType;
      }

  protected:
    /**
     * Creates a message object with the given request type and version. This
     * is only called by subclass constructors, since this class is abstract.
     *
     * @param objType Type of the message object.
     * @param version The version number of the object definition.
     */
    QRMessageObj(IpcMessageObjType objType, IpcMessageObjVersion version)
      : IpcMessageObj(objType, version)
      {
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
          "+++QRMessageObj alloc: allocCount_ = %d", ++allocCount_);
      }

  private:
    // Copy construction/assignment not defined.
    QRMessageObj(const QRMessageObj&);
    QRMessageObj& operator=(const QRMessageObj&);

    static const char* const MessageTypeNames[];
    QRMessageTypeEnum requestType_;
};  // class QRMessageObj

/**
 * Message object type that has no payload, just a message type. The message
 * type conveys a command to the recipient, like CLEANUP or INITIALIZE.
 */
class QRSimpleMessageObj : public QRMessageObj
{
  public:
    /**
     * Creates a message object with no content other than a request type and
     * version. All the parameters are optional, which allows instances of this
     * class to be created by extraction from an incoming message stream.
     *
     * @param objType The object's request type.
     * @param version The object's version.
     */
    QRSimpleMessageObj(IpcMessageObjType objType = UNSPECIFIED_QR_MESSAGE,
                       IpcMessageObjVersion version = QR_XML_MSG_VERSION)
      : QRMessageObj(objType, version)
      {}

    virtual ~QRSimpleMessageObj()
      {}

    /**
     * Returns the size in bytes of this object's serialialized form for message
     * transmission. Since the message has no body, there is no packing or
     * unpacking per se.
     *
     * @return Size of the serialized form of this object.
     */
    virtual IpcMessageObjSize packedLength()
      {
        return sizeof(*this);
      }

  private:
    // Copy construction/assignment not defined.
    QRSimpleMessageObj(const QRSimpleMessageObj&);
    QRSimpleMessageObj& operator=(const QRSimpleMessageObj&);
};  // class QRSimpleMessageObj

/**
 * Message object type that carries a descriptor in XML form.
 */
class QRXmlMessageObj : public QRMessageObj
{
  public:
    /**
     * Creates a message object that carries an XML payload. All the parameters
     * are optional, which allows instances of this class to be created by
     * extraction from an incoming message stream.
     *
     * @param xml The contained XML.
     * @param objType The object's request type.
     * @param version The object's version.
     * @return 
     */
    QRXmlMessageObj(const NAString* xml = NULL,
                    IpcMessageObjType objType = UNSPECIFIED_QR_MESSAGE,
                    IpcMessageObjVersion version = QR_XML_MSG_VERSION)
      : QRMessageObj(objType, version)
      {
        setData(xml);
      }

    virtual ~QRXmlMessageObj()
      {}

    /**
     * Fills the object's character buffer with the text from the serialized
     * XML document.
     * @param xml XML text object used to set this object's payload.
     */
    void setData(const NAString* xml)
      {
        if (xml)
          strcpy(xmlText_, xml->data());
        else
          xmlText_[0] = '\0';
      }

    /**
     * Returns the size in bytes of this object's serialialized form for message
     * transmission. Since the message consists of a single char array, there
     * is no packing or unpacking per se.
     *
     * @return Size of the serialized form of this object.
     */
    virtual IpcMessageObjSize packedLength()
      {
        // Message consists of a single char array, so there is no packing or
        // unpacking per se.
        return (sizeof(*this) - sizeof(xmlText_) + strlen(xmlText_) + 1);
      }

    /**
     * Returns the size of the object's XML payload.
     * @return Size of the XML text.
     */
    IpcMessageObjSize getLength()
      {
        return strlen(xmlText_);
      }

    /**
     * Returns a pointer to the XML text contained by this object.
     * @return XML text.
     */
    char* getData()
      {
        return xmlText_;
      }

    /**
     * Writes the content of this message object to the log file.
     * @param logger The logger to use.
     */
    void log(QRLogger& logger)
      {
        logger.log(CAT_SQL_COMP_QR_IPC, LL_DEBUG, xmlText_);
      }

  private:
    // Copy construction/assignment not defined.
    QRXmlMessageObj(const QRXmlMessageObj&);
    QRXmlMessageObj& operator=(const QRXmlMessageObj&);

    char xmlText_[MSGBUFFERMAX];
};  // class QRXmlMessageObj

/**
 * Message object type that consists of a status code with no additional
 * content. This includes a success code for requests like \c PUBLISH that
 * do not return an XML document.
 */
class QRStatusMessageObj : public QRMessageObj
{
  public:
    /**
     * Creates a message object that carries an status value (return code) as
     * its payload. All the parameters are optional, which allows instances of
     * this class to be created by extraction from an incoming message stream.
     * 
     * @param result The status value to include as the payload of this
     *               message object.
     * @param version The object's version.
     */
    QRStatusMessageObj(QRRequestResult result = Success,
                       IpcMessageObjVersion version = QR_STATUS_MSG_VERSION)
      : QRMessageObj(STATUS_RESPONSE, version),
        statusCode_(result)
      {}

    virtual ~QRStatusMessageObj()
      {}

    /**
     * Returns the size in bytes of this object's serialialized form for message
     * transmission. Since the message consists of a single int for the status,
     * there is no packing or unpacking per se.
     *
     * @return Size of the serialized form of this object.
     */
    virtual IpcMessageObjSize packedLength()
      {
        return sizeof(*this);
      }

    /**
     * Returns the status code that comprises the payload of this message object.
     * @return The status code conveyed by this message object.
     */
    QRRequestResult getStatusCode() const
      {
        return statusCode_;
      }

    /**
     * Sets the payload for this message object (a status code).
     * @param statusCode The status code to associate with this message object.
     */
    void setStatusCode(QRRequestResult statusCode)
      {
        statusCode_ = statusCode;
      }

  private:
    // Copy construction/assignment not defined.
    QRStatusMessageObj(const QRStatusMessageObj&);
    QRStatusMessageObj& operator=(const QRStatusMessageObj&);

    QRRequestResult statusCode_;
};  // class QRStatusMessageObj

/**
 * Message stream used to carry all MVQR message objects. The callbacks for
 * this message stream are used only for logging, since the processing of
 * sent and received messages is handled at the point of call.
 */
class QRMessageStream : public IpcMessageStream
{
  public:
    /**
     * Creates a message stream used to convey Query Rewrite message objects.
     *
     * @param *env The IPC environment containing the stream.
     * @param logger Logger to use.
     * @param thisEnd Name of the program unit defining the stream (used only
     *                for logging.
     * @param heap Heap used for dynamic allocation.
     * @param msgType Type of messages carried by the stream.
     */
    QRMessageStream(IpcEnvironment *env,
                    const NAString& thisEnd,
                    NAMemory* heap = NULL,
                    IpcMessageType msgType = UNSPECIFIED_QR_MESSAGE)
      : IpcMessageStream(env, msgType, QR_MSG_STREAM_VERSION, 0, TRUE),
        waited_(TRUE),
        thisEnd_(thisEnd, heap),
        otherEnd_(heap)
      {
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
          "+++QRMessageStream alloc: allocCount_ = %d", ++allocCount_);
      }
    
    ~QRMessageStream()
      {
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
          "---QRMessageStream dealloc: allocCount_ = %d", --allocCount_);
      }

    static THREAD_P Int32 allocCount_;

    /**
     * Sets the name designating the other endpoint in the communication, for
     * use with logging.
     */
    void setOtherEnd();

    /**
     * Callback function to log a message when a complete message stream has
     * been sent.
     *
     * @param connection The IpcConnection through which the message has been
     *                   sent.
     */
    virtual void actOnSend(IpcConnection* connection);

    /**
     * Callback function to log a message when a complete message stream has
     * been received.
     *
     * @param connection The IpcConnection through which the message has been
     *                   received.
     */
    virtual void actOnReceive(IpcConnection* connection);

    /**
     * Sends the passed response to the client.
     * @param response The response object to send.
     */
    void respond(QRMessageObj* response);

  protected:
    NABoolean waited_;
    NAString thisEnd_;
    NAString otherEnd_;
};  // class QRMessageStream

#endif  /* _QRIPC_H_ */
