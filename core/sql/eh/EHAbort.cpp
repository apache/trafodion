/* -*-C++-*-
 *****************************************************************************
 *
 * File:         EHAbort.C
 * Description:  global function to abort the current process
 *               The global function EHAbort was derived from
 *               the function CascadesAbort in the file opt.C
 *
 *               
 * Created:      5/18/95
 * Language:     C++
 *
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
 *
 *
 *****************************************************************************
 */


#include "EHCommonDefs.h"


#include <stdlib.h>
#include <stdio.h>


#include "logmxevent.h"
// -----------------------------------------------------------------------
// global function to abort the process
// -----------------------------------------------------------------------
void EHAbort(const char * filename, Int32 lineno, const char * msg)
{
  fflush(stdout);
  fprintf(stderr, "\n*** Fatal Error *** Exception handler aborted "
          "in \"%s\", line %d:\n%s\n\n",
          filename, lineno, msg);
  fflush(stderr);

  SQLMXLoggingArea::logSQLMXAbortEvent(filename, lineno, msg);


  // This env variable should be used by development only and only when
  // the tdm_nonstop is started from the command line (windows up).
  #if defined(DEBUG) || defined(_DEBUG)
  if (getenv("SQLMX_FAILURE"))
    {
      char message[256];
      LPCSTR messagep = (const char *)&message;
      char line__[9];

      strcpy(message,"EHAbort: *** Fatal Error *** Exception handler aborted in ");
      strcat(message,filename);
      strcat(message,", line ");
#ifdef NA_ITOA_NOT_SUPPORTED
      snprintf(line__, 9, "%d",lineno);
#else
      itoa(lineno,line__,10);
#endif // NA_ITOA_NOT_SUPPORTED
      strcat(message,line__);
      strcat(message,msg);
	  
      MessageBox( NULL,
		  (CHAR *)&message,
		  "NonStop SQL/MX",
		  MB_OK|MB_ICONINFORMATION );
    };
  #endif   // DEBUG || _DEBUG

  
  
  exit(1);
  
} // EHAbort()

//
// End of File
//

