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
 * File:         ComRtUtils.h
 * Description:  Some common OS functions that are called by the
 *               executor (run-time) and may also b called by other components
 *
 * Created:      7/4/97
 * Language:     C++
 *
 *
 *****************************************************************************
 */
#ifndef COMRTUTILS_H
#define COMRTUTILS_H

#include "NABoolean.h"
#include "NAString.h"
#include "Platform.h"
#include "NAMemory.h"
#include "Int64.h"

#include <fstream>
using namespace std;
#include "seabed/ms.h"
#ifdef NA_64BIT
// dg64 - the following includes defines min
#ifdef min
#undef min
#endif // min
#endif // NA_64BIT

#define MAX_SEGMENT_NAME_LEN  255
#define MAX_NO_OF_SEGMENTS    256
#define PROCESSNAME_STRING_LEN    40
#define PROGRAM_NAME_LEN    64
#define BDR_CLUSTER_NAME_LEN 24
#define BDR_CLUSTER_NAME_KEY "BDR_CLUSTER"


// Keep in sync with common/ComRtUtils.cpp ComRtGetModuleFileName():
//                               0123456789*123456789*123456789*1: position0-31
#define systemModulePrefix	    "HP_SYSTEM_CATALOG.SYSTEM_SCHEMA."
#define systemModulePrefixLen	32
#define systemModulePrefixODBC	"HP_SYSTEM_CATALOG.MXCS_SCHEMA."
#define systemModulePrefixLenODBC	30

//const NAString InternalModNameList;

// returns TRUE, if modName is an internal module name
NABoolean ComRtIsInternalModName(const char * modName);

// returns 'next' internal mod name.
// 'index' keeps track of the current mod name returned. It should
// be initialized to 0 on the first call to this method.
const char * ComRtGetNextInternalModName(Lng32 &index, char * modNameBuf);

// -----------------------------------------------------------------------
// Class to read an oss file by oss path name or Guardian File name
// -----------------------------------------------------------------------

#ifdef NA_STD_NAMESPACE
#include <iosfwd>
using namespace std;
#endif

class ModuleOSFile
{
public:

  ModuleOSFile();
  ~ModuleOSFile();
  Int32 open(const char *fname);
  Int32 openGuardianFile (const char *fname);
  Int32 close();
  Int32 readpos(char *buf, Lng32 pos, Lng32 len, short &countRead);

private:

  fstream fs_;
};

// -----------------------------------------------------------------------
// All of the methods below require a buffer that holds the output
// string.  The true length of the output (string length w/o NUL
// terminator) is returned in resultLength (may be greater than
// inputBufferLength). The output string in buffer is NUL-
// terminated. The return code is either 0 or an operating system
// error or -1 if the buffer wasn't large enough. The "resultLength"
// output parameter is set if the return code is 0 or -1.
// The diagnostics area is not set by these calls.
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Get the directory name where NonStop SQL software resides
// (from registry on NT, $SYSTEM.SYSTEM on NSK)
// -----------------------------------------------------------------------
Lng32 ComRtGetInstallDir(
     char *buffer,
     Lng32 inputBufferLength,
     Lng32 *resultLength);

// -----------------------------------------------------------------------
// Convert a 3-part module name into the file name in which the module
// is stored
// -----------------------------------------------------------------------
Lng32 ComRtGetModuleFileName(
     const char *moduleName,
     const char *moduleDir,
     char *buffer,
     Lng32 inputBufferLength,
     char * sysModuleDir,  // location of SYSTEMMODULES
     char * userModuleDir, // location of USERMODULES
     Lng32 *resultLength,
     short &isSystemModule);

// -----------------------------------------------------------------------
// Get the cluster (EXPAND node) name (returns "NSK" on NT)
// -----------------------------------------------------------------------
Lng32 ComRtGetOSClusterName(
     char *buffer,
     Lng32 inputBufferLength,
     Lng32 *resultLength,
     short * nodeNumber = NULL);

// -----------------------------------------------------------------------
// Get the MP system catalog name.
// -----------------------------------------------------------------------
Lng32 ComRtGetMPSysCatName(
     char *sysCatBuffer,    /* out */
     Lng32 inputBufferLength,/* in  */
     char *inputSysName,    /* in must set to NULL if no name is passed */
     Lng32 *sysCatLength,    /* out */
     short *detailError,    /* out */
     NAMemory *heap = 0 );  /* in  */

// -----------------------------------------------------------------------
// Determine if the name is an NSK name, \sys.$vol.subvol.file, look for
// \ . $ characters in the string.
// -----------------------------------------------------------------------
NABoolean ComRtIsNSKName(char *name);

Int64 ComRtGetJulianFromUTC(timespec ts);
// -----------------------------------------------------------------------
//
// ComRtGetProgramInfo()
//
// Outputs:
// 1) the pathname of the directory where the application program
//    is being run from.
//    For OSS processes, this will be the fully qualified oss directory
//      pathname.
//    For Guardian processes, pathname is not set
// 2) the process type (oss or guardian).
// 3) Other output values are: cpu, pin, nodename, nodename Len, processCreateTime
//       and processNameString in the format <\node_name>.<cpu>,<pin>
//
// // Return status:      0, if all ok. <errnum>, in case of an error.
//
// -----------------------------------------------------------------------
Lng32 ComRtGetProgramInfo(char * pathName,    /* out */
			 Lng32 pathNameMaxLen, /* in */
			 short  &processType, /* out */
			 Int32  &cpu, /* cpu */
			 pid_t  &pin, /* pin */
			 Lng32   &nodeNumber,
			 char * nodeName, // GuaNodeNameMaxLen+1
			 short  &nodeNameLen,
			 Int64  &processCreateTime,
			  char *processNameString,
			  char *parentProcessNameString = NULL
#ifdef SQ_PHANDLE_VERIFIER
                         , SB_Verif_Type *verifier = NULL
#endif
			 );

