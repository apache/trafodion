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
// code that is not compiled, but for some reason is reported
// just excluding the entire file
#include "AppliedStatMan.h"
#include "Analyzer.h"

void QueryAnalysis::initializeQueryGraphs()
{
  //FILE * f = stdout;

  //fprintf(f,"### InitializingQueryGraphs \n");

  //This method should be called after ASM initialization
  AppliedStatMan * appStatMan = ASM();

  if (appStatMan == NULL)
    return;

  ARRAY(JBB *) allJBBs = getJBBs();
  CollIndex remainingJBBs = allJBBs.entries();
  for (CollIndex i = 0; remainingJBBs > 0; i++)
  {
    if (allJBBs.used(i))
    {
      (allJBBs[i])->initializeQueryGraph();
    remainingJBBs--;
    }
  }
}

void JBB::initializeQueryGraph()
{
  //FILE * f = stdout;

  //fprintf(f,"### Enter QueryGraph\n");
  //iterate over each JBBC in the JBB

#ifndef NDEBUG
  if(0){
    debugQueryGraph();
  }
#endif

  JBBSubset allJBBCs = getMainJBBSubset();
  CANodeIdSet JBBCs = allJBBCs.getJBBCs();

  for(CANodeId JBBCId = JBBCs.init(); JBBCs.next(JBBCId); JBBCs.advance(JBBCId))
  {
    JBBC * currentJBBC = (JBBCId.getNodeAnalysis())->getJBBC();
    //initialize the Query Graph aspect of currentJBBC
    currentJBBC->initializeQueryGraphNode();
  }

  //enumerate reduction paths
  enumerateReductionPaths();
}
//JBB::initializeQueryGraph()

void JBB::enumerateReductionPaths()
{
  //FILE * f = stdout;

  //get list of reducers
  ReducerStat * reducers = getReducerStat();

  //get a list of local leaf reducers
  CANodeIdSet localLeafReducers = reducers->getLocalLeafReducers();
    //fprintf(f,"### Local Leaf reducers\n");
    //localLeafReducers.print();

  //iterate over all local leaf reducers
  CANodeId JBBCId;
  for(JBBCId = localLeafReducers.init();
      localLeafReducers.next(JBBCId);
      localLeafReducers.advance(JBBCId))
  {
    JBBC * currentJBBC = (JBBCId.getNodeAnalysis())->getJBBC();
    //start linear reduction paths from currentJBBC
    currentJBBC->startReductionPaths();
  }

  //get a list of local non leaf reducers
  CANodeIdSet localNonLeafReducers = reducers->getLocalNonLeafReducers();
    //fprintf(f,"### Local Non Leaf reducers\n");
    //localNonLeafReducers.print();

  //iterate over all local non leaf reducers
  for(JBBCId = localNonLeafReducers.init();
      localNonLeafReducers.next(JBBCId);
      localNonLeafReducers.advance(JBBCId))
  {
    JBBC * currentJBBC = (JBBCId.getNodeAnalysis())->getJBBC();
    //start linear reduction paths from currentJBBC
    currentJBBC->startReductionPaths();
  }


  //get a list of join leaf reducers
  CANodeIdSet  joinLeafReducers = reducers->getJoinLeafReducers();
    //fprintf(f,"### Join Leaf reducers\n");
    //joinLeafReducers.print();

  //iterate over all join leaf reducers
  //put this back in after ASM starts providing join reducers
  for(JBBCId = joinLeafReducers.init();
      joinLeafReducers.next(JBBCId);
      joinLeafReducers.advance(JBBCId))
  {
    JBBC * currentJBBC = (JBBCId.getNodeAnalysis())->getJBBC();
    //start linear reduction paths from currentJBBC
    currentJBBC->startReductionPaths();
  }


  //get a list of join non leaf reducers
  CANodeIdSet joinNonLeafReducers = reducers->getJoinNonLeafReducers();
    //fprintf(f,"### Join Leaf reducers\n");
    //joinNonLeafReducers.print();

  //iterate over all join non leaf reducers
  for(JBBCId = joinNonLeafReducers.init();
      joinNonLeafReducers.next(JBBCId);
      joinNonLeafReducers.advance(JBBCId))
  {
    JBBC * currentJBBC = (JBBCId.getNodeAnalysis())->getJBBC();
    //start linear reduction paths from currentJBBC
    currentJBBC->startReductionPaths();
  }

  //iterate over each JBBC in the JBB
  JBBSubset allJBBCs = getMainJBBSubset();
    CANodeIdSet JBBCs = allJBBCs.getJBBCs();

  for(JBBCId = JBBCs.init(); JBBCs.next(JBBCId); JBBCs.advance(JBBCId))
  {
    JBBC * currentJBBC = (JBBCId.getNodeAnalysis())->getJBBC();
    //initialize the Query Graph aspect of currentJBBC
    currentJBBC->mergeReductionPaths();
  }

}
//JBB::enumerateReductionPaths()

