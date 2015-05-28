/* -*-C++-*-
/**********************************************************************
*
* File:         OptimizerSimulator.cpp
* Description:  This file is the source file for Optimizer Simulator
*               component (OSIM).
*
* Created:      12/2006
* Language:     C++
*
*
**********************************************************************/
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

#include "OptimizerSimulator.h"
#include "NADefaults.h"
#include "CmpContext.h"
#include "CompException.h"
#include "SchemaDB.h"
#include "NATable.h"
#include "ObjectNames.h"
#include "NAClusterInfo.h"
#include "ControlDB.h"
#include "RelControl.h"
#include "CmpStatement.h"
#include "QCache.h"
#include <errno.h>
#include "ComCextdecs.h" 
#include "opt_error.h"
#include "ComRtUtils.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>



#include "vproc.h"

extern THREAD_P NAClusterInfo *gpClusterInfo;
// extern THREAD_P NAClusterInfo * gpClusterInfo;
extern const WordAsBits SingleBitArray[];


ULng32 hashFunc_short(const short& Short)
{
  return (ULng32)Short;
}
ULng32 hashFunc_int(const Int32& Int)
{
  return (ULng32)Int;
}
ULng32 hashFunc_NAString(const NAString& str)
{
  return (ULng32) NAString::hash(str);
}

NABoolean fileExists(const char *filename, NABoolean & isDir)
{
  struct stat sb;
  Int32 rVal = stat(filename, &sb);
  isDir = FALSE;
  if(S_ISDIR(sb.st_mode))
      isDir = TRUE;
  return rVal != -1;
}

const char* OptimizerSimulator::sysCallName[NUM_OF_SYSCALLS]= {
  "NODENAME_TO_NODENUMBER_",
  "NODENUMBER_TO_NODENAME_",
  "FILENAME_TO_PROCESSHANDLE_",
  "PROCESSHANDLE_DECOMPOSE_",
  "REMOTEPROCESSORSTATUS",
  "FILENAME_DECOMPOSE_",
  "MYSYSTEMNUMBER",
  "GETSYSTEMNAME",
  "getEstimatedRows",
  "getJustInTimeStatsRowcount",
  "getNodeAndClusterNumbers",
  // following are not system calls
  // following are files used to log information
  "VIEWS",
  "TABLES",
  "SYNONYMS",
  "DEFAULT_DEFAULTSFILE",
  "CQD_DEFAULTSFILE",
  "DEF_TABLE_DEFAULTSFILE",
  "COMPUTED_DEFAULTSFILE",
  "DERIVED_DEFAULTSFILE",
  "UNINITIALIZED_DEFAULTSFILE",
  "IMMUTABLE_DEFAULTSFILE",
  "PLATFORM_DEFAULTSFILE",
  "QUERIES",
  "VPROC",
  "captureSysType"
};

// This constructor for OptimizerSimulator initializes the
// OSIM mode and log directory after reading the env variables.
OptimizerSimulator::OptimizerSimulator(CollHeap *heap)
:heap_(heap),
 nodeNum_(-1),
 clusterNum_(-1),
 mySystemNumber_(-1),
 capturedNodeAndClusterNum_(FALSE),
 capturedInitialData_(FALSE),
 usingCaptureHint_(FALSE),
 hashDictionariesInitialized_(FALSE),
 sysCallsDisabled_(0),
 osimLogdir_(NULL),
 osimMode_(OptimizerSimulator::OFF),
 hashDict_NODENAME_TO_NODENUMBER_(NULL),
 hashDict_NODENUMBER_TO_NODENAME_(NULL),
 hashDict_FILENAME_TO_PROCESSHANDLE_(NULL),
 hashDict_PROCESSHANDLE_DECOMPOSE_(NULL),
 hashDict_REMOTEPROCESSORSTATUS(NULL),
 hashDict_FILENAME_DECOMPOSE_(NULL),
 hashDict_GETSYSTEMNAME(NULL),
 hashDict_getEstimatedRows(NULL),
 hashDict_getJustInTimeStatsRowcount(NULL),
 hashDict_Views(NULL),
 hashDict_Tables(NULL),
 hashDict_Synonyms(NULL)
{
  for (sysCall sc=FIRST_SYSCALL; sc<NUM_OF_SYSCALLS; sc = sysCall(sc+1))
  {
    sysCallLogfile[sc]=NULL;
    writeSysCallStream[sc]=NULL;
  }

  // Set up Optimizer Simulator
  setOsimModeAndLogDir(osimMode_, osimLogdir_, FALSE, TRUE);
}

// LCOV_EXCL_START :dpm
// Print OSIM error message
void OSIM_errorMessage(const char *msg)
{
  CURRCONTEXT_OPTSIMULATOR->errorMessage(msg);
}

void OptimizerSimulator::errorMessage(const char *msg)
{
  // ERROR message
  *CmpCommon::diags() << DgSqlCode(-OSIM_ERRORORWARNING)
                      << DgString0(msg);
}

// Print OSIM warning message
void OSIM_warningMessage(const char *msg)
{
  CURRCONTEXT_OPTSIMULATOR->warningMessage(msg);
}

void OptimizerSimulator::warningMessage(const char *msg)
{
  // WARNING message
  *CmpCommon::diags() << DgSqlCode(OSIM_ERRORORWARNING)
                      << DgString0(msg);
}
// LCOV_EXCL_STOP

NABoolean OptimizerSimulator::setOsimModeAndLogDir(osimMode targetMode,
                                                   const char * logDir,
                                                   NABoolean usingCaptureHint,
                                                   NABoolean calledFromConstructor)
{
  if (targetMode != OFF)
    {
      errorMessage("OSIM is not supported in this version of the code.");
      return FALSE;
    }

  return TRUE;
}

NABoolean OptimizerSimulator::createLogDir(const char * logDir)
{
  Int32 rval = mkdir(logDir, S_IRWXU | S_IRWXG );

  Int32 error = errno;

  if (rval != 0)
    switch (error)
    {
      case EACCES:
        {
          char errMsg[37+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg), "Could not create directory %s, permission denied.", logDir);
          errorMessage(errMsg);
          return FALSE;
        }
        break;
      case ENOENT:
        {
          char errMsg[58+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg), 
                   "Could not create directory %s, a component of the path does not exist.",
                   logDir);
          errorMessage(errMsg);
          return FALSE;
        }
        break;
      case EROFS:
        {
          char errMsg[40+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg), "Could not create directory %s, read-only filesystem.", logDir);
          errorMessage(errMsg);
          return FALSE;
        }
        break;
      case ENOTDIR:
        {
          char errMsg[62+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg),
                   "Could not create directory %s, a component of the path is not a directory.",
                   logDir);
          errorMessage(errMsg);
          return FALSE;
        }
        break;
      default:
        {
          char errMsg[58+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg),
                   "Could not create %s, errno is %d",
                   logDir, error);
          errorMessage(errMsg);
          return FALSE;
        }
        break;
    }

  return TRUE;

}

void OptimizerSimulator::initOptimizerSimulator()
{
  initHashDictionaries();
  initSysCallLogfiles();

  if (osimMode_ == SIMULATE)
    readSysCallLogfiles();
}

void OptimizerSimulator::initHashDictionaries()
{
  // Initialize hash dictionary variables for all the system calls.
  if(!hashDictionariesInitialized_)
  {
    hashDict_NODENAME_TO_NODENUMBER_ = new(heap_) NAHashDictionary<NAString, NAString>
                                                  (&hashFunc_NAString, 101, TRUE, heap_);
    hashDict_NODENUMBER_TO_NODENAME_ = new(heap_) NAHashDictionary<Int32, NAString>
                                                  (&hashFunc_int, 101, TRUE, heap_);
    hashDict_FILENAME_TO_PROCESSHANDLE_ = new(heap_) NAHashDictionary<NAString, NAString>
                                                     (&hashFunc_NAString, 101, TRUE, heap_);
    hashDict_PROCESSHANDLE_DECOMPOSE_ = new(heap_) NAHashDictionary<NAString, NAString>
                                                   (&hashFunc_NAString, 101, TRUE, heap_);
    hashDict_REMOTEPROCESSORSTATUS = new(heap_) NAHashDictionary<short, Int32 >
                                                (&hashFunc_short, 101, TRUE, heap_);
    hashDict_FILENAME_DECOMPOSE_ = new(heap_) NAHashDictionary<NAString, NAString>
                                              (&hashFunc_NAString, 101, TRUE, heap_);
    hashDict_GETSYSTEMNAME = new(heap_) NAHashDictionary<short, NAString>
                                        (&hashFunc_short, 101, TRUE, heap_);
    hashDict_getEstimatedRows = new(heap_) NAHashDictionary<NAString, double>
                                           (&hashFunc_NAString, 101, TRUE, heap_);
    hashDict_getJustInTimeStatsRowcount = new(heap_) NAHashDictionary<NAString, NAString>
                                           (&hashFunc_NAString, 101, FALSE, heap_);
    hashDict_Views = new(heap_) NAHashDictionary<NAString, Int32>
                                           (&hashFunc_NAString, 101, TRUE, heap_);
    hashDict_Tables = new(heap_) NAHashDictionary<NAString, Int32>
                                           (&hashFunc_NAString, 101, TRUE, heap_);
    hashDict_Synonyms = new(heap_) NAHashDictionary<NAString, Int32>
                                           (&hashFunc_NAString, 101, TRUE, heap_);
    hashDictionariesInitialized_ = TRUE;
  }

}

