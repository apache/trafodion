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
#include <sys/types.h>

#include "Cmdline_Args.h"
#include "ComDiags.h"
#include "DgBaseType.h"
#include "ErrorMessage.h"
#include "SQLJFile.h"
#include "DgBaseType.h"
#include "ExtQualModuleNames.h"
#include "mxCompileUserModule.h"

// constructor
SQLJFile::SQLJFile(std::string &filename)
  : ApplicationFile(filename), nExtracted_(0), modNamList_("")
{
  mdf_[0]=0;
}

// destructor
SQLJFile::~SQLJFile()
{
}

// open SQLJ file for extraction of embedded module definitions.
// return true if all OK.
bool SQLJFile::openFile(Cmdline_Args &args)
{
  // put together MDFWriter invocation
  Lng32 cmdLen;
  char cmd[1024], *cmdP=cmd, *moduleNamesOption, cmfn[L_tmpnam+40];
  char *errFileName,errFileNm[L_tmpnam+20];
  char *outputFileName,outputFileNm[L_tmpnam+20];
  if ((moduleNamesOption=getModuleNamesOption(args,cmfn)) == NULL ||
      (errFileName=getTempFileName(errFileNm)) == NULL ||
      (outputFileName=getTempFileName(outputFileNm)) == NULL) {
    return ApplicationFile::openFile(args);
  }

  // invoke MDFWriter to extract modules
  const char mdfWriter[]= "java sqlj.runtime.profile.util.MDFWriter";
  cmdLen = strlen(mdfWriter) + strlen(moduleNamesOption) + 2 +
    strlen("-CerrorFileName=") + strlen(errFileName) + 2 +
    strlen("-CMDFList=") + strlen(outputFileName) + 1 +
    args.application().length();
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
  }
  strcpy(cmdP, mdfWriter);
  strcat(cmdP, " ");
  strcat(cmdP, moduleNamesOption);
  strcat(cmdP, " -CerrorFileName=");
  strcat(cmdP, errFileName);
  strcat(cmdP, " -CMDFList=");
  strcat(cmdP, outputFileName);
  strcat(cmdP, " ");
  strcat(cmdP, args.application().c_str());
  cout << cmdP << endl;
  Int32 rc = system(cmdP);
  if (cmdP != cmd) {
    delete cmdP;
  }

  // interpret MDFWriter's return code
  if (rc == 0) { // MDFWriter invocation OK
    // check MDFWriter output
    if (ACCESS(outputFileName, READABLE) != 0) { // no extractions
      *mxCUMptr << FAIL << DgSqlCode(-2217) << DgString0(outputFileName);
    }
    // open MDFWriter output
    else if ((appFile_=fopen(outputFileName, "r")) == NULL) {
      *mxCUMptr << FAIL << DgSqlCode(-2218) << DgString0(outputFileName);
    }
    else { // all OK
      if (!args.keepMdf()) { 
        // clean up temporary file (names of modules to extract)
        remove(modNamList_.c_str());
      }
    }
  }
  else if (rc == -1
           ) { 
    // function fails: cannot create child process 
    // or cannot get child shell's exit status
    *mxCUMptr << FAIL << DgSqlCode(-2216) << DgInt0(rc);
  }
  else { // unsuccessful module extraction
    *mxCUMptr << FAIL << DgSqlCode(-2216) << DgInt0(rc);
  }
  // report any MDFWriter errors/warnings now to avoid confusing the user
  // with possibly interleaved MDFWriter/mxcmp errors/warnings.
  printMDFWriterErrors(errFileName);
  return ApplicationFile::openFile(args);
}

// find next embedded module definition from application file
bool 
SQLJFile::findNextModule(std::string &modName)
{
  // make sure arguments are reasonable
  if (!appFile_) {
    return false; // nothing doing
  }
  if (fgets(mdf_, MAXMDF, appFile_) != NULL) {
    if (strlen(mdf_) >= MAXMDF-1) { // filename is too long
      *mxCUMptr << WARNING << DgSqlCode(2236);
      // toss rest of line
      Int32 nxtCh;
      do {
        nxtCh = fgetc(appFile_);
      } while (nxtCh != '\n' && nxtCh != EOF);
    }
    modName = mdf_;
    return true; // found one
  }
  return false; // found nothing
}

// extract embedded module definition from appFile_ into tempfile
bool 
SQLJFile::processModule()
{
  // make sure arguments are reasonable
  if (!appFile_ || !args_) {
    return false; // nothing doing
  }
  nExtracted_++;
  // SQL compile the extracted module definition
  return mxcmpModule(mdf_);
}