void JBB::displayReductionPaths()
{
  FILE * f = stdout;

  fprintf(f,"All reduction paths in this JBB\n");

  JBBSubset allJBBCs = getMainJBBSubset();
  CANodeIdSet JBBCs = allJBBCs.getJBBCs();

  for(CANodeId JBBCId = JBBCs.init(); JBBCs.next(JBBCId); JBBCs.advance(JBBCId))
  {
    JBBC * currentJBBC = (JBBCId.getNodeAnalysis())->getJBBC();
    //initialize the Query Graph aspect of currentJBBC
    currentJBBC->displayQueryGraphNodeText();
  }
}

#ifndef NDEBUG
#pragma nowarn(770)  // warning elimination
void JBB::debugQueryGraph(){
  //get integer representing the reduction path to be built
  Lng32 reductionPathString=0;

  CANodeIdSet currentReductionPath;
  EstLogPropSharedPtr currentReductionPathProps;

  //add the first JBBC to the reduction path, before extending the
  //reduction path by doing a join with the second JBBC.
  if(reductionPathString > 0){
    //get the first JBBC
    Int32 firstJBBCNodeId = reductionPathString % 100;
    //update reductionPathString now that we have extracted the last two digits
    reductionPathString = reductionPathString / 100;
    //get the first JBBC and insert into the empty reduction path
    CANodeId firstJBBC(firstJBBCNodeId);
    currentReductionPath.insert(firstJBBC);
    currentReductionPathProps = (QueryAnalysis::ASM())->\
      getStatsForCANodeIdSet(currentReductionPath);
  }

  //loop over the reductionPathString joining in a JBBC at a time
  //each JBBC is represent by a two digit number that gives its id
  while (reductionPathString > 0){
    //extend currentReductionPath by joining to next JBBC in the
    //reductionPathString

    //first get the CANodeId of the JBBC to join to the currentReductionPath
    Int32 jBBCToJoinId = reductionPathString % 100;
    //update reductionPathString now that we have extracted the last two digits
    reductionPathString = reductionPathString / 100;
    CANodeId jBBCToJoin(jBBCToJoinId);
    CANodeIdSet jBBCToJoinSet(jBBCToJoin);

    //extend the reduction path by doing a join of the current reduction path
    //with jBBCToJoin
    EstLogPropSharedPtr joinEstLogProp = (QueryAnalysis::ASM())->\
      joinJBBChildren(currentReductionPath, jBBCToJoinSet);

    //insert join JBBC into current reduction path
    currentReductionPath.insert(jBBCToJoin);

    //update currentReductionPath
    currentReductionPathProps = (QueryAnalysis::ASM())->\
      getStatsForCANodeIdSet(currentReductionPath);
  }

}
#pragma warn(770)  // warning elimination
#endif

const NAString JBBC::getQueryGraphNodeText() const{
  NAString result("QueryGraph Info for node "+ getId().getText()+"\n");

  //iterator over all the reduction paths at this node.
  CollIndex i;
  for(i=0; i < reductionPaths_->entries();i++)
  {
    result += ("Reduction Path " + istring(i) + "\n");

    //print the reduction path
    result += (*reductionPaths_)[i]->getText();

#pragma warning (disable : 4244)  //warning elimination
#pragma nowarn(1506)   // warning elimination 
    Int32 cardinality = ((*cardinalityOfReductionPaths_)[i]).getValue();
#pragma warn(1506)  // warning elimination 
#pragma warning (default : 4244)  //warning elimination
    result += ("cardinality: " + istring(cardinality)+"\n");
  }

  return result;
}

