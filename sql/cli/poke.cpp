/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<unistd.h>
#include<cextdecs.h(BREAKMESSAGE_SEND_, FILENAME_TO_PATHNAME_, OSS_PID_NULL_, \
		    PROCESSHANDLE_GETMINE_, \
                    PROCESSHANDLE_DECOMPOSE_, PROCESS_GETINFO_, PROCESSSTRING_SCAN_, STRING_UPSHIFT_)>
#include<cextdecs.h(CONVERTTIMESTAMP, INTERPRETTIMESTAMP, PROCESS_GETINFOLIST_)>
#include<cextdecs.h(FILE_OPEN_, FILE_CLOSE_, WRITEREADX, PROCESSHANDLE_TO_FILENAME_)>
#include<tdmext.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<tal.h>
#include"PortProcessCalls.h"

bool ipc = false;

void ShowProc(char *processString, short dateAndTime[8], char homeTerm[64], char progOrPathName[64]);

class Memory;
class Target {
  friend class Memory;
private:
  char processString_[24];
  short error, processHandle_[10];
  short cpu_;
  short pin_;
  char homeTerm_[64];
  char name_[64];
  short creationTime_[8];
public:
  Int32 Set(char *processString, Int32 quiet = false);
  Int32 Break();
  Int32 Signal(Int32 cmdNum);
};

class Memory {
private:
  short processHandle_[10];
  char processString_[24];
  short cpu_;
  short pin_;
  short creationTime_[8];
  char homeTerm_[64];
  char name_[64];
  short fileError_;
  short fNum_;
public:
  Memory() : fNum_(-1) {}
  void StartDisplay(Int32 isTarget, Target *target, Int32 debugFlavor, Int32 summary);
  void Stop();
  void Pause(Int32 isTarget);
};

Memory memory;
Int32 debugFlavor = true;
int longOpt = false;

class ProcessEntry {
public:
  Lng32 shown_;
  Int64 julian_;
  char processString_[24];
//  short dateAndTime_[8];
//  char homeTerm_[64];
//  char progOrPathName_[64];
};

class ProcessEntryArray {
public:
  ProcessEntryArray(Int32 numElements) :
    numElements_(numElements),
    numEntries_(0)
  {
    processEntryArray_ = new ProcessEntry[numElements_];
  }
  ~ProcessEntryArray()
  {
    delete [] processEntryArray_;
  }
  void add(Int64 julian, char processString[24]/*, short dateAndTime[8], char homeTerm[64], char progOrPathName[64]*/);
  void show();

  Int32	numElements_;
  Int32	numEntries_;
  ProcessEntry *processEntryArray_ ;
};

void ProcessEntryArray::add(Int64 julian,
			    char processString[24]/*,
			    short dateAndTime[8],
			    char homeTerm[64],
			    char progOrPathName[64]*/)
  {
    processEntryArray_[numEntries_].shown_ = false;
    processEntryArray_[numEntries_].julian_ = julian;
//    strcpy(processEntryArray_[numEntries_].processString_, processString);
    strcpy(processEntryArray_[numEntries_++].processString_, processString);
/*    for (int i = 0; i < 8; i++)
      processEntryArray_[numEntries_].dateAndTime_[i] = dateAndTime[i];
    strcpy(processEntryArray_[numEntries_].homeTerm_, homeTerm);
    strcpy(processEntryArray_[numEntries_++].progOrPathName_, progOrPathName);*/
  }
void ProcessEntryArray::show()
  {
    Target target;
    Int32 isTarget;
    Int64 lowestJulian;
    Int32 lowestElement;
    for (Int32 i = 0; i < numEntries_; i++) {
      lowestJulian = 0x7FFFFFFFFFFFFFFF;
      for (Int32 j = 0; j < numEntries_; j++) {
      
  //      ShowProc(processEntryArray_[i].processString_,
  //               processEntryArray_[i].dateAndTime_,
  //               processEntryArray_[i].homeTerm_,
  //               processEntryArray_[i].progOrPathName_);
	if (processEntryArray_[j].shown_ == false &&
	    processEntryArray_[j].julian_ < lowestJulian) {
	  lowestJulian = processEntryArray_[j].julian_;
	  lowestElement = j;
	}
      }
      processEntryArray_[lowestElement].shown_ = true;
      isTarget = target.Set(processEntryArray_[lowestElement].processString_, true);
      memory.StartDisplay(isTarget, &target, debugFlavor, longOpt ? false : true);
      memory.Stop();
    }
  }

