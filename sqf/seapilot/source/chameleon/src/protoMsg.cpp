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
 * ============================================================================
 *
 *       Filename:  protoMsg.cpp
 *
 *    Description:  source file for class GpbMessage
 *
 *        Version:  1.0
 *        Created:  08/27/11 11:22:58
 *       Revision:  none
 *       Compiler:  gcc
 *
 * ============================================================================
 */

#include <boost/foreach.hpp>

#include "common/protoMsg.h"


using namespace std;
using namespace google;

#define foreach BOOST_FOREACH

// declaration of private utilities
static boost::any GetMsgFieldValue(const protobuf::Message &,
                                 const string &);
static boost::any GetMsgFieldValue(const protobuf::Message &msg,
                        const protobuf::FieldDescriptor *field);

static vector<boost::any> GetMsgFieldValues(const protobuf::Message &,
                                 const string &);
static vector<boost::any> GetMsgFieldValues(const protobuf::Message &msg,
                        const protobuf::FieldDescriptor *field);

static string GetMsgFieldType(const protobuf::Message &,
                            const string &);

static int SetMsgFieldValue(protobuf::Message *msg,
                           const string &, const string &);
static int SetMsgFieldValue(protobuf::Message *msg,
                           const protobuf::FieldDescriptor *,
                           const string &);

static int AddMsgFieldValues(protobuf::Message *msg,
                           const string &, const vector<string> &);
static int AddMsgFieldValue(protobuf::Message *msg,
                           const protobuf::FieldDescriptor *,
                           const string &);
static int UpdateMsgTimestamp(protobuf::Message *msg,
                           const string &, const string &, const string &pnm);

static int FillMessage(protobuf::Message *, list<string> &,
                      int, string &);

static int InitInfoHeader(protobuf::Message *, int,
                         const protobuf::FieldDescriptor *);

// endo of declaration of private utilities

inline string getNextToken(list<string> &tokenlist)
{
   string token;
   if (!tokenlist.empty()) {
      token = tokenlist.front();
      tokenlist.pop_front();
   }
   return token;
}

///////////////////////////////////////////////////////////
// class GbpMessage
GpbMessage::GpbMessage(protobuf::compiler::MultiFileErrorCollector *errorCollector) 
   : errorCollector_(errorCollector)
   , importer_(&sourceTree_, errorCollector_)
   , messageFactory_(importer_.pool())
   , message_(NULL)
{
}

GpbMessage::~GpbMessage()
{
   if (NULL != message_) {
      delete message_;
      message_ = NULL;
   }
}

// public interfaces
int GpbMessage::import(const string &protoSrc, AMQPRoutingKey &routingKey)
{
   if (NULL != message_) {
      delete message_;
      message_ = NULL;
   }
 
   string sRoot(""), sTmp;
   if( !sourceTree_.VirtualFileToDiskFile(sRoot, &sTmp)) {
      sourceTree_.MapPath(sRoot, protoSrc);
   }

   string pubName = routingKey.GetAsMessageName();

   importer_.Import(routingKey.GetAsProtofileName());
   const protobuf::Descriptor 
      *desc = importer_.pool()->FindMessageTypeByName(pubName);

   if(NULL != desc) {
      const protobuf::Message *protoMsg = messageFactory_.GetPrototype(desc);
 
      if (NULL != protoMsg) {
         message_ = protoMsg->New();
      }
   }

   return NULL != message_ ? 0: SP_PROTO_IMPORT_FAIL;
}

boost::any GpbMessage::getFieldValue(const string &fieldName)  
{
   fieldName_ = fieldName;
    return NULL == message_ ? boost::any() : GetMsgFieldValue(*message_, fieldName);  
} // getFieldValue

vector<boost::any> GpbMessage::getFieldValues(const string &fieldName)
{
   vector<boost::any> v;
   fieldName_ = fieldName;
   return NULL == message_ ? v : GetMsgFieldValues(*message_, fieldName);
}

string GpbMessage::getFieldType(const string &fieldName)
{
   fieldName_ = fieldName;
   return NULL == message_ ? "" : GetMsgFieldType(*message_, fieldName);
} // getFieldType

