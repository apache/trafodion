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
 * File:         UdrDebug.cpp
 * Description:  debug functions for the UDR server
 *               
 *               
 * Created:      6/20/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"
#include "UdrFFDC.h"
#include "UdrDebug.h"

#ifdef UDR_DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "NABoolean.h"
FILE *udrDebugFile = stdout;

NABoolean doUdrDebug(){
  static NABoolean doUdrDebugFlag = FALSE;
  static NABoolean firstTime = TRUE;
  if (firstTime)
  {
    firstTime = FALSE;
    //
    // Note: the "UDR_DEBUG" string is split into two adjacent strings so
    // the preprocessor does not perform macro substitution on UDR_DEBUG.
    //
    if (getenv("UDR_""DEBUG") || getenv("MXUDR_""DEBUG"))
    {
      doUdrDebugFlag = TRUE;
    }
  }
  return doUdrDebugFlag;
}

void udrDebug(const char *formatString, ...)
{
  if (doUdrDebug())
  {
    va_list args;
    va_start(args, formatString);
    fprintf(udrDebugFile, "[MXUDR] ");
    vfprintf(udrDebugFile, formatString, args);
    fprintf(udrDebugFile, "\n");
    fflush(udrDebugFile);
  }
}


#endif // UDR_DEBUG





