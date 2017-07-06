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
// Implementation for the classes listed in SystemParameters.h

#define pdctctlz_h_dct_get_by_name_	 // so that we only get dct_get_by_name
#define pdctctlz_h_including_section	 // from pdctctlz.h
#define pdctctlz_h_

//
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
//                            !!! ATTENTION !!!
//
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// For some reason, due (I believe) to strange circular dependencies, by
// adding the compiler directive <#include "NAStringDef.h"> and using
// NAString objects, the linker spits out some extremely bizarre
// (completely inexplicable) errors while building tdm_sqlcli.dll.
//
// So : the code below was rewritten to remove all usage of NAString
// objects.  
//
//
// $$$ NB: ENSCRIBE ERROR CODES NOT CHECKED !!!
//
// Currently, the class NAClusterInfo calls a private method
// ::filloutDisks(), which calls a few subroutines from Enscribe.
// However, these calls ignore the return values from Enscribe, and this
// is clearly incorrect behavior.  Someday (soon ...?) we need to change
// the signature of the NAClusterInfo ctors so that the user of this class
// can say whether we should handle (i.e., longjmp on) fatal errors
// returned from Enscribe.  Given this parameter to the method
// ::filloutDisks(), we then will know whether we need to check these
// return error codes or not.

#include "NAClusterInfo.h"
#include "NADefaults.h"
#include "CompException.h"
#include <cextdecs/cextdecs.h>
#include <limits.h>

//str
#include <string.h>
//#include "NAStringDef.h"
//str

#include <stdio.h>
#include "guardian/pdctctlz.h"
#include "nsk/nskport.h"

#include "NodeMap.h"
#include "NATable.h"
#include "SchemaDB.h"

#include "ComRtUtils.h"


#include "OptimizerSimulator.h"
// Global pointer to OptimizerSimulator
#include "CmpErrors.h"

#include "seabed/ms.h"
#include <cstdlib>
#include <sys/stat.h>

THREAD_P NABoolean gIsStaticCompiler = FALSE;
void SetStaticCompiler(NABoolean isStaticCompiler)
{
  gIsStaticCompiler = isStaticCompiler;
}

// LCOV_EXCL_START
NABoolean IsStaticCompiler()
{
  return gIsStaticCompiler;
} 
// LCOV_EXCL_STOP

//------------------------------------------------------------------------
// Global pointer to cluster information is initially null and remains so
// until actually required.
//------------------------------------------------------------------------
THREAD_P NAClusterInfo* gpClusterInfo = NULL;
//<pb>
//==============================================================================
//  Set up global pointer to cluster information if this hasn't been done 
// previously.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void setUpClusterInfo(CollHeap* heap)
{
  #ifndef NDEBUG
  // LCOV_EXCL_START
  if (getenv("NO_SERVICES"))  // KSKSKS
    return;                   // KSKSKS
  // LCOV_EXCL_STOP
  #endif


  //-------------------------------------------------------
  // Set up cluster information based on hardware platform.
  //-------------------------------------------------------
  if (OSIM_runningSimulation() && !OSIM_ClusterInfoInitialized())
  {
    switch (CURRCONTEXT_OPTSIMULATOR->getCaptureSysType())
    {
        case OptimizerSimulator::OSIM_LINUX:
            if(gpClusterInfo) NADELETEBASIC(gpClusterInfo, heap);
            gpClusterInfo = new (heap) NAClusterInfoLinux (heap);
            break;
        default:
            CMPASSERT(0); // Case not handled
            break;
    }
  } 
  else if(OSIM_runningInCaptureMode() && !OSIM_ClusterInfoInitialized())
  {
      if(gpClusterInfo) NADELETEBASIC(gpClusterInfo, heap);
      gpClusterInfo = new (heap) NAClusterInfoLinux (heap);
  }
  else
  {
    //-------------------------------------------
    // Return now if cluster info already set up.
    //-------------------------------------------
    if (!gpClusterInfo)
        gpClusterInfo = new (heap) NAClusterInfoLinux (heap);
  }
}
//<pb>

static ULng32 intHashFunc(const Int32& Int)
{
  return (ULng32)Int;
}

