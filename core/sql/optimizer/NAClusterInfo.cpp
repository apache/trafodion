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

ULng32 dp2DescHashFunc(const DP2name& name);

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

//---------------------------------------------------------
//DP2name is a wrapper class for fully specified dp2 names. 
//We cannot hash on primitives like characters.
//--------------------------------------------------------
DP2name::DP2name(char* dp2name, CollHeap* heap)
: heap_(heap)
{
  dp2name_=new(heap_) (char[strlen(dp2name)+1]);
  strcpy(dp2name_,dp2name);
}

void DP2name::getDp2Name(char* &name) const
{
  name = new (CmpCommon::statementHeap()) char[strlen(dp2name_)+1];
  strcpy(name,dp2name_);
}

inline NABoolean DP2name::operator==(const DP2name &dp2Name)
{
  if(strcmp(dp2Name.dp2name_,dp2name_)==0) return TRUE;
  return FALSE;
}

// LCOV_EXCL_START
ULng32 DP2name::hash() const 
{
  return dp2DescHashFunc(*this);
}
// LCOV_EXCL_STOP

DP2name::~DP2name()
{
  NADELETEBASIC(dp2name_,heap_);
}

//------------------------------------------------------------
//DP2Info is a wrapper class for cluster and CPU information for a 
//particular DP2. As a member it has a DP2Name and 3 integers 
//representing the cluster, primary CPU and secondary CPU associated 
//with the DP2.
//-------------------------------------------------------------  

DP2info::DP2info(Lng32 clusterNum, Lng32 primary, Lng32 secondary)
{
  clusterNumber_=clusterNum;
  primaryCPU_=primary;
  secondaryCPU_=secondary;

}

void DP2info::getDp2Info(Int32  & clusterNum,
                         Int32  & primary,
                         Int32  & secondary)
{
  clusterNum=clusterNumber_;
  primary=primaryCPU_;
  secondary=secondaryCPU_;
}

//<pb>


//  hash function for dp2name
ULng32 dp2DescHashFunc(const DP2name& name) 
{
  char * dp2name;
 
  // method getDp2Name allocates an array of char for dp2name
  name.getDp2Name(dp2name);

  ULng32 index=0;
  size_t nameLen = strlen(dp2name);
  for(CollIndex i=0;i<=nameLen;i++){
    index += (unsigned char) dp2name[i];
  }

  NADELETEBASIC(dp2name, CmpCommon::statementHeap());

  return index;
}

