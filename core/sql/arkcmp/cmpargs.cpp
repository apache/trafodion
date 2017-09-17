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
******************************************************************************
*
* File:         cmpargs.C
* Created:      
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "Platform.h"   

#include <ctype.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmpargs.h"
#include "NAExit.h"
#include "SchemaDB.h"
#include "CmpErrors.h"
#include "ErrorMessage.h"



// In order to support template instantiation for TANDEM builds (using
// c89), we conditionally include cmp_templ.C.  Furthermore, we
// #define a special macro, __CMP_TMPL_INCLUDED so that
// we obtain the template implementation code from that file.
//

static ostream &operator<<(ostream &dest, ComDiagsArea& diags)
{
  NADumpDiags(dest, &diags, FALSE, NO_COMMENT /*info msg style*/);
  return dest;
}

static void printCmdline(Int32 argc, char **argv)
{
  static char TDM_ARKCMP[] = "tdm_arkcmp";
  if (argc == 0 || argv[0] == NULL || argv[0][0] == '\0')
    {
      if (argc == 0) argc = 1;
      argv[0] = TDM_ARKCMP;
    }
  for (Int32 i = 0; i < argc; i++)
    {
      if (i) cout << ' ';
      cout << argv[i];
    }
  cout << endl;
}


// process -g {moduleGlobal|moduleLocal}
Int32 Cmdline_Args::doModuleGlobalLocal(char *arg, Int32 argc, char **argv, ComDiagsArea& diags)
{
  Int32 result=0;
  if (!strcmp(arg, "moduleGlobal")) {
    modulePlacement_ = MOD_GLOBAL;
  }
  else if (!strcmp(arg, "moduleLocal")) {
    modulePlacement_ = MOD_LOCAL;
  }
  else if (!strlen(arg)) {
    diags << DgSqlCode(mxcmpUmOptionGargumentMissing); cout << diags;
    usage(argc, argv);
    result = -1;
  }
  else {
    diags << DgSqlCode(mxcmpUmOptionGargumentUnrecognized); cout << diags;
    usage(argc, argv);
    result = -1;
  }
  return result;
}

// process -g {moduleGlobal|moduleLocal[=OSSdirectory]}
Int32 Cmdline_Args::doModuleGlobalLocalDir(char *arg, Int32 argc, char **argv,
                                         Int32 &gCount, ComDiagsArea& diags)
{
  Int32 result = 0;
  // at most one -g moduleBlah option is allowed
  if (gCount > 0) {
    diags << DgSqlCode(mxcmpUmAtMostOneoptionGisAllowed); cout << diags;
    usage(argc, argv);
    result = -1;
  }
  gCount++;
  // process -g {moduleGlobal|moduleLocal}
  char *eq = strchr(arg, '=');
  if (!eq) { // -g {moduleGlobal|moduleLocal}
    return doModuleGlobalLocal(arg, argc, argv, diags);
  }
  // else -g moduleLocal=OSSdirectory
  if (strlen(eq+1) >= 1024) { // OSSdirectory is too long
    diags << DgSqlCode(mxcmpUmOssDirectoryPathTooLong) << DgString0(eq+1); cout << diags;
    usage(argc, argv);
    result = -1;
  }
  else { // OSSdirectory < 1024
    // verify we have -g moduleLocal=OSSdirectory
    Int32 kwdLen=(Int32)strlen("moduleLocal");
    if (eq-arg == kwdLen && strncmp(arg,"moduleLocal",kwdLen)==0) { 
      modulePlacement_ = MOD_LOCAL;
      // reject any Expand or Guardian path
      if (!strncmp(eq+1,"/E/",3) || !strncmp(eq+1,"/G/",3)) {
        diags << DgSqlCode(mxcmpUmModuleLocalSpecifyDir); cout << diags;
        usage(argc, argv);
        result = -1;
      }
      else {
        // copy OSSdirectory
        moduleDir_ = eq+1;
      }
    }
    else {
      diags << DgSqlCode(mxcmpUmUnsupportedArgumentInOptionG); cout << diags;
      usage(argc, argv);
      result = -1;
    }
  }
  return result;
}

