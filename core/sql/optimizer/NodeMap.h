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
//==============================================================================
//  A node map entry associates a partition with a node in the cluster.  A node
// map is an array of node map entries.
//==============================================================================
#ifndef __NODE_MAP_H
#define __NODE_MAP_H

#include "NAClusterInfo.h"
#include "NABoolean.h"
#include "CmpCommon.h"
#include "opt.h"

//-----------------------------
// Classes defined in this file
//-----------------------------
class NodeMapEntry;
class NodeMap;

//-----------------------------
// Forward references
//-----------------------------
class Generator;
class PartitioningFunction;
class HivePartitionAndBucketKey;
class HHDFSStatsBase;

//----------------------------------------------------------------
//  Needed for passing CollIndex pointers as reference parameters.
//----------------------------------------------------------------
typedef CollIndex* CollIndexPointer;
typedef LIST(const char*) DP2VolumeNamesContainer;

//-----------------------------------------
//  Indication that any node is acceptable.
//-----------------------------------------
const Lng32 ANY_NODE = -1;
const Lng32 LOCAL_CLUSTER = 0;

//--------------------------------------------
//  A node map entry associates a process with
// its SMP node and its cluster.
//--------------------------------------------
class NodeMapEntry : public NABasicObject {

public:

  enum PartitionState {ACTIVE, IDLE, NOT_ACTIVE, ACTIVE_NO_DATA, UNKNOWN};

  //----------------------------
  // Constructors and destructor
  //----------------------------
  NodeMapEntry(char* fullName, char* givenName, CollHeap* heap = 0, Int32 tableIdent =0, NABoolean noService=FALSE);

  NodeMapEntry(PartitionState state = ACTIVE)
  : dp2Name_       (0),
    partitionName_ (0),
    heap_          (0),
    nodeNumber_    (ANY_NODE),
    clusterNumber_ (LOCAL_CLUSTER),
    partitionState_(state),
    givenName_(0)
  {}

  NodeMapEntry(const NodeMapEntry& other, CollHeap* heap = 0);

  virtual ~NodeMapEntry()           { NADELETEBASIC(dp2Name_,heap_);
                                      NADELETEBASIC(partitionName_,heap_);
				      NADELETEBASIC(givenName_,heap_);
                                    }

  //-------------------------------
  // Overloaded assignment operator
  //-------------------------------
  NodeMapEntry& operator=(const NodeMapEntry& other);

  //-------------------
  // Accessor functions
  //-------------------
  virtual const char* getDP2Name()       const { return dp2Name_;       }
  virtual const char* getPartitionName() const { return partitionName_; }
  virtual const char* getGivenName()	 const { return givenName_; }
  virtual Int32         getNodeNumber()    const { return nodeNumber_;    }
  virtual Int32         getClusterNumber() const { return clusterNumber_; }
  virtual NABoolean   isPartitionActive() const
                               { return (partitionState_ == ACTIVE); }
  virtual PartitionState  getPartitionState() const
                               { return partitionState_; }

  //------------------
  // Mutator functions
  //------------------
  virtual void setDp2Name(char* dp2Name);
  virtual void setNodeNumber(Int32 nodeNumber)  {nodeNumber_ = nodeNumber;      }
  virtual void setClusterNumber(Int32 clusterNumber)
                                              {clusterNumber_ = clusterNumber;}
  virtual void setPartitionState(const PartitionState& partState)
                               { partitionState_ = partState; }

  //------------------
  //  Print functions.
  //------------------
  void display() const;

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "NodeMapEntry") const;

  // Generate a string representation of this NodeMapEntry.
  const NAString getText() const;

protected:
  // Pointer to heap in which disk process name above is allocated.  If disk
  // process name above is not allocated, this pointer is null.
  CollHeap* heap_;

  
private:

  // Disk process name associated with this entry and implemented as a
  // simple null terminated character array.  A null pointer value
  // indicates a node map entry for a process other than disk process.
  char* dp2Name_;

  // The full name.  E.g., dp2Name_ might be "$DATA"
  // while this member'll be "\NSK.$DATA.ZSDHH99A.XFT8000".
  //
  // If this member is non-null, then so will dp2Name_ be;
  // however, the converse is not necessarily true --
  // if dp2Name_ is reset (by setDp2Name() or copy ctor or assignment oper),
  // then we lose the full partn name info by deleting and NULLifying it.
  //
  // Useful in Binder -- see containsPartition() method
  // in NodeMap::, NAFileSet::, NATable::.
  char* partitionName_;

  // Integer value identifying SMP node associated with this entry.
  Int32 nodeNumber_;

  // Integer value identifying cluster associated with this entry.
  Int32 clusterNumber_;

  // Enum that indicates the state of the partitions:
  // (See AP doc for more details)
  // NOT_ACTIVE    : the partition won't receive requests from the
  //                 PA, thus it won't do ANY work
  // IDLE          : The partition receives at least one request from
  //                 the PA but it does not read nor return any data
  // ACTIVE:       : The partition receives at least one request from
  //                 the PA and it reads and returns data
  // ACTIVE_NO_DATA: The partition receives at least one request from
  //                 the PA and it reads date but it does not
  //                 return any data to its parent (this happens
  //                 because the executor preds. block all data)
  // UNKNOWN       : We don't know the state of the partition because
  //                 of limitations in the active partition detection
  //                 algorithm.
  //--------------------------------------------------------------------
  // We assume all partitions are active, later we may change
  // the state
  PartitionState partitionState_;

  // holds the name given to this partition by the PARTITION NAME clause
  // used to restrict statement to a specified list of partitions.
  char* givenName_;

}; // NodeMapEntry
//<pb>


