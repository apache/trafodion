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
* File:         GenRelSet.C
* Description:  Union operators
*               
*               
* Created:      5/17/94
* Language:     C++
*
*
*
*
******************************************************************************
*/
#include "ComOptIncludes.h"
#include "GroupAttr.h"
#include "RelSet.h"
#include "Generator.h"
#include "GenExpGenerator.h"
//#include "ex_stdh.h"
#include "ExpCriDesc.h"
#include "ComTdb.h"
//#include "ex_tcb.h"
#include "ComTdbUnion.h"
#include "DefaultConstants.h"
#include "sql_buffer.h"
#include "sql_buffer_size.h"

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   MergeUnion::codeGen()
//
/////////////////////////////////////////////////////////////////////


short MergeUnion::codeGen(Generator * generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  Space * space = generator->getSpace();
  
  MapTable * my_map_table = generator->appendAtEnd();

  ////////////////////////////////////////////////////////////////////////////
  //
  // Layout at this node:
  //
  // |------------------------------------------------------------------------|
  // | input data  |  Unioned data | left child's data | right child's data   |
  // | ( I tupps ) |  ( 1 tupp )   | ( L tupps )       |  ( R tupp )          |
  // |------------------------------------------------------------------------|
  // <-- returned row to parent --->
  // <------------ returned row from left child ------->
  // <-------------------- returned row from right child --------------------->
  //
  // input data:        the atp input to this node by its parent. 
  // unioned data:      tupp where the unioned result is moved
  // left child data:   tupps appended by the left child
  // right child data:  tupps appended by right child
  //
  // Input to left child:    I + 1 tupps
  // Input to right child:   I + 1 + L tupps
  //
  // Tupps returned from left and right child are only used to create the
  // unioned data. They are not returned to parent.
  //
  ////////////////////////////////////////////////////////////////////////////

  ex_cri_desc * given_desc
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returned_desc = NULL;
  if(child(0) || child(1))
    returned_desc = new(space) ex_cri_desc(given_desc->noTuples() + 1, space);
  else
    returned_desc = given_desc;

  // expressions to move the left and right child's output to the
  // unioned row.
  ex_expr * left_expr = 0;
  ex_expr * right_expr = 0;

  // expression to compare left and right child's output to
  // evaluate merge union.
  ex_expr * merge_expr = 0;

  // Expression to conditionally execute the left or right child.
  ex_expr *cond_expr = NULL;

  // Expression to handle triggered action excpetion 
  ex_expr *trig_expr = NULL;

  // It is OK for neither child to exist when generating a merge union TDB
  // for index maintenenace. The children are filled in at build time.
  //
  GenAssert((child(0) AND child(1)) OR (NOT child(0) AND NOT (child(1))),
	    "MergeUnion -- missing one child");
  ComTdb * left_child_tdb = NULL;
  ComTdb * right_child_tdb = NULL;
  ExplainTuple *leftExplainTuple = NULL;
  ExplainTuple *rightExplainTuple = NULL;
  NABoolean afterUpdate = FALSE;
  NABoolean rowsFromLeft = TRUE;
  NABoolean rowsFromRight = TRUE;

  if(child(0) && child(1)) {
 
    // if an update operation is found  before the execution of the
  // IF statement, set afterUpdate to 1 indicating that an update operation
  // was performed before the execution of the IF statement.  Which 
  // is used at runtime to decide whether to set rollbackTransaction in the 
  // diagsArea

 
  if (generator->updateWithinCS() && getUnionForIF()) {
    afterUpdate = TRUE;
  }

    // generate the left child
    generator->setCriDesc(returned_desc, Generator::DOWN);
    child(0)->codeGen(generator);
    left_child_tdb = (ComTdb *)(generator->getGenObj());
    leftExplainTuple = generator->getExplainTuple();

  // MVs --
  // If the left child does not have any outputs, don't expect any rows.
  if (child(0)->getGroupAttr()->getCharacteristicOutputs().isEmpty())
    rowsFromLeft = FALSE;

  // if an update operation is found in the left subtree of this Union then
  // set rowsFromLeft to 0 which is passed on to execution tree indicating
  // that this Union node is not  expecting rows from the left child, then
  // foundAnUpdate_ is reset so it can be reused while doing codGen() on 
  // the right sub tree

    if (getUnionForIF()) {
      if (! getCondEmptyIfThen()) {
        if (generator->foundAnUpdate())   {
          rowsFromLeft = FALSE;
          generator->setFoundAnUpdate(FALSE);
        } 
      } 
      else {
        rowsFromLeft = FALSE;  
      }
    }
    
    // descriptor returned by left child is given to right child as input.
    generator->setCriDesc(generator->getCriDesc(Generator::UP), 
			  Generator::DOWN);
    child(1)->codeGen(generator);
    right_child_tdb = (ComTdb *)(generator->getGenObj());
    rightExplainTuple = generator->getExplainTuple();

  // MVs
  // If the right child does not have any outputs, don't expect any rows.
  if (child(1)->getGroupAttr()->getCharacteristicOutputs().isEmpty())
    rowsFromRight = FALSE;

  // if an update operation is found in the right subtree of this CS then
  // set rowsFromRight to 0 which is passed on to execution tree indicating
  // that this CS node is not  expecting rows from the right child, then
  // foundAnUpdate_ is reset so it can be reused while doing codGen() on
  // the left or right child of another CS node


  if (getUnionForIF()) {
    if (! getCondEmptyIfElse()) {
      if (generator->foundAnUpdate())  {
        rowsFromRight = FALSE;
      } 
    } 
    else {
      rowsFromRight = FALSE;  
    }


    // we cannot always expect a row from a conditional operator. If it is an
    // IF statement without an ELSE and the condition fails then we do not get
    // any rows back. So we allow a conditional union operator to handle all
    // errors below it and for the purposes of 8015 error / 8014 warning
    // treat it as an update node. In this way the nodes above it do not expect
    // any row from this child and do not raise an error if no row is returned.
    // 8014/8015 type errors within this IF statement are handled as in any
    // regular CS.

    generator->setFoundAnUpdate(TRUE);
  }

  }

  // Create the unioned row. 
  // colMapTable() is a list of ValueIdUnion nodes where each node points to
  // the corresponding left and the right output entries.
  // Generate expressions to move the left and right child's output to
  // the unioned row. 
  ValueIdList left_val_id_list;
  ValueIdList right_val_id_list;
  CollIndex   i;                                      

  for (i = 0; i < colMapTable().entries(); i++)       
    {
      ValueIdUnion * vidu_node = (ValueIdUnion *)(((colMapTable()[i]).getValueDesc())->getItemExpr());

      Cast * cnode;
      if (vidu_node->getResult().getType().getTypeQualifier() != NA_ROWSET_TYPE) {
        // move left child's output to result. The 'type' of Cast result is same
        // as that of the vidu_node.
        cnode = new(generator->wHeap())
	           Cast(((vidu_node->getLeftSource()).getValueDesc())->getItemExpr(),
	                &(vidu_node->getResult().getType()));
      }
      else {
	// We indicate that the whole array is to be copied
	SQLRowset *rowsetInfo = (SQLRowset *) &(vidu_node->getResult().getType());
        SQLRowset *newRowset =  new (generator->wHeap()) 
	                         SQLRowset(generator->wHeap(), rowsetInfo->getElementType(),
	                                   rowsetInfo->getMaxNumElements(),
                                           rowsetInfo->getNumElements());
	newRowset->useTotalSize() = TRUE;
        cnode = new(generator->wHeap())
	           Cast(((vidu_node->getLeftSource()).getValueDesc())->getItemExpr(),
	                newRowset);
      }

      cnode->bindNode(generator->getBindWA());
      
      left_val_id_list.insert(cnode->getValueId());

      if (vidu_node->getResult().getType().getTypeQualifier() != NA_ROWSET_TYPE) {
        // move left child's output to result. The 'type' of Cast result is same
        // as that of the vidu_node.
        cnode = new(generator->wHeap())
	           Cast(((vidu_node->getRightSource()).getValueDesc())->getItemExpr(),
	                &(vidu_node->getResult().getType()));
      }
      else {
	// We indicate that the whole array is to be copied
	SQLRowset *rowsetInfo = (SQLRowset *) &(vidu_node->getResult().getType());
        SQLRowset *newRowset =  new (generator->wHeap()) 
	                         SQLRowset(generator->wHeap(), rowsetInfo->getElementType(),
	                                   rowsetInfo->getMaxNumElements(),
                                           rowsetInfo->getNumElements());
	newRowset->useTotalSize() = TRUE;
        cnode = new(generator->wHeap())
	           Cast(((vidu_node->getRightSource()).getValueDesc())->getItemExpr(),
	                newRowset);
      }

      cnode->bindNode(generator->getBindWA());
      right_val_id_list.insert(cnode->getValueId());
    }
  
  ExpTupleDesc * tuple_desc = 0;
  ULng32 tuple_length = 0;
  if(child(0) && child(1)) {
    exp_gen->generateContiguousMoveExpr(left_val_id_list,
					0, // don't add convert nodes
					1, returned_desc->noTuples() - 1,
					ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
					tuple_length,
					&left_expr,
					&tuple_desc,
					ExpTupleDesc::SHORT_FORMAT);
  

    exp_gen->generateContiguousMoveExpr(right_val_id_list,
					0, // don't add convert nodes
					1, returned_desc->noTuples() - 1,
					ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
					tuple_length,
					&right_expr);
  }
  
  // add value ids for all vidu_nodes to my map table. This is the
  // the map table that will be returned. The attributes of the value ids
  // are same as that of left(or right) expression outputs.
  for (i = 0; i < colMapTable().entries(); i++)
    {
      ValueIdUnion * vidu_node = (ValueIdUnion *)(((colMapTable()[i]).getValueDesc())->getItemExpr());
      
      Attributes * attr =
	generator->addMapInfoToThis(my_map_table, vidu_node->getValueId(), 
				    generator->getMapInfo(left_val_id_list[i])->getAttr())->getAttr();
      attr->setAtp(0);
    }
  
  // describe the returned unioned row
  returned_desc->setTupleDescriptor(returned_desc->noTuples() - 1, tuple_desc);
  
  // if sort-merge union is being done, generate expression to
  // compare the left and the right values.
  // This predicate should return TRUE if the left value is
  // less than the right value.
  merge_expr = 0;
  if (getMergeExpr()) {
    // generate the merge predicate. 
    ItemExpr * mergeExpr = new(generator->wHeap()) BoolResult(getMergeExpr());
    mergeExpr->bindNode(generator->getBindWA());    
      
    exp_gen->generateExpr(mergeExpr->getValueId(),
			  ex_expr::exp_SCAN_PRED,
			  &merge_expr);   
  }

  // If conditional union, generate conditional expression, and ignore
  // right child if it was just being used as a no-op.
  cond_expr = 0;
  if (NOT condExpr().isEmpty()) {
    ItemExpr *condExp = condExpr().rebuildExprTree(ITM_AND, TRUE, TRUE);

    exp_gen->generateExpr(condExp->getValueId(), 
                          ex_expr::exp_SCAN_PRED, 
		          &cond_expr);
  }

  // If conditional union, generate triggered action exception error 
  if (NOT trigExceptExpr().isEmpty()) {
    ItemExpr *trigExp = trigExceptExpr().rebuildExprTree(ITM_AND, TRUE, TRUE);

    exp_gen->generateExpr(trigExp->getValueId(), 
                          ex_expr::exp_SCAN_PRED, 
		          &trig_expr);
  }

  // remove both children's map table. Nothing from child's context
  // should be visible from here on upwards.
  generator->removeAll(my_map_table);

  // Ensure the default buffer size is at least as large as the unioned output
  // row.
  UInt32 outputBuffSize = MAXOF( getDefault(GEN_UN_BUFFER_SIZE),
                                 tuple_length );
  outputBuffSize = SqlBufferNeededSize( 1,                 // # of tuples
                                        outputBuffSize,
                                        SqlBuffer::NORMAL_
                                        );

  ComTdbUnion * union_tdb 
    = new(space) ComTdbUnion(
			     left_child_tdb,
			     right_child_tdb,
			     left_expr,
			     right_expr,
			     merge_expr,
			     cond_expr,
			     trig_expr,
                             tuple_length, // unioned rowlen
			     returned_desc->noTuples()-1, // tupp index for
			                                  // unioned buffer
			     given_desc,
			     returned_desc, 
			     (queue_index)getDefault(GEN_UN_SIZE_DOWN),
			     (queue_index)getDefault(GEN_UN_SIZE_UP),
			     (Cardinality) (getInputCardinality() * getEstRowsUsed()).getValue(),
			     getDefault(GEN_UN_NUM_BUFFERS),
			     outputBuffSize,
			     getOrderedUnion(),
                             getBlockedUnion(),  //++ Triggers -
                             hasNoOutputs(),     //++ Triggers -
                             rowsFromLeft,
                             rowsFromRight,
                             afterUpdate,
			     getInNotAtomicStatement());

  generator->initTdbFields(union_tdb);

  // If it does not have two children, this is index maintenance code and
  // should not be Explained  
  if (!generator->explainDisabled()) {
    generator->setExplainTuple(addExplainInfo(union_tdb,
					    leftExplainTuple,
					    rightExplainTuple,
					    generator));
  }

  // restore the original down cri desc since this node changed it.
  generator->setCriDesc(given_desc, Generator::DOWN);

  // set the new up cri desc.
  generator->setCriDesc(returned_desc, Generator::UP);

  generator->setGenObj(this, union_tdb);

  return 0;  
}

short Union::codeGen(Generator *)
{
  return -1;
}

