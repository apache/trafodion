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

#include "Platform.h" // Must precede zsysc.h or weird errors occur

#include "QmsRequest.h"
#include "QmsQms.h"
#include "QmsInitializer.h"
#include "QRIpc.h"

using namespace QR;
#define XML_BUFF_SIZE 32768

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/int/opts.h"
#include <sys/time.h>

#include "nsk/nskprocess.h"
extern "C" {
#include "cextdecs/cextdecs.h"
#include "zsysc.h"
}

QRMessageRequest::~QRMessageRequest()
{
  delete msgObj_;
}

// Receive should have been done on message stream before calling this.
// An object is extracted from the message stream, from which the XML descriptor
// text and length is set up.
QRRequestResult QRMessageRequest::openXmlStream()
{
  IpcMessageStream::MessageStateEnum state = msgStream_.getState();
  if (state == IpcMessageStream::RECEIVED)
    {
      if (msgStream_.moreObjects())
        {
          QRMessageTypeEnum reqType;
          readRequestType(reqType);
          msgObj_ = new QRXmlMessageObj(NULL, reqType);
          QRXmlMessageObj* xmlMsgObj = static_cast<QRXmlMessageObj*>(msgObj_);
          msgStream_ >> *xmlMsgObj;
          xmlTextPtr_   = xmlMsgObj->getData();
          xmlCharsLeft_ = xmlMsgObj->getLength();
          QRLogger::log(CAT_QMS_XML, LL_DEBUG, "XML received is:");
          QRLogger::log1(CAT_QMS_XML, LL_DEBUG, xmlTextPtr_);
          return Success;
        }
      else
        {
          QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
            "In QRMessageRequest::openXmlStream(): no objects in stream");
          return ProtocolError;
        }
    }
  else
    {
      QRLogger::log(CAT_QMS_MAIN, LL_INFO,
        "In QRMessageRequest::openXmlStream(): message stream state is %d",
                  state);
      return ProtocolError;
    }
} // End of openXmlStream

NABoolean QRMessageRequest::readRequestType(QRMessageTypeEnum& type)
{
  if (msgStream_.moreObjects())
    {
      type = (QRMessageTypeEnum)msgStream_.getNextObjType();
      return TRUE;
    }
  else
    return FALSE;
}  // End of readRequestType

// Fills the buffer with as much of the remaining XML text as will fit.
size_t QRMessageRequest::readXml(char* buffer, size_t bufferSize)
{
  if (!xmlTextPtr_ || !xmlCharsLeft_)
    return 0;

  Int32 charsToCopy = (xmlCharsLeft_ <= bufferSize
                        ? xmlCharsLeft_
                        : bufferSize);
  strncpy(buffer, xmlTextPtr_, charsToCopy);
  xmlTextPtr_ += charsToCopy;
  xmlCharsLeft_ -= charsToCopy;
  return charsToCopy;
} // End of readXML

// Open the input file for a command-line execution of qms.
QRRequestResult QRCommandLineRequest::openXmlStream()
{
  if (isInlined_)
    return Success;

  char xmlFileName[100];
  inFile_ >> xmlFileName;
  QRLogger::log(CAT_QMS_MAIN, LL_DEBUG,
    "XML file containing descriptor is %s", xmlFileName);
  if (xmlFile_.rdbuf()->is_open()) // may be open from a previous line in input file
    {
      xmlFile_.clear();
      xmlFile_.close();
    }

  xmlFile_.open(xmlFileName, ios::in);
  if (xmlFile_.fail())
    QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
      "Failed to open %s", xmlFileName);

  if (xmlFile_.rdbuf()->is_open())
    return Success;
  else
    {
      QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
        "Could not open XML file %s for request.", xmlFileName);
      return BadFile;
    }
} // End of openXmlStream

NABoolean QRCommandLineRequest::readRequestType(QRMessageTypeEnum& type)
{
  char name[100];
  inFile_ >> name;  // This is a problem - can overflow buffer!!! TBD!!!

  if (inFile_.fail() == TRUE)
    return FALSE;

  if(!inFile_.good())
    return FALSE;

  type = resolveRequestName(name);

  isInlined_ = FALSE;
 
  return TRUE;
} // End of readRequestType

