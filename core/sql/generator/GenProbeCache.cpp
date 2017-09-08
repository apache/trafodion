// **********************************************************************
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
// **********************************************************************
//
#include "ComOptIncludes.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "GroupAttr.h"
#include "ExpCriDesc.h"
#include "DefaultConstants.h"
#include "ComTdbProbeCache.h"
#include "RelProbeCache.h"
#include "DefaultConstants.h"
#include "ExpSqlTupp.h"         // for sizeof(tupp_descriptor)
#include "ComDefs.h"            // to get common defines (ROUND8)

/////////////////////////////////////////////////////////////////////
//
// Contents:
//   
//   ProbeCache::codeGen()
//
//////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
// Layout at this node:
// (not including the mandatory consts and temps entries)
//
// Returned atp layout (to parent of ProbeCache):
//
// |-------------|   |------------------------------|
// | input data  |   | input data  |   output row   |
// | ( I tupps ) |   | ( I tupps ) |   ( 1 tupp )   |
// |-------------|   |------------------------------|
// <- input ATP ->   <-- returned ATP to parent ---->
//
// input data: the atp input to this node by its parent. 
// output row: optional tupp for cached inner table result.  Note 
//             that there is no output row (and no tupp) if
//             ProbeCache is on the right of a semi-join or
//             anti-semi-join.
//             
//
// ATP layout of queue entries between ProbeCache node and its child:
//
// |-------------|   |-----------------------------|
// | input data  |   | input data |  output row    |
// | ( I tupps ) |   | ( I tupps )|  ( J tupps )   |
// |-------------|   |-----------------------------|
// <ATP to child >   <-- returned ATP to parent --->
//
// input data: same as atp input from this node's parent.
// output row: optional inner table result returned from child. 
//             See note above about semi-join and anti-semi-join.
// 
//
// Work atp layout:
//
// |----------------------------------------------------------|
// | Constants|   Temps  | Probe Hash| Probe Row | Inner Row  |
// | (1 tupp) | (1 tupp) | (1 tupp)  | (1 tupp)  | (1 tupp)   |
// |----------------------------------------------------------|
//
// Constants:         the constants tupp from the given Atp.
// Temps:             the temps tupp from the given Atp.
// Probe Hash:        the target for the hash of the probe input hash
// Probe Row:         the target for the probe input encode expr.
// Inner Row:         the target for the inner row's move expr,
//                    if ProbeCache is not on the right of a 
//                    semi-join or anti-semi-join.
//
////////////////////////////////////////////////////////////////////////////