//hash funtion for tableIdentifier passed in from NATable
ULng32 tableIdentHashFunc( const CollIndex& ident)
{
  return (ULng32)(ident);
}
//hash function for clusterNumber
ULng32 clusterNumHashFunc(const CollIndex& num)
{
  return (ULng32)(num);
}

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
   maxOSV_(COM_VERS_UNKNOWN),
   maxOSVName_(heap),
   inTestMode_(FALSE),
   useAggregationNodesOnly_(FALSE)
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
      // Hash Map to store NodeName and NoideIds
      nodeNameToNodeIdMap_ = new (heap) NAHashDictionary<NAString, Int32>
          (NAString::hash, 101, TRUE, heap_);
          
      clusterToCPUMap_ = new(heap) NAHashDictionary<CollIndex,maps>
                                                          (&clusterNumHashFunc,17,TRUE, heap);
                                                          
      nodeIdToNodeNameMap_ = new(heap) NAHashDictionary<Int32, NAString>
                                                          (&intHashFunc, 101,TRUE,heap);
                                                          
      activeClusters_= NULL;
      physicalSMPCount_ = -1;

      NADefaults::getNodeAndClusterNumbers(localSMP_ , localCluster_);

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

      maps *cpuList=new(heap) maps(heap);
      physicalSMPCount_ = 0;

      NAList<CollIndex> storageList(heap, nodeCount);

      for (Int32 i = 0; i < nodeCount; i++)
      {
        if (nodeInfo[i].spare_node)
          continue;

        // The zone type must either be an aggregation node or storage node
        // to be included in the list of CPUs.
        if ((nodeInfo[i].type & MS_Mon_ZoneType_Aggregation) != 0 ||
            ((nodeInfo[i].type & MS_Mon_ZoneType_Storage) != 0 ))
        {
          if ( (nodeInfo[i].type & MS_Mon_ZoneType_Storage) != 0 )
            storageList.insert(nodeInfo[i].nid);

          if ( (nodeInfo[i].type & MS_Mon_ZoneType_Storage) == 0 )
            cpuList->insertToAggregationNodeList(nodeInfo[i].nid);

          if (!nodeInfo[i].spare_node)
             physicalSMPCount_++;

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

      // Fix Bugzilla #1210. Put the aggregation nodes at the beginning of 
      // the list. ESP logical node map synthesization code can take the 
      // advantage of this and place more ESPs on aggregation nodes when 
      // the node map size is less than the total number of SQL nodes.
      *(cpuList->list) = *(cpuList->listOfAggregationOnlyNodes);
      cpuList->list->insert(storageList);

      // if there exists no aggregation only nodes, allow all nodes to
      // host esps.
      if (cpuList->listOfAggregationOnlyNodes->entries() == 0) {
        for (Int32 i = 0; i<cpuList->list->entries(); i++)
          cpuList->insertToAggregationNodeList((*(cpuList->list))[i]);
      }

      NADELETEBASIC(nodeInfo, heap);


      CollIndex *ptrClusterNum = new(heap) CollIndex(localCluster_);
      CollIndex *cluster=clusterToCPUMap_->insert(ptrClusterNum,cpuList); 

      CMPASSERT(cluster);

      break;
    }
    case OptimizerSimulator::SIMULATE:

      clusterToCPUMap_ = NULL;
      nodeIdToNodeNameMap_ = NULL;
      activeClusters_= NULL;
      physicalSMPCount_ = -1;
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

  CollIndex *key;  
  maps * value;
  UInt32 i=0;

  if(clusterToCPUMap_)
  {
    // clear and delete clusterToCPUMap_
    //iterate over all the entries in clusterToCPUMap_
    NAHashDictionaryIterator<CollIndex,maps> clusterToCPUMapIter(*clusterToCPUMap_);

    for ( i = 0 ; i < clusterToCPUMapIter.entries() ; i++)
    {
       clusterToCPUMapIter.getNext (key,value) ;
       NADELETEBASIC(key,CmpCommon::contextHeap());
       delete value;
    }
    clusterToCPUMap_->clear();
    delete clusterToCPUMap_;
  }
  
  // clear and delete activeClusters_ list
  if(activeClusters_)
  {
    activeClusters_->clear();
    delete activeClusters_;
  }

}

Lng32
NAClusterInfo::getNumActiveCluster()
{
  if(NOT activeClusters_) createActiveClusterList();
  CMPASSERT(activeClusters_->entries());
  return activeClusters_->entries();
}// NAClusterInfo::getNumActiveClusters()

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

#pragma warn(1506)  // warning elimination 