class HHDFSFileStats;

struct HiveScanInfo
{
   void print(FILE* ofd, const char* indent, const char* title) const;

  HiveScanInfo(HHDFSFileStats* file=NULL, Int64 off=0, Int64 span=0, NABoolean loc=FALSE)
     : file_(file), offset_(off), span_(span), isLocal_(loc) {}
     
   HHDFSFileStats* file_;
   Int64 offset_;
   Int64 span_;
   NABoolean isLocal_;
};
   
class HiveNodeMapEntry : public NodeMapEntry {

public:

   HiveNodeMapEntry(PartitionState state = ACTIVE, CollHeap* heap=0)
    : NodeMapEntry(state), scanInfo_(heap, 0) {}

   HiveNodeMapEntry(const HiveNodeMapEntry&, CollHeap* heap=0);

   ~HiveNodeMapEntry() {}

   void addScanInfo(HiveScanInfo info) 
     { scanInfo_.insertAt(scanInfo_.entries(), info); }

   HiveNodeMapEntry& operator=(const HiveNodeMapEntry& other);


   void print(FILE* ofd, const char* indent, const char* title) const;
   
   LIST(HiveScanInfo)& getScanInfo() { return scanInfo_; }

protected:
   LIST(HiveScanInfo) scanInfo_;
};

class HBaseNodeMapEntry : public NodeMapEntry {

public:

   HBaseNodeMapEntry(PartitionState state = ACTIVE, CollHeap* heap=0)
    : NodeMapEntry(state)
    {}

   HBaseNodeMapEntry(const HBaseNodeMapEntry&, CollHeap* heap=0);
   ~HBaseNodeMapEntry() {}

   HBaseNodeMapEntry& operator=(const HBaseNodeMapEntry& other);

   void print(FILE* ofd, const char* indent, const char* title) const;

};

   

//----------------------------------------------
//  A node map encapsulates access to an ordered
// collection of node map entries.
//----------------------------------------------
class NodeMap : public NABasicObject {

public:

  friend class NodeMapIterator;
  enum NodeMapType {SQ=0, HIVE=1, HBASE=2};

  //-----------------------------
  // Constructors and destructor.
  //-----------------------------
  NodeMap (CollHeap* heap, NodeMap::NodeMapType type = SQ)
    : map_(heap,0)
    ,numActivePartitions_(-1)
    ,numEstActivePartitionsAtRuntime_(-1)
    ,numOfDP2Volumes_(-1)
    ,numOfActiveDP2Volumes_(-1)
    ,heap_(heap)
    ,type_(type)
  {}

  NodeMap (CollHeap* heap,
           const CollIndex numEntries,
           const NodeMapEntry::PartitionState state = NodeMapEntry::ACTIVE,
           NodeMap::NodeMapType NodeMapKind = SQ
          );

  NodeMap (const NodeMap& other, CollHeap* heap = 0);

  virtual ~NodeMap();

  NodeMap* copy(CollHeap* heap = 0) const;

  void setToRandCPU(CollIndex x) { setNodeNumber(x, ANY_NODE); }

  inline CollIndex getNumEntries() const { return map_.entries(); }

  NABoolean containsPartition(const char *fullName) const;

  NodeMap* synthesizeLogicalMap (const CollIndex logicalNumEntries,
                                 NABoolean forESP);

  void deriveGrouping (const CollIndex         numGroups,
                             CollIndexPointer& groupStart,
                             CollIndexPointer& groupSize);

  NABoolean allNodesSpecified(void) const;
  NABoolean allNodesAreWildcards() const;

  //--------------------------------
  // Accessor functions.
  //--------------------------------
  const NodeMapEntry* getNodeMapEntry(const CollIndex position)const
  {
    CMPASSERT(map_.getUsage(position) != UNUSED_COLL_ENTRY);
    return map_[position];
  }
  NodeMapEntry* getNodeMapEntry(const CollIndex position)
  {
    CMPASSERT(map_.getUsage(position) != UNUSED_COLL_ENTRY);
    return map_[position];
  }
  NABoolean isActive(const CollIndex position) const;
  Lng32      getNodeNumber(const CollIndex position) const;
  Lng32      getPopularNodeNumber(const CollIndex beginPos, 
                                  const CollIndex endPos) const;
  Lng32      mapNodeNameToNodeNum(const NAString node) const;
  Lng32      getClusterNumber(const CollIndex position) const;
  Int32       getTableIdent(void) const  {return tableIdent_; }
  NABoolean isMultiCluster(CollIndex start, CollIndex end, 
                           NABoolean activeOnly) const;


