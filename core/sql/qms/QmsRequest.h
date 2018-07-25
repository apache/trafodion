// **********************************************************************
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
// **********************************************************************

#ifndef _QMSREQUEST_H_
#define _QMSREQUEST_H_

#include <fstream>
#include "QRSharedPtr.h"
#include "QRIpc.h"
#include "QRMessage.h"
#include "ComRtUtils.h"

class IpcMessageStream;
class QRMessageObj;

/**
 * \file
 * Defines the request classes used by the Query Matching Server (QMS).
 * QMS is available in two modes: as a command-line executable, and as a
 * server process accessed via IPC.
 * \n\n
 * The purpose of the command-line version is to provide a simplified testing
 * tool for validating the operation of the Query Rewrite mechanism. When
 * invoked from the command line, the program should receive two arguments,
 * the name of an input file and an output file. The input file should contain
 * a number of message specifiers (one per line), each consisting of the header
 * part of the message and a reference to a file containing the associated XML
 * document (if the specific message type requires one). The output file will
 * contain the return codes indicating the outcome of processing each message in
 * the input file, as well as result descriptors for any messages that specify a
 * MATCH request.
 * \n\n
 * When invoked without the two command-line parameters, QMS executes as a
 * server process, waiting for requests to be delivered via IPC.
 */

using namespace QR;

// Classes defined in this file.
class QRRequest;
class   QRMessageRequest;
class   QRCommandLineRequest;
class QmsGuaReceiveControlConnection;
class QmsMessageStream;

/**
 * Abstract class that defines an interface used to access the information
 * comprising a QMS request. Member functions read the request type, prepare
 * the input XML stream for reading, and read the XML text one buffer at a time.
 * Subclasses of this class are defined for requests embedded in a file read
 * from a command-line invocation of QMS, and for requests passed through a
 * message interface.
 */
class QRRequest : public NAIntrusiveSharedPtrObject
{
  public:

