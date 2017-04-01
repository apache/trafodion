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
* File:         udrserv.cpp
* Description:  This is the main program for UDR Server process. The tasks for
*               this process are to :
*               . Handle messages to and from the Master Executor
*               . process SP requests from executor
*               . process result set requests from the executor/CLI
*
* Created:      01/01/2001
* Language:     C++
*
*****************************************************************************
*/

#include "Platform.h"
#include <fstream>
#include <iostream>


#include "NAMemory.h"
#include "ComTransInfo.h"
#include "udrserv.h"
#include "UdrStreams.h"
  #include "ComDiags.h"
#include "udrdecs.h"
#include "ErrorMessage.h"
#include "UdrFFDC.h"
#include "UdrDebug.h"
#include "UdrAbortCallBack.h"
#include "LmJavaOptions.h"
#include "UdrCfgParser.h"
#include "ComRtUtils.h"
#include "LmLangManagerJava.h"
#include "LmLangManagerC.h"
#include "LmRoutine.h"
#include "ComDefs.h"
#include "sqludr.h"
#include "NAUserId.h"

#include "SCMVersHelp.h"
DEFINE_DOVERS(tdm_udrserv)
#include "dtm/tm.h"


#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

  #include <unistd.h>
  #define GETPID getpid

#include "Measure.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef UDR_OSS_DEBUG
  #include <signal.h>
#endif

#ifdef UDR_DEBUG
#include "HeapLog.h"
#endif

// Global pointer to the UdrGlobals instance
UdrGlobals *UDR_GLOBALS = NULL;

extern THREAD_P SQLEXPORT_LIB_FUNC jmp_buf ExportJmpBuf;
extern THREAD_P SQLEXPORT_LIB_FUNC jmp_buf* ExportJmpBufPtr;

void processASessionMessage(UdrGlobals *UdrGlob,
                            UdrServerReplyStream &msgStream,
                            UdrSessionMsg &request);
void processAnEnterTxMessage(UdrGlobals *UdrGlob,
                             UdrServerReplyStream &msgStream,
                             UdrEnterTxMsg &request);
void processASuspendTxMessage(UdrGlobals *UdrGlob,
                              UdrServerReplyStream &msgStream,
                              UdrSuspendTxMsg &request);
void processAnExitTxMessage(UdrGlobals *UdrGlob,
                            UdrServerReplyStream &msgStream,
                            UdrExitTxMsg &request);
void processALoadMessage(UdrGlobals *UdrGlob,
                         UdrServerReplyStream &msgStream,
                         UdrLoadMsg &request,
                         IpcEnvironment &env);
void processAnUnLoadMessage(UdrGlobals *UdrGlob,
                            UdrServerReplyStream &msgStream,
                            UdrUnloadMsg &request);
void processAnInvokeMessage(UdrGlobals *UdrGlob,
                            UdrServerDataStream &msgStream,
                            UdrDataBuffer &request);
void processAnRSLoadMessage(UdrGlobals *UdrGlob,
                            UdrServerReplyStream &msgStream,
                            UdrRSLoadMsg &request);
void processAnRSCloseMessage(UdrGlobals *UdrGlob,
                             UdrServerReplyStream &msgStream,
                             UdrRSCloseMsg &request);
void processAnRSUnloadMessage(UdrGlobals *UdrGlob,
                              UdrServerReplyStream &msgStream,
                              UdrRSUnloadMsg &request);

NABoolean processCmdLine(UdrGlobals *UdrGlob, Int32 argc, char **argv);


// static long getClientId(UdrGlobals *udrGlob);


void processARequest(UdrGlobals *UdrGlob,
                     UdrServerReplyStream &msgStream,
                     IpcEnvironment &env);

static void verifyUdrServer(UdrGlobals &glob);

static const char *LmResultToString(const LmResult &r);

static void DumpDiags(ostream &stream, ComDiagsArea *d, const char *prefix);
static void DumpProcessInfo();
static ComDiagsArea *addOrCreateErrorDiags(UdrGlobals *UdrGlob,
                                           Lng32 errorNumber,
                                           Lng32 intErrorInfo,
                                           const char *charErrorInfo,
                                           ComDiagsArea *diags);

//
// Function to invoke a Java method using the Language Manager
// (LM) API. This function supports command-line LM method invocations
// and is never called when MXUDR runs as a server process.
//
static const char *MXUDR_PREFIX = "[MXUDR]";
static const char *MXUDR_PREFIX_PLUS_SPACE = "[MXUDR] ";
static FILE *MXUDR_OUTFILE = stdout; // TODO: fix when stderr available
static Int32 invokeUdrMethod(const char *method,
                           const char *container,
                           const char *path,
                           NABoolean isJava,
                           NABoolean isUdf,
                           NABoolean txRequired,
                           NABoolean useVarchar,
                           Int32 argc,
                           char *argv[],
                           Int32 nResultSets,
                           Int32 nTimesToInvoke,
                           UdrGlobals &glob);

// LCOV_EXCL_START
// Dead Code
// These methods are not used, and the interface has not been tested for a long time.
// We might want to retire them
static Int32 processCommandsFromFile(const char *filename, UdrGlobals &glob);
static Int32 processSingleCommandFromFile(FILE *f, UdrGlobals &glob);
// LCOV_EXCL_STOP

// Changed the default to 512 to limit java heap size used by SQL processes.
// Keep this define in sync with executor/JavaObjectInterface.cpp
#define DEFAULT_JVM_MAX_HEAP_SIZE 512
#define DEFAULT_COMPRESSED_CLASSSPACE_SIZE 128
#define DEFAULT_MAX_METASPACE_SIZE 128

static NAString initErrText("");
/*************************************************************************
   Helper function to propagate all Java-related environment settings
   found in an optional configuration file into an LmJavaOptions instance.

   File must be in location indicated by envvar TRAFUDRCFG, or if not found, 
   use default of $TRAF_HOME/conf/trafodion.udr.config
*************************************************************************/
void readCfgFileSection ( const char *section, LmJavaOptions &javaOptions )
{
   char *p = NULL;
   char buffer [BUFFMAX+1];

   while ((UdrCfgParser::readSection(section, buffer, sizeof(buffer), initErrText )) > 0)
   {
      UDR_DEBUG2("[%s] entry: %s", section, buffer);

      if (strcmp(section, "java") == 0)
      {
         javaOptions.addOption(buffer, TRUE);
      }
      else 
         if (strcmp(section, "env") == 0)
         {
            if ((p = UdrCfgParser::textPos( buffer, "CLASSPATH" )))
            {
               javaOptions.addSystemProperty("java.class.path", p);
            }
            else 
               if ((p = UdrCfgParser::textPos( buffer, "JREHOME" )))
               {
                  javaOptions.addSystemProperty("sqlmx.udr.jrehome", p);
               }
            putenv(buffer);
         }
         else
         {
            break;
         }
   }  //while
}

void InitializeJavaOptionsFromEnvironment(LmJavaOptions &javaOptions
                                         ,NAMemory *heap
                                         )
{
   char *val;

   if ((val = getenv("CLASSPATH")))
   {
      javaOptions.addSystemProperty("java.class.path", val);
   }
   char maxHeapOption[64];
   int maxHeapEnvvarMB = DEFAULT_JVM_MAX_HEAP_SIZE;
   sprintf(maxHeapOption, "-Xmx%dm", maxHeapEnvvarMB);
   javaOptions.addOption((const char *)maxHeapOption, TRUE);

   char compressedClassSpaceSizeOptions[64];
   int compressedClassSpaceSize = 0;
   const char *compressedClassSpaceSizeStr = getenv("JVM_COMPRESSED_CLASS_SPACE_SIZE");
   if (compressedClassSpaceSizeStr)
      compressedClassSpaceSize = atoi(compressedClassSpaceSizeStr);
   if (compressedClassSpaceSize <= 0)
      compressedClassSpaceSize = DEFAULT_COMPRESSED_CLASSSPACE_SIZE;
   sprintf(compressedClassSpaceSizeOptions, "-XX:CompressedClassSpaceSize=%dm", compressedClassSpaceSize);
   javaOptions.addOption((const char *)compressedClassSpaceSizeOptions, TRUE);

   char maxMetaspaceSizeOptions[64];
   int maxMetaspaceSize = 0;
   const char *maxMetaspaceSizeStr = getenv("JVM_MAX_METASPACE_SIZE");
   if (maxMetaspaceSizeStr)
      maxMetaspaceSize = atoi(maxMetaspaceSizeStr);
   if (maxMetaspaceSize <= 0)
      maxMetaspaceSize = DEFAULT_MAX_METASPACE_SIZE;
   sprintf(maxMetaspaceSizeOptions, "-XX:MaxMetaspaceSize=%dm", maxMetaspaceSize);
   javaOptions.addOption((const char *)maxMetaspaceSizeOptions, TRUE);

   /* Look for java startup options and envvars in configuration file */
   if (UdrCfgParser::cfgFileIsOpen(initErrText))
   {
      readCfgFileSection("env", javaOptions);
      readCfgFileSection("java", javaOptions);
      UdrCfgParser::closeCfgFile();
   }

   if ((val = getenv("TRAF_UDR_JAVA_OPTIONS")))
   {
      const char *delimiters = " \t";
      bool newlineIsPresent = (strchr(val, '\n') ? true : false);
      if (newlineIsPresent)
      {
         delimiters = "\n";
      }
      javaOptions.addOptions(val, delimiters, TRUE);
   }
}

/*************************************************************************
// Helper function to change =_DEFAULTS define value.
// MXOSRVR gets the =_DAFAULTS value that contains the primary node name.
// This causes problem if the MXOSRVR is on secondary nodes. gethostbyname()
// tries to open TCPIP process on primary node instead of secondary node.
//
// This method changes the system name in the =_DEFAULTS define
// to current node name.
*************************************************************************/

//-----------------------------------------------------------------


static void runServer(Int32 argc, char **argv);

