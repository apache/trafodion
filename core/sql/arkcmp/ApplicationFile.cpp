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
#include <iostream>
#include "ApplicationFile.h"
#include "Cmdline_Args.h"
#include "ComDiags.h"
#include "DgBaseType.h"
#include "ELFFile.h"
#include "NAAssert.h"
#include "SQLJFile.h"
#include "mxCompileUserModule.h"


// factory method (or virtual constructor)
ApplicationFile* ApplicationFile::makeApplicationFile(std::string& filename)
{
  // on NT/Win2K/XP, pretend it's an ELF file. SQLJ is not yet on NT.
  return new ELFFile(filename);
}

// constructor
ApplicationFile::ApplicationFile(std::string &filename)
  : fileName_(filename), nCompiles_(0), nFailures_(0), args_(NULL)
  , appFile_(NULL)
{
}

// destructor
ApplicationFile::~ApplicationFile()
{
}

// open application file for reading. return true if all OK.
// NB: It seems illogical to name this method "openFile" when it does not
// open any file at all. However, the classes ApplicationFile, ELFFile,
// SQLJFile and mxCompileUserModuleMain.cpp implement an instance of the
// Factory design pattern. The makeApplicationFile() returns a pointer to
// an ApplicationFile (either an ELFFile or a SQLJFile instance) appFile 
// and main() simply calls appFile->openFile(), appFile->closFile(), 
// appFile->processModule(), and so on, relying on these virtual functions
// to invoke the appropriate instance of these methods to make it work.
bool ApplicationFile::openFile(Cmdline_Args &args)
{
  args_ = &args;
  return appFile_ != NULL;
}

// close application file
bool ApplicationFile::closeFile() // return true if all OK
{
  bool result = fclose(appFile_) == 0;
  appFile_ = NULL;
  return result;
}

// create a temporary file name that is not the name of an existing file.
// requires: allocated length of tNam >= L_tmpnam+10.
// returns : tNam if all OK; NULL otherwise.
char* ApplicationFile::getTempFileName(char *tNam)
{
  // create temporary file name
  char tempName[L_tmpnam], *tmp;
  tmp = tmpnam(tempName);
  if (!tmp) {
    *mxCUMptr << FAIL << DgSqlCode(-2205);
    return tmp;
  }
  else {
    // use tempName || ourProcessID as the temporary file name
    char pid[20]; 
    sprintf(pid, "%0d", _getpid());
    strcpy(tNam, tmp);
    strcat(tNam, pid);
    return tNam;
  }
}

// invoke mxcmp on module definition file
bool ApplicationFile::mxcmpModule(char *mdf)
{
  nCompiles_++;
  char cmd[1024], *cmdP=cmd, *mxcmp = getenv("MXCMP");
  #define DEFAULT_MXCMP "tdm_arkcmp"
  mxcmp = mxcmp ? mxcmp : DEFAULT_MXCMP;
  // make sure we have enough space for the mxcmp invocation string
  assert(args_ != NULL);
  Int32 cmdLen = strlen(mxcmp)+args_->application().length()+
    args_->otherArgs().length()+strlen(mdf)+7;
  // for efficiency we try to use the stack-allocated cmd variable.
  // but, no matter how big we declare it, eg: char cmd[12345],
  // it is always possible for someone like QA try something like
  //   mxCompileUserModule -d CQD1=v1 -d CQD2=v2 ... -d CQDn=vn my.exe
  // whereby the mxcmp invocation string overflows cmd. We can always
  // dynamically allocate and use cmdP but "new" is very inefficient
  // compared to stack allocation. So, we try to get the best of both
  // by using stack allocation for the 90% case and use "new" only for
  // the pathological 10% (customer is using a program generator to 
  // invoke us or QA is simply trying to break our code) case.
  if (cmdLen > 1024) {
    cmdP = new char[cmdLen];
    // mxCompileUserModuleMain.cpp already did set_new_handler(), so
    // if this new fails, mainNewHandler() will handle that exception.
  }
  if (!cmdP) {
    // coverity flags a REVERSE_INULL cid on cmdP -- it's complaining
    // we're not checking that cmdP is non-null. So, we oblige.
    nFailures_++;
    return false;
  }
  strcpy(cmdP, mxcmp);
  strcat(cmdP, " ");
  strcat(cmdP, args_->otherArgs().c_str());
  strcat(cmdP, " ");
  strcat(cmdP, mdf);
  cout << cmdP << endl;
  Int32 rc = system(cmdP);
  // free any space used by mxcmp invocation string
  if (cmdP && cmdP != cmd) {
    delete cmdP;
  }
  if (rc == 0) { // success
    if (!args_->keepMdf()) {
      remove(mdf);
    }
  }
  else if (rc == -1
           ) { 
    // function fails: cannot create child process 
    // or cannot get child shell's exit status
    *mxCUMptr << FAIL << DgSqlCode(-2221) << DgInt0(rc);
    nFailures_++;
  }
  else { // unsuccessful mxcmp invocation
    Int32 sqlcode, int0;
    mxcmpExitCode retcode;
    int0 = rc;
    retcode = ERROR; // cannot tell ERROR from WARNING on NT
    sqlcode = 2201;
    *mxCUMptr << retcode << DgSqlCode(-sqlcode) << DgInt0(int0);
    nFailures_++;
  }
  return rc==0;
}

// print summary statistics
void ApplicationFile::printSummary()
{
  cout << modulesFound() << " modules found, "
       << modulesExtracted() << " modules extracted." << endl
       << nCompiles_ << " mxcmp invocations: " 
       << nCompiles_-nFailures_ << " succeeded, "
       << nFailures_ << " failed." << endl;
}