    /**
     * Creates a request object.
     */
    QRRequest(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
        wrongRequestName_(heap)
      {}

    virtual ~QRRequest()
      {}

    /**
    * Parses an XML document, which should be one of the three Query Rewrite
    * descriptor types, and builds a hierarchy of classes representing the
    * elements of the document.
    *
    * @param request Object specifying the request (e.g., match), including the
    *                XML document to be parsed.
    * @param[out] descriptor Pointer to class instance representing the document
    *                        element of the parsed document.
    * @return Status indicator.
    */
    static QRRequestResult parseXMLDoc(QRRequest& request,
                                       XMLElementPtr& descriptor);
    /**
     * Handles a Publish request.
     * @param request The PUBLISH request.
     * @param msgStream If non-null, use this message stream to send a SUCCESS
     *                  response after reading the request but before processing it.
     * @return Return code.
     */
    static QRRequestResult handlePublishRequest(QRRequest& request,
                                                QRMessageStream* msgStream,
                                                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
    /**
     * Handles a Match request.
     * A Match request determines a set of candidate MVs that can be substituted
     * for JBB subsets in a query.
     * @param request The MATCH request.
     * @return Return code.
     */
    static QRRequestResult handleMatchRequest(QRRequest& request,
                                              XMLFormattedString& resultXML);

    /**
     * Performs initialization of QMS upon receipt of a request to do so.
     * @return Status of the initialization.
     */
    static QRRequestResult handleInitializeRequest();

    /**
     * Reads and returns the request type from the input stream.
     *
     * @param[out] requestType String that the request verb is read into.
     * @return \c true if the request type was read, \c false if there is no
     *         input available.
     */
    virtual NABoolean readRequestType(QRMessageTypeEnum& type) = 0;

    /**
     * Takes whatever action is necessary to ensure that the input stream for
     * the XML text is ready to read.
     *
     * @return Status of the operation.
     */
    virtual QRRequestResult openXmlStream() = 0;

    /**
     * Reads a buffer full of the XML document from the input stream.
     *
     * @param buffer Buffer into which to transfer XML text.
     * @param bufferSize Available size of the buffer.
     * @return Number of characters actually transferred.
     */
    virtual size_t readXml(char *buffer, size_t bufferSize) = 0;

     /**
      * After the request type has been read, return the next parameter.
      * @return 
      */
    virtual void getNextParameter(NAString& param) = 0;
          
    QRMessageTypeEnum resolveRequestName(char* name)
    {
      //QRRequestType result = QRMessage::resolveRequestName(name);
      QRMessageTypeEnum result = QRMessage::resolveRequestName(name);
      if (result == ERROR_REQUEST)
      {
        wrongRequestName_ = name;
      }

      return result;
    }

    const char *getWrongRequestName()
    {
      return wrongRequestName_.data();
    }

  protected:
    NAString  wrongRequestName_;

  private:
    // Copy construction/assignment not defined.
    QRRequest(const QRRequest&);
    QRRequest& operator=(const QRRequest&);
};  // class QRRequest

/**
 * Subclass representing a request passed through a messaging interface to QMS.
 */
class QRMessageRequest : public QRRequest
{
  public:

    /**
     * Creates a request object that uses the designated message stream.
     */
    QRMessageRequest(IpcMessageStream& msgStream,
                     ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRRequest(ADD_MEMCHECK_ARGS_PASS(heap)),
        msgStream_(msgStream),
        msgObj_(NULL),
        xmlTextPtr_(NULL),
        xmlCharsLeft_(0)
      {}

    virtual ~QRMessageRequest();

    /**
     * Processes a request originating from the message interface. Valid requests
     * include Initialize, Match, Publish, Cleanup, and Check Load. A message
     * object of the appropriate type for the response is created and returned
     * as the function value.
     *
     * @param msgStream The message stream carrying the request.
     * @return Message object to be returned as the response to this request.
     */
    static QRMessageObj* processRequestMessage(QRMessageStream* msgStream);

    /**
     * Returns the message stream that carries this request.
     * @return Reference to the request's message stream.
     */
    IpcMessageStream& getMessageStream() const
      {
        return msgStream_;
      }

    /**
     * Returns the request type of the message.
     *
     * @param[out] requestType String that the request verb is read into.
     * @return \c true if the request type was read, \c false if there is no
     *         input available.
     */
    virtual NABoolean readRequestType(QRMessageTypeEnum& type);

    /**
     * Receives a request message from client, and prepares the message content
     * to be read.
     *
     * @return Status of the operation.
     */
    virtual QRRequestResult openXmlStream();

    /**
     * Reads a buffer full of the XML document from the input stream.
     *
     * @param buffer Buffer into which to transfer XML text.
     * @param bufferSize Available size of the buffer.
     * @return Number of characters actually transferred.
     */
    virtual size_t readXml(char *buffer, size_t bufferSize);

    virtual void getNextParameter(NAString& param)
    {
      // Not implemented yet.
    }

    IpcMessageType getType() const
      {
        return msgStream_.getType();
      }
 
    /**
     * Obtains the XML text.
     * @return char pointer to the XML text.
     */
     char * getXmlText() { return xmlTextPtr_; };

private:
    // Copy construction/assignment not defined.
    QRMessageRequest(const QRMessageRequest&);
    QRMessageRequest& operator=(const QRMessageRequest&);

    IpcMessageStream& msgStream_;
    QRMessageObj* msgObj_;
    char* xmlTextPtr_;
    IpcMessageObjSize xmlCharsLeft_;
 
};  // class QRMessageRequest

/**
 * Subclass representing a request drawn from an input file for command-line
 * QMS. The same object is used for each request in the file. Each request
 * occupies a single line of the input file.
 */
class QRCommandLineRequest : public QRRequest
{
  public:
    /**
     * Creates an object that handles a request specified on a single line of
     * the input file used in an invocation of QMS from a command line.
     *
     * @param inFile File to read requests from.
     */
    QRCommandLineRequest(ifstream &inFile, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRRequest(ADD_MEMCHECK_ARGS_PASS(heap)),
        inFile_(inFile),
	isInlined_(FALSE),
	inlinedLineRead_(FALSE)
      {}

    virtual ~QRCommandLineRequest()
      {}

    /**
     * Reads a sequence of message specifications and responds to them by writing
     * results to the designated output file. There is one message specification
     * per line in the input file specified by the first command line argument.
     * Each line consists of a request type indicator and the input necessary to
     * process the request, typically the name of a file containing a MV or query
     * descriptor.
     *
     * @param argc Number of arguments on the command line.
     * @param argv The command-line arguments: input and output file.
     * @return Status indicator.
     */
    static Int32 processCommandLine(Int32 argc, char *argv[]);

    /**
     * Reads the request type from the input file. The request type is given by
     * the first word (whitespace-delimited) on the line.
     *
     * @param requestType Buffer to read the request type name into.
     * @return \c true if the value was read, \c false if end of file.
     */
    virtual NABoolean readRequestType(QRMessageTypeEnum& type);

    /**
     * Reads the name of the XML file containing the descriptor that accompanies
     * the request, and opens the file. For command-line QMS, the name of the
     * XML file follows the request type on the same line. Each line of the file
     * represents a separate request.
     *
     * @return Status indicator for the operation.
     */
    virtual QRRequestResult openXmlStream();

    /**
     * Reads up to \c bufferSize characters of the XML file into \c buffer.
     * The number of characters actually read is the return value, so the
     * function should be called until it returns 0. The expat parser can be
     * fed input a buffer at a time, so calls to #XML_Parse can alternate with
     * calls to this function.
     *
     * @param buffer Character array to read XML text into.
     * @param bufferSize Number of characters available in the array.
     * @return Number of characters actually read.
     */
     virtual size_t readXml(char *buffer, size_t bufferSize)
     {
       if (isInlined_)
         return readXmlInlined(buffer, bufferSize);
       else
         return readXmlFile(buffer, bufferSize);
     }

     /**
      * After the request type has been read, return the next parameter.
      * @return 
      */
     virtual void getNextParameter(NAString& param);
          
protected:
    size_t readXmlFile(char *buffer, size_t bufferSize)
      {
        if ( (!xmlFile_.rdbuf()->is_open()) ||
             (xmlFile_.eof()) )
          return 0;

        xmlFile_.read(buffer, bufferSize);
        return xmlFile_.gcount();
      }

    size_t readXmlInlined(char *buffer, size_t bufferSize)
      {
        if (!inFile_.rdbuf()->is_open() || inlinedLineRead_)
          return 0;

        inlinedLineRead_ = TRUE;
        memset(buffer, 0, bufferSize);
        inFile_.getline(buffer, bufferSize);
        return strlen(buffer);
      }

private:
    // Copy construction/assignment not defined.
    QRCommandLineRequest(const QRCommandLineRequest&);
    QRCommandLineRequest& operator=(const QRCommandLineRequest&);

    ifstream& inFile_;
    ifstream  xmlFile_;
    NABoolean isInlined_;
    NABoolean inlinedLineRead_;

}; // class QRCommandLineRequest

class QmsGuaReceiveControlConnection : public GuaReceiveControlConnection
{
  public:
    QmsGuaReceiveControlConnection(IpcEnvironment* env);

    virtual ~QmsGuaReceiveControlConnection()
      {}

    virtual void actOnSystemMessage(short messageNum,
                                    IpcMessageBufferPtr sysMsg,
                                    IpcMessageObjSize sysMsgLen,
                                    short clientFileNumber,
                                    const GuaProcessHandle& clientPhandle,
                                    GuaConnectionToClient* connection);

  private:
    NABoolean isPrivateQms_;
    char procName_[PROCESSNAME_STRING_LEN];
};  // QmsGuaReceiveControlConnection

class QmsMessageStream : public QRMessageStream
{
  public:
    /**
     * Creates a message stream used to convey messages to/from QMS.
     *
     * @param *env The IPC environment containing the stream.
     * @param thisEnd Name of the program unit defining the stream (used only
     *                for logging).
     * @param heap Heap used for dynamic allocation.
     * @param msgType Type of messages carried by the stream.
     */
    QmsMessageStream(IpcEnvironment *env,
                     const NAString& thisEnd,
                     NAMemory* heap = NULL,
                     IpcMessageType msgType = UNSPECIFIED_QR_MESSAGE)
      : QRMessageStream(env, thisEnd, heap, msgType)
      {}
    
    ~QmsMessageStream()
      {}

    /**
     * Callback function invoked after a message is sent through the stream.
     * @param connection The IpcConnection through which the message has been
     *                   sent.
     */
    //virtual void actOnSend(IpcConnection* connection);

    /**
     * Callback function invoked after a message is received through the stream.
     * @param connection The IpcConnection through which the message has been
     *                   received.
     */
    virtual void actOnReceive(IpcConnection* connection);

    virtual void actOnSendAllComplete()
    {
      clearAllObjects();
      receive(FALSE);
    }

  private:
      QmsMessageStream(const QmsMessageStream&);
      Int32 operator=(const QmsMessageStream&);
}; // QmsMessageStream

#endif  /* _QMSREQUEST_H_ */