Int32 main(Int32 argc, char **argv)
{
  dovers(argc, argv);
  file_debug_hook("udrserv", "udrserv.hook");
  try {
    file_init_attach(&argc, &argv, TRUE, (char*) "");
  }
  catch (SB_Fatal_Excep &e) {
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, e.what(), 0);
    exit(1);
  }
  try {
    file_mon_process_startup(true);
  }
  catch (SB_Fatal_Excep &e) {
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, e.what(), 0);
    exit(1);
  }
  // Commented out the call to my_mpi_fclose() below as
  // the function UdrExitHandler(in file UdrFFDC.cpp) takes care of
  // calling my_mpi_fclose() so we do not need to register that 
  // function here as an exit handler.
  // atexit(my_mpi_fclose);

  LOG_AGENT

  UdrAbortCallBack udrACB;
  registerAbortCallBack(&udrACB);
  UDR_DEBUG_SIGNAL_HANDLERS("[MXUDR] The original signal handlers");
  setUdrSignalHandlers();
  saveUdrTrapSignalHandlers();
  UDR_DEBUG_SIGNAL_HANDLERS("[MXUDR] Saved UDR trap signal handlers");
  setExitHandler();
  UDR_DEBUG0("[MXUDR] Registered Exit handler");

 
  // setup log4cxx, need to be done here so initLog4cxx can have access to
  // process information since it is needed to compose the log name
  // the log4cxx log name for this master and all its subordinates will be
  // based on this process' node number and its pid
  QRLogger::instance().setModule(QRLogger::QRL_UDR);
  QRLogger::instance().initLog4cxx("log4cxx.trafodion.udr.config");

  // Synchronize C and C++ output streams
  ios::sync_with_stdio();

  // Redirect stdout and stderr to files named in environment
  // variables
  const char *stdOutFile = getenv("SQLMX_UDR_STDOUT");
  const char *stdErrFile = getenv("SQLMX_UDR_STDERR");
  Int32 fdOut = -1;
  Int32 fdErr = -1;
  
  if (stdOutFile && stdOutFile[0])
  {
    fdOut = open(stdOutFile,
                 O_WRONLY | O_APPEND | O_CREAT | O_SYNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fdOut >= 0)
    {
      fprintf(stdout, "[Redirecting MXUDR stdout to %s]\n", stdOutFile);
      fflush(stdout);
      dup2(fdOut, fileno(stdout));
    }
    else
    {
      fprintf(stdout, "*** WARNING: could not open %s for redirection: %s.\n",
              stdOutFile, strerror(errno));
    }
  }

  if (stdErrFile && stdErrFile[0])
  {
    fdErr = open(stdErrFile,
                 O_WRONLY | O_APPEND | O_CREAT | O_SYNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fdErr >= 0)
    {
      fprintf(stdout, "[Redirecting MXUDR stderr to %s]\n", stdErrFile);
      fflush(stdout);
      dup2(fdErr, fileno(stderr));
    }
    else
    {
      fprintf(stdout, "*** WARNING: could not open %s for redirection: %s.\n",
              stdErrFile, strerror(errno));
    }
  }


  runServer(argc, argv);

  if (fdOut >= 0)
  {
    close(fdOut);
  }
  if (fdErr >= 0)
  {
    close(fdErr);
  }

  return 0;
}

static void runServer(Int32 argc, char **argv)
{
#ifdef NA_DEBUG_C_RUNTIME
  UDR_DEBUG1("Process ID: %ld", (Lng32) GETPID());
  UDR_DEBUG0("[BEGIN argv]");
  Int32 i;
  for (i = 0; i < argc; i++)
  {
    if (argv[i])
      UDR_DEBUG1("  '%s'", argv[i]);
    else
      UDR_DEBUG0("  NULL");
  }
  UDR_DEBUG0("[END argv]");
  DumpProcessInfo();
#endif

  if (setjmp(ExportJmpBuf))
  {
    UDR_ASSERT(0, "An ExportJmpBuf longjmp occurred.");
  }

  ExportJmpBufPtr = &ExportJmpBuf;

  const char *moduleName = "runServer";

  NABoolean showMsgBox = FALSE;
  static Int32 pid = getpid();
  char stmp[255];
  sprintf(stmp,"Process Launched %d", pid);

  if (getenv("SQL_MSGBOX_PROCESS") != NULL)
  {
    MessageBox(NULL, stmp , "tdm_udrserv", MB_OK|MB_ICONINFORMATION);
    showMsgBox = TRUE;
  }

#ifdef NA_DEBUG_C_RUNTIME
  putenv((char *)"MXUDR_DEBUG_BUILD=1");
#endif

#ifdef UDR_OSS_DEBUG
  // enable more strict heap checking for the debug build, so
  // that we catch more bugs.
  if (getenv("DBG_HEAP_CHECK") != NULL) 
    heap_check_always(1);
#endif

#ifndef NDEBUG
  if (getenv("DEBUG_SERVER") != NULL)
  {
    DebugBreak();
  }
#endif // NDEBUG


  UdrTraceFile = stdout;
  
  // We now create two heaps. One is the "IPC" heap and is intended
  // for objects that only persist while we are processing the current
  // request. The other is the "UDR" heap and is intended for global
  // objects or anything that must not go away after we stop
  // processing the current request.
  NAHeap *udrHeap = new NAHeap("UDR Global Heap",
                               NAMemory::DERIVED_FROM_SYS_HEAP,
                               256 * 1024 // 256K block size
                               );

  udrHeap->setJmpBuf(&UdrHeapLongJmpTgt);
  Int32 udrJmpRc = setjmp(UdrHeapLongJmpTgt);
  if (udrJmpRc)
     UDR_ABORT("udrHeap allocation failed.");

  NAHeap *ipcHeap = new NAHeap("UDR IPC Heap",
                               NAMemory::DERIVED_FROM_SYS_HEAP,
                               256 * 1024 // 256K block size
                               );

#ifdef UDR_DEBUG
  if (getenv("SQLMX_UDR_MEMORY_LOG"))
    HeapLogRoot::control(LOG_START);
#endif

  ipcHeap->setJmpBuf(&IpcHeapLongJmpTgt);
  Int32 ipcJmpRc = setjmp(IpcHeapLongJmpTgt);
  if (ipcJmpRc)
     UDR_ABORT("ipcHeap allocation failed.");

  UDR_GLOBALS = new (udrHeap) UdrGlobals(udrHeap, ipcHeap);

  // Move environment settings into the global LmJavaOptions object
  InitializeJavaOptionsFromEnvironment(*(UDR_GLOBALS->getJavaOptions()),
                                       udrHeap);

  if (processCmdLine(UDR_GLOBALS, argc, argv))
  {
    return;
  }

  GuaReceiveControlConnection *ctrlConn = UDR_GLOBALS->getControlConnection();
  IpcEnvironment *env = UDR_GLOBALS->getIpcEnvironment();

  // Wait for the first open message
  ctrlConn->waitForMaster();

  doMessageBox(UDR_GLOBALS, TRACE_SHOW_DIALOGS,
	       UDR_GLOBALS->showMain_, "GotConnection");

  NABoolean doTrace = (UDR_GLOBALS->verbose_ &&
                       UDR_GLOBALS->traceLevel_ >= TRACE_IPMS &&
                       UDR_GLOBALS->showMain_) ? TRUE : FALSE;

  while (1)
  {
    if (env->getAllConnections()->entries() > 0)
    {
      NAList<UdrServerReplyStream *> &replyStreams =
        UDR_GLOBALS->getReplyStreams();

      while(replyStreams.entries())
      {
        UdrServerReplyStream *stream = replyStreams[0];
        replyStreams.removeAt(0);

#ifdef NA_DEBUG_C_RUNTIME
          Lng32 crashPoint = 0;
#endif
        if (stream->moreObjects())
        {
          UdrIpcObjectType msgType =
           (UdrIpcObjectType) stream->getNextObjType();

#ifdef NA_DEBUG_C_RUNTIME
          // Bring the process down if an environment variable value
          // matches the message type
          const char *crashString = getenv("MXUDR_CRASH_POINT");
          if (crashString && crashString[0])
            crashPoint = atol(crashString);
          else
            crashPoint = 0;

          if (crashPoint == (Lng32) msgType)
          {
            ServerDebug("  CRASH POINT is %ld (%s)", crashPoint,
                        GetUdrIpcTypeString((UdrIpcObjectType) crashPoint));
            ServerDebug("  MXUDR about to exit");
            exit(1);
          }
#endif
          // Do some real work on the incoming message
          processARequest(UDR_GLOBALS, *stream, *env);

#ifdef NA_DEBUG_C_RUNTIME
          // Bring the process down if the environment variable checked
          // earlier has a negative value and its positive value matches
          // the message type
          if (crashPoint && (-crashPoint == (Lng32) msgType))
          {
            if (crashPoint == -UDR_MSG_DATA_HEADER ||
                crashPoint == -UDR_MSG_CONTINUE_REQUEST ||
                crashPoint == -UDR_MSG_RS_DATA_HEADER ||
                crashPoint == -UDR_MSG_RS_CONTINUE)
            {
              // For data requests, let's wait for 5 sec before crashing
              Sleep(5 * 1000); // 5 seconds in millisec
            }

            ServerDebug("  CRASH POINT is %ld (%s)", crashPoint,
                        GetUdrIpcTypeString((UdrIpcObjectType) -crashPoint));
            ServerDebug("  MXUDR about to exit");
            exit(1);
          }
#endif
        }

        // All messages in the stream are processed
        stream->addToCompletedList();
      }

      // All work is completed, now wait on all connections.
      if (doTrace)
        ServerDebug("[UdrServ (%s)] Wait for next message\n", moduleName);

      env->deleteCompletedMessages();
      // This change is made because QCD6 changes with Gil's code
      // checked in 9/05/26, all UDR tests were failing. 
      // The UDR server was crashing in waitOnSet() in IPc.cpp file
      // when it was making system call WAIT.
      // So this is the work around for time being. Is is required
      // to change other places after consulting Gil when he
      // comes back from vacation. 
      // env->getAllConnections()->waitOnAll(IpcInfiniteTimeout);
      ctrlConn->wait(IpcInfiniteTimeout);

    } // if (env->getAllConnections()->entries() > 0)
  } // while (1)

}  // runServer

void processARequest(UdrGlobals *UdrGlob,
                     UdrServerReplyStream &msgStream,
                     IpcEnvironment &env)
{
  const char *moduleName = "processARequest";
  IpcMessageType msgType;
  IpcMessageObjVersion msgVer;

  doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
               UdrGlob->showMain_, moduleName);

  UdrGlob->objectCount_++;
  UdrGlob->numReqSP_++;
  UdrGlob->numReqUDR_++;
  UdrGlob->requestCount_++;

  msgType = msgStream.getNextObjType();
  msgVer = msgStream.getNextObjVersion();
  UdrGlob->currentMsgSize_ = msgStream.getNextObjSize();

  if (UdrGlob->verbose_ &&
      UdrGlob->traceLevel_ >= TRACE_IPMS  &&
      UdrGlob->showMain_)
  {
    ServerDebug("[UdrServ (%s)] Processing Request# %ld",
                moduleName,
                UdrGlob->objectCount_);
    ServerDebug("         Msg Type: %ld (%s)",
                 (Lng32) msgType,
                 GetUdrIpcTypeString(UdrIpcObjectType(msgType)));
    ServerDebug("         Msg Version: %ld", (Lng32) msgVer);
  }

   if (!IsNAStringSpaceOrEmpty(initErrText))
   {
      if (msgType == UDR_MSG_LOAD)
      {
         controlErrorReply(UdrGlob,
            msgStream,
            UDR_ERR_MESSAGE_PROCESSING,    //errorNumber
            0,                             //intErrorInfo,
            initErrText.data()             //charErrorInfo
            );
         return;
      }
   }

  //
  // Each case in the following switch statement is for a different
  // message type.
  //
  // For control messages we do the following:
  // - Some bookkeeping in the UdrGlobals object
  // - Extract the incoming message from the message stream
  // - Call a global function to process the request and return a reply
  //
  // For data messages our work is different. We have to route the
  // incoming message to the appropriate buffered stream. The routing
  // involves using the UdrHandle from the incoming message to perform
  // a lookup into the global SPInfo list.
  //
  switch (msgType)
  {
    case UDR_MSG_SESSION:
    {
      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "Session Type");

      UdrSessionMsg &request = *(new (UdrGlob->getIpcHeap())
                                 UdrSessionMsg(UdrGlob->getIpcHeap()));

      msgStream >> request;

      processASessionMessage(UdrGlob, msgStream, request);

      request.decrRefCount();
    }
    break;

    case UDR_MSG_LOAD:
    {
      UdrGlob->numReqLoadSP_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "Load Type");

      UdrLoadMsg &request = *(new (UdrGlob->getIpcHeap())
                              UdrLoadMsg(UdrGlob->getIpcHeap()));

      msgStream >> request;

      processALoadMessage(UdrGlob, msgStream, request, env);

      request.decrRefCount();
    }
    break;

    case UDR_MSG_UNLOAD:
    {
      UdrGlob->numReqUnloadSP_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "Unload Type");

      UdrUnloadMsg &request = *(new (UdrGlob->getIpcHeap())
                                UdrUnloadMsg(UdrGlob->getIpcHeap()));
      msgStream >> request;

      processAnUnLoadMessage(UdrGlob, msgStream, request);

      request.decrRefCount();