void QRCommandLineRequest::getNextParameter(NAString& param)
{
  char buffer[128] = "";
  inFile_ >> buffer;
  param = buffer;
}

QmsGuaReceiveControlConnection::QmsGuaReceiveControlConnection(IpcEnvironment* env)
  : GuaReceiveControlConnection(env)
{
  memset(procName_, 0, sizeof(procName_));
  short retval = msg_mon_get_my_process_name(procName_, sizeof(procName_) - 1);

  // If the process is "unnamed" (i.e., the name is not one we assign to a
  // public qms), mark it as private, so we'll know to terminate if we get a
  // close message.
#ifndef NA_WINNT
  // Look for a redefinition of the process prefix.
  char prefixValue[36];
  short prefixValueLen = 0;
  short retcode = 0;

#elif defined (NA_LINUX)

  char *defaultsValue = getenv("_MX_QMS_PROCESS_PREFIX");
  
  if (defaultsValue != NULL)
  {
     memset (prefixValue, 0, sizeof(prefixValue)-1);
     strncpy(prefixValue,defaultsValue, sizeof(prefixValue)-1);
  }
  else
    retcode = -1;

#endif
  if (retcode == 0)
    {
      // Define successfully found and read.
      prefixValue[prefixValueLen] = '\0';
      const char* dollar = strchr(prefixValue, '$');
      if (!dollar)
        dollar = QMS_PROCESS_PREFIX;
      char* period =  (char *) strchr(dollar, '.');
      if (period)
        *period = '\0';
      isPrivateQms_ = !strstr(procName_, dollar);
    }
  else
    // No define, use standard prefix.
    isPrivateQms_ = !strstr(procName_, QMS_PROCESS_PREFIX);
#else
    isPrivateQms_ = !strstr(procName_, QMS_PROCESS_PREFIX);
#endif
};

void QmsGuaReceiveControlConnection::actOnSystemMessage
                                       (short messageNum,
                                        IpcMessageBufferPtr sysMsg,
                                        IpcMessageObjSize sysMsgLen,
                                        short clientFileNumber,
                                        const GuaProcessHandle& clientPhandle,
                                        GuaConnectionToClient* connection)
{
  if (messageNum == ZSYS_VAL_SMSG_OPEN)
    {
      NAString qmsName("qms");
      QmsMessageStream* msgStream =
              //new QmsMessageStream(const_cast<IpcEnvironment*>(getEnv()));
              new QmsMessageStream(getEnv(), "qms");
      msgStream->addRecipient(connection);
      //connection->receive(msgStream);
      msgStream->receive(FALSE);
    }
  else if (messageNum == ZSYS_VAL_SMSG_CLOSE ||
           messageNum == ZSYS_VAL_SMSG_PROCDEATH)
    {
#ifdef NA_WINNT
      QRLogger::log(CAT_QR_IPC, LL_INFO,
        "Client gone, exiting QMS with %d requestors remaining", getNumRequestors());
      NAExit(0);
#else
      if (isPrivateQms_)
        {
          QRLogger::log(CAT_QR_IPC, LL_INFO,
            "Client gone, private QMS %s exiting", procName_);
          NAExit(0);
        }
      else if (clientPhandle == getConnection()->getOtherEnd().getPhandle())
        // QMM is gone, so we exit.
        NAExit(0);
      else
        connection->setFatalError(NULL);  //@ZX
#endif
    }
}

void QmsMessageStream::actOnReceive(IpcConnection* connection)
{
  QRLogger::log(CAT_QR_IPC, LL_DEBUG,
    "Reached QmsMessageStream::actOnReceive()");

  // Do logging of received message.
  QRMessageStream::actOnReceive(connection);

  QRMessageObj* responseObj = NULL;
  IpcMessageObjType msgType = getNextObjType();
  try
    {
      responseObj = QRMessageRequest::processRequestMessage(this);
    }
  catch(...)
    {}

  // Publish messages are responded to (with a SUCCESS response) after reading
  // the request message, but before processing the request. So skip the
  // response for a publish, it has already been done.
  if (msgType != PUBLISH_REQUEST)
    respond(responseObj);
}

