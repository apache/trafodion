/* -*-C++-*-
***********************************************************************
*
* File:         OptimizerSimulator.h
* Description:  This file is the header file for Optimizer Simulator
*               component (OSIM).
*
* Created:      12/2006
* Language:     C++
*
*
**********************************************************************/
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

#ifndef __OPTIMIZERSIMULATOR_H
#define __OPTIMIZERSIMULATOR_H

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fstream>
#include <CmpCommon.h>
#include "BaseTypes.h"
#include "CollHeap.h"

// forward declaration to allow usage of NATable *
class NATable;

// Define PATH_MAX, FILENAME_MAX, LINE_MAX for both NT and NSK.
  // _MAX_PATH and _MAX_FNAME are defined in stdlib.h that is
  // included above.
  #define OSIM_PATHMAX PATH_MAX
  #define OSIM_FNAMEMAX FILENAME_MAX
  #define OSIM_LINEMAX 2048

// This class initializes OSIM and implements capture and simulation
// methods needed for OSIM.
class OptimizerSimulator : public NABasicObject
{
  public:
    // The default OSIM mode is OFF and is set to either CAPTURE or
    // SIMULATE by an environment variable OSIM_MODE.
    enum osimMode {
      OFF,
      CAPTURE,
      SIMULATE
    };

    enum sysCall {
      FIRST_SYSCALL,
      NODENAME_TO_NODENUMBER_=FIRST_SYSCALL,
      NODENUMBER_TO_NODENAME_,
      FILENAME_TO_PROCESSHANDLE_,
      PROCESSHANDLE_DECOMPOSE_,
      REMOTEPROCESSORSTATUS,
      FILENAME_DECOMPOSE_,
      MYSYSTEMNUMBER,
      GETSYSTEMNAME,
      getEstimatedRows,
      getJustInTimeStatsRowcount,
      getNodeAndClusterNumbers,
      // following are not system calls
      // following are files used to log information
      VIEWSFILE,
      TABLESFILE,
      SYNONYMSFILE,
      DEFAULT_DEFAULTSFILE,
      CQD_DEFAULTSFILE,
      DERIVED_DEFAULTSFILE,
      DEF_TABLE_DEFAULTSFILE,
      COMPUTED_DEFAULTSFILE,
      UNINITIALIZED_DEFAULTSFILE,
      IMMUTABLE_DEFAULTSFILE,
      PLATFORM_DEFAULTSFILE,
      QUERIESFILE,
      VPROCFILE,
      CAPTURE_SYS_TYPE,
      LAST_SYSCALL=CAPTURE_SYS_TYPE,
      NUM_OF_SYSCALLS
    };

    // sysType indicates the type of system that captured the queries
    enum sysType {
      OSIM_UNKNOWN_SYSTYPE,
      OSIM_NSK,
      OSIM_WINNT,
      OSIM_LINUX
    };

    // Constructor
    OptimizerSimulator(CollHeap *heap);

    // Accessor methods to get and set osimMode_.
    void setOsimMode(osimMode mode) { osimMode_ = mode; }
    osimMode getOsimMode() { return osimMode_; }

    // Accessor methods to get and set osimLogdir_.
    void setOsimLogdir(const char *logdir) {
      if (logdir) {
        osimLogdir_ = new (heap_) char[strlen(logdir)+1];
        strcpy(osimLogdir_, logdir);
      } else {
        osimLogdir_ = NULL;
      }
    }
    char* getOsimLogdir() { return osimLogdir_; }

    void initOptimizerSimulator();
    void initHashDictionaries();
    void setLogFilepath(sysCall sc);
    void openAndAddHeaderToLogfile(sysCall sc);
    void initSysCallLogfiles();
    void readSysCallLogfiles();
    void captureSystemInfo();

    void captureSysType();
    sysType getCaptureSysType();
    void readLogFile_captureSysType();

    void capture_NODENAME_TO_NODENUMBER_(short error,
                                         const char *nodeName,
                                         short nodeNameLen,
                                         Int32 *nodeNumber);
    void readLogfile_NODENAME_TO_NODENUMBER_();
    short simulate_NODENAME_TO_NODENUMBER_(const char *nodeName,
                                           short nodeNameLen,
                                           Int32 *nodeNumber);