#ifdef UDR_DEBUG
      if (getenv("SQLMX_UDR_MEMORY_LOG"))
      {
        printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
        printf("Memory usage after processing request: %s \n",
	       	GetUdrIpcTypeString(UdrIpcObjectType(msgType)));
        printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
        HeapLogRoot::display(FALSE);
      }
#endif

    }
    break;

    case UDR_MSG_DATA_HEADER:
    {
      UdrGlob->numReqDataSP_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "Invoke Type");

      UdrDataHeader h(INVALID_UDR_HANDLE, NULL);
      msgStream >> h;

      SPInfo *sp = UdrGlob->getSPList()->spFind(h.getHandle());
      if (sp)
      {
        UdrServerDataStream *other = sp->getDataStream();
        msgStream.routeMessage(*other);
      }
      else
      {
        UdrGlob->numErrDataSP_++;
        controlErrorReply(UdrGlob,
                          msgStream,
                          UDR_ERR_MISSING_UDRHANDLE,
                          0,
                          "Data header"
                          );
      }

    } // case UDR_MSG_DATA_HEADER
    break;

    case UDR_MSG_TMUDF_DATA_HEADER:
    {
      UdrGlob->numReqDataSP_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "TMUDF Request");

      UdrTmudfDataHeader h(INVALID_UDR_HANDLE, NULL);
      msgStream >> h;

      SPInfo *sp = UdrGlob->getSPList()->spFind(h.getHandle());
      if (sp)
      {
        UdrServerDataStream *other = sp->getDataStream();
        msgStream.routeMessage(*other);
      }
      else
      {
        UdrGlob->numErrRSFetch_++;
        controlErrorReply(UdrGlob,
                          msgStream,
                          UDR_ERR_MISSING_UDRHANDLE,
                          0,
                          "TMUDF Data header"
                          );
      }

    } // case UDR_MSG_TMUDF_DATA_HEADER
    break;

    case UDR_MSG_CONTINUE_REQUEST:
    {
      UdrGlob->numReqContinueSP_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "Continue Type");

      UdrContinueMsg r(INVALID_UDR_HANDLE, NULL);
      msgStream >> r;

      SPInfo *sp = UdrGlob->getSPList()->spFind(r.getHandle());
      if (sp)
      {
        UdrServerDataStream *other = sp->getDataStream();
        msgStream.routeMessage(*other);
      }
      else
      {
        UdrGlob->numErrContinueSP_++;
        controlErrorReply (UdrGlob,
                           msgStream,
                           UDR_ERR_MISSING_UDRHANDLE,
                           0,
                           "Continue request"
                           );
      }

    } // case UDR_MSG_CONTINUE_REQUEST
    break;

    case UDR_MSG_RS_LOAD:
    {
      UdrGlob->numReqRSLoad_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "RS Load Type");

      UdrRSLoadMsg &request = *(new (UdrGlob->getIpcHeap())
                                UdrRSLoadMsg(UdrGlob->getIpcHeap()));
      msgStream >> request;

      processAnRSLoadMessage(UdrGlob, msgStream, request);
      request.decrRefCount();
    }
    break;

    case UDR_MSG_RS_DATA_HEADER:
    {
      UdrGlob->numReqRSFetch_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "RS Fetch Type");

      UdrRSDataHeader h(INVALID_UDR_HANDLE, NULL);
      msgStream >> h;

      SPInfo *sp = UdrGlob->getSPList()->spFind(h.getHandle());
      if (sp)
      {
        UdrServerDataStream *other = sp->getDataStream();
        msgStream.routeMessage(*other);
      }
      else
      {
        UdrGlob->numErrRSFetch_++;
        controlErrorReply(UdrGlob,
                          msgStream,
                          UDR_ERR_MISSING_UDRHANDLE,
                          0,
                          "RS Data header"
                          );
      }

    } // case UDR_MSG_RS_DATA_HEADER
    break;

    case UDR_MSG_RS_CONTINUE:
    {
      UdrGlob->numReqRSContinue_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "RS Continue Type");

      UdrRSContinueMsg &request = *(new (UdrGlob->getIpcHeap())
                                    UdrRSContinueMsg(UdrGlob->getIpcHeap()));
      msgStream >> request;

      SPInfo *sp = UdrGlob->getSPList()->spFind(request.getHandle());
      if (sp)
      {
        UdrServerDataStream *other = sp->getDataStream();
        msgStream.routeMessage(* other);
      }
      else
      {
        UdrGlob->numErrRSContinue_++;
        controlErrorReply(UdrGlob,
                          msgStream,
                          UDR_ERR_MISSING_UDRHANDLE,
                          0,
                          "RS Continue"
                          );
      }

      request.decrRefCount();
    }
    break;

    case UDR_MSG_RS_CLOSE:
    {
      UdrGlob->numReqRSClose_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "RS Close Type");

      UdrRSCloseMsg &request = *(new (UdrGlob->getIpcHeap())
                                 UdrRSCloseMsg(UdrGlob->getIpcHeap()));
      msgStream >> request;

      processAnRSCloseMessage(UdrGlob, msgStream, request);
      request.decrRefCount();

    }
    break;

    case UDR_MSG_RS_UNLOAD:
    {
      UdrGlob->numReqRSUnload_++;

      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "RS Unload Type");

      UdrRSUnloadMsg &request = *(new (UdrGlob->getIpcHeap())
                                  UdrRSUnloadMsg(UdrGlob->getIpcHeap()));
      msgStream >> request;

      processAnRSUnloadMessage(UdrGlob, msgStream, request);

      request.decrRefCount();
    }
    break;

    case UDR_MSG_ENTER_TX:
    {
      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "ENTER TX Type");

      UdrEnterTxMsg &request = *(new (UdrGlob->getIpcHeap())
                                 UdrEnterTxMsg(UdrGlob->getIpcHeap()));

      msgStream >> request;

      processAnEnterTxMessage(UdrGlob, msgStream, request);

      request.decrRefCount();
    } // case UDR_MSG_ENTER_TX
    break;

    case UDR_MSG_SUSPEND_TX:
    {
      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "SUSPEND TX Type");
      
      UdrSuspendTxMsg &request = *(new (UdrGlob->getIpcHeap())
                                   UdrSuspendTxMsg(UdrGlob->getIpcHeap()));
      
      msgStream >> request;
      
      processASuspendTxMessage(UdrGlob, msgStream, request);
      
      request.decrRefCount();
    } // case UDR_MSG_SUSPEND_TX
    break;

    case UDR_MSG_EXIT_TX:
    {
      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showMain_, "EXIT TX Type");

      UdrExitTxMsg &request = *(new (UdrGlob->getIpcHeap())
                                 UdrExitTxMsg(UdrGlob->getIpcHeap()));

      msgStream >> request;

      processAnExitTxMessage(UdrGlob, msgStream, request);

      request.decrRefCount();
    } // case UDR_MSG_EXIT_TX
    break;

    default:
    {
      UdrGlob->numErrUDR_++;
      controlErrorReply(UdrGlob,
                        msgStream,
                        UDR_ERR_UNKNOWN_MSG_TYPE,
                        (Lng32) msgType,
                        NULL);

    } // default
    break;

  } // switch (msgType)

  // reset currSP_ 
  UdrGlob->setCurrSP (NULL);

}  // processARequest()

// If param 'sp' is set, we check if we have to quiesce executor.
// If it's NULL, we do not do any attempt to quiesce executor. So it's
// preferable to set it to NULL, if the caller knows quiesce is not needed.
void sendControlReply(UdrGlobals *UdrGlob,
                      UdrServerReplyStream &msgStream,
                      SPInfo *sp)
{
  const char *moduleName = "sendControlReply";

  doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
               UdrGlob->showMain_, moduleName);

  NABoolean doTrace = (UdrGlob->verbose_ &&
                       UdrGlob->traceLevel_ >= TRACE_IPMS &&
                       UdrGlob->showMain_) ? TRUE : FALSE;

  UdrGlob->replyCount_++;

  if (doTrace)
    ServerDebug("[UdrServ (%s)] Sending reply# %d to client",
                moduleName,
                UdrGlob->replyCount_);

  if (sp)
  {
    sp->prepareToReply(msgStream);
  }
  else
  {
    // We are replying to a request that does not require an SPInfo.
    // We assume that we are already in the correct transaction related
    // to this request (and there is no problem even if we are in different
    // transaction) and these requests do not need executor to be quiesced.

    // Do nothing.
  }

  NABoolean waited = TRUE;
  msgStream.send(waited);

  if (doTrace)
  {
    CollIndex i = 0;
    msgStream.getRecipients().setToNext(i);
    IpcConnection *conn = msgStream.getRecipients().element(i);

    ServerDebug("        Sent reply %ld to client on connection 0x%08X",
                UdrGlob->replyCount_,
                conn);
    ServerDebug("[UdrServ (%s)] Done", moduleName);
  }
}  // sendControlReply()

// If param 'sp' is set, we check if we have to quiesce executor.
// If it's NULL, we do not do any attempt to quiesce executor. So it's
// preferable to set it to NULL, if the caller knows quiesce is not needed.
void sendDataReply(UdrGlobals *UdrGlob,
                   UdrServerDataStream &msgStream,
                   SPInfo *sp)
{
  const char *moduleName = "sendDataReply";

  NABoolean doTrace = (UdrGlob->verbose_ &&
                       UdrGlob->traceLevel_ >= TRACE_IPMS &&
                       UdrGlob->showMain_) ? TRUE : FALSE;

  if (UdrGlob->traceLevel_ >= TRACE_SHOW_DIALOGS && UdrGlob->showMain_)
  {
    MessageBox(NULL, moduleName , UdrGlob->serverName_,
               MB_OK | MB_ICONINFORMATION);
  }


  UdrGlob->replyCount_++;

  if (doTrace)
    ServerDebug("[UdrServ (%s)] Sending reply# %d to client",
                moduleName,
                UdrGlob->replyCount_);

  if (sp)
  {
    sp->prepareToReply(msgStream);
  }
  else
  {
    // We are replying to a request that does not require an SPInfo.
    // We assume that we are already in the correct transaction related
    // to this request (and there is no problem even if we are in different
    // transaction) and these requests do not need executor to be quiesced.

    // Do nothing.
  }

  msgStream.sendResponse();

  // We used to have the following code. But after having many
  // connections in UDR server, the following check may not be
  // correct. Also, there is no easy way of getting connection
  // pointer from buffered stream. Btw, this may not be necessary.
  // The regressions are working fine without this 'wait' loop.
  // One day we can remove this completely once we are sure that
  // we are not breaking nowait apps. If we have problems with nowait
  // apps, we will have to fix this.
  //
  // With buffered data streams we need to make sure all
  // response buffers have been sent before returning from this
  // function. There is no "waited" send method on the buffered
  // stream class so we implement the "wait" here.
  //
  // IpcConnection *conn = env.getControlConnection()->getConnection();
  // while (conn && conn->sendIOPending())
  // {
  //   conn->wait(IpcInfiniteTimeout);
  // }

  if (doTrace)
  {
    ServerDebug("        Sent reply %ld to client", UdrGlob->replyCount_);
    ServerDebug("[UdrServ (%s)] Done", moduleName);
  }

} // sendDataReply()

