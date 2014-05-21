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
#ifndef TEXT_PROTOCOL_ADAPTER
#define TEXT_PROTOCOL_ADAPTER

#include <string>

#include <google/protobuf/compiler/importer.h>

#include "wrapper/externroutingkey.h"

class ProtoLoaderErrorCollector :
      public google::protobuf::compiler::MultiFileErrorCollector
{
   public:
      ProtoLoaderErrorCollector() { };
      ~ProtoLoaderErrorCollector() { };
      void AddError(const std::string &filename,
                           int32_t line,
                           int32_t column,
                           const std::string &message);
};


struct TextProtocolMessage 
{
   public:
      TextProtocolMessage() : data_(NULL), length_(0), size_(0) {}
      ~TextProtocolMessage() 
      {
         if (data_ != NULL) delete [] data_;
         length_ = size_ = 0;
      }

   public:
      char *data_;		//! Actual data
      int32_t length_;		//! Used length of the data
      int32_t size_;		//! Actual size of the data 
};


class TextProtocolAdapter
{
   public:
      //! Constructor
      TextProtocolAdapter();
      //! Desctructor
      ~TextProtocolAdapter();

      //! \brief init
      //! \param [in] timout the time to wait if something goes wrong
      //! \param [in] listenFifo name of the fifo to listen on
      //! \param [in] logFile name of the log file
      void init(size_t timeout, 
                          const std::string& listenFifo,
                          const std::string &protoSrc);

      //! \brief Runs the program, invoked by main.
      //! \param[in] host broker IP address
      //! \param[in] port broker port number
      //! \return program return code
      int doit(const std::string &host, int port);

   private:

      //! Populates the default components of the routing key.
      //! \param[in] extKey external routing key
      //! \param[in] intKey internal routing key
      //! return success or failure
      bool completeRoutingKey(std::string &extKey, AMQPRoutingKey &intKey);

      //! \brief Gets the next message.
      //! \param[in] fd file descriptor to read from
      //! \return success or failure
      int getNextMessage(int fd, TextProtocolMessage &message);

      void dispatchMessage(const std::string &data);

      //! \brief call getNextMessage and dispatch it
      //! \return SP_SUCCESS on success
      int getAndDispatchMessage();

      //! \brief tryInit try to init AMQP connection and fifo
      //! \param[in] host the hostname or ip address of broker
      //! \param[in] port the port number of broker
      int tryInit(const std::string &host, int port);

      //! Client name which indicates source of message
      std::string clientName_;

      //! wakeup time for retry connect to broker
      size_t timeout_;

      //! the name of the fifo
      std::string listenFifo_;
    
      //! file descriptor for fifo
      int fdFifo_;

      std::string protoSrc_;
};

#endif  // TEXT_PROTOCOL_ADAPTER

