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
 * File:         UdrFFDC.cpp
 * Description:  MXUDR FFDC
 *
 * Created:      5/4/02
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <signal.h>

#include "vproc.h"

#include "udrglobals.h"
#include "udrutil.h"
#include "UdrDebug.h"
#include "UdrFFDC.h"
#include "logmxevent.h"

#include "copyright.h"

extern void my_mpi_fclose();

#define TEXT_SIZE 1024

extern UdrGlobals *UDR_GLOBALS; // UDR globals area

struct SignalAttr {
public:
  Int32 sigType;
  const char * sigName;
  struct sigaction udrSigAction;
  struct sigaction javaSigAction;
};

// We install UDR signal handler for all the signals that can cause the program to 
// terminate by default. The signals that are discarded by default (SIGIGN) or 
// the signals that can not be caught do not have entries in the following table.
static SignalAttr signalAttrTable [] = 
{
 // Avoid the signals used by Foundation
 // Signal     Name           Udr Sigaction        Java Sigaction
 { SIGHUP,     "SIGHUP",      {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGINT,     "SIGINT",      {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGQUIT,    "SIGQUIT",     {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGILL,     "SIGILL",      {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 /*  SIGURG -- discarded by default */
 /* SIGTRAP -- used by SQF */
 { SIGABRT,    "SIGABRT",     {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGBUS,     "SIGBUS",      {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGFPE,     "SIGFPE",      {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 /*  SIGKILL -- can not be caught */
 /*  SIGUNCP -- discarded by default */
 { SIGSEGV,    "SIGSEGV",     {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 /*  SIGWINCH -- discarded by default */
 /* SIGPIPE -- used by SQF */
 /* SIGALRM -- used by SQF */
 { SIGTERM,    "SIGTERM",     {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 /* SIGUSR1 -- used by SQF */
 /* SIGUSR2 -- used by SQF */
 /*  SIGCHLD -- discarded by default */
 /*  SIGSTOP -- can not be caught */
 /*  SIGTSTP -- does not result in abend */
 /*  SIGCONT -- execution continues */
 /*  SIGTTIN -- does not result in abend */
 /*  SIGTTOU -- does not result in abend */
 /* SIGPROF -- used by SQF */
 { SIGVTALRM,  "SIGVTALRM",   {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGXCPU,    "SIGXCPU",     {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGXFSZ,    "SIGXFSZ",     {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGSTKFLT,  "SIGSTKFLT",   {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGIO,      "SIGIO",       {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGPWR,     "SIGPWR",      {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
 { SIGUNUSED,  "SIGUNUSED",   {SIG_DFL,0, 0},      { SIG_DFL, 0, 0} },
};

static void getActiveRoutineInfoMsg(char *msg, NABoolean &isRoutineActive)
{
  char routineName[ComMAX_ANSI_NAME_EXTERNAL_LEN + 1];
  char routineLanguage[10];
  char routineType[20];

  routineName[0] = '\0'; 
  routineType[0] = '\0'; 
  routineLanguage[0] = '\0';

  // Note: accessing the global UdrGlobals instance directly
  if (UDR_GLOBALS)
  {
    getActiveRoutineInfo(UDR_GLOBALS, routineName, routineType, 
                         routineLanguage, isRoutineActive);
  }

  if (routineName[0] == '\0')
    sprintf(msg, "A user routine is not currently being processed");
  else {
    sprintf(msg, 
            "User routine being processed : %s, Routine Type : %s, Language Type : %s", 
            routineName, routineType, routineLanguage);
    if (isRoutineActive == TRUE)  
      strcat(msg, ", Error occurred inside the user routine code");
    else
      strcat(msg, ", Error occurred outside the user routine code");
  }
}

void logEMS(const char *msg) {
  SQLMXLoggingArea::logSQLMXAbortEvent("",
				       0,
				       msg);
}


// This method writes to stderr which gets redirected to monitor log file
inline void logStdErr(const char *msg) {
  write (2, msg, strlen(msg));
}

void comTFDS(const char *msg1, const char *msg2, const char *msg3, const char *msg4, const char *msg5, NABoolean dialOut
	   , NABoolean writeToSeaLog 
             )
{
  setSignalHandlersToDefault();

  if(!msg1)
    msg1 = "";
  if(!msg2)
    msg2 = "";
  if(!msg3)
    msg3 = "";
  if(!msg4)
    msg4 = "";
  if(!msg5)
    msg5 = "";

  UDR_DEBUG1("[SIGNAL] %s", msg1);
  UDR_DEBUG1("[SIGNAL] %s", msg2);
  UDR_DEBUG1("[SIGNAL] %s", msg3);
  UDR_DEBUG1("[SIGNAL] %s", msg4);
  UDR_DEBUG1("[SIGNAL] %s", msg5);

  UDR_DEBUG0("[SIGNAL] Logging an EMS message");
  char msg[TEXT_SIZE];
  strncpy(msg, msg1, sizeof(msg));
  strncat(msg, ", ", sizeof(msg)-strlen(msg));
  strncat(msg, msg2, sizeof(msg)-strlen(msg));
  strncat(msg, ", ", sizeof(msg)-strlen(msg));
  strncat(msg, msg3, sizeof(msg)-strlen(msg));
  strncat(msg, ", ", sizeof(msg)-strlen(msg));
  strncat(msg, msg4, sizeof(msg)-strlen(msg));
  strncat(msg, ", ", sizeof(msg)-strlen(msg));
  strncat(msg, msg5, sizeof(msg)-strlen(msg));
  if (writeToSeaLog)
     logEMS(msg);
  else
     logStdErr(msg);

  UDR_DEBUG0("[SIGNAL] Aborting...");
  abort();
}

void makeTFDSCall(const char *msg, const char *file, UInt32 line, NABoolean dialOut
	        , NABoolean writeToSeaLog 
                 )
{
  const char *msg1 = COPYRIGHT_UDRSERV_PRODNAME_H " Internal Error";
  const char *msg1a = COPYRIGHT_UDRSERV_PRODNAME_H " Abort";
  const char *msg2 = msg;
  char msg3[TEXT_SIZE];
  char msg4[2 * TEXT_SIZE]; // to accommodate 3-part ANSI name for routine
  // TEXT_SIZE is big enough
  if (line > 0)
    snprintf(msg3, TEXT_SIZE, "Error occurred in source file %s on line %d", file, line);
  else
    strcpy(msg3, "Source file information unavailable");

  NABoolean isRoutineActive;
  getActiveRoutineInfoMsg(msg4, isRoutineActive);

  comTFDS(dialOut ? msg1 : msg1a, 
          msg2, 
	  msg3, 
	  msg4, 
	  NULL, 
	  dialOut
	  , writeToSeaLog 
	  );
}


// Udr Trap(NSK)/signal(Linux) handler routine
// Main purpose of this handler routine on NSK is to call TFDS when the trap occurs and
// log an EMS event message showing signal name and any active user routine name at that time.
// On Linux, we do not call Sealog as it can cause a hang while sending message.
// Instead we write the message to STDERR which gets redirected to monitor log.
// Also the handler uses some library functions such as sprintf which are not known to be
// "async signal safe", so if there are any issues found, it may need changes.

void UdrSignalHandler( Int32 signo )
{

  // Block any more signals inside signal handler
  sigset_t sigMask;
  sigfillset (&sigMask);
  sigprocmask(SIG_BLOCK, &sigMask, NULL);

  UDR_DEBUG1("[SIGNAL] Caught signal %d", signo);
  const char *msg1 = COPYRIGHT_UDRSERV_PRODNAME_H " Signal Handler";
  char msg2[TEXT_SIZE];
  char msg3[2 * TEXT_SIZE]; // to accommodate 3-part ANSI name for routine

  sprintf(msg2, "SIGNAL %d: ", signo); // TEXT_SIZE is big enough.
  const char *sigText;
  switch( signo )
    {
    case SIGHUP:
      sigText = "Hangup.";
      break;
    case SIGINT:
      sigText = "Interrupt.";
      break;
    case SIGQUIT:
      sigText = "Quit.";
      break;
    case SIGILL:
      sigText = "Instruction failure.";
      break;
    case SIGABRT:
      sigText = "Abnormal termination.";
      break;
    case SIGBUS:
      sigText = "Bus error.";
      break;
    case SIGFPE:
      sigText = "Arithmetic overflow.";
      break;
    case SIGSEGV:
      sigText = "Illegal address reference.";
      break;
    case SIGPIPE:
      sigText = "Pipe error.";
      break;
    case SIGALRM:
      sigText = "Alarm.";
      break;
    case SIGTERM:
      sigText = "Software Terminate.";
      break;
    case SIGUSR1:
      sigText = "User defined 1.";
      break;
    case SIGUSR2:
      sigText = "User defined 2.";
      break;
    case SIGTSTP:
      sigText = "Interactive Stop.";
      break;

    case SIGVTALRM:
      sigText = "Virtual alarm clock.";
      break;
    case SIGXCPU:
      sigText = "CPU time limit exceeded.";
      break;
    case SIGXFSZ:
      sigText = "File size limit exceeded.";
      break;
    case SIGSTKFLT:
      sigText = "Stack fault.";
      break;
    case SIGIO:
      sigText = "I/O now possible.";
      break;
    case SIGPWR:
      sigText = "Power failure.";
      break;
    case SIGUNUSED:
      sigText = "Unused signal.";
      break;
    case SIGTTIN:
      sigText = "Background Read error.";
      break;
    case SIGTTOU:
      sigText = "Background Write error.";
      break;
    default:
      sigText = "Unknown error.";
    }
  strncat(msg2, sigText, sizeof(msg2) - strlen(msg2));

  NABoolean isRoutineActive;
  getActiveRoutineInfoMsg(msg3, isRoutineActive);

  comTFDS(msg1, msg2, msg3, NULL, NULL, TRUE, FALSE);
} // UdrTrapHandler()


void setUdrSignalHandlers()
{
  struct sigaction oldAction, sigAct;

  sigemptyset(&(sigAct.sa_mask));
  sigAct.sa_flags   = 0;
  sigAct.sa_handler = UdrSignalHandler;
  for(Int32 i=0; i<sizeof(signalAttrTable)/sizeof(SignalAttr); i++){
    if (sigaction (signalAttrTable[i].sigType, &sigAct, &oldAction)) {
      cerr << "[MXUDR DEBUG SIGNAL HANDLER] " 
	   << "sigaction() failed, cannot set signal handler for " 
	   << signalAttrTable[i].sigName << endl;
    }
  }
}

void printSignalHandlers()
{
  struct sigaction oldAction;
  void (*oldHdlr)(Int32);
  for(Int32 i=0; i<sizeof(signalAttrTable)/sizeof(SignalAttr); i++){
    if (sigaction (signalAttrTable[i].sigType, (struct sigaction*)NULL, &oldAction)) {
      cerr << "[MXUDR DEBUG SIGNAL HANDLER] " 
	   << "sigaction() failed, cannot obtain old signal handler for " 
	   << signalAttrTable[i].sigName << endl;
    }
    else {
      oldHdlr = oldAction.sa_handler;
      if(oldHdlr == SIG_DFL)
        printf("[MXUDR DEBUG SIGNAL HANDLER] %s SIG_DFL\n", signalAttrTable[i].sigName);
      else if(oldHdlr == SIG_IGN)
        printf("[MXUDR DEBUG SIGNAL HANDLER] %s SIG_IGN\n", signalAttrTable[i].sigName);
      else if(oldHdlr != SIG_ERR)
        printf("[MXUDR DEBUG SIGNAL HANDLER] %s %p\n", signalAttrTable[i].sigName, oldHdlr);
    }
  }
}

// Saves the UDR trap signal handlers 
NABoolean saveUdrTrapSignalHandlers()
{
  UDR_DEBUG0("[SIGNAL] Save  UDR Trap signal handlers");
  struct sigaction oldAction;

  for(Int32 i=0; i<sizeof(signalAttrTable)/sizeof(SignalAttr); i++){
    if (sigaction (signalAttrTable[i].sigType, (struct sigaction*)NULL, &oldAction)) {
      cerr << "[MXUDR DEBUG SIGNAL HANDLER] " 
	   << "sigaction() failed, cannot obtain old signal handler for " 
	   << signalAttrTable[i].sigName << endl;
      return FALSE;
    }

    signalAttrTable[i].udrSigAction.sa_handler = oldAction.sa_handler;
    signalAttrTable[i].udrSigAction.sa_mask = oldAction.sa_mask;
    signalAttrTable[i].udrSigAction.sa_flags = oldAction.sa_flags;
  }
  return TRUE;
}

// Sets the UDR server signal handlers to default so that the JVM won't complain. 
NABoolean setSignalHandlersToDefault()
{
  struct sigaction oldAction, sigAct;

  sigemptyset(&(sigAct.sa_mask));
  sigAct.sa_flags   = 0;
  sigAct.sa_handler = SIG_DFL;

  UDR_DEBUG0("[SIGNAL] Set signal handlers to default before Java LM initialization");
  for(Int32 i=0; i<sizeof(signalAttrTable)/sizeof(SignalAttr); i++){
    if (sigaction(signalAttrTable[i].sigType, &sigAct, &oldAction)) {
      char msg[256];
      sprintf(msg, 
             "[UDR WARNING] sigaction() failed, cannot change %s to the default handler\n", 
             signalAttrTable[i].sigName);
      logStdErr(msg);
      return FALSE;
    }
  }
  return TRUE;
}

// Restores the UDR server signal handlers to JVM signal handlers before re-entering
// any Java code.  
NABoolean restoreJavaSignalHandlers()
{
  UDR_DEBUG0("[SIGNAL] Restore Java signal handlers before entering Java LM");
  struct sigaction oldAction, sigAct;

  sigemptyset(&(sigAct.sa_mask));
  sigAct.sa_flags   = 0;

  for(Int32 i=0; i<sizeof(signalAttrTable)/sizeof(SignalAttr); i++){
    if (sigaction (signalAttrTable[i].sigType, (struct sigaction*)NULL, &oldAction)) {
      cerr << "[MXUDR DEBUG SIGNAL HANDLER] " 
	   << "sigaction() failed, cannot obtain old signal handler for " 
	   << signalAttrTable[i].sigName << endl;
      return FALSE;
    }
    if ( oldAction.sa_handler == signalAttrTable[i].javaSigAction.sa_handler ) {
       UDR_DEBUG1("[SIGNAL] Java signal handler for %s already active in restoreJavaSignalHandlers",
                   signalAttrTable[i].sigName);
       continue;
    }

    sigAct.sa_handler = signalAttrTable[i].javaSigAction.sa_handler;
    sigAct.sa_mask = signalAttrTable[i].javaSigAction.sa_mask;
    sigAct.sa_flags = signalAttrTable[i].javaSigAction.sa_flags;
    if (sigaction (signalAttrTable[i].sigType, &sigAct, (struct sigaction*)NULL)) {
      cerr << "[MXUDR DEBUG SIGNAL HANDLER] " 
	   << "sigaction() failed, cannot restore Java signal handler for " 
	   << signalAttrTable[i].sigName << endl;
      return FALSE;
    }
  }
  return TRUE;
}

// Restores the UDR server signal handlers to original signal handlers after returning
// from  Java code.  Also saves the current java signal handlers optionally.
NABoolean restoreUdrTrapSignalHandlers(NABoolean saveJavaSignalHandlers)
{
  UDR_DEBUG0("[SIGNAL] Restore UDR Trap signal handlers after returning from Java LM");
  struct sigaction oldAction, sigAct;

  sigemptyset(&(sigAct.sa_mask));
  sigAct.sa_flags   = 0;

  for(Int32 i=0; i<sizeof(signalAttrTable)/sizeof(SignalAttr); i++){
    if (sigaction (signalAttrTable[i].sigType, (struct sigaction*)NULL, &oldAction)) {
      cerr << "[MXUDR DEBUG SIGNAL HANDLER] " 
	   << "sigaction() failed, cannot obtain old signal handler for " 
	   << signalAttrTable[i].sigName << endl;
      return FALSE;
    }
    if ( oldAction.sa_handler == signalAttrTable[i].udrSigAction.sa_handler ) {
       UDR_DEBUG1("[SIGNAL] UDR trap signal handler for %s already active in restoreUdrTrapSignalHandlers",
                    signalAttrTable[i].sigName);
       continue;
    }

    sigAct.sa_handler = signalAttrTable[i].udrSigAction.sa_handler;
    sigAct.sa_mask = signalAttrTable[i].udrSigAction.sa_mask;
    sigAct.sa_flags = signalAttrTable[i].udrSigAction.sa_flags;
    if (sigaction (signalAttrTable[i].sigType, &sigAct, (struct sigaction*)NULL)) {
      cerr << "[MXUDR DEBUG SIGNAL HANDLER] " 
	   << "sigaction() failed, cannot restore UDR trap signal handler for " 
	   << signalAttrTable[i].sigName << endl;
      return FALSE;
    }
    else {
      if (saveJavaSignalHandlers == TRUE) {
        signalAttrTable[i].javaSigAction.sa_handler = oldAction.sa_handler;
        signalAttrTable[i].javaSigAction.sa_mask = oldAction.sa_mask;
        signalAttrTable[i].javaSigAction.sa_flags = oldAction.sa_flags;
      }
    }
  }
  return TRUE;
}


// Udr exit handler routine
// Main purpose of this handler routine is to check if the UDR called exit
// which it is not supposed to do. In case UDR called exit, we log an
// event message and abort.
void UdrExitHandler()
{
  const char *msg1 = COPYRIGHT_UDRSERV_PRODNAME_H " Exit Handler";
  char msg2[TEXT_SIZE];
  char msg3[2 * TEXT_SIZE]; // to accommodate 3-part ANSI name for routine

  UDR_DEBUG0("[EXIT HANDLER] Entered MXUDR Exit handler");

  NABoolean isRoutineActive;
  getActiveRoutineInfoMsg(msg3, isRoutineActive);
  // If the UDR was active and it called exit intentionally, 
  // log a message and capture TFDS state.
  if (isRoutineActive == TRUE) {
    UDR_DEBUG0("[EXIT HANDLER] UDR invoked exit()");
    strcpy(msg2, "UDR invoked exit() "); 
    comTFDS(msg1, msg2, msg3, NULL, NULL, FALSE, FALSE);
  }

  // Call MPI exit handler routine from this exit handler
  my_mpi_fclose();

}

void setExitHandler()
{

  if (getenv("SQLMX_UDR_NO_EXIT_HANDLER"))
  {
    return;
  }

  UDR_DEBUG0("[EXIT HANDLER] Setting up the MXUDR Exit handler");

  Int32 rc = atexit(UdrExitHandler);

    UDR_DEBUG0("[EXIT HANDLER] Completed setting up the MXUDR Exit handler");

}