/*------------------------------------------------------------- 
NAClusterInfo::createActiveClusterList()
 Helper function for getSuperNodeMap() 
 Goes through the following algorithm to identify 
the all the active clusters for the current statement if
value for REMOTE_ESP_PARALLELISM is SYSTEM.
It also identifies active clusters for 'ON' or 'OFF'.

ALGORITHM implemented by SQL/MX:

Query involves table A, B, C and D. 

Base table for A is distributed on systems 1, 2, 5
Base table for B is distributed on systems 3, 6
Base table for C is distributed on systems 2, 3
Base table for D is distributed on systems 4

Target systems 1,2 3, 4, 5, 6

SQL/MX will first go through the list of tables and find 
the tables that have a system common with any other table in 
the list. In this case table A has system 2 common with table C 
and vice versa and table B has system 3 common with table C and
 vice versa. Now we take the super set of systems for Table A, B 
 and C which gives us ( 1, 2, 3, 5, 6) as active systems. 
 Target system 4 got excluded.

Another example:
	A typical star join schema where the dimension tables 
        are small and the fact table is large. 

Fact table is distributed on 1, 2, 3
Dimension one table is on 1
Dimension two table is on 2

Scope chosen by SQL/MX will be 1, 2, 3 

SQL/MX is going to bring up ESPs on CPUs in all these systems and 
generate as much parallelism it can. At the same time, it tries 
to make sure that scan ESPs are co-located to be nearest to their 
dp2s or active partitions, reducing remote communication.

Special Cases:
	
a) If it is a single table query then all the target systems will be considered active.
 
b)	If the algorithm cannot come up with any active system and 
the local system is not completely restricted by the user then ESPs 
will be brought up only on the local system. On the other hand, if 
the local system is unavailable ( CPU map completely restricts the 
use of local system) then no ESP will be brought up and as a result 
there will be no parallel processing. 

------------------------------------------------------------------*/
#pragma nowarn(1506)   // warning elimination 
#pragma nowarn(262)   // warning elimination 
void 
NAClusterInfo::createActiveClusterList()
{
  //CMPASSERT(tableToClusterMap_);
  activeClusters_ = new(CmpCommon::statementHeap()) 
    NAList<CollIndex>(CmpCommon::statementHeap());

  // Linux and NT behavior
  activeClusters_->insert(localCluster_);
 
}
#pragma warn(262)  // warning elimination
#pragma warn(1506)  // warning elimination 




//<pb>
//==============================================================================
//  Determine how many SMPs are available in the cluster.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  number of available SMPs in the cluster.
//
//==============================================================================
#pragma nowarn(1506)   // warning elimination
Int32
NAClusterInfo::computeNumOfSMPs()
{
    Int32 count =0;
    if(NOT activeClusters_) createActiveClusterList();

    for(CollIndex index=0; index < activeClusters_->entries(); index++)
    {
      maps *cpuList = clusterToCPUMap_->getFirstValue(&((*activeClusters_)[index]));
      if(cpuList) 
      {
        count += cpuList->getCpuCount(getUseAggregationNodesOnly());
        continue;
      }
    
#pragma warning (disable : 4244)   //warning elimination
      getProcessorStatus(cpuList,(*activeClusters_)[index]);
#pragma warning (default : 4244)   //warning elimination
      count +=cpuList->list->entries();
    }
   return count;
}

void NAClusterInfo::setUseAggregationNodesOnly(NABoolean x) 
{  
    if ( useAggregationNodesOnly_ != x )  {
       useAggregationNodesOnly_ = x; 
       computeNumOfSMPs();
    }
}

Int32
NAClusterInfo::numOfPhysicalSMPs()
{
  if (physicalSMPCount_ < 0)
    physicalSMPCount_ = computeNumOfSMPs();

  return physicalSMPCount_;
}

Int32
NAClusterInfo::numOfSMPs()
{
  CMPASSERT(physicalSMPCount_ > 0);

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
    physicalSMPCount_ = MINOF(physicalSMPCount_, 
        (Int32)(ActiveSchemaDB()->getDefaults().getAsLong(PARALLEL_NUM_ESPS)));
  }

  return physicalSMPCount_; 

} // NAClusterInfo::numOfSMPs()  
#pragma warn(1506)  // warning elimination 

//----------------------------------------------------------
// getProcessorStatus()
// Input: clusterNumber
// Output: CPU's in the cluster that are running
//----------------------------------------------------------

void 
NAClusterInfo::getProcessorStatus(maps* &outcpuList,short clusterNum)
{
  CMPASSERT(0);
}