void OptimizerSimulator::setLogFilepath(sysCall sc)
{
  // Allocate memory for file pathname:
  // dirname + '/' + syscallname + ".txt" + '\0'
  size_t pathLen = strlen(osimLogdir_)+1+strlen(sysCallName[sc])+4+1;
  sysCallLogfile[sc] = new (heap_) char[pathLen];
  // Construct an absolute pathname for the file.
  strcpy(sysCallLogfile[sc], osimLogdir_);
  strcat(sysCallLogfile[sc], "/");
  strcat(sysCallLogfile[sc], sysCallName[sc]);
  strcat(sysCallLogfile[sc], ".txt");
}

void OptimizerSimulator::openAndAddHeaderToLogfile(sysCall sc)
{
  NABoolean isDir;

  if(fileExists(sysCallLogfile[sc],isDir))
  {
    char errMsg1[118+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg1, sizeof(errMsg1), "The target log file %s already exists. "
                     "Delete this and other existing log files before "
                     "running the OSIM in CAPTURE mode.", sysCallLogfile[sc]);
    OsimLogException(errMsg1, __FILE__, __LINE__).throwException();
  }

  // Create the file and write header lines to it.
  ofstream outLogfile(sysCallLogfile[sc]);

  outLogfile << sysCallName[sc] << ":" << endl;
  // Indent the output.
  outLogfile << "  ";
  switch (sc)
  {
    case NODENAME_TO_NODENUMBER_:
      outLogfile.width(5); outLogfile << "ERROR" << "  ";
      outLogfile.width(9);  outLogfile << "NODENAME" << "  ";
      outLogfile.width(10); outLogfile << "NODENUMBER" << "  " << endl;
      break;
    case NODENUMBER_TO_NODENAME_:
      outLogfile.width(5); outLogfile << "ERROR" << "  ";
      outLogfile.width(10); outLogfile << "NODENUMBER" << "  ";
      outLogfile.width(9);  outLogfile << "NODENAME" << "  " << endl;
      break;
    case REMOTEPROCESSORSTATUS:
      outLogfile.width(5); outLogfile << "STATUS" << "  ";
      outLogfile.width(10); outLogfile << "CLUSTERNUM" << "  " << endl;
      break;
    case FILENAME_TO_PROCESSHANDLE_:
      outLogfile.width(5); outLogfile << "ERROR" << "  ";
      outLogfile.width(36); outLogfile << "FILENAME" << "  ";
      outLogfile.width(13); outLogfile << "PROCESSHANDLE" << "  " << endl;
      break;
    case PROCESSHANDLE_DECOMPOSE_:
      outLogfile.width(5); outLogfile << "ERROR" << "  ";
      outLogfile.width(59); outLogfile << "PROCESSHANDLE" << "  ";
      outLogfile.width(3); outLogfile << "CPU" << "  ";
      outLogfile.width(7); outLogfile << "PIN" << "  ";
      outLogfile.width(10); outLogfile << "NODENUMBER" << "  " << endl;
      break;
    case MYSYSTEMNUMBER:
      outLogfile.width(10); outLogfile << "SYSNUM"<< endl;
      break;
    case GETSYSTEMNAME:
      outLogfile.width(5); outLogfile << "ERROR" << "  ";
      outLogfile.width(10); outLogfile << "SYSNUM" << "  ";
      outLogfile.width(10); outLogfile << "SYSNAME"<< endl;
      break;
    case FILENAME_DECOMPOSE_:
      outLogfile.width(5); outLogfile << "ERROR" << "  ";
      outLogfile.width(36); outLogfile << "FILENAME" << "  ";
      outLogfile.width(6);  outLogfile << "LEVEL" << "  ";
      outLogfile.width(7);  outLogfile << "OPTIONS" << "  ";
      outLogfile.width(7);  outLogfile << "SUBPART" << "  ";
      outLogfile.width(36); outLogfile << "PARTNAME" << "  " << endl;
      break;
    case getEstimatedRows:
      outLogfile.width(36); outLogfile << "TABLENAME" << "  ";
      outLogfile.width(8); outLogfile << "ROWCOUNT" << "  " << endl;
      break;
    case getJustInTimeStatsRowcount:
      outLogfile.width(36); outLogfile << "PREDICATE" << "  ";
      outLogfile.width(8); outLogfile << "ROWCOUNT" << "  ";
      outLogfile.width(8); outLogfile << "SAMPLE FRACTION" << "  ";
      outLogfile.width(8); outLogfile << "ROWS IN TABLE" << "  " << endl;
      break;
    case getNodeAndClusterNumbers:
      outLogfile.width(8); outLogfile << "NODENUM" << "  ";
      outLogfile.width(12); outLogfile << "CLUSTERNUM" << "  " << endl;
      break;
    case VIEWSFILE:
    case TABLESFILE:
    case SYNONYMSFILE:
    case DEFAULT_DEFAULTSFILE:
    case CQD_DEFAULTSFILE:
    case DEF_TABLE_DEFAULTSFILE:
    case COMPUTED_DEFAULTSFILE:
    case DERIVED_DEFAULTSFILE:
    case UNINITIALIZED_DEFAULTSFILE:
    case IMMUTABLE_DEFAULTSFILE:
    case PLATFORM_DEFAULTSFILE:
    case QUERIESFILE:
    case VPROCFILE:
    case CAPTURE_SYS_TYPE:
      outLogfile << endl;
      break;
    default:
      break;
  }
  outLogfile.close();
  writeSysCallStream[sc] = new (heap_) ofstream(sysCallLogfile[sc],ios::app);
}

// Initialize the log files if OSIM is running under either CAPTURE
// or SIMULATE mode. If the OSIM is not running under CAPTURE mode,
// add the header lines to the file. Just set the file name variables
// to NULL if OSIM is not running(OFF).
void OptimizerSimulator::initSysCallLogfiles()
{
  for (sysCall sc=FIRST_SYSCALL; sc<NUM_OF_SYSCALLS; sc = sysCall(sc+1))
  {
    switch (osimMode_)
    {
      case OFF:
        // OFF mode indicates no log files needed.
        sysCallLogfile[sc] = NULL;
        break;
      case CAPTURE:
        // Set log file path.
        setLogFilepath(sc);
        // Add header to the log file.
        openAndAddHeaderToLogfile(sc);
        break;
      case SIMULATE:
        // Set log file path.
        setLogFilepath(sc);
        break;
      default:
        // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
        errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
        break;
    }
  }
}

void OptimizerSimulator::readSysCallLogfiles()
{
  readLogfile_NODENAME_TO_NODENUMBER_();
  readLogfile_NODENUMBER_TO_NODENAME_();
  readLogfile_FILENAME_TO_PROCESSHANDLE_();
  readLogfile_PROCESSHANDLE_DECOMPOSE_();
  readLogfile_REMOTEPROCESSORSTATUS();
  readLogfile_FILENAME_DECOMPOSE_();
  readLogfile_MYSYSTEMNUMBER();
  readLogfile_GETSYSTEMNAME();
  readLogfile_getEstimatedRows();
  readLogfile_getJustInTimeStatsRowcount();
  readLogFile_getNodeAndClusterNumbers();
  readLogFile_captureSysType();
}

void OptimizerSimulator::captureSystemInfo()
{
  if (osimMode_ != CAPTURE)
    return;

  // On Linux, all of the information about the cluster is determined in
  // in the NAClusterInfo code.  There is not a reason to call the
  // FILENAME_TO_PROCESSHANDLE_ and PROCESSHANDLE_DECOMPOSE_ code here
  // on Linux at this time.
 
}

// BEGIN *********** System Call: NODENAME_TO_NODENUMBER_() *************
//
void OptimizerSimulator::capture_NODENAME_TO_NODENUMBER_(short error,
                                                         const char *fileName,
                                                         short nodeNameLen,
                                                         Int32 *nodeNumber)
{
  char nodeName[36];
  char errNodeNum[12];

  // filename is in the format \<node_name>.$<volume>.<subvolume>.<file>
  strncpy(nodeName, fileName, nodeNameLen);
  nodeName[nodeNameLen] = '\0';

  // Concatenate error and nodeNumber.
  sprintf(errNodeNum, ":%d:%d:", error, *nodeNumber);

  NAString *key_nodeName = new (heap_) NAString(nodeName, heap_);
  NAString *val_nodeNumber = new (heap_) NAString(errNodeNum, heap_);
  if (hashDict_NODENAME_TO_NODENUMBER_->contains(key_nodeName))
  {
    NAString *chkValue = hashDict_NODENAME_TO_NODENUMBER_->getFirstValue(key_nodeName);
    if(chkValue->compareTo(val_nodeNumber->data()))
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    NAString *check = hashDict_NODENAME_TO_NODENUMBER_->insert(key_nodeName,
                                                               val_nodeNumber);
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[NODENAME_TO_NODENUMBER_];
    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(5); (*outLogfile) << error << "  ";
    (*outLogfile).width(9); (*outLogfile) << nodeName << "  ";
    (*outLogfile).width(10);(*outLogfile) << *nodeNumber << "  " << endl;
    (*outLogfile).width(origWidth);
  }
}

void OptimizerSimulator::readLogfile_NODENAME_TO_NODENUMBER_()
{
  char nodeName[36];
  char errNodeNum[50];
  Int32 nodeNumber;
  short error;
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[NODENAME_TO_NODENUMBER_],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[NODENAME_TO_NODENUMBER_]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[NODENAME_TO_NODENUMBER_]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read nodeName and errNodeNum from the file
    inLogfile >> error >> nodeName >> nodeNumber;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    // Concatenate error and nodeNumber.
    snprintf(errNodeNum, sizeof(errNodeNum), ":%d:%d:", error, nodeNumber);
    NAString *key_nodeName = new (heap_) NAString(nodeName, heap_);
    NAString *val_nodeNumber = new (heap_) NAString(errNodeNum, heap_);
    NAString *check = hashDict_NODENAME_TO_NODENUMBER_->insert(key_nodeName,
                                                               val_nodeNumber);
  }
}