void JBBC::printQueryGraphNodeText (FILE *f,
  const char * prefix,
  const char * suffix) const
{
#ifndef NDEBUG
  fprintf (f, getQueryGraphNodeText());
#endif
};

void JBBC::displayQueryGraphNodeText() const
{
  printQueryGraphNodeText();
};

void JBBC::initializeQueryGraphNode()
{
  //Allocate the list used to store reduction paths
  //use whatever heap the JBBC is on
  reductionPaths_ = new (heap_) NAList<CANodeIdSet *>(heap_);

  cardinalityOfReductionPaths_ =  new (heap_) NAList<CostScalar>(heap_);
  //create a list of QueryGraphConnections where each connection represents
  //a incoming connection
  CANodeIdSet connectedJBBCs = getJoinedJBBCs();

  //Allocate the list that will store all incoming connections
#pragma nowarn(1506)   // warning elimination 
  Int32 numIncomingConnections = connectedJBBCs.entries();
#pragma warn(1506)  // warning elimination 
  incomingConnections_ = new (heap_) NAList<QueryGraphConnection *>(heap_,numIncomingConnections);

  //create a QueryGraphConnection object representing a connection from
  //each of the connected JBBCs to 'this' JBBC.
  for(CANodeId JBBCId = connectedJBBCs.init();
      connectedJBBCs.next(JBBCId);
      connectedJBBCs.advance(JBBCId))
  {
    //create a QueryGraphConnection Object representing a connection
    //from JBBC with id JBBCId to 'this' JBBC
    QueryGraphConnection * newConnection =
      new (heap_) QueryGraphConnection(getId(), JBBCId);
    incomingConnections_->insert(newConnection);
  }

  //set the after local predicate cardinality of this node
  CANodeIdSet localNodeId(this->getId());
  afterLocalPredCardinality_ = ((QueryAnalysis::ASM())->getStatsForCANodeIdSet(localNodeId))->getResultCardinality();
}
//JBBC::initializeQueryGraphNode()


void JBBC::extendReductionPath(CANodeIdSet currentReductionPath,
                               CostScalar cardinalityOfCurrentReductionPath,
                               CANodeId from)
{
  //Node Id set representing this JBBC
  //CANodeIdSet thisJBBC(getId());

  //variable to get logical properties for the current reduction path
  //EstLogPropSharedPtr logPropsOfCurrentReductionPath;

  //check if we are not starting a reduction path i.e. we are extending a
  //reduction path that has already been started. A reduction path is started
  //by calling startReductionPaths(), which passes NULL_CA_ID in parameter
  //'from'
  if(from != NULL_CA_ID)
  {
    //store the reduction path passed in
    CANodeIdSet * reductionPathToStore = new (collHeap()) CANodeIdSet(currentReductionPath);
    reductionPaths_->insert(reductionPathToStore);

    //find the connection from which this reduction path is coming in
    CollIndex i;
    for(i = 0; i < incomingConnections_->entries(); i++)
    {
      if((*incomingConnections_)[i]->getConnectedJBBC() == from)
      {
        //add the latest connection to the QueryGraphConnection Object
        (*incomingConnections_)[i]->addIncomingReductionPath(reductionPathToStore,
                                    cardinalityOfCurrentReductionPath);
         break;
      }
    }

    //insert cardinality into array of cardinalities this will be used by
    //getReducingJoinSet to get cardinalities without the need to lookup the
    //cache.
    cardinalityOfReductionPaths_->insert(cardinalityOfCurrentReductionPath);
  }

  //add this node's JBBCId to the reduction path, since it has to be extended.
  //This is the reduction path that will be joined to the connected JBBCs (i.e.
  //extended).
  currentReductionPath.insert(getId());


  //try to extend the reduction path to each connected JBBC except the
  //JBBC where  currentReductionPath came from
  //get all the connected JBBCs
  CANodeIdSet connectedJBBCs = getJoinedJBBCs();

  //iterate over all the connected JBBCs, trying to extend the reduction path
  //to each connected JBBC except the one from which currentReductionPath
  //came from
  CANodeId JBBCId;
  for(JBBCId = connectedJBBCs.init();
      connectedJBBCs.next(JBBCId);
      connectedJBBCs.advance(JBBCId))
  {
    //check if current JBBC is not the JBBC from which currentReductionPath
    //came from and that currentJBBC is not already in the reduction path.
    //By checking if the JBBC we are trying to extend the reduction path to,
    //is not already in the reduction path, we avoid cycles.
    if((JBBCId != from) && (!currentReductionPath.contains(JBBCId)))
    {
      //get a handle to the connected JBBC
      JBBC * connectedJBBC = JBBCId.getNodeAnalysis()->getJBBC();


      //only try extending reduction path if cardinality of the connected
      //table is at least 10% more than the cardinality of the reduction
      //path to extend
      if((cardinalityOfCurrentReductionPath * 1.1) <
          connectedJBBC->getAfterLocalPredCardinality())
      {
        //Do a join of currentReductionPath with the JBBC represented by JBBCId
        CANodeIdSet currentJBBC(JBBCId);
        EstLogPropSharedPtr joinEstLogProp = (QueryAnalysis::ASM())->joinJBBChildren(currentReductionPath, currentJBBC);

        CostScalar joinCardinality = joinEstLogProp->getResultCardinality();

        //if the cardinality of the join is atleast 5 % lower than the
        //after local predicate cardinality of the connected JBBC,
        //extend the reduction path
        if(joinCardinality < (connectedJBBC->getAfterLocalPredCardinality()*.95))
        {
          connectedJBBC->extendReductionPath(currentReductionPath, joinCardinality,getId());
        }
      }

    }
  }
}
//JBBC::extendReductionPath()