char* Cmdline_Args::getModuleDir() const
{
  if (moduleLocal()) {
    return CONST_CAST(char*,moduleDir_.data()); // use this local directory
  }
  else {
    // use global USERMODULES directory. What we want to do here is
    // return "/usr/tandem/sqlmx/USERMODULES/";
    // But, unfortunately, our caller will pass our return value to
    // ComRtGetModuleFileName(modulename, moduledir, ...) whose logic
    // relies on a null moduledir to compute the global module directory.
    // So, we are forced to do
    return NULL; 
    // Otherwise, mxcmp may place the module in /usr/tandem/sqlmx/USERMODULES
    // whereas the debug version of the executor may look for it in the
    // SQLMX_MODULE_DIR environment variable setting and the result is an
    // error 8809 (unable to open the module file).
  }
}
 
bool Cmdline_Args::moduleLocal() const
{
  return FALSE;
}

static void DisplayDebugBox()
{
  Int32 pid = getpid();
  char stmp[256];
  snprintf(stmp, sizeof(stmp), "Process Launched %d", pid);
  MessageBox(NULL, stmp , "MXCMP", MB_OK|MB_ICONINFORMATION);
}

Cmdline_Args::Cmdline_Args() 
  : modulePlacement_(NOT_SET), 
    moduleDir_("."), // local module dir defaults to current dir
    application_(), 
    moddef_(), 
    module_(), 
    isStat_(FALSE),    // static compile with mdf name 
    hasListing_(FALSE),
    isVerbose_(FALSE),
    ignoreErrors_(FALSE),
    allocMethod_(IPC_ALLOC_DONT_CARE),
    socketArg_(0),
    portArg_(0),
    settings_(NULL), // This NAArray goes on system heap. NULL for heap*
    noSeabaseDefTableRead_(FALSE)
{}

void Cmdline_Args::processArgs(Int32 argc, char **argv)
{
  Space localHeap;
  ComDiagsArea diags(&localHeap);

  // The following GUI code is to start up arkcmp process into debug
  // only if you want to go into debug mode before the arguments are parsed.
  if (getenv("SQL_CMP_MSGBOX_PROCESS"))
    MessageBox(NULL, "Process Launched", "tdm_arkcmp", MB_OK|MB_ICONINFORMATION);

#ifndef NDEBUG
  if (getenv("ARKCMP_ARGS_ECHO"))
    {
      // We'll output ">> sh tdm_arkcmp -fooOptions arg" (a la sqlci, regress)
      cout << ">> sh ";
      printCmdline(argc, argv);
    }
#endif

  
  Int32 extraArgs = 0;
  for ( Int32 i=1; i < argc; i++ )	// start at 1 (args) not 0 (progname)
    {
      if (!argv[i]) continue;

      if (strcmp(argv[i], "-debug") == 0)
        {
	  DisplayDebugBox();
        }
      else if (strcmp("-fork", argv[i]) == 0)
        {
          allocMethod_ = IPC_POSIX_FORK_EXEC;
        }
      else if (strcmp("-service", argv[i]) == 0)
        {
          // /etc/inetd.conf must be configured with the "-service" option
          allocMethod_ = IPC_INETD;
        }
      else if (strcmp(argv[i], "-oss") == 0)
        {
	  allocMethod_ = IPC_SPAWN_OSS_PROCESS;
        }
      else if (strcmp("-guardian", argv[i]) == 0)
        {
          allocMethod_ = IPC_LAUNCH_GUARDIAN_PROCESS;
        }
      else if (strcmp(argv[i], "-##") == 0) {
	  if (!extraArgs) extraArgs = -1;
	  break;
        }
      else if (strcmp(argv[i], "-noSeabaseDefTableRead") == 0)
	{
	  noSeabaseDefTableRead_ = true;
	}
      else
        extraArgs++;
    }

  if (allocMethod_ != IPC_ALLOC_DONT_CARE || socketArg_)  {
    if (extraArgs == 0) { 	// arkcmp -socket s p
      return;
    }
    else if (extraArgs > 0) {	// arkcmp -mModuleX -socket -v  (stmt="ocket")
      allocMethod_ = IPC_ALLOC_DONT_CARE;
      socketArg_ = portArg_ = 0;
    }
    else {			// arkcmp -socket s p -## -mModuleX -sOcket -v
      // TODO: need to init getopt such that optind points at (past?)
      // this "-##" demarcator ...
    }
  }

  if (argc <= 1) usage(argc, argv);	// no "?" errmsg, just this helpful info

  Int32 ch, gCount=0;
  while ((ch = getopt(argc, argv, "a:d:eg:h?l:v")) != -1) {
    switch (ch) 
      {
      case 'a':  // application
	application_ = optarg;
	isStat_ = TRUE;
        break;
      case 'd': // -d default_attrib=default_value
        { 
          NAString nam(optarg);
          // find the = if it exists
#pragma nowarn(1506)   // warning elimination 
          Int32 loopx = 0, len = nam.length();
#pragma warn(1506)  // warning elimination 
          while((loopx < len) && ((nam)(loopx) != '=') )loopx++;
          if (len == loopx) {
            diags << DgSqlCode(mxcmpUmIllformatedOptionD) << DgString0(optarg);
            cout << diags;
            usage(argc, argv);
          }
          else {
            // do substring copy of default attribute value
            NAString val = (nam)(loopx+1,len-loopx-1);
            // do substring copy of default attribute name
            nam = (nam)(0,loopx);
            // compose default attribute name & value into a ControlSetting
            ControlSetting s(nam, val);
            addSetting(s); // add ControlSetting to our collection. this 
            // should make a copy. we wish to create this collection in the
            // stmt heap but stmt heap is not yet created at this point.
          }
          break;
        }
      case 'e': // ignore errors, return as warnings.
        ignoreErrors_ = TRUE;
        isStat_ = TRUE;
        break;

      case 'g':  // moduleglobal or modulelocal
        doModuleGlobalLocalDir(optarg, argc, argv, gCount, diags);
        break;

      case 'h':
      case '?':
        usage(0, NULL);	// display nice help message
	break;
      case 'l':  // listing is turned on
        hasListing_ = TRUE;
        isStat_ = TRUE;
        break;

      case 'v':
        isVerbose_ = TRUE;
        break;

      default: 
        {
          char buf[2];
          buf[0] = (char)ch; buf[1] = 0;
          diags << DgSqlCode(mxcmpUmNoCaseForOption) << DgString0(buf) << DgInt1((Int32)ch);
          cout << diags;
	  usage(argc, argv);
        }
        
    } // switch
  } // while

  Int32 sane = TRUE;

  if (optind < argc)
    {
    if (optind < argc-1)
      {
        diags << DgSqlCode(mxcmpUmTooManyArgumentsOrOptionsIllplaced); cout << diags;
	usage(argc, argv);
      }
      moddef_ = argv[optind];
      isStat_ = TRUE;
    }

  if ( isVerbose_ )  
    printArgs();

  // Sanity checks.  Keep in sync with the code above AND with the
  // very helpful usage() output below!
  //
  if (isStat_ && (!module_.isNull()))
    sane = FALSE;
  if (!sane)
    {
      diags << DgSqlCode(mxcmpUmInvalidCombinationOfOptions); cout << diags;
      usage(argc, argv);
    }
} 

