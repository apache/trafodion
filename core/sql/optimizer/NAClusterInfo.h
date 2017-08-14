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
// This file contains a class, NAClusterInfo, which provides information about
// the cluster in which SQL/MX is running. It assumes that all nodes in
// it are identical (in terms of parameters like the number of CPUs and the
// kind of CPU they are, as well as on the memory available and the page size
// etc.) It also tells which DP2 processs are running in which SMP. It assumes
// that SMPs are numbered 0, 1, 2, ... etc

#ifndef __NA_CLUSTER_INFO_H
#define __NA_CLUSTER_INFO_H

#include "Platform.h"
#include "CmpCommon.h"
#include "ObjectNames.h"

#include "seabed/ms.h"

void SetStaticCompiler(NABoolean isStaticCompiler);
NABoolean IsStaticCompiler();

//-----------------------------
// Classes defined in this file
//-----------------------------

class NodeMapEntry;
class NodeMap;
class NAClusterInfo;

//----------------------
// Known processor types
//----------------------
 enum ProcesorTypes {
   CPU_ARCH_INTEL_80386,
   CPU_ARCH_INTEL_80486,
   CPU_ARCH_PENTIUM,
   CPU_ARCH_PENTIUM_PRO,
   CPU_ARCH_MIPS,
   CPU_ARCH_ALPHA,
   CPU_ARCH_PPC,
   CPU_ARCH_UNKNOWN
 };
//<pb>
//------------------------------------------------------------------------------
//  Cluster information is truly global.  If ARKCMP ever becomes multi-threaded,
// this cluster information should be accessible to all threads.
//
//  Cluster information is set up once and then used until ARKCMP terminates.
// The only way to refresh the cluster information is to stop and restart
// ARKCMP.
//------------------------------------------------------------------------------
extern THREAD_P NAClusterInfo* gpClusterInfo;
extern void setUpClusterInfo(CollHeap* heap);

class NAClusterInfo : public NABasicObject
{
public:
 friend class NADefaults;
   

  NAClusterInfo(CollHeap * heap);
  ~NAClusterInfo();

  virtual Int32      processorFrequency() const = 0;
  virtual float    ioTransferRate() const = 0;
  virtual float    seekTime() const = 0;
  virtual Int32      cpuArchitecture() const = 0;

  virtual size_t   numberOfCpusPerSMP() const = 0;

  virtual size_t   pageSize() const = 0;
  virtual size_t   physicalMemoryAvailable() const = 0;
  virtual size_t   totalMemoryAvailable() const = 0;
  virtual size_t   virtualMemoryAvailable() = 0;

  // number of physical nodes (from Trafodion monitor or OSIM)
  Int32 numOfPhysicalSMPs();
  // this is an adjusted number, based on CQDs
  Int32 numOfSMPs();

  // This is called by captureNAClusterInfo() to capture the OSIM
  // information that is specific to the operating system. Each new
  // platform must define this.
  virtual void captureOSInfo(ofstream & f) const = 0;

  Int32 getNumActiveCluster() const { return 1; }
  NABoolean smpActive(Int32 smp) const;

  // return total number of CPUs (includes all, that is, even down CPUs)
  Lng32 getTotalNumberOfCPUs();
  const NAArray<CollIndex> &getCPUArray() { return cpuArray_; }

  Lng32 mapNodeNameToNodeNum(const NAString &node) const;
  void cleanupPerStatement();

  // The OSIM uses these following methods to capture and simulate
  // cluster information respectively.
  void initializeForOSIMCapture();
  void captureNAClusterInfo(ofstream & naclfile);
  void simulateNAClusterInfo();
  NABoolean NODE_ID_TO_NAME(Int32 nodeId, char *nodeName, short maxLen, short *actualLen);
  // three methods to enter, leave and test the test mode. The test 
  // mode is for testing POS.

// LCOV_EXCL_START
  NABoolean inTestMode() const { return inTestMode_; };
  void setTestMode() { inTestMode_ = TRUE; };
  void resetTestMode() { inTestMode_ = FALSE; };
// LCOV_EXCL_STOP

protected :

  //------------------------------------------------------------------------
  // localSMP_ is the current node ID.
  //------------------------------------------------------------------------
  short localSMP_;

  //------------------------------------------------------------------------
  // heap_ is where this NAClusterInfo was allocated.  This should be the
  // context heap.
  //------------------------------------------------------------------------
  CollHeap * heap_;

  // ------------------------------------------------------------------------
  // A list of node ids of available nodes. Typically, this will be
  // a list of the numbers 0 ... n-1 but in some cases a node in
  // the middle may be removed, so we end up with "holes" in the
  // node ids.
  // ------------------------------------------------------------------------
  NAArray<CollIndex> cpuArray_;

  //------------------------------------------------------------------------
  // hashdictionary used to store the mapping of cluster name to cluster id
  // This structure is stored on the context heap
  // because we don't expect this mapping to change during a session..
  //------------------------------------------------------------------------
  NAHashDictionary<Int32, NAString>* nodeIdToNodeNameMap_;

  // hashdictionary that maps nodeName to nodeId.
  NAHashDictionary<NAString, Int32> *nodeNameToNodeIdMap_;

  NABoolean inTestMode_; // test mode indicator
};

class NAClusterInfoLinux : public NAClusterInfo
{
public:
   NAClusterInfoLinux(CollHeap * heap);
   ~NAClusterInfoLinux();
   Int32      processorFrequency() const;
   float    ioTransferRate() const;
   float    seekTime() const;
   Int32      cpuArchitecture() const;

   //-------------------------------------------------------------------------
   // On Linux, numberOfCpusPerSMP() returns the number of Linux nodes in the
   // cluster.
   //-------------------------------------------------------------------------
   size_t   numberOfCpusPerSMP() const;

   size_t   pageSize() const;
   size_t   physicalMemoryAvailable() const;
   size_t   totalMemoryAvailable() const;
   size_t   virtualMemoryAvailable();

   void captureOSInfo(ofstream &) const;

   Int32 get_pid() { return pid_; };
   Int32 get_nid() { return nid_; };

private:
   void     determineLinuxSysInfo();

   void     simulateNAClusterInfoLinux();

   int pid_; // the pid of the current process 
   int nid_; // the nid of the current process 

private:

  //-------------------------------------------------------------------------
  // Stores the frequency of the SMP, in Megahertz
  //-------------------------------------------------------------------------
  Int32           frequency_;

  //-------------------------------------------------------------------------
  // Stores the IO transfer rate of the disk, in MB/sec
  //-------------------------------------------------------------------------
  float         iorate_;

  //-------------------------------------------------------------------------
  // Stores the average seek time of the disk, in ms
  //-------------------------------------------------------------------------
  float         seekTime_;

  //-------------------------------------------------------------------------
  // Stores the memory page size, in kilobytes.
  //-------------------------------------------------------------------------
  size_t        pageSize_;

  //-------------------------------------------------------------------------
  // Stores the total memory available, in bytes.
  //-------------------------------------------------------------------------
  size_t totalMemoryAvailable_;

  //-------------------------------------------------------------------------
  // Number of CPU cores per Linux node.
  //-------------------------------------------------------------------------
  size_t numCPUcoresPerNode_;

};

#endif // __NA_CLUSTER_INFO_H