// LCOV_EXCL_START
static void displayUsageInfo()
{
  fprintf(stdout, "Usage:\n"); 
  fprintf(stdout, "  mxudr -help\n");
  fprintf(stdout, "    Prints this help information.\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "  mxudr -verify\n");
  fprintf(stdout, "    Verify UDR Server and Language Manager startup.\n");
  fprintf(stdout, "\n");

#ifdef NA_DEBUG_C_RUNTIME
  // The "-invoke" and "-obey" options are supported in the release
  // build but we do not tell users about them in this help message.
  fprintf(stdout, "  mxudr -invoke [-n N] [-param java|c] [-rs numRS]\n");
  fprintf(stdout, "        [-noTx] [-vc] <method> <file> <path> [args]\n");
  fprintf(stdout, "    Invoke a method using the Language Manager\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "    -n N : N is number of times to invoke <method>.\n");
  fprintf(stdout, "           Default is 1.\n");
  fprintf(stdout, "    -param java|c : Parameter style. Default is Java.\n");
  fprintf(stdout, "    -rs numRS : Number of ResultSet parameters. For.\n");
  fprintf(stdout, "                Java only. Default is 0.\n");
  fprintf(stdout, "    -noTx : <method> is executed without a transaction.\n");
  fprintf(stdout, "    -vc : Parameters are considered VARCHAR.\n");
  fprintf(stdout, "          If not specified, parameters are CHAR.\n");
  fprintf(stdout, "    <method> : Name of Java method or C function.\n");
  fprintf(stdout, "    <file> : Name of Java class file or C DLL.\n");
  fprintf(stdout, "    <path> : Location of <file>.\n");
  fprintf(stdout, "    [args] : Arguments separated by spaces.\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "  mxudr -obey <filename>\n");
  fprintf(stdout, "    Processes commands from a text file <filename>\n");
  fprintf(stdout, "\n");
#endif

  fprintf(stdout, "\n");
}
// LCOV_EXCL_STOP

NABoolean processCmdLine(UdrGlobals *UdrGlob, Int32 argc, char **argv)
{
  const char *moduleName = "processCmdLine";

  doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
               UdrGlob->showMain_, moduleName);

  char tmp1[255];

   if (!IsNAStringSpaceOrEmpty(initErrText) )
   {
      if (!stricmp(argv[1], "-obey") || !stricmp(argv[1], "-help") 
      || !stricmp(argv[1], "-invoke") || !stricmp(argv[1], "-verify") ) 
      {
         ComDiagsArea *initDiags = addOrCreateErrorDiags(UdrGlob,
                                                    UDR_ERR_MESSAGE_PROCESSING,
                                                    0,
                                                    initErrText.data(),    
                                                    NULL);
         DumpDiags(cout, initDiags, MXUDR_PREFIX_PLUS_SPACE);
         return TRUE;
      }
   }

  //
  // Process environmental variables...
  //
  UdrGlob->verbose_ = FALSE;
  UdrGlob->traceLevel_ = 0;
  UdrGlob->logFileProvided_ = FALSE;


// LCOV_EXCL_START
  // NSK or Linux can use environment variables...
  if (const char *logFileName = getenv("UDRSERV_TRACE_FILENAME"))
  {
    UdrTraceFile = fopen(logFileName, "a");
    if (UdrTraceFile == NULL)
    {
      UdrTraceFile = stdout;
      sprintf(tmp1, "    Log File: Std Out - Rejected: %.60s", logFileName);
    }
    else
    {
      UdrGlob->logFileProvided_ = TRUE;
      sprintf(tmp1, "    Log File: %.60s", logFileName);
    }
  }
  else
  {
    UdrTraceFile = stdout;
    sprintf(tmp1, "\tLog File: Std Out");
  }



  if (UdrGlob->verbose_)
  {
    ServerDebug("[UdrServ (%.30s)] Runtime Parameters:", moduleName);
    ServerDebug(tmp1);

    ServerDebug("    Trace Level %d", UdrGlob->traceLevel_);
    ServerDebug("    Verbose Mode Active");
  }

  if (UdrGlob->showInvoke_)
    ServerDebug("    Trace Invoke Module Active");

  if (UdrGlob->showLoad_)
    ServerDebug("    Trace Load Module Active");

  if (UdrGlob->showMain_)
    ServerDebug("    Trace Main Module Active");

  if (UdrGlob->showSPInfo_)
    ServerDebug("    Trace SPInfo Module Active");

  if (UdrGlob->showUnload_)
    ServerDebug("    Trace Unload Module Active");
// LCOV_EXCL_STOP

  //
  // Now we parse command-line arguments. We support the following
  // command-line operations:
  //
  //  -help          Display a usage message
  //
  //  -verify        Load the Language Manager and exit
  //
  //  -invoke        Call a UDR method via the Language Manager
  //
  //  -obey          Process commands from a text file
  //

  for (Int32 i = 1; i < argc; i++)
  {
    if (!stricmp(argv[i], "-help"))
    {
      UdrGlob->setCommandLineMode(TRUE);
      displayUsageInfo();
      exit(1);
    }
    else if (!stricmp(argv[i], "-verify"))
    {
      UdrGlob->setCommandLineMode(TRUE);
      verifyUdrServer(*UdrGlob);
      exit(0);
    }

    // See if an LM method invocation is being requested. Syntax for
    // LM method invocations is shown in the displayUsageInfo()
    // function.
    else if (!stricmp(argv[i], "-invoke"))
    {
      UdrGlob->setCommandLineMode(TRUE);

      Int32 nTimesToInvoke = 1;
      Int32 nResultSets = 0;
      NABoolean usageError = FALSE;
      const char *paramStyle = "java";
      NABoolean txRequired = TRUE;
      NABoolean isUdf = FALSE;
      NABoolean useVarchar = FALSE;

      // This while loop scans for invoke options that begin with a
      // dash.
      while (!usageError && ((i + 1) < argc))
      {
        if (argv[i + 1][0] == '-')
        {
          i++;
          if (!stricmp(argv[i], "-n"))
          {
            if ((i + 1) < argc)
            {
              i++;
              nTimesToInvoke = atoi(argv[i]);
            }
            else
            {
              usageError = TRUE;
            }
          }
          else if (!stricmp(argv[i], "-rs"))
          {
            if ((i + 1) < argc)
            {
              i++;
              nResultSets = atoi(argv[i]);
            }
            else
            {
              usageError = TRUE;
            }
          }
	  else if (!stricmp(argv[i], "-param"))
	  {
            if ((i+1) < argc)
	    {
              i++;
              paramStyle = argv[i];
	    }
	    else
	    {
              usageError = TRUE;
	    }
	  }
          else if (!stricmp(argv[i], "-notx"))
          {
            txRequired = FALSE;
          }
          else if (!stricmp(argv[i], "-vc"))
          {
            useVarchar = TRUE;
          }
          else if (!stricmp(argv[i], "-udf"))
          {
            isUdf = TRUE;
          }
          else
          {
            usageError = TRUE;
          }
        }
	else
	{
          break;
	}
      } // while (!usageError && ((i + 1) < argc))

      // We must have at least 3 more arguments for method, container,
      // and path
      if (argc <= (i + 3))
      {
        usageError = TRUE;
      }

      // We expect param styles "c" and "java"
      if (stricmp(paramStyle, "c") == 0)
      {
        // Do not manage a transaction for C routines
        txRequired = FALSE;
      }
      else if (stricmp(paramStyle, "java") == 0)
      {
        // Nothing to do
      }
      else
      {
        usageError = TRUE;
      }

      if (usageError)
      {
        displayUsageInfo();
        exit(1);
      }

      const char *method = argv[++i];
      const char *container = argv[++i];
      const char *path = argv[++i];
      Int32 methodArgc = argc - (i + 1);
      char **methodArgv = NULL;
      if (methodArgc > 0)
      {
        methodArgv = &(argv[i + 1]);
      }

      NABoolean isJava = (stricmp(paramStyle, "java") == 0 ? TRUE : FALSE);

      // If the user is invoking a C routine, assume it is a UDF not a
      // stored procedure
      if (!isJava)
        isUdf = TRUE;

      Int32 result = invokeUdrMethod(method, container, path,
                                   isJava, isUdf, txRequired, useVarchar,
                                   methodArgc, methodArgv, nResultSets,
                                   nTimesToInvoke, *UdrGlob);
      
      exit(result);

    } // -invoke

    // See if the user wants us to obey commands from a text file
    else if (!stricmp(argv[i], "-obey"))
    {
      // LCOV_EXCL_START
      // Dead Code
      // obey option is not tested, and Andy thinks it should be obsoleted.
      UdrGlob->setCommandLineMode(TRUE);

      NABoolean usageError = FALSE;
      
      if ((i + 1) >= argc)
      {
        usageError = TRUE;
      }

      if (usageError)
      {
        displayUsageInfo();
        exit(1);
      }

      i++;

      Int32 result = processCommandsFromFile(argv[i], *UdrGlob);
      exit(result);

      // LCOV_EXCL_STOP
    } // -obey

  } // for each arg in argv

  return FALSE;

} // processCmdLine

#if 0
//
// $$$$ getClientId() is disabled for now. It is returning errors
// after certain system messages arrive on $RECEIVE. It needs to be
// redesigned. When it gets redesigned, it's not clear whether a
// certain technique is valid. The UDR server layer (i.e. this source
// file) calls receive() on an IpcMessageStream object and following
// receive() the UDR server layer calls FILE_GETRECEIVEINFO_ to see
// which process sent the last request. I don't think the UDR server
// layer should be querying Guardian for info about $RECEIVE. The IPC
// layer is managing $RECEIVE and the UDR server probably should't
// assume that the next message being provided by the IPC layer is the
// most recent arrival on $RECEIVE. The system message problem is one
// example. The IPC layer can have data to present to the UDR server
// layer but that data is not the most recent $RECEIVE arrival. The
// most recent $RECEIVE arrival might have been a system message that
// is of no concern to the UDR server, e.g. a remote CPU went down.
//
Lng32 getClientId(UdrGlobals *UdrGlob)
{
  // Returns either 0 OK or UDR_ERR_TOO_MANY_OPENERS

  const char *moduleName = "getClientId";
  GuaErrorNumber guaErrorInfo;
  GuaProcessHandle openPhandle;

  doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
               UdrGlob->showMain_, moduleName);

  // get current clients process ID
  guaErrorInfo = FILE_GETRECEIVEINFO_((short *) &receiveInfo);

  if (guaErrorInfo != GuaOK)
  {
    UDR_ASSERT(0, "Fatal error in FILE_GETRECEIVEINFO_");
  }

  openPhandle = receiveInfo.phandle_;
  short retcode = GETTRANSID(UdrGlob->transId_);

  // Locate ID in openerProcessId list
  for (Int32 i=0; i<UdrGlob->gNumOpeners_; i++)
  {
    if (openPhandle == UdrGlob->gOpenerPhandle_[i])
    {
      return 0;
    }
  }

  // Not found, add to list
  if (UdrGlob->gNumOpeners_ < UDRMAXOPENERS_V100)
  {
    // Add to list
    UdrGlob->gOpenerPhandle_[UdrGlob->gNumOpeners_++] = openPhandle;
    return 0;
  }

  // List is full
  return UDR_ERR_TOO_MANY_OPENERS;

} // getClientId
#endif

//
// Helper functions to return a UdrErrorReply object followed by a SQL
// diagnostics area on a UDR stream. The diags area will contain a
// single condition. The intErrorInfo and charErrorInfo parameters are
// optional information items for a particular condition. For example,
// the "invalid handle" condition requires one additional string item
// so the caller of this function should pass a valid charErrorInfo
// pointer when returning that condition.
//
// Code outside this file will use the global functions
// controlErrorReply() and dataErrorReply() to send UdrErrorReply
// objects.
//
static ComDiagsArea *addOrCreateErrorDiags(UdrGlobals *UdrGlob,
                                           Lng32 errorNumber,
                                           Lng32 intErrorInfo,
                                           const char *charErrorInfo,
                                           ComDiagsArea *diags)
{
  if (!diags)
  {
    diags = ComDiagsArea::allocate(UdrGlob->getIpcHeap());
  }

  if (diags && errorNumber != 0)
  {
    switch (errorNumber)
    {
      case UDR_ERR_UNKNOWN_MSG_TYPE:
      case UDR_ERR_TOO_MANY_OPENERS:
        *diags << DgSqlCode(-errorNumber);
        *diags << DgInt0((Int32)intErrorInfo);
        break;

      case UDR_ERR_MISSING_UDRHANDLE:
      case UDR_ERR_MISSING_LMROUTINE:
      case UDR_ERR_MISSING_RSHANDLE:
        *diags << DgSqlCode(-errorNumber);
        if (charErrorInfo)
        {
          *diags << DgString0(charErrorInfo);
        }
        break;

      case UDR_ERR_INVALID_RS_INDEX:
        *diags << DgSqlCode(-errorNumber)
               << DgString0(charErrorInfo)
               << DgInt0((Int32) intErrorInfo);
        break;

      case UDR_ERR_INVALID_RS_STATE:
        *diags << DgSqlCode(-errorNumber);
        *diags << DgString0(charErrorInfo);
        break;

      default:
      {
        *diags << DgSqlCode(-UDR_ERR_MESSAGE_PROCESSING);
        switch (intErrorInfo)
        {
          case INVOKE_ERR_NO_REQUEST_BUFFER:
            *diags << DgString0("Invoke: no request buffer.");
            break;
          case INVOKE_ERR_NO_INPUT_ROW:
            *diags << DgString0("Invoke: no input row.");
            break;
          case INVOKE_ERR_NO_REPLY_DATA_BUFFER:
            *diags << DgString0("Invoke: allocate reply buffer failed.");
            break;
          case INVOKE_ERR_NO_REPLY_ROW:
            *diags << DgString0("Invoke: allocate reply row failed.");
            break;
          case INVOKE_ERR_NO_ERROR_ROW:
            *diags << DgString0("Invoke: allocate error row failed.");
            break;
          case INVOKE_ERR_NO_REPLY_BUFFER:
            *diags << DgString0("Allocate reply buffer failed.");
            break;
          case RS_ERR_NO_REPLY_MSG:
            *diags << DgString0("Allocate RS reply failed.");
            break;
          case RS_ERR_NO_REPLY_DATA_BUFFER:
            *diags << DgString0("Allocate RS reply data buffer failed.");
            break;
          default:
            if (charErrorInfo)
               *diags << DgString0(charErrorInfo);
            else
               *diags << DgString0("");
            break;
        } // switch (intErrorInfo)

      } // default
        break;

    } // switch (errorNumber)

  } // if (diags)

  return diags;
}

static void doErrorDebugging(UdrGlobals *UdrGlob,
                             const char *moduleName,
                             Lng32 errorNumber, Lng32 intErrorInfo,
                             const char *charErrorInfo)
{
  /*
  char errorText[MAXERRTEXT];
  sprintf(errorText, "(%.30s) UDR Error Reply: %d, %d",
          moduleName, errorNumber, intErrorInfo);
  ServerDebug("[UdrServ (%s)]  %s\n", moduleName, errorText);
  doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
               UdrGlob->showMain_, errorText);
  */

  if (UdrGlob->verbose_ &&
      UdrGlob->traceLevel_ >= TRACE_IPMS &&
      UdrGlob->showMain_)
  {
    ServerDebug("[UdrServ (%s)] send error reply", moduleName);
  }

}

void controlErrorReply(UdrGlobals *UdrGlob,
                       UdrServerReplyStream &msgStream,
                       Lng32 errorNumber,
                       Lng32 intErrorInfo,
                       const char *charErrorInfo)
{
  const char *moduleName = "controlErrorReply";
  doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS, UdrGlob->showMain_, moduleName);

  UdrErrorReply *reply =
    new (UdrGlob->getIpcHeap()) UdrErrorReply(UdrGlob->getIpcHeap());

  msgStream.clearAllObjects();
  msgStream << *reply;
  reply->decrRefCount();

  ComDiagsArea *diags = addOrCreateErrorDiags(UdrGlob,
                                              errorNumber,
                                              intErrorInfo,
                                              charErrorInfo,
                                              NULL);
  if (diags)
  {
    msgStream << *diags;
    diags->decrRefCount();
  }

  doErrorDebugging(UdrGlob, moduleName,
                   errorNumber, intErrorInfo, charErrorInfo);

  sendControlReply(UdrGlob, msgStream, NULL);
  UdrGlob->numErrSP_++;

}  // controlErrorReply

