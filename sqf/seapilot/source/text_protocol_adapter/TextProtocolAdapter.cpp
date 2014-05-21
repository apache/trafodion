// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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

#include <sys/types.h>    // mkfifo
#include <sys/stat.h>     // mkfifo
#include <fcntl.h>        // open
#include <errno.h>        // errno

#include <string>
#include <vector>
#include <list>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "sqevlog/evl_sqlog_writer.h"   // for SQEVL_SEABRIDGE

#include "common/spptLogger.h"
#include "common/protoMsg.h"
#include "TextProtocolAdapter.h"

using namespace std;
using namespace boost;
namespace fs=boost::filesystem;

// ProtoLoaderErrorCollector
void ProtoLoaderErrorCollector::AddError(const string &filename,
                           int32_t line,
                           int32_t column,
                           const string &message)
{
   spptErrorVar errVar;
   errVar.setFileName(filename);
   errVar.setFileLineNumber(line);
   errVar.setFileColumnNumber(column);
   errVar.setErrorString(message);

   logError(TPA_PUBNAME_IMPORT_ERROR, errVar);
}

inline void checkTime(struct timespec &tp0)
{
   struct timespec tp1;
   clock_gettime(CLOCK_REALTIME, &tp1);    

   string strTime("Time elapsed: ");
   strTime.append(str(format("%.6f") % (tp1.tv_sec - tp0.tv_sec 
                                       + (tp1.tv_nsec - tp0.tv_nsec)*1e-9))
                 ).append(" seconds.");

   logTrace(strTime);
   tp0 = tp1;
}

///////////////////////////////////////////////////////////
// class TextProtocolAdapter
TextProtocolAdapter::TextProtocolAdapter()
   : fdFifo_(0)
{
}

TextProtocolAdapter::~TextProtocolAdapter()
{
   if (fdFifo_ != 0) {
      close(fdFifo_);
   }
   remove(listenFifo_.c_str());
}

void TextProtocolAdapter::init(size_t timeout,
                 const string& listenFifo, const string &protoSrc)
{
   timeout_ = timeout;
   listenFifo_ = listenFifo;
   protoSrc_ = protoSrc;
}

bool TextProtocolAdapter::completeRoutingKey(string &extKey, AMQPRoutingKey &intKey)
{
   bool gotError = false;
   AMQPRoutingKey partialKey;

   SP_PublicationCategoryType category;
   SP_PublicationPackageType  package;
   SP_PublicationScopeType    scope;
   SP_PublicationSecurityType security;
   SP_PublicationProtocolType protocol;
   string publicationName;

   SPExtRoutingKeyErr retErr = ExtRoutingKey_ToInternal(extKey, &partialKey);
   if(retErr != SPERKERR_NOERR)
   {
      string strError;
      ExtRoutingKey_ErrToText(retErr, &strError);

      spptErrorVar errVar;
      errVar.setRouteKey(extKey);
      errVar.setErrorString(strError);

      logError(TPA_ROUTINGKEY_CONVERSION_ERROR, errVar);

      gotError = true;
   }
   if(!gotError)
   {
      if((category = partialKey.GetCategory()) == SP_NULLCATEGORY) {
         // Category may be supplied, default is health/state
         category = SP_HEALTH_STATE;
      }

      if((package = partialKey.GetPackage()) == SP_NULLPACKAGE) {
         // Package must be supplied
         spptErrorVar errVar;
         errVar.setRouteKey(extKey);
         errVar.setErrorString("package");

         logError(TPA_ROUTINGKEY_MISSING_COMPONENT_ERROR, errVar);
         gotError = true;
      }

      if((scope = partialKey.GetScope()) == SP_NULLSCOPE) {
         // Scope must be supplied
         spptErrorVar errVar;
         errVar.setRouteKey(extKey);
         errVar.setErrorString("scope");

         logError(TPA_ROUTINGKEY_MISSING_COMPONENT_ERROR, errVar);
         gotError = true;
      }

      if((security = partialKey.GetSecurity()) == SP_NULLSECURITY) {
         // Security must be supplied
         spptErrorVar errVar;
         errVar.setRouteKey(extKey);
         errVar.setErrorString("security");

         logError(TPA_ROUTINGKEY_MISSING_COMPONENT_ERROR, errVar);
         gotError = true;
      }

      if(partialKey.GetProtocol() == SP_NULLPROTOCOL) {
         // Default protocol
         protocol = SP_GPBPROTOCOL;
      }
      else {
         // Protocol must not be supplied
         spptErrorVar errVar;
         errVar.setRouteKey(extKey);

         logError(TPA_ROUTINGKEY_PROTOCOL_ERROR, errVar);
         gotError = true;
      }

      if((publicationName = partialKey.GetPublicationName()) == "") {
         // Publication name must be supplied
         spptErrorVar errVar;
         errVar.setRouteKey(extKey);
         errVar.setErrorString("publication name");

         logError(TPA_ROUTINGKEY_MISSING_COMPONENT_ERROR, errVar);
         gotError = true;
      }
   }

   if(!gotError)
   {
      AMQPRoutingKey completeKey(category, package, scope, security, 
                                 protocol, publicationName);
      intKey = completeKey;
   }

   return gotError;
}