CostScalar JBBC::getAfterLocalPredCardinality()
{
  return afterLocalPredCardinality_;
};
//JBBC::getAfterLocalPredCardinality()

NABoolean JBBC::mergeReductionPaths()
{
  if((incomingConnections_->entries() < 3)&&
     (reductionPaths_->entries() < 2))
  return FALSE;

  NABoolean returnValue=FALSE;

  //create CANodeIdSet to hold mergedReductionPath
  //initially the set is empty because nothing has been merged
  CANodeIdSet mergedReductionPath;

    //create CANodeIdSet to keep track of connections which are
    //merged. Initially empty because no connections have been merged
    CANodeIdSet mergedConnections;

  //iterate over all the connections
  CollIndex i;
  for(i=0; i < incomingConnections_->entries(); i++)
  {
    //Get the most reducing reduction path coming through this connection
    const CANodeIdSet * bestIncomingReductionPath =
      (*incomingConnections_)[i]->getBestIncomingReductionPath();

    //If this connection has a reduction path coming over it
    if(bestIncomingReductionPath){
      CANodeIdSet tempNodeSet = *bestIncomingReductionPath;
      //Join mergedReductionPath with bestIncomingReductionPath
      EstLogPropSharedPtr joinEstLogProp =
        (QueryAnalysis::ASM())->joinJBBChildren(mergedReductionPath, tempNodeSet);

      //update the merged reduction path to include JBBCs in the path just merged
      mergedReductionPath.insert(*bestIncomingReductionPath);

      //update mergedConnections set to indicate the reduction path
      //from this connenction (i.e. incomingConnections[i]) was merged
      mergedConnections.insert((*incomingConnections_)[i]->getConnectedJBBC());
    }
  }


  //if a merge of two or more reduction paths occurred
  if(mergedConnections.entries() > 1)
  {
    //do a join of the merged reduction path with this JBBC to see if
    //the merged reduction path causes a significant reduction in this
    //JBBC
    CANodeIdSet thisJBBC;
    thisJBBC.insert(getId());
    EstLogPropSharedPtr joinEstLogProp = (QueryAnalysis::ASM())->joinJBBChildren(mergedReductionPath, thisJBBC);

    CostScalar mergedReductionPathCardinality = joinEstLogProp->getResultCardinality();

    //if merged path produces at least a 5% reduction in cardinality of
    //this JBBC
    if(mergedReductionPathCardinality <
        (getAfterLocalPredCardinality() * .95))
    {
      returnValue = TRUE;
      //insert merged reduction path in the list of reduction paths
      //store the reduction path passed in
      //store the reduction path passed in
      CANodeIdSet * reductionPathToStore = new (collHeap())\
                                           CANodeIdSet(mergedReductionPath);
      reductionPaths_->insert(reductionPathToStore);

      //insert cardinality into array of cardinalities this will be used by
      //getReducingJoinSet to get cardinalities without the need to lookup the
      //cache.
      cardinalityOfReductionPaths_->insert(mergedReductionPathCardinality);

      //insert this nodes id into merged reduction path as it is to be extended
      //to connected JBBCs now.
      mergedReductionPath.insert(getId());

      //iterate over each connection, if a connection did not
      //participate in the merge try to extend the reduction path
      //to that connected JBBC. The connectedJBBC will be used to
      //identify the connection.
      for(i=0; i < incomingConnections_->entries(); i++)
      {
        //Get CANodeId of connected JBBC
        CANodeId connectedNodeId = ((*incomingConnections_)[i])->\
                                                           getConnectedJBBC();
        //Check to see that this is not a connected node from where one of the
        //merged reduction paths come.
        //Also check if this node is already part of the merged reduction path
        //this avoids cycles
        if((!mergedConnections.contains(connectedNodeId))&&
           (!mergedReductionPath.contains(connectedNodeId)))
        {
          //Get reference to connected JBBC
          JBBC * connectedJBBC =
           (connectedNodeId.getNodeAnalysis())->getJBBC();

          //Do a join of the merged reduction path with the connected JBBC
          CANodeIdSet connectedNode(connectedNodeId);
          joinEstLogProp = (QueryAnalysis::ASM())->\
                           joinJBBChildren(mergedReductionPath, connectedNode);

          CostScalar joinCardinality = joinEstLogProp->getResultCardinality();

          //only extend merged reduction path to a connected JBBC
          //if cardinality of connected JBBC is 10% more than the
          //cardinality of the reduction path to extend
          if((mergedReductionPathCardinality*1.1) <
            connectedJBBC->getAfterLocalPredCardinality())
          {
            //if the cardinality of the join is atleast 5 % lower than the
            //after local predicate cardinality of the connected JBBC,
            //extend the reduction path
            if(joinCardinality < (connectedJBBC->getAfterLocalPredCardinality()*.95))
            {
              //Extend merged redution path to connected JBBC
              connectedJBBC->\
                extendReductionPath(mergedReductionPath, joinCardinality,getId());
            }
          }
        }
      }
    }

  }

  return returnValue;
};
//JBBC::mergeReductionPaths()

