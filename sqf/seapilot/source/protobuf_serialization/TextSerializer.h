#ifndef TEXT_SERIALIZER_H
#define TEXT_SERIALIZER_H

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

#include <string>
#include <iostream>
#include <fstream>
#include <stack>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/text_format.h>

#include "wrapper/routingkey.h"

// for error reporting
#include "MalTrinityErrorStack.h"

// to get token types for errors
#include "unc.events.pb.h"

using namespace std;
using namespace Trinity;

const char * const COMMENT_BLOCK_START_STRING     = "/*";
const char * const COMMENT_BLOCK_END_STRING       = "*/";
const char * const ROUTING_KEY_BLOCK_START_STRING = "/@";
const char * const ROUTING_KEY_BLOCK_END_STRING   = "@/";
const char * const MESSAGE_BLOCK_START_STRING     = "/=";
const char * const MESSAGE_BLOCK_END_STRING       = "=/";
const char * const NEWLINE_STRING                 = "\n";

//! This is the base class of the Protobuf Serialization Library.
//! This is an abstract class: no objects of this class can be instantiated or destroyed.
class TextSerializer
{
    protected:

        TextSerializer(MalTrinityErrorStack & errorStack);
        virtual ~TextSerializer();

        MalTrinityErrorStack & errorStack_;
};

#endif // TEXT_SERIALIZER_H
