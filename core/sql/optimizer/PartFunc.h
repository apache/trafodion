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
#ifndef PARTFUNC_H
#define PARTFUNC_H
/* -*-C++-*-
*************************************************************************
*
* File:         PartFunc.h
* Description:  Partitioning Function
* Created:      01/03/96
* Language:     C++
*
*
*
*
*************************************************************************
*/

// -----------------------------------------------------------------------
#include "Int64.h"
#include "Collections.h"
#include "ItemExpr.h"
#include "ItemExprList.h"
#include "RelExpr.h"
#include "NodeMap.h"

// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------
class PartitioningFunction;

class SinglePartitionPartitioningFunction;
class ReplicateViaBroadcastPartitioningFunction;
class ReplicateNoBroadcastPartitioningFunction;
class HashPartitioningFunction;
class TableHashPartitioningFunction;
class HashDistPartitioningFunction;
class Hash2PartitioningFunction;
class RangePartitionBoundaries;
class RangePartitioningFunction;
class LogPhysPartitioningFunction;
class RoundRobinPartitioningFunction;
class HivePartitioningFunction;
class SkewedDataPartitioningFunction;


// ----------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------
class PartitioningRequirement;
class NormWA;
class Generator;
class Attributes;
class NAColumnArray;
class SearchKey;
class SkewedValueList;
typedef LIST(Int64) Int64List;
typedef NABoolean (*compFuncPtrT)(const char* low, const char* key, const char* high, Int32 keyLen, NABoolean checkLast);

// ----------------------------------------------------------------------
// literals for special numbers of partitions (don't care, exactly one)
// ----------------------------------------------------------------------
enum
{
  ANY_NUMBER_OF_PARTITIONS = -1,
  EXACTLY_ONE_PARTITION = 1
};

// ----------------------------------------------------------------------
// literals for partition grouping distribution to use                  
//  (use default, uniform # of physical parts, uniform # of active parts)
// ----------------------------------------------------------------------
enum PartitionGroupingDistEnum
{
  DEFAULT_PARTITION_GROUPING = -1,
  UNIFORM_PHYSICAL_PARTITION_GROUPING = 0,
  UNIFORM_ACTIVE_PARTITION_GROUPING = 1
};


// A class representing skew property
class skewProperty : public NABasicObject {

public:

  // -----------------------------------------------------------------------
  // literals for skew data handling 
  // -----------------------------------------------------------------------
  enum skewDataHandlingEnum
  { ANY,                  // Any skew
    UNIFORM_DISTRIBUTE,   // skewed values are uniformly distributed 
                          // (e.g., through round-robin)
    BROADCAST             // skewed values are broadcasted
  };

  skewProperty(enum skewDataHandlingEnum x = ANY, 
               SkewedValueList* v= NULL,
               Int32 numEsps = -1,
               NAMemory* heap = CmpCommon::statementHeap()
               ): 
      indicator_(x), skewValues_(v), heap_(heap), numESPs_(numEsps),
      broadcastOneRow_(FALSE){};

  skewProperty(const skewProperty& sk): 
      indicator_(sk.indicator_), skewValues_(sk.skewValues_), heap_(sk.heap_),
      numESPs_(sk.numESPs_),
      broadcastOneRow_(sk.broadcastOneRow_) {}

  ~skewProperty() {};

  enum skewDataHandlingEnum getIndicator() const { return indicator_; }; 
  void setIndicator(enum skewDataHandlingEnum x) { indicator_ = x; }; 

  NABoolean getBroadcastOneRow() const { return broadcastOneRow_; }; 
  void setBroadcastOneRow(NABoolean x) { broadcastOneRow_ = x; }; 


  const SkewedValueList* getSkewValues() const { return skewValues_; };
  void setSkewValues(const SkewedValueList* v) { skewValues_ = v; };

  void set(skewProperty& sk) 
  { 
    setIndicator(sk.getIndicator()); 
    setSkewValues(sk.getSkewValues()); 
    setBroadcastOneRow(sk.getBroadcastOneRow());
  };

  NABoolean operator ==(const skewProperty&) const;

  // If abbre. form 
  //   = FALSE: skewed distribution method name and values are returned
  //   = TRUE:  only the abbreviation of the skewed distribution method 
  //            is returned
  const NAString getText(NABoolean inAbbreviatedForm = FALSE) const;

  NABoolean isAnySkew() const { return indicator_ == skewProperty::ANY; };

  NABoolean isUniformDistributed() const { return indicator_ == skewProperty::UNIFORM_DISTRIBUTE; };
  NABoolean isBroadcasted() const { return indicator_ == skewProperty::BROADCAST; };

  NABoolean hasSkewValues() const
  { return skewValues_ AND skewValues_->entries() > 0; };

  Int32 getAntiSkewESPs() const { return numESPs_; };

  NABoolean skewedListHasOnlyNonSkewedNull() const
  {
    return skewValues_->hasOnlyNonSkewedNull();
  }
protected:
  enum skewDataHandlingEnum indicator_; // How the data is skewed
  const SkewedValueList* skewValues_;   // The skewed values. Multiple skew
                                        // property objects can share 
                                        // a single skew value list
  NAMemory * heap_; // the heap    

  NABoolean broadcastOneRow_;

  // the number of ESPs that will deal with skew. -1 means "use all"
  Int32 numESPs_;
};

// Define one useful object: the any-skew object
extern const skewProperty ANY_SKEW_PROPERTY;


// -----------------------------------------------------------------------
// PartitioningFunction
//
// A base class for defining the partitioning characteristics for
// horizontally partitioned data. We envisage its use for hash,
// range or any other built-in (system supported) partitioning schemes.
//
// The partition function specifies
//   1) the number of partitions (>= 1) and
//   2) a partitioning key and
//   3) an expression that can be used for distributing
//      data over the partitions.
//
// -----------------------------------------------------------------------
class PartitioningFunction : public NABasicObject
{
protected:
  // --------------------------------------------------------------------
  // Partitioning function type identifier.
  // It should be visible only to the derived classes.
  // --------------------------------------------------------------------
  enum PartitioningFunctionTypeEnum
    {
      SINGLE_PARTITION_PARTITIONING_FUNCTION,
      REPLICATE_VIA_BROADCAST_PARTITIONING_FUNCTION,
      REPLICATE_NO_BROADCAST_PARTITIONING_FUNCTION,
      HASH_PARTITIONING_FUNCTION,
      HASH_DIST_PARTITIONING_FUNCTION,
      HASH2_PARTITIONING_FUNCTION,
      RANGE_PARTITIONING_FUNCTION,
      LOGPHYS_PARTITIONING_FUNCTION,
      ROUND_ROBIN_PARTITIONING_FUNCTION,
      SKEWEDDATA_PARTITIONING_FUNCTION,
      HIVE_PARTITIONING_FUNCTION
    };

public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  PartitioningFunction(const PartitioningFunctionTypeEnum ftype,
                       NodeMap* nodeMap = 0,
                       NAMemory* heap = CmpCommon::statementHeap())
    : functionType_(ftype),
      partitioningExpression_(NULL),
      dataConversionErrorFlag_(NULL),
      assignPartition_(FALSE),
      partKeyPredsCreated_(FALSE),
      partitionSelectionExpr_(NULL),
      nodeMap_(nodeMap),
      setupForStatement_(FALSE),
      resetAfterStatement_(FALSE),
      restrictedBeginPartNumber_(-1),
      restrictedEndPartNumber_(-1),
      activeStreams_(0.),
      heap_(heap)
  {
  }

  PartitioningFunction(const PartitioningFunctionTypeEnum ftype,
                       const ValueIdSet& partitioningKey,
                       NodeMap* nodeMap= 0,
                       NAMemory* heap = CmpCommon::statementHeap())
    : functionType_(ftype),
      partitioningKeyColumns_(partitioningKey),
      partitioningExpression_(NULL),
      dataConversionErrorFlag_(NULL),
      assignPartition_(FALSE),
      partKeyPredsCreated_(FALSE),
      partitionSelectionExpr_(NULL),
      nodeMap_(nodeMap),
      setupForStatement_(FALSE),
      resetAfterStatement_(FALSE),
      restrictedBeginPartNumber_(-1),
      restrictedEndPartNumber_(-1),
      activeStreams_(0.),
      heap_(heap)
  {
  }

  PartitioningFunction(const PartitioningFunction& other,
                       NAMemory* heap = CmpCommon::statementHeap())
    : functionType_(other.functionType_),
      partitioningKeyColumns_(other.partitioningKeyColumns_),
      partitioningKeyPredicates_(other.partitioningKeyPredicates_),
      partitionInputValues_(other.partitionInputValues_),
      partitionInputValuesLayout_(other.partitionInputValuesLayout_),
      partKeyPredsCreated_(other.partKeyPredsCreated_),
      partitioningExpression_(other.partitioningExpression_),
      dataConversionErrorFlag_(other.dataConversionErrorFlag_),
      assignPartition_(other.assignPartition_),
      partitionSelectionExpr_(other.partitionSelectionExpr_),
      partitionSelectionExprInputs_(other.partitionSelectionExprInputs_),
      nodeMap_((other.nodeMap_) ? other.nodeMap_->copy(heap) : 0),
      setupForStatement_(FALSE),
      resetAfterStatement_(FALSE),
      restrictedBeginPartNumber_(other.restrictedBeginPartNumber_),
      restrictedEndPartNumber_(other.restrictedEndPartNumber_),
      activeStreams_(other.activeStreams_),
      heap_(heap)
  {
  }

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~PartitioningFunction();

  // ---------------------------------------------------------------------
  // Perform type-safe pointer casts.
  // ---------------------------------------------------------------------
  virtual const
  LogPhysPartitioningFunction* castToLogPhysPartitioningFunction() const;

  virtual const
  SinglePartitionPartitioningFunction*
                   castToSinglePartitionPartitioningFunction() const;

  virtual const
  ReplicateViaBroadcastPartitioningFunction*
                   castToReplicateViaBroadcastPartitioningFunction() const;

  virtual const
  ReplicateNoBroadcastPartitioningFunction*
                   castToReplicateNoBroadcastPartitioningFunction() const;

  virtual const
  HashPartitioningFunction* castToHashPartitioningFunction() const;

  virtual const
  TableHashPartitioningFunction* castToTableHashPartitioningFunction() const;

  virtual const
  HashDistPartitioningFunction* castToHashDistPartitioningFunction() const;

  virtual const
  Hash2PartitioningFunction* castToHash2PartitioningFunction() const;

  virtual const
  RangePartitioningFunction* castToRangePartitioningFunction() const;

  virtual const
  RoundRobinPartitioningFunction* castToRoundRobinPartitioningFunction() const;

  virtual const
  SkewedDataPartitioningFunction* castToSkewedDataPartitioningFunction() const;

  virtual const
  HivePartitioningFunction* castToHivePartitioningFunction() const;

  // ---------------------------------------------------------------------
  // Accessor method for the partitioning key.
  // ---------------------------------------------------------------------
  const ValueIdSet& getPartitioningKey() const
                                       { return partitioningKeyColumns_; }

  // ---------------------------------------------------------------------
  // Accessor method for the partial partitioning key. A partial key 
  // describes part of the data. In default case, the partial key is the 
  // full partitioning key. This is not true for SkewedDataPartioningFunction.
  // ---------------------------------------------------------------------
  virtual const ValueIdSet& getPartialPartitioningKey() const 
  { return getPartitioningKey(); }