int TextProtocolAdapter::getNextMessage(int fd, TextProtocolMessage &message)
{
   logTrace("getNextMessage");

   clientName_ = "not determined";

   bool gotError = false;
   // will read the header into here
   const size_t HEADER_SIZE = 6;
   char headerBuffer[HEADER_SIZE] = "";

   // read the header, which is the number of characters in the message, terminated by a ':'
   int currentByte = 0;
   size_t bytesRead;
   do {
      bytesRead = read(fd, headerBuffer+currentByte, 1);
   } while( (headerBuffer[currentByte++] != ':') && (bytesRead > 0) && (currentByte < HEADER_SIZE));

   if(bytesRead <= 0)
   {
      // must have failed to read correctly for some reason
      spptErrorVar errVar;
      errVar.setErrorString("header");

      logError(TPA_MESSAGE_READ_ERROR, errVar);
      return TPA_MESSAGE_READ_ERROR;
   }
   
   int32_t msgSize = strtol(headerBuffer, NULL, 10);
   if (msgSize <= 0) {
      // may be something wrong in the header buffer
      spptErrorVar errVar;
      errVar.setErrorString("header");

      logError(TPA_MESSAGE_READ_ERROR, errVar);
      return TPA_MESSAGE_READ_ERROR;
   }

   message.length_= msgSize;

   // allocate memory
   if (message.size_ <= message.length_) { // need to enlarge 

      if (message.data_ != NULL) { // free 
         delete[] message.data_;
      }

      message.size_ = message.length_ * 2 + 1;
      message.data_ = new char[message.size_];
   }

   // do the actual read
   currentByte = 0;
   do
   {
      bytesRead = read(fd, message.data_+currentByte, message.length_ - currentByte -1);
      currentByte += bytesRead;
   } while((bytesRead > 0) && (currentByte < message.length_ - 1));

   message.data_[currentByte] = '\0';

   if(bytesRead <= 0)
   {
      // must have failed to read correctly for some reason
      spptErrorVar errVar;
      errVar.setErrorString("body");

      logError(TPA_MESSAGE_READ_ERROR, errVar);
      return TPA_MESSAGE_READ_ERROR;
   }

   char trailerChar[1];

   // read the trailing \n
   bytesRead = read(fd, trailerChar, 1);
   if(bytesRead <= 0)
   {
      // must have failed to read correctly for some reason
      spptErrorVar errVar;
      errVar.setErrorString(message.data_);

      logError(TPA_MESSAGE_TRAILER_ERROR, errVar);
      return TPA_MESSAGE_TRAILER_ERROR;
   }

   if('\n' != trailerChar[0])
   {
      // message has some extra stuff; clean it out
      do
      {
         bytesRead = read(fd, trailerChar, 1);
      }while((bytesRead > 0) && ('\n' != trailerChar[0]) );
      spptErrorVar errVar;
      errVar.setErrorString(message.data_);

      logError(TPA_MESSAGE_TRAILER_ERROR, errVar);
      TPA_MESSAGE_TRAILER_ERROR;
   }
 
   logTrace(message.data_);

   return 0;
}

