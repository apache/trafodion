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
 * File:         arkcmp.cpp
 * Description:  This is the main program for arkcmp process. The tasks for
 *               this process is to :
 *               . dynamic compile - process requests from executor
 *               . static compile - invoked from c89, compile Module definition
 *                 file into module file.
 *               . interface to SQLCAT - DDL.
 *
 * Created:      06/20/96
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */


#undef  NA_COMPILE_INSTANTIATE
#define NA_COMPILE_INSTANTIATE

#include "Platform.h"
#include "NewDel.h"
#include <fstream>
#include <string.h>

#include "seabed/ms.h"
#include "seabed/fs.h"
extern void my_mpi_fclose();
#include "SCMVersHelp.h"
DEFINE_DOVERS(tdm_arkcmp)


#include "CmpMessage.h"
#include "CmpConnection.h"
#include "CmpContext.h"
#include "CmpCommon.h"
#include "CmpStoredProc.h"
#include "CmpErrLog.h"
#include "CompException.h"
#include "cmpargs.h"
#include "logmxevent.h"
#include "QCache.h"
#include "QueryCacheSt.h"
#include "NATableSt.h"
#include "ComCextdecs.h"
#define CLI_DLL
#include "SQLCLIdev.h"
#undef CLI_DLL

#include "EHException.h"
#include "CmpEHCallBack.h"
#include "Globals.h"

#include "ObjectNames.h"



#include "CmpISPInterface.h"

#include "QRLogger.h"


THREAD_P jmp_buf ExportJmpBuf;

ostream &operator<<(ostream &dest, const ComDiagsArea& da);


extern CmpISPInterface cmpISPInterface;

// The following global is defined in CmpConnection.cpp
extern THREAD_P NABoolean CmpMainISPConnection;


// mainNewHandler_CharSave and mainNewHandler are used in the error
// handling when running out of virtual memory for the main program.
//
// Save 4K bytes of memory for the error handling when running out of VM.

static char* mainNewHandler_CharSave = new char[4096];

  static Int32  mainNewHandler(size_t s)
{
  if (mainNewHandler_CharSave)
  {
    delete[] mainNewHandler_CharSave;
    mainNewHandler_CharSave = NULL;
    CmpErrLog((char *)"Memory allocation failure");
    ArkcmpFatalError(ARKCMP_ERROR_PREFIX "Out of virtual memory.", NOMEM_SEV);
  }
  return 0;
}

static void longJumpHandler()
{
  // check if we have emergency memory for reporting error.
  if (mainNewHandler_CharSave == 0)
    exit(1);

  delete [] mainNewHandler_CharSave;
  mainNewHandler_CharSave = 0;
  ArkcmpFatalError(ARKCMP_ERROR_PREFIX "Assertion failed (thrown by longjmp).");
}
  
// this copy is used by remote tdm_arkcmp process
static jmp_buf CmpInternalErrorJmpBuf;

static void initializeArkcmp(Int32 argc, char** argv)
{
  if (setjmp(ExportJmpBuf)) 
    longJumpHandler();

  ExportJmpBufPtr = &ExportJmpBuf;

  
  if (setjmp(CmpInternalErrorJmpBuf))
    longJumpHandler();

  CmpInternalErrorJmpBufPtr = &CmpInternalErrorJmpBuf;
}

// Please do not remove this function. Otherwise a linkage
// error will occur.
void deinitializeArkcmp()
{
}


static ofstream* initializeArkcmpCoutCerr()
{
  // Initialize cout and cerr.

  ofstream* outstream = 0;
  char* streamName;
  char* NULLSTREAM = (char *)"/dev/null";
      streamName = NULLSTREAM;      
  outstream = new ofstream(streamName, ios::app);
  cout.rdbuf(outstream->rdbuf());
  cin.rdbuf(outstream->rdbuf());

  return outstream;
}



//extern void bloom_filter_test();
//extern void  hdfs_list_test(int argc, char **argv);

// argv[1] : <n> as in CHAR(<n>)
// argv[2]:  input file name, in which each line is a string 
//           at least <n> bytes long


