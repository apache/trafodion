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
 *****************************************************************************
 *
 * File:         TestDriver.cpp
 * Description:  Driver for testing the new CLI calls.
 *               
 *               
 * Created:      7/18/00
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

// #include "cli_stdh.h"
// #include "Cli.h"

#include "stdio.h"

#include "stdlib.h"

#include "time.h"

#include "sqlcli.h"

#include "cli_stdh.h"

static Int32 contextTest();

Int32 main()
{
	// Create a Procedure

	// Prepare a CALL statement

	// Describe Input

	// Describe Output

	// Execute the CALL statement prepared

	return 0;
}

Int32 contextTest(){

#define NUM_OF_CTX 8
	`
#define NUM_OF_ITER 16

  SQLCTX_HANDLE ctxVec[NUM_OF_CTX];

  char idVec[NUM_OF_CTX][32];

  for(Int32 i=0; i< NUM_OF_CTX; i++) {

    char *authId = &idVec[i][0];

    itoa(i%4 + 1000, authId, 10);	 

    Int32 code = SQL_EXEC_CreateContext(&ctxVec[i], authId);
    
    if(code != SUCCESS){

      cout << "Error in Creating Context:" << i;

      exit(-1);
    }
  }

  for(i=0; i< NUM_OF_ITER; i++){
    
    srand( (UInt32)time( NULL ) );
      
    Int32 hdNum = rand() % NUM_OF_CTX;
    
    SQLCTX_HANDLE currHandle = 0;

    SQLCTX_HANDLE prevHandle = 0;
    
    Int32 code = SQL_EXEC_CurrentContext(&currHandle);
    
    if(code != SUCCESS){
      
      cout << "Error in Obtaining Current Context";
      
      exit(-1);
    }

    code = SQL_EXEC_SwitchContext(ctxVec[hdNum], &prevHandle);
    
    if(code != SUCCESS){
	
      cout << "Error in Switching to Context:" << ctxVec[hdNum];
    }

    if(prevHandle != currHandle){

      cout << "Error in ContextManagement, Inconsistency";
      
      exit(-1);
    }
  }

  for(i=0; i< NUM_OF_ITER; i++){
    
    srand( (UInt32)time( NULL ) );
        
    Int32 hdNum = rand() % NUM_OF_CTX;

    Int32 code = SQL_EXEC_DeleteContext(ctxVec[hdNum]);
    
    if(code != SUCCESS){
      
      cout << "Error in Deleting Context:" << ctxVec[hdNum];

      exit(-1);     
    }
  }
  return 0;
}
