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

#include "ProtobufSerializationTextFormatParserErrorCollector.h"

ProtobufSerializationTextFormatParserErrorCollector::ProtobufSerializationTextFormatParserErrorCollector(Trinity::MalTrinityErrorStack & errorStack) : errorStack_(errorStack)
{
}

ProtobufSerializationTextFormatParserErrorCollector::~ProtobufSerializationTextFormatParserErrorCollector()
{
}

void ProtobufSerializationTextFormatParserErrorCollector::AddWarning(int line, int column, const string & message)
{
    // The given line and column numbers are zero-based,
    // so we add 1 to each before reporting them.
    stringstream oneBasedLineNumberStringStream;
    oneBasedLineNumberStringStream << line + 1;
    stringstream oneBasedColumnNumberStringStream;
    oneBasedColumnNumberStringStream << column + 1;

    errorStack_.addError(PS_TEXT_FORMAT_PARSE_WARNING, SQ_LOG_WARNING,
                         message, unc::events::kErrorStringFieldNumber,
                         oneBasedLineNumberStringStream.str(), unc::events::kFileLineNumberFieldNumber,
                         oneBasedColumnNumberStringStream.str(), unc::events::kFileColumnNumberFieldNumber);
}

void ProtobufSerializationTextFormatParserErrorCollector::AddError(int line, int column, const string & message)
{
    // The given line and column numbers are zero-based,
    // so we add 1 to each before reporting them.
    stringstream oneBasedLineNumberStringStream;
    oneBasedLineNumberStringStream << line + 1;
    stringstream oneBasedColumnNumberStringStream;
    oneBasedColumnNumberStringStream << column + 1;

    errorStack_.addError(PS_TEXT_FORMAT_PARSE_ERROR, SQ_LOG_ERR,
                         message, unc::events::kErrorStringFieldNumber,
                         oneBasedLineNumberStringStream.str(), unc::events::kFileLineNumberFieldNumber,
                         oneBasedColumnNumberStringStream.str(), unc::events::kFileColumnNumberFieldNumber);
}