// OUT: processPriority: current priority of process
Lng32 ComRtGetProcessPriority(Lng32  &processPriority /* out */);

Lng32 ComRtSetProcessPriority(Lng32 priority,
			     NABoolean isDelta);

// OUT: pagesInUse: Pages(16k) currently in use by process
Lng32 ComRtGetProcessPagesInUse(Int64  &pagesInUse /* out */);

// IN:  if cpu, pin and nodeName are passed in, is that to find process.
//      Otherwise, use current process
// OUT: processCreateTime: time when this process was created.
Lng32 ComRtGetProcessCreateTime(short  *cpu, /* cpu */
			       pid_t  *pin, /* pin */
			       short  *nodeNumber,
			       Int64  &processCreateTime,
			       short  &errorDetail
			       );


Lng32 ComRtGetIsoMappingEnum();
char * ComRtGetIsoMappingName();

// -----------------------------------------------------------------------
// Upshift a simple char string
// -----------------------------------------------------------------------
void ComRt_Upshift (char * buf);

const char * ComRtGetEnvValueFromEnvvars(const char ** envvars,
					 const char * envvar,
					 Lng32 * envvarPos = NULL);

#if defined (_DEBUG) && !defined (ARKFS_OPEN) && !defined (__EID)
// -----------------------------------------------------------------------
// Convenient handling of envvars: Return a value if one exists
// NB: DEBUG mode only!
// -----------------------------------------------------------------------
NABoolean ComRtGetEnvValue(const char * envvar, const char ** envvarValue = NULL);

NABoolean ComRtGetEnvValue(const char * envvar, Lng32 * envvarValue);

NABoolean ComRtGetValueFromFile (const char * envvar, char * valueBuffer,
                                 const UInt32 valueBufferSizeInBytes);
#endif // #if defined (_DEBUG) ...

// -----------------------------------------------------------------------
// Get the MX system catalog name.
// -----------------------------------------------------------------------
Lng32 ComRtGetMXSysVolName(
     char *sysCatBuffer,                /* out */
     Lng32 inputBufferLength,            /* in  */
     Lng32 *sysCatLength,                /* out */
     const char *nodeName,              /* in */
     NABoolean fakeReadError,           /* in */
     NABoolean fakeCorruptAnchorError   /* in */
     );

// -----------------------------------------------------------------------
// Extract System MetaData Location ( VolumeName ).
// -----------------------------------------------------------------------
Lng32 extract_SMDLocation(
     char *buffer, /* in */
     Int32 bufferLength, /* in */
     char *SMDLocation); /* out */

// -----------------------------------------------------------------------
// Validate MetaData Location ( VolumeName ) format.
// -----------------------------------------------------------------------
Lng32 validate_SMDLocation(
      char *SMDLocation); /* in */

#ifdef _DEBUG
// -----------------------------------------------------------------------
// Print an NT memory map (debug only)
// -----------------------------------------------------------------------
#if (MSC_VER >= 1300)
void ComRtDisplayVirtualMemoryMap(std::ostream* outstream);
#else
void ComRtDisplayVirtualMemoryMap(ostream* outstream);
#endif
#endif
NABoolean ComRtIsNeoSystem(void);

typedef struct SEGMENT_INFO
{
  char segName_[MAX_SEGMENT_NAME_LEN+1];
  Lng32 segNo_;
  Lng32 noOfCpus_;
  Lng32 cpuStatus_;
  NABoolean nodeDown_;
} SEGMENT_INFO;
Lng32 ComRtGetSegsInfo(SEGMENT_INFO *segs, Lng32 maxNoSegs, Lng32 &noOfSegs,
		NAHeap *heap);
NABoolean ComRtGetCpuStatus(char *nodeName, short cpuNum);
Lng32 ComRtTransIdToText(Int64 transId, char *buf, short len);

// A function to return the string "UNKNOWN (<val>)" which can be
// useful when displaying values from an enumeration and an unexpected
// value is encountered. The function is thread-safe. The returned
// string can be overwritten by another call to the function from the
// same thread.
const char *ComRtGetUnknownString(Int32 val);

void genLinuxCorefile(const char *eventMsg);   // no-op except on Linux.

#ifdef _DEBUG
static THREAD_P UInt32 TraceAllocSize = 0;
void saveTrafStack(LIST(TrafAddrStack*) *la, void *addr);
bool delTrafStack(LIST(TrafAddrStack*) *la, void *addr);
void dumpTrafStack(LIST(TrafAddrStack *) *la, const char *header, bool toFile = false);
#endif // DEBUG

Int16 getBDRClusterName(char *bdrClusterName);

SB_Phandle_Type *get_phandle_with_retry(char *pname, short *fserr = NULL);

#endif // COMRTUTILS_H