void TextProtocolAdapter::dispatchMessage(const string& data)
{
   logTrace("dispatchMessage");

   vector<string> data_vec;

   split(data_vec, data, boost::is_any_of(":"));

    //pluck out the routing key
   string extKey(data_vec[0]);
    
   // data_vec[1] : timestampUTC;
   // data_vec[2] : timestampLCT

   //pluck out the client name
   clientName_ = data_vec[3];
   AMQPRoutingKey intKey;

   if(!completeRoutingKey(extKey, intKey)) {
      bool gotError = false;

      ProtoLoaderErrorCollector errorCollector;
      GpbMessage msg(&errorCollector);

      int32_t rc;
      if (SP_SUCCESS == (rc = msg.import(protoSrc_, intKey))) {

         list<string> tokenlist;
         split(tokenlist, data, boost::is_any_of("\t")); // assuming there will be no tab 
                                                // before the <info_header>
         if (0 != (rc = msg.fill(tokenlist, SQEVL_SEABRIDGE))) {
            spptErrorVar errVar;
            switch (rc) {
                case SP_PROTO_MSG_NOT_INIT:
                   break;
                case SP_PROTO_INVALID_REPEATED_NUMBER:
                   errVar.setErrorString(data);
                   logError(TPA_INVALID_REPEATED_NUMBER, errVar);
                   break;

                case SP_PROTO_REQUIRED_FIELD_EMPTY:
                   errVar.setPublicationColumn(msg.getFieldName());
                   logError(TPA_FIELD_ASSIGN_NULL_ERROR, errVar);
                   break;

                case SP_PROTO_INVALID_FIELDNAME:
                   errVar.setPublicationColumn(msg.getFieldName());
                   errVar.setPublicationName(intKey.GetAsMessageName());
                   logError(TPA_FIELD_NOT_FOUND_ERROR, errVar);
                   break;

                case SP_PROTO_INVALID_FIELDTYPE:
                   errVar.setPublicationColumn(msg.getFieldName());
                   logError(TPA_UNSUPPORTED_FIELD_ERROR, errVar);
            }
            
            gotError = true;
         }
         else {
            //update timestamp
            rc = msg.updateTimestamp(data_vec[1], data_vec[2], data_vec[3]);

            if (SP_SUCCESS != rc) {
               gotError = true;
               printf("TPA: error update\n");
            }
         }

         // Trace the message
         int64_t timestampUTC = strtoll(data_vec[1].c_str(), NULL, 10);
         time_t messageTime = timestampUTC/1000000;

         char buf[256];
         ctime_r(&messageTime, buf);
         buf[strlen(buf)-1] = '\0';

         string strMsg("Message time = ");
         strMsg.append(buf);

         struct timespec tp0;
         clock_gettime(CLOCK_REALTIME, &tp0);

         ctime_r(&tp0.tv_sec, buf);
         buf[strlen(buf)-1] = '\0';

         strMsg.append(", TPA time = ").append(buf).append(", Elapsed time = ");
         strMsg.append(str(format("%.6f") % (tp0.tv_sec + tp0.tv_nsec/1e9 - timestampUTC/1e6)));
         strMsg.append(" seconds");

         if (gotError) {
              strMsg.append(", Message = ").append(data);
         }
         else {
            logTrace("Routingkey: " + intKey.GetAsString());
            logTrace("Msg: " + msg.getDebugString());

            string strDataString = msg.getDataString();
            if (!strDataString.empty()) {
               int sendErr = sendAMQPMessage(false, strDataString,
                                      SP_CONTENT_TYPE_APP, intKey, false);
               if(sendErr != SP_SUCCESS)
               {
                  logTrace("TPA: send error " + intKey.GetAsString() +"Msg: " + msg.getDebugString());
                  spptErrorVar errVar;
                  errVar.setRouteKey(extKey);

                  logError(TPA_MESSAGE_DESERIALIZE_ERROR, errVar);
               }
               else
                  logTrace("TPA: send OK " + intKey.GetAsString() + "Msg: " + msg.getDebugString() );

            }
         }

         logInfo(strMsg);
      } 
      else {
         spptErrorVar errVar;
         errVar.setFileName(intKey.GetAsProtofileName());

         logError(TPA_PROTO_IMPORT_FAIL, errVar);
      }
   } // if(!completeRoutingKey(extKey, intKey))
}