short ProbeCache::codeGen(Generator *generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  MapTable * last_map_table = generator->getLastMapTable();

  ex_cri_desc * given_desc
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returned_desc
    = new(space) ex_cri_desc(given_desc->noTuples() + 1, space);

  // cri descriptor for work atp has 5 entries:
  // entry #0 for const
  // entry #1 for temp
  // entry #2 for hash value of probe input data in Probe Cache Manager
  // entry #3 for encoded probe input data in Probe Cache Manager
  // enrry #4 for inner table row data in this operator's cache buffer
  Int32 work_atp = 1;
  ex_cri_desc * work_cri_desc = new(space) ex_cri_desc(5, space);
  unsigned short hashValIdx          = 2; 
  unsigned short encodedProbeDataIdx = 3;
  unsigned short innerRowDataIdx     = 4;

    // generate code for child tree, and get its tdb and explain tuple.
  child(0)->codeGen(generator);
  ComTdb * child_tdb = (ComTdb *)(generator->getGenObj());
  ExplainTuple *childExplainTuple = generator->getExplainTuple();


  //////////////////////////////////////////////////////
  // Generate up to 4 runtime expressions.
  //////////////////////////////////////////////////////

  // Will use child's char. inputs (+ execution count) for the next
  // two runtime expressions.
  ValueIdList inputsToUse = child(0).getGroupAttr()->getCharacteristicInputs();
  
  inputsToUse.insert(generator->getOrAddStatementExecutionCount());

  // Expression #1 gets the hash value of the probe input data
  ValueIdList hvAsList;

  // Executor has hard-coded assumption that the result is long, 
  // so add a Cast node to convert result to a long.

  ItemExpr *probeHashAsIe = new (generator->wHeap())
    HashDistPartHash(inputsToUse.rebuildExprTree(ITM_ITEM_LIST));

  probeHashAsIe->bindNode(generator->getBindWA());

  NumericType &nTyp = (NumericType &)probeHashAsIe->getValueId().getType();
  GenAssert(nTyp.isSigned() == FALSE,
            "Unexpected signed HashDistPartHash.");

  GenAssert(probeHashAsIe->getValueId().getType().supportsSQLnullLogical()
            == FALSE, "Unexpected nullable HashDistPartHash.");

  ItemExpr *hvAsIe = new (generator->wHeap()) Cast(
       probeHashAsIe, 
       new (generator->wHeap()) 
            SQLInt(generator->wHeap(), FALSE,   // false == unsigned.
                   FALSE    // false == not nullable.
                  ));

  hvAsIe->bindNode(generator->getBindWA());

  hvAsList.insert(hvAsIe->getValueId());

  ex_expr *hvExpr   = NULL;
  ULng32 hvLength;
  exp_gen->generateContiguousMoveExpr(
              hvAsList,
              0, // don't add convert node
              work_atp,
              hashValIdx,
              ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
              hvLength,
              &hvExpr);

  GenAssert(hvLength == sizeof(Lng32),
            "Unexpected length of result of hash function.");

  // Expression #2 encodes the probe input data for storage in 
  // the ProbeCacheManager.

  ValueIdList encodeInputAsList;

  CollIndex inputListIndex;
  for (inputListIndex = 0; 
       inputListIndex < inputsToUse.entries(); 
       inputListIndex++) {   

    ItemExpr *inputIe = 
         (inputsToUse[inputListIndex].getValueDesc())->getItemExpr();

    if (inputIe->getValueId().getType().getVarLenHdrSize() > 0)
      {
        // This logic copied from Sort::codeGen().
        // Explode varchars by moving them to a fixed field
        // whose length is equal to the max length of varchar.
        // 5/8/98: add support for VARNCHAR

        const CharType& char_type =
          (CharType&)(inputIe->getValueId().getType());

	if (!CollationInfo::isSystemCollation(char_type.getCollation()))
	{
	  inputIe = new(generator->wHeap())
              Cast (inputIe,
                    (new(generator->wHeap())
                      SQLChar(
		          generator->wHeap(), CharLenInfo(char_type.getStrCharLimit(), char_type.getDataStorageSize()),
                          char_type.supportsSQLnull(),
                          FALSE, FALSE, FALSE,
                          char_type.getCharSet(),
                          char_type.getCollation(),
                          char_type.getCoercibility()
                              )
                    )
                   );
	}
      }

    CompEncode * enode = new(generator->wHeap()) 
      CompEncode(inputIe, FALSE /* ascend/descend doesn't matter*/);

    enode->bindNode(generator->getBindWA());
    encodeInputAsList.insert(enode->getValueId());
  }

  ex_expr *encodeInputExpr = NULL;
  ULng32 encodedInputLength;
  exp_gen->generateContiguousMoveExpr(encodeInputAsList, 
                              0, //don't add conv nodes
                              work_atp, encodedProbeDataIdx,
                              ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
                              encodedInputLength, &encodeInputExpr);


  // Expression #3 moves the inner table data into a buffer pool.  
  // This is also the tuple returned to ProbeCache's parent. 

  ex_expr * innerRecExpr = NULL;
  ValueIdList innerTableAsList = getGroupAttr()->getCharacteristicOutputs();

  //////////////////////////////////////////////////////
  // Describe the returned row and add the returned 
  // values to the map table.
  //////////////////////////////////////////////////////

  // determine internal format
  NABoolean useCif = FALSE;
  ExpTupleDesc::TupleDataFormat tupleFormat = generator->getInternalFormat();
  //tupleFormat = determineInternalFormat( innerTableAsList, this, useCif,generator);

  ULng32 innerRecLength = 0;
  ExpTupleDesc * innerRecTupleDesc = 0;
  MapTable * returnedMapTable = NULL;

  exp_gen->generateContiguousMoveExpr(innerTableAsList, 
                              -1, // do add conv nodes 
			      work_atp, innerRowDataIdx,
			      tupleFormat,
			      innerRecLength, &innerRecExpr,
			      &innerRecTupleDesc, ExpTupleDesc::SHORT_FORMAT,
                              &returnedMapTable);

  returned_desc->setTupleDescriptor(returned_desc->noTuples() - 1, 
        innerRecTupleDesc);

  // remove all appended map tables and return the returnedMapTable
  generator->removeAll(last_map_table);
  generator->appendAtEnd(returnedMapTable);
  // This returnedMapTable will contain the value ids that are being returned 
  // (the inner table probed).

  // Massage the atp and atp_index of the innerTableAsList.
  for (CollIndex i = 0; i < innerTableAsList.entries(); i++)
    {
      ValueId innerValId = innerTableAsList[i];

      Attributes *attrib =
	generator->getMapInfo(innerValId)->getAttr();

      // All reference to the returned values from this point on
      // will be at atp = 0, atp_index = last entry in returned desc.
      attrib->setAtp(0);
      attrib->setAtpIndex(returned_desc->noTuples() - 1);
    }

  // Expression #4 is a selection predicate, to be applied
  // before returning rows to the parent

  ex_expr * selectPred = NULL;

  if (!selectionPred().isEmpty())
    {
      ItemExpr * selPredTree =
        selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(selPredTree->getValueId(),
                            ex_expr::exp_SCAN_PRED,
                            &selectPred);
    }

  //////////////////////////////////////////////////////
  // Prepare params for ComTdbProbeCache.
  //////////////////////////////////////////////////////

  queue_index pDownSize = (queue_index)getDefault(GEN_PROBE_CACHE_SIZE_DOWN);
  queue_index pUpSize   = (queue_index)getDefault(GEN_PROBE_CACHE_SIZE_UP);

  // Make sure that the ProbeCache queues can support the childs queues.
  if(pDownSize < child_tdb->getInitialQueueSizeDown()) {
    pDownSize = child_tdb->getInitialQueueSizeDown();
    pDownSize = MINOF(pDownSize, 32768);
  }
  if(pUpSize < child_tdb->getInitialQueueSizeUp()) {
    pUpSize = child_tdb->getInitialQueueSizeUp();
    pUpSize = MINOF(pUpSize, 32768);
  }

  ULng32 pcNumEntries = numCachedProbes_;
  
  // Number of entries in the probe cache cannot be less than 
  // max parent down queue size.  Before testing and adjusting the 
  // max queue size, it is necessary to make sure it is a power of
  // two, rounding up if necessary.  This is to match the logic in
  // ex_queue::resize.

  queue_index pdq2 = 1;
  queue_index bits = pDownSize;
  while (bits && pdq2 < pDownSize) {
    bits = bits  >> 1;
    pdq2 = pdq2 << 1;
  }
  if (pcNumEntries < pdq2)
    pcNumEntries = pdq2;

  numInnerTuples_ = getDefault(GEN_PROBE_CACHE_NUM_INNER);

  if (innerRecExpr == NULL)
    {
      // For semi-join and anti-semi-join, executor need not allocate
      // a buffer.  Set the tdb's buffer size to 0 to be consistent.
      numInnerTuples_ = 0;
    }
  else if (numInnerTuples_ == 0)
    {
      // Handle special value, 0, which tells code gen to 
      // decided on buffer size: i.e., large enough to accomodate
      // all parent up queue entries and all probe cache entries 
      // having a different inner table row.

      // As we did for the down queue, make sure the up queue size 
      // specified is a power of two.

      queue_index puq2 = 1;
      queue_index bits = pUpSize;
      while (bits && puq2 < pUpSize) {
        bits = bits  >> 1;
        puq2 = puq2 << 1;
      }
      numInnerTuples_ = puq2 + pcNumEntries;
    }
  else 
    {
      // Just use the default.  ExSimpleSqlBuffer takes care
      // that there is room enough for one row.
    }

   double  memoryLimitPerInstance =
      ActiveSchemaDB()->getDefaults().getAsLong(EXE_MEMORY_FOR_PROBE_CACHE_IN_MB) * 1024 * 1024;
   double estimatedMemory;
   
   if (numInnerTuples_ > 0) {
      estimatedMemory = numInnerTuples_ * innerRecLength;
      if (estimatedMemory > memoryLimitPerInstance) {
          numInnerTuples_ = memoryLimitPerInstance / innerRecLength;
          queue_index pUpSize_calc;
 
          pUpSize_calc = numInnerTuples_ ;
          queue_index pq2 = 1;
          queue_index bits = pUpSize_calc;
          while (bits && pq2 < pUpSize_calc) {
              bits = bits  >> 1;
              pq2 = pq2 << 1;
          }
          pUpSize = MINOF(pq2/2, 4); 
          pDownSize = MINOF(pq2/2, 4);
          pcNumEntries = MINOF(numInnerTuples_, 4);
          numInnerTuples_ = pcNumEntries;
          numCachedProbes_ = pcNumEntries;
      }
   } 


  ///////////////////////////////////////////////////////
  // Create and initialize the ComTdbProbeCache, generate
  // the explain tuple, set the new up queue cri desc 
  // and give the generator this newly generated object.
  ///////////////////////////////////////////////////////
  
  ComTdbProbeCache *probeCacheTdb = new (space) ComTdbProbeCache (
                 hvExpr,
		 encodeInputExpr,
		 innerRecExpr,
                 selectPred,
		 encodedInputLength,
		 innerRecLength,
                 pcNumEntries,
		 returned_desc->noTuples() - 1,
                 hashValIdx,
                 encodedProbeDataIdx,
                 innerRowDataIdx,
		 child_tdb,
		 given_desc,
		 returned_desc,
		 pDownSize,
		 pUpSize,
		 (Cardinality) 
                   (getInputCardinality() * getEstRowsUsed()).getValue(),
		 numInnerTuples_) ;

  generator->initTdbFields(probeCacheTdb);

  // Make sure that the ProbeCache queues were not set too large
  if ((probeCacheTdb->getInitialQueueSizeDown() > pDownSize) ||
      (probeCacheTdb->getInitialQueueSizeUp() > pUpSize)) {
    probeCacheTdb->setQueueResizeParams(pDownSize,
                                        pUpSize,
                                        probeCacheTdb->getQueueResizeLimit(),
                                        probeCacheTdb->getQueueResizeFactor());
  }

  double probeCacheMemEst = getEstimatedRunTimeMemoryUsage(probeCacheTdb);
  generator->addToTotalEstimatedMemory(probeCacheMemEst);
  Lng32 pcMemEstInKBPerNode = getEstimatedRunTimeMemoryUsage(TRUE).value() / 1024;
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(probeCacheTdb, childExplainTuple, 0, generator));
  }

  generator->setCriDesc(returned_desc, Generator::UP);

  generator->setGenObj(this, probeCacheTdb);

  return 0;
}