int GpbMessage::setFieldValue(const string &fieldName, const string &token)
{
   fieldName_ = fieldName;
   return NULL == message_ ?
      SP_PROTO_MSG_NOT_INIT : SetMsgFieldValue(message_, fieldName, token);
} // setFieldValue

int GpbMessage::addFieldValues(const string &fieldName, 
                               const vector<string> &tokens)
{
   fieldName_ = fieldName;
   return NULL == message_ ?
      SP_PROTO_MSG_NOT_INIT : AddMsgFieldValues(message_, fieldName, tokens);
} // addFieldValues

int GpbMessage::updateTimestamp(const string &strUtc, const string &strLct, const string &pnm)
{
   return NULL == message_ ?
      SP_PROTO_MSG_NOT_INIT : UpdateMsgTimestamp(message_, strUtc, strLct, pnm);
}

int GpbMessage::fill(const string &data)
{
   if (NULL == message_) {
      return SP_PROTO_MSG_NOT_INIT;
   }

   message_->ParseFromString(data);
   return 0;
} // fill

int GpbMessage::fill(list<string> &tokenlist, int componentId)
{
   return NULL == message_ ?
      SP_PROTO_MSG_NOT_INIT : FillMessage(message_, tokenlist, 
                                          componentId, fieldName_);
} // fill

string GpbMessage::getDebugString() const
{
   return NULL == message_ ? "" : message_->DebugString();
}

string GpbMessage::getDataString() const
{
   if (NULL == message_) {
      return "";
   }

   string data;
   if (message_->SerializeToString(&data)) {
      return data;
   }
   else {
      return "";
   }
}
// end of class GbpMessage

///////////////////////////////////////////////////////////
// definitions of private utilities
boost::any GetMsgFieldValue(const protobuf::Message &msg,
                                       const string &fieldName) 
{
   const protobuf::Descriptor *desc = msg.GetDescriptor();
   size_t npos;
   if ((npos = fieldName.find('.')) != string::npos) 
   {
      // embedded message
      string prefix = fieldName.substr(0, npos);
      string suffix = fieldName.substr(npos+1);

      const protobuf::FieldDescriptor *field = desc->FindFieldByName(prefix);
      if (NULL == field) {
         return boost::any();
      }
      const protobuf::Reflection *reflect = msg.GetReflection();
      const protobuf::Message &aMsg = reflect->GetMessage(msg, field);
      return GetMsgFieldValue(aMsg, suffix);
   }

   const protobuf::FieldDescriptor *field = desc->FindFieldByName(fieldName);

   return NULL == field ? boost::any() : GetMsgFieldValue(msg, field);
} // getMsgFieldValue

boost::any GetMsgFieldValue(const protobuf::Message &msg,
                            const protobuf::FieldDescriptor *field) 
{
   boost::any a_value;
   const protobuf::Reflection *reflect = msg.GetReflection();
   switch (field->cpp_type()) {
      case protobuf::FieldDescriptor::CPPTYPE_INT32:
         a_value = reflect->GetInt32(msg, field);
         break;

      case protobuf::FieldDescriptor::CPPTYPE_INT64:
         a_value = reflect->GetInt64(msg, field);
         break;

      case protobuf::FieldDescriptor::CPPTYPE_UINT32:
         a_value = reflect->GetUInt32(msg, field);
         break;

      case protobuf::FieldDescriptor::CPPTYPE_UINT64:
         a_value = reflect->GetUInt64(msg, field);
         break;

      case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
         a_value = reflect->GetDouble(msg, field);
         break;

      case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
         a_value = reflect->GetFloat(msg, field);
         break;

      case protobuf::FieldDescriptor::CPPTYPE_STRING:
         a_value = reflect->GetString(msg, field);
         break;

      case protobuf::FieldDescriptor::CPPTYPE_BOOL:
      case protobuf::FieldDescriptor::CPPTYPE_ENUM:
      case protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
      default:
         break;
   }

   return a_value;
} // getMsgFieldValue(const Message &msg, 