static void error(const char* text)
{
  cerr << text << endl;
}

static void usage(char *progName)
{
  cerr << "Usage: " << progName << " infile outfile" << endl;
  QRLogger::log(CAT_QR_IPC, LL_ERROR,
    "Usage: %s infile outfile", progName);
}

// These static member functions are called from QmsMain.cpp.

QRRequestResult QRRequest::parseXMLDoc(QRRequest& request,
                                       XMLElementPtr& descriptor)
{
  QRRequestResult openResult = request.openXmlStream();
  if (openResult != Success)
    return openResult;

  NAHeap* xmlParseHeap = (NAHeap*)(QmsInitializer::getInstance()->getHeap());
  try
    {
      char xmlBuffer[XML_BUFF_SIZE];
      memset(xmlBuffer, 0, XML_BUFF_SIZE);

      QRElementMapper em;
      XMLDocument doc = XMLDocument(xmlParseHeap, em);

      Int32 charsRead = 1; // make nonzero so we will enter loop
      while (charsRead)
	{
	  charsRead = request.readXml(xmlBuffer, XML_BUFF_SIZE);
	  if (charsRead)
	    doc.parse(xmlBuffer, charsRead, FALSE);
	  else
	    descriptor = doc.parse(xmlBuffer, charsRead, TRUE);
	}

    if (!descriptor)
      {
        QRLogger::log(CAT_QMS_MAIN, LL_WARN,
          "XMLDocument.parse() returned NULL.");
        return XMLParseError;
      }
    else
      {
        QRLogger::log(CAT_QMS_MAIN, LL_INFO,
          "Parsed XML document successfully.");
      }
   }
  catch (XMLException& ex)
    {
      QRLogger::log(CAT_QMS_MAIN, LL_MVQR_FAIL,
        "An XMLException occurred: %s", ex.getMessage());
      return XMLParseError;
    }
  catch (QRDescriptorException& ex)
    {
      QRLogger::log(CAT_QMS_MAIN, LL_MVQR_FAIL,
        "A QRDescriptorException occurred: %s", ex.getMessage());
      return XMLParseError;
    }
  catch (...)
    {
      QRLogger::log(CAT_QMS_MAIN, LL_MVQR_FAIL,
        "An Unknown exception occurred");
      return InternalError;
    }

  return Success;
}  // End of parseXmlDoc


QRRequestResult QRRequest::handlePublishRequest(QRRequest& request,
                                                QRMessageStream* msgStream,
                                                ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
{
  Qms& qms = *Qms::getInstance();
  XMLElementPtr publishDescriptor = NULL;
  QRRequestResult result = parseXMLDoc(request, publishDescriptor);

  // Publish response (always send SUCCESS) is sent before processing request.
  // msgStream will be null if this is called for a command-line request.
  if (msgStream)
    msgStream->respond(new QRStatusMessageObj(QR::Success));

  if (result != Success)
    return result;

  // If parsed successfully, make sure it was a Publish descriptor instead of
  // something else.
  if (publishDescriptor->getElementType() !=  ET_PublishDescriptor)
  {
    QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
      "XML document parsed ok, but had wrong document element -- %s",
                publishDescriptor->getElementName());
    deletePtr(publishDescriptor);
    return WrongDescriptor;
  }

  QRPublishDescriptorPtr pubDesc = static_cast<QRPublishDescriptorPtr>(publishDescriptor);
  const NAString& mvName = pubDesc->getName();
  NAString timestamp(heap);

  // Check if the data we already have is newer than the Publish message.
  Int64 newTimestamp = atoInt64(pubDesc->getRedefTimestamp());
  const Int64* existingRedefTimestamp = qms.getMVTimestamp(mvName);
  if (existingRedefTimestamp != NULL &&  // The MV already exists
      newTimestamp != 0              &&  // This is not a testing result from CropDescriptor
      *existingRedefTimestamp >= newTimestamp)
  {
    // The Publish message is stale. Ignore it.
    deletePtr(publishDescriptor);
    return result;
  }

  // Insert a new MV descriptor before doing any updates to it.
  QRMVDescriptorPtr mvDesc = pubDesc->getMV();
  if (mvDesc != NULL)
  {
    if (qms.contains(mvName))
    {
      // This is a republish - drop the old definition of the MV
      // before inserting the new one.
      qms.drop(mvName);
    }
    qms.insert(mvDesc);

    // The Publish descriptor will soon be deleted. Make sure the
    // MV descriptor will not be deleted with it.
    pubDesc->setMV(NULL);
  }

  qms.touch(mvName, timestamp);

  const NAPtrList<QRUpdatePtr>& upds = pubDesc->getUpdateList();
  CollIndex maxEntries = upds.entries();
  for (CollIndex i = 0; i < maxEntries; i++)
  {
    QRUpdatePtr update = upds[i];

    switch(update->getUpdateType())
    {
      case QRUpdate::ALTER:
	qms.alter(mvName, update->getIgnoreChanges());
	break;

      case QRUpdate::REFRESH:
	qms.refresh(mvName, update->getRefreshTimestamp(), FALSE);
	break;
	
      case QRUpdate::RECOMPUTE:
	qms.refresh(mvName, update->getRefreshTimestamp(), TRUE);
	break;

      case QRUpdate::DROP:
	qms.drop(mvName);
	break;

      case QRUpdate::RENAME:
	qms.rename(mvName, update->getNewName());
	break;

      case QRUpdate::DEFAULT:
	// TBD !!!
	break;
    }
  }

  deletePtr(pubDesc);

  return result;
} // End of handlePublishRequest

