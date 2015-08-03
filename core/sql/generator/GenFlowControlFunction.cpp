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
//
// This file contains the implementations for ::preCodeGen and ::codeGen 
// for the item expression subclasses related to flow control. This 
// currently includes ItmDoWhileFunction, ItmBlockFunction,
// ItmPersistentExpressionVar and ItmWhileFunction.
//

// Includes
//
#include "ItemExpr.h"
#include "ItmFlowControlFunction.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "exp_clause_derived.h"

// ItmDoWhileFunction::preCodeGen
//
// Nothing to do in preCodeGen for DoWhile.
//
ItemExpr *ItmDoWhileFunction::preCodeGen(Generator *generator) {
  return ItemExpr::preCodeGen(generator);
}

// ItmDoWhileFunction::codeGen
//
// The DoWhile function executes the code represented by child(0) until 
// the condition represented by child(1) becomes false. The result of 
// the DoWhile is the final value of child(0).
//
// The looping is accomplished by inserting a NOOP/BRANCH pair. The NOOP
// is inserted before generating the code for either child and serves as a 
// branch target. The BRANCH is inserted after generating the code for
// both children and is targeted at the NOOP clause based on the result of
// child(1). Between the NOOP and the BRANCH the body of the loop (child(0)) 
// and the termination condition (child(1)) are repeatedly evaluated. After
// the branch the final result of the loop body is assigned as the result
// of the DoWhile.
//
short ItmDoWhileFunction::codeGen(Generator * generator) {
  // Get local handles...
  //
  Attributes **attr;
  Space* space = generator->getSpace();
  CollHeap *wHeap = generator->wHeap();
  ExpGenerator *exp = generator->getExpGenerator();

  // If this DoWhile has already been codeGenned, then bug out...
  // Otherwise, allocate space for the result if necessary and set
  // attr[0] to point to the result attribute data. Also, mark this
  // node as codeGenned.
  //
  if (exp->genItemExpr(this, &attr, 2, 0) == 1)
    return 0;

  // Insert the NOOP clause to use as the branch target for the
  // start of the loop body.
  //
  ex_clause * branchTarget = new(space) ex_noop_clause();
  exp->linkClause(this, branchTarget);

  // CodeGen the body of the loop.
  //
  child(0)->codeGen(generator);

  // The result of the DoWhile is the result of the body of the loop. Set
  // the src attribute for the convert (added below) to be the body of
  // the while loop. The dst attribute has already been set in genItemExpr().
  //
  attr[1] = generator->getMapInfo
    (child(0)->castToItemExpr()->getValueId())->getAttr();

  // CodeGen the loop termination condition.
  //
  child(1)->codeGen(generator);

  // Construct a BRANCH clause to loop back and repeat the expression
  // and condition if the condition evaluates to TRUE.
  //
  Attributes ** branchAttrs = new(wHeap) Attributes*[2];
  Attributes *boolAttr = generator->getMapInfo
    (child(1)->castToItemExpr()->getValueId())->getAttr();
  branchAttrs[0] = boolAttr->newCopy(wHeap);
  branchAttrs[1] = boolAttr->newCopy(wHeap);
  //  branchAttrs[0]->copyLocationAttrs(boolAttr);
  //  branchAttrs[1]->copyLocationAttrs(boolAttr);

  branchAttrs[0]->resetShowplan();
  ex_branch_clause * branchClause
    = new(space) ex_branch_clause(ITM_OR, branchAttrs, space);
  branchClause->set_branch_clause(branchTarget);

  // Insert the branch clause into the expression.
  //
  exp->linkClause(this, branchClause);

  // Allocate a convert clause to move the result from child(0) to the
  // result of this node. This move is necessary so that future
  // side-effects of the result of child(0) -- if it is a local variable,
  // for instance -- will not change the result of the DoWhile.
  //
  ex_conv_clause * convClause =
    new(generator->getSpace()) ex_conv_clause
    (getOperatorType(), attr, space);
  exp->linkClause(this, convClause);

  return 0;
}

// ItmBlockFunction::preCodeGen
//
// Nothing to do in preCodeGen for Block.
//
ItemExpr *ItmBlockFunction::preCodeGen(Generator *generator) {
  return ItemExpr::preCodeGen(generator);
}

// ItmBlockFunction::codeGen
//
// The Block function executes the code represented by both its children and
// then returns the result of the right child (child(1)).
//
short ItmBlockFunction::codeGen(Generator * generator) {
  // Get local handles...
  //
  Attributes **attr;
  Space* space = generator->getSpace();
  CollHeap *heap = generator->wHeap();
  ExpGenerator *exp = generator->getExpGenerator();

  // If this Block has already been codeGenned, then bug out...
  // Otherwise, allocate space for the result if necessary and set
  // attr[0] to point to the result attribute data. Also, mark this
  // node as codeGenned.
  //
  if (exp->genItemExpr(this, &attr, 2, 0) == 1)
    return 0;

  // CodeGen the left child.
  //
  child(0)->codeGen(generator);

  // CodeGen the right child.
  //
  child(1)->codeGen(generator);

  // The result of the Block is the result of the right child. Set
  // the src attribute for the convert (added below) to be the right child
  // The dst attribute has already been set in genItemExpr().
  //
  attr[1] = generator->getMapInfo
    (child(1)->castToItemExpr()->getValueId())->getAttr();

  // Allocate a convert clause to move the result from child(1) to the
  // result of this node. This move is necessary so that future
  // side-effects of the result of child(1) -- if it is a local variable,
  // for instance -- will not change the result of the Block.
  //
  ex_conv_clause * convClause =
    new(generator->getSpace()) ex_conv_clause
    (getOperatorType(), attr, space);
  generator->getExpGenerator()->linkClause(this, convClause);

  return 0;
}