int TextProtocolAdapter::tryInit(const string &host, int port)
{
   logTrace("Removing old fifo " + listenFifo_);

   int ret = 0;

   fs::path pFifo(listenFifo_);
   if (exists(pFifo)) {
      ret = remove(listenFifo_.c_str());
   }
 
   if (ret != 0) {
      spptErrorVar errVar;
      errVar.setFileName(listenFifo_);
      errVar.setErrorNumber(ret);

      logError(TPA_REMOVE_FILE_ERROR, errVar);
      return TPA_REMOVE_FILE_ERROR;
   }

   usleep(500000); // wait for a while, 0.5 second

   logTrace("Making fifo " + listenFifo_);

   ret = mkfifo(listenFifo_.c_str(), 0770);
   if (ret < 0)
   {
      spptErrorVar errVar;
      errVar.setFileName(listenFifo_);
      errVar.setErrorNumber(ret);

      logError(TPA_FIFO_CREATE_ERROR, errVar);
      return TPA_FIFO_CREATE_ERROR;
   }
   logTrace("Creating OK");

   logTrace("Open fifo for reading");

   fdFifo_ = open(listenFifo_.c_str(), O_RDWR);
   if (fdFifo_ <= 0) {
      spptErrorVar errVar;
      errVar.setFileName(listenFifo_);
      errVar.setErrorNumber(errno);

      logError(TPA_FIFO_OPEN_ERROR, errVar);
      return TPA_FIFO_OPEN_ERROR;
   }
   logTrace("Opening OK");

   logTrace(str(format("Connect to %s : %d") % host % port));

   ret = createAMQPConnection(host.c_str(), port);
   if (ret != SP_SUCCESS)
   {
      // A failed connection must be reported directly.
      spptErrorVar errVar;
      errVar.setErrorString(str(format("%s:%d") % host % port));

      logError(TPA_AMQP_CONNECT_ERROR, errVar);
      return TPA_AMQP_CONNECT_ERROR;
   }
   logTrace("Connecting OK");

   return SP_SUCCESS;
}

int TextProtocolAdapter::getAndDispatchMessage()
{
   int ret = SP_SUCCESS;

   TextProtocolMessage m;

   if(fdFifo_ > 0) {
#ifdef _DEBUG
      //init timecheck
      struct timespec tp0;
      clock_gettime(CLOCK_REALTIME, &tp0); 
#endif
      if (SP_SUCCESS == (ret=getNextMessage(fdFifo_, m))) {
#ifdef _DEBUG
         checkTime(tp0);
#endif
         dispatchMessage(m.data_);
      }
      else {
         close(fdFifo_);
      }
#ifdef _DEBUG
      checkTime(tp0);
#endif
   }
   else {
      spptErrorVar errVar;
      errVar.setFileName(listenFifo_);
      errVar.setErrorNumber(errno);

      logError(TPA_FIFO_OPEN_ERROR, errVar);
      ret = TPA_FIFO_OPEN_ERROR;
   }

   return ret;
}

enum {STATE_DOWN = 0, STATE_RECOVERY, STATE_UP, STATE_SHUTDOWN};

#ifdef _DEBUG
const char* stateName[] = {"Down", "Recovery", "Main", "Shutdown"};
#endif
int TextProtocolAdapter::doit(const string &host, 
                        int port) 
{
   int retCode = 0;

   int curState = STATE_RECOVERY; //init state

   bool shutdown = false;
   do {
#ifdef _DEBUG
      stringstream ss;
      ss << "Curstate: "<<stateName[curState];
      logTrace(ss.str());
#endif
      switch (curState) {
      case STATE_DOWN:
         sleep(timeout_);
         curState = STATE_RECOVERY;
         break;

      case STATE_RECOVERY:
         retCode = tryInit(host, port);
         if (retCode == SP_SUCCESS) {
            curState = STATE_UP;
         }
         else {
            curState = STATE_DOWN;
         }
         break;

      case STATE_UP:
         retCode = getAndDispatchMessage();
         if (retCode != SP_SUCCESS) {
            curState = STATE_DOWN;
         }
         break;

      case STATE_SHUTDOWN:
         shutdown = true;
         break;

      default:
         break;
      }
   } while (!shutdown);

   return retCode;
}