void Cmdline_Args::overwriteSettings() const
{
  for (CollIndex x=0; x<settings_.entries(); x++) {
    NAString value(settings_[x].attrValue);
    ActiveSchemaDB()->getDefaults().validateAndInsert
      (settings_[x].attrName.data(), value, FALSE /*don't reset*/);
  }
}

void Cmdline_Args::usage(Int32 argc, char ** argv)
{
  // If user typed only "arkcmp" w/no options or args, print only usage msg;
  // if any opts or args, print only the bad cmdline.
  if (argc > 1)
    {
      printCmdline(argc, argv);
      NAExit(1);
    }

  Space localHeap;
  ComDiagsArea diags(&localHeap); diags << DgSqlCode(mxcmpUsage); cout << diags;

  NAExit(0);
  
} // Cmdline_Args::usage()

void Cmdline_Args::printArgs()
{
  // cout, not cerr
  // cout changed to cerr in order to satisfy th requirements for c89. 
  cerr << "Print command line arguments" << endl;
  cerr << "application_ = " << application_ << endl;
  cerr << " hasListing_ = " << hasListing_ << endl;
  cerr << "     moddef_ = " << moddef_ << endl; 
  cerr << "     module_ = " << module_ << endl;
  
  cerr << "   isStat_ = " << isStat_ << endl;
  cerr << "isVerbose_ = " << isVerbose_ << endl;
  cerr << "ignoreErrors_ = " << ignoreErrors_ << endl;
  cerr << "modulePlacement_ = " << modulePlacement_ << endl;
  cerr << "moduleDir_ = " << moduleDir_.data() << endl;

  // print command-line-specified control query defaults
  for (CollIndex x=0; x<settings_.entries(); x++) {
    cerr << settings_[x].attrName.data() << "=" 
         << settings_[x].attrValue.data() << endl;
  }
  cerr << endl;
} // end Cmdline_Args::printArgs()