JBBSubset * JBBC::getReducingJoinSet(CANodeIdSet & availableJBBCs,
                                     CANodeIdSet * usedSet)
{
  //cardinality produced by the best reducing join set.
  CostScalar minCardinality = getAfterLocalPredCardinality();

  //the best reducing join set as computed till now
  CANodeIdSet * currentBestRJS = NULL;

  //if no recomputation required
  if(!usedSet){
    //iterator over all the reduction paths at this node.
    CollIndex i;
    for(i=0; i < reductionPaths_->entries();i++)
    {
      //check if the current reduction path is a subset of the
      //availableJBBCs
      if(availableJBBCs.contains(*(*reductionPaths_)[i]))
      {
        //get cardinaliy of current reduction path
        CostScalar cardinalityOfCurrentReductionPath =
          (*cardinalityOfReductionPaths_)[i];

          //if cardinality of current reduction path is lower than
          //minCardinality i.e. current reduction path produces a
          //greater reduction.
          if(cardinalityOfCurrentReductionPath < minCardinality)
          {
          //set currentBestRJS to current reduction path
          currentBestRJS = (*reductionPaths_)[i];
          //set minCardinality to cardinality of current reduction
          //path.
          minCardinality = cardinalityOfCurrentReductionPath;
        }
      }
    }
  }
  //recomputation is required
  else
  {
    //Set available and used JBBCs
    CANodeIdSet extendedAvailableJBBCs = availableJBBCs;
    extendedAvailableJBBCs += *usedSet;

    //iterate over all the reduction paths at this node.
    CollIndex i;
    for(i=0; i<reductionPaths_->entries();i++)
    {
      if(extendedAvailableJBBCs.contains(*(*reductionPaths_)[i]))
      {
        //get cardinaliy of current reduction path
        CostScalar cardinalityOfCurrentReductionPath =
          (*cardinalityOfReductionPaths_)[i];

        //if cardinality of current reduction path is lower than
        //minCardinality i.e. current reduction path produces a
        //greater reduction.
        if(cardinalityOfCurrentReductionPath < minCardinality)
        {
          if(currentBestRJS && availableJBBCs.contains(*currentBestRJS))
          {
            //set currentBestRJS to current reduction path
            currentBestRJS = (*reductionPaths_)[i];
            //set minCardinality to cardinality of current reduction
            //path.
            minCardinality = cardinalityOfCurrentReductionPath;
          }
          //recomputation is required for this reduction path
          else{
            //JBBCs that are part of the reduction path and have
            //to be exclude from the used set before doing a join
            //to recompute reduction path
            CANodeIdSet elementsToRemove = *(*reductionPaths_)[i];
            elementsToRemove.intersectSet(availableJBBCs);

            //JBBCs that have to be combined (joined) to the RP
            //in a recomputed reduction path.
            CANodeIdSet recomputationSet = *usedSet;
            recomputationSet.subtractSet(elementsToRemove);

            //use ASM to figure out the cardinality produced by
            //combining recomputationSet with the current reduction
            //path
            CANodeIdSet currentReductionPath = *(*reductionPaths_)[i];
            //add this JBBC to current reduction path since we want the
            //cardinality of join between the recomputed reduction path
            //and this JBBC.
            currentReductionPath.insert(getId());

            EstLogPropSharedPtr recomputedLogProps =
                          (QueryAnalysis::ASM())->joinJBBChildren(currentReductionPath,
                                                                 recomputationSet);
            CostScalar recomputedCardinality = recomputedLogProps->getResultCardinality();

            if(recomputedCardinality < minCardinality)
            {
              //set currentBestRJS to recomputed reduction path
              currentBestRJS = new (CmpCommon::statementHeap()) CANodeIdSet(*(*reductionPaths_)[i]);
              currentBestRJS->insert(recomputationSet);

              //set minCardinality to cardinality of current reduction
              //path.
              minCardinality = recomputedCardinality;
            }
          }
        }
      }
    }
  }

  //JBBSubset for returning
  JBBSubset * bestRJS=NULL;

  //if a RJS was found return it as a JBBSubset
  if(currentBestRJS)
  {
    //create JBBSubset
    bestRJS = new (CmpCommon::statementHeap()) JBBSubset();
    //add elements from currentBestRJS
    bestRJS->addJBBCs(*currentBestRJS);
    }

  return bestRJS;
};
//JBBC::getReducingJoinSet

