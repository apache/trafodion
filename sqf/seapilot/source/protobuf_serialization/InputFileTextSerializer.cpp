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

#include "InputFileTextSerializer.h"

#include <ctype.h>  // to get isspace()

InputFileTextSerializer::InputFileTextSerializer(MalTrinityErrorStack & errorStack, google::protobuf::compiler::Importer & importer, const string & messagesFileName)
: TextSerializer(errorStack), importer_(importer), messageFactory_(importer.pool()), messagesFileName_(messagesFileName), protobufSerializationTextFormatParserErrorCollector_(errorStack)
{
    textFormatParser_.RecordErrorsTo(&protobufSerializationTextFormatParserErrorCollector_);
}

InputFileTextSerializer::~InputFileTextSerializer()
{
    // Note: there is no need to call closeMessagesFile() in this destructor.
    // The fstream destructor will automatically close the file cleanly,
    // when the fstream object falls out of scope.
}

bool InputFileTextSerializer::openMessagesFile(const streampos & startPosition)
{
    // clear the error state flags
    messagesFile_.clear();

    messagesFile_.open(messagesFileName_.c_str(), ifstream::in);

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

    // clear the error state flags
    messagesFile_.clear();

    messagesFile_.seekg(startPosition);

    // Check if the file seek operation encountered an error.
    // By default fstream operations do not throw exceptions on error.
    // Unfortunately, we cannot realiably use strerror() with fstream
    // to obtain a human-readable error message.
    if(messagesFile_.fail() || messagesFile_.bad())
    {
        errorStack_.addError(PS_FILE_SEEK_ERROR, SQ_LOG_ERR,
                             messagesFileName_, unc::events::kFileNameFieldNumber);
        return false;
    }

    return true;
}

bool InputFileTextSerializer::closeMessagesFile()
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

bool InputFileTextSerializer::readNextMessage(bool & reachedEOF, streampos & currentPosition, AMQPRoutingKey * & routingKey, google::protobuf::Message * & message)
{
    // initialize output pointers; this forces callers to properly handle errors!
    routingKey = NULL;
    message = NULL;

    string routingKeyText;
    string messageText;

    bool readNextMessageTextRC = readNextMessageText(reachedEOF, currentPosition, routingKeyText, messageText);

    if(readNextMessageTextRC == false)
    {
        return readNextMessageTextRC;
    }
    else if(routingKeyText.length() == 0 || messageText.length() == 0)
    {
        if (reachedEOF)
        {
            return true;  // end of file is not an error
        }

        errorStack_.addError(PS_SYNTAX_PARSE_ERROR, SQ_LOG_ERR,
                             messagesFileName_, unc::events::kFileNameFieldNumber);
        return false;
    }

    // create a new routing key object
    routingKey = new AMQPRoutingKey(routingKeyText);
    if(routingKey == NULL)
    {
        errorStack_.addError(PS_OBJECT_CREATION_FAILURE, SQ_LOG_ERR,
                             "AMQPRoutingKey", unc::events::kErrorStringFieldNumber);
        return false;
    }

    string messageName = routingKey->GetAsMessageName();
    string protoFileName = routingKey->GetAsProtofileName();

    // load the protobuf definition
    const google::protobuf::FileDescriptor * fileDescriptor = importer_.Import(protoFileName);
    if(fileDescriptor == NULL)
    {
        // Import errors will be collected by the caller's google::protobuf::compiler::MultiFileErrorCollector
        // adaptor object, since we don't own the google::protobuf::compiler::Importer in this class!
        // We don't generate a duplicate error message here. Errors are reported on the first import
        // attempt of a particular proto file. In other words, all future attempts to
        // import the same file will return NULL without reporting any errors.

        // release memory before returning!
        delete routingKey;
        routingKey = NULL;

        return false;
    }

    // Obtain the message descriptor using the fully-qualified message name.
    // This call will find both top-level and nested descriptors.
    // NULL is returned if the message descriptor cannot be found.
    const google::protobuf::Descriptor * messageDescriptor = importer_.pool()->FindMessageTypeByName(messageName);
    if(messageDescriptor == NULL)
    {
        errorStack_.addError(PS_PROTO_MESSAGE_DESCRIPTOR_FIND_ERROR, SQ_LOG_ERR,
                             messageName, unc::events::kErrorStringFieldNumber);

        // release memory before returning!
        delete routingKey;
        routingKey = NULL;

        return false;
    }

    // Create a new protobuf message object using the message prototype.
    // The New() method uses the C++ new operator under the covers,
    // to create a new object of the given message type. Hence, there should be no
    // issue deleteting the protobuf message object using the C++ delete operator.
    message = messageFactory_.GetPrototype(messageDescriptor)->New();
    if(message == NULL)
    {
        errorStack_.addError(PS_OBJECT_CREATION_FAILURE, SQ_LOG_ERR,
                             messageName, unc::events::kErrorStringFieldNumber);

        // release memory before returning!
        delete routingKey;
        routingKey = NULL;

        return false;
    }

    // parse text representation of protobuf message object
    bool parseFromStringRC = textFormatParser_.ParseFromString(messageText, message);
    if(parseFromStringRC == false)
    {
        // Parser errors and warnings will be placed on the errorStack_
        // via the protobufSerializationTextFormatParserErrorCollector_.

        // release memory before returning!
        delete routingKey;
        routingKey = NULL;
        delete message;
        message = NULL;

        return false;
    }

    return true;
}

