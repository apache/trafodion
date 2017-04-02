/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
****************************************************************************
*
* File:         Healper functions for use in bin/clitest.cpp
* Description:  Test driver functions useing exe util cli interface
*
*
*
*
****************************************************************************
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "BaseTypes.h"
#include "NAAssert.h"
#include "stdlib.h"
#include "stdio.h"
#include "sqlcli.h"
#include "ComDiags.h"
#include "ex_stdh.h"
#include "memorymonitor.h"
#include "ex_exe_stmt_globals.h"
#include "ex_esp_frag_dir.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_split_bottom.h"
#include "ex_send_bottom.h"
#include "NAExit.h"
#include "ExSqlComp.h"
#include "Globals.h"
#include "Int64.h"
#include "SqlStats.h"
#include "ComUser.h"
#include "ExpError.h"
#include "ComSqlId.h"
#include "ex_globals.h"
#include "ex_tcb.h"
#include "ExExeUtil.h"
#include "Globals.h"
#include "Context.h"
#include <sys/stat.h>
#include <stdlib.h>

Int32 extractLobHandle(CliGlobals *cliglob, char *& lobHandle, 
		       char *lobColumnName, char *tableName);

Int32 extractLengthOfLobColumn(CliGlobals *cliglob, char * lobHandle, Int64 &lengthOfLob,char *lobColumnName, char *tableName);

Int32 extractLobToBuffer(CliGlobals *cliglob, char * lobHandle, Int64 &lengthOfLob, 
			 char *lobColumnName, char *tableName);
Int32 extractLobToFileInChunks(CliGlobals *cliglob, char * lobHandle, char *filename, Int64 &lengthOfLob, 
			 char *lobColumnName, char *tableName);
Int32 insertBufferToLob(CliGlobals *cliglob,char *tbaleName);
Int32 updateBufferToLob(CliGlobals *cliglob, char *tableName, char *columnName);
Int32 updateAppendBufferToLob(CliGlobals *cliGlob, char *tableName, char *columnName);
Int32 updateBufferToLobHandle(CliGlobals *cliglob,  char *handle);
Int32 updateTruncateLobHandle(CliGlobals *cliglob,  char *handle);
Int32 updateAppendBufferToLobHandle(CliGlobals *cliglob,  char *handle,Int64 updateLen, Int64 sourceAddress);