short OptimizerSimulator::simulate_NODENAME_TO_NODENUMBER_(const char *fileName,
                                                           short nodeNameLen,
                                                           Int32 *nodeNumber)
{
  char nodeName[36];
  char errNodeNum[12];
  short error;

  // filename is in the format \<node_name>.$<volume>.<subvolume>.<file>
  strncpy(nodeName, fileName, nodeNameLen);
  nodeName[nodeNameLen] = '\0';

  NAString key_nodeName(nodeName, heap_);
  if (hashDict_NODENAME_TO_NODENUMBER_->contains(&key_nodeName))
  {
    NAString *val_nodeNumber = hashDict_NODENAME_TO_NODENUMBER_->getFirstValue(&key_nodeName);
    strcpy(errNodeNum, val_nodeNumber->data());
    error = (short)atoi(strtok(errNodeNum, ":"));
    *nodeNumber = atoi(strtok(NULL, ":"));
    return error;
  }
  // Should NOT reach here.
  CMPASSERT(FALSE);
  return -1;
}

short OSIM_NODENAME_TO_NODENUMBER_(const char *nodeName, short nodeNameLen, Int32 *nodeNumber)
{
  short error = 0;

  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(1))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
#pragma nowarn(252)   // warning elimination
      error = NODENAME_TO_NODENUMBER_ ((char *) nodeName, nodeNameLen, nodeNumber);
#pragma warn(252)  // warning elimination
      if(CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
        // Capture nodeName and primaryNodeNumber into the log file.
        CURRCONTEXT_OPTSIMULATOR->capture_NODENAME_TO_NODENUMBER_(error, nodeName, nodeNameLen, nodeNumber);
        break;
    case OptimizerSimulator::SIMULATE:
      // Get the primaryNodeNumber for corresponding nodeName from the Hash Dictonary
      // that was build using captured values.
      error = CURRCONTEXT_OPTSIMULATOR->simulate_NODENAME_TO_NODENUMBER_(nodeName, nodeNameLen, nodeNumber);
      break;
    default:
      //The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return error;
}

// END ************* System Call: NODENAME_TO_NODENUMBER_() *************


// BEGIN *********** System Call: NODENUMBER_TO_NODENAME_() *************
//
void OptimizerSimulator::capture_NODENUMBER_TO_NODENAME_(short error,
                                                         Int32 nodeNumber,
                                                         char *nodeNameStr,
                                                         short maxLen,
                                                         short *actualLen)
{
  char nodeName[36];
  char errNodeName[17];

  // Get the nodeName for actualLen bytes.
  strncpy(nodeName, nodeNameStr, *actualLen);
  nodeName[*actualLen] = '\0';

  // Concatenate error and nodeName.
  snprintf(errNodeName, sizeof(errNodeName), ":%d:%s:", error, nodeName);

  Int32 *key_nodeNumber = new Int32(nodeNumber);
  NAString *val_nodeName = new (heap_) NAString(nodeName, heap_);
  if (hashDict_NODENUMBER_TO_NODENAME_->contains(key_nodeNumber))
  {
    NAString *chkValue = hashDict_NODENUMBER_TO_NODENAME_->getFirstValue(key_nodeNumber);
    if(chkValue->compareTo(val_nodeName->data()))
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    Int32 *check = hashDict_NODENUMBER_TO_NODENAME_->insert(key_nodeNumber,
                                                          val_nodeName);
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[NODENUMBER_TO_NODENAME_];

    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(5); (*outLogfile) << error << "  ";
    (*outLogfile).width(10); (*outLogfile) << nodeNumber << "  ";
    (*outLogfile).width(9); (*outLogfile) << nodeName << "  " << endl;
    (*outLogfile).width(origWidth);
  }
}

void OptimizerSimulator::readLogfile_NODENUMBER_TO_NODENAME_()
{
  char nodeName[36];
  char errNodeName[17];
  Int32 nodeNumber;
  short error;
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[NODENUMBER_TO_NODENAME_],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[NODENUMBER_TO_NODENAME_]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[NODENUMBER_TO_NODENAME_]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read nodeNumber and errNodeName from the file
    inLogfile >> error >> nodeNumber >> nodeName;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    // Concatenate error and nodeName.
    snprintf(errNodeName, sizeof(errNodeName), ":%d:%s:", error, nodeName);
    Int32 *key_nodeNumber = new Int32(nodeNumber);
    NAString *val_nodeName = new (heap_) NAString(errNodeName, heap_);
    Int32 *check = hashDict_NODENUMBER_TO_NODENAME_->insert(key_nodeNumber,
                                                          val_nodeName);
  }
}

short OptimizerSimulator::simulate_NODENUMBER_TO_NODENAME_(Int32 nodeNumber,
                                                           char *nodeName,
                                                           short maxLen,
                                                           short *actualLen)
{
  char errNodeName[17];
  short error;

  if (hashDict_NODENUMBER_TO_NODENAME_->contains(&nodeNumber))
  {
    NAString *val_nodeName = hashDict_NODENUMBER_TO_NODENAME_->getFirstValue(&nodeNumber);
    strcpy(errNodeName, val_nodeName->data());
    error = (short)atoi(strtok(errNodeName, ":"));
    strcpy(nodeName, strtok(NULL, ":"));
    *actualLen = (short)strlen(nodeName);
    return error;
  }
  // Should NOT reach here.
  CMPASSERT(FALSE);
  return -1;
}

short OSIM_NODENUMBER_TO_NODENAME_(Int32 nodeNumber, char *nodeName, short maxLen, short *actualLen)
{
  short error = 0;
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(2))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
      error = NODENUMBER_TO_NODENAME_(nodeNumber, nodeName, maxLen, actualLen);
      if(CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
        CURRCONTEXT_OPTSIMULATOR->capture_NODENUMBER_TO_NODENAME_(error, nodeNumber,
                                                      nodeName, maxLen, actualLen);
              break;
    case OptimizerSimulator::SIMULATE:
      error = CURRCONTEXT_OPTSIMULATOR->simulate_NODENUMBER_TO_NODENAME_(nodeNumber, nodeName,
                                                              maxLen, actualLen);
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return error;
}

// END ************* System Call: NODENUMBER_TO_NODENAME_() *************

// BEGIN *********** System Call: FILENAME_TO_PROCESSHANDLE_() *************
//
void OptimizerSimulator::capture_FILENAME_TO_PROCESSHANDLE_(short error,
                                                            char *fileName,
                                                            short length,
                                                            short procHandle[])
{
  char errProcHandle[68];

  sprintf(errProcHandle, ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:",
                           error,
                           procHandle[0], procHandle[1], procHandle[2],
                           procHandle[3], procHandle[4], procHandle[5],
                           procHandle[6], procHandle[7], procHandle[8], procHandle[9]);

  NAString *key_fileName = new (heap_) NAString(fileName, heap_);
  NAString *val_procHandle = new (heap_) NAString(errProcHandle, heap_);

  if (hashDict_FILENAME_TO_PROCESSHANDLE_->contains(key_fileName))
  {
    NAString *chkValue = hashDict_FILENAME_TO_PROCESSHANDLE_->getFirstValue(key_fileName);
    if(chkValue->compareTo(val_procHandle->data()))
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    NAString *check = hashDict_FILENAME_TO_PROCESSHANDLE_->insert(key_fileName,
                                                                  val_procHandle);
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[FILENAME_TO_PROCESSHANDLE_];

    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(5);  (*outLogfile) << error << "  ";
    (*outLogfile).width(36); (*outLogfile) << fileName << "  ";
    // procHandle is an array of 10 short values.
    for(Int32 i=0; i<10; i++)
    {
      (*outLogfile).width(5); (*outLogfile) << procHandle[i] << " ";
    }
    (*outLogfile) << endl;
    (*outLogfile).width(origWidth);
  }
}

void OptimizerSimulator::readLogfile_FILENAME_TO_PROCESSHANDLE_()
{
  short error;
  char fileName[36];
  short procHandle[10];
  char errProcHandle[68];
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[FILENAME_TO_PROCESSHANDLE_],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[FILENAME_TO_PROCESSHANDLE_]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[FILENAME_TO_PROCESSHANDLE_]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read fileName and errProcHandle from the file
    inLogfile >> error >> fileName;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    for (Int32 i=0; i<10; i++)
      // read elements of processHandle array.
      inLogfile >> procHandle[i];
    sprintf(errProcHandle, ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:",
                             error,
                             procHandle[0], procHandle[1], procHandle[2],
                             procHandle[3], procHandle[4], procHandle[5],
                             procHandle[6], procHandle[7], procHandle[8], procHandle[9]);
    NAString *key_fileName = new (heap_) NAString(fileName, heap_);
    NAString *val_procHandle = new (heap_) NAString(errProcHandle, heap_);
    NAString *check = hashDict_FILENAME_TO_PROCESSHANDLE_->insert(key_fileName,
                                                                  val_procHandle);
  }
}