// return "" or "-CMFN=modNamList_" where modNamList_ is a temp file that 
// has the list of module names to be extracted by MDFWriter.
// requires: cmfn is long enough to hold "-CMFN=modNamList_".
char* SQLJFile::getModuleNamesOption(Cmdline_Args &args, char *cmfn)
{
  ExtQualModuleNames* EQMNs = args.getModuleNames();
  Int32 count;
  if (!EQMNs || (count=EQMNs->count()) <= 0) {
    return ""; // means extract all
  }
  else {
    // create a temporary file 
    char *modNamFil, modNam[L_tmpnam+20], templNam[L_tmpnam+26];
    modNamFil = getTempFileName(modNam);
    if (!modNamFil) {
      return NULL;
    }
    FILE *tFil = createTempFile(modNamFil, templNam);
    if (!tFil) {
      return NULL;
    }
    // write into temp file the names of modules to be extracted
    for (Int32 x=0; x<count; x++) {
      const ThreePartModuleName& modNam = EQMNs->at(x);
      fprintf(tFil, "%s\n", modNam.catalog.c_str());
      fprintf(tFil, "%s\n", modNam.schema.c_str());
      fprintf(tFil, "%s\n", modNam.module.c_str());
    }
    fclose(tFil);
    // return "-CMFN=modNamList_"
    strcpy(cmfn, "-CMFN=");
    strcat(cmfn, templNam);
    strcat(cmfn, " ");
    modNamList_ = templNam;
    return cmfn;
  }
}

// print MDFWriter errors to cout
void SQLJFile::printMDFWriterErrors(char *errFileName)
{
  char args[1024], EorW[10];
  Int32  errNum;
  FILE *errFile = fopen(errFileName, "r");
  if (errFile) {
    // accommodate case of MDFWriter dumping more entries into its errFile
    // than can fit into the fixed-size diags area. Do this by feeding
    // and then dumping diags one entry at a time.
    ComDiagsArea *myDiags = ComDiagsArea::allocate(mxCUMptr->heap());
    while (fscanf(errFile, "%s %d ", EorW, &errNum) != EOF) {
      size_t sLen = 0;
      if (fgets(args, 1024, errFile) == NULL) { // fgets got EOF or an error
        args[0] = 0; // empty string
        *mxCUMptr << FAIL;
      }
      else { // fgets got something
        sLen = strlen(args);
        // chop off terminating newline
        if (args[sLen-1] == '\n') {
          args[sLen-1] = 0;
        }
        if (sLen >= 1023) { // diagnostic msg is too long
          // toss rest of line
          Int32 nxtCh;
          do {
            nxtCh = fgetc(errFile);
          } while (nxtCh != '\n' && nxtCh != EOF);
        }
      }
      if (!myDiags) { // no diags
        *mxCUMptr << FAIL;
        if (sLen >= 1023) { // diagnostic msg is too long
          cerr << "Diagnostic message is over 1023 characters long." << endl;
        }
        // echo error file entry to cerr
        cerr << EorW << " " << errNum << " " << args << endl;
      }
      else {
        if (sLen >= 1023) { // diagnostic msg is too long
          *mxCUMptr << WARNING;
          *myDiags << DgSqlCode(2237);
        }
        switch (errNum) {
        case 2224:
        case 2225:
        case 2226:
          *mxCUMptr << FAIL; 
          *myDiags << DgSqlCode(-errNum);
          break;
        case 2227:
        case 2228:
        case 2230:
          *mxCUMptr << FAIL;
          *myDiags << DgSqlCode(-errNum) << DgString0(args);
          break;
        case 2229:
          *mxCUMptr << ERROR;
          *myDiags << DgSqlCode(-errNum) << DgString0(args);
          break;
        default:
          *mxCUMptr << (EorW[0]=='E' ? ERROR :
                        (EorW[0]=='W' ? WARNING : FAIL));
          *myDiags << DgSqlCode(-2231) << DgInt0(errNum) 
                   << DgString0(EorW) << DgString1(args);
          break;
        } // end switch
        NADumpDiags(cerr, myDiags, TRUE);
        myDiags->clear();
      } // end if
    } // end while
    if (myDiags) {
      myDiags->decrRefCount();
    }
    fclose(errFile);
  }
  else {
    *mxCUMptr << FAIL << DgSqlCode(-2218) << DgString0(errFileName);
  }
  // clean up temporary file (MDFWriter errors)
  remove(errFileName);
}

// return true iff this is a SQLJ JAR or profile file
bool SQLJFile::isSQLJ(std::string& filename)
{
  return 
    filename.rfind(".ser") != std::string::npos || // a SQLJ profile file
    filename.rfind(".jar") != std::string::npos;   // a SQLJ jar file
}


