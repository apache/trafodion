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
#ifndef APPLICATIONFILE__H
#define APPLICATIONFILE__H

/* -*-C++-*-
 *****************************************************************************
 * File:         ApplicationFile.h
 * Description:  This abstract class has the code that is common to finding &
 *               extracting embedded module definitions from C/Cobol ELF files
 *               and from SQLJ profile & JAR files.
 * Created:      04/11/2003
 * Language:     C++
 *****************************************************************************
 */
#include <stdio.h>
#include <string>
#include <fcntl.h>    // for _O_CREAT, O_CREAT
#include <sys/stat.h> // for _S_IWRITE, S_IWUSR

  #include <io.h>      // for _access(), _open(), _close(), _write()
  #include <process.h> // for _getpid()
  #define ACCESS _access
  #define CLOSE  _close
  #define READABLE 4
  #define EXECUTABLE 4

#include "Platform.h"  // 64-bit

class Cmdline_Args;

class ApplicationFile {
 public:
  // factory method
  static ApplicationFile* makeApplicationFile(std::string& filename);

  // constructor
  ApplicationFile(std::string &filename);

  // destructor
  virtual ~ApplicationFile();

  // open application file for reading
  virtual bool openFile(Cmdline_Args &args); // (IN): for module name list
  // return true if all OK

  // close application file
  virtual bool closeFile(); // return true if all OK

  // log any errors
  virtual void logErrors() = 0;

  // find next embedded module definition from this aplication file.
  // return false if no module definition found.
  virtual bool findNextModule
    (std::string &modName) // (OUT): name of module found
    = 0;

  // extract embedded module definition & SQL compile it.
  // return true if all OK.
  virtual bool processModule() = 0;

  // print summary statistics
  void printSummary();

  virtual Int32 modulesFound() = 0;
  virtual Int32 modulesExtracted() = 0;

 protected:
  // create a temporary file and open it for write
  FILE* createTempFile(char* nam, char *newNam);

  // create a temporary file name that is not the name of an existing file
  // requires: allocated length of tNam >= L_tmpnam+10.
  // returns : tNam if all OK; NULL otherwise.
  char* getTempFileName(char *tNam);

  // invoke mxcmp on module definition file
  bool mxcmpModule(char *mdf);

  std::string fileName_; // application file name
  Int32         nCompiles_;// number of mxcmp invocations
  Int32         nFailures_;// number of unsuccessful mxcmp invocations

  Cmdline_Args *args_;   // for determining if all named modules were found
  FILE         *appFile_;// handle to application file or 
                         // file of extracted SQLJ mdfs
};

#endif // APPLICATIONFILE__H