  // ---------------------------------------------------------------------
  // Accessor method for the number of partitions.
  // ---------------------------------------------------------------------
  virtual Lng32 getCountOfPartitions() const;

  // --------------------------------------------------------------------
  // Method used for run-time type identification.
  // --------------------------------------------------------------------
  PartitioningFunctionTypeEnum getPartitioningFunctionType() const
                                                { return functionType_; }

  // ---------------------------------------------------------------------
  // Partitioning Function Type Tests
  // ---------------------------------------------------------------------
  NABoolean isPartitioned() const
  { 
    return (getCountOfPartitions() > EXACTLY_ONE_PARTITION); 
  }

  inline NABoolean isASinglePartitionPartitioningFunction() const
    { return (functionType_ == SINGLE_PARTITION_PARTITIONING_FUNCTION); }
  inline NABoolean isAReplicateViaBroadcastPartitioningFunction() const
    { return (functionType_ == REPLICATE_VIA_BROADCAST_PARTITIONING_FUNCTION); }
  inline NABoolean isAReplicateNoBroadcastPartitioningFunction() const
    { return (functionType_ == REPLICATE_NO_BROADCAST_PARTITIONING_FUNCTION); }
  inline NABoolean isAReplicationPartitioningFunction() const
    { return (isAReplicateViaBroadcastPartitioningFunction() OR
              isAReplicateNoBroadcastPartitioningFunction()); }
  inline NABoolean isAHashPartitioningFunction() const
                { return (functionType_ == HASH_PARTITIONING_FUNCTION); }
  inline NABoolean isATableHashPartitioningFunction() const
                { return ((functionType_ == HASH_DIST_PARTITIONING_FUNCTION) ||
                          (functionType_ == HASH2_PARTITIONING_FUNCTION)); }
  inline NABoolean isAHashDistPartitioningFunction() const
                { return (functionType_ == HASH_DIST_PARTITIONING_FUNCTION); }
  inline NABoolean isAHash2PartitioningFunction() const
                { return (functionType_ == HASH2_PARTITIONING_FUNCTION); }
  inline NABoolean isARangePartitioningFunction() const
               { return (functionType_ == RANGE_PARTITIONING_FUNCTION); }
  inline NABoolean isALogPhysPartitioningFunction() const
               { return (functionType_ == LOGPHYS_PARTITIONING_FUNCTION); }
  inline NABoolean isARoundRobinPartitioningFunction() const
         { return (functionType_ == ROUND_ROBIN_PARTITIONING_FUNCTION); }
  inline NABoolean isASkewedDataPartitioningFunction() const
                { return (functionType_ == SKEWEDDATA_PARTITIONING_FUNCTION); }
  inline NABoolean isAHivePartitioningFunction() const
                { return (functionType_ == HIVE_PARTITIONING_FUNCTION); }
  inline NABoolean isARandomPartitioningFunction() const
    { return isAHash2PartitioningFunction() &&
        partitioningKeyColumns_.hasRandom(); }

  // ---------------------------------------------------------------------
  // Method to test if the partitioning key contains any approximate
  // numeric type columns. Necessary because in some cases certain
  // parallel operations do not function properly if the partitioning
  // key of the table contains approximate numeric columns.
  // ---------------------------------------------------------------------
  NABoolean partKeyContainsFloatColumn() const;

  // ---------------------------------------------------------------------
  // Accessor function for retrieving entries from node map.
  // ---------------------------------------------------------------------
  inline const NodeMapEntry* getNodeMapEntry(CollIndex position) const
                           { return nodeMap_->getNodeMapEntry(position); }

  // ---------------------------------------------------------------------
  // Retrieve a pointer to partitioning function's node map.
  // ---------------------------------------------------------------------
  virtual const NodeMap* getNodeMap() const;

  // use any existing nodemap from my req or my child (or synthesize one) that 
  // matches my partition count requirement
  void useNodeMapFromReqOrChild(PartitioningRequirement *req, 
                                PartitioningFunction *childPF,
                                NABoolean forESP);

  // ---------------------------------------------------------------------
  // Replace existing node map with a specified node map.
  // ---------------------------------------------------------------------
  virtual void replaceNodeMap(NodeMap* nodeMap);

  // --------------------------------------------------------------------
  // A method for copying the partitioning function.
  // --------------------------------------------------------------------
  virtual PartitioningFunction* copy() const;

  // --------------------------------------------------------------------
  // Rewrite the partitioning keys of the partitioning function in
  // terms of the VEGReference for the VEG to which the partitioning
  // key column belongs.
  // --------------------------------------------------------------------
  virtual void normalizePartitioningKeys(NormWA& normWARef);

  // --------------------------------------------------------------------
  // Each partitioning function constructs a set of partitioning key
  // predicates. They are used for restricting accesses to a specific
  // partition or to a specific set of partitions.  This method is
  // used by the optimizer.
  //
  // Some partitioning functions can not create partitioning key
  // predicates (they're either not smart enough or it is impossible
  // to do so, e.g. because the partitioning key is not available to
  // them).
  // --------------------------------------------------------------------
  virtual NABoolean canProducePartitioningKeyPredicates() const;
  const ValueIdSet& getPartitioningKeyPredicates() const;

  // ----
  // The partition input values are the "variables" that appear
  // in a partitioning key predicate. They are used for identifying
  // the specific partition that is defined by the key. They are
  // created for constructing the partitioning key predicates.
  // ----
  const ValueIdSet& getPartitionInputValues() const;

  // ----
  // Method for obtaining the layout of the partition input values
  // in the buffers that are allocated for the partitioning key.
  // It is used by the code generator.
  // ----
  const ValueIdList& getPartitionInputValuesLayout() const;

  // Create the above expressions (partitioning key predicates and
  // partition input values including layout) for a non-const object
  virtual void createPartitioningKeyPredicates();

  // Replace the pivs, partitioning key predicates and partitioning 
  // expression with those passed in.
  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  // Compare two part funcs on push-down compatibility
  virtual NABoolean 
     partFuncAndFuncPushDownCompatible(const PartitioningFunction&) const 
   { return FALSE;};

  // Test whether it is necessary to check two search keys specify
  // the same partition. 
  virtual NABoolean checkSamePartitionNeeded() const { return TRUE; };

  // --------------------------------------------------------------------
  // A method that is used by optimizer for comparing partitioning 
  // function with the random number partitioning function i.e. it only 
  // compares the partitioning function type and number of partitions. 
  // It does not compare partitioning key.
  // --------------------------------------------------------------------
  virtual COMPARE_RESULT comparePartFuncsForUnion
                              (const PartitioningFunction &other) const;

  NABoolean isKnownReplicaPartFunc() const;

  // --------------------------------------------------------------------
  // A method that is used for comparing two partitioning functions
  // by the optimizer.
  // --------------------------------------------------------------------
  virtual COMPARE_RESULT comparePartFuncToFunc
                              (const PartitioningFunction &other) const;

  // ---------------------------------------------------------------------
  // Check whether one partitioning function is a grouping of another.
  // A grouping of a partitioning function can be created by combining
  // two of its partitions zero or more times. The combined partitions
  // do not have to be adjacent.
  // ---------------------------------------------------------------------
  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  // ---------------------------------------------------------------------
  // Transform a partitioning function into a partitioning requirement
  // ---------------------------------------------------------------------
  virtual PartitioningRequirement* makePartitioningRequirement();

