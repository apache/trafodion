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
* File:         clitest.cpp
* Description:  Test driver useing exe util cli interface
*
*
*
*
****************************************************************************
*/
#include "Platform.h"


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
#include "blobtest.h"

// DEFINE_DOVERS(clitest)

int main()
{
  SQLCTX_HANDLE defContext = 0;
  Lng32 retCode = SQL_EXEC_CreateContext(&defContext, NULL, 0);

  if(retCode == 0){
    cerr << "success -- new handle:" << defContext << endl;
  }
  else{
    cerr << "error -- " << endl;
   
  }
  CliGlobals * cliGlob = GetCliGlobals();
  //extract length of blob column from a table with 1 lob column.
  Int64 lengthOfLob= 0;
  retCode = extractLengthOfBlobColumn(cliGlob, lengthOfLob,(char *)"c2",(char *)"tlob1");
 
  
  
  return 0;
  
}


