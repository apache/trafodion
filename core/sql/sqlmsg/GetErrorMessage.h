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
//
**********************************************************************/
#ifndef GETERRORMESSAGE_H
#define GETERRORMESSAGE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         GetErrorMessage.h
 * RCS:          $Id: GetErrorMessage.h,v 1.12 1998/07/20 07:26:40  Exp $
 * Description:
 *
 * Created:      4/5/96
 * Modified:     $ $Date: 1998/07/20 07:26:40 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *****************************************************************************
 */

#include "NABoolean.h"
#include "NAWinNT.h"

enum MsgTextType { ERROR_TEXT=1,
                   CAUSE_TEXT,
                   EFFECT_TEXT,
                   RECOVERY_TEXT,
                   SQL_STATE,
                   HELP_ID,
                   EMS_SEVERITY,
                   EMS_EVENT_TARGET,
                   EMS_EXPERIENCE_LEVEL
                 };

#define SQLERRORS_MSGFILE_VERSION_INFO	   10	// see sqlci Env command
#define SQLERRORS_MSGFILE_NOT_FOUND	16000	
#define SQLERRORS_MSG_NOT_FOUND		16001	

#ifdef __cplusplus
extern "C"
{
#endif


void 	 GetErrorMessageRebindMessageFile();
char 	*GetPastHeaderOfErrorMessage(char *text);


const char 	*GetErrorMessageFileName();

short GetErrorMessageRC (Lng32 num, NAWchar *msgBuf, Lng32 bufSize);

short GetErrorMessage (Lng32 error_code, 
                                          NAWchar*& return_text,
                                          MsgTextType M_type = ERROR_TEXT,
                                          NAWchar *alternate_return_text = NULL,
                                          Int32 recurse_level = 0, 
                                          NABoolean prefixAdded = TRUE);




void ErrorMessageOverflowCheckW (NAWchar *buf, size_t maxsiz);


#ifdef __cplusplus
}
#endif

#endif /* GETERRORMESSAGE_H */