void dataErrorReply(UdrGlobals *UdrGlob,
                    UdrServerDataStream &msgStream,
                    Lng32 errorNumber,
                    Lng32 intErrorInfo,
                    const char *charErrorInfo,
                    ComDiagsArea *diags)
{
  const char *moduleName = "dataErrorReply";
  doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS, UdrGlob->showMain_, moduleName);

  // Free up any receive buffers no longer in use
  msgStream.cleanupBuffers();

  UdrErrorReply *reply =
    new (UdrGlob->getIpcHeap()) UdrErrorReply(UdrGlob->getIpcHeap());

  msgStream << *reply;
  reply->decrRefCount();

  if (!diags)
    diags = addOrCreateErrorDiags(UdrGlob,
                                  errorNumber,
                                  intErrorInfo,
                                  charErrorInfo,
                                  NULL);
  if (diags)
  {
    msgStream << *diags;
    diags->decrRefCount();
  }

  doErrorDebugging(UdrGlob, moduleName,
                   errorNumber, intErrorInfo, charErrorInfo);

  UdrGlob->numErrUDR_++;
  UdrGlob->numErrSP_++;
  UdrGlob->numErrInvokeSP_++;
  // UDR_ASSERT(0, "UDR Data Error Reply");

}  // dataErrorReply

static void verifyUdrServer(UdrGlobals &glob){
  LmResult result = LM_OK;
  NAMemory *h = glob.getUdrHeap();
  //
  // Create a diags area
  //
  ComDiagsArea *diags = ComDiagsArea::allocate(h);

  // Create an LM instance
  LmLanguageManager *lm = glob.getOrCreateLM(result,
                                             COM_LANGUAGE_JAVA,
                                             diags);
  if (!lm && result == LM_OK)
  {
    result = LM_ERR;
  }

  // Destroy the LM and the JVM. This will flush any JVM output
  // buffers such as those containing data from the -verbose or
  // -Xrunhprof options.
  glob.destroyJavaLMAndJVM();
  fflush(stderr);
  fflush(stdout);

  if (result == LM_OK)
  {
    fprintf(stdout, "MXUDR Status: OK!\n");
  }
  else
  {
    fprintf(stdout, "MXUDR Status: Errors were encountered!\n");
    fprintf(stdout, "Language Manager constructor returned %s\n", 
            LmResultToString(result));
    DumpDiags(cout, diags, "");
  }
}

static const char *LmResultToString(const LmResult &r)
{
  switch (r)
  {
    case LM_OK:                   return "LM_OK";
    case LM_ERR:                  return "LM_ERR";
    case LM_CONT_NO_READ_ACCESS:  return "LM_CONT_NO_READ_ACCESS";
    case LM_CONV_REQUIRED:        return "LM_CONV_REQUIRED";
    case LM_CONV_ERROR:           return "LM_CONV_ERROR";
    case LM_PARAM_OVERFLOW:       return "LM_PARAM_OVERFLOW";
    default:                      return ComRtGetUnknownString((Int32) r);
  }
}

static void DumpDiags(ostream &stream, ComDiagsArea *d, const char *prefix)
{
  if (d && d->getNumber() > 0)
  {
    stream << prefix << "BEGIN SQL diagnostics" << endl;
    NADumpDiags(stream,     // ostream &
                d,          // ComDiagsArea *
                TRUE,       // add newline after each diagnostic?
                FALSE,      // to avoid "--" comment prefix
                NULL,       // FILE *
                1           // for verbose output
                );
    stream << prefix << "END SQL diagnostics" << endl;
  }
}