//-----------------------------------------------------------------
//NAClusterInfo::getSuperNodeMap()
// called by NodeMap.cpp
// Returns the active clusters and their corresponding active CPUs
// clusterList and cpuList has one to one relationship i.e. cpuList[0]
// contains cpus for cluster in clusterList[0]
//-----------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination 
NABoolean
NAClusterInfo::getSuperNodemap(NAArray<CollIndex>* &clusterList, 
                               NAArray<NAList<CollIndex>*>* &cpuList,
                               Int32 &cpuCount)
{
  cpuCount = 0;
  if(NOT activeClusters_) createActiveClusterList();
  clusterList = new(HEAP) NAArray<CollIndex>(HEAP,activeClusters_->entries());
  for(CollIndex entry =0; entry< activeClusters_->entries();entry++)
  {
    clusterList->insertAt(entry,(*activeClusters_)[entry]);
  }

#ifndef NDEBUG
// LCOV_EXCL_START
  if(getenv("debug_MNO"))
  {
    FILE * ofd = fopen("superNodeMap","ac");
    BUMP_INDENT(DEFAULT_INDENT);
    fprintf(ofd,"%s %s\n",NEW_INDENT,"Active Clusters: ");
    for(CollIndex n=0;n<activeClusters_->entries();n++)
    {
      fprintf(ofd, "%s %d \n", NEW_INDENT, (*activeClusters_)[n]); 
    }
    fprintf(ofd,"*********************************************************************\n");
    fprintf(ofd,"%s %s\n",    NEW_INDENT,"Active Cluster and its CPUs");
    fclose(ofd);
  }
// LCOV_EXCL_STOP
#endif

  cpuList= new(HEAP) NAArray<NAList<CollIndex>*> (HEAP,activeClusters_->entries());
  maps * cpuForCluster=NULL;

  for(CollIndex index = 0;index<activeClusters_->entries();index++)
  {
    cpuForCluster = (maps*)(clusterToCPUMap_->getFirstValue(&(*activeClusters_)[index]));
#pragma warning (disable : 4244)   //warning elimination
    if (NOT cpuForCluster)
    {
      getProcessorStatus(cpuForCluster,(*activeClusters_)[index]);
    }
#pragma warning (default : 4244)   //warning elimination
  
    NABoolean aggreNodeOnly = 
        CmpCommon::getDefault(ESP_ON_AGGREGATION_NODES_ONLY) == DF_ON;

    NAList<CollIndex>*  cList = cpuForCluster->getCpuList(aggreNodeOnly); 

    NAList<CollIndex> * ptrCpuForCluster = new(HEAP) NAList<CollIndex>(*cList,HEAP);
    cpuCount += cList->entries();

#ifndef NDEBUG
// LCOV_EXCL_START
  if(getenv("debug_MNO"))
  {
    FILE * ofd = fopen("superNodeMap","ac");
    BUMP_INDENT(DEFAULT_INDENT);
    fprintf(ofd,"%s %s %2d\n",NEW_INDENT,
                           "Active cluster  ",(*activeClusters_)[index]);
    fprintf(ofd,"%s %s", NEW_INDENT, "CPUS:  ");
    for(CollIndex m=0;m<ptrCpuForCluster->entries();m++)
    {
      fprintf(ofd,"%d %s",(*ptrCpuForCluster)[m],"  ");
    }
    fprintf(ofd,"\n");
    fclose(ofd);
  }
// LCOV_EXCL_STOP
#endif

    cpuList->insertAt(index,ptrCpuForCluster);
  }

#ifndef NDEBUG
// LCOV_EXCL_START
if(getenv("debug_MNO"))
{
  FILE * ofd = fopen("superNodeMap","ac");
  fprintf(ofd,"*********************************************************************\n");
  fclose(ofd);
}
// LCOV_EXCL_STOP
#endif

  return TRUE;

}

// Returns total number of CPUs (including down CPUs)
Lng32 NAClusterInfo::getTotalNumberOfCPUs()
{
  Lng32 cpuCount = 0;
  if (NOT activeClusters_) createActiveClusterList();

  for(CollIndex index = 0;index<activeClusters_->entries();index++)
  {
    maps *cpuForCluster = (maps*)
      (clusterToCPUMap_->getFirstValue(&(*activeClusters_)[index]));
    if (cpuForCluster && cpuForCluster->list)
      cpuCount += cpuForCluster->list->entries();
  }

#ifndef NDEBUG
// LCOV_EXCL_START
  if ( inTestMode() ) {
    NADefaults & defs = ActiveSchemaDB()->getDefaults();
    cpuCount = (Int32)(defs.getAsLong(POS_TEST_NUM_NODES));
  }
// LCOV_EXCL_STOP
#endif

  return cpuCount;
}

