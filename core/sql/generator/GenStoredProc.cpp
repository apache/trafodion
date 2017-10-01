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
* File:         GenStoredProc.cpp
* Description:  Stored Proc operator
*
* Created:      6/19/1997
* Language:     C++
*
*
*
*
******************************************************************************
*/
#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "RelStoredProc.h"
#include "BindWA.h"
#include "Generator.h"
#include "GenExpGenerator.h"
//#include "ex_stdh.h"
#include "ExpCriDesc.h"
#include "ComTdb.h"
//#include "ex_tcb.h"
#include "ComTdbStoredProc.h"
#include "exp_clause_derived.h"

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   RelStoredProc::codeGen()
//
//////////////////////////////////////////////////////////////////////
short generateSPIOExpr(RelInternalSP * sp,
		       Generator * generator,
		       ExSPInputOutput * &inputExpr,
		       ExSPInputOutput * &outputExpr)
{
  ExpGenerator * expGen = generator->getExpGenerator();
  Space * genSpace = generator->getSpace();

  // Generate input(extract) expr
  FragmentDir * compFragDir = generator->getFragmentDir();

  // create the fragment (independent code space) for this expression
  CollIndex myFragmentId = compFragDir->pushFragment(FragmentDir::MASTER);
  Space * space = generator->getSpace();

  // Start generation by creating the ExSPInputOutput class.
  //It will be initialized later.
  ExSPInputOutput * lInputExpr = new(space) ExSPInputOutput();

  ULng32 recordLen;
  ExpTupleDesc * tupleDesc = NULL;

  if (expGen->processValIdList(sp->procTypes(),
			       ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			       recordLen,
			       0, 1,
			       &tupleDesc, 
			       ExpTupleDesc::LONG_FORMAT) == -1)
    return -1;

  ConvInstruction * cia = 
    (ConvInstruction *)space->allocateMemory(sp->procTypes().entries() * 
					    sizeof(ConvInstruction));

  ULng32 totalLen = space->getAllocatedSpaceSize();
 
  lInputExpr->initialize(tupleDesc, totalLen, cia);

  ExSPInputOutputPtr(lInputExpr).pack(space);

  // the generated expr is generated in chunks internally by
  // the space class. Make it contiguous by allocating and
  // moving it to a contiguous area.
  char * expr = new(generator->wHeap()) char[totalLen];
  space->makeContiguous((char *)expr, totalLen);
  inputExpr =
    (ExSPInputOutput *)(genSpace->allocateAndCopyToAlignedSpace((char *)expr, totalLen, 0));

  compFragDir->removeFragment();

  // Delete expr
  NADELETEBASIC(expr, generator->wHeap());
  expr = NULL;

  // Now generate the move to output row expr
  myFragmentId = compFragDir->pushFragment(FragmentDir::MASTER);
  space = generator->getSpace();

  ExSPInputOutput * lOutputExpr = new(space) ExSPInputOutput();

  if (expGen->processValIdList(sp->getTableDesc()->getColumnList(),
			       ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			       recordLen,
			       0, 1,
			       &tupleDesc, 
			       ExpTupleDesc::LONG_FORMAT) == -1)
    return -1;

  cia = 
    (ConvInstruction *)space->allocateMemory(sp->getTableDesc()->getColumnList().entries() * 
					    sizeof(ConvInstruction));

  for (short i = 0; i < (short) tupleDesc->numAttrs(); i++)
    {
      ex_conv_clause tempClause;

      cia[i] = tempClause.findInstruction(REC_BYTE_V_ASCII, 
					  -1, // no op 
					  tupleDesc->getAttr(i)->getDatatype(), 
					  tupleDesc->getAttr(i)->getLength(),
					  0);
      
    }

  totalLen = space->getAllocatedSpaceSize();
 
  lOutputExpr->initialize(tupleDesc, totalLen, cia);

  ExSPInputOutputPtr(lOutputExpr).pack(space);

  expr = new(generator->wHeap()) char[totalLen];
  space->makeContiguous((char *)expr, totalLen);
  outputExpr =
    (ExSPInputOutput *)(genSpace->allocateAndCopyToAlignedSpace((char *)expr, totalLen, 0));

  compFragDir->removeFragment();

  // Delete expr
  NADELETEBASIC(expr, generator->wHeap());
  expr = NULL;
  
  return 0;
}