// Function to invoke a routine body using the Language Manager (LM)
// API. This function supports command-line invocations and is never
// called when MXUDR runs as a server. 
//
// Assumptions
// * the number of arguments to the routine is the number of strings
//   passed on the command-line
// * all arguments are character strings
// * For Java 
//   (1) the argument type must be String[]
//   (2) All arguments are considered INOUT params, except as given in (3)
//   (3) for 'main' method the arguments are considered to be IN params
//
static Int32 invokeUdrMethod(const char *method,
                           const char *container,
                           const char *path,
                           NABoolean isJava,
                           NABoolean isUdf,
                           NABoolean txRequired,
                           NABoolean useVarchar,
                           Int32 argc,
                           char *argv[],
                           Int32 nResultSets,
                           Int32 nTimesToInvoke,
                           UdrGlobals &glob)
{
  LmResult result = LM_OK;
  FILE *f = MXUDR_OUTFILE;
  const char *prefix = MXUDR_PREFIX;
  const char *prefixPlusSpace = MXUDR_PREFIX_PLUS_SPACE;
  NAMemory *h = glob.getUdrHeap();
  Int32 i;
  Lng32 cliResult = 0;

  fprintf(f, "%s Preparing to invoke a routine body\n", prefix);
  fprintf(f, "%s Language %s, routine type %s\n", prefix,
          (isJava ? "JAVA" : "C"), (isUdf ? "UDF" : "STORED PROCEDURE"));
  fprintf(f, "%s Container: %s/%s\n", prefix, path, container);
  fprintf(f, "%s Routine: %s\n", prefix, method);
  fprintf(f, "%s The routine will be invoked %d time%s\n",
          prefix, nTimesToInvoke, (nTimesToInvoke == 1 ? "" : "s"));
  for (i = 0; i < argc; i++)
  {
    fprintf(f, "%s argv[%d] = \"%s\"\n", prefix, i, argv[i]);
  }
  fflush(f);

  // Create a diags area
  ComDiagsArea *diags = ComDiagsArea::allocate(h);

  // Create an LM instance
  fprintf(f, "%s About to create an LM instance\n", prefix);
  fflush(f);
  LmLanguageManager *lm =
    glob.getOrCreateLM(result,
                       isJava ? COM_LANGUAGE_JAVA : COM_LANGUAGE_C,
                       diags);

  if (!lm && result == LM_OK)
  {
    // This is not supposed to happen. We will treat it the same as
    // LM_ERR.
    result = LM_ERR;
  }

  fprintf(f, "%s LM constructor returned %s\n",
          prefix, LmResultToString(result));
  fflush(f);
  DumpDiags(cout, diags, prefixPlusSpace);

  // For Java we need to assemble the method signature. All parameters
  // are considered INOUT strings and return type is void.
  // One exception is parameters to 'main' method are considered
  // IN strings and the signature contains only one String param.
  char *sig = NULL;
  if (isJava && result == LM_OK)
  {
    const char *stringSig = "[Ljava/lang/String;";
    const char *rsSig = "[Ljava/sql/ResultSet;";
    Int32 stringSigLen = str_len(stringSig);
    Int32 rsSigLen = str_len(rsSig);
    sig = new (h) char [(argc * stringSigLen) + (nResultSets * rsSigLen) + 20];
    sig[0] = 0;
    str_cat(sig, "(", sig);
    if (strcmp(method, "main") == 0)
    {
      str_cat(sig, stringSig, sig);
    }
    else
    {
      for (i = 0; i < argc; i++)
        str_cat(sig, stringSig, sig);

      for (i = 0; i < nResultSets; i++)
        str_cat(sig, rsSig, sig);
    }
    str_cat(sig, ")V", sig);
    fprintf(f, "%s Java signature: %s\n", prefix, sig);
    fflush(f);
  }

  // Allocate buffers for input and output below while setting up
  // LmParameter structures.
  char *inputRow = NULL, *outputRow = NULL;

  // Prepare an LmParameter array. For UDFs we need one additional
  // LmParameter for the return value. We assume the output value will
  // be a character string of no more than 32 bytes.
  ComUInt32 maxUdfOutLen = 32;
  LmParameter *lmParams = NULL;
  Int32 numLmParams = argc;

  if (isUdf && result == LM_OK)
    numLmParams++;

  Int32 inputRowLen = 0;
  Int32 outputRowLen = 0;

  if (result == LM_OK)
  {
    // Find the input row length
    for (i = 0; i < argc; i++)
    {
      // * 2 bytes null indicator always aligned on 8-byte boundary
      // * 2 bytes for alignment
      // * for VARCHAR values, 2 bytes for the length indicator
      // * data
      inputRowLen += (2 + 2 + (useVarchar ? 2 : 0) + str_len(argv[i]));
      inputRowLen = ROUND8(inputRowLen);
    }

    // Find the output row length
    if (isUdf)
    {
      // * 2 bytes null indicator
      // * 2 bytes for alignment
      // * for VARCHAR values, 2 bytes for the length indicator
      // * data (with room for a null terminator)
      outputRowLen = (2 + 2 + (useVarchar ? 2 : 0) + maxUdfOutLen + 1);
      outputRowLen = ROUND8(outputRowLen);
    }
    else
    {
      // For SPJs, all the parameters are assumed to be INOUT. So,
      // input and output row lengths are same
      outputRowLen = inputRowLen;
    }

    // Assign memory for the input and output rows
    inputRow = (char *) h->allocateMemory(inputRowLen);
    memset(inputRow, 0, inputRowLen);
    outputRow = (char *) h->allocateMemory(outputRowLen);
    memset(outputRow, 0, outputRowLen);

    if (numLmParams > 0)
    {
      lmParams = (LmParameter *)
        h->allocateMemory(numLmParams * sizeof(LmParameter));
      memset(lmParams, 0, numLmParams * sizeof(LmParameter));
    }

    ComFSDataType fsType (useVarchar ? COM_VCHAR_FSDT : COM_FCHAR_FSDT);
    Int32 currParamOffset = 0;
    for (i = 0; i < argc; i++)
    {
      currParamOffset = ROUND8(currParamOffset);
      Int32 argLen = str_len(argv[i]);

      // Figure out the type and varchar indicator offsets
      Int32 vcIndOffset = 0, vcIndLen = 0;
      UInt32 dataOffset = currParamOffset + 4;
      if (useVarchar)
      {
        vcIndOffset = currParamOffset + 4;
        vcIndLen = 2;
        dataOffset += 2;
      }

      if (isUdf || strcmp(method, "main") == 0)
      {
        lmParams[i].init(fsType,
                         0, // precision
                         0, // scale,
                         CharInfo::ISO88591,
                         CharInfo::DefaultCollation,
                         COM_INPUT_COLUMN,
                         FALSE, // objMap
                         RS_NONE,
                         dataOffset,           // IN  offset
                         argLen,               //     len
                         currParamOffset,      //     null offset
                         2,                    //     null len
                         vcIndOffset,          //     vc offset
                         vcIndLen,             //     vc len
                         0,                    // OUT offset
                         0,                    //     len
                         0,                    //     null offset
                         0,                    //     null len
                         0,                    //     vc offset
                         0,                    //     vc len
                         NULL); // name
      }
      else
      {
        lmParams[i].init(fsType,
                         0, // precision
                         0, // scale,
                         CharInfo::ISO88591,
                         CharInfo::DefaultCollation,
                         COM_INOUT_COLUMN,
                         FALSE, // objMap
                         RS_NONE,
                         dataOffset,           // IN  offset
                         argLen,               //     len
                         currParamOffset,      //     null offset
                         2,                    //     null len
                         vcIndOffset,          //     vc offset
                         vcIndLen,             //     vc len
                         dataOffset,           // OUT offset
                         argLen,               //     len
                         currParamOffset,      //     null offset
                         2,                    //     null len
                         vcIndOffset,          //     vc offset
                         vcIndLen,             //     vc len
                         NULL); // name
      }

      // copy the value to inputRow
      str_cpy_all(inputRow + dataOffset, argv[i], argLen);
      
      if (useVarchar)
        *(short *)(inputRow + vcIndOffset) = argLen;
      
      // Move currParamOffset beyond the current parameter
      currParamOffset += 2; // null indicator
      currParamOffset += 2; // padding for alignment
      currParamOffset += (useVarchar ? 2 : 0); // VC len
      currParamOffset += argLen; // data
      currParamOffset = ROUND8(currParamOffset);

    } // for each input param

    if (isUdf)
    {
      // Add the trailing output param
      lmParams[i].init(fsType,
                       0, // precision
                       0, // scale,
                       CharInfo::ISO88591,
                       CharInfo::DefaultCollation,
                       COM_OUTPUT_COLUMN,
                       FALSE, // objMap
                       RS_NONE,
                       0,                      // IN  offset
                       0,                      //     len
                       0,                      //     null offset
                       0,                      //     null len
                       0,                      //     vc offset
                       0,                      //     vc len
                       (useVarchar ? 6 : 4),   // OUT offset
                       maxUdfOutLen,           //     len
                       0,                      //     null offset
                       2,                      //     null len
                       (useVarchar ? 2 : 0),   //     vc offset
                       (useVarchar ? 2 : 0),   //     vc len
                       NULL); // name

    } // if (isUdf)
  } // if (result == LM_OK)
  
  LmRoutine *lmRoutine = NULL;
  i = 0;

  ComRoutineExternalSecurity externalSecurity =
		                     COM_ROUTINE_EXTERNAL_SECURITY_INVOKER;
  Int32 routineOwnerId = ROOT_USER_ID; //dbRoot

  while (result == LM_OK && i++ < nTimesToInvoke)
  {
    // Begin a transaction
    if (txRequired && result == LM_OK)
    {
      Int32  transtag;
      Int32 tmfResult;
      tmfResult = BEGINTRANSACTION((Int32 *)&transtag);
      if (tmfResult != 0)
      {
        fprintf(f, "%s BEGINTRANSACTION() returned %d\n",
                prefix, tmfResult);
        fflush(f);
        result = LM_ERR;
      }
      else
      {
        fprintf(f, "%s Started a transaction\n", prefix);
        fflush(f);
      }
    }
    
    if (i == 1)
    {    
      // Call LM::getRoutine()
      if (result == LM_OK)
      {
        result = lm->getRoutine(numLmParams,
                                lmParams,
                                0,   //numTableInfo
                                NULL,//tableInfo
                                NULL, // return value
                                (isJava ? COM_STYLE_JAVA_CALL : COM_STYLE_SQL),
                                (txRequired ? COM_TRANSACTION_REQUIRED : COM_NO_TRANSACTION_REQUIRED),                                	
                                COM_MODIFIES_SQL,
                                "", // parentQid
                                inputRowLen,
                                outputRowLen,
                                method, // sqlName
                                method, // externalName
                                sig,
                                container,
                                path,
                                container,
                                glob.getCurrentUserName(), 
                                glob.getSessionUserName(),
                                externalSecurity,
                                routineOwnerId,
                                &lmRoutine,
                                NULL,
                                NULL,
                                nResultSets,
                                diags);
        if (result != LM_OK)
        {
          fprintf(f, "%s getRoutine() returned %s\n",
                  prefix, LmResultToString(result));
          fflush(f);
        }

        DumpDiags(cout, diags, prefixPlusSpace);
      }
    }

    // Call LM::invokeRoutine()
    if (result == LM_OK && lmRoutine)
    {
      fprintf(f, "\n%s About to invoke the routine body...\n", prefix);
      fflush(f);

      result = lm->invokeRoutine(lmRoutine, inputRow, outputRow, diags);

      if (result != LM_OK)
      {
        fprintf(f, "%s invokeRoutine() returned %s\n",
                prefix, LmResultToString(result));
        fflush(f);
      }
      else
      {
        fprintf(f, "%s The routine body has returned\n", prefix);
        if (isUdf)
        {
          char *udfRetVal = outputRow + (useVarchar ? 6 : 4);
          if (useVarchar)
          {
            LmParameter &p = lmParams[numLmParams - 1];
            ComUInt32 actual = p.actualOutDataSize(outputRow);
            if (actual > maxUdfOutLen)
              actual = maxUdfOutLen;
            udfRetVal[actual] = 0;
          }
          else
          {
            udfRetVal[maxUdfOutLen] = 0;
          }

          fprintf(f, "%s Return value [%s]\n", prefix, udfRetVal);
        }
        fflush(f);
      }

      DumpDiags(cout, diags, prefixPlusSpace);
    }
    
    // Quiesce the executor
    if (txRequired && result == LM_OK)
    {
      cliResult = SQL_EXEC_Xact(SQLTRANS_QUIESCE, NULL);
      if (cliResult != 0)
      {
        fprintf(f, "%s SQL_EXEC_Xact() returned %d\n",
                prefix, cliResult);
        fflush(f);
        result = LM_ERR;
      }
      else
      {
        fprintf(f, "%s Quiesced the executor\n", prefix);
        fflush(f);
      }
    }

    // End the transaction
    if (txRequired && result == LM_OK)
    {
      Int32 tmfResult = ENDTRANSACTION();
      if (tmfResult != 0)
      {
        fprintf(f, "%s ENDTRANSACTION() returned %d\n",
                prefix, tmfResult);
        fflush(f);
        result = LM_ERR;
      }
      else
      {
        fprintf(f, "%s Ended the transaction\n", prefix);
        fflush(f);
      }
    }
  }

  if (result == LM_OK)
  {  
      fprintf(f, "\n");
      fprintf(f, "%s Freeing resources...\n", prefix);
      fflush(f);
  }

  // Call LM::putRoutine()
  if (result == LM_OK && lmRoutine)
  {
    result = lm->putRoutine(lmRoutine, diags);
    if (result != LM_OK)
    {
      fprintf(f, "%s putRoutine() returned %s\n",
              prefix, LmResultToString(result));
      fflush(f);
    }
    else
    {
      fprintf(f, "%s putRoutine() was successful\n", prefix);
      fflush(f);
    }
    DumpDiags(cout, diags, prefixPlusSpace);
  }
  
  // Destroy the LM and the JVM. This will flush any JVM output
  // buffers such as those containing data from the -verbose or
  // -Xrunhprof options.
  fprintf(f, "%s About to shut down the LM\n", prefix);
  fflush(f);
  if (isJava)
    glob.destroyJavaLMAndJVM();
  else
    glob.destroyCLM();
  fflush(stderr);
  fflush(stdout);

  if (result == LM_OK)
  {
    fprintf(f, "%s Done\n\n", prefix);
  }
  else
  {
    fprintf(f, "%s Done. Errors were encountered.\n\n", prefix);
  }
  fflush(f);

  // Release allocated memory
  if (sig)
    h->deallocateMemory(sig);
  if (inputRow)
    h->deallocateMemory(inputRow);
  if (outputRow)
    h->deallocateMemory(outputRow);
 
  return result;

} // invokeUdrMethod()