  // --------------------------------------------------------------------
  // Change the number of partitions in the partitioning function, if
  // possible. Because of limitations in the code to do this, the
  // new number of partitions is merely a suggestion, and the actual
  // number used is returned as the result value.
  // --------------------------------------------------------------------
  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist = 
                            DEFAULT_PARTITION_GROUPING);

  // --------------------------------------------------------------------
  // Copy this partitioning function and rewrite the copy in terms of
  // the top or bottom values that are contained in the map.
  // If the parameter mapItUp is set to TRUE, then the partitioning
  // function is rewritten in terms of the values in the top map.
  // Otherwise, it is rewritten in terms of values in the bottom map.
  // The virtual function remapIt() implements the remapping.
  // --------------------------------------------------------------------
  virtual PartitioningFunction* copyAndRemap
                           (ValueIdMap& map, NABoolean mapItUp) const;

  virtual void remapIt(const PartitioningFunction* opf,
                       ValueIdMap& map, NABoolean mapItUp);

  // --------------------------------------------------------------------
  // A method for creating a partitioning function for the IndexDesc
  // based on the partitioning function for the NAFileSet
  // --------------------------------------------------------------------
  virtual PartitioningFunction* createPartitioningFunctionForIndexDesc
                                   (IndexDesc *idesc) const;

  // --------------------------------------------------------------------
  // Constructor for the partitioning expression.
  // It is a bound ItemExpr tree, i.e, each operator has a ValueId
  // and a NAType assigned to it.
  //
  // This method is used by the code generator.
  //
  // Notionally, the partitioning expression would have the following
  // prototype in the C language:
  //   int getPartitionNumber(<param1>, <param2>, ..., <paramN>)
  // The role of the partitioning expression is to receive zero or more
  // values as parametric inputs and return an index that denotes the
  // data stream/partition to which the row of interest should belong.
  // The index is usually called the "partition number". The values
  // that it receives as parametric inputs are called the
  // "partitioning key values". Incidentally, getPartitioningKey()
  // returns precisely those expressions that are evaluated at
  // run-time for computing the partitioning key values.
  // --------------------------------------------------------------------
  virtual ItemExpr* createPartitioningExpression();
  ItemExpr *getPartitioningExpression() const
                                      { return partitioningExpression_; }

  // Create the partition selection expression.  'Partition selection'
  // means that an expression is used to determine the partition to
  // access as opposed to using the File System to determine the range
  // of partitions to access based on a set of partitioning key
  // predicates.  'Partition assignment' is a form of partition
  // selection in which the expression calculates the partition into
  // which a row is inserted.  Partition assignment is currently only
  // used for Round Robin partitioning.  Partition selection is
  // currently used for Hash Dist and Round Robin Partitioning. And
  // the File System is used for Range Partitioning.  The default
  // implementation of this method returns NULL, meaning no partition
  // selection expression can be created.  If a partitioning selection
  // expression is created, it is cached in the data member
  // 'partitionSelectionExpr_' and the partition selection inputs are
  // generated and stored in 'partitionSelectionExprInputs_'. This
  // method is redefined for HashDistPartitioningFunction and
  // RoundRobinPartitioningFunction.
  // 
  virtual ItemExpr *
  createPartitionSelectionExpr(const SearchKey *partSearchKey,
                               const ValueIdSet &availableValues) 
  { return NULL; };

  // Return the cached partition selection expression. See
  // createPartitionSelectionExpr() above.
  //
  ItemExpr *partitionSelectionExpr() const { return partitionSelectionExpr_;};
  ItemExpr * &partitionSelectionExpr() { return partitionSelectionExpr_;};

  // The PartitionAccess::codeGen() calls createPartSelectionExprFromSearchKey
  // to create a partition selection expression based on the search key
  // expressions.  Base classes, such as Hash2PartitioningFunction,
  // may have a more complicated expression than the default case which
  // simply copies the search key expressions into the begin and end
  // partition selection expressions.
  virtual void
  createPartSelectionExprFromSearchKey(const ValueId beginPartSelId,
                                       const ValueId endPartSelId,
                                       ValueIdList &partSelectionValIds) const;

  // partitionSelectionExprInputs(): A list of inputs to the partition
  // selection expression.  This list is populated when the partition
  // selection expression is created.  The partition selection
  // expression needs two inputs: the partition number and the total
  // number of partitions.  These are set up as internal host
  // variables when the partition selection expression is created. The
  // inputs should be layed out as follows:
  //
  //   |-----------------------------------------|
  //   | partition number | number of partitions |
  //   | (4 byte integer) | (4 byte integer)     |
  //   |-----------------------------------------|
  //
  // These variables are actually both inputs and the output of the
  // partition selection function.  Round Robin partitioning uses
  // both fields as inputs and produces its output in the first
  // field.  HashDist partitioning uses the second field as an
  // input and produces its output in the first field.  It is
  // inportant that the variables be layed out in this order, because
  // when the partition selection expressions are generated, the
  // resulting integer must map to the same location as the 'partition
  // number' field.  This happens (luckily or by design) because they
  // are both the first values in their respective lists.
  //
  const ValueIdList &partitionSelectionExprInputs() const
  { return partitionSelectionExprInputs_;};

  ValueIdList &partitionSelectionExprInputs()
  { return partitionSelectionExprInputs_;};

  // ---------------------------------------------------------------------
  // True if partition assignment needs to be done when inserting
  // rows.  'Partition assignment' is a form of partition selection in
  // which the expression calculates the partition into which a row is
  // inserted.  Partition assignment is currently only used for Round
  // Robin partitioning. This flag is set to TRUE (during binding)
  // when inserting into a Round Robin partitioned table.  This will
  // cause the partitioning function to generate a partition selection
  // expression which does the proper calculation.  This flag is
  // initialized to false by all constructors.
  // ---------------------------------------------------------------------
  inline NABoolean assignPartition() const   { return assignPartition_; }
  
  // set the 'assignPartition' indicator to the specified value.
  //
  inline void setAssignPartition(NABoolean assignPartition)
                                  { assignPartition_ = assignPartition; }


  inline CostScalar getActiveStreams() const   { return activeStreams_; }
  inline void setActiveStreams(CostScalar streams)
                                { activeStreams_ = streams; }

  // This method indicates if the partitioning function uses the File
  // System to determine the range of partitions to access.  If the
  // method returns TRUE, the File System is used. If it returns
  // FALSE, then a partition selection expression is used to determine
  // the range.  The default implementation of this method returns
  // FALSE (use partition selection expression).  Currently, only the
  // range partitioning function redefines this method to return TRUE.
  //
  virtual NABoolean usesFSForPartitionSelection() const { return FALSE; }

  // --------------------------------------------------------------------
  // Rewrite the partitioning keys of the partitioning function that
  // are expressed using VEGReferences in terms of the available values.
  // --------------------------------------------------------------------
  virtual void preCodeGen(const ValueIdSet& availableValues);

  // --------------------------------------------------------------------
  // Generate an equivalent executor structure.
  // --------------------------------------------------------------------
  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  // --------------------------------------------------------------------
  // Assign offsets to partition input values that are sent in a tuple.
  // This unusual method is used because the layout gets determined
  // in two different places (the sender and the receiver) and it must
  // match. Also, range partitioning requires a non-standard alignment.
  // --------------------------------------------------------------------
  virtual void generatePivLayout(Generator *generator,
				 Lng32 &partitionInputDataLength,
				 Lng32 atp,
				 Lng32 atpIndex,
				 Attributes ***pivAttrs);

  // Make a new partSearchKey with the partitioning key preds of
  // the partitioning function, if there are any. Note that ignoring
  // the part key preds will result in a wrong answer if we use
  // PA_PARTITION_GROUPING, since the PA node is the node responsible
  // for the grouping. If it doesn't select a subgroup of partitions,
  // too much data may be returned. For now we only consider a
  // search key for the PA node, MDAM to be implemented later.
  // MDAM will be useful for combining user-specified part key preds
  // with logicalPartFunc->getPartitioningKeyPredicates().
  virtual SearchKey *createSearchKey(const IndexDesc *indexDesc,
                                     ValueIdSet availInputs,
                                     ValueIdSet additionalPreds) const
  {return NULL;};

  virtual NABoolean shouldUseSynchronousAccess(
    const ReqdPhysicalProperty* rpp,
    const EstLogPropSharedPtr& inputLogProp,
    GroupAttributes* ga) const;
 
  ItemExpr* getConvErrorExpr() const { return dataConversionErrorFlag_; }

  // if begin or endPartNumber_ is specified, then the range selected
  // by this partitioning function is restricted to the specified range
  // when partitions are selected at runtime.
  // Used when users specify a begin/end (to/from) partition number
  // range in the table name specification.
  NABoolean partitionRangeRestricted() const
  {
    return ((restrictedBeginPartNumber_ > 0) || 
	    (restrictedEndPartNumber_ > 0));
  }
  
  Lng32 getRestrictedBeginPartNumber() const { return restrictedBeginPartNumber_; }
  Lng32 getRestrictedEndPartNumber()   const { return restrictedEndPartNumber_; }
  void setRestrictedBeginPartNumber(Lng32 v) { restrictedBeginPartNumber_ = v; }
  void setRestrictedEndPartNumber(Lng32 v)   { restrictedEndPartNumber_ = v; }

  // ---------------------------------------------------------------------
  // Print and get a short descriptive text
  // ---------------------------------------------------------------------
  virtual const NAString getText() const;
  virtual void print(FILE* ofd = stdout,
                     const char* indent = DEFAULT_INDENT,
                     const char* title = "PartitioningFunction") const;

  void display() const;

  virtual void setupForStatement();
  virtual void resetAfterStatement();

  // Does this partitioning function refer to any remote partitions
  inline NABoolean hasRemotePartitions() const {
    return (getNodeMap()
            && getNodeMap()->hasRemotePartitions());
  }

  virtual NABoolean canHandleSkew() const { return FALSE; };

  virtual UInt32 computeHashValue(char* data, UInt32 flags, Int32 len)
  { return 0; };

  virtual ItemExpr *getHashingExpression() const 
  { return NULL; };

  // A virtual method returning a compiler-time hashing expression that
  // hashes a skew value into a hash value. Called during codeGen phase.
  virtual ItemExpr *buildHashingExpressionForExpr(ItemExpr*) const 
  { return NULL; }

  inline void hasNoPartitioningKeyPredicates()
                                         { partKeyPredsCreated_ = TRUE; }
protected:

  // --------------------------------------------------------------------
  // Modify key, key predicates and partition input values
  // --------------------------------------------------------------------
  inline void setPartKey(const ValueIdSet &key)
                                       { partitioningKeyColumns_ = key; }
  inline const ValueIdSet & partitioningKeyPredicates()
                                   { return partitioningKeyPredicates_; }
  inline void storePartitioningKeyPredicates(const ValueIdSet& partKeyPreds)
                           { partitioningKeyPredicates_ = partKeyPreds;
			     partKeyPredsCreated_ = TRUE; }
// warning elimination (removed "inline")
  void storePartitionInputValues(const ValueIdList& partInputValues)
                       { partitionInputValues_ = partInputValues;
                         partitionInputValuesLayout_ = partInputValues; }
  inline NABoolean partKeyPredsCreated() const { return partKeyPredsCreated_; }


  // --------------------------------------------------------------------
  // The get() and store() primitives permit each derived class to store
  // and retrieve the expression after it is created.
  // --------------------------------------------------------------------
  ItemExpr* getExpression() const     { return partitioningExpression_; }
  void storeExpression(ItemExpr* partExpr)
                                  { partitioningExpression_ = partExpr; }
  void storeConvErrorExpr(ItemExpr* convErrExpr)
                              { dataConversionErrorFlag_ = convErrExpr; }

  //heap
  NAMemory * heap_;

  // A helper function to create a cast expression casting the input 
  // expresssion iv to otype. Also create the data conversion error expression
  // dataConversionErrorFlag_ if it is not null.
  ItemExpr* getCastedItemExpre(ItemExpr* iv, const NAType& otype, CollHeap*) ;

  // Helper function to create a simple partitioning key predicate of the form
  // <partNum> between <piv1> and <piv2>

  // Can be called a first time with partNumExpr = NULL (e.g. in the
  // binder) and then a second time with something like _SALT_
  // specified for partNumExpr (e.g. in preCodeGen).
  void createBetweenPartitioningKeyPredicates(
       const char * pivLoName,
       const char * pivHiName,
       ItemExpr   * partNumExpr = NULL,
       NABoolean    useHash2Split = FALSE);

private:

  // --------------------------------------------------------------------
  // For run-time type identification.
  // --------------------------------------------------------------------
  PartitioningFunctionTypeEnum  functionType_;

  // --------------------------------------------------------------------
  // A set of key columns that are used for determining the specific
  // partition to which a row of a table should belong.
  // If an ordering exists on the key columns, its implementation is
  // specific to the partitioning function, i.e., a derived class.
  // --------------------------------------------------------------------
  ValueIdSet  partitioningKeyColumns_;

  // --------------------------------------------------------------------
  // Storage for the partitioning key predicates and the partition
  // input values that are built.
  // The partition inputs values is a set of values (variables) that
  // identify the specific partition or a set of partitions that
  // must be accessed.
  // --------------------------------------------------------------------
  ValueIdSet  partitioningKeyPredicates_;
  ValueIdSet  partitionInputValues_;
  ValueIdList partitionInputValuesLayout_;
  NABoolean   partKeyPredsCreated_;

  // --------------------------------------------------------------------
  // The implementation for the partitioning function.
  // --------------------------------------------------------------------
  ItemExpr* partitioningExpression_;

  // ---------------------------------------------------------------------
  // An integer variable that will hold the status of a data
  // conversion in the partitioning expression. The variable may
  // indicate that we couldn't convert a value and that therefore the
  // result of the expression is invalid. Depending on the context,
  // this is either an internal error or it means that we need to
  // agree on a default partition number for those cases, or it means
  // that we can discard the row that caused this error.
  // ---------------------------------------------------------------------
  ItemExpr *dataConversionErrorFlag_;

  // ---------------------------------------------------------------------
  // For Round Robin, and possibly other types of partitioning, it is
  // necessary at insertion to assign a partition to the row being
  // inserted.  The partitioniong function determines if the partition
  // selection expression is to do partition assignment through use of
  // this boolean.  The binder sets the boolean to TRUE: 
  // - If the table is being inserted into and the partitioning function is
  //   RoundRobin.
  // ---------------------------------------------------------------------
  NABoolean assignPartition_;

  // ------------------------------------------------------------------------
  //  An association of a partition's process (either ESP or DP2) with an SMP
  // node and its cluster.  The node map has an entry for each partition.
  // ------------------------------------------------------------------------
  NodeMap* nodeMap_;

  // The partition selection expression.  'Partition selection' means
  // that an expression is used to determine the partition to access
  // as opposed to using the File System to determine the range of
  // partitions to access based on a set of partitioning key
  // predicates.
  //
  ItemExpr *partitionSelectionExpr_;

  // A list of inputs to the partition selection expression.  This
  // list is populated when the partition selection expression is
  // created.  The partition selection expression needs two inputs:
  // the partition number and the total number of partitions.
  //
  ValueIdList partitionSelectionExprInputs_;

  // search key of a (select) predicate. Used by in-DP2 
  // Compound Statement to assure all its containing 
  // statements share the same serach key.

  NABoolean setupForStatement_;
  NABoolean resetAfterStatement_;

  Lng32 restrictedBeginPartNumber_;
  Lng32 restrictedEndPartNumber_;
  // to store number of active streams
  CostScalar activeStreams_;

}; // class PartitioningFunction


