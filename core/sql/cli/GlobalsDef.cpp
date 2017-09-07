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

#include "Platform.h"


#define CLI_GLOBALS_DEF_

#include <stdlib.h>
#include "cli_stdh.h"
//#include "Ipc.h"
//#include "ex_stdh.h"
//#include "ex_frag_rt.h"

// This DLL exports the global variables used in executor.
// Since executor libraries are packaged in 2 ways ( user )
// . tdm_sqlcli.dll, this is for application programmer
// . cli, executor, exp, common, ..etc. static linked libs
//   for internal components
// For a program both statically and dynamically linked in these
// libraries gets more than one set of global variables which causes
// problems. So the globals used in executor are extracted into this
// DLL and linked in to tdm_sqlcli.dll , also any other programs needs
// to link cli.lib

__declspec(dllexport) CliGlobals * cli_globals = 0;
THREAD_P jmp_buf ExportJmpBuf;