enum cmd{ error_ = 0,
    abend_   = 1,
    break_   = 2,
    debug_   = 3,
    exit_    = 4,
    help_    = 5,
    int_     = 6,
    ipc_     = 7,
    kill_    = 8,
    memory_  = 9,
    pause_   = 10,
    quit_    = 11,
    release_ = 12,
    show_    = 13,
    stop_    = 14,
    target_  = 15,
    term_    = 16};

enum opt {homeTermOpt_ = 1,
          helpOpt_    = 2,
	  ipcOpt_     = 3,
	  longOpt_    = 4,
	  releaseOpt_ = 5};

const Int32 cmdCount = 16, maxCmdStrLen = 7;

const Int32 optCount = 5, maxOptStrLen = 2;

char cmdStr[cmdCount][maxCmdStrLen+1] =
  { "ABEND",
    "BREAK",
    "DEBUG",
    "EXIT;",
    "HELP",
    "INT",
    "IPC",
    "KILL",
    "MEMORY",
    "PAUSE",
    "QUIT",
    "RELEASE",
    "SHOW",
    "STOP",
    "TARGET",
    "TERM"};

char optStr[optCount][maxOptStrLen+1] =
  { "-g",
    "-h",
    "-i",
    "-l",
    "-r"};

Int32 GetCmd(char *cmdStrArg) {
#pragma nowarn(1506)   // warning elimination 
  STRING_UPSHIFT_(cmdStrArg, strlen(cmdStrArg), cmdStrArg, strlen(cmdStrArg));
#pragma warn(1506)  // warning elimination 
  if (strlen(cmdStrArg) > maxCmdStrLen)
    return error_;
  for (Int32 i = 0; i < cmdCount; )
    if (!memcmp(cmdStrArg, cmdStr[i++], strlen(cmdStrArg))) {
      if (i == cmdCount || memcmp(cmdStrArg, cmdStr[i], strlen(cmdStrArg)))
        return i;
      else
        return error_;
      }
  return error_;
}

Int32 GetOpt(char *optStrArg) {
  if (strlen(optStrArg) > maxOptStrLen)
    return error_;
  for (Int32 i = 0; i < optCount; )
    if (!memcmp(optStrArg, optStr[i++], strlen(optStrArg))) {
      if (i == optCount || memcmp(optStrArg, optStr[i], strlen(optStrArg)))
        return i;
      else
        return error_;
      }
  return error_;
}

#pragma nowarn(770)   // warning elimination 
void GetCreationTime(short processHandle[10], short creationTime[8]) {
  short fileError, retValLen;
  Lng32 retDateTime;
  Int64 julian, local;
  short ctAttr = 53;
  fileError = PROCESS_GETINFOLIST_(,,,,processHandle,&ctAttr,1,(short *)&julian,4,&retValLen); 
  local = CONVERTTIMESTAMP(julian); 
  retDateTime = INTERPRETTIMESTAMP(local, creationTime);
}
#pragma warn(770)  // warning elimination 

void ShowProc(char *processString, short dateAndTime[8], char homeTerm[64], char progOrPathName[64]) {
  char buffer[200];
  sprintf(buffer, "%s %02u/%02u/%u %02u:%02u:%02u %s %s \n",
          processString,
          dateAndTime[1], dateAndTime[2], dateAndTime[0],
          dateAndTime[3], dateAndTime[4], dateAndTime[5],
          homeTerm,
          progOrPathName);
  cout << buffer;
//  cout << "" << processString << " "
//       << dateAndTime[1] << "/" << dateAndTime[2] << "/" << dateAndTime[0] << " "
//       << dateAndTime[3] << ":" << dateAndTime[4] << ":" << dateAndTime[5] << " "
//       << homeTerm <<  " " << progOrPathName << endl;
}

