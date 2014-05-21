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

#include "OutputFileTextSerializer.h"

OutputFileTextSerializer::OutputFileTextSerializer(MalTrinityErrorStack & errorStack, const string & messagesFileName)
: TextSerializer(errorStack), messagesFileName_(messagesFileName)
{
}

OutputFileTextSerializer::~OutputFileTextSerializer()
{
    // Note: there is no need to call closeMessagesFile() in this destructor.
    // The fstream destructor will automatically close the file cleanly,
    // when the fstream object falls out of scope.
}

bool OutputFileTextSerializer::openMessagesFile(bool truncateMessagesFile)
{
    // clear the error state flags
    messagesFile_.clear();

    if(truncateMessagesFile)
    {
        messagesFile_.open(messagesFileName_.c_str(), ofstream::out | ofstream::trunc);
    }
    else
    {
        messagesFile_.open(messagesFileName_.c_str(), ofstream::out | ofstream::app);
    }

    // Check if the file open operation encountered an error.
    // By default fstream operations do not throw exceptions on error.
    // Unfortunately, we cannot realiably use strerror() with fstream
    // to obtain a human-readable error message.
    if(messagesFile_.fail())
    {
        errorStack_.addError(PS_FILE_OPEN_ERROR, SQ_LOG_ERR,
                             messagesFileName_, unc::events::kFileNameFieldNumber);
        return false;
    }

    return true;
}

bool OutputFileTextSerializer::closeMessagesFile()
{
    // clear the error state flags
    messagesFile_.clear();

    messagesFile_.close();

    // Check if the file close operation encountered an error.
    // By default fstream operations do not throw exceptions on error.
    // Unfortunately, we cannot realiably use strerror() with fstream
    // to obtain a human-readable error message.
    if(messagesFile_.fail())
    {
        errorStack_.addError(PS_FILE_CLOSE_ERROR, SQ_LOG_ERR,
                             messagesFileName_, unc::events::kFileNameFieldNumber);
        return false;
    }

    return true;
}

bool OutputFileTextSerializer::writeNextMessage(const AMQPRoutingKey & routingKey, const google::protobuf::Message & message, const string & comments)
{
    // Cast away the const qualifier on the routingKey parameter, because the GetAsString() method will
    // only work on a non-const AMQPRoutingKey object (otherwise we get a compiler error), but we want
    // the parameter of the writeNextMessage method to be const. We are certain that the writeNextMessage
    // method will not modify the routingKey variable!
    AMQPRoutingKey & nonConstRoutingKey = const_cast<AMQPRoutingKey &>(routingKey);

    // generate text representation of AMQPRoutingKey object
    string routingKeyString = nonConstRoutingKey.GetAsString();

    // generate text representation of protobuf message object
    string messageTextString;
    bool printToStringRC = textFormatPrinter_.PrintToString(message, &messageTextString);
    if(!printToStringRC)
    {
        // Unfortunately, no error information is returned from
        // TextFormat::Printer::PrintToString().
        errorStack_.addError(PS_TEXT_FORMAT_PRINT_ERROR, SQ_LOG_ERR);
        return false;
    }

    // create the formatted text for the next message entry
    string nextMessageEntry = "";

    // comment block
    if(comments.length() > 0)
    {
        nextMessageEntry += COMMENT_BLOCK_START_STRING;
        nextMessageEntry += NEWLINE_STRING;
        nextMessageEntry += comments;
        nextMessageEntry += NEWLINE_STRING;
        nextMessageEntry += COMMENT_BLOCK_END_STRING;
        nextMessageEntry += NEWLINE_STRING;
    }

    // routing key block
    nextMessageEntry += ROUTING_KEY_BLOCK_START_STRING;
    nextMessageEntry += NEWLINE_STRING;
    nextMessageEntry += routingKeyString;
    nextMessageEntry += NEWLINE_STRING;
    nextMessageEntry += ROUTING_KEY_BLOCK_END_STRING;
    nextMessageEntry += NEWLINE_STRING;

    // message block
    nextMessageEntry += MESSAGE_BLOCK_START_STRING;
    nextMessageEntry += NEWLINE_STRING;
    nextMessageEntry += messageTextString; // already contains a newline at the end
    nextMessageEntry += MESSAGE_BLOCK_END_STRING;
    nextMessageEntry += NEWLINE_STRING;

    // clear the error state flags
    messagesFile_.clear();

    // separate message entries with newlines (for human readability)
    // endl also flushes the stream buffer
    messagesFile_ << nextMessageEntry << endl;

    // Check if the file write operation encountered an error.
    // By default fstream operations do not throw exceptions on error.
    // Unfortunately, we cannot realiably use strerror() with fstream
    // to obtain a human-readable error message.
    if(messagesFile_.fail() || messagesFile_.bad())
    {
        errorStack_.addError(PS_FILE_WRITE_ERROR, SQ_LOG_ERR,
                             messagesFileName_, unc::events::kFileNameFieldNumber);
        return false;
    }

    return true;
}
