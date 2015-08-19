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

//
// Implement properties
//

#ifndef __SB_PROPS_H_
#define __SB_PROPS_H_

#include "smap.h"

class SB_Props : public SB_Smap {
public:
    SB_Props();
    SB_Props(bool pv_getenv);
    virtual ~SB_Props();

    bool load(const char *pp_file);
    bool store(const char *pp_file);

private:
    bool repl_var(char *pp_value);

    bool iv_getenv;
};

#endif // !__SB_PROPS_H_