void Memory::Stop() {
  char buffer[8];
  short countRead;
  _cc_status CC;
  short fileErr;
  if (fNum_ != -1) {
    strcpy(buffer, "q");
#pragma nowarn(1506)   // warning elimination 
    CC = WRITEREADX(fNum_, buffer, strlen(buffer) + 1, 0, &countRead);
#pragma warn(1506)  // warning elimination 
    if (CC)
      cout << "WRITEREADX returned " << CC << endl;
    fileErr = FILE_CLOSE_(fNum_);
    if (fileErr)
      cout << "FILE_CLOSE of spawned process returned " << fileErr << endl;
    fNum_ = -1;
  }
}

#pragma nowarn(770)   // warning elimination 
void Memory::StartDisplay(Int32 isTarget, Target *target, Int32 debugFlavor, Int32 summary = false) {

  process_extension procExt = DEFAULT_PROCESS_EXTENSION;
  process_extension_results procRes = DEFAULT_PROCESS_EXTENSION_RESULTS;
  inheritance inherit;
  extern char **environ;
  Int32 fd_count, fd_map[3];
  char *argv[3];
  char buf[9];
  if (debugFlavor)
    strcpy(buf, "muse");
  else
    strcpy(buf, "muse_SRL");
  char pokeSpawned[] = "ZZPOKESPAWNED";
  argv[0] = buf;
  argv[1] = pokeSpawned;
  argv[2] = 0;
  pid_t pid;
  _cc_status CC;
  short fileErr, fNameLen;
  char fName[64];
  char *processName;
  short countRead;
  char buffer[32];
  short creationTime[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  if (!isTarget)
    cout << "TARGET has not been specified" << endl;
  else {
    if (fNum_ == -1) {
      procExt.pe_name_options = _TPC_GENERATE_NAME;
      inherit.flags = SPAWN_SETGROUP;
      inherit.pgroup = SPAWN_NEWPGROUP;
      fd_map[0] = 0; /* Note 2 */
      fd_map[1] = 1;
      fd_map[2] = 2;
      fd_count = 3;
      procExt.pe_cpu = target->cpu_;
      pid = tdm_spawnp(buf,
                       fd_count,
                       0,
                       &inherit,
                       argv,
                       environ,
                       &procExt,
                       &procRes);
      if (pid < 0)
        cout << "Could not spawn memory monitor process" << endl;
      else {
    //    pid = waitpid(pid, &status, 0);
    //    if (pid < 0)
    //      cout << "Error waiting for memory monitor process" << endl;
        fileErr = PROCESSHANDLE_TO_FILENAME_(procRes.pr_phandle,fName,64,&fNameLen,1);
        fName[fNameLen] = '\0';
        processName = fName;
        while (*processName != '$') {
          processName += 1;
        }
#pragma nowarn(1506)   // warning elimination 
        fileErr = FILE_OPEN_(processName, strlen(processName), &fNum_);
#pragma warn(1506)  // warning elimination 
        if (fileErr) {
          cout << "FILE_OPEN_ of spawned process returned " << fileErr << endl;
          cout.flush();
        }
        strcpy(processString_, target->processString_);
        memcpy(creationTime_, target->creationTime_, sizeof(creationTime_));
        memcpy(processHandle_, target->processHandle_, sizeof(processHandle_));
        strcpy(homeTerm_, target->homeTerm_);
        strcpy(name_, target->name_);
        ShowProc(processString_, creationTime_, homeTerm_, name_);
	cout.flush();
	if (summary) {
	  sprintf(buffer, "s");
#pragma nowarn(1506)   // warning elimination 
	  CC = WRITEREADX(fNum_, buffer, strlen(buffer) + 1, 2, &countRead);
#pragma warn(1506)  // warning elimination 
	  if (CC == 0 && countRead == 2 && *(short *)buffer != 0)
	    Stop();
	  else if (CC)
	    cout << "WRITEREADX returned " << CC << endl;
	}
	if (ipc)
	{
	  sprintf(buffer, "ipc\0");
	  CC = WRITEREADX(fNum_, buffer, strlen(buffer) + 1, 2, &countRead);
#pragma warn(1506)  // warning elimination 
	  if (CC == 0 && countRead == 2 && *(short *)buffer != 0)
	    Stop();
	  else if (CC)
	    cout << "WRITEREADX returned " << CC << endl;
	}
        sprintf(buffer, "pi %d\0", target->pin_);
#pragma nowarn(1506)   // warning elimination 
        CC = WRITEREADX(fNum_, buffer, strlen(buffer) + 1, 2, &countRead);
#pragma warn(1506)  // warning elimination 
        if (CC == 0 && countRead == 2 && *(short *)buffer != 0)
          Stop();
        else if (CC)
          cout << "WRITEREADX returned " << CC << endl;
      }
    }
    else {
      ShowProc(processString_, creationTime_, homeTerm_, name_);
      GetCreationTime(processHandle_, creationTime);
      if (memcmp(creationTime, creationTime_, sizeof(creationTime)))
        cout << "*** Process has terminated ***" << endl;
      strcpy(buffer, "d");
#pragma nowarn(1506)   // warning elimination 
      CC = WRITEREADX(fNum_, buffer, strlen(buffer) + 1, 0, &countRead);
#pragma warn(1506)  // warning elimination 
      if (CC)
        cout << "WRITEREADX returned " << CC << endl;
    }
  }
}
#pragma warn(770)  // warning elimination 

void Memory::Pause(Int32 isTarget) {
  char buffer[32];
  short countRead;
  _cc_status CC;

  if (!isTarget)
    cout << "TARGET has not been specified" << endl;
  else if (fNum_ == -1)
    cout << "Memory monitoring has not be activated" << endl;
  else {
      strcpy(buffer, "pa");
#pragma nowarn(1506)   // warning elimination 
      CC = WRITEREADX(fNum_, buffer, strlen(buffer) + 1, 0, &countRead);
#pragma warn(1506)  // warning elimination 
      if (CC)
        cout << "WRITEREADX returned " << CC << endl;
  }
}

Int32 Target::Set(char *processString, Int32 quiet) {
  short progFileLen, pathLen, homeTermLen;
  short fileError, lengthUsed, nullHandle[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  Int32 success = 0;
  pid_t pid; 
#pragma nowarn(1506)   // warning elimination 
  fileError = PROCESSSTRING_SCAN_(processString,
                                         strlen(processString),
                                         &lengthUsed,
                                         processHandle_);
#pragma warn(1506)  // warning elimination 


//Phandle wrapper in porting layer
  NAProcessHandle phandle((short *)&processHandle_);

  if (fileError || !memcmp(processHandle_, nullHandle, 20)) {
    cout << "Null process handle (error " << fileError << ") on process string " << processString << endl;
  }
  else {
    fileError = PROCESS_GETINFO_(processHandle_,,,,,,homeTerm_, 64, &homeTermLen,,,,,,name_, 64, &progFileLen);
    if (!fileError) {
       fileError = phandle.decompose();
       if (!fileError) {       
         cpu_ = phandle.getCpu();
         pin_ = phandle.getPin();
       }
    }
    if (!fileError) {
      strcpy(processString_, processString);
      success = 1;
      name_[progFileLen] = '\0';
      homeTerm_[homeTermLen] = '\0';
      fileError = PROCESS_GETINFO_(processHandle_,,,,,,,,,,,,,,,,,,,,,, (Lng32 *)&pid);
      if (!((Lng32)pid == OSS_PID_NULL_())) {
        fileError = FILENAME_TO_PATHNAME_(name_, progFileLen, name_, 64, &pathLen);
        if (!fileError) {
          name_[pathLen] = '\0';
        }
      }
      GetCreationTime(processHandle_, creationTime_);
      if (!quiet)
        ShowProc(processString_, creationTime_, homeTerm_, name_);
    }
  }
  return success;
}

Int32 Target::Break() {
  short fileError;
  short success = 0;
    if (Set(processString_)) {
      fileError = BREAKMESSAGE_SEND_(processHandle_, 0);
      if (fileError)
        cout << "File error " << fileError << "sending break message" << endl;
      else {
        cout << "BREAK message sent" << endl;
        success = 1;
      }
    }
    else
      cout << "TARGET process has gone bad" << endl;
    return success;
}

Int32 Target::Signal(Int32 cmdNum) {
  extern Int32 errno; 
  Int32 signal =  cmdNum == abend_ ? SIGABEND :
                cmdNum == int_ ? SIGINT :
                cmdNum == kill_ ? SIGKILL : 
                cmdNum == stop_ ? SIGSTOP : SIGTERM;
  Int32 retVal;
  short fileError;
  short success = 0;
  pid_t pid; 
    if (Set(processString_)) {
      fileError = PROCESS_GETINFO_(processHandle_,,,,,,,,,,,,,,,,,,,,,, (Lng32 *)&pid);
      if (fileError)
        cout << "File error " << fileError << "getting process id" << endl;
      else {
        success = 1;
        retVal = kill(pid, signal);
        if (retVal)
           cout << "Error number " << errno << " occurred" << endl;
        else
          cout << "Signal sent" << endl;
      }
    }
    else
      cout << "TARGET process has gone bad" << endl;
    return success;
}

void DoHelp() {
  cout << "ABEND                            Send SIGABEND signal" << endl;
  cout << "BREAK                            Send BREAK message" << endl;
  cout << "DEBUG                            Set debug flavor memory monitoring (default)" << endl;
  cout << "EXIT/QUIT                        Exit POKE program" << endl;
  cout << "INT                              Send SIGINT (ctl-c) signal" << endl;
  cout << "KILL                             Send SIGKILL signal" << endl;
  cout << "MEMORY                           Activate memory monitoring or update usage" << endl;
  cout << "PAUSE                            Toggle executor memory semaphore" << endl;
  cout << "RELEASE                          Set release flavor memory monitoring" << endl;
  cout << "SHOW                             Show SQL/MX processes" << endl;
  cout << "STOP                             Send SIGSTOP signal" << endl;
  cout << "TARGET [cpu,pin | $process-name] Specify target process" << endl;
  cout << "TERM                             Send SIGTERM signal" << endl;
}

void DoShow(char *myHomeTerm) {
  ProcessEntryArray * processEntryArray = new ProcessEntryArray(20000);
  Int32 cpu, isCpu, pin;
  char progFile[64], pathName[64], homeTerm[64];
  char processString[8];
  short fileError, lengthUsed, processHandle[10];
  short nullHandle[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  short progFileLen, pathNameLen, homeTermLen;
  short creationTime[8];
  pid_t pid;
  Int64 julian;
  short retValLen, ctAttr = 53;
  short diff;
  for (cpu = 0; cpu < 16; cpu++) {
    for (pin = 0, isCpu = 1; pin < 1600 && isCpu; pin ++) {
      sprintf(processString, "%d", cpu);
      strcat(processString, ",");
      sprintf(processString + strlen(processString), "%d", pin);
#pragma nowarn(1506)   // warning elimination 
      fileError = PROCESSSTRING_SCAN_(processString,
                                             strlen(processString),
                                             &lengthUsed,
                                             processHandle);
#pragma warn(1506)  // warning elimination 
      if (!fileError && memcmp(processHandle, nullHandle, 20)) {
        fileError = PROCESS_GETINFO_(processHandle,,,,,,homeTerm, 64, &homeTermLen,,,,,,progFile, 64, &progFileLen);
        progFile[progFileLen] = '\0';
        homeTerm[homeTermLen] = '\0';
#pragma nowarn(1506)   // warning elimination 
	if (myHomeTerm)
	  diff = strlen(myHomeTerm) < strlen(homeTerm) ?
	    strlen(homeTerm) - strlen(myHomeTerm) : 0;
#pragma warn(1506)  // warning elimination 
        GetCreationTime(processHandle, creationTime);
        fileError = PROCESS_GETINFO_(processHandle,,,,,,,,,,,,,,,,,,,,,, (Lng32 *)&pid);
        if ((Lng32)pid == OSS_PID_NULL_()) {
          if (!myHomeTerm && (
	      !strcmp(progFile + progFileLen - 7, "INSPECT") ||
              !strcmp(progFile + progFileLen - 5, "MXESP") ||
              !strcmp(progFile + progFileLen - 8, "MXRTDSRV") ||
              !strcmp(progFile + progFileLen - 6, "SQLCAT")))
            ShowProc(processString, creationTime, homeTerm, progFile);
	  else if (myHomeTerm && !strcmp(myHomeTerm, homeTerm + diff) &&
		   !strcmp(progFile + progFileLen - 5, "MXESP")) {
//	    isTarget = target.Set(processString, true);
//	    memory.StartDisplay(isTarget, &target, debugFlavor);
//	    memory.Stop();
	    fileError = PROCESS_GETINFOLIST_(,,,,processHandle,&ctAttr,1,(short *)&julian,4,&retValLen); 
	    processEntryArray->add(julian, processString/*, creationTime, homeTerm, progFile*/);
	  }
        }
        else {
          fileError = FILENAME_TO_PATHNAME_(progFile, progFileLen, pathName, 64, &pathNameLen);
          if (!fileError) {
            pathName[pathNameLen] = '\0';
            if (!myHomeTerm && (
	         !strcmp(pathName + pathNameLen - 4, "mxci") ||
		 !strcmp(pathName + pathNameLen - 8, "mxci_SRL") ||
                 !strcmp(pathName + pathNameLen - 5, "mxcmp")))
	      ShowProc(processString, creationTime, homeTerm, pathName);
	    else if (myHomeTerm && !strcmp(myHomeTerm, homeTerm + diff) &&
		     strcmp(pathName + pathNameLen - 4, "poke") &&
		     strcmp(pathName + pathNameLen - 3, "ksh")) {
//	      isTarget = target.Set(processString, true);
//	      memory.StartDisplay(isTarget, &target, debugFlavor);
//	      memory.Stop();
	      fileError = PROCESS_GETINFOLIST_(,,,,processHandle,&ctAttr,1,(short *)&julian,4,&retValLen);
	      processEntryArray->add(julian, processString/*, creationTime, homeTerm, pathName*/);
	    }
          }
        }
      }
      else if (pin == 0)
        isCpu = 0;
    }
  }
  processEntryArray->show();
}

void sigHandler(Int32 signalType) {
  cout << "Exiting due to ctrl-c" << endl;
  memory.Stop();
  exit(0);
}

void DoOptionHelp() {
  cout << "Usage: poke [ -g [ <home term> ]  ]" << endl;
  cout << "	    [ -r ] [ -l ] [-h]" << endl;
  cout << "" << endl;
  cout << "The -g option is used to obtain executor and ESP heap usage information for the" << endl;
  cout << "group of SQL/MX processes associated with this, or optionally the specified," << endl;
  cout << "home terminal." << endl;
  cout << "" << endl;
  cout << "The -r option indicates that the group of SQL/MX processes are release, rather" << endl;
  cout << "than debug flavor. The -l option is used to obtain long detailed rather than" << endl;
  cout << "short summary heap usage information." << endl;
  cout << "" << endl;
  cout << "The -h option is used to obtain command line option help." << endl;
  cout << "" << endl;
  cout << "\"Flavorless\" scripts can be written by using the SQLMX_FLAVOR=r environment" << endl;
  cout << "variable rather than the -r command line option." << endl;
}

#pragma nowarn(770)   // warning elimination 
main(Int32 argc, char *argv[]) {
  Int32 homeTerm = false;
  short error, homeTermLen, processHandle[10];
  char myHomeTerm[64];
  Target target;
  char cmdLine[80];
  // char argvBuffer[16];
  char *nextPtr;
  char processString[24];

  //Phandle wrapper in porting layer
  NAProcessHandle phandle;

  Int32 isTarget = 0, argNum, cmdNum = error_;
  void (*sigHandlerAddr)(Int32) = sigHandler;
  char * envFlavor = getenv("SQLMX_FLAVOR");
  if (envFlavor && strlen(envFlavor) == 1 && (*envFlavor == 'r' || *envFlavor == 'R'))
    debugFlavor = false;
/*  if (argc == 2) {
    strcpy(argvBuffer, argv[1]);
    if (!strcmp(argvBuffer, "-h")) {
      homeTerm = true;
      return 0;
    }
  }
  else if (argc == 3 && !strcmp(argv[1], "-h")) {
    homeTerm = true;
    strcpy(myHomeTerm, argv[2]);
    DoShow(myHomeTerm);
    return 0;
  }*/
  for (Int32 i = 1; i < argc; i++) {
    argNum = GetOpt(argv[i]);
    switch (argNum) {
    case helpOpt_:
      DoOptionHelp();
      return 0;
    case homeTermOpt_:
      homeTerm = true;
      if ((i + 1) < argc && *argv[i + 1] != '-') {
        i += 1;
	strcpy(myHomeTerm, argv[i]);
      }
      else {
        error = phandle.getmine((short *)&processHandle);
	error = PROCESS_GETINFO_(processHandle,,,,,,myHomeTerm, 64, &homeTermLen);
	myHomeTerm[homeTermLen] = '\0';
      }
      break;
    case ipcOpt_:
      ipc = ~ipc;
      break;
    case longOpt_:
      longOpt = true;
      break;
    case releaseOpt_:
      debugFlavor = false;
      break;
    default:
      cout << argv[i] << " is not a valid argument" << endl;
      return 0;
    }
  }
  if (homeTerm) {
    DoShow(myHomeTerm);
    return 0;
  }
  cout << "Compaq NonStop(TM) SQL/MX Poke Tool 1.2 " << endl;
  DoShow(NULL);
  signal(SIGINT, sigHandlerAddr);
  while (cmdNum != quit_ && cmdNum != exit_) {
    cout << "-> ";
    cin.getline(cmdLine, 80);
    nextPtr = (char *)memchr(cmdLine, ' ', strlen(cmdLine));
    if (nextPtr)
      *nextPtr++ = '\0';
    cmdNum = GetCmd(cmdLine);
    switch (cmdNum) {
    case error_:
      cout << cmdLine << " is not a command" << endl;
      break;
    case break_:
      if (nextPtr != 0)
        cout << "BREAK doesn't have a parameter" << endl;
      else if (!isTarget)
        cout << "TARGET has not been specified" << endl;
      else
        isTarget = target.Break();
      break;
    case target_:
      if (nextPtr == 0) {
        if (isTarget) {
          memory.Stop();
          isTarget = target.Set(processString);
        }
        else
          cout << "TARGET requires cpu,pin or $process-name" << endl;
      }
      else {
        memory.Stop();
        strcpy(processString, nextPtr);
        isTarget = target.Set(processString);
      }
      break;

    case ipc_:
      ipc = ~ipc;
      break;

    case abend_:
    case int_:
    case kill_:
    case stop_:
    case term_:
      if (nextPtr != 0)
        cout << "TERM doesn't have a parameter" << endl;
      else if (!isTarget)
        cout << "TARGET has not been specified" << endl;
      else {
        isTarget = target.Signal(cmdNum);
      }
      break;
    case help_:
      if (nextPtr)
        cout << "HELP doesn't have a parameter" << endl;
      else
        DoHelp();
      break;
    case show_:
      if (nextPtr)
        cout << "SHOW doesn't have a parameter" << endl;
      else
        DoShow(NULL);
      break;
    case memory_:
      if (nextPtr)
        cout << "MEMORY doesn't have a paramenter" << endl;
      else
        memory.StartDisplay(isTarget, &target, debugFlavor);
      break;
    case pause_:
      if (nextPtr)
        cout << "PAUSE doesn't have a parameter" << endl;
      else {
        memory.Pause(isTarget);
      }
      break;
    case exit_:
    case quit_:
      if (nextPtr) {
        cout << "QUIT doesn't have a parameter" << endl;
        cmdNum = error_;
      }
      memory.Stop();
      break;
    case release_:
      if (nextPtr) {
        cout << "RELEASE doesn't have a parameter" << endl;
        cmdNum = error_;
      }
      else {
        memory.Stop();
	debugFlavor = 0;
      }
      break;
    case debug_:
      if (nextPtr) {
        cout << "DEBUG doesn't have a parameter" << endl;
        cmdNum = error_;
      }
      else {
        memory.Stop();
	debugFlavor = 1;
      }
      break;
    }
  }
  return 0;
}
#pragma warn(770)  // warning elimination 