QueryGraphConnection::QueryGraphConnection(CANodeId targetJBBC,
                                           CANodeId connectedJBBC,
                                           CollHeap *outHeap)
{
  targetJBBC_ = targetJBBC;

  connectedJBBC_ = connectedJBBC;

  bestIncomingReductionPath_ = NULL;

  cardinalityOfBestIncomingReductionPath_ = 0;

  heap_ = outHeap;

};
//QueryGraphConnection::QueryGraphConnection

NABoolean QueryGraphConnection::addIncomingReductionPath
                         (CANodeIdSet * incomingReductionPath,
                        CostScalar cardinalityOfIncomingReductionPath)
{

  //if this is the first reduction path coming in over this connection
  if(!bestIncomingReductionPath_)
  {
    //incoming reduction path is the best reduction path as it is the
    //only reduction path coming over this connection till now
    bestIncomingReductionPath_ = incomingReductionPath;

    //set the cardinality of the best incoming reduction path
    cardinalityOfBestIncomingReductionPath_ =
      cardinalityOfIncomingReductionPath;

    //return TRUE indicating bestIncomingReductionPath_ was updated
    return TRUE;
  }
  else
  {
    if(cardinalityOfIncomingReductionPath <
       cardinalityOfBestIncomingReductionPath_)
       {
         //incoming reduction path is the best reduction path as it has a
         //lower cardinality than the current best reduction path
         bestIncomingReductionPath_ = incomingReductionPath;

         //update the cardinality of the best incoming reduction path
         cardinalityOfBestIncomingReductionPath_ =
           cardinalityOfIncomingReductionPath;

         //return TRUE indicating bestIncomingReductionPath_ was updated
           return TRUE;
       }
  }

  //bestIncomingReductionPath_ was not updated
  return FALSE;
};
//QueryGraphConnection::addIncomingReductionPath

const CANodeIdSet * QueryGraphConnection::getBestIncomingReductionPath()
{
  return bestIncomingReductionPath_;
}
//QueryGraphConnection::getBestIncomingReductionPath
