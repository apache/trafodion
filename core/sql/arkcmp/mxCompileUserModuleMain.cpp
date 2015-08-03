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
 * File:         mxCompileUserModule.cpp
 * Description:  This is the main program for SQL compiling a C/C++/Cobol
 *               executable/library or SQLJ ser/jar file that has embedded
 *               module definitions.
 * Created:      03/03/2003
 * Language:     C++
 *
 *****************************************************************************
 */

#include <iostream>

#include "Cmdline_Args.h"
#include "ApplicationFile.h"
#include "ComDiags.h"
#include "DgBaseType.h"
#include "mxCompileUserModule.h"
#include <new.h>

mxCompileUserModule *mxCUMptr=NULL;

// mainNewHandler_CharSave and mainNewHandler are used in the error
// handling when running out of virtual memory for the main program.
// Save 1K bytes of memory for the error handling when running out of VM.
static char* mainNewHandler_CharSave = new char[1024];

  static Int32  mainNewHandler(size_t)

Int32 main(Int32 argc, char **argv)
{

  _set_new_handler(mainNewHandler);

  // for NA_YOS newHandler_NSK needs to be added, once it is ready -- Sri gadde

  mxCompileUserModule mxCUM;
  mxCUMptr = &mxCUM;

  // process command line arguments
  Cmdline_Args args;
  args.processArgs(argc, argv);
  ApplicationFile *appFile=NULL;

  // check if application file exists
  if (ACCESS(args.application().c_str(), READABLE) != 0) {
    mxCUM << ERROR << DgSqlCode(-2223)
          << DgString0(args.application().c_str());
  }
  else {
    // ask factory to create an ELFFile or a SQLJFile
    appFile = ApplicationFile::makeApplicationFile(args.application());
    if (!appFile) { // no, it's not an application file
      mxCUM << ERROR << DgSqlCode(-2202)
            << DgString0(args.application().c_str());
    }
    else {
      // open the application file
      if (appFile->openFile(args)) {
        // process appFile's embedded module definitions
        std::string modName;
        while (appFile->findNextModule(modName)) {
          // extract embedded module definition & SQL compile it
          if (!appFile->processModule()) {
            mxCUM << WARNING << DgSqlCode(-2204) << DgString0(modName.c_str());
            // set mxCUM to WARNING at least. processModule may have set
            // it to WARNING, ERROR, or FAIL. If we get here, mxCUM
            // should never be set to SUCCEED.
          }
        }
        // close the application file
        appFile->closeFile();
        appFile->logErrors();
      }
    }
  }
  mxCUM.dumpDiags();
  if (appFile) {
    appFile->printSummary();
    delete appFile;
  }
  return mxCUM.returnCode();
}