short OptimizerSimulator::simulate_FILENAME_TO_PROCESSHANDLE_(char *fileName,
                                                              short length,
                                                              short procHandle[])
{
  char errProcHandle[68];
  short error;

  NAString key_fileName(fileName, heap_);
  if (hashDict_FILENAME_TO_PROCESSHANDLE_->contains(&key_fileName))
  {
    NAString *val_procHandle = hashDict_FILENAME_TO_PROCESSHANDLE_->getFirstValue(&key_fileName);
    strcpy(errProcHandle, val_procHandle->data());
    error = (short)atoi(strtok(errProcHandle, ":"));
    for (Int32 i=0; i<10; i++)
      procHandle[i] = (short)atoi(strtok(NULL, ":"));
    return error;
  }
  // Should NOT reach here.
  CMPASSERT(FALSE);
  return -1;
}

short OSIM_FILENAME_TO_PROCESSHANDLE_(char *fileName, short length, short procHandle[])
{
  short error = FEOK;
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;
  char * localFileName;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(3))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
     if (strncmp(fileName, "\\NSK.NONSTOP_SQLMX", 18) == 0)
       localFileName = (char *) "$SYSTEM";
     else
        localFileName = fileName;
#ifdef SQ_NEW_PHANDLE
     SB_Phandle_Type *tempPhandle;
#else
     short *tempPhandle;
#endif // SQ_NEW_PHANDLE
     tempPhandle = get_phandle_with_retry ((char *)localFileName, &error);
     if (tempPhandle)
     {
#ifdef SQ_NEW_PHANDLE
        memmove((void *)procHandle, (void *)tempPhandle, sizeof(SB_Phandle_Type));
#else
        memmove(procHandle, tempPhandle, 20);
#endif // SQ_NEW_PHANDLE
     }
      if (CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
        CURRCONTEXT_OPTSIMULATOR->capture_FILENAME_TO_PROCESSHANDLE_(error, fileName, length, procHandle);
        break;
    case OptimizerSimulator::SIMULATE:
      error = CURRCONTEXT_OPTSIMULATOR->simulate_FILENAME_TO_PROCESSHANDLE_(fileName, length, procHandle);
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return error;
}
// END ************* System Call: FILENAME_TO_PROCESSHANDLE_() *************

// BEGIN *********** System Call: PROCESSHANDLE_DECOMPOSE_() *************
//
void OptimizerSimulator::capture_PROCESSHANDLE_DECOMPOSE_(short error,
                                                          short procHandle[],
                                                          short *cpu,
                                                          short *pin,
                                                          Int32 *nodeNumber)
{
  char cProcHandle[68];
  char errProcInfo[23];

  sprintf(cProcHandle, ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:",
                         procHandle[0], procHandle[1], procHandle[2],
                         procHandle[3], procHandle[4], procHandle[5],
                         procHandle[6], procHandle[7], procHandle[8], procHandle[9]);

  sprintf(errProcInfo, ":%d:%d:%d:%d:", error, *cpu, *pin, *nodeNumber);

  NAString *key_procHandle = new (heap_) NAString(cProcHandle, heap_);
  NAString *val_procInfo = new (heap_) NAString(errProcInfo, heap_);
  if (hashDict_PROCESSHANDLE_DECOMPOSE_->contains(key_procHandle))
  {
    NAString *chkValue = hashDict_PROCESSHANDLE_DECOMPOSE_->getFirstValue(key_procHandle);
    if(chkValue->compareTo(val_procInfo->data()))
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    NAString *check = hashDict_PROCESSHANDLE_DECOMPOSE_->insert(key_procHandle,
                                                                val_procInfo);
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[PROCESSHANDLE_DECOMPOSE_];

    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(5); (*outLogfile) << error << "  ";
    // procHandle is an array of 10 short values.
    for(Int32 i=0; i<10; i++)
    {
      (*outLogfile).width(5); (*outLogfile) << procHandle[i] << " ";
    }
    (*outLogfile).width(3);  (*outLogfile) << *cpu << "  ";
    (*outLogfile).width(7);  (*outLogfile) << *pin << "  ";
    (*outLogfile).width(10); (*outLogfile) << *nodeNumber << "  " << endl;
    (*outLogfile).width(origWidth);
  }
}

void OptimizerSimulator::readLogfile_PROCESSHANDLE_DECOMPOSE_()
{
  short error, cpu, pin;
  short procHandle[10];
  Int32 nodeNumber;
  char cProcHandle[68];
  char errProcInfo[23];
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[PROCESSHANDLE_DECOMPOSE_],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[PROCESSHANDLE_DECOMPOSE_]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[PROCESSHANDLE_DECOMPOSE_]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read error from the file.
    inLogfile >> error;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    for (Int32 i=0; i<10; i++)
      // read elements of processHandle array.
      inLogfile >> procHandle[i];
    // read cpu, pin and nodenumber from the file.
    inLogfile >> cpu >> pin >> nodeNumber;
    sprintf(cProcHandle, ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:",
                           procHandle[0], procHandle[1], procHandle[2],
                           procHandle[3], procHandle[4], procHandle[5],
                           procHandle[6], procHandle[7], procHandle[8], procHandle[9]);
    sprintf(errProcInfo, ":%d:%d:%d:%d:", error, cpu, pin, nodeNumber);
    NAString *key_procHandle = new (heap_) NAString(cProcHandle, heap_);
    NAString *val_procInfo = new (heap_) NAString(errProcInfo, heap_);
    NAString *check = hashDict_PROCESSHANDLE_DECOMPOSE_->insert(key_procHandle,
                                                                val_procInfo);
  }
}

short OptimizerSimulator::simulate_PROCESSHANDLE_DECOMPOSE_(short procHandle[],
                                                            short *cpu,
                                                            short *pin,
                                                            Int32 *nodeNumber)
{
  char cProcHandle[68];
  char errProcInfo[23];
  short error;

  sprintf(cProcHandle, ":%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:",
                         procHandle[0], procHandle[1], procHandle[2],
                         procHandle[3], procHandle[4], procHandle[5],
                         procHandle[6], procHandle[7], procHandle[8], procHandle[9]);

  NAString key_procHandle(cProcHandle, heap_);
  if(hashDict_PROCESSHANDLE_DECOMPOSE_->contains(&key_procHandle))
  {
    NAString *val_procInfo = hashDict_PROCESSHANDLE_DECOMPOSE_->getFirstValue(&key_procHandle);
    strcpy(errProcInfo, val_procInfo->data());
    error = (short)atoi(strtok(errProcInfo, ":"));
    *cpu = (short)atoi(strtok(NULL, ":"));
    *pin = (short)atoi(strtok(NULL, ":"));
    *nodeNumber = atoi(strtok(NULL, ":"));
    return error;
  }
  // Should NOT reach here.
  CMPASSERT(FALSE);
  return -1;
}

short OSIM_PROCESSHANDLE_DECOMPOSE_(short procHandle[], short *cpu, short *pin, Int32 *nodeNumber)
{
  short error = 0;

  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(4))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
      error = PROCESSHANDLE_DECOMPOSE_(procHandle, cpu, pin, nodeNumber);
      if (CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
        CURRCONTEXT_OPTSIMULATOR->capture_PROCESSHANDLE_DECOMPOSE_(error, procHandle, cpu, pin, nodeNumber);
      break;
    case OptimizerSimulator::SIMULATE:
      error = CURRCONTEXT_OPTSIMULATOR->simulate_PROCESSHANDLE_DECOMPOSE_(procHandle, cpu, pin, nodeNumber);
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return error;
}
// END ************* System Call: PROCESSHANDLE_DECOMPOSE_() *************

// BEGIN *********** System Call: REMOTEPROCESSORSTATUS() *************
//
void OptimizerSimulator::capture_REMOTEPROCESSORSTATUS(Lng32 status,
                                                       short clusterNum)
{

  short *key_clusterNum = new short(clusterNum);
  Lng32 *val_status = new Lng32(status);
  if (hashDict_REMOTEPROCESSORSTATUS->contains(key_clusterNum))
  {
    Lng32 *chkValue = hashDict_REMOTEPROCESSORSTATUS->getFirstValue(key_clusterNum);
    if (*chkValue != status)
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    short *check = hashDict_REMOTEPROCESSORSTATUS->insert(key_clusterNum,
                                                          val_status);
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[REMOTEPROCESSORSTATUS];

    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(5); (*outLogfile) << status << "  ";
    (*outLogfile).width(10); (*outLogfile) << clusterNum << endl;
    (*outLogfile).width(origWidth);
  }
}

/*
void OptimizerSimulator::log_REMOTEPROCESSORSTATUS()
{
  short *clusterNum;
  long *status;

  // Iterator for searching the clusterNum and corresponding cpuList
  // in hashDict_REMOTEPROCESSORSTATUS.
  NAHashDictionaryIterator<short, int > rpsIter (*hashDict_REMOTEPROCESSORSTATUS, NULL, NULL);

  // Open file in append mode.
  ofstream * outLogfile = writeSysCallStream[REMOTEPROCESSORSTATUS];

  for (unsigned int i=0; i<rpsIter.entries(); i++)
  {
    // Search for the clusterNum
    rpsIter.getNext(clusterNum, status);
    int origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(5); (*outLogfile) << *status << "  ";
    (*outLogfile).width(10); (*outLogfile) << *clusterNum << endl;
    (*outLogfile).width(origWidth);
  }
}
*/

void OptimizerSimulator::readLogfile_REMOTEPROCESSORSTATUS()
{
  Lng32 status;
  short clusterNum;
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[REMOTEPROCESSORSTATUS],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[REMOTEPROCESSORSTATUS]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[REMOTEPROCESSORSTATUS]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read status and clusterNum
    inLogfile >> status >> clusterNum;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    short *key_clusterNum = new short(clusterNum);
    Lng32 *val_status = new Lng32(status);
    short *check = hashDict_REMOTEPROCESSORSTATUS->insert(key_clusterNum,
                                                          val_status);
  }
}