Int32 main(Int32 argc, char **argv)
{


  dovers(argc, argv);
  try
  {
    file_init_attach(&argc, &argv, TRUE, (char *)"");
    file_mon_process_startup(true);
    msg_debug_hook("arkcmp", "ark.hook");
    atexit(my_mpi_fclose);
  }
  catch (...)
  {
    cerr << "Error while initializing messaging system. Exiting..." << endl;
    exit(1);
  }

  IdentifyMyself::SetMyName(I_AM_SQL_COMPILER);

  // Instantiate CliGlobals
  CliGlobals *cliGlobals = CliGlobals::createCliGlobals(FALSE);

  if (getenv("SQL_CMP_MSGBOX_PROCESS") != NULL )
     MessageBox(NULL, "Server: Process Launched", "tdm_arkcmp",
		MB_OK|MB_ICONINFORMATION);
  // The following call causes output messages to be displayed in the
  // same order on NSK and Windows.
  cout.sync_with_stdio();

  initializeArkcmp(argc, argv);
  QRLogger::initLog4cxx(QRLogger::QRL_MXCMP);

  cmpISPInterface.InitISPFuncs();

  // we wish stmt heap could be passed to this constructor,
  // but stmt heap is not yet created at this point in time.
  Cmdline_Args cmdlineArgs;
  cmdlineArgs.processArgs(argc, argv);

  try
  {

    { // a local ctor scope, within a try block
      CmpIpcEnvironment ipcEnv;
      ExCmpMessage exCmpMessage(&ipcEnv);
      ipcEnv.initControl(cmdlineArgs.allocMethod(),
			 cmdlineArgs.socketArg(),
			 cmdlineArgs.portArg());
      exCmpMessage.addRecipient
        (ipcEnv.getControlConnection()->getConnection());

      CmpMessageConnectionType connectionType;
      // to receive the Connection Type information
      exCmpMessage.clearAllObjects();
      exCmpMessage.receive();

      // Set up the context info for the connection, it contains the variables
      // persistent through each statement loops.


      // Check for env var that indicates this is a secondary mxcmp process.
      // Keep this value in CmpContext so that secondary mxcmp on NSK
      // will allow MP DDL issued from a parent mxcmp process.
      NABoolean IsSecondaryMxcmp = FALSE;
      /*if (getenv("SECONDARY_MXCMP") != NULL)
	IsSecondaryMxcmp = TRUE;
      else
        // Any downstream process will be a "SECONDARY_MXCMP".
        putenv("SECONDARY_MXCMP=1");*/
      
      CmpContext *context=NULL;
      NAHeap *parentHeap = GetCliGlobals()->getCurrContextHeap();
      NAHeap *cmpContextHeap = new (parentHeap)
                      NAHeap((char *)"Cmp Context Heap",
                           parentHeap,
                           (Lng32)524288);
      try
      {
        context= new (cmpContextHeap) CmpContext
          (CmpContext::IS_DYNAMIC_SQL |
           (IsSecondaryMxcmp ? CmpContext::IS_SECONDARY_MXCMP : 0) |
           (CmpMainISPConnection ? CmpContext::IS_ISP : 0) |
           (CmpContext::IS_MXCMP),
           cmpContextHeap);

      }
      catch (...)
      {
        ArkcmpErrorMessageBox
          (ARKCMP_ERROR_PREFIX "- Cannot initialize Compiler global data.", 
           ERROR_SEV, FALSE, FALSE, TRUE);
        exit(1);
      }

      //  moved down the IdentifyMyself so that it can be determined that the
      //  context has not yet been set up
      //IdentifyMyself::SetMyName(I_AM_SQL_COMPILER);
      context->initContextGlobals();
      context->envs()->cleanup();
      // ## This dump-diags only goes to arkcmp console --
      // ## we really need to reply to sqlci (or whoever is requester)
      // ## with these diags!  (Work to be done in Ipc and CmpConnection...)
      NADumpDiags(cerr, CmpCommon::diags(), TRUE/*newline*/);

      assert(cmpCurrentContext == context);
      exCmpMessage.setCmpContext(context);


      while (!exCmpMessage.end())
	{     
	  // Clear the SQL text buffer in the event logging area.
	  cmpCurrentContext->resetLogmxEventSqlText();

	  exCmpMessage.clearAllObjects();
	  exCmpMessage.receive();
#ifdef NA_LINUX_SETENV
	  //SQ_TBD
	  putenv("SQLMX_MODULE_DIR=../sqlmx/USERMODULES/");
	  putenv("SQLMX_SYSMODULE_DIR=../sqlmx/SYSTEMMODULES/");
#endif
	}
      // in most (probably all?) cases, an mxci-spawned mxcmp calls NAExit
      // in CmpConnection.cpp. So, everything below this line is probably
      // dead code.
      CURRENTQCACHE->finalize((char *)"Dynamic (non-NAExit case) ");
    }
  }
  catch(BaseException& bE)
    {
      char msg[500];
      sprintf(msg, "%s BaseException: %s %d", ARKCMP_ERROR_PREFIX,
              bE.getFileName(), bE.getLineNum());
      ArkcmpFatalError(msg);
  }
  catch(...)
    {
      ArkcmpFatalError(ARKCMP_ERROR_PREFIX "Fatal exception.");
    }

   ENDTRANSACTION();


  return 0;  
}

// stubs
Int32 arkcmp_main_entry()
{ // return 1 for embedded cmpiler not created
  return 1;
}
// no-op
void arkcmp_main_exit() {}


