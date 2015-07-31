//------------------------------------------------------------------
//
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

#include <string.h>

#include "seabed/excep.h"


SB_Excep::SB_Excep(const char *pp_msg) {
    if (pp_msg != NULL) {
        ip_msg = new char[strlen(pp_msg)+1];
        strcpy(ip_msg, pp_msg);
    } else
        ip_msg = NULL;
}

SB_Excep::SB_Excep(const SB_Excep &pr_excep)
: std::exception(pr_excep) {
    if (pr_excep.ip_msg != NULL) {
        ip_msg = new char[strlen(pr_excep.ip_msg)+1];
        strcpy(ip_msg, pr_excep.ip_msg);
    } else
        ip_msg = NULL;
}

SB_Excep &SB_Excep::operator =(const SB_Excep &pr_excep) {
    if (this != &pr_excep) {
        if (ip_msg != NULL)
            delete [] ip_msg;
        ip_msg = new char[strlen(pr_excep.ip_msg)+1];
        strcpy(ip_msg, pr_excep.ip_msg);
    }
    return *this;
}

SB_Excep::~SB_Excep() throw () {
    if (ip_msg != NULL)
        delete [] ip_msg;
}

const char *SB_Excep::what() const throw () {
    return ip_msg;
}

SB_Fatal_Excep::SB_Fatal_Excep(const char *pp_msg)
: SB_Excep(pp_msg) {
}

SB_Fatal_Excep::~SB_Fatal_Excep() throw () {
}