//============================================================================
// Methods for class NAClusterInfo; it provides information about the cluster
// in which we are running.
//============================================================================

//============================================================================
//  NAClusterInfo constructor.
//
// Input: heap pointer(should always be context heap
//  
//
// Output: Retrieves information for the local cluster. This includes information
// regarding its CPUs as well as for the dp2s. All these information will be cached
// in the appropriate structure.
//
// Return:
//  none
//
//==============================================================================
NAClusterInfo::NAClusterInfo(CollHeap * heap)
 : heap_(heap), 
   cpuList_(heap),
   inTestMode_(FALSE)
{
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(CURRCONTEXT_OPTSIMULATOR && 
     !CURRCONTEXT_OPTSIMULATOR->isCallDisabled(9))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::LOAD:
    case OptimizerSimulator::CAPTURE:
    {
      Int32 dummyClusterNum;

      // Hash Map to store NodeName and NoideIds
      nodeNameToNodeIdMap_ = new (heap) NAHashDictionary<NAString, Int32>
          (NAString::hash, 101, TRUE, heap_);
      nodeIdToNodeNameMap_ = new(heap) NAHashDictionary<Int32, NAString>
                                                          (&intHashFunc, 101,TRUE,heap);
                                                          
      NADefaults::getNodeAndClusterNumbers(localSMP_ , dummyClusterNum);

      Int32 nodeCount = 0;
      Int32 nodeMax = 0;
      MS_Mon_Node_Info_Entry_Type *nodeInfo = NULL;

      // Get the number of nodes to know how much info space to allocate
      Int32 error = msg_mon_get_node_info(&nodeCount, 0, NULL);
      CMPASSERT(error == 0);
      CMPASSERT(nodeCount > 0);

      // Allocate the space for node info entries
      nodeInfo = (MS_Mon_Node_Info_Entry_Type *) new(heap) 
                  char[nodeCount * sizeof(MS_Mon_Node_Info_Entry_Type)];
      CMPASSERT(nodeInfo);


      // Get the node info
      memset(nodeInfo, 0, sizeof(nodeInfo));
      nodeMax = nodeCount;
      error = msg_mon_get_node_info(&nodeCount, nodeMax, nodeInfo);
      CMPASSERT(error == 0);

      for (Int32 i = 0; i < nodeCount; i++)
      {
        if (nodeInfo[i].spare_node)
          continue;

        // The zone type must either be an aggregation node or storage node
        // to be included in the list of CPUs.
        if ((nodeInfo[i].type & MS_Mon_ZoneType_Aggregation) != 0 ||
            ((nodeInfo[i].type & MS_Mon_ZoneType_Storage) != 0 ))
        {
          cpuList_.insert(nodeInfo[i].nid);

          // store nodeName-nodeId pairs
          NAString *key_nodeName = new (heap_) NAString(nodeInfo[i].node_name, heap_);
          size_t pos = key_nodeName->index('.');
          if (pos && pos != NA_NPOS)
            key_nodeName->remove(pos);
#ifdef _DEBUG
          else {
             // The node names for virtual nodes seen with workstations are of
             // format <nodeName>:0, <nodeName>:1 etc. In debug mode, we work with
             // such node names by removing all substrings starting at ':' and 
             // insert the node name into the nodeIdToNodeNameMap_.
             pos = key_nodeName->index(':');
             if (pos && pos != NA_NPOS)
               key_nodeName->remove(pos);
          }
#endif

          Int32 *val_nodeId = new Int32(nodeInfo[i].nid);
          nodeNameToNodeIdMap_->insert(key_nodeName, val_nodeId);

          // store nodeId->nadeName 
          //share the same memory with nodeNameToNodeIdMap_
          nodeIdToNodeNameMap_->insert(val_nodeId, key_nodeName);
        }
      }

      NADELETEBASIC(nodeInfo, heap);

      break;
    }
    case OptimizerSimulator::SIMULATE:

      nodeIdToNodeNameMap_ = NULL;
      cpuList_.clear();
      //load NAClusterInfo from OSIM file
      simulateNAClusterInfo();
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
} // NAClusterInfo::NAClusterInfo()