short RelInternalSP::codeGen(Generator * generator)
{
  Space * space          = generator->getSpace();
  ExpGenerator * exp_gen = generator->getExpGenerator();
  MapTable * last_map_table = generator->getLastMapTable();

  ex_expr * input_expr  = NULL;
  ex_expr * output_expr = NULL;

  ////////////////////////////////////////////////////////////////////////////
  //
  // Returned atp layout:
  //
  // |--------------------------------|
  // | input data  |  stored proc row |
  // | ( I tupps ) |  ( 1 tupp )      |
  // |--------------------------------|
  // <-- returned row to parent ---->
  //
  // input data:        the atp input to this node by its parent. 
  // stored proc row:   tupp where the row read from SP is moved.
  //
  ////////////////////////////////////////////////////////////////////////////

  ex_cri_desc * given_desc 
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returned_desc 
    = new(space) ex_cri_desc(given_desc->noTuples() + 1, space);
 
  // cri descriptor for work atp has 3 entries:
  // -- the first two entries for consts and temps.
  // -- Entry 3(index #2) is where the input and output rows will be created.
  ex_cri_desc * work_cri_desc = new(space) ex_cri_desc(3, space);
  const Int32 work_atp = 1;
  const Int32 work_atp_index = 2;
 
  ExpTupleDesc * input_tuple_desc = NULL;
  ExpTupleDesc * output_tuple_desc = NULL;

  // Generate expression to create the input row that will be
  // given to the stored proc.
  // The input value is in sp->getProcAllParams() 
  // and has to be converted to sp->procType().
  // Generate Cast node to convert procParam to ProcType.
  // If procType is a varchar, explode it. This is done
  // so that values could be extracted correctly.
  ValueIdList procVIDList;
  for (CollIndex i = 0; i < procTypes().entries(); i++)
    {
      Cast * cn;

      if ((procTypes())[i].getType().getVarLenHdrSize() > 0) 
	{

// 5/9/98: add support for VARNCHAR
          const CharType& char_type =
                (CharType&)((procTypes())[i].getType());

	  // Explode varchars by moving them to a fixed field
	  // whose length is equal to the max length of varchar.
	  cn = new(generator->wHeap()) 
	    Cast ((getProcAllParamsVids())[i].getItemExpr(), 
		  (new(generator->wHeap())
		   SQLChar(generator->wHeap(),
		           CharLenInfo(char_type.getStrCharLimit(), char_type.getDataStorageSize()),
			   char_type.supportsSQLnull(),
			   FALSE, FALSE, FALSE,
			   char_type.getCharSet(),
			   char_type.getCollation(),
			   char_type.getCoercibility()
/*
                           (procTypes())[i].getType().getNominalSize(),
			   (procTypes())[i].getType().supportsSQLnull()
*/
                          )
                  )
                 );
	  
	  // Move the exploded field to a varchar field since 
	  // procType is varchar.
	  // Can optimize by adding an option to convert node to
	  // blankpad. TBD.
	  //
	  cn = new(generator->wHeap())
	    Cast(cn, &((procTypes())[i].getType()));
	} 
      else
 	cn = new(generator->wHeap()) Cast((getProcAllParamsVids())[i].getItemExpr(),
					  &((procTypes())[i].getType()));

      cn->bindNode(generator->getBindWA());
      procVIDList.insert(cn->getValueId());
    }

  ULng32 inputRowlen_ = 0;
  exp_gen->generateContiguousMoveExpr(procVIDList, -1, /*add conv nodes*/
				      work_atp, work_atp_index,
				      ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
				      inputRowlen_,
				      &input_expr,
				      &input_tuple_desc,
				      ExpTupleDesc::LONG_FORMAT);

  // add all columns from this SP to the map table. 
  ULng32 tupleLength;
  exp_gen->processValIdList(getTableDesc()->getColumnList(),
			    ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			    tupleLength,
			    work_atp, 
			    work_atp_index);
 
  // Generate expression to move the output row returned by the
  // stored proc back to parent.
  ULng32 outputRowlen_ = 0;
  MapTable * returnedMapTable = 0;
  exp_gen->generateContiguousMoveExpr(getTableDesc()->getColumnList(),
				      -1 /*add conv nodes*/,
				      0, returned_desc->noTuples() - 1,
				      ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
				      outputRowlen_,
				      &output_expr,
				      &output_tuple_desc,
				      ExpTupleDesc::LONG_FORMAT,
				      &returnedMapTable);
 
  // Now generate expressions used to extract or move input or
  // output values. See class ExSPInputOutput.
  ExSPInputOutput * extractInputExpr = NULL;
  ExSPInputOutput * moveOutputExpr = NULL;
  
  generateSPIOExpr(this, generator,
		   extractInputExpr,
		   moveOutputExpr);

  // done with expressions at this operator. Remove the appended map tables.
  generator->removeAll(last_map_table);

  // append the map table containing the returned columns
  generator->appendAtEnd(returnedMapTable);

  NAString procNameAsNAString(procName_);
  char * sp_name = 
    space->allocateAndCopyToAlignedSpace(procNameAsNAString,
					 procNameAsNAString.length(), 0);

  ExpGenerator *expGen = generator->getExpGenerator();

  // expression to conditionally return 0 or more rows.
  ex_expr *predExpr = NULL;

  // generate tuple selection expression, if present
  if(NOT selectionPred().isEmpty())
  {
    ItemExpr* pred = selectionPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
    expGen->generateExpr(pred->getValueId(),ex_expr::exp_SCAN_PRED,&predExpr);
  }

  ComTdbStoredProc * sp_tdb = new(space)
    ComTdbStoredProc(sp_name, 
		     input_expr,
		     inputRowlen_,
		     output_expr,
		     outputRowlen_,
		     work_cri_desc,
		     work_atp_index,
		     given_desc,
		     returned_desc,
		     extractInputExpr,
		     moveOutputExpr,
		     2,
		     1024,
		     (Cardinality) getGroupAttr()->
		     getOutputLogPropList()[0]->
		     getResultCardinality().value(),
		     5,
		     64000,  //10240
		     predExpr,
		     (UInt16) arkcmpInfo_);
		      
  generator->initTdbFields(sp_tdb);

  if(!generator->explainDisabled()) 
    {
      generator->setExplainTuple(
				 addExplainInfo(sp_tdb, 0, 0, generator));
    }
  // Do not infer that any transaction started can 
  // be in READ ONLY mode if ISPs are present.
  generator->setNeedsReadWriteTransaction(TRUE);

  generator->setCriDesc(given_desc, Generator::DOWN);
  generator->setCriDesc(returned_desc, Generator::UP);
  generator->setGenObj(this, sp_tdb);

  // Some built-in functions require a TMF transaction
  // because they get their information from catman
  generator->setTransactionFlag(getRequiresTMFTransaction());

  return 0;
}