// -----------------------------------------------------------------------
// SINGLE PARTITION PARTITIONING FUNCTION
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// A function for creating a single partition.
// Set of partitioning keys is empty.
// The partitioning expression is a ConstValue(0).
// -----------------------------------------------------------------------
class SinglePartitionPartitioningFunction : public PartitioningFunction
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  SinglePartitionPartitioningFunction(NodeMap* nodeMap = 0,
                                      NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(SINGLE_PARTITION_PARTITIONING_FUNCTION,
                            nodeMap,heap)
  { hasNoPartitioningKeyPredicates(); }

  SinglePartitionPartitioningFunction (const SinglePartitionPartitioningFunction& other,
                                       NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(other,heap)
  { hasNoPartitioningKeyPredicates(); }

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~SinglePartitionPartitioningFunction();

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  SinglePartitionPartitioningFunction*
                        castToSinglePartitionPartitioningFunction() const;

  virtual Lng32 getCountOfPartitions() const;

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual PartitioningFunction* copy() const;

  virtual void createPartitioningKeyPredicates();

  // Replace the pivs, partitioning key predicates and partitioning 
  // expression with those passed in.
  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  virtual ItemExpr* createPartitioningExpression() ;

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  virtual NABoolean shouldUseSynchronousAccess(
    const ReqdPhysicalProperty* rpp,
    const EstLogPropSharedPtr& inputLogProp,
    GroupAttributes* ga) const;

  NABoolean checkSamePartitionNeeded() const { return FALSE; };
  NABoolean 
     partFuncAndFuncPushDownCompatible(const PartitioningFunction&) const;

  virtual const NAString getText() const;
  virtual void print(
       FILE* ofd = stdout,
       const char* indent = DEFAULT_INDENT,
       const char* title = "PartitioningFunction") const;

private :

}; // class SinglePartitionPartitioningFunction


// -----------------------------------------------------------------------
// REPLICATE VIA BROADCAST PARTITIONING FUNCTION
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// A function for replicating data via a broadcast - i.e. repartitioning
// the data but sending all the data to all the consumers.
// Set of partitioning keys is empty.
// The partitioning expression is absent.
// It allows the optimizer to be ask for the replication of data
// such as for the parallel execution PLAN2 for hash joins.
// The ReplicateViaBroadcastPartitioningFunction has an empty partitioning 
// key. This causes it to assign every row that is supplied to it to
// be assigned to each partition that it forms.
// -----------------------------------------------------------------------
class ReplicateViaBroadcastPartitioningFunction : public PartitioningFunction
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  ReplicateViaBroadcastPartitioningFunction(Lng32 numberOfPartitions,
                                            NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(REPLICATE_VIA_BROADCAST_PARTITIONING_FUNCTION,
                            NULL, heap),
       numberOfPartitions_(numberOfPartitions)
  { hasNoPartitioningKeyPredicates(); }

  ReplicateViaBroadcastPartitioningFunction
    (Lng32 numberOfPartitions, NodeMap *nodemap,
     NAMemory* heap = CmpCommon::statementHeap())
    : PartitioningFunction(REPLICATE_VIA_BROADCAST_PARTITIONING_FUNCTION,
                           nodemap, heap)
    , numberOfPartitions_(numberOfPartitions)
    { hasNoPartitioningKeyPredicates(); }

  ReplicateViaBroadcastPartitioningFunction(
               const ReplicateViaBroadcastPartitioningFunction& other,
               NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(other,heap),
       numberOfPartitions_(other.numberOfPartitions_)
  { hasNoPartitioningKeyPredicates(); }

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~ReplicateViaBroadcastPartitioningFunction();

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  ReplicateViaBroadcastPartitioningFunction* 
    castToReplicateViaBroadcastPartitioningFunction() const;

  virtual Lng32 getCountOfPartitions() const;

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual PartitioningFunction* copy() const;

  virtual void createPartitioningKeyPredicates();

  // Replace the pivs, partitioning key predicates and partitioning 
  // expression with those passed in.
  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  virtual ItemExpr* createPartitioningExpression() ;

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist = 
                            DEFAULT_PARTITION_GROUPING);

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningFunction") const;

private :

  // ---------------------------------------------------------------------
  // The number of partitions that are desired.
  // ---------------------------------------------------------------------
  Lng32 numberOfPartitions_;

}; // class ReplicateViaBroadcastPartitioningFunction


// -----------------------------------------------------------------------
// REPLICATE NO BROADCAST PARTITIONING FUNCTION
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// A function for replicating data with no broadcast - i.e. each ESP     
// process asks his child to access all partitions, instead of only 1/N
// of the data. 
// Set of partitioning keys is empty.
// The partitioning expression is absent.
// It allows the optimizer to be ask for the replication of data
// such as for the parallel execution PLAN2 for nested joins.
// The ReplicateNoBroadcastPartitioningFunction has an empty partitioning 
// key. This causes it to assign every row that is supplied to it to
// be assigned to each partition that it forms.
// -----------------------------------------------------------------------
class ReplicateNoBroadcastPartitioningFunction : public PartitioningFunction
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  ReplicateNoBroadcastPartitioningFunction(Lng32 numberOfPartitions,
                                           NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(REPLICATE_NO_BROADCAST_PARTITIONING_FUNCTION,
                            heap),
       numberOfPartitions_(numberOfPartitions)
  { hasNoPartitioningKeyPredicates(); }

  ReplicateNoBroadcastPartitioningFunction
    (Lng32 numberOfPartitions, NodeMap *nodemap,
     NAMemory* heap = CmpCommon::statementHeap())
    : PartitioningFunction(REPLICATE_NO_BROADCAST_PARTITIONING_FUNCTION,
                           nodemap, heap)
    , numberOfPartitions_(numberOfPartitions)
    { hasNoPartitioningKeyPredicates(); }

  ReplicateNoBroadcastPartitioningFunction(
               const ReplicateNoBroadcastPartitioningFunction& other,
               NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(other,heap),
       numberOfPartitions_(other.numberOfPartitions_)
  { hasNoPartitioningKeyPredicates(); }

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~ReplicateNoBroadcastPartitioningFunction();

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  ReplicateNoBroadcastPartitioningFunction* 
    castToReplicateNoBroadcastPartitioningFunction() const;

  virtual Lng32 getCountOfPartitions() const;

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual PartitioningFunction* copy() const;

  virtual void createPartitioningKeyPredicates();

  // Replace the pivs, partitioning key predicates and partitioning 
  // expression with those passed in.
  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  virtual ItemExpr* createPartitioningExpression() ;

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist = 
                            DEFAULT_PARTITION_GROUPING);

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningFunction") const;

private :

  // ---------------------------------------------------------------------
  // The number of partitions that are desired.
  // ---------------------------------------------------------------------
  Lng32 numberOfPartitions_;

}; // class ReplicateNoBroadcastPartitioningFunction


// -----------------------------------------------------------------------
// HASH PARTITIONING FUNCTION
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// HashPartitioningFunction
// -----------------------------------------------------------------------
class HashPartitioningFunction : public PartitioningFunction
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  HashPartitioningFunction(Lng32 numberOfHashPartitions,
                           NodeMap* nodeMap,
                           NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(HASH_PARTITIONING_FUNCTION, nodeMap,heap),
       numberOfHashPartitions_(numberOfHashPartitions)
  {}

  HashPartitioningFunction(const ValueIdSet& partitioningKeyColumns,
                           const ValueIdList& partitioningKeyColumnList,
			   Lng32 numberOfHashPartitions,
                           NodeMap* nodeMap = 0,
                           NAMemory* heap = CmpCommon::statementHeap(),
                           const PartitioningFunctionTypeEnum ftype
                            = HASH_PARTITIONING_FUNCTION
                           )
     : PartitioningFunction(ftype,
                            partitioningKeyColumns,
                            nodeMap,
                            heap),
       keyColumnList_(partitioningKeyColumnList),
       originalKeyColumnList_(partitioningKeyColumnList),
       numberOfHashPartitions_(numberOfHashPartitions)
  {
    // MUST be given some partitioning keys and a hash table size.
    CMPASSERT((NOT getPartitioningKey().isEmpty()) AND 
              (NOT getKeyColumnList().isEmpty()) AND
              numberOfHashPartitions_);
  }

  HashPartitioningFunction(const HashPartitioningFunction& other,
                           NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(other,heap),
       keyColumnList_(other.keyColumnList_),
       originalKeyColumnList_(other.originalKeyColumnList_),
       numberOfHashPartitions_(other.numberOfHashPartitions_)
  {
    // MUST be given some partitioning keys and a hash table size.
    CMPASSERT( (NOT getPartitioningKey().isEmpty()) AND 
               (NOT getKeyColumnList().isEmpty()) AND
               numberOfHashPartitions_);
  }

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~HashPartitioningFunction();

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  HashPartitioningFunction* castToHashPartitioningFunction() const;

  virtual Lng32 getCountOfPartitions() const;

  const ValueIdList& getKeyColumnList() const   { return keyColumnList_; }
  const ValueIdList& getOriginalKeyColumnList() const
  { return originalKeyColumnList_; }

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual PartitioningFunction* copy() const;

  virtual COMPARE_RESULT comparePartFuncToFunc
                               (const PartitioningFunction &other) const;

  virtual void createPartitioningKeyPredicates();

  // Replace the pivs, partitioning key predicates and partitioning 
  // expression with those passed in.
  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  // build the part expr using the part key columns as input
  virtual ItemExpr* createPartitioningExpression() ;

  virtual void remapIt(const PartitioningFunction* opf,
                       ValueIdMap& map, NABoolean mapItUp);

  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist = 
                            DEFAULT_PARTITION_GROUPING);

  virtual void preCodeGen(const ValueIdSet& availableValues);

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);
  
  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningFunction") const;

  ItemExpr * buildHashingExpressionForExpr(ItemExpr* expr) const;
  ItemExpr *getHashingExpression() const;
  UInt32 computeHashValue(char* data, UInt32 flags, Int32 len);

protected:
  virtual const NAString getTextImp(const char*) const;

  COMPARE_RESULT comparePartKeyToKey(const PartitioningFunction &other) const;

private:

protected:

  // ---------------------------------------------------------------------
  // The size of the hash table that is built using this partitioning
  // function.
  // ---------------------------------------------------------------------
  Lng32 numberOfHashPartitions_;

  // ----------------------------------------------------------------------
  // An order-sensitive representation for the partitioning keys.
  // ----------------------------------------------------------------------
  ValueIdList keyColumnList_;

  // ---------------------------------------------------------------------
  // The original keyColumnList_. This is different from keyColumnList_
  // only if the original PartitioningFunction got remapped with remapIt.
  // Used to determine the original data types of the key columns.
  // ---------------------------------------------------------------------
  ValueIdList originalKeyColumnList_;

}; // class HashPartitioningFunction