  NABoolean smooth(Lng32 numNodes) ;

  // These functions should be const but because of the
  // cached values and the fact that mutable is not
  // supported, they are not.
  CollIndex getNumOfDP2Volumes();
  CollIndex getNumActivePartitions();

  CollIndex getEstNumActivePartitionsAtRuntime();

  CollIndex getNumActiveDP2Volumes();

  Int32 getNumberOfUniqueNodes() const;

  NABoolean isCoLocated(const NodeMap*) const;

  //--------------------------------
  // Mutator functions.
  //--------------------------------
  void setNodeMapEntry(const CollIndex     position,
                       const NodeMapEntry& entry,
                       CollHeap*           heap = 0);


  void setPartitionState(const CollIndex& position,
                         const NodeMapEntry::PartitionState& state);

  void setNodeNumber(const CollIndex position, const Lng32 nodeNumber);
  void setClusterNumber(const CollIndex position, const Lng32 clusterNumber);

  void setNumActivePartitions(const CollIndex &numActPart)
  { numActivePartitions_ = (Lng32)numActPart; }

  void setEstNumActivePartitionsAtRuntime(const CollIndex &numActPart)
  { numEstActivePartitionsAtRuntime_ = (Lng32)numActPart; }

  void setTableIdent(Int32 tableIdent) { tableIdent_ = tableIdent; }

  // Does this node map have any remote partitions
  NABoolean hasRemotePartitions() const;

  static NABoolean useLocalityForHiveScanInfo();

  // For Hive tables, assign scan ranges to each partition
  void assignScanInfos(HivePartitionAndBucketKey *hiveSearchKey);

  // balance out the assigned scan ranges to distribute work more evenly
  void balanceScanInfos(HivePartitionAndBucketKey *hiveSearchKey,
                        Int64 totalBytesToRead,
                        Int64 *&espDistribution);

  //------------------
  //  Generator methods
  //------------------
  static short codeGen(const PartitioningFunction *partFunc,
		       const Lng32 numESPs,
		       Generator *generator);

  //------------------
  //  Print functions.
  //------------------
  void display() const;

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "NodeMap") const;

  // Generate a string representation of this NodeMap.
  const NAString getText() const;

  NodeMap::NodeMapType type() const { return type_; }

private:

  NodeMap () ; // *must* specify a CollHeap *
  NodeMap & operator = (const NodeMap &) ; // don't use this fn unless you write it

  CollHeap *heap() { return heap_; }

  void resetCachedValues();

  // An array of pointers to node map entries.
  ARRAY (NodeMapEntry *) map_;

  // The number of partitions doing work for a particular query
  // Value of -1 means the member has not been set.
  Lng32 numActivePartitions_;

  // This variable is set to the maximum number of partitions that can be active,
  // if a query has an equality predicate with a host/parameter variable
  // on leading partition column.
  // Value of -1 means the member has not been set.
  Lng32 numEstActivePartitionsAtRuntime_;

  // The number of DP2 volumes
  // Value of -1 means the member has not been set.
  Lng32 numOfDP2Volumes_;

  // The number of active DP2 volumes
  // Value of -1 means the member has not been set.
  Lng32 numOfActiveDP2Volumes_;

  // Identifier for the table.
  Int32 tableIdent_;


  CollHeap* heap_;

  NodeMap::NodeMapType type_;

}; // NodeMap

class NodeMapIterator {

public:
   NodeMapIterator(NodeMap& x) : nodeMap_(x), idx_(-1) { init(); };
   ~NodeMapIterator() {};

   void init() 
   { 
      if ( nodeMap_.map_.entries() > 0 )
        idx_ = 0;
   };

   NodeMapEntry* getEntry() 
   {
      if ( idx_ < 0 || idx_ >= nodeMap_.map_.entries() ||
           nodeMap_.map_.getUsage(idx_) == UNUSED_COLL_ENTRY )
        return NULL;
  
      return nodeMap_.map_[idx_];
   }

   void advance() 
   {
      if (idx_ == nodeMap_.map_.entries()-1) 
         return;

      do{ 
        idx_++;
      }
      while (idx_ < nodeMap_.map_.entries() &&
             nodeMap_.map_.getUsage(idx_) == UNUSED_COLL_ENTRY); 
   }

   NodeMapEntry* advanceAndGetEntry() 
   { advance(); return getEntry(); }
   

protected:
   CollIndex idx_;
   NodeMap& nodeMap_;
};

#endif





