/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineCSql.cpp
* Description:  
* Created:      08/02/2009
* Language:     C++
*
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

#include "LmRoutineCSql.h"
#include "LmParameter.h"
#include "sqludr.h"
#include "ComDefs.h"
#include "wstr.h"

// Routine body signatures
typedef Lng32 (*UDRFN1) (char*,
                        short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN2) (char*,char*,
                        short*,short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN3) (char*,char*,char*,
                        short*,short*,short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN4) (char*,char*,char*,char*,
                        short*,short*,short*,short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN5) (char*,char*,char*,char*,char*,
                        short*,short*,short*,short*,short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN6) (char*,char*,char*,char*,char*,char*,
                        short*,short*,short*,short*,short*,short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN7) (char*,char*,char*,char*,char*,char*,char*,
                        short*,short*,short*,short*,short*,short*,short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN8) (char*,char*,char*,char*,char*,char*,char*,char*,
                        short*,short*,short*,short*,short*,short*,short*,
                        short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN9) (char*,char*,char*,char*,char*,char*,char*,char*,
		        char*,
                        short*,short*,short*,short*,short*,short*,short*,
                        short*,short*,
                        SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN10) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN11) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN12) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN13) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN14) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN15) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN16) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN17) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN18) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN19) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN20) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN21) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN22) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN23) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN24) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN25) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN26) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN27) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN28) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN29) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN30) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN31) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);
typedef Lng32 (*UDRFN32) (char*,char*,char*,char*,char*,char*,char*,char*,
		         char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
			 char*,char*,char*,char*,char*,char*,char*,char*,
                         short*,short*,short*,short*,short*,short*,short*,
                         short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,short*,short*,short*,
			 short*,short*,short*,short*,
                         SQLUDR_TRAIL_ARGS);