Lng32 OptimizerSimulator::simulate_REMOTEPROCESSORSTATUS(short clusterNum)
{
  if(hashDict_REMOTEPROCESSORSTATUS->contains(&clusterNum))
  {
    Lng32 *val_status = hashDict_REMOTEPROCESSORSTATUS->getFirstValue(&clusterNum);
    return *(val_status);
  }
  // Should NOT reach here.
  CMPASSERT(FALSE);
  return -1;
}

Lng32 OSIM_REMOTEPROCESSORSTATUS(short clusterNum)
{
  Lng32 status = 0;
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(5))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
      status = REMOTEPROCESSORSTATUS(clusterNum);
      if(CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
        CURRCONTEXT_OPTSIMULATOR->capture_REMOTEPROCESSORSTATUS(status, clusterNum);
      break;
    case OptimizerSimulator::SIMULATE:
      status = CURRCONTEXT_OPTSIMULATOR->simulate_REMOTEPROCESSORSTATUS(clusterNum);
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return status;
}
// END ************* System Call: REMOTEPROCESSORSTATUS() *************

// BEGIN *********** System Call: FILENAME_DECOMPOSE_() *************
//
void OptimizerSimulator::capture_FILENAME_DECOMPOSE_(short error,
                                                     char *fileName,
                                                     short fileNameLen,
                                                     char *partName,
                                                     short maxLen,
                                                     short *partNameLen,
                                                     short level,
                                                     short options,
                                                     short subpart)
{
  // "fileName:level:options:subpart:". Three options are single digits.
  char tmpFileName[36];
  char cFileName[44];
  char cPartName[36];
  char errPartName[44];

  // Copy fileName to cFileName.
  strncpy(tmpFileName, fileName, fileNameLen);
  tmpFileName[fileNameLen] = '\0';
  // Concatenate level, options, subpart to filename before logging.
  snprintf(cFileName, sizeof(cFileName), ":%s:%d:%d:%d:", tmpFileName, level, options, subpart);

  strncpy(cPartName, partName, *partNameLen);
  cPartName[*partNameLen] = '\0';
  snprintf(errPartName, sizeof(errPartName), ":%d:%s:", error, cPartName);

  NAString *key_fileName = new (heap_) NAString(cFileName, heap_);
  NAString *val_partName = new (heap_) NAString(errPartName, heap_);
  if (hashDict_FILENAME_DECOMPOSE_->contains(key_fileName))
  {
    NAString *chkValue = hashDict_FILENAME_DECOMPOSE_->getFirstValue(key_fileName);
    if (chkValue->compareTo(val_partName->data()))
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    NAString *check = hashDict_FILENAME_DECOMPOSE_->insert(key_fileName,
                                                           val_partName);
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[FILENAME_DECOMPOSE_];

    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(5); (*outLogfile) << error << "  ";
    (*outLogfile).width(36); (*outLogfile) << tmpFileName << "  ";
    (*outLogfile).width(6);  (*outLogfile) << level << "  ";
    (*outLogfile).width(7);  (*outLogfile) << options << "  ";
    (*outLogfile).width(7);  (*outLogfile) << subpart << "  ";
    (*outLogfile).width(36); (*outLogfile) << cPartName << "  " << endl;
    (*outLogfile).width();
  }
}

void OptimizerSimulator::readLogfile_FILENAME_DECOMPOSE_()
{
  short error, level, options, subpart;
  char tmpFileName[36];
  char fileName[44];
  char partName[36];
  char errPartName[44];
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[FILENAME_DECOMPOSE_],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[FILENAME_DECOMPOSE_]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[FILENAME_DECOMPOSE_]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read error, fileName and partName the file
    inLogfile >> error >> tmpFileName >> level >> options >> subpart >> partName;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    // Concatenate level, options, subpart to filename before logging.
    snprintf(fileName, sizeof(fileName), ":%s:%d:%d:%d:", tmpFileName, level, options, subpart);
    snprintf(errPartName, sizeof(errPartName), ":%d:%s:", error, partName);
    NAString *key_fileName = new (heap_) NAString(fileName, heap_);
    NAString *val_partName = new (heap_) NAString(errPartName, heap_);
    NAString *check = hashDict_FILENAME_DECOMPOSE_->insert(key_fileName,
                                                           val_partName);
  }
}

// LCOV_EXCL_START :nsk
short OptimizerSimulator::simulate_FILENAME_DECOMPOSE_(char *fileName,
                                                       short fileNameLen,
                                                       char *partName,
                                                       short maxLen,
                                                       short *partNameLen,
                                                       short level,
                                                       short options,
                                                       short subpart)
{
  char tmpFileName[36];
  char cFileName[44];
  char errPartName[44];
  short error;

  // Copy fileName to cFileName.
  strncpy(tmpFileName, fileName, fileNameLen);
  tmpFileName[fileNameLen] = '\0';
  // Concatenate level, options, subpart to filename before logging.
  snprintf(cFileName, sizeof(cFileName), ":%s:%d:%d:%d:", tmpFileName, level, options, subpart);

  NAString key_fileName(cFileName, heap_);

  if (hashDict_FILENAME_DECOMPOSE_->contains(&key_fileName))
  {
    NAString *val_partName = hashDict_FILENAME_DECOMPOSE_->getFirstValue(&key_fileName);
    strcpy(errPartName, val_partName->data());
    error = (short)atoi(strtok(errPartName, ":"));
    strcpy(partName, strtok(NULL, ":"));
    *partNameLen = (short)strlen(partName);
    return error;
  }
  // Should NOT reach here.
  CMPASSERT(FALSE);
  return -1;
}
// LCOV_EXCL_STOP

short OSIM_FILENAME_DECOMPOSE_(char *fileName,
                               short fileNameLen,
                               char *partName,
                               short maxLen,
                               short *partNameLen,
                               short level,
                               short options,
                               short subpart)
{
  short error = 0;
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(6))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
    /*
    case OptimizerSimulator::SIMULATE:
      error = CURRCONTEXT_OPTSIMULATOR->simulate_FILENAME_DECOMPOSE_(fileName,
                                                         fileNameLen,
                                                         partName,
                                                         maxLen,
                                                         partNameLen,
                                                         level,
                                                         options,
                                                         subpart);
      break;
    */
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return error;
}
// END ************* System Call: FILENAME_DECOMPOSE_() *************
// BEGIN *********** System Call: MYSYSTEMNUMBER() *************
//
void OptimizerSimulator::capture_MYSYSTEMNUMBER(short sysNum)
{
  if (mySystemNumber_ == -1)
  {
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[MYSYSTEMNUMBER];

    // Write data at the end of the file.
    Int32 origWidth = (*outLogfile).width();
    (*outLogfile) << "  ";
    (*outLogfile).width(10); (*outLogfile) << sysNum << endl;
    (*outLogfile).width(origWidth);
    mySystemNumber_ = sysNum;
  }
}

void OptimizerSimulator::readLogfile_MYSYSTEMNUMBER()
{
  short sysNum;
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[MYSYSTEMNUMBER],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[MYSYSTEMNUMBER]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[MYSYSTEMNUMBER]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  if(inLogfile.good())
  {
    // read sysNum and errSysName from the file
    inLogfile >> sysNum;

    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
    {
      mySystemNumber_ = -1;
    }
    else{
      mySystemNumber_ = sysNum;
    }
  }
}

short OptimizerSimulator::simulate_MYSYSTEMNUMBER()
{
  return mySystemNumber_;
}

short OSIM_MYSYSTEMNUMBER()
{
  short sysNum = 0;
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(7))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
      sysNum = MYSYSTEMNUMBER();
      if(CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
        CURRCONTEXT_OPTSIMULATOR->capture_MYSYSTEMNUMBER(sysNum);
      break;
    case OptimizerSimulator::SIMULATE:
      sysNum = CURRCONTEXT_OPTSIMULATOR->simulate_MYSYSTEMNUMBER();
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return sysNum;
}
// END ************* System Call: MYSYSTEMNUMBER() *************

// BEGIN *********** System Call: GETSYSTEMNAME() *************
//
void OptimizerSimulator::capture_GETSYSTEMNAME(short error,
                                               short sysNum,
                                               short *sysName)
{
  char sysNameStr[9];
  char errSysName[17];

  strncpy(sysNameStr, (char *)sysName, 8);
  sysNameStr[8] = '\0';

  // Concatenate error and sysName.
  snprintf(errSysName, sizeof(errSysName), ":%d:%s:", error, sysNameStr);

  short *key_sysNum = new short(sysNum);
  NAString *val_sysName = new (heap_) NAString(errSysName, heap_);
  if (hashDict_GETSYSTEMNAME->contains(key_sysNum))
  {
    NAString *chkValue = hashDict_GETSYSTEMNAME->getFirstValue(key_sysNum);
    if(chkValue->compareTo(val_sysName->data()))
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    short *check = hashDict_GETSYSTEMNAME->insert(key_sysNum,
                                                val_sysName);
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[GETSYSTEMNAME];

    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(5); (*outLogfile) << error << "  ";
    (*outLogfile).width(10); (*outLogfile) << sysNum << "  ";
    (*outLogfile).width(10); (*outLogfile) << sysNameStr << endl;
    (*outLogfile).width(origWidth);
  }
}