// -----------------------------------------------------------------------
// TableHashPartitioningFunction
// -----------------------------------------------------------------------
class TableHashPartitioningFunction : public PartitioningFunction
{
  friend class SkewedDataPartitioningFunction;

public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  TableHashPartitioningFunction(const PartitioningFunctionTypeEnum ftype,
                                Lng32 numberOfHashPartitions,
                                NodeMap* nodeMap,
                                NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(ftype, nodeMap, heap),
       numberOfOrigHashPartitions_(numberOfHashPartitions),
       numberOfPartitions_(numberOfHashPartitions),
       setupForStatement_(FALSE),
       resetAfterStatement_(FALSE),
       doVarCharCast_(FALSE)
  {}

  TableHashPartitioningFunction(const PartitioningFunctionTypeEnum ftype,
                                const ValueIdSet& partitioningKeyColumns,
                                const ValueIdList& partitioningKeyColumnList,
                                Lng32 numberOfHashPartitions,
                                NodeMap* nodeMap = 0,
                                NAMemory* heap = CmpCommon::statementHeap())
    : PartitioningFunction(ftype,
                           partitioningKeyColumns,
                           nodeMap,
                           heap),
       keyColumnList_(partitioningKeyColumnList),
       originalKeyColumnList_(partitioningKeyColumnList),
       numberOfOrigHashPartitions_(numberOfHashPartitions),
       numberOfPartitions_(numberOfHashPartitions),
       setupForStatement_(FALSE),
       resetAfterStatement_(FALSE),
       doVarCharCast_(FALSE)
  {
    // MUST be given some partitioning keys and a hash table size.
    CMPASSERT((NOT getPartitioningKey().isEmpty()) AND
              (NOT getKeyColumnList().isEmpty()) AND
              numberOfHashPartitions);
  }

  TableHashPartitioningFunction(const TableHashPartitioningFunction& other,
                                NAMemory* heap = CmpCommon::statementHeap())
    : PartitioningFunction(other, heap),
      keyColumnList_(other.keyColumnList_),
      originalKeyColumnList_(other.originalKeyColumnList_),
      numberOfOrigHashPartitions_(other.numberOfOrigHashPartitions_),
      numberOfPartitions_(other.numberOfPartitions_),
      setupForStatement_(other.setupForStatement_),
      resetAfterStatement_(other.resetAfterStatement_),
      doVarCharCast_(other.doVarCharCast_)
  {
    // MUST be given some partitioning keys and a hash table size.
    CMPASSERT((NOT getPartitioningKey().isEmpty()) AND
              (NOT getKeyColumnList().isEmpty()) AND
              numberOfOrigHashPartitions_ AND
              numberOfPartitions_);
  }

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~TableHashPartitioningFunction();

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  TableHashPartitioningFunction* castToTableHashPartitioningFunction() const;

  virtual Lng32 getCountOfPartitions() const;

  virtual void normalizePartitioningKeys(NormWA& normWARef);

  virtual void createPartitioningKeyPredicates();
  void createPartitioningKeyPredicatesForSaltedTable(ValueId saltCol);

  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual void remapIt(const PartitioningFunction* opf,
                       ValueIdMap& map, NABoolean mapItUp);

  virtual PartitioningFunction *
  createPartitioningFunctionForIndexDesc(IndexDesc *idesc) const;

  virtual void preCodeGen(const ValueIdSet& availableValues);

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength) = 0;

  // Make a new partSearchKey that indicates that
  // PA_PARTITION_GROUPING is being done.  Note that a search key can
  // not be generated which can group hashed partitions.  For
  // TableHashPartitioning, a flag in the search key is used to
  // indicate that PA_PARTITION_GROUPING is being done and the
  // begin/end key values of the search key are set to the partition
  // input values of the partitioning function.
  virtual SearchKey *createSearchKey(const IndexDesc *indexDesc,
                                     ValueIdSet availInputs,
                                     ValueIdSet additionalPreds) const;

  virtual ItemExpr* createPartitioningExpression() ;

  void createPartitionSelectionExprInputs();

  virtual ItemExpr *
  createPartitionSelectionExpr(const SearchKey *partSearchKey,
                               const ValueIdSet &availableValues);

  // ---------------------------------------------------------------------
  // The original (physical) number of hash partitions before any scaling.
  // ---------------------------------------------------------------------
  Lng32 getCountOfOrigHashPartitions() const
  // Replace the pivs, partitioning key predicates and partitioning
  // expression with those passed in.
  { return numberOfOrigHashPartitions_;};

  // ---------------------------------------------------------------------
  // Accessor method for the list of key columns.
  // TableHashPartitioningFunction is sensitive to the order of the
  // partitioning keys.
  // ---------------------------------------------------------------------
  const ValueIdList& getKeyColumnList() const   { return keyColumnList_; }
  const ValueIdList& getOriginalKeyColumnList() const
  { return originalKeyColumnList_; }

  void setupForStatement();
  void resetAfterStatement();

  virtual const NAString getText() const = 0;


  ItemExpr *buildHashingExpressionForExpr(ItemExpr*) const;
  ItemExpr *getHashingExpression() const;
  UInt32 computeHashValue(char* data, UInt32 flags, Int32 len);

protected :

  // ---------------------------------------------------------------------
  // The number of partitions of the original (physical) partitioning
  // function before any scaling.
  // ---------------------------------------------------------------------
  Lng32 numberOfOrigHashPartitions_;

  // After any scaling
  //
  Lng32 numberOfPartitions_;

  // ----------------------------------------------------------------------
  // An order-sensitive representation for the partitioning keys.
  // ----------------------------------------------------------------------
  ValueIdList keyColumnList_;

  // ---------------------------------------------------------------------
  // The original keyColumnList_. This is different from keyColumnList_
  // only if the original PartitioningFunction got remapped with remapIt.
  // Used to determine the original data types of the key columns.
  // Note that the original data types of a requirement must match the
  // actual data types exactly if we want to match it with an actual table.
  // ---------------------------------------------------------------------
  ValueIdList originalKeyColumnList_;

  NABoolean setupForStatement_;
  NABoolean resetAfterStatement_;

private:
  virtual ItemExpr *buildPartitioningExpression(
                       const ValueIdList &keyCols) const = 0;
  virtual ItemExpr *buildPartitioningSelectionExpr(
                       const ValueIdList &keyCols,
                       ItemExpr *numParts) const = 0;

  ItemExpr* createPartitioningExpressionImp(NABoolean doVarCharCast) ;

  NABoolean doVarCharCast_;
}; // class TableHashPartitioningFunction


// -----------------------------------------------------------------------
// HASH DISTRIBUTION PARTITIONING FUNCTION for Hash Fragmentation of a table
// The external hash partitioning function.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// HashDistPartitioningFunction
// -----------------------------------------------------------------------
class HashDistPartitioningFunction : public TableHashPartitioningFunction
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  HashDistPartitioningFunction(Lng32 numberOfHashPartitions,
                               NodeMap* nodeMap,
                               NAMemory* heap = CmpCommon::statementHeap())
    : TableHashPartitioningFunction(HASH_DIST_PARTITIONING_FUNCTION,
                                    numberOfHashPartitions, nodeMap, heap)
  {};

  HashDistPartitioningFunction(const ValueIdSet& partitioningKeyColumns,
                               const ValueIdList& partitioningKeyColumnList,
                               Lng32 numberOfHashPartitions,
                               NodeMap* nodeMap = 0,
                               NAMemory* heap = CmpCommon::statementHeap())
     : TableHashPartitioningFunction(HASH_DIST_PARTITIONING_FUNCTION,
                                     partitioningKeyColumns,
                                     partitioningKeyColumnList,
                                     numberOfHashPartitions,
                                     nodeMap,
                                     heap)
  {}

  HashDistPartitioningFunction(const HashDistPartitioningFunction& other,
                               NAMemory* heap = CmpCommon::statementHeap())
     : TableHashPartitioningFunction(other, heap)
  {}

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~HashDistPartitioningFunction();

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  HashDistPartitioningFunction* castToHashDistPartitioningFunction() const;

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  virtual PartitioningFunction* copy() const;

  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningFunction") const;

  virtual PartitioningFunction *
  createPartitioningFunctionForIndexDesc(IndexDesc *idesc) const;

  virtual COMPARE_RESULT comparePartFuncToFunc
                               (const PartitioningFunction &other) const;

  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist =
                            DEFAULT_PARTITION_GROUPING);

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

protected:

private :
  virtual ItemExpr *buildPartitioningExpression(const ValueIdList &keyCols) const;
  virtual ItemExpr *buildPartitioningSelectionExpr(const ValueIdList &keyCols,
                                                   ItemExpr *numParts) const;
}; // class HashDistPartitioningFunction

// -----------------------------------------------------------------------
// Hash2PartitioningFunction
// -----------------------------------------------------------------------
class Hash2PartitioningFunction : public TableHashPartitioningFunction
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  Hash2PartitioningFunction(Lng32 numberOfHashPartitions,
                              NodeMap* nodeMap,
                              NAMemory* heap = CmpCommon::statementHeap())
    : TableHashPartitioningFunction(HASH2_PARTITIONING_FUNCTION,
                                    numberOfHashPartitions, nodeMap, heap)
  {}

  Hash2PartitioningFunction(const ValueIdSet& partitioningKeyColumns,
                              const ValueIdList& partitioningKeyColumnList,
                              Lng32 numberOfHashPartitions,
                              NodeMap* nodeMap = 0,
                              NAMemory* heap = CmpCommon::statementHeap())
     : TableHashPartitioningFunction(HASH2_PARTITIONING_FUNCTION,
                                     partitioningKeyColumns,
                                     partitioningKeyColumnList,
                                     numberOfHashPartitions,
                                     nodeMap,
                                     heap)
  {}

  Hash2PartitioningFunction(const Hash2PartitioningFunction& other,
                              NAMemory* heap = CmpCommon::statementHeap())
    : TableHashPartitioningFunction(other, heap)
  {}

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~Hash2PartitioningFunction();

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  Hash2PartitioningFunction* castToHash2PartitioningFunction() const;

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  virtual PartitioningFunction* copy() const;

  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningFunction") const;

  virtual PartitioningFunction *
  createPartitioningFunctionForIndexDesc(IndexDesc *idesc) const;

  virtual void
  createPartSelectionExprFromSearchKey(const ValueId beginPartSelId,
                                       const ValueId endPartSelId,
                                       ValueIdList &partSelectionValIds) const;

  virtual COMPARE_RESULT comparePartFuncToFunc
                               (const PartitioningFunction &other) const;

  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist =
                            DEFAULT_PARTITION_GROUPING);

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  NABoolean canHandleSkew() const { return TRUE; };

private :
  ItemExpr *buildPartitioningExpression(const ValueIdList &keyCols) const;
  virtual ItemExpr *buildPartitioningSelectionExpr(const ValueIdList &keyCols,
                                                   ItemExpr *numParts) const;

}; // class Hash2PartitioningFunction