// SQLUDR_INVOKE is a global function that acts as the gateway into
// the routine body. It only contains a switch construct and calls
// appropriate function pointer depending on the argc parameter.
ComSInt32 SQLUDR_INVOKE(void *fp,
                        SQLUDR_UINT32 argc,
                        char **data,
                        short *ind,
                        SQLUDR_CHAR sqlState[SQLUDR_SQLSTATE_SIZE],
                        SQLUDR_CHAR msgText[SQLUDR_MSGTEXT_SIZE],
                        SQLUDR_INT32 callType,
                        SQLUDR_STATEAREA *stateArea,
                        SQLUDR_UDRINFO *udrInfo)
{
  ComSInt32 rc = 0;

  switch (argc)
  {
    case 1:
      rc = ((UDRFN1) fp) (data[0],
                          &ind[0],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 2:
      rc = ((UDRFN2) fp) (data[0], data[1],
                          &ind[0], &ind[1],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 3:
      rc = ((UDRFN3) fp) (data[0], data[1], data[2],
                          &ind[0], &ind[1], &ind[2],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 4:
      rc = ((UDRFN4) fp) (data[0], data[1], data[2], data[3],
                          &ind[0], &ind[1], &ind[2], &ind[3],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 5:
      rc = ((UDRFN5) fp) (data[0], data[1], data[2], data[3],
                          data[4],
                          &ind[0], &ind[1], &ind[2], &ind[3], 
                          &ind[4],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 6:
      rc = ((UDRFN6) fp) (data[0], data[1], data[2], data[3],
                          data[4], data[5],
                          &ind[0], &ind[1], &ind[2], &ind[3], 
                          &ind[4], &ind[5],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 7:
      rc = ((UDRFN7) fp) (data[0], data[1], data[2], data[3],
                          data[4], data[5], data[6],
                          &ind[0], &ind[1], &ind[2], &ind[3], 
                          &ind[4], &ind[5], &ind[6],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 8:
      rc = ((UDRFN8) fp) (data[0], data[1], data[2], data[3],
                          data[4], data[5], data[6], data[7],
                          &ind[0], &ind[1], &ind[2], &ind[3], 
                          &ind[4], &ind[5], &ind[6], &ind[7],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 9:
      rc = ((UDRFN9) fp) (data[0], data[1], data[2], data[3],
                          data[4], data[5], data[6], data[7],
			  data[8],
                          &ind[0], &ind[1], &ind[2], &ind[3], 
                          &ind[4], &ind[5], &ind[6], &ind[7],
			  &ind[8],
                          sqlState, msgText, callType,
                          stateArea, udrInfo);
      break;
    case 10:
      rc = ((UDRFN10) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
			   data[8], data[9],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
			   &ind[8], &ind[9],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 11:
      rc = ((UDRFN11) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
			   data[8], data[9], data[10],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
			   &ind[8], &ind[9], &ind[10],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 12:
      rc = ((UDRFN12) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 13:
      rc = ((UDRFN13) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 14:
      rc = ((UDRFN14) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 15:
      rc = ((UDRFN15) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 16:
      rc = ((UDRFN16) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 17:
      rc = ((UDRFN17) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 18:
      rc = ((UDRFN18) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 19:
      rc = ((UDRFN19) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 20:
      rc = ((UDRFN20) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 21:
      rc = ((UDRFN21) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 22:
      rc = ((UDRFN22) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 23:
      rc = ((UDRFN23) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 24:
      rc = ((UDRFN24) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 25:
      rc = ((UDRFN25) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
			   data[24],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
			   &ind[24],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 26:
      rc = ((UDRFN26) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
			   data[24], data[25],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
			   &ind[24], &ind[25],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 27:
      rc = ((UDRFN27) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
			   data[24], data[25], data[26],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
			   &ind[24], &ind[25], &ind[26],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 28:
      rc = ((UDRFN28) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
			   data[24], data[25], data[26], data[27],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
			   &ind[24], &ind[25], &ind[26], &ind[27],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 29:
      rc = ((UDRFN29) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
			   data[24], data[25], data[26], data[27],
			   data[28],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
			   &ind[24], &ind[25], &ind[26], &ind[27],
			   &ind[28],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 30:
      rc = ((UDRFN30) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
			   data[24], data[25], data[26], data[27],
			   data[28], data[29],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
			   &ind[24], &ind[25], &ind[26], &ind[27],
			   &ind[28], &ind[29],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 31:
      rc = ((UDRFN31) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
			   data[24], data[25], data[26], data[27],
			   data[28], data[29], data[30],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
			   &ind[24], &ind[25], &ind[26], &ind[27],
			   &ind[28], &ind[29], &ind[30],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    case 32:
      rc = ((UDRFN32) fp) (data[0], data[1], data[2], data[3],
                           data[4], data[5], data[6], data[7],
  			   data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15],
			   data[16], data[17], data[18], data[19],
			   data[20], data[21], data[22], data[23],
			   data[24], data[25], data[26], data[27],
			   data[28], data[29], data[30], data[31],
                           &ind[0], &ind[1], &ind[2], &ind[3], 
                           &ind[4], &ind[5], &ind[6], &ind[7],
  			   &ind[8], &ind[9], &ind[10], &ind[11],
			   &ind[12], &ind[13], &ind[14], &ind[15],
			   &ind[16], &ind[17], &ind[18], &ind[19],
			   &ind[20], &ind[21], &ind[22], &ind[23],
			   &ind[24], &ind[25], &ind[26], &ind[27],
			   &ind[28], &ind[29], &ind[30], &ind[31],
                           sqlState, msgText, callType,
                           stateArea, udrInfo);
      break;
    default:
      LM_ASSERT1(0, "Parameters more than 32 are not allowed.");
      break;
  }

  return rc;
  
} // SQLUDR_INVOKE

LmRoutineCSql::LmRoutineCSql(const char   *sqlName,
                             const char   *externalName,
                             const char   *librarySqlName,
                             ComUInt32    numSqlParam,
                             char         *routineSig,
                             ComUInt32    maxResultSets,
                             ComRoutineTransactionAttributes transactionAttrs,
                             ComRoutineSQLAccess sqlAccessMode,
                             ComRoutineExternalSecurity externalSecurity,
                             Int32 routineOwnerId,
                             const char   *parentQid,
                             ComUInt32    inputRowLen,
                             ComUInt32    outputRowLen,
                             const char   *currentUserName,
                             const char   *sessionUserName,
                             LmParameter  *parameters,
                             LmLanguageManagerC *lm,
                             LmHandle     routine,
                             LmContainer  *container,
                             ComDiagsArea *diagsArea)
  : LmRoutineC(sqlName, externalName, librarySqlName, numSqlParam, routineSig,
               maxResultSets,
               COM_LANGUAGE_C,
               COM_STYLE_SQL,
               transactionAttrs,
               sqlAccessMode,
               externalSecurity, 
	       routineOwnerId,
               parentQid, inputRowLen, outputRowLen,
               currentUserName, sessionUserName, 
               parameters, lm, routine, container, diagsArea),
    cBuf_(NULL),
    data_(NULL),
    ind_(numSqlParam * sizeof(short))
{
  ComUInt32 i = 0;
  data_ = (char **) collHeap()->allocateMemory(numSqlParam * sizeof(char *));
  
  // Allocate C data buffers. Each LmCBuffer instance points to a C
  // buffer and the data_ buffer is an array of pointers to the C data
  // buffers. The LmCBuffer is mainly used to track the actual size of
  // the C buffers because each buffer has some extra bytes at the end
  // to protect against buffer overwrites.
  //
  // cBuf_ -> LmCBuffer  LmCBuffer  LmCBuffer ...
  //            |          |          |
  //            v          v          v
  //           buffer     buffer     buffer   ...
  //             ^          ^          ^
  //             |          |          |
  //          data_[0]   data_[1]   data_[2]  ...
  //

  // NOTE: the cBuf_ array is allocated on the C++ heap because we
  // want to manage the collection as a single array, and we want
  // constructors and destructors to be called when the collection is
  // created and destroyed. Right now, NAMemory and NABasic object
  // interfaces do not provide the appropriate array versions of new
  // and delete operators to accomplish these things.
  cBuf_ = new LmCBuffer[numSqlParam_];
  LM_ASSERT(cBuf_);

  for (i = 0; i < numSqlParam_; i++)
  {
    LmParameter &p = lmParams_[i];
    LmCBuffer &cBuf = cBuf_[i];
    ComUInt32 dataBytes = 0;

    switch (p.direction())
    {
      // NOTE: The code currently supports IN and OUT parameters for C
      // routines. There is no reason we couldn't support INOUT as
      // well, which will be needed if we ever provide stored
      // procedures written in C. But the INOUT code paths have not
      // been implemented yet.
      case COM_INPUT_COLUMN:
        dataBytes = p.inSize();
        break;
      case COM_OUTPUT_COLUMN:
        dataBytes = p.outSize();
        break;
      default:
        LM_ASSERT(0);
        break;
    }

    switch (p.fsType())
    {
      case COM_VCHAR_FSDT:
      case COM_VCHAR_DBL_FSDT:
      {
        // VARCHAR(N) CHARACTER SET ISO88591
        // VARCHAR(N) CHARACTER SET UCS2

        // This is a VARCHAR parameter. Allocate one buffer that will
        // hold the VC struct and the data. Data will begin at the first
        // 8-byte boundary following the VC struct.
        ComUInt32 vcBytes = ROUND8(sizeof(SQLUDR_VC_STRUCT)) + dataBytes;
        cBuf.init(vcBytes);
        data_[i] = cBuf.getBuffer();
        
        // Initialize the VC struct
        SQLUDR_VC_STRUCT *vc = (SQLUDR_VC_STRUCT *) cBuf.getBuffer();
        char *charPtr = (char *) vc;
        vc->data = charPtr + ROUND8(sizeof(SQLUDR_VC_STRUCT));
        vc->length = dataBytes;
      }
      break;

      case COM_FCHAR_FSDT:
      case COM_FCHAR_DBL_FSDT:
      case COM_SIGNED_DECIMAL_FSDT:
      case COM_UNSIGNED_DECIMAL_FSDT:
      case COM_DATETIME_FSDT:
      case COM_SIGNED_NUM_BIG_FSDT:
      case COM_UNSIGNED_NUM_BIG_FSDT:
      {
        // CHAR(N) CHARACTER SET ISO88591
        // CHAR(N) CHARACTER SET UCS2
        // DECIMAL [UNSIGNED]
        // DATE, TIME, TIMESTAMP
        // NUMERIC precision > 18

        // These types require a null-terminated C string. Add one to
        // dataBytes to account for the null terminator.
        cBuf.init(dataBytes + 1);
        data_[i] = cBuf.getBuffer();
      }
      break;

      default:
      {
        // All other types
        cBuf.init(dataBytes);
        data_[i] = cBuf.getBuffer();
      }
      break;

    } // switch (p.fsType())
  } // for each param
    
} // LmRoutineCSql::LmRoutineCSql

LmRoutineCSql::~LmRoutineCSql()
{
  // Free each C buffer then free the memory for the LmCBuffer
  // instances
  for (ComUInt32 i = 0; i < numSqlParam_; i++)
  {
    LmCBuffer &cBuf = cBuf_[i];
    cBuf.release();
  }
  delete [] cBuf_;
}

LmResult LmRoutineCSql::invokeRoutine(void *inputRow,
				      void *outputRow,
                                      ComDiagsArea *da)
{
  ComUInt32 argc = numSqlParam_;
  ComUInt32 i = 0;
  short *udrInd = (short *) ind_.getBuffer();

  // Code handles UDF functions with upto 32 parameters.
  LM_ASSERT1(argc <= 32, "Parameters more than 32 are not allowed.");

  // We can return early if the caller is requesting a FINAL call but
  // not FINAL is necessary because the INITIAL call was never made
  if (callType_ == SQLUDR_CALLTYPE_FINAL && !finalCallRequired_)
    return LM_OK;

  ComUInt32 numIn = 0;
  ComUInt32 numOut = 0;

  // Build the argument vector
  if (callType_ != SQLUDR_CALLTYPE_FINAL)
  {
    for (i = 0; i < argc; i++)
    {
      LmParameter &p = lmParams_[i];
      LM_ASSERT1(p.direction() != COM_INOUT_COLUMN,
	         "INOUT parameters are not supported for C routines");

      if (p.isIn())
      {
        numIn++;

        // Set the input null indicator
	NABoolean nullVal = p.isNullInput((char *) inputRow);
        udrInd[i] = (nullVal ? SQLUDR_NULL : SQLUDR_NOTNULL);

        // Make a copy of the input value
        if (! nullVal)
        {
          char *inData = ((char *) inputRow) + p.inDataOffset();
          char *inCopy = data_[i];
          ComUInt32 inBytes = p.inSize();

          switch (p.fsType())
          {
            case COM_FCHAR_FSDT:
            {
              // CHAR(N) CHARACTER SET ISO88591
              memcpy(inCopy, inData, inBytes);
              inCopy[inBytes] = 0;
            }
            break;
            
            case COM_FCHAR_DBL_FSDT:
            {
              // CHAR(N) CHARACTER SET UCS2
              memcpy(inCopy, inData, inBytes);
              inCopy[inBytes] = 0;
              inCopy[inBytes + 1] = 0;
            }
            break;

            case COM_VCHAR_FSDT:
            case COM_VCHAR_DBL_FSDT:
            {
              // VARCHAR(N) CHARACTER SET ISO88591
              // VARCHAR(N) CHARACTER SET UCS2
              SQLUDR_VC_STRUCT *vc = (SQLUDR_VC_STRUCT *) inCopy;
	      ComUInt32 inDataLen = p.actualInDataSize(inputRow);
	      memcpy(&(vc->length), &inDataLen, 4);
              memcpy(vc->data, inData, inDataLen);
            }
            break;

            case COM_SIGNED_BIN16_FSDT:
            case COM_UNSIGNED_BIN16_FSDT:
            {
              // SMALLINT [UNSIGNED]
              // NUMERIC 0 <= precision <= 4
              memcpy(inCopy, inData, 2);
            }
            break;

            case COM_SIGNED_BIN32_FSDT:
            case COM_UNSIGNED_BIN32_FSDT:
            case COM_FLOAT32_FSDT:
            {
              // INTEGER [UNSIGNED]
              // REAL
              // NUMERIC 5 <= precision <= 9
              memcpy(inCopy, inData, 4);
            }
            break;

            case COM_SIGNED_BIN64_FSDT:
            case COM_FLOAT64_FSDT:
            {
              // LARGEINT
              // FLOAT
              // DOUBLE PRECISION
              // NUMERIC 10 <= precision <= 18
              memcpy(inCopy, inData, 8);
            }
            break;

            case COM_SIGNED_DECIMAL_FSDT:
            case COM_UNSIGNED_DECIMAL_FSDT:
            case COM_DATETIME_FSDT:
            case COM_SIGNED_NUM_BIG_FSDT:
            case COM_UNSIGNED_NUM_BIG_FSDT:
	    {
              // DECIMAL [UNSIGNED]
	      // DATE, TIME, TIMESTAMP
              // NUMERIC precision > 18
	      memcpy(inCopy, inData, inBytes);
	      inCopy[inBytes] = '\0';
	    }
	    break;

            default:
            {
              char msg[256];
              sprintf(msg, "Unknown parameter type: %d", p.fsType());
              LM_ASSERT1(0, msg);
            }
            break;
            
          } // switch (p.fsType())
        } // if not null
      } // if (isInput)

      else
      {
        numOut++;

        // Set the output null indicator
        udrInd[i] = SQLUDR_NOTNULL;

        // Initialize the output buffer
        char *outData = (char *)outputRow +  p.outDataOffset();
        char *outCopy = data_[i];
        ComUInt32 outBytes = p.outSize();
        
        switch (p.fsType())
        {
          case COM_FCHAR_FSDT:
          {
            // CHAR(N) CHARACTER SET ISO88591
            memset(outCopy, ' ', outBytes);
            outCopy[outBytes] = 0;
          }
          break;

          case COM_FCHAR_DBL_FSDT:
          {
            // CHAR(N) CHARACTER SET UCS2
            NAWchar *wOutCopy = (NAWchar *) outCopy;
            ComUInt32 outChars = outBytes / sizeof(NAWchar);
            wc_str_pad(wOutCopy, outChars); // pads with space by default
            wOutCopy[outChars] = 0;
          }
          break;

          case COM_VCHAR_FSDT:
          case COM_VCHAR_DBL_FSDT:
          {
            // VARCHAR(N) CHARACTER SET ISO88591
            // VARCHAR(N) CHARACTER SET UCS2
            SQLUDR_VC_STRUCT *vc = (SQLUDR_VC_STRUCT *) outCopy;
            vc->length = outBytes;
            memset(vc->data, 0, outBytes);
          }
          break;

          case COM_SIGNED_BIN16_FSDT:
          case COM_UNSIGNED_BIN16_FSDT:
          {
            // SMALLINT [UNSIGNED]
            // NUMERIC 0 <= precision <= 4
            memset(outCopy, 0, 2);
          }
          break;
          
          case COM_SIGNED_BIN32_FSDT:
          case COM_UNSIGNED_BIN32_FSDT:
          case COM_FLOAT32_FSDT:
          {
            // INTEGER [UNSIGNED]
            // REAL
            // NUMERIC 5 <= precision <= 9
            memset(outCopy, 0, 4);
          }
          break;
          
          case COM_SIGNED_BIN64_FSDT:
          case COM_FLOAT64_FSDT:
          {
            // LARGEINT
            // FLOAT
            // DOUBLE PRECISION
            // NUMERIC 10 <= precision <= 18
            memset(outCopy, 0, 8);
          }
          break;

          case COM_SIGNED_DECIMAL_FSDT:
          case COM_UNSIGNED_DECIMAL_FSDT:
	  case COM_DATETIME_FSDT:
          case COM_SIGNED_NUM_BIG_FSDT:
          case COM_UNSIGNED_NUM_BIG_FSDT:
	  {
            // DECIMAL [UNSIGNED]
	    // DATE, TIME, TIMESTAMP
            // NUMERIC precision > 18
	    memset(outCopy, ' ', outBytes);
            outCopy[outBytes] = 0;
	  }
	  break;

          default:
          {
            char msg[256];
            sprintf(msg, "Unknown parameter type value: %d", p.fsType());
            LM_ASSERT1(0, msg);
          }
          break;
          
        } // switch (p.fsType())
      } // if (isInput) else ...
    } // for each parameter
  } // if this is not a FINAL call
  
  else
  {
    // This is a FINAL call. Set all null indicators to SQLUDR_NULL
    // and zero out data buffers.
    for (i = 0; i < argc; i++)
    {
      udrInd[i] = SQLUDR_NULL;
      LmCBuffer &cBuf = cBuf_[i];
      cBuf.set(0);
    }
  }

  // Initialize SQLSTATE to all '0' characters and add a null terminator
  str_pad(sqlState_, SQLUDR_SQLSTATE_SIZE - 1, '0');
  sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
  
  // Initialize SQL text to all zero bytes
  str_pad(msgText_, SQLUDR_MSGTEXT_SIZE, '\0');

  // Now we can call the routine body...
  ComSInt32 rc = SQLUDR_INVOKE(routine_, argc, data_, udrInd,
                               sqlState_, msgText_, callType_,
                               stateArea_, udrInfo_);
  
  // Set the call type for the next invocation to NORMAL if this is
  // an INITIAL call
  if (callType_ == SQLUDR_CALLTYPE_INITIAL)
  {
    callType_ = SQLUDR_CALLTYPE_NORMAL;
    finalCallRequired_ = TRUE;
  }
  else if (callType_ == SQLUDR_CALLTYPE_FINAL)
  {
    // We are done if this is a FINAL call
    finalCallRequired_ = FALSE;
    return LM_OK;
  }

  LmResult lmResult = LM_OK;

  if (rc != SQLUDR_ERROR)
  {
    // Copy data and null indicator to caller's output buffers
    for (i = numIn; i < argc && lmResult == LM_OK; i++)
    {
      LmParameter &p = lmParams_[i];
      NABoolean isOutput = (p.direction() == COM_OUTPUT_COLUMN ? TRUE : FALSE);
      if (isOutput)
      {
        // Look at the returned null indicator. Raise an error if the
        // routine returned an invalid null indicator.
        NABoolean isNull = TRUE;
        switch(udrInd[i])
        {
          case SQLUDR_NOTNULL:
            isNull = FALSE;
            break;

          case SQLUDR_NULL:
            break;

          default:
          {
            *da << DgSqlCode(-LME_UDF_INVALID_DATA)
                << DgString0(getNameForDiags())
                << DgInt0((Lng32) i + 1)
                << DgString1("Invalid null indicator");
            lmResult = LM_ERR;
          }
          break;
        }

        // Write the null indicator into the output row
        if (lmResult == LM_OK)
          p.setNullOutput((char *) outputRow, isNull);

	// If value is not NULL, set data.
        if (lmResult == LM_OK && !isNull)
        {
          char *outData = ((char *)outputRow) + p.outDataOffset();
          char *outCopy = data_[i];
          ComUInt32 outBytes = p.outSize();

          switch (p.fsType())
          {
            case COM_FCHAR_FSDT:
            case COM_FCHAR_DBL_FSDT:
            {
              // CHAR(N) CHARACTER SET ISO88591
              // CHAR(N) CHARACTER SET UCS2
              p.setOutChar(outputRow, outCopy, outBytes);
            }
            break;

            case COM_VCHAR_FSDT:
            case COM_VCHAR_DBL_FSDT:
            {
              // VARCHAR(N) CHARACTER SET ISO88591
              // VARCHAR(N) CHARACTER SET UCS2
              SQLUDR_VC_STRUCT *vc = (SQLUDR_VC_STRUCT *) outCopy;
              if (vc->length > outBytes)
              {
                char msg[100];
                sprintf(msg, "VARCHAR length should not exceed %d",
                        outBytes);
                *da << DgSqlCode(-LME_UDF_INVALID_DATA)
                    << DgString0(getNameForDiags())
                    << DgInt0((Lng32) i + 1)
                    << DgString1(msg);
                lmResult = LM_ERR;
              }
              else
              {
                p.setOutChar(outputRow, vc->data, vc->length);
              }
            }
            break;

            case COM_SIGNED_BIN16_FSDT:
            case COM_UNSIGNED_BIN16_FSDT:
            {
              // SMALLINT [UNSIGNED]
              // NUMERIC 0 <= precision <= 4
              memcpy(outData, outCopy, 2);
            }
            break;

            case COM_SIGNED_BIN32_FSDT:
            case COM_UNSIGNED_BIN32_FSDT:
            case COM_FLOAT32_FSDT:
            {
              // INTEGER [UNSIGNED]
              // REAL
              // NUMERIC 5 <= precision <= 9
              memcpy(outData, outCopy, 4);
            }
            break;

            case COM_SIGNED_BIN64_FSDT:
            case COM_FLOAT64_FSDT:
            {
              // LARGEINT
              // FLOAT
              // DOUBLE PRECISION
              // NUMERIC 10 <= precision <= 18
              memcpy(outData, outCopy, 8);
            }
            break;

            case COM_SIGNED_DECIMAL_FSDT:
            case COM_UNSIGNED_DECIMAL_FSDT:
	    case COM_DATETIME_FSDT:
            case COM_SIGNED_NUM_BIG_FSDT:
            case COM_UNSIGNED_NUM_BIG_FSDT:
            {
	      // DECIMAL [UNSIGNED]
	      // DATE, TIME, TIMESTAMP
              // NUMERIC precision > 18
	      p.setOutChar(outputRow, outCopy, outBytes);
	    }
	    break;

            default:
            {
              char msg[256];
              sprintf(msg, "Unknown parameter type value: %d", p.fsType());
              LM_ASSERT1(0, msg);
            }
            break;
            
          } // switch (p.fsType())
        } // if (lmResult == LM_OK && !isNull)
      } // if (isOutput)
    } // for each LmParameter
  } // if (rc != SQLUDR_ERROR)
  
  if (lmResult == LM_OK && rc != SQLUDR_SUCCESS)
  {
    sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
    lmResult = processReturnStatus(rc, da);
  }

  return lmResult;

} // LmRoutineCSql::invokeRoutine
