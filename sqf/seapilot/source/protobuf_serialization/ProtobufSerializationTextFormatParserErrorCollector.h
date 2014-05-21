// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef PROTOBUF_SERIALIZATION_TEXT_FORMAT_PARSER_ERROR_COLLECTOR_H
#define PROTOBUF_SERIALIZATION_TEXT_FORMAT_PARSER_ERROR_COLLECTOR_H

#include <sstream>

#include <google/protobuf/io/tokenizer.h>

// for error reporting
#include "MalTrinityErrorStack.h"

// to get token types for errors
#include "unc.events.pb.h"

using namespace std;
using namespace Trinity;

//! This is an adaptor class to record parse warnings and errors encountered by google::protobuf::TextFormat::Parser.
class ProtobufSerializationTextFormatParserErrorCollector : public google::protobuf::io::ErrorCollector
{
    public:
        ProtobufSerializationTextFormatParserErrorCollector(Trinity::MalTrinityErrorStack & errorStack);
        virtual ~ProtobufSerializationTextFormatParserErrorCollector();

        //! This method gets called when a parse warning is encountered, at the given line and column numbers.
        virtual void AddWarning(int line, int column, const string & message);

        //! This method gets called when a parse error is encountered, at the given line and column numbers.
        virtual void AddError(int line, int column, const string & message);

    private:

        MalTrinityErrorStack & errorStack_;
};

#endif // PROTOBUF_SERIALIZATION_TEXT_FORMAT_PARSER_ERROR_COLLECTOR_H
