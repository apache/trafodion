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
 * File:         SqlciMain.cpp
 * Description:  Main program for mxci
 *               
 *               
 * Created:      4/15/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "BaseTypes.h"
#include "NAAssert.h"
#include "SqlciEnv.h"
#include "SqlciNode.h"
#include "TempIncludes.h"
//#include "UtilInt.h"
#include "StmtCompilationMode.h" // because of a kludge in eh/EHException.cpp
#include "ComDiags.h"


  // The following are needed only until the "shadow-process" is implemented.
#include "rosetta/rosgen.h"
#define psecure_h_including_section
#define psecure_h_security_app_priv_
#define psecure_h_security_ntuser_set_
#include "security/psecure.h"





#include "seabed/ms.h"
#include "seabed/fs.h"
#include "SCMBuildStr.h"
#include "SCMVersHelp.h"
#ifdef _DEBUG
#include "Globals.h"
#endif  // _DEBUG

DEFINE_DOVERS(sqlci)

#if defined(_DEBUG)
#include "security/dsecure.h"
#define psecure_h_including_section
#define psecure_h_security_psb_set_
#define psecure_h_security_psb_get_
#include "security/psecure.h"
#define MAX_PRINTABLE_SID_LENGTH 256
#endif  //_DEBUG

#ifdef CLI_DLL
#include "SQLCLIdev.h" 
#endif

#include "EHException.h"
#include "MxciEHCallBack.h"

#ifdef _DEBUG_RTS
#include "ComQueue.h"
#include "Globals.h"
#include "SqlStats.h"
#include "dhalt.h"
#endif

#include "QRLogger.h"

extern void my_mpi_fclose();


  //    jmp_buf Buf;
THREAD_P jmp_buf ExportJmpBuf;
extern THREAD_P jmp_buf* ExportJmpBufPtr;

SqlciEnv * global_sqlci_env = NULL ; // Global sqlci_env for break key handling purposes.

// This processes options of the form:
//   <hyphen><single option char><zero or more blanks><required argument string>
// e.g.  "-ifilename"  "-i filename".
// An (English) error message is emitted for "-i" without an argument,
// "-" without an option letter, and "x" without a hyphen, but
// all other options pass thru with no error, as they may be an X option
// processed later in the InitializeX call.
//
// If the "-u name" option is specified, a copy of the name is
// returned in the user_name output parameter.
// The name is always uppercased regardless of whether it is enclosed
// in double quotes.
void processOption(Int32 argc, char *argv[], Int32 &i, 
		   const char *& in_filename,
		   const char *& input_string,
		   const char *& out_filename,
		   char *& sock_port,
                   NAString &user_name
		   )
{
  Int32 iorig = i;
  char op = '\0';
// const char *argAfterOp = NULL;
  char *argAfterOp = NULL;
  Int32 status;
  if (argv[i][0] == '-')
  {
    op = argv[i][1];
    if (op)
      if (argv[i][2])
	argAfterOp = &argv[i][2];
    else if (i < argc-1)
      argAfterOp = argv[++i];
  }

  if (argAfterOp)
  {
    switch (op)
    {
      case 's':	sock_port = argAfterOp;				// socket port number
			break;
      case 'i':	in_filename = argAfterOp;			// input file
			break;
      case 'o':	out_filename = argAfterOp;			// output file
		break;
      case 'q':	input_string = argAfterOp;			// input qry
		break;
//    case 'p':	sqlci->getUtil()->setProgramName(argAfterOp);	// utility name
//    		break;
//    case 't':	sqlci->getUtil()->setTerminalName(argAfterOp);	// utility debug
//		sqlci->getUtil()->setDebug(11);			// inspect flag
//		break;

      case 'u':
      {
        // The user_name output parameter will contain a copy of the
        // name
        user_name = argAfterOp;
        user_name.toUpper();

      } // case 'u'
      break;

      default:
	; // ok -- may be an X option, processed later in InitializeX

    } // switch (op)
  } // if (argAfterOp)

  else
  {
    // No argument appears after the option
    switch (op)
    {
      case 'v': printf("sqlci version: %s\n", SCMBuildStr);
        exit(0);
        break;

      case 'i':
      case 'p':
      case 't':
        cerr << argv[0] << ": " << argv[iorig]
             << " option requires an argument -- ignored" << endl;
        break;
        
      case '\0':
        cerr << argv[0] << ": " << argv[iorig]
             << " option is unknown -- ignored" << endl;
        break;
        
      default:
	; // ok -- may be an X option, processed later in InitializeX
        
    } // switch (op)
  } // if (argAfterOp) else ...
}