    void capture_NODENUMBER_TO_NODENAME_(short error,
                                         Int32 nodeNumber,
                                         char *nodeName,
                                         short maxLen,
                                         short *actualLen);
    void readLogfile_NODENUMBER_TO_NODENAME_();
    short simulate_NODENUMBER_TO_NODENAME_(Int32 nodeNumber,
                                           char *nodeName,
                                           short maxLen,
                                           short *actualLen);

    void capture_FILENAME_TO_PROCESSHANDLE_(short error,
                                            char *fileName,
                                            short length,
                                            short procHandle[]);
    void readLogfile_FILENAME_TO_PROCESSHANDLE_();
    short simulate_FILENAME_TO_PROCESSHANDLE_(char *fileName,
                                              short length,
                                              short procHandle[]);

    void capture_PROCESSHANDLE_DECOMPOSE_(short error,
                                          short procHandle[],
                                          short *cpu,
                                          short *pin,
                                          Int32 *nodeNumber);
    void readLogfile_PROCESSHANDLE_DECOMPOSE_();
    short simulate_PROCESSHANDLE_DECOMPOSE_(short procHandle[],
                                            short *cpu,
                                            short *pin,
                                            Int32 *nodeNumber);

    void capture_REMOTEPROCESSORSTATUS(Lng32 status,
                                       short clusterNum);
    void log_REMOTEPROCESSORSTATUS();
    void readLogfile_REMOTEPROCESSORSTATUS();
    Lng32 simulate_REMOTEPROCESSORSTATUS(short clusterNum);

    void capture_FILENAME_DECOMPOSE_(short error,
                                     char *fileName,
                                     short fileNameLen,
                                     char *partName,
                                     short maxLen,
                                     short *partNameLen,
                                     short level,
                                     short options=0,
                                     short subpart=0);
    void readLogfile_FILENAME_DECOMPOSE_();
    short simulate_FILENAME_DECOMPOSE_(char *fileName,
                                       short fileNameLen,
                                       char *partName,
                                       short maxLen,
                                       short *partNameLen,
                                       short level,
                                       short options=0,
                                       short subpart=0);

    void capture_MYSYSTEMNUMBER(short sysNum);
    void readLogfile_MYSYSTEMNUMBER();
    short simulate_MYSYSTEMNUMBER();

    void capture_GETSYSTEMNAME(short error, short sysNum, short *sysName);
    void readLogfile_GETSYSTEMNAME();
    short simulate_GETSYSTEMNAME(short sysNum, short *sysName);

    void capture_getEstimatedRows(const char *tableName, double estRows);
    void readLogfile_getEstimatedRows();
    double simulate_getEstimatedRows(const char *tableName);

    void capture_getJustInTimeStatsRowcount(const char *queryText, double rowCount, float sampleFraction, double rowsInTable);
    void readLogfile_getJustInTimeStatsRowcount();
    void simulate_getJustInTimeStatsRowcount(const char *queryText, double *rowCount, float *sampleFraction, double *rowsInTable);