QRRequestResult QRRequest::handleMatchRequest(QRRequest& request,
                                              XMLFormattedString& resultXML)
{
  Qms& qms = *Qms::getInstance();

  // Get the XML file containing the query descriptor and parse it.
  XMLElementPtr descriptor = NULL;
  QRRequestResult result = parseXMLDoc(request, descriptor);
  
  if (result == Success)
  {
    // If parsed successfully, make sure it was a query descriptor instead of
    // something else.
    if (descriptor->getElementType() != ET_QueryDescriptor)
    {
      result = WrongDescriptor;
      QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
        "XML document parsed ok, but had wrong document element -- %s",
                  descriptor->getElementName());
    }

    // If it is Success, perform "match" and add the resulting Result
    // descriptor to the output file.
    QRQueryDescriptorPtr queryDescriptor = 
              static_cast<QRQueryDescriptorPtr>(descriptor);
    QRResultDescriptorPtr resultDesc = 
                              qms.match(queryDescriptor, 
                                        QmsInitializer::getInstance()->getHeap());

    // Serialize the result descriptor document into a string and write it to the
    // output file.
    resultDesc->toXML(resultXML);
    deletePtr(resultDesc);
  }
  // The descriptor memory is released by Qms.
  //deletePtr(descriptor);

  return result;
}  // handleMatchRequest()


QRRequestResult QRRequest::handleInitializeRequest()
{
  Qms& qms = *Qms::getInstance();
  QmsInitializer& qmsInitializer = *QmsInitializer::getInstance(&qms);

  try
  {
    Lng32 result = qmsInitializer.performInitialization();
    QRLogger::log(CAT_QMS_MAIN, LL_INFO,
      "QMS initialization completed. Result code = %d. Processed %d MVs in %d catalogs.", 
      result, qmsInitializer.getNumberOfMVs(), qmsInitializer.getNumberOfCatalogs());
  }
  catch(QRException& ex)
  {
    QRLogger::log(CAT_QMS_MAIN, LL_ERROR, "QMS initialization failure: %s",
                  ex.getMessage());
    return InternalError;
  }
  catch(...)
  {
    QRLogger::log(CAT_QMS_MAIN, LL_ERROR, "Unknown internal error occurred.");
    return InternalError;
  }

  return Success;
}

