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
#ifndef _UDR_FFDC_H_
#define _UDR_FFDC_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         UdrFFDC.h
 * Description:  MXUDR FFDC
 *               
 *               
 * Created:      5/4/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"
#include "NABoolean.h"


#include "NABoolean.h"

void comTFDS(const char *msg1, const char *msg2, const char *msg3, const char *msg4, const char *msg5, 
             NABoolean dialOut = TRUE
	   , NABoolean writeToSeaLog = TRUE
	     );
void makeTFDSCall(const char *msg, const char *file, UInt32 line, 
                  NABoolean dialOut = TRUE
	        , NABoolean writeToSeaLog = TRUE
	     );
void setUdrSignalHandlers();
void printSignalHandlers();
void logEMSWithoutDialOut(const char *msg);
void logEMS(const char *msg);
NABoolean saveUdrTrapSignalHandlers();
NABoolean setSignalHandlersToDefault();
NABoolean restoreJavaSignalHandlers();
NABoolean restoreUdrTrapSignalHandlers(NABoolean saveJavaSignalHandlers);
void setExitHandler();

#endif // _UDR_FFDC_H_