// setMaxOSV should be called for all NATable in the current Statement
// before the versioning check.
void NAClusterInfo::setMaxOSV(QualifiedName &qualName, COM_VERSION osv)
{
  if((maxOSV_ < osv) OR
    (maxOSV_ == COM_VERS_UNKNOWN))
  {
    maxOSV_ = osv;
    maxOSVName_ = qualName;
  }
}

#pragma nowarn(161)   // warning elimination 
void NAClusterInfo::cleanupPerStatement()
{
  //After every statement activeClusters_ should be NULL 
  // because statement heap has been cleared already. 
  activeClusters_ = NULL;
  // reset the mebers for versioning support
  maxOSV_ = COM_VERS_UNKNOWN;
}
#pragma warn(161)  // warning elimination 

void NAClusterInfo::initializeForOSIMCapture()
{
  UInt32 i=0;
  // clear out clusterToCPUMap_;
  if (clusterToCPUMap_)
  {
      CollIndex * clusterNum;
      maps * cpuMap;
      NAHashDictionaryIterator<CollIndex,maps> clusterToCPUMapIter
                                             (*clusterToCPUMap_);
      for (i=0; i<clusterToCPUMapIter.entries(); i++)
      {
          clusterToCPUMapIter.getNext(clusterNum,cpuMap);

          // only delete entries from other clusters
          if(*clusterNum != (CollIndex)localCluster_)
          {
            // On Linux, there is only one cluster. The following code will not be exercised. 
            // LCOV_EXCL_START
            clusterToCPUMap_->remove(clusterNum);
            NADELETEBASIC(clusterNum,heap_);
            delete cpuMap;
            // LCOV_EXCL_STOP
          }
      }//for
   }
}

NAClusterInfoLinux::NAClusterInfoLinux(CollHeap * heap) : NAClusterInfo(heap)
, numTSEs_(0), tseInfo_(NULL), nid_(0), pid_(0)
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
   NADELETEBASIC(tseInfo_, heap_);
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

Int32 compareTSEs( const void* a, const void* b ) 
{  
  // compare function
  MS_Mon_Process_Info_Type* arg1 = (MS_Mon_Process_Info_Type*) a;
  MS_Mon_Process_Info_Type* arg2 = (MS_Mon_Process_Info_Type*) b;

  if ( arg1->nid < arg2->nid )
    return -1;
  else  {
    if( arg1->nid == arg2->nid )
      return strcmp(arg1->process_name, arg2->process_name);
    else
     return 1;
  }
}

// setup TSE info for the POS. The method collects all TSEs in the cluster,
// filter out $SYSTEM, and sort the array in assending order on nid (node id).
// The method also can fake the number of TSEs when operated under POS test
// mode (cqd POS_TEST_MODE 'on'). In this special mode, the # of TSTs are 
// cqd POS_TEST_NUM_NODES times cqd POS_TEST_NUM_VOLUMES_PER_NODE.
// All faked TSEs are named numerically from 1 to # of TSTs. The test mode
// is for testing the disk_pool sub-feature.
void NAClusterInfoLinux::setupTSEinfoForPOS()
{
   if ( tseInfo_ ) {
      // LCOV_EXCL_START
      NADELETEBASIC(tseInfo_, heap_);
      tseInfo_ = NULL; numTSEs_ = 0;
      // LCOV_EXCL_STOP
   }

   short ret_val = msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                             &numTSEs_,
                                             0,  // max ignored if info is NULL
                                             NULL);

   if ( ret_val != 0 ) 
     return;

   tseInfo_ = new (heap_) MS_Mon_Process_Info_Type [numTSEs_];

   ret_val = msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                             &numTSEs_,
                                             numTSEs_,
                                             tseInfo_);


   if ( ret_val != 0 ) {
      // LCOV_EXCL_START
      NADELETEBASIC(tseInfo_, heap_);
      tseInfo_ = NULL; numTSEs_ = 0;
      return;
      // LCOV_EXCL_STOP
   }

   pid_ = getpid();

   for (Lng32 i= 0; i< numTSEs_; i++) {

     if ( tseInfo_[i].pid == pid_ )
       nid_ = tseInfo_[i].nid;

     // NOTE: The system metadata may be located in a volume other than
     //  $SYSTEM.  The following could change.  For now, skip any volumes
     //  called $SYSTEM.  The audit volumes aren't returned from the
     //  when MS_ProcessType_TSE is passed to msg_mon_get_process_info_type().
     //  Can add code here to filter out other TSEs if needed

     // here we replace a backup DP2 process or $SYSTEM process with the last
     // entry in the array in the hope that it is a good one.
     if (tseInfo_[i].backup != 0 ||
         strncmp(tseInfo_[i].process_name, "$SYSTEM", 7) == 0 )
     {
        if ( i < numTSEs_ - 1 ) {
          tseInfo_[i] = tseInfo_[numTSEs_-1]; // replace it with the
                                              // last entry from the array
          i--; // the previous last entry should be checked aginst backup and $system
               // because of process pairs
        }
        numTSEs_--;
     }
   }
  
