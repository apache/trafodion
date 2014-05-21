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
/*
 * =====================================================================================
 *
 *       Filename:  protoMsg.h
 *
 *    Description:  header file for gpbMessage
 *
 *        Version:  1.0
 *        Created:  08/27/11 11:22:58
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef __PROTO_MSG_H
#define __PROTO_MSG_H

#include <string>
#include <vector>
#include <list>

#include <boost/any.hpp>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

#include "wrapper/amqpwrapper.h"
#include "common/evl_sqlog_eventnum.h"
#include "sqevlog/evl_sqlog_writer.h"

class GpbMessage // class to handle google protocol buffer message
{

   public:
      GpbMessage(google::protobuf::compiler::MultiFileErrorCollector
                 *errorCollector = NULL);
      ~GpbMessage();

      int import(const std::string &, AMQPRoutingKey &);

      //\! brief get field value from a message
      //\! return boost::any, if fails any is empty (check any.empty())
      boost::any getFieldValue(const std::string &) ;
//      boost::any getFieldValue(int32_t) ;

      std::vector<boost::any> getFieldValues(const std::string &) ;

      //\! brief get field type from a message
      //\! return the message_type of a field if the field is valid
      string getFieldType(const std::string &) ;
//      string getFieldType(int32_t) ;

      //\! brief set field value from a message,
      //\! return 0 if the operation is correct, or error code
      int setFieldValue(const std::string &, const std::string &);

      int addFieldValues(const std::string &, const std::vector<std::string> &);

      //\! brief update timestamps
      //\! return 0 if the operation is correct, or error code
      int updateTimestamp(const std::string &, const std::string &, const std::string & pnm = "");

      //\! brief get the human readable string, never fail
      std::string getDebugString() const;

      //\! brief get the compact string
      //\! return if fail, return an empty string
      std::string getDataString() const;

      //\! brief fill the message
      //\! return 0 if the operation succeed, or error code
      int fill(const std::string &);

      //\! brief fill the message
      //\! return 0 if the operation succeed, or error code
      int fill(std::list<std::string> &, int);

      //\! brief return information if the above operations failed
      std::string getFieldName() const { return fieldName_; }

   private:
      google::protobuf::compiler::MultiFileErrorCollector *errorCollector_;
      google::protobuf::compiler::DiskSourceTree sourceTree_;
      google::protobuf::compiler::Importer importer_;
      google::protobuf::DynamicMessageFactory messageFactory_;

      google::protobuf::Message *message_;

      std::string fieldName_;
};
#endif