void OptimizerSimulator::readLogfile_GETSYSTEMNAME()
{
  short error;
  short sysNum;
  char sysName[9];
  char errSysName[17];
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[GETSYSTEMNAME],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[GETSYSTEMNAME]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[GETSYSTEMNAME]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read sysNum and errSysName from the file
    inLogfile >> error >> sysNum >> sysName;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    // Concatenate error and sysName.
    snprintf(errSysName, sizeof(errSysName), ":%d:%-8s:", error, sysName);

    short *key_sysNum = new short(sysNum);
    NAString *val_sysName = new (heap_) NAString(errSysName, heap_);
    short *check = hashDict_GETSYSTEMNAME->insert(key_sysNum,
                                                  val_sysName);
  }
}

short OptimizerSimulator::simulate_GETSYSTEMNAME(short sysNum, short *sysName)
{
  char errSysName[17];
  short error;

  if (hashDict_GETSYSTEMNAME->contains(&sysNum))
  {
    NAString *val_sysName = hashDict_GETSYSTEMNAME->getFirstValue(&sysNum);
    strcpy(errSysName, val_sysName->data());
    error = (short)atoi(strtok(errSysName, ":"));
    strcpy((char *)sysName, strtok(NULL, ":"));
    return error;
  }
  // Should NOT reach here.
  CMPASSERT(FALSE);
  return -1;
}

short OSIM_GETSYSTEMNAME(short sysNum, short *sysName)
{
  short error = 0;
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(8))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
      error = GETSYSTEMNAME(sysNum, sysName);
      if(CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
        CURRCONTEXT_OPTSIMULATOR->capture_GETSYSTEMNAME(error, sysNum, sysName);
      break;
    case OptimizerSimulator::SIMULATE:
      error = CURRCONTEXT_OPTSIMULATOR->simulate_GETSYSTEMNAME(sysNum, sysName);
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return error;
}
// END ************* System Call: GETSYSTEMNAME() *************

// BEGIN *********** System Call: getEstimatedRows() ****************
//
void OptimizerSimulator::capture_getEstimatedRows(const char *tableName,
                                                  double estRows)
{
  NAString *key_tableName = new (heap_) NAString(tableName, heap_);
  double *val_estRows = new double(estRows);

  if (hashDict_getEstimatedRows->contains(key_tableName))
  {
    double *chkValue = hashDict_getEstimatedRows->getFirstValue(key_tableName);
    if (*chkValue != estRows)
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    NAString *check = hashDict_getEstimatedRows->insert(key_tableName,
                                                        val_estRows);
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[getEstimatedRows];
    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(36); (*outLogfile) << tableName << "  ";
    (*outLogfile).width(36); (*outLogfile) << estRows << endl;
    (*outLogfile).width(origWidth);
  }
}

void OptimizerSimulator::readLogfile_getEstimatedRows()
{
  char tableName[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1];
  double estRows;
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[getEstimatedRows],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[getEstimatedRows]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[getEstimatedRows]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read tableName and estRows from the file
    inLogfile >> tableName >> estRows;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    NAString *key_tableName = new (heap_) NAString(tableName, heap_);
    double *val_estRows = new double(estRows);
    NAString *check = hashDict_getEstimatedRows->insert(key_tableName,
                                                        val_estRows);
  }
}

double OptimizerSimulator::simulate_getEstimatedRows(const char *tableName)
{
  NAString key_tableName(tableName, heap_);
  if (hashDict_getEstimatedRows->contains(&key_tableName))
  {
    double *val_estRows = hashDict_getEstimatedRows->getFirstValue(&key_tableName);
    return *(val_estRows);
  }
  // Should NOT reach here.
  char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
  snprintf(errMsg, sizeof(errMsg), "Estimated Rows information is not available for the following table: %s",
                       tableName);

  warningMessage(errMsg);
  return -1;
}
// END ************* System Call: getEstimatedRows() *************

// BEGIN *********** System Call: getJustInTimeStatsRowcount() ****************
//
void OptimizerSimulator::capture_getJustInTimeStatsRowcount(const char *origPredicate,
							    double rowCount,
							    float sampleFraction,
							    double rowsInTable)
{
  char rowCountSampleFractionStr[60];
  sprintf(rowCountSampleFractionStr, ":%f:%f:%f:", rowCount, sampleFraction, rowsInTable);

  NAString *key_origPredicate = new (heap_) NAString(origPredicate, heap_);
  NAString *val_Str = new (heap_) NAString(rowCountSampleFractionStr, heap_);

  NAString *check = hashDict_getJustInTimeStatsRowcount->insert(key_origPredicate, val_Str);

  // Open file in append mode.
  ofstream * outLogfile = writeSysCallStream[getJustInTimeStatsRowcount];

  Int32 origWidth = (*outLogfile).width();

  // Write data at the end of the file.

  (*outLogfile).width(36); (*outLogfile) << origPredicate << ";   ";
  (*outLogfile).width(36); (*outLogfile) << rowCount;
  (*outLogfile).width(36); (*outLogfile) << sampleFraction;
  (*outLogfile).width(36); (*outLogfile) << rowsInTable << endl;
  (*outLogfile).width(origWidth);
}

void OptimizerSimulator::readLogfile_getJustInTimeStatsRowcount()
{
  char origPredicate[32000];
  double rowCount;
  float sampleFraction = 0;
  double rowsInTable;
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[getJustInTimeStatsRowcount],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[getJustInTimeStatsRowcount]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[getJustInTimeStatsRowcount]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  while(inLogfile.good())
  {
    // read queryText and rowCount from the file
    inLogfile.getline(origPredicate, 30000, ';');
    inLogfile >> rowCount;
    inLogfile >> sampleFraction;
    inLogfile >> rowsInTable;
    inLogfile.ignore(OSIM_LINEMAX, '\n');

    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;

    char rowCountSampleFractionStr[60];
    sprintf(rowCountSampleFractionStr, ":%f:%f:%f:", rowCount, sampleFraction, rowsInTable);

    NAString *key_origPredicate = new (heap_) NAString(origPredicate, heap_);
    *key_origPredicate = key_origPredicate->strip(NAString::both);

    NAString *val_Str = new (heap_) NAString(rowCountSampleFractionStr, heap_);

    NAString *check = hashDict_getJustInTimeStatsRowcount->insert(key_origPredicate,
                                                        val_Str);
  }
}

void OptimizerSimulator::simulate_getJustInTimeStatsRowcount(const char *origPredicate, 
							     double *rowCount, 
							     float *sampleFraction,
							     double *rowsInTable)
{
  char rowCountSampleFractionStr[60];
  NAString key_origPredicate(origPredicate, heap_);

  if (hashDict_getJustInTimeStatsRowcount->contains(&key_origPredicate))
  {
    NAString *val_Str = hashDict_getJustInTimeStatsRowcount->getFirstValue(&key_origPredicate);
    strcpy(rowCountSampleFractionStr, val_Str->data());
    *rowCount = (double)atof(strtok(rowCountSampleFractionStr, ":"));
    *sampleFraction = (float)atof(strtok(NULL, ":"));
    *rowsInTable = (double)atof(strtok(NULL, ":"));
    return;
  }

  // Should NOT reach here.
  //Setting rowCount to -1 to indicate that CTS information is missing for the query.
  *rowCount = -1;

  char errMsg[30000]; // Error msg below + Query Text + '\0'
  snprintf(errMsg, sizeof(errMsg), "CompileTimeStats information is not available for the following predicate: %s",
                       origPredicate);

  warningMessage(errMsg);
}
// END ************* System Call: getJustInTimeStatsRowcount() *************

void OptimizerSimulator::capture_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum)
{
    if (capturedNodeAndClusterNum_)
      return;

    nodeNum_ = nodeNum;
    clusterNum_ = clusterNum;

    capturedNodeAndClusterNum_ = TRUE;
}

void OptimizerSimulator::log_getNodeAndClusterNumbers()
{
    // Open file in append mode.
    ofstream * outLogfile = writeSysCallStream[getNodeAndClusterNumbers];
    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(8); (*outLogfile) << nodeNum_ << "  ";
    (*outLogfile).width(12); (*outLogfile) << clusterNum_ << endl;
    (*outLogfile).width(origWidth);
}

void OptimizerSimulator::readLogFile_getNodeAndClusterNumbers()
{
  short nodeNum;
  Int32 clusterNum;
  NABoolean isDir;

  if(!fileExists(sysCallLogfile[getNodeAndClusterNumbers],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error msg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogfile[getNodeAndClusterNumbers]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogfile[getNodeAndClusterNumbers]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  if(inLogfile.good())
  {
    // read nodeNum and clusterNum from the file
    inLogfile >> nodeNum >> clusterNum;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit if there was no data to read above.
    if(!inLogfile.good())
    {
      nodeNum_ = -1;
      clusterNum_ = -1;
    }
    else{
      nodeNum_ = nodeNum;
      clusterNum_ = clusterNum;
    }
  }

}

void OptimizerSimulator::simulate_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum)
{
  nodeNum = nodeNum_;
  clusterNum = clusterNum_;
}

void  OSIM_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum){
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(!CURRCONTEXT_OPTSIMULATOR->isCallDisabled(10))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)

  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
      NADefaults::getNodeAndClusterNumbers(nodeNum, clusterNum);
      CURRCONTEXT_OPTSIMULATOR->capture_getNodeAndClusterNumbers(nodeNum, clusterNum);
      break;
    case OptimizerSimulator::SIMULATE:
      CURRCONTEXT_OPTSIMULATOR->simulate_getNodeAndClusterNumbers(nodeNum, clusterNum);
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
}