bool InputFileTextSerializer::readNextMessageText(bool & reachedEOF, streampos & currentPosition, string & routingKeyText, string & messageText)
{
    bool inCommentBlock = false;
    bool inRoutingKeyBlock = false;
    bool inMessageBlock = false;

    string lineText;

    // clear the error state flags
    messagesFile_.clear();

    // loop while (EOF or I/O Error is not encountered)
    while(getline(messagesFile_,lineText))
    {
        // remove any trailing white space (in particular, if it is
        // an MS-DOS file, there might be an annoying trailing \r
        // character)
        if (lineText.size() > 0)
        {
            size_t size = lineText.size();
            size_t position = size-1;
            size_t whiteSpaceCount = 0;
            const char * lineTextStr = lineText.c_str();
            while ((whiteSpaceCount < size) && 
                   (isspace(lineTextStr[position])))
            {
                position--;
                whiteSpaceCount++;
            }
            if (whiteSpaceCount > 0)
            {
                position++; // because we backed up before last white space
                lineText.erase(position,whiteSpaceCount);
            }
        } 

        // *** comment block processing *** ///
        if(lineText == COMMENT_BLOCK_START_STRING) // start of comment block
        {
            inCommentBlock = true;
            continue; // while loop
        }
        else if(lineText == COMMENT_BLOCK_END_STRING) // end of comment block
        {
            inCommentBlock = false;
            continue; // while loop
        }
        else if(inCommentBlock) // text of comment block
        {
            // we ignore comments
            continue; // while loop
        }
        // *** routing key block processing *** ///
        else if(lineText == ROUTING_KEY_BLOCK_START_STRING) // start of routing key block
        {
            inRoutingKeyBlock = true;
            routingKeyText = "";
            continue; // while loop
        }
        else if(lineText == ROUTING_KEY_BLOCK_END_STRING) // end of routing key block
        {
            inRoutingKeyBlock = false;
            continue; // while loop
        }
        else if(inRoutingKeyBlock) // text of routing key block
        {
            routingKeyText += lineText;
            continue; // while loop
        }
        // *** message block processing *** ///
        else if(lineText == MESSAGE_BLOCK_START_STRING) // start of message block
        {
            inMessageBlock = true;
            messageText = "";
            continue; // while loop
        }
        else if(lineText == MESSAGE_BLOCK_END_STRING) // end of message block
        {
            inMessageBlock = false;
            // when we reach the end of the message block,
            // we have read one message entry in the file
            break; // while loop
        }
        else if(inMessageBlock) // text of message block
        {
            messageText += lineText;
            // need to add a space to separate lines of the message text string
            // because the getline function discards the newline delimiter character
            // after reading each line of the messages file
            messageText += " ";
            continue; // while loop
        }
    }

    reachedEOF = messagesFile_.eof();

    // Check if the getline read operation encountered an error.
    // Note that the fail bit will always be set when the eof bit
    // is set, so we don't report an error in this case.
    // By default fstream operations do not throw exceptions on error.
    // Unfortunately, we cannot realiably use strerror() with fstream
    // to obtain a human-readable error message.
    if((!reachedEOF && (messagesFile_.fail() || messagesFile_.bad()))
       || (reachedEOF && messagesFile_.bad()))
    {
        errorStack_.addError(PS_FILE_READ_ERROR, SQ_LOG_ERR,
                             messagesFileName_, unc::events::kFileNameFieldNumber);
        return false;
    }

    // Obtain the absolute position of the get pointer.
    // The get pointer determines the next location in the
    // input sequence to be read by the next input operation.
    currentPosition = messagesFile_.tellg();

    // tellg() indicates failure by returning a value of -1.
    // tellg() will always return -1 if EOF has been reached,
    // so we don't report an error in this case.
    if(!reachedEOF && currentPosition == -1)
    {
        errorStack_.addError(PS_FILE_GET_POSITION_ERROR, SQ_LOG_ERR,
                             messagesFileName_, unc::events::kFileNameFieldNumber);
        return false;
    }

    return true;
}