#ifdef _DEBUG_RTS
_callable void removeProcess()
{
  CliGlobals *cliGlobals = GetCliGlobals();
  StatsGlobals *statsGlobals = cliGlobals->getStatsGlobals();
  if (statsGlobals != NULL)
  {
    int error = statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
                cliGlobals->myPin());
    if (error == 0)
    {
      statsGlobals->removeProcess(cliGlobals->myPin());
      statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(), cliGlobals->myPin());
    }
    else if (error == 4011)
    {
      // BINSEM_LOCK_ timed out. Halt the CPU
      PROCESSOR_HALT_ (SQL_FS_INTERNAL_ERROR);
    }
  }
}
#endif

Int32 main (Int32 argc, char *argv[])
{
  dovers(argc, argv);

  // check this before file_init_attach overwrites the user env
  NABoolean sync_with_stdio = (getenv("NO_SYNC_WITH_STDIO") == NULL);

  try
  {
    file_init_attach(&argc, &argv, TRUE, (char *)"");
    msg_debug_hook("sqlci", "sqlci.hook");
    file_mon_process_startup2(true, false);
    atexit(my_mpi_fclose);
  }
  catch (...)
  {
    cerr << "Error while initializing messaging system. Please make sure Trafodion is started and up. Exiting..." << endl;
    exit(1);
  }

  if (sync_with_stdio)
    ios::sync_with_stdio();


  // Establish app user id from the current NT process user identity.
  // This must be done explicitly until the "shadow-process" mechanism
  // is fully implemented.  (It is done too late in cli/Context.cpp.)
  // FX: I'm not sure whether the following code applies
  // 	 to NT only.

 
  // process command line options
  char * in_filename = NULL;
  char * input_string = NULL;
  char * out_filename = NULL;
  char * sock_port = NULL;
  NAString user_name("");
  Int32 i = 1;
  for (; i < argc; i++)
    processOption(argc, argv, 
                           i, 
                          (const char *&)in_filename, 
                          (const char *&)input_string,
                          (const char *&)out_filename,
                          (char *&)sock_port,
                          user_name
      );

  if (sock_port) 
  {
  }  

  // create a SQLCI object
  SqlciEnv * sqlci = new SqlciEnv();
  global_sqlci_env = sqlci;

  if (user_name.length() > 0)
    sqlci->setUserNameFromCommandLine(user_name);


  if (setjmp(ExportJmpBuf))
  {

    printf("\nSQLCI terminating due to assertion failure");
    delete sqlci;
    exit(1); // NAExit(1);
  } 

  ExportJmpBufPtr = &ExportJmpBuf;

  if ((!in_filename) &&
      (out_filename))
    {
      sqlci->setNoBanner(TRUE);

      // create a logfile with the name out_filename.
      // Do not do that if an in_filename is specified. Users should
      // put the log command in the input file.
      char * logf = new char[strlen("LOG ") + 
			    strlen(out_filename) + 
			    strlen(" clear;") +
			    1];
      sprintf(logf, "LOG %s clear;", out_filename);
      sqlci->run(NULL, logf);
      delete logf;

      sqlci->get_logfile()->setNoDisplay(TRUE);
    }

  // setup log4cxx, need to be done here so initLog4cxx can have access to
  // process information since it is needed to compose the log name
  QRLogger::initLog4cxx(QRLogger::QRL_MXEXE);

  // run it -- this is where the action is!
  if (in_filename || input_string)
    sqlci->run(in_filename, input_string);
  else
    sqlci->run();
    
  if ((!in_filename) &&
      (out_filename))
    {
      sqlci->run(NULL, (char *)"LOG;");
    }

  // Now we are done, delete SQLCI object
  delete sqlci;
#ifdef _DEBUG_RTS
  removeProcess();
#endif
#ifdef _DEBUG
  // Delete all contexts
  GetCliGlobals()->deleteContexts();
#endif  // _DEBUG
  return 0;
}