// LCOV_EXCL_START
// Dead Code
// Andy thinks we should retire this interface...

// Function to open a file and then repeatedly call a helper function
// that will process one command from that file.
static Int32 processCommandsFromFile(const char *filename, UdrGlobals &glob)
{
  Int32 result = 0;
  FILE *out = MXUDR_OUTFILE;
  const char *prefix = MXUDR_PREFIX;

  fprintf(out, "%s Commands will be processed from file %s\n",
          prefix, filename);

  FILE *in = fopen(filename, "r");
  if (!in)
  {
    fprintf(out, "%s *** ERROR: Could not open input file %s: %s.\n",
            prefix, filename, strerror(errno));
    return 1;
  }

  while (processSingleCommandFromFile(in, glob) == 0)
  {
  }

  fprintf(out, "\n");

  return result;
}

// Function to process one command from a text file. Returns 0 if the
// command was successfully processed, non-zero otherwise. Error
// messages are written to MXUDR_OUTFILE if anything goes wrong.
static Int32 processSingleCommandFromFile(FILE *in, UdrGlobals &glob)
{
  FILE *out = MXUDR_OUTFILE;
  const char *prefix = MXUDR_PREFIX;

  char target[1024];
  char originalCommand[1024];
  char *resultOfRead = NULL;
  char *trimmed;

  // The while loop will repeat until either we cannot read any more
  // data from the file, or we have successfully extracted the next
  // non-blank line.
  NABoolean done = false;
  while (!done)
  {
    // Read a line from the file
    resultOfRead = fgets(target, 1024, in);
    if (resultOfRead != target)
    {
      if (ferror(in))
      {
        fprintf(out, "%s *** ERROR: fgets failed due to an I/O error: %s.\n",
                prefix, strerror(errno));
        return 1;
      }
      return 1;
    }
    
    // Trim leading and trailing spaces
    Int32 len = str_len(target);
    trimmed = target;
    if (len > 0)
    {
      while (isSpace8859_1(trimmed[0]))
      {
        trimmed++;
      }
      len = str_len(trimmed);
      while (len > 0 && isSpace8859_1(trimmed[len - 1]))
      {
        trimmed[len - 1] = 0;
        len--;
      }
    }

    if (str_len(trimmed) > 0)
    {
      done = true;
    }

  }

  // Save a copy of the original command
  originalCommand[0] = 0;
  str_cat(originalCommand, trimmed, originalCommand);

  char *tok;
  const char *delim = " ";

  // Now we (very primitively) walk through each token in the command,
  // doing some semantic checking along the way. Right now the only
  // supported command is "invoke [-n N] method class path [arg]..."

  // $$$$ TBD
  // * GET THIS TO WORK WITH RESULT SETS
  // * SUPPORT -notx OPTION
  // * SUPPORT -udf OPTION
  // * SUPPORT -param C
  // * SUPPORT -vc

  NABoolean txRequired = TRUE;
  NABoolean isUdf = FALSE;
  NABoolean isJava = TRUE;
  NABoolean useVarchar = FALSE;

  tok = strtok(trimmed, delim);
  if (tok == NULL)
  {
    fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
    return 1;
  }

  if (stricmp(tok, "invoke") != 0)
  {
    fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
    return 1;
  }

  tok = strtok(NULL, delim);
  if (tok == NULL)
  {
    fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
    return 1;
  }

  Int32 nTimesToInvoke = 1;
  Int32 nResultSets = 0;
  const char *paramStyle = "java";
  if (stricmp(tok, "-n") == 0)
  {
    tok = strtok(NULL, delim);
    if (tok == NULL)
    {
      fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
      return 1;
    }
    nTimesToInvoke = atoi(tok);

    // Next token needs to be "-param <param style>"
    tok = strtok(NULL, delim);
    if (tok == NULL || stricmp(tok, "-param") != 0)
    {
      fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
      return 1;
    }

    tok = strtok(NULL, delim);
    if (tok == NULL)
    {
      fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
      return 1;
    }
    paramStyle = tok;

    tok = strtok(NULL, delim);
    if (tok == NULL)
    {
      fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
      return 1;
    }
  }

  const char *method = tok;

  tok = strtok(NULL, delim);
  if (tok == NULL)
  {
    fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
    return 1;
  }

  const char *container = tok;

  tok = strtok(NULL, delim);
  if (tok == NULL)
  {
    fprintf(out, "%s *** ERROR: Bad command: %s\n", prefix, originalCommand);
    return 1;
  }

  const char *path = tok;

  // Done with semantic checks. All remaining arguments will be input
  // to the user's Java method. We collect them into the argv array.
  const Int32 maxNumArgs = 1024;
  char *argv[maxNumArgs];
  Int32 argc = 0;
  while ((tok = strtok(NULL, delim)) != NULL)
  {
    if (argc < maxNumArgs)
    {
      argv[argc] = tok;
    }
    argc++;
  }

  if (argc > maxNumArgs)
  {
    fprintf(out,
            "%s *** ERROR: The number of arguments (%d) must not exceed %d\n",
            prefix, argc, maxNumArgs);
    return 1;
  }

  // Now we have a complete description of the method that the user
  // wants us to invoke. Call a helper function to do the work.

  const char *longLine =
    "----------------------------------------------------------------------";

  fprintf(out, "%s %s\n", prefix, longLine);
  fprintf(out, "%s  Command: %s\n", prefix, originalCommand);
  fprintf(out, "%s %s\n", prefix, longLine);

  Int32 result = invokeUdrMethod(method, container, path,
                               isJava, isUdf, txRequired, useVarchar,
                               argc, argv, nResultSets,
                               nTimesToInvoke, glob);
      
  return result;

} // processSingleCommandFromFile


void udrAssert(const char *f, Int32 l, const char *m)
{
  if(!f)
    f = "";
  if(!m)
    m = "";
  cout << "MXUDR Assertion: " << m << endl;
  cout << "at FILE: " << f << " LINE: " << l << endl;
  makeTFDSCall(m, f, l);
  // should not reach here
}

void udrAbort(const char *f, Int32 l, const char *m)
{
  if(!f)
    f = "";
  if(!m)
    m = "";
  cout << "MXUDR Abort: " << m << endl;
  cout << "at FILE: " << f << " LINE: " << l << endl;
  makeTFDSCall(m, f, l, FALSE);
  // should not reach here
}
// LCOV_EXCL_STOP

// Stubs
#ifdef UDR_OSS_RELEASE
#include "CharType.h"
#include "NAType.h"

NABoolean NAType::isComparable(const NAType &other,
			       ItemExpr *parentOp,
			       Int32 emitErr) const
{ return FALSE; }

NABoolean CharType::isComparable(const NAType &other,
			       ItemExpr *parentOp,
			       Int32 emitErr) const
{ return FALSE; }
#endif