    void capture_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum);
    void log_getNodeAndClusterNumbers();
    void readLogFile_getNodeAndClusterNumbers();
    void simulate_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum);

    void capture_CQDs();
    void capture_TableOrView(NATable * naTab);

    void capturePrologue();
    void captureQueryText(const char * query);
    void captureQueryShape(const char * shape);
    void captureVPROC();
    void cleanup();
    void cleanupSimulator();
    void cleanupAfterStatement();

    void errorMessage(const char *msg);
    void warningMessage(const char *msg);

    NABoolean setOsimModeAndLogDir(osimMode mode,
                                   const char * logdir,
                                   NABoolean usingCaptureHint,
                                   NABoolean calledFromConstructor=FALSE);

    NABoolean isCallDisabled(ULng32 callBitPosition);

    NABoolean runningSimulation();
    NABoolean runningInCaptureMode();

  private:
    NABoolean createLogDir(const char * logDir);
    // This is the directory OSIM uses to read/write log files.
    // It is set by an environment variable OSIM_LOGDIR.
    char *osimLogdir_;
    // This is the mode under which OSIM is running (default is OFF).
    // It is set by an environment variable OSIM_MODE.
    osimMode osimMode_;

    // System call names.
    static const char *sysCallName[NUM_OF_SYSCALLS];
    // File pathnames for log files that contain system call data.
    char *sysCallLogfile[NUM_OF_SYSCALLS];
    ofstream *writeSysCallStream[NUM_OF_SYSCALLS];

    // Hash Dictionaries corresponding to all system calls.
    NAHashDictionary<NAString, NAString> *hashDict_NODENAME_TO_NODENUMBER_;
    NAHashDictionary<Int32, NAString> *hashDict_NODENUMBER_TO_NODENAME_;
    NAHashDictionary<NAString, NAString> *hashDict_FILENAME_TO_PROCESSHANDLE_;
    NAHashDictionary<NAString, NAString> *hashDict_PROCESSHANDLE_DECOMPOSE_;
    NAHashDictionary<short, Lng32> *hashDict_REMOTEPROCESSORSTATUS;
    NAHashDictionary<NAString, NAString> *hashDict_FILENAME_DECOMPOSE_;
    NAHashDictionary<short, NAString> *hashDict_GETSYSTEMNAME;
    NAHashDictionary<NAString, double> *hashDict_getEstimatedRows;
    NAHashDictionary<NAString, NAString> *hashDict_getJustInTimeStatsRowcount;
    
    NAHashDictionary<NAString, Int32> *hashDict_Views;
    NAHashDictionary<NAString, Int32> *hashDict_Tables;
    NAHashDictionary<NAString, Int32> *hashDict_Synonyms;

    short nodeNum_;
    Int32 clusterNum_;
    short mySystemNumber_;
    sysType captureSysType_;
    NABoolean capturedNodeAndClusterNum_;
    NABoolean capturedInitialData_;
    NABoolean usingCaptureHint_;
    NABoolean hashDictionariesInitialized_;

    //for debugging
    ULng32 sysCallsDisabled_;

    CollHeap *heap_;
};

// System call wrappers.
  short OSIM_NODENAME_TO_NODENUMBER_(const char *nodeName,
                                     short nodeNameLen,
                                     Int32 *nodeNumber);
  short OSIM_NODENUMBER_TO_NODENAME_(Int32 nodeNumber,
                                     char *nodeName,
                                     short maxLen,
                                     short *actualLen);
  short OSIM_FILENAME_TO_PROCESSHANDLE_(char *fileName,
                                        short length,
                                        short procHandle[]);
  short OSIM_PROCESSHANDLE_DECOMPOSE_(short procHandle[],
                                      short *cpu,
                                      short *pin,
                                      Int32 *nodeNumber);
  Lng32 OSIM_REMOTEPROCESSORSTATUS(short clusterNum);
  short OSIM_FILENAME_DECOMPOSE_(char *fileName,
                                 short fileNameLen,
                                 char *partName,
                                 short maxLen,
                                 short *partNameLen,
                                 short level,
                                 short options=0,
                                 short subpart=0);

  short OSIM_GETSYSTEMNAME(short sysNum, short *sysName);
  short OSIM_MYSYSTEMNUMBER();
  void  OSIM_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum);
  void  OSIM_captureTableOrView(NATable * naTab);
  void  OSIM_capturePrologue();
  void  OSIM_captureQueryText(const char * query);
  void  OSIM_captureQueryShape(const char * shape);
  // errorMessage and warningMessage wrappers.
  void OSIM_errorMessage(const char *msg);
  void OSIM_warningMessage(const char *msg);

  NABoolean OSIM_runningSimulation();
  NABoolean OSIM_runningInCaptureMode();
  NABoolean OSIM_catIsDisabled();
  NABoolean OSIM_ustatIsDisabled();

  NABoolean OSIM_isNTbehavior();
  NABoolean OSIM_isNSKbehavior();
  NABoolean OSIM_isLinuxbehavior();

  //Stubs for NT & LINUX
  Lng32 REMOTEPROCESSORSTATUS(short clusterNum);

#endif