vector<boost::any> GetMsgFieldValues(const protobuf::Message &msg,
                            const string &fieldName) 
{
   const protobuf::Descriptor *desc = msg.GetDescriptor();
   vector<boost::any> v;
   size_t npos;
   if ((npos = fieldName.find('.')) != string::npos)
   {
      // embedded message
      string prefix = fieldName.substr(0, npos);
      string suffix = fieldName.substr(npos+1);

      const protobuf::FieldDescriptor *field = desc->FindFieldByName(prefix);
      if (NULL == field) {
         return v;
      }

      const protobuf::Reflection *reflect = msg.GetReflection();
      const protobuf::Message &aMsg = reflect->GetMessage(msg, field);

      return GetMsgFieldValues(aMsg, suffix);
   }

   const protobuf::FieldDescriptor *field = desc->FindFieldByName(fieldName);

   return NULL == field ? v : GetMsgFieldValues(msg, field);
}

vector<boost::any> GetMsgFieldValues(const protobuf::Message &msg,
                            const protobuf::FieldDescriptor *field) 
{
   vector<boost::any> v;

   if ( !field->is_repeated()) {
      return v;
   }

   boost::any a_value;
   const protobuf::Reflection *reflect = msg.GetReflection();
   int32_t fieldSize = reflect->FieldSize(msg, field);

   for(int32_t idx = 0; idx < fieldSize; ++idx) 
   {
      switch (field->cpp_type()) {
         case protobuf::FieldDescriptor::CPPTYPE_INT32:
            a_value = reflect->GetRepeatedInt32(msg, field, idx);
            break;

         case protobuf::FieldDescriptor::CPPTYPE_INT64:
            a_value = reflect->GetRepeatedInt64(msg, field, idx);
            break;
   
         case protobuf::FieldDescriptor::CPPTYPE_UINT32:
            a_value = reflect->GetRepeatedUInt32(msg, field, idx);
            break;
   
         case protobuf::FieldDescriptor::CPPTYPE_UINT64:
            a_value = reflect->GetRepeatedUInt64(msg, field, idx);
            break;
   
         case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            a_value = reflect->GetRepeatedDouble(msg, field, idx);
            break;

         case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            a_value = reflect->GetRepeatedFloat(msg, field, idx);
            break;

         case protobuf::FieldDescriptor::CPPTYPE_STRING:
            a_value = reflect->GetRepeatedString(msg, field, idx);
            break;

         case protobuf::FieldDescriptor::CPPTYPE_BOOL:
         case protobuf::FieldDescriptor::CPPTYPE_ENUM:
         case protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
         default:
            break;
      }
      if (!a_value.empty()) {
         v.push_back(a_value);
      }
   }
   return v;
}

string GetMsgFieldType(const protobuf::Message &msg,
                                  const string &fieldName) 
{
   const protobuf::Descriptor *desc = msg.GetDescriptor();
   size_t npos;
   if ((npos = fieldName.find('.')) != string::npos)
   {
      string prefix = fieldName.substr(0, npos);
      string suffix = fieldName.substr(npos+1);

      const protobuf::FieldDescriptor *field = desc->FindFieldByName(prefix);
      if (NULL == field) {
         return "";
      }

      const protobuf::Reflection *reflect = msg.GetReflection();
      const protobuf::Message &aMsg = reflect->GetMessage(msg, field);

      return GetMsgFieldType(aMsg, suffix);
   }

   const protobuf::FieldDescriptor *field = desc->FindFieldByName(fieldName);

   if (NULL == field) {
      return  "";
   } 
   else {
      return field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE ?
         field->message_type()->name() : "";
   }
} // getMsgFieldType

int SetMsgFieldValue(protobuf::Message *msg, const string &fieldName, 
                        const string &token)
{
   const protobuf::Descriptor *desc = msg->GetDescriptor();
   size_t npos;
   if ((npos = fieldName.find('.')) != string::npos)
   {
      // embedded message
      string prefix = fieldName.substr(0, npos);
      string suffix = fieldName.substr(npos+1);

      const protobuf::FieldDescriptor *field = desc->FindFieldByName(prefix);
      if (NULL == field) {
         return SP_PROTO_INVALID_FIELDNAME;
      }
      const protobuf::Reflection *reflect = msg->GetReflection();
      protobuf::Message *pMsg = reflect->MutableMessage(msg, field);

      return SetMsgFieldValue(pMsg, suffix, token);
   }

   const protobuf::FieldDescriptor *field = desc->FindFieldByName(fieldName);

   if (NULL == field) {
      return SP_PROTO_INVALID_FIELDNAME;
   }

   return SetMsgFieldValue(msg, field, token);

} // setMsgFieldValue