void OptimizerSimulator::capture_CQDs()
{
  char *buf = new(CmpCommon::statementHeap()) char[32768];

  // Open file in append mode.
  ofstream * defaultDefaultsLogfile =
    writeSysCallStream[DEFAULT_DEFAULTSFILE];
  ofstream * cqdDefaultsLogfile =
    writeSysCallStream[CQD_DEFAULTSFILE];
  ofstream * defTableDefaultsLogfile =
    writeSysCallStream[DEF_TABLE_DEFAULTSFILE];
  ofstream * computedDefaultsLogfile =
    writeSysCallStream[COMPUTED_DEFAULTSFILE];
  ofstream * uninitDefaultsLogfile =
    writeSysCallStream[UNINITIALIZED_DEFAULTSFILE];
  ofstream * immutableDefaultsLogfile =
    writeSysCallStream[IMMUTABLE_DEFAULTSFILE];
  ofstream * derivedDefaultsLogfile =
    writeSysCallStream[DERIVED_DEFAULTSFILE];
  ofstream * platformDefaultsLogfile =
    writeSysCallStream[PLATFORM_DEFAULTSFILE];

  // send all externalized CQDs.
  NADefaults &defs = CmpCommon::context()->schemaDB_->getDefaults();

  for (UInt32 i = 0; i < defs.numDefaultAttributes(); i++)
  {
    const char *attrName = defs.lookupAttrName (i);
    const char *val=defs.getValue(i);
    snprintf(buf, sizeof(buf), "CONTROL QUERY DEFAULT %s '%s';", attrName, val);

    DefaultConstants attrEnum = NADefaults::lookupAttrName(attrName);
    switch(defs.getProvenance(attrEnum))
    {
      case NADefaults::UNINITIALIZED:
        (*uninitDefaultsLogfile) << buf << endl;
        break;
      case NADefaults::INIT_DEFAULT_DEFAULTS:
        (*defaultDefaultsLogfile) << buf << endl;
        break;
      case NADefaults::DERIVED:
        (*derivedDefaultsLogfile) << buf << endl;
        break;
      case NADefaults::READ_FROM_SQL_TABLE:
        (*defTableDefaultsLogfile) << buf << endl;
        break;
      case NADefaults::COMPUTED:
        (*computedDefaultsLogfile) << buf << endl;
        break;
      case NADefaults::SET_BY_CQD:
        (*cqdDefaultsLogfile) << buf << endl;
        break;
      case NADefaults::IMMUTABLE:
        (*immutableDefaultsLogfile) << buf << endl;
        break;
      default:
        break;
    }

    // Need to capture platform-specific CQDs, they need to be set 
    // correctly at simulation time on windows side
    switch(i)
    {
    case CAT_LARGE_BLOCKS_LARGE_KEYS:
    case CAT_LARGE_BLOCKS_MAX_KEYSIZE:
    case CAT_LARGE_BLOCKS_MAX_ROWSIZE:
    case COMP_BOOL_202:
    case COMP_INT_67:
    case CPUCOST_COMPARE_SIMPLE_DATA_TYPE:
    case DISTRIBUTE_OPENS:
    case ESP_ON_AGGREGATION_NODES_ONLY:
    case POS_DEFAULT_LARGEST_DISK_SIZE_GB:
    case EXE_HGB_INITIAL_HT_SIZE:
    case EXE_MEMORY_AVAILABLE_IN_MB:
    case EXE_SINGLE_BMO_QUOTA:
    case FLOATTYPE:
    case GEN_HGBY_BUFFER_SIZE:
    case GEN_HSHJ_BUFFER_SIZE:
    case GEN_MEM_PRESSURE_THRESHOLD:
    case INPUT_CHARSET:
    case OLAP_BUFFER_SIZE:
    case POS:
    case SCRATCH_IO_BLOCKSIZE_SORT:
    case SCRATCH_MAX_OPENS_HASH:
    case SIMPLE_COST_MODEL:
    case SORT_MERGE_BUFFER_UNIT_56KB:
    case TARGET_CODE:
    case TERMINAL_CHARSET:
      (*platformDefaultsLogfile) << buf << endl;
      break;
    default:
        break;
    }
  }
  NADELETEBASIC(buf, CmpCommon::statementHeap());
}

void OSIM_captureTableOrView(NATable * naTab)
{
  if(CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
    CURRCONTEXT_OPTSIMULATOR->capture_TableOrView(naTab);
}

void OptimizerSimulator::capture_TableOrView(NATable * naTab)
{
  const char * viewText = naTab->getViewText();
  const QualifiedName objQualifiedName = naTab->getTableName();
  NAString objName = objQualifiedName.getQualifiedNameAsAnsiString();

  // The following is to handle '/' character that can appear in the delimited
  // object names while creating file names for view, synonyms or tables.
  size_t i = -1;
  NAString fileName = objName;
  i = objName.first('"');

  if(i != -1)
  {
    for(i = fileName.first('"'); i < fileName.length(); i++)
    {
      if(fileName[i] == '/' || fileName[i] == '"')
      {
        fileName.remove(i, 1);
        i--;
      }
    }
  }

  short pathLen = 0;
  // Handle Synonym first
  if(naTab->getIsSynonymTranslationDone())
  {
    NAString synRefName = naTab->getSynonymReferenceName();

    if(!hashDict_Synonyms->contains(&objName))
    {      
      // Open file in append mode.
      ofstream * synonymListFile = writeSysCallStream[SYNONYMSFILE];
      (*synonymListFile) << objName.data() <<endl;

      pathLen = strlen(osimLogdir_)+1+strlen(fileName.data())+1;
      char * synonymFileName = new (CmpCommon::statementHeap()) char[pathLen];
      // Construct an absolute pathname for the file.
      strcpy(synonymFileName, osimLogdir_);
      strcat(synonymFileName, "/");
      strcat(synonymFileName, fileName.data());

      // open file to write out the view text
      ofstream synonymLogfile(synonymFileName);
      synonymLogfile << "create catalog " << objQualifiedName.getCatalogName().data()
                  << ";" << endl;
      synonymLogfile << "create schema " << objQualifiedName.getCatalogName().data()
                  << "." << objQualifiedName.getSchemaName().data() << ";"
                  << endl;
      synonymLogfile << "create synonym " << objName << " for " << synRefName << ";" << endl;

      // insert viewName into hash table
      // this is used to check if the view has already
      // been written out to disk
      NAString * synonymName = new (heap_) NAString(objName, heap_);
      Int32 * dummy = new Int32(0);
      hashDict_Synonyms->insert(synonymName, dummy);
    }

    objName = synRefName;

    i = -1;
    fileName = objName;
    i = objName.first('"');
    if(i != -1)
    {
      for(i = fileName.first('"'); i < fileName.length(); i++)
      {
	if(fileName[i] == '/' || fileName[i] == '"')
	{
	  fileName.remove(i, 1);
	  i--;
	}
      }
    }
  }

  if (viewText)
  {
    //handle views

    // * write out viewName to VIEWS.txt file

    // * if viewText not already written out then write out viewText
    if(!hashDict_Views->contains(&objName))
    {
      // Open file in append mode.
      ofstream * viewsListFile = writeSysCallStream[VIEWSFILE];
      (*viewsListFile) << objName.data() <<endl;

      // create view file name

      // Allocate memory for file pathname:
      // dirname + '/' + <view name> + '\0'
      pathLen = strlen(osimLogdir_)+1+strlen(fileName.data())+1;
      char * viewFileName = new (CmpCommon::statementHeap()) char[pathLen];
      // Construct an absolute pathname for the file.
      strcpy(viewFileName, osimLogdir_);
      strcat(viewFileName, "/");
      strcat(viewFileName, fileName.data());

      // open file to write out the view text
      ofstream viewLogfile(viewFileName);
      viewLogfile << "create catalog " << objQualifiedName.getCatalogName().data()
                  << ";" << endl;
      viewLogfile << "create schema " << objQualifiedName.getCatalogName().data()
                  << "." << objQualifiedName.getSchemaName().data() << ";"
                  << endl;

      // Need to set CQDs to enable special syntax if running in special modes
      if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON)
      {
	viewLogfile << "control query default DISABLE_READ_ONLY 'ON';" << endl;
	viewLogfile << "control query default MODE_SPECIAL_1 'ON';" << endl;
      }

      if (CmpCommon::getDefault(MODE_SPECIAL_2) == DF_ON)
      {
	viewLogfile << "control query default DISABLE_READ_ONLY 'ON';" << endl;
	viewLogfile << "control query default MODE_SPECIAL_2 'ON';" << endl;
      }

      viewLogfile << viewText <<endl;

      // insert viewName into hash table
      // this is used to check if the view has already
      // been written out to disk
      NAString * viewName = new (heap_) NAString(objName, heap_);
      Int32 * dummy = new Int32(0);
      hashDict_Views->insert(viewName, dummy);
    }
  }
  else if (naTab->getSpecialType() == ExtendedQualName::NORMAL_TABLE)
  {
    // handle base tables

    // if table not already captured then:
    // * write out table name to tables.txt file
    // * call mxexportddl to capture tables metadata
    if(!hashDict_Tables->contains(&objName))
    {
      // * write out tableName to TABLES.txt file

      // Open file in append mode.
      ofstream * tablesListFile = writeSysCallStream[TABLESFILE];
      (*tablesListFile) << objName.data() <<endl;

      // insert tableName into hash table
      // this is used to check if the table has already
      // been written out to disk
      NAString * tableName = new (heap_) NAString(objName, heap_);
      Int32 * dummy = new Int32(0);
      hashDict_Tables->insert(tableName, dummy);
    }
  }
}