QRMessageObj* QRMessageRequest::processRequestMessage(QRMessageStream* msgStream)
{
  QmsInitializer& qmsInitializer = *QmsInitializer::getInstance();
  CollHeap* qmsHeap = qmsInitializer.getHeap();
  QRRequestResult result;
  XMLFormattedString resultXML(qmsHeap);
  QRMessageObj* responseMsgPtr = NULL;
  QRMessageRequest request(*msgStream);
  
  // We use this to remove a message object from the stream when the body of
  // the message is not used, e.g. Initialize and Cleanup messages. The message
  // must be extracted from the stream or the while loop below won't terminate.
  // The message object type must be set to the type of the message being
  // extracted from the stream, or the IPC code will fail an assertion.
  QRSimpleMessageObj dummyMsgObj;

  if (!msgStream->moreObjects())
    {
      QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
        "QMS received an empty message stream.");
      return new QRStatusMessageObj(QR::ProtocolError);
    }

  // Use this in loop condition to prevent infinite loop in case of corrupt
  // message stream.
  NABoolean noBadRequest = TRUE;

  while (noBadRequest && msgStream->moreObjects())
    {
      switch (msgStream->getNextObjType())
        {
          case MATCH_REQUEST:
            result = handleMatchRequest(request, resultXML);

            // collect and log qms stats
            if (qmsInitializer.doCollectQMSStats())
            {
              qmsInitializer.logQMSStats();
            }

            if (result == QR::Success)
              {
                QRLogger::log(CAT_QMS_XML, LL_DEBUG, "Result descriptor for match is:");
                QRLogger::log1(CAT_QMS_XML, LL_DEBUG, resultXML.data());
                responseMsgPtr = new QRXmlMessageObj(&resultXML,
                                                    MATCH_RESPONSE);
              }
            else
              {
                QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
                  "handleMatchRequest() returned error status: %d", result);
                // Empty msg, denoting failure. Can't send back a QRStatusMessage,
                // because IPC code asserts that message is of type expected.
                responseMsgPtr = new QRXmlMessageObj(NULL, MATCH_RESPONSE);
              }
            break;

          case PUBLISH_REQUEST:
            result = handlePublishRequest(request, msgStream,
                                          ADD_MEMCHECK_ARGS(qmsHeap));
            // collect and log qms stats
            if (qmsInitializer.doCollectQMSStats())
            {
              qmsInitializer.logQMSStats();
            }

            if (result == QR::Success)
              QRLogger::log(CAT_QMS_MAIN, LL_INFO,
                "PUBLISH was successful.");

            else
              QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
                "PUBLISH failed, status is %d", result);
            responseMsgPtr = new QRStatusMessageObj(result);
            break;

          case INITIALIZE_REQUEST:
            result = handleInitializeRequest();
            dummyMsgObj.setType(msgStream->getNextObjType());
            msgStream->extractNextObj(dummyMsgObj, FALSE);
            responseMsgPtr = new QRStatusMessageObj(result);
            break;

          case CLEANUP_REQUEST:
            Qms::deleteInstance();
            QRLogger::log(CAT_QMS_MAIN, LL_DEBUG,
                          "Qms instance deleted per CLEANUP request");
            dummyMsgObj.setType(msgStream->getNextObjType());
            msgStream->extractNextObj(dummyMsgObj, FALSE);
            responseMsgPtr = new QRStatusMessageObj(QR::Success);
            break;

          case CHECK_REQUEST:
            QRLogger::log(CAT_QMS_MAIN, LL_WARN,
              "'Check Load' request not yet handled");
            dummyMsgObj.setType(msgStream->getNextObjType());
            msgStream->extractNextObj(dummyMsgObj, FALSE);
            responseMsgPtr = new QRStatusMessageObj(QR::InvalidRequest);
            break;

          case DEFAULTS_REQUEST:
            QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
              "'Defaults' request not valid for QMS");
            dummyMsgObj.setType(msgStream->getNextObjType());
            msgStream->extractNextObj(dummyMsgObj, FALSE);
            responseMsgPtr = new QRStatusMessageObj(QR::InvalidRequest);
            break;

          default:
            QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
              "Unhandled message: %d", msgStream->getNextObjType());
            noBadRequest = FALSE;
            responseMsgPtr = new QRStatusMessageObj(QR::InvalidRequest);
            break;
        } // switch
    } // while

  return responseMsgPtr;
} // End of processRequestMessage

