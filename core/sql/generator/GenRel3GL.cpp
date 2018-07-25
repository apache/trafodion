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
******************************************************************************
*
* File:         GenRel3GL.cpp
* Description:  Generate code for 3GL operators.
*               
* Created:      12/8/97
* Language:     C++
*
******************************************************************************
*/

#include "GroupAttr.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "ComTdbCompoundStmt.h"
#include "Rel3GL.h" 
#include "DefaultConstants.h"
#include "ExpCriDesc.h"

//////////////////////////////////////////////////////////////////////////////
//
// PhysCompoundStmt methods.
//
//////////////////////////////////////////////////////////////////////////////

RelExpr *PhysCompoundStmt::copyTopNode(RelExpr *derivedNode, CollHeap *oHeap)
{

  PhysCompoundStmt* result;

  if (derivedNode == NULL)
    result = new (oHeap) PhysCompoundStmt(NULL, NULL);
  else
    result = (PhysCompoundStmt*)derivedNode;

  return CompoundStmt::copyTopNode(result, oHeap);

} // PhysCompoundStmt::copyTopNode

short PhysCompoundStmt::codeGen(Generator * generator)
{
  ////////////////////////////////////////////////////////////////////////////
  //
  // Layout of row returned by this node.
  //
  // |-----------------------------------------------------|
  // | input data  | left child's data | right child's data|
  // | ( I tupps ) |  ( L tupps )      | ( R tupps )       |
  // |-----------------------------------------------------|
  //
  // <-- returned row from left ------->
  // <------------------ returned row from right ---------->
  //
  // input data:        the atp input to this node by its parent. 
  // left child data:   tupps appended by the left child
  // right child data:  tupps appended by right child
  //
  // Returned row to parent contains:
  //
  //   I + L + R tupps.
  //
  ////////////////////////////////////////////////////////////////////////////

  GenAssert((child(0) AND child(1)), "PhysCompoundStmt: missing one child");

  // Get handels and init.
  Space *space = generator->getSpace();
  ex_cri_desc *givenDesc = generator->getCriDesc(Generator::DOWN);

  ComTdb *leftTdb = NULL;
  ComTdb *rightTdb = NULL;

  ex_cri_desc *leftCRI = NULL;
  ex_cri_desc *rightCRI = NULL;
  ex_cri_desc *returnedCRI = NULL;

  ExplainTuple *leftExplainTuple = NULL;
  ExplainTuple *rightExplainTuple = NULL;

  // Generate code for left child.
  if (child(0))
  {
    child(0)->codeGen(generator);
    leftTdb = (ComTdb *)(generator->getGenObj());
    leftExplainTuple = generator->getExplainTuple();
    leftCRI = generator->getCriDesc(Generator::UP);
  }

  // if an update operation is found in the left subtree of this CS then
  // set rowsFromLeft to 0 which is passed on to execution tree indicating
  // that this CS node is not  expecting rows from the left child, then
  // foundAnUpdate_ is reset so it can be reused while doing codGen() on 
  // the right sub tree

  NABoolean rowsFromLeft = TRUE;

  if (generator->foundAnUpdate())
  {
    rowsFromLeft = FALSE;
    generator->setFoundAnUpdate(FALSE);
  }

  // if an update operation is found during or before the execution of the
  // left subtree, set afterUpdate to 1 indicating that an update operation
  // was performed before or during the execution of the left child.  Which 
  // is used at runtime to decide whether to set rollbackTransaction in the 
  // diagsArea

  NABoolean afterUpdate = FALSE;

  if (generator->updateWithinCS()) {
    afterUpdate = TRUE;
  }

  // Generate code for right child.
  if (child(1)) 
  {
    generator->setCriDesc(leftCRI, Generator::DOWN);
   
    child(1)->codeGen(generator);
    rightTdb = (ComTdb *)(generator->getGenObj());
    rightExplainTuple = generator->getExplainTuple();
    rightCRI = generator->getCriDesc(Generator::UP);
  }

  // if an update operation is found in the right subtree of this CS then
  // set rowsFromRight to 0 which is passed on to execution tree indicating
  // that this CS node is not  expecting rows from the right child, then
  // foundAnUpdate_ is reset so it can be reused while doing codGen() on
  // the left or right child of another CS node

  NABoolean rowsFromRight = TRUE;

  if (generator->foundAnUpdate())
  {
    rowsFromRight = FALSE;
    generator->setFoundAnUpdate(FALSE);
  }

  // Soln 10-050728-0208
  // Set the foundAnUpdate flag in the generator if there is an update in either
  // of the children.
  // if ((rowsFromLeft == 0) && (rowsFromRight == 0)) {
  if ((rowsFromLeft == 0) || (rowsFromRight == 0)) {
    generator->setFoundAnUpdate(TRUE);
  }

   
  // Create returned CRI desc and CompoundStmt TDB.
  returnedCRI = new(space) ex_cri_desc(rightCRI->noTuples(), space);

  ComTdbCompoundStmt *tdb = 
    new(space) ComTdbCompoundStmt(leftTdb, rightTdb,
			         givenDesc, returnedCRI,
			         (queue_index)getDefault(GEN_CS_SIZE_DOWN),
			         (queue_index)getDefault(GEN_CS_SIZE_UP),
			         getDefault(GEN_CS_NUM_BUFFERS), 
			         getDefault(GEN_CS_BUFFER_SIZE),
                                 rowsFromLeft,
                                 rowsFromRight,
                                 afterUpdate);
  generator->initTdbFields(tdb);

  // Add expain info.
  if (!generator->explainDisabled()) 
  {
    generator->setExplainTuple(
      addExplainInfo(tdb, leftExplainTuple, rightExplainTuple, generator));
  }

  // Restore the original down CRI desc.
  generator->setCriDesc(givenDesc, Generator::DOWN);

  // Set the new up CRI desc.
  generator->setCriDesc(returnedCRI, Generator::UP);
  
  generator->setGenObj(this, tdb);

  return 0;

} // PhysCompoundStmt::codeGen