//
// Function to do all the work of processing a session message. Right
// now the only session message we support is JVM startup options. Any
// other type of session message will be ignored.
//
void processASessionMessage(UdrGlobals *glob,
                            UdrServerReplyStream &msgStream,
                            UdrSessionMsg &request)
{
  const char *moduleName = "processASessionMessage";

#ifdef UDR_OSS_DEBUG

  // In the debug build we allow some environment settings to force
  // the server to do some bad things such as exit and send signals to
  // other processes. For testing purposes only.
  //
  //   MXUDR_SESSION_CRASH    Exit immediately
  //
  //   MXUDR_SESSION_SIGNAL   Assume the value is an OSS process ID
  //                          and kill that process
  //

  char *crash = getenv("MXUDR_SESSION_CRASH");
  if (crash && crash[0])
  {
    UDR_DEBUG0("*** About to call exit(1) because environment");
    UDR_DEBUG1("*** variable MXUDR_SESSION_CRASH is set to '%s'.", crash);
    exit(1);
  }

  char *pname = getenv("MXUDR_SESSION_SIGNAL");
  if (pname && pname[0])
  {
    Int32 pnum = atoi(pname);
    Int32 signum = SIGINT;

    UDR_DEBUG2("*** About to call kill(%d,%d) because environment",
               pnum, signum);
    UDR_DEBUG1("*** variable MXUDR_SESSION_SIGNAL is set to '%s'.", pname);

    Int32 result = kill(pnum, signum);
    UDR_DEBUG1("*** kill() returned %d", result);

    // Now sleep for 5 seconds. Sometimes the signal does not get
    // delivered right away and we do not want the client to receive
    // our reply message before the signal is processed.
    const Int32 secondsToSleep = 5;
    DELAY(secondsToSleep * 100);
  }

#endif // UDR_OSS_DEBUG

  if (request.getType() == UdrSessionMsg::UDR_SESSION_TYPE_JAVA_OPTIONS)
  {
    LmJavaOptions *javaOptions = glob->getJavaOptions();

    // 1. Clear any existing options
    javaOptions->removeAllOptions();
    
    // 2. Move environment settings into javaOptions
    InitializeJavaOptionsFromEnvironment(*javaOptions, glob->getUdrHeap());

   if (!IsNAStringSpaceOrEmpty(initErrText) )
    {
      controlErrorReply(glob,
                        msgStream,
                        UDR_ERR_MESSAGE_PROCESSING,    //errorNumber,
                        0,                             //intErrorInfo,
                        initErrText.data()             //charErrorInfo
                        );
      return;
    }
    
    // 3. Move options from the message into javaOptions so that they
    // take precedence over environment settings. The message is
    // expected to contain two strings. First is a string of delimited
    // JVM startup options. Second is the set of delimiter characters
    // (default is a single space).
    ComUInt32 numStrings = request.numStrings();
    if (numStrings >= 1)
    {
      const char *options = request.getString(0);
      const char *delimiters = " ";
      if (numStrings >= 2)
      {
        delimiters = request.getString(1);
      }
      javaOptions->addOptions(options, delimiters, FALSE);
    }
  }

  // Build a reply and send it
  UdrSessionReply *reply = new (glob->getIpcHeap())
    UdrSessionReply(glob->getIpcHeap());
  reply->setHandle(request.getHandle());

  msgStream.clearAllObjects();
  msgStream << *reply;

  if (glob->verbose_ &&
      glob->traceLevel_ >= TRACE_IPMS &&
      glob->showUnload_)
  {
    ServerDebug("[UdrServ (%s)] Send Session Reply", moduleName);
  }

  sendControlReply(glob, msgStream, NULL);
  reply->decrRefCount();

}

//
// Helper function to dump memory stats to the UDR debug stream. Right
// now this only works on NSK and is only enabled for debug builds.
//
static void DumpProcessInfo()
{
#ifndef UDR_OSS_DEBUG
  return;
#else
  short attr_list[] = 
    {   38  // PIN
      , 54  // Current Page Count
      , 59  // Page Fault Count
      , 76  // PFS size
      , 77  // Maximum PFS size
      , 109 // Size of global data
      , 103 // Current main stack size
      , 104 // Maximum main stack size
      , 111 // Current size of native heap
      , 112 // Maximum size of native heap
      , 113 // Guaranteed Swap Size
      , 119 // Process is TNS/R Native?
      , 39  // Program file name
    };

  const short attr_count = sizeof(attr_list) / sizeof(attr_list[0]);

  const short return_values_maxlen = 256;
  short return_values_list[return_values_maxlen];
  memset(return_values_list, 0, sizeof(return_values_list));
  
  short return_values_len;
  short rc, errordetail;

  rc = PROCESS_GETINFOLIST_(  // cpu
                            , // pin
                            , // nodename
                            , // nodename_length
                            , // process_handle
                            , attr_list
                            , attr_count
                            , return_values_list
                            , return_values_maxlen
                            , &return_values_len
                            , &errordetail
                            );
  
  UDR_DEBUG0("[BEGIN DumpProcessInfo]");
  
  if (rc)
  {
    UDR_DEBUG0("  *** Error occurred in PROCESS_GETINFOLIST_");
    UDR_DEBUG2("  *** Return code %d, error detail %d",
               (Int32) rc, (Int32) errordetail);
  }
  else
  {  
    UDR_DEBUG1("  Process ID %d", (Int32) return_values_list[0]);
    UDR_DEBUG1("  Program file %s", (char *) &return_values_list[22]);
    UDR_DEBUG1("  Current pages %d", (Int32) return_values_list[1]);
    UDR_DEBUG1("  Page faults %ld", *((Lng32 *) &return_values_list[2]));
    UDR_DEBUG1("  PFS size %ld", *((Lng32 *) &return_values_list[4]));
    UDR_DEBUG1("  Max PFS size %ld", *((Lng32 *) &return_values_list[6]));
    UDR_DEBUG1("  Global data size %ld", *((Lng32 *) &return_values_list[8]));
    UDR_DEBUG1("  Stack size %ld", *((Lng32 *) &return_values_list[10]));
    UDR_DEBUG1("  Max stack size %ld",
               *((Lng32 *) &return_values_list[12]));
    UDR_DEBUG1("  Native heap size %ld", *((Lng32 *) &return_values_list[14]));
    UDR_DEBUG1("  Max native heap size %ld",
               *((Lng32 *) &return_values_list[16]));
    UDR_DEBUG1("  Guaranteed swap space %ld",
               *((Lng32 *) &return_values_list[18]));
    UDR_DEBUG1("  TNS/R Native flag %d", (Int32) return_values_list[20]);
  }

  UDR_DEBUG0("[END DumpProcessInfo]");
#endif  
}

// ENTER TX message processing. The message is given to corresponding
// spinfo. The message is not replied immediately and only replied
// when EXIT TX or SUSPEND TX arrives.
void processAnEnterTxMessage(UdrGlobals *glob,
                             UdrServerReplyStream &msgStream,
                             UdrEnterTxMsg &request)
{
  const char *moduleName = "processAnEnterTxMessage";

  SPInfo *sp = glob->getSPList()->spFind(request.getHandle());
  if (sp)
  {
    // There should be only one connection associated with this message
    // stream. We use that connection to pass on to TxStream.
    UDR_ASSERT((msgStream.getRecipients().entries() == 1),
      "A UdrServerReplyStream must be associated with a single Connection");

    CollIndex i = 0;
    msgStream.getRecipients().setToNext(i);
    IpcConnection *conn = msgStream.getRecipients().element(i);

    UdrServerReplyStream *stream = sp->getTxStream();
    if (stream->getState() == IpcMessageStream::RECEIVED)
    {
      // A message is already waiting on the stream. This is an error
      controlErrorReply(glob,
                        msgStream,
                        UDR_ERR_MESSAGE_PROCESSING,
                        0,
                        "An Enter TX message is already pending.");
      return;
    }

    // Give connection that the EnterTX appeared on as second param.
    msgStream.giveMessageTo(*stream, conn);

    if (glob->verbose_ && glob->traceLevel_ >= TRACE_IPMS)
    {
      ServerDebug("[UdrServ (%s)] Enter TX message is given to SP %s_" INT64_SPEC,
                  moduleName,
                  sp->getSqlName(),
                  sp->getUdrHandle());
    }
  }
  else
  {
    controlErrorReply(glob,
                      msgStream,
                      UDR_ERR_MISSING_UDRHANDLE,
                      0,
                      "Enter TX");
  }
}

void processASuspendTxMessage(UdrGlobals *glob,
                              UdrServerReplyStream &msgStream,
                              UdrSuspendTxMsg &request)
{
  const char *moduleName = "processASuspendTxMessage";

  // First reply to Enter TX message
  SPInfo *sp = glob->getSPList()->spFind(request.getHandle());
  if (sp)
  {
    NABoolean doneWithRS = FALSE;
    sp->replyToEnterTxMsg(doneWithRS);
  }
  else
  {
    controlErrorReply(glob,
                      msgStream,
                      UDR_ERR_MISSING_UDRHANDLE,
                      0,
                      "Suspend TX");
    return;
  }

  UdrSuspendTxReply *reply = new (glob->getIpcHeap())
    UdrSuspendTxReply(glob->getIpcHeap());
  reply->setHandle(request.getHandle());
  
  msgStream.clearAllObjects();
  msgStream << *reply;
  
  if (glob->verbose_ &&
      glob->traceLevel_ >= TRACE_IPMS &&
      glob->showUnload_)
  {
    ServerDebug("[UdrServ (%s)] Send SUSPEND TX Reply", moduleName);
  }
  
  // No need to quiesce executor for replying to this message, so
  // set NULL at the third param.
  sendControlReply(glob, msgStream, NULL);
  reply->decrRefCount();
}

void processAnExitTxMessage(UdrGlobals *glob,
                            UdrServerReplyStream &msgStream,
                            UdrExitTxMsg &request)
{
  const char *moduleName = "processAnExitTxMessage";

  // First reply to Enter TX message
  SPInfo *sp = glob->getSPList()->spFind(request.getHandle());
  if (sp)
  {
    // Reply to Enter Tx.
    NABoolean doneWithRS = TRUE;
    sp->replyToEnterTxMsg(doneWithRS);
  }
  else
  {
    controlErrorReply(glob,
                      msgStream,
                      UDR_ERR_MISSING_UDRHANDLE,
                      0,
                      "Exit TX");
    return;
  }

  UdrExitTxReply *reply = new (glob->getIpcHeap())
    UdrExitTxReply(glob->getIpcHeap());
  reply->setHandle(request.getHandle());

  msgStream.clearAllObjects();
  msgStream << *reply;

  if (glob->verbose_ &&
      glob->traceLevel_ >= TRACE_IPMS &&
      glob->showUnload_)
  {
    ServerDebug("[UdrServ (%s)] Sending EXIT TX Reply", moduleName);
  }

  // No need to quiesce executor for replying to this message, so
  // set NULL at the third param.
  sendControlReply(glob, msgStream, NULL);
  reply->decrRefCount();
}

Int32 PerformWaitedReplyToClient(UdrGlobals *UdrGlob,
                                UdrServerDataStream &msgStream)
{
  //Most of the calls here is copied from runServer() above.

  sendDataReply(UdrGlob, msgStream, NULL);

  //Note down the current sp so that we can restore it back
  //once ProcessARequest() call is complete. There are scenarios
  //in ProcessARequest to reset spInfo in udrGlobals once Spinfo
  //returns control back to udrserv.
  SPInfo *sp = UdrGlob->getCurrSP();

  //get control connection to IPC and env
  GuaReceiveControlConnection *ctrlConn = UdrGlob->getControlConnection();
  IpcEnvironment *env = UdrGlob->getIpcEnvironment();

  env->deleteCompletedMessages();

  NAList<UdrServerReplyStream *> &replyStreams =
        UdrGlob->getReplyStreams();

  // Make sure we wait for a new message, even it it takes multiple
  // I/Os due to multi-chunking. Also try to avoid hanging at this
  // point, due to errors.
  while (!replyStreams.entries() &&
         ctrlConn->getConnection()->receiveIOPending() &&
         ctrlConn->getConnection()->getState() != IpcConnection::ERROR_STATE &&
         ctrlConn->getNumRequestors() > 0)
  {
    // Nothing on the queue, go ahead and wait...
    // we have performed a reply, now wait indefinitely
    ctrlConn->wait(IpcInfiniteTimeout);
  }

  //Wait is over, continue processing.
  if(!replyStreams.entries())
  {
    return SQLUDR_ERROR;
  }
  
  //expect only one reply stream at location 0 ?
  UdrServerReplyStream *stream = replyStreams[0];
  replyStreams.removeAt(0);

  processARequest(UdrGlob, *stream, *env);

  //Restore SpInfo to udrGlobal
  UdrGlob->setCurrSP(sp);

  stream->addToCompletedList();

  return SQLUDR_SUCCESS;

}