// -----------------------------------------------------------------------
// SkewedDataPartitioningFunction 
//
//  A partitioning function describing the partitioning of skew data.
//
//  One unique feature of this function is that its partitioning key 
//  contains a special value such that this function will
//  not match with any other non skeweddata partfunc. 
//
//  In addition, this partitioning function contains a partital 
//  partitioning function describing the non-skewed data 
//  portion. This function is accessable through the virtual method 
//  getPartialPartitioningFunction(). All other non-skew partitioning functions 
//  implement this method by returning their original partitioning keys
//  (i.e., getPartitioningKey() == getPartialPartitioningKey() for all non-skew
//  partfuncs.
//
//  Most required methods for this class are deligated to the contained 
//  partial partitioning function. 
// -----------------------------------------------------------------------
class SkewedDataPartitioningFunction : public PartitioningFunction
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  SkewedDataPartitioningFunction(PartitioningFunction* partFuncForUnskewed,
                              const skewProperty& sk,
                              NAMemory* heap = CmpCommon::statementHeap()
                                );

  SkewedDataPartitioningFunction(const SkewedDataPartitioningFunction& other,
                              NAMemory* heap = CmpCommon::statementHeap());

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~SkewedDataPartitioningFunction() {};

  // get the partition key for the non-skewed data 
  const ValueIdSet& getPartialPartitioningKey() const 
  { return partialPartFunc_ -> getPartitioningKey(); }

  // get the partition function for the non-skewed data 
  const PartitioningFunction* getPartialPartitioningFunction() const 
  { return partialPartFunc_; }

  void createPartitioningKeyPredicates();
  Lng32 getCountOfPartitions() const;
  void createPIV(ValueIdList &partInputValues);

  void replacePivs(const ValueIdList& newPivs,
                   const ValueIdSet& newPartKeyPreds);

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  SkewedDataPartitioningFunction* castToSkewedDataPartitioningFunction() const
  { return this; };

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual PartitioningFunction* copyAndRemap
                           (ValueIdMap& map, NABoolean mapItUp) const;

  ItemExpr* createPartitioningExpression();

  virtual void preCodeGen(const ValueIdSet& availableValues);
  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  virtual PartitioningFunction* copy() const;

  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningFunction") const;


  virtual COMPARE_RESULT comparePartFuncToFunc
                               (const PartitioningFunction &other) const;

  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist =
                            DEFAULT_PARTITION_GROUPING);

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  // Helper functions on skewed values
  const skewProperty& getSkewProperty() const { return skewProperty_; };
  void setSkewProperty(const skewProperty& sk) { skewProperty_ = sk; };

  // A virtual method returning the hash value list for skewed values.
  // Used during run-time to identify input rows containing any skew
  // values.
  virtual Int64List* buildHashListForSkewedValues();

  // A virtual method returning a run-time hashing expression that hashes a
  // skew value into a hash value. Called during codeGen phase.
  ItemExpr *getHashingExpression() const 
  { return partialPartFunc_-> getHashingExpression(); };

  // A method returns the hash for a skew value
  UInt32 computeHashValue(char* data, UInt32 flags,  Int32 len)
  { return partialPartFunc_-> computeHashValue(data, flags, len); };

  // A virtual method returning a compiler-time hashing expression that
  // hashes a skew value into a hash value. Called during codeGen phase.
  ItemExpr *buildHashingExpressionForExpr(ItemExpr* ie) const
  { return partialPartFunc_-> buildHashingExpressionForExpr(ie); };


protected:

  // the partfunc describes the non-skewed part of the data
  PartitioningFunction* partialPartFunc_;

  // the description of the skewed part of the data
  skewProperty skewProperty_;

  // a hash list for skewed values to help speedy identification of skew 
  // values during run-time.
  Int64List* skewHashList_;

}; // class SkewedDataPartitioningFunction

// -----------------------------------------------------------------------
// hiveHashPartitioningFunction
// -----------------------------------------------------------------------
class HivePartitioningFunction : public HashPartitioningFunction
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  HivePartitioningFunction(Lng32 numberOfHashPartitions,
                           NodeMap* nodeMap,
                           NAMemory* heap = CmpCommon::statementHeap())
     : HashPartitioningFunction(HIVE_PARTITIONING_FUNCTION, nodeMap,heap)
  {}

  HivePartitioningFunction(const ValueIdSet& partitioningKeyColumns,
                           const ValueIdList& partitioningKeyColumnList,
			   Lng32 numberOfHashPartitions,
                           NodeMap* nodeMap = 0,
                           NAMemory* heap = CmpCommon::statementHeap()) 
     : HashPartitioningFunction(partitioningKeyColumns,
                           partitioningKeyColumnList,
			   numberOfHashPartitions,
                           nodeMap,
                           heap, 
                           HIVE_PARTITIONING_FUNCTION)
  {
  }

  HivePartitioningFunction(const HashPartitioningFunction& other,
                           NAMemory* heap = CmpCommon::statementHeap())
     : HashPartitioningFunction(other,heap)
  {
  }

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~HivePartitioningFunction();

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  HivePartitioningFunction* castToHivePartitioningFunction() const
  { return this; } ;

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual PartitioningFunction* copy() const;

  virtual void createPartitioningKeyPredicates();

  // build the part expr using the part key columns as input
  //virtual ItemExpr* createPartitioningExpression() ;

  //virtual void remapIt(const PartitioningFunction* opf,
  //                     ValueIdMap& map, NABoolean mapItUp);

  //virtual short codeGen(Generator* generator, Lng32 partInputDataLength);
  
  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningFunction") const;

  ItemExpr * buildHashingExpressionForExpr(ItemExpr* expr) const;
  ItemExpr *getHashingExpression() const;
  UInt32 computeHashValue(char* data, UInt32 flags, Int32 len);

  PartitioningFunction*
    createPartitioningFunctionForIndexDesc(IndexDesc *idesc) const;

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  virtual void normalizePartitioningKeys(NormWA& normWARef);

private :

}; // class HivePartitioningFunction




// -----------------------------------------------------------------------
// RANGE PARTITIONING FUNCTION
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// A range partitioning scheme distributes data amongst a set of
// partitions according to a predefined range of values that each
// partition can contain. Each partition therefore publishes the
// minimum value that it can contain in each of the partitioning
// key column. We call a tuple of minimum permissible key values
// for a partition, a partition boundary. A range-partitioned table
// with n partition has n+1 boundaries, numbered 0 through n, where
// boundary n is the maximum possible partitioning key value in the table.
//
// If the number of partitions is n, then there are n+1 partition
// boundaries. Entries 1 through n-1 define the actual boundaries
// between two consecutive partitions. Entries 0 and n contain the
// minimum and maximum permissible values, respectively. They are
// initialized to NULL in the constructor and updated later by the
// method RangePartitioningFunction::completePartitionBoundaries()
// -----------------------------------------------------------------------
class RangePartitionBoundaries : public NABasicObject
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // Allocate an array with 'numberOfPartitions+1' elements.
  // --------------------------------------------------------------------
  RangePartitionBoundaries(Lng32 numberOfPartitions,
			                     Lng32 numberOfPartitioningKeyColumns,
                           NAMemory *h = CmpCommon::statementHeap());

  // copy constructor
  RangePartitionBoundaries(const RangePartitionBoundaries& other,
                           NAMemory *h = CmpCommon::statementHeap())
    : partKeyColumnCount_(other.partKeyColumnCount_),
      origPartKeyColumnCount_(other.origPartKeyColumnCount_),
      boundaryValuesList_(other.boundaryValuesList_,h),
      boundaryStringsList_(other.boundaryStringsList_,h),
      boundaryValues_(other.boundaryValues_,h),
      binaryBoundaryValues_(other.binaryBoundaryValues_,h),
      partitionCount_(other.partitionCount_),
      origPartitionCount_(other.origPartitionCount_),
      encodedBoundaryKeyLength_(other.encodedBoundaryKeyLength_),
      setupForStatement_(other.setupForStatement_),
      resetAfterStatement_(other.resetAfterStatement_),
      setBinaryBoundaryFirstLastKey_(other.setBinaryBoundaryFirstLastKey_),
      heap_ (h)
  {}

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~RangePartitionBoundaries();

  // --------------------------------------------------------------------
  // Each partition boundary is a tuple that contains as many values
  // as there are partitioning key columns.
  // --------------------------------------------------------------------
  void defineUnboundBoundary(Lng32 partitionNumber,
          const ItemExpr* boundaryValue,
          const char *encodedKeyValue);

  // --------------------------------------------------------------------
  // Each partition boundary is a tuple that contains as many values
  // as there are partitioning key columns.
  // --------------------------------------------------------------------
  void defineBoundary(Lng32 partitionNumber,
		      const ItemExprList* boundaryValue,
		      const char *encodedKeyValue);

  // --------------------------------------------------------------------
  // bind a unbound boundary value and add it to the list of
  // boundaryValues_
  // --------------------------------------------------------------------
  void bindAddBoundaryValue(Lng32 partitionNumber);

  void checkConsistency(const Lng32 numberOfPartitions) const;

  // ---------------------------------------------------------------------
  // The number of partitions = the number of partition boundaries.
  // ---------------------------------------------------------------------
  Lng32 getCountOfPartitions() const            { return partitionCount_; }

  // ---------------------------------------------------------------------
  // the length of the encoded partition boundary key
  // ---------------------------------------------------------------------
  Lng32 getEncodedBoundaryKeyLength() const
                                     { return encodedBoundaryKeyLength_; }

  // ---------------------------------------------------------------------
  // Like for partitioning functions, change the number of partitions
  // to be close to a suggested new value and return the chosen new value.
  // ---------------------------------------------------------------------
  Lng32 scaleNumberOfPartitions(Lng32 suggestedNewNumberOfPartitions,
                               const NodeMap* nodeMap,
                               PartitionGroupingDistEnum partGroupDist = 
                                 DEFAULT_PARTITION_GROUPING);

  // ---------------------------------------------------------------------
  // Check whether one set of boundaries is a grouping of another
  // ---------------------------------------------------------------------
  NABoolean isAGroupingOf(const RangePartitionBoundaries &other,
                          Lng32* maxPartsPerGroup = NULL) const;

  // --------------------------------------------------------------------
  // Indexing operator
  // --------------------------------------------------------------------
  const ItemExprList* getBoundaryValues(Lng32 index) const;
  const char * getBinaryBoundaryValue(Lng32 index) const;

  // ---------------------------------------------------------------------
  // A method that is used for comparing two range partition boundaries
  // by the optimizer.
  // ---------------------------------------------------------------------
  NABoolean compareRangePartitionBoundaries(
       const RangePartitionBoundaries& other,
       NABoolean groupingAllowed = FALSE,
       Lng32* maxPartsPerGroup = NULL) const;

  // ---------------------------------------------------------------------
  // Merge two compatible sets of boundaries and produce a corresponding
  // node map for the new boundaries.
  // ---------------------------------------------------------------------
  RangePartitionBoundaries * merge(
       const RangePartitionBoundaries& other,
       const NodeMap& thisNodeMap,
             NodeMap& resultNodeMap   ) const;

 // ---------------------------------------------------------------------
  // Determine the minimum number of partitioning keys based on the
  // start key values that are specified. Columns for which no explicit
  // start key values are specified need not be part of the part key
  // ---------------------------------------------------------------------
  Lng32 getOptimizedNumberOfPartKeys();

  // ---------------------------------------------------------------------
  // Add the start boundary for the first partition (min key) and the
  // end boundary for the last range partition (max key).
  // This method is defined in PartFunc.cpp
  // ---------------------------------------------------------------------
  void completePartitionBoundaries(const ValueIdList& partitioningKeyOrder,
				   Lng32 encodedBoundaryKeyLength);


  // find a boundary pair [low, high) with smallest low value in which keys fall, and return the
  // // index of the boundary low. Return -1 otherwise, or the key lengths are different.
  Int32 findBeginBoundary(char* encodedKey, Int32 keyLen, compFuncPtrT compFunc) const;

  // find a boundary pair [low, high) with the largest low value in which keys fall, and return the
  // // index of the boundary low. Return -1 otherwise, or the key lengths are different.
  Int32 findEndBoundary(char* encodedKey, Int32 keyLen, compFuncPtrT compFunc) const;

  void setupForStatement(NABoolean useStringVersion);
  void resetAfterStatement();

  // ---------------------------------------------------------------------
  // Print
  // ---------------------------------------------------------------------
  void display() const                                        { print(); }

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "RangePartitionBoundaries") const;
private:

  // --------------------------------------------------------------------
  // This data is maintained simply to check the consistency of the
  // values supplied for each partition boundary.
  // --------------------------------------------------------------------
  Lng32 partKeyColumnCount_;
  Lng32 origPartKeyColumnCount_;

  ARRAY(const ItemExpr *) boundaryValuesList_;

  // The string list stores the string version of the boundary values
  // needed to reconstruct boundaryValuesList_ after one compilation 
  // a statement.
  // Each string is a comma separated list of SQL constants, representing
  // one partition boundary.
  ARRAY(const NAString*) boundaryStringsList_;

  // --------------------------------------------------------------------
  // Each partition boundary is a tuple that contains as many values
  // as there are key columns.
  // --------------------------------------------------------------------
  ARRAY(const ItemExprList *) boundaryValues_;
  ARRAY(const char *) binaryBoundaryValues_;

  Lng32 partitionCount_;
  Lng32 origPartitionCount_;

  // --------------------------------------------------------------------
  // length of the encoded boundary keys
  // --------------------------------------------------------------------
  Lng32 encodedBoundaryKeyLength_;

  NABoolean setupForStatement_;
  NABoolean resetAfterStatement_;
  NABoolean setBinaryBoundaryFirstLastKey_;

  //heap
  NAMemory * heap_;

}; // class RangePartitionBoundaries