CostScalar ProbeCache::getEstimatedRunTimeMemoryUsage(NABoolean perNode, Lng32 *numStreams)
{
  const Lng32 probeSize = 
      getGroupAttr()->getCharacteristicInputs().getRowLength();
  const Lng32 numCacheEntries = 
    ActiveSchemaDB()->getDefaults().getAsLong(GEN_PROBE_CACHE_NUM_ENTRIES);
  const Int32 perProbeOverhead = 32 ; // bytes
  const double cacheSize = (probeSize + perProbeOverhead) * numCacheEntries;  // in bytes

  const Lng32 resultSize = 
      getGroupAttr()->getCharacteristicOutputs().getRowLength();
  Lng32 numBufferPoolEntries = 
    ActiveSchemaDB()->getDefaults().getAsLong(GEN_PROBE_CACHE_NUM_INNER);
  if (numBufferPoolEntries == 0)
  {
      numBufferPoolEntries = numCacheEntries + 
            ActiveSchemaDB()->getDefaults().getAsLong(GEN_PROBE_CACHE_SIZE_UP);
  }
  const double outputBufferSize = resultSize * numBufferPoolEntries;  // in bytes

  // totalMemory is perNode at this point of time.
  double totalMemory = cacheSize + outputBufferSize;
  Lng32 numOfStreams = 1;
  const PhysicalProperty* const phyProp = getPhysicalProperty();
  if (phyProp)
  {
     PartitioningFunction * partFunc = phyProp -> getPartitioningFunction() ;
     numOfStreams = partFunc->getCountOfPartitions();
     if (numOfStreams <= 0)
        numOfStreams = 1;
     double  memoryLimitPerInstance =
          ActiveSchemaDB()->getDefaults().getAsLong(EXE_MEMORY_FOR_PROBE_CACHE_IN_MB) * 1024 * 1024;
     if (totalMemory > memoryLimitPerInstance)
        totalMemory = memoryLimitPerInstance;          
     totalMemory *= numOfStreams;
  }
  if (numStreams != NULL)
     *numStreams = numOfStreams;
  if ( perNode == TRUE ) 
     totalMemory /= MINOF(MAXOF(((NAClusterInfoLinux*)gpClusterInfo)->getTotalNumberOfCPUs(), 1), numOfStreams);
  else
     totalMemory /= numOfStreams;
  return totalMemory;
}

double ProbeCache::getEstimatedRunTimeMemoryUsage(ComTdb * tdb)
{
  // tdb is ignored for ProbeCache because this operator
  // does not participate in the BMO quota system.
  Lng32 numOfStreams = 1;
  CostScalar totalMemory = getEstimatedRunTimeMemoryUsage(FALSE, &numOfStreams);
  totalMemory = totalMemory * numOfStreams ;
  return totalMemory.value();
}