Int32 QRCommandLineRequest::processCommandLine(Int32 argc, char *argv[])
{
  QmsInitializer& qmsInitializer = *QmsInitializer::getInstance();
  NAMemory* heap = qmsInitializer.getHeap();

  if (stricmp(argv[1], "redescribe")==0)
  {
    if (argc != 3 && argc != 4)
    {
      cerr << "Usage: " << argv[0] << " <MV-name> [PUBLISH]" << endl;
      cerr << "       " << argv[0] << " ALL       [PUBLISH]" << endl;
      return -1;
    }

    NAString* mvName = NULL;
    if (stricmp(argv[2], "all")!=0)
      mvName = new NAString(argv[2]);

    NABoolean rePublish = FALSE;
    if (argc == 4 && stricmp(argv[3], "publish")==0)
      rePublish = TRUE;
                     
    qmsInitializer.reDescriber(mvName, rePublish);

    return 0;
  }

  if (argc != 3)
    {
      usage(argv[0]);
      return -1;
    }

  // create the input stream
  ifstream inFile(argv[1]);
  // create the output stream
  ofstream outFile(argv[2]);

  if (!inFile.rdbuf()->is_open())
    {
      QRLogger::log(CAT_QMS_MAIN, LL_ERROR, "Can't open input file. %s", argv[1]);
      error("Can't open input file.");
      usage(argv[0]);
      return -1;
    }

  if (!outFile.rdbuf()->is_open())
    {
      QRLogger::log(CAT_QMS_MAIN, LL_ERROR, "Can't open output file. %s", argv[2]);
      error("Can't open output file.");
      usage(argv[0]);
      return -1;
    }

  QRCommandLineRequest request(inFile);

  QRMessageTypeEnum requestType;
  Int32 requestCount = 0;
  QRRequestResult result = Success;

  // Read the first request and enter the loop to process it. Keep looping while
  // there is more input.
  NABoolean moreInput = request.readRequestType(requestType);
  while (moreInput)
    {
      const char* reqName = QRMessage::getRequestName(requestType);
      QRLogger::log(CAT_QMS_MAIN, LL_INFO,
                                "\n==>>Request received: %s", reqName);
      requestCount++;

      switch(requestType)
      {
      case PUBLISH_REQUEST:
        result = handlePublishRequest(request, NULL,
                                      ADD_MEMCHECK_ARGS(heap));
	  break;

      case MATCH_REQUEST:
	{
          XMLFormattedString resultXML;
	  result = handleMatchRequest(request, resultXML);

	  if (result == Success)
	  {
  	    outFile.write(resultXML.data(), resultXML.length());
  	    outFile.flush();
	  }
        }
	break;

      case CHECK_REQUEST:
	break;

      case INITIALIZE_REQUEST:
        result = handleInitializeRequest();
        break;

      case CLEANUP_REQUEST:
        Qms::deleteInstance();
        QRLogger::log(CAT_QMS_MAIN, LL_DEBUG,
            "Qms instance deleted per CLEANUP request");
        break;

      case WORKLOAD_REQUEST:
        {
          Qms& qms = *Qms::getInstance();
          NAString param(heap);
	  Int32 minQueriesPerMV = 0;
	  request.getNextParameter(param);
	  if (param != "")
	  {
	    minQueriesPerMV = atoi(param.data());
	  }
          qms.workloadAnalysis(outFile, minQueriesPerMV, heap);
          outFile.flush();
        }
        break;

      case COMMENT_REQUEST:
          inFile.ignore(INT_MAX, '\n');  // ignore comment in command file
          break;

      default:
          // Invalid or unknown request.
          if (requestType == ALLOCATE_REQUEST)
            QRLogger::log(CAT_QMS_MAIN, LL_WARN,
              "ALLOCATE request was directed to QMS rather than QMM");
          else
            QRLogger::log(CAT_QMS_MAIN, LL_ERROR,
              "Unknown request type -- %s", request.getWrongRequestName());
          break;
      } // switch(RequestType)

      // collect and log qms stats
      if (qmsInitializer.doCollectQMSStats())
      {
        qmsInitializer.logQMSStats();
      }

      moreInput = request.readRequestType(requestType);

    } // while (moreInput)

  return 0;
} // End of processCommandLine