// -----------------------------------------------------------------------
// RangePartitioningFunction
// -----------------------------------------------------------------------
class RangePartitioningFunction : public PartitioningFunction
{
public:

  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  RangePartitioningFunction(RangePartitionBoundaries* partitionBoundaries,
                            NodeMap* nodeMap,
                            NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(RANGE_PARTITIONING_FUNCTION,
                            nodeMap,
                            heap),
       partitionBoundaries_(partitionBoundaries),
       setupForStatement_(FALSE),
       resetAfterStatement_(FALSE)
 {}

  RangePartitioningFunction(const ValueIdSet& partitioningKeyColumns,
			    const ValueIdList& partitioningKeyColumnsList,
			    const ValueIdList& partitioningKeyColumnsOrder,
			    RangePartitionBoundaries* partitionBoundaries,
                            NodeMap* nodeMap,
                            NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(RANGE_PARTITIONING_FUNCTION,
                            partitioningKeyColumns,
                            nodeMap,
                            heap),
       keyColumnList_(partitioningKeyColumnsList),
       orderOfKeyValues_(partitioningKeyColumnsOrder),
       originalKeyColumnList_(partitioningKeyColumnsList),
       partitionBoundaries_(partitionBoundaries),
       setupForStatement_(FALSE),
       resetAfterStatement_(FALSE)
  {
    // MUST be given some partitioning keys and partition boundaries.
    CMPASSERT(partitioningKeyColumns.entries() AND
	      partitioningKeyColumnsList.entries() AND
	      partitioningKeyColumnsList.entries() ==
	         partitioningKeyColumnsOrder.entries() AND
	      partitionBoundaries);
  }

  RangePartitioningFunction(const RangePartitioningFunction& other,
                            NAMemory* heap = CmpCommon::statementHeap());

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~RangePartitioningFunction();

  // ---------------------------------------------------------------------
  // Perform a type-safe pointer cast.
  // ---------------------------------------------------------------------
  virtual const
  RangePartitioningFunction* castToRangePartitioningFunction() const;

  // ---------------------------------------------------------------------
  // The number of range partitions that will be formed using this scheme.
  // ---------------------------------------------------------------------
  virtual Lng32 getCountOfPartitions() const;

  // ---------------------------------------------------------------------
  // Accessor method for the list of key columns.
  // ---------------------------------------------------------------------
  const ValueIdList& getKeyColumnList() const   { return keyColumnList_; }

  // ---------------------------------------------------------------------
  // Accessor method for the range partition boundaries.
  // ---------------------------------------------------------------------
  const RangePartitionBoundaries* getRangePartitionBoundaries() const
                                          { return partitionBoundaries_; }

  // ---------------------------------------------------------------------
  // A list of expression that define the sort order on the values
  // that are contained in each key column.
  // If the values in a certain key column appear in the ascending
  // sequence, it contains the ValueId of the key column. Otherwise,
  // it contains the ValueId of an Inverse(key column) expression.
  // ---------------------------------------------------------------------
  const ValueIdList& getOrderOfKeyValues() const
                                            { return orderOfKeyValues_;  }

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual COMPARE_RESULT comparePartFuncToFunc
                               (const PartitioningFunction &other) const;

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual PartitioningFunction* copy() const;

  virtual void normalizePartitioningKeys(NormWA& normWARef);

  virtual void createPartitioningKeyPredicates();

  // Replace the pivs, partitioning key predicates and partitioning 
  // expression with those passed in.
  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist = 
                            DEFAULT_PARTITION_GROUPING);

  virtual void remapIt(const PartitioningFunction* opf,
                       ValueIdMap& map, NABoolean mapItUp);

  virtual PartitioningFunction* createPartitioningFunctionForIndexDesc
                                   (IndexDesc *idesc) const;

  virtual ItemExpr* createPartitioningExpression() ;

  virtual void preCodeGen(const ValueIdSet& availableValues);

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  virtual void generatePivLayout(Generator *generator,
				 Lng32 &partitionInputDataLength,
				 Lng32 atp,
				 Lng32 atpIndex,
				 Attributes ***pivAttrs);

  void setupForStatement();
  void resetAfterStatement();

  // Make a new partSearchKey with the partitioning key preds of
  // the partitioning function, if there are any. Note that ignoring
  // the part key preds will result in a wrong answer if we use
  // PA_PARTITION_GROUPING, since the PA node is the node responsible
  // for the grouping. If it doesn't select a subgroup of partitions,
  // too much data may be returned. For now we only consider a
  // search key for the PA node, MDAM to be implemented later.
  // MDAM will be useful for combining user-specified part key preds
  // with logicalPartFunc->getPartitioningKeyPredicates().
  virtual SearchKey *createSearchKey(const IndexDesc *indexDesc,
                                     ValueIdSet availInputs,
                                     ValueIdSet additionalPreds) const;

  virtual NABoolean usesFSForPartitionSelection() const { return TRUE; }

  virtual NABoolean shouldUseSynchronousAccess(
    const ReqdPhysicalProperty* rpp,
    const EstLogPropSharedPtr& inputLogProp,
    GroupAttributes* ga) const;

  NABoolean 
     partFuncAndFuncPushDownCompatible(const PartitioningFunction&) const;

  // ---------------------------------------------------------------------
  // Compute the number of active partitions. Active partitions are those
  // that will be accessed applying the search key skey.
  // ---------------------------------------------------------------------
  Int32 computeNumOfActivePartitions(SearchKey* skey, const TableDesc* tDesc) const;

  virtual const NAString getText() const;
  virtual void print( FILE* ofd = stdout,
                      const char* indent = DEFAULT_INDENT,
                      const char* title = "PartitioningFunction") const;
private:

  // ----------------------------------------------------------------------
  // An order-sensitive representation for the keys.
  // ----------------------------------------------------------------------
  ValueIdList keyColumnList_;

  // --------------------------------------------------------------------
  // If the values in a certain key column appear in the ascending
  // sequence, it contains the ValueId of the key column. Otherwise,
  // it contains the ValueId of an Inverse(key column) expression.
  // --------------------------------------------------------------------
  ValueIdList orderOfKeyValues_;

  // ---------------------------------------------------------------------
  // The original keyColumnList_. This is different from keyColumnList_
  // only if the original PartitioningFunction got remapped with remapIt.
  // Used to determine the original data types of the key columns.
  // ---------------------------------------------------------------------
  ValueIdList originalKeyColumnList_;

  // ----------------------------------------------------------------------
  // The partition boundaries for range-partitioned data.
  // ----------------------------------------------------------------------
  RangePartitionBoundaries* partitionBoundaries_;

  NABoolean setupForStatement_;
  NABoolean resetAfterStatement_;

}; // class RangePartitioningFunction

// -----------------------------------------------------------------------
// LogPhysPartitioningFunction
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// This partitioning function is actually an overlay of two different
// partitioning functions and it is generated by a DP2 scan node.
// This partitioning function reflects both the physical organization
// of the data in the DP2 partitions and the logical partitioning of
// the result, represented by the data returned by one or more DP2
// exchange nodes.
// -----------------------------------------------------------------------

class LogPhysPartitioningFunction : public PartitioningFunction
{
public:

  // ---------------------------------------------------------------------
  // An enumerated type that indicates the type of logical partitioning.
  //
  // ANY_LOGICAL_PARTITIONING
  //    This literal only occurs in requirements as a "don't care"
  //    entry. It is never actually synthesized.
  //
  // PA_PARTITION_GROUPING
  //	In this case, the PA node on top of a DP2 scan groups one or
  //    more DP2 partitions and represents them to its parent as a
  //    single partitions. No two PA nodes access the same DP2 partition.
  //    This also covers the most simple cases where there is only one
  //    DP2 partition and one PA or where the PA's partitioning function
  //    is the same as the table's "natural" partitioning function.
  //
  // LOGICAL_SUBPARTITIONING
  //    More than one PA may access a given DP2 partition and a PA may
  //    also access more than one DP2 partition. The PA nodes divide
  //    the table into exclusive ranges that are defined by the
  //    clustering key of the table which is also the partitioning key.
  //
  // HORIZONTAL_PARTITION_SLICING
  //    Similar to LOGICAL_SUBPARTITIONING, except that the clustering
  //    key is not the partitioning key of the table.
  //
  // PA_GROUPED_REPARTITIONING
  //    The PA nodes on top of the DP2 scan will perform
  //    PA_PARTITION_GROUPING to read the data and then repartition it,
  //    using an additional ESP exchange on top of the PA node. The number
  //    of ESPs that read DP2 data and repartition is determined by the
  //    DP2 exchange in this case, only the number of clients (PA nodes)
  //    is determined by the DP2 scan node.
  //
  // ---------------------------------------------------------------------
  enum logPartType
  {
    ANY_LOGICAL_PARTITIONING,
    PA_PARTITION_GROUPING,
    LOGICAL_SUBPARTITIONING,
    HORIZONTAL_PARTITION_SLICING,
    PA_GROUPED_REPARTITIONING
  };

  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  LogPhysPartitioningFunction(
       PartitioningFunction * logPartFunc,
       PartitioningFunction * physPartFunc,
       logPartType            logPartType,
       Lng32                   numOfClients,
       NABoolean              usePapa,
       NABoolean              synchronousAccess,
       NAMemory* heap = CmpCommon::statementHeap());

