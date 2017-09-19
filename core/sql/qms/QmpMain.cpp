// **********************************************************************
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
// **********************************************************************

#include <ComCextdecs.h>
#include <Int64.h>
#include "QRSharedPtr.h"
#include "QRLogger.h"
#include "QmsMain.h"
#include "QRDescriptor.h"
#include "QRQueriesImpl.h"
#include "QueryRewriteServer.h"
#include "QmpPublish.h"
#include "NAType.h"

#include "nsk/nskport.h"
#include "seabed/ms.h"
#include "seabed/fs.h"
extern void my_mpi_fclose();
#include "SCMVersHelp.h"
DEFINE_DOVERS(tdm_arkqmp)

/**
 * \file
 * Contains the main() function for the Query Publisher Process, QMP, executable.
 * QMP is started by the Query Matching Monitor, QMM, process.
 * QMP reads MVQR-related defaults from the SYSTEM DEFAULTS table and
 * sends them to the QMM process.
 * QMP also reads from the MANAGEABILITY.MV_REWRITE.REWRITE_PUBLISH table.
 * Rows are read and PUBLISH requests are sent to the QMM process.
 * The QMM process then sends them to the Query Matching Server, QMS, processes in its queue.
 */

using namespace QR;

// The default is to publish to QMM.
PublishTarget publishTarget = PUBLISH_TO_QMM;

// Output file when PUBLISH_TO_FILE.
const char* targetFilename = NULL;

// This is needed to avoid a link error.
// LCOV_EXCL_START :ale
NABoolean NAType::isComparable(const NAType &other,
			       ItemExpr *parentOp,
			       Int32 emitErr) const
{ return FALSE; }
// LCOV_EXCL_STOP

void usage(char *progName)
{
  cerr << "Usage: " << progName << " -target {QMM | QMS | FILE <filename>}" << endl;
}

/**
 * No known input command-line arguments at this time
 *
 * @param argc Number of arguments on the command line.
 * @param argv None at this time.
 * @return TRUE if command line parsed correctly, FALSE if error.
 */
static NABoolean processCommandLine(Int32 argc, char *argv[])
{
  // If no command line arguments are provided, the default is to publish to QMM.
  // -oss is unconditionally added by IpcGuardianServer::spawnProcess() when qmm
  // uses it to create qmp.

  if (argc == 1 || argc == 2 && !stricmp(argv[1], "-oss"))
    return TRUE;

  if (argc < 3 || stricmp(argv[1], "-target"))
  {
    usage(argv[0]);
    return FALSE;
  }

  if (!stricmp(argv[2], "QMM"))
    publishTarget = PUBLISH_TO_QMM;
  else if (!stricmp(argv[2], "QMS"))
    publishTarget = PUBLISH_TO_QMS;
  else if (!stricmp(argv[2], "FILE"))
  {
    if (argc < 4)
    {
      usage(argv[0]);
      return FALSE;
    }

    publishTarget = PUBLISH_TO_FILE;
    targetFilename = argv[3];
  }

  return TRUE;
} // End of processCommandLine

extern "C"
{
Int32 sq_fs_dllmain();
}


Int32 main(Int32 argc, char *argv[])
{
  dovers(argc, argv);

  try
  {
    file_init_attach(&argc, &argv, TRUE, (char *)"");
    sq_fs_dllmain();
    msg_debug_hook("tdm_arkqmp", "tdm_arkqmp.hook");
    file_mon_process_startup(true);
    atexit(my_mpi_fclose);
  }
  // LCOV_EXCL_START :rfi
  catch (...)
  {
    cerr << "Error while initializing messaging system. Exiting..." << endl;
    exit(1);
  }
  // LCOV_EXCL_STOP

  NAHeap qmpHeap("QMP Heap", NAMemory::DERIVED_FROM_SYS_HEAP, (Lng32)131072);

  QmpPublish qmpPublisher(&qmpHeap);

  // Establish the IPC heap and cache the IpcEnvironment ptr from
  // MvQueryRewriteServer.
  MvQueryRewriteServer::setHeap(&qmpHeap);
  qmpPublisher.setIpcEnvironment(MvQueryRewriteServer::getIpcEnv());

  Lng32 result = 0;
  
  QRLogger::instance().setModule(QRLogger::QRL_QMP);
  QRLogger::instance().initLog4cxx("log4cxx.qmp.config");

  QRLogger::log(CAT_QMP, LL_INFO, "MXQMP process was started.");

  char dateBuffer[30];  // Standard format is: YYYY-MM-DD HH:MM:SS.MMM.MMM
  MvQueryRewriteServer::getFormattedTimestamp(dateBuffer);
  //logMessage("\n\n\n================================================================================");
  QRLogger::log(CAT_QMP, LL_INFO,
    "Command-line QMP invoked with %d arguments.", argc);
  //for (int i=0; i<argc; i++)
  //  logMessage2("Program argument %d is %s", i, argv[i]);
   
#ifdef NA_WINNT
  if (getenv("QMP_MSGBOX_PROCESS") != NULL)
  {
    MessageBox( NULL, "QMP Process Launched", (CHAR *)argv[0], MB_OK|MB_ICONINFORMATION );
  };
#endif
    
  // Process any command-line arguments.
  if (processCommandLine(argc, argv) == FALSE)
  {
    // LCOV_EXCL_START :rfi
    QRLogger::log(CAT_QMP, LL_ERROR,
      "QMP processing of command-line arguments failed.");
    return -1;
    // LCOV_EXCL_STOP
  }

  if (qmpPublisher.setTarget(publishTarget, targetFilename) == FALSE)
  {
    // LCOV_EXCL_START :rfi
    QRLogger::log(CAT_QMP, LL_ERROR,
      "QMP opening publish target failed.");
    return -1;
    // LCOV_EXCL_STOP
  }

  // Process the REWRITE_PUBLISH table reading from the stream
  do 
  {
    try
    {
      //result = 
      qmpPublisher.performRewritePublishReading();
      QRLogger::log(CAT_QMP, LL_DEBUG,
        "QMP REWRITE_TABLE reading completed with result code - %d", result);
    }
    // LCOV_EXCL_START :rfi
    catch(...)
    {
      // Handle database errors here.
      QRLogger::log(CAT_QMP, LL_ERROR, "Unknown exception - exiting.");
      exit(0);
    }
    // LCOV_EXCL_STOP

    // Delay waiting to try again for a successful SQL table read
    // Wait 3 minutes
    if (publishTarget != PUBLISH_TO_FILE)
      DELAY(18000);
  } while (publishTarget != PUBLISH_TO_FILE);

  QRLogger::log(CAT_QMP, LL_INFO, "MXQMP process has terminated.");
  return result;
}  // End of mainline

