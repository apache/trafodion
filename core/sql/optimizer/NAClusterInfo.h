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

#define MAX_NUM_SMPS_NSK  16     // number of SMPs in the cluster for NSK
#define MAX_NUM_SMPS_SQ   512    // number of CPUs in the cluster for SQ
//<pb>
ULng32 clusterNumHashFunc(const CollIndex& num);

//used to encapsulate dp2 names
class DP2name : public NABasicObject
{
  public:
     DP2name(char* dp2name,CollHeap* heap);
     void getDp2Name(char* &name)const;
     NABoolean operator==(const DP2name & dp2Name);
     ULng32 hash() const;
     ~DP2name();
  private:
    char* dp2name_;
    CollHeap* heap_;
};

//encapsulated information for a dp2 like cluster no., primary and secondary
//cpu.
class DP2info : public NABasicObject
{
  public:
    DP2info(Lng32 clusterNum,Lng32 primary, Lng32 secondary);
    void getDp2Info(Int32 & clusterNum, Int32& primary, Int32& secondary);
    NABoolean operator==(DP2info dp2info)
    {
      if(dp2info.clusterNumber_==clusterNumber_ && dp2info.primaryCPU_ == primaryCPU_)
        return TRUE;

      return FALSE;

    }
  private:
    Lng32 clusterNumber_;
    Lng32 primaryCPU_;
    Lng32 secondaryCPU_;
};

//encapsulated a basic list, provides a equality method and always requires
//a heap pointer.

class maps : public NABasicObject {
public:
 
  maps(CollHeap * heap, CollIndex size =0)
  {
    list= new(heap) NAList<CollIndex>(heap,size);
    listOfAggregationOnlyNodes = new(heap) NAList<CollIndex>(heap,size);
  }

  maps(NAList<CollIndex>* list, NAList<CollIndex>* listForAggrNodes) 
  {
    this->list = list;
    this->listOfAggregationOnlyNodes = listForAggrNodes;
  }

  NABoolean operator==(maps cpuList)
  {
    if(*list==*(cpuList.list) && 
       *listOfAggregationOnlyNodes == *(cpuList.listOfAggregationOnlyNodes)) return TRUE;
    return FALSE;
  }

  ~maps()
  {
    delete list;
    delete listOfAggregationOnlyNodes;
  }

  Int32 getCpuCount(NABoolean aggregationNodeOnly);
  NAList<CollIndex>*  getCpuList(NABoolean aggregationNodeOnly = FALSE);

  void insertToAggregationNodeList(CollIndex cpu) 
    { listOfAggregationOnlyNodes->insert(cpu); };

  NAList<CollIndex>* list;
  NAList<CollIndex>* listOfAggregationOnlyNodes;
};

// for osim to work on nt
#define FileSystemErrorRemoteNodeDown 250
#define FileSystemErrorRemoteNodeUnavailable 18
#define FileSystemErrorNamedProcessNotInDCT 14

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

  NABoolean smpActive(Int32 smp) const;

  //This method returns the list of active clusters and a list of CPU for each
  //cluster in the same order. It decides on active clusters based on default
  //REMOTE_ESP_PARALLELISM. If we are going to implement CPU map, only this
  //method needs to be augmented.
  NABoolean getSuperNodemap(NAArray<CollIndex>* &clusterList,
			    NAArray<NAList<CollIndex>*>* &cpuList, Int32 &cpuCount);

  // return total number of CPUs (includes all, that is, even down CPUs)
  Lng32 getTotalNumberOfCPUs();

  Lng32 getNumActiveCluster();
  Lng32 mapNodeNameToNodeNum(const NAString &node) const;
  void cleanupPerStatement();

  void setMaxOSV(QualifiedName &qualName, COM_VERSION osv);

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

  NABoolean getUseAggregationNodesOnly() const 
      { return useAggregationNodesOnly_; }

  void setUseAggregationNodesOnly(NABoolean x);

protected :

  Int32 computeNumOfSMPs();

  //Helper function for getSuperNodeMap(). This actually implements the
  //active cluster algorithm.
  void createActiveClusterList();

  void getProcessorStatus(maps* & outcpuList,short clusterNum);

  //------------------------------------------------------------------------
  // localCluster_ used to be the segment number.  On Linux, it is
  // set to zero.
  //------------------------------------------------------------------------
  Int32 localCluster_;

  //------------------------------------------------------------------------
  // On NSK, localSMP_ is the CPU number within the segment.  On Linux,
  // localSMP_ is the current node ID.
  //------------------------------------------------------------------------
  short localSMP_;

  //------------------------------------------------------------------------
  // physical number of SMPs/Linux nodes (real configuration or OSIM config)
  //------------------------------------------------------------------------
  Int32 physicalSMPCount_;

  //------------------------------------------------------------------------
  // heap_ is where this NAClusterInfo was allocated.  This should be the
  // context heap.
  //------------------------------------------------------------------------
  CollHeap * heap_;

  //------------------------------------------------------------------------
  // hashdictionary used to store the mapping of cluster name to cluster id
  // This structure is stored on the context heap
  // because we don't expect this mapping to change during a session..
  //------------------------------------------------------------------------
  NAHashDictionary<Int32, NAString>* nodeIdToNodeNameMap_;

  // ------------------------------------------------------------------------
  // On NSK and Windows, this maps from cluster number to its cpu
  // configuration.  On Linux, this maps the single system number to the
  // Linux node IDs that are configured as aggregation (compute) nodes.
  // This is also stored on the context heap as we don't expect it to change
  // during a session.
  // ------------------------------------------------------------------------
  NAHashDictionary<CollIndex,maps> * clusterToCPUMap_;

  // ------------------------------------------------------------------------

  // hashdictionary that maps nodeName to nodeId.
  NAHashDictionary<NAString, Int32> *nodeNameToNodeIdMap_;

  // List containing the active clusters or the super node map where ESP's
  // will be brought up.  
  // This is stored on the statement heap.
  // ------------------------------------------------------------------------
  NAList<CollIndex> * activeClusters_;

  NABoolean inTestMode_; // test mode indicator

  NABoolean useAggregationNodesOnly_; 

private:
  static NABoolean IsRemoteNodeDown(short error);
  static const char *GetNodeName(const char *dp2Name, char *buffer, Int32 size);

  COM_VERSION maxOSV_;
  QualifiedName maxOSVName_;
};

#define MAX_NUM_TSES 1024

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

   // get the kth TSE entry from a list of sorted TSE elements
   MS_Mon_Process_Info_Type* getTSEInfoForPOS(Int32 k);

   // get the total number of TSE elements
   Int32 numTSEsForPOS(); 

   Int32 get_pid() { return pid_; };
   Int32 get_nid() { return nid_; };

protected:

   void setupTSEinfoForPOS();

private:
   void     determineLinuxSysInfo();

   void     simulateNAClusterInfoLinux();

   // TSE info used by POS
   MS_Mon_Process_Info_Type* tseInfo_;
   Int32 numTSEs_;

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
