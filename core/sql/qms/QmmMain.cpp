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

#include "QueryRewriteServer.h"
#include "QmmQmm.h"
#include "QRSharedPtr.h"
#include "QRLogger.h"
#include "NAType.h"

#include "nsk/nskport.h"
#include "seabed/ms.h"
#include "seabed/fs.h"
extern void my_mpi_fclose();
#include "SCMVersHelp.h"
DEFINE_DOVERS(tdm_arkqmm)

/**
 * \file
 * Contains the main() function for the Query Matching Monitor (QMM) executable.
 * The initial instance of QMM is a Guardian persistent process (the "head QMM"),
 * which starts the remaining QMMs and the Query Matching Publisher (QMP). The 
 * QMP determines the number of QMSs for each QMM to start and passes this info
 * to the head QMM, which forwards it to the other QMMs. The QMMs create their
 * respective pools of QMS processes, spreading them across CPUs to enhance
 * parallelism. There is one QMM for a given number of CPUs (divisible by the
 * size of a segment).
 *
 * QMMs have published names, which allows MXCMPs to contact them if they are
 * unable to use their local QMS and need to be allocated one.
 */

using namespace QR;

#define XML_BUFF_SIZE 32768
static NAHeap qmmHeap("QMM Heap",
                      NAMemory::DERIVED_FROM_SYS_HEAP,
                      (Lng32)131072);

/**
 * Reads command-line arguments passed to QMM. Returns the selected values for
 * the cpu to start QMP on, how or if it is to be started, and whether to listen
 * for events with receive() or waitOnAll(). Defaults should be assigned to these
 * variables before calling; they are not set if the corresponding argument does
 * not appear on the command line.
 *
 * @param argc Number of arguments on the command line.
 * @param argv Array of arguments.
 * @param [out] qmpCpu The cpu number to start the QMP process on.
 * @param [out] startOpt Indicator of how QMP is to be started, if at all.
 * @param [out] listenOpt Whether to doreceive() or waitOnAll().
 */
static void processCommandLine(Int32 argc, char *argv[],
                               short& qmpCpu, StartOpt& startOpt,
                               ListenOpt& listenOpt)
{
  Int32 currArg = 1;
  while (currArg < argc)
  {
    if (!stricmp(argv[currArg], "-qmpcpu"))
      {
        currArg++;
        assertLogAndThrow(CAT_QMM, LL_ERROR,
                          currArg < argc, QmmException,
                          "Cpu number did not follow \"-qmpcpu\"");
        qmpCpu = atoi(argv[currArg]);
        if (qmpCpu < 0 || qmpCpu > CPUS_PER_SEGMENT - 1)
          qmpCpu = IPC_CPU_DONT_CARE;
        currArg++;
      }
    else if (!stricmp(argv[currArg], "-start"))
      {
        currArg++;
        assertLogAndThrow(CAT_QMM, LL_ERROR,
                          currArg < argc, QmmException,
                          "Start option did not follow \"-start\"");
        if (!stricmp(argv[currArg], "spawn"))
          startOpt = SPAWN;
        else if (!stricmp(argv[currArg], "server"))
          startOpt = SERVER;
        else if (!stricmp(argv[currArg], "none"))
          startOpt = NONE;
        else
          assertLogAndThrow1(CAT_QMM, LL_ERROR,
                             FALSE, QmmException,
                             "Invalid value for \"-start\" parameter -- %s",
                             argv[currArg])
        currArg++;
      }
    else if (!stricmp(argv[currArg], "-listen"))
      {
        currArg++;
        assertLogAndThrow(CAT_QMM, LL_ERROR,
                          currArg < argc, QmmException,
                          "Start option did not follow \"-listen\"");
        if (!stricmp(argv[currArg], "receive"))
          listenOpt = RECEIVE;
        else if (!stricmp(argv[currArg], "waitonall"))
          listenOpt = WAITONALL;
        else if (!stricmp(argv[currArg], "waitcc"))
          listenOpt = WAITCC;
        else
          assertLogAndThrow1(CAT_QMM, LL_ERROR,
                             FALSE, QmmException,
                             "Invalid value for \"-listen\" parameter -- %s",
                             argv[currArg])
        currArg++;
      }
    else
      assertLogAndThrow1(CAT_QMM, LL_ERROR,
                         FALSE, QmmException,
                         "Unrecognized parameter -- %s", argv[currArg])
  }
} // processCommandLine

// This is needed to avoid a link error.
NABoolean NAType::isComparable(const NAType &other,
                               ItemExpr *parentOp,
                               Int32 emitErr) const
{ return FALSE; }

static short getDefaultQmpCpu(const IpcEnvironment* ipcEnv)
{
  short qmpCpu;

  // Default QMP location is the same cpu QMM is running on.
  SB_Phandle_Type procHandle;
  Int32 lc_cpu;
  XPROCESSHANDLE_GETMINE_(&procHandle);
  short error = XPROCESSHANDLE_DECOMPOSE_ (&procHandle, &lc_cpu);
  qmpCpu = lc_cpu;

  if (error)
    {
      QRLogger::log(CAT_QMM, LL_ERROR,
                               "XPROCESSHANDLE_DECOMPOSE_ returned error %d", error);
      qmpCpu = 3;  // best-guess default
    }

  return qmpCpu;
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
    msg_debug_hook("tdm_arkqmm", "tdm_arkqmm.hook");
    file_mon_process_startup(true);
    atexit(my_mpi_fclose);
  }
  catch (...)
  {
    cerr << "Error while initializing messaging system. Exiting..." << endl;
    exit(1);
  }

  Lng32 result = 0;

  QRLogger::instance().setModule(QRLogger::QRL_QMM);
  QRLogger::instance().initLog4cxx("log4cxx.qmm.config");

  QRLogger::log(CAT_QMM, LL_INFO,
    "Command-line QMM invoked with %d arguments.", argc);
  //for (int i=0; i<argc; i++)
  //  debugMessage2("Program argument %d is %s", i, argv[i]);
   
  Qmm* qmm = NULL;
  try
    {
      qmm = Qmm::getInstance(&qmmHeap);

      // Get default startup arguments. Must do this before processing command
      // line args, which may override these values.
      StartOpt startOpt = SPAWN;
      ListenOpt listenOpt = WAITCC;
      short qmpCpu = getDefaultQmpCpu(qmm->getEnvironment());

      processCommandLine(argc, argv, qmpCpu, startOpt, listenOpt);
      qmm->setListenOpt(listenOpt);
      qmm->setQmpStartOpt(startOpt);
      #ifndef NA_WINNT
      qmm->allocateQmsPool();
      qmm->checkAndRetryQms();
      #else
      qmm->allocateQms();
      #endif
      qmm->startQmp(qmpCpu);
    }
  catch (QmmException& ex)
    {
      QRLogger::log(CAT_QMM, LL_ERROR,
        "QMM initialization failed: %s", ex.getMessage());
      return -1;
    }
  catch (...)
    {
      QRLogger::log(CAT_QMM, LL_ERROR,
        "QMM initialization failed: unknown exception occurred");
      return -1;
    }

  if (qmm)
    {
      QRLogger::log(CAT_QMM, LL_INFO,
        "QMM initialization complete, waiting for messages...");
      qmm->executeMessageLoop();
    }
  else
    QRLogger::log(CAT_QMM, LL_ERROR,
      "QMM exiting; creation of instance failed, no exception thrown.");

}  // main

