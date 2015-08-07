//******************************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
//******************************************************************************
#ifndef INCLUDE_LD_GLOBALS_H
#define INCLUDE_LD_GLOBALS_H  1
#include "common/evl_sqlog_eventnum.h"
#include <string>

struct AuthEvents
{
DB_SECURITY_EVENTID eventID;
std::string         eventText;
std::string         filename;
int32_t             lineNumber;
};

void clearAuthEvents();

size_t getAuthEventCount();

const AuthEvents & getAuthEvent(size_t index);

void logAuthEvent(
   DB_SECURITY_EVENTID eventID,
   const char *        msg,
   const std::string & file_name, 
   int32_t             line_number); 

#endif 