  LogPhysPartitioningFunction (const LogPhysPartitioningFunction& other,
                               NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(other,heap),
       logPartFunc_(other.logPartFunc_),physPartFunc_(other.physPartFunc_),
       realPartFunc_(other.realPartFunc_),
       logPartType_(other.logPartType_),numOfClients_(other.numOfClients_),
       usePapa_(other.usePapa_),
       synchronousAccess_(other.synchronousAccess_)
  {}

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  virtual ~LogPhysPartitioningFunction();

  // ---------------------------------------------------------------------
  // accessor methods
  // ---------------------------------------------------------------------

  inline PartitioningFunction * getLogPartitioningFunction() const
                                                  { return logPartFunc_; }
  inline PartitioningFunction * getPhysPartitioningFunction() const
                                                 { return physPartFunc_; }
  inline PartitioningFunction * getRealPartitioningFunction() const
                                                 { return realPartFunc_; }
  inline logPartType getLogPartType() const       { return logPartType_; }
  inline Lng32 getNumOfClients() const            { return numOfClients_; }
  inline NABoolean getUsePapa() const                 { return usePapa_; }
  inline NABoolean getSynchronousAccess() const
                                            { return synchronousAccess_; }

  // ---------------------------------------------------------------------
  // mutator methods
  // ---------------------------------------------------------------------
  inline void setNumOfClients(Lng32 numOfClients)
                                         { numOfClients_ = numOfClients; } 

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
  virtual const
  LogPhysPartitioningFunction * castToLogPhysPartitioningFunction() const;

  virtual Lng32 getCountOfPartitions() const;

  virtual PartitioningRequirement* makePartitioningRequirement();

  // ---------------------------------------------------------------------
  // Retrieve a reference to partitioning function node map.
  // ---------------------------------------------------------------------
  virtual const NodeMap* getNodeMap() const;

  // get any existing (logical or physical) nodemap (or synthesize one) that 
  // matches logPartFunc_'s partition count requirement
  NodeMap* getOrMakeSuitableNodeMap(NABoolean forESP) const;

  virtual PartitioningFunction* copy() const;

  virtual NABoolean canProducePartitioningKeyPredicates() const; // yes

  virtual void createPartitioningKeyPredicates();

  // Replace the pivs, partitioning key predicates and partitioning 
  // expression with those passed in.
  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  virtual ItemExpr* createPartitioningExpression() ;

  PartitioningFunction* createRealPartitioningFunction();

  // Can this logPhys partitioning function maintain the order of an
  // individual partition of the physical partitioning function.  In
  // order to maintain the order, a merge expression may be required.
  //
  NABoolean canMaintainSortOrder(const ValueIdList& sortOrder) const;

  virtual COMPARE_RESULT comparePartFuncToFunc
                               (const PartitioningFunction &other) const;

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  virtual void remapIt(const PartitioningFunction* opf,
                       ValueIdMap& map, NABoolean mapItUp);

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  virtual const NAString getText() const;

  //combined output of getLogForSplitTop and getPhysForSplitTop is similar
  //to getText, however, they are used whenever LogPhysPartitioningFunction
  //is the bottom partitioning function of the split_top node
  virtual const NAString getLogForSplitTop() const;
  virtual const NAString getPhysForSplitTop() const;

  virtual void print(
       FILE* ofd = stdout,
       const char* indent = DEFAULT_INDENT,
       const char* title = "PartitioningFunction") const;


private:

  // ---------------------------------------------------------------------
  // The "logical" part of the partitioning function. This is the top
  // partitioning function that the DP2 exchange will produce. Its
  // number of partitions indicates the number of processes with a PA
  // node and therefore the number of partitions in the ESP (or
  // master) in the non-DP2 plan fragment above. If we perform
  // PA_GROUPED_REPARTITIONING then the logical partitioning function
  // is the top partitioning function of the ESP exchange.
  // ---------------------------------------------------------------------
  PartitioningFunction * logPartFunc_;

  // ---------------------------------------------------------------------
  // The "physical" part of the partitioning function. It describes
  // the partitioning scheme of the DP2 table (or the DP2 tables in
  // case we are performing a join in DP2).
  // ---------------------------------------------------------------------
  PartitioningFunction * physPartFunc_;

  // ---------------------------------------------------------------------
  // The "real" partitioning scheme is calculated as an overlay of the
  // logical and physical partitioning scheme, except for
  // PA_GROUPED_REPARTITIONING where it is a grouping of the physical
  // partitioning function.
  //
  // For example, if both logical and physical partitioning function
  // are range partitioning functions on the clustering key and if we
  // are not performing PA_GROUPED_REPARTITIONING, then the "real"
  // partitioning function can be obtained by combining the partition
  // boundaries of both logical and physical partitioning function.
  // This would be needed to determine the partitioning key.
  //
  // Note that for any correctness conditions for operators in DP2, the
  // real partitioning function is what counts. So, if we want to
  // perform a type 1 join in DP2, we have to make sure that the *real*
  // partitioning functions of both its children are equivalent and
  // have the equi-join columns as their partitioning keys.
  // ---------------------------------------------------------------------
  PartitioningFunction * realPartFunc_;

  // ---------------------------------------------------------------------
  // The logical partitioning type indicates to the DP2 exchange above
  // which type of logical partitioning should be performed. See the
  // comment above in the enum declaration about possible types.
  // ---------------------------------------------------------------------
  logPartType          logPartType_;

  // ---------------------------------------------------------------------
  // The number of clients is the actual number of PA nodes used. This
  // does not need to be identical to the number of logical partitions,
  // if we use PAPA nodes. If the number of clients is different from the
  // number of logical partitions, it must be a multiple of the number
  // of logical partitions. We will then generate a PAPA node with
  // #log parts / numOfClients_ PA nodes.
  // ---------------------------------------------------------------------
  Lng32                 numOfClients_;

  // ---------------------------------------------------------------------
  // Indicator whether to use a PAPA node. The number of partitions of
  // the logical partitioning function divided by the number of clients
  // should already indicate when it is necessary to use a PAPA. This
  // flag is always set when the above ratio is > 1, and it may be set
  // when we want a PAPA with only one PA underneath it (e.g. for
  // certain situations where a PAPA is needed, like insert VSBB).
  // ---------------------------------------------------------------------
  NABoolean            usePapa_;

  // ---------------------------------------------------------------------
  // Indicator whether synchronous access will be used to satisfy a
  // required order or arrangement that came from above the DP2 exchange.
  // ---------------------------------------------------------------------
  NABoolean            synchronousAccess_;

}; // class LogPhysPartitioningFunction


// -----------------------------------------------------------------------
// ROUND ROBIN PARTITIONING FUNCTION
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// A Round Robin partitioning scheme distributes data amongst a set of
// partitions in a random fashion so as to balance the number of records
// in each partition.
// -----------------------------------------------------------------------
class RoundRobinPartitioningFunction : public PartitioningFunction
{
public:

  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  RoundRobinPartitioningFunction(const Lng32 partitionCount,
                                 NodeMap* nodeMap,
                                 NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(ROUND_ROBIN_PARTITIONING_FUNCTION,
                            nodeMap,
                            heap),
       numberOfOrigRRPartitions_(partitionCount),
       partitionCount_(partitionCount),
       setupForStatement_(FALSE),
       resetAfterStatement_(FALSE)
  {}

  RoundRobinPartitioningFunction(const Lng32 partitionCount,
                                 const ValueIdSet& partitioningKeyColumns,
                                 NodeMap* nodeMap,
                                 NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(ROUND_ROBIN_PARTITIONING_FUNCTION,
                            partitioningKeyColumns,
                            nodeMap,
                            heap),
       numberOfOrigRRPartitions_(partitionCount),
       partitionCount_(partitionCount),
       setupForStatement_(FALSE),
       resetAfterStatement_(FALSE)
  {
    // MUST be given one (SYSKEY) column as the partitioning key and
    // a greater than zero number or partitions
    //
    CMPASSERT((getPartitioningKey().entries() == 1) AND 
              (partitionCount_ > 0));
  }

  RoundRobinPartitioningFunction(const RoundRobinPartitioningFunction& other,
                                 NAMemory* heap = CmpCommon::statementHeap())
     : PartitioningFunction(other,heap),
       numberOfOrigRRPartitions_(other.numberOfOrigRRPartitions_),
       partitionCount_(other.partitionCount_),
       setupForStatement_(other.setupForStatement_),
       resetAfterStatement_(other.resetAfterStatement_)
  {
    // MUST be given one (SYSKEY) column as the partitioning key and
    // a greater than zero number or partitions
    //
    CMPASSERT((getPartitioningKey().entries() == 1) AND 
              (partitionCount_ > 0));
  }

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
// warning elimination (removed "inline")
  virtual ~RoundRobinPartitioningFunction() {}

  // ---------------------------------------------------------------------
  // see base class for explanations of the virtual methods
  // ---------------------------------------------------------------------
// warning elimination (removed "inline")
  virtual const RoundRobinPartitioningFunction *
  castToRoundRobinPartitioningFunction() const { return this; }

  virtual Lng32 getCountOfPartitions() const    { return partitionCount_; }

  virtual PartitioningRequirement* makePartitioningRequirement();

  virtual PartitioningFunction* copy() const;

  virtual COMPARE_RESULT comparePartFuncToFunc
                               (const PartitioningFunction &other) const;

  virtual NABoolean isAGroupingOf(const PartitioningFunction &other,
                                  Lng32* maxPartsPerGroup = NULL) const;

  virtual void createPartitioningKeyPredicates();

  // Replace the pivs, partitioning key predicates and partitioning 
  // expression with those passed in.
  virtual void replacePivs(
    const ValueIdList& newPivs,
    const ValueIdSet& newPartKeyPreds);

  virtual PartitioningFunction *
  scaleNumberOfPartitions(Lng32 &suggestedNewNumberOfPartitions,
                          PartitionGroupingDistEnum partGroupDist = 
                            DEFAULT_PARTITION_GROUPING);

  virtual PartitioningFunction* createPartitioningFunctionForIndexDesc
                                   (IndexDesc *idesc) const;

  virtual ItemExpr* createPartitioningExpression() ;

  virtual ItemExpr *
  createPartitionSelectionExpr(const SearchKey *partSearchKey,
                               const ValueIdSet &availableValues);

  virtual short codeGen(Generator* generator, Lng32 partInputDataLength);

  // Make a new partSearchKey that indicates that
  // PA_PARTITION_GROUPING is being done.  Note that a search key can
  // not be generated which can group RR partitions.  For
  // RoundRobinPartitioning, a flag in the search key is used to
  // indicate that PA_PARTITION_GROUPING is being done and the
  // begin/end key values of the search key are set to the partition
  // input values of the partitioning function.
  virtual SearchKey *createSearchKey(const IndexDesc *indexDesc,
                                     ValueIdSet availInputs,
                                     ValueIdSet additionalPreds) const;

  // ---------------------------------------------------------------------
  // The original (physical) number of RR partitions before any scaling.
  // ---------------------------------------------------------------------
  Lng32 getCountOfOrigRRPartitions() const { return numberOfOrigRRPartitions_;};

  void setupForStatement();
  void resetAfterStatement();

  virtual const NAString getText() const;
  virtual void print(
       FILE* ofd = stdout,
       const char* indent = DEFAULT_INDENT,
       const char* title = "PartitioningFunction") const;

private:

  // ---------------------------------------------------------------------
  // The number of partitions of the original (physical) partitioning
  // function before any scaling.
  // ---------------------------------------------------------------------
  Lng32 numberOfOrigRRPartitions_;

  // ----------------------------------------------------------------------
  // The number of partitions.
  // ----------------------------------------------------------------------
  Lng32 partitionCount_;

  NABoolean setupForStatement_;
  NABoolean resetAfterStatement_;

}; // class RoundRobinPartitioningFunction
//


#endif /* PARTFUNC_H */
