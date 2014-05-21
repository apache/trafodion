// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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

#include "PTPAMessageTextParameters.h"

#include <sstream>

#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>

using namespace google;
using namespace Trinity;

// Constructor
PTPAMessageTextParameters::PTPAMessageTextParameters() : msg_(0)
{
    // nothing more to do
}

// Destructor
PTPAMessageTextParameters::~PTPAMessageTextParameters()
{
    // nothing to do; note that we do not own the Google protobuf object so
    // we don't free it
}

const char * PTPAMessageTextParameters::defaultMessage()
{
    return defaultMessage_.c_str();
}

std::string PTPAMessageTextParameters::getParameterValue(std::string & parameterName)
{
    std::string value;  // return empty string if we don't find value

    if (msg_)
    {
        // Traverse to the innermost nesting, extract the field value,
        // convert that to a string, and return. If we don't find the
        // field value, return the empty string. (We might not find it
        // simply because it is absent.)

        const protobuf::Message * msg = msg_;
        const protobuf::Reflection *reflect = msg_->GetReflection();
        const protobuf::FieldDescriptor *fd = 0;
        const protobuf::Descriptor *messageDesc =  msg_->GetDescriptor();

        std::string fieldName = parameterName;
        size_t dotPosition = 0;

        while (std::string::npos != (dotPosition = fieldName.find('.'))) // nested assignment is intentional
        {
            std::string nestedMessageFieldName = fieldName.substr(0,dotPosition);  // peel off next qualifier
            fieldName = fieldName.substr(dotPosition+1);  // peel that qualifier off the front

            fd = messageDesc->FindFieldByName(nestedMessageFieldName);
            if (fd)  // if we found it
            {
                if (protobuf::FieldDescriptor::CPPTYPE_MESSAGE == fd->cpp_type())
                {
                    // Traverse down one level of nesting to the nested message
                    msg = &reflect->GetMessage(*msg, fd);
                    reflect = msg->GetReflection();
                    messageDesc = msg->GetDescriptor();
                }
                else
                {
                    // very strange; we found a qualifier, but it is not
                    // a nested message; we don't expect that
                    messageDesc = 0;
                }

            }
            else
            {
                // didn't find the nested message field
                messageDesc = 0;
            }
        }

        // if we successfully traversed to the innnermost message, find the field

        if (messageDesc)
        {
            fd = messageDesc->FindFieldByName(fieldName);
            if ((fd) && (reflect->HasField(*msg,fd)))
            {
                switch (fd->cpp_type())
                {
                case protobuf::FieldDescriptor::CPPTYPE_INT32 :
                {
                    int32_t dest = reflect->GetInt32(*msg, fd);
                    std::stringstream out;
                    out << dest;
                    value = out.str();
                }
                    break;
                case protobuf::FieldDescriptor::CPPTYPE_INT64 :
                {
                    int64_t dest = reflect->GetInt64(*msg, fd);
                    std::stringstream out;
                    out << dest;
                    value = out.str();
                }
                    break;
                case protobuf::FieldDescriptor::CPPTYPE_UINT32:
                {
                    uint32_t dest = reflect->GetUInt32(*msg, fd);
                    std::stringstream out;
                    out << dest;
                    value = out.str();
                }
                    break;
                case protobuf::FieldDescriptor::CPPTYPE_UINT64 :
                {
                    uint64_t dest = reflect->GetUInt64(*msg, fd);
                    std::stringstream out;
                    out << dest;
                    value = out.str();
                }
                    break;
                case protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                {
                    double dest = reflect->GetDouble(*msg, fd);
                    std::stringstream out;
                    out << dest;
                    value = out.str();
                }
                    break;
                case protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                {
                    float dest = reflect->GetFloat(*msg, fd);
                    std::stringstream out;
                    out << dest;
                    value = out.str();
                }
                    break;
                case protobuf::FieldDescriptor::CPPTYPE_STRING:
                {
                    value = reflect->GetString(*msg, fd);
                }
                    break;
                default:
                {
                    // unexpected or unsupported type; just return empty string
                }
                    break;
                }
            }
        }
    }

    return value;
}

void PTPAMessageTextParameters::setDefaultMessage(const std::string & defaultMessage)
{
    defaultMessage_ = defaultMessage;
}

void PTPAMessageTextParameters::setProtobuf(const google::protobuf::Message *msg)
{
    msg_ = msg;
}