void OptimizerSimulator::captureQueryText(const char * query)
{
  // Open file in append mode.
  ofstream * outLogfile = writeSysCallStream[QUERIESFILE];

  (*outLogfile) << "--BeginQuery" << endl;
  (*outLogfile) << query ;
  Int32 queryStrLen = strlen(query);
  // put in a semi-colon at end of query if it is missing
  if (query[queryStrLen]!= ';')
    (*outLogfile) << ";";
  (*outLogfile) << endl;
  (*outLogfile) << "--EndQuery" << endl;
}

void OSIM_captureQueryText(const char * query)
{
  CURRCONTEXT_OPTSIMULATOR->captureQueryText(query);
}

void OptimizerSimulator::captureQueryShape(const char * shape)
{
  // Open file in append mode.
  ofstream * outLogfile = writeSysCallStream[QUERIESFILE];

  (*outLogfile) << "--QueryShape: " << shape << ";" << endl;
}

void OSIM_captureQueryShape(const char * shape)
{
  CURRCONTEXT_OPTSIMULATOR->captureQueryShape(shape);
}

#define DefToStr2(a) #a
#define DefToStr(a) "" DefToStr2(a) ""

void OptimizerSimulator::captureVPROC()
{

  NAString vprocNAString;

  vprocNAString = DefToStr(PRODNUMCMP);
  vprocNAString += "_";
  vprocNAString += DefToStr(DATE1CMP);
  vprocNAString += "_";
  vprocNAString += DefToStr(CMP_CC_LABEL);

  CMPASSERT(!vprocNAString.isNull());     // not empty

#pragma nowarn(1506)   // warning elimination
  Int32 length = strlen(vprocNAString) + 1;
#pragma warn(1506)  // warning elimination

  char * vproc = new (CmpCommon::contextHeap()) char[length];
  strncpy(vproc, vprocNAString, length);


  // get a handle to the file stream
  ofstream * outLogfile = writeSysCallStream[VPROCFILE];

  // write out the vproc
  (*outLogfile) << vproc << endl;
}

// LCOV_EXCL_START :nsk
// LCOV_EXCL_STOP

void OptimizerSimulator::capturePrologue()
{
  if (osimMode_ == OptimizerSimulator::CAPTURE)
  {
    if (!capturedInitialData_)
    {
      capture_CQDs();
      gpClusterInfo->initializeForOSIMCapture();
      gpClusterInfo->captureNAClusterInfo();
      captureVPROC();

      // Write the system type to a file.
      captureSysType();

      //log_REMOTEPROCESSORSTATUS();
      log_getNodeAndClusterNumbers();
      ControlDB * cdb = ActiveControlDB();

      if (cdb->getRequiredShape())
      {
        const char * requiredShape =
          cdb->getRequiredShape()->getShapeText().data();
        captureQueryText(requiredShape);
      }
      capturedInitialData_ = TRUE;
    }

    const char * queryText = CmpCommon::context()->statement()->userSqlText();
    captureQueryText(queryText);
  }
}

void  OSIM_capturePrologue()
{
  CURRCONTEXT_OPTSIMULATOR->capturePrologue();
}

void OptimizerSimulator::cleanup()
{
  mySystemNumber_ = -1;
  capturedInitialData_ = FALSE;
  usingCaptureHint_ = FALSE;
  if (osimLogdir_)
  {
    NADELETEBASIC(osimLogdir_,heap_);osimLogdir_ = NULL;
  }
  osimMode_ = OptimizerSimulator::OFF;

  // delete file names
  for (sysCall sc=FIRST_SYSCALL; sc<NUM_OF_SYSCALLS; sc = sysCall(sc+1))
  {
    if (sysCallLogfile[sc])
    {
      NADELETEBASIC(sysCallLogfile[sc],heap_); sysCallLogfile[sc]=NULL;
    }
    if(writeSysCallStream[sc])
    {
      writeSysCallStream[sc]->close();
      NADELETE(writeSysCallStream[sc],ofstream,heap_); writeSysCallStream[sc]=NULL;
    }
  }

  // clear out hash dictionaries
  hashDict_NODENAME_TO_NODENUMBER_->clear(TRUE);
  hashDict_NODENUMBER_TO_NODENAME_->clear(TRUE);
  hashDict_FILENAME_TO_PROCESSHANDLE_->clear(TRUE);
  hashDict_PROCESSHANDLE_DECOMPOSE_->clear(TRUE);
  hashDict_REMOTEPROCESSORSTATUS->clear(TRUE);
  hashDict_FILENAME_DECOMPOSE_->clear(TRUE);
  hashDict_GETSYSTEMNAME->clear(TRUE);
  hashDict_getEstimatedRows->clear(TRUE);
  hashDict_getJustInTimeStatsRowcount->clear(TRUE);
  hashDict_Views->clear(TRUE);
  hashDict_Tables->clear(TRUE);
  hashDict_Synonyms->clear(TRUE);
}

void OptimizerSimulator::cleanupSimulator()
{
  cleanup();
  //clear out QueryCache
  CURRENTQCACHE->makeEmpty();
  //clear out NATableCache
  CmpCommon::context()->schemaDB_->getNATableDB()->setCachingOFF();
  CmpCommon::context()->schemaDB_->getNATableDB()->setCachingON();
  //clear out HistogramCache
  if(CURRCONTEXT_HISTCACHE)
    CURRCONTEXT_HISTCACHE->invalidateCache();
  nodeNum_ = -1;
  clusterNum_ = -1;
  capturedNodeAndClusterNum_ = FALSE;
}

void OptimizerSimulator::cleanupAfterStatement()
{
  if (usingCaptureHint_)
    cleanup();
};

NABoolean OptimizerSimulator::isCallDisabled(ULng32 callBitPosition)
{
  if(callBitPosition > 32)
    return FALSE;

  ULng32 bitMask = SingleBitArray[callBitPosition];

  if(bitMask & sysCallsDisabled_)
    return TRUE;

  return FALSE;
}

void OptimizerSimulator::captureSysType()
{
  const char *sysType = "LINUX";

  ofstream* outLogfile= writeSysCallStream[CAPTURE_SYS_TYPE];
  (*outLogfile) << sysType << endl;
}

OptimizerSimulator::sysType OptimizerSimulator::getCaptureSysType()
{
  return captureSysType_;
}

void OptimizerSimulator::readLogFile_captureSysType()
{
  // This is not an error.  If the file doesn't exist, assume that
  // the captured system type is NSK.
  NABoolean isDir;
  if(!fileExists(sysCallLogfile[CAPTURE_SYS_TYPE],isDir))
  {
    captureSysType_ = OSIM_NSK;
    return;
  }

  ifstream inLogfile(sysCallLogfile[CAPTURE_SYS_TYPE]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

  char captureSysTypeString[64];
  inLogfile >> captureSysTypeString;

  if (strncmp(captureSysTypeString,"LINUX",5) == 0)
    captureSysType_ = OSIM_LINUX;
  else if (strncmp(captureSysTypeString,"NSK",3) == 0)
    captureSysType_ = OSIM_NSK;
  else if (strncmp(captureSysTypeString,"WINNT",5) == 0)
    captureSysType_ = OSIM_WINNT;
  else
    CMPASSERT(0); // Something is wrong with the log file.
}

inline NABoolean OptimizerSimulator::runningSimulation()
{
  if(CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::SIMULATE)
    return TRUE;
  return FALSE;
}

NABoolean OSIM_isNTbehavior()
{
  if (CURRCONTEXT_OPTSIMULATOR != NULL &&
      CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::SIMULATE &&
      CURRCONTEXT_OPTSIMULATOR->getCaptureSysType() == OptimizerSimulator::OSIM_WINNT)
    return TRUE;
  else
    return FALSE;
}

NABoolean OSIM_isNSKbehavior()
{
  if (CURRCONTEXT_OPTSIMULATOR != NULL &&
      CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::SIMULATE &&
      CURRCONTEXT_OPTSIMULATOR->getCaptureSysType() == OptimizerSimulator::OSIM_NSK)
    return TRUE;
  else
    return FALSE;
}

NABoolean OSIM_isLinuxbehavior()
{
  if (CURRCONTEXT_OPTSIMULATOR != NULL &&
      CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::SIMULATE &&
      CURRCONTEXT_OPTSIMULATOR->getCaptureSysType() == OptimizerSimulator::OSIM_LINUX)
    return TRUE;
  else
    return FALSE;
}

NABoolean OptimizerSimulator::runningInCaptureMode()
{
  if(this->getOsimMode() == OptimizerSimulator::CAPTURE)
    return TRUE;
  return FALSE;
}

NABoolean OSIM_runningSimulation()
{
  if (CURRCONTEXT_OPTSIMULATOR)
    return CURRCONTEXT_OPTSIMULATOR->runningSimulation();
  else
    return FALSE;
}

NABoolean OSIM_runningInCaptureMode()
{
  if (CURRCONTEXT_OPTSIMULATOR)
    return CURRCONTEXT_OPTSIMULATOR->runningInCaptureMode();
  else
    return FALSE;
}

NABoolean OSIM_catIsDisabled()
{
  if (CURRCONTEXT_OPTSIMULATOR)
    return CURRCONTEXT_OPTSIMULATOR->isCallDisabled(11);

  return TRUE;
}

NABoolean OSIM_ustatIsDisabled()
{
  if (CURRCONTEXT_OPTSIMULATOR)
    return CURRCONTEXT_OPTSIMULATOR->isCallDisabled(12);

  return TRUE;
}

//Stubs for NT
Lng32 REMOTEPROCESSORSTATUS(short clusterNum)
{
  CMPASSERT(FALSE);
  return -1;
}