NAClusterInfo::~NAClusterInfo()
{
  if (nodeNameToNodeIdMap_)
  {
    nodeNameToNodeIdMap_->clear();
    delete nodeNameToNodeIdMap_;
  }

  if(nodeIdToNodeNameMap_)
  {
    nodeIdToNodeNameMap_->clear();
    delete nodeIdToNodeNameMap_;
  }
}

Lng32
NAClusterInfo::mapNodeNameToNodeNum(const NAString &keyNodeName) const
{
  if ( nodeNameToNodeIdMap_->contains(&keyNodeName) )
  {
    Int32 *nodeValue = nodeNameToNodeIdMap_->getFirstValue(&keyNodeName);
    return *nodeValue;
  }
  else return ANY_NODE;

} // NodeMap::getNodeNmber

NABoolean NAClusterInfo::NODE_ID_TO_NAME(Int32 nodeId, char *nodeName, short maxLen, short *actualLen)
{
    //Currently, this method behaves as same as NODENUMBER_TO_NODENAME_(),
    //which always returns "\\NSK", the only reason for doing this is to
    //avoid diff in regression test and core file dumped when exiting sqlci.(don't know why.)
    NODENUMBER_TO_NODENAME_(nodeId, nodeName, maxLen, actualLen);
    return TRUE;
    //Following code may be used in future to provide real node id to name map.
    *actualLen = 0;
    if (nodeIdToNodeNameMap_->contains(&nodeId))
    {
        NAString * value = nodeIdToNodeNameMap_->getFirstValue(&nodeId);
        *actualLen = value->length();
        strncpy(nodeName, value->data(), maxLen < (*actualLen) ? maxLen : (*actualLen));
        return TRUE;
    }
    return FALSE;
}

Int32
NAClusterInfo::numOfPhysicalSMPs()
{
  return cpuList_.entries();
}

Int32
NAClusterInfo::numOfSMPs()
{
  Int32 result = cpuList_.entries();

  // This is temporary patch for PARALLEL_NUM_ESPS issue. This CQD should
  // be used in many places for costing, NodeMap allocation, synthesizing
  // physProperties and so on. But currently it is used only in
  // RelRoot::createContextForAChild() creating lots of discrepansies in 
  // the code. Sept. 2006

  // Get the value as a token code, no errmsg if not a keyword.
  if ( (CmpCommon::getDefault(COMP_BOOL_136) == DF_ON) AND
       (CmpCommon::getDefault(PARALLEL_NUM_ESPS, 0) != DF_SYSTEM)
     )
  {
    // -------------------------------------------------------------------
    // A value for PARALLEL_NUM_ESPS exists.  Use it for the count of cpus
    //  but don't exceed the number of cpus available in the cluster.
    // -------------------------------------------------------------------
    result = MINOF(result, 
        (Int32)(ActiveSchemaDB()->getDefaults().getAsLong(PARALLEL_NUM_ESPS)));
  }

  return result; 

} // NAClusterInfo::numOfSMPs()  
#pragma warn(1506)  // warning elimination 

// Returns total number of CPUs (including down CPUs)
Lng32 NAClusterInfo::getTotalNumberOfCPUs()
{
  Lng32 cpuCount = cpuList_.entries();

#ifndef NDEBUG
// LCOV_EXCL_START
  if ( inTestMode() ) {
    NADefaults & defs = ActiveSchemaDB()->getDefaults();
    cpuCount = (Int32)(defs.getAsLong(POS_TEST_NUM_NODES));
  }
// LCOV_EXCL_STOP
#endif
  // 
  return cpuCount;
}

void NAClusterInfo::cleanupPerStatement()
{
}

void NAClusterInfo::initializeForOSIMCapture()
{
}

NAClusterInfoLinux::NAClusterInfoLinux(CollHeap * heap) : NAClusterInfo(heap),
                                                          nid_(0), pid_(0)
{
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(CURRCONTEXT_OPTSIMULATOR && 
     !CURRCONTEXT_OPTSIMULATOR->isCallDisabled(9))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::CAPTURE:
    case OptimizerSimulator::LOAD:
      determineLinuxSysInfo();

      // For CAPTURE mode, the data will be captured later in CmpMain::compile()
      break;
    case OptimizerSimulator::SIMULATE:
      // Simulate the NAClusterInfo.
      simulateNAClusterInfoLinux();
      break;
    // LCOV_EXCL_START
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
    // LCOV_EXCL_STOP
  }
}