#ifndef NDEBUG
   // LCOV_EXCL_START
   if (ActiveSchemaDB() && CmpCommon::getDefault(POS_TEST_MODE) == DF_ON) {
      NADefaults & defs = ActiveSchemaDB()->getDefaults();
      Int32 num_faked_nodes = (Int32)(defs.getAsLong(POS_TEST_NUM_NODES));
      Int32 num_faked_tses_per_node = 
            (Int32)(defs.getAsLong(POS_TEST_NUM_VOLUMES_PER_NODE));

      Int32 tses = 0;
      if (num_faked_nodes * num_faked_tses_per_node <= MAX_NUM_TSES) {
        for (Int32 i=0; i<num_faked_nodes; i++) {
           for (Int32 j=0; j<num_faked_tses_per_node; j++) {
              tseInfo_[tses].nid = i;

              char buf[20]; str_itoa(tses, buf);
              strcpy(tseInfo_[tses].process_name, buf);

              tses++;
           }
        }
        numTSEs_ = tses;
      }
   }
   // LCOV_EXCL_STOP
#endif
     
   qsort(tseInfo_, numTSEs_, sizeof(MS_Mon_Process_Info_Type), compareTSEs);
}


// get the TSE info for the kth volume (in the sorted order).
MS_Mon_Process_Info_Type* NAClusterInfoLinux::getTSEInfoForPOS(Int32 k)
{
   if ( tseInfo_ == NULL ) 
     setupTSEinfoForPOS();

   if ( k >= 0 && k < numTSEs_ )
     return &tseInfo_[k];
   else 
     return 0;
}

// get the total # of TSEs
Int32 NAClusterInfoLinux::numTSEsForPOS() 
{ 
   if ( tseInfo_ == NULL )
     setupTSEinfoForPOS();

   return numTSEs_; 
}


// LCOV_EXCL_START
NABoolean NAClusterInfo::IsRemoteNodeDown(short error)
{
  if ((error == FileSystemErrorRemoteNodeDown ) ||
      (error == FileSystemErrorRemoteNodeUnavailable) ||
      (error == FileSystemErrorNamedProcessNotInDCT) )
    return TRUE;
  else
    return FALSE;
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
const char * NAClusterInfo::GetNodeName(const char *dp2Name, char *buffer, Int32 size)
{
  strncpy(buffer, dp2Name, size);

  char *pos = buffer;

  Int32 i;
  for(i=0; i<size; i++){
    if(pos[i] == '.'){
      pos[i] = 0;
      break;
    }
  }

  DCMPASSERT(i<size);

  return buffer;
}
// LCOV_EXCL_STOP

NAList<CollIndex>*  maps::getCpuList(NABoolean aggregationNodeOnly)
{
   return (aggregationNodeOnly) ? listOfAggregationOnlyNodes : list;
}

Int32 maps::getCpuCount(NABoolean aggregationNodeOnly)
{
   return (aggregationNodeOnly) ? listOfAggregationOnlyNodes->entries() : 
                                  list->entries();
}