// ItmPersistentExpressionVar::codeGen
//
// Adds the persistent variable to the expression generator.
//
short ItmPersistentExpressionVar::codeGen(Generator * generator) {

  // If the variable has already been codeGenned, bug out...
  //
  MapInfo * mi = generator->getMapInfoAsIs(getValueId());
  if (mi && mi->isCodeGenerated()) return 0;

  // Otherwise, generate the code and add it to the map table.
  //
  generator->getExpGenerator()->addPersistent(getValueId(), 
					      generator->getMapTable());

  // Add the initial value to the persistent list in the ExpGenerator.
  //
  generator->getExpGenerator()->linkPersistent(this);

  // ok...
  //
  return 0;
}

// ItmWhileFunction::preCodeGen
//
// Nothing to do in preCodeGen for While.
//
ItemExpr *ItmWhileFunction::preCodeGen(Generator *generator) {
  return ItemExpr::preCodeGen(generator);
}

// ItmWhileFunction::codeGen
//
// The While function is for loops that must have their condition evaluated
// before any executions of the body.
//
// The While function evaluates the condition represented by child(1) and if it is true 
// it executes the code represented by child(0). The loop is repeated as long as 
// child(1) is true. The result of the While is the final value of child(0).
//
// The looping is accomplished by inserting a NOOP/unconditional BRANCH pair, and a 
// NOOP/BRANCH pair.  The unconditional branch is evaluated only once and branches 
// immediately to the loop condition for evaluation. 

// The NOOPs are inserted before generating the code for each child and serve
// as branch targets for the loop branch and the undonditional brnach, repsecitevly.
// Between the first NOOP and the BRANCH the body of the loop (child(0)) 
// and the termination condition (child(1)) are repeatedly evaluated. 
//
//  Before code gen:
//
//                      child(0)  <--- loop body
//                      child(1)  <--- loop condition
//
//  After Code gen:
//
//               -----<-unconditional branch
//               |           |
//               |  --->branchTarget (noop)
//               |  |        |
//               |  |   child(0)         <--- loop body
//               |  |        |
//               - -|-> branchTarget2 (noop)
//                  |        |
//                  --<-child(1)          <--- loop condition
//
short ItmWhileFunction::codeGen(Generator * generator) {
  // Get local handles...
  //
  Attributes **attr;
  Space* space = generator->getSpace();
  CollHeap *wHeap = generator->wHeap();
  ExpGenerator *exp = generator->getExpGenerator();

  // If this While has already been codeGenned, then bug out...
  // Otherwise, allocate space for the result if necessary and set
  // attr[0] to point to the result attribute data. Also, mark this
  // node as codeGenned.
  //
  if (exp->genItemExpr(this, &attr, 2, 0) == 1)
    return 0;

  // Insert the unconditional branch to evaluate the condition 
  // before entering the loop.
  //
  ex_branch_clause * startBranchClause
    = new(space) ex_branch_clause(ITM_RETURN_TRUE, space);

  exp->linkClause(this, startBranchClause);

  // Insert the NOOP clause to use as the branch target for the
  // start of the loop body.
  //
  ex_clause * branchTarget = new(space) ex_noop_clause();
  exp->linkClause(this, branchTarget);

  // CodeGen the body of the loop.
  //
  child(0)->codeGen(generator);

  // The result of the While is the result of the body of the loop. Set
  // the src attribute for the convert (added below) to be the body of
  // the while loop. The dst attribute has already been set in genItemExpr().
  //
  attr[1] = generator->getMapInfo
    (child(0)->castToItemExpr()->getValueId())->getAttr();

  // Insert the NOOP clause to use as the branch target for the
  // unconditional branch to the loop condition.
  //

  ex_clause * branchTarget2 = new(space) ex_noop_clause();
  exp->linkClause(this, branchTarget2);

  // CodeGen the loop termination condition.
  //
  child(1)->codeGen(generator);

  // Construct a BRANCH clause to loop back and repeat the expression
  // and condition if the condition evaluates to TRUE.
  //
  Attributes ** branchAttrs = new(wHeap) Attributes*[2];
  Attributes *boolAttr = generator->getMapInfo
    (child(1)->castToItemExpr()->getValueId())->getAttr();
  branchAttrs[0] = boolAttr->newCopy(wHeap);
  branchAttrs[1] = boolAttr->newCopy(wHeap);
  //  branchAttrs[0]->copyLocationAttrs(boolAttr);
  //  branchAttrs[1]->copyLocationAttrs(boolAttr);

  branchAttrs[0]->resetShowplan();
  ex_branch_clause * branchClause
    = new(space) ex_branch_clause(ITM_OR, branchAttrs, space);
  branchClause->set_branch_clause(branchTarget);

  //
  // set the target of the unconditional branch to be the branch clause
  //

  startBranchClause->set_branch_clause(branchTarget2);

  // Insert the branch clause into the expression.
  //
  exp->linkClause(this, branchClause);

  // Allocate a convert clause to move the result from child(0) to the
  // result of this node. This move is necessary so that future
  // side-effects of the result of child(0) -- if it is a local variable,
  // for instance -- will not change the result of the While.
  //
  ex_conv_clause * convClause =
    new(generator->getSpace()) ex_conv_clause
    (getOperatorType(), attr, space);
  exp->linkClause(this, convClause);

  return 0;
}