int SetMsgFieldValue(protobuf::Message *msg,
           const protobuf::FieldDescriptor *field, const string &token)
{
   if (token.empty()) {
      return field->is_required() ? SP_PROTO_REQUIRED_FIELD_EMPTY : 0;
   }

   const protobuf::Reflection *reflect = msg->GetReflection();

   switch(field->cpp_type()) 
   {
      case protobuf::FieldDescriptor::CPPTYPE_INT32:
      {
         int32_t val = strtol(token.c_str(), NULL, 10);
         reflect->SetInt32(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_INT64:
      {
         int64_t val = strtoll(token.c_str(), NULL, 10);
         reflect->SetInt64(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_UINT32:
      {
         uint32_t val = strtoul(token.c_str(), NULL, 10);
         reflect->SetUInt32(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_UINT64:
      {
         uint64_t val = strtoull(token.c_str(), NULL, 10);
         reflect->SetUInt64(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      {
         float val = strtof(token.c_str(), NULL);
         reflect->SetFloat(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      {
         double val = strtod(token.c_str(), NULL);
         reflect->SetDouble(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_STRING:
         reflect->SetString(msg, field, token.c_str());
         break;

      case protobuf::FieldDescriptor::CPPTYPE_BOOL:
      case protobuf::FieldDescriptor::CPPTYPE_ENUM:
      case protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
      default:
         return SP_PROTO_INVALID_FIELDTYPE;
         break;
   }

   return 0;
}// setMsgFieldValue

int AddMsgFieldValues(protobuf::Message *msg, 
                        const string &fieldName, const vector<string> &tokens)
{
   const protobuf::Descriptor *desc = msg->GetDescriptor();
   size_t npos;
   if ((npos = fieldName.find('.')) != string::npos)
   {
      // embedded message
      string prefix = fieldName.substr(0, npos);
      string suffix = fieldName.substr(npos+1);

      const protobuf::FieldDescriptor *field = desc->FindFieldByName(prefix);
      if (NULL == field) {
         return SP_PROTO_INVALID_FIELDNAME;
      }
      const protobuf::Reflection *reflect = msg->GetReflection();
      protobuf::Message *pMsg = reflect->MutableMessage(msg, field);

      return AddMsgFieldValues(pMsg, suffix, tokens);
   }

   const protobuf::FieldDescriptor *field = desc->FindFieldByName(fieldName);

   if (NULL == field || !field->is_repeated()) {
      return SP_PROTO_INVALID_FIELDNAME;
   }

   for (uint32_t idx=0; idx < tokens.size(); ++idx) {
      int32_t rc = AddMsgFieldValue(msg, field, tokens[idx]);
      if (rc) {
         return rc;
      }
   }

   return 0;
} // addMsgFieldValues


int AddMsgFieldValue(protobuf::Message *msg, 
           const protobuf::FieldDescriptor *field, const string &token)
{
   if (token.empty()) {
      return field->is_required() ? SP_PROTO_REQUIRED_FIELD_EMPTY : 0;
   }
   const protobuf::Reflection *reflect = msg->GetReflection();

   switch(field->cpp_type()) 
   {
      case protobuf::FieldDescriptor::CPPTYPE_INT32:
      {
         int32_t val = strtol(token.c_str(), NULL, 10);
         reflect->AddInt32(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_INT64:
      {
         int64_t val = strtoll(token.c_str(), NULL, 10);
         reflect->AddInt64(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_UINT32:
      {
         uint32_t val = strtoul(token.c_str(), NULL, 10);
         reflect->AddUInt32(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_UINT64:
      {
         uint64_t val = strtoull(token.c_str(), NULL, 10);
         reflect->AddUInt64(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      {
         float val = strtof(token.c_str(), NULL);
         reflect->AddFloat(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      {
         double val = strtod(token.c_str(), NULL);
         reflect->AddDouble(msg, field, val);
         break;
      }
      case protobuf::FieldDescriptor::CPPTYPE_STRING:
         reflect->AddString(msg, field, token.c_str());
         break;

      case protobuf::FieldDescriptor::CPPTYPE_BOOL:
      case protobuf::FieldDescriptor::CPPTYPE_ENUM:
      case protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
      default:
         return SP_PROTO_INVALID_FIELDTYPE;
         break;
   }

   return 0;
} // addMessageValue

int FillMessage(protobuf::Message *msg, list<string> &tokenlist, 
                            int componentId, string &curField)
{
   const protobuf::Reflection *reflect = msg->GetReflection();
   const protobuf::Descriptor *desc = msg->GetDescriptor();
   for(int32_t idxTop = 0; idxTop < desc->field_count(); idxTop++)
   {
      string token = getNextToken(tokenlist);
      int rc;

      const protobuf::FieldDescriptor *field = desc->field(idxTop);
      if (field->is_repeated())
      {
         int32_t numRepeat = strtol(token.c_str(), NULL, 10);

         if (numRepeat < 0) {
           return SP_PROTO_INVALID_REPEATED_NUMBER;
         }
         
         for(int32_t idxRepeat = 0; idxRepeat < numRepeat; ++idxRepeat)
         {
            if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) 
            {
               protobuf::Message *pMsg = reflect->AddMessage(msg, field);
               if ((rc = FillMessage(pMsg, tokenlist, componentId, curField)) != 0) {
                  return rc;
               }
            }
            else
            {
               token = getNextToken(tokenlist);
               curField = field->name();
               if ((rc = AddMsgFieldValue(msg, field, token)) != 0) {
                  return rc;
               }
            }
         }
      }
      else {
         if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE) 
         {
            string msgType = field->message_type()->name();

            if ("info_header" == msgType) {
               InitInfoHeader(msg, componentId, field);
            }
            else {
               protobuf::Message *msgField = 
                             reflect->MutableMessage(msg, field);
               tokenlist.push_front(token); //unswitch one token for non-header
               if ((rc = FillMessage(msgField, tokenlist, componentId, curField)) != 0) {
                  return rc;
               }
            }
         }
         else {
            curField = field->name();
            if ((rc = SetMsgFieldValue(msg, field, token)) != 0) {
               return rc;
            }
         }
      }
   }
   
   return 0;
} // FillMessage

int UpdateMsgTimestamp(protobuf::Message *msg, 
      const string &strUtc, const string &strLct, const string &pnm )
{
   int rc = 0;
   const protobuf::Descriptor* desc = msg->GetDescriptor();
   for(int index=0; index < desc->field_count(); index++)
   {
      const protobuf::FieldDescriptor* field = desc->field(index);
      if (field->cpp_type() == protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
      {
         if (field->is_repeated()) {
            continue;
         }
         const protobuf::Reflection *reflect = msg->GetReflection();
         protobuf::Message *pMsg = reflect->MutableMessage(msg, field);
         rc = UpdateMsgTimestamp(pMsg, strUtc, strLct,pnm);

         if (rc) return rc;
      }
      else {
         if (field->name() == "info_generation_time_ts_utc" 
            || field->name() == "generation_time_ts_utc" ) {
            rc = SetMsgFieldValue(msg, field, strUtc);

            if (rc) return rc;
         }
         else if (field->name() == "info_generation_time_ts_lct" 
            || field->name() == "generation_time_ts_lct" ) {
            rc = SetMsgFieldValue(msg, field, strLct);

            if (rc) return rc;
         }
         else if  (field->name() == "info_process_name") {
            if(pnm != "")//update it
              rc = SetMsgFieldValue(msg, field, pnm);
            if (rc) return rc;
         }
      }
   }

   return 0;
}

int InitInfoHeader(protobuf::Message *msg, int componentId,
                               const protobuf::FieldDescriptor *fdInfoHeader)
{
   const protobuf::Reflection *reflect = msg->GetReflection();
   protobuf::Message *msgInfoHeader = reflect->MutableMessage(msg, fdInfoHeader);

   int initErr = initAMQPInfoHeaderReflecting(msgInfoHeader, 
                                              componentId, fdInfoHeader);

   return initErr;
} //initInfoHeader

