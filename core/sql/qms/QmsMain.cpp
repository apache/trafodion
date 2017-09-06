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

#include "Platform.h"
#include <Int64.h>
#include "QmsRequest.h"
#include "QRMessage.h"
#include "XMLUtil.h"
#include "QRDescriptor.h"
#include "QRLogger.h"
#include "QmsInitializer.h"
#include "QueryRewriteServer.h"
#include "QRIpc.h"

#include "nsk/nskport.h"
#include "seabed/ms.h"
#include "seabed/fs.h"
extern void my_mpi_fclose();
#include "SCMVersHelp.h"
DEFINE_DOVERS(tdm_arkqms)

/**
 * \file
 * Contains the main() function for the qms executable, which processes the
 * input file presented as a command-line argument, or, when launched by QMM as
 * a server process, enters a loop in which it waits for requests from client
 * processes (either QMM or MXCMP). Requests are processed using a number of
 * static, nonmember functions defined in this file, as well as classes derived
 * from QRRequest.
 */

// The code below must be executed before the IPC function that spawns the qms
// process will return to qmm, so we do it before initializing qms so the
// various qms processes aren't initialized serially.
static QmsGuaReceiveControlConnection* initializeIPC(IpcEnvironment*& env)
{
  MvQueryRewriteServer::setHeap(NULL);
  env = MvQueryRewriteServer::getIpcEnv();
  // Have to allocate this from heap, because ~IpcEnvironment deletes it.
  QmsGuaReceiveControlConnection* conn = new QmsGuaReceiveControlConnection(env);
  env->setControlConnection(conn);
  while (!conn->getConnection())
    conn->wait(IpcInfiniteTimeout);
  return conn;
}

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
    msg_debug_hook("tdm_arkqms", "tdm_arkqms.hook");
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

  NABoolean performSMDInit = FALSE;
  NABoolean performXMLInit = FALSE;

  // Uncomment to allow time to attach debugger before initialization. The
  // MessageBox thing below uses an env var and won't work unless QMS is run
  // from a command-line in the shell that defines the env var.
  //Sleep(30000);

  QRLogger::instance().setModule(QRLogger::QRL_QMS);
  QRLogger::instance().initLog4cxx("log4cxx.qms.config");
 
  QRLogger::log(CAT_QMS_MAIN, LL_INFO, "=================================================");
  QRLogger::log(CAT_QMS_MAIN, LL_INFO, "=================================================");
  QRLogger::log(CAT_QMS_MAIN, LL_INFO, "=================================================");
  QRLogger::log(CAT_QMS_MAIN, LL_INFO, "QMS process was started.");
  QRLogger::log(CAT_QMS_MAIN, LL_INFO, "QMS invoked with %d arguments.", argc);
  for (Int32 i=0; i<argc; i++)
    QRLogger::log(CAT_QMS_MAIN, LL_DEBUG, "  argument %d = %s", i, argv[i]);

  IpcEnvironment* env = NULL;
  QmsGuaReceiveControlConnection* conn = NULL;

  // If invoked via the message interface, -oss will be the program
  // parameter after the program name.
  NABoolean commandLineInterface = (strncmp(argv[1], "-guardian", 9) != 0);

  // For message interface, initialize IPC so it will return to qmm from the
  // process creation code before we begin qms initialization.
  if (!commandLineInterface)
  {
    conn = initializeIPC(env);
    QRLogger::log(CAT_QMS_MAIN, LL_DEBUG, "QMS invoked via messaging interface.");
  }
  else
    QRLogger::log(CAT_QMS_MAIN, LL_DEBUG, "QMS invoked via command-line interface.");

  // Create the singleton instances of Qms and QmsInitializer.
  NAHeap qmsHeap("QMS Heap", NAMemory::DERIVED_FROM_SYS_HEAP, (Lng32)131072);
  Qms& qms = *Qms::getInstance(&qmsHeap);
  QmsInitializer& qmsInitializer = *QmsInitializer::getInstance(&qms);

  // If the command line interface was used, batch process the set of requests
  // specified in the input file.
  if (commandLineInterface)
  {
    try
    {
      return QRCommandLineRequest::processCommandLine(argc, argv);
    }
    catch (QRException e)
    {
      // Ignore exceptions for now.
      QRLogger::log(CAT_QMS_MAIN, LL_ERROR, "QMS aborted.");
      return -1;
    }
  }

  cout << "Waiting for messages..." << endl;

  while (TRUE)
    {
      conn->wait();
      //env->getAllConnections()->waitOnAll(IpcInfiniteTimeout, FALSE);
    }

} // End of mainline