NAClusterInfoLinux::~NAClusterInfoLinux()
{
}

Int32 NAClusterInfoLinux::processorFrequency() const
{
  return frequency_;
}

float NAClusterInfoLinux::ioTransferRate() const
{
  return iorate_;
}

float NAClusterInfoLinux::seekTime() const
{
  return seekTime_;
}

Int32 NAClusterInfoLinux::cpuArchitecture() const
{
  return CPU_ARCH_UNKNOWN;
}

size_t NAClusterInfoLinux::numberOfCpusPerSMP() const
{
  return numCPUcoresPerNode_;
}

size_t NAClusterInfoLinux::pageSize() const
{
  return pageSize_;
}

// Return the physical memory available in kilobytes
size_t NAClusterInfoLinux::physicalMemoryAvailable() const
{
  // NSK returns the total memory available so we do the same thing
  // on Linux.  This allows the plans to stay constant even as
  // the amount of memory fluctuates.
  return totalMemoryAvailable_;
}

size_t NAClusterInfoLinux::totalMemoryAvailable() const
{
  return totalMemoryAvailable_;
}

size_t NAClusterInfoLinux::virtualMemoryAvailable()
{
  // Just return a constant (like NSK does).
  return 256000000/1024;
}

#define LINUX_DEFAULT_FREQ 3000
#define LINUX_IO_RATE  75.0
#define LINUX_SEEK_RATE 0.0038

void NAClusterInfoLinux::determineLinuxSysInfo()
{
  // Set the page size in killobytes and determine how much memory
  // is available on this node (in kilobytes).
  pageSize_ = (size_t)sysconf(_SC_PAGESIZE) / 1024U;
  totalMemoryAvailable_ = pageSize_ * (size_t)sysconf(_SC_PHYS_PAGES);
  numCPUcoresPerNode_ = sysconf(_SC_NPROCESSORS_ONLN);

  frequency_ = 0.0;

  // Read the CPU frequency from the sysfs filesystem.
  ifstream infoFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
  if (infoFile.fail()) {
    // This code should log a warning.

    // use /proc/cpuinfo
    char var[256];
    ifstream cpuInfoFile("/proc/cpuinfo");
    const char* freqToken = "cpu MHz";
    Lng32 freqTokenLen = strlen(freqToken);
    while(cpuInfoFile.good())
    {
      // Read the variable name from the file.
      cpuInfoFile.getline(var, sizeof(var), ':'); // read the token part
      Lng32 len = strlen(var);
      if(len >= freqTokenLen && !strncmp(var, freqToken, freqTokenLen))
      {
        cpuInfoFile >> frequency_;
        break;
      }
      cpuInfoFile.getline(var, sizeof(var)); // read the value part
    }

    if ( frequency_ == 0.0 )
       // Use the default frequency
       frequency_ = LINUX_DEFAULT_FREQ;
  } else {
    ULng32 freqUlongVal;
    infoFile >> freqUlongVal;
    frequency_ = freqUlongVal / 1000;
    infoFile.close();
  }

  // These should be determined programmatically, but are hard-coded for now.
  iorate_ = LINUX_IO_RATE;
  seekTime_ = LINUX_SEEK_RATE;
}

//============================================================================
// This method writes the information related to the NAClusterInfoLinux class
// to a logfile called "NAClusterInfo.txt".
//============================================================================
void NAClusterInfoLinux::captureOSInfo(ofstream & nacllinuxfile) const
{

  nacllinuxfile << "frequency_: " << frequency_ << endl
                << "iorate_: " << iorate_ << endl
                << "seekTime_: "<< seekTime_ << endl
                << "pageSize_: " << pageSize_ << endl
                << "totalMemoryAvailable_: " << totalMemoryAvailable_ << endl
                << "numCPUcoresPerNode_: " << numCPUcoresPerNode_ << endl;
}
